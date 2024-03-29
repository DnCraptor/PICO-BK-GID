﻿#include "pch.h"
#include "GFResizeDialogHelper.h"
#ifdef UI
GFResizeDialogHelper::GFResizeDialogHelper() :
	m_pDialog(nullptr),
	m_OrigSize(CSize(0, 0)),
	m_OrigWndSize(CSize(0, 0)),
	m_Registry(false),
	m_CurSize(CSize(0, 0)),
	m_DialogID(_T(""))
{
}

GFResizeDialogHelper::~GFResizeDialogHelper()
{
	if (m_DialogID.GetLength() && m_Registry && m_CurSize.cx > 0 && m_CurSize.cy > 0)
	{
		CString regsz;
		regsz.Format(_T("ResizeDlg\\Class:%s"), (LPCTSTR)m_DialogID);
		AfxGetApp()->WriteProfileInt(regsz, _T("CX"), m_CurSize.cx);
		AfxGetApp()->WriteProfileInt(regsz, _T("CY"), m_CurSize.cy);
	}
}

// ---------------------------------------------------
// Init a Resize Helper
// ---------------------------------------------------
void GFResizeDialogHelper::Init(CDialog *pDialog)
{
	m_pDialog = pDialog;
	ResetSize();
}

void GFResizeDialogHelper::ResetSize()
{
	CRect rect;
	m_pDialog->GetWindowRect(&rect);
	m_OrigWndSize = rect.Size();
	m_pDialog->GetClientRect(&rect);
	m_OrigSize = rect.Size();
}

// ---------------------------------------------------
// adding a new Ctrl to the sizer
// ---------------------------------------------------
void GFResizeDialogHelper::AddCtrl(int id, int align, BOOL keep_dx, BOOL keep_dy)
{
	CWnd *pWnd = m_pDialog->GetDlgItem(id);
	ASSERT(pWnd);
	ASSERT(pWnd->GetSafeHwnd());
	ASSERT(::IsWindow(pWnd->GetSafeHwnd()));
	CTRL_ALIGN ctrl;
	ctrl.idWnd = id;
	ctrl.align = align;
	ctrl.keep_dx = keep_dx;
	ctrl.keep_dy = keep_dy;
	pWnd->GetWindowRect(&ctrl.place);
	m_pDialog->ScreenToClient(&ctrl.place);
	m_Ctrls.push_back(ctrl);
}


// ---------------------------------------------------
// OnSize handling of the Child Ctrls
// ---------------------------------------------------
void GFResizeDialogHelper::OnSize(int cx, int cy)
{
	m_CurSize.cx = cx;
	m_CurSize.cy = cy;
	size_t vsize = m_Ctrls.size();

	for (size_t i = 0; i < vsize; ++i)
	{
		CTRL_ALIGN &ctrl = m_Ctrls[i];
		int x = ctrl.place.left;
		int width = ctrl.place.Width();

		if (!ctrl.keep_dx)
		{
			width += (cx - m_OrigSize.cx); // new size increment = dialog increment
		}

		if (ctrl.align & DT_RIGHT)
		{
			x = ctrl.place.right - width + cx - m_OrigSize.cx;
		}
		else if (ctrl.align & DT_CENTER)
		{
			// ctrl center
			int xcenter = (ctrl.place.left + ctrl.place.right) / 2;
			// dialog offset
			xcenter += (cx - m_OrigSize.cx) / 2;
			x = xcenter - width / 2;
		}

		int y = ctrl.place.top;
		int height = ctrl.place.Height();

		if (!ctrl.keep_dy)
		{
			height += (cy - m_OrigSize.cy);
		}

		if (ctrl.align & DT_BOTTOM)
		{
			// y += cy-m_OrigSize.cy;
			y = ctrl.place.bottom - height + cy - m_OrigSize.cy;
		}
		else if (ctrl.align & DT_VCENTER)
		{
			// ctrl center
			int ycenter = (ctrl.place.top + ctrl.place.bottom) / 2;
			// dialog offset
			ycenter += (cy - m_OrigSize.cy) / 2;
			y = ycenter - height / 2;
		}

		width = max(width, 10);
		height = max(height, 10);
		x = max(0, x);
		y = max(0, y);
		m_pDialog->GetDlgItem(ctrl.idWnd)->MoveWindow(x, y, width, height, FALSE);
		// ::MoveWindow(ctrl.pWnd->GetSafeHwnd(), x,y,width, height, TRUE);
	}

	if (m_pDialog && m_pDialog->GetSafeHwnd() && ::IsWindow(m_pDialog->GetSafeHwnd()))
	{
		m_pDialog->RedrawWindow();
	}
}


// ---------------------------------------------------
// get a minmax info, so the dialog won't get smaller than in the resource editor
// ---------------------------------------------------
void GFResizeDialogHelper::GetMinMaxInfo(MINMAXINFO *pMMI)
{
	// maximized rectangle
	pMMI->ptMaxPosition.x = 0;
	pMMI->ptMaxPosition.y = 0;
	pMMI->ptMaxSize.x = ::GetSystemMetrics(SM_CXSCREEN);
	pMMI->ptMaxSize.y = ::GetSystemMetrics(SM_CYSCREEN);
	// maximum size
	pMMI->ptMaxTrackSize = pMMI->ptMaxSize;
	// minimum size
	pMMI->ptMinTrackSize.x = m_OrigWndSize.cx;
	pMMI->ptMinTrackSize.y = m_OrigWndSize.cy;
}

// enable writing / loading current dialog size from registry
void GFResizeDialogHelper::ReadWriteToRegistry()
{
	if (m_pDialog)
	{
		m_DialogID = m_pDialog->GetRuntimeClass()->m_lpszClassName;
		m_Registry = true;
		CSize newsz;
		CString regsz;
		regsz.Format(_T("ResizeDlg\\Class:%s"), (LPCTSTR)m_DialogID);
		newsz.cx = AfxGetApp()->GetProfileInt(regsz, _T("CX"), m_OrigWndSize.cx);
		newsz.cy = AfxGetApp()->GetProfileInt(regsz, _T("CY"), m_OrigWndSize.cy);
		CPoint scr;
		scr.x = ::GetSystemMetrics(SM_CXSCREEN);
		scr.y = ::GetSystemMetrics(SM_CYSCREEN);
		m_pDialog->MoveWindow((scr.x - newsz.cx) / 2, (scr.y - newsz.cy) / 2, newsz.cx, newsz.cy);
	}
}
#endif