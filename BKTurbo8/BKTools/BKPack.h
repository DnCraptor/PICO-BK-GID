#pragma once

#define DBG_LOG 1
#include "BKTools.h"

struct BKPParam
{
	uint16_t    DSHORT; // прибавка к длине в зависимости от того
	// длинный распаковщик или нет, если длинный тут 0, иначе 022
	uint16_t    C2;     // константы для упак.табл.слов
	uint16_t    C1;     // константы для упак.табл.слов
	uint8_t     BESC2;  // байт ESC2
	uint8_t     BESC1;  // байт ESC1
	uint16_t    AWREG;  // адрес рабочей области
	uint16_t    DELJMP; // поправка в случае нестандартного адреса пуска
	uint16_t    LENAZ;  // длина блока файла до адреса 1000 (блока автозапуска)
	uint16_t    EDAT;   // конечный адрес упаковки
	uint16_t    BDAT;   // адрес начала пакуемых данных
	uint16_t    LENAUN; // длина автораспаковщика с таблицами
	uint16_t    LENW;   // длина загруженного файла в словах
	uint16_t    NTW;    // число элементов в TB и TW(без ESC)
	uint16_t    CNTEC1; // экономия от кодировки слов байтами
	uint16_t    DELW;   // смещение к словам
	uint16_t    ABEG;   // адрес начала упаковки
	uint16_t    LENP;   // длина упакованной программы
	uint16_t    N1B;    // максимальное DELW, кодируемое байтом
	uint16_t    NTWB;   // длина упакованной TW
	uint16_t    EPK;    // конец упакованной программы + AUNP

	bool        bData;          // опция Code/Data (false - код, true - данные)
	bool        bLongUnpacker;  // опция Short/Long распаковщик (false - короткий, true - длинный)
	// параметры для длинного распаковщика
	uint16_t    nFileLen;   // размер файла
	uint16_t    nChecksum;  // контрольная сумма файла
};

struct Pair
{
	uint16_t nMulti;    // Кратность
	uint16_t nWord;     // Слово
};

extern MemoryModel MMemory;
extern BKPParam BKPkParam;

constexpr auto LENMAS = 01000;
constexpr auto LENMAS_W = 0400;
constexpr auto SB_SIZE = 040;

extern uint16_t TW[LENMAS_W];
extern uint16_t BKPackAUNP[];

// максимальный размер файла, который можно обработать алгоритмом на данный момент
#define MAXLEN 074000
// смещения до параметров в блоке автораспаковки
#define LUNPF$  2
#define CS$     016
#define WE$     034
#define C1$     060
#define C2$     072
#define WB$     0104
#define LPK1$   0122
#define LAUN$   0126
#define LUNP$   0140
#define WH$     0150
#define DELW1$  0164
#define EOFS$   0216
#define EOFS2$  0220

bool SEARCH(uint16_t *m, int nLen, int &nIdx, uint16_t w);
bool CWB(uint16_t &R5);
void UNPK(int R1, int R4);
bool BKPacking(int nFileLoadAddr, int nFileLen, int nFileActualAddr);
bool BKUnPacking(int nFileLoadAddr, int nFileLen, int nFileActualAddr);

