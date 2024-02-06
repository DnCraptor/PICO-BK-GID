// BKVKBDView.cpp: файл реализации
//
#ifdef UI
#include "pch.h"
#include "BKVKBDView.h"
#include "BKMessageBox.h"
#include "Config.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CBKVKBDView, CDockablePane)

CBKVKBDView::CBKVKBDView(UINT nID)
	: m_pKbdButn(nullptr)
	, m_nViewID(nID)
{
	m_rgnRes.CreateRectRgn(0, 0, 0, 0);
	m_br.CreateSysColorBrush(COLOR_BTNFACE);
}


CBKVKBDView::~CBKVKBDView()
    = default;

BEGIN_MESSAGE_MAP(CBKVKBDView, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

int CBKVKBDView::CreateKeyboard()
{
	m_pKbdButn = std::make_unique<CBKKbdButn>(m_nViewID);

	if (m_pKbdButn)
	{
		int cx = m_pKbdButn->GetWidth();
		int cy = m_pKbdButn->GetHeihgt();

		if (cx > 0 && cy > 0)
		{
			if (m_pKbdButn->Create(nullptr, _T("BK Virtual Keyboard"),
			                       WS_VISIBLE | WS_CHILD,
			                       CRect(0, 0, cx, cy), this, 0))
			{
				m_pKbdButn->AdjustLayout();
			}
			else
			{
				g_BKMsgBox.Show(IDS_BK_ERROR_NOCREATEVKBD, MB_OK);
				return -1;
			}
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
		return -1;
	}

	return 0;
}

int CBKVKBDView::SetKeyboardView(UINT nID)
{
	if (m_nViewID != nID)
	{
		m_nViewID = nID;

		if (m_pKbdButn)
		{
			m_pKbdButn->SetID(nID);
			m_pKbdButn->RedrawWindow();
		}
	}

	return 0;
}

int CBKVKBDView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	return CreateKeyboard();
}


void CBKVKBDView::OnSize(UINT nType, int cx, int cy)
{
	if (m_pKbdButn)
	{
		m_pKbdButn->AdjustLayout();
	}

	CDockablePane::OnSize(nType, cx, cy);
}

// после отсоединения надо чтобы перепозиционировалась клавиатура
void CBKVKBDView::OnAfterFloat()
{
	CDockablePane::OnAfterFloat();

	if (m_pKbdButn)
	{
		m_pKbdButn->AdjustLayout();
	}
}

void CBKVKBDView::OnAfterDockFromMiniFrame()
{
	CDockablePane::OnAfterDockFromMiniFrame();

	if (m_pKbdButn)
	{
		m_pKbdButn->AdjustLayout();
	}
}

void CBKVKBDView::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	if (m_pKbdButn && IsWindowVisible())
	{
		m_pKbdButn->Invalidate(FALSE);
	}

	// Не вызывать CDockablePane::OnPaint() для сообщений рисования
}


BOOL CBKVKBDView::OnEraseBkgnd(CDC *pDC)
{
	if (m_pKbdButn)
	{
		CRect rcScreen;
		m_pKbdButn->GetClientRect(rcScreen);
		m_pKbdButn->ClientToScreen(rcScreen);
		ScreenToClient(rcScreen);
		CRect rcClient;
		GetClientRect(rcClient);
		CRgn rgnScreen;
		rgnScreen.CreateRectRgnIndirect(rcScreen);
		CRgn rgnClient;
		rgnClient.CreateRectRgnIndirect(rcClient);
		m_rgnRes.CombineRgn(&rgnClient, &rgnScreen, RGN_DIFF);
		pDC->SelectClipRgn(&m_rgnRes);
		CBrush *pOld = pDC->SelectObject(&m_br);
		pDC->PatBlt(0, 0, rcClient.Width(), rcClient.Height(), PATCOPY);
		pDC->SelectObject(pOld);
	}

	return TRUE;
}


void CBKVKBDView::OnDestroy()
{
	if (m_pKbdButn)
	{
		m_pKbdButn->DestroyWindow();
		m_pKbdButn.reset();
	}

	CDockablePane::OnDestroy();
}

