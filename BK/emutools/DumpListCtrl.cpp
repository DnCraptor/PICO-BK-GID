#include "pch.h"
#include "DumpListCtrl.h"
#include "MemDumpView.h"
#include "Config.h"
#include "Screen_Sizes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


constexpr auto MAX_DUMP_COLS_WORD = 4;
constexpr auto MAX_DUMP_COLS_BYTE = 8;
constexpr auto COLUMN_WIDTH_ADDR  = 60;
constexpr auto COLUMN_WIDTH_WORD  = 68;
constexpr auto COLUMN_WIDTH_BYTE  = 34;
constexpr auto COLUMN_WIDTH_TXT   = 70;

IMPLEMENT_DYNAMIC(CDumpListCtrl, CMultiEditListCtrl)

CDumpListCtrl::CDumpListCtrl()
	: m_nDmpWndAddr(0)
	, m_nDmpWndLen(65536)
	, m_nDmpWndEndAddr(65536)
	, m_DisplayMode(DUMP_DISPLAY_MODE::DD_UNINIT)
	, m_nDumpAddr(0)
	, m_nBottomAddr(0)
{
}


CDumpListCtrl::~CDumpListCtrl()
    = default;

BEGIN_MESSAGE_MAP(CDumpListCtrl, CMultiEditListCtrl)
	ON_WM_SIZE()
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_NOTIFY_REFLECT(LVN_KEYDOWN, &CDumpListCtrl::OnLvnKeydown)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CDumpListCtrl::OnLvnItemchanged)
END_MESSAGE_MAP()


void CDumpListCtrl::SetDisplayMode(DUMP_DISPLAY_MODE mode, bool bFill)
{
	{
		std::lock_guard<std::mutex> lk(m_lock);
		if (m_DisplayMode != mode)
		{
			// сперва удалим всё
			DeleteAllItems();
			// потом удалим все столбцы
			CHeaderCtrl *pHeader = GetHeaderCtrl();
			int n = pHeader->GetItemCount();

			for (int i = 0; i < n; ++i)
			{
				DeleteColumn(0);
			}

			m_DisplayMode = mode;
			SetHeader();
		}
		SetEditAddress();
	}

	if (bFill)
	{
		AdjustRows(); // заполняем всё доступное пространство строками
		AdjustScroll();
	}
}

void CDumpListCtrl::SetAddress(const uint16_t addr)
{
	m_nDumpAddr = addr;

	if (m_nDumpAddr < m_nDmpWndAddr)
	{
		m_nDumpAddr = m_nDmpWndAddr;
	}
	else if (m_nDumpAddr >= m_nDmpWndEndAddr)
	{
		m_nDumpAddr = m_nDmpWndEndAddr - 8;
	}
}

uint16_t CDumpListCtrl::GetAddress() const
{
	return m_nDumpAddr;
}

void CDumpListCtrl::Init(DUMP_DISPLAY_MODE mode)
{
	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_SNAPTOGRID);
	SetDisplayMode(mode, false);
}

void CDumpListCtrl::AdjustRows()
{
	const int nRows = GetItemCount();
	const int n = GetCountPerPage();

	if (n == -1 || n == nRows)
	{
		return;
	}

	if (m_lock.try_lock())
	{
		// если рисуем или делаются изменения - надо подождать
		if (n < nRows)
		{
			// надо удалить лишние строки
			for (int i = nRows - 1; i >= n; i--)
			{
				DeleteItem(i);
				m_nBottomAddr -= 010;

				if (m_nBottomAddr < m_nDmpWndAddr)
				{
					m_nBottomAddr += m_nDmpWndLen;
				}
			}
		}
		else if (n > nRows)
		{
			// надо добавить недостающие строки
			for (int i = nRows; i < n; ++i)
			{
				InsertNewLine(i);
			}
		}

		// иначе ничего делать не надо
		m_lock.unlock();
	}
}

