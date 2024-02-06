
// BK.cpp : Определяет поведение классов для приложения.
//
#ifdef UI
#include "pch.h"
#include "BK.h"

#include "MainFrm.h"
#include "BKDoc.h"
#include "BKView.h"
#include "StaticLink.h"
#include "Config.h"
#include "ToolManager.h"
#include "variables.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CBKApp

BEGIN_MESSAGE_MAP(CBKApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CBKApp::OnAppAbout)
	// Стандартная команда настройки печати
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
END_MESSAGE_MAP()


// создание CBKApp

CBKApp::CBKApp()
	: m_mtInstance(TRUE, _T("BK Emulator Instance"))
	, m_strProgramTitleVersion(_T(""))
	, m_strProgramName(_T(""))
	, m_strCompileVersion(_T(""))
	, m_nAppLook(0)
	, m_bIsCopy(false)
	, m_bHiColorIcons(true)
	, m_bNewConfig(false)
{
	// TODO: замените ниже строку идентификатора приложения строкой уникального идентификатора; рекомендуемый
	// формат для строки: ИмяКомпании.ИмяПродукта.СубПродукт.СведенияОВерсии
	SetAppID(_T("gid.EMULATOR.BK0010-0011M.Win"));
	// регистрируем наше глобальное сообщение
	m_nInterAppGlobalMsg = RegisterWindowMessage(_T("Inter BKEmu Application Global Message"));
	// TODO: добавьте код создания,
	// Размещает весь важный код инициализации в InitInstance
}

CBKApp::~CBKApp()
{
	if (!m_bIsCopy)
	{
		m_mtInstance.Unlock();
	}
}

// Единственный объект CBKApp

CBKApp theApp;


// инициализация CBKApp

BOOL CBKApp::InitInstance()
{
	_tsetlocale(LC_ALL, _T("Russian"));
	// InitCommonControlsEx() требуются для Windows XP, если манифест
	// приложения использует ComCtl32.dll версии 6 или более поздней версии для включения
	// стилей отображения. В противном случае будет возникать сбой при создании любого окна.
	INITCOMMONCONTROLSEX InitCtrls{};
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Выберите этот параметр для включения всех общих классов управления, которые необходимо использовать
	// в вашем приложении.
	InitCtrls.dwICC = ICC_WIN95_CLASSES | 0xff00; // сделаем вообще всё, даже не существующее в WinXP, всё равно ни на что не влияет.
	InitCommonControlsEx(&InitCtrls);
	CWinAppEx::InitInstance();

	// Инициализация библиотек OLE
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	// Проверка на две запущенные копии.
	if (!m_mtInstance.Lock(0)) // если не можем заблокировать мутекс, значит прога уже запущена
	{
		m_bIsCopy = true;
		// Если существует другой экземпляр эмулятора, найдём его окно
		// посылаем свой ИД потока, чтобы сюда получить HWND окна основной проги.
		TRACE("copy thread -> broadcast\n");
		::PostMessage(HWND_BROADCAST, m_nInterAppGlobalMsg, QUESTION_PRIME_HWND, static_cast<LPARAM>(this->m_nThreadID));
		return TRUE; // а дальше ничего делать не надо.
	}

	AfxEnableControlContainer();
	EnableTaskbarInteraction(TRUE);
	// Для использования элемента управления RichEdit требуется метод AfxInitRichEdit2()
	// AfxInitRichEdit2();
	// Стандартная инициализация
	// Если эти возможности не используются и необходимо уменьшить размер
	// конечного исполняемого файла, необходимо удалить из следующего
	// конкретные процедуры инициализации, которые не требуются
	// Измените раздел реестра, в котором хранятся параметры
	// TODO: следует изменить эту строку на что-нибудь подходящее,
	// например на название организации
	SetRegistryKey(_T("gid prod."));
	// подменим профиль, чтобы не плодить кучу разных настроек в реестре
	free((void *)m_pszProfileName); // освободим память выделенную под прошлое имя
	m_pszProfileName = _tcsdup(_T("BKEMUL")); // зададим своё
	// LoadStdProfileSettings(0);  // Загрузите стандартные параметры INI-файла (включая MRU)
	InitContextMenuManager();
	InitShellManager();
	InitKeyboardManager();
	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
	                                             RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);
	// Зарегистрируйте шаблоны документов приложения. Шаблоны документов
	//  выступают в роли посредника между документами, окнами рамок и представлениями
	EnableD2DSupport(D2D1_FACTORY_TYPE_MULTI_THREADED);
	CSingleDocTemplate *pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
	    IDR_MAINFRAME,
	    RUNTIME_CLASS(CBKDoc),
	    RUNTIME_CLASS(CMainFrame),       // основное окно рамки SDI
	    RUNTIME_CLASS(CBKView));

	if (!pDocTemplate)
	{
		TRACE0("Не удалось создать CSingleDocTemplate.\n");
		AfxMessageBox(_T("Не удалось создать CSingleDocTemplate.\n"));
		return FALSE;
	}

	AddDocTemplate(pDocTemplate);
	// инициализируем публичные переменные
	// Version build
