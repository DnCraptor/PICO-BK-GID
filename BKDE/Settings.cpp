#include "pch.h"
#include "Settings.h"

/*
Код стыренный из BKBTL. Потому что лень самому изобретать.
*/

const LPCTSTR CSettings::IMAGE_PATH = _T("ImagesPath");
const LPCTSTR CSettings::STORE_PATH = _T("StoresPath");
const LPCTSTR CSettings::LOAD_PATH = _T("LoadPath");
const LPCTSTR CSettings::USE_BIN_FORMAT = _T("UseBin");
const LPCTSTR CSettings::USE_LONGBIN = _T("UseLongBin");
const LPCTSTR CSettings::USE_LOG_EXTRACT = _T("UseAnalyseExtract");

CSettings::CSettings()
{
	const CWinApp *pWinApp = AfxGetApp();
	m_hkeyRegKey = HKEY(INVALID_HANDLE_VALUE);
	m_strRegKeyName = _T("Software\\") + CString(pWinApp->m_pszRegistryKey) + _T("\\") +
	                  CString(pWinApp->m_pszProfileName);
}

CSettings::~CSettings()
    = default;

void CSettings::Init()
{
	LSTATUS lResult = ::RegOpenKeyEx(HKEY_CURRENT_USER, m_strRegKeyName, 0, KEY_ALL_ACCESS, &m_hkeyRegKey);

	if (lResult != ERROR_SUCCESS)
	{
		lResult = ::RegCreateKeyEx(HKEY_CURRENT_USER, m_strRegKeyName, 0, nullptr, 0, KEY_ALL_ACCESS,
		                           nullptr, &m_hkeyRegKey, nullptr);
	}
}

void CSettings::Done()
{
	::RegCloseKey(m_hkeyRegKey);
}

bool CSettings::SaveStringValue(LPCTSTR sName, CString &strValue)
{
	LSTATUS lResult;
	lResult = ::RegSetValueEx(m_hkeyRegKey, sName, 0, REG_SZ, reinterpret_cast<const BYTE *>(strValue.GetString()),
	                          (strValue.GetLength() + 1) * sizeof(TCHAR));
	return (lResult == ERROR_SUCCESS);
}

bool CSettings::SaveStringValue(LPCTSTR sName, fs::path &strValue)
{
	LSTATUS lResult;
	lResult = ::RegSetValueEx(m_hkeyRegKey, sName, 0, REG_SZ, reinterpret_cast<const BYTE *>(strValue.c_str()),
	                          (strValue.wstring().size() + 1) * sizeof(TCHAR));
	return (lResult == ERROR_SUCCESS);
}

bool CSettings::LoadStringValue(LPCTSTR sName, CString &strValue)
{
	TCHAR pBuffer[_MAX_PATH];
	DWORD dwType;
	DWORD dwBufLength = _MAX_PATH * sizeof(TCHAR);
	memset(pBuffer, 0, dwBufLength);
	LSTATUS lResult = ::RegQueryValueEx(m_hkeyRegKey, sName, nullptr, &dwType, reinterpret_cast<LPBYTE>(pBuffer), &dwBufLength);

	if (lResult == ERROR_SUCCESS && dwType == REG_SZ)
	{
		strValue = CString(pBuffer);
		return true;
	}

	strValue = _T("");
	return false;
}

bool CSettings::LoadStringValue(LPCTSTR sName, fs::path &strValue)
{
	TCHAR pBuffer[_MAX_PATH];
	DWORD dwType;
	DWORD dwBufLength = _MAX_PATH * sizeof(TCHAR);
	memset(pBuffer, 0, dwBufLength);
	LSTATUS lResult = ::RegQueryValueEx(m_hkeyRegKey, sName, nullptr, &dwType, reinterpret_cast<LPBYTE>(pBuffer), &dwBufLength);

	if (lResult == ERROR_SUCCESS && dwType == REG_SZ)
	{
		strValue = fs::path(pBuffer);
		return true;
	}

	strValue.clear();
	return false;
}
