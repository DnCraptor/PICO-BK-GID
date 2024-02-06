
// BKMEMDlg.cpp : файл реализации
//

#include "pch.h"
#include "Config.h"
#include "BKMEMDlg.h"
#include "BKMessageBox.h"
#include "Screen.h"
#include "SprWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CBKMEMDlg, CBaseDialog)

LPCTSTR	CBKMEMDlg::m_lpClassName = _T("MySprClass");

// диалоговое окно CBKMEMDlg
CBKMEMDlg::CBKMEMDlg(BK_DEV_MPI nBKModel, BK_DEV_MPI nBKFDDModel, uint8_t *MainMem, uint8_t *AddMem, CWnd *pParent /*=nullptr*/)
	: CBaseDialog(CBKMEMDlg::IDD, pParent)
	, m_Memory(MainMem)
	, m_MemoryADD(AddMem)
	, m_BKModel(nBKModel)
	, m_BKFDDmodel(nBKFDDModel)
	, m_nTabsCount(0)
	, m_nSelectedTab(0)
{
	ZeroMemory(m_Container, sizeof(m_Container));
	m_Screen[MM_FIRST_PAGE] = nullptr;
	m_Screen[MM_SECOND_PAGE] = nullptr;
}

CBKMEMDlg::~CBKMEMDlg()
{
	UnregisterClass(m_lpClassName, AfxGetInstanceHandle());
}

void CBKMEMDlg::DoDataExchange(CDataExchange *pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_MM_MEMTABS, m_tab);
}

BEGIN_MESSAGE_MAP(CBKMEMDlg, CBaseDialog)
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_MM_MEMTABS, &CBKMEMDlg::OnTcnSelchangeTab1)
	ON_BN_CLICKED(IDC_BUTTON_MM_COLOR_MODE_P1, &CBKMEMDlg::OnBnClickedButtonMmColorModeP1)
	ON_BN_CLICKED(IDC_BUTTON_MM_BW_MODE_P1, &CBKMEMDlg::OnBnClickedButtonMmBwModeP1)
	ON_BN_CLICKED(IDC_BUTTON_MM_LOAD_P1, &CBKMEMDlg::OnBnClickedButtonMmLoadP1)
	ON_BN_CLICKED(IDC_BUTTON_MM_SAVE_P1, &CBKMEMDlg::OnBnClickedButtonMmSaveP1)
	ON_BN_CLICKED(IDC_BUTTON_MM_SPRITE_P1, &CBKMEMDlg::OnBnClickedButtonMmSpriteP1)
	ON_BN_CLICKED(IDC_BUTTON_MM_COLOR_MODE_P2, &CBKMEMDlg::OnBnClickedButtonMmColorModeP2)
	ON_BN_CLICKED(IDC_BUTTON_MM_BW_MODE_P2, &CBKMEMDlg::OnBnClickedButtonMmBwModeP2)
	ON_BN_CLICKED(IDC_BUTTON_MM_LOAD_P2, &CBKMEMDlg::OnBnClickedButtonMmLoadP2)
	ON_BN_CLICKED(IDC_BUTTON_MM_SAVE_P2, &CBKMEMDlg::OnBnClickedButtonMmSaveP2)
	ON_BN_CLICKED(IDC_BUTTON_MM_SPRITE_P2, &CBKMEMDlg::OnBnClickedButtonMmSpriteP2)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

struct btnList
{
	UINT nBtnID;
	UINT nToolTipID;
	int nIcon;
};

