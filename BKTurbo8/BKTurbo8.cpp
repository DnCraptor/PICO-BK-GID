// BKTurbo8.cpp: определяет точку входа для консольного приложения.
//

#include "pch.h"
#include "BKTurbo8.h"
#include "StringUtil.h"

#include "Globals.h"
#include "ErrorManager.h"
#include "BKToken.h"
#include "Assemble.h"
#include "LabelManager.h"
#include "Parser.h"
#include "Parser2.h"

#include "Listing.h"
#include "Object.h"
#include "ScriptAsm.h"

#include <clocale>
#include <regex>
#include "getopt.h"
#pragma warning(disable:4996)

constexpr auto EXT_OBJ = 0;
constexpr auto EXT_LST = 1;
constexpr auto EXT_BIN = 2;
constexpr auto EXT_RAW = 3;
static fs::path m_strExt[] =
{
	L".obj",
	L".lst",
	L".bin",
	L".raw"
};

namespace BKTurbo8
{
	fs::path m_strListingFilename, m_strObjectFileName, m_strTableFileName, m_strResultFile;
	OP_MODE m_operation = OP_MODE::UNKN;

	int m_nStartAddress = -1;   // адрес задаваемвый из командной строки
	uint32_t m_flags = BKT_FLAG_NONE;
}

std::wstring &ReplaceString(std::wstring &subject, const std::wstring &search,
                            const std::wstring &replace)
{
	size_t pos = 0;

	while ((pos = subject.find(search, pos)) != std::wstring::npos)
	{
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}

	return subject;
}

int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
#ifdef _DEBUG
	g_RefsTable.SetDumpName(L"_refs.txt");
	g_labelGlobalDefs.SetDumpName(L"_globals.txt");
	g_labelLocalDefs.SetDumpName(L"_locals.txt");
#endif
	_wsetlocale(LC_ALL, L"Russian");
	g_Globals.InitGlobalParameters();
	g_Globals.SetBasePath(fs::current_path());
	static struct option long_options[] =
	{
		{ L"help",        ARG_NONE, nullptr, L'?' },
		{ L"verbose",     ARG_NONE, nullptr, L'v' },
		{ L"raw",         ARG_NONE, nullptr, L'r' },
		{ L"input",       ARG_REQ,  nullptr, L'i' },
		{ L"listing",     ARG_OPT,  nullptr, L'l' },
		{ L"object",      ARG_OPT,  nullptr, L'o' },
		{ L"table",       ARG_OPT,  nullptr, L't' },
		{ L"address",     ARG_REQ,  nullptr, L's' },
		{ nullptr,        ARG_NULL, nullptr, ARG_NULL }
	};
	static wchar_t optstring[] = L"?vri:l::o::t::s:";
	int option_index = 0;
	int c;

	while ((c = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1)
	{
		// Handle options
		c = tolower(c);

		switch (c)
		{
			case L'?':
				BKTurbo8::Usage();
				return 0;

			case L'v':
				g_Globals.SetVerbose(true);
				break;

			case L'i': // задаём кодировку входного файла
			{
				std::wstring strTmp = (optarg) ? std::wstring(optarg) : L"a";
				g_Globals.SetCharset(CReader::FILE_CHARSET::UNDEFINE);

				if (!strTmp.empty())
				{
					wchar_t wch = tolower(strTmp.at(0)); // оставим только первую букву

					switch (wch)
					{
						case L'k':
							g_Globals.SetCharset(CReader::FILE_CHARSET::KOI8);
							break;

						case L'o':
							g_Globals.SetCharset(CReader::FILE_CHARSET::CP866);
							break;

						case L'w':
							g_Globals.SetCharset(CReader::FILE_CHARSET::CP1251);
							break;

						case L'8':
							g_Globals.SetCharset(CReader::FILE_CHARSET::UTF8);
							break;

						case L'u':
							g_Globals.SetCharset(CReader::FILE_CHARSET::UTF16LE);
							break;
					}
				}
			}
			break;

			case L'r':
				BKTurbo8::m_flags |= BKT_FLAG_MAKERAW;
				break;

			case L'l':
				BKTurbo8::m_flags |= BKT_FLAG_MAKELISTING;

				if (optarg)
				{
					BKTurbo8::m_strListingFilename = fs::path(optarg);
				}

				break;

			case L'o':
				BKTurbo8::m_flags |= BKT_FLAG_MAKEOBJECT;

				if (optarg)
				{
					BKTurbo8::m_strObjectFileName = fs::path(optarg);
				}

				break;

			case L't':
				BKTurbo8::m_flags |= BKT_FLAG_MAKETABLE;

				if (optarg)
				{
					BKTurbo8::m_strTableFileName = fs::path(optarg);
				}

				break;

			case L's':
			{
				int nTmp = _tcstol(optarg, nullptr, 8) & 0xffff;

				if (0 <= nTmp && nTmp <= HIGH_BOUND)
				{
					BKTurbo8::m_nStartAddress = nTmp;
				}
			}
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1)
	{
		wprintf(L"Ошибка в командной строке: отсутствует команда.\n\n");
		BKTurbo8::Usage();
		return 0;
	}

	std::wstring strCommand = strUtil::strToLower(std::wstring(*argv).substr(0, 2));

	if (strCommand == L"li")
	{
		BKTurbo8::m_operation = OP_MODE::LINK;
	}
	else if (strCommand == L"co")
	{
		BKTurbo8::m_operation = OP_MODE::COMP;
	}
	else if (strCommand == L"cl")
	{
		BKTurbo8::m_operation = OP_MODE::CL;
	}
	else
	{
		wprintf(L"Ошибка в командной строке: неопознанная команда.\n\n");
		BKTurbo8::Usage();
		return 0;
	}

	argc--;
	argv++;

	if (argc < 1)
	{
		wprintf(L"Ошибка в командной строке: не хватает аргументов после команды.\n\n");
		BKTurbo8::Usage();
		return 0;
	}

	switch (BKTurbo8::m_operation)
	{
		case OP_MODE::LINK:
		{
			BKTurbo8::m_strResultFile = fs::path(*argv);
			argc--;
			argv++;

			if (argc < 1)
			{
				wprintf(L"Ошибка в командной строке: не хватает аргументов после команды LI.\n\n");
				BKTurbo8::Usage();
				return 0;
			}

			wprintf(L"Линковка...\nСоздаётся файл: %s\n", BKTurbo8::m_strResultFile.c_str());

			if (BKTurbo8::m_nStartAddress != -1) // если в командной строке был задан адрес загрузки
			{
				// то применим его
				g_Globals.SetStartAddress(BKTurbo8::m_nStartAddress);
				g_Globals.SetLinkerAddr(BKTurbo8::m_nStartAddress); // а адреса объектных модулей будем игнорировать
			}
		}
		break;

		case OP_MODE::COMP:
			if (BKTurbo8::m_nStartAddress != -1) // если в командной строке был задан адрес загрузки
			{
				// то применим его
				g_Globals.SetStartAddress(BKTurbo8::m_nStartAddress);
			}

			break;
	}

	bool bRes = true;

	for (; argc > 0; argc--, argv++) // теперь, пока аргументы не кончатся - обрабатывать
	{
		// достанем из аргумента путь, если он есть
		fs::path filePath = g_Globals.GetBasePath(); // предполагаем базовый путь
		fs::path srcArg(*argv); // преобразуем в путь аргумент

		if (srcArg.is_absolute()) // абсолютный путь задан?
		{
			filePath = srcArg.parent_path(); // да - тогда берём его
		}
		else if (srcArg.is_relative() && srcArg.has_parent_path()) // относительный путь есть?
		{
			filePath /= srcArg.parent_path(); // да - тогда присоединяем его к базовому
		}

		std::wstring mask = srcArg.filename().wstring();
		mask = ReplaceString(mask, L".", L"[.]");
		mask = ReplaceString(mask, L"*", L".*");
		mask = ReplaceString(mask, L"?", L".");
		std::wregex fileMask(mask, std::regex_constants::icase);
		std::error_code ec;
		fs::directory_iterator dit(filePath, fs::directory_options::skip_permission_denied | fs::directory_options::follow_directory_symlink, ec);

		for (auto const &it : dit)
		{
			fs::path file = it.path(); // имя найденного файла

			// обрабатываем только файлы, директории игнорируем
			if (fs::is_regular_file(file) && std::regex_match(file.filename().wstring(), fileMask))
			{
				if (BKTurbo8::m_operation == OP_MODE::LINK)
				{
					wprintf(L"Линкуется объектный файл: %s ", file.filename().c_str());
					bRes = ObjManger::LoadObj(file);

					if (bRes)
					{
						wprintf(L"- OK\n");
						LabelManager::LabelLinking();
						g_Globals.GrowLinkObjLength(); // это надо делать только после линковки.
					}
					else
					{
						wprintf(L"- Ошибка\n");
						break;
					}
				}
				else if (BKTurbo8::m_operation == OP_MODE::COMP || BKTurbo8::m_operation == OP_MODE::CL)
				{
					BKTurbo8::workCycle(file);
					g_Globals.ReInitGlobalParameters();
					// чистим все векторы.
					g_RefsTable.Clear();
					g_labelGlobalDefs.Clear();
					g_labelLocalDefs.Clear();
					g_Listing.clear();
					g_ScriptDefs.clear();

					if (BKTurbo8::m_nStartAddress != -1) // если в командной строке был задан адрес загрузки
					{
						// то применим его
						g_Globals.SetStartAddress(BKTurbo8::m_nStartAddress);
					}
					else
					{
						g_Globals.SetStartAddress(DEFAULT_START_ADDRESS);
					}
				}
			}
		}

		if (!bRes)
		{
			break;
		}
	}

	if (BKTurbo8::m_operation == OP_MODE::LINK && bRes)
	{
		wprintf(L"Адрес компоновки = %#6o\n", g_Globals.GetStartAddress());
		wprintf(L"Размер исполняемого файла = %#6o\n", g_Globals.GetProgramLength());
		// здесь применяем скрипты
		g_ScriptAsm.RunScript();
		BKTurbo8::SaveFile(BKTurbo8::m_strResultFile);

		if (BKTurbo8::m_flags & BKT_FLAG_MAKETABLE)
		{
			fs::path name = (BKTurbo8::m_strTableFileName.empty() ? BKTurbo8::m_strResultFile : BKTurbo8::m_strTableFileName).stem();
			name += fs::path(L"_tbl");
			ObjManger::MakeObj(name, m_strExt[EXT_OBJ], true);
		}

		if (BKTurbo8::m_flags & BKT_FLAG_MAKEOBJECT)
		{
			ObjManger::MakeObj(BKTurbo8::m_strObjectFileName.empty() ? BKTurbo8::m_strResultFile : BKTurbo8::m_strObjectFileName, m_strExt[EXT_OBJ]);
		}

		if (BKTurbo8::m_flags & BKT_FLAG_MAKELISTING)
		{
			ListingManager::MakeListing(BKTurbo8::m_strListingFilename.empty() ? BKTurbo8::m_strResultFile : BKTurbo8::m_strListingFilename, m_strExt[EXT_LST]);
		}

		BKTurbo8::PrintLabelTable(BKTurbo8::m_strListingFilename.empty() ? BKTurbo8::m_strResultFile : BKTurbo8::m_strListingFilename, m_strExt[EXT_LST]);
	}

	return 0;
}

int BKTurbo8::workCycle(fs::path &strInFileName)
{
	g_Globals.PushReader(nullptr); // условие выхода из цикла
	g_pReader = new CReader(strInFileName, g_Globals.GetCharset());
#if (DEBUG_LABEL_MANAGER)
	DebugInit();
#endif

	if (!g_pReader)
	{
		wprintf(L"Недостаточно памяти.\n");
		return -1;
	}

	switch (m_operation)
	{
		case OP_MODE::COMP:
			wprintf(L"Компиляция исполняемого файла...\n");
			// режим компиляции CO
			g_Globals.SetLinkMode(LINKING_MODE::CO);
			break;

		case OP_MODE::CL:
			wprintf(L"Компиляция объектного файла...\n");
			// режим компиляции CL
			g_Globals.SetLinkMode(LINKING_MODE::CL);
			break;
	}

	wprintf(L"Файл: %s\nАвтоопределение кодировки: ", strInFileName.c_str());

	switch (g_pReader->GetFileCharset())
	{
		case CReader::FILE_CHARSET::KOI8:
			wprintf(L"КОИ8-Р\n");
			break;

		case CReader::FILE_CHARSET::CP866:
			wprintf(L"OEM CP-866\n");
			break;

		case CReader::FILE_CHARSET::CP1251:
			wprintf(L"ANSI CP-1251\n");
			break;

		case CReader::FILE_CHARSET::UTF8:
			wprintf(L"UTF-8\n");
			break;

		case CReader::FILE_CHARSET::UTF16LE:
			wprintf(L"UTF-16LE(Unicode)\n");
			break;

		case CReader::FILE_CHARSET::UNDEFINE:
			wprintf(L"Не определена. Возможно, это не текстовый файл.\n");
			return -2;

		default:
			wprintf(L"\rОшибка открытия файла.     \n");
			return -3;
	}

	do
	{
		// первый проход
		while (!g_Globals.isENDReached() && !g_pReader->isEOF())
		{
			ParseLine();

			if (g_Globals.GetPC() >= HIGH_BOUND)
			{
				wprintf(L"Критическая ошибка:: Достигнут предел свободной памяти!\n");
				break;
			}
		}

		if (!g_Globals.isENDReached())
		{
			if (g_Globals.isInInclude())
			{
				g_Globals.SetInInclude(false);
			}
			else
			{
				ErrorManager::OutError(ERRNUM::E_118, false); // Отсутствует .END
			}
		}

		delete g_pReader; // удаляем текущий экземпляр
		g_pReader = g_Globals.PopReader(); // достаём предыдущий, если есть
	}
	while (g_pReader);   // и продолжаем, пока есть что обрабатывать

	int nErr = ErrorManager::IsError();

	if (nErr)
	{
		// если были ошибки - одно действие
		wprintf(L"Количество ошибок: %d\n", nErr);
	}
	else
	{
		// если не было ошибок - другое
		if (m_operation == OP_MODE::CL)
		{
			ObjManger::MakeObj(m_strObjectFileName.empty() ? strInFileName : m_strObjectFileName, m_strExt[EXT_OBJ]);
		}
		else
		{
			wprintf(L"Адрес компоновки = %#6o\n", g_Globals.GetStartAddress());
			// здесь применяем скрипты
			g_ScriptAsm.RunScript();

			if (m_flags & BKT_FLAG_MAKEOBJECT)
			{
				ObjManger::MakeObj(m_strObjectFileName.empty() ? strInFileName : m_strObjectFileName, m_strExt[EXT_OBJ]);
			}

			SaveFile(strInFileName);
		}

		wprintf(L"Размер исполняемого файла = %#6o\n", g_Globals.GetProgramLength());

		if (m_flags & BKT_FLAG_MAKETABLE)
		{
			fs::path name = (m_strTableFileName.empty() ? strInFileName : m_strTableFileName).stem();
			name += L"_tbl";
			ObjManger::MakeObj(name, m_strExt[EXT_OBJ], true);
		}
	}

	// листинг делаем в любом случае

	if (m_flags & BKT_FLAG_MAKELISTING)
	{
		ListingManager::MakeListing(m_strListingFilename.empty() ? strInFileName : m_strListingFilename, m_strExt[EXT_LST]);
	}

	PrintLabelTable(m_strListingFilename.empty() ? strInFileName : m_strListingFilename, m_strExt[EXT_LST]);
	return 0;
}


/*
Пропускаем остаток строки.
Выход: true - встретился конец файла
false - просто конец строки.
*/
bool BKTurbo8::SkipTailString(wchar_t &ch)
{
	do
	{
		ch = g_pReader->readChar();
	}
	while (!g_pReader->isEOF() && g_pReader->getCurrCharType() != CReader::CHAR_TYPE::LN);

	if (g_pReader->isEOF())
	{
		return true;
	}

	return false;
}

/*
*Парсим строку, в строке может быть неограниченное число ассемблерных команд.
*Перед командой может быть неограниченное число меток.
*Конец файла тоже считается концом строки, затем надо делать конкретную проверку на именно конец файла.
g_pReader->m_nLineNum
*/
void BKTurbo8::ParseLine()
{
	CBKToken token;
	int cp = g_Globals.GetPC();    // текущий временный PC, который приращивается во время построения инструкции или псевдокоманды
	ListingManager::PrepareLine(cp);
	wchar_t ch = g_pReader->readChar(); // читаем первый символ

	for (;;) // крутимся в цикле, пока не достигнем конца строки
	{
		// делаем так, чтобы можно было обрабатывать несколько инструкций в одной строке
		g_Globals.SetInString(false);
		g_Globals.SetAriphmType(0);
		g_Globals.SetPC(cp); // обычно сюда попадаем перед началом очередной команды, поэтому сохраняем счётчик.

		if (g_pReader->SkipWhitespaces(ch))  // пропустим возможные пробелы
		{
			// если после чтения получился конец строки,
			break;  // то выход из цикла.
		}

		// обрабатываем многострочный комментарий /*...*/
		// нормально парсится инлайн коммент, за которым может быть инструкция.
		// комментарий не может быть внутри инструкции, т.к. такой парсинг не реализован
		if (ch == L'/' && g_pReader->getNextChar() == L'*')
		{
			bool bex = false;
			ListingManager::AddPrepareLine(0, ListType::LT_COMMENTARY);

			do
			{
				ch = g_pReader->readChar();

				if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::LN)
				{
					ListingManager::PrepareLine(cp);
					ListingManager::AddPrepareLine(0, ListType::LT_COMMENTARY);
				}

				if (g_pReader->isEOF())
				{
					bex = true;
					break;
				}
			}
			while (!(ch == L'*' && g_pReader->getNextChar() == L'/'));

			if (bex)
			{
				break;
			}

			ch = g_pReader->readChar();     // читаем *
			ch = g_pReader->readChar(true); // читаем /
			continue;
		}

		// теперь обрабатываем строку
		if (ch == L';' || (ch == L'/' && g_pReader->getNextChar() == L'/')) // если комментарий ";" или "// "
		{
			SkipTailString(ch);
			ListingManager::AddPrepareLine(0, ListType::LT_COMMENTARY);
			break; // выходим из цикла, независимо от результата
		}

		if (ch == L'.') // если псевдокоманда или точка
		{
			// обработка псевдокоманды
			ch = g_pReader->readChar(); // читаем следующий за точкой символ.

			if (g_Globals.isInScript())
			{
				goto l_Script;
			}
			else // тут парсим обычный ассемблер
			{
				// присваивание значения точке
				if (Parser::needChar(L'=', ch))
				{
					ListingManager::AddPrepareLine(cp, ListType::LT_ASSIGN);
					ch = g_pReader->readChar(); // читаем следующий символ.
					int result = 0;

					if (g_RPNParser.FullAriphmParser(ch)) // если арифметическое выражение распарсилось
					{
						if (g_RPNParser.CalcRpn(result))
						{
							cp = g_Globals.CorrectOffset(result);
							g_Globals.SetPC(cp); // !!! Вот это вот не работает в режиме CL
						}
						else
						{
							// встретились неопределённые метки.
							// неопределённые метки запрещены.
							ErrorManager::OutError(ERRNUM::E_119);
							SkipTailString(ch);
							break;
						}

						ListingManager::AddPrepareLine2(result);
					}
					else
					{
						SkipTailString(ch);
						break;
					}
				}
				else
				{
l_Script:

					if (!g_pReader->readToken(&token, ch)) // и читаем лексему
					{
						// если токен не прочёлся, значит сразу за точкой встретился посторонний символ.
						ErrorManager::OutError(ERRNUM::E_103);
						SkipTailString(ch);
						break;
					}

					if (!Assemble::PseudoCommandExec(&token, cp, ch))
					{
						// если встретилась ошибка, пропустим всю строку
						SkipTailString(ch);
					}
					else
					{
						if (g_Globals.isStepInclude())
						{
							g_Globals.SetStepInclude(false);
							break;
						}
					}

					ListingManager::AddPrepareLine2(cp);
				}
			}
		}
		else // дальше что-то, либо метка, либо присваивание, либо команда.
		{
			if (ch == L'%') // если присваивание синонима регистру.
			{
				if (g_pReader->readTokenR(&token, ch)) // Читаем имя регистра
				{
					// тут надо реализовать механизм присваивания
					if (Parser::needChar(L'=', ch)) // если за лексемой, возможно через пробелы идёт '=',
					{
						ListingManager::AddPrepareLine(cp, ListType::LT_ASSIGNREG);
						// то это присваивание
						ch = g_pReader->readChar(true); // пропустим =

						// синоним должен начинаться с буквы
						if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::LETTERS)
						{
							CBKToken synonim;

							if (g_pReader->readToken(&synonim, ch))
							{
								if (Parser::AddRegSynonim(&token, &synonim))
								{
									int num = token.getName().at(1) - L'0'; // номер регистра
									ListingManager::AddPrepareLine2(num);
									continue; // продолжим обработку.
								}

								ErrorManager::OutError(ERRNUM::E_111); // неверное имя регистра
								SkipTailString(ch);
								break;
							}
						}

						// неверное имя синонима
					}

					// синтаксическая ошибка
				}

				// если токен не прочёлся, значит сразу встретился посторонний символ.
				ErrorManager::OutError(ERRNUM::E_103); // Недопустимый символ в строке.
				SkipTailString(ch);
				break;
			}

			if (!g_pReader->readToken(&token, ch)) // считаем это лексемой
			{
				// если токен не прочёлся, значит сразу встретился посторонний символ.
				ErrorManager::OutError(ERRNUM::E_103); // Недопустимый символ в строке.
				SkipTailString(ch);
				break;
			}

			if (g_Globals.isInScript())
			{
				ListingManager::AddPrepareLine(cp, ListType::LT_COMMENTARY);
				// тут парсим скрипт
				std::wstring scriptLbl, scriptCmd;

				// если сразу за лексемой идёт двоеточие
				if (ch == L':') // значит это метка.
				{
					ch = g_pReader->readChar();         // пропускаем ':' и пустоту за ним
					// добавим имя метки в строку скрипта
					scriptLbl = token.getName();
					g_pReader->SkipWhitespaces(ch);
				}
				else
				{
					scriptCmd = token.getName();
				}

				// теперь надо прочитать всё либо до конца строки, либо до комментария.
				// потому как допускаем пустые строки и просто метку в строке без команды
				for (;;)
				{
					if (g_pReader->getCurrCharType() == CReader::CHAR_TYPE::LN)
					{
						break;
					}

					if (Assemble::CheckComment(ch))  // если комментарий - то выходим, его тут не обработать
					{
						break;
					}

					scriptCmd.push_back(ch);
					ch = g_pReader->readChar();
				}

				g_ScriptAsm.AddScriptLine(ScriptLine_t{ CBKToken(scriptLbl), CBKToken(scriptCmd) });
				ListingManager::AddPrepareLine2(cp);
			}
			else // тут парсим обычный ассемблер
			{
				// если сразу за лексемой идёт двоеточие
				if (ch == L':') // значит это метка.
				{
					ch = g_pReader->readChar(true);         // пропускаем ':' и пустоту за ним
					wchar_t wch = token.getName().at(0);    // определяем тип метки.
					// если метка начинается с буквы - это глобальная метка
					// если с цифры - то это локальная метка.
					// остальные символы - любые допустимые.
					bool bl = false;

					if (g_pReader->AnalyseChar(wch) == CReader::CHAR_TYPE::DIGITS)
					{
						// локальная метка
						bl = LabelManager::AddLocalLabel(&token, g_Globals.GetPC(), LBL_LOCAL);
					}
					else
					{
						// глобальная метка
						bl = LabelManager::AddGlobalLabel(&token, g_Globals.GetPC(), LBL_GLOBAL);
					}

					ListingManager::AddPrepareLine(cp, ListType::LT_LABEL);

					if (!bl) // если метка не добавилась в таблицу
					{
						ErrorManager::OutError(ERRNUM::E_113); // то ошибка - повторное определение метки
					}
				}
				else if (Parser::needChar(L'=', ch)) // если за лексемой, возможно через пробелы идёт '=',
				{
					// то это присваивание
					ch = g_pReader->readChar(true); // пропустим =
					int result = 0;
					ListingManager::AddPrepareLine(cp, ListType::LT_ASSIGN);
					g_Globals.SetAriphmType(ARIPHM_NOLOCLABEL);

					if (g_RPNParser.FullAriphmParser(ch)) // если арифметическое выражение распарсилось
					{
						if (g_RPNParser.GetNamesNum()) // если там были имена меток или других определений
						{
							// добавляем метку без значения, обозначим, что оно ещё не вычислилось
							// без удаления локальных меток
							if (!g_labelGlobalDefs.AddLabel(&token, 0, LBL_WEAKDEFINE))
							{
								ErrorManager::OutError(ERRNUM::E_113); // если не добавилось - ошибка. уже есть такая метка
							}

							// а в таблицу ссылок добавим невычисленную цепочку выражения
							LabelManager::AddLabelReference(&token, 0, ARL_DEFINE);
						}
						else if (g_RPNParser.CalcRpn(result)) // были только числа - вычисляем.
						{
							// добавляем метку со значением, без удаления локальных меток
							if (!g_labelGlobalDefs.AddLabel(&token, result, LBL_DEFINE))
							{
								ErrorManager::OutError(ERRNUM::E_113); // если не добавилось - ошибка. уже есть такая метка
							}
						}
						else
						{
							assert(false); // такого быть не должно, надо разбираться.
						}

						int n = g_labelGlobalDefs.SearchLabel(&token);
						// для определения передаём номер элемента в таблице,
						// потом его оттуда прочитаем, если он добавился, если нет - будет 0
						ListingManager::AddPrepareLine2(n);
					}
					else
					{
						SkipTailString(ch);
						break;
					}
				}
				else
				{
					g_pReader->SkipWhitespaces(ch); // пропускаем пробелы - на всякий случай

					// обработка ассемблерной команды.
					if (!Assemble::AssembleCPUInstruction(&token, cp, ch))
					{
						// assert(false);
						// если встретилась ошибка, пропустим всю строку
						SkipTailString(ch);
					}

					ListingManager::AddPrepareLine2(cp);
				}
			}
		}
	}
}

void BKTurbo8::SaveFile(const fs::path &strName)
{
	fs::path fext = strName.extension();

	// если получается очень большой файл
	if (g_Globals.GetProgramLength() >= HIGH_BOUND)
	{
		m_flags |= BKT_FLAG_MAKERAW; // то бин делать никак нельзя
	}

	fs::path strTmp = strName.parent_path() / strName.stem();
	// если у файла стандартное расширение, то его убираем, остальные - оставляем
	std::wstring strExt = strUtil::trim(strUtil::strToLower(fext.wstring()));

	if (strExt != L".asm" && strExt != L".obj")
	{
		strTmp += fext;
	}

	// сделаем новое имя файла - добавим расширение бин или рав
	fs::path outName = strTmp;
	outName += m_strExt[m_flags & BKT_FLAG_MAKERAW ? EXT_RAW : EXT_BIN];
	FILE *of = _wfopen(outName.c_str(), L"wb");

	if (of)
	{
		if (!(m_flags & BKT_FLAG_MAKERAW))
		{
			uint16_t bin[2]
			{
				static_cast<uint16_t>(g_Globals.GetStartAddress()),
				static_cast<uint16_t>(g_Globals.GetProgramLength())
			};
			fwrite(bin, 1, sizeof(uint16_t) * 2, of);
		}

		fwrite(&g_Memory.b[BASE_ADDRESS], 1, g_Globals.GetProgramLength(), of);
		fclose(of);
	}
	else
	{
		wprintf(L"Ошибка создания файла %s\n", outName.c_str());
	}
}

// вывод на экран таблицы меток.
void BKTurbo8::PrintLabelTable(const fs::path &strName, const fs::path &strExt)
{
	// сделаем новое имя файла
	fs::path outName = strName;
	outName.replace_extension(strExt);
	FILE *of = nullptr;

	if (m_flags & BKT_FLAG_MAKELISTING)
	{
		of = _wfopen(outName.c_str(), L"a+t");
	}

	// выведем таблицу глобальных меток.
	std::wstring wstr1 = L"\nГлобальные метки\n\n";

	if (g_Globals.isVerbose())
	{
		wprintf(wstr1.c_str());
	}

	if (of)
	{
		ListingManager::OutString(of, &wstr1);
	}

	size_t sz = g_labelGlobalDefs.getSize();
	size_t maxlen = 0;

	// сперва найдём самую длинную метку
	for (size_t i = 0; i < sz; ++i)
	{
		std::wstring name = g_labelGlobalDefs.GetLabel(i)->getName();

		if (name.length() > maxlen)
		{
			maxlen = name.length();
		}
	}

	// теперь сформируем строку формата
	static wchar_t buf[32] = { 0 };

	if (maxlen > 16)
	{
		maxlen = 16;
	}

	swprintf_s(buf, 32, L"%%-%ds = %%07o\0", int(maxlen));
	int n = 84 / (int(maxlen) + 12); // вот столько влазит в экран
	int j = n;

	for (size_t i = 0; i < sz; ++i)
	{
		std::wstring name = g_labelGlobalDefs.GetLabel(i)->getName();
		int value = g_labelGlobalDefs.GetValue(i);

		if ((g_labelGlobalDefs.GetType(i) & LBL_DEFINITE_MASK) == LBL_GLOBAL)
		{
			value = g_Globals.GetRealAddress(value);
		}

		if ((g_labelGlobalDefs.GetType(i) & LBL_DEFINITE_MASK) == LBL_WEAKDEFINE)
		{
			name = L"*" + name;
		}

		if (name.length() > maxlen)
		{
			// если метка очень длинная, то её только одну в строке выведем.
			if (g_Globals.isVerbose())
			{
				wprintf(L"%-s = %07o", name.c_str(), (value & 0xffff));
			}

			if (of)
			{
				fwprintf(of, L"%-s = %07o", name.c_str(), (value & 0xffff));
			}

			j = n;

			if (g_Globals.isVerbose())
			{
				wprintf(L"\n");
			}

			if (of)
			{
				fwprintf(of, L"\n");
			}
		}
		else
		{
			if (g_Globals.isVerbose())
			{
				wprintf(buf, name.c_str(), (value & 0xffff));
			}

			if (of)
			{
				fwprintf(of, buf, name.c_str(), (value & 0xffff));
			}

			if (--j <= 0)
			{
				j = n;

				if (g_Globals.isVerbose())
				{
					wprintf(L"\n");
				}

				if (of)
				{
					fwprintf(of, L"\n");
				}
			}
			else
			{
				if (g_Globals.isVerbose())
				{
					wprintf(L"  ");
				}

				if (of)
				{
					fwprintf(of, L"  ");
				}
			}
		}
	}

	// а теперь выведем оставшиеся ссылки на не найденные метки
	sz = g_RefsTable.getSize();

	if (sz)
	{
		wstr1 = L"\n\nСсылки на неопределённые метки!\n\n";
		wprintf(wstr1.c_str());

		if (of)
		{
			ListingManager::OutString(of, &wstr1);
		}

		// сперва найдём самую длинную метку
		for (size_t i = 0; i < sz; ++i)
		{
			CRPNParser::RPNChain chain = g_RefsTable.GetElement(i).getRPN();

			for (auto &cn : chain)
			{
				if ((cn.type == CRPNParser::A_TYPE::LABEL) || ((cn.type == CRPNParser::A_TYPE::LOC_LABEL) && (int(cn.number) < 0)))
				{
					const std::wstring &name = cn.token.getName();

					if (name.length() > maxlen)
					{
						maxlen = name.length();
					}
				}
			}
		}

		// теперь сформируем строку формата
		if (maxlen > 16)
		{
			maxlen = 16;
		}

		swprintf_s(buf, 32, L"%%07o : %%-%ds\0", int(maxlen));
		n = 84 / (int(maxlen) + 12); // вот столько влазит в экран
		j = n;

		for (size_t i = 0; i < sz; ++i)
		{
			CRPNParser::RPNChain chain = g_RefsTable.GetElement(i).getRPN();

			for (auto &cn : chain)
			{
				if (cn.type == CRPNParser::A_TYPE::LABEL || ((cn.type == CRPNParser::A_TYPE::LOC_LABEL) && (int(cn.number) < 0)))
				{
					const std::wstring &name = cn.token.getName();
					int value = g_RefsTable.GetElement(i).getAddress();
					value = g_Globals.GetRealAddress(value);

					if (name.length() > maxlen)
					{
						// если метка очень длинная, то её только одну в строке выведем.
						wprintf(L"%07o : %-s", value, name.c_str());

						if (of)
						{
							fwprintf(of, L"%07o : %-s", value, name.c_str());
						}

						j = n;
						wprintf(L"\n");

						if (of)
						{
							fwprintf(of, L"\n");
						}
					}
					else
					{
						wprintf(buf, value, name.c_str());

						if (of)
						{
							fwprintf(of, buf, value, name.c_str());
						}

						if (--j <= 0)
						{
							j = n;
							wprintf(L"\n");

							if (of)
							{
								fwprintf(of, L"\n");
							}
						}
						else
						{
							wprintf(L"  ");

							if (of)
							{
								fwprintf(of, L"  ");
							}
						}
					}
				}
			}
		}
	}

	wprintf(L"\n");

	if (of)
	{
		fwprintf(of, L"\n"); fclose(of);
	}
}


