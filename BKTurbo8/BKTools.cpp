// BKTools.cpp: определяет точку входа для консольного приложения.
//

#include "pch.h"
#include <cwctype>
#include <clocale>
#include "BKTools.h"
#include "BKPack.h"
#include "BKCrun.h"

#include "getopt.h"

#pragma warning(disable:4996)

// глобальные переменные
fs::path g_strInFileName;   // имя входного файла
fs::path g_strOutFileName;  // имя выходного файла
bool g_bRaw = false;        // флаг, создавать выходной файл в RAW форме
bool g_bUnpack = false;     // флаг, задействовать распаковщик
int g_nWorkArea = -1;       // адрес конца рабочей области по умолчанию. (-1 - не задано, выбирается автоматически)

FILE *g_File = nullptr;     // файл
int g_nFileLen = 0, g_nFileAddress = 01000; // параметры файла по умолчанию
int g_nFileLoadAddress = 0; // адрес загрузки файла в массив

BKTools g_tool = UNDEFINE;  // инструмент по умолчанию

MemoryModel MMemory;


void Usage()
{
	wprintf(L"Кросс мультитул BKTools для БК-0010(01) БК-0011(М)\n" \
	        L"(с) 2017-2023 gid\n\n" \
	        L"Использование:\n" \
	        L"BKTools -? (--help)\n" \
	        L"  Вывод этой справки.\n\n" \
	        L"BKTools -t<name> [-d][-l][-r][-u][-w<0addr>] <input_file_name> [output_file_name]\n" \
	        L"  -t<name> (--tool name) - выбор необходимого инструмента.\n" \
	        L"      name: BKPACK - упаковщик/распаковщик BKpack.\n" \
	        L"            BKCRUNCH - упаковщик/распаковщик Cruncher.\n\n" \
	        L"  -d (--data) - флаг упаковки данных, а не программы.\n" \
	        L"      По умолчанию упаковка программы.\n\n" \
	        L"  -l (--long) - использовать длинный автораспаковщик.\n" \
	        L"      По умолчанию - обычный короткий. Не знаю, пользовался ли кто либо\n" \
	        L"      когда-нибудь таким, но функционал сохранён.\n\n" \
	        L"  -r (--raw) - создавать просто файл, не использовать формат '.bin'.\n\n" \
	        L"  -u (--unpack) - распаковка запакованного файла.\n" \
	        L"      Если этот ключ не указан, производится упаковка в заданный архив.\n\n" \
	        L"  -w<0addr> (--work 0addr) - задать адрес рабочей области. Только для упаковки\n" \
	        L"      BKPack. Для простоты, задаётся адрес конца рабочей области, например 040000,\n" \
	        L"      если нужно, чтобы рабочая область располагалась перед областью экрана.\n\n" \
	        L"  input_file_name - входной файл.\n" \
	        L"  output_file_name - необязательное имя выходного файла, если нужно задать\n" \
	        L"      своё имя файла создаваемому новому файлу.\n" \
	        L"  По умолчанию, у выходного имени файла заменяется расширение на '.bkp'\n" \
	        L"  (или на '.bkfile' при распаковке) или добавляется, если его не было.\n");
}

// открываем и анализируем входной файл
// если всё хорошо, загружаем его в MMemory с индекса 0.
bool OpenInFile()
{
	bool bRet = false;
	uint16_t mBinHeader[2];

	if (fs::exists(g_strInFileName))
	{
		size_t size = fs::file_size(g_strInFileName);

		if (size < 0160000)
		{
			if ((g_File = _wfopen(g_strInFileName.c_str(), L"rb")) == nullptr)
			{
				wprintf(L"Ошибка:: не удалось открыть файл '%s'.\n", g_strInFileName.c_str());
			}
			else
			{
				// файл открыт, теперь надо узнать, .bin формат или нет,
				// и какой адрес запуска у файла
				int nOffset = 0;
				g_nFileLen = size;

				if (sizeof(mBinHeader) == fread(mBinHeader, 1, sizeof(mBinHeader), g_File))
				{
					if (mBinHeader[1] == g_nFileLen - 4)
					{
						nOffset = 4;
					}
					else if (mBinHeader[1] == g_nFileLen - 22)
					{
						nOffset = 20;
					}

					if (nOffset) // если действительно бин
					{
						g_nFileAddress = mBinHeader[0];  // берём данные из заголовка
						g_nFileLen = mBinHeader[1];
					}

					if (0 != fseek(g_File, nOffset, SEEK_SET))
					{
						wprintf(L"Ошибка:: не удалось прочитать файл '%s'.\n", g_strInFileName.c_str());
					}
					else
					{
						// читаем данные в массив
						// g_nFileLoadAddress = g_nFileAddress; // загружаем по адресу загрузки
						if (g_nFileLen == fread(&MMemory.b[g_nFileLoadAddress], 1, g_nFileLen, g_File))
						{
							fclose(g_File);

							if (g_bUnpack)
							{
								wprintf(L"Распаковываем '%s' :: ", g_strInFileName.c_str());
							}
							else
							{
								wprintf(L"Пакуем '%s' :: ", g_strInFileName.c_str());
							}

							bRet = true;
						}
						else
						{
							wprintf(L"Ошибка:: не удалось прочитать файл '%s'.\n", g_strInFileName.c_str());
						}
					}
				}
				else
				{
					wprintf(L"Ошибка:: не удалось прочитать файл '%s'.\n", g_strInFileName.c_str());
				}
			}
		}
		else
		{
			wprintf(L"Ошибка:: файл слишком большой.\n");
		}
	}
	else
	{
		wprintf(L"Ошибка:: файл не найден.\n");
	}

	return bRet;
}

