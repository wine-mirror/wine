/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * WINE Drivers functions
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1998 Marcus Meissner
 * Copyright 1999 Eric Pouech
 */

#include <string.h>
#include "heap.h"
#include "windef.h"
#include "wine/winbase16.h"
#include "wingdi.h"
#include "winuser.h"
#include "driver.h"
#include "ldt.h"
#include "module.h"
#include "mmddk.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(driver);

/* The following definitions are WINE internals */
/* FIXME: This is a WINE internal struct and should be moved in include/wine directory
 * Please note that WINE shares 16 and 32 bit drivers on a single list...
 * Basically, we maintain an external double view on drivers, so that a 16 bit drivers 
 * can be loaded/used... by 32 functions transparently 
 */

/* Who said goofy boy ? */
#define	WINE_DI_MAGIC	0x900F1B01

typedef struct tagWINE_DRIVER
{
    DWORD			dwMagic;
    char			szAliasName[128];
    /* as usual LPWINE_DRIVER == hDriver32 */
    HDRVR16			hDriver16;
    union {
       struct {
	  HMODULE16		hModule;
	  DRIVERPROC16          lpDrvProc;
       } d16;
       struct {
	  HMODULE		hModule;
	  DRIVERPROC		lpDrvProc;
       } d32;
    } d;
    DWORD		  	dwDriverID;
    DWORD			dwFlags;
    struct tagWINE_DRIVER*	lpPrevItem;
    struct tagWINE_DRIVER*	lpNextItem;
} WINE_DRIVER, *LPWINE_DRIVER;

LPWINE_DRIVER	DRIVER_RegisterDriver16(LPCSTR, HMODULE16, DRIVERPROC16, LPARAM, BOOL);
LPWINE_DRIVER	DRIVER_RegisterDriver32(LPCSTR, HMODULE,   DRIVERPROC,   LPARAM, BOOL);

static LPWINE_DRIVER	lpDrvItemList = NULL;

/* TODO list :
 *	- LoadModule count and clean up is not handled correctly (it's not a 
 *	  problem as long as FreeLibrary is not working correctly)
 *	- shoudln't the allocations be done on a per process basis ?
 *	- split 16/32 bit functions between DLLs as windows do (16 bit in USER, 32 bit in WINMM)
 */

/* ### start build ### */
extern LONG CALLBACK DRIVER_CallTo16_long_lwwll(FARPROC16,LONG,WORD,WORD,LONG,LONG);
/* ### stop build ### */

/**************************************************************************
 *			LoadStartupDrivers			[internal]
 */
static void WINE_UNUSED DRIVER_LoadStartupDrivers(void)
{
    char  	str[256];
    
    if (GetPrivateProfileStringA("drivers", NULL, "", str, sizeof(str), "SYSTEM.INI") < 2) {
    	ERR("Can't find drivers section in system.ini\n");
    } else {
	HDRVR16	hDrv;
	LPSTR	ptr;

	for (ptr = str; lstrlenA(ptr) != 0; ptr += lstrlenA(ptr) + 1) {
	    TRACE("str='%s'\n", ptr);
	    hDrv = OpenDriver16(ptr, "drivers", 0L);
	    TRACE("hDrv=%04x\n", hDrv);
	}
	TRACE("end of list !\n");
    }
}

/**************************************************************************
 *			DRIVER_GetNumberOfModuleRefs		[internal]
 *
 * Returns the number of open drivers which share the same module.
 */
static	WORD	DRIVER_GetNumberOfModuleRefs(LPWINE_DRIVER lpNewDrv)
{
    LPWINE_DRIVER	lpDrv;
    WORD		count = 0;
    
    for (lpDrv = lpDrvItemList; lpDrv; lpDrv = lpDrv->lpNextItem) {
	if (lpDrv->dwFlags & WINE_GDF_16BIT) {
	    if (lpDrv->d.d16.hModule == lpNewDrv->d.d16.hModule) {
		count++;
	    }
	} else {
	    if (lpDrv->d.d32.hModule == lpNewDrv->d.d32.hModule) {
		count++;
	    }
	}
    }
    return count;
}

/**************************************************************************
 *				DRIVER_FindFromHDrvr16		[internal]
 *
 * From a hDrvr being 16 bits, returns the WINE internal structure.
 */
static	LPWINE_DRIVER	DRIVER_FindFromHDrvr16(HDRVR16 hDrvr)
{    
    LPWINE_DRIVER	lpDrv;
    
    for (lpDrv = lpDrvItemList; lpDrv; lpDrv = lpDrv->lpNextItem) {
	if (lpDrv->hDriver16 == hDrvr) {
	    break;
	}
    }
    return lpDrv;
}

/**************************************************************************
 *				DRIVER_FindFromHDrvr		[internal]
 * 
 * From a hDrvr (being 16 or 32 bits), returns the WINE internal structure.
 */
static	LPWINE_DRIVER	DRIVER_FindFromHDrvr(HDRVR hDrvr)
{    
    if (!IsBadWritePtr((void*)hDrvr, sizeof(WINE_DRIVER)) && 
	((LPWINE_DRIVER)hDrvr)->dwMagic == WINE_DI_MAGIC) {
	return (LPWINE_DRIVER)hDrvr;
    }
    return DRIVER_FindFromHDrvr16(hDrvr);
}

/**************************************************************************
 *				DRIVER_MapMsg16To32		[internal]
 *
 * Map a 16 bit driver message to a 32 bit driver message.
 *  1 : ok, some memory allocated, need to call DRIVER_UnMapMsg16To32
 *  0 : ok, no memory allocated
 * -1 : ko, unknown message
 * -2 : ko, memory problem
 */
