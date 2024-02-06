// ScreenDIB.cpp: файл реализации
//

#include "pch.h"
#include "SprScr.h"

constexpr uint32_t Color_Black       = 0x000000;
constexpr uint32_t Color_Red         = 0xff0000;
constexpr uint32_t Color_Green       = 0x00ff00;
constexpr uint32_t Color_Blue        = 0x0000ff;
constexpr uint32_t Color_Yellow      = 0xffff00;
constexpr uint32_t Color_Cyan        = 0x00ffff;
constexpr uint32_t Color_Magenta     = 0xff00ff;
constexpr uint32_t Color_DarkRed     = 0xaa0000;
constexpr uint32_t Color_RedBrown    = 0x700000;
constexpr uint32_t Color_Violet      = 0xaa00ff;
constexpr uint32_t Color_VioletBlue  = 0x7000ff;
constexpr uint32_t Color_LightGreen  = 0x55ff00;
constexpr uint32_t Color_Salatovyi   = 0xaaff00;
constexpr uint32_t Color_White       = 0xffffff;

const uint32_t CSprScr::g_pColorPalettes[16][4] =
{
	//                                                            Palette#     01           10          11
	Color_Black, Color_Blue, Color_Green, Color_Red,                // 00    синий   |   зелёный  |  красный
	Color_Black, Color_Yellow, Color_Magenta, Color_Red,            // 01   жёлтый   |  сиреневый |  красный
	Color_Black, Color_Cyan, Color_Blue, Color_Magenta,             // 02   голубой  |    синий   | сиреневый
	Color_Black, Color_Green, Color_Cyan, Color_Yellow,             // 03   зелёный  |   голубой  |  жёлтый
	Color_Black, Color_Magenta, Color_Cyan, Color_White,            // 04  сиреневый |   голубой  |   белый
	Color_Black, Color_White, Color_White, Color_White,             // 05    белый   |    белый   |   белый
	Color_Black, Color_DarkRed, Color_RedBrown, Color_Red,          // 06  тёмн-красн| красн-корич|  красный    !
	Color_Black, Color_Salatovyi, Color_LightGreen, Color_Yellow,   // 07  салатовый | светл-зелен|  жёлтый     !
	Color_Black, Color_Violet, Color_VioletBlue, Color_Magenta,     // 08  фиолетовый| фиол-синий | сиреневый   !
	Color_Black, Color_LightGreen, Color_VioletBlue, Color_RedBrown,// 09 светл-зелен| фиол-синий |красн-корич  !
	Color_Black, Color_Salatovyi, Color_Violet, Color_DarkRed,      // 10  салатовый | фиолетовый |тёмн-красный !
	Color_Black, Color_Cyan, Color_Yellow, Color_Red,               // 11   голубой  |   жёлтый   |  красный
	Color_Black, Color_Red, Color_Green, Color_Cyan,                // 12   красный  |   зелёный  |  голубой
	Color_Black, Color_Cyan, Color_Yellow, Color_White,             // 13   голубой  |   жёлтый   |   белый
	Color_Black, Color_Yellow, Color_Green, Color_White,            // 14   жёлтый   |   зелёный  |   белый
	Color_Black, Color_Cyan, Color_Green, Color_White               // 15   голубой  |   зелёный  |   белый
};

// 0,1,2,3
const uint32_t CSprScr::g_pMonochromePalette[2][4] =
{
	Color_Black, Color_White, Color_Black, Color_White,
	Color_Black, Color_Black, Color_White, Color_White
};

const uint32_t CSprScr::g_pAdaptMonochromePalette[2][4] =
{
	Color_Black, 0x555555, 0xa9a9a9, Color_White,
	Color_Black, 0x555555, 0xa9a9a9, Color_White
};

// CSprScr

IMPLEMENT_DYNAMIC(CSprScr, CWnd)

CSprScr::CSprScr()
	: m_pBuffer(nullptr)
	, m_nBufSize(0)
	, m_cx(0)
	, m_cy(0)
	, m_orig_cx(0)
	, m_orig_cy(0)
	, m_nScale(1)
	, m_PaletteNum(0)
	, m_bColorMode(true)
	, m_bAdapt(false)
	, m_bBusy(false)
	, m_bChangeMode(false)
	, m_bits(nullptr)
	, m_hwndScreen(nullptr)   // Screen View window handle
	, m_hdd(nullptr)
	, m_hbmp(nullptr)
	, m_bmpinfo()
{
}

CSprScr::~CSprScr()
    = default;

