
// BKView.cpp : реализация класса CBKView
//
#ifdef UI
#include "pch.h"
// SHARED_HANDLERS можно определить в обработчиках фильтров просмотра реализации проекта ATL, эскизов
// и поиска; позволяет совместно использовать код документа в данным проекте.
#ifndef SHARED_HANDLERS
#include "BK.h"
#endif

#include "BKDoc.h"
#include "BKView.h"
#include "MainFrm.h"
#include "Screen.h"
#include "PrintDlg.h"
#include "BKMessageBox.h"
#include "Config.h"

#include "Board.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef _DEBUG
// при отладке полезно выключать хук, а то очень тормозит при трассировке
// главное - не забыть включить обратно
// #define SET_KBD_HOOK 1
#else
// в релизной версии обязательно должно быть включено. А то блин склероз.
#define SET_KBD_HOOK 1
#endif
// CBKView

IMPLEMENT_DYNCREATE(CBKView, CView)

BEGIN_MESSAGE_MAP(CBKView, CView)
	// Стандартные команды печати
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CBKView::OnFilePrintPreview)
	ON_COMMAND(ID_FILE_CUSTOM_PRINT, &CBKView::OnCustomFilePrint)
	ON_MESSAGE(WM_VKBD_DOWN, &CBKView::OnVirtualKeyDown)
	ON_MESSAGE(WM_VKBD_UP, &CBKView::OnVirtualKeyUp)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_TIMER()
	ON_WM_CONTEXTMENU()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_SYSKEYDOWN()
	ON_WM_SYSKEYUP()
END_MESSAGE_MAP()

// создание/уничтожение CBKView

CBKView::CBKView()
	: m_pScreen(nullptr)
	, m_bPrintScreen(true)
	, m_bPrintInverse(true)
	, m_nRenderType(CONF_SCREEN_RENDER::NONE)
	, m_nCurrentCapsState(CAPS_STATE::UNINIT)
{
	m_kprs.clear();
	m_strPrintTitle = CString(MAKEINTRESOURCE(IDS_PRINT_TITLE));
}

CBKView::~CBKView()
{
	DeleteSCR();
}

BOOL CBKView::PreCreateWindow(CREATESTRUCT &cs)
{
	// TODO: изменить класс Window или стили посредством изменения
	// CREATESTRUCT cs
	cs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_SAVEBITS,
	                                   ::LoadCursor(nullptr, IDC_ARROW), HBRUSH(::GetStockObject(GRAY_BRUSH)), nullptr);
	return CView::PreCreateWindow(cs);
}


// рисование CBKView

void CBKView::OnDraw(CDC * /*pDC*/)
{
	CBKDoc *pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (!pDoc)
	{
		return;
	}

	DrawScreen();
}


inline void CBKView::DrawScreen()
{
	// эта функция почему-то работает гораздо хреновее если её вызывать непосредственно, чем вызов OnDraw
	// и лучше её не использовать
	if (IsWindowVisible())
	{
		m_pScreen->DrawScreen(true);
	}
}

// печать CBKView

void CBKView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL CBKView::OnPreparePrinting(CPrintInfo *pInfo)
{
	// подготовка по умолчанию
	return DoPreparePrinting(pInfo);
}

void CBKView::OnBeginPrinting(CDC * /*pDC*/, CPrintInfo * /*pInfo*/)
{
	// TODO: добавьте дополнительную инициализацию перед печатью
}

void CBKView::OnEndPrinting(CDC * /*pDC*/, CPrintInfo * /*pInfo*/)
{
	// TODO: добавьте очистку после печати
}

