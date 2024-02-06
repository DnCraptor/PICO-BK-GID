// DebuggerAssembly.cpp: продолжение implementation of the CDebugger class.
//

#include "pch.h"
#include "Debugger.h"
#ifdef DEBUGGER
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

const std::vector <CDebugger::CPUCommandStruct> CDebugger::m_pCPUCommands =
{
	{ L"HALT",   0000000,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR }, // op
	{ L"WAIT",   0000001,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"RTI",    0000002,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"BPT",    0000003,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"IOT",    0000004,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"RESET",  0000005,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"RTT",    0000006,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"START",  0000012,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"STEP",   0000016,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"NOP",    0000240,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"CLC",    0000241,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"CLV",    0000242,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"CLVC",   0000243,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"CLZ",    0000244,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"CLZC",   0000245,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"CLZV",   0000246,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"CLZVC",  0000247,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"CLN",    0000250,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"CLNC",   0000251,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"CLNV",   0000252,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"CLNVC",  0000253,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"CLNZ",   0000254,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"CLNZC",  0000255,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"CLNZV",  0000256,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"CCC",    0000257,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"SEC",    0000261,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"SEV",    0000262,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"SEVC",   0000263,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"SEZ",    0000264,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"SEZC",   0000265,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"SEZV",   0000266,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"SEZVC",  0000267,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"SEN",    0000270,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"SENC",   0000271,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"SENV",   0000272,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"SENVC",  0000273,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"SENZ",   0000274,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"SENZC",  0000275,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"SENZV",  0000276,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"SCC",    0000277,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"RET",    0000207,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },
	{ L"RETURN", 0000207,    CPUCmdGroup::NOOPS, OpcodeType::REGULAR },

	{ L"BR",     0000400,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR }, // op lll
	{ L"BNE",    0001000,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR },
	{ L"BEQ",    0001400,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR },
	{ L"BGE",    0002000,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR },
	{ L"BLT",    0002400,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR },
	{ L"BGT",    0003000,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR },
	{ L"BLE",    0003400,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR },
	{ L"BPL",    0100000,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR },
	{ L"BMI",    0100400,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR },
	{ L"BHI",    0101000,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR },
	{ L"BLOS",   0101400,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR },
	{ L"BVC",    0102000,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR },
	{ L"BVS",    0102400,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR },
	{ L"BCC",    0103000,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR },
	{ L"BCS",    0103400,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR },
	{ L"BHIS",   0103000,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR },
	{ L"BLO",    0103400,    CPUCmdGroup::CBRANCH, OpcodeType::REGULAR },

	{ L"MUL",    0070000,    CPUCmdGroup::EIS, OpcodeType::EIS }, // op ss,r
	{ L"DIV",    0071000,    CPUCmdGroup::EIS, OpcodeType::EIS },
	{ L"ASH",    0072000,    CPUCmdGroup::EIS, OpcodeType::EIS },
	{ L"ASHC",   0073000,    CPUCmdGroup::EIS, OpcodeType::EIS },

	{ L"EMT",    0104000,    CPUCmdGroup::TRAP, OpcodeType::REGULAR }, // op nnn
	{ L"TRAP",   0104400,    CPUCmdGroup::TRAP, OpcodeType::REGULAR },

	{ L"SOB",    0077000,    CPUCmdGroup::SOB, OpcodeType::REGULAR }, // op r,ll

	{ L"MARK",   0006400,    CPUCmdGroup::MARK, OpcodeType::REGULAR }, // op nn

	{ L"JSR",    0004000,    CPUCmdGroup::TWOOPREG, OpcodeType::REGULAR }, // op r,dd
	{ L"XOR",    0074000,    CPUCmdGroup::TWOOPREG, OpcodeType::REGULAR },

	{ L"FADD",   0075000,    CPUCmdGroup::FIS, OpcodeType::FIS }, // op r
	{ L"FSUB",   0075010,    CPUCmdGroup::FIS, OpcodeType::FIS },
	{ L"FMUL",   0075020,    CPUCmdGroup::FIS, OpcodeType::FIS },
	{ L"FDIV",   0075030,    CPUCmdGroup::FIS, OpcodeType::FIS },
	{ L"RTS",    0000200,    CPUCmdGroup::FIS, OpcodeType::FIS },

	{ L"PUSH",   0010046,    CPUCmdGroup::PUSH, OpcodeType::REGULAR }, // op ss
	{ L"PUSHB",  0110046,    CPUCmdGroup::PUSH, OpcodeType::REGULAR },

	{ L"JMP",    0000100,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR }, // op dd
	{ L"SWAB",   0000300,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"MTPS",   0106400,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"MFPS",   0106700,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"MTPD",   0106600,    CPUCmdGroup::ONEOPS, OpcodeType::MMG },
	{ L"MFPD",   0106500,    CPUCmdGroup::ONEOPS, OpcodeType::MMG },
	{ L"MTPI",   0006600,    CPUCmdGroup::ONEOPS, OpcodeType::MMG },
	{ L"MFPI",   0006500,    CPUCmdGroup::ONEOPS, OpcodeType::MMG },
	{ L"SXT",    0006700,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"CALL",   0004700,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"POP",    0012600,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"POPB",   0112600,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"CLR",    0005000,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"COM",    0005100,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"INC",    0005200,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"DEC",    0005300,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"NEG",    0005400,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"ADC",    0005500,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"SBC",    0005600,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"TST",    0005700,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"ROR",    0006000,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"ROL",    0006100,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"ASR",    0006200,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"ASL",    0006300,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"CLRB",   0105000,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"COMB",   0105100,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"INCB",   0105200,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"DECB",   0105300,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"NEGB",   0105400,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"ADCB",   0105500,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"SBCB",   0105600,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"TSTB",   0105700,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"RORB",   0106000,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"ROLB",   0106100,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"ASRB",   0106200,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },
	{ L"ASLB",   0106300,    CPUCmdGroup::ONEOPS, OpcodeType::REGULAR },

	{ L"MOV",    0010000,    CPUCmdGroup::TWOOPS, OpcodeType::REGULAR }, // op ss, dd
	{ L"CMP",    0020000,    CPUCmdGroup::TWOOPS, OpcodeType::REGULAR },
	{ L"BIT",    0030000,    CPUCmdGroup::TWOOPS, OpcodeType::REGULAR },
	{ L"BIC",    0040000,    CPUCmdGroup::TWOOPS, OpcodeType::REGULAR },
	{ L"BIS",    0050000,    CPUCmdGroup::TWOOPS, OpcodeType::REGULAR },
	{ L"MOVB",   0110000,    CPUCmdGroup::TWOOPS, OpcodeType::REGULAR },
	{ L"CMPB",   0120000,    CPUCmdGroup::TWOOPS, OpcodeType::REGULAR },
	{ L"BITB",   0130000,    CPUCmdGroup::TWOOPS, OpcodeType::REGULAR },
	{ L"BICB",   0140000,    CPUCmdGroup::TWOOPS, OpcodeType::REGULAR },
	{ L"BISB",   0150000,    CPUCmdGroup::TWOOPS, OpcodeType::REGULAR },
	{ L"ADD",    0060000,    CPUCmdGroup::TWOOPS, OpcodeType::REGULAR },
	{ L"SUB",    0160000,    CPUCmdGroup::TWOOPS, OpcodeType::REGULAR },
	// FPU инструкции.
	{ L"CFCC",   0170000,    CPUCmdGroup::NOOPS, OpcodeType::FPU }, // класс 0 - без операндов
	{ L"SETF",   0170001,    CPUCmdGroup::NOOPS, OpcodeType::FPU }, // op
	{ L"SETI",   0170002,    CPUCmdGroup::NOOPS, OpcodeType::FPU },
	{ L"SETD",   0170011,    CPUCmdGroup::NOOPS, OpcodeType::FPU },
	{ L"SETL",   0170012,    CPUCmdGroup::NOOPS, OpcodeType::FPU },

	{ L"LDFPS",  0170100,    CPUCmdGroup::ONEOPS, OpcodeType::FPU }, // класс 1 - с одним операндом
	{ L"STFPS",  0170200,    CPUCmdGroup::ONEOPS, OpcodeType::FPU },
	{ L"STST",   0170300,    CPUCmdGroup::ONEOPS, OpcodeType::FPU },
	{ L"CLRD",   0170400,    CPUCmdGroup::FPUCLASS1, OpcodeType::FPU },
	{ L"CLRF",   0170400,    CPUCmdGroup::FPUCLASS1, OpcodeType::FPU },
	{ L"TSTD",   0170500,    CPUCmdGroup::FPUCLASS1, OpcodeType::FPU },
	{ L"TSTF",   0170500,    CPUCmdGroup::FPUCLASS1, OpcodeType::FPU },
	{ L"ABSD",   0170600,    CPUCmdGroup::FPUCLASS1, OpcodeType::FPU },
	{ L"ABSF",   0170600,    CPUCmdGroup::FPUCLASS1, OpcodeType::FPU },
	{ L"NEGD",   0170700,    CPUCmdGroup::FPUCLASS1, OpcodeType::FPU },
	{ L"NEGF",   0170700,    CPUCmdGroup::FPUCLASS1, OpcodeType::FPU },

	{ L"MULD",   0171000,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU }, // класс 2 - с двумя операндами
	{ L"MULF",   0171000,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"MODD",   0171400,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"MODF",   0171400,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"ADDD",   0172000,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"ADDF",   0172000,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"LDD",    0172400,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"LDF",    0172400,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"SUBD",   0173000,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"SUBF",   0173000,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"CMPD",   0173400,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"CMPF",   0173400,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"STD",    0174000,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"STF",    0174000,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"DIVD",   0174400,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"DIVF",   0174400,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"STEXP",  0175000,    CPUCmdGroup::FPUCLASS2_1, OpcodeType::FPU },
	{ L"STCDI",  0175400,    CPUCmdGroup::FPUCLASS2_1, OpcodeType::FPU },
	{ L"STCDL",  0175400,    CPUCmdGroup::FPUCLASS2_1, OpcodeType::FPU },
	{ L"STCFI",  0175400,    CPUCmdGroup::FPUCLASS2_1, OpcodeType::FPU },
	{ L"STCFL",  0175400,    CPUCmdGroup::FPUCLASS2_1, OpcodeType::FPU },
	{ L"STCDF",  0176000,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"STCFD",  0176000,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"LDEXP",  0176400,    CPUCmdGroup::FPUCLASS2_1, OpcodeType::FPU },
	{ L"LDCID",  0177000,    CPUCmdGroup::FPUCLASS2_1, OpcodeType::FPU },
	{ L"LDCLD",  0177000,    CPUCmdGroup::FPUCLASS2_1, OpcodeType::FPU },
	{ L"LDCIF",  0177000,    CPUCmdGroup::FPUCLASS2_1, OpcodeType::FPU },
	{ L"LDCLF",  0177000,    CPUCmdGroup::FPUCLASS2_1, OpcodeType::FPU },
	{ L"LDCFD",  0177400,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU },
	{ L"LDCDF",  0177400,    CPUCmdGroup::FPUCLASS2, OpcodeType::FPU }
};

