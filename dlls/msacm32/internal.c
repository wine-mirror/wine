/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 */

#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "wintypes.h"
#include "debug.h"
#include "driver.h"
#include "heap.h"
#include "mmsystem.h"
#include "msacm.h"
#include "msacmdrv.h"

/**********************************************************************/

HANDLE32 MSACM_hHeap32 = (HANDLE32) NULL;
PWINE_ACMDRIVERID32 MSACM_pFirstACMDriverID32 = NULL;
PWINE_ACMDRIVERID32 MSACM_pLastACMDriverID32 = NULL;

/***********************************************************************
 *           MSACM_RegisterDriver32() 
 */
PWINE_ACMDRIVERID32 MSACM_RegisterDriver32(
  LPSTR pszDriverAlias, LPSTR pszFileName,
  PWINE_ACMLOCALDRIVER32 pLocalDriver)
{
  PWINE_ACMDRIVERID32 padid;
  padid = (PWINE_ACMDRIVERID32) HeapAlloc(
    MSACM_hHeap32, 0, sizeof(WINE_ACMDRIVERID32)
  );
  padid->pszDriverAlias = 
    HEAP_strdupA(MSACM_hHeap32, 0, pszDriverAlias);
  padid->pszFileName = 
    HEAP_strdupA(MSACM_hHeap32, 0, pszFileName);
  padid->pACMLocalDriver = pLocalDriver; 
  padid->bEnabled = TRUE;
  padid->pACMDriver = NULL;
  padid->pNextACMDriverID = NULL;
  padid->pPreviousACMDriverID = 
    MSACM_pLastACMDriverID32;
  MSACM_pLastACMDriverID32 = padid;
  if(!MSACM_pFirstACMDriverID32)
    MSACM_pFirstACMDriverID32 = padid;

  return padid;
}

/***********************************************************************
 *           MSACM_RegisterAllDrivers32() 
 */
void MSACM_RegisterAllDrivers32()
{
  PWINE_ACMBUILTINDRIVER32 pbd;
  LPSTR pszBuffer;
  DWORD dwBufferLength;

  /* FIXME 
   *  What if the user edits system.ini while the program is running?
   *  Does Windows handle that?
   */
  if(!MSACM_pFirstACMDriverID32)
    return;

  /* FIXME: Do not work! How do I determine the section length? */
  dwBufferLength = 
    GetPrivateProfileSection32A("drivers32", NULL, 0, "system.ini");

  pszBuffer = (LPSTR) HeapAlloc(
    MSACM_hHeap32, 0, dwBufferLength
  );
  if(GetPrivateProfileSection32A(
    "drivers32", pszBuffer, dwBufferLength, "system.ini"))
    {
      char *s = pszBuffer;
      while(*s)
	{
	  if(!lstrncmpi32A("MSACM.", s, 6))
	    {
	      char *s2 = s;
	      while(*s2 != '\0' && *s2 != '=') s2++;
	      if(*s2)
		{
		  *s2++='\0';
		  MSACM_RegisterDriver32(s, s2, NULL);
		}
	    }  
	  s += lstrlen32A(s) + 1; /* Either next char or \0 */
	}
    }

  /* FIXME
   *   Check if any of the builtin driver was added
   *   when the external drivers was. 
   */

  pbd = MSACM_BuiltinDrivers32;
  while(pbd->pszDriverAlias)
    {
      PWINE_ACMLOCALDRIVER32 pld;
      pld = HeapAlloc(MSACM_hHeap32, 0, sizeof(WINE_ACMLOCALDRIVER32));
      pld->pfnDriverProc = pbd->pfnDriverProc;
      MSACM_RegisterDriver32(pbd->pszDriverAlias, NULL, pld);
      pbd++;
    }
   HeapFree(MSACM_hHeap32, 0, pszBuffer);
}

/***********************************************************************
 *           MSACM_UnregisterDriver32()
 */
PWINE_ACMDRIVERID32 MSACM_UnregisterDriver32(PWINE_ACMDRIVERID32 p)
{
  PWINE_ACMDRIVERID32 pNextACMDriverID;

  if(p->pACMDriver)
    acmDriverClose32((HACMDRIVER32) p->pACMDriver, 0);

  if(p->pszDriverAlias)
    HeapFree(MSACM_hHeap32, 0, p->pszDriverAlias);
  if(p->pszFileName)
    HeapFree(MSACM_hHeap32, 0, p->pszFileName);
  if(p->pACMLocalDriver)
    HeapFree(MSACM_hHeap32, 0, p->pACMLocalDriver);

  if(p->pPreviousACMDriverID)
    p->pPreviousACMDriverID->pNextACMDriverID = p->pNextACMDriverID;
  if(p->pNextACMDriverID)
    p->pNextACMDriverID->pPreviousACMDriverID = p->pPreviousACMDriverID;

  pNextACMDriverID = p->pNextACMDriverID;

  HeapFree(MSACM_hHeap32, 0, p);

  return pNextACMDriverID;
}

/***********************************************************************
 *           MSACM_UnregisterAllDrivers32()
 * FIXME
 *   Where should this function be called?
 */
void MSACM_UnregisterAllDrivers32()
{
  PWINE_ACMDRIVERID32 p = MSACM_pFirstACMDriverID32;
  while(p) p = MSACM_UnregisterDriver32(p);
}

/***********************************************************************
 *           MSACM_GetDriverID32() 
 */
PWINE_ACMDRIVERID32 MSACM_GetDriverID32(HACMDRIVERID32 hDriverID)
{
  return (PWINE_ACMDRIVERID32) hDriverID;
}

/***********************************************************************
 *           MSACM_GetDriver32()
 */
PWINE_ACMDRIVER32 MSACM_GetDriver32(HACMDRIVER32 hDriver)
{
  return (PWINE_ACMDRIVER32) hDriver;
}

/***********************************************************************
 *           MSACM_GetObj32()
 */
PWINE_ACMOBJ32 MSACM_GetObj32(HACMOBJ32 hObj)
{
  return (PWINE_ACMOBJ32) hObj;
}

/***********************************************************************
 *           MSACM_OpenDriverProc32
 * FIXME
 *  This function should be integrated with OpenDriver,
 *  renamed and moved there.
 */
HDRVR32 MSACM_OpenDriverProc32(DRIVERPROC32 pfnDriverProc)
{
#if 0
  LPDRIVERITEM32A pDrvr;

  /* FIXME: This is a very bad solution */
  pDrvr = (LPDRIVERITEM32A) HeapAlloc(MSACM_hHeap32, HEAP_ZERO_MEMORY, sizeof(DRIVERITEM32A));
  pDrvr->count = 1;
  pDrvr->driverproc = pfnDriverProc;
  
  /* FIXME: Send DRV_OPEN among others to DriverProc */

  return (HDRVR32) pDrvr;
#else
  return (HDRVR32) 0;
#endif
}



