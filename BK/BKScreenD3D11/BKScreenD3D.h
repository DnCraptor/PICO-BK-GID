#pragma once
#ifdef BKSCRDLL_EXPORTS
#define BKSCRDLL_API __declspec(dllexport)
#else
#define BKSCRDLL_API __declspec(dllimport)
#endif
/*
class CCritSec
{
public:

    CCritSec() :
        m_cs()
    {
        InitializeCriticalSection(&m_cs);
    }

    ~CCritSec()
    {
        DeleteCriticalSection(&m_cs);
    }

    _Acquires_lock_(this->m_cs)
        void Lock(void)
    {
        EnterCriticalSection(&m_cs);
    }

    _Releases_lock_(this->m_cs)
        void Unlock(void)
    {
        LeaveCriticalSection(&m_cs);
    }

private:

    CRITICAL_SECTION m_cs;
};

class CAutoLock
{
public:

    _Acquires_lock_(this->m_pLock->m_cs)
        CAutoLock(CCritSec* pLock) :
        m_pLock(pLock)
    {
        m_pLock->Lock();
    }

    _Releases_lock_(this->m_pLock->m_cs)
        ~CAutoLock(void)
    {
        m_pLock->Unlock();
    }

private:

    CCritSec * m_pLock;
};
*/

#include "Screen_Shared.h"
#include "LockVarType.h"
#include <d3d11_1.h>

BKEXTERN_C class BKSCRDLL_API CScreenD3D : public CBKScreen_Shared
{
	protected:
		LockVarType                     m_lockDrawing;
		D3D11_VIEWPORT                  m_vp;
		RECT                            m_rectWndVP;            // размеры viewporta в оконном режиме
		// left, top - координаты экрана, right, bottom - размеры, а не координаты!!!
		DXGI_MODE_DESC                  m_fullScrMode;
		DXGI_MODE_DESC                  m_windowMode;

		D3D_DRIVER_TYPE                 m_driverType;           // Этот параметр указывает, производить вычисления в видеокарте или в центральном процессоре.
		D3D_FEATURE_LEVEL               m_featureLevel;         // Параметр, указывающий, какую версию DirectX поддерживает наша видеокарта.
		ID3D11Device                   *m_pd3dDevice;           // интерфейс для создания ресурсов
		ID3D11Device1                  *m_pd3dDevice1;
		ID3D11DeviceContext            *m_pImmediateContext;    // интерфейс для вывода графической информации
		ID3D11DeviceContext1           *m_pImmediateContext1;
		IDXGISwapChain                 *m_pSwapChain;           // интерфейс для работы с буферами - задним и передним
		IDXGISwapChain1                *m_pSwapChain1;
		ID3D11RenderTargetView         *m_pRenderTargetView;    // объект заднего буфера, в котором происходит отрисовка
//      ID3D11Texture2D                *m_pDepthStencil;
//      ID3D11DepthStencilView         *m_pDepthStencilView;
		ID3D11VertexShader             *m_pVertexShader;
		ID3D11PixelShader              *m_pPixelShader;
		ID3D11InputLayout              *m_pVertexLayout;
		ID3D11Buffer                   *m_pVertexBuffer;
		ID3D11Buffer                   *m_pCBNeverChanges;
//      ID3D11Buffer                   *m_pCBChangeOnResize;
//      ID3D11Buffer                   *m_pCBChangesEveryFrame;
		ID3D11Texture2D                *m_pTexture;             // Наша текстура
		ID3D11ShaderResourceView       *m_pTextureRV;
		ID3D11SamplerState             *m_pSamplerPoint;
		ID3D11SamplerState             *m_pSamplerLinear;

		WINDOWPLACEMENT                 m_fullscreenPlacement;

		void                            ChangeSize(int cx, int cy);

		bool                            m_bSmoothing;
		HRESULT                         SetSamplerState();

		HRESULT                         CreateTexture();
		void                            ReleaseTexture();

	public:
		CScreenD3D();
		virtual ~CScreenD3D() override;

		virtual HRESULT     BKSS_ScreenView_Init(BKScreen_t *pScPar, CWnd *pwndScr) override;
		virtual HRESULT     BKSS_ScreenView_ReInit(BKScreen_t *pScPar) override;
		virtual void        BKSS_ScreenView_Done() override;
		virtual void        BKSS_DrawScreen() override;
		virtual void        BKSS_RestoreFullScreen() override;
		virtual bool        BKSS_SetFullScreenMode() override;
		virtual bool        BKSS_SetWindowMode() override;

		virtual void        BKSS_SetSmoothing(bool bSmoothing) override;
		virtual void        BKSS_OnSize(int cx, int cy) override;
};


