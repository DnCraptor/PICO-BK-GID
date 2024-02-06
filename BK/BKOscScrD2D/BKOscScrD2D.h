#pragma once
#include "BKOscScr_Shared.h"

#ifdef BKOSCDLL_EXPORTS
#define BKOSCDLL_API __declspec(dllexport)
#else
#define BKOSCDLL_API __declspec(dllimport)
#endif


#include <d2d1.h>
#include <d2d1helper.h>

BKEXTERN_C class BKOSCDLL_API CBKOscScrD2D: public CBKOScScr_Shared
{

		D2D1_RECT_F            m_rectWnd; // размеры viewportа в оконном режиме

		ID2D1Factory          *m_pDirect2dFactory;
		ID2D1HwndRenderTarget *m_pRenderTarget;
		ID2D1SolidColorBrush  *m_pLightSlateGrayBrush;
		ID2D1SolidColorBrush  *m_pWhiteBrush;
		ID2D1SolidColorBrush  *m_pGreenBrush;
		ID2D1StrokeStyle      *m_pStrokeStyleCustomOffsetZero;

		HRESULT CreateFactory();

	public:
		virtual void BKOSC_OnDisplay(SAMPLE_INT *inBuf, const int inLen) override;
		virtual HRESULT BKOSC_Screen_Init(CWnd *pWndScreen) override;
		virtual void BKOSC_Screen_Done() override;
		virtual void BKOSC_OnSize(const int cx, const int cy) override;
		virtual HRESULT BKOSC_OnSetBuffer(const int nLen) override;

		CBKOscScrD2D();
		virtual ~CBKOscScrD2D() override;
};
