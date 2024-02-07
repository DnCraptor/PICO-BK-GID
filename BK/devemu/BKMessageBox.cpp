#include "pch.h"
#include "BKMessageBox.h"
#include "resource.h"
#ifdef UI
//const CString CBKMessageBox::m_strCaption = _T("BKEmu сообщает");

int CBKMessageBox::Show(CString strText, UINT nType, UINT nIDHelp)
{
	CString strCaption(MAKEINTRESOURCE(IDS_MSGBOX_CAPTION));
	return MessageBox(m_hwnd, strText.GetString(), strCaption.GetString(), nType | MB_SETFOREGROUND);
}

int CBKMessageBox::Show(LPCTSTR lpszText, UINT nType, UINT nIDHelp)
{
	// return AfxMessageBox(lpszText, nType | MB_SETFOREGROUND, nIDHelp);
	CString strCaption(MAKEINTRESOURCE(IDS_MSGBOX_CAPTION));
	return MessageBox(m_hwnd, lpszText, strCaption.GetString(), nType | MB_SETFOREGROUND);
}

int CBKMessageBox::Show(UINT strID, UINT nType, UINT nIDHelp)
{
	CString strMessage;
	BOOL bValid = strMessage.LoadString(strID);

	if (!bValid)
	{
		strMessage.Format(IDS_MSGBOX_ERRMSG, strID);
	}

	return Show(strMessage.GetString(), nType, nIDHelp);
}
#else
int CBKMessageBox::Show(UINT strID, UINT nType, UINT nIDHelp) {
	return Show(CString(strID), nType, nIDHelp);
}

int CBKMessageBox::Show(const CString& strText, UINT nType, UINT nIDHelp) {
	FIL file;
	f_open(&file, "\\bk.log", FA_WRITE | FA_OPEN_ALWAYS | FA_OPEN_APPEND);
	UINT bw;
	f_write(&file, strText.GetString(), strText.GetLength(), &bw);
	f_write(&file, "\n", 1, &bw);
	f_close(&file);
	return S_OK;
}
#endif

CBKMessageBox g_BKMsgBox;
