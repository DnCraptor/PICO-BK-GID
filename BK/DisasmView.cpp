#include "pch.h"
#ifdef UI
#include "DisasmView.h"
#include "Config.h"
#include "Debugger.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CDisasmView
IMPLEMENT_DYNAMIC(CDisasmView, CDocPaneDlgViewBase)

CDisasmView::CDisasmView()
	: m_strDisasmAddr(_T("000000"))
{
}

CDisasmView::~CDisasmView()
    = default;

void CDisasmView::DoDataExchange(CDataExchange *pDX)
{
	CDocPaneDlgViewBase::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_DA_DISASM, m_ListDisasm);
	DDX_Control(pDX, IDC_EDIT_DA_DBGADDR, m_EditAddr);
	DDX_Text(pDX, IDC_EDIT_DA_DBGADDR, m_strDisasmAddr);
	DDX_Check(pDX, IDC_CHECK_DA_MMG, g_Config.m_bMMG);
	DDX_Check(pDX, IDC_CHECK_DA_EIS, g_Config.m_bEIS);
	DDX_Check(pDX, IDC_CHECK_DA_FIS, g_Config.m_bFIS);
	DDX_Check(pDX, IDC_CHECK_DA_FPU, g_Config.m_bFPU);
}

BEGIN_MESSAGE_MAP(CDisasmView, CDocPaneDlgViewBase)
	ON_MESSAGE(WM_INITDIALOG, &CDisasmView::HandleInitDialog)
	ON_WM_SETFOCUS()
	ON_MESSAGE(WM_DBG_TOP_ADDRESS_UPDATE, &CDisasmView::OnDisasmTopAddressUpdate)
	ON_MESSAGE(WM_DBG_TOP_ADDRESS_SET, &CDisasmView::OnDisasmTopAddressSet)
	ON_MESSAGE(WM_DBG_CURRENT_ADDRESS_CHANGE, &CDisasmView::OnDisasmCurrentAddressChange)
	ON_BN_CLICKED(IDC_CHECK_DA_MMG, &CDisasmView::ChangeInstructionSet)
	ON_BN_CLICKED(IDC_CHECK_DA_EIS, &CDisasmView::ChangeInstructionSet)
	ON_BN_CLICKED(IDC_CHECK_DA_FIS, &CDisasmView::ChangeInstructionSet)
	ON_BN_CLICKED(IDC_CHECK_DA_FPU, &CDisasmView::ChangeInstructionSet)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// обработчики сообщений CDisasmView

void CDisasmView::OnSetFocus(CWnd *pOldWnd)
{
	m_ListDisasm.SetFocus();
}



LRESULT CDisasmView::HandleInitDialog(WPARAM wp, LPARAM lp)
{
	CDocPaneDlgViewBase::HandleInitDialog(wp, lp);
	m_EditAddr.SetLimitText(6);
	m_ToolTip.AddTool(GetDlgItem(IDC_EDIT_DA_DBGADDR), IDS_TOOLTIP_EDIT_MD_DUMPADDR);
	m_ToolTip.AddTool(GetDlgItem(IDC_CHECK_DA_MMG), IDS_TOOLTIP_CHECK_DA_MMG);
	m_ToolTip.AddTool(GetDlgItem(IDC_CHECK_DA_EIS), IDS_TOOLTIP_CHECK_DA_EIS);
	m_ToolTip.AddTool(GetDlgItem(IDC_CHECK_DA_FIS), IDS_TOOLTIP_CHECK_DA_FIS);
	m_ToolTip.AddTool(GetDlgItem(IDC_CHECK_DA_FPU), IDS_TOOLTIP_CHECK_DA_FPU);
	m_ListDisasm.SetFont(&m_hFont, FALSE);
	m_ListDisasm.Init();
	return TRUE; // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDisasmView::SetAddr(uint16_t addr)
{
	(void)OnDisasmCurrentAddressChange(static_cast<WPARAM>(addr), 0);
}

void CDisasmView::AttachDebugger(CDebugger *pDebugger)
{
	m_ListDisasm.AttachDebugger(pDebugger);
	pDebugger->AttachWnd(this);
	SetDisasmAddr(g_Config.m_nDisasmAddr);
	m_ListDisasm.SetSelectionMark(0);
	m_ListDisasm.Invalidate(FALSE); // перерисуем дизассемблер
}

uint16_t CDisasmView::GetCursorAddr()
{
	return m_ListDisasm.GetCursorAddr();
}

uint16_t CDisasmView::GetBottomAddr()
{
	return m_ListDisasm.GetBottomAddr();
}

void CDisasmView::SetDisasmAddr(uint16_t addr)
{
	m_strDisasmAddr = Global::WordToOctString(addr);
	UpdateData(FALSE);
}

// это сообщение приходит из контрола - поля ввода адреса
// когда там делаются изменения, чтобы перерисовать список
LRESULT CDisasmView::OnDisasmTopAddressUpdate(WPARAM, LPARAM)
{
	UpdateData(TRUE);
	uint16_t nAddr = Global::OctStringToWord(m_strDisasmAddr);
	g_Config.m_nDisasmAddr = nAddr;
	// обновим значение, чтобы оно было всегда 6 значным
	SetDisasmAddr(nAddr);  // обновим адрес в поле адреса.
	m_ListDisasm.Invalidate(FALSE); // перерисуем дизассемблер
	return S_OK;
}

// это сообщение передаётся из отладчика когда происходит отладочный останов
// или ручное изменение значения PC (обычно в отладочном останове)
LRESULT CDisasmView::OnDisasmCurrentAddressChange(WPARAM wp, LPARAM)
{
	SetDisasmAddr(static_cast<uint16_t>(wp));
	m_ListDisasm.Invalidate(FALSE); // перерисуем дизассемблер
	return S_OK;
}

// это сообщение передаётся из списка m_ListDisasm сюда, когда там в результате
// действий меняется адрес, и надо его обновить
LRESULT CDisasmView::OnDisasmTopAddressSet(WPARAM wp, LPARAM)
{
	SetDisasmAddr(static_cast<uint16_t>(wp));
	return S_OK;
}

void CDisasmView::ChangeInstructionSet()
{
	UpdateData(TRUE);
	m_ListDisasm.ChangeInstructionSet();
	m_ListDisasm.Invalidate(FALSE);
	m_ListDisasm.SetFocus();
}
#endif