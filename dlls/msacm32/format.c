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
 *           acmFormatChooseA (MSACM32.23)
 */
MMRESULT32 WINAPI acmFormatChoose32A(
  PACMFORMATCHOOSE32A pafmtc)
{
  FIXME(msacm, "(%p): stub\n", pafmtc);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFormatChooseW (MSACM32.24)
 */
MMRESULT32 WINAPI acmFormatChoose32W(
  PACMFORMATCHOOSE32W pafmtc)
{
  FIXME(msacm, "(%p): stub\n", pafmtc);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFormatDetailsA (MSACM32.25)
 */
MMRESULT32 WINAPI acmFormatDetails32A(
  HACMDRIVER32 had, PACMFORMATDETAILS32A pafd, DWORD fdwDetails)
{
  if(fdwDetails & ~(ACM_FORMATDETAILSF_FORMAT))
    return MMSYSERR_INVALFLAG;

  /* FIXME
   *   How does the driver know if the ANSI or
   *   the UNICODE variant of PACMFORMATDETAILS is used?
   *   It might check cbStruct or does it only accept ANSI.
   */
  return (MMRESULT32) acmDriverMessage32(
    had, ACMDM_FORMAT_DETAILS,
    (LPARAM) pafd,  (LPARAM) fdwDetails
  );
}

/***********************************************************************
 *           acmFormatDetailsW (MSACM32.26)
 */
MMRESULT32 WINAPI acmFormatDetails32W(
  HACMDRIVER32 had, PACMFORMATDETAILS32W pafd, DWORD fdwDetails)
{
  FIXME(msacm, "(0x%08x, %p, %ld): stub\n", had, pafd, fdwDetails);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFormatEnumA (MSACM32.27)
 */
MMRESULT32 WINAPI acmFormatEnum32A(
  HACMDRIVER32 had, PACMFORMATDETAILS32A pafd,
  ACMFORMATENUMCB32A fnCallback, DWORD dwInstance, DWORD fdwEnum)
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
MMRESULT32 WINAPI acmFormatEnum32W(
  HACMDRIVER32 had, PACMFORMATDETAILS32W pafd,
  ACMFORMATENUMCB32W fnCallback, DWORD dwInstance,  DWORD fdwEnum)
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
MMRESULT32 WINAPI acmFormatSuggest32(
  HACMDRIVER32 had, PWAVEFORMATEX pwfxSrc, PWAVEFORMATEX pwfxDst,
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
MMRESULT32 WINAPI acmFormatTagDetails32A(
  HACMDRIVER32 had, PACMFORMATTAGDETAILS32A paftd, DWORD fdwDetails)
{
  if(fdwDetails & 
     ~(ACM_FORMATTAGDETAILSF_FORMATTAG|ACM_FORMATTAGDETAILSF_LARGESTSIZE))
    return MMSYSERR_INVALFLAG;

  /* FIXME
   *   How does the driver know if the ANSI or
   *   the UNICODE variant of PACMFORMATTAGDETAILS is used?
   *   It might check cbStruct or does it only accept ANSI.
   */
  return (MMRESULT32) acmDriverMessage32(
    had, ACMDM_FORMATTAG_DETAILS,
    (LPARAM) paftd,  (LPARAM) fdwDetails
  );
}

/***********************************************************************
 *           acmFormatTagDetailsW (MSACM32.31)
 */
MMRESULT32 WINAPI acmFormatTagDetails32W(
  HACMDRIVER32 had, PACMFORMATTAGDETAILS32W paftd, DWORD fdwDetails)
{
  FIXME(msacm, "(0x%08x, %p, %ld): stub\n", had, paftd, fdwDetails);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFormatTagEnumA (MSACM32.32)
 */
MMRESULT32 WINAPI acmFormatTagEnum32A(
  HACMDRIVER32 had, PACMFORMATTAGDETAILS32A paftd,
  ACMFORMATTAGENUMCB32A fnCallback, DWORD dwInstance, DWORD fdwEnum)
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
MMRESULT32 WINAPI acmFormatTagEnum32W(
  HACMDRIVER32 had, PACMFORMATTAGDETAILS32W paftd,
  ACMFORMATTAGENUMCB32W fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
  FIXME(msacm, "(0x%08x, %p, %p, %ld, %ld): stub\n",
    had, paftd, fnCallback, dwInstance, fdwEnum
  ); 
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}
