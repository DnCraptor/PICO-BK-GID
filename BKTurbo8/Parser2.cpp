#include "pch.h"
#include "Parser2.h"
#include "Parser.h"
#include "Globals.h"
#include "LabelManager.h"
#include "ErrorManager.h"
#pragma warning(disable:4996)
#pragma warning(disable:4146)

CRPNParser g_RPNParser;

CRPNParser::CRPNParser()
	: m_nLastError(0)
	, m_nNames(0)
	, m_nLocNames(0)
	, m_nCounter(0)
{}

CRPNParser::~CRPNParser()
    = default;

// разбор полноценного арифметического выражения
// получение записи в обратной польской нотации
bool CRPNParser::FullAriphmParser(wchar_t &ch)
{
	m_vRPNChain.clear(); // очищаем запись
	m_nNames = 0;       // счётчик встреченных имён меток или констант
	m_nLocNames = 0;    // счётчик встреченных имён локальных меток
	m_nCounter = 0;
	return Expression(ch);
}

void CRPNParser::PushNode(A_OP op)
{
	A_NODE node{ A_TYPE::OPERATION, op };
	m_vRPNChain.push_back(node);
}
void CRPNParser::PushNode(uint32_t num)
{
	A_NODE node{ A_TYPE::NUMBER, num };
	m_vRPNChain.push_back(node);
	m_nCounter++;
}
void CRPNParser::PushNode(CBKToken &token)
{
	A_NODE node{ A_TYPE::LABEL, token };
	m_vRPNChain.push_back(node);
	m_nNames++;
	m_nCounter++;
}
void CRPNParser::PushNodeLL(CBKToken &token)
{
	// добавление локальной метки в качестве операнда арифметического выражения
	A_NODE node{ A_TYPE::LOC_LABEL, token };
	// ищем её в таблице.
	int p = g_labelLocalDefs.SearchLabel(&token);

	if (p >= 0) // если метка найдена, т.е. оша была определена раньше
	{
		node.number = g_labelLocalDefs.GetValue(p); // то возьмём её значение сразу
	}
	else
	{
		node.number = -1; //!!! если не найдена, то метки нету ещё, мы её только собираемся добавлять в таблицу
		// или она будет определена позже.
	}

	/*
	Для локальных меток вводим определённые магические числа:
	-1 - ссылка на ещё пока неопределённую локальную метку. Это типичная ситуация при просмотре вперёд
	-2 - ссылка на так и не определённую локальную метку. Т.е. ситуация, когда была ссылка на локальную метку
	в промежутке между глобальными, но метка так и не встретилась. Это типичная ошибочная ситуация.
	Тут в отличие от ветвлений, нет просмотра вперёд через глобальную метку.
	число >=0 - это локальное значение метки в объектном модуле. Чтобы его можно было скомпоновать с любого
	заданного адреса.

	*/
	m_vRPNChain.push_back(node);
	m_nLocNames++;  // локальная метка именем не считается, она должна вычисляться, как встретится
	m_nCounter++;
}
void CRPNParser::PushNodeDot()
{
	if (g_Globals.GetAriphmType() & ARIPHM_INBRANCH)
	{
		m_nLocNames++;
	}
	else
	{
		m_nNames++; // точка тоже считается именем, ибо невычислимо в объектном файле сразу, если есть
	}

	A_NODE node{ A_TYPE::DOT_PC, static_cast<uint32_t>(g_Globals.GetPC()) };
	m_vRPNChain.push_back(node);
	m_nCounter++;
}

// пересчёт количества имён.
void CRPNParser::RecalcChain(RPNChain &rpn)
{
	m_nNames = m_nLocNames = m_nCounter = 0;

	for (auto &n : rpn)
	{
		switch (n.type)
		{
			case A_TYPE::LABEL:
				m_nNames++;
				m_nCounter++;
				break;

			case A_TYPE::LOC_LABEL:
				m_nLocNames++;
				m_nCounter++;
				break;

			case A_TYPE::DOT_PC:
				if (g_Globals.GetAriphmType() & ARIPHM_INBRANCH)
				{
					m_nLocNames++;
				}
				else
				{
					m_nNames++;
				}

				m_nCounter++;
				break;
		}
	}
}


