#include "pch.h"
#include <cwctype>

#include "ScriptAsm.h"
#include "LabelManager.h"
#include "Globals.h"

CScriptAsm g_ScriptAsm; // объект класса для работы

std::vector<Script_t> g_ScriptDefs; //вектор векторов скрипта.


enum SCRIPT_OPCODE : uint16_t
{
	XCHG = 0,
	MOV,
	CMP,
	BIT,
	BIC,
	BIS,
	AND,
	XOR,
	ADD,
	SUB,

	CLR = 020,
	COM,
	INC,
	DEC,
	NEG,
	ADC,
	SBC,
	TST,
	ROR,
	ROL,
	ASR,
	ASL,
	RCR,
	RCL,

	STF = 040,

	BR = 060,
	BNE,
	BEQ,
	BGE,
	BLT,
	BGT,
	BLE,
	BPL,
	BMI,
	BHI,
	BLOS,
	BVC,
	BVS,
	BCC,
	BCS,
	SOB,

};

static const std::vector<CPUCommandStruct>g_ScripCmds =
{
	{L"MOV",         SCRIPT_OPCODE::MOV, CPUCmdGroup::TWOOPS},
	{L"MOVB", 0200 + SCRIPT_OPCODE::MOV, CPUCmdGroup::TWOOPS},
	{L"CMP",         SCRIPT_OPCODE::CMP, CPUCmdGroup::TWOOPS},
	{L"CMPB", 0200 + SCRIPT_OPCODE::CMP, CPUCmdGroup::TWOOPS},
	{L"BIT",         SCRIPT_OPCODE::BIT, CPUCmdGroup::TWOOPS},
	{L"BITB", 0200 + SCRIPT_OPCODE::BIT, CPUCmdGroup::TWOOPS},
	{L"BIC",         SCRIPT_OPCODE::BIC, CPUCmdGroup::TWOOPS},
	{L"BICB", 0200 + SCRIPT_OPCODE::BIC, CPUCmdGroup::TWOOPS},
	{L"BIS",         SCRIPT_OPCODE::BIS, CPUCmdGroup::TWOOPS},
	{L"BISB", 0200 + SCRIPT_OPCODE::BIS, CPUCmdGroup::TWOOPS},
	{L"AND",         SCRIPT_OPCODE::AND, CPUCmdGroup::TWOOPS},
	{L"ANDB", 0200 + SCRIPT_OPCODE::AND, CPUCmdGroup::TWOOPS},
	{L"XOR",         SCRIPT_OPCODE::XOR, CPUCmdGroup::TWOOPS},
	{L"XORB", 0200 + SCRIPT_OPCODE::XOR, CPUCmdGroup::TWOOPS},
	{L"ADD",         SCRIPT_OPCODE::ADD, CPUCmdGroup::TWOOPS},
	{L"ADDB", 0200 + SCRIPT_OPCODE::ADD, CPUCmdGroup::TWOOPS},
	{L"SUB",         SCRIPT_OPCODE::SUB, CPUCmdGroup::TWOOPS},
	{L"SUBB", 0200 + SCRIPT_OPCODE::SUB, CPUCmdGroup::TWOOPS},
	{L"XCHG",        SCRIPT_OPCODE::XCHG, CPUCmdGroup::TWOOPS},
	{L"SWAB", 0200 + SCRIPT_OPCODE::XCHG, CPUCmdGroup::ONEOPS},

	{L"CLR",         SCRIPT_OPCODE::CLR, CPUCmdGroup::ONEOPS},
	{L"CLRB", 0200 + SCRIPT_OPCODE::CLR, CPUCmdGroup::ONEOPS},
	{L"COM",         SCRIPT_OPCODE::COM, CPUCmdGroup::ONEOPS},
	{L"COMB", 0200 + SCRIPT_OPCODE::COM, CPUCmdGroup::ONEOPS},
	{L"INC",         SCRIPT_OPCODE::INC, CPUCmdGroup::ONEOPS},
	{L"INCB", 0200 + SCRIPT_OPCODE::INC, CPUCmdGroup::ONEOPS},
	{L"DEC",         SCRIPT_OPCODE::DEC, CPUCmdGroup::ONEOPS},
	{L"DECB", 0200 + SCRIPT_OPCODE::DEC, CPUCmdGroup::ONEOPS},
	{L"NEG",         SCRIPT_OPCODE::NEG, CPUCmdGroup::ONEOPS},
	{L"NEGB", 0200 + SCRIPT_OPCODE::NEG, CPUCmdGroup::ONEOPS},
	{L"ADC",         SCRIPT_OPCODE::ADC, CPUCmdGroup::ONEOPS},
	{L"ADCB", 0200 + SCRIPT_OPCODE::ADC, CPUCmdGroup::ONEOPS},
	{L"SBC",         SCRIPT_OPCODE::SBC, CPUCmdGroup::ONEOPS},
	{L"SBCB", 0200 + SCRIPT_OPCODE::SBC, CPUCmdGroup::ONEOPS},
	{L"TST",         SCRIPT_OPCODE::TST, CPUCmdGroup::ONEOPS},
	{L"TSTB", 0200 + SCRIPT_OPCODE::TST, CPUCmdGroup::ONEOPS},
	{L"ROR",         SCRIPT_OPCODE::ROR, CPUCmdGroup::ONEOPS},
	{L"RORB", 0200 + SCRIPT_OPCODE::ROR, CPUCmdGroup::ONEOPS},
	{L"ROL",         SCRIPT_OPCODE::ROL, CPUCmdGroup::ONEOPS},
	{L"ROLB", 0200 + SCRIPT_OPCODE::ROL, CPUCmdGroup::ONEOPS},
	{L"ASR",         SCRIPT_OPCODE::ASR, CPUCmdGroup::ONEOPS},
	{L"ASRB", 0200 + SCRIPT_OPCODE::ASR, CPUCmdGroup::ONEOPS},
	{L"ASL",         SCRIPT_OPCODE::ASL, CPUCmdGroup::ONEOPS},
	{L"ASLB", 0200 + SCRIPT_OPCODE::ASL, CPUCmdGroup::ONEOPS},
	{L"RCR",         SCRIPT_OPCODE::RCR, CPUCmdGroup::ONEOPS},
	{L"RCRB", 0200 + SCRIPT_OPCODE::RCR, CPUCmdGroup::ONEOPS},
	{L"RCL",         SCRIPT_OPCODE::RCL, CPUCmdGroup::ONEOPS},
	{L"RCLB", 0200 + SCRIPT_OPCODE::RCL, CPUCmdGroup::ONEOPS},

	{L"CLF",         SCRIPT_OPCODE::STF, CPUCmdGroup::NOOPS},
	{L"STF",  0200 + SCRIPT_OPCODE::STF, CPUCmdGroup::NOOPS},

	{ L"BR",     SCRIPT_OPCODE::BR,     CPUCmdGroup::CBRANCH }, // op lll
	{ L"BNE",    SCRIPT_OPCODE::BNE,    CPUCmdGroup::CBRANCH },
	{ L"BEQ",    SCRIPT_OPCODE::BEQ,    CPUCmdGroup::CBRANCH },
	{ L"BGE",    SCRIPT_OPCODE::BGE,    CPUCmdGroup::CBRANCH },
	{ L"BLT",    SCRIPT_OPCODE::BLT,    CPUCmdGroup::CBRANCH },
	{ L"BGT",    SCRIPT_OPCODE::BGT,    CPUCmdGroup::CBRANCH },
	{ L"BLE",    SCRIPT_OPCODE::BLE,    CPUCmdGroup::CBRANCH },
	{ L"BPL",    SCRIPT_OPCODE::BPL,    CPUCmdGroup::CBRANCH },
	{ L"BMI",    SCRIPT_OPCODE::BMI,    CPUCmdGroup::CBRANCH },
	{ L"BHI",    SCRIPT_OPCODE::BHI,    CPUCmdGroup::CBRANCH },
	{ L"BLOS",   SCRIPT_OPCODE::BLOS,   CPUCmdGroup::CBRANCH },
	{ L"BVC",    SCRIPT_OPCODE::BVC,    CPUCmdGroup::CBRANCH },
	{ L"BVS",    SCRIPT_OPCODE::BVS,    CPUCmdGroup::CBRANCH },
	{ L"BCC",    SCRIPT_OPCODE::BCC,    CPUCmdGroup::CBRANCH },
	{ L"BCS",    SCRIPT_OPCODE::BCS,    CPUCmdGroup::CBRANCH },
	{ L"BHIS",   SCRIPT_OPCODE::BCC,    CPUCmdGroup::CBRANCH },
	{ L"BLO",    SCRIPT_OPCODE::BCS,    CPUCmdGroup::CBRANCH },

	{ L"SOB",    SCRIPT_OPCODE::SOB,    CPUCmdGroup::SOB } // op r,ll
};

