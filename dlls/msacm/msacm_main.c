/*
 *      MSACM library
 *
 *      Copyright 1998  Patrik Stridvall
 */

#include "winbase.h"
#include "winerror.h"
#include "msacm.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(msacm)

/***********************************************************************
 *           ACMGETVERSION (MSACM.7)
 */
DWORD WINAPI acmGetVersion16()
{
  FIXME(msacm, "(): stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0; /* FIXME */
}

/***********************************************************************
 *           ACMMETRICS (MSACM.8)
 */

MMRESULT16 WINAPI acmMetrics16(
  HACMOBJ16 hao, UINT16 uMetric, LPVOID pMetric)
{
  FIXME(msacm, "(0x%04x, %d, %p): stub\n", hao, uMetric, pMetric);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMDRIVERENUM (MSACM.10)
 */
MMRESULT16 WINAPI acmDriverEnum16(
  ACMDRIVERENUMCB16 fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
  FIXME(msacm, "(%p, %ld, %ld): stub\n",
    fnCallback, dwInstance, fdwEnum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMDRIVERDETAILS (MSACM.11)
 */

MMRESULT16 WINAPI acmDriverDetails16(
  HACMDRIVERID16 hadid, LPACMDRIVERDETAILS16 padd, DWORD fdwDetails)
{
  FIXME(msacm, "(0x%04x, %p, %ld): stub\n", hadid, padd, fdwDetails);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMDRIVERADD (MSACM.12)
 */
MMRESULT16 WINAPI acmDriverAdd16(
  LPHACMDRIVERID16 phadid, HINSTANCE16 hinstModule,
  LPARAM lParam, DWORD dwPriority, DWORD fdwAdd)
{
  FIXME(msacm, "(%p, 0x%04x, %ld, %ld, %ld): stub\n",
    phadid, hinstModule, lParam, dwPriority, fdwAdd
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMDRIVERREMOVE (MSACM.13)
 */
MMRESULT16 WINAPI acmDriverRemove16(
  HACMDRIVERID16 hadid, DWORD fdwRemove)
{
  FIXME(msacm, "(0x%04x, %ld): stub\n", hadid, fdwRemove);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMDRIVEROPEN (MSACM.14)
 */
MMRESULT16 WINAPI acmDriverOpen16(
  LPHACMDRIVER16 phad, HACMDRIVERID16 hadid, DWORD fdwOpen)
{
  FIXME(msacm, "(%p, 0x%04x, %ld): stub\n", phad, hadid, fdwOpen);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMDRIVERCLOSE (MSACM.15)
 */
MMRESULT16 WINAPI acmDriverClose16(
  HACMDRIVER16 had, DWORD fdwClose)
{
  FIXME(msacm, "(0x%04x, %ld): stub\n", had, fdwClose);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMDRIVERMESSAGE (MSACM.16)
 */
LRESULT WINAPI acmDriverMessage16(
  HACMDRIVER16 had, UINT16 uMsg, LPARAM lParam1, LPARAM lParam2)
{
  FIXME(msacm, "(0x%04x, %d, %ld, %ld): stub\n",
    had, uMsg, lParam1, lParam2
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ACMDRIVERID (MSACM.17)
 */
MMRESULT16 WINAPI acmDriverID16(
  HACMOBJ16 hao, LPHACMDRIVERID16 phadid, DWORD fdwDriverID)
{
  FIXME(msacm, "(0x%04x, %p, %ld): stub\n", hao, phadid, fdwDriverID);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMDRIVERPRIORITY (MSACM.18)
 */
MMRESULT16 WINAPI acmDriverPriority16(
 HACMDRIVERID16 hadid, DWORD dwPriority, DWORD fdwPriority)
{
  FIXME(msacm, "(0x%04x, %ld, %ld): stub\n",
    hadid, dwPriority, fdwPriority
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMFORMATTAGDETAILS (MSACM.30)
 */
MMRESULT16 WINAPI acmFormatTagDetails16(
  HACMDRIVER16 had, LPACMFORMATTAGDETAILS16 paftd, DWORD fdwDetails)
{
  FIXME(msacm, "(0x%04x, %p, %ld): stub\n", had, paftd, fdwDetails);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMFORMATTAGENUM (MSACM.31)
 */
MMRESULT16 WINAPI acmFormatTagEnum16(
  HACMDRIVER16 had, LPACMFORMATTAGDETAILS16 paftd,
  ACMFORMATTAGENUMCB16 fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
  FIXME(msacm, "(0x%04x, %p, %p, %ld, %ld): stub\n",
    had, paftd, fnCallback, dwInstance, fdwEnum
  ); 
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMFORMATCHOOSE (MSACM.40)
 */
MMRESULT16 WINAPI acmFormatChoose16(
  LPACMFORMATCHOOSE16 pafmtc)
{
  FIXME(msacm, "(%p): stub\n", pafmtc);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMFORMATDETAILS (MSACM.41)
 */
MMRESULT16 WINAPI acmFormatDetails16(
  HACMDRIVER16 had, LPACMFORMATDETAILS16 pafd, DWORD fdwDetails)
{
  FIXME(msacm, "(0x%04x, %p, %ld): stub\n", had, pafd, fdwDetails);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMFORMATENUM (MSACM.42)
 */
MMRESULT16 WINAPI acmFormatEnum16(
  HACMDRIVER16 had, LPACMFORMATDETAILS16 pafd,
  ACMFORMATENUMCB16 fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
  FIXME(msacm, "(0x%04x, %p, %p, %ld, %ld): stub\n",
    had, pafd, fnCallback, dwInstance, fdwEnum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMFORMATSUGGEST (MSACM.45)
 */
MMRESULT16 WINAPI acmFormatSuggest16(
  HACMDRIVER16 had, LPWAVEFORMATEX pwfxSrc, 
  LPWAVEFORMATEX pwfxDst, DWORD cbwfxDst, DWORD fdwSuggest)
{
  FIXME(msacm, "(0x%04x, %p, %p, %ld, %ld): stub\n",
    had, pwfxSrc, pwfxDst, cbwfxDst, fdwSuggest
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMFILTERTAGDETAILS (MSACM.50)
 */
MMRESULT16 WINAPI acmFilterTagDetails16(
  HACMDRIVER16 had, LPACMFILTERTAGDETAILS16 paftd, DWORD fdwDetails)
{
  FIXME(msacm, "(0x%04x, %p, %ld): stub\n", had, paftd, fdwDetails);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMFILTERTAGENUM (MSACM.51)
 */
MMRESULT16 WINAPI acmFilterTagEnum16(
  HACMDRIVER16 had, LPACMFILTERTAGDETAILS16 paftd,
  ACMFILTERTAGENUMCB16 fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
  FIXME(msacm, "(0x%04x, %p, %p, %ld, %ld): stub\n",
    had, paftd, fnCallback, dwInstance, fdwEnum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMFILTERCHOOSE (MSACM.60)
 */
MMRESULT16 WINAPI acmFilterChoose16(
  LPACMFILTERCHOOSE16 pafltrc)
{
  FIXME(msacm, "(%p): stub\n", pafltrc);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMFILTERDETAILS (MSACM.61)
 */
MMRESULT16 WINAPI acmFilterDetails16(
  HACMDRIVER16 had, LPACMFILTERDETAILS16 pafd, DWORD fdwDetails)
{
  FIXME(msacm, "(0x%04x, %p, %ld): stub\n", had, pafd, fdwDetails);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMFILTERENUM (MSACM.62)
 */
MMRESULT16 WINAPI acmFilterEnum16(
  HACMDRIVER16 had, LPACMFILTERDETAILS16 pafd,
  ACMFILTERENUMCB16 fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
  FIXME(msacm, "(0x%04x, %p, %p, %ld, %ld): stub\n",
    had, pafd, fnCallback, dwInstance, fdwEnum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMSTREAMOPEN (MSACM.70)
 */
MMRESULT16 WINAPI acmStreamOpen16(
  LPHACMSTREAM16 phas, HACMDRIVER16 had,
  LPWAVEFORMATEX pwfxSrc, LPWAVEFORMATEX pwfxDst,
  LPWAVEFILTER pwfltr, DWORD dwCallback,
  DWORD dwInstance, DWORD fdwOpen)
{
  FIXME(msacm, "(%p, 0x%04x, %p, %p, %p, %ld, %ld, %ld): stub\n",
    phas, had, pwfxSrc, pwfxDst, pwfltr,
    dwCallback, dwInstance, fdwOpen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMSTREAMCLOSE (MSACM.71)
 */
MMRESULT16 WINAPI acmStreamClose16(
  HACMSTREAM16 has, DWORD fdwClose)
{
  FIXME(msacm, "(0x%04x, %ld): stub\n", has, fdwClose);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMSTREAMSIZE (MSACM.72)
 */
MMRESULT16 WINAPI acmStreamSize16(
  HACMSTREAM16 has, DWORD cbInput, 
  LPDWORD pdwOutputBytes, DWORD fdwSize)
{
  FIXME(msacm, "(0x%04x, %ld, %p, %ld): stub\n",
    has, cbInput, pdwOutputBytes, fdwSize
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMSTREAMCONVERT (MSACM.75)
 */
MMRESULT16 WINAPI acmStreamConvert16(
  HACMSTREAM16 has, LPACMSTREAMHEADER16 pash, DWORD fdwConvert)
{
  FIXME(msacm, "(0x%04x, %p, %ld): stub\n", has, pash, fdwConvert);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMSTREAMRESET (MSACM.76)
 */
MMRESULT16 WINAPI acmStreamReset16(
  HACMSTREAM16 has, DWORD fdwReset)
{
  FIXME(msacm, "(0x%04x, %ld): stub\n", has, fdwReset);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMSTREAMPREPAREHEADER (MSACM.77)
 */
MMRESULT16 WINAPI acmStreamPrepareHeader16(
  HACMSTREAM16 has, LPACMSTREAMHEADER16 pash, DWORD fdwPrepare)
{
  FIXME(msacm, "(0x%04x, %p, %ld): stub\n", has, pash, fdwPrepare);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMSTREAMUNPREPAREHEADER (MSACM.78)
 */
MMRESULT16 WINAPI acmStreamUnprepareHeader16(
  HACMSTREAM16 has, LPACMSTREAMHEADER16 pash, DWORD fdwUnprepare)
{
  FIXME(msacm, "(0x%04x, %p, %ld): stub\n",
    has, pash, fdwUnprepare
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           ACMAPPLICATIONEXIT (MSACM.150)
 * FIXME
 *   No documentation found.
 */

/***********************************************************************
 *           ACMHUGEPAGELOCK (MSACM.175)
 * FIXME
 *   No documentation found.
 */

/***********************************************************************
 *           ACMHUGEPAGEUNLOCK (MSACM.176)
 * FIXME
 *   No documentation found.
 */

/***********************************************************************
 *           ACMOPENCONVERSION (MSACM.200)
 * FIXME
 *   No documentation found.
 */

/***********************************************************************
 *           ACMCLOSECONVERSION (MSACM.201)
 * FIXME
 *   No documentation found.
 */

/***********************************************************************
 *           ACMCONVERT (MSACM.202)
 * FIXME
 *   No documentation found.
 */

/***********************************************************************
 *           ACMCHOOSEFORMAT (MSACM.203)
 * FIXME
 *   No documentation found.
 */


