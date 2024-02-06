// BKOscScrGDI.cpp: определяет экспортированные функции для приложения DLL.
//

#include "pch.h"

#define USEGDIPLUS 0
// с USEGDIPLUS работает чудовищно медленно, без него и то быстрее
#if (USEGDIPLUS)
#define GDIPVER 0x0110
#include <GdiPlus.h>
#pragma comment(lib, "gdiplus.lib")
#endif

#include "BKOscScrGDI.h"
#include "atltypes.h"

#ifdef GDI

BKEXTERN_C
{

	BKOSCDLL_API BKOSCSCRHANDLE APIENTRY GetBKOscScr()
	{
		return new CBKOscScrGDI;
	}

	CBKOscScrGDI::CBKOscScrGDI()
		: m_pDC(nullptr)
		, m_hwndScreen(nullptr)
	{
	}

	CBKOscScrGDI::~CBKOscScrGDI()
	= default;

	void CBKOscScrGDI::BKOSC_OnSize(const int cx, const int cy)
	{
	}

	HRESULT CBKOscScrGDI::BKOSC_OnSetBuffer(const int nLen)
	{
		return S_OK;
	}


	HRESULT CBKOscScrGDI::BKOSC_Screen_Init(CWnd * pWndScreen)
	{
		HRESULT hr = E_FAIL;

		if (pWndScreen)
		{
			m_pDC = pWndScreen->GetDC();
			m_hdc = m_pDC->GetSafeHdc();
			m_hwndScreen = pWndScreen->GetSafeHwnd();
			hr = S_OK;
#if (USEGDIPLUS)
			Gdiplus::GdiplusStartup(&m_gdiplusToken, &m_gdiplusStratupInput, nullptr);
#endif
		}

		return hr;
	}

	void CBKOscScrGDI::BKOSC_Screen_Done()
	{
#if (USEGDIPLUS)
		Gdiplus::GdiplusShutdown(m_gdiplusToken);
#endif
	}

	void CBKOscScrGDI::BKOSC_OnDisplay(SAMPLE_INT * inBuf, const int inLen)
	{
#if (USEGDIPLUS)
		static Gdiplus::Pen penWhite(Gdiplus::Color(255, 255, 255, 255));
		static Gdiplus::Pen penGreen(Gdiplus::Color(255, 0, 255, 0));
		static Gdiplus::Pen penLightSlateGray(Gdiplus::Color(255, 0x77, 0x88, 0x99));
		static Gdiplus::SolidBrush sbr(Gdiplus::Color(255, 0, 0, 0));

		CRect rectClient;
		GetClientRect(m_hwndScreen, &rectClient);

		Gdiplus::Graphics graphics(m_hdc);
		Gdiplus::PointF rtSize(rectClient.Width(), rectClient.Height());

		// очищаем экран
		graphics.FillRectangle(&sbr, Gdiplus::RectF(0, 0, rtSize.X, rtSize.Y));

		// rtSize.x - ширина окна
		// rtSize.y - высота окна
		Gdiplus::REAL scalex = rtSize.X / Gdiplus::REAL(inLen);
		Gdiplus::REAL half = rtSize.Y / 2;

		// рисуем разделительную линию между каналами
		graphics.DrawLine(&penLightSlateGray, Gdiplus::PointF(0, half), Gdiplus::PointF(rtSize.X, half));

		Gdiplus::REAL h4 = rtSize.Y / 4;
		Gdiplus::REAL h34 = rtSize.Y * 3 / 4;
		Gdiplus::PointF pntbgnL(0, h4);
		Gdiplus::PointF pntendL(rtSize.X, h4);
		Gdiplus::PointF pntbgnR(0, h34);
		Gdiplus::PointF pntendR(rtSize.X, h34);
		// рисуем линии центров каналов.
		graphics.DrawLine(&penWhite, pntbgnL, pntendL);
		graphics.DrawLine(&penWhite, pntbgnR, pntendR);

		for (int i = 0; i < inLen; ++i)
		{
		    // рисуем левый канал -1.0 .. 1.0 => height/2 .. 0
		    pntendR.X = pntendL.X = Gdiplus::REAL(i) * scalex;
		    pntendL.Y = h4 - Gdiplus::REAL(*inBuf++) * h4;
		    graphics.DrawLine(&penGreen, pntbgnL, pntendL);
		    pntbgnL = pntendL;
		    // рисуем правый канал -1.0 .. 1.0 => height .. height/2
		    pntendR.Y = h34 - Gdiplus::REAL(*inBuf++) * h4;
		    graphics.DrawLine(&penGreen, pntbgnR, pntendR);
		    pntbgnR = pntendR;
		}
#else
		static CPen penWhite(PS_SOLID, 0, RGB(255, 255, 255));
		static CPen penGreen(PS_SOLID, 0, RGB(0, 255, 0));
		static CPen penLightSlateGray(PS_SOLID, 0, RGB(0x77, 0x88, 0x99));
		CRect rectClient;
		GetClientRect(m_hwndScreen, &rectClient);
		// очищаем экран
		m_pDC->FillSolidRect(&rectClient, RGB(0, 0, 0));
		CPoint rtSize(rectClient.Width(), rectClient.Height());
		// rtSize.x - ширина окна
		// rtSize.y - высота окна
		float scalex = float(rtSize.x) / float(inLen);
		LONG half = rtSize.y / 2;
		// рисуем разделительную линию между каналами
		m_pDC->SelectObject(&penLightSlateGray);
		m_pDC->MoveTo(0, half);  // ставим точку
		m_pDC->LineTo(rtSize.x, half);
		LONG h4 = rtSize.y / 4;
		LONG h34 = rtSize.y * 3 / 4;
		CPoint pntbgnL(0,        h4);
		CPoint pntendL(rtSize.x, h4);
		CPoint pntbgnR(0,        h34);
		CPoint pntendR(rtSize.x, h34);
		// рисуем линии центров каналов.
		m_pDC->SelectObject(&penLightSlateGray);
		m_pDC->MoveTo(pntbgnL);
		m_pDC->LineTo(pntendL);
		m_pDC->MoveTo(pntbgnR);
		m_pDC->LineTo(pntendR);
		m_pDC->SelectObject(&penGreen);

		for (int i = 0; i < inLen; ++i)
		{
			// рисуем левый канал -1.0 .. 1.0 => height/2 .. 0
			pntendR.x = pntendL.x = LONG(float(i) * scalex);
			pntendL.y = h4 - LONG((*inBuf++) * h4);
			m_pDC->MoveTo(pntbgnL);
			m_pDC->LineTo(pntendL);
			pntbgnL = pntendL;
			// рисуем правый канал -1.0 .. 1.0 => height .. height/2
			pntendR.y = h34 - LONG((*inBuf++) * h4);
			m_pDC->MoveTo(pntbgnR);
			m_pDC->LineTo(pntendR);
			pntbgnR = pntendR;
		}
#endif
	}
};

#endif // GDI