void CDumpListCtrl::AdjustScroll()
{
	SetScrollRange(SB_VERT, m_nDmpWndAddr, m_nDmpWndEndAddr + 8, FALSE); // надо делать постоянно, видимо стандартные средства постоянно сбрасывают на свои значения
	SetScrollPos(SB_VERT, m_nDumpAddr, TRUE);
	ShowScrollBar(SB_VERT, TRUE);
}

void CDumpListCtrl::SetEditAddress()
{
	// в окошке адреса изменим значение
	GetParent()->SetDlgItemText(IDC_EDIT_MD_DUMPADDR, Global::WordToOctString(m_nDumpAddr));
}

void CDumpListCtrl::ChangeAddr()
{
	SetEditAddress();
	RefreshItems();
}

void CDumpListCtrl::SetHeader()
{
	const int nPixelW = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSX);
	CHeaderCtrl *pHeader = GetHeaderCtrl();
	HDITEM hditem;
	ZeroMemory(&hditem, sizeof(HDITEM));
	hditem.mask = HDI_FORMAT | HDI_WIDTH;
	hditem.fmt = HDF_CENTER | HDF_STRING | HDF_FIXEDWIDTH;
	hditem.cxy = ::MulDiv(COLUMN_WIDTH_ADDR, nPixelW, DEFAULT_DPIX);
	CString s(MAKEINTRESOURCE(IDS_DEBUG_ADDRESS));
	InsertColumn(DUMP_LISTW::W_ADDR, s, LVCFMT_CENTER | LVCFMT_FIXED_WIDTH, ::MulDiv(COLUMN_WIDTH_ADDR, nPixelW, DEFAULT_DPIX));
	pHeader->SetItem(DUMP_LISTW::W_ADDR, &hditem); // почему-то зафиксировать ширину первого столбца можно только так
	int col = DUMP_LISTW::W_COL1;   // применим метод из просто С. col - это счётчик цикла for, значение которого сохраняется

	// после выхода из цикла
	switch (m_DisplayMode)
	{
		case DUMP_DISPLAY_MODE::DD_WORD_VIEW:
		{
			// добавляем столбцы
			for (int n = 0; col <= DUMP_LISTW::W_COL4; ++col, n += 2)
			{
				s.Format(_T("%02d"), n);
				InsertColumn(col, s, LVCFMT_CENTER | LVCFMT_FIXED_WIDTH, ::MulDiv(COLUMN_WIDTH_WORD, nPixelW, DEFAULT_DPIX));
			}

			break;
		}

		case DUMP_DISPLAY_MODE::DD_BYTE_VIEW:
		{
			// добавляем столбцы
			for (int n = 0; col <= DUMP_LISTB::B_COL8; ++col, ++n)
			{
				s.Format(_T("%02d"), n);
				InsertColumn(col, s, LVCFMT_CENTER | LVCFMT_FIXED_WIDTH, ::MulDiv(COLUMN_WIDTH_BYTE, nPixelW, DEFAULT_DPIX));
			}

			break;
		}
	}

	// после отработки цикла for, i будет равно номеру следующей колонки, как раз то, что надо
	InsertColumn(col, _T("01234567"), LVCFMT_CENTER | LVCFMT_FIXED_WIDTH, ::MulDiv(COLUMN_WIDTH_TXT, nPixelW, DEFAULT_DPIX));
}

