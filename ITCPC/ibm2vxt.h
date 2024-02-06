#pragma once
#include "BKFile.h"
#include "WinFile.h"
#include "BKASCFile.h"
#include "BaseFile.h"

namespace ITCPC
{
	uint8_t VXT_Subr(CBaseFile *pInFile, unsigned int &dwFlags, int &nOtstup, TCHAR &wchPrSym);
	void IBM2VXT(CBaseFile *pInFile, CBaseFile *pOutFile);
}
