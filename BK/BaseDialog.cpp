#ifdef BASE_DIALOG
#include "pch.h"
#include "BaseDialog.h"
#include "Config.h"

#include <afxdatarecovery.h>
#include <..\src\mfc\afximpl.h>

#define DELETE_EXCEPTION(e) do { if(e) { e->Delete(); } } while (0)

IMPLEMENT_DYNAMIC(CBaseDialog, CBasePane);


/////////////////////////////////////////////////////////////////////////////
// CDialog - Modeless and Modal

BEGIN_MESSAGE_MAP(CBaseDialog, CBasePane)
	ON_COMMAND(IDOK, &CBaseDialog::OnOK)
	ON_COMMAND(IDCANCEL, &CBaseDialog::OnCancel)
	ON_MESSAGE(WM_COMMANDHELP, &CBaseDialog::OnCommandHelp)
	ON_MESSAGE(WM_HELPHITTEST, &CBaseDialog::OnHelpHitTest)
	ON_MESSAGE(WM_INITDIALOG, &CBaseDialog::HandleInitDialog)
	ON_WM_SETFONT()
	ON_WM_PAINT()
	ON_WM_QUERYENDSESSION()
	ON_WM_ENDSESSION()
	ON_WM_CTLCOLOR()
	ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()

BOOL CBaseDialog::PreTranslateMessage(MSG *pMsg)
{
	// for modeless processing (or modal)
	ASSERT(m_hWnd != nullptr);

	// allow tooltip messages to be filtered
	if (CBasePane::PreTranslateMessage(pMsg))
	{
		return TRUE;
	}

	// don't translate dialog messages when in Shift+F1 help mode
	CFrameWnd *pFrameWnd = GetTopLevelFrame();

	if (pFrameWnd != nullptr && pFrameWnd->m_bHelpMode)
	{
		return FALSE;
	}

	// fix around for VK_ESCAPE in a multiline Edit that is on a Dialog
	// that doesn't have a cancel or the cancel is disabled.
	if (pMsg->message == WM_KEYDOWN &&
	        (pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_CANCEL) &&
	        (::GetWindowLong(pMsg->hwnd, GWL_STYLE) & ES_MULTILINE) &&
	        _AfxCompareClassName(pMsg->hwnd, _T("Edit")))
	{
		HWND hItem = ::GetDlgItem(m_hWnd, IDCANCEL);

		if (hItem == nullptr || ::IsWindowEnabled(hItem))
		{
			SendMessage(WM_COMMAND, IDCANCEL, 0);
			return TRUE;
		}
	}

	// filter both messages to dialog and from children
	return PreTranslateInput(pMsg);
}