HHOOK CBKView::m_hHook = nullptr;   // handle to the hook procedure
bool CBKView::m_bAppFocused = false; // флаг что фокус на окне, чтобы если окно не в фокусе, кнопки WIN не перехватывались
static CBKView *g_pthisObj = nullptr; // Указатель на объект this, чтобы можно было вызвать нестатические функции из статической
LRESULT CALLBACK CBKView::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	auto lp = (KBDLLHOOKSTRUCT *)lParam;

	if (nCode < 0)
	{
		return CallNextHookEx(m_hHook, nCode, wParam, lParam);
	}

	if (m_bAppFocused)
	{
		if (nCode == HC_ACTION)
		{
			switch (wParam)
			{
				case WM_KEYDOWN:
				case WM_SYSKEYDOWN:
					if (lp->vkCode == VK_LWIN || lp->vkCode == VK_RWIN)
					{
						g_pthisObj->EmulateKeyDown(lp->vkCode, lp->flags);
						return 1;
					}

					if ((lp->vkCode == VK_ESCAPE) && ((lp->flags & LLKHF_ALTDOWN) != 0))
					{
						// АР2+КТ
						g_pthisObj->EmulateKeyDown(lp->vkCode, lp->flags);
						return 1;
					}

					if ((lp->vkCode == VK_ESCAPE) && ((GetKeyState(VK_CONTROL) & 0x8000) != 0))
					{
						// СУ+КТ
						g_pthisObj->EmulateKeyDown(lp->vkCode, lp->flags);
						return 1;
					}

					if ((lp->vkCode == VK_F4) && ((lp->flags & LLKHF_ALTDOWN) != 0))
					{
						// АР2+F4
						g_pthisObj->EmulateKeyDown(lp->vkCode, lp->flags);
						return 1;
					}

					break;

				case WM_KEYUP:
				case WM_SYSKEYUP:
					if (lp->vkCode == VK_LWIN || lp->vkCode == VK_RWIN)
					{
						g_pthisObj->EmulateKeyUp(lp->vkCode, lp->flags);
						return 1;
					}

					if ((lp->vkCode == VK_ESCAPE) && ((lp->flags & LLKHF_ALTDOWN) != 0))
					{
						// АР2+КТ
						g_pthisObj->EmulateKeyUp(lp->vkCode, lp->flags);
						return 1;
					}

					if ((lp->vkCode == VK_ESCAPE) && ((GetKeyState(VK_CONTROL) & 0x8000) != 0))
					{
						// СУ+КТ
						g_pthisObj->EmulateKeyUp(lp->vkCode, lp->flags);
						return 1;
					}

					if ((lp->vkCode == VK_F4) && ((lp->flags & LLKHF_ALTDOWN) != 0))
					{
						// АР2+F4
						g_pthisObj->EmulateKeyUp(lp->vkCode, lp->flags);
						return 1;
					}

					break;
			}
		}
	}

	return CallNextHookEx(m_hHook, nCode, wParam, lParam);
}


// CBKVKBDView *CBKView::GetKBDView()
// {
//  auto mw = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame());
//  return mw->GetBKVKBDViewPtr();
// }

int CBKView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
	{
		ASSERT(false);
		return -1;
	}

#ifdef SET_KBD_HOOK
	// ставим хук для перехвата кнопок левый, правый WIN и прочие системные кнопки
	g_pthisObj = this;
	m_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)&LowLevelKeyboardProc, AfxGetInstanceHandle() /*nullptr*/, 0);
	ASSERT(m_hHook != nullptr);
#endif // SET_KBD_HOOK
	SetTimer(BKTIMER_MOUSE, 10, nullptr);  // запустить таймер для мыши
	int ret = CreateSCR();
	return ret;
}


bool CBKView::ReCreateSCR()
{
	if (m_nRenderType != g_Config.m_nScreenRenderType)
	{
		DeleteSCR();
		return (CreateSCR() == 0);
	}

	return true;
}

int CBKView::CreateSCR()
{
	if (!m_pScreen)
	{
		m_nRenderType = CONF_SCREEN_RENDER::NONE;
		m_pScreen = std::make_unique<CScreen>(g_Config.m_nScreenRenderType);

		if (!m_pScreen)
		{
			g_BKMsgBox.Show(IDS_BK_ERROR_NOCSCREENCREATE);
			TRACE0("Не удалось создать класс CScreen\n");
			return -1;
		}

		CPoint svw = m_pScreen->GetScreenViewport();
		int nPixelW = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSX);
		int nPixelH = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSY);

		if (!m_pScreen->Create(nullptr, _T("BKScreen"), WS_VISIBLE | WS_CHILD, CRect(0, 0, ::MulDiv(svw.x, nPixelW, DEFAULT_DPIX), ::MulDiv(svw.y, nPixelH, DEFAULT_DPIY)), this, 0))
		{
			g_BKMsgBox.Show(IDS_BK_ERROR_NOCSCREENINIT);
			TRACE0("Не удалось создать панель Screen View\n");
			return -1;      // не удалось создать
		}

		m_nRenderType = g_Config.m_nScreenRenderType;
		auto pMF = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame());

		if (pMF)
		{
			pMF->SetScreen(GetScreen(), &m_debugger);
		}

		m_pScreen->AdjustLayout(nPixelW, nPixelH);
	}

	return 0;
}

void CBKView::DeleteSCR()
{
	if (m_pScreen)
	{
		m_pScreen->DestroyWindow();

		if (m_pScreen)
		{
			m_pScreen.reset();
		}

		m_nRenderType = CONF_SCREEN_RENDER::NONE;
	}
}


