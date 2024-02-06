#include "pch.h"
#include "Config.h"
#include "EnterEditCtrl.h"
#ifdef UI
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CEnterEdit, CEdit)

CEnterEdit::CEnterEdit()
    = default;

CEnterEdit::~CEnterEdit()
    = default;

BEGIN_MESSAGE_MAP(CEnterEdit, CEdit)
	ON_WM_KEYDOWN()
	ON_WM_CHAR()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()


void CEnterEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_RETURN)
	{
		GetParent()->PostMessage(WM_DBG_TOP_ADDRESS_UPDATE, static_cast<WPARAM>(GetDlgCtrlID()));
		return;
	}

	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CEnterEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_RETURN)
	{
		//GetParent()->PostMessage(WM_DBG_TOP_ADDRESS_UPDATE, static_cast<WPARAM>(GetDlgCtrlID()));
		return; // запрет обработки по-умолчанию
	}

	// запрещаем цифры 8 и 9
	if (nChar == 0x38 || nChar == 0x39)
	{
		return;
	}

	CEdit::OnChar(nChar, nRepCnt, nFlags);
}

void CEnterEdit::OnKillFocus(CWnd *pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	GetParent()->PostMessage(WM_DBG_TOP_ADDRESS_UPDATE, static_cast<WPARAM>(GetDlgCtrlID()));
}
#endif