void CDumpListCtrl::InsertNewLine(const int row)
{
	const auto pDlg = reinterpret_cast<CMemDumpView *>(GetParent());
	uint16_t nAddr = m_nDumpAddr + row * 010;

	if (nAddr >= m_nDmpWndEndAddr)
	{
		nAddr -= m_nDmpWndLen;
	}

	CString str;
	Global::WordToOctString(nAddr, str), str += _T(':');
	InsertItem(row, str);
	EnableEdit(false, row, DUMP_LISTW::W_ADDR);
	SetItemBkColor(::GetSysColor(COLOR_BTNFACE), row, DUMP_LISTW::W_ADDR);
	str.Empty();
	int col = DUMP_LISTW::W_COL1;

	switch (m_DisplayMode)
	{
		case DUMP_DISPLAY_MODE::DD_WORD_VIEW:
			for (; col <= DUMP_LISTW::W_COL4; ++col)
			{
				const uint16_t nVal = pDlg->GetDebugMemDumpWord(nAddr);
				SetItemWithModified(nVal, row, col);
				str += Global::BKToSafeWIDEChar(LOBYTE(nVal));
				str += Global::BKToSafeWIDEChar(HIBYTE(nVal));
				nAddr += 2;

				if (nAddr >= m_nDmpWndEndAddr)
				{
					nAddr -= m_nDmpWndLen;
				}
			}

			break;

		case DUMP_DISPLAY_MODE::DD_BYTE_VIEW:
			for (; col <= DUMP_LISTB::B_COL8; ++col)
			{
				const uint8_t nVal = pDlg->GetDebugMemDumpByte(nAddr);
				SetItemWithModifiedByte(nVal, row, col);
				str += Global::BKToSafeWIDEChar(nVal);
				nAddr++;

				if (nAddr >= m_nDmpWndEndAddr)
				{
					nAddr -= m_nDmpWndLen;
				}
			}

			break;
	}

	SetItemWithModifiedASCII(str, row, col);
	EnableEdit(false, row, col);
	m_nBottomAddr = nAddr;
}

void CDumpListCtrl::AddrPlus(const int nValue)
{
	m_nDumpAddr += nValue;

	if (m_nDumpAddr >= m_nDmpWndEndAddr)
	{
		m_nDumpAddr -= m_nDmpWndLen;
	}
}

void CDumpListCtrl::AddrMinus(const int nValue)
{
	m_nDumpAddr -= nValue;

	if (m_nDumpAddr < m_nDmpWndAddr)
	{
		m_nDumpAddr += m_nDmpWndLen;
	}
}

void CDumpListCtrl::RefreshItems(uint16_t nNewAddr, bool bNewAddrIsValid)
{
	const int nRows = GetItemCount();

	if (nRows <= 0) // если нет никаких строк
	{
		return; // не обновлять
	}

	if (m_lock.try_lock())
	{
		if (bNewAddrIsValid) // если необходимо - переопределим адрес
		{
			SetAddress(nNewAddr & 0177770);
		}

		const auto pDlg = static_cast<CMemDumpView *>(GetParent());
		CString str;
		uint16_t nAddr = 0;

		switch (m_DisplayMode)
		{
			case DUMP_DISPLAY_MODE::DD_WORD_VIEW:
			{
				for (int row = 0; row < nRows; ++row)
				{
					nAddr = m_nDumpAddr + row * 010;

					if (nAddr >= m_nDmpWndEndAddr)
					{
						nAddr -= m_nDmpWndLen;
					}

					Global::WordToOctString(nAddr, str), str += _T(":");
					SetItemText(row, DUMP_LISTW::W_ADDR, str);
					str.Empty();

					for (int col = DUMP_LISTW::W_COL1; col <= DUMP_LISTW::W_COL4; ++col)
					{
						const uint16_t nVal = pDlg->GetDebugMemDumpWord(nAddr);
						SetItemWithModified(nVal, row, col);
						str += Global::BKToSafeWIDEChar(LOBYTE(nVal));
						str += Global::BKToSafeWIDEChar(HIBYTE(nVal));
						nAddr += 2;

						if (nAddr >= m_nDmpWndEndAddr)
						{
							nAddr -= m_nDmpWndLen;
						}
					}

					SetItemWithModifiedASCII(str, row, DUMP_LISTW::W_ASCII);
				}

				break;
			}

			case DUMP_DISPLAY_MODE::DD_BYTE_VIEW:
			{
				for (int row = 0; row < nRows; ++row)
				{
					nAddr = m_nDumpAddr + row * 010;

					if (nAddr >= m_nDmpWndEndAddr)
					{
						nAddr -= m_nDmpWndLen;
					}

					Global::WordToOctString(nAddr, str), str += _T(":");
					SetItemText(row, DUMP_LISTB::B_ADDR, str);
					str.Empty();

					for (int col = DUMP_LISTB::B_COL1; col <= DUMP_LISTB::B_COL8; ++col)
					{
						const uint8_t nVal = pDlg->GetDebugMemDumpByte(nAddr);
						SetItemWithModifiedByte(nVal, row, col);
						str += Global::BKToSafeWIDEChar(nVal);
						nAddr++;

						if (nAddr >= m_nDmpWndEndAddr)
						{
							nAddr -= m_nDmpWndLen;
						}
					}

					SetItemWithModifiedASCII(str, row, DUMP_LISTB::B_ASCII);
				}

				break;
			}
		}

		m_nBottomAddr = nAddr;
		m_lock.unlock();
	}

	AdjustScroll();
}



