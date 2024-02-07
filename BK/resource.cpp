#include "resource.h"
#include "CString.h"

BOOL CString::LoadString(int res) {
    if (m_pszData) {
        delete m_pszData;
        m_pszData = 0;
    }
    switch (res) {
       case IDS_INI_BK10_RE2_108_BASIC3 :   *this += "BK10 Basic3"; break;
       case IDS_INI_BK11_RE_OPT_PG12_1 :    *this += "BK11 Optional Page12.1"; break;
       case IDS_INI_BK11_RE_OPT_PG12_2 :    *this += "BK11 Optional Page12.2"; break;
       case IDS_INI_BK11_RE_OPT_PG13_1 :    *this += "BK11 Optional Page13.1"; break;
       case IDS_INI_BK11_RE_OPT_PG13_2 :    *this += "BK11 Optional Page13.2"; break;
       case IDS_INI_BK11_RE2_198_BASIC1 :   *this += "BK11 Basic1"; break;
       case IDS_INI_BK11_RE2_199_BASIC2 :   *this += "BK11 Basic2"; break;
       case IDS_INI_BK11_RE2_200_BASIC3 :   *this += "BK11 Basic3"; break;
       case IDS_INI_BK11_RE2_201_BOS :      *this += "BK11 Monitor BOS"; break;
       case IDS_INI_BK11_RE2_202_EXT :      *this += "BK11 Monitor EXT"; break;
       case IDS_INI_BK11_RE2_203_MSTD :     *this += "BK11 MSTD"; break;
       case IDS_INI_BK11M_RE2_324_BOS :     *this += "BK11M Monitor BOS"; break;
       case IDS_INI_BK11M_RE2_325_EXT :     *this += "BK11M Monitor EXT"; break;
       case IDS_INI_BK11M_RE2_327_BASIC1 :  *this += "BK11M Basic1"; break;
       case IDS_INI_BK11M_RE2_328_BASIC2 :  *this += "BK11M Basic2"; break;
       case IDS_INI_BK11M_RE2_329_BASIC3 :  *this += "BK11M Basic3"; break;
       case IDS_FILEEXT_BINARY  :           *this += ".bin"; break;
       case IDS_FILEEXT_MEMORYSTATE :       *this += ".msf"; break;
       case IDS_FILEEXT_ROM :               *this += ".rom"; break;
       case IDS_FILEEXT_SCRIPT :            *this += ".bkscript"; break;
       case IDS_FILEEXT_TAPE  :             *this += ".tap"; break;
       case IDS_FILEEXT_WAVE  :             *this += ".wav"; break;
       default:
           return false;
    }
    return true;
}
