// BKScreenD3D.cpp: определяет экспортированные функции для приложения DLL.
//

#include "pch.h"
#include "BKScreenD3D.h"

#ifdef D3D

#pragma comment (lib,"d3d9.lib")
#pragma comment (lib,"D3dx9.lib")
#ifdef _DEBUG
#if (_MSC_VER >= 1900)
#pragma comment (lib,"legacy_stdio_definitions.lib")
#endif
#include <DxErr.h>
#pragma comment (lib,"DxErr.lib")
#endif

BKEXTERN_C
{
	BKSCRDLL_API BKSCREENHANDLE APIENTRY GetBKScreen()
	{
		return new CScreenD3D;
	}

	CScreenD3D::CScreenD3D()
		: m_pD3D(nullptr)
		, m_pd3dDevice(nullptr)
		, m_pVB(nullptr)
		, m_pTexture(nullptr)
		, m_nFilter(D3DTEXF_LINEAR)
	{
	}

	CScreenD3D::~CScreenD3D()
	{
	}


#define VLINES_NUM 2
#define POINTS_NUM (VLINES_NUM*2)
#define TRIANGLES_NUM (2*(VLINES_NUM-1))

	void CScreenD3D::BKSS_OnSize(int cx, int cy)
	{
//      if (m_pwndScreen)
//      {
//          m_pwndScreen->GetClientRect(&m_rectWnWnd);
//      }
	}

	HRESULT CScreenD3D::BKSS_ScreenView_Init(BKScreen_t *pScPar, CWnd * pwndScr)
	{
		CBKScreen_Shared::BKSS_ScreenView_Init(pScPar, pwndScr);

		if (!m_hwndScreen)
		{
			return E_FAIL;
		}

		// Создаём D3D объект.
		if (nullptr == (m_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
		{
			return E_FAIL;
		}

		if (m_pwndScreen)
		{
			m_pwndScreen->GetClientRect(&m_rectWnWnd);
		}

		// получим параметры оконного режима
		m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &m_d3ddmWIN);
		// а теперь получим параметры полноэкранного режима, это будет так же текущий режим дисплея
		m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &m_d3ddmFS);

		if (m_bFullScreen)
		{
			setFSparam();
		}
		else
		{
			setWNDparam();
		}

		HRESULT hr;
#ifdef _DEBUG
		const TCHAR *errstr;
		const TCHAR *errdesc;
#endif
		D3DDEVTYPE deviceTypes[] =
		{
			D3DDEVTYPE_HAL,
			D3DDEVTYPE_REF,
			D3DDEVTYPE_SW
		};
		int numDeviceTypes = ARRAYSIZE(deviceTypes);

		// Создаём D3DDevice
		for (int deviceTypeIndex = 0; deviceTypeIndex < numDeviceTypes; ++deviceTypeIndex)
		{
			hr = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, deviceTypes[deviceTypeIndex], m_hwndScreen,
			                          D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING,
			                          &m_d3dpp, &m_pd3dDevice);

			if (SUCCEEDED(hr))
			{
				break;
			}

			if (FAILED(hr))
			{
#ifdef _DEBUG
				errstr = DXGetErrorString(hr);
				errdesc = DXGetErrorDescription(hr);
				CString str = CString(errstr) + _T("\r\n") + CString(errdesc);
				AfxMessageBox(str, MB_OK);
#endif
				hr = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, deviceTypes[deviceTypeIndex], m_hwndScreen,
				                          D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_MIXED_VERTEXPROCESSING,
				                          &m_d3dpp, &m_pd3dDevice);

				if (SUCCEEDED(hr))
				{
					break;
				}

				if (FAILED(hr))
				{
#ifdef _DEBUG
					errstr = DXGetErrorString(hr);
					errdesc = DXGetErrorDescription(hr);
					CString str = CString(errstr) + _T("\r\n") + CString(errdesc);
					AfxMessageBox(str, MB_OK);
#endif
					hr = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, deviceTypes[deviceTypeIndex], m_hwndScreen,
					                          D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_SOFTWARE_VERTEXPROCESSING,
					                          &m_d3dpp, &m_pd3dDevice);

					if (SUCCEEDED(hr))
					{
						break;
					}

					if (FAILED(hr))
					{
#ifdef _DEBUG
						errstr = DXGetErrorString(hr);
						errdesc = DXGetErrorDescription(hr);
						CString str = CString(errstr) + _T("\r\n") + CString(errdesc);
						AfxMessageBox(str, MB_OK);
#endif
					}
				}
			}
		}

		if (FAILED(hr))
		{
			AfxMessageBox(_T("Не создаётся экран."), MB_OK);
			return hr;
		}

		setDeviceParam();
		hr = m_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		                         D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
		hr = m_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr); // показ вторичного буфера
		hr = CreateTexture();
		return hr;
	}

	HRESULT CScreenD3D::BKSS_ScreenView_ReInit(BKScreen_t *pScPar)
	{
		CBKScreen_Shared::BKSS_ScreenView_ReInit(pScPar);
		return CreateTexture();
	}

	void CScreenD3D::BKSS_ScreenView_Done()
	{
		if (m_pd3dDevice)
		{
			m_pd3dDevice->SetTexture(0, nullptr);
		}

		SAFE_RELEASE(m_pTexture);
		SAFE_RELEASE(m_pVB);
		SAFE_RELEASE(m_pd3dDevice);
		SAFE_RELEASE(m_pD3D);
	}

	void CScreenD3D::BKSS_DrawScreen()
	{
		m_lockDrawing.Lock();
		HRESULT hr = m_pd3dDevice->TestCooperativeLevel();

		if (hr != D3DERR_DEVICELOST)
		{
			if (hr == D3DERR_DEVICENOTRESET)
			{
				m_pd3dDevice->SetTexture(0, nullptr);
				SAFE_RELEASE(m_pTexture);
				SAFE_RELEASE(m_pVB);
				hr = m_pd3dDevice->Reset(&m_d3dpp);
				setDeviceParam();
				CreateTexture();
			}

			D3DLOCKED_RECT d3dlr;

			if (SUCCEEDED(m_pTexture->LockRect(0, &d3dlr, nullptr, 0)))
			{
				// копируем новый битмап в текстуру
				memcpy(d3dlr.pBits, m_screen.pTexture, m_screen.nTextureSize);
				m_pTexture->UnlockRect(0);
			}

			// очищаем экран.
//          m_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
//                              D3DCOLOR_XRGB(0, 0, 0), 0.0f, 0);

			if (SUCCEEDED(m_pd3dDevice->BeginScene()))
			{
				// Рисуем наши треугольники, на которые натягивается текстура
				hr = m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, TRIANGLES_NUM);
				hr = m_pd3dDevice->EndScene();
			}

			// Показываем на экран содержимое бакбуфера
			hr = m_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
		}

		m_lockDrawing.UnLock();
	}

	bool CScreenD3D::BKSS_SetFullScreenMode()
	{
		m_lockDrawing.Lock();
		bool bRet = false;
		SetWindowLongPtr(m_hwndScreen, GWL_STYLE, (GetWindowLongPtr(m_hwndScreen, GWL_STYLE) & ~WS_CHILD) | WS_POPUP);
		m_pwndScreen->SetWindowPos(nullptr, 0, 0, m_d3ddmFS.Width, m_d3ddmFS.Height, SWP_SHOWWINDOW);
		m_pd3dDevice->SetTexture(0, nullptr);
		SAFE_RELEASE(m_pTexture);
		SAFE_RELEASE(m_pVB);
		setFSparam();
		HRESULT hr = m_pd3dDevice->Reset(&m_d3dpp);
#ifdef _DEBUG
		const TCHAR *errstr = DXGetErrorString(hr);
		const TCHAR *errdesc = DXGetErrorDescription(hr);
#endif

		if (SUCCEEDED(hr))
		{
			setDeviceParam();

			if (SUCCEEDED(CreateTexture()))
			{
				m_bFullScreen = true;
				bRet = true;
			}
		}
		else
		{
			m_lockDrawing.UnLock();
			return BKSS_SetWindowMode(); // делаем обратно оконный режим
		}

		m_lockDrawing.UnLock();
		return bRet;
	}

	bool CScreenD3D::BKSS_SetWindowMode()
	{
		m_lockDrawing.Lock();
		bool bRet = false;
		SetWindowLongPtr(m_hwndScreen, GWL_STYLE, (GetWindowLongPtr(m_hwndScreen, GWL_STYLE) & ~WS_POPUP) | WS_CHILD);
		m_pd3dDevice->SetTexture(0, nullptr);
		SAFE_RELEASE(m_pTexture);
		SAFE_RELEASE(m_pVB);
		setWNDparam();
		HRESULT hr = m_pd3dDevice->Reset(&m_d3dpp);
#ifdef _DEBUG
		const TCHAR *errstr = DXGetErrorString(hr);
		const TCHAR *errdesc = DXGetErrorDescription(hr);
#endif

		if (SUCCEEDED(hr))
		{
			setDeviceParam();

			if (SUCCEEDED(CreateTexture()))
			{
				m_bFullScreen = false;
				bRet = true;
			}
		}

		m_lockDrawing.UnLock();
		return bRet;
	}

	void CScreenD3D::setDeviceParam()
	{
		HRESULT hr;
		// Выключаем выборку обратных сторон треугольников
		hr = m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		// Выключаем D3D освещение
		hr = m_pd3dDevice->SetRenderState(D3DRS_LIGHTING, D3DZB_FALSE);
		// Выключаем Z буфер
		hr = m_pd3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
		hr = m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, D3DZB_FALSE);
		hr = m_pd3dDevice->SetViewport(&m_d3dvp);
	}

	void CScreenD3D::setFSparam()
	{
		ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
		m_d3dpp.BackBufferWidth = m_d3ddmFS.Width;
		m_d3dpp.BackBufferHeight = m_d3ddmFS.Height;
		m_d3dpp.BackBufferFormat = m_d3ddmWIN.Format;
		m_d3dpp.BackBufferCount = D3DPRESENT_BACK_BUFFERS_MAX;
		m_d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE; // D3DMULTISAMPLE_4_SAMPLES;
		m_d3dpp.MultiSampleQuality = 0; // 4;
		m_d3dpp.SwapEffect = D3DSWAPEFFECT_FLIP; // D3DSWAPEFFECT_DISCARD;
		m_d3dpp.Windowed = false;
		m_d3dpp.EnableAutoDepthStencil = false;
		m_d3dpp.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
		m_d3dpp.FullScreen_RefreshRateInHz = m_d3ddmFS.RefreshRate;
		m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
		m_d3dvp.X = m_screen.rcFSViewPort.left;
		m_d3dvp.Y = m_screen.rcFSViewPort.top;
		m_d3dvp.Width = m_screen.rcFSViewPort.right;
		m_d3dvp.Height = m_screen.rcFSViewPort.bottom;
		m_d3dvp.MinZ = 0.0f;
		m_d3dvp.MaxZ = 0.0f;
	}

	void CScreenD3D::setWNDparam()
	{
		// Задаём структуру для создания D3DDevice.
		ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
		m_d3dpp.BackBufferWidth = m_rectWnWnd.right;
		m_d3dpp.BackBufferHeight = m_rectWnWnd.bottom;
		m_d3dpp.BackBufferFormat = m_d3ddmWIN.Format;
		m_d3dpp.BackBufferCount = 1;
		m_d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
		m_d3dpp.MultiSampleQuality = 0;
		m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		m_d3dpp.Windowed = true;
		m_d3dpp.EnableAutoDepthStencil = false;
		m_d3dpp.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
		m_d3dvp.X = m_rectWnWnd.left;
		m_d3dvp.Y = m_rectWnWnd.top;
		m_d3dvp.Width = m_rectWnWnd.right;
		m_d3dvp.Height = m_rectWnWnd.bottom;
		m_d3dvp.MinZ = 0.0f;
		m_d3dvp.MaxZ = 0.0f;
	}
	void CScreenD3D::BKSS_SetSmoothing(bool bSmoothing)
	{
		m_nFilter = bSmoothing ? D3DTEXF_LINEAR : D3DTEXF_POINT;

		if (m_pd3dDevice)
		{
			m_lockDrawing.Lock();
			SetSamplerState();
			m_lockDrawing.UnLock();
		}
	}

	HRESULT CScreenD3D::SetSamplerState()
	{
		// Поэкспериментируйте - увидите разницу:
		//
		// D3DTEXF_NONE         - фильтрация не используется
		// D3DTEXF_POINT        - точечная фильтрация
		// D3DTEXF_LINEAR       - линейная фильтрация
		// D3DTEXF_ANISOTROPIC  - анизотропная фильтрация
		HRESULT hr = m_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, m_nFilter);
		hr = m_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, m_nFilter);
		hr = m_pd3dDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_ANISOTROPIC);
		return hr;
	}

	HRESULT CScreenD3D::CreateTexture()
	{
		HRESULT hr = m_pd3dDevice->SetTexture(0, nullptr);
		SAFE_RELEASE(m_pTexture);
		hr = D3DXCreateTexture(m_pd3dDevice, m_screen.nWidth, m_screen.nHeight, 0, 0,
		                       D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_pTexture);

		if (SUCCEEDED(hr))
		{
//          D3DLOCKED_RECT d3dlr;
//
//          if (SUCCEEDED(m_pTexture->LockRect(0, &d3dlr, nullptr, 0)))
//          {
//              // если текстуру удалось заблокировать, а т.к. больше никто кроме нас её не использует, то удастся всегда.
//              // копируем битмап в текстуру
//              memcpy(d3dlr.pBits, m_bits, m_screen.nWidth * m_screen.nHeight * sizeof(uint32_t));
//              m_pTexture->UnlockRect(0);
//          }
		}
		else
		{
			return hr;
		}

		// Создаём вертексный буфер.
		SAFE_RELEASE(m_pVB);

		if (FAILED(m_pd3dDevice->CreateVertexBuffer(POINTS_NUM * sizeof(CUSTOMVERTEX),
		                                            0, D3DFVF_CUSTOMVERTEX,
		                                            D3DPOOL_MANAGED, &m_pVB, nullptr)))
		{
			return E_FAIL;
		}

		// Заполняем вертексный буфер.
		// минимально необходимое кол-во треугольников 2, но можно сделать и больше, но непонятно, нужно ли
		CUSTOMVERTEX *pVertices;

		if (FAILED(m_pVB->Lock(0, 0, (void **)&pVertices, 0)))
		{
			return E_FAIL;
		}

		for (int i = 0; i < VLINES_NUM; ++i)
		{
			pVertices[2 * i + 0].color = D3DCOLOR_XRGB(255, 255, 255);
			pVertices[2 * i + 1].color = D3DCOLOR_XRGB(255, 255, 255);
			pVertices[2 * i + 0].position = D3DXVECTOR3(((FLOAT)i / (VLINES_NUM - 1)) * 2.0f - 1.0f, -1.0f, 0.0f);
			pVertices[2 * i + 1].position = D3DXVECTOR3(((FLOAT)i / (VLINES_NUM - 1)) * 2.0f - 1.0f,  1.0f, 0.0f);
		}

		m_pVB->Unlock();
		hr = m_pd3dDevice->SetStreamSource(0, m_pVB, 0, sizeof(CUSTOMVERTEX));
		hr = m_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
		// устанавливаем нашу текстуру и её параметры
		hr = m_pd3dDevice->SetTexture(0, m_pTexture);
		hr = m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		hr = m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		hr = m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		hr = m_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		// Устанавливаем фильтрацию текстур
		hr = SetSamplerState();
		// задаём всякие преобразования матрицы.
		hr = SetupMatrices();
		return hr;
	}

	HRESULT CScreenD3D::SetupMatrices()
	{
		HRESULT hr;
		// Задаём матрицу мира, тут всю сцену можно вертеть как хочется, но нам это не надо.
//  D3DXMATRIXA16 matWorld;
//  D3DXMatrixIdentity( &matWorld );
//  D3DXMatrixRotationX( &matWorld, timeGetTime() / 1000.0f );
//  m_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );
		// вот вариант отображения двумерного вида, через ортогональную проекцию
		D3DXMATRIX  matProj, matView, mTrans;
		// Установка двухмерного представления и состояния визуализации
		D3DXMatrixIdentity(&matView);
		hr = m_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);
		// Установка ортогональной проекции, т.е двухмерная графика в трехмерном пространстве
		D3DXMatrixOrthoLH(&matProj, 1.0f, -1.0f, 0.0f, 1.0f);
		// и немного сдвинем текстуру. а то она почему-то сдвинута.
		D3DXMatrixTranslation(&mTrans, 0.5f, 0.501f, 0.0f);
		// Задание матрицы проецирования
		hr = m_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
		hr = m_pd3dDevice->SetTransform(D3DTS_TEXTURE0, &mTrans);

		hr = m_pd3dDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2;
		hr = m_pd3dDevice->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION);
		return hr;
	}

};

#endif // D3D
