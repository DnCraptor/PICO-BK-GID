#include "pch.h"
#include "WinFile.h"
#include <cmath>
#include <Windows.h>


const uint8_t CWinFile::table_2s[128] =
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


CWinFile::CWinFile(const fs::path &strName, FILE_CHARSET nManualCharSet)
	: CBaseFile(strName)
	, m_nManualCharset(nManualCharSet)
	, m_nLen(0)
{
	m_nTabWidth = 4;
}


CWinFile::~CWinFile()
{
	Close();
}

// выход: true - Файл открылся, false - не открылся.
bool CWinFile::Open(bool bWrite)
{
	bool bRet = false;
	std::wstring strMode;

	if (!bWrite)
	{
		FILE_CHARSET nDetectedCharset = AnalyseCharset();

		// если кодировку вручную не задавали
		if (m_nManualCharset == FILE_CHARSET::UNDEFINE)
		{
			// то будем использовать автоопределённую.
			m_nManualCharset = nDetectedCharset;
		}
		else if (m_nManualCharset == FILE_CHARSET::UTF8 && nDetectedCharset == FILE_CHARSET::UTF16LE)
		{
			m_nManualCharset = nDetectedCharset; // уточняем юникод
		}
		else if (m_nManualCharset == FILE_CHARSET::UTF16LE && nDetectedCharset == FILE_CHARSET::UTF8)
		{
			m_nManualCharset = nDetectedCharset; // уточняем юникод
		}

		wprintf(L"Автоопределение кодировки: %s, преобразование будет из: %s\n", GetFormatName(nDetectedCharset).c_str(), GetFormatName(m_nManualCharset).c_str());
		// а иначе считаем, что пользователю лучше знать, какую он использует кодировку.
		strMode = { L"r" };

		switch (m_nManualCharset)
		{
			case FILE_CHARSET::KOI8:
				strMode += { L"b" }; // при открытии в текстовом режиме, если конец строки не \r\n, начинаются феерические глюки с определением конца файла и позиционированием.
				break;
			case FILE_CHARSET::CP866:
				strMode += { L"b" };
				break;
			case FILE_CHARSET::CP1251:
				strMode += { L"b" };
				break;
			case FILE_CHARSET::UTF8:
				strMode += { L"b" };
				break;
			case FILE_CHARSET::UTF16LE:
				strMode += { L"b, ccs=UTF-16LE" };
				break;
		}
	}
	else
	{
		if (m_nManualCharset == FILE_CHARSET::UNDEFINE)
		{
			// то будем использовать по-умолчанию юникод
			m_nManualCharset = FILE_CHARSET::UTF16LE;
		}

		strMode = { L"w" };

		switch (m_nManualCharset)
		{
			case FILE_CHARSET::KOI8:
				strMode += { L"b" };
				break;
			case FILE_CHARSET::CP866:
				strMode += { L"t" };
				break;
			case FILE_CHARSET::CP1251:
				strMode += { L"t" };
				break;
			case FILE_CHARSET::UTF8:
				strMode += { L"t, ccs=UTF-8" };
				break;
			case FILE_CHARSET::UTF16LE:
				strMode += { L"t, ccs=UTF-16LE" };
				break;
		}

		if (!CheckOutFile()) // если случилась какая-то фигня
		{
			m_strFileName = L"Out.tmp"; // имя файла будет такое
		}
	}

	m_f = BKOpen(m_strFileName, strMode);

	if (m_f)  // если файл открылся, то всё нормально
	{
		if (!bWrite)
		{
			m_nFileLength = fs::file_size(m_strFileName);
			uint8_t BOM[3];

			if (m_nManualCharset == FILE_CHARSET::UTF16LE)
			{
				// сперва поищем BOM
				fread(BOM, 1, 2, m_f);

				if (BOM[0] == 0xFF && BOM[1] == 0xFE)
				{
				}
				else
				{
					fseek(m_f, 0, SEEK_SET); // файл оказался без БОМ
				}
			}
			else if (m_nManualCharset == FILE_CHARSET::UTF8)
			{
				fread(BOM, 1, 3, m_f);

				if (BOM[0] == 0xEF && BOM[1] == 0xBB && BOM[2] == 0xBF)
				{
				}
				else
				{
					fseek(m_f, 0, SEEK_SET); // файл оказался без БОМ
				}
			}
		}

		bRet = true;
	}

	return bRet;
}


wchar_t CWinFile::ReadChar()
{
	uint8_t ch[2] = {0};
	wchar_t wch[2] = {0};

	if (m_f)
	{
		switch (m_nManualCharset)
		{
			case FILE_CHARSET::KOI8:
				fread(&ch[0], 1, sizeof(uint8_t), m_f);
				return (ch[0] < 128) ? wchar_t(ch[0]) : m_koi8tbl[ch[0] - 128];

			case FILE_CHARSET::CP866:
				fread(&ch[0], 1, sizeof(uint8_t), m_f);
				MultiByteToWideChar(CP_OEMCP, 0, reinterpret_cast<LPCSTR>(ch), 1, reinterpret_cast<LPWSTR>(wch), 1);
				return wch[0];

//              setlocale(LC_CTYPE, "Russian_Russia.ACP");
//              setlocale(LC_CTYPE, "Russian_Russia.OCP");
// говорят, что надо делать такое, чтобы функция mbtowc могла конвертировать правильно
// вот только про setlocale(LC_CTYPE написано, что оно не затрагивает функцию mbtowc

			case FILE_CHARSET::CP1251:
				fread(&ch[0], 1, sizeof(uint8_t), m_f);
				MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<LPCSTR>(ch), 1, reinterpret_cast<LPWSTR>(wch), 1);
				return wch[0];

			case FILE_CHARSET::UTF8:
				return ReadUTF8Char();

			case FILE_CHARSET::UTF16LE:
				return (wchar_t)fgetwc(m_f); // эта функция очень плохо относится к символу конца строки, особенно если это \n вместо \r\n

			default:
				return 0;
		}
	}

	return 0;
}

