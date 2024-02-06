// BKDL.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "pch.h"
#include "BKDL.h"
#include <list>
#include <clocale>
#include "getopt.h"

#include "BKFloppyImage_ANDos.h"
#include "BKFloppyImage_MKDos.h"
#include "BKFloppyImage_RT11.h"
#include "BKFloppyImage_Csidos3.h"
#include "BKFloppyImage_HCDos.h"
#include "BKFloppyImage_AODos.h"
#include "BKFloppyImage_NORD.h"
#include "BKFloppyImage_MicroDos.h"
#include "BKFloppyImage_MSDOS.h"
#include "BKFloppyImage_Optok.h"
#include "BKFloppyImage_Holography.h"
#include "BKFloppyImage_DaleOS.h"
#include "StringUtil.h"

// !TODO: доделать вывод сообщений об ошибках в CSV. Сейчас они игнорируются

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


FILE *file_txt = nullptr;
FILE *file_lst = nullptr;

// опции
bool g_bRecursive = false;      // флаг, искать ли образы во вложенных подкаталогах
bool g_bCSV = false;            // флаг работы в режиме этом.
bool g_bShowDeleted = false;    // флаг показа удалённых и плохих файлов

// имена файлов результата работы
const fs::path strCSVFile = L"Files.csv";
fs::path strLstFile = L"Files.lst";
fs::path strTxtFile = L"Files.txt";
// разные сообщения
const wchar_t *strUnableEnterDir = L"Unable to enter the dir %s\n";
const wchar_t *strDirEmpty = L"Nothing at all in current directory!\n";
const wchar_t *strReadError = L"%s = Read Error!\n";
const wchar_t *strOpenError = L"%s = Cannot open!\n";
const wchar_t *strCatalogError = L"%s = Read Catalog Error!\n";
const wchar_t *strParameters = L"%s = Size: %d; Format: %s%s\n";
const wchar_t *strLine = L"-------------------------------------------------------------------------\n";

std::wstring g_strIndent;       // отступы слева для формирования стуктуры вложенностей

CBKParseImage g_imgParser;      // объект парсера

CSV_Fields g_sCSVFields;

// Единственный объект приложения
CWinApp theApp;

#pragma warning(disable:4996)

int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	setlocale(LC_ALL, "Russian");
	int nRetCode = 0;
	HMODULE hModule = ::GetModuleHandle(nullptr);

	if (hModule != nullptr)
	{
		// инициализировать MFC, а также печать и сообщения об ошибках про сбое
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: вставьте сюда код для приложения.
			wprintf(L"Критическая ошибка: сбой при инициализации MFC\n");
			nRetCode = 1;
		}
		else
		{
			static struct option long_options[] =
			{
				{L"help",        ARG_NONE, nullptr, L'h'},
				{L"recursive",   ARG_NONE, nullptr, L'r'},
				{L"csv",         ARG_NONE, nullptr, L'c'},
				{L"del",         ARG_NONE, nullptr, L'd'},
				{L"list",        ARG_REQ,  nullptr, L'l'},
				{L"text",        ARG_REQ,  nullptr, L't'},
				{ ARG_NULL, ARG_NULL, ARG_NULL, ARG_NULL}
			};
			static wchar_t optstring[] = L"hrcdl:t:";
			bool bShowHelp = false;
			int option_index = 0;
			int c;

			/* не прокатывает. если запускаем без параметров, никаких справок,
			сразу пытаемся делать свои дела. с настройками по умолчанию. как правило безуспешно.

			if (argc < 2)
			{
			    // если запускаем без аргументов, то сразу вывести подсказку и выйти.
			    Usage();
			    return 0;
			}*/

			while ((c = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1)
			{
				// Handle options
				switch (c)
				{
					case L'r':
						g_bRecursive = true;
						break;

					case L'c':
						g_bCSV = true;
						strLstFile = strCSVFile;
						break;

					case L'd':
						g_bShowDeleted = true;
						break;

					case L'l':
						if (optarg)
						{
							strLstFile = fs::path(optarg);
						}

						break;

					case L't':
						if (optarg)
						{
							strTxtFile = fs::path(optarg);
						}

						break;

					case L'h':
						bShowHelp = true;
						break;
				}

				if (bShowHelp)
				{
					break;
				}
			}

			if (bShowHelp)
			{
				Usage();
				return 0;
			}

			argc -= optind;
			argv += optind;
			// директория, откуда начинать искать образы
			fs::path strFindPath = (argc < 1) ? fs::current_path() : fs::path(argv[0]);
			file_lst = _wfopen(strLstFile.c_str(), L"wt");
			file_txt = _wfopen(strTxtFile.c_str(), L"wt");

			if (file_lst == nullptr)
			{
				wprintf(L"Ошибка создания файла %s\n", strLstFile.c_str());
			}
			else if (file_txt == nullptr)
			{
				wprintf(L"Ошибка создания файла %s\n", strTxtFile.c_str());
			}
			else
			{
				if (g_bCSV)
				{
					fwprintf(file_lst, L"Image Path; Image Name; OSType; Bootable; LogDisk; Path in Img; Name; Blk Size; Addr; Size; Attr;\n");
				}

				OutProgress(strFindPath);
				ScanDir(strFindPath);
				wprintf(L"\n");
				fclose(file_lst);
				fclose(file_txt);
			}
		}
	}
	else
	{
		// TODO: измените код ошибки в соответствии с потребностями
		wprintf(L"Критическая ошибка: сбой GetModuleHandle\n");
		nRetCode = 1;
	}

	return nRetCode;
}


