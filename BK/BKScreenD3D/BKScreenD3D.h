#pragma once
#ifdef BKSCRDLL_EXPORTS
#define BKSCRDLL_API __declspec(dllexport)
#else
#define BKSCRDLL_API __declspec(dllimport)
#endif

// !!! Лучше не использовать.
// 1. работает тормознуто в полноэкранном режиме (зависит от драйверов и видеокарты)
// 2. не работает одновременно несколько 3d окон на экране (ибо для этого нужно городить целую систему).
// 3. Хрень какая-то. В полноэкранном режиме Alt-Tab не работает. Т.е. работает, но т.к. окно поверх всех
//   то не видно ничего, как под ним делается переключение окон. И оно нихера не сворачивается.
//   и как я это сделал, я забыл, как теперь вернуть всё по-стандартному, не знаю.


#include "Screen_Shared.h"

#include <d3d9.h>
#include <d3dx9.h>
#include <D3dx9tex.h>
#include "LockVarType.h"
struct CUSTOMVERTEX
{
	D3DXVECTOR3 position; // The position
	D3DCOLOR    color;    // The color
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2)

BKEXTERN_C class BKSCRDLL_API CScreenD3D : public CBKScreen_Shared
{
	protected:
		LockVarType             m_lockDrawing;
		LPDIRECT3D9             m_pD3D;     // Используется для создания D3DDevice
		LPDIRECT3DDEVICE9       m_pd3dDevice;   // Это наше устройство вывода
		LPDIRECT3DVERTEXBUFFER9 m_pVB;      // Буфер с вертексами
		LPDIRECT3DTEXTURE9      m_pTexture; // Наша текстура
		D3DDISPLAYMODE          m_d3ddmWIN; // режим дисплея для оконного режима
		D3DDISPLAYMODE          m_d3ddmFS;  // режим дисплея для полноэкранного режима.
		D3DPRESENT_PARAMETERS   m_d3dpp;    // параметры создаваемого устройства вывода
		D3DVIEWPORT9            m_d3dvp;
		RECT                    m_rectWnWnd; // размеры viewporta в оконном режиме
		// left, top - координаты экрана, right, bottom - размеры, а не координаты!!!

		HRESULT                 CreateTexture();
		HRESULT                 SetupMatrices();
		void                    setFSparam();
		void                    setWNDparam();
		void                    setDeviceParam();

		DWORD                   m_nFilter;
		HRESULT                 SetSamplerState();

	public:
		CScreenD3D();
		virtual ~CScreenD3D() override;

		virtual HRESULT         BKSS_ScreenView_Init(BKScreen_t *pScPar, CWnd *pwndScr) override;
		virtual HRESULT         BKSS_ScreenView_ReInit(BKScreen_t *pScPar) override;
		virtual void            BKSS_ScreenView_Done() override;
		virtual void            BKSS_DrawScreen() override;
		virtual bool            BKSS_SetFullScreenMode() override;
		virtual bool            BKSS_SetWindowMode() override;

		virtual void            BKSS_SetSmoothing(bool bSmoothing) override;
		virtual void            BKSS_OnSize(int cx, int cy) override;
};


