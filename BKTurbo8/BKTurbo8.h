#pragma once

enum class OP_MODE
{
	UNKN,
	LINK,
	COMP,
	CL
};

constexpr uint32_t BKT_FLAG_NONE        = 0;
constexpr uint32_t BKT_FLAG_MAKELISTING = (1 << 0);
constexpr uint32_t BKT_FLAG_MAKEOBJECT  = (1 << 1);
constexpr uint32_t BKT_FLAG_MAKETABLE   = (1 << 2);
constexpr uint32_t BKT_FLAG_MAKERAW     = (1 << 3);

namespace BKTurbo8
{
	int  workCycle(fs::path &strInFileName);
	void Usage();
	bool SkipTailString(wchar_t &ch);
	void ParseLine();
	void SaveFile(const fs::path &strName);
	void PrintLabelTable(const fs::path &strName, const fs::path &strExt);
}
