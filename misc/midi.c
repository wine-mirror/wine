/*
 * Sample MIDI Wine Driver for Linux
 *
 * Copyright 1994 Martin Ayotte
 */
static char Copyright[] = "Copyright  Martin Ayotte, 1994";

#ifndef WINELIB
#define BUILTIN_MMSYSTEM
#endif 

#ifdef BUILTIN_MMSYSTEM

#define DEBUG_MCIMIDI

#include "stdio.h"
#include "win.h"
#include "user.h"
#include "driver.h"
#include "mmsystem.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#ifdef linux
#include <linux/soundcard.h>
#endif

#ifdef linux
#define MIDI_DEV "/dev/midi"

#ifdef SOUND_VERSION
#define IOCTL(a,b,c)		ioctl(a,b,&c)
#else
#define IOCTL(a,b,c)		(c = ioctl(a,b,c) )
#endif

#define MAX_MIDIINDRV 	2
#define MAX_MIDIOUTDRV 	2
#define MAX_MCIMIDIDRV 	2

typedef struct {
	int				unixdev;
	int				state;
	DWORD			bufsize;
	MIDIOPENDESC	midiDesc;
	WORD			wFlags;
	MIDIHDR 		lpQueueHdr;
	DWORD			dwTotalPlayed;
	} LINUX_MIDIIN;

typedef struct {
	int				unixdev;
	int				state;
	DWORD			bufsize;
	MIDIOPENDESC	midiDesc;
	WORD			wFlags;
	MIDIHDR 		lpQueueHdr;
	DWORD			dwTotalPlayed;
	} LINUX_MIDIOUT;

typedef struct {
	int     nUseCount;          /* Incremented for each shared open */
	BOOL    fShareable;         /* TRUE if first open was shareable */
	WORD    wNotifyDeviceID;    /* MCI device ID with a pending notification */
	HANDLE  hCallback;          /* Callback handle for pending notification */
	HMMIO	hFile;				/* mmio file handle open as Element		*/
	MCI_OPEN_PARMS openParms;
	MIDIHDR		MidiHdr;
	WORD		dwStatus;
	} LINUX_MCIMIDI;

static LINUX_MIDIIN		MidiInDev[MAX_MIDIINDRV];
static LINUX_MIDIOUT	MidiOutDev[MAX_MIDIOUTDRV];
static LINUX_MCIMIDI	MCIMidiDev[MAX_MCIMIDIDRV];
#endif

DWORD MIDI_mciOpen(DWORD dwFlags, LPMCI_OPEN_PARMS lpParms);
DWORD MIDI_mciClose(UINT wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms);
DWORD MIDI_mciPlay(UINT wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms);
DWORD MIDI_mciRecord(UINT wDevID, DWORD dwFlags, LPMCI_RECORD_PARMS lpParms);
DWORD MIDI_mciStop(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);
DWORD MIDI_mciPause(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);
DWORD MIDI_mciResume(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);
DWORD MIDI_mciSet(UINT wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms);
DWORD MIDI_mciStatus(UINT wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms);
DWORD MIDI_mciGetDevCaps(UINT wDevID, DWORD dwFlags, LPMCI_GETDEVCAPS_PARMS lpParms);
DWORD MIDI_mciInfo(UINT wDevID, DWORD dwFlags, LPMCI_INFO_PARMS lpParms);


