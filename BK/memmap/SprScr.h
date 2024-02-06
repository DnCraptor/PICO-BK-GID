#pragma once

#include <vfw.h>

class CSprScr : public CWnd
{
		DECLARE_DYNAMIC(CSprScr)

		std::unique_ptr<uint8_t[]> m_pBuffer;
		size_t          m_nBufSize;

		int             m_cx, m_cy;     // размеры окна viewporta, с учётом масштабирования
		int             m_orig_cx, m_orig_cy;   // размеры Картинки без учёта масштабирования
		int             m_nScale;       // текущее масштабирование
		int             m_PaletteNum;

		bool            m_bColorMode;   // флаг цветного режима
		bool            m_bAdapt;       // флаг адаптивного ЧБ режима

		bool            m_bBusy;        // флаг, что занято выводом на экран
		bool            m_bChangeMode;  // флаг, что занято изменением режимов, как правило - размера

		uint32_t       *m_bits;

		HWND            m_hwndScreen;  // Screen View window handle
		HDRAWDIB        m_hdd;
		HBITMAP         m_hbmp;
		BITMAPINFO      m_bmpinfo;
		std::vector<uint32_t> m_vColTable32;
		std::vector<uint32_t> m_vMonoTable32;

	public:
		CSprScr();
		virtual ~CSprScr() override;
		void DrawScreen();
		void ReDrawScreen();
		void AdjustLayout(const int cx, const int cy, const int ysp);

		void SetParameters(std::unique_ptr<uint8_t[]> buffer, const size_t size, const int scale)
		{
			m_pBuffer = std::move(buffer);
			m_nBufSize = size;
			SetScale(scale);
		}

		inline size_t GetBufferSize() const
		{
			return m_nBufSize;
		}

		void SetScale(const int scale);

		inline int GetScale() const
		{
			return m_nScale;
		}

		void SetPalette(int palette);

		inline int GetPalette() const
		{
			return m_PaletteNum;
		}

		void SetAdaptMode(const bool bFlag);

		inline bool IsAdaptMode() const
		{
			return m_bAdapt;
		}

		void SetColorMode(const bool bColorMode = true);

		inline bool IsColorMode() const
		{
			return m_bColorMode;
		}

	protected:
		void			ScreenView_Init();
		void			ScreenView_Done();
		void			InitColorTables();
		void			PrepareScreenRGB32();
		void			CalcOrigSize();
		virtual BOOL	PreCreateWindow(CREATESTRUCT &cs) override;
		afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg void	OnDestroy();
		afx_msg BOOL	OnEraseBkgnd(CDC *pDC);
		DECLARE_MESSAGE_MAP()
};

