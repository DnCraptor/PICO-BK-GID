#include "pch.h"

#include "Reader.h"
#include "BKToken.h"
#include "Globals.h"

#pragma warning(disable:4996)


const uint8_t CReader::table_2s[128] =
{
	0xFF, 0xFF, 0xFF, 0xC7, 0xFE, 0xBE, 0xF7, 0xFB,
	0xFD, 0xBF, 0xF7, 0xF9, 0xFC, 0xBE, 0xF1, 0x80, 0xFF, 0xFF, 0xF7, 0xBB, 0xFF, 0xFF, 0xFF,
	0xCF, 0xDE, 0xBF, 0xD1, 0x08, 0xFF, 0xBF, 0xF1, 0xBF, 0xFF, 0xFF, 0xFF, 0xC7, 0x1D, 0x3F,
	0x7F, 0x81, 0xA7, 0xB6, 0xF2, 0x82, 0xFF, 0xFF, 0x75, 0xDB, 0xFC, 0xBF, 0xD7, 0x9D, 0xFF,
	0xAE, 0xFB, 0xDF, 0xFF, 0xFF, 0xFF, 0xC7, 0x84, 0xB7, 0xF3, 0x9F, 0xFF, 0xFF, 0xFF, 0xDB,
	0xFF, 0xBF, 0xFF, 0xFF, 0xFD, 0xBF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE7, 0xC7, 0x84, 0x9E, 0xF0,
	0x12, 0xBC, 0xBF, 0xF0, 0x84, 0xA4, 0xBA, 0x10, 0x10, 0xA4, 0xBE, 0xB8, 0x88, 0xAC, 0xBF,
	0xF7, 0x0A, 0x84, 0x86, 0x90, 0x08, 0x04, 0x00, 0x00, 0x03, 0x7F, 0xFD, 0xF7, 0xC1, 0x7D,
	0xAE, 0x6F, 0xCB, 0x15, 0x3D, 0xFC, 0x00, 0x7F, 0x7D, 0xE7, 0xC2, 0x7F, 0xFD, 0xF7, 0xC3
};


CReader::CReader(const fs::path &strFileName, const FILE_CHARSET nInputCharset)
	: m_strFileName(strFileName)
	, m_chCurrent(0)
	, m_chNext(0)
	, m_nCurrCharType(CHAR_TYPE::LN)
	, m_file(nullptr)
	, m_nCharset(nInputCharset)
	, m_nLineNum(0)
	, m_nLen(0)
	, m_nPos(-1)
{
	const FILE_CHARSET nDetectedCharset = AnalyseCharset(strFileName);

	if (nDetectedCharset == FILE_CHARSET::FILEERROR)
	{
		m_nCharset = nDetectedCharset;
		return;
	}

	// если кодировку вручную не задавали
	if (m_nCharset == FILE_CHARSET::UNDEFINE)
	{
		// то будем использовать автоопределённую.
		m_nCharset = nDetectedCharset;
	}
	else if (m_nCharset == FILE_CHARSET::UTF8 && nDetectedCharset == FILE_CHARSET::UTF16LE)
	{
		m_nCharset = nDetectedCharset; // уточняем юникод
	}
	else if (m_nCharset == FILE_CHARSET::UTF16LE && nDetectedCharset == FILE_CHARSET::UTF8)
	{
		m_nCharset = nDetectedCharset; // уточняем юникод
	}

	// а иначе считаем, что пользователю лучше знать, какую он использует кодировку.

	switch (m_nCharset)
	{
		case FILE_CHARSET::KOI8:
			m_file = _wfopen(strFileName.c_str(), L"rb"); // чтобы читать всю строку зараз.
			break;

		case FILE_CHARSET::CP866:
			m_file = _wfopen(strFileName.c_str(), L"rb");
			break;

		case FILE_CHARSET::CP1251:
			m_file = _wfopen(strFileName.c_str(), L"rb");
			break;

		case FILE_CHARSET::UTF8:
			m_file = _wfopen(strFileName.c_str(), L"rb");
			// skip BOM
			{
				uint8_t BOM[3];
				fread(BOM, 1, 3, m_file);

				if (BOM[0] == 0xEF && BOM[1] == 0xBB && BOM[2] == 0xBF)
				{
					// пропускаем BOM
				}
				else
				{
					fseek(m_file, 0, SEEK_SET);
				}
			}
			break;

		case FILE_CHARSET::UTF16LE:
			m_file = _wfopen(strFileName.c_str(), L"rb,ccs=UTF-16LE");
			// skip BOM
			{
				uint8_t BOM[2];
				fread(BOM, 1, 2, m_file);

				if (BOM[0] == 0xFF && BOM[1] == 0xFE)
				{
					// пропускаем BOM
				}
				else
				{
					fseek(m_file, 0, SEEK_SET);
				}
			}
			break;

		default:
			return;
	}

	readChar();
}


