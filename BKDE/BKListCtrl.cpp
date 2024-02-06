#include "pch.h"
#include "resource.h"
#include "BKDEDefines.h"
#include "BKListCtrl.h"

IMPLEMENT_DYNAMIC(CBKListCtrl, CListCtrl)

CBKListCtrl::CBKListCtrl()
	: m_bSpecificColumn(false)
	, m_nWorkMode(MODE_CTRL::START)
	, m_bSelectAll(false)
	, m_nLastMouseClicked(0)
{
}


CBKListCtrl::~CBKListCtrl()
{
	m_font.DeleteObject();
	m_fontBold.DeleteObject();
}

BEGIN_MESSAGE_MAP(CBKListCtrl, CListCtrl)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CBKListCtrl::OnCustomDraw)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CBKListCtrl::OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CBKListCtrl::OnLvnItemchanged)
	ON_NOTIFY_REFLECT(NM_CLICK, &CBKListCtrl::OnNMClick)
	ON_NOTIFY_REFLECT(NM_RCLICK, &CBKListCtrl::OnNMRClick)
	ON_WM_KEYDOWN()

	ON_WM_DROPFILES()
	ON_WM_MBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

void CBKListCtrl::PreSubclassWindow()
{
	LOGFONT lf;
	GetFont()->GetLogFont(&lf);
	lf.lfHeight -= 1; // увеличим шрифт на 1 пункт
	lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	lf.lfQuality = CLEARTYPE_QUALITY;
#ifdef TARGET_WINXP
	_tcscpy_s(lf.lfFaceName, LF_FACESIZE, _T("Lucida Console\0"));
#else
	_tcscpy_s(lf.lfFaceName, LF_FACESIZE, _T("Consolas\0"));
#endif
	m_font.CreateFontIndirect(&lf);
	lf.lfWeight = FW_BLACK;
	m_fontBold.CreateFontIndirect(&lf);
	CListCtrl::PreSubclassWindow();
	SetFont(&m_font, FALSE);
}

int CALLBACK CBKListCtrl::MyCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	auto pList = reinterpret_cast<CBKListCtrl *>(lParamSort);
	LV_FINDINFO FindInfo;
	ZeroMemory(&FindInfo, sizeof(LVFINDINFO));
	FindInfo.flags = LVFI_PARAM;
	FindInfo.lParam = lParam1;
	int idx1 = pList->FindItem(&FindInfo);
	FindInfo.lParam = lParam2;
	int idx2 = pList->FindItem(&FindInfo);
	auto fr1 = reinterpret_cast<BKDirDataItem *>(pList->GetItemData(idx1));
	auto fr2 = reinterpret_cast<BKDirDataItem *>(pList->GetItemData(idx2));
	// сперва сортируем по типу записи, по возрастанию
	int res = static_cast<int>(fr1->nRecType) - static_cast<int>(fr2->nRecType);

	if (res == 0)
	{
		// если равны - сортируем по возрастанию по именам
		res = fr1->strName.compare(fr2->strName);
	}

	return res;
	// выход:
	// <0 если первый итем должен идти первым
	// >0 если второй итем должен идти первым
	// =0 если они равны
}

int CALLBACK CBKListCtrl::MyCompareProc2(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	auto pList = reinterpret_cast<CBKListCtrl *>(lParamSort);
	auto idx1 = static_cast<int>(lParam1 & 0xffff);
	auto idx2 = static_cast<int>(lParam2 & 0xffff);
	auto type1 = static_cast<int>((lParam1 >> 16) & 0xffff);
	auto type2 = static_cast<int>((lParam2 >> 16) & 0xffff);
	// размер или директория
	// 0 - UP, 1 - DIR, 2 - file
	// сперва сортируем по типу записи, по возрастанию
	int res = type1 - type2;

	if (res == 0)
	{
		// если равны - сортируем по возрастанию по именам
		CString str1 = pList->GetItemText(idx1, LC_FNAME_ST);
		CString str2 = pList->GetItemText(idx2, LC_FNAME_ST);
		// имена файлов
		int res = str1.Compare(str2);
	}

	return res;
	// выход:
	// <0 если первый итем должен идти первым
	// >0 если второй итем должен идти первым
	// =0 если они равны
}

