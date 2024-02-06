// MultiEditListCtrl.cpp : implementation file
//

#include "pch.h"
#include "MultiEditListCtrl.h"
#include "Config.h"
#include "BKMessageBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMultiEdit

IMPLEMENT_DYNAMIC(CMultiEdit, CEdit)

CMultiEdit::CMultiEdit(CMultiEditListCtrl *pListCtrl, const int nItem, const int nSubItem)
	: m_pParent(pListCtrl)
	, m_nItem(nItem)
	, m_nSubItem(nSubItem)
	, m_bFirstPress(true)
{
}

CMultiEdit::~CMultiEdit()
    = default;

BEGIN_MESSAGE_MAP(CMultiEdit, CEdit)
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_CHAR()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


void CMultiEdit::OnKillFocus(CWnd *pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
// если закомментировать эти строки, то можно будет редактировать сразу несколько элементов.
	SetItem();
	delete this;
}


void CMultiEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch (nChar)
	{
		case VK_RETURN: // ввод - принимаем значение
			SetItem();
			[[fallthrough]];
		case VK_ESCAPE: // Esc - отмена редактирования
			m_pParent->SetEndEdit(m_nItem, m_nSubItem);
			delete this;
			return;

		case VK_LEFT:   // стрелки и HOME, END - обработка как обычно
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
		case VK_HOME:
		case VK_END:
			break;

		default:        // все остальные клавиши - при первом нажатии стереть всё

			// тут много лишних клавиш попадает на это действие: F1-F12 и т.п.
			if (m_bFirstPress)
			{
				SetWindowText(_T(""));
			}
	}

	m_bFirstPress = false;
	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CMultiEdit::OnLButtonDown(UINT nFlags, CPoint point)
{
	// если хоть что-то мышкой делали, то ничего стирать уже не надо
	m_bFirstPress = false;
	CEdit::OnLButtonDown(nFlags, point);
}


void CMultiEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_RETURN)
	{
		return; // запрет обработки по-умолчанию
	}

	// запрещаем цифры 8 и 9
//  if (nChar == 0x38 || nChar == 0x39)
//  {
//      return;
//  }
// нельзя запрещать. иначе мы не можем редактировать всякие вещи
	CEdit::OnChar(nChar, nRepCnt, nFlags);
}


