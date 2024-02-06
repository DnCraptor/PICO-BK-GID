#pragma once
#ifdef UI
#include "SprScr.h"

// CSprView

class CSprView : public CScrollView
{
		DECLARE_DYNAMIC(CSprView)

		std::unique_ptr<CSprScr> m_pScreen;
		int     m_cx, m_cy; // размеры viewportа

	public:
		CSprView();
		virtual ~CSprView() override;
		// сквозные функции, просто передают параметры дальше
		inline void SetParameters(std::unique_ptr<uint8_t[]> buffer, const size_t size, const int scale)
		{
			m_pScreen->SetParameters(std::move(buffer), size, scale);
		}

		inline void SetScale(const int scale) const
		{
			m_pScreen->SetScale(scale);
		}

		inline void SetPalette(const int palette) const
		{
			m_pScreen->SetPalette(palette);
		}

		inline int GetPalette() const
		{
			return m_pScreen->GetPalette();
		}

		inline void SetAdaptMode(const bool bFlag) const
		{
			m_pScreen->SetAdaptMode(bFlag);
		}

		inline bool IsAdaptMode() const
		{
			return m_pScreen->IsAdaptMode();
		}

		inline void SetColorMode(const bool bColorMode = true) const
		{
			m_pScreen->SetColorMode(bColorMode);
		}

		inline bool IsColorMode() const
		{
			return m_pScreen->IsColorMode();
		}

	protected:
		virtual void OnDraw(CDC *pDC) override;
		virtual BOOL PreCreateWindow(CREATESTRUCT &cs) override;
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg void OnDestroy();
		DECLARE_MESSAGE_MAP()
};
#endif