#pragma once

#include "resource.h"

#include "BKParseImage.h"
#include "BKFloppyImage_Prototype.h"

PaneInfo                m_PaneInfo;
std::vector<PaneInfo>   m_vSelItems;

std::unique_ptr<CBKFloppyImage_Prototype> m_pFloppyImage;
std::vector<std::unique_ptr<CBKFloppyImage_Prototype>> m_vpImages; // тут будут храниться объекты, когда мы заходим в лог.диски

struct CSV_Fields
{
	fs::path strImagePath;
	fs::path strImageName;
	std::wstring strOSType;
	bool bBootable;
	bool bLogDisk;
	fs::path strInImgPath;
	fs::path strInImgName;
	int nBlkSize;
	int nAddr;
	int nSize;
	std::wstring strAttr;
};

bool ScanDir(const fs::path &strInPath);
void OutProgress(const fs::path &strPath);
void Usage();

bool OutDir();

bool Open(PARSE_RESULT &pr, bool bLogDisk = false); // открыть файл по результатам парсинга
bool ReOpen(); // переинициализация уже открытого образа
void Close(); // закрыть текущий файл
const std::wstring GetImgFormatName(IMAGE_TYPE nType = IMAGE_TYPE::UNKNOWN);
void ClearImgVector();
void PushCurrentImg();
bool PopCurrentImg();





