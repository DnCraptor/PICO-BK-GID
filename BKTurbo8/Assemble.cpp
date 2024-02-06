#include "pch.h"
#include <cwctype>

#include "Globals.h"
#include "Assemble.h"
#include "BKToken.h"
#include "LabelManager.h"
#include "Parser.h"
#include "Listing.h"
#include "ErrorManager.h"
#include "ScriptAsm.h"

static const std::vector <CPUCommandStruct> g_CPUCommands =
{
	{ L"HALT",   0000000,    CPUCmdGroup::NOOPS }, // op
	{ L"WAIT",   0000001,    CPUCmdGroup::NOOPS },
	{ L"RTI",    0000002,    CPUCmdGroup::NOOPS },
	{ L"BPT",    0000003,    CPUCmdGroup::NOOPS },
	{ L"IOT",    0000004,    CPUCmdGroup::NOOPS },
	{ L"RESET",  0000005,    CPUCmdGroup::NOOPS },
	{ L"RTT",    0000006,    CPUCmdGroup::NOOPS },
	{ L"START",  0000012,    CPUCmdGroup::NOOPS },
	{ L"STEP",   0000016,    CPUCmdGroup::NOOPS },
	{ L"NOP",    0000240,    CPUCmdGroup::NOOPS },
	{ L"CLC",    0000241,    CPUCmdGroup::NOOPS },
	{ L"CLV",    0000242,    CPUCmdGroup::NOOPS },
	{ L"CLVC",   0000243,    CPUCmdGroup::NOOPS },
	{ L"CLZ",    0000244,    CPUCmdGroup::NOOPS },
	{ L"CLZC",   0000245,    CPUCmdGroup::NOOPS },
	{ L"CLZV",   0000246,    CPUCmdGroup::NOOPS },
	{ L"CLZVC",  0000247,    CPUCmdGroup::NOOPS },
	{ L"CLN",    0000250,    CPUCmdGroup::NOOPS },
	{ L"CLNC",   0000251,    CPUCmdGroup::NOOPS },
	{ L"CLNV",   0000252,    CPUCmdGroup::NOOPS },
	{ L"CLNVC",  0000253,    CPUCmdGroup::NOOPS },
	{ L"CLNZ",   0000254,    CPUCmdGroup::NOOPS },
	{ L"CLNZC",  0000255,    CPUCmdGroup::NOOPS },
	{ L"CLNZV",  0000256,    CPUCmdGroup::NOOPS },
	{ L"CCC",    0000257,    CPUCmdGroup::NOOPS },
	{ L"SEC",    0000261,    CPUCmdGroup::NOOPS },
	{ L"SEV",    0000262,    CPUCmdGroup::NOOPS },
	{ L"SEVC",   0000263,    CPUCmdGroup::NOOPS },
	{ L"SEZ",    0000264,    CPUCmdGroup::NOOPS },
	{ L"SEZC",   0000265,    CPUCmdGroup::NOOPS },
	{ L"SEZV",   0000266,    CPUCmdGroup::NOOPS },
	{ L"SEZVC",  0000267,    CPUCmdGroup::NOOPS },
	{ L"SEN",    0000270,    CPUCmdGroup::NOOPS },
	{ L"SENC",   0000271,    CPUCmdGroup::NOOPS },
	{ L"SENV",   0000272,    CPUCmdGroup::NOOPS },
	{ L"SENVC",  0000273,    CPUCmdGroup::NOOPS },
	{ L"SENZ",   0000274,    CPUCmdGroup::NOOPS },
	{ L"SENZC",  0000275,    CPUCmdGroup::NOOPS },
	{ L"SENZV",  0000276,    CPUCmdGroup::NOOPS },
	{ L"SCC",    0000277,    CPUCmdGroup::NOOPS },
	{ L"RET",    0000207,    CPUCmdGroup::NOOPS },
	{ L"RETURN", 0000207,    CPUCmdGroup::NOOPS },

	{ L"BR",     0000400,    CPUCmdGroup::CBRANCH }, // op lll
	{ L"BNE",    0001000,    CPUCmdGroup::CBRANCH },
	{ L"BEQ",    0001400,    CPUCmdGroup::CBRANCH },
	{ L"BGE",    0002000,    CPUCmdGroup::CBRANCH },
	{ L"BLT",    0002400,    CPUCmdGroup::CBRANCH },
	{ L"BGT",    0003000,    CPUCmdGroup::CBRANCH },
	{ L"BLE",    0003400,    CPUCmdGroup::CBRANCH },
	{ L"BPL",    0100000,    CPUCmdGroup::CBRANCH },
	{ L"BMI",    0100400,    CPUCmdGroup::CBRANCH },
	{ L"BHI",    0101000,    CPUCmdGroup::CBRANCH },
	{ L"BLOS",   0101400,    CPUCmdGroup::CBRANCH },
	{ L"BVC",    0102000,    CPUCmdGroup::CBRANCH },
	{ L"BVS",    0102400,    CPUCmdGroup::CBRANCH },
	{ L"BCC",    0103000,    CPUCmdGroup::CBRANCH },
	{ L"BCS",    0103400,    CPUCmdGroup::CBRANCH },
	{ L"BHIS",   0103000,    CPUCmdGroup::CBRANCH },
	{ L"BLO",    0103400,    CPUCmdGroup::CBRANCH },

	{ L"MUL",    0070000,    CPUCmdGroup::EIS }, // op ss,r
	{ L"DIV",    0071000,    CPUCmdGroup::EIS },
	{ L"ASH",    0072000,    CPUCmdGroup::EIS },
	{ L"ASHC",   0073000,    CPUCmdGroup::EIS },

	{ L"EMT",    0104000,    CPUCmdGroup::TRAP }, // op nnn
	{ L"TRAP",   0104400,    CPUCmdGroup::TRAP },

	{ L"SOB",    0077000,    CPUCmdGroup::SOB }, // op r,ll

	{ L"MARK",   0006400,    CPUCmdGroup::MARK }, // op nn

	{ L"JSR",    0004000,    CPUCmdGroup::TWOOPREG }, // op r,dd
	{ L"XOR",    0074000,    CPUCmdGroup::TWOOPREG },

	{ L"FADD",   0075000,    CPUCmdGroup::FIS }, // op r
	{ L"FSUB",   0075010,    CPUCmdGroup::FIS },
	{ L"FMUL",   0075020,    CPUCmdGroup::FIS },
	{ L"FDIV",   0075030,    CPUCmdGroup::FIS },
	{ L"RTS",    0000200,    CPUCmdGroup::FIS },

	{ L"PUSH",   0010046,    CPUCmdGroup::PUSH }, // op ss
	{ L"PUSHB",  0110046,    CPUCmdGroup::PUSH },

	{ L"JMP",    0000100,    CPUCmdGroup::ONEOPS }, // op dd
	{ L"SWAB",   0000300,    CPUCmdGroup::ONEOPS },
	{ L"MTPS",   0106400,    CPUCmdGroup::ONEOPS },
	{ L"MFPS",   0106700,    CPUCmdGroup::ONEOPS },
	{ L"MTPD",   0106600,    CPUCmdGroup::ONEOPS },
	{ L"MFPD",   0106500,    CPUCmdGroup::ONEOPS },
	{ L"MTPI",   0006600,    CPUCmdGroup::ONEOPS },
	{ L"MFPI",   0006500,    CPUCmdGroup::ONEOPS },
	{ L"SXT",    0006700,    CPUCmdGroup::ONEOPS },
	{ L"CALL",   0004700,    CPUCmdGroup::ONEOPS },
	{ L"POP",    0012600,    CPUCmdGroup::ONEOPS },
	{ L"POPB",   0112600,    CPUCmdGroup::ONEOPS },
	{ L"CLR",    0005000,    CPUCmdGroup::ONEOPS },
	{ L"COM",    0005100,    CPUCmdGroup::ONEOPS },
	{ L"INC",    0005200,    CPUCmdGroup::ONEOPS },
	{ L"DEC",    0005300,    CPUCmdGroup::ONEOPS },
	{ L"NEG",    0005400,    CPUCmdGroup::ONEOPS },
	{ L"ADC",    0005500,    CPUCmdGroup::ONEOPS },
	{ L"SBC",    0005600,    CPUCmdGroup::ONEOPS },
	{ L"TST",    0005700,    CPUCmdGroup::ONEOPS },
	{ L"ROR",    0006000,    CPUCmdGroup::ONEOPS },
	{ L"ROL",    0006100,    CPUCmdGroup::ONEOPS },
	{ L"ASR",    0006200,    CPUCmdGroup::ONEOPS },
	{ L"ASL",    0006300,    CPUCmdGroup::ONEOPS },
	{ L"CLRB",   0105000,    CPUCmdGroup::ONEOPS },
	{ L"COMB",   0105100,    CPUCmdGroup::ONEOPS },
	{ L"INCB",   0105200,    CPUCmdGroup::ONEOPS },
	{ L"DECB",   0105300,    CPUCmdGroup::ONEOPS },
	{ L"NEGB",   0105400,    CPUCmdGroup::ONEOPS },
	{ L"ADCB",   0105500,    CPUCmdGroup::ONEOPS },
	{ L"SBCB",   0105600,    CPUCmdGroup::ONEOPS },
	{ L"TSTB",   0105700,    CPUCmdGroup::ONEOPS },
	{ L"RORB",   0106000,    CPUCmdGroup::ONEOPS },
	{ L"ROLB",   0106100,    CPUCmdGroup::ONEOPS },
	{ L"ASRB",   0106200,    CPUCmdGroup::ONEOPS },
	{ L"ASLB",   0106300,    CPUCmdGroup::ONEOPS },

	{ L"MOV",    0010000,    CPUCmdGroup::TWOOPS }, // op ss, dd
	{ L"CMP",    0020000,    CPUCmdGroup::TWOOPS },
	{ L"BIT",    0030000,    CPUCmdGroup::TWOOPS },
	{ L"BIC",    0040000,    CPUCmdGroup::TWOOPS },
	{ L"BIS",    0050000,    CPUCmdGroup::TWOOPS },
	{ L"MOVB",   0110000,    CPUCmdGroup::TWOOPS },
	{ L"CMPB",   0120000,    CPUCmdGroup::TWOOPS },
	{ L"BITB",   0130000,    CPUCmdGroup::TWOOPS },
	{ L"BICB",   0140000,    CPUCmdGroup::TWOOPS },
	{ L"BISB",   0150000,    CPUCmdGroup::TWOOPS },
	{ L"ADD",    0060000,    CPUCmdGroup::TWOOPS },
	{ L"SUB",    0160000,    CPUCmdGroup::TWOOPS },
	// FPU инструкции.
	{ L"CFCC",   0170000,    CPUCmdGroup::NOOPS }, // класс 0 - без операндов
	{ L"SETF",   0170001,    CPUCmdGroup::NOOPS }, // op
	{ L"SETI",   0170002,    CPUCmdGroup::NOOPS },
	{ L"SETD",   0170011,    CPUCmdGroup::NOOPS },
	{ L"SETL",   0170012,    CPUCmdGroup::NOOPS },

	{ L"LDFPS",  0170100,    CPUCmdGroup::ONEOPS }, // класс 1 - с одним операндом
	{ L"STFPS",  0170200,    CPUCmdGroup::ONEOPS },
	{ L"STST",   0170300,    CPUCmdGroup::ONEOPS },
	{ L"CLRD",   0170400,    CPUCmdGroup::FPUCLASS1 },
	{ L"CLRF",   0170400,    CPUCmdGroup::FPUCLASS1 },
	{ L"TSTD",   0170500,    CPUCmdGroup::FPUCLASS1 },
	{ L"TSTF",   0170500,    CPUCmdGroup::FPUCLASS1 },
	{ L"ABSD",   0170600,    CPUCmdGroup::FPUCLASS1 },
	{ L"ABSF",   0170600,    CPUCmdGroup::FPUCLASS1 },
	{ L"NEGD",   0170700,    CPUCmdGroup::FPUCLASS1 },
	{ L"NEGF",   0170700,    CPUCmdGroup::FPUCLASS1 },

	{ L"MULD",   0171000,    CPUCmdGroup::FPUCLASS2 }, // класс 2 - с двумя операндами
	{ L"MULF",   0171000,    CPUCmdGroup::FPUCLASS2 },
	{ L"MODD",   0171400,    CPUCmdGroup::FPUCLASS2 },
	{ L"MODF",   0171400,    CPUCmdGroup::FPUCLASS2 },
	{ L"ADDD",   0172000,    CPUCmdGroup::FPUCLASS2 },
	{ L"ADDF",   0172000,    CPUCmdGroup::FPUCLASS2 },
	{ L"LDD",    0172400,    CPUCmdGroup::FPUCLASS2 },
	{ L"LDF",    0172400,    CPUCmdGroup::FPUCLASS2 },
	{ L"SUBD",   0173000,    CPUCmdGroup::FPUCLASS2 },
	{ L"SUBF",   0173000,    CPUCmdGroup::FPUCLASS2 },
	{ L"CMPD",   0173400,    CPUCmdGroup::FPUCLASS2 },
	{ L"CMPF",   0173400,    CPUCmdGroup::FPUCLASS2 },
	{ L"STD",    0174000,    CPUCmdGroup::FPUCLASS2 },
	{ L"STF",    0174000,    CPUCmdGroup::FPUCLASS2 },
	{ L"DIVD",   0174400,    CPUCmdGroup::FPUCLASS2 },
	{ L"DIVF",   0174400,    CPUCmdGroup::FPUCLASS2 },
	{ L"STEXP",  0175000,    CPUCmdGroup::FPUCLASS2_1 },
	{ L"STCDI",  0175400,    CPUCmdGroup::FPUCLASS2_1 },
	{ L"STCDL",  0175400,    CPUCmdGroup::FPUCLASS2_1 },
	{ L"STCFI",  0175400,    CPUCmdGroup::FPUCLASS2_1 },
	{ L"STCFL",  0175400,    CPUCmdGroup::FPUCLASS2_1 },
	{ L"STCDF",  0176000,    CPUCmdGroup::FPUCLASS2 },
	{ L"STCFD",  0176000,    CPUCmdGroup::FPUCLASS2 },
	{ L"LDEXP",  0176400,    CPUCmdGroup::FPUCLASS2_1 },
	{ L"LDCID",  0177000,    CPUCmdGroup::FPUCLASS2_1 },
	{ L"LDCLD",  0177000,    CPUCmdGroup::FPUCLASS2_1 },
	{ L"LDCIF",  0177000,    CPUCmdGroup::FPUCLASS2_1 },
	{ L"LDCLF",  0177000,    CPUCmdGroup::FPUCLASS2_1 },
	{ L"LDCFD",  0177400,    CPUCmdGroup::FPUCLASS2 },
	{ L"LDCDF",  0177400,    CPUCmdGroup::FPUCLASS2 }
};

