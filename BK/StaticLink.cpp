// StaticLink.cpp : implementation file
//
#ifdef UI
#include "pch.h"
#include "StaticLink.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStaticLink

IMPLEMENT_DYNAMIC(CStaticLink, CStatic)

COLORREF CStaticLink::m_colorUnvisited = RGB(0, 0, 255);        // blue
COLORREF CStaticLink::m_colorVisited = RGB(128, 0, 128);        // purple

CStaticLink::CStaticLink()
	: m_bVisited(false)
	, m_cursor(nullptr)
	, m_cursorArrow(nullptr)
	, m_cursorHand(nullptr)
	, m_tme{}
{
}

CStaticLink::~CStaticLink()
{
	m_Font.DeleteObject();
	m_UnderlineFont.DeleteObject();
}

BEGIN_MESSAGE_MAP(CStaticLink, CStatic)
	ON_WM_CTLCOLOR_REFLECT()
	ON_CONTROL_REFLECT(STN_CLICKED, &CStaticLink::OnStnClicked)
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStaticLink message handlers


void CStaticLink::PreSubclassWindow()
{
	LOGFONT log;
	CFont *pf = GetFont();
	pf->GetLogFont(&log);
	m_Font.CreateFontIndirect(&log);
	log.lfUnderline = 1;
	m_UnderlineFont.CreateFontIndirect(&log);
	ModifyStyle(0, SS_NOTIFY);
	m_cursorHand = ::LoadCursor(nullptr, IDC_HAND);
	m_cursor = m_cursorArrow = ::LoadCursor(nullptr, IDC_ARROW);
	m_tme =
	{
		sizeof(TRACKMOUSEEVENT),
		TME_HOVER | TME_LEAVE,
		GetSafeHwnd(),
		20
	};
	CStatic::PreSubclassWindow();
}


HBRUSH CStaticLink::CtlColor(CDC *pDC, UINT nCtlColor)
{
	pDC->SetTextColor(m_bVisited ? m_colorVisited : m_colorUnvisited);
	pDC->SetBkColor(::GetSysColor(COLOR_BTNFACE));
	// transparent text.
	//pDC->SetBkMode(TRANSPARENT);
	return (HBRUSH)GetStockObject(NULL_BRUSH);
}


void CStaticLink::OnStnClicked()
{
	CString strURL;
	GetWindowText(strURL);
	HINSTANCE h = ShellExecute(nullptr, _T("open"), strURL, nullptr, nullptr, SW_SHOWNORMAL);

	if ((UINT)h > 32)
	{
		m_bVisited = true; // (not really--might not have found link)
		Invalidate(); // repaint to show visited color
	}
}


void CStaticLink::OnMouseMove(UINT nFlags, CPoint point)
{
	m_tme.dwFlags = TME_HOVER | TME_LEAVE;
	m_tme.dwHoverTime = 20;
	::TrackMouseEvent(&m_tme);
	::SetCursor(m_cursor);
}

void CStaticLink::OnMouseHover(UINT nFlags, CPoint point)
{
	m_cursor = m_cursorHand;
	::SetCursor(m_cursor);
	SetFont(&m_UnderlineFont);
	m_tme.dwFlags = TME_CANCEL;
	m_tme.dwHoverTime = HOVER_DEFAULT;
	::TrackMouseEvent(&m_tme);
}

void CStaticLink::OnMouseLeave()
{
	m_cursor = m_cursorArrow;
	::SetCursor(m_cursor);
	SetFont(&m_Font);
	m_tme.dwFlags = TME_CANCEL;
	m_tme.dwHoverTime = 20;
	::TrackMouseEvent(&m_tme);
}


#endif