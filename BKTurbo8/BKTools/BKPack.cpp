#include "pch.h"
#include "BKPack.h"
#pragma warning(disable:4996)

BKPParam BKPkParam;

uint16_t TB[LENMAS_W];      // Таблица байтов
uint16_t TW[LENMAS_W];      // Таблица слов
uint8_t TWB[LENMAS];        // Упакованная таблица слов
uint8_t SB[SB_SIZE];        // Шкала байтов

// блок автораспаковки
uint16_t BKPackAUNP[] =
{
	0012704,
	0000000,
	0010437,
	0000266,
	0010437,
	0000350,
	0012737,
	0000000,
	0000312,
	0010704,    // 0
	0062704,
	0000236,
	0010400,
	0012703,    // 10
	0001000,
	0005001,
	0012702,
	0000020,    // 20
	0005740,
	0005043,
	0006110,
	0103012,    // 30
	0112405,
	0162705,
	0000000,
	0100404,    // 40
	0000305,
	0152405,
	0162705,
	0000000,    // 50
	0160501,
	0010113,
	0077216,
	0020327,    // 60
	0000000,
	0001354,
	0012702,
	0000034,    // 70
	0014043,
	0077202,
	0062700,
	0000130,    // 100
	0012702,
	0000000,
	0000113,
	0114044,    // 110
	0077202,
	0062704,
	0000000,
	0114001,    // 120
	0006301,
	0016101,
	0000400,
	0001410,    // 130
	0005301,
	0001411,
	0102404,
	0062701,    // 140
	0000000,
	0010144,
	0000410,
	0005300,    // 150
	0111044,
	0114044,
	0000760,
	0114002,    // 160
	0011444,
	0105302,
	0001375,
	0020400,    // 170
	0001352,
	0000207,
	0000000
};
// размер блока автораспаковки в байтах
#define LENAUNP (sizeof(BKPackAUNP))


#include <algorithm>

std::vector<uint8_t> mas2v[256];
std::vector<Pair> awd;       // вот такой массив
std::vector<Pair> dhib_v;    // вот такой массив

/*
Тут настраиваются начальные параметры

nFileLoadAddr - адрес загрузки файла в массив MMemory, предполагается 0
nFileLen - размер файла, загруженного в массив MMemory
nFileActualAddr - адрес загрузки файла из каталога, обычно это может браться из бин заголовка.
*/
bool prepareData(int nFileLoadAddr, int nFileLen, int nFileActualAddr)
{
	constexpr auto MAGIC_NUMBER = 01070;
	constexpr auto DEFAULT_LOAD_ADDR = 01000;
	int nEndAddr = nFileLoadAddr + nFileLen;

	if (nEndAddr & 1)
	{
		MMemory.b[nEndAddr++] = 0;
	}

	BKPkParam.BDAT = nFileLoadAddr;
	int nFileEvenLen = nEndAddr - nFileLoadAddr;

	if (nFileEvenLen > MAXLEN)
	{
		return false;
	}

	BKPkParam.LENW = nFileEvenLen / 2;
	BKPkParam.LENAZ = 0;
	int nOffset = DEFAULT_LOAD_ADDR - nFileActualAddr; // если адрес загрузки <01000, то там может быть автозапуск

	if (nOffset > 0)
	{
		BKPkParam.LENAZ = nOffset; // размер блока автозапуска
		BKPkParam.BDAT += nOffset; // его не пакуем
		BKPkParam.LENW -= nOffset / 2; // корректируем длину
	}

	// Вычислим подходящий адрес рабочей области
	int nActualEndAddr = nFileActualAddr + nFileLen;

	if (g_nWorkArea != -1)
	{
		// если задаём адрес рабочей области в командной строке, то у неё приоритет
		if ((g_nWorkArea == 0) || (nFileActualAddr < (g_nWorkArea - MAGIC_NUMBER) && g_nWorkArea < nActualEndAddr))
		{
			// если ошибочно рабочая область попадает на файл, ну мало ли, ошиблись при задании числа
			// то надо подкорректировать рабочую область
			g_nWorkArea = nActualEndAddr + MAGIC_NUMBER;
		}

		BKPkParam.AWREG = g_nWorkArea - MAGIC_NUMBER;
	}
	else
	{
		int nWorkArea = 040000 - MAGIC_NUMBER; // адрес рабочей области по умолчанию

		if (nActualEndAddr > nWorkArea)
		{
			// если не помещается перед экраном
			nWorkArea += 040000; // проверим конец памяти

			if (nActualEndAddr > nWorkArea)
			{
				nWorkArea -= 070000; // тогда перед 10000 заведомо
			}

			// !!! тут надо бы модифицировать для БК11М, чтобы в три страницы
			// могло распаковываться сразу, очень большой файл
		}

		BKPkParam.AWREG = nWorkArea;
	}

	BKPkParam.nFileLen = nFileLen;
	int nCrc = 0;

	// посчитать КС
	for (int i = 0; i < nFileLen; ++i)
	{
		nCrc += MMemory.b[nFileLoadAddr + i];

		if (nCrc & 0xFFFF0000)
		{
			// если случился перенос в 17 разряд (т.е. бит С для word)
			nCrc &= 0x0000FFFF; // его обнулим
			nCrc++; // но прибавим к сумме
		}
	}

	BKPkParam.nChecksum = nCrc;
	BKPkParam.EDAT = BKPkParam.BDAT + BKPkParam.LENW * 2;
	return true;
}