uint8_t CBKVKBDView::GetUniqueKeyNum(const uint16_t nScanCode) const
{
	return m_pKbdButn->GetUniqueKeyNum(nScanCode & 0xff);
}
/*
 *преобразование ПК сканкодов в БК сканкоды в соответствии с текущей раскладкой клавиатуры,
 *чтобы буквы на клавишах соответствовали выдаваемым сканкодам на БК
 *Вход:
 *key - ПКшный скан код, получаемый из обработчика событий OnKeyDown
 *bExtended - флаг расширенного сканкода
 *Выход: true - успешное перекодирование
 *       false - неудача, т.к. ПКшному скан коду не нашлось БКшного сопоставления
 *nScanCode - БКшный сканкод
 *nInt - прерывание, которое вызывает нажатие клавиши
 *Значение nScanCode и nInt верно только при возвращаемом значении true
 **/
bool CBKVKBDView::TranslateKey(int key, const bool bExtended, uint16_t *nScanCode, uint16_t *nInt) const
{
	*nScanCode = 0;
	*nInt = INTERRUPT_60;

	// сперва обработаем коды из диапазона 0..37, для которых есть клавиши
	if (bExtended)
	{
		switch (key)
		{
			case VK_LEFT:       // стрелка влево -- стрелка влево
				*nScanCode = BKKEY_L_ARROW;
				return true;

			case VK_RIGHT:      // стрелка вправо -- стрелка вправо
				*nScanCode = BKKEY_R_ARROW;
				return true;

			case VK_UP:         // стрелка вверх -- стрелка вверх
				*nScanCode = BKKEY_U_ARROW;
				return true;

			case VK_DOWN:       // стрелка вниз -- стрелка вниз
				*nScanCode = BKKEY_D_ARROW;
				return true;

			case VK_INSERT:     // Insert -- BC
				*nScanCode = BKKEY_VS;
				return true;

			case VK_DELETE:     // Delete -- ГРАФ
				*nScanCode = BKKEY_GRAF;
				return true;

			case VK_END:        // End -- ЗАП
				*nScanCode = BKKEY_ZAP;
				return true;

			case VK_PRIOR:      // PageUp -- СБР ТАБ
				*nScanCode = BKKEY_SBRTAB;
				return true;

			case VK_HOME:       // Home -- УСТ ТАБ
				*nScanCode = BKKEY_USTTAB;
				return true;

			case VK_NEXT:       // PageDown -- СТИР
				*nScanCode = BKKEY_STIR;
				return true;
		}

		return false;
	}

	switch (key)
	{
		case VK_RWIN:       // Правый Win -- Лат
			if (g_Config.m_bNativeRusLatSwitch)
			{
				*nScanCode = BKKEY_LAT;
			}
			else
			{
				// кнопка win (любая) переключатель рус/лат
				*nScanCode = GetXLatStatus() ? BKKEY_LAT : BKKEY_RUS;
			}

			return true;

		case VK_LWIN:       // Левый Win -- Рус
			if (g_Config.m_bNativeRusLatSwitch)
			{
				*nScanCode = BKKEY_RUS;
			}
			else
			{
				// кнопка win (любая) переключатель рус/лат
				*nScanCode = GetXLatStatus() ? BKKEY_LAT : BKKEY_RUS;
			}

			return true;

		case VK_TAB:
			*nInt = INTERRUPT_274; // оказывается ТАБ тоже только по 274 вектору
			*nScanCode = BKKEY_TAB;
			return true;

		case VK_ESCAPE:         // KT
			*nScanCode = BKKEY_KT;
			return true;

		case VK_RETURN:         // ввод
			*nScanCode = BKKEY_ENTER;
			return true;

		case VK_BACK:           // забой
			*nScanCode = BKKEY_ZAB;
			return true;

		case VK_F1:             // F1 -- ПОВТ
			*nInt = INTERRUPT_274;
			*nScanCode = BKKEY_POVT;
			return true;

		case VK_F2:             // F2 -- удаление строки справа от курсора
			*nInt = INTERRUPT_274; // 5 волшебных кнопок, которые при нажатии вызывают прерывание по вектору 274
			*nScanCode = BKKEY_RGTDEL;
			return true;

		case VK_F3:             // F3 -- сдвижка ( |<-- )
			*nScanCode = BKKEY_SDVIG;
			return true;

		case VK_F4:             // F4 -- раздвижка ( |--> )
			*nScanCode = BKKEY_RAZDVIG;
			return true;

		case VK_F5:             // F5 -- ИНДСУ
			*nInt = INTERRUPT_274;
			*nScanCode = BKKEY_INDSU;
			return true;

		case VK_F6:             // F6 -- БЛОКРЕД
			*nInt = INTERRUPT_274;
			*nScanCode = BKKEY_BLOKRED;
			return true;

		case VK_F7:             // F7 -- ШАГ
			*nInt = INTERRUPT_274;
			*nScanCode = BKKEY_SHAG;
			return true;

		case VK_F8:             // F8 -- СБР
			*nScanCode = BKKEY_SBR;
			return true;

		case VK_SPACE:
			*nScanCode = BKKEY_PROBEL;
			return true;
	}

	// потом обработаем коды, требующие обработки
	if (GetXLatStatus())
	{
		key = RusModeTranslation(key);
	}
	else
	{
		key = g_Config.m_bJCUKENKbd ? LatModeTranslationJCUK(key) : LatModeTranslation(key);
	}

	if (key >= 0) // если нужный код, то обработаем
	{
		// если символы перед буквами, на которые СУ не влияет или он просто не нажат.
		if (040 <= key && key <= 077)
		{
			*nScanCode = key;
			return true;
		}

		// если буквы
		if (0101 <= key && key <= 0132)
		{
			bool cap = GetCapitalStatus();
			bool shift = GetShiftStatus();

			// если режим строчных букв - то их в строчные
			if (GetXLatStatus()) // в русской раскладке
			{
				if (g_Config.m_bBKKeyboard) // если эмулируем БКшную клавиатуру
				{
					// нажатие шифт делает загл -> стр только
					if (cap && !shift)
					{
						key += 040;
					}
				}
				else // если не эмулируем.
				{
					// нажатие шифт меняет стр <-> загл
					if (!(cap ^ !shift))
					{
						key += 040;
					}
				}
			}
			else // в латинской раскладке
			{
				if (g_Config.m_bBKKeyboard) // если эмулируем БКшную клавиатуру
				{
					// нажатие шифт делает стр -> загл только
					if (!cap && !shift)
					{
						key += 040;
					}
				}
				else // если не эмулируем.
				{
					// нажатие шифт меняет стр <-> загл
					if (!(cap ^ shift))
					{
						key += 040;
					}
				}
			}

			*nScanCode = key;
		}
		// если не буквы, но коды >= 0100, то они уже как надо странслированы ранее.
		else if ((0100 == key) || (0133 <= key && key <= 0137) ||
		         (0140 == key) || (0173 <= key && key <= 0177)
		        )
		{
			*nScanCode = key;
		}
		else
		{
			// если ни в одно из предыдущих условий не попали - то неправильный код, такого алгоритмом не предусмотрено
			ASSERT(false);
			// поэтому можно смело прошлый else if преобразовать в просто else без условий, а этот else совсем убрать
			return false;
		}

		// и теперь обработаем кнопку СУ, если нажата
		if (GetSUStatus())
		{
			*nScanCode &= ~0140;
		}

		return true;
	}

	// сюда попадают коды, кнопок для которых на бк не предусмотрено
	return false;
}