static int DRIVER_MapMsg16To32(WORD wMsg, DWORD* lParam1, DWORD* lParam2)
{
    int	ret = -1;
    
    switch (wMsg) {
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_DISABLE:
    case DRV_FREE:
    case DRV_QUERYCONFIGURE:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
    case DRV_EXITAPPLICATION:	
    case DRV_POWER:
	/* lParam1 and lParam2 are not used */
	ret = 0;
	break;
    case DRV_OPEN:
    case DRV_CLOSE:
	/* lParam1 is a NULL terminated string */
	/* lParam2 is a pointer to an MCI_OPEN_DRIVER_PARMS for an MCI device */
	if (*lParam1)
	    *lParam1 = (DWORD)PTR_SEG_TO_LIN(*lParam1);
	if (*lParam2 && wMsg == DRV_OPEN) {
            LPMCI_OPEN_DRIVER_PARMS16	modp16 = PTR_SEG_TO_LIN(*lParam2);
            char*			ptr = HeapAlloc(GetProcessHeap(), 0, sizeof(LPMCI_OPEN_DRIVER_PARMS16) + sizeof(MCI_OPEN_DRIVER_PARMSA));
            LPMCI_OPEN_DRIVER_PARMSA	modp32;
	    
	    if (ptr) {
		*(LPMCI_OPEN_DRIVER_PARMS16*)ptr = modp16;
		modp32 = (LPMCI_OPEN_DRIVER_PARMSA)(ptr + sizeof(LPMCI_OPEN_DRIVER_PARMSA));

		modp32->wDeviceID = modp16->wDeviceID;
		modp32->lpstrParams = PTR_SEG_TO_LIN(modp16->lpstrParams);
	    } else {
		return -2;
	    }
	    *lParam2 = (DWORD)modp32;
	}
	ret = 1;
	break;
    case DRV_CONFIGURE:
    case DRV_INSTALL:
	/* lParam1 is a handle to a window (or not used), lParam2 is a pointer to DRVCONFIGINFO */
	if (*lParam2) {
            LPDRVCONFIGINFO	dci32 = HeapAlloc(GetProcessHeap(), 0, sizeof(DRVCONFIGINFO));
            LPDRVCONFIGINFO16	dci16 = PTR_SEG_TO_LIN(*lParam2);
	    
	    if (dci32) {
		dci32->dwDCISize = sizeof(DRVCONFIGINFO);
		dci32->lpszDCISectionName = HEAP_strdupAtoW(GetProcessHeap(), 0, PTR_SEG_TO_LIN(dci16->lpszDCISectionName));
		dci32->lpszDCIAliasName   = HEAP_strdupAtoW(GetProcessHeap(), 0, PTR_SEG_TO_LIN(dci16->lpszDCIAliasName));
		if (dci32->lpszDCISectionName == NULL || dci32->lpszDCIAliasName == NULL)
		    return -2;
	    } else {
		return -2;
	    }
	    *lParam2 = (DWORD)dci32;
	    ret = 1;
	} else {
	    ret = 0;
	}
	break;
    default:
	if ((wMsg >= 0x800 && wMsg < 0x900) || (wMsg >= 0x4000 && wMsg < 0x4100 )) {
	    /* FIXME: another hack to handle MCI and ICM messages... 
	     * should find a *NICE* way to integrate DRIVER_ and
	     * MCI_ mapping/unmapping functions
	     */
	    ret = 0;
	} else {
	   FIXME("Unknown message 0x%04x\n", wMsg);
	}
    }
    return ret;
}

/**************************************************************************
 *				DRIVER_MapMsg16To32		[internal]
 *
 * UnMap a 16 bit driver message to a 32 bit driver message.
 *  0 : ok
 * -1 : ko
 * -2 : ko, memory problem
 */
static int DRIVER_UnMapMsg16To32(WORD wMsg, DWORD lParam1, DWORD lParam2)
{
    int	ret = -1;
    
    switch (wMsg) {
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_DISABLE:
    case DRV_FREE:
    case DRV_QUERYCONFIGURE:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
    case DRV_EXITAPPLICATION:
    case DRV_POWER:
	/* lParam1 and lParam2 are not used */
    case DRV_OPEN:
    case DRV_CLOSE:
	/* lParam1 is a NULL terminated string */
	/* lParam2 is a pointer to an MCI_OPEN_DRIVER_PARMS for an MCI device */
	if (lParam2 && wMsg == DRV_OPEN) {
	    LPMCI_OPEN_DRIVER_PARMSA	modp32 = (LPMCI_OPEN_DRIVER_PARMSA)lParam2;
	    LPMCI_OPEN_DRIVER_PARMS16	modp16 = *(LPMCI_OPEN_DRIVER_PARMS16*)(lParam2 - sizeof(LPMCI_OPEN_DRIVER_PARMSA));

	    modp16->wCustomCommandTable = modp32->wCustomCommandTable;
	    modp16->wType = modp32->wType;
	    if (!HeapFree(GetProcessHeap(), 0, modp32))
		FIXME("bad free line=%d\n", __LINE__);
	}
	ret = 0;
	break;
    case DRV_CONFIGURE: 
    case DRV_INSTALL:
	/* lParam1 is a handle to a window (or not used), lParam2 is a pointer to DRVCONFIGINFO, lParam2 */
	if (lParam2) {
	    LPDRVCONFIGINFO	dci32 = (LPDRVCONFIGINFO)lParam2;
	    if (!HeapFree(GetProcessHeap(), 0, (LPVOID)dci32->lpszDCISectionName))
		FIXME("bad free line=%d\n", __LINE__);
	    if (!HeapFree(GetProcessHeap(), 0, (LPVOID)dci32->lpszDCIAliasName))
		FIXME("bad free line=%d\n", __LINE__);
	    if (!HeapFree(GetProcessHeap(), 0, dci32))
		FIXME("bad free line=%d\n", __LINE__);
	}
	ret = 0;
	break;
    default:
	if ((wMsg >= 0x800 && wMsg < 0x900) || (wMsg >= 0x4000 && wMsg < 0x4100 )) {
	    /* FIXME: another hack to handle MCI and ICM messages... 
	     * should find a *NICE* way to integrate DRIVER_ and
	     * MCI_ mapping/unmapping functions
	     */
	    ret = 0;
	} else {
	   FIXME("Unknown message 0x%04x\n", wMsg);
	}	
    }
    return ret;
}

