/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 */

#include "winbase.h"
#include "winerror.h"
#include "windef.h"
#include "winuser.h"
#include "debug.h"
#include "driver.h"
#include "heap.h"
#include "mmsystem.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "winreg.h"

/***********************************************************************
 *           acmDriverAddA (MSACM32.2)
 */
MMRESULT WINAPI acmDriverAddA(
  PHACMDRIVERID phadid, HINSTANCE hinstModule,
  LPARAM lParam, DWORD dwPriority, DWORD fdwAdd)
{
  PWINE_ACMLOCALDRIVER pld;
  if(!phadid)
    return MMSYSERR_INVALPARAM;

  /* Check if any unknown flags */
  if(fdwAdd & 
     ~(ACM_DRIVERADDF_FUNCTION|ACM_DRIVERADDF_NOTIFYHWND|
       ACM_DRIVERADDF_GLOBAL))
    return MMSYSERR_INVALFLAG;

  /* Check if any incompatible flags */
  if((fdwAdd & ACM_DRIVERADDF_FUNCTION) && 
     (fdwAdd & ACM_DRIVERADDF_NOTIFYHWND))
    return MMSYSERR_INVALFLAG;

  pld = HeapAlloc(MSACM_hHeap, 0, sizeof(WINE_ACMLOCALDRIVER));
  pld->pfnDriverProc = (DRIVERPROC) 
    GetProcAddress(hinstModule, "DriverProc");
  *phadid = (HACMDRIVERID) MSACM_RegisterDriver(NULL, NULL, pld);

  /* FIXME: lParam, dwPriority and fdwAdd ignored */

  return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverAddW (MSACM32.3)
 * FIXME
 *   Not implemented
 */
MMRESULT WINAPI acmDriverAddW(
  PHACMDRIVERID phadid, HINSTANCE hinstModule,
  LPARAM lParam, DWORD dwPriority, DWORD fdwAdd)
{
  FIXME(msacm, "(%p, 0x%08x, %ld, %ld, %ld): stub\n",
    phadid, hinstModule, lParam, dwPriority, fdwAdd
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmDriverClose (MSACM32.4)
 */
MMRESULT WINAPI acmDriverClose(
  HACMDRIVER had, DWORD fdwClose)
{
  PWINE_ACMDRIVER p;

  if(fdwClose)
    return MMSYSERR_INVALFLAG;

  p = MSACM_GetDriver(had);
  if(!p)
    return MMSYSERR_INVALHANDLE;

  p->obj.pACMDriverID->pACMDriver = NULL;

  /* FIXME: CloseDriver32 not implemented */
#if 0
  if(p->hDrvr)
    CloseDriver(p->hDrvr, 0, 0);
#endif

  HeapFree(MSACM_hHeap, 0, p);

  return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverDetailsA (MSACM32.5)
 */
MMRESULT WINAPI acmDriverDetailsA(
  HACMDRIVERID hadid, PACMDRIVERDETAILSA padd, DWORD fdwDetails)
{
  PWINE_ACMDRIVERID p;
  MMRESULT mmr;
  BOOL bOpenTemporary;

  p = MSACM_GetDriverID(hadid);
  if(!p)
    return MMSYSERR_INVALHANDLE;
  
  if(fdwDetails)
    return MMSYSERR_INVALFLAG;

  bOpenTemporary = !p->pACMDriver;
  if(bOpenTemporary) {
    bOpenTemporary = TRUE;
    acmDriverOpen((PHACMDRIVER) &p->pACMDriver, hadid, 0);
  }
  
  /* FIXME
   *   How does the driver know if the ANSI or 
   *   the UNICODE variant of PACMDRIVERDETAILS is used?
   *   It might check cbStruct or does it only accept ANSI.
   */
  mmr = (MMRESULT) acmDriverMessage(
    (HACMDRIVER) p->pACMDriver, ACMDM_DRIVER_DETAILS, 
    (LPARAM) padd,  0
  );

  if(bOpenTemporary) {
    acmDriverClose((HACMDRIVER) p->pACMDriver, 0);
    p->pACMDriver = NULL;
  }

  return mmr;
}

/***********************************************************************
 *           acmDriverDetailsW (MSACM32.6)
 * FIXME
 *   Not implemented
 */
MMRESULT WINAPI acmDriverDetailsW(
  HACMDRIVERID hadid, PACMDRIVERDETAILSW padd, DWORD fdwDetails)
{
  FIXME(msacm, "(0x%08x, %p, %ld): stub\n", hadid, padd, fdwDetails);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmDriverEnum (MSACM32.7)
 */
MMRESULT WINAPI acmDriverEnum(
  ACMDRIVERENUMCB fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
  PWINE_ACMDRIVERID p;

  if(!fnCallback)
    {
      return MMSYSERR_INVALPARAM;
    }

  if(fdwEnum && ~(ACM_DRIVERENUMF_NOLOCAL|ACM_DRIVERENUMF_DISABLED))
    {
      return MMSYSERR_INVALFLAG;
    }

  p = MSACM_pFirstACMDriverID;
  while(p)
    {
      (*fnCallback)((HACMDRIVERID) p, dwInstance, ACMDRIVERDETAILS_SUPPORTF_CODEC);
      p = p->pNextACMDriverID;
    }

  return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverID (MSACM32.8)
 */
MMRESULT WINAPI acmDriverID(
  HACMOBJ hao, PHACMDRIVERID phadid, DWORD fdwDriverID)
{
  PWINE_ACMOBJ pao;

  pao = MSACM_GetObj(hao);
  if(!pao)
    return MMSYSERR_INVALHANDLE;

  if(!phadid)
    return MMSYSERR_INVALPARAM;

  if(fdwDriverID)
    return MMSYSERR_INVALFLAG;

  *phadid = (HACMDRIVERID) pao->pACMDriverID;

  return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverMessage (MSACM32.9)
 * FIXME
 *   Not implemented
 */
LRESULT WINAPI acmDriverMessage(
  HACMDRIVER had, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
  PWINE_ACMDRIVER pad = MSACM_GetDriver(had);
  if(!pad)
    return MMSYSERR_INVALPARAM;

  /* FIXME: Check if uMsg legal */

  if(!SendDriverMessage(pad->hDrvr, uMsg, lParam1, lParam2))
    return MMSYSERR_NOTSUPPORTED;

  return MMSYSERR_NOERROR;
}


/***********************************************************************
 *           acmDriverOpen (MSACM32.10)
 */
MMRESULT WINAPI acmDriverOpen(
  PHACMDRIVER phad, HACMDRIVERID hadid, DWORD fdwOpen)
{
  PWINE_ACMDRIVERID padid;

  if(!phad)
    return MMSYSERR_INVALPARAM;

  padid = MSACM_GetDriverID(hadid); 
  if(!padid)
    return MMSYSERR_INVALHANDLE;

  if(fdwOpen)
    return MMSYSERR_INVALFLAG;

  if(padid->pACMDriver)
    {
      /* FIXME: Is it allowed? */
      ERR(msacm, "Can't open driver twice\n");
      return MMSYSERR_ERROR;
    }

  padid->pACMDriver = HeapAlloc(
    MSACM_hHeap, 0, sizeof(WINE_ACMDRIVER)
  );
  padid->pACMDriver->obj.pACMDriverID = padid;
  
  if(!padid->pACMLocalDriver)
      padid->pACMDriver->hDrvr =
        OpenDriverA(padid->pszDriverAlias, "drivers32", 0);
  else
    {
      padid->pACMDriver->hDrvr = MSACM_OpenDriverProc(
	padid->pACMLocalDriver->pfnDriverProc
      );
    }

  if(!padid->pACMDriver->hDrvr)
    return MMSYSERR_ERROR;
 
  /* FIXME: Create a WINE_ACMDRIVER32 */
  *phad = (HACMDRIVER) NULL;

  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmDriverPriority (MSACM32.11)
 */
MMRESULT WINAPI acmDriverPriority(
  HACMDRIVERID hadid, DWORD dwPriority, DWORD fdwPriority)
{
  PWINE_ACMDRIVERID padid;
  CHAR szSubKey[17];
  CHAR szBuffer[256];
  LONG lBufferLength = sizeof(szBuffer);
  LONG lError;
  HKEY hPriorityKey;
  DWORD dwPriorityCounter;

  padid = MSACM_GetDriverID(hadid);
  if(!padid)
    return MMSYSERR_INVALHANDLE;

  /* Check for unknown flags */
  if(fdwPriority & 
     ~(ACM_DRIVERPRIORITYF_ENABLE|ACM_DRIVERPRIORITYF_DISABLE|
       ACM_DRIVERPRIORITYF_BEGIN|ACM_DRIVERPRIORITYF_END))
    return MMSYSERR_INVALFLAG;

  /* Check for incompatible flags */
  if((fdwPriority & ACM_DRIVERPRIORITYF_ENABLE) &&
    (fdwPriority & ACM_DRIVERPRIORITYF_DISABLE))
    return MMSYSERR_INVALFLAG;

  /* Check for incompatible flags */
  if((fdwPriority & ACM_DRIVERPRIORITYF_BEGIN) &&
    (fdwPriority & ACM_DRIVERPRIORITYF_END))
    return MMSYSERR_INVALFLAG;

  lError = RegOpenKeyA(HKEY_CURRENT_USER, 
    "Software\\Microsoft\\Multimedia\\"
      "Audio Compression Manager\\Priority v4.00",
    &hPriorityKey
  );
  /* FIXME: Create key */
  if(lError != ERROR_SUCCESS)
    return MMSYSERR_ERROR;

  for(dwPriorityCounter = 1; ; dwPriorityCounter++)
  {
    wsnprintfA(szSubKey, 17, "Priorty%ld", dwPriorityCounter);
    lError = RegQueryValueA(hPriorityKey, szSubKey, szBuffer, &lBufferLength);
    if(lError != ERROR_SUCCESS)
      break;

    FIXME(msacm, "(0x%08x, %ld, %ld): stub (partial)\n", 
      hadid, dwPriority, fdwPriority
    );
    break;
  }

  RegCloseKey(hPriorityKey);

  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmDriverRemove (MSACM32.12)
 */
MMRESULT WINAPI acmDriverRemove(
  HACMDRIVERID hadid, DWORD fdwRemove)
{
  PWINE_ACMDRIVERID padid;

  padid = MSACM_GetDriverID(hadid);
  if(!padid)
    return MMSYSERR_INVALHANDLE;

  if(fdwRemove)
    return MMSYSERR_INVALFLAG;

  MSACM_UnregisterDriver(padid);

  return MMSYSERR_NOERROR;
}

