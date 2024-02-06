#include "pch.h"
#include "BKPack.h"
#pragma warning(disable:4996)


int g_nTableOffset = 0;
/*
Файл загружен, нужен его анализ

nFileLoadAddr - адрес загрузки файла в массив MMemory, предполагается 0
nFileLen - размер файла, загруженного в массив MMemory
nFileActualAddr - адрес загрузки файла из каталога, обычно это может браться из бин заголовка.
*/
bool PackAnalyse(int nFileLoadAddr, int nFileLen, int nFileActualAddr)
{
	// нужно узнать, есть ли автозапуск.
	int nOffset = 01000 - nFileActualAddr; // если адрес загрузки < 01000, то там может быть автозапуск
	BKPkParam.LENAZ = 0;
	BKPkParam.BDAT = nFileLoadAddr;

	if (nOffset > 0)
	{
		BKPkParam.LENAZ = nOffset; // размер блока автозапуска
		BKPkParam.BDAT += nOffset; // его не трогаем
	}

	// какой тип автораспаковщика
	BKPkParam.DSHORT = 022; // прибавка к длине автораспаковщика

	if (MMemory.w[BKPkParam.BDAT / 2 + 3] == 0266 && MMemory.w[BKPkParam.BDAT / 2 + 5] == 0350)
	{
		BKPkParam.bLongUnpacker = true;
		BKPkParam.DSHORT = 0;
	}

	bool bOk = true;
	// нужно определить, а это вообще bkpack или нет?
	int nBase = BKPkParam.BDAT - BKPkParam.DSHORT;
	// есть 2 достаточно гарантированных участка для сравнения
	int wBase = nBase / 2; // начало проги в словах
	int k = 0130 / 2; // первый участок для сравнения
	bool bNoInt1 = false;

	for (auto n = 0; n < 4; ++n) // сравним 4 слова
	{
		if (MMemory.w[wBase + k] != BKPackAUNP[k]) // если там не то, что ожидаем
		{
			bNoInt1 = true; // то это не БКпак
		}

		k++;
	}

	k += 11; // переходим ко второму интервалу
	bool bNoInt2 = false;

	for (auto n = 0; n < 12; ++n) // сравним 12 слова
	{
		if (MMemory.w[wBase + k] != BKPackAUNP[k]) // если там не то, что ожидаем
		{
			bNoInt2 = true; // то это не БКпак
		}

		k++;
	}

	if (bNoInt1 && bNoInt2) // если хотя бы один интервал совпал, то это скорее всего бкпак
	{
		return false;
	}

	// попали сюда, значит это БКпак
	// извлечь из автораспаковщика нужные данные
	if (MMemory.w[(nBase + EOFS$) / 2] == 0207)
	{
		BKPkParam.bData = true;
	}

	// если точка перехода модифицирована
	if (MMemory.w[(nBase + EOFS$) / 2] == 0137 || MMemory.w[(nBase + EOFS$) / 2] == 04737)
	{
		BKPkParam.AWREG = 0137; // флаг того, что в EOFS2$ хранится не смещение, а реальный адрес
	}

	BKPkParam.C1 = MMemory.w[(nBase + C1$) / 2];
	BKPkParam.C2 = MMemory.w[(nBase + C2$) / 2];
	BKPkParam.DELW = MMemory.w[(nBase + DELW1$) / 2] - 1;
	BKPkParam.LENAUN = MMemory.w[(nBase + LAUN$) / 2]; // длина автораспаковщика с таблицами
	g_nTableOffset = MMemory.w[(nBase + 026) / 2] + nBase + 024;
	BKPkParam.EPK = nFileLoadAddr + nFileLen; // это конечный адрес запакованной проги
	BKPkParam.nFileLen = MMemory.w[(nBase + LUNP$) / 2]; // это длина распакованной проги
	BKPkParam.DELJMP = MMemory.w[(nBase + EOFS2$) / 2]; // смещение для запуска
	return true;
}

// распаковываем таблицу TWB.
uint16_t ExpandTW()
{
	memset(TW, 0, sizeof(TW));
	uint16_t R4 = g_nTableOffset; // начало TWB
	uint16_t R0 = R4; // конец SB
	int R3 = LENMAS_W;
	uint16_t R1 = 0;

	do
	{
		R0 -= 2;
		uint16_t nBit = 1 << 15;

		for (int R2 = 0; R2 < 020; ++R2)
		{
			TW[--R3] = 0;

			if (MMemory.w[R0 / 2] & nBit)
			{
				auto R5 = short(char(MMemory.b[R4++]));
				R5 -= BKPkParam.C1;

				if (R5 >= 0)
				{
					R5 <<= 8;
					R5 |= MMemory.b[R4++];
					R5 -= BKPkParam.C2;
				}

				R1 -= R5;
				TW[R3] = R1;
			}

			nBit >>= 1;
		}
	}
	while (R3 > 0);

#if defined(_DEBUG) && defined(DBG_LOG)
	FILE *tf = fopen("TW_unp.txt", "wt");
	int n = 0;
	int r = 037000;

	for (int i = 0; i < LENMAS_W / 4; ++i)
	{
		fprintf(tf, "%06o: %06o ", r, TW[n++]);
		fprintf(tf, "%06o ", TW[n++]);
		fprintf(tf, "%06o ", TW[n++]);
		fprintf(tf, "%06o\n", TW[n++]);
		r += 010;
	}

	fclose(tf);
#endif
	return R0;
}

