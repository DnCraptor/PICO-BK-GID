
// BKDEDlg.cpp : файл реализации
//

#include "pch.h"
#include "BKDE.h"
#include "BKDEDlg.h"
#include "afxdialogex.h"
#include "FFPickerDlg.h"
#include "RenameDialog.h"
#include "ChangeAddrDialog.h"
#include "StringUtil.h"
#include <afxhtml.h>
#include "fdrawcmd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define STATUSBAR_POS_IMGNAME 0
#define STATUSBAR_POS_OSTYPE 1
#define STATUSBAR_POS_IMGSIZE 2
#define STATUSBAR_POS_SYSTEM 3
#define STATUSBAR_POS_OPENMODE 4

// Диалоговое окно CAboutDlg используется для описания сведений о приложении

class CAboutDlg : public CDialogEx
{
	public:
		CAboutDlg();

// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
		enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV

// Реализация
		CHtmlView *m_Ie;
	protected:
		virtual BOOL OnInitDialog() override;
		DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
	, m_Ie(nullptr)
{
}

void CAboutDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	DWORD dwHandle;
	CString name = CString(theApp.m_pszAppName) + _T(".exe");
	DWORD dwSize = GetFileVersionInfoSize(name.GetString(), &dwHandle);

	if (dwSize)
	{
		auto lpData = std::vector<TCHAR>(dwSize);

		if (lpData.data())
		{
			::GetFileVersionInfo(name.GetString(), dwHandle, dwSize, lpData.data());
			UINT uBufLen;
			VS_FIXEDFILEINFO *lpfi;
			::VerQueryValue(lpData.data(), _T("\\"), reinterpret_cast<void **>(&lpfi), &uBufLen);
			CString s;
			s.Format(_T("BK Disk Explorer v%d.%d.%d.%d "),
			         HIWORD(lpfi->dwFileVersionMS), LOWORD(lpfi->dwFileVersionMS),
			         HIWORD(lpfi->dwFileVersionLS), LOWORD(lpfi->dwFileVersionLS));
#ifdef _WIN64
#define TITLE_SUFFIX _T("x64")
#else
#define TITLE_SUFFIX _T("x86")
#endif
			s += TITLE_SUFFIX;
			SetDlgItemText(IDC_STATIC_VERS, s);
#undef TITLE_SUFFIX
		}
	}

	CRect rect;
	GetDlgItem(IDC_STATIC_ABOUT)->GetWindowRect(&rect);
	ScreenToClient(&rect);
	m_Ie = dynamic_cast<CHtmlView *>(RUNTIME_CLASS(CHtmlView)->CreateObject());

	if (m_Ie)
	{
		if (m_Ie->Create(nullptr, _T("WebAbout"), WS_CHILD | WS_VISIBLE, rect, this, AFX_IDW_PANE_FIRST, nullptr))
		{
			m_Ie->LoadFromResource(IDR_HTML1);
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// Исключение: страница свойств OCX должна возвращать значение FALSE
}

// диалоговое окно CBKDEDlg

UINT CBKDEDlg::auIDStatusBar[] =
{
	ID_SEPARATOR,
	ID_INDICATOR_DOSTYPE,
	ID_INDICATOR_IMGSIZE,
	ID_INDICATOR_SYSTEM,
	ID_INDICATOR_OPENMODE
};


CBKDEDlg::CBKDEDlg(CWnd *pParent /*=nullptr*/)
	: CDialogEx(IDD_BKDE_DIALOG, pParent)
	, m_hAccelTable(nullptr)
	, m_bStatusBarCreated(false)
	, m_nStartModeItemPos(0)
	, m_ApplicationMenu(nullptr)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CBKDEDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_CONTENT, m_ListControl);
	DDX_Control(pDX, IDC_CHECK_BINEXTRACT, m_CheckBinExtract);
	DDX_Control(pDX, IDC_CHECK_LOGEXTRACT, m_CheckLogExtract);
	DDX_Control(pDX, IDC_CHECK_LONGBIN, m_CheckLongBin);
}

BEGIN_MESSAGE_MAP(CBKDEDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_GETMINMAXINFO()
	ON_WM_CLOSE()
	ON_WM_DROPFILES()
	ON_WM_HELPINFO()

	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, &CBKDEDlg::OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, &CBKDEDlg::OnToolTipText)

	ON_MESSAGE(WM_MAKE_DROP, &CBKDEDlg::OnMakeDrop)
	ON_MESSAGE(WM_GET_CMDLINE, &CBKDEDlg::OnGetCmdLine)
	ON_MESSAGE(WM_SEND_PROCESSING, &CBKDEDlg::OnSendItemProcessing)
	ON_MESSAGE(WM_SEND_IMGNAMEPRC, &CBKDEDlg::OnSendImageProcessing)
	ON_MESSAGE(WM_SEND_ERRORNUM, &CBKDEDlg::OnSendErrorNum)
	ON_MESSAGE(WM_SEND_MESSAGEBOX, &CBKDEDlg::OnSendMessageBox)
	ON_MESSAGE(WM_OUT_CURR_FILE_PATH, &CBKDEDlg::OnOutCurrentFilePath)
	ON_MESSAGE(WM_OUT_CURR_IMG_INFO, &CBKDEDlg::OnOutCurrentImageInfo)
	ON_MESSAGE(WM_OUT_OF_IMAGE, &CBKDEDlg::OnOutImage)
	ON_MESSAGE(WM_PUT_INTO_LD, &CBKDEDlg::OnPutIntoLD)
	ON_MESSAGE(WM_SEND_ENABLE_BUTTON, &CBKDEDlg::OnSendEnableButton)
	ON_MESSAGE(WM_GET_ENABLE_BUTTON, &CBKDEDlg::OnGetEnableButton)

	ON_BN_CLICKED(IDC_BUTTON_ABOUT, &CBKDEDlg::OnBnClickedButtonAbout)
	ON_BN_CLICKED(IDC_MFCMENUBUTTON_OPEN, &CBKDEDlg::OnBnClickedMfcmenubuttonOpen)
	ON_BN_CLICKED(IDC_BUTTON_VIEW, &CBKDEDlg::OnBnClickedButtonView)
	ON_BN_CLICKED(IDC_BUTTON_VIEW_AS_SPR, &CBKDEDlg::OnBnClickedButtonViewSprite)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CBKDEDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_EXTRACT, &CBKDEDlg::OnBnClickedButtonExtract)
	ON_BN_CLICKED(IDC_BUTTON_RENAME, &CBKDEDlg::OnBnClickedButtonRename)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, &CBKDEDlg::OnBnClickedButtonDelete)

	ON_BN_CLICKED(IDC_CHECK_BINEXTRACT, &CBKDEDlg::OnBnClickedCheckBinextract)
	ON_BN_CLICKED(IDC_CHECK_LOGEXTRACT, &CBKDEDlg::OnBnClickedCheckLogextract)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE, &CBKDEDlg::OnBnClickedClose)

	ON_BN_CLICKED(IDC_CHECK_LONGBIN, &CBKDEDlg::OnBnClickedCheckLongbin)

	ON_COMMAND(ID_CONTEXT_RENAME, &CBKDEDlg::OnContextMenuRename)
	ON_COMMAND(ID_CONTEXT_CHGADDR, &CBKDEDlg::OnContextChangeAddr)
	ON_COMMAND(ID_CONTEXT_DELETE, &CBKDEDlg::OnContextDelete)
	ON_COMMAND(ID_CONTEXT_EXTRACT, &CBKDEDlg::OnContextExtract)
	ON_COMMAND(ID_CONTEXT_VIEWASTEXT, &CBKDEDlg::OnContextViewastext)
	ON_COMMAND(ID_CONTEXT_VIEWASSPRITE, &CBKDEDlg::OnContextViewassprite)
