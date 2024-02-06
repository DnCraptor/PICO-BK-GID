
// BKOscScr.cpp : файл реализации
//
#ifdef UI
#include "pch.h"
#include "BKOscScr.h"
#include "Config.h"

#undef BKOSCDLL_EXPORTS

#include "BKOscScrOGL\BKOscScrOGL.h"
#include "BKOscScrD2D\BKOscScrD2D.h"
#include "BKOscScrGDI\BKOscScrGDI.h"

#include "BKMessageBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// диалоговое окно CBKOscScr
IMPLEMENT_DYNAMIC(CBKOscScr, CWnd)


#ifdef _WIN64
#define BK_OSCOGLDLLNAME _T("Dll\\BKOscScrOGL_x64.dll")
#define BK_OSCD2DDLLNAME _T("Dll\\BKOscScrD2D_x64.dll")
#define BK_OSCGDIDLLNAME _T("Dll\\BKOscScrGDI_x64.dll")
#else
#define BK_OSCOGLDLLNAME _T("Dll\\BKOscScrOGL.dll")
#define BK_OSCD2DDLLNAME _T("Dll\\BKOscScrD2D.dll")
#define BK_OSCGDIDLLNAME _T("Dll\\BKOscScrGDI.dll")
#endif

#define BK_OSCSCRDLLFUNC "GetBKOscScr"

CBKOscScr::CBKOscScr(CONF_OSCILLOSCOPE_RENDER nRenderType)
	: m_pSharedFunc(nullptr)
	, m_hModule(nullptr)
	, m_hwndScreen(nullptr)  // Screen View window handle
	, m_inBuf(nullptr)
	, m_inLen(0)
	, m_inLenByte(0)
{
	switch (nRenderType)// Берём номер из конфига. 0 - OGL, 1 - D2D, 2 - GDI+ (глючный)
	{
		default:
		case CONF_OSCILLOSCOPE_RENDER::OPENGL:
			m_strDllName = BK_OSCOGLDLLNAME;
			break;

		case CONF_OSCILLOSCOPE_RENDER::D2D:
			m_strDllName = BK_OSCD2DDLLNAME;
			break;

		case CONF_OSCILLOSCOPE_RENDER::GDI:
			m_strDllName = BK_OSCGDIDLLNAME;
			break;
	}

	m_hModule = ::LoadLibrary(m_strDllName);

	if (m_hModule)
	{
		auto pGetBKOSC = reinterpret_cast<GETBKOSC>(::GetProcAddress(m_hModule, BK_OSCSCRDLLFUNC));

		if (pGetBKOSC)
		{
			switch (nRenderType)// Берём номер из конфига. 0 - OGL, 1 - D2D, 2 - GDI+ (глючный)
			{
				default:
				case CONF_OSCILLOSCOPE_RENDER::OPENGL:
					m_pSharedFunc = dynamic_cast<CBKOscScrOGL *>(pGetBKOSC());
					break;

				case CONF_OSCILLOSCOPE_RENDER::D2D:
					m_pSharedFunc = dynamic_cast<CBKOscScrD2D *>(pGetBKOSC());
					break;

				case CONF_OSCILLOSCOPE_RENDER::GDI:
					m_pSharedFunc = dynamic_cast<CBKOscScrGDI *>(pGetBKOSC());
					break;
			}
		}
		else
		{
			CString str;
			str.Format(IDS_BK_ERROR_SCRDLLFUNCERR, _T(BK_OSCSCRDLLFUNC));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		CString str;
		str.Format(IDS_BK_ERROR_SCRDLLINITERR, m_strDllName);
		g_BKMsgBox.Show(str, MB_OK);
	}
}

CBKOscScr::~CBKOscScr()
{
	if (m_pSharedFunc)
	{
		delete m_pSharedFunc;
	}

	if (m_hModule)
	{
		::FreeLibrary(m_hModule);
	}
}

BEGIN_MESSAGE_MAP(CBKOscScr, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
END_MESSAGE_MAP()


// обработчики сообщений CBKOscScr


int CBKOscScr::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	if (m_pSharedFunc == nullptr)
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_SCRDLLFUNCPTRERR, MB_OK);
		return -1;
	}

	m_hwndScreen = GetSafeHwnd();

	if (m_hwndScreen != nullptr)
	{
		if (SUCCEEDED(m_pSharedFunc->BKOSC_Screen_Init(this)))
		{
			return TRUE;  // возврат значения TRUE, если фокус не передан элементу управления
		}
	}

	CString str;
	str.Format(IDS_BK_ERROR_SCRDLLINITERR, m_strDllName);
	g_BKMsgBox.Show(str, MB_OK);
	m_pSharedFunc->BKOSC_Screen_Done();
	return -1;
}

void CBKOscScr::OnDestroy()
{
	// перед выходом, дождёмся прекращения рисования
	{
		std::lock_guard<std::mutex> lk(m_lockBusy);
		// и заблокируем рисование во избежание потенциальных дедлоков
		m_pSharedFunc->BKOSC_Screen_Done();
	}
	CWnd::OnDestroy();
}


void CBKOscScr::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: добавьте свой код обработчика сообщений
	// Не вызывать CBKOscScr::OnPaint() для сообщений рисования
	std::lock_guard<std::mutex> lk(m_lockBusy);
	m_pSharedFunc->BKOSC_OnDisplay(m_inBuf.get(), m_inLen);
}

void CBKOscScr::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	std::lock_guard<std::mutex> lk(m_lockBusy);
	m_pSharedFunc->BKOSC_OnSize(cx, cy);
}

BOOL CBKOscScr::OnEraseBkgnd(CDC *pDC)
{
	return TRUE;
}


void CBKOscScr::SetBuffer(int buffer_len_in_samples)
{
	if (m_inLen != buffer_len_in_samples)
	{
		std::lock_guard<std::mutex> lk(m_lockBusy);
		m_inLen = buffer_len_in_samples; // длина буфера в сэмплах
		m_inLenByte = buffer_len_in_samples * SAMPLE_INT_BLOCKALIGN; // длина буфера в байтах

		if (m_inBuf) // ранее созданный массив надо удалить, если он был
		{
			m_inBuf.reset();
		}

		m_inBuf = std::make_unique<SAMPLE_INT[]>(buffer_len_in_samples * BUFFER_CHANNELS); // новый массив новой длины

		if (m_inBuf == nullptr)
		{
			g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
		}

		m_pSharedFunc->BKOSC_OnSetBuffer(m_inLenByte);
	}
}

void CBKOscScr::FillBuffer(SAMPLE_INT *inBuf)
{
	std::lock_guard<std::mutex> lk(m_lockBusy);
	memcpy(m_inBuf.get(), inBuf, m_inLenByte);
}

#endif