// пропуск пробелов
// Вход: pch - указатель на текущий символ в ассемблируемой строке.
// выход: true - нашли непробельный символ
//        false - конец строки
bool CScriptAsm::SkipWhitespace(wchar_t **pch)
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
bool CScriptAsm::ReadToken(CBKToken &token, wchar_t **pch, const bool bNoSkip)
{
	if (bNoSkip || SkipWhitespace(pch))
	{
		std::wstring str;

		while (**pch)
		{
			// лексемой считается последовательность букв и цифр.
			// начинающихся с буквы.
			wchar_t ch = std::towupper(**pch);

			if ((L'A' <= ch && ch <= L'Z') || (ch == L'_') || (ch == L'$'))
			{
				str.push_back(ch);
			}
			else if (L'0' <= ch && ch <= L'9')
			{
				str.push_back(ch);
			}
			else
			{
				break;
			}

			(*pch)++;
		}

		token.setName(str);
		return true;
	}

	token.clear();
	return false;
}

// чтение восьмеричного числа
// Вход: str - строка, формируемого числа
//       pch - указатель на текущий символ в ассемблируемой строке.
//       bNoSkip - true - не пропускать пробелы перед чтением, false - пропускать
// выход: true - Ok
//        false - конец строки
bool CScriptAsm::ReadOctalNumber(int &num, wchar_t **pch, const bool bNoSkip)
{
	num = 0;

	if (bNoSkip || SkipWhitespace(pch))
	{
		std::wstring str;

		while (**pch)
		{
			// числом считается последовательность цифр 0..7.
			wchar_t ch = **pch;

			if (isOctalDigit(ch))
			{
				str.push_back(ch);
			}
			else
			{
				break;
			}

			(*pch)++;
		}

		num = _tcstol(str.c_str(), nullptr, 8);
		return true;
	}

	return false;
}

