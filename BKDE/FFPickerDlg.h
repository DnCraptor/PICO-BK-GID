#pragma once
#include "afxshelltreectrl.h"
#include "afxshelllistctrl.h"
#include "afxwin.h"


// диалоговое окно FFPickerDlg

class CFFPickerDlg : public CDialogEx
{
		DECLARE_DYNAMIC(CFFPickerDlg)

	public:
		CFFPickerDlg(CWnd *pParent = nullptr);   // стандартный конструктор
		virtual ~CFFPickerDlg() override;
		void SetStartFolder(CString &strPath);

		CStringArray m_SelectedItemList;

	protected:
		CMFCShellTreeCtrl   m_MFCShellTree;
		CMFCShellListCtrl   m_MFCShellList;
		CString             m_strAddress;
		bool                m_bFirstUp;

		void SetCurrFolder(CString &strPath);


// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
		enum { IDD = IDD_DIALOG_FFP };
#endif

	protected:
		virtual BOOL OnInitDialog() override;
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV
		virtual void OnOK() override;
		afx_msg LRESULT OnChangeFolder(WPARAM, LPARAM);
		DECLARE_MESSAGE_MAP()
};
