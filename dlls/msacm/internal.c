/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 *		  1999	Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>

#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"
#include "mmsystem.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "wineacm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msacm);

/**********************************************************************/

HANDLE MSACM_hHeap = (HANDLE) NULL;
PWINE_ACMDRIVERID MSACM_pFirstACMDriverID = NULL;
PWINE_ACMDRIVERID MSACM_pLastACMDriverID = NULL;

#if 0
/***********************************************************************
 *           MSACM_DumpCache
 */
static	void MSACM_DumpCache(PWINE_ACMDRIVERID padid)
{
    unsigned 	i;

    TRACE("cFilterTags=%lu cFormatTags=%lu fdwSupport=%08lx\n",
	  padid->cFilterTags, padid->cFormatTags, padid->fdwSupport);
    for (i = 0; i < padid->cache->cFormatTags; i++) {
	TRACE("\tdwFormatTag=%lu cbwfx=%lu\n",
	      padid->aFormatTag[i].dwFormatTag, padid->aFormatTag[i].cbwfx);
    }
}
#endif

/***********************************************************************
 *           MSACM_FindFormatTagInCache 		[internal]
 *
 *	Returns TRUE is the format tag fmtTag is present in the cache.
 *	If so, idx is set to its index.
 */
BOOL MSACM_FindFormatTagInCache(WINE_ACMDRIVERID* padid, DWORD fmtTag, LPDWORD idx)
{
    unsigned 	i;

    for (i = 0; i < padid->cFormatTags; i++) {
	if (padid->aFormatTag[i].dwFormatTag == fmtTag) {
	    if (idx) *idx = i;
	    return TRUE;
	}
    }
    return FALSE;
}

/***********************************************************************
 *           MSACM_FillCache
 */
static BOOL MSACM_FillCache(PWINE_ACMDRIVERID padid)
{
    HACMDRIVER		        had = 0;
    int			        ntag;
    ACMDRIVERDETAILSW	        add;
    ACMFORMATTAGDETAILSW        aftd;

    if (acmDriverOpen(&had, (HACMDRIVERID)padid, 0) != 0)
	return FALSE;

    padid->aFormatTag = NULL;
    add.cbStruct = sizeof(add);
    if (MSACM_Message(had, ACMDM_DRIVER_DETAILS, (LPARAM)&add,  0))
	goto errCleanUp;

    if (add.cFormatTags > 0) {
	padid->aFormatTag = HeapAlloc(MSACM_hHeap, HEAP_ZERO_MEMORY,
				      add.cFormatTags * sizeof(padid->aFormatTag[0]));
	if (!padid->aFormatTag) goto errCleanUp;
    }

    padid->cFormatTags = add.cFormatTags;
    padid->cFilterTags = add.cFilterTags;
    padid->fdwSupport  = add.fdwSupport;

    aftd.cbStruct = sizeof(aftd);

    for (ntag = 0; ntag < add.cFormatTags; ntag++) {
	aftd.dwFormatTagIndex = ntag;
	if (MSACM_Message(had, ACMDM_FORMATTAG_DETAILS, (LPARAM)&aftd, ACM_FORMATTAGDETAILSF_INDEX)) {
	    TRACE("IIOs (%s)\n", padid->pszDriverAlias);
	    goto errCleanUp;
	}
	padid->aFormatTag[ntag].dwFormatTag = aftd.dwFormatTag;
	padid->aFormatTag[ntag].cbwfx = aftd.cbFormatSize;
    }

    acmDriverClose(had, 0);

    return TRUE;

errCleanUp:
    if (had) acmDriverClose(had, 0);
    HeapFree(MSACM_hHeap, 0, padid->aFormatTag);
    padid->aFormatTag = NULL;
    return FALSE;
}

/***********************************************************************
 *           MSACM_GetRegistryKey
 */
static	LPSTR	MSACM_GetRegistryKey(const WINE_ACMDRIVERID* padid)
{
    static const char*	baseKey = "Software\\Microsoft\\AudioCompressionManager\\DriverCache\\";
    LPSTR	ret;
    int		len;

    if (!padid->pszDriverAlias) {
	ERR("No alias needed for registry entry\n");
	return NULL;
    }
    len = strlen(baseKey);
    ret = HeapAlloc(MSACM_hHeap, 0, len + strlen(padid->pszDriverAlias) + 1);
    if (!ret) return NULL;

    strcpy(ret, baseKey);
    strcpy(ret + len, padid->pszDriverAlias);
    CharLowerA(ret + len);
    return ret;
}

