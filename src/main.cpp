extern "C" {
#include "ff.h"
#include "f_util.h"
#include "config_em.h"
#include "manager.h"
}

#include <pico/time.h>
#include <pico/multicore.h>
#include <hardware/pwm.h>
#include <hardware/clocks.h>
#include <pico/stdlib.h>
#include <hardware/vreg.h>
#include <pico/stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Board_10.h"
#include "Board_10_FDD.h"
#include "Config.h"
#include "resource.h"
#include "debug.h"
#include <WinUser.h>

extern "C" {
//#include "psram_spi.h"
#include "nespad.h"
#include "util_Wii_Joy.h"
#include "vga.h"
#include "ps2.h"
#include "usb.h"
#include "vga.h"
#include "aySoundSoft.h"
#include "pico-vision.h"
}

volatile config_em_t g_conf {
   false, // is_covox_on
   true, // is_AY_on
   true, // color_mode
   BK_0011M_FDD, // bk0010mode
   0, // snd_volume
   0, // graphics_pallette_idx
   0330, // shift_y
   256, // graphics_buffer_height
   0, // v_buff_offset
};

bool PSRAM_AVAILABLE = false;
bool SD_CARD_AVAILABLE = false;

uint8_t __aligned(4096) TEXT_VIDEO_RAM[VIDEORAM_SIZE] = { 0 };
//uint8_t __aligned(4096) SCREEN_RAM[32ul << 10] = { 0 };

pwm_config config = pwm_get_default_config();
#define PWM_PIN0 (26)
#define PWM_PIN1 (27)

void PWM_init_pin(uint8_t pinN, uint16_t max_lvl) {
    gpio_set_function(pinN, GPIO_FUNC_PWM);
    pwm_config_set_clkdiv(&config, 1.0);
    pwm_config_set_wrap(&config, max_lvl); // MAX PWM value
    pwm_init(pwm_gpio_to_slice_num(pinN), &config, true);
}
#if NESPAD_ENABLED
int timer_period = 54925;
#endif
struct semaphore vga_start_semaphore;
/* Renderer loop on Pico's second core */
void __time_critical_func(render_core)() {
    graphics_init();
    graphics_set_buffer(TEXT_VIDEO_RAM, 512, 256); // TODO:
    graphics_set_textbuffer(TEXT_VIDEO_RAM);
    graphics_set_bgcolor(0x80808080);
    graphics_set_offset(0, 0);
    graphics_set_flashmode(true, true);
    sem_acquire_blocking(&vga_start_semaphore);
}

void inInit(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}

#ifdef USE_WII
static repeating_timer_t Wii_timer;
static bool __not_in_flash_func(Wii_Joystick_Timer_CB)(repeating_timer_t *rt) {
    if (is_WII_Init()) {
        Wii_decode_joy();
    }
    return true;
}
#endif

static repeating_timer_t main_timer;
static bool __not_in_flash_func(main_timer_callback)(repeating_timer_t *rt) {
    if (manager_started) {
        return true;
    }
    CMotherBoard* m_pBoard = (CMotherBoard*)rt->user_data;
    //int i = 20;
    //while(--i)
        m_pBoard->TimerThreadFunc();
    return true;
}

#ifdef SOUND_SYSTEM
static repeating_timer_t timer;
static bool __not_in_flash_func(AY_timer_callback)(repeating_timer_t *rt) {
    static uint16_t outL = 0;  
    static uint16_t outR = 0;
    pwm_set_gpio_level(PWM_PIN0, outR); // Право
    pwm_set_gpio_level(PWM_PIN1, outL); // Лево
    outL = outR = 0;
    if (manager_started) {
        return true;
    }
#ifdef AYSOUND
    if (g_conf.is_AY_on) {
        uint8_t* AY_data = get_AY_Out(5);
        outL = ((uint16_t)AY_data[0] + AY_data[1]) << 1;
        outR = ((uint16_t)AY_data[2] + AY_data[1]) << 1;
    }
#endif
#ifdef COVOX
    if (!outL && !outR && g_conf.is_covox_on && (true_covox || az_covox_L || az_covox_R)) {
        outL = (az_covox_L + (uint16_t)true_covox);
        outR = (az_covox_R + (uint16_t)true_covox);
    }
#endif
    if (outR || outL) {
        register uint8_t mult = g_conf.snd_volume;
        if (mult >= 0) {
            if (mult > 5) mult = 5;
            outL <<= mult;
            outR <<= mult;
        } else {
            register int8_t div = -mult;
            if (div > 16) div = 16;
            outL >>= div;
            outR >>= div;
        }
        pwm_set_gpio_level(BEEPER_PIN, 0);
    }
    return true;
}
#endif

