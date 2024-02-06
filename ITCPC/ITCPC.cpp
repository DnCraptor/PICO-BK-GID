// ITCPC.cpp: определяет точку входа для консольного приложения.
//

#include "pch.h"
#include "ITCPC.h"
#include <locale>
#include <regex>

#include "getopt.h"
#include "bk2ibm.h"
#include "ibm2bk.h"
#include "ibm2vxt.h"

namespace ITCPC
{
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


	workMode WorkParam;
}

int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	setlocale(LC_ALL, "Russian");
	int nRetCode = 0;
	static struct option long_options[] =
	{
		{ L"help",        ARG_NONE, nullptr, L'h' },
		{ L"input",       ARG_REQ,  nullptr, L'i' },
		{ L"output",      ARG_REQ,  nullptr, L'o' },
		{ L"tab",         ARG_REQ,  nullptr, L't' },
		{ L"recursive",   ARG_NONE, nullptr, L'r' },
		{ nullptr,        ARG_NULL, nullptr, ARG_NULL }
	};
	static wchar_t optstring[] = L"hi:o:t:r";
	ITCPC::WorkParam.chInM = L'A';
	ITCPC::WorkParam.chOutM = L'U';
	ITCPC::WorkParam.nTabWidth = 0;

	if (argc < 2)
	{
		// если запускаем без аргументов, то сразу вывести подсказку и выйти.
		ITCPC::Usage();
		return nRetCode;
	}

	std::wstring strTmp;
	int option_index = 0;
	int c;

	while ((c = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1)
	{
		// Handle options
		switch (c)
		{
			case L'i':
				strTmp = (optarg) ? std::wstring(optarg) : L"A";

				if (!strTmp.empty())
				{
					std::locale loc;
					ITCPC::WorkParam.chInM = std::toupper(strTmp.at(0), loc); // оставим только первую букву
				}

				break;

			case L'o':
				strTmp = (optarg) ? std::wstring(optarg) : L"U";

				if (!strTmp.empty())
				{
					std::locale loc;
					ITCPC::WorkParam.chOutM = std::toupper(strTmp.at(0), loc); // оставим только первую букву
				}

				break;

			case L't':
				ITCPC::WorkParam.nTabWidth = (optarg) ? wcstol(optarg, nullptr, 10) : 0;
				break;

			case L'r':
				ITCPC::WorkParam.bRecursive = true;
				break;

			case L'h':
				ITCPC::Usage();
				return nRetCode;
		}
	}

	// проверяем параметры на корректность.
	if (!ITCPC::CheckInputParams())
	{
		ITCPC::Usage();
		return nRetCode;
	}

	argc -= optind;
	argv += optind;
	wprintf(L"Преобразование %s -> %s\n", ITCPC::GetFormatName(ITCPC::WorkParam.chInM).c_str(), ITCPC::GetFormatName(ITCPC::WorkParam.chOutM).c_str());

	if (ITCPC::WorkParam.chInM == ITCPC::WorkParam.chOutM)
	{
		wprintf(L"Бессмысленное преобразование делать не будем.\n");
		return nRetCode;
	}

	//  if (ITCPC::WorkParam.chInM == L'A' && (ITCPC::WorkParam.chOutM != L'E' && ITCPC::WorkParam.chOutM != L'V'))
	//  {
	//      wprintf(L"Подозрительное преобразование на всякий случай делать не будем.\n");
	//      return nRetCode;
	//  }
	// из бейсик аск в вортекс преобразование невозможно
	// т.к. в аск потоке не реализован просмотр вперёд и произвольное перемещение по потоку.
	if (ITCPC::WorkParam.chInM == L'B' && ITCPC::WorkParam.chOutM == L'V')
	{
		wprintf(L"Такое преобразование невозможно по техническим причинам.\n");
		return nRetCode;
	}

	int nFilesCount = 0; // счётчик обработанных файлов

	for (; argc > 0; argc--, argv++) // теперь, пока аргументы не кончатся - обрабатывать
	{
		// достанем из аргумента путь, если он есть
		fs::path filePath = fs::current_path(); // предполагаем базовый путь
		fs::path srcArg(*argv); // преобразуем в путь аргумент

		if (srcArg.is_absolute()) // абсолютный путь задан?
		{
			filePath = srcArg.parent_path(); // да - тогда берём его
		}
		else if (srcArg.is_relative() && srcArg.has_parent_path()) // относительный путь есть?
		{
			filePath = fs::current_path() / srcArg.parent_path(); // да - тогда присоединяем его к базовому
		}

		// делаем из шаблона файла регулярное выражение
		std::wstring mask = srcArg.filename().wstring();
		mask = ITCPC::ReplaceString(mask, L".", L"[.]");
		mask = ITCPC::ReplaceString(mask, L"*", L".*");
		mask = ITCPC::ReplaceString(mask, L"?", L".");
		std::wregex fileMask(mask, std::regex_constants::icase);
		// теперь получим все файлы из директории.
		// эта штука не умеет выбирать из директории только нужные файлы, поэтому приходится
		// фильтровать вручную с помощью регулярных выражений
		std::error_code ec;

		if (ITCPC::WorkParam.bRecursive)
		{
			fs::recursive_directory_iterator dit(filePath, fs::directory_options::skip_permission_denied | fs::directory_options::follow_directory_symlink, ec);

			if (ec)
			{
				printf("Ошибка: '%s'\n", ec.message().c_str());
			}
			else
			{
				for (auto const &it : dit)
				{
					if (fs::is_regular_file(it.path()) && std::regex_match(it.path().filename().wstring(), fileMask))
					{
						ITCPC::Process(it.path());
						nFilesCount++;
					}
				}
			}
		}
		else
		{
			fs::directory_iterator dit(filePath, fs::directory_options::skip_permission_denied | fs::directory_options::follow_directory_symlink, ec);

			if (ec)
			{
				printf("Ошибка: '%s'\n", ec.message().c_str());
			}
			else
			{
				for (auto const &it : dit)
				{
					if (fs::is_regular_file(it.path()) && std::regex_match(it.path().filename().wstring(), fileMask))
					{
						ITCPC::Process(it.path());
						nFilesCount++;
					}
				}
			}
		}
	}

	wprintf(L"Готово, обработано %d файлов.\n", nFilesCount);
	return nRetCode;
}

void ITCPC::Process(const fs::path &file)
{
	// делаем имя входного файла
	WorkParam.strInFileName = file; // полный путь к имени файла
	// делаем имя выходного файла.
	MakeOutName();
	wprintf(L"'%s' -> '%s'\n", WorkParam.strInFileName.c_str(), WorkParam.strOutFileName.filename().c_str());
	CBaseFile *pInFile = nullptr;
	CBaseFile *pOutFile = nullptr;

	switch (WorkParam.chInM)
	{
		default:
		case L'A':
			pInFile = new CWinFile(WorkParam.strInFileName);
			break;

		case L'U':
			pInFile = new CWinFile(WorkParam.strInFileName, CWinFile::FILE_CHARSET::UTF16LE);
			break;

		case L'W':
			pInFile = new CWinFile(WorkParam.strInFileName, CWinFile::FILE_CHARSET::CP1251);
			break;

		case L'O':
			pInFile = new CWinFile(WorkParam.strInFileName, CWinFile::FILE_CHARSET::CP866);
			break;

		case L'K':
			pInFile = new CWinFile(WorkParam.strInFileName, CWinFile::FILE_CHARSET::KOI8);
			break;

		case L'E':
			pInFile = new CBKFile(WorkParam.strInFileName);
			break;

		case L'V':
			pInFile = new CBKFile(WorkParam.strInFileName);
			break;

		case L'B':
			pInFile = new CBKASCFile(WorkParam.strInFileName);
			break;
	}

	if (pInFile)
	{
		if (2 <= WorkParam.nTabWidth && WorkParam.nTabWidth <= 16)
		{
			pInFile->setTabWidth(WorkParam.nTabWidth);
		}
	}

	switch (WorkParam.chOutM)
	{
		default:
		case L'U':
			pOutFile = new CWinFile(WorkParam.strOutFileName, CWinFile::FILE_CHARSET::UTF16LE);
			BK2IBM(pInFile, pOutFile);
			break;

		case L'8':
			pOutFile = new CWinFile(WorkParam.strOutFileName, CWinFile::FILE_CHARSET::UTF8);
			BK2IBM(pInFile, pOutFile);
			break;

		case L'W':
			pOutFile = new CWinFile(WorkParam.strOutFileName, CWinFile::FILE_CHARSET::CP1251);
			BK2IBM(pInFile, pOutFile);
			break;

		case L'O':
			pOutFile = new CWinFile(WorkParam.strOutFileName, CWinFile::FILE_CHARSET::CP866);
			BK2IBM(pInFile, pOutFile);
			break;

		case L'K':
			pOutFile = new CWinFile(WorkParam.strOutFileName, CWinFile::FILE_CHARSET::KOI8);
			BK2IBM(pInFile, pOutFile);
			break;

		case L'E':
			pOutFile = new CBKFile(WorkParam.strOutFileName);
			IBM2BK(pInFile, pOutFile);
			break;

		case L'V':
			pOutFile = new CBKFile(WorkParam.strOutFileName);
			IBM2VXT(pInFile, pOutFile);
			break;

		case L'B':
			pOutFile = new CBKASCFile(WorkParam.strOutFileName);
			TXT2ASC(pInFile, pOutFile);
			break;
	}

	if (pInFile)
	{
		delete pInFile;
		pInFile = nullptr;
	}

	if (pOutFile)
	{
		delete pOutFile;
		pOutFile = nullptr;
	}
}

void ITCPC::Usage()
{
	wprintf(L"Конвертор текстов Win <--> БК\n" \
	        L"Использование:\n" \
	        L"ITCPC -h (--help)\n" \
	        L"  Вывод этой справки.\n" \
	        L"ITCPC [-i<in_mode>] [-o<out_mode>] [-t<n>] [-r] <InputFile1.ext> {<InputFileN.ext> ...}\n" \
	        L"  -i<in_mode> (--input <in_mode>) - задать формат входного файла.\n" \
	        L"  Возможные режимы: a - автоопределение (по умолчанию, если параметр\n" \
	        L"      отсутствует). Не работает для БКшных файлов. Они опознаются как\n" \
	        L"      файл в KOI8R, и будут обработаны неправильно.\n" \
	        L"                    b - Basic ASC файлы. InputFile1.ext должен быть\n" \
	        L"      заголовочным .ASC файлом в .bin формате.\n" \
	        L"                    u - Unicode UTF-16LE или Unicode UTF-8, форматы\n" \
	        L"      корректно определяются даже без BOM.\n" \
	        L"                    w - Windows CP-1251\n" \
	        L"                    o - OEM CP-866\n" \
	        L"                    k - KOI8R\n" \
	        L"                    e - БКшный файл в формате Edasp\n" \
	        L"                    v - БКшный файл в формате Vortex\n\n" \
	        L"  -o<out_mode> (--output <out_mode>) - задать формат выходного файла.\n" \
	        L"  Возможные режимы: u - Unicode UTF-16LE (по умолчанию, если параметр\n" \
	        L"      отсутствует)\n" \
	        L"                    8 - Unicode UTF-8\n" \
	        L"                    w - Windows CP-1251\n" \
	        L"                    o - OEM CP-866\n" \
	        L"                    k - KOI8R\n" \
	        L"                    e - БКшный файл в формате Edasp\n" \
	        L"                    v - БКшный файл в формате Vortex\n" \
	        L"                    b - Basic ASC файлы (Имеет смысл только для листингов\n" \
	        L"      текстов на Бейсике).\n\n" \
	        L"  -t<n> (--tab <n>) - задать ширину табулятора для входного файла.\n" \
	        L"      Допускаются значения от 2 до 16, остальные игнорируются.\n" \
	        L"      Принятые значения по умолчанию: для файлов Windows tab = 4,\n" \
	        L"      для файлов БК tab = 8.\n\n" \
	        L"  -r (--recursive) - рекурсивный обход всех подкаталогов.\n\n" \
	        L"  { InputFile.ext } - список входных файлов. Допускается использование масок и\n" \
	        L"      задание путей, как абсолютных, так и относительных.\n");
}

bool ITCPC::CheckInputParams()
{
	static std::wstring strInP{ L"ABUWOKEV" };
	static std::wstring strOutP{ L"8BUWOKEV" };

	if (strInP.find(WorkParam.chInM) == std::wstring::npos)
	{
		return false;
	}

	if (strOutP.find(WorkParam.chOutM) == std::wstring::npos)
	{
		return false;
	}

	return true;
}

std::wstring ITCPC::GetFormatName(const wchar_t ch)
{
	switch (ch)
	{
		case L'A':
			return { L"Авто" };
		case L'U':
			return { L"Unicode UTF-16LE" };
		case L'W':
			return { L"Windows 1251" };
		case L'O':
			return { L"OEM CP-866" };
		case L'K':
			return { L"KOI8R" };
		case L'E':
			return { L"Edasp" };
		case L'V':
			return { L"Vortex" };
		case L'8':
			return { L"Unicode UTF-8" };
		case L'B':
			return { L"Basic ASC" };
	}

	return { L"Неизвестно" };
}

// делаем имя выходного файла
void ITCPC::MakeOutName()
{
	// сперва скопируем имя файла
	WorkParam.strOutFileName = WorkParam.strInFileName;
	// если есть расширение, найдём его
	std::wstring outExt;

	switch (WorkParam.chOutM)
	{
		case L'E':
			outExt = { L".edp" };
			break;

		case L'V':
			outExt = { L".vxt" };
			break;

		case L'B':
			outExt = { L".asc" };
			break;

		default:
			outExt = { L".txt" };
			break;
	}

	if (WorkParam.strOutFileName.has_extension()) // если расширение есть
	{
		std::wstring ext = WorkParam.strOutFileName.extension().wstring();

		if (_wcsicmp(ext.c_str(), outExt.c_str()) == 0)
		{
			// если расширения равны
			WorkParam.strOutFileName += outExt; // то добавим ещё одно.
		}
		else
		{
			// если расширения не равны
			WorkParam.strOutFileName.replace_extension(outExt); // то заменим старое на новое
		}
	}
	else
	{
		// а если расширения нету
		WorkParam.strOutFileName.replace_extension(outExt); // то просто добавим новое.
	}
}