void BKUnpack()
{
	// сперва восстановим целостность массива
	int R0 = BKPkParam.EPK;
	int R2 = BKPkParam.LENAUN; // размер автораспаковщика с таблицами
	int R4 = BKPkParam.BDAT + R2;

	for (int i = 0; i < R2; ++i)
	{
		MMemory.b[--R4] = MMemory.b[--R0];
	}

	// теперь распаковываем
	R4 += BKPkParam.nFileLen;
	uint16_t R1 = 0;

	do
	{
		assert(R0 > 0);
		short i = short(char(MMemory.b[--R0])) + LENMAS_W / 2;

		if (0 <= i && i < LENMAS_W)
		{
			R1 = TW[i];
		}
		else
		{
			assert(false);
		}

		if (R1 == 0)
		{
			MMemory.b[--R4] = MMemory.b[R0];
			MMemory.b[--R4] = MMemory.b[--R0];
		}
		else if (R1 == 1)
		{
			int R2 = MMemory.b[--R0];

			if (R2 == 0)
			{
				R2 = 0400;
			}

			for (int i = 0; i < R2; ++i)
			{
				uint16_t w = MMemory.w[R4 / 2];
				R4 -= 2;
				MMemory.w[R4 / 2] = w;
			}
		}
		else if (R1 == 0100000)
		{
			MMemory.b[--R4] = MMemory.b[--R0];
			MMemory.b[--R4] = MMemory.b[--R0];
		}
		else
		{
			R1 += BKPkParam.DELW;
			R4 -= 2;
			MMemory.w[R4 / 2] = R1;
		}
	}
	while (R4 != R0);

	if (BKPkParam.AWREG != 0137)
	{
		BKPkParam.DELJMP += R0 + g_nFileAddress; // Вот адрес запуска.
	}

	// теперь, если есть блок автозапуска, то надо восстановить его как было
	if (BKPkParam.LENAZ && !BKPkParam.bData)
	{
		int R0 = BKPkParam.BDAT - 012; // адрес в памяти ячейки 766

		if (R0 < g_nFileLoadAddress)
		{
			R0 = g_nFileLoadAddress; // если перескочили за начало - скорректируем
		}

		uint16_t wStart = MMemory.w[R0 / 2]; // предполагаем, что это текущий адрес автозапуска
		R0 = BKPkParam.BDAT - 034; // адрес в памяти ячейки 744

		if (R0 < g_nFileLoadAddress)
		{
			R0 = g_nFileLoadAddress; // если перескочили за начало - скорректируем
		}

		// находим конец массива автозапуска
		while (MMemory.w[R0 / 2] == wStart && R0 < BKPkParam.BDAT)
		{
			R0 += 2;
		}

		R0 -= 2;

		if (R0 < g_nFileLoadAddress)
		{
			R0 = g_nFileLoadAddress; // если перескочили за начало - скорректируем
		}

		do
		{
			MMemory.w[R0 / 2] = BKPkParam.DELJMP; // меняем адрес пуска на тот, что был в оригинале
			R0 -= 2;
		}
		while (R0 >= g_nFileLoadAddress && MMemory.w[R0 / 2] == wStart);   // во всех словах, совпадающих с исходным
	}
}


bool BKUnPacking(int nFileLoadAddr, int nFileLen, int nFileActualAddr)
{
	if (PackAnalyse(nFileLoadAddr, nFileLen, nFileActualAddr))
	{
		uint16_t R0 = ExpandTW();
		R0 -= 034 * 2; // магические числа. весь расчёт, что блок автораспаковки не будет меняться
		R0 += MMemory.w[(BKPkParam.BDAT - BKPkParam.DSHORT + 0122) / 2]; // определим реальный размер массива, а не тот что прочитали

		if (R0 > BKPkParam.EPK)
		{
			wprintf(L"Ошибка:: оборван конец файла. Массив повреждён.\n");
			return false;
		}

		BKPkParam.EPK = R0; // пофиксим реальный конец массива
		BKUnpack();
		wprintf(L" OK\n");
		return true;
	}

	wprintf(L"Ошибка:: не опознан упаковщик BKPack или файл повреждён.\n");
	return false;
}