// Построение распределения старших байтов в старшей половине DHIB
// и сортировка программы
void DSTHIB()
{
	uint16_t DHIB_LENMAS[LENMAS_W];
	memset(DHIB_LENMAS, 0, sizeof(DHIB_LENMAS));
	int nAddr = BKPkParam.BDAT / 2; // возьмём адрес начала упаковки в словах
	int nEndw = BKPkParam.EDAT / 2; // получим адрес конца в словах

	do
	{
		int nIdx = (MMemory.w[nAddr] >> 8) & 0xff; // получим слово
		mas2v[nIdx].push_back(MMemory.w[nAddr] & 0xff); // сохраним младший байт этого слова в массиве
		DHIB_LENMAS[nIdx]++;    // увеличим счётчик таких слов

		if (MMemory.w[++nAddr] == MMemory.w[nAddr - 1])
		{
			// пропуск >=3 повторов
			int nCount = 0;

			while (nAddr < nEndw)
			{
				nCount++;

				if (MMemory.w[++nAddr] != MMemory.w[nAddr - 1])
				{
					break;
				}
			}

			if (nCount < 2)
			{
				nAddr -= nCount;
			}
		}
	}
	while (nAddr < nEndw);

	// Отсортировать распределение старших байтов
	for (int i = 0; i < LENMAS_W; ++i)
	{
		uint16_t w = short(char(i)) + 0400;
		dhib_v.push_back({ DHIB_LENMAS[i], w });
	}

	// сортируем в обратном порядке по количеству
	std::sort(dhib_v.begin(), dhib_v.end(), [&](Pair i, Pair j)
	{
		if (i.nMulti == j.nMulti)
		{
			return (i.nWord > j.nWord);
		}

		return (i.nMulti > j.nMulti);
	});
#if defined(_DEBUG) && defined(DBG_LOG)
	// тест
	FILE *tf = fopen("dhib_lenmas.txt", "wt");
	int r = 0135000;

	for (int i = 0; i < LENMAS_W; i += 4)
	{
		fprintf(tf, "%06o: %06o ", r, DHIB_LENMAS[i]);
		fprintf(tf, "%06o ", DHIB_LENMAS[i + 1]);
		fprintf(tf, "%06o ", DHIB_LENMAS[i + 2]);
		fprintf(tf, "%06o\n", DHIB_LENMAS[i + 3]);
		r += 010;
	}

	fclose(tf);
	tf = fopen("dhib_v.txt", "wt");
	r = 0134000;

	for (int i = 0; i < LENMAS_W; i += 2)
	{
		fprintf(tf, "%06o: %06o %06o ", r, dhib_v.at(i).nMulti, dhib_v.at(i).nWord);
		fprintf(tf, "%06o %06o\n", dhib_v.at(i + 1).nMulti, dhib_v.at(i + 1).nWord);
		r += 010;
	}

	fclose(tf);
#endif // _DEBUG
}
// Построение распределения слов c K >= 2
// сразу будем упорядочивать
void DSTWRD()
{
	uint16_t DHIB[LENMAS_W];    // Распределение старших байтов
	uint8_t nHiByte = -1;

	for (auto &R4 : mas2v)
	{
		memset(DHIB, 0, sizeof(DHIB));
		nHiByte++;

		if (!R4.empty())
		{
			// удаляем из цепочек повторяющиеся байты
			// и считаем общее количество каждого из уникальных байтов
			std::vector<std::vector<uint8_t>::iterator> vErasedPos;

			for (auto p = R4.begin(); p != R4.end(); ++p)
			{
				if (++DHIB[*p] > 1)
				{
					vErasedPos.push_back(p);
				}
			}

			// вот теперь удаляем с конца, чтобы итераторы оставались валидными
			while (!vErasedPos.empty())
			{
				R4.erase(vErasedPos.back());
				vErasedPos.pop_back();
			}

			// снова проходим по массиву
			for (uint8_t &b : R4)
			{
				if (DHIB[b] >= 2)   // количество таких байтов
				{
					// ст.байт - num цепочки, мл.байт - байт из цепочки
					uint16_t w = (uint16_t(nHiByte) << 8) | b; // воссоздаём слово
					awd.push_back({ DHIB[b], w });
				}
			}
		}
	}

	// и отсортируем в убывающем порядке по количеству.
	std::sort(awd.begin(), awd.end(), [&](Pair i, Pair j)
	{
		if (i.nMulti == j.nMulti)
		{
			return (i.nWord > j.nWord);
		}

		return (i.nMulti > j.nMulti);
	});

	// размер массива awd не должен превышать 0376 значений, всё что больше надо отбросить
	while (awd.size() > 0376)
	{
		awd.pop_back();
	}

#if defined(_DEBUG) && defined(DBG_LOG)
	FILE *tf = fopen("awd.txt", "wt");
	int r = 0134000;

	for (int i = 0; i < 0376; i += 2)
	{
		fprintf(tf, "%06o: %06o %06o ", r, awd.at(i + 1).nMulti, awd.at(i + 1).nWord);
		fprintf(tf, "%06o %06o\n", awd.at(i).nMulti, awd.at(i).nWord);
		r -= 010;
	}

	fclose(tf);
#endif
}

