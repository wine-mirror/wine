/*
 * Sample MCI CDAUDIO Wine Driver for Linux
 *
 * Copyright 1994 Martin Ayotte
 */
static char Copyright[] = "Copyright  Martin Ayotte, 1994";

#ifndef WINELIB
#define BUILTIN_MMSYSTEM
#endif 

#ifdef BUILTIN_MMSYSTEM

/*
#define DEBUG_CDAUDIO
*/

#include "stdio.h"
#include "win.h"
#include "user.h"
#include "driver.h"
#include "mmsystem.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#ifdef linux
#include <linux/soundcard.h>
#include <linux/cdrom.h>
#endif

#define SOUND_DEV "/dev/dsp"
#define CDAUDIO_DEV "/dev/sbpcd"

#ifdef SOUND_VERSION
#define IOCTL(a,b,c)		ioctl(a,b,&c)
#else
#define IOCTL(a,b,c)		(c = ioctl(a,b,c) )
#endif

#define MAX_CDAUDIODRV 		2
#define MAX_CDAUDIO_TRACKS 	256

#define CDFRAMES_PERSEC 	75
#define CDFRAMES_PERMIN 	4500
#define SECONDS_PERMIN	 	60

#ifdef linux
typedef struct {
    int     nUseCount;          /* Incremented for each shared open */
    BOOL    fShareable;         /* TRUE if first open was shareable */
    WORD    wNotifyDeviceID;    /* MCI device ID with a pending notification */
    HANDLE  hCallback;          /* Callback handle for pending notification */
	MCI_OPEN_PARMS openParms;
	DWORD	dwTimeFormat;
	int		unixdev;
	struct cdrom_subchnl	sc;
	int		mode;
	UINT	nCurTrack;
	DWORD	dwCurFrame;
	UINT	nTracks;
	DWORD	dwTotalLen;
	LPDWORD	lpdwTrackLen;
	LPDWORD	lpdwTrackPos;
	DWORD	dwFirstOffset;
	} LINUX_CDAUDIO;

static LINUX_CDAUDIO	CDADev[MAX_CDAUDIODRV];
#endif

UINT CDAUDIO_GetNumberOfTracks(UINT wDevID);
BOOL CDAUDIO_GetTracksInfo(UINT wDevID);
BOOL CDAUDIO_GetCDStatus(UINT wDevID);
DWORD CDAUDIO_CalcTime(UINT wDevID, DWORD dwFormatType, DWORD dwFrame);


/*-----------------------------------------------------------------------*/