void Usage()
{
	wprintf(L"\nBKDL v2.0 Создание списка содержимого образов дискет БК.\n" \
	        L"Использование:\n" \
	        L"BKDL -h (--help)\n" \
	        L"  Вывод этой справки.\n" \
	        L"BKDL [-r][-c][-d] [-l<Имя файла>] [-t<Имя файла>] [<Путь>]\n" \
	        L"Все параметры являются необязательными.\n" \
	        L"  -r (--recursive) - искать образы так же и во вложенных подкаталогах,\n" \
	        L"    по умолчанию поиск только в заданном каталоге.\n" \
	        L"  -c (--csv) - на выходе создавать таблицу вместо списка,\n" \
	        L"    по умолчанию создаётся список.\n" \
	        L"  -d (--del) - включать в результаты удалённые и плохие файлы.\n" \
	        L"  -l<Имя файла> (--list <Имя файла>) - задать своё имя файла полного списка,\n" \
	        L"    по умолчанию: Files.lst или Files.csv в режиме csv\n" \
	        L"  -t<Имя файла> (--text <Имя файла>) - задать своё имя файла краткого списка,\n" \
	        L"    по умолчанию: Files.txt\n" \
	        L"  <Путь> - путь к директории в которой ищутся и сканируются образы.\n" \
	        L"Если в пути есть пробелы, его надо брать в кавычки.\n" \
	        L"Если путь не указан, то берётся текущая директория, из которой запущена\n" \
	        L"программа.\n" \
	        L"Файлы полного списка и краткого списка создаются всегда в текущей директории,\n" \
	        L"из которой запущена программа.\n");
}


bool Open(PARSE_RESULT &pr, bool bLogDisk)
{
	if (bLogDisk)
	{
		PushCurrentImg();
	}
	else
	{
		Close();
	}

	switch (pr.imageOSType)
	{
		case IMAGE_TYPE::ANDOS:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_ANDos>(pr);
			break;

		case IMAGE_TYPE::DXDOS:
			// тоже использует андосный класс. (он теперь универсальный,
			// понимает любой формат фат12 за исключением правильных каталогов)
			// как только понадобится, на его основе можно будет сделать настоящий понимальщик мсдосного формата
			m_pFloppyImage = std::make_unique<CBKFloppyImage_ANDos>(pr); // !!! неверно, нужно исправить работу с подкаталогами
			break;

		case IMAGE_TYPE::MSDOS:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_MSDOS>(pr); // !!! Недоделано!
			break;

		case IMAGE_TYPE::MKDOS:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_MKDos>(pr);
			break;

		case IMAGE_TYPE::NCDOS:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_HCDos>(pr);
			break;

		case IMAGE_TYPE::AODOS:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_AODos>(pr);
			break;

		case IMAGE_TYPE::NORD:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_Nord>(pr);
			break;

		case IMAGE_TYPE::MIKRODOS:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_MicroDos>(pr);
			break;

		case IMAGE_TYPE::CSIDOS3:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_Csidos3>(pr);
			break;

		case IMAGE_TYPE::RT11:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_RT11>(pr);
			break;

		case IMAGE_TYPE::OPTOK:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_Optok>(pr);
			break;

		case IMAGE_TYPE::HOLOGRAPHY:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_Holography>(pr);
			break;

		case IMAGE_TYPE::DALE:
			m_pFloppyImage = std::make_unique < СBKFloppyImage_DaleOS > (pr);
			break;

		default:
			ASSERT(false);
	}

	if (m_pFloppyImage)
	{
		m_pFloppyImage->OpenFloppyImage();
	}

	return ReOpen();
}

