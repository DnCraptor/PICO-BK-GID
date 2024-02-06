// PrintDlg.cpp : implementation file
//

#include "pch.h"
#include "resource.h"
#include "Config.h"
#include "PrintDlg.h"
#include "Debugger.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPrintDlg

IMPLEMENT_DYNAMIC(CPrintDlg, CPrintDialog)

CPrintDlg::CPrintDlg(BOOL bPrintSetupOnly, DWORD dwFlags, CWnd *pParentWnd)
	: CPrintDialog(bPrintSetupOnly, dwFlags, pParentWnd)
	, m_pDebugger(nullptr)
{
	m_pd.lpPrintTemplateName = MAKEINTRESOURCE(IDD_PRINT);
	// m_pd.lpSetupTemplateName = MAKEINTRESOURCE(IDD_PRINT);
	m_pd.Flags |= PD_ENABLEPRINTTEMPLATE;
	// m_pd.Flags |= PD_ENABLESETUPTEMPLATE;
	m_pd.hInstance = AfxGetInstanceHandle();
	// AfxGetApp()->GetPrinterDeviceDefaults(&m_pd); - вот из-за этой строки диалог вызывал повреждение памяти. Ни в коем случае не использовать такое больше.
	m_pd.nCopies = 1;
}
CPrintDlg::~CPrintDlg()
    = default;

BEGIN_MESSAGE_MAP(CPrintDlg, CPrintDialog)

	ON_WM_SHOWWINDOW()
	ON_COMMAND(IDC_PRN_PRINT_SCREEN, &CPrintDlg::OnPrintScreen)
	ON_COMMAND(IDC_PRN_PRINT_CODE, &CPrintDlg::OnPrintCode)
	ON_COMMAND(IDC_PRN_INVERSE, &CPrintDlg::OnInverse)
	ON_COMMAND(IDOK, &CPrintDlg::OnOk)
	ON_EN_CHANGE(IDC_PRN_START_ADDR, &CPrintDlg::OnStartAddr)
	ON_EN_CHANGE(IDC_PRN_END_ADDR, &CPrintDlg::OnEndAddr)

END_MESSAGE_MAP()


void CPrintDlg::OnPrintScreen()
{
	m_bPrintScreen = IsDlgButtonChecked(IDC_PRN_PRINT_SCREEN);
	HideControls();
}


void CPrintDlg::OnPrintCode()
{
	m_bPrintScreen = IsDlgButtonChecked(IDC_PRN_PRINT_SCREEN);
	HideControls();
}


void CPrintDlg::OnInverse()
{
	m_bInverse = IsDlgButtonChecked(IDC_PRN_INVERSE);
}


void CPrintDlg::OnStartAddr()
{
	GetStartAddress();
}


void CPrintDlg::OnEndAddr()
{
	GetEndAddress();
}


void CPrintDlg::OnOk()
{
	if (!GetStartAddress())
	{
		return;
	}

	if (!GetEndAddress())
	{
		return;
	}

	GetDlgItemText(IDC_PRN_TITLE, m_strTitle);
	CPrintDialog::OnOK();
}


void CPrintDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CPrintDialog::OnShowWindow(bShow, nStatus);
	CheckDlgButton(IDC_PRN_PRINT_SCREEN, m_bPrintScreen);
	CheckDlgButton(IDC_PRN_INVERSE, m_bInverse);
	SetDlgItemText(IDC_PRN_START_ADDR, Global::WordToOctString(m_startAddr));
	SetDlgItemText(IDC_PRN_END_ADDR, Global::WordToOctString(m_endAddr));
	SetDlgItemText(IDC_PRN_TITLE, m_strTitle);
	HideControls();
}


void CPrintDlg::HideControls()
{
	CWnd *pWnd = GetDlgItem(IDC_PRN_INVERSE);

	if (pWnd)
	{
		pWnd->EnableWindow(m_bPrintScreen);
	}

	pWnd = GetDlgItem(IDC_PRN_START_ADDR);

	if (pWnd)
	{
		pWnd->EnableWindow(!m_bPrintScreen);
	}

	pWnd = GetDlgItem(IDC_PRN_END_ADDR);

	if (pWnd)
	{
		pWnd->EnableWindow(!m_bPrintScreen);
	}
}


BOOL CPrintDlg::GetStartAddress()
{
	CString strAddress;
	GetDlgItemText(IDC_PRN_START_ADDR, strAddress);
	m_startAddr = Global::OctStringToWord(strAddress);
	CalcPages();
	return TRUE;
}


BOOL CPrintDlg::GetEndAddress()
{
	CString straddress;
	GetDlgItemText(IDC_PRN_END_ADDR, straddress);
	m_endAddr = Global::OctStringToWord(straddress);
	CalcPages();
	return TRUE;
}


void CPrintDlg::CalcPages()
{
	ASSERT(m_pDebugger);
	uint16_t curAddr = m_startAddr;
	CString strInstr;
	uint16_t codes[3];
	int nPages = 1;
	int nLines = 0;

	while (curAddr < m_endAddr)
	{
		curAddr += m_pDebugger->DebugInstruction(curAddr, strInstr, codes) * 2;
		nLines++;

		if (nLines == 60)
		{
			nPages++;
			nLines = 0;
		}
	}

	SetDlgItemInt(IDC_PRN_PAGES, nPages);
	m_nPages = nPages;
}
