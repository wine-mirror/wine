/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Sample MCI CDAUDIO Wine Driver for Linux
 *
 * Copyright 1994    Martin Ayotte
 * Copyright 1998-99 Eric Pouech
 */

#include "winuser.h"
#include "driver.h"
#include "mmddk.h"
#include "cdrom.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(cdaudio)

typedef struct {
    UINT		wDevID;
    int     		nUseCount;          /* Incremented for each shared open */
    BOOL  		fShareable;         /* TRUE if first open was shareable */
    WORD    		wNotifyDeviceID;    /* MCI device ID with a pending notification */
    HANDLE 		hCallback;          /* Callback handle for pending notification */
    DWORD		dwTimeFormat;
    WINE_CDAUDIO	wcda;
    int			mciMode;
} WINE_MCICDAUDIO;

/*-----------------------------------------------------------------------*/

/**************************************************************************
 * 				CDAUDIO_drvOpen			[internal]	
 */
static	DWORD	CDAUDIO_drvOpen(LPSTR str, LPMCI_OPEN_DRIVER_PARMSA modp)
{
    WINE_MCICDAUDIO*	wmcda = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,  sizeof(WINE_MCICDAUDIO));

    if (!wmcda)
	return 0;

    wmcda->wDevID = modp->wDeviceID;
    mciSetDriverData(wmcda->wDevID, (DWORD)wmcda);
    modp->wCustomCommandTable = MCI_NO_COMMAND_TABLE;
    modp->wType = MCI_DEVTYPE_SEQUENCER;
    return modp->wDeviceID;
}

/**************************************************************************
 * 				CDAUDIO_drvClose		[internal]	
 */
