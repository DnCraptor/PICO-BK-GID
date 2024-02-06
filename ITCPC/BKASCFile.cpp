#include "pch.h"
#include "BKASCFile.h"

#pragma warning(disable:4996)

CBKASCFile::CBKASCFile(const fs::path &strName)
	: CBaseFile(strName)
	, m_bEof(false)
	, m_nInternalBlockNumber(0)
	, m_nInternalPos(0)
	, m_pWrBuf{0}
	, m_bFlushed(false)
{
	m_nTabWidth = 8;
}


CBKASCFile::~CBKASCFile()
{
	Close();
}

// выход: true - Файл открылся, false - не открылся.
bool CBKASCFile::Open(bool bWrite)
{
	if (bWrite)
	{
		// сформируем корректное имя
		std::wstring buff = { L"      " }; // размер 6 пробелов
		std::wstring strFileName = m_strFileName.filename().wstring();
		size_t len = strFileName.length();

		for (size_t i = 0; i < 6; ++i) // возьмём только первые 6 символов
		{
			// если длина имени меньше 6 символов или встретилась точка
			if (i >= len || strFileName[i] == L'.')
			{
				break; // прекращаем цикл
			}

			buff[i] = strFileName[i]; // формируем ASC бейсикове имя
		}

		m_strInternalAscName = m_strFileName.parent_path() / (buff + L".ASC");
		m_nInternalBlockNumber = 0;
		m_bEof = false;
		// сформируем заголовочный файл
		m_f = BKOpen((m_strInternalAscName.wstring() + L".bin"), L"wb");

		if (m_f)
		{
			WriteWord(0x3dee);
			WriteWord(2);
			WriteWord(0);
			Close();
			m_nInternalPos = 0;
			return true;
		}

		return false; // не открылось для записи
	}

	bool bRet = false;
	/*
	на вход подаётся имя файла заголовка. Причём формат всех файлов - .bin - генерирующихся
	эмулятором.
	*/
	// открываем файл заголовок и проверяем содержимое.
	m_f = BKOpen(m_strFileName, L"rb");

	if (m_f)
	{
		size_t len = fs::file_size(m_strFileName);

		if (len == 6)
		{
			uint16_t w = ReadWord();

			if (w == 0x3dee) // магические числа. это адрес загрузки бин файла. фиктивный, но по нему распознаётся .asc файл
			{
				w = ReadWord();

				if (w == 2) // размер содержимого.
				{
					w = ReadWord();

					if (w == 0) // собственно содержимое.
					{
						Close();
						bRet = true;
					}
				}
			}

			if (bRet)
			{
				// заголовочный файл верный
				// теперь делаем внутреннее имя файла.
				std::wstring ext = m_strFileName.extension().wstring();

				if (!wcscoll(ext.c_str(), L".bin")) // если расширение не бин
				{
					// то удаляем расширение
					m_strInternalAscName = m_strFileName.parent_path() / m_strFileName.stem();
					m_nInternalBlockNumber = 0;
					MakeintenalCurrentName();
					m_bEof = false;
					bRet = OpenInternalFile();
				}
			}
		}
	}

	return bRet;
}

void CBKASCFile::MakeintenalCurrentName()
{
	std::vector<wchar_t> buffer(_MAX_PATH);
	swprintf(buffer.data(), L" #%03d.bin", m_nInternalBlockNumber);
	m_strInternalCurrentName = m_strInternalAscName.wstring() + std::wstring(buffer.data());
}

bool CBKASCFile::OpenInternalFile()
{
	Close();
	m_nInternalPos = 0;
	m_f = BKOpen(m_strInternalCurrentName, L"rb");

	if (m_f)
	{
		uint16_t w = ReadWord();

		if (w == 0x3dee)
		{
			w = ReadWord();

			if (w == 256)
			{
				return true;
			}
		}
	}

	return false;
}

