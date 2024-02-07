#pragma once

#include "targetver.h"
#include <cstdio>
#include <tchar.h>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // некоторые конструкторы CString будут явными
#define _AFX_NO_MFC_CONTROLS_IN_DIALOGS         // удалить поддержку элементов управления MFC в диалоговых окнах

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Исключите редко используемые компоненты из заголовков Windows
#endif

#include <afx.h>
#include <afxwin.h>         // основные и стандартные компоненты MFC
#include <afxext.h>         // расширения MFC
// #ifndef _AFX_NO_OLE_SUPPORT
// #include <afxdtctl.h>           // поддержка MFC для общих элементов управления Internet Explorer 4
// #endif
// #ifndef _AFX_NO_AFXCMN_SUPPORT
// #include <afxcmn.h>                     // поддержка MFC для общих элементов управления Windows
// #endif // _AFX_NO_AFXCMN_SUPPORT

#include <iostream>
#include <memory>
#include <fs.h>
