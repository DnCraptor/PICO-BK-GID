#pragma once
#include <winnt.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <algorithm>

class CString {
    std::string s;
public:
    typedef char XCHAR;
	typedef LPSTR PXSTR;
    typedef LPSTR PCXSTR;
    CString(): s() {}
    CString(const std::string& _s): s(_s) {}
    CString(const CString& c): s(c.s) {}
    CString(const PCXSTR src) : s(src) {}
    CString(const char* src) : s(src) {}
    CString(const PCXSTR src, size_t len) : s(src, len) {}
    CString(const DWORD resource) { LoadString((int)resource); }
    inline void Empty() { s.clear(); }
    BOOL LoadString(int res);
    inline void Format(int res, const CString& arg) {
        if (LoadString(res)) {
            char tmp[80];
            snprintf(tmp, 80, arg.s.c_str());
            s = tmp;
        }
    }
    ~CString() {}
    inline PXSTR GetBufferSetLength(size_t sz) { // TODO:
        s.reserve(sz);
        return (PXSTR)s.c_str();
    }
    inline void ReleaseBuffer() {
        // TODO: ?
    }
    inline void ReleaseBuffer(size_t sz) {
        s[sz] = 0;
    }
    inline PCXSTR GetString() const {
        return (PCXSTR)s.c_str();
    }
    inline PXSTR GetBuffer() { return (PXSTR)s.c_str(); } // TODO:
    inline bool IsEmpty() const { return s.empty(); }
    inline size_t GetLength() const { return s.length(); }
    inline CHAR operator[](size_t si) const { return s[si]; }
    inline CString& MakeLower() {
        for(auto& c : s)
            c = tolower(c);
        return *this;
    }
    inline CString& MakeUpper() {
        for(auto& c : s)
            c = toupper(c);
        return *this;
    }
    inline int CollateNoCase(const CString& str) const {
        CString s1(str); s1.MakeLower();
        CString s2(GetString()); s2.MakeLower();
        return s1.Compare(s2);
    }
    inline void Format(PCXSTR pszFormat, ...) {
	    va_list argList;
        size_t len = strlen(pszFormat) << 1;
        PXSTR t = new XCHAR[len];
        snprintf(t, len, pszFormat, argList);
        s = t;
        delete t;
    }
    inline int Compare(const CString& si) const {
        return strcmp(s.c_str(), si.s.c_str());
    }
    inline friend CString operator+(const CString& str1, char ch2) {
        CString strResult(str1);
        strResult.s += ch2;
        return( strResult );
    }
    inline friend CString operator+(const CString& str1, const CString& str2) {
        CString strResult(str1);
        strResult.s += str2.s;
        return( strResult );
    }
    inline friend BOOL operator==(const CString& str1, const CString& str2) {
        return str1.s == str2.s;
    }
    inline int Find(char c, size_t from = 0) const {
        return s.find_first_of(c, from);
    }
    inline int Find(const CString& str, size_t from = 0) const {
        return s.find_first_of(str.s, from);
    }
    inline char GetAt(size_t d) const { return s[d]; }
    inline void AddChar(char c) { s += c; }
    inline CString& TrimRight(char c) {
        s.erase(s.find_last_not_of(c) + 1);
        return (*this);
    }
    inline CString& Trim() {
        static const char* ws = " \t\n\r\f\v";
        s.erase(s.find_last_not_of(ws) + 1); // rtrim
        s.erase(0, s.find_first_not_of(ws)); // ltrim
        return (*this);
    }
    inline CString& Trim(char c) {
        s.erase(s.find_last_not_of(c) + 1); // rtrim
        s.erase(0, s.find_first_not_of(c)); // ltrim
        return (*this);
    }
    inline CString Left(int i) const {
        return CString(s.substr(0, i));
    }
    inline CString Right(int i) const {
        return CString(s.substr(i));
    }
    inline CString Mid(int i) const {
        return CString(s.substr(i));
    }
    inline CString Mid(int i, int y) const {
        return CString(s.substr(i, y));
    }
    inline CString& operator+=(const CString& str) {
        s += str.s;
        return (*this);
    }
    inline CString& Insert(int i, const char* pc) {
        s.insert(i, pc);
        return (*this);
    }
    inline CString& Insert(int i, const CString& cs) {
        s.insert(i, cs.s);
        return (*this);
    }
    inline CString& Replace(char c1, char c2) {
        std::replace(s.begin(), s.end(), c1, c2);
        return (*this);
    }
    inline CString& SetAt(size_t i, char c) {
        s[i] = c;
        return (*this);
    }
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
