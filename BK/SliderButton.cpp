// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.
#ifdef UI
#include "pch.h"
#include "SliderButton.h"
#include "Config.h"
#include "Screen_Sizes.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_SERIAL(CSliderButton, CMFCToolBarButton, 1)

constexpr int nDefaultWidth = 150;

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction


#pragma warning (disable : 4355)

CSliderButton::CSliderButton() :
	m_wndSlider(*this)
{
	m_dwStyle = WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS | TBS_TOOLTIPS | WS_TABSTOP;
	m_dwStyleEx = TBSTYLE_FLAT;
	m_iWidth = nDefaultWidth;
	m_nMin = 0;
	m_nMax = 0;
	m_nValue = 0;
}
//**************************************************************************************
CSliderButton::CSliderButton(UINT uiId,
                             int iImage,
                             DWORD dwStyle,
                             int iWidth) :
	CMFCToolBarButton(uiId, iImage),
	m_wndSlider(*this)
{
	m_dwStyle = dwStyle | WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS | TBS_TOOLTIPS | WS_TABSTOP;
	m_dwStyleEx = TBSTYLE_FLAT;
	m_iWidth = (iWidth == 0) ? nDefaultWidth : iWidth;
	m_nMin = 0;
	m_nMax = 0;
	m_nValue = 0;
}

#pragma warning (default : 4355)