static const std::vector <Assemble::PseudoCommandStruct> g_PseudoCommands =
{
	{ L"ADDR",   true,   false, Assemble::PSC_addr },
	{ L"LA",     true,   false, Assemble::PSC_la },
	{ L"LINK",   true,   false, Assemble::PSC_la }, // синоним для LA
	{ L"PRINT",  true,   false, Assemble::PSC_print },
	{ L"BLKW",   true,   false, Assemble::PSC_blkw },
	{ L"WORD",   true,   false, Assemble::PSC_word },
	{ L"RAD50",  true,   false, Assemble::PSC_rad50 },
	{ L"END",    false,  false, Assemble::PSC_end },
	{ L"EVEN",   false,  false, Assemble::PSC_even },
	{ L"BLKB",   false,  false, Assemble::PSC_blkb },
	{ L"BYTE",   false,  false, Assemble::PSC_byte },
	{ L"ASCII",  false,  false, Assemble::PSC_ascii },
	{ L"ASCIZ",  false,  false, Assemble::PSC_asciz },
	{ L"ORG",    false,  false, Assemble::PSC_org },
	{ L"FLT2",   true,   false, Assemble::PSC_flt2 },
	{ L"FLT4",   true,   false, Assemble::PSC_flt4 },
	{ L"INCLUDE", false, false, Assemble::PSC_Include },
	{ L"ENABL",  false,  false, Assemble::PSC_enabl },
	{ L"DSABL",  false,  false, Assemble::PSC_dsabl },
	{ L"SCRIPT", false,  false, Assemble::PSC_Script },
	{ L"ENDS",   false,  true,  Assemble::PSC_EndScript },
};