CReader::~CReader()
{
	if (m_file)
	{
		fclose(m_file);
	}

	if (m_file)
	{
		fclose(m_file);
	}
}

wchar_t CReader::getChar()
{
	if ((m_nPos == -1 || m_nPos >= m_nLen) && !feof(m_file))
	{
		m_nLineNum++;
		getString(); // формируем текущую строку, для вывода на экран в сообщении об ошибке
	}

	// после чтения строки m_nPos и m_nLen могут измениться, поэтому надо проверять заново
	if (feof(m_file) && m_nPos >= m_nLen)
	{
		return 032;
	}

	return m_strCur[m_nPos++];
}

constexpr auto BUFFER_SIZE = 65536;


void CReader::getString()
{
	auto wch = std::make_unique<wchar_t[]>(BUFFER_SIZE);

	if (wch)
	{
		size_t len = 0;
		memset(wch.get(), 0, BUFFER_SIZE * sizeof(wchar_t));

		switch (m_nCharset)
		{
			case FILE_CHARSET::KOI8:
			{
				auto cch = std::make_unique<char[]>(BUFFER_SIZE);

				if (cch)
				{
					memset(cch.get(), 0, BUFFER_SIZE);
					char *str = fgets(cch.get(), BUFFER_SIZE - 1, m_file);
					len = (str) ? strlen(str) : 0;

					if ((len > 1) && (str[len - 2] == '\r'))
					{
						str[--len] = 0;
						str[len - 1] = '\n';
					}

					for (size_t i = 0; i < len; ++i)
					{
						uint8_t b = str[i];
						wch[i] = (b < 128) ? wchar_t(b) : koi8tbl[b - 128];
					}
				}

				break;
			}

			case FILE_CHARSET::CP866:
			{
				auto cch = std::make_unique<char[]>(BUFFER_SIZE);

				if (cch)
				{
					memset(cch.get(), 0, BUFFER_SIZE);
					char *str = fgets(cch.get(), BUFFER_SIZE - 1, m_file);

					if (str)
					{
						len = strlen(str);

						if ((len > 1) && (str[len - 2] == '\r'))
						{
							str[--len] = 0;
							str[len - 1] = '\n';
						}

						MultiByteToWideChar(CP_OEMCP, 0, str, static_cast<DWORD>(len), reinterpret_cast<LPWSTR>(wch.get()), static_cast<int>(len));
					}
				}

				break;
			}

			case FILE_CHARSET::CP1251:
			{
				auto cch = std::make_unique<char[]>(BUFFER_SIZE);

				if (cch)
				{
					memset(cch.get(), 0, BUFFER_SIZE);
					char *str = fgets(cch.get(), BUFFER_SIZE - 1, m_file);

					if (str)
					{
						len = strlen(str);

						if ((len > 1) && (str[len - 2] == '\r'))
						{
							str[--len] = 0;
							str[len - 1] = '\n';
						}

						MultiByteToWideChar(CP_ACP, 0, str, static_cast<DWORD>(len), reinterpret_cast<LPWSTR>(wch.get()), static_cast<int>(len));
					}
				}

				break;
			}

			case FILE_CHARSET::UTF8:
			{
				auto cch = std::make_unique<char[]>(BUFFER_SIZE);

				if (cch)
				{
					memset(cch.get(), 0, BUFFER_SIZE);
					char *str = fgets(cch.get(), BUFFER_SIZE - 1, m_file);

					if (str)
					{
						len = strlen(str);

						if ((len > 1) && (str[len - 2] == '\r'))
						{
							str[--len] = 0;
							str[len - 1] = '\n';
						}

						MultiByteToWideChar(CP_UTF8, 0, str, static_cast<DWORD>(len), reinterpret_cast<LPWSTR>(wch.get()), static_cast<int>(len));
						len = wcslen(wch.get());
					}
				}

				break;
			}

			case FILE_CHARSET::UTF16LE:
				fgetws(wch.get(), BUFFER_SIZE - 1, m_file);
				len = wcslen(wch.get());

				if ((len > 1) && (wch[len - 2] == '\r'))
				{
					wch[--len] = 0;
					wch[len - 1] = '\n';
				}

				break;

			default:
				assert(false);
		}

		if (feof(m_file))
		{
			if (wch[len - 1] != L'\n')
			{
				wch[len++] = L'\n';
			}
		}

		wch[len] = 0;
		m_strCur = std::wstring(wch.get());
		m_nLen = len;
		m_nPos = 0;
	}
	else
	{
		m_strCur.clear();
	}
}