void CMultiEdit::SetItem()
{
	CString strText;
	GetWindowText(strText);
	auto pLVNotyfy = std::make_unique<NMLISTVIEW>();

	if (pLVNotyfy)
	{
		ZeroMemory(pLVNotyfy.get(), sizeof(NMLISTVIEW));
		pLVNotyfy->hdr.hwndFrom = m_pParent->GetSafeHwnd();
		pLVNotyfy->hdr.idFrom = m_pParent->GetDlgCtrlID();
		pLVNotyfy->hdr.code = LVN_ITEMCHANGING;
		pLVNotyfy->iItem = m_nItem;
		pLVNotyfy->iSubItem = m_nSubItem;
		pLVNotyfy->lParam = reinterpret_cast<LPARAM>(&strText); // эту строку можно передавать вот так, потому что SendMessage ждёт, пока
		// функция, которая там принимает эту строку, обработает её, и не использует на неё указатели.
		CWnd *pParent = m_pParent->GetParent();
		ASSERT(pParent);

		if (pParent)
		{
			LRESULT bProtect = pParent->SendMessage(WM_NOTIFY, pLVNotyfy->hdr.idFrom, reinterpret_cast<LPARAM>(pLVNotyfy.get()));

			if (!bProtect)
			{
				pLVNotyfy->hdr.code = LVN_ITEMCHANGED;
				m_pParent->SetEndEdit(m_nItem, m_nSubItem); // флаг нужно сбросить перед подачей сообщения
				// а то измения отображаются на экране
				pParent->SendMessage(WM_NOTIFY, pLVNotyfy->hdr.idFrom, reinterpret_cast<LPARAM>(pLVNotyfy.get()));
			}
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CMultiEditListCtrl

IMPLEMENT_DYNAMIC(CMultiEditListCtrl, CListCtrl)

CMultiEditListCtrl::CMultiEditListCtrl()
	: m_pEdit(nullptr)
	, m_bDigits(true) // принимаем только цифры по умолчанию
	, m_StdBgColor(0)
	, m_StdColor(0)
{
}

CMultiEditListCtrl::~CMultiEditListCtrl()
{
	ClearColorTable();
}


/////////////////////////////////////////////////////////////////////////////
// CMultiEditListCtrl message handlers

BEGIN_MESSAGE_MAP(CMultiEditListCtrl, CListCtrl)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CMultiEditListCtrl::OnDblclk)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CMultiEditListCtrl::OnCustomDraw)
	ON_NOTIFY_REFLECT(LVN_INSERTITEM, &CMultiEditListCtrl::OnLvnInsertitem)
	ON_NOTIFY_REFLECT(LVN_DELETEITEM, &CMultiEditListCtrl::OnLvnDeleteitem)
	ON_NOTIFY_REFLECT(LVN_DELETEALLITEMS, &CMultiEditListCtrl::OnLvnDeleteallitems)
END_MESSAGE_MAP()


void CMultiEditListCtrl::PreSubclassWindow()
{
	LOGFONT lfnt;
	GetFont()->GetLogFont(&lfnt);
	m_font.CreateFontIndirect(&lfnt);
	m_StdColor = ::GetSysColor(COLOR_BTNTEXT);
	m_StdBgColor = ::GetSysColor(COLOR_WINDOW);
	CListCtrl::PreSubclassWindow();
}


void CMultiEditListCtrl::OnDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	auto pNMLV = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	CPoint point(pNMLV->ptAction);
	UINT uFlags;
	HitTest(point, &uFlags);
	const int nRow = pNMLV->iItem;
	const int nCol = pNMLV->iSubItem;

	if ((uFlags & LVHT_ONITEMLABEL) && !(GetStyle() & LVS_EDITLABELS))
	{
		if (m_itemsTable[nRow][nCol].bEditable)
		{
			m_itemsTable[nRow][nCol].bEdited = true;
			// If label click - create edit ctrl for item
			CRect rcSubItem;
			GetSubItemRect(nRow, nCol, LVIR_LABEL, rcSubItem);
			ClientToScreen(&rcSubItem);
			m_pEdit = new CMultiEdit(this, nRow, nCol);

			if (m_pEdit)
			{
				DWORD dwStyle = WS_VISIBLE | WS_POPUP | WS_BORDER | ES_WANTRETURN | ES_MULTILINE;

				if (m_bDigits)
				{
					dwStyle |= ES_NUMBER;
				}

				m_pEdit->CreateEx(0, _T("edit"), nullptr, dwStyle, rcSubItem, this, 0);
				m_pEdit->SetFont(&m_font);
				m_pEdit->SetWindowText(GetItemText(nRow, nCol));
				m_pEdit->SetFocus();
			}
			else
			{
				g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
			}
		}
	}

	*pResult = S_OK;
}

void CMultiEditListCtrl::SetEndEdit(const int nItem, const int nSubitem)
{
	m_itemsTable[nItem][nSubitem].bEdited = false;
}


void CMultiEditListCtrl::OnCustomDraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	auto pCustomDraw = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
	*pResult = CDRF_DODEFAULT;

	switch (pCustomDraw->nmcd.dwDrawStage)
	{
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;

		case CDDS_ITEMPREPAINT:
			*pResult = CDRF_NOTIFYSUBITEMDRAW;
			break;

		case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
		{
			ITEM_INFO &ii = m_itemsTable[pCustomDraw->nmcd.dwItemSpec][pCustomDraw->iSubItem];
			pCustomDraw->clrText = ii.clrText;
			pCustomDraw->clrTextBk = ii.clrTextBk;
			// *pResult = CDRF_NEWFONT;
			break;
		}
	}
}


void CMultiEditListCtrl::OnLvnInsertitem(NMHDR *pNMHDR, LRESULT *pResult)
{
	auto pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int n = GetColumnCount();
	std::vector<ITEM_INFO> ptr(n);

	for (auto &i : ptr)
	{
		i.clrText = m_StdColor;
		i.clrTextBk = m_StdBgColor;
	}

	if (pNMLV->iItem >= int(m_itemsTable.size()))
	{
		m_itemsTable.push_back(ptr);
	}
	else
	{
		m_itemsTable.insert(m_itemsTable.begin() + pNMLV->iItem, ptr);
	}

	*pResult = S_OK;
}

