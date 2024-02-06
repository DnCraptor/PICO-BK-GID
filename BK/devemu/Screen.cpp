// Screen.cpp: файл реализации
//

#include "pch.h"
#include "Screen.h"
#include "Config.h"

#undef BKSCRDLL_EXPORTS
#ifdef UI
#include "BKScreenD2D\BKScreenD2D.h"
#ifdef TARGET_WINXP
#include "BKScreenD3D\BKScreenD3D.h"
#else
#include "BKScreenD3D11\BKScreenD3D.h"
#endif
#include "BKScreenDIB\BKScreenDIB.h"
#include "BKScreenOGL\BKScreenOGL.h"

#include "BKMessageBox.h"

// CScreen

IMPLEMENT_DYNAMIC(CScreen, CWnd)

CScreen::CScreen(CONF_SCREEN_RENDER nRenderType)
	: m_pBuffer(nullptr)
	, m_nBufSize(0)
	, m_pscrSharedFunc(nullptr)
	, m_hModule(nullptr)
	, m_pColTable32(nullptr)
	, m_pMonoTable32(nullptr)
	, m_bSmoothing(false)
	, m_bColorMode(true)
	, m_bAdapt(false)
	, m_bExtended(false)
	, m_bLuminoforeEmul(false)
	, m_nOfs(0330)
	, m_nPaletteNum_m256(0)
	, m_nFrame(0)
	, m_nCurFPS(0)
	, m_pCurrentScreen(nullptr)
	, m_nCurrentScreen(0)
	, m_hChildStd_IN_Rd(nullptr)
	, m_hChildStd_IN_Wr(nullptr)
	, m_bCaptureProcessed(false)
	, m_bCaptureFlag(false)

#if (_DEBUG && DBG_OUT_SCREENFRAMELENGTH)
	, m_nTickCounter(0)
	, dbgFile(nullptr)
#endif // _DEBUG

	  // мышь марсианка
	, m_nPointX(0)
	, m_nPointY(0)
	, m_MouseValue(0)
	, m_bMouseMove(false)
	, m_bMouseOutEna(false)
	, m_nMouseEnaStrobe(0)
{
	InitVars(nRenderType);
}

CScreen::CScreen(CONF_SCREEN_RENDER nRenderType, uint8_t *buffer, uint16_t size)
	: m_pBuffer(buffer)
	, m_nBufSize(size)
	, m_pscrSharedFunc(nullptr)
	, m_hModule(nullptr)
	, m_pColTable32(nullptr)
	, m_pMonoTable32(nullptr)
	, m_bSmoothing(false)
	, m_bColorMode(true)
	, m_bAdapt(false)
	, m_bExtended(false)
	, m_bLuminoforeEmul(false)
	, m_nOfs(0330)
	, m_nPaletteNum_m256(0)
	, m_nFrame(0)
	, m_nCurFPS(0)
	, m_pCurrentScreen(nullptr)
	, m_hChildStd_IN_Rd(nullptr)
	, m_hChildStd_IN_Wr(nullptr)
	, m_bCaptureProcessed(false)
	, m_bCaptureFlag(false)

#if (_DEBUG && DBG_OUT_SCREENFRAMELENGTH)
	, m_nTickCounter(0)
	, dbgFile(nullptr)
#endif // _DEBUG

	  // мышь марсианка
	, m_nPointX(0)
	, m_nPointY(0)
	, m_MouseValue(0)
	, m_bMouseMove(false)
	, m_bMouseOutEna(false)
	, m_nMouseEnaStrobe(0)
{
	InitVars(nRenderType);
}

#ifdef _WIN64
#define BK_OGLDLLNAME _T("Dll\\BKScreenOGL_x64.dll")
#define BK_D2DDLLNAME _T("Dll\\BKScreenD2D_x64.dll")
#define BK_DIBDLLNAME _T("Dll\\BKScreenDIB_x64.dll")
#define BK_D3DDLLNAME _T("Dll\\BKScreenD3D_x64.dll")
#else
#define BK_OGLDLLNAME _T("Dll\\BKScreenOGL.dll")
#define BK_D2DDLLNAME _T("Dll\\BKScreenD2D.dll")
#define BK_DIBDLLNAME _T("Dll\\BKScreenDIB.dll")
#define BK_D3DDLLNAME _T("Dll\\BKScreenD3D.dll")
#endif

#define BK_SCRDLLFUNC "GetBKScreen"