void CBKListCtrl::Init(MODE_CTRL nMode)
{
	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	// сперва надо удалить все строки
	DeleteAllItems();
	// затем надо удалить все колонки
	const int nColumnCount = GetHeaderCtrl()->GetItemCount();

	for (int i = 0; i < nColumnCount; ++i)
	{
		DeleteColumn(0);
	}

	m_nWorkMode = nMode;
	const int nPixelW = GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSX);

	switch (nMode)
	{
		case MODE_CTRL::START:
			InsertColumn(LC_FNAME_ST,   CString(MAKEINTRESOURCE(IDS_LC_FNAME_HDR)), LVCFMT_LEFT, ::MulDiv(LC_FNAME_ST_COL_WIDTH, nPixelW, DEFAULT_DPIX));
			InsertColumn(LC_SIZE_ST,    CString(MAKEINTRESOURCE(IDS_LC_SIZE_HDR)), LVCFMT_LEFT, ::MulDiv(LC_SIZE_ST_COL_WIDTH, nPixelW, DEFAULT_DPIX));
			InsertColumn(LC_OSTYPE_ST,  CString(MAKEINTRESOURCE(IDS_LC_OSTYPE_HDR)), LVCFMT_LEFT, ::MulDiv(LC_OSTYPE_ST_COL_WIDTH, nPixelW, DEFAULT_DPIX));
			InsertColumn(LC_SYSTYPE_ST, CString(MAKEINTRESOURCE(IDS_LC_BOOTABLE_HDR)), LVCFMT_LEFT, ::MulDiv(LC_SYSTYPE_ST_COL_WIDTH, nPixelW, DEFAULT_DPIX));
			break;

		default:
		case MODE_CTRL::MAIN:
			InsertColumn(LC_FNAME_POS,    CString(MAKEINTRESOURCE(IDS_LC_FNAME_HDR)), LVCFMT_LEFT, ::MulDiv(LC_FNAME_COL_WIDTH, nPixelW, DEFAULT_DPIX));
			InsertColumn(LC_TYPE_POS,     CString(MAKEINTRESOURCE(IDS_LC_TYPE_HDR)), LVCFMT_LEFT, ::MulDiv(LC_TYPE_COL_WIDTH, nPixelW, DEFAULT_DPIX));
			InsertColumn(LC_BLK_SIZE_POS, CString(MAKEINTRESOURCE(IDS_LC_BLKSZ_HDR)), LVCFMT_RIGHT, ::MulDiv(LC_BLKSZ_COL_WIDTH, nPixelW, DEFAULT_DPIX));
			InsertColumn(LC_ADDRESS_POS,  CString(MAKEINTRESOURCE(IDS_LC_ADDRESS_HDR)), LVCFMT_RIGHT, ::MulDiv(LC_ADDRESS_COL_WIDTH, nPixelW, DEFAULT_DPIX));
			InsertColumn(LC_SIZE_POS,     CString(MAKEINTRESOURCE(IDS_LC_SIZE_HDR)), LVCFMT_RIGHT, ::MulDiv(LC_SIZE_COL_WIDTH, nPixelW, DEFAULT_DPIX));
			InsertColumn(LC_ATTR_POS,     CString(MAKEINTRESOURCE(IDS_LC_ATTR_HDR)), LVCFMT_LEFT, ::MulDiv(LC_ATTR_COL_WIDTH, nPixelW, DEFAULT_DPIX));
			break;
	}

	UpdateData(FALSE);
}

void CBKListCtrl::SetSpecificColumn(UINT nID)
{
	if (nID)
	{
		if (m_bSpecificColumn) // если уже есть, то надо сменить заголовок
		{
			DeleteColumn(LC_SPECIFIC_POS); // проще удалить и заново создать, чем через структуру менять заголовок
		}

		const int nPixelW = GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSX);
		InsertColumn(LC_SPECIFIC_POS, CString(MAKEINTRESOURCE(nID)), LVCFMT_LEFT, ::MulDiv(LC_SPECIFIC_COL_WIDTH, nPixelW, DEFAULT_DPIX));
		UpdateData(FALSE);
		m_bSpecificColumn = true;
	}
	else
	{
		if (m_bSpecificColumn) // удаляем только если есть.
		{
			DeleteColumn(LC_SPECIFIC_POS);
			UpdateData(FALSE);
			m_bSpecificColumn = false;
		}
	}
}


