/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*				   
 * Sample Wine Driver for MCI wave forms
 *
 * Copyright 	1994 Martin Ayotte
 *		1999 Eric Pouech
 */
/*
 * FIXME:
 *	- record/play should and must be done asynchronous
 */

#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "driver.h"
#include "mmddk.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mciwave)

typedef struct {
    UINT			wDevID;
    HANDLE			hWave;
    int				nUseCount;	/* Incremented for each shared open */
    BOOL			fShareable;	/* TRUE if first open was shareable */
    WORD			wNotifyDeviceID;/* MCI device ID with a pending notification */
    HMMIO			hFile;		/* mmio file handle open as Element */
    MCI_WAVE_OPEN_PARMSA 	openParms;
    LPWAVEFORMATEX		lpWaveFormat;
    BOOL			fInput;		/* FALSE = Output, TRUE = Input */
    volatile WORD		dwStatus;	/* one from MCI_MODE_xxxx */
    DWORD			dwMciTimeFormat;/* One of the supported MCI_FORMAT_xxxx */
    DWORD			dwFileOffset;   /* Offset of chunk in mmio file */
    DWORD			dwLength;	/* number of bytes in chunk for playing */
    DWORD			dwPosition;	/* position in bytes in chunk for playing */
    HANDLE			hEvent;		/* for synchronization */
    DWORD			dwEventCount;	/* for synchronization */
} WINE_MCIWAVE;

/* ===================================================================
 * ===================================================================
 * FIXME: should be using the new mmThreadXXXX functions from WINMM
 * instead of those
 * it would require to add a wine internal flag to mmThreadCreate
 * in order to pass a 32 bit function instead of a 16 bit one
 * ===================================================================
 * =================================================================== */

struct SCA {
    UINT 	wDevID;
    UINT 	wMsg;
    DWORD 	dwParam1;
    DWORD 	dwParam2;
    BOOL	allocatedCopy;
};

/**************************************************************************
 * 				MCI_SCAStarter			[internal]
 */
static DWORD CALLBACK	MCI_SCAStarter(LPVOID arg)
{
    struct SCA*	sca = (struct SCA*)arg;
    DWORD		ret;

    TRACE("In thread before async command (%08x,%u,%08lx,%08lx)\n",
	  sca->wDevID, sca->wMsg, sca->dwParam1, sca->dwParam2);
    ret = mciSendCommandA(sca->wDevID, sca->wMsg, sca->dwParam1 | MCI_WAIT, sca->dwParam2);
    TRACE("In thread after async command (%08x,%u,%08lx,%08lx)\n",
	  sca->wDevID, sca->wMsg, sca->dwParam1, sca->dwParam2);
    if (sca->allocatedCopy)
	HeapFree(GetProcessHeap(), 0, (LPVOID)sca->dwParam2);
    HeapFree(GetProcessHeap(), 0, sca);
    ExitThread(ret);
    WARN("Should not happen ? what's wrong \n");
    /* should not go after this point */
    return ret;
}

/**************************************************************************
 * 				MCI_SendCommandAsync		[internal]
 */
static	DWORD MCI_SendCommandAsync(UINT wDevID, UINT wMsg, DWORD dwParam1, 
				   DWORD dwParam2, UINT size)
{
    struct SCA*	sca = HeapAlloc(GetProcessHeap(), 0, sizeof(struct SCA));

    if (sca == 0)
	return MCIERR_OUT_OF_MEMORY;

    sca->wDevID   = wDevID;
    sca->wMsg     = wMsg;
    sca->dwParam1 = dwParam1;
    
    if (size) {
	sca->dwParam2 = (DWORD)HeapAlloc(GetProcessHeap(), 0, size);
	if (sca->dwParam2 == 0) {
	    HeapFree(GetProcessHeap(), 0, sca);
	    return MCIERR_OUT_OF_MEMORY;
	}
	sca->allocatedCopy = TRUE;
	/* copy structure passed by program in dwParam2 to be sure 
	 * we can still use it whatever the program does 
	 */
	memcpy((LPVOID)sca->dwParam2, (LPVOID)dwParam2, size);
    } else {
	sca->dwParam2 = dwParam2;
	sca->allocatedCopy = FALSE;
    }

    if (CreateThread(NULL, 0, MCI_SCAStarter, sca, 0, NULL) == 0) {
	WARN("Couldn't allocate thread for async command handling, sending synchonously\n");
	return MCI_SCAStarter(&sca);
    }
    return 0;
}

/*======================================================================*
 *                  	    MCI WAVE implemantation			*
 *======================================================================*/

static DWORD WAVE_mciResume(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);

/**************************************************************************
 * 				MCIWAVE_drvOpen			[internal]	
 */
static	DWORD	WAVE_drvOpen(LPSTR str, LPMCI_OPEN_DRIVER_PARMSA modp)
{
    WINE_MCIWAVE*	wmw = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINE_MCIWAVE));

    if (!wmw)
	return 0;

    wmw->wDevID = modp->wDeviceID;
    mciSetDriverData(wmw->wDevID, (DWORD)wmw);
    modp->wCustomCommandTable = MCI_NO_COMMAND_TABLE;
    modp->wType = MCI_DEVTYPE_WAVEFORM_AUDIO;
    return modp->wDeviceID;
}

/**************************************************************************
 * 				MCIWAVE_drvClose		[internal]	
 */
static	DWORD	WAVE_drvClose(DWORD dwDevID)
{
    WINE_MCIWAVE*  wmw = (WINE_MCIWAVE*)mciGetDriverData(dwDevID);

    if (wmw) {
	HeapFree(GetProcessHeap(), 0, wmw);	
	mciSetDriverData(dwDevID, 0);
	return 1;
    }
    return 0;
}

/**************************************************************************
 * 				WAVE_mciGetOpenDev		[internal]	
 */
static WINE_MCIWAVE*  WAVE_mciGetOpenDev(UINT wDevID)
{
    WINE_MCIWAVE*	wmw = (WINE_MCIWAVE*)mciGetDriverData(wDevID);
    
    if (wmw == NULL || wmw->nUseCount == 0) {
	WARN("Invalid wDevID=%u\n", wDevID);
	return 0;
    }
    return wmw;
}

/**************************************************************************
 * 				WAVE_ConvertByteToTimeFormat	[internal]	
 */
static	DWORD 	WAVE_ConvertByteToTimeFormat(WINE_MCIWAVE* wmw, DWORD val, LPDWORD lpRet)
{
    DWORD	ret = 0;
    
    switch (wmw->dwMciTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
	ret = (val * 1000) / wmw->lpWaveFormat->nAvgBytesPerSec;
	break;
    case MCI_FORMAT_BYTES:
	ret = val;
	break;
    case MCI_FORMAT_SAMPLES: /* FIXME: is this correct ? */
	ret = (val * 8) / wmw->lpWaveFormat->wBitsPerSample;
	break;
    default:
	WARN("Bad time format %lu!\n", wmw->dwMciTimeFormat);
    }
    TRACE("val=%lu=0x%08lx [tf=%lu] => ret=%lu\n", val, val, wmw->dwMciTimeFormat, ret);
    *lpRet = 0;
    return ret;
}

/**************************************************************************
 * 				WAVE_ConvertTimeFormatToByte	[internal]	
 */
static	DWORD 	WAVE_ConvertTimeFormatToByte(WINE_MCIWAVE* wmw, DWORD val)
{
    DWORD	ret = 0;
    
    switch (wmw->dwMciTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
	ret = (val * wmw->lpWaveFormat->nAvgBytesPerSec) / 1000;
	break;
    case MCI_FORMAT_BYTES:
	ret = val;
	break;
    case MCI_FORMAT_SAMPLES: /* FIXME: is this correct ? */
	ret = (val * wmw->lpWaveFormat->wBitsPerSample) / 8;
	break;
    default:
	WARN("Bad time format %lu!\n", wmw->dwMciTimeFormat);
    }
    TRACE("val=%lu=0x%08lx [tf=%lu] => ret=%lu\n", val, val, wmw->dwMciTimeFormat, ret);
    return ret;
}

/**************************************************************************
 * 			WAVE_mciReadFmt	                        [internal]
 */
