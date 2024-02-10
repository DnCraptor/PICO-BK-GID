#include "CPSRAM.h"
#include "debug.h"

extern "C" {
    #include "psram_spi.h"
}

uint8_t PSRAM::m_screen[SCREEN_RAM_SZ] = { 0 };

void PSRAM::set8(size_t addr, uint8_t val) {
    write8psram(addr, val);
    if (addr >= m_offset && addr < m_offset + SCREEN_RAM_SZ) {
        m_screen[addr - m_offset] = val;
    }
}

void PSRAM::set16(size_t addr, uint16_t val) {
    write16psram(addr, val);
    if (addr >= m_offset && addr < m_offset + SCREEN_RAM_SZ) {
        *(uint16_t*)&m_screen[addr - m_offset] = val;
    }
}

uint8_t PSRAM::get8(size_t addr) {
    if (addr >= m_offset && addr < m_offset + SCREEN_RAM_SZ) {
        return m_screen[addr - m_offset];
    }
    return read8psram(addr);
}

uint16_t PSRAM::get16(size_t addr) {
    if (addr >= m_offset && addr < m_offset + SCREEN_RAM_SZ) {
        return *(uint16_t*)&m_screen[addr - m_offset];
    }
    return read16psram(addr);
}

void PSRAM::clear() {
    for (size_t addr = 0; addr < m_size; addr += 2) {
        set16(addr, 0);
    }
}

uint8_t* PSRAM::screen_base(size_t offset) {
    if (m_offset != offset) {
        TRACE_T("PSRAM::screen_base(%08Xh) m_offset: %08Xh", offset, m_offset);
        m_offset = offset;
        for (size_t addr = offset; addr < offset + SCREEN_RAM_SZ; addr += 2) {
            m_screen[addr - offset] = get16(addr);
        }
    }
    return m_screen;
}