void CBKListCtrl::OnCustomDraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	auto pLVCustomDraw = reinterpret_cast<NMLVCUSTOMDRAW *>(pNMHDR);
	NMCUSTOMDRAW pLV = pLVCustomDraw->nmcd;
	// pLV.dwItemSpec - номер item'а
	*pResult = CDRF_DODEFAULT;

	switch (m_nWorkMode)
	{
		case MODE_CTRL::START:
			switch (pLVCustomDraw->nmcd.dwDrawStage)
			{
				case CDDS_PREPAINT:
					*pResult |= CDRF_NOTIFYITEMDRAW;
					break;

				case CDDS_ITEMPREPAINT:
				{
					const auto nType = static_cast<BKDirDataItem::RECORD_TYPE>((GetItemData(pLV.dwItemSpec) >> 16) & 0xffff);

					if (nType == BKDirDataItem::RECORD_TYPE::UP || nType == BKDirDataItem::RECORD_TYPE::DIR)
					{
						SelectObject(pLVCustomDraw->nmcd.hdc, m_fontBold);
						pLVCustomDraw->clrText = RGB(0, 0, 0x80);
					}
				}
				break;
			}

			break;

		case MODE_CTRL::MAIN:
			switch (pLVCustomDraw->nmcd.dwDrawStage)
			{
				case CDDS_PREPAINT:
					*pResult |= CDRF_NOTIFYITEMDRAW;
					break;

				case CDDS_ITEMPREPAINT:
					const auto fr = reinterpret_cast<BKDirDataItem *>(GetItemData(pLV.dwItemSpec));

					if (fr->nAttr & (FR_ATTR::DIR | FR_ATTR::LOGDISK))
					{
						SelectObject(pLVCustomDraw->nmcd.hdc, m_fontBold);
					}

					if (fr->nAttr & FR_ATTR::PROTECTED)
					{
						pLVCustomDraw->clrText = RGB(0, 0, 160);
					}

					if (fr->nAttr & FR_ATTR::HIDDEN)
					{
						pLVCustomDraw->clrText = RGB(0x66, 0x66, 0x66);
					}

					if (fr->nAttr & FR_ATTR::LOGDISK)
					{
						// логический диск выделим своим цветом, даже если он protected or hidden
						pLVCustomDraw->clrText = RGB(0, 160, 0);
					}

					if (fr->nAttr & FR_ATTR::DELETED)
					{
						pLVCustomDraw->clrText = RGB(0xcc, 0xcc, 0xcc);
					}

					if (fr->nAttr & FR_ATTR::BAD)
					{
						pLVCustomDraw->clrText = RGB(0xcc, 0, 0);
					}

					break;
			}

			break;

		default:
			break;
	}
}

