// Debugger.cpp: implementation of the CDebugger class.
//
#include "pch.h"
#include "Debugger.h"
#include "Config.h"
#include "Board.h"
#include "BKMessageBox.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

int CDebugger::m_outLevel = 0;

constexpr auto SRC = 2;
constexpr auto DST = 0;

const CString CDebugger::m_strRegNames[8] =
{
    _T(COLORED_TAG)_T("3R0"),
    _T(COLORED_TAG)_T("3R1"),
    _T(COLORED_TAG)_T("3R2"),
    _T(COLORED_TAG)_T("3R3"),
    _T(COLORED_TAG)_T("3R4"),
    _T(COLORED_TAG)_T("3R5"),
    _T(COLORED_TAG)_T("3SP"),
    _T(COLORED_TAG)_T("3PC")
};
const CString CDebugger::m_strRegNamesFPU[8] =
{
    _T(COLORED_TAG)_T("3AC0"),
    _T(COLORED_TAG)_T("3AC1"),
    _T(COLORED_TAG)_T("3AC2"),
    _T(COLORED_TAG)_T("3AC3"),
    _T(COLORED_TAG)_T("3AC4"),
    _T(COLORED_TAG)_T("3AC5"),
    _T(COLORED_TAG)_T("3AC6"),
    _T(COLORED_TAG)_T("3AC7")
};

// Формат отображения режимов адресации
const CString CDebugger::m_strAddrFormat[8] =
{
    _T("%s"),
    _T(COLORED_TAG)_T("5(%s")_T(COLORED_TAG)_T("5)"),
    _T(COLORED_TAG)_T("5(%s")_T(COLORED_TAG)_T("5)+"),
    _T(COLORED_TAG)_T("5@(%s")_T(COLORED_TAG)_T("5)+"),
    _T(COLORED_TAG)_T("5-(%s")_T(COLORED_TAG)_T("5)"),
    _T(COLORED_TAG)_T("5@-(%s")_T(COLORED_TAG)_T("5)"),
    _T(COLORED_TAG)_T("4%o")_T(COLORED_TAG)_T("5(%s")_T(COLORED_TAG)_T("5)"),
    _T(COLORED_TAG)_T("5@")_T(COLORED_TAG)_T("4%o")_T(COLORED_TAG)_T("5(%s")_T(COLORED_TAG)_T("5)")
};
// Формат отображения режимов адресации, если регистр PC
const CString CDebugger::m_strAddrFormat_PC[8] =
{
    _T(COLORED_TAG)_T("3PC"),
    _T(COLORED_TAG)_T("5(")_T(COLORED_TAG)_T("3PC")_T(COLORED_TAG)_T("5)"),
    _T(COLORED_TAG)_T("5#")_T(COLORED_TAG)_T("4%o"),
    _T(COLORED_TAG)_T("5@#")_T(COLORED_TAG)_T("1%o"),
    _T(COLORED_TAG)_T("5-(")_T(COLORED_TAG)_T("3PC")_T(COLORED_TAG)_T("5)"),
    _T(COLORED_TAG)_T("5@-(")_T(COLORED_TAG)_T("3PC")_T(COLORED_TAG)_T("5)"),
    _T(COLORED_TAG)_T("1%o"),
    _T(COLORED_TAG)_T("5@")_T(COLORED_TAG)_T("1%o")
};
// Формат отображения аргумента - адрес
const CString CDebugger::m_strArgFormat_Addr = _T(COLORED_TAG)_T("1%06o");
// Формат отображения аргумента - число
const CString CDebugger::m_strArgFormat_Number = _T(COLORED_TAG)_T("4%o");
// запятая между аргументом 1 и 2
const CString CDebugger::m_strArgFormat_Comma = _T(COLORED_TAG)_T("5,");

#ifdef DEBUGGER

static const COLORREF g_crDebugColorHighLighting[HLCOLOR_NUM_COLS] =
{
	RGB(0, 0, 0),           // HLCOLOR_DEFAULT
	RGB(0x66, 0, 0),        // HLCOLOR_ADDRESS
	RGB(0, 0, 0xcc),        // HLCOLOR_MNEMONIC
	RGB(0, 0x66, 0xcc),     // HLCOLOR_REGISTER
	RGB(0xff, 0x66, 0),     // HLCOLOR_NUMBER
	RGB(0x33, 0x33, 0x33),  // HLCOLOR_SYMBOL
	RGB(0, 0, 0x99),        // HLCOLOR_MNEMONICFPU
	RGB(0, 0x66, 0x99)      // HLCOLOR_REGISTERFPU
};

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction
CDebugger::CDebugger()
	: m_pBoard(nullptr)
	, m_pDisasmDlg(nullptr)
	, m_pInstrRefsMap(nullptr)
	, m_bPrevCmdC(false)
	, m_wPC(0)
	, m_wInstr(0)
	, m_bCBug(false)
	, m_wFreg(0)
	, m_bFD(false)
	, m_bFL(false)
	, m_bOperandType(false)
{
	InitMaps();
}

CDebugger::~CDebugger()
{
}

void CDebugger::SetCurrentAddress(uint16_t address) const
{
	g_Config.m_nDisasmAddr = address;
	m_pDisasmDlg->PostMessage(WM_DBG_CURRENT_ADDRESS_CHANGE, static_cast<WPARAM>(address));  // обновим адрес в поле адреса.
}

// поиск в списке точек останова, заданной точки останова
bool CDebugger::IsBpeakpointExists(const CBreakPoint &breakpoint) const
{
	if (m_breakpointList.GetCount())
	{
		POSITION pos = m_breakpointList.GetHeadPosition();

		while (pos)
		{
			const CBreakPoint &bpt = m_breakpointList.GetNext(pos);

			if ((bpt.GetType() == breakpoint.GetType())
			        && (bpt.GetAddress() == breakpoint.GetAddress()))
			{
				return true;
			}
		}
	}

	return false;
}


// поиск в списке точек останова, точки с заданным адресом
bool CDebugger::GetDebugPCBreak(uint16_t addr) const
{
	// оставим это излишество на случай, если решим алгоритмы поменять
	return IsBpeakpointAtAddress(addr);
}

// поиск в списке точек останова, точки с заданным адресом
bool CDebugger::IsBpeakpointAtAddress(const uint16_t addr) const
{
	if (m_breakpointList.GetCount())
	{
		POSITION pos = m_breakpointList.GetHeadPosition();

		while (pos)
		{
			const CBreakPoint &bpt = m_breakpointList.GetNext(pos);

			if (bpt.IsAddress() && bpt.GetAddress() == addr)
			{
				return true;
			}
		}
	}

	return false;
}


bool CDebugger::SetSimpleBreakpoint(uint16_t addr)
{
	CBreakPoint breakpoint(addr);

	if (IsBpeakpointExists(breakpoint))
	{
		return false;
	}

	m_breakpointList.AddTail(breakpoint);
	return true;
}


bool CDebugger::SetSimpleBreakpoint()
{
	return SetSimpleBreakpoint(GetCursorAddress());
}


bool CDebugger::RemoveBreakpoint(const uint16_t addr)
{
	POSITION pos = m_breakpointList.GetHeadPosition();

	while (pos)
	{
		POSITION oldPos = pos;
		const CBreakPoint &bpt = m_breakpointList.GetNext(pos);

		if (bpt.IsAddress() && bpt.GetAddress() == addr)
		{
			m_breakpointList.RemoveAt(oldPos);
			return true;
		}
	}

	return false;
}


bool CDebugger::RemoveBreakpoint()
{
	return RemoveBreakpoint(GetCursorAddress());
}


void CDebugger::ClearBreakpointList()
{
	m_breakpointList.RemoveAll();
}

bool CDebugger::CheckDebuggedLine(uint16_t wLineAddr) const
{
	if (m_pBoard)
	{
		return m_pBoard->IsCPUBreaked() && (wLineAddr == m_pBoard->GetRON(CCPU::REGISTER::PC));
	}

	return false;
}

uint16_t CDebugger::GetCursorAddress()
{
	ASSERT(m_pDisasmDlg);
	uint16_t nAddr = m_pDisasmDlg->GetCursorAddr();
	return nAddr;
}


uint16_t CDebugger::GetBottomAddress()
{
	ASSERT(m_pDisasmDlg);
	uint16_t nAddr = m_pDisasmDlg->GetBottomAddr();
	return nAddr;
}

/////////////////////////////////////////////////////////////////////////////
// используется для получения значений портов и только
uint16_t CDebugger::GetPortValue(const int addr) const
{
	ASSERT(m_pBoard);

	if (m_pBoard)
	{
		switch (addr)
		{
			case SYS_PORT_177660:
				return m_pBoard->m_reg177660;

			case SYS_PORT_177662_IN:
				return m_pBoard->m_reg177662in;

			case SYS_PORT_177662_OUT:
				return m_pBoard->m_reg177662out;

			case SYS_PORT_177664:
				return m_pBoard->m_reg177664;

			case SYS_PORT_177700:
				return m_pBoard->GetWordIndirect(0177700);

			case SYS_PORT_177702:
				return m_pBoard->GetWordIndirect(0177702);

			case SYS_PORT_177704:
				return m_pBoard->GetWordIndirect(0177704);

			case SYS_PORT_177706:
				return m_pBoard->GetWordIndirect(0177706);

			case SYS_PORT_177710:
				return m_pBoard->GetWordIndirect(0177710);

			case SYS_PORT_177712:
				return m_pBoard->GetWordIndirect(0177712);

			case SYS_PORT_177714_IN:
				return m_pBoard->m_reg177714in;

			case SYS_PORT_177714_OUT:
				return m_pBoard->m_reg177714out;

			case SYS_PORT_177716_IN:
				return m_pBoard->m_reg177716in;

			case SYS_PORT_177716_OUT_TAP:
				return m_pBoard->m_reg177716out_tap;

			case SYS_PORT_177716_OUT_MEM:
				return m_pBoard->m_reg177716out_mem;
		}

		ASSERT(false);
	}

	return 0;
}
// получение состояния режимов контроллера АльтПро
uint16_t CDebugger::GetAltProData(const int reg) const
{
	ASSERT(m_pBoard);

	if (m_pBoard)
	{
		switch (reg)
		{
			case 0:
				return m_pBoard->GetAltProMode();

			case 1:
				return m_pBoard->GetAltProCode();
		}

		ASSERT(false);
	}

	return 0;
}
// получение состояния режимов контроллера FDD
uint16_t CDebugger::GetFDDData(const int reg) const
{
	ASSERT(m_pBoard);

	if (m_pBoard)
	{
		switch (reg)
		{
			case 0:
				return m_pBoard->GetFDD()->GetStateDebug();

			case 1:
				return m_pBoard->GetFDD()->GetCmdDebug();

			case 2:
				return m_pBoard->GetFDD()->GetDataDebug();

			case 3:
				return m_pBoard->GetFDD()->GetWriteDataDebug();
		}

		ASSERT(false);
	}

	return 0;
}
uint16_t CDebugger::GetDebugHDDRegs(const int nDrive, const int num, const bool bReadMode) const
{
	ASSERT(m_pBoard);

	if (m_pBoard)
	{
		return m_pBoard->GetFDD()->ReadDebugHDDRegisters(nDrive, static_cast<HDD_REGISTER>(num), bReadMode);
	}

	return 0;
}

