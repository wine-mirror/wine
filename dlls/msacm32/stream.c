/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 */

#include "windows.h"
#include "winerror.h"
#include "wintypes.h"
#include "debug.h"
#include "mmsystem.h"
#include "msacm.h"
#include "msacmdrv.h"

/***********************************************************************
 *           acmStreamClose (MSACM32.37)
 */
MMRESULT32 WINAPI acmStreamClose32(
  HACMSTREAM32 has, DWORD fdwClose)
{
  FIXME(msacm, "(0x%08x, %ld): stub\n", has, fdwClose);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmStreamConvert (MSACM32.38)
 */
MMRESULT32 WINAPI acmStreamConvert32(
  HACMSTREAM32 has, PACMSTREAMHEADER32 pash, DWORD fdwConvert)
{
  FIXME(msacm, "(0x%08x, %p, %ld): stub\n", has, pash, fdwConvert);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmStreamMessage (MSACM32.39)
 */
MMRESULT32 WINAPI acmStreamMessage32(
  HACMSTREAM32 has, UINT32 uMsg, LPARAM lParam1, LPARAM lParam2)
{
  FIXME(msacm, "(0x%08x, %u, %ld, %ld): stub\n",
    has, uMsg, lParam1, lParam2
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmStreamOpen (MSACM32.40)
 */
MMRESULT32 WINAPI acmStreamOpen32(
  PHACMSTREAM32 phas, HACMDRIVER32 had, PWAVEFORMATEX32 pwfxSrc,
  PWAVEFORMATEX32 pwfxDst, PWAVEFILTER32 pwfltr, DWORD dwCallback,
  DWORD dwInstance, DWORD fdwOpen)
{
  FIXME(msacm, "(%p, 0x%08x, %p, %p, %p, %ld, %ld, %ld): stub\n",
    phas, had, pwfxSrc, pwfxDst, pwfltr,
    dwCallback, dwInstance, fdwOpen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}


/***********************************************************************
 *           acmStreamPrepareHeader (MSACM32.41)
 */
MMRESULT32 WINAPI acmStreamPrepareHeader32(
  HACMSTREAM32 has, PACMSTREAMHEADER32 pash, DWORD fdwPrepare)
{
  FIXME(msacm, "(0x%08x, %p, %ld): stub\n", has, pash, fdwPrepare);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmStreamReset (MSACM32.42)
 */
MMRESULT32 WINAPI acmStreamReset32(
  HACMSTREAM32 has, DWORD fdwReset)
{
  FIXME(msacm, "(0x%08x, %ld): stub\n", has, fdwReset);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmStreamSize (MSACM32.43)
 */
MMRESULT32 WINAPI acmStreamSize32(
  HACMSTREAM32 has, DWORD cbInput, 
  LPDWORD pdwOutputBytes, DWORD fdwSize)
{
  FIXME(msacm, "(0x%08x, %ld, %p, %ld): stub\n",
    has, cbInput, pdwOutputBytes, fdwSize
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmStreamUnprepareHeader (MSACM32.44)
 */
MMRESULT32 WINAPI acmStreamUnprepareHeader32(
  HACMSTREAM32 has, PACMSTREAMHEADER32 pash, DWORD fdwUnprepare)
{
  FIXME(msacm, "(0x%08x, %p, %ld): stub\n",
    has, pash, fdwUnprepare
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}
