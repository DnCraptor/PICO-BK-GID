#pragma once

#include <inttypes.h>
#include <string.h>

#define SCREEN_RAM_SZ (16 << 10)

class PSRAM {
	    size_t m_size;
		size_t m_offset;
		static uint8_t m_screen[SCREEN_RAM_SZ];
	public:
	    PSRAM() : m_size(0), m_offset(16 << 10) {}
	    void set8(size_t addr, uint8_t val);
		void set16(size_t addr, uint16_t val);
		uint8_t get8(size_t addr);
		uint16_t get16(size_t addr);
		void clear();
		void resize(size_t s) { m_size = s; }
		uint8_t* screen_base(size_t offset);
};
