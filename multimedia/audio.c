/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*				   
 * Sample Wine Driver for Open Sound System (featured in Linux and FreeBSD)
 *
 * Copyright 1994 Martin Ayotte
 *           1999 Eric Pouech (async playing in waveOut)
 */
/*
 * FIXME:
 *	pause in waveOut does not work correctly
 * 	implement async handling in waveIn
 */

/*#define EMULATE_SB16*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "wine/winuser16.h"
#include "driver.h"
#include "multimedia.h"
#include "heap.h"
#include "ldt.h"
#include "debug.h"

#ifdef HAVE_OSS

#define SOUND_DEV "/dev/dsp"
#define MIXER_DEV "/dev/mixer"

#define MAX_WAVEOUTDRV 	(1)
#define MAX_WAVEINDRV 	(1)

/* state diagram for waveOut writing:
 * FIXME: I'm not 100% pleased with the handling of RESETTING and CLOSING which are 
 * temporary states.
 * Basically, they are events handled in waveOutXXXX functions (open, write, pause, restart), 
 * and the others (resetn close) which are handled by the player. We (I ?) use transient
 * states (RESETTING and CLOSING) to send the event from the waveOutXXX functions to 
 * the player. Not very good.
 *
 * 		open()		STOPPED
 * PAUSED	write()		PAUSED
 * STOPPED	write()		PLAYING
 * (other)	write()		<error>
 * (any)	pause()		PAUSED
 * PAUSED	restart()	PLAYING if hdr, STOPPED otherwise
 * 		reset()		RESETTING => STOPPED
 *		close()		CLOSING => CLOSED
 */

#define	WINE_WS_PLAYING		0
#define	WINE_WS_PAUSED		1
#define	WINE_WS_STOPPED		2
#define	WINE_WS_RESETTING	3
#define WINE_WS_CLOSING		4
#define WINE_WS_CLOSED		5

typedef struct {
    int				unixdev;
    volatile int		state;	/* one of the WINE_WS_ manifest constants */
    DWORD			dwFragmentSize;
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    PCMWAVEFORMAT		format;
    LPWAVEHDR			lpQueueHdr;
    DWORD			dwTotalPlayed;
    HANDLE			hThread;
    CRITICAL_SECTION		critSect;
    /* used only by wodPlayer */
    DWORD			dwOffCurrHdr;		/* offset in lpQueueHdr->lpWaveHdr->lpData for fragments */
    DWORD			dwRemain;		/* number of bytes to write to end the current fragment  */
    DWORD			dwNotifiedBytes;	/* number of bytes for which wavehdr notification has been done */
    WAVEHDR*			lpNotifyHdr;		/* list of wavehdr for which write() has been called, pending for notification */
} WINE_WAVEOUT;

typedef struct {
    int				unixdev;
    volatile int		state;
    DWORD			dwFragmentSize;	/* OpenSound '/dev/dsp' give us that size */
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    PCMWAVEFORMAT		format;
    LPWAVEHDR			lpQueueHdr;
    DWORD			dwTotalRecorded;
} WINE_WAVEIN;

static WINE_WAVEOUT	WOutDev   [MAX_WAVEOUTDRV];
static WINE_WAVEIN	WInDev    [MAX_WAVEOUTDRV];

/*======================================================================*
 *                  Low level WAVE implemantation			*
 *======================================================================*/

/**************************************************************************
 * 			WAVE_NotifyClient			[internal]
 */
static DWORD WAVE_NotifyClient(UINT16 wDevID, WORD wMsg, 
			       DWORD dwParam1, DWORD dwParam2)
{
    TRACE(wave, "wDevID = %04X wMsg = %d dwParm1 = %04lX dwParam2 = %04lX\n",wDevID, wMsg, dwParam1, dwParam2);
    
    switch (wMsg) {
    case WOM_OPEN:
    case WOM_CLOSE:
    case WOM_DONE:
	if (wDevID > MAX_WAVEOUTDRV) return MCIERR_INTERNAL;
	
	if (WOutDev[wDevID].wFlags != DCB_NULL && 
	    !DriverCallback16(WOutDev[wDevID].waveDesc.dwCallBack, 
			      WOutDev[wDevID].wFlags, 
			      WOutDev[wDevID].waveDesc.hWave, 
			      wMsg, 
			      WOutDev[wDevID].waveDesc.dwInstance, 
			      dwParam1, 
			      dwParam2)) {
	    WARN(wave, "can't notify client !\n");
	    return MMSYSERR_NOERROR;
	}
	break;
	
    case WIM_OPEN:
    case WIM_CLOSE:
    case WIM_DATA:
	if (wDevID > MAX_WAVEINDRV) return MCIERR_INTERNAL;
	
	if (WInDev[wDevID].wFlags != DCB_NULL && 
	    !DriverCallback16(WInDev[wDevID].waveDesc.dwCallBack, 
			      WInDev[wDevID].wFlags, 
			      WInDev[wDevID].waveDesc.hWave, 
			      wMsg, 
			      WInDev[wDevID].waveDesc.dwInstance, 
			      dwParam1, 
			      dwParam2)) {
	    WARN(wave, "can't notify client !\n");
	    return MMSYSERR_NOERROR;
	}
	break;
    }
    return 0;
}

/*======================================================================*
 *                  Low level WAVE OUT implemantation			*
 *======================================================================*/

/**************************************************************************
 * 				wodPlayer_WriteFragments	[internal]
 *
 * wodPlayer helper. Writes as many fragments it can to unixdev.
 * Returns TRUE in case of buffer underrun.
 */