/**************************************************************************
 *				DRIVER_MapMsg32To16		[internal]
 *
 * Map a 32 bit driver message to a 16 bit driver message.
 *  1 : ok, some memory allocated, need to call DRIVER_UnMapMsg32To16
 *  0 : ok, no memory allocated
 * -1 : ko, unknown message
 * -2 : ko, memory problem
 */
static int DRIVER_MapMsg32To16(WORD wMsg, DWORD* lParam1, DWORD* lParam2)
{
    int	ret = -1;
    
    switch (wMsg) {
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_DISABLE:
    case DRV_FREE:
    case DRV_QUERYCONFIGURE:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
    case DRV_EXITAPPLICATION:	
    case DRV_POWER:
	/* lParam1 and lParam2 are not used */
	ret = 0;
	break;
    case DRV_OPEN:
    case DRV_CLOSE:
	/* lParam1 is a NULL terminated string */
	/* lParam2 is a pointer to an MCI_OPEN_DRIVER_PARMS for an MCI device */
        if (*lParam1) {
            LPSTR str = SEGPTR_STRDUP((LPSTR)*lParam1);
            if (str) {
		*lParam1 = (LPARAM)SEGPTR_GET(str);
		ret = 0;
	    } else {
		ret = -2;
	    }
        } else {
	    ret = 0;
	}
	if (*lParam2 && wMsg == DRV_OPEN) {
            LPMCI_OPEN_DRIVER_PARMS16	modp16;
	    char* 			ptr = SEGPTR_ALLOC(sizeof(LPMCI_OPEN_DRIVER_PARMSA) + sizeof(MCI_OPEN_DRIVER_PARMS16));
            LPMCI_OPEN_DRIVER_PARMSA	modp32 = (LPMCI_OPEN_DRIVER_PARMSA)(*lParam2);
	    
	    if (ptr) {
		*(LPMCI_OPEN_DRIVER_PARMSA*)ptr = modp32;
		modp16 = (LPMCI_OPEN_DRIVER_PARMS16)(ptr + sizeof(LPMCI_OPEN_DRIVER_PARMSA));

		modp16->wDeviceID = modp32->wDeviceID;
		modp16->lpstrParams = PTR_SEG_TO_LIN(modp32->lpstrParams);
	    } else {
		return -2;
	    }
	    *lParam2 = (DWORD)SEGPTR_GET(modp16);
	    ret = 1;
	}
	break;
    case DRV_CONFIGURE:
    case DRV_INSTALL:
	/* lParam1 is a handle to a window (or not used), lParam2 is a pointer to DRVCONFIGINFO */
	if (*lParam2) {
            LPDRVCONFIGINFO16	dci16 = (LPDRVCONFIGINFO16)SEGPTR_ALLOC(sizeof(DRVCONFIGINFO16));
            LPDRVCONFIGINFO	dci32 = (LPDRVCONFIGINFO)(*lParam2);
	    
	    if (dci16) {
		LPSTR	str1, str2;
		
		dci16->dwDCISize = sizeof(DRVCONFIGINFO16);
		
		if ((str1 = HEAP_strdupWtoA(GetProcessHeap(), 0, dci32->lpszDCISectionName)) != NULL &&
		    (str2 = SEGPTR_STRDUP(str1)) != NULL) {
		    dci16->lpszDCISectionName = (LPSTR)SEGPTR_GET(str2);
		    if (!HeapFree(GetProcessHeap(), 0, str1))
			FIXME("bad free line=%d\n", __LINE__);
		} else {
		    return -2;
		}
		if ((str1 = HEAP_strdupWtoA(GetProcessHeap(), 0, dci32->lpszDCIAliasName)) != NULL &&
		    (str2 = SEGPTR_STRDUP(str1)) != NULL) {
		    dci16->lpszDCIAliasName = (LPSTR)SEGPTR_GET(str2);
		    if (!HeapFree(GetProcessHeap(), 0, str1))
			FIXME("bad free line=%d\n", __LINE__);
		} else {
		    return -2;
		}
	    } else {
		return -2;
	    }
	    *lParam2 = (LPARAM)SEGPTR_GET(dci16);
	    ret = 1;
	} else {
	    ret = 0;
	}
	break;
    default:
	if ((wMsg >= 0x800 && wMsg < 0x900) || (wMsg >= 0x4000 && wMsg < 0x4100 )) {
	    /* FIXME: another hack to handle MCI and ICM messages... 
	     * should find a *NICE* way to integrate DRIVER_ and
	     * MCI_ mapping/unmapping functions
	     */
	    ret = 0;
	} else {
	   FIXME("Unknown message 0x%04x\n", wMsg);
	}	
    }
    return ret;
}

/**************************************************************************
 *				DRIVER_UnMapMsg32To16		[internal]
 *
 * UnMap a 32 bit driver message to a 16 bit driver message.
 *  0 : ok
 * -1 : ko
 * -2 : ko, memory problem
 */