// Построение таблиц TB и TW
void BLDTAB()
{
	// определяем длину таблиц и переписываем
	BKPkParam.NTW = 0;
	short nRatio = 0 - dhib_v[255].nMulti - dhib_v[254].nMulti;
	int R1 = 254;

	for (auto &pair : awd)
	{
		short nSt = pair.nMulti - dhib_v[--R1].nMulti - 1; // счётчик слов - счётчик байтов

		if (nSt <= 0)   // если ВЕЛИЧИНА ЭКОНОМИИ получилась <= 0
		{
			break;  // прервать цикл
		}

		BKPkParam.NTW++; // подсчитаем кол-во обработанных элементов
		nRatio += nSt;
		pair.nMulti = dhib_v[R1].nWord; // слово из dhib_v сделаем кратностью awd (на будущее)
	}

	BKPkParam.CNTEC1 = nRatio;  // сохраняем результат, и потом нигде не используем
	memset(TB, 0, sizeof(TB));
	memset(TW, 0, sizeof(TW));
	R1 = 0;
	int R4 = 0;
	TB[R4++] = dhib_v[255].nWord;   // 2 байта для ESC
	TB[R4++] = dhib_v[254].nWord;

	for (int R2 = 0; R2 < BKPkParam.NTW; ++R2)
	{
		TW[R1++] = awd[R2].nWord;
		TB[R4++] = awd[R2].nMulti; // вот зачем мы в прошлый раз сюда копировали слово из dhib_v
	}

	// сортируем массив TB, в возрастающем порядке
	std::sort(TB, TB + BKPkParam.NTW + 2, [&](uint16_t i, uint16_t j)
	{
		return (i < j);
	});
	// построим шкалу байтов
	memset(SB, 0, sizeof(SB));

	for (int R2 = 0; R2 < BKPkParam.NTW + 2; ++R2)
	{
		auto R0 = short(char(TB[R2] & 0xff));
		int b = 1 << (R0 & 7);
		int a = (R0 >> 3) + SB_SIZE / 2;

		if (0 <= a && a < SB_SIZE)
		{
			SB[a] |= b;
		}
		else
		{
			assert(false);
		}
	}

	// сортируем массив TW, в возрастающем порядке
	std::sort(TW, TW + BKPkParam.NTW, [&](uint16_t i, uint16_t j)
	{
		return (i < j);
	});
#if defined(_DEBUG) && defined(DBG_LOG)
	FILE *tf = fopen("TW.txt", "wt");
	int r = 0135000;

	for (int i = 0; i < LENMAS_W; i += 4)
	{
		fprintf(tf, "%06o: %06o ", r, TW[i]);
		fprintf(tf, "%06o ", TW[i + 1]);
		fprintf(tf, "%06o ", TW[i + 2]);
		fprintf(tf, "%06o\n", TW[i + 3]);
		r += 010;
	}

	fclose(tf);
	tf = fopen("TB.txt", "wt");
	r = 0134000;

	for (int i = 0; i < LENMAS_W; i += 4)
	{
		fprintf(tf, "%06o: %06o ", r, TB[i]);
		fprintf(tf, "%06o ", TB[i + 1]);
		fprintf(tf, "%06o ", TB[i + 2]);
		fprintf(tf, "%06o\n", TB[i + 3]);
		r += 010;
	}

	fclose(tf);
	tf = fopen("SB.txt", "wt");
	r = 0136000;

	for (int i = 0; i < 040; i += 8)
	{
		fprintf(tf, "%06o: %03o ", r, SB[i]);
		fprintf(tf, "%03o ", SB[i + 1]);
		fprintf(tf, "%03o ", SB[i + 2]);
		fprintf(tf, "%03o ", SB[i + 3]);
		fprintf(tf, "%03o ", SB[i + 4]);
		fprintf(tf, "%03o ", SB[i + 5]);
		fprintf(tf, "%03o ", SB[i + 6]);
		fprintf(tf, "%03o\n", SB[i + 7]);
		r += 010;
	}

	fclose(tf);
#endif
}

