// FFPickerDlg.cpp: файл реализации
//

#include "pch.h"
#include "resource.h"
#include "FFPickerDlg.h"
#include "afxdialogex.h"


// диалоговое окно FFPickerDlg

IMPLEMENT_DYNAMIC(CFFPickerDlg, CDialogEx)

CFFPickerDlg::CFFPickerDlg(CWnd *pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_FFP, pParent)
	, m_strAddress(_T(""))
	, m_bFirstUp(false)
{
}

CFFPickerDlg::~CFFPickerDlg()
    = default;

void CFFPickerDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCSHELLTREE1, m_MFCShellTree);
	DDX_Control(pDX, IDC_MFCSHELLLIST1, m_MFCShellList);
	DDX_Text(pDX, IDC_EDIT1, m_strAddress);
}


BEGIN_MESSAGE_MAP(CFFPickerDlg, CDialogEx)
	ON_REGISTERED_MESSAGE(AFX_WM_CHANGE_CURRENT_FOLDER, &CFFPickerDlg::OnChangeFolder)
END_MESSAGE_MAP()


// обработчики сообщений FFPickerDlg
LRESULT CFFPickerDlg::OnChangeFolder(WPARAM, LPARAM)
{
	CString strPath;

	if (!m_MFCShellList.GetCurrentFolder(strPath) &&
	        !m_MFCShellList.GetCurrentFolderName(strPath))
	{
		strPath = _T("????");
	}
	else
	{
		SetCurrFolder(strPath);
	}

	return 0;
}

void CFFPickerDlg::SetCurrFolder(CString &strPath)
{
	SetDlgItemText(IDC_EDIT1, strPath.GetString());

	// первый вызов приходит во время инициализации диалога, когда контролы ещё не готовы
	// поэтому случается assetrion failed
	// поэтому надо игнорировать UpdateData пока диалога ещё нету.
	if (m_bFirstUp)
	{
		UpdateData(TRUE);
	}
	else
	{
		m_bFirstUp = true;
	}
}


BOOL CFFPickerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_MFCShellTree.SetRelatedList(&m_MFCShellList);
	m_MFCShellList.DisplayFolder(m_strAddress);
	m_SelectedItemList.RemoveAll();
	return TRUE;  // return TRUE unless you set the focus to a control
	// Исключение: страница свойств OCX должна возвращать значение FALSE
}


void CFFPickerDlg::SetStartFolder(CString &strPath)
{
	m_strAddress = strPath;
}


void CFFPickerDlg::OnOK()
{
	const int nSel = m_MFCShellList.GetSelectedCount();
	int nItem = -1;
	CString strPath;

	for (int i = 0; i < nSel; ++i)
	{
		nItem = m_MFCShellList.GetNextItem(nItem, LVNI_SELECTED);

		if (m_MFCShellList.GetItemPath(strPath, nItem))
		{
			m_SelectedItemList.Add(strPath);
		}
	}

	CDialogEx::OnOK();
}