/*
алгоритм обработки клавиатуры.
в регистре 177662 мы имеем 7 битный скан код
1. Шифт
    влияет только на диапазон 054..073
    054..057 - прибавляет 020 к значению
    060..073 - вычитает 020 из значения
    Ещё только в строчном режиме Шифт делает из строчных букв заглавные
2. СУ
    влияет только на диапазон 0100..0137
    делает операцию key &= ~0140
3. Загл
    отменяет действие кнопки Стр, по сути ничего другого не делает
4. Стр
    влияет только на диапазон 0100..0137
    прибавляет 040 к значению
5. Ар2
    на коды не влияет, просто задаёт вектор прерывания.

-------------------------------------------------------------
скан код        РС клавиша
                (знак/шифт знак)
-------------------------------------------------------------
VK_OEM_PLUS     = / +
VK_OEM_COMMA    , / <
VK_OEM_MINUS    - / _
VK_OEM_PERIOD   . / >
VK_OEM_1        ; / :
VK_OEM_2        / / ?
VK_OEM_3        ` / ~
VK_OEM_4        [ / {
VK_OEM_5        \ / |
VK_OEM_6        ] / }
VK_OEM_7        ' / "
эти клавиши не подчиняются общей логике модификации шифтом, как буквы A..Z,
поэтому их нужно обрабатывать отдельно
-------------------------------------------------------------
Плюс ко всему, сделаем полную трансляцию кодов в соответствии с раскладкой писи клавиатуры
в лат режиме - qwerty
в рус режиме - йцукен
*/