const std::vector <CDebugger::Registers> CDebugger::m_pCPURegs =
{
	{_T("R0"), 0},
	{_T("R1"), 1},
	{_T("R2"), 2},
	{_T("R3"), 3},
	{_T("R4"), 4},
	{_T("R5"), 5},
	{_T("R6"), 6},
	{_T("R7"), 7},
	{_T("SP"), 6},
	{_T("PC"), 7}
};

const std::vector <CDebugger::Registers> CDebugger::m_pFPURegs =
{
	{_T("AC0"), 0},
	{_T("AC1"), 1},
	{_T("AC2"), 2},
	{_T("AC3"), 3},
	{_T("AC4"), 4},
	{_T("AC5"), 5},
	{_T("AC6"), 6},
	{_T("AC7"), 7}
};


// пропуск пробелов
// Вход: pch - указатель на текущий символ в ассемблируемой строке.
// выход: true - нашли непробельный символ
//        false - конец строки
bool CDebugger::SkipWhitespace(TCHAR **pch)
{
	while (**pch)
	{
		if (**pch > 040)
		{
			return true;
		}

		(*pch)++;
	}

	return false;
}

// чтение лексемы
// Вход: str - строка, формируемой лексемы
//       pch - указатель на текущий символ в ассемблируемой строке.
//       bNoSkip - true - не пропускать пробелы перед чтением, false - пропускать
// выход: true - Ok
//        false - конец строки
bool CDebugger::ReadToken(CString &str, TCHAR **pch, bool bNoSkip)
{
	if (bNoSkip || SkipWhitespace(pch))
	{
		bool bFirstSymbol = true;
		str.Empty();

		while (**pch)
		{
			// лексемой считается последовательность букв и цифр.
			// начинающихся с буквы.
			TCHAR ch = **pch;

			if ((_T('A') <= ch && ch <= _T('Z')) || (_T('a') <= ch && ch <= _T('z')))
			{
				str += ch;
			}
			else if (!bFirstSymbol && (_T('0') <= ch && ch <= _T('9')))
			{
				str += ch;
			}
			else
			{
				break;
			}

			bFirstSymbol = false;
			(*pch)++;
		}

		str.MakeUpper();
		return true;
	}

	return false;
}