static	BOOL	wodPlayer_WriteFragments(WINE_WAVEOUT* wwo)
{
    LPWAVEHDR		lpWaveHdr;
    LPWAVEHDR		wh;
    LPBYTE		lpData;
    int			count;
    audio_buf_info 	abinfo;
    
    for (;;) {
	/* get number of writable fragments */
	if (ioctl(wwo->unixdev, SNDCTL_DSP_GETOSPACE, &abinfo) == -1) {
	    perror("ioctl SNDCTL_DSP_GETOSPACE");
	    wwo->hThread = 0;
	    LeaveCriticalSection(&wwo->critSect);
	    ExitThread(-1);
	}
	TRACE(wave, "Can write %d fragments\n", abinfo.fragments);

	if (abinfo.fragments == 0)	/* output queue is full, wait a bit */
	    return FALSE;

	lpWaveHdr = wwo->lpQueueHdr;
	if (!lpWaveHdr) {
	    if (wwo->dwRemain > 0 &&				/* still data to send to complete current fragment */
		wwo->dwNotifiedBytes >= wwo->dwFragmentSize &&  /* first fragment has been played */
		abinfo.fragstotal - abinfo.fragments < 2) {     /* done with all waveOutWrite()' fragments */
		    
		TRACE(wave, "Oooch, buffer underrun !\n");
		return TRUE; /* force resetting of waveOut device */
	    }
	    return FALSE;	/* wait a bit */
	}
	
	if (wwo->dwOffCurrHdr == 0) {
	    TRACE(wave, "Starting a new wavehdr %p of %ld bytes\n", lpWaveHdr, lpWaveHdr->dwBufferLength);
	    if (lpWaveHdr->dwFlags & WHDR_BEGINLOOP)
		FIXME(wave, "NIY: loops (%lu) in wavehdr\n", lpWaveHdr->dwLoops);
	}
	
	lpData = ((DWORD)lpWaveHdr == lpWaveHdr->reserved) ?
	    (LPBYTE)lpWaveHdr->lpData : (LPBYTE)PTR_SEG_TO_LIN(lpWaveHdr->lpData);
	
	/* finish current wave hdr ? */
	if (wwo->dwOffCurrHdr + wwo->dwRemain >= lpWaveHdr->dwBufferLength) { 
	    DWORD	toWrite = lpWaveHdr->dwBufferLength - wwo->dwOffCurrHdr;
	    
	    /* write end of current wave hdr */
	    count = write(wwo->unixdev, lpData + wwo->dwOffCurrHdr, toWrite);
	    TRACE(wave, "write(%p[%5lu], %5lu) => %d\n", lpData, wwo->dwOffCurrHdr, toWrite, count);
	    
	    /* move lpWaveHdr to notify list */
	    if (wwo->lpNotifyHdr) {
		for (wh = wwo->lpNotifyHdr; wh->lpNext != NULL; wh = wh->lpNext);
		wh->lpNext = lpWaveHdr;
	    } else {
		wwo->lpNotifyHdr = lpWaveHdr;
	    }
	    wwo->lpQueueHdr = lpWaveHdr->lpNext;
	    lpWaveHdr->lpNext = 0;
	    
	    wwo->dwOffCurrHdr = 0;
	    if ((wwo->dwRemain -= toWrite) == 0)
		wwo->dwRemain = wwo->dwFragmentSize;
	    continue; /* try to go to use next wavehdr */
	}  else	{
	    count = write(wwo->unixdev, lpData + wwo->dwOffCurrHdr, wwo->dwRemain);
	    TRACE(wave, "write(%p[%5lu], %5lu) => %d\n", lpData, wwo->dwOffCurrHdr, wwo->dwRemain, count);
	    wwo->dwOffCurrHdr += count;
	    wwo->dwRemain = wwo->dwFragmentSize;
	}
    }
}

/**************************************************************************
 * 				wodPlayer_WriteFragments	[internal]
 *
 * wodPlayer helper. Notifies (and remove from queue) all the wavehdr which content
 * have been played (actually to speaker, not to unixdev fd).
 */
static	void	wodPlayer_Notify(WINE_WAVEOUT* wwo, WORD uDevID, BOOL force)
{
    LPWAVEHDR		lpWaveHdr;
    count_info 		cinfo;
    
    /* get effective number of written bytes */
    if (!force) {
	if (ioctl(wwo->unixdev, SNDCTL_DSP_GETOPTR, &cinfo) == -1) {
	    perror("ioctl SNDCTL_DSP_GETOPTR");
	    wwo->hThread = 0;
	    LeaveCriticalSection(&wwo->critSect);
	    ExitThread(-1);
	}
	TRACE(wave, "Played %d bytes\n", cinfo.bytes);
	/* FIXME: this will not work if a RESET or SYNC occurs somewhere */
	wwo->dwTotalPlayed = cinfo.bytes;
    }
    if (force || cinfo.bytes > wwo->dwNotifiedBytes) {
	/* remove all wavehdr which can be notified */
	while (wwo->lpNotifyHdr && 
	       (force || (cinfo.bytes >= wwo->dwNotifiedBytes + wwo->lpNotifyHdr->dwBufferLength))) {
	    lpWaveHdr = wwo->lpNotifyHdr;
	    
	    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
	    lpWaveHdr->dwFlags |= WHDR_DONE;
	    if (!force)
		wwo->dwNotifiedBytes += lpWaveHdr->dwBufferLength;
	    wwo->lpNotifyHdr = lpWaveHdr->lpNext;
	    
	    /* FIXME: should we keep the wwo->critsect locked during callback ? */
	    /* normally yes, only a limited set of functions can be called...
	     * the ones which can be called inside an i386 interruption
	     * so this should be fine
	     */
	    LeaveCriticalSection(&wwo->critSect);
	    TRACE(wave, "Notifying client with %p\n", lpWaveHdr);
	    if (WAVE_NotifyClient(uDevID, WOM_DONE, lpWaveHdr->reserved, 0) != MMSYSERR_NOERROR) {
		WARN(wave, "can't notify client !\n");
	    }
	    EnterCriticalSection(&wwo->critSect);
	}
    }
}

/**************************************************************************
 * 				wodPlayer			[internal]
 */
static	DWORD	WINAPI	wodPlayer(LPVOID pmt)
{
    WORD		uDevID = (DWORD)pmt;
    WINE_WAVEOUT*	wwo = (WINE_WAVEOUT*)&WOutDev[uDevID];
    int			laststate = -1;
    WAVEHDR*		lpWaveHdr;
    DWORD		dwSleepTime;

    wwo->dwNotifiedBytes = 0;
    wwo->dwOffCurrHdr = 0;
    wwo->dwRemain = wwo->dwFragmentSize;
    wwo->lpNotifyHdr = NULL;

    /* make sleep time to be # of ms to output a fragment */
    dwSleepTime = (wwo->dwFragmentSize * 1000) / wwo->format.wf.nAvgBytesPerSec;

    for (;;) {
	
	EnterCriticalSection(&wwo->critSect);
	
	switch (wwo->state) {
	case WINE_WS_PAUSED:
	    wodPlayer_Notify(wwo, uDevID, FALSE);
	    if (laststate != WINE_WS_PAUSED) {
		if (ioctl(wwo->unixdev, SNDCTL_DSP_POST, 0) == -1) {
		    perror("ioctl SNDCTL_DSP_POST");
		    wwo->hThread = 0;
		    LeaveCriticalSection(&wwo->critSect);
		    ExitThread(-1);
		}
	    }
	    break;
	case WINE_WS_STOPPED:
	    /* should we kill the Thread here and to restart it later when needed ? */
	    break;
	case WINE_WS_PLAYING:
	    if (!wodPlayer_WriteFragments(wwo)) {
		wodPlayer_Notify(wwo, uDevID, FALSE);
		break;
	    }
	    /* fall thru */
	case WINE_WS_RESETTING:
	    /* updates current notify list */
	    wodPlayer_Notify(wwo, uDevID, FALSE);
	    /* flush all possible output */
	    if (ioctl(wwo->unixdev, SNDCTL_DSP_SYNC, 0) == -1) {
		perror("ioctl SNDCTL_DSP_SYNC");
		wwo->hThread = 0;
		LeaveCriticalSection(&wwo->critSect);
		ExitThread(-1);
	    }
	    /* DSP_SYNC resets the number of send bytes */
	    wwo->dwNotifiedBytes = 0;
	    /* empty notify list */
	    wodPlayer_Notify(wwo, uDevID, TRUE);
	    if (wwo->lpNotifyHdr) {
		ERR(wave, "out of sync\n");
	    }
	    wwo->dwOffCurrHdr = 0;
	    /* get rid also of all the current queue */
	    for (lpWaveHdr = wwo->lpQueueHdr; lpWaveHdr; lpWaveHdr = lpWaveHdr->lpNext) {
		lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
		lpWaveHdr->dwFlags |= WHDR_DONE;
		
		if (WAVE_NotifyClient((DWORD)pmt, WOM_DONE, lpWaveHdr->reserved, 0) != MMSYSERR_NOERROR) {
		    WARN(wave, "can't notify client !\n");
		}
	    }
	    wwo->state = WINE_WS_STOPPED;
	    break;
	case WINE_WS_CLOSING:
	    /* this should not happen since the device must have been reset before */
	    if (wwo->lpNotifyHdr || wwo->lpQueueHdr) {
		ERR(wave, "out of sync\n");
	    }
	    wwo->hThread = 0;
	    wwo->state = WINE_WS_CLOSED;
	    LeaveCriticalSection(&wwo->critSect);
	    DeleteCriticalSection(&wwo->critSect);
	    ExitThread(0);
	    /* shouldn't go here */
	default:
	    FIXME(wave, "unknown state %d\n", wwo->state);
	    break;
	}
	laststate = wwo->state;
	LeaveCriticalSection(&wwo->critSect);
	/* sleep for one fragment to be sent */
	Sleep(dwSleepTime);
    }
    ExitThread(0);
}

