#pragma once
#include "BKOscScr_Shared.h"

#ifdef BKOSCDLL_EXPORTS
#define BKOSCDLL_API __declspec(dllexport)
#else
#define BKOSCDLL_API __declspec(dllimport)
#endif

#ifdef OPENGL

#include <gl/gl.h>
#include <gl/glu.h>
#include <vector>

#pragma warning(disable:4251) // задолбало, это предупреждение из-за компиляции с ключом /MTd

BKEXTERN_C class BKOSCDLL_API CBKOscScrOGL: public CBKOScScr_Shared
{
		static const std::vector<GLdouble> m_cpColors3;
		static const std::vector<GLdouble> m_cpVertices2;
		static const std::vector<GLint>    m_cpIndices1;

		int             m_cx, m_cy;

		std::vector<GLdouble> m_pVerticesL;
		std::vector<GLdouble> m_pVerticesR;
		std::vector<GLint> m_pIndices2;

		HDC             m_HDC;
		HGLRC           m_hGLRC;

		bool SetWindowPixelFormat();
		bool create_context();

	public:
		virtual void BKOSC_OnDisplay(SAMPLE_INT *inBuf, const int inLen) override;
		virtual HRESULT BKOSC_Screen_Init(CWnd *pWndScreen) override;
		virtual void BKOSC_Screen_Done() override;
		virtual void BKOSC_OnSize(const int cx, const int cy) override;
		virtual HRESULT BKOSC_OnSetBuffer(const int nLen) override;

		CBKOscScrOGL();
		virtual ~CBKOscScrOGL() override;
};

#endif