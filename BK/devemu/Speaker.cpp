// Speaker.cpp: implementation of the CSpeaker class.
//
#include "pch.h"
#include "Config.h"
#include "Speaker.h"

#ifdef ORIG_SPEAKER
/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction
CSpeaker::CSpeaker()
	: m_pReceiveTapeSamples(nullptr)
	, m_nAverage(0.0)
	, m_tickCount(0)
	, m_ips(0)
{
	SetFCFilterValue(6.8e-9 * 8200);

	if (CreateFIRBuffers(FIR_LENGTH))
	{
		ReInit();
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}
}

CSpeaker::~CSpeaker()
{
}

void CSpeaker::ReInit()
{
	// нормализация частоты среза: w = fs / (Fd/2)
	// как на самом деле должно быть, в libdspl-2.0 не описано, но эта формула
	// на слух даёт примерно подходящий результат
	double w0 = 2 * 8500.0 / double(g_Config.m_nSoundSampleRate);
	double w1 = 0.0;
	int res = fir_linphase(m_nFirLength, w0, w1, FIR_FILTER::LOWPASS,
	                       FIR_WINDOW::BLACKMAN_HARRIS, true, 0.0, m_pH.get());
}

void CSpeaker::Reset()
{
	m_tickCount = 0;
}

void CSpeaker::ConfigureTapeBuffer(int ips)
{
	m_ips = ips;

	if (m_pReceiveTapeSamples)
	{
		m_pReceiveTapeSamples.reset();
	}

	m_pReceiveTapeSamples = std::make_unique<SAMPLE_INT[]>(m_ips);

	if (m_pReceiveTapeSamples)
	{
		ZeroMemory(m_pReceiveTapeSamples.get(), m_ips * SAMPLE_INT_SIZE);
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}
}

/*
Вход: pBuff - указатель на общий буфер, данные там типа SAMPLE_INT
      nSampleLen - длина буфера pBuff в cэмплах
*/
void CSpeaker::ReceiveTapeBuffer(void *pBuff, int nSampleLen)
{
	ASSERT(m_ips == nSampleLen); // m_ips == должно быть размер в сэмплах!
	Reset(); // обнуляем TickCounter,
	auto inBuf = reinterpret_cast<SAMPLE_INT *>(pBuff);
	SAMPLE_INT avg = 0.0; // инт не хватает, случается переполнение

	// Заполняем из входного буфера внутренний. из стерео делаем моно.
	for (int n = 0; n < m_ips; ++n)
	{
		SAMPLE_INT l = *inBuf++;
		SAMPLE_INT r = *inBuf++;
		SAMPLE_INT t = (l + r) / 2.0; // микшируем

		if (m_bDCOffset)
		{
			// для воспроизведения используем буфер левого канала,
			// для приёма с ленты - используем буфер правого канала
			t = DCOffset(t, m_dAvgR, m_pdDCBufR.get(), m_nDCBufPosR); // на выходе m_nBufferPosR указывает на следующую позицию.
		}

		m_pReceiveTapeSamples[n] = t;
		avg += t;
	}

	m_nAverage = avg / SAMPLE_INT(m_ips);
}


bool CSpeaker::GetTapeSample()
{
	return (m_pReceiveTapeSamples[m_tickCount++] > m_nAverage);
}

/*
 Спикер использует только верхний полупериод сэмпла
 Посмотрев амплитуды звуков БК и эмулятора, у меня закрадывается подозрение,
 что нужно использовать преобразование ШИМ->АМ или как-то иначе эмулировать
 поведение конденсатора.
 Значения амплитуд неточные, измерены на глаз.
*/
const SAMPLE_INT CSpeaker::m_dSpeakerValues[8] =
{
	SAMPLE_INT(MIN_SAMPLE) * 0.00 / FLOAT_BASE, // 0 0   0%
	SAMPLE_INT(MAX_SAMPLE) * 0.15 / FLOAT_BASE, // 1 4   15%
	SAMPLE_INT(MAX_SAMPLE) * 0.30 / FLOAT_BASE, // 2 40  30%
	SAMPLE_INT(MAX_SAMPLE) * 0.40 / FLOAT_BASE, // 3 44  40%
	SAMPLE_INT(MAX_SAMPLE) * 0.70 / FLOAT_BASE, // 4 100 70%
	SAMPLE_INT(MAX_SAMPLE) * 0.80 / FLOAT_BASE, // 5 104 80%
	SAMPLE_INT(MAX_SAMPLE) * 0.94 / FLOAT_BASE, // 6 140 94%
	SAMPLE_INT(MAX_SAMPLE) * 1.00 / FLOAT_BASE  // 7 144 100%
};

