#include "pch.h"
#include "ChangeAddrDialog.h"

CChangeAddrDlg::CChangeAddrDlg()
	: CDialogEx(IDD_DIALOG_CHANGEADDR)
	, m_strValue(_T(""))
{
}

CChangeAddrDlg::CChangeAddrDlg(const int nVal)
	: CDialogEx(IDD_DIALOG_CHANGEADDR)
{
// #pragma warning(disable:4996)
//  wchar_t buff[16];
//  _snwprintf(buff, 16, L"%06o\0", nVal);
//  m_strValue = CString(buff);
// если кстринг, то чего уж, и функцией специальной воспользуемся
	m_strValue.Format(_T("%06o"), nVal);
}

int CChangeAddrDlg::GetAddr()
{
	int word = _tcstol(m_strValue.GetString(), nullptr, 8);
	return word;
}

void CChangeAddrDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_CHANGEADDR, m_strValue);
}

BEGIN_MESSAGE_MAP(CChangeAddrDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CChangeAddrDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CChangeAddrDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	UpdateData(TRUE);
	CWnd *edit = GetDlgItem(IDC_EDIT_CHANGEADDR);

	if (edit)
	{
		edit->SetFocus();
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// Исключение: страница свойств OCX должна возвращать значение FALSE
}

void CChangeAddrDlg::OnBnClickedOk()
{
	UpdateData(TRUE);
	CDialogEx::OnOK();
}

