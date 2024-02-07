#pragma once

#ifndef logMsg
   
#ifdef MNGR_DEBUG
extern void logMsg(char* msg);
#define printf(...) { char tmp[80]; snprintf(tmp, 80, __VA_ARGS__); logMsg(tmp); }
#define DBGM_PRINT( X) printf X
#else
#define DBGM_PRINT( X)
#endif

#endif

#ifdef __cplusplus

#include "BKMessageBox.h"
#define TRACE_T(...) { char tmp[80]; snprintf(tmp, 80, __VA_ARGS__); g_BKMsgBox.Show(CString(tmp), 0); }

#endif