/**************************************************************************
* 				MIDI_NotifyClient			[internal]
*/
DWORD MIDI_NotifyClient(UINT wDevID, WORD wMsg, 
				DWORD dwParam1, DWORD dwParam2)
{
#ifdef linux
	if (MidiInDev[wDevID].wFlags != DCB_NULL && !DriverCallback(
		MidiInDev[wDevID].midiDesc.dwCallback, MidiInDev[wDevID].wFlags, 
		MidiInDev[wDevID].midiDesc.hMidi, wMsg, 
		MidiInDev[wDevID].midiDesc.dwInstance, dwParam1, dwParam2)) {
		printf("MIDI_NotifyClient // can't notify client !\n");
		return MMSYSERR_NOERROR;
		}
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
* 				AUDIO_DriverProc		[sample driver]
*/
LRESULT MIDI_DriverProc(DWORD dwDevID, HDRVR hDriv, WORD wMsg, 
						DWORD dwParam1, DWORD dwParam2)
{
#ifdef linux
	switch(wMsg) {
		case DRV_LOAD:
			return (LRESULT)1L;
		case DRV_FREE:
			return (LRESULT)1L;
		case DRV_OPEN:
			return (LRESULT)1L;
		case DRV_CLOSE:
			return (LRESULT)1L;
		case DRV_ENABLE:
			return (LRESULT)1L;
		case DRV_DISABLE:
			return (LRESULT)1L;
		case DRV_QUERYCONFIGURE:
			return (LRESULT)1L;
		case DRV_CONFIGURE:
			MessageBox((HWND)NULL, "Sample Midi Linux Driver !", 
								"MMLinux Driver", MB_OK);
			return (LRESULT)1L;
		case DRV_INSTALL:
			return (LRESULT)DRVCNF_RESTART;
		case DRV_REMOVE:
			return (LRESULT)DRVCNF_RESTART;
		case MCI_OPEN_DRIVER:
		case MCI_OPEN:
			return MIDI_mciOpen(dwParam1, (LPMCI_OPEN_PARMS)dwParam2);
		case MCI_CLOSE_DRIVER:
		case MCI_CLOSE:
			return MIDI_mciClose(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
		case MCI_PLAY:
			return MIDI_mciPlay(dwDevID, dwParam1, (LPMCI_PLAY_PARMS)dwParam2);
		case MCI_RECORD:
			return MIDI_mciRecord(dwDevID, dwParam1, (LPMCI_RECORD_PARMS)dwParam2);
		case MCI_STOP:
			return MIDI_mciStop(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
		case MCI_SET:
			return MIDI_mciSet(dwDevID, dwParam1, (LPMCI_SET_PARMS)dwParam2);
		case MCI_PAUSE:
			return MIDI_mciPause(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
		case MCI_RESUME:
			return MIDI_mciResume(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
		case MCI_STATUS:
			return MIDI_mciStatus(dwDevID, dwParam1, (LPMCI_STATUS_PARMS)dwParam2);
		case MCI_GETDEVCAPS:
			return MIDI_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)dwParam2);
		case MCI_INFO:
			return MIDI_mciInfo(dwDevID, dwParam1, (LPMCI_INFO_PARMS)dwParam2);
		default:
			return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
		}
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				MIDI_mciOpen			[internal]	
*/
DWORD MIDI_mciOpen(DWORD dwFlags, LPMCI_OPEN_PARMS lpParms)
{
#ifdef linux
	int			hFile;
	UINT 		wDevID;
	OFSTRUCT	OFstruct;
	MIDIOPENDESC 	MidiDesc;
	DWORD		dwRet;
	char		str[128];
	LPSTR		ptr;
#ifdef DEBUG_MCIMIDI
	printf("MIDI_mciOpen(%08X, %08X)\n", dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	wDevID = lpParms->wDeviceID;
	if (MCIMidiDev[wDevID].nUseCount > 0) {
		/* The driver already open on this channel */
		/* If the driver was opened shareable before and this open specifies */
		/* shareable then increment the use count */
		if (MCIMidiDev[wDevID].fShareable && (dwFlags & MCI_OPEN_SHAREABLE))
			++MCIMidiDev[wDevID].nUseCount;
		else
			return MCIERR_MUST_USE_SHAREABLE;
		}
	else {
		MCIMidiDev[wDevID].nUseCount = 1;
		MCIMidiDev[wDevID].fShareable = dwFlags & MCI_OPEN_SHAREABLE;
		}
    if (dwFlags & MCI_OPEN_ELEMENT) {
		printf("MIDI_mciOpen // MCI_OPEN_ELEMENT '%s' !\n", 
								lpParms->lpstrElementName);
/*		printf("MIDI_mciOpen // cdw='%s'\n", DOS_GetCurrentDir(DOS_GetDefaultDrive())); */
		if (strlen(lpParms->lpstrElementName) > 0) {
			strcpy(str, lpParms->lpstrElementName);
			AnsiUpper(str);
			MCIMidiDev[wDevID].hFile = mmioOpen(str, NULL, 
				MMIO_ALLOCBUF | MMIO_READWRITE | MMIO_EXCLUSIVE);
			if (MCIMidiDev[wDevID].hFile == 0) {
				printf("MIDI_mciOpen // can't find file='%s' !\n", str);
				return MCIERR_FILE_NOT_FOUND;
				}
			}
		else 
			MCIMidiDev[wDevID].hFile = 0;
		}
	printf("MIDI_mciOpen // hFile=%u\n", MCIMidiDev[wDevID].hFile);
	memcpy(&MCIMidiDev[wDevID].openParms, lpParms, sizeof(MCI_OPEN_PARMS));
	MCIMidiDev[wDevID].wNotifyDeviceID = lpParms->wDeviceID;
	MCIMidiDev[wDevID].dwStatus = MCI_MODE_STOP;
	MidiDesc.hMidi = 0;
	if (MCIMidiDev[wDevID].hFile != 0) {
		MMCKINFO	mmckInfo;
		MMCKINFO	ckMainRIFF;
		if (mmioDescend(MCIMidiDev[wDevID].hFile, &ckMainRIFF, NULL, 0) != 0) {
			return MCIERR_INTERNAL;
			}
#ifdef DEBUG_MCIMIDI
		printf("MIDI_mciOpen // ParentChunk ckid=%.4s fccType=%.4s cksize=%08lX \n",
				(LPSTR)&ckMainRIFF.ckid, (LPSTR)&ckMainRIFF.fccType,
				ckMainRIFF.cksize);
#endif
		if (ckMainRIFF.ckid != FOURCC_RIFF) return MCIERR_INTERNAL;
		if (ckMainRIFF.fccType != mmioFOURCC('R', 'M', 'I', 'D') &&
			ckMainRIFF.fccType != mmioFOURCC('M', 'T', 'h', 'd')) {
			return MCIERR_INTERNAL;
			}
		mmckInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
		if (mmioDescend(MCIMidiDev[wDevID].hFile, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK) != 0) {
			return MCIERR_INTERNAL;
			}
#ifdef DEBUG_MCIMIDI
		printf("MIDI_mciOpen // Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX \n",
				(LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType,
				mmckInfo.cksize);
#endif
		}
	dwRet = modMessage(0, MODM_OPEN, 0, (DWORD)&MidiDesc, CALLBACK_NULL);
	dwRet = midMessage(0, MIDM_OPEN, 0, (DWORD)&MidiDesc, CALLBACK_NULL);
	return 0;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
* 				MIDI_mciClose		[internal]
*/
DWORD MIDI_mciClose(UINT wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
#ifdef linux
	DWORD		dwRet;
#ifdef DEBUG_MCIMIDI
	printf("MIDI_mciClose(%u, %08X, %08X);\n", wDevID, dwParam, lpParms);
#endif
	if (MCIMidiDev[wDevID].dwStatus != MCI_MODE_STOP) {
		MIDI_mciStop(wDevID, MCI_WAIT, lpParms);
		}
	MCIMidiDev[wDevID].dwStatus = MCI_MODE_STOP;
	MCIMidiDev[wDevID].nUseCount--;
	if (MCIMidiDev[wDevID].nUseCount == 0) {
		if (MCIMidiDev[wDevID].hFile != 0) {
			close(MCIMidiDev[wDevID].hFile);
			MCIMidiDev[wDevID].hFile = 0;
			}
		dwRet = modMessage(0, MODM_CLOSE, 0, 0L, 0L);
		if (dwRet != MMSYSERR_NOERROR) return MCIERR_INTERNAL;
		dwRet = midMessage(0, MIDM_CLOSE, 0, 0L, 0L);
		if (dwRet != MMSYSERR_NOERROR) return MCIERR_INTERNAL;
		}
	return 0;
#else
	return 0;
#endif
}


/**************************************************************************
* 				MIDI_mciPlay		[internal]
*/
DWORD MIDI_mciPlay(UINT wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
#ifdef linux
	int			count;
	int			start, end;
	LPMIDIHDR	lpMidiHdr;
	DWORD		dwRet;
#ifdef DEBUG_MCIMIDI
	printf("MIDI_mciPlay(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (MCIMidiDev[wDevID].hFile == 0) {
		printf("MIDI_mciPlay // can't find file='%s' !\n", 
				MCIMidiDev[wDevID].openParms.lpstrElementName);
		return MCIERR_FILE_NOT_FOUND;
		}
	start = 1; 		end = 99999;
	if (dwFlags & MCI_FROM) {
		start = lpParms->dwFrom; 
		printf("MIDI_mciPlay // MCI_FROM=%d \n", start);
		}
	if (dwFlags & MCI_TO) {
		end = lpParms->dwTo;
		printf("MIDI_mciPlay // MCI_TO=%d \n", end);
		}
/**/
	if (dwFlags & MCI_NOTIFY) {
		printf("MIDI_mciPlay // MCI_NOTIFY %08X !\n", lpParms->dwCallback);
		switch(fork()) {
			case -1:
				printf("MIDI_mciPlay // Can't 'fork' process !\n");
				break;
			case 0:
				printf("MIDI_mciPlay // process started ! play in background ...\n");
				break;
			default:
				printf("MIDI_mciPlay // process started ! return to caller...\n");
				return 0;
			}
		}
/**/
	lpMidiHdr = &MCIMidiDev[wDevID].MidiHdr;
	lpMidiHdr->lpData = (LPSTR) malloc(64000);
	lpMidiHdr->dwBufferLength = 32000;
	lpMidiHdr->dwUser = 0L;
	lpMidiHdr->dwFlags = 0L;
	dwRet = modMessage(0, MODM_PREPARE, 0, (DWORD)lpMidiHdr, sizeof(MIDIHDR));
/*	printf("MIDI_mciPlay // after MODM_PREPARE \n"); */
	MCIMidiDev[wDevID].dwStatus = MCI_MODE_PLAY;
	while(MCIMidiDev[wDevID].dwStatus != MCI_MODE_STOP) {
		printf("MIDI_mciPlay // MCIMidiDev[wDevID].dwStatus=%p %d\n",
			&MCIMidiDev[wDevID].dwStatus, MCIMidiDev[wDevID].dwStatus);
		count = mmioRead(MCIMidiDev[wDevID].hFile, lpMidiHdr->lpData, lpMidiHdr->dwBufferLength);
		if (count < 1) break;
		lpMidiHdr->dwBytesRecorded = count;
#ifdef DEBUG_MCIMIDI
		printf("MIDI_mciPlay // before MODM_LONGDATA lpMidiHdr=%08X dwBytesRecorded=%u\n",
					lpMidiHdr, lpMidiHdr->dwBytesRecorded);
#endif
/*		dwRet = modMessage(0, MODM_LONGDATA, 0, (DWORD)lpMidiHdr, sizeof(MIDIHDR)); */
		}
	dwRet = modMessage(0, MODM_UNPREPARE, 0, (DWORD)lpMidiHdr, sizeof(MIDIHDR));
	if (lpMidiHdr->lpData != NULL) {
		free(lpMidiHdr->lpData);
		lpMidiHdr->lpData = NULL;
		}
	MCIMidiDev[wDevID].dwStatus = MCI_MODE_STOP;
	if (dwFlags & MCI_NOTIFY) {
#ifdef DEBUG_MCIMIDI
		printf("MIDI_mciPlay // MCI_NOTIFY_SUCCESSFUL %08X !\n", lpParms->dwCallback);
#endif
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			MCIMidiDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		exit(1);
		}
	return 0;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
* 				MIDI_mciRecord			[internal]
*/
DWORD MIDI_mciRecord(UINT wDevID, DWORD dwFlags, LPMCI_RECORD_PARMS lpParms)
{
#ifdef linux
	int			count;
	int			start, end;
	LPMIDIHDR	lpMidiHdr;
	DWORD		dwRet;
#ifdef DEBUG_MCIMIDI
	printf("MIDI_mciRecord(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (MCIMidiDev[wDevID].hFile == 0) {
		printf("MIDI_mciRecord // can't find file='%s' !\n", 
				MCIMidiDev[wDevID].openParms.lpstrElementName);
		return MCIERR_FILE_NOT_FOUND;
		}
	start = 1; 		end = 99999;
	if (dwFlags & MCI_FROM) {
		start = lpParms->dwFrom; 
		printf("MIDI_mciRecord // MCI_FROM=%d \n", start);
		}
	if (dwFlags & MCI_TO) {
		end = lpParms->dwTo;
		printf("MIDI_mciRecord // MCI_TO=%d \n", end);
		}
	lpMidiHdr = &MCIMidiDev[wDevID].MidiHdr;
	lpMidiHdr->lpData = (LPSTR) malloc(64000);
	lpMidiHdr->dwBufferLength = 32000;
	lpMidiHdr->dwUser = 0L;
	lpMidiHdr->dwFlags = 0L;
	dwRet = midMessage(0, MIDM_PREPARE, 0, (DWORD)lpMidiHdr, sizeof(MIDIHDR));
	printf("MIDI_mciRecord // after MIDM_PREPARE \n");
	MCIMidiDev[wDevID].dwStatus = MCI_MODE_RECORD;
	while(MCIMidiDev[wDevID].dwStatus != MCI_MODE_STOP) {
		printf("MIDI_mciRecord // MCIMidiDev[wDevID].dwStatus=%p %d\n",
			&MCIMidiDev[wDevID].dwStatus, MCIMidiDev[wDevID].dwStatus);
		lpMidiHdr->dwBytesRecorded = 0;
		dwRet = midMessage(0, MIDM_START, 0, 0L, 0L);
		printf("MIDI_mciRecord // after MIDM_START lpMidiHdr=%08X dwBytesRecorded=%u\n",
					lpMidiHdr, lpMidiHdr->dwBytesRecorded);
		if (lpMidiHdr->dwBytesRecorded == 0) break;
		}
	printf("MIDI_mciRecord // before MIDM_UNPREPARE \n");
	dwRet = midMessage(0, MIDM_UNPREPARE, 0, (DWORD)lpMidiHdr, sizeof(MIDIHDR));
	printf("MIDI_mciRecord // after MIDM_UNPREPARE \n");
	if (lpMidiHdr->lpData != NULL) {
		free(lpMidiHdr->lpData);
		lpMidiHdr->lpData = NULL;
		}
	MCIMidiDev[wDevID].dwStatus = MCI_MODE_STOP;
	if (dwFlags & MCI_NOTIFY) {
#ifdef DEBUG_MCIMIDI
		printf("MIDI_mciRecord // MCI_NOTIFY_SUCCESSFUL %08X !\n", lpParms->dwCallback);
#endif
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			MCIMidiDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
	return 0;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
* 				MIDI_mciStop			[internal]
*/
DWORD MIDI_mciStop(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_MCIMIDI
	printf("MIDI_mciStop(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	MCIMidiDev[wDevID].dwStatus = MCI_MODE_STOP;
	printf("MIDI_mciStop // MCIMidiDev[wDevID].dwStatus=%p %d\n",
			&MCIMidiDev[wDevID].dwStatus, MCIMidiDev[wDevID].dwStatus);
	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}


/**************************************************************************
* 				MIDI_mciPause			[internal]
*/
DWORD MIDI_mciPause(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_MCIMIDI
	printf("MIDI_mciPause(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}


/**************************************************************************
* 				MIDI_mciResume			[internal]
*/
DWORD MIDI_mciResume(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_MCIMIDI
	printf("MIDI_mciResume(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}


/**************************************************************************
* 				MIDI_mciSet			[internal]
*/
DWORD MIDI_mciSet(UINT wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_MCIMIDI
	printf("MIDI_mciSet(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
#ifdef DEBUG_MCIMIDI
	printf("MIDI_mciSet // dwTimeFormat=%08X\n", lpParms->dwTimeFormat);
	printf("MIDI_mciSet // dwAudio=%08X\n", lpParms->dwAudio);
#endif
	if (dwFlags & MCI_SET_TIME_FORMAT) {
		switch (lpParms->dwTimeFormat) {
			case MCI_FORMAT_MILLISECONDS:
				printf("MIDI_mciSet // MCI_FORMAT_MILLISECONDS !\n");
				break;
			case MCI_FORMAT_BYTES:
				printf("MIDI_mciSet // MCI_FORMAT_BYTES !\n");
				break;
			case MCI_FORMAT_SAMPLES:
				printf("MIDI_mciSet // MCI_FORMAT_SAMPLES !\n");
				break;
			default:
				printf("MIDI_mciSet // bad time format !\n");
				return MCIERR_BAD_TIME_FORMAT;
			}
		}
	if (dwFlags & MCI_SET_VIDEO) return MCIERR_UNSUPPORTED_FUNCTION;
	if (dwFlags & MCI_SET_DOOR_OPEN) return MCIERR_UNSUPPORTED_FUNCTION;
	if (dwFlags & MCI_SET_DOOR_CLOSED) return MCIERR_UNSUPPORTED_FUNCTION;
	if (dwFlags & MCI_SET_AUDIO) {
		printf("MIDI_mciSet // MCI_SET_AUDIO !\n");
		}
	if (dwFlags && MCI_SET_ON) {
		printf("MIDI_mciSet // MCI_SET_ON !\n");
		if (dwFlags && MCI_SET_AUDIO_LEFT) {
			printf("MIDI_mciSet // MCI_SET_AUDIO_LEFT !\n");
			}
		if (dwFlags && MCI_SET_AUDIO_RIGHT) {
			printf("MIDI_mciSet // MCI_SET_AUDIO_RIGHT !\n");
			}
		}
	if (dwFlags & MCI_SET_OFF) {
		printf("MIDI_mciSet // MCI_SET_OFF !\n");
		}
	if (dwFlags & MCI_SEQ_SET_MASTER) {
		printf("MIDI_mciSet // MCI_SEQ_SET_MASTER !\n");
		}
	if (dwFlags & MCI_SEQ_SET_SLAVE) {
		printf("MIDI_mciSet // MCI_SEQ_SET_SLAVE !\n");
		}
	if (dwFlags & MCI_SEQ_SET_OFFSET) {
		printf("MIDI_mciSet // MCI_SEQ_SET_OFFSET !\n");
		}
	if (dwFlags & MCI_SEQ_SET_PORT) {
		printf("MIDI_mciSet // MCI_SEQ_SET_PORT !\n");
		}
	if (dwFlags & MCI_SEQ_SET_TEMPO) {
		printf("MIDI_mciSet // MCI_SEQ_SET_TEMPO !\n");
		}
 	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}


/**************************************************************************
* 				MIDI_mciStatus		[internal]
*/
DWORD MIDI_mciStatus(UINT wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_MCIMIDI
	printf("MIDI_mciStatus(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	if (dwFlags & MCI_STATUS_ITEM) {
		switch(lpParms->dwItem) {
			case MCI_STATUS_CURRENT_TRACK:
				lpParms->dwReturn = 1;
				break;
			case MCI_STATUS_LENGTH:
				lpParms->dwReturn = 5555;
				if (dwFlags & MCI_TRACK) {
					lpParms->dwTrack = 1;
					lpParms->dwReturn = 2222;
					}
				break;
			case MCI_STATUS_MODE:
				lpParms->dwReturn = MCI_MODE_STOP;
				break;
			case MCI_STATUS_MEDIA_PRESENT:
				printf("MIDI_mciStatus // MCI_STATUS_MEDIA_PRESENT !\n");
				lpParms->dwReturn = TRUE;
				break;
			case MCI_STATUS_NUMBER_OF_TRACKS:
				lpParms->dwReturn = 1;
				break;
			case MCI_STATUS_POSITION:
				lpParms->dwReturn = 3333;
				if (dwFlags & MCI_STATUS_START) {
					lpParms->dwItem = 1;
					}
				if (dwFlags & MCI_TRACK) {
					lpParms->dwTrack = 1;
					lpParms->dwReturn = 777;
					}
				break;
			case MCI_STATUS_READY:
				printf("MIDI_mciStatus // MCI_STATUS_READY !\n");
				lpParms->dwReturn = TRUE;
				break;
			case MCI_STATUS_TIME_FORMAT:
				printf("MIDI_mciStatus // MCI_STATUS_TIME_FORMAT !\n");
				lpParms->dwReturn = MCI_FORMAT_MILLISECONDS;
				break;
			case MCI_SEQ_STATUS_DIVTYPE:
				printf("MIDI_mciStatus // MCI_SEQ_STATUS_DIVTYPE !\n");
				lpParms->dwReturn = 0;
				break;
			case MCI_SEQ_STATUS_MASTER:
				printf("MIDI_mciStatus // MCI_SEQ_STATUS_MASTER !\n");
				lpParms->dwReturn = 0;
				break;
			case MCI_SEQ_STATUS_SLAVE:
				printf("MIDI_mciStatus // MCI_SEQ_STATUS_SLAVE !\n");
				lpParms->dwReturn = 0;
				break;
			case MCI_SEQ_STATUS_OFFSET:
				printf("MIDI_mciStatus // MCI_SEQ_STATUS_OFFSET !\n");
				lpParms->dwReturn = 0;
				break;
			case MCI_SEQ_STATUS_PORT:
				printf("MIDI_mciStatus // MCI_SEQ_STATUS_PORT !\n");
				lpParms->dwReturn = 0;
				break;
			case MCI_SEQ_STATUS_TEMPO:
				printf("MIDI_mciStatus // MCI_SEQ_STATUS_TEMPO !\n");
				lpParms->dwReturn = 0;
				break;
			default:
				printf("MIDI_mciStatus // unknowm command %04X !\n", lpParms->dwItem);
				return MCIERR_UNRECOGNIZED_COMMAND;
			}
		}
	if (dwFlags & MCI_NOTIFY) {
		printf("MIDI_mciStatus // MCI_NOTIFY_SUCCESSFUL %08X !\n", lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			MCIMidiDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
 	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}

/**************************************************************************
* 				MIDI_mciGetDevCaps		[internal]
*/
DWORD MIDI_mciGetDevCaps(UINT wDevID, DWORD dwFlags, 
					LPMCI_GETDEVCAPS_PARMS lpParms)
{
#ifdef linux
	printf("MIDI_mciGetDevCaps(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	if (dwFlags & MCI_GETDEVCAPS_ITEM) {
		switch(lpParms->dwItem) {
			case MCI_GETDEVCAPS_CAN_RECORD:
				lpParms->dwReturn = TRUE;
				break;
			case MCI_GETDEVCAPS_HAS_AUDIO:
				lpParms->dwReturn = TRUE;
				break;
			case MCI_GETDEVCAPS_HAS_VIDEO:
				lpParms->dwReturn = FALSE;
				break;
			case MCI_GETDEVCAPS_DEVICE_TYPE:
				lpParms->dwReturn = MCI_DEVTYPE_SEQUENCER;
				break;
			case MCI_GETDEVCAPS_USES_FILES:
				lpParms->dwReturn = TRUE;
				break;
			case MCI_GETDEVCAPS_COMPOUND_DEVICE:
				lpParms->dwReturn = TRUE;
				break;
			case MCI_GETDEVCAPS_CAN_EJECT:
				lpParms->dwReturn = FALSE;
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
 	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}

/**************************************************************************
* 				MIDI_mciInfo			[internal]
*/
DWORD MIDI_mciInfo(UINT wDevID, DWORD dwFlags, LPMCI_INFO_PARMS lpParms)
{
#ifdef linux
	printf("MIDI_mciInfo(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	lpParms->lpstrReturn = NULL;
	switch(dwFlags) {
		case MCI_INFO_PRODUCT:
			lpParms->lpstrReturn = "Linux Sound System 0.5";
			break;
		case MCI_INFO_FILE:
			lpParms->lpstrReturn = "FileName";
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


/*-----------------------------------------------------------------------*/


/**************************************************************************
* 				midGetDevCaps			[internal]
*/
DWORD midGetDevCaps(WORD wDevID, LPMIDIINCAPS lpCaps, DWORD dwSize)
{
	printf("midGetDevCaps(%u, %08X, %08X);\n", wDevID, lpCaps, dwSize);
	return MMSYSERR_NOTENABLED;
}

/**************************************************************************
* 				midOpen					[internal]
*/
DWORD midOpen(WORD wDevID, LPMIDIOPENDESC lpDesc, DWORD dwFlags)
{
	printf("modOpen(%u, %08X, %08X);\n", wDevID, lpDesc, dwFlags);
	return MMSYSERR_NOTENABLED;
}

/**************************************************************************
* 				midClose				[internal]
*/
DWORD midClose(WORD wDevID)
{
	printf("midClose(%u);\n", wDevID);
	return MMSYSERR_NOTENABLED;
}

/**************************************************************************
* 				midAddBuffer		[internal]
*/
DWORD midAddBuffer(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
	printf("midAddBuffer(%u, %08X, %08X);\n", wDevID, lpMidiHdr, dwSize);
	return MMSYSERR_NOTENABLED;
}

/**************************************************************************
* 				midPrepare			[internal]
*/
DWORD midPrepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
	printf("midPrepare(%u, %08X, %08X);\n", wDevID, lpMidiHdr, dwSize);
	return MMSYSERR_NOTENABLED;
}

/**************************************************************************
* 				midUnprepare			[internal]
*/
DWORD midUnprepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
	printf("midUnprepare(%u, %08X, %08X);\n", wDevID, lpMidiHdr, dwSize);
	return MMSYSERR_NOTENABLED;
}

/**************************************************************************
* 				midReset				[internal]
*/
DWORD midReset(WORD wDevID)
{
	printf("midReset(%u);\n", wDevID);
	return MMSYSERR_NOTENABLED;
}


/**************************************************************************
* 				midStart				[internal]
*/
DWORD midStart(WORD wDevID)
{
	printf("midStart(%u);\n", wDevID);
	return MMSYSERR_NOTENABLED;
}


/**************************************************************************
* 				midStop					[internal]
*/
DWORD midStop(WORD wDevID)
{
	printf("midStop(%u);\n", wDevID);
	return MMSYSERR_NOTENABLED;
}


/**************************************************************************
* 				midMessage				[sample driver]
*/
DWORD midMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2)
{
	printf("midMessage(%u, %04X, %08X, %08X, %08X);\n", 
			wDevID, wMsg, dwUser, dwParam1, dwParam2);
	switch(wMsg) {
		case MIDM_OPEN:
			return midOpen(wDevID, (LPMIDIOPENDESC)dwParam1, dwParam2);
		case MIDM_CLOSE:
			return midClose(wDevID);
		case MIDM_ADDBUFFER:
			return midAddBuffer(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
		case MIDM_PREPARE:
			return midPrepare(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
		case MIDM_UNPREPARE:
			return midUnprepare(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
		case MIDM_GETDEVCAPS:
			return midGetDevCaps(wDevID, (LPMIDIINCAPS)dwParam1, dwParam2);
		case MIDM_GETNUMDEVS:
			return 1L;
		case MIDM_RESET:
			return midReset(wDevID);
		case MIDM_START:
			return midStart(wDevID);
		case MIDM_STOP:
			return midStop(wDevID);
		}
	return MMSYSERR_NOTSUPPORTED;
}



/*-----------------------------------------------------------------------*/


/**************************************************************************
* 				modGetDevCaps			[internal]
*/
DWORD modGetDevCaps(WORD wDevID, LPMIDIOUTCAPS lpCaps, DWORD dwSize)
{
	printf("modGetDevCaps(%u, %08X, %08X);\n", wDevID, lpCaps, dwSize);
	return MMSYSERR_NOTENABLED;
}


/**************************************************************************
* 				modOpen					[internal]
*/
DWORD modOpen(WORD wDevID, LPMIDIOPENDESC lpDesc, DWORD dwFlags)
{								 
	printf("modOpen(%u, %08X, %08X);\n", wDevID, lpDesc, dwFlags);
	return MMSYSERR_NOTENABLED;
}


/**************************************************************************
* 				modClose				[internal]
*/
DWORD modClose(WORD wDevID)
{
	printf("modClose(%u);\n", wDevID);
	return MMSYSERR_NOTENABLED;
}

/**************************************************************************
* 				modData					[internal]
*/
DWORD modData(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
	printf("modData(%u, %08X, %08X);\n", wDevID, lpMidiHdr, dwSize);
	return MMSYSERR_NOTENABLED;
}

/**************************************************************************
* 				modLongData					[internal]
*/
DWORD modLongData(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
	printf("modLongData(%u, %08X, %08X);\n", wDevID, lpMidiHdr, dwSize);
	return MMSYSERR_NOTENABLED;
}

/**************************************************************************
* 				modPrepare				[internal]
*/
DWORD modPrepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
	printf("modPrepare(%u, %08X, %08X);\n", wDevID, lpMidiHdr, dwSize);
	return MMSYSERR_NOTENABLED;
}

/**************************************************************************
* 				modUnprepare			[internal]
*/
DWORD modUnprepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
	printf("modUnprepare(%u, %08X, %08X);\n", wDevID, lpMidiHdr, dwSize);
	return MMSYSERR_NOTENABLED;
}

/**************************************************************************
* 				modReset				[internal]
*/
DWORD modReset(WORD wDevID)
{
	printf("modReset(%u);\n", wDevID);
	return MMSYSERR_NOTENABLED;
}


/**************************************************************************
* 				modGetPosition			[internal]
*/
DWORD modGetPosition(WORD wDevID, LPMMTIME lpTime, DWORD uSize)
{
	printf("modGetposition(%u, %08X, %08X);\n", wDevID, lpTime, uSize);
	return MMSYSERR_NOTENABLED;
}


/**************************************************************************
* 				modMessage			[sample driver]
*/
DWORD modMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2)
{
	printf("modMessage(%u, %04X, %08X, %08X, %08X);\n", 
			wDevID, wMsg, dwUser, dwParam1, dwParam2);
	switch(wMsg) {
		case MODM_OPEN:
			return modOpen(wDevID, (LPMIDIOPENDESC)dwParam1, dwParam2);
		case MODM_CLOSE:
			return modClose(wDevID);
		case MODM_DATA:
			return modData(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
		case MODM_LONGDATA:
			return modLongData(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
		case MODM_PREPARE:
			return modPrepare(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
		case MODM_UNPREPARE:
			return modUnprepare(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
		case MODM_GETDEVCAPS:
			return modGetDevCaps(wDevID, (LPMIDIOUTCAPS)dwParam1, dwParam2);
		case MODM_GETNUMDEVS:
			return 1L;
		case MODM_GETVOLUME:
			return 0L;
		case MODM_SETVOLUME:
			return 0L;
		case MODM_RESET:
			return modReset(wDevID);
		}
	return MMSYSERR_NOTSUPPORTED;
}


/*-----------------------------------------------------------------------*/


#endif /* #ifdef BUILTIN_MMSYSTEM */