void CScreen::InitVars(CONF_SCREEN_RENDER nRenderType)
{
	m_nViewWidth = (1024);
	m_dAspectRatio = (4.0 / 3.0);
	m_nViewHeight = (static_cast<int>(m_nViewWidth / m_dAspectRatio));

	switch (nRenderType)// Берём номер из конфига. 0 - OGL, 1 - D2D, 2 - DIB, 3 - D3D (глючный для WinXP)
	{
		case CONF_SCREEN_RENDER::OPENGL:
			m_strDllName = BK_OGLDLLNAME;
			break;

		case CONF_SCREEN_RENDER::D2D:
			m_strDllName = BK_D2DDLLNAME;
			break;

		default:
		case CONF_SCREEN_RENDER::VFW:
			m_strDllName = BK_DIBDLLNAME;
			break;

		case CONF_SCREEN_RENDER::D3D:
			m_strDllName = BK_D3DDLLNAME;
			break;
	}

	m_hModule = LoadLibrary(m_strDllName);

	if (m_hModule)
	{
		auto pGetBKScr = reinterpret_cast<GETBKSCR>(GetProcAddress(m_hModule, BK_SCRDLLFUNC));

		if (pGetBKScr)
		{
			switch (nRenderType)// Берём номер из конфига. 0 - OGL, 1 - D2D, 2 - DIB, 3 - D3D (глючный для WinXP)
			{
				case CONF_SCREEN_RENDER::OPENGL:
					m_pscrSharedFunc = dynamic_cast<CScreenOGL *>(pGetBKScr());
					break;

				case CONF_SCREEN_RENDER::D2D:
					m_pscrSharedFunc = dynamic_cast<CScreenD2D *>(pGetBKScr());
					break;

				default:
				case CONF_SCREEN_RENDER::VFW:
					m_pscrSharedFunc = dynamic_cast<CScreenDIB *>(pGetBKScr());
					break;

				case CONF_SCREEN_RENDER::D3D:
					m_pscrSharedFunc = dynamic_cast<CScreenD3D *>(pGetBKScr());
					break;
			}
		}
		else
		{
			CString str;
			str.Format(IDS_BK_ERROR_SCRDLLFUNCERR, _T(BK_SCRDLLFUNC));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		CString str;
		str.Format(IDS_BK_ERROR_SCRDLLINITERR, m_strDllName);
		g_BKMsgBox.Show(str, MB_OK);
	}

	if (m_pscrSharedFunc)
	{
		m_bReverseScreen = m_pscrSharedFunc->BKSS_GetReverseFlag();
	}
}

CScreen::~CScreen()
{
	if (m_bCaptureProcessed)
	{
		CancelCapture();
	}

	ClearObjects();
}


BEGIN_MESSAGE_MAP(CScreen, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()


void CScreen::OnDestroy()
{
	KillTimer(BKTIMER_SCREEN_FPS);
	KillTimer(BKTIMER_MOUSE);

	if (m_bCaptureProcessed)
	{
		CancelCapture();
	}

	// перед выходом дождёмся прекращения выполнения рисования
	while (m_lockBusy.IsLocked())
	{
		SleepEx(0, TRUE);
	}

	// и заблокируем его во избежание потенциальных дедлоков
	m_lockBusy.Lock();

	if (m_pscrSharedFunc)
	{
		m_pscrSharedFunc->BKSS_ScreenView_Done();
	}

	m_lockBusy.UnLock();
	CWnd::OnDestroy();
}

void CScreen::ClearObjects()
{
	SAFE_DELETE(m_pscrSharedFunc);
	SAFE_DELETE_ARRAY(m_BKScreen.pTexture);
	SAFE_DELETE_ARRAY(m_AZScreen.pTexture);

	if (m_hModule)
	{
		FreeLibrary(m_hModule);
		m_hModule = nullptr;
	}
}

// обработчики сообщений CScreen
BOOL CScreen::PreCreateWindow(CREATESTRUCT &cs)
{
	m_pwndParent = FromHandle(cs.hwndParent);
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_SAVEBITS,
	                                   ::LoadCursor(nullptr, IDC_ARROW), HBRUSH(::GetStockObject(BLACK_BRUSH)), nullptr);
	return CWnd::PreCreateWindow(cs);
}

int CScreen::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	if (m_pscrSharedFunc == nullptr)
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_SCRDLLFUNCPTRERR, MB_OK);
		return -1;
	}

	if (!InitColorTables())
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_SCRCOLORTABLEERR, MB_OK);
		return -1;
	}

	SetTimer(BKTIMER_SCREEN_FPS, 1000, nullptr);
	SetTimer(BKTIMER_MOUSE, 40, nullptr);
#if (_DEBUG && DBG_OUT_SCREENFRAMELENGTH)
	m_nTickCounter = GetTickCount();
	dbgFile = fopen("dbgScreen.txt", "wt");
#endif // _DEBUG
	// заполняем параметры экрана.
	// 1. рассчитаем размеры окна полноэкранного режима
	m_BKScreen.rcFSDim.left = 0;
	m_BKScreen.rcFSDim.top =  0;
	HDC dc = ::GetDC(nullptr);
	const int dx = m_BKScreen.rcFSDim.right = ::GetDeviceCaps(dc, HORZRES);   // ширина экрана
	const int dy = m_BKScreen.rcFSDim.bottom = ::GetDeviceCaps(dc, VERTRES);  // высота экрана
	::ReleaseDC(nullptr, dc);
	int wx = static_cast<int>(static_cast<double>(dy) * m_dAspectRatio);  // ширина экрана при высоте dy в пропорциях 4/3
	int wy = static_cast<int>(static_cast<double>(dx) / m_dAspectRatio);  // высота экрана при ширине dx в пропорциях 4/3

	// 2. рассчитаем размеры рисуемой картинки в полноэкранном режиме
	if (dx <= dy) // если монитор повёрнут на 90 градусов, или нестандартный - квадратный
	{
		if (dx < wx) // если не влазит по ширине, вписываем в ширину
		{
			wy = dy;
			wx = static_cast<int>(static_cast<double>(wy) * m_dAspectRatio);
		}
		else
		{
			// то вписываем картинку в высоту
			wx = dx;
			wy = static_cast<int>(static_cast<double>(wx) / m_dAspectRatio);
		}
	}
	else    // если монитор в обычном положении
	{
		if (dx < wx) // если не влазит по ширине, вписываем в ширину
		{
			wx = dx;
			wy = static_cast<int>(static_cast<double>(wx) / m_dAspectRatio);
		}
		else
		{
			// то вписываем картинку в высоту
			wy = dy;
			wx = static_cast<int>(static_cast<double>(wy) * m_dAspectRatio);
		}
	}

	m_BKScreen.rcFSViewPort.left = (dx - wx) / 2;   // выравниваем по центру
	m_BKScreen.rcFSViewPort.top = (dy - wy) / 2;
	m_BKScreen.rcFSViewPort.right = wx;
	m_BKScreen.rcFSViewPort.bottom = wy;
	// 3. создаём и инициализируем экраны
	m_BKScreen.createTexture(BK_SCREEN_WIDTH, BK_SCREEN_HEIGHT);
	// теперь - экран для AZBK
	m_AZScreen.rcFSDim = m_BKScreen.rcFSDim;
	m_AZScreen.rcFSViewPort = m_BKScreen.rcFSViewPort;
	m_AZScreen.createTexture(AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT);
	m_pCurrentScreen = &m_BKScreen; //текущим назначаем экран БК

	if (SUCCEEDED(m_pscrSharedFunc->BKSS_ScreenView_Init(m_pCurrentScreen, this)))
	{
		return 1;
	}

	CString str;
	str.Format(IDS_BK_ERROR_SCRDLLINITERR, m_strDllName);
	g_BKMsgBox.Show(str, MB_OK);
	m_pscrSharedFunc->BKSS_ScreenView_Done();
	ClearObjects();
	return -1;
}