void CBKView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	int nPixelW = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSX);
	int nPixelH = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSY);
	m_pScreen->AdjustLayout(nPixelW, nPixelH);
	CRect rect;
	m_pScreen->GetClientRect(&rect);
	int w = rect.Width(), h = rect.Height();

	if (w && h)
	{
		GetParentFrame()->SendMessage(WM_SCREENSIZE_CHANGED, w, h);
	}
}

void CBKView::OnContextMenu(CWnd * /* pWnd */, CPoint point)
{
	if (!g_Config.m_bMouseMars)
	{
#ifndef SHARED_HANDLERS
		theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_SCRPARAM, point.x, point.y, this, FALSE);
#endif
	}
}


// диагностика CBKView

#ifdef _DEBUG
void CBKView::AssertValid() const
{
	CView::AssertValid();
}

void CBKView::Dump(CDumpContext &dc) const
{
	CView::Dump(dc);
}

CBKDoc *CBKView::GetDocument() const // встроена неотлаженная версия
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CBKDoc)));
	return (CBKDoc *)m_pDocument;
}
#endif // _DEBUG


// обработчики сообщений CBKView


void CBKView::OnCustomFilePrint()
{
	auto pdlg = std::make_unique<CPrintDlg>(FALSE);

	if (pdlg)
	{
		pdlg->m_pDebugger = &m_debugger;
		pdlg->m_strTitle = m_strPrintTitle;
		pdlg->m_bPrintScreen = m_bPrintScreen;
		pdlg->m_bInverse = m_bPrintInverse;
		pdlg->m_startAddr = g_Config.m_nDisasmAddr;
		pdlg->m_endAddr = m_debugger.GetBottomAddress();

		if (pdlg->DoModal() == IDOK)
		{
			m_strPrintTitle = pdlg->m_strTitle;
			m_bPrintScreen = pdlg->m_bPrintScreen != 0;
			m_bPrintInverse = pdlg->m_bInverse != 0;
			Print(pdlg.get());
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}

	auto pMF = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame());

	if (pMF)
	{
		pMF->SetFocusToBK();
	}
}




void CBKView::Print(CPrintDlg *pPrintDlg)
{
	uint16_t startAddress = pPrintDlg->m_startAddr;
	uint16_t endAddress   = pPrintDlg->m_endAddr;
	auto pdc = std::make_unique<CDC>();

	if (pdc)
	{
		// Attach a printer DC
		pdc->Attach(pPrintDlg->GetPrinterDC());
		pdc->m_bPrinting = TRUE;
		// Initialize print document details
		DOCINFO di;
		ZeroMemory(&di, sizeof(DOCINFO));
		di.cbSize = sizeof(DOCINFO);
		di.lpszDocName = m_strPrintTitle;
		// Begin a new print job
		BOOL bPrintingOK = pdc->StartDoc(&di);
		// Get the printing extents and store in the m_rectDraw field of a
		// CPrintInfo object
		CPrintInfo Info;

		if (m_bPrintScreen)
		{
			Info.SetMaxPage(1);
		}
		else
		{
			Info.SetMaxPage(pPrintDlg->m_nPages);
		}

		Info.m_rectDraw.SetRect(0, 0, pdc->GetDeviceCaps(HORZRES), pdc->GetDeviceCaps(VERTRES));

		// OnBeginPrinting (&dc, &Info);
		for (UINT page = Info.GetMinPage(); page <= Info.GetMaxPage() && bPrintingOK; page++)
		{
			// begin new page
			pdc->StartPage();
			Info.m_nCurPage = page;
			startAddress = PrintPage(pdc.get(), &Info, startAddress, endAddress);
			bPrintingOK = (pdc->EndPage() > 0);  // end page
		}

		// OnEndPrinting (&dc, &Info);
		if (bPrintingOK)
		{
			// end a print job
			pdc->EndDoc();
		}
		else
		{
			// abort job.
			pdc->AbortDoc();
		}

		// detach the printer DC
		pdc->Detach();
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}
}


