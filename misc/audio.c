/*
 * Sample Wine Driver for Linux
 *
 * Copyright 1994 Martin Ayotte
 */
static char Copyright[] = "Copyright  Martin Ayotte, 1994";

#ifndef WINELIB
#define BUILTIN_MMSYSTEM
#endif 

#ifdef BUILTIN_MMSYSTEM

#define DEBUG_MCIWAVE

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
#define SOUND_DEV "/dev/dsp"

#ifdef SOUND_VERSION
#define IOCTL(a,b,c)		ioctl(a,b,&c)
#else
#define IOCTL(a,b,c)		(c = ioctl(a,b,c) )
#endif

#define MAX_WAVOUTDRV 	2
#define MAX_WAVINDRV 	2
#define MAX_MCIWAVDRV 	2

typedef struct {
	int				unixdev;
	int				state;
	DWORD			bufsize;
	WAVEOPENDESC	waveDesc;
	WORD			wFlags;
	PCMWAVEFORMAT	Format;
	LPWAVEHDR 		lpQueueHdr;
	DWORD			dwTotalPlayed;
	} LINUX_WAVEOUT;

typedef struct {
	int				unixdev;
	int				state;
	DWORD			bufsize;	/* Linux '/dev/dsp' give us that size */
	WAVEOPENDESC	waveDesc;
	WORD			wFlags;
	PCMWAVEFORMAT	Format;
	LPWAVEHDR 		lpQueueHdr;
	DWORD			dwTotalRecorded;
	} LINUX_WAVEIN;

typedef struct {
    int     nUseCount;          /* Incremented for each shared open */
    BOOL    fShareable;         /* TRUE if first open was shareable */
    WORD    wNotifyDeviceID;    /* MCI device ID with a pending notification */
    HANDLE  hCallback;          /* Callback handle for pending notification */
	HMMIO	hFile;				/* mmio file handle open as Element		*/
	MCI_WAVE_OPEN_PARMS openParms;
	PCMWAVEFORMAT	WaveFormat;
	WAVEHDR		WaveHdr;
	} LINUX_MCIWAVE;

static LINUX_WAVEOUT	WOutDev[MAX_WAVOUTDRV];
static LINUX_WAVEIN		WInDev[MAX_WAVOUTDRV];
static LINUX_MCIWAVE	MCIWavDev[MAX_MCIWAVDRV];
#endif

DWORD WAVE_mciOpen(DWORD dwFlags, LPMCI_WAVE_OPEN_PARMS lpParms);
DWORD WAVE_mciClose(UINT wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms);
DWORD WAVE_mciPlay(UINT wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms);
DWORD WAVE_mciRecord(UINT wDevID, DWORD dwFlags, LPMCI_RECORD_PARMS lpParms);
DWORD WAVE_mciStop(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);
DWORD WAVE_mciPause(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);
DWORD WAVE_mciResume(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);
DWORD WAVE_mciSet(UINT wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms);
DWORD WAVE_mciStatus(UINT wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms);
DWORD WAVE_mciGetDevCaps(UINT wDevID, DWORD dwFlags, LPMCI_GETDEVCAPS_PARMS lpParms);
DWORD WAVE_mciInfo(UINT wDevID, DWORD dwFlags, LPMCI_INFO_PARMS lpParms);

DWORD wodGetDevCaps(WORD wDevID, LPWAVEOUTCAPS lpCaps, DWORD dwSize);
DWORD wodOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags);
DWORD wodClose(WORD wDevID);
DWORD wodWrite(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize);
DWORD wodPrepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize);
DWORD wodUnprepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize);


