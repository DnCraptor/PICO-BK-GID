#pragma once
#include "BKToken.h"

#define L_BRACKET L'<'
#define L_BRACKET2 L'['
#define R_BRACKET L'>'
#define R_BRACKET2 L']'

enum
{
	RPNERR_NOERR = 0,
	RPNERR_NONEUNOPR,       // нет операнда унарной операции
	RPNERR_NONERGTOP,       // нет операнда источника в двухоперандной операции
	RPNERR_NONELFTOP,       // нет операнда приёмника в двухоперандной операции
	RPNERR_OPRERR,          // неверная операция
	RPNERR_ARGERR,          // неверный тип операнда
	RPNERR_UNEXPECTEDEND,   // неожиданный конец цепочки
	RPNERR_UNEXPECTEDARG,   // лишние аргументы в выражении
};

class CRPNParser
{
	public:
		enum class A_OP : uint8_t
		{
			NOP,        // Нет операции
			UN_PLUS,    // унарный плюс             + (по идее, должно игнорироваться)
			UN_MINUS,   // унарный минус            -
			UN_NOT,     // унарное нет              ~ (инвертирование)
			PLUS,       // арифм.плюс               +
			MINUS,      // арифм.минус              -
			MUL,        // арифм.умножение          *
			DIV,        // арифм.деление            /
			DIV_FR,     // арифм.получение остатка  %
			SHL,        // арифм.сдвиг влево        << (плохо - совпадает со скобками, но других символов нету)
			SHR,        // арифм.сдвиг вправо       >> (плохо - совпадает со скобками, но других символов нету)
			OR,         // бинарное ИЛИ             |
			XOR,        // бинарное исключающее ИЛИ ^
			AND         // бинарное И               &

			// в принципе можно сделать ещё и логические !, &&, || и операции сравнения <= >= < > == !=
			// но не знай как решить конфликт скобок.
			// или нужно два парсера. один с треугольными скобками для арифметических выражений
			// и второй с круглыми скобками для логическо-арифметических, чтобы использовать в условных операторах
			// больше им применения пока не находится
		};
		enum class A_TYPE : uint8_t
		{
			NUMBER,         // число
			LABEL,          // метка
			LOC_LABEL,      // локальная метка
			DOT_PC,         // ссылка на счётчик команд
			OPERATION       // операция
		};
	protected:
		// структура узла записи арифметического выражения
		class A_NODE
		{
			public:
				A_TYPE      type;   // тип узла
				A_OP        op;     // вид операции
				// операнд, может быть число или метка
				uint32_t    number;
				CBKToken    token;
			public:
				A_NODE()
					: type(A_TYPE::NUMBER)
					, op(A_OP::NOP)
					, number(0)
				{}
				A_NODE(A_TYPE _t, A_OP _o)
					: type(_t)
					, op(_o)
					, number(0)
				{}
				A_NODE(A_TYPE _t, uint32_t _n)
					: type(_t)
					, op(A_OP::NOP)
					, number(_n)
				{}
				A_NODE(A_TYPE _t, CBKToken &_tok)
					: type(_t)
					, op(A_OP::NOP)
					, number(0)
					, token(_tok)
				{}
				void clear()
				{
					type = A_TYPE::NUMBER;
					op = A_OP::NOP;
					number = 0;
					token.clear();
				}

				void Store(FILE *f)
				{
					OBJTags tag = OBJTags::OBJ_Node;
					fwrite(&tag, 1, sizeof(tag), f);
					fwrite(&type, 1, sizeof(type), f);

					switch (type)
					{
						case A_TYPE::NUMBER:
							tag = OBJTags::OBJ_NodeNMR;
							fwrite(&tag, 1, sizeof(tag), f);
							fwrite(&number, 1, sizeof(number), f);
							break;

						case A_TYPE::LOC_LABEL:
							tag = OBJTags::OBJ_NodeLBL;
							fwrite(&tag, 1, sizeof(tag), f);
							fwrite(&number, 1, sizeof(number), f);
							token.Store(f);
							break;

						case A_TYPE::LABEL:
							tag = OBJTags::OBJ_NodeLBL;
							fwrite(&tag, 1, sizeof(tag), f);
							token.Store(f);
							break;

						case A_TYPE::DOT_PC:
							tag = OBJTags::OBJ_NodeDotPC;
							fwrite(&tag, 1, sizeof(tag), f);
							fwrite(&number, 1, sizeof(number), f);
							break;

						case A_TYPE::OPERATION:
							tag = OBJTags::OBJ_NodeOP;
							fwrite(&tag, 1, sizeof(tag), f);
							fwrite(&op, 1, sizeof(op), f);
							break;
					}
				}

