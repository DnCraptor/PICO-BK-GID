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

#endif

CBKMessageBox g_BKMsgBox;