void CReader::StoreState()
{
	RdrState s
	{
		m_chCurrent,
		m_chNext,
		m_nCurrCharType,
		m_nLen,
		m_nPos,
		m_nLineNum,
		_ftelli64(m_file),
		m_strCur
	};
	m_vState.push_back(s);
}


wchar_t CReader::RestoreState()
{
	if (!m_vState.empty())
	{
		RdrState &s     = m_vState.back();
		m_chCurrent     = s.chCurrent;
		m_chNext        = s.chNext;
		m_nCurrCharType = s.nCurrCharType;
		m_nLen          = s.nLen;
		m_nPos          = s.nPos;

		if (m_nLineNum != s.nLineNum)
		{
			m_nLineNum = s.nLineNum;
			_fseeki64(m_file, s.nFilePos, SEEK_SET); //подглючивает для UTF-8 и UNICODE
			m_strCur = s.strCurStr;
		}

		m_vState.pop_back();
		return m_chCurrent;
	}

	return 0;
}

void CReader::PopState()
{
	if (!m_vState.empty())
	{
		m_vState.pop_back();
	}
}

/*
 * Получение очередного символа.
 * С пропуском пробельных символов, за исключением конца строки, если задано режимом
 * С преобразованием маленьких букв в большие, если не приём строки в кавычках
 */
wchar_t CReader::readChar(bool bSkipWS)
{
	do
	{
		m_chCurrent = m_chNext;
		m_chNext = getChar();

		if (m_chNext <= 011)
		{
			m_chNext = L' ';
		}

		/*
		тут можно сделать регистронезависимость. преобразовывать a..z в A..Z
		именно тут, т.к. readChar используется для приёма символов
		*/
		if (!g_Globals.isInString()) // если мы не внутри строки
		{
			if ((L'a' <= m_chCurrent) && (m_chCurrent <= L'z')) // если у нас маленькие буквы
			{
				m_chCurrent = std::towupper(m_chCurrent); // превращаем маленькие лат. буквы в большие.
			}

			// это касается только латинских букв. все остальные - остаются как есть
		}

		// заодно и проанализируем текущий символ.
		m_nCurrCharType = AnalyseChar(m_chCurrent);
	}
	while (bSkipWS && m_nCurrCharType == CHAR_TYPE::SPACES); // и заодно если задано пропускать пробельные символы, то мы их пропускаем

	return m_chCurrent;
}

bool CReader::isEOF() const
{
	return ((feof(m_file) && m_nPos >= m_nLen) || (m_chCurrent == 032 && m_chNext == 032));
}

/*
 * Анализ входного символа.
 * Выход: тип символа
 */
CReader::CHAR_TYPE CReader::AnalyseChar(wchar_t ch) const
{
//  if (ch == 032) // эта штука более вредит, чем помогает
//  {
//      return CT_EOF;
//  }
	if (ch == 0 || ch == 012 || ch == 032) // 032 == ^Z
	{
		return CHAR_TYPE::LN;
	}

	if (((L'A' <= ch) && (ch <= L'Z')) || (ch == L'_') || (ch == L'$'))
	{
		return CHAR_TYPE::LETTERS;
	}

	if ((L'0' <= ch) && (ch <= L'9'))
	{
		return CHAR_TYPE::DIGITS;
	}

	if (ch <= L' ')
	{
		return CHAR_TYPE::SPACES;
	}

	return CHAR_TYPE::OTHERS;
}

/*
 *Пропуск пробельных символов. На входе ch содержит текущий символ.
 *выход: true - конец строки
 *       false - не конец строки
 *На выходе ch содержит текущий не пробельный символ.
 */
bool CReader::SkipWhitespaces(wchar_t &ch)
{
	CHAR_TYPE ct = AnalyseChar(ch);

	while (ct == CHAR_TYPE::SPACES)
	{
		ch = readChar(true);
		ct = getCurrCharType();
	}

	return (ct == CHAR_TYPE::LN);
}

/*
 * Чтение лексемы.
 * На входе ch содержит текущий символ.
 * На выходе ch содержит текущий символ, не входящий в лексему.
 * Выход: true - в token занесено значение
 * false - не занесено.
 */
