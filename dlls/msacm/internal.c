/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 *		  1999	Eric Pouech
 */

#include <string.h>

#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "driver.h"
#include "heap.h"
#include "mmsystem.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "wineacm.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(msacm)	

/**********************************************************************/

HANDLE MSACM_hHeap = (HANDLE) NULL;
PWINE_ACMDRIVERID MSACM_pFirstACMDriverID = NULL;
PWINE_ACMDRIVERID MSACM_pLastACMDriverID = NULL;

/***********************************************************************
 *           MSACM_RegisterDriver32() 
 */
PWINE_ACMDRIVERID MSACM_RegisterDriver(LPSTR pszDriverAlias, LPSTR pszFileName,
				       HINSTANCE hinstModule)
{ 
    PWINE_ACMDRIVERID padid;

    TRACE("('%s', '%s', 0x%08x)\n", pszDriverAlias, pszFileName, hinstModule);

    padid = (PWINE_ACMDRIVERID) HeapAlloc(MSACM_hHeap, 0, sizeof(WINE_ACMDRIVERID));
    padid->pszDriverAlias = HEAP_strdupA(MSACM_hHeap, 0, pszDriverAlias);
    padid->pszFileName = HEAP_strdupA(MSACM_hHeap, 0, pszFileName);
    padid->hInstModule = hinstModule;
    padid->bEnabled = TRUE;
    padid->pACMDriverList = NULL;
    padid->pNextACMDriverID = NULL;
    padid->pPrevACMDriverID = MSACM_pLastACMDriverID;
    if (MSACM_pLastACMDriverID)
	MSACM_pLastACMDriverID->pNextACMDriverID = padid;
    MSACM_pLastACMDriverID = padid;
    if (!MSACM_pFirstACMDriverID)
	MSACM_pFirstACMDriverID = padid;
    
    return padid;
}

/***********************************************************************
 *           MSACM_RegisterAllDrivers32() 
 */
void MSACM_RegisterAllDrivers(void)
{
    LPSTR pszBuffer;
    DWORD dwBufferLength;
    
    /* FIXME 
     *  What if the user edits system.ini while the program is running?
     *  Does Windows handle that?
     */
    if (MSACM_pFirstACMDriverID)
	return;
    
    /* FIXME: Do not work! How do I determine the section length? */
    dwBufferLength = 1024;
/* EPP 	GetPrivateProfileSectionA("drivers32", NULL, 0, "system.ini"); */
    
    pszBuffer = (LPSTR) HeapAlloc(MSACM_hHeap, 0, dwBufferLength);
    if (GetPrivateProfileSectionA("drivers32", pszBuffer, dwBufferLength, "system.ini")) {
	char* s = pszBuffer;
	while (*s) {
	    if (!lstrncmpiA("MSACM.", s, 6)) {
		char *s2 = s;
		while (*s2 != '\0' && *s2 != '=') s2++;
		if (*s2) {
		    *s2++ = '\0';
		    MSACM_RegisterDriver(s, s2, 0);
		}
	    }  
	    s += lstrlenA(s) + 1; /* Either next char or \0 */
	}
    }
    
    HeapFree(MSACM_hHeap, 0, pszBuffer);
}

/***********************************************************************
 *           MSACM_UnregisterDriver32()
 */
PWINE_ACMDRIVERID MSACM_UnregisterDriver(PWINE_ACMDRIVERID p)
{
    PWINE_ACMDRIVERID pNextACMDriverID;
    
    while (p->pACMDriverList)
	acmDriverClose((HACMDRIVER) p->pACMDriverList, 0);
    
    if (p->pszDriverAlias)
	HeapFree(MSACM_hHeap, 0, p->pszDriverAlias);
    if (p->pszFileName)
	HeapFree(MSACM_hHeap, 0, p->pszFileName);
    
    if (p == MSACM_pFirstACMDriverID)
	MSACM_pFirstACMDriverID = p->pNextACMDriverID;
    if (p == MSACM_pLastACMDriverID)
	MSACM_pLastACMDriverID = p->pPrevACMDriverID;

    if (p->pPrevACMDriverID)
	p->pPrevACMDriverID->pNextACMDriverID = p->pNextACMDriverID;
    if (p->pNextACMDriverID)
	p->pNextACMDriverID->pPrevACMDriverID = p->pPrevACMDriverID;
    
    pNextACMDriverID = p->pNextACMDriverID;
    
    HeapFree(MSACM_hHeap, 0, p);
    
    return pNextACMDriverID;
}

/***********************************************************************
 *           MSACM_UnregisterAllDrivers32()
 * FIXME
 *   Where should this function be called?
 */
void MSACM_UnregisterAllDrivers(void)
{
    PWINE_ACMDRIVERID p;

    for (p = MSACM_pFirstACMDriverID; p; p = MSACM_UnregisterDriver(p));
}

/***********************************************************************
 *           MSACM_GetDriverID32() 
 */
PWINE_ACMDRIVERID MSACM_GetDriverID(HACMDRIVERID hDriverID)
{
    return (PWINE_ACMDRIVERID)hDriverID;
}

/***********************************************************************
 *           MSACM_GetDriver32()
 */
PWINE_ACMDRIVER MSACM_GetDriver(HACMDRIVER hDriver)
{
    return (PWINE_ACMDRIVER)hDriver;
}

/***********************************************************************
 *           MSACM_GetObj32()
 */
PWINE_ACMOBJ MSACM_GetObj(HACMOBJ hObj)
{
    return (PWINE_ACMOBJ)hObj;
}


