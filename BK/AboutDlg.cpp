/*
 * Copyright (C) 2022 SCALOlaz
 * 1,2,3 pages for ABOUT
 */
#ifdef UI
#include "pch.h"
//#include "resource.h"
#include "AboutDlg.h"

// 1 page
IMPLEMENT_DYNAMIC(AboutDlg, CDialogEx)
AboutDlg::AboutDlg(CWnd *pParent /*=nullptr*/)
	: CDialogEx(IDD, pParent)
{}

AboutDlg::~AboutDlg()
    = default;

void AboutDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_HOMEPAGE, m_staticHomePage);
	DDX_Control(pDX, IDC_STATIC_HOMEPAGE2, m_staticHomePage2);
	DDX_Control(pDX, IDC_STATIC_MAIL, m_staticMail);
	DDX_Control(pDX, IDC_STATIC_MAIL2, m_staticMail2);
}

BEGIN_MESSAGE_MAP(AboutDlg, CDialogEx)
END_MESSAGE_MAP()

BOOL AboutDlg::OnInitDialog()
{
	return CDialogEx::OnInitDialog();
}

// 2 page
IMPLEMENT_DYNAMIC(AboutLinks, CDialogEx)

AboutLinks::AboutLinks(CWnd *pParent /*=nullptr*/)
	: CDialogEx(IDD, pParent)
{}

AboutLinks::~AboutLinks()
    = default;

void AboutLinks::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_GAMES, m_staticGames);
	DDX_Control(pDX, IDC_STATIC_FORUM, m_staticForum);
	DDX_Control(pDX, IDC_STATIC_SVN, m_staticSVN);
	DDX_Control(pDX, IDC_STATIC_SVN2, m_staticSVN2);
	DDX_Control(pDX, IDC_STATIC_SVN3, m_staticSVN3);
	DDX_Control(pDX, IDC_STATIC_SVN4, m_staticSVN4);
	DDX_Control(pDX, IDC_STATIC_SVN5, m_staticSVN5);
	// and Some Other Links
}

BEGIN_MESSAGE_MAP(AboutLinks, CDialogEx)
END_MESSAGE_MAP()

BOOL AboutLinks::OnInitDialog()
{
	return CDialogEx::OnInitDialog();
}

// 3 page
IMPLEMENT_DYNAMIC(AboutThx, CDialogEx)

LPCTSTR	AboutThx::m_lpClassName = _T("MyHtmlClass");

AboutThx::AboutThx(CWnd *pParent /*=nullptr*/)
	: CDialogEx(IDD, pParent)
	, m_pFrmWnd(nullptr)
{}

AboutThx::~AboutThx()
{
	UnregisterClass(m_lpClassName, AfxGetInstanceHandle());
}

void AboutThx::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(AboutThx, CDialogEx)
END_MESSAGE_MAP()

BOOL AboutThx::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	CRect rect;
	GetDlgItem(IDC_ABOUT_EDIT)->GetWindowRect(&rect);
	ScreenToClient(&rect);
	m_pFrmWnd = new CHtmlWnd(); // эту штуку нельзя делать умным указателем, т.к. там всё как-то удаляется ещё до того, как начинает работать механизм умного удаления

	// и вот меня гложет чувство, что всё это можно делать как-то проще, все эти фреймы и виды в них.
	// но как - не знаю.
	if (m_pFrmWnd)
	{
		WNDCLASS wc;
		memset(&wc, 0, sizeof(wc));
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_PARENTDC;
		wc.hInstance = AfxGetInstanceHandle();
		wc.lpfnWndProc = ::DefWindowProc;
		wc.lpszClassName = m_lpClassName;
		wc.hbrBackground = (HBRUSH)::GetStockObject(GRAY_BRUSH);
		AfxRegisterClass(&wc);

		if (m_pFrmWnd->Create(
		            m_lpClassName, // Имя класса виндовс
		            _T("WebThanxWnd"), // имя окна
		            WS_CHILD | WS_VISIBLE, // window styles
		            rect, // размер окна
		            this, //GetParentOwner(), // родитель окна
		            nullptr,
		            0 // extended window styles
		        ))
		{
			m_pFrmWnd->ShowWindow(SW_SHOW);
		}
	}

	return TRUE;
}

IMPLEMENT_DYNAMIC(CHtmlWnd, CFrameWnd)

CHtmlWnd::CHtmlWnd()
	: m_Ie(nullptr)
{
}

CHtmlWnd::~CHtmlWnd()
    = default;

int CHtmlWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	m_Ie = dynamic_cast<CHtmlView *>(RUNTIME_CLASS(CHtmlView)->CreateObject());

	if (m_Ie)
	{
		if (m_Ie->Create(nullptr, _T("WebThanx"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CRect(0, 0, lpCreateStruct->cx, lpCreateStruct->cy), this, IDC_ABOUT_EDIT, nullptr))
		{
			m_Ie->LoadFromResource(IDR_HTML1);
			SetActiveView(m_Ie);
		}
	}

	return 0;
}

BEGIN_MESSAGE_MAP(CHtmlWnd, CFrameWnd)
	ON_WM_CREATE()
END_MESSAGE_MAP()
#endif