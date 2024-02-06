#pragma once
#include <winnt.h>
#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_

#define WAVE_FORMAT_PCM 1
#define MAXINT32 0xFFFFFFFF

/*
 *  extended waveform format structure used for all non-PCM formats. this
 *  structure is common to all non-PCM formats.
 */
typedef struct tWAVEFORMATEX
{
    WORD        wFormatTag;         /* format type */
    WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
    DWORD       nSamplesPerSec;     /* sample rate */
    DWORD       nAvgBytesPerSec;    /* for buffer estimation */
    WORD        nBlockAlign;        /* block size of data */
    WORD        wBitsPerSample;     /* number of bits per sample of mono data */
    WORD        cbSize;             /* the count in bytes of the size of */
                                    /* extra information (after cbSize) */
} WAVEFORMATEX, *PWAVEFORMATEX, NEAR *NPWAVEFORMATEX, FAR *LPWAVEFORMATEX;

#endif /* _WAVEFORMATEX_ */
typedef const WAVEFORMATEX FAR *LPCWAVEFORMATEX;

class WAVEHDR{};
DECLARE_HANDLE(HWAVEOUT);
#define CALLBACK
#include "CFile.h"

#define WINMMAPI 
#define WINAPI
#define _Return_type_success_(X)
typedef _Return_type_success_( return == 0) UINT        MMRESULT;   /* error return code, 0 means no error */
#define _In_opt_
#define _In_
#define _Out_

WINMMAPI
MMRESULT
WINAPI
waveOutSetVolume(
    _In_opt_ HWAVEOUT hwo,
    _In_ DWORD dwVolume
    );

WINMMAPI
MMRESULT
WINAPI
waveOutGetVolume(
    _In_opt_ HWAVEOUT hwo,
    _Out_ LPDWORD pdwVolume
    );