// вход: token - текущая прочитанная инструкция, или по крайней мере что-то похожее.
//      cp - текущий временный PC, на выходе должен указывать на слово за командой.
// выход: true - нормально
//      false - ошибка
bool Assemble::AssembleCPUInstruction(CBKToken *token, int &cp, wchar_t &ch)
{
	if (cp & 1)
	{
		ErrorManager::OutError(ERRNUM::E_117);  // Нечётный адрес команды.
		g_Memory.b[cp++] = 0;   // выровняем и будем компилировать дальше.
	}

	g_Globals.SetOperandType(OPERAND_TYPE::DST); // считаем, что по умолчанию - приёмник
	g_Globals.SetAriphmType(0);             // Всё разрешено

	for (auto &cmd : g_CPUCommands)  // пока не конец таблицы
	{
		if (token->calcHash(cmd.strName) == token->getHash()) // если нашли мнемонику
		{
#ifdef _DEBUG

			// проверка на ложное срабатывание.
			if (cmd.strName != token->getName())
			{
				wprintf(L"HASH Error: %s:%#zX ~~ %s:%#zX\n", cmd.strName.c_str(), token->calcHash(cmd.strName), token->getName().c_str(), token->getHash());
				assert(false);
			}

#endif
			// если нашли нужную мнемонику
			// нужно взять опкод.
			g_Memory.w[cp / 2] = cmd.nOpcode;
			g_Memory.w[cp / 2 + 1] = 0; // следующие два слова заранее обнулим
			g_Memory.w[cp / 2 + 2] = 0;
			bool bRet = false;
			ListingManager::AddPrepareLine(cp, ListType::LT_INSTRUCTION);

			// и обработать операнды в соответствии с типом группы, к которой принадлежит опкод.
			switch (cmd.nGroup)
			{
				case CPUCmdGroup::NOOPS:        // ничего делать не надо.
					bRet = true;
					break;

				case CPUCmdGroup::CBRANCH:
					bRet = assembleBR(cp, ch);
					break;

				case CPUCmdGroup::EIS:
					bRet = assemble2ROP(cp, ch);
					break;

				case CPUCmdGroup::TRAP:
					bRet = assembleTRAP(cp, ch);
					break;

				case CPUCmdGroup::SOB:
					bRet = assembleSOB(cp, ch);
					break;

				case CPUCmdGroup::MARK:
					bRet = assembleMARK(cp, ch);
					break;

				case CPUCmdGroup::TWOOPREG:
					bRet = assemble2OPR(cp, ch);
					break;

				case CPUCmdGroup::FIS:
					bRet = assemble1OPR(cp, ch);
					break;

				case CPUCmdGroup::PUSH:
					g_Globals.SetOperandType(OPERAND_TYPE::SRC); // меняем тип операнда

				// и переходим к выполнению CPUCmdGroup::ONEOPS
				case CPUCmdGroup::ONEOPS:
					bRet = assemble1OP(cp, ch);
					break;

				case CPUCmdGroup::TWOOPS:
					bRet = assemble2OP(cp, ch);
					break;

				case CPUCmdGroup::FPUCLASS1:
					bRet = assemble1OPFPU(cp, ch);
					break;

				case CPUCmdGroup::FPUCLASS2:
					bRet = assemble2OPRFPU(cp, ch, true);
					break;

				case CPUCmdGroup::FPUCLASS2_1:
					bRet = assemble2OPRFPU(cp, ch, false);
					break;
			}

			cp += 2;
			return bRet;
		}
	}

	// ошибка - неопознанная команда.
	ErrorManager::OutError(ERRNUM::E_106); // Неправильная команда.
	return false;
}