uint16_t CBKView::PrintPage(CDC *pDC, CPrintInfo *pInfo, uint16_t currAddress, uint16_t endAddress)
{
	// Get paper sizes
	int width = pInfo->m_rectDraw.Width();
	int height = pInfo->m_rectDraw.Height();
	int lines_per_page = PRINT_LINES_PER_PAGE; // Lines per page
	int fnt_height = height / lines_per_page;
	int tabs = width / PRINT_TAB_LENGTH;
	CPen pen;
	pen.CreatePen(PS_SOLID, 10, RGB(0, 0, 0));
	CFont fnt;
	fnt.CreatePointFont(fnt_height, _T("Arial"), pDC);
	CFont *pOld = pDC->SelectObject(&fnt);
	CPen *pOldPen = pDC->SelectObject(&pen);
	pDC->SetTextColor(RGB(0, 0, 0));
	pDC->SetBkColor(RGB(255, 255, 255));

	if (m_bPrintScreen)
	{
		// Print Screenshot
		auto pdc = std::make_unique<CDC>();

		if (pdc)
		{
			pdc->CreateCompatibleDC(pDC);
			// Get Screenshot handle
			CPoint svw = m_pScreen->GetScreenViewport();
			auto hScreenShot = (HBITMAP)CopyImage(m_pScreen->GetScreenshot(), IMAGE_BITMAP, svw.x, svw.y, LR_COPYDELETEORG);

			if (hScreenShot)
			{
				BITMAP bm{};
				GetObject(hScreenShot, sizeof(BITMAP), &bm);
				// Calculate print rectangle
				int len_w = (width < height) ? width : height;
				len_w = len_w * 10 / 12;
				int len_h = len_w * 3 / 4; // делаем пропорции 4/3
				int xOfs = (pInfo->m_rectDraw.Width() - len_w) / 2;
				int yOfs = (pInfo->m_rectDraw.Height() - len_h) / 2;
				// Stretch Screenshot to Print page
				int stretchMode = m_bPrintInverse ? SRCINVERT : SRCCOPY;
				HGDIOBJ obj = pdc->SelectObject(hScreenShot);
				pDC->SetStretchBltMode(HALFTONE);
				pDC->StretchBlt(xOfs, yOfs, len_w, len_h, pdc.get(), 0, 0, bm.bmWidth, bm.bmHeight, stretchMode);
				pdc->SelectObject(obj);
				DeleteObject(hScreenShot);
			}
			else
			{
				g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
			}

			pdc->DeleteDC();
		}
		else
		{
			g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
		}
	}
	else
	{
		for (int line = 3; line < lines_per_page - 3; line++)
		{
			CRect rcString(0, line * fnt_height, width, line * fnt_height + fnt_height);
			CString strInstr;
			uint16_t codes[8];
			const int nLen = m_debugger.DebugInstruction(currAddress, strInstr, codes);

			// у strInstr надо удалить маркеры колоризации.
			for (;;)
			{
				int pos = strInstr.Find(_T(COLORED_TAG), 0); // находим первый попавшийся маркер

				if (pos < 0)
				{
					break;
				}

				int len = strInstr.GetLength();
				strInstr = strInstr.Left(pos) + strInstr.Right(len - pos - (COLORED_TAG_LENGTH + 1));
			}

			CString str;
			str.Format(_T("%07o  |"), currAddress);
			rcString.left = tabs * 1;
			pDC->DrawText(str, &rcString, DT_LEFT | DT_EXPANDTABS);
			rcString.left = tabs * 2;
			pDC->DrawText(strInstr, &rcString, DT_LEFT | DT_EXPANDTABS);
			// Добавим комментарий. Это у нас просто машинные инструкции ассемблерной команды
			str = _T("  |") + Global::WordToOctString(codes[0]); // код инструкции у нас по любому всегда есть

			// а дальше от 0 до 6-х слов аргументов
			// для инструкций FIS если регистр PC, то 4 слова аргументов
			for (int i = 1; i < nLen; ++i)
			{
				str += _T("   ") + Global::WordToOctString(codes[i]);
			}

			rcString.left = tabs * 6;
			pDC->DrawText(str, &rcString, DT_LEFT | DT_EXPANDTABS);
			currAddress += nLen * 2;

			if (currAddress >= endAddress)
			{
				break;
			}
		}
	}

	pDC->MoveTo(0, 3 * fnt_height - fnt_height / 2);
	pDC->LineTo(width, 3 * fnt_height - fnt_height / 2);
	pDC->MoveTo(0, (lines_per_page - 3) * fnt_height + fnt_height / 2);
	pDC->LineTo(width, (lines_per_page - 3) * fnt_height + fnt_height / 2);
	CRect rcString;
	rcString.SetRect(0, fnt_height, width, fnt_height + fnt_height);
	CString strTitle(MAKEINTRESOURCE(IDS_EMUL_TITLE));
	int oldMode = pDC->SetBkMode(TRANSPARENT);
	pDC->DrawText(strTitle, &rcString, DT_LEFT | DT_EXPANDTABS);
	pDC->DrawText(m_strPrintTitle, &rcString, DT_RIGHT | DT_EXPANDTABS);
	pDC->SetBkMode(oldMode);
	pDC->SetTextColor(RGB(255, 255, 255));
	pDC->SetBkColor(RGB(0, 0, 0));
	rcString.SetRect(0, (lines_per_page - 2) * fnt_height, width, (lines_per_page - 2) * fnt_height + fnt_height);
	CString strPages;
	strPages.Format(IDS_PRINT_PAGES, pInfo->m_nCurPage, pInfo->GetMaxPage());
	pDC->PatBlt(rcString.left, rcString.top, rcString.Width(), rcString.Height(), BLACKNESS);
	pDC->DrawText(strPages, &rcString, DT_RIGHT | DT_EXPANDTABS);
	pDC->SelectObject(pOld);
	pDC->SelectObject(pOldPen);
	return currAddress;
}