static	DWORD	CDAUDIO_drvClose(DWORD dwDevID)
{
    WINE_MCICDAUDIO*  wmcda = (WINE_MCICDAUDIO*)mciGetDriverData(dwDevID);

    if (wmcda) {
	HeapFree(GetProcessHeap(), 0, wmcda);
	mciSetDriverData(dwDevID, 0);
    }
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciGetOpenDrv		[internal]	
 */
static WINE_MCICDAUDIO*  CDAUDIO_mciGetOpenDrv(UINT wDevID)
{
    WINE_MCICDAUDIO*	wmcda = (WINE_MCICDAUDIO*)mciGetDriverData(wDevID);
    
    if (wmcda == NULL || wmcda->nUseCount == 0 || wmcda->wcda.unixdev <= 0) {
	WARN("Invalid wDevID=%u\n", wDevID);
	return 0;
    }
    return wmcda;
}

/**************************************************************************
 * 				CDAUDIO_mciMode			[internal]
 */
static	int	CDAUDIO_mciMode(int wcdaMode)
{
    switch (wcdaMode) {
    case WINE_CDA_DONTKNOW:	return MCI_MODE_STOP;
    case WINE_CDA_NOTREADY:	return MCI_MODE_STOP;
    case WINE_CDA_OPEN:		return MCI_MODE_OPEN;
    case WINE_CDA_PLAY:		return MCI_MODE_PLAY;
    case WINE_CDA_STOP:		return MCI_MODE_STOP;
    case WINE_CDA_PAUSE:	return MCI_MODE_PAUSE;
    default:
	FIXME("Unknown mode %04x\n", wcdaMode);
    }
    return MCI_MODE_STOP;
}

/**************************************************************************
 * 				CDAUDIO_mciGetError		[internal]
 */
static	int	CDAUDIO_mciGetError(WINE_MCICDAUDIO* wmcda)
{
    switch (wmcda->wcda.cdaMode) {
    case WINE_CDA_DONTKNOW:
    case WINE_CDA_NOTREADY:	return MCIERR_DEVICE_NOT_READY;
    case WINE_CDA_OPEN:		return MCIERR_DEVICE_OPEN;
    case WINE_CDA_PLAY:		
    case WINE_CDA_STOP:		
    case WINE_CDA_PAUSE:	break;
    default:
	FIXME("Unknown mode %04x\n", wmcda->wcda.cdaMode);
    }
    return MCIERR_DRIVER_INTERNAL;
}

/**************************************************************************
 * 			CDAUDIO_CalcFrame			[internal]
 */
static DWORD CDAUDIO_CalcFrame(WINE_MCICDAUDIO* wmcda, DWORD dwTime)
{
    DWORD	dwFrame = 0;
    UINT	wTrack;
    
    TRACE("(%p, %08lX, %lu);\n", wmcda, wmcda->dwTimeFormat, dwTime);
    
    switch (wmcda->dwTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
	dwFrame = (dwTime * CDFRAMES_PERSEC) / 1000;
	TRACE("MILLISECONDS %lu\n", dwFrame);
	break;
    case MCI_FORMAT_MSF:
	TRACE("MSF %02u:%02u:%02u\n",
	      MCI_MSF_MINUTE(dwTime), MCI_MSF_SECOND(dwTime), MCI_MSF_FRAME(dwTime));
	dwFrame += CDFRAMES_PERMIN * MCI_MSF_MINUTE(dwTime);
	dwFrame += CDFRAMES_PERSEC * MCI_MSF_SECOND(dwTime);
	dwFrame += MCI_MSF_FRAME(dwTime);
	break;
    case MCI_FORMAT_TMSF:
    default:
	/* unknown format ! force TMSF ! ... */
	wTrack = MCI_TMSF_TRACK(dwTime);
	TRACE("MSF %02u-%02u:%02u:%02u\n",
	      MCI_TMSF_TRACK(dwTime), MCI_TMSF_MINUTE(dwTime), 
	      MCI_TMSF_SECOND(dwTime), MCI_TMSF_FRAME(dwTime));
	TRACE("TMSF trackpos[%u]=%lu\n",
	      wTrack, wmcda->wcda.lpdwTrackPos[wTrack - 1]);
	dwFrame = wmcda->wcda.lpdwTrackPos[wTrack - 1];
	dwFrame += CDFRAMES_PERMIN * MCI_TMSF_MINUTE(dwTime);
	dwFrame += CDFRAMES_PERSEC * MCI_TMSF_SECOND(dwTime);
	dwFrame += MCI_TMSF_FRAME(dwTime);
	break;
    }
    return dwFrame;
}

/**************************************************************************
 * 			CDAUDIO_CalcTime			[internal]
 */
static DWORD CDAUDIO_CalcTime(WINE_MCICDAUDIO* wmcda, DWORD dwFrame, LPDWORD lpRet)
{
    DWORD	dwTime = 0;
    UINT	wTrack;
    UINT	wMinutes;
    UINT	wSeconds;
    UINT	wFrames;
    
    TRACE("(%p, %08lX, %lu);\n", wmcda, wmcda->dwTimeFormat, dwFrame);
    
    switch (wmcda->dwTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
	dwTime = (dwFrame * 1000) / CDFRAMES_PERSEC;
	TRACE("MILLISECONDS %lu\n", dwTime);
	*lpRet = 0;
	break;
    case MCI_FORMAT_MSF:
	wMinutes = dwFrame / CDFRAMES_PERMIN;
	wSeconds = (dwFrame - CDFRAMES_PERMIN * wMinutes) / CDFRAMES_PERSEC;
	wFrames = dwFrame - CDFRAMES_PERMIN * wMinutes - CDFRAMES_PERSEC * wSeconds;
	dwTime = MCI_MAKE_MSF(wMinutes, wSeconds, wFrames);
	TRACE("MSF %02u:%02u:%02u -> dwTime=%lu\n",
	      wMinutes, wSeconds, wFrames, dwTime);
	*lpRet = MCI_COLONIZED3_RETURN;
	break;
    case MCI_FORMAT_TMSF:
    default:
	/* unknown format ! force TMSF ! ... */
	for (wTrack = 0; wTrack < wmcda->wcda.nTracks; wTrack++) {
	    /*dwTime += wmcda->lpdwTrackLen[wTrack - 1];
	      TRACE("Adding trk#%u curpos=%u \n", dwTime);
	      if (dwTime >= dwFrame) break; */
	    if (wmcda->wcda.lpdwTrackPos[wTrack - 1] >= dwFrame) break;
	}
	wMinutes = dwFrame / CDFRAMES_PERMIN;
	wSeconds = (dwFrame - CDFRAMES_PERMIN * wMinutes) / CDFRAMES_PERSEC;
	wFrames = dwFrame - CDFRAMES_PERMIN * wMinutes - CDFRAMES_PERSEC * wSeconds;
	dwTime = MCI_MAKE_TMSF(wTrack, wMinutes, wSeconds, wFrames);
	TRACE("%02u-%02u:%02u:%02u\n", wTrack, wMinutes, wSeconds, wFrames);
	*lpRet = MCI_COLONIZED4_RETURN;
	break;
    }
    return dwTime;
}

static DWORD CDAUDIO_mciSeek(UINT wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms);
static DWORD CDAUDIO_mciStop(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);

/**************************************************************************
 * 				CDAUDIO_mciOpen			[internal]
 */
static DWORD CDAUDIO_mciOpen(UINT wDevID, DWORD dwFlags, LPMCI_OPEN_PARMSA lpOpenParms)
{
    DWORD		dwDeviceID;
    WINE_MCICDAUDIO* 	wmcda = (WINE_MCICDAUDIO*)mciGetDriverData(wDevID);
    MCI_SEEK_PARMS 	seekParms;

    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpOpenParms);
    
    if (lpOpenParms == NULL) 		return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmcda == NULL)			return MCIERR_INVALID_DEVICE_ID;

    dwDeviceID = lpOpenParms->wDeviceID;

    if (wmcda->nUseCount > 0) {
	/* The driver is already open on this channel */
	/* If the driver was opened shareable before and this open specifies */
	/* shareable then increment the use count */
	if (wmcda->fShareable && (dwFlags & MCI_OPEN_SHAREABLE))
	    ++wmcda->nUseCount;
	else
	    return MCIERR_MUST_USE_SHAREABLE;
    } else {
	wmcda->nUseCount = 1;
	wmcda->fShareable = dwFlags & MCI_OPEN_SHAREABLE;
    }
    if (dwFlags & MCI_OPEN_ELEMENT) {
	TRACE("MCI_OPEN_ELEMENT !\n");
	/*		return MCIERR_NO_ELEMENT_ALLOWED; */
    }

    wmcda->wNotifyDeviceID = dwDeviceID;
    if (CDAUDIO_Open(&wmcda->wcda) == -1) {
	--wmcda->nUseCount;
	return MCIERR_HARDWARE;
    }
    wmcda->mciMode = MCI_MODE_STOP;
    wmcda->dwTimeFormat = MCI_FORMAT_TMSF;
    if (!CDAUDIO_GetTracksInfo(&wmcda->wcda)) {
	WARN("error reading TracksInfo !\n");
	/*		return MCIERR_INTERNAL; */
    }
    
    CDAUDIO_mciSeek(wDevID, MCI_SEEK_TO_START, &seekParms);

    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciClose		[internal]
 */
