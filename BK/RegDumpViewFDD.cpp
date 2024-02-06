#ifdef UI
#include "pch.h"
#include "RegDumpViewFDD.h"
#include "Screen_Sizes.h"
#include "Debugger.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

constexpr auto COLUMN_WIDTH = 60;
constexpr auto COLUMN_WIDTH_PRT = 78;
constexpr auto COLUMN_WIDTH_REG = 38;

const UINT CRegDumpViewFDD::m_pLISTFDDIDs[LSTFDD_ROW::FDDROWNUM] =
{
	IDS_MEMORY_177130IN, IDS_MEMORY_177130OUT,
	IDS_MEMORY_177132IN, IDS_MEMORY_177132OUT
};
const CString CRegDumpViewFDD::m_plistHDDHDrs[LSTHDD_ROW::HDDROWNUM] =
{
	_T("Rgm"),
	_T("1F0"),
	_T("1F1"),
	_T("1F2"),
	_T("1F3"),
	_T("1F4"),
	_T("1F5"),
	_T("1F6"),
	_T("1F7"),
	_T("3F6"),
	_T("3F7"),
};

/////////////////////////////////////////////////////////////////////////////
// CRegDumpViewFDD
IMPLEMENT_DYNAMIC(CRegDumpViewFDD, CDocPaneDlgViewBase)

CRegDumpViewFDD::CRegDumpViewFDD()
	: m_pDebugger(nullptr)
{
}

CRegDumpViewFDD::~CRegDumpViewFDD()
    = default;

void CRegDumpViewFDD::DoDataExchange(CDataExchange *pDX)
{
	CDocPaneDlgViewBase::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_RDD_FDDREGS, m_listFDD);
	DDX_Control(pDX, IDC_LIST_RDD_HDDREGS, m_listHDD);
}

BEGIN_MESSAGE_MAP(CRegDumpViewFDD, CDocPaneDlgViewBase)
	ON_MESSAGE(WM_INITDIALOG, &CRegDumpViewFDD::HandleInitDialog)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_RDD_FDDREGS, &CRegDumpViewFDD::OnLvnItemchangedMdFddData)
END_MESSAGE_MAP()

// Вход: list - список, grp - элемент "Группа"
// height - количество строк в списке
// Вход-Выход:  ptLT - координата верхней левой точки списка, если 0 - то берутся текущие
//              maxGW - максимальная ширина группы
void CRegDumpViewFDD::ResizeList(CMultiEditListCtrl *list, CWnd *grp, int height, CPoint &ptLT, int &maxGW)
{
	CRect listRect, grpRect, itemRect;
	list->GetWindowRect(&listRect);     // текущие размеры списка
	ScreenToClient(&listRect);

	if (ptLT.x)
	{
		listRect.left = ptLT.x;
	}

	if (ptLT.y)
	{
		listRect.top = ptLT.y;
	}

	grp->GetWindowRect(&grpRect);   // текущие размеры группы
	ScreenToClient(&grpRect);
	list->GetItemRect(0, itemRect, LVIR_BOUNDS); // получим размеры элемента строки списка
	// и на их основе расчитаем новые размеры списка, чтобы был точно по размерам элементов
	int cx = itemRect.Width() + ::GetSystemMetrics(SM_CXDLGFRAME);
	int cy = itemRect.Height() * height + ::GetSystemMetrics(SM_CYDLGFRAME);
	list->SetWindowPos(nullptr, listRect.left, listRect.top, cx, cy, SWP_SHOWWINDOW); // изменяем размеры списка
	int nPixelW = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSX);
	int nPixelH = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSY);
	int padX = ::MulDiv(4, nPixelW, DEFAULT_DPIX); // отступ группы от списка слева и справа
	int padY = ::MulDiv(5, nPixelH, DEFAULT_DPIY); // отступ группы от списка сверху и снизу
	grpRect.left = listRect.left - padX;
	grpRect.top = listRect.top - ::GetSystemMetrics(SM_CYCAPTION) / 2 - padY;
	grpRect.right = listRect.left + cx + padX;
	grpRect.bottom = listRect.top + cy + padY;
	grp->SetWindowPos(nullptr, grpRect.left, grpRect.top, grpRect.Width(), grpRect.Height(), SWP_SHOWWINDOW); // изменяем размеры группы

	if (maxGW < grpRect.Width())
	{
		maxGW = grpRect.Width();
	}

	ptLT.x = listRect.left;
	ptLT.y = grpRect.bottom + ::GetSystemMetrics(SM_CYCAPTION) / 2 + padY;
}


void CRegDumpViewFDD::SetGrpWidth(CWnd *grp, int width)
{
	CRect grpRect;
	grp->GetWindowRect(&grpRect);   // текущие размеры группы
	ScreenToClient(&grpRect);
	grp->SetWindowPos(nullptr, grpRect.left, grpRect.top, width, grpRect.Height(), SWP_SHOWWINDOW); // изменяем размеры группы
}