bool CRPNParser::isOROps(wchar_t ch)
{
	return (ch == L'|' || ch == L'^');
}
bool CRPNParser::isANDOps(wchar_t ch)
{
	return (ch == L'&');
}
bool CRPNParser::isADDOps(wchar_t ch)
{
	return (ch == L'+' || ch == L'-');
}
bool CRPNParser::isMULOps(wchar_t ch)
{
	if (ch == L'*' || ch == L'/' || ch == L'%')
	{
		return true;
	}

	if (ch == L'<' && g_pReader->getNextChar() == ch)
	{
		g_pReader->readChar();
		return true;
	}

	if (ch == L'>' && g_pReader->getNextChar() == ch)
	{
		g_pReader->readChar();
		return true;
	}

	return false;
}


// начало.
bool CRPNParser::Expression(wchar_t &ch)
{
	if (Term_log(ch))
	{
		g_pReader->SkipWhitespaces(ch);

		while (g_pReader->getCurrCharType() != CReader::CHAR_TYPE::LN && isOROps(ch))
		{
			wchar_t op = ch;
			ch = g_pReader->readChar(true);

			if (!Term_log(ch))
			{
				// какая-то ошибка, уже обработанная
				return false;
			}

			switch (op)
			{
				case L'|':
					PushNode(A_OP::OR);
					break;

				case L'^':
					PushNode(A_OP::XOR);
					break;
			}
		}

		return true;
	}

	// какая-то ошибка, уже обработанная
	return false;
}

bool CRPNParser::Term_log(wchar_t &ch)
{
	if (Expr_ar(ch))
	{
		g_pReader->SkipWhitespaces(ch);

		while (g_pReader->getCurrCharType() != CReader::CHAR_TYPE::LN && isANDOps(ch))
		{
			wchar_t op = ch;
			ch = g_pReader->readChar(true);

			if (!Expr_ar(ch))
			{
				// какая-то ошибка, уже обработанная
				return false;
			}

			switch (op)
			{
				case L'&':
					PushNode(A_OP::AND);
					break;
			}
		}

		return true;
	}

	// какая-то ошибка, уже обработанная
	return false;
}
bool CRPNParser::Expr_ar(wchar_t &ch)
{
	if (Term(ch))
	{
		g_pReader->SkipWhitespaces(ch);

		while (g_pReader->getCurrCharType() != CReader::CHAR_TYPE::LN && isADDOps(ch))
		{
			wchar_t op = ch;
			ch = g_pReader->readChar(true);

			if (!Term(ch))
			{
				// какая-то ошибка, уже обработанная
				return false;
			}

			switch (op)
			{
				case L'+':
					PushNode(A_OP::PLUS);
					break;

				case L'-':
					PushNode(A_OP::MINUS);
					break;
			}
		}

		return true;
	}

	// какая-то ошибка, уже обработанная
	return false;
}
bool CRPNParser::Term(wchar_t &ch)
{
	if (Factor(ch))
	{
		g_pReader->SkipWhitespaces(ch);

		while (g_pReader->getCurrCharType() != CReader::CHAR_TYPE::LN && isMULOps(ch))
		{
			wchar_t op = ch;
			ch = g_pReader->readChar(true);

			if (!Factor(ch))
			{
				// какая-то ошибка, уже обработанная
				return false;
			}

			switch (op)
			{
				case L'*':
					PushNode(A_OP::MUL);
					break;

				case L'/':
					PushNode(A_OP::DIV);
					break;

				case L'%':
					PushNode(A_OP::DIV_FR);
					break;

				case L'>':
					PushNode(A_OP::SHR);
					break;

				case L'<':
					PushNode(A_OP::SHL);
					break;
			}
		}

		return true;
	}

	// какая-то ошибка, уже обработанная
	return false;
}

