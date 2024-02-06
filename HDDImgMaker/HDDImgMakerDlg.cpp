
// HDDImgMakerDlg.cpp : файл реализации
//

#include "pch.h"
#include "HDDImgMaker.h"
#include "HDDImgMakerDlg.h"
#include "afxdialogex.h"
#include <thread>

#pragma warning(disable:4996)

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

constexpr auto HDI_EXT = 0;
constexpr auto HDIX_EXT = 1;
constexpr auto IMG_EXT = 2;

// расширения разных типов виртуальных HDD
const fs::path CHDDImgMakerDlg::strVHDDext[3] =
{
	_T(".hdi"),
	_T(".hdix"),
	_T(".img")
};

constexpr auto HDIX_ID_ROW_LEN = 16; // количество байтов в строке


int CHDDImgMakerDlg::ShowMessageBox(UINT msgID, UINT nType /*= 0*/, UINT nIDhelp /*= 0*/)
{
	CString strMsg;
	VERIFY(strMsg.LoadString(msgID));
	return AfxMessageBox(strMsg.GetString(), nType, nIDhelp);
}

int CHDDImgMakerDlg::ShowMessageBox(UINT msgID, CString str, UINT nType, UINT nIDhelp)
{
	CString strMsg;
	VERIFY(strMsg.LoadString(msgID));
	return AfxMessageBox((strMsg + str).GetString(), nType, nIDhelp);
}

int CHDDImgMakerDlg::ShowMessageBoxErr(UINT msgID, UINT nType /*= 0*/, UINT nIDhelp /*= 0*/, bool reg /* = false*/)
{
	BAR_Error(reg);
	return ShowMessageBox(msgID, nType, nIDhelp);
}

int CHDDImgMakerDlg::ShowMessageBoxErr(UINT msgID, CString str, UINT nType /*= 0*/, UINT nIDhelp /*= 0*/, bool reg /* = false*/)
{
	BAR_Error(reg);
	return ShowMessageBox(msgID, str, nType, nIDhelp);
}

// Диалоговое окно CAboutDlg используется для описания сведений о приложении

class CAboutDlg : public CDialogEx
{
	public:
		CAboutDlg();

// Данные диалогового окна
		enum { IDD = IDD_ABOUTBOX };

	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV
		virtual BOOL OnInitDialog() override;

// Реализация
	protected:
		DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg()
	: CDialogEx(CAboutDlg::IDD)
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
	CDialogEx::OnInitDialog();
	DWORD dwHandle;
	CString name = CString(theApp.m_pszAppName) + _T(".exe");
	const DWORD dwSize = ::GetFileVersionInfoSize(name.GetString(), &dwHandle);

	if (dwSize && dwHandle)
	{
		CString str = _T("BK Virtual HDD Maker ");
		CString s;
		auto lpData = std::vector<TCHAR>(dwSize);

		if (lpData.data())
		{
			::GetFileVersionInfo(name.GetString(), dwHandle, dwSize, lpData.data());
			UINT uBufLen;
			VS_FIXEDFILEINFO *lpfi = nullptr;
			::VerQueryValue(lpData.data(), _T("\\"), (void **)&lpfi, &uBufLen);
			s.Format(_T("v%d.%d.%d.%d "),
			         (lpfi->dwFileVersionMS >> 16), lpfi->dwFileVersionMS & 0xFFFF,
			         (lpfi->dwFileVersionLS >> 16), lpfi->dwFileVersionLS & 0xFFFF);
#ifdef _WIN64
			s += _T("x64");
#else
			s += _T("x86");
#endif
		}
		else
		{
			s = _T("v???");
		}

		GetDlgItem(IDC_STATIC2)->SetWindowText(str + s);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// Исключение: страница свойств OCX должна возвращать значение FALSE
}

// диалоговое окно CHDDImgMakerDlg



CHDDImgMakerDlg::CHDDImgMakerDlg(CWnd *pParent /*=nullptr*/)
	: CDialogEx(CHDDImgMakerDlg::IDD, pParent)
	, m_ButtonMenu(nullptr)
	, m_strImgName(_T(""))
	, m_strExistingImgName(_T(""))
	, m_strModelName(_T("BKEMU HARD DRIVE IMAGE"))
	, m_strSerialNumber(_T(""))
	, m_nCylinders(1)
	, m_nHeads(1)
	, m_nSectors(1)
	, m_bCancel(false)
	, m_bConvert(false)
	, m_file(nullptr)
	, m_fileExisting(nullptr)
	, m_bBreakThr(false)
	, m_bChangeGeometry(true)
	, m_bReduceSize(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CHDDImgMakerDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_IMGNAME, m_strImgName);
	DDX_Text(pDX, IDC_EDIT_EXISTING_IMGNAME, m_strExistingImgName);
	DDX_Text(pDX, IDC_EDIT_MODELNAME, m_strModelName);
	DDV_MaxChars(pDX, m_strModelName, 40);
	DDX_Text(pDX, IDC_EDIT_SERNUMBER, m_strSerialNumber);
	DDV_MaxChars(pDX, m_strSerialNumber, 20);
	DDX_Text(pDX, IDC_EDIT_CYLINDERS, m_nCylinders);
	DDV_MinMaxInt(pDX, m_nCylinders, 1, 65536);
	DDX_Text(pDX, IDC_EDIT_HEADS, m_nHeads);
	DDV_MinMaxInt(pDX, m_nHeads, 1, 16);
	DDX_Text(pDX, IDC_EDIT_SECTORS, m_nSectors);
	DDV_MinMaxInt(pDX, m_nSectors, 1, 256);
	DDX_Control(pDX, IDC_SPIN_CYLINDERS, m_spinCylinders);
	DDX_Control(pDX, IDC_SPIN_HEADS, m_spinHeads);
	DDX_Control(pDX, IDC_SPIN_SECTORS, m_spinSectors);
	DDX_Control(pDX, IDC_PROGRESS_CREATE, m_progress);
	DDX_Control(pDX, IDC_COMBO_HDITYPE, m_TypeCombo);
	DDX_Check(pDX, IDC_CHECK_REDUCESIZE, m_bReduceSize);
}

