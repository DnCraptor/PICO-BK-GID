// Device.h: interface for the CDevice class.
//
#pragma once
#include <winnt.h>
// флаги операций для функций обащения к системным регистрам
#define GSSR_NONE 0
#define GSSR_BYTEOP 1
#define GSSR_INTERNAL 2


class CDevice : public CObject
{
	protected:
		ULONGLONG           m_tickCount; // Device ticks
		CDevice            *m_pParent;

	public:
		CDevice();
		virtual ~CDevice() override;

		void AttachParent(CDevice *pParent)
		{
			m_pParent = pParent;
		};
		// Method for count device ticks
		void                Reset();
		virtual void        NextTick();

		// Virtual method called after reset command
		virtual void        OnReset() = 0;

		// Methods for Set/Get byte/word
		virtual void        GetByte(const uint16_t addr, uint8_t *pValue) = 0;
		virtual void        GetWord(const uint16_t addr, uint16_t *pValue) = 0;
		virtual void        SetByte(const uint16_t addr, uint8_t value) = 0;
		virtual void        SetWord(const uint16_t addr, uint16_t value) = 0;
};

