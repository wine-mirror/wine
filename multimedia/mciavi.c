/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * Digital video MCI Wine Driver
 *
 * Copyright 1999 Eric POUECH
 */

#include <string.h>
#include "winuser.h"
#include "mmddk.h"
#include "user.h"
#include "driver.h"
#include "digitalv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mciavi)

typedef struct {
    UINT		wDevID;
    int			nUseCount;          	/* Incremented for each shared open          */
    BOOL16  		fShareable;         	/* TRUE if first open was shareable 	     */
    WORD		wNotifyDeviceID;    	/* MCI device ID with a pending notification */
    HANDLE16 		hCallback;         	/* Callback handle for pending notification  */
    HMMIO		hFile;	            	/* mmio file handle open as Element          */
    WORD		wStatus;		/* One of MCI_MODE_XXX			     */
    MCI_OPEN_PARMSA 	openParms;
    DWORD		dwTimeFormat;
} WINE_MCIAVI;

/*======================================================================*
 *                  	    MCI AVI implemantation			*
 *======================================================================*/

/**************************************************************************
 * 				AVI_drvOpen			[internal]	
 */
static	DWORD	AVI_drvOpen(LPSTR str, LPMCI_OPEN_DRIVER_PARMSA modp)
{
    WINE_MCIAVI*	wma = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINE_MCIAVI));

    if (!wma)
	return 0;

    wma->wDevID = modp->wDeviceID;
    mciSetDriverData(wma->wDevID, (DWORD)wma);
    modp->wCustomCommandTable = MCI_NO_COMMAND_TABLE;
    modp->wType = MCI_DEVTYPE_DIGITAL_VIDEO;
    return modp->wDeviceID;
}

/**************************************************************************
 * 				MCIAVI_drvClose		[internal]	
 */
static	DWORD	AVI_drvClose(DWORD dwDevID)
{
    WINE_MCIAVI*  wma = (WINE_MCIAVI*)mciGetDriverData(dwDevID);

    if (wma) {
	HeapFree(GetProcessHeap(), 0, wma);	
	mciSetDriverData(dwDevID, 0);
	return 1;
    }
    return 0;
}

/**************************************************************************
 * 				AVI_mciGetOpenDev		[internal]	
 */
static WINE_MCIAVI*  AVI_mciGetOpenDev(UINT16 wDevID)
{
    WINE_MCIAVI*	wma = (WINE_MCIAVI*)mciGetDriverData(wDevID);
    
    if (wma == NULL || wma->nUseCount == 0) {
	WARN("Invalid wDevID=%u\n", wDevID);
	return 0;
    }
    return wma;
}

static	DWORD	AVI_mciStop(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);

/***************************************************************************
 * 				AVI_mciOpen			[internal]
 */
static	DWORD	AVI_mciOpen(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_OPEN_PARMSA lpParms)
{
    WINE_MCIAVI*	wma = (WINE_MCIAVI*)mciGetDriverData(wDevID);
    
    TRACE("(%04x, %08lX, %p) : semi-stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL) 		return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)			return MCIERR_INVALID_DEVICE_ID;
    
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
	TRACE("MCI_OPEN_ELEMENT !\n");
	/*		return MCIERR_NO_ELEMENT_ALLOWED; */
    }
    
    wma->openParms.dwCallback = lpParms->dwCallback;
    wma->openParms.wDeviceID  = (WORD)lpParms->wDeviceID;
    wma->openParms.lpstrDeviceType = lpParms->lpstrDeviceType;
    wma->openParms.lpstrElementName = lpParms->lpstrElementName;
    wma->openParms.lpstrAlias = lpParms->lpstrAlias;
    
    wma->wNotifyDeviceID = lpParms->wDeviceID;
    /* FIXME: do real open */
    wma->wStatus = MCI_MODE_STOP;
    wma->dwTimeFormat = MCI_FORMAT_TMSF;
    
    return 0;
    
}

/***************************************************************************
 * 				AVI_mciClose			[internal]
 */
static	DWORD	AVI_mciClose(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lX, %p) : semi-stub\n", wDevID, dwFlags, lpParms);    
    
    if (wma == NULL) 	return MCIERR_INVALID_DEVICE_ID;
    
    if (wma->nUseCount == 1) {
	AVI_mciStop(wDevID, 0, NULL);
	/* FIXME: do real closing */
    }
    wma->nUseCount--;
    return 0;
}

/***************************************************************************
 * 				AVI_mciPlay			[internal]
 */
