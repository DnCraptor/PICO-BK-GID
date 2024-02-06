#include "pch.h"
#include "LabelTable.h"

#pragma warning(disable:4996)


CLabelTable::CLabelTable()
{
#ifdef _DEBUG
	m_strDumpName = L"_labeltable.txt";
#endif
}


CLabelTable::~CLabelTable()
{
	Clear();
}

void CLabelTable::Clear()
{
	LabelTable.clear();
#ifdef _DEBUG
	Dump();
#endif
}

// выход: true - метка добавлена.
//      false - метка не добавлена, потому что такая уже есть
bool CLabelTable::AddLabel(CBKToken *token, const int value, const uint32_t lt)
{
	if (SearchLabel(token) == -1) // если такая метка не найдена
	{
		// добавляем
		Label lbl{ lt, value, token };
		LabelTable.push_back(lbl);
#ifdef _DEBUG
		Dump();
#endif
		return true;
	}

	return false; // если метка уже есть - сообщим об этом
}

bool CLabelTable::AddLabel(Label &lbl)
{
	if (SearchLabel(lbl.getPToken()) == -1) // если такая метка не найдена
	{
		// добавляем
		LabelTable.push_back(lbl);
#ifdef _DEBUG
		Dump();
#endif
		return true;
	}

	return false; // если метка уже есть - сообщим об этом
}

/*
Выдача имени метки по номеру
*/
CBKToken *CLabelTable::GetLabel(const size_t n)
{
	if (n < getSize()) // если номер в допустимых пределах
	{
		return LabelTable.at(n).getPToken(); // выдаём
	}

	return nullptr; // иначе null
}

/*
Удаление метки из таблицы по номеру
*/
void CLabelTable::DeleteLabel(const size_t n)
{
	if (n < getSize()) // если номер в допустимых пределах
	{
		LabelTable.erase(LabelTable.begin() + n); // стираем
#ifdef _DEBUG
		Dump();
#endif
	}
}

/*
Удаление метки из таблицы по имени
*/
void CLabelTable::DeleteLabel(CBKToken *token)
{
	const int n = SearchLabel(token); // ищем метку

	if (n >= 0) // если нашли
	{
		DeleteLabel(n); // удаляем по номеру
	}
}

/*
поиск метки в таблице определений, если не найдено, возвращает -1
иначе - номер позиции.
*/
int CLabelTable::SearchLabel(CBKToken *token)
{
	const size_t sz = getSize();

	for (size_t i = 0; i < sz; ++i)
	{
		const CBKToken &lt = LabelTable.at(i).getToken();

		if (lt.getHash() == token->getHash())
		{
#ifdef _DEBUG

			if (lt.getName() != token->getName())
			{
				wprintf(L"HASH Error: %s:%#zX ~~ %s:%#zX\n", lt.getName().c_str(), lt.getHash(), token->getName().c_str(), token->getHash());
				assert(false);
			}

#endif
			return static_cast<int>(i);
		}
	}

	return -1; // ничего не найдено
}

/*
Выдача значения метки по имени
*/
int CLabelTable::GetValue(CBKToken *token)
{
	const int n = SearchLabel(token); // ищем метку

	if (n >= 0) // если нашли
	{
		return LabelTable.at(n).getValue(); // возвращаем её значение
	}

	return -1; // если не нашли
}

/*
Выдача значения метки по номеру
*/
int CLabelTable::GetValue(const size_t n)
{
	if (n < getSize()) // если номер в допустимых пределах
	{
		return LabelTable.at(n).getValue(); // возвращаем её значение
	}

	return -1; // если не нашли
}

/*
Выдача типа метки по имени
*/
uint32_t CLabelTable::GetType(CBKToken *token)
{
	const int n = SearchLabel(token);

	if (n >= 0)
	{
		return LabelTable.at(n).getType();
	}

	return LBL_UNKNOWN;
}

/*
Выдача типа метки по номеру
*/
uint32_t CLabelTable::GetType(const size_t n)
{
	if (n < getSize())
	{
		return LabelTable.at(n).getType();
	}

	return LBL_UNKNOWN;
}


CRefsTable::CRefsTable()
{
#ifdef _DEBUG
	m_strDumpName = L"_labeltable.txt";
#endif
}


CRefsTable::~CRefsTable()
{
	Clear();
}

void CRefsTable::Clear()
{
	RefsTable.clear();
#ifdef _DEBUG
	Dump();
#endif
}

