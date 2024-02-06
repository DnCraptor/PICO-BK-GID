// COscillatorlView.cpp: файл реализации
//
#ifdef UI
#include "pch.h"
#include "OscillatorView.h"
#include "BKMessageBox.h"
#include "Config.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

// COscillatorlView

IMPLEMENT_DYNAMIC(COscillatorlView, CDockablePane)

COscillatorlView::COscillatorlView()
	: m_pBKOSC(nullptr)
	, m_nRenderType(CONF_OSCILLOSCOPE_RENDER::NONE)
{
}

COscillatorlView::~COscillatorlView()
    = default;

BEGIN_MESSAGE_MAP(COscillatorlView, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// обработчики сообщений COscillatorlView

int COscillatorlView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	switch (CreateOSC())
	{
		case 0:
			g_BKMsgBox.Show(IDS_BK_ERROR_NOCBKOSCSCRCREATE);

		case -1:
			return -1;
	}

	return 0;
}

void COscillatorlView::OnAfterFloat()
{
	CDockablePane::OnAfterFloat();
	AdjustLayout();
}

void COscillatorlView::OnAfterDockFromMiniFrame()
{
	CDockablePane::OnAfterDockFromMiniFrame();
	AdjustLayout();
}

void COscillatorlView::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void COscillatorlView::AdjustLayout()
{
	if (GetSafeHwnd() && m_pBKOSC)
	{
		CRect rectClient;
		GetClientRect(rectClient);
		int c = GetCaptionHeight();
		rectClient.bottom += c;
		m_pBKOSC->SetWindowPos(nullptr, 0, 0, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
	}

	CDockablePane::AdjustLayout();
}

BOOL COscillatorlView::OnEraseBkgnd(CDC *pDC)
{
	return FALSE;
	//return CDockablePane::OnEraseBkgnd(pDC);
}

void COscillatorlView::OnPaint()
{
	CPaintDC dc(this); // хоть эта штука и не используется в функции, но если её закомментировать, перерисовка перестанет работать.
	// Не вызывать CDockablePane::OnPaint() для сообщений рисования

	if (m_pBKOSC && IsWindowVisible())
	{
		m_pBKOSC->Invalidate(FALSE);
	}
}

// пересоздание объекта m_pBKOSC
bool COscillatorlView::ReCreateOSC()
{
	if (m_nRenderType != g_Config.m_nOscRenderType)
	{
		DeleteOSC();

		if (CreateOSC() != 1)
		{
			return false;
		}
	}

	return true;
}

void COscillatorlView::DeleteOSC()
{
	if (m_pBKOSC)
	{
		m_pBKOSC->DestroyWindow();
		m_pBKOSC.reset();
		m_nRenderType = CONF_OSCILLOSCOPE_RENDER::NONE;
	}
}


int COscillatorlView::CreateOSC()
{
	if (!m_pBKOSC)
	{
		m_nRenderType = CONF_OSCILLOSCOPE_RENDER::NONE;
		CRect rect;
		GetWindowRect(&rect);
		m_pBKOSC = std::make_unique<CBKOscScr>(g_Config.m_nOscRenderType);

		if (m_pBKOSC)
		{
			if (!m_pBKOSC->Create(nullptr, _T("Oscillator"), WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPSIBLINGS, CRect(0, 0, rect.Width(), rect.Height()), this, 0))
			{
				g_BKMsgBox.Show(IDS_BK_ERROR_NOCBKOSCSCRINIT);
				TRACE0("Не удалось создать объект Oscillator\n");
				return -1;      // не удалось создать
			}

			m_nRenderType = g_Config.m_nOscRenderType;
			AdjustLayout();
			return 1;
		}

		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}

	return 0;
}


void COscillatorlView::OnDestroy()
{
	DeleteOSC();
	CDockablePane::OnDestroy();
}
#endif