static btnList mmButtons[10] =
{
	{ IDC_BUTTON_MM_COLOR_MODE_P1,   IDS_BUTTON_COLORMODE_STR, 0 },
	{ IDC_BUTTON_MM_COLOR_MODE_P2,   IDS_BUTTON_COLORMODE_STR, 0 },
	{ IDC_BUTTON_MM_BW_MODE_P1,      IDS_BUTTON_ADAPTIVEBWMODE_STR, 1 },
	{ IDC_BUTTON_MM_BW_MODE_P2,      IDS_BUTTON_ADAPTIVEBWMODE_STR, 1 },
	{ IDC_BUTTON_MM_LOAD_P1,         IDS_BUTTON_LOAD_STR, 2 },
	{ IDC_BUTTON_MM_LOAD_P2,         IDS_BUTTON_LOAD_STR, 2 },
	{ IDC_BUTTON_MM_SAVE_P1,         IDS_BUTTON_SAVE_STR, 3 },
	{ IDC_BUTTON_MM_SAVE_P2,         IDS_BUTTON_SAVE_STR, 3 },
	{ IDC_BUTTON_MM_SPRITE_P1,       IDS_BUTTON_SPRITE_STR, 4 },
	{ IDC_BUTTON_MM_SPRITE_P2,       IDS_BUTTON_SPRITE_STR, 4 }
};

BOOL CBKMEMDlg::OnInitDialog()
{
	CBaseDialog::OnInitDialog();
	CreateScreen(MM_FIRST_PAGE, IDC_STATIC_MM_P1);
	CreateScreen(MM_SECOND_PAGE, IDC_STATIC_MM_P2);
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
	CBitmap m_bmparr;
	m_bmparr.LoadBitmap(IDB_MM_BUTTONS);
	CImageList m_imagelist;
	m_imagelist.Create(16, 15, ILC_COLORDDB | ILC_MASK, 0, 0);
	m_imagelist.Add(&m_bmparr, RGB(192, 192, 192));

	for (auto &n : mmButtons)
	{
		CWnd *p = GetDlgItem(n.nBtnID);
		m_ToolTip.AddTool(p, n.nToolTipID);
		reinterpret_cast<CButton *>(p)->SetIcon(m_imagelist.ExtractIcon(n.nIcon));
	}

	m_imagelist.DeleteImageList();
	m_bmparr.DeleteObject();

	// создаём вкладки
	switch (m_BKModel)
	{
		case BK_DEV_MPI::BK0010:
			CreateTabs_10();
			break;

		case BK_DEV_MPI::BK0011:
		case BK_DEV_MPI::BK0011M:
			CreateTabs_11M();
			break;

		default:
			ASSERT(false); // неопределённых значений не должно быть в принципе
	}

	switch (m_BKFDDmodel)
	{
		case BK_DEV_MPI::STD_FDD:
		case BK_DEV_MPI::SAMARA:
			if (m_BKModel == BK_DEV_MPI::BK0010)
			{
				AddTabsEXT16();
			}

			break;

		case BK_DEV_MPI::A16M:
			AddTabsA16M();
			break;

		case BK_DEV_MPI::SMK512:
			AddTabsSMK512();
			break;
	}

	SelectTab(); // выберем какую-нибудь вкладку (по умолчанию - нулевую)
	CenterWindow(GetParent());
	return TRUE; // возврат значения TRUE, если фокус не передан элементу управления
}

void CBKMEMDlg::SelectTab()
{
	if (m_Container[m_nSelectedTab][MM_FIRST_PAGE].bExist)
	{
		SetTabParam(MM_FIRST_PAGE);
	}

	if (m_Container[m_nSelectedTab][MM_SECOND_PAGE].bExist)
	{
		m_Screen[MM_SECOND_PAGE]->ShowWindow(SW_SHOW);
		SetTabParam(MM_SECOND_PAGE);
	}
	else
	{
		m_Screen[MM_SECOND_PAGE]->ShowWindow(SW_HIDE);
	}
}

void CBKMEMDlg::SetTabParam(const int nPage)
{
	MM16kPage_t &pp = m_Container[m_nSelectedTab][nPage];
	// переключим отображаемый буфер
	m_Screen[nPage]->ChangeBuffer(pp.pBuffer, pp.nBufSize);
	// зададим режимы отображения
	SetColormode(nPage, pp.bColorMode);
	SetBWMode(nPage, pp.bBWAdaptMode);
}

