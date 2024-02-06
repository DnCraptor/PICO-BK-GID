#include "pch.h"
#include "Parser.h"
#include "Parser2.h"
#include "Globals.h"
#include "LabelTable.h"
#include "LabelManager.h"
#include "ErrorManager.h"
#pragma warning(disable:4996)


static const std::vector <Registers_t> FPURegNames =
{
	{ 0, {CBKToken(L"AC0")} },
	{ 1, {CBKToken(L"AC1")} },
	{ 2, {CBKToken(L"AC2")} },
	{ 3, {CBKToken(L"AC3")} },
	{ 4, {CBKToken(L"AC4")} },
	{ 5, {CBKToken(L"AC5")} },
	{ 6, {CBKToken(L"AC6")} },
	{ 7, {CBKToken(L"AC7")} }
};


static const wchar_t RADIX50[050] =
{
	// 000..007
	L' ', L'A', L'B', L'C', L'D', L'E', L'F', L'G',
	// 010..017
	L'H', L'I', L'J', L'K', L'L', L'M', L'N', L'O',
	// 020..027
	L'P', L'Q', L'R', L'S', L'T', L'U', L'V', L'W',
	// 030..037
	L'X', L'Y', L'Z', L'$', L'.', L' ', L'0', L'1',
	// 040..047
	L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9'
};


// установка в опкоде имени регистра или адресации
void Parser::SetAddresationRegister(int data)
{
	if (g_Globals.GetOperandType() == OPERAND_TYPE::SRC)
	{
		data <<= 6; // формируем источник
	}

	g_Memory.w[g_Globals.GetPC() / 2] |= data;
}

int Parser::GetAddresationRegister()
{
	int data = g_Memory.w[g_Globals.GetPC() / 2];

	if (g_Globals.GetOperandType() == OPERAND_TYPE::SRC)
	{
		data >>= 6; // формируем источник
	}

	return data;
}

// прочитать и опознать имя регистра
bool Parser::ReadRegName(wchar_t &ch)
{
	CBKToken reg;

	if (g_pReader->readToken(&reg, ch))
	{
		if (ParseRegName(&reg))
		{
			return true;
		}
	}

	return false;
}

/*
опознать имя регистра
вход: reg - прочитанное имя
выход: true - имя регистра опознано
       false - не опознано
*/
bool Parser::ParseRegName(CBKToken *reg)
{
	for (auto &RegName : g_RegNames)
	{
		for (auto &name : RegName.names)
		{
			if (name.getHash() == reg->getHash())
			{
				SetAddresationRegister(RegName.nNum);
				return true;
			}
		}
	}

	return false;
}

bool Parser::ParseFPURegName(CBKToken *reg, bool bFirst)
{
	if (bFirst)
	{
		// надо ограничить имена первыми четырьмя.
		for (int i = 0; i < 4; ++i)
		{
			if (FPURegNames.at(i).names.at(0).getHash() == reg->getHash())
			{
				SetAddresationRegister(FPURegNames[i].nNum);
				return true;
			}
		}

		return false;
	}

	for (auto &RegName : FPURegNames)
	{
		if (RegName.names.at(0).getHash() == reg->getHash())
		{
			SetAddresationRegister(RegName.nNum);
			return true;
		}
	}

	return false;
}

// проверка (Rn)
// выход:
// 0 - всё нормально
// 1 - не распознанный регистр
// 2 - отсутствует открывающая скобка
// 3 - отсутствует закрывающая скобка
int Parser::CheckReg(wchar_t &ch)
{
	if (ch != L'(')
	{
		return 2;
	}

	ch = g_pReader->readChar();

	if (!ReadRegName(ch))
	{
		return 1;
	}

	if (ch != L')')
	{
		return 3;
	}

	ch = g_pReader->readChar();
	return 0;
}


bool Parser::AddRegSynonim(CBKToken *reg, CBKToken *synonim)
{
	wchar_t num = reg->getName().at(1) - L'0'; // номер регистра

	if (0 <= num && num <= 7)
	{
		// сперва поищем, нету ли уже такого.
		for (auto &n : g_RegNames.at(num).names)
		{
			if (n.getHash() == synonim->getHash())
			{
				return false; // уже есть, не надо
			}
		}

		g_RegNames.at(num).names.push_back(*synonim); // добавим новый синоним
		return true;
	}

	return false;
}

