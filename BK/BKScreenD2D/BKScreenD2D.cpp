// BKScreenD2D.cpp: определяет экспортированные функции для приложения DLL.
//

#include "pch.h"
#include "BKScreenD2D.h"
#include "Screen_Sizes.h"
#ifdef D2D
#pragma comment (lib, "d2d1.lib")

template<class Interface>
inline void SafeRelease(Interface **ppInterfaceToRelease)
{
	if (*ppInterfaceToRelease != nullptr)
	{
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = nullptr;
	}
}

BKEXTERN_C
{
	BKSCRDLL_API BKSCREENHANDLE APIENTRY GetBKScreen()
	{
		return new CScreenD2D;
	}

	CScreenD2D::CScreenD2D()
		: m_pDirect2dFactory(nullptr)
		, m_pRenderTarget(nullptr)
		, m_pBitmap(nullptr)
		, m_bSmoothing(false)
	{
	}

	CScreenD2D::~CScreenD2D()
	= default;

	HRESULT CScreenD2D::BKSS_ScreenView_Init(BKScreen_t *pScPar, CWnd * pwndScr)
	{
		CBKScreen_Shared::BKSS_ScreenView_Init(pScPar, pwndScr);
		// здесь rect своего типа

		if (m_hwndScreen == nullptr)
		{
			return E_FAIL;
		}

		CalcFSWnd();    // вычисляем размеры окна в полноэкранном режиме
		// Create a Direct2D factory.
		HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &m_pDirect2dFactory);

		if (FAILED(hr))
		{
			hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);
		}

		if (SUCCEEDED(hr))
		{
			hr = CreateDeviceResources();

			if (SUCCEEDED(hr))
			{
				if (m_bFullScreen)
				{
					BKSS_SetFullScreenMode();
				}
				else
				{
					BKSS_SetWindowMode();
				}
			}
		}

		return hr;
	}


	void CScreenD2D::CalcFSWnd()
	{
		// вычисляем размеры окна в полноэкранном режиме
		m_sizeFS = D2D1::SizeU(
		               m_screen.rcFSDim.right - m_screen.rcFSDim.left,
		               m_screen.rcFSDim.bottom - m_screen.rcFSDim.top
		           );
		m_rectFSWnd_f.left = static_cast<FLOAT>(m_screen.rcFSViewPort.left);
		m_rectFSWnd_f.top = static_cast<FLOAT>(m_screen.rcFSViewPort.top);
		m_rectFSWnd_f.right = static_cast<FLOAT>(m_screen.rcFSViewPort.left + m_screen.rcFSViewPort.right); // а тут не размер, а конечная точка
		m_rectFSWnd_f.bottom = static_cast<FLOAT>(m_screen.rcFSViewPort.top + m_screen.rcFSViewPort.bottom);
	}

	HRESULT CScreenD2D::BKSS_ScreenView_ReInit(BKScreen_t *pScPar)
	{
		CBKScreen_Shared::BKSS_ScreenView_ReInit(pScPar);
		CalcFSWnd(); // вычисляем размеры окна в полноэкранном режиме
		DiscardBitmap();
		HRESULT hr = CreateBitmap();

		if (SUCCEEDED(hr))
		{
			if (m_bFullScreen)
			{
				BKSS_SetFullScreenMode();
			}
			else
			{
				BKSS_SetWindowMode();
			}
		}

		return hr;
	}

	void CScreenD2D::BKSS_ScreenView_Done()
	{
		SafeRelease(&m_pBitmap);
		SafeRelease(&m_pRenderTarget);
		SafeRelease(&m_pDirect2dFactory);
	}


	HRESULT CScreenD2D::CreateDeviceResources()
	{
		HRESULT hr = S_OK;

		if (!m_pRenderTarget)
		{
			RECT rc; // размеры окна оконного режима
			::GetClientRect(m_hwndScreen, &rc);
			m_sizeWnd = D2D1::SizeU(
			                rc.right - rc.left,
			                rc.bottom - rc.top
			            );
			m_rectWnd_f = { 0.0f, 0.0f, static_cast<FLOAT>(m_sizeWnd.width), static_cast<FLOAT>(m_sizeWnd.height) };
			m_rtprop = D2D1::RenderTargetProperties();
			m_rtprop.pixelFormat = D2D1::PixelFormat(
			                           DXGI_FORMAT_B8G8R8A8_UNORM, // поддерживается только _UNORM
			                           D2D1_ALPHA_MODE_IGNORE
			                       );
			//m_pDirect2dFactory->GetDesktopDpi(&m_rtprop.dpiX, &m_rtprop.dpiY); //тут так нельзя, масшабирование слетает.
			m_rtprop.dpiX = DEFAULT_DPIX; m_rtprop.dpiY = DEFAULT_DPIY;
			m_rtprop.type = D2D1_RENDER_TARGET_TYPE_HARDWARE; // с D2D1_RENDER_TARGET_TYPE_HARDWARE работает только DXGI_FORMAT_R8G8B8A8
			// DXGI_FORMAT_B8G8R8A8 работает со всеми типами, _HARDWARE _SOFTWARE, _DEFAULT
			// Create a Direct2D render target.
			hr = m_pDirect2dFactory->CreateHwndRenderTarget(
			         m_rtprop,
			         D2D1::HwndRenderTargetProperties(m_hwndScreen, m_sizeWnd),
			         &m_pRenderTarget
			     );

			if (FAILED(hr))
			{
				// если не удалось включить хардварное ускорение
				m_rtprop.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
				hr = m_pDirect2dFactory->CreateHwndRenderTarget(
				         m_rtprop,
				         D2D1::HwndRenderTargetProperties(m_hwndScreen, m_sizeWnd),
				         &m_pRenderTarget
				     );
				// попробуем то, что предлагается по умолчанию.
			}

			if (SUCCEEDED(hr))
			{
				hr = CreateBitmap();
			}
		}

		return hr;
	}

	HRESULT CScreenD2D::CreateBitmap()
	{
		// теперь надо создать битмап
		D2D1_SIZE_U bmpsize{ m_screen.nWidth, m_screen.nHeight };
		D2D1_BITMAP_PROPERTIES bmprp = D2D1::BitmapProperties();
		bmprp.pixelFormat = m_rtprop.pixelFormat;
		m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		m_pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		return m_pRenderTarget->CreateBitmap(bmpsize, bmprp, &m_pBitmap);
	}

	void CScreenD2D::DiscardBitmap()
	{
		SafeRelease(&m_pBitmap);
	}

	void CScreenD2D::DiscardDeviceResources()
	{
		DiscardBitmap();
		SafeRelease(&m_pRenderTarget);
	}

	void CScreenD2D::BKSS_SetSmoothing(bool bSmoothing)
	{
		m_bSmoothing = bSmoothing;
	}

	void CScreenD2D::BKSS_DrawScreen()
	{
		HRESULT hr = m_pBitmap->CopyFromMemory(nullptr, m_screen.pTexture, m_screen.nPitch);
		hr = CreateDeviceResources();

		if (SUCCEEDED(hr) && !(m_pRenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
		{
			m_pRenderTarget->BeginDraw();
			// вот так примерно делается поворот. но не учитываются пропорции экрана, там надо видимо ещё что-то делать.
			//m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(90.f, D2D1::Point2F(m_rectWnd_f.right/2, m_rectWnd_f.bottom/2)));
			//m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Green)); //очистка экрана
			// Draw a bitmap.
			m_pRenderTarget->DrawBitmap(
			    m_pBitmap,
			    m_rectCurrentViewport_f,
			    1.0f,
			    (m_bSmoothing ? D2D1_BITMAP_INTERPOLATION_MODE_LINEAR : D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR)
			);
			hr = m_pRenderTarget->EndDraw();
			m_pRenderTarget->CheckWindowState();

			if (hr == D2DERR_RECREATE_TARGET)
			{
				DiscardDeviceResources();
			}
		}
	}

	bool CScreenD2D::BKSS_SetFullScreenMode()
	{
		bool bRet = false;

		if (!m_bFullScreen)
		{
			// запомним состояние главного окна. На будущее, когда придумаем, как правильно
			// работать в мультимониторных конфигурациях
			m_mainPlacement.length = sizeof(WINDOWPLACEMENT);
			::GetWindowPlacement(m_pwndMain->GetSafeHwnd(), &m_mainPlacement);
			// запомним состояние окна экрана в оконном режиме
			m_windowedPlacement.length = sizeof(WINDOWPLACEMENT);
			::GetWindowPlacement(m_hwndScreen, &m_windowedPlacement);
			// меняем стили с чилд на попап
			::SetWindowLongPtr(m_hwndScreen, GWL_STYLE, (GetWindowLongPtr(m_hwndScreen, GWL_STYLE) & ~WS_CHILD) | WS_POPUP);
			::SetParent(m_hwndScreen, nullptr); // отвязываем от родителя
			// сворачиваем главное окно в иконку.
			WINDOWPLACEMENT mnplacement = m_mainPlacement;
			mnplacement.flags |= WPF_SETMINPOSITION;
			mnplacement.showCmd = SW_SHOWMINIMIZED;
			::SetWindowPlacement(m_pwndMain->GetSafeHwnd(), &mnplacement);
			// Но главное окно никуда не девается. А всё потому, что мы принудительно ему фокус передаём!
			// Мало того, если принудительно скрыть главное окно
			// эмулятор вообще останавливает работу. Так что такая фича не прокатывает.
			// разворачивем экран на весь экран монитора, при этом он всегда разворачивается
			// на основном мониторе
			m_pwndScreen->SetWindowPos(&CWnd::wndTop, m_screen.rcFSDim.left, m_screen.rcFSDim.top, m_screen.rcFSDim.right, m_screen.rcFSDim.bottom, SWP_SHOWWINDOW);
			m_pRenderTarget->Resize(m_sizeFS);
			m_bFullScreen = true;
			bRet = true;
		}

		m_rectCurrentViewport_f = m_rectFSWnd_f;
		return bRet;
	}

	bool CScreenD2D::BKSS_SetWindowMode()
	{
		if (m_bFullScreen)
		{
			// возвращаем привязку к родителю
			::SetParent(m_hwndScreen, m_pwndParent->GetSafeHwnd());
			// возвращаем стиль чилд и убираем попап
			::SetWindowLongPtr(m_hwndScreen, GWL_STYLE, (GetWindowLongPtr(m_hwndScreen, GWL_STYLE) & ~WS_POPUP) | WS_CHILD);
			// возвращаем положение экрана в оконном режиме
			::SetWindowPlacement(m_hwndScreen, &m_windowedPlacement);
			// возвращаем положение главного окна
			::SetWindowPlacement(m_pwndMain->GetSafeHwnd(), &m_mainPlacement);
			m_pRenderTarget->Resize(m_sizeWnd);
			m_bFullScreen = false;
		}

		m_rectCurrentViewport_f = m_rectWnd_f;
		return true;
	}
};

#endif // D2D
