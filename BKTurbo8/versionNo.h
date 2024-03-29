
#pragma once
#ifndef VERSIONNO_H
#define VERSIONNO_H

#define VERSION_MAJ 23
#define VERSION_MIN 10
#define VERSION_REV 2210
#define VERSION_BLD 5228

// Macros utils
#define RES_STRINGIZE1(a) #a
#define RES_STRINGIZE(a) RES_STRINGIZE1(a)

#define VERSION_MAJ_STR RES_STRINGIZE(VERSION_MAJ)
#define VERSION_MIN_STR RES_STRINGIZE(VERSION_MIN)
#define VERSION_REV_STR RES_STRINGIZE(VERSION_REV)
#define VERSION_BLD_STR RES_STRINGIZE(VERSION_BLD)

//не хочет работать.
#if defined _X64
#define FILE_POSTFIX64 "_x64"
#else
#define FILE_POSTFIX64 ""
#endif

#if defined TARGET_BIN2OBJ
#define APPNAME "BKbin2obj"
#define FILE_PRODUCT_STR "Конвертер bin в obj"
#define FILE_DESCRIPTION_STR "Конвертер бинарных файлов в файлы типа OBJ для кросс-ассемблера BKTurbo8"
#elif defined TARGET_TURBO8
#define APPNAME "BKTurbo8"
#define FILE_PRODUCT_STR "Кросс-ассемблер BKTurbo8"
#define FILE_DESCRIPTION_STR "Кросс-ассемблер BKTurbo8 для создания бинарных исполняемых файлов на системах PDP-11 BK0010(-01), BK0011(M)"
#elif defined TARGET_BKTOOLS
#define APPNAME "BKTools"
#define FILE_PRODUCT_STR "Инструменты для для кросс-ассемблера BKTurbo8"
#define FILE_DESCRIPTION_STR "Набор инструментов для кросс-ассемблера BKTurbo8"
#else
#define APPNAME "Unknown"
#define FILE_PRODUCT_STR "No data"
#define FILE_DESCRIPTION_STR "No data"
#endif
#define FILE_COMPANY_STR "gid"
#define FILE_COPYRIGHT_STR "Копировать правильно 2016-2023 (с) gid."

#endif // VERSIONNO_H
