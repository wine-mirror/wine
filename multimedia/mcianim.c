/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Sample MCI ANIMATION Wine Driver for Linux
 *
 * Copyright 1994 Martin Ayotte
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "windows.h"
#include "user.h"
#include "driver.h"
#include "mmsystem.h"
#include "debug.h"
#include "multimedia.h"

#define MAX_ANIMDRV 		2

#define ANIMFRAMES_PERSEC 	30
#define ANIMFRAMES_PERMIN 	1800
#define SECONDS_PERMIN	 	60

typedef struct {
	int     	nUseCount;          /* Incremented for each shared open */
	BOOL16  	fShareable;         /* TRUE if first open was shareable */
	WORD    	wNotifyDeviceID;    /* MCI device ID with a pending notification */
	HANDLE16 	hCallback;          /* Callback handle for pending notification */
	MCI_OPEN_PARMS32A 	openParms;
	DWORD		dwTimeFormat;
	int		mode;
	UINT16		nCurTrack;
	DWORD		dwCurFrame;
	UINT16		nTracks;
	DWORD		dwTotalLen;
	LPDWORD		lpdwTrackLen;
	LPDWORD		lpdwTrackPos;
} WINE_MCIANIM;

static WINE_MCIANIM	AnimDev[MAX_ANIMDRV];

/*-----------------------------------------------------------------------*/

/**************************************************************************
 * 				ANIM_mciGetOpenDrv		[internal]	
 */
static WINE_MCIANIM*  ANIM_mciGetOpenDrv(UINT16 wDevID)
{
    if (wDevID >= MAX_ANIMDRV || AnimDev[wDevID].nUseCount == 0) {
	WARN(mcianim, "Invalid wDevID=%u\n", wDevID);
	return 0;
    }
    return &AnimDev[wDevID];
}

/**************************************************************************
 * 				ANIM_mciOpen			[internal]
 */
