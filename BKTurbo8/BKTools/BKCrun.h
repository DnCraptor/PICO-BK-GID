#pragma once

#include "BKTools.h"

extern MemoryModel MMemory;
extern MemoryModel CrunMemory;

void CrunPack(int nFileLoadAddr, int nFileLen, int nFileActualAddr);
void CrunUnPack(int nFileLoadAddr, int nFileLen, int nFileActualAddr);