static	DWORD	AVI_mciPlay(UINT16 wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    wma->wStatus = MCI_MODE_PLAY;
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE("MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wma->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/***************************************************************************
 * 				AVI_mciRecord			[internal]
 */
static	DWORD	AVI_mciRecord(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_RECORD_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    wma->wStatus = MCI_MODE_RECORD;
    return 0;
}

/***************************************************************************
 * 				AVI_mciStop			[internal]
 */
static	DWORD	AVI_mciStop(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    wma->wStatus = MCI_MODE_STOP;
    return 0;
}

/***************************************************************************
 * 				AVI_mciPause			[internal]
 */
static	DWORD	AVI_mciPause(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    wma->wStatus = MCI_MODE_PAUSE;
    return 0;
}

/***************************************************************************
 * 				AVI_mciResume			[internal]
 */
static	DWORD	AVI_mciResume(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    wma->wStatus = MCI_MODE_PLAY;
    return 0;
}

/***************************************************************************
 * 				AVI_mciSeek			[internal]
 */
static	DWORD	AVI_mciSeek(UINT16 wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/***************************************************************************
 * 				AVI_mciSet			[internal]
 */
static	DWORD	AVI_mciSet(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_SET_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (dwFlags & MCI_SET_TIME_FORMAT) {
	switch (lpParms->dwTimeFormat) {
	case MCI_FORMAT_MILLISECONDS:
	    TRACE("MCI_FORMAT_MILLISECONDS !\n");
	    wma->dwTimeFormat = MCI_FORMAT_MILLISECONDS;
	    break;
	case MCI_FORMAT_FRAMES:
	    TRACE("MCI_FORMAT_FRAMES !\n");
	    wma->dwTimeFormat = MCI_FORMAT_FRAMES;
	    break;
	default:
	    WARN("Bad time format %lu!\n", lpParms->dwTimeFormat);
	    return MCIERR_BAD_TIME_FORMAT;
	}
    }
    
    if (dwFlags & MCI_SET_DOOR_OPEN) {
	TRACE("No support for door open !\n");
	return MCIERR_UNSUPPORTED_FUNCTION;
    }
    if (dwFlags & MCI_SET_DOOR_CLOSED) {
	TRACE("No support for door close !\n");
	return MCIERR_UNSUPPORTED_FUNCTION;
    }
    if (dwFlags & MCI_SET_ON) {
	char	buffer[256];
	
	strcpy(buffer, "MCI_SET_ON:");
	
	if (dwFlags & MCI_SET_VIDEO) {
	    strncat(buffer, " video", sizeof(buffer));
	}
	if (dwFlags & MCI_SET_AUDIO) {
	    strncat(buffer, " audio", sizeof(buffer));
	    if (lpParms->dwAudio & MCI_SET_AUDIO_ALL)
		strncat(buffer, " all", sizeof(buffer));
	    if (lpParms->dwAudio & MCI_SET_AUDIO_LEFT)
		strncat(buffer, " left", sizeof(buffer));
	    if (lpParms->dwAudio & MCI_SET_AUDIO_RIGHT)
		strncat(buffer, " right", sizeof(buffer));
	}
	if (dwFlags & MCI_DGV_SET_SEEK_EXACTLY) {
	    strncat(buffer, " seek_exactly", sizeof(buffer));
	}
	TRACE("%s\n", buffer);
    }
    
    if (dwFlags & MCI_SET_OFF) {
	char	buffer[256];
	
	strcpy(buffer, "MCI_SET_OFF:");
	if (dwFlags & MCI_SET_VIDEO) {
	    strncat(buffer, " video", sizeof(buffer));
	}
	if (dwFlags & MCI_SET_AUDIO) {
	    strncat(buffer, " audio", sizeof(buffer));
	    if (lpParms->dwAudio & MCI_SET_AUDIO_ALL)
		strncat(buffer, " all", sizeof(buffer));
	    if (lpParms->dwAudio & MCI_SET_AUDIO_LEFT)
		strncat(buffer, " left", sizeof(buffer));
	    if (lpParms->dwAudio & MCI_SET_AUDIO_RIGHT)
		strncat(buffer, " right", sizeof(buffer));
	}
	if (dwFlags & MCI_DGV_SET_SEEK_EXACTLY) {
	    strncat(buffer, " seek_exactly", sizeof(buffer));
	}
	TRACE("%s\n", buffer);
    }
    if (dwFlags & MCI_DGV_SET_FILEFORMAT) {
	LPSTR	str = "save";
	if (dwFlags & MCI_DGV_SET_STILL)	
	    str = "capture";
	
	switch (lpParms->dwFileFormat) {
	case MCI_DGV_FF_AVI: 	TRACE("Setting file format (%s) to 'AVI'\n", str); 	break;
	case MCI_DGV_FF_AVSS: 	TRACE("Setting file format (%s) to 'AVSS'\n", str);	break;
	case MCI_DGV_FF_DIB: 	TRACE("Setting file format (%s) to 'DIB'\n", str);	break;
	case MCI_DGV_FF_JFIF: 	TRACE("Setting file format (%s) to 'JFIF'\n", str);	break;
	case MCI_DGV_FF_JPEG: 	TRACE("Setting file format (%s) to 'JPEG'\n", str);	break;
	case MCI_DGV_FF_MPEG: 	TRACE("Setting file format (%s) to 'MPEG'\n", str); 	break;
	case MCI_DGV_FF_RDIB:	TRACE("Setting file format (%s) to 'RLE DIB'\n", str);	break;
	case MCI_DGV_FF_RJPEG: 	TRACE("Setting file format (%s) to 'RJPEG'\n", str);	break;
	default:		TRACE("Setting unknown file format (%s): %ld\n", str, lpParms->dwFileFormat);
	}
    }
    
    if (dwFlags & MCI_DGV_SET_SPEED) {
	TRACE("Setting speed to %ld\n", lpParms->dwSpeed);
    }
    
    return 0;
}

/***************************************************************************
 * 				AVI_mciStatus			[internal]
 */
static	DWORD	AVI_mciStatus(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_STATUS_PARMSA lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    DWORD		ret = 0;

    TRACE("(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (dwFlags & MCI_STATUS_ITEM) {
	switch (lpParms->dwItem) {
	case MCI_STATUS_CURRENT_TRACK:
	    lpParms->dwReturn = 1;
	    TRACE("MCI_STATUS_CURRENT_TRACK => %lu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_LENGTH:
	    if (dwFlags & MCI_TRACK) {
		/* lpParms->dwTrack contains track # */
		lpParms->dwReturn = 0x1234;
	    } else {
		lpParms->dwReturn = 0x4321;
	    }
	    TRACE("MCI_STATUS_LENGTH => %lu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_MODE:
 	    lpParms->dwReturn = MAKEMCIRESOURCE(wma->wStatus, wma->wStatus);
	    ret = MCI_RESOURCE_RETURNED;
	    TRACE("MCI_STATUS_MODE => %u\n", LOWORD(lpParms->dwReturn));
	    break;
	case MCI_STATUS_MEDIA_PRESENT:
	    TRACE("MCI_STATUS_MEDIA_PRESENT => TRUE\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_NUMBER_OF_TRACKS:
	    lpParms->dwReturn = 3;
	    TRACE("MCI_STATUS_NUMBER_OF_TRACKS => %lu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_POSITION:
	    /* FIXME: do I need to use MCI_TRACK ? */
	    lpParms->dwReturn = 0x0123;
	    TRACE("MCI_STATUS_POSITION %s => %lu\n", 
		  (dwFlags & MCI_STATUS_START) ? "start" : "current", lpParms->dwReturn);
	    break;
	case MCI_STATUS_READY:
	    lpParms->dwReturn = (wma->wStatus == MCI_MODE_NOT_READY) ?
		MAKEMCIRESOURCE(FALSE, MCI_FALSE) : MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    TRACE("MCI_STATUS_READY = %u\n", LOWORD(lpParms->dwReturn));
	    break;
	case MCI_STATUS_TIME_FORMAT:
	    lpParms->dwReturn = MAKEMCIRESOURCE(wma->dwTimeFormat, wma->dwTimeFormat);
	    TRACE("MCI_STATUS_TIME_FORMAT => %u\n", LOWORD(lpParms->dwReturn));
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	default:
	    FIXME("Unknowm command %08lX !\n", lpParms->dwItem);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    } else {
	WARN("No Status-Item!\n");
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_NOTIFY) {
	TRACE("MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wma->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    
    return ret;
}

/***************************************************************************
 * 				AVI_mciGetDevCaps			[internal]
 */
static	DWORD	AVI_mciGetDevCaps(UINT16 wDevID, DWORD dwFlags,  LPMCI_GETDEVCAPS_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    DWORD		ret;

    TRACE("(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL) 	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (dwFlags & MCI_GETDEVCAPS_ITEM) {
	switch (lpParms->dwItem) {
	case MCI_GETDEVCAPS_DEVICE_TYPE:
	    TRACE("MCI_GETDEVCAPS_DEVICE_TYPE !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(MCI_DEVTYPE_DIGITAL_VIDEO, MCI_DEVTYPE_DIGITAL_VIDEO);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_HAS_AUDIO:
	    TRACE("MCI_GETDEVCAPS_HAS_AUDIO !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_HAS_VIDEO:
	    TRACE("MCI_GETDEVCAPS_HAS_VIDEO !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_USES_FILES:
	    TRACE("MCI_GETDEVCAPS_USES_FILES !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_COMPOUND_DEVICE:
	    TRACE("MCI_GETDEVCAPS_COMPOUND_DEVICE !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_EJECT:
	    TRACE("MCI_GETDEVCAPS_CAN_EJECT !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_PLAY:
	    TRACE("MCI_GETDEVCAPS_CAN_PLAY !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_RECORD:
	    TRACE("MCI_GETDEVCAPS_CAN_RECORD !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_SAVE:
	    TRACE("MCI_GETDEVCAPS_CAN_SAVE !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	default:
	    FIXME("Unknown capability (%08lx) !\n", lpParms->dwItem);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    } else {
	WARN("No GetDevCaps-Item !\n");
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    return ret;
}

/***************************************************************************
 * 				AVI_mciInfo			[internal]
 */
static	DWORD	AVI_mciInfo(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_INFO_PARMSA lpParms)
{
    LPSTR		str = 0;
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04X, %08lX, %p) : stub;\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL || lpParms->lpstrReturn == NULL)
	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL) return MCIERR_INVALID_DEVICE_ID;

    TRACE("buf=%p, len=%lu\n", lpParms->lpstrReturn, lpParms->dwRetSize);
    
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
	WARN("Don't know this info command (%lu)\n", dwFlags);
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    return MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize, str);
}

/***************************************************************************
 * 				AVI_mciPut			[internal]
 */
static	DWORD	AVI_mciPut(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_PUT_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    RECT		rc;
    char		buffer[256];
    
    TRACE("(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (dwFlags & MCI_DGV_RECT) {
	rc = lpParms->rc;
    } else {
	SetRectEmpty(&rc);
    }
    
    *buffer = 0;
    if (dwFlags & MCI_DGV_PUT_CLIENT) {
	strncat(buffer, "PUT_CLIENT", sizeof(buffer));
    }
    if (dwFlags & MCI_DGV_PUT_DESTINATION) {
	strncat(buffer, "PUT_DESTINATION", sizeof(buffer));
    }
    if (dwFlags & MCI_DGV_PUT_FRAME) {
	strncat(buffer, "PUT_FRAME", sizeof(buffer));
    }
    if (dwFlags & MCI_DGV_PUT_SOURCE) {
	strncat(buffer, "PUT_SOURCE", sizeof(buffer));
    }
    if (dwFlags & MCI_DGV_PUT_VIDEO) {
	strncat(buffer, "PUT_VIDEO", sizeof(buffer));
    }
    if (dwFlags & MCI_DGV_PUT_WINDOW) {
	strncat(buffer, "PUT_WINDOW", sizeof(buffer));
    }
    TRACE("%s (%d,%d,%d,%d)\n", buffer, rc.left, rc.top, rc.right, rc.bottom);
    
    return 0;
}

/***************************************************************************
 * 				AVI_mciWindow			[internal]
 */
static	DWORD	AVI_mciWindow(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_WINDOW_PARMSA lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (dwFlags & MCI_DGV_WINDOW_HWND) {
	TRACE("Setting hWnd to %08lx\n", (DWORD)lpParms->hWnd);
    }
    if (dwFlags & MCI_DGV_WINDOW_STATE) {
	TRACE("Setting nCmdShow to %d\n", lpParms->nCmdShow);
    }
    if (dwFlags & MCI_DGV_WINDOW_TEXT) {
	TRACE("Setting caption to '%s'\n", lpParms->lpstrText);
    }
    
    return 0;
}

/*****************************************************************************
 * 				AVI_mciLoad			[internal]
 */
static DWORD	AVI_mciLoad(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_LOAD_PARMSA lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciSave			[internal]
 */
static	DWORD	AVI_mciSave(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_SAVE_PARMSA lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciFreeze			[internal]
 */
static	DWORD	AVI_mciFreeze(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_RECT_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciRealize			[internal]
 */
static	DWORD	AVI_mciRealize(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciUnFreeze			[internal]
 */
static	DWORD	AVI_mciUnFreeze(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_RECT_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciUpdate			[internal]
 */
static	DWORD	AVI_mciUpdate(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_UPDATE_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciWhere			[internal]
 */
static	DWORD	AVI_mciWhere(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_RECT_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciStep			[internal]
 */
static	DWORD	AVI_mciStep(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_STEP_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciCopy			[internal]
 */
static	DWORD	AVI_mciCopy(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_COPY_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciCut			[internal]
 */
static	DWORD	AVI_mciCut(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_CUT_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciDelete			[internal]
 */
static	DWORD	AVI_mciDelete(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_DELETE_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciPaste			[internal]
 */
static	DWORD	AVI_mciPaste(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_PASTE_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciCue			[internal]
 */
static	DWORD	AVI_mciCue(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_CUE_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciCapture			[internal]
 */
static	DWORD	AVI_mciCapture(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_CAPTURE_PARMSA lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciMonitor			[internal]
 */
static	DWORD	AVI_mciMonitor(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_MONITOR_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciReserve			[internal]
 */
static	DWORD	AVI_mciReserve(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_RESERVE_PARMSA lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciSetAudio			[internal]
 */
static	DWORD	AVI_mciSetAudio(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_SETAUDIO_PARMSA lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciSignal			[internal]
 */
static	DWORD	AVI_mciSignal(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_SIGNAL_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciSetVideo			[internal]
 */
static	DWORD	AVI_mciSetVideo(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_SETVIDEO_PARMSA lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciQuality			[internal]
 */
static	DWORD	AVI_mciQuality(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_QUALITY_PARMSA lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciList			[internal]
 */
static	DWORD	AVI_mciList(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_LIST_PARMSA lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciUndo			[internal]
 */
static	DWORD	AVI_mciUndo(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciConfigure			[internal]
 */
static	DWORD	AVI_mciConfigure(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/******************************************************************************
 * 				AVI_mciRestore			[internal]
 */
static	DWORD	AVI_mciRestore(UINT16 wDevID, DWORD dwFlags, LPMCI_DGV_RESTORE_PARMSA lpParms)
{
    WINE_MCIAVI*	wma = AVI_mciGetOpenDev(wDevID);
    
    TRACE("(%04x, %08lx, %p) : stub\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    return 0;
}

/*======================================================================*
 *                  	    MCI AVI entry points			*
 *======================================================================*/

/**************************************************************************
 * 				MCIAVI_DriverProc	[sample driver]
 */
LONG CALLBACK	MCIAVI_DriverProc(DWORD dwDevID, HDRVR hDriv, DWORD wMsg, 
				  DWORD dwParam1, DWORD dwParam2)
{
    switch (wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return AVI_drvOpen((LPSTR)dwParam1, (LPMCI_OPEN_DRIVER_PARMSA)dwParam2);
    case DRV_CLOSE:		return AVI_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "Sample AVI Wine Driver !", "MM-Wine Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
	
    case MCI_OPEN_DRIVER:	return AVI_mciOpen      (dwDevID, dwParam1, (LPMCI_DGV_OPEN_PARMSA)     dwParam2);
    case MCI_CLOSE_DRIVER:	return AVI_mciClose     (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
    case MCI_PLAY:		return AVI_mciPlay      (dwDevID, dwParam1, (LPMCI_PLAY_PARMS)          dwParam2);
    case MCI_RECORD:		return AVI_mciRecord    (dwDevID, dwParam1, (LPMCI_DGV_RECORD_PARMS)    dwParam2);
    case MCI_STOP:		return AVI_mciStop      (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
    case MCI_SET:		return AVI_mciSet       (dwDevID, dwParam1, (LPMCI_DGV_SET_PARMS)       dwParam2);
    case MCI_PAUSE:		return AVI_mciPause     (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
    case MCI_RESUME:		return AVI_mciResume    (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
    case MCI_STATUS:		return AVI_mciStatus    (dwDevID, dwParam1, (LPMCI_DGV_STATUS_PARMSA)   dwParam2);
    case MCI_GETDEVCAPS:	return AVI_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)    dwParam2);
    case MCI_INFO:		return AVI_mciInfo      (dwDevID, dwParam1, (LPMCI_DGV_INFO_PARMSA)     dwParam2);
    case MCI_SEEK:		return AVI_mciSeek      (dwDevID, dwParam1, (LPMCI_SEEK_PARMS)          dwParam2);
    case MCI_PUT:		return AVI_mciPut	(dwDevID, dwParam1, (LPMCI_DGV_PUT_PARMS)       dwParam2);		
    case MCI_WINDOW:		return AVI_mciWindow	(dwDevID, dwParam1, (LPMCI_DGV_WINDOW_PARMSA)   dwParam2);		
    case MCI_LOAD:		return AVI_mciLoad      (dwDevID, dwParam1, (LPMCI_DGV_LOAD_PARMSA)     dwParam2);		
    case MCI_SAVE:		return AVI_mciSave      (dwDevID, dwParam1, (LPMCI_DGV_SAVE_PARMSA)     dwParam2);		
    case MCI_FREEZE:		return AVI_mciFreeze	(dwDevID, dwParam1, (LPMCI_DGV_RECT_PARMS)      dwParam2);
    case MCI_REALIZE:		return AVI_mciRealize   (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
    case MCI_UNFREEZE:		return AVI_mciUnFreeze  (dwDevID, dwParam1, (LPMCI_DGV_RECT_PARMS)      dwParam2);
    case MCI_UPDATE:		return AVI_mciUpdate    (dwDevID, dwParam1, (LPMCI_DGV_UPDATE_PARMS)    dwParam2);
    case MCI_WHERE:		return AVI_mciWhere	(dwDevID, dwParam1, (LPMCI_DGV_RECT_PARMS)      dwParam2);
    case MCI_STEP:		return AVI_mciStep      (dwDevID, dwParam1, (LPMCI_DGV_STEP_PARMS)      dwParam2);
    case MCI_COPY:		return AVI_mciCopy      (dwDevID, dwParam1, (LPMCI_DGV_COPY_PARMS)      dwParam2);		
    case MCI_CUT:		return AVI_mciCut       (dwDevID, dwParam1, (LPMCI_DGV_CUT_PARMS)       dwParam2);		
    case MCI_DELETE:		return AVI_mciDelete    (dwDevID, dwParam1, (LPMCI_DGV_DELETE_PARMS)    dwParam2);				
    case MCI_PASTE:		return AVI_mciPaste     (dwDevID, dwParam1, (LPMCI_DGV_PASTE_PARMS)     dwParam2);				
    case MCI_CUE:		return AVI_mciCue       (dwDevID, dwParam1, (LPMCI_DGV_CUE_PARMS)       dwParam2);
	/* Digital Video specific */	
    case MCI_CAPTURE:		return AVI_mciCapture   (dwDevID, dwParam1, (LPMCI_DGV_CAPTURE_PARMSA)  dwParam2);
    case MCI_MONITOR:		return AVI_mciMonitor   (dwDevID, dwParam1, (LPMCI_DGV_MONITOR_PARMS)   dwParam2);
    case MCI_RESERVE:		return AVI_mciReserve   (dwDevID, dwParam1, (LPMCI_DGV_RESERVE_PARMSA)  dwParam2);
    case MCI_SETAUDIO:		return AVI_mciSetAudio  (dwDevID, dwParam1, (LPMCI_DGV_SETAUDIO_PARMSA) dwParam2);
    case MCI_SIGNAL:		return AVI_mciSignal    (dwDevID, dwParam1, (LPMCI_DGV_SIGNAL_PARMS)    dwParam2);
    case MCI_SETVIDEO:		return AVI_mciSetVideo  (dwDevID, dwParam1, (LPMCI_DGV_SETVIDEO_PARMSA) dwParam2);
    case MCI_QUALITY:		return AVI_mciQuality   (dwDevID, dwParam1, (LPMCI_DGV_QUALITY_PARMSA)  dwParam2);
    case MCI_LIST:		return AVI_mciList      (dwDevID, dwParam1, (LPMCI_DGV_LIST_PARMSA)     dwParam2);
    case MCI_UNDO:		return AVI_mciUndo      (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
    case MCI_CONFIGURE:		return AVI_mciConfigure (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
    case MCI_RESTORE:		return AVI_mciRestore   (dwDevID, dwParam1, (LPMCI_DGV_RESTORE_PARMSA)	dwParam2);
	
    case MCI_SPIN:
    case MCI_ESCAPE:		
	WARN("Unsupported command=%s\n", MCI_MessageToString(wMsg));
	break;
    case MCI_OPEN:
    case MCI_CLOSE:
	FIXME("Shouldn't receive a MCI_OPEN or CLOSE message\n");
	break;
    default:			
	TRACE("Sending msg=%s to default driver proc\n", MCI_MessageToString(wMsg));
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return MCIERR_UNRECOGNIZED_COMMAND;
}

    
