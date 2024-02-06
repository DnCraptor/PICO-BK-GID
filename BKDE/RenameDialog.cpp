#include "pch.h"
#include "RenameDialog.h"

CRenameDlg::CRenameDlg()
	: CDialogEx(IDD_DIALOG_RENAME)
	, m_strValue(_T(""))
{
}

CRenameDlg::CRenameDlg(const std::wstring &strVal)
	: CDialogEx(IDD_DIALOG_RENAME)
	, m_strValue(CString(strVal.c_str()))
{
}

void CRenameDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_RENFIELD, m_strValue);
}

BEGIN_MESSAGE_MAP(CRenameDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CRenameDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CRenameDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	UpdateData(TRUE);
	CWnd *edit = GetDlgItem(IDC_EDIT_RENFIELD);

	if (edit)
	{
		edit->SetFocus();
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// Исключение: страница свойств OCX должна возвращать значение FALSE
}

void CRenameDlg::OnBnClickedOk()
{
	UpdateData(TRUE);
	CDialogEx::OnOK();
}

