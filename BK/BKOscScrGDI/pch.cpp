// pch.cpp: файл исходного кода, соответствующий предварительно скомпилированному заголовочному файлу

#include "pch.h"

// При использовании предварительно скомпилированных заголовочных файлов необходим следующий файл исходного кода для выполнения сборки.

// Update Manifest
// cf: http://blogs.msdn.com/b/oldnewthing/archive/2007/05/31/2995284.aspx
//
// We use features from GDI+ v1.1 which is new as of Windows Vista. There is no redistributable for Windows XP.
// This adds information to the .exe manifest to force GDI+ 1.1 version of gdiplus.dll to be loaded on Vista
// without this, Vista defaults to loading version 1.0 and our application will fail to launch with missing entry points.
#if _WIN64
#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.GdiPlus' version='1.1.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.GdiPlus' version='1.1.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif