/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 */

#include "winbase.h"
#include "winerror.h"
#include "wintypes.h"
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
MMRESULT32 WINAPI acmDriverAdd32A(
  PHACMDRIVERID32 phadid, HINSTANCE32 hinstModule,
  LPARAM lParam, DWORD dwPriority, DWORD fdwAdd)
{
  PWINE_ACMLOCALDRIVER32 pld;
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

  pld = HeapAlloc(MSACM_hHeap32, 0, sizeof(WINE_ACMLOCALDRIVER32));
  pld->pfnDriverProc = (DRIVERPROC32) 
    GetProcAddress32(hinstModule, "DriverProc");
  *phadid = (HACMDRIVERID32) MSACM_RegisterDriver32(NULL, NULL, pld);

  /* FIXME: lParam, dwPriority and fdwAdd ignored */

  return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverAddW (MSACM32.3)
 * FIXME
 *   Not implemented
 */
MMRESULT32 WINAPI acmDriverAdd32W(
  PHACMDRIVERID32 phadid, HINSTANCE32 hinstModule,
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
MMRESULT32 WINAPI acmDriverClose32(
  HACMDRIVER32 had, DWORD fdwClose)
{
  PWINE_ACMDRIVER32 p;

  if(fdwClose)
    return MMSYSERR_INVALFLAG;

  p = MSACM_GetDriver32(had);
  if(!p)
    return MMSYSERR_INVALHANDLE;

  p->obj.pACMDriverID->pACMDriver = NULL;

  /* FIXME: CloseDriver32 not implemented */
#if 0
  if(p->hDrvr)
    CloseDriver32(p->hDrvr, 0, 0);
#endif

  HeapFree(MSACM_hHeap32, 0, p);

  return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverDetailsA (MSACM32.5)
 */
MMRESULT32 WINAPI acmDriverDetails32A(
  HACMDRIVERID32 hadid, PACMDRIVERDETAILS32A padd, DWORD fdwDetails)
{
  PWINE_ACMDRIVERID32 p;
  MMRESULT32 mmr;
  BOOL32 bOpenTemporary;

  p = MSACM_GetDriverID32(hadid);
  if(!p)
    return MMSYSERR_INVALHANDLE;
  
  if(fdwDetails)
    return MMSYSERR_INVALFLAG;

  bOpenTemporary = !p->pACMDriver;
  if(bOpenTemporary) {
    bOpenTemporary = TRUE;
    acmDriverOpen32((PHACMDRIVER32) &p->pACMDriver, hadid, 0);
  }
  
  /* FIXME
   *   How does the driver know if the ANSI or 
   *   the UNICODE variant of PACMDRIVERDETAILS is used?
   *   It might check cbStruct or does it only accept ANSI.
   */
  mmr = (MMRESULT32) acmDriverMessage32(
    (HACMDRIVER32) p->pACMDriver, ACMDM_DRIVER_DETAILS, 
    (LPARAM) padd,  0
  );

  if(bOpenTemporary) {
    acmDriverClose32((HACMDRIVER32) p->pACMDriver, 0);
    p->pACMDriver = NULL;
  }

  return mmr;
}

/***********************************************************************
 *           acmDriverDetailsW (MSACM32.6)
 * FIXME
 *   Not implemented
 */
MMRESULT32 WINAPI acmDriverDetails32W(
  HACMDRIVERID32 hadid, PACMDRIVERDETAILS32W padd, DWORD fdwDetails)
{
  FIXME(msacm, "(0x%08x, %p, %ld): stub\n", hadid, padd, fdwDetails);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmDriverEnum (MSACM32.7)
 */
MMRESULT32 WINAPI acmDriverEnum32(
  ACMDRIVERENUMCB32 fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
  PWINE_ACMDRIVERID32 p;

  if(!fnCallback)
    {
      return MMSYSERR_INVALPARAM;
    }

  if(fdwEnum && ~(ACM_DRIVERENUMF_NOLOCAL|ACM_DRIVERENUMF_DISABLED))
    {
      return MMSYSERR_INVALFLAG;
    }

  p = MSACM_pFirstACMDriverID32;
  while(p)
    {
      (*fnCallback)((HACMDRIVERID32) p, dwInstance, ACMDRIVERDETAILS_SUPPORTF_CODEC);
      p = p->pNextACMDriverID;
    }

  return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverID (MSACM32.8)
 */
MMRESULT32 WINAPI acmDriverID32(
  HACMOBJ32 hao, PHACMDRIVERID32 phadid, DWORD fdwDriverID)
{
  PWINE_ACMOBJ32 pao;

  pao = MSACM_GetObj32(hao);
  if(!pao)
    return MMSYSERR_INVALHANDLE;

  if(!phadid)
    return MMSYSERR_INVALPARAM;

  if(fdwDriverID)
    return MMSYSERR_INVALFLAG;

  *phadid = (HACMDRIVERID32) pao->pACMDriverID;

  return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverMessage (MSACM32.9)
 * FIXME
 *   Not implemented
 */
LRESULT WINAPI acmDriverMessage32(
  HACMDRIVER32 had, UINT32 uMsg, LPARAM lParam1, LPARAM lParam2)
{
  PWINE_ACMDRIVER32 pad = MSACM_GetDriver32(had);
  if(!pad)
    return MMSYSERR_INVALPARAM;

  /* FIXME: Check if uMsg legal */

  if(!SendDriverMessage32(pad->hDrvr, uMsg, lParam1, lParam2))
    return MMSYSERR_NOTSUPPORTED;

  return MMSYSERR_NOERROR;
}


/***********************************************************************
 *           acmDriverOpen (MSACM32.10)
 */
MMRESULT32 WINAPI acmDriverOpen32(
  PHACMDRIVER32 phad, HACMDRIVERID32 hadid, DWORD fdwOpen)
{
  PWINE_ACMDRIVERID32 padid;

  if(!phad)
    return MMSYSERR_INVALPARAM;

  padid = MSACM_GetDriverID32(hadid); 
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
    MSACM_hHeap32, 0, sizeof(WINE_ACMDRIVER32)
  );
  padid->pACMDriver->obj.pACMDriverID = padid;
  
  if(!padid->pACMLocalDriver)
      padid->pACMDriver->hDrvr =
        OpenDriver32A(padid->pszDriverAlias, "drivers32", 0);
  else
    {
      padid->pACMDriver->hDrvr = MSACM_OpenDriverProc32(
	padid->pACMLocalDriver->pfnDriverProc
      );
    }

  if(!padid->pACMDriver->hDrvr)
    return MMSYSERR_ERROR;
 
  /* FIXME: Create a WINE_ACMDRIVER32 */
  *phad = (HACMDRIVER32) NULL;

  return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmDriverPriority (MSACM32.11)
 */
MMRESULT32 WINAPI acmDriverPriority32(
  HACMDRIVERID32 hadid, DWORD dwPriority, DWORD fdwPriority)
{
  PWINE_ACMDRIVERID32 padid;
  CHAR szSubKey[17];
  CHAR szBuffer[256];
  LONG lBufferLength = sizeof(szBuffer);
  LONG lError;
  HKEY hPriorityKey;
  DWORD dwPriorityCounter;

  padid = MSACM_GetDriverID32(hadid);
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

  lError = RegOpenKey32A(HKEY_CURRENT_USER, 
    "Software\\Microsoft\\Multimedia\\"
      "Audio Compression Manager\\Priority v4.00",
    &hPriorityKey
  );
  /* FIXME: Create key */
  if(lError != ERROR_SUCCESS)
    return MMSYSERR_ERROR;

  for(dwPriorityCounter = 1; ; dwPriorityCounter++)
  {
    wsnprintf32A(szSubKey, 17, "Priorty%ld", dwPriorityCounter);
    lError = RegQueryValue32A(hPriorityKey, szSubKey, szBuffer, &lBufferLength);
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
MMRESULT32 WINAPI acmDriverRemove32(
  HACMDRIVERID32 hadid, DWORD fdwRemove)
{
  PWINE_ACMDRIVERID32 padid;

  padid = MSACM_GetDriverID32(hadid);
  if(!padid)
    return MMSYSERR_INVALHANDLE;

  if(fdwRemove)
    return MMSYSERR_INVALFLAG;

  MSACM_UnregisterDriver32(padid);

  return MMSYSERR_NOERROR;
}

