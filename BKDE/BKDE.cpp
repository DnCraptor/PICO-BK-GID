
// BKDE.cpp : Определяет поведение классов для приложения.
//

#include "pch.h"
#include "BKDE.h"
#include "BKDEDlg.h"
#include <clocale>
#include "..\shared\ToolManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

GETTOOLSPARAM_CODE(CODE_PASSWORD, CODE_ANSVER_BKDE);

// CBKDEApp

BEGIN_MESSAGE_MAP(CBKDEApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// создание CBKDEApp

CBKDEApp::CBKDEApp()
{
	// TODO: добавьте код создания,
	// Размещает весь важный код инициализации в InitInstance
}


// Единственный объект CBKDEApp

CBKDEApp theApp;


// инициализация CBKDEApp

BOOL CBKDEApp::InitInstance()
{
	_tsetlocale(LC_ALL, _T("Russian"));
//  m_nInterAppToolGlobalMessage = ::RegisterGlobalMessage();
	// InitCommonControlsEx() требуется для Windows XP, если манифест
	// приложения использует ComCtl32.dll версии 6 или более поздней версии для включения
	// стилей отображения.  В противном случае будет возникать сбой при создании любого окна.
	INITCOMMONCONTROLSEX InitCtrls{};
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Выберите этот параметр для включения всех общих классов управления, которые необходимо использовать
	// в вашем приложении.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);
	CWinApp::InitInstance();

	// Инициализация библиотек OLE
	if (!AfxOleInit())
	{
		AfxMessageBox(_T("AfxOleInit Error!"));
		return FALSE;
	}

	AfxEnableControlContainer();
	// Создать диспетчер оболочки, в случае, если диалоговое окно содержит
	// представление дерева оболочки или какие-либо его элементы управления.
	auto pShellManager = new CShellManager;
	// Активация визуального диспетчера "Классический Windows" для включения элементов управления MFC
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
	// Стандартная инициализация
	// Если эти возможности не используются и необходимо уменьшить размер
	// конечного исполняемого файла, необходимо удалить из следующих
	// конкретных процедур инициализации, которые не требуются
	// Измените раздел реестра, в котором хранятся параметры
	// следует изменить эту строку на что-нибудь подходящее,
	// например на название организации
	SetRegistryKey(_T("gid prod."));
	// подменим профиль, чтобы не плодить кучу разных настроек в реестре
	free((void *)m_pszProfileName); // освободим память выделенную под прошлое имя
	m_pszProfileName = _tcsdup(_T("BKDiskExplorer")); // зададим своё
	// AfxMessageBox(theApp.m_lpCmdLine, MB_OK, 0); // когда кидаем файл на ярлык, передача происходит через командную строку.
//  ::PostMessage(HWND_BROADCAST, m_nInterAppToolGlobalMessage, TOOL_BKDE_INITIATED, 0);
	CBKDEDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();

	if (nResponse == IDOK)
	{
		// TODO: Введите код для обработки закрытия диалогового окна
		//  с помощью кнопки "ОК"
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Введите код для обработки закрытия диалогового окна
		//  с помощью кнопки "Отмена"
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Предупреждение. Не удалось создать диалоговое окно, поэтому работа приложения неожиданно завершена.\n");
		TRACE(traceAppMsg, 0, "Предупреждение. При использовании элементов управления MFC для диалогового окна невозможно #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}

	// Удалить диспетчер оболочки, созданный выше.
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#ifndef _AFXDLL
	ControlBarCleanUp();
#endif
//  ::PostMessage(HWND_BROADCAST, m_nInterAppToolGlobalMessage, TOOL_BKDE_CLOSED, 0);
	// Поскольку диалоговое окно закрыто, возвратите значение FALSE, чтобы можно было выйти из
	//  приложения вместо запуска генератора сообщений приложения.
	return FALSE;
}

// int CBKDEApp::ExitInstance()
// {
//  ::PostMessage(HWND_BROADCAST, theApp.m_nInterAppToolGlobalMessage, TOOL_BKDE_TERMINATED, 0);
//  return CWinApp::ExitInstance();
// }

