#include "pch.h"
#include "BK.h"
#include "variables.h"

/*
Идея подхватываемых инструментов из директории Tools, чтобы они в меню появлялись
Но в то же время, являлись бы и самостоятельными программами.

1. Сканируем директорию Tools, просматриваем все *.exe файлы.
2. Каждому найденному файлу надо послать сообщение с кодовым словом,
в ответ он должен выдать параметры: имя для меню и командную
строку, с которой его запускать.
Т.о. мы из всего набора файлов выделим нужные нам и под нашу платформу.
3. Сформировать меню.

Механизм сообщений требует запуска файла.
что не подходит.
Значит у файла должна быть экспортируемая функция, которую можно вызвать
извне как у dllки

В ресурсах инструмента должны быть созданы два строковых параметра
IDS_MODULE_MENU_NAME - имя инструмента, которое будет отображаться в меню
IDS_MODULE_MENU_START - параметры командной строки, если их нет - то просто
ресурс с пустой строкой.
*/
#include "Config.h"
#include "ToolManager.h"
#ifdef UI
TCHAR g_pToolsMenuData[MAX_MENU_ITEMS][_MAX_PATH];
int g_nToolBKDEMenuIdx = -1; //-1 - бкде не обнаружен

void FillTools(CMFCMenuBar *pwndMenuBar)
{
	TOOLS_PARAMS sTP;
	DWORD dwCode;
	bool bExists = false;
	constexpr int nInstrumentMenuPos = 6; // по идее 6 - номер меню "инструменты"
	// !!! Как вместо волшебного числа использовать нормальные методы, не знаю
	CMenu *pmenu = CMenu::FromHandle(pwndMenuBar->GetHMenu());
	CMenu *psmenu = pmenu->GetSubMenu(nInstrumentMenuPos);   // сюда надо добавлять пункты меню
	MENUITEMINFO mii;
	ZeroMemory(&mii, sizeof(MENUITEMINFO));
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_DATA | MIIM_ID;
	mii.fType = MFT_STRING;
	UINT nMenuPos = 0;
	fs::path strWildcard = g_Config.m_strToolsPath / _T("*.exe");
	// начинаем поиск
	CFileFind finder;
	BOOL bWorking = finder.FindFile(strWildcard.c_str());

	while (bWorking)
	{
		bWorking = finder.FindNextFile();

		// проигнорируем . и .. файлы; чтобы не зациклиться насмерть
		if (finder.IsDots())
		{
			continue;
		}

		// если это не директория, то обработаем
		if (!finder.IsDirectory())
		{
			CString strCurrentTool = finder.GetFilePath();
			HINSTANCE module = LoadLibrary(strCurrentTool);

			if (module)
			{
				auto _GetToolsParam = (GETTOOLSPARAM)GetProcAddress(module, "GetToolsParam");

				if (_GetToolsParam)
				{
					dwCode = CODE_PASSWORD; // для х64 код 0xda64
					ZeroMemory(&sTP, sizeof(TOOLS_PARAMS));
					DWORD dwResName, dwResStart;
					HRESULT result = _GetToolsParam(&dwCode, &dwResName, &dwResStart);

					if (SUCCEEDED(result))
					{
						if ((dwCode & 0xffffff00) == CODE_ANSVER) // для х64 ответ 0х64, это чтобы разделять между собой платформы
						{
							// данные из структуры применимы
							bExists = true;

							// поймаем конкретно BKDE, его путь будет использоваться для запуска
							// проги из меню дисков.
							if (dwCode == CODE_ANSVER_BKDE)
							{
								g_nToolBKDEMenuIdx = nMenuPos;
							}

							// теперь надо добавить в меню новый пункт меню
							::LoadString(module, dwResName, sTP.ToolName, _MAX_PATH);
							::LoadString(module, dwResStart, sTP.ToolStart, _MAX_PATH);
							CString str = strCurrentTool + _T(" ") + CString(sTP.ToolStart);
							_tcscpy_s(g_pToolsMenuData[nMenuPos], _MAX_PATH, str.GetString());
							mii.dwItemData = reinterpret_cast<ULONG_PTR>(&g_pToolsMenuData[nMenuPos]);
							mii.wID = ID_TOOL_MENU_0 + nMenuPos;
							mii.dwTypeData = sTP.ToolName;
							mii.cch = static_cast<UINT>(_tcslen(sTP.ToolName));
							psmenu->InsertMenuItem(nMenuPos++, &mii, TRUE);

							if (nMenuPos >= MAX_MENU_ITEMS)
							{
								break;
							}
						}
					}
				}

				FreeLibrary(module);
			}
		}

		// директории игнорируем
	}

	finder.Close();
	// эта фигня не работает принципиально, к сожалению.
	pmenu->EnableMenuItem(nInstrumentMenuPos, MF_BYPOSITION | (bExists ? MF_ENABLED : MF_GRAYED | MF_DISABLED));
}