// разбор операнда
bool Parser::Operand_analyse(int &cp, wchar_t &ch, bool bFPU)
{
	int result = 0;
	bool bNeedAddRefs = false;
	g_pReader->SkipWhitespaces(ch); // пропустим на всякий случай пробелы
	CReader::CHAR_TYPE ct = g_pReader->getCurrCharType(); // какой символ у нас текущий?
	CBKToken token;

	if (ct == CReader::CHAR_TYPE::LN) // строка закончилась
	{
		goto err115; // ошибка в команде
	}

	if (ch == L'@') // если первый символ - собака, то это относительная адресация
	{
		bFPU = false; // FPU регистры запрещены для адресаций !0
		SetAddresationRegister(010);    // фиксируем относительную адресацию
		ch = g_pReader->readChar();     // читаем следующий символ
		ct = g_pReader->getCurrCharType();
	}

	if (ct == CReader::CHAR_TYPE::SPACES || ct == CReader::CHAR_TYPE::LN) // если за собакой пусто или конец строки
	{
err103:
		ErrorManager::OutError(ERRNUM::E_123); // Неверная адресация.
		return false;
	}

	if (ch == L'%') // если %, то это имя регистра
	{
		g_pReader->StoreState();

		if (g_pReader->readTokenR(&token, ch)) // читаем лексему
		{
			if (ParseRegName(&token)) // если это имя регистра
			{
				cp -= 2; // регистровая операция
				g_pReader->PopState();
				return true;
			}
		}

		// если не имя регистра, значит ошибка
		g_pReader->PopState();
		goto err113;
	}

	if (ct == CReader::CHAR_TYPE::LETTERS) // если буква - то это или метка или имя регистра
	{
		g_pReader->StoreState();
		g_pReader->readToken(&token, ch); // читаем лексему

		if (bFPU) // если FPU инструкция, то для адресации 0 исп. FPU регистры
		{
			if (ParseFPURegName(&token, false))
			{
				cp -= 2; // регистровая операция
				g_pReader->PopState();
				return true;
			}
		}
		else // для простых инструкций - РОН.
		{
			if (ParseRegName(&token)) // если это имя регистра
			{
				cp -= 2; // регистровая операция
				g_pReader->PopState();
				return true;
			}
		}

		// если не имя регистра, значит имя метки
		ch = g_pReader->RestoreState();
		// если это метка - надо её обработать и разобрать арифметическое выражение
		goto l_digits;
	}
	else if (ct == CReader::CHAR_TYPE::DIGITS || ch == L'^' || (ch == L_BRACKET || ch == L_BRACKET2))
	{
		// если цифра - то это индекс адресации 6, или число в адресации 67
		// если открывающая скобка - то это арифметическое выражение, а за ним - или индекс адресации 6, или число в адресации 67
l_digits:

		// разбираем арифметическое выражение
		if (g_RPNParser.FullAriphmParser(ch)) // если распарсилось
		{
			if (g_RPNParser.GetNamesNum() || g_RPNParser.GetLocNamesNum()) // если там были имена меток или других определений
			{
				bNeedAddRefs = true;
			}
			else if (!g_RPNParser.CalcRpn(result)) // были только числа - вычисляем.
			{
				assert(false); // такого быть не должно, надо разбираться.
				return false;
			}
		}
		else
		{
			return false;
		}

		int ret = CheckReg(ch); // за арифм выражением может быть имя регистра в скобках

		switch (ret)
		{
			case 1:
err113:         // открывающая скобка есть, но за ней не имя регистра
				ErrorManager::OutError(ERRNUM::E_111); // Ошибка в имени регистра.
				return false;

			case 2:

				// открывающей скобки нет - предполагаем адресацию [10]+67

				// если включен enabl то адресацию 67 нужно трактовать как 37.
				// а адресацию 77 - нельзя, оно не преобразовывается
				if (g_Globals.isEnabl() && (GetAddresationRegister() & 070) == 0)
				{
					if (bNeedAddRefs)
					{
						// в таблицу ссылок добавим невычисленную цепочку выражения
						LabelManager::AddLabelReference(nullptr, cp, ARL_CMDARG | ARL_RELATIVE_LABEL);
					}

					SetAddresationRegister(037);
				}
				else
				{
					if (bNeedAddRefs)
					{
						// в таблицу ссылок добавим невычисленную цепочку выражения
						LabelManager::AddLabelReference(nullptr, cp, ARL_CMDARG | ARL_OFFSET_LABEL);
					}
					else
					{
						result -= (g_Globals.GetRealAddress(cp) + 2);
					}

					SetAddresationRegister(067);
				}

				g_Memory.w[cp / 2] = result;
				return true;

			case 3:
err115:         // открывающая скобка есть и регистр есть, а закрывающий нет
				ErrorManager::OutError(ERRNUM::E_123); // Ошибка в команде.
				return false;
		}

		// всё есть - регистр в скобках - адресация [10]+6х
		if (bNeedAddRefs)
		{
			// в таблицу ссылок добавим невычисленную цепочку выражения
			LabelManager::AddLabelReference(nullptr, cp, ARL_CMDARG | ARL_RELATIVE_LABEL);
		}

		SetAddresationRegister(060); // всё есть - регистр в скобках - адресация 7х
		g_Memory.w[cp / 2] = result;
		return true;
	}
	else
	{
		if (ch == L'#') // если текущий символ - решётка, (перед ним может быть собака)
		{
			SetAddresationRegister(027); // значит это непосредственная адресация
			ch = g_pReader->readChar(); // смотрим следующий символ

			// разбираем арифметическое выражение
			if (g_RPNParser.FullAriphmParser(ch)) // если распарсилось
			{
				if (g_RPNParser.GetNamesNum() || g_RPNParser.GetLocNamesNum()) // если там были имена меток или других определений
				{
					// в таблицу ссылок добавим невычисленную цепочку выражения
					LabelManager::AddLabelReference(nullptr, cp, ARL_CMDARG | ARL_RELATIVE_LABEL);
				}
				else if (g_RPNParser.CalcRpn(result)) // были только числа - вычисляем.
				{
					g_Memory.w[cp / 2] = result; // если это было правильное выражение - зафиксируем
				}
				else
				{
					assert(false); // такого быть не должно, надо разбираться.
				}

				return true;
			}

			return false;
		}

		if (ch == L'-') // если текущий символ - минус, то это может быть декремент или арифм выражение
		{
			ct = g_pReader->getNextCharType(); // смотрим символ за минусом

			if (ct != CReader::CHAR_TYPE::OTHERS || g_pReader->getNextChar() == L'^') // если за минусом буква, цифра, пусто или конец строки
			{
				goto l_digits;      // идём обрабатывать арифметическое выражение
			}

			ch = g_pReader->readChar(); // иначе - прочитаем символ за минусом
			int ret = CheckReg(ch); // там ожидается регистр в скобках

			switch (ret)
			{
				case 1:
					goto err113; // открывающая скобка есть, но за ней не имя регистра

				case 2:
					goto err103; // открывающий скобки нет. там какая-то ерунда

				case 3:
					goto err115; // открывающая скобка есть и регистр есть, а закрывающий нет
			}

			SetAddresationRegister(040); // всё нормально - декрементная операция
		}
		else // далее или относительная или инкрементная операции
		{
			g_pReader->StoreState();
			int ret = CheckReg(ch); // там ожидается регистр в скобках

			switch (ret)
			{
				case 1:
					g_pReader->PopState();
					goto err113; // открывающая скобка есть, но за ней не имя регистра

				case 2:
					ch = g_pReader->RestoreState();
					goto l_digits; // открывающий скобки нет. там какая-то ерунда

				case 3:
					g_pReader->PopState();
					goto err115; // открывающая скобка есть и регистр есть, а закрывающий нет
			}

			g_pReader->PopState();

			// всё нормально - уточним операцию
			if (ch == L'+') // если сразу за закрывающей скобкой есть +
			{
				ch = g_pReader->readChar(); // его прочитаем
				SetAddresationRegister(020); // и это инкрементная операция
			}
			else
			{
				// если конструкция @(Rn) то ошибка
				// а MACRO-11 интерпретирует её как @0(Rn)
				if ((GetAddresationRegister() & 070) == 010)
				{
					goto err103; // не допустим этого
				}

				SetAddresationRegister(010); // нет плюса - относительная
			}
		}

		cp -= 2; // регистровая операция
	}

	return true;
}

