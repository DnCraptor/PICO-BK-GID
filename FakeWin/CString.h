#pragma once
#include <winnt.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

class CString {
    public:
    typedef char XCHAR;
	typedef LPSTR PXSTR;
    typedef LPSTR PCXSTR;
    CString(): m_pszData(0) {}
    CString(const PCXSTR src) {
        size_t len = strlen(src);
        m_pszData = new XCHAR[len + 1];
        strcpy(m_pszData, src);
    }
    CString(const PCXSTR src, size_t len) {
        m_pszData = new XCHAR[len + 1];
        strncpy(m_pszData, src, len);
        m_pszData[len] = 0;
    }
    CString(const char* src) {
        size_t len = strlen(src);
        m_pszData = new XCHAR[len + 1];
        strcpy(m_pszData, src);
    }
    CString(const DWORD resource): m_pszData(0) {
        // TODO:
    }
    void Empty() {
        if (m_pszData) {
            delete m_pszData;
            m_pszData = 0;
        }
        m_pszData = new XCHAR[1];
        m_pszData[0] = 0;
    }
    BOOL LoadString(int res) {
        // TODO:
        return false;
    }
    void Format(int res, const CString& arg) {
// TODO:
    }
    ~CString() {
        if (m_pszData) {
            delete m_pszData;
            m_pszData = 0;
        }
    }
    PXSTR GetBufferSetLength(size_t sz) {
        if (m_pszData) {
            delete m_pszData;
        }
        m_pszData = new XCHAR[sz];
        m_pszData[sz - 1] = 0;
        return m_pszData;
    }
    void ReleaseBuffer(size_t sz) {
        if (!m_pszData) return;
        m_pszData[sz] = 0;
    }
    PCXSTR GetString() const {
        return( m_pszData );
    }
    bool IsEmpty() const {
        if (m_pszData)
            return m_pszData[0] == 0;
        return true;
    }
    size_t GetLength() const {
        if (m_pszData)
            return strlen(m_pszData);
        return 0;
    }
    CHAR operator[](size_t s) const { return m_pszData[s]; }
    void MakeLower() {
        if (m_pszData) {
            PXSTR p = m_pszData;
            for ( ; *p; ++p) *p = tolower(*p);
        }
    }
    int CollateNoCase(const char* str) const {
        CString s1(str); s1.MakeLower();
        CString s2(GetString()); s2.MakeLower();
        return s1.Compare(s2);
    }
    int CollateNoCase(const CString& str) const {
        CString s1(str); s1.MakeLower();
        CString s2(GetString()); s2.MakeLower();
        return s1.Compare(s2);
    }
    void Format(PCXSTR pszFormat, ...) {
	    va_list argList;
        size_t len = strlen(pszFormat) << 1;
        PXSTR t = new XCHAR[len];
        snprintf(t, len, pszFormat, argList);
        len = strlen(t);
        m_pszData = new XCHAR[len + 1];
        strcpy(m_pszData, t);
        delete t;
    }
    int Compare(const CString& s) const {
        if (!m_pszData && !s.m_pszData) return 0;
        if (!m_pszData) return -1;
        if (!s.m_pszData) return 1;
        return strcmp(m_pszData, s.m_pszData);
    }
    friend CString operator+(const CString& str1, char ch2) {
        CString strResult(str1);
        strResult.AddChar(ch2);
        return( strResult );
    }
    friend CString operator+(const CString& str1, const CString& str2) {
        CString strResult(str1);
        strResult += str2;
        return( strResult );
    }
    friend BOOL operator==(const CString& str1, const CString& str2) {
        return str1.Compare(str2) == 0;
    }
    int Find(char c, size_t from = 0) const {
        if (!m_pszData) return -1;
        PXSTR p = m_pszData + from;
        int res = 0;
        while(*p != c) ++res;
        return res;
    }
    int Find(const CString& str, size_t from = 0) const {
        if (!m_pszData) return -1;
        PXSTR p = m_pszData + from;
        char* r = strstr(p, str.GetString());
        return m_pszData - r;
    }
    char GetAt(size_t d) const {
        return m_pszData[d];
    }
    void AddChar(char c) {
        if (!m_pszData) {
            m_pszData = new XCHAR[2];
            m_pszData[0] = c;
            m_pszData[1] = 0;
        } else {
            size_t len = strlen(m_pszData) + 2;
            PXSTR t = new XCHAR[len];
            strcpy(t, m_pszData);
            t[--len] = 0;
            t[--len] = c;
            delete m_pszData;
            m_pszData = t;
        }
    }
    CString& Trim() {
        if (!m_pszData) return (*this);
        PXSTR p1 = m_pszData;
        while (*p1 == ' ' || *p1 == '\t') { p1++; }
        int idx0 = strlen(m_pszData);
        while(idx0 > 0 && (m_pszData[idx0 - 1] == ' ' || m_pszData[idx0 - 1] == '\t')) { m_pszData[--idx0] = 0; }
        if (m_pszData == p1) return (*this);
        strcpy(m_pszData, p1);
        return (*this);
    }
    CString& Trim(char c) {
        if (!m_pszData) return (*this);
        PXSTR p1 = m_pszData;
        while (*p1 == c) { p1++; }
        int idx0 = strlen(m_pszData);
        while(idx0 > 0 && (m_pszData[idx0 - 1] == c)) { m_pszData[--idx0] = 0; }
        if (m_pszData == p1) return (*this);
        strcpy(m_pszData, p1);
        return (*this);
    }
    CString Left(int i) const {
        CString res(m_pszData, i);
        return res;
    }
    CString Right(int i) const {
        CString res(m_pszData + i);
        return res;
    }
    CString Mid(int i) const {
        CString res(m_pszData + i);
        return res;
    }
    CString Mid(int i, int y) const {
        CString res(m_pszData + i, y - i);
        return res;
    }
    CString& operator+=(const CString& str) {
        size_t len2 = str.GetLength();
        if (!m_pszData) {
            m_pszData = new XCHAR[len2 + 1];
            strcpy(m_pszData, str.GetString());
        } else {
            size_t len1 = strlen(m_pszData);
            PXSTR t = new XCHAR[len1 + len2];
            strcpy(t, m_pszData);
            strcpy(t + len1, str.GetString());
            delete m_pszData;
            m_pszData = t;
        }
        return (*this);
    }
    CString& Insert(int, const char*) {
        // TODO:
        return (*this);
    }
    CString& Replace(char c1, char c2) {
        if (!m_pszData) return (*this);
        PXSTR p = m_pszData;
        while (*p) {
            if (*p == c1) *p = c2;
            p++;
        }
        return (*this);
    }
    CString& SetAt(size_t i, char c) {
        if (!m_pszData) return (*this);
        m_pszData[i] = c;
        return (*this);
    }
    private:
    	PXSTR m_pszData;
};

typedef char TCHAR;

#define LOCALE_NOUSEROVERRIDE 0
class COleDateTime {
    public:
    void ParseDateTime(LPCTSTR, int, int) {}
    CString Format(LPCTSTR pFormat) const { return ""; }
};

extern "C" {
#include <stdlib.h>
}
typedef int                           errno_t;
inline static int _ttoi(const CString& c) { return atoi(c.GetString()); }
inline static int _tstoi(const CString& c) { return atoi(c.GetString()); }
#define _ttof(X) atof(X.GetString())
inline static long _tcstol(const CString& c, char **__restrict __end_PTR, int __base) { return strtol(c.GetString(), __end_PTR, __base); }