// найти константу DELW - смещение к словам
void FNDCST()
{
	// найдём макс. интервал в TW
	short R5 = TW[0];   // интервал
	short R1 = R5;      // DELW+1
	int nIdx = BKPkParam.NTW - 1;

	if (nIdx >= 0)
	{
		R5 -= TW[nIdx]; // сначала будет W[N]-W[1]
	}

	while (nIdx > 0)
	{
		short R4 = TW[nIdx] - TW[nIdx - 1];

		if (R4 > R5)
		{
			R5 = R4;
			R1 = TW[nIdx];
		}

		nIdx--;
	}

	// находим DELW из условий DELW, DELW + 1, DELW + 100000 не из TW
	// такое обязательно найдётся на макс. интервале
	int R0 = 0;
	R1--;

	do
	{
		R1--;
		R5 = R1 + 0100000;
		R0 = 0; // начинаем поиск всегда с начала
	}
	while (SEARCH(TW, BKPkParam.NTW, R0, R5));

	BKPkParam.DELW = R1;
}

// поиск в упорядоченном массиве слов
//      *m - массив
//       nIdx - начальный индекс
//       nLen - интервал, в котором ищем
//       w - искомое слово
// выход: nIdx - индекс найденного
//       false - если не найдено
bool SEARCH(uint16_t *m, int nLen, int &nIdx, uint16_t w)
{
	int L = nIdx;
	int U = nIdx + nLen;

	for (;;)
	{
		if (L == U)
		{
			return false;
		}

		nIdx = (L + U) / 2;

		if (w < m[nIdx])
		{
			U = nIdx;
		}
		else if (w > m[nIdx])
		{
			L = nIdx + 1;
		}
		else
		{
			break;
		}
	}

	return true;
}

