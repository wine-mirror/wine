/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 *		  1999	Eric Pouech
 */

#include "winbase.h"
#include "winerror.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "debugtools.h"
#include "driver.h"
#include "heap.h"
#include "mmsystem.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "wineacm.h"
#include "winreg.h"

DEFAULT_DEBUG_CHANNEL(msacm)
	
/***********************************************************************
 *           acmDriverAddA (MSACM32.2)
 */
MMRESULT WINAPI acmDriverAddA(PHACMDRIVERID phadid, HINSTANCE hinstModule,
			      LPARAM lParam, DWORD dwPriority, DWORD fdwAdd)
{
    if (!phadid)
	return MMSYSERR_INVALPARAM;
    
    /* Check if any unknown flags */
    if (fdwAdd & 
	~(ACM_DRIVERADDF_FUNCTION|ACM_DRIVERADDF_NOTIFYHWND|
	  ACM_DRIVERADDF_GLOBAL))
	return MMSYSERR_INVALFLAG;
    
    /* Check if any incompatible flags */
    if ((fdwAdd & ACM_DRIVERADDF_FUNCTION) && 
	(fdwAdd & ACM_DRIVERADDF_NOTIFYHWND))
	return MMSYSERR_INVALFLAG;
    
    /* FIXME: in fact, should GetModuleFileName(hinstModule) and do a 
     * LoadDriver on it, to be sure we can call SendDriverMessage on the
     * hDrvr handle.
     */
    *phadid = (HACMDRIVERID) MSACM_RegisterDriver(NULL, NULL, hinstModule);
    
    /* FIXME: lParam, dwPriority and fdwAdd ignored */
    
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverAddW (MSACM32.3)
 * FIXME
 *   Not implemented
 */
MMRESULT WINAPI acmDriverAddW(PHACMDRIVERID phadid, HINSTANCE hinstModule,
			      LPARAM lParam, DWORD dwPriority, DWORD fdwAdd)
{
    FIXME("(%p, 0x%08x, %ld, %ld, %ld): stub\n",
	  phadid, hinstModule, lParam, dwPriority, fdwAdd);
    
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmDriverClose (MSACM32.4)
 */
MMRESULT WINAPI acmDriverClose(HACMDRIVER had, DWORD fdwClose)
{
    PWINE_ACMDRIVER  p;
    PWINE_ACMDRIVER* tp;
    
    if (fdwClose)
	return MMSYSERR_INVALFLAG;
    
    p = MSACM_GetDriver(had);
    if (!p)
	return MMSYSERR_INVALHANDLE;

    for (tp = &(p->obj.pACMDriverID->pACMDriverList); *tp; *tp = (*tp)->pNextACMDriver) {
	if (*tp == p) {
	    *tp = (*tp)->pNextACMDriver;
	    break;
	}
    }
    
    if (p->hDrvr && !p->obj.pACMDriverID->pACMDriverList)
	CloseDriver(p->hDrvr, 0, 0);
    
    HeapFree(MSACM_hHeap, 0, p);
    
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverDetailsA (MSACM32.5)
 */
MMRESULT WINAPI acmDriverDetailsA(HACMDRIVERID hadid, PACMDRIVERDETAILSA padd, DWORD fdwDetails)
{
    MMRESULT mmr;
    ACMDRIVERDETAILSW	addw;
    
    addw.cbStruct = sizeof(addw);
    mmr = acmDriverDetailsW(hadid, &addw, fdwDetails);
    if (mmr == 0) {
	padd->fccType = addw.fccType; 
	padd->fccComp = addw.fccComp; 
	padd->wMid = addw.wMid; 
	padd->wPid = addw.wPid; 
	padd->vdwACM = addw.vdwACM; 
	padd->vdwDriver = addw.vdwDriver; 
	padd->fdwSupport = addw.fdwSupport; 
	padd->cFormatTags = addw.cFormatTags; 
	padd->cFilterTags = addw.cFilterTags; 
	padd->hicon = addw.hicon; 
	lstrcpyWtoA(padd->szShortName, addw.szShortName);
	lstrcpyWtoA(padd->szLongName, addw.szLongName);
	lstrcpyWtoA(padd->szCopyright, addw.szCopyright);
	lstrcpyWtoA(padd->szLicensing, addw.szLicensing);
	lstrcpyWtoA(padd->szFeatures, addw.szFeatures);
    }
    return mmr;
}

/***********************************************************************
 *           acmDriverDetailsW (MSACM32.6)
 */
MMRESULT WINAPI acmDriverDetailsW(HACMDRIVERID hadid, PACMDRIVERDETAILSW padd, DWORD fdwDetails)
{
    HACMDRIVER acmDrvr;
    MMRESULT mmr;
    
    if (fdwDetails)
	return MMSYSERR_INVALFLAG;
    
    mmr = acmDriverOpen(&acmDrvr, hadid, 0);
    if (mmr == 0) {
	mmr = (MMRESULT)acmDriverMessage(acmDrvr, ACMDM_DRIVER_DETAILS, (LPARAM) padd,  0);
    
	acmDriverClose(acmDrvr, 0);
    }
    
    return mmr;
}

/***********************************************************************
 *           acmDriverEnum (MSACM32.7)
 */
MMRESULT WINAPI acmDriverEnum(ACMDRIVERENUMCB fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
    PWINE_ACMDRIVERID	p;
    DWORD		fdwSupport;

    if (!fnCallback) {
	return MMSYSERR_INVALPARAM;
    }
    
    if (fdwEnum && ~(ACM_DRIVERENUMF_NOLOCAL|ACM_DRIVERENUMF_DISABLED)) {
	return MMSYSERR_INVALFLAG;
    }
    
    for (p = MSACM_pFirstACMDriverID; p; p = p->pNextACMDriverID) {
	fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
	if (!p->bEnabled) {
	    if (fdwEnum & ACM_DRIVERENUMF_DISABLED)
		fdwSupport |= ACMDRIVERDETAILS_SUPPORTF_DISABLED;
	    else
		continue;
	}
	(*fnCallback)((HACMDRIVERID) p, dwInstance, fdwSupport);
    }
    
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverID (MSACM32.8)
 */
MMRESULT WINAPI acmDriverID(HACMOBJ hao, PHACMDRIVERID phadid, DWORD fdwDriverID)
{
    PWINE_ACMOBJ pao;
    
    pao = MSACM_GetObj(hao);
    if (!pao)
	return MMSYSERR_INVALHANDLE;
    
    if (!phadid)
	return MMSYSERR_INVALPARAM;
    
    if (fdwDriverID)
	return MMSYSERR_INVALFLAG;
    
    *phadid = (HACMDRIVERID) pao->pACMDriverID;
    
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverMessage (MSACM32.9)
 * FIXME
 *   Not implemented
 */
LRESULT WINAPI acmDriverMessage(HACMDRIVER had, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    PWINE_ACMDRIVER pad = MSACM_GetDriver(had);
    if (!pad)
	return MMSYSERR_INVALPARAM;
    
    /* FIXME: Check if uMsg legal */
    
    if (!SendDriverMessage(pad->hDrvr, uMsg, lParam1, lParam2))
	return MMSYSERR_NOTSUPPORTED;
    
    return MMSYSERR_NOERROR;
}


/***********************************************************************
 *           acmDriverOpen (MSACM32.10)
 */
MMRESULT WINAPI acmDriverOpen(PHACMDRIVER phad, HACMDRIVERID hadid, DWORD fdwOpen)
{
    PWINE_ACMDRIVERID	padid;
    PWINE_ACMDRIVER	pad;

    TRACE("(%p, %x, %08lu)\n", phad, hadid, fdwOpen);

    if (!phad)
	return MMSYSERR_INVALPARAM;
    
    padid = MSACM_GetDriverID(hadid); 
    if (!padid)
	return MMSYSERR_INVALHANDLE;
    
    if (fdwOpen)
	return MMSYSERR_INVALFLAG;
    
    pad = HeapAlloc(MSACM_hHeap, 0, sizeof(WINE_ACMDRIVER));
    if (!pad) return MMSYSERR_NOMEM;

    pad->obj.pACMDriverID = padid;
    
    if (!padid->hInstModule)
	pad->hDrvr = OpenDriverA(padid->pszDriverAlias, "drivers32", 0);
    else
	pad->hDrvr = padid->hInstModule;
    
    if (!pad->hDrvr) {
	HeapFree(MSACM_hHeap, 0, pad);
	return MMSYSERR_ERROR;
    }
    
    pad->pfnDriverProc = GetProcAddress(pad->hDrvr, "DriverProc");

    /* insert new pad at beg of list */
    pad->pNextACMDriver = padid->pACMDriverList;
    padid->pACMDriverList = pad;

    /* FIXME: Create a WINE_ACMDRIVER32 */
    *phad = (HACMDRIVER)pad;
    
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverPriority (MSACM32.11)
 */
MMRESULT WINAPI acmDriverPriority(HACMDRIVERID hadid, DWORD dwPriority, DWORD fdwPriority)
{
    PWINE_ACMDRIVERID padid;
    CHAR szSubKey[17];
    CHAR szBuffer[256];
    LONG lBufferLength = sizeof(szBuffer);
    LONG lError;
    HKEY hPriorityKey;
    DWORD dwPriorityCounter;
    
    padid = MSACM_GetDriverID(hadid);
    if (!padid)
	return MMSYSERR_INVALHANDLE;
    
    /* Check for unknown flags */
    if (fdwPriority & 
	~(ACM_DRIVERPRIORITYF_ENABLE|ACM_DRIVERPRIORITYF_DISABLE|
	  ACM_DRIVERPRIORITYF_BEGIN|ACM_DRIVERPRIORITYF_END))
	return MMSYSERR_INVALFLAG;
    
    /* Check for incompatible flags */
    if ((fdwPriority & ACM_DRIVERPRIORITYF_ENABLE) &&
	(fdwPriority & ACM_DRIVERPRIORITYF_DISABLE))
	return MMSYSERR_INVALFLAG;
    
    /* Check for incompatible flags */
    if ((fdwPriority & ACM_DRIVERPRIORITYF_BEGIN) &&
	(fdwPriority & ACM_DRIVERPRIORITYF_END))
	return MMSYSERR_INVALFLAG;
    
    lError = RegOpenKeyA(HKEY_CURRENT_USER, 
			 "Software\\Microsoft\\Multimedia\\"
			 "Audio Compression Manager\\Priority v4.00",
			 &hPriorityKey
			 );
    /* FIXME: Create key */
    if (lError != ERROR_SUCCESS)
	return MMSYSERR_ERROR;
    
    for (dwPriorityCounter = 1; ; dwPriorityCounter++)	{
	wsnprintfA(szSubKey, 17, "Priorty%ld", dwPriorityCounter);
	lError = RegQueryValueA(hPriorityKey, szSubKey, szBuffer, &lBufferLength);
	if (lError != ERROR_SUCCESS)
	    break;
	
	FIXME("(0x%08x, %ld, %ld): stub (partial)\n", 
	      hadid, dwPriority, fdwPriority);
	break;
    }
    
    RegCloseKey(hPriorityKey);
    
    return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmDriverRemove (MSACM32.12)
 */
MMRESULT WINAPI acmDriverRemove(HACMDRIVERID hadid, DWORD fdwRemove)
{
    PWINE_ACMDRIVERID padid;
    
    padid = MSACM_GetDriverID(hadid);
    if (!padid)
	return MMSYSERR_INVALHANDLE;
    
    if (fdwRemove)
	return MMSYSERR_INVALFLAG;
    
    MSACM_UnregisterDriver(padid);
    
    return MMSYSERR_NOERROR;
}