END_MESSAGE_MAP()

BOOL CBKDEDlg::OnToolTipText(UINT, NMHDR *pNMHDR, LRESULT *pResult)
{
	ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);

	// if there is a top level routing frame then let it handle the message
	if (GetRoutingFrame() != nullptr)
	{
		return FALSE;
	}

	// to be thorough we will need to handle UNICODE versions of the message also !!
	auto pTTTA = reinterpret_cast<TOOLTIPTEXTA *>(pNMHDR);
	auto pTTTW = reinterpret_cast<TOOLTIPTEXTW *>(pNMHDR);
	constexpr auto nBufSize = 512;
	TCHAR szFullText[nBufSize];
	CString strTipText;
	UINT_PTR nID = pNMHDR->idFrom;

	if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
	        pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool
		nID = ::GetDlgCtrlID(reinterpret_cast<HWND>(nID));
	}

	if (nID != 0) // will be zero on a separator
	{
		AfxLoadString(nID, szFullText, nBufSize);
		strTipText = szFullText;
#ifndef _UNICODE

		if (pNMHDR->code == TTN_NEEDTEXTA)
		{
			lstrcpyn(pTTTA->szText, strTipText, sizeof(pTTTA->szText));
		}
		else
		{
			_mbstowcsz(pTTTW->szText, strTipText, sizeof(pTTTW->szText));
		}

#else

		if (pNMHDR->code == TTN_NEEDTEXTA)
		{
			_wcstombsz(pTTTA->szText, strTipText, sizeof(pTTTA->szText));
		}
		else
		{
			lstrcpyn(pTTTW->szText, strTipText, sizeof(pTTTW->szText) / sizeof(pTTTW->szText[0]));
		}

#endif
		*pResult = 0;
		// bring the tooltip window above other popup windows
		::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
		               SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOOWNERZORDER);
		return TRUE;
	}

	return FALSE;
}

// обработчики сообщений CBKDEDlg

BOOL CBKDEDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// Добавление пункта "О программе..." в системное меню.
	// IDM_ABOUTBOX должен быть в пределах системной команды.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);
	CMenu *pSysMenu = GetSystemMenu(FALSE);

	if (pSysMenu != nullptr)
	{
		CString strAboutMenu;
		BOOL bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);

		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Задаёт значок для этого диалогового окна.  Среда делает это автоматически,
	//  если главное окно приложения не является диалоговым
	SetIcon(m_hIcon, TRUE);         // Крупный значок
	SetIcon(m_hIcon, FALSE);        // Мелкий значок
	m_strImgName.clear();
	PreUseLoadSettings();
	m_hAccelTable = ::LoadAccelerators(::AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_ACCELERATOR1));

	// создаём статусбар внизу диалогового окна
	if (m_StatusBar.Create(this))
	{
		m_StatusBar.SetIndicators(auIDStatusBar, sizeof(auIDStatusBar) / sizeof(UINT));
		// Make a sunken or recessed border around the first pane
		m_StatusBar.SetPaneInfo(STATUSBAR_POS_IMGNAME, m_StatusBar.GetItemID(STATUSBAR_POS_IMGNAME), SBPS_STRETCH, 0);
		m_StatusBar.SetPaneInfo(STATUSBAR_POS_OSTYPE, m_StatusBar.GetItemID(STATUSBAR_POS_OSTYPE), SBPS_NORMAL, 70);
		m_StatusBar.SetPaneInfo(STATUSBAR_POS_IMGSIZE, m_StatusBar.GetItemID(STATUSBAR_POS_IMGSIZE), SBPS_NORMAL, 50);
		m_StatusBar.SetPaneInfo(STATUSBAR_POS_SYSTEM, m_StatusBar.GetItemID(STATUSBAR_POS_SYSTEM), SBPS_NORMAL, 70);
		m_StatusBar.SetPaneInfo(STATUSBAR_POS_OPENMODE, m_StatusBar.GetItemID(STATUSBAR_POS_OPENMODE), SBPS_NORMAL, 24);
		m_bStatusBarCreated = true;
	}

	// We need to resize the dialog to make room for control bars.
	// First, figure out how big the control bars are.
	CRect rcClientStart;
	CRect rcClientNow;
	GetClientRect(rcClientStart);
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST,
	               0, RepositionFlags::reposQuery, rcClientNow);
	// Now move all the controls so they are in the same relative
	// position within the remaining client area as they would be
	// with no control bars.
//  CPoint ptOffset(rcClientNow.left - rcClientStart.left,
//                  rcClientNow.top - rcClientStart.top);
//  CRect rcChild;
//  CWnd *pwndChild = GetWindow(GW_CHILD);
//
//  while (pwndChild)
//  {
//      pwndChild->GetWindowRect(rcChild);
//      ScreenToClient(rcChild);
//      rcChild.OffsetRect(ptOffset);
//      pwndChild->MoveWindow(rcChild, FALSE);
//      pwndChild = pwndChild->GetNextWindow();
//  }
	// Adjust the dialog window dimensions
	CRect rcWindow;
	GetWindowRect(rcWindow);
	// зададим ограничение минимального размера диалога
	m_OrigWndSize = rcWindow.Size();
	//  rcWindow.right += rcClientStart.Width() - rcClientNow.Width();
//  rcWindow.bottom += (rcClientStart.Height() - rcClientNow.Height());
//  MoveWindow(rcWindow, FALSE);
	// And position the control bars
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);
	m_BKImage.AttachView(&m_ListControl);
	m_ListControl.Init(CBKListCtrl::MODE_CTRL::START);
	FillMenuButton();
	// m_CheckLongBin.SetCheck(BST_CHECKED); // включаем формат длинного бин
	OnBnClickedCheckLongbin();
	OnBnClickedCheckBinextract();
	m_nStartModeItemPos = 0;
	ChangeCurrDir();
	DisableControls();

	if (lstrlen(theApp.m_lpCmdLine)) // если в командной строке что-то есть,
	{
		// AfxMessageBox(theApp.m_lpCmdLine);
		CString str = CString(theApp.m_lpCmdLine);
		str.Trim();
		str.Trim(_T('"'));

		if (!str.IsEmpty()) // тут просто уберём кавычки, чтобы проверить, а не пустая ли командная строка?
		{
			PostMessage(WM_GET_CMDLINE); // сообщим, что это что-то надо бы попробовать открыть.
			// но если там какая-то херня, то прога будет ругаться.
		}
	}

	return TRUE;  // возврат значения TRUE, если фокус не передан элементу управления
}