const char CBKVKBDView::LatShiftDigitTable[10] =
{
//  0    1    2    3    4    5    6    7    8    9
	')', '!', '@', '#', '$', '%', '^', '&', '*', '('
};
const char CBKVKBDView::RusShiftDigitTable[10] =
{
//  0    1    2    3    4    5    6    7    8    9
	')', '!', '"', '#', '$', '%', '&', '\'', '*', '('
};
const char CBKVKBDView::RusAlphaBetTable[26] =
{
//  A    B    C    D    E    F    G    H    I    J    K    L    M
	'F', 'I', 'S', 'W', 'U', 'A', 'P', 'R', '{', 'O', 'L', 'D', 'X',
//  N    O    P    Q    R    S    T    U    V    W    X    Y    Z
	'T', '}', 'Z', 'J', 'K', 'Y', 'E', 'G', 'M', 'C', '~', 'N', 'Q'
};
// массивы отличаются друг от друга всего тремя числами.
// это массив для маленьких букв шщч
const char CBKVKBDView::RusAlphaBetTableShift[26] =
{
//  A    B    C    D    E    F    G    H    I    J    K    L    M
	'F', 'I', 'S', 'W', 'U', 'A', 'P', 'R', '[', 'O', 'L', 'D', 'X',
//  N    O    P    Q    R    S    T    U    V    W    X    Y    Z
	'T', ']', 'Z', 'J', 'K', 'Y', 'E', 'G', 'M', 'C', '^', 'N', 'Q'
};
// трансляция цифробуквенных клавиш в лат режиме
int CBKVKBDView::LatModeTranslation(const int key) const
{
	if (GetShiftStatus())
	{
		// если нажат шифт
		switch (key)
		{
			case VK_OEM_PLUS:
				return 053;     // +

			case VK_OEM_COMMA:
				return 074;     // <

			case VK_OEM_MINUS:
				return 0137;    // _

			case VK_OEM_PERIOD:
				return 076;     // >

			case VK_OEM_1:
				return 072;     // :

			case VK_OEM_2:
				return 077;     // ?

			case VK_OEM_3:
				return 0176;    // ~

			case VK_OEM_4:
				return 0173;    // {

			case VK_OEM_5:
				return 0174;    // |

			case VK_OEM_6:
				return 0175;    // }

			case VK_OEM_7:
				return 042;     // "
		}

		if (060 <= key && key <= 071)
		{
			return LatShiftDigitTable[key - 060];
		}
	}
	else
	{
		// если не нажат
		switch (key)
		{
			case VK_OEM_PLUS:
				return 075;     // =

			case VK_OEM_COMMA:
				return 054;     // ,

			case VK_OEM_MINUS:
				return 055;     // -

			case VK_OEM_PERIOD:
				return 056;     // .

			case VK_OEM_1:
				return 073;     // ;

			case VK_OEM_2:
				return 057;     // /

			case VK_OEM_3:
				return 0140;    // `

			case VK_OEM_4:
				return 0133;    // [

			case VK_OEM_5:
				return 0134;    // обратный слеш '\'

			case VK_OEM_6:
				return 0135;    // ]

			case VK_OEM_7:
				return 047;     // '
		}

		if (060 <= key && key <= 071)
		{
			return key;
		}
	}

	// A-Z передаются без изменений
	if (0101 <= key && key <= 0132)
	{
		return key;
	}

	return -1;
}