// добавить ESC1 и ESC2 в TW и упорядочить её по новому
void CORRTW()
{
	for (int R2 = 0; R2 < BKPkParam.NTW; ++R2)
	{
		TW[R2] -= BKPkParam.DELW; // как будет в AUNP
	}

	TW[BKPkParam.NTW++] = 1;        // ESC1
	TW[BKPkParam.NTW++] = 0100000;  // ESC2
	// отсортировать TW по новому
	std::sort(TW, TW + BKPkParam.NTW, [&](uint16_t i, uint16_t j)
	{
		return (i < j);
	});
	// найдём ESC1 и ESC2
	uint16_t R5 = 1;
	CWB(R5);
	BKPkParam.BESC1 = R5 & 0xff;
	R5 = 0100000;
	CWB(R5);
	BKPkParam.BESC2 = R5 & 0xff;
#if defined(_DEBUG) && defined(DBG_LOG)
	FILE *tf = fopen("TW_corr.txt", "wt");
	int r = 0135000;

	for (int i = 0; i < LENMAS_W; i += 4)
	{
		fprintf(tf, "%06o: %06o ", r, TW[i]);
		fprintf(tf, "%06o ", TW[i + 1]);
		fprintf(tf, "%06o ", TW[i + 2]);
		fprintf(tf, "%06o\n", TW[i + 3]);
		r += 010;
	}

	fclose(tf);
#endif
}

// преобразовать слово в байт
// вход:    R5 - слово
// выход:   R5 - байт
//       false - если не найден
bool CWB(uint16_t &R5)
{
	// поиск в TW
	int R0 = 0;

	if (SEARCH(TW, BKPkParam.NTW, R0, R5))
	{
		// кодируем слово байтом
		R5 = TB[BKPkParam.NTW - R0 - 1];
		return true;
	}

	return false;
}