// сборка двухоперандных команд
bool Assemble::assemble2OP(int &cp, wchar_t &ch)
{
	g_Globals.SetOperandType(OPERAND_TYPE::SRC); // сперва ss

	if (assemble1OP(cp, ch))   // обработаем один операнд
	{
		if (Parser::needChar(L',', ch))    // проверим наличие второго
		{
			g_Globals.SetOperandType(OPERAND_TYPE::DST); // теперь dd
			ch = g_pReader->readChar(true); // пропускаем запятую
			return assemble1OP(cp, ch);     // обработаем второй операнд
		}

		ErrorManager::OutError(ERRNUM::E_124);    // Ошибка в команде - нет второго операнда.
		return false;
	}

	return false;
}

// сборка обычных однооперандных команд
bool Assemble::assemble1OP(int &cp, wchar_t &ch)
{
	cp += 2; // двигаем указатель на местоположение операнда
	return Parser::Operand_analyse(cp, ch, false); // разбираемся, что там такое
}

// сборка однооперандных команд FPU class 1, где операнд - FPU/общ.
bool Assemble::assemble1OPFPU(int &cp, wchar_t &ch)
{
	cp += 2; // двигаем указатель на местоположение операнда
	return Parser::Operand_analyse(cp, ch, true); // разбираемся, что там такое
}

// сборка однооперандных команд, где операнд - регистр
bool Assemble::assemble1OPR(int &cp, wchar_t &ch)
{
	g_pReader->SkipWhitespaces(ch);

	if (Parser::ReadRegName(ch))
	{
		return true;
	}

	ErrorManager::OutError(ERRNUM::E_111); // Ошибка в имени регистра.
	return false;
}

// обработка первого операнда - регистра FPU.
bool Assemble::assemble1OPRFPU(int &cp, wchar_t &ch)
{
	g_pReader->SkipWhitespaces(ch);
	CBKToken reg;

	if (g_pReader->readToken(&reg, ch))
	{
		if (Parser::ParseFPURegName(&reg, true))
		{
			return true;
		}
	}

	ErrorManager::OutError(ERRNUM::E_132); // Ошибка в имени регистра FPU.
	return false;
}

// сборка двухоперандных команд, где первый операнд - регистр
bool Assemble::assemble2OPR(int &cp, wchar_t &ch)
{
	g_Globals.SetOperandType(OPERAND_TYPE::SRC); // сперва ss

	if (assemble1OPR(cp, ch))       // обрабатываем регистр
	{
		if (Parser::needChar(L',', ch))     // проверим наличие второго операнда
		{
			g_Globals.SetOperandType(OPERAND_TYPE::DST); // теперь dd
			ch = g_pReader->readChar(true); // пропускаем запятую
			return assemble1OP(cp, ch);     // обработаем второй операнд
		}

		ErrorManager::OutError(ERRNUM::E_124);    // Ошибка в команде - нет второго операнда.
		return false;
	}

	return false;
}

// сборка двухоперандных команд FPU, где первый операнд - регистр AC0..AC3
// а второй - FPU/общ.
bool Assemble::assemble2OPRFPU(int &cp, wchar_t &ch, bool bFPU)
{
	g_Globals.SetOperandType(OPERAND_TYPE::SRC); // сперва ss

	if (assemble1OPRFPU(cp, ch))    // обрабатываем регистр
	{
		if (Parser::needChar(L',', ch))     // проверим наличие второго операнда
		{
			g_Globals.SetOperandType(OPERAND_TYPE::DST); // теперь dd
			ch = g_pReader->readChar(true); // пропускаем запятую

			if (bFPU)
			{
				return assemble1OPFPU(cp, ch);  // обработаем второй операнд
			}

			return assemble1OP(cp, ch);     // обработаем второй операнд
		}

		ErrorManager::OutError(ERRNUM::E_124);    // Ошибка в команде - нет второго операнда.
		return false;
	}

	return false;
}


// сборка двухоперандных команд, где второй операнд - регистр
// Это EIS команды, у которых  при записи мнемоники почему-то поменяны местами
// источник и приёмник, как у x86
bool Assemble::assemble2ROP(int &cp, wchar_t &ch)
{
	g_Globals.SetOperandType(OPERAND_TYPE::DST); // сперва dd

	if (assemble1OP(cp, ch))   // обработаем первый операнд
	{
		if (Parser::needChar(L',', ch))    // проверим наличие второго
		{
			g_Globals.SetOperandType(OPERAND_TYPE::SRC); // теперь ss
			ch = g_pReader->readChar(true); // пропускаем запятую
			return assemble1OPR(cp, ch);    // обработаем второй операнд
		}

		ErrorManager::OutError(ERRNUM::E_124);    // Ошибка в команде - нет второго операнда.
		return false;
	}

	return false;
}

// сборка команд ветвления
bool Assemble::assembleBR(int &cp, wchar_t &ch)
{
	bool bRet = false;
	g_Globals.SetAriphmType(ARIPHM_INBRANCH);

	if (g_RPNParser.FullAriphmParser(ch))
	{
		int result = 0;

		if (bRet = g_RPNParser.CalcRpn(result)) // если получилось вычислить
		{
			if (BranchVerify(g_Globals.GetRealAddress(cp), result))
			{
				g_Memory.b[cp] = (result & 0xff); // задаём финальное смещение
				return true;
			}

			ErrorManager::OutError(ERRNUM::E_109); // Ошибка длины перехода по оператору ветвления.
			return false;
		}

		if (g_RPNParser.GetNamesNum() || g_RPNParser.GetLocNamesNum())
		{
			// в таблицу ссылок добавим невычисленную цепочку выражения
			bRet = LabelManager::AddLabelReference(nullptr, cp, ARL_CMDARG | ARL_BRANCH_LABEL);
		}
		else
		{
			assert(false); // такого быть не должно, надо разбираться.
		}
	}

	return bRet;   // выходим
}