/**************************************************************************
* 				CDAUDIO_mciOpen			[internal]
*/
DWORD CDAUDIO_mciOpen(DWORD dwFlags, LPMCI_OPEN_PARMS lpParms)
{
#ifdef linux
	UINT	wDevID;
	int		cdrom;
#ifdef DEBUG_CDAUDIO
	printf("CDAUDIO_mciOpen(%08X, %08X);\n", dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	wDevID = lpParms->wDeviceID;
	if (CDADev[wDevID].nUseCount > 0) {
		/* The driver already open on this channel */
		/* If the driver was% op, ened shareable before and this open specifies */
		/* shareable then increment the use count */
		if (CDADev[wDevID].fShareable && (dwFlags & MCI_OPEN_SHAREABLE))
			++CDADev[wDevID].nUseCount;
		else
			return MCIERR_MUST_USE_SHAREABLE;
		}
	else {
		CDADev[wDevID].nUseCount = 1;
		CDADev[wDevID].fShareable = dwFlags & MCI_OPEN_SHAREABLE;
		}
    if (dwFlags & MCI_OPEN_ELEMENT) {
		printf("CDAUDIO_mciOpen // MCI_OPEN_ELEMENT !\n");
/*		return MCIERR_NO_ELEMENT_ALLOWED; */
		}
	memcpy(&CDADev[wDevID].openParms, lpParms, sizeof(MCI_OPEN_PARMS));
	CDADev[wDevID].wNotifyDeviceID = lpParms->wDeviceID;
	CDADev[wDevID].unixdev = open (CDAUDIO_DEV, O_RDONLY, 0);
	if (CDADev[wDevID].unixdev == -1) {
		printf("CDAUDIO_mciOpen // can't open '%s' !\n", CDAUDIO_DEV);
		return MCIERR_HARDWARE;
		}
	CDADev[wDevID].mode = 0;
	CDADev[wDevID].dwTimeFormat = MCI_FORMAT_TMSF;
	CDADev[wDevID].nCurTrack = 0;
	CDADev[wDevID].nTracks = 0;
	CDADev[wDevID].dwTotalLen = 0;
	CDADev[wDevID].dwFirstOffset = 0;
	CDADev[wDevID].lpdwTrackLen = NULL;
	CDADev[wDevID].lpdwTrackPos = NULL;
	if (!CDAUDIO_GetTracksInfo(wDevID)) {
		printf("CDAUDIO_mciOpen // error reading TracksInfo !\n");
/*		return MCIERR_INTERNAL; */
		}
	if (dwFlags & MCI_NOTIFY) {
		printf("CDAUDIO_mciOpen // MCI_NOTIFY_SUCCESSFUL %08X !\n", lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			CDADev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
 	return 0;
#else
	return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
* 				CDAUDIO_mciClose		[internal]
*/
DWORD CDAUDIO_mciClose(UINT wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_CDAUDIO
	printf("CDAUDIO_mciClose(%u, %08X, %08X);\n", wDevID, dwParam, lpParms);
#endif
	if (CDADev[wDevID].lpdwTrackLen != NULL) free(CDADev[wDevID].lpdwTrackLen);
	if (CDADev[wDevID].lpdwTrackPos != NULL) free(CDADev[wDevID].lpdwTrackPos);
	close(CDADev[wDevID].unixdev);
#endif
}

/**************************************************************************
* 				CDAUDIO_mciGetDevCaps	[internal]
*/
DWORD CDAUDIO_mciGetDevCaps(UINT wDevID, DWORD dwFlags, 
						LPMCI_GETDEVCAPS_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_CDAUDIO
	printf("CDAUDIO_mciGetDevCaps(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	if (dwFlags & MCI_GETDEVCAPS_ITEM) {
		printf("CDAUDIO_mciGetDevCaps // MCI_GETDEVCAPS_ITEM dwItem=%08X;\n", 
				lpParms->dwItem);
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
	printf("CDAUDIO_mciGetDevCaps // lpParms->dwReturn=%08X;\n", lpParms->dwReturn);
 	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}

/**************************************************************************
* 				CDAUDIO_mciInfo			[internal]
*/
DWORD CDAUDIO_mciInfo(UINT wDevID, DWORD dwFlags, LPMCI_INFO_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_CDAUDIO
	printf("CDAUDIO_mciInfo(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	lpParms->lpstrReturn = NULL;
	switch(dwFlags) {
		case MCI_INFO_PRODUCT:
			lpParms->lpstrReturn = "Linux CDROM 0.5";
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
* 				CDAUDIO_mciStatus		[internal]
*/
DWORD CDAUDIO_mciStatus(UINT wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_CDAUDIO
	printf("CDAUDIO_mciStatus(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	if (CDADev[wDevID].unixdev == 0) return MMSYSERR_NOTENABLED;
	if (dwFlags & MCI_NOTIFY) {
#ifdef DEBUG_CDAUDIO
		printf("CDAUDIO_mciStatus // MCI_NOTIFY_SUCCESSFUL %08X !\n", lpParms->dwCallback);
#endif
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			CDADev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
	if (dwFlags & MCI_STATUS_ITEM) {
		switch(lpParms->dwItem) {
			case MCI_STATUS_CURRENT_TRACK:
				if (!CDAUDIO_GetCDStatus(wDevID)) return MCIERR_INTERNAL;
				lpParms->dwReturn = CDADev[wDevID].nCurTrack;
#ifdef DEBUG_CDAUDIO
				printf("CDAUDIO_mciStatus // CURRENT_TRACK=%u!\n", lpParms->dwReturn);
#endif
			 	return 0;
			case MCI_STATUS_LENGTH:
				if (CDADev[wDevID].nTracks == 0) {
					if (!CDAUDIO_GetTracksInfo(wDevID)) {
						printf("CDAUDIO_mciStatus // error reading TracksInfo !\n");
						return MCIERR_INTERNAL;
						}
					}
				if (dwFlags & MCI_TRACK) {
					printf("CDAUDIO_mciStatus // MCI_TRACK #%u LENGTH=??? !\n", 
														lpParms->dwTrack);
					if (lpParms->dwTrack > CDADev[wDevID].nTracks)
						return MCIERR_OUTOFRANGE;
					lpParms->dwReturn = CDADev[wDevID].lpdwTrackLen[lpParms->dwTrack];
					}
				else
					lpParms->dwReturn = CDADev[wDevID].dwTotalLen;
				lpParms->dwReturn = CDAUDIO_CalcTime(wDevID, 
					CDADev[wDevID].dwTimeFormat, lpParms->dwReturn);
				printf("CDAUDIO_mciStatus // LENGTH=%u !\n", lpParms->dwReturn);
			 	return 0;
			case MCI_STATUS_MODE:
				if (!CDAUDIO_GetCDStatus(wDevID)) return MCIERR_INTERNAL;
				lpParms->dwReturn = CDADev[wDevID].mode;
#ifdef DEBUG_CDAUDIO
				printf("CDAUDIO_mciStatus // MCI_STATUS_MODE=%08X !\n", 
												lpParms->dwReturn);
#endif
			 	return 0;
			case MCI_STATUS_MEDIA_PRESENT:
				lpParms->dwReturn = (CDADev[wDevID].nTracks > 0) ? TRUE : FALSE;
				if (lpParms->dwReturn == FALSE)
					printf("CDAUDIO_mciStatus // MEDIA_NOT_PRESENT !\n");
				else
					printf("CDAUDIO_mciStatus // MCI_STATUS_MEDIA_PRESENT !\n");
			 	return 0;
			case MCI_STATUS_NUMBER_OF_TRACKS:
				lpParms->dwReturn = CDAUDIO_GetNumberOfTracks(wDevID);
				printf("CDAUDIO_mciStatus // MCI_STATUS_NUMBER_OF_TRACKS = %u !\n",
													lpParms->dwReturn);
				if (lpParms->dwReturn == (WORD)-1) return MCIERR_INTERNAL;
			 	return 0;
			case MCI_STATUS_POSITION:
				if (!CDAUDIO_GetCDStatus(wDevID)) return MCIERR_INTERNAL;
				lpParms->dwReturn = CDADev[wDevID].dwCurFrame;
				if (dwFlags & MCI_STATUS_START) {
					lpParms->dwReturn = CDADev[wDevID].dwFirstOffset;
#ifdef DEBUG_CDAUDIO
					printf("CDAUDIO_mciStatus // get MCI_STATUS_START !\n");
#endif
					}
				if (dwFlags & MCI_TRACK) {
					if (lpParms->dwTrack > CDADev[wDevID].nTracks)
						return MCIERR_OUTOFRANGE;
					lpParms->dwReturn = CDADev[wDevID].lpdwTrackPos[lpParms->dwTrack - 1];
#ifdef DEBUG_CDAUDIO
					printf("CDAUDIO_mciStatus // get MCI_TRACK #%u !\n", lpParms->dwTrack);
#endif
					}
				lpParms->dwReturn = CDAUDIO_CalcTime(wDevID, 
					CDADev[wDevID].dwTimeFormat, lpParms->dwReturn);
#ifdef DEBUG_CDAUDIO
				printf("CDAUDIO_mciStatus // MCI_STATUS_POSITION=%08X !\n",
														lpParms->dwReturn);
#endif
			 	return 0;
			case MCI_STATUS_READY:
				printf("CDAUDIO_mciStatus // MCI_STATUS_READY !\n");
				lpParms->dwReturn = TRUE;
			 	return 0;
			case MCI_STATUS_TIME_FORMAT:
				printf("CDAUDIO_mciStatus // MCI_STATUS_TIME_FORMAT !\n");
				lpParms->dwReturn = MCI_FORMAT_MILLISECONDS;
			 	return 0;
			default:
				printf("CDAUDIO_mciStatus // unknowm command %04X !\n", lpParms->dwItem);
				return MCIERR_UNRECOGNIZED_COMMAND;
			}
		}
	printf("CDAUDIO_mciStatus // not MCI_STATUS_ITEM !\n");
 	return 0;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
* 				CDAUDIO_CalcTime			[internal]
*/
DWORD CDAUDIO_CalcTime(UINT wDevID, DWORD dwFormatType, DWORD dwFrame)
{
	DWORD	dwTime = 0;
#ifdef linux
	UINT	wTrack;
	UINT	wMinutes;
	UINT	wSeconds;
	UINT	wFrames;
#ifdef DEBUG_CDAUDIO
	printf("CDAUDIO_CalcTime(%u, %08X, %lu);\n", wDevID, dwFormatType, dwFrame);
#endif
TryAGAIN:
	switch (dwFormatType) {
		case MCI_FORMAT_MILLISECONDS:
			dwTime = dwFrame / CDFRAMES_PERSEC * 1000;
#ifdef DEBUG_CDAUDIO
			printf("CDAUDIO_CalcTime // MILLISECONDS %u\n", dwTime);
#endif
			break;
		case MCI_FORMAT_MSF:
			wMinutes = dwFrame / CDFRAMES_PERMIN;
			wSeconds = (dwFrame - CDFRAMES_PERMIN * wMinutes) / CDFRAMES_PERSEC;
			wFrames = dwFrame - CDFRAMES_PERMIN * wMinutes - 
								CDFRAMES_PERSEC * wSeconds;
			dwTime = MCI_MAKE_MSF(wMinutes, wSeconds, wFrames);
#ifdef DEBUG_CDAUDIO
			printf("CDAUDIO_CalcTime // MSF %02u:%02u:%02u -> dwTime=%u\n", 
								wMinutes, wSeconds, wFrames, dwTime);
#endif
			break;
		case MCI_FORMAT_TMSF:
			for (wTrack = 0; wTrack < CDADev[wDevID].nTracks; wTrack++) {
/*				dwTime += CDADev[wDevID].lpdwTrackLen[wTrack - 1];
				printf("Adding trk#%u curpos=%u \n", dwTime);
				if (dwTime >= dwFrame) break; */
				if (CDADev[wDevID].lpdwTrackPos[wTrack - 1] >= dwFrame) break;
				}
			wMinutes = dwFrame / CDFRAMES_PERMIN;
			wSeconds = (dwFrame - CDFRAMES_PERMIN * wMinutes) / CDFRAMES_PERSEC;
			wFrames = dwFrame - CDFRAMES_PERMIN * wMinutes - 
								CDFRAMES_PERSEC * wSeconds;
			dwTime = MCI_MAKE_TMSF(wTrack, wMinutes, wSeconds, wFrames);
#ifdef DEBUG_CDAUDIO
			printf("CDAUDIO_CalcTime // %02u-%02u:%02u:%02u\n", 
					wTrack, wMinutes, wSeconds, wFrames);
#endif
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
* 				CDAUDIO_CalcFrame			[internal]
*/
DWORD CDAUDIO_CalcFrame(UINT wDevID, DWORD dwFormatType, DWORD dwTime)
{
	DWORD	dwFrame = 0;
#ifdef linux
	UINT	wTrack;
#ifdef DEBUG_CDAUDIO
	printf("CDAUDIO_CalcFrame(%u, %08X, %lu);\n", wDevID, dwFormatType, dwTime);
#endif
TryAGAIN:
	switch (dwFormatType) {
		case MCI_FORMAT_MILLISECONDS:
			dwFrame = dwTime * CDFRAMES_PERSEC / 1000;
#ifdef DEBUG_CDAUDIO
			printf("CDAUDIO_CalcFrame // MILLISECONDS %u\n", dwFrame);
#endif
			break;
		case MCI_FORMAT_MSF:
#ifdef DEBUG_CDAUDIO
			printf("CDAUDIO_CalcFrame // MSF %02u:%02u:%02u\n", 
				MCI_MSF_MINUTE(dwTime), MCI_MSF_SECOND(dwTime), 
				MCI_MSF_FRAME(dwTime));
#endif
			dwFrame += CDFRAMES_PERMIN * MCI_MSF_MINUTE(dwTime);
			dwFrame += CDFRAMES_PERSEC * MCI_MSF_SECOND(dwTime);
			dwFrame += MCI_MSF_FRAME(dwTime);
			break;
		case MCI_FORMAT_TMSF:
			wTrack = MCI_TMSF_TRACK(dwTime);
#ifdef DEBUG_CDAUDIO
			printf("CDAUDIO_CalcFrame // TMSF %02u-%02u:%02u:%02u\n", 
					MCI_TMSF_TRACK(dwTime), MCI_TMSF_MINUTE(dwTime), 
					MCI_TMSF_SECOND(dwTime), MCI_TMSF_FRAME(dwTime));
			printf("CDAUDIO_CalcFrame // TMSF trackpos[%u]=%u\n",
				wTrack, CDADev[wDevID].lpdwTrackPos[wTrack - 1]);
#endif
			dwFrame = CDADev[wDevID].lpdwTrackPos[wTrack - 1];
			dwFrame += CDFRAMES_PERMIN * MCI_TMSF_MINUTE(dwTime);
			dwFrame += CDFRAMES_PERSEC * MCI_TMSF_SECOND(dwTime);
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
* 				CDAUDIO_GetNumberOfTracks		[internal]
*/
UINT CDAUDIO_GetNumberOfTracks(UINT wDevID)
{
#ifdef linux
	struct cdrom_tochdr	hdr;
	if (CDADev[wDevID].nTracks == 0) {
		if (ioctl(CDADev[wDevID].unixdev, CDROMREADTOCHDR, &hdr)) {
			printf("GetNumberOfTracks(%u) // Error occured !\n", wDevID);
			return (WORD)-1;
			}
		CDADev[wDevID].nTracks = hdr.cdth_trk1;
		}
	return CDADev[wDevID].nTracks;
#else
	return (WORD)-1;
#endif
}

/**************************************************************************
* 				CDAUDIO_GetNumberOfTracks		[internal]
*/
BOOL CDAUDIO_GetTracksInfo(UINT wDevID)
{
#ifdef linux
	int		i, length;
	int		start, last_start;
	int		total_length = 0;
	struct cdrom_tocentry	entry;
	if (CDADev[wDevID].nTracks == 0) {
		if (CDAUDIO_GetNumberOfTracks(wDevID) == (WORD)-1) return FALSE;
		}
	printf("CDAUDIO_GetTracksInfo // nTracks=%u\n", CDADev[wDevID].nTracks);
	if (CDADev[wDevID].lpdwTrackLen != NULL) 
		free(CDADev[wDevID].lpdwTrackLen);
	CDADev[wDevID].lpdwTrackLen = (LPDWORD)malloc(
		(CDADev[wDevID].nTracks + 1) * sizeof(DWORD));
	if (CDADev[wDevID].lpdwTrackPos != NULL) 
		free(CDADev[wDevID].lpdwTrackPos);
	CDADev[wDevID].lpdwTrackPos = (LPDWORD)malloc(
		(CDADev[wDevID].nTracks + 1) * sizeof(DWORD));
	if (CDADev[wDevID].lpdwTrackLen == NULL ||
		CDADev[wDevID].lpdwTrackPos == NULL) {
		printf("CDAUDIO_GetTracksInfo // error allocating track table !\n");
		return FALSE;
		}
	memset(CDADev[wDevID].lpdwTrackLen, 0, 
		(CDADev[wDevID].nTracks + 1) * sizeof(DWORD));
	memset(CDADev[wDevID].lpdwTrackPos, 0, 
		(CDADev[wDevID].nTracks + 1) * sizeof(DWORD));
	for (i = 0; i <= CDADev[wDevID].nTracks; i++) {
		if (i == CDADev[wDevID].nTracks)
			entry.cdte_track = CDROM_LEADOUT;
		else
			entry.cdte_track = i + 1;
		entry.cdte_format = CDROM_MSF;
		if (ioctl(CDADev[wDevID].unixdev, CDROMREADTOCENTRY, &entry)) {
			printf("CDAUDIO_GetTracksInfo // error read entry\n");
			return FALSE;
			}
		start = CDFRAMES_PERSEC * (SECONDS_PERMIN * 
			entry.cdte_addr.msf.minute + entry.cdte_addr.msf.second) + 
			entry.cdte_addr.msf.frame;
		if (i == 0) {
			CDADev[wDevID].dwFirstOffset = last_start = start;
			printf("CDAUDIO_GetTracksInfo // dwFirstOffset=%u\n", start);
			}
		else {
			length = start - last_start;
			last_start = start;
			start = last_start - length;
			total_length += length;
			CDADev[wDevID].lpdwTrackLen[i - 1] = length;
			CDADev[wDevID].lpdwTrackPos[i - 1] = start;
			printf("CDAUDIO_GetTracksInfo // track #%u start=%u len=%u\n", 
													i, start, length);
			}
		}
	CDADev[wDevID].dwTotalLen = total_length;
	printf("CDAUDIO_GetTracksInfo // total_len=%u\n", total_length);
	fflush(stdout);
	return TRUE;
#else
	return FALSE;
#endif
}


/**************************************************************************
* 				CDAUDIO_GetNumberOfTracks		[internal]
*/
BOOL CDAUDIO_GetCDStatus(UINT wDevID)
{
#ifdef linux
	int		oldmode = CDADev[wDevID].mode;
	CDADev[wDevID].sc.cdsc_format = CDROM_MSF;
	if (ioctl(CDADev[wDevID].unixdev, CDROMSUBCHNL, &CDADev[wDevID].sc)) {
#ifdef DEBUG_CDAUDIO
		printf("CDAUDIO_GetCDStatus // opened or no_media !\n");
#endif
		CDADev[wDevID].mode = MCI_MODE_NOT_READY;
		return TRUE;
		}
	switch (CDADev[wDevID].sc.cdsc_audiostatus) {
		case CDROM_AUDIO_INVALID:
			printf("CDAUDIO_GetCDStatus // device doesn't support status !\n");
			return FALSE;
		case CDROM_AUDIO_NO_STATUS: 
			CDADev[wDevID].mode = MCI_MODE_STOP;
#ifdef DEBUG_CDAUDIO
			printf("CDAUDIO_GetCDStatus // MCI_MODE_STOP !\n");
#endif
			break;
		case CDROM_AUDIO_PLAY: 
			CDADev[wDevID].mode = MCI_MODE_PLAY;
#ifdef DEBUG_CDAUDIO
			printf("CDAUDIO_GetCDStatus // MCI_MODE_PLAY !\n");
#endif
			break;
		case CDROM_AUDIO_PAUSED:
			CDADev[wDevID].mode = MCI_MODE_PAUSE;
#ifdef DEBUG_CDAUDIO
			printf("CDAUDIO_GetCDStatus // MCI_MODE_PAUSE !\n");
#endif
			break;
		default:
			printf("CDAUDIO_GetCDStatus // status=%02X !\n", 
					CDADev[wDevID].sc.cdsc_audiostatus);
		}
	CDADev[wDevID].nCurTrack = CDADev[wDevID].sc.cdsc_trk;
	CDADev[wDevID].dwCurFrame = 
		CDFRAMES_PERMIN * CDADev[wDevID].sc.cdsc_absaddr.msf.minute +
		CDFRAMES_PERSEC * CDADev[wDevID].sc.cdsc_absaddr.msf.second +
		CDADev[wDevID].sc.cdsc_absaddr.msf.frame;
#ifdef DEBUG_CDAUDIO
	printf("CDAUDIO_GetCDStatus // %02u-%02u:%02u:%02u \n", 
		CDADev[wDevID].sc.cdsc_trk,
		CDADev[wDevID].sc.cdsc_absaddr.msf.minute,
		CDADev[wDevID].sc.cdsc_absaddr.msf.second,
		CDADev[wDevID].sc.cdsc_absaddr.msf.frame);
#endif
	if (oldmode != CDADev[wDevID].mode && oldmode == MCI_MODE_OPEN) {
		if (!CDAUDIO_GetTracksInfo(wDevID)) {
			printf("CDAUDIO_GetCDStatus // error updating TracksInfo !\n");
			return MCIERR_INTERNAL;
			}
		}
	return TRUE;
#else
	return FALSE;
#endif
}

/**************************************************************************
* 				CDAUDIO_mciPlay			[internal]
*/
DWORD CDAUDIO_mciPlay(UINT wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
#ifdef linux
	int 	start, end;
	struct 	cdrom_msf	msf;
#ifdef DEBUG_CDAUDIO
	printf("CDAUDIO_mciPlay(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	if (CDADev[wDevID].unixdev == 0) return MMSYSERR_NOTENABLED;
	start = 0; 		end = CDADev[wDevID].dwTotalLen;
	CDADev[wDevID].nCurTrack = 1;
	if (dwFlags & MCI_FROM) {
		start = CDAUDIO_CalcFrame(wDevID, 
			CDADev[wDevID].dwTimeFormat, lpParms->dwFrom); 
#ifdef DEBUG_CDAUDIO
		printf("CDAUDIO_mciPlay // MCI_FROM=%08X -> %u \n", 
								lpParms->dwFrom, start);
#endif
		}
	if (dwFlags & MCI_TO) {
		end = CDAUDIO_CalcFrame(wDevID, 
			CDADev[wDevID].dwTimeFormat, lpParms->dwTo);
#ifdef DEBUG_CDAUDIO
		printf("CDAUDIO_mciPlay // MCI_TO=%08X -> %u \n", 
								lpParms->dwTo, end);
#endif
		}
	start += CDADev[wDevID].dwFirstOffset;	
	end += CDADev[wDevID].dwFirstOffset;
	msf.cdmsf_min0 = start / CDFRAMES_PERMIN;
	msf.cdmsf_sec0 = (start % CDFRAMES_PERMIN) / CDFRAMES_PERSEC;
	msf.cdmsf_frame0 = start % CDFRAMES_PERSEC;
	msf.cdmsf_min1 = end / CDFRAMES_PERMIN;
	msf.cdmsf_sec1 = (end % CDFRAMES_PERMIN) / CDFRAMES_PERSEC;
	msf.cdmsf_frame1 = end % CDFRAMES_PERSEC;
	if (ioctl(CDADev[wDevID].unixdev, CDROMSTART)) {
		printf("CDAUDIO_mciPlay // motor doesn't start !\n");
		return MCIERR_HARDWARE;
		}
	if (ioctl(CDADev[wDevID].unixdev, CDROMPLAYMSF, &msf)) {
		printf("CDAUDIO_mciPlay // device doesn't play !\n");
		return MCIERR_HARDWARE;
		}
#ifdef DEBUG_CDAUDIO
	printf("CDAUDIO_mciPlay // msf = %d:%d:%d %d:%d:%d\n",
		msf.cdmsf_min0, msf.cdmsf_sec0, msf.cdmsf_frame0,
		msf.cdmsf_min1, msf.cdmsf_sec1, msf.cdmsf_frame1);
#endif
	CDADev[wDevID].mode = MCI_MODE_PLAY;
	if (dwFlags & MCI_NOTIFY) {
		printf("CDAUDIO_mciPlay // MCI_NOTIFY_SUCCESSFUL %08X !\n", lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			CDADev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
	return 0;
#else
	return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
* 				CDAUDIO_mciStop			[internal]
*/
DWORD CDAUDIO_mciStop(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_CDAUDIO
	printf("CDAUDIO_mciStop(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	if (ioctl(CDADev[wDevID].unixdev, CDROMSTOP)) return MCIERR_HARDWARE;
	CDADev[wDevID].mode = MCI_MODE_STOP;
	if (dwFlags & MCI_NOTIFY) {
		printf("CDAUDIO_mciStop // MCI_NOTIFY_SUCCESSFUL %08X !\n", lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			CDADev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
 	return 0;
#else
	return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
* 				CDAUDIO_mciPause		[internal]
*/
DWORD CDAUDIO_mciPause(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_CDAUDIO
	printf("CDAUDIO_mciPause(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	if (ioctl(CDADev[wDevID].unixdev, CDROMPAUSE)) return MCIERR_HARDWARE;
	CDADev[wDevID].mode = MCI_MODE_PAUSE;
	if (dwFlags & MCI_NOTIFY) {
		printf("CDAUDIO_mciPause // MCI_NOTIFY_SUCCESSFUL %08X !\n", lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			CDADev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
	return 0;
#else
	return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
* 				CDAUDIO_mciResume		[internal]
*/
DWORD CDAUDIO_mciResume(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_CDAUDIO
	printf("CDAUDIO_mciResume(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	if (ioctl(CDADev[wDevID].unixdev, CDROMRESUME)) return MCIERR_HARDWARE;
	CDADev[wDevID].mode = MCI_MODE_STOP;
	if (dwFlags & MCI_NOTIFY) {
		printf("CDAUDIO_mciResume // MCI_NOTIFY_SUCCESSFUL %08X !\n", lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			CDADev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
	return 0;
#else
	return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
* 				CDAUDIO_mciSeek			[internal]
*/
DWORD CDAUDIO_mciSeek(UINT wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
#ifdef linux
	DWORD	dwRet;
	MCI_PLAY_PARMS PlayParms;
#ifdef DEBUG_CDAUDIO
	printf("CDAUDIO_mciSeek(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	if (ioctl(CDADev[wDevID].unixdev, CDROMRESUME)) return MCIERR_HARDWARE;
	CDADev[wDevID].mode = MCI_MODE_SEEK;
	switch(dwFlags) {
		case MCI_SEEK_TO_START:
			PlayParms.dwFrom = 0;
			break;
		case MCI_SEEK_TO_END:
			PlayParms.dwFrom = CDADev[wDevID].dwTotalLen;
			break;
		case MCI_TO:
			PlayParms.dwFrom = lpParms->dwTo;
			break;
		}
	dwRet = CDAUDIO_mciPlay(wDevID, MCI_WAIT | MCI_FROM, &PlayParms);
	if (dwRet != 0) return dwRet;
	dwRet = CDAUDIO_mciStop(wDevID, MCI_WAIT, (LPMCI_GENERIC_PARMS)&PlayParms);
	if (dwFlags & MCI_NOTIFY) {
		printf("CDAUDIO_mciSeek // MCI_NOTIFY_SUCCESSFUL %08X !\n", lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			CDADev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
	return dwRet;
#else
	return MCIERR_HARDWARE;
#endif
}


/**************************************************************************
* 				CDAUDIO_mciSet			[internal]
*/
DWORD CDAUDIO_mciSet(UINT wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_CDAUDIO
	printf("CDAUDIO_mciSet(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
/*
	printf("CDAUDIO_mciSet // dwTimeFormat=%08X\n", lpParms->dwTimeFormat);
	printf("CDAUDIO_mciSet // dwAudio=%08X\n", lpParms->dwAudio);
*/
	if (dwFlags & MCI_SET_TIME_FORMAT) {
		switch (lpParms->dwTimeFormat) {
			case MCI_FORMAT_MILLISECONDS:
#ifdef DEBUG_CDAUDIO
				printf("CDAUDIO_mciSet // MCI_FORMAT_MILLISECONDS !\n");
#endif
				break;
			case MCI_FORMAT_MSF:
#ifdef DEBUG_CDAUDIO
				printf("CDAUDIO_mciSet // MCI_FORMAT_MSF !\n");
#endif
				break;
			case MCI_FORMAT_TMSF:
#ifdef DEBUG_CDAUDIO
				printf("CDAUDIO_mciSet // MCI_FORMAT_TMSF !\n");
#endif
				break;
			default:
				printf("CDAUDIO_mciSet // bad time format !\n");
				return MCIERR_BAD_TIME_FORMAT;
			}
		CDADev[wDevID].dwTimeFormat = lpParms->dwTimeFormat;
		}
	if (dwFlags & MCI_SET_DOOR_OPEN) {
		printf("CDAUDIO_mciSet // MCI_SET_DOOR_OPEN !\n");
		if (ioctl(CDADev[wDevID].unixdev, CDROMEJECT)) return MCIERR_HARDWARE;
		CDADev[wDevID].nTracks = 0;
		}
	if (dwFlags & MCI_SET_DOOR_CLOSED) {
		printf("CDAUDIO_mciSet // MCI_SET_DOOR_CLOSED !\n");
		if (ioctl(CDADev[wDevID].unixdev, CDROMEJECT)) return MCIERR_HARDWARE;
		CDADev[wDevID].nTracks = 0;
		}
	if (dwFlags & MCI_SET_VIDEO) return MCIERR_UNSUPPORTED_FUNCTION;
	if (dwFlags & MCI_SET_ON) return MCIERR_UNSUPPORTED_FUNCTION;
	if (dwFlags & MCI_SET_OFF) return MCIERR_UNSUPPORTED_FUNCTION;
	if (dwFlags & MCI_NOTIFY) {
		printf("CDAUDIO_mciSet // MCI_NOTIFY_SUCCESSFUL %08X !\n", lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			CDADev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
	return 0;
#else
	return MCIERR_HARDWARE;
#endif
}


/**************************************************************************
* 				CDAUDIO_DriverProc		[sample driver]
*/
LRESULT CDAUDIO_DriverProc(DWORD dwDevID, HDRVR hDriv, WORD wMsg, 
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
			return CDAUDIO_mciOpen(dwParam1, (LPMCI_OPEN_PARMS)dwParam2); 
		case DRV_CLOSE:
		case MCI_CLOSE_DRIVER:
		case MCI_CLOSE:
			return CDAUDIO_mciClose(dwDevID, dwParam1, 
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
			return CDAUDIO_mciGetDevCaps(dwDevID, dwParam1, 
					(LPMCI_GETDEVCAPS_PARMS)dwParam2);
		case MCI_INFO:
			return CDAUDIO_mciInfo(dwDevID, dwParam1, 
						(LPMCI_INFO_PARMS)dwParam2);
		case MCI_STATUS:
			return CDAUDIO_mciStatus(dwDevID, dwParam1, 
						(LPMCI_STATUS_PARMS)dwParam2);
		case MCI_SET:
			return CDAUDIO_mciSet(dwDevID, dwParam1, 
						(LPMCI_SET_PARMS)dwParam2);
		case MCI_PLAY:
			return CDAUDIO_mciPlay(dwDevID, dwParam1, 
						(LPMCI_PLAY_PARMS)dwParam2);
		case MCI_STOP:
			return CDAUDIO_mciStop(dwDevID, dwParam1, 
					(LPMCI_GENERIC_PARMS)dwParam2);
		case MCI_PAUSE:
			return CDAUDIO_mciPause(dwDevID, dwParam1, 
					(LPMCI_GENERIC_PARMS)dwParam2);
		case MCI_RESUME:
			return CDAUDIO_mciResume(dwDevID, dwParam1, 
					(LPMCI_GENERIC_PARMS)dwParam2);
		case MCI_SEEK:
			return CDAUDIO_mciSeek(dwDevID, dwParam1, 
					(LPMCI_SEEK_PARMS)dwParam2);
		case MCI_SET_DOOR_OPEN:
			printf("CDAUDIO_DriverProc // MCI_SET_DOOR_OPEN !\n");
			if (ioctl(CDADev[dwDevID].unixdev, CDROMEJECT)) return MCIERR_HARDWARE;
			CDADev[dwDevID].nTracks = 0;
			return 0;
		case MCI_SET_DOOR_CLOSED:
			printf("CDAUDIO_DriverProc // MCI_SET_DOOR_CLOSED !\n");
			if (ioctl(CDADev[dwDevID].unixdev, CDROMEJECT, 1)) return MCIERR_HARDWARE;
			CDADev[dwDevID].nTracks = 0;
			return 0;
		default:
			return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
		}
#else
	return MCIERR_HARDWARE;
#endif
}


/*-----------------------------------------------------------------------*/

#endif /* #ifdef BUILTIN_MMSYSTEM */
