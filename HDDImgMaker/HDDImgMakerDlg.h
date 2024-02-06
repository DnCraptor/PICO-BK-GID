
// HDDImgMakerDlg.h : файл заголовка
//

#pragma once
#include "afxwin.h"
#include "Ini.h"
#include "HDIStuff.h"
#include <mutex>

using TOOLTIP_STRUCT = struct
{
	int nID;
	UINT nStrResID;
};

#define WM_END_PROCESS      WM_USER + 11
#define WM_ONSTART_PROGRESS WM_USER + 12
#define WM_ONSTOP_PROGRESS  WM_USER + 13
#define WM_ONSEND_PROGRESS  WM_USER + 14

// диалоговое окно CHDDImgMakerDlg
class CHDDImgMakerDlg : public CDialogEx
{
		CSpinButtonCtrl m_spinCylinders;
		CSpinButtonCtrl m_spinHeads;
		CSpinButtonCtrl m_spinSectors;
		CProgressCtrl   m_progress;
		CComboBox       m_TypeCombo;
		CToolTipCtrl    m_tt;

		static const fs::path strVHDDext[3];
		fs::path        m_strAppPath;       // Текущий путь, где создавать файл образа по умолчанию
		CString         m_strImgName;       // Имя создаваемого файла образа, переменная - завязанная на поле ввода IDC_EDIT_IMGNAME
		fs::path        m_pathImgName;      // Имя создаваемого файла образа - эта переменная используется для работы с путями
		CString         m_strExistingImgName;   // Имя существующего файла образа для конвертации, переменная - завязанная на поле ввода IDC_EDIT_EXISTING_IMGNAME
		fs::path        m_pathExistingImgName;  // Имя существующего файла образа для конвертации - эта переменная используется для работы с путями
		fs::path        m_strExistingImgPath; // Текущий путь, где искать существующие файлы образов по умолчанию
		CString         m_strModelName;     // Модель HDD, переменная завязанная на поле ввода IDC_EDIT_MODELNAME
		CString         m_strSerialNumber;  // Серийный номер HDD, переменная завязанная на поле ввода IDC_EDIT_SERNUMBER
		int             m_nCylinders;
		int             m_nHeads;
		int             m_nSectors;
		int             m_barMaxvalue;
		bool            m_bCancel;          // флаг отмены выбора через диалог результирующего файла
		bool            m_bConvert;         // флаг конвертации существующего образа
		bool            m_bChangeGeometry;  // флаг разрешения менять геометрию
		BOOL            m_bReduceSize;      // опция - обрезать размер файла, завязана на чекбокс IDC_CHECK_REDUCESIZE
		HICON           m_hIcon;
		HMENU           m_ButtonMenu;

		FILE           *m_file;
		FILE           *m_fileExisting;

		SYS_SECTOR      m_sector0;

		volatile bool   m_bBreakThr;        // флаг прерывания потока.
		std::mutex      m_mutThr;           // флаг работы потока

// Создание
	public:
		CHDDImgMakerDlg(CWnd *pParent = nullptr);  // стандартный конструктор

// Данные диалогового окна
		enum { IDD = IDD_HDDIMGMAKER_DIALOG };

	protected:
		virtual void    DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV
		virtual BOOL    PreTranslateMessage(MSG *pMsg) override;


// Реализация
	protected:
		int             ShowMessageBox(UINT msgID, UINT nType = 0, UINT nIDhelp = 0);
		int             ShowMessageBox(UINT msgID, CString str, UINT nType = 0, UINT nIDhelp = 0);
		int             ShowMessageBoxErr(UINT msgID, UINT nType = 0, UINT nIDhelp = 0, bool reg = false);
		int             ShowMessageBoxErr(UINT msgID, CString str, UINT nType = 0, UINT nIDhelp = 0, bool reg = false);

		void            OutByteSize();
		void            _UpdateData();
		void            CorrectGeometry(const __int64 nFileSize);
		void            AnalyseImage(FILE *pFile, const __int64 nFileSize);
		void            EnableGeometryChange(bool bEnable);
		void            ChangeMakeButtonText();
		void            MakeSysSector();
		void            WorkHDI(int nTotalSectors);
		void            WorkHDIX(int nTotalSectors);

		void            ConvertHdi2Img();
		void            ConvertHdi2Hdix();
		void            ConvertHdix2Hdi();
		void            threadFunc(const int nTotalSectors, const int nType);
		void            SetProgress(int v);
		void            BAR_Error(bool reg = false);
		void            BAR_Normal(const int pos, const int val = 255);

		// Созданные функции схемы сообщений
		virtual BOOL OnInitDialog() override;
		afx_msg LRESULT OnEndProcess(WPARAM wp, LPARAM);
		afx_msg LRESULT OnStartProgress(WPARAM, LPARAM);
		afx_msg LRESULT OnStopProgress(WPARAM, LPARAM);
		afx_msg LRESULT OnSendProgress(WPARAM wp, LPARAM);
		afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
		afx_msg void OnPaint();
		afx_msg HCURSOR OnQueryDragIcon();
		afx_msg void OnDestroy();

		afx_msg void OnEnChangeEditCylinders();
		afx_msg void OnEnChangeEditHeads();
		afx_msg void OnEnChangeEditSectors();
		afx_msg void OnBnClickedButtonSernumber();
		afx_msg void OnBnClickedButtonBrowse();
		afx_msg void OnBnClickedButtonBrowseExisting();
		afx_msg void OnBnClickedButtonMake();
		afx_msg void OnEnKillfocusEditImgname();
		afx_msg void OnEnKillfocusEditExistingImgname();
		afx_msg void OnEnKillfocusEditSectors();
		afx_msg void OnEnSetfocusEditSectors();
		afx_msg void OnEnKillfocusEditHeads();
		afx_msg void OnEnSetfocusEditHeads();
		afx_msg void OnEnKillfocusEditCylinders();
		afx_msg void OnEnSetfocusEditCylinders();
		afx_msg void OnCbnSelchangeComboHditype();
		afx_msg void OnBnClickedMfcmenubutton1();
		afx_msg void OnBnClickedCheckReducesize();
		DECLARE_MESSAGE_MAP()
};