// ищем ожидаемый символ.
// вход: ch - ожидаемый символ
//       pch - указатель на текущий символ в ассемблируемой строке.
//       bNoSkip - true - не пропускать пробелы перед чтением, false - пропускать
// выход: true - есть он там, указатель ставится за него
//      false - нету, или конец строки, указатель останавливается на любом непробельном символе
//      или на конце строки.
bool CScriptAsm::NeedChar(TCHAR ch, TCHAR **pch, const bool bNoSkip)
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

// на входе: ln - номер текущей строки скрипта
//          sc - указатель на текущий скрипт
// выход: номер следующей строки, если -1 - ошибка разбора скрипта
size_t CScriptAsm::ProcessScriptLine(const size_t ln, const std::vector<ScriptLine_t> *sc)
{
	if (!sc->at(ln).Cmd.getName().empty())
	{
		auto pch = const_cast<wchar_t *>(sc->at(ln).Cmd.getName().c_str());
		CBKToken token;

		if (ReadToken(token, &pch, false))
		{
			for (auto &cmd : g_ScripCmds)
			{
				if (token.calcHash(cmd.strName) == token.getHash())
				{
					m_bByteOp = !!(cmd.nOpcode & 0200);
					m_Nbit = (m_bByteOp ? 0200 : 0100000);

					switch (cmd.nGroup)
					{
						case CPUCmdGroup::TWOOPS:
							if (ParseScripOp(SC_SRC, &pch))
							{
								if (NeedChar(_T(','), &pch, false))    // проверим наличие второго
								{
									if (ParseScripOp(SC_DST, &pch))
									{
										ProcessTwoOps(cmd.nOpcode & 0177);
										return ln + 1;
									}
								}
								else
								{
									wprintf(L"SCRIPT: Отсутствует второй операнд.\n");
								}
							}

							break;

						case CPUCmdGroup::ONEOPS:
							if (ParseScripOp(SC_DST, &pch))
							{
								ProcessOneOps(cmd.nOpcode & 0177);
								return ln + 1;
							}

							break;

						case CPUCmdGroup::NOOPS:
							ProcessNoOps(cmd.nOpcode & 0177, &pch);
							return ln + 1;

						case CPUCmdGroup::CBRANCH:
						{
							CBKToken token;

							if (ReadToken(token, &pch, false))
							{
								// нужно по всему скрипту поискать такую метку
								for (size_t i = 0; i < sc->size(); ++i)
								{
									if (sc->at(i).Label.getHash() == token.getHash())
									{
										return ProcessBranch(cmd.nOpcode) ? i : ln + 1;
									}
								}

								// не нашли
								wprintf(L"SCRIPT:: Не найдена метка перехода в скрипте.\n");
							}
							else
							{
								wprintf(L"SCRIPT:: Неверный символ в строке.\n");
							}
						}
						break;

						case CPUCmdGroup::SOB:
						{
							CBKToken reg;

							if (ReadToken(reg, &pch, false))
							{
								int num = 0;

								if (CheckRegName(num, reg))
								{
									if (NeedChar(_T(','), &pch, false))    // проверим наличие второго
									{
										CBKToken token;

										if (ReadToken(token, &pch, false))
										{
											// нужно по всему скрипту поискать такую метку
											for (size_t i = 0; i < sc->size(); ++i)
											{
												if (sc->at(i).Label.getHash() == token.getHash())
												{
													if (--m_Vars[num])
													{
														return i;
													}

													return ln + 1;
												}
											}

											// не нашли
											wprintf(L"SCRIPT:: Не найдена метка перехода в скрипте.\n");
										}
										else
										{
											wprintf(L"SCRIPT:: Неверный символ в строке.\n");
										}
									}
									else
									{
										wprintf(L"SCRIPT: Отсутствует метка перехода в команде.\n");
									}
								}
								else
								{
									wprintf(L"SCRIPT:: Неверное имя регистра.\n");
								}
							}
							else
							{
								wprintf(L"SCRIPT:: Неверный символ в строке.\n");
							}
						}
						break; // case
					}

					break; // выход из цикла for
				}
			}

			// кончился список, и не нашли в нём нужного
			wprintf(L"SCRIPT:: Неверная команда.\n");
		}

		// токен не прочёлся
		wprintf(L"SCRIPT:: Неверный символ в строке.\n");
		return -1;
	}

	return ln + 1;
}