void CMultiEditListCtrl::OnLvnDeleteitem(NMHDR *pNMHDR, LRESULT *pResult)
{
	auto pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int item = pNMLV->iItem;

	if (item < GetItemCount())
	{
		if (item < int(m_itemsTable.size()))
		{
			m_itemsTable[item].clear();
			m_itemsTable.erase(m_itemsTable.begin() + item);
		}
	}

	*pResult = S_OK;
}


void CMultiEditListCtrl::OnLvnDeleteallitems(NMHDR *pNMHDR, LRESULT *pResult)
{
	//auto pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	ClearColorTable();
	*pResult = S_OK;
}


void CMultiEditListCtrl::InitColorTable()
{
}


void CMultiEditListCtrl::ClearColorTable()
{
	for (auto &n : m_itemsTable)
	{
		n.clear();
	}

	m_itemsTable.clear();
}


int CMultiEditListCtrl::GetColumnCount() const
{
	return GetHeaderCtrl()->GetItemCount();
}


void CMultiEditListCtrl::EnableEdit(const bool bEnable, const int nRow, const int nColumn)
{
	const int nItemCount = GetItemCount();
	ASSERT(nItemCount - 1 >= nRow);

	if (nItemCount - 1 < nRow)
	{
		return;
	}

	const int nColumnCount = GetColumnCount();
	ASSERT(nColumnCount - 1 >= nColumn);

	if (nColumnCount - 1 < nColumn)
	{
		return;
	}

	m_itemsTable[nRow][nColumn].bEditable = bEnable;
}


void CMultiEditListCtrl::EnableRowEdit(const bool bEnable, const int nRow)
{
	const int nItemCount = GetItemCount();
	ASSERT(nItemCount - 1 >= nRow);

	if (nItemCount - 1 < nRow)
	{
		return;
	}

	const int nColumnCount = GetColumnCount();

	for (int n = 0; n < nColumnCount; ++n)
	{
		m_itemsTable[nRow][n].bEditable = bEnable;
	}
}


void CMultiEditListCtrl::EnableColumnEdit(const bool bEnable, const int nColumn)
{
	const int nColumnCount = GetColumnCount();
	ASSERT(nColumnCount - 1 >= nColumn);

	if (nColumnCount - 1 < nColumn)
	{
		return;
	}

	const int nRowCount = GetItemCount();

	for (int n = 0; n < nRowCount; ++n)
	{
		m_itemsTable[n][nColumn].bEditable = bEnable;
	}
}


void CMultiEditListCtrl::SetItemColor(const COLORREF col, const int nRow, const int nColumn)
{
	const int nItemCount = GetItemCount();
	ASSERT(nItemCount - 1 >= nRow);

	if (nItemCount - 1 < nRow)
	{
		return;
	}

	const int nColumnCount = GetColumnCount();
	ASSERT(nColumnCount - 1 >= nColumn);

	if (nColumnCount - 1 < nColumn)
	{
		return;
	}

	ITEM_INFO *p = &(m_itemsTable[nRow][nColumn]);
	p->clrText = col;
	p->clrModText = col;
	p->bModified = true;
}


void CMultiEditListCtrl::SetRowColor(const COLORREF col, const int nRow)
{
	const int nItemCount = GetItemCount();
	ASSERT(nItemCount - 1 >= nRow);

	if (nItemCount - 1 < nRow)
	{
		return;
	}

	const int nColumnCount = GetColumnCount();

	for (int n = 0; n < nColumnCount; ++n)
	{
		ITEM_INFO *p = &(m_itemsTable[nRow][n]);
		p->clrText = col;
		p->clrModText = col;
		p->bModified = true;
	}
}


void CMultiEditListCtrl::SetColumnColor(const COLORREF col, const int nColumn)
{
	const int nColumnCount = GetColumnCount();
	ASSERT(nColumnCount - 1 >= nColumn);

	if (nColumnCount - 1 < nColumn)
	{
		return;
	}

	const int nRowCount = GetItemCount();

	for (int n = 0; n < nRowCount; ++n)
	{
		ITEM_INFO *p = &(m_itemsTable[n][nColumn]);
		p->clrText = col;
		p->clrModText = col;
		p->bModified = true;
	}
}