/////////////////////////////////////////////////////////////////////////////
// обработчики сообщений CRegDumpViewFDD

LRESULT CRegDumpViewFDD::HandleInitDialog(WPARAM wp, LPARAM lp)
{
	CDocPaneDlgViewBase::HandleInitDialog(wp, lp);
	int nPixelW = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSX);
	CString strHeader;
	// Создадим список регистров FDD
	// 2 колонки, 1-я - имя регистра, 2-я - его значение
	m_listFDD.SetFont(&m_hFont, FALSE);
	m_listFDD.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_SNAPTOGRID);
	m_listFDD.InsertColumn(LSTFDD_COL::NAME, LPSTR_TEXTCALLBACK, LVCFMT_LEFT, ::MulDiv(COLUMN_WIDTH_PRT, nPixelW, DEFAULT_DPIX));
	m_listFDD.InsertColumn(LSTFDD_COL::VALUE, LPSTR_TEXTCALLBACK, LVCFMT_LEFT, ::MulDiv(COLUMN_WIDTH, nPixelW, DEFAULT_DPIX));

	for (int i = LSTFDD_ROW::REG177130IN; i <= LSTFDD_ROW::REG177132OUT; ++i)
	{
		m_listFDD.InsertItem(i, CString(MAKEINTRESOURCE(m_pLISTFDDIDs[i])));
		m_listFDD.SetItem(i, LSTFDD_COL::VALUE, LVIF_TEXT, nullptr, 0, 0, 0, 0);
	}

	m_listFDD.SetColumnBkColor(::GetSysColor(COLOR_BTNFACE), LSTFDD_COL::NAME);
	m_listFDD.EnableColumnEdit(false, LSTFDD_COL::NAME);
	m_listFDD.EnableColumnEdit(false, LSTFDD_COL::VALUE); // запретим редактировать. только для просмотра
	// Создадим список регистров HDD
	// 6 колонок, 1-я и 4-я - адрес порта/регистра, 2-я и 5-я - их значения по чтению, 3-я и 6-я - их значения по записи
	m_listHDD.SetFont(&m_hFont, FALSE);
	m_listHDD.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_SNAPTOGRID);
	m_listHDD.InsertColumn(LSTHDD_COL::NAME1, LPSTR_TEXTCALLBACK, LVCFMT_LEFT, ::MulDiv(COLUMN_WIDTH_REG, nPixelW, DEFAULT_DPIX));
	m_listHDD.InsertColumn(LSTHDD_COL::VALUE11, LPSTR_TEXTCALLBACK, LVCFMT_LEFT, ::MulDiv(COLUMN_WIDTH, nPixelW, DEFAULT_DPIX));
	m_listHDD.InsertColumn(LSTHDD_COL::VALUE12, LPSTR_TEXTCALLBACK, LVCFMT_LEFT, ::MulDiv(COLUMN_WIDTH, nPixelW, DEFAULT_DPIX));
	m_listHDD.InsertColumn(LSTHDD_COL::NAME2, LPSTR_TEXTCALLBACK, LVCFMT_LEFT, ::MulDiv(COLUMN_WIDTH_REG, nPixelW, DEFAULT_DPIX));
	m_listHDD.InsertColumn(LSTHDD_COL::VALUE21, LPSTR_TEXTCALLBACK, LVCFMT_LEFT, ::MulDiv(COLUMN_WIDTH, nPixelW, DEFAULT_DPIX));
	m_listHDD.InsertColumn(LSTHDD_COL::VALUE22, LPSTR_TEXTCALLBACK, LVCFMT_LEFT, ::MulDiv(COLUMN_WIDTH, nPixelW, DEFAULT_DPIX));
	strHeader = m_plistHDDHDrs[0];
	m_listHDD.InsertItem(0, strHeader);
	m_listHDD.SetItem(0, LSTHDD_COL::VALUE11, LVIF_TEXT, _T("Read"), 0, 0, 0, 0);
	m_listHDD.SetItem(0, LSTHDD_COL::VALUE12, LVIF_TEXT, _T("Write"), 0, 0, 0, 0);
	m_listHDD.SetItem(0, LSTHDD_COL::NAME2, LVIF_TEXT, strHeader, 0, 0, 0, 0);
	m_listHDD.SetItem(0, LSTHDD_COL::VALUE21, LVIF_TEXT, _T("Read"), 0, 0, 0, 0);
	m_listHDD.SetItem(0, LSTHDD_COL::VALUE22, LVIF_TEXT, _T("Write"), 0, 0, 0, 0);

	for (int i = LSTHDD_ROW::R1F0; i <= LSTHDD_ROW::R3F7; ++i)
	{
		strHeader = m_plistHDDHDrs[i];
		m_listHDD.InsertItem(i, strHeader);
		m_listHDD.SetItem(i, LSTHDD_COL::VALUE11, LVIF_TEXT, LPSTR_TEXTCALLBACK, 0, 0, 0, 0);
		m_listHDD.SetItem(i, LSTHDD_COL::VALUE12, LVIF_TEXT, LPSTR_TEXTCALLBACK, 0, 0, 0, 0);
		m_listHDD.SetItem(i, LSTHDD_COL::NAME2, LVIF_TEXT, strHeader, 0, 0, 0, 0);
		m_listHDD.SetItem(i, LSTHDD_COL::VALUE21, LVIF_TEXT, LPSTR_TEXTCALLBACK, 0, 0, 0, 0);
		m_listHDD.SetItem(i, LSTHDD_COL::VALUE22, LVIF_TEXT, LPSTR_TEXTCALLBACK, 0, 0, 0, 0);
	}

	m_listHDD.SetRowBkColor(::GetSysColor(COLOR_BTNFACE), LSTHDD_ROW::MODE);
	m_listHDD.SetColumnBkColor(::GetSysColor(COLOR_BTNFACE), LSTHDD_COL::NAME1);
	m_listHDD.EnableColumnEdit(false, LSTHDD_COL::NAME1);
	m_listHDD.EnableColumnEdit(false, LSTHDD_COL::VALUE11); // значения редактировать тоже запрещаем
	m_listHDD.EnableColumnEdit(false, LSTHDD_COL::VALUE12); // значения редактировать тоже запрещаем
	m_listHDD.SetColumnBkColor(::GetSysColor(COLOR_BTNFACE), LSTHDD_COL::NAME2);
	m_listHDD.EnableColumnEdit(false, LSTHDD_COL::NAME2);
	m_listHDD.EnableColumnEdit(false, LSTHDD_COL::VALUE21); // значения редактировать тоже запрещаем
	m_listHDD.EnableColumnEdit(false, LSTHDD_COL::VALUE22); // значения редактировать тоже запрещаем
	int nMaxGrpWidth = 0;
	CPoint ptLT{ 0, 0 };
	ResizeList(&m_listFDD, GetDlgItem(IDC_STATIC_RDD_FDDREGTITLE), LSTFDD_ROW::FDDROWNUM, ptLT, nMaxGrpWidth);
	ResizeList(&m_listHDD, GetDlgItem(IDC_STATIC_RDD_HDDREGTITLE), LSTHDD_ROW::HDDROWNUM, ptLT, nMaxGrpWidth);
	// теперь сделаем группы все одной ширины
	SetGrpWidth(GetDlgItem(IDC_STATIC_RDD_FDDREGTITLE), nMaxGrpWidth);
	SetGrpWidth(GetDlgItem(IDC_STATIC_RDD_HDDREGTITLE), nMaxGrpWidth);
	//и под конец изменим размеры самого окна
	CRect rect;
	GetWindowRect(&rect);
	ScreenToClient(&rect);
	rect.bottom = ptLT.y - ::GetSystemMetrics(SM_CYCAPTION) / 2;
	rect.right = ptLT.x + nMaxGrpWidth;
	m_sizeDefault = rect.Size();
	return TRUE; // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CRegDumpViewFDD::DisplayRegDump()
{
	if (IsWindowVisible())
	{
		// выводим на экран данные FDD
		DisplayFDDData();
		DisplayHDDData();
	}
}