void CBKView::OnInitialUpdate()
{
	CView::OnInitialUpdate();
	auto mw = GetParentFrame();

	if (mw)
	{
		mw->PostMessage(WM_START_PLATFORM);
	}
}

void CBKView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	auto mw = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame());
	auto board = mw->GetBoard();
	auto vkbdvw = mw->GetBKVKBDViewPtr();

	if (nChar == VK_MENU) // если нажат Ctrl, то нажатие alt отлавливается тут и только тут
	{
		// uint16_t uScan = HIWORD(pMsg->lParam) & 0x1FF; // это чтобы различать левый/правый альты, если uScan == 56, то левый альт, иначе - правый.
		// если nFlags & 0x100 == 0, то левый альт, если != 0 то правый
		vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_AR2, true);
		vkbdvw->Invalidate(FALSE); // vkbdvw->RedrawWindow();
	}
	else if (board)
	{
		// эмуляция джойстика
		// если условия для джойстика совпадут, то клавиатура не эмулируется,
		// иначе - обрабатывается эмуляция клавиатуры
		if (g_Config.m_bJoystick && !(nFlags & KF_EXTENDED))
		{
			volatile uint16_t &joystick = board->m_reg177714in;

			for (auto &jp : g_Config.m_arJoystick)
			{
				if (nChar == jp.nVKey)
				{
					joystick |= jp.nMask;
					TRACE("Joystick Set Mask\n");
					return;
				}
			}
		}

		EmulateKeyDown(nChar, nFlags);
	}
}


void CBKView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	auto mw = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame());
	auto board = mw->GetBoard();
	auto vkbdvw = mw->GetBKVKBDViewPtr();

	if (nChar == VK_MENU) // после нажатия alt+клавиша, отжатие alt отлавливается тут и только тут
	{
		// uint16_t uScan = HIWORD(pMsg->lParam) & 0x1FF; // это чтобы различать левый/правый альты, если uScan == 56, то левый альт, иначе - правый.
		// если nFlags & 0x100 == 0, то левый альт, если != 0 то правый
		vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_AR2, false);
		vkbdvw->Invalidate(FALSE); // vkbdvw->RedrawWindow();
	}
	else if (board)
	{
		// эмуляция джойстика
		// если условия для джойстика совпадут, то клавиатура не эмулируется,
		// иначе - обрабатывается эмуляция клавиатуры
		if (g_Config.m_bJoystick && !(nFlags & KF_EXTENDED))
		{
			volatile uint16_t &joystick = board->m_reg177714in;

			for (auto &jp : g_Config.m_arJoystick)
			{
				if (nChar == jp.nVKey)
				{
					joystick &= ~jp.nMask;
					TRACE("Joystick Clear Mask\n");
					return;
				}
			}
		}

		EmulateKeyUp(nChar, nFlags);
	}
}

void CBKView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	auto mw = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame());
	auto vkbdvw = mw->GetBKVKBDViewPtr();

	// Если нажали любой Alt
	if (nChar == VK_MENU)
	{
		// uint16_t uScan = HIWORD(pMsg->lParam) & 0x1FF; // это чтобы различать левый/правый альты, если uScan == 56, то левый альт, иначе - правый.
		// если nFlags & 0x100 == 0, то левый альт, если != 0 то правый
		vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_AR2, true);
		vkbdvw->Invalidate(FALSE); // vkbdvw->RedrawWindow();
	}
	else if (mw->GetBoard())
	{
		EmulateKeyDown(nChar, nFlags);
	}
}