bool CRefsTable::AddRefs(CBKToken *token, const int addr, uint32_t lt)
{
	// если токена нет, заменим его значением по умолчанию
	CBKToken tok = ((lt & ARL_DEFINITE_MASK) != ARL_DEFINE) ? CBKToken{} : (token ? *token : CBKToken{});

	if (g_RPNParser.GetLocNamesNum())
	{
		lt |= ARL_LOCAL;
	}

	LabelRef lbl{ lt, addr, &tok, g_RPNParser.GetRPN() };
	RefsTable.push_back(lbl);
#ifdef _DEBUG
	Dump();
#endif
	return true;
}

bool CRefsTable::AddRefs(LabelRef &lbl)
{
	RefsTable.push_back(lbl);
#ifdef _DEBUG
	Dump();
#endif
	return true;
}

uint32_t CRefsTable::GetType(const size_t n)
{
	if (n < getSize())
	{
		return RefsTable.at(n).getType();
	}

	return LBL_UNKNOWN;
}

void CRefsTable::DeleteElement(const size_t n)
{
	if (n < getSize()) // если номер в допустимых пределах
	{
		RefsTable.erase(RefsTable.begin() + n); // стираем
	}
}

// при добавлении глобальной метки, нужно пробежаться по таблице ссылок, и отметить
// все ссылки на локальные метки не в ветвлениях недействительными.
void CRefsTable::LockUndefinedLocalLabels()
{
	// просмотрим таблицу
	for (auto &ref : RefsTable)  // для всех ссылок на метки
	{
		uint32_t lti = ref.getType();

		if ((lti & ARL_LOCAL) && ((lti & REFERENCE_TYPE_MASK) != ARL_BRANCH_LABEL))
		{
			CRPNParser::RPNChain &rpn = ref.getRPN();

			for (auto &n : rpn)
			{
				if (n.type == CRPNParser::A_TYPE::LOC_LABEL && n.number == -1) // если значение метки было неопределено
				{
					n.number = -2;  // то метка будет недействительна и это ошибка - ссылка не неопределённую метку.
				}
			}
		}
	}

#ifdef _DEBUG
	Dump();
#endif
}

void CRefsTable::FixLocalLabels(CBKToken *token, const int value)
{
	// просмотрим таблицу
	for (auto &ref : RefsTable) // для всех ссылок на метки
	{
		uint32_t lti = ref.getType();

		if ((lti & ARL_LOCAL) && ((lti & REFERENCE_TYPE_MASK) != ARL_BRANCH_LABEL))
		{
			CRPNParser::RPNChain &rpn = ref.getRPN();

			for (auto &n : rpn)
			{
				if (n.type == CRPNParser::A_TYPE::LOC_LABEL && n.number == -1) // если значение метки было неопределено
				{
					if (n.token.getHash() == token->getHash())
					{
						n.number = value; // то метка теперь будет иметь такое значение
					}
				}
			}
		}
	}

#ifdef _DEBUG
	Dump();
#endif
}

#ifdef _DEBUG
void CLabelTable::SetDumpName(const std::wstring &strDumpName)
{
	m_strDumpName = strDumpName;
}

void CLabelTable::Dump()
{
	FILE *fd = _wfopen(m_strDumpName.c_str(), L"wt");
	auto sz = LabelTable.size();

	if (sz)
	{
		fwprintf(fd, L"Table size : %zd.\n", sz);
		fwprintf(fd, L"Type      Value     Name.\n");

		for (auto &lbl : LabelTable)
		{
			uint32_t t = lbl.getType();

			switch (t & LBL_DEFINITE_MASK)
			{
				case LBL_UNKNOWN:
					fwprintf(fd, L"UNKNOWN   ");
					break;

				case LBL_GLOBAL:
					fwprintf(fd, L"Global    ");
					break;

				case LBL_LOCAL:
					fwprintf(fd, L"Local     ");
					break;

				case LBL_DEFINE:
					fwprintf(fd, L"Define    ");
					break;

				case LBL_WEAKDEFINE:
					fwprintf(fd, L"*Define   ");
					break;
			}

			int v = lbl.getValue();
			CBKToken tok = lbl.getToken();
			fwprintf(fd, L"%07o   %s\n", v & 0xffff, tok.getName().c_str());
		}
	}
	else
	{
		fwprintf(fd, L"Empty.\n");
	}

	fclose(fd);
}

void CRefsTable::SetDumpName(const std::wstring &strDumpName)
{
	m_strDumpName = strDumpName;
}

