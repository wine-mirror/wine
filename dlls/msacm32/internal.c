/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 */

#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "driver.h"
#include "heap.h"
#include "mmsystem.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "debugtools.h"

/**********************************************************************/

HANDLE MSACM_hHeap = (HANDLE) NULL;
PWINE_ACMDRIVERID MSACM_pFirstACMDriverID = NULL;
PWINE_ACMDRIVERID MSACM_pLastACMDriverID = NULL;

/***********************************************************************
 *           MSACM_RegisterDriver32() 
 */
PWINE_ACMDRIVERID MSACM_RegisterDriver(
  LPSTR pszDriverAlias, LPSTR pszFileName,
  PWINE_ACMLOCALDRIVER pLocalDriver)
{
  PWINE_ACMDRIVERID padid;
  padid = (PWINE_ACMDRIVERID) HeapAlloc(
    MSACM_hHeap, 0, sizeof(WINE_ACMDRIVERID)
  );
  padid->pszDriverAlias = 
    HEAP_strdupA(MSACM_hHeap, 0, pszDriverAlias);
  padid->pszFileName = 
    HEAP_strdupA(MSACM_hHeap, 0, pszFileName);
  padid->pACMLocalDriver = pLocalDriver; 
  padid->bEnabled = TRUE;
  padid->pACMDriver = NULL;
  padid->pNextACMDriverID = NULL;
  padid->pPreviousACMDriverID = 
    MSACM_pLastACMDriverID;
  MSACM_pLastACMDriverID = padid;
  if(!MSACM_pFirstACMDriverID)
    MSACM_pFirstACMDriverID = padid;

  return padid;
}

/***********************************************************************
 *           MSACM_RegisterAllDrivers32() 
 */
void MSACM_RegisterAllDrivers()
{
  PWINE_ACMBUILTINDRIVER pbd;
  LPSTR pszBuffer;
  DWORD dwBufferLength;

  /* FIXME 
   *  What if the user edits system.ini while the program is running?
   *  Does Windows handle that?
   */
  if(!MSACM_pFirstACMDriverID)
    return;

  /* FIXME: Do not work! How do I determine the section length? */
  dwBufferLength = 
    GetPrivateProfileSectionA("drivers32", NULL, 0, "system.ini");

  pszBuffer = (LPSTR) HeapAlloc(
    MSACM_hHeap, 0, dwBufferLength
  );
  if(GetPrivateProfileSectionA(
    "drivers32", pszBuffer, dwBufferLength, "system.ini"))
    {
      char *s = pszBuffer;
      while(*s)
	{
	  if(!lstrncmpiA("MSACM.", s, 6))
	    {
	      char *s2 = s;
	      while(*s2 != '\0' && *s2 != '=') s2++;
	      if(*s2)
		{
		  *s2++='\0';
		  MSACM_RegisterDriver(s, s2, NULL);
		}
	    }  
	  s += lstrlenA(s) + 1; /* Either next char or \0 */
	}
    }

  /* FIXME
   *   Check if any of the builtin driver was added
   *   when the external drivers was. 
   */

  pbd = MSACM_BuiltinDrivers;
  while(pbd->pszDriverAlias)
    {
      PWINE_ACMLOCALDRIVER pld;
      pld = HeapAlloc(MSACM_hHeap, 0, sizeof(WINE_ACMLOCALDRIVER));
      pld->pfnDriverProc = pbd->pfnDriverProc;
      MSACM_RegisterDriver(pbd->pszDriverAlias, NULL, pld);
      pbd++;
    }
   HeapFree(MSACM_hHeap, 0, pszBuffer);
}

/***********************************************************************
 *           MSACM_UnregisterDriver32()
 */
PWINE_ACMDRIVERID MSACM_UnregisterDriver(PWINE_ACMDRIVERID p)
{
  PWINE_ACMDRIVERID pNextACMDriverID;

  if(p->pACMDriver)
    acmDriverClose((HACMDRIVER) p->pACMDriver, 0);

  if(p->pszDriverAlias)
    HeapFree(MSACM_hHeap, 0, p->pszDriverAlias);
  if(p->pszFileName)
    HeapFree(MSACM_hHeap, 0, p->pszFileName);
  if(p->pACMLocalDriver)
    HeapFree(MSACM_hHeap, 0, p->pACMLocalDriver);

  if(p->pPreviousACMDriverID)
    p->pPreviousACMDriverID->pNextACMDriverID = p->pNextACMDriverID;
  if(p->pNextACMDriverID)
    p->pNextACMDriverID->pPreviousACMDriverID = p->pPreviousACMDriverID;

  pNextACMDriverID = p->pNextACMDriverID;

  HeapFree(MSACM_hHeap, 0, p);

  return pNextACMDriverID;
}

/***********************************************************************
 *           MSACM_UnregisterAllDrivers32()
 * FIXME
 *   Where should this function be called?
 */
void MSACM_UnregisterAllDrivers()
{
  PWINE_ACMDRIVERID p = MSACM_pFirstACMDriverID;
  while(p) p = MSACM_UnregisterDriver(p);
}

/***********************************************************************
 *           MSACM_GetDriverID32() 
 */
PWINE_ACMDRIVERID MSACM_GetDriverID(HACMDRIVERID hDriverID)
{
  return (PWINE_ACMDRIVERID) hDriverID;
}

/***********************************************************************
 *           MSACM_GetDriver32()
 */
PWINE_ACMDRIVER MSACM_GetDriver(HACMDRIVER hDriver)
{
  return (PWINE_ACMDRIVER) hDriver;
}

/***********************************************************************
 *           MSACM_GetObj32()
 */
PWINE_ACMOBJ MSACM_GetObj(HACMOBJ hObj)
{
  return (PWINE_ACMOBJ) hObj;
}

/***********************************************************************
 *           MSACM_OpenDriverProc32
 * FIXME
 *  This function should be integrated with OpenDriver,
 *  renamed and moved there.
 */
HDRVR MSACM_OpenDriverProc(DRIVERPROC pfnDriverProc)
{
#if 0
  LPDRIVERITEMA pDrvr;

  /* FIXME: This is a very bad solution */
  pDrvr = (LPDRIVERITEMA) HeapAlloc(MSACM_hHeap, HEAP_ZERO_MEMORY, sizeof(DRIVERITEMA));
  pDrvr->count = 1;
  pDrvr->driverproc = pfnDriverProc;
  
  /* FIXME: Send DRV_OPEN among others to DriverProc */

  return (HDRVR) pDrvr;
#else
  return (HDRVR) 0;
#endif
}



