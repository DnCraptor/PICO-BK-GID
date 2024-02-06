// ScriptRunner.cpp: implementation of the CScriptRunner class.
//


#include "pch.h"
#include "ScriptRunner.h"
#include "Board.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

// замедлитель. чтобы передаваемые символы не терялись
constexpr auto TICK_DELAY = 70;
// задержка начала выполнения скрипта в миллисекундах
constexpr auto SCRIPT_START_DELAY = 1000;

// управляющие символы
constexpr TCHAR SCRIPT_PREFIX = _T('|');
constexpr TCHAR SCRIPT_SET_DELAY = _T('#'); //|#
constexpr TCHAR SCRIPT_STOP_KEY  = _T('^'); //|^
constexpr TCHAR SCRIPT_AR2_KEY   = _T('!'); //|!
// |0 .. |9 - индексы аргументов из списка аргументов

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CScriptRunner::CScriptRunner()
	: m_pBoard(nullptr)
	, m_nScriptLinePos(0)
	, m_nScriptLineLen(0)
	, m_nScriptCurrTick(0)
	, m_nTickDelay(SCRIPT_START_DELAY)
	, m_bStopButton(false)
	, m_bRus(false)
	, m_bIsAr2Press(false)
	, m_bHasScript(false)
{
}

CScriptRunner::~CScriptRunner()
{
	if (m_fileScript.m_o)
	{
		m_fileScript.Close();
	}

	m_listArgs.RemoveAll();
}


void CScriptRunner::SetScript(const fs::path &strScriptPath, const fs::path &strScriptFileName, bool bXlat)
{
	if (m_fileScript.m_o)
	{
		m_fileScript.Close();
	}

	m_strScriptFile = strScriptPath / strScriptFileName;
	m_bHasScript = !strScriptFileName.empty();
	m_bRus = bXlat;
}

void CScriptRunner::StopScript()
{
	SetScript(_T(""), _T(""), false);
	m_listArgs.RemoveAll();
}

void CScriptRunner::SetArgumentList(ScriptArgList &ArgList)
{
	m_listArgs.AddTail(&ArgList);
}


void CScriptRunner::SetArgument(CString &strArg)
{
	m_listArgs.AddTail(strArg);
}

bool CScriptRunner::RunScript()
{
	ASSERT(m_pBoard);

	if (m_bHasScript && m_pBoard)   // Если скрипт задан, и если есть эмулируемое устройство
	{
		if (!m_fileScript.m_o) // если файл ещё не открыт, попробуем открыть
		{
			// если файл не открыт, откроем его
			if (!m_fileScript.Open(m_strScriptFile.c_str(), CFile::modeRead))
			{
				// Если файл скрипта не открывается, то реинициализируем обработчик
				// и выйдем с ошибкой
				StopScript();
				return false;
			}

			// Если скрипт открылся, инициализируем переменные
			m_strScriptLine.Empty();
			m_nScriptLinePos = m_nScriptLineLen = 0;
			m_nScriptCurrTick = GetTickCount(); // Запускаем таймер
			m_nTickDelay = SCRIPT_START_DELAY;
		}

		if (m_bStopButton)
		{
			m_bStopButton = false;
			m_pBoard->UnStopInterrupt();
		}

		m_pBoard->m_reg177716in |= 0100;    // снимем флаг нажатия клавиши

		// Если скрипт открыт и всё в порядке
		// Ждём определённое время перед запуском скрипта
		if ((GetTickCount() - m_nScriptCurrTick) < m_nTickDelay)
		{
			return true;
		}

		m_nScriptCurrTick = GetTickCount();
		m_nTickDelay = TICK_DELAY;

		if (m_nScriptLinePos < m_nScriptLineLen) // Если есть необработанные символы в строке
		{
			if (!CheckEscChar(m_strScriptLine[m_nScriptLinePos]))
			{
				// Передадим новый символ только если регистр БК 177660 готов
				// т.е. ждём, пока БК прочтёт следующий код из 177662
				if (!(m_pBoard->m_reg177660 & 0200))
				{
					// TRACE1("script send char: %c\n", m_strScriptLine[m_nScriptLinePos]);
					ParseNextChar(m_strScriptLine[m_nScriptLinePos++]);
				}
			}
		}
		else
		{
			// Если дошли до конца текущей строки,
			m_nScriptLinePos = 0;

			// прочитаем следующую
			if (!m_fileScript.ReadString(m_strScriptLine))
			{
				// Если строки закончились,
				// закроем файл и переинициализируем переменные
				StopScript();
				return false;
			}

			m_strScriptLine.TrimRight(_T('\r'));
			m_strScriptLine += _T('\n'); // добавим перевод каретки в конец строки
			m_nScriptLineLen = m_strScriptLine.GetLength();
		}

		return true;
	}

	return false;
}