static DWORD CDAUDIO_mciClose(UINT wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);

    TRACE("(%04X, %08lX, %p);\n", wDevID, dwParam, lpParms);

    if (wmcda == NULL) 	return MCIERR_INVALID_DEVICE_ID;
    
    if (wmcda->nUseCount == 1) {
	/* FIXME: I don't think we have to stop CD on exit
	 * CDAUDIO_mciStop(wDevID, 0, NULL); 
	 */
	CDAUDIO_Close(&wmcda->wcda);
    }
    wmcda->nUseCount--;
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciGetDevCaps		[internal]
 */
static DWORD CDAUDIO_mciGetDevCaps(UINT wDevID, DWORD dwFlags, 
				   LPMCI_GETDEVCAPS_PARMS lpParms)
{
    DWORD	ret = 0;

    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;

    if (dwFlags & MCI_GETDEVCAPS_ITEM) {
	TRACE("MCI_GETDEVCAPS_ITEM dwItem=%08lX;\n", lpParms->dwItem);

	switch (lpParms->dwItem) {
	case MCI_GETDEVCAPS_CAN_RECORD:
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_HAS_AUDIO:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_HAS_VIDEO:
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_DEVICE_TYPE:
	    lpParms->dwReturn = MAKEMCIRESOURCE(MCI_DEVTYPE_CD_AUDIO, MCI_DEVTYPE_CD_AUDIO);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_USES_FILES:
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_COMPOUND_DEVICE:
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_EJECT:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_PLAY:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_SAVE:
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	default:
	    ERR("Unsupported %lx devCaps item\n", lpParms->dwItem);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    } else {
	TRACE("No GetDevCaps-Item !\n");
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    TRACE("lpParms->dwReturn=%08lX;\n", lpParms->dwReturn);
    return ret;
}

/**************************************************************************
 * 				CDAUDIO_mciInfo			[internal]
 */
static DWORD CDAUDIO_mciInfo(UINT wDevID, DWORD dwFlags, LPMCI_INFO_PARMSA lpParms)
{
    LPSTR		str = 0;
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    
    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL || lpParms->lpstrReturn == NULL)
	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmcda == NULL) return MCIERR_INVALID_DEVICE_ID;

    TRACE("buf=%p, len=%lu\n", lpParms->lpstrReturn, lpParms->dwRetSize);
    
    switch(dwFlags) {
    case MCI_INFO_PRODUCT:
	str = "Wine's audio CD";
	break;
    default:
	WARN("Don't know this info command (%lu)\n", dwFlags);
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    return MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize, str);
}

