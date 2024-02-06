#pragma once
#include "HDIStuff.h"
#include "BKParseImage.h"

// структура опций
struct ProrgamOptions
{
	bool        bAltPro;    // флаг альтпро
	bool        bSamara;    // флаг самара
	bool        bCreate;    // true - создание образа, false - разбор
	bool        bConvert;   // true - конвертация образа, false - нет
	bool        bHDI;       // флаг создания образа в формате HDI
	uint16_t    C;          // геометрия: количество дорожек создаваемого образа
	uint16_t    H;          // геометрия: количество головок создаваемого образа
	uint16_t    S;          // геометрия: количество секторов на дорожке создаваемого образа
	fs::path    strPath;    // путь куда сохранять образы разделов, или где искать их при сборке
	fs::path    strAppPath; // текущий путь, откуда запустили прогу
	fs::path    strImgName; // задаваемое имя образа.
	ProrgamOptions()
		: bAltPro(true)
		, bSamara(false)
		, bHDI(false)
		, bCreate(false)
		, bConvert(false)
		, C(80)
		, H(4)
		, S(24)
	{
	}
};

extern std::wstring g_HDIModelName;

namespace BKHDDTool
{
	void Usage();

	void ShowMsg1(const std::wstring &n1, const std::wstring &n2);
	void ShowMsg2(const std::wstring &n1);

	void PrintInfo(IMGFormat *imgf);
	bool AnalyseImage(FILE *f, IMGFormat *imgf);
	bool AssemblyImage(FILE *f, IMGFormat *imgf);

	bool CheckFormat_hdd(FILE *f, IMGFormat *imgf);

	void MakeOutDir(bool bChangeOnly = false);
	bool ReadLogDiskImage(FILE *f, IMGFormat *imgf, const fs::path &strName);
	bool WriteLogDiskImage(FILE *f, int nld, uint32_t nLen);
	bool WriteEmptySectors(FILE *f, IMGFormat *imgf, int nWriteNum = 0);
	void WriteEmptyTail(FILE *f, IMGFormat *imgf);

	bool SplitAltPro(FILE *f, IMGFormat *imgf);
	bool SplitSamara(FILE *f, IMGFormat *imgf);

	void PrintOSType(const PARSE_RESULT &pr);
	bool AssemblyAltPro(FILE *f, IMGFormat *imgf);
	bool AssemblySamara(FILE *f, IMGFormat *imgf);

	bool ConvertLogDiskImage(FILE *inf, FILE *outf, IMGFormat *OutImgf, uint32_t nLenBlk);
	bool ConvertAltPro2Samara(FILE *f, IMGFormat *imgf);
	bool ConvertSamara2Altpro(FILE *f, IMGFormat *imgf);
}