void CSpeaker::SetData(uint16_t inVal)
{
	if (m_bEnableSound)
	{
		int w = inVal & 0144; // получим маску звуковых битов

		if (w & 04) // 2й бит переместим в 4й
		{
			w |= 020;
		}

		w >>= 4; // сдвинем на 4 бита вправо. в результате получится число 0..7
		m_dAccL = m_dSpeakerValues[w];
	}
	else
	{
		m_dAccL = 0.0;
	}
}

void CSpeaker::GetSample(sOneSample *pSm)
{
	SAMPLE_INT s = m_bFilter ? RCFilterCalc(m_RCFL) : m_dAccL;

	if (m_bDCOffset)
	{
		// для воспроизведения используем буфер левого канала,
		// для приёма с ленты - используем буфер правого канала
		s = DCOffset(s, m_dAvgL, m_pdDCBufL.get(), m_nDCBufPosL); // на выходе m_nBufferPosL указывает на следующую позицию.
	}

	// фильтр
	pSm->s[OSR] = pSm->s[OSL] = FIRFilter(s, m_pdFBufL.get(), m_nFBufPosL);
}

#else
CSpeaker::CSpeaker() {
	SetFCFilterValue(6.8e-9 * 8200);
	if (CreateFIRBuffers(FIR_LENGTH)) {
		ReInit();
	} else {
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}
}

CSpeaker::~CSpeaker() {
}

void CSpeaker::ReInit() {
	// нормализация частоты среза: w = fs / (Fd/2)
	// как на самом деле должно быть, в libdspl-2.0 не описано, но эта формула
	// на слух даёт примерно подходящий результат
	double w0 = 2 * 8500.0 / double(g_Config.m_nSoundSampleRate);
	double w1 = 0.0;
	int res = fir_linphase(m_nFirLength, w0, w1, FIR_FILTER::LOWPASS,
	                       FIR_WINDOW::BLACKMAN_HARRIS, true, 0.0, m_pH.get());
}

void CSpeaker::Reset() {
///	m_tickCount = 0;
}

void CSpeaker::ConfigureTapeBuffer(int ips) {
	TRACE_T("CSpeaker::ConfigureTapeBuffer(%d)", ips);
}

/*
Вход: pBuff - указатель на общий буфер, данные там типа SAMPLE_INT
      nSampleLen - длина буфера pBuff в cэмплах
*/
void CSpeaker::ReceiveTapeBuffer(void *pBuff, int nSampleLen) {
	TRACE_T("CSpeaker::ReceiveTapeBuffer(%d)", nSampleLen);
}


bool CSpeaker::GetTapeSample() {
	TRACE_T("CSpeaker::GetTapeSample");
	return false;
}

void CSpeaker::SetData(uint16_t inVal) {
	if (m_bEnableSound) {
		int w = inVal & 0144; // получим маску звуковых битов
		if (w & 04) // 2й бит переместим в 4й
		{
			w |= 020;
		}
		w >>= 4; // сдвинем на 4 бита вправо. в результате получится число 0..7
		m_dAccL = w; /// m_dSpeakerValues[w];
		TRACE_T("CSpeaker::SetData(%d), m_dAccL: %d", inVal, m_dAccL);
	} else {
		m_dAccL = 0.0;
		TRACE_T("CSpeaker::SetData(%d), m_dAccL: %d", inVal, m_dAccL);
	}
}

void CSpeaker::GetSample(sOneSample *pSm) {
	TRACE_T("CSpeaker::GetSample(%08Xh)", pSm);
	SAMPLE_INT s = m_bFilter ? RCFilterCalc(m_RCFL) : m_dAccL;
	if (m_bDCOffset) {
		// для воспроизведения используем буфер левого канала,
		// для приёма с ленты - используем буфер правого канала
		s = DCOffset(s, m_dAvgL, m_pdDCBufL.get(), m_nDCBufPosL); // на выходе m_nBufferPosL указывает на следующую позицию.
	}
	// фильтр
	pSm->s[OSR] = pSm->s[OSL] = FIRFilter(s, m_pdFBufL.get(), m_nFBufPosL);
}
#endif
