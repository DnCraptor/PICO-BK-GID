#pragma once

#include "BKToken.h"

struct Registers_t
{
	int nNum;
	std::vector<CBKToken>names;
};

enum class CPUCmdGroup
{
	NOOPS,
	CBRANCH,
	EIS,
	TRAP,
	SOB,
	MARK,
	TWOOPREG,
	FIS,
	PUSH,
	ONEOPS,
	TWOOPS,
	FPUCLASS1,
	FPUCLASS2,
	FPUCLASS2_1,
};

struct CPUCommandStruct
{
	std::wstring    strName;    // Имя мнемоники
	uint16_t        nOpcode;    // генерируемый опкод
	CPUCmdGroup     nGroup;     // группа, к которой принадлежит команда
};

