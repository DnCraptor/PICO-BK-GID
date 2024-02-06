// BKOscScrOGL.cpp: определяет экспортированные функции для приложения DLL.
//

#include "pch.h"
#include "BKOscScrOGL.h"
#ifdef OPENGL
#pragma comment (lib,"opengl32.lib")
#pragma comment (lib,"glu32.lib")

BKEXTERN_C
{
	BKOSCDLL_API BKOSCSCRHANDLE WINAPI GetBKOscScr()
	{
		return new CBKOscScrOGL;
	}

	// сколько вершин, столько и цветов должно быть
	// группа из трёх чисел - цвет одной вершины
	const std::vector<GLdouble> CBKOscScrOGL::m_cpColors3 =
	{
		1.0, 1.0, 1.0,  1.0, 1.0, 1.0,
		0.5, 0.5, 0.5,  0.5, 0.5, 0.5,
		0.5, 0.5, 0.5,  0.5, 0.5, 0.5
	};

	// группа из двух чисел - координаты вершины
	const std::vector<GLdouble> CBKOscScrOGL::m_cpVertices2 =
	{
		-1.0,  0.0,    1.0,  0.0, // разделительная линия между каналами
		    -1.0,  0.5,    1.0,  0.5, // линии центров каналов.
		    -1.0, -0.5,    1.0, -0.5  // линии центров каналов.
	    };
	// массив порядка просмотра вершин и цветов в двух верхних массивах.
	const std::vector<GLint> CBKOscScrOGL::m_cpIndices1 = { 0, 1, 2, 3, 4, 5 };

	CBKOscScrOGL::CBKOscScrOGL()
		: m_HDC(nullptr)
		, m_hGLRC(nullptr)
		, m_cx(0)
		, m_cy(0)
	{
	}

	CBKOscScrOGL::~CBKOscScrOGL()
	    = default;


	void CBKOscScrOGL::BKOSC_OnSize(const int cx, const int cy)
	{
		if (cx > 0 && cy > 0)
		{
			m_cx = cx;
			m_cy = cy;
		}
	}

	HRESULT CBKOscScrOGL::BKOSC_OnSetBuffer(const int nLen)
	{
		m_pVerticesL.clear();
		m_pVerticesR.clear();
		m_pIndices2.clear();
		m_pVerticesL.resize(static_cast<size_t>(nLen) * 2);
		m_pVerticesR.resize(static_cast<size_t>(nLen) * 2);
		m_pIndices2.resize(nLen);

		if (m_pVerticesL.data() && m_pVerticesR.data() && m_pIndices2.data())
		{
			for (int i = 0; i < nLen; ++i)
			{
				m_pIndices2.at(i) = i;
			}

			return S_OK;
		}

		return E_FAIL;
	}

	HRESULT CBKOscScrOGL::BKOSC_Screen_Init(CWnd * pWndScreen)
	{
		m_HDC = pWndScreen->GetDC()->GetSafeHdc();

		if (m_HDC)
		{
			if (SetWindowPixelFormat())
			{
				m_hGLRC = wglCreateContext(m_HDC);

				if (m_hGLRC)
				{
					if (wglMakeCurrent(m_HDC, m_hGLRC))
					{
						return S_OK;
					}
				}
			}
		}

		wglMakeCurrent(nullptr, nullptr);
		return E_FAIL;
	}

	void CBKOscScrOGL::BKOSC_Screen_Done()
	{
		if (wglGetCurrentContext())
		{
			wglMakeCurrent(nullptr, nullptr);
		}

		if (m_hGLRC)
		{
			wglDeleteContext(m_hGLRC);
			m_hGLRC = nullptr;
		}

		m_HDC = nullptr;
	}

	bool CBKOscScrOGL::SetWindowPixelFormat()
	{
		static PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR), // size of this pfd
			1, // version number
			PFD_DRAW_TO_WINDOW | // support window
			PFD_SUPPORT_OPENGL | // support OpenGL
			PFD_DOUBLEBUFFER | PFD_DEPTH_DONTCARE, // double buffered
			PFD_TYPE_RGBA, // RGBA type
			32, // 32-bit color depth
			0, 0, 0, 0, 0, 0, // color bits ignored
			0, // no alpha buffer
			0, // shift bit ignored
			0, // no accumulation buffer
			0, 0, 0, 0, // accum bits ignored
			32, // 32-bit z-buffer
			0, // no stencil buffer
			0, // no auxiliary buffer
			PFD_MAIN_PLANE, // main layer
			0, // reserved
			0, 0, 0 // layer masks ignored
		};
		int nGLPixelIndex = ::ChoosePixelFormat(m_HDC, &pfd);

		if (nGLPixelIndex == 0) // Let's choose a default index.
		{
			nGLPixelIndex = 1;

			if (::DescribePixelFormat(m_HDC, nGLPixelIndex, sizeof(PIXELFORMATDESCRIPTOR), &pfd) == 0)
			{
				return false;
			}
		}

		return !!::SetPixelFormat(m_HDC, nGLPixelIndex, &pfd);
	}


	bool CBKOscScrOGL::create_context()
	{
		if (m_hGLRC) // каждый раз пересоздаём контекст
		{
			wglMakeCurrent(nullptr, nullptr);
			wglDeleteContext(m_hGLRC);
			m_hGLRC = wglCreateContext(m_HDC);

			if (m_hGLRC)
			{
				return !!wglMakeCurrent(m_HDC, m_hGLRC);
			}
		}

		return false;
	}

	// вход: inBuf - буфер сэмплов
	//      inLen - размер буфера в сэмплах
	void CBKOscScrOGL::BKOSC_OnDisplay(SAMPLE_INT * inBuf, const int inLen)
	{
		if (!wglMakeCurrent(m_HDC, m_hGLRC))
		{
			if (!create_context())
			{
				return;
			}
		}

		glViewport(0, 0, m_cx, m_cy);
		const GLdouble s = static_cast<GLdouble>(inLen) / 2.0;
		// теперь надо сформировать массивы вертексов.
		GLdouble i = 0.0f;
		auto pVL = m_pVerticesL.data();
		auto pVR = m_pVerticesR.data();

		for (int n = inLen; n > 0; n--)
		{
			// рисуем левый канал -1.0 .. 1.0 => 0 .. 1
			const GLdouble fX = i++ / s - 1.001;
			*pVL++ = fX;
			*pVL++ = (static_cast<GLdouble>(*inBuf++) / 2.0 + 0.5);
			// рисуем правый канал -1.0 .. 1.0 => -1 .. 0
			*pVR++ = fX;
			*pVR++ = (static_cast<GLdouble>(*inBuf++) / 2.0 - 0.5);
		}

		glClearColor(0.0, 0.0, 0.0, 0.0); // очистка экрана
		glClear(GL_COLOR_BUFFER_BIT);
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(3, GL_DOUBLE, 0, m_cpColors3.data());
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_DOUBLE, 0, m_cpVertices2.data());
		glDrawElements(GL_LINES, 6, GL_UNSIGNED_INT, m_cpIndices1.data());
		glDisableClientState(GL_COLOR_ARRAY);
		glColor3f(0.0f, 1.0f, 0.0f);
		glVertexPointer(2, GL_DOUBLE, 0, m_pVerticesL.data());
		glDrawElements(GL_LINE_STRIP, inLen, GL_UNSIGNED_INT, m_pIndices2.data());
		glVertexPointer(2, GL_DOUBLE, 0, m_pVerticesR.data());
		glDrawElements(GL_LINE_STRIP, inLen, GL_UNSIGNED_INT, m_pIndices2.data());
		glDisableClientState(GL_VERTEX_ARRAY);
		glFlush();
		SwapBuffers(m_HDC);
		wglMakeCurrent(nullptr, nullptr);
	}

};
#endif