// поиск заданного символа.
// если найден - то остановка на нём и выход - true
// если не найден - выход - false и остановка на том, что найдено
bool Parser::needChar(wchar_t nch, wchar_t &ch)
{
	bool bln = g_pReader->SkipWhitespaces(ch); // пропускаем возможные пробелы

	if (!bln) // если не конец строки
	{
		return (ch == nch); // смотрим, то ли нашли, что нам нужно.
	}

	return false;
}


////////////////////////////////////////////////////////////////////////
// парсер чисел.

bool Parser::HexNumberParser(wchar_t &ch, int &ret)
{
	bool bOk = true;
	ret = 0;

	while (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::DIGITS || g_pReader->getCurrCharType() == CReader::CHAR_TYPE::LETTERS)
	{
		int d = 0;

		if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::DIGITS)
		{
			d = (ch - L'0');
		}
		else if (L'A' <= ch && ch <= L'F')
		{
			d = 10 + (ch - L'A');
		}
		else
		{
			bOk = false;
			break;
		}

		ret *= 16;
		ret += d;
		ch = g_pReader->readChar();
	}

	return bOk;
}

bool Parser::DecNumberParser(wchar_t &ch, int &ret)
{
	ret = 0;

	while (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::DIGITS)
	{
		ret *= 10;
		ret += (ch - L'0');
		ch = g_pReader->readChar();
	}

	return true;
}