void CBKMEMDlg::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	m_nSelectedTab = m_tab.GetCurSel();

	if (m_nSelectedTab < 0)
	{
		m_nSelectedTab = 0;
	}

	SelectTab();
	*pResult = S_OK;
}

static UINT mmBtn2[2][5] =
{
	{
		IDC_BUTTON_MM_COLOR_MODE_P1,
		IDC_BUTTON_MM_BW_MODE_P1,
		IDC_BUTTON_MM_SPRITE_P1,
		IDC_BUTTON_MM_SAVE_P1,
		IDC_BUTTON_MM_LOAD_P1,
	},
	{
		IDC_BUTTON_MM_COLOR_MODE_P2,
		IDC_BUTTON_MM_BW_MODE_P2,
		IDC_BUTTON_MM_SPRITE_P2,
		IDC_BUTTON_MM_SAVE_P2,
		IDC_BUTTON_MM_LOAD_P2,
	}
};


void CBKMEMDlg::CreateScreen(const int nPage, const int idUI)
{
	CString str;
	str.Format(_T("BKScreen_P%d"), nPage + 1);
	// если рендер D3D - то карту памяти выводим, используя DrawDIB
	m_Screen[nPage] = std::make_unique<CScreen>((g_Config.m_nScreenRenderType == CONF_SCREEN_RENDER::D3D ? CONF_SCREEN_RENDER::VFW : g_Config.m_nScreenRenderType));

	if (m_Screen[nPage])
	{
		CRect rect;
		GetDlgItem(idUI)->GetWindowRect(&rect);
		ScreenToClient(&rect);
		GetDlgItem(idUI)->DestroyWindow();
		const int nPixelW = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSX);
		const int nPixelH = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSY);
		rect.right = rect.left + ::MulDiv(BK_SCREEN_WIDTH, nPixelW, DEFAULT_DPIX);
		rect.bottom = rect.top + ::MulDiv(BK_SCREEN_HEIGHT, nPixelH, DEFAULT_DPIY);

		// размеры - константа 512х256
		if (!m_Screen[nPage]->Create(nullptr, str,
		                             WS_VISIBLE | WS_CHILD,
		                             rect,
		                             &m_tab, 0)
		   )
		{
			TRACE(_T("Не удалось создать экран %s\n"), str);
			m_Screen[nPage].reset(); // не удалось создать
		}

		// подвинем кнопки
		CPoint pt = { rect.right + ::MulDiv(14, nPixelW, DEFAULT_DPIX), rect.top }; // это верхний левый угол, с которого начинается столбец кнопок
		const int nMargin = ::MulDiv(4, nPixelH, DEFAULT_DPIY);
		CRect sr, dr;
		int i = 0;

		for (; i < 2; ++i)
		{
			CWnd *p = GetDlgItem(mmBtn2[nPage][i]);
			p->GetWindowRect(sr);
			dr = { pt.x, pt.y, pt.x + sr.Width(), pt.y + sr.Height() };
			p->MoveWindow(&dr, FALSE);
			pt.y += sr.Height() + nMargin;
		}

		pt.y = rect.bottom; // это нижний левый угол, с которого начинается столбец кнопок

		for (; i < 5; ++i)
		{
			CWnd *p = GetDlgItem(mmBtn2[nPage][i]);
			p->GetWindowRect(sr);
			dr = { pt.x, pt.y - sr.Height(), pt.x + sr.Width(), pt.y };
			p->MoveWindow(&dr, FALSE);
			pt.y -= sr.Height() + nMargin;
		}

		this->GetWindowRect(rect);
		ScreenToClient(&rect);
		const int x = rect.left;
		rect.left -= x; rect.right -= x;
		const int y = rect.top;
		rect.top -= y; rect.bottom -= y;
		rect.right = -x + dr.right + ::MulDiv(14, nPixelW, DEFAULT_DPIX);
		ClientToScreen(&rect);
		MoveWindow(&rect, FALSE);
	}
	else
	{
		// не удалось создать
		TRACE(_T("Недостаточно памяти для создания экрана %s\n"), str);
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}
}

