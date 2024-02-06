// BKScreenDIB.cpp: определяет экспортированные функции для приложения DLL.
//

#include "pch.h"
#include "BKScreenDIB.h"

#ifdef DIB
#pragma comment (lib,"Vfw32.lib")

BKEXTERN_C
{
	BKSCRDLL_API BKSCREENHANDLE APIENTRY GetBKScreen()
	{
		return new CScreenDIB;
	}

	CScreenDIB::CScreenDIB()
		: m_hdd(nullptr)
		, m_hbmp(nullptr)
		, m_cx(0)
		, m_cy(0)
	{
	}

	CScreenDIB::~CScreenDIB()
	= default;

	void CScreenDIB::BKSS_OnSize(int cx, int cy)
	{
		if ((cy > 0) && (cx > 0))
		{
			m_cx = cx;
			m_cy = cy;
		}
	}

	HRESULT CScreenDIB::BKSS_ScreenView_Init(BKScreen_t *pScPar, CWnd * pwndScr)
	{
		CBKScreen_Shared::BKSS_ScreenView_Init(pScPar, pwndScr);
		m_hdd = DrawDibOpen();
		ASSERT(m_hwndScreen != nullptr);
		return BKSS_ScreenView_ReInit(pScPar);
	}

	HRESULT CScreenDIB::BKSS_ScreenView_ReInit(BKScreen_t *pScPar)
	{
		CBKScreen_Shared::BKSS_ScreenView_ReInit(pScPar);
		SAFE_DELETE_OBJECT(m_hbmp);
		HDC hdc = ::GetDC(m_hwndScreen);
		ZeroMemory(&m_bmpinfo, sizeof(BITMAPINFO));
		m_bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		m_bmpinfo.bmiHeader.biWidth = m_screen.nWidth;
		m_bmpinfo.bmiHeader.biHeight = m_screen.nHeight;
		m_bmpinfo.bmiHeader.biPlanes = 1;
		m_bmpinfo.bmiHeader.biBitCount = 32;
		m_bmpinfo.bmiHeader.biCompression = BI_RGB;
		m_bmpinfo.bmiHeader.biSizeImage = 0;
		m_bmpinfo.bmiHeader.biXPelsPerMeter = 0;
		m_bmpinfo.bmiHeader.biYPelsPerMeter = 0;
		m_bmpinfo.bmiHeader.biClrUsed = 0;
		m_bmpinfo.bmiHeader.biClrImportant = 0;
		uint32_t *pbits;
		m_hbmp = CreateDIBSection(hdc, &m_bmpinfo, DIB_RGB_COLORS, (void **)&pbits, nullptr, 0);
		::ReleaseDC(m_hwndScreen, hdc);

		if (m_bFullScreen)
		{
			BKSS_SetFullScreenMode();
		}
		else
		{
			BKSS_SetWindowMode();
		}

		return S_OK;
	}

	void CScreenDIB::BKSS_ScreenView_Done()
	{
		SAFE_DELETE_OBJECT(m_hbmp); // удаляем объект
		DrawDibClose(m_hdd);
	}

	void CScreenDIB::BKSS_DrawScreen()
	{
		HDC hdc = ::GetDC(m_hwndScreen);

		if (m_bFullScreen)
		{
			DrawDibDraw(m_hdd, hdc,
			            m_screen.rcFSViewPort.left, m_screen.rcFSViewPort.top, m_screen.rcFSViewPort.right, m_screen.rcFSViewPort.bottom,
			            &m_bmpinfo.bmiHeader, m_screen.pTexture,
			            0, 0, m_screen.nWidth, m_screen.nHeight,
			            DDF_BUFFER | DDF_HALFTONE | DDF_FULLSCREEN);
		}
		else
		{
			DrawDibDraw(m_hdd, hdc,
			            0, 0, m_cx, m_cy,
			            &m_bmpinfo.bmiHeader, m_screen.pTexture,
			            0, 0, m_screen.nWidth, m_screen.nHeight,
			            DDF_BUFFER | DDF_HALFTONE | DDF_NOTKEYFRAME);
		}

		::ReleaseDC(m_hwndScreen, hdc);
	}

	bool CScreenDIB::BKSS_SetFullScreenMode()
	{
		bool bRet = false;

		if (!m_bFullScreen)
		{
			// запомним состояние главного окна. На будущее, когда придумаем, как правильно
			// работать в мультимониторных конфигурациях
			m_mainPlacement.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(m_pwndMain->GetSafeHwnd(), &m_mainPlacement);
			// запомним состояние окна экрана в оконном режиме
			m_windowedPlacement.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(m_hwndScreen, &m_windowedPlacement);
			// меняем стили с чилд на попап
			SetWindowLongPtr(m_hwndScreen, GWL_STYLE, (GetWindowLongPtr(m_hwndScreen, GWL_STYLE) & ~WS_CHILD) | WS_POPUP);
			SetParent(m_hwndScreen, nullptr); // отвязываем от родителя
			// сворачиваем главное окно в иконку.
			WINDOWPLACEMENT mnplacement = m_mainPlacement;
			mnplacement.flags |= WPF_SETMINPOSITION;
			mnplacement.showCmd = SW_SHOWMINIMIZED;
			SetWindowPlacement(m_pwndMain->GetSafeHwnd(), &mnplacement);
			// разворачиваем экран на весь экран монитора, при этом он всегда разворачивается
			// на основном мониторе
			m_pwndScreen->SetWindowPos(&CWnd::wndTop, m_screen.rcFSDim.left, m_screen.rcFSDim.top, m_screen.rcFSDim.right, m_screen.rcFSDim.bottom, SWP_SHOWWINDOW);
			// почему-то окошко делается прозрачным,
			// поэтому мы вручную закрасим его чёрным.
			PAINTSTRUCT ps;
			BeginPaint(m_hwndScreen, &ps);
			FillRect(ps.hdc, &ps.rcPaint, HBRUSH(::GetStockObject(BLACK_BRUSH)));
			EndPaint(m_hwndScreen, &ps);
			m_bFullScreen = true;
			bRet = true;
		}

		return bRet;
	}

	bool CScreenDIB::BKSS_SetWindowMode()
	{
		// Hide Background window
		if (m_bFullScreen)
		{
			// возвращаем привязку к родителю
			SetParent(m_hwndScreen, m_pwndParent->GetSafeHwnd());
			// возвращаем стиль чилд и убираем попап
			SetWindowLongPtr(m_hwndScreen, GWL_STYLE, (GetWindowLongPtr(m_hwndScreen, GWL_STYLE) & ~WS_POPUP) | WS_CHILD);
			// возвращаем положение экрана в оконном режиме
			SetWindowPlacement(m_hwndScreen, &m_windowedPlacement);
			// возвращаем положение главного окна
			SetWindowPlacement(m_pwndMain->GetSafeHwnd(), &m_mainPlacement);
			m_bFullScreen = false;
		}

		return true;
	}

};

#endif