static	DWORD WAVE_mciReadFmt(WINE_MCIWAVE* wmw, MMCKINFO* pckMainRIFF)
{
    MMCKINFO	mmckInfo;
    long	r;

    mmckInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if (mmioDescend(wmw->hFile, &mmckInfo, pckMainRIFF, MMIO_FINDCHUNK) != 0)
	return MCIERR_INVALID_FILE;
    TRACE("Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX \n",
	  (LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType, mmckInfo.cksize);

    wmw->lpWaveFormat = HeapAlloc(GetProcessHeap(), 0, mmckInfo.cksize);
    r = mmioRead(wmw->hFile, (HPSTR)wmw->lpWaveFormat, mmckInfo.cksize);
    if (r < sizeof(WAVEFORMAT))
	return MCIERR_INVALID_FILE;
    
    TRACE("wFormatTag=%04X !\n",   wmw->lpWaveFormat->wFormatTag);
    TRACE("nChannels=%d \n",       wmw->lpWaveFormat->nChannels);
    TRACE("nSamplesPerSec=%ld\n",  wmw->lpWaveFormat->nSamplesPerSec);
    TRACE("nAvgBytesPerSec=%ld\n", wmw->lpWaveFormat->nAvgBytesPerSec);
    TRACE("nBlockAlign=%d \n",     wmw->lpWaveFormat->nBlockAlign);
    TRACE("wBitsPerSample=%u !\n", wmw->lpWaveFormat->wBitsPerSample);
    if (r >= (long)sizeof(WAVEFORMATEX))
	TRACE("cbSize=%u !\n", wmw->lpWaveFormat->cbSize);
	
    mmioAscend(wmw->hFile, &mmckInfo, 0);
    mmckInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioDescend(wmw->hFile, &mmckInfo, pckMainRIFF, MMIO_FINDCHUNK) != 0) {
	TRACE("can't find data chunk\n");
	return MCIERR_INVALID_FILE;
    }
    TRACE("Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX \n",
	  (LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType, mmckInfo.cksize);
    TRACE("nChannels=%d nSamplesPerSec=%ld\n",
	  wmw->lpWaveFormat->nChannels, wmw->lpWaveFormat->nSamplesPerSec);
    wmw->dwLength = mmckInfo.cksize;
    wmw->dwFileOffset = mmckInfo.dwDataOffset;
    return 0;
}

/**************************************************************************
 * 			WAVE_mciOpen	                        [internal]
 */
static DWORD WAVE_mciOpen(UINT wDevID, DWORD dwFlags, LPMCI_WAVE_OPEN_PARMSA lpOpenParms)
{
    DWORD		dwRet = 0;
    DWORD		dwDeviceID;
    WINE_MCIWAVE*	wmw = (WINE_MCIWAVE*)mciGetDriverData(wDevID);
    
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
    wmw->hWave = 0;

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
				       MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
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
	if (wmw->lpWaveFormat) {
	    switch (wmw->lpWaveFormat->wFormatTag) {
	    case WAVE_FORMAT_PCM:
		if (wmw->lpWaveFormat->nAvgBytesPerSec != 
		    wmw->lpWaveFormat->nSamplesPerSec * wmw->lpWaveFormat->nBlockAlign) {
		    WARN("Incorrect nAvgBytesPerSec (%ld), setting it to %ld\n", 
			wmw->lpWaveFormat->nAvgBytesPerSec, 
			wmw->lpWaveFormat->nSamplesPerSec * 
			 wmw->lpWaveFormat->nBlockAlign);
		    wmw->lpWaveFormat->nAvgBytesPerSec = 
			wmw->lpWaveFormat->nSamplesPerSec * 
			wmw->lpWaveFormat->nBlockAlign;
		}
		break;
	    }
	}
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
static DWORD WAVE_mciCue(UINT wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
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
    
    FIXME("(%u, %08lX, %p); likely to fail\n", wDevID, dwParam, lpParms);
    
    if (wmw == NULL)	return MCIERR_INVALID_DEVICE_ID;
    
    /* always close elements ? */    
    if (wmw->hFile != 0) {
	mmioClose(wmw->hFile, 0);
	wmw->hFile = 0;
    }
    
    dwRet = MMSYSERR_NOERROR;  /* assume success */
    
    if ((dwParam & MCI_WAVE_INPUT) && !wmw->fInput) {
	dwRet = waveOutClose(wmw->hWave);
	if (dwRet != MMSYSERR_NOERROR) return MCIERR_INTERNAL;
	wmw->fInput = TRUE;
    } else if (wmw->fInput) {
	dwRet = waveInClose(wmw->hWave);
	if (dwRet != MMSYSERR_NOERROR) return MCIERR_INTERNAL;
	wmw->fInput = FALSE;
    }
    wmw->hWave = 0;
    return (dwRet == MMSYSERR_NOERROR) ? 0 : MCIERR_INTERNAL;
}

/**************************************************************************
 * 				WAVE_mciStop			[internal]
 */
static DWORD WAVE_mciStop(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD 		dwRet = 0;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    
    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    /* wait for playback thread (if any) to exit before processing further */
    switch (wmw->dwStatus) {
    case MCI_MODE_PAUSE:
    case MCI_MODE_PLAY:
    case MCI_MODE_RECORD:
	{
	    int oldStat = wmw->dwStatus;
	    wmw->dwStatus = MCI_MODE_NOT_READY;
	    if (oldStat == MCI_MODE_PAUSE)
		dwRet = (wmw->fInput) ? waveInReset(wmw->hWave) : waveOutReset(wmw->hWave);
	}
	while (wmw->dwStatus != MCI_MODE_STOP)
	    Sleep(10);
	break;
    }

    wmw->dwPosition = 0;

    /* sanity resets */
    wmw->dwStatus = MCI_MODE_STOP;
    
    if ((dwFlags & MCI_NOTIFY) && lpParms) {
	mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			wmw->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    
    return dwRet;
}

/**************************************************************************
 *				WAVE_mciClose		[internal]
 */
static DWORD WAVE_mciClose(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
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
	if (wmw->hFile != 0) {
	    mmioClose(wmw->hFile, 0);
	    wmw->hFile = 0;
	}
    }
    
    HeapFree(GetProcessHeap(), 0, wmw->lpWaveFormat);
    wmw->lpWaveFormat = NULL;

    if ((dwFlags & MCI_NOTIFY) && lpParms) {
	mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			wmw->wNotifyDeviceID,
			(dwRet == 0) ? MCI_NOTIFY_SUCCESSFUL : MCI_NOTIFY_FAILURE);
    }

    return 0;
}

/**************************************************************************
 * 				WAVE_mciPlayCallback		[internal]
 */
static	void	CALLBACK WAVE_mciPlayCallback(HWAVEOUT hwo, UINT uMsg, 
					      DWORD dwInstance,  
					      DWORD dwParam1, DWORD dwParam2)
{
    WINE_MCIWAVE*	wmw = (WINE_MCIWAVE*)dwInstance;

    switch (uMsg) {
    case WOM_OPEN:
    case WOM_CLOSE:
	break;
    case WOM_DONE:
	InterlockedIncrement(&wmw->dwEventCount);
	TRACE("Returning waveHdr=%lx\n", dwParam1);
	SetEvent(wmw->hEvent);
	break;
    default:
	ERR("Unknown uMsg=%d\n", uMsg);
    }
}

static void WAVE_mciPlayWaitDone(WINE_MCIWAVE* wmw)
{
    for (;;) {
	ResetEvent(wmw->hEvent);
	if (InterlockedDecrement(&wmw->dwEventCount) >= 0) {
	    break;
	}
	InterlockedIncrement(&wmw->dwEventCount);
	
	WaitForSingleObject(wmw->hEvent, INFINITE);
    }
}

/**************************************************************************
 * 				WAVE_mciPlay		[internal]
 */
static DWORD WAVE_mciPlay(UINT wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
    DWORD		end;
    LONG		bufsize, count, left;
    DWORD		dwRet = 0;
    LPWAVEHDR		waveHdr = NULL;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    int			whidx;

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
    
	if (wmw->dwStatus == MCI_MODE_PAUSE) {
	    /* FIXME: parameters (start/end) in lpParams may not be used */
	    return WAVE_mciResume(wDevID, dwFlags, (LPMCI_GENERIC_PARMS)lpParms);
	}
    
    /** This function will be called again by a thread when async is used.
     * We have to set MCI_MODE_PLAY before we do this so that the app can spin
     * on MCI_STATUS, so we have to allow it here if we're not going to start this thread.
     */
    if ((wmw->dwStatus != MCI_MODE_STOP) && ((wmw->dwStatus != MCI_MODE_PLAY) && (dwFlags & MCI_WAIT))) {
	return MCIERR_INTERNAL;
    }

    wmw->dwStatus = MCI_MODE_PLAY;
    
    if (!(dwFlags & MCI_WAIT)) {
	return MCI_SendCommandAsync(wmw->wNotifyDeviceID, MCI_PLAY, dwFlags, 
				    (DWORD)lpParms, sizeof(MCI_PLAY_PARMS));
    }

    end = 0xFFFFFFFF;
    if (lpParms && (dwFlags & MCI_FROM)) {
	wmw->dwPosition = WAVE_ConvertTimeFormatToByte(wmw, lpParms->dwFrom); 
    }
    if (lpParms && (dwFlags & MCI_TO)) {
	end = WAVE_ConvertTimeFormatToByte(wmw, lpParms->dwTo);
    }
    
    TRACE("Playing from byte=%lu to byte=%lu\n", wmw->dwPosition, end);
    
    if (end <= wmw->dwPosition)
	return TRUE;

#define	WAVE_ALIGN_ON_BLOCK(wmw,v) \
((((v) + (wmw)->lpWaveFormat->nBlockAlign - 1) / (wmw)->lpWaveFormat->nBlockAlign) * (wmw)->lpWaveFormat->nBlockAlign)

    wmw->dwPosition = WAVE_ALIGN_ON_BLOCK(wmw, wmw->dwPosition);
    wmw->dwLength   = WAVE_ALIGN_ON_BLOCK(wmw, wmw->dwLength);
    /* go back to begining of chunk plus the requested position */
    /* FIXME: I'm not sure this is correct, notably because some data linked to 
     * the decompression state machine will not be correcly initialized.
     * try it this way (other way would be to decompress from 0 up to dwPosition
     * and to start sending to hWave when dwPosition is reached)
     */
    mmioSeek(wmw->hFile, wmw->dwFileOffset + wmw->dwPosition, SEEK_SET); /* >= 0 */

    /* By default the device will be opened for output, the MCI_CUE function is there to
     * change from output to input and back
     */
    /* FIXME: how to choose between several output channels ? here mapper is forced */
    dwRet = waveOutOpen(&wmw->hWave, WAVE_MAPPER, wmw->lpWaveFormat, 
			(DWORD)WAVE_mciPlayCallback, (DWORD)wmw, CALLBACK_FUNCTION);

    if (dwRet != 0) {
	TRACE("Can't open low level audio device %ld\n", dwRet);
	dwRet = MCIERR_DEVICE_OPEN;
	wmw->hWave = 0;
	goto cleanUp;
    }

    /* make it so that 3 buffers per second are needed */
    bufsize = WAVE_ALIGN_ON_BLOCK(wmw, wmw->lpWaveFormat->nAvgBytesPerSec / 3);

    waveHdr = HeapAlloc(GetProcessHeap(), 0, 2 * sizeof(WAVEHDR) + 2 * bufsize);
    waveHdr[0].lpData = (char*)waveHdr + 2 * sizeof(WAVEHDR);
    waveHdr[1].lpData = (char*)waveHdr + 2 * sizeof(WAVEHDR) + bufsize;
    waveHdr[0].dwUser         = waveHdr[1].dwUser         = 0L;
    waveHdr[0].dwLoops        = waveHdr[1].dwLoops        = 0L;
    waveHdr[0].dwFlags        = waveHdr[1].dwFlags        = 0L;
    waveHdr[0].dwBufferLength = waveHdr[1].dwBufferLength = bufsize;
    if (waveOutPrepareHeader(wmw->hWave, &waveHdr[0], sizeof(WAVEHDR)) || 
	waveOutPrepareHeader(wmw->hWave, &waveHdr[1], sizeof(WAVEHDR))) {
	dwRet = MCIERR_INTERNAL;
	goto cleanUp;
    }

    whidx = 0;
    left = min(wmw->dwLength, end - wmw->dwPosition);
    wmw->hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    wmw->dwEventCount = 1L; /* for first buffer */

    TRACE("Playing (normalized) from byte=%lu for %lu bytes\n", wmw->dwPosition, left);
    
    /* FIXME: this doesn't work if wmw->dwPosition != 0 */
    while (left > 0 && wmw->dwStatus != MCI_MODE_STOP && wmw->dwStatus != MCI_MODE_NOT_READY) {
	count = mmioRead(wmw->hFile, waveHdr[whidx].lpData, min(bufsize, left));
	TRACE("mmioRead bufsize=%ld count=%ld\n", bufsize, count);
	if (count < 1) 
	    break;
	/* count is always <= bufsize, so this is correct regarding the 
	 * waveOutPrepareHeader function 
	 */
	waveHdr[whidx].dwBufferLength = count;
	waveHdr[whidx].dwFlags &= ~WHDR_DONE;
	TRACE("before WODM_WRITE lpWaveHdr=%p dwBufferLength=%lu dwBytesRecorded=%lu\n",
	      &waveHdr[whidx], waveHdr[whidx].dwBufferLength, 
	      waveHdr[whidx].dwBytesRecorded);
	dwRet = waveOutWrite(wmw->hWave, &waveHdr[whidx], sizeof(WAVEHDR));
	left -= count;
	wmw->dwPosition += count;
	TRACE("after WODM_WRITE dwPosition=%lu\n", wmw->dwPosition);

	WAVE_mciPlayWaitDone(wmw);
	whidx ^= 1;
    }

    WAVE_mciPlayWaitDone(wmw); /* to balance first buffer */

    /* just to get rid of some race conditions between play, stop and pause */
    waveOutReset(wmw->hWave);

    waveOutUnprepareHeader(wmw->hWave, &waveHdr[0], sizeof(WAVEHDR));
    waveOutUnprepareHeader(wmw->hWave, &waveHdr[1], sizeof(WAVEHDR));

    dwRet = 0;

cleanUp:    
    HeapFree(GetProcessHeap(), 0, waveHdr);

    if (wmw->hWave) {
	waveOutClose(wmw->hWave);
	wmw->hWave = 0;
    }
    CloseHandle(wmw->hEvent);

    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			wmw->wNotifyDeviceID, 
			dwRet ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL);
    }

    wmw->dwStatus = MCI_MODE_STOP;

    return dwRet;
}

