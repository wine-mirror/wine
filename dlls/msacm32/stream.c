/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 */

#include "winbase.h"
#include "winerror.h"
#include "windef.h"
#include "debugtools.h"
#include "mmsystem.h"
#include "msacm.h"
#include "msacmdrv.h"

DEFAULT_DEBUG_CHANNEL(msacm)

/***********************************************************************
 *           acmStreamClose (MSACM32.37)
 */
MMRESULT WINAPI acmStreamClose(
  HACMSTREAM has, DWORD fdwClose)
{
  FIXME("(0x%08x, %ld): stub\n", has, fdwClose);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmStreamConvert (MSACM32.38)
 */
MMRESULT WINAPI acmStreamConvert(
  HACMSTREAM has, PACMSTREAMHEADER pash, DWORD fdwConvert)
{
  FIXME("(0x%08x, %p, %ld): stub\n", has, pash, fdwConvert);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmStreamMessage (MSACM32.39)
 */
MMRESULT WINAPI acmStreamMessage(
  HACMSTREAM has, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
  FIXME("(0x%08x, %u, %ld, %ld): stub\n",
    has, uMsg, lParam1, lParam2
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmStreamOpen (MSACM32.40)
 */
MMRESULT WINAPI acmStreamOpen(
  PHACMSTREAM phas, HACMDRIVER had, PWAVEFORMATEX pwfxSrc,
  PWAVEFORMATEX pwfxDst, PWAVEFILTER pwfltr, DWORD dwCallback,
  DWORD dwInstance, DWORD fdwOpen)
{
  FIXME("(%p, 0x%08x, %p, %p, %p, %ld, %ld, %ld): stub\n",
    phas, had, pwfxSrc, pwfxDst, pwfltr,
    dwCallback, dwInstance, fdwOpen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}


/***********************************************************************
 *           acmStreamPrepareHeader (MSACM32.41)
 */
MMRESULT WINAPI acmStreamPrepareHeader(
  HACMSTREAM has, PACMSTREAMHEADER pash, DWORD fdwPrepare)
{
  FIXME("(0x%08x, %p, %ld): stub\n", has, pash, fdwPrepare);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmStreamReset (MSACM32.42)
 */
MMRESULT WINAPI acmStreamReset(
  HACMSTREAM has, DWORD fdwReset)
{
  FIXME("(0x%08x, %ld): stub\n", has, fdwReset);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmStreamSize (MSACM32.43)
 */
MMRESULT WINAPI acmStreamSize(
  HACMSTREAM has, DWORD cbInput, 
  LPDWORD pdwOutputBytes, DWORD fdwSize)
{
  FIXME("(0x%08x, %ld, %p, %ld): stub\n",
    has, cbInput, pdwOutputBytes, fdwSize
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmStreamUnprepareHeader (MSACM32.44)
 */
MMRESULT WINAPI acmStreamUnprepareHeader(
  HACMSTREAM has, PACMSTREAMHEADER pash, DWORD fdwUnprepare)
{
  FIXME("(0x%08x, %p, %ld): stub\n",
    has, pash, fdwUnprepare
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}