// упаковать таблицу слов
// JPCK = false: только подсчитать параметры
void PACKTW(bool JPCK)
{
	memset(TWB, 0, sizeof(TWB));
	// вычислим макс. приращение
	uint16_t R5 = 0; // предыдущий элемент
	uint16_t R4 = 0; // макс. разность

	for (int nIdx = 0; nIdx < BKPkParam.NTW; ++nIdx)
	{
		uint16_t R1 = TW[nIdx]; // текущая разность
		R1 -= R5;
		R5 += R1; // восстановим новый элемент

		if (R4 < R1)
		{
			R4 = R1;
		}
	}

	// делим на 377 с округлением вниз, чтобы определить N1B
	BKPkParam.N1B = 0400 - (R4 / 0377); // эквивалентный код, дающий результат как в оригинале
	// Вычислим C1=N1B-200; C2=-377*N1B
	BKPkParam.C1 = BKPkParam.N1B - 0200;
	BKPkParam.C2 = -(BKPkParam.N1B * 0377);
	// упаковываем TW
	R5 = 0; // предыдущий элемент
	int nIdxPack = 0;

	for (int nIdx = 0; nIdx < BKPkParam.NTW; ++nIdx)
	{
		R4 = TW[nIdx];
		R4 -= R5;
		R5 += R4;

		if (JPCK)
		{
			if (R4 > BKPkParam.N1B) // сравним с N1B
			{
				// если больше
				TWB[nIdxPack++] = (((BKPkParam.C2 - R4) >> 8) + BKPkParam.C1) & 0xff; // to сначала пишем Hi(C2-DEL)+C1
				TWB[nIdxPack++] = (BKPkParam.C2 - R4) & 0xff; // а затем Lo(C2-DEL)
			}
			else
			{
				// если меньше или равно
				// кодируем 1 байтом C1-DEL
				TWB[nIdxPack++] = (BKPkParam.C1 - R4) & 0xff;
			}
		}
		else
		{
			// будем не писать, а только двигать R1
			nIdxPack++;

			if (R4 > BKPkParam.N1B)
			{
				nIdxPack++;
			}
		}
	}

	// вычислим новую длину TW
	BKPkParam.NTWB = nIdxPack; // длина в байтах
	// AUNP - адрес начала автораспаковщика
	// EAUNP - адрес конца автораспаковщика
	// LENAUNP = EAUNP-AUNP
	BKPkParam.LENAUN = nIdxPack + uint16_t(LENAUNP) + SB_SIZE - BKPkParam.DSHORT;
#if defined(_DEBUG) && defined(DBG_LOG)

	if (JPCK)
	{
		FILE *tf = fopen("TWB.txt", "wt");
		int r = 0135000;

		for (int i = 0; i < LENMAS; i += 8)
		{
			fprintf(tf, "%06o: %03o ", r, TWB[i]);
			fprintf(tf, "%03o ", TWB[i + 1]);
			fprintf(tf, "%03o ", TWB[i + 2]);
			fprintf(tf, "%03o ", TWB[i + 3]);
			fprintf(tf, "%03o ", TWB[i + 4]);
			fprintf(tf, "%03o ", TWB[i + 5]);
			fprintf(tf, "%03o ", TWB[i + 6]);
			fprintf(tf, "%03o\n", TWB[i + 7]);
			r += 010;
		}

		fclose(tf);
	}

#endif
}

// построение упакованной программы
void BLDPK()
{
	int R1 = BKPkParam.BDAT / 2;    // адрес чтения программы (в словах)
	int R4 = BKPkParam.BDAT;        // адрес записи упаковки (в байтах)
	BKPkParam.ABEG = R1;            // адрес начала упаковки (в словах)
	int nEndw = BKPkParam.LENW + R1; // получим адрес конца в словах

	do
	{
		uint16_t R5 = MMemory.w[R1];
		// проверка серии
		int R2 = R1 + 1;

		while (R2 < nEndw)
		{
			if (MMemory.w[R2] == R5)
			{
				R2++;
			}
			else
			{
				break;
			}
		}

		R2--;       // адрес последнего слова серии
		R2 -= R1;   // длина серии в словах - 1

		if (R2 >= 2)
		{
			// будем упаковывать
			R1 += R2;

			// кодируем повтор с помощью ESC1
			// длина серии в словах-1 = количество повторений
			while (R2 - 0400 > 0) // длинные серии разобьём по 400
			{
				R2 -= 0400;
				MMemory.b[R4++] = 0; // K = 400
				MMemory.b[R4++] = BKPkParam.BESC1; // ESC1
			}

			MMemory.b[R4++] = R2 & 0xff; // 1 <= K <= 400
			MMemory.b[R4++] = BKPkParam.BESC1; // ESC1
		}
		else
		{
			// 0DH
			R5 -= BKPkParam.DELW;

			if (!(R5 == 1 || R5 == 0100000) && CWB(R5))
			{
				// кодируем слово байтом
				MMemory.b[R4++] = R5 & 0xff;
				R1++; // следующее слово
			}
			else
			{
				// 0DL
				auto R0 = short(char((MMemory.w[R1] >> 8) & 0xff));
				int b = 1 << (R0 & 7);// маска
				int a = (R0 >> 3) + SB_SIZE / 2;// адрес байта в SB

				if (!(0 <= a && a < SB_SIZE))
				{
					assert(false);
				}

				// поиск старшего байта в SB
				if (SB[a] & b)
				{
					// кодируем старший байт с помощью ESC2
					if (R1 * 2 != R4) // если адрес чтения == адрес записи,
					{
						MMemory.b[R4++] = MMemory.w[R1] & 0xff; // переписываем младший байт
						MMemory.b[R4++] = (MMemory.w[R1] >> 8) & 0xff; // переписываем старший байт
						R1++;
						MMemory.b[R4++] = BKPkParam.BESC2;  // ESC2

						if (R1 * 2 == R4) // если теперь совпал,
						{
							UNPK(R1, R4); // to распакуем назад то, что упаковали
							BKPkParam.ABEG = R1;
						}

						continue;
					}
				}

				// 0DM
				// просто переписываем слово
				MMemory.b[R4++] = MMemory.w[R1] & 0xff; // переписываем младший байт
				MMemory.b[R4++] = (MMemory.w[R1] >> 8) & 0xff; // переписываем старший байт
				R1++;
			}
		}

		// проверяем, не догнал ли адрес записи адрес чтения

		if (R1 * 2 == R4)
		{
			BKPkParam.ABEG = R1;
		}
	}
	while (R1 < nEndw);

	BKPkParam.LENP = R4;
}