//**************************************************************************************
CSliderButton::~CSliderButton()
{
	if (m_wndSlider.GetSafeHwnd() != nullptr)
	{
		m_wndSlider.DestroyWindow();
	}
}
//**************************************************************************************
SIZE CSliderButton::OnCalculateSize(CDC *pDC, const CSize & /*sizeDefault*/, BOOL bHorz)
{
	if (!IsWindowVisible())
	{
		if (m_wndSlider.GetSafeHwnd() != nullptr)
		{
			m_wndSlider.ShowWindow(SW_HIDE);
		}

		return CSize(0, 0);
	}

	int nPixelW = ::GetDeviceCaps(pDC->GetSafeHdc(), LOGPIXELSX);
	int nPixelH = ::GetDeviceCaps(pDC->GetSafeHdc(), LOGPIXELSY);
	const int nDefaultHeight = g_Config.m_bBigButtons ? 28 : 20;

	if (bHorz)
	{
		if (m_wndSlider.GetSafeHwnd() != nullptr && !IsHidden())
		{
			m_wndSlider.ShowWindow(SW_SHOWNOACTIVATE);
			m_wndSlider.ModifyStyle(TBS_VERT, TBS_HORZ);
		}

		return CSize(::MulDiv(m_iWidth, nPixelW, DEFAULT_DPIX), ::MulDiv(nDefaultHeight, nPixelH, DEFAULT_DPIY));
	}

	if (m_wndSlider.GetSafeHwnd() != nullptr && !IsHidden())
	{
		m_wndSlider.ShowWindow(SW_SHOWNOACTIVATE);
		m_wndSlider.ModifyStyle(TBS_HORZ, TBS_VERT);
	}

	return CSize(::MulDiv(nDefaultHeight, nPixelW, DEFAULT_DPIX), ::MulDiv(m_iWidth, nPixelH, DEFAULT_DPIY));
}
//**************************************************************************************
void CSliderButton::Serialize(CArchive &ar)
{
	CMFCToolBarButton::Serialize(ar);

	if (ar.IsLoading())
	{
		ar >> m_iWidth;
		ar >> m_dwStyle;
		ar >> m_dwStyleEx;
		int     nMin;
		int     nMax;
		int     nValue;
		ar >> nMin;
		ar >> nMax;
		ar >> nValue;
		SetRange(nMin, nMax);
		SetValue(nValue, FALSE);
	}
	else
	{
		ar << m_iWidth;
		ar << m_dwStyle;
		ar << m_dwStyleEx;
		ar << m_nMin;
		ar << m_nMax;
		ar << m_nValue;
	}
}
//**************************************************************************************
void CSliderButton::OnMove()
{
	if (m_wndSlider.GetSafeHwnd() != nullptr &&
	        (m_wndSlider.GetStyle() & WS_VISIBLE))
	{
		m_wndSlider.SetWindowPos(nullptr,
		                         m_rect.left + 1, m_rect.top + 1, m_rect.Width() - 2, m_rect.Height() - 1,
		                         SWP_NOZORDER | SWP_NOACTIVATE);
	}
}
//**************************************************************************************
void CSliderButton::OnSize(int iSize)
{
	m_iWidth = iSize;
	m_rect.right = m_rect.left + m_iWidth;

	if (m_wndSlider.GetSafeHwnd() != nullptr &&
	        (m_wndSlider.GetStyle() & WS_VISIBLE))
	{
		m_wndSlider.SetWindowPos(nullptr,
		                         m_rect.left + 1, m_rect.top + 1, m_rect.Width() - 2, m_rect.Height() - 1,
		                         SWP_NOZORDER | SWP_NOACTIVATE);
	}
}
//**************************************************************************************
void CSliderButton::OnChangeParentWnd(CWnd *pWndParent)
{
	if (m_wndSlider.GetSafeHwnd() != nullptr)
	{
		CWnd *pWndParentCurr = m_wndSlider.GetParent();
		ASSERT(pWndParentCurr != nullptr);

		if (pWndParent != nullptr && pWndParentCurr != nullptr &&
		        pWndParentCurr->GetSafeHwnd() == pWndParent->GetSafeHwnd())
		{
			return;
		}

		m_wndSlider.DestroyWindow();
	}

	if (pWndParent == nullptr || pWndParent->GetSafeHwnd() == nullptr)
	{
		return;
	}

	if (!m_wndSlider.CreateEx(m_dwStyleEx, m_dwStyle, m_rect, pWndParent, m_nID))
	{
		ASSERT(false);
		return;
	}

	m_wndSlider.SetRange(m_nMin, m_nMax);
	m_wndSlider.SetPos(m_nValue);
	m_wndSlider.SetOwner(pWndParent->GetTopLevelFrame());
}
//**************************************************************************************
void CSliderButton::CopyFrom(const CMFCToolBarButton &s)
{
	CMFCToolBarButton::CopyFrom(s);
	auto &src = (const CSliderButton &) s;
	m_dwStyle = src.m_dwStyle;
	m_dwStyleEx = src.m_dwStyleEx;
	m_iWidth = src.m_iWidth;
	m_nMin = src.m_nMin;
	m_nMax = src.m_nMax;
	m_nValue = src.m_nValue;
}
//************************************************************************************
void CSliderButton::SetRange(int iMin, int iMax)
{
	m_nMin = iMin;
	m_nMax = iMax;

	if (m_wndSlider.GetSafeHwnd() != nullptr)
	{
		m_wndSlider.SetRange(iMin, iMax, TRUE);
	}

	CObList listButtons;

	if (CMFCToolBar::GetCommandButtons(m_nID, listButtons) > 0)
	{
		for (POSITION pos = listButtons.GetHeadPosition(); pos != nullptr;)
		{
			auto pSlider = DYNAMIC_DOWNCAST(CSliderButton, listButtons.GetNext(pos));

			if (pSlider != nullptr && pSlider != this)
			{
				pSlider->m_nMin = iMin;
				pSlider->m_nMax = iMax;

				if (pSlider->m_wndSlider.GetSafeHwnd() != nullptr)
				{
					pSlider->m_wndSlider.SetRange(iMin, iMax, TRUE);
				}
			}
		}
	}
}

//************************************************************************************
void CSliderButton::SetValueSt(UINT uiCmd, int iValue, BOOL bNotify)
{
	CObList listButtons;

	if (CMFCToolBar::GetCommandButtons(uiCmd, listButtons) > 0)
	{
		for (POSITION pos = listButtons.GetHeadPosition(); pos != nullptr;)
		{
			auto pSlider = DYNAMIC_DOWNCAST(CSliderButton, listButtons.GetNext(pos));

			if (pSlider != nullptr && pSlider->GetHwnd() != nullptr)
			{
				pSlider->m_nValue = iValue;
				pSlider->m_wndSlider.SetPos(iValue);

				// pSlider->m_wndSlider.UpdateWindow();
				if (bNotify && pSlider->m_wndSlider.GetSafeHwnd() != nullptr)
				{
					pSlider->m_wndSlider.GetOwner()->SendMessage(WM_COMMAND, uiCmd);
				}
			}
		}
	}
}

