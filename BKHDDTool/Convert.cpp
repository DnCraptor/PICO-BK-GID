#include "pch.h"

#include "BKHDDTool.h"
#include "HDIStuff.h"

extern USector g_sector;
extern ProrgamOptions g_options;
extern uint16_t g_MBRAltPro[SECTOR_SIZEW];

// конвертируем образ альтпро в самару
// Вход: f - входной файл образа
//  imgf - его формат и характеристики.
bool BKHDDTool::ConvertAltPro2Samara(FILE *f, IMGFormat *InImgF)
{
	bool bRet = false;
	int nErrors = 0;
	USector buf{}, block1{};
	memset(block1.b, 0, SECTOR_SIZEB);

	// прочитаем сектор 8, нам нужна таблица разделов.
	if ((fseek(f, (AHDD_PT_SEC + (InImgF->bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET) == 0)
	        && (fread(g_sector.b, 1, SECTOR_SIZEB, f) == SECTOR_SIZEB))
	{
		HDIStuff::InverseSector(g_sector);
		//////////////////////////////////
		IMGFormat OutImgF = *InImgF;
		OutImgF.bSamara = true;
		OutImgF.nCyl = 0;
		OutImgF.nHead = 0;
		OutImgF.nSector = 0;
		OutImgF.nLogDiskNum = 0;
		uint16_t Outboot = g_sector.b[AHDD_UNI_B];
		uint32_t Outlba = 3;
		fs::path title = g_options.strImgName.stem();
		title += L".samara";
		title += g_options.strImgName.extension();
		fs::path strOutImgName = g_options.strImgName;
		strOutImgName.replace_filename(title);
		FILE *OutF = _wfopen(strOutImgName.c_str(), L"wb");

		if (OutF == nullptr)
		{
			wprintf(L"Не удалось создать файл %s\n", strOutImgName.c_str());
			return false;
		}

		if (OutImgF.bHDI)
		{
			// создаём заголовочный сектор HDI.
			HDIStuff::CreateHDISector(&OutImgF, reinterpret_cast<SYS_SECTOR *>(buf.b), HDIStuff::GenerateSerialNumber(), g_HDIModelName);

			if (SECTOR_SIZEB != fwrite(buf.b, 1, SECTOR_SIZEB, OutF))
			{
				wprintf(L"Не удалось записать HDI сектор в файл %s\n", strOutImgName.c_str());
			}
		}

		WriteEmptySectors(OutF, InImgF, Outlba); // сперва запишем 3 пустых сектора.
		//////////////////////////////////

		for (int nld = 0; nld < InImgF->nLogDiskNum; ++nld)
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
			uint32_t lba = (nCyl * InImgF->H + nHead) * InImgF->S; // рассчитываем начало раздела в блоках
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

			if (fseek(f, (lba + (InImgF->bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET) == 0)
			{
				//////////////////////////////////
				// сперва надо создать начальный сектор
				uint16_t OutLen = nLen & 0xffff;
				memset(buf.b, 0, SECTOR_SIZEB);
				buf.w[SHDD_NLD_W] = OutImgF.nLogDiskNum + 2;      // номер ld
				buf.w[SHDD_LD_LEN_W] = OutLen;  // длина ld
				buf.w[SHDD_LD_FLAGS_W] = bProtected ? ALD_ATTR::WP : ALD_ATTR::NONE; // флаги - все нули
				buf.w[SHDD_ADR_BOOT_W] = 02000; // адрес загрузки загрузчика :)
				buf.w[SHHD_ADR_PAR_W] = 01000;  // адрес блока параметров для загрузчика
				buf.w[SHDD_PAGE_W] = 014400;    // состояние регистра страниц
				block1.w[SHDD_PART_W + OutImgF.nLogDiskNum * 2] = Outlba & 0xffff;
				block1.w[SHDD_PART_W + 1 + OutImgF.nLogDiskNum * 2] = (Outlba >> 16) & 0xffff;
				HDIStuff::InverseSector(buf);

				if ((fwrite(buf.b, 1, SECTOR_SIZEB, OutF)) != SECTOR_SIZEB)
				{
					wprintf(L"Ошибка записи в файл %s \n", strOutImgName.c_str());
					return false;
				}

				//////////////////////////////////

				if (!(bRet = ConvertLogDiskImage(f, OutF, &OutImgF, OutLen)))
				{
					wprintf(L"\nОшибка извлечения раздела %d\n", nld);
					nErrors++;
				}

				//////////////////////////////////
				Outlba += OutLen + 1;
				OutImgF.nLogDiskNum++;
				//////////////////////////////////
			}
			else
			{
				wprintf(L"\nОшибка перемещения к разделу %d\n", nld);
				nErrors++;
			}
		}

		//////////////////////////////////
		if (bRet)
		{
			WriteEmptyTail(OutF, &OutImgF);
			block1.w[SHDD_BOOT_W] = Outboot; // boot
			block1.w[SHDD_CYLVOL_W] = OutImgF.H * OutImgF.S;
			block1.b[SHDD_HEAD_B] = (OutImgF.H & 0xff) - 1;
			block1.b[SHDD_SEC_B] = (OutImgF.S & 0xff);
			fseek(OutF, (SHDD_PT_SEC + (OutImgF.bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET);
			HDIStuff::InverseSector(block1);

			if ((fwrite(block1.b, 1, SECTOR_SIZEB, OutF)) != SECTOR_SIZEB)
			{
				wprintf(L"Ошибка записи в файл %s \n", strOutImgName.c_str());
				bRet = false;
			}
		}

		//////////////////////////////////
		wprintf(L"\nУспешно сконвертировано %d разделов\n", InImgF->nLogDiskNum - nErrors);
		fclose(OutF);

		if (nErrors)
		{
			wprintf(L"С ошибками - %d разделов\n", nErrors);
		}
	}
	else
	{
		wprintf(L"Ошибка чтения таблицы разделов формата 'АльтПро'\n");
	}

	return bRet;
}


// конвертируем образ самара в альтпро
// Вход: f - входной файл образа
//  imgf - его формат и характеристики.
bool BKHDDTool::ConvertSamara2Altpro(FILE *f, IMGFormat *InImgF)
{
	bool bRet = false;
	int nErrors = 0;
	USector buf{}, block1{};
	memset(block1.b, 0, SECTOR_SIZEB);

	// прочитаем сектор 2, нам нужна таблица разделов.
	if ((fseek(f, (SHDD_PT_SEC + (InImgF->bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET) == 0)
	        && (fread(g_sector.b, 1, SECTOR_SIZEB, f) == SECTOR_SIZEB))
	{
		HDIStuff::InverseSector(g_sector);
		//////////////////////////////////
		IMGFormat OutImgF = *InImgF;
		OutImgF.bSamara = false;
		OutImgF.nCyl = 0;
		OutImgF.nHead = 0;
		OutImgF.nSector = 0;
		OutImgF.nLogDiskNum = 0;
		// в block1 будем формировать таблицу разделов.
		memset(block1.b, 0, SECTOR_SIZEB);
		uint16_t boot = g_sector.w[SHDD_BOOT_W];
		fs::path title = g_options.strImgName.stem();
		title += L".altpro";
		title += g_options.strImgName.extension();
		fs::path strOutImgName = g_options.strImgName;
		strOutImgName.replace_filename(title);
		FILE *OutF = _wfopen(strOutImgName.c_str(), L"wb");

		if (OutF == nullptr)
		{
			wprintf(L"Не удалось создать файл %s\n", strOutImgName.c_str());
			return false;
		}

		if (OutImgF.bHDI)
		{
			// создаём заголовочный сектор HDI.
			HDIStuff::CreateHDISector(&OutImgF, reinterpret_cast<SYS_SECTOR *>(buf.b), HDIStuff::GenerateSerialNumber(), g_HDIModelName);

			if (SECTOR_SIZEB != fwrite(buf.b, 1, SECTOR_SIZEB, OutF))
			{
				wprintf(L"Не удалось записать HDI сектор в файл %s\n", strOutImgName.c_str());
			}
		}

		WriteEmptySectors(OutF, &OutImgF, OutImgF.S); // запишем пустую нулевую дорожку
		//////////////////////////////////

		for (int nld = 0; nld < InImgF->nLogDiskNum; ++nld)
		{
			uint32_t lba = ((uint32_t(g_sector.w[SHDD_PART_W + 1 + nld * 2]) << 16) | uint32_t(g_sector.w[SHDD_PART_W + nld * 2]));

			// перемещаемся к началу лог диска
			if (fseek(f, (lba + (InImgF->bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET) == 0)
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

					bool bProtect = false;

					if (attr & ALD_ATTR::WP)
					{
						wprintf(L"WP ");
						bProtect = true;
					}

					if (attr & ALD_ATTR::RP)
					{
						wprintf(L"RP ");
					}

					if (attr & ALD_ATTR::FWP)
					{
						wprintf(L"FWP ");
						bProtect = true;
					}

					if (attr & ALD_ATTR::FRP)
					{
						wprintf(L"FRP ");
					}

					wprintf(L"]\n");

					//////////////////////////////////
					if (OutImgF.nCyl >= 04000)
					{
						wprintf(L"Ошибка! Превышение допустимого номера цилиндра!\n");
						nErrors++;
						break;
					}

					int beginLba = (OutImgF.nCyl * OutImgF.H + OutImgF.nHead) * OutImgF.S;
					uint16_t w1 = (OutImgF.nHead & 0xf) | (OutImgF.nCyl << 4);

					if (bProtect)
					{
						w1 = ~w1;
					}

					block1.w[AHDD_PART_W - OutImgF.nLogDiskNum * 2] = w1;

					//////////////////////////////////
					if (!(bRet = ConvertLogDiskImage(f, OutF, &OutImgF, nLen)))
					{
						wprintf(L"\nОшибка извлечения раздела %d\n", nld);
						nErrors++;
					}

					//////////////////////////////////
					bRet = WriteEmptySectors(OutF, &OutImgF);
					int endLba = (OutImgF.nCyl * OutImgF.H + OutImgF.nHead) * OutImgF.S;
					int len = endLba - beginLba;
					// размер высчитывается вместе с пустым пространством, выравнивающим разделы по началу дорожки
					// хотя никто не мешает задавать реальный размер в блоках, но в оригинале почему-то не так
					block1.w[AHDD_PART_W - 1 - OutImgF.nLogDiskNum * 2] = len;
					OutImgF.nLogDiskNum++;
					//////////////////////////////////
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

		//////////////////////////////////
		if (bRet)
		{
			WriteEmptyTail(OutF, &OutImgF);
			block1.w[AHDD_CYL_W] = OutImgF.C;
			block1.w[AHDD_HEAD_W] = OutImgF.H & 0xff; // нужен только младший байт
			block1.w[AHDD_SEC_W] = OutImgF.S;
			block1.w[AHDD_LOGD_W] = (OutImgF.nLogDiskNum & 0xffff) | (boot << 8);
			uint16_t crc = 012701;

			// посчитаем контрольную сумму
			for (int i = 0; i < (OutImgF.nLogDiskNum * 2 + 4); ++i)
			{
				crc += block1.w[AHDD_CYL_W - i];
			}

			block1.w[AHDD_PART_W - OutImgF.nLogDiskNum * 2] = crc;
			fseek(OutF, (AHDD_PT_SEC + (OutImgF.bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET);
			HDIStuff::InverseSector(block1);

			if ((fwrite(block1.b, 1, SECTOR_SIZEB, OutF)) != SECTOR_SIZEB)
			{
				wprintf(L"Ошибка записи в файл %s \n", strOutImgName.c_str());
				bRet = false;
			}

			fseek(OutF, ((OutImgF.bHDI) ? SECTOR_SIZEB : 0), SEEK_SET);
			memcpy(block1.b, g_MBRAltPro, SECTOR_SIZEB);
			HDIStuff::InverseSector(block1);

			if ((fwrite(block1.b, 1, SECTOR_SIZEB, OutF)) != SECTOR_SIZEB)
			{
				wprintf(L"Ошибка записи в файл %s \n", strOutImgName.c_str());
				bRet = false;
			}
		}

		//////////////////////////////////
		wprintf(L"\nУспешно сконвертировано %d разделов\n", InImgF->nLogDiskNum - nErrors);
		fclose(OutF);

		if (nErrors)
		{
			wprintf(L"С ошибками - %d разделов\n", nErrors);
		}
	}
	else
	{
		wprintf(L"Ошибка чтения таблицы разделов формата 'Самара'\n");
	}

	return bRet;
}


//
// inf - входной файл образа
// outf - выходной файл образа
// OutImgf - выходной формат
// nLenBlk - размер в блоках
bool BKHDDTool::ConvertLogDiskImage(FILE *inf, FILE *outf, IMGFormat *OutImgf, uint32_t nLenBlk)
{
	bool bRet = true;
	USector buf{};

	for (uint32_t i = 0; i < nLenBlk; ++i)
	{
		memset(buf.b, 0, SECTOR_SIZEB); // для начала очистим буфер. если размер файла будет не кратен блоку,
		// остаток сектора должен быть заполнен нулями.

		if (!feof(inf))
		{
			fread(buf.b, 1, SECTOR_SIZEB, inf); // читаем сколько сможем
		}

		if (ferror(inf))
		{
			wprintf(L"\nОшибка чтения из входного образа\n");
			bRet = false;
			break;
		}

		if (SECTOR_SIZEB != fwrite(buf.b, 1, SECTOR_SIZEB, outf))
		{
			wprintf(L"\nОшибка записи в выходной образ\n");
			bRet = false;
			break;
		}

		if (++OutImgf->nSector == OutImgf->S)
		{
			OutImgf->nSector = 0;

			if (++OutImgf->nHead == OutImgf->H)
			{
				OutImgf->nHead = 0;

				if (++OutImgf->nCyl > OutImgf->C)
				{
					// wprintf(L"Переполнение образа диска\n");
					OutImgf->C = OutImgf->nCyl; // корректируем размер в сторону увеличения.
					bRet = false;
				}
			}
		}
	}

	fflush(outf);
	return bRet;
}