void CRefsTable::Dump()
{
	FILE *fd = _wfopen(m_strDumpName.c_str(), L"wt");
	auto sz = RefsTable.size();

	if (sz)
	{
		fwprintf(fd, L"Table size : %zd.\n", sz);
		fwprintf(fd, L"Type      Flags       Addr    Define.\n");

		for (auto &ref : RefsTable)
		{
			uint32_t t = ref.getType();

			switch (t & ARL_DEFINITE_MASK)
			{
				case ARL_DEFINE:
					fwprintf(fd, L"Define    ");
					break;

				case ARL_CMDARG:
					fwprintf(fd, L"Argument  ");
					break;

				default:
					fwprintf(fd, L"ERROR!    ");
			}

			if (t & ARL_LOCAL)
			{
				fwprintf(fd, L"L");
			}
			else
			{
				fwprintf(fd, L" ");
			}

			if (t & ARL_BYTEL)
			{
				fwprintf(fd, L"B");
			}
			else
			{
				fwprintf(fd, L" ");
			}

			switch (t & REFERENCE_TYPE_MASK)
			{
				case ARL_BRANCH_LABEL:
					fwprintf(fd, L" Branch   ");
					break;

				case ARL_OFFSET_LABEL:
					fwprintf(fd, L" Offset   ");
					break;

				case ARL_RELATIVE_LABEL:
					fwprintf(fd, L" Relative ");
					break;

				default:
					fwprintf(fd, L" Regular  ");
			}

			int v = ref.getAddress();
			CBKToken tok = ref.getDefine();
			fwprintf(fd, L"%07o   %s\n", v & 0xffff, tok.getName().c_str());
			// Chain
			CRPNParser::RPNChain chain = ref.getRPN();
			auto sz1 = chain.size();

			if (sz1)
			{
				fwprintf(fd, L"%zd:\t", sz1);

				for (size_t n = 0; n < sz1; ++n)
				{
					CRPNParser::A_TYPE t = chain.at(n).type;

					switch (t)
					{
						case CRPNParser::A_TYPE::NUMBER:
							fwprintf(fd, L"NUM:%07o ", chain.at(n).number & 0xffff);
							break;

						case CRPNParser::A_TYPE::LABEL:
							fwprintf(fd, L"GL:%s ", chain.at(n).token.getName().c_str());
							break;

						case CRPNParser::A_TYPE::LOC_LABEL:
							fwprintf(fd, L"LL:%s ", chain.at(n).token.getName().c_str());
							break;

						case CRPNParser::A_TYPE::DOT_PC:
							fwprintf(fd, L"DOT:%07o ", chain.at(n).number & 0xffff);
							break;

						case CRPNParser::A_TYPE::OPERATION:
							fwprintf(fd, L"OP:");

							switch (chain.at(n).op)
							{
								case CRPNParser::A_OP::NOP:
									fwprintf(fd, L"nop ");
									break;

								case CRPNParser::A_OP::UN_PLUS:
									fwprintf(fd, L"+u ");
									break;

								case CRPNParser::A_OP::UN_MINUS:
									fwprintf(fd, L"-u ");
									break;

								case CRPNParser::A_OP::UN_NOT:
									fwprintf(fd, L"~u ");
									break;

								case CRPNParser::A_OP::PLUS:
									fwprintf(fd, L"+ ");
									break;

								case CRPNParser::A_OP::MINUS:
									fwprintf(fd, L"- ");
									break;

								case CRPNParser::A_OP::MUL:
									fwprintf(fd, L"* ");
									break;

								case CRPNParser::A_OP::DIV:
									fwprintf(fd, L"/ ");
									break;

								case CRPNParser::A_OP::DIV_FR:
									fwprintf(fd, L"%% ");
									break;

								case CRPNParser::A_OP::SHL:
									fwprintf(fd, L"<< ");
									break;

								case CRPNParser::A_OP::SHR:
									fwprintf(fd, L">> ");
									break;

								case CRPNParser::A_OP::OR:
									fwprintf(fd, L"| ");
									break;

								case CRPNParser::A_OP::XOR:
									fwprintf(fd, L"^ ");
									break;

								case CRPNParser::A_OP::AND:
									fwprintf(fd, L"& ");
									break;

								default:
									fwprintf(fd, L"err ");
							}

							break;

						default:
							fwprintf(fd, L"ERR ");
					}
				}

				fwprintf(fd, L"\n");
			}
			else
			{
				fwprintf(fd, L"\tEmpty chain!\n");
			}
		}
	}
	else
	{
		fwprintf(fd, L"Empty.\n");
	}

	fclose(fd);
}

#endif