bool Parser::OctNumberParser(wchar_t &ch, int &ret)
{
	ret = 0;
	bool bOk = true;

	while (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::DIGITS)
	{
		if (L'0' <= ch && ch <= L'7')
		{
			ret *= 8;
			ret += ch & 7;
		}
		else
		{
			// если встретится 8 или 9 считаем их не цифрами и говорим, что ошибка парсинга
			bOk = false;
			break;
		}

		ch = g_pReader->readChar();
	}

	return bOk;
}

bool Parser::BinNumberParser(wchar_t &ch, int &ret)
{
	bool bOk = true;
	ret = 0;

	while (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::DIGITS)
	{
		if (L'0' <= ch && ch <= L'1')
		{
			ret *= 2;
			ret += ch & 1;
		}
		else
		{
			bOk = false;
			break;
		}

		ch = g_pReader->readChar();
	}

	return bOk;
}


/*взято из кросс ассемблера MACRO-11
 Copyright (c) 2001, Richard Krehbiel
 формат внутреннего представления плавающего числа.
 первое слово: бит 15 - знак, затем 8 бит - порядок, затем 7 бит - мантисса
 второе слово: продолжение мантиссы
 третье и 4е слово: продолжение мантиссы

 соответственно точность определяется отбрасыванием 2, 3 и 4 слов мантиссы
 */