// TODO: проблема с вопросительным знаком в лат раскладке. Для него просто нет кнопок.
// подумать, как переразместить символы на кнопках.
int CBKVKBDView::LatModeTranslationJCUK(const int key) const
{
	if (GetShiftStatus())   // если нажат шифт
	{
		if (060 <= key && key <= 071)
		{
			return RusShiftDigitTable[key - 060];
		}

		// трансляция A-Z
		if (0101 <= key && key <= 0132)
		{
			if (g_Config.m_bBKKeyboard || GetCapitalStatus()) // если заглавные буквы или эмулируем клавиатуру БК
			{
				// то маленькие буквы
				return RusAlphaBetTable[key - 0101];
			}

			// если не заглавные и не эмулируем клавиатуру БК и нажат шифт
			return RusAlphaBetTableShift[key - 0101]; // то большие буквы
		}

		switch (key)
		{
			case VK_OEM_PLUS:
				return 053;     // +

			case VK_OEM_COMMA:
				return 0102;    // б

			case VK_OEM_MINUS:
				return 072;     // :

			case VK_OEM_PERIOD:
				return (g_Config.m_bBKKeyboard || GetCapitalStatus()) ? 0140 : 0100;    // ю

			case VK_OEM_1:
				return 0126;    // ж

			case VK_OEM_2:
				return 054;     // ,

			case VK_OEM_3:
				return 076;     // >

			case VK_OEM_4:
				return 0110;    // х

			case VK_OEM_5:
				return 073;     // ;

			case VK_OEM_6:
				return (g_Config.m_bBKKeyboard || GetCapitalStatus()) ? 0177 : 0137;    // ъ

			case VK_OEM_7:
				return (g_Config.m_bBKKeyboard || GetCapitalStatus()) ? 0174 : 0134;    // э
		}
	}
	else  // если не нажат
	{
		if (060 <= key && key <= 071)
		{
			return key;
		}

		// трансляция A-Z
		if (0101 <= key && key <= 0132)
		{
			if (GetCapitalStatus()) // если заглавные буквы
			{
				// то большие буквы
				return RusAlphaBetTableShift[key - 0101];
			}

			//маленькие буквы
			return RusAlphaBetTable[key - 0101];
		}

		switch (key)
		{
			case VK_OEM_PLUS:
				return 075;     // =

			case VK_OEM_COMMA:
				return 0102;    // Б

			case VK_OEM_MINUS:
				return 055;     // -

			case VK_OEM_PERIOD:
				return GetCapitalStatus() ? 0100 : 0140;    // Ю

			case VK_OEM_1:
				return 0126;    // Ж

			case VK_OEM_2:
				return 056;     // .

			case VK_OEM_3:
				return 074;     // <

			case VK_OEM_4:
				return 0110;    // Х

			case VK_OEM_5:
				return 057;     // /

			case VK_OEM_6:
				return GetCapitalStatus() ? 0137 : 0177;    // Ъ

			case VK_OEM_7:
				return GetCapitalStatus() ? 0134 : 0174;    // Э
		}
	}

	return -1;
}