BOOL CBKDEDlg::PreTranslateMessage(MSG *pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN)
		{
			m_ListControl.OnKeyDown(UINT(pMsg->wParam), 0, 0);
			return TRUE;
		}
	}

	if (m_hAccelTable)
	{
		if (::TranslateAccelerator(m_hWnd, m_hAccelTable, pMsg))
		{
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CBKDEDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// При добавлении кнопки свёртывания в диалоговое окно нужно воспользоваться приведённым ниже кодом,
// чтобы нарисовать значок.  Для приложений MFC, использующих модель документов или представлений,
// это автоматически выполняется рабочей областью.

void CBKDEDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // контекст устройства для рисования
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
		// Выравнивание значка по центру клиентского прямоугольника
		const int cxIcon = GetSystemMetrics(SM_CXICON);
		const int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		const int x = (rect.Width() - cxIcon + 1) / 2;
		const int y = (rect.Height() - cyIcon + 1) / 2;
		// Нарисуйте значок
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// Система вызывает эту функцию для получения отображения курсора при перемещении
//  свёрнутого окна.
HCURSOR CBKDEDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CBKDEDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	if (m_bStatusBarCreated)
	{
		// вместе с изменением размеров окна, изменяем размеры и положение статус бара
		// чтобы он всегда внизу окна был
		CRect rcWrect, rcSrect;
		GetClientRect(rcWrect);
		m_StatusBar.GetClientRect(rcSrect);
		const int w = rcWrect.Width();
		const int h = rcSrect.Height();
		const int x = rcSrect.left;
		const int y = rcWrect.bottom - h;
		m_StatusBar.SetWindowPos(nullptr, x, y, w, h, SWP_NOACTIVATE | SWP_NOZORDER);
	}
}


void CBKDEDlg::OnGetMinMaxInfo(MINMAXINFO *lpMMI)
{
	// maximized rectangle
	lpMMI->ptMaxPosition = {0, 0};
	lpMMI->ptMaxSize = { ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN) };
	// maximum size
	lpMMI->ptMaxTrackSize = lpMMI->ptMaxSize;
	// minimum size
	lpMMI->ptMinTrackSize = { m_OrigWndSize.cx, m_OrigWndSize.cy };
}



void CBKDEDlg::OnOK()
{
	// клавиша ВВОД - ничего не делает
	//CDialogEx::OnOK();
}


void CBKDEDlg::OnCancel()
{
	// клавиша ESC - ничего не делает
	//CDialogEx::OnCancel();
}

void CBKDEDlg::OnClose()
{
	PreCloseStoreSettings();
	CDialogEx::EndDialog(IDOK);
}

void CBKDEDlg::OnBnClickedClose()
{
	PreCloseStoreSettings();
	CDialogEx::EndDialog(IDOK);
}

void CBKDEDlg::OnBnClickedCheckBinextract()
{
	const bool bStatus = !!m_CheckBinExtract.GetCheck();
	m_BKImage.SetCheckBinExtractStatus(bStatus);
	m_CheckLongBin.EnableWindow(bStatus);
	m_ListControl.SetFocus();
}
void CBKDEDlg::OnBnClickedCheckLongbin()
{
	m_BKImage.SetCheckUseLongBinStatus(!!m_CheckLongBin.GetCheck());
	m_ListControl.SetFocus();
}
void CBKDEDlg::OnBnClickedCheckLogextract()
{
	m_BKImage.SetCheckLogExtractStatus(!!m_CheckLogExtract.GetCheck());
	m_ListControl.SetFocus();
}

void CBKDEDlg::OnSetFocus(CWnd *pOldWnd)
{
	CDialogEx::OnSetFocus(pOldWnd);
	m_ListControl.SetFocus();
}

int CBKDEDlg::ShowMessageBox(UINT strID, UINT nType)
{
	CString strMsg = CString(MAKEINTRESOURCE(strID));
	return ShowMessageBox(strMsg, nType);
}

int CBKDEDlg::ShowMessageBox(CString &str, UINT nType)
{
	CString strCapt = CString(MAKEINTRESOURCE(IDS_MSGCAPT));
	return MessageBox(str, strCapt, nType | MB_SETFOREGROUND);
}

int CBKDEDlg::SettingCheckState(const CString &str)
{
	return (str.CompareNoCase(_T("Yes")) && str.CompareNoCase(_T("1"))) ? BST_UNCHECKED : BST_CHECKED;
}

CString CBKDEDlg::CheckStateSet(int nCheck)
{
	return !!nCheck ? _T("Yes") : _T("No");
}

void CBKDEDlg::PreUseLoadSettings()
{
	CString str;
	m_Settings.Init();
	m_Settings.LoadStringValue(CSettings::IMAGE_PATH, m_strImageDir);
	m_Settings.LoadStringValue(CSettings::STORE_PATH, m_strStorePath);
	m_Settings.LoadStringValue(CSettings::LOAD_PATH, m_strLoadPath);
	m_Settings.LoadStringValue(CSettings::USE_LONGBIN, str);
	m_CheckLongBin.SetCheck(SettingCheckState(str));
	OnBnClickedCheckLongbin();
	m_Settings.LoadStringValue(CSettings::USE_BIN_FORMAT, str);
	m_CheckBinExtract.SetCheck(SettingCheckState(str));
	OnBnClickedCheckBinextract();
	m_Settings.LoadStringValue(CSettings::USE_LOG_EXTRACT, str);
	m_CheckLogExtract.SetCheck(SettingCheckState(str));
	OnBnClickedCheckLogextract();
	m_Settings.Done();
}

void CBKDEDlg::PreCloseStoreSettings()
{
	m_Settings.Init();
	m_Settings.SaveStringValue(CSettings::IMAGE_PATH, m_strImageDir);
	m_Settings.SaveStringValue(CSettings::STORE_PATH, m_strStorePath);
	m_Settings.SaveStringValue(CSettings::LOAD_PATH, m_strLoadPath);
	m_Settings.SaveStringValue(CSettings::USE_BIN_FORMAT, CheckStateSet(m_CheckBinExtract.GetCheck()));
	m_Settings.SaveStringValue(CSettings::USE_LONGBIN, CheckStateSet(m_CheckLongBin.GetCheck()));
	m_Settings.SaveStringValue(CSettings::USE_LOG_EXTRACT, CheckStateSet(m_CheckLogExtract.GetCheck()));
	m_Settings.Done();
	m_BKImage.Close();

	if (m_ApplicationMenu)
	{
		DestroyMenu(m_ApplicationMenu);
		m_ApplicationMenu = nullptr;
	}
}

void CBKDEDlg::FillStatusBar()
{
	// выведем имя образа
	m_StatusBar.SetPaneText(STATUSBAR_POS_IMGNAME, m_strImgName.c_str());
	// выведем тип ОС
	CString str = CString(m_BKImage.GetImgFormatName().c_str());
	m_StatusBar.SetPaneText(STATUSBAR_POS_OSTYPE, str);
	// выведем размер
	CString strF = CString(MAKEINTRESOURCE(IDS_IMAGE_SIZE));
	str.Format(strF, m_sParseResult.nImageSize / 1024);
	m_StatusBar.SetPaneText(STATUSBAR_POS_IMGSIZE, str);
	// выведем загрузочный или нет
	str.Empty();

	if (m_sParseResult.bImageBootable)
	{
		str = CString(MAKEINTRESOURCE(IDS_IMGTYPE_SYSTEM));
	}

	m_StatusBar.SetPaneText(STATUSBAR_POS_SYSTEM, str);
	str = (m_BKImage.GetImageOpenStatus()) ? _T("R") : _T("RW");
	m_StatusBar.SetPaneText(STATUSBAR_POS_OPENMODE, str);
}

void CBKDEDlg::ClearStatusBar()
{
	m_StatusBar.SetPaneText(STATUSBAR_POS_IMGNAME, _T(""));
	m_StatusBar.SetPaneText(STATUSBAR_POS_OSTYPE, _T(""));
	m_StatusBar.SetPaneText(STATUSBAR_POS_IMGSIZE, _T(""));
	m_StatusBar.SetPaneText(STATUSBAR_POS_SYSTEM, _T(""));
	m_StatusBar.SetPaneText(STATUSBAR_POS_OPENMODE, _T(""));
}


void CBKDEDlg::EnableButton(UINT nID, BOOL bStatus)
{
	switch (nID)
	{
		case ID_CONTEXT_CHGADDR: // не диалоговый контрол.
			break;

		default:
			GetDlgItem(nID)->EnableWindow(bStatus);
	}

	m_BtnCurrStatus.insert_or_assign(nID, bStatus);
}


