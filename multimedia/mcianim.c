/*
 * Sample MCI ANIMATION Wine Driver for Linux
 *
 * Copyright 1994 Martin Ayotte
 */

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
#include "debug.h"

#define MAX_ANIMDRV 		2

#define ANIMFRAMES_PERSEC 	30
#define ANIMFRAMES_PERMIN 	1800
#define SECONDS_PERMIN	 	60

#if defined(linux) || defined(__FreeBSD__)
typedef struct {
    int     nUseCount;          /* Incremented for each shared open */
    BOOL16  fShareable;         /* TRUE if first open was shareable */
    WORD    wNotifyDeviceID;    /* MCI device ID with a pending notification */
    HANDLE16 hCallback;          /* Callback handle for pending notification */
	MCI_OPEN_PARMS openParms;
	DWORD	dwTimeFormat;
	int		mode;
	UINT16	nCurTrack;
	DWORD	dwCurFrame;
	UINT16	nTracks;
	DWORD	dwTotalLen;
	LPDWORD	lpdwTrackLen;
	LPDWORD	lpdwTrackPos;
	} LINUX_ANIM;

static LINUX_ANIM	AnimDev[MAX_ANIMDRV];
#endif


/*-----------------------------------------------------------------------*/

/**************************************************************************
* 				ANIM_mciOpen			[internal]
*/
static DWORD ANIM_mciOpen(UINT16 wDevID, DWORD dwFlags, LPMCI_OPEN_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
	LPSTR		lpstrElementName;
	char		str[128];

	dprintf_mcianim(stddeb,"ANIM_mciOpen(%04X, %08lX, %p);\n", 
					wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
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
	dprintf_mcianim(stddeb,"ANIM_mciOpen // wDevID=%04X\n", wDevID);
	lpParms->wDeviceID = wDevID;
	dprintf_mcianim(stddeb,"ANIM_mciOpen // lpParms->wDevID=%04X\n", lpParms->wDeviceID);
    if (dwFlags & MCI_OPEN_ELEMENT) {
		lpstrElementName = (LPSTR)PTR_SEG_TO_LIN(lpParms->lpstrElementName);
		dprintf_mcianim(stddeb,"ANIM_mciOpen // MCI_OPEN_ELEMENT '%s' !\n",
						lpstrElementName);
		if (strlen(lpstrElementName) > 0) {
			strcpy(str, lpstrElementName);
			CharUpper32A(str);
			}
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
		mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
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
static DWORD ANIM_mciClose(UINT16 wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
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
static DWORD ANIM_mciGetDevCaps(UINT16 wDevID, DWORD dwFlags, 
						LPMCI_GETDEVCAPS_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
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
* 				ANIM_CalcTime			[internal]
*/
static DWORD ANIM_CalcTime(UINT16 wDevID, DWORD dwFormatType, DWORD dwFrame)
{
	DWORD	dwTime = 0;
#if defined(linux) || defined(__FreeBSD__)
	UINT16	wTrack;
	UINT16	wMinutes;
	UINT16	wSeconds;
	UINT16	wFrames;
	dprintf_mcianim(stddeb,"ANIM_CalcTime(%u, %08lX, %lu);\n", 
			wDevID, dwFormatType, dwFrame);
    
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
		default:
			/* unknown format ! force TMSF ! ... */
			dwFormatType = MCI_FORMAT_TMSF;
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
		}
#endif
	return dwTime;
}


/**************************************************************************
* 				ANIM_CalcFrame			[internal]
*/
static DWORD ANIM_CalcFrame(UINT16 wDevID, DWORD dwFormatType, DWORD dwTime)
{
	DWORD	dwFrame = 0;
#if defined(linux) || defined(__FreeBSD__)
	UINT16	wTrack;
	dprintf_mcianim(stddeb,"ANIM_CalcFrame(%u, %08lX, %lu);\n", 
			wDevID, dwFormatType, dwTime);

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
		default:
			/* unknown format ! force TMSF ! ... */
			dwFormatType = MCI_FORMAT_TMSF;
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
		}
#endif
	return dwFrame;
}


/**************************************************************************
* 				ANIM_mciInfo			[internal]
*/
static DWORD ANIM_mciInfo(UINT16 wDevID, DWORD dwFlags, LPMCI_INFO_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
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
static DWORD ANIM_mciStatus(UINT16 wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
	dprintf_mcianim(stddeb,"ANIM_mciStatus(%u, %08lX, %p);\n", 
			wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	if (dwFlags & MCI_NOTIFY) {
		dprintf_mcianim(stddeb,
			"ANIM_mciStatus // MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
			lpParms->dwCallback);
		mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
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
* 				ANIM_mciPlay			[internal]
*/
static DWORD ANIM_mciPlay(UINT16 wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
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
		mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
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
static DWORD ANIM_mciStop(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
	dprintf_mcianim(stddeb,"ANIM_mciStop(%u, %08lX, %p);\n", 
			wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	AnimDev[wDevID].mode = MCI_MODE_STOP;
	if (dwFlags & MCI_NOTIFY) {
		dprintf_mcianim(stddeb,
			"ANIM_mciStop // MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
			lpParms->dwCallback);
		mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
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
static DWORD ANIM_mciPause(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
	dprintf_mcianim(stddeb,"ANIM_mciPause(%u, %08lX, %p);\n", 
		wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	AnimDev[wDevID].mode = MCI_MODE_PAUSE;
	if (dwFlags & MCI_NOTIFY) {
		dprintf_mcianim(stddeb,
			"ANIM_mciPause // MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
			lpParms->dwCallback);
		mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
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
static DWORD ANIM_mciResume(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
	dprintf_mcianim(stddeb,"ANIM_mciResume(%u, %08lX, %p);\n", 
		wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	AnimDev[wDevID].mode = MCI_MODE_STOP;
	if (dwFlags & MCI_NOTIFY) {
		dprintf_mcianim(stddeb,
			"ANIM_mciResume // MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
			lpParms->dwCallback);
		mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
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
static DWORD ANIM_mciSeek(UINT16 wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
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
		mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
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
static DWORD ANIM_mciSet(UINT16 wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
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
		mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
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
LONG ANIM_DriverProc(DWORD dwDevID, HDRVR16 hDriv, WORD wMsg, 
							DWORD dwParam1, DWORD dwParam2)
{
#if defined(linux) || defined(__FreeBSD__)
	switch(wMsg) {
		case DRV_LOAD:
			return 1;
		case DRV_FREE:
			return 1;
		case DRV_OPEN:
		case MCI_OPEN_DRIVER:
		case MCI_OPEN:
			return ANIM_mciOpen(dwDevID, dwParam1, 
					(LPMCI_OPEN_PARMS)PTR_SEG_TO_LIN(dwParam2)); 
		case DRV_CLOSE:
		case MCI_CLOSE_DRIVER:
		case MCI_CLOSE:
			return ANIM_mciClose(dwDevID, dwParam1, 
					(LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
		case DRV_ENABLE:
			return 1;
		case DRV_DISABLE:
			return 1;
		case DRV_QUERYCONFIGURE:
			return 1;
		case DRV_CONFIGURE:
			MessageBox16(0, "Sample MultiMedia Linux Driver !", 
								"MMLinux Driver", MB_OK);
			return 1;
		case DRV_INSTALL:
			return DRVCNF_RESTART;
		case DRV_REMOVE:
			return DRVCNF_RESTART;
		case MCI_GETDEVCAPS:
			return ANIM_mciGetDevCaps(dwDevID, dwParam1, 
					(LPMCI_GETDEVCAPS_PARMS)PTR_SEG_TO_LIN(dwParam2));
		case MCI_INFO:
			return ANIM_mciInfo(dwDevID, dwParam1, 
						(LPMCI_INFO_PARMS)PTR_SEG_TO_LIN(dwParam2));
		case MCI_STATUS:
			return ANIM_mciStatus(dwDevID, dwParam1, 
						(LPMCI_STATUS_PARMS)PTR_SEG_TO_LIN(dwParam2));
		case MCI_SET:
			return ANIM_mciSet(dwDevID, dwParam1, 
						(LPMCI_SET_PARMS)PTR_SEG_TO_LIN(dwParam2));
		case MCI_PLAY:
			return ANIM_mciPlay(dwDevID, dwParam1, 
						(LPMCI_PLAY_PARMS)PTR_SEG_TO_LIN(dwParam2));
		case MCI_STOP:
			return ANIM_mciStop(dwDevID, dwParam1, 
					(LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
		case MCI_PAUSE:
			return ANIM_mciPause(dwDevID, dwParam1, 
					(LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
		case MCI_RESUME:
			return ANIM_mciResume(dwDevID, dwParam1, 
					(LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
		case MCI_SEEK:
			return ANIM_mciSeek(dwDevID, dwParam1, 
					(LPMCI_SEEK_PARMS)PTR_SEG_TO_LIN(dwParam2));
		default:
			return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
		}
#else
	return MCIERR_HARDWARE;
#endif
}


/*-----------------------------------------------------------------------*/