void CDumpListCtrl::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	auto pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int nSubItem = pNMLV->iSubItem;
	int nItem = pNMLV->iItem;
	const auto pDlg = static_cast<CMemDumpView *>(GetParent());

	switch (m_DisplayMode)
	{
		case DUMP_DISPLAY_MODE::DD_WORD_VIEW:
		{
			if (CDumpListCtrl::DUMP_LISTW::W_COL1 <= nSubItem && nSubItem <= CDumpListCtrl::DUMP_LISTW::W_COL4)
			{
				int nAddr = m_nDumpAddr + nItem * 010;
				int nPos = (nSubItem - CDumpListCtrl::DUMP_LISTW::W_COL1) * 2; // 2 - кол-во charов в wordе
				nAddr += nPos;

				if (nAddr >= m_nDmpWndEndAddr)
				{
					nAddr -= m_nDmpWndLen;
				}

				auto pstr = reinterpret_cast<CString *>(pNMLV->lParam);
				uint16_t value = Global::OctStringToWord(*pstr);
				CString str = GetItemText(nItem, CDumpListCtrl::DUMP_LISTW::W_ASCII);
				str.SetAt(nPos++, Global::BKToSafeWIDEChar(LOBYTE(value)));
				str.SetAt(nPos, Global::BKToSafeWIDEChar(HIBYTE(value)));
				pDlg->SetDebugMemDumpWord(nAddr, value);
				SetItemWithModified(value, nItem, nSubItem);
				SetItemWithModifiedASCII(str, nItem, CDumpListCtrl::DUMP_LISTW::W_ASCII);
			}

			break;
		}

		case DUMP_DISPLAY_MODE::DD_BYTE_VIEW:
		{
			if (CDumpListCtrl::DUMP_LISTB::B_COL1 <= nSubItem && nSubItem <= CDumpListCtrl::DUMP_LISTB::B_COL8)
			{
				int nAddr = m_nDumpAddr + nItem * 010;
				int nPos = (nSubItem - CDumpListCtrl::DUMP_LISTB::B_COL1);
				nAddr += nPos;

				if (nAddr >= m_nDmpWndEndAddr)
				{
					nAddr -= m_nDmpWndLen;
				}

				auto pstr = reinterpret_cast<CString *>(pNMLV->lParam);
				uint8_t value = LOBYTE(Global::OctStringToWord(*pstr));
				TCHAR ch = Global::BKToSafeWIDEChar(value);
				CString str = GetItemText(nItem, CDumpListCtrl::DUMP_LISTB::B_ASCII);
				str.SetAt(nPos, ch);
				pDlg->SetDebugMemDumpByte(nAddr, value);
				SetItemWithModifiedByte(value, nItem, nSubItem);
				SetItemWithModifiedASCII(str, nItem, CDumpListCtrl::DUMP_LISTB::B_ASCII);
			}

			break;
		}
	}

	*pResult = S_OK;
}


void CDumpListCtrl::OnSize(UINT nType, int cx, int cy)
{
	CMultiEditListCtrl::OnSize(nType, cx, cy);
	AdjustRows();
	AdjustScroll();
}


