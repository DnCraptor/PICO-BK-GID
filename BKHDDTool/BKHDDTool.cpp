/*
*  работа с образами винчестера БК в форматах "Самара" и "Альтпро"
*  Прога по мотивам исходника bkhdd.cpp (c) 2010 Terra software
*/
// BKHDDTool.cpp: определяет точку входа для консольного приложения.
//

#include "pch.h"

#include "BKHDDTool.h"
#include <clocale>
#include <ctime>

#include "getopt.h"

// mbr для Альтпро
uint16_t g_MBRAltPro[SECTOR_SIZEW] = { 0xA0, 0x15DF, 0xFF87, 0xFFEA, 0x15DF, 0x1C, 0xFFE0, 0x87, 0x0, };

USector g_sector;
ProrgamOptions g_options;

std::wstring g_HDIModelName = L"BKEMU HARD DRIVE IMAGE";

static const std::wstring g_nameAltpro = L"АльтПро";
static const std::wstring g_nameSamara = L"Самара";

static const std::wstring g_pstrExts[3] =
{
	L".img",
	L".bkd",
	L".dsk"
};

int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	srand((unsigned)time(nullptr));
	setlocale(LC_ALL, "Russian");
	int nRetCode = 0;
	FILE *pHDDfile = nullptr;
	g_options.strAppPath = fs::current_path();
	static struct option long_options[] =
	{
		{ L"help",        ARG_NONE, nullptr, L'?' },
		{ L"altpro",      ARG_NONE, nullptr, L'a' },
		{ L"samara",      ARG_NONE, nullptr, L'm' },
		{ L"hdi",         ARG_NONE, nullptr, L'i' },
		{ L"convert",     ARG_NONE, nullptr, L'o' },
		{ L"path",        ARG_REQ,  nullptr, L'p' },
		{ L"cylinder",    ARG_REQ,  nullptr, L'c' },
		{ L"head",        ARG_REQ,  nullptr, L'h' },
		{ L"sector",      ARG_REQ,  nullptr, L's' },
		{ nullptr,        ARG_NULL, nullptr, ARG_NULL }
	};
	static wchar_t optstring[] = L"?amiop:c:h:s:";
	int option_index = 0;
	bool bShowHelp = false;
	int c;

	while ((c = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1)
	{
		// Handle options
		switch (c)
		{
			case L'?':
				bShowHelp = true;
				break;

			case L'a':
				g_options.bAltPro = true;
				g_options.bSamara = false;
				break;

			case L'm':
				g_options.bAltPro = false;
				g_options.bSamara = true;
				break;

			case L'i':
				g_options.bHDI = true;
				break;

			case L'p':
				g_options.strPath = optarg ? fs::path(optarg) : g_options.strAppPath;
				break;

			case L'o':
				g_options.bConvert = true;
				break;

			case L'c':
				g_options.C = _tcstol(optarg, nullptr, 10) & 0xffff;
				g_options.bCreate = true;
				break;

			case L'h':
				g_options.H = _tcstol(optarg, nullptr, 10) & 0xffff;
				g_options.bCreate = true;
				break;

			case L's':
				g_options.S = _tcstol(optarg, nullptr, 10) & 0xffff;
				g_options.bCreate = true;
				break;
		}

		if (bShowHelp)
		{
			BKHDDTool::Usage();
			return 0;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1)
	{
		BKHDDTool::Usage();
	}
	else
	{
		g_options.strImgName = fs::path(argv[0]);

		if (g_options.bCreate)
		{
			if (!g_options.strImgName.has_extension())
			{
				g_options.strImgName.replace_extension(L".img");
			}

			if (g_options.bHDI)
			{
				// если создаём hdi файл, то подменим расширение.
				g_options.strImgName.replace_extension(L".hdi");
			}

			// создаём образ
			pHDDfile = _wfopen(g_options.strImgName.c_str(), L"wb");

			if (pHDDfile)
			{
				IMGFormat format;
				format.bHDI = g_options.bHDI;
				format.bSamara = g_options.bSamara;
				format.C = g_options.C;
				format.H = g_options.H;
				format.S = g_options.S;
				format.nLogDiskNum = 0;

				if (g_options.bHDI)
				{
					// создаём заголовочный сектор HDI.
					HDIStuff::CreateHDISector(&format, reinterpret_cast<SYS_SECTOR *>(g_sector.b), HDIStuff::GenerateSerialNumber(), g_HDIModelName);

					if (SECTOR_SIZEB != fwrite(g_sector.b, 1, SECTOR_SIZEB, pHDDfile))
					{
						wprintf(L"Не удалось записать HDI сектор в файл '%s'\n", g_options.strImgName.c_str());
					}
				}

				BKHDDTool::AssemblyImage(pHDDfile, &format);
				fclose(pHDDfile);
			}
			else
			{
				wprintf(L"Не удалось создать файл '%s'\n", g_options.strImgName.c_str());
				nRetCode = 1;
			}
		}
		else
		{
			// разбираем образ
			pHDDfile = _wfopen(g_options.strImgName.c_str(), L"rb");

			if (pHDDfile)
			{
				IMGFormat format;

				if (BKHDDTool::AnalyseImage(pHDDfile, &format))
				{
					if (g_options.bConvert)
					{
						if (format.bSamara)
						{
							BKHDDTool::ConvertSamara2Altpro(pHDDfile, &format);
						}
						else
						{
							BKHDDTool::ConvertAltPro2Samara(pHDDfile, &format);
						}
					}
					else
					{
						if (format.bSamara)
						{
							BKHDDTool::SplitSamara(pHDDfile, &format);
						}
						else
						{
							BKHDDTool::SplitAltPro(pHDDfile, &format);
						}
					}
				}

				fclose(pHDDfile);
			}
			else
			{
				wprintf(L"Не удалось открыть файл '%s'\n", g_options.strImgName.c_str());
				nRetCode = 1;
			}
		}
	}

	return nRetCode;
}

void BKHDDTool::PrintInfo(IMGFormat *imgf)
{
	wprintf(L"Тип образа: %s\n", (imgf->bHDI) ? L"HDI" : L"IMG");
	wprintf(L"Формат образа: %s\n", (imgf->bSamara) ? g_nameSamara.c_str() : g_nameAltpro.c_str());
	wprintf(L"Геометрия\nКоличество цилиндров: %d\n", imgf->C);
	wprintf(L"Количество головок:   %d\n", imgf->H);
	wprintf(L"Количество секторов:  %d\n", imgf->S);
}

bool BKHDDTool::AssemblyImage(FILE *f, IMGFormat *imgf)
{
	PrintInfo(imgf);
	MakeOutDir(true);

	const bool bRet = (imgf->bSamara) ? AssemblySamara(f, imgf) : AssemblyAltPro(f, imgf);

	return bRet;
}

bool BKHDDTool::AnalyseImage(FILE *f, IMGFormat *imgf)
{
	if (CheckFormat_hdd(f, imgf))
	{
		PrintInfo(imgf);
		wprintf(L"\nКоличество разделов: %d\n", imgf->nLogDiskNum);
		MakeOutDir();
		return true;
	}

	if (imgf->nIOStatus == IMGIOSTATUS::IO_ERROR)
	{
		wprintf(L"Ошибка чтения файла!\n");
	}
	else
	{
		wprintf(L"Формат нераспознан!\n");
	}

	return false;
}

void BKHDDTool::ShowMsg1(const std::wstring &n1, const std::wstring &n2)
{
	wprintf(L"Формат образа опознан как '%s', но в опциях задано обрабатывать как '%s'\n", n1.c_str(), n2.c_str());
	wprintf(L"Образ будет обработан как '%s'\n", n1.c_str());
}

void BKHDDTool::ShowMsg2(const std::wstring &n1)
{
	wprintf(L"Формат образа не опознан, но в опциях задано обрабатывать как '%s'\n", n1.c_str());
	wprintf(L"Образ будет обработан как '%s'\n", n1.c_str());
}

// проверка формата и заодно определение геометрии
// выход: true - параметры структуры IMGFormat достоверны
//       false - нет. формат не распознан
bool BKHDDTool::CheckFormat_hdd(FILE *f, IMGFormat *imgf)
{
	if (fread(g_sector.b, 1, SECTOR_SIZEB, f) == SECTOR_SIZEB)  // прочтём первый сектор.
	{
		// подсчитаем контрольную сумму
		if (HDIStuff::CheckCS(&g_sector))
		{
			imgf->bHDI = true; // если первый сектор правильный - то это вероятно образ HDI
		}
		else
		{
			imgf->bHDI = false; // если нет - то это просто IMG
			fseek(f, 0, SEEK_SET); // вернёмся к началу образа.
		}
	}
	else
	{
		imgf->nIOStatus = IMGIOSTATUS::IO_ERROR;
		return false;
	}

	IMGFormat aimgf(imgf);
	IMGFormat simgf(imgf);
	bool bSamara = false;
	bool bAltPro = HDIStuff::CheckAltPro(f, &aimgf);  // проверка на альтпро.

	if (aimgf.nIOStatus == IMGIOSTATUS::IO_ERROR)
	{
		return false;
	}

	if (bAltPro)
	{
		*imgf = aimgf;
	}
	else
	{
		bSamara = HDIStuff::CheckSamara(f, &simgf);   // проверка на формат Самара

		if (simgf.nIOStatus == IMGIOSTATUS::IO_ERROR)
		{
			return false;
		}

		if (bSamara)
		{
			*imgf = simgf;
		}
	}

	bool bRet = bAltPro || bSamara;

	if (bSamara && g_options.bAltPro)
	{
		ShowMsg1(g_nameSamara, g_nameAltpro);
	}
	else if (bAltPro && g_options.bSamara)
	{
		ShowMsg1(g_nameAltpro, g_nameSamara);
	}

	if (!bRet) // образ никак неопознан.
	{
		if (g_options.bAltPro)
		{
			ShowMsg2(g_nameAltpro);
			g_options.bSamara = false;
			*imgf = aimgf;
			bRet = true;
		}
		else if (g_options.bSamara)
		{
			ShowMsg2(g_nameSamara);
			g_options.bSamara = true;
			*imgf = simgf;
			bRet = true;
		}
	}

	return bRet;
}

// разбираем образ винта альтпро на лог.диски
bool BKHDDTool::SplitAltPro(FILE *f, IMGFormat *imgf)
{
	bool bRet = false;
	int nErrors = 0;
	imgf->nIOStatus = IMGIOSTATUS::IO_OK;

	// прочитаем сектор 8, нам нужна таблица разделов.
	if ((fseek(f, (AHDD_PT_SEC + (imgf->bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET) == 0)
	        && (fread(g_sector.b, 1, SECTOR_SIZEB, f) == SECTOR_SIZEB))
	{
		HDIStuff::InverseSector(g_sector);

		for (int nld = 0; nld < imgf->nLogDiskNum; ++nld)
		{
			uint16_t nCyl = g_sector.w[AHDD_PART_W - nld * 2]; // первое слово из таблицы разделов (старший бит - флаг защиты от записи)
			bool bProtected = false;

			if (static_cast<short>(nCyl) < 0)
			{
				bProtected = true;
				nCyl = ~nCyl;
			}

			uint16_t nHead = nCyl & 0xF; // номер головки из таблицы разделов
			nCyl >>= 4; // номер дорожки из таблицы разделов
			uint32_t lba = (nCyl * imgf->H + nHead) * imgf->S; // рассчитываем начало раздела в блоках
			uint32_t nLen = g_sector.w[AHDD_PART_W - 1 - nld * 2]; // размер логического диска в блоках
			wprintf(L"part #%d:: %c: cyl = %d\thd = %d\tsec = 1\tlba = %ld\tlen = %d", nld, L'C' + nld, nCyl, nHead, lba, nLen);

			if (bProtected)
			{
				wprintf(L" [WP]\n");
			}
			else
			{
				wprintf(L" [ ]\n");
			}

			if (fseek(f, (lba + (imgf->bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET) == 0)
			{
				// переместимся к началу раздела
				if (!(bRet = WriteLogDiskImage(f, nld, nLen)))
				{
					wprintf(L"\nОшибка извлечения раздела %d\n", nld);
					nErrors++;
				}
			}
			else
			{
				wprintf(L"\nОшибка перемещения к разделу %d\n", nld);
				nErrors++;
			}
		}

		wprintf(L"\nУспешно извлечено %d разделов\n", imgf->nLogDiskNum - nErrors);

		if (nErrors)
		{
			wprintf(L"С ошибками - %d разделов\n", nErrors);
		}
	}
	else
	{
		imgf->nIOStatus = IMGIOSTATUS::IO_ERROR;
		wprintf(L"Ошибка чтения таблицы разделов формата '%s'\n", g_nameAltpro.c_str());
	}

	return bRet;
}

// разбираем образ винта самара на лог.диски
bool BKHDDTool::SplitSamara(FILE *f, IMGFormat *imgf)
{
	bool bRet = false;
	int nErrors = 0;
	USector buf{};
	imgf->nIOStatus = IMGIOSTATUS::IO_OK;

	// прочитаем сектор 2, нам нужна таблица разделов.
	if ((fseek(f, (SHDD_PT_SEC + (imgf->bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET) == 0)
	        && (fread(g_sector.b, 1, SECTOR_SIZEB, f) == SECTOR_SIZEB))
	{
		HDIStuff::InverseSector(g_sector);

		for (int nld = 0; nld < imgf->nLogDiskNum; ++nld)
		{
			uint32_t lba = ((uint32_t(g_sector.w[SHDD_PART_W + 1 + nld * 2]) << 16) | uint32_t(g_sector.w[SHDD_PART_W + nld * 2]));

			// перемещаемся к началу лог диска
			if (fseek(f, (lba + (imgf->bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET) == 0)
			{
				// читаем начальный блок
				if ((fread(buf.b, 1, SECTOR_SIZEB, f)) == SECTOR_SIZEB)
				{
					HDIStuff::InverseSector(buf);
					uint16_t nLen = buf.w[SHDD_LD_LEN_W];
					uint8_t num = buf.b[SHDD_NLD_W];
					uint8_t attr = buf.b[SHDD_LD_FLAGS_W * 2];
					wprintf(L"part #%d:: %c: lba = %ld\tlen = %d\t", nld, L'A' + num, lba, nLen);
					wprintf(L"ldr=%06o bpar=%06o pg=%06o [", buf.w[SHDD_ADR_BOOT_W], buf.w[SHHD_ADR_PAR_W], buf.w[SHDD_PAGE_W]);

					if (attr & ALD_ATTR::BAD)
					{
						wprintf(L"BAD ");
					}

					if (attr & ALD_ATTR::WP)
					{
						wprintf(L"WP ");
					}

					if (attr & ALD_ATTR::RP)
					{
						wprintf(L"RP ");
					}

					if (attr & ALD_ATTR::FWP)
					{
						wprintf(L"FWP ");
					}

					if (attr & ALD_ATTR::FRP)
					{
						wprintf(L"FRP ");
					}

					wprintf(L"]\n");

					// переместимся к началу раздела
					if (!(bRet = WriteLogDiskImage(f, nld, nLen)))
					{
						wprintf(L"\nОшибка извлечения раздела %d\n", nld);
						nErrors++;
					}
				}
				else
				{
					wprintf(L"\nОшибка чтения начального блока раздела %d\n", nld);
					nErrors++;
				}
			}
			else
			{
				wprintf(L"\nОшибка перемещения к начальному блоку раздела %d\n", nld);
				nErrors++;
			}
		}

		wprintf(L"\nУспешно извлечено %d разделов\n", imgf->nLogDiskNum - nErrors);

		if (nErrors)
		{
			wprintf(L"С ошибками - %d разделов\n", nErrors);
		}
	}
	else
	{
		imgf->nIOStatus = IMGIOSTATUS::IO_ERROR;
		wprintf(L"Ошибка чтения таблицы разделов формата '%s'\n", g_nameSamara.c_str());
	}

	return bRet;
}

CBKParseImage g_ImgParser;

void BKHDDTool::PrintOSType(const PARSE_RESULT &pr)
{
	const std::wstring strOS = CBKParseImage::GetOSName(pr.imageOSType);
	wprintf(L"%s\n", strOS.c_str());
}

// запись раздела из образа винчестера в отдельный образ
bool BKHDDTool::WriteLogDiskImage(FILE *f, int nld, uint32_t nLen)
{
	bool bRet = true;
	USector buf{};
	wchar_t pOutName[64] = { 0 };
	// сформируем очередное имя
	_swprintf(pOutName, L"%03d_part.bkd", nld);
	FILE *outf = _wfopen(pOutName, L"wb");

	if (outf)
	{
		for (uint32_t i = 0; i < nLen; ++i)
		{
			if (SECTOR_SIZEB == fread(buf.b, 1, SECTOR_SIZEB, f))
			{
				HDIStuff::InverseSector(buf);

				if (SECTOR_SIZEB != fwrite(buf.b, 1, SECTOR_SIZEB, outf))
				{
					wprintf(L"\nОшибка записи в образ FDD %s\n", pOutName);
					bRet = false;
					break;
				}
			}
			else
			{
				wprintf(L"\nОшибка чтения из образа HDD %s\n", g_options.strImgName.c_str());
				bRet = false;
				break;
			}
		}

		fclose(outf);

		if (bRet)
		{
			PARSE_RESULT pr = g_ImgParser.ParseImage(fs::path(pOutName), 0);
			wprintf(L"ОС: ");
			PrintOSType(pr);
		}
	}
	else
	{
		bRet = false;
	}

	return bRet;
}

// запись раздела из отдельного образа в образ винчестера
bool BKHDDTool::ReadLogDiskImage(FILE *f, IMGFormat *imgf, const fs::path &strName)
{
	bool bRet = true;
	USector buf{};
	PARSE_RESULT pr = g_ImgParser.ParseImage(strName, 0);

	if (pr.imageOSType != IMAGE_TYPE::ERROR_NOIMAGE)
	{
		unsigned long len = (pr.nImageSize + (SECTOR_SIZEB - 1)) / SECTOR_SIZEB; // размер файла в блоках
		// если ОС раздела не опознана, то берём размер файла в блоках, иначе - берём размер из ОС
		unsigned long partLen = (pr.imageOSType == IMAGE_TYPE::UNKNOWN) ? len : pr.nPartLen;

		// если размер оказался нулевым
		if (partLen == 0)
		{
			partLen = 1600; // заменим размером по умолчанию
		}

		FILE *inf = _wfopen(strName.c_str(), L"rb");

		if (inf)
		{
			wprintf(L"Размер файла в блоках: %d, размер раздела: %d, ОС: ", len, partLen);
			PrintOSType(pr);

			for (unsigned long i = 0; i < partLen; ++i)
			{
				memset(buf.b, 0, SECTOR_SIZEB); // для начала очистим буфер. если размер файла будет не кратен блоку,
				// остаток сектора должен быть заполнен нулями.

				if (!feof(inf))
				{
					fread(buf.b, 1, SECTOR_SIZEB, inf); // читаем сколько сможем
				}

				if (ferror(inf))
				{
					wprintf(L"\nОшибка чтения из образа FDD %s\n", strName.c_str());
					bRet = false;
					break;
				}

				HDIStuff::InverseSector(buf);

				if (SECTOR_SIZEB != fwrite(buf.b, 1, SECTOR_SIZEB, f))
				{
					imgf->nIOStatus = IMGIOSTATUS::IO_ERROR;
					wprintf(L"\nОшибка записи в образ HDD %s\n", g_options.strImgName.c_str());
					bRet = false;
					break;
				}

				if (++imgf->nSector == imgf->S)
				{
					imgf->nSector = 0;

					if (++imgf->nHead == imgf->H)
					{
						imgf->nHead = 0;

						if (++imgf->nCyl > imgf->C)
						{
							// wprintf(L"Переполнение образа диска\n");
							imgf->C = imgf->nCyl; // корректируем размер в сторону увеличения.
							bRet = false;
						}
					}
				}
			}

			fclose(inf);
		}
		else
		{
			bRet = false;
		}
	}
	else
	{
		bRet = false;
	}

	return bRet;
}

void BKHDDTool::MakeOutDir(bool bChangeOnly)
{
	if (!g_options.strPath.empty()) // если путь задали
	{
		if (g_options.strPath.is_relative()) // если задан относительный путь
		{
			g_options.strPath = g_options.strAppPath / g_options.strPath; // делаем абсолютный
		}

		if (!bChangeOnly)
		{
			if (!fs::exists(g_options.strPath))
			{
				std::error_code ec;

				if (!fs::create_directory(g_options.strPath, ec))
				{
					wprintf(L"Не удалось создать директорию '%s' :", g_options.strPath.c_str());
					printf("%s \n", ec.message().c_str());
				}
			}
		}

		std::error_code ec;
		fs::current_path(g_options.strPath, ec);

		if (ec)
		{
			wprintf(L"Не удалось найти директорию %s\n", g_options.strPath.c_str());
		}
	}
}

// запись пустых секторов от текущего места до конца дорожки.
// либо, если указано количество nWriteNum - заданное кол-во от текущего места
bool BKHDDTool::WriteEmptySectors(FILE *f, IMGFormat *imgf, int nWriteNum)
{
	bool bRet = true;
	USector buf{};
	memset(buf.b, 0377, SECTOR_SIZEB);
	int nStart, nStop;

	if (nWriteNum == 0)
	{
		nStart = imgf->nSector;

		if (nStart == 0) // если и так на начале дорожки, то ничего делать не надо.
		{
			return true;
		}

		nStop = imgf->S;
	}
	else
	{
		nStart = 0;
		nStop = nWriteNum;
	}

	for (int i = nStart; i < nStop; ++i)
	{
		if ((fwrite(buf.b, 1, SECTOR_SIZEB, f)) != SECTOR_SIZEB)
		{
			imgf->nIOStatus = IMGIOSTATUS::IO_ERROR;
			wprintf(L"Ошибка записи в файл '%s'\n", g_options.strImgName.c_str());
			return false;
		}

		if (++imgf->nSector == imgf->S)
		{
			imgf->nSector = 0;

			if (++imgf->nHead == imgf->H)
			{
				imgf->nHead = 0;

				if (++imgf->nCyl > imgf->C)
				{
					// wprintf(L"Переполнение образа диска\n");
					imgf->C = imgf->nCyl;
					bRet = false;
				}
			}
		}
	}

	return bRet;
}

// если задано достаточно большое кол-во дорожек, и за концом разделов есть место
// то надо забить его нулями
void BKHDDTool::WriteEmptyTail(FILE *f, IMGFormat *imgf)
{
	USector buf{};
	memset(buf.b, 0377, SECTOR_SIZEB);

	while (true)
	{
		if ((fwrite(buf.b, 1, SECTOR_SIZEB, f)) != SECTOR_SIZEB)
		{
			imgf->nIOStatus = IMGIOSTATUS::IO_ERROR;
			wprintf(L"Ошибка записи в файл '%s'\n", g_options.strImgName.c_str());
			break;
		}

		if (++imgf->nSector == imgf->S)
		{
			imgf->nSector = 0;

			if (++imgf->nHead == imgf->H)
			{
				imgf->nHead = 0;

				if (++imgf->nCyl > imgf->C)
				{
					break;
				}
			}
		}
	}
}


bool BKHDDTool::AssemblyAltPro(FILE *f, IMGFormat *imgf)
{
	bool bRet = false;
	// в g_sector будем формировать таблицу разделов.
	memset(g_sector.b, 0, SECTOR_SIZEB);
	imgf->nCyl = 0;
	imgf->nHead = 0;
	imgf->nSector = 0;
	imgf->nLogDiskNum = 0;
	uint16_t boot = 2;
	WriteEmptySectors(f, imgf, imgf->S); // запишем пустую нулевую дорожку
	std::vector <fs::path> aFileList;
	std::error_code ec;

	for (auto const &it : fs::directory_iterator(g_options.strPath, fs::directory_options::skip_permission_denied | fs::directory_options::follow_directory_symlink, ec))
	{
		fs::path file = it.path(); // имя найденного файла

		if (fs::is_regular_file(file))
		{
			std::wstring pExt = file.extension().wstring();

			for (const auto &pstrExt : g_pstrExts)
			{
				if (wcscoll(pstrExt.c_str(), pExt.c_str()) == 0)
				{
					aFileList.emplace_back(file);
					break;
				}
			}
		}
	}

	if (!aFileList.empty())
	{
		// отсортируем найденное в порядке возрастания
		std::sort(aFileList.begin(), aFileList.end());

		// затем по порядку всё это и обработаем
		for (auto &file : aFileList)
		{
			uint32_t beginLba = (imgf->nCyl * imgf->H + imgf->nHead) * imgf->S;
			g_sector.w[AHDD_PART_W - imgf->nLogDiskNum * 2] = (imgf->nHead & 0xf) | (imgf->nCyl << 4);
			wprintf(L"Раздел: %03d, файл: %s\n", imgf->nLogDiskNum, file.filename().make_preferred().c_str());

			if (imgf->nCyl >= 04000)
			{
				wprintf(L"Ошибка! Превышение допустимого номера цилиндра!\n");
				break;
			}

			bRet = ReadLogDiskImage(f, imgf, file);
			bRet = WriteEmptySectors(f, imgf);
			uint32_t endLba = (imgf->nCyl * imgf->H + imgf->nHead) * imgf->S;
			uint32_t len = endLba - beginLba;
			// размер высчитывается вместе с пустым пространством, выравнивающим разделы по началу дорожки
			// хотя никто не мешает задавать реальный размер в блоках, но в оригинале почему-то не так
			g_sector.w[AHDD_PART_W - 1 - imgf->nLogDiskNum * 2] = len;
			imgf->nLogDiskNum++;
		}

		aFileList.clear();
	}

	if (bRet)
	{
		WriteEmptyTail(f, imgf);
		g_sector.w[AHDD_CYL_W] = imgf->C;
		g_sector.w[AHDD_HEAD_W] = imgf->H & 0xff; // нужен только младший байт
		g_sector.w[AHDD_SEC_W] = imgf->S;
		g_sector.w[AHDD_LOGD_W] = (imgf->nLogDiskNum & 0xffff) | (boot << 8);
		uint16_t crc = 012701;

		// посчитаем контрольную сумму
		for (int i = 0; i < (imgf->nLogDiskNum * 2 + 4); ++i)
		{
			crc += g_sector.w[AHDD_CYL_W - i];
		}

		g_sector.w[AHDD_PART_W - imgf->nLogDiskNum * 2] = crc;
		fseek(f, (AHDD_PT_SEC + (imgf->bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET);
		HDIStuff::InverseSector(g_sector);

		if ((fwrite(g_sector.b, 1, SECTOR_SIZEB, f)) != SECTOR_SIZEB)
		{
			imgf->nIOStatus = IMGIOSTATUS::IO_ERROR;
			wprintf(L"Ошибка записи в файл '%s' \n", g_options.strImgName.c_str());
			bRet = false;
		}

		fseek(f, (imgf->bHDI ? SECTOR_SIZEB : 0), SEEK_SET);
		memcpy(g_sector.b, g_MBRAltPro, SECTOR_SIZEB);
		HDIStuff::InverseSector(g_sector);

		if ((fwrite(g_sector.b, 1, SECTOR_SIZEB, f)) != SECTOR_SIZEB)
		{
			imgf->nIOStatus = IMGIOSTATUS::IO_ERROR;
			wprintf(L"Ошибка записи в файл '%s' \n", g_options.strImgName.c_str());
			bRet = false;
		}
	}

	return bRet;
}

bool BKHDDTool::AssemblySamara(FILE *f, IMGFormat *imgf)
{
	bool bRet = false;
	USector buf{};
	// в g_sector будем формировать таблицу разделов.
	memset(g_sector.b, 0, SECTOR_SIZEB);
	imgf->nCyl = 0;
	imgf->nHead = 0;
	imgf->nSector = 0;
	imgf->nLogDiskNum = 0;
	uint16_t boot = 2;
	uint32_t lba = 3;
	WriteEmptySectors(f, imgf, lba); // сперва запишем 3 пустых сектора.
	std::vector <fs::path> aFileList;
	std::error_code ec;

	for (auto const &it : fs::directory_iterator(g_options.strPath, fs::directory_options::skip_permission_denied | fs::directory_options::follow_directory_symlink, ec))
	{
		fs::path file = it.path(); // имя найденного файла

		if (fs::is_regular_file(file))
		{
			std::wstring pExt = file.extension().wstring();

			for (const auto &pstrExt : g_pstrExts)
			{
				if (wcscoll(pstrExt.c_str(), pExt.c_str()) == 0)
				{
					aFileList.emplace_back(file);
					break;
				}
			}
		}
	}

	if (!aFileList.empty())
	{
		// отсортируем найденное в порядке возрастания
		std::sort(aFileList.begin(), aFileList.end());

		// затем по порядку всё это и обработаем
		for (auto &file : aFileList)
		{
			long nBlkLen = fs::file_size(file);
			// сперва приходится высчитать будущий размер тут
			// а потом то же самое, в  функции ReadLogDiskImage - нерационально
			nBlkLen = (nBlkLen + (SECTOR_SIZEB - 1)) / SECTOR_SIZEB; // размер файла в блоках

			if (nBlkLen < 12)
			{
				wprintf(L"ВНИМАНИЕ: размер образа '%s' меньше 12 блоков (%d).\nПропускаем такой образ.\n", file.filename().make_preferred().c_str(), nBlkLen);
				continue;
			}

			if (nBlkLen > 0xffff)
			{
				wprintf(L"ВНИМАНИЕ: размер образа '%s' превышает 65535 блоков (%d).\nПропускаем такой образ.\n", file.filename().make_preferred().c_str(), nBlkLen);
				continue;
			}

			// размер раздела не может быть меньше 1600 блоков. это просто нерационально.
			// !!! может-может. В ксидос может.
			uint16_t len = nBlkLen & 0xffff;
			// сперва надо создать начальный сектор
			memset(buf.b, 0, SECTOR_SIZEB);
			buf.w[SHDD_NLD_W] = imgf->nLogDiskNum + 2;      // номер ld
			buf.w[SHDD_LD_LEN_W] = len;     // длина ld
			buf.w[SHDD_LD_FLAGS_W] = 0;     // флаги - все нули
			buf.w[SHDD_ADR_BOOT_W] = 02000; // адрес загрузки загрузчика :)
			buf.w[SHHD_ADR_PAR_W] = 01000;  // адрес блока параметров для загрузчика
			buf.w[SHDD_PAGE_W] = 014400;    // состояние регистра страниц
			g_sector.w[SHDD_PART_W + imgf->nLogDiskNum * 2] = lba & 0xffff;
			g_sector.w[SHDD_PART_W + 1 + imgf->nLogDiskNum * 2] = (lba >> 16) & 0xffff;
			HDIStuff::InverseSector(buf);

			if ((fwrite(buf.b, 1, SECTOR_SIZEB, f)) != SECTOR_SIZEB)
			{
				imgf->nIOStatus = IMGIOSTATUS::IO_ERROR;
				wprintf(L"Ошибка записи в файл '%s' \n", g_options.strImgName.c_str());
				bRet = false;
			}

			wprintf(L"Раздел: %03d, файл: %s\n", imgf->nLogDiskNum, file.filename().make_preferred().c_str());
			bRet = ReadLogDiskImage(f, imgf, file);
			lba += len + 1;
			imgf->nLogDiskNum++;
		}

		aFileList.clear();
	}

	if (bRet)
	{
		WriteEmptyTail(f, imgf);
		g_sector.w[SHDD_BOOT_W] = boot; // boot
		g_sector.w[SHDD_CYLVOL_W] = imgf->H * imgf->S;
		g_sector.b[SHDD_HEAD_B] = (imgf->H & 0xff) - 1;
		g_sector.b[SHDD_SEC_B] = (imgf->S & 0xff);
		fseek(f, (SHDD_PT_SEC + (imgf->bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET);
		HDIStuff::InverseSector(g_sector);

		if ((fwrite(g_sector.b, 1, SECTOR_SIZEB, f)) != SECTOR_SIZEB)
		{
			imgf->nIOStatus = IMGIOSTATUS::IO_ERROR;
			wprintf(L"Ошибка записи в файл '%s' \n", g_options.strImgName.c_str());
			bRet = false;
		}
	}

	return bRet;
}

void BKHDDTool::Usage()
{
	wprintf(L"Программа для работы с образами винчестера БК0010-11\n" \
	        L"(c) 2010 Terra software, 2014-2023 gid\n" \
	        L"\n" \
	        L"Использование:\n" \
	        L"BKHDDtool -? (--help)\n" \
	        L"  Вывод этой справки.\n" \
	        L"\n" \
	        L"BKHDDtool [-a | -m][-i][-o][-p<Путь>][-c<N> -h<N> -s<N>] <Имя образа HDD>\n" \
	        L"  -a (--altpro) - использовать формат образа диска 'АльтПро'\n" \
	        L"  -m (--samara) - использовать формат образа диска 'Самара'\n" \
	        L"    Если не задана ни одна опция, по умолчанию используется формат 'АльтПро'.\n" \
	        L"  -i (--hdi) - создавать образ в формате hdi. При разборке ключ игнорируется.\n" \
	        L"    При преобразовании ключ игнорируется.\n" \
	        L"  -o (--convert) - преобразование образа из одного формата в другой.\n" \
	        L"    Если ключ задан - преобразование, при этом ключ -p игнорируется, если\n" \
	        L"    не задан - разборка.\n" \
	        L"  -p<Путь> (--path <Путь>) - директория, где искать образы дискет для\n" \
	        L"    сборки образа винчестера, или куда сохранять образы дискет при разборке\n" \
	        L"    образа винчестера.\n" \
	        L"  -c<N> (--cylinder <N>) - количество цилиндров (дорожек)\n" \
	        L"  -h<N> (--head <N>) - количество головок\n" \
	        L"  -s<N> (--sector <N>) - количество секторов на дорожке\n" \
	        L"    Если хоть один из этих ключей задан, то считается, что выбран режим сборки\n" \
	        L"    образа, при этом ключ -o игнорируется, ключи -a или -m задают формат\n" \
	        L"    образа.\n" \
	        L"  <Имя образа HDD> - возможно задать с полным путём к файлу.\n" \
	        L"\n" \
	        L"Программа может работать в трёх режимах:\n" \
	        L"1. Разборка образа на логические диски.\n" \
	        L"BKHDDtool [-a | -m][-p<Путь>] <Имя образа HDD>\n" \
	        L"  Задавать формат не обязательно, он определяется автоматически. Но если\n" \
	        L"формат не опознан, то образ будет обрабатываться в заданном формате.\n" \
	        L"АльтПро - формат по умолчанию, если ключи не заданы\n" \
	        L"\n" \
	        L"2. Сборка образов из логических дисков.\n" \
	        L"BKHDDtool [-a | -m][-i][-p<Путь>][-c<N> -h<N> -s<N>] <Имя образа HDD>\n" \
	        L"  Если задан хоть один ключ, определяющий геометрию, то это означает создание\n" \
	        L"образа винчестера из образов дискет.\n" \
	        L"\n" \
	        L"3. Преобразование образов из одного формата в другой.\n" \
	        L"BKHDDtool [-a | -m] -o <Имя образа HDD>\n" \
	        L"  Производится преобразование формата образа 'АльтПро'<->'Самара'\n" \
	        L"Формат образа определяется автоматически, и в соответствии с этим преобразуется.\n" \
	        L"\n" \
	        L"  Если ключ -p не задан, то используется та же директория, где находится\n" \
	        L"образ.\n" \
	        L"  При разборке создаются файлы nnn_part.bkd, где nnn - номера разделов в том\n" \
	        L"порядке, в котором они находились в образе винчестера.\n" \
	        L"  При сборке образы разделов сортируются в алфавитном порядке, и в таком же\n" \
	        L"порядке размещаются в образе винчестера.\n" \
	        L"\n" \
	        L"После создания образ винчестера можно записать на винчестер или карту памяти\n" \
	        L"с помощью бесплатной утилиты dd -http://www.chrysocome.net/dd\n");
}

/*
Структура диска альтпро.
Блоки 0..6 (секторы 1..7) - резидентные модули и т.п.
Блок 7 (Сектор 8) - таблица разделов и разные полезные данные.
формат сектора 8:
имя      | смещение | размер | назначение
------------------------------------------------
HD$SPD   | 0        | 2      | признак быстрого чтения (125252)
         | 2        | 766    | область таблицы разделов (помещается 125. лог дисков и
         |          |        | 1 слово - контрольная сумма, вместе с геометрией)
HD$LOG   | 770      | 1      | всего лог.дисков
HD$UNI   | 771      | 1      | # устр. для загрузки по умолч. (0 - А, 2 - С ...)
HD$SEC   | 772      | 2      | секторов на дорожке
HD$HEA   | 774      | 1      | всего головок
HD$DRV   | 775      | 1      | код # привода,уст.внешним драйвером
HD$TRK   | 776      | 2      | всего дорожек

таблица разделов как стек растёт с верхних адресов к нижним.
Записи о логических дисках идут по порядку, начиная с логического диска с номером 0.
начинается с 766, заканчивается 2
Формат записи следующий (отсчёт естественно в направлении от HD$LOG-2 к HD$SPD):
1 слово - номер головки (младшие 4 бита) и номер дорожки (остальные 12 бит)
          ограничение: макс. 4096 из 65536 возможных.
  если это слово < 0 (т.е. на дорожки отводится уже 11 бит.)
          ограничение стало: макс. 2048 из 65536 возможных.
  то раздел считается защищённым от записи.
  это слово не просто <0, оно всё целиком инвертировано!
  разделы выравниваются по началу цилиндра, т.е. начало всегда сектор 1.

2 слово - объём логического диска в блоках, т.е. 65536 = макс. 32Мб.
(предполагается, что каждый лог. диск начинается с первого сектора дорожки, определённой первым словом).

--------------------------------------------------------------------------
Структура диска самара.
первые три блока - МБР
Блок 0 (сектор 1) - не используется, зарезервировано
Блок 1 (сектор 2) - геометрия и таблица разделов
Блок 2 (сектор 3) - не используется, зарезервировано

Блок1
смещение | размер | назначение
------------------------------------------------
0        | 2      | # устр. для загрузки по умолч. (0 - А, 2 - С ...)
2        | 2      | объём цилиндра (общее количество секторов на дорожке) == H * S
4        | 1      | количество секторов на дорожке
5        | 1      | номер последней головки (H - 1)
6        | x      | таблица разделов, максимум 64 раздела (пока будем считать что это максимум)

формат элемента таблицы разделов:
смещение | размер | назначение
------------------------------------------------
0        | 4      | номер логического блока начала раздела (в реальности используется 20 битное слово)


каждый логический диск начинается с начального блока
формат:
смещение | размер | назначение
------------------------------------------------
0        | 1      | номер лог. диска (0 - А, 2 - С ...)
1        | 1      | ? не используется.
2        | 2      | размер лог. диска в блоках
4        | 1      | флаги - признаки
5        | 1      | ? не используется.
6        | 2      | адрес загрузки загрузчика лог. диска
10       | 2      | адрес блока параметров для загрузчика
12       | 2      | состояние регистра страниц

; ФОРМАТ БАЙТА ПРИЗНАКОВ ДИСКА
;1-битые сектора
;2-защ от записи
;4-защ от чтения
;10-полн.защ от записи
;20-полн.защ от чтения
;40
;100
;200

*/