bool Assemble::assembleSOB(int &cp, wchar_t &ch)
{
	g_Globals.SetOperandType(OPERAND_TYPE::SRC);     // сперва ss

	if (assemble1OPR(cp, ch))  // обработаем имя регистра
	{
		if (Parser::needChar(L',', ch))    // проверим наличие второго
		{
			g_Globals.SetOperandType(OPERAND_TYPE::DST);    // теперь dd
			ch = g_pReader->readChar(true); // пропускаем запятую
			// обработаем второй операнд
			g_Globals.SetAriphmType(ARIPHM_INBRANCH);

			if (g_RPNParser.FullAriphmParser(ch))
			{
				int result = 0;

				if (g_RPNParser.CalcRpn(result)) // вычисляем.
				{
					result -= (g_Globals.GetRealAddress(cp) + 2);    // смещение

					if (result < 0)     // если смещение < 0,  то всё верно
					{
						result = -result;   // вычисляем код смещения
						result /= 2;

						if (result <= 077)  // если он влазит
						{
							g_Memory.w[cp / 2] |= static_cast<uint16_t>(result); // формируем опкод
							return true;
						}
					}
				}
			}

			ErrorManager::OutError(ERRNUM::E_102); // Ошибка длины или направления перехода в команде SOB.
			return false;
		}

		ErrorManager::OutError(ERRNUM::E_120);    // Ошибка в команде.
		return false;
	}

	return false;
}

bool Assemble::assembleTRAP(int &cp, wchar_t &ch)
{
	g_Globals.SetAriphmType(ARIPHM_NOLOCLABEL);
	int nOp = g_Memory.w[cp / 2]; // код опкода нужен, чтобы текст ошибки правильный вывести
	int result = 0;

	if (g_RPNParser.FullAriphmParser(ch)) // если арифметическое выражение распарсилось
	{
		if (g_RPNParser.CalcRpn(result))
		{
			if (std::abs(result) <= 0377)
			{
				g_Memory.w[cp / 2] |= static_cast<uint16_t>(result);
				return true;
			}

			ErrorManager::OutError(ERRNUM::E_125); // Переполнение байтового аргумента.
			return false;
		}

		if (g_RPNParser.GetNamesNum() || g_RPNParser.GetLocNamesNum()) // если там были имена меток или других определений
		{
			// в таблицу ссылок добавим невычисленную цепочку выражения
			LabelManager::AddLabelReference(nullptr, cp, ARL_CMDARG | ARL_RELATIVE_LABEL | ARL_BYTEL);
			return true;
		}

		// что-то непонятное случилось. надо разбираться
		assert(false);
	}

	ErrorManager::OutError((nOp == 0104400) ? ERRNUM::E_121 : ERRNUM::E_122); // Ошибка аргумента TRAP, EMT.
	return false;
}

bool Assemble::assembleMARK(int &cp, wchar_t &ch)
{
	int result = 0;
	g_Globals.SetAriphmType(ARIPHM_NOLOCLABEL | ARIPHM_NOUNDLABEL);

	if (g_RPNParser.FullAriphmParser(ch)) // если арифметическое выражение распарсилось
	{
		if (g_RPNParser.CalcRpn(result))
		{
			if (unsigned int(result) < 077)
			{
				g_Memory.w[cp / 2] |= static_cast<uint16_t>(result);
				return true;
			}
		}
	}

	ErrorManager::OutError(ERRNUM::E_110); // Ошибка аргумента MARK
	return false;
}


// вход: token - текущая прочитанная псевдокоманда, или по крайней мере что-то похожее.
//      cp - текущий временный PC, на выходе должен указывать на слово за командой.
// выход: true - нормально
//      false - ошибка
bool Assemble::PseudoCommandExec(CBKToken *token, int &cp, wchar_t &ch)
{
	g_Globals.SetOperandType(OPERAND_TYPE::DST);

	for (auto &cmd : g_PseudoCommands)
	{
		if (g_Globals.isInScript() && !cmd.bInScript)
		{
			// при обработке скрипта только разрешённые псевдокоманды
			continue;
		}

		if (token->calcHash(cmd.strName) == token->getHash()) // если нашли подходящую псевдокоманду
		{
#ifdef _DEBUG

			if (cmd.strName != token->getName())
			{
				wprintf(L"HASH Error: %s:%#zX ~~ %s:%#zX\n", cmd.strName.c_str(), token->calcHash(cmd.strName), token->getName().c_str(), token->getHash());
				assert(false);
			}

#endif

			// если команда допускает только чётные адреса
			if (cmd.bEvenAddr)
			{
				// нужна доп проверка на чётность адреса
				if (cp & 1)
				{
					ErrorManager::OutError(ERRNUM::E_117); // Нечётный адрес команды.
					g_Memory.b[cp++] = 0; // выравниваем до чётного
					return false;
				}
			}

			g_pReader->SkipWhitespaces(ch);
			g_Memory.w[cp / 2 + 1] = 0;
			g_Memory.w[cp / 2 + 2] = 0;
			return cmd.PSCFunction(cp, ch);
		}
	}

	ErrorManager::OutError(ERRNUM::E_105); // Неправильная псевдокоманда.
	return false;
}


bool Assemble::PSC_addr(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(cp, ListType::LT_PSC_ADDR);
	g_Memory.w[cp / 2] = 0;

	if (Parser::ReadRegName(ch))
	{
		uint16_t t = g_Memory.w[cp / 2];  // t - номер регистра
		g_Memory.w[cp / 2] |= 010700; // формируем 1070R
		cp += 2;
		g_Memory.w[cp / 2] = 062700 | t; // формируем 6270R
		cp += 2;

		if (Parser::needChar(L',', ch))
		{
			ch = g_pReader->readChar(true);
			int result;

			if (g_RPNParser.FullAriphmParser(ch)) // если арифметическое выражение распарсилось
			{
				if (g_RPNParser.GetNamesNum() || g_RPNParser.GetLocNamesNum()) // если там были имена меток или других определений
				{
					// в таблицу ссылок добавим невычисленную цепочку выражения
					LabelManager::AddLabelReference(nullptr, cp, ARL_CMDARG | ARL_OFFSET_LABEL);
					g_Memory.w[cp / 2] = 4;
					cp += 2;
					return true;
				}

				if (g_RPNParser.CalcRpn(result)) // были только числа - вычисляем.
				{
					g_Memory.w[cp / 2] = result + 4 - (g_Globals.GetRealAddress(cp) + 2);
					cp += 2; // формируем смещение
					return true;
				}

				assert(false); // такого быть не должно, надо разбираться.
			}
		}

		cp += 2;
		ErrorManager::OutError(ERRNUM::E_124); // Ошибка в псевдокоманде.
		return false;
	}

	ErrorManager::OutError(ERRNUM::E_111); // Ошибка в имени регистра.
	return false;
}

bool Assemble::PSC_la(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(cp, ListType::LT_PSC_LA);

	if (cp == BASE_ADDRESS)
	{
		int result = 0;
		g_Globals.SetAriphmType(ARIPHM_NOLOCLABEL | ARIPHM_NOUNDLABEL);

		if (g_RPNParser.FullAriphmParser(ch)) // если арифметическое выражение распарсилось
		{
			if (g_RPNParser.CalcRpn(result))
			{
				g_Globals.SetStartAddress(result);
				return true;
			}
		}

		return false;
	}

	ErrorManager::OutError(ERRNUM::E_101); // Псевдокоманда .LA должна быть первой в тексте.
	return false;
}

