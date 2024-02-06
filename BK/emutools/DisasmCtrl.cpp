// DisasmCtrl.cpp : implementation file
//
#ifdef UI
#include "pch.h"
#include "resource.h"
#include "DisasmCtrl.h"
#include "Debugger.h"
#include "Config.h"
#include "BKMessageBox.h"
#include "Screen_Sizes.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDisasmCtrl

IMPLEMENT_DYNAMIC(CDisasmCtrl, CMultiEditListCtrl)

CDisasmCtrl::CDisasmCtrl()
	: m_pDebugger(nullptr)
	, m_nSelection(-1)
{
	m_hBPIcon = ::LoadIcon(::AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_DBG_BPT));
	m_hCurrIcon = ::LoadIcon(::AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_DBG_CUR));
}

CDisasmCtrl::~CDisasmCtrl()
{
	if (m_hBPIcon)
	{
		DestroyIcon(m_hBPIcon);
	}

	if (m_hCurrIcon)
	{
		DestroyIcon(m_hCurrIcon);
	}

	m_vItemsParam.clear();
}

BEGIN_MESSAGE_MAP(CDisasmCtrl, CMultiEditListCtrl)
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_MOUSEWHEEL()
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CDisasmCtrl::OnItemchanged)
	ON_NOTIFY_REFLECT(LVN_KEYDOWN, &CDisasmCtrl::OnKeydown)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CDisasmCtrl::OnCustomDraw)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CDisasmCtrl::OnNMDblclk)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDisasmCtrl message handlers

void CDisasmCtrl::Init()
{
	ModifyStyle(0, LVS_REPORT | LVS_OWNERDATA | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER | SBS_VERT);
	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_SNAPTOGRID);
	// Create Header
	int nPixelW = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSX);
	CString strHeader;
	InsertColumn(DISASM_LIST::COL_MARK, CString(MAKEINTRESOURCE(IDS_DEBUG_MARK)), LVCFMT_LEFT, ::MulDiv(20, nPixelW, DEFAULT_DPIX));
	InsertColumn(DISASM_LIST::COL_ADDR, CString(MAKEINTRESOURCE(IDS_DEBUG_ADDRESS)), LVCFMT_LEFT, ::MulDiv(50, nPixelW, DEFAULT_DPIX));
	InsertColumn(DISASM_LIST::COL_INSTR, CString(MAKEINTRESOURCE(IDS_DEBUG_COMMAND)), LVCFMT_LEFT, ::MulDiv(200, nPixelW, DEFAULT_DPIX));
	InsertColumn(DISASM_LIST::COL_COMMENT, CString(MAKEINTRESOURCE(IDS_DEBUG_COMMENTS)), LVCFMT_LEFT, ::MulDiv(150, nPixelW, DEFAULT_DPIX));
	AcceptDigits(false);
	EnableColumnEdit(false, DISASM_LIST::COL_MARK);
	EnableColumnEdit(false, DISASM_LIST::COL_ADDR);
	EnableColumnEdit(true, DISASM_LIST::COL_INSTR);
	EnableColumnEdit(false, DISASM_LIST::COL_COMMENT);
	OutContent();
}

void CDisasmCtrl::ChangeInstructionSet()
{
	m_pDebugger->InitMaps(true);
}

uint16_t CDisasmCtrl::GetCursorAddr() const
{
	int nRow = GetSelectionMark();
	return (nRow < 0) ? 0 : m_vItemsParam[nRow].nAddr;
}

uint16_t CDisasmCtrl::GetBottomAddr() const
{
	int nLast = GetItemCount() - 1;

	if (nLast < 0)
	{
		nLast = 0;
	}

	return m_vItemsParam[nLast].nAddr + (m_vItemsParam[nLast].nLen << 1);
}

uint16_t CDisasmCtrl::GetLineAddress(int nRow) const
{
	if (nRow < 0 || nRow >= GetItemCount())
	{
		return 0;
	}

	if (nRow == 0)
	{
		return g_Config.m_nDisasmAddr;
	}

	nRow--;
	return m_vItemsParam[nRow].nAddr + (m_vItemsParam[nRow].nLen << 1);
}