uint16_t CDebugger::GetDebugMemDumpWord(const uint16_t addr) const
{
	if (m_pBoard)
	{
		return m_pBoard->GetWordIndirect(addr & 0177776);
	}

	return 0;
}
uint8_t CDebugger::GetDebugMemDumpByte(const uint16_t addr) const
{
	if (m_pBoard)
	{
		return m_pBoard->GetByteIndirect(addr);
	}

	return 0;
}
// разбор команды по заданному адресу.
// вход:    pc - адрес, по которому находится команда
// выход:   длина команды в словах
//          strInstr - мнемоника
//          strArg - аргументы команды, если есть
//          codes - массив машинных кодов, длиной [длина команды в словах]
//
int CDebugger::DebugInstruction(uint16_t pc, CString &strInstr, uint16_t *codes)
{
	ASSERT(m_pBoard);
	int length = 1;

	if (m_pBoard)
	{
		m_bPrevCmdCp = m_bPrevCmdC;
		m_bPrevCmdC = false;
		m_wInstr = m_pBoard->GetWordIndirect(pc);
		m_wPC = pc + 2;
		codes[0] = m_wInstr;
		m_strArg.Empty();
		m_strInstr = *(m_pInstrRefsMap[m_wInstr].pMnemonic);
		/* много скобок, но так надо. такой хитрый синтаксис */
		length += (this->*(m_pInstrRefsMap[m_wInstr].DisasmInstrRef))(codes); // эта функция ещё формирует m_strInstr и m_strArg
		strInstr = m_strInstr + m_strArg;
	}

	return length;
}

