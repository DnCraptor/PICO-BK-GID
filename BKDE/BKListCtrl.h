#pragma once
#include "afxcmn.h"

#include "ImgUtil.h"

class CBKListCtrl :
	public CListCtrl
{
		DECLARE_DYNAMIC(CBKListCtrl)

	public:
		enum class MODE_CTRL {START, MAIN}; // режимы работы контрола.
		// номера колонок для основного режима
		enum { LC_FNAME_POS = 0, LC_TYPE_POS, LC_BLK_SIZE_POS, LC_ADDRESS_POS, LC_SIZE_POS, LC_ATTR_POS, LC_SPECIFIC_POS };
		// номера колонок для стартового режима
		enum { LC_FNAME_ST = 0, LC_SIZE_ST, LC_OSTYPE_ST, LC_SYSTYPE_ST };

	protected:
		CFont			m_fontBold, m_font;
		bool			m_bSpecificColumn;
		MODE_CTRL		m_nWorkMode; // текущий режим работы

		bool			m_bSelectAll;
		int				m_nLastMouseClicked;

		void			ClearSelectAll();
		void			SetSelectAll();
		void			SetSelectCurrent(int nItem);
		void			SetSelectRange(int nFrom, int nTo) const;

		void			StepTo(int nFlags, int nItem, bool bMM);
		void			MouseClick(int nItem);
		void			EnterTo(int nItem) const;
		void			ImgEnterTo(int nItem) const;
		void			StepLeft(int nItem, bool bMM);
		void			StepRight(int nItem, bool bMM);
		void			ChangeSelection(const int nItem, const int nNextItem, const bool bMM);

		virtual void    PreSubclassWindow() override;

	public:
		CBKListCtrl();
		virtual ~CBKListCtrl() override;

		void			Init(MODE_CTRL nMode = MODE_CTRL::MAIN);
		void			SetSpecificColumn(UINT nID);
		static int CALLBACK MyCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
		static int CALLBACK MyCompareProc2(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	public:
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	protected:
		afx_msg void OnCustomDraw(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnNMClick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnDropFiles(HDROP hDropInfo);
		afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
		afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
		DECLARE_MESSAGE_MAP()
};