bool CRPNParser::Factor(wchar_t &ch)
{
	wchar_t chEndBracket = 0;

	if (g_pReader->SkipWhitespaces(ch))
	{
		// конец строки
		// Ошибка - конец строки там, где не положено, пока не придумаю, как определить конец арифметического выражения
		// в многострочной записи, конец строки будет ошибкой
		ErrorManager::OutError(ERRNUM::E_134);
		return false;
	}

	switch (ch)
	{
		case L'+':
		{
			// унарный плюс
			ch = g_pReader->readChar(true); // получим следующий символ
			bool bRes = Factor(ch);
			PushNode(A_OP::UN_PLUS); // можно было бы просто игнорировать, но мы честно будем выполнять действие
			return bRes;
		}

		case L'-':
		{
			// унарный минус
			ch = g_pReader->readChar(true); // получим следующий символ
			bool bRes = Factor(ch);
			PushNode(A_OP::UN_MINUS);
			return bRes;
		}

		case L'~':
		{
			// унарный нет - это будет простое инвертирование, а не логическое не
			ch = g_pReader->readChar(true); // получим следующий символ
			bool bRes = Factor(ch);
			PushNode(A_OP::UN_NOT);
			return bRes;
		}

		case L'^':
		{
			if (g_pReader->getNextChar() == L'C')
			{
				// унарный нет в формате макро11
				ch = g_pReader->readChar(true); // получим следующий символ
				ch = g_pReader->readChar(true); // получим следующий символ
				bool bRes = Factor(ch);
				PushNode(A_OP::UN_NOT);
				return bRes;
			}

			break; // иначе пойдём распознавать аргументы
		}

		case L_BRACKET2:
			chEndBracket = R_BRACKET2;
			goto _l_brk;

		case L_BRACKET:
			chEndBracket = R_BRACKET;
_l_brk:
			m_nCounter++;   // если скобка - то самое первое число в арифметическом выражении в ветвлениях

			// не будет интерпретироваться как метка.
			if (!(g_Globals.GetAriphmType() & ARIPHM_NOBRACKETS))
			{
				// открывающая скобка
				ch = g_pReader->readChar(true); // получим следующий символ

				if (Expression(ch))
				{
					// если выражение распарсилось без ошибок
					if (!g_pReader->SkipWhitespaces(ch))
					{
						// если не случился внезапный конец строки
						if (ch == chEndBracket)
						{
							m_nCounter--;
							// если закрывающая скобка на месте
							ch = g_pReader->readChar(true); // прочитаем скобку
							return true; // то только тогда всё в порядке
						}
					}
				}
			}

			ErrorManager::OutError(ERRNUM::E_103); //синтаксическая ошибка
			return false;
	}

	bool bMakeHalfLabel = false;

	// добавим для совместимости деление метки на два
	if (ch == L'/')
	{
		// если за слешем сразу буква, то это то, что надо.
		if (g_pReader->getNextCharType() == CReader::CHAR_TYPE::LETTERS)
		{
			ch = g_pReader->readChar();
			bMakeHalfLabel = true;
		}
		else
		{
			ErrorManager::OutError(ERRNUM::E_103); //синтаксическая ошибка
			return false;
		}
	}

	if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::LETTERS)
	{
		// имя метки
		CBKToken label;
		bool bRes = g_pReader->readToken(&label, ch);
		PushNode(label);

		if (bMakeHalfLabel)
		{
			PushNode(2);
			PushNode(A_OP::DIV);
		}

		return bRes;
	}

	if (ch == L'\'')
	{
		// получение одного символа
		g_Globals.SetInString(true);
		ch = g_pReader->readChar();
		g_Globals.SetInString(false);

		if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::LN)
		{
			ErrorManager::OutError(ERRNUM::E_107); // Ошибка или отсутствие числового аргумента.
			return false; // стоп компиляция.
		}

		int number = UNICODEtoBK_ch(ch);
		PushNode(number);
		ch = g_pReader->readChar(true); // нужно прочитать ещё один раз, чтобы подвинуть курсор
		return true;
	}

	if (ch == L'"')
	{
		// получение двух символов
		g_Globals.SetInString(true);
		ch = g_pReader->readChar();
		g_Globals.SetInString(false);

		if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::LN)
		{
			ErrorManager::OutError(ERRNUM::E_108); // Отсутствуют или недостаточно символов после \"
			return false; // стоп компиляция.
		}

		int number = UNICODEtoBK_ch(ch);
		g_Globals.SetInString(true);
		ch = g_pReader->readChar();
		g_Globals.SetInString(false);

		if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::LN)
		{
			ErrorManager::OutError(ERRNUM::E_108); // Отсутствуют или недостаточно символов после \"
			return false; // стоп компиляция.
		}

		number |= uint16_t(UNICODEtoBK_ch(ch)) << 8;
		PushNode(number);
		ch = g_pReader->readChar(true); // нужно прочитать ещё один раз, чтобы подвинуть курсор
		return true;
	}

	if (ch == L'.') // если текущий символ .
	{
		ch = g_pReader->readChar(true); // получаем следующий символ
		/*
		Тут надо сделать, чтобы возвращалось не значение, а в таблицу добавлялась
		особая ссылка на метку
		*/
		PushNodeDot();
		return true;
	}

	if (ch == L'^') // если текущий символ ^
	{
		int result = 0;

		if (Parser::Macro11NumberParser(result, ch))
		{
			PushNode(result);
			return true;
		}
	}
	else if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::DIGITS)
	{
		// если локальные метки разрешены
		if (!(g_Globals.GetAriphmType() & ARIPHM_NOLOCLABEL))
		{
			// если это первый аргумент, но только в ветвлениях и скобок в арифметическом выражении нет.
			if (g_Globals.GetAriphmType() & ARIPHM_INBRANCH && m_nCounter == 0)
			{
				// то его по умолчанию считаем меткой, так мы можем собирать старый формат
				// где локальная метка - обычное число.
				CBKToken token;
				g_pReader->readToken(&token, ch); // прочитаем токен
				PushNodeLL(token);
				return true;
			}

			// если это уже не первый аргумент, то определяем - число или метка по символу $ на конце
			g_pReader->StoreState(); //сохраним состояние.
			CBKToken token;
			g_pReader->readToken(&token, ch); // прочитаем токен
			auto len = token.getName().length(); // получим размер прочитанного

			if (len > 0 && token.getName().at(len - 1) == L'$') // если что-то есть и последний символ $
			{
				g_pReader->PopState(); // удалить сохранённое состояние
				// то это локальная метка
				PushNodeLL(token);
				return true;
			}

			// иначе - это возможно число
			ch = g_pReader->RestoreState(); // восстановим состояние
		}

		int result = 0;

		if (Parser::AdvancedNumberParser(result, ch))
		{
			// !!! в макро11 отсутствие числового или вообще любого аргумента интерпретируется значением по умолчанию.
			// а здесь аргумент должен присутствовать.
			PushNode(result);
			return true;
		}
	}

	// синтаксическая ошибка
	ErrorManager::OutError(ERRNUM::E_104); // Ошибка или отсутствие числового аргумента.
	return false; // стоп компиляция.
}