/***********************************************************************
 *           MSACM_ReadCache
 */
static BOOL MSACM_ReadCache(PWINE_ACMDRIVERID padid)
{
    LPSTR	key = MSACM_GetRegistryKey(padid);
    HKEY	hKey;
    DWORD	type, size;

    if (!key) return FALSE;

    padid->aFormatTag = NULL;

    if (RegCreateKeyA(HKEY_LOCAL_MACHINE, key, &hKey))
	goto errCleanUp;

    size = sizeof(padid->cFormatTags);
    if (RegQueryValueExA(hKey, "cFormatTags", 0, &type, (void*)&padid->cFormatTags, &size))
	goto errCleanUp;
    size = sizeof(padid->cFilterTags);
    if (RegQueryValueExA(hKey, "cFilterTags", 0, &type, (void*)&padid->cFilterTags, &size))
	goto errCleanUp;
    size = sizeof(padid->fdwSupport);
    if (RegQueryValueExA(hKey, "fdwSupport", 0, &type, (void*)&padid->fdwSupport, &size))
	goto errCleanUp;

    if (padid->cFormatTags > 0) {
	size = padid->cFormatTags * sizeof(padid->aFormatTag[0]);
	padid->aFormatTag = HeapAlloc(MSACM_hHeap, HEAP_ZERO_MEMORY, size);
	if (!padid->aFormatTag) goto errCleanUp;
	if (RegQueryValueExA(hKey, "aFormatTagCache", 0, &type, (void*)padid->aFormatTag, &size))
	    goto errCleanUp;
    }
    HeapFree(MSACM_hHeap, 0, key);
    return TRUE;

 errCleanUp:
    HeapFree(MSACM_hHeap, 0, key);
    HeapFree(MSACM_hHeap, 0, padid->aFormatTag);
    padid->aFormatTag = NULL;
    RegCloseKey(hKey);
    return FALSE;
}

/***********************************************************************
 *           MSACM_WriteCache
 */
static	BOOL MSACM_WriteCache(PWINE_ACMDRIVERID padid)
{
    LPSTR	key = MSACM_GetRegistryKey(padid);
    HKEY	hKey;

    if (!key) return FALSE;

    if (RegCreateKeyA(HKEY_LOCAL_MACHINE, key, &hKey))
	goto errCleanUp;

    if (RegSetValueExA(hKey, "cFormatTags", 0, REG_DWORD, (void*)&padid->cFormatTags, sizeof(DWORD)))
	goto errCleanUp;
    if (RegSetValueExA(hKey, "cFilterTags", 0, REG_DWORD, (void*)&padid->cFilterTags, sizeof(DWORD)))
	goto errCleanUp;
    if (RegSetValueExA(hKey, "fdwSupport", 0, REG_DWORD, (void*)&padid->fdwSupport, sizeof(DWORD)))
	goto errCleanUp;
    if (RegSetValueExA(hKey, "aFormatTagCache", 0, REG_BINARY,
		       (void*)padid->aFormatTag,
		       padid->cFormatTags * sizeof(padid->aFormatTag[0])))
	goto errCleanUp;
    HeapFree(MSACM_hHeap, 0, key);
    return TRUE;

 errCleanUp:
    HeapFree(MSACM_hHeap, 0, key);
    return FALSE;
}

/***********************************************************************
 *           MSACM_RegisterDriver()
 */