void CBKMEMDlg::CreateTabs_10()
{
	m_nTabsCount = 1;
	CString strtab;

	for (int i = 0; i < m_nTabsCount; ++i)
	{
		const size_t nOffs = size_t(i) * 0100000;
		m_Container[i][MM_FIRST_PAGE]  = { true, true, false, m_Memory + nOffs,          040000 };
		m_Container[i][MM_SECOND_PAGE] = { true, true, false, m_Memory + nOffs + 040000, 040000 };
		strtab.Format(_T("BK10_pg%d%d"), i * 2, i * 2 + 1);
		m_tab.InsertItem(i, strtab);
	}
}

void CBKMEMDlg::CreateTabs_11M()
{
	m_nTabsCount = 4;
	static int Pages[8] = { 0, 1, 2, 3, 5, 6, 4, 7 };
	CString strtab;

	for (int i = 0; i < m_nTabsCount; ++i)
	{
		const int n0 = Pages[i * 2];
		const int n1 = Pages[i * 2 + 1];
		m_Container[i][MM_FIRST_PAGE]  = { true, true, false, m_Memory + (size_t(n0) << 14), 040000 };
		m_Container[i][MM_SECOND_PAGE] = { true, true, false, m_Memory + (size_t(n1) << 14), 040000 };
		strtab.Format(_T("BK11M_pg%d%d"), n0, n1);
		m_tab.InsertItem(i, strtab);
	}
}

void CBKMEMDlg::AddTabsEXT16()
{
	const int i = m_nTabsCount++;
	CString strtab;
	m_Container[i][MM_FIRST_PAGE] = { true, true, false, m_MemoryADD, 040000 };
	m_Container[i][MM_SECOND_PAGE] = { false, false, false, nullptr, 0 };
	m_tab.InsertItem(i, _T("Ext16k"));
}

void CBKMEMDlg::AddTabsA16M()
{
	const int i = m_nTabsCount++;
	CString strtab;
	m_Container[i][MM_FIRST_PAGE]  = { true, true, false, m_MemoryADD, 040000 };
	m_Container[i][MM_SECOND_PAGE] = { false, false, false, nullptr, 0};
	m_tab.InsertItem(i, _T("A16M_pg0"));
}

void CBKMEMDlg::AddTabsSMK512()
{
	const int n = m_nTabsCount;
	m_nTabsCount += 16;
	CString strtab;

	for (int i = n, j = 0; i < m_nTabsCount; ++i, ++j)
	{
		const size_t nOffs = size_t(j) * 0100000;
		m_Container[i][MM_FIRST_PAGE]  = { true, true, false, m_MemoryADD + nOffs,          040000 };
		m_Container[i][MM_SECOND_PAGE] = { true, true, false, m_MemoryADD + nOffs + 040000, 040000 };
		strtab = _T("SMK_") + g_arStrSMKPgNums[j];
		m_tab.InsertItem(i, strtab);
	}
}

void CBKMEMDlg::DrawTab()
{
	if (!m_lockStop.IsLocked())
	{
		m_lockDraw.Lock();

		if (m_Container[m_nSelectedTab][MM_FIRST_PAGE].bExist)
		{
			m_Screen[MM_FIRST_PAGE]->ReDrawScreen();
		}

		if (m_Container[m_nSelectedTab][MM_SECOND_PAGE].bExist)
		{
			m_Screen[MM_SECOND_PAGE]->ReDrawScreen();
		}

		m_lockDraw.UnLock();
	}
}