bool ReOpen()
{
	if (m_pFloppyImage)
	{
		return true;
	}

	return false;
}


void Close()
{
	if (m_pFloppyImage)
	{
		m_pFloppyImage->CloseFloppyImage();
		m_pFloppyImage.reset();
	}
}

const std::wstring GetImgFormatName(IMAGE_TYPE nType)
{
	if (nType == IMAGE_TYPE::UNKNOWN && m_pFloppyImage)
	{
		nType = m_pFloppyImage->GetImgOSType();
	}

	return CBKParseImage::GetOSName(nType);
}

void ClearImgVector()
{
	Close();

	for (auto &bkdi : m_vpImages)
	{
		bkdi.reset();
	}

	m_vpImages.clear();
	m_PaneInfo.clear();
}

void PushCurrentImg()
{
	m_vpImages.push_back(std::move(m_pFloppyImage));
}

bool PopCurrentImg()
{
	if (!m_vpImages.empty())
	{
		if (m_pFloppyImage)
		{
			m_pFloppyImage.reset();
		}

		m_pFloppyImage = std::move(m_vpImages.back());
		m_vpImages.pop_back();
		return true;
	}

	return false;
}


void OutProgress(const fs::path &strPath)
{
	static int nFilesTotal = -1; // начинаем с -1 потому что такой выбран алгоритм, первый вызов делается до цикла, остальные - в цикле.
	// чтобы строка влезала в 80 символов, надо урезать слишком длинные пути
	// длина постоянного текстового сообщения 30 символов, предполагаем 4хзначное число обработанных образов
	// значит на путь остаётся 50 символов
	std::wstring outPath = strPath.wstring();
	auto nPathLen = outPath.length();

	if (nPathLen > 50)
	{
		outPath = L"..." + outPath.substr(nPathLen - 47); // 47 - чтобы 3 точки полезали
	}

	wprintf(L"Images processed: %d; Dir: %-50s\r", ++nFilesTotal, outPath.c_str());
}

// сканирование директории на наличие в ней образов и их обработка
bool ScanDir(const fs::path &strInPath)
{
	std::list<fs::path> listDirs; // список найденных директорий для рекурсивной обработки
	g_sCSVFields.strImagePath = strInPath;

	if (!g_bCSV)
	{
		fwprintf(file_lst, L"%s\n", strInPath.c_str());
		fwprintf(file_lst, strLine);
	}

	fwprintf(file_txt, L"%s\n", strInPath.c_str());
	fwprintf(file_txt, strLine);
	std::error_code ec;
	fs::current_path(strInPath, ec);

	if (ec)
	{
		fwprintf(file_lst, strUnableEnterDir, strInPath.c_str());
		fwprintf(file_txt, strUnableEnterDir, strInPath.c_str());
		return false;
	}

	for (auto const &it : fs::directory_iterator(strInPath, fs::directory_options::skip_permission_denied | fs::directory_options::follow_directory_symlink, ec))
	{
		fs::path strImgPathName = it.path(); // имя найденного файла

		if (fs::is_directory(strImgPathName))
		{
			if (g_bRecursive)
			{
				// директорию помещаем в список на обработку
				listDirs.emplace_back(strImgPathName);
			}
			else
			{
				// если не рекурсивно - все директории игнорируем
				continue;
			}
		}
		else if (fs::is_regular_file(strImgPathName))
		{
			// файлы образов будем выбирать по расширениям.
			// хотя можно и по результатам парсинга, но так не принято
			std::wstring strExt = strUtil::strToLower(strImgPathName.extension().wstring());

			for (const auto &pstrExt : g_pstrExts)
			{
				if (strExt == pstrExt) // если файл с нужным расширением
				{
					fs::path strName = strImgPathName.filename(); // только имя без пути
					g_sCSVFields.strImageName = strName;
					PARSE_RESULT pres = g_imgParser.ParseImage(strImgPathName, 0);

					if (pres.imageOSType == IMAGE_TYPE::ERROR_NOIMAGE)
					{
						fwprintf(file_lst, strReadError, strName.c_str());
						fwprintf(file_txt, strReadError, strName.c_str());
					}
					else
					{
						std::wstring strFormat = GetImgFormatName(pres.imageOSType);
						g_sCSVFields.strOSType = strFormat;
						g_sCSVFields.bBootable = pres.bImageBootable;
						g_sCSVFields.bLogDisk = false;

						if (!g_bCSV)
						{
							std::wstring strBootable = (pres.bImageBootable) ? L"; Bootable" : L"";
							fwprintf(file_lst, strParameters, strName.c_str(), pres.nImageSize, strFormat.c_str(), strBootable.c_str());
						}

						fwprintf(file_txt, _T("%s\n"), strImgPathName.c_str());

						if (pres.imageOSType != IMAGE_TYPE::UNKNOWN) // если файловая система опознана - обрабатываем
						{
							ClearImgVector(); // очищаем всё
							g_strIndent.clear(); // отступы тоже

							if (Open(pres)) // если файл открылся
							{
								if (m_pFloppyImage->ReadCurrentDir()) // читаем каталог образа
								{
									m_PaneInfo.strCurrPath = L"/"; // начинаем с корневой директории
									// и выведем каталог файлов рекурсивно
									OutDir();
								}
								else
								{
									if (!g_bCSV)
									{
										fwprintf(file_lst, strCatalogError, strName.c_str());
									}

									fwprintf(file_txt, strCatalogError, strName.c_str());
								}

								Close();
							}
							else
							{
								if (!g_bCSV)
								{
									fwprintf(file_lst, strOpenError, strName.c_str());
								}

								fwprintf(file_txt, strOpenError, strName.c_str());
							}

							if (!g_bCSV)
							{
								fwprintf(file_lst, L"\n\n");
							}

							fwprintf(file_txt, L"\n\n");
						}

						OutProgress(strInPath);
					}

					break;
				}
			}
		}
		else
		{
			// остальное будем игнорировать
			ASSERT(false);
		}
	}

	// сперва обработаем все файлы в подкаталоге, а потом обработаем вложенные подкаталоги
	// если такая опция задана
	if (g_bRecursive)
	{
		while (!listDirs.empty())
		{
			ScanDir(listDirs.front());
			listDirs.pop_front();
		}
	}

	listDirs.clear(); // на всякий случай. хотя и так должно быть там пусто
	return true;
}

// рекурсивный вывод каталога образа
bool OutDir()
{
	std::list<BKDirDataItem> listBKDirs;   // список найденных директорий для рекурсивной обработки
	g_sCSVFields.strInImgPath = m_PaneInfo.strCurrPath;

	if (!g_bCSV)
	{
		fwprintf(file_lst, L"[+] %s\n", m_PaneInfo.strCurrPath.c_str());
	}

	fwprintf(file_txt, L"%s[+] %s\n", g_strIndent.c_str(), m_PaneInfo.strCurrPath.c_str());
	g_strIndent += L"    "; // увеличим отступ для файлов этой директории
	bool bRet = true;
	// начинаем
	BKDirDataItem fr;
	bool bEnd = m_pFloppyImage->GetStartFileName(&fr);

	if (bEnd)
	{
		do
		{
			if (fr.nRecType != BKDirDataItem::RECORD_TYPE::UP) // если это не ".."
			{
				if (fr.nAttr & FR_ATTR::DIR || fr.nAttr & FR_ATTR::LOGDISK)
				{
					if (fr.nAttr & FR_ATTR::LINK)
					{
						// если ссылка
						fwprintf(file_lst, L"    %-16s = LINK\n", fr.strName.c_str());
						fwprintf(file_txt, L"%s%s (LINK)\n", g_strIndent.c_str(), fr.strName.c_str());
					}
					else
					{
						// если это директория или лог. диск
						// то поместим их в список для обработки
						listBKDirs.push_back(fr);
					}
				}
				else
				{
					g_sCSVFields.strInImgName = fr.strName;
					g_sCSVFields.nBlkSize = fr.nBlkSize;
					g_sCSVFields.nAddr = fr.nAddress;
					g_sCSVFields.nSize = fr.nSize;
					// сформируем список атрибутов
					std::wstring strAttr;
					bool bIsAttr = false;

					if (fr.nAttr & FR_ATTR::DELETED)
					{
						strAttr += L"DEL";
						bIsAttr = true;
					}

					if (fr.nAttr & FR_ATTR::HIDDEN)
					{
						if (!strAttr.empty())
						{
							strAttr.push_back(L' ');
						}

						strAttr += L"HDN";
						bIsAttr = true;
					}

					if (fr.nAttr & FR_ATTR::BAD)
					{
						if (!strAttr.empty())
						{
							strAttr.push_back(L' ');
						}

						strAttr += L"BAD";
						bIsAttr = true;
					}

					if (bIsAttr)
					{
						g_sCSVFields.strAttr = strAttr;
					}
					else
					{
						g_sCSVFields.strAttr.clear();
					}

					if (!(fr.nAttr & FR_ATTR::DELETED || fr.nAttr & FR_ATTR::BAD) || g_bShowDeleted)
					{
						if (g_bCSV)
						{
							fwprintf(file_lst, L"%s;%s;%s;%s;%s;%s;%s;%d;%d;%d;%s\n",
							         g_sCSVFields.strImagePath.c_str(),
							         g_sCSVFields.strImageName.c_str(),
							         g_sCSVFields.strOSType.c_str(),
							         (g_sCSVFields.bBootable ? L"Yes" : L"No"),
							         (g_sCSVFields.bLogDisk ? L"Yes" : L"No"),
							         g_sCSVFields.strInImgPath.c_str(),
							         g_sCSVFields.strInImgName.c_str(),
							         g_sCSVFields.nBlkSize,
							         g_sCSVFields.nAddr,
							         g_sCSVFields.nSize,
							         g_sCSVFields.strAttr.c_str());
						}
						else
						{
							fwprintf(file_lst, L"    %-16s = Blk: %04d ; Addr: %06o ; Size: %06o", fr.strName.c_str(),
							         fr.nBlkSize, fr.nAddress, fr.nSize);

							if (bIsAttr)
							{
								fwprintf(file_lst, L"  [%s]", strAttr.c_str());
							}

							fwprintf(file_lst, L"\n");
						}

						// если файл
						fwprintf(file_txt, L"%s%s", g_strIndent.c_str(), fr.strName.c_str());

						if (bIsAttr)
						{
							fwprintf(file_txt, L"  [%s]", strAttr.c_str());
						}

						fwprintf(file_txt, L"\n");
					}
				}
			}
		}
		while (bEnd = m_pFloppyImage->GetNextFileName(&fr));
	}

	// теперь обработаем все каталоги и лог диски
	while (!listBKDirs.empty())
	{
		// если директория, надо в неё зайти и получить список всех записей.
		fr = listBKDirs.front();
		BKDirDataItem efr = fr; // запись для выхода из директории
		listBKDirs.pop_front();

		if (fr.nAttr & FR_ATTR::LOGDISK)
		{
			g_sCSVFields.bLogDisk = true;
			// если лог диск
			unsigned long nBaseOffset = m_pFloppyImage->GetBaseOffset() + fr.nStartBlock * BLOCK_SIZE;
			PARSE_RESULT pres = g_imgParser.ParseImage(m_pFloppyImage->GetCurrImgName(), nBaseOffset);

			if (pres.imageOSType == IMAGE_TYPE::ERROR_NOIMAGE)
			{
				if (!g_bCSV)
				{
					fwprintf(file_lst, L"[*]%s%s = LOGDISK: IO Error!\n", m_PaneInfo.strCurrPath.c_str(), pres.strName.c_str());
				}

				fwprintf(file_txt, L"%s[*]%s%s = LOGDISK: IO Error!\n", g_strIndent.c_str(), m_PaneInfo.strCurrPath.c_str(), pres.strName.c_str());
			}
			else if (pres.imageOSType == IMAGE_TYPE::UNKNOWN)
			{
				// нужно отдельно обрабатывать такую ситуацию
				std::wstring strFormat = GetImgFormatName(pres.imageOSType);

				if (!g_bCSV)
				{
					fwprintf(file_lst, L"[+] %s%s = LOGDISK: %s  Blk: %04d ; Size: %06o\n", m_PaneInfo.strCurrPath.c_str(), fr.strName.c_str(), strFormat.c_str(), fr.nBlkSize, fr.nSize);
				}

				fwprintf(file_txt, L"%s[+] %s%s = LOGDISK: %s\n", g_strIndent.c_str(), m_PaneInfo.strCurrPath.c_str(), fr.strName.c_str(), strFormat.c_str());
			}
			else
			{
				std::wstring strFormat = GetImgFormatName(pres.imageOSType);
				std::wstring strBootable = (pres.bImageBootable) ? L"; Bootable" : L"";

				if (!g_bCSV)
				{
					fwprintf(file_lst, L"[+] %s%s = LOGDISK: %s%s  Blk: %04d ; Size: %06o\n", m_PaneInfo.strCurrPath.c_str(), fr.strName.c_str(), strFormat.c_str(), strBootable.c_str(), fr.nBlkSize, fr.nSize);
				}

				fwprintf(file_txt, L"%s[+] %s%s = LOGDISK: %s%s\n", g_strIndent.c_str(), m_PaneInfo.strCurrPath.c_str(), fr.strName.c_str(), strFormat.c_str(), strBootable.c_str());
				std::wstring strOtstup1 = g_strIndent;
				m_vSelItems.push_back(m_PaneInfo); // сохраняем текущее
				// заполняем новое.
				std::wstring name = strUtil::replaceChar(fr.strName, L'/', L'_'); // меняем '/' на '_', чтобы не путалась.
				m_PaneInfo.strCurrPath += name + L"/";
				m_PaneInfo.nCurDir = fr.nDirNum;
				m_PaneInfo.nParentDir = fr.nDirBelong;
				m_PaneInfo.nCurItem = 0;

				if (Open(pres, true))
				{
					if (m_pFloppyImage->ReadCurrentDir()) // читаем каталог образа
					{
						// и выведем каталог рекурсивно
						bRet = OutDir();
					}
					else
					{
						bRet = false;
					}

					if (!PopCurrentImg())
					{
						return false;
					}

					if (!ReOpen())
					{
						return false;
					}
				}
				else
				{
					bRet = false;
				}

				g_strIndent = strOtstup1;

				// различие выхода из образа и из лог.диска - не пустой вектор.
				// если вектор пуст - то выход из образа, иначе - выход из лог.диска
				if (!m_vSelItems.empty())
				{
					m_PaneInfo = m_vSelItems.back();
					m_vSelItems.pop_back();
				}
				else
				{
					m_PaneInfo.clear();
					m_PaneInfo.nCurDir = -1;
				}
			}

			g_sCSVFields.bLogDisk = false;
		}
		else
		{
			if (m_pFloppyImage->ChangeDir(&fr))
			{
				m_vSelItems.push_back(m_PaneInfo); // сохраняем текущее
				// заполняем новое.
				std::wstring name = strUtil::replaceChar(fr.strName.wstring(), L'/', L'_'); // меняем '/' на '_', чтобы не путалась.
				m_PaneInfo.strCurrPath += name + L"/";
				m_PaneInfo.nCurDir = fr.nDirNum;
				m_PaneInfo.nParentDir = fr.nDirBelong;
				m_PaneInfo.nCurItem = 0;
				bRet = OutDir();

				if (!bRet)
				{
					return bRet;
				}

				// тут надо выйти из директории.
				efr.nDirNum = fr.nDirBelong;
				efr.nRecType = BKDirDataItem::RECORD_TYPE::UP;
				efr.strName = L"..";

				if (m_pFloppyImage->ChangeDir(&efr))
				{
					g_strIndent = g_strIndent.substr(0, g_strIndent.length() - 4);

					if (m_pFloppyImage->GetCurrDirNum() == -1)
					{
						bRet = false;
					}

					// различие выхода из образа и из лог.диска - не пустой вектор.
					// если вектор пуст - то выход из образа, иначе - выход из лог.диска
					if (!m_vSelItems.empty())
					{
						m_PaneInfo = m_vSelItems.back();
						m_vSelItems.pop_back();
					}
					else
					{
						m_PaneInfo.clear();
						m_PaneInfo.nCurDir = -1;
					}
				}
				else
				{
					if (!g_bCSV)
					{
						fwprintf(file_lst, L"Cannot exit from dir: %s\n", fr.strName.c_str());
					}

					fwprintf(file_txt, L"Cannot exit from dir: %s\n", fr.strName.c_str());
					bRet = false;
				}
			}
			else
			{
				if (!g_bCSV)
				{
					fwprintf(file_lst, L"Cannot enter to dir: %s\n", fr.strName.c_str());
				}

				fwprintf(file_txt, L"Cannot enter to dir: %s\n", fr.strName.c_str());
				bRet = false;
			}
		}
	}

	return bRet;
}

