#pragma once
#include "LabelTable.h"

#define DEBUG_LABEL_MANAGER 0

extern CRefsTable  g_RefsTable;     // таблица ссылок на метки, локальные и глобальные.
extern CLabelTable g_labelGlobalDefs;  // таблица глобальных меток
extern CLabelTable g_labelLocalDefs;   // таблица локальных меток, должна очищаться каждый раз при добавлении метки в таблицу глобальных меток

namespace LabelManager
{
	bool AddLocalLabel(CBKToken *token, const int value, const uint32_t lt);
	bool AddGlobalLabel(CBKToken *token, const int value, const uint32_t lt);
	bool AddLabelReference(CBKToken *token, const int value, const uint32_t lt);
	// поиск метки в таблицах
	bool SearchLabelInTables(CBKToken *token, int &value, uint32_t &lt);

	void ProcLocLabels();
	void LabelLinking();

#if (DEBUG_LABEL_MANAGER)
	// /-----------------------debug---------------------------
	extern FILE *dbgF;
	void DebugInit();
	// /-----------------------debug---------------------------
#endif
}