void CBKListCtrl::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	auto pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int item = pNMListView->iItem;
	*pResult = 0;

	switch (m_nWorkMode)
	{
		case MODE_CTRL::START:
			// ничего делать не надо
			break;

		case MODE_CTRL::MAIN:
		{
			const auto fr = reinterpret_cast<BKDirDataItem *>(GetItemData(item));

			if ((pNMListView->uChanged & LVIF_STATE) && (pNMListView->uNewState & LVNI_SELECTED))
			{
				// директории смотреть в текстовом или спрайтовом виде нельзя
				const BOOL bViewStatus = !(fr->nAttr & (FR_ATTR::DIR | FR_ATTR::LOGDISK));
				GetParentOwner()->SendMessage(WM_SEND_ENABLE_BUTTON, WPARAM(IDC_BUTTON_VIEW), LPARAM(bViewStatus));
				GetParentOwner()->SendMessage(WM_SEND_ENABLE_BUTTON, WPARAM(IDC_BUTTON_VIEW_AS_SPR), LPARAM(bViewStatus));
				// запрещаем удалять уже удалённое
				// но вообще механизм защиты в функциях реализован
				BOOL bDelStatus = !(fr->nAttr & FR_ATTR::DELETED || fr->nRecType == BKDirDataItem::RECORD_TYPE::UP);
				GetParentOwner()->SendMessage(WM_SEND_ENABLE_BUTTON, WPARAM(IDC_BUTTON_DELETE), LPARAM(bDelStatus));
				// запрещаем переименовывать удалённое
				GetParentOwner()->SendMessage(WM_SEND_ENABLE_BUTTON, WPARAM(IDC_BUTTON_RENAME), LPARAM(bDelStatus));

				// на основе статуса удаления, составим статус изменения адреса загрузки файла
				// в дополнение ко всему, у директорий и т.п. тоже адрес менять нельзя
				if (fr->nAttr & (FR_ATTR::DIR | FR_ATTR::LOGDISK | FR_ATTR::LINK))
				{
					bDelStatus = FALSE;
				}

				GetParentOwner()->SendMessage(WM_SEND_ENABLE_BUTTON, WPARAM(ID_CONTEXT_CHGADDR), LPARAM(bDelStatus));
			}

			break;
		}

		default:
			break;
	}
}

void CBKListCtrl::OnDropFiles(HDROP hDropInfo)
{
	this->GetParentOwner()->SendMessage(WM_MAKE_DROP, 0, reinterpret_cast<LPARAM>(hDropInfo));
	// CListCtrl::OnDropFiles(hDropInfo);
}


void CBKListCtrl::ClearSelectAll()
{
	const int items = GetItemCount();

	for (int i = 0; i < items; ++i)
	{
		auto fr = reinterpret_cast<BKDirDataItem *>(GetItemData(i));

		if (fr)
		{
			fr->bSelected = false;
		}
	}

	SetItemState(-1, ~LVIS_SELECTED, LVIS_SELECTED);
	m_bSelectAll = false;
}

void CBKListCtrl::SetSelectAll()
{
	const int items = GetItemCount();

	for (int i = 0; i < items; ++i)
	{
		auto fr = reinterpret_cast<BKDirDataItem *>(GetItemData(i));

		if (fr)
		{
			fr->bSelected = true;
		}
	}

	SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED);
	m_bSelectAll = true;
}

void CBKListCtrl::SetSelectCurrent(int nItem)
{
	auto fr_current = reinterpret_cast<BKDirDataItem *>(GetItemData(nItem));

	if (fr_current)
	{
		if (fr_current->bSelected)
		{
			SetItemState(nItem, ~LVIS_SELECTED, LVIS_SELECTED);
			fr_current->bSelected = false;
		}
		else
		{
			SetItemState(nItem, LVIS_SELECTED, LVIS_SELECTED);
			fr_current->bSelected = true;
		}
	}
}

void CBKListCtrl::SetSelectRange(int nFrom, int nTo) const
{
	if (nFrom > nTo)
	{
		std::swap(nTo, nFrom);
	}

	for (int i = nFrom; i <= nTo; ++i)
	{
		auto fr = reinterpret_cast<BKDirDataItem *>(GetItemData(i));

		if (fr)
		{
			fr->bSelected = true;
		}
	}
}

