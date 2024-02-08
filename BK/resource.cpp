#include "resource.h"
#include "CString.h"

BOOL CString::LoadString(int res) {
    switch (res) {
       case IDS_INI_BK10_RE2_108_BASIC3 :   s = "BK10 Basic3"; break;
       case IDS_INI_BK11_RE_OPT_PG12_1 :    s = "BK11 Optional Page12.1"; break;
       case IDS_INI_BK11_RE_OPT_PG12_2 :    s = "BK11 Optional Page12.2"; break;
       case IDS_INI_BK11_RE_OPT_PG13_1 :    s = "BK11 Optional Page13.1"; break;
       case IDS_INI_BK11_RE_OPT_PG13_2 :    s = "BK11 Optional Page13.2"; break;
       case IDS_INI_BK11_RE2_198_BASIC1 :   s = "BK11 Basic1"; break;
       case IDS_INI_BK11_RE2_199_BASIC2 :   s = "BK11 Basic2"; break;
       case IDS_INI_BK11_RE2_200_BASIC3 :   s = "BK11 Basic3"; break;
       case IDS_INI_BK11_RE2_201_BOS :      s = "BK11 Monitor BOS"; break;
       case IDS_INI_BK11_RE2_202_EXT :      s = "BK11 Monitor EXT"; break;
       case IDS_INI_BK11_RE2_203_MSTD :     s = "BK11 MSTD"; break;
       case IDS_INI_BK11M_RE2_324_BOS :     s = "BK11M Monitor BOS"; break;
       case IDS_INI_BK11M_RE2_325_EXT :     s = "BK11M Monitor EXT"; break;
       case IDS_INI_BK11M_RE2_327_BASIC1 :  s = "BK11M Basic1"; break;
       case IDS_INI_BK11M_RE2_328_BASIC2 :  s = "BK11M Basic2"; break;
       case IDS_INI_BK11M_RE2_329_BASIC3 :  s = "BK11M Basic3"; break;
       case IDS_FILEEXT_BINARY  :           s = ".bin"; break;
       case IDS_FILEEXT_MEMORYSTATE :       s = ".msf"; break;
       case IDS_FILEEXT_ROM :               s = ".rom"; break;
       case IDS_FILEEXT_SCRIPT :            s = ".bkscript"; break;
       case IDS_FILEEXT_TAPE  :             s = ".tap"; break;
       case IDS_FILEEXT_WAVE  :             s = ".wav"; break;
       default: s.clear();  return false;
    }
    return true;
}
