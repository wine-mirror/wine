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
#include "debugtools.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "wineacm.h"

DEFAULT_DEBUG_CHANNEL(msacm);
	
/**********************************************************************/
	
HINSTANCE	MSACM_hInstance32 = 0;

/***********************************************************************
 *           MSACM_LibMain (MSACM32.init) 
 */
BOOL WINAPI MSACM32_LibMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("0x%x 0x%lx %p\n", hInstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        MSACM_hHeap = HeapCreate(0, 0x10000, 0);
        MSACM_hInstance32 = hInstDLL;
        MSACM_RegisterAllDrivers();
	break;
    case DLL_PROCESS_DETACH:
        MSACM_UnregisterAllDrivers();
        HeapDestroy(MSACM_hHeap);
        MSACM_hHeap = (HANDLE) NULL;
        MSACM_hInstance32 = (HINSTANCE)NULL;
	break;
    case DLL_THREAD_ATTACH:
	break;
    case DLL_THREAD_DETACH:
	break;
    default:
	break;
    }
    return TRUE;
}

/***********************************************************************
 *           XRegThunkEntry (MSACM32.1)
 * FIXME
 *   No documentation found.
 */

/***********************************************************************
 *           acmGetVersion (MSACM32.34)
 */
DWORD WINAPI acmGetVersion(void)
{
    OSVERSIONINFOA version;
    GetVersionExA( &version );
    switch(version.dwPlatformId)
    {
    case VER_PLATFORM_WIN32_NT:
	return 0x04000565; /* 4.0.1381 */
    default:
        FIXME("%ld not supported",version.dwPlatformId);
    case VER_PLATFORM_WIN32_WINDOWS:
	return 0x04000000; /* 4.0.0 */
  }
}

/***********************************************************************
 *           acmMessage32 (MSACM32.35)
 * FIXME
 *   No documentation found.
 */

/***********************************************************************
 *           acmMetrics (MSACM32.36)
 */
MMRESULT WINAPI acmMetrics(HACMOBJ hao, UINT uMetric, LPVOID pMetric)
{
    PWINE_ACMOBJ 	pao = MSACM_GetObj(hao, WINE_ACMOBJ_DONTCARE);
    BOOL 		bLocal = TRUE;
    PWINE_ACMDRIVERID	padid;
    DWORD		val = 0;
    MMRESULT		mmr = MMSYSERR_NOERROR;

    TRACE("(0x%08x, %d, %p);\n", hao, uMetric, pMetric);
    
    switch (uMetric) {
    case ACM_METRIC_COUNT_DRIVERS:
	bLocal = FALSE;
	/* fall thru */
    case ACM_METRIC_COUNT_LOCAL_DRIVERS:
	if (!pao)
	    return MMSYSERR_INVALHANDLE;
	for (padid = MSACM_pFirstACMDriverID; padid; padid = padid->pNextACMDriverID)
	    if (padid->bEnabled /* && (local(padid) || !bLocal) */)
		val++;
	*(LPDWORD)pMetric = val;
	break;

    case ACM_METRIC_COUNT_CODECS:
	if (!pao)
	    return MMSYSERR_INVALHANDLE;
	bLocal = FALSE;
	/* fall thru */
    case ACM_METRIC_COUNT_LOCAL_CODECS:
	/* FIXME: don't know how to differentiate codec, converters & filters yet */
	for (padid = MSACM_pFirstACMDriverID; padid; padid = padid->pNextACMDriverID)
	    if (padid->bEnabled /* && (local(padid) || !bLocal) */)
		val++;
	*(LPDWORD)pMetric = val;
	break;

    case ACM_METRIC_COUNT_CONVERTERS:
	bLocal = FALSE;
	/* fall thru */
    case ACM_METRIC_COUNT_LOCAL_CONVERTERS:
	/* FIXME: don't know how to differentiate codec, converters & filters yet */
	for (padid = MSACM_pFirstACMDriverID; padid; padid = padid->pNextACMDriverID)
	    if (padid->bEnabled /* && (local(padid) || !bLocal) */)
		val++;
	*(LPDWORD)pMetric = val;
	break;

    case ACM_METRIC_COUNT_FILTERS:
	bLocal = FALSE;
	/* fall thru */
    case ACM_METRIC_COUNT_LOCAL_FILTERS:
	/* FIXME: don't know how to differentiate codec, converters & filters yet */
	for (padid = MSACM_pFirstACMDriverID; padid; padid = padid->pNextACMDriverID)
	    if (padid->bEnabled /* && (local(padid) || !bLocal) */)
		val++;
	*(LPDWORD)pMetric = val;
	break;

    case ACM_METRIC_COUNT_DISABLED:
	bLocal = FALSE;
	/* fall thru */
    case ACM_METRIC_COUNT_LOCAL_DISABLED:
	if (!pao)
	    return MMSYSERR_INVALHANDLE;  
	for (padid = MSACM_pFirstACMDriverID; padid; padid = padid->pNextACMDriverID)
	    if (!padid->bEnabled /* && (local(padid) || !bLocal) */)
		val++;
	*(LPDWORD)pMetric = val;
	break;
    
    case ACM_METRIC_MAX_SIZE_FORMAT:
	{
	    ACMFORMATTAGDETAILSW	aftd;

	    aftd.cbStruct = sizeof(aftd);
	    aftd.dwFormatTag = WAVE_FORMAT_UNKNOWN;

	    if (hao == (HACMOBJ)NULL) {
		mmr = acmFormatTagDetailsW((HACMDRIVER)NULL, &aftd, ACM_FORMATTAGDETAILSF_LARGESTSIZE);
	    } else if (MSACM_GetObj(hao, WINE_ACMOBJ_DRIVER)) {
		mmr = acmFormatTagDetailsW((HACMDRIVER)hao, &aftd, ACM_FORMATTAGDETAILSF_LARGESTSIZE);
	    } else if (MSACM_GetObj(hao, WINE_ACMOBJ_DRIVERID)) {
		HACMDRIVER	had;
		
		if (acmDriverOpen(&had, (HACMDRIVERID)hao, 0) == 0) {
		    mmr = acmFormatTagDetailsW((HACMDRIVER)hao, &aftd, ACM_FORMATTAGDETAILSF_LARGESTSIZE);
		    acmDriverClose(had, 0);
		}
	    } else {
		mmr = MMSYSERR_INVALHANDLE;
	    }
	    if (mmr == MMSYSERR_NOERROR) *(LPDWORD)pMetric = aftd.cbFormatSize;
	}
        break;

    case ACM_METRIC_COUNT_HARDWARE:
    case ACM_METRIC_HARDWARE_WAVE_INPUT:
    case ACM_METRIC_HARDWARE_WAVE_OUTPUT:
    case ACM_METRIC_MAX_SIZE_FILTER:
    case ACM_METRIC_DRIVER_SUPPORT:
    case ACM_METRIC_DRIVER_PRIORITY:
    default:
	FIXME("(0x%08x, %d, %p): stub\n", hao, uMetric, pMetric);
	mmr = MMSYSERR_NOTSUPPORTED;
    }
    return mmr;
}
