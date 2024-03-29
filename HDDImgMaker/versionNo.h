#ifndef VERSIONNO_H
#define VERSIONNO_H

#define VERSION_MAJ 1
#define VERSION_MIN 2
#define VERSION_REV 2311
#define VERSION_BLD 259

// Macros utils
#define RES_STRINGIZE1(a) #a
#define RES_STRINGIZE(a) RES_STRINGIZE1(a)

#define VERSION_MAJ_STR RES_STRINGIZE(VERSION_MAJ)
#define VERSION_MIN_STR RES_STRINGIZE(VERSION_MIN)
#define VERSION_REV_STR RES_STRINGIZE(VERSION_REV)
#define VERSION_BLD_STR RES_STRINGIZE(VERSION_BLD)

//не хочет работать.
#ifdef _X64
#define FILE_POSTFIX64 "_x64"
#else
#define FILE_POSTFIX64 ""
#endif

#define APPNAME "HDDImgMaker"
#define FILE_COMPANY_STR "gid"
#define FILE_PRODUCT_STR "Создание образов виртуальных HDD для эмулятора БК"
#define FILE_DESCRIPTION_STR "Создание и конвертирование из посекторных образов реальных HDD образов виртуальных HDD для эмулятора БК и обратное преобразование в реальные посекторные образы HDD"
#define FILE_COPYRIGHT_STR "Копировать правильно 2012-2023 (с) gid."

#endif // VERSIONNO_H