static int DRIVER_UnMapMsg32To16(WORD wMsg, DWORD lParam1, DWORD lParam2)
{
    int	ret = -1;
    
    switch (wMsg) {
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_DISABLE:
    case DRV_FREE:
    case DRV_QUERYCONFIGURE:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
    case DRV_EXITAPPLICATION:
    case DRV_POWER:
	/* lParam1 and lParam2 are not used */
    case DRV_OPEN:
    case DRV_CLOSE:
	/* lParam1 is a NULL terminated string, lParam2 is unknown => may lead to some problem */
	/* lParam2 is a pointer to an MCI_OPEN_DRIVER_PARMS for an MCI device */
	if (lParam1) if (!SEGPTR_FREE(PTR_SEG_TO_LIN(lParam1)))
	    FIXME("bad free line=%d\n", __LINE__);

	if (lParam2 && wMsg == DRV_OPEN) {
	    LPMCI_OPEN_DRIVER_PARMS16	modp16 = (LPMCI_OPEN_DRIVER_PARMS16)PTR_SEG_TO_LIN(lParam2);
	    LPMCI_OPEN_DRIVER_PARMSA	modp32 = *(LPMCI_OPEN_DRIVER_PARMSA*)((char*)modp16 - sizeof(LPMCI_OPEN_DRIVER_PARMSA));

	    modp32->wCustomCommandTable = modp16->wCustomCommandTable;
	    modp32->wType = modp16->wType;
	    if (!SEGPTR_FREE((char*)modp16 - sizeof(LPMCI_OPEN_DRIVER_PARMSA)))
		FIXME("bad free line=%d\n", __LINE__);
	}
	ret = 0;
	break;
    case DRV_CONFIGURE: 
    case DRV_INSTALL:
	/* lParam1 is a handle to a window (or not used), lParam2 is a pointer to DRVCONFIGINFO, lParam2 */
	if (lParam2) {
	    LPDRVCONFIGINFO16	dci16 = (LPDRVCONFIGINFO16)PTR_SEG_TO_LIN(lParam2);
	    
	    if (!SEGPTR_FREE(PTR_SEG_TO_LIN(dci16->lpszDCISectionName)))
		FIXME("bad free line=%d\n", __LINE__);
	    if (!SEGPTR_FREE(PTR_SEG_TO_LIN(dci16->lpszDCIAliasName)))
		FIXME("bad free line=%d\n", __LINE__);
	    if (!SEGPTR_FREE(dci16))
		FIXME("bad free line=%d\n", __LINE__);
	}
	ret = 0;
	break;
    default:
	if ((wMsg >= 0x800 && wMsg < 0x900) || (wMsg >= 0x4000 && wMsg < 0x4100 )) {
	    /* FIXME: another hack to handle MCI and ICM messages... 
	     * should find a *NICE* way to integrate DRIVER_ and
	     * MCI_ mapping/unmapping functions
	     */
	    ret = 0;
	} else {
	   FIXME("Unknown message 0x%04x\n", wMsg);
	}
    }
    return ret;
}

/**************************************************************************
 *				SendDriverMessage		[USER.251]
 */
LRESULT WINAPI SendDriverMessage16(HDRVR16 hDriver, UINT16 msg, LPARAM lParam1,
                                   LPARAM lParam2)
{
    LPWINE_DRIVER 	lpDrv;
    LRESULT 		retval = 0;
    int			mapRet;
    
    TRACE("(%04x, %04X, %08lX, %08lX)\n", hDriver, msg, lParam1, lParam2);
    
    lpDrv = DRIVER_FindFromHDrvr16(hDriver);
    if (lpDrv != NULL && lpDrv->hDriver16 == hDriver) {
	if (lpDrv->dwFlags & WINE_GDF_16BIT) {
	    TRACE("Before CallDriverProc proc=%p driverID=%08lx hDrv=%u wMsg=%04x p1=%08lx p2=%08lx\n", 
		  lpDrv->d.d16.lpDrvProc, lpDrv->dwDriverID, hDriver, msg, lParam1, lParam2);		  
	    retval = DRIVER_CallTo16_long_lwwll((FARPROC16)lpDrv->d.d16.lpDrvProc, lpDrv->dwDriverID, 
						hDriver, msg, lParam1, lParam2);
	} else {
	    mapRet = DRIVER_MapMsg16To32(msg, &lParam1, &lParam2);
	    if (mapRet >= 0) {
		TRACE("Before func32 call proc=%p driverID=%08lx hDrv=%u wMsg=%04x p1=%08lx p2=%08lx\n", 
		      lpDrv->d.d32.lpDrvProc, lpDrv->dwDriverID, (HDRVR)lpDrv, msg, lParam1, lParam2);		  
		retval = lpDrv->d.d32.lpDrvProc(lpDrv->dwDriverID, (HDRVR)lpDrv, msg, lParam1, lParam2);
		if (mapRet >= 1) {
		    DRIVER_UnMapMsg16To32(msg, lParam1, lParam2);
		}
	    } else {
		retval = 0;
	    }
	}
    } else {
	WARN("Bad driver handle %u\n", hDriver);
    }
    
    TRACE("retval = %ld\n", retval);
    return retval;
}

/**************************************************************************
 *				SendDriverMessage		[WINMM.19]
 */