// *************************************************************
//
//  RegDelnodeRecurse()
//
//  Purpose:    Deletes a registry key and all its subkeys / values.
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
//
//  Return:     true if successful.
//              false if an error occurs.
//
// *************************************************************

bool RegDelnodeRecurse(HKEY hKeyRoot, CString &lpSubKey)
{
	TCHAR szName[_MAX_PATH];
	HKEY hKey;
	FILETIME ftWrite;
	// First, see if we can delete the key without having
	// to recurse.
	LSTATUS lResult = RegDeleteKey(hKeyRoot, lpSubKey.GetString());

	if (lResult == ERROR_SUCCESS)
	{
		return true;
	}

	lResult = RegOpenKeyEx(hKeyRoot, lpSubKey.GetString(), 0, KEY_READ, &hKey);

	if (lResult != ERROR_SUCCESS)
	{
		if (lResult == ERROR_FILE_NOT_FOUND)
		{
			// printf("Key not found.\n");
			return true;
		}

		// printf("Error opening key.\n");
		return false;
	}

	// Check for an ending slash and add one if it is missing.
	lpSubKey.TrimRight(_T('\\'));
	lpSubKey += _T('\\');
	// Enumerate the keys
	DWORD dwSize = _MAX_PATH;
	lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, nullptr,
	                       nullptr, nullptr, &ftWrite);

	if (lResult == ERROR_SUCCESS)
	{
		do
		{
			CString str = lpSubKey + szName;

			if (!RegDelnodeRecurse(hKeyRoot, str))
			{
				break;
			}

			dwSize = _MAX_PATH;
			lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, nullptr,
			                       nullptr, nullptr, &ftWrite);
		}
		while (lResult == ERROR_SUCCESS);
	}

	lpSubKey.TrimRight(_T('\\'));
	RegCloseKey(hKey);
	// Try again to delete the key.
	lResult = RegDeleteKey(hKeyRoot, lpSubKey.GetString());

	if (lResult == ERROR_SUCCESS)
	{
		return true;
	}

	return false;
}

// *************************************************************
//
//  RegDelnode()
//
//  Purpose:    Deletes a registry key and all its subkeys / values.
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
//
//  Return:     true if successful.
//              false if an error occurs.
//
// *************************************************************

bool RegDelnode(HKEY hKeyRoot, CString &lpSubKey)
{
	return RegDelnodeRecurse(hKeyRoot, lpSubKey);
}

