#include "pch.h"
#include "BKKbdButn.h"
#include "BK.h"
#include "Config.h"
#ifdef UI
IMPLEMENT_DYNAMIC(CBKKbdButn, CWnd)

CBKKbdButn::CBKKbdButn(UINT nID)
	: m_cx(0)
	, m_cy(0)
	, m_imgW(0)
	, m_imgH(0)
	, m_nIdx(-1)
	, m_nArraySize(-1)
	, m_bAR2Pressed(false)
	, m_bSUPressed(false)
	, m_bShiftPressed(false)
	, m_bRShiftPressed(false)
	, m_bZaglPressed(false)
	, m_bXlatMode(false)
	, m_bControlKeyPressed(false)
	, m_bRegularKeyPressed(false)
	, m_pBKKeyboardArray(nullptr)
{
	SetID(nID);
	m_ImgScr.Create(m_imgW, m_imgH, m_Img.GetBPP());
}

void CBKKbdButn::SetID(const UINT nID)
{
	UINT nID_p = nID; // ид битмапа нажатой клавиатуры

	switch (nID)
	{
		case IDB_BITMAP_SOFT:
			m_pBKKeyboardArray = const_cast<BKKey *>(m_ButnKbdKeys);
			nID_p = IDB_BITMAP_SOFT_P;  // Битмап нажатия для скрипучей клавы
			break;

		case IDB_BITMAP_PLEN:
			m_pBKKeyboardArray = const_cast<BKKey *>(m_PlenKbdKeys);
			nID_p = IDB_BITMAP_PLEN_P;  // Битмап нажатия для силиконовой клавы
			break;

		default:
			return;
	}

	if (!m_Img.IsNull())
	{
		m_Img.Destroy();
	}

	if (!m_Img_p.IsNull())
	{
		m_Img_p.Destroy();
	}

	// загрузка png из ресурсов
	CPngImage img;  // эта штука умеет загружать. но ничего больше полезного не умеет

	if (img.Load(nID))  // грузим картинку
	{
		m_Img.Attach((HBITMAP)img.Detach());    // передаём битмап нужному объекту
		img.CleanUp();  // очистка
	}

	// и всё то же самое для второго рисунка
	if (img.Load(nID_p))
	{
		m_Img_p.Attach((HBITMAP)img.Detach());
		img.CleanUp();
	}

//  m_Img.LoadFromResource(AfxGetInstanceHandle(), nID);
//  m_Img_p.LoadFromResource(AfxGetInstanceHandle(), nID_p);
	m_imgW = m_Img.GetWidth();
	m_imgH = m_Img.GetHeight();
	m_nArraySize = GetArraySize();
	m_nAR2Index = GetKeyIndexById(BKKeyType::ALT);
	m_nSUIndex = GetKeyIndexById(BKKeyType::CTRL);
	m_nLShiftIndex = GetKeyIndexById(BKKeyType::LSHIFT);
	m_nRShiftIndex = GetKeyIndexById(BKKeyType::RSHIFT);
}

CBKKbdButn::~CBKKbdButn()
{
	ClearObj();
}

void CBKKbdButn::ClearObj()
{
	if (!m_ImgScr.IsNull())
	{
		m_ImgScr.Destroy();
	}

	if (!m_Img.IsNull())
	{
		m_Img.Destroy();
	}

	if (!m_Img_p.IsNull())
	{
		m_Img_p.Destroy();
	}
}

BEGIN_MESSAGE_MAP(CBKKbdButn, CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_ERASEBKGND()

	ON_MESSAGE(WM_VKBD_DN_CALLBACK, &CBKKbdButn::OnRealKeyDown)
	ON_MESSAGE(WM_VKBD_UP_CALLBACK, &CBKKbdButn::OnRealKeyUp)
	ON_WM_SIZE()
END_MESSAGE_MAP()


void CBKKbdButn::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	HDC hdc = dc.GetSafeHdc();
	SetStretchBltMode(hdc, HALFTONE);
	// сперва копируем основное изображение в буфер
	m_Img.BitBlt(m_ImgScr.GetDC(), CPoint(0, 0), SRCCOPY);
	m_ImgScr.ReleaseDC();

	if (m_nIdx >= 0) // если нажата левая кнопка, и там где надо
	{
		// нужно отрисовать нажатую кнопку
		_FocusPressedkey(m_nIdx);
	}

	if (m_bAR2Pressed)
	{
		_FocusPressedkey(m_nAR2Index);
	}

	if (m_bSUPressed)
	{
		_FocusPressedkey(m_nSUIndex);
	}

	if (m_bShiftPressed)
	{
		_FocusPressedkey(m_nLShiftIndex);
	}

	if (m_bRShiftPressed)
	{
		_FocusPressedkey(m_nRShiftIndex);
	}

	// и затем рисуем смасштабированно из буфера.
	m_ImgScr.Draw(hdc, 0, 0, m_cx, m_cy, 0, 0, m_imgW, m_imgH);
}