// чтение восьмеричного числа
// Вход: str - строка, формируемого числа
//       pch - указатель на текущий символ в ассемблируемой строке.
//       bNoSkip - true - не пропускать пробелы перед чтением, false - пропускать
// выход: true - Ok
//        false - конец строки
bool CDebugger::ReadOctalNumber(int &num, TCHAR **pch, bool bNoSkip)
{
	num = 0;

	if (bNoSkip || SkipWhitespace(pch))
	{
		CString str;

		while (**pch)
		{
			// числом считается последовательность цифр 0..7.
			TCHAR ch = **pch;

			if ((_T('0') <= ch && ch <= _T('7')))
			{
				str += ch;
			}
			else
			{
				break;
			}

			(*pch)++;
		}

		num = _tcstol(str, nullptr, 8);
		return true;
	}

	return false;
}

bool CDebugger::isEoln(TCHAR **pch)
{
	return !(**pch);
}



// ищем ожидаемый символ.
// вход: ch - ожидаемый символ
//       pch - указатель на текущий символ в ассемблируемой строке.
//       bNoSkip - true - не пропускать пробелы перед чтением, false - пропускать
// выход: true - есть он там, указатель ставится за него
//      false - нету, или конец строки, указатель останавливается на любом непробельном символе
//      или на конце строки.
bool CDebugger::NeedChar(TCHAR ch, TCHAR **pch, bool bNoSkip)
{
	if (bNoSkip || SkipWhitespace(pch))
	{
		if (ch == **pch)
		{
			(*pch)++;
			return true;
		}
	}

	return false;
}