void CRegDumpViewFDD::DisplayFDDData()
{
	if (m_pDebugger)
	{
		for (int i = LSTFDD_ROW::REG177130IN; i <= LSTFDD_ROW::REG177132OUT; ++i)
		{
			m_listFDD.SetItemWithModified(m_pDebugger->GetFDDData(i), i, LSTFDD_COL::VALUE);
		}
	}
}

void CRegDumpViewFDD::DisplayHDDData()
{
	if (m_pDebugger)
	{
		for (int i = LSTHDD_ROW::R1F0; i <= LSTHDD_ROW::R3F7; ++i)
		{
			m_listHDD.SetItemWithModified(m_pDebugger->GetDebugHDDRegs(0, i, true), i, LSTHDD_COL::VALUE11);
			m_listHDD.SetItemWithModified(m_pDebugger->GetDebugHDDRegs(0, i, false), i, LSTHDD_COL::VALUE12);
			m_listHDD.SetItemWithModified(m_pDebugger->GetDebugHDDRegs(1, i, true), i, LSTHDD_COL::VALUE21);
			m_listHDD.SetItemWithModified(m_pDebugger->GetDebugHDDRegs(1, i, false), i, LSTHDD_COL::VALUE22);
		}
	}
}

void CRegDumpViewFDD::OnLvnItemchangedMdFddData(NMHDR *pNMHDR, LRESULT *pResult)
{
	auto pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: добавьте свой код обработчика уведомлений
	*pResult = S_OK;
}

#endif