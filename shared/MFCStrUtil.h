#pragma once
// эти функции только для проектов, написанных под MFC
namespace MFCStrUtil
{
	// набор функций для манипуляции с именем файла
	CString         GetCurrentPath();
	CString        &NormalizePath(CString &strPath);
	CString         GetFileDrive(const CString &strFile);
	CString         GetFilePath(const CString &strFile);
	CString         GetFileTitle(const CString &strFile);
	CString         GetFileName(const CString &strFile);
	CString         GetFileExt(const CString &strFile);
	CString         Str2WideStr(const char *srcStr, int len = -1);
}
