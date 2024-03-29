#ifndef VERSIONNO_H
#define VERSIONNO_H

#define VERSION_MAJ 23
#define VERSION_MIN 10
#define VERSION_REV 0914
#define VERSION_BLD 4137

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

#define APPNAME "ITCPC"
#define FILE_PRODUCT_STR "Конвертер текстов BK <--> PC"
#define FILE_DESCRIPTION_STR "Конвертер текстовых файлов из форматов БК в простой текст PC и наоборот"
#define FILE_COMPANY_STR "gid"
#define FILE_COPYRIGHT_STR "Копировать правильно 2000-2021 (с) gid."

#endif // VERSIONNO_H