void CSliderButton::SetValue(int iValue, BOOL bNotify)
{
	m_nValue = iValue;

	if (GetHwnd() != nullptr)
	{
		m_wndSlider.SetPos(iValue);
	}

	CObList listButtons;

	if (CMFCToolBar::GetCommandButtons(m_nID, listButtons) > 0)
	{
		for (POSITION pos = listButtons.GetHeadPosition(); pos != nullptr;)
		{
			auto pSlider = DYNAMIC_DOWNCAST(CSliderButton, listButtons.GetNext(pos));

			if (pSlider != nullptr && pSlider != this)
			{
				pSlider->m_nValue = iValue;

				if (pSlider->GetHwnd() != nullptr)
				{
					pSlider->m_wndSlider.SetPos(iValue);
				}
			}
		}
	}

	if (bNotify && m_wndSlider.GetSafeHwnd() != nullptr)
	{
		m_wndSlider.GetOwner()->SendMessage(WM_COMMAND, m_nID);
	}
}

int CSliderButton::GetValue()
{
	int nRet = -1;

	if (GetHwnd() != nullptr)
	{
		nRet = m_wndSlider.GetPos();
	}

	CObList listButtons;

	if (CMFCToolBar::GetCommandButtons(m_nID, listButtons) > 0)
	{
		for (POSITION pos = listButtons.GetHeadPosition(); pos != nullptr;)
		{
			auto pSlider = DYNAMIC_DOWNCAST(CSliderButton, listButtons.GetNext(pos));

			if (pSlider != nullptr && pSlider != this)
			{
				nRet = pSlider->m_wndSlider.GetPos();
			}
		}
	}

	return nRet;
}


//************************************************************************************
int CSliderButton::GetPosSt(UINT uiCmd)
{
	CObList listButtons;

	if (CMFCToolBar::GetCommandButtons(uiCmd, listButtons) > 0)
	{
		for (POSITION pos = listButtons.GetHeadPosition(); pos != nullptr;)
		{
			auto pSlider = DYNAMIC_DOWNCAST(CSliderButton, listButtons.GetNext(pos));

			if (pSlider != nullptr && pSlider->GetHwnd() != nullptr)
			{
				return pSlider->m_wndSlider.GetPos();
			}
		}
	}

	return -1;
}
//****************************************************************************************
void CSliderButton::OnShow(BOOL bShow)
{
	if (m_wndSlider.GetSafeHwnd() != nullptr)
	{
		if (bShow)
		{
			m_wndSlider.ShowWindow(SW_SHOWNOACTIVATE);
			OnMove();
		}
		else
		{
			m_wndSlider.ShowWindow(SW_HIDE);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CCustomSliderCtrl

IMPLEMENT_DYNAMIC(CCustomSliderCtrl, CSliderCtrl)

CCustomSliderCtrl::CCustomSliderCtrl(CSliderButton &btnSlider) :
	m_btnSlider(btnSlider)
{
}

CCustomSliderCtrl::~CCustomSliderCtrl()
    = default;


BEGIN_MESSAGE_MAP(CCustomSliderCtrl, CSliderCtrl)
	ON_WM_MOUSEMOVE()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCustomSliderCtrl message handlers

void CCustomSliderCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	int nPos = GetPos();
	CSliderCtrl::OnMouseMove(nFlags, point);

	if ((nFlags & MK_LBUTTON) && GetPos() != nPos)
	{
		m_btnSlider.SetValue(GetPos(), TRUE);
	}
}

void CCustomSliderCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	int nPos = GetPos();
	CSliderCtrl::OnKeyDown(nChar, nRepCnt, nFlags);

	if (GetPos() != nPos)
	{
		m_btnSlider.SetValue(GetPos(), TRUE);
	}
}

void CCustomSliderCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	CSliderCtrl::OnLButtonUp(nFlags, point);

	if (IsWindowVisible())
	{
		m_btnSlider.SetValue(GetPos(), TRUE);
	}
}
#endif