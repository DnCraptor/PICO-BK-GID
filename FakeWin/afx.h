#pragma once

#define __AFXWIN_H__
#define TRACE0(X)
#define TRACE1(X, Y)
#define TRACE(X, Y, Z)

#include <winnt.h>
#include <CString.h>
#include <CFile.h>
#include <CArray.h>

/*============================================================================*/
// STDIO file implementation

class CStdioFile : public CFile
{
};
