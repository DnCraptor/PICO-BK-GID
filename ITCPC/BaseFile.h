#pragma once
#include <cerrno>
#include <cstring>

class CBaseFile
{
	protected:
		FILE           *m_f;
		fs::path        m_strFileName;
		size_t          m_nFileLength;
		int             m_nTabWidth; // ширина табулятора по умолчанию для заданного типа файла.
		wchar_t         BKToUNICODE_Byte(uint8_t b);
		uint8_t         UNICODEtoBK_Byte(wchar_t tch);
		FILE           *BKOpen(const fs::path &filepath, const std::wstring &mode);
	public:
		static const uint8_t m_LowRow[32];
		static const wchar_t m_koi8tbl[128];

		CBaseFile(const fs::path &strName);
		virtual ~CBaseFile();

		virtual fs::path &getFileName()
		{
			return m_strFileName;
		}

		virtual const int getTabWidth()
		{
			return m_nTabWidth;
		}

		virtual void    setTabWidth(int n)
		{
			if (2 <= n && n <= 16)
			{
				m_nTabWidth = n;
			}
		}

		virtual bool    Open(bool bWrite = false); // открытие файла, имя которого задано в конструкторе
		virtual void    Close();
		virtual wchar_t ReadChar();
		virtual uint8_t ReadByte();
		virtual uint16_t ReadWord();
		virtual bool    WriteChar(wchar_t ch);
		virtual bool    WriteByte(uint8_t b);
		virtual long    GetPos();
		virtual int     SetPos(long pos, long mode = SEEK_SET);
		virtual bool    isEOF();
		virtual bool    CheckOutFile();
};