void CScreen::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	m_pscrSharedFunc->BKSS_OnSize(cx, cy);
}


BOOL CScreen::OnEraseBkgnd(CDC *pDC)
{
	return TRUE;
}

BOOL CScreen::PreTranslateMessage(MSG *pMsg)
{
	switch (pMsg->message)
	{
		// транслируем сообщения клавиатуры в CBKView.
		// это крайне необходимо для рендера D3D11, без этого в полноэкранном режиме никак
		// клавиатуру не обработать.
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			TRACE("CScreen translate kbd message to CBKView\n");
			m_pwndParent->PostMessage(pMsg->message, pMsg->wParam, pMsg->lParam);
			return TRUE;
	}

	return CWnd::PreTranslateMessage(pMsg);
}

void CScreen::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
		case BKTIMER_SCREEN_FPS:
			m_mutFPS.lock();
			m_nCurFPS = m_nFrame;
			m_nFrame = 0;
			m_mutFPS.unlock();
			break;

		case BKTIMER_MOUSE:
			if (m_bMouseMove)
			{
				m_MouseValue &= ~017;
				m_bMouseMove = false;
			}

			break;
	}
}

void CScreen::DrawScreen(const bool bCheckFPS, const int screen) const
{
	if (screen != m_nCurrentScreen)
	{
		return;
	}

	if (m_pCurrentScreen->pTexture == nullptr || m_lockChangeMode.IsLocked() || m_lockBusy.IsLocked())
	{
		return;
	}

	m_lockBusy.Lock();
	m_pscrSharedFunc->BKSS_DrawScreen();
	m_lockBusy.UnLock();
#if (_DEBUG && DBG_OUT_SCREENFRAMELENGTH)
	DWORD nTmpTick = GetTickCount();
	DWORD nFrame = nTmpTick - m_nTickCounter;
	m_nTickCounter = nTmpTick;
	fprintf(dbgFile, "%d ms\n", nFrame);
#endif // _DEBUG

	if (m_bCaptureFlag)
	{
		WriteToPipe();
	}

	if (bCheckFPS)
	{
		m_mutFPS.lock();
		// Посчитаем FPS
		m_nFrame++;
		m_mutFPS.unlock();
	}
}


void CScreen::RestoreFS()
{
	m_pscrSharedFunc->BKSS_RestoreFullScreen();
}

bool CScreen::SetFullScreenMode()
{
	while (m_lockBusy.IsLocked() || m_lockPrepare.IsLocked())
	{
		SleepEx(0, TRUE);    // если выполняется отрисовка, то подождём, пока процедуры не завершатся
	}

	m_lockChangeMode.Lock();
	bool bRet = m_pscrSharedFunc->BKSS_SetFullScreenMode();
	m_lockChangeMode.UnLock();
	return bRet;
}

bool CScreen::SetWindowMode()
{
	while (m_lockBusy.IsLocked() || m_lockPrepare.IsLocked())
	{
		SleepEx(0, TRUE);    // если выполняется отрисовка, то подождём, пока процедуры не завершатся
	}

	m_lockChangeMode.Lock();
	bool bRet = m_pscrSharedFunc->BKSS_SetWindowMode();
	m_lockChangeMode.UnLock();
	int nPixelW = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSX);
	int nPixelH = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSY);
	AdjustLayout(nPixelW, nPixelH);
	return bRet;
}

bool CScreen::IsFullScreenMode() const
{
	return m_pscrSharedFunc->BKSS_IsFullScreenMode();
}