// Проверка управляющих символов
// выход: true - управляющий символ обработан, и нужно выйти из обработчика
// false - нужно продолжить
bool CScriptRunner::CheckEscChar(TCHAR ch)
{
	if (ch == SCRIPT_PREFIX)
	{
		m_nScriptLinePos++; // пропускаем управляющий символ
		ch = m_strScriptLine[m_nScriptLinePos++]; // смотрим следующий

		if (ch == SCRIPT_PREFIX)
		{
			// это значит, что надо передать знак управляющего символа в качестве обычного символа
			m_nScriptLinePos--;
			return false;
		}

		if (ch == SCRIPT_SET_DELAY) // это снова надо установить задержку
		{
			m_nScriptCurrTick = GetTickCount();
			m_nTickDelay = SCRIPT_START_DELAY;
			return true;
		}

		if (ch == SCRIPT_STOP_KEY) // эмуляция кнопки СТОП
		{
			m_bStopButton = true;
			m_pBoard->StopInterrupt();
			return true;
		}

		if (ch == SCRIPT_AR2_KEY) // нажатие кнопки АР2
		{
			m_bIsAr2Press = true;
			return true;
		}

		// для начала будем обрабатывать только 10 аргументов, если будет мало,
		// то надо будет усложнять и парсить число.
		if (_T('0') <= ch && ch <= _T('9')) // обработка аргумента
		{
			int index = ch - _T('0');

			if (index < m_listArgs.GetCount())
			{
				POSITION pos = m_listArgs.FindIndex(index);

				if (pos)
				{
					CString strArg = m_listArgs.GetAt(pos);

					if (!strArg.IsEmpty())
					{
						m_strScriptLine.Insert(m_nScriptLinePos, strArg);
						m_nScriptLineLen = m_strScriptLine.GetLength();
						return false;
					}
				}
			}

			return true;
		}
	}

	return false;
}

void CScriptRunner::ParseNextChar(TCHAR ch)
{
	uint8_t koi_ch = Global::WIDEtoBKChar(ch);

	if (!m_bRus && koi_ch >= 0300) // если буквы русские, а режим - не русский
	{
		m_nScriptLinePos--; // откатим позицию
		koi_ch = BKKEY_RUS; // пошлём код переключения языка
		m_bRus = true; // включаем русский режим
	}
	else if (m_bRus && (0100 <= koi_ch && koi_ch <= 0177)) // если буквы не русские, а режим - русский
	{
		m_nScriptLinePos--; // откатим позицию
		koi_ch = BKKEY_LAT; // пошлём код переключения языка
		m_bRus = false; // выключаем русский режим
	}

	m_pBoard->m_reg177662in = koi_ch & 0177; // Отправим код символа в 177662
	m_pBoard->m_reg177716in &= ~0100;    // Установим флаг нажатия клавиши в 177716
	m_pBoard->KeyboardInterrupt(m_bIsAr2Press ? INTERRUPT_274 : INTERRUPT_60);
	m_bIsAr2Press = false;
}

