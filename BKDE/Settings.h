#pragma once
/*
Код стыренный из BKBTL. Потому что лень самому изобретать.
*/

class CSettings
{
		HKEY				m_hkeyRegKey;
		CString				m_strRegKeyName;

	public:

		static const LPCTSTR IMAGE_PATH;
		static const LPCTSTR STORE_PATH;
		static const LPCTSTR LOAD_PATH;
		static const LPCTSTR USE_BIN_FORMAT;
		static const LPCTSTR USE_LONGBIN;
		static const LPCTSTR USE_LOG_EXTRACT;

		CSettings();
		~CSettings();
		void Init();
		void Done();
		bool SaveStringValue(LPCTSTR sName, CString &strValue);
		bool LoadStringValue(LPCTSTR sName, CString &strValue);

		bool SaveStringValue(LPCTSTR sName, fs::path &strValue);
		bool LoadStringValue(LPCTSTR sName, fs::path &strValue);
};


