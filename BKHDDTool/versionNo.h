#ifndef VERSIONNO_H
#define VERSIONNO_H

#define VERSION_MAJ 23
#define VERSION_MIN 10
#define VERSION_REV 1011
#define VERSION_BLD 0959

// Macros utils
#define RES_STRINGIZE1(a) #a
#define RES_STRINGIZE(a) RES_STRINGIZE1(a)

#define VERSION_MAJ_STR RES_STRINGIZE(VERSION_MAJ)
#define VERSION_MIN_STR RES_STRINGIZE(VERSION_MIN)
#define VERSION_REV_STR RES_STRINGIZE(VERSION_REV)
#define VERSION_BLD_STR RES_STRINGIZE(VERSION_BLD)

#ifdef _X64
#define FILE_POSTFIX64 "_x64"
#else
#define FILE_POSTFIX64 ""
#endif

#define APPNAME "BKHDDTool"
#define FILE_PRODUCT_STR "Утилита для работы с виртуальными HDD эмулятора БК"
#define FILE_DESCRIPTION_STR "Разборка, сборка, конвертирование образов виртуальных HDD эмулятора БК"
#define FILE_COMPANY_STR "gid"
#define FILE_COPYRIGHT_STR "Копировать правильно 2016-2023 (с) gid."

#endif // VERSIONNO_H