/* Parse PDP-11 64-bit floating point format. */
/* Give a pointer to "size" words to receive the result. */
/* Note: there are probably degenerate cases that store incorrect
results.  For example, I think rounding up a FLT2 might cause
exponent overflow.  Sorry. */
/* Note also that the full 49 bits of precision probably aren't
available on the source platform, given the widespread application
of IEEE floating point formats, so expect some differences.  Sorry
again. */
bool Parser::parse_float(wchar_t &ch,
                         int size,   // 1 для ^f; 2 для .FLT2; 4 для .FLT4
                         uint16_t *flt) // массив из 1..4х слов, куда сохраняется результат
{
	// сперва надо прочитать в буфер всё число.
	std::wstring str;

	while (g_pReader->getCurrCharType() != CReader::CHAR_TYPE::SPACES && g_pReader->getCurrCharType() != CReader::CHAR_TYPE::LN)
	{
		// вот ещё одна проверка на посторонние символы
		if (ch == L'+' || ch == L'-' || ch == L'E' || ch == L'.' || g_pReader->getCurrCharType() == CReader::CHAR_TYPE::DIGITS)
		{
			if (ch == L'.')
			{
				ch = L','; // меняем точку на запятую, потому что для scanf нужна ','
			}

			str.push_back(ch); // соберём все цифры в строку.
			ch = g_pReader->readChar();
		}
		else
		{
			break;
		}
	}

	for (int i = 0; i < size; ++i)
	{
		flt[i] = 0;  // очищаем результат
	}

	double d = 0;   // значение
	int n = 0;
	int i = swscanf(str.c_str(), L"%lf%n", &d, &n); // парсим число, на выходе i == кол-во сконвертированных полей

	if (i == 0)
	{
		ErrorManager::OutError(ERRNUM::E_128); // сообщим об этом
		return false;    // не получилось сконвертировать.
	}

	// если обработали меньше символов, чем захватили - то захватили
	// какие-то посторонние символы, которых быть там не должно
	if (size_t(n) < str.length())
	{
		ErrorManager::OutError(ERRNUM::E_103); // сообщим об этом
	}

	uint16_t uexp = 0;              // экспонента
	uint64_t ufrac = 0;             // мантисса
	uint16_t sign = 0;              // маска знака

	if (d != 0.0)
	{
		int sexp;   // знаковый порядок
		double frac = frexp(d, &sexp);  // отделяем порядок и мантиссу

		if (frac < 0)
		{
			sign = 0100000;             // знак минуса
			frac = -frac;               // корректируем мантиссу
		}

		/* это длинное число - это 2 в степени 56 */
		ufrac = static_cast<uint64_t>(frac * 72057594037927936.0);  // выравниваем биты мантиссы

		/* Round from FLT4 to FLT2 */
		if (size < 4)
		{
			if (size < 2)
			{
				ufrac += 0x800000000000;    // округляем для 16-битного представления
			}
			else
			{
				ufrac += 0x80000000;        // округляем для 32-битного представления
			}

			if (ufrac >= 0x100000000000000) // переполнение? (это то же число 2 в степени 56)
			{
				ufrac >>= 1;                // нормализуем
				sexp--;
			}
		}

		if (sexp < -128 || sexp > 127)
		{
			// порядок выходит за пределы?
			ErrorManager::OutError(ERRNUM::E_128); // сообщим об этом
			return false;
		}

		uexp = sexp + 128;              // делаем беззнаковый порядок - 128
		uexp &= 0xff;                   // оставляем только младший байт
	}

	// если d == 0.0, то сохраняем результат - точный 0
	flt[0] = static_cast<uint16_t>((static_cast<uint64_t>(sign) | (static_cast<uint64_t>(uexp) << 7) | ((ufrac >> 48) & 0x7F))); // первое слово

	if (size > 1)
	{
		flt[1] = static_cast<uint16_t>((ufrac >> 32) & 0xffff); // второе слово

		if (size > 2)
		{
			flt[2] = static_cast<uint16_t>((ufrac >> 16) & 0xffff);  // третье
			flt[3] = static_cast<uint16_t>(ufrac & 0xffff);          // четвёртое
		}
	}

	return true;
}


/*
парсер числа
выход: true - число в result достоверно
false - было переполнение, и число в result недостоверно

Попадая сюда имеем лексему, гарантированно начинающуюся с цифры

префиксная форма - без проблем.
0xabcd - 16ричное число с префиксом
0d9999 - 10чное число с префиксом - даж не знаю, нужно ли.
07777 - 8 ричное число с префиксом
0b1111 - двоичное число с префиксом
0o777 - сделать можно, но бессмысленно
9999. - 10чное число с суффиксом  - без проблем
7777 - 8 ричное число по умолчанию  - без проблем
*/
bool Parser::AdvancedNumberParser(int &result, wchar_t &ch)
{
	int nTmp = 0;
	bool bRet = true;
	std::wstring str;
	bool bHas8 = false;

	if (ch == L'0') // если первая цифра 0
	{
		// и если за нулём спец код
		if (g_pReader->getNextChar() == L'X')
		{
			ch = g_pReader->readChar();
			ch = g_pReader->readChar();
			// парсим 16ричное число
			bRet = HexNumberParser(ch, nTmp);
			goto l_aex;
		}
		else if (g_pReader->getNextChar() == L'D')
		{
			ch = g_pReader->readChar();
			ch = g_pReader->readChar();
			// парсим 10чное число
			bRet = DecNumberParser(ch, nTmp);
			goto l_aex;
		}
		else if (g_pReader->getNextChar() == L'B')
		{
			ch = g_pReader->readChar();
			ch = g_pReader->readChar();
			// парсим 2чное число
			bRet = BinNumberParser(ch, nTmp);
			goto l_aex;
		}
	}

	// а иначе, парсим просто число, 8чное или 10чное, зависит от точки на конце.
	// конструкция 0123. допустима, и считается десятичным числом, несмотря на ведущий 0

	while (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::DIGITS)
	{
		str.push_back(ch); // соберём все цифры в строку.

		// заодно проверим, есть ли в числе цифры 8 и 9
		if (ch == L'8' || ch == L'9')
		{
			bHas8 = true;
		}

		ch = g_pReader->readChar();
	}

	if (ch == L'.') // если завершающий символ точка
	{
		ch = g_pReader->readChar(true); // точку тоже захватим
		// парсим 10чное число

		if (str.empty())
		{
			bRet = false;
		}
		else
		{
			nTmp = std::stoi(str, nullptr, 10);
		}
	}
	else if (bHas8)
	{
		// ошибка, десятичное число без точки
		nTmp = 0;
		bRet = false;
	}
	else
	{
		// парсим 8чное число
		if (str.empty())
		{
			bRet = false;
		}
		else
		{
			nTmp = std::stoi(str, nullptr, 8);
		}
	}

l_aex:
	result = nTmp;

	if (!bRet)
	{
		ErrorManager::OutError(ERRNUM::E_131); // ошибка в числе
	}

	if (nTmp > 65535)
	{
		result &= 0xffff;
		ErrorManager::OutError(ERRNUM::E_126); // переполнение слова
		return false;
	}

	return bRet;
}