BEGIN_MESSAGE_MAP(CHDDImgMakerDlg, CDialogEx)
	ON_MESSAGE(WM_END_PROCESS, &CHDDImgMakerDlg::OnEndProcess)
	ON_MESSAGE(WM_ONSTART_PROGRESS, &CHDDImgMakerDlg::OnStartProgress)
	ON_MESSAGE(WM_ONSTOP_PROGRESS, &CHDDImgMakerDlg::OnStopProgress)
	ON_MESSAGE(WM_ONSEND_PROGRESS, &CHDDImgMakerDlg::OnSendProgress)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_EN_CHANGE(IDC_EDIT_CYLINDERS, &CHDDImgMakerDlg::OnEnChangeEditCylinders)
	ON_EN_CHANGE(IDC_EDIT_HEADS, &CHDDImgMakerDlg::OnEnChangeEditHeads)
	ON_EN_CHANGE(IDC_EDIT_SECTORS, &CHDDImgMakerDlg::OnEnChangeEditSectors)
	ON_BN_CLICKED(IDC_BUTTON_SERNUMBER, &CHDDImgMakerDlg::OnBnClickedButtonSernumber)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CHDDImgMakerDlg::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_EXISTING, &CHDDImgMakerDlg::OnBnClickedButtonBrowseExisting)
	ON_BN_CLICKED(IDC_BUTTON_MAKE, &CHDDImgMakerDlg::OnBnClickedButtonMake)
	ON_EN_KILLFOCUS(IDC_EDIT_IMGNAME, &CHDDImgMakerDlg::OnEnKillfocusEditImgname)
	ON_EN_KILLFOCUS(IDC_EDIT_EXISTING_IMGNAME, &CHDDImgMakerDlg::OnEnKillfocusEditExistingImgname)
	ON_EN_KILLFOCUS(IDC_EDIT_SECTORS, &CHDDImgMakerDlg::OnEnKillfocusEditSectors)
	ON_EN_SETFOCUS(IDC_EDIT_SECTORS, &CHDDImgMakerDlg::OnEnSetfocusEditSectors)
	ON_EN_KILLFOCUS(IDC_EDIT_HEADS, &CHDDImgMakerDlg::OnEnKillfocusEditHeads)
	ON_EN_SETFOCUS(IDC_EDIT_HEADS, &CHDDImgMakerDlg::OnEnSetfocusEditHeads)
	ON_EN_KILLFOCUS(IDC_EDIT_CYLINDERS, &CHDDImgMakerDlg::OnEnKillfocusEditCylinders)
	ON_EN_SETFOCUS(IDC_EDIT_CYLINDERS, &CHDDImgMakerDlg::OnEnSetfocusEditCylinders)
	ON_CBN_SELCHANGE(IDC_COMBO_HDITYPE, &CHDDImgMakerDlg::OnCbnSelchangeComboHditype)
	ON_BN_CLICKED(IDC_MFCMENUBUTTON1, &CHDDImgMakerDlg::OnBnClickedMfcmenubutton1)
	ON_BN_CLICKED(IDC_CHECK_REDUCESIZE, &CHDDImgMakerDlg::OnBnClickedCheckReducesize)
END_MESSAGE_MAP()