HBITMAP CScreen::GetScreenshot() const
{
	if (m_bReverseScreen)
	{
		// для DIBDraw надо перевернуть экран, тупо не знаю, как это сделать с помощью готовых средств.
		// поэтому - вручную.
		auto pNewBits = std::make_unique<uint32_t[]>(static_cast<size_t>(m_pCurrentScreen->nWidth) * m_pCurrentScreen->nHeight);
		uint32_t *pBits = m_pCurrentScreen->pTexture + (m_pCurrentScreen->nWidth * m_pCurrentScreen->nHeight);
		uint32_t *pBitd = pNewBits.get();

		for (int y = m_pCurrentScreen->nHeight; y > 0; y--)
		{
			pBits -= m_pCurrentScreen->nWidth;
			memcpy(pBitd, pBits, m_pCurrentScreen->nWidth * sizeof(uint32_t));
			pBitd += m_pCurrentScreen->nWidth;
		}

		HBITMAP hBm = CreateBitmap(m_pCurrentScreen->nWidth, m_pCurrentScreen->nHeight, 1, 32, pNewBits.get());
		return hBm;
	}

	HBITMAP hBm = CreateBitmap(m_pCurrentScreen->nWidth, m_pCurrentScreen->nHeight, 1, 32, m_pCurrentScreen->pTexture);
	return hBm;
}

void CScreen::SetSmoothing(bool bSmoothing)
{
	m_bSmoothing = bSmoothing;
	m_pscrSharedFunc->BKSS_SetSmoothing(m_bSmoothing);
}

void CScreen::SetColorMode(bool bColorMode)
{
	m_bColorMode = bColorMode;
	m_pscrSharedFunc->BKSS_SetColorMode();
}

constexpr auto nPitch = 8 * sizeof(uint32_t);

// этот алгоритм применяется только в режиме отладки, когда нужно принудительно перерисовывать весь экран
// и в карте памяти
void CScreen::PrepareScreenRGB32(uint8_t *ScreenBuffer) const
{
//  TRACE0("*** DEBUG PREPARE ***\n");
	if (m_BKScreen.pTexture == nullptr || m_lockChangeMode.IsLocked())
	{
		return;
	}

	m_lockPrepare.Lock();
	const uint8_t scroll = (m_nOfs - 0330) & 0377;

	// Render to bitmap
	const uint32_t *pCurPalette = m_bColorMode ? m_pColTable32.get() : m_pMonoTable32.get();
	const int linesToShow = m_bExtended ? m_BKScreen.nHeight / 4 : m_BKScreen.nHeight;

	for (int y = 0; y < linesToShow; ++y)
	{
		// указатель на начало строки
		uint32_t *pBits = m_BKScreen.pTexture + (static_cast<size_t>(m_BKScreen.nWidth) * (m_bReverseScreen ? (255 - static_cast<size_t>(y)) : static_cast<size_t>(y)));
		uint8_t *pVideo = ScreenBuffer + (static_cast<size_t>((y + scroll) & 0377) * 64);

		for (int x = m_BKScreen.nWidth / 8; x > 0; x--) // рисуем побайтно
		{
			const uint32_t *pPalette = pCurPalette + ((m_nPaletteNum_m256 + *pVideo++) << 3);
			// один байт - это 8 бит
			memcpy(pBits, pPalette, nPitch);
			pBits += 8;
		}
	}

	if (m_bExtended)
	{
		if (m_bReverseScreen)
		{
			// Для DrawDIB обнулять нужно область с начала массива экрана до 3/4 массива экрана
			memset(m_BKScreen.pTexture, Color_Black, (static_cast<size_t>(m_BKScreen.nHeight) - m_BKScreen.nHeight / 4) * m_BKScreen.nWidth * sizeof(uint32_t));
		}
		else
		{
			// Для всех обнулять нужно область с 1/4 массива экрана до конца массива
			memset(m_BKScreen.pTexture + (m_BKScreen.nWidth * m_BKScreen.nHeight / 4), Color_Black, (static_cast<size_t>(m_BKScreen.nHeight) - m_BKScreen.nHeight / 4) * m_BKScreen.nWidth * sizeof(uint32_t));
		}
	}

	m_lockPrepare.UnLock();
}

// нельзя использовать constexpr, т.к. нужен именно макрос,
// т.к. нужно именно целочисленное умножение и деление в выражении
// и скобки нельзя
#define LUMINOFORE_COEFF 3 / 8
// #define  LUMINOFORE_COEFF 2 / 3