void CBKKbdButn::AdjustLayout()
{
	if (m_hwndParent == nullptr)
	{
		return;
	}

	CRect rcScreen(0, 0, m_imgW, m_imgH);
	CRect rcNewScreen = rcScreen;
	CRect rcClient;
	m_cwndParent->GetClientRect(&rcClient);

	if (rcScreen.Width() * rcClient.Height() > rcClient.Width() * rcScreen.Height())
	{
		rcNewScreen.right = rcClient.Width();
		rcNewScreen.bottom = rcScreen.bottom * rcClient.Width() / rcScreen.right;
	}
	else
	{
		rcNewScreen.bottom = rcClient.Height();
		rcNewScreen.right = rcScreen.right * rcClient.Height() / rcScreen.bottom;
	}

	if (rcNewScreen.Width() > m_imgW)
	{
		rcNewScreen.right = m_imgW;
	}

	if (rcNewScreen.Height() > m_imgH)
	{
		rcNewScreen.bottom = m_imgH;
	}

	int x_offs = (rcClient.Width() - rcNewScreen.Width()) / 2;
	int y_offs = (rcClient.Height() - rcNewScreen.Height()) / 2;
	rcClient.right = rcClient.left + rcNewScreen.Width();
	rcClient.bottom = rcClient.top + rcNewScreen.Height();
	rcClient.OffsetRect(x_offs, y_offs);
	SetWindowPos(nullptr, rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height(), SWP_NOZORDER);
}

void CBKKbdButn::_FocusPressedkey(const int nIdx)
{
	// нужно отрисовать нажатую кнопку
	const BKKey *pKey = &m_pBKKeyboardArray[nIdx];
	// затем накладываем нажатую кнопку
	m_Img_p.BitBlt(m_ImgScr.GetDC(), pKey->x1, pKey->y1, pKey->x2 - pKey->x1, pKey->y2 - pKey->y1,
	               pKey->x1, pKey->y1, SRCCOPY);
	m_ImgScr.ReleaseDC();
}


BOOL CBKKbdButn::OnEraseBkgnd(CDC *pDC)
{
	return TRUE; // CWnd::OnEraseBkgnd(pDC);
}

void CBKKbdButn::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	if ((cy > 0) && (cx > 0))
	{
		m_cx = cx;
		m_cy = cy;
	}
}

BOOL CBKKbdButn::DestroyWindow()
{
	// TODO: добавьте специализированный код или вызов базового класса
	ClearObj();
	return CWnd::DestroyWindow();
}