bool CReader::readToken(CBKToken *token, wchar_t &ch)
{
	std::wstring strTokenBuffer;
	CHAR_TYPE ct = AnalyseChar(ch);
	int nCnt = 0;

	while (ct == CHAR_TYPE::DIGITS || ct == CHAR_TYPE::LETTERS || ((ch == L'.') && nCnt))
	{
		strTokenBuffer.push_back(ch);
		ch = readChar();
		ct = getCurrCharType();
		nCnt++; // вводим защиту. символ  L'.' не может быть первым.
	}

	if (!strTokenBuffer.empty())
	{
		token->setName(strTokenBuffer);
		return true;
	}

	return false;
}

/*
 * Специальный вариант, чтения имени регистра вида %n. Читается только 2 символа, % и цифра за ним
 * На входе ch содержит текущий символ.
 * На выходе ch содержит текущий символ, не входящий в лексему.
 * Выход: true - в token занесено значение
 * false - не занесено.
 */
bool CReader::readTokenR(CBKToken *token, wchar_t &ch)
{
	std::wstring strTokenBuffer;
	CHAR_TYPE ct = AnalyseChar(ch);

	if (ch == L'%')
	{
		strTokenBuffer.push_back(ch);
		ch = readChar();
		ct = getCurrCharType();

		if (ct == CHAR_TYPE::DIGITS)
		{
			strTokenBuffer.push_back(ch);
			ch = readChar();
			ct = getCurrCharType();
			token->setName(strTokenBuffer);
			return true;
		}
	}

	return false;
}

CReader::FILE_CHARSET CReader::AnalyseCharset(const fs::path &strFileName)
{
	m_file = _wfopen(strFileName.c_str(), L"rb");

	if (m_file)
	{
		uint8_t BOM[3];
		FILE_CHARSET nRet = FILE_CHARSET::UNDEFINE;
		// сперва поищем BOM
		fread(BOM, 1, 3, m_file);

		if (BOM[0] == 0xFF && BOM[1] == 0xFE)
		{
			nRet = FILE_CHARSET::UTF16LE;
		}
		else if (BOM[0] == 0xEF && BOM[1] == 0xBB && BOM[2] == 0xBF)
		{
			// утф8 без бом будет определяться некорректно возможно
			nRet = FILE_CHARSET::UTF8;
		}
		else
		{
			// дальше надо анализировать кодировку.
			fseek(m_file, 0, SEEK_SET);
			nRet = def_code(255);
		}

		fclose(m_file);
		m_file = nullptr;
		return nRet;
	}

	return FILE_CHARSET::FILEERROR;
}

int CReader::work_2s(int c1, int c2, int check, uint8_t buf[128]) const
{
	int i = (c1 << 2) + (c2 >> 3); /* Номер байта в массиве. */
	int mask = 0x80 >> (c2 & 7); /* Маска, соответствующая номеру бита в байте. */

	/* Если check=1, проверяем: если соответствующий бит массива buf равен 0,
	значит, указанное сочетание уже встречалось раньше. Тогда выходим из
	функции, возвращая 0. Если же сочетание не встречалось, то помечаем, что
	оно встретилось (обнуляем соответствующий бит массива buf). */

	if (check == 1)
	{
		if ((buf[i] & mask) == 0)
		{
			return (0);
		}

		buf[i] &= ~mask;
	}

	/* Проверяем, допустимо сочетание или нет. */

	if ((table_2s[i] & mask) != 0)
	{
		return (1);    /* Допустимо. */
	}

	return (2);        /* Недопустимо. */
}

int CReader::get_char()
{
	uint8_t ch = 0;

	if (!feof(m_file))
	{
		m_nLen += fread(&ch, 1, 1, m_file);
		return ch;
	}

	return -1;
}

