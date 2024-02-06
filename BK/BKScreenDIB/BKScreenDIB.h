#pragma once
#ifdef BKSCRDLL_EXPORTS
#define BKSCRDLL_API __declspec(dllexport)
#else
#define BKSCRDLL_API __declspec(dllimport)
#endif

// Этот класс экспортирован из BKScreenDIB.dll
#include "Screen_Shared.h"

#include <vfw.h>

BKEXTERN_C class BKSCRDLL_API CScreenDIB : public CBKScreen_Shared
{
	protected:

		int                 m_cx, m_cy;

		HDRAWDIB            m_hdd;
		BITMAPINFO          m_bmpinfo;
		HBITMAP             m_hbmp;

	public:
		CScreenDIB();
		virtual ~CScreenDIB() override;

		virtual HRESULT     BKSS_ScreenView_Init(BKScreen_t *pScPar, CWnd *pwndScr) override;
		virtual HRESULT     BKSS_ScreenView_ReInit(BKScreen_t *pScPar) override;
		virtual void        BKSS_ScreenView_Done() override;
		virtual void        BKSS_DrawScreen() override;
		virtual bool        BKSS_SetFullScreenMode() override;
		virtual bool        BKSS_SetWindowMode() override;
		virtual void        BKSS_OnSize(int cx, int cy) override;

		virtual bool BKSS_GetReverseFlag() override
		{
			return true;
		}
};


