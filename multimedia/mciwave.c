/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*				   
 * Sample Wine Driver for Open Sound System (featured in Linux and FreeBSD)
 *
 * Copyright 1994 Martin Ayotte
 */
/*
 * FIXME:
 *	- record/play should and must be done asynchronous
 *	- segmented/linear pointer problems (lpData in waveheaders,W*_DONE cbs)
 */

#define EMULATE_SB16

#define DEBUG_MCIWAVE

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "winuser.h"
#include "driver.h"
#include "multimedia.h"
#include "mmsystem.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mciwave)

#define MAX_MCIWAVEDRV 	(1)

typedef struct {
    UINT16			wDevID;
    UINT16			wWavID;
    int				nUseCount;	/* Incremented for each shared open */
    BOOL16			fShareable;	/* TRUE if first open was shareable */
    WORD			wNotifyDeviceID;/* MCI device ID with a pending notification */
    HANDLE16			hCallback;	/* Callback handle for pending notification */
    HMMIO			hFile;		/* mmio file handle open as Element */
    MCI_WAVE_OPEN_PARMSA 	openParms;
    WAVEOPENDESC 		waveDesc;
    PCMWAVEFORMAT		WaveFormat;
    WAVEHDR			WaveHdr;
    BOOL16			fInput;		/* FALSE = Output, TRUE = Input */
    WORD			dwStatus;	/* one from MCI_MODE_xxxx */
    DWORD			dwMciTimeFormat;/* One of the supported MCI_FORMAT_xxxx */
    DWORD			dwFileOffset;   /* Offset of chunk in mmio file */
    DWORD			dwLength;	/* number of bytes in chunk for playing */
    DWORD			dwPosition;	/* position in bytes in chunk for playing */
} WINE_MCIWAVE;

static WINE_MCIWAVE	MCIWaveDev[MAX_MCIWAVEDRV];

/*======================================================================*
 *                  	    MCI WAVE implemantation			*
 *======================================================================*/

static DWORD WAVE_mciResume(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);

/**************************************************************************
 * 				MCIWAVE_drvGetDrv		[internal]	
 */
static WINE_MCIWAVE*  WAVE_drvGetDrv(UINT16 wDevID)
{
    int	i;

    for (i = 0; i < MAX_MCIWAVEDRV; i++) {
	if (MCIWaveDev[i].wDevID == wDevID) {
	    return &MCIWaveDev[i];
	}
    }
    return 0;
}

/**************************************************************************
 * 				MCIWAVE_drvOpen			[internal]	
 */
static	DWORD	WAVE_drvOpen(LPSTR str, LPMCI_OPEN_DRIVER_PARMSA modp)
{
    int	i;

    for (i = 0; i < MAX_MCIWAVEDRV; i++) {
	if (MCIWaveDev[i].wDevID == 0) {
	    MCIWaveDev[i].wDevID = modp->wDeviceID;
	    modp->wCustomCommandTable = -1;
	    modp->wType = MCI_DEVTYPE_WAVEFORM_AUDIO;
	    return modp->wDeviceID;
	}
    }
    return 0;
}

/**************************************************************************
 * 				MCIWAVE_drvClose		[internal]	
 */
static	DWORD	WAVE_drvClose(DWORD dwDevID)
{
    WINE_MCIWAVE*  wmcda = WAVE_drvGetDrv(dwDevID);

    if (wmcda) {
	wmcda->wDevID = 0;
	return 1;
    }
    return 0;
}

/**************************************************************************
 * 				WAVE_mciGetOpenDev		[internal]	
 */
static WINE_MCIWAVE*  WAVE_mciGetOpenDev(UINT16 wDevID)
{
    WINE_MCIWAVE*	wmw = WAVE_drvGetDrv(wDevID);
    
    if (wmw == NULL || wmw->nUseCount == 0) {
	WARN("Invalid wDevID=%u\n", wDevID);
	return 0;
    }
    return wmw;
}

static	DWORD 	WAVE_ConvertByteToTimeFormat(WINE_MCIWAVE* wmw, DWORD val)
{
    DWORD	ret = 0;
    
    switch (wmw->dwMciTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
	ret = (val * 1000) / wmw->WaveFormat.wf.nAvgBytesPerSec;
	break;
    case MCI_FORMAT_BYTES:
	ret = val;
	break;
    case MCI_FORMAT_SAMPLES: /* FIXME: is this correct ? */
	ret = (val * 8) / wmw->WaveFormat.wBitsPerSample;
	break;
    default:
	WARN("Bad time format %lu!\n", wmw->dwMciTimeFormat);
    }
    TRACE("val=%lu=0x%08lx [tf=%lu] => ret=%lu\n", val, val, wmw->dwMciTimeFormat, ret);
    return ret;
}

