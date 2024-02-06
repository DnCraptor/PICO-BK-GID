// BKScreenD3D.cpp: определяет экспортированные функции для приложения DLL.
//
// Косяк: после выхода из полноэкранного режима ломается D2D осциллограф.
// хотя это скорее всего косяк самого осциллографа.

#include "pch.h"
#include "SafeReleaseDefines.h"

#ifdef D3D

// эти вещи компилируются на лету, поэтому студия может ругаться, что инклуды не найдены
#include "PixelShader.hps"
#include "VertexShader.hvs"

#include <directxmath.h>
#include <directxcolors.h>
#include <vector>
#include "BKScreenD3D.h"

#pragma comment (lib,"d3d11.lib")
#pragma comment (lib,"dxguid.lib")
#pragma comment (lib,"winmm.lib")

#ifdef _DEBUG
#include "..\DX11Err\DxErr.h"
#endif

using namespace DirectX;

struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};

struct CBNeverChanges
{
	XMMATRIX mView;
	XMMATRIX mProjection;
};

// struct CBChangeOnResize
// {
//  XMMATRIX mProjection;
// };

// struct CBChangesEveryFrame
// {
//  XMMATRIX mWorld;
// };


// CScreenD3D
BKEXTERN_C
{
	BKSCRDLL_API BKSCREENHANDLE APIENTRY GetBKScreen()
	{
		return new CScreenD3D;
	}

//  XMMATRIX                            g_World;
#ifdef _DEBUG
	const TCHAR *errstr;
#define DXERRORDESC_SIZE 1024
	TCHAR errdesc[DXERRORDESC_SIZE];
	void DebugMessage(HRESULT hr)
	{
		errstr = DXGetErrorString(hr);
		DXGetErrorDescription(hr, errdesc, DXERRORDESC_SIZE);
		CString str = CString(errstr) + _T("\r\n") + CString(errdesc);
		TRACE(str + _T("\n"));
		AfxMessageBox(str, MB_OK);
	}
#endif

#define BUFFERS_COUNT 2
#define VIEWS_COUNT 1


	CScreenD3D::CScreenD3D()
		: m_driverType(D3D_DRIVER_TYPE_NULL)
		, m_featureLevel(D3D_FEATURE_LEVEL_11_0)
		, m_pd3dDevice(nullptr)
		, m_pd3dDevice1(nullptr)
		, m_pImmediateContext(nullptr)
		, m_pImmediateContext1(nullptr)
		, m_pSwapChain(nullptr)
		, m_pSwapChain1(nullptr)
		, m_pRenderTargetView(nullptr)
//  , m_pDepthStencil(nullptr)
//  , m_pDepthStencilView(nullptr)
		, m_pVertexShader(nullptr)
		, m_pPixelShader(nullptr)
		, m_pVertexLayout(nullptr)
		, m_pVertexBuffer(nullptr)
		, m_pCBNeverChanges(nullptr)
//  , m_pCBChangeOnResize(nullptr)
//  , m_pCBChangesEveryFrame(nullptr)
		, m_pTexture(nullptr)
		, m_pTextureRV(nullptr)
		, m_pSamplerPoint(nullptr)
		, m_pSamplerLinear(nullptr)
	{
		ZeroMemory(&m_fullScrMode, sizeof(DXGI_MODE_DESC));
		ZeroMemory(&m_windowMode, sizeof(DXGI_MODE_DESC));
		ZeroMemory(&m_fullscreenPlacement, sizeof(WINDOWPLACEMENT));
		ZeroMemory(&m_windowedPlacement, sizeof(WINDOWPLACEMENT));
	}

	CScreenD3D::~CScreenD3D()
	= default;

// обработчики сообщений CScreenD3D
	void CScreenD3D::BKSS_OnSize(int cx, int cy)
	{
		if (m_hwndScreen && (cx | cy))
		{
			if (m_pSwapChain)
			{
				ChangeSize(cx, cy);
			}
		}
	}

	HRESULT CScreenD3D::BKSS_ScreenView_Init(BKScreen_t *pScPar, CWnd * pwndScr)
	{
		CBKScreen_Shared::BKSS_ScreenView_Init(pScPar, pwndScr);

		if (!m_hwndScreen)
		{
			return -1;
		}

		::GetClientRect(m_hwndScreen, &m_rectWndVP);
		HRESULT hr = S_OK;
		UINT createDeviceFlags = 0;
#ifdef _DEBUG
// однако, вот эта штука внезапно перестала работать после переустановки Win7.
// что такое нужно установить чтобы оно опять заработало?
// А вот что:
// For Windows 7 with Platform Update for Windows 7 (KB2670838) or Windows 8.x,
//  to create a device that supports the debug layer,
//  install the Windows Software Development Kit (SDK) for Windows 8.x to get D3D11_1SDKLayers.dll.
// For Windows 10, to create a device that supports the debug layer, enable
//  the "Graphics Tools" optional feature. Go to the Settings panel, under System, Apps & features,
//  Manage optional Features, Add a feature, and then look for "Graphics Tools".
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		D3D_DRIVER_TYPE driverTypes[] =
		{
			D3D_DRIVER_TYPE_HARDWARE,
			D3D_DRIVER_TYPE_WARP,
			D3D_DRIVER_TYPE_REFERENCE,
		};
		D3D_FEATURE_LEVEL featureLevels[] =
		{
			//D3D_FEATURE_LEVEL_12_2,
			//D3D_FEATURE_LEVEL_12_1,
			//D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1
		};
		constexpr UINT numFeatureLevels = _ARRAYSIZE(featureLevels);

		for (auto &driverType : driverTypes)
		{
			hr = D3D11CreateDevice(nullptr, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			                       D3D11_SDK_VERSION, &m_pd3dDevice, &m_featureLevel, &m_pImmediateContext);

			if (hr == E_INVALIDARG || hr == E_FAIL)
			{
				// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
				hr = D3D11CreateDevice(nullptr, driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
				                       D3D11_SDK_VERSION, &m_pd3dDevice, &m_featureLevel, &m_pImmediateContext);
			}

			if (SUCCEEDED(hr))
			{
				m_driverType = driverType;
				break;
			}
		}

		if (FAILED(hr))
		{
#ifdef _DEBUG
			DebugMessage(hr);
#endif
			return hr;
		}

		// Obtain DXGI factory from device (since we used nullptr for pAdapter above)
		IDXGIFactory1 *dxgiFactory = nullptr;
		{
			IDXGIDevice *dxgiDevice = nullptr;
			hr = m_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void **>(&dxgiDevice));

			if (SUCCEEDED(hr))
			{
				IDXGIAdapter *adapter = nullptr;
				hr = dxgiDevice->GetAdapter(&adapter);

				if (SUCCEEDED(hr))
				{
					hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void **>(&dxgiFactory));
					adapter->Release();
				}

				dxgiDevice->Release();
			}
		}

		if (FAILED(hr))
		{
#ifdef _DEBUG
			DebugMessage(hr);
#endif
			return hr;
		}

		// Create swap chain
		IDXGIFactory2 *dxgiFactory2 = nullptr;
		hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void **>(&dxgiFactory2));

		if (dxgiFactory2)
		{
			// DirectX 11.1 or later
			hr = m_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void **>(&m_pd3dDevice1));

			if (SUCCEEDED(hr))
			{
				(void)m_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void **>(&m_pImmediateContext1));
			}

			DXGI_SWAP_CHAIN_DESC1 sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.Width = m_rectWndVP.right;
			sd.Height = m_rectWndVP.bottom;
			sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sd.SampleDesc.Count = VIEWS_COUNT;
			sd.SampleDesc.Quality = 0;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.BufferCount = BUFFERS_COUNT;
			sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // DXGI_SWAP_EFFECT_SEQUENTIAL;
			DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsd;
			ZeroMemory(&fsd, sizeof(fsd));
			fsd.RefreshRate.Numerator = 60;
			fsd.RefreshRate.Denominator = 1;
			fsd.Scaling = DXGI_MODE_SCALING_CENTERED;
			fsd.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
			fsd.Windowed = TRUE;
			hr = dxgiFactory2->CreateSwapChainForHwnd(m_pd3dDevice, m_hwndScreen, &sd, &fsd, nullptr, &m_pSwapChain1);

			if (SUCCEEDED(hr))
			{
				hr = m_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void **>(&m_pSwapChain));
			}

			dxgiFactory2->Release();
		}
		else
		{
			// DirectX 11.0 systems
			DXGI_SWAP_CHAIN_DESC sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.SampleDesc.Count = VIEWS_COUNT;
			sd.SampleDesc.Quality = 0;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.BufferCount = BUFFERS_COUNT;
			sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			sd.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
			sd.BufferDesc.Width = m_rectWndVP.right;
			sd.BufferDesc.Height = m_rectWndVP.bottom;
			sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sd.BufferDesc.RefreshRate.Numerator = 60;
			sd.BufferDesc.RefreshRate.Denominator = 1;
			sd.OutputWindow = m_hwndScreen;
			sd.Windowed = TRUE;
			hr = dxgiFactory->CreateSwapChain(m_pd3dDevice, &sd, &m_pSwapChain);
		}

		// Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
		hr = dxgiFactory->MakeWindowAssociation(m_hwndScreen, DXGI_MWA_NO_ALT_ENTER);
		dxgiFactory->Release();

		if (FAILED(hr))
		{
#ifdef _DEBUG
			DebugMessage(hr);
#endif
			return hr;
		}

		DXGI_SWAP_CHAIN_DESC swchDesc;
		hr = m_pSwapChain->GetDesc(&swchDesc);
		m_fullScrMode = m_windowMode = swchDesc.BufferDesc;
		// ща поразбираемся с режимами
		IDXGIOutput *pOutput = nullptr;
		hr = m_pSwapChain->GetContainingOutput(&pOutput);

		if (SUCCEEDED(hr))
		{
			UINT numModes = 0;
			// Instead, use IDXGIOutput1::GetDisplayModeList1, which supports stereo display mode.
			// сперва узнаем сколько режимов вернёт функция
			hr = pOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numModes, nullptr);

			if (numModes)
			{
				auto pModes = std::vector<DXGI_MODE_DESC>(numModes);

				if (pModes.data())
				{
					// теперь заполним массив
					hr = pOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numModes, pModes.data());
					UINT nModeNum = -1;
					UINT w = 0, h = 0;

					for (UINT i = 0; i < numModes; ++i)
					{
						if ((w < pModes[i].Width) || (h < pModes[i].Height))
						{
							// находим первое из наибольших разрешений
							w = pModes[i].Width;
							h = pModes[i].Height;
							nModeNum = i;
						}
					}

					if (nModeNum != -1)
					{
						m_fullScrMode = pModes[nModeNum];
					}
				}
			}

			pOutput->Release();
		}

		// Create a render target view
		ID3D11Texture2D *pBackBuffer = nullptr;
		hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&pBackBuffer));

		if (FAILED(hr))
		{
#ifdef _DEBUG
			DebugMessage(hr);
#endif
			return hr;
		}

		hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
		pBackBuffer->Release();

		if (FAILED(hr))
		{
#ifdef _DEBUG
			DebugMessage(hr);
#endif
			return hr;
		}