// сохраняем результат работы BKPacka из
// массива MMemory
bool SaveOutFileBKPack()
{
	bool bRet = false;
	uint16_t mBinHeader[2] {};

	if ((g_File = _wfopen(g_strOutFileName.c_str(), L"wb")) == nullptr)
	{
		wprintf(L"Ошибка:: не удалось открыть файл для записи '%s'.\n", g_strOutFileName.c_str());
	}
	else
	{
		// добавим заголовок .bin
		mBinHeader[0] = g_nFileAddress;
		mBinHeader[1] = g_bUnpack ? BKPkParam.nFileLen : BKPkParam.EPK;

		if (!g_bRaw)
		{
			fwrite(mBinHeader, 1, sizeof(mBinHeader), g_File);
		}

		fwrite(&MMemory.b[g_nFileLoadAddress], 1, mBinHeader[1], g_File);
		fclose(g_File);
		bRet = true;
	}

	return bRet;
}
// сохраняем результат работы BKCrunch из
// массива CrunMemory
bool SaveOutFileCrunch()
{
	bool bRet = false;
	uint16_t mBinHeader[2] {};

	if ((g_File = _wfopen(g_strOutFileName.c_str(), L"wb")) == nullptr)
	{
		wprintf(L"Ошибка:: не удалось открыть файл для записи '%s'.\n", g_strOutFileName.c_str());
	}
	else
	{
		// добавим заголовок .bin
		mBinHeader[0] = g_nFileAddress;
		mBinHeader[1] = g_nFileLen;

		if (!g_bRaw)
		{
			fwrite(mBinHeader, 1, sizeof(mBinHeader), g_File);
		}

		fwrite(&CrunMemory.b[0], 1, mBinHeader[1], g_File);
		fclose(g_File);
		bRet = true;
	}

	return bRet;
}
// формируем имя выходного файла
void getOutName(int argc, wchar_t **argv, fs::path &ext)
{
	if (argc >= 1)
	{
		g_strOutFileName = (*argv) ? fs::path(*argv) : L"out";
	}
	else
	{
		g_strOutFileName = g_strInFileName;
	}

	// добавим или заменим расширение
	if (g_strOutFileName.has_extension())
	{
		g_strOutFileName.replace_extension(ext);
	}
	else
	{
		g_strOutFileName += ext;
	}
}

int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	_wsetlocale(LC_ALL, L"Russian");
	static struct option long_options[] =
	{
		{ L"help",         ARG_NONE, nullptr, L'?' },
		{ L"tool",         ARG_REQ,  nullptr, L't' },
		{ L"raw",          ARG_NONE, nullptr, L'r' },
		{ L"unpack",       ARG_NONE, nullptr, L'u' },
		{ L"data",         ARG_NONE, nullptr, L'd' },
		{ L"long",         ARG_NONE, nullptr, L'l' },
		{ L"work",         ARG_REQ,  nullptr, L'w' },
		{ nullptr,         ARG_NULL, nullptr, ARG_NULL }
	};
	static wchar_t optstring[] = L"?t:rudlw:";
	int option_index = 0;
	int c;
	memset(&BKPkParam, 0, sizeof(BKPkParam));

	while ((c = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1)
	{
		// Handle options
		c = tolower(c);

		switch (c)
		{
			case L'?':
				Usage();
				return 0;

			case L't':
				if (optarg)
				{
					std::wstring tool = std::wstring(optarg);

					for (auto &i : tool)
					{
						i = std::towupper(i);
					}

					if (tool == L"BKPACK")
					{
						g_tool = BKPACK;
					}
					else if (tool == L"BKCRUNCH")
					{
						g_tool = BKCRUNCH;
					}
					else
					{
						Usage();
						return 0;
					}
				}
				else
				{
					Usage();
					return 0;
				}

				break;

			case L'd':
				BKPkParam.bData = true;
				break;

			case L'l':
				BKPkParam.bLongUnpacker = true;
				break;

			case L'r':
				g_bRaw = true;
				break;

			case L'u':
				g_bUnpack = true;
				break;

			case L'w':
			{
				int nTmp = _tcstol(optarg, nullptr, 8) & 0xffff;

				if (0 <= nTmp && nTmp <= 0140000)
				{
					g_nWorkArea = nTmp;
				}
			}
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1)
	{
		wprintf(L"Ошибка в командной строке: не задано имя входного файла.\n\n");
		Usage();
	}
	else
	{
		g_strInFileName = fs::path(*argv);
		argc--;
		argv++;

		switch (g_tool)
		{
			case BKPACK:
			{
				getOutName(argc, argv, g_bUnpack ? fs::path(L".bkfile") : fs::path(L".bkp"));

				if (OpenInFile())
				{
					if (g_bUnpack)
					{
						if (BKUnPacking(g_nFileLoadAddress, g_nFileLen, g_nFileAddress))
						{
							SaveOutFileBKPack();
						}
					}
					else
					{
						if (BKPacking(0, g_nFileLen, g_nFileAddress))
						{
							SaveOutFileBKPack();
						}
					}
				}
			}
			break;

			case BKCRUNCH:
			{
				getOutName(argc, argv, g_bUnpack ? fs::path(L".bkfile") : fs::path(L".crn"));

				if (OpenInFile())
				{
					if (g_bUnpack)
					{
						CrunUnPack(0, g_nFileLen, g_nFileAddress);
						SaveOutFileCrunch();
					}
					else
					{
						CrunPack(0, g_nFileLen, g_nFileAddress);
						SaveOutFileCrunch();
					}
				}
			}
			break;

			default:
				wprintf(L"Забыли задать правильный рабочий инструмент.\n");
				Usage();
				break;
		}
	}

	return 0;
}

