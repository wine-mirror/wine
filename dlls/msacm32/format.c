/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 */

#include "winbase.h"
#include "winerror.h"
#include "windef.h"
#include "debug.h"
#include "mmsystem.h"
#include "msacm.h"
#include "msacmdrv.h"

/***********************************************************************
 *           acmFormatChooseA (MSACM32.23)
 */
MMRESULT WINAPI acmFormatChooseA(
  PACMFORMATCHOOSEA pafmtc)
{
  FIXME(msacm, "(%p): stub\n", pafmtc);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFormatChooseW (MSACM32.24)
 */
MMRESULT WINAPI acmFormatChooseW(
  PACMFORMATCHOOSEW pafmtc)
{
  FIXME(msacm, "(%p): stub\n", pafmtc);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFormatDetailsA (MSACM32.25)
 */
MMRESULT WINAPI acmFormatDetailsA(
  HACMDRIVER had, PACMFORMATDETAILSA pafd, DWORD fdwDetails)
{
  if(fdwDetails & ~(ACM_FORMATDETAILSF_FORMAT))
    return MMSYSERR_INVALFLAG;

  /* FIXME
   *   How does the driver know if the ANSI or
   *   the UNICODE variant of PACMFORMATDETAILS is used?
   *   It might check cbStruct or does it only accept ANSI.
   */
  return (MMRESULT) acmDriverMessage(
    had, ACMDM_FORMAT_DETAILS,
    (LPARAM) pafd,  (LPARAM) fdwDetails
  );
}

/***********************************************************************
 *           acmFormatDetailsW (MSACM32.26)
 */
MMRESULT WINAPI acmFormatDetailsW(
  HACMDRIVER had, PACMFORMATDETAILSW pafd, DWORD fdwDetails)
{
  FIXME(msacm, "(0x%08x, %p, %ld): stub\n", had, pafd, fdwDetails);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFormatEnumA (MSACM32.27)
 */
MMRESULT WINAPI acmFormatEnumA(
  HACMDRIVER had, PACMFORMATDETAILSA pafd,
  ACMFORMATENUMCBA fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
  FIXME(msacm, "(0x%08x, %p, %p, %ld, %ld): stub\n",
    had, pafd, fnCallback, dwInstance, fdwEnum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFormatEnumW (MSACM32.28)
 */
MMRESULT WINAPI acmFormatEnumW(
  HACMDRIVER had, PACMFORMATDETAILSW pafd,
  ACMFORMATENUMCBW fnCallback, DWORD dwInstance,  DWORD fdwEnum)
{
  FIXME(msacm, "(0x%08x, %p, %p, %ld, %ld): stub\n",
    had, pafd, fnCallback, dwInstance, fdwEnum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFormatSuggest (MSACM32.29)
 */
MMRESULT WINAPI acmFormatSuggest(
  HACMDRIVER had, PWAVEFORMATEX pwfxSrc, PWAVEFORMATEX pwfxDst,
  DWORD cbwfxDst, DWORD fdwSuggest)
{
  FIXME(msacm, "(0x%08x, %p, %p, %ld, %ld): stub\n",
    had, pwfxSrc, pwfxDst, cbwfxDst, fdwSuggest
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFormatTagDetailsA (MSACM32.30)
 */
MMRESULT WINAPI acmFormatTagDetailsA(
  HACMDRIVER had, PACMFORMATTAGDETAILSA paftd, DWORD fdwDetails)
{
  if(fdwDetails & 
     ~(ACM_FORMATTAGDETAILSF_FORMATTAG|ACM_FORMATTAGDETAILSF_LARGESTSIZE))
    return MMSYSERR_INVALFLAG;

  /* FIXME
   *   How does the driver know if the ANSI or
   *   the UNICODE variant of PACMFORMATTAGDETAILS is used?
   *   It might check cbStruct or does it only accept ANSI.
   */
  return (MMRESULT) acmDriverMessage(
    had, ACMDM_FORMATTAG_DETAILS,
    (LPARAM) paftd,  (LPARAM) fdwDetails
  );
}

/***********************************************************************
 *           acmFormatTagDetailsW (MSACM32.31)
 */
MMRESULT WINAPI acmFormatTagDetailsW(
  HACMDRIVER had, PACMFORMATTAGDETAILSW paftd, DWORD fdwDetails)
{
  FIXME(msacm, "(0x%08x, %p, %ld): stub\n", had, paftd, fdwDetails);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFormatTagEnumA (MSACM32.32)
 */
MMRESULT WINAPI acmFormatTagEnumA(
  HACMDRIVER had, PACMFORMATTAGDETAILSA paftd,
  ACMFORMATTAGENUMCBA fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
  FIXME(msacm, "(0x%08x, %p, %p, %ld, %ld): stub\n",
    had, paftd, fnCallback, dwInstance, fdwEnum
  ); 
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFormatTagEnumW (MSACM32.33)
 */
MMRESULT WINAPI acmFormatTagEnumW(
  HACMDRIVER had, PACMFORMATTAGDETAILSW paftd,
  ACMFORMATTAGENUMCBW fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
  FIXME(msacm, "(0x%08x, %p, %p, %ld, %ld): stub\n",
    had, paftd, fnCallback, dwInstance, fdwEnum
  ); 
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}
