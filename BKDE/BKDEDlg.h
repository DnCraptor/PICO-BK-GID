
// BKDEDlg.h : файл заголовка
//

#pragma once

#include "afxwin.h"
#include "afxcmn.h"
#include <map>
#include <vector>

#include "Dlgbars.h"
#include "Settings.h"
#include "BKParseImage.h"
#include "BKImage.h"
#include "BKListCtrl.h"



// диалоговое окно CBKDEDlg
class CBKDEDlg : public CDialogEx
{
// Создание
	public:
		CBKDEDlg(CWnd *pParent = nullptr);    // стандартный конструктор

// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
		enum { IDD = IDD_BKDE_DIALOG };
#endif

	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV


// Реализация
	protected:
		static UINT     auIDStatusBar[];

		HICON           m_hIcon;
		HACCEL          m_hAccelTable;
		CSize           m_OrigWndSize;
		CSettings       m_Settings;             // класс для работы с настройками.
		CBKParseImage   m_ParserImage;          // класс парсера образа диска. Приходится создавать объект, т.к. иначе надо будет делать все функции статические
		CBKImage        m_BKImage;
		CDlgStatusBar   m_StatusBar;

		CButton         m_CheckBinExtract;      // чекбокс, работать с бин файлами или нет.
		CButton         m_CheckLongBin;         // Выбор формата сохраняемого BIN файла
		CButton         m_CheckLogExtract;      // чекбокс, создавать ли логи извлекаемых файлов


		CBKListCtrl     m_ListControl;

		bool            m_bStatusBarCreated;    // флаг создания строки состояния, чтобы при изменении размеров в OnSize двигать строку состояния только когда она создана

		fs::path        m_strImageDir;          // директория, где хранятся образы
		fs::path        m_strStorePath;         // путь, по которому сохраняем файлы
		fs::path        m_strLoadPath;          // путь, по которому берём файлы для загрузки, в идеале он может не совпадать с m_strStorePath
		fs::path        m_strImgName;           // имя образа, который обозреваем
		PARSE_RESULT    m_sParseResult;         // структура:
		/* опознанный тип ФС образа
		размер файла образа
		флаг, признак системного образа
		*/
		std::vector<int> m_vecSMIPos;           // вектор, куда сохраняются позиции курсора в списке выбора образов, при навигации по директориям
		int             m_nStartModeItemPos;    // позиция курсора в списке выбора образов
		std::map<UINT, BOOL> m_ButtonsStatus;   // массив разрешающих флагов для кнопок.
		std::map<UINT, BOOL> m_BtnCurrStatus;   // массив текущих флагов, для синхронизации с контекстным меню
		HMENU           m_ApplicationMenu;

		void            FillMenuButton();

		int             ShowMessageBox(UINT strID, UINT nType = MB_OK);
		int             ShowMessageBox(CString &str, UINT nType = MB_OK);
		int             SettingCheckState(const CString &str);
		CString         CheckStateSet(int nCheck);
		void            PreUseLoadSettings();
		void            PreCloseStoreSettings();
		void            FillStatusBar();
		void            ClearStatusBar();

		bool            GetFolder(LPTSTR szPath);

		static int CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
		LRESULT         OnMakeDrop(WPARAM wParam, LPARAM lParam);
		LRESULT         OnGetCmdLine(WPARAM wParam, LPARAM lParam);
		LRESULT         OnSendItemProcessing(WPARAM wParam, LPARAM lParam);
		LRESULT         OnSendImageProcessing(WPARAM wParam, LPARAM lParam);
		LRESULT         OnSendErrorNum(WPARAM wParam, LPARAM lParam);
		LRESULT         OnSendMessageBox(WPARAM wParam, LPARAM lParam);
		LRESULT         OnOutCurrentFilePath(WPARAM wParam, LPARAM lParam);
		LRESULT         OnOutCurrentImageInfo(WPARAM wParam, LPARAM lParam);
		LRESULT         OnOutImage(WPARAM wParam, LPARAM lParam);
		LRESULT         OnPutIntoLD(WPARAM wParam, LPARAM lParam);
		LRESULT         OnSendEnableButton(WPARAM wParam, LPARAM lParam);
		LRESULT         OnGetEnableButton(WPARAM wParam, LPARAM lParam);

		void            OpenImage();
		void            SetEnableButtons(uint32_t flg);
		void            DisableControls();
		void            EnableButton(UINT nID, BOOL bStatus = FALSE);
		ADDOP_RESULT    SendObject(const fs::path &strName);

		bool            ScanCurrDir();
		void            ChangeCurrDir();

		// Созданные функции схемы сообщений
		virtual BOOL OnInitDialog() override;
		virtual BOOL PreTranslateMessage(MSG *pMsg) override;
		virtual void OnOK() override;
		virtual void OnCancel() override;
		afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
		afx_msg void OnPaint();
		afx_msg HCURSOR OnQueryDragIcon();
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnClose();
		afx_msg void OnSetFocus(CWnd *pOldWnd);
		afx_msg void OnDropFiles(HDROP hDropInfo);
		afx_msg void OnGetMinMaxInfo(MINMAXINFO *lpMMI);
		afx_msg void OnBnClickedMfcmenubuttonOpen();
		afx_msg void OnBnClickedCheckBinextract();
		afx_msg void OnBnClickedCheckLogextract();
		afx_msg void OnBnClickedCheckLongbin();
		afx_msg void OnBnClickedClose();
		afx_msg void OnBnClickedButtonOpen();
		afx_msg void OnBnClickedButtonExtract();
		afx_msg void OnBnClickedButtonView();
		afx_msg void OnBnClickedButtonAbout();
		afx_msg void OnBnClickedButtonAdd();
		afx_msg void OnBnClickedButtonViewSprite();
		afx_msg void OnBnClickedButtonRename();
		afx_msg void OnBnClickedButtonDelete();
		afx_msg BOOL OnToolTipText(UINT nID, NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg BOOL OnHelpInfo(HELPINFO *pHelpInfo);
		afx_msg void OnContextMenuRename();
		afx_msg void OnContextChangeAddr();
		afx_msg void OnContextDelete();
		afx_msg void OnContextExtract();
		afx_msg void OnContextViewastext();
		afx_msg void OnContextViewassprite();
		DECLARE_MESSAGE_MAP()
};