static FATFS fatfs;

inline static int parse_conf_word(const char* buf, const char* param, size_t plen, size_t lim) {
    char b[16];
    const char* pc = strnstr(buf, param, lim);
    DBGM_PRINT(("param %s pc: %08Xh", param, pc));
    if (pc) {
        pc += plen - 1;
        const char* pc2 = strnstr(pc, "\r\n", lim - (pc - buf));
        DBGM_PRINT(("param %s \\r\\n pc: %08Xh pc2: %08Xh", param, pc, pc2));
        if (pc2) {
            memcpy(b, pc, pc2 - pc);
            b[pc2 - pc] = 0;
            DBGM_PRINT(("param %s b: %s atoi(b): %d", param, b, atoi(b)));
            return atoi(b);
        }
        pc2 = strnstr(pc, ";", lim - (pc - buf));
        DBGM_PRINT(("param %s ; pc2: %08Xh", param, pc2));
        if (pc2) {
            memcpy(b, pc, pc2 - pc);
            b[pc2 - pc] = 0;
            DBGM_PRINT(("param %s b: %s atoi(b): %d", param, b, atoi(b)));
            return atoi(b);
        }
        pc2 = strnstr(pc, "\n", lim - (pc - buf));
        DBGM_PRINT(("param %s \\n pc2: %08Xh", param, pc2));
        if (pc2) {
            memcpy(b, pc, pc2 - pc);
            b[pc2 - pc] = 0;
            DBGM_PRINT(("param %s b: %s atoi(b): %d", param, b, atoi(b)));
            return atoi(b);
        }
        DBGM_PRINT(("param %s pc: %d atoi(pc): %d", param, pc, atoi(pc)));
        return atoi(pc);
    }
    return -100;
}

extern "C" bool is_swap_wins_enabled;
extern "C" volatile bool is_dendy_joystick;
extern "C" volatile bool is_kbd_joystick;

inline static void read_config(const char* path) {
    FIL fil;
    if (f_open(&fil, path, FA_READ) != FR_OK) {
        DBGM_PRINT(("f_open %s failed", path));
        return;
    }
    char buf[256] = { 0 };
    UINT br;
    if (f_read(&fil, buf, 256, &br) != FR_OK) {
        DBGM_PRINT(("f_read %s failed. br: %d", br));
        f_close(&fil);
        return;
    }
    DBGM_PRINT(("f_read %s passed. br: %d", br));
    const char p0[] = "mode:";
    int mode = parse_conf_word(buf, p0, sizeof(p0), 256);
    if (mode >= 0 && mode <= BK_0011M) {
        g_conf.bk0010mode = (bk_mode_t)mode;
    }
    const char p1[] = "is_covox_on:";
    mode = parse_conf_word(buf, p1, sizeof(p1), 256);
    if (mode >= 0 && mode <= 1) {
        g_conf.is_covox_on = (bool)mode;
    }
    const char p2[] = "is_AY_on:";
    mode = parse_conf_word(buf, p2, sizeof(p2), 256);
    if (mode >= 0 && mode <= 1) {
        g_conf.is_AY_on = (bool)mode;
    }
    const char p3[] = "color_mode:";
    mode = parse_conf_word(buf, p3, sizeof(p3), 256);
    if (mode >= 0 && mode <= 1) {
        g_conf.color_mode = (bool)mode;
    }
    const char p4[] = "snd_volume:";
    mode = parse_conf_word(buf, p4, sizeof(p4), 256);
    if (mode >= -16 && mode <= 5) {
        g_conf.snd_volume = mode;
    }
    const char p5[] = "graphics_pallette_idx:";
    mode = parse_conf_word(buf, p5, sizeof(p5), 256);
    if (mode >= 0 && mode <= 15) {
        g_conf.graphics_pallette_idx = mode;
    }
    const char p6[] = "is_swap_wins_enabled:";
    mode = parse_conf_word(buf, p6, sizeof(p6), 256);
    if (mode >= 0 && mode <= 1) {
        is_swap_wins_enabled = (bool)mode;
    }
    const char p7[] = "is_dendy_joystick:";
    mode = parse_conf_word(buf, p7, sizeof(p7), 256);
    if (mode >= 0 && mode <= 1) {
        is_dendy_joystick = (bool)mode;
    }
    const char p8[] = "is_kbd_joystick:";
    mode = parse_conf_word(buf, p8, sizeof(p8), 256);
    if (mode >= 0 && mode <= 1) {
        is_kbd_joystick = (bool)mode;
    }
    f_close(&fil);
}