void CBKDEDlg::DisableControls()
{
	EnableButton(IDC_BUTTON_VIEW, FALSE);
	EnableButton(IDC_BUTTON_VIEW_AS_SPR, FALSE);
	EnableButton(IDC_BUTTON_ADD, FALSE);
	EnableButton(IDC_BUTTON_RENAME, FALSE);
	EnableButton(IDC_BUTTON_EXTRACT, FALSE);
	EnableButton(IDC_BUTTON_DELETE, FALSE);
	SetDlgItemText(IDC_STATIC_INFO, CString(MAKEINTRESOURCE(IDS_INFO_NOIMAGE)));
}

// разрешение/запрещение кнопок.
// вход: wParam - ID контрола
//      lParam - состояние: true - разрешить, false - запретить
LRESULT CBKDEDlg::OnSendEnableButton(WPARAM wParam, LPARAM lParam)
{
	const auto n = static_cast<UINT>(wParam);
	const auto f = static_cast<BOOL>(lParam);

	if (m_ButtonsStatus.count(n)) // если ключ есть
	{
		if (m_ButtonsStatus[n]) // и флаг разрешения есть
		{
			EnableButton(n, f); // то только тогда выполняем
		}
	}
	else
	{
		EnableButton(n, f); // если ключа нету, выполняем всегда
	}

	return S_OK;
}

// разрешение/запрещение кнопок.
// вход: wParam - ID контрола
//      lParam - состояние: true - разрешить, false - запретить
void CBKDEDlg::SetEnableButtons(uint32_t flg)
{
	struct pair
	{
		UINT n;
		uint32_t f;
	};
	static const pair IDs[7] =
	{
		{ IDC_BUTTON_EXTRACT, ENABLE_BUTON_EXTRACT},
		{ IDC_BUTTON_VIEW, ENABLE_BUTON_VIEW},
		{ IDC_BUTTON_VIEW_AS_SPR, ENABLE_BUTON_VIEWSPR},
		{ IDC_BUTTON_ADD, ENABLE_BUTON_ADD},
		{ IDC_BUTTON_DELETE, ENABLE_BUTON_DEL },
		{ IDC_BUTTON_RENAME, ENABLE_BUTON_REN },
		{ ID_CONTEXT_CHGADDR, ENABLE_CONTEXT_CHANGEADDR },
	};

	for (auto &ID : IDs)
	{
		BOOL f = !!(flg & ID.f);
		m_ButtonsStatus.insert(std::make_pair(ID.n, f));
		EnableButton(ID.n, f);
	}
}

LRESULT CBKDEDlg::OnGetEnableButton(WPARAM wParam, LPARAM lParam)
{
	const auto n = static_cast<UINT>(wParam);
	const auto r = static_cast<LRESULT>(m_BtnCurrStatus[n]);
	return r;
}


void CBKDEDlg::OnBnClickedButtonAbout()
{
	CAboutDlg dlgAbout;
	dlgAbout.DoModal();
	m_ListControl.SetFocus();
}

void CBKDEDlg::OnBnClickedButtonOpen()
{
	CString strFilterImg = CString(MAKEINTRESOURCE(IDS_FILEFILTER_IMG));
	CFileDialog dlg(TRUE, nullptr, nullptr,
	                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_EXPLORER,
	                strFilterImg, this);

	if (!m_strImageDir.empty()) // зададим начальную директорию.
	{
		dlg.GetOFN().lpstrInitialDir = m_strImageDir.c_str();    // это будет прошлая директория
	}

	if (dlg.DoModal() == IDOK)
	{
		const fs::path strName(dlg.GetPathName().GetString());

		if (m_strImgName.compare(strName) == 0)
		{
			// этот файл уже открыт, незачем второй раз открывать.
			return;
		}

		m_strImgName = strName;
		m_strImageDir = m_strImgName.parent_path();
		OpenImage();
	}

	m_ListControl.SetFocus();
}

void CBKDEDlg::OnBnClickedButtonView()
{
	// акселераторам пофиг на статус disabled windows
	// они всё равно вызывают эти функции
	if (GetDlgItem(IDC_BUTTON_VIEW)->IsWindowEnabled())
	{
		m_BKImage.ViewSelected(true);
	}

	m_ListControl.SetFocus();
}

void CBKDEDlg::OnBnClickedButtonViewSprite()
{
	if (GetDlgItem(IDC_BUTTON_VIEW_AS_SPR)->IsWindowEnabled())
	{
		m_BKImage.ViewSelected(false);
	}

	m_ListControl.SetFocus();
}

void CBKDEDlg::OnBnClickedButtonAdd()
{
	if (GetDlgItem(IDC_BUTTON_ADD)->IsWindowEnabled())
	{
		CFFPickerDlg dlg(this);

		if (!m_strLoadPath.empty())
		{
			// установка каталога по умолчанию
			dlg.SetStartFolder(CString(m_strLoadPath.c_str()));
		}

		if (dlg.DoModal() == IDOK)
		{
			bool bPathExtracted = false;
			CString strPath, strDrive;
			ADDOP_RESULT res;
			const INT_PTR nSel = dlg.m_SelectedItemList.GetCount();

			if (nSel)
			{
				CBKImage::ItemPanePos pp = m_BKImage.GetTopItemIndex();

				for (INT_PTR i = 0; i < nSel; ++i)
				{
					const fs::path str(dlg.m_SelectedItemList[i].GetString());

					if (!bPathExtracted)
					{
						bPathExtracted = true;
						m_strLoadPath = str.parent_path();
					}

					// GetFileAttributes(str.c_str());

					if (fs::exists(str))
					{
						res = SendObject(str);

						if (res.bFatal)
						{
							break;
						}
					}
				}

				m_ListControl.DeleteAllItems();
				m_BKImage.ReadCurrentDir(pp);
			}
		}
	}

	m_ListControl.SetFocus();
}

void CBKDEDlg::OnBnClickedButtonRename()
{
	if (GetDlgItem(IDC_BUTTON_RENAME)->IsWindowEnabled())
	{
		const UINT uSelectedCount = m_ListControl.GetSelectedCount();
		int  nItem = -1;

		if (uSelectedCount > 0)
		{
			// если что-то выделенное есть
			for (UINT i = 0; i < uSelectedCount; ++i) // обработаем все выделенные элементы
			{
				nItem = m_ListControl.GetNextItem(nItem, LVNI_SELECTED);
				// и просто переименуем вручную, без всяких масок и шаблонов
				const auto fr = reinterpret_cast<BKDirDataItem *>(m_ListControl.GetItemData(nItem));
				CRenameDlg dlg(fr->strName);

				if (dlg.DoModal() == IDOK)
				{
					const std::wstring newName = dlg.GetName();

					if (!newName.empty() && (newName != fr->strName))
					{
						fr->strName = newName;
						m_BKImage.RenameRecord(fr);
					}
				}
			}
		}
	}

	m_ListControl.SetFocus();
}

void CBKDEDlg::OnBnClickedButtonExtract()
{
	if (GetDlgItem(IDC_BUTTON_EXTRACT)->IsWindowEnabled())
	{
		auto szOutPath = std::vector<TCHAR>(_MAX_PATH);

		if (GetFolder(szOutPath.data()))
		{
			m_strStorePath = fs::path(szOutPath.data());
			m_BKImage.ExtractSelected(m_strStorePath);
		}
	}

	m_ListControl.SetFocus();
}