BOOL CBKKbdButn::PreCreateWindow(CREATESTRUCT &cs)
{
	m_hwndParent = cs.hwndParent;
	m_cwndParent = FromHandle(m_hwndParent);
	cs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_SAVEBITS,
	                                   ::LoadCursor(nullptr, IDC_ARROW), HBRUSH(COLOR_WINDOW + 1), nullptr);
	return CWnd::PreCreateWindow(cs);
}
// предполагается реакция на нажатие/отжатие реальных кнопок на реальной клавиатуре
// но что-то никак не понятно, где ловить нажатия и как превращать их в индекс кнопки
LRESULT CBKKbdButn::OnRealKeyDown(WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

LRESULT CBKKbdButn::OnRealKeyUp(WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}


void CBKKbdButn::OnLButtonDown(UINT nFlags, CPoint point)
{
	// с учётом масштабирования, надо преобразовать полученные координаты
	// в координаты рисунка.
	auto x = int(double(point.x) * double(m_imgW) / double(m_cx) + 0.5);
	auto y = int(double(point.y) * double(m_imgH) / double(m_cy) + 0.5);
	m_nIdx = GetKeyIndex(x, y);

	if (m_nIdx >= 0)
	{
		const BKKey *pKey = &m_pBKKeyboardArray[m_nIdx];

		if (pKey->nType != BKKeyType::RESERVED)
		{
			SetCapture();
			auto mw = theApp.GetMainWnd();

			switch (pKey->nType)
			{
				case BKKeyType::ALT:
					mw->SendMessageToDescendants(!m_bAR2Pressed ? WM_VKBD_DOWN : WM_VKBD_UP, static_cast<WPARAM>(pKey->nType), 0, FALSE, FALSE);
					break;

				case BKKeyType::CTRL:
					mw->SendMessageToDescendants(!m_bSUPressed ? WM_VKBD_DOWN : WM_VKBD_UP, static_cast<WPARAM>(pKey->nType), 0, FALSE, FALSE);
					break;

				case BKKeyType::LSHIFT:
					mw->SendMessageToDescendants(!m_bShiftPressed ? WM_VKBD_DOWN : WM_VKBD_UP, static_cast<WPARAM>(pKey->nType), 0, FALSE, FALSE);
					break;

				case BKKeyType::RSHIFT:
					mw->SendMessageToDescendants(!m_bRShiftPressed ? WM_VKBD_DOWN : WM_VKBD_UP, static_cast<WPARAM>(pKey->nType), 0, FALSE, FALSE);
					break;

				case BKKeyType::ZAGL:
					mw->SendMessageToDescendants(WM_VKBD_DOWN, static_cast<WPARAM>(pKey->nType), 0, FALSE, FALSE);
					break;

				case BKKeyType::STR:
					mw->SendMessageToDescendants(WM_VKBD_DOWN, static_cast<WPARAM>(pKey->nType), 0, FALSE, FALSE);
					break;

				case BKKeyType::REGULAR:
				{
					m_bRegularKeyPressed = true;
					const uint8_t nScanCode = TranslateScanCode(pKey->nScanCode);
					const uint8_t nInterrupt = (m_bAR2Pressed) ? INTERRUPT_274 : pKey->nInterrupt;
					const uint8_t nUnique = pKey->nUniqueNum;
					const LPARAM lParam = (static_cast<UINT>(nUnique) << 16) | (static_cast<UINT>(nInterrupt) << 8) | static_cast<UINT>(nScanCode);
					mw->SendMessageToDescendants(WM_VKBD_DOWN, static_cast<WPARAM>(pKey->nType), lParam, FALSE, FALSE);
				}
				break;

				case BKKeyType::STOP:
					ControlKeysUp();
					mw->SendMessageToDescendants(WM_VKBD_DOWN, static_cast<WPARAM>(pKey->nType), 0, FALSE, FALSE);
					break;
			}

			// если хоть одна из этих клавиш нажата - то флаг нажатой клавиши установлен
			m_bControlKeyPressed = m_bAR2Pressed || m_bSUPressed || m_bShiftPressed || m_bRShiftPressed;
		}
	}

	Invalidate(FALSE);
	CWnd::OnLButtonDown(nFlags, point);
}

void CBKKbdButn::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_nIdx >= 0)
	{
		const BKKey *pKey = &m_pBKKeyboardArray[m_nIdx];

		if (pKey->nType != BKKeyType::RESERVED)
		{
			ReleaseCapture();

			if (m_bRegularKeyPressed)
			{
				m_bRegularKeyPressed = false;
				const uint8_t nScanCode = TranslateScanCode(pKey->nScanCode);
				const uint8_t nInt = (m_bAR2Pressed) ? INTERRUPT_274 : pKey->nInterrupt;
				const uint8_t nUnique = pKey->nUniqueNum;
				const LPARAM lParam = (static_cast<UINT>(nUnique) << 16) | (static_cast<UINT>(nInt) << 8) | static_cast<UINT>(nScanCode);
				theApp.GetMainWnd()->SendMessageToDescendants(WM_VKBD_UP, static_cast<WPARAM>(pKey->nType), lParam, FALSE, FALSE);

				if (m_bControlKeyPressed)
				{
					ControlKeysUp();
				}
			}
			else if (pKey->nType == BKKeyType::STOP)
			{
				theApp.GetMainWnd()->SendMessageToDescendants(WM_VKBD_UP, static_cast<WPARAM>(pKey->nType), 0, FALSE, FALSE);
			}
		}

		m_nIdx = -1;
	}

	Invalidate(FALSE);
	CWnd::OnLButtonUp(nFlags, point);
}

void CBKKbdButn::ControlKeysUp()
{
	auto mw = theApp.GetMainWnd();

	if (m_bAR2Pressed)
	{
		mw->SendMessageToDescendants(WM_VKBD_UP, static_cast<WPARAM>(BKKeyType::ALT), 0, FALSE, FALSE);
	}

	if (m_bSUPressed)
	{
		mw->SendMessageToDescendants(WM_VKBD_UP, static_cast<WPARAM>(BKKeyType::CTRL), 0, FALSE, FALSE);
	}

	if (m_bShiftPressed)
	{
		mw->SendMessageToDescendants(WM_VKBD_UP, static_cast<WPARAM>(BKKeyType::LSHIFT), 0, FALSE, FALSE);
	}

	if (m_bRShiftPressed)
	{
		mw->SendMessageToDescendants(WM_VKBD_UP, static_cast<WPARAM>(BKKeyType::RSHIFT), 0, FALSE, FALSE);
	}

	m_bControlKeyPressed = false;
	Invalidate(FALSE);
}

