#include "pch.h"

#include "BaseFile.h"

// нижняя таблица для конвертации ibm 2 bk
const uint8_t CBaseFile::m_LowRow[32] =
{
	10,  127, 127, 162, 184, 180, 169,  46,
	127,   9,  10,  43,  43,   0, 127,  36,
	190, 177, 179,  33, 160,  36,  95, 179,
	179, 173, 190, 177,  95, 181, 179, 173
};


// таблица соответствия верхней половины аскии кодов с 128 по 255, включая псевдографику
const wchar_t CBaseFile::m_koi8tbl[128] =   // {200..237} этих символов на бк нету.
{
	L' ', L' ', L' ', L' ', L' ', L' ', L' ', L' ',
	L' ', L' ', L' ', L' ', L' ', L' ', L' ', L' ',
	L' ', L' ', L' ', L' ', L' ', L' ', L' ', L' ',
	L' ', L' ', L' ', L' ', L' ', L' ', L' ', L' ',
	// {240..277}
	0xb6,    0x2534,  0x2665,  0x2510,  0x2561,  0x251c,  0x2514,  0x2550,
	0x2564,  0x2660,  0x250c,  0x252c,  0x2568,  0x2193,  0x253c,  0x2551,
	0x2524,  0x2190,  0x256c,  0x2191,  0x2663,  0x2500,  0x256b,  0x2502,
	0x2666,  0x2518,  0x256a,  0x2565,  0x2567,  0x255e,  0x2192,  0x2593,
	// {300..337}
	L'ю', L'а', L'б', L'ц', L'д', L'е', L'ф', L'г',
	L'х', L'и', L'й', L'к', L'л', L'м', L'н', L'о',
	L'п', L'я', L'р', L'с', L'т', L'у', L'ж', L'в',
	L'ь', L'ы', L'з', L'ш', L'э', L'щ', L'ч', L'ъ',
	// {340..377}
	L'Ю', L'А', L'Б', L'Ц', L'Д', L'Е', L'Ф', L'Г',
	L'Х', L'И', L'Й', L'К', L'Л', L'М', L'Н', L'О',
	L'П', L'Я', L'Р', L'С', L'Т', L'У', L'Ж', L'В',
	L'Ь', L'Ы', L'З', L'Ш', L'Э', L'Щ', L'Ч', L'Ъ'
};


CBaseFile::CBaseFile(const fs::path &strName)
	: m_f(nullptr)
	, m_strFileName(strName)
	, m_nFileLength(0)
	, m_nTabWidth(8)
{
}


CBaseFile::~CBaseFile()
{
	Close();
}

bool CBaseFile::Open(bool bWrite)
{
	return false;
}

void CBaseFile::Close()
{
	if (m_f)
	{
		fclose(m_f);
		m_f = nullptr;
		m_nFileLength = 0;
	}
}

wchar_t CBaseFile::ReadChar()
{
	return 0;
}

uint8_t CBaseFile::ReadByte()
{
	uint8_t b = 0;

	if (m_f)
	{
		fread(&b, 1, sizeof(uint8_t), m_f);
	}

	return b;
}

uint16_t CBaseFile::ReadWord()
{
	uint16_t w = 0;

	if (m_f)
	{
		fread(&w, 1, sizeof(uint16_t), m_f);
	}

	return w;
}

bool CBaseFile::WriteChar(wchar_t ch)
{
	return false;
}

bool CBaseFile::WriteByte(uint8_t b)
{
	size_t n = 0;

	if (m_f)
	{
		n = fwrite(&b, 1, sizeof(uint8_t), m_f);
	}

	return (n == sizeof(uint8_t));
}

long CBaseFile::GetPos()
{
	if (m_f)
	{
		return ftell(m_f);
	}

	return 0;
}

int CBaseFile::SetPos(long pos, long mode)
{
	if (m_f)
	{
		return fseek(m_f, pos, mode);
	}

	return 0;
}

bool CBaseFile::isEOF()
{
	if (m_f)
	{
		return feof(m_f) != 0;
	}

	return true;
}

// Проверим, а не существует ли уже такой файл?
bool CBaseFile::CheckOutFile()
{
	constexpr auto MAX_COUNT = 10000;
	int nNum = 0;
	fs::path newFileName = m_strFileName;
	std::wstring strExt = newFileName.extension().wstring();
	std::wstring strName = newFileName.stem().wstring();

	while (fs::exists(newFileName) && nNum < MAX_COUNT)
	{
		// сделаем новое имя
		nNum++;
		std::wstring strNameN = strName + L"(" + std::to_wstring(nNum) + L")" + strExt;
		newFileName.replace_filename(strNameN);
	} // будем проверять имена с цифрой до тех пор, пока не найдём

	if (nNum >= MAX_COUNT) // если вышли по такому условию, то явно что-то не так
	{
		return false;
	}

	m_strFileName = newFileName;
	return true;
}

wchar_t CBaseFile::BKToUNICODE_Byte(uint8_t b)
{
	if (b == 0)
	{
		return 0;
	}

	if (b < 32)
	{
		return wchar_t(b); // на БК коды меньше пробела - обычно управляющие символы. и требуют обработки по своему.
	}

	if (32 <= b && b < 127)
	{
		return wchar_t(b);
	}

	if (b == 127)
	{
		return wchar_t(0x25a0);
	}

	if (b >= 128)
	{
		return m_koi8tbl[b - 128];
	}

	return 0; // этот return не должен выполниться в принципе.
}

/*
преобразование юникодного символа, в бкшный кои8.
вход:
tch - преобразуемый символ
выход:
кои8 символ
*/
uint8_t CBaseFile::UNICODEtoBK_Byte(wchar_t tch)
{
	if (tch < 32) // если символ меньше пробела,
	{
		return m_LowRow[tch];
	}

	if (32 <= tch && tch < 127) // если буквы-цифры- знаки препинания
	{
		return (tch & 0xff); // то буквы-цифры- знаки препинания
	}

	if (tch == 0x25a0) // если такое
	{
		return 127; // то это. это единственное исключение в нижней половине аски кодов
	}

	// если всякие другие символы
	// то ищем в таблице нужный нам символ, а его номер - будет кодом кои8
	if (tch == L'ё')
	{
		tch = L'е'; // меняем ё на е, т.к. на БК нету такой буквы
	}
	else if (tch == L'Ё')
	{
		tch = L'Е'; // меняем ё на е, т.к. на БК нету такой буквы
	}

	for (int i = 32; i < 128; ++i)
	{
		if (m_koi8tbl[i] == tch)
		{
			return uint8_t(i & 0xff) + 0200;
		}
	}

	return 32; // если такого символа нету в таблице - будет пробел
}

FILE *CBaseFile::BKOpen(const fs::path &filepath, const std::wstring &mode)
{
#if defined(_WIN32)
	FILE *file = ::_wfopen(filepath.c_str(), mode.c_str());
#else
	fs::path convertedMode = mode;
	FILE *file = ::fopen(filepath.c_str(), convertedMode.c_str());
#endif

	if (!file)
	{
		const char *reason = strerror(errno);
		std::string err{ "opening '" };
		err += filepath.generic_string() + std::string{ "' failed: " } + reason;
		printf(err.c_str());
	}

	return file;
}