CReader::FILE_CHARSET CReader::def_code(int n)
{
	m_nLen = 0;
	/* В массиве buf_1 хранится информация о том, какие сочетания русских букв
	уже встречались в варианте ALT, а в массиве buf_2 - в варианте WIN. */
	uint8_t buf_1[128];
	uint8_t buf_2[128];
	int bad_1 = 0;
	int bad_2 = 0;
	int bad_3 = 0;
	int all_1 = 0;
	int all_3 = 0;  /* all_2=all_3 */
	int c1; int c2 = -1; /* Символы текущего обрабатываемого сочетания. */
	/* Инициализация buf_1 и buf_2. */
	memset(buf_1, 0xff, sizeof(buf_1));
	memset(buf_2, 0xff, sizeof(buf_2));
	double nD0 = 0.0, nD1 = 0.0, nX04 = 0.0, nX00 = 0.0;
	double n00_suffix = 0.0;    // тут латинские и прочие буквы для юникода.
	double n04_suffix = 0.0;    // тут русские буквы для юникода.
	double nD0_suffix = 0.0;    // тут русские буквы с префиксом D0 в UTF8
	double nD1_suffix = 0.0;    // тут русские буквы с префиксом D1 в UTF8

	/* Главный цикл - обработка сочетаний для каждого из трёх вариантов. Цикл
	выполняется, пока не кончится текст или в каком-либо из вариантов не
	встретится n сочетаний. */

	while (((c1 = c2, c2 = get_char()) != -1) && (all_1 < n) && (all_3 < n))
	{
		// --- определение utf8, utf16 (только русский диапазон) ----------------
		switch (c2)
		{
			case 00:
				nX00++;
				break;

			case 04:
				nX04++;
				break;

			case 0xd0:
				nD0++;
				break;

			case 0xd1:
				nD1++;
				break;

			default:
			{
				if ((c1 == 0x00) && (0 <= c2 && c2 <= 0x7f))
				{
					n00_suffix++;
				}

				if ((c1 == 0x04) && ((0x10 <= c2 && c2 <= 0x19) || (0x1a <= c2 && c2 <= 0x2f) || (c2 == 01) || (0x30 <= c2 && c2 <= 0x39) || (0x3a <= c2 && c2 <= 0x4f) || (c2 == 51)))
				{
					n04_suffix++;
				}

				if ((c1 == 0xd0) && ((0x90 <= c2 && c2 <= 0xbf) || c2 == 0x81))
				{
					nD0_suffix++;
				}

				if ((c1 == 0xd1) && ((0x80 <= c2 && c2 <= 0x8f) || c2 == 0x91))
				{
					nD1_suffix++;
				}
			}
		}

		// ----------------------------------------------------------------------

		/* Вариант ALT. Вначале проверяем, являются ли символы текущего сочетания
		кодами русских букв в кодировке ALT. */

		if ((((c1 >= 0x80) && (c1 < 0xB0)) || ((c1 >= 0xE0) && (c1 < 0xF0))) &&
		        (((c2 >= 0x80) && (c2 < 0xB0)) || ((c2 >= 0xE0) && (c2 < 0xF0))))
		{
			switch (work_2s(alt2num(c1), alt2num(c2), 1, buf_1)) /* Обработали. */
			{
				case 2: bad_1++;

				case 1: all_1++;
			}
		}

		/* Варианты WIN и KOI. Вначале проверяем, являются ли символы текущего
		сочетания кодами русских букв в этих кодировках (в обеих кодировках
		диапазоны кодов русских букв совпадают). */

		if ((c1 & c2) >= 0xC0) /* Эквивалентно условию (c1>=0xC0)&&(c2>=0xC0). */
		{
			switch (work_2s(c1 & 31, c2 & 31, 1, buf_2)) /* Обработали. */
			{
				case 0: continue; /* Если сочетание букв уже встречалось в варианте WIN,
                              то оно уже встречалось и в варианте KOI, так что
                              пропускаем обработку варианта KOI и переходим
                              к следующей итерации главного цикла. */

				case 2: bad_2++;
			}

			/* Если сочетание букв ещё не встречалось в варианте WIN, то оно заведомо
			не встречалось и в варианте KOI, поэтому специально проверять это не
			надо - значит, функцию work_2s вызываем с параметром check, равным 0. */

			switch (work_2s(koi2num(c1), koi2num(c2), 0, nullptr)) /* Обработали. */
			{
				case 2: bad_3++;

				case 1: all_3++;
			}
		}
	}

	// --- определение utf8, utf16 (только русский диапазон) ----------------
	// теперь по полученным числам проверяем
	double nOnePercent = (double)m_nLen / 500.0; // полпроцента, один - слишком много
	// потому что псевдографика ломает всю статистику. там нужно более продвинутые алгоритмы придумывать

	/*
	* Если в тексте очень уж часто встречается код 04, а верхних кодов достаточно мало
	* то это - Unicode (UTF-16 LE)(CP-1200)
	*/
//  bool b1 = !!(nX00 + n00_suffix);
//  bool b2 = (abs(nX00 - n00_suffix) < nOnePercent);
//  bool b3 = !!(nX04 + n04_suffix);
//  bool b4 = (abs(nX04 - n04_suffix) < nOnePercent);
	if ((nX00 + n00_suffix) && (abs(nX00 - n00_suffix) < nOnePercent)
	        && (nX04 + n04_suffix) && (abs(nX04 - n04_suffix) < nOnePercent))
	{
		return FILE_CHARSET::UTF16LE;
	}

	/*
	* Если количество символов D1 и количество кодов из интервала 80..9f и
	* количество символов D0 и количество кодов из интервала b0..cf
	* примерно равны друг другу (разница в пределах 1% от размера файла),
	* то это скорее всего UTF-8 (распознаётся только русская кодировка, остальные нам не надо.)
	*/
//  bool b1 = !!(nD0 + nD0_suffix);
//  bool b2 = (abs(nD0 - nD0_suffix) < nOnePercent);
//  bool b3 = !!(nD1 + nD1_suffix);
//  bool b4 = (abs(nD1 - nD1_suffix) < nOnePercent);
	if ((nD0 + nD0_suffix > nOnePercent) && (abs(nD0 - nD0_suffix) < nOnePercent)
	        && (nD1 + nD1_suffix > nOnePercent) && (abs(nD1 - nD1_suffix) < nOnePercent))
	{
		return FILE_CHARSET::UTF8;
	}

	// ----------------------------------------------------------------------

	/* Данные собраны. Теперь, если в каком-либо из вариантов недопустимых
	сочетаний не больше 1/32 от общего их числа, то считаем, что их и не
	было. */

	if (bad_1 <= (all_1 >> 5))
	{
		bad_1 = 0;
	}

	if (bad_2 <= (all_3 >> 5))
	{
		bad_2 = 0;
	}

	if (bad_3 <= (all_3 >> 5))
	{
		bad_3 = 0;
	}

	/* Получаем результат. */
	unsigned int a = ((255 - bad_1) << 8) + all_1;
	unsigned int b = ((255 - bad_2) << 8) + all_3;
	unsigned int c = ((255 - bad_3) << 8) + all_3;

	if ((a >= b) && (a >= c))
	{
		return FILE_CHARSET::CP866;
	}

	if (b >= c)
	{
		return FILE_CHARSET::CP1251;
	}

	return FILE_CHARSET::KOI8;
}