/**************************************************************************
 * 			wodGetDevCaps				[internal]
 */
static DWORD wodGetDevCaps(WORD wDevID, LPWAVEOUTCAPS16 lpCaps, DWORD dwSize)
{
    int 	audio;
    int		smplrate;
    int		samplesize = 16;
    int		dsp_stereo = 1;
    int		bytespersmpl;
    int 	caps;
    
    TRACE(wave, "(%u, %p, %lu);\n", wDevID, lpCaps, dwSize);
    
    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;
    if (access(SOUND_DEV,0) != 0) return MMSYSERR_NOTENABLED;
    
    if (wDevID >= MAX_WAVEOUTDRV) {
	TRACE(wave, "MAX_WAVOUTDRV reached !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    if (WOutDev[wDevID].unixdev == 0) {
	audio = open(SOUND_DEV, O_WRONLY, 0);
	if (audio == -1) return MMSYSERR_ALLOCATED;
    } else {
	audio = WOutDev[wDevID].unixdev;
    }

    /* FIXME: some programs compare this string against the content of the registry
     * for MM drivers. The name have to match in order the program to work 
     * (e.g. MS win9x mplayer.exe)
     */
#ifdef EMULATE_SB16
    lpCaps->wMid = 0x0002;
    lpCaps->wPid = 0x0104;
    strcpy(lpCaps->szPname, "SB16 Wave Out");
#else
    lpCaps->wMid = 0x00FF; 	/* Manufac ID */
    lpCaps->wPid = 0x0001; 	/* Product ID */
    /*    strcpy(lpCaps->szPname, "OpenSoundSystem WAVOUT Driver");*/
    strcpy(lpCaps->szPname, "CS4236/37/38");
#endif
    lpCaps->vDriverVersion = 0x0100;
    lpCaps->dwFormats = 0x00000000;
    lpCaps->dwSupport = WAVECAPS_VOLUME;
    
    /* First bytespersampl, then stereo */
    bytespersmpl = (IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, samplesize) != 0) ? 1 : 2;
    
    lpCaps->wChannels = (IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo) != 0) ? 1 : 2;
    if (lpCaps->wChannels > 1) lpCaps->dwSupport |= WAVECAPS_LRVOLUME;
    
    smplrate = 44100;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	lpCaps->dwFormats |= WAVE_FORMAT_4M08;
	if (lpCaps->wChannels > 1)
	    lpCaps->dwFormats |= WAVE_FORMAT_4S08;
	if (bytespersmpl > 1) {
	    lpCaps->dwFormats |= WAVE_FORMAT_4M16;
	    if (lpCaps->wChannels > 1)
		lpCaps->dwFormats |= WAVE_FORMAT_4S16;
	}
    }
    smplrate = 22050;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	lpCaps->dwFormats |= WAVE_FORMAT_2M08;
	if (lpCaps->wChannels > 1)
	    lpCaps->dwFormats |= WAVE_FORMAT_2S08;
	if (bytespersmpl > 1) {
	    lpCaps->dwFormats |= WAVE_FORMAT_2M16;
	    if (lpCaps->wChannels > 1)
		lpCaps->dwFormats |= WAVE_FORMAT_2S16;
	}
    }
    smplrate = 11025;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	lpCaps->dwFormats |= WAVE_FORMAT_1M08;
	if (lpCaps->wChannels > 1)
	    lpCaps->dwFormats |= WAVE_FORMAT_1S08;
	if (bytespersmpl > 1) {
	    lpCaps->dwFormats |= WAVE_FORMAT_1M16;
	    if (lpCaps->wChannels > 1)
		lpCaps->dwFormats |= WAVE_FORMAT_1S16;
	}
    }
    if (IOCTL(audio, SNDCTL_DSP_GETCAPS, caps) == 0) {
	if ((caps & DSP_CAP_REALTIME) && !(caps && DSP_CAP_BATCH))
	    lpCaps->dwFormats |= WAVECAPS_SAMPLEACCURATE;
    }
    if (WOutDev[wDevID].unixdev == 0) {
	close(audio);
    }
    TRACE(wave, "dwFormats = %08lX\n", lpCaps->dwFormats);
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 * 				wodOpen				[internal]
 */