// вход: pc - текущий адрес инструкции, нужен для расчёта смещений
//       buf - буфер, куда формируется ассемблерная команда. макс. 8 слов.
//       pStr - указатель на ассемблируемую строку.
//             строка должна оканчиваться нулём. Это обязательно.
// выход: длина кода в словах
//      -1, 0 - ошибка ассемблирования
int CDebugger::AssembleCPUInstruction(int pc, uint16_t *buf, CString *pStr)
{
	TCHAR *pch = pStr->GetBuffer();
	m_bOperandType = false; // считаем, что по умолчанию - приёмник

	// пропустим начальные пробелы
	if (!SkipWhitespace(&pch))
	{
		return -1; // если там ничего кроме пробелов нет - выходим
	}

	CString strMnem;
	bool bMinus = false;
	TCHAR ch = *pch; // первый символ

	if (ch == _T('.'))  // псевдокоманда .WORD
	{
		pch++;
		// получим мнемонику команды
		ReadToken(strMnem, &pch, false);

		if (strMnem == _T("WORD"))
		{
l_pscomWord:

			if (!SkipWhitespace(&pch))
			{
				return -1;
			}

			if (*pch == _T('-'))
			{
				bMinus = true;
				pch++; // передвигаемся дальше
			}

l_WordArg:
			int num = 0;

			if (ReadOctalNumber(num, &pch, true))
			{
				if (bMinus)
				{
					num = -num;
				}

				buf[0] = num & 0xffff;
				return 1;
			}
		}

		return -1;
	}

	if (ch == _T('-'))
	{
		// предположительно отрицательное число
		pch++;
		ch = *pch;
		bMinus = true;
	}

	if ((_T('0') <= ch && ch <= _T('7')))
	{
		// предположительно число.
		goto l_WordArg;
	}

	// если минус не перед числом - ошибка
	if (bMinus)
	{
		return -1;
	}

	// получим мнемонику команды
	ReadToken(strMnem, &pch, false);

	if (strMnem == _T("WORD"))
	{
		// псеводкдманда WORD без точки спереди
		goto l_pscomWord;
	}

// кроме ассемблерных команд можно задавать числа следующим образом:
//  .word xxx
//  word xxx (без точки)
//  xxx (просто число без всяких псевдокоманд)
//  допускается отрицательное число

	for (const auto &cmd : m_pCPUCommands) // пока не конец таблицы
	{
		if (!g_Config.m_bMMG && cmd.nType == OpcodeType::MMG)
		{
			// если выключены mmg инструкции, их игнорируем
			continue;
		}

		if (!g_Config.m_bEIS && cmd.nType == OpcodeType::EIS)
		{
			// если выключены eis инструкции, их игнорируем
			continue;
		}

		if (!g_Config.m_bFIS && cmd.nType == OpcodeType::FIS)
		{
			// если выключены fis инструкции, их игнорируем
			continue;
		}

		if (!g_Config.m_bFPU && cmd.nType == OpcodeType::FPU)
		{
			// если выключены fpu инструкции, их игнорируем
			continue;
		}

		if (cmd.strName == strMnem) // если нашли мнемонику
		{
			int nLen = 0; // длина команды в словах.
			// если нашли нужную мнемонику
			// нужно взять опкод.
			buf[nLen++] = cmd.nOpcode;
			buf[nLen]     = 0; // следующие два слова заранее обнулим
			buf[nLen + 1] = 0;
			pc += 2;
			bool bRet = true;

			// и обработать операнды в соответствии с типом группы, к которой принадлежит опкод.
			switch (cmd.nGroup)
			{
				case CPUCmdGroup::NOOPS:        // ничего делать не надо.
					break;

				case CPUCmdGroup::CBRANCH:
					bRet = assembleBR(pc, buf, nLen, &pch);
					break;

				case CPUCmdGroup::EIS:
					bRet = assemble2ROP(pc, buf, nLen, &pch);
					break;

				case CPUCmdGroup::TRAP:
					bRet = assembleTRAP(buf, nLen, &pch);
					break;

				case CPUCmdGroup::SOB:
					bRet = assembleSOB(pc, buf, nLen, &pch);
					break;

				case CPUCmdGroup::MARK:
					bRet = assembleMARK(buf, nLen, &pch);
					break;

				case CPUCmdGroup::TWOOPREG:
					bRet = assemble2OPR(pc, buf, nLen, &pch);
					break;

				case CPUCmdGroup::FIS:
					bRet = assemble1OPR(pc, buf, nLen, &pch);
					break;

				case CPUCmdGroup::PUSH:
					m_bOperandType = true; // меняем тип операнда
					[[fallthrough]];
				// и переходим к выполнению CPUCmdGroup::ONEOPS
				case CPUCmdGroup::ONEOPS:
					bRet = assemble1OP(pc, buf, nLen, &pch);
					break;

				case CPUCmdGroup::TWOOPS:
					bRet = assemble2OP(pc, buf, nLen, &pch);
					break;

				case CPUCmdGroup::FPUCLASS1:
					bRet = assemble1OPFPU(pc, buf, nLen, &pch);
					break;

				case CPUCmdGroup::FPUCLASS2:
					bRet = assemble2OPRFPU(pc, buf, nLen, &pch, true);
					break;

				case CPUCmdGroup::FPUCLASS2_1:
					bRet = assemble2OPRFPU(pc, buf, nLen, &pch, false);
					break;
			}

			if (bRet)
			{
				return nLen;
			}

			// ошибка в операндах.
			return 0;
		}
	}

	// ошибка - неопознанная команда.
	return -1;
}