void CBKView::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	auto mw = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame());
	auto vkbdvw = mw->GetBKVKBDViewPtr();

	if (nChar == VK_MENU) // после нажатия alt, отжатие alt отлавливается тут и только тут, если не нажимали при этом других клавиш
	{
		// uint16_t uScan = HIWORD(pMsg->lParam) & 0x1FF; // это чтобы различать левый/правый альты, если uScan == 56, то левый альт, иначе - правый.
		// если nFlags & 0x100 == 0, то левый альт, если != 0 то правый
		vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_AR2, false);
		vkbdvw->Invalidate(FALSE); // vkbdvw->RedrawWindow();
	}
	else if (mw->GetBoard())
	{
		EmulateKeyUp(nChar, nFlags);
	}
}

void CBKView::EmulateKeyDown(UINT nChar, UINT nFlags)
{
	auto mw = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame());
	auto board = mw->GetBoard();
	auto vkbdvw = mw->GetBKVKBDViewPtr();

	switch (nChar)
	{
		case VK_PAUSE:      // Если нажали СТОП
		case VK_F12:
			board->StopInterrupt(); // нажали на кнопку стоп
			break;

		case VK_SHIFT:      // Если нажали Шифт
			vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_SHIFT, true);
			vkbdvw->Invalidate(FALSE); // vkbdvw->RedrawWindow();
			break;

		case VK_CONTROL:    // Если нажали СУ (Любой Ctrl)
			// если nFlags & 0x100 == 0, то левый ctrl, если != 0 то правый
			vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_SU, true);
			vkbdvw->Invalidate(FALSE); // vkbdvw->RedrawWindow();
			// вот ещё так можно определять
//          GetAsyncKeyState(VK_LCONTROL); // <0 - нажато, >=0 - нет; ret&1 - кнопка нажата после предыдущего вызова GetAsyncKeyState
//          GetAsyncKeyState(VK_RCONTROL);
			break;

		/* кнопки Загл и Стр будем эмулировать капслоком. капслок включён - стр включено, выключен - загл включено, или наоборот */
		case VK_CAPITAL:
		{
			bool b = !!(GetKeyState(VK_CAPITAL) & 1);
			m_nCurrentCapsState = b ? CAPS_STATE::PRESSED : CAPS_STATE::UNPRESSED; // сохраним состояние клавиши Капслок
			vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_CAPS, b);
		}
			break;

		default:
		{
			// Запишем код клавиши в регистр 177662
			uint16_t nScanCode = 0;
			uint16_t nInt = 0;
			bool bSuccess = vkbdvw->TranslateKey(nChar, !!(nFlags & KF_EXTENDED), &nScanCode, &nInt);

			if (bSuccess) // если скан код верный
			{
				// uint8_t nUnique = vkbdvw->GetUniqueKeyNum(nScanCode);
				UINT nUnique = nChar;

				// проверяем, зажали мы клавишу и держим её нажатой?
				if (AddKeyToKPRS(nUnique)) // такая клавиша уже нажата
				{
					TRACE3("key %d (char %d) already pushed, pressed chars: %d\n", nUnique, nScanCode, m_kprs.vKeys.size());
					bSuccess = !g_Config.m_bBKKeyboard; // если выключена эмуляция, то обрабатывать
				}
				else // если нет, то это новая нажатая клавиша
				{
					TRACE3("push key %d (char %d), pressed chars: %d\n", nUnique, nScanCode, m_kprs.vKeys.size());
					bSuccess = !m_kprs.bKeyPressed; // ПКшный автоповтор делать?
				}

				if (bSuccess) // условие обработки выполняется?
				{
					m_kprs.bKeyPressed = g_Config.m_bBKKeyboard; // отключаем автоповтор, как на реальной клавиатуре БК
					TRACE2("processing key %d (char %d)\n", nUnique, nScanCode);

					board->m_reg177662in = nScanCode & 0177;
					// если ещё прошлый код не прочитали, новый игнорируем.
					if (!(board->m_reg177660 & 0200))
					{
						// сюда заходим только если прочитан прошлый код
						board->KeyboardInterrupt((vkbdvw->GetAR2Status()) ? INTERRUPT_274 : nInt);
					}

					// Установим в регистре 177716 флаг нажатия клавиши
					board->m_reg177716in &= ~0100;
				}
			}
		}
	}
}

bool CBKView::AddKeyToKPRS(const uint8_t nUnique)
{
	// посмотрим в массиве
	for (auto &vKey : m_kprs.vKeys)
	{
		if (vKey == nUnique) // если такой код уже есть
		{
			return true;    // значит мы эту клавишу уже нажали и ещё не отпустили
		}
	}

	// такого кода в массиве нету,
	m_kprs.vKeys.emplace_back(nUnique); // добавляем
	return false;
}