void DeleteUnneededKeys()
{
	// всякий раз, после того как были изменены тулбары и меню, надо будет удалять из реестра
	// старые данные, чтобы менюшки брались новые, а не из реестра.
	// тут надо будет поменять значение константы.
	// DESTRUCT_VAL определяется в файле BK\variables.h
	auto hkeyRegKey = (HKEY)INVALID_HANDLE_VALUE;
	bool bDestructActions = false;
	CString strReg;
	// удаляем параметры меню,
	// HKEY_CURRENT_USER\Software\{theApp.m_pszRegistryKey}\{theApp.m_pszProfileName}\Workspace\MFCToolBar-593980
	// а заодно и акселераторы (\Keyboard-0)
	CString strRegPath = _T("Software\\") + CString(theApp.m_pszRegistryKey) + _T("\\") +
	                     CString(theApp.m_pszProfileName) + _T("\\") + CString(theApp.GetRegistryBase());
	// открываем ключ
	LSTATUS lResult = RegOpenKeyEx(HKEY_CURRENT_USER, strRegPath, 0, KEY_ALL_ACCESS, &hkeyRegKey);

	if (lResult == ERROR_SUCCESS)
	{
		// читаем значение
		DWORD dwType;
		uint8_t pBuffer[sizeof(DWORD)] = { 0, };
		auto pdwBuffer = reinterpret_cast<DWORD *>(&pBuffer[0]);
		DWORD nBufLen = sizeof(DWORD);
		lResult = RegQueryValueEx(hkeyRegKey, _T("AppCode"), nullptr, &dwType, pBuffer, &nBufLen);

		if (lResult == ERROR_SUCCESS)
		{
			// если прочиталось
			if (dwType == REG_DWORD)
			{
				// если тип DWORD
				if (*pdwBuffer != DESTRUCT_VAL)
				{
					// если значение не совпадает с контрольным
					bDestructActions = true;
				}
			}
			else
			{
				// если не DWORD - удалим такой ключ, чтобы создать правильный
				lResult = RegDeleteValue(hkeyRegKey, _T("AppCode"));
				bDestructActions = true;
			}
		}
		else
		{
			// если нет такого ключа
			bDestructActions = true;
		}

		if (bDestructActions)
		{
			*pdwBuffer = DESTRUCT_VAL;
			lResult = RegSetValueEx(hkeyRegKey, _T("AppCode"), 0, REG_DWORD, pBuffer, sizeof(DWORD));
		}

		RegCloseKey(hkeyRegKey);
	}

	// удаляем акселераторы (необязательно, но я задолбался уже, меняешь в проге акселератор, а он не
	// применяется, т.к. берётся старый из реестра)
	if (bDestructActions)
	{
		strReg = strRegPath + _T("\\Keyboard-0");
		RegDelnode(HKEY_CURRENT_USER, strReg);
	}

	// удаляем главное меню, т.к. часть его формируется динамически, и опять таки новое не применяется
	// потому что старое в реестре перебивает новое
	CString strKey;
	strKey.Format(_T("\\MFCToolBar-%d"), AFX_IDW_MENUBAR * 10);
	strReg = strRegPath + strKey;
	RegDelnode(HKEY_CURRENT_USER, strReg);
	// удаляем Стандартный тулбар (т.к. при подмене кнопок они не видны, т.к. перебиваются старыми из реестра)
	strKey.Format(_T("\\MFCToolBar-%d"), AFX_IDW_TOOLBAR);
	strReg = strRegPath + strKey;
	RegDelnode(HKEY_CURRENT_USER, strReg);
	// и обязательно удаляем ключ OrigResetItems из звукового тулбара, иначе не будет устанавливаться
	// начальная позиция слайдера громкости, т.к. в реестре сохранены старые данные.
	static const UINT ToolbarIDs[3] = { IDW_SOUND_TOOLBAR, IDW_DEBUG_TOOLBAR, IDW_VCAPT_TOOLBAR };

	for (auto ToolbarID : ToolbarIDs)
	{
		strKey.Format(_T("\\MFCToolBar-%d"), ToolbarID);
		strReg = strRegPath + strKey;

		if (bDestructActions)
		{
			// удаляем весь ключ
			RegDelnode(HKEY_CURRENT_USER, strReg);
		}
		else
		{
			lResult = RegOpenKeyEx(HKEY_CURRENT_USER, strReg, 0, KEY_ALL_ACCESS, &hkeyRegKey);

			if (lResult == ERROR_SUCCESS)
			{
				RegDeleteValue(hkeyRegKey, _T("OrigResetItems"));
				RegDeleteValue(hkeyRegKey, _T("OriginalItems"));
				RegCloseKey(hkeyRegKey);
			}
		}
	}
}

void LaunchTool(UINT id)
{
	// запускаем выбранный инструмент
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (CreateProcess(nullptr,     // No module name (use command line)
	                  g_pToolsMenuData[id],   // Command line
	                  nullptr,                // Process handle not inheritable
	                  nullptr,                // Thread handle not inheritable
	                  FALSE,                  // Set handle inheritance to FALSE
	                  NORMAL_PRIORITY_CLASS,  // No creation flags
	                  nullptr,                // Use parent's environment block
	                  nullptr,                // Use parent's starting directory
	                  &si,                    // Pointer to STARTUPINFO structure
	                  &pi))                   // Pointer to PROCESS_INFORMATION structure
	{
		// Wait until child process exits.
		// WaitForSingleObject( pi.hProcess, INFINITE );
		// Close process and thread handles.
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}

#endif