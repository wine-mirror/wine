/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Sample MCI CDAUDIO Wine Driver for Linux
 *
 * Copyright 1994    Martin Ayotte
 * Copyright 1998-99 Eric Pouech
 */

#include "winuser.h"
#include "driver.h"
#include "multimedia.h"
#include "debug.h"
#include "cdrom.h"

typedef struct {
    int     		nUseCount;          /* Incremented for each shared open */
    BOOL16  		fShareable;         /* TRUE if first open was shareable */
    WORD    		wNotifyDeviceID;    /* MCI device ID with a pending notification */
    HANDLE16 		hCallback;          /* Callback handle for pending notification */
    MCI_OPEN_PARMS16 	openParms;
    DWORD		dwTimeFormat;
    WINE_CDAUDIO	wcda;
    int			mciMode;
} WINE_MCICDAUDIO;

#define MAX_CDAUDIODRV 			(1)
static WINE_MCICDAUDIO	CDADev[MAX_CDAUDIODRV];

/*-----------------------------------------------------------------------*/

/**************************************************************************
 * 				CDAUDIO_mciGetOpenDrv		[internal]	
 */
static WINE_MCICDAUDIO*  CDAUDIO_mciGetOpenDrv(UINT16 wDevID)
{
    if (wDevID >= MAX_CDAUDIODRV || CDADev[wDevID].nUseCount == 0 || 
	CDADev[wDevID].wcda.unixdev <= 0) {
	WARN(cdaudio, "Invalid wDevID=%u\n", wDevID);
	return 0;
    }
    return &CDADev[wDevID];
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
	FIXME(cdaudio, "Unknown mode %04x\n", wcdaMode);
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
	FIXME(cdaudio, "Unknown mode %04x\n", wmcda->wcda.cdaMode);
    }
    return MCIERR_DRIVER_INTERNAL;
}

/**************************************************************************
 * 			CDAUDIO_CalcFrame			[internal]
 */