// обработка ветвления.
// на входе опкод
// на выходе - true нужно делать переход
//             false - не нужно
bool CScriptAsm::ProcessBranch(const uint16_t opcode)
{
	switch (opcode)
	{
		case SCRIPT_OPCODE::BR:
			return true;

		case SCRIPT_OPCODE::BNE:
			return !GetZ();

		case SCRIPT_OPCODE::BEQ:
			return GetZ();

		case SCRIPT_OPCODE::BGE:
			return (GetN() == GetV());

		case SCRIPT_OPCODE::BLT:
			return (GetN() != GetV());

		case SCRIPT_OPCODE::BGT:
			return !(GetZ() || (GetN() != GetV()));

		case SCRIPT_OPCODE::BLE:
			return (GetZ() || (GetN() != GetV()));

		case SCRIPT_OPCODE::BPL:
			return !GetN();

		case SCRIPT_OPCODE::BMI:
			return GetN();

		case SCRIPT_OPCODE::BHI:
			return !(GetZ() || GetC());

		case SCRIPT_OPCODE::BLOS:
			return (GetZ() || GetC());

		case SCRIPT_OPCODE::BVC:
			return !GetV();

		case SCRIPT_OPCODE::BVS:
			return GetV();

		case SCRIPT_OPCODE::BCC: //BHIS
			return !GetC();

		case SCRIPT_OPCODE::BCS: //BLO
			return GetC();
	}

	assert(false);
	return false;
}

