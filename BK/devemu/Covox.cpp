// Covox.cpp: implementation of the CCovox class.
//


#include "pch.h"
#include "Config.h"
#include "Covox.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction


CCovox::CCovox()
{
	if (CreateFIRBuffers(FIR_LENGTH))
	{
		ReInit();
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}
}

CCovox::~CCovox()
    = default;

void CCovox::ReInit()
{
	double w0 = 2 * 11000.0 / double(g_Config.m_nSoundSampleRate);
	double w1 = 0.0;
	int res = fir_linphase(m_nFirLength, w0, w1, FIR_FILTER::LOWPASS,
	                       FIR_WINDOW::BLACKMAN_HARRIS, true, 0.0, m_pH.get());
}

void CCovox::SetData(uint16_t inVal)
{
	if (m_bEnableSound)
	{
		m_dAccL  = double(LOBYTE(inVal)) / 256.0;
		m_dAccR = m_bStereo ? double(HIBYTE(inVal)) / 256.0 : m_dAccL;
	}
	else
	{
		m_dAccL = m_dAccR = 0.0;
	}
}

void CCovox::GetSample(sOneSample *pSm)
{
	SAMPLE_INT l = m_dAccL;
	SAMPLE_INT r = m_dAccR;

	if (m_bDCOffset)
	{
		l = DCOffset(l, m_dAvgL, m_pdDCBufL.get(), m_nDCBufPosL);
		r = DCOffset(r, m_dAvgR, m_pdDCBufR.get(), m_nDCBufPosR);
	}

	pSm->s[OSL] = FIRFilter(l, m_pdFBufL.get(), m_nFBufPosL);
	pSm->s[OSR] = FIRFilter(r, m_pdFBufR.get(), m_nFBufPosR);
}