LRESULT WINAPI SendDriverMessage(HDRVR hDriver, UINT msg, LPARAM lParam1,
				 LPARAM lParam2)
{
    LPWINE_DRIVER	lpDrv;
    LRESULT 		retval = 0;
    int			mapRet;
    
    TRACE("(%04x, %04X, %08lX, %08lX)\n", hDriver, msg, lParam1, lParam2);
    
    lpDrv = DRIVER_FindFromHDrvr(hDriver);

    if (lpDrv != NULL) {
	if (lpDrv->dwFlags & WINE_GDF_16BIT) {
	    mapRet = DRIVER_MapMsg32To16(msg, &lParam1, &lParam2);
	    if (mapRet >= 0) {
		TRACE("Before CallDriverProc proc=%p driverID=%08lx hDrv=%u wMsg=%04x p1=%08lx p2=%08lx\n", 
		      lpDrv->d.d16.lpDrvProc, lpDrv->dwDriverID, lpDrv->hDriver16, msg, lParam1, lParam2);		  
		retval = DRIVER_CallTo16_long_lwwll((FARPROC16)lpDrv->d.d16.lpDrvProc, lpDrv->dwDriverID, 
						    lpDrv->hDriver16, msg, lParam1, lParam2);
		if (mapRet >= 1) {
		    DRIVER_UnMapMsg32To16(msg, lParam1, lParam2);
		}
	    } else {
		retval = 0;
	    }
	} else {
	    TRACE("Before func32 call proc=%p driverID=%08lx hDrv=%u wMsg=%04x p1=%08lx p2=%08lx\n", 
		  lpDrv->d.d32.lpDrvProc, lpDrv->dwDriverID, hDriver, msg, lParam1, lParam2);		  
	    retval = lpDrv->d.d32.lpDrvProc(lpDrv->dwDriverID, hDriver, msg, lParam1, lParam2);
	}
    } else {
	WARN("Bad driver handle %u\n", hDriver);
    }
    TRACE("retval = %ld\n", retval);
    
    return retval;
}

/**************************************************************************
 *				DRIVER_RemoveFromList		[internal]
 *
 * Generates all the logic to handle driver closure / deletion
 * Removes a driver struct to the list of open drivers.
 */
static	BOOL	DRIVER_RemoveFromList(LPWINE_DRIVER lpDrv)
{
    lpDrv->dwDriverID = 0;
    if (DRIVER_GetNumberOfModuleRefs(lpDrv) == 1) {
	SendDriverMessage((HDRVR)lpDrv, DRV_DISABLE, 0L, 0L);
	SendDriverMessage((HDRVR)lpDrv, DRV_FREE,    0L, 0L);
    }
    
    if (lpDrv->lpPrevItem)
	lpDrv->lpPrevItem->lpNextItem = lpDrv->lpNextItem;
    else
	lpDrvItemList = lpDrv->lpNextItem;
    if (lpDrv->lpNextItem)
	lpDrv->lpNextItem->lpPrevItem = lpDrv->lpPrevItem;

    return TRUE;
}

/**************************************************************************
 *				DRIVER_AddToList		[internal]
 *
 * Adds a driver struct to the list of open drivers.
 * Generates all the logic to handle driver creation / open.
 */
static	BOOL	DRIVER_AddToList(LPWINE_DRIVER lpNewDrv, LPARAM lParam, BOOL bCallFrom32)
{
    lpNewDrv->dwMagic = WINE_DI_MAGIC;
    /* First driver to be loaded for this module, need to load correctly the module */
    if (DRIVER_GetNumberOfModuleRefs(lpNewDrv) == 0) {
	if (SendDriverMessage((HDRVR)lpNewDrv, DRV_LOAD, 0L, 0L) != DRV_SUCCESS) {
	    TRACE("DRV_LOAD failed on driver 0x%08lx\n", (DWORD)lpNewDrv);
	    return FALSE;
	}
	if (SendDriverMessage((HDRVR)lpNewDrv, DRV_ENABLE, 0L, 0L) != DRV_SUCCESS) {
	    TRACE("DRV_ENABLE failed on driver 0x%08lx\n", (DWORD)lpNewDrv);
	    return FALSE;
	}
    }

    lpNewDrv->lpNextItem = NULL;
    if (lpDrvItemList == NULL) {
	lpDrvItemList = lpNewDrv;
	lpNewDrv->lpPrevItem = NULL;
    } else {
	LPWINE_DRIVER	lpDrv = lpDrvItemList;	/* find end of list */
	while (lpDrv->lpNextItem != NULL)
	    lpDrv = lpDrv->lpNextItem;
	
	lpDrv->lpNextItem = lpNewDrv;
	lpNewDrv->lpPrevItem = lpDrv;
    }
    /* Now just open a new instance of a driver on this module */
    if (bCallFrom32) {
	lpNewDrv->dwDriverID = SendDriverMessage((HDRVR)lpNewDrv, DRV_OPEN, 0L, lParam);
    } else {
	lpNewDrv->dwDriverID = SendDriverMessage16(lpNewDrv->hDriver16, DRV_OPEN, 0L, lParam);
    }
    if (lpNewDrv->dwDriverID == 0) {
	TRACE("DRV_OPEN failed on driver 0x%08lx\n", (DWORD)lpNewDrv);
	DRIVER_RemoveFromList(lpNewDrv);
	return FALSE;
    }
    
    return TRUE;
}

/**************************************************************************
 *				DRIVER_CreateDrvr16		[internal]
 *
 * Creates unique ID for 16 bit drivers.
 */
static	HDRVR16	DRIVER_CreateDrvr16()
{
    static	WORD	DRIVER_hDrvr16Counter = 0;
    
    while (DRIVER_FindFromHDrvr16(++DRIVER_hDrvr16Counter));
    return DRIVER_hDrvr16Counter;
}

/**************************************************************************
 *				DRIVER_CloseDriver		[internal]
 *
 */