// обработка однооперандных команд.
// на входе опкод
void CScriptAsm::ProcessOneOps(const uint16_t opcode)
{
	if (opcode == SCRIPT_OPCODE::XCHG) // swab
	{
		// это словная операция над байтами слова
		m_bByteOp = false;
	}

	const uint32_t old = m_ALU = get_arg(SC_DST);

	switch (opcode)
	{
		case SCRIPT_OPCODE::XCHG:   // SWAB
		{
			m_ALU = (((m_ALU) & 0x00FF) << 8) | (((m_ALU) >> 8) & 0x00FF);
			const uint8_t lb = m_ALU & 0xff;
			SetN(!!(lb & 0200));
			SetZ(lb == 0);
			SetV(false);
			SetC(false);
		}
		break;

		case SCRIPT_OPCODE::CLR:
			m_ALU = 0;
			SetN(false); SetZ(true); SetV(false); SetC(false);
			break;

		case SCRIPT_OPCODE::COM:
			m_ALU = ~m_ALU;
			Set_NZ(); SetV(false); SetC(true);
			break;

		case SCRIPT_OPCODE::INC:
			m_ALU++;
			Set_NZ(); Set_V(old);
			break;

		case SCRIPT_OPCODE::DEC:
			m_ALU--;
			Set_NZ(); Set_IV(old);
			break;

		case SCRIPT_OPCODE::NEG:
#if defined _MSC_VER
#pragma warning(disable:4146)
			m_ALU = -m_ALU; // хотя, в VS прокатывает, оно использует neg eax
#pragma warning(default:4146)
#else
			// т.к. m_ALU - uint32_t, не все могут применять унарный минус к переменной
			m_ALU = ~m_ALU + 1;
#endif
			Set_NZ();

			if (m_bByteOp) // число 0100000 не меняет знак, оно становится самим собой. поэтому считается, что V
			{
				SetV(LOBYTE(m_ALU) == 0200);
			}
			else
			{
				SetV(LOWORD(m_ALU) == 0100000);
			}

			SetC(!GetZ()); // очищается, если 0, иначе устанавливается
			break;

		case SCRIPT_OPCODE::ADC:
			m_ALU += (GetC() ? 1 : 0);
			Set_NZ();
			Set_V(old);
			Set_C();
			break;

		case SCRIPT_OPCODE::SBC:
			m_ALU -= (GetC() ? 1 : 0);
			Set_NZ();
			Set_IV(old);
			Set_C();
			break;

		case SCRIPT_OPCODE::TST:
			Set_NZ(); SetV(false); SetC(false);
			break;

		case SCRIPT_OPCODE::ROR:
		{
			const bool bC = !!(m_ALU & 1); // в будущее С помещаем младший бит
			m_ALU >>= 1;

			if (GetC())
			{
				m_ALU |= m_Nbit;    // текущее С помещаем в старший разряд
			}

			Set_NZ();
			SetV(GetN() ^ bC);
			SetC(bC);
		}
		break;

		case SCRIPT_OPCODE::ROL:
		{
			const bool bC = !!(m_ALU & m_Nbit); // в будущее С помещаем старший бит
			m_ALU <<= 1;

			if (GetC())
			{
				m_ALU |= 1;    // в младший бит помещаем текущее С
			}

			Set_NZ();
			SetV(GetN() ^ bC);
			SetC(bC);
		}
		break;

		case SCRIPT_OPCODE::ASR:
		{
			const bool bN = !!(m_ALU & m_Nbit);
			const bool bC = !!(m_ALU & 1); // в будущее С помещаем младший бит
			m_ALU >>= 1;

			if (bN)
			{
				m_ALU |= m_Nbit;    // в старший бит помещаем текущее N, т.е. знак остаётся как был
			}

			Set_NZ();
			SetV(GetN() ^ bC);
			SetC(bC);
		}
		break;

		case SCRIPT_OPCODE::ASL:
		{
			const bool bC = !!(m_ALU & m_Nbit); // в будущее С помещаем старший бит
			m_ALU <<= 1; // сдвиг
			Set_NZ();
			SetV(GetN() ^ bC);
			SetC(bC);
		}
		break;

		case SCRIPT_OPCODE::RCR:
		{
			const bool bC = !!(m_ALU & 1); // в будущее С помещаем младший бит
			m_ALU >>= 1;

			if (bC)
			{
				m_ALU |= m_Nbit;    // в старший разряд помещаем бывший младший
			}

			Set_NZ();
			SetV(GetN() ^ bC);
			SetC(bC);
		}
		break;

		case SCRIPT_OPCODE::RCL:
		{
			const bool bC = !!(m_ALU & m_Nbit); // в будущее С помещаем старший бит
			m_ALU <<= 1;

			if (bC)
			{
				m_ALU |= 1;    // в младший бит помещаем бывший старший
			}

			Set_NZ();
			SetV(GetN() ^ bC);
			SetC(bC);
		}
		break;
	}

	set_arg(SC_DST, m_ALU);
}