uint16_t CDisasmCtrl::StepBackward(int nSteps /*= 1*/) const
{
	uint16_t addr = g_Config.m_nDisasmAddr;
	addr -= static_cast<uint16_t>(sizeof(uint16_t)) * nSteps;
	g_Config.m_nDisasmAddr = addr;
	return addr;
}

uint16_t CDisasmCtrl::StepForward(int nSteps /*= 1*/) const
{
	uint16_t addr = g_Config.m_nDisasmAddr;

	if (nSteps < GetItemCount())
	{
		addr = m_vItemsParam[nSteps].nAddr;
	}
	else
	{
		addr = GetBottomAddr();
	}

	g_Config.m_nDisasmAddr = addr;
	return addr;
}

void CDisasmCtrl::AttachDebugger(CDebugger *pDebugger)
{
	m_pDebugger = pDebugger;
}

void CDisasmCtrl::OnSize(UINT nType, int cx, int cy)
{
	CListCtrl::OnSize(nType, cx, cy);
	OutContent();
}

void CDisasmCtrl::OutContent()
{
	if (m_lock.try_lock())  // Lock for recurse call preserve
	{
		// Delete All Items
		DeleteAllItems();
		m_vItemsParam.clear();
		const int nCount = GetCountPerPage();

		if (nCount != -1)
		{
			// Insert new items
			for (int i = 0; i < nCount; ++i)
			{
				ItemParam p{ 0177777, 0, _T("")};
				m_vItemsParam.emplace_back(p);
				InsertItem(i, LPSTR_TEXTCALLBACK);

				if (i == m_nSelection)
				{
					SetItemState(m_nSelection, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				}
			}
		}

		// Unlock
		m_lock.unlock();
	}
}


void CDisasmCtrl::DisasmStepUp(int nCount) const
{
	// сдвигаемся вверх
	const uint16_t addr = StepBackward(nCount);
	GetParent()->PostMessage(WM_DBG_TOP_ADDRESS_SET, static_cast<WPARAM>(addr));
}

void CDisasmCtrl::DisasmStepDn(int nCount) const
{
	// сдвигаемся вниз
	const uint16_t addr = StepForward(nCount);
	GetParent()->PostMessage(WM_DBG_TOP_ADDRESS_SET, static_cast<WPARAM>(addr));
}

void CDisasmCtrl::OnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	const auto pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int nSubItem = pNMLV->iSubItem;
	int nItem = pNMLV->iItem;

	// Change selection marker
	if (nItem != -1 && pNMLV->uNewState & LVIS_SELECTED)
	{
		m_nSelection = nItem;
	}

	if (nSubItem == DISASM_LIST::COL_INSTR)
	{
		auto pstr = reinterpret_cast<CString *>(pNMLV->lParam);
		int nAddr = m_vItemsParam[nItem].nAddr;
		// теперь тут нужно ассемблировать команду в строке по заданному адресу.
		uint16_t tmpCmdBuf[8] = { 0 };
		int len = m_pDebugger->AssembleCPUInstruction(nAddr, &tmpCmdBuf[0], pstr);

		if (len > 0)
		{
			for (int i = 0; i < len; ++i)
			{
				m_pDebugger->SetDebugMemDump(nAddr, tmpCmdBuf[i]);
				nAddr += 2;
			}

			Invalidate(FALSE);
		}
	}

	*pResult = S_OK;
}


void CDisasmCtrl::OnKeydown(NMHDR *pNMHDR, LRESULT *pResult)
{
	const auto pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	const int nLastItem = GetItemCount() - 1;
	const int nSel = GetSelectionMark();
	const int nCount = GetCountPerPage();

	if (nSel == 0)
	{
		if (pLVKeyDow->wVKey == VK_UP)
		{
			// если мы вверху и нажали кнопку вверх
			DisasmStepUp();
		}
		else if (pLVKeyDow->wVKey == VK_PRIOR)
		{
			// если мы вверху и нажали кнопку pgup
			DisasmStepUp(nCount);
		}

		RedrawItems(0, nLastItem); // и всё перерисовываем
	}
	else if (nSel == nLastItem)
	{
		if (pLVKeyDow->wVKey == VK_DOWN)
		{
			// если мы внизу и нажали кнопку вниз
			DisasmStepDn();
		}
		else if (pLVKeyDow->wVKey == VK_NEXT)
		{
			// если мы внизу и нажали кнопку pgdn
			DisasmStepDn(nCount);
		}

		RedrawItems(0, nLastItem); // и всё перерисовываем
	}

	*pResult = S_OK;
}