//      // Create depth stencil texture
//      D3D11_TEXTURE2D_DESC descDepth;
//      ZeroMemory(&descDepth, sizeof(descDepth));
//      descDepth.Width = m_rectWndVP.right;
//      descDepth.Height = m_rectWndVP.bottom;
//      descDepth.MipLevels = 1;
//      descDepth.ArraySize = 1;
//      descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
//      descDepth.SampleDesc.Count = 1;
//      descDepth.SampleDesc.Quality = 0;
//      descDepth.Usage = D3D11_USAGE_DEFAULT;
//      descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
//      descDepth.CPUAccessFlags = 0;
//      descDepth.MiscFlags = 0;
//      hr = m_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &m_pDepthStencil);
//      if (FAILED(hr))
//      {
// #ifdef _DEBUG
//          DebugMessage(hr);
// #endif
//          return hr;
//      }
//
//      // Create the depth stencil view
//      D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
//      ZeroMemory(&descDSV, sizeof(descDSV));
//      descDSV.Format = descDepth.Format;
//      descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
//      descDSV.Texture2D.MipSlice = 0;
//      hr = m_pd3dDevice->CreateDepthStencilView(m_pDepthStencil, &descDSV, &m_pDepthStencilView);
//      if (FAILED(hr))
//      {
// #ifdef _DEBUG
//          DebugMessage(hr);
// #endif
//          return hr;
//      }
//      m_pImmediateContext->OMSetRenderTargets(VIEWS_COUNT, &m_pRenderTargetView, m_pDepthStencilView);
		m_pImmediateContext->OMSetRenderTargets(VIEWS_COUNT, &m_pRenderTargetView, nullptr);
		// Setup the viewport
		m_vp.Width  = static_cast<FLOAT>(m_rectWndVP.right);
		m_vp.Height = static_cast<FLOAT>(m_rectWndVP.bottom);
		m_vp.MinDepth = 0.0f;
		m_vp.MaxDepth = 0.0f;
		m_vp.TopLeftX = 0;
		m_vp.TopLeftY = 0;
		m_pImmediateContext->RSSetViewports(VIEWS_COUNT, &m_vp);
		// Create the vertex shader
		hr = m_pd3dDevice->CreateVertexShader(g_vsshader, sizeof(g_vsshader), nullptr, &m_pVertexShader);

		if (FAILED(hr))
		{
#ifdef _DEBUG
			DebugMessage(hr);
#endif
			return hr;
		}

		// Define the input layout
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = _ARRAYSIZE(layout);
		// Create the input layout
		hr = m_pd3dDevice->CreateInputLayout(layout, numElements, g_vsshader, sizeof(g_vsshader), &m_pVertexLayout);

		if (FAILED(hr))
		{
#ifdef _DEBUG
			DebugMessage(hr);
#endif
			return hr;
		}

		// Set the input layout
		m_pImmediateContext->IASetInputLayout(m_pVertexLayout);
		// Create the pixel shader
		hr = m_pd3dDevice->CreatePixelShader(g_psshader, sizeof(g_psshader), nullptr, &m_pPixelShader);

		if (FAILED(hr))
		{
#ifdef _DEBUG
			DebugMessage(hr);
#endif
			return hr;
		}

		// Create vertex buffer
		SimpleVertex vertices[] =
		{
			// вертикально
			{ XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(1.0f,  1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
			// поворот вправо
//          { XMFLOAT3( 1.0f,  1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
//          { XMFLOAT3( 1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
//          { XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
//          { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
			// поворот влево
//          { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
//          { XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
//          { XMFLOAT3( 1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
//          { XMFLOAT3( 1.0f,  1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
			//вверх ногами
//          { XMFLOAT3( 1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
//          { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
//          { XMFLOAT3( 1.0f,  1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
//          { XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
		};
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(SimpleVertex) * _ARRAYSIZE(vertices);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = vertices;
		hr = m_pd3dDevice->CreateBuffer(&bd, &InitData, &m_pVertexBuffer);

		if (FAILED(hr))
		{
#ifdef _DEBUG
			DebugMessage(hr);
#endif
			return hr;
		}

		// Set vertex buffer
		UINT stride = sizeof(SimpleVertex);
		UINT offset = 0;
		m_pImmediateContext->IASetVertexBuffers(0, BUFFERS_COUNT, &m_pVertexBuffer, &stride, &offset);
		// Set primitive topology
		m_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		// Create the constant buffers
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(CBNeverChanges);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pCBNeverChanges);

		if (FAILED(hr))
		{
#ifdef _DEBUG
			DebugMessage(hr);
#endif
			return hr;
		}

//      bd.ByteWidth = sizeof(CBChangeOnResize);
//      hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pCBChangeOnResize);
//      if (FAILED(hr))
//          return hr;
//      bd.ByteWidth = sizeof(CBChangesEveryFrame);
//      hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pCBChangesEveryFrame);
//      if (FAILED(hr))
//          return hr;
		hr = CreateTexture();

		if (FAILED(hr))
		{
			return hr;
		}

		SetSamplerState();
		// Initialize the world matrices
		// g_World = XMMatrixIdentity();
		// Initialize the view matrix
		XMVECTOR Eye = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);    // Точка из которой мы смотрим
		XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);      // Точка в которую смотрим
		XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);      // Где верх?
		XMMATRIX g_View = XMMatrixLookAtLH(Eye, At, Up);
		// Initialize the projection matrix
		// XMMATRIX g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1.0f, 0.01f, 100.0f);
		XMMATRIX g_Projection = XMMatrixOrthographicLH(2.0f, 2.0f, 0.0f, 1.0f);
		CBNeverChanges cbNeverChanges{};
		cbNeverChanges.mProjection = XMMatrixTranspose(g_Projection);
		cbNeverChanges.mView = XMMatrixTranspose(g_View);
		m_pImmediateContext->UpdateSubresource(m_pCBNeverChanges, 0, nullptr, &cbNeverChanges, 0, 0);