void CScriptAsm::ProcessTwoOps(const uint16_t opcode)
{
	uint32_t src = get_arg(SC_SRC);
	const uint32_t old = m_ALU = get_arg(SC_DST);

	switch (opcode)
	{
		case SCRIPT_OPCODE::XCHG:
			std::swap(m_ALU, src);
			Set_NZ();
			SetV(false);
			set_arg(SC_SRC, src);
			break;

		case SCRIPT_OPCODE::MOV:
			m_ALU = (m_bByteOp) ? (int)(char)(src & 0xff) : src;
			Set_NZ();
			SetV(false);
			break;

		case SCRIPT_OPCODE::CMP:
			m_ALU = src - m_ALU;
			Set_NZ();
			SetV(!!(((src ^ old) & ~(old ^ m_ALU)) & m_Nbit));
			Set_C();
			return;

		case SCRIPT_OPCODE::BIT:
			m_ALU &= src;
			Set_NZ();
			SetV(false);
			return;

		case SCRIPT_OPCODE::BIC:
			m_ALU &= ~src;
			Set_NZ();
			SetV(false);
			break;

		case SCRIPT_OPCODE::BIS:
			m_ALU |= src;
			Set_NZ();
			SetV(false);
			break;

		case SCRIPT_OPCODE::AND:
			m_ALU &= src;
			Set_NZ();
			SetV(false);
			break;

		case SCRIPT_OPCODE::XOR:
			m_ALU ^= src;
			Set_NZ();
			SetV(false);
			break;

		case SCRIPT_OPCODE::ADD:
			m_ALU += src;
			Set_NZ();
			SetV(!!((~(src ^ old) & (old ^ m_ALU)) & m_Nbit));
			Set_C();
			break;

		case SCRIPT_OPCODE::SUB:
			m_ALU -= src;
			Set_NZ();
			SetV(!!((src ^ old) & ~(src ^ m_ALU) & m_Nbit));
			Set_C();
			break;
	}

	set_arg(SC_DST, m_ALU);
}

void CScriptAsm::ProcessNoOps(const uint16_t opcode, wchar_t **pch)
{
	if (SkipWhitespace(pch))
	{
		uint16_t flags = 0;

		for (;;)
		{
			wchar_t ch = **pch; (*pch)++;
			ch = std::towupper(ch);

			if (ch == L'N')
			{
				flags |= PSW_FLG::N;
			}
			else if (ch == L'Z')
			{
				flags |= PSW_FLG::Z;
			}
			else if (ch == L'V')
			{
				flags |= PSW_FLG::V;
			}
			else if (ch == L'C')
			{
				flags |= PSW_FLG::C;
			}
			else
			{
				break;
			}
		}

		if (m_bByteOp)
		{
			m_PSW |= flags;
		}
		else
		{
			m_PSW &= ~flags;
		}
	}
}

uint32_t CScriptAsm::get_arg(const int op)
{
	if (m_bByteOp)
	{
		switch (m_OpAddr[op].adressation)
		{
			case 0:
				return m_Vars[m_OpAddr[op].address] & 0xff;

			case 5:
				return m_OpAddr[op].address & 0xff; // для непосредственной адресации это само значение
		}

		return g_Memory.b[m_OpAddr[op].address];
	}

	switch (m_OpAddr[op].adressation)
	{
		case 0:
			return m_Vars[m_OpAddr[op].address];

		case 5:
			return m_OpAddr[op].address; // для непосредственной адресации это само значение
	}

	return g_Memory.w[m_OpAddr[op].address / 2];
}


void CScriptAsm::set_arg(const int op, const uint32_t v)
{
	if (m_bByteOp)
	{
		if (m_OpAddr[op].adressation)
		{
			if (m_OpAddr[op].adressation == 5)
			{
				// в приёмнике непосредственная адресация не имеет смысла.
				return;
			}

			g_Memory.b[m_OpAddr[op].address] = v & 0xff;
		}
		else
		{
			m_Vars[m_OpAddr[op].address] = (short)(char)(v & 0xff);
		}
	}
	else
	{
		if (m_OpAddr[op].adressation)
		{
			if (m_OpAddr[op].adressation == 5)
			{
				// в приёмнике непосредственная адресация не имеет смысла.
				return;
			}

			g_Memory.w[m_OpAddr[op].address / 2] = v & 0xffff;
		}
		else
		{
			m_Vars[m_OpAddr[op].address] = v & 0xffff;
		}
	}
}

bool CScriptAsm::isLetter(wchar_t ch)
{
	ch = std::towupper(ch);

	if ((L'A' <= ch && ch <= L'Z') || (ch == L'_') || (ch == L'$'))
	{
		return true;
	}

	return false;
}

bool CScriptAsm::isOctalDigit(const wchar_t ch)
{
	if (L'0' <= ch && ch <= L'7')
	{
		return true;
	}

	return false;
}