BOOL DRIVER_CloseDriver(LPWINE_DRIVER lpDrv, DWORD lParam1, DWORD lParam2)
{
    if (lpDrv != NULL) {
	SendDriverMessage((HDRVR)lpDrv, DRV_CLOSE, lParam1, lParam2);
    
	if (DRIVER_RemoveFromList(lpDrv)) {
	    HeapFree(GetProcessHeap(), 0, lpDrv);
	    return TRUE;
	}
    }
    WARN("Failed to close driver\n");
    return FALSE;
}

/**************************************************************************
 *				DRIVER_RegisterDriver16		[internal]
 *
 * Creates all the WINE internal representations for a 16 bit driver.
 * The driver is also open by sending the correct messages.
 */
LPWINE_DRIVER DRIVER_RegisterDriver16(LPCSTR lpName, HMODULE16 hModule, DRIVERPROC16 lpProc, 
				      LPARAM lParam, BOOL bCallFrom32)
{
    LPWINE_DRIVER	lpDrv;
    
    lpDrv = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_DRIVER));
    if (lpDrv != NULL) {
	lpDrv->dwFlags         = WINE_GDF_EXIST|WINE_GDF_16BIT;
	lpDrv->dwDriverID      = 0;
	lpDrv->hDriver16       = DRIVER_CreateDrvr16();
	lstrcpynA(lpDrv->szAliasName, lpName, sizeof(lpDrv->szAliasName));
	lpDrv->d.d16.hModule   = hModule;
	lpDrv->d.d16.lpDrvProc = lpProc;
	
	if (!DRIVER_AddToList(lpDrv, lParam, bCallFrom32)) {
	    HeapFree(GetProcessHeap(), 0, lpDrv);
	    lpDrv = NULL;
	}
    }
    return lpDrv;
}

/**************************************************************************
 *				DRIVER_RegisterDriver32		[internal]
 *
 * Creates all the WINE internal representations for a 32 bit driver.
 * The driver is also open by sending the correct messages.
 */
LPWINE_DRIVER DRIVER_RegisterDriver32(LPCSTR lpName, HMODULE hModule, DRIVERPROC lpProc, 
				      LPARAM lParam, BOOL bCallFrom32)
{
    LPWINE_DRIVER	lpDrv;
    
    lpDrv = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_DRIVER));
    if (lpDrv != NULL) {
	lpDrv->dwFlags          = WINE_GDF_EXIST;
	lpDrv->dwDriverID       = 0;
	lpDrv->hDriver16        = DRIVER_CreateDrvr16();
	lstrcpynA(lpDrv->szAliasName, lpName, sizeof(lpDrv->szAliasName));
	lpDrv->d.d32.hModule    = hModule;
	lpDrv->d.d32.lpDrvProc  = lpProc;
	
	if (!DRIVER_AddToList(lpDrv, lParam, bCallFrom32)) {
	    HeapFree(GetProcessHeap(), 0, lpDrv);
	    lpDrv = NULL;
	}
    }
    return lpDrv;
}

/**************************************************************************
 *				DRIVER_TryOpenDriver16		[internal]
 *
 * Tries to load a 16 bit driver whose DLL's (module) name is lpFileName.
 */
static	HDRVR16	DRIVER_TryOpenDriver16(LPCSTR lpFileName, LPARAM lParam, BOOL bCallFrom32)
{
    LPWINE_DRIVER 	lpDrv = NULL;
    LPCSTR		lpSFN;
    HMODULE16		hModule;
    DRIVERPROC16	lpProc;
    
    TRACE("('%s', %08lX, %d);\n", lpFileName, lParam, bCallFrom32);
    
    if (lstrlenA(lpFileName) < 1) 
	return 0;
    
    lpSFN = strrchr(lpFileName, '\\');
    lpSFN = (lpSFN) ? (lpSFN + 1) : lpFileName;
    
    if ((hModule = LoadModule16(lpFileName, (LPVOID)-1)) >= 32) {
	if ((lpProc = (DRIVERPROC16)WIN32_GetProcAddress16(hModule, "DRIVERPROC")) != NULL) {
	    lpDrv = DRIVER_RegisterDriver16(lpSFN, hModule, lpProc, lParam, bCallFrom32);
	} else {
	    FreeLibrary16(hModule);
	    TRACE("No DriverProc found\n");
	    lpDrv = 0;
	}
    } else {
	TRACE("Unable to load 16 bit module (%s): %d\n", lpFileName, hModule);
    }
    return lpDrv ? lpDrv->hDriver16 : 0;
}

/**************************************************************************
 *				DRIVER_TryOpenDriver32		[internal]
 *
 * Tries to load a 32 bit driver whose DLL's (module) name is lpFileName.
 */
static	HDRVR	DRIVER_TryOpenDriver32(LPCSTR lpFileName, LPARAM lParam, BOOL bCallFrom32)
{
    LPWINE_DRIVER 	lpDrv = NULL;
    LPCSTR		lpSFN;
    HMODULE		hModule;
    DRIVERPROC		lpProc;
    
    TRACE("('%s', %08lX, %d);\n", lpFileName, lParam, bCallFrom32);
    
    if (lstrlenA(lpFileName) < 1) 
	return 0;
    
    lpSFN = strrchr(lpFileName, '\\');
    lpSFN = (lpSFN) ? (lpSFN + 1) : lpFileName;
    
    if ((hModule = LoadLibraryA(lpFileName)) != 0) {
	if ((lpProc = GetProcAddress(hModule, "DriverProc")) != NULL) {
	    lpDrv = DRIVER_RegisterDriver32(lpSFN, hModule, lpProc, lParam, bCallFrom32);
	} else {
	    FreeLibrary(hModule);
	    lpDrv = 0;
	    TRACE("No DriverProc found\n");
	}
    } else {
	TRACE("Unable to load 32 bit module \"%s\"\n", lpFileName);
    }
    TRACE("=> %p\n", lpDrv);
    return (HDRVR)lpDrv;
}