/*
преобразование юникодной строки в бкшный кои8.
вход:
ustr - преобразуемая строка
pBuff - буфер, куда выводится результат
bufSize - размер буфера
bFillBuf - флаг. если строка короче размера буфера, буфер до конца забивается пробелами. (конца строки - 0 нету.)
*/
void UNICODEtoBK(std::wstring &ustr, uint8_t *pBuff, size_t bufSize, bool bFillBuf)
{
	const size_t len = ustr.length();
	size_t bn = 0;

	for (size_t n = 0; n < len; ++n)
	{
		pBuff[bn++] = UNICODEtoBK_ch(ustr[n]);

		if (bn >= bufSize)
		{
			break;
		}
	}

	if (bFillBuf)
	{
		while (bn < bufSize)
		{
			pBuff[bn++] = 32;
		}
	}
}
/*
преобразование юникодного символа в бкшный кои8.
вход:
uchr - преобразуемый символ
выход: результат
*/
uint8_t UNICODEtoBK_ch(wchar_t uchr)
{
	if (uchr == L'\t')
	{
		return 9;
	}

	if (uchr == L'\n')
	{
		return 10;
	}

	if (uchr < 32) // если символ меньше пробела,
	{
		return 32;  // то будет пробел
	}

	if ((32 <= uchr) && (uchr < 127)) // если буквы-цифры- знаки препинания
	{
		return uint8_t(uchr); // то буквы-цифры- знаки препинания
	}

	if (uchr == 0x25a0) // если такое
	{
		return 127; // то это. это единственное исключение в нижней половине аски кодов
	}

	// если всякие другие символы
	// то ищем в таблице нужный нам символ, а его номер - будет кодом кои8

	if (uchr == L'ё')
	{
		uchr = L'е'; // меняем ё на е, т.к. на БК нету такой буквы
	}
	else if (uchr == L'Ё')
	{
		uchr = L'Е'; // меняем ё на е, т.к. на БК нету такой буквы
	}

	for (uint8_t i = 32; i < 128; ++i)
	{
		if (koi8tbl[i] == uchr)
		{
			return (i + 0200);
		}
	}

	return 32; // если такого символа нету в таблице - будет пробел
}