static void init_fs() {
    FRESULT result = f_mount(&fatfs, "", 1);
    if (FR_OK != result) {
        printf("Unable to mount SD-card: %s (%d)", FRESULT_str(result), result);
    } else {
        SD_CARD_AVAILABLE = true;
    }

    if (SD_CARD_AVAILABLE) {
///        DIR dir;
///        if (f_opendir(&dir, "\\BK") != FR_OK) {
///            f_mkdir("\\BK");
///        } else {
///            f_closedir(&dir);
///        }
    //    insertdisk(0, fdd0_sz(), fdd0_rom(), "\\BK\\fdd0.img");
    //    insertdisk(1, fdd1_sz(), fdd1_rom(), "\\BK\\fdd1.img"); // TODO: why not attached?
    //    insertdisk(2, 819200, 0, "\\BK\\hdd0.img");
    //    insertdisk(3, 819200, 0, "\\BK\\hdd1.img"); // TODO: why not attached?
///        read_config("\\BK\\bk.conf");
    }
}

int main() {
#if (OVERCLOCKING > 270)
    hw_set_bits(&vreg_and_chip_reset_hw->vreg, VREG_AND_CHIP_RESET_VREG_VSEL_BITS);
    sleep_ms(33);
    set_sys_clock_khz(OVERCLOCKING * 1000, true);
#else
    vreg_set_voltage(VREG_VOLTAGE_1_15);
    sleep_ms(33);
    set_sys_clock_khz(270000, true);
#endif
    PWM_init_pin(BEEPER_PIN, (1 << 12) - 1);
#ifdef SOUND_SYSTEM
    PWM_init_pin(PWM_PIN0, (1 << 12) - 1);
    PWM_init_pin(PWM_PIN1, (1 << 12) - 1);
#endif

#if LOAD_WAV_PIO
    //пин ввода звука
    inInit(LOAD_WAV_PIO);
#endif

    TRACE_T(("gpio_init(PICO_DEFAULT_LED_PIN)"));
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    for (int i = 0; i < 6; i++) {
        sleep_ms(23);
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(23);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
    }
    TRACE_T(("Before Init_Wii_Joystick"));
    #ifdef USE_WII
    if (!Init_Wii_Joystick()) {
    #endif
        #ifdef USE_NESPAD
        TRACE_T(("Before nespad_begin"));
        nespad_begin(clock_get_hz(clk_sys) / 1000, NES_GPIO_CLK, NES_GPIO_DATA, NES_GPIO_LAT);
        #endif
    #ifdef USE_WII
    } else {
        add_repeating_timer_ms(60, Wii_Joystick_Timer_CB, NULL, &Wii_timer);
    }
    #endif
    TRACE_T(("Before keyboard_init"));
    keyboard_init();

    sem_init(&vga_start_semaphore, 0, 1);
    multicore_launch_core1(render_core);
    sem_release(&vga_start_semaphore);

    sleep_ms(50);

    memset(TEXT_VIDEO_RAM, 0, sizeof TEXT_VIDEO_RAM);

//    init_psram();
    init_fs();
    reset(11);
    graphics_set_mode(g_conf.color_mode ? BK_256x256x2 : BK_512x256x1);

#ifdef SOUND_SYSTEM
	int hz = 44100;	//44000 //44100 //96000 //22050
	// negative timeout means exact delay (rather than delay between callbacks)
	if (!add_repeating_timer_us(-1000000 / hz, AY_timer_callback, NULL, &timer)) {
		TRACE_T("Failed to add AY_timer_callback timer");
	}
#endif
    g_Config.InitConfig(CString("bk.ini"));
    g_Config.VerifyRoms(); // проверим наличие, но продолжим выполнение при отсутствии чего-либо
    CMainFrame *mf = new CMainFrame(new CScreen());
    CMotherBoard *m_pBoard = new CMotherBoard();
    CSpeaker m_speaker;
    CBkSound *m_pSound = new CBkSound();
    int nMtc = m_pSound->ReInit(g_Config.m_nSoundSampleRate); // пересоздаём звук с новыми параметрами, на выходе - длина медиафрейма в сэмплах
	m_speaker.ReInit();     // ещё надо переинициализирвоать устройства, там
	m_speaker.ConfigureTapeBuffer(nMtc);// переопределяем буферы в зависимости от текущей частоты дискретизации
    CCovox m_covox;
	m_covox.ReInit();       // есть вещи, зависящие от частоты дискретизации,
///    CMenestrel m_menestrel;
///	m_menestrel.ReInit();   //
///    CAYSnd m_aySnd;
///	m_aySnd.ReInit();       // которая теперь величина переменная. Но требует перезапуска конфигурации.
///	m_paneOscillatorView.ReCreateOSC(); // пересоздаём осциллограф с новыми параметрами
	// при необходимости откорректируем размер приёмного буфера.
///	m_paneOscillatorView.SetBuffer(nMtc); //SendMessage(WM_OSC_SETBUFFER, WPARAM(nMtc));
	m_pBoard->SetFDDType(g_Config.m_BKFDDModel);
	// присоединим устройства, чтобы хоть что-то было для выполнения ResetHot
	m_pBoard->AttachWindow(mf);  // цепляем к MotherBoard этот класс
	// порядок имеет значение. сперва нужно делать обязательно AttachWindow(this)
	// и только затем m_pBoard->SetMTC(). И эта функция обязательна, там звуковой буфер вычисляется
	// и выделяется
	m_pBoard->SetMTC(nMtc); // и здесь ещё. тройная работа получается.
	// Присоединяем к новосозданному чипу устройства
	m_pBoard->AttachSound(m_pSound);
	m_pBoard->AttachSpeaker(&m_speaker);
///	m_pBoard->AttachMenestrel(&m_menestrel);
	m_pBoard->AttachCovox(&m_covox);
///	m_pBoard->AttachAY8910(&m_aySnd);
	// если в ини файле задана частота, то применим её, вместо частоты по умолчанию.
	m_pBoard->NormalizeCPU();
	// Цепляем к новому чипу отладчик, т.е. наоборот, к отладчику чип
///	m_pDebugger->AttachBoard(GetBoard());
	//	m_paneRegistryDumpViewCPU.SetFreqParam();
	// Цепляем обработчик скриптов
///	m_Script.AttachBoard(GetBoard());
	if (m_pBoard->InitBoard(g_Config.m_nCPURunAddr)) {
		m_pBoard->StartTimerThread();
		m_pBoard->RunCPU();
      //  if (!add_repeating_timer_ms(20, main_timer_callback, m_pBoard, &main_timer)) {
	//	    TRACE_T("Failed to add main_timer_callback timer");
	 //   }
        m_pBoard->TimerThreadFunc();
    }
    while (1) {
        sleep_ms(20);
    }
    return 0;
}
