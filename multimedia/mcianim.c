/*
 * Sample MCI ANIMATION Wine Driver for Linux
 *
 * Copyright 1994 Martin Ayotte
 *
static char Copyright[] = "Copyright  Martin Ayotte, 1994";
*/
#ifndef WINELIB
#define BUILTIN_MMSYSTEM
#endif 

#ifdef BUILTIN_MMSYSTEM

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "windows.h"
#include "user.h"
#include "driver.h"
#include "mmsystem.h"
#include "stddebug.h"
/* #define DEBUG_MCIANIM */
#define DEBUG_MCIANIM
#include "debug.h"

#define MAX_ANIMDRV 		2

#define ANIMFRAMES_PERSEC 	30
#define ANIMFRAMES_PERMIN 	1800
#define SECONDS_PERMIN	 	60

#ifdef linux
typedef struct {
    int     nUseCount;          /* Incremented for each shared open */
    BOOL    fShareable;         /* TRUE if first open was shareable */
    WORD    wNotifyDeviceID;    /* MCI device ID with a pending notification */
    HANDLE  hCallback;          /* Callback handle for pending notification */
	MCI_OPEN_PARMS openParms;
	DWORD	dwTimeFormat;
	int		mode;
	UINT	nCurTrack;
	DWORD	dwCurFrame;
	UINT	nTracks;
	DWORD	dwTotalLen;
	LPDWORD	lpdwTrackLen;
	LPDWORD	lpdwTrackPos;
	} LINUX_ANIM;

static LINUX_ANIM	AnimDev[MAX_ANIMDRV];
#endif


DWORD ANIM_CalcTime(UINT wDevID, DWORD dwFormatType, DWORD dwFrame);
DWORD ANIM_CalcFrame(UINT wDevID, DWORD dwFormatType, DWORD dwTime);


/*-----------------------------------------------------------------------*/

