#pragma once

union MemoryModel
{
	uint8_t b[65536];
	uint16_t w[32768];
};

enum BKTools
{
	UNDEFINE,
	BKPACK,
	BKCRUNCH
};

extern fs::path g_strInFileName;    // имя входного файла
extern fs::path g_strOutFileName;   // имя выходного файла
extern bool g_bRaw;         // флаг, создавать выходной файл в RAW форме
extern bool g_bUnpack;      // флаг, задействовать распаковщик
extern int g_nWorkArea;     // адрес конца рабочей области по умолчанию. (-1 - не задано, выбирается автоматически)

extern FILE *g_File;        // файл
extern int g_nFileLen, g_nFileAddress, g_nFileLoadAddress; // параметры файла по умолчанию
extern BKTools g_tool;