void CBKDEDlg::OnBnClickedButtonDelete()
{
	if (GetDlgItem(IDC_BUTTON_DELETE)->IsWindowEnabled())
	{
		m_BKImage.DeleteSelected();
	}

	m_ListControl.SetFocus();
}


// рекурсивно передаём всё, что можем
ADDOP_RESULT CBKDEDlg::SendObject(const fs::path &strName)
{
	ADDOP_RESULT res, dir_res;

	if (fs::is_directory(strName))
	{
		// если директория
		// сперва передадим имя директории, по идее оно должно создаться в образе
		bool bDirExist = false;
l_repeatDir:
		dir_res = res = m_BKImage.AddObject(strName, bDirExist); // добавим директорию

		if (res.bFatal)
		{
			// если фатальная - то просто выведем сообщение.
			goto l_fatal; // всё равно тут всё на goto построено. одним больше, одним меньше
		}
		else if (res.nError != ADD_ERROR::OK_NOERROR) // если какая-то ошибка
		{
			// два варианта - директории не поддерживаются и пытались создать уже существующую директорию
			switch (res.nImageErrorNumber)
			{
				case IMAGE_ERROR::FS_DIR_EXIST:
					bDirExist = true; // устанавливаем флаг игнорирования ошибки создания существующей директории
					goto l_repeatDir; // и повторим. Теоретически тут не должно быть зацикливания, т.к. эта ошибка уже никак не может возникнуть

				case IMAGE_ERROR::FS_NOT_SUPPORT_DIRS:
					res.nError = ADD_ERROR::OK_NOERROR; // игнорируем эту ошибку
					res.nImageErrorNumber = IMAGE_ERROR::OK_NOERRORS;
					break;

				default:
					OnSendErrorNum(WPARAM(MB_ICONERROR), static_cast<LPARAM>(res.nImageErrorNumber));
					ASSERT(false); // что-то не так.
					return res;
			}
		}

		// затем заходим в директорию, и передаём всё содержимое.
		std::error_code ec;
		fs::directory_iterator dit(strName, fs::directory_options::skip_permission_denied | fs::directory_options::follow_directory_symlink, ec);

		for (auto const &it : dit)
		{
			res = SendObject(it.path()); // имя найденного файла

			// здесь все ошибки уже обработаны, надо отлавливать фатальную ситуацию,
			// чтобы прервать цикл.
			if (res.bFatal)
			{
				break;
			}
		}

		m_BKImage.OutFromDirObject(&dir_res.afr); // и выйдем из директории в образе
	}
	else
	{
l_repeatFile:
		// если не директория
		res = m_BKImage.AddObject(strName);

		// обработаем ошибки
		if (res.bFatal)
		{
l_fatal:

			// если фатальная - то просто выведем сообщение.
			if (res.nError == ADD_ERROR::IMAGE_ERROR)
			{
				OnSendErrorNum(WPARAM(MB_ICONERROR), static_cast<LPARAM>(res.nImageErrorNumber));
			}
			else
			{
				OnSendErrorNum(WPARAM(MB_ICONERROR), static_cast<LPARAM>(0x10000 | static_cast<int>(res.nError)));
			}
		}
		else if (res.nError != ADD_ERROR::OK_NOERROR) // если какая-то ошибка
		{
			// если не фатальная, то
			if (res.nImageErrorNumber == IMAGE_ERROR::FS_FILE_EXIST)
			{
				CString str;
				CString strf = CString(MAKEINTRESOURCE(IDS_INFO_FILE_EXIST));
				str.Format(strf, res.afr.strName.c_str());
				int definite = ShowMessageBox(str, MB_YESNOCANCEL | MB_ICONINFORMATION);

				switch (definite)
				{
					case IDYES:
					{
						// удаляем существующий файл и повторяем попытку
						ADDOP_RESULT res2 = m_BKImage.DeleteObject(&res.afr, false); // удаляем пока без форсирования

						// если файл удалить не получается
						if (res2.bFatal)
						{
							goto l_fatal;
						}
						else if (res2.nError != ADD_ERROR::OK_NOERROR)
						{
							if (res2.nImageErrorNumber == IMAGE_ERROR::FS_FILE_PROTECTED) // если удалить не получается из-за атрибутов
							{
								CString str;
								CString strf = CString(MAKEINTRESOURCE(IDS_INFO_FILE_PROTECT));
								str.Format(strf, res.afr.strName.c_str());
								int definite2 = ShowMessageBox(str, MB_YESNO | MB_ICONINFORMATION);

								switch (definite2)
								{
									case IDYES:
									{
										ADDOP_RESULT res3 = m_BKImage.DeleteObject(&res.afr, true);  // удаляем с форсированием

										if (res3.nError == ADD_ERROR::OK_NOERROR) // если удалилось
										{
											goto l_repeatFile; // то идём снова добавлять файл
										}
										else
										{
											goto l_fatal;
										}
									}

									case IDNO:
										goto l_cancelFile;
								}
							}
							else
							{
								goto l_fatal;
							}
						}
						else
						{
							goto l_repeatFile; // то идём снова добавлять файл
						}

						break;
					}

					case IDNO:
						// TODO: надо предложить переименовать файл. Нарисовать диалог и функционал редактирования Edit Control
						break;

					case IDCANCEL:
l_cancelFile:
						res.bFatal = true;
						res.nError = ADD_ERROR::USER_CANCEL;
						break;
				}
			}
			else
			{
				OnSendErrorNum(WPARAM(MB_ICONERROR), static_cast<LPARAM>(res.nImageErrorNumber));
				ASSERT(false); // что-то не так.
			}
		}
	}

	return res;
}

void CBKDEDlg::OnDropFiles(HDROP hDropInfo)
{
	const UINT nFileCount = DragQueryFile(hDropInfo, 0xFFFFFFFF, nullptr, 0);

	if (nFileCount >= 1) // дропнули какие-то файлы
	{
		std::wstring str(2560, L'\0');
		DragQueryFile(hDropInfo, 0, str.data(), 2560); // получим первый дропнутый файл.
		// тут если сделать str.shrink_to_fit(), то со строкой происходит что-то странное
		// и с путём тоже - directory_iterator перестаёт с ним работать
		// в общем тут fs::path надо формировать не из строки, а из массива wchar_t
		m_strImgName = fs::path(str.c_str()); // вот он наш файл
		str.clear();
		OpenImage();
	}

	DragFinish(hDropInfo);
}

// выбор образа из списка выведенных образов текущей директории
// вход - lParam указатель на имя файла
//      wParam - номер итема, на котором стоял курсор
LRESULT CBKDEDlg::OnSendImageProcessing(WPARAM wParam, LPARAM lParam)
{
	// получим гарантированно валидное имя
	m_nStartModeItemPos = static_cast<int>(wParam);
	const auto nType = static_cast<BKDirDataItem::RECORD_TYPE>((m_ListControl.GetItemData(m_nStartModeItemPos) >> 16) & 0xffff);

	if (nType == BKDirDataItem::RECORD_TYPE::FILE)
	{
		m_strImgName = m_strImageDir / fs::path((*(reinterpret_cast<CString *>(lParam))).GetString());
		OpenImage();
		return S_OK;
	}

	if (nType == BKDirDataItem::RECORD_TYPE::UP || nType == BKDirDataItem::RECORD_TYPE::DIR)
	{
		if (nType == BKDirDataItem::RECORD_TYPE::UP)
		{
			// выходим из директории
			std::error_code ec;
			fs::current_path(L"..", ec);

			if (!ec)
			{
				m_strImageDir = fs::current_path();
			}

			if (!m_vecSMIPos.empty())
			{
				m_nStartModeItemPos = m_vecSMIPos.back();
				m_vecSMIPos.pop_back();
			}
			else
			{
				m_nStartModeItemPos = 0;
			}
		}
		else
		{
			m_vecSMIPos.push_back(m_nStartModeItemPos);
			m_nStartModeItemPos = 0;
			// заходим внутрь директории
			m_strImageDir /= fs::path((*(reinterpret_cast<CString *>(lParam))).GetString());
		}

		m_ListControl.DeleteAllItems();
		ChangeCurrDir();
	}

	return S_OK;
}