void CDebugger::RegisterMethodRef(uint16_t start, uint16_t end, const CString *mnemonic, CalcInstrLenRef ilenmref, CalcNextAddrRef nxamref, DisassembleInstrRef dsimref)
{
	for (int opcode = start; opcode <= end; ++opcode)
	{
		m_pInstrRefsMap[opcode].pMnemonic = mnemonic;
		m_pInstrRefsMap[opcode].InstrLenRef = ilenmref;
		m_pInstrRefsMap[opcode].NextAddrRef = nxamref;
		m_pInstrRefsMap[opcode].DisasmInstrRef = dsimref;
	}
}
void CDebugger::InitMaps(bool bReinit)
{
	if (bReinit)
	{
		if (m_pInstrRefsMap)
		{
			m_pInstrRefsMap.reset();
		}
	}
	else if (m_pInstrRefsMap)
	{
		return;
	}

	m_pInstrRefsMap = std::make_unique<InstrFuncRefs[]>(65536);

	if (m_pInstrRefsMap)
	{
		RegisterMethodRef(0000000, 0177777, &m_strMnemonix[   0 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrUNKNOWN,  &CDebugger::DisassembleUnknown);   // uint16_t
		RegisterMethodRef(0000000, 0000000, &m_strMnemonix[   1 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrUNKNOWN,  &CDebugger::DisassembleNoArgs);    // HALT
		RegisterMethodRef(0000001, 0000001, &m_strMnemonix[   2 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleNoArgs);    // WAIT
		RegisterMethodRef(0000002, 0000002, &m_strMnemonix[   3 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRTI,      &CDebugger::DisassembleNoArgs);    // RTI
		RegisterMethodRef(0000003, 0000003, &m_strMnemonix[   4 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleNoArgs);    // BPT
		RegisterMethodRef(0000004, 0000004, &m_strMnemonix[   5 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleNoArgs);    // IOT
		RegisterMethodRef(0000005, 0000005, &m_strMnemonix[   6 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleNoArgs);    // RESET
		RegisterMethodRef(0000006, 0000006, &m_strMnemonix[   7 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRTI,      &CDebugger::DisassembleNoArgs);    // RTT
		RegisterMethodRef(0000010, 0000013, &m_strMnemonix[   8 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrUNKNOWN,  &CDebugger::DisassembleNoArgs);    // START
		RegisterMethodRef(0000014, 0000017, &m_strMnemonix[   9 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrUNKNOWN,  &CDebugger::DisassembleNoArgs);    // STEP
		RegisterMethodRef(0000100, 0000177, &m_strMnemonix[  10 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrJMP,      &CDebugger::DisassembleTwoField);  // JMP
		RegisterMethodRef(0000200, 0000207, &m_strMnemonix[  11 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRTS,      &CDebugger::DisassembleRTS);       // RTS / RETURN
		RegisterMethodRef(0000240, 0000257, &m_strMnemonix[  12 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleCLS);       // CLS
		RegisterMethodRef(0000260, 0000277, &m_strMnemonix[  13 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleSET);       // SET
		RegisterMethodRef(0000300, 0000377, &m_strMnemonix[  14 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrSWAB_MFPS, &CDebugger::DisassembleTwoField); // SWAB
		RegisterMethodRef(0000400, 0000777, &m_strMnemonix[  15 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBR,       &CDebugger::DisassembleBR);        // BR
		RegisterMethodRef(0001000, 0001377, &m_strMnemonix[  16 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBNE,      &CDebugger::DisassembleBR);        // BNE
		RegisterMethodRef(0001400, 0001777, &m_strMnemonix[  17 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBEQ,      &CDebugger::DisassembleBR);        // BEQ
		RegisterMethodRef(0002000, 0002377, &m_strMnemonix[  18 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBGE,      &CDebugger::DisassembleBR);        // BGE
		RegisterMethodRef(0002400, 0002777, &m_strMnemonix[  19 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBLT,      &CDebugger::DisassembleBR);        // BLT
		RegisterMethodRef(0003000, 0003377, &m_strMnemonix[  20 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBGT,      &CDebugger::DisassembleBR);        // BGT
		RegisterMethodRef(0003400, 0003777, &m_strMnemonix[  21 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBLE,      &CDebugger::DisassembleBR);        // BLE
		RegisterMethodRef(0004000, 0004777, &m_strMnemonix[  22 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrJMP,      &CDebugger::DisassembleJSR);       // JSR / CALL
		RegisterMethodRef(0005000, 0005077, &m_strMnemonix[  23 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // CLR
		RegisterMethodRef(0005100, 0005177, &m_strMnemonix[  24 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // COM
		RegisterMethodRef(0005200, 0005277, &m_strMnemonix[  25 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // INC
		RegisterMethodRef(0005300, 0005377, &m_strMnemonix[  26 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // DEC
		RegisterMethodRef(0005400, 0005477, &m_strMnemonix[  27 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // NEG
		RegisterMethodRef(0005500, 0005577, &m_strMnemonix[  28 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // ADC
		RegisterMethodRef(0005600, 0005677, &m_strMnemonix[  29 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // SBC
		RegisterMethodRef(0005700, 0005777, &m_strMnemonix[  30 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleTwoField);  // TST
		RegisterMethodRef(0006000, 0006077, &m_strMnemonix[  31 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // ROR
		RegisterMethodRef(0006100, 0006177, &m_strMnemonix[  32 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // ROL
		RegisterMethodRef(0006200, 0006277, &m_strMnemonix[  33 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // ASR
		RegisterMethodRef(0006300, 0006377, &m_strMnemonix[  34 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // ASL
		RegisterMethodRef(0006400, 0006477, &m_strMnemonix[  35 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrMARK,     &CDebugger::DisassembleMARK);      // MARK

		if (g_Config.m_bMMG)
		{
			RegisterMethodRef(0006500, 0006577, &m_strMnemonix[  36 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrUNKNOWN,  &CDebugger::DisassembleTwoField);  // MFPI
			RegisterMethodRef(0006600, 0006677, &m_strMnemonix[  37 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrUNKNOWN,  &CDebugger::DisassembleTwoField);  // MTPI
		}

		RegisterMethodRef(0006700, 0006777, &m_strMnemonix[  38 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // SXT
		RegisterMethodRef(0010000, 0017777, &m_strMnemonix[  39 ], &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular4, &CDebugger::DisassembleFourField); // MOV
		RegisterMethodRef(0020000, 0027777, &m_strMnemonix[  40 ], &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleCMP);       // CMP
		RegisterMethodRef(0030000, 0037777, &m_strMnemonix[  41 ], &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleFourField); // BIT
		RegisterMethodRef(0040000, 0047777, &m_strMnemonix[  42 ], &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular4, &CDebugger::DisassembleFourField); // BIC
		RegisterMethodRef(0050000, 0057777, &m_strMnemonix[  43 ], &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular4, &CDebugger::DisassembleFourField); // BIS
		RegisterMethodRef(0060000, 0067777, &m_strMnemonix[  44 ], &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular4, &CDebugger::DisassembleFourField); // ADD

		if (g_Config.m_bEIS || g_Config.m_bVM1G)
		{
			RegisterMethodRef(0070000, 0070777, &m_strMnemonix[  45 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleEISExt);    // MUL
		}

		if (g_Config.m_bEIS)
		{
			RegisterMethodRef(0071000, 0071777, &m_strMnemonix[  46 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleEISExt);    // DIV
			RegisterMethodRef(0072000, 0072777, &m_strMnemonix[  47 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleEISExt);    // ASH
			RegisterMethodRef(0073000, 0073777, &m_strMnemonix[  48 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleEISExt);    // ASHC
		}

		RegisterMethodRef(0074000, 0074777, &m_strMnemonix[  49 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleXOR);       // XOR

		if (g_Config.m_bFIS)
		{
			RegisterMethodRef(0075000, 0075007, &m_strMnemonix[  50 ], &CDebugger::CalcLenFIS,        &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleFIS);       // FADD
			RegisterMethodRef(0075010, 0075017, &m_strMnemonix[  51 ], &CDebugger::CalcLenFIS,        &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleFIS);       // FSUB
			RegisterMethodRef(0075020, 0075027, &m_strMnemonix[  52 ], &CDebugger::CalcLenFIS,        &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleFIS);       // FMUL
			RegisterMethodRef(0075030, 0075037, &m_strMnemonix[  53 ], &CDebugger::CalcLenFIS,        &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleFIS);       // FDIV
		}

		RegisterMethodRef(0077000, 0077777, &m_strMnemonix[  54 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrSOB,      &CDebugger::DisassembleSOB);       // SOB
		RegisterMethodRef(0100000, 0100377, &m_strMnemonix[  55 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBPL,      &CDebugger::DisassembleBR);        // BPL
		RegisterMethodRef(0100400, 0100777, &m_strMnemonix[  56 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBMI,      &CDebugger::DisassembleBR);        // BMI
		RegisterMethodRef(0101000, 0101377, &m_strMnemonix[  57 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBHI,      &CDebugger::DisassembleBR);        // BHI
		RegisterMethodRef(0101400, 0101777, &m_strMnemonix[  58 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBLOS,     &CDebugger::DisassembleBR);        // BLOS
		RegisterMethodRef(0102000, 0102377, &m_strMnemonix[  59 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBVC,      &CDebugger::DisassembleBR);        // BVC
		RegisterMethodRef(0102400, 0102777, &m_strMnemonix[  60 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBVS,      &CDebugger::DisassembleBR);        // BVS
		RegisterMethodRef(0103000, 0103377, &m_strMnemonix[  61 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBCC,      &CDebugger::DisassembleBCC);       // BHIS/BCC
		RegisterMethodRef(0103400, 0103777, &m_strMnemonix[  62 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBCS,      &CDebugger::DisassembleBCS);       // BLO/BCS
		RegisterMethodRef(0104000, 0104377, &m_strMnemonix[  63 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleEMT);       // EMT
		RegisterMethodRef(0104400, 0104777, &m_strMnemonix[  64 ], &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleEMT);       // TRAP
		RegisterMethodRef(0105000, 0105077, &m_strMnemonix[  65 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // CLRB
		RegisterMethodRef(0105100, 0105177, &m_strMnemonix[  66 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // COMB
		RegisterMethodRef(0105200, 0105277, &m_strMnemonix[  67 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // INCB
		RegisterMethodRef(0105300, 0105377, &m_strMnemonix[  68 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // DECB
		RegisterMethodRef(0105400, 0105477, &m_strMnemonix[  69 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // NEGB
		RegisterMethodRef(0105500, 0105577, &m_strMnemonix[  70 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // ADCB
		RegisterMethodRef(0105600, 0105677, &m_strMnemonix[  71 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // SBCB
		RegisterMethodRef(0105700, 0105777, &m_strMnemonix[  72 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleTwoField);  // TSTB
		RegisterMethodRef(0106000, 0106077, &m_strMnemonix[  73 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // RORB
		RegisterMethodRef(0106100, 0106177, &m_strMnemonix[  74 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // ROLB
		RegisterMethodRef(0106200, 0106277, &m_strMnemonix[  75 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // ASRB
		RegisterMethodRef(0106300, 0106377, &m_strMnemonix[  76 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField);  // ASLB
		RegisterMethodRef(0106400, 0106477, &m_strMnemonix[  77 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleTwoField);  // MTPS

		if (g_Config.m_bMMG)
		{
			RegisterMethodRef(0106500, 0106577, &m_strMnemonix[  78 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrUNKNOWN,  &CDebugger::DisassembleTwoField);  // MFPD
			RegisterMethodRef(0106600, 0106677, &m_strMnemonix[  79 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrUNKNOWN,  &CDebugger::DisassembleTwoField);  // MTPD
		}

		RegisterMethodRef(0106700, 0106777, &m_strMnemonix[  80 ], &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrSWAB_MFPS, &CDebugger::DisassembleTwoField); // MFPS
		RegisterMethodRef(0110000, 0117777, &m_strMnemonix[  81 ], &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrMOVB,     &CDebugger::DisassembleFourField); // MOVB
		RegisterMethodRef(0120000, 0127777, &m_strMnemonix[  82 ], &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleCMP);       // CMPB
		RegisterMethodRef(0130000, 0137777, &m_strMnemonix[  83 ], &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleFourField); // BITB
		RegisterMethodRef(0140000, 0147777, &m_strMnemonix[  84 ], &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular4, &CDebugger::DisassembleFourField); // BICB
		RegisterMethodRef(0150000, 0157777, &m_strMnemonix[  85 ], &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular4, &CDebugger::DisassembleFourField); // BISB
		RegisterMethodRef(0160000, 0167777, &m_strMnemonix[  86 ], &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular4, &CDebugger::DisassembleFourField); // SUB

		//FPU Instructions
		//TODO: уточнить расчёт следующего адреса, когда приёмник - PC
		if (g_Config.m_bFPU)
		{
			RegisterMethodRef(0170000, 0170000, &m_strMnemonix[  87 ], &CDebugger::CalcLenOneWord, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleNoArgs);        // CFCC
			RegisterMethodRef(0170001, 0170001, &m_strMnemonix[  88 ], &CDebugger::CalcLenOneWord, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleSETF);          // SETF
			RegisterMethodRef(0170002, 0170002, &m_strMnemonix[  89 ], &CDebugger::CalcLenOneWord, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleSETI);          // SETI
			RegisterMethodRef(0170011, 0170011, &m_strMnemonix[  90 ], &CDebugger::CalcLenOneWord, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleSETD);          // SETD
			RegisterMethodRef(0170012, 0170012, &m_strMnemonix[  91 ], &CDebugger::CalcLenOneWord, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleSETL);          // SETL
			RegisterMethodRef(0170100, 0170177, &m_strMnemonix[  92 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleTwoField);    // LDFPS
			RegisterMethodRef(0170200, 0170277, &m_strMnemonix[  93 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleTwoField);    // STFPS
			RegisterMethodRef(0170300, 0170377, &m_strMnemonix[  94 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleTwoField);    // STST
			RegisterMethodRef(0170400, 0170477, &m_strMnemonix[  95 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleTwoFieldFPUD); // CLRD/CLRF
			RegisterMethodRef(0170500, 0170577, &m_strMnemonix[  96 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleTwoFieldFPUD); // TSTD/TSTF
			RegisterMethodRef(0170600, 0170677, &m_strMnemonix[  97 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleTwoFieldFPUD); // ABSD/ABSF
			RegisterMethodRef(0170700, 0170777, &m_strMnemonix[  98 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleTwoFieldFPUD); // NEGD/NEGF
			RegisterMethodRef(0171000, 0171377, &m_strMnemonix[  99 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldFPUD); // MULD/MULF
			RegisterMethodRef(0171400, 0171777, &m_strMnemonix[ 100 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldFPUD); // MODD/MODF
			RegisterMethodRef(0172000, 0172377, &m_strMnemonix[ 101 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldFPUD); // ADDD/ADDF
			RegisterMethodRef(0172400, 0172777, &m_strMnemonix[ 102 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldLDX); // LDD/LDF
			RegisterMethodRef(0173000, 0173377, &m_strMnemonix[ 103 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldFPUD); // SUBD/SUBF
			RegisterMethodRef(0173400, 0173777, &m_strMnemonix[ 104 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldFPUD); // CMPD/CMPF
			RegisterMethodRef(0174000, 0174377, &m_strMnemonix[ 105 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACSFieldSTX); // STD/STF
			RegisterMethodRef(0174400, 0174777, &m_strMnemonix[ 106 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldFPUD); // DIVD/DIVF
			RegisterMethodRef(0175000, 0175377, &m_strMnemonix[ 107 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACSFieldSTEXP); // STEXP
			RegisterMethodRef(0175400, 0175777, &m_strMnemonix[ 108 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACSFieldSTCXJ); // STCDI/STCDL/STCFI/STCFL
			RegisterMethodRef(0176000, 0176377, &m_strMnemonix[ 109 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACSFieldSTCXY); // STCDF/STCFD
			RegisterMethodRef(0176400, 0176777, &m_strMnemonix[ 110 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldLDEXP); // LDEXP
			RegisterMethodRef(0177000, 0177377, &m_strMnemonix[ 111 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldLDCJX); // LDCID/LDCLD/LDCIF/LDCLF
			RegisterMethodRef(0177400, 0177777, &m_strMnemonix[ 112 ], &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldLDCXY); // LDCFD/LDCDF
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}
}


bool CDebugger::SetDebugRegs(const CCPU::REGISTER nAddress, const uint16_t nValue)
{
	if (m_pBoard)
	{
		if (CCPU::REGISTER::R0 <= nAddress && nAddress <= CCPU::REGISTER::PC)
		{
			m_pBoard->SetRON(nAddress, nValue);
		}
		else if (CCPU::REGISTER::PSW == nAddress)
		{
			m_pBoard->SetPSW(nValue);
		}
		else
		{
			ASSERT(false);
		}

		if (nAddress == CCPU::REGISTER::PC)
		{
			SetCurrentAddress(nValue);
		}
	}

	return true;
}

bool CDebugger::SetDebugPorts(const uint16_t nAddress, const uint16_t nValue)
{
	if (m_pBoard)
	{
		switch (nAddress)
		{
			case SYS_PORT_177660:
				m_pBoard->SetWordIndirect(0177660, nValue);
				break;

			case SYS_PORT_177662_IN:
				m_pBoard->m_reg177662in = nValue;
				break;

			case SYS_PORT_177662_OUT:
				m_pBoard->SetWordIndirect(0177662, nValue);
				break;

			case SYS_PORT_177664:
				m_pBoard->SetWordIndirect(0177664, nValue);
				break;

			case SYS_PORT_177700:
				m_pBoard->SetWordIndirect(0177700, nValue);
				break;

			case SYS_PORT_177702:
				m_pBoard->SetWordIndirect(0177702, nValue);
				break;

			case SYS_PORT_177704:
				m_pBoard->SetWordIndirect(0177704, nValue);
				break;

			case SYS_PORT_177706:
				m_pBoard->SetWordIndirect(0177706, nValue);
				break;

			case SYS_PORT_177710:
				m_pBoard->SetWordIndirect(0177710, nValue);
				break;

			case SYS_PORT_177712:
				m_pBoard->SetWordIndirect(0177712, nValue);
				break;

			case SYS_PORT_177714_IN:
				m_pBoard->m_reg177714in = nValue;
				break;

			case SYS_PORT_177714_OUT:
				m_pBoard->SetWordIndirect(0177714, nValue);
				break;

			case SYS_PORT_177716_IN:
				m_pBoard->m_reg177716in = nValue;
				break;

			case SYS_PORT_177716_OUT_TAP:
				m_pBoard->Set177716RegTap(nValue);
				// В БК 2й разряд SEL1 фиксирует любую запись в этот регистр, взводя триггер D9.1 на неограниченное время, сбрасывается который любым чтением этого регистра.
				m_pBoard->m_reg177716in |= 4;
				break;

			case SYS_PORT_177716_OUT_MEM:
				m_pBoard->Set177716RegMem(nValue);
				m_pBoard->m_reg177716in |= 4;
				break;

			default:
				ASSERT(false);
		}
	}

	return true;
}

bool CDebugger::SetDebugMemDump(const uint16_t nAddress, const uint16_t nValue)
{
	if (m_pBoard)
	{
		m_pBoard->SetWordIndirect(nAddress, nValue);
	}

	return true;
}

bool CDebugger::SetDebugMemDump(const uint16_t nAddress, const uint8_t nValue)
{
	if (m_pBoard)
	{
		m_pBoard->SetByteIndirect(nAddress, nValue);
	}

	return true;
}

bool CDebugger::SetDebugAltProData(const uint16_t nAddress, const uint16_t nValue)
{
	if (m_pBoard)
	{
		switch (nAddress)
		{
			case 0:
				m_pBoard->SetAltProMode(nValue);
				break;

			case 1:
				m_pBoard->SetAltProCode(nValue);
				break;
		}
	}

	return true;
}
#else

// Проверяем на подходящую инструкцию, если задано условие "шаг с обходом"
// т.е. не заходить в подпрограмму, а выполнить её как одну команду.
// Если при выполнении адрес возврата будет изменён, то управление будет потеряно.
bool CDebugger::IsInstructionOver(const uint16_t instruction) const
{
	switch (instruction)
	{
		case PI_BPT:
		case PI_IOT:
			return true;    // Внутрь BPT, IOT не заходим
	}

	switch (instruction & ~0377)
	{
		case PI_EMT:
		case PI_TRAP:
			return true;    // Внутрь EMT, TRAP не заходим
	}

	switch (instruction & ~0777)
	{
		case PI_JSR:
		case PI_SOB:
			return true;    // Внутрь JSR не заходим, циклы SOB пошагово не выполняем.
	}

	return false;
}

int CDebugger::CalcInstructionLength(uint16_t instr)
{
	m_wInstr = instr; // фиксируем опкод инструкции, длину которой подсчитываем
	/* много скобок, но так надо. такой хитрый синтаксис */
	return (this->*(DEBUG_EIS_MAP[instr].InstrLenRef))();
}


/*
расчёт следующего адреса, который будет после выполнения текущей инструкции.
вход: pc - текущий адрес.
выход:
    адрес следующий за инструкцией
    или ADDRESS_NONE, если его нельзя рассчитать
*/
uint16_t CDebugger::CalcNextAddr(uint16_t pc)
{
	m_wInstr = m_pBoard->GetWordIndirect(pc);
	m_wPC = pc + 2;

	if (m_bCBug)
	{
		m_bCBug = false;
		m_wFreg = m_pBoard->GetPSW() & 0177776;
	}
	else
	{
		m_wFreg = m_pBoard->GetPSW();
	}

	/* много скобок, но так надо. такой хитрый синтаксис */
	return (this->*(DEBUG_EIS_MAP[m_wInstr].NextAddrRef))(); // возвращаем из функции высчитанный адрес.
}

uint16_t CDebugger::CalcNextAddrUNKNOWN() {
	return CMotherBoard::ADDRESS_NONE;
}

// разбор команды по заданному адресу.
// вход:    pc - адрес, по которому находится команда
// выход:   длина команды в словах
//          strInstr - мнемоника
//          strArg - аргументы команды, если есть
//          codes - массив машинных кодов, длиной [длина команды в словах]
//
int CDebugger::DebugInstruction(uint16_t pc, CString &strInstr, uint16_t *codes) {
	int length = 1;
	if (m_pBoard)
	{
		m_bPrevCmdCp = m_bPrevCmdC;
		m_bPrevCmdC = false;
		m_wInstr = m_pBoard->GetWordIndirect(pc);
		m_wPC = pc + 2;
		codes[0] = m_wInstr;
		m_strArg.Empty();
		m_strInstr = m_strMnemonix[DEBUG_EIS_MAP[m_wInstr].mnemonicIdx];
		/* много скобок, но так надо. такой хитрый синтаксис */
		length += (this->*(DEBUG_EIS_MAP[m_wInstr].DisasmInstrRef))(codes); // эта функция ещё формирует m_strInstr и m_strArg
		strInstr = m_strInstr + m_strArg;
	}
	return length;
}
#endif

void CDebugger::AttachBoard(CMotherBoard *pBoard) {
	m_pBoard = pBoard;
	if (m_pBoard) {
		m_pBoard->AttachDebugger(this);
	}
}

// таблица мнемоник для дизассемблера.
const CString CDebugger::m_strMnemonix[MNEMONIX_NUM] =
{
	CString(_T(".WORD ")),
	CString(_T("HALT  ")),
	CString(_T("WAIT  ")),
	CString(_T("RTI   ")),
	CString(_T("BPT   ")),
	CString(_T("IOT   ")),
	CString(_T("RESET ")),
	CString(_T("RTT   ")),
	CString(_T("START ")),
	CString(_T("STEP  ")),
	CString(_T("JMP   ")),
	CString(_T("RTS   ")),
	CString(_T("NOP   ")),
	CString(_T("NOP260")),
	CString(_T("SWAB  ")),
	CString(_T("BR    ")),
	CString(_T("BNE   ")),
	CString(_T("BEQ   ")),
	CString(_T("BGE   ")),
	CString(_T("BLT   ")),
	CString(_T("BGT   ")),
	CString(_T("BLE   ")),
	CString(_T("JSR   ")),
	CString(_T("CLR   ")),
	CString(_T("COM   ")),
	CString(_T("INC   ")),
	CString(_T("DEC   ")),
	CString(_T("NEG   ")),
	CString(_T("ADC   ")),
	CString(_T("SBC   ")),
	CString(_T("TST   ")),
	CString(_T("ROR   ")),
	CString(_T("ROL   ")),
	CString(_T("ASR   ")),
	CString(_T("ASL   ")),
	CString(_T("MARK  ")),
	CString(_T("MFPI  ")),
	CString(_T("MTPI  ")),
	CString(_T("SXT   ")),
	CString(_T("MOV   ")),
	CString(_T("CMP   ")),
	CString(_T("BIT   ")),
	CString(_T("BIC   ")),
	CString(_T("BIS   ")),
	CString(_T("ADD   ")),
	CString(_T("MUL   ")),
	CString(_T("DIV   ")),
	CString(_T("ASH   ")),
	CString(_T("ASHC  ")),
	CString(_T("XOR   ")),
	CString(_T("FADD  ")),
	CString(_T("FSUB  ")),
	CString(_T("FMUL  ")),
	CString(_T("FDIV  ")),
	CString(_T("SOB   ")),
	CString(_T("BPL   ")),
	CString(_T("BMI   ")),
	CString(_T("BHI   ")),
	CString(_T("BLOS  ")),
	CString(_T("BVC   ")),
	CString(_T("BVS   ")),
	CString(_T("BCC   ")),
	CString(_T("BCS   ")),
	CString(_T("EMT   ")),
	CString(_T("TRAP  ")),
	CString(_T("CLRB  ")),
	CString(_T("COMB  ")),
	CString(_T("INCB  ")),
	CString(_T("DECB  ")),
	CString(_T("NEGB  ")),
	CString(_T("ADCB  ")),
	CString(_T("SBCB  ")),
	CString(_T("TSTB  ")),
	CString(_T("RORB  ")),
	CString(_T("ROLB  ")),
	CString(_T("ASRB  ")),
	CString(_T("ASLB  ")),
	CString(_T("MTPS  ")),
	CString(_T("MFPD  ")),
	CString(_T("MTPD  ")),
	CString(_T("MFPS  ")),
	CString(_T("MOVB  ")),
	CString(_T("CMPB  ")),
	CString(_T("BITB  ")),
	CString(_T("BICB  ")),
	CString(_T("BISB  ")),
	CString(_T("SUB   ")),

	CString(_T("CFCC  ")),
	CString(_T("SETF  ")),
	CString(_T("SETI  ")),
	CString(_T("SETD  ")),
	CString(_T("SETL  ")),
	CString(_T("LDFPS ")),
	CString(_T("STFPS ")),
	CString(_T("STST  ")),
	CString(_T("CLRx  ")),
	CString(_T("TSTx  ")),
	CString(_T("ABSx  ")),
	CString(_T("NEGx  ")),
	CString(_T("MULx  ")),
	CString(_T("MODx  ")),
	CString(_T("ADDx  ")),
	CString(_T("LDx   ")),
	CString(_T("SUBx  ")),
	CString(_T("CMPx  ")),
	CString(_T("STx   ")),
	CString(_T("DIVx  ")),
	CString(_T("STEXP ")),
	CString(_T("STCxj ")),
	CString(_T("STCxy ")),
	CString(_T("LDEXP ")),
	CString(_T("LDCjx ")),
	CString(_T("LDCxy "))
};

int CDebugger::CalcLenOneWord() const {
	return 2;
}

int CDebugger::DisassembleUnknown(uint16_t *codes)
{
	m_strArg.Format(m_strArgFormat_Addr, m_wInstr);
	return 0;
}


uint16_t CDebugger::CalcNextAddrRegular()
{
	return m_wPC - 2 + CalcInstructionLength(m_wInstr);
}


uint16_t CDebugger::CalcNextAddrRegular4()
{
	// тут перехватываем и обрабатываем ситуации типа MOV xxx,PC
	if ((m_wInstr & 077) == 07) // если приёмник PC
	{
		uint16_t opcode = m_wInstr & 0170000;
		uint16_t arg; // внутри GetArgD(SRC) меняется m_wPC, поэтому эту функцию нельзя вызывать перед switch,
		// из-за того, что CMP и BIT тогда начнут глючить

		if (m_wInstr & 0100000)
		{
			switch (opcode)
			{
				case PI_MOV + 0100000:
					arg = GetArgD(SRC);
					return short(char(arg & 0377));

				case PI_BIC + 0100000:
					arg = GetArgD(SRC);
					return m_wPC & (~(arg & 0377));

				case PI_BIS + 0100000:
					arg = GetArgD(SRC);
					return m_wPC | (arg & 0377);

				case PI_SUB:
					arg = GetArgD(SRC);
					return m_wPC - arg;
			}
		}
		else
		{
			switch (opcode)
			{
				case PI_MOV:
					return GetArgD(SRC);

				case PI_BIC:
					arg = GetArgD(SRC);
					return m_wPC & (~arg);

				case PI_BIS:
					arg = GetArgD(SRC);
					return m_wPC | arg;

				case PI_ADD:
					arg = GetArgD(SRC);
					return m_wPC + arg;
			}
		}
	}

	return CalcNextAddrRegular();
}


// вход: pos = источник/приёмник
// нужно, чтобы высчитать адрес перехода для команд типа mov xxx,PC
uint16_t CDebugger::GetArgD(int pos)
{
	const auto reg = static_cast<CCPU::REGISTER>(Global::GetDigit(m_wInstr, pos++));
	const uint16_t offs = (reg == CCPU::REGISTER::PC) ? 2 : 0;
	uint16_t r = (reg == CCPU::REGISTER::PC) ? m_wPC : GetRegister(reg); // содержимое регистра
	uint16_t arg = 0;
	const int meth = Global::GetDigit(m_wInstr, pos);

	switch (meth)
	{
		case 0: // R0,      PC
			arg = r;
			break;

		case 1: // (R0),    (PC)
		case 2: // (R0)+,   #012345
			arg = m_pBoard->GetWordIndirect(r);
			m_wPC += offs;
			break;

		case 3: // @(R0)+,  @#012345
			arg = m_pBoard->GetWordIndirect(r);
			arg = m_pBoard->GetWordIndirect(arg);
			m_wPC += offs;
			break;

		case 4: // -(R0),   -(PC)
			arg = m_pBoard->GetWordIndirect(r - 2);
			m_wPC -= offs;
			break;

		case 5: // @-(R0),  @-(PC)
			arg = m_pBoard->GetWordIndirect(r - 2);
			arg = m_pBoard->GetWordIndirect(arg);
			m_wPC -= offs;
			break;

		case 6: // 345(R0), 345
		{
			const uint16_t index = m_pBoard->GetWordIndirect(m_wPC);
			m_wPC += offs;
			arg = m_pBoard->GetWordIndirect(index + r + offs); // после чтения индекса PC ещё увеличивается на 2
		}
		break;

		case 7: // @345(R0),@345
		{
			const uint16_t index = m_pBoard->GetWordIndirect(m_wPC);
			m_wPC += offs;
			arg = m_pBoard->GetWordIndirect(index + r + offs); // после чтения индекса PC ещё увеличивается на 2
			arg = m_pBoard->GetWordIndirect(arg);
		}
		break;
	}

	return arg;
}

uint16_t CDebugger::CalcNextAddrRegular2()
{
	// тут перехватываем и обрабатываем ситуации типа CLR PC, SWAB PC, ASL PC и т.п.
	if ((m_wInstr & 077) == 07) // если приёмник PC
	{
		uint16_t opcode = m_wInstr & 07700;

		if (m_wInstr & 0100000)
		{
			char c = m_pBoard->GetPSWBit(PSW_BIT::C) ? 1 : 0;
			auto nPCl = char(m_wPC & 0377);

			switch (opcode)
			{
				case PI_CLR: // clrb
					nPCl = 0;
					break;

				case PI_COM: // comb
					nPCl = ~nPCl;
					break;

				case PI_INC: // incb
					nPCl++;
					break;

				case PI_DEC: // decb
					nPCl--;
					break;

				case PI_NEG: // negb
					nPCl = -nPCl;
					break;

				case PI_ADC: // adcb
					nPCl += c;
					break;

				case PI_SBC: // sbcb
					nPCl -= c;
					break;

				case PI_ROR: // rorb
					nPCl >>= 1;

					if (c)
					{
						nPCl |= 0200;
					}

					break;

				case PI_ROL: // rolb
					nPCl <<= 1;
					nPCl += c;
					break;

				case PI_ASR: // asrb
					nPCl /= 2;
					break;

				case PI_ASL: // aslb
					nPCl <<= 1;
					break;

				case 06700: // MFPS
					nPCl = m_pBoard->GetPSW() & 0377;
					return short(nPCl);
			}

			return (m_wPC & 0177400) | nPCl;
		}

		short c = m_pBoard->GetPSWBit(PSW_BIT::C) ? 1 : 0;
		auto nPC = short(m_wPC);

		switch (opcode)
		{
			case PI_SWAB:
				nPC = SWAP_BYTE(nPC);
				break;

			case PI_CLR: // clr
				nPC = 0;
				break;

			case PI_COM: // com
				nPC = ~nPC;
				break;

			case PI_INC: // inc
				nPC++;
				break;

			case PI_DEC: // dec
				nPC--;
				break;

			case PI_NEG: // neg
				nPC = -nPC;
				break;

			case PI_ADC: // adc
				nPC += c;
				break;

			case PI_SBC: // sbc
				nPC -= c;
				break;

			case PI_ROR: // ror
				nPC >>= 1;

				if (c)
				{
					nPC |= 0100000;
				}

				break;

			case PI_ROL: // rol
				nPC <<= 1;
				nPC += c;
				break;

			case PI_ASR: // asr
				nPC = nPC / 2;
				break;

			case PI_ASL: // asl
				nPC <<= 1;
				break;

			case PI_SXT: // sxt
				nPC = m_pBoard->GetPSWBit(PSW_BIT::N) ? -1 : 0;
				break;
		}

		return nPC;
	}

	return CalcNextAddrRegular();
}


uint16_t CDebugger::CalcNextAddrSWAB_MFPS()
{
	if ((m_wInstr & 070) == 0)
	{
		// баг процессора: если метод адресации 0, то внутренний регистр условий не обновляется
		m_bCBug = true;
	}

	return CalcNextAddrRegular2();
}


uint16_t CDebugger::CalcNextAddrMOVB()
{
	if ((m_wInstr & 070) == 0)
	{
		// баг процессора: если метод адресации 0, то внутренний регистр условий не обновляется
		m_bCBug = true;
	}

	return CalcNextAddrRegular4();
}

uint16_t CDebugger::CalcNextAddrRTI()
{
	return m_pBoard->GetWordIndirect(GetRegister(CCPU::REGISTER::SP));
}

uint16_t CDebugger::CalcNextAddrRTS()
{
	auto reg = static_cast<CCPU::REGISTER>(Global::GetDigit(m_wInstr, 0));

	if (reg == CCPU::REGISTER::PC)
	{
		return m_pBoard->GetWordIndirect(GetRegister(CCPU::REGISTER::SP));
	}

	return GetRegister(reg);
}

uint16_t CDebugger::CalcNextAddrJMP()
{
	int meth = Global::GetDigit(m_wInstr, 1);

	if (meth == 0)
	{
		return CMotherBoard::ADDRESS_NONE;
	}

	return GetArgAddrD(meth, static_cast<CCPU::REGISTER>(Global::GetDigit(m_wInstr, 0)));
}


// нужно, чтобы высчитать адрес перехода для JMP и JSR
uint16_t CDebugger::GetArgAddrD(int meth, CCPU::REGISTER reg) const
{
	uint16_t offs = (reg == CCPU::REGISTER::PC) ? 2 : 0;
	uint16_t r = (reg == CCPU::REGISTER::PC) ? m_wPC : GetRegister(reg);
	uint16_t arg = 0;
	uint16_t index;

	switch (meth)
	{
		case 0: // R0,      PC
			arg = static_cast<uint16_t>(reg);
			break;

		case 1: // (R0),    (PC)
		case 2: // (R0)+,   #012345
			arg = r;
			break;

		case 3: // @(R0)+,  @#012345
			arg = m_pBoard->GetWordIndirect(r);
			break;

		case 4: // -(R0),   -(PC)
			arg = r - 2;
			break;

		case 5: // @-(R0),  @-(PC)
			arg = m_pBoard->GetWordIndirect(r - 2);
			break;

		case 6: // 345(R0), 345
			index = m_pBoard->GetWordIndirect(m_wPC);
			arg = index + r + offs;
			break;

		case 7: // @345(R0),@345
			index = m_pBoard->GetWordIndirect(m_wPC);
			arg = m_pBoard->GetWordIndirect(index + r + offs); // после чтения индекса PC ещё увеличивается на 2
			break;
	}

	return arg;
}

uint16_t CDebugger::CalcNextAddrMARK()
{
	return GetRegister(CCPU::REGISTER::R5);
}

uint16_t CDebugger::CalcNextAddrSOB()
{
	auto reg = static_cast<CCPU::REGISTER>(Global::GetDigit(m_wInstr, 2));
	return m_wPC - ((GetRegister(reg) - 1) ? (m_wInstr & 077) * 2 : 0);
}

uint16_t CDebugger::CalcNextAddrBR()
{
	short br = ((short)(char)LOBYTE(m_wInstr)) * 2;
	return m_wPC + br;
}

uint16_t CDebugger::CalcNextAddrBNE()
{
	short br = ((short)(char)LOBYTE(m_wInstr)) * 2;
	return m_wPC + (GetFREGBit(PSW_BIT::Z) ? 0 : br);
}

uint16_t CDebugger::CalcNextAddrBEQ()
{
	short br = ((short)(char)LOBYTE(m_wInstr)) * 2;
	return m_wPC + (GetFREGBit(PSW_BIT::Z) ? br : 0);
}

uint16_t CDebugger::CalcNextAddrBGE()
{
	bool bn = GetFREGBit(PSW_BIT::N);
	bool bv = GetFREGBit(PSW_BIT::V);
	short br = ((short)(char)LOBYTE(m_wInstr)) * 2;
	return m_wPC + ((bn == bv) ? br : 0);
}

uint16_t CDebugger::CalcNextAddrBLT()
{
	bool bn = GetFREGBit(PSW_BIT::N);
	bool bv = GetFREGBit(PSW_BIT::V);
	short br = ((short)(char)LOBYTE(m_wInstr)) * 2;
	return m_wPC + ((bn != bv) ? br : 0);
}

uint16_t CDebugger::CalcNextAddrBGT()
{
	bool bn = GetFREGBit(PSW_BIT::N);
	bool bz = GetFREGBit(PSW_BIT::Z);
	bool bv = GetFREGBit(PSW_BIT::V);
	short br = ((short)(char)LOBYTE(m_wInstr)) * 2;
	return m_wPC + ((bz || (bn != bv)) ? 0 : br);
}

uint16_t CDebugger::CalcNextAddrBLE()
{
	bool bn = GetFREGBit(PSW_BIT::N);
	bool bz = GetFREGBit(PSW_BIT::Z);
	bool bv = GetFREGBit(PSW_BIT::V);
	short br = ((short)(char)LOBYTE(m_wInstr)) * 2;
	return m_wPC + ((bz || (bn != bv)) ? br : 0);
}

uint16_t CDebugger::CalcNextAddrBPL()
{
	short br = ((short)(char)LOBYTE(m_wInstr)) * 2;
	return m_wPC + (GetFREGBit(PSW_BIT::N) ? 0 : br);
}

uint16_t CDebugger::CalcNextAddrBMI()
{
	short br = ((short)(char)LOBYTE(m_wInstr)) * 2;
	return m_wPC + (GetFREGBit(PSW_BIT::N) ? br : 0);
}

uint16_t CDebugger::CalcNextAddrBHI()
{
	bool bz = GetFREGBit(PSW_BIT::Z);
	bool bc = GetFREGBit(PSW_BIT::C);
	short br = ((short)(char)LOBYTE(m_wInstr)) * 2;
	return m_wPC + ((bz || bc) ? 0 : br);
}

uint16_t CDebugger::CalcNextAddrBLOS()
{
	bool bz = GetFREGBit(PSW_BIT::Z);
	bool bc = GetFREGBit(PSW_BIT::C);
	short br = ((short)(char)LOBYTE(m_wInstr)) * 2;
	return m_wPC + ((bz || bc) ? br : 0);
}

uint16_t CDebugger::CalcNextAddrBVC()
{
	short br = ((short)(char)LOBYTE(m_wInstr)) * 2;
	return m_wPC + (GetFREGBit(PSW_BIT::V) ? 0 : br);
}

uint16_t CDebugger::CalcNextAddrBVS()
{
	short br = ((short)(char)LOBYTE(m_wInstr)) * 2;
	return m_wPC + (GetFREGBit(PSW_BIT::V) ? br : 0);
}

uint16_t CDebugger::CalcNextAddrBCC()
{
	short br = ((short)(char)LOBYTE(m_wInstr)) * 2;
	return m_wPC + (GetFREGBit(PSW_BIT::C) ? 0 : br);
}

uint16_t CDebugger::CalcNextAddrBCS()
{
	short br = ((short)(char)LOBYTE(m_wInstr)) * 2;
	return m_wPC + (GetFREGBit(PSW_BIT::C) ? br : 0);
}

uint16_t CDebugger::GetRegister(const CCPU::REGISTER reg) const {
	if (m_pBoard) {
		return m_pBoard->GetRON(reg);
	}
	return 0;
}


int CDebugger::DisassembleNoArgs(uint16_t *codes)
{
	return 0;
}

int CDebugger::DisassembleCLS(uint16_t *codes)
{
	if (m_wInstr == PI_CCC)
	{
		m_strInstr = _T("CCC");
	}
	else if (m_wInstr & 0xf) // если признаки есть
	{
		m_strInstr = _T("CL");

		if (m_wInstr & (1 << static_cast<int>(PSW_BIT::N)))
		{
			m_strInstr += _T('N');
		}

		if (m_wInstr & (1 << static_cast<int>(PSW_BIT::Z)))
		{
			m_strInstr += _T('Z');
		}

		if (m_wInstr & (1 << static_cast<int>(PSW_BIT::V)))
		{
			m_strInstr += _T('V');
		}

		if (m_wInstr & (1 << static_cast<int>(PSW_BIT::C)))
		{
			m_strInstr += _T('C');
		}
	}

	return 0;
}

int CDebugger::DisassembleSET(uint16_t *codes)
{
	if (m_wInstr == PI_SCC)
	{
		m_strInstr = _T("SCC");
	}
	else if (m_wInstr & 0xf) // если признаки есть
	{
		m_strInstr = _T("SE");

		if (m_wInstr & (1 << static_cast<int>(PSW_BIT::N)))
		{
			m_strInstr += _T('N');
		}

		if (m_wInstr & (1 << static_cast<int>(PSW_BIT::Z)))
		{
			m_strInstr += _T('Z');
		}

		if (m_wInstr & (1 << static_cast<int>(PSW_BIT::V)))
		{
			m_strInstr += _T('V');
		}

		if (m_wInstr & (1 << static_cast<int>(PSW_BIT::C)))
		{
			m_strInstr += _T('C');
		}
	}

	return 0;
}

int CDebugger::DisassembleRTS(uint16_t *codes)
{
	auto r = Global::GetDigit(m_wInstr, 0);

	if (static_cast<CCPU::REGISTER>(r) == CCPU::REGISTER::PC)
	{
		m_strInstr = _T("RETURN");
	}
	else
	{
		m_strArg = m_strRegNames[r];
	}

	return 0;
}

int CDebugger::DisassembleTwoField(uint16_t *codes)
{
	return ConvertArgToString(DST, m_wPC, m_strArg, codes[1]);
}

int CDebugger::DisassembleMARK(uint16_t *codes)
{
	m_strArg.Format(m_strArgFormat_Number, m_wInstr & 077);
	return 0;
}

int CDebugger::DisassembleEMT(uint16_t *codes)
{
	m_strArg.Format(m_strArgFormat_Number, LOBYTE(m_wInstr));
	return 0;
}

int CDebugger::DisassembleBR(uint16_t *codes)
{
	m_strArg.Format(m_strArgFormat_Addr, (m_wPC + (short)(char)LOBYTE(m_wInstr) * 2) & 0xffff);
	return 0;
}

int CDebugger::DisassembleBCC(uint16_t *codes)
{
	if (m_bPrevCmdCp)
	{
		m_strInstr = _T("BHIS  ");
	}

	return DisassembleBR(codes);
}

int CDebugger::DisassembleBCS(uint16_t *codes)
{
	if (m_bPrevCmdCp)
	{
		m_strInstr = _T("BLO   ");
	}

	return DisassembleBR(codes);
}

int CDebugger::DisassembleJSR(uint16_t *codes)
{
	int r = Global::GetDigit(m_wInstr, 2);
	CString strDst;
	int length = ConvertArgToString(DST, m_wPC, strDst, codes[1]);

	if (static_cast<CCPU::REGISTER>(r) == CCPU::REGISTER::PC)
	{
		m_strInstr = _T("CALL  ");
		m_strArg = strDst;
	}
	else
	{
		m_strArg = m_strRegNames[r] + m_strArgFormat_Comma + strDst;
	}

	return length;
}

int CDebugger::DisassembleEISExt(uint16_t *codes)
{
	CString strSrc;
	int length = ConvertArgToString(DST, m_wPC, strSrc, codes[1]);
	m_strArg = strSrc + m_strArgFormat_Comma + m_strRegNames[Global::GetDigit(m_wInstr, 2)];
	return length;
}

int CDebugger::DisassembleXOR(uint16_t *codes)
{
	CString strDst;
	int length = ConvertArgToString(DST, m_wPC, strDst, codes[1]);
	m_strArg = m_strRegNames[Global::GetDigit(m_wInstr, 2)] + m_strArgFormat_Comma + strDst;
	return length;
}

int CDebugger::DisassembleFIS(uint16_t *codes)
{
	int reg = Global::GetDigit(m_wInstr, 0);
	m_strArg = m_strRegNames[reg];

	// если адрес блока параметров в PC, то размер аргументов 4 слова
	if (static_cast<CCPU::REGISTER>(reg) == CCPU::REGISTER::PC)
	{
		uint16_t pc = m_wPC + 2;

		for (int i = 1; i <= 4; ++i)
		{
			codes[i] = m_pBoard->GetWordIndirect(pc);
			pc += 2;
		}

		return 4;
	}

	return 0;
}

int CDebugger::DisassembleSOB(uint16_t *codes)
{
	CString strDst;
	strDst.Format(m_strArgFormat_Addr, (m_wPC - (m_wInstr & 077) * 2) & 0xffff);
	m_strArg = m_strRegNames[Global::GetDigit(m_wInstr, 2)] + m_strArgFormat_Comma + strDst;
	return 0;
}

int CDebugger::DisassembleFourField(uint16_t *codes)
{
	CString strSrc;
	CString strDst;
	int length = ConvertArgToString(SRC, m_wPC, strSrc, codes[1]);
	length += ConvertArgToString(DST, m_wPC + length + length, strDst, codes[1 + length]);
	m_strArg = strSrc + m_strArgFormat_Comma + strDst;
	return length;
}

int CDebugger::DisassembleCMP(uint16_t *codes)
{
	m_bPrevCmdC = true;
	return DisassembleFourField(codes);
}

int CDebugger::DisassembleSETF(uint16_t *codes)
{
	m_bFD = false;
	return 0;
}

int CDebugger::DisassembleSETD(uint16_t *codes)
{
	m_bFD = true;
	return 0;
}

int CDebugger::DisassembleSETL(uint16_t *codes)
{
	m_bFL = true;
	return 0;
}

int CDebugger::DisassembleSETI(uint16_t *codes)
{
	m_bFL = false;
	return 0;
}

int CDebugger::DisassembleTwoFieldFPUD(uint16_t *codes)
{
	//модифицируем мнемонику в соответствии с типом
	m_strInstr.SetAt(3, (m_bFD ? _T('D') : _T('F')));
	return ConvertArgToStringFPU(DST, m_wPC, m_strArg, codes[1]);
}

int CDebugger::DisassembleACDFieldFPUD(uint16_t *codes)
{
	//модифицируем мнемонику в соответствии с типом
	m_strInstr.SetAt(3, (m_bFD ? _T('D') : _T('F')));
	return DisassembleACDFieldLDEXP(codes);
}

int CDebugger::DisassembleACDFieldLDX(uint16_t *codes)
{
	//модифицируем мнемонику в соответствии с типом
	m_strInstr.SetAt(2, (m_bFD ? _T('D') : _T('F')));
	return DisassembleACDFieldLDEXP(codes);
}

int CDebugger::DisassembleACDFieldLDEXP(uint16_t *codes)
{
	CString strSrc;
	int length = ConvertArgToStringFPU(DST, m_wPC, strSrc, codes[1]);
	m_strArg = strSrc + m_strArgFormat_Comma + m_strRegNamesFPU[Global::GetDigit(m_wInstr, 2) & 3];
	return length;
}

int CDebugger::DisassembleACDFieldLDCJX(uint16_t *codes)
{
	//модифицируем мнемонику в соответствии с типом
	m_strInstr.SetAt(3, (m_bFL ? _T('L') : _T('I')));
	m_strInstr.SetAt(4, (m_bFD ? _T('D') : _T('F')));
	return DisassembleACDFieldLDEXP(codes);
}

int CDebugger::DisassembleACDFieldLDCXY(uint16_t *codes)
{
	//модифицируем мнемонику в соответствии с типом
	m_strInstr.SetAt(3, (m_bFD ? _T('D') : _T('F')));
	m_strInstr.SetAt(4, (m_bFD ? _T('F') : _T('D')));
	return DisassembleACDFieldLDEXP(codes);
}

int CDebugger::DisassembleACSFieldFPUD(uint16_t *codes)
{
	//модифицируем мнемонику в соответствии с типом
	m_strInstr.SetAt(3, (m_bFD ? _T('D') : _T('F')));
	return DisassembleACSFieldSTEXP(codes);
}

int CDebugger::DisassembleACSFieldSTX(uint16_t *codes)
{
	//модифицируем мнемонику в соответствии с типом
	m_strInstr.SetAt(2, (m_bFD ? _T('D') : _T('F')));
	return DisassembleACSFieldSTEXP(codes);
}

int CDebugger::DisassembleACSFieldSTEXP(uint16_t *codes)
{
	CString strDst;
	int length = ConvertArgToStringFPU(DST, m_wPC, strDst, codes[1]);
	m_strArg = m_strRegNamesFPU[Global::GetDigit(m_wInstr, 2) & 3] + m_strArgFormat_Comma + strDst;
	return length;
}

int CDebugger::DisassembleACSFieldSTCXJ(uint16_t *codes)
{
	//модифицируем мнемонику в соответствии с типом
	m_strInstr.SetAt(3, (m_bFD ? _T('D') : _T('F')));
	m_strInstr.SetAt(4, (m_bFL ? _T('L') : _T('I')));
	return DisassembleACSFieldSTEXP(codes);
}

int CDebugger::DisassembleACSFieldSTCXY(uint16_t *codes)
{
	//модифицируем мнемонику в соответствии с типом
	m_strInstr.SetAt(3, (m_bFD ? _T('D') : _T('F')));
	m_strInstr.SetAt(4, (m_bFD ? _T('F') : _T('D')));
	return DisassembleACSFieldSTEXP(codes);
}


int CDebugger::ConvertArgToString(int arg, uint16_t pc, CString &strSrc, uint16_t &code) const
{
	ASSERT(m_pBoard);
	int reg = Global::GetDigit(m_wInstr, arg++);
	int meth = Global::GetDigit(m_wInstr, arg);

	if (static_cast<CCPU::REGISTER>(reg) == CCPU::REGISTER::PC)
	{
		switch (meth)
		{
			case 2:
			case 3:
				code = m_pBoard->GetWordIndirect(pc);
				strSrc.Format(m_strAddrFormat_PC[meth], code);
				return 1;

			case 6:
			case 7:
				code = m_pBoard->GetWordIndirect(pc);
				strSrc.Format(m_strAddrFormat_PC[meth], (pc + code + 2) & 0xffff);
				return 1;

			default:
				strSrc = m_strAddrFormat_PC[meth];
		}
	}
	else
	{
		switch (meth)
		{
			case 6:
			case 7:
				code = m_pBoard->GetWordIndirect(pc);
				strSrc.Format(m_strAddrFormat[meth], code, m_strRegNames[reg]);
				return 1;

			default:
				strSrc.Format(m_strAddrFormat[meth], m_strRegNames[reg]);
		}
	}

	return 0;
}

int CDebugger::ConvertArgToStringFPU(int arg, uint16_t pc, CString &strSrc, uint16_t &code) const
{
	int arg1 = arg;
	auto reg = Global::GetDigit(m_wInstr, arg1++);
	int meth = Global::GetDigit(m_wInstr, arg1);

	//если адресация не 0, то это обычный метод
	if (meth)
	{
		return ConvertArgToString(arg, pc, strSrc, code);
	}

	strSrc.Format(m_strAddrFormat[meth], m_strRegNamesFPU[reg]);
	return 0;
}

inline int CDebugger::CalcLenTwoFields() const
{
	return 2 + CalcArgLength(DST);
}

inline int CDebugger::CalcLenFIS() const
{
	auto reg = static_cast<CCPU::REGISTER>(Global::GetDigit(m_wInstr, 0));

	// если адрес блока параметров в PC, то длина команды 5 слов
	if (reg == CCPU::REGISTER::PC)
	{
		return 10;
	}

	return 2;
}


int CDebugger::CalcArgLength(int pos) const
{
	auto reg = static_cast<CCPU::REGISTER>(Global::GetDigit(m_wInstr, pos++));
	int meth = Global::GetDigit(m_wInstr, pos);
	int arg = 0;

	switch (meth)
	{
		/*
		case 0: // R0,      PC
		case 1: // (R0),    (PC)
		case 4: // -(R0),   -(PC)
		case 5: // @-(R0),  @-(PC)
		    break;
		*/
		case 2: // (R0)+,   #012345
		case 3: // @(R0)+,  @#012345
			if (reg == CCPU::REGISTER::PC)
			{
				arg = 2;
			}

			break;

		case 6: // 345(R0), 345
		case 7: // @345(R0),@345
			arg = 2;
			break;
	}

	return arg;
}

int CDebugger::CalcLenFourFields() const
{
	return 2 + CalcArgLength(DST) + CalcArgLength(SRC);
}

CDebugger::DebugMap CDebugger::DEBUG_EIS_MAP = {};

#define RegisterMethodRef(X, Y, M, L, N, D) else if (code >= X && code <= Y) { res = {M, L, N, D}; }

CDebugger::InstrFuncRefs CDebugger::DebugMap::operator[](uint16_t code) {
	InstrFuncRefs res = {0, &CDebugger::CalcLenOneWord, &CDebugger::CalcNextAddrUNKNOWN,  &CDebugger::DisassembleUnknown};
	if (code == 0) { // HALT
		res = {1, &CDebugger::CalcLenOneWord, &CDebugger::CalcNextAddrUNKNOWN,  &CDebugger::DisassembleNoArgs};
	} else if (code == 1) { // WAIT
	    res = {2, &CDebugger::CalcLenOneWord, &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleNoArgs};
	}
	RegisterMethodRef(0000002, 0000002,    3 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRTI,      &CDebugger::DisassembleNoArgs)    // RTI
	RegisterMethodRef(0000003, 0000003,    4 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleNoArgs)    // BPT
	RegisterMethodRef(0000004, 0000004,    5 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleNoArgs)    // IOT
	RegisterMethodRef(0000005, 0000005,    6 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleNoArgs)    // RESET
	RegisterMethodRef(0000006, 0000006,    7 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRTI,      &CDebugger::DisassembleNoArgs)    // RTT
	RegisterMethodRef(0000010, 0000013,    8 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrUNKNOWN,  &CDebugger::DisassembleNoArgs)    // START
	RegisterMethodRef(0000014, 0000017,    9 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrUNKNOWN,  &CDebugger::DisassembleNoArgs)    // STEP
	RegisterMethodRef(0000100, 0000177,   10 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrJMP,      &CDebugger::DisassembleTwoField)  // JMP
	RegisterMethodRef(0000200, 0000207,   11 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRTS,      &CDebugger::DisassembleRTS)       // RTS / RETURN
	RegisterMethodRef(0000240, 0000257,   12 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleCLS)       // CLS
	RegisterMethodRef(0000260, 0000277,   13 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleSET)       // SET
	RegisterMethodRef(0000300, 0000377,   14 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrSWAB_MFPS, &CDebugger::DisassembleTwoField) // SWAB
	RegisterMethodRef(0000400, 0000777,   15 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBR,       &CDebugger::DisassembleBR)        // BR
	RegisterMethodRef(0001000, 0001377,   16 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBNE,      &CDebugger::DisassembleBR)        // BNE
	RegisterMethodRef(0001400, 0001777,   17 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBEQ,      &CDebugger::DisassembleBR)        // BEQ
	RegisterMethodRef(0002000, 0002377,   18 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBGE,      &CDebugger::DisassembleBR)        // BGE
	RegisterMethodRef(0002400, 0002777,   19 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBLT,      &CDebugger::DisassembleBR)        // BLT
	RegisterMethodRef(0003000, 0003377,   20 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBGT,      &CDebugger::DisassembleBR)        // BGT
	RegisterMethodRef(0003400, 0003777,   21 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBLE,      &CDebugger::DisassembleBR)        // BLE
	RegisterMethodRef(0004000, 0004777,   22 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrJMP,      &CDebugger::DisassembleJSR)       // JSR / CALL
	RegisterMethodRef(0005000, 0005077,   23 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // CLR
	RegisterMethodRef(0005100, 0005177,   24 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // COM
	RegisterMethodRef(0005200, 0005277,   25 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // INC
	RegisterMethodRef(0005300, 0005377,   26 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // DEC
	RegisterMethodRef(0005400, 0005477,   27 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // NEG
	RegisterMethodRef(0005500, 0005577,   28 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // ADC
	RegisterMethodRef(0005600, 0005677,   29 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // SBC
	RegisterMethodRef(0005700, 0005777,   30 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleTwoField)  // TST
	RegisterMethodRef(0006000, 0006077,   31 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // ROR
	RegisterMethodRef(0006100, 0006177,   32 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // ROL
	RegisterMethodRef(0006200, 0006277,   33 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // ASR
	RegisterMethodRef(0006300, 0006377,   34 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // ASL
	RegisterMethodRef(0006400, 0006477,   35 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrMARK,     &CDebugger::DisassembleMARK)      // MARK
	RegisterMethodRef(0006700, 0006777,   38 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // SXT
	RegisterMethodRef(0010000, 0017777,   39 , &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular4, &CDebugger::DisassembleFourField) // MOV
	RegisterMethodRef(0020000, 0027777,   40 , &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleCMP)       // CMP
	RegisterMethodRef(0030000, 0037777,   41 , &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleFourField) // BIT
	RegisterMethodRef(0040000, 0047777,   42 , &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular4, &CDebugger::DisassembleFourField) // BIC
	RegisterMethodRef(0050000, 0057777,   43 , &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular4, &CDebugger::DisassembleFourField) // BIS
	RegisterMethodRef(0060000, 0067777,   44 , &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular4, &CDebugger::DisassembleFourField) // ADD
	RegisterMethodRef(0074000, 0074777,   49 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleXOR)       // XOR
	RegisterMethodRef(0077000, 0077777,   54 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrSOB,      &CDebugger::DisassembleSOB)       // SOB
	RegisterMethodRef(0100000, 0100377,   55 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBPL,      &CDebugger::DisassembleBR)        // BPL
	RegisterMethodRef(0100400, 0100777,   56 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBMI,      &CDebugger::DisassembleBR)        // BMI
	RegisterMethodRef(0101000, 0101377,   57 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBHI,      &CDebugger::DisassembleBR)        // BHI
	RegisterMethodRef(0101400, 0101777,   58 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBLOS,     &CDebugger::DisassembleBR)        // BLOS
	RegisterMethodRef(0102000, 0102377,   59 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBVC,      &CDebugger::DisassembleBR)        // BVC
	RegisterMethodRef(0102400, 0102777,   60 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBVS,      &CDebugger::DisassembleBR)        // BVS
	RegisterMethodRef(0103000, 0103377,   61 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBCC,      &CDebugger::DisassembleBCC)       // BHIS/BCC
	RegisterMethodRef(0103400, 0103777,   62 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrBCS,      &CDebugger::DisassembleBCS)       // BLO/BCS
	RegisterMethodRef(0104000, 0104377,   63 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleEMT)       // EMT
	RegisterMethodRef(0104400, 0104777,   64 , &CDebugger::CalcLenOneWord,    &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleEMT)       // TRAP
	RegisterMethodRef(0105000, 0105077,   65 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // CLRB
	RegisterMethodRef(0105100, 0105177,   66 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // COMB
	RegisterMethodRef(0105200, 0105277,   67 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // INCB
	RegisterMethodRef(0105300, 0105377,   68 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // DECB
	RegisterMethodRef(0105400, 0105477,   69 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // NEGB
	RegisterMethodRef(0105500, 0105577,   70 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // ADCB
	RegisterMethodRef(0105600, 0105677,   71 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // SBCB
	RegisterMethodRef(0105700, 0105777,   72 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleTwoField)  // TSTB
	RegisterMethodRef(0106000, 0106077,   73 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // RORB
	RegisterMethodRef(0106100, 0106177,   74 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // ROLB
	RegisterMethodRef(0106200, 0106277,   75 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // ASRB
	RegisterMethodRef(0106300, 0106377,   76 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular2, &CDebugger::DisassembleTwoField)  // ASLB
	RegisterMethodRef(0106400, 0106477,   77 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleTwoField)  // MTPS
	RegisterMethodRef(0106700, 0106777,   80 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrSWAB_MFPS, &CDebugger::DisassembleTwoField) // MFPS
	RegisterMethodRef(0110000, 0117777,   81 , &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrMOVB,     &CDebugger::DisassembleFourField) // MOVB
	RegisterMethodRef(0120000, 0127777,   82 , &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleCMP)       // CMPB
	RegisterMethodRef(0130000, 0137777,   83 , &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleFourField) // BITB
	RegisterMethodRef(0140000, 0147777,   84 , &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular4, &CDebugger::DisassembleFourField) // BICB
	RegisterMethodRef(0150000, 0157777,   85 , &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular4, &CDebugger::DisassembleFourField) // BISB
	RegisterMethodRef(0160000, 0167777,   86 , &CDebugger::CalcLenFourFields, &CDebugger::CalcNextAddrRegular4, &CDebugger::DisassembleFourField) // SUB
	if (g_Config.m_bMMG) {
		if(0) {}
		RegisterMethodRef(0006500, 0006577,   36 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrUNKNOWN,  &CDebugger::DisassembleTwoField)  // MFPI
		RegisterMethodRef(0006600, 0006677,   37 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrUNKNOWN,  &CDebugger::DisassembleTwoField)  // MTPI
		RegisterMethodRef(0106500, 0106577,   78 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrUNKNOWN,  &CDebugger::DisassembleTwoField)  // MFPD
		RegisterMethodRef(0106600, 0106677,   79 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrUNKNOWN,  &CDebugger::DisassembleTwoField)  // MTPD
	}
	if (g_Config.m_bEIS || g_Config.m_bVM1G) {
		if(0) {}
		RegisterMethodRef(0070000, 0070777,   45 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleEISExt)    // MUL
	}
	if (g_Config.m_bEIS)
	{
		if(0) {}
		RegisterMethodRef(0071000, 0071777,   46 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleEISExt)    // DIV
		RegisterMethodRef(0072000, 0072777,   47 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleEISExt)    // ASH
		RegisterMethodRef(0073000, 0073777,   48 , &CDebugger::CalcLenTwoFields,  &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleEISExt)    // ASHC
	}
	if (g_Config.m_bFIS) {
		if(0) {}
		RegisterMethodRef(0075000, 0075007,   50 , &CDebugger::CalcLenFIS,        &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleFIS)       // FADD
		RegisterMethodRef(0075010, 0075017,   51 , &CDebugger::CalcLenFIS,        &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleFIS)       // FSUB
		RegisterMethodRef(0075020, 0075027,   52 , &CDebugger::CalcLenFIS,        &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleFIS)       // FMUL
		RegisterMethodRef(0075030, 0075037,   53 , &CDebugger::CalcLenFIS,        &CDebugger::CalcNextAddrRegular,  &CDebugger::DisassembleFIS)       // FDIV
	}
	//FPU Instructions
	//TODO: уточнить расчёт следующего адреса, когда приёмник - PC
	if (g_Config.m_bFPU) {
		if(0) {}
		RegisterMethodRef(0170000, 0170000,   87 , &CDebugger::CalcLenOneWord, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleNoArgs)        // CFCC
		RegisterMethodRef(0170001, 0170001,   88 , &CDebugger::CalcLenOneWord, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleSETF)          // SETF
		RegisterMethodRef(0170002, 0170002,   89 , &CDebugger::CalcLenOneWord, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleSETI)          // SETI
		RegisterMethodRef(0170011, 0170011,   90 , &CDebugger::CalcLenOneWord, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleSETD)          // SETD
		RegisterMethodRef(0170012, 0170012,   91 , &CDebugger::CalcLenOneWord, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleSETL)          // SETL
		RegisterMethodRef(0170100, 0170177,   92 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleTwoField)    // LDFPS
		RegisterMethodRef(0170200, 0170277,   93 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleTwoField)    // STFPS
		RegisterMethodRef(0170300, 0170377,   94 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleTwoField)    // STST
		RegisterMethodRef(0170400, 0170477,   95 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleTwoFieldFPUD) // CLRD/CLRF
		RegisterMethodRef(0170500, 0170577,   96 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleTwoFieldFPUD) // TSTD/TSTF
		RegisterMethodRef(0170600, 0170677,   97 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleTwoFieldFPUD) // ABSD/ABSF
		RegisterMethodRef(0170700, 0170777,   98 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleTwoFieldFPUD) // NEGD/NEGF
		RegisterMethodRef(0171000, 0171377,   99 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldFPUD) // MULD/MULF
		RegisterMethodRef(0171400, 0171777,  100 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldFPUD) // MODD/MODF
		RegisterMethodRef(0172000, 0172377,  101 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldFPUD) // ADDD/ADDF
		RegisterMethodRef(0172400, 0172777,  102 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldLDX) // LDD/LDF
		RegisterMethodRef(0173000, 0173377,  103 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldFPUD) // SUBD/SUBF
		RegisterMethodRef(0173400, 0173777,  104 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldFPUD) // CMPD/CMPF
		RegisterMethodRef(0174000, 0174377,  105 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACSFieldSTX) // STD/STF
		RegisterMethodRef(0174400, 0174777,  106 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldFPUD) // DIVD/DIVF
		RegisterMethodRef(0175000, 0175377,  107 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACSFieldSTEXP) // STEXP
		RegisterMethodRef(0175400, 0175777,  108 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACSFieldSTCXJ) // STCDI/STCDL/STCFI/STCFL
		RegisterMethodRef(0176000, 0176377,  109 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACSFieldSTCXY) // STCDF/STCFD
		RegisterMethodRef(0176400, 0176777,  110 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldLDEXP) // LDEXP
		RegisterMethodRef(0177000, 0177377,  111 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldLDCJX) // LDCID/LDCLD/LDCIF/LDCLF
		RegisterMethodRef(0177400, 0177777,  112 , &CDebugger::CalcLenTwoFields, &CDebugger::CalcNextAddrRegular, &CDebugger::DisassembleACDFieldLDCXY) // LDCFD/LDCDF
	}
	return res;
}

CDebugger::CDebugger()
	: m_pBoard(nullptr)
	, m_bPrevCmdC(false)
	, m_wPC(0)
	, m_wInstr(0)
	, m_bCBug(false)
	, m_wFreg(0)
	, m_bFD(false)
	, m_bFL(false)
{
}

// Проверяем на подходящую инструкцию, если задано условие отладки "шаг с выходом"
bool CDebugger::IsInstructionOut(const uint16_t instruction)
{
	switch (instruction)
	{
		case PI_BPT:
		case PI_IOT:
			m_outLevel++; // Считаем заходы в прерывания по векторам 14 и 20
			return false;
	}

	switch (instruction & ~0377)
	{
		case PI_EMT:
		case PI_TRAP:
			m_outLevel++; // Считаем заходы в прерывания по векторам 30 и 34
			return false;
	}

	switch (instruction & ~0777)
	{
		case PI_JSR:
			m_outLevel++; // считаем заходы в подпрограммы
			return false;
	}

	// заходы считаем, чтобы не останавливаться на выходе из вложенных прерываний
	// а остановиться на выходе именно из той функции/прерывания, в которой сейчас находимся.

	if (instruction == PI_RTI || instruction == PI_RTT || (instruction & ~7) == PI_RTS)
	{
		if (--m_outLevel == -1) // Считаем выходы из прерываний/подпрограмм.
		{
			m_outLevel = 0;     // если выход из прерывания/подпрограммы на один больше, чем заход,
			return true;        // то мы вышли из функции и надо сделать останов
		}
	}

	// смешиваем подпрограммы и прерывания потому, что бывает выход из прерывания по RTS PC.
	// в случаях, когда нужно усложнить понимание и код программы
	return false;
}