static DWORD CDAUDIO_CalcFrame(WINE_MCICDAUDIO* wmcda, DWORD dwTime)
{
    DWORD	dwFrame = 0;
    UINT16	wTrack;
    
    TRACE(cdaudio,"(%p, %08lX, %lu);\n", wmcda, wmcda->dwTimeFormat, dwTime);
    
    switch (wmcda->dwTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
	dwFrame = (dwTime * CDFRAMES_PERSEC) / 1000;
	TRACE(cdaudio, "MILLISECONDS %lu\n", dwFrame);
	break;
    case MCI_FORMAT_MSF:
	TRACE(cdaudio, "MSF %02u:%02u:%02u\n",
	      MCI_MSF_MINUTE(dwTime), MCI_MSF_SECOND(dwTime), MCI_MSF_FRAME(dwTime));
	dwFrame += CDFRAMES_PERMIN * MCI_MSF_MINUTE(dwTime);
	dwFrame += CDFRAMES_PERSEC * MCI_MSF_SECOND(dwTime);
	dwFrame += MCI_MSF_FRAME(dwTime);
	break;
    case MCI_FORMAT_TMSF:
    default:
	/* unknown format ! force TMSF ! ... */
	wTrack = MCI_TMSF_TRACK(dwTime);
	TRACE(cdaudio, "MSF %02u-%02u:%02u:%02u\n",
	      MCI_TMSF_TRACK(dwTime), MCI_TMSF_MINUTE(dwTime), 
	      MCI_TMSF_SECOND(dwTime), MCI_TMSF_FRAME(dwTime));
	TRACE(cdaudio, "TMSF trackpos[%u]=%lu\n",
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
static DWORD CDAUDIO_CalcTime(WINE_MCICDAUDIO* wmcda, DWORD dwFrame)
{
    DWORD	dwTime = 0;
    UINT16	wTrack;
    UINT16	wMinutes;
    UINT16	wSeconds;
    UINT16	wFrames;
    
    TRACE(cdaudio,"(%p, %08lX, %lu);\n", wmcda, wmcda->dwTimeFormat, dwFrame);
    
    switch (wmcda->dwTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
	dwTime = (dwFrame * 1000) / CDFRAMES_PERSEC;
	TRACE(cdaudio, "MILLISECONDS %lu\n", dwTime);
	break;
    case MCI_FORMAT_MSF:
	wMinutes = dwFrame / CDFRAMES_PERMIN;
	wSeconds = (dwFrame - CDFRAMES_PERMIN * wMinutes) / CDFRAMES_PERSEC;
	wFrames = dwFrame - CDFRAMES_PERMIN * wMinutes - CDFRAMES_PERSEC * wSeconds;
	dwTime = MCI_MAKE_MSF(wMinutes, wSeconds, wFrames);
	TRACE(cdaudio,"MSF %02u:%02u:%02u -> dwTime=%lu\n",wMinutes, wSeconds, wFrames, dwTime);
	break;
    case MCI_FORMAT_TMSF:
    default:
	/* unknown format ! force TMSF ! ... */
	for (wTrack = 0; wTrack < wmcda->wcda.nTracks; wTrack++) {
	    /*				dwTime += wmcda->lpdwTrackLen[wTrack - 1];
					TRACE(cdaudio, "Adding trk#%u curpos=%u \n", dwTime);
					if (dwTime >= dwFrame) break; */
	    if (wmcda->wcda.lpdwTrackPos[wTrack - 1] >= dwFrame) break;
	}
	wMinutes = dwFrame / CDFRAMES_PERMIN;
	wSeconds = (dwFrame - CDFRAMES_PERMIN * wMinutes) / CDFRAMES_PERSEC;
	wFrames = dwFrame - CDFRAMES_PERMIN * wMinutes - CDFRAMES_PERSEC * wSeconds;
	dwTime = MCI_MAKE_TMSF(wTrack, wMinutes, wSeconds, wFrames);
	TRACE(cdaudio, "%02u-%02u:%02u:%02u\n", wTrack, wMinutes, wSeconds, wFrames);
	break;
    }
    return dwTime;
}

static DWORD CDAUDIO_mciSeek(UINT16 wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms);
static DWORD CDAUDIO_mciStop(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);

/**************************************************************************
 * 				CDAUDIO_mciOpen			[internal]
 */
static DWORD CDAUDIO_mciOpen(UINT16 wDevID, DWORD dwFlags, LPMCI_OPEN_PARMS32A lpOpenParms)
{
    DWORD		dwDeviceID;
    WINE_MCICDAUDIO* 	wmcda;
    MCI_SEEK_PARMS 	seekParms;

    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpOpenParms);
    
    if (lpOpenParms == NULL) 		return MCIERR_NULL_PARAMETER_BLOCK;
    if (wDevID > MAX_CDAUDIODRV)	return MCIERR_INVALID_DEVICE_ID;

    dwDeviceID = lpOpenParms->wDeviceID;

    wmcda = &CDADev[wDevID];

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
	TRACE(cdaudio,"MCI_OPEN_ELEMENT !\n");
	/*		return MCIERR_NO_ELEMENT_ALLOWED; */
    }

    wmcda->openParms.dwCallback = lpOpenParms->dwCallback;
    wmcda->openParms.wDeviceID  = (WORD)lpOpenParms->wDeviceID;
    wmcda->openParms.wReserved0 = 0; /*????*/
    wmcda->openParms.lpstrDeviceType = lpOpenParms->lpstrDeviceType;
    wmcda->openParms.lpstrElementName = lpOpenParms->lpstrElementName;
    wmcda->openParms.lpstrAlias = lpOpenParms->lpstrAlias;

    wmcda->wNotifyDeviceID = dwDeviceID;
    if (CDAUDIO_Open(&wmcda->wcda) == -1) {
	--wmcda->nUseCount;
	return MCIERR_HARDWARE;
    }
    wmcda->mciMode = MCI_MODE_STOP;
    wmcda->dwTimeFormat = MCI_FORMAT_TMSF;
    if (!CDAUDIO_GetTracksInfo(&wmcda->wcda)) {
	WARN(cdaudio,"error reading TracksInfo !\n");
	/*		return MCIERR_INTERNAL; */
    }
    
    CDAUDIO_mciSeek(wDevID, MCI_SEEK_TO_START, &seekParms);

    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciClose		[internal]
 */
static DWORD CDAUDIO_mciClose(UINT16 wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);

    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwParam, lpParms);

    if (wmcda == NULL) 	return MCIERR_INVALID_DEVICE_ID;
    
    if (wmcda->nUseCount == 1) {
	CDAUDIO_mciStop(wDevID, 0, NULL);
	CDAUDIO_Close(&wmcda->wcda);
    }
    wmcda->nUseCount--;
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciGetDevCaps		[internal]
 */
static DWORD CDAUDIO_mciGetDevCaps(UINT16 wDevID, DWORD dwFlags, 
				   LPMCI_GETDEVCAPS_PARMS lpParms)
{
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;

    if (dwFlags & MCI_GETDEVCAPS_ITEM) {
	TRACE(cdaudio, "MCI_GETDEVCAPS_ITEM dwItem=%08lX;\n", lpParms->dwItem);

	switch(lpParms->dwItem) {
	case MCI_GETDEVCAPS_CAN_RECORD:
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_HAS_AUDIO:
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_HAS_VIDEO:
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_DEVICE_TYPE:
	    lpParms->dwReturn = MCI_DEVTYPE_CD_AUDIO;
	    break;
	case MCI_GETDEVCAPS_USES_FILES:
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_COMPOUND_DEVICE:
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_CAN_EJECT:
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_CAN_PLAY:
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_CAN_SAVE:
	    lpParms->dwReturn = FALSE;
	    break;
	default:
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    }
    TRACE(cdaudio, "lpParms->dwReturn=%08lX;\n", lpParms->dwReturn);
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciInfo			[internal]
 */
static DWORD CDAUDIO_mciInfo(UINT16 wDevID, DWORD dwFlags, LPMCI_INFO_PARMS16 lpParms)
{
    DWORD		ret = 0;
    LPSTR		str = 0;
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL || lpParms->lpstrReturn == NULL) {
	ret = MCIERR_NULL_PARAMETER_BLOCK;
    } else if (wmcda == NULL) {
	ret = MCIERR_INVALID_DEVICE_ID;
    } else {
	TRACE(cdaudio, "buf=%p, len=%lu\n", lpParms->lpstrReturn, lpParms->dwRetSize);
	
	switch(dwFlags) {
	case MCI_INFO_PRODUCT:
	    str = "Wine's audio CD";
	    break;
	default:
	    WARN(cdaudio, "Don't know this info command (%lu)\n", dwFlags);
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

/**************************************************************************
 * 				CDAUDIO_mciStatus		[internal]
 */
static DWORD CDAUDIO_mciStatus(UINT16 wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    DWORD	        ret = 0;
    
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL) {
	ret = MCIERR_NULL_PARAMETER_BLOCK;
    } else if (wmcda == NULL) {
	ret = MCIERR_INVALID_DEVICE_ID;
    } else {
	if (dwFlags & MCI_NOTIFY) {
	    TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	    mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			      wmcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
	}
	if (dwFlags & MCI_STATUS_ITEM) {
	    switch (lpParms->dwItem) {
	    case MCI_STATUS_CURRENT_TRACK:
		if (!CDAUDIO_GetCDStatus(&wmcda->wcda)) {
		    return CDAUDIO_mciGetError(wmcda);
		}
		lpParms->dwReturn = wmcda->wcda.nCurTrack;
		TRACE(cdaudio,"CURRENT_TRACK=%lu!\n", lpParms->dwReturn);
		return 0;
	    case MCI_STATUS_LENGTH:
		if (wmcda->wcda.nTracks == 0) {
		    if (!CDAUDIO_GetTracksInfo(&wmcda->wcda)) {
			WARN(cdaudio, "error reading TracksInfo !\n");
			return CDAUDIO_mciGetError(wmcda);
		    }
		}
		if (dwFlags & MCI_TRACK) {
		    TRACE(cdaudio,"MCI_TRACK #%lu LENGTH=??? !\n", lpParms->dwTrack);
		    if (lpParms->dwTrack > wmcda->wcda.nTracks || lpParms->dwTrack == 0)
			return MCIERR_OUTOFRANGE;
		    lpParms->dwReturn = wmcda->wcda.lpdwTrackLen[lpParms->dwTrack - 1];
		} else {
		    lpParms->dwReturn = wmcda->wcda.dwTotalLen;
		}
		lpParms->dwReturn = CDAUDIO_CalcTime(wmcda, lpParms->dwReturn);
		TRACE(cdaudio,"LENGTH=%lu !\n", lpParms->dwReturn);
		return 0;
	    case MCI_STATUS_MODE:
		if (!CDAUDIO_GetCDStatus(&wmcda->wcda)) 
		    return CDAUDIO_mciGetError(wmcda);
		lpParms->dwReturn = CDAUDIO_mciMode(wmcda->wcda.cdaMode);
		if (!lpParms->dwReturn) lpParms->dwReturn = wmcda->mciMode;
		TRACE(cdaudio,"MCI_STATUS_MODE=%08lX !\n", lpParms->dwReturn);
		return 0;
	    case MCI_STATUS_MEDIA_PRESENT:
		if (!CDAUDIO_GetCDStatus(&wmcda->wcda)) 
		    return CDAUDIO_mciGetError(wmcda);
		lpParms->dwReturn = (wmcda->wcda.nTracks > 0) ? TRUE : FALSE;
		TRACE(cdaudio,"MCI_STATUS_MEDIA_PRESENT =%s!\n", lpParms->dwReturn ? "Y" : "N");
		return 0;
	    case MCI_STATUS_NUMBER_OF_TRACKS:
		lpParms->dwReturn = CDAUDIO_GetNumberOfTracks(&wmcda->wcda);
		TRACE(cdaudio,"MCI_STATUS_NUMBER_OF_TRACKS = %lu !\n", lpParms->dwReturn);
		if (lpParms->dwReturn == (WORD)-1) 
		    return CDAUDIO_mciGetError(wmcda);
		return 0;
	    case MCI_STATUS_POSITION:
		if (!CDAUDIO_GetCDStatus(&wmcda->wcda)) 
		    return CDAUDIO_mciGetError(wmcda);
		lpParms->dwReturn = wmcda->wcda.dwCurFrame;
		if (dwFlags & MCI_STATUS_START) {
		    lpParms->dwReturn = wmcda->wcda.dwFirstOffset;
		    TRACE(cdaudio,"get MCI_STATUS_START !\n");
		}
		if (dwFlags & MCI_TRACK) {
		    if (lpParms->dwTrack > wmcda->wcda.nTracks || lpParms->dwTrack == 0)
			return MCIERR_OUTOFRANGE;
		    lpParms->dwReturn = wmcda->wcda.lpdwTrackPos[lpParms->dwTrack - 1];
		    TRACE(cdaudio,"get MCI_TRACK #%lu !\n", lpParms->dwTrack);
		}
		lpParms->dwReturn = CDAUDIO_CalcTime(wmcda, lpParms->dwReturn);
		TRACE(cdaudio,"MCI_STATUS_POSITION=%08lX !\n", lpParms->dwReturn);
		return 0;
	    case MCI_STATUS_READY:
		TRACE(cdaudio,"MCI_STATUS_READY !\n");
		lpParms->dwReturn = (wmcda->wcda.cdaMode != WINE_CDA_DONTKNOW && wmcda->wcda.cdaMode != WINE_CDA_NOTREADY);
		TRACE(cdaudio,"MCI_STATUS_READY=%ld!\n", lpParms->dwReturn);
		return 0;
	    case MCI_STATUS_TIME_FORMAT:
		lpParms->dwReturn = wmcda->dwTimeFormat;
		TRACE(cdaudio,"MCI_STATUS_TIME_FORMAT =%08lx!\n", lpParms->dwReturn);
		return 0;
	    case MCI_CDA_STATUS_TYPE_TRACK:
		if (!(dwFlags & MCI_TRACK)) 
		    return MCIERR_MISSING_PARAMETER;
		if (lpParms->dwTrack > wmcda->wcda.nTracks || lpParms->dwTrack == 0)
		    return MCIERR_OUTOFRANGE;
		lpParms->dwReturn = (wmcda->wcda.lpbTrackFlags[lpParms->dwTrack - 1] & 
				     CDROM_DATA_TRACK) ? MCI_CDA_TRACK_OTHER : MCI_CDA_TRACK_AUDIO;
		TRACE(cdaudio, "MCI_CDA_STATUS_TYPE_TRACK[%ld]=%08lx\n", lpParms->dwTrack, lpParms->dwReturn);
		return 0;
	    default:
		WARN(cdaudio, "unknown command %08lX !\n", lpParms->dwItem);
		return MCIERR_UNRECOGNIZED_COMMAND;
	    }
	}
    }
    WARN(cdaudio, "not MCI_STATUS_ITEM !\n");
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciPlay			[internal]
 */
static DWORD CDAUDIO_mciPlay(UINT16 wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
    int 		start, end;
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    DWORD		ret = 0;
    
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL) {
	ret = MCIERR_NULL_PARAMETER_BLOCK;
    } else if (wmcda == NULL) {
	ret = MCIERR_INVALID_DEVICE_ID;
    } else {
	if (wmcda->wcda.nTracks == 0) {
	    if (!CDAUDIO_GetTracksInfo(&wmcda->wcda)) {
		WARN(cdaudio, "error reading TracksInfo !\n");
		return MCIERR_DRIVER_INTERNAL;
	    }
	}
	end = wmcda->wcda.dwTotalLen;
	wmcda->wcda.nCurTrack = 1;
	if (dwFlags & MCI_FROM) {
	    start = CDAUDIO_CalcFrame(wmcda, lpParms->dwFrom); 
	    TRACE(cdaudio,"MCI_FROM=%08lX -> %u \n", lpParms->dwFrom, start);
	} else {
	    if (!CDAUDIO_GetCDStatus(&wmcda->wcda)) return MCIERR_DRIVER_INTERNAL;
	    start = wmcda->wcda.dwCurFrame;
	}
	if (dwFlags & MCI_TO) {
	    end = CDAUDIO_CalcFrame(wmcda, lpParms->dwTo);
	    TRACE(cdaudio, "MCI_TO=%08lX -> %u \n", lpParms->dwTo, end);
	}
	end += wmcda->wcda.dwFirstOffset;
	
	if (CDAUDIO_Play(&wmcda->wcda, start, end) == -1)
	    return MCIERR_HARDWARE;
	wmcda->mciMode = MCI_MODE_PLAY;
	if (dwFlags & MCI_NOTIFY) {
	    TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	    /*
	      mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
	      wmcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
	    */
	}
    }
    return ret;
}

/**************************************************************************
 * 				CDAUDIO_mciStop			[internal]
 */
static DWORD CDAUDIO_mciStop(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmcda == NULL)	return MCIERR_INVALID_DEVICE_ID;
    
    if (CDAUDIO_Stop(&wmcda->wcda) == -1)
	return MCIERR_HARDWARE;
    wmcda->mciMode = MCI_MODE_STOP;
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciPause		[internal]
 */
static DWORD CDAUDIO_mciPause(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmcda == NULL)	return MCIERR_INVALID_DEVICE_ID;
    
    if (CDAUDIO_Pause(&wmcda->wcda, 1) == -1)
	return MCIERR_HARDWARE;
    wmcda->mciMode = MCI_MODE_PAUSE;
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
        TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciResume		[internal]
 */
static DWORD CDAUDIO_mciResume(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmcda == NULL)	return MCIERR_INVALID_DEVICE_ID;
    
    if (CDAUDIO_Pause(&wmcda->wcda, 0) == -1)
	return MCIERR_HARDWARE;
    wmcda->mciMode = MCI_MODE_STOP;
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciSeek			[internal]
 */
static DWORD CDAUDIO_mciSeek(UINT16 wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
    DWORD	dwRet;
    MCI_PLAY_PARMS 	playParms;
    
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmcda == NULL)	return MCIERR_INVALID_DEVICE_ID;
    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;
    
    wmcda->mciMode = MCI_MODE_SEEK;
    switch (dwFlags & ~(MCI_NOTIFY|MCI_WAIT)) {
    case MCI_SEEK_TO_START:
	TRACE(cdaudio, "Seeking to start\n");
	playParms.dwFrom = 0;
	break;
    case MCI_SEEK_TO_END:
	TRACE(cdaudio, "Seeking to end\n");
	playParms.dwFrom = wmcda->wcda.dwTotalLen;
	break;
    case MCI_TO:
	TRACE(cdaudio, "Seeking to %lu\n", lpParms->dwTo);
	playParms.dwFrom = lpParms->dwTo;
	break;
    default:
	TRACE(cdaudio, "Seeking to ??=%lu\n", dwFlags);
	return MCIERR_UNSUPPORTED_FUNCTION;
    }
    dwRet = CDAUDIO_mciPlay(wDevID, MCI_WAIT | MCI_FROM, &playParms);
    if (dwRet != 0) return dwRet;
    dwRet = CDAUDIO_mciStop(wDevID, MCI_WAIT, (LPMCI_GENERIC_PARMS)&playParms);
    if (dwFlags & MCI_NOTIFY) {
	TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return dwRet;
}

/**************************************************************************
 * 				CDAUDIO_mciSetDoor		[internal]
 */
static DWORD	CDAUDIO_mciSetDoor(UINT16 wDevID, int open)
{
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    
    TRACE(cdaudio, "(%04x, %s) !\n", wDevID, (open) ? "OPEN" : "CLOSE");
    
    if (wmcda == NULL) return MCIERR_INVALID_DEVICE_ID;
    
    if (CDAUDIO_SetDoor(&wmcda->wcda, open) == -1)
	return MCIERR_HARDWARE;
    wmcda->mciMode = (open) ? MCI_MODE_OPEN : MCI_MODE_STOP;
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciSet			[internal]
 */
static DWORD CDAUDIO_mciSet(UINT16 wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = CDAUDIO_mciGetOpenDrv(wDevID);
    
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmcda == NULL)	return MCIERR_INVALID_DEVICE_ID;
    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;;
    /*
      TRACE(cdaudio,"dwTimeFormat=%08lX\n", lpParms->dwTimeFormat);
      TRACE(cdaudio,"dwAudio=%08lX\n", lpParms->dwAudio);
    */
    if (dwFlags & MCI_SET_TIME_FORMAT) {
	switch (lpParms->dwTimeFormat) {
	case MCI_FORMAT_MILLISECONDS:
	    TRACE(cdaudio, "MCI_FORMAT_MILLISECONDS !\n");
	    break;
	case MCI_FORMAT_MSF:
	    TRACE(cdaudio,"MCI_FORMAT_MSF !\n");
	    break;
	case MCI_FORMAT_TMSF:
	    TRACE(cdaudio,"MCI_FORMAT_TMSF !\n");
	    break;
	default:
	    WARN(cdaudio, "bad time format !\n");
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
	TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 			MCICDAUDIO_DriverProc32		[sample driver]
 */
LONG MCICDAUDIO_DriverProc32(DWORD dwDevID, HDRVR16 hDriv, DWORD wMsg, 
			     DWORD dwParam1, DWORD dwParam2)
{
    switch(wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return 1;
    case DRV_CLOSE:		return 1;
    case DRV_ENABLE:		return 1;	
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBox32A(0, "Sample Multimedia Linux Driver !", "MMLinux Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
	
    case MCI_OPEN_DRIVER:	return CDAUDIO_mciOpen(dwDevID, dwParam1, (LPMCI_OPEN_PARMS32A)dwParam2);
    case MCI_CLOSE_DRIVER:	return CDAUDIO_mciClose(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_GETDEVCAPS:	return CDAUDIO_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)dwParam2);
    case MCI_INFO:		return CDAUDIO_mciInfo(dwDevID, dwParam1, (LPMCI_INFO_PARMS16)dwParam2);
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
	WARN(cdaudio, "Unsupported command=%s\n", MCI_CommandToString(wMsg));
	break;
    case MCI_OPEN:
    case MCI_CLOSE:
	FIXME(cdaudio, "Shouldn't receive a MCI_OPEN or CLOSE message\n");
	break;
    default:
	TRACE(cdaudio, "Sending msg=%s to default driver proc\n", MCI_CommandToString(wMsg));
	return DefDriverProc32(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return MCIERR_UNRECOGNIZED_COMMAND;
}

/*-----------------------------------------------------------------------*/