bool Assemble::PSC_print(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(cp, ListType::LT_PSC_PRINT);

	if (ch == L'#')
	{
		ch = g_pReader->readChar();

		if (assemble1OP(cp, ch))
		{
			g_Memory.w[cp / 2 - 1] = 012701; cp += 2;
			g_Memory.w[cp / 2] = 05002; cp += 2;
			g_Memory.w[cp / 2] = 0104020; cp += 2;
			return true;
		}
	}

	// выполним условие - что на выходе cp должен указывать на адрес за командой
	cp += 8; g_Memory.w[cp / 2 - 1] = 0;
	ErrorManager::OutError(ERRNUM::E_112); // Ошибка в псевдокоманде.
	return false;
}

bool Assemble::PSC_blkw(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(cp, ListType::LT_PSC_BLKW);
	int result = 0;
	g_Globals.SetAriphmType(ARIPHM_NOUNDLABEL);

	if (g_RPNParser.FullAriphmParser(ch)) // если арифметическое выражение распарсилось
	{
		if (g_RPNParser.CalcRpn(result))
		{
			result *= 2;

			if (cp + result >= HIGH_BOUND)
			{
				ErrorManager::OutError(ERRNUM::E_115); // Ошибка в псевдокоманде.
				return false;
			}

			for (int i = 0; i < result; ++i)
			{
				g_Memory.b[cp++] = 0;
			}

			return true;
		}
	}

	// здесь, при ошибке, условие - что на выходе cp должен указывать на адрес за командой, невыполнимо
	ErrorManager::OutError(ERRNUM::E_112); // Ошибка в псевдокоманде.
	return false;
}

bool Assemble::PSC_blkb(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(cp, ListType::LT_PSC_BLKB);
	int result = 0;
	g_Globals.SetAriphmType(ARIPHM_NOUNDLABEL);

	if (g_RPNParser.FullAriphmParser(ch)) // если арифметическое выражение распарсилось
	{
		if (g_RPNParser.CalcRpn(result))
		{
			if (cp + result >= HIGH_BOUND)
			{
				ErrorManager::OutError(ERRNUM::E_114); // Ошибка в псевдокоманде.
				return false;
			}

			for (int i = 0; i < result; ++i)
			{
				g_Memory.b[cp++] = 0;
			}

			return true;
		}
	}

	// здесь, при ошибке, условие - что на выходе cp должен указывать на адрес за командой, невыполнимо
	ErrorManager::OutError(ERRNUM::E_112); // Ошибка в псевдокоманде.
	return false;
}

// .ORG n - выравнивание по адресу. !!! Эта команда не работает в режиме CL
bool Assemble::PSC_org(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(cp, ListType::LT_PSC_LA);
	int result = 0;
	g_Globals.SetAriphmType(ARIPHM_NOUNDLABEL);

	if (g_RPNParser.FullAriphmParser(ch)) // если арифметическое выражение распарсилось
	{
		if (g_RPNParser.CalcRpn(result))
		{
			result = g_Globals.CorrectOffset(result);

			if (cp + result >= HIGH_BOUND)
			{
				ErrorManager::OutError(ERRNUM::E_116); // Ошибка в псевдокоманде.
				return false;
			}

			while (cp < result)         // выравниваем адрес до аргумента
			{
				g_Memory.b[cp++] = 0;
			}

			return true;
		}
	}

	// здесь, при ошибке, условие - что на выходе cp должен указывать на адрес за командой, невыполнимо
	ErrorManager::OutError(ERRNUM::E_112); // Ошибка в псевдокоманде.
	return false;
}

bool Assemble::PSC_flt2(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(cp, ListType::LT_PSC_FLT2);
	uint16_t flt[2];

	for (;;)
	{
		if (Parser::parse_float(ch, 2, flt))
		{
			g_Memory.w[cp / 2] = flt[0]; cp += 2;
			g_Memory.w[cp / 2] = flt[1]; cp += 2;
		}
		else
		{
			ErrorManager::OutError(ERRNUM::E_128);// ошибка парсинга flt2
			return false;
		}

		if (!Parser::needChar(L',', ch))
		{
			break;
		}

		ch = g_pReader->readChar(true);
	}

	return true;
}

bool Assemble::PSC_flt4(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(cp, ListType::LT_PSC_FLT4);
	uint16_t flt[4];

	for (;;)
	{
		if (Parser::parse_float(ch, 4, flt))
		{
			g_Memory.w[cp / 2] = flt[0]; cp += 2;
			g_Memory.w[cp / 2] = flt[1]; cp += 2;
			g_Memory.w[cp / 2] = flt[2]; cp += 2;
			g_Memory.w[cp / 2] = flt[3]; cp += 2;
		}
		else
		{
			ErrorManager::OutError(ERRNUM::E_128);// ошибка парсинга flt4
			return false;
		}

		if (!Parser::needChar(L',', ch))
		{
			break;
		}

		ch = g_pReader->readChar(true);
	}

	return true;
}

bool Assemble::PSC_word(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(cp, ListType::LT_PSC_WORD);
	bool bError = false;

	for (;;)
	{
		uint32_t nOffsFl = ARL_RELATIVE_LABEL;
		int result = 0;

		if (ch == L'@')
		{
			ch = g_pReader->readChar(); // пропустим "@"
			nOffsFl = ARL_OFFSET_LABEL;
		}

		g_Memory.w[cp / 2] = 0; // заранее обнулим

		if (g_RPNParser.FullAriphmParser(ch)) // если распарсилось
		{
			if (g_RPNParser.GetNamesNum() || g_RPNParser.GetLocNamesNum()) // если там были имена меток или других определений
			{
				// в таблицу ссылок добавим невычисленную цепочку выражения
				LabelManager::AddLabelReference(nullptr, cp, ARL_CMDARG | nOffsFl);

				if (nOffsFl == ARL_OFFSET_LABEL)
				{
					g_Memory.w[cp / 2] = 2;
				}
			}
			else if (g_RPNParser.CalcRpn(result)) // были только числа - вычисляем.
			{
				g_Memory.w[cp / 2] = (nOffsFl == ARL_OFFSET_LABEL) ? result - g_Globals.GetRealAddress(cp) : result;
			}
			else
			{
				assert(false); // такого быть не должно, надо разбираться.
				bError = true;
			}
		}
		else
		{
			bError = true;
		}

		cp += 2;    // двигаем указатель

		// если ошибка или нет продолжения - выход из цикла
		if (bError || !Parser::needChar(L',', ch))
		{
			break;
		}

		ch = g_pReader->readChar(true); // пропускаем запятую
		g_Globals.SetPC(cp); // устанавливаем новый PC для "точки"
	}

	return !bError;
}