bool CDebugger::assembleBR(int &pc, uint16_t *buf, int &len, TCHAR **pch)
{
	int dest = 0;

	if (ReadOctalNumber(dest, pch, false))
	{
		// теперь надо вычислить смещение.
		int offs = (dest - pc) / 2;

		if (offs >= 0) // для перехода вперёд
		{
			if (offs < 0200) // если укладываемся
			{
				buf[0] |= offs & 0xff; // то формируем опкод
				return true; // и выходим
			}
		}
		else // для перехода назад
		{
			if (-offs <= 0200)
			{
				buf[0] |= offs & 0xff;
				return true;
			}
		}
	}

	return false;
}

bool CDebugger::assembleTRAP(uint16_t *buf, int &len, TCHAR **pch)
{
	int num = 0;

	if (ReadOctalNumber(num, pch, false))
	{
		if (0 <= num && num < 0400)
		{
			buf[0] |= num & 0xff; // то формируем опкод
			return true; // и выходим
		}
	}

	return false;
}

bool CDebugger::assembleMARK(uint16_t *buf, int &len, TCHAR **pch)
{
	int num = 0;

	if (ReadOctalNumber(num, pch, false))
	{
		if (0 <= num && num < 077)
		{
			buf[0] |= num & 077; // то формируем опкод
			return true; // и выходим
		}
	}

	return false;
}