wchar_t CBKASCFile::ReadChar()
{
	uint8_t ch = 0;
	wchar_t tch = 0;

	// если конец файла - выдаём признак конца файла
	if (m_bEof)
	{
		return 032;
	}

	// если блок закончился - надо перейти к следующему блоку
	if (m_nInternalPos >= bufSize)
	{
		m_nInternalBlockNumber++;
		MakeintenalCurrentName();

		if (!OpenInternalFile()) // если следующий блок не открывается
		{
			m_bEof = true; // значит считаем, что конец файла
			return 032;
		}
	}

	if (m_f)
	{
		size_t n = fread(&ch, 1, sizeof(uint8_t), m_f);

		if (n == 1) // сколько прочлось? сколько нужно
		{
			m_nInternalPos++; // увеличиваем позицию.

			if (ch == 032) // вот это правильный конец файла
			{
				m_bEof = true;
				tch = ch;
			}
			else
			{
				tch = BKToUNICODE_Byte(ch);
			}
		}
		else
		{
			m_bEof = true;// если что-то не так - говорим что конец файла
			return 032;
		}
	}

	return tch;
}

bool CBKASCFile::WriteWord(uint16_t w)
{
	size_t n = 0;

	if (m_f)
	{
		n = fwrite(&w, 1, sizeof(uint16_t), m_f);
	}

	return (n == sizeof(uint16_t));
}

long CBKASCFile::GetPos()
{
	if (m_f)
	{
		return ftell(m_f) - 4;
	}

	return 0;
}

int CBKASCFile::SetPos(long pos, long mode)
{
	if (m_f)
	{
		return fseek(m_f, pos + 4, mode);
	}

	return 0;
}

bool CBKASCFile::WriteChar(wchar_t tch)
{
	bool bRet = true;

	// если буфер заполнен
	if (m_nInternalPos >= bufSize)
	{
		bRet = FlushInternalWrBuff(); // сбрасываем его на диск
	}

	if (tch == 032)
	{
		m_pWrBuf[m_nInternalPos++] = 032;
		m_bEof = true;
	}
	else
	{
		m_pWrBuf[m_nInternalPos++] = UNICODEtoBK_Byte(tch);
	}

	m_bFlushed = false;
	return bRet;
}

bool CBKASCFile::FlushInternalWrBuff()
{
	if (!m_bFlushed)
	{
		bool bRet = false;
		m_nInternalPos = 0;
		MakeintenalCurrentName();
		m_f = BKOpen(m_strInternalCurrentName, L"wb");

		if (m_f)
		{
			WriteWord(0x3dee);
			WriteWord(bufSize);
			fwrite(m_pWrBuf, 1, bufSize, m_f);
			memset(m_pWrBuf, ' ', bufSize);
			m_nInternalBlockNumber++;
			bRet = true;
			Close();
		}

		m_bFlushed = true;
		return bRet;
	}

	return true;
}

void ITCPC::TXT2ASC(CBaseFile *pInFile, CBaseFile *pOutFile)
{
	if (pInFile && pOutFile)
	{
		int nCharsCount = 0;

		if (!pInFile->Open(false))
		{
			wprintf(L"Ошибка! %s не открывается.\n", pInFile->getFileName().c_str());
			return;
		}

		if (!pOutFile->Open(true))
		{
			wprintf(L"Ошибка! %s не открывается.\n", pOutFile->getFileName().c_str());
			return;
		}

		while (!pInFile->isEOF())
		{
			wchar_t Sym = pInFile->ReadChar();

			if (pInFile->isEOF())
			{
				pOutFile->WriteChar(032); nCharsCount++;
				reinterpret_cast<CBKASCFile *>(pOutFile)->FlushInternalWrBuff();
				break;
			}

			if (Sym == 0)
			{
				Sym = 10;
			}
			else if (Sym == 13)
			{
				continue;
			}
			else if (Sym == 9)
			{
				int nNextTabPos = (nCharsCount / pInFile->getTabWidth() + 1) * pInFile->getTabWidth();

				do
				{
					pOutFile->WriteChar(L' '); nCharsCount++;
				}
				while (nCharsCount < nNextTabPos);

				continue;
			}

			pOutFile->WriteChar(Sym); nCharsCount++;
		}
	}
	else
	{
		wprintf(L"Недостаточно памяти.\n");
	}
}
