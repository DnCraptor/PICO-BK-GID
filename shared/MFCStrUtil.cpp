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
	CString str("\\");
	return str;
}

CString MFCStrUtil::GetFileDrive(const CString &strFile)
{
	CString str("\\");
	return str;
}

CString MFCStrUtil::GetFilePath(const CString &strFile)
{
	return strFile;
}

CString MFCStrUtil::GetFileTitle(const CString &strFile)
{
	return strFile;
}

CString MFCStrUtil::GetFileName(const CString &strFile)
{
	int i = 0;
    const char* p = strFile.GetString();
	while(*p) {
		if (*p == '\\') i = p - strFile.GetString();
	}
	CString str(strFile.GetString() + i + 1);
	return str;
}

CString MFCStrUtil::GetFileExt(const CString &strFile)
{
	int i = 0;
    const char* p = strFile.GetString();
	while(*p) {
		if (*p == '.') i = p - strFile.GetString();
	}
	if (!i) return "";
	CString str(strFile.GetString() + i + 1);
	return str;
}

CString MFCStrUtil::Str2WideStr(const char *srcStr, int len)
{
	return srcStr;
}