bool CWinFile::WriteChar(wchar_t tch)
{
	uint8_t ch[2] = {0};
	wchar_t wch[2] = { tch, 0};
	size_t n = 0;

	if (m_f)
	{
		switch (m_nManualCharset)
		{
			case FILE_CHARSET::KOI8:
				ch[0] = UNICODEtoBK_Byte(tch);
				n = fwrite(&ch[0], 1, sizeof(uint8_t), m_f);
				break;

			case FILE_CHARSET::CP866:
				WideCharToMultiByte(CP_OEMCP, 0, reinterpret_cast<LPCWSTR>(wch), 1, reinterpret_cast<LPSTR>(ch), 1, nullptr, nullptr);
				n = fputc(ch[0], m_f);
				break;

			case FILE_CHARSET::CP1251:
				WideCharToMultiByte(CP_ACP, 0, reinterpret_cast<LPCWSTR>(wch), 1, reinterpret_cast<LPSTR>(ch), 1, nullptr, nullptr);
				n = fputc(ch[0], m_f);
				break;

			case FILE_CHARSET::UTF8:
			case FILE_CHARSET::UTF16LE:
				n = fputwc(tch, m_f);
				break;

			default:
				return false;
		}
	}

	return (n != 0);
}


CWinFile::FILE_CHARSET CWinFile::AnalyseCharset()
{
	FILE_CHARSET nRet = FILE_CHARSET::KOI8; // если все наши попытки распознать кодировку будут неудачны, по умолчанию считаем, что должно быть КОИ8
	uint8_t BOM[3] {};
	Close(); // если файл был открыт, то его надо закрыть.
	// открываем файл в бинарном режиме
	m_f = BKOpen(m_strFileName, L"rb");

	if (m_f)
	{
		// сперва поищем BOM
		fread(BOM, 1, 3, m_f);

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
			fseek(m_f, 0, SEEK_SET);
			m_nLen = 0;
			nRet = def_code(255);
		}

		Close();
	}

	return nRet;
}

int CWinFile::work_2s(int c1, int c2, int check, uint8_t buf[128])
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

	return (2);         /* Недопустимо. */
}

int CWinFile::get_char()
{
	uint8_t ch = 0;

	if (!feof(m_f))
	{
		m_nLen += fread(&ch, 1, sizeof(uint8_t), m_f);
		return ch;
	}

	return -1;
}

CWinFile::FILE_CHARSET CWinFile::def_code(int n)
{
	/* В массиве buf_1 хранится информация о том, какие сочетания руских букв
	уже встречались в варианте ALT, а в массиве buf_2 - в варианте WIN. */
	uint8_t buf_1[128];
	uint8_t buf_2[128];
	int bad_1 = 0;
	int bad_2 = 0;
	int bad_3 = 0;
	int all_1 = 0;
	int all_3 = 0;  /* all_2=all_3 */
	int c1; int c2 = 0; /* Символы текущего обрабатываемого сочетания. */
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
					[[fallthrough]];

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
					[[fallthrough]];

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
	if ((nD0 + nD0_suffix) && (abs(nD0 - nD0_suffix) < nOnePercent)
	        && (nD1 + nD1_suffix) && (abs(nD1 - nD1_suffix) < nOnePercent))
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
	// Выход: 0 - текст в кодировке ALT, 1 - WIN, 2 - KOI.
	{
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
}


std::wstring CWinFile::GetFormatName(FILE_CHARSET fchr)
{
	switch (fchr)
	{
		case FILE_CHARSET::UTF16LE:
			return { L"Unicode UTF-16LE" };
		case FILE_CHARSET::CP1251:
			return { L"Windows 1251" };
		case FILE_CHARSET::CP866:
			return { L"OEM CP-866" };
		case FILE_CHARSET::KOI8:
			return { L"KOI8R" };
		case FILE_CHARSET::UTF8:
			return { L"Unicode UTF-8" };
	}

	return { L"Неизвестно" };
}
// из-за того, что fgetpos/fsetpos хреново работают с UTF-8, будем самостоятельно читать UTF8 из бинарного файла.
wchar_t CWinFile::ReadUTF8Char()
{
	/*
	The encoding used to represent Unicode into bytes is based on rules that define how to break-up the bit-string representing an UCS into bytes.

	If an UCS fits 7 bits, its coded as 0xxxxxxx. This makes ASCII character represented by themselves
	If an UCS fits 11 bits, it is coded as 110xxxxx 10xxxxxx
	If an UCS fits 16 bits, it is coded as 1110xxxx 10xxxxxx 10xxxxxx
	If an UCS fits 21 bits, it is coded as 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	If an UCS fits 26 bits, it is coded as 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
	If an UCS fits 31 bits, it is coded as 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
	*/
	bool bFail = false;
	uint8_t ch[8] = {0};
	wchar_t wch[2] = {0};
	int pos = 0;
	uint8_t mask = 0x80;
	fread(&ch[pos++], 1, sizeof(uint8_t), m_f); // читаем первый символ

	if ((ch[0] & mask) != 0)
	{
		mask >>= 1;

		while ((ch[0] & mask))
		{
			fread(&ch[pos++], 1, sizeof(uint8_t), m_f);
			mask >>= 1;

			if (pos >= 7)
			{
				// значит ошибка. неверная кодировка
				bFail = true;
				break;
			}
		}
	}

	if (!bFail)
	{
		MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCSTR>(ch), -1, reinterpret_cast<LPWSTR>(wch), 1);
		return wch[0];
	}

	return 0;
}

