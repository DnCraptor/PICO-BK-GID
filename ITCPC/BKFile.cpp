#include "pch.h"
#include "BKFile.h"

CBKFile::CBKFile(const fs::path &strName)
	: CBaseFile(strName)
{
	m_nTabWidth = 8;
}


CBKFile::~CBKFile()
{
	Close();
}

// выход: true - Файл открылся, false - не открылся.
bool CBKFile::Open(bool bWrite)
{
	bool bRet = false;
	std::wstring strMode;

	if (!bWrite)
	{
		strMode = { L"r" };
	}
	else
	{
		strMode = { L"w" };

		if (!CheckOutFile()) // если случилась какая-то фигня
		{
			m_strFileName = L"Out.tmp"; // имя файла будет такое
		}
	}

	m_f = BKOpen(m_strFileName, { strMode + L"b" });

	if (m_f) // если файл открылся, то всё нормально
	{
		if (!bWrite)
		{
			m_nFileLength = fs::file_size(m_strFileName);
		}

		bRet = true;
	}

	return bRet;
}

wchar_t CBKFile::ReadChar()
{
	uint8_t ch = 0;

	if (m_f)
	{
		fread(&ch, 1, sizeof(uint8_t), m_f);
		return BKToUNICODE_Byte(ch);
	}

	return 0;
}

bool CBKFile::WriteChar(wchar_t tch)
{
	size_t n = 0;

	if (m_f)
	{
		uint8_t ch = UNICODEtoBK_Byte(tch);
		n = fwrite(&ch, 1, sizeof(uint8_t), m_f);
	}

	return (n != 0);
}
