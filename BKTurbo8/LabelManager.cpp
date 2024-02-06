#include "pch.h"
#include "LabelManager.h"
#include "Assemble.h"
#include "Globals.h"
#include "ErrorManager.h"

CRefsTable  g_RefsTable;        // таблица ссылок на метки, локальные и глобальные.
CLabelTable g_labelGlobalDefs;  // таблица глобальных меток
CLabelTable g_labelLocalDefs;   // таблица локальных меток, должна очищаться каждый раз при добавлении метки в таблицу глобальных меток

#pragma warning(disable:4996)

#if (DEBUG_LABEL_MANAGER)
// /-----------------------debug---------------------------
FILE *LabelManager::dbgF = nullptr;
void LabelManager::DebugInit()
{
	dbgF = _wfopen(L"debug_lbl.log", L"wt");
}
// /-----------------------debug---------------------------
#endif
/*
Добавление метки в таблицу локальных меток
*/
bool LabelManager::AddLocalLabel(CBKToken *token, const int value, const uint32_t lt)
{
	bool bRet = g_labelLocalDefs.AddLabel(token, value, lt);
#if (DEBUG_LABEL_MANAGER)

	// /-----------------------debug---------------------------
	if (bRet)
	{
		fwprintf(dbgF, L"\nAdd Local %06o: '%s'\t(%d)\n", value, token->getName().c_str(), g_labelLocalDefs.getSize());
	}
	else
	{
		fwprintf(dbgF, L"\n!Add Local %06o: '%s'\t (%d) Already defined.\n", value, token->getName().c_str(), g_labelLocalDefs.getSize());
	}

	// /-----------------------debug---------------------------
#endif

	if (bRet)
	{
		g_RefsTable.FixLocalLabels(token, value);
		ProcLocLabels(); // пробежимся по таблице
	}

	return bRet;
}

/*
Добавление метки в таблицу глобальных меток
*/
bool LabelManager::AddGlobalLabel(CBKToken *token, const int value, const uint32_t lt)
{
	bool bRet = false;

	if (bRet = g_labelGlobalDefs.AddLabel(token, value, lt))
	{
		g_labelLocalDefs.Clear();
		g_RefsTable.LockUndefinedLocalLabels(); // зафиксируем ссылки на неопределённые локальные метки
	}

#if (DEBUG_LABEL_MANAGER)

	// /-----------------------debug---------------------------
	if (bRet)
	{
		fwprintf(dbgF, L"\nAdd Global %06o: '%s'\t(%d)\n", value, token->getName().c_str(), g_labelGlobalDefs.getSize());
		fwprintf(dbgF, L"Clear Local defs. (%d)\n", g_labelLocalDefs.getSize());
	}
	else
	{
		fwprintf(dbgF, L"\n!Add Global %06o: '%s'\t(%d) Already defined.\n", value, token->getName().c_str(), g_labelGlobalDefs.getSize());
		fwprintf(dbgF, L"!Not Clear Local defs. (%d)\n", g_labelLocalDefs.getSize());
	}

	// /-----------------------debug---------------------------
#endif
	return bRet;
}

/*
Добавление ссылки на метку в таблицу ссылок
*/
bool LabelManager::AddLabelReference(CBKToken *token, const int value, const uint32_t lt)
{
	bool bRet = g_RefsTable.AddRefs(token, value, lt);
	return bRet;
}

