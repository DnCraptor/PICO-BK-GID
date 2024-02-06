#pragma once
#ifdef BKSCRDLL_EXPORTS
#define BKSCRDLL_API __declspec(dllexport)
#else
#define BKSCRDLL_API __declspec(dllimport)
#endif

// Этот класс экспортирован из BKScreenD2D.dll
#include "Screen_Shared.h"

#include <d2d1.h>
#include <d2d1helper.h>

BKEXTERN_C class BKSCRDLL_API CScreenD2D : public CBKScreen_Shared
{
	protected:

		ID2D1Factory       *m_pDirect2dFactory;
		ID2D1HwndRenderTarget *m_pRenderTarget;
		ID2D1Bitmap        *m_pBitmap;
		D2D1_RENDER_TARGET_PROPERTIES m_rtprop;

		D2D1_RECT_F         m_rectFSWnd_f; //размеры viewportа в полноэкранном режиме
		D2D1_RECT_F         m_rectWnd_f; //размеры viewportа в оконном режиме
		D2D1_RECT_F         m_rectCurrentViewport_f;

		D2D1_SIZE_U         m_sizeWnd, m_sizeFS; //размеры рендертаргета в оконном и полноэкранном режимах
		bool                m_bSmoothing;


		HRESULT             CreateDeviceResources();
		HRESULT             CreateBitmap();
		void                DiscardDeviceResources();
		void                DiscardBitmap();
		void                CalcFSWnd();

	public:
		CScreenD2D();
		virtual ~CScreenD2D() override;

		virtual HRESULT     BKSS_ScreenView_Init(BKScreen_t *pScPar, CWnd *pwndScr) override;
		virtual HRESULT     BKSS_ScreenView_ReInit(BKScreen_t *pScPar) override;
		virtual void        BKSS_ScreenView_Done() override;
		virtual void        BKSS_DrawScreen() override;
		virtual bool        BKSS_SetFullScreenMode() override;
		virtual bool        BKSS_SetWindowMode() override;
		virtual void        BKSS_SetSmoothing(bool bSmoothing) override;
};


