#include "resource.h"       // основные символы

// Диалоговое окно CRenameDlg используется для описания сведений о приложении

class CRenameDlg : public CDialogEx
{
	protected:
		CString m_strValue; // DDX_Text не поддерживает std::wstring, только CString

	public:
		CRenameDlg();
		CRenameDlg(const std::wstring &strVal);
		std::wstring GetName()
		{
			return std::wstring(m_strValue.GetString());
		}
// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
		enum { IDD = IDD_DIALOG_RENAME };
#endif

	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV

// Реализация
	protected:
		virtual BOOL OnInitDialog() override;
		afx_msg void OnBnClickedOk();

		DECLARE_MESSAGE_MAP()
};