// Это старая функция, сейчас не используется.
// Но если что-то надо будет изменить, тренироваться будем на ней, а потом копипастить в PrepareScreenLineWordRGB32
// т.к. та функция - просто дважды повторённая эта.
// на входе у обеих функций:
// nLineNum - номер строки 0..255
// nByteNum - смещение байта/слова в строке относительно начала строки.
void CScreen::PrepareScreenLineByteRGB32(int nLineNum, int nByteNum, uint8_t b) const
{
	if (m_BKScreen.pTexture == nullptr || m_lockChangeMode.IsLocked())
	{
		return;
	}

	m_lockPrepare.Lock();
	const uint8_t nScreenLine = nLineNum & 0377;
	const uint8_t yy = m_bReverseScreen ? 255 - nScreenLine : nScreenLine;

	uint32_t *pBits = m_BKScreen.pTexture + (static_cast<size_t>(yy) * m_BKScreen.nWidth + static_cast<size_t>(nByteNum) * 8);
	const uint32_t *pCurPalette = m_bColorMode ? m_pColTable32.get() : m_pMonoTable32.get();

	if (m_bExtended && nScreenLine >= (m_BKScreen.nHeight / 4))
	{
		// обнуляем
		if (m_bLuminoforeEmul)
		{
			for (int i = 8; i > 0; i--)
			{
				auto c = reinterpret_cast<BCOLOR *>(pBits);
				// blue - 0, green - 1, red - 2, alpha - 3;
				c->bb[0] = LOBYTE(static_cast<uint16_t>(c->bb[0]) * LUMINOFORE_COEFF);
				c->bb[1] = LOBYTE(static_cast<uint16_t>(c->bb[1]) * LUMINOFORE_COEFF);
				c->bb[2] = LOBYTE(static_cast<uint16_t>(c->bb[2]) * LUMINOFORE_COEFF);
				pBits++;
			}
		}
		else
		{
			memset(pBits, 0, nPitch);
		}
	}
	else
	{
		// копируем 1 байт
		const uint32_t *pPalette = pCurPalette + ((m_nPaletteNum_m256 + b) << 3);

		if (m_bLuminoforeEmul)
		{
			for (int i = 8; i > 0; i--)
			{
				// эмуляция затухания люминофора. Как-то не очень хорошо.
				// не удаётся подобрать вариант, чтобы и шлейф не мешал
				// и мерцания не было.
				// наверное люминофор гаснет нелинейно.
				auto c = reinterpret_cast<BCOLOR *>(pBits);
				const auto p = reinterpret_cast<BCOLOR *>(const_cast<uint32_t *>(pPalette));
				// blue - 0, green - 1, red - 2, alpha - 3;
				c->bb[0] = p->bb[0] ? p->bb[0] : LOBYTE(static_cast<uint16_t>(c->bb[0]) * LUMINOFORE_COEFF);
				c->bb[1] = p->bb[1] ? p->bb[1] : LOBYTE(static_cast<uint16_t>(c->bb[1]) * LUMINOFORE_COEFF);
				c->bb[2] = p->bb[2] ? p->bb[2] : LOBYTE(static_cast<uint16_t>(c->bb[2]) * LUMINOFORE_COEFF);
				//c->bb[3] = p->bb[3];
				pBits++;
				pPalette++;
			}
		}
		else
		{
			memcpy(pBits, pPalette, nPitch);
		}
	}

	m_lockPrepare.UnLock();
}

void CScreen::PrepareScreenLineWordRGB32(int nLineNum, int nByteNum, uint16_t w) const
{
	// !много копипасты. если что-то менять, не забыть поменять в трёх местах
	if (m_BKScreen.pTexture == nullptr || m_lockChangeMode.IsLocked())
	{
		return;
	}

	m_lockPrepare.Lock();
	// первый байт
	uint8_t b = LOBYTE(w);
	const uint8_t nScreenLine = nLineNum & 0377;
	const uint8_t yy = m_bReverseScreen ? 255 - nScreenLine : nScreenLine;
	uint32_t *pBits_A = m_BKScreen.pTexture + (static_cast<size_t>(yy) * m_BKScreen.nWidth + static_cast<size_t>(nByteNum) * 8);
	uint32_t *pBits = pBits_A;
	const uint32_t *pCurPalette = m_bColorMode ? m_pColTable32.get() : m_pMonoTable32.get();

	if (m_bExtended && nScreenLine >= (m_BKScreen.nHeight / 4))
	{
		// обнуляем
		if (m_bLuminoforeEmul)
		{
			for (int i = 8; i > 0; i--)
			{
				auto c = reinterpret_cast<BCOLOR *>(pBits);
				// blue - 0, green - 1, red - 2, alpha - 3;
				c->bb[0] = LOBYTE(static_cast<uint16_t>(c->bb[0]) * LUMINOFORE_COEFF);
				c->bb[1] = LOBYTE(static_cast<uint16_t>(c->bb[1]) * LUMINOFORE_COEFF);
				c->bb[2] = LOBYTE(static_cast<uint16_t>(c->bb[2]) * LUMINOFORE_COEFF);
				pBits++;
			}
		}
		else
		{
			memset(pBits, 0, nPitch);
		}
	}
	else
	{
		// копируем 1 байт
		const uint32_t *pPalette = pCurPalette + ((m_nPaletteNum_m256 + b) << 3);

		if (m_bLuminoforeEmul)
		{
			for (int i = 8; i > 0; i--)
			{
				// эмуляция затухания люминофора. Как-то не очень хорошо.
				// не удаётся подобрать вариант, чтобы и шлейф не мешал
				// и мерцания не было.
				// наверное люминофор гаснет нелинейно.
				auto c = reinterpret_cast<BCOLOR *>(pBits);
				const auto p = reinterpret_cast<BCOLOR *>(const_cast<uint32_t*>(pPalette));
				// blue - 0, green - 1, red - 2, alpha - 3;
				c->bb[0] = p->bb[0] ? p->bb[0] : LOBYTE(static_cast<uint16_t>(c->bb[0]) * LUMINOFORE_COEFF);
				c->bb[1] = p->bb[1] ? p->bb[1] : LOBYTE(static_cast<uint16_t>(c->bb[1]) * LUMINOFORE_COEFF);
				c->bb[2] = p->bb[2] ? p->bb[2] : LOBYTE(static_cast<uint16_t>(c->bb[2]) * LUMINOFORE_COEFF);
				//c->bb[3] =  p->bb[3];
				pBits++;
				pPalette++;
			}
		}
		else
		{
			memcpy(pBits, pPalette, nPitch);
		}
	}

	// копипаста второй байт
	b = HIBYTE(w);
	pBits = pBits_A + 8;

	if (m_bExtended && nScreenLine >= (m_BKScreen.nHeight / 4))
	{
		// обнуляем
		if (m_bLuminoforeEmul)
		{
			for (int i = 8; i > 0; i--)
			{
				auto c = reinterpret_cast<BCOLOR *>(pBits);
				// blue - 0, green - 1, red - 2, alpha - 3;
				c->bb[0] = LOBYTE(static_cast<uint16_t>(c->bb[0]) * LUMINOFORE_COEFF);
				c->bb[1] = LOBYTE(static_cast<uint16_t>(c->bb[1]) * LUMINOFORE_COEFF);
				c->bb[2] = LOBYTE(static_cast<uint16_t>(c->bb[2]) * LUMINOFORE_COEFF);
				pBits++;
			}
		}
		else
		{
			memset(pBits, 0, nPitch);
		}
	}
	else
	{
		// копируем 1 байт
		const uint32_t *pPalette = pCurPalette + ((m_nPaletteNum_m256 + b) << 3);

		if (m_bLuminoforeEmul)
		{
			for (int i = 8; i > 0; i--)
			{
				// эмуляция затухания люминофора. Как-то не очень хорошо.
				// не удаётся подобрать вариант, чтобы и шлейф не мешал
				// и мерцания не было.
				// наверное люминофор гаснет нелинейно.
				auto c = reinterpret_cast<BCOLOR *>(pBits);
				const auto p = reinterpret_cast<BCOLOR *>(const_cast<uint32_t *>(pPalette));
				// blue - 0, green - 1, red - 2, alpha - 3;
				c->bb[0] = p->bb[0] ? p->bb[0] : LOBYTE(static_cast<uint16_t>(c->bb[0]) * LUMINOFORE_COEFF);
				c->bb[1] = p->bb[1] ? p->bb[1] : LOBYTE(static_cast<uint16_t>(c->bb[1]) * LUMINOFORE_COEFF);
				c->bb[2] = p->bb[2] ? p->bb[2] : LOBYTE(static_cast<uint16_t>(c->bb[2]) * LUMINOFORE_COEFF);
				//c->bb[3] = p->bb[3];
				pBits++;
				pPalette++;
			}
		}
		else
		{
			memcpy(pBits, pPalette, nPitch);
		}
	}

#ifdef _DEBUG
	/////////////////////////////////////////////////////////////////////////////////
	// Отладочный видимый ход луча
	// Нужно только при отладке экрана, включать только при необходимости.
	// в релизной версии принудительно отключим.
	// PrepareScreenLineByte_Debug(nLineNum, nByteNum);
	/////////////////////////////////////////////////////////////////////////////////
#endif
	m_lockPrepare.UnLock();
}

