/*
 * Digital video MCI Wine Driver
 *
 * Copyright 1999 Eric POUECH
 */

#include "wine/winuser16.h"
#include "multimedia.h"
#include "user.h"
#include "driver.h"
#include "xmalloc.h"
#include "debug.h"
#include "callback.h"
#include "options.h"

typedef struct {
    int			nUseCount;          	/* Incremented for each shared open          */
    BOOL16  		fShareable;         	/* TRUE if first open was shareable 	     */
    WORD		wNotifyDeviceID;    	/* MCI device ID with a pending notification */
    HANDLE16 		hCallback;         	/* Callback handle for pending notification  */
    HMMIO		hFile;	            	/* mmio file handle open as Element          */
    MCI_OPEN_PARMSA 	openParms;
    DWORD		dwTimeFormat;
} WINE_MCIAVI;

#define MAX_MCIAVIDRV 	(1)
static WINE_MCIAVI	MCIAviDev[MAX_MCIAVIDRV];

/*======================================================================*
 *                  	    MCI AVI implemantation			*
 *======================================================================*/

/**************************************************************************
 * 				AVI_mciGetOpenDev		[internal]	
 */
static WINE_MCIAVI*  AVI_mciGetOpenDev(UINT16 wDevID)
{
    if (wDevID >= MAX_MCIAVIDRV || MCIAviDev[wDevID].nUseCount == 0) {
	WARN(mciavi, "Invalid wDevID=%u\n", wDevID);
	return 0;
    }
    return &MCIAviDev[wDevID];
}

static	DWORD	AVI_mciStop(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);

static	DWORD	AVI_mciOpen(UINT16 wDevID, DWORD dwFlags, LPMCI_OPEN_PARMSA lpParms)
{
    WINE_MCIAVI*	wma;

    TRACE(mciavi, "(%04x, %08lX, %p) : semi-stub\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) 		return MCIERR_NULL_PARAMETER_BLOCK;
    if (wDevID > MAX_MCIAVIDRV)		return MCIERR_INVALID_DEVICE_ID;

    wma = &MCIAviDev[wDevID];

    if (wma->nUseCount > 0) {
	/* The driver is already open on this channel */
	/* If the driver was opened shareable before and this open specifies */
	/* shareable then increment the use count */
	if (wma->fShareable && (dwFlags & MCI_OPEN_SHAREABLE))
	    ++wma->nUseCount;
	else
	    return MCIERR_MUST_USE_SHAREABLE;
    } else {
	wma->nUseCount = 1;
	wma->fShareable = dwFlags & MCI_OPEN_SHAREABLE;
    }
    if (dwFlags & MCI_OPEN_ELEMENT) {
	TRACE(cdaudio,"MCI_OPEN_ELEMENT !\n");
	/*		return MCIERR_NO_ELEMENT_ALLOWED; */
    }

    wma->openParms.dwCallback = lpParms->dwCallback;
    wma->openParms.wDeviceID  = (WORD)lpParms->wDeviceID;
    wma->openParms.lpstrDeviceType = lpParms->lpstrDeviceType;
    wma->openParms.lpstrElementName = lpParms->lpstrElementName;
    wma->openParms.lpstrAlias = lpParms->lpstrAlias;

    wma->wNotifyDeviceID = lpParms->wDeviceID;
    /* FIXME: do real open */
    /*    wmcda->mciMode = MCI_MODE_STOP; */
    wma->dwTimeFormat = MCI_FORMAT_TMSF;

    return 0;

}

static	DWORD	AVI_mciClose(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);

    TRACE(mciavi, "(%04x, %08lX, %p) : semi-stub\n", wDevID, dwFlags, lpParms);    

    if (wma == NULL) 	return MCIERR_INVALID_DEVICE_ID;
    
    if (wma->nUseCount == 1) {
	AVI_mciStop(wDevID, 0, NULL);
	/* FIXME: do real closing */
    }
    wma->nUseCount--;
    return 0;
}

