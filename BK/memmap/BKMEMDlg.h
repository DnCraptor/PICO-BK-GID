
// BKMEMDlg.h : файл заголовка
// !!!доработать

#pragma once
#include "resource.h"
#include "LockVarType.h"
#include "BaseDialog.h"

#define MM_FIRST_PAGE 0
#define MM_SECOND_PAGE 1
#define MM_NUM_PAGES 2

struct MM16kPage_t
{
	bool        bExist;         // флаг, что эта страница существует и её надо перерисовывать
	bool        bColorMode;     // режим отображения текущей страницы - цветной/ЧБ
	bool        bBWAdaptMode;   // режим отображения текущей страницы - адаптивный/обычный
	uint8_t    *pBuffer;        // указатель на отображаемый буфер в окне экрана
	uint16_t    nBufSize;       // размер этого буфера
};

// диалоговое окно CBKMEMDlg
class CScreen;

class CBKMEMDlg : public CBaseDialog
{
		DECLARE_DYNAMIC(CBKMEMDlg)

		static LPCTSTR	m_lpClassName;
		// Создание
	public:
		CBKMEMDlg(BK_DEV_MPI nBKModel, BK_DEV_MPI nBKFDDModel, uint8_t *MainMem, uint8_t *AddMem, CWnd *pParent = nullptr);
		virtual ~CBKMEMDlg() override;
		// Данные диалогового окна

		enum { IDD = IDD_BKMEM_MAP_DLG };

		void        DrawTab();

	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;   // поддержка DDX/DDV

		void        CreateScreen(const int nPage, const int idUI);
		void        CreateTabs_10();
		void        CreateTabs_11M();
		void        AddTabsEXT16();
		void        AddTabsA16M();
		void        AddTabsSMK512();
		void        SelectTab();
		void        SetTabParam(const int nPage);
		void        ChangeColormode(const int nPage);
		void        SetColormode(const int nPage, const bool bMode);
		void        ChangeBWMode(const int nPage);
		void        SetBWMode(const int nPage, const bool bMode);
		void        ViewSprite(const int nPage);
		void        SaveImg(const int nPage);
		void        LoadImg(const int nPage);

		// Реализация
	protected:
		uint8_t    *m_Memory;
		uint8_t    *m_MemoryADD;
		BK_DEV_MPI  m_BKModel;
		BK_DEV_MPI  m_BKFDDmodel;
		LockVarType m_lockStop, m_lockDraw;

		/*
		для конфигурации БК10 - 1 вкладка: 0 и 1 страницы (1 - экран)
		для конфигурации БК11 - 4 вкладки: 8 страниц, по 2 шт. во вкладке
		для A16M - вкладка, с одной страницей. (добавить реализовать такую возможность)
		для смк512 - 16 вкладок
		*/
		int         m_nTabsCount;
		int         m_nSelectedTab;

		CMFCToolTipCtrl m_ToolTip;

		std::unique_ptr<CScreen> m_Screen[MM_NUM_PAGES];
		MM16kPage_t m_Container[20][MM_NUM_PAGES]; // массив окон во вкладках.

		CTabCtrl    m_tab;

		// Созданные функции схемы сообщений
		virtual BOOL OnInitDialog() override;
		virtual void OnCancel() override;
		virtual BOOL PreTranslateMessage(MSG *pMsg) override;
		afx_msg void OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnBnClickedButtonMmColorModeP1();
		afx_msg void OnBnClickedButtonMmBwModeP1();
		afx_msg void OnBnClickedButtonMmLoadP1();
		afx_msg void OnBnClickedButtonMmSaveP1();
		afx_msg void OnBnClickedButtonMmSpriteP1();
		afx_msg void OnBnClickedButtonMmColorModeP2();
		afx_msg void OnBnClickedButtonMmBwModeP2();
		afx_msg void OnBnClickedButtonMmLoadP2();
		afx_msg void OnBnClickedButtonMmSaveP2();
		afx_msg void OnBnClickedButtonMmSpriteP2();
		afx_msg void OnDestroy();
		DECLARE_MESSAGE_MAP()
};

