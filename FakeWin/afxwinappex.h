#pragma once

#define afx_msg

#include <CString.h>
#include <WinUser.h>

#define DECLARE_MESSAGE_MAP()

class CWinAppEx {
	public:
		virtual BOOL InitInstance() = 0;
		virtual int ExitInstance() = 0;
		virtual void PreLoadState() = 0;
		virtual void LoadCustomState() = 0;
		virtual void SaveCustomState() = 0;
		virtual BOOL PreTranslateMessage(MSG *pMsg) = 0;
		virtual int Run() = 0;
        virtual ~CWinAppEx() {}
};

class CMutex {};
