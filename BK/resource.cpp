﻿#include "resource.h"
#include "CString.h"
#include "debug.h"

BOOL CString::LoadString(int res) {
    switch (res) {
        case IDS_BK_ERROR_NOTENMEMR :       s = "Not enough RAM"; break;
       case IDS_INI_ADDR_DISASM     :       s = "Start disasm address"; break;
       case IDS_INI_ADDR_DUMP_0     :       s = "Start dump address #0"; break;
       case IDS_INI_ASK_FOR_BREAK   :       s = "Show dialog on access violation"; break;
       case IDS_INI_AUTO_BEG_TAPE   :       s = "Tape auto start detection"; break;
       case IDS_INI_AUTO_END_TAPE   :       s = "Tape auto end detection"; break;
       case IDS_INI_AY8910          :       s = "AY8910 enabled"; break;
       case IDS_INI_AY8910_FILTER   :       s = "AY8910 filter enabled"; break;
       case IDS_INI_BK10_RE2_017_MONITOR :  s = "BK10 Monitor"; break;
       case IDS_INI_BK10_RE2_018_FOCAL :    s = "BK10 Focal"; break;
       case IDS_INI_BK10_RE2_019_MSTD :     s = "BK10 MSTD"; break;
       case IDS_INI_BK10_RE2_106_BASIC1 :   s = "BK10 Basic1"; break;
       case IDS_INI_BK10_RE2_107_BASIC2 :   s = "BK10 Basic2"; break;
       case IDS_INI_BK10_RE2_108_BASIC3 :   s = "BK10 Basic3"; break;
       case IDS_INI_BK11_RE_OPT_PG12_1 :    s = "BK11 Optional Page12.1"; break;
       case IDS_INI_BK11_RE_OPT_PG12_2 :    s = "BK11 Optional Page12.2"; break;
       case IDS_INI_BK11_RE_OPT_PG13_1 :    s = "BK11 Optional Page13.1"; break;
       case IDS_INI_BK11_RE_OPT_PG13_2 :    s = "BK11 Optional Page13.2"; break;
       case IDS_INI_BK11_RE2_198_BASIC1 :   s = "BK11 Basic1"; break;
       case IDS_INI_BK11_RE2_199_BASIC2 :   s = "BK11 Basic2"; break;
       case IDS_INI_BK11_RE2_200_BASIC3 :   s = "BK11 Basic3"; break;
       case IDS_INI_BK11_RE2_201_BOS :      s = "BK11 Monitor BOS"; break;
       case IDS_INI_BK11_RE2_202_EXT :      s = "BK11 Monitor EXT"; break;
       case IDS_INI_BK11_RE2_203_MSTD :     s = "BK11 MSTD"; break;
       case IDS_INI_BK11M_RE2_324_BOS :     s = "BK11M Monitor BOS"; break;
       case IDS_INI_BK11M_RE2_325_EXT :     s = "BK11M Monitor EXT"; break;
       case IDS_INI_BK11M_RE2_327_BASIC1 :  s = "BK11M Basic1"; break;
       case IDS_INI_BK11M_RE2_328_BASIC2 :  s = "BK11M Basic2"; break;
       case IDS_INI_BK11M_RE2_329_BASIC3 :  s = "BK11M Basic3"; break;
       case IDS_FILEEXT_BINARY  :           s = ".bin"; break;
       case IDS_FILEEXT_MEMORYSTATE :       s = ".msf"; break;
       case IDS_FILEEXT_ROM :               s = ".rom"; break;
       case IDS_FILEEXT_SCRIPT :            s = ".bkscript"; break;
       case IDS_FILEEXT_TAPE  :             s = ".tap"; break;
       case IDS_FILEEXT_WAVE  :             s = ".wav"; break;
       case IDS_INI_MEM_DIRECTORY    :      s ="Memory directory"; break;
       case IDS_INI_OSCRENDER_TYPE   :      s ="Oscilloscope render type"; break;
       case IDS_INI_PAUSE_CPU        :      s ="Pause CPU after emulator start"; break;
       case IDS_INI_PROGRAM_DIRECTORY  :    s ="Program directory"; break;
       case IDS_INI_REGSDUMP_INTERVAL  :    s ="Registers dump interval"; break;
       case IDS_INI_ROM_DIRECTORY    :      s ="Rom directory"; break;
       case IDS_INI_SAVES_DEFAULT    :      s ="Use Saves directory as default"; break;
       case IDS_INI_SAVES_DIRECTORY  :      s ="User Saves directory"; break;
       case IDS_INI_SCRIPTS_DIRECTORY  :    s ="Scripts directory"; break;
       case IDS_INI_SCRRENDER_TYPE   :      s ="Screen render type"; break;
       case IDS_INI_SECTIONNAME_DIRECTORIES:s ="Directories"; break;
       case IDS_INI_SECTIONNAME_DRIVES  :   s ="Drives"; break;
       case IDS_INI_SECTIONNAME_MAIN  :     s ="Main"; break;
       case IDS_INI_SECTIONNAME_OPTIONS  :  s ="Options"; break;
       case IDS_INI_SECTIONNAME_PARAMETERS: s ="Parameters"; break;
       case IDS_INI_BK11M_RE2_330_MSTD :    s ="BK11M MSTD"; break;
       case IDS_INI_BKKEYBOARD      :       s ="Emulate BK keyboard"; break;
       case IDS_INI_BKMODEL         :       s ="BK model"; break;
       case IDS_INI_BLACK_WHITE     :       s ="Adapt black & white mode"; break;
       case IDS_INI_COLOR_MODE      :       s ="Run in color mode"; break;
       case IDS_INI_COVOX           :       s ="Covox enabled"; break;
       case IDS_INI_COVOX_FILTER    :       s ="Covox filter enabled"; break;
       case IDS_INI_COVOX_STEREO    :       s ="Stereo covox"; break;
       case IDS_INI_CPU_FREQUENCY   :       s ="CPU frequency"; break;
       case IDS_INI_CPU_RUN_ADDR    :       s ="CPU start address"; break;
       case IDS_INI_DRIVEA          :       s ="Drive A:"; break;
       case IDS_INI_DRIVEB          :       s ="Drive B:"; break;
       case IDS_INI_DRIVEC          :       s ="Drive C:"; break;
       case IDS_INI_DRIVED          :       s ="Drive D:"; break;
       case IDS_INI_ORIG_SCRNSHOT_SIZE :    s ="Origin screenshot size"; break;
       case IDS_INI_BIGBUTTONS      :       s ="Big buttons for Instrumental Panel"; break;
       case IDS_INI_PALBW           :       s ="BW Palette"; break;
       case IDS_INI_PALADAPTBW      :       s ="Adapt BW Palette"; break;
       case IDS_INI_PALCOL00        :       s ="Color Palette 00"; break;
       case IDS_INI_PALCOL01        :       s ="Color Palette 01"; break;
       case IDS_INI_PALCOL02        :       s ="Color Palette 02"; break;
       case IDS_INI_PALCOL03        :       s ="Color Palette 03"; break;
       case IDS_INI_PALCOL04        :       s ="Color Palette 04"; break;
       case IDS_INI_PALCOL05        :       s ="Color Palette 05"; break;
       case IDS_INI_PALCOL06        :       s ="Color Palette 06"; break;
       case IDS_INI_PALCOL07        :       s ="Color Palette 07"; break;
       case IDS_INI_PALCOL08        :       s ="Color Palette 08"; break;
       case IDS_INI_PALCOL09        :       s ="Color Palette 09"; break;
       case IDS_INI_PALCOL10        :       s ="Color Palette 10"; break;
       case IDS_INI_PALCOL11        :       s ="Color Palette 11"; break;
       case IDS_INI_PALCOL12        :       s ="Color Palette 12"; break;
       case IDS_INI_PALCOL13        :       s ="Color Palette 13"; break;
       case IDS_INI_PALCOL14        :       s ="Color Palette 14"; break;
       case IDS_INI_PALCOL15        :       s ="Color Palette 15"; break;
       case IDS_INI_MOUSEM          :       s ="Emulate Mouse"; break;
       case IDS_INI_BKJOY_UP        :       s ="Joystick Up"; break;
       case IDS_INI_BKJOY_RIGHT     :       s ="Joystick Right"; break;
       case IDS_INI_BKJOY_DOWN      :       s ="Joystick Down"; break;
       case IDS_INI_BKJOY_LEFT      :       s ="Joystick Left"; break;
       case IDS_INI_BKJOY_FIRE      :       s ="Joystick Fire"; break;
       case IDS_INI_BKJOY_ALTFIRE   :       s ="Joystick AltFire"; break;
       case IDS_INI_BKJOY_A         :       s ="Joystick A Button"; break;
       case IDS_INI_BKJOY_B         :       s ="Joystick B Button"; break;
       case IDS_INI_SECTIONNAME_PALETTES :  s ="Palettes"; break;
       case IDS_INI_SECTIONNAME_JOYSTICK :  s ="Joystick Parameters"; break;
       case IDS_MEMORY_177130OUT    :       s ="177130|Зп"; break;
       case IDS_MEMORY_177132IN     :       s ="177132|Чт"; break;
       case IDS_MEMORY_177132OUT    :       s ="177132|Зп"; break;
       case IDS_MEMORY_177660       :       s ="177660"; break;
       case IDS_MEMORY_177662IN     :       s ="177662|Чт"; break;
       case IDS_MEMORY_177662OUT    :       s ="177662|Зп"; break;
       case IDS_MEMORY_177664       :       s ="177664"; break;
       case IDS_MEMORY_177700       :       s ="177700"; break;
       case IDS_MEMORY_177702       :       s ="177702"; break;
       case IDS_MEMORY_177704       :       s ="177704"; break;
       case IDS_MEMORY_177706       :       s ="177706"; break;
       case IDS_MEMORY_177710       :       s ="177710"; break;
       case IDS_MEMORY_177712       :       s ="177712"; break;
       case IDS_MEMORY_177714IN     :       s ="177714|Чт"; break;
       case IDS_MEMORY_177714OUT    :       s ="177714|Зп"; break;
       case IDS_MEMORY_177716IN     :       s ="177716|Чт"; break;
       case IDS_MEMORY_177130IN     :       s ="177130|Чт"; break;
       case IDS_INI_SECTIONNAME_ROMMODULES :s ="Rom modules"; break;
       case IDS_INI_SHOW_PERFORMANCE_STRING:s ="Show performance string"; break;
       case IDS_INI_SOUND_SAMPLE_RATE :     s ="Sound Sample Rate"; break;
       case IDS_INI_SOUNDVOLUME     :       s ="Sound volume"; break;
       case IDS_INI_SPEAKER         :       s ="Speaker enabled"; break;
       case IDS_INI_SPEAKER_FILTER  :       s ="Speaker filter enabled"; break;
       case IDS_INI_SSHOT_DIRECTORY :       s ="Screenshots directory"; break;
       case IDS_INI_SSHOT_NUM       :       s ="Screenshot number"; break;
       case IDS_INI_TAPES_DIRECTORY :       s ="Tapes directory"; break;
       case IDS_INI_TOOLS_DIRECTORY :       s ="Tools directory"; break;
       case IDS_INI_VKBD_TYPE       :       s ="Virtual Keyboard Type"; break;
       case IDS_INI_SOUNDCHIPFREQ   :       s ="SoundChip Frequency"; break;
       case IDS_INI_SOUNDCHIPMODEL  :       s ="SoundChip Model"; break;
       case IDS_INI_LONGBIN         :       s ="Use long Bin format"; break;
       case IDS_INI_FFMPEGCMDLINE   :       s ="FFMPEG Cmd Line"; break;
       case IDS_INI_SMOOTHING       :       s ="Smoothing Screen"; break;
       case IDS_INI_EMUL_LOAD_TAPE  :       s ="Emulate load tape operations"; break;
       case IDS_INI_EMUL_SAVE_TAPE  :       s ="Emulate save tape operations"; break;
       case IDS_INI_EMULATE_FDDIO   :       s ="Emulate FDD IO"; break;
       case IDS_INI_FDR             :       s ="FDD KNGMD"; break;
       case IDS_INI_FDR_A16M        :       s ="FDD A16M"; break;
       case IDS_INI_FDR_SAMARA      :       s ="FDD Samara"; break;
       case IDS_INI_FDR_SMK512      :       s ="FDD SMK512"; break;
       case IDS_INI_FDR253          :       s ="FDD KNGMD253"; break;
       case IDS_INI_FILENAME        :       s ="bk.ini"; break;
       case IDS_INI_FULL_SCREEN     :       s ="Run in fullscreen mode"; break;
       case IDS_INI_HDD0            :       s ="HDD0"; break;
       case IDS_INI_HDD1            :       s ="HDD1"; break;
       case IDS_INI_ICLBLOCK        :       s ="IC Load Block"; break;
       case IDS_INI_IMG_DIRECTORY   :       s ="IMG directory"; break;
       case IDS_INI_JOYSTICK        :       s ="Joystick enabled"; break;
       case IDS_INI_LUMINOFOREMODE  :       s ="Screen fade emulation"; break;
       case IDS_MSCHWM_TURBOSOUND   :       s ="TurboSound"; break;
       case IDS_MSCHWM_GID          :       s ="gid"; break;
       case IDS_INI_2AYWORKMODE     :       s ="Multi AY Work Mode"; break;
       case IDS_INI_ENABLEMMG       :       s ="Enable MMG Instructions"; break;
       case IDS_INI_ENABLEEIS       :       s ="Enable EIS Instructions"; break;
       case IDS_INI_ENABLEFIS       :       s ="Enable FIS Instructions"; break;
       case IDS_INI_ENABLEFPU       :       s ="Enable FPU Instructions"; break;
       case IDS_BK_ERROR_SCRDLLINITERR:     s ="Не удалось инициализировать рендер из библиотеки DLL '%s'."; break;
       case IDS_BK_ERROR_SCRCOLORTABLEERR:  s ="Не удалось инициализировать цветовую таблицу экрана."; break;
       case IDS_BK_ERROR_WRONGFILE  :       s ="Не тот файл подсовываете. Повторим?"; break;
       case IDS_INI_AY1PANAL        :       s ="AY Channel A Pan Left"; break;
       case IDS_INI_AY1PANBL        :       s ="AY Channel B Pan Left"; break;
       case IDS_INI_AY1PANCL        :       s ="AY Channel C Pan Left"; break;
       case IDS_INI_AY1VOLA         :       s ="AY Channel A Volume"; break;
       case IDS_INI_AY1VOLB         :       s ="AY Channel B Volume"; break;
       case IDS_INI_AY1VOLC         :       s ="AY Channel C Volume"; break;
       case IDS_CONFAY_AY38910      :       s ="AY-3-8910"; break;
       case IDS_CONFAY_YM2149F      :       s ="YM2149F"; break;
       case IDS_BK_ERROR_NOCREATEVKBD:      s ="Окно CBKKbdButn не захотело создаваться."; break;
       case IDS_INI_BK10_OPT_MSTD   :       s ="BK10 Optional MSTD"; break;
       case IDS_INI_MENESTREL       :       s ="Menestrel enabled"; break;
       case IDS_INI_MENESTREL_FILTER:       s ="Menestrel filter enabled"; break;
       case IDS_INI_SPEAKER_DCOFFSET:       s ="Speaker DC Offset correct"; break;
       case IDS_TOOLTIP_CHECK_DA_FIS:       s ="Команды FMUL, FDIV, FADD, FSUB."; break;
       case IDS_TOOLTIP_CHECK_DA_FPU:       s ="Команды математического сопроцессора\nс опкодами 0170000..0177777."; break;
       case IDS_INI_SSHOT_DATENUM   :       s ="Date Instead Of Screenshot Number"; break;
       case IDS_INI_EMULATE_CBUG    :       s ="Emulate C Bug"; break;
       case IDS_INI_EMULATE_177702  :       s ="Emulate 177702 behavior"; break;
       case IDS_INI_EMULATE_VM1G    :       s ="Emulate CPU 1801VM1G"; break;
       case IDS_INI_EMULATE_EIS     :       s ="Emulate EIS Instructions Set"; break;
       case IDS_INI_EMULATE_FIS     :       s ="Emulate FIS Instructions Set"; break;
       case IDS_INI_JCUKENKBD       :       s ="Emulate JCUKEN Layout"; break;
       case IDS_INI_DMP_LIST_POS_0  :       s ="Dump List Pos #0"; break;
       case IDS_INI_DMP_LIST_POS_1  :       s ="Dump List Pos #1"; break;
       case IDS_INI_DMP_LIST_POS_2  :       s ="Dump List Pos #2"; break;
       case IDS_INI_DMP_LIST_POS_3  :       s ="Dump List Pos #3"; break;
       case IDS_INI_BK10_RE2_084_FOCAL :    s ="BK10 Focal_84"; break;
       case IDS_CONFNAME_BK_0010    :       s ="БК 0010Ш"; break;
       case IDS_INI_2NDAY8910       :       s ="2nd AY8910 Enabled"; break;
       case IDS_INI_SOUNDCHIPMODEL2 :       s ="SoundChip #2 Model"; break;
       case IDS_INI_AY2PANAL        :       s ="AY2 Channel A Pan Left"; break;
       case IDS_INI_AY2PANBL        :       s ="AY2 Channel B Pan Left"; break;
       case IDS_INI_AY2PANCL        :       s ="AY2 Channel C Pan Left"; break;
       case IDS_INI_AY2VOLA         :       s ="AY2 Channel A Volume"; break;
       case IDS_INI_AY2VOLB         :       s ="AY2 Channel B Volume"; break;
       case IDS_INI_AY2VOLC         :       s ="AY2 Channel C Volume"; break;
       case IDS_MSCHWM_GRYPHON      :       s ="Gryphon"; break;
       case IDS_INI_COVOX_DCOFFSET  :       s ="Covox DC Offset correct"; break;
       case IDS_INI_AY8910_DCOFFSET :       s ="AY8910 DC Offset correct"; break;
       case IDS_INI_MENESTREL_DCOFFSET:     s ="Menestrel DC Offset correct"; break;
       case IDS_INI_EXCLUSIVEOPENIMAGES:    s ="Exclusive Open Image Files"; break;
       case IDS_INI_ADDR_DUMP_1     :       s ="Start dump address #1"; break;
       case IDS_INI_ADDR_DUMP_2     :       s ="Start dump address #2"; break;
       case IDS_INI_ADDR_DUMP_3     :       s ="Start dump address #3"; break;
       case IDS_INI_SCRWIDTH        :       s ="Screen Width"; break;
       case IDS_INI_SCRASPECT       :       s ="Screen Aspect Ratio"; break;
       case IDS_TOOLTIP_EDIT_MD_DUMPADDR:   s ="Здесь задаётся адрес дампа в восьмеричной форме."; break;
       case IDS_TOOLTIP_BUTTON_MD_DUMPMODE: s ="Смена режима отображения дампа слова/байты."; break;
       case IDS_TOOLTIP_BUTTON_MD_SAVE :    s ="Сохранить дамп памяти в файл в формате .bin\nС зажатым 'Shift' - параметры операции."; break;
       case IDS_TOOLTIP_BUTTON_MD_LOAD :    s ="Загрузить дамп памяти в формате .bin\nС зажатым 'Shift' - параметры операции."; break;
       case IDS_INI_NATIVERUSLATSWITCH :    s ="Native RusLat Switch"; break;
       case IDS_TOOLTIP_BUTTON_RDC_CVMODE : s ="Переключение режима дополнительного представления содержимого регистров."; break;
       case IDS_TOOLTIP_EDIT_RDC_CPUFREQ :  s ="Поле для ручного задания частоты."; break;
       default:
            TRACE_T("LoadString failed for %d", res);
            s.clear();
            return false;
    }
    return true;
}