void CBKDEDlg::ChangeCurrDir()
{
	if (ScanCurrDir())
	{
		if (m_nStartModeItemPos < 0)
		{
			m_nStartModeItemPos = 0;
		}

		m_ListControl.SetFocus();
		m_ListControl.SetItemState(m_nStartModeItemPos, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		m_ListControl.SetHotItem(m_nStartModeItemPos);

		// и убедимся, чтобы выделенный элемент всегда был на виду
		if (!m_ListControl.EnsureVisible(m_nStartModeItemPos, FALSE))
		{
			CRect r;
			m_ListControl.GetItemRect(m_nStartModeItemPos, &r, LVIR_BOUNDS);
			m_ListControl.Scroll(CSize(0, r.Height()));
		}
	}
	else // если ничего не вывелось
	{
		m_ListControl.Init(CBKListCtrl::MODE_CTRL::MAIN); // переключимся в основной режим.
	}
}


LRESULT CBKDEDlg::OnGetCmdLine(WPARAM wParam, LPARAM lParam)
{
	m_strImgName = fs::path(theApp.m_lpCmdLine); // вот он наш файл. но вообще-то там может быть всё что угодно.
	OpenImage();
	return S_OK;
}

LRESULT CBKDEDlg::OnSendItemProcessing(WPARAM wParam, LPARAM lParam)
{
	m_BKImage.ItemProcessing(static_cast<int>(wParam), reinterpret_cast<BKDirDataItem *>(lParam));
	return S_OK;
}

LRESULT CBKDEDlg::OnOutImage(WPARAM wParam, LPARAM lParam)
{
	if (m_BKImage.PopCurrentImg())
	{
		// вышли из логического диска
		// теперь надо получить базовое смещение и размер для текущего диска.
		// (предполагаем что возможна неограниченная вложенность лог. дисков друг в друга)
		const unsigned long nBaseOffset = m_BKImage.GetBaseOffset();
		m_sParseResult = m_ParserImage.ParseImage(m_strImgName, nBaseOffset);
		m_sParseResult.nImageSize = m_BKImage.GetImgSize();
		uint32_t flg = m_BKImage.ReOpen();
		SetEnableButtons(flg);
		FillStatusBar();
		return S_OK;
	}

	// в стеке пусто, значит совсем вышли из образа
	m_BKImage.Close();
	DisableControls();
	ClearStatusBar();
	m_strImgName.clear();
	m_ListControl.Init(CBKListCtrl::MODE_CTRL::START);
	ChangeCurrDir();
	return LRESULT(1L);
}

/*
операция захода в логический диск.
Входные параметры:
wParam - базовое смещение логического диска в образе
lParam - размер образа
*/
LRESULT CBKDEDlg::OnPutIntoLD(WPARAM wParam, LPARAM lParam)
{
	auto ret = LRESULT(1L);
	const auto nBaseOffset = static_cast<unsigned long>(wParam);
	m_sParseResult = m_ParserImage.ParseImage(m_strImgName, nBaseOffset);
	/*
	для лог. дисков при выходе отсюда будет неправильный размер лог. диска.
	размер надо брать оттуда же, откуда брать базовое смещение для лог диска.
	ещё 1 проблема:
	1) для лог.диска корневой директорией должно быть имя лог.диска, и его надо где-то
	как-то хранить, куда-то передавать.
	*/
	m_sParseResult.nImageSize = static_cast<unsigned long>(lParam);

	if (m_sParseResult.imageOSType == IMAGE_TYPE::ERROR_NOIMAGE)
	{
		ShowMessageBox(IDS_MSG_IMGERROR);
	}
	else if (m_sParseResult.imageOSType == IMAGE_TYPE::UNKNOWN)
	{
		ShowMessageBox(IDS_MSG_IMGUNKNOWN);
	}
	else
	{
		// теперь, если образ опознался, надо создать объект, соответствующий ФС и вывести содержимое
		if (uint32_t flg = m_BKImage.Open(m_sParseResult, true))
		{
			SetEnableButtons(flg);
			m_ListControl.DeleteAllItems();
			CBKImage::ItemPanePos pp(0, 0);
			m_BKImage.ReadCurrentDir(pp);
			ret = S_OK;
		}
		else
		{
			ShowMessageBox(IDS_ERR_NOTENOUGHT_MEMORY);
		}

		FillStatusBar();
	}

	return ret;
}


/*
Вход:
wParam - индекс иконки
lParam -
старшее слово - тип ошибки:
0 - IMAGE_ERROR_NUMS
1 - ADDOP_ERRORS
младшее слово - код ошибки.
*/
LRESULT CBKDEDlg::OnSendErrorNum(WPARAM wParam, LPARAM lParam)
{
	auto nIcon = static_cast<UINT>(wParam);

	if (nIcon == 0)
	{
		nIcon = MB_ICONEXCLAMATION;
	}

	int nErrType = HIWORD(lParam);

	switch (nErrType)
	{
		case 0:
		{
			const auto nErr = static_cast<IMAGE_ERROR>(LOWORD(lParam));

			if (nErr < IMAGE_ERROR::NUMBERS)
			{
				CString str = g_ImageErrorStr[static_cast<int>(nErr)].c_str();
				ShowMessageBox(str, MB_OK | nIcon);
			}
			else
			{
				CString str = _T("Ошибочный параметр в IMAGE_ERROR_NUMBER");
				ShowMessageBox(str, MB_OK | nIcon);
			}

			break;
		}

		case 1:
		{
			const auto nErr = static_cast<ADD_ERROR>(LOWORD(lParam));

			if (nErr < ADD_ERROR::NUMBERS)
			{
				CString str = g_AddOpErrorStr[static_cast<int>(nErr)].c_str();
				ShowMessageBox(str, MB_OK | MB_ICONERROR);
			}
			else
			{
				CString str = _T("Ошибочный параметр в ADDOP_ERRORS");
				ShowMessageBox(str, MB_OK | nIcon);
			}

			break;
		}

		default:
			ASSERT(false);
			break;
	}

	return S_OK;
}

/*
Входные параметры:
wParam - индекс иконки
lParam - указатель на текст типа TCHAR*
Выход: результат мессагебокса, обычно - код нажатой кнопки
*/
LRESULT CBKDEDlg::OnSendMessageBox(WPARAM wParam, LPARAM lParam)
{
	auto nIcon = static_cast<UINT>(wParam);

	if (nIcon == 0)
	{
		nIcon = MB_OK | MB_ICONEXCLAMATION;
	}

	CString str = (reinterpret_cast<TCHAR *>(lParam));
	return LRESULT(ShowMessageBox(str, nIcon));
}

LRESULT CBKDEDlg::OnOutCurrentFilePath(WPARAM wParam, LPARAM lParam)
{
	const auto pstr = reinterpret_cast<std::wstring *>(lParam);
	SetDlgItemText(IDC_STATIC_PATH, pstr->c_str());
	return S_OK;
}

LRESULT CBKDEDlg::OnOutCurrentImageInfo(WPARAM wParam, LPARAM lParam)
{
	const auto pstr = reinterpret_cast<std::wstring *>(lParam);
	SetDlgItemText(IDC_STATIC_INFO, pstr->c_str());
	return LRESULT();
}

LRESULT CBKDEDlg::OnMakeDrop(WPARAM wParam, LPARAM lParam)
{
	if (m_BKImage.IsImageOpen())
	{
		const auto hDropInfo = reinterpret_cast<HDROP>(lParam);
		const UINT nFileCount = DragQueryFile(hDropInfo, -1, nullptr, 0);
		UINT n = 0;
		ADDOP_RESULT res;
		CBKImage::ItemPanePos pp = m_BKImage.GetTopItemIndex();

		while (n < nFileCount) // дропнули какие-то файлы
		{
			std::wstring str(2560, L'\0');
			DragQueryFile(hDropInfo, n, str.data(), 2560); // получим очередной дропнутый файл.
			// тут если сделать str.shrink_to_fit(), то со строкой происходит что-то странное
			// и с путём тоже - directory_iterator перестаёт с ним работать
			// в общем тут fs::path надо формировать не из строки, а из массива wchar_t
			fs::path p(str.c_str());
			str.clear();

			if (fs::exists(p))
			{
				res = SendObject(p);
				// здесь все ошибки уже обработаны, надо отлавливать фатальную ситуацию,
				// чтобы прервать цикл.
			}

			if (res.bFatal)
			{
				break;
			}

			n++;
		}

		m_ListControl.DeleteAllItems();
		m_BKImage.ReadCurrentDir(pp);
		m_ListControl.SetFocus();
		DragFinish(hDropInfo);
	}
	else
	{
		OnSendErrorNum(WPARAM(MB_ICONERROR), static_cast<LPARAM>(0x10000 | static_cast<int>(ADD_ERROR::IMAGE_NOT_OPEN)));
	}

	return S_OK;
}


/*
операция выбора/открытия нового файла образа.
*/
void CBKDEDlg::OpenImage()
{
	// именем файла и добавляется к пути по умолчанию
//  AfxMessageBox(m_strImgPathName);
	m_sParseResult = m_ParserImage.ParseImage(m_strImgName, 0);

	if (m_sParseResult.imageOSType == IMAGE_TYPE::ERROR_NOIMAGE)
	{
		ShowMessageBox(IDS_MSG_IMGERROR);
	}
	else if (m_sParseResult.imageOSType == IMAGE_TYPE::UNKNOWN)
	{
		ShowMessageBox(IDS_MSG_IMGUNKNOWN);
	}
	else
	{
		m_ListControl.Init(CBKListCtrl::MODE_CTRL::MAIN);
		// теперь, если образ опознался, надо создать объект, соответствующий ФС и вывести содержимое
		m_BKImage.ClearImgVector();

		if (uint32_t flg = m_BKImage.Open(m_sParseResult))
		{
			SetEnableButtons(flg);
			CBKImage::ItemPanePos pp(0, 0);
			m_BKImage.ReadCurrentDir(pp);
		}
		else
		{
			ShowMessageBox(IDS_ERR_NOTENOUGHT_MEMORY);
		}

		FillStatusBar();
	}
}

int CALLBACK CBKDEDlg::BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	TCHAR szPath[_MAX_PATH];

	switch (uMsg)
	{
		case BFFM_INITIALIZED:
			if (lpData)
			{
				::SendMessage(hWnd, BFFM_SETSELECTION, TRUE, lpData);
			}

			break;

		case BFFM_SELCHANGED:
			SHGetPathFromIDList(LPITEMIDLIST(lParam), szPath);
			::SendMessage(hWnd, BFFM_SETSTATUSTEXT, WPARAM(0), reinterpret_cast<LPARAM>(szPath));
			break;
	}

	return 0;
}

