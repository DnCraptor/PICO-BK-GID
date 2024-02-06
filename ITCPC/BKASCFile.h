#pragma once
#include "BaseFile.h"

constexpr auto bufSize = 256;

class CBKASCFile :
	public CBaseFile
{
		bool            m_bEof;
		int             m_nInternalBlockNumber;
		int             m_nInternalPos;
		char            m_pWrBuf[bufSize];
		bool            m_bFlushed;

		fs::path        m_strInternalAscName;
		fs::path        m_strInternalCurrentName;
		void            MakeintenalCurrentName();
		bool            OpenInternalFile();

	public:
		CBKASCFile(const fs::path &strName);
		virtual ~CBKASCFile() override;

		virtual bool    Open(bool bWrite) override; // открытие файла, имя которого задано в конструкторе
		virtual wchar_t ReadChar() override;
		virtual bool    WriteChar(wchar_t tch) override;
		virtual bool    WriteWord(uint16_t w);
		virtual long    GetPos() override;
		virtual int     SetPos(long pos, long mode) override;

		virtual bool    isEOF() override
		{
			return m_bEof;
		}

		bool            FlushInternalWrBuff();

};

namespace ITCPC
{
	void TXT2ASC(CBaseFile *pInFile, CBaseFile *pOutFile);
}