bool LabelManager::SearchLabelInTables(CBKToken *token, int &value, uint32_t &lt)
{
	value = 0;
#if (DEBUG_LABEL_MANAGER)
	// /-----------------------debug---------------------------
	fwprintf(dbgF, L"\tSearch label: '%s'\n", token->getName().c_str());
	// /-----------------------debug---------------------------
#endif
	int n = g_labelLocalDefs.SearchLabel(token);

	if (n >= 0)
	{
		value = g_labelLocalDefs.GetValue(n);
		lt = g_labelLocalDefs.GetType(n);

		if ((lt & LBL_DEFINITE_MASK) == LBL_LOCAL)
		{
			value = g_Globals.GetRealAddress(value);
		}

#if (DEBUG_LABEL_MANAGER)
		// /-----------------------debug---------------------------
		fwprintf(dbgF, L"\tFound in Local Table: %06o\n", value);
		// /-----------------------debug---------------------------
#endif
	}
	else
	{
		n = g_labelGlobalDefs.SearchLabel(token);

		if (n >= 0)
		{
			value = g_labelGlobalDefs.GetValue(n);
			lt = g_labelGlobalDefs.GetType(n);

			if ((lt & LBL_DEFINITE_MASK) == LBL_GLOBAL)
			{
				value = g_Globals.GetRealAddress(value);
			}

			if ((lt & LBL_DEFINITE_MASK) == LBL_WEAKDEFINE)
			{
				// такие определения невалидны
				return false;
			}

#if (DEBUG_LABEL_MANAGER)
			// /-----------------------debug---------------------------
			fwprintf(dbgF, L"\tFound in Global Table: %06o\n", value);
			// /-----------------------debug---------------------------
#endif
		}
	}

#if (DEBUG_LABEL_MANAGER)

	// /-----------------------debug---------------------------
	if (n < 0)
	{
		fwprintf(dbgF, L"\tLabel not found.\n");
	}

	// /-----------------------debug---------------------------
#endif
	return (n >= 0);
}

/*
линковка меток
*/


// нужно пробежаться по таблице ссылок и для всех выражений в которых есть локальыне метки или которые помечены как branch
// попробовать вычислить выражение.
void LabelManager::ProcLocLabels()
{
	std::vector<size_t> vInt;
	size_t sz = g_RefsTable.getSize();

	// просмотрим таблицу
	for (size_t i = 0; i < sz; ++i) // для всех ссылок на метки
	{
		uint32_t lti = g_RefsTable.GetType(i);
		LabelRef &ref = g_RefsTable.GetElement(i);

		if ((lti & REFERENCE_TYPE_MASK) == ARL_BRANCH_LABEL)
		{
			int result = 0;

			// если вычислилось - то ок
			if (g_RPNParser.CalcRpn(result, ref.getRPN()))
			{
				int cp = ref.getAddress();

				if (Assemble::BranchVerify(g_Globals.GetRealAddress(cp), result))
				{
					g_Memory.b[cp] = (result & 0xff); // задаём финальное смещение
				}
				else
				{
					ErrorManager::OutError(ERRNUM::E_109, true, cp); // Ошибка длины перехода по оператору ветвления.
				}

				vInt.push_back(i);
			}

			// если не вычислилось, то продолжим как есть.
		}
		else if ((lti & ARL_LOCAL) && (g_Globals.GetLinkMode() != LINKING_MODE::CL))
		{
			// при режиме CL тут надо не вычислять значение, а исправлять цепочку RPN.
			// находить там нужную локальную метку, у которой значение -1 и менять его на полученное.
			int result = 0;

			// если вычислилось - то ок
			if (g_RPNParser.CalcRpn(result, ref.getRPN()))
			{
				int cp = ref.getAddress();

				if ((lti & REFERENCE_TYPE_MASK) == ARL_OFFSET_LABEL)
				{
					result = (result - (g_Globals.GetRealAddress(cp) + 2));
					g_Memory.w[cp / 2] += static_cast<uint16_t>(result);
				}
				else
				{
					if (lti & ARL_BYTEL)
					{
						g_Memory.b[cp] = result & 0xff;
					}
					else
					{
						g_Memory.w[cp / 2] = result & 0xffff;
					}
				}

				vInt.push_back(i);
			}
		}
	}

	// удаляем всё что обработали

	while (!vInt.empty())
	{
		size_t d = vInt.back();
		g_RefsTable.DeleteElement(d);
		vInt.pop_back();
	}

#ifdef _DEBUG
	g_RefsTable.Dump();
	g_labelGlobalDefs.Dump();
	g_labelLocalDefs.Dump();
#endif
}


