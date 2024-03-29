#pragma once

#include <winnt.h>
#include <ff.h>
#include <debug.h>
#include "CPSRAM.h"

#define AFX_DATA

class CFileException{};

class CFile /***: public CObject*/ {
    public:
// Flag values
	enum OpenFlags {
		modeRead =         (int) 0x00000,
		modeWrite =        (int) 0x00001,
		modeReadWrite =    (int) 0x00002,
		shareCompat =      (int) 0x00000,
		shareExclusive =   (int) 0x00010,
		shareDenyWrite =   (int) 0x00020,
		shareDenyRead =    (int) 0x00030,
		shareDenyNone =    (int) 0x00040,
		modeNoInherit =    (int) 0x00080,
#ifdef _UNICODE
		typeUnicode =      (int) 0x00400, // used in derived classes (e.g. CStdioFile) only
#endif
		modeCreate =       (int) 0x01000,
		modeNoTruncate =   (int) 0x02000,
		typeText =         (int) 0x04000, // used in derived classes (e.g. CStdioFile) only
		typeBinary =       (int) 0x08000, // used in derived classes (e.g. CStdioFile) only
		osNoBuffer =       (int) 0x10000,
		osWriteThrough =   (int) 0x20000,
		osRandomAccess =   (int) 0x40000,
		osSequentialScan = (int) 0x80000,
		};

	enum Attribute {
		normal     = 0x00,                // note: not same as FILE_ATTRIBUTE_NORMAL
		readOnly   = FILE_ATTRIBUTE_READONLY,
		hidden     = FILE_ATTRIBUTE_HIDDEN,
		system     = FILE_ATTRIBUTE_SYSTEM,
		volume     = 0x08,
		directory  = FILE_ATTRIBUTE_DIRECTORY,
		archive    = FILE_ATTRIBUTE_ARCHIVE,
		device     = FILE_ATTRIBUTE_DEVICE,
		temporary  = FILE_ATTRIBUTE_TEMPORARY,
		sparse     = FILE_ATTRIBUTE_SPARSE_FILE,
		reparsePt  = FILE_ATTRIBUTE_REPARSE_POINT,
		compressed = FILE_ATTRIBUTE_COMPRESSED,
		offline    = FILE_ATTRIBUTE_OFFLINE,
		notIndexed = FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,
		encrypted  = FILE_ATTRIBUTE_ENCRYPTED
		};

	enum SeekPosition { begin = 0x0, current = 0x1, end = 0x2 };

    static const HANDLE hFileNull;
    FIL m_file;
    bool m_o;
    CFile() : m_o(false) {}
    virtual ~CFile() {
        if (m_o) Close();
    }
    virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError = NULL) {
        UINT mode = FA_READ;
        if (nOpenFlags & CFile::modeWrite) mode |= (FA_WRITE | FA_OPEN_APPEND);
        if (nOpenFlags & CFile::modeCreate) mode |= FA_CREATE_ALWAYS;
        m_o = f_open(&m_file, lpszFileName, mode) == FR_OK;
    //    if (m_o) TRACE_T("[Open] file: '%s'; flags: %08Xh; &m_file: %08Xh; res: %d", lpszFileName, nOpenFlags, &m_file, m_o);
        return m_o;
    }
    virtual size_t GetLength() const { return f_size(&m_file); }
    virtual void Close() {
    //    TRACE_T("[Close] &m_file: %08Xh", &m_file);
        if (m_o) f_close(&m_file);
        m_o = false;
    }
    virtual UINT Read(PSRAM* psram, size_t offset, UINT nCount) {
        UINT rd;
        uint8_t lpBuf[512];
        int cnt = (int)nCount;
        UINT res = 0;
        while(cnt > 0) {
            if (f_read(&m_file, lpBuf, 512, &rd) != FR_OK) {
                TRACE_T("[Read] failed &m_file: %08Xh offset: %08Xh rd: %08Xh", &m_file, offset, rd);
                return res;
            }
            cnt -= rd;
            res += rd;
            for (int i = 0; i < rd; i += 2) {
                psram->set16(offset + i, *(uint16_t*)&lpBuf[i]);
            }
            offset += rd;
        }
        return res;
    }
    virtual UINT Read(void* lpBuf, UINT nCount) {
        UINT rd;
        f_read(&m_file, lpBuf, nCount, &rd);
        return rd;
    }
    virtual UINT GetPosition() const { return f_tell(&m_file); }
    virtual BOOL ReadString(CString& rString) {
        UINT br;
        rString.Empty();
        char c;
        while (f_size(&m_file) >= f_tell(&m_file)) {
            if (f_read(&m_file, &c, 1, &br) != FR_OK) {
            //    TRACE_T("F1 %s", rString.GetString());
                return false;
            }
            if (br != 1) {
            //    TRACE_T("F2 %s", rString.GetString());
                return false;
            }
            if (c == '\r') continue;
            if (c == '\n') {
            //    TRACE_T("T %s", rString.GetString());
                return true;
            }
            rString.AddChar(c);
        }
    }
    virtual BOOL WriteString(const CString& rString) {
        const char * pstr = rString.GetString();
        UINT bw;
        return f_write(&m_file, pstr, rString.GetLength(), &bw) == FR_OK;
    }
    virtual BOOL Write(const CString& rString, size_t sz) {
        UINT bw;
        return f_write(&m_file, rString.GetString(), sz, &bw) == FR_OK;
    }
    virtual BOOL Write(const void* buff, size_t sz) {
        UINT bw;
        return f_write(&m_file, buff, sz, &bw) == FR_OK;
    }
    virtual int Seek(int nOffset, UINT nFrom) {
        if (FR_OK != f_lseek(&m_file, nOffset)) return -1;
        return nOffset;
    }
};
class CMemFile : public CFile {
    public:
    CMemFile(uint8_t *pBuff, UINT nSize, UINT) : CFile() {
        Open("CMemFile.todo", CFile::modeWrite);
    }
};