/**************************************************************************
 *				OpenDriver16		        [USER.252]
 */
HDRVR16 WINAPI OpenDriver16(LPCSTR lpDriverName, LPCSTR lpSectionName, LPARAM lParam)
{
    HDRVR16 		hDriver = 0;
    char		drvName[128];
    
    TRACE("('%s', '%s', %08lX);\n", lpDriverName, lpSectionName, lParam);
    
    if (lpSectionName == NULL) {
	hDriver = DRIVER_TryOpenDriver16(lpDriverName, lParam, FALSE);
	if (!hDriver) {
	    hDriver = DRIVER_TryOpenDriver32(lpDriverName, lParam, FALSE);
	}
	if (!hDriver) {
	    /* in case hDriver is NULL, search in Drivers32 section */
	    lpSectionName = "Drivers";
	}
    }
    if (!hDriver && GetPrivateProfileStringA(lpSectionName, lpDriverName, "", 
					     drvName, sizeof(drvName), "SYSTEM.INI") > 0) {
	hDriver = DRIVER_TryOpenDriver16(drvName, lParam, FALSE);
    }
    if (!hDriver)
	ERR("Failed to open driver %s from system.ini file, section %s\n", lpDriverName, lpSectionName);
    else 
	TRACE("=> %08x\n", hDriver);
    return hDriver;
}

/**************************************************************************
 *				OpenDriverA		        [WINMM.15]
 * (0,1,DRV_LOAD  ,0       ,0)
 * (0,1,DRV_ENABLE,0       ,0)
 * (0,1,DRV_OPEN  ,buf[256],0)
 */
HDRVR WINAPI OpenDriverA(LPCSTR lpDriverName, LPCSTR lpSectionName, LPARAM lParam) 
{
    HDRVR 		hDriver = 0;
    char 		drvName[128];

    TRACE("(%s, %s, 0x%08lx);\n", debugstr_a(lpDriverName), debugstr_a(lpSectionName), lParam);
    
    if (lpSectionName == NULL) {
	lstrcpynA(drvName, lpDriverName, sizeof(drvName));
	hDriver = DRIVER_TryOpenDriver32(lpDriverName, lParam, TRUE);
	if (!hDriver) {
	    hDriver = DRIVER_TryOpenDriver16(lpDriverName, lParam, TRUE);
	}
	if (!hDriver) {
	    if (GetPrivateProfileStringA("Drivers32", lpDriverName, "", drvName,
					 sizeof(drvName), "SYSTEM.INI")) {
		hDriver = DRIVER_TryOpenDriver32(drvName, lParam, TRUE);
		
	    }
	}
	if (!hDriver) {
	    if (GetPrivateProfileStringA("Drivers", lpDriverName, "", drvName,
					 sizeof(drvName), "SYSTEM.INI")) {
		hDriver = DRIVER_TryOpenDriver16(drvName, lParam, TRUE);
		
	    }
	}
    } else {
	if (GetPrivateProfileStringA(lpSectionName, lpDriverName, "", drvName,
				     sizeof(drvName), "SYSTEM.INI")) {
	    hDriver = DRIVER_TryOpenDriver32(drvName, lParam, TRUE);
	    if (!hDriver) {
		hDriver = DRIVER_TryOpenDriver16(drvName, lParam, TRUE);
	    }
	}
    }
    if (!hDriver)
	ERR("Failed to open driver %s from section %s\n", lpDriverName, lpSectionName);
    else 
	TRACE("=> %08x\n", hDriver);
    return hDriver;
}

/**************************************************************************
 *				OpenDriverW		        [WINMM.15]
 */
HDRVR WINAPI OpenDriverW(LPCWSTR lpDriverName, LPCWSTR lpSectionName, LPARAM lParam)
{
    LPSTR 		dn = HEAP_strdupWtoA(GetProcessHeap(), 0, lpDriverName);
    LPSTR 		sn = HEAP_strdupWtoA(GetProcessHeap(), 0, lpSectionName);
    HDRVR		ret = OpenDriverA(dn, sn, lParam);
    
    if (dn) HeapFree(GetProcessHeap(), 0, dn);
    if (sn) HeapFree(GetProcessHeap(), 0, sn);
    return ret;
}

/**************************************************************************
 *			CloseDriver16				[USER.253]
 */
LRESULT WINAPI CloseDriver16(HDRVR16 hDrvr, LPARAM lParam1, LPARAM lParam2)
{
    TRACE("(%04x, %08lX, %08lX);\n", hDrvr, lParam1, lParam2);
    
    return DRIVER_CloseDriver(DRIVER_FindFromHDrvr16(hDrvr), lParam1, lParam2);
}

/**************************************************************************
 *			CloseDriver				[WINMM.4]
 */
LRESULT WINAPI CloseDriver(HDRVR hDrvr, LPARAM lParam1, LPARAM lParam2)
{
    TRACE("(%04x, %08lX, %08lX);\n", hDrvr, lParam1, lParam2);
    
    return DRIVER_CloseDriver(DRIVER_FindFromHDrvr(hDrvr), lParam1, lParam2);
}

/**************************************************************************
 *				GetDriverModuleHandle	[USER.254]
 */