#define STRINGIZE1(a) #a
#define STRINGIZE(a) STRINGIZE1(a)
#if (_MSC_VER >= 1930)
	CString strVN = _T(", VS 2022");
#elif (_MSC_VER >= 1920)
	CString strVN = _T(", VS 2019");
#elif (_MSC_VER >= 1910)
	CString strVN = _T(", VS 2017");
#elif (_MSC_VER >= 1900)
	CString strVN = _T(", VS 2015");
// вот это вот всё можно смело удалять. потому что в этих студиях собираться не будет.
//     разве что в 2013, если долго мучиться и убирать новомодные конструкции и править вызовы АПИ
// #elif (_MSC_VER >= 1800) // 1800: MSVC 2013 (yearly release cycle)
//  CString strVN = _T(", VS 2013");
//  // Не уверен что приложение сможет собираться в ранних версиях Студии и вот это всё ниже - нужно нам. Отселе
// #elif (_MSC_VER >= 1700)
//  CString strVN = _T(", VS 2012");
// #elif (_MSC_VER >= 1600)
//  CString strVN = _T(", VS 2010");
// #elif (_MSC_VER >= 1500)
//  CString strVN = _T(", VS 2008");
// #elif (_MSC_VER >= 1400)
//  CString strVN = _T(", VS 2005");
// #elif (_MSC_VER >= 1310)
//  CString strVN = _T(", VS 2003");
// #elif (_MSC_VER >= 1300)
//  CString strVN = _T(", VS v7.0");
// #elif (_MSC_VER >= 1200)
//  CString strVN = _T(", VS v6.0");
// #elif (_MSC_VER >= 1100)
//  CString strVN = _T(", VS v5.0");
//  // Доселе
#else
	CString strVN = _T(", VS Old");
#endif
#ifdef SCALOLAZ_VERSION
	m_strCompileVersion = _T("Compiled: ") + Global::getCompileDate(_T("%d-%m-%Y")) + _T(", ") + Global::getCompileTime(_T("%H:%M:%S"))
	                      // Вот кстати %T Equivalent to %H:%M:%S, the ISO 8601 time format
	                      // подробнее про форматы даты времени там: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/strftime-wcsftime-strftime-l-wcsftime-l?view=msvc-170
	                      // зачем писать много, когда можно коротко.
	                      // да и дата по русски выглядит привычней: 31 окт 2022 лучше ведь, чем 31-10-2022
#else
	m_strCompileVersion = _T("Compiled: ") + Global::getCompileDate(_T("%d %b %Y")) + _T(", ") + Global::getCompileTime(_T("%T"))
#endif
	                      + strVN + _T(" (") _T(STRINGIZE(_MSC_FULL_VER)) _T(".") _T(STRINGIZE(_MSC_BUILD)) _T(")");
	m_strProgramName = CString(m_pszExeName) + _T(".exe");;
	MakeTitleVersion();
	m_bNewConfig = g_Config.InitConfig(CString(MAKEINTRESOURCE(IDS_INI_FILENAME)));
	g_Config.VerifyRoms(); // проверим наличие, но продолжим выполнение при отсутствии чего-либо
	// Разрешить использование расширенных символов в горячих клавишах меню
	CMFCToolBar::m_bExtCharTranslation = TRUE;
	// Синтаксический разбор командной строки на стандартные команды оболочки, DDE, открытие файлов
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// чтобы самим обрабатывать командную строку подправим немного результат парсинга.
	// в принципе можно вообще закомментировать строку выше.
	cmdInfo.m_nShellCommand = CCommandLineInfo::FileNew;
	// Включить открытие выполнения DDE
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	// Команды диспетчеризации, указанные в командной строке. Значение FALSE будет возвращено, если
	// приложение было запущено с параметром /RegServer, /Register, /Unregserver или /Unregister.
	if (!ProcessShellCommand(cmdInfo))
	{
		TRACE0("Неверные команды диспетчеризации.\n");
		AfxMessageBox(_T("Неверные команды диспетчеризации.\n"));
		return FALSE;
	}

	// Одно и только одно окно было инициализировано, поэтому отобразите и обновите его
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	// вызов DragAcceptFiles только при наличии суффикса
	//  В приложении SDI это должно произойти после ProcessShellCommand
	// Включить открытие перетаскивания
	m_pMainWnd->DragAcceptFiles();
	return TRUE;
}