bool CBKView::DelKeyFromKPRS(const uint8_t nUnique)
{
	if (!m_kprs.vKeys.empty())
	{
		for (auto p = m_kprs.vKeys.begin(); p != m_kprs.vKeys.end(); ++p)
		{
			if (*p == nUnique)
			{
				m_kprs.vKeys.erase(p);
				return true;
			}
		}
	}

	return false;
}


void CBKView::EmulateKeyUp(UINT nChar, UINT nFlags)
{
	auto mw = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame());
	auto board = mw->GetBoard();
	auto vkbdvw = mw->GetBKVKBDViewPtr();

	switch (nChar)
	{
		case VK_PAUSE:
		case VK_F12:
			board->UnStopInterrupt(); // отжали кнопку стоп
			break;

		case VK_SHIFT:
			vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_SHIFT, false);
			vkbdvw->Invalidate(FALSE); // vkbdvw->RedrawWindow();
			break;

		case VK_CONTROL:
			vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_SU, false);
			vkbdvw->Invalidate(FALSE); // vkbdvw->RedrawWindow();
			break;

		default:
		{
			uint16_t nScanCode = 0;
			uint16_t nInt = 0;
			bool bSuccess = vkbdvw->TranslateKey(nChar, !!(nFlags & KF_EXTENDED), &nScanCode, &nInt);

			if (bSuccess)
			{
				// uint8_t nUnique = vkbdvw->GetUniqueKeyNum(nScanCode);
				UINT nUnique = nChar;

				switch (nScanCode)
				{
					case BKKEY_LAT:
						vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_XLAT, false);
						break;

					case BKKEY_RUS:
						vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_XLAT, true);
						break;
				}

				if (DelKeyFromKPRS(nUnique))
				{
					TRACE3("pop key %d (char %d), pressed chars: %d\n", nUnique, nScanCode, m_kprs.vKeys.size());
				}

				if (m_kprs.vKeys.empty())
				{
					TRACE0("unhit all keys!\n");
					// Установим в регистре 177716 флаг отпускания клавиши
					board->m_reg177716in |= 0100;
					m_kprs.bKeyPressed = false;
				}
			}
		}
	}
}


/*
*wParam - флаг клавиши BKKeyType
*Потенциально можно передавать и остальные управляющие клавиши, для отображения статуса.
*Нужно только подумать, как сделать так, чтобы не конфликтовало с обычной клавиатурой.
*lParam - если обычная клавиша, то мл.байт - сканкод, ст.байт - прерывание.
**/
LRESULT CBKView::OnVirtualKeyDown(WPARAM wParam, LPARAM lParam)
{
	auto mw = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame());

	if (mw)
	{
		auto board = mw->GetBoard();
		auto vkbdvw = mw->GetBKVKBDViewPtr();

		switch (static_cast<BKKeyType>(wParam))
		{
			case BKKeyType::REGULAR:
			{
				uint8_t nScanCode = LOBYTE(lParam);
				uint8_t nInt = HIBYTE(lParam);
				bool bSuccess = false;
				uint8_t nUnique = (lParam >> 16) & 0xff;

				// проверяем, зажали мы клавишу и держим её нажатой?
				if (!AddKeyToKPRS(nUnique))   // если нет, то это новая нажатая клавиша
				{
					TRACE1("push vkbd char %d\n", nScanCode);
					bSuccess = !m_kprs.bKeyPressed; // ПКшный автоповтор делать?
				}
				else // такая клавиша уже нажата
				{
					bSuccess = !g_Config.m_bBKKeyboard; // если выключена эмуляция, то обрабатывать
				}

				if (bSuccess) // условие обработки выполняется?
				{
					m_kprs.bKeyPressed = g_Config.m_bBKKeyboard; // отключаем автоповтор, как на реальной клавиатуре БК
					TRACE1("processing vkbd char %d\n", nScanCode);

					board->m_reg177662in = nScanCode & 0177;
					// если ещё прошлый код не прочитали, новый игнорируем.
					if (!(board->m_reg177660 & 0200))
					{
						// сюда заходим только если прочитан прошлый код
						board->KeyboardInterrupt(nInt);
					}

					// Установим в регистре 177716 флаг нажатия клавиши
					board->m_reg177716in &= ~0100;
				}
			}
			break;

			case BKKeyType::LSHIFT:
			case BKKeyType::RSHIFT:
				vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_SHIFT, true);
				break;

			case BKKeyType::CTRL:
				vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_SU, true);
				break;

			case BKKeyType::ALT:
				vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_AR2, true);
				break;

			case BKKeyType::ZAGL:
				CapsLockOn();
				m_nCurrentCapsState = CAPS_STATE::PRESSED;
				break;

			case BKKeyType::STR:
				CapsLockOff();
				m_nCurrentCapsState = CAPS_STATE::UNPRESSED;
				break;

			case BKKeyType::STOP:
				board->StopInterrupt(); // нажали на кнопку стоп
				break;
		}
	}

	return S_OK;
}

