// BKOscScrD2D.cpp: определяет экспортированные функции для приложения DLL.
//

#include "pch.h"
#include "BKOscScrD2D.h"

#ifdef D2D

#pragma comment (lib, "d2d1.lib")


template<class Interface>
inline void
SafeRelease(Interface **ppInterfaceToRelease)
{
	if (*ppInterfaceToRelease != nullptr)
	{
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = nullptr;
	}
}

BKEXTERN_C
{
	BKOSCDLL_API BKOSCSCRHANDLE APIENTRY GetBKOscScr()
	{
		return new CBKOscScrD2D;
	}

	CBKOscScrD2D::CBKOscScrD2D()
		: m_pDirect2dFactory(nullptr)
		, m_pRenderTarget(nullptr)
		, m_pLightSlateGrayBrush(nullptr)
		, m_pWhiteBrush(nullptr)
		, m_pGreenBrush(nullptr)
		, m_pStrokeStyleCustomOffsetZero(nullptr)
	{
	}

	CBKOscScrD2D::~CBKOscScrD2D()
	= default;


	void CBKOscScrD2D::BKOSC_OnSize(const int cx, const int cy)
	{
		if (m_pRenderTarget)
		{
			// Note: This method can fail, but it's okay to ignore the
			// error here, because the error will be returned again
			// the next time EndDraw is called.
			m_pRenderTarget->Resize(D2D1::SizeU(cx, cy));
		}
	}

	HRESULT CBKOscScrD2D::BKOSC_OnSetBuffer(const int nLen)
	{
		return S_OK;
	}

	HRESULT CBKOscScrD2D::CreateFactory()
	{
		HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &m_pDirect2dFactory);

		if (FAILED(hr))
		{
			hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);
		}

		return hr;
	}

	HRESULT CBKOscScrD2D::BKOSC_Screen_Init(CWnd * pWndScreen)
	{
		RECT rc; // размеры окна оконного режима
		HWND hwndScreen = pWndScreen->GetSafeHwnd();
		::GetClientRect(hwndScreen, &rc);
		D2D1_SIZE_U size = D2D1::SizeU(
		                       rc.right - rc.left,
		                       rc.bottom - rc.top
		                   );
		m_rectWnd = { 0.0f, 0.0f, FLOAT(size.width), FLOAT(size.height) };
		// Dash array for dashStyle D2D1_DASH_STYLE_CUSTOM
		static float dashes[] = { 1.0f, 2.0f, 2.0f, 3.0f, 2.0f, 2.0f };
		// Create a Direct2D factory.
		HRESULT hr = CreateFactory();

		if (SUCCEEDED(hr))
		{
			D2D1_RENDER_TARGET_PROPERTIES rtprop = D2D1::RenderTargetProperties();
			D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(
			                                    DXGI_FORMAT_B8G8R8A8_UNORM, // поддерживается только _UNORM
			                                    D2D1_ALPHA_MODE_IGNORE
			                                );
			rtprop.pixelFormat = pixelFormat;
			m_pDirect2dFactory->GetDesktopDpi(&rtprop.dpiX, &rtprop.dpiY);
			//GetDpiForWindow(hwndScreen); // эта штука не работает в винхп
			// с D2D1_RENDER_TARGET_TYPE_HARDWARE работает только DXGI_FORMAT_R8G8B8A8
			// DXGI_FORMAT_B8G8R8A8 работает со всеми типами, _HARDWARE _SOFTWARE, _DEFAULT
			rtprop.type = D2D1_RENDER_TARGET_TYPE_HARDWARE;
			// Create a Direct2D render target.
			hr = m_pDirect2dFactory->CreateHwndRenderTarget(
			         rtprop,
			         D2D1::HwndRenderTargetProperties(hwndScreen, size),
			         &m_pRenderTarget
			     );

			if (FAILED(hr))
			{
				// если не удалось включить хардварное ускорение
				rtprop.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
				hr = m_pDirect2dFactory->CreateHwndRenderTarget(
				         rtprop,
				         D2D1::HwndRenderTargetProperties(hwndScreen, size),
				         &m_pRenderTarget
				     );
				// попробуем то, что предлагается по умолчанию.
			}

			// Stroke Style with Dash Style -- Custom
			if (SUCCEEDED(hr))
			{
				hr = m_pDirect2dFactory->CreateStrokeStyle(
				         D2D1::StrokeStyleProperties(
				             D2D1_CAP_STYLE_FLAT,
				             D2D1_CAP_STYLE_FLAT,
				             D2D1_CAP_STYLE_ROUND,
				             D2D1_LINE_JOIN_MITER,
				             10.0f,
				             D2D1_DASH_STYLE_CUSTOM,
				             0.0f),
				         dashes,
				         ARRAYSIZE(dashes),
				         &m_pStrokeStyleCustomOffsetZero
				     );
			}

			if (SUCCEEDED(hr))
			{
				// Create a gray brush.
				hr = m_pRenderTarget->CreateSolidColorBrush(
				         D2D1::ColorF(D2D1::ColorF::LightSlateGray),
				         &m_pLightSlateGrayBrush
				     );
			}

			if (SUCCEEDED(hr))
			{
				// Create a white brush.
				hr = m_pRenderTarget->CreateSolidColorBrush(
				         D2D1::ColorF(D2D1::ColorF::White),
				         &m_pWhiteBrush
				     );
			}

			if (SUCCEEDED(hr))
			{
				// Create a lime brush.
				hr = m_pRenderTarget->CreateSolidColorBrush(
				         D2D1::ColorF(D2D1::ColorF::Lime),
				         &m_pGreenBrush
				     );
			}
		}

		return hr;
	}

	void CBKOscScrD2D::BKOSC_Screen_Done()
	{
		SafeRelease(&m_pLightSlateGrayBrush);
		SafeRelease(&m_pWhiteBrush);
		SafeRelease(&m_pGreenBrush);
		SafeRelease(&m_pDirect2dFactory);
		SafeRelease(&m_pRenderTarget);
		SafeRelease(&m_pStrokeStyleCustomOffsetZero);
	}



	void CBKOscScrD2D::BKOSC_OnDisplay(SAMPLE_INT * inBuf, const int inLen)
	{
		m_pRenderTarget->BeginDraw();
		// очищаем экран
		m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
		const D2D1_SIZE_F rtSize = m_pRenderTarget->GetSize();
		const FLOAT scalex = rtSize.width / FLOAT(inLen);
		const FLOAT half = rtSize.height / 2.0f;
		// рисуем разделительную линию между каналами
		m_pRenderTarget->DrawLine(
		    D2D1::Point2F(0.0f, half),
		    D2D1::Point2F(rtSize.width, half),
		    //m_pLightSlateGrayBrush
		    m_pWhiteBrush
		    // ,0.5f
		);
		const FLOAT h4 = rtSize.height / 4.0f;
		const FLOAT h34 = rtSize.height * 0.75f;
		D2D1_POINT_2F pntbgnL = D2D1::Point2F(0.0f,         h4);
		D2D1_POINT_2F pntendL = D2D1::Point2F(rtSize.width, pntbgnL.y);
		D2D1_POINT_2F pntbgnR = D2D1::Point2F(0.0f,         h34);
		D2D1_POINT_2F pntendR = D2D1::Point2F(rtSize.width, pntbgnR.y);
		// рисуем линии центров каналов.
		m_pRenderTarget->DrawLine(pntbgnL, pntendL, m_pLightSlateGrayBrush, 1.0f, m_pStrokeStyleCustomOffsetZero);  //m_pWhiteBrush
		m_pRenderTarget->DrawLine(pntbgnR, pntendR, m_pLightSlateGrayBrush, 1.0f, m_pStrokeStyleCustomOffsetZero);  //m_pWhiteBrush

		FLOAT fi = 0;
		for (int i = 0; i < inLen; ++i, fi++)
		{
			// рисуем левый канал -1.0 .. 1.0 => height/2 .. 0
			pntendR.x = pntendL.x = fi * scalex;
			pntendL.y = h4 - FLOAT(*inBuf++) * h4;
			m_pRenderTarget->DrawLine(pntbgnL, pntendL, m_pGreenBrush);
			pntbgnL = pntendL;
			// рисуем правый канал -1.0 .. 1.0 => height .. height/2
			pntendR.y = h34 - FLOAT(*inBuf++) * h4;
			m_pRenderTarget->DrawLine(pntbgnR, pntendR, m_pGreenBrush);
			pntbgnR = pntendR;
		}

		HRESULT hr = m_pRenderTarget->EndDraw();
	}
};

#endif // D2D