/**************************************************************************
 * 				WAVE_mciRecord			[internal]
 */
static DWORD WAVE_mciRecord(UINT wDevID, DWORD dwFlags, LPMCI_RECORD_PARMS lpParms)
{
    int		       	start, end;
    LONG		bufsize;
    WAVEHDR		waveHdr;
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
    waveHdr.lpData = HeapAlloc(GetProcessHeap(), 0, bufsize);
    waveHdr.dwBufferLength = bufsize;
    waveHdr.dwUser = 0L;
    waveHdr.dwFlags = 0L;
    waveHdr.dwLoops = 0L;
    dwRet = waveInPrepareHeader(wmw->hWave, &waveHdr, sizeof(WAVEHDR));

    for (;;) { /* FIXME: I don't see any waveInAddBuffer ? */
	waveHdr.dwBytesRecorded = 0;
	dwRet = waveInStart(wmw->hWave);
	TRACE("waveInStart => lpWaveHdr=%p dwBytesRecorded=%lu\n",
	      &waveHdr, waveHdr.dwBytesRecorded);
	if (waveHdr.dwBytesRecorded == 0) break;
    }
    dwRet = waveInUnprepareHeader(wmw->hWave, &waveHdr, sizeof(WAVEHDR));
    HeapFree(GetProcessHeap(), 0, waveHdr.lpData);

    if (dwFlags & MCI_NOTIFY) {
	mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			wmw->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				WAVE_mciPause			[internal]
 */
static DWORD WAVE_mciPause(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD 		dwRet;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    
    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (wmw->dwStatus == MCI_MODE_PLAY) {
	wmw->dwStatus = MCI_MODE_PAUSE;
    } 
    
    if (wmw->fInput)	dwRet = waveInStop(wmw->hWave);
    else		dwRet = waveOutPause(wmw->hWave);
    
    return (dwRet == MMSYSERR_NOERROR) ? 0 : MCIERR_INTERNAL;
}

/**************************************************************************
 * 				WAVE_mciResume			[internal]
 */
static DWORD WAVE_mciResume(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    DWORD		dwRet = 0;
    
    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (wmw->dwStatus == MCI_MODE_PAUSE) {
	wmw->dwStatus = MCI_MODE_PLAY;
    } 
    
    if (wmw->fInput)	dwRet = waveInStart(wmw->hWave);
    else		dwRet = waveOutRestart(wmw->hWave);
    return (dwRet == MMSYSERR_NOERROR) ? 0 : MCIERR_INTERNAL;
}

/**************************************************************************
 * 				WAVE_mciSeek			[internal]
 */
static DWORD WAVE_mciSeek(UINT wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
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
	    wmw->dwPosition = wmw->dwLength;
	} else if (dwFlags & MCI_TO) {
	    wmw->dwPosition = WAVE_ConvertTimeFormatToByte(wmw, lpParms->dwTo);
	} else {
	    WARN("dwFlag doesn't tell where to seek to...\n");
	    return MCIERR_MISSING_PARAMETER;
	}
	
	TRACE("Seeking to position=%lu bytes\n", wmw->dwPosition);
	
	if (dwFlags & MCI_NOTIFY) {
	    mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			    wmw->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
	}
    }
    return ret;	
}    