// если при разборе арифметического выражения встретился символ ^,
// то его читаем и вызываем эту функцию,
// на входе: ch - следующий за ^ символ.
/*
префиксная форма макро11:
^xabcd - 16ричное число с префиксом
^habcd - 16ричное число с префиксом
^d9999 - 10чное число с префиксом
^o7777 - 8 ричное число с префиксом
^b1111 - двоичное число с префиксом
^f12.3 - плавающее число.
^rabc - три символа в коде Radix - 50
^c - следующий операнд надо будет инвертировать
*/
bool Parser::Macro11NumberParser(int &result, wchar_t &ch)
{
	int nTmp = 0;
	ch = g_pReader->readChar();
	bool bRet = true;

	switch (ch)
	{
		case L'X':
		case L'H':
			ch = g_pReader->readChar();
			// парсим 16ричное число
			bRet = HexNumberParser(ch, nTmp);
			break;

		case L'D':
			ch = g_pReader->readChar();
			// парсим 10чное число
			bRet = DecNumberParser(ch, nTmp);
			break;

		case L'O':
			ch = g_pReader->readChar();
			// парсим 8чное число
			bRet = OctNumberParser(ch, nTmp);
			break;

		case L'B':
			ch = g_pReader->readChar();
			// парсим 2чное число
			bRet = BinNumberParser(ch, nTmp);
			break;

		case L'F':
		{
			ch = g_pReader->readChar();
			uint16_t flt[2] = { 0 };

			// парсим плавающее число
			if (bRet = parse_float(ch, 1, flt))
			{
				nTmp = flt[0];
			}

			break;
		}

		case L'R':
		{
			ch = g_pReader->readChar();
			// парсим радикс
			result = 0;
			int nMultipler = 03100;

			for (int i = 0; i < 3; ++i) // берём три следующих символа
			{
				int nChCode = 0;

				for (int n = 0; n < 050; ++n)
				{
					if (RADIX50[n] == ch)
					{
						nChCode = n;    // нашли
						break;
					}
				}

				// неверные символы радикс просто считаем пробелами.
				result += nChCode * nMultipler; // упаковываем очередной символ
				nMultipler /= 050;
				// следующий символ
				ch = g_pReader->readChar();

				// если , - следующий операнд
				// если CT_LN - то конец строки, значит конец операнда
				// пробелы в операндах в данной команде допускаются
				// если ; - то начался комментарий
				// или с-подобный комментарий
				if (ch == L',' || ch == L';' || g_pReader->getCurrCharType() == CReader::CHAR_TYPE::LN /* || g_pReader->getCurrCharType() == CReader::CHAR_TYPE::SPACES*/
				        || (ch == L'/' && (g_pReader->getNextChar() == L'/' || g_pReader->getNextChar() == L'*')))
				{
					// прервёмся
					break;
				}
			}

			return true;
		}

		default:
			result = 0; // ошибка - неверный символ в строке.
			return false;
	}

	result = nTmp;

	if (!bRet)
	{
		ErrorManager::OutError(ERRNUM::E_131); // ошибка в числе
	}

	if (nTmp > 65535)
	{
		result &= 0xffff;
		ErrorManager::OutError(ERRNUM::E_126); // переполнение слова
		return false;
	}

	return bRet;
}