void LabelManager::LabelLinking()
{
	size_t sz = g_RefsTable.getSize();

	if (sz)
	{
		std::vector<size_t> vInt;
		bool bRept = false;

		do
		{
			bRept = false;
			sz = g_RefsTable.getSize();

			// компонуем метки
			for (size_t i = 0; i < sz; ++i) // для всех ссылок на метки
			{
				uint32_t lti = g_RefsTable.GetType(i);
				LabelRef &ref = g_RefsTable.GetElement(i);

				// если это переход (к обработке обязательно) или окончательная обработка (после команды .END)
				if (((lti & REFERENCE_TYPE_MASK) == ARL_BRANCH_LABEL) || (g_Globals.GetLinkMode() == LINKING_MODE::FINAL))
				{
					if ((lti & ARL_DEFINITE_MASK) == ARL_DEFINE)
					{
						int result = 0;

						// если вычислилось - то ок
						if (g_RPNParser.CalcRpn(result, ref.getRPN()))
						{
							CBKToken *token = ref.getPDefine();
							int n = g_labelGlobalDefs.SearchLabel(token);

							if (n >= 0)
							{
								Label &lbl = g_labelGlobalDefs.GetElement(n);

								if ((lbl.getType() & LBL_DEFINITE_MASK) == LBL_WEAKDEFINE)
								{
									lbl.setType(LBL_DEFINE);
									lbl.setValue(result);
								}
								else
								{
									if (lbl.getValue() != result)
									{
										ErrorManager::OutError(ERRNUM::E_113); // иначе ошибка - повторное определение метки
									}
								}
							}
							else
							{
								LabelManager::AddGlobalLabel(token, result, LBL_DEFINE);
#ifdef _DEBUG
								g_labelGlobalDefs.Dump();
								g_labelLocalDefs.Dump();
#endif
							}

							bRept = true;
							vInt.push_back(i);
						}
					}
					else if ((lti & REFERENCE_TYPE_MASK) == ARL_BRANCH_LABEL) // эта ветка никогда не должна выполняться, т.к.
					{
						int result = 0;

						// если вычислилось - то ок
						if (g_RPNParser.CalcRpn(result, ref.getRPN()))
						{
							int cp = ref.getAddress();

							if (Assemble::BranchVerify(g_Globals.GetRealAddress(cp), result))
							{
								g_Memory.b[cp] = (result & 0xff); // задаём финальное смещение
							}
							else
							{
								ErrorManager::OutError(ERRNUM::E_109, true, cp); // Ошибка длины перехода по оператору ветвления.
							}

							vInt.push_back(i);
						}

						// если не вычислилось, то продолжим как есть.
					}
					else
					{
						int result = 0;

						// если вычислилось - то ок
						if (g_RPNParser.CalcRpn(result, ref.getRPN()))
						{
							int cp = ref.getAddress();

							if ((lti & REFERENCE_TYPE_MASK) == ARL_OFFSET_LABEL)
							{
								result = (result - (g_Globals.GetRealAddress(cp) + 2));
								g_Memory.w[cp / 2] += static_cast<uint16_t>(result);
							}
							else
							{
								if (lti & ARL_BYTEL)
								{
									g_Memory.b[cp] = result & 0xff;
								}
								else
								{
									g_Memory.w[cp / 2] = result & 0xffff;
								}
							}

							vInt.push_back(i);
						}
					}
				}
			}

			// удаляем всё что обработали

			while (!vInt.empty())
			{
				size_t d = vInt.back();
				g_RefsTable.DeleteElement(d);
				vInt.pop_back();
			}

#ifdef _DEBUG
			g_RefsTable.Dump();
			g_labelGlobalDefs.Dump();
			g_labelLocalDefs.Dump();
#endif
		}
		while (bRept);
	}
}
