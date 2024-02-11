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
#include <hardware/pio.h>
#define TRACE_T(...) { gpio_put(PICO_DEFAULT_LED_PIN, true); char tmp[128]; snprintf(tmp, 128, __VA_ARGS__); g_BKMsgBox.Show(CString(tmp), 0); gpio_put(PICO_DEFAULT_LED_PIN, false); }

#endif