BOOL CBaseDialog::OnCmdMsg(UINT nID, int nCode, void *pExtra, AFX_CMDHANDLERINFO *pHandlerInfo)
{
	if (CBasePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
	{
		return TRUE;
	}

	if ((nCode != CN_COMMAND && nCode != CN_UPDATE_COMMAND_UI) ||
	        !IS_COMMAND_ID(nID) || nID >= 0xf000)
	{
		// control notification or non-command button or system command
		return FALSE;       // not routed any further
	}

	// if we have an owner window, give it second crack
	CWnd *pOwner = GetParent();

	if (pOwner != nullptr)
	{
		TRACE(traceCmdRouting, 1, "Routing command id 0x%04X to owner window.\n", nID);
		ASSERT(pOwner != this);

		if (pOwner->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		{
			return TRUE;
		}
	}

	// last crack goes to the current CWinThread object
	CWinThread *pThread = AfxGetThread();

	if (pThread != nullptr)
	{
		TRACE(traceCmdRouting, 1, "Routing command id 0x%04X to app.\n", nID);

		if (pThread->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		{
			return TRUE;
		}
	}

	TRACE(traceCmdRouting, 1, "IGNORING command id 0x%04X sent to %hs dialog.\n", nID,
	      GetRuntimeClass()->m_lpszClassName);
	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// Modeless Dialogs have 2-phase construction

CBaseDialog::CBaseDialog()
{
	ASSERT(m_hWnd == nullptr);
	Initialize();
}

CBaseDialog::~CBaseDialog()
{
	if (m_hWnd != nullptr)
	{
		TRACE(traceAppMsg, 0, "Warning: calling DestroyWindow in CBaseDialog::~CBaseDialog --\n");
		TRACE(traceAppMsg, 0, "\tOnDestroy or PostNcDestroy in derived class will not be called.\n");
		DestroyWindow();
	}
}

void CBaseDialog::Initialize()
{
	m_nIDHelp = 0;
	m_lpszBarTemplateName = nullptr;
	m_hDialogTemplate = nullptr;
	m_lpDialogTemplate = nullptr;
	m_lpDialogInit = nullptr;
	m_pParentWnd = nullptr;
	m_hWndTop = nullptr;
	m_pOccDialogInfo = nullptr;
	m_bClosedByEndDialog = FALSE;
}

BOOL CBaseDialog::OnQueryEndSession()
{
	CWinApp *pApp = AfxGetApp();

	if (pApp != nullptr && pApp->m_pMainWnd == this)
	{
		if (AfxGetThreadState()->m_lastSentMsg.lParam & ENDSESSION_CLOSEAPP)
		{
			// Restart Manager is querying about restarting the application
			return pApp->SupportsRestartManager();
		}
	}

	return TRUE;
}

void CBaseDialog::OnEndSession(BOOL bEnding)
{
	if (!bEnding)
	{
		return;
	}

	CWinApp *pApp = AfxGetApp();

	if (pApp != nullptr && pApp->m_pMainWnd == this)
	{
		if (AfxGetThreadState()->m_lastSentMsg.lParam & ENDSESSION_CLOSEAPP)
		{
			// Restart Manager is restarting the application
			CDataRecoveryHandler *pHandler = pApp->GetDataRecoveryHandler();

			if (pHandler)
			{
				pHandler->SetShutdownByRestartManager(TRUE);
			}
		}
	}
}

BOOL CBaseDialog::Create(LPCTSTR lpszTemplateName, CWnd *pParentWnd /*= nullptr*/)
{
	ASSERT(IS_INTRESOURCE(lpszTemplateName) ||
	       AfxIsValidString(lpszTemplateName));
	m_lpszBarTemplateName = (LPTSTR)lpszTemplateName;  // used for help

	if (IS_INTRESOURCE(m_lpszBarTemplateName) && m_nIDHelp == 0)
	{
		m_nIDHelp = LOWORD((DWORD_PTR)m_lpszBarTemplateName);
	}

#ifdef _DEBUG

	if (!_AfxCheckDialogTemplate(lpszTemplateName, FALSE))
	{
		ASSERT(false);          // invalid dialog template name
		PostNcDestroy();        // cleanup if Create fails too soon
		return FALSE;
	}

#endif //_DEBUG
	HINSTANCE hInst = AfxFindResourceHandle(lpszTemplateName, RT_DIALOG);
	HRSRC hResource = ::FindResource(hInst, lpszTemplateName, RT_DIALOG);
	HGLOBAL hTemplate = LoadResource(hInst, hResource);
	BOOL bResult = CreateIndirect(hTemplate, pParentWnd, hInst);
	FreeResource(hTemplate);
	return bResult;
}

// for backward compatibility
BOOL CBaseDialog::CreateIndirect(HGLOBAL hDialogTemplate, CWnd *pParentWnd /*= nullptr*/)
{
	return CreateIndirect(hDialogTemplate, pParentWnd, nullptr);
}

BOOL CBaseDialog::CreateIndirect(HGLOBAL hDialogTemplate, CWnd *pParentWnd, HINSTANCE hInst)
{
	ASSERT(hDialogTemplate != nullptr);
	auto lpDialogTemplate = (LPCDLGTEMPLATE)LockResource(hDialogTemplate);
	BOOL bResult = CreateIndirect(lpDialogTemplate, pParentWnd, nullptr, hInst);
	UnlockResource(hDialogTemplate);
	return bResult;
}

// for backward compatibility
BOOL CBaseDialog::CreateIndirect(LPCDLGTEMPLATE lpDialogTemplate, CWnd *pParentWnd /*= nullptr*/, void *lpDialogInit /*= nullptr*/)
{
	return CreateIndirect(lpDialogTemplate, pParentWnd, lpDialogInit, nullptr);
}

BOOL CBaseDialog::CreateIndirect(LPCDLGTEMPLATE lpDialogTemplate, CWnd *pParentWnd, void *lpDialogInit, HINSTANCE hInst)
{
	ASSERT(lpDialogTemplate != nullptr);

	if (pParentWnd == nullptr)
	{
		pParentWnd = AfxGetMainWnd();
	}

	m_lpDialogInit = lpDialogInit;
	return CreateDlgIndirect(lpDialogTemplate, pParentWnd, hInst);
}

BOOL CBaseDialog::SetOccDialogInfo(_AFX_OCC_DIALOG_INFO *pOccDialogInfo)
{
	m_pOccDialogInfo = pOccDialogInfo;
	return TRUE;
}

_AFX_OCC_DIALOG_INFO *CBaseDialog::GetOccDialogInfo()
{
	return m_pOccDialogInfo;
}

/////////////////////////////////////////////////////////////////////////////
// Modal Dialogs

// Modal Constructors just save parameters
CBaseDialog::CBaseDialog(LPCTSTR lpszTemplateName, CWnd *pParentWnd /*= nullptr*/)
{
	ASSERT(IS_INTRESOURCE(lpszTemplateName) ||
	       AfxIsValidString(lpszTemplateName));
	Initialize();
	m_pParentWnd = pParentWnd;
	m_lpszBarTemplateName = (LPTSTR)lpszTemplateName;

	if (IS_INTRESOURCE(m_lpszBarTemplateName))
	{
		m_nIDHelp = LOWORD((DWORD_PTR)m_lpszBarTemplateName);
	}
}

CBaseDialog::CBaseDialog(UINT nIDTemplate, CWnd *pParentWnd /*= nullptr*/)
{
	Initialize();
	m_pParentWnd = pParentWnd;
	m_lpszBarTemplateName = MAKEINTRESOURCE(nIDTemplate);
	m_nIDHelp = nIDTemplate;
}

BOOL CBaseDialog::InitModalIndirect(LPCDLGTEMPLATE lpDialogTemplate, CWnd *pParentWnd /*= nullptr*/, void *lpDialogInit /*= nullptr*/)
{
	// must be called on an empty constructed CDialog
	ASSERT(m_lpszBarTemplateName == nullptr);
	ASSERT(m_lpDialogTemplate == nullptr);
	ASSERT(lpDialogTemplate != nullptr);

	if (m_pParentWnd == nullptr)
	{
		m_pParentWnd = pParentWnd;
	}

	m_lpDialogTemplate = lpDialogTemplate;
	m_lpDialogInit = lpDialogInit;
	return TRUE;    // always ok (DoModal actually brings up dialog)
}

BOOL CBaseDialog::InitModalIndirect(HGLOBAL hDialogTemplate, CWnd *pParentWnd /*= nullptr*/)
{
	// must be called on an empty constructed CDialog
	ASSERT(m_lpszBarTemplateName == nullptr);
	ASSERT(m_hDialogTemplate == nullptr);
	ASSERT(hDialogTemplate != nullptr);

	if (m_pParentWnd == nullptr)
	{
		m_pParentWnd = pParentWnd;
	}

	m_hDialogTemplate = hDialogTemplate;
	return TRUE;    // always ok (DoModal actually brings up dialog)
}

HWND CBaseDialog::PreModal()
{
	// cannot call DoModal on a dialog already constructed as modeless
	ASSERT(m_hWnd == nullptr);
	// allow OLE servers to disable themselves
	CWinApp *pApp = AfxGetApp();

	if (pApp != nullptr)
	{
		pApp->EnableModeless(FALSE);
	}

	// find parent HWND
	HWND hWnd = CWnd::GetSafeOwner_(m_pParentWnd->GetSafeHwnd(), &m_hWndTop);
	// hook for creation of dialog
	AfxHookWindowCreate(this);
	// return window to use as parent for dialog
	return hWnd;
}

void CBaseDialog::PostModal()
{
	AfxUnhookWindowCreate();   // just in case
	Detach();               // just in case

	// re-enable windows
	if (::IsWindow(m_hWndTop))
	{
		::EnableWindow(m_hWndTop, TRUE);
	}

	m_hWndTop = nullptr;
	CWinApp *pApp = AfxGetApp();

	if (pApp != nullptr)
	{
		pApp->EnableModeless(TRUE);
	}
}

INT_PTR CBaseDialog::DoModal()
{
	// can be constructed with a resource template or InitModalIndirect
	ASSERT(m_lpszBarTemplateName != nullptr || m_hDialogTemplate != nullptr ||
	       m_lpDialogTemplate != nullptr);
	// load resource as necessary
	LPCDLGTEMPLATE lpDialogTemplate = m_lpDialogTemplate;
	HGLOBAL hDialogTemplate = m_hDialogTemplate;
	HINSTANCE hInst = AfxGetResourceHandle();

	if (m_lpszBarTemplateName != nullptr)
	{
		hInst = AfxFindResourceHandle(m_lpszBarTemplateName, RT_DIALOG);
		HRSRC hResource = ::FindResource(hInst, m_lpszBarTemplateName, RT_DIALOG);
		hDialogTemplate = LoadResource(hInst, hResource);
	}

	if (hDialogTemplate != nullptr)
	{
		lpDialogTemplate = (LPCDLGTEMPLATE)LockResource(hDialogTemplate);
	}

	// return -1 in case of failure to load the dialog template resource
	if (lpDialogTemplate == nullptr)
	{
		return -1;
	}

	// disable parent (before creating dialog)
	HWND hWndParent = PreModal();
	AfxUnhookWindowCreate();
	BOOL bEnableParent = FALSE;
	CWnd *pMainWnd = nullptr;
	BOOL bEnableMainWnd = FALSE;

	if (hWndParent && hWndParent != ::GetDesktopWindow() && ::IsWindowEnabled(hWndParent))
	{
		::EnableWindow(hWndParent, FALSE);
		bEnableParent = TRUE;
		pMainWnd = AfxGetMainWnd();

		if (pMainWnd && pMainWnd->IsFrameWnd() && pMainWnd->IsWindowEnabled())
		{
			//
			// We are hosted by non-MFC container
			//
			pMainWnd->EnableWindow(FALSE);
			bEnableMainWnd = TRUE;
		}
	}

	TRY
	{
		// create modeless dialog
		AfxHookWindowCreate(this);

		if (!CreateRunDlgIndirect(lpDialogTemplate, CWnd::FromHandle(hWndParent), hInst) && !m_bClosedByEndDialog)
		{
			// If the resource handle is a resource-only DLL, the dialog may fail to launch. Use the
			// module instance handle as the fallback dialog creator instance handle if necessary.
			CreateRunDlgIndirect(lpDialogTemplate, CWnd::FromHandle(hWndParent), AfxGetInstanceHandle());
		}

		m_bClosedByEndDialog = FALSE;
	}
	CATCH_ALL(e)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed.\n");
		DELETE_EXCEPTION(e);
		m_nModalResult = -1;
	}
	END_CATCH_ALL

	if (bEnableMainWnd)
	{
		pMainWnd->EnableWindow(TRUE);
	}

	if (bEnableParent)
	{
		::EnableWindow(hWndParent, TRUE);
	}

	if (hWndParent != nullptr && ::GetActiveWindow() == m_hWnd)
	{
		::SetActiveWindow(hWndParent);
	}

	// destroy modal window
	DestroyWindow();
	PostModal();

	// unlock/free resources as necessary
	if (m_lpszBarTemplateName != nullptr || m_hDialogTemplate != nullptr)
	{
		UnlockResource(hDialogTemplate);
	}

	if (m_lpszBarTemplateName != nullptr)
	{
		FreeResource(hDialogTemplate);
	}

	return m_nModalResult;
}

void CBaseDialog::EndDialog(int nResult)
{
	ASSERT(::IsWindow(m_hWnd));
	m_bClosedByEndDialog = TRUE;

	if (m_nFlags & (WF_MODALLOOP | WF_CONTINUEMODAL))
	{
		EndModalLoop(nResult);
	}

	::EndDialog(m_hWnd, nResult);
}

/////////////////////////////////////////////////////////////////////////////
// Standard CDialog implementation

void CBaseDialog::OnSetFont(CFont *pFont, BOOL bRedraw)
{
	OnSetFont(pFont);
	Default();
}

void CBaseDialog::PreInitDialog()
{
	// ignore it
}

LRESULT CBaseDialog::HandleInitDialog(WPARAM, LPARAM)
{
	PreInitDialog();
	// create OLE controls
	COccManager *pOccManager = afxOccManager;

	if ((pOccManager != nullptr) && (m_pOccDialogInfo != nullptr))
	{
		BOOL bDlgInit;

		if (m_lpDialogInit != nullptr)
			bDlgInit = pOccManager->CreateDlgControls(this, m_lpDialogInit,
			                                          m_pOccDialogInfo);
		else
			bDlgInit = pOccManager->CreateDlgControls(this, m_lpszBarTemplateName,
			                                          m_pOccDialogInfo);

		if (!bDlgInit)
		{
			TRACE(traceAppMsg, 0, "Warning: CreateDlgControls failed during dialog init.\n");
			EndDialog(-1);
			return FALSE;
		}
	}

	// Default will call the dialog proc, and thus OnInitDialog
	LRESULT bResult = Default(); // тут OnInitDialog не вызывается,
	bResult = bResult && OnInitDialog(); // поэтому вызовем его сами

	if (bResult && (m_nFlags & WF_OLECTLCONTAINER))
	{
		if (m_pCtrlCont != nullptr)
		{
			m_pCtrlCont->m_pSiteFocus = nullptr;
		}

		CWnd *pWndNext = GetNextDlgTabItem(nullptr);

		if (pWndNext != nullptr)
		{
			pWndNext->SetFocus();   // UI Activate OLE control
			bResult = FALSE;
		}
	}

	return bResult;
}

void CBaseDialog::OnSetFont(CFont *pFont)
{
	// ignore it
}

BOOL CBaseDialog::OnInitDialog()
{
	// execute dialog RT_DLGINIT resource
	BOOL bDlgInit;

	if (m_lpDialogInit != nullptr)
	{
		bDlgInit = ExecuteDlgInit(m_lpDialogInit);
	}
	else
	{
		bDlgInit = ExecuteDlgInit(m_lpszBarTemplateName);
	}

	if (!bDlgInit)
	{
		TRACE(traceAppMsg, 0, "Warning: ExecuteDlgInit failed during dialog init.\n");
		EndDialog(-1);
		return FALSE;
	}

	LoadDynamicLayoutResource(m_lpszBarTemplateName);

	// transfer data into the dialog from member variables
	if (!UpdateData(FALSE))
	{
		TRACE(traceAppMsg, 0, "Warning: UpdateData failed during dialog init.\n");
		EndDialog(-1);
		return FALSE;
	}

	// enable/disable help button automatically
	CWnd *pHelpButton = GetDlgItem(ID_HELP);

	if (pHelpButton != nullptr)
	{
		pHelpButton->ShowWindow(AfxHelpEnabled() ? SW_SHOW : SW_HIDE);
	}

	CRect rect;
	GetWindowRect(&rect);
	m_sizeDialog = rect.Size();
	return TRUE;    // set focus to first one
}

void CBaseDialog::OnOK()
{
	if (!UpdateData(TRUE))
	{
		TRACE(traceAppMsg, 0, "UpdateData failed during dialog termination.\n");
		// the UpdateData routine will set focus to correct item
		return;
	}

	EndDialog(IDOK);
}

void CBaseDialog::OnCancel()
{
	EndDialog(IDCANCEL);
}

BOOL CBaseDialog::CheckAutoCenter()
{
	// load resource as necessary
	LPCDLGTEMPLATE lpDialogTemplate = m_lpDialogTemplate;
	HGLOBAL hDialogTemplate = m_hDialogTemplate;

	if (m_lpszBarTemplateName != nullptr)
	{
		HINSTANCE hInst = AfxFindResourceHandle(m_lpszBarTemplateName, RT_DIALOG);
		HRSRC hResource = ::FindResource(hInst, m_lpszBarTemplateName, RT_DIALOG);
		hDialogTemplate = LoadResource(hInst, hResource);
	}

	if (hDialogTemplate != nullptr)
	{
		lpDialogTemplate = (LPCDLGTEMPLATE)LockResource(hDialogTemplate);
	}

	// determine if dialog should be centered
	BOOL bResult = TRUE;

	if (lpDialogTemplate != nullptr)
	{
		DWORD dwStyle = lpDialogTemplate->style;
		short x;
		short y;

		if (((DLGTEMPLATEEX *)lpDialogTemplate)->signature == 0xFFFF)
		{
			// it's a DIALOGEX resource
			dwStyle = ((DLGTEMPLATEEX *)lpDialogTemplate)->style;
			x = ((DLGTEMPLATEEX *)lpDialogTemplate)->x;
			y = ((DLGTEMPLATEEX *)lpDialogTemplate)->y;
		}
		else
		{
			// it's a DIALOG resource
			x = lpDialogTemplate->x;
			y = lpDialogTemplate->y;
		}

		bResult = !(dwStyle & (DS_CENTER | DS_CENTERMOUSE | DS_ABSALIGN)) &&
		          x == 0 && y == 0;
	}

	// unlock/free resources as necessary
	if (m_lpszBarTemplateName != nullptr || m_hDialogTemplate != nullptr)
	{
		UnlockResource(hDialogTemplate);
	}

	if (m_lpszBarTemplateName != nullptr)
	{
		FreeResource(hDialogTemplate);
	}

	return bResult; // TRUE if auto-center is ok
}

/////////////////////////////////////////////////////////////////////////////
// CDialog support for context sensitive help.

LRESULT CBaseDialog::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
	if (lParam == 0 && m_nIDHelp != 0)
	{
		lParam = HID_BASE_RESOURCE + m_nIDHelp;
	}

	if (lParam != 0)
	{
		CWinApp *pApp = AfxGetApp();

		if (pApp != nullptr)
		{
			pApp->WinHelpInternal(lParam);
		}

		return TRUE;
	}

	return FALSE;
}

LRESULT CBaseDialog::OnHelpHitTest(WPARAM wParam, LPARAM lParam)
{
	if (m_nIDHelp != 0)
	{
		return HID_BASE_RESOURCE + m_nIDHelp;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CDialog Diagnostics

#ifdef _DEBUG
void CBaseDialog::AssertValid() const
{
	CBasePane::AssertValid();
}

void CBaseDialog::Dump(CDumpContext &dc) const
{
	CBasePane::Dump(dc);
	dc << "m_lpszBarTemplateName = ";

	if (IS_INTRESOURCE(m_lpszBarTemplateName))
	{
		dc << (int)LOWORD((DWORD_PTR)m_lpszBarTemplateName);
	}
	else
	{
		dc << m_lpszBarTemplateName;
	}

	dc << "\nm_hDialogTemplate = " << (void *)m_hDialogTemplate;
	dc << "\nm_lpDialogTemplate = " << (void *)m_lpDialogTemplate;
	dc << "\nm_pParentWnd = " << (void *)m_pParentWnd;
	dc << "\nm_nIDHelp = " << m_nIDHelp;
	dc << "\n";
}
#endif //_DEBUG

inline BOOL CBaseDialog::Create(UINT nIDTemplate, CWnd *pParentWnd)
{
	return CBaseDialog::Create(ATL_MAKEINTRESOURCE(nIDTemplate), pParentWnd);
}
inline void CBaseDialog::MapDialogRect(LPRECT lpRect) const
{
	ASSERT(::IsWindow(m_hWnd)); ::MapDialogRect(m_hWnd, lpRect);
}
inline void CBaseDialog::SetHelpID(UINT nIDR)
{
	m_nIDHelp = nIDR;
}
inline void CBaseDialog::NextDlgCtrl() const
{
	ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_NEXTDLGCTL, 0, 0);
}
inline void CBaseDialog::PrevDlgCtrl() const
{
	ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_NEXTDLGCTL, 1, 0);
}
inline void CBaseDialog::GotoDlgCtrl(CWnd *pWndCtrl)
{
	ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)pWndCtrl->m_hWnd, 1L);
}
inline void CBaseDialog::SetDefID(UINT nID)
{
	ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, DM_SETDEFID, nID, 0);
}
inline DWORD CBaseDialog::GetDefID() const
{
	ASSERT(::IsWindow(m_hWnd)); return DWORD(::SendMessage(m_hWnd, DM_GETDEFID, 0, 0));
}

HBRUSH CBaseDialog::OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor)
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


void CBaseDialog::OnGetMinMaxInfo(MINMAXINFO *lpMMI)
{
	// maximized rectangle
	lpMMI->ptMaxPosition.x = 0;
	lpMMI->ptMaxPosition.y = 0;
	lpMMI->ptMaxSize.x = ::GetSystemMetrics(SM_CXSCREEN);
	lpMMI->ptMaxSize.y = ::GetSystemMetrics(SM_CYSCREEN);
	// maximum size
	lpMMI->ptMaxTrackSize = lpMMI->ptMaxSize;
	// minimum size
	lpMMI->ptMinTrackSize.x = m_sizeDialog.cx;
	lpMMI->ptMinTrackSize.y = m_sizeDialog.cy;
}
#endif
