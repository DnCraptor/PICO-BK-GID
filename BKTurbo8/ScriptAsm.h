#pragma once

#include "BKToken.h"

struct ScriptLine_t         // элемент строки скрипта
{
	CBKToken Label;         // метка строки, если есть
	CBKToken Cmd;           // команда строки
};

// текст скрипта
using ScriptText_t = std::vector<ScriptLine_t>;

struct Script_t             // скрипт
{
	CBKToken name;          // имя скрипта, опционально
	ScriptText_t script;    // текст скрипта
};

extern std::vector<Script_t> g_ScriptDefs;

class CScriptAsm
{
		enum
		{
			SC_SRC = 0,
			SC_DST
		};

		enum PSW_FLG : uint16_t
		{
			C = 1,
			V = 2,
			Z = 4,
			N = 8,
		};

		struct Operand
		{
			int adressation;    // адресация, если 0, то актуальный адрес==номер регистра
			int address;        // актуальный адрес
		};

		uint16_t            m_PSW;
		uint16_t            m_Vars[8];  //массив переменных R0..R7
		Operand             m_OpAddr[2]; // параметры операндов: источника и приёмника
		bool                m_bByteOp;  // флаг байтовой операции
		uint32_t            m_ALU;      // приёмник
		uint32_t            m_Nbit;     // знаковый разряд для операции. 200 для байта, 100000 для слова
		CBKToken            m_scriptName;
		ScriptText_t        m_script;   // скрипт, который выделяется из исходного текста при парсинге.

	public:
		CScriptAsm()
			: m_PSW(0)
			, m_OpAddr()
			, m_Vars()
			, m_bByteOp(false)
		{};

		~CScriptAsm()
		{
			m_script.clear();
		};

		void            Init(const std::wstring &strName)
		{
			m_scriptName.setName(strName);
			m_script.clear();
		}

		void            AddScriptLine(const ScriptLine_t &sl)
		{
			m_script.push_back(sl);
		}

		void            PushScript()
		{
			g_ScriptDefs.push_back({ m_scriptName, m_script });
		}

		void            RunScript();

		void            Store(FILE *f);
		bool            Load(const uint32_t nLen, FILE *f);

	protected:
		bool            SkipWhitespace(wchar_t **pch);
		bool            ReadToken(CBKToken &token, wchar_t **pch, const bool bNoSkip);
		bool            ReadOctalNumber(int &num, wchar_t **pch, const bool bNoSkip);

		bool            NeedChar(TCHAR ch, TCHAR **pch, const bool bNoSkip);
		bool            isLetter(wchar_t ch);
		bool            isOctalDigit(const wchar_t ch);
		bool            ParseScripOp(int op, wchar_t **pch);
		bool            CheckRegName(int &num, CBKToken &reg);
		int             CheckRelReg(int &num, wchar_t **pch);

		size_t          ProcessScriptLine(const size_t ln, const std::vector<ScriptLine_t> *sc);

		bool            ProcessBranch(const uint16_t opcode);
		void            ProcessOneOps(const uint16_t opcode);
		void            ProcessTwoOps(const uint16_t opcode);
		void            ProcessNoOps(const uint16_t opcode, wchar_t **pch);

		uint32_t        get_arg(const int op);
		void            set_arg(const int op, const uint32_t v);

		inline bool     GetPSWBit(const PSW_FLG flg)
		{
			return !!(m_PSW & flg);
		}
		inline void     SetPSWBit(const PSW_FLG flg, const bool val)
		{
			if (val)
			{
				m_PSW |= flg;
			}
			else
			{
				m_PSW &= ~flg;
			}
		}

		inline void     SetC(const bool bFlag)
		{
			SetPSWBit(PSW_FLG::C, bFlag);
		}

		inline bool     GetC()
		{
			return GetPSWBit(PSW_FLG::C);
		}

		inline void     SetV(const bool bFlag)
		{
			SetPSWBit(PSW_FLG::V, bFlag);
		}

		inline bool     GetV()
		{
			return GetPSWBit(PSW_FLG::V);
		}

		inline void     SetZ(const bool bFlag)
		{
			SetPSWBit(PSW_FLG::Z, bFlag);
		}

		inline bool     GetZ()
		{
			return GetPSWBit(PSW_FLG::Z);
		}

		inline void     SetN(const bool bFlag)
		{
			SetPSWBit(PSW_FLG::N, bFlag);
		}

		inline bool     GetN()
		{
			return GetPSWBit(PSW_FLG::N);
		}

		inline void     Set_NZ()
		{
			// установка N
			SetN(!!(m_ALU & m_Nbit));

			// установка Z
			if (m_bByteOp)
			{
				SetZ(!LOBYTE(m_ALU));
			}
			else
			{
				SetZ(!LOWORD(m_ALU));
			}
		}
		inline void     Set_V(const uint32_t old)
		{
			// установка V - это смена значения знакового бита. с 0 на 1 (для сложений)
			SetV(!!(~old & m_ALU & m_Nbit));
		}
		inline void     Set_IV(const uint32_t old)
		{
			// установка V - это смена значения знакового бита. с 1 на 0 (для вычитаний)
			SetV(!!(old & ~m_ALU & m_Nbit));
		}
		inline void     Set_C()
		{
			// установка C
			SetC(!!(m_ALU & (m_bByteOp ? 0xffffff00 : 0xffff0000)));
		}

};

extern CScriptAsm g_ScriptAsm;