// распаковка (для восстановления зря упакованной части)
// Вход: R4 - конечный адрес чтения
//       R1 - адрес записи
void UNPK(int R1, int R4)
{
	do
	{
		uint16_t R5 = short(char(MMemory.b[--R4]));
		R5 += 0400;
		int R0 = 0;

		if (SEARCH(TB, BKPkParam.NTW, R0, R5))
		{
			R5 = TW[BKPkParam.NTW - R0 - 1]; // слово

			if (R5 == 1)
			{
				// повтор
				R5 = MMemory.b[--R4];

				do
				{
					uint16_t w = MMemory.w[R1];
					MMemory.w[--R1] = w;
				}
				while (--R5);
			}
			else if (R5 == 0100000)
			{
				uint16_t w = (uint16_t(MMemory.b[--R4]) << 8);
				w |= MMemory.b[--R4];
				MMemory.w[--R1] = w;
			}
			else
			{
				R5 += BKPkParam.DELW;
				MMemory.w[--R1] = R5;
			}
		}
		else
		{
			// непосредственный перенос
			uint16_t w = (uint16_t(MMemory.b[R4]) << 8);
			w |= MMemory.b[--R4];
			MMemory.w[--R1] = w;
		}
	}
	while (R1 * 2 != R4);
}

// корректировать автозапуск
void CORRAP()
{
	BKPkParam.DELJMP = 0;

	if (BKPkParam.LENAZ)
	{
		int R0 = BKPkParam.BDAT - 034; // адрес в памяти ячейки 744

		if (R0 < 0)
		{
			R0 = 0; // если начальный адрес > 744, то берём оттуда
		}

		int R1 = MMemory.w[R0 / 2]; // адрес пуска
		int R4 = R1;
		R1 -= 01000;

		if (R1 > 0) // если <= 1000, то не меняем
		{
			BKPkParam.DELJMP = R1;

			// находим конец массива автозапуска
			while (MMemory.w[R0 / 2] == R4 && R0 < BKPkParam.BDAT)
			{
				R0 += 2;
			}

			R0 -= 4;

			if (R0 < 0)
			{
				R0 = 0; // если перескочили за начало - скорректируем
			}

			do
			{
				MMemory.w[R0 / 2] = 01000; // меняем адрес пуска на 1000
				R0 -= 2;
			}
			while (R0 > 0 && MMemory.w[R0 / 2] == R4);   // во всех словах, совпадающих с исходным
		}
	}
}