// сборка двухоперандных команд, где второй операнд - регистр
// Это EIS команды, у которых  при записи мнемоники почему-то поменяны местами
// источник и приёмник, как у x86
bool CDebugger::assemble2ROP(int &pc, uint16_t *buf, int &len, TCHAR **pch)
{
	m_bOperandType = false; // сперва dd

	if (assemble1OP(pc, buf, len, pch))   // обработаем первый операнд
	{
		if (NeedChar(_T(','), pch, false))    // проверим наличие второго
		{
			m_bOperandType = true; // теперь ss
			return ReadRegName(buf, pch, false);    // обработаем второй операнд
		}

		// Ошибка в команде - нет второго операнда.
		return false;
	}

	return false;
}

// сборка обычных однооперандных команд
bool CDebugger::assemble1OP(int &pc, uint16_t *buf, int &len, TCHAR **pch)
{
	return Operand_analyse(pc, buf, len, pch, false); // разбираемся, что там такое
}

bool CDebugger::assemble1OPFPU(int &pc, uint16_t *buf, int &len, TCHAR **pch)
{
	return Operand_analyse(pc, buf, len, pch, true); // разбираемся, что там такое
}


bool CDebugger::ReadRegName(uint16_t *buf, TCHAR **pch, bool bNoSkip)
{
	CString str;

	if (ReadToken(str, pch, bNoSkip))
	{
		for (const auto &reg : m_pCPURegs)
		{
			if (reg.name == str) // если нашли мнемонику
			{
				SetAddrReg(buf, reg.nNum);
				return true;
			}
		}
	}

	return false;
}