//      CBChangeOnResize cbChangesOnResize;
//      cbChangesOnResize.mProjection = XMMatrixTranspose(g_Projection);
//      m_pImmediateContext->UpdateSubresource(m_pCBChangeOnResize, 0, nullptr, &cbChangesOnResize, 0, 0);
		return S_OK;
	}


	HRESULT CScreenD3D::BKSS_ScreenView_ReInit(BKScreen_t *pScPar)
	{
		CBKScreen_Shared::BKSS_ScreenView_ReInit(pScPar);
		ReleaseTexture();
		return CreateTexture();
	}

	HRESULT CScreenD3D::CreateTexture()
	{
		// создаём текстуру
		D3D11_TEXTURE2D_DESC descTex;
		ZeroMemory(&descTex, sizeof(descTex));
		descTex.Width = m_screen.nWidth;
		descTex.Height = m_screen.nHeight;
		descTex.MipLevels = 1;
		descTex.ArraySize = 1;
		descTex.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		descTex.SampleDesc.Count = 1;
		descTex.SampleDesc.Quality = 0;
		descTex.Usage = D3D11_USAGE_DEFAULT;
		descTex.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		descTex.CPUAccessFlags = 0;
		descTex.MiscFlags = 0;
		HRESULT hr = m_pd3dDevice->CreateTexture2D(&descTex, nullptr, &m_pTexture);

		if (FAILED(hr))
		{
#ifdef _DEBUG
			DebugMessage(hr);
#endif
			return hr;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = descTex.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		hr = m_pd3dDevice->CreateShaderResourceView(m_pTexture, &srvDesc, &m_pTextureRV);

		if (FAILED(hr))
		{
#ifdef _DEBUG
			DebugMessage(hr);
#endif
		}

		return hr;
	}

	void CScreenD3D::ReleaseTexture()
	{
		if (m_pTextureRV)
		{
			m_pTextureRV->Release();
		}

		if (m_pTexture)
		{
			m_pTexture->Release();
		}
	}

	void CScreenD3D::BKSS_ScreenView_Done()
	{
		if (m_pImmediateContext)
		{
			m_pImmediateContext->ClearState();
		}

		if (m_pSamplerLinear)
		{
			m_pSamplerLinear->Release();
		}

		if (m_pSamplerPoint)
		{
			m_pSamplerPoint->Release();
		}

		ReleaseTexture();

		if (m_pCBNeverChanges)
		{
			m_pCBNeverChanges->Release();
		}

//      if (m_pCBChangeOnResize) m_pCBChangeOnResize->Release();
//      if (m_pCBChangesEveryFrame) m_pCBChangesEveryFrame->Release();
		if (m_pVertexBuffer)
		{
			m_pVertexBuffer->Release();
		}

		if (m_pVertexLayout)
		{
			m_pVertexLayout->Release();
		}

		if (m_pVertexShader)
		{
			m_pVertexShader->Release();
		}

		if (m_pPixelShader)
		{
			m_pPixelShader->Release();
		}

//      if (m_pDepthStencil) m_pDepthStencil->Release();
//      if (m_pDepthStencilView) m_pDepthStencilView->Release();
		if (m_pRenderTargetView)
		{
			m_pRenderTargetView->Release();
		}

		if (m_pSwapChain1)
		{
			m_pSwapChain1->Release();
		}

		if (m_pSwapChain)
		{
			m_pSwapChain->Release();
		}

		if (m_pImmediateContext1)
		{
			m_pImmediateContext1->Release();
		}

		if (m_pImmediateContext)
		{
			m_pImmediateContext->Release();
		}

		if (m_pd3dDevice1)
		{
			m_pd3dDevice1->Release();
		}

		if (m_pd3dDevice)
		{
			m_pd3dDevice->Release();
		}
	}

	void CScreenD3D::BKSS_DrawScreen()
	{
		BOOL bFS;
		m_pSwapChain->GetFullscreenState(&bFS, nullptr);

		if (m_bFullScreen && !bFS)
		{
			return;
		}

		m_lockDrawing.Lock();
		m_pImmediateContext->OMSetRenderTargets(VIEWS_COUNT, &m_pRenderTargetView, nullptr); // Если задан DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, то тут всегда надо задавать RenderTargets
		// Source Row Pitch = [size of one element in bytes] * [number of elements in one row]
		// Source Depth Pitch = [Source Row Pitch] * [number of rows(height)]
		m_pImmediateContext->UpdateSubresource(m_pTexture, 0, nullptr, m_screen.pTexture,
		                                       m_screen.nPitch,
		                                       m_screen.nTextureSize);
		// Update our time
//      static float t = 0.0f;
//      if (m_driverType == D3D_DRIVER_TYPE_REFERENCE)
//      {
//          t += (float)XM_PI * 0.0125f;
//      }
//      else
//      {
//          static ULONGLONG timeStart = 0;
//          ULONGLONG timeCur = GetTickCount64();
//          if (timeStart == 0)
//              timeStart = timeCur;
//          t = (timeCur - timeStart) / 1000.0f;
//      }
		//
		// Clear the back buffer
		//
		// m_pImmediateContext->ClearRenderTargetView(m_pRenderTargetView, Colors::Lime);
		//
		// Clear the depth buffer to 1.0 (max depth)
		//
		// m_pImmediateContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
		//
		// Update variables that change once per frame
		//
//      CBChangesEveryFrame cb;
//      cb.mWorld = XMMatrixRotationRollPitchYaw(t,t,t); // вращаем по трём осям сразу
//      // cb.mWorld = XMMatrixTranspose(g_World);
//      m_pImmediateContext->UpdateSubresource(m_pCBChangesEveryFrame, 0, nullptr, &cb, 0, 0);
		//
		// Render the cube
		//
		m_pImmediateContext->VSSetShader(m_pVertexShader, nullptr, 0);
		m_pImmediateContext->VSSetConstantBuffers(0, 1, &m_pCBNeverChanges);
//      m_pImmediateContext->VSSetConstantBuffers(1, 1, &m_pCBChangeOnResize);
//      m_pImmediateContext->VSSetConstantBuffers(2, 1, &m_pCBChangesEveryFrame);
		m_pImmediateContext->PSSetShader(m_pPixelShader, nullptr, 0);
//      m_pImmediateContext->PSSetConstantBuffers(2, 1, &m_pCBChangesEveryFrame);
		m_pImmediateContext->PSSetShaderResources(0, 1, &m_pTextureRV);

		if (m_bSmoothing)
		{
			m_pImmediateContext->PSSetSamplers(0, 1, &m_pSamplerLinear);
		}
		else
		{
			m_pImmediateContext->PSSetSamplers(0, 1, &m_pSamplerPoint);
		}

		m_pImmediateContext->Draw(4, 0);
		//
		// Present our back buffer to our front buffer
		//
		m_pSwapChain->Present(1, 0);
		m_lockDrawing.UnLock();
	}

	void CScreenD3D::BKSS_RestoreFullScreen()
	{
		BOOL bFS;
		m_pSwapChain->GetFullscreenState(&bFS, nullptr);

		if (m_bFullScreen && !bFS)
		{
			// вот эта штука переключает в полноэкранный режим и не даёт переключиться на другие окна
			HRESULT hr = m_pSwapChain->ResizeTarget(&m_fullScrMode);
			hr = m_pSwapChain->SetFullscreenState(TRUE, nullptr);
			ChangeSize(m_fullScrMode.Width, m_fullScrMode.Height);
		}
	}

	bool CScreenD3D::BKSS_SetFullScreenMode()
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
			HRESULT hr;

			if (m_fullScrMode.Width)
			{
//              DEVMODE devMode;
//              ZeroMemory(&devMode, sizeof(DEVMODE));
//              devMode.dmSize = sizeof(DEVMODE);
//              devMode.dmPelsWidth = m_fullScrMode.Width;
//              devMode.dmPelsHeight = m_fullScrMode.Height;
//              devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
//              ChangeDisplaySettings(&devMode, CDS_FULLSCREEN);
				hr = m_pSwapChain->ResizeTarget(&m_fullScrMode);
				hr = m_pSwapChain->SetFullscreenState(TRUE, nullptr);
				ChangeSize(m_fullScrMode.Width, m_fullScrMode.Height);

				if (SUCCEEDED(hr))
				{
					bRet = true;
					m_bFullScreen = true;
					// сохраним расположение окна в полноэкранном режиме
					m_fullscreenPlacement.length = sizeof(WINDOWPLACEMENT);
					GetWindowPlacement(m_hwndScreen, &m_fullscreenPlacement);
				}
				else
				{
#ifdef _DEBUG
					errstr = DXGetErrorString(hr);
					DXGetErrorDescription(hr, errdesc, DXERRORDESC_SIZE);
					CString str = CString(errstr) + _T("\r\n") + CString(errdesc);
					TRACE(str + _T("\n"));
					// AfxMessageBox(str, MB_OK);
#endif
					// восстанавливаем расположение окна с рамкой
					SetWindowPlacement(m_hwndScreen, &m_windowedPlacement);
					// возвращаем положение главного окна
					SetWindowPlacement(m_pwndMain->GetSafeHwnd(), &m_mainPlacement);
					hr = m_pSwapChain->ResizeTarget(&m_windowMode);
					hr = m_pSwapChain->SetFullscreenState(FALSE, nullptr);
					ChangeSize(m_windowMode.Width, m_windowMode.Height);
				}
			}
		}

		return bRet;
	}

	bool CScreenD3D::BKSS_SetWindowMode()
	{
		bool bRet = false;

		if (m_bFullScreen)
		{
			HRESULT hr;
			// возвращаем привязку к родителю
			SetParent(m_hwndScreen, m_pwndParent->GetSafeHwnd());
			// возвращаем стиль чилд и убираем попап
			SetWindowLongPtr(m_hwndScreen, GWL_STYLE, (GetWindowLongPtr(m_hwndScreen, GWL_STYLE) & ~WS_POPUP) | WS_CHILD);
			// возвращаем положение экрана в оконном режиме
			SetWindowPlacement(m_hwndScreen, &m_windowedPlacement);
			// возвращаем положение главного окна
			SetWindowPlacement(m_pwndMain->GetSafeHwnd(), &m_mainPlacement);
			hr = m_pSwapChain->ResizeTarget(&m_windowMode);
			hr = m_pSwapChain->SetFullscreenState(FALSE, nullptr);
			ChangeSize(m_rectWndVP.right, m_rectWndVP.bottom);

			if (SUCCEEDED(hr))
			{
				bRet = true;
				m_bFullScreen = false;
			}
			else
			{
#ifdef _DEBUG
				DebugMessage(hr);
#endif
			}
		}

		return bRet;
	}

	void CScreenD3D::BKSS_SetSmoothing(bool bSmoothing)
	{
		m_bSmoothing = bSmoothing;
	}

	void CScreenD3D::ChangeSize(int cx, int cy)
	{
		// m_lockDrawing.Lock();
		if (!m_lockDrawing.TimedLock(20))
		{
			return;
		}

		m_pImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
		// Release all outstanding references to the swap chain's buffers.
		m_pRenderTargetView->Release();
		HRESULT hr;
		// Preserve the existing buffer count and format.
		// Automatically choose the width and height to match the client rect for HWNDs.
		hr = m_pSwapChain->ResizeBuffers(BUFFERS_COUNT, cx, cy, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
		// Perform error handling here!
		// Get buffer and create a render-target-view.
		ID3D11Texture2D *pBackBuffer = nullptr;
		hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&pBackBuffer));
		// Perform error handling here!
		hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
		// Perform error handling here!
		pBackBuffer->Release();
		m_pImmediateContext->OMSetRenderTargets(VIEWS_COUNT, &m_pRenderTargetView, nullptr);
		// Set up the viewport.
		m_vp.Width = FLOAT(cx);
		m_vp.Height = FLOAT(cy);
		m_pImmediateContext->RSSetViewports(VIEWS_COUNT, &m_vp);
		m_lockDrawing.UnLock();
	}

	HRESULT CScreenD3D::SetSamplerState()
	{
		// Create the sample state
		D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory(&sampDesc, sizeof(sampDesc));
		sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		HRESULT hr = m_pd3dDevice->CreateSamplerState(&sampDesc, &m_pSamplerLinear);

		if (FAILED(hr))
		{
#ifdef _DEBUG
			DebugMessage(hr);
#endif
		}

		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		hr = m_pd3dDevice->CreateSamplerState(&sampDesc, &m_pSamplerPoint);

		if (FAILED(hr))
		{
#ifdef _DEBUG
			DebugMessage(hr);
#endif
		}

		return hr;
	}

};

#endif // D3D