BOOL CDisasmCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	const int nLastItem = GetItemCount() - 1;

	if (zDelta < 0)
	{
		DisasmStepDn();
	}
	else if (zDelta > 0)
	{
		DisasmStepUp();
	}

	RedrawItems(0, nLastItem); // и всё перерисовываем
	return TRUE;
}

void CDisasmCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	const auto pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	const int nRow = pNMItemActivate->iItem;
	const int nCol = pNMItemActivate->iSubItem;

	if (nCol == DISASM_LIST::COL_MARK) // если кликнули по самому первому столбцу
	{
		if (m_pDebugger)
		{
			const uint16_t addr = m_vItemsParam[nRow].nAddr;

			if (!m_pDebugger->SetSimpleBreakpoint(addr))
			{
				m_pDebugger->RemoveBreakpoint(addr);
			}
		}

		Invalidate(FALSE);
	}
	else if (nCol == DISASM_LIST::COL_INSTR)
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
			// у strInstr надо удалить маркеры колоризации.
			CString &strInstr = m_vItemsParam[nRow].instr;

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

			m_pEdit->SetWindowText(strInstr);
			m_pEdit->SetFocus();
		}
		else
		{
			g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
		}
	}

	*pResult = S_OK;
}

void CDisasmCtrl::OnCustomDraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	const auto pLVCD = reinterpret_cast<NMLVCUSTOMDRAW *>(pNMHDR);
	*pResult = CDRF_DODEFAULT;

	switch (pLVCD->nmcd.dwDrawStage)
	{
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;

		case CDDS_ITEMPREPAINT:
		{
			// меняем цвет курсора выделения
			const auto nIndex = static_cast<int>(pLVCD->nmcd.dwItemSpec);

			if (GetItemState(nIndex, LVIS_SELECTED | LVIS_FOCUSED)
			        && ::GetFocus() == pLVCD->nmcd.hdr.hwndFrom)
			{
				pLVCD->clrTextBk = RGB(0xff, 0xff, 0); // RGB(0, 0xcc, 0xff);
				SetItemState(nIndex, 0, LVIS_SELECTED);
			}

			*pResult = CDRF_NOTIFYPOSTPAINT;
		}
		break;

//      case (CDDS_ITEMPREPAINT | CDDS_SUBITEM):
//          // здесь можно поменять текст конкретного итема
//          // только это всё равно медленнее работает, чем прорисовать всю строку за раз
//          switch (pLVCD->iSubItem)
//          {
//          case DISASM_LIST::COL_MARK:
//              break;
//          case DISASM_LIST::COL_ADDR:
//                  break;
//          case DISASM_LIST::COL_INSTR:
//              break;
//          }
//          break;

		case CDDS_ITEMPOSTPAINT:
			if (m_pDebugger && m_pDebugger->GetBoard()) //пока board не приаттачен, делать нечего, и ничего не работает
			{
				CDC *pDC = CDC::FromHandle(pLVCD->nmcd.hdc);
				// Get item index
				const auto nIndex = static_cast<int>(pLVCD->nmcd.dwItemSpec);
				// Дизассемблируем машинный код
				uint16_t instrOpcode[8];
				CString strInstruction;
				const uint16_t wLineAddr = GetLineAddress(nIndex);
				const int len = m_pDebugger->DebugInstruction(wLineAddr, strInstruction, instrOpcode);
				// Выводим маркер
				CRect rect;
				GetSubItemRect(nIndex, DISASM_LIST::COL_MARK, LVIR_LABEL, rect);

				if (m_pDebugger->IsBpeakpointAtAddress(wLineAddr))
				{
					::DrawIconEx(pDC->m_hDC, rect.left, rect.top, m_hBPIcon, 16, 16, 0, nullptr, DI_NORMAL);
				}

				if (m_pDebugger->CheckDebuggedLine(wLineAddr))
				{
					::DrawIconEx(pDC->m_hDC, rect.left, rect.top, m_hCurrIcon, 16, 16, 0, nullptr, DI_NORMAL);
				}

				m_vItemsParam[nIndex] = { wLineAddr, len, strInstruction };
				// Выводим адрес
				CString strTxt = Global::WordToOctString(wLineAddr);
				GetSubItemRect(nIndex, DISASM_LIST::COL_ADDR, LVIR_BOUNDS, rect);
				rect.left += 4; rect.right -= 4;
				pDC->SetTextColor(g_crDebugColorHighLighting[HLCOLOR_ADDRESS]);
				pDC->DrawText(strTxt, &rect, DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS);
				// Выводим инструкцию
				GetSubItemRect(nIndex, DISASM_LIST::COL_INSTR, LVIR_BOUNDS, rect);
				rect.left += 4; rect.right -= 4;
				pDC->SetTextColor(g_crDebugColorHighLighting[HLCOLOR_MNEMONIC]);
				DrawColoredText(pDC, rect, strInstruction);
				// Выводим комментарий. Это у нас просто машинные инструкции ассемблерной команды
				strTxt = Global::WordToOctString(instrOpcode[0]); // код инструкции у нас по любому всегда есть

				// а дальше от 0 до 2-х слов аргументов
				// для инструкций FIS если регистр PC, то 4 слова аргументов
				for (int i = 1; i < len; ++i)
				{
					strTxt += _T(' ') + Global::WordToOctString(instrOpcode[i]);
				}

				GetSubItemRect(nIndex, DISASM_LIST::COL_COMMENT, LVIR_BOUNDS, rect);
				rect.left += 4; rect.right -= 4;
				pDC->SetTextColor(g_crDebugColorHighLighting[HLCOLOR_DEFAULT]);
				pDC->DrawText(strTxt, &rect, DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS);
				*pResult = CDRF_SKIPDEFAULT;
			}

			break;
	}
}