static	DWORD	AVI_mciPlay(UINT16 wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
    TRACE(mciavi, "(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    return 0;
}

static	DWORD	AVI_mciRecord(UINT16 wDevID, DWORD dwFlags, LPMCI_RECORD_PARMS lpParms)
{
    TRACE(mciavi, "(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    return 0;
}

static	DWORD	AVI_mciStop(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    TRACE(mciavi, "(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    return 0;
}

static	DWORD	AVI_mciSet(UINT16 wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
{
    TRACE(mciavi, "(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    return 0;
}

static	DWORD	AVI_mciPause(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    TRACE(mciavi, "(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    return 0;
}

static	DWORD	AVI_mciResume(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    TRACE(mciavi, "(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    return 0;
}

static	DWORD	AVI_mciStatus(UINT16 wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
    TRACE(mciavi, "(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    return 0;
}

static	DWORD	AVI_mciGetDevCaps(UINT16 wDevID, DWORD dwFlags,  LPMCI_GETDEVCAPS_PARMS lpParms)
{
    WINE_MCIAVI*	wmm = AVI_mciGetOpenDev(wDevID);
    
    TRACE(mciavi, "(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL) 	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmm == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (dwFlags & MCI_GETDEVCAPS_ITEM) {
	switch (lpParms->dwItem) {
	case MCI_GETDEVCAPS_DEVICE_TYPE:
	    TRACE(mciavi, "MCI_GETDEVCAPS_DEVICE_TYPE !\n");
	    lpParms->dwReturn = MCI_DEVTYPE_DIGITAL_VIDEO;
	    break;
	case MCI_GETDEVCAPS_HAS_AUDIO:
	    TRACE(mciavi, "MCI_GETDEVCAPS_HAS_AUDIO !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_HAS_VIDEO:
	    TRACE(mciavi, "MCI_GETDEVCAPS_HAS_VIDEO !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_USES_FILES:
	    TRACE(mciavi, "MCI_GETDEVCAPS_USES_FILES !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_COMPOUND_DEVICE:
	    TRACE(mciavi, "MCI_GETDEVCAPS_COMPOUND_DEVICE !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_CAN_EJECT:
	    TRACE(mciavi, "MCI_GETDEVCAPS_CAN_EJECT !\n");
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_CAN_PLAY:
	    TRACE(mciavi, "MCI_GETDEVCAPS_CAN_PLAY !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_CAN_RECORD:
	    TRACE(mciavi, "MCI_GETDEVCAPS_CAN_RECORD !\n");
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_CAN_SAVE:
	    TRACE(mciavi, "MCI_GETDEVCAPS_CAN_SAVE !\n");
	    lpParms->dwReturn = FALSE;
	    break;
	default:
	    TRACE(mciavi, "Unknown capability (%08lx) !\n", lpParms->dwItem);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    } else {
	TRACE(mciavi, "No GetDevCaps-Item !\n");
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    return 0;
}

static	DWORD	AVI_mciInfo(UINT16 wDevID, DWORD dwFlags, LPMCI_INFO_PARMSA lpParms)
{
    DWORD		ret = 0;
    LPSTR		str = 0;
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);

    TRACE(mciavi, "(%04X, %08lX, %p) : stub;\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL || lpParms->lpstrReturn == NULL) {
	ret = MCIERR_NULL_PARAMETER_BLOCK;
    } else if (wma == NULL) {
	ret = MCIERR_INVALID_DEVICE_ID;
    } else {
	TRACE(mciavi, "buf=%p, len=%lu\n", lpParms->lpstrReturn, lpParms->dwRetSize);
	
	switch (dwFlags) {
	case MCI_INFO_PRODUCT:
	    str = "Wine's AVI player";
	    break;
	case MCI_INFO_FILE:
	    str = "";
	    break;
#if 0
	    /* FIXME: the following manifest constants are not defined in <WINE>/include/mmsystem.h */
	case MCI_INFO_COPYRIGHT:
	    break;
	case MCI_INFO_NAME:
	    break;
#endif
	default:
	    WARN(mciavi, "Don't know this info command (%lu)\n", dwFlags);
	    ret = MCIERR_UNRECOGNIZED_COMMAND;
	}
    }
    if (str) {
	ret = MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize, str);
    } else {
	lpParms->lpstrReturn[0] = 0;
    }
    
    return ret;
}

static	DWORD	AVI_mciSeek(UINT16 wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
    TRACE(mciavi, "(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    return 0;
}


/*======================================================================*
 *                  	    MCI AVI entry points			*
 *======================================================================*/

/**************************************************************************
 * 				MCIAVI_DriverProc32	[sample driver]
 */
LONG MCIAVI_DriverProc32(DWORD dwDevID, HDRVR16 hDriv, DWORD wMsg, 
				 DWORD dwParam1, DWORD dwParam2)
{
    switch (wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return 1;
    case DRV_CLOSE:		return 1;
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "Sample AVI Wine Driver !", "MM-Wine Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    case MCI_OPEN_DRIVER:	return AVI_mciOpen      (dwDevID, dwParam1, (LPMCI_OPEN_PARMSA)   dwParam2);
    case MCI_CLOSE_DRIVER:	return AVI_mciClose     (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)   dwParam2);
    case MCI_PLAY:		return AVI_mciPlay      (dwDevID, dwParam1, (LPMCI_PLAY_PARMS)      dwParam2);
    case MCI_RECORD:		return AVI_mciRecord    (dwDevID, dwParam1, (LPMCI_RECORD_PARMS)    dwParam2);
    case MCI_STOP:		return AVI_mciStop      (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)   dwParam2);
    case MCI_SET:		return AVI_mciSet       (dwDevID, dwParam1, (LPMCI_SET_PARMS)       dwParam2);
    case MCI_PAUSE:		return AVI_mciPause     (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)   dwParam2);
    case MCI_RESUME:		return AVI_mciResume    (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)   dwParam2);
    case MCI_STATUS:		return AVI_mciStatus    (dwDevID, dwParam1, (LPMCI_STATUS_PARMS)    dwParam2);
    case MCI_GETDEVCAPS:	return AVI_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)dwParam2);
    case MCI_INFO:		return AVI_mciInfo      (dwDevID, dwParam1, (LPMCI_INFO_PARMSA)   dwParam2);
    case MCI_SEEK:		return AVI_mciSeek      (dwDevID, dwParam1, (LPMCI_SEEK_PARMS)      dwParam2);
    case MCI_LOAD:		
    case MCI_SAVE:		
    case MCI_FREEZE:		
    case MCI_PUT:		
    case MCI_REALIZE:		
    case MCI_UNFREEZE:		
    case MCI_UPDATE:		
    case MCI_WHERE:		
    case MCI_WINDOW:		
    case MCI_STEP:		
    case MCI_SPIN:		
    case MCI_ESCAPE:		
    case MCI_COPY:		
    case MCI_CUT:		
    case MCI_DELETE:		
    case MCI_PASTE:		
	WARN(mciavi, "Unsupported command=%s\n", MCI_CommandToString(wMsg));
	break;
    case MCI_OPEN:
    case MCI_CLOSE:
	FIXME(mciavi, "Shouldn't receive a MCI_OPEN or CLOSE message\n");
	break;
    default:			
	TRACE(mciavi, "Sending msg=%s to default driver proc\n", MCI_CommandToString(wMsg));
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return MCIERR_UNRECOGNIZED_COMMAND;
}