bool Assemble::PSC_byte(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(cp, ListType::LT_PSC_BYTE);
	bool bError = false;

	for (;;)
	{
		int result = 0;
		g_Memory.b[cp] = 0;

		if (g_RPNParser.FullAriphmParser(ch)) // если распарсилось
		{
			if (g_RPNParser.GetNamesNum() || g_RPNParser.GetLocNamesNum()) // если там были имена меток или других определений
			{
				// в таблицу ссылок добавим невычисленную цепочку выражения
				LabelManager::AddLabelReference(nullptr, cp, ARL_CMDARG | ARL_RELATIVE_LABEL | ARL_BYTEL);
			}
			else if (g_RPNParser.CalcRpn(result)) // были только числа - вычисляем.
			{
				if (std::abs(result) > 0377)
				{
					ErrorManager::OutError(ERRNUM::E_125); // Переполнение байтового аргумента.
					//break; // это будем считать предупреждением, прерывать работу не будем
				}

				g_Memory.b[cp] = uint8_t(result & 0xff);
			}
			else
			{
				assert(false); // такого быть не должно, надо разбираться.
				bError = true;
			}
		}
		else
		{
			bError = true;
		}

		cp++; // двигаем указатель

		// если ошибка или нет продолжения - выход из цикла
		if (bError || !Parser::needChar(L',', ch))
		{
			break;
		}

		ch = g_pReader->readChar(true);
		g_Globals.SetPC(cp); // устанавливаем новый PC для "точки"
	}

	return !bError;
}

bool Assemble::PSC_end(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(cp, ListType::LT_PSC_END);
	g_Globals.SetENDReached(true);

	if (g_Globals.isInInclude())
	{
		g_Globals.SetInInclude(false);
	}

	g_Globals.SetProgramLength(cp - BASE_ADDRESS);

	if (g_Globals.GetLinkMode() != LINKING_MODE::CL)
	{
		g_Globals.SetLinkMode(LINKING_MODE::FINAL);
	}

	LabelManager::LabelLinking();
	g_labelLocalDefs.Clear();
	return true;
}

bool Assemble::PSC_even(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(cp, ListType::LT_PSC_EVEN);

	if (cp & 1)
	{
		g_Memory.b[cp++] = 0;
	}

	return true;
}

bool Assemble::StrQuotes(wchar_t ch)
{
	// первым квотируемым символом считаем любой символ пунктуации и т.п., забыл как группа этих символов называется
	// вообще, по правилам MACRO, квотируемым считается вообще любой символ, кроме <, но это уж как-то слишком
	// усложняет понимание вида текстовой строки, поэтому ограничим себя
	if ((L'!' <= ch && ch <= L'/') || (ch == L':' || ch == L'=' || ch == L'?' || ch == L'@') || (0133 <= ch && ch <= 0137) || (0173 <= ch && ch <= 0177))
	{
		return true;
	}

	return false;
}

bool Assemble::PSC_rad50(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(cp, ListType::LT_PSC_RAD50);

	for (;;)
	{
		uint16_t w = 0; // текущее слово в radix50
		int nCnt = 0; // счётчик символов в слове
		int nMultipler = 03100;

		if (StrQuotes(ch))
		{
			wchar_t chQuoter = ch;
			g_Globals.SetInString(true);

			for (;;)
			{
				ch = g_pReader->readChar(); // берём очередной символ

				if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::LN)
				{
					ErrorManager::OutError(ERRNUM::E_127); // Неожиданный конец строкового аргумента.
					return false;
				}

				if (ch == chQuoter) // строка закончилась
				{
					if (nCnt) // если что-то было ещё принято
					{
						g_Memory.w[cp / 2] = w; // сохраним
						cp += 2;
					}

					ch = g_pReader->readChar(true); // пропускаем закрывающую кавычку
					break;
				}

				// теперь поищем символ в таблице
				uint16_t nChCode = 0;   // код текущего символа по умолчанию, если символ не будет найден в таблице, будет значение по умолчанию
				ch = std::towupper(ch);

				for (int n = 0; n < 050; ++n)
				{
					if (RADIX50[n] == ch)
					{
						nChCode = n;    // нашли
						break;
					}
				}

				w += nChCode * nMultipler; // упаковываем очередной символ
				nMultipler /= 050;

				if (++nCnt >= 3)    // если пора сохранять полное слово
				{
					nCnt = 0; nMultipler = 03100;
					g_Memory.w[cp / 2] = w; // сохраним
					cp += 2;
					w = 0;
				}
			}

			g_Globals.SetInString(false);
		}
		else if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::LN)
		{
			// если конец строки - выходим
			break;
		}
		else if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::SPACES && g_pReader->SkipWhitespaces(ch))
		{
			// если встретили пробел и пока пропускали пробелы дошли до конца строки - выходим
			break;
		}
		else if (CheckComment(ch))
		{
			// если пропускали пробелы и наткнулись на комментарий - то выходим, его тут не обработать
			break;
		}
		else
		{
			ErrorManager::OutError(ERRNUM::E_112); // Ошибка в псевдокоманде.
			return false;
		}
	}

	return true;
}