void CDumpListCtrl::OnLvnKeydown(NMHDR *pNMHDR, LRESULT *pResult)
{
	const auto pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	const int nOffs = GetItemCount() * 010;
	*pResult = 1;

	switch (pLVKeyDow->wVKey)
	{
		case VK_UP:
			AddrMinus(010);
			break;

		case VK_DOWN:
			AddrPlus(010);
			break;

		case VK_PRIOR:  //PgUp
			AddrMinus(nOffs);
			break;

		case VK_NEXT: //PgDn
			SetAddress(m_nBottomAddr);
			break;

		case VK_HOME:
			SetAddress(m_nDmpWndAddr);
			break;

		case VK_END:
			SetAddress(m_nDmpWndEndAddr - nOffs);
			break;

		default:
			*pResult = 0;
	}

	if (*pResult)
	{
		ChangeAddr();
	}
}


void CDumpListCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
	const int nOffs = GetItemCount() * 010;
	bool bChange = true;

	switch (nSBCode)
	{
		case SB_LINEUP:
			AddrMinus(010);
			break;

		case SB_LINEDOWN:
			AddrPlus(010);
			break;

		case SB_PAGEUP:
			AddrMinus(nOffs);
			break;

		case SB_PAGEDOWN:
			SetAddress(m_nBottomAddr);
			break;

		case SB_TOP:
			SetAddress(m_nDmpWndAddr);
			break;

		case SB_BOTTOM:
			SetAddress(m_nDmpWndEndAddr - nOffs);
			break;

		case SB_THUMBTRACK:
			SetAddress(nPos & 0177770);
			break;

		default:
			bChange = false;
	}

	if (bChange)
	{
		ChangeAddr();
	}
}


BOOL CDumpListCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	const int nOffs = (nFlags & MK_CONTROL) ? GetItemCount() * 010 : 010;

	if (zDelta < 0)
	{
		AddrPlus(nOffs);
		ChangeAddr();
	}
	else if (zDelta > 0)
	{
		AddrMinus(nOffs);
		ChangeAddr();
	}

	return TRUE;
}


BOOL CDumpListCtrl::PreCreateWindow(CREATESTRUCT &cs)
{
	// попытка сделать принудительный показ вертикального скроллбара.
	// но всё равно не работает, вертикальный скроллбар рисуется поверх правой колонки.
	cs.style |= WS_VSCROLL | WS_CLIPSIBLINGS;
	return CMultiEditListCtrl::PreCreateWindow(cs);
}

void CDumpListCtrl::SetItemWithModifiedByte(uint8_t byte, int nItem, int nSubitem, bool bColored)
{
	ITEM_INFO *p = &(m_itemsTable[nItem][nSubitem]);

	// если этот итем в данный момент редактируется, то ничего не делать
	if (p->bEdited)
	{
		return;
	}

	CString str;

	if (bColored)
	{
		// если значение изменилось
		if (p->nValue != byte)
		{
			p->nValue = byte;
			p->bModified = true; // пометим, что оно изменилось
			p->clrText = MODIFIED_COLOR; // выведем его красным цветом
			Global::ByteToOctString(byte, str);
			SetItemText(nItem, nSubitem, str);
		}
		// если не менялось, но в прошлый раз было выведено красным цветом
		else if (p->bModified)
		{
			p->bModified = false; // пометим, что больше не нужно ничего рисовать, пока значение снова не изменится
			p->clrText = p->clrModText; // выведем его обычным цветом
			Global::ByteToOctString(byte, str);
			SetItemText(nItem, nSubitem, str);
		}

		// иначе - вообще не надо ничего выводить.
	}
	else
	{
		// если значение изменилось
		if (p->nValue != byte)
		{
			p->nValue = byte;
			p->bModified = false;
			// выведем его обычным цветом
			Global::ByteToOctString(byte, str);
			SetItemText(nItem, nSubitem, str);
		}

		// если не менялось - не выводим ничего
	}
}


