#pragma once

#define __AFXWIN_H__
#define TRACE0(...)
#define TRACE1(...)
#define TRACE(...)

#include <winnt.h>
#include <CString.h>
#include <CFile.h>
#include <CArray.h>

/*============================================================================*/
// STDIO file implementation

class CStdioFile : public CFile
{
};