#ifdef _DEBUG
// отладочная трассировка луча.
// ставится красная точка на следующем месте, где будет луч.
void CScreen::PrepareScreenLineByte_Debug(int nLineNum, int nByteNum) const
{
	nByteNum += 2;

	if (nByteNum >= 0100)
	{
		nByteNum = 0;
		nLineNum++;
		nLineNum &= 0377;
	}

	const uint8_t nScreenLine = nLineNum & 0377;
	const uint8_t yy = m_bReverseScreen ? 255 - nScreenLine : nScreenLine;

	uint32_t *pBits = m_BKScreen.pTexture + (static_cast<size_t>(yy) * m_BKScreen.nWidth + static_cast<size_t>(nByteNum) * 8);
	// копируем 1 байт (ставим красную точку)
	// красную не всегда видно, может что-то ещё надо будет придумать. может ксорить.
	const uint32_t *pPalette = m_pColTable32.get() + (3 * 8);

	for (int i = 2; i > 0; i--)
	{
		*pBits++ = *pPalette++;
	}
}
#endif


constexpr auto COLTABLE_SIZE = (16 * 256 * 8);
// 16 палитр, 256 вариантов значений байта, на каждый вариант - 8 uint32_tов,

bool CScreen::InitColorTables()
{
	bool bRet = false;

	while (m_lockPrepare.IsLocked())   // если выполняется подготовка, то подождём, пока процедуры не завершатся
	{
		SleepEx(0, TRUE);
	}

	m_lockChangeMode.Lock();
	// Create color tables of all display modes for faster drawing

	if (m_pColTable32)
	{
		m_pColTable32.reset();
	}

	if (m_pMonoTable32)
	{
		m_pMonoTable32.reset();
	}

	m_pColTable32  = std::make_unique<uint32_t[]>(COLTABLE_SIZE);
	m_pMonoTable32 = std::make_unique<uint32_t[]>(COLTABLE_SIZE);

	if ((m_pColTable32 != nullptr) && (m_pMonoTable32 != nullptr))
	{
		for (size_t p = 0; p < 16; ++p)   // для каждой из 16 палитр
		{
			// вот это делает одну палитру. а нам надо таких 16.
			for (size_t i = 0; i < 256; ++i)   // для каждого значения байта (от 0 до 255) делаем маску, чтоб работать сразу с байтами и не заморачиваться с битами
			{
				uint32_t *pColBuff32 =  &m_pColTable32[(p * 256 + i) * 8];
				uint32_t *pMonBuff32 = &m_pMonoTable32[(p * 256 + i) * 8];

				for (int n = 0; n < 4; ++n)   // в каждом байте 4 двухбития, на каждые два бита, у нас отводится по 4 дворда
				{
					int n2 = n * 2;

					switch ((i >> n2) & 3)
					{
						// Black
						case 0:
							pColBuff32[n2] = g_pColorPalettes[p][0];
							pMonBuff32[n2++] = (m_bAdapt) ? g_pAdaptMonochromePalette[0][0] : g_pMonochromePalette[0][0];
							pColBuff32[n2] = g_pColorPalettes[p][0];
							pMonBuff32[n2] = (m_bAdapt) ? g_pAdaptMonochromePalette[1][0] : g_pMonochromePalette[1][0];
							break;

						// Blue and Gray
						case 1:
							pColBuff32[n2] = g_pColorPalettes[p][1];
							pMonBuff32[n2++] = (m_bAdapt) ? g_pAdaptMonochromePalette[0][1] : g_pMonochromePalette[0][1];
							pColBuff32[n2] = g_pColorPalettes[p][1];
							pMonBuff32[n2] = (m_bAdapt) ? g_pAdaptMonochromePalette[1][1] : g_pMonochromePalette[1][1];
							break;

						// Green and Gray
						case 2:
							pColBuff32[n2] = g_pColorPalettes[p][2];
							pMonBuff32[n2++] = (m_bAdapt) ? g_pAdaptMonochromePalette[0][2] : g_pMonochromePalette[0][2];
							pColBuff32[n2] = g_pColorPalettes[p][2];
							pMonBuff32[n2] = (m_bAdapt) ? g_pAdaptMonochromePalette[1][2] : g_pMonochromePalette[1][2];
							break;

						// Red
						case 3:
							pColBuff32[n2] = g_pColorPalettes[p][3];
							pMonBuff32[n2++] = (m_bAdapt) ? g_pAdaptMonochromePalette[0][3] : g_pMonochromePalette[0][3];
							pColBuff32[n2] = g_pColorPalettes[p][3];
							pMonBuff32[n2] = (m_bAdapt) ? g_pAdaptMonochromePalette[1][3] : g_pMonochromePalette[1][3];
							break;
					}
				}
			} // end for(i)
		} // end for(p)

		bRet = true;
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}

	m_lockChangeMode.UnLock();
	return bRet;
}