void CBKListCtrl::MouseClick(int nItem)
{
	if (GetKeyState(VK_SHIFT) < 0)
	{
		SetSelectRange(m_nLastMouseClicked, nItem);
	}
	else if (GetKeyState(VK_CONTROL) < 0)
	{
		SetSelectCurrent(nItem);
	}
	else
	{
		ClearSelectAll();
	}

	SetItemState(nItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	m_nLastMouseClicked = nItem;
}

void CBKListCtrl::EnterTo(int nItem) const
{
	const auto fr = reinterpret_cast<BKDirDataItem *>(GetItemData(nItem));
	GetParentOwner()->SendMessage(WM_SEND_PROCESSING, WPARAM(nItem), reinterpret_cast<LPARAM>(fr));
}

void CBKListCtrl::ImgEnterTo(int nItem) const
{
	// надо получить имя файла, на котором кликнули
	const CString str = GetItemText(nItem, LC_FNAME_ST);
	GetParentOwner()->SendMessage(WM_SEND_IMGNAMEPRC, WPARAM(nItem), reinterpret_cast<LPARAM>(&str));
}

void CBKListCtrl::StepTo(int nFlags, int nItem, bool bMM)
{
	if (bMM && m_bSelectAll)
	{
		ClearSelectAll();
	}

	const int nNextItem = GetNextItem(nItem, nFlags);

	if (nNextItem >= 0)
	{
		ChangeSelection(nItem, nNextItem, bMM);

		if (!EnsureVisible(nNextItem, FALSE))
		{
			CRect r;
			GetItemRect(nNextItem, &r, LVIR_BOUNDS);

			if (nFlags == LVNI_ABOVE)
			{
				Scroll(CSize(0, -r.Height()));
			}
			else if (nFlags == LVNI_BELOW)
			{
				Scroll(CSize(0, r.Height()));
			}
		}
	}
}

void CBKListCtrl::StepLeft(int nItem, bool bMM)
{
	const int topItem = GetTopIndex();
	const int nItemCnt = GetItemCount();
	int highCnt = GetCountPerPage();

	if (highCnt > nItemCnt)
	{
		highCnt = nItemCnt;
	}

	int nextItem;

	if (nItem > topItem)
	{
		nextItem = topItem;
		// и скроллить не надо
	}
	else
	{
		nextItem = topItem - highCnt;

		if (nextItem < 0)
		{
			nextItem = 0;
		}

		// если скроллить меньше, чем на полный экран, то скроллим только на то, что можем
		const int n = topItem - highCnt;

		if (n < 0)
		{
			highCnt += n;
		}

		if (highCnt > 0) // если можно скроллить, то скроллим
		{
			CRect r;
			GetItemRect(topItem, &r, LVIR_BOUNDS);
			Scroll(CSize(0, -highCnt * r.Height()));
		}
	}

	ChangeSelection(nItem, nextItem, bMM);
}

void CBKListCtrl::StepRight(int nItem, bool bMM)
{
	const int topItem = GetTopIndex();
	const int nItemCnt = GetItemCount();
	int highCnt = GetCountPerPage();

	if (highCnt > nItemCnt)
	{
		highCnt = nItemCnt;
	}

	int nextItem = topItem + highCnt;

	if (nextItem >= nItemCnt)
	{
		nextItem = nItemCnt - 1;
		// и скроллить не надо
	}
	else
	{
		// если скроллить меньше, чем на полный экран, то скроллим только на то, что можем
		const int n = nItemCnt - (topItem + highCnt);

		if (n < highCnt)
		{
			if (n > 0)
			{
				nextItem -= highCnt - n;
			}

			highCnt = n;
		}

		if (highCnt > 0) // если можно скроллить, то скроллим
		{
			CRect r;
			GetItemRect(topItem, &r, LVIR_BOUNDS);
			Scroll(CSize(0, highCnt * r.Height()));
		}
	}

	ChangeSelection(nItem, nextItem, bMM);
}

void CBKListCtrl::ChangeSelection(const int nItem, const int nNextItem, const bool bMM)
{
	BKDirDataItem *fr = nullptr;

	if (bMM)
	{
		fr = reinterpret_cast<BKDirDataItem *>(GetItemData(nItem));
	}

	if (fr && fr->bSelected)
	{
		SetItemState(nItem, ~(LVIS_FOCUSED), LVIS_FOCUSED);
	}
	else
	{
		SetItemState(nItem, ~(LVIS_FOCUSED | LVIS_SELECTED), LVIS_FOCUSED | LVIS_SELECTED);
	}

	SetItemState(nNextItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
}


void CBKListCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	int nItem = -1;
	// находим итем, на котором стоит курсор
	nItem = GetNextItem(nItem, LVNI_FOCUSED);

	if (nItem >= 0) // что-нибудь делаем, если есть хоть какой-нибудь итем
	{
		switch (m_nWorkMode)
		{
			case MODE_CTRL::START:
			{
				// тут надо тоже сделать навигацию кнопками
				switch (nChar)
				{
					case VK_RETURN:
						return ImgEnterTo(nItem);

					case VK_LEFT:
						return StepLeft(nItem, false);

					case VK_RIGHT:
						return StepRight(nItem, false);
				}

				break; // пойдём выполним действие по умолчанию
			}

			case MODE_CTRL::MAIN:
			{
				switch (nChar)
				{
					case VK_RETURN:
						return EnterTo(nItem);

					case 0x41:      // CTRL+A (Select all rows)
					{
						if (GetKeyState(VK_CONTROL) < 0)
						{
							if (!(GetStyle() & LVS_SINGLESEL))
							{
								SetSelectAll();
							}
						}

						return;
					}

					case VK_PRIOR: // выход из директории
					{
						if ((nFlags & KF_EXTENDED) && (GetKeyState(VK_CONTROL) < 0))
						{
							return EnterTo(0);
						}

						break; // пойдём выполним действие по умолчанию
					}

					case VK_NEXT: // вход в директорию, если курсор стоит на директории
					{
						auto fr_current = reinterpret_cast<BKDirDataItem *>(GetItemData(nItem));

						if (
						    ((nFlags & KF_EXTENDED) && (GetKeyState(VK_CONTROL) < 0)) &&
						    (fr_current->nAttr & FR_ATTR::DIR)
						)
						{
							return EnterTo(nItem);
						}

						break; // пойдём выполним действие по умолчанию
					}

					case VK_INSERT:
					{
						// а не знаю, как по Ins выделить элемент, тут ведь движение стрелками снимает выделение. надо по-другому
						// как-то это дело обрабатывать, чтоб как в винраре было.
						SetSelectCurrent(nItem);
						int nNextItem = GetNextItem(nItem, LVNI_BELOW);

						if (nNextItem >= 0)
						{
							SetItemState(nNextItem, LVIS_FOCUSED, LVIS_FOCUSED);

							if (!EnsureVisible(nNextItem, FALSE))
							{
								CRect r;
								GetItemRect(nNextItem, &r, LVIR_BOUNDS);
								Scroll(CSize(0, r.Height()));
							}
						}

						return;
					}

					case VK_UP:
						return StepTo(LVNI_ABOVE, nItem, true);

					case VK_DOWN:
						return StepTo(LVNI_BELOW, nItem, true);

					case VK_LEFT:
						return StepLeft(nItem, true);

					case VK_RIGHT:
						return StepRight(nItem, true);
				} // switch (nChar)
			}
			break;
		} // switch (m_nWorkMode)
	} // if

	CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

// средняя кнопка мыши будет работать как кнопка ввод
void CBKListCtrl::OnMButtonDown(UINT nFlags, CPoint point)
{
	int nItem = -1;
	// находим итем, на котором стоит курсор
	nItem = GetNextItem(nItem, LVNI_FOCUSED);

	if (nItem >= 0)
	{
		switch (m_nWorkMode)
		{
			case MODE_CTRL::START:
			{
				return ImgEnterTo(nItem);
			}

			case MODE_CTRL::MAIN:
			{
				return EnterTo(nItem);
			}
		}
	}

	return CListCtrl::OnMButtonDown(nFlags, point);
}

// колесо мыши - как стрелки вверх/вниз
BOOL CBKListCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	int nItem = -1;
	// находим итем, на котором стоит курсор
	nItem = GetNextItem(nItem, LVNI_FOCUSED);

	if (nItem >= 0)
	{
		switch (m_nWorkMode)
		{
			case MODE_CTRL::START:
			{
				if (zDelta > 0)
				{
					StepTo(LVNI_ABOVE, nItem, false);
					return TRUE;
				}

				if (zDelta < 0)
				{
					StepTo(LVNI_BELOW, nItem, false);
					return TRUE;
				}

				break;
			}

			case MODE_CTRL::MAIN:
			{
				if (zDelta > 0)
				{
					StepTo(LVNI_ABOVE, nItem, true);
					return TRUE;
				}

				if (zDelta < 0)
				{
					StepTo(LVNI_BELOW, nItem, true);
					return TRUE;
				}

				break;
			}
		}
	}

	return CListCtrl::OnMouseWheel(nFlags, zDelta, pt);
}

// двойной клик левой кнопкой мыши - как клавиша ввод
void CBKListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	const auto pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = S_OK;

	if (pNMItemActivate->iItem >= 0)
	{
		switch (m_nWorkMode)
		{
			case MODE_CTRL::START:
			{
				ImgEnterTo(pNMItemActivate->iItem);
				break;
			}

			case MODE_CTRL::MAIN:
			{
				EnterTo(pNMItemActivate->iItem);
				break;
			}
		}
	}
}