int CBKApp::ExitInstance()
{
	if (!m_bIsCopy)
	{
		g_Config.UnInitConfig();
	}

	AfxOleTerm(FALSE);
	return CWinAppEx::ExitInstance();
}

// обработчики сообщений CBKApp
void CBKApp::MakeTitleVersion()
{
	DWORD dwHandle;
	DWORD dwSize = ::GetFileVersionInfoSize(m_strProgramName.GetString(), &dwHandle);

	if (dwSize)
	{
		auto lpData = std::vector<TCHAR>(dwSize);

		if (lpData.data())
		{
			::GetFileVersionInfo(m_strProgramName.GetString(), dwHandle, dwSize, lpData.data());
			UINT uBufLen;
			VS_FIXEDFILEINFO *lpfi = nullptr;
			::VerQueryValue(lpData.data(), _T("\\"), (void **)&lpfi, &uBufLen);
			CString t(MAKEINTRESOURCE(IDS_EMUL_TITLE));
			CString s;
#ifdef SCALOLAZ_VERSION
			s.Format(_T(" v%d.%d r%d "), // " v%d.%d.%d.%d "
			         HIWORD(lpfi->dwFileVersionMS), LOWORD(lpfi->dwFileVersionMS),
			         /*HIWORD(lpfi->dwFileVersionLS),*/ LOWORD(lpfi->dwFileVersionLS));
#else
			s.Format(_T(" v%d.%d.%d.%d "),
			         HIWORD(lpfi->dwFileVersionMS), LOWORD(lpfi->dwFileVersionMS),
			         HIWORD(lpfi->dwFileVersionLS), LOWORD(lpfi->dwFileVersionLS));
#endif
#ifdef _WIN64
#define TITLE_SUFFIX _T("x64")
#else
#define TITLE_SUFFIX _T("x86")
#endif
			m_strProgramTitleVersion = t + s + TITLE_SUFFIX;
#undef TITLE_SUFFIX
		}
	}
}


// CBKApp настройка методов загрузки и сохранения

void CBKApp::PreLoadState()
{
// это разные попап менюшки
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_SCRPARAM_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_SCRPARAM);
	/*
	 а вот таким образом я тупо удаляю из реестра ветки с акселераторами и меню
	 перед загрузкой параметров из реестра. ибо не знаю как переписать функции LoadState и SaveState
	 так, чтобы они делали то, что мне нужно. А новые акселераторы из проги почему-то перебиваются старыми
	 из реестра, и меню, которое генерируется динамически, тоже перебивается старым меню из реестра.
	 Поэтому данные о них лучше нафиг поудалять.
	 Акселераторы и стандартный тулбар можно будет не трогать в финальной версии.
	*/
	::DeleteUnneededKeys();
}

void CBKApp::LoadCustomState()
{
	TRACE0("Вызов LoadCustomState.\n");
}

void CBKApp::SaveCustomState()
{
	TRACE0("Вызов SaveCustomState.\n");
}

BOOL CBKApp::PreTranslateMessage(MSG *pMsg)
{
	/* обработка сообщения, которым обмениваются между собой копии приложения
	    копия обрабатывает ответ от основной проги */

	// если это наше сообщение, и это ответ от основной проги
	if (pMsg->message == m_nInterAppGlobalMsg && pMsg->wParam == ANSWER_PRIME_HWND)
	{
		TRACE("copy thread -> MainWnd\n");
		// тут мы получили ответ от первой копии проги, получили хэндл окна
		// теперь наша задача, отправить параметры командной строки основной копии проги.
		//
		// Заполним поля структуры COPYDATA
		//
		TCHAR pInstanceCommandPrompt[1024] = { 0 }; // буфер для передачи командной строки
		CString strCommands = ::GetCommandLine(); // получим командную строку
		// и скопируем её в буфер
		memcpy(pInstanceCommandPrompt, strCommands.GetString(), min(strCommands.GetLength() * sizeof(TCHAR), sizeof(pInstanceCommandPrompt)));
		// можно сделать динамический массив нужного размера. Но и так сойдёт.
		COPYDATASTRUCT cd =
		{
			2,  // тип наших данных. Любое понравившееся число. По нему прога определяет, как надо интерпретировать передаваемые данные.
			sizeof(pInstanceCommandPrompt), // размер передаваемого буфера
			reinterpret_cast<PVOID>(pInstanceCommandPrompt)
		};
		// вызываем функцию, передающую данные в &cd основной копии проги
		::SendMessage(reinterpret_cast<HWND>(pMsg->lParam), WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cd));
		// дождёмся, когда на той стороне данные обработают
		PostThreadMessage(WM_QUIT, 0, 0); // всё, делать больше нечего. киляемся
		return TRUE;
	}

	return CWinAppEx::PreTranslateMessage(pMsg);
}


