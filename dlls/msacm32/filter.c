/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 */

#include "winbase.h"
#include "winerror.h"
#include "wintypes.h"
#include "debug.h"
#include "mmsystem.h"
#include "msacm.h"
#include "msacmdrv.h"

/***********************************************************************
 *           acmFilterChooseA (MSACM32.13)
 */
MMRESULT32 WINAPI acmFilterChoose32A(
  PACMFILTERCHOOSE32A pafltrc)
{
  FIXME(msacm, "(%p): stub\n", pafltrc);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFilterChooseW (MSACM32.14)
 */
MMRESULT32 WINAPI acmFilterChoose32W(
  PACMFILTERCHOOSE32W pafltrc)
{
  FIXME(msacm, "(%p): stub\n", pafltrc);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFilterDetailsA (MSACM32.15)
 */
MMRESULT32 WINAPI acmFilterDetails32A(
  HACMDRIVER32 had, PACMFILTERDETAILS32A pafd, DWORD fdwDetails)
{
  if(fdwDetails & ~(ACM_FILTERDETAILSF_FILTER))
    return MMSYSERR_INVALFLAG;

  /* FIXME
   *   How does the driver know if the ANSI or
   *   the UNICODE variant of PACMFILTERDETAILS is used?
   *   It might check cbStruct or does it only accept ANSI.
   */
  return (MMRESULT32) acmDriverMessage32(
    had, ACMDM_FILTER_DETAILS,
    (LPARAM) pafd,  (LPARAM) fdwDetails
  );
}

/***********************************************************************
 *           acmFilterDetailsW (MSACM32.16)
 */
MMRESULT32 WINAPI acmFilterDetails32W(
  HACMDRIVER32 had, PACMFILTERDETAILS32W pafd, DWORD fdwDetails)
{
  FIXME(msacm, "(0x%08x, %p, %ld): stub\n", had, pafd, fdwDetails);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFilterEnumA (MSACM32.17)
 */
MMRESULT32 WINAPI acmFilterEnum32A(
  HACMDRIVER32 had, PACMFILTERDETAILS32A pafd, 
  ACMFILTERENUMCB32A fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
  FIXME(msacm, "(0x%08x, %p, %p, %ld, %ld): stub\n",
    had, pafd, fnCallback, dwInstance, fdwEnum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFilterEnumW (MSACM32.18)
 */
MMRESULT32 WINAPI acmFilterEnum32W(
  HACMDRIVER32 had, PACMFILTERDETAILS32W pafd, 
  ACMFILTERENUMCB32W fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
  FIXME(msacm, "(0x%08x, %p, %p, %ld, %ld): stub\n",
    had, pafd, fnCallback, dwInstance, fdwEnum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFilterTagDetailsA (MSACM32.19)
 */
MMRESULT32 WINAPI acmFilterTagDetails32A(
  HACMDRIVER32 had, PACMFILTERTAGDETAILS32A paftd, DWORD fdwDetails)
{
  if(fdwDetails & 
     ~(ACM_FILTERTAGDETAILSF_FILTERTAG|
       ACM_FILTERTAGDETAILSF_LARGESTSIZE))
    return MMSYSERR_INVALFLAG;

  /* FIXME
   *   How does the driver know if the ANSI or
   *   the UNICODE variant of PACMFILTERTAGDETAILS is used?
   *   It might check cbStruct or does it only accept ANSI.
   */
  return (MMRESULT32) acmDriverMessage32(
    had, ACMDM_FILTERTAG_DETAILS,
    (LPARAM) paftd,  (LPARAM) fdwDetails
  );
}

/***********************************************************************
 *           acmFilterTagDetailsW (MSACM32.20)
 */
MMRESULT32 WINAPI acmFilterTagDetails32W(
  HACMDRIVER32 had, PACMFILTERTAGDETAILS32W paftd, DWORD fdwDetails)
{
  FIXME(msacm, "(0x%08x, %p, %ld): stub\n", had, paftd, fdwDetails);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFilterTagEnumA (MSACM32.21)
 */
MMRESULT32 WINAPI acmFilterTagEnum32A(
  HACMDRIVER32 had, PACMFILTERTAGDETAILS32A  paftd,
  ACMFILTERTAGENUMCB32A fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
  FIXME(msacm, "(0x%08x, %p, %p, %ld, %ld): stub\n",
    had, paftd, fnCallback, dwInstance, fdwEnum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFilterTagEnumW (MSACM32.22)
 */
MMRESULT32 WINAPI acmFilterTagEnum32W(
  HACMDRIVER32 had, PACMFILTERTAGDETAILS32W paftd,
  ACMFILTERTAGENUMCB32W fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
  FIXME(msacm, "(0x%08x, %p, %p, %ld, %ld): stub\n",
    had, paftd, fnCallback, dwInstance, fdwEnum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}
