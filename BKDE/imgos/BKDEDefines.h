#pragma once

#ifndef BKDEDEFINES_H
#define BKDEDEFINES_H

constexpr auto DEFAULT_DPIX = 96;
constexpr auto DEFAULT_DPIY = 96;

#define WM_MAKE_DROP            (WM_USER +  1)
#define WM_GET_CMDLINE          (WM_USER +  2)
#define WM_OUT_OF_IMAGE         (WM_USER +  3)
#define WM_PUT_INTO_LD          (WM_USER +  4)
#define WM_OUT_CURR_FILE_PATH   (WM_USER +  5)
#define WM_OUT_CURR_IMG_INFO    (WM_USER +  6)
#define WM_SEND_ERRORNUM        (WM_USER +  7)
#define WM_SEND_MESSAGEBOX      (WM_USER +  8)
#define WM_SEND_PROCESSING      (WM_USER +  9)
#define WM_SEND_IMGNAMEPRC      (WM_USER + 10)
#define WM_SEND_ENABLE_BUTTON   (WM_USER + 11)
#define WM_GET_ENABLE_BUTTON    (WM_USER + 13)

constexpr auto LC_FNAME_ST_COL_WIDTH = 200;
constexpr auto LC_SIZE_ST_COL_WIDTH = 70;
constexpr auto LC_OSTYPE_ST_COL_WIDTH = 70;
constexpr auto LC_SYSTYPE_ST_COL_WIDTH = 76;


constexpr auto LC_FNAME_COL_WIDTH = 200;
constexpr auto LC_TYPE_COL_WIDTH = 70;
constexpr auto LC_BLKSZ_COL_WIDTH = 50;
constexpr auto LC_ADDRESS_COL_WIDTH = 60;
constexpr auto LC_SIZE_COL_WIDTH = 80;
constexpr auto LC_ATTR_COL_WIDTH = 70;
constexpr auto LC_SPECIFIC_COL_WIDTH = 108;

// биты-признаки флагов разрешения контролов
constexpr uint32_t ENABLE_BUTON_EXTRACT = 0x0001;
constexpr uint32_t ENABLE_BUTON_VIEW    = 0x0002;
constexpr uint32_t ENABLE_BUTON_VIEWSPR = 0x0004;
constexpr uint32_t ENABLE_BUTON_ADD     = 0x0008;
constexpr uint32_t ENABLE_BUTON_DEL     = 0x0010;
constexpr uint32_t ENABLE_BUTON_REN     = 0x0020;
// и действий
constexpr uint32_t ENABLE_CONTEXT_CHANGEADDR    = 0x0100;


#endif