// добавить автораспаковщик
void ADDAUN()
{
	int R4 = BKPkParam.BDAT;
	int R1 = R4; // адрес записи AUNP
	R4 += BKPkParam.LENP; // конечный адрес упакованного файла, может быть нечётный!
	// перепаковать AUNP и сдвинуть хвост упакованного файла
	auto *AUNPB = reinterpret_cast<uint8_t *>(BKPackAUNP); // поэтому все операции делать в байтах

	for (int R0 = BKPkParam.DSHORT; R0 < LENAUNP; ++R0)
	{
		MMemory.b[R4++] = MMemory.b[R1];    // вот: автораспаковщик поместим в начало,
		MMemory.b[R1++] = AUNPB[R0];        // а что было на его месте - в конец файла
	}

	// перепаковать SB
	for (uint8_t R0 : SB)
	{
		MMemory.b[R4++] = MMemory.b[R1];
		MMemory.b[R1++] = R0;
	}

	// перепаковать TW
	for (int R0 = 0; R0 < BKPkParam.NTWB; ++R0)
	{
		MMemory.b[R4++] = MMemory.b[R1];
		MMemory.b[R1++] = TWB[R0];
	}

	BKPkParam.EPK = R4; // запомним конец упакованного
	// настроить AUNP
	R1 = BKPkParam.BDAT;
	int R5 = BKPkParam.DSHORT;
	R1 -= R5;

	if (!R5)
	{
		// только для длинного AUNP
		MMemory.w[(R1 + LUNPF$) / 2] = BKPkParam.nFileLen;
		MMemory.w[(R1 + CS$) / 2] = BKPkParam.nChecksum;
	}

	int R0 = BKPkParam.AWREG + 070; // адрес TW = адрес рабочей области + 070
	MMemory.w[(R1 + WE$) / 2] += R0;
	MMemory.w[(R1 + C1$) / 2] = BKPkParam.C1;
	MMemory.w[(R1 + C2$) / 2] = BKPkParam.C2;
	MMemory.w[(R1 + WB$) / 2] += R0;
	MMemory.w[(R1 + LPK1$) / 2] += BKPkParam.NTWB;
	MMemory.w[(R1 + LPK1$) / 2] += BKPkParam.LENP;
	MMemory.w[(R1 + LAUN$) / 2] = BKPkParam.LENAUN;
	MMemory.w[(R1 + LUNP$) / 2] = BKPkParam.LENW * 2;
	MMemory.w[(R1 + WH$) / 2] += R0;
	MMemory.w[(R1 + DELW1$) / 2] = BKPkParam.DELW + 1;

	if (!BKPkParam.bData)
	{
		MMemory.w[(R1 + EOFS$) / 2] = 0160; // JMP (R0) для программ
	}

	R0 = BKPkParam.BDAT - BKPkParam.ABEG * 2 + BKPkParam.DELJMP;
	MMemory.w[(R1 + EOFS2$) / 2] = R0;
}

//////////////////////////////////////////////////////////////////////////

bool BKPacking(int nFileLoadAddr, int nFileLen, int nFileActualAddr)
{
	bool bRet = false;
	BKPkParam.DSHORT = (BKPkParam.bLongUnpacker) ? 0 : 022;

	if (prepareData(nFileLoadAddr, nFileLen, nFileActualAddr)) // анализ загруженного файла
	{
		wprintf(L".");
		DSTHIB();       // построение распределения старших байтов и сортировка
		wprintf(L".");
		DSTWRD();       // построение распределения слов
		wprintf(L".");
		BLDTAB();       // построение TB и TW
		wprintf(L".");
		FNDCST();       // найти константу DELW
		wprintf(L".");
		CORRTW();       // добавить ESC в TW
		wprintf(L".");
		PACKTW(false);  // вычислить размер таблицы слов и LENAUN
		wprintf(L".");
		BLDPK();        // построение упакованной программы
		wprintf(L".");
		PACKTW(true);   // упаковать таблицу слов
		wprintf(L".");
		CORRAP();       // корректировать автозапуск
		wprintf(L".");
		ADDAUN();       // формируем файл
		int nRatio = BKPkParam.EPK * 100 / BKPkParam.nFileLen;
		wprintf(L" OK, Ratio %d%%\n", nRatio);
		bRet = true;
	}
	else
	{
		wprintf(L"Ошибка:: файл слишком большой для упаковки.\n");
	}

	return bRet;
}