int CBKApp::Run()
{
	if (!m_bIsCopy) //если не копия
	{
		return CWinAppEx::Run(); // запускаем обычный рун
	}

	return CWinThread::Run(); //если копия - запускаем треадный рун, там не делается проверка на существование главного окна
}


// Диалоговое окно CAboutDlg используется для описания сведений о приложении
#include "AboutDlg.h"

class CAboutDlg : public CBaseDialog
{
	public:
		// Данные диалогового окна
		enum { IDD = IDD_ABOUTTABS };

		CAboutDlg();
		virtual ~CAboutDlg() override
		    = default;

	private:
		CMFCTabCtrl     m_ctrTab;
		CStatic         m_wndTabLoc;
		std::vector<CDialogEx *> m_vDlgs;
		void            AddTab(CDialogEx *pDlg, UINT nDlgID, UINT nTitleID);
		void            ReleaseTabs();

// Реализация
	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV
		virtual BOOL OnInitDialog() override;
		afx_msg void OnDestroy();
		DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CBaseDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange *pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ABOUTTAB, m_wndTabLoc);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CBaseDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
	CBaseDialog::OnInitDialog();
	SetDlgItemText(IDC_STATIC_EMULTITLE, theApp.m_strProgramTitleVersion);
	SetDlgItemText(IDC_STATIC_EMULCOMPILED, theApp.m_strCompileVersion);
	// тут колхозим табы
	CRect rectTab;
	m_wndTabLoc.GetWindowRect(&rectTab);
	ScreenToClient(&rectTab);
	m_ctrTab.Create(CMFCTabCtrl::STYLE_3D_ROUNDED, rectTab, this, IDC_ABOUTTAB,
	                CMFCTabCtrl::LOCATION_TOP);
	m_ctrTab.AutoDestroyWindow(FALSE);
	m_ctrTab.EnableTabSwap(TRUE);
//  m_ctrTab.EnableAutoColor(TRUE);
	m_ctrTab.SetActiveTabBoldFont(TRUE);
	auto pPage1 = new AboutDlg;
	AddTab(pPage1, AboutDlg::IDD, IDS_ABOUT_TABTITLE0); // Название первой вкладки
	auto pPage2 = new AboutLinks;
	AddTab(pPage2, AboutLinks::IDD, IDS_ABOUT_TABTITLE1);   // Название второй вкладки
	auto pPage3 = new AboutThx;
	AddTab(pPage3, AboutThx::IDD, IDS_ABOUT_TABTITLE2); // Название третьей вкладки
	m_ctrTab.RecalcLayout();
	m_ctrTab.RedrawWindow();
//  Суваем пеенге картиновку в шапку Эбаута, во всю широтень
	CPngImage m_png_logo;
	m_png_logo.Load(IDR_MAIN_PNG);
	auto pStatic = reinterpret_cast<CStatic *>(GetDlgItem(IDC_PICTURE_ABOUT));
	pStatic->ModifyStyle(0, BS_BITMAP);
	pStatic->SetBitmap(m_png_logo);
	CenterWindow(GetParent());
	return TRUE;  // return TRUE unless you set the focus to a control
	// Исключение: страница свойств OCX должна возвращать значение FALSE
}

void CAboutDlg::AddTab(CDialogEx *pDlg, UINT nDlgID, UINT nTitleID)
{
	m_vDlgs.push_back(pDlg);
	VERIFY(pDlg->Create(nDlgID, &m_ctrTab));
	m_ctrTab.AddTab(pDlg, nTitleID, (UINT) - 1, FALSE);
}

void CAboutDlg::ReleaseTabs()
{
	m_ctrTab.CleanUp();

	for (auto &p : m_vDlgs)
	{
		if (p)
		{
			p->DestroyWindow();
			delete p;
			p = nullptr;
		}
	}

	m_vDlgs.clear();
}

// Команда приложения для запуска диалога
void CBKApp::OnAppAbout()
{
	auto aboutDlg = std::make_unique<CAboutDlg>();
	aboutDlg->DoModal();
}


void CAboutDlg::OnDestroy()
{
	CBaseDialog::OnDestroy();
	ReleaseTabs();
}
#endif