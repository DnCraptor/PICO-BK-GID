#include "pch.h"
#include "MFCStrUtil.h"
// эти функции только для проектов, написанных под MFC

CString &MFCStrUtil::NormalizePath(CString &strPath)
{
	if (!strPath.IsEmpty())
	{
		strPath.TrimRight(_T('\\'));
		strPath += _T('\\');
	}

	return strPath;
}

CString MFCStrUtil::GetCurrentPath()
{
	CString strDrive;
	CString strPath;
	CString pModuleName;
	BOOL res = ::GetModuleFileName(AfxGetInstanceHandle(), pModuleName.GetBufferSetLength(_MAX_PATH), _MAX_PATH);
	pModuleName.ReleaseBuffer();

	if (res)
	{
		return GetFilePath(pModuleName);
	}

	return _T("");
}

CString MFCStrUtil::GetFileDrive(const CString &strFile)
{
	// если путь начинается с двух слешей, то это UNC
	if (strFile.GetAt(0) == _T('\\') && strFile.GetAt(1) == _T('\\'))
	{
		// ну и возвратим эти два слеша, чтобы было понятно, что это не отночительный путь
		return _T("\\\\");
	}

	CString strDrive;
	_tsplitpath_s(strFile.GetString(), strDrive.GetBufferSetLength(_MAX_DIR), _MAX_DIR, nullptr, 0, nullptr, 0, nullptr, 0);
	strDrive.ReleaseBuffer();
	return strDrive.Trim();
}

CString MFCStrUtil::GetFilePath(const CString &strFile)
{
	CString strDrive;
	CString strPath;
	_tsplitpath_s(strFile.GetString(), strDrive.GetBufferSetLength(_MAX_DIR), _MAX_DIR, strPath.GetBufferSetLength(_MAX_PATH), _MAX_PATH, nullptr, 0, nullptr, 0);
	strDrive.ReleaseBuffer();
	strPath.ReleaseBuffer();
	CString path = strDrive.Trim() + strPath.Trim();
	return NormalizePath(path);
}

CString MFCStrUtil::GetFileTitle(const CString &strFile)
{
	CString strName;
	_tsplitpath_s(strFile.GetString(), nullptr, 0, nullptr, 0, strName.GetBufferSetLength(_MAX_FNAME), _MAX_FNAME, nullptr, 0);
	strName.ReleaseBuffer();
	return strName.Trim();
}

CString MFCStrUtil::GetFileName(const CString &strFile)
{
	CString strName;
	CString strExt;
	_tsplitpath_s(strFile.GetString(), nullptr, 0, nullptr, 0, strName.GetBufferSetLength(_MAX_FNAME), _MAX_FNAME, strExt.GetBufferSetLength(_MAX_EXT), _MAX_EXT);
	strName.ReleaseBuffer();
	strExt.ReleaseBuffer();
	return strName.Trim() + strExt.Trim();
}

CString MFCStrUtil::GetFileExt(const CString &strFile)
{
	CString strExt;
	_tsplitpath_s(strFile.GetString(), nullptr, 0, nullptr, 0, nullptr, 0, strExt.GetBufferSetLength(_MAX_EXT), _MAX_EXT);
	strExt.ReleaseBuffer();
	return strExt.Trim();
}

CString MFCStrUtil::Str2WideStr(const char *srcStr, int len)
{
	int nChars = MultiByteToWideChar(CP_ACP, 0, srcStr, len, nullptr, 0);
	auto pwcsName = std::make_unique<WCHAR[]>(nChars);
	MultiByteToWideChar(CP_ACP, 0, srcStr, len, (LPWSTR)pwcsName.get(), nChars);
	return {pwcsName.get()};
}
