#pragma once
#ifdef BKSCRDLL_EXPORTS
#define BKSCRDLL_API __declspec(dllexport)
#else
#define BKSCRDLL_API __declspec(dllimport)
#endif

#ifdef OPENGL

// Этот класс экспортирован из BKScreenOGL.dll
#include "Screen_Shared.h"

#include "gl/gl.h"
#include "gl/glu.h"
#include "wglext.h"

using RGBCols = struct
{
	GLdouble r;
	GLdouble g;
	GLdouble b;
};

BKEXTERN_C class BKSCRDLL_API CScreenOGL : public CBKScreen_Shared
{
		static const GLdouble m_cpTexcoords2[8];
		GLdouble m_cpVertices2[8];
		static const GLint    m_cpIndices1[4];

	protected:
		bool                m_bScrParamChanged;
		bool                m_bScrSizeChanged;

		HDC                 m_HDC;

		HGLRC               hGLRC;
		RECT                m_rectWndVP;    // размеры viewporta в оконном режиме
		RECT                m_rectViewPort; // текущие размеры viewporta
		// left, top - координаты экрана, right, bottom - размеры, а не координаты!!!

		PIXELFORMATDESCRIPTOR m_pfd;
		GLint               m_nTextureParam;

		PFNWGLSWAPINTERVALEXTPROC       wglSwapIntervalEXT;
		PFNWGLGETSWAPINTERVALEXTPROC    wglGetSwapIntervalEXT;

	public:
		CScreenOGL();
		virtual ~CScreenOGL() override;

		virtual HRESULT     BKSS_ScreenView_Init(BKScreen_t *pScPar, CWnd *pwndScr) override;
		virtual HRESULT     BKSS_ScreenView_ReInit(BKScreen_t *pScPar) override;
		virtual void        BKSS_ScreenView_Done() override;
		virtual void        BKSS_DrawScreen() override;
		virtual bool        BKSS_SetFullScreenMode() override;
		virtual bool        BKSS_SetWindowMode() override;

		virtual void        BKSS_SetSmoothing(bool bSmoothing) override;
		virtual void        BKSS_SetColorMode() override;
		virtual void        BKSS_OnSize(int cx, int cy) override;

	protected:
		void                CreateContext();
		bool                SetWindowPixelFormat();
		void                set_screen_param();
		void                clear();

		bool WGLExtensionSupported(const char *extension_name);
};

#endif