BEGIN_MESSAGE_MAP(CSprScr, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
END_MESSAGE_MAP()



// обработчики сообщений CSprScr
void CSprScr::ScreenView_Init()
{
	m_hdd = DrawDibOpen();
	ASSERT(m_hwndScreen != nullptr);
	InitColorTables();

	if (m_hbmp)
	{
		DeleteObject(m_hbmp);
		m_hbmp = nullptr;
	}

	HDC hdc = ::GetDC(m_hwndScreen);
	m_bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_bmpinfo.bmiHeader.biWidth = m_cx;
	m_bmpinfo.bmiHeader.biHeight = m_cy;
	m_bmpinfo.bmiHeader.biPlanes = 1;
	m_bmpinfo.bmiHeader.biBitCount = 32;
	m_bmpinfo.bmiHeader.biCompression = BI_RGB;
	m_bmpinfo.bmiHeader.biSizeImage = 0;
	m_bmpinfo.bmiHeader.biXPelsPerMeter = 0;
	m_bmpinfo.bmiHeader.biYPelsPerMeter = 0;
	m_bmpinfo.bmiHeader.biClrUsed = 0;
	m_bmpinfo.bmiHeader.biClrImportant = 0;
	m_hbmp = CreateDIBSection(hdc, &m_bmpinfo, DIB_RGB_COLORS, (void **) &m_bits, nullptr, 0);
	::ReleaseDC(m_hwndScreen, hdc);
}

void CSprScr::ScreenView_Done()
{
	SAFE_DELETE_OBJECT(m_hbmp);
	DrawDibClose(m_hdd);
}


void CSprScr::OnDestroy()
{
	ScreenView_Done();
	CWnd::OnDestroy();
}

int CSprScr::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	m_hwndScreen = GetSafeHwnd();
	ScreenView_Init();
	return 0;
}

BOOL CSprScr::OnEraseBkgnd(CDC *pDC)
{
	return TRUE;
	// return CWnd::OnEraseBkgnd(pDC);
}

BOOL CSprScr::PreCreateWindow(CREATESTRUCT &cs)
{
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_PARENTDC | CS_SAVEBITS,
	                                   ::LoadCursor(nullptr, IDC_ARROW), HBRUSH(COLOR_WINDOW + 1), nullptr);
	m_cx = cs.cx;
	m_cy = cs.cy;
	CalcOrigSize();
	return CWnd::PreCreateWindow(cs);
}

void CSprScr::CalcOrigSize()
{
	m_orig_cx = m_cx / m_nScale;
	m_orig_cy = m_cy / m_nScale;
}

void CSprScr::AdjustLayout(const int cx, const int cy, const int ysp)
{
	m_cy = cy;
	m_cx = cx;
	MoveWindow(0, -ysp, m_cx, m_cy, FALSE);
	CalcOrigSize();
	PrepareScreenRGB32();
}

void CSprScr::SetColorMode(const bool bColorMode)
{
	if (m_bColorMode != bColorMode)
	{
		m_bColorMode = bColorMode;
		ReDrawScreen();
	}
}

void CSprScr::SetAdaptMode(const bool bFlag)
{
	if (m_bAdapt != bFlag)
	{
		m_bAdapt = bFlag;
		InitColorTables();
		ReDrawScreen();
	}
}

void CSprScr::SetScale(const int scale)
{
	if (m_nScale != scale)
	{
		m_nScale = scale;
	}
}

void CSprScr::SetPalette(int palette)
{
	palette &= 0xf;

	if (m_PaletteNum != palette)
	{
		m_PaletteNum = palette;
		ReDrawScreen();
	}
}

void CSprScr::DrawScreen()
{
	if (m_bits == nullptr || m_bBusy || m_bChangeMode)
	{
		return;
	}

	HDC hdc = ::GetDC(m_hwndScreen);
	/*
	m_orig_cx, m_orig_cy - координаты оригинального размера
	m_cx, m_cy - смасштабированные координаты
	а само масштабирование делается средствами DrawDibDraw
	*/
	BOOL bRes = DrawDibDraw(m_hdd, hdc,
	                        0, 0, m_cx, m_cy,
	                        &m_bmpinfo.bmiHeader, m_bits,
	                        0, 0, m_orig_cx, m_orig_cy,
	                        0);
	::ReleaseDC(m_hwndScreen, hdc);
}

void CSprScr::ReDrawScreen()
{
	PrepareScreenRGB32();
	DrawScreen();
}

