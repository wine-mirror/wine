/*
 *      mmreg.h   -       Declarations for ???
 */

#include "wintypes.h"

/***********************************************************************
 * Defines/Enums
 */

#define WAVE_FILTER_UNKNOWN     0x0000
#define WAVE_FILTER_DEVELOPMENT 0xFFFF

/***********************************************************************
 * Structures
 */

typedef struct _WAVEFILTER {
  DWORD   cbStruct;
  DWORD   dwFilterTag;
  DWORD   fdwFilter;
  DWORD   dwReserved[5];
} WAVEFILTER16, *PWAVEFILTER16, *NPWAVEFILTER16, *LPWAVEFILTER16,
  WAVEFILTER32, *PWAVEFILTER32;

typedef struct _WAVEFORMATEX {
  WORD   wFormatTag;
  WORD   nChannels;
  DWORD  nSamplesPerSec;
  DWORD  nAvgBytesPerSec;
  WORD   nBlockAlign;
  WORD   wBitsPerSample;
  WORD   cbSize;
} WAVEFORMATEX16, *PWAVEFORMATEX16, *NWAVEFORMATEX16, *LPWAVEFORMATEX16, 
  WAVEFORMATEX32, *PWAVEFORMATEX32;