BOOL CHDDImgMakerDlg::PreTranslateMessage(MSG *pMsg)
{
	if (WM_MOUSEFIRST <= pMsg->message && pMsg->message <= WM_MOUSELAST)
	{
		m_tt.RelayEvent(pMsg);
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

static const TOOLTIP_STRUCT tt_strc[] =
{
	{ IDC_EDIT_IMGNAME,           IDS_TTSTR_EDIT_IMGNAME           },
	{ IDC_BUTTON_BROWSE,          IDS_TTSTR_BUTTON_BROWSE          },
	{ IDC_EDIT_EXISTING_IMGNAME,  IDS_TTSTR_EDIT_EXISTING_IMGNAME  },
	{ IDC_BUTTON_BROWSE_EXISTING, IDS_TTSTR_BUTTON_BROWSE_EXISTING },
	{ IDC_EDIT_CYLINDERS,         IDS_TTSTR_EDIT_CYLINDERS         },
	{ IDC_SPIN_CYLINDERS,         IDS_TTSTR_SPIN_CYLINDERS         },
	{ IDC_EDIT_HEADS,             IDS_TTSTR_EDIT_HEADS             },
	{ IDC_SPIN_HEADS,             IDS_TTSTR_SPIN_HEADS             },
	{ IDC_EDIT_SECTORS,           IDS_TTSTR_EDIT_SECTORS           },
	{ IDC_SPIN_SECTORS,           IDS_TTSTR_SPIN_SECTORS           },
	{ IDC_EDIT_MODELNAME,         IDS_TTSTR_EDIT_MODELNAME         },
	{ IDC_EDIT_SERNUMBER,         IDS_TTSTR_EDIT_SERNUMBER         },
	{ IDC_BUTTON_SERNUMBER,       IDS_TTSTR_BUTTON_SERNUMBER       },
	{ IDC_MFCMENUBUTTON1,         IDS_TTSTR_MFCMENUBUTTON1         },
	{ IDC_COMBO_HDITYPE,          IDS_TTSTR_COMBO_HDITYPE          },
	{ IDC_BUTTON_MAKE,            IDS_TTSTR_BUTTON_MAKE            },
	{ IDC_CHECK_REDUCESIZE,       IDS_TTSTR_CHECK_REDUCESIZE       },
	{ IDOK,                       IDS_TTSTR_IDOK                   }
};

// обработчики сообщений CHDDImgMakerDlg

// порядок зависит от того, как задаётся меню в OnInitDialog в векторе pMenu
constexpr auto CONVERT_HDI2IMG = 0;
constexpr auto CONVERT_HDIX2HDI = 1;
constexpr auto CONVERT_HDI2HDIX = 2;
// а это дополнительно
constexpr auto WORK_CREATEHDI = 3;
constexpr auto WORK_CONVERTHDI = 4;


BOOL CHDDImgMakerDlg::OnInitDialog()
{
	srand(unsigned(time(nullptr)));
	CDialogEx::OnInitDialog();
	// Добавление пункта "О программе..." в системное меню.
	// IDM_ABOUTBOX должен быть в пределах системной команды.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);
	CMenu *pSysMenu = GetSystemMenu(FALSE);

	if (pSysMenu != nullptr)
	{
		CString strAboutMenu;
		const BOOL bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);

		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Задаёт значок для этого диалогового окна. Среда делает это автоматически,
	//  если главное окно приложения не является диалоговым
	SetIcon(m_hIcon, TRUE);         // Крупный значок
	SetIcon(m_hIcon, FALSE);        // Мелкий значок
	m_strAppPath = fs::current_path();
	// создаём объект тултипов
	m_tt.Create(this);
	m_tt.SetMaxTipWidth(256);

	for (const auto &i : tt_strc)
	{
		m_tt.AddTool(GetDlgItem(i.nID), CString(MAKEINTRESOURCE(i.nStrResID)).GetString());
	}

	m_TypeCombo.InsertString(HDI_EXT, strVHDDext[HDI_EXT].c_str());
	m_TypeCombo.InsertString(HDIX_EXT, strVHDDext[HDIX_EXT].c_str());
	m_TypeCombo.SetCurSel(HDI_EXT);
	// Load application list into menu button
	m_ButtonMenu = CreatePopupMenu();
	static std::vector<CString> pMenu =
	{
		_T("hdix -> hdi\0"),
		_T("hdi -> hdix\0")
	};
	MENUITEMINFO MenuInfo;
	memset(&MenuInfo, 0, sizeof(MENUITEMINFO));
	MenuInfo.cbSize = sizeof(MENUITEMINFO);
	MenuInfo.fMask = MIIM_ID | MIIM_STRING | MIIM_DATA;
	MenuInfo.wID = 1;

	for (auto &str : pMenu)
	{
		MenuInfo.dwTypeData = str.GetBuffer();
		MenuInfo.cch = UINT(str.GetLength());
		InsertMenuItem(m_ButtonMenu, MenuInfo.wID, TRUE, &MenuInfo);
		MenuInfo.wID++;
	}

	// Attach menu to CMFCMenuButton
	auto pMenuButton = dynamic_cast<CMFCMenuButton *>(GetDlgItem(IDC_MFCMENUBUTTON1));
	pMenuButton->m_bOSMenu = FALSE;
	pMenuButton->m_bRightArrow = FALSE;
	pMenuButton->m_bStayPressed = TRUE;
	pMenuButton->m_bDefaultClick = TRUE;
	pMenuButton->m_hMenu = m_ButtonMenu;
	m_strExistingImgPath = m_strAppPath;
	m_bCancel = false;
	m_bConvert = false;
	BAR_Normal(0);
	m_spinCylinders.SetRange32(1, 65536);
	m_spinCylinders.SetPos32(m_nCylinders);
	m_spinCylinders.SetBuddy(GetDlgItem(IDC_EDIT_CYLINDERS));
	m_spinHeads.SetRange32(1, 16);
	m_spinHeads.SetPos32(m_nHeads);
	m_spinHeads.SetBuddy(GetDlgItem(IDC_EDIT_HEADS));
	m_spinSectors.SetRange32(1, 256);
	m_spinSectors.SetPos32(m_nSectors);
	m_spinSectors.SetBuddy(GetDlgItem(IDC_EDIT_SECTORS));
	OutByteSize();
	OnBnClickedButtonSernumber();
	ChangeMakeButtonText();
	return TRUE;  // возврат значения TRUE, если фокус не передан элементу управления
}

void CHDDImgMakerDlg::OnDestroy()
{
	if (m_ButtonMenu)
	{
		DestroyMenu(m_ButtonMenu);
		m_ButtonMenu = nullptr;
	}

	m_bBreakThr = true;
	MSG msg;
	BOOL bRet;

	while (!m_mutThr.try_lock()) // ждём завершения потока
	{
		// пока поток работает, берём на себя трансляцию очереди сообщений
		if (bRet = GetMessage(&msg, GetSafeHwnd(), 0, 0))
		{
			if (bRet != -1)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	m_mutThr.unlock();
	CDialogEx::OnDestroy();
}

void CHDDImgMakerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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
//  чтобы нарисовать значок.  Для приложений MFC, использующих модель документов или представлений,
//  это автоматически выполняется рабочей областью.

void CHDDImgMakerDlg::OnPaint()
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
HCURSOR CHDDImgMakerDlg::OnQueryDragIcon()
{
	return HCURSOR(m_hIcon);
}


void CHDDImgMakerDlg::SetProgress(int v)
{
	m_progress.SetPos(v);
	v = v * 100 / m_barMaxvalue;
	CString s;
	s.Format(_T("%d%%"), v);
	SetDlgItemText(IDC_STATIC_PERCENT, s);
}

void CHDDImgMakerDlg::BAR_Error(bool reg)   // Если вызываем с параметром TRUE, рисуется красивая
// полосятина для ПАУЗЫ.
{
	if (m_progress.GetPos() == 0 || m_progress.GetPos() < m_barMaxvalue / 2)
	{
		SetProgress(m_barMaxvalue);
	}

	m_progress.SetStep(m_barMaxvalue / 5);
	m_progress.SetState(reg ? PBST_PAUSED : PBST_ERROR);
}

void CHDDImgMakerDlg::BAR_Normal(const int pos, const int val)
{
	m_barMaxvalue = val;
	m_progress.SetPos(pos);
	m_progress.SetRange32(pos, val);
	m_progress.SetStep(1);
	m_progress.SetState(PBST_NORMAL);
}

// вывод размера получаемого образа в килобайтах.
void CHDDImgMakerDlg::OutByteSize()
{
	UpdateData(TRUE);
	const __int64 nsize = static_cast<__int64>(m_nCylinders) * m_nHeads * m_nSectors * SECTOR_SIZEB / 1024;
	CString t;
	t.Format(_T("%I64d Кб"), nsize);
	SetDlgItemText(IDC_STATIC_RESULT_SIZE, t);
}

// обновление переменных имён образов, завязанных на поля ввода
// и соответствующих им fs::path
void CHDDImgMakerDlg::_UpdateData()
{
	UpdateData(TRUE);
	m_strImgName.Trim();
	m_pathImgName = fs::path(m_strImgName.GetString()).make_preferred();
	m_strExistingImgName.Trim();
	m_pathExistingImgName = fs::path(m_strExistingImgName.GetString()).make_preferred();
	UpdateData(FALSE);
}

// корректировать геометрию, если она не соответствует размеру
// просто уменьшаем цилиндры, пока не будем влазить в размер
void CHDDImgMakerDlg::CorrectGeometry(const __int64 nRealSize)
{
	// вычисляем размер по геометрии
	__int64 nCalcSize = static_cast<__int64>(m_nCylinders) * m_nHeads * m_nSectors * SECTOR_SIZEB;

	if (nCalcSize > nRealSize) // если расчётный размер больше фактического
	{
		if (ShowMessageBox(IDS_STR_MSG_CORR_SIZE, MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
		{
			do
			{
				m_nCylinders--;
				nCalcSize = static_cast<__int64>(m_nCylinders) * m_nHeads * m_nSectors * SECTOR_SIZEB;
			}
			while (nCalcSize > nRealSize);

			UpdateData(FALSE);
			OutByteSize();
		}
	}
}

// определение формата образа
void CHDDImgMakerDlg::AnalyseImage(FILE *pFile, const __int64 nFileSize)
{
	IMGFormat imgf;
	const bool bStat = HDIStuff::CheckFormat(pFile, &imgf);

	if (imgf.nIOStatus == IMGIOSTATUS::IO_ERROR)
	{
		ShowMessageBoxErr(IDS_STR_MSG_READERROR, m_strExistingImgName, MB_OK | MB_ICONEXCLAMATION);
	}
	else
	{
		if (bStat)
		{
			// всё в порядке, можно конвертировать
			m_bConvert = true;
			// обновляем данные и запрещаем менять размеры
			m_bChangeGeometry = false;
			EnableGeometryChange(false);
			m_nCylinders = imgf.C;
			m_nHeads = imgf.H;
			m_nSectors = imgf.S;
			UpdateData(FALSE);
			CString str(MAKEINTRESOURCE(imgf.bSamara ? IDS_STR_NAME_SAMARA : IDS_STR_NAME_ALTPRO));
			SetDlgItemText(IDC_STATIC_FORMAT, str);
			OutByteSize();
			CorrectGeometry(nFileSize);
		}
		else
		{
			ShowMessageBoxErr(IDS_STR_MSG_UNKNOWNFORMAT, MB_OK | MB_ICONINFORMATION);
		}
	}
}

// вкл./выкл. элементов измениния геометрии
void CHDDImgMakerDlg::EnableGeometryChange(bool bEnable)
{
	GetDlgItem(IDC_EDIT_CYLINDERS)->EnableWindow(bEnable);
	GetDlgItem(IDC_EDIT_HEADS)->EnableWindow(bEnable);
	GetDlgItem(IDC_EDIT_SECTORS)->EnableWindow(bEnable);
	GetDlgItem(IDC_SPIN_CYLINDERS)->EnableWindow(bEnable);
	GetDlgItem(IDC_SPIN_HEADS)->EnableWindow(bEnable);
	GetDlgItem(IDC_SPIN_SECTORS)->EnableWindow(bEnable);
}

// текст на кнопке в зависимости от действия: создать/конвертировать
void CHDDImgMakerDlg::ChangeMakeButtonText()
{
	CString str(MAKEINTRESOURCE(m_bConvert ? IDS_STR_BTNTEXT_CONVERT : IDS_STR_BTNTEXT_CREATE));
	SetDlgItemText(IDC_BUTTON_MAKE, str);
}

// создание паспорта HDD
void CHDDImgMakerDlg::MakeSysSector()
{
	IMGFormat imgf;
	imgf.C = m_nCylinders;
	imgf.H = m_nHeads;
	imgf.S = m_nSectors;
	HDIStuff::CreateHDISector(&imgf, &m_sector0, std::wstring(m_strSerialNumber), std::wstring(m_strModelName));
}


void CHDDImgMakerDlg::WorkHDI(int nTotalSectors)
{
	// теперь создадим образ
	MakeSysSector();

	/*
	если мы конвертируем из существующего образа, то
	здесь надо обработчик вставить
	*/
	if (m_bConvert)
	{
		if (m_fileExisting = _tfopen(m_strExistingImgName.GetString(), _T("rb")))
		{
			if (m_file = _tfopen(m_strImgName.GetString(), _T("wb")))
			{
				fwrite(&m_sector0, 1, sizeof(SYS_SECTOR), m_file);
				std::thread thr = std::thread(&CHDDImgMakerDlg::threadFunc, this, nTotalSectors, WORK_CONVERTHDI);

				if (thr.joinable())
				{
					thr.detach();
				}
			}
			else
			{
				fclose(m_fileExisting);
				ShowMessageBoxErr(IDS_STR_MSG_CREATEERROR, m_strImgName, MB_OK | MB_ICONEXCLAMATION);
			}
		}
		else
		{
			ShowMessageBoxErr(IDS_STR_MSG_READERROR, m_strExistingImgName, MB_OK | MB_ICONEXCLAMATION);
		}
	}
	else
	{
		if (m_file = _tfopen(m_strImgName.GetString(), _T("wb")))
		{
			fwrite(&m_sector0, 1, sizeof(SYS_SECTOR), m_file);
			std::thread thr = std::thread(&CHDDImgMakerDlg::threadFunc, this, nTotalSectors, WORK_CREATEHDI);

			if (thr.joinable())
			{
				thr.detach();
			}
		}
		else
		{
			ShowMessageBoxErr(IDS_STR_MSG_CREATEERROR, m_strImgName, MB_OK | MB_ICONEXCLAMATION);
		}
	}
}

/*
нужен ини файл в котором будет храниться такая инфа
[HDIX]
Image File = (String)
Type = ATA / ATAPI - определяет, что за образ: hdd или cdrom

[HDD_ID]
тут будет храниться дамп паспорта для hdd, для сдром - пусто
0x0000 = 0x00 0x00 ... 0x00
0x0010 = 0x00 0x00 ... 0x00
вот в таком виде. имя параметра - адрес в хексе,
значение - 16 байтов в хексе.
*/
void CHDDImgMakerDlg::WorkHDIX(int nTotalSectors)
{
	MakeSysSector();
	auto pIni = std::make_unique<CIni>();
	pIni->SetIniFileName(m_pathImgName);
	// надо добавить строковые ресурсы. секцию и ключи
	pIni->SetValueString(IDS_HDIX_SECTION, IDS_HDIX_IMG_TYPE, _T("ATA"));
	// генерация дампа паспорта.
	const uint8_t *const pSector0 = reinterpret_cast<uint8_t *>(&m_sector0);
	CString strKey, strVal, str;
	int nAddr = 0;

	while (nAddr < SECTOR_SIZEB)
	{
		strKey.Format(_T("0x%04X"), nAddr); // формируем имя параметра
		strVal.Empty();

		for (int i = 0; i < HDIX_ID_ROW_LEN; ++i)
		{
			str.Format(_T("0x%02x"), pSector0[nAddr++]);

			if (i)
			{
				strVal += _T(" ");
			}

			strVal += str;
		}

		pIni->SetValueString(IDS_HDIX_IDSECTION, strKey, strVal);
	}

	if (m_bConvert)
	{
		// тут надо сконвертировать hdix файл.
		fs::path strName;

		if (_tcsicoll(m_pathImgName.stem().c_str(), m_pathExistingImgName.stem().c_str()) == 0)
		{
			// если имена файлов совпадают, то оригинальный img
			// надо скопировать под новым именем, и дать ему расширение .bak или .orig
			CopyFile(m_strExistingImgName, m_strExistingImgName + _T(".bak"), FALSE);
			strName = m_pathExistingImgName.filename();
		}
		else
		{
			strName = m_pathImgName.filename();
			strName.replace_extension(m_pathExistingImgName.extension());
			CopyFile(m_strExistingImgName, (m_pathImgName.parent_path() / strName).c_str(), FALSE);
		}

		pIni->SetValueString(IDS_HDIX_SECTION, IDS_HDIX_IMG_FILENAME, CString(strName.c_str()));
		SetProgress(nTotalSectors);

		if (pIni->FlushIni())
		{
			ShowMessageBox(IDS_STR_MSG_CONVERTSUCCESS, MB_OK | MB_ICONINFORMATION);
		}
		else
		{
			ShowMessageBoxErr(IDS_STR_MSG_CONVERTERROR, MB_OK | MB_ICONEXCLAMATION);
		}

		SetProgress(0);
	}
	else
	{
		// тут надо просто создать hdix файл.
		fs::path strName = m_pathImgName.filename();
		strName.replace_extension(strVHDDext[IMG_EXT]);
		pIni->SetValueString(IDS_HDIX_SECTION, IDS_HDIX_IMG_FILENAME, CString(strName.c_str()));
		strName = m_pathImgName.parent_path() / strName;

		if (pIni->FlushIni())
		{
			if (m_file = _tfopen(strName.c_str(), _T("wb")))
			{
				std::thread thr = std::thread(&CHDDImgMakerDlg::threadFunc, this, nTotalSectors, WORK_CREATEHDI);

				if (thr.joinable())
				{
					thr.detach();
				}
			}
			else
			{
				ShowMessageBoxErr(IDS_STR_MSG_FILECREATEERROR, strName.c_str(), MB_OK | MB_ICONEXCLAMATION);
			}
		}
		else
		{
			ShowMessageBoxErr(IDS_STR_MSG_FILECREATEERROR, m_strImgName, MB_OK | MB_ICONINFORMATION);
		}
	}

	pIni.reset();
}

void CHDDImgMakerDlg::OnBnClickedMfcmenubutton1()
{
	const CMFCMenuButton *const pMenuButton = dynamic_cast<CMFCMenuButton *>(GetDlgItem(IDC_MFCMENUBUTTON1));
	const int nSel = pMenuButton->m_nMenuResult; // выделенный пункт меню

	switch (nSel)
	{
		default:
		case CONVERT_HDI2IMG:
			ConvertHdi2Img();
			break;

		case CONVERT_HDIX2HDI:
			ConvertHdix2Hdi();
			break;

		case CONVERT_HDI2HDIX:
			ConvertHdi2Hdix();
			break;
	}
}

/*
Обратное преобразование hdi в img
*/
void CHDDImgMakerDlg::ConvertHdi2Img()
{
	CString strFilterImg(MAKEINTRESOURCE(IDS_STR_FILTERHDI));
	CFileDialog dlg(TRUE, strVHDDext[HDI_EXT].c_str(), nullptr,
	                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_FILEMUSTEXIST,
	                strFilterImg, this);

	if (dlg.DoModal() == IDOK)
	{
		fs::path strHDIName{dlg.GetPathName().Trim().GetString()};

		if (!strHDIName.empty())
		{
			fs::path strImgName = strHDIName;
			strImgName.replace_extension(strVHDDext[IMG_EXT]);

			if (m_fileExisting = _tfopen(strHDIName.c_str(), _T("rb")))
			{
				fread(&m_sector0, 1, sizeof(SYS_SECTOR), m_fileExisting); // сперва прочитаем заголовок.

				// проверим контрольную сумму, вдруг нам фуфло какое-то подсовывают
				if (HDIStuff::CheckCS(reinterpret_cast<USector *>(&m_sector0)))
				{
					if (m_file = _tfopen(strImgName.c_str(), _T("wb")))
					{
						const int nTotalSectors = m_sector0.cylinders * m_sector0.heads * m_sector0.sectors; // произведение максимальных значений влазит в 32 бита
						BAR_Normal(0, nTotalSectors);
						std::thread thr = std::thread(&CHDDImgMakerDlg::threadFunc, this, nTotalSectors, CONVERT_HDI2IMG);

						if (thr.joinable())
						{
							thr.detach();
						}
					}
					else
					{
						fclose(m_fileExisting);
						ShowMessageBoxErr(IDS_STR_MSG_CREATEERROR, strImgName.c_str(), MB_OK | MB_ICONEXCLAMATION);
					}
				}
				else
				{
					ShowMessageBoxErr(IDS_STR_MSG_CORRUPTHDI, MB_OK | MB_ICONEXCLAMATION);
				}
			}
			else
			{
				ShowMessageBoxErr(IDS_STR_MSG_READERROR, strHDIName.c_str(), MB_OK | MB_ICONEXCLAMATION);
			}
		}
	}
}

void CHDDImgMakerDlg::ConvertHdi2Hdix()
{
	CString strFilterImg(MAKEINTRESOURCE(IDS_STR_FILTERHDI));
	CFileDialog dlg(TRUE, strVHDDext[HDI_EXT].c_str(), nullptr,
	                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_FILEMUSTEXIST,
	                strFilterImg, this);

	if (dlg.DoModal() == IDOK)
	{
		fs::path strHDIName{ dlg.GetPathName().Trim().GetString() };

		if (!strHDIName.empty())
		{
			fs::path strHdixName = strHDIName;
			strHdixName.replace_extension(strVHDDext[HDIX_EXT]);
			fs::path strImgName = strHDIName;
			strImgName.replace_extension(strVHDDext[IMG_EXT]);

			if (m_fileExisting = _tfopen(strHDIName.c_str(), _T("rb")))
			{
				fread(&m_sector0, 1, sizeof(SYS_SECTOR), m_fileExisting); // сперва прочитаем заголовок.
				// проверим контрольную сумму, вдруг нам фуфло какое-то подсовывают
				auto pSector0 = reinterpret_cast<USector *>(&m_sector0);

				if (HDIStuff::CheckCS(pSector0))
				{
					auto pIni = std::make_unique<CIni>();
					pIni->SetIniFileName(strHdixName);
					// надо добавить строковые ресурсы. секцию и ключи
					pIni->SetValueString(IDS_HDIX_SECTION, IDS_HDIX_IMG_TYPE, _T("ATA"));
					// генерация дампа паспорта.
					CString strKey, strVal;
					int nAddr = 0;
					CString str;

					while (nAddr < SECTOR_SIZEB)
					{
						strKey.Format(_T("0x%04X"), nAddr); // формируем имя параметра
						strVal.Empty();

						for (int i = 0; i < HDIX_ID_ROW_LEN; ++i)
						{
							str.Format(_T("0x%02x"), pSector0->b[nAddr++]);

							if (i)
							{
								strVal += _T(" ");
							}

							strVal += str;
						}

						pIni->SetValueString(IDS_HDIX_IDSECTION, strKey, strVal);
					}

					pIni->SetValueString(IDS_HDIX_SECTION, IDS_HDIX_IMG_FILENAME, CString(strImgName.filename().c_str()));

					if (pIni->FlushIni()) // создаём hdix сигнатуру
					{
						// создаём img файл
						if (m_file = _tfopen(strImgName.c_str(), _T("wb")))
						{
							const int nTotalSectors = m_sector0.cylinders * m_sector0.heads * m_sector0.sectors; // произведение максимальных значений влазит в 32 бита
							BAR_Normal(0, nTotalSectors);
							std::thread thr = std::thread(&CHDDImgMakerDlg::threadFunc, this, nTotalSectors, CONVERT_HDI2HDIX);

							if (thr.joinable())
							{
								thr.detach();
							}
						}
						else
						{
							fclose(m_fileExisting);
							ShowMessageBoxErr(IDS_STR_MSG_CREATEERROR, strImgName.c_str(), MB_OK | MB_ICONEXCLAMATION);
						}
					}
					else
					{
						fclose(m_fileExisting);
						ShowMessageBoxErr(IDS_STR_MSG_FILECREATEERROR, strHdixName.c_str(), MB_OK | MB_ICONINFORMATION);
					}

					pIni.reset();
				}
				else
				{
					ShowMessageBoxErr(IDS_STR_MSG_CORRUPTHDI, MB_OK | MB_ICONEXCLAMATION);
				}
			}
			else
			{
				ShowMessageBoxErr(IDS_STR_MSG_READERROR, strHDIName.c_str(), MB_OK | MB_ICONEXCLAMATION);
			}
		}
	}
}

void CHDDImgMakerDlg::ConvertHdix2Hdi()
{
	CString strFilterImg(MAKEINTRESOURCE(IDS_STR_FILTERHDIX));
	CFileDialog dlg(TRUE, strVHDDext[HDIX_EXT].c_str(), nullptr,
	                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_FILEMUSTEXIST,
	                strFilterImg, this);

	if (dlg.DoModal() == IDOK)
	{
		fs::path strHdixName{ dlg.GetPathName().Trim().GetString() };

		if (!strHdixName.empty())
		{
			auto pIni = std::make_unique<CIni>();
			pIni->SetIniFileName(strHdixName);
			fs::path strHdiName = strHdixName;
			strHdiName.replace_extension(strVHDDext[HDI_EXT]);
			fs::path strImgName = strHdixName.parent_path();
			strImgName /= fs::path{ pIni->GetValueString(IDS_HDIX_SECTION, IDS_HDIX_IMG_FILENAME, _T("")).GetString() };

			if (pIni->GetValueString(IDS_HDIX_SECTION, IDS_HDIX_IMG_TYPE, _T("")).CollateNoCase(_T("ATA")) == 0)
			{
				// нужно прочитать и сформировать паспорт
				auto pSector0 = reinterpret_cast<USector *>(&m_sector0);
				// сейчас надо получить все ключи из заданной секции.
				CString strSection;
				VERIFY(strSection.LoadString(IDS_HDIX_IDSECTION));
				CString strKey, strValue;

				if (pIni->FindKeyStart(strSection, strKey, strValue))
				{
					do
					{
						int nAddr = _tcstol(strKey.GetString(), nullptr, 0);

						if (0 <= nAddr && nAddr < SECTOR_SIZEB)
						{
							auto pVal = strValue.GetBuffer();

							do
							{
								const int v = _tcstol(pVal, &pVal, 0); // с автоопределением основания
								pSector0->b[nAddr++] = v;
							}
							while (_tcslen(pVal));
						}
					}
					while (pIni->FindKeyNext(strKey, strValue));
				}

				// теперь посчитаем КС
				if (HDIStuff::CheckCS(pSector0))
				{
					if (m_fileExisting = _tfopen(strImgName.c_str(), _T("rb")))
					{
						const int nTotalSectors = m_sector0.cylinders * m_sector0.heads * m_sector0.sectors; // произведение максимальных значений влазит в 32 бита
						BAR_Normal(0, nTotalSectors);

						if (m_file = _tfopen(strHdiName.c_str(), _T("wb")))
						{
							fwrite(&m_sector0, 1, sizeof(SYS_SECTOR), m_file);
							std::thread thr = std::thread(&CHDDImgMakerDlg::threadFunc, this, nTotalSectors, CONVERT_HDIX2HDI);

							if (thr.joinable())
							{
								thr.detach();
							}
						}
						else
						{
							fclose(m_fileExisting);
							ShowMessageBoxErr(IDS_STR_MSG_CREATEERROR, strHdiName.c_str(), MB_OK | MB_ICONEXCLAMATION);
						}
					}
					else
					{
						ShowMessageBoxErr(IDS_STR_MSG_READERROR, strImgName.c_str(), MB_OK | MB_ICONEXCLAMATION);
					}
				}
				else
				{
					// не совпала КС
					ShowMessageBoxErr(IDS_STR_MSG_PASSPCHKSUMERR, MB_OK | MB_ICONEXCLAMATION);
				}
			}
			else
			{
				ShowMessageBoxErr(IDS_STR_MSG_UNABLECONVERT, strHdixName.c_str(), MB_OK | MB_ICONEXCLAMATION);
			}

			pIni.reset();
		}
	}
}

void CHDDImgMakerDlg::threadFunc(const int nTotalSectors, const int nType)
{
	std::lock_guard<std::mutex> lk(m_mutThr);
	SendMessage(WM_ONSTART_PROGRESS);
	int st = nTotalSectors / 100;
	int v = st;
	auto pSector = std::vector<uint8_t>(SECTOR_SIZEB);
	ZeroMemory(pSector.data(), SECTOR_SIZEB);

	switch (nType)
	{
		case CONVERT_HDI2IMG:
		case CONVERT_HDIX2HDI:
		case WORK_CONVERTHDI:
		{
			bool bReadData = true; // флаг продолжения, если образ короче, чем задано в геометрии

			for (int i = 0; i < nTotalSectors; ++i)
			{
				if (m_bBreakThr)
				{
					break;
				}

				if (bReadData)
				{
					const size_t r = fread(pSector.data(), 1, SECTOR_SIZEB, m_fileExisting);

					if (r != SECTOR_SIZEB)
					{
						if (!m_bReduceSize)
						{
							// если не задана опция - ограничить размер
							// то спросим, чё делать
							const int answer = ShowMessageBox(IDS_STR_MSG_BROKENHDI, MB_OKCANCEL | MB_ICONQUESTION | MB_SETFOREGROUND | MB_TOPMOST);

							if (answer == IDCANCEL)
							{
								break;
							}
						}
						else // если задана опция - ограничить размер
						{
							break; // то не дополняем нулями до полного размера, ограничимся тем, что есть
						}

						bReadData = false; // выбираем дополнение образа нулями
						ZeroMemory(pSector.data(), SECTOR_SIZEB);
					}
				}

				// для упрощения анализа нужно инвертировать БКшный образ.
				// для реального образа - не нужно.
				// HDIStuff::InverseSector(*(reinterpret_cast<USector*>(pSector.data())));
				fwrite(pSector.data(), 1, SECTOR_SIZEB, m_file);

				if (i >= v)
				{
					PostMessage(WM_ONSEND_PROGRESS, i);
					v += st;
				}
			}
		}
		break;

		case CONVERT_HDI2HDIX:
		{
			for (int i = 0; i < nTotalSectors; ++i)
			{
				if (m_bBreakThr)
				{
					break;
				}

				fread(pSector.data(), 1, SECTOR_SIZEB, m_fileExisting);
				fwrite(pSector.data(), 1, SECTOR_SIZEB, m_file);

				if (i >= v)
				{
					PostMessage(WM_ONSEND_PROGRESS, i);
					v += st;
				}
			}
		}
		break;

		case WORK_CREATEHDI:
		{
			for (int i = 0; i < nTotalSectors; ++i)
			{
				if (m_bBreakThr)
				{
					break;
				}

				fwrite(pSector.data(), 1, SECTOR_SIZEB, m_file);

				if (i >= v)
				{
					PostMessage(WM_ONSEND_PROGRESS, i);
					v += st;
				}
			}
		}
		break;
	}

	if (nType != WORK_CREATEHDI)
	{
		fclose(m_fileExisting);
	}

	fclose(m_file);
	SendMessage(WM_ONSTOP_PROGRESS);

	if (!m_bBreakThr)
	{
		PostMessage(WM_END_PROCESS, (nType == WORK_CREATEHDI) ? IDS_STR_MSG_CREATESUCCESS : IDS_STR_MSG_CONVERTSUCCESS);
	}
}



void CHDDImgMakerDlg::OnEnChangeEditCylinders()
{
	OutByteSize();
}

void CHDDImgMakerDlg::OnEnChangeEditHeads()
{
	OutByteSize();
}

void CHDDImgMakerDlg::OnEnChangeEditSectors()
{
	OutByteSize();
}

void CHDDImgMakerDlg::OnBnClickedButtonSernumber()
{
	m_strSerialNumber = CString(HDIStuff::GenerateSerialNumber().c_str());
	UpdateData(FALSE);
}

void CHDDImgMakerDlg::OnBnClickedButtonBrowse()
{
	CString strFilterImg(MAKEINTRESOURCE(IDS_STR_FILTERVHDD));
	CFileDialog dlg(TRUE, strVHDDext[HDI_EXT].c_str(), nullptr,
	                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_EXPLORER,
	                strFilterImg, this);

	if (dlg.DoModal() == IDOK)
	{
		m_strImgName = dlg.GetPathName().Trim();
		m_pathImgName = fs::path(m_strImgName.GetString()).make_preferred();
		UpdateData(FALSE);
		m_bCancel = false;
	}
	else
	{
		m_bCancel = true;
	}
}

void CHDDImgMakerDlg::OnBnClickedButtonBrowseExisting()
{
	CString strFilterImg(MAKEINTRESOURCE(IDS_STR_FILTERALLIMG));
	CFileDialog dlg(TRUE, strVHDDext[IMG_EXT].c_str(), nullptr,
	                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_EXPLORER,
	                strFilterImg, this);

	if (dlg.DoModal() == IDOK)
	{
		m_strExistingImgName = dlg.GetPathName().Trim();
		m_pathExistingImgName = fs::path(m_strExistingImgName.GetString()).make_preferred();
		UpdateData(FALSE);
		OnEnKillfocusEditExistingImgname();
	}
}

void CHDDImgMakerDlg::OnCbnSelchangeComboHditype()
{
	const int nTypeHDI = m_TypeCombo.GetCurSel();
	m_pathImgName = fs::path(m_strImgName.GetString());

	if (!m_pathImgName.empty() && _tcsicoll(m_pathImgName.extension().c_str(), strVHDDext[nTypeHDI].c_str()) != 0)
	{
		m_pathImgName.replace_extension(strVHDDext[nTypeHDI]);
		m_strImgName = CString(m_pathImgName.make_preferred().c_str());
		UpdateData(FALSE);
	}
}

void CHDDImgMakerDlg::OnBnClickedButtonMake()
{
	_UpdateData();
	const int nTypeHDI = m_TypeCombo.GetCurSel(); // и далее в зависимости от выбранного формата формировать образ.

	// если мы конвертируем, и имя не задано, то его можно создать из существующего
	if (m_bConvert && m_pathImgName.empty())
	{
		m_pathImgName = m_pathExistingImgName.parent_path() / m_pathExistingImgName.filename();
		m_strImgName = m_pathImgName.make_preferred().c_str();
	}

	// если имя файла не задано, принудительно заставим выбрать.
	while (m_pathImgName.empty())
	{
wrongName:
		OnBnClickedButtonBrowse();

		// если передумали, то просто выход.
		if (m_bCancel)
		{
			m_bCancel = false;
			return;
		}
	}

	// нужна проверка на валидность имени. т.к. там может быть и путь, а может и не быть.
	// если пути нет, надо добавить путь по умолчанию.
	fs::path strExt = m_pathImgName.extension();

	if (strExt.empty())
	{
		strExt = strVHDDext[nTypeHDI];
	}
	else if (_tcsicoll(strExt.c_str(), strVHDDext[nTypeHDI].c_str()) != 0)
	{
		strExt += strVHDDext[nTypeHDI];
	}

	fs::path strImgPath = m_pathImgName.parent_path();
	fs::path strImgName;

	if (m_pathImgName.has_stem())
	{
		strImgName = m_pathImgName.stem();
		strImgName += strExt;
	}

	// имя файла пусто, даже расширения нет
	if (strImgName.empty())
	{
		goto wrongName; // заставим выбрать или вписать хоть что-то
	}

	if (strImgPath.empty()) // если пути вообще нет
	{
		m_pathImgName = m_strExistingImgPath / strImgName; // значит создадим файл рядом с существующим образом, а если его нету - то рядом с прогой
	}
	else if (m_pathImgName.is_relative())  // если буквы диска нету, но есть путь
	{
		m_pathImgName = m_strExistingImgPath / m_pathImgName.parent_path() / strImgName; // значит человек хочет, чтобы файл создавался по пути, относительно пути существующего образа, а если его нету - то проги
	}
	else
	{
		m_pathImgName = m_pathImgName.parent_path() / strImgName;
	}

	m_strImgName = m_pathImgName.make_preferred().c_str();
	// иначе - есть всё, значит всё нормально
	UpdateData(FALSE); // заставим показать новый m_strImgName
	const int nTotalSectors = m_nCylinders * m_nHeads * m_nSectors; // произведение максимальных значений влазит в 32 бита
	BAR_Normal(0, nTotalSectors);

	// если имена исходного образа и результирующего совпадают, то ничего делать не надо
	if (m_strImgName != m_strExistingImgName)
	{
		switch (nTypeHDI)
		{
			default:
			case HDI_EXT:
				WorkHDI(nTotalSectors);
				break;

			case HDIX_EXT:
				WorkHDIX(nTotalSectors);
				break;
		}
	}
}



void CHDDImgMakerDlg::OnEnKillfocusEditImgname()
{
	_UpdateData();

	if (m_strExistingImgName.IsEmpty())
	{
		SetDlgItemText(IDC_STATIC_FORMAT, _T(""));
	}
}

// имя существующего образа изменилось.
void CHDDImgMakerDlg::OnEnKillfocusEditExistingImgname()
{
	_UpdateData();
	m_bConvert = false;
	// проверка на валидность имени. т.к. там может быть и путь, а может и не быть.
	// если пути нет, надо добавить путь по умолчанию.

	// имя файла пусто
	if (m_pathExistingImgName.empty() || !m_pathExistingImgName.has_stem())
	{
		// разрешим менять геометрию
		m_bChangeGeometry = true;
		EnableGeometryChange(true);
		ChangeMakeButtonText();
		return;
	}

	fs::path strExt = m_pathExistingImgName.extension();

	if (strExt.empty())
	{
		strExt = strVHDDext[IMG_EXT];
	}

	fs::path strImgName = m_pathExistingImgName.stem();
	strImgName += strExt;

	if (m_pathExistingImgName.parent_path().empty()) // если пути вообще нет
	{
		m_pathExistingImgName = m_strAppPath / strImgName; // значит предполагаем, что файл находится рядом с прогой
	}
	else if (m_pathExistingImgName.is_relative() && m_pathExistingImgName.has_parent_path())  // если буквы диска нету, но есть путь
	{
		m_pathExistingImgName = m_strAppPath / m_pathExistingImgName.parent_path() / strImgName; // значит человек хочет, чтобы файл искался по пути, относительно пути проги
	}
	else
	{
		m_pathExistingImgName = m_pathExistingImgName.parent_path() / strImgName;
	}

	m_strExistingImgName = m_pathExistingImgName.c_str();
	UpdateData(FALSE);
	// теперь, после окончательного формирования выделим путь, который получился в результате
	m_strExistingImgPath = m_pathExistingImgName.parent_path();
	// иначе - есть всё, значит всё нормально
	SetDlgItemText(IDC_STATIC_FORMAT, _T(""));

	// теперь открываем файл и анализируем его.
	if (m_fileExisting = _tfopen(m_pathExistingImgName.c_str(), _T("rb")))
	{
		// определяем тип образа: AltPro/Samara, читаем геометрию, обновляем данные
		// и запрещаем менять размеры, если всё нормально.
		const __int64 nFileSize = fs::file_size(m_pathExistingImgName); // получим фактический размер файла
		AnalyseImage(m_fileExisting, nFileSize);
		fclose(m_fileExisting);
	}
	else
	{
		ShowMessageBox(IDS_STR_MSG_OPENERROR, m_strExistingImgName, MB_OK | MB_ICONEXCLAMATION);
		m_strExistingImgName.Empty();
		m_pathExistingImgName.clear();
		UpdateData(FALSE);
	}

	ChangeMakeButtonText();
}


void CHDDImgMakerDlg::OnEnKillfocusEditSectors()
{
	UpdateData(TRUE);
}

void CHDDImgMakerDlg::OnEnSetfocusEditSectors()
{
	UpdateData(FALSE);
}

void CHDDImgMakerDlg::OnEnKillfocusEditHeads()
{
	UpdateData(TRUE);
}

void CHDDImgMakerDlg::OnEnSetfocusEditHeads()
{
	UpdateData(FALSE);
}

void CHDDImgMakerDlg::OnEnKillfocusEditCylinders()
{
	UpdateData(TRUE);
}

void CHDDImgMakerDlg::OnEnSetfocusEditCylinders()
{
	UpdateData(FALSE);
}


LRESULT CHDDImgMakerDlg::OnEndProcess(WPARAM wp, LPARAM)
{
	ShowMessageBox(UINT(wp), MB_OK | MB_ICONINFORMATION);
	SetProgress(0);
	return S_OK;
}

LRESULT CHDDImgMakerDlg::OnStartProgress(WPARAM, LPARAM)
{
	EnableGeometryChange(false);
	GetDlgItem(IDC_BUTTON_MAKE)->EnableWindow(FALSE);
	GetDlgItem(IDC_MFCMENUBUTTON1)->EnableWindow(FALSE);
	return S_OK;
}

LRESULT CHDDImgMakerDlg::OnStopProgress(WPARAM, LPARAM)
{
	SetProgress(m_barMaxvalue);
	EnableGeometryChange(m_bChangeGeometry);
	GetDlgItem(IDC_BUTTON_MAKE)->EnableWindow(TRUE);
	GetDlgItem(IDC_MFCMENUBUTTON1)->EnableWindow(TRUE);
	return S_OK;
}

LRESULT CHDDImgMakerDlg::OnSendProgress(WPARAM wp, LPARAM)
{
	SetProgress(int(wp));
	return S_OK;
}

void CHDDImgMakerDlg::OnBnClickedCheckReducesize()
{
	UpdateData(TRUE);
	UpdateData(FALSE);
}
