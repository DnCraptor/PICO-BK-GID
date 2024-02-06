#include "pch.h"
#ifdef UI
#include "DocPaneDlgViewBase.h"
#include "BKMessageBox.h"
#include "Config.h"
#include "Screen_Sizes.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif


/////////////////////////////////////////////////////////////////////////////
// CDocPaneDlgViewBase

IMPLEMENT_DYNAMIC(CDocPaneDlgViewBase, CPaneDialog)

CDocPaneDlgViewBase::CDocPaneDlgViewBase()
    = default;

CDocPaneDlgViewBase::~CDocPaneDlgViewBase()
{
	m_hFont.DeleteObject();
}

BEGIN_MESSAGE_MAP(CDocPaneDlgViewBase, CPaneDialog)
	ON_MESSAGE(WM_INITDIALOG, &CDocPaneDlgViewBase::HandleInitDialog)
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// обработчики сообщений CDocPaneDlgViewBase

void CDocPaneDlgViewBase::OnAfterFloat()
{
	AdjustLayout();
	CPaneDialog::OnAfterFloat();
}

void CDocPaneDlgViewBase::OnAfterDockFromMiniFrame()
{
	AdjustLayout();
	CPaneDialog::OnAfterDockFromMiniFrame();
}

BOOL CDocPaneDlgViewBase::OnShowControlBarMenu(CPoint pt)
{
	CRect rc;
	GetClientRect(&rc);
	ClientToScreen(&rc);

	if (rc.PtInRect(pt))
	{
		return TRUE;    // hide a pane contextmenu on client area
	}

	// show on caption bar
	return CPaneDialog::OnShowControlBarMenu(pt);
}

void CDocPaneDlgViewBase::CorrectHeight()
{
	if (GetSafeHwnd())
	{
		CSize size = m_sizeDefault;
		int c = GetCaptionHeight();
		size.cy += c;
		SetMinSize(size);
	}
}

void CDocPaneDlgViewBase::AdjustLayout()
{
	CorrectHeight();
	CPaneDialog::AdjustLayout();
}

LRESULT CDocPaneDlgViewBase::HandleInitDialog(WPARAM wp, LPARAM lp)
{
	CPaneDialog::HandleInitDialog(wp, lp);
	CRect rect;
	GetWindowRect(&rect);
	m_sizeDefault = rect.Size();
	Global::SetMonospaceFont(GetSafeHwnd(), &m_hFont);
	// Подсказки
	m_ToolTip.Create(this, WS_POPUP | TTS_NOPREFIX | TTS_USEVISUALSTYLE | TTS_ALWAYSTIP | TTS_NOANIMATE);
	m_ToolTip.SetDelayTime(TTDT_AUTOPOP, 20000);
	m_ToolTip.SetDelayTime(TTDT_INITIAL, 300);
	// *** you could change the default settings ***
	CMFCToolTipInfo params;
	params.m_bBoldLabel = TRUE;
	params.m_bDrawDescription = TRUE;
	params.m_bDrawIcon = TRUE;
	params.m_bRoundedCorners = TRUE;
	params.m_bBalloonTooltip = TRUE;
	params.m_bDrawSeparator = FALSE;
	params.m_bVislManagerTheme = TRUE;
	params.m_clrFill = RGB(255, 255, 255);
	params.m_clrFillGradient = RGB(228, 228, 240);
	params.m_clrText = RGB(61, 83, 80);
	params.m_clrBorder = RGB(144, 149, 168);
	m_ToolTip.SetParams(&params);
	m_ToolTip.SetMaxTipWidth(400);
	m_ToolTip.SetFixedWidth(400, 450);
	m_ToolTip.Activate(TRUE);
	return TRUE;
}

BOOL CDocPaneDlgViewBase::PreTranslateMessage(MSG *pMsg)
{
	if (pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MOUSELAST)
	{
		m_ToolTip.RelayEvent(pMsg);
	}

	return CPaneDialog::PreTranslateMessage(pMsg);
}


BOOL CDocPaneDlgViewBase::PreCreateWindow(CREATESTRUCT &cs)
{
	cs.style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	return CPaneDialog::PreCreateWindow(cs);
}

BOOL CDocPaneDlgViewBase::OnEraseBkgnd(CDC *pDC)
{
	DoPaint(pDC);
	return TRUE;
}

HBRUSH CDocPaneDlgViewBase::OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor)
{
#define MAX_CLASS_NAME 255
#define STATIC_CLASS _T("Static")
#define BUTTON_CLASS _T("Button")
#define EDIT_CLASS _T("Edit")
#define SLIDER_CLASS _T("msctls_trackbar32")

	if (nCtlColor == CTLCOLOR_STATIC || nCtlColor == CTLCOLOR_BTN)
	{
		TCHAR lpszClassName[MAX_CLASS_NAME + 1];
		::GetClassName(pWnd->GetSafeHwnd(), lpszClassName, MAX_CLASS_NAME);
		CString strClass = lpszClassName;

		if (strClass == STATIC_CLASS)
		{
			//тут простые статиктексты
			pDC->SetBkMode(TRANSPARENT);
			//pDC->SetTextColor(g_Config.m_clrText);
			return (HBRUSH) ::GetStockObject(HOLLOW_BRUSH);
		}

		if (strClass == BUTTON_CLASS)
		{
			if (nCtlColor == CTLCOLOR_STATIC)
			{
				//вот тут обрабатываются групбоксы, чекбоксы и радиокнопки
				//а надо, групбоксы как-то опознавать, чтоб отдельно обработать
				pDC->SetBkMode(TRANSPARENT);
				//pDC->SetTextColor(g_Config.m_clrText);
#ifdef TARGET_WINXP
				return CBasePane::OnCtlColor(pDC, pWnd, nCtlColor);
#else
				return (HBRUSH) ::GetStockObject(HOLLOW_BRUSH);
#endif
			}

			//тут нажимные кнопки, но к ним всё равно эти стили не применяются
			//pDC->SetTextColor(g_Config.m_clrText);
			pDC->SetBkMode(TRANSPARENT);
			return (HBRUSH) ::GetStockObject(HOLLOW_BRUSH);
		}

		if (strClass == SLIDER_CLASS)
		{
			pDC->SetBkMode(TRANSPARENT);
#ifdef TARGET_WINXP
			return CBasePane::OnCtlColor(pDC, pWnd, nCtlColor);
#else
			return (HBRUSH) ::GetStockObject(HOLLOW_BRUSH);
#endif
		}
	}

	return CBasePane::OnCtlColor(pDC, pWnd, nCtlColor);
}


#endif