uint8_t CBKKbdButn::TranslateScanCode(uint8_t nScanCode) const
{
	if (m_bSUPressed)
	{
		if ((0100 <= nScanCode) && (nScanCode <= 0137))
		{
			return nScanCode & 037;
		}
	}

	if (GetShiftStatus())
	{
		if ((054 <= nScanCode) && (nScanCode <= 057))
		{
			return nScanCode + 020;
		}

		if ((060 <= nScanCode) && (nScanCode <= 073))
		{
			return nScanCode - 020;
		}
	}

	if ((0100 <= nScanCode) && (nScanCode <= 0137))
	{
		// если режим строчных букв - то их в строчные
		if (m_bXlatMode)
		{
			if (!(m_bZaglPressed ^ !GetShiftStatus()))
			{
				nScanCode += 040;
			}
		}
		else
		{
			if (!(m_bZaglPressed ^ GetShiftStatus()))
			{
				nScanCode += 040;
			}
		}
	}

	return nScanCode;
}

int CBKKbdButn::GetKeyIndex(const int x, const int y) const
{
	ASSERT(m_pBKKeyboardArray);

	for (int i = 0; i < m_nArraySize; ++i)
	{
		if ((m_pBKKeyboardArray[i].x1 <= x && x <= m_pBKKeyboardArray[i].x2)
		        && (m_pBKKeyboardArray[i].y1 <= y && y <= m_pBKKeyboardArray[i].y2)
		   )
		{
			return i;
		}
	}

	return -1;
}

int CBKKbdButn::GetKeyIndexById(const BKKeyType nType) const
{
	ASSERT(m_pBKKeyboardArray);

	for (int i = 0; i < m_nArraySize; ++i)
	{
		if (m_pBKKeyboardArray[i].nType == nType)
		{
			return i;
		}
	}

	return -1;
}

int CBKKbdButn::GetArraySize() const
{
	ASSERT(m_pBKKeyboardArray);
	int i = -1;

	if (m_pBKKeyboardArray)
	{
		while (m_pBKKeyboardArray[++i].nType != BKKeyType::ENDARRAY)
		{
			if (i == INT_MAX)
			{
				return -1;
			}
		}
	}

	return i;
}

uint8_t CBKKbdButn::GetUniqueKeyNum(const uint8_t nScancode) const
{
	ASSERT(m_pBKKeyboardArray);

	if (m_pBKKeyboardArray)
	{
		for (int i = 0; i < m_nArraySize; ++i)
		{
			if (m_pBKKeyboardArray[i].nType == BKKeyType::REGULAR)
			{
				if (m_pBKKeyboardArray[i].nScanCode == nScancode)
				{
					return m_pBKKeyboardArray[i].nUniqueNum;
				}
			}
		}
	}

	return 0;
}