/**************************************************************************
 * 				WAVE_mciSet			[internal]
 */
static DWORD WAVE_mciSet(UINT wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
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
static DWORD WAVE_mciStatus(UINT wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    DWORD		ret = 0;

    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (dwFlags & MCI_STATUS_ITEM) {
	switch (lpParms->dwItem) {
	case MCI_STATUS_CURRENT_TRACK:
	    lpParms->dwReturn = 1;
	    TRACE("MCI_STATUS_CURRENT_TRACK => %lu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_LENGTH:
	    if (!wmw->hFile) {
		lpParms->dwReturn = 0;
		return MCIERR_UNSUPPORTED_FUNCTION;
	    }
	    /* only one track in file is currently handled, so don't take care of MCI_TRACK flag */
	    lpParms->dwReturn = WAVE_ConvertByteToTimeFormat(wmw, wmw->dwLength, &ret);
	    TRACE("MCI_STATUS_LENGTH => %lu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_MODE:
	    TRACE("MCI_STATUS_MODE => %u\n", wmw->dwStatus);
	    lpParms->dwReturn = MAKEMCIRESOURCE(wmw->dwStatus, wmw->dwStatus);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_MEDIA_PRESENT:
	    TRACE("MCI_STATUS_MEDIA_PRESENT => TRUE!\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_NUMBER_OF_TRACKS:
	    /* only one track in file is currently handled, so don't take care of MCI_TRACK flag */
	    lpParms->dwReturn = 1;
	    TRACE("MCI_STATUS_NUMBER_OF_TRACKS => %lu!\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_POSITION:
	    if (!wmw->hFile) {
		lpParms->dwReturn = 0;
		return MCIERR_UNSUPPORTED_FUNCTION;
	    }
	    /* only one track in file is currently handled, so don't take care of MCI_TRACK flag */
	    lpParms->dwReturn = WAVE_ConvertByteToTimeFormat(wmw, 
							     (dwFlags & MCI_STATUS_START) ? 0 : wmw->dwPosition,
							     &ret);
	    TRACE("MCI_STATUS_POSITION %s => %lu\n", 
		  (dwFlags & MCI_STATUS_START) ? "start" : "current", lpParms->dwReturn);
	    break;
	case MCI_STATUS_READY:
	    lpParms->dwReturn = (wmw->dwStatus == MCI_MODE_NOT_READY) ?
		MAKEMCIRESOURCE(FALSE, MCI_FALSE) : MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    TRACE("MCI_STATUS_READY => %u!\n", LOWORD(lpParms->dwReturn));
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_TIME_FORMAT:
	    lpParms->dwReturn = MAKEMCIRESOURCE(wmw->dwMciTimeFormat, wmw->dwMciTimeFormat);
	    TRACE("MCI_STATUS_TIME_FORMAT => %lu\n", lpParms->dwReturn);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_WAVE_INPUT:
	    TRACE("MCI_WAVE_INPUT !\n");
	    lpParms->dwReturn = 0;
	    ret = MCIERR_WAVE_INPUTUNSPECIFIED;
	    break;
	case MCI_WAVE_OUTPUT:
	    TRACE("MCI_WAVE_OUTPUT !\n");
	    {
		UINT	id;
		if (waveOutGetID(wmw->hWave, &id) == MMSYSERR_NOERROR) {
		    lpParms->dwReturn = id;
		} else {
		    lpParms->dwReturn = 0;
		    ret = MCIERR_WAVE_INPUTUNSPECIFIED;
		}
	    }
	    break;
	case MCI_WAVE_STATUS_AVGBYTESPERSEC:
	    if (!wmw->hFile) {
		lpParms->dwReturn = 0;
		return MCIERR_UNSUPPORTED_FUNCTION;
	    }
	    lpParms->dwReturn = wmw->lpWaveFormat->nAvgBytesPerSec;
	    TRACE("MCI_WAVE_STATUS_AVGBYTESPERSEC => %lu!\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_STATUS_BITSPERSAMPLE:
	    if (!wmw->hFile) {
		lpParms->dwReturn = 0;
		return MCIERR_UNSUPPORTED_FUNCTION;
	    }
	    lpParms->dwReturn = wmw->lpWaveFormat->wBitsPerSample;
	    TRACE("MCI_WAVE_STATUS_BITSPERSAMPLE => %lu!\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_STATUS_BLOCKALIGN:
	    if (!wmw->hFile) {
		lpParms->dwReturn = 0;
		return MCIERR_UNSUPPORTED_FUNCTION;
	    }
	    lpParms->dwReturn = wmw->lpWaveFormat->nBlockAlign;
	    TRACE("MCI_WAVE_STATUS_BLOCKALIGN => %lu!\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_STATUS_CHANNELS:
	    if (!wmw->hFile) {
		lpParms->dwReturn = 0;
		return MCIERR_UNSUPPORTED_FUNCTION;
	    }
	    lpParms->dwReturn = wmw->lpWaveFormat->nChannels;
	    TRACE("MCI_WAVE_STATUS_CHANNELS => %lu!\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_STATUS_FORMATTAG:
	    if (!wmw->hFile) {
		lpParms->dwReturn = 0;
		return MCIERR_UNSUPPORTED_FUNCTION;
	    }
	    lpParms->dwReturn = wmw->lpWaveFormat->wFormatTag;
	    TRACE("MCI_WAVE_FORMATTAG => %lu!\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_STATUS_LEVEL:
	    TRACE("MCI_WAVE_STATUS_LEVEL !\n");
	    lpParms->dwReturn = 0xAAAA5555;
	    break;
	case MCI_WAVE_STATUS_SAMPLESPERSEC:
	    if (!wmw->hFile) {
		lpParms->dwReturn = 0;
		return MCIERR_UNSUPPORTED_FUNCTION;
	    }
	    lpParms->dwReturn = wmw->lpWaveFormat->nSamplesPerSec;
	    TRACE("MCI_WAVE_STATUS_SAMPLESPERSEC => %lu!\n", lpParms->dwReturn);
	    break;
	default:
	    WARN("unknown command %08lX !\n", lpParms->dwItem);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    }
    if (dwFlags & MCI_NOTIFY) {
	mciDriverNotify((HWND)LOWORD(lpParms->dwCallback), 
			wmw->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return ret;
}

/**************************************************************************
 * 				WAVE_mciGetDevCaps		[internal]
 */
static DWORD WAVE_mciGetDevCaps(UINT wDevID, DWORD dwFlags, 
				LPMCI_GETDEVCAPS_PARMS lpParms)
{
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    DWORD		ret = 0;

    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (dwFlags & MCI_GETDEVCAPS_ITEM) {
	switch(lpParms->dwItem) {
	case MCI_GETDEVCAPS_DEVICE_TYPE:
	    lpParms->dwReturn = MAKEMCIRESOURCE(MCI_DEVTYPE_WAVEFORM_AUDIO, MCI_DEVTYPE_WAVEFORM_AUDIO);
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
	case MCI_GETDEVCAPS_USES_FILES:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_COMPOUND_DEVICE:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_RECORD:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_EJECT:
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_PLAY:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_SAVE:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_WAVE_GETDEVCAPS_INPUTS:
	    lpParms->dwReturn = 1;
	    break;
	case MCI_WAVE_GETDEVCAPS_OUTPUTS:
	    lpParms->dwReturn = 1;
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

/**************************************************************************
 * 				WAVE_mciInfo			[internal]
 */
static DWORD WAVE_mciInfo(UINT wDevID, DWORD dwFlags, LPMCI_INFO_PARMSA lpParms)
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
	
	switch (dwFlags & ~(MCI_WAIT|MCI_NOTIFY)) {
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
    case MCI_INFO:		return WAVE_mciInfo      (dwDevID, dwParam1, (LPMCI_INFO_PARMSA)       dwParam2);
    case MCI_SEEK:		return WAVE_mciSeek      (dwDevID, dwParam1, (LPMCI_SEEK_PARMS)        dwParam2);		
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
	FIXME("Unsupported yet command [%lu]\n", wMsg);
	break;
    case MCI_WINDOW:
	TRACE("Unsupported command [%lu]\n", wMsg);
	break;
    case MCI_OPEN:
    case MCI_CLOSE:
	ERR("Shouldn't receive a MCI_OPEN or CLOSE message\n");
	break;
    default:
	FIXME("is probably wrong msg [%lu]\n", wMsg);
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return MCIERR_UNRECOGNIZED_COMMAND;
}