void CBKMEMDlg::OnCancel()
{
	CBaseDialog::OnCancel();
	DestroyWindow();
}



void CBKMEMDlg::OnDestroy()
{
	// посылаем в MainFrame сообщение о закрытии всего этого тут
	GetParentFrame()->SendMessage(WM_MEMMAP_CLOSE);
	CBaseDialog::OnDestroy();
	m_lockStop.Lock();

	while (m_lockDraw.IsLocked())
	{
		Sleep(1); // тут может быть дедлок. если UI в m_pdlgBKMem->DrawTab() захочет сообщений;
	}

	m_tab.DeleteAllItems();
	// DestroyWindow надо делать вручную
	m_Screen[MM_FIRST_PAGE]->DestroyWindow();
	m_Screen[MM_SECOND_PAGE]->DestroyWindow();
	delete this; // самоудаляемся
	// и тут надо быть осторожным. Всё, что навыделяли - удалять вручную.
}

BOOL CBKMEMDlg::PreTranslateMessage(MSG *pMsg)
{
	if (pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MOUSELAST)
	{
		m_ToolTip.RelayEvent(pMsg);
	}

	return CBaseDialog::PreTranslateMessage(pMsg);
}


void CBKMEMDlg::OnBnClickedButtonMmColorModeP1()
{
	ChangeColormode(MM_FIRST_PAGE);
}


void CBKMEMDlg::OnBnClickedButtonMmBwModeP1()
{
	ChangeBWMode(MM_FIRST_PAGE);
}


void CBKMEMDlg::OnBnClickedButtonMmLoadP1()
{
	LoadImg(MM_FIRST_PAGE);
}


void CBKMEMDlg::OnBnClickedButtonMmSaveP1()
{
	SaveImg(MM_FIRST_PAGE);
}


void CBKMEMDlg::OnBnClickedButtonMmSpriteP1()
{
	ViewSprite(MM_FIRST_PAGE);
}


void CBKMEMDlg::OnBnClickedButtonMmColorModeP2()
{
	ChangeColormode(MM_SECOND_PAGE);
}


void CBKMEMDlg::OnBnClickedButtonMmBwModeP2()
{
	ChangeBWMode(MM_SECOND_PAGE);
}


void CBKMEMDlg::OnBnClickedButtonMmLoadP2()
{
	LoadImg(MM_SECOND_PAGE);
}


void CBKMEMDlg::OnBnClickedButtonMmSaveP2()
{
	SaveImg(MM_SECOND_PAGE);
}


void CBKMEMDlg::OnBnClickedButtonMmSpriteP2()
{
	ViewSprite(MM_SECOND_PAGE);
}

void CBKMEMDlg::ChangeColormode(const int nPage)
{
	MM16kPage_t &pp = m_Container[m_nSelectedTab][nPage];
	const bool b = !pp.bColorMode;
	pp.bColorMode = b;
	SetColormode(nPage, b);
}

void CBKMEMDlg::SetColormode(const int nPage, const bool bMode)
{
	m_Screen[nPage]->SetColorMode(bMode);

	switch (nPage)
	{
		case MM_FIRST_PAGE:
			reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_MM_BW_MODE_P1))->EnableWindow(!bMode);
			break;

		case MM_SECOND_PAGE:
			reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_MM_BW_MODE_P2))->EnableWindow(!bMode);
			break;
	}
}

void CBKMEMDlg::ChangeBWMode(const int nPage)
{
	MM16kPage_t &pp = m_Container[m_nSelectedTab][nPage];
	const bool b = !pp.bBWAdaptMode;
	pp.bBWAdaptMode = b;
	SetBWMode(nPage, b);
}

void CBKMEMDlg::SetBWMode(const int nPage, const bool bMode)
{
	m_Screen[nPage]->SetAdaptMode(bMode);
}