void CMultiEditListCtrl::SetItemBkColor(const COLORREF col, const int nRow, const int nColumn)
{
	const int nItemCount = GetItemCount();
	ASSERT(nItemCount - 1 >= nRow);

	if (nItemCount - 1 < nRow)
	{
		return;
	}

	const int nColumnCount = GetColumnCount();
	ASSERT(nColumnCount - 1 >= nColumn);

	if (nColumnCount - 1 < nColumn)
	{
		return;
	}

	ITEM_INFO *p = &(m_itemsTable[nRow][nColumn]);
	p->clrTextBk = col;
	p->clrModTextBk = col;
	p->bModified = true;
}


void CMultiEditListCtrl::SetRowBkColor(const COLORREF col, const int nRow)
{
	const int nItemCount = GetItemCount();
	ASSERT(nItemCount - 1 >= nRow);

	if (nItemCount - 1 < nRow)
	{
		return;
	}

	const int nColumnCount = GetColumnCount();

	for (int n = 0; n < nColumnCount; ++n)
	{
		ITEM_INFO *p = &(m_itemsTable[nRow][n]);
		p->clrTextBk = col;
		p->clrModTextBk = col;
		p->bModified = true;
	}
}


void CMultiEditListCtrl::SetColumnBkColor(const COLORREF col, const int nColumn)
{
	const int nColumnCount = GetColumnCount();
	ASSERT(nColumnCount - 1 >= nColumn);

	if (nColumnCount - 1 < nColumn)
	{
		return;
	}

	const int nRowCount = GetItemCount();

	for (int n = 0; n < nRowCount; ++n)
	{
		ITEM_INFO *p = &(m_itemsTable[n][nColumn]);
		p->clrTextBk = col;
		p->clrModTextBk = col;
		p->bModified = true;
	}
}


void CMultiEditListCtrl::SetItemWithModified(const uint16_t word, const int nItem, const int nSubitem, bool bColored)
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
		if (p->nValue != word)
		{
			p->nValue = word;
			p->bModified = true; // пометим, что оно изменилось
			p->clrText = MODIFIED_COLOR;    // выведем его красным цветом
			Global::WordToOctString(word, str);
			SetItemText(nItem, nSubitem, str);
		}
		// если не менялось, но в прошлый раз было выведено красным цветом
		else if (p->bModified)
		{
			p->bModified = false; // и пометим, что больше не нужно ничего рисовать, пока значение снова не изменится
			p->clrText = p->clrModText;     // восстановим его обычный цвет
			Global::WordToOctString(word, str);
			SetItemText(nItem, nSubitem, str);
		}

		// иначе - вообще не надо ничего выводить.
	}
	else
	{
		// если значение изменилось
		if (p->nValue != word)
		{
			p->nValue = word;
			p->bModified = false;
			// выведем его обычным цветом
			Global::WordToOctString(word, str);
			SetItemText(nItem, nSubitem, str);
		}

		// если не менялось - не выводим ничего
	}
}

void CMultiEditListCtrl::SetItemWithModifiedASCII(const CString &str, const int nItem, const int nSubitem)
{
// недостаток метода - если в строке изменился хоть байт - подсвечивается вся строка
	ITEM_INFO *p = &(m_itemsTable[nItem][nSubitem]);

	// если этот итем в данный момент редактируется, то ничего не делать
	if (p->bEdited)
	{
		return;
	}

	if (GetItemText(nItem, nSubitem) != str)
	{
		p->bModified = true; // пометим, что оно изменилось
		p->clrText = MODIFIED_COLOR; // выведем его красным цветом
		SetItemText(nItem, nSubitem, str);
	}
	// если не менялось, но в прошлый раз было выведено красным цветом
	else if (p->bModified)
	{
		p->bModified = false; // пометим, что больше не нужно ничего рисовать, пока значение снова не изменится
		p->clrText = p->clrModText; // выведем его обычным цветом
		SetItemText(nItem, nSubitem, str);
	}

	// иначе - вообще не надо ничего выводить.
}

