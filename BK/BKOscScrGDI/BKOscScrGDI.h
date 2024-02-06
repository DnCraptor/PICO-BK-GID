#pragma once
#include "BKOscScr_Shared.h"

#ifdef BKOSCDLL_EXPORTS
#define BKOSCDLL_API __declspec(dllexport)
#else
#define BKOSCDLL_API __declspec(dllimport)
#endif

// Вот это всем тормозам тормоз!!!
// пока не будет найден способ использовать аппаратное ускорение, НЕ ПРИМЕНЯТЬ!

BKEXTERN_C class BKOSCDLL_API CBKOscScrGDI: public CBKOScScr_Shared
{
		HWND                m_hwndScreen;
		CDC                 *m_pDC;
		HDC                 m_hdc;
#if (USEGDIPLUS)
		Gdiplus::GdiplusStartupInput m_gdiplusStratupInput;
		ULONG_PTR m_gdiplusToken;
#endif
	public:
		virtual void BKOSC_OnDisplay(SAMPLE_INT *inBuf, const int inLen) override;
		virtual HRESULT BKOSC_Screen_Init(CWnd *pWndScreen) override;
		virtual void BKOSC_Screen_Done() override;
		virtual void BKOSC_OnSize(const int cx, const int cy) override;
		virtual HRESULT BKOSC_OnSetBuffer(const int nLen) override;

		CBKOscScrGDI();
		virtual ~CBKOscScrGDI() override;
};