// клик левой кнопкой мыши
void CBKListCtrl::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	const auto pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = S_OK;

	if (pNMItemActivate->iItem >= 0)
	{
		switch (m_nWorkMode)
		{
			case MODE_CTRL::START:
				// ничего делать не надо
				break;

			case MODE_CTRL::MAIN:
				MouseClick(pNMItemActivate->iItem);
				break;
		}
	}
}

// клик правой кнопкой мыши.
void CBKListCtrl::OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	const auto pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = S_OK;

	if (pNMItemActivate->iItem >= 0)
	{
		switch (m_nWorkMode)
		{
			case MODE_CTRL::START:
				// ничего делать не надо
				break;

			case MODE_CTRL::MAIN:
			{
				CPoint point;
				LVHITTESTINFO ht;
				ZeroMemory(&ht, sizeof(LVHITTESTINFO));
				GetCursorPos(&point);
				ScreenToClient(&point);
				ht.pt = point;
				SubItemHitTest(&ht);

				if (ht.iItem != -1)
				{
					ClientToScreen(&point);
					CMenu mnu;
					mnu.LoadMenu(IDR_MENU_MAIN); // идентификатор меню в ресурсах
					CMenu *pMenu = mnu.GetSubMenu(0);
					auto bStatus = static_cast<BOOL>(GetParentOwner()->SendMessage(WM_GET_ENABLE_BUTTON, WPARAM(IDC_BUTTON_VIEW)));
					UINT nStatus = MF_BYCOMMAND | (bStatus ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
					pMenu->EnableMenuItem(ID_CONTEXT_VIEWASTEXT, nStatus);
					bStatus = static_cast<BOOL>(GetParentOwner()->SendMessage(WM_GET_ENABLE_BUTTON, WPARAM(IDC_BUTTON_VIEW_AS_SPR)));
					nStatus = MF_BYCOMMAND | (bStatus ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
					pMenu->EnableMenuItem(ID_CONTEXT_VIEWASSPRITE, nStatus);
					bStatus = static_cast<BOOL>(GetParentOwner()->SendMessage(WM_GET_ENABLE_BUTTON, WPARAM(IDC_BUTTON_RENAME)));
					nStatus = MF_BYCOMMAND | (bStatus ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
					pMenu->EnableMenuItem(ID_CONTEXT_RENAME, nStatus);
					bStatus = static_cast<BOOL>(GetParentOwner()->SendMessage(WM_GET_ENABLE_BUTTON, WPARAM(ID_CONTEXT_CHGADDR)));
					nStatus = MF_BYCOMMAND | (bStatus ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
					pMenu->EnableMenuItem(ID_CONTEXT_CHGADDR, nStatus);
					bStatus = static_cast<BOOL>(GetParentOwner()->SendMessage(WM_GET_ENABLE_BUTTON, WPARAM(IDC_BUTTON_EXTRACT)));
					nStatus = MF_BYCOMMAND | (bStatus ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
					pMenu->EnableMenuItem(ID_CONTEXT_EXTRACT, nStatus);
					bStatus = static_cast<BOOL>(GetParentOwner()->SendMessage(WM_GET_ENABLE_BUTTON, WPARAM(IDC_BUTTON_DELETE)));
					nStatus = MF_BYCOMMAND | (bStatus ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
					pMenu->EnableMenuItem(ID_CONTEXT_DELETE, nStatus);
					pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd());
					mnu.DestroyMenu();
				}
			}
			break;
		}
	}
}