// трансляция цифробуквенных клавиш в рус режиме
int CBKVKBDView::RusModeTranslation(const int key) const
{
	if (GetShiftStatus())   // если нажат шифт
	{
		if (060 <= key && key <= 071)
		{
			return RusShiftDigitTable[key - 060];
		}

		// трансляция A-Z
		if (0101 <= key && key <= 0132)
		{
			if (g_Config.m_bBKKeyboard || GetCapitalStatus()) // если заглавные буквы или эмулируем клавиатуру БК
			{
				// то маленькие буквы
				return RusAlphaBetTableShift[key - 0101];
			}

			// если не заглавные и не эмулируем клавиатуру БК и нажат шифт
			return RusAlphaBetTable[key - 0101]; // то большие буквы
		}

		switch (key)
		{
			case VK_OEM_PLUS:
				return 053;     // +

			case VK_OEM_COMMA:
				return 0102;    // б

			case VK_OEM_MINUS:
				return 040;    // свободное место, не знай какой символ назначить

			case VK_OEM_PERIOD:
				return (g_Config.m_bBKKeyboard || GetCapitalStatus()) ? 0100 : 0140;    // ю

			case VK_OEM_1:
				return 0126;    // ж

			case VK_OEM_2:
				return 054;     // ,

			case VK_OEM_3:
				return 072;     // :

			case VK_OEM_4:
				return 0110;    // х

			case VK_OEM_5:
				return 077;     // ?

			case VK_OEM_6:
				return (g_Config.m_bBKKeyboard || GetCapitalStatus()) ? 0137 : 0177;    // ъ

			case VK_OEM_7:
				return (g_Config.m_bBKKeyboard || GetCapitalStatus()) ? 0134 : 0174;    // э
		}
	}
	else  // если не нажат
	{
		if (060 <= key && key <= 071)
		{
			return key;
		}

		// трансляция A-Z
		if (0101 <= key && key <= 0132)
		{
			if (GetCapitalStatus()) // если заглавные буквы
			{
				// то большие буквы
				return RusAlphaBetTable[key - 0101];
			}

			//маленькие буквы
			return RusAlphaBetTableShift[key - 0101];
		}

		switch (key)
		{
			case VK_OEM_PLUS:
				return 075;     // =

			case VK_OEM_COMMA:
				return 0102;    // Б

			case VK_OEM_MINUS:
				return 055;     // -

			case VK_OEM_PERIOD:
				return GetCapitalStatus() ? 0140 : 0100;    // Ю

			case VK_OEM_1:
				return 0126;    // Ж

			case VK_OEM_2:
				return 056;     // .

			case VK_OEM_3:
				return 073;     // ;

			case VK_OEM_4:
				return 0110;    // Х

			case VK_OEM_5:
				return 057;     // /

			case VK_OEM_6:
				return GetCapitalStatus() ? 0177 : 0137;    // Ъ

			case VK_OEM_7:
				return GetCapitalStatus() ? 0174 : 0134;    // Э
		}
	}

	return -1;
}

void CBKVKBDView::SetKeyboardStatus(const STATUS_FIELD pane, const bool set)
{
	switch (pane)
	{
		case STATUS_FIELD::KBD_XLAT:
			SetXLatStatus(set);
			break;

		case STATUS_FIELD::KBD_CAPS:
			SetCapitalStatus(set);
			break;

		case STATUS_FIELD::KBD_AR2:
			SetAR2Status(set);
			break;

		case STATUS_FIELD::KBD_SU:
			SetSUStatus(set);
			break;

		case STATUS_FIELD::KBD_SHIFT:
			SetShiftStatus(set);
			break;
	}

	OutKeyboardStatus(pane);
}

bool CBKVKBDView::GetKeyboardStatus(const STATUS_FIELD pane) const
{
	switch (pane)
	{
		case STATUS_FIELD::KBD_XLAT:
			return GetXLatStatus();

		case STATUS_FIELD::KBD_CAPS:
			return GetCapitalStatus();

		case STATUS_FIELD::KBD_AR2:
			return GetAR2Status();

		case STATUS_FIELD::KBD_SU:
			return GetSUStatus();

		case STATUS_FIELD::KBD_SHIFT:
			return GetShiftStatus();
	}

	return false;
}

void CBKVKBDView::OutKeyboardStatus(const STATUS_FIELD pane) const
{
	CString str;

	switch (pane)
	{
		case STATUS_FIELD::KBD_XLAT:
			str = GetXLatStatus() ? _T("РУС") : _T("ЛАТ");
			break;

		case STATUS_FIELD::KBD_CAPS:
			str = GetCapitalStatus() ? _T("ЗАГЛ") : _T("СТР");
			break;

		case STATUS_FIELD::KBD_AR2:
			str = GetAR2Status() ? _T("АР2") : _T("");
			break;

		case STATUS_FIELD::KBD_SU:
			str = GetSUStatus() ? _T("СУ") : _T("");
			break;

		default:
			return;
	}

	GetParentFrame()->SendMessage(WM_OUTKBDSTATUS, static_cast<WPARAM>(pane), reinterpret_cast<LPARAM>(&str));
}
#endif