// диалог выбора директории для сохранения извлекаемых файлов
bool CBKDEDlg::GetFolder(LPTSTR szPath)
{
	szPath[0] = 0;
	bool result = false;
	LPMALLOC pMalloc;

	if (::SHGetMalloc(&pMalloc) == NOERROR)
	{
		BROWSEINFO bi;
		::ZeroMemory(&bi, sizeof bi);
		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_SHAREABLE | BIF_USENEWUI | BIF_UAHINT | BIF_NEWDIALOGSTYLE | BIF_BROWSEINCLUDEURLS; // показывать файлы -  | BIF_BROWSEINCLUDEFILES | BIF_BROWSEFILEJUNCTIONS;
		// дескриптор окна-владельца диалога
		bi.hwndOwner = GetSafeHwnd();
		// добавление заголовка к диалогу
		CString strStr = MAKEINTRESOURCE(IDS_SELFLDRTTL);
		bi.lpszTitle = strStr.GetString();
		// отображение текущего каталога
		bi.lpfn      = BrowseCallbackProc;
		bi.ulFlags  |= BIF_STATUSTEXT;

		// установка каталога по умолчанию
		if (!m_strStorePath.empty())
		{
			// установка каталога по умолчанию
			bi.lParam = reinterpret_cast<LPARAM>(m_strStorePath.c_str());
		}

		// установка корневого каталога
		/* не нужно. и так как надо хорошо показывает. но вдруг пригодится где-нибудь.
		IShellFolder *pDF;
		if (SHGetDesktopFolder(&pDF) == NOERROR)
		{
		    LPITEMIDLIST pIdl = nullptr;
		    ULONG        chEaten;
		    ULONG        dwAttributes;

		    USES_CONVERSION;
		    LPOLESTR oleStr = T2OLE(_T("C:"));

		    pDF->ParseDisplayName(nullptr ,nullptr, oleStr, &chEaten, &pIdl, &dwAttributes);
		    pDF->Release();

		    bi.pidlRoot = pIdl;
		} */
		LPITEMIDLIST pidl = ::SHBrowseForFolder(&bi);

		if (pidl != nullptr)
		{
			if (::SHGetPathFromIDList(pidl, szPath))
			{
				result = true;
			}

			pMalloc->Free(pidl);
		}

		/*
		if (bi.pidlRoot != nullptr)
		{
		    pMalloc->Free((void*)bi.pidlRoot);
		}*/
		pMalloc->Release();
	}

	return result;
}