static	DWORD 	WAVE_ConvertTimeFormatToByte(WINE_MCIWAVE* wmw, DWORD val)
{
    DWORD	ret = 0;
    
    switch (wmw->dwMciTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
	ret = (val * wmw->WaveFormat.wf.nAvgBytesPerSec) / 1000;
	break;
    case MCI_FORMAT_BYTES:
	ret = val;
	break;
    case MCI_FORMAT_SAMPLES: /* FIXME: is this correct ? */
	ret = (val * wmw->WaveFormat.wBitsPerSample) / 8;
	break;
    default:
	WARN("Bad time format %lu!\n", wmw->dwMciTimeFormat);
    }
    TRACE("val=%lu=0x%08lx [tf=%lu] => ret=%lu\n", val, val, wmw->dwMciTimeFormat, ret);
    return ret;
}

static	DWORD WAVE_mciReadFmt(WINE_MCIWAVE* wmw, MMCKINFO* pckMainRIFF)
{
    MMCKINFO	mmckInfo;
    
    mmckInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if (mmioDescend(wmw->hFile, &mmckInfo, pckMainRIFF, MMIO_FINDCHUNK) != 0)
	return MCIERR_INVALID_FILE;
    TRACE("Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX \n",
	  (LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType, mmckInfo.cksize);
    if (mmioRead(wmw->hFile, (HPSTR)&wmw->WaveFormat,
		 (long)sizeof(PCMWAVEFORMAT)) != (long)sizeof(PCMWAVEFORMAT))
	return MCIERR_INVALID_FILE;
    
    TRACE("wFormatTag=%04X !\n",   wmw->WaveFormat.wf.wFormatTag);
    TRACE("nChannels=%d \n",       wmw->WaveFormat.wf.nChannels);
    TRACE("nSamplesPerSec=%ld\n",  wmw->WaveFormat.wf.nSamplesPerSec);
    TRACE("nAvgBytesPerSec=%ld\n", wmw->WaveFormat.wf.nAvgBytesPerSec);
    TRACE("nBlockAlign=%d \n",     wmw->WaveFormat.wf.nBlockAlign);
    TRACE("wBitsPerSample=%u !\n", wmw->WaveFormat.wBitsPerSample);
    mmckInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioDescend(wmw->hFile, &mmckInfo, pckMainRIFF, MMIO_FINDCHUNK) != 0)
	return MCIERR_INVALID_FILE;
    TRACE("Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX \n",
	  (LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType, mmckInfo.cksize);
    TRACE("nChannels=%d nSamplesPerSec=%ld\n",
	  wmw->WaveFormat.wf.nChannels, wmw->WaveFormat.wf.nSamplesPerSec);
    wmw->dwLength = mmckInfo.cksize;
    wmw->dwFileOffset = mmioSeek(wmw->hFile, 0, SEEK_CUR); /* >= 0 */
    return 0;
}

/**************************************************************************
 * 			WAVE_mciOpen	                        [internal]
 */
static DWORD WAVE_mciOpen(UINT16 wDevID, DWORD dwFlags, LPMCI_WAVE_OPEN_PARMSA lpOpenParms)
{
    DWORD		dwRet = 0;
    DWORD		dwDeviceID;
    WINE_MCIWAVE*	wmw = WAVE_drvGetDrv(wDevID);
    
    TRACE("(%04X, %08lX, %p)\n", wDevID, dwFlags, lpOpenParms);
    if (lpOpenParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmw == NULL) 		return MCIERR_INVALID_DEVICE_ID;

    if (dwFlags & MCI_OPEN_SHAREABLE)
	return MCIERR_HARDWARE;
    
    if (wmw->nUseCount > 0) {
	/* The driver is already opened on this channel
	 * Wave driver cannot be shared
	 */
	return MCIERR_DEVICE_OPEN;
    }
    wmw->nUseCount++;
    
    dwDeviceID = lpOpenParms->wDeviceID;
    
    wmw->fInput = FALSE;
    wmw->wWavID = 0;

    TRACE("wDevID=%04X (lpParams->wDeviceID=%08lX)\n", wDevID, dwDeviceID);
    
    if (dwFlags & MCI_OPEN_ELEMENT) {
	if (dwFlags & MCI_OPEN_ELEMENT_ID) {
	    /* could it be that (DWORD)lpOpenParms->lpstrElementName 
	     * contains the hFile value ? 
	     */
	    dwRet = MCIERR_UNRECOGNIZED_COMMAND;
	} else {
	    LPCSTR	lpstrElementName = lpOpenParms->lpstrElementName;
	    
	    /*FIXME : what should be done id wmw->hFile is already != 0, or the driver is playin' */
	    TRACE("MCI_OPEN_ELEMENT '%s' !\n", lpstrElementName);
	    if (lpstrElementName && (strlen(lpstrElementName) > 0)) {
		wmw->hFile = mmioOpenA((LPSTR)lpstrElementName, NULL, 
				       MMIO_ALLOCBUF | MMIO_READWRITE | MMIO_DENYWRITE);
		if (wmw->hFile == 0) {
		    WARN("can't find file='%s' !\n", lpstrElementName);
		    dwRet = MCIERR_FILE_NOT_FOUND;
		}
	    } else {
		wmw->hFile = 0;
	    }
	}
    }
    TRACE("hFile=%u\n", wmw->hFile);
    
    memcpy(&wmw->openParms, lpOpenParms, sizeof(MCI_WAVE_OPEN_PARMSA));
    wmw->wNotifyDeviceID = dwDeviceID;
    wmw->dwStatus = MCI_MODE_NOT_READY;	/* while loading file contents */
    
    wmw->waveDesc.hWave = 0;
    
    if (dwRet == 0 && wmw->hFile != 0) {
	MMCKINFO	ckMainRIFF;
	
	if (mmioDescend(wmw->hFile, &ckMainRIFF, NULL, 0) != 0) {
	    dwRet = MCIERR_INVALID_FILE;
	} else {
	    TRACE("ParentChunk ckid=%.4s fccType=%.4s cksize=%08lX \n",
		  (LPSTR)&ckMainRIFF.ckid, (LPSTR)&ckMainRIFF.fccType, ckMainRIFF.cksize);
	    if ((ckMainRIFF.ckid != FOURCC_RIFF) ||
		(ckMainRIFF.fccType != mmioFOURCC('W', 'A', 'V', 'E'))) {
		dwRet = MCIERR_INVALID_FILE;
	    } else {
		dwRet = WAVE_mciReadFmt(wmw, &ckMainRIFF);
	    }
	}
    } else {
	wmw->dwLength = 0;
    }
    if (dwRet == 0) {
	wmw->WaveFormat.wf.nAvgBytesPerSec = 
	    wmw->WaveFormat.wf.nSamplesPerSec * wmw->WaveFormat.wf.nBlockAlign;
	wmw->waveDesc.lpFormat = (LPWAVEFORMAT)&wmw->WaveFormat;
	wmw->dwPosition = 0;
	
	wmw->dwStatus = MCI_MODE_STOP;
    } else {
	wmw->nUseCount--;
	if (wmw->hFile != 0)
	    mmioClose(wmw->hFile, 0);
	wmw->hFile = 0;
    }
    return dwRet;
}

/**************************************************************************
 *                               WAVE_mciCue             [internal]
 */
static DWORD WAVE_mciCue(UINT16 wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
    /*
      FIXME
      
      This routine is far from complete. At the moment only a check is done on the
      MCI_WAVE_INPUT flag. No explicit check on MCI_WAVE_OUTPUT is done since that
      is the default.
      
      The flags MCI_NOTIFY (and the callback parameter in lpParms) and MCI_WAIT
      are ignored
    */
    
    DWORD		dwRet;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    
    TRACE("(%u, %08lX, %p);\n", wDevID, dwParam, lpParms);
    
    if (wmw == NULL)	return MCIERR_INVALID_DEVICE_ID;
    
    /* always close elements ? */    
    if (wmw->hFile != 0) {
	mmioClose(wmw->hFile, 0);
	wmw->hFile = 0;
    }
    
    dwRet = MMSYSERR_NOERROR;  /* assume success */
    
    if ((dwParam & MCI_WAVE_INPUT) && !wmw->fInput) {
	dwRet = wodMessage(wmw->wWavID, WODM_CLOSE, 0, 0L, 0L);
	if (dwRet != MMSYSERR_NOERROR) return MCIERR_INTERNAL;
	dwRet = widMessage(wmw->wWavID, WIDM_OPEN, 0, (DWORD)&wmw->waveDesc, CALLBACK_NULL);
	wmw->fInput = TRUE;
    } else if (wmw->fInput) {
	dwRet = widMessage(wmw->wWavID, WIDM_CLOSE, 0, 0L, 0L);
	if (dwRet != MMSYSERR_NOERROR) return MCIERR_INTERNAL;
	dwRet = wodMessage(wmw->wWavID, WODM_OPEN, 0, (DWORD)&wmw->waveDesc, CALLBACK_NULL);
	wmw->fInput = FALSE;
    }
    return (dwRet == MMSYSERR_NOERROR) ? 0 : MCIERR_INTERNAL;
}

/**************************************************************************
 * 				WAVE_mciStop			[internal]
 */
static DWORD WAVE_mciStop(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD 		dwRet;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    
    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    wmw->dwStatus = MCI_MODE_STOP;
    wmw->dwPosition = 0;
    TRACE("wmw->dwStatus=%d\n", wmw->dwStatus);
    
    if (wmw->fInput)
	dwRet = widMessage(wmw->wWavID, WIDM_RESET, 0, dwFlags, (DWORD)lpParms);
    else
	dwRet = wodMessage(wmw->wWavID, WODM_RESET, 0, dwFlags, (DWORD)lpParms);
    
    if (dwFlags & MCI_NOTIFY) {
	TRACE("MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmw->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    
    return (dwRet == MMSYSERR_NOERROR) ? 0 : MCIERR_INTERNAL;
}

/**************************************************************************
 *				WAVE_mciClose		[internal]
 */
static DWORD WAVE_mciClose(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD		dwRet = 0;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    
    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (wmw->dwStatus != MCI_MODE_STOP) {
	dwRet = WAVE_mciStop(wDevID, MCI_WAIT, lpParms);
    }
    
    wmw->nUseCount--;
    
    if (wmw->nUseCount == 0) {
	DWORD	mmRet;
	if (wmw->hFile != 0) {
	    mmioClose(wmw->hFile, 0);
	    wmw->hFile = 0;
	}
	mmRet = (wmw->fInput) ? widMessage(wmw->wWavID, WIDM_CLOSE, 0, 0L, 0L) :
	    wodMessage(wmw->wWavID, WODM_CLOSE, 0, 0L, 0L);
	
	if (mmRet != MMSYSERR_NOERROR) dwRet = MCIERR_INTERNAL;
    }
    
    if ((dwFlags & MCI_NOTIFY) && lpParms) {
	TRACE("MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmw->wNotifyDeviceID,
			  (dwRet == 0) ? MCI_NOTIFY_SUCCESSFUL : MCI_NOTIFY_FAILURE);
    }
    return 0;
}

/**************************************************************************
 * 				WAVE_mciPlay		[internal]
 */
static DWORD WAVE_mciPlay(UINT16 wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
    DWORD		end;
    LONG		bufsize, count;
    HGLOBAL16		hData;
    DWORD		dwRet;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    
    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    
    if (wmw->fInput) {
	WARN("cannot play on input device\n");
	return MCIERR_NONAPPLICABLE_FUNCTION;
    }
    
    if (wmw->hFile == 0) {
	WARN("Can't play: no file='%s' !\n", wmw->openParms.lpstrElementName);
	return MCIERR_FILE_NOT_FOUND;
    }
    
    if (!(dwFlags & MCI_WAIT)) {
	return MCI_SendCommandAsync(wmw->wNotifyDeviceID, MCI_PLAY, dwFlags, 
				    (DWORD)lpParms, sizeof(MCI_PLAY_PARMS));
    }

    if (wmw->dwStatus != MCI_MODE_STOP) {
	if (wmw->dwStatus == MCI_MODE_PAUSE) {
	    /* FIXME: parameters (start/end) in lpParams may not be used */
	    return WAVE_mciResume(wDevID, dwFlags, (LPMCI_GENERIC_PARMS)lpParms);
	}
	return MCIERR_INTERNAL;
    }

    end = 0xFFFFFFFF;
    if (lpParms && (dwFlags & MCI_FROM)) {
	wmw->dwPosition = WAVE_ConvertTimeFormatToByte(wmw, lpParms->dwFrom); 
    }
    if (lpParms && (dwFlags & MCI_TO)) {
	end = WAVE_ConvertTimeFormatToByte(wmw, lpParms->dwTo);
    }
    
    TRACE("Playing from byte=%lu to byte=%lu\n", wmw->dwPosition, end);
    
    /* go back to begining of chunk */
    mmioSeek(wmw->hFile, wmw->dwFileOffset, SEEK_SET); /* >= 0 */
    
    /* By default the device will be opened for output, the MCI_CUE function is there to
     * change from output to input and back
     */
    /* FIXME: how to choose between several output channels ? here 0 is forced */
    dwRet = wodMessage(0, WODM_OPEN, 0, (DWORD)&wmw->waveDesc, CALLBACK_NULL);
    if (dwRet != 0) {
	TRACE("Can't open low level audio device %ld\n", dwRet);
	return MCIERR_DEVICE_OPEN;
    }

    /* at 22050 bytes per sec => 30 ms by block */
    bufsize = 10240;
    hData = GlobalAlloc16(GMEM_MOVEABLE, bufsize);
    wmw->WaveHdr.lpData = (LPSTR)GlobalLock16(hData);
    
    wmw->dwStatus = MCI_MODE_PLAY;
    
    /* FIXME: this doesn't work if wmw->dwPosition != 0 */
    /* FIXME: use several WaveHdr for smoother playback */
    /* FIXME: use only regular MMSYS functions, not calling directly the driver */
    while (wmw->dwStatus != MCI_MODE_STOP) {
	wmw->WaveHdr.dwUser = 0L;
	wmw->WaveHdr.dwFlags = 0L;
	wmw->WaveHdr.dwLoops = 0L;
	count = mmioRead(wmw->hFile, wmw->WaveHdr.lpData, bufsize);
	TRACE("mmioRead bufsize=%ld count=%ld\n", bufsize, count);
	if (count < 1) 
	    break;
	dwRet = wodMessage(wmw->wWavID, WODM_PREPARE, 0, (DWORD)&wmw->WaveHdr, sizeof(WAVEHDR));
	wmw->WaveHdr.dwBufferLength = count;
	wmw->WaveHdr.dwBytesRecorded = 0;
	/* FIXME */
	wmw->WaveHdr.reserved = (DWORD)&wmw->WaveHdr;
	TRACE("before WODM_WRITE lpWaveHdr=%p dwBufferLength=%lu dwBytesRecorded=%lu\n",
	      &wmw->WaveHdr, wmw->WaveHdr.dwBufferLength, wmw->WaveHdr.dwBytesRecorded);
	dwRet = wodMessage(wmw->wWavID, WODM_WRITE, 0, (DWORD)&wmw->WaveHdr, sizeof(WAVEHDR));
	/* FIXME: should use callback mechanisms from audio driver */
#if 1
	while (!(wmw->WaveHdr.dwFlags & WHDR_DONE))
	    Sleep(1);
#endif
	wmw->dwPosition += count;
	TRACE("after WODM_WRITE dwPosition=%lu\n", wmw->dwPosition);
	dwRet = wodMessage(wmw->wWavID, WODM_UNPREPARE, 0, (DWORD)&wmw->WaveHdr, sizeof(WAVEHDR));
    }
    
    if (wmw->WaveHdr.lpData != NULL) {
	GlobalUnlock16(hData);
	GlobalFree16(hData);
	wmw->WaveHdr.lpData = NULL;
    }

    wodMessage(wmw->wWavID, WODM_RESET,  0, 0L, 0L);
    wodMessage(wmw->wWavID, WODM_CLOSE, 0, 0L, 0L);

    wmw->dwStatus = MCI_MODE_STOP;
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE("MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmw->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				WAVE_mciRecord			[internal]
 */
static DWORD WAVE_mciRecord(UINT16 wDevID, DWORD dwFlags, LPMCI_RECORD_PARMS lpParms)
{
    int		       	start, end;
    LONG		bufsize;
    HGLOBAL16		hData;
    LPWAVEHDR		lpWaveHdr;
    DWORD		dwRet;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    
    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    
    if (!wmw->fInput) {
	WARN("cannot record on output device\n");
	return MCIERR_NONAPPLICABLE_FUNCTION;
    }
    
    if (wmw->hFile == 0) {
	WARN("can't find file='%s' !\n", 
	     wmw->openParms.lpstrElementName);
	return MCIERR_FILE_NOT_FOUND;
    }
    start = 1; 	end = 99999;
    if (dwFlags & MCI_FROM) {
	start = lpParms->dwFrom; 
	TRACE("MCI_FROM=%d \n", start);
    }
    if (dwFlags & MCI_TO) {
	end = lpParms->dwTo;
	TRACE("MCI_TO=%d \n", end);
    }
    bufsize = 64000;
    lpWaveHdr = &wmw->WaveHdr;
    hData = GlobalAlloc16(GMEM_MOVEABLE, bufsize);
    lpWaveHdr->lpData = (LPSTR)GlobalLock16(hData);
    lpWaveHdr->dwBufferLength = bufsize;
    lpWaveHdr->dwUser = 0L;
    lpWaveHdr->dwFlags = 0L;
    lpWaveHdr->dwLoops = 0L;
    dwRet = widMessage(wmw->wWavID, WIDM_PREPARE, 0, (DWORD)lpWaveHdr, sizeof(WAVEHDR));
    TRACE("after WIDM_PREPARE \n");
    while (TRUE) {
	lpWaveHdr->dwBytesRecorded = 0;
	dwRet = widMessage(wmw->wWavID, WIDM_START, 0, 0L, 0L);
	TRACE("after WIDM_START lpWaveHdr=%p dwBytesRecorded=%lu\n",
	      lpWaveHdr, lpWaveHdr->dwBytesRecorded);
	if (lpWaveHdr->dwBytesRecorded == 0) break;
    }
    TRACE("before WIDM_UNPREPARE \n");
    dwRet = widMessage(wmw->wWavID, WIDM_UNPREPARE, 0, (DWORD)lpWaveHdr, sizeof(WAVEHDR));
    TRACE("after WIDM_UNPREPARE \n");
    if (lpWaveHdr->lpData != NULL) {
	GlobalUnlock16(hData);
	GlobalFree16(hData);
	lpWaveHdr->lpData = NULL;
    }
    if (dwFlags & MCI_NOTIFY) {
	TRACE("MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmw->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				WAVE_mciPause			[internal]
 */
static DWORD WAVE_mciPause(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD 		dwRet;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    
    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (wmw->dwStatus == MCI_MODE_PLAY) {
	wmw->dwStatus = MCI_MODE_PAUSE;
    } 
    
    if (wmw->fInput)	dwRet = widMessage(wmw->wWavID, WIDM_PAUSE, 0, 0L, 0L);
    else		dwRet = wodMessage(wmw->wWavID, WODM_PAUSE, 0, 0L, 0L);
    
    return (dwRet == MMSYSERR_NOERROR) ? 0 : MCIERR_INTERNAL;
}

/**************************************************************************
 * 				WAVE_mciResume			[internal]
 */
static DWORD WAVE_mciResume(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    DWORD		dwRet = 0;
    
    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (wmw->dwStatus == MCI_MODE_PAUSE) {
	wmw->dwStatus = MCI_MODE_PLAY;
    } 
    
    /* FIXME: I doubt WIDM_START is correct */
    if (wmw->fInput)	dwRet = widMessage(wmw->wWavID, WIDM_START, 0, 0L, 0L);
    else		dwRet = wodMessage(wmw->wWavID, WODM_RESTART, 0, 0L, 0L);
    return (dwRet == MMSYSERR_NOERROR) ? 0 : MCIERR_INTERNAL;
}

/**************************************************************************
 * 				WAVE_mciSeek			[internal]
 */
static DWORD WAVE_mciSeek(UINT16 wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
    DWORD		ret = 0;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    
    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL) {
	ret = MCIERR_NULL_PARAMETER_BLOCK;
    } else if (wmw == NULL) {
	ret = MCIERR_INVALID_DEVICE_ID;
    } else {
	WAVE_mciStop(wDevID, MCI_WAIT, 0);
	
	if (dwFlags & MCI_SEEK_TO_START) {
	    wmw->dwPosition = 0;
	} else if (dwFlags & MCI_SEEK_TO_END) {
	    wmw->dwPosition = 0xFFFFFFFF; /* fixme */
	} else if (dwFlags & MCI_TO) {
	    wmw->dwPosition = WAVE_ConvertTimeFormatToByte(wmw, lpParms->dwTo);
	} else {
	    WARN("dwFlag doesn't tell where to seek to...\n");
	    return MCIERR_MISSING_PARAMETER;
	}
	
	TRACE("Seeking to position=%lu bytes\n", wmw->dwPosition);
	
	if (dwFlags & MCI_NOTIFY) {
	    TRACE("MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	    mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			      wmw->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
	}
    }
    return ret;	
}    

/**************************************************************************
 * 				WAVE_mciSet			[internal]
 */
static DWORD WAVE_mciSet(UINT16 wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
{
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    
    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (dwFlags & MCI_SET_TIME_FORMAT) {
	switch (lpParms->dwTimeFormat) {
	case MCI_FORMAT_MILLISECONDS:
	    TRACE("MCI_FORMAT_MILLISECONDS !\n");
	    wmw->dwMciTimeFormat = MCI_FORMAT_MILLISECONDS;
	    break;
	case MCI_FORMAT_BYTES:
	    TRACE("MCI_FORMAT_BYTES !\n");
	    wmw->dwMciTimeFormat = MCI_FORMAT_BYTES;
	    break;
	case MCI_FORMAT_SAMPLES:
	    TRACE("MCI_FORMAT_SAMPLES !\n");
	    wmw->dwMciTimeFormat = MCI_FORMAT_SAMPLES;
	    break;
	default:
	    WARN("Bad time format %lu!\n", lpParms->dwTimeFormat);
	    return MCIERR_BAD_TIME_FORMAT;
	}
    }
    if (dwFlags & MCI_SET_VIDEO) {
	TRACE("No support for video !\n");
	return MCIERR_UNSUPPORTED_FUNCTION;
    }
    if (dwFlags & MCI_SET_DOOR_OPEN) {
	TRACE("No support for door open !\n");
	return MCIERR_UNSUPPORTED_FUNCTION;
    }
    if (dwFlags & MCI_SET_DOOR_CLOSED) {
	TRACE("No support for door close !\n");
	return MCIERR_UNSUPPORTED_FUNCTION;
    }
    if (dwFlags & MCI_SET_AUDIO) {
	if (dwFlags & MCI_SET_ON) {
	    TRACE("MCI_SET_ON audio !\n");
	} else if (dwFlags & MCI_SET_OFF) {
	    TRACE("MCI_SET_OFF audio !\n");
	} else {
	    WARN("MCI_SET_AUDIO without SET_ON or SET_OFF\n");
	    return MCIERR_BAD_INTEGER;
	}
	
	if (lpParms->dwAudio & MCI_SET_AUDIO_ALL)
	    TRACE("MCI_SET_AUDIO_ALL !\n");
	if (lpParms->dwAudio & MCI_SET_AUDIO_LEFT)
	    TRACE("MCI_SET_AUDIO_LEFT !\n");
	if (lpParms->dwAudio & MCI_SET_AUDIO_RIGHT)
	    TRACE("MCI_SET_AUDIO_RIGHT !\n");
    }
    if (dwFlags & MCI_WAVE_INPUT) 
	TRACE("MCI_WAVE_INPUT !\n");
    if (dwFlags & MCI_WAVE_OUTPUT) 
	TRACE("MCI_WAVE_OUTPUT !\n");
    if (dwFlags & MCI_WAVE_SET_ANYINPUT) 
	TRACE("MCI_WAVE_SET_ANYINPUT !\n");
    if (dwFlags & MCI_WAVE_SET_ANYOUTPUT) 
	TRACE("MCI_WAVE_SET_ANYOUTPUT !\n");
    if (dwFlags & MCI_WAVE_SET_AVGBYTESPERSEC) 
	TRACE("MCI_WAVE_SET_AVGBYTESPERSEC !\n");
    if (dwFlags & MCI_WAVE_SET_BITSPERSAMPLE) 
	TRACE("MCI_WAVE_SET_BITSPERSAMPLE !\n");
    if (dwFlags & MCI_WAVE_SET_BLOCKALIGN) 
	TRACE("MCI_WAVE_SET_BLOCKALIGN !\n");
    if (dwFlags & MCI_WAVE_SET_CHANNELS) 
	TRACE("MCI_WAVE_SET_CHANNELS !\n");
    if (dwFlags & MCI_WAVE_SET_FORMATTAG) 
	TRACE("MCI_WAVE_SET_FORMATTAG !\n");
    if (dwFlags & MCI_WAVE_SET_SAMPLESPERSEC) 
	TRACE("MCI_WAVE_SET_SAMPLESPERSEC !\n");
    return 0;
}

/**************************************************************************
 * 				WAVE_mciStatus		[internal]
 */
static DWORD WAVE_mciStatus(UINT16 wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    
    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (dwFlags & MCI_STATUS_ITEM) {
	switch(lpParms->dwItem) {
	case MCI_STATUS_CURRENT_TRACK:
	    lpParms->dwReturn = 1;
	    TRACE("MCI_STATUS_CURRENT_TRACK => %lu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_LENGTH:
	    /* only one track in file is currently handled, so don't take care of MCI_TRACK flag */
	    lpParms->dwReturn = WAVE_ConvertByteToTimeFormat(wmw, wmw->dwLength);
	    TRACE("MCI_STATUS_LENGTH => %lu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_MODE:
	    lpParms->dwReturn = wmw->dwStatus;
	    TRACE("MCI_STATUS_MODE => %lu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_MEDIA_PRESENT:
	    TRACE("MCI_STATUS_MEDIA_PRESENT => TRUE!\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_STATUS_NUMBER_OF_TRACKS:
	    /* only one track in file is currently handled, so don't take care of MCI_TRACK flag */
	    lpParms->dwReturn = 1;
	    TRACE("MCI_STATUS_NUMBER_OF_TRACKS => %lu!\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_POSITION:
	    /* only one track in file is currently handled, so don't take care of MCI_TRACK flag */
	    lpParms->dwReturn = WAVE_ConvertByteToTimeFormat(wmw, 
							     (dwFlags & MCI_STATUS_START) ? 0 : wmw->dwPosition);
	    TRACE("MCI_STATUS_POSITION %s => %lu\n", 
		  (dwFlags & MCI_STATUS_START) ? "start" : "current", lpParms->dwReturn);
	    break;
	case MCI_STATUS_READY:
	    lpParms->dwReturn = (wmw->dwStatus != MCI_MODE_NOT_READY);
	    TRACE("MCI_STATUS_READY => %lu!\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_TIME_FORMAT:
	    lpParms->dwReturn = wmw->dwMciTimeFormat;
	    TRACE("MCI_STATUS_TIME_FORMAT => %lu\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_INPUT:
	    TRACE("MCI_WAVE_INPUT !\n");
	    lpParms->dwReturn = 0;
	    break;
	case MCI_WAVE_OUTPUT:
	    TRACE("MCI_WAVE_OUTPUT !\n");
	    lpParms->dwReturn = 0;
	    break;
	case MCI_WAVE_STATUS_AVGBYTESPERSEC:
	    lpParms->dwReturn = wmw->WaveFormat.wf.nAvgBytesPerSec;
	    TRACE("MCI_WAVE_STATUS_AVGBYTESPERSEC => %lu!\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_STATUS_BITSPERSAMPLE:
	    lpParms->dwReturn = wmw->WaveFormat.wBitsPerSample;
	    TRACE("MCI_WAVE_STATUS_BITSPERSAMPLE => %lu!\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_STATUS_BLOCKALIGN:
	    lpParms->dwReturn = wmw->WaveFormat.wf.nBlockAlign;
	    TRACE("MCI_WAVE_STATUS_BLOCKALIGN => %lu!\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_STATUS_CHANNELS:
	    lpParms->dwReturn = wmw->WaveFormat.wf.nChannels;
	    TRACE("MCI_WAVE_STATUS_CHANNELS => %lu!\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_STATUS_FORMATTAG:
	    lpParms->dwReturn = wmw->WaveFormat.wf.wFormatTag;
	    TRACE("MCI_WAVE_FORMATTAG => %lu!\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_STATUS_LEVEL:
	    TRACE("MCI_WAVE_STATUS_LEVEL !\n");
	    lpParms->dwReturn = 0xAAAA5555;
	    break;
	case MCI_WAVE_STATUS_SAMPLESPERSEC:
	    lpParms->dwReturn = wmw->WaveFormat.wf.nSamplesPerSec;
	    TRACE("MCI_WAVE_STATUS_SAMPLESPERSEC => %lu!\n", lpParms->dwReturn);
	    break;
	default:
	    WARN("unknown command %08lX !\n", lpParms->dwItem);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    }
    if (dwFlags & MCI_NOTIFY) {
	TRACE("MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmw->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				WAVE_mciGetDevCaps		[internal]
 */
static DWORD WAVE_mciGetDevCaps(UINT16 wDevID, DWORD dwFlags, 
				LPMCI_GETDEVCAPS_PARMS lpParms)
{
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    
    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (dwFlags & MCI_GETDEVCAPS_ITEM) {
	switch(lpParms->dwItem) {
	case MCI_GETDEVCAPS_DEVICE_TYPE:
	    lpParms->dwReturn = MCI_DEVTYPE_WAVEFORM_AUDIO;
	    break;
	case MCI_GETDEVCAPS_HAS_AUDIO:
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_HAS_VIDEO:
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_USES_FILES:
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_COMPOUND_DEVICE:
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_CAN_RECORD:
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_CAN_EJECT:
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_CAN_PLAY:
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_CAN_SAVE:
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_WAVE_GETDEVCAPS_INPUTS:
	    lpParms->dwReturn = 1;
	    break;
	case MCI_WAVE_GETDEVCAPS_OUTPUTS:
	    lpParms->dwReturn = 1;
	    break;
	default:
	    TRACE("Unknown capability (%08lx) !\n", lpParms->dwItem);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    }
    return 0;
}

/**************************************************************************
 * 				WAVE_mciInfo			[internal]
 */
static DWORD WAVE_mciInfo(UINT16 wDevID, DWORD dwFlags, LPMCI_INFO_PARMS16 lpParms)
{
    DWORD		ret = 0;
    LPCSTR		str = 0;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    
    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL || lpParms->lpstrReturn == NULL) {
	ret = MCIERR_NULL_PARAMETER_BLOCK;
    } else if (wmw == NULL) {
	ret = MCIERR_INVALID_DEVICE_ID;
    } else {
	TRACE("buf=%p, len=%lu\n", lpParms->lpstrReturn, lpParms->dwRetSize);
	
	switch(dwFlags) {
	case MCI_INFO_PRODUCT:
	    str = "Wine's audio player";
	    break;
	case MCI_INFO_FILE:
	    str = wmw->openParms.lpstrElementName;
	    break;
	case MCI_WAVE_INPUT:
	    str = "Wine Wave In";
	    break;
	case MCI_WAVE_OUTPUT:
	    str = "Wine Wave Out";
	    break;
	default:
	    WARN("Don't know this info command (%lu)\n", dwFlags);
	    ret = MCIERR_UNRECOGNIZED_COMMAND;
	}
    }
    if (str) {
	if (strlen(str) + 1 > lpParms->dwRetSize) {
	    ret = MCIERR_PARAM_OVERFLOW;
	} else {
	    lstrcpynA(lpParms->lpstrReturn, str, lpParms->dwRetSize);
	}
    } else {
	lpParms->lpstrReturn[0] = 0;
    }
    
    return ret;
}

/**************************************************************************
 * 				MCIWAVE_DriverProc		[sample driver]
 */
LONG CALLBACK	MCIWAVE_DriverProc(DWORD dwDevID, HDRVR hDriv, DWORD wMsg, 
				   DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%08lX, %04X, %08lX, %08lX, %08lX)\n", 
	  dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    
    switch(wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return WAVE_drvOpen((LPSTR)dwParam1, (LPMCI_OPEN_DRIVER_PARMSA)dwParam2);
    case DRV_CLOSE:		return WAVE_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "Sample MultiMedia Driver !", "OSS Driver", MB_OK);	return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    case MCI_OPEN_DRIVER:	return WAVE_mciOpen      (dwDevID, dwParam1, (LPMCI_WAVE_OPEN_PARMSA)  dwParam2);
    case MCI_CLOSE_DRIVER:	return WAVE_mciClose     (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)     dwParam2);
    case MCI_CUE:		return WAVE_mciCue       (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)     dwParam2);
    case MCI_PLAY:		return WAVE_mciPlay      (dwDevID, dwParam1, (LPMCI_PLAY_PARMS)        dwParam2);
    case MCI_RECORD:		return WAVE_mciRecord    (dwDevID, dwParam1, (LPMCI_RECORD_PARMS)      dwParam2);
    case MCI_STOP:		return WAVE_mciStop      (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)     dwParam2);
    case MCI_SET:		return WAVE_mciSet       (dwDevID, dwParam1, (LPMCI_SET_PARMS)         dwParam2);
    case MCI_PAUSE:		return WAVE_mciPause     (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)     dwParam2);
    case MCI_RESUME:		return WAVE_mciResume    (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)     dwParam2);
    case MCI_STATUS:		return WAVE_mciStatus    (dwDevID, dwParam1, (LPMCI_STATUS_PARMS)      dwParam2);
    case MCI_GETDEVCAPS:	return WAVE_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)  dwParam2);
    case MCI_INFO:		return WAVE_mciInfo      (dwDevID, dwParam1, (LPMCI_INFO_PARMS16)      dwParam2);
    case MCI_SEEK:		return WAVE_mciSeek      (dwDevID, dwParam1, (LPMCI_SEEK_PARMS)        dwParam2);		
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
	WARN("Unsupported command=%s\n", MCI_CommandToString(wMsg));
	break;
    case MCI_OPEN:
    case MCI_CLOSE:
	FIXME("Shouldn't receive a MCI_OPEN or CLOSE message\n");
	break;
    default:
	FIXME("is probably wrong msg=%s\n", MCI_CommandToString(wMsg));
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return MCIERR_UNRECOGNIZED_COMMAND;
}