void CSprScr::PrepareScreenRGB32()
{
	if (m_orig_cx * m_orig_cy <= 0)
	{
		return;
	}

	m_bBusy = true;

	if (m_hbmp)
	{
		DeleteObject(m_hbmp);
		m_hbmp = nullptr;
	}

	HDC hdc = ::GetDC(m_hwndScreen);
	m_bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_bmpinfo.bmiHeader.biWidth = m_orig_cx;
	m_bmpinfo.bmiHeader.biHeight = m_orig_cy;
	m_hbmp = CreateDIBSection(hdc, &m_bmpinfo, DIB_RGB_COLORS, (void **) &m_bits, nullptr, 0);
	::ReleaseDC(m_hwndScreen, hdc);
	size_t i = 0;
	const int nLineBytes = m_orig_cx / 8;

	for (int y = m_orig_cy - 1; y >= 0; y--)
	{
		uint32_t *pBits = m_bits + size_t(y) * m_orig_cx;
		int x = 0;

		while (x < nLineBytes)
		{
			uint32_t *pPalette = (m_bColorMode ? m_vColTable32.data() : m_vMonoTable32.data()) + (((m_PaletteNum << 8) + m_pBuffer[i++]) << 3);
			memcpy(pBits, pPalette, 8 * sizeof(uint32_t)); // копируем 1 байт
			pBits += 8;
			x++;

			if (i > m_nBufSize)
			{
				while (x < nLineBytes) // заполняем остаток строки чёрным цветом
				{
					ZeroMemory(pBits, 8 * sizeof(uint32_t));
					pBits += 8;
					x++;
				}

				break;
			}
		}
	}

	m_bBusy = false;
}

constexpr auto COLTABLE_SIZE = (16 * 256 * 8);
// 16 палитр, 256 вариантов значений байта, на каждый вариант - 8 uint32_tов,
void CSprScr::InitColorTables()
{
	m_bChangeMode = true;

	// Create color tables of all display modes for faster drawing
	m_vColTable32.clear();
	m_vMonoTable32.clear();

	m_vColTable32.resize(COLTABLE_SIZE);
	m_vMonoTable32.resize(COLTABLE_SIZE);

	for (size_t p = 0; p < 16; ++p) // для каждой из 16 палитр
	{
		// вот это делает одну палитру. а нам надо таких 16.
		for (size_t i = 0; i < 256; ++i) // для каждого значения байта (от 0 до 255) делаем маску, чтоб работать сразу с байтами и не заморачиваться с битами
		{
			uint32_t *pColBuff32 =  &m_vColTable32[(p * 256 + i) * 8];
			uint32_t *pMonBuff32 = &m_vMonoTable32[(p * 256 + i) * 8];

			for (int n = 0; n < 4; ++n) // в каждом байте 4 двухбития, на каждые два бита, у нас отводится по 4 дворда
			{
				int n2 = n * 2;

				switch ((i >> n2) & 3)
				{
					// Black
					case 0:
						pColBuff32[n2] = g_pColorPalettes[p][0];
						pMonBuff32[n2++] = (m_bAdapt) ? g_pAdaptMonochromePalette[0][0] : g_pMonochromePalette[0][0];
						pColBuff32[n2] = g_pColorPalettes[p][0];
						pMonBuff32[n2] = (m_bAdapt) ? g_pAdaptMonochromePalette[1][0] : g_pMonochromePalette[1][0];
						break;

					// Blue and Gray
					case 1:
						pColBuff32[n2] = g_pColorPalettes[p][1];
						pMonBuff32[n2++] = (m_bAdapt) ? g_pAdaptMonochromePalette[0][1] : g_pMonochromePalette[0][1];
						pColBuff32[n2] = g_pColorPalettes[p][1];
						pMonBuff32[n2] = (m_bAdapt) ? g_pAdaptMonochromePalette[1][1] : g_pMonochromePalette[1][1];
						break;

					// Green and Gray
					case 2:
						pColBuff32[n2] = g_pColorPalettes[p][2];
						pMonBuff32[n2++] = (m_bAdapt) ? g_pAdaptMonochromePalette[0][2] : g_pMonochromePalette[0][2];
						pColBuff32[n2] = g_pColorPalettes[p][2];
						pMonBuff32[n2] = (m_bAdapt) ? g_pAdaptMonochromePalette[1][2] : g_pMonochromePalette[1][2];
						break;

					// Red
					case 3:
						pColBuff32[n2] = g_pColorPalettes[p][3];
						pMonBuff32[n2++] = (m_bAdapt) ? g_pAdaptMonochromePalette[0][3] : g_pMonochromePalette[0][3];
						pColBuff32[n2] = g_pColorPalettes[p][3];
						pMonBuff32[n2] = (m_bAdapt) ? g_pAdaptMonochromePalette[1][3] : g_pMonochromePalette[1][3];
						break;
				}
			}
		} // end for(i)
	} // end for(p)

	m_bChangeMode = false;
}