void CBKView::CapsLockOn()
{
	if (!(GetKeyState(VK_CAPITAL) & 1)) // если капслок выключен
	{
		// включить
		keybd_event(VK_CAPITAL, 0, KEYEVENTF_EXTENDEDKEY | 0, 0);               // Нажать и отпустить клавишу
		keybd_event(VK_CAPITAL, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0); //
		// Это вызовет событие OnKeyDown и обработается там
	}
}

void CBKView::CapsLockOff()
{
	if (GetKeyState(VK_CAPITAL) & 1) // если капслок включён
	{
		// выключить
		keybd_event(VK_CAPITAL, 0, KEYEVENTF_EXTENDEDKEY | 0, 0);               // Нажать и отпустить клавишу
		keybd_event(VK_CAPITAL, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0); //
		// Это вызовет событие OnKeyDown и обработается там
	}
}


LRESULT CBKView::OnVirtualKeyUp(WPARAM wParam, LPARAM lParam)
{
	auto mw = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame());

	if (mw)
	{
		auto board = mw->GetBoard();
		auto vkbdvw = mw->GetBKVKBDViewPtr();

		switch (static_cast<BKKeyType>(wParam))
		{
			case BKKeyType::REGULAR:
			{
				uint8_t nScanCode = LOBYTE(lParam);
				uint8_t nUnique = (lParam >> 16) & 0xff;

				switch (nScanCode)
				{
					case BKKEY_LAT:
						vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_XLAT, false);
						break;

					case BKKEY_RUS:
						vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_XLAT, true);
						break;
				}

				if (DelKeyFromKPRS(nUnique))
				{
					TRACE1("pop vkbd char %d\n", nScanCode);
				}

				if (m_kprs.vKeys.empty())
				{
					TRACE0("unhit all keys!\n");
					// Установим в регистре 177716 флаг отпускания клавиши
					board->m_reg177716in |= 0100;
					m_kprs.bKeyPressed = false;
				}
			}
			break;

			case BKKeyType::LSHIFT:
			case BKKeyType::RSHIFT:
				vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_SHIFT, false);
				break;

			case BKKeyType::CTRL:
				vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_SU, false);
				break;

			case BKKeyType::ALT:
				vkbdvw->SetKeyboardStatus(STATUS_FIELD::KBD_AR2, false);
				break;

			case BKKeyType::STOP:
				board->UnStopInterrupt(); // отжали кнопку стоп
				break;
		}
	}

	return S_OK;
}



void CBKView::OnClose()
{
	KillTimer(BKTIMER_MOUSE);      // остановить таймер для мыши

	if (m_hHook)
	{
		BOOL bRes = UnhookWindowsHookEx(m_hHook);
		ASSERT(bRes);
	}

	CView::OnClose();
}


void CBKView::OnSetFocus(CWnd *pOldWnd)
{
	CView::OnSetFocus(pOldWnd); // можно и на себе фокус оставлять

	if (m_pScreen->IsFullScreenMode())
	{
		if (g_Config.m_nScreenRenderType == CONF_SCREEN_RENDER::D3D)
		{
			m_pScreen->RestoreFS();
		}
		else
		{
			m_pScreen->BringWindowToTop();
		}
	}

	// вернём состояние капслока в программе
	switch (m_nCurrentCapsState)
	{
		case CAPS_STATE::PRESSED:
			CapsLockOn();
			break;
		case CAPS_STATE::UNPRESSED:
			CapsLockOff();
			break;
	}

	m_bAppFocused = true;
}


void CBKView::OnKillFocus(CWnd *pNewWnd)
{
	CView::OnKillFocus(pNewWnd);

	// вернём состояние капслока вне программы
	if (g_Config.m_bSysCapsStatus)
	{
		CapsLockOn();
	}
	else
	{
		CapsLockOff();
	}

	m_kprs.clear(); // если фокус теряется, то все нажатые клавиши считаются не нажатыми.
	m_bAppFocused = false;
}


#endif