PWINE_ACMDRIVERID MSACM_RegisterDriver(LPSTR pszDriverAlias, LPSTR pszFileName,
				       HINSTANCE hinstModule)
{
    PWINE_ACMDRIVERID	padid;

    TRACE("('%s', '%s', %p)\n", pszDriverAlias, pszFileName, hinstModule);

    padid = (PWINE_ACMDRIVERID) HeapAlloc(MSACM_hHeap, 0, sizeof(WINE_ACMDRIVERID));
    padid->obj.dwType = WINE_ACMOBJ_DRIVERID;
    padid->obj.pACMDriverID = padid;
    padid->pszDriverAlias = NULL;
    if (pszDriverAlias)
    {
        padid->pszDriverAlias = HeapAlloc( MSACM_hHeap, 0, strlen(pszDriverAlias)+1 );
        strcpy( padid->pszDriverAlias, pszDriverAlias );
    }
    padid->pszFileName = NULL;
    if (pszFileName)
    {
        padid->pszFileName = HeapAlloc( MSACM_hHeap, 0, strlen(pszFileName)+1 );
        strcpy( padid->pszFileName, pszFileName );
    }
    padid->hInstModule = hinstModule;

    padid->pACMDriverList = NULL;
    padid->pNextACMDriverID = NULL;
    padid->pPrevACMDriverID = MSACM_pLastACMDriverID;
    if (MSACM_pLastACMDriverID)
	MSACM_pLastACMDriverID->pNextACMDriverID = padid;
    MSACM_pLastACMDriverID = padid;
    if (!MSACM_pFirstACMDriverID)
	MSACM_pFirstACMDriverID = padid;
    /* disable the driver if we cannot load the cache */
    if (!MSACM_ReadCache(padid) && !MSACM_FillCache(padid)) {
	WARN("Couldn't load cache for ACM driver (%s)\n", pszFileName);
	MSACM_UnregisterDriver(padid);
	return NULL;
    }
    return padid;
}

/***********************************************************************
 *           MSACM_RegisterAllDrivers()
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

    /* FIXME: Does not work! How do I determine the section length? */
    dwBufferLength = 1024;
/* EPP 	GetPrivateProfileSectionA("drivers32", NULL, 0, "system.ini"); */

    pszBuffer = (LPSTR) HeapAlloc(MSACM_hHeap, 0, dwBufferLength);
    if (GetPrivateProfileSectionA("drivers32", pszBuffer, dwBufferLength, "system.ini")) {
	char* s = pszBuffer;
	while (*s) {
	    if (!strncasecmp("MSACM.", s, 6)) {
		char *s2 = s;
		while (*s2 != '\0' && *s2 != '=') s2++;
		if (*s2) {
		    *s2 = '\0';
		    MSACM_RegisterDriver(s, s2 + 1, 0);
		    *s2 = '=';
		}
	    }
	    s += strlen(s) + 1; /* Either next char or \0 */
	}
    }

    HeapFree(MSACM_hHeap, 0, pszBuffer);

    MSACM_RegisterDriver("msacm32.dll", "msacm32.dll", 0);
}

/***********************************************************************
 *           MSACM_UnregisterDriver()
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
    HeapFree(MSACM_hHeap, 0, p->aFormatTag);

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
 *           MSACM_UnregisterAllDrivers()
 */
void MSACM_UnregisterAllDrivers(void)
{
    PWINE_ACMDRIVERID p = MSACM_pFirstACMDriverID;

    while (p) {
	MSACM_WriteCache(p);
	p = MSACM_UnregisterDriver(p);
    }
}

/***********************************************************************
 *           MSACM_GetObj()
 */
PWINE_ACMOBJ MSACM_GetObj(HACMOBJ hObj, DWORD type)
{
    PWINE_ACMOBJ	pao = (PWINE_ACMOBJ)hObj;

    if (pao == NULL || IsBadReadPtr(pao, sizeof(WINE_ACMOBJ)) ||
	((type != WINE_ACMOBJ_DONTCARE) && (type != pao->dwType)))
	return NULL;
    return pao;
}

/***********************************************************************
 *           MSACM_GetDriverID()
 */
PWINE_ACMDRIVERID MSACM_GetDriverID(HACMDRIVERID hDriverID)
{
    return (PWINE_ACMDRIVERID)MSACM_GetObj((HACMOBJ)hDriverID, WINE_ACMOBJ_DRIVERID);
}

/***********************************************************************
 *           MSACM_GetDriver()
 */
PWINE_ACMDRIVER MSACM_GetDriver(HACMDRIVER hDriver)
{
    return (PWINE_ACMDRIVER)MSACM_GetObj((HACMOBJ)hDriver, WINE_ACMOBJ_DRIVER);
}

/***********************************************************************
 *           MSACM_Message()
 */
MMRESULT MSACM_Message(HACMDRIVER had, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    PWINE_ACMDRIVER	pad = MSACM_GetDriver(had);

    return pad ? SendDriverMessage(pad->hDrvr, uMsg, lParam1, lParam2) : MMSYSERR_INVALHANDLE;
}