				bool Load(FILE *f)
				{
					OBJTags tag;
					fread(&tag, 1, sizeof(tag), f);

					if (tag == OBJTags::OBJ_Node)
					{
						size_t r = fread(&type, 1, sizeof(type), f); //читаем тип операции

						if (r == sizeof(type))
						{
							switch (type)
							{
								case A_TYPE::NUMBER:
									fread(&tag, 1, sizeof(tag), f);

									if (tag == OBJTags::OBJ_NodeNMR)
									{
										op = A_OP::NOP;
										token.clear();
										r = fread(&number, 1, sizeof(number), f);

										if (r == sizeof(number))
										{
											return true;
										}
									}

									break;

								case A_TYPE::LOC_LABEL:
									fread(&tag, 1, sizeof(tag), f);

									if (tag == OBJTags::OBJ_NodeLBL)
									{
										op = A_OP::NOP;
										r = fread(&number, 1, sizeof(number), f);

										if (r == sizeof(number))
										{
											return token.Load(f);
										}
									}

									break;

								case A_TYPE::LABEL:
									fread(&tag, 1, sizeof(tag), f);

									if (tag == OBJTags::OBJ_NodeLBL)
									{
										op = A_OP::NOP;
										number = 0;
										return token.Load(f);
									}

									break;

								case A_TYPE::DOT_PC:
									fread(&tag, 1, sizeof(tag), f);

									if (tag == OBJTags::OBJ_NodeDotPC)
									{
										op = A_OP::NOP;
										token.clear();
										r = fread(&number, 1, sizeof(number), f);

										if (r == sizeof(number))
										{
											return true;
										}
									}

									break;

								case A_TYPE::OPERATION:
									fread(&tag, 1, sizeof(tag), f);

									if (tag == OBJTags::OBJ_NodeOP)
									{
										number = 0;
										token.clear();
										r = fread(&op, 1, sizeof(op), f);

										if (r == sizeof(op))
										{
											return true;
										}
									}

									break;
							}
						}
					}

					return false;
				}
		};

	public:
		using RPNChain = std::vector<A_NODE>;

	protected:
		RPNChain        m_vRPNChain;
		int             m_nLastError;
		int             m_nNames;           // счётчик глобальных имён
		int             m_nLocNames;        // счётчик локальных имён
		int             m_nCounter;         // счётчик операндов, чтобы определить первый.

		void            PushNode(A_OP op);
		void            PushNode(uint32_t num);
		void            PushNode(CBKToken &token);
		void            PushNodeLL(CBKToken &token);
		void            PushNodeDot();

		bool            isOROps(wchar_t ch);
		bool            isANDOps(wchar_t ch);
		bool            isADDOps(wchar_t ch);
		bool            isMULOps(wchar_t ch);

		bool            Expression(wchar_t &ch);
		bool            Term_log(wchar_t &ch);
		bool            Expr_ar(wchar_t &ch);
		bool            Term(wchar_t &ch);
		bool            Factor(wchar_t &ch);

		void            RecalcChain(RPNChain &rpn);

	public:
		CRPNParser();
		~CRPNParser();

		inline int      GetNamesNum() const
		{
			return m_nNames;
		}

		inline int      GetLocNamesNum() const
		{
			return m_nLocNames;
		}

		bool            FullAriphmParser(wchar_t &ch);
		bool            CalcRpn(int &result);
		bool            CalcRpn(int &result, RPNChain &rpn);

		RPNChain       &GetRPN();

		void            Store(FILE *f); // сохранение в объектный файл
		bool            Load(FILE *f);  // чтение из объектного файла
		void            Store(FILE *f, RPNChain &rpn);  // сохранение в объектный файл
		bool            Load(FILE *f, RPNChain &rpn);   // чтение из объектного файла
};

extern CRPNParser g_RPNParser;