void CScreen::ChangeScreen(int nScr)
{
	while (m_lockBusy.IsLocked() || m_lockPrepare.IsLocked())
	{
		SleepEx(0, TRUE);    // если выполняется отрисовка, то подождём, пока процедуры не завершатся
	}

	m_lockPrepare.Lock();

	switch (nScr)
	{
		default:
		case 0:
			m_pCurrentScreen = &m_BKScreen; //текущим назначаем экран БК
			m_nCurrentScreen = 0;
			break;

		case 1:
			m_pCurrentScreen = &m_AZScreen; //текущим назначаем экран AZBK
			m_nCurrentScreen = 1;
			break;
	}

	m_pscrSharedFunc->BKSS_ScreenView_ReInit(m_pCurrentScreen);
	m_lockPrepare.UnLock();
}

void CScreen::AdjustLayout(int nPW, int nPH)
{
	if (m_pwndParent == nullptr)
	{
		return;
	}

	while (m_lockChangeMode.IsLocked() || m_lockBusy.IsLocked())
	{
		SleepEx(0, TRUE);    // если выполняется отрисовка, то подождём, пока процедуры не завершатся
	}

	m_lockChangeMode.Lock();
	int nDispScrW = ::MulDiv(m_nViewWidth, nPW, DEFAULT_DPIX);
	int nDispScrH = ::MulDiv(m_nViewHeight, nPH, DEFAULT_DPIY);
	CRect rcScreen(0, 0, nDispScrW, nDispScrH);
	CRect rcNewScreen = rcScreen;
	CRect rcClient;
	m_pwndParent->GetClientRect(&rcClient);

	if (rcScreen.Width() * rcClient.Height() > rcClient.Width() * rcScreen.Height())
	{
		rcNewScreen.right = rcClient.Width();
		rcNewScreen.bottom = rcScreen.bottom * rcClient.Width() / rcScreen.right;
	}
	else
	{
		rcNewScreen.bottom = rcClient.Height();
		rcNewScreen.right = rcScreen.right * rcClient.Height() / rcScreen.bottom;
	}

	if (rcNewScreen.Width() > nDispScrW)
	{
		rcNewScreen.right = nDispScrW;
	}

	if (rcNewScreen.Height() > nDispScrH)
	{
		rcNewScreen.bottom = nDispScrH;
	}

	int x_offs = (rcClient.Width() - rcNewScreen.Width()) / 2;
	int y_offs = (rcClient.Height() - rcNewScreen.Height()) / 2;
	rcClient.right = rcClient.left + rcNewScreen.Width();
	rcClient.bottom = rcClient.top + rcNewScreen.Height();
	rcClient.OffsetRect(x_offs, y_offs);
	SetWindowPos(nullptr, rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height(), SWP_NOZORDER);
	m_lockChangeMode.UnLock();
}

void CScreen::SetCaptureStatus(bool bCapture, const CString &strUniq)
{
	if (bCapture)
	{
		PrepareCapture(strUniq);
	}
	else
	{
		CancelCapture();
	}
}