/**************************************************************************
* 				ANIM_mciOpen			[internal]
*/
DWORD ANIM_mciOpen(DWORD dwFlags, LPMCI_OPEN_PARMS lpParms)
{
#ifdef linux
	UINT	wDevID;
	dprintf_mcianim(stddeb,"ANIM_mciOpen(%08lX, %p);\n", dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	wDevID = lpParms->wDeviceID;
	if (AnimDev[wDevID].nUseCount > 0) {
		/* The driver already open on this channel */
		/* If the driver was opened shareable before and this open specifies */
		/* shareable then increment the use count */
		if (AnimDev[wDevID].fShareable && (dwFlags & MCI_OPEN_SHAREABLE))
			++AnimDev[wDevID].nUseCount;
		else
			return MCIERR_MUST_USE_SHAREABLE;
		}
	else {
		AnimDev[wDevID].nUseCount = 1;
		AnimDev[wDevID].fShareable = dwFlags & MCI_OPEN_SHAREABLE;
		}
    if (dwFlags & MCI_OPEN_ELEMENT) {
        dprintf_mcianim(stddeb,"ANIM_mciOpen // MCI_OPEN_ELEMENT !\n");
        printf("ANIM_mciOpen // MCI_OPEN_ELEMENT !\n");
/*		return MCIERR_NO_ELEMENT_ALLOWED; */
		}
	memcpy(&AnimDev[wDevID].openParms, lpParms, sizeof(MCI_OPEN_PARMS));
	AnimDev[wDevID].wNotifyDeviceID = lpParms->wDeviceID;
	AnimDev[wDevID].mode = 0;
	AnimDev[wDevID].dwTimeFormat = MCI_FORMAT_TMSF;
	AnimDev[wDevID].nCurTrack = 0;
	AnimDev[wDevID].nTracks = 0;
	AnimDev[wDevID].dwTotalLen = 0;
	AnimDev[wDevID].lpdwTrackLen = NULL;
	AnimDev[wDevID].lpdwTrackPos = NULL;
	if (dwFlags & MCI_NOTIFY) {
		dprintf_mcianim(stddeb,
			"ANIM_mciOpen // MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
			lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			AnimDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
 	return 0;
#else
	return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
* 				ANIM_mciClose		[internal]
*/
DWORD ANIM_mciClose(UINT wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
#ifdef linux
	dprintf_mcianim(stddeb,"ANIM_mciClose(%u, %08lX, %p);\n", 
				wDevID, dwParam, lpParms);
	if (AnimDev[wDevID].lpdwTrackLen != NULL) free(AnimDev[wDevID].lpdwTrackLen);
	if (AnimDev[wDevID].lpdwTrackPos != NULL) free(AnimDev[wDevID].lpdwTrackPos);
#endif
        return 0;
}

/**************************************************************************
* 				ANIM_mciGetDevCaps	[internal]
*/
DWORD ANIM_mciGetDevCaps(UINT wDevID, DWORD dwFlags, 
						LPMCI_GETDEVCAPS_PARMS lpParms)
{
#ifdef linux
	dprintf_mcianim(stddeb,"ANIM_mciGetDevCaps(%u, %08lX, %p);\n", 
				wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	if (dwFlags & MCI_GETDEVCAPS_ITEM) {
        	dprintf_mcianim(stddeb,
		"ANIM_mciGetDevCaps // MCI_GETDEVCAPS_ITEM dwItem=%08lX;\n",
				lpParms->dwItem);
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
	dprintf_mcianim(stddeb,
		"ANIM_mciGetDevCaps // lpParms->dwReturn=%08lX;\n", 
		lpParms->dwReturn);
 	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}

/**************************************************************************
* 				ANIM_mciInfo			[internal]
*/
DWORD ANIM_mciInfo(UINT wDevID, DWORD dwFlags, LPMCI_INFO_PARMS lpParms)
{
#ifdef linux
	dprintf_mcianim(stddeb,"ANIM_mciInfo(%u, %08lX, %p);\n", 
		wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	lpParms->lpstrReturn = NULL;
	switch(dwFlags) {
		case MCI_INFO_PRODUCT:
			lpParms->lpstrReturn = "Linux ANIMATION 0.5";
			break;
		case MCI_INFO_FILE:
			lpParms->lpstrReturn = 
				(LPSTR)AnimDev[wDevID].openParms.lpstrElementName;
			break;
		case MCI_ANIM_INFO_TEXT:
			lpParms->lpstrReturn = "Animation Window";
			break;
		default:
			return MCIERR_UNRECOGNIZED_COMMAND;
		}
	if (lpParms->lpstrReturn != NULL)
		lpParms->dwRetSize = strlen(lpParms->lpstrReturn);
	else
		lpParms->dwRetSize = 0;
 	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}

/**************************************************************************
* 				ANIM_mciStatus		[internal]
*/
DWORD ANIM_mciStatus(UINT wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
#ifdef linux
	dprintf_mcianim(stddeb,"ANIM_mciStatus(%u, %08lX, %p);\n", 
			wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	if (dwFlags & MCI_NOTIFY) {
		dprintf_mcianim(stddeb,
			"ANIM_mciStatus // MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
			lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			AnimDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
	if (dwFlags & MCI_STATUS_ITEM) {
		switch(lpParms->dwItem) {
			case MCI_STATUS_CURRENT_TRACK:
				lpParms->dwReturn = AnimDev[wDevID].nCurTrack;
				dprintf_mcianim(stddeb,"ANIM_mciStatus // CURRENT_TRACK=%lu!\n", lpParms->dwReturn);
			 	return 0;
			case MCI_STATUS_LENGTH:
				if (dwFlags & MCI_TRACK) {
					dprintf_mcianim(stddeb,"ANIM_mciStatus // MCI_TRACK #%lu LENGTH=??? !\n",
														lpParms->dwTrack);
					if (lpParms->dwTrack > AnimDev[wDevID].nTracks)
						return MCIERR_OUTOFRANGE;
					lpParms->dwReturn = AnimDev[wDevID].lpdwTrackLen[lpParms->dwTrack];
					}
				else
					lpParms->dwReturn = AnimDev[wDevID].dwTotalLen;
				lpParms->dwReturn = ANIM_CalcTime(wDevID, 
					AnimDev[wDevID].dwTimeFormat, lpParms->dwReturn);
                		dprintf_mcianim(stddeb,"ANIM_mciStatus // LENGTH=%lu !\n", lpParms->dwReturn);
			 	return 0;
			case MCI_STATUS_MODE:
				lpParms->dwReturn = AnimDev[wDevID].mode;
				dprintf_mcianim(stddeb,"ANIM_mciStatus // MCI_STATUS_MODE=%08lX !\n",
												lpParms->dwReturn);
			 	return 0;
			case MCI_STATUS_MEDIA_PRESENT:
				lpParms->dwReturn = TRUE;
				dprintf_mcianim(stddeb,"ANIM_mciStatus // MCI_STATUS_MEDIA_PRESENT !\n");
			 	return 0;
			case MCI_STATUS_NUMBER_OF_TRACKS:
				lpParms->dwReturn = 1;
				dprintf_mcianim(stddeb,"ANIM_mciStatus // MCI_STATUS_NUMBER_OF_TRACKS = %lu !\n",
											lpParms->dwReturn);
				if (lpParms->dwReturn == (WORD)-1) return MCIERR_INTERNAL;
			 	return 0;
			case MCI_STATUS_POSITION:
				lpParms->dwReturn = AnimDev[wDevID].dwCurFrame;
				if (dwFlags & MCI_STATUS_START) {
					lpParms->dwReturn = 0;
					dprintf_mcianim(stddeb,"CDAUDIO_mciStatus // get MCI_STATUS_START !\n");
					}
				if (dwFlags & MCI_TRACK) {
					if (lpParms->dwTrack > AnimDev[wDevID].nTracks)
						return MCIERR_OUTOFRANGE;
					lpParms->dwReturn = AnimDev[wDevID].lpdwTrackPos[lpParms->dwTrack - 1];
					dprintf_mcianim(stddeb,"ANIM_mciStatus // get MCI_TRACK #%lu !\n", lpParms->dwTrack);
					}
				lpParms->dwReturn = ANIM_CalcTime(wDevID, 
					AnimDev[wDevID].dwTimeFormat, lpParms->dwReturn);
					dprintf_mcianim(stddeb,"ANIM_mciStatus // MCI_STATUS_POSITION=%08lX !\n",
														lpParms->dwReturn);
			 	return 0;
			case MCI_STATUS_READY:
				dprintf_mcianim(stddeb,"ANIM_mciStatus // MCI_STATUS_READY !\n");
				lpParms->dwReturn = TRUE;
			 	return 0;
			case MCI_STATUS_TIME_FORMAT:
				dprintf_mcianim(stddeb,"ANIM_mciStatus // MCI_STATUS_TIME_FORMAT !\n");
				lpParms->dwReturn = MCI_FORMAT_MILLISECONDS;
			 	return 0;
			default:
				fprintf(stderr,"ANIM_mciStatus // unknown command %08lX !\n", lpParms->dwItem);
				return MCIERR_UNRECOGNIZED_COMMAND;
			}
		}
    fprintf(stderr,"ANIM_mciStatus // not MCI_STATUS_ITEM !\n");
 	return 0;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
* 				ANIM_CalcTime			[internal]
*/
DWORD ANIM_CalcTime(UINT wDevID, DWORD dwFormatType, DWORD dwFrame)
{
	DWORD	dwTime = 0;
#ifdef linux
	UINT	wTrack;
	UINT	wMinutes;
	UINT	wSeconds;
	UINT	wFrames;
	dprintf_mcianim(stddeb,"ANIM_CalcTime(%u, %08lX, %lu);\n", 
			wDevID, dwFormatType, dwFrame);
TryAGAIN:
	switch (dwFormatType) {
		case MCI_FORMAT_MILLISECONDS:
			dwTime = dwFrame / ANIMFRAMES_PERSEC * 1000;
			dprintf_mcianim(stddeb,
				"ANIM_CalcTime // MILLISECONDS %lu\n", dwTime);
			break;
		case MCI_FORMAT_MSF:
			wMinutes = dwFrame / ANIMFRAMES_PERMIN;
			wSeconds = (dwFrame - ANIMFRAMES_PERMIN * wMinutes) / ANIMFRAMES_PERSEC;
			wFrames = dwFrame - ANIMFRAMES_PERMIN * wMinutes - 
								ANIMFRAMES_PERSEC * wSeconds;
			dwTime = MCI_MAKE_MSF(wMinutes, wSeconds, wFrames);
			dprintf_mcianim(stddeb,"ANIM_CalcTime // MSF %02u:%02u:%02u -> dwTime=%lu\n",
								wMinutes, wSeconds, wFrames, dwTime);
			break;
		case MCI_FORMAT_TMSF:
			for (wTrack = 0; wTrack < AnimDev[wDevID].nTracks; wTrack++) {
/*				dwTime += AnimDev[wDevID].lpdwTrackLen[wTrack - 1];
				printf("Adding trk#%u curpos=%u \n", dwTime);
				if (dwTime >= dwFrame) break; */
				if (AnimDev[wDevID].lpdwTrackPos[wTrack - 1] >= dwFrame) break;
				}
			wMinutes = dwFrame / ANIMFRAMES_PERMIN;
			wSeconds = (dwFrame - ANIMFRAMES_PERMIN * wMinutes) / ANIMFRAMES_PERSEC;
			wFrames = dwFrame - ANIMFRAMES_PERMIN * wMinutes - 
								ANIMFRAMES_PERSEC * wSeconds;
			dwTime = MCI_MAKE_TMSF(wTrack, wMinutes, wSeconds, wFrames);
			dprintf_mcianim(stddeb,
				"ANIM_CalcTime // %02u-%02u:%02u:%02u\n",
				wTrack, wMinutes, wSeconds, wFrames);
			break;
		default:
			/* unknown format ! force TMSF ! ... */
			dwFormatType = MCI_FORMAT_TMSF;
			goto TryAGAIN;
		}
#endif
	return dwTime;
}


/**************************************************************************
* 				ANIM_CalcFrame			[internal]
*/
DWORD ANIM_CalcFrame(UINT wDevID, DWORD dwFormatType, DWORD dwTime)
{
	DWORD	dwFrame = 0;
#ifdef linux
	UINT	wTrack;
	dprintf_mcianim(stddeb,"ANIM_CalcFrame(%u, %08lX, %lu);\n", 
			wDevID, dwFormatType, dwTime);
TryAGAIN:
	switch (dwFormatType) {
		case MCI_FORMAT_MILLISECONDS:
			dwFrame = dwTime * ANIMFRAMES_PERSEC / 1000;
			dprintf_mcianim(stddeb,
				"ANIM_CalcFrame // MILLISECONDS %lu\n", dwFrame);
			break;
		case MCI_FORMAT_MSF:
			dprintf_mcianim(stddeb,
				"ANIM_CalcFrame // MSF %02u:%02u:%02u\n",
				MCI_MSF_MINUTE(dwTime), MCI_MSF_SECOND(dwTime), 
				MCI_MSF_FRAME(dwTime));
			dwFrame += ANIMFRAMES_PERMIN * MCI_MSF_MINUTE(dwTime);
			dwFrame += ANIMFRAMES_PERSEC * MCI_MSF_SECOND(dwTime);
			dwFrame += MCI_MSF_FRAME(dwTime);
			break;
		case MCI_FORMAT_TMSF:
			wTrack = MCI_TMSF_TRACK(dwTime);
			dprintf_mcianim(stddeb,
				"ANIM_CalcFrame // TMSF %02u-%02u:%02u:%02u\n",
				MCI_TMSF_TRACK(dwTime), MCI_TMSF_MINUTE(dwTime), 
				MCI_TMSF_SECOND(dwTime), MCI_TMSF_FRAME(dwTime));
			dprintf_mcianim(stddeb,
				"ANIM_CalcFrame // TMSF trackpos[%u]=%lu\n",
				wTrack, AnimDev[wDevID].lpdwTrackPos[wTrack - 1]);
			dwFrame = AnimDev[wDevID].lpdwTrackPos[wTrack - 1];
			dwFrame += ANIMFRAMES_PERMIN * MCI_TMSF_MINUTE(dwTime);
			dwFrame += ANIMFRAMES_PERSEC * MCI_TMSF_SECOND(dwTime);
			dwFrame += MCI_TMSF_FRAME(dwTime);
			break;
		default:
			/* unknown format ! force TMSF ! ... */
			dwFormatType = MCI_FORMAT_TMSF;
			goto TryAGAIN;
		}
#endif
	return dwFrame;
}



/**************************************************************************
* 				ANIM_mciPlay			[internal]
*/
DWORD ANIM_mciPlay(UINT wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
#ifdef linux
	int 	start, end;
	dprintf_mcianim(stddeb,"ANIM_mciPlay(%u, %08lX, %p);\n", 
		wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	start = 0; 		end = AnimDev[wDevID].dwTotalLen;
	AnimDev[wDevID].nCurTrack = 1;
	if (dwFlags & MCI_FROM) {
		start = ANIM_CalcFrame(wDevID, 
			AnimDev[wDevID].dwTimeFormat, lpParms->dwFrom); 
        dprintf_mcianim(stddeb,"ANIM_mciPlay // MCI_FROM=%08lX -> %u \n",
				lpParms->dwFrom, start);
		}
	if (dwFlags & MCI_TO) {
		end = ANIM_CalcFrame(wDevID, 
			AnimDev[wDevID].dwTimeFormat, lpParms->dwTo);
        	dprintf_mcianim(stddeb,
			"ANIM_mciPlay // MCI_TO=%08lX -> %u \n",
			lpParms->dwTo, end);
		}
	AnimDev[wDevID].mode = MCI_MODE_PLAY;
	if (dwFlags & MCI_NOTIFY) {
		dprintf_mcianim(stddeb,
			"ANIM_mciPlay // MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
			lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			AnimDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
	return 0;
#else
	return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
* 				ANIM_mciStop			[internal]
*/
DWORD ANIM_mciStop(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#ifdef linux
	dprintf_mcianim(stddeb,"ANIM_mciStop(%u, %08lX, %p);\n", 
			wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	AnimDev[wDevID].mode = MCI_MODE_STOP;
	if (dwFlags & MCI_NOTIFY) {
		dprintf_mcianim(stddeb,
			"ANIM_mciStop // MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
			lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			AnimDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
 	return 0;
#else
	return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
* 				ANIM_mciPause		[internal]
*/
DWORD ANIM_mciPause(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#ifdef linux
	dprintf_mcianim(stddeb,"ANIM_mciPause(%u, %08lX, %p);\n", 
		wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	AnimDev[wDevID].mode = MCI_MODE_PAUSE;
	if (dwFlags & MCI_NOTIFY) {
		dprintf_mcianim(stddeb,
			"ANIM_mciPause // MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
			lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			AnimDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
	return 0;
#else
	return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
* 				ANIM_mciResume		[internal]
*/
DWORD ANIM_mciResume(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#ifdef linux
	dprintf_mcianim(stddeb,"ANIM_mciResume(%u, %08lX, %p);\n", 
		wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	AnimDev[wDevID].mode = MCI_MODE_STOP;
	if (dwFlags & MCI_NOTIFY) {
		dprintf_mcianim(stddeb,
			"ANIM_mciResume // MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
			lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			AnimDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
	return 0;
#else
	return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
* 				ANIM_mciSeek			[internal]
*/
DWORD ANIM_mciSeek(UINT wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
#ifdef linux
	DWORD	dwRet;
	MCI_PLAY_PARMS PlayParms;
	dprintf_mcianim(stddeb,"ANIM_mciSeek(%u, %08lX, %p);\n", 
		wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	AnimDev[wDevID].mode = MCI_MODE_SEEK;
	switch(dwFlags) {
		case MCI_SEEK_TO_START:
			PlayParms.dwFrom = 0;
			break;
		case MCI_SEEK_TO_END:
			PlayParms.dwFrom = AnimDev[wDevID].dwTotalLen;
			break;
		case MCI_TO:
			PlayParms.dwFrom = lpParms->dwTo;
			break;
		}
	dwRet = ANIM_mciPlay(wDevID, MCI_WAIT | MCI_FROM, &PlayParms);
	if (dwRet != 0) return dwRet;
	dwRet = ANIM_mciStop(wDevID, MCI_WAIT, (LPMCI_GENERIC_PARMS)&PlayParms);
	if (dwFlags & MCI_NOTIFY) {
		dprintf_mcianim(stddeb,
			"ANIM_mciSeek // MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
			lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			AnimDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
	return dwRet;
#else
	return MCIERR_HARDWARE;
#endif
}


/**************************************************************************
* 				ANIM_mciSet			[internal]
*/
DWORD ANIM_mciSet(UINT wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
{
#ifdef linux
	dprintf_mcianim(stddeb,"ANIM_mciSet(%u, %08lX, %p);\n", 
		wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
/*
	printf("ANIM_mciSet // dwTimeFormat=%08lX\n", lpParms->dwTimeFormat);
	printf("ANIM_mciSet // dwAudio=%08lX\n", lpParms->dwAudio);
*/
	if (dwFlags & MCI_SET_TIME_FORMAT) {
		switch (lpParms->dwTimeFormat) {
			case MCI_FORMAT_MILLISECONDS:
				dprintf_mcianim(stddeb,
					"ANIM_mciSet // MCI_FORMAT_MILLISECONDS !\n");
				break;
			case MCI_FORMAT_MSF:
				dprintf_mcianim(stddeb,"ANIM_mciSet // MCI_FORMAT_MSF !\n");
				break;
			case MCI_FORMAT_TMSF:
				dprintf_mcianim(stddeb,"ANIM_mciSet // MCI_FORMAT_TMSF !\n");
				break;
			default:
				fprintf(stderr,"ANIM_mciSet // bad time format !\n");
				return MCIERR_BAD_TIME_FORMAT;
			}
		AnimDev[wDevID].dwTimeFormat = lpParms->dwTimeFormat;
		}
	if (dwFlags & MCI_SET_VIDEO) return MCIERR_UNSUPPORTED_FUNCTION;
	if (dwFlags & MCI_SET_ON) return MCIERR_UNSUPPORTED_FUNCTION;
	if (dwFlags & MCI_SET_OFF) return MCIERR_UNSUPPORTED_FUNCTION;
	if (dwFlags & MCI_NOTIFY) {
		dprintf_mcianim(stddeb,
			"ANIM_mciSet // MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
			lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			AnimDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
	return 0;
#else
	return MCIERR_HARDWARE;
#endif
}


/**************************************************************************
* 				ANIM_DriverProc		[sample driver]
*/
LRESULT ANIM_DriverProc(DWORD dwDevID, HDRVR hDriv, WORD wMsg, 
							DWORD dwParam1, DWORD dwParam2)
{
#ifdef linux
	switch(wMsg) {
		case DRV_LOAD:
			return (LRESULT)1L;
		case DRV_FREE:
			return (LRESULT)1L;
		case DRV_OPEN:
		case MCI_OPEN_DRIVER:
		case MCI_OPEN:
			return ANIM_mciOpen(dwParam1, (LPMCI_OPEN_PARMS)dwParam2); 
		case DRV_CLOSE:
		case MCI_CLOSE_DRIVER:
		case MCI_CLOSE:
			return ANIM_mciClose(dwDevID, dwParam1, 
					(LPMCI_GENERIC_PARMS)dwParam2);
		case DRV_ENABLE:
			return (LRESULT)1L;
		case DRV_DISABLE:
			return (LRESULT)1L;
		case DRV_QUERYCONFIGURE:
			return (LRESULT)1L;
		case DRV_CONFIGURE:
			MessageBox((HWND)NULL, "Sample MultiMedia Linux Driver !", 
								"MMLinux Driver", MB_OK);
			return (LRESULT)1L;
		case DRV_INSTALL:
			return (LRESULT)DRVCNF_RESTART;
		case DRV_REMOVE:
			return (LRESULT)DRVCNF_RESTART;
		case MCI_GETDEVCAPS:
			return ANIM_mciGetDevCaps(dwDevID, dwParam1, 
					(LPMCI_GETDEVCAPS_PARMS)dwParam2);
		case MCI_INFO:
			return ANIM_mciInfo(dwDevID, dwParam1, 
						(LPMCI_INFO_PARMS)dwParam2);
		case MCI_STATUS:
			return ANIM_mciStatus(dwDevID, dwParam1, 
						(LPMCI_STATUS_PARMS)dwParam2);
		case MCI_SET:
			return ANIM_mciSet(dwDevID, dwParam1, 
						(LPMCI_SET_PARMS)dwParam2);
		case MCI_PLAY:
			return ANIM_mciPlay(dwDevID, dwParam1, 
						(LPMCI_PLAY_PARMS)dwParam2);
		case MCI_STOP:
			return ANIM_mciStop(dwDevID, dwParam1, 
					(LPMCI_GENERIC_PARMS)dwParam2);
		case MCI_PAUSE:
			return ANIM_mciPause(dwDevID, dwParam1, 
					(LPMCI_GENERIC_PARMS)dwParam2);
		case MCI_RESUME:
			return ANIM_mciResume(dwDevID, dwParam1, 
					(LPMCI_GENERIC_PARMS)dwParam2);
		case MCI_SEEK:
			return ANIM_mciSeek(dwDevID, dwParam1, 
					(LPMCI_SEEK_PARMS)dwParam2);
		default:
			return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
		}
#else
	return MCIERR_HARDWARE;
#endif
}


/*-----------------------------------------------------------------------*/

#endif /* #ifdef BUILTIN_MMSYSTEM */
