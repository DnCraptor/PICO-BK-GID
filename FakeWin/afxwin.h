#pragma once
#include <windef.h>
#define AFX_NOVTABLE
typedef int INT_PTR;

class CArchive{
    public:
    bool IsStoring() { return false; }
    void WriteCount(INT_PTR) {}
    DWORD_PTR ReadCount() {}
    void Write(const void* lpBuf, UINT nMax) {}
    void EnsureRead(void *lpBuf, UINT nCount) {}
};

class AFX_NOVTABLE CObject {
protected:
   CObject() {}
public:
   virtual ~CObject() = default;
   virtual BOOL IsSerializable() const { return false; }
   virtual void Serialize(CArchive& ar) {}
};

DECLARE_HANDLE(HDC);

class CDC : public CObject {

};

class AFX_NOVTABLE CCmdTarget : public CObject {

};

class CWnd : public CCmdTarget {
public:
    CWnd* GetParent() const { return 0; }
    CWnd* GetParentOwner() const { return 0; }
    HWND GetSafeHwnd() const { return 0; }
};

#include <pico/time.h>

inline static int InterlockedCompareExchange(DWORD volatile *dwLockVar, int, int) { // TODO:
   return false;
}

inline static void Sleep(int ms) { // TODO:
    sleep_ms(ms);
}

inline static DWORD GetTickCount() {
    return time_us_64() / 1000;
}

#define FALSE   0
#define TRUE    1
#define NULL    0

class CFont {};