void CScreen::PrepareCapture(const CString &strUniq)
{
	if (m_bCaptureProcessed)
	{
		CancelCapture();
	}

	SECURITY_ATTRIBUTES saAttr{};
	// Set the bInheritHandle flag so pipe handles are inherited.
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = nullptr;

	// Create a pipe for the child process's STDIN.
	if (!CreatePipe(&m_hChildStd_IN_Rd, &m_hChildStd_IN_Wr, &saAttr, 0 /*m_BitBufferSize*/))
	{
		TRACE("Stdin CreatePipe failed\n");
		return;
	}

	// Ensure the write handle to the pipe for STDIN is not inherited.
	if (!SetHandleInformation(m_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
	{
		TRACE("Stdin Wr SetHandleInformation failed\n");
		return;
	}

	// Create a child process that uses the previously created pipes for STDIN and STDOUT.
	CString strName = _T("capture_") + strUniq + _T(".mp4");
	fs::path strPathName = g_Config.m_strScreenShotsPath / strName.GetString();
	CString szCmdline;
	szCmdline.Format(g_Config.m_strFFMPEGLine, m_pCurrentScreen->nWidth, m_pCurrentScreen->nHeight);
	szCmdline += _T(" \"") + CString(strPathName.c_str()) + _T("\"");
	// Set up members of the PROCESS_INFORMATION structure.
	PROCESS_INFORMATION piProcInfo;
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
	// Set up members of the STARTUPINFO structure.
	// This structure specifies the STDIN and STDOUT handles for redirection.
	STARTUPINFO siStartInfo;
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.dwFlags = STARTF_USESTDHANDLES;
	siStartInfo.hStdError = nullptr; // GetStdHandle(STD_ERROR_HANDLE);
	siStartInfo.hStdOutput = nullptr; // GetStdHandle(STD_OUTPUT_HANDLE);
	siStartInfo.hStdInput = m_hChildStd_IN_Rd;
	// Create the child process.
	BOOL bSuccess = CreateProcess(nullptr,
	                              szCmdline.GetBuffer(),     // command line
	                              nullptr,            // process security attributes
	                              nullptr,            // primary thread security attributes
	                              TRUE,               // handles are inherited
	                              CREATE_NEW_CONSOLE, /*CREATE_NO_WINDOW,*/   // creation flags
	                              nullptr,            // use parent's environment
	                              nullptr,            // use parent's current directory
	                              &siStartInfo,       // STARTUPINFO pointer
	                              &piProcInfo);     // receives PROCESS_INFORMATION

	// If an error occurs, exit the application.
	if (!bSuccess)
	{
		TRACE("CreateProcess failed\n");
		return;
	}

	WaitForInputIdle(piProcInfo.hProcess, 1000);
	// Close handles to the child process and its primary thread.
	// Some applications might keep these handles to monitor the status
	// of the child process, for example.
	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);
	m_bCaptureProcessed = true;
	m_bCaptureFlag = true;
}

void CScreen::CancelCapture()
{
	if (m_bCaptureProcessed)
	{
		std::lock_guard<std::mutex> locker(m_mutCapture);
		// Close the pipe handle so the child process stops reading.
		CloseHandle(m_hChildStd_IN_Wr);
		CloseHandle(m_hChildStd_IN_Rd);
		m_bCaptureProcessed = false;
		m_bCaptureFlag = false;
	}
}

// Read from a file and write its contents to the pipe for the child's STDIN.
void CScreen::WriteToPipe() const
{
	DWORD dwWritten;
	std::lock_guard<std::mutex> locker(m_mutCapture);

	if (m_bCaptureProcessed)
	{
		BOOL bSuccess = WriteFile(m_hChildStd_IN_Wr, m_pCurrentScreen->pTexture, m_pCurrentScreen->nTextureSize, &dwWritten, nullptr);
	}
}


// мышь марсианка


uint16_t CScreen::GetMouseStatus()
{
	if (m_bMouseOutEna)
	{
		m_bMouseOutEna = false;
		return m_MouseValue;
	}

	return 0;
}

void CScreen::SetMouseStrobe(uint16_t data)
{
	if (!m_nMouseEnaStrobe && (data & 010))
	{
		m_bMouseOutEna = true;
	}
	else
	{
		m_bMouseOutEna = false;
	}

	m_nMouseEnaStrobe = (data & 010);
}

void CScreen::OnMouseMove(UINT nFlags, CPoint point)
{
	// определим направление движения
	int dx = m_nPointX - point.x;
	int dy = m_nPointY - point.y;
	m_nPointX = point.x;
	m_nPointY = point.y;
	m_MouseValue &= ~017;

	if (dx > 0)         // направление влево
	{
		m_MouseValue |= 010;   // LEFT
	}
	else if (dx < 0)    // направление вправо
	{
		m_MouseValue |= 002;    // RIGHT
	}

	// если == 0, то никуда не движемся

	if (dy > 0)         // направление вверх
	{
		m_MouseValue |= 001;   // UP
	}
	else if (dy < 0)    // направление вниз
	{
		m_MouseValue |= 004;   // DOWN
	}

	// если == 0, то никуда не движемся
	m_bMouseMove = !!(m_MouseValue & 017);
	CWnd::OnMouseMove(nFlags, point);
}


void CScreen::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_MouseValue |= 040;
	CWnd::OnLButtonDown(nFlags, point);
}


void CScreen::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_MouseValue &= ~040;
	CWnd::OnLButtonUp(nFlags, point);
}


void CScreen::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_MouseValue |= 0100;
	CWnd::OnRButtonDown(nFlags, point);
}


void CScreen::OnRButtonUp(UINT nFlags, CPoint point)
{
	m_MouseValue &= ~0100;
	CWnd::OnRButtonUp(nFlags, point);
}
#else
#endif