bool CBKDEDlg::ScanCurrDir()
{
	// если начальная директория, где лежат образы пуста
	if (m_strImageDir.empty())
	{
		return false; // то ничего не делать
	}

	// если задана неверная директория
	if (!fs::exists(m_strImageDir))
	{
		return false; // то ничего не делать
	}

	// зайдём в нужный каталог
	std::error_code ec;
	fs::current_path(m_strImageDir, ec);

	if (ec)
	{
		return false; // если не смогли, то просто выйдем
	}

	// отобразим путь к директории, которую будем отображать
	SetDlgItemText(IDC_STATIC_PATH, m_strImageDir.c_str());
	// формируем первый элемент ".."
	int item = 0;
	m_ListControl.InsertItem(item, L"..");
	m_ListControl.SetItemText(item, CBKListCtrl::LC_SIZE_ST, g_strUp.c_str());
	m_ListControl.SetItemData(item, static_cast<DWORD_PTR>((static_cast<int>(BKDirDataItem::RECORD_TYPE::UP) << 16) | item));
	item++;
	// за ним формируем всё, что найдём в директории
	fs::directory_iterator dit(m_strImageDir, fs::directory_options::skip_permission_denied | fs::directory_options::follow_directory_symlink, ec);

	for (auto const &it : dit)
	{
		fs::path file = it.path(); // имя найденного файла

		if (fs::is_directory(file))
		{
			m_ListControl.InsertItem(item, file.filename().c_str());
			m_ListControl.SetItemText(item, CBKListCtrl::LC_SIZE_ST, g_strDir.c_str());
			m_ListControl.SetItemData(item, static_cast<DWORD_PTR>((static_cast<int>(BKDirDataItem::RECORD_TYPE::DIR) << 16) | item));
			item++;
		}
		else if (fs::is_regular_file(file))
		{
			// если файл - достанем расширение
			std::wstring strExt = strUtil::strToLower(file.extension().wstring());

			for (const auto &pstrExt : g_pstrExts)  // теперь по списку известных расширений
			{
				if (strExt == pstrExt) // если совпало
				{
					// нужно определить что это.
					const PARSE_RESULT pr = m_ParserImage.ParseImage(file, 0);
					CString str;
					m_ListControl.InsertItem(item, file.filename().c_str());
					str.Format(_T("%d"), pr.nImageSize);
					m_ListControl.SetItemText(item, CBKListCtrl::LC_SIZE_ST, str.GetString());
					str = CString(m_BKImage.GetImgFormatName(pr.imageOSType).c_str());
					m_ListControl.SetItemText(item, CBKListCtrl::LC_OSTYPE_ST, str);
					m_ListControl.SetItemText(item, CBKListCtrl::LC_SYSTYPE_ST, (pr.bImageBootable ? _T("Да") : _T("Нет")));
					m_ListControl.SetItemData(item, static_cast<DWORD_PTR>((static_cast<int>(BKDirDataItem::RECORD_TYPE::FILE) << 16) | item));
					item++;
					break;
				}
			}
		}
	}

	m_ListControl.SortItems(CBKListCtrl::MyCompareProc2, reinterpret_cast<DWORD_PTR>(&m_ListControl));
	return true;
}


BOOL CBKDEDlg::OnHelpInfo(HELPINFO *pHelpInfo)
{
	// чтоб не вылазила справка, сделаем так
	return FALSE; // CDialogEx::OnHelpInfo(pHelpInfo);
}




////////////////////////////////////////////////////////// /
// крайне экспериментальная часть,
// которую придётся удалить, если ничего не получится.

// тут надо немного улучшить код и универсальности добавить


void CBKDEDlg::FillMenuButton()
{
	static TCHAR *pMenu[2] =
	{
		(TCHAR *)L"A:\0",
		(TCHAR *)L"B:\0",
	};
	// Load application list into menu button
	m_ApplicationMenu = CreatePopupMenu();
	MENUITEMINFO MenuInfo;
	memset(&MenuInfo, 0, sizeof(MENUITEMINFO));
	MenuInfo.cbSize = sizeof(MENUITEMINFO);
	MenuInfo.fMask = MIIM_ID | MIIM_STRING | MIIM_DATA;
	MenuInfo.wID = 1;

	for (auto &i : pMenu)
	{
		MenuInfo.dwTypeData = i;
		MenuInfo.cch = static_cast<UINT>(wcslen(MenuInfo.dwTypeData));
		InsertMenuItem(m_ApplicationMenu, MenuInfo.wID, TRUE, &MenuInfo);
		MenuInfo.wID++;
	}

	// Attach menu to CMFCMenuButton
	auto m_ApplicationList = reinterpret_cast<CMFCMenuButton *>(GetDlgItem(IDC_MFCMENUBUTTON_OPEN));
	m_ApplicationList->m_bOSMenu = FALSE;
	m_ApplicationList->m_bRightArrow = FALSE;
	m_ApplicationList->m_bStayPressed = TRUE;
	m_ApplicationList->m_bDefaultClick = TRUE;
	m_ApplicationList->m_hMenu = m_ApplicationMenu;
}

void CBKDEDlg::OnBnClickedMfcmenubuttonOpen()
{
	auto m_ApplicationList = reinterpret_cast<CMFCMenuButton *>(GetDlgItem(IDC_MFCMENUBUTTON_OPEN));
	int nDrive = m_ApplicationList->m_nMenuResult;

	if (nDrive == 0)
	{
		return OnBnClickedButtonOpen();
	}

	nDrive--; // номер привода.
	wchar_t szDevice[32];
	DWORD dwVersion = 0;
	DWORD dwRet;
	HANDLE h = CreateFile(L"\\\\.\\fdrawcmd", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

	if (h != INVALID_HANDLE_VALUE)
	{
		DeviceIoControl(h, IOCTL_FDRAWCMD_GET_VERSION, nullptr, 0, &dwVersion, sizeof(dwVersion), &dwRet, nullptr);
		CloseHandle(h);
	}

	bool bFDRaw = true;

	if (!dwVersion)
	{
		// ("fdrawcmd.sys не установлен, смотрите: http://simonowen.com/fdrawcmd/\n");
		bFDRaw = false;
	}
	else if (HIWORD(dwVersion) != HIWORD(FDRAWCMD_VERSION))
	{
		// ("Установленный fdrawcmd.sys не совместим с этой программой.\n");
		bFDRaw = false;
	}

	if (bFDRaw)
	{
		wsprintf(szDevice, L"\\\\.\\fdraw%u", nDrive);
		h = CreateFile(szDevice, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

		if (h == INVALID_HANDLE_VALUE)
		{
			// error
			bFDRaw = false;
		}
		else
		{
			int DISK_DATARATE = FD_RATE_250K;          // 2 is 250 kbit/sec
			DeviceIoControl(h, IOCTL_FD_SET_DATA_RATE, &DISK_DATARATE, sizeof(DISK_DATARATE), nullptr, 0, &dwRet, nullptr);
			CloseHandle(h);
		}
	}

	// вот такая штука, чтобы без goto обойтись.
	if (!bFDRaw)
	{
		wchar_t szDrive[4] = L"A:\0";
		szDrive[0] += nDrive;
		wsprintf(szDevice, L"\\\\.\\%s", szDrive);
	}

	// !!!оказывается \\\\.\\fdraw через  fopen  не открывается!!! только через HANDLE
	m_strImgName = fs::path(szDevice);
	OpenImage();
	m_ListControl.SetFocus();
}


void CBKDEDlg::OnContextChangeAddr()
{
	const size_t uSelectedCount = m_ListControl.GetSelectedCount();
	int  nItem = -1;

	if (uSelectedCount > 0)
	{
		// если что-то выделенное есть
		for (size_t i = 0; i < uSelectedCount; ++i) // обработаем все выделенные элементы
		{
			nItem = m_ListControl.GetNextItem(nItem, LVNI_SELECTED);
			const auto fr = reinterpret_cast<BKDirDataItem *>(m_ListControl.GetItemData(nItem));

			if (fr->nAttr & (FR_ATTR::DIR | FR_ATTR::LINK | FR_ATTR::LOGDISK))
			{
				// для этих нельзя поменять адрес
			}
			else
			{
				CChangeAddrDlg dlg(fr->nAddress);

				if (dlg.DoModal() == IDOK)
				{
					const int newAddr = dlg.GetAddr();

					if (newAddr != fr->nAddress)
					{
						fr->nAddress = newAddr;
						m_BKImage.RenameRecord(fr);
					}
				}
			}
		}
	}

	m_ListControl.SetFocus();
}


void CBKDEDlg::OnContextMenuRename()
{
	OnBnClickedButtonRename();
}


void CBKDEDlg::OnContextDelete()
{
	OnBnClickedButtonDelete();
}


void CBKDEDlg::OnContextExtract()
{
	OnBnClickedButtonExtract();
}


void CBKDEDlg::OnContextViewastext()
{
	OnBnClickedButtonView();
}


void CBKDEDlg::OnContextViewassprite()
{
	OnBnClickedButtonViewSprite();
}