bool CDebugger::ReadFPURegName(uint16_t *buf, TCHAR **pch, bool bShort, bool bNoSkip)
{
	CString str;

	if (ReadToken(str, pch, bNoSkip))
	{
		int n = bShort ? 4 : 8;

		for (int i = 0; i < n; ++i)
		{
			if (m_pFPURegs[i].name == str) // если нашли мнемонику
			{
				SetAddrReg(buf, m_pFPURegs[i].nNum);
				return true;
			}
		}
	}

	return false;
}



void CDebugger::SetAddrReg(uint16_t *buf, uint16_t val)
{
	if (m_bOperandType)
	{
		val <<= 6;
	}

	buf[0] |= val;
}


// сборка однооперандных команд, где операнд - регистр
bool CDebugger::assemble1OPR(int &pc, uint16_t *buf, int &len, TCHAR **pch)
{
	return ReadRegName(buf, pch, false);
}

bool CDebugger::assemble1OPRFPU(int &pc, uint16_t *buf, int &len, TCHAR **pch)
{
	return ReadFPURegName(buf, pch, true, false);
}


// сборка двухоперандных команд
bool CDebugger::assemble2OP(int &pc, uint16_t *buf, int &len, TCHAR **pch)
{
	m_bOperandType = true; // сперва ss

	if (assemble1OP(pc, buf, len, pch))   // обработаем один операнд
	{
		if (NeedChar(_T(','), pch, false))  // проверим наличие второго
		{
			m_bOperandType = false; // теперь dd
			return assemble1OP(pc, buf, len, pch);     // обработаем второй операнд
		}

		// Ошибка в команде - нет второго операнда.
		return false;
	}

	return false;
}
// сборка двухоперандных команд, где первый операнд - регистр
bool CDebugger::assemble2OPR(int &pc, uint16_t *buf, int &len, TCHAR **pch)
{
	m_bOperandType = true; // сперва ss

	if (ReadRegName(buf, pch, false))  // обрабатываем регистр
	{
		if (NeedChar(_T(','), pch, false))    // проверим наличие второго
		{
			m_bOperandType = false; // теперь dd
			return assemble1OP(pc, buf, len, pch);     // обработаем второй операнд
		}

		// Ошибка в команде - нет второго операнда.
		return false;
	}

	return false;
}
// сборка двухоперандных команд FPU, где первый операнд - регистр AC0..AC3
// а второй - FPU/общ.
bool CDebugger::assemble2OPRFPU(int &pc, uint16_t *buf, int &len, TCHAR **pch, bool bFPU)
{
	m_bOperandType = true; // сперва ss

	if (assemble1OPRFPU(pc, buf, len, pch))  // обрабатываем регистр
	{
		if (NeedChar(_T(','), pch, false))    // проверим наличие второго
		{
			m_bOperandType = false; // теперь dd

			if (bFPU)
			{
				return assemble1OPFPU(pc, buf, len, pch);  // обработаем второй операнд
			}

			return assemble1OP(pc, buf, len, pch);     // обработаем второй операнд
		}

		// Ошибка в команде - нет второго операнда.
		return false;
	}

	return false;
}




bool CDebugger::assembleSOB(int &pc, uint16_t *buf, int &len, TCHAR **pch)
{
	m_bOperandType = true;     // сперва ss

	if (ReadRegName(buf, pch, false))  // обработаем имя регистра
	{
		if (NeedChar(_T(','), pch, false))    // проверим наличие второго
		{
			m_bOperandType = false;    // теперь dd
			int dest = 0;

			if (ReadOctalNumber(dest, pch, false))
			{
				uint16_t offs = (pc - dest) / 2;

				if (0 <= offs && offs < 077)
				{
					buf[0] |= offs & 077; // то формируем опкод
					return true; // и выходим
				}
			}

			// Ошибка длины или направления перехода в команде SOB.
			return false;
		}

		// Ошибка в команде.
		return false;
	}

	return false;
}




