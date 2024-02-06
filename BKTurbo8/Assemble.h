#pragma once

#include "BKToken.h"
#include "GlobalStructs.h"

namespace Assemble
{
	using funcRef = bool (*)(int &, wchar_t &);

	struct PseudoCommandStruct
	{
		std::wstring    strName;
		bool            bEvenAddr;
		bool            bInScript;
		funcRef         PSCFunction;
	};


	bool AssembleCPUInstruction(CBKToken *token, int &cp, wchar_t &ch);
	bool assemble2OP(int &cp, wchar_t &ch);
	bool assemble1OP(int &cp, wchar_t &ch);
	bool assemble1OPFPU(int &cp, wchar_t &ch);
	bool assemble1OPR(int &cp, wchar_t &ch);
	bool assemble1OPRFPU(int &cp, wchar_t &ch);
	bool assemble2OPR(int &cp, wchar_t &ch);
	bool assemble2OPRFPU(int &cp, wchar_t &ch, bool bFPU);
	bool assemble2ROP(int &cp, wchar_t &ch);
	bool assembleSOB(int &cp, wchar_t &ch);
	bool assembleTRAP(int &cp, wchar_t &ch);
	bool assembleMARK(int &cp, wchar_t &ch);
	bool assembleBR(int &cp, wchar_t &ch);

	bool PseudoCommandExec(CBKToken *token, int &cp, wchar_t &ch);
	bool StrQuotes(wchar_t ch);
	bool PSC_addr(int &cp, wchar_t &ch);
	bool PSC_la(int &cp, wchar_t &ch);
	bool PSC_print(int &cp, wchar_t &ch);
	bool PSC_blkw(int &cp, wchar_t &ch);
	bool PSC_blkb(int &cp, wchar_t &ch);
	bool PSC_word(int &cp, wchar_t &ch);
	bool PSC_byte(int &cp, wchar_t &ch);
	bool PSC_end(int &cp, wchar_t &ch);
	bool PSC_even(int &cp, wchar_t &ch);
	bool PSC_rad50(int &cp, wchar_t &ch);
	bool PSC_ascii(int &cp, wchar_t &ch);
	bool PSC_asciz(int &cp, wchar_t &ch);
	bool PSC_org(int &cp, wchar_t &ch);
	bool PSC_flt2(int &cp, wchar_t &ch);
	bool PSC_flt4(int &cp, wchar_t &ch);
	bool PSC_Include(int &cp, wchar_t &ch);
	bool GetName(std::wstring &strName, wchar_t &ch);
	bool PSC_Script(int &cp, wchar_t &ch);
	bool PSC_EndScript(int &cp, wchar_t &ch);
	bool CheckComment(wchar_t &ch);
	bool BranchVerify(int nCommandAddr, int &nTargetAddr);
	bool PSC_enabl(int &cp, wchar_t &ch);
	bool PSC_dsabl(int &cp, wchar_t &ch);
}