// вычисление, или нет, если есть неопределённые метки.
// выход: true - удалось вычислить цепочку, result - достоверный результат
//      false - или не удалось, или ошибка.
//      если result == 0 - то есть ещё в цепочке неопределённая метка
//      если result == -1 - то ошибка в цепочке, и вычислить её невозможно
bool CRPNParser::CalcRpn(int &result, RPNChain &rpn)
{
	RPNChain vStack; // стек элементов

	try
	{
		size_t len = rpn.size();

		for (size_t n = 0; n < len; ++n) // для всей записи
		{
			A_NODE &node = rpn.at(n);

			switch (node.type)
			{
				case A_TYPE::OPERATION:  // если узел - операция

					//выполним операцию
					switch (node.op)
					{
						case A_OP::UN_PLUS:
							if (!vStack.empty())
							{
								A_NODE a = vStack.back();
								vStack.pop_back(); // делаем действие "ничего"
								vStack.push_back(a);
								break;
							}

							m_nLastError = RPNERR_NONEUNOPR;
							throw; // ошибка в цепочке

						case A_OP::UN_MINUS:
							if (!vStack.empty())
							{
								A_NODE a = vStack.back();
								vStack.pop_back();
								a.number = -a.number;
								vStack.push_back(a);
								break;
							}

							m_nLastError = RPNERR_NONEUNOPR;
							throw; // ошибка в цепочке

						case A_OP::UN_NOT:
							if (!vStack.empty())
							{
								A_NODE a = vStack.back();
								vStack.pop_back();
								a.number = ~a.number;
								vStack.push_back(a);
								break;
							}

							m_nLastError = RPNERR_NONEUNOPR;
							throw; // ошибка в цепочке

						case A_OP::PLUS:
							if (!vStack.empty())
							{
								A_NODE a = vStack.back();
								vStack.pop_back();

								if (!vStack.empty())
								{
									A_NODE b = vStack.back();
									vStack.pop_back();
									b.number += a.number;
									vStack.push_back(b);
									break;
								}

								m_nLastError = RPNERR_NONELFTOP;
							}
							else
							{
								m_nLastError = RPNERR_NONERGTOP;
							}

							throw; // ошибка в цепочке

						case A_OP::MINUS:
							if (!vStack.empty())
							{
								A_NODE a = vStack.back();
								vStack.pop_back();

								if (!vStack.empty())
								{
									A_NODE b = vStack.back();
									vStack.pop_back();
									b.number -= a.number;
									vStack.push_back(b);
									break;
								}

								m_nLastError = RPNERR_NONELFTOP;
							}
							else
							{
								m_nLastError = RPNERR_NONERGTOP;
							}

							throw; // ошибка в цепочке

						case A_OP::MUL:
							if (!vStack.empty())
							{
								A_NODE a = vStack.back();
								vStack.pop_back();

								if (!vStack.empty())
								{
									A_NODE b = vStack.back();
									vStack.pop_back();
									b.number *= a.number;
									vStack.push_back(b);
									break;
								}

								m_nLastError = RPNERR_NONELFTOP;
							}
							else
							{
								m_nLastError = RPNERR_NONERGTOP;
							}

							throw; // ошибка в цепочке

						case A_OP::DIV:
							if (!vStack.empty())
							{
								A_NODE a = vStack.back();
								vStack.pop_back();

								if (!vStack.empty())
								{
									A_NODE b = vStack.back();
									vStack.pop_back();
									b.number /= a.number;
									vStack.push_back(b);
									break;
								}

								m_nLastError = RPNERR_NONELFTOP;
							}
							else
							{
								m_nLastError = RPNERR_NONERGTOP;
							}

							throw; // ошибка в цепочке

						case A_OP::DIV_FR:
							if (!vStack.empty())
							{
								A_NODE a = vStack.back();
								vStack.pop_back();

								if (!vStack.empty())
								{
									A_NODE b = vStack.back();
									vStack.pop_back();
									b.number %= a.number;
									vStack.push_back(b);
									break;
								}

								m_nLastError = RPNERR_NONELFTOP;
							}
							else
							{
								m_nLastError = RPNERR_NONERGTOP;
							}

							throw; // ошибка в цепочке

						case A_OP::SHL:
							if (!vStack.empty())
							{
								A_NODE a = vStack.back();
								vStack.pop_back();

								if (!vStack.empty())
								{
									A_NODE b = vStack.back();
									vStack.pop_back();
									b.number <<= a.number;
									vStack.push_back(b);
									break;
								}

								m_nLastError = RPNERR_NONELFTOP;
							}
							else
							{
								m_nLastError = RPNERR_NONERGTOP;
							}

							throw; // ошибка в цепочке

						case A_OP::SHR:
							if (!vStack.empty())
							{
								A_NODE a = vStack.back();
								vStack.pop_back();

								if (!vStack.empty())
								{
									A_NODE b = vStack.back();
									vStack.pop_back();
									b.number >>= a.number;
									vStack.push_back(b);
									break;
								}

								m_nLastError = RPNERR_NONELFTOP;
							}
							else
							{
								m_nLastError = RPNERR_NONERGTOP;
							}

							throw; // ошибка в цепочке

						case A_OP::OR:
							if (!vStack.empty())
							{
								A_NODE a = vStack.back();
								vStack.pop_back();

								if (!vStack.empty())
								{
									A_NODE b = vStack.back();
									vStack.pop_back();
									b.number |= a.number;
									vStack.push_back(b);
									break;
								}

								m_nLastError = RPNERR_NONELFTOP;
							}
							else
							{
								m_nLastError = RPNERR_NONERGTOP;
							}

							throw; // ошибка в цепочке

						case A_OP::XOR:
							if (!vStack.empty())
							{
								A_NODE a = vStack.back();
								vStack.pop_back();

								if (!vStack.empty())
								{
									A_NODE b = vStack.back();
									vStack.pop_back();
									b.number ^= a.number;
									vStack.push_back(b);
									break;
								}

								m_nLastError = RPNERR_NONELFTOP;
							}
							else
							{
								m_nLastError = RPNERR_NONERGTOP;
							}

							throw; // ошибка в цепочке

						case A_OP::AND:
							if (!vStack.empty())
							{
								A_NODE a = vStack.back();
								vStack.pop_back();

								if (!vStack.empty())
								{
									A_NODE b = vStack.back();
									vStack.pop_back();
									b.number &= a.number;
									vStack.push_back(b);
									break;
								}

								m_nLastError = RPNERR_NONELFTOP;
							}
							else
							{
								m_nLastError = RPNERR_NONERGTOP;
							}

							throw; // ошибка в цепочке

						default:
							m_nLastError = RPNERR_OPRERR;
							throw; // ошибка в цепочке
					}

					break;

				case A_TYPE::LABEL:
				{
					// если метка - поищем её в таблице меток
					int p = g_labelGlobalDefs.SearchLabel(&node.token);

					if (p >= 0) // если метка найдена
					{
						// если определение валидно
						if ((g_labelGlobalDefs.GetType(p) & LBL_DEFINITE_MASK) != LBL_WEAKDEFINE)
						{
							int value = g_labelGlobalDefs.GetValue(p);

							if ((g_labelGlobalDefs.GetType(p) & LBL_DEFINITE_MASK) == LBL_GLOBAL)
							{
								value = g_Globals.GetRealAddress(value);
							}

							A_NODE a{ A_TYPE::NUMBER, static_cast<uint32_t>(value) };
							vStack.push_back(a);    // поместим в стек значение метки
							break;
						}
					}
				}

_l_errexit:         // если не найдена - то дальнейшие вычисления невозможны

					// (хотя, в принципе, если есть скобки или вычисления высшего приоритета,
					// то и там можно повычислять, но что-то сложновато выходит, проще застопориться
					// на первой встреченной неопределённой метке.)
					// всё оставшееся - в стек
				for (size_t j = n; j < len; ++j)
				{
					vStack.push_back(rpn.at(j));
				}

				rpn.clear();
				rpn = vStack; // теперь это наша новая цепочка
				RecalcChain(rpn);
				result = 0;
				return false;

				case A_TYPE::LOC_LABEL:

					// если локальная метка, то
					if (int(node.number) >= 0)
					{
						// если локальная метка была найдена, то берём её значение
						int value = node.number + g_Globals.GetLinkerAddr();
						value = g_Globals.GetRealAddress(value);
						A_NODE a{ A_TYPE::NUMBER, static_cast<uint32_t>(value) };
						vStack.push_back(a);    // поместим в стек значение метки
						break;
					}

					if (node.number == -1)
					{
						// если локальная метка не была найдена, это характерно для переходов вперёд
						// поищем её в таблице
						int p = g_labelLocalDefs.SearchLabel(&node.token);

						if (p >= 0) // если метка найдена
						{
							int value = g_labelLocalDefs.GetValue(p);
							value += g_Globals.GetLinkerAddr();
							value = g_Globals.GetRealAddress(value);
							A_NODE a{ A_TYPE::NUMBER, static_cast<uint32_t>(value) };
							vStack.push_back(a);    // поместим в стек значение метки
							break;
						}
					}

					goto _l_errexit;    // а если так и не была найдена, то всё, никогда её не найдём. ошибка

				case A_TYPE::DOT_PC:

					// в режиме CL это не должно применяться, но только если не ветвление
					if ((g_Globals.GetLinkMode() == LINKING_MODE::CL) && !(g_Globals.GetAriphmType() & ARIPHM_INBRANCH))
					{
						goto _l_errexit;
					}

					// и делаем его числом.
					node.number += g_Globals.GetLinkerAddr();
					node.number = g_Globals.GetRealAddress(node.number);
					node.type = A_TYPE::NUMBER;

				//break; и в стек его

				case A_TYPE::NUMBER:
					// если число - поместим его в стек
					vStack.push_back(rpn.at(n));
					break;

				default:
					assert(false);
					m_nLastError = RPNERR_ARGERR;
					throw; // прям вообще ошибка. Других значений быть не должно
			}
		}

		// если попали сюда, то вся цепочка вычислена
		if (!vStack.empty()) // результат - узел в стеке
		{
			A_NODE a = vStack.back();
			vStack.pop_back();
			result = a.number; // достаём

			if (!vStack.empty()) // если в стеке ещё что-то осталось
			{
				m_nLastError = RPNERR_UNEXPECTEDARG;
				throw; // ошибка в цепочке
			}

			return true;    // выходим с корректным результатом
		}

		m_nLastError = RPNERR_UNEXPECTEDEND;
		throw; // ошибка в цепочке
	}
	catch (...)
	{
		rpn.clear(); //очищаем запись
		vStack.clear();
		m_nNames = m_nLocNames = m_nCounter = 0;
		// тут будем обрабатывать ситуацию с ошибкой в цепочке
		result = -1;    // что делать с такой цепочкой, будем решать в другом месте
		// вообще говоря - это будет означать, что алгоритм преобразования выражения
		// в RPN глючит
		return false;
	}
}