// op - индекс:SC_SRC, SC_DST
bool CScriptAsm::ParseScripOp(const int op, wchar_t **pch)
{
	if (SkipWhitespace(pch))
	{
		wchar_t ch = **pch; // берём текущий символ.

		if (ch == L'#')
		{
			// непосредственная адресация
			(*pch)++;

			if (isLetter(**pch))
			{
				// метка
				CBKToken token;

				if (ReadToken(token, pch, true))
				{
					int n = g_labelGlobalDefs.SearchLabel(&token);

					if (n >= 0)
					{
						// если такая метка нашлась.
						m_OpAddr[op].adressation = 5;
						int value = g_labelGlobalDefs.GetValue(n);
//                      if ((g_labelGlobalDefs.GetType(n) & LBL_DEFINITE_MASK) == LBL_GLOBAL)
//                      {
//                          value = g_Globals.GetRealAddress(value);
//                      }
						m_OpAddr[op].address = value;
						return true;
					}

					wprintf(L"SCRIPT: Неверное имя метки в скрипте.\n");
				}
				else
				{
					wprintf(L"SCRIPT: Неверный символ в строке.\n");
				}
			}
			else if (isOctalDigit(**pch))
			{
				// число
				int num = 0;

				if (ReadOctalNumber(num, pch, true))
				{
					m_OpAddr[op].adressation = 5;
					m_OpAddr[op].address = num;
					return true;
				}
			}

			return false;
		}

		if (ch == L'-')
		{
			// возможно автодекремент
			(*pch)++;
			int num = 0;

			if (CheckRelReg(num, pch) == 0) // да
			{
				m_OpAddr[op].adressation = 3;
				m_Vars[num] -= m_bByteOp ? 1 : 2;
				m_OpAddr[op].address = m_Vars[num];
				return true;
			}

			wprintf(L"SCRIPT: Неверно задана адресация операнда.\n");
			return false;
		}

		if (ch == L'(')
		{
			// возможно относительная или автоинкремент
			int num = 0;

			if (CheckRelReg(num, pch) == 0) // да
			{
				if (**pch == L'+')
				{
					(*pch)++;
					// автоинкремент
					m_OpAddr[op].adressation = 2;
					m_OpAddr[op].address = m_Vars[num];
					m_Vars[num] += m_bByteOp ? 1 : 2;
					return true;
				}

				// относительная
				m_OpAddr[op].adressation = 1;
				m_OpAddr[op].address = m_Vars[num];
				return true;
			}

			wprintf(L"SCRIPT: Неверно задана адресация операнда.\n");
			return false;
		}

		if (isLetter(ch))
		{
			CBKToken token;

			if (ReadToken(token, pch, true))
			{
				int num = 0;

				if (CheckRegName(num, token))
				{
					if (**pch == L'[' && num >= 4)
					{
						(*pch)++;
						// индексная адресация
						int ind = 0;

						if (ReadToken(token, pch, true))
						{
							if (CheckRegName(ind, token))
							{
								if (**pch == L']')
								{
									(*pch)++;
									m_OpAddr[op].adressation = 4;
									m_OpAddr[op].address = m_Vars[num] + m_Vars[ind] * (m_bByteOp ? 1 : 2);
									return true;
								}

								wprintf(L"SCRIPT: Неверно задана адресация операнда.\n");
							}
						}
						else
						{
							wprintf(L"SCRIPT: Неверный символ в строке.\n");
						}

						return false;
					}

					// регистровая адресация
					m_OpAddr[op].adressation = 0;
					m_OpAddr[op].address = num;
					return true;
				}

				// или абсолютная адресация
				const int n = g_labelGlobalDefs.SearchLabel(&token);

				if (n >= 0)
				{
					// если такая метка нашлась.
					m_OpAddr[op].adressation = 6;
					int value = g_labelGlobalDefs.GetValue(n);
//                  if ((g_labelGlobalDefs.GetType(n) & LBL_DEFINITE_MASK) == LBL_GLOBAL)
//                  {
//                      value = g_Globals.GetRealAddress(value);
//                  }
					m_OpAddr[op].address = value;
					return true;
				}

				wprintf(L"SCRIPT: Неверное имя метки в скрипте.\n");
			}
		}
		else if (isOctalDigit(ch))
		{
			// число
			int num = 0;

			if (ReadOctalNumber(num, pch, true))
			{
				m_OpAddr[op].adressation = 6;
				m_OpAddr[op].address = (num + BASE_ADDRESS - g_Globals.GetStartAddress()) & 0xffff;
				return true;
			}
		}
	}

	return false;
}