static DWORD ANIM_mciOpen(UINT16 wDevID, DWORD dwFlags, LPMCI_OPEN_PARMS32A lpOpenParms)
{
    DWORD		dwDeviceID;
    WINE_MCIANIM*	wma;

    TRACE(mcianim,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpOpenParms);

    if (lpOpenParms == NULL) return MCIERR_INTERNAL;
    if (wDevID >= MAX_ANIMDRV)	return MCIERR_INVALID_DEVICE_ID;

    wma = &AnimDev[wDevID];

    if (wma->nUseCount > 0) {
	/* The driver already open on this channel */
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

    dwDeviceID = lpOpenParms->wDeviceID;

    TRACE(mcianim,"wDevID=%04X\n", wDevID);
    /* FIXME this is not consistent with other implementations */
    lpOpenParms->wDeviceID = wDevID;

    /*TRACE(mcianim,"lpParms->wDevID=%04X\n", lpParms->wDeviceID);*/
    if (dwFlags & MCI_OPEN_ELEMENT) {
	TRACE(mcianim,"MCI_OPEN_ELEMENT '%s' !\n", lpOpenParms->lpstrElementName);
	if (lpOpenParms->lpstrElementName && strlen(lpOpenParms->lpstrElementName) > 0) {
	}
	FIXME(mcianim, "element is not opened\n");
    }
    memcpy(&wma->openParms, lpOpenParms, sizeof(MCI_OPEN_PARMS32A));
    wma->wNotifyDeviceID = dwDeviceID;
    wma->mode = 0;
    wma->dwTimeFormat = MCI_FORMAT_TMSF;
    wma->nCurTrack = 0;
    wma->nTracks = 0;
    wma->dwTotalLen = 0;
    wma->lpdwTrackLen = NULL;
    wma->lpdwTrackPos = NULL;
    /*
      Moved to mmsystem.c mciOpen routine 
      
      if (dwFlags & MCI_NOTIFY) {
      TRACE(mcianim, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
      lpParms->dwCallback);
      mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
      wma->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
      }
    */
    return 0;
}

/**************************************************************************
 * 				ANIM_mciClose		[internal]
 */
static DWORD ANIM_mciClose(UINT16 wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIANIM*	wma = ANIM_mciGetOpenDrv(wDevID);

    TRACE(mcianim,"(%u, %08lX, %p);\n", wDevID, dwParam, lpParms);

    if (wma == NULL)	 return MCIERR_INVALID_DEVICE_ID;

    if (--wma->nUseCount == 0) {
	if (wma->lpdwTrackLen != NULL) free(wma->lpdwTrackLen);
	if (wma->lpdwTrackPos != NULL) free(wma->lpdwTrackPos);
    }
    return 0;
}

/**************************************************************************
 * 				ANIM_mciGetDevCaps	[internal]
 */
static DWORD ANIM_mciGetDevCaps(UINT16 wDevID, DWORD dwFlags, 
				LPMCI_GETDEVCAPS_PARMS lpParms)
{
    WINE_MCIANIM*	wma = ANIM_mciGetOpenDrv(wDevID);

    TRACE(mcianim,"(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)	 return MCIERR_INVALID_DEVICE_ID;

    if (dwFlags & MCI_GETDEVCAPS_ITEM) {
	TRACE(mcianim, "MCI_GETDEVCAPS_ITEM dwItem=%08lX;\n", lpParms->dwItem);
	switch(lpParms->dwItem) {
	case MCI_GETDEVCAPS_CAN_RECORD:
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_HAS_AUDIO:
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_HAS_VIDEO:
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_DEVICE_TYPE:
	    lpParms->dwReturn = MCI_DEVTYPE_ANIMATION;
	    break;
	case MCI_GETDEVCAPS_USES_FILES:
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_COMPOUND_DEVICE:
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_CAN_EJECT:
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_CAN_PLAY:
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_CAN_SAVE:
	    lpParms->dwReturn = FALSE;
	    break;
	default:
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    }
    TRACE(mcianim, "lpParms->dwReturn=%08lX;\n", lpParms->dwReturn);
    return 0;
}


/**************************************************************************
 * 				ANIM_CalcTime			[internal]
 */
static DWORD ANIM_CalcTime(WINE_MCIANIM* wma, DWORD dwFormatType, DWORD dwFrame)
{
    DWORD	dwTime = 0;
    UINT16	wTrack;
    UINT16	wMinutes;
    UINT16	wSeconds;
    UINT16	wFrames;
    
    TRACE(mcianim,"(%p, %08lX, %lu);\n", wma, dwFormatType, dwFrame);
    
    switch (dwFormatType) {
    case MCI_FORMAT_MILLISECONDS:
	dwTime = dwFrame / ANIMFRAMES_PERSEC * 1000;
	TRACE(mcianim, "MILLISECONDS %lu\n", dwTime);
	break;
    case MCI_FORMAT_MSF:
	wMinutes = dwFrame / ANIMFRAMES_PERMIN;
	wSeconds = (dwFrame - ANIMFRAMES_PERMIN * wMinutes) / ANIMFRAMES_PERSEC;
	wFrames = dwFrame - ANIMFRAMES_PERMIN * wMinutes - 
	    ANIMFRAMES_PERSEC * wSeconds;
	dwTime = MCI_MAKE_MSF(wMinutes, wSeconds, wFrames);
	TRACE(mcianim,"MSF %02u:%02u:%02u -> dwTime=%lu\n",
	      wMinutes, wSeconds, wFrames, dwTime);
	break;
    default:
	/* unknown format ! force TMSF ! ... */
	dwFormatType = MCI_FORMAT_TMSF;
    case MCI_FORMAT_TMSF:
	for (wTrack = 0; wTrack < wma->nTracks; wTrack++) {
	    /*				dwTime += wma->lpdwTrackLen[wTrack - 1];
					TRACE(mcianim, "Adding trk#%u curpos=%u \n", dwTime);
					if (dwTime >= dwFrame) break; */
	    if (wma->lpdwTrackPos[wTrack - 1] >= dwFrame) break;
	}
	wMinutes = dwFrame / ANIMFRAMES_PERMIN;
	wSeconds = (dwFrame - ANIMFRAMES_PERMIN * wMinutes) / ANIMFRAMES_PERSEC;
	wFrames = dwFrame - ANIMFRAMES_PERMIN * wMinutes - 
	    ANIMFRAMES_PERSEC * wSeconds;
	dwTime = MCI_MAKE_TMSF(wTrack, wMinutes, wSeconds, wFrames);
	TRACE(mcianim, "%02u-%02u:%02u:%02u\n",
	      wTrack, wMinutes, wSeconds, wFrames);
	break;
    }
    return dwTime;
}


/**************************************************************************
 * 				ANIM_CalcFrame			[internal]
 */
static DWORD ANIM_CalcFrame(WINE_MCIANIM* wma, DWORD dwFormatType, DWORD dwTime)
{
    DWORD	dwFrame = 0;
    UINT16	wTrack;

    TRACE(mcianim,"(%p, %08lX, %lu);\n", wma, dwFormatType, dwTime);
    
    switch (dwFormatType) {
    case MCI_FORMAT_MILLISECONDS:
	dwFrame = dwTime * ANIMFRAMES_PERSEC / 1000;
	TRACE(mcianim, "MILLISECONDS %lu\n", dwFrame);
	break;
    case MCI_FORMAT_MSF:
	TRACE(mcianim, "MSF %02u:%02u:%02u\n",
	      MCI_MSF_MINUTE(dwTime), MCI_MSF_SECOND(dwTime), 
	      MCI_MSF_FRAME(dwTime));
	dwFrame += ANIMFRAMES_PERMIN * MCI_MSF_MINUTE(dwTime);
	dwFrame += ANIMFRAMES_PERSEC * MCI_MSF_SECOND(dwTime);
	dwFrame += MCI_MSF_FRAME(dwTime);
	break;
    default:
	/* unknown format ! force TMSF ! ... */
	dwFormatType = MCI_FORMAT_TMSF;
    case MCI_FORMAT_TMSF:
	wTrack = MCI_TMSF_TRACK(dwTime);
	TRACE(mcianim, "TMSF %02u-%02u:%02u:%02u\n",
	      MCI_TMSF_TRACK(dwTime), MCI_TMSF_MINUTE(dwTime), 
	      MCI_TMSF_SECOND(dwTime), MCI_TMSF_FRAME(dwTime));
	TRACE(mcianim, "TMSF trackpos[%u]=%lu\n",
	      wTrack, wma->lpdwTrackPos[wTrack - 1]);
	dwFrame = wma->lpdwTrackPos[wTrack - 1];
	dwFrame += ANIMFRAMES_PERMIN * MCI_TMSF_MINUTE(dwTime);
	dwFrame += ANIMFRAMES_PERSEC * MCI_TMSF_SECOND(dwTime);
	dwFrame += MCI_TMSF_FRAME(dwTime);
	break;
    }
    return dwFrame;
}


/**************************************************************************
 * 				ANIM_mciInfo			[internal]
 */
static DWORD ANIM_mciInfo(UINT16 wDevID, DWORD dwFlags, LPMCI_INFO_PARMS16 lpParms)
{
    WINE_MCIANIM*	wma = ANIM_mciGetOpenDrv(wDevID);
    DWORD		ret = 0;
    LPSTR		str = 0;

    TRACE(mcianim,"(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL || lpParms->lpstrReturn == NULL) {
	ret = MCIERR_NULL_PARAMETER_BLOCK;
    } else if (wma == NULL) {
	ret = MCIERR_INVALID_DEVICE_ID;
    } else {
	TRACE(mcianim, "buf=%p, len=%lu\n", lpParms->lpstrReturn, lpParms->dwRetSize);

    switch(dwFlags) {
    case MCI_INFO_PRODUCT:
	    str = "Wine's animation";
	break;
    case MCI_INFO_FILE:
	    str = wma->openParms.lpstrElementName;
	break;
    case MCI_ANIM_INFO_TEXT:
	    str = "Animation Window";
	break;
    default:
	    WARN(mcianim, "Don't know this info command (%lu)\n", dwFlags);
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
 * 				ANIM_mciStatus		[internal]
 */
static DWORD ANIM_mciStatus(UINT16 wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
    WINE_MCIANIM*	wma = ANIM_mciGetOpenDrv(wDevID);

    TRACE(mcianim,"(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) return MCIERR_INTERNAL;
    if (wma == NULL) return MCIERR_INVALID_DEVICE_ID;

    if (dwFlags & MCI_NOTIFY) {
	TRACE(mcianim, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wma->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    if (dwFlags & MCI_STATUS_ITEM) {
	switch(lpParms->dwItem) {
	case MCI_STATUS_CURRENT_TRACK:
	    lpParms->dwReturn = wma->nCurTrack;
	    TRACE(mcianim,"CURRENT_TRACK=%lu!\n", lpParms->dwReturn);
	    return 0;
	case MCI_STATUS_LENGTH:
	    if (dwFlags & MCI_TRACK) {
		TRACE(mcianim,"MCI_TRACK #%lu LENGTH=??? !\n",
		      lpParms->dwTrack);
		if (lpParms->dwTrack > wma->nTracks)
		    return MCIERR_OUTOFRANGE;
		lpParms->dwReturn = wma->lpdwTrackLen[lpParms->dwTrack];
	    }
	    else
		lpParms->dwReturn = wma->dwTotalLen;
	    lpParms->dwReturn = ANIM_CalcTime(wma, wma->dwTimeFormat, lpParms->dwReturn);
	    TRACE(mcianim,"LENGTH=%lu !\n", lpParms->dwReturn);
	    return 0;
	case MCI_STATUS_MODE:
	    lpParms->dwReturn = wma->mode;
	    TRACE(mcianim,"MCI_STATUS_MODE=%08lX !\n",
		  lpParms->dwReturn);
	    return 0;
	case MCI_STATUS_MEDIA_PRESENT:
	    lpParms->dwReturn = TRUE;
	    TRACE(mcianim,"MCI_STATUS_MEDIA_PRESENT !\n");
	    return 0;
	case MCI_STATUS_NUMBER_OF_TRACKS:
	    lpParms->dwReturn = 1;
	    TRACE(mcianim,"MCI_STATUS_NUMBER_OF_TRACKS = %lu !\n",
		  lpParms->dwReturn);
	    if (lpParms->dwReturn == (WORD)-1) return MCIERR_INTERNAL;
	    return 0;
	case MCI_STATUS_POSITION:
	    lpParms->dwReturn = wma->dwCurFrame;
	    if (dwFlags & MCI_STATUS_START) {
		lpParms->dwReturn = 0;
		TRACE(mcianim,"get MCI_STATUS_START !\n");
	    }
	    if (dwFlags & MCI_TRACK) {
		if (lpParms->dwTrack > wma->nTracks)
		    return MCIERR_OUTOFRANGE;
		lpParms->dwReturn = wma->lpdwTrackPos[lpParms->dwTrack - 1];
		TRACE(mcianim,"get MCI_TRACK #%lu !\n", lpParms->dwTrack);
	    }
	    lpParms->dwReturn = ANIM_CalcTime(wma, wma->dwTimeFormat, lpParms->dwReturn);
	    TRACE(mcianim,"MCI_STATUS_POSITION=%08lX !\n",
		  lpParms->dwReturn);
	    return 0;
	case MCI_STATUS_READY:
	    TRACE(mcianim,"MCI_STATUS_READY !\n");
	    lpParms->dwReturn = TRUE;
	    return 0;
	case MCI_STATUS_TIME_FORMAT:
	    TRACE(mcianim,"MCI_STATUS_TIME_FORMAT !\n");
	    lpParms->dwReturn = MCI_FORMAT_MILLISECONDS;
	    return 0;
	default:
	    WARN(mcianim,"Unknown command %08lX !\n", lpParms->dwItem);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    }
    WARN(mcianim,"Not MCI_STATUS_ITEM !\n");
    return 0;
}


/**************************************************************************
 * 				ANIM_mciPlay			[internal]
 */
static DWORD ANIM_mciPlay(UINT16 wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
    WINE_MCIANIM*	wma = ANIM_mciGetOpenDrv(wDevID);
    int 	start, end;

    TRACE(mcianim,"(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) return MCIERR_INTERNAL;
    if (wma == NULL) return MCIERR_INVALID_DEVICE_ID;

    start = 0; 		end = wma->dwTotalLen;
    wma->nCurTrack = 1;
    if (dwFlags & MCI_FROM) {
	start = ANIM_CalcFrame(wma, wma->dwTimeFormat, lpParms->dwFrom); 
        TRACE(mcianim,"MCI_FROM=%08lX -> %u \n", lpParms->dwFrom, start);
    }
    if (dwFlags & MCI_TO) {
	end = ANIM_CalcFrame(wma, wma->dwTimeFormat, lpParms->dwTo);
	TRACE(mcianim, "MCI_TO=%08lX -> %u \n", lpParms->dwTo, end);
    }
    wma->mode = MCI_MODE_PLAY;
    if (dwFlags & MCI_NOTIFY) {
	TRACE(mcianim, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wma->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				ANIM_mciStop			[internal]
 */
static DWORD ANIM_mciStop(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIANIM*	wma = ANIM_mciGetOpenDrv(wDevID);

    TRACE(mcianim,"(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) return MCIERR_INTERNAL;
    if (wma == NULL) return MCIERR_INVALID_DEVICE_ID;

    wma->mode = MCI_MODE_STOP;
    if (dwFlags & MCI_NOTIFY) {
	TRACE(mcianim, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			wma->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				ANIM_mciPause		[internal]
 */
static DWORD ANIM_mciPause(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIANIM*	wma = ANIM_mciGetOpenDrv(wDevID);

    TRACE(mcianim,"(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    if (lpParms == NULL) return MCIERR_INTERNAL;
    wma->mode = MCI_MODE_PAUSE;
    if (dwFlags & MCI_NOTIFY) {
	TRACE(mcianim, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			wma->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				ANIM_mciResume		[internal]
 */
static DWORD ANIM_mciResume(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIANIM*	wma = ANIM_mciGetOpenDrv(wDevID);

    TRACE(mcianim,"(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    if (lpParms == NULL) return MCIERR_INTERNAL;
    wma->mode = MCI_MODE_STOP;
    if (dwFlags & MCI_NOTIFY) {
	TRACE(mcianim, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			wma->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				ANIM_mciSeek			[internal]
 */
static DWORD ANIM_mciSeek(UINT16 wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
    WINE_MCIANIM*	wma = ANIM_mciGetOpenDrv(wDevID);
    DWORD	dwRet;
    MCI_PLAY_PARMS PlayParms;

    TRACE(mcianim,"(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) return MCIERR_INTERNAL;
    wma->mode = MCI_MODE_SEEK;
    switch(dwFlags) {
    case MCI_SEEK_TO_START:
	PlayParms.dwFrom = 0;
	break;
    case MCI_SEEK_TO_END:
	PlayParms.dwFrom = wma->dwTotalLen;
	break;
    case MCI_TO:
	PlayParms.dwFrom = lpParms->dwTo;
	break;
    }
    dwRet = ANIM_mciPlay(wDevID, MCI_WAIT | MCI_FROM, &PlayParms);
    if (dwRet != 0) return dwRet;
    dwRet = ANIM_mciStop(wDevID, MCI_WAIT, (LPMCI_GENERIC_PARMS)&PlayParms);
    if (dwFlags & MCI_NOTIFY) {
	TRACE(mcianim, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			wma->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return dwRet;
}


/**************************************************************************
 * 				ANIM_mciSet			[internal]
 */
static DWORD ANIM_mciSet(UINT16 wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
{
    WINE_MCIANIM*	wma = ANIM_mciGetOpenDrv(wDevID);

    TRACE(mcianim,"(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    if (lpParms == NULL) return MCIERR_INTERNAL;
    if (wma == NULL) return MCIERR_INVALID_DEVICE_ID;
    /*
      TRACE(mcianim,"(dwTimeFormat=%08lX)\n", lpParms->dwTimeFormat);
      TRACE(mcianim,"(dwAudio=%08lX)\n", lpParms->dwAudio);
    */
    if (dwFlags & MCI_SET_TIME_FORMAT) {
	switch (lpParms->dwTimeFormat) {
	case MCI_FORMAT_MILLISECONDS:
	    TRACE(mcianim, "MCI_FORMAT_MILLISECONDS !\n");
	    break;
	case MCI_FORMAT_MSF:
	    TRACE(mcianim,"MCI_FORMAT_MSF !\n");
	    break;
	case MCI_FORMAT_TMSF:
	    TRACE(mcianim,"MCI_FORMAT_TMSF !\n");
	    break;
	default:
	    WARN(mcianim,"Bad time format !\n");
	    return MCIERR_BAD_TIME_FORMAT;
	}
	wma->dwTimeFormat = lpParms->dwTimeFormat;
    }
    if (dwFlags & MCI_SET_VIDEO) return MCIERR_UNSUPPORTED_FUNCTION;
    if (dwFlags & MCI_SET_ON) return MCIERR_UNSUPPORTED_FUNCTION;
    if (dwFlags & MCI_SET_OFF) return MCIERR_UNSUPPORTED_FUNCTION;
    if (dwFlags & MCI_NOTIFY) {
	TRACE(mcianim, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			wma->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				ANIM_DriverProc32		[sample driver]
 */
LONG MCIANIM_DriverProc32(DWORD dwDevID, HDRVR16 hDriv, DWORD wMsg, 
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
    case DRV_CONFIGURE:		MessageBox16(0, "Sample MultiMedia Linux Driver !", "MMLinux Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;

    case MCI_OPEN_DRIVER:	return ANIM_mciOpen(dwDevID, dwParam1, (LPMCI_OPEN_PARMS32A)dwParam2); 
    case MCI_CLOSE_DRIVER:	return ANIM_mciClose(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_GETDEVCAPS:	return ANIM_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)dwParam2);
    case MCI_INFO:		return ANIM_mciInfo(dwDevID, dwParam1, (LPMCI_INFO_PARMS16)dwParam2);
    case MCI_STATUS:		return ANIM_mciStatus(dwDevID, dwParam1, (LPMCI_STATUS_PARMS)dwParam2);
    case MCI_SET:		return ANIM_mciSet(dwDevID, dwParam1, (LPMCI_SET_PARMS)dwParam2);
    case MCI_PLAY:		return ANIM_mciPlay(dwDevID, dwParam1, (LPMCI_PLAY_PARMS)dwParam2);
    case MCI_STOP:		return ANIM_mciStop(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_PAUSE:		return ANIM_mciPause(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_RESUME:		return ANIM_mciResume(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_SEEK:		return ANIM_mciSeek(dwDevID, dwParam1, (LPMCI_SEEK_PARMS)dwParam2);
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
	WARN(mcianim, "Unsupported command=%s\n", MCI_CommandToString(wMsg));
	break;
    case MCI_OPEN:
    case MCI_CLOSE:
	FIXME(mcianim, "Shouldn't receive a MCI_OPEN or CLOSE message\n");
	break;
    default:			
	TRACE(mcianim, "Sending msg=%s to default driver proc\n", MCI_CommandToString(wMsg));
	return DefDriverProc32(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return MCIERR_UNRECOGNIZED_COMMAND;
}


/*-----------------------------------------------------------------------*/
