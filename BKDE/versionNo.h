
#pragma once
#ifndef VERSIONNO_H
#define VERSIONNO_H

#define VERSION_MAJ 2
#define VERSION_MIN 2
#define VERSION_REV 2312
#define VERSION_BLD 899

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

#if defined TARGET_BKDE
#define APPNAME "BKDE"
#define FILE_PRODUCT_STR "BK Disk Explorer"
#elif defined TARGET_BKDL
#define APPNAME "BKDL"
#define FILE_PRODUCT_STR "BK Disk Structure Listener"
#else
#define APPNAME "Unknown"
#define FILE_PRODUCT_STR "No data"
#endif
#define FILE_COMPANY_STR "gid"
#define FILE_DESCRIPTION_STR "Работа с образами дискет БК."
#define FILE_COPYRIGHT_STR "Копировать правильно 2012-2023 (с) gid."

#endif // VERSIONNO_H