// проверка на имя регистра
bool CScriptAsm::CheckRegName(int &num, CBKToken &reg)
{
	for (auto &r : g_RegNames)
	{
		if (r.names.at(0).getHash() == reg.getHash())
		{
			num = r.nNum;
			return true;
		}
	}

	return false;
}


// проверка на относительную адресацию
// выход: 0 - ОК
// 1 - нет открывающей скобки
// 2 - нет имени регистра
// 3 - нет закрывающей скобки
int CScriptAsm::CheckRelReg(int &num, wchar_t **pch)
{
	if (**pch == L'(')
	{
		(*pch)++;
		num = 0;
		CBKToken reg;

		if (ReadToken(reg, pch, true))
		{
			if (CheckRegName(num, reg))
			{
				if (**pch == L')')
				{
					(*pch)++;
					return 0;
				}

				wprintf(L"SCRIPT:: Нет закрывающей скобки.\n");
				return 3;
			}

			wprintf(L"SCRIPT:: Неверное имя регистра.\n");
		}
		else
		{
			wprintf(L"SCRIPT:: Нет имени регистра.\n");
		}

		return 2;
	}

	wprintf(L"SCRIPT:: Нет открывающей скобки.\n");
	return 1;
}

void CScriptAsm::RunScript()
{
	if (!g_ScriptDefs.empty())
	{
		std::vector<Script_t> scripts;

		for (const auto &s : g_ScriptDefs)
		{
			const std::wstring name = s.name.getName().empty() ? L"UNNAMED" : s.name.getName();
			bool bOK = true;
			wprintf(L"SCRIPT:: Выполнение скрипта: \"%s\"\n", name.c_str());
			size_t currentLine = 0; //текущая выполняемая строка
			size_t line = 0;

			do
			{
				line = ProcessScriptLine(currentLine, &s.script);

				if (line == -1)
				{
					std::wstring strLine = s.script.at(currentLine).Label.getName() +
					                       s.script.at(currentLine).Cmd.getName();
					wprintf(L"SCRIPT:: Ошибка выполнения скрипта в строке:\n\t%s\n", strLine.c_str());
					bOK = false; // ошибка выполнения скрипта
					break;
				}

				currentLine = line;
			}
			while (currentLine < s.script.size());

			if (bOK)
			{
				wprintf(L"SCRIPT:: Успешно.\n");
			}
			else
			{
				// если в скрипте случилась ошибка, то сохраним его
				scripts.push_back(s);
			}
		}

		// оставим только скрипты с ошибками, они попадут в объектный файл.
		g_ScriptDefs = scripts;
		// а если ошибок не было, то g_ScriptDefs станет пустым
	}
}

void CScriptAsm::Store(FILE *f)
{
	for (const auto &s : g_ScriptDefs)
	{
		if (!s.script.empty())
		{
			const OBJTags tag = OBJTags::OBJ_ScriptBody;
			fwrite(&tag, 1, sizeof(tag), f);
			const size_t len = s.script.size();
			fwrite(&len, 1, sizeof(len), f);
			s.name.Store(f);

			for (const auto &sl : s.script)
			{
				sl.Label.Store(f);
				sl.Cmd.Store(f);
			}
		}
	}
}

bool CScriptAsm::Load(const uint32_t nLen, FILE *f)
{
	g_ScriptDefs.clear();

	for (uint32_t i = 0; i < nLen; ++i) //количество скриптов
	{
		OBJTags tag;
		fread(&tag, 1, sizeof(tag), f);

		if (tag == OBJTags::OBJ_ScriptBody)
		{
			size_t len = 0;

			if (sizeof(len) == fread(&len, 1, sizeof(len), f)) // количество строк в скрипте
			{
				ScriptText_t script;
				CBKToken scriptName;

				if (scriptName.Load(f))
				{
					for (size_t j = 0; j < len; ++j)
					{
						CBKToken lbl, cmd;

						if (lbl.Load(f) && cmd.Load(f))
						{
							script.push_back(ScriptLine_t{ lbl, cmd });
						}
						else
						{
							return false;
						}
					}

					g_ScriptDefs.push_back({ scriptName, script });
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	return true;
}

