#pragma once

#include "BKToken.h"
#include "GlobalStructs.h"

extern const wchar_t RADIX50[050];

namespace Parser
{
	bool ReadRegName(wchar_t &ch);
	bool ParseRegName(CBKToken *reg);
	bool ParseFPURegName(CBKToken *reg, bool bFirst);
	void SetAddresationRegister(int data);
	int  GetAddresationRegister();
	int  CheckReg(wchar_t &ch);
	bool AddRegSynonim(CBKToken *reg, CBKToken *synonim);
	bool Operand_analyse(int &cp, wchar_t &ch, bool bFPU);
	bool needChar(wchar_t nch, wchar_t &ch);

	bool parse_float(wchar_t &ch, int size, uint16_t *flt);
	bool HexNumberParser(wchar_t &ch, int &ret);
	bool DecNumberParser(wchar_t &ch, int &ret);
	bool OctNumberParser(wchar_t &ch, int &ret);
	bool BinNumberParser(wchar_t &ch, int &ret);
	bool AdvancedNumberParser(int &result, wchar_t &ch);
	bool Macro11NumberParser(int &result, wchar_t &ch);
}