// разбор операнда
bool CDebugger::Operand_analyse(int &pc, uint16_t *buf, int &len, TCHAR **pch, bool bFPU)
{
	bool bMinus = false;

	if (SkipWhitespace(pch))
	{
		TCHAR ch = **pch;

		if (ch == _T('@'))
		{
			bFPU = false;
			// собака - может быть, может не быть
			SetAddrReg(buf, 010); // устанавливаем признак относительной адресации
			(*pch)++; // передвигаемся дальше
			ch = **pch; // читаем следующий символ

			if (ch == 0) // если внезапный конец строки
			{
				return false;
			}
		}

		if (ch == _T('#'))
		{
			// непосредственная адресация, за ней - число
			(*pch)++; // передвигаемся дальше
			bool bMinus = false;

			if (**pch == _T('-'))
			{
				bMinus = true;
				(*pch)++; // передвигаемся дальше
			}

			int num = 0;

			if (ReadOctalNumber(num, pch, true))
			{
				SetAddrReg(buf, 027);

				if (bMinus)
				{
					num = -num;
				}

				buf[len++] = num & 0xffff;
				pc += 2;
				return true;
			}

			return false;
		}

		if (ch == _T('('))
		{
			(*pch)++;

			// скобка, за ней - имя регистра
			if (ReadRegName(buf, pch, true)) // если регистр опознан
			{
				if (NeedChar(_T(')'), pch, true)) // если за ним - закрывающия скобка
				{
					if (**pch == _T('+'))
					{
						(*pch)++;
						SetAddrReg(buf, 020);
					}
					else
					{
						SetAddrReg(buf, 010);
					}

					return true;
				}
			}

			return false;
		}

		if (ch == _T('-'))
		{
			// если за минусом - открывающая скобка
			(*pch)++;
			ch = **pch;

			if (ch == _T('('))
			{
				(*pch)++;

				// скобка, за ней - имя регистра
				if (ReadRegName(buf, pch, true)) // если регистр опознан
				{
					if (NeedChar(_T(')'), pch, true)) // если за ним - закрывающия скобка
					{
						SetAddrReg(buf, 040);
						return true;
					}
				}

				return false;
			}

			// тут может быть отрицательное число
			if (_T('0') <= ch && ch <= _T('7'))
			{
				bMinus = true;
				goto l_Digits;
			}

			return false;
		}

		if (_T('0') <= ch && ch <= _T('7'))
		{
			// тут может быть число, а за ним - может быть регистр в скобках, а может и не быть
l_Digits:
			int num = 0;

			if (ReadOctalNumber(num, pch, true))
			{
				if (bMinus)
				{
					num = -num;
				}

				// если за числом регистр в скобках
				if (NeedChar(_T('('), pch, true))
				{
					// скобка, за ней - имя регистра
					if (ReadRegName(buf, pch, true)) // если регистр опознан
					{
						if (NeedChar(_T(')'), pch, true)) // если за ним - закрывающия скобка
						{
							// то это индексная адресация
							SetAddrReg(buf, 060);
							buf[len++] = num & 0xffff;
							pc += 2;
							return true;
						}
					}

					return false;
				}

				// это относительная адресация.
				SetAddrReg(buf, 067);
				pc += 2;
				uint16_t offs = num - pc;
				buf[len++] = offs;
				return true;
			}

			return false;
		}

		if ((_T('A') <= ch && ch <= _T('Z')) || (_T('a') <= ch && ch <= _T('z')))
		{
			// тут может быть только регистр
			if (bFPU)
			{
				return ReadFPURegName(buf, pch, false, true); // если регистр опознан
			}

			return ReadRegName(buf, pch, false); // если регистр опознан
		}
	}

	return false;
}
#endif