void CDisasmCtrl::DrawColoredText(CDC *pDC, CRect &rect, CString &str)
{
	bool bEnd = false;

	if (!str.IsEmpty())
	{
		// новый алгоритм. тут возможно начало строки без тега
		// и вообще можно вывести строку без тега
		int beginpos = 0; // начальная позиция текста
		const int endpos = str.GetLength(); // конечная позиция текста

		while (true)
		{
			// ищем тег
			int tagpos = str.Find(_T(COLORED_TAG), beginpos);

			if (tagpos == -1) // если не нашли
			{
				tagpos = endpos;
				bEnd = true;
			}

			if (beginpos < tagpos) // если перед тегом есть какой-то текст
			{
				CString substr = str.Mid(beginpos, tagpos - beginpos);
				CSize size = pDC->GetTextExtent(substr); // текущая позиция вывода текста
				pDC->DrawText(substr, &rect, DT_LEFT | DT_VCENTER);
				rect.left += size.cx;

				if (rect.left >= rect.right)
				{
					bEnd = true;
				}
			}

			if (bEnd) // если тегов больше нет
			{
				break; // не надо их искать, выходим
			}

			const int clpos = tagpos + COLORED_TAG_LENGTH;

			if (clpos < endpos)
			{
				int colornum = (str.GetAt(clpos) - _T('0')) & 7; // номер цвета

				if (colornum > HLCOLOR_NUM_COLS)
				{
					colornum = HLCOLOR_DEFAULT;
				}

				pDC->SetTextColor(g_crDebugColorHighLighting[colornum]); // устанавливаем цвет
				beginpos = clpos + 1; // пойдём искать новый тег
			}
			else
			{
				break; // строка оборвана
			}
		}
	}
}


void CDisasmCtrl::OnSetFocus(CWnd *pOldWnd)
{
	CListCtrl::OnSetFocus(pOldWnd);

	if (GetSelectionMark() < 0)
	{
		SetSelectionMark(0);
	}

	Invalidate(FALSE);
}

#endif