bool Assemble::PSC_ascii(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(cp, ListType::LT_PSC_ASCII);
	g_Globals.SetAriphmType(ARIPHM_NOBRACKETS | ARIPHM_NOLOCLABEL);

	for (;;)
	{
		if (StrQuotes(ch)) // если квотируемый символ, считаем что это кавычка
		{
			// и принимаем строку
			wchar_t chQuoter = ch;
			g_Globals.SetInString(true);

			for (;;)
			{
				ch = g_pReader->readChar();

				if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::LN)
				{
					ErrorManager::OutError(ERRNUM::E_127); // Неожиданный конец строкового аргумента.
					return false;
				}

				if (ch == chQuoter)
				{
					ch = g_pReader->readChar(true); // пропускаем закрывающую кавычку
					break;
				}

				g_Memory.b[cp++] = UNICODEtoBK_ch(ch);
			}

			g_Globals.SetInString(false);
		}
		else if (ch == L'<') // если левая скобка, то принимаем число
		{
			ch = g_pReader->readChar(true); // пропустим скобку
			int result = 0;
			bool bRes = false;

			if (g_RPNParser.FullAriphmParser(ch))
			{
				bRes = g_RPNParser.CalcRpn(result);

				if (!bRes) // если там были имена меток или других определений
				{
					// в таблицу ссылок добавим невычисленную цепочку выражения
					LabelManager::AddLabelReference(nullptr, cp, ARL_CMDARG | ARL_RELATIVE_LABEL | ARL_BYTEL);
					result = 0;
				}
			}

			if (g_pReader->SkipWhitespaces(ch)) // пропускаем пробелы
			{
				//если внезапный конец строки
				g_Memory.b[cp++] = 0;
				ErrorManager::OutError(ERRNUM::E_130); // Ошибка в аргументах псевдокоманды.
				return false;
			}

			if (ch == L'>') // если нашлась закрывающая скобка
			{
				ch = g_pReader->readChar(true); // пропустим скобку

				if (bRes && (std::abs(result) <= 0377)) // если аргумент верный
				{
					g_Memory.b[cp++] = uint8_t(result & 0xff); // то его сохраняем
				}
				else
				{
					g_Memory.b[cp++] = 0;
					ErrorManager::OutError(ERRNUM::E_125); // Переполнение байтового аргумента.
					return false;
				}
			}
			else
			{
				g_Memory.b[cp++] = 0;
				ErrorManager::OutError(ERRNUM::E_130); // Ошибка в аргументах псевдокоманды.
				return false;
			}
		}
		else if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::LN)    // если конец строки - то выходим
		{
			break;
		}
		else if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::SPACES && g_pReader->SkipWhitespaces(ch))
		{
			// если пробел и пока пропускали пробелы наткнулись на конец строки
			break; // то выходим
		}
		else if (CheckComment(ch))
		{
			// если пропускали пробелы и наткнулись на комментарий - то выходим, его тут не обработать
			break;
		}
		else
		{
			ErrorManager::OutError(ERRNUM::E_112); // Ошибка в псевдокоманде.
			return false;
		}
	}

	return true;
}

bool Assemble::PSC_asciz(int &cp, wchar_t &ch)
{
	bool bRet = PSC_ascii(cp, ch);
	g_Memory.b[cp++] = 0;
	return bRet;
}

// проверка комментария, выход:
// true - встретили комментарий,
// false - не комментарий, а что-то другое
bool Assemble::CheckComment(wchar_t &ch)
{
	if (ch == L';' ||
	        (ch == L'/' && (g_pReader->getNextChar() == L'/' || g_pReader->getNextChar() == L'*')))
	{
		return true;
	}

	return false;
}


// проверка длины перехода по ветвлению
// вход: nTargetAddr - адрес метки
//      nCommandAddr - адрес команды. откуда переход
//  важное условие - в команде уже подставлено значение
// выход: nTargetAddr - корректное смещение
//      true - всё нормально
//      false - ошибка длины переход
bool Assemble::BranchVerify(int nCommandAddr, int &nTargetAddr)
{
	nTargetAddr -= (nCommandAddr + 2);
	nTargetAddr /= 2;

	if (nTargetAddr >= 0)
	{
		if (nTargetAddr < 0200)
		{
			return true;
		}
	}
	else
	{
		nTargetAddr = -nTargetAddr;

		if (nTargetAddr <= 0200)
		{
			nTargetAddr -= 0400;
			nTargetAddr = -nTargetAddr;
			return true;
		}
	}

	return false;
}

bool Assemble::PSC_enabl(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(0, ListType::LT_COMMENTARY);
	g_Globals.SetEnabl(true);
	return true;
}

bool Assemble::PSC_dsabl(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(0, ListType::LT_COMMENTARY);
	g_Globals.SetEnabl(false);
	return true;
}

// выполнение .include filename
bool Assemble::PSC_Include(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(0, ListType::LT_COMMENTARY);
	std::wstring strFileName;

	if (!GetName(strFileName, ch))
	{
		// если есть открывающая кавычка, но нет закрывающей - ошибка
		ErrorManager::OutError(ERRNUM::E_127);
		return false;
	}

	if (!strFileName.empty())
	{
		auto pReader = new CReader(strFileName, g_Globals.GetCharset());

		if (pReader)
		{
			if (pReader->GetFileCharset() == CReader::FILE_CHARSET::FILEERROR)
			{
				delete pReader;
				ErrorManager::OutError(ERRNUM::E_129); // Ошибка в псевдокоманде.
				return false;
			}

			if (g_Globals.FindReader(strFileName)) // если уже есть среди добавленных
			{
				delete pReader; // не будем ещё раз добавлять
			}
			else
			{
				// добавляем
				g_Globals.PushReader(g_pReader);
				g_pReader = pReader;
				return true;
			}
		}
	}
	else
	{
		// если нет имени файла - ошибка
		ErrorManager::OutError(ERRNUM::E_112);
	}

	return false;
}


// приём имени - аргумента псевдокоманды.
// выход: strName - принятое имя.
// выход: true - имя ОК, не важно чем закончилось
//        false - не ОК - есть открывающая кавычка, но нет закрывающей

bool Assemble::GetName(std::wstring &strName, wchar_t &ch)
{
	g_Globals.SetInString(true);
	wchar_t chQuoter = 0;

	if (StrQuotes(ch)) // если квотируемый символ, считаем что это кавычка
	{
		chQuoter = ch;
		ch = g_pReader->readChar();
	}

	// и принимаем строку
	for (;;)
	{
		if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::LN)
		{
			break;
		}

		if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::SPACES && chQuoter == 0)
		{
			break; // если имя не в кавычках и встретили пробел - то конец имени
		}

		if (CheckComment(ch))  // если комментарий - то выходим, его тут не обработать
		{
			break;
		}

		if (ch == chQuoter)
		{
			ch = g_pReader->readChar();
			chQuoter = 0;
			break;
		}

		strName.push_back(ch);
		ch = g_pReader->readChar();
	}

	g_Globals.SetInString(false);

	if (chQuoter)
	{
		// если есть открывающая кавычка, но нет закрывающей - ошибка
		return false;
	}

	return true;
}

bool Assemble::PSC_Script(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(0, ListType::LT_COMMENTARY);
	std::wstring strName;

	if (!GetName(strName, ch))
	{
		// если есть открывающая кавычка, но нет закрывающей - ошибка
		ErrorManager::OutError(ERRNUM::E_127);
		return false;
	}

	g_Globals.SetInScript(true);
	g_ScriptAsm.Init(strName);
	return true;
}

bool Assemble::PSC_EndScript(int &cp, wchar_t &ch)
{
	ListingManager::AddPrepareLine(0, ListType::LT_COMMENTARY);

	if (g_Globals.isInScript())
	{
		g_Globals.SetInScript(false);
		g_ScriptAsm.PushScript();
		return true;
	}

	ErrorManager::OutError(ERRNUM::E_133);
	return false;
}