/**************************************************************************
* 				WAVE_NotifyClient			[internal]
*/
DWORD WAVE_NotifyClient(UINT wDevID, WORD wMsg, 
				DWORD dwParam1, DWORD dwParam2)
{
#ifdef linux
	if (WInDev[wDevID].wFlags != DCB_NULL && !DriverCallback(
		WInDev[wDevID].waveDesc.dwCallBack, WInDev[wDevID].wFlags, 
		WInDev[wDevID].waveDesc.hWave, wMsg, 
		WInDev[wDevID].waveDesc.dwInstance, dwParam1, dwParam2)) {
		printf("WAVE_NotifyClient // can't notify client !\n");
		return MMSYSERR_NOERROR;
		}
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
* 				AUDIO_DriverProc		[sample driver]
*/
LRESULT WAVE_DriverProc(DWORD dwDevID, HDRVR hDriv, WORD wMsg, 
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
			MessageBox((HWND)NULL, "Sample MultiMedia Linux Driver !", 
								"MMLinux Driver", MB_OK);
			return (LRESULT)1L;
		case DRV_INSTALL:
			return (LRESULT)DRVCNF_RESTART;
		case DRV_REMOVE:
			return (LRESULT)DRVCNF_RESTART;
		case MCI_OPEN_DRIVER:
		case MCI_OPEN:
			return WAVE_mciOpen(dwParam1, (LPMCI_WAVE_OPEN_PARMS)dwParam2);
		case MCI_CLOSE_DRIVER:
		case MCI_CLOSE:
			return WAVE_mciClose(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
		case MCI_PLAY:
			return WAVE_mciPlay(dwDevID, dwParam1, (LPMCI_PLAY_PARMS)dwParam2);
		case MCI_RECORD:
			return WAVE_mciRecord(dwDevID, dwParam1, (LPMCI_RECORD_PARMS)dwParam2);
		case MCI_STOP:
			return WAVE_mciStop(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
		case MCI_SET:
			return WAVE_mciSet(dwDevID, dwParam1, (LPMCI_SET_PARMS)dwParam2);
		case MCI_PAUSE:
			return WAVE_mciPause(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
		case MCI_RESUME:
			return WAVE_mciResume(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
		case MCI_STATUS:
			return WAVE_mciStatus(dwDevID, dwParam1, (LPMCI_STATUS_PARMS)dwParam2);
		case MCI_GETDEVCAPS:
			return WAVE_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)dwParam2);
		case MCI_INFO:
			return WAVE_mciInfo(dwDevID, dwParam1, (LPMCI_INFO_PARMS)dwParam2);
		default:
			return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
		}
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				WAVE_mciOpen	*/
DWORD WAVE_mciOpen(DWORD dwFlags, LPMCI_WAVE_OPEN_PARMS lpParms)
{
#ifdef linux
	int			hFile;
	UINT 		wDevID;
	OFSTRUCT	OFstruct;
	LPPCMWAVEFORMAT	lpWaveFormat;
	WAVEOPENDESC 	WaveDesc;
	DWORD		dwRet;
	char		str[128];
	LPSTR		ptr;
#ifdef DEBUG_MCIWAVE
	printf("WAVE_mciOpen(%08X, %08X)\n", dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	wDevID = lpParms->wDeviceID;
	if (MCIWavDev[wDevID].nUseCount > 0) {
		/* The driver already open on this channel */
		/* If the driver was opened shareable before and this open specifies */
		/* shareable then increment the use count */
		if (MCIWavDev[wDevID].fShareable && (dwFlags & MCI_OPEN_SHAREABLE))
			++MCIWavDev[wDevID].nUseCount;
		else
			return MCIERR_MUST_USE_SHAREABLE;
		}
	else {
		MCIWavDev[wDevID].nUseCount = 1;
		MCIWavDev[wDevID].fShareable = dwFlags & MCI_OPEN_SHAREABLE;
		}
    if (dwFlags & MCI_OPEN_ELEMENT) {
		printf("WAVE_mciOpen // MCI_OPEN_ELEMENT '%s' !\n", 
								lpParms->lpstrElementName);
/*		printf("WAVE_mciOpen // cdw='%s'\n", DOS_GetCurrentDir(DOS_GetDefaultDrive())); */
		if (strlen(lpParms->lpstrElementName) > 0) {
			strcpy(str, lpParms->lpstrElementName);
			AnsiUpper(str);
			MCIWavDev[wDevID].hFile = mmioOpen(str, NULL, 
				MMIO_ALLOCBUF | MMIO_READWRITE | MMIO_EXCLUSIVE);
			if (MCIWavDev[wDevID].hFile == 0) {
				printf("WAVE_mciOpen // can't find file='%s' !\n", str);
				return MCIERR_FILE_NOT_FOUND;
				}
			}
		else 
			MCIWavDev[wDevID].hFile = 0;
		}
	printf("WAVE_mciOpen // hFile=%u\n", MCIWavDev[wDevID].hFile);
	memcpy(&MCIWavDev[wDevID].openParms, lpParms, sizeof(MCI_WAVE_OPEN_PARMS));
	MCIWavDev[wDevID].wNotifyDeviceID = lpParms->wDeviceID;
	lpWaveFormat = &MCIWavDev[wDevID].WaveFormat;
	WaveDesc.hWave = 0;
	WaveDesc.lpFormat = (LPWAVEFORMAT)lpWaveFormat;
	lpWaveFormat->wf.wFormatTag = WAVE_FORMAT_PCM;
	lpWaveFormat->wBitsPerSample = 8;
	lpWaveFormat->wf.nChannels = 1;
	lpWaveFormat->wf.nSamplesPerSec = 11025;
	lpWaveFormat->wf.nAvgBytesPerSec = 11025;
	lpWaveFormat->wf.nBlockAlign = 1;
	if (MCIWavDev[wDevID].hFile != 0) {
		MMCKINFO	mmckInfo;
		MMCKINFO	ckMainRIFF;
		if (mmioDescend(MCIWavDev[wDevID].hFile, &ckMainRIFF, NULL, 0) != 0) {
			return MCIERR_INTERNAL;
			}
#ifdef DEBUG_MCIWAVE
		printf("WAVE_mciOpen // ParentChunk ckid=%.4s fccType=%.4s cksize=%08lX \n",
				(LPSTR)&ckMainRIFF.ckid, (LPSTR)&ckMainRIFF.fccType,
				ckMainRIFF.cksize);
#endif
		if ((ckMainRIFF.ckid != FOURCC_RIFF) ||
		    (ckMainRIFF.fccType != mmioFOURCC('W', 'A', 'V', 'E'))) {
			return MCIERR_INTERNAL;
			}
		mmckInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
		if (mmioDescend(MCIWavDev[wDevID].hFile, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK) != 0) {
			return MCIERR_INTERNAL;
			}
#ifdef DEBUG_MCIWAVE
		printf("WAVE_mciOpen // Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX \n",
				(LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType,
				mmckInfo.cksize);
#endif
		if (mmioRead(MCIWavDev[wDevID].hFile, (HPSTR) lpWaveFormat,
		    (long) sizeof(PCMWAVEFORMAT)) != (long) sizeof(PCMWAVEFORMAT)) {
			return MCIERR_INTERNAL;
			}
		mmckInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
		if (mmioDescend(MCIWavDev[wDevID].hFile, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK) != 0) {
			return MCIERR_INTERNAL;
			}
#ifdef DEBUG_MCIWAVE
		printf("WAVE_mciOpen // Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX \n",
				(LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType,
				mmckInfo.cksize);
		printf("WAVE_mciOpen // nChannels=%d nSamplesPerSec=%d\n",
			lpWaveFormat->wf.nChannels, lpWaveFormat->wf.nSamplesPerSec);
#endif
		lpWaveFormat->wBitsPerSample = 0;
		}
	lpWaveFormat->wf.nAvgBytesPerSec = 
		lpWaveFormat->wf.nSamplesPerSec * lpWaveFormat->wf.nBlockAlign;
	dwRet = wodMessage(0, WODM_OPEN, 0, (DWORD)&WaveDesc, CALLBACK_NULL);
	dwRet = widMessage(0, WIDM_OPEN, 0, (DWORD)&WaveDesc, CALLBACK_NULL);
	return 0;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				WAVE_mciClose		[internal]
*/
DWORD WAVE_mciClose(UINT wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
#ifdef linux
	DWORD		dwRet;
#ifdef DEBUG_MCIWAVE
	printf("WAVE_mciClose(%u, %08X, %08X);\n", wDevID, dwParam, lpParms);
#endif
	MCIWavDev[wDevID].nUseCount--;
	if (MCIWavDev[wDevID].nUseCount == 0) {
		if (MCIWavDev[wDevID].hFile != 0) {
			close(MCIWavDev[wDevID].hFile);
			MCIWavDev[wDevID].hFile = 0;
			}
		dwRet = wodMessage(0, WODM_CLOSE, 0, 0L, 0L);
		if (dwRet != MMSYSERR_NOERROR) return MCIERR_INTERNAL;
		dwRet = widMessage(0, WIDM_CLOSE, 0, 0L, 0L);
		if (dwRet != MMSYSERR_NOERROR) return MCIERR_INTERNAL;
		}
	return 0;
#else
	return 0;
#endif
}


/**************************************************************************
* 				WAVE_mciPlay		[internal]
*/
DWORD WAVE_mciPlay(UINT wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
#ifdef linux
	int			count;
	int			start, end;
	LPWAVEHDR		lpWaveHdr;
	DWORD		dwRet;
#ifdef DEBUG_MCIWAVE
	printf("WAVE_mciPlay(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (MCIWavDev[wDevID].hFile == 0) {
		printf("WAVE_mciPlay // can't find file='%s' !\n", 
				MCIWavDev[wDevID].openParms.lpstrElementName);
		return MCIERR_FILE_NOT_FOUND;
		}
	start = 1; 		end = 99999;
	if (dwFlags & MCI_FROM) {
		start = lpParms->dwFrom; 
		printf("WAVE_mciPlay // MCI_FROM=%d \n", start);
		}
	if (dwFlags & MCI_TO) {
		end = lpParms->dwTo;
		printf("WAVE_mciPlay // MCI_TO=%d \n", end);
		}
/*
	if (dwFlags & MCI_NOTIFY) {
		printf("WAVE_mciPlay // MCI_NOTIFY %08X !\n", lpParms->dwCallback);
		switch(fork()) {
			case -1:
				printf("WAVE_mciPlay // Can't 'fork' process !\n");
				break;
			case 0:
				break;         
			default:
				printf("WAVE_mciPlay // process started ! return to caller...\n");
				return 0;
			}
		}
*/
	lpWaveHdr = &MCIWavDev[wDevID].WaveHdr;
	lpWaveHdr->lpData = (LPSTR) malloc(64000);
	lpWaveHdr->dwBufferLength = 32000;
	lpWaveHdr->dwUser = 0L;
	lpWaveHdr->dwFlags = 0L;
	lpWaveHdr->dwLoops = 0L;
	dwRet = wodMessage(0, WODM_PREPARE, 0, (DWORD)lpWaveHdr, sizeof(WAVEHDR));
/*	printf("WAVE_mciPlay // after WODM_PREPARE \n"); */
	while(TRUE) {
		count = mmioRead(MCIWavDev[wDevID].hFile, lpWaveHdr->lpData, lpWaveHdr->dwBufferLength);
		if (count < 1) break;
		lpWaveHdr->dwBytesRecorded = count;
#ifdef DEBUG_MCIWAVE
		printf("WAVE_mciPlay // before WODM_WRITE lpWaveHdr=%08X dwBytesRecorded=%u\n",
					lpWaveHdr, lpWaveHdr->dwBytesRecorded);
#endif
		dwRet = wodMessage(0, WODM_WRITE, 0, (DWORD)lpWaveHdr, sizeof(WAVEHDR));
		}
	dwRet = wodMessage(0, WODM_UNPREPARE, 0, (DWORD)lpWaveHdr, sizeof(WAVEHDR));
	if (lpWaveHdr->lpData != NULL) {
		free(lpWaveHdr->lpData);
		lpWaveHdr->lpData = NULL;
		}
	if (dwFlags & MCI_NOTIFY) {
#ifdef DEBUG_MCIWAVE
		printf("WAVE_mciPlay // MCI_NOTIFY_SUCCESSFUL %08X !\n", lpParms->dwCallback);
#endif
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			MCIWavDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
	return 0;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
* 				WAVE_mciRecord			[internal]
*/
DWORD WAVE_mciRecord(UINT wDevID, DWORD dwFlags, LPMCI_RECORD_PARMS lpParms)
{
#ifdef linux
	int			count;
	int			start, end;
	LPWAVEHDR		lpWaveHdr;
	DWORD		dwRet;
#ifdef DEBUG_MCIWAVE
	printf("WAVE_mciRecord(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (MCIWavDev[wDevID].hFile == 0) {
		printf("WAVE_mciRecord // can't find file='%s' !\n", 
				MCIWavDev[wDevID].openParms.lpstrElementName);
		return MCIERR_FILE_NOT_FOUND;
		}
	start = 1; 		end = 99999;
	if (dwFlags & MCI_FROM) {
		start = lpParms->dwFrom; 
		printf("WAVE_mciRecord // MCI_FROM=%d \n", start);
		}
	if (dwFlags & MCI_TO) {
		end = lpParms->dwTo;
		printf("WAVE_mciRecord // MCI_TO=%d \n", end);
		}
	lpWaveHdr = &MCIWavDev[wDevID].WaveHdr;
	lpWaveHdr->lpData = (LPSTR) malloc(64000);
	lpWaveHdr->dwBufferLength = 32000;
	lpWaveHdr->dwUser = 0L;
	lpWaveHdr->dwFlags = 0L;
	lpWaveHdr->dwLoops = 0L;
	dwRet = widMessage(0, WIDM_PREPARE, 0, (DWORD)lpWaveHdr, sizeof(WAVEHDR));
	printf("WAVE_mciRecord // after WIDM_PREPARE \n");
	while(TRUE) {
		lpWaveHdr->dwBytesRecorded = 0;
		dwRet = widMessage(0, WIDM_START, 0, 0L, 0L);
		printf("WAVE_mciRecord // after WIDM_START lpWaveHdr=%08X dwBytesRecorded=%u\n",
					lpWaveHdr, lpWaveHdr->dwBytesRecorded);
		if (lpWaveHdr->dwBytesRecorded == 0) break;
		}
	printf("WAVE_mciRecord // before WIDM_UNPREPARE \n");
	dwRet = widMessage(0, WIDM_UNPREPARE, 0, (DWORD)lpWaveHdr, sizeof(WAVEHDR));
	printf("WAVE_mciRecord // after WIDM_UNPREPARE \n");
	if (lpWaveHdr->lpData != NULL) {
		free(lpWaveHdr->lpData);
		lpWaveHdr->lpData = NULL;
		}
	if (dwFlags & MCI_NOTIFY) {
#ifdef DEBUG_MCIWAVE
		printf("WAVE_mciRecord // MCI_NOTIFY_SUCCESSFUL %08X !\n", lpParms->dwCallback);
#endif
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			MCIWavDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
	return 0;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
* 				WAVE_mciStop			[internal]
*/
DWORD WAVE_mciStop(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_MCIWAVE
	printf("WAVE_mciStop(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}


/**************************************************************************
* 				WAVE_mciPause			[internal]
*/
DWORD WAVE_mciPause(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_MCIWAVE
	printf("WAVE_mciPause(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}


/**************************************************************************
* 				WAVE_mciResume			[internal]
*/
DWORD WAVE_mciResume(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_MCIWAVE
	printf("WAVE_mciResume(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}


/**************************************************************************
* 				WAVE_mciSet			[internal]
*/
DWORD WAVE_mciSet(UINT wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_MCIWAVE
	printf("WAVE_mciSet(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
#endif
	if (lpParms == NULL) return MCIERR_INTERNAL;
#ifdef DEBUG_MCIWAVE
	printf("WAVE_mciSet // dwTimeFormat=%08X\n", lpParms->dwTimeFormat);
	printf("WAVE_mciSet // dwAudio=%08X\n", lpParms->dwAudio);
#endif
	if (dwFlags & MCI_SET_TIME_FORMAT) {
		switch (lpParms->dwTimeFormat) {
			case MCI_FORMAT_MILLISECONDS:
				printf("WAVE_mciSet // MCI_FORMAT_MILLISECONDS !\n");
				break;
			case MCI_FORMAT_BYTES:
				printf("WAVE_mciSet // MCI_FORMAT_BYTES !\n");
				break;
			case MCI_FORMAT_SAMPLES:
				printf("WAVE_mciSet // MCI_FORMAT_SAMPLES !\n");
				break;
			default:
				printf("WAVE_mciSet // bad time format !\n");
				return MCIERR_BAD_TIME_FORMAT;
			}
		}
	if (dwFlags & MCI_SET_VIDEO) return MCIERR_UNSUPPORTED_FUNCTION;
	if (dwFlags & MCI_SET_DOOR_OPEN) return MCIERR_UNSUPPORTED_FUNCTION;
	if (dwFlags & MCI_SET_DOOR_CLOSED) return MCIERR_UNSUPPORTED_FUNCTION;
	if (dwFlags & MCI_SET_AUDIO) {
		printf("WAVE_mciSet // MCI_SET_AUDIO !\n");
		}
	if (dwFlags && MCI_SET_ON) {
		printf("WAVE_mciSet // MCI_SET_ON !\n");
		if (dwFlags && MCI_SET_AUDIO_LEFT) {
			printf("WAVE_mciSet // MCI_SET_AUDIO_LEFT !\n");
			}
		if (dwFlags && MCI_SET_AUDIO_RIGHT) {
			printf("WAVE_mciSet // MCI_SET_AUDIO_RIGHT !\n");
			}
		}
	if (dwFlags & MCI_SET_OFF) {
		printf("WAVE_mciSet // MCI_SET_OFF !\n");
		}
	if (dwFlags & MCI_WAVE_INPUT) {
		printf("WAVE_mciSet // MCI_WAVE_INPUT !\n");
		}
	if (dwFlags & MCI_WAVE_OUTPUT) {
		printf("WAVE_mciSet // MCI_WAVE_OUTPUT !\n");
		}
	if (dwFlags & MCI_WAVE_SET_ANYINPUT) {
		printf("WAVE_mciSet // MCI_WAVE_SET_ANYINPUT !\n");
		}
	if (dwFlags & MCI_WAVE_SET_ANYOUTPUT) {
		printf("WAVE_mciSet // MCI_WAVE_SET_ANYOUTPUT !\n");
		}
	if (dwFlags & MCI_WAVE_SET_AVGBYTESPERSEC) {
		printf("WAVE_mciSet // MCI_WAVE_SET_AVGBYTESPERSEC !\n");
		}
	if (dwFlags & MCI_WAVE_SET_BITSPERSAMPLE) {
		printf("WAVE_mciSet // MCI_WAVE_SET_BITSPERSAMPLE !\n");
		}
	if (dwFlags & MCI_WAVE_SET_BLOCKALIGN) {
		printf("WAVE_mciSet // MCI_WAVE_SET_BLOCKALIGN !\n");
		}
	if (dwFlags & MCI_WAVE_SET_CHANNELS) {
		printf("WAVE_mciSet // MCI_WAVE_SET_CHANNELS !\n");
		}
	if (dwFlags & MCI_WAVE_SET_FORMATTAG) {
		printf("WAVE_mciSet // MCI_WAVE_SET_FORMATTAG !\n");
		}
	if (dwFlags & MCI_WAVE_SET_SAMPLESPERSEC) {
		printf("WAVE_mciSet // MCI_WAVE_SET_SAMPLESPERSEC !\n");
		}
 	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}


/**************************************************************************
* 				WAVE_mciStatus		[internal]
*/
DWORD WAVE_mciStatus(UINT wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
#ifdef linux
#ifdef DEBUG_MCIWAVE
	printf("WAVE_mciStatus(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
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
				printf("WAVE_mciStatus // MCI_STATUS_MEDIA_PRESENT !\n");
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
				printf("WAVE_mciStatus // MCI_STATUS_READY !\n");
				lpParms->dwReturn = TRUE;
				break;
			case MCI_STATUS_TIME_FORMAT:
				printf("WAVE_mciStatus // MCI_STATUS_TIME_FORMAT !\n");
				lpParms->dwReturn = MCI_FORMAT_MILLISECONDS;
				break;
			case MCI_WAVE_INPUT:
				printf("WAVE_mciStatus // MCI_WAVE_INPUT !\n");
				lpParms->dwReturn = 0;
				break;
			case MCI_WAVE_OUTPUT:
				printf("WAVE_mciStatus // MCI_WAVE_OUTPUT !\n");
				lpParms->dwReturn = 0;
				break;
			case MCI_WAVE_STATUS_AVGBYTESPERSEC:
				printf("WAVE_mciStatus // MCI_WAVE_STATUS_AVGBYTESPERSEC !\n");
				lpParms->dwReturn = 22050;
				break;
			case MCI_WAVE_STATUS_BITSPERSAMPLE:
				printf("WAVE_mciStatus // MCI_WAVE_STATUS_BITSPERSAMPLE !\n");
				lpParms->dwReturn = 8;
				break;
			case MCI_WAVE_STATUS_BLOCKALIGN:
				printf("WAVE_mciStatus // MCI_WAVE_STATUS_BLOCKALIGN !\n");
				lpParms->dwReturn = 1;
				break;
			case MCI_WAVE_STATUS_CHANNELS:
				printf("WAVE_mciStatus // MCI_WAVE_STATUS_CHANNELS !\n");
				lpParms->dwReturn = 1;
				break;
			case MCI_WAVE_STATUS_FORMATTAG:
				printf("WAVE_mciStatus // MCI_WAVE_FORMATTAG !\n");
				lpParms->dwReturn = WAVE_FORMAT_PCM;
				break;
			case MCI_WAVE_STATUS_LEVEL:
				printf("WAVE_mciStatus // MCI_WAVE_STATUS_LEVEL !\n");
				lpParms->dwReturn = 0xAAAA5555;
				break;
			case MCI_WAVE_STATUS_SAMPLESPERSEC:
				printf("WAVE_mciStatus // MCI_WAVE_STATUS_SAMPLESPERSEC !\n");
				lpParms->dwReturn = 22050;
				break;
			default:
				printf("WAVE_mciStatus // unknowm command %04X !\n", lpParms->dwItem);
				return MCIERR_UNRECOGNIZED_COMMAND;
			}
		}
	if (dwFlags & MCI_NOTIFY) {
		printf("WAVE_mciStatus // MCI_NOTIFY_SUCCESSFUL %08X !\n", lpParms->dwCallback);
		mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			MCIWavDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
		}
 	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}

/**************************************************************************
* 				WAVE_mciGetDevCaps		[internal]
*/
DWORD WAVE_mciGetDevCaps(UINT wDevID, DWORD dwFlags, 
					LPMCI_GETDEVCAPS_PARMS lpParms)
{
#ifdef linux
	printf("WAVE_mciGetDevCaps(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
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
				lpParms->dwReturn = MCI_DEVTYPE_WAVEFORM_AUDIO;
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
			case MCI_WAVE_GETDEVCAPS_INPUTS:
				lpParms->dwReturn = 1;
				break;
			case MCI_WAVE_GETDEVCAPS_OUTPUTS:
				lpParms->dwReturn = 1;
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
* 				WAVE_mciInfo			[internal]
*/
DWORD WAVE_mciInfo(UINT wDevID, DWORD dwFlags, LPMCI_INFO_PARMS lpParms)
{
#ifdef linux
	printf("WAVE_mciInfo(%u, %08X, %08X);\n", wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	lpParms->lpstrReturn = NULL;
	switch(dwFlags) {
		case MCI_INFO_PRODUCT:
			lpParms->lpstrReturn = "Linux Sound System 0.5";
			break;
		case MCI_INFO_FILE:
			lpParms->lpstrReturn = "FileName";
			break;
		case MCI_WAVE_INPUT:
			lpParms->lpstrReturn = "Linux Sound System 0.5";
			break;
		case MCI_WAVE_OUTPUT:
			lpParms->lpstrReturn = "Linux Sound System 0.5";
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
* 				wodGetDevCaps				[internal]
*/
DWORD wodGetDevCaps(WORD wDevID, LPWAVEOUTCAPS lpCaps, DWORD dwSize)
{
#ifdef linux
	int 	audio;
	int		smplrate;
	int		samplesize = 16;
	int		dsp_stereo = 1;
	int		bytespersmpl;
	printf("wodGetDevCaps(%u, %08X, %u);\n", wDevID, lpCaps, dwSize);
	if (lpCaps == NULL) return MMSYSERR_NOTENABLED;
	audio = open (SOUND_DEV, O_WRONLY, 0);
	if (audio == -1) return MMSYSERR_NOTENABLED;
	lpCaps->wMid = 0xFF; 	/* Manufac ID */
	lpCaps->wPid = 0x01; 	/* Product ID */
	strcpy(lpCaps->szPname, "Linux WAV Driver");
	lpCaps->dwFormats = 0;
	lpCaps->dwSupport = 0;
	lpCaps->wChannels = (IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo) != 0) ? 1 : 2;
	bytespersmpl = (IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, samplesize) != 0) ? 1 : 2;
	smplrate = 44100;
	if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
		lpCaps->dwFormats |= WAVE_FORMAT_4M08;
		if (lpCaps->wChannels > 1)	lpCaps->dwFormats |= WAVE_FORMAT_4S08;
		if (bytespersmpl > 1) {
			lpCaps->dwFormats |= WAVE_FORMAT_4M16;
			if (lpCaps->wChannels > 1)	lpCaps->dwFormats |= WAVE_FORMAT_4S16;
			}
		}
	smplrate = 22050;
	if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
		lpCaps->dwFormats |= WAVE_FORMAT_2M08;
		if (lpCaps->wChannels > 1)	lpCaps->dwFormats |= WAVE_FORMAT_2S08;
		if (bytespersmpl > 1) {
			lpCaps->dwFormats |= WAVE_FORMAT_2M16;
			if (lpCaps->wChannels > 1)	lpCaps->dwFormats |= WAVE_FORMAT_2S16;
			}
		}
	smplrate = 11025;
	if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
		lpCaps->dwFormats |= WAVE_FORMAT_1M08;
		if (lpCaps->wChannels > 1)	lpCaps->dwFormats |= WAVE_FORMAT_1S08;
		if (bytespersmpl > 1) {
			lpCaps->dwFormats |= WAVE_FORMAT_1M16;
			if (lpCaps->wChannels > 1)	lpCaps->dwFormats |= WAVE_FORMAT_1S16;
			}
		}
	close(audio);
	printf("wodGetDevCaps // dwFormats = %08X\n", lpCaps->dwFormats);
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
* 				wodOpen				[internal]
*/
DWORD wodOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
#ifdef linux
	int 		audio;
	int			abuf_size;
	int			smplrate;
	int			samplesize;
	int			dsp_stereo;
	printf("wodOpen(%u, %08X, %08X);\n", wDevID, lpDesc, dwFlags);
	if (lpDesc == NULL) {
		printf("Linux 'wodOpen' // Invalid Parameter !\n");
		return MMSYSERR_INVALPARAM;
		}
	if (wDevID >= MAX_WAVOUTDRV) {
		printf("Linux 'wodOpen' // MAX_WAVOUTDRV reached !\n");
		return MMSYSERR_ALLOCATED;
		}
	WOutDev[wDevID].unixdev = 0;
	audio = open (SOUND_DEV, O_WRONLY, 0);
	if (audio == -1) {
		printf("Linux 'wodOpen' // can't open !\n");
		return MMSYSERR_NOTENABLED;
		}
	IOCTL(audio, SNDCTL_DSP_GETBLKSIZE, abuf_size);
	if (abuf_size < 4096 || abuf_size > 65536) {
		if (abuf_size == -1)
			printf("Linux 'wodOpen' // IOCTL can't 'SNDCTL_DSP_GETBLKSIZE' !\n");
		else
			printf("Linux 'wodOpen' // SNDCTL_DSP_GETBLKSIZE Invalid bufsize !\n");
		return MMSYSERR_NOTENABLED;
		}
	WOutDev[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
	switch(WOutDev[wDevID].wFlags) {
		case DCB_NULL:
			printf("Linux 'wodOpen' // CALLBACK_NULL !\n");
			break;
		case DCB_WINDOW:
			printf("Linux 'wodOpen' // CALLBACK_WINDOW !\n");
			break;
		case DCB_TASK:
			printf("Linux 'wodOpen' // CALLBACK_TASK !\n");
			break;
		case DCB_FUNCTION:
			printf("Linux 'wodOpen' // CALLBACK_FUNCTION !\n");
			break;
		}
	WOutDev[wDevID].lpQueueHdr = NULL;
	WOutDev[wDevID].unixdev = audio;
	WOutDev[wDevID].dwTotalPlayed = 0;
	WOutDev[wDevID].bufsize = abuf_size;
	memcpy(&WOutDev[wDevID].waveDesc, lpDesc, sizeof(WAVEOPENDESC));
	if (lpDesc->lpFormat->wFormatTag != WAVE_FORMAT_PCM) {
		printf("Linux 'wodOpen' // Bad format %04X !\n", 
						lpDesc->lpFormat->wFormatTag);
		return WAVERR_BADFORMAT;
		}
	memcpy(&WOutDev[wDevID].Format, lpDesc->lpFormat, sizeof(PCMWAVEFORMAT));
	if (WOutDev[wDevID].Format.wf.nChannels == 0) return WAVERR_BADFORMAT;
	if (WOutDev[wDevID].Format.wf.nSamplesPerSec == 0) return WAVERR_BADFORMAT;
	if (WOutDev[wDevID].Format.wBitsPerSample == 0) {
		WOutDev[wDevID].Format.wBitsPerSample = 8 *
		(WOutDev[wDevID].Format.wf.nAvgBytesPerSec /
		WOutDev[wDevID].Format.wf.nSamplesPerSec) /
		WOutDev[wDevID].Format.wf.nChannels;
		}
	samplesize = WOutDev[wDevID].Format.wBitsPerSample;
	smplrate = WOutDev[wDevID].Format.wf.nSamplesPerSec;
	dsp_stereo = (WOutDev[wDevID].Format.wf.nChannels > 1) ? TRUE : FALSE;
	IOCTL(audio, SNDCTL_DSP_SPEED, smplrate);
	IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, samplesize);
	IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo);
	printf("Linux 'wodOpen' // wBitsPerSample=%u !\n", 
				WOutDev[wDevID].Format.wBitsPerSample);
	printf("Linux 'wodOpen' // nSamplesPerSec=%u !\n", 
				WOutDev[wDevID].Format.wf.nSamplesPerSec);
	printf("Linux 'wodOpen' // nChannels=%u !\n", 
				WOutDev[wDevID].Format.wf.nChannels);
	if (WAVE_NotifyClient(wDevID, WOM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
		printf("Linux 'wodOpen' // can't notify client !\n");
		return MMSYSERR_INVALPARAM;
		}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				wodClose			[internal]
*/
DWORD wodClose(WORD wDevID)
{
#ifdef linux
	printf("wodClose(%u);\n", wDevID);
	if (WOutDev[wDevID].unixdev == 0) {
		printf("Linux 'wodClose' // can't close !\n");
		return MMSYSERR_NOTENABLED;
		}
	close(WOutDev[wDevID].unixdev);
	WOutDev[wDevID].unixdev = 0;
	WOutDev[wDevID].bufsize = 0;
	if (WAVE_NotifyClient(wDevID, WOM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
		printf("Linux 'wodClose' // can't notify client !\n");
		return MMSYSERR_INVALPARAM;
		}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				wodWrite			[internal]
*/
DWORD wodWrite(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
#ifdef linux
	printf("wodWrite(%u, %08X, %08X);\n", wDevID, lpWaveHdr, dwSize);
	if (WOutDev[wDevID].unixdev == 0) {
		printf("Linux 'wodWrite' // can't play !\n");
		return MMSYSERR_NOTENABLED;
		}
	if (lpWaveHdr->lpData == NULL) return WAVERR_UNPREPARED;
	if (!(lpWaveHdr->dwFlags & WHDR_PREPARED)) return WAVERR_UNPREPARED;
	if (lpWaveHdr->dwFlags & WHDR_INQUEUE) return WAVERR_STILLPLAYING;
	lpWaveHdr->dwFlags &= ~WHDR_DONE;
	lpWaveHdr->dwFlags |= WHDR_INQUEUE;
	printf("wodWrite() // dwBytesRecorded %u !\n", lpWaveHdr->dwBytesRecorded);
	if (write (WOutDev[wDevID].unixdev, lpWaveHdr->lpData, 
		lpWaveHdr->dwBytesRecorded) != lpWaveHdr->dwBytesRecorded) {
		return MMSYSERR_NOTENABLED;
		}
	lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
	lpWaveHdr->dwFlags |= WHDR_DONE;
	if (WAVE_NotifyClient(wDevID, WOM_DONE, 0L, 0L) != MMSYSERR_NOERROR) {
		printf("Linux 'wodWrite' // can't notify client !\n");
		return MMSYSERR_INVALPARAM;
		}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				wodPrepare			[internal]
*/
DWORD wodPrepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
#ifdef linux
	printf("wodPrepare(%u, %08X, %08X);\n", wDevID, lpWaveHdr, dwSize);
	if (WOutDev[wDevID].unixdev == 0) {
		printf("Linux 'wodPrepare' // can't prepare !\n");
		return MMSYSERR_NOTENABLED;
		}
	if (WOutDev[wDevID].lpQueueHdr != NULL) {
		printf("Linux 'wodPrepare' // already prepare !\n");
		return MMSYSERR_NOTENABLED;
		}
	WOutDev[wDevID].dwTotalPlayed = 0;
	WOutDev[wDevID].lpQueueHdr = lpWaveHdr;
	if (lpWaveHdr->dwFlags & WHDR_INQUEUE) return WAVERR_STILLPLAYING;
	lpWaveHdr->dwFlags |= WHDR_PREPARED;
	lpWaveHdr->dwFlags &= ~WHDR_DONE;
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				wodUnprepare			[internal]
*/
DWORD wodUnprepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
#ifdef linux
	printf("wodUnprepare(%u, %08X, %08X);\n", wDevID, lpWaveHdr, dwSize);
	if (WOutDev[wDevID].unixdev == 0) {
		printf("Linux 'wodUnprepare' // can't unprepare !\n");
		return MMSYSERR_NOTENABLED;
		}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				wodRestart				[internal]
*/
DWORD wodRestart(WORD wDevID)
{
#ifdef linux
	printf("wodRestart(%u);\n", wDevID);
	if (WOutDev[wDevID].unixdev == 0) {
		printf("Linux 'wodRestart' // can't restart !\n");
		return MMSYSERR_NOTENABLED;
		}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				wodReset				[internal]
*/
DWORD wodReset(WORD wDevID)
{
#ifdef linux
	printf("wodReset(%u);\n", wDevID);
	if (WOutDev[wDevID].unixdev == 0) {
		printf("Linux 'wodReset' // can't reset !\n");
		return MMSYSERR_NOTENABLED;
		}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
* 				wodGetPosition			[internal]
*/
DWORD wodGetPosition(WORD wDevID, LPMMTIME lpTime, DWORD uSize)
{
#ifdef linux
	int		time;
	printf("wodGetPosition(%u, %08X, %u);\n", wDevID, lpTime, uSize);
	if (WOutDev[wDevID].unixdev == 0) {
		printf("Linux 'wodGetPosition' // can't get pos !\n");
		return MMSYSERR_NOTENABLED;
		}
	if (lpTime == NULL)	return MMSYSERR_INVALPARAM;
TryAGAIN:
	switch(lpTime->wType) {
		case TIME_BYTES:
			lpTime->u.cb = WOutDev[wDevID].dwTotalPlayed;
			printf("wodGetPosition // TIME_BYTES=%u\n", lpTime->u.cb);
			break;
		case TIME_SAMPLES:
			lpTime->u.sample = WOutDev[wDevID].dwTotalPlayed * 8 /
						WOutDev[wDevID].Format.wBitsPerSample;
			printf("wodGetPosition // TIME_SAMPLES=%u\n", lpTime->u.sample);
			break;
		case TIME_MS:
			lpTime->u.ms = WOutDev[wDevID].dwTotalPlayed /
					(WOutDev[wDevID].Format.wf.nAvgBytesPerSec / 1000);
			printf("wodGetPosition // TIME_MS=%u\n", lpTime->u.ms);
			break;
		case TIME_SMPTE:
			time = WOutDev[wDevID].dwTotalPlayed /
				(WOutDev[wDevID].Format.wf.nAvgBytesPerSec / 1000);
			lpTime->u.smpte.hour = time / 108000;
			time -= lpTime->u.smpte.hour * 108000;
			lpTime->u.smpte.min = time / 1800;
			time -= lpTime->u.smpte.min * 1800;
			lpTime->u.smpte.sec = time / 30;
			time -= lpTime->u.smpte.sec * 30;
			lpTime->u.smpte.frame = time;
			lpTime->u.smpte.fps = 30;
			printf("wodGetPosition // TIME_SMPTE=%02u:%02u:%02u:%02u\n", 
					lpTime->u.smpte.hour, lpTime->u.smpte.min,
					lpTime->u.smpte.sec, lpTime->u.smpte.frame);
			break;
		default:
			printf("wodGetPosition() format not supported ! use TIME_MS !\n");
			lpTime->wType = TIME_MS;
			goto TryAGAIN;
		}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				wodSetVolume			[internal]
*/
DWORD wodSetVolume(WORD wDevID, DWORD dwParam)
{
#ifdef linux
	int 	mixer;
	int		volume = 50;
	printf("wodSetVolume(%u, %08X);\n", wDevID, dwParam);
	if (WOutDev[wDevID].unixdev == 0) {
		printf("Linux 'wodSetVolume' // can't set volume !\n");
		return MMSYSERR_NOTENABLED;
		}
	if ((mixer = open("/dev/mixer", O_RDWR)) < 0) {
		printf("Linux 'wodSetVolume' // mixer device not available !\n");
		return MMSYSERR_NOTENABLED;
		}
    if (ioctl(mixer, SOUND_MIXER_WRITE_PCM, &volume) == -1) {
		printf("Linux 'wodSetVolume' // unable set mixer !\n");
		return MMSYSERR_NOTENABLED;
		}
	close(mixer);
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
* 				wodMessage			[sample driver]
*/
DWORD wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2)
{
	printf("wodMessage(%u, %04X, %08X, %08X, %08X);\n", 
			wDevID, wMsg, dwUser, dwParam1, dwParam2);
	switch(wMsg) {
		case WODM_OPEN:
			return wodOpen(wDevID, (LPWAVEOPENDESC)dwParam1, dwParam2);
		case WODM_CLOSE:
			return wodClose(wDevID);
		case WODM_WRITE:
			return wodWrite(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
		case WODM_PAUSE:
			return 0L;
		case WODM_GETPOS:
			return wodGetPosition(wDevID, (LPMMTIME)dwParam1, dwParam2);
		case WODM_BREAKLOOP:
			return 0L;
		case WODM_PREPARE:
			return wodPrepare(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
		case WODM_UNPREPARE:
			return wodUnprepare(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
		case WODM_GETDEVCAPS:
			return wodGetDevCaps(wDevID, (LPWAVEOUTCAPS)dwParam1, dwParam2);
		case WODM_GETNUMDEVS:
			return 1L;
		case WODM_GETPITCH:
			return 0L;
		case WODM_SETPITCH:
			return 0L;
		case WODM_GETPLAYBACKRATE:
			return 0L;
		case WODM_SETPLAYBACKRATE:
			return 0L;
		case WODM_GETVOLUME:
			return 0L;
		case WODM_SETVOLUME:
			return wodSetVolume(wDevID, dwParam1);
		case WODM_RESTART:
			return wodRestart(wDevID);
		case WODM_RESET:
			return wodReset(wDevID);
		}
	return MMSYSERR_NOTSUPPORTED;
}


/*-----------------------------------------------------------------------*/

/**************************************************************************
* 				widGetDevCaps				[internal]
*/
DWORD widGetDevCaps(WORD wDevID, LPWAVEINCAPS lpCaps, DWORD dwSize)
{
#ifdef linux
	int 	audio;
	int		smplrate;
	int		samplesize = 16;
	int		dsp_stereo = 1;
	int		bytespersmpl;
	printf("widGetDevCaps(%u, %08X, %u);\n", wDevID, lpCaps, dwSize);
	if (lpCaps == NULL) return MMSYSERR_NOTENABLED;
	audio = open (SOUND_DEV, O_RDONLY, 0);
	if (audio == -1) return MMSYSERR_NOTENABLED;
	lpCaps->wMid = 0xFF; 	/* Manufac ID */
	lpCaps->wPid = 0x01; 	/* Product ID */
	strcpy(lpCaps->szPname, "Linux WAV Driver");
	lpCaps->dwFormats = 0;
	lpCaps->wChannels = (IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo) != 0) ? 1 : 2;
	bytespersmpl = (IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, samplesize) != 0) ? 1 : 2;
	smplrate = 44100;
	if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
		lpCaps->dwFormats |= WAVE_FORMAT_4M08;
		if (lpCaps->wChannels > 1)	lpCaps->dwFormats |= WAVE_FORMAT_4S08;
		if (bytespersmpl > 1) {
			lpCaps->dwFormats |= WAVE_FORMAT_4M16;
			if (lpCaps->wChannels > 1)	lpCaps->dwFormats |= WAVE_FORMAT_4S16;
			}
		}
	smplrate = 22050;
	if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
		lpCaps->dwFormats |= WAVE_FORMAT_2M08;
		if (lpCaps->wChannels > 1)	lpCaps->dwFormats |= WAVE_FORMAT_2S08;
		if (bytespersmpl > 1) {
			lpCaps->dwFormats |= WAVE_FORMAT_2M16;
			if (lpCaps->wChannels > 1)	lpCaps->dwFormats |= WAVE_FORMAT_2S16;
			}
		}
	smplrate = 11025;
	if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
		lpCaps->dwFormats |= WAVE_FORMAT_1M08;
		if (lpCaps->wChannels > 1)	lpCaps->dwFormats |= WAVE_FORMAT_1S08;
		if (bytespersmpl > 1) {
			lpCaps->dwFormats |= WAVE_FORMAT_1M16;
			if (lpCaps->wChannels > 1)	lpCaps->dwFormats |= WAVE_FORMAT_1S16;
			}
		}
	close(audio);
	printf("widGetDevCaps // dwFormats = %08X\n", lpCaps->dwFormats);
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
* 				widOpen				[internal]
*/
DWORD widOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
#ifdef linux
	int 		audio;
	int			abuf_size;
	int			smplrate;
	int			samplesize;
	int			dsp_stereo;
	printf("widOpen(%u, %08X, %08X);\n", wDevID, lpDesc, dwFlags);
	if (lpDesc == NULL) {
		printf("Linux 'widOpen' // Invalid Parameter !\n");
		return MMSYSERR_INVALPARAM;
		}
	if (wDevID >= MAX_WAVINDRV) {
		printf("Linux 'widOpen' // MAX_WAVINDRV reached !\n");
		return MMSYSERR_ALLOCATED;
		}
	WInDev[wDevID].unixdev = 0;
	audio = open (SOUND_DEV, O_RDONLY, 0);
	if (audio == -1) {
		printf("Linux 'widOpen' // can't open !\n");
		return MMSYSERR_NOTENABLED;
		}
	IOCTL(audio, SNDCTL_DSP_GETBLKSIZE, abuf_size);
	if (abuf_size < 4096 || abuf_size > 65536) {
		if (abuf_size == -1)
			printf("Linux 'widOpen' // IOCTL can't 'SNDCTL_DSP_GETBLKSIZE' !\n");
		else
			printf("Linux 'widOpen' // SNDCTL_DSP_GETBLKSIZE Invalid bufsize !\n");
		return MMSYSERR_NOTENABLED;
		}
	WInDev[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
	switch(WInDev[wDevID].wFlags) {
		case DCB_NULL:
			printf("Linux 'widOpen' // CALLBACK_NULL !\n");
			break;
		case DCB_WINDOW:
			printf("Linux 'widOpen' // CALLBACK_WINDOW !\n");
			break;
		case DCB_TASK:
			printf("Linux 'widOpen' // CALLBACK_TASK !\n");
			break;
		case DCB_FUNCTION:
			printf("Linux 'widOpen' // CALLBACK_FUNCTION !\n");
			break;
		}
	WInDev[wDevID].lpQueueHdr = NULL;
	WInDev[wDevID].unixdev = audio;
	WInDev[wDevID].bufsize = abuf_size;
	WInDev[wDevID].dwTotalRecorded = 0;
	memcpy(&WInDev[wDevID].waveDesc, lpDesc, sizeof(WAVEOPENDESC));
	if (lpDesc->lpFormat->wFormatTag != WAVE_FORMAT_PCM) {
		printf("Linux 'widOpen' // Bad format %04X !\n", 
						lpDesc->lpFormat->wFormatTag);
		return WAVERR_BADFORMAT;
		}
	memcpy(&WInDev[wDevID].Format, lpDesc->lpFormat, sizeof(PCMWAVEFORMAT));
	WInDev[wDevID].Format.wBitsPerSample = 8; /* <-------------- */
	if (WInDev[wDevID].Format.wf.nChannels == 0) return WAVERR_BADFORMAT;
	if (WInDev[wDevID].Format.wf.nSamplesPerSec == 0) return WAVERR_BADFORMAT;
	if (WInDev[wDevID].Format.wBitsPerSample == 0) {
		WInDev[wDevID].Format.wBitsPerSample = 8 *
		(WInDev[wDevID].Format.wf.nAvgBytesPerSec /
		WInDev[wDevID].Format.wf.nSamplesPerSec) /
		WInDev[wDevID].Format.wf.nChannels;
		}
	samplesize = WInDev[wDevID].Format.wBitsPerSample;
	smplrate = WInDev[wDevID].Format.wf.nSamplesPerSec;
	dsp_stereo = (WInDev[wDevID].Format.wf.nChannels > 1) ? TRUE : FALSE;
	IOCTL(audio, SNDCTL_DSP_SPEED, smplrate);
	IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, samplesize);
	IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo);
#ifdef DEBUG_MCIWAVE
	printf("Linux 'widOpen' // wBitsPerSample=%u !\n", 
				WInDev[wDevID].Format.wBitsPerSample);
	printf("Linux 'widOpen' // nSamplesPerSec=%u !\n", 
				WInDev[wDevID].Format.wf.nSamplesPerSec);
	printf("Linux 'widOpen' // nChannels=%u !\n", 
				WInDev[wDevID].Format.wf.nChannels);
	printf("Linux 'widOpen' // nAvgBytesPerSec=%u\n",
			WInDev[wDevID].Format.wf.nAvgBytesPerSec); 
#endif
	if (WAVE_NotifyClient(wDevID, WIM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
		printf("Linux 'widOpen' // can't notify client !\n");
		return MMSYSERR_INVALPARAM;
		}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				widClose			[internal]
*/
DWORD widClose(WORD wDevID)
{
#ifdef linux
	printf("widClose(%u);\n", wDevID);
	if (WInDev[wDevID].unixdev == 0) {
		printf("Linux 'widClose' // can't close !\n");
		return MMSYSERR_NOTENABLED;
		}
	close(WInDev[wDevID].unixdev);
	WInDev[wDevID].unixdev = 0;
	WInDev[wDevID].bufsize = 0;
	if (WAVE_NotifyClient(wDevID, WIM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
		printf("Linux 'widClose' // can't notify client !\n");
		return MMSYSERR_INVALPARAM;
		}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				widAddBuffer		[internal]
*/
DWORD widAddBuffer(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
#ifdef linux
	int			count	= 1;
	LPWAVEHDR 	lpWIHdr;
	printf("widAddBuffer(%u, %08X, %08X);\n", wDevID, lpWaveHdr, dwSize);
	if (WInDev[wDevID].unixdev == 0) {
		printf("Linux 'widAddBuffer' // can't do it !\n");
		return MMSYSERR_NOTENABLED;
		}
	if (WInDev[wDevID].lpQueueHdr == NULL || 
		!(lpWaveHdr->dwFlags & WHDR_PREPARED)) {
		printf("Linux 'widAddBuffer' // never been prepared !\n");
		return WAVERR_UNPREPARED;
		}
	if ((lpWaveHdr->dwFlags & WHDR_INQUEUE) &&
		(WInDev[wDevID].lpQueueHdr != lpWaveHdr)) {
		/* except if it's the one just prepared ... */
		printf("Linux 'widAddBuffer' // header already in use !\n");
		return WAVERR_STILLPLAYING;
		}
	lpWaveHdr->dwFlags |= WHDR_PREPARED;
	lpWaveHdr->dwFlags |= WHDR_INQUEUE;
	lpWaveHdr->dwFlags &= ~WHDR_DONE;
	lpWaveHdr->dwBytesRecorded = 0;
	/* added to the queue, except if it's the one just prepared ... */
	if (WInDev[wDevID].lpQueueHdr != lpWaveHdr) {
		lpWIHdr = WInDev[wDevID].lpQueueHdr;
		while (lpWIHdr->lpNext != NULL) {
			lpWIHdr = lpWIHdr->lpNext;
			count++;
			}
		lpWIHdr->lpNext = lpWaveHdr;
		count++;
		}
	printf("widAddBuffer // buffer added ! (now %u in queue)\n", count);
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				widPrepare			[internal]
*/
DWORD widPrepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
#ifdef linux
	printf("widPrepare(%u, %08X, %08X);\n", wDevID, lpWaveHdr, dwSize);
	if (WInDev[wDevID].unixdev == 0) {
		printf("Linux 'widPrepare' // can't prepare !\n");
		return MMSYSERR_NOTENABLED;
		}
	if (WInDev[wDevID].lpQueueHdr != NULL) {
		printf("Linux 'widPrepare' // already prepare !\n");
		return WAVERR_BADFORMAT;
		}
	WInDev[wDevID].dwTotalRecorded = 0;
	WInDev[wDevID].lpQueueHdr = lpWaveHdr;
	if (lpWaveHdr->dwFlags & WHDR_INQUEUE) return WAVERR_STILLPLAYING;
	lpWaveHdr->dwFlags |= WHDR_PREPARED;
	lpWaveHdr->dwFlags |= WHDR_INQUEUE;
	lpWaveHdr->dwFlags &= ~WHDR_DONE;
	lpWaveHdr->dwBytesRecorded = 0;
	printf("Linux 'widPrepare' // header prepared !\n");
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				widUnprepare			[internal]
*/
DWORD widUnprepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
#ifdef linux
	printf("widUnprepare(%u, %08X, %08X);\n", wDevID, lpWaveHdr, dwSize);
	if (WInDev[wDevID].unixdev == 0) {
		printf("Linux 'widUnprepare' // can't unprepare !\n");
		return MMSYSERR_NOTENABLED;
		}
	lpWaveHdr->dwFlags &= ~WHDR_PREPARED;
	lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
	lpWaveHdr->dwFlags |= WHDR_DONE;
	WInDev[wDevID].lpQueueHdr = NULL;
	printf("Linux 'widUnprepare' // all headers unprepared !\n");
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				widStart				[internal]
*/
DWORD widStart(WORD wDevID)
{
#ifdef linux
	int			count	= 1;
	LPWAVEHDR 	lpWIHdr;
	printf("widStart(%u);\n", wDevID);
	if (WInDev[wDevID].unixdev == 0) {
		printf("Linux 'widStart' // can't start recording !\n");
		return MMSYSERR_NOTENABLED;
		}
	if (WInDev[wDevID].lpQueueHdr == NULL || 
		WInDev[wDevID].lpQueueHdr->lpData == NULL) {
		printf("Linux 'widStart' // never been prepared !\n");
		return WAVERR_UNPREPARED;
		}
	lpWIHdr = WInDev[wDevID].lpQueueHdr;
	while(lpWIHdr != NULL) {
		lpWIHdr->dwBufferLength &= 0xFFFF;
		printf("widStart // recording buf#%u=%08X size=%u \n", 
			count, lpWIHdr->lpData, lpWIHdr->dwBufferLength);
		fflush(stdout);
		read (WInDev[wDevID].unixdev, lpWIHdr->lpData,
							lpWIHdr->dwBufferLength);
		lpWIHdr->dwBytesRecorded = lpWIHdr->dwBufferLength;
		WInDev[wDevID].dwTotalRecorded += lpWIHdr->dwBytesRecorded;
		lpWIHdr->dwFlags &= ~WHDR_INQUEUE;
		lpWIHdr->dwFlags |= WHDR_DONE;
		if (WAVE_NotifyClient(wDevID, WIM_DATA, (DWORD)lpWIHdr, 0L) != 
			MMSYSERR_NOERROR) {
			printf("Linux 'widStart' // can't notify client !\n");
			return MMSYSERR_INVALPARAM;
			}
		lpWIHdr = lpWIHdr->lpNext;
		count++;
		}
	printf("widStart // end of recording !\n");
	fflush(stdout);
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				widStop					[internal]
*/
DWORD widStop(WORD wDevID)
{
#ifdef linux
	printf("widStop(%u);\n", wDevID);
	if (WInDev[wDevID].unixdev == 0) {
		printf("Linux 'widStop' // can't stop !\n");
		return MMSYSERR_NOTENABLED;
		}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				widReset				[internal]
*/
DWORD widReset(WORD wDevID)
{
#ifdef linux
	printf("widReset(%u);\n", wDevID);
	if (WInDev[wDevID].unixdev == 0) {
		printf("Linux 'widReset' // can't reset !\n");
		return MMSYSERR_NOTENABLED;
		}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				widGetPosition			[internal]
*/
DWORD widGetPosition(WORD wDevID, LPMMTIME lpTime, DWORD uSize)
{
#ifdef linux
	int		time;
#ifdef DEBUG_MCIWAVE
	printf("widGetPosition(%u, %08X, %u);\n", wDevID, lpTime, uSize);
#endif
	if (WInDev[wDevID].unixdev == 0) {
		printf("Linux 'widGetPosition' // can't get pos !\n");
		return MMSYSERR_NOTENABLED;
		}
	if (lpTime == NULL)	return MMSYSERR_INVALPARAM;
TryAGAIN:
#ifdef DEBUG_MCIWAVE
	printf("widGetPosition // wType=%04X !\n", lpTime->wType);
	printf("widGetPosition // wBitsPerSample=%u\n",
			WInDev[wDevID].Format.wBitsPerSample); 
	printf("widGetPosition // nSamplesPerSec=%u\n",
			WInDev[wDevID].Format.wf.nSamplesPerSec); 
	printf("widGetPosition // nChannels=%u\n",
			WInDev[wDevID].Format.wf.nChannels); 
	printf("widGetPosition // nAvgBytesPerSec=%u\n",
			WInDev[wDevID].Format.wf.nAvgBytesPerSec); 
 	fflush(stdout);
#endif
	switch(lpTime->wType) {
		case TIME_BYTES:
			lpTime->u.cb = WInDev[wDevID].dwTotalRecorded;
			printf("widGetPosition // TIME_BYTES=%u\n", lpTime->u.cb);
			break;
		case TIME_SAMPLES:
			lpTime->u.sample = WInDev[wDevID].dwTotalRecorded * 8 /
						WInDev[wDevID].Format.wBitsPerSample;
			printf("widGetPosition // TIME_SAMPLES=%u\n", lpTime->u.sample);
			break;
		case TIME_MS:
			lpTime->u.ms = WInDev[wDevID].dwTotalRecorded /
					(WInDev[wDevID].Format.wf.nAvgBytesPerSec / 1000);
			printf("widGetPosition // TIME_MS=%u\n", lpTime->u.ms);
			break;
		case TIME_SMPTE:
			time = WInDev[wDevID].dwTotalRecorded /
				(WInDev[wDevID].Format.wf.nAvgBytesPerSec / 1000);
			lpTime->u.smpte.hour = time / 108000;
			time -= lpTime->u.smpte.hour * 108000;
			lpTime->u.smpte.min = time / 1800;
			time -= lpTime->u.smpte.min * 1800;
			lpTime->u.smpte.sec = time / 30;
			time -= lpTime->u.smpte.sec * 30;
			lpTime->u.smpte.frame = time;
			lpTime->u.smpte.fps = 30;
			printf("widGetPosition // TIME_SMPTE=%02u:%02u:%02u:%02u\n", 
					lpTime->u.smpte.hour, lpTime->u.smpte.min,
					lpTime->u.smpte.sec, lpTime->u.smpte.frame);
			break;
		default:
			printf("widGetPosition() format not supported ! use TIME_MS !\n");
			lpTime->wType = TIME_MS;
			goto TryAGAIN;
		}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				widMessage			[sample driver]
*/
DWORD widMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2)
{
	printf("widMessage(%u, %04X, %08X, %08X, %08X);\n", 
			wDevID, wMsg, dwUser, dwParam1, dwParam2);
	switch(wMsg) {
		case WIDM_OPEN:
			return widOpen(wDevID, (LPWAVEOPENDESC)dwParam1, dwParam2);
		case WIDM_CLOSE:
			return widClose(wDevID);
		case WIDM_ADDBUFFER:
			return widAddBuffer(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
		case WIDM_PREPARE:
			return widPrepare(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
		case WIDM_UNPREPARE:
			return widUnprepare(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
		case WIDM_GETDEVCAPS:
			return widGetDevCaps(wDevID, (LPWAVEINCAPS)dwParam1, dwParam2);
		case WIDM_GETNUMDEVS:
			return 1L;
		case WIDM_GETPOS:
			return widGetPosition(wDevID, (LPMMTIME)dwParam1, dwParam2);
		case WIDM_RESET:
			return widReset(wDevID);
		case WIDM_START:
			return widStart(wDevID);
		case WIDM_STOP:
			return widStop(wDevID);
		}
	return MMSYSERR_NOTSUPPORTED;
}


/*-----------------------------------------------------------------------*/



/**************************************************************************
* 				midMessage			[sample driver]
*/
DWORD midMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2)
{
	return MMSYSERR_NOTENABLED;
}

/**************************************************************************
* 				modMessage			[sample driver]
*/
DWORD modMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2)
{
	return MMSYSERR_NOTENABLED;
}

#endif /* #ifdef BUILTIN_MMSYSTEM */
