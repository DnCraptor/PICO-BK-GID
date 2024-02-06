#pragma once
#ifdef TOOL_MANAGER
#ifdef _WIN64
constexpr auto CODE_PASSWORD = 0x6400da;
constexpr auto CODE_ANSVER = 0x64be00;
constexpr auto CODE_ANSVER_BKDE = 0x64bede;
#else
constexpr auto CODE_PASSWORD = 0x8600da;
constexpr auto CODE_ANSVER = 0x86be00;
constexpr auto CODE_ANSVER_BKDE = 0x86bede;
#endif

struct TOOLS_PARAMS
{
	TCHAR ToolName[_MAX_PATH];
	TCHAR ToolStart[_MAX_PATH];
};

constexpr auto MAX_MENU_ITEMS = 10;

extern TCHAR g_pToolsMenuData[MAX_MENU_ITEMS][_MAX_PATH];
extern int g_nToolBKDEMenuIdx;
using GETTOOLSPARAM = HRESULT(*)(DWORD *, DWORD *, DWORD *);

// код для x86: 0xda86, для x64: 0xda64
// ответ для x86: 0x86, для x64: 0x64
#define GETTOOLSPARAM_CODE(password, ansver)                            \
	extern "C" __declspec(dllexport) HRESULT GetToolsParam(DWORD *dwCode, DWORD *dwResName, DWORD *dwResCmd)    \
	{                                                                       \
		if (*dwCode == password)                                            \
		{                                                                   \
			*dwCode = ansver;                                               \
			*dwResName = IDS_MODULE_MENU_NAME;                              \
			*dwResCmd = IDS_MODULE_MENU_START;                              \
			return S_OK;                                                    \
		}                                                                   \
		return -1;                                                          \
	}

void FillTools(CMFCMenuBar *pwndMenuBar);

#define TOOL_BKDE_INITIATED 0x1001
#define TOOL_BKDE_TERMINATED 0x1002
#define TOOL_BKDE_CLOSED 0x1003
#define TOOL_IDDIMGMAKER_INITIATED 0x1101
#define TOOL_IDDIMGMAKER_TERMINATED 0x1102

// глобальное сообщение для обмена между эмулятором и утилитами
inline UINT RegisterGlobalMessage()
{
	return RegisterWindowMessage(_T("Inter BKEmu Tool Signal Global Message"));
}


/*
В принципе работает, но из-за того, что в реестре сохраняются параметры меню,
изменения не будут видны, пока не стереть из реестра старые параметры
HKEY_CURRENT_USER\Software\gid prod.\{theApp.m_pszProfileName}\Workspace\MFCToolBar-593980
*/
void DeleteUnneededKeys();
bool RegDelnode(HKEY hKeyRoot, CString &lpSubKey);
bool RegDelnodeRecurse(HKEY hKeyRoot, CString &lpSubKey);
void LaunchTool(UINT id);
#endif