void CBKMEMDlg::ViewSprite(const int nPage)
{
	MM16kPage_t &pp = m_Container[m_nSelectedTab][nPage];
	auto ptr = std::make_unique<uint8_t[]>(pp.nBufSize); // сделаем копию
	memcpy(ptr.get(), pp.pBuffer, pp.nBufSize);         // и передадим ее просмотрщику, чтобы статичная картинка была
	auto pSprWnd = new CSprWnd(std::move(ptr), pp.nBufSize); // обязательно создавать динамически.

	if (pSprWnd)
	{
		WNDCLASS wc;
		memset(&wc, 0, sizeof(wc));
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.hInstance = AfxGetInstanceHandle();
		wc.lpfnWndProc = ::DefWindowProc;
		wc.lpszClassName = m_lpClassName;
		wc.hbrBackground = (HBRUSH)::GetStockObject(GRAY_BRUSH);
		AfxRegisterClass(&wc);
		CString str, str2;
		m_tab.GetWindowText(str);
		str2.Format(_T(" %d"), nPage);

		if (pSprWnd->Create(
		            m_lpClassName, // Имя класса виндовс
		            _T("Sprite View : ") + str + str2, // имя окна
		            WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, // window styles
		            CRect(0, 0, 200, 200), // размер окна
		            GetParentOwner(), // родитель окна
		            nullptr,
		            WS_EX_TOOLWINDOW // | WS_EX_OVERLAPPEDWINDOW // extended window styles
		        ))
		{
			pSprWnd->ShowWindow(SW_SHOW);
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}

	m_Screen[nPage]->SetFocus();
}

#pragma warning(disable:4996)

void CBKMEMDlg::SaveImg(const int nPage)
{
	MM16kPage_t &pp = m_Container[m_nSelectedTab][nPage];

	if (!!(::GetAsyncKeyState(VK_SHIFT) & 0x8000))
	{
		// сохраним страницу как бин файл
		const uint16_t nAddr = 040000;
		const uint16_t nLen = pp.nBufSize;
		CString strDefExt(MAKEINTRESOURCE(IDS_FILEEXT_BINARY));
		CString strFilter(MAKEINTRESOURCE(IDS_FILEFILTER_BIN));
		CFileDialog dlg(FALSE, strDefExt, nullptr,
		                OFN_HIDEREADONLY | OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_EXPLORER,
		                strFilter, this);
		dlg.GetOFN().lpstrInitialDir = g_Config.m_strBinPath.c_str(); // диалог всегда будем начинать с директории для bin файлов

		if (dlg.DoModal() == IDOK)
		{
			fs::path str = dlg.GetPathName().GetString();
			Global::SaveBinFile(pp.pBuffer, nAddr, nLen, str);
		}
	}
	else
	{
		HBITMAP hBm = nullptr;
		std::unique_ptr<uint32_t[]> pNewBits;

		if (pp.bColorMode)
		{
			// сохранение страницы как изображения. совершенно особый подход.
			// сохранять будем как есть
			pNewBits = std::make_unique<uint32_t[]>(256 * 256); // новое битовое поле

			if (pNewBits)
			{
				int nbi = 0;

				// формируем битмап
				for (int y = 0; y < 256; ++y)
				{
					// алгоритм не оптимален, но нам нужна принципиальная работоспособность
					// оптимизируем потом
					for (int x = 0; x < 256; ++x)
					{
						// берём очередной бит.
						int b = y * 0100 + x / 4;
						int m = ((x % 4) * 2);
						uint8_t v = pp.pBuffer[b];
						int c = (v >> m) & 3;
						pNewBits[nbi++] = g_pColorPalettes[0][c];
					}
				}

				hBm = CreateBitmap(256, 256, 1, 32, pNewBits.get());
			}
			else
			{
				g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
			}
		}
		else
		{
			// создаём чёрно-белую картинку
			pNewBits = std::make_unique<uint32_t[]>(512 * 256); // новое битовое поле

			if (pNewBits)
			{
				int nbi = 0;

				// формируем битмап
				for (int y = 0; y < 256; ++y)
				{
					// алгоритм не оптимален, но нам нужна принципиальная работоспособность
					// оптимизируем потом
					for (int x = 0; x < 512; ++x)
					{
						// берём очередной бит.
						int b = y * 0100 + x / 8;
						int m = (x % 8);
						uint8_t v = pp.pBuffer[b];
						int c = (v >> m) & 1;
						pNewBits[nbi++] = g_pMonochromePalette[0][c];
					}
				}

				hBm = CreateBitmap(512, 256, 1, 32, pNewBits.get());
			}
			else
			{
				g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
			}
		}

		if (pNewBits)
		{
			CImage img;
			img.Attach(hBm);
			CString strFilter;
			CSimpleArray<GUID> aguidFileTypes;
			img.GetExporterFilterString(strFilter, aguidFileTypes);
			CFileDialog dlg(FALSE, _T("jpg"), nullptr,
			                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER,
			                strFilter, this);

			if (dlg.DoModal() == IDOK)
			{
				DWORD nFilterSave = dlg.m_ofn.nFilterIndex;
				GUID guid = nFilterSave > 0 ? aguidFileTypes[nFilterSave - 1] : GUID(GUID_NULL);
				CString strFileName = dlg.GetPathName();

				if (dlg.GetFileExt().IsEmpty())
				{
					if (strFileName[strFileName.GetLength() - 1] == '.')
					{
						strFileName = strFileName.Left(strFileName.GetLength() - 1);
					}

					CString strExt;

					if (nFilterSave == 0)
					{
						strExt = _T(".jpg"); // default to JPEG
					}
					else
					{
						// Look up the first extension in the filters
						int nCount = (nFilterSave * 2) - 1;
						int nLeft = 0;

						while (nCount)
						{
							if (strFilter[nLeft++] == '|')
							{
								nCount--;
							}
						}

						ASSERT(nLeft < strFilter.GetLength());
						strExt = strFilter.Tokenize(_T(";|"), nLeft).MakeLower();
						strExt = ::PathFindExtension(strExt);
					}

					strFileName += strExt;
				}

				img.Save(strFileName, guid);
			}

			img.Detach();
			DeleteObject(hBm);
		}
	}

	m_Screen[nPage]->SetFocus();
}

void CBKMEMDlg::LoadImg(const int nPage)
{
	MM16kPage_t &pp = m_Container[m_nSelectedTab][nPage];

	if (!!(::GetAsyncKeyState(VK_SHIFT) & 0x8000))
	{
		// загрузим страницу как бин файл
		CString strDefExt(MAKEINTRESOURCE(IDS_FILEEXT_BINARY));
		CString strFilter(MAKEINTRESOURCE(IDS_FILEFILTER_BIN));
		CFileDialog dlg(TRUE, strDefExt, nullptr,
		                OFN_HIDEREADONLY | OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_EXPLORER,
		                strFilter, this);
		dlg.GetOFN().lpstrInitialDir = g_Config.m_strBinPath.c_str(); // диалог всегда будем начинать с директории для bin файлов

		if (dlg.DoModal() == IDOK)
		{
			fs::path str = dlg.GetPathName().GetString();
			uint16_t nAddr, nLen;
			std::unique_ptr<uint8_t[]> buf;

			if (Global::LoadBinFile(buf, nAddr, nLen, str, true))
			{
				if (nLen >= pp.nBufSize)
				{
					nLen = pp.nBufSize;
				}

				memcpy(pp.pBuffer, buf.get(), nLen);
			}
		}
	}
	else
	{
		CImage img;
		CString strImporters;
		CSimpleArray<GUID> aguidFileTypes;
		CString sf(MAKEINTRESOURCE(IDS_FILEFILTER_IMGLOAD));
		img.GetImporterFilterString(strImporters, aguidFileTypes, sf);
		CFileDialog dlg(TRUE, _T("png"), nullptr,
		                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER,
		                strImporters, this);

		if (dlg.DoModal() == IDOK)
		{
			if (SUCCEEDED(img.Load(dlg.GetPathName())))
			{
				int w = img.GetWidth();
				int h = img.GetHeight();

				if (pp.bColorMode)
				{
					w = min(256, w); // если подсовывают большую картинку -
					h = min(256, h); // будем тупо обрезать её

					for (int y = 0; y < h; ++y)
					{
						// алгоритм не оптимален, но нам нужна принципиальная работоспособность
						// оптимизируем потом
						for (int x = 0; x < w; ++x)
						{
							// берём очередной пиксел.
							const COLORREF c = img.GetPixel(x, y);
							const int b = y * 0100 + x / 4; // байт
							const int m = ((x % 4) * 2); // маска в нём
							// теперь определим, какой цвет у пикселя
							int bc = 0; // чёрный
							const int cr = (c & 0x0000ff);
							const int cg = (c & 0x00ff00) >> 8;
							const int cb = (c & 0xff0000) >> 16;
							// теперь узнаем, какого цвета больше.
							const int p = max(max(cg, cb), cr);

							if (p > 0x3f) //если макс значение выше порога, то только тогда будем определять цвет
							{
								if (cr > max(cg, cb))
								{
									bc = 3; // то это красный цвет
								}
								else if (cg > max(cr, cb))
								{
									bc = 2; // то это зелёный цвет
								}
								else if (cb > max(cr, cg))
								{
									bc = 1; // то это синий цвет
								}
							}

							// все остальные комбинации - чёрный цвет. нефиг тут.
							pp.pBuffer[b] &= ~(3 << m);
							pp.pBuffer[b] |= bc << m; // задаём цвет
						}
					}
				}
				else
				{
					w = min(512, w); // если подсовывают большую картинку -
					h = min(256, h); // будем тупо обрезать её

					for (int y = 0; y < h; ++y)
					{
						// алгоритм не оптимален, но нам нужна принципиальная работоспособность
						// оптимизируем потом
						for (int x = 0; x < w; ++x)
						{
							// берём очередной пиксел.
							const COLORREF c = img.GetPixel(x, y);
							const int b = y * 0100 + x / 8; // байт
							const int m = (x % 8); // маска в нём
							// теперь определим, какой цвет у пикселя
							int bc = 0; // чёрный
							const int cr = (c & 0x0000ff);
							const int cg = (c & 0x00ff00) >> 8;
							const int cb = (c & 0xff0000) >> 16;
							const int p = max(max(cg, cb), cr);

							if (p > 0x7f)
							{
								bc = 1; // то это белый цвет
							}

							// все остальные комбинации - чёрный цвет. нефиг тут.
							pp.pBuffer[b] &= ~(1 << m);
							pp.pBuffer[b] |= bc << m; // задаём цвет
						}
					}
				}

				img.Destroy();
			}
		}
	}

	m_Screen[nPage]->SetFocus();
}


/*
вот так эта вся херня выглядит.
CBKMEMDlg-----------------------------------------------+
| CTabCtrl----------------------------------------+     |
| | CScreen-------------------------------------+ | +-+ |
| | |                                           | | +-+ |
| | |                                           | | +-+ |
| | |                                           | | +-+ |
| | |                                           | | +-+ |
| | |                                           | | +-+ |
| | +-------------------------------------------+ |     |
| | CScreen-------------------------------------+ | +-+ |
| | |                                           | | +-+ |
| | |                                           | | +-+ |
| | |                                           | | +-+ |
| | |                                           | | +-+ |
| | |                                           | | +-+ |
| | +-------------------------------------------+ |     |
| +-----------------------------------------------+     |
+-------------------------------------------------------+
Справа от CTabCtrl - кнопки
*/