HMODULE16 WINAPI GetDriverModuleHandle16(HDRVR16 hDrvr)
{
    LPWINE_DRIVER 	lpDrv;
    HMODULE16 		hModule = 0;
    
    TRACE("(%04x);\n", hDrvr);
    
    lpDrv = DRIVER_FindFromHDrvr16(hDrvr);
    if (lpDrv != NULL && lpDrv->hDriver16 == hDrvr && (lpDrv->dwFlags & WINE_GDF_16BIT)) {
	hModule = lpDrv->d.d16.hModule;
    }
    TRACE("=> %d\n", hModule);
    return hModule;
}

/**************************************************************************
 *				GetDriverFlags		[WINMM.13]
 * [in] hDrvr handle to the driver
 *
 * Returns:
 *	0x00000000 if hDrvr is an invalid handle
 *	0x80000000 if hDrvr is a valid 32 bit driver
 *	0x90000000 if hDrvr is a valid 16 bit driver
 */
DWORD	WINAPI GetDriverFlags(HDRVR hDrvr)
{
    LPWINE_DRIVER 	lpDrv;
    DWORD		ret = 0;

    TRACE("(%04x)\n", hDrvr);

    if ((lpDrv = DRIVER_FindFromHDrvr(hDrvr)) != NULL) {
	ret = lpDrv->dwFlags;
    }
    return ret;
}

/**************************************************************************
 *				GetDriverModuleHandle	[WINMM.14]
 */
HMODULE WINAPI GetDriverModuleHandle(HDRVR hDrvr)
{
    LPWINE_DRIVER 	lpDrv;
    HMODULE		hModule = 0;
    
    TRACE("(%04x);\n", hDrvr);
    
    lpDrv = DRIVER_FindFromHDrvr(hDrvr);
    if (lpDrv != NULL && !(lpDrv->dwFlags & WINE_GDF_16BIT)) {
	hModule = lpDrv->d.d32.hModule;
    }
    TRACE("=> %d\n", hModule);
    return hModule;
}

/**************************************************************************
 *				DefDriverProc16			[USER.255]
 */
LRESULT WINAPI DefDriverProc16(DWORD dwDevID, HDRVR16 hDriv, UINT16 wMsg, 
                               LPARAM lParam1, LPARAM lParam2)
{
    TRACE("devID=0x%08lx hDrv=0x%04x wMsg=%04x lP1=0x%08lx lP2=0x%08lx\n",
	  dwDevID, hDriv, wMsg, lParam1, lParam2);

    switch(wMsg) {
    case DRV_LOAD:		
    case DRV_FREE:		
    case DRV_ENABLE:
    case DRV_DISABLE:
	return (LRESULT)1L;
    case DRV_OPEN:		
    case DRV_CLOSE:		
    case DRV_QUERYCONFIGURE:
	return (LRESULT)0L;
    case DRV_CONFIGURE:		
	MessageBoxA(0, "Driver isn't configurable !", "Wine Driver", MB_OK); 
	return (LRESULT)0L;
    case DRV_INSTALL:		
    case DRV_REMOVE:		
	return DRV_SUCCESS;
    default:			
	return (LRESULT)0L;
    }
}

/**************************************************************************
 *				GetDriverInfo			[USER.256]
 */
BOOL16 WINAPI GetDriverInfo16(HDRVR16 hDrvr, LPDRIVERINFOSTRUCT16 lpDrvInfo)
{
    LPWINE_DRIVER 	lpDrv;
    BOOL16		ret = FALSE;
    
    TRACE("(%04x, %p);\n", hDrvr, lpDrvInfo);
    
    if (lpDrvInfo == NULL || lpDrvInfo->length != sizeof(DRIVERINFOSTRUCT16)) 
	return FALSE;
    
    lpDrv = DRIVER_FindFromHDrvr16(hDrvr);
    if (lpDrv != NULL && lpDrv->hDriver16 == hDrvr && 
	(lpDrv->dwFlags & WINE_GDF_16BIT)) {
	lpDrvInfo->hDriver = lpDrv->hDriver16;
	lpDrvInfo->hModule = lpDrv->d.d16.hModule;
	lstrcpynA(lpDrvInfo->szAliasName, lpDrv->szAliasName, sizeof(lpDrvInfo->szAliasName));
	ret = TRUE;
    }
    
    return ret;
}

/**************************************************************************
 *				GetNextDriver			[USER.257]
 */
HDRVR16 WINAPI GetNextDriver16(HDRVR16 hDrvr, DWORD dwFlags)
{
    HDRVR16 		hRetDrv = 0;
    LPWINE_DRIVER 	lpDrv;
    
    TRACE("(%04x, %08lX);\n", hDrvr, dwFlags);
    
    if (hDrvr == 0) {
	if (lpDrvItemList == NULL) {
	    FIXME("drivers list empty !\n");
	    /* FIXME: code was using DRIVER_LoadStartupDrivers(); before ? 
	     * I (EPP) don't quite understand this 
	     */
	    if (lpDrvItemList == NULL) 
		return 0;
	}
	lpDrv = lpDrvItemList;
	if (dwFlags & GND_REVERSE) {
	    while (lpDrv->lpNextItem)
		lpDrv = lpDrv->lpNextItem;
	}
    } else {
	lpDrv = DRIVER_FindFromHDrvr16(hDrvr);
	if (lpDrv != NULL) {
	    if (dwFlags & GND_REVERSE) {
		lpDrv = (lpDrv->lpPrevItem) ? lpDrv->lpPrevItem : NULL;
	    } else {
		lpDrv = (lpDrv->lpNextItem) ? lpDrv->lpNextItem : NULL;
	    }
	}
    }
    
    hRetDrv = (lpDrv) ? lpDrv->hDriver16 : (HDRVR16)0;
    TRACE("return %04x !\n", hRetDrv);
    return hRetDrv;
}