const BKKey CBKKbdButn::m_ButnKbdKeys[] =
{
	{ BKKeyType::REGULAR,   5,   5,  100,  68, 0001, INTERRUPT_274,  1 }, // ПОВТ
	{ BKKeyType::REGULAR, 102,   5,  199,  68, 0003, INTERRUPT_60,   2 }, // КТ
	{ BKKeyType::REGULAR, 201,   5,  297,  68, 0013, INTERRUPT_274,  3 }, // del rgt
	{ BKKeyType::REGULAR, 299,   5,  396,  68, 0026, INTERRUPT_60,   4 }, // del
	{ BKKeyType::REGULAR, 398,   5,  494,  68, 0027, INTERRUPT_60,   5 }, // ins
	{ BKKeyType::REGULAR, 496,   5,  593,  68, 0002, INTERRUPT_274,  6 }, // ИНДСУ
	{ BKKeyType::REGULAR, 595,   5,  691,  68, 0004, INTERRUPT_274,  7 }, // БЛОКРЕД
	{ BKKeyType::REGULAR, 693,   5,  789,  68, 0000, INTERRUPT_274,  8 }, // ШАГ
	{ BKKeyType::REGULAR, 792,   5,  904,  68, 0014, INTERRUPT_60,   9 }, // СБР
	{ BKKeyType::STOP,    906,   5, 1019,  68, 0000, 0000,          10 }, // СТОП
	{ BKKeyType::LSHIFT,    5,  70,   68, 134, 0000, 0000,          11 }, // shift
	{ BKKeyType::REGULAR,  70,  70,  134, 134, 0073, INTERRUPT_60,  12 }, // ;
	{ BKKeyType::REGULAR, 136,  70,  200, 134, 0061, INTERRUPT_60,  13 }, // 1
	{ BKKeyType::REGULAR, 201,  70,  265, 134, 0062, INTERRUPT_60,  14 }, // 2
	{ BKKeyType::REGULAR, 267,  70,  331, 134, 0063, INTERRUPT_60,  15 }, // 3
	{ BKKeyType::REGULAR, 333,  70,  396, 134, 0064, INTERRUPT_60,  16 }, // 4
	{ BKKeyType::REGULAR, 398,  70,  462, 134, 0065, INTERRUPT_60,  17 }, // 5
	{ BKKeyType::REGULAR, 464,  70,  528, 134, 0066, INTERRUPT_60,  18 }, // 6
	{ BKKeyType::REGULAR, 530,  70,  593, 134, 0067, INTERRUPT_60,  19 }, // 7
	{ BKKeyType::REGULAR, 595,  70,  659, 134, 0070, INTERRUPT_60,  20 }, // 8
	{ BKKeyType::REGULAR, 661,  70,  725, 134, 0071, INTERRUPT_60,  21 }, // 9
	{ BKKeyType::REGULAR, 726,  70,  790, 134, 0060, INTERRUPT_60,  22 }, // 0
	{ BKKeyType::REGULAR, 792,  70,  856, 134, 0055, INTERRUPT_60,  23 }, // -
	{ BKKeyType::REGULAR, 857,  70,  921, 134, 0072, INTERRUPT_60,  24 }, // :
	{ BKKeyType::REGULAR, 923,  70, 1019, 134, 0030, INTERRUPT_60,  25 }, // backspace
	{ BKKeyType::REGULAR,   5, 136,  100, 200, 0011, INTERRUPT_274, 26 }, // ТАБ
	{ BKKeyType::REGULAR, 102, 136,  166, 200, 0112, INTERRUPT_60,  27 }, // J
	{ BKKeyType::REGULAR, 168, 136,  232, 200, 0103, INTERRUPT_60,  28 }, // C
	{ BKKeyType::REGULAR, 234, 136,  297, 200, 0125, INTERRUPT_60,  29 }, // U
	{ BKKeyType::REGULAR, 299, 136,  363, 200, 0113, INTERRUPT_60,  30 }, // K
	{ BKKeyType::REGULAR, 365, 136,  429, 200, 0105, INTERRUPT_60,  31 }, // E
	{ BKKeyType::REGULAR, 431, 136,  494, 200, 0116, INTERRUPT_60,  32 }, // N
	{ BKKeyType::REGULAR, 496, 136,  560, 200, 0107, INTERRUPT_60,  33 }, // G
	{ BKKeyType::REGULAR, 562, 136,  626, 200, 0133, INTERRUPT_60,  34 }, // [
	{ BKKeyType::REGULAR, 628, 136,  691, 200, 0135, INTERRUPT_60,  35 }, // ]
	{ BKKeyType::REGULAR, 693, 136,  757, 200, 0132, INTERRUPT_60,  36 }, // Z
	{ BKKeyType::REGULAR, 759, 136,  823, 200, 0110, INTERRUPT_60,  37 }, // H
	{ BKKeyType::REGULAR, 825, 136,  888, 200, 0137, INTERRUPT_60,  38 }, // Ъ
	{ BKKeyType::REGULAR, 890, 136,  954, 200, 0057, INTERRUPT_60,  39 }, // /
	{ BKKeyType::REGULAR, 956, 136, 1019, 200, 0023, INTERRUPT_60,  40 }, // ВС
	{ BKKeyType::CTRL,      5, 202,  117, 265, 0000, 0000,          41 }, // СУ
	{ BKKeyType::REGULAR, 119, 202,  183, 265, 0106, INTERRUPT_60,  42 }, // F
	{ BKKeyType::REGULAR, 184, 202,  248, 265, 0131, INTERRUPT_60,  43 }, // Y
	{ BKKeyType::REGULAR, 250, 202,  314, 265, 0127, INTERRUPT_60,  44 }, // W
	{ BKKeyType::REGULAR, 316, 202,  380, 265, 0101, INTERRUPT_60,  45 }, // A
	{ BKKeyType::REGULAR, 382, 202,  445, 264, 0120, INTERRUPT_60,  46 }, // P
	{ BKKeyType::REGULAR, 447, 202,  511, 265, 0122, INTERRUPT_60,  47 }, // R
	{ BKKeyType::REGULAR, 513, 202,  577, 265, 0117, INTERRUPT_60,  48 }, // O
	{ BKKeyType::REGULAR, 579, 202,  642, 265, 0114, INTERRUPT_60,  49 }, // L
	{ BKKeyType::REGULAR, 644, 202,  708, 265, 0104, INTERRUPT_60,  50 }, // D
	{ BKKeyType::REGULAR, 710, 202,  773, 265, 0126, INTERRUPT_60,  51 }, // V
	{ BKKeyType::REGULAR, 775, 202,  839, 265, 0134, INTERRUPT_60,  52 }, // Э
	{ BKKeyType::REGULAR, 841, 202,  905, 265, 0056, INTERRUPT_60,  53 }, // .
	{ BKKeyType::REGULAR, 906, 202, 1019, 265, 0012, INTERRUPT_60,  54 }, // enter
	{ BKKeyType::ZAGL,      5, 267,   85, 331, 0000, 0000,          55 }, // ЗАГЛ
	{ BKKeyType::STR,      87, 267,  150, 331, 0000, 0000,          56 }, // СТР
	{ BKKeyType::REGULAR, 153, 267,  217, 331, 0121, INTERRUPT_60,  57 }, // Q
	{ BKKeyType::REGULAR, 218, 267,  282, 331, 0136, INTERRUPT_60,  58 }, // Ч
	{ BKKeyType::REGULAR, 284, 267,  348, 331, 0123, INTERRUPT_60,  59 }, // S
	{ BKKeyType::REGULAR, 350, 267,  413, 331, 0115, INTERRUPT_60,  60 }, // M
	{ BKKeyType::REGULAR, 415, 267,  479, 331, 0111, INTERRUPT_60,  61 }, // I
	{ BKKeyType::REGULAR, 481, 267,  544, 331, 0124, INTERRUPT_60,  62 }, // T
	{ BKKeyType::REGULAR, 546, 267,  610, 331, 0130, INTERRUPT_60,  63 }, // X
	{ BKKeyType::REGULAR, 612, 267,  676, 331, 0102, INTERRUPT_60,  64 }, // B
	{ BKKeyType::REGULAR, 678, 267,  741, 331, 0100, INTERRUPT_60,  65 }, // @
	{ BKKeyType::REGULAR, 743, 267,  823, 331, 0054, INTERRUPT_60,  66 }, // ,
	{ BKKeyType::REGULAR,   5, 333,  134, 396, 0016, INTERRUPT_60,  67 }, // РУС
	{ BKKeyType::ALT,     136, 333,  216, 396, 0000, 0000,          68 }, // АР2
	{ BKKeyType::REGULAR, 218, 333,  691, 396, 0040, INTERRUPT_60,  69 }, // space
	{ BKKeyType::REGULAR, 693, 333,  822, 396, 0017, INTERRUPT_60,  70 }, // ЛАТ
	{ BKKeyType::REGULAR, 824, 267,  888, 396, 0010, INTERRUPT_60,  71 }, // left
	{ BKKeyType::REGULAR, 890, 267,  954, 331, 0032, INTERRUPT_60,  72 }, // up
	{ BKKeyType::REGULAR, 890, 333,  954, 396, 0033, INTERRUPT_60,  73 }, // down
	{ BKKeyType::REGULAR, 956, 267, 1019, 396, 0031, INTERRUPT_60,  74 }, // right

	{ BKKeyType::ENDARRAY,  0,   0,    0,   0,    0,    0,           0 }
};