void BKTurbo8::Usage()
{
	wprintf(L"Кросс ассемблер Turbo8 для БК0010-БК0011М\n" \
	        L"(с) 2014-2023 gid\n\n" \
	        L"Использование:\n" \
	        L"BKTurbo8 -? (--help)\n" \
	        L"  Вывод этой справки.\n\n" \
	        L"Имеется два режима работы. Режим компиляции, с созданием объектных модулей\n" \
	        L"и исполняемого файла. И режим линковки объектных модулей.\n\n" \
	        L"1. Режим компиляции.\n" \
	        L"BKTurbo8 [-i<c>][-v][-r][-l[name]][-o[name]][-s<0addr>] <cmd> <file_1 *[ file_n]>\n" \
	        L"  -i<c> (--input <c>) - задать кодировку исходного файла.\n" \
	        L"      Возможные кодировки:\n" \
	        L"        a - автоопределение (по умолчанию)\n" \
	        L"        k - KOI8-R\n" \
	        L"        o - OEM CP866\n" \
	        L"        w - ANSI CP1251\n" \
	        L"        8 - UTF8\n" \
	        L"        u - UNICODE UTF16LE\n" \
	        L"  Если автоопределение определило кодировку некорректно, необходимо задать\n" \
	        L"верную кодировку данным ключом.\n" \
	        L"  -v (--verbose) - вывод большего количества информации на экран.\n" \
	        L"      На данный момент дополнительно выводится таблица меток программы.\n\n" \
	        L"  -r (--raw) - создавать просто бинарный массив, не использовать формат .bin.\n\n" \
	        L"  -l[name] (--listing[=name]) - генерировать lst Файл.\n" \
	        L"      Если имя файла задано, то используется оно для генерации листинга, если\n" \
	        L"      нет - то берётся имя файла исходного текста.\n\n" \
	        L"  -o[name] (--object[=name]) - генерировать объектный файл.\n" \
	        L"      Если имя файла задано, то используется оно для генерации листинга, если\n" \
	        L"      нет - то берётся имя файла исходного текста.\n\n" \
	        L"  -t[name] (--table[=name]) - создавать особый объектный файл, в котором\n" \
	        L"      содержатся только глобальные метки. (См. документацию)\n\n" \
	        L"  -s<0addr> (--address <0addr>) - задать начальный адрес компиляции.\n" \
	        L"      Адрес задаётся в восьмеричном виде.\n\n" \
	        L"  <cmd> - команда компиляции:\n" \
	        L"      CO - полная компиляция. В результате при отсутствии ошибок создаётся\n" \
	        L"           бинарный исполняемый файл и опционально создаются объектные файлы,\n" \
	        L"           заданные соответствующими ключами.\n" \
	        L"      CL - компиляция в объектный файл для дальнейшей линковки с другими\n" \
	        L"           объектными файлами. В результате при отсутствии ошибок всегда\n" \
	        L"           создаётся объектный файл. Бинарный файл не создаётся.\n" \
	        L"      Файл листинга создаётся в любом случае. При наличии ошибок код ошибки\n" \
	        L"      и его текстовое пояснение помещаются перед строкой листинга, вызвавшей\n" \
	        L"      ошибку.\n" \
	        L"      В конец файла листинга записывается таблица глобальных меток, а также\n" \
	        L"      список ссылок на неопределённые метки, если они есть.\n\n" \
	        L"  <file_1 *[ file_n]> - список исходных файлов, перечисленных через пробел.\n" \
	        L"      Допускаются маски файлов.\n\n" \
	        L"2. Режим линковки.\n" \
	        L"BKTurbo8 [-v][-r][-l[name]][-o[name]][-s<0addr>] LI <outfile> <file_1 *[ file_n]>\n" \
	        L"  Ключ -i не используется.\n\n" \
	        L"  Ключи -v, -r, -l, -o, -t и -s имеют тот же смысл, что и в режиме компиляции.\n\n" \
	        L"  Команда линковки - LI, за командой следует обязательное имя выходного файла\n" \
	        L"  <outfile>, маска файла не допускается. А затем список файлов объектных\n" \
	        L"  модулей.\n" \
	        L"  Листинг при этом не создаётся, потому что не из чего, но если задан ключ -l,\n" \
	        L"  в файл листинга сохраняется список меток, а так же список ссылок на \n" \
	        L"  неопределённые метки, если они есть.\n\n" \
	        L"  <file_1 *[ file_n]> - список файлов объектных модулей, перечисленных через\n" \
	        L"      пробел. Допускаются маски файлов.\n\n");
}

