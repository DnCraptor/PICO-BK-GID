#include "resource.h"       // основные символы

// Диалоговое окно CChangeAddrDlg используется для описания сведений о приложении

class CChangeAddrDlg : public CDialogEx
{
	protected:
		CString m_strValue; // DDX_Text не поддерживает std::wstring, только CString

	public:
		CChangeAddrDlg();
		CChangeAddrDlg(const int nVal);
		int GetAddr();
// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
		enum { IDD = IDD_DIALOG_CHANGEADDR };
#endif

	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV

// Реализация
	protected:
		virtual BOOL OnInitDialog() override;
		afx_msg void OnBnClickedOk();

		DECLARE_MESSAGE_MAP()
};

