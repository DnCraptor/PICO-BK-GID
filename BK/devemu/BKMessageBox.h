﻿// BKMessageBox.h: interface for the CBKMessageBox class.
//
#pragma once
#include <WinUser.h>
#include <CString.h>
#ifdef UI
#include "resource.h"

class CBKMessageBox
{
		//const static CString m_strCaption;
		HWND m_hwnd;
	public:
		CBKMessageBox()
		{
			m_hwnd = AfxGetMainWnd()->GetSafeHwnd();
		};
		virtual ~CBKMessageBox()
		    = default;
		int Show(CString strText, UINT nType = MB_OK, UINT nIDHelp = 0);
		int Show(LPCTSTR lpszText, UINT nType = MB_OK, UINT nIDHelp = 0);
		int Show(UINT strID, UINT nType = MB_OK, UINT nIDHelp = 0);
};

#else
class CBKMessageBox {
	public:
	CBKMessageBox() {}
	virtual ~CBKMessageBox() = default;
	int Show(UINT strID, UINT nType = MB_OK, UINT nIDHelp = 0);
	int Show(const CString& strText, UINT nType = MB_OK, UINT nIDHelp = 0);
};
#endif

extern CBKMessageBox g_BKMsgBox;