const BKKey CBKKbdButn::m_PlenKbdKeys[] =
{
	{ BKKeyType::ALT,        4,   4,   66,  68, 0000, 0000,          68 }, // НР
	{ BKKeyType::CTRL,      68,   4,  130,  68, 0000, 0000,          41 }, // СУ
	{ BKKeyType::STOP,     132,   4,  193,  68, 0000, 0000,          10 }, // СТОП
	{ BKKeyType::REGULAR,  195,   4,  256,  68, 0000, INTERRUPT_274,  8 }, // ШАГ
	{ BKKeyType::REGULAR,  258,   4,  320,  68, 0002, INTERRUPT_274,  6 }, // ИНДСУ
	{ BKKeyType::REGULAR,  322,   4,  384,  68, 0004, INTERRUPT_274,  7 }, // БЛОКРЕД
	{ BKKeyType::REGULAR,  386,   4,  448,  68, 0005, INTERRUPT_274, 75 }, // ГРАФ
	{ BKKeyType::REGULAR,  450,   4,  511,  68, 0006, INTERRUPT_274, 76 }, // ЗАП
	{ BKKeyType::REGULAR,  513,   4,  574,  68, 0007, INTERRUPT_274, 77 }, // СТИР
	{ BKKeyType::REGULAR,  576,   4,  638,  68, 0015, INTERRUPT_60,  78 }, // УСТТАБ
	{ BKKeyType::REGULAR,  640,   4,  702,  68, 0020, INTERRUPT_60,  79 }, // СБРТАБ
	{ BKKeyType::REGULAR,  704,   4,  766,  68, 0003, INTERRUPT_60,   2 }, // КТ
	{ BKKeyType::REGULAR,  768,   4,  829,  68, 0023, INTERRUPT_60,  40 }, // ВС
	{ BKKeyType::REGULAR,  831,   4,  893,  68, 0014, INTERRUPT_60,   9 }, // СБР
	{ BKKeyType::REGULAR,  896,   4,  956,  68, 0024, INTERRUPT_60,  80 }, // ГТ
	{ BKKeyType::REGULAR,  958,   4, 1020,  68, 0013, INTERRUPT_274,  3 }, // СБР пр
	{ BKKeyType::REGULAR,    4,  70,   66, 134, 0073, INTERRUPT_60,  12 }, // ;
	{ BKKeyType::REGULAR,   68,  70,  130, 134, 0061, INTERRUPT_60,  13 }, // 1
	{ BKKeyType::REGULAR,  132,  70,  193, 134, 0062, INTERRUPT_60,  14 }, // 2
	{ BKKeyType::REGULAR,  195,  70,  256, 134, 0063, INTERRUPT_60,  15 }, // 3
	{ BKKeyType::REGULAR,  258,  70,  320, 134, 0064, INTERRUPT_60,  16 }, // 4
	{ BKKeyType::REGULAR,  322,  70,  384, 134, 0065, INTERRUPT_60,  17 }, // 5
	{ BKKeyType::REGULAR,  386,  70,  448, 134, 0066, INTERRUPT_60,  18 }, // 6
	{ BKKeyType::REGULAR,  450,  70,  511, 134, 0067, INTERRUPT_60,  19 }, // 7
	{ BKKeyType::REGULAR,  513,  70,  574, 134, 0070, INTERRUPT_60,  20 }, // 8
	{ BKKeyType::REGULAR,  576,  70,  638, 134, 0071, INTERRUPT_60,  21 }, // 9
	{ BKKeyType::REGULAR,  640,  70,  702, 134, 0060, INTERRUPT_60,  22 }, // 0
	{ BKKeyType::REGULAR,  704,  70,  766, 134, 0055, INTERRUPT_60,  23 }, // -
	{ BKKeyType::REGULAR,  768,  70,  829, 134, 0034, INTERRUPT_60,  81 }, // left-up
	{ BKKeyType::REGULAR,  831,  70,  893, 134, 0032, INTERRUPT_60,  72 }, // up
	{ BKKeyType::REGULAR,  896,  70,  956, 134, 0035, INTERRUPT_60,  82 }, // right-up
	{ BKKeyType::REGULAR,  958,  70, 1020, 134, 0001, INTERRUPT_274,  1 }, // ПОВТ
	{ BKKeyType::REGULAR,    4, 136,   66, 200, 0112, INTERRUPT_60,  27 }, // J
	{ BKKeyType::REGULAR,   68, 136,  130, 200, 0103, INTERRUPT_60,  28 }, // C
	{ BKKeyType::REGULAR,  132, 136,  193, 200, 0125, INTERRUPT_60,  29 }, // U
	{ BKKeyType::REGULAR,  195, 136,  256, 200, 0113, INTERRUPT_60,  30 }, // K
	{ BKKeyType::REGULAR,  258, 136,  320, 200, 0105, INTERRUPT_60,  31 }, // E
	{ BKKeyType::REGULAR,  322, 136,  384, 200, 0116, INTERRUPT_60,  32 }, // N
	{ BKKeyType::REGULAR,  386, 136,  448, 200, 0107, INTERRUPT_60,  33 }, // G
	{ BKKeyType::REGULAR,  450, 136,  511, 200, 0133, INTERRUPT_60,  34 }, // [
	{ BKKeyType::REGULAR,  513, 136,  574, 200, 0135, INTERRUPT_60,  35 }, // ]
	{ BKKeyType::REGULAR,  576, 136,  638, 200, 0132, INTERRUPT_60,  36 }, // Z
	{ BKKeyType::REGULAR,  640, 136,  702, 200, 0110, INTERRUPT_60,  37 }, // H
	{ BKKeyType::REGULAR,  704, 136,  766, 200, 0072, INTERRUPT_60,  24 }, // :
	{ BKKeyType::REGULAR,  768, 136,  829, 200, 0010, INTERRUPT_60,  71 }, // left
	{ BKKeyType::REGULAR,  831, 136,  893, 200, 0022, INTERRUPT_60,  83 }, // top
	{ BKKeyType::REGULAR,  896, 136,  956, 200, 0031, INTERRUPT_60,  74 }, // right
	{ BKKeyType::RESERVED, 958, 136, 1020, 200, 0000, 0000,          86 }, // Резерв
	{ BKKeyType::REGULAR,    4, 202,   66, 265, 0106, INTERRUPT_60,  42 }, // F
	{ BKKeyType::REGULAR,   68, 202,  130, 265, 0131, INTERRUPT_60,  43 }, // Y
	{ BKKeyType::REGULAR,  132, 202,  193, 265, 0127, INTERRUPT_60,  44 }, // W
	{ BKKeyType::REGULAR,  195, 202,  256, 265, 0101, INTERRUPT_60,  45 }, // A
	{ BKKeyType::REGULAR,  258, 202,  320, 265, 0120, INTERRUPT_60,  46 }, // P
	{ BKKeyType::REGULAR,  322, 202,  384, 265, 0122, INTERRUPT_60,  47 }, // R
	{ BKKeyType::REGULAR,  386, 202,  448, 265, 0117, INTERRUPT_60,  48 }, // O
	{ BKKeyType::REGULAR,  450, 202,  511, 265, 0114, INTERRUPT_60,  49 }, // L
	{ BKKeyType::REGULAR,  513, 202,  574, 265, 0104, INTERRUPT_60,  50 }, // D
	{ BKKeyType::REGULAR,  576, 202,  638, 265, 0126, INTERRUPT_60,  51 }, // V
	{ BKKeyType::REGULAR,  640, 202,  702, 265, 0134, INTERRUPT_60,  52 }, // Э
	{ BKKeyType::REGULAR,  704, 202,  766, 265, 0056, INTERRUPT_60,  53 }, // .
	{ BKKeyType::REGULAR,  768, 202,  829, 265, 0037, INTERRUPT_60,  84 }, // left-down
	{ BKKeyType::REGULAR,  831, 202,  893, 265, 0033, INTERRUPT_60,  73 }, // down
	{ BKKeyType::REGULAR,  896, 202,  956, 265, 0036, INTERRUPT_60,  85 }, // right-down
	{ BKKeyType::RESERVED, 958, 202, 1020, 265, 0000, 0000,          87 }, // Резерв
	{ BKKeyType::REGULAR,    4, 267,   66, 331, 0121, INTERRUPT_60,  57 }, // Q
	{ BKKeyType::REGULAR,   68, 267,  130, 331, 0136, INTERRUPT_60,  58 }, // Ч
	{ BKKeyType::REGULAR,  132, 267,  193, 331, 0123, INTERRUPT_60,  59 }, // S
	{ BKKeyType::REGULAR,  195, 267,  256, 331, 0115, INTERRUPT_60,  60 }, // M
	{ BKKeyType::REGULAR,  258, 267,  320, 331, 0111, INTERRUPT_60,  61 }, // I
	{ BKKeyType::REGULAR,  322, 267,  384, 331, 0124, INTERRUPT_60,  62 }, // T
	{ BKKeyType::REGULAR,  386, 267,  448, 331, 0130, INTERRUPT_60,  63 }, // X
	{ BKKeyType::REGULAR,  450, 267,  511, 331, 0102, INTERRUPT_60,  64 }, // B
	{ BKKeyType::REGULAR,  513, 267,  574, 331, 0100, INTERRUPT_60,  65 }, // @
	{ BKKeyType::REGULAR,  576, 267,  638, 331, 0054, INTERRUPT_60,  66 }, // ,
	{ BKKeyType::REGULAR,  640, 267,  702, 331, 0057, INTERRUPT_60,  39 }, // /
	{ BKKeyType::REGULAR,  704, 267,  766, 331, 0137, INTERRUPT_60,  38 }, // ЗБ
	{ BKKeyType::REGULAR,  768, 267,  829, 331, 0026, INTERRUPT_60,   4 }, // сдвижка
	{ BKKeyType::REGULAR,  831, 267,  893, 331, 0030, INTERRUPT_60,  25 }, // backspace
	{ BKKeyType::REGULAR,  896, 267,  956, 331, 0027, INTERRUPT_60,   5 }, // раздвижка
	{ BKKeyType::RESERVED, 958, 267, 1020, 331, 0000, 0000,          88 }, // Резерв
	{ BKKeyType::LSHIFT,     4, 333,   66, 397, 0000, 0000,          11 }, // ПР
	{ BKKeyType::ZAGL,      68, 333,  130, 397, 0000, 0000,          55 }, // ЗАГЛ
	{ BKKeyType::REGULAR,  132, 333,  256, 397, 0016, INTERRUPT_60,  67 }, // РУС
	{ BKKeyType::REGULAR,  258, 333,  511, 397, 0040, INTERRUPT_60,  69 }, // space
	{ BKKeyType::REGULAR,  513, 333,  638, 397, 0017, INTERRUPT_60,  70 }, // ЛАТ
	{ BKKeyType::STR,      640, 333,  702, 397, 0000, 0000,          56 }, // СТР
	{ BKKeyType::RSHIFT,   704, 333,  766, 397, 0000, 0000,          11 }, // ПР-2
	{ BKKeyType::REGULAR,  768, 333,  829, 397, 0011, INTERRUPT_274, 26 }, // ТАБ
	{ BKKeyType::REGULAR,  831, 333,  956, 397, 0012, INTERRUPT_60,  54 }, // ВВОД
	{ BKKeyType::RESERVED, 958, 333, 1020, 397, 0000, 0000,          89 }, // Резерв

	{ BKKeyType::ENDARRAY,   0,   0,    0,   0,    0,    0,           0 }
};


#endif