/**************************************************************************
 * 				CDAUDIO_mciStatus		[internal]
 */
static DWORD CDAUDIO_mciStatus(UINT wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    DWORD	        ret = 0;
    
    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmcda == NULL) return MCIERR_INVALID_DEVICE_ID;

    if (dwFlags & MCI_NOTIFY) {
	TRACE("MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			wmcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    if (dwFlags & MCI_STATUS_ITEM) {
	switch (lpParms->dwItem) {
	case MCI_STATUS_CURRENT_TRACK:
	    if (!CDAUDIO_GetCDStatus(&wmcda->wcda)) {
		return CDAUDIO_mciGetError(wmcda);
	    }
	    lpParms->dwReturn = wmcda->wcda.nCurTrack;
	    TRACE("CURRENT_TRACK=%lu!\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_LENGTH:
	    if (wmcda->wcda.nTracks == 0) {
		if (!CDAUDIO_GetTracksInfo(&wmcda->wcda)) {
		    WARN("error reading TracksInfo !\n");
		    return CDAUDIO_mciGetError(wmcda);
		}
	    }
	    if (dwFlags & MCI_TRACK) {
		TRACE("MCI_TRACK #%lu LENGTH=??? !\n", lpParms->dwTrack);
		if (lpParms->dwTrack > wmcda->wcda.nTracks || lpParms->dwTrack == 0)
		    return MCIERR_OUTOFRANGE;
		lpParms->dwReturn = wmcda->wcda.lpdwTrackLen[lpParms->dwTrack - 1];
	    } else {
		lpParms->dwReturn = wmcda->wcda.dwTotalLen;
	    }
	    lpParms->dwReturn = CDAUDIO_CalcTime(wmcda, lpParms->dwReturn, &ret);
	    TRACE("LENGTH=%lu !\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_MODE:
	    if (!CDAUDIO_GetCDStatus(&wmcda->wcda)) 
		return CDAUDIO_mciGetError(wmcda);
	    lpParms->dwReturn = CDAUDIO_mciMode(wmcda->wcda.cdaMode);
	    if (!lpParms->dwReturn) lpParms->dwReturn = wmcda->mciMode;
	    TRACE("MCI_STATUS_MODE=%08lX !\n", lpParms->dwReturn);
	    lpParms->dwReturn = MAKEMCIRESOURCE(lpParms->dwReturn, lpParms->dwReturn);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_MEDIA_PRESENT:
	    if (!CDAUDIO_GetCDStatus(&wmcda->wcda)) 
		return CDAUDIO_mciGetError(wmcda);
	    lpParms->dwReturn = (wmcda->wcda.nTracks == 0) ? 
		MAKEMCIRESOURCE(FALSE, MCI_FALSE) : MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    TRACE("MCI_STATUS_MEDIA_PRESENT =%s!\n", LOWORD(lpParms->dwReturn) ? "Y" : "N");
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_NUMBER_OF_TRACKS:
	    lpParms->dwReturn = CDAUDIO_GetNumberOfTracks(&wmcda->wcda);
	    TRACE("MCI_STATUS_NUMBER_OF_TRACKS = %lu !\n", lpParms->dwReturn);
	    if (lpParms->dwReturn == (WORD)-1) 
		return CDAUDIO_mciGetError(wmcda);
	    break;
	case MCI_STATUS_POSITION:
	    if (!CDAUDIO_GetCDStatus(&wmcda->wcda)) 
		return CDAUDIO_mciGetError(wmcda);
	    lpParms->dwReturn = wmcda->wcda.dwCurFrame;
	    if (dwFlags & MCI_STATUS_START) {
		lpParms->dwReturn = wmcda->wcda.dwFirstOffset;
		TRACE("get MCI_STATUS_START !\n");
	    }
	    if (dwFlags & MCI_TRACK) {
		if (lpParms->dwTrack > wmcda->wcda.nTracks || lpParms->dwTrack == 0)
		    return MCIERR_OUTOFRANGE;
		lpParms->dwReturn = wmcda->wcda.lpdwTrackPos[lpParms->dwTrack - 1];
		TRACE("get MCI_TRACK #%lu !\n", lpParms->dwTrack);
	    }
	    lpParms->dwReturn = CDAUDIO_CalcTime(wmcda, lpParms->dwReturn, &ret);
	    TRACE("MCI_STATUS_POSITION=%08lX !\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_READY:
	    TRACE("MCI_STATUS_READY !\n");
	    lpParms->dwReturn = (wmcda->wcda.cdaMode == WINE_CDA_DONTKNOW ||
				 wmcda->wcda.cdaMode == WINE_CDA_NOTREADY) ?
		MAKEMCIRESOURCE(FALSE, MCI_FALSE) : MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    TRACE("MCI_STATUS_READY=%u!\n", LOWORD(lpParms->dwReturn));
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_TIME_FORMAT:
	    lpParms->dwReturn = MAKEMCIRESOURCE(wmcda->dwTimeFormat, wmcda->dwTimeFormat);
	    TRACE("MCI_STATUS_TIME_FORMAT=%08x!\n", LOWORD(lpParms->dwReturn));
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_CDA_STATUS_TYPE_TRACK:
	    if (!(dwFlags & MCI_TRACK)) 
		ret = MCIERR_MISSING_PARAMETER;
	    else if (lpParms->dwTrack > wmcda->wcda.nTracks || lpParms->dwTrack == 0)
		ret = MCIERR_OUTOFRANGE;
	    else
		lpParms->dwReturn = (wmcda->wcda.lpbTrackFlags[lpParms->dwTrack - 1] & 
				     CDROM_DATA_TRACK) ? MCI_CDA_TRACK_OTHER : MCI_CDA_TRACK_AUDIO;
	    TRACE("MCI_CDA_STATUS_TYPE_TRACK[%ld]=%08lx\n", lpParms->dwTrack, lpParms->dwReturn);
	    break;
	default:
	    FIXME("unknown command %08lX !\n", lpParms->dwItem);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    } else {
	WARN("not MCI_STATUS_ITEM !\n");
    }
    return ret;
}

/**************************************************************************
 * 				CDAUDIO_mciPlay			[internal]
 */
static DWORD CDAUDIO_mciPlay(UINT wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
    int 		start, end;
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    DWORD		ret = 0;
    
    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL) {
	ret = MCIERR_NULL_PARAMETER_BLOCK;
    } else if (wmcda == NULL) {
	ret = MCIERR_INVALID_DEVICE_ID;
    } else {
	if (wmcda->wcda.nTracks == 0) {
	    if (!CDAUDIO_GetTracksInfo(&wmcda->wcda)) {
		WARN("error reading TracksInfo !\n");
		return MCIERR_DRIVER_INTERNAL;
	    }
	}
	wmcda->wcda.nCurTrack = 1;
	if (dwFlags & MCI_FROM) {
	    start = CDAUDIO_CalcFrame(wmcda, lpParms->dwFrom);
	    TRACE("MCI_FROM=%08lX -> %u \n", lpParms->dwFrom, start);
	} else {
	    if (!CDAUDIO_GetCDStatus(&wmcda->wcda)) return MCIERR_DRIVER_INTERNAL;
	    start = wmcda->wcda.dwCurFrame;
	}
	if (dwFlags & MCI_TO) {
	    end = CDAUDIO_CalcFrame(wmcda, lpParms->dwTo);
	    TRACE("MCI_TO=%08lX -> %u \n", lpParms->dwTo, end);
	} else {
	    end = wmcda->wcda.dwFirstOffset + wmcda->wcda.dwTotalLen;
	}
	
	if (CDAUDIO_Play(&wmcda->wcda, start, end) == -1)
	    return MCIERR_HARDWARE;
	wmcda->mciMode = MCI_MODE_PLAY;
	if (dwFlags & MCI_NOTIFY) {
	    TRACE("MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	    /*
	      mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
	      wmcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
	    */
	}
    }
    return ret;
}

/**************************************************************************
 * 				CDAUDIO_mciStop			[internal]
 */
static DWORD CDAUDIO_mciStop(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    
    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmcda == NULL)	return MCIERR_INVALID_DEVICE_ID;
    
    if (CDAUDIO_Stop(&wmcda->wcda) == -1)
	return MCIERR_HARDWARE;

    wmcda->mciMode = MCI_MODE_STOP;
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE("MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			wmcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciPause		[internal]
 */
static DWORD CDAUDIO_mciPause(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    
    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmcda == NULL)	return MCIERR_INVALID_DEVICE_ID;
    
    if (CDAUDIO_Pause(&wmcda->wcda, 1) == -1)
	return MCIERR_HARDWARE;
    wmcda->mciMode = MCI_MODE_PAUSE;
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
        TRACE("MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			wmcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciResume		[internal]
 */
static DWORD CDAUDIO_mciResume(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    
    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmcda == NULL)	return MCIERR_INVALID_DEVICE_ID;
    
    if (CDAUDIO_Pause(&wmcda->wcda, 0) == -1)
	return MCIERR_HARDWARE;
    wmcda->mciMode = MCI_MODE_STOP;
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE("MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			  wmcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciSeek			[internal]
 */
static DWORD CDAUDIO_mciSeek(UINT wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
    DWORD		at;
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    
    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmcda == NULL)	return MCIERR_INVALID_DEVICE_ID;
    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;
    
    wmcda->mciMode = MCI_MODE_SEEK;
    switch (dwFlags & ~(MCI_NOTIFY|MCI_WAIT)) {
    case MCI_SEEK_TO_START:
	TRACE("Seeking to start\n");
	at = 0;
	break;
    case MCI_SEEK_TO_END:
	TRACE("Seeking to end\n");
	at = wmcda->wcda.dwTotalLen;
	break;
    case MCI_TO:
	TRACE("Seeking to %lu\n", lpParms->dwTo);
	at = lpParms->dwTo;
	break;
    default:
	TRACE("Seeking to ??=%lu\n", dwFlags);
	return MCIERR_UNSUPPORTED_FUNCTION;
    }
    if (CDAUDIO_Seek(&wmcda->wcda, at) == -1) {
	return MCIERR_HARDWARE;
    }
    if (dwFlags & MCI_NOTIFY) {
	TRACE("MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			  wmcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciSetDoor		[internal]
 */
static DWORD	CDAUDIO_mciSetDoor(UINT wDevID, int open)
{
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    
    TRACE("(%04x, %s) !\n", wDevID, (open) ? "OPEN" : "CLOSE");
    
    if (wmcda == NULL) return MCIERR_INVALID_DEVICE_ID;
    
    if (CDAUDIO_SetDoor(&wmcda->wcda, open) == -1)
	return MCIERR_HARDWARE;
    wmcda->mciMode = (open) ? MCI_MODE_OPEN : MCI_MODE_STOP;
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciSet			[internal]
 */
static DWORD CDAUDIO_mciSet(UINT wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    
    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmcda == NULL)	return MCIERR_INVALID_DEVICE_ID;
    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;;
    /*
      TRACE("dwTimeFormat=%08lX\n", lpParms->dwTimeFormat);
      TRACE("dwAudio=%08lX\n", lpParms->dwAudio);
    */
    if (dwFlags & MCI_SET_TIME_FORMAT) {
	switch (lpParms->dwTimeFormat) {
	case MCI_FORMAT_MILLISECONDS:
	    TRACE("MCI_FORMAT_MILLISECONDS !\n");
	    break;
	case MCI_FORMAT_MSF:
	    TRACE("MCI_FORMAT_MSF !\n");
	    break;
	case MCI_FORMAT_TMSF:
	    TRACE("MCI_FORMAT_TMSF !\n");
	    break;
	default:
	    WARN("bad time format !\n");
	    return MCIERR_BAD_TIME_FORMAT;
	}
	wmcda->dwTimeFormat = lpParms->dwTimeFormat;
    }
    if (dwFlags & MCI_SET_DOOR_OPEN) {
	CDAUDIO_mciSetDoor(wDevID, TRUE);
    }
    if (dwFlags & MCI_SET_DOOR_CLOSED) {
	CDAUDIO_mciSetDoor(wDevID, FALSE);
    }
    if (dwFlags & MCI_SET_VIDEO) return MCIERR_UNSUPPORTED_FUNCTION;
    if (dwFlags & MCI_SET_ON) return MCIERR_UNSUPPORTED_FUNCTION;
    if (dwFlags & MCI_SET_OFF) return MCIERR_UNSUPPORTED_FUNCTION;
    if (dwFlags & MCI_NOTIFY) {
	TRACE("MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			  wmcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 			MCICDAUDIO_DriverProc			[sample driver]
 */
LONG CALLBACK	MCICDAUDIO_DriverProc(DWORD dwDevID, HDRVR hDriv, DWORD wMsg, 
				      DWORD dwParam1, DWORD dwParam2)
{
    switch(wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return CDAUDIO_drvOpen((LPSTR)dwParam1, (LPMCI_OPEN_DRIVER_PARMSA)dwParam2);
    case DRV_CLOSE:		return CDAUDIO_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;	
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "Sample Multimedia Driver !", "Wine Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
	
    case MCI_OPEN_DRIVER:	return CDAUDIO_mciOpen(dwDevID, dwParam1, (LPMCI_OPEN_PARMSA)dwParam2);
    case MCI_CLOSE_DRIVER:	return CDAUDIO_mciClose(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_GETDEVCAPS:	return CDAUDIO_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)dwParam2);
    case MCI_INFO:		return CDAUDIO_mciInfo(dwDevID, dwParam1, (LPMCI_INFO_PARMSA)dwParam2);
    case MCI_STATUS:		return CDAUDIO_mciStatus(dwDevID, dwParam1, (LPMCI_STATUS_PARMS)dwParam2);
    case MCI_SET:		return CDAUDIO_mciSet(dwDevID, dwParam1, (LPMCI_SET_PARMS)dwParam2);
    case MCI_PLAY:		return CDAUDIO_mciPlay(dwDevID, dwParam1, (LPMCI_PLAY_PARMS)dwParam2);
    case MCI_STOP:		return CDAUDIO_mciStop(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_PAUSE:		return CDAUDIO_mciPause(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_RESUME:		return CDAUDIO_mciResume(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_SEEK:		return CDAUDIO_mciSeek(dwDevID, dwParam1, (LPMCI_SEEK_PARMS)dwParam2);
    /* FIXME: I wonder if those two next items are really called ? */
    case MCI_SET_DOOR_OPEN:	return CDAUDIO_mciSetDoor(dwDevID, TRUE);
    case MCI_SET_DOOR_CLOSED:	return CDAUDIO_mciSetDoor(dwDevID, FALSE);
    /* commands that should be supported */
    case MCI_LOAD:		
    case MCI_SAVE:		
    case MCI_FREEZE:		
    case MCI_PUT:		
    case MCI_REALIZE:		
    case MCI_UNFREEZE:		
    case MCI_UPDATE:		
    case MCI_WHERE:		
    case MCI_STEP:		
    case MCI_SPIN:		
    case MCI_ESCAPE:		
    case MCI_COPY:		
    case MCI_CUT:		
    case MCI_DELETE:		
    case MCI_PASTE:		
	FIXME("Unsupported yet command=%s\n", MCI_MessageToString(wMsg));
	break;
    /* commands that should report an error */
    case MCI_WINDOW:		
	FIXME("Unsupported command=%s\n", MCI_MessageToString(wMsg));
	break;
    case MCI_OPEN:
    case MCI_CLOSE:
	ERR("Shouldn't receive a MCI_OPEN or CLOSE message\n");
	break;
    default:
	TRACE("Sending msg=%s to default driver proc\n", MCI_MessageToString(wMsg));
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return MCIERR_UNRECOGNIZED_COMMAND;
}

/*-----------------------------------------------------------------------*/