static DWORD wodOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    int 	 	audio, abuf_size, smplrate, samplesize, dsp_stereo;
    LPWAVEFORMAT	lpFormat;
    
    TRACE(wave, "(%u, %p, %08lX);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN(wave, "Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_WAVEOUTDRV) {
	TRACE(wave, "MAX_WAVOUTDRV reached !\n");
	return MMSYSERR_BADDEVICEID;
    }
    WOutDev[wDevID].unixdev = 0;
    if (access(SOUND_DEV, 0) != 0) 
	return MMSYSERR_NOTENABLED;
    audio = open(SOUND_DEV, O_WRONLY, 0);
    if (audio == -1) {
	WARN(wave, "can't open !\n");
	return MMSYSERR_ALLOCATED ;
    }
    IOCTL(audio, SNDCTL_DSP_GETBLKSIZE, abuf_size);
    if (abuf_size < 1024 || abuf_size > 65536) {
	if (abuf_size == -1)
	    WARN(wave, "IOCTL can't 'SNDCTL_DSP_GETBLKSIZE' !\n");
	else
	    WARN(wave, "SNDCTL_DSP_GETBLKSIZE Invalid dwFragmentSize !\n");
	return MMSYSERR_NOTENABLED;
    }
    WOutDev[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
    
    /* FIXME: copy lpFormat too? */
    memcpy(&WOutDev[wDevID].waveDesc, lpDesc, sizeof(WAVEOPENDESC));
    TRACE(wave, "lpDesc->lpFormat = %p\n", lpDesc->lpFormat);
    lpFormat = lpDesc->lpFormat; 
    TRACE(wave, "lpFormat = %p\n", lpFormat);
    if (lpFormat->wFormatTag != WAVE_FORMAT_PCM) {
	WARN(wave, "Bad format %04X !\n", lpFormat->wFormatTag);
	WARN(wave, "Bad nChannels %d !\n", lpFormat->nChannels);
	WARN(wave, "Bad nSamplesPerSec %ld !\n", lpFormat->nSamplesPerSec);
	return WAVERR_BADFORMAT;
    }
    memcpy(&WOutDev[wDevID].format, lpFormat, sizeof(PCMWAVEFORMAT));
    if (WOutDev[wDevID].format.wf.nChannels == 0) return WAVERR_BADFORMAT;
    if (WOutDev[wDevID].format.wf.nSamplesPerSec == 0) return WAVERR_BADFORMAT;
    TRACE(wave, "wBitsPerSample=%u !\n", WOutDev[wDevID].format.wBitsPerSample);
    
    if (WOutDev[wDevID].format.wBitsPerSample == 0) {
	WOutDev[wDevID].format.wBitsPerSample = 8 *
	    (WOutDev[wDevID].format.wf.nAvgBytesPerSec /
	     WOutDev[wDevID].format.wf.nSamplesPerSec) /
	    WOutDev[wDevID].format.wf.nChannels;
    }
    
    WOutDev[wDevID].lpQueueHdr = NULL;
    WOutDev[wDevID].unixdev = audio;
    WOutDev[wDevID].dwTotalPlayed = 0;
    WOutDev[wDevID].dwFragmentSize = abuf_size;
    WOutDev[wDevID].state = WINE_WS_STOPPED;
    WOutDev[wDevID].hThread = 0;
    
    samplesize = WOutDev[wDevID].format.wBitsPerSample;
    smplrate = WOutDev[wDevID].format.wf.nSamplesPerSec;
    dsp_stereo = (WOutDev[wDevID].format.wf.nChannels > 1) ? TRUE : FALSE;
    
    /* First size and stereo then samplerate */
    IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, samplesize);
    IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo);
    IOCTL(audio, SNDCTL_DSP_SPEED, smplrate);
    
    /* FIXME: do we need to make critsect global ? */
    InitializeCriticalSection(&WOutDev[wDevID].critSect);
    MakeCriticalSectionGlobal(&WOutDev[wDevID].critSect);
    
    TRACE(wave, "wBitsPerSample=%u !\n", WOutDev[wDevID].format.wBitsPerSample);
    TRACE(wave, "nAvgBytesPerSec=%lu !\n", WOutDev[wDevID].format.wf.nAvgBytesPerSec);
    TRACE(wave, "nSamplesPerSec=%lu !\n", WOutDev[wDevID].format.wf.nSamplesPerSec);
    TRACE(wave, "nChannels=%u !\n", WOutDev[wDevID].format.wf.nChannels);
    
    if (WAVE_NotifyClient(wDevID, WOM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN(wave, "can't notify client !\n");
	return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodClose			[internal]
 */
static DWORD wodClose(WORD wDevID)
{
    DWORD	ret = MMSYSERR_NOERROR;
    
    TRACE(wave, "(%u);\n", wDevID);
    
    if (wDevID > MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN(wave, "bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    if (WOutDev[wDevID].lpQueueHdr != NULL) {
	WARN(wave, "buffers still playing !\n");
	ret = WAVERR_STILLPLAYING;
    } else {
	EnterCriticalSection(&WOutDev[wDevID].critSect);
	WOutDev[wDevID].state = WINE_WS_CLOSING;
	LeaveCriticalSection(&WOutDev[wDevID].critSect);
	
	while (WOutDev[wDevID].state != WINE_WS_CLOSED)
	    Sleep(10);
	
	close(WOutDev[wDevID].unixdev);
	WOutDev[wDevID].unixdev = 0;
	WOutDev[wDevID].dwFragmentSize = 0;
	if (WAVE_NotifyClient(wDevID, WOM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
	    WARN(wave, "can't notify client !\n");
	    ret = MMSYSERR_INVALPARAM;
	}
    }
    return ret;
}

/**************************************************************************
 * 				wodWrite			[internal]
 * 
 */
static DWORD wodWrite(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE(wave, "(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    
    /* first, do the sanity checks... */
    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
        WARN(wave, "bad dev ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    if (lpWaveHdr->lpData == NULL || !(lpWaveHdr->dwFlags & WHDR_PREPARED)) 
	return WAVERR_UNPREPARED;
    
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE) 
	return WAVERR_STILLPLAYING;
    
    lpWaveHdr->dwFlags &= ~WHDR_DONE;
    lpWaveHdr->dwFlags |= WHDR_INQUEUE;
    lpWaveHdr->lpNext = 0;
    
    EnterCriticalSection(&WOutDev[wDevID].critSect);
    /* insert buffer in queue */
    if (WOutDev[wDevID].lpQueueHdr) {
	WAVEHDR*	wh;
	for (wh = WOutDev[wDevID].lpQueueHdr; wh->lpNext; wh = wh->lpNext);
	wh->lpNext = lpWaveHdr;
    } else {
	WOutDev[wDevID].lpQueueHdr = lpWaveHdr;
    }
    /* starts thread if wod is not paused */
    switch (WOutDev[wDevID].state) {
    case WINE_WS_PAUSED: 
    case WINE_WS_PLAYING: 
	/* nothing to do */ 
	break;
    case WINE_WS_STOPPED:
	if (WOutDev[wDevID].hThread == 0) {
	    WOutDev[wDevID].state = WINE_WS_PLAYING;
	    WOutDev[wDevID].hThread = CreateThread(NULL, 0, wodPlayer, (LPVOID)(DWORD)wDevID, 0, NULL);
	}
	break;
    default:
	FIXME(wave, "Unsupported state %d\n", WOutDev[wDevID].state);
    }
    LeaveCriticalSection(&WOutDev[wDevID].critSect);
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodPrepare			[internal]
 */
static DWORD wodPrepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE(wave, "(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    
    if (wDevID > MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN(wave, "bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE)
	return WAVERR_STILLPLAYING;
    
    lpWaveHdr->dwFlags |= WHDR_PREPARED;
    lpWaveHdr->dwFlags &= ~WHDR_DONE;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodUnprepare			[internal]
 */
static DWORD wodUnprepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE(wave, "(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    
    if (wDevID > MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN(wave, "bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE)
	return WAVERR_STILLPLAYING;
    
    lpWaveHdr->dwFlags &= ~WHDR_PREPARED;
    lpWaveHdr->dwFlags |= WHDR_DONE;
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodPause				[internal]
 */
static DWORD wodPause(WORD wDevID)
{
    TRACE(wave, "(%u);!\n", wDevID);
    
    if (wDevID > MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN(wave, "bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    EnterCriticalSection(&WOutDev[wDevID].critSect);
    WOutDev[wDevID].state = WINE_WS_PAUSED;
    LeaveCriticalSection(&WOutDev[wDevID].critSect);
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodStop					[internal]
 */
static DWORD wodStop(WORD wDevID)
{
    FIXME(wave, "(%u); NIY!\n", wDevID);
    
    if (wDevID > MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN(wave, "bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodRestart				[internal]
 */
static DWORD wodRestart(WORD wDevID)
{
    TRACE(wave, "(%u);\n", wDevID);
    
    if (wDevID > MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN(wave, "bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    EnterCriticalSection(&WOutDev[wDevID].critSect);
    if (WOutDev[wDevID].state == WINE_WS_PAUSED) {
	if (WOutDev[wDevID].lpQueueHdr != NULL) {
	    WOutDev[wDevID].state = WINE_WS_PLAYING;
	    if (WOutDev[wDevID].hThread == 0)
		WOutDev[wDevID].hThread = CreateThread(NULL, 0, wodPlayer, (LPVOID)(DWORD)wDevID, 0, NULL);
	} else {
	    WOutDev[wDevID].state = WINE_WS_STOPPED;
	}
    }
    LeaveCriticalSection(&WOutDev[wDevID].critSect);
    
    /* FIXME: is NotifyClient with WOM_DONE right ? (Comet Busters 1.3.3 needs this notification) */
    /* FIXME: Myst crashes with this ... hmm -MM
       if (WAVE_NotifyClient(wDevID, WOM_DONE, 0L, 0L) != MMSYSERR_NOERROR) {
       WARN(wave, "can't notify client !\n");
       return MMSYSERR_INVALPARAM;
       }
    */
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodReset				[internal]
 */
static DWORD wodReset(WORD wDevID)
{
    TRACE(wave, "(%u);\n", wDevID);
    
    if (wDevID > MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN(wave, "bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    EnterCriticalSection(&WOutDev[wDevID].critSect);
    if (WOutDev[wDevID].hThread == 0) {
	WOutDev[wDevID].state = WINE_WS_STOPPED;
    } else {
	WOutDev[wDevID].state = WINE_WS_RESETTING;
    }
    LeaveCriticalSection(&WOutDev[wDevID].critSect);
    
    while (WOutDev[wDevID].state != WINE_WS_STOPPED)
	Sleep(10);
    
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 * 				wodGetPosition			[internal]
 */
static DWORD wodGetPosition(WORD wDevID, LPMMTIME16 lpTime, DWORD uSize)
{
    int		time;
    
    TRACE(wave, "(%u, %p, %lu);\n", wDevID, lpTime, uSize);
    
    if (wDevID > MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN(wave, "bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;
    TRACE(wave, "wType=%04X wBitsPerSample=%u nSamplesPerSec=%lu nChannels=%u nAvgBytesPerSec=%lu\n", 
	  lpTime->wType, WOutDev[wDevID].format.wBitsPerSample, 
	  WOutDev[wDevID].format.wf.nSamplesPerSec, WOutDev[wDevID].format.wf.nChannels, 
	  WOutDev[wDevID].format.wf.nAvgBytesPerSec); 
    TRACE(wave, "dwTotalPlayed=%lu\n", WOutDev[wDevID].dwTotalPlayed);
    
    switch (lpTime->wType) {
    case TIME_BYTES:
	lpTime->u.cb = WOutDev[wDevID].dwTotalPlayed;
	TRACE(wave, "TIME_BYTES=%lu\n", lpTime->u.cb);
	break;
    case TIME_SAMPLES:
	lpTime->u.sample = WOutDev[wDevID].dwTotalPlayed * 8 /
	    WOutDev[wDevID].format.wBitsPerSample;
	TRACE(wave, "TIME_SAMPLES=%lu\n", lpTime->u.sample);
	break;
    case TIME_SMPTE:
	time = WOutDev[wDevID].dwTotalPlayed /
	    (WOutDev[wDevID].format.wf.nAvgBytesPerSec / 1000);
	lpTime->u.smpte.hour = time / 108000;
	time -= lpTime->u.smpte.hour * 108000;
	lpTime->u.smpte.min = time / 1800;
	time -= lpTime->u.smpte.min * 1800;
	lpTime->u.smpte.sec = time / 30;
	time -= lpTime->u.smpte.sec * 30;
	lpTime->u.smpte.frame = time;
	lpTime->u.smpte.fps = 30;
	TRACE(wave, "TIME_SMPTE=%02u:%02u:%02u:%02u\n",
	      lpTime->u.smpte.hour, lpTime->u.smpte.min,
	      lpTime->u.smpte.sec, lpTime->u.smpte.frame);
	break;
    default:
	FIXME(wave, "Format %d not supported ! use TIME_MS !\n", lpTime->wType);
	lpTime->wType = TIME_MS;
    case TIME_MS:
	lpTime->u.ms = WOutDev[wDevID].dwTotalPlayed /
	    (WOutDev[wDevID].format.wf.nAvgBytesPerSec / 1000);
	TRACE(wave, "TIME_MS=%lu\n", lpTime->u.ms);
	break;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodGetVolume			[internal]
 */
static DWORD wodGetVolume(WORD wDevID, LPDWORD lpdwVol)
{
    int 	mixer;
    int		volume, left, right;
    
    TRACE(wave, "(%u, %p);\n", wDevID, lpdwVol);
    
    if (lpdwVol == NULL) 
	return MMSYSERR_NOTENABLED;
    if ((mixer = open(MIXER_DEV, O_RDONLY)) < 0) {
	WARN(wave, "mixer device not available !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, SOUND_MIXER_READ_PCM, &volume) == -1) {
	WARN(wave, "unable read mixer !\n");
	return MMSYSERR_NOTENABLED;
    }
    close(mixer);
    left = volume & 0x7F;
    right = (volume >> 8) & 0x7F;
    TRACE(wave, "left=%d right=%d !\n", left, right);
    *lpdwVol = MAKELONG(left << 9, right << 9);
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 * 				wodSetVolume			[internal]
 */
static DWORD wodSetVolume(WORD wDevID, DWORD dwParam)
{
    int 	mixer;
    int		volume;
    TRACE(wave, "(%u, %08lX);\n", wDevID, dwParam);
    volume = (LOWORD(dwParam) >> 9 & 0x7F) + 
	((HIWORD(dwParam) >> 9  & 0x7F) << 8);
    if ((mixer = open(MIXER_DEV, O_WRONLY)) < 0) {
	WARN(wave, "mixer device not available !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, SOUND_MIXER_WRITE_PCM, &volume) == -1) {
	WARN(wave, "unable set mixer !\n");
	return MMSYSERR_NOTENABLED;
    }
    close(mixer);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodGetNumDevs			[internal]
 */
static	DWORD	wodGetNumDevs(void)
{
    DWORD	ret = 1;
    
    /* FIXME: For now, only one sound device (SOUND_DEV) is allowed */
    int audio = open(SOUND_DEV, O_WRONLY, 0);
    
    if (audio == -1) {
	if (errno != EBUSY)
	    ret = 0;
    } else {
	close(audio);
    }
    return ret;
}

/**************************************************************************
 * 				wodMessage			[sample driver]
 */
DWORD WINAPI wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			DWORD dwParam1, DWORD dwParam2)
{
    TRACE(wave, "(%u, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);
    
    switch (wMsg) {
    case WODM_OPEN:	 	return wodOpen		(wDevID, (LPWAVEOPENDESC)dwParam1,	dwParam2);
    case WODM_CLOSE:	 	return wodClose		(wDevID);
    case WODM_WRITE:	 	return wodWrite		(wDevID, (LPWAVEHDR)dwParam1,		dwParam2);
    case WODM_PAUSE:	 	return wodPause		(wDevID);
    case WODM_STOP:	 	return wodStop		(wDevID);
    case WODM_GETPOS:	 	return wodGetPosition	(wDevID, (LPMMTIME16)dwParam1, 		dwParam2);
    case WODM_BREAKLOOP: 	return MMSYSERR_NOTSUPPORTED;
    case WODM_PREPARE:	 	return wodPrepare	(wDevID, (LPWAVEHDR)dwParam1, 		dwParam2);
    case WODM_UNPREPARE: 	return wodUnprepare	(wDevID, (LPWAVEHDR)dwParam1, 		dwParam2);
    case WODM_GETDEVCAPS:	return wodGetDevCaps	(wDevID, (LPWAVEOUTCAPS16)dwParam1,	dwParam2);
    case WODM_GETNUMDEVS:	return wodGetNumDevs	();
    case WODM_GETPITCH:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPITCH:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETVOLUME:	return wodGetVolume	(wDevID, (LPDWORD)dwParam1);
    case WODM_SETVOLUME:	return wodSetVolume	(wDevID, dwParam1);
    case WODM_RESTART:		return wodRestart	(wDevID);
    case WODM_RESET:		return wodReset		(wDevID);
    default:
	FIXME(wave, "unknown message %d!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*======================================================================*
 *                  Low level WAVE IN implemantation			*
 *======================================================================*/

/**************************************************************************
 * 			widGetDevCaps				[internal]
 */
static DWORD widGetDevCaps(WORD wDevID, LPWAVEINCAPS16 lpCaps, DWORD dwSize)
{
    int 	audio,smplrate,samplesize=16,dsp_stereo=1,bytespersmpl;
    
    TRACE(wave, "(%u, %p, %lu);\n", wDevID, lpCaps, dwSize);
    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;
    if (access(SOUND_DEV,0) != 0) return MMSYSERR_NOTENABLED;
    audio = open(SOUND_DEV, O_RDONLY, 0);
    if (audio == -1) return MMSYSERR_ALLOCATED ;
#ifdef EMULATE_SB16
    lpCaps->wMid = 0x0002;
    lpCaps->wPid = 0x0004;
    strcpy(lpCaps->szPname, "SB16 Wave In");
#else
    lpCaps->wMid = 0x00FF; 	/* Manufac ID */
    lpCaps->wPid = 0x0001; 	/* Product ID */
    strcpy(lpCaps->szPname, "OpenSoundSystem WAVIN Driver");
#endif
    lpCaps->dwFormats = 0x00000000;
    lpCaps->wChannels = (IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo) != 0) ? 1 : 2;
    bytespersmpl = (IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, samplesize) != 0) ? 1 : 2;
    smplrate = 44100;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	lpCaps->dwFormats |= WAVE_FORMAT_4M08;
	if (lpCaps->wChannels > 1)
	    lpCaps->dwFormats |= WAVE_FORMAT_4S08;
	if (bytespersmpl > 1) {
	    lpCaps->dwFormats |= WAVE_FORMAT_4M16;
	    if (lpCaps->wChannels > 1)
		lpCaps->dwFormats |= WAVE_FORMAT_4S16;
	}
    }
    smplrate = 22050;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	lpCaps->dwFormats |= WAVE_FORMAT_2M08;
	if (lpCaps->wChannels > 1)
	    lpCaps->dwFormats |= WAVE_FORMAT_2S08;
	if (bytespersmpl > 1) {
	    lpCaps->dwFormats |= WAVE_FORMAT_2M16;
	    if (lpCaps->wChannels > 1)
		lpCaps->dwFormats |= WAVE_FORMAT_2S16;
	}
    }
    smplrate = 11025;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	lpCaps->dwFormats |= WAVE_FORMAT_1M08;
	if (lpCaps->wChannels > 1)
	    lpCaps->dwFormats |= WAVE_FORMAT_1S08;
	if (bytespersmpl > 1) {
	    lpCaps->dwFormats |= WAVE_FORMAT_1M16;
	    if (lpCaps->wChannels > 1)
		lpCaps->dwFormats |= WAVE_FORMAT_1S16;
	}
    }
    close(audio);
    TRACE(wave, "dwFormats = %08lX\n", lpCaps->dwFormats);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widOpen				[internal]
 */
static DWORD widOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    int 		audio,abuf_size,smplrate,samplesize,dsp_stereo;
    LPWAVEFORMAT	lpFormat;
    
    TRACE(wave, "(%u, %p, %08lX);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN(wave, "Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_WAVEINDRV) {
	TRACE(wave, "MAX_WAVINDRV reached !\n");
	return MMSYSERR_ALLOCATED;
    }
    WInDev[wDevID].unixdev = 0;
    if (access(SOUND_DEV,0) != 0) return MMSYSERR_NOTENABLED;
    audio = open(SOUND_DEV, O_RDONLY, 0);
    if (audio == -1) {
	WARN(wave, "can't open !\n");
	return MMSYSERR_ALLOCATED;
    }
    IOCTL(audio, SNDCTL_DSP_GETBLKSIZE, abuf_size);
    if (abuf_size < 1024 || abuf_size > 65536) {
	if (abuf_size == -1)
	    WARN(wave, "IOCTL can't 'SNDCTL_DSP_GETBLKSIZE' !\n");
	else
	    WARN(wave, "SNDCTL_DSP_GETBLKSIZE Invalid dwFragmentSize !\n");
	return MMSYSERR_NOTENABLED;
    }
    WInDev[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    if (WInDev[wDevID].lpQueueHdr) {
	HeapFree(GetProcessHeap(), 0, WInDev[wDevID].lpQueueHdr);
	WInDev[wDevID].lpQueueHdr = NULL;
    }
    WInDev[wDevID].unixdev = audio;
    WInDev[wDevID].dwFragmentSize = abuf_size;
    WInDev[wDevID].dwTotalRecorded = 0;
    memcpy(&WInDev[wDevID].waveDesc, lpDesc, sizeof(WAVEOPENDESC));
    lpFormat = (LPWAVEFORMAT) lpDesc->lpFormat; 
    if (lpFormat->wFormatTag != WAVE_FORMAT_PCM) {
	WARN(wave, "Bad format %04X !\n",
	     lpFormat->wFormatTag);
	return WAVERR_BADFORMAT;
    }
    memcpy(&WInDev[wDevID].format, lpFormat, sizeof(PCMWAVEFORMAT));
    WInDev[wDevID].format.wBitsPerSample = 8; /* <-------------- */
    if (WInDev[wDevID].format.wf.nChannels == 0) return WAVERR_BADFORMAT;
    if (WInDev[wDevID].format.wf.nSamplesPerSec == 0) return WAVERR_BADFORMAT;
    if (WInDev[wDevID].format.wBitsPerSample == 0) {
	WInDev[wDevID].format.wBitsPerSample = 8 *
	    (WInDev[wDevID].format.wf.nAvgBytesPerSec /
	     WInDev[wDevID].format.wf.nSamplesPerSec) /
	    WInDev[wDevID].format.wf.nChannels;
    }
    samplesize = WInDev[wDevID].format.wBitsPerSample;
    smplrate = WInDev[wDevID].format.wf.nSamplesPerSec;
    dsp_stereo = (WInDev[wDevID].format.wf.nChannels > 1) ? TRUE : FALSE;
    IOCTL(audio, SNDCTL_DSP_SPEED, smplrate);
    IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, samplesize);
    IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo);
    TRACE(wave, "wBitsPerSample=%u !\n", WInDev[wDevID].format.wBitsPerSample);
    TRACE(wave, "nSamplesPerSec=%lu !\n", WInDev[wDevID].format.wf.nSamplesPerSec);
    TRACE(wave, "nChannels=%u !\n", WInDev[wDevID].format.wf.nChannels);
    TRACE(wave, "nAvgBytesPerSec=%lu\n", WInDev[wDevID].format.wf.nAvgBytesPerSec); 
    if (WAVE_NotifyClient(wDevID, WIM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN(wave, "can't notify client !\n");
	return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widClose			[internal]
 */
static DWORD widClose(WORD wDevID)
{
    TRACE(wave, "(%u);\n", wDevID);
    if (wDevID > MAX_WAVEINDRV) return MMSYSERR_INVALPARAM;
    if (WInDev[wDevID].unixdev == 0) {
	WARN(wave, "can't close !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (WInDev[wDevID].lpQueueHdr != NULL) {
	WARN(wave, "still buffers open !\n");
	return WAVERR_STILLPLAYING;
    }
    close(WInDev[wDevID].unixdev);
    WInDev[wDevID].unixdev = 0;
    WInDev[wDevID].dwFragmentSize = 0;
    if (WAVE_NotifyClient(wDevID, WIM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN(wave, "can't notify client !\n");
	return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widAddBuffer		[internal]
 */
static DWORD widAddBuffer(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    int		count	= 1;
    LPWAVEHDR 	lpWIHdr;
    
    TRACE(wave, "(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    if (WInDev[wDevID].unixdev == 0) {
	WARN(wave, "can't do it !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (!(lpWaveHdr->dwFlags & WHDR_PREPARED)) {
	TRACE(wave, "never been prepared !\n");
	return WAVERR_UNPREPARED;
    }
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE) {
	TRACE(wave,	"header already in use !\n");
	return WAVERR_STILLPLAYING;
    }
    lpWaveHdr->dwFlags |= WHDR_PREPARED;
    lpWaveHdr->dwFlags |= WHDR_INQUEUE;
    lpWaveHdr->dwFlags &= ~WHDR_DONE;
    lpWaveHdr->dwBytesRecorded = 0;
    if (WInDev[wDevID].lpQueueHdr == NULL) {
	WInDev[wDevID].lpQueueHdr = lpWaveHdr;
    } else {
	lpWIHdr = WInDev[wDevID].lpQueueHdr;
	while (lpWIHdr->lpNext != NULL) {
	    lpWIHdr = lpWIHdr->lpNext;
	    count++;
	}
	lpWIHdr->lpNext = lpWaveHdr;
	lpWaveHdr->lpNext = NULL;
	count++;
    }
    TRACE(wave, "buffer added ! (now %u in queue)\n", count);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widPrepare			[internal]
 */
static DWORD widPrepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE(wave, "(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    if (WInDev[wDevID].unixdev == 0) {
	WARN(wave, "can't prepare !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE)
	return WAVERR_STILLPLAYING;
    lpWaveHdr->dwFlags |= WHDR_PREPARED;
    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
    lpWaveHdr->dwFlags &= ~WHDR_DONE;
    lpWaveHdr->dwBytesRecorded = 0;
    TRACE(wave, "header prepared !\n");
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widUnprepare			[internal]
 */
static DWORD widUnprepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE(wave, "(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    if (WInDev[wDevID].unixdev == 0) {
	WARN(wave, "can't unprepare !\n");
	return MMSYSERR_NOTENABLED;
    }
    lpWaveHdr->dwFlags &= ~WHDR_PREPARED;
    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
    lpWaveHdr->dwFlags |= WHDR_DONE;
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widStart				[internal]
 */
static DWORD widStart(WORD wDevID)
{
    int		count	= 1;
    int         bytesRead;
    LPWAVEHDR	*lpWaveHdr;
    LPBYTE	lpData;

    TRACE(wave, "(%u);\n", wDevID);
    if (WInDev[wDevID].unixdev == 0) {
	WARN(wave, "can't start recording !\n");
	return MMSYSERR_NOTENABLED;
    }
    
    lpWaveHdr = &(WInDev[wDevID].lpQueueHdr);
    TRACE(wave, "lpWaveHdr = %08lx\n",(DWORD)lpWaveHdr);

    if (!*lpWaveHdr || !(*lpWaveHdr)->lpData) {
	TRACE(wave, "never been prepared !\n");
	return WAVERR_UNPREPARED;
    }
    
    while (*lpWaveHdr != NULL) {

	lpData = ((DWORD)*lpWaveHdr == (*lpWaveHdr)->reserved) ?
	    (LPBYTE)(*lpWaveHdr)->lpData : (LPBYTE)PTR_SEG_TO_LIN((*lpWaveHdr)->lpData);

	TRACE(wave, "recording buf#%u=%p size=%lu \n",
	      count, lpData, (*lpWaveHdr)->dwBufferLength);
	fflush(stddeb);

	bytesRead = read(WInDev[wDevID].unixdev, lpData, (*lpWaveHdr)->dwBufferLength);

	if (bytesRead == -1)
	    perror("read from audio device");

	TRACE(wave, "bytesread=%d (%ld)\n", bytesRead, (*lpWaveHdr)->dwBufferLength);
	(*lpWaveHdr)->dwBytesRecorded = bytesRead;
	WInDev[wDevID].dwTotalRecorded += (*lpWaveHdr)->dwBytesRecorded;
	(*lpWaveHdr)->dwFlags &= ~WHDR_INQUEUE;
	(*lpWaveHdr)->dwFlags |= WHDR_DONE;
	
	if (WAVE_NotifyClient(wDevID, WIM_DATA, (*lpWaveHdr)->reserved, (*lpWaveHdr)->dwBytesRecorded) != MMSYSERR_NOERROR) {
	    WARN(wave, "can't notify client !\n");
	    return MMSYSERR_INVALPARAM;
	}
	/* removes the current block from the queue */
	*lpWaveHdr = (*lpWaveHdr)->lpNext;
	count++;
    }
    TRACE(wave, "end of recording !\n");
    fflush(stddeb);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widStop					[internal]
 */
static DWORD widStop(WORD wDevID)
{
    TRACE(wave, "(%u);\n", wDevID);
    if (WInDev[wDevID].unixdev == 0) {
	WARN(wave, "can't stop !\n");
	return MMSYSERR_NOTENABLED;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widReset				[internal]
 */
static DWORD widReset(WORD wDevID)
{
    TRACE(wave, "(%u);\n", wDevID);
    if (WInDev[wDevID].unixdev == 0) {
	WARN(wave, "can't reset !\n");
	return MMSYSERR_NOTENABLED;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widGetPosition			[internal]
 */
static DWORD widGetPosition(WORD wDevID, LPMMTIME16 lpTime, DWORD uSize)
{
    int		time;
    
    TRACE(wave, "(%u, %p, %lu);\n", wDevID, lpTime, uSize);
    if (WInDev[wDevID].unixdev == 0) {
	WARN(wave, "can't get pos !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;
    TRACE(wave, "wType=%04X !\n", lpTime->wType);
    TRACE(wave, "wBitsPerSample=%u\n", WInDev[wDevID].format.wBitsPerSample); 
    TRACE(wave, "nSamplesPerSec=%lu\n", WInDev[wDevID].format.wf.nSamplesPerSec); 
    TRACE(wave, "nChannels=%u\n", WInDev[wDevID].format.wf.nChannels); 
    TRACE(wave, "nAvgBytesPerSec=%lu\n", WInDev[wDevID].format.wf.nAvgBytesPerSec); 
    fflush(stddeb);
    switch (lpTime->wType) {
    case TIME_BYTES:
	lpTime->u.cb = WInDev[wDevID].dwTotalRecorded;
	TRACE(wave, "TIME_BYTES=%lu\n", lpTime->u.cb);
	break;
    case TIME_SAMPLES:
	lpTime->u.sample = WInDev[wDevID].dwTotalRecorded * 8 /
	    WInDev[wDevID].format.wBitsPerSample;
	TRACE(wave, "TIME_SAMPLES=%lu\n", lpTime->u.sample);
	break;
    case TIME_SMPTE:
	time = WInDev[wDevID].dwTotalRecorded /
	    (WInDev[wDevID].format.wf.nAvgBytesPerSec / 1000);
	lpTime->u.smpte.hour = time / 108000;
	time -= lpTime->u.smpte.hour * 108000;
	lpTime->u.smpte.min = time / 1800;
	time -= lpTime->u.smpte.min * 1800;
	lpTime->u.smpte.sec = time / 30;
	time -= lpTime->u.smpte.sec * 30;
	lpTime->u.smpte.frame = time;
	lpTime->u.smpte.fps = 30;
	TRACE(wave, "TIME_SMPTE=%02u:%02u:%02u:%02u\n",
	      lpTime->u.smpte.hour, lpTime->u.smpte.min,
	      lpTime->u.smpte.sec, lpTime->u.smpte.frame);
	break;
    case TIME_MS:
	lpTime->u.ms = WInDev[wDevID].dwTotalRecorded /
	    (WInDev[wDevID].format.wf.nAvgBytesPerSec / 1000);
	TRACE(wave, "TIME_MS=%lu\n", lpTime->u.ms);
	break;
    default:
	FIXME(wave, "format not supported (%u) ! use TIME_MS !\n", lpTime->wType);
	lpTime->wType = TIME_MS;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widMessage			[sample driver]
 */
DWORD WINAPI widMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			DWORD dwParam1, DWORD dwParam2)
{
    TRACE(wave, "(%u, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    switch (wMsg) {
    case WIDM_OPEN:		return widOpen(wDevID, (LPWAVEOPENDESC)dwParam1, dwParam2);
    case WIDM_CLOSE:		return widClose(wDevID);
    case WIDM_ADDBUFFER:	return widAddBuffer(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WIDM_PREPARE:		return widPrepare(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WIDM_UNPREPARE:	return widUnprepare(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WIDM_GETDEVCAPS:	return widGetDevCaps(wDevID, (LPWAVEINCAPS16)dwParam1,dwParam2);
    case WIDM_GETNUMDEVS:	return wodGetNumDevs();	/* same number of devices in output as in input */
    case WIDM_GETPOS:		return widGetPosition(wDevID, (LPMMTIME16)dwParam1, dwParam2);
    case WIDM_RESET:		return widReset(wDevID);
    case WIDM_START:		return widStart(wDevID);
    case WIDM_PAUSE:		return widStop(wDevID);
    case WIDM_STOP:		return widStop(wDevID);
    default:
	FIXME(wave, "unknown message %u!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*======================================================================*
 *              Low level WAVE implemantation - DriverProc		*
 *======================================================================*/

/**************************************************************************
 * 				WAVE_DriverProc			[sample driver]
 */
LONG WAVE_DriverProc(DWORD dwDevID, HDRVR16 hDriv, DWORD wMsg, 
		     DWORD dwParam1, DWORD dwParam2)
{
    TRACE(wave, "(%08lX, %04X, %08lX, %08lX, %08lX)\n", dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    switch (wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return 1;
    case DRV_CLOSE:		return 1;
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "Sample MultiMedia Linux Driver !", "MMLinux Driver", MB_OK);	return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    default:
	FIXME(wave, "is probably wrong msg=0x%04lx\n", wMsg);
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return MMSYSERR_NOTENABLED;
}

#else /* !HAVE_OSS */

/**************************************************************************
 * 				wodMessage			[sample driver]
 */
DWORD WINAPI wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			DWORD dwParam1, DWORD dwParam2)
{
    FIXME(wave, "(%u, %04X, %08lX, %08lX, %08lX):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				widMessage			[sample driver]
 */
DWORD WINAPI widMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			DWORD dwParam1, DWORD dwParam2)
{
    FIXME(wave, "(%u, %04X, %08lX, %08lX, %08lX):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				WAVE_DriverProc			[sample driver]
 */
LONG WAVE_DriverProc(DWORD dwDevID, HDRVR16 hDriv, DWORD wMsg, 
		     DWORD dwParam1, DWORD dwParam2)
{
    return MMSYSERR_NOTENABLED;
}
#endif /* HAVE_OSS */