bool CRPNParser::CalcRpn(int &result)
{
	return CalcRpn(result, m_vRPNChain);
}

CRPNParser::RPNChain &CRPNParser::GetRPN()
{
	return m_vRPNChain;
}


void CRPNParser::Store(FILE *f)
{
	Store(f, m_vRPNChain);
}

void CRPNParser::Store(FILE *f, RPNChain &rpn)
{
	auto sz = static_cast<uint32_t>(rpn.size());
	OBJTags tag = OBJTags::OBJ_RPNChain;
	fwrite(&tag, 1, sizeof(tag), f);
	fwrite(&sz, 1, sizeof(sz), f);

	for (uint32_t i = 0; i < sz; ++i)
	{
		rpn.at(i).Store(f);
	}
}

bool CRPNParser::Load(FILE *f)
{
	return Load(f, m_vRPNChain);
}

bool CRPNParser::Load(FILE *f, RPNChain &rpn)
{
	OBJTags tag;
	fread(&tag, 1, sizeof(tag), f);

	if (tag == OBJTags::OBJ_RPNChain)
	{
		rpn.clear(); //очищаем
		uint32_t sz = 0;
		size_t r = fread(&sz, 1, sizeof(sz), f);

		if (r == sizeof(sz))
		{
			for (uint32_t i = 0; i < sz; ++i)
			{
				A_NODE node;
				bool bRes = node.Load(f);

				if (bRes)
				{
					rpn.push_back(node);
				}
				else
				{
					return false;
				}
			}

			// если размер вектора 0 - то это не будем считать ошибкой, так записали.
			return true;
		}
	}

	return false;
}

