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
#include "mmddk.h"
#include "oss.h"
#include "heap.h"
#include "ldt.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(wave)

#ifdef HAVE_OSS

#define SOUND_DEV "/dev/dsp"
#define MIXER_DEV "/dev/mixer"

#define MAX_WAVEOUTDRV 	(1)
#define MAX_WAVEINDRV 	(1)

/* state diagram for waveOut writing:
 *
 * +---------+-------------+---------------+---------------------------------+
 * |  state  |  function   |     event     |            new state	     |
 * +---------+-------------+---------------+---------------------------------+
 * |	     | open()	   |		   | STOPPED		       	     |
 * | PAUSED  | write()	   | 		   | PAUSED		       	     |
 * | STOPPED | write()	   | <thrd create> | PLAYING		  	     |
 * | PLAYING | write()	   | HEADER        | PLAYING		  	     |
 * | (other) | write()	   | <error>       |		       		     |
 * | (any)   | pause()	   | PAUSING	   | PAUSED		       	     |
 * | PAUSED  | restart()   | RESTARTING    | PLAYING (if no thrd => STOPPED) |
 * | (any)   | reset()	   | RESETTING     | STOPPED		      	     |
 * | (any)   | close()	   | CLOSING	   | CLOSED		      	     |
 * +---------+-------------+---------------+---------------------------------+
 */

/* states of the playing device */
#define	WINE_WS_PLAYING		0
#define	WINE_WS_PAUSED		1
#define	WINE_WS_STOPPED		2
#define WINE_WS_CLOSED		3

/* events to be send to device */
#define WINE_WM_PAUSING		(WM_USER + 1)
#define WINE_WM_RESTARTING	(WM_USER + 2)
#define WINE_WM_RESETTING	(WM_USER + 3)
#define WINE_WM_CLOSING		(WM_USER + 4)
#define WINE_WM_HEADER		(WM_USER + 5)

typedef struct {
    int				unixdev;
    volatile int		state;			/* one of the WINE_WS_ manifest constants */
    DWORD			dwFragmentSize;		/* size of OSS buffer fragment */
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    PCMWAVEFORMAT		format;
    LPWAVEHDR			lpQueueHdr;		/* pending buffers for playing */
    LPWAVEHDR			lpNotifyHdr;		/* list of wavehdr for which write() has been called, pending for notification */

    DWORD			dwPlayedTotal;		/* number of bytes played since opening */
    DWORD			dwPlayed;		/* number of bytes played since last DSP_RESET */
    DWORD			dwNotifiedBytes;	/* number of bytes for which wavehdr notification has been done */

    /* info on current lpQueueHdr->lpWaveHdr */
    DWORD			dwOffCurrHdr;		/* offset in lpQueueHdr->lpWaveHdr->lpData for fragments */
    DWORD			dwRemain;		/* number of bytes to write to end the current fragment  */

    /* synchronization stuff */
    HANDLE			hThread;
    DWORD			dwThreadID;
    HANDLE			hEvent;

    WORD			wMaxFragments;		/* max number of fragments that can be written to dsp */
    WORD			wFragsUsedInQueue;	/* current number of used fragments inside dsp queue */
} WINE_WAVEOUT;

typedef struct {
    int				unixdev;
    volatile int		state;
    DWORD			dwFragmentSize;		/* OpenSound '/dev/dsp' give us that size */
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
static DWORD WAVE_NotifyClient(UINT wDevID, WORD wMsg, DWORD dwParam1, 
			       DWORD dwParam2)
{
    TRACE("wDevID = %04X wMsg = %d dwParm1 = %04lX dwParam2 = %04lX\n",wDevID, wMsg, dwParam1, dwParam2);
    
    switch (wMsg) {
    case WOM_OPEN:
    case WOM_CLOSE:
    case WOM_DONE:
	if (wDevID > MAX_WAVEOUTDRV) return MCIERR_INTERNAL;
	
	if (WOutDev[wDevID].wFlags != DCB_NULL && 
	    !DriverCallback(WOutDev[wDevID].waveDesc.dwCallback, 
			    WOutDev[wDevID].wFlags, 
			    WOutDev[wDevID].waveDesc.hWave, 
			    wMsg, 
			    WOutDev[wDevID].waveDesc.dwInstance, 
			    dwParam1, 
			    dwParam2)) {
	    WARN("can't notify client !\n");
	    return MMSYSERR_NOERROR;
	}
	break;
	
    case WIM_OPEN:
    case WIM_CLOSE:
    case WIM_DATA:
	if (wDevID > MAX_WAVEINDRV) return MCIERR_INTERNAL;
	
	if (WInDev[wDevID].wFlags != DCB_NULL && 
	    !DriverCallback(WInDev[wDevID].waveDesc.dwCallback, 
			    WInDev[wDevID].wFlags, 
			    WInDev[wDevID].waveDesc.hWave, 
			    wMsg, 
			    WInDev[wDevID].waveDesc.dwInstance, 
			    dwParam1, 
			    dwParam2)) {
	    WARN("can't notify client !\n");
	    return MMSYSERR_NOERROR;
	}
	break;
    default:
	FIXME("Unknown CB message %u\n", wMsg);
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
    LPBYTE		lpData;
    int			count;

    for (;;) {
	TRACE("Fragments: %d used on fd %d\n", wwo->wFragsUsedInQueue, wwo->unixdev);
	if (wwo->wFragsUsedInQueue == wwo->wMaxFragments) 	/* output queue is full, wait a bit */
	    return FALSE;

	lpWaveHdr = wwo->lpQueueHdr;
	if (!lpWaveHdr) {
	    if (wwo->dwRemain > 0 &&				/* still data to send to complete current fragment */
		wwo->dwNotifiedBytes >= wwo->dwFragmentSize &&  /* first fragment has been played */
		wwo->wFragsUsedInQueue < 2) {     		/* done with all waveOutWrite()' fragments */
		/* FIXME: should do better handling here */
		TRACE("Oooch, buffer underrun !\n");
		return TRUE; /* force resetting of waveOut device */
	    }
	    return FALSE;	/* wait a bit */
	}
	
	if (wwo->dwOffCurrHdr == 0) {
	    TRACE("Starting a new wavehdr %p of %ld bytes\n", lpWaveHdr, lpWaveHdr->dwBufferLength);
	    if (lpWaveHdr->dwFlags & WHDR_BEGINLOOP)
		FIXME("NIY: loops (%lu) in wavehdr\n", lpWaveHdr->dwLoops);
	}
	
	lpData = lpWaveHdr->lpData;

	/* finish current wave hdr ? */
	if (wwo->dwOffCurrHdr + wwo->dwRemain >= lpWaveHdr->dwBufferLength) { 
	    DWORD	toWrite = lpWaveHdr->dwBufferLength - wwo->dwOffCurrHdr;
	    
	    /* write end of current wave hdr */
	    count = write(wwo->unixdev, lpData + wwo->dwOffCurrHdr, toWrite);
	    TRACE("write(%p[%5lu], %5lu) => %d\n", lpData, wwo->dwOffCurrHdr, toWrite, count);
	    
	    if (count > 0) {
		LPWAVEHDR*	wh;
		
		/* move lpWaveHdr to the end of notify list */
		for (wh = &(wwo->lpNotifyHdr); *wh; wh = &((*wh)->lpNext));
		    *wh = lpWaveHdr;

		wwo->lpQueueHdr = lpWaveHdr->lpNext;
		lpWaveHdr->lpNext = 0;
		
		wwo->dwOffCurrHdr = 0;
		if ((wwo->dwRemain -= count) == 0) {
		    wwo->dwRemain = wwo->dwFragmentSize;
		    wwo->wFragsUsedInQueue++;
		}
	    }
	    continue; /* try to go to use next wavehdr */
	}  else	{
	    count = write(wwo->unixdev, lpData + wwo->dwOffCurrHdr, wwo->dwRemain);
	    TRACE("write(%p[%5lu], %5lu) => %d\n", lpData, wwo->dwOffCurrHdr, wwo->dwRemain, count);
	    if (count > 0) {
		wwo->dwOffCurrHdr += count;
		wwo->dwRemain = wwo->dwFragmentSize;
		wwo->wFragsUsedInQueue++;
	    }
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
	    wwo->state = WINE_WS_STOPPED;
	    ExitThread(-1);
	}
	TRACE("Played %d bytes (played=%ld) on fd %d\n", cinfo.bytes, wwo->dwPlayed, wwo->unixdev);
	wwo->wFragsUsedInQueue -= cinfo.bytes / wwo->dwFragmentSize - wwo->dwPlayed / wwo->dwFragmentSize;
	wwo->dwPlayed = cinfo.bytes;
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
	    
	    TRACE("Notifying client with %p\n", lpWaveHdr);
	    if (WAVE_NotifyClient(uDevID, WOM_DONE, (DWORD)lpWaveHdr, 0) != MMSYSERR_NOERROR) {
		WARN("can't notify client !\n");
	    }
	}
    }
}

/**************************************************************************
 * 				wodPlayer_Reset			[internal]
 *
 * wodPlayer helper. Resets current output stream.
 */
static	void	wodPlayer_Reset(WINE_WAVEOUT* wwo, WORD uDevID, BOOL reset)
{
    LPWAVEHDR		lpWaveHdr;

    /* updates current notify list */
    wodPlayer_Notify(wwo, uDevID, FALSE);

    /* flush all possible output */
    if (ioctl(wwo->unixdev, SNDCTL_DSP_RESET, 0) == -1) {
	perror("ioctl SNDCTL_DSP_RESET");
	wwo->hThread = 0;
	wwo->state = WINE_WS_STOPPED;
	ExitThread(-1);
    }

    wwo->dwOffCurrHdr = 0;
    if (reset) {
	/* empty notify list */
	wodPlayer_Notify(wwo, uDevID, TRUE);
	if (wwo->lpNotifyHdr) {
	    ERR("out of sync\n");
	}
	/* get rid also of all the current queue */
	for (lpWaveHdr = wwo->lpQueueHdr; lpWaveHdr; lpWaveHdr = lpWaveHdr->lpNext) {
	    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
	    lpWaveHdr->dwFlags |= WHDR_DONE;
	    
	    if (WAVE_NotifyClient(uDevID, WOM_DONE, (DWORD)lpWaveHdr, 0) != MMSYSERR_NOERROR) {
		WARN("can't notify client !\n");
	    }
	}
	wwo->lpQueueHdr = 0;
	wwo->state = WINE_WS_STOPPED;
	wwo->dwPlayedTotal = 0;
    } else {
	/* move notify list to begining of lpQueueHdr list */
	while (wwo->lpNotifyHdr) {
	    lpWaveHdr = wwo->lpNotifyHdr;
	    wwo->lpNotifyHdr = lpWaveHdr->lpNext;
	    lpWaveHdr->lpNext = wwo->lpQueueHdr;
	    wwo->lpQueueHdr = lpWaveHdr;
	}
	wwo->state = WINE_WS_PAUSED;
	wwo->dwPlayedTotal += wwo->dwPlayed;
    }
    wwo->dwNotifiedBytes = wwo->dwPlayed = 0;
    wwo->wFragsUsedInQueue = 0;
}

/**************************************************************************
 * 				wodPlayer			[internal]
 */
static	DWORD	CALLBACK	wodPlayer(LPVOID pmt)
{
    WORD		uDevID = (DWORD)pmt;
    WINE_WAVEOUT*	wwo = (WINE_WAVEOUT*)&WOutDev[uDevID];
    WAVEHDR*		lpWaveHdr;
    DWORD		dwSleepTime;
    MSG			msg;

    PeekMessageA(&msg, 0, 0, 0, 0);
    wwo->state = WINE_WS_STOPPED;

    wwo->dwNotifiedBytes = 0;
    wwo->dwOffCurrHdr = 0;
    wwo->dwRemain = wwo->dwFragmentSize;
    wwo->lpQueueHdr = NULL;
    wwo->lpNotifyHdr = NULL;
    wwo->wFragsUsedInQueue = 0;
    wwo->dwPlayedTotal = 0;
    wwo->dwPlayed = 0;

    TRACE("imhere[0]\n");
    SetEvent(wwo->hEvent);

    /* make sleep time to be # of ms to output a fragment */
    dwSleepTime = (wwo->dwFragmentSize * 1000) / wwo->format.wf.nAvgBytesPerSec;

    for (;;) {
	/* wait for dwSleepTime or an event in thread's queue */
	/* FIXME: could improve wait time depending on queue state,
	 * ie, number of queued fragments
	 */
	TRACE("imhere[1]\n");
	MsgWaitForMultipleObjects(0, NULL, FALSE, 
				  (wwo->state == WINE_WS_PLAYING) ? 
				     (MAX(wwo->wFragsUsedInQueue, 4) - 2) * dwSleepTime : 
				     /*INFINITE*/100, 
				  QS_POSTMESSAGE);
	TRACE("imhere[2]\n");
	wodPlayer_Notify(wwo, uDevID, FALSE);
	while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
	    switch (msg.message) {
	    case WINE_WM_PAUSING:
		wodPlayer_Reset(wwo, uDevID, FALSE);
		wwo->state = WINE_WS_PAUSED;
		SetEvent(wwo->hEvent);
		break;
	    case WINE_WM_RESTARTING:
		wwo->state = WINE_WS_PLAYING;
		SetEvent(wwo->hEvent);
		break;
	    case WINE_WM_HEADER:
		lpWaveHdr = (LPWAVEHDR)msg.lParam;
		
		lpWaveHdr->dwFlags &= ~WHDR_DONE;
		lpWaveHdr->dwFlags |= WHDR_INQUEUE;
		lpWaveHdr->lpNext = 0;
		
		/* insert buffer at the end of queue */
		{
		    LPWAVEHDR*	wh;
		    for (wh = &(wwo->lpQueueHdr); *wh; wh = &((*wh)->lpNext));
		    *wh = lpWaveHdr;
		}
		if (wwo->state == WINE_WS_STOPPED)
		    wwo->state = WINE_WS_PLAYING;
		break;
	    case WINE_WM_RESETTING:
		wodPlayer_Reset(wwo, uDevID, TRUE);
		SetEvent(wwo->hEvent);
		break;
	    case WINE_WM_CLOSING:
		/* sanity check: this should not happen since the device must have been reset before */
		if (wwo->lpNotifyHdr || wwo->lpQueueHdr) {
		    ERR("out of sync\n");
		}
		wwo->hThread = 0;
		wwo->state = WINE_WS_CLOSED;
		SetEvent(wwo->hEvent);
		ExitThread(0);
		/* shouldn't go here */
	    default:
		FIXME("unknown message %d\n", msg.message);
		break;
	    }
	}
	if (wwo->state == WINE_WS_PLAYING) {
	    wodPlayer_WriteFragments(wwo);
	}
	wodPlayer_Notify(wwo, uDevID, FALSE);
    }
    ExitThread(0);
    /* just for not generating compilation warnings... should never be executed */
    return 0; 
}

/**************************************************************************
 * 			wodGetDevCaps				[internal]
 */
static DWORD wodGetDevCaps(WORD wDevID, LPWAVEOUTCAPSA lpCaps, DWORD dwSize)
{
    int 	audio;
    int		smplrate;
    int		samplesize = 16;
    int		dsp_stereo = 1;
    int		bytespersmpl;
    int 	caps;
    int		mask;
    
    TRACE("(%u, %p, %lu);\n", wDevID, lpCaps, dwSize);
    
    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;
    if (access(SOUND_DEV,0) != 0) return MMSYSERR_NOTENABLED;
    
    if (wDevID >= MAX_WAVEOUTDRV) {
	TRACE("MAX_WAVOUTDRV reached !\n");
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
    
    IOCTL(audio, SNDCTL_DSP_GETFMTS, mask);
    TRACE("OSS dsp mask=%08x\n", mask);
    mask = AFMT_QUERY;
    IOCTL(audio, SNDCTL_DSP_SETFMT, mask);
    TRACE("OSS dsp current=%08x\n", mask);

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
    TRACE("dwFormats = %08lX\n", lpCaps->dwFormats);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodOpen				[internal]
 */
static DWORD wodOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    int 	 	audio;
    int			sample_rate;
    int			sample_size;
    int			dsp_stereo;
    int			audio_fragment;
    int			fragment_size;
    WAVEOUTCAPSA 	woc;

    TRACE("(%u, %p, %08lX);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN("Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_WAVEOUTDRV) {
	TRACE("MAX_WAVOUTDRV reached !\n");
	return MMSYSERR_BADDEVICEID;
    }
    wodGetDevCaps(wDevID, &woc, sizeof(woc));

    /* only PCM format is supported so far... */
    if (lpDesc->lpFormat->wFormatTag != WAVE_FORMAT_PCM ||
	lpDesc->lpFormat->nChannels == 0 ||
	lpDesc->lpFormat->nSamplesPerSec == 0) {
	WARN("Bad format: tag=%04X nChannels=%d nSamplesPerSec=%ld !\n", 
	     lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
	     lpDesc->lpFormat->nSamplesPerSec);
	return WAVERR_BADFORMAT;
    }

    if (dwFlags & WAVE_FORMAT_QUERY) {
	TRACE("Query format: tag=%04X nChannels=%d nSamplesPerSec=%ld !\n", 
	     lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
	     lpDesc->lpFormat->nSamplesPerSec);
	return MMSYSERR_NOERROR;
    }

    WOutDev[wDevID].unixdev = 0;
    if (access(SOUND_DEV, 0) != 0) 
	return MMSYSERR_NOTENABLED;
    audio = open(SOUND_DEV, O_WRONLY, 0);
    if (audio == -1) {
	WARN("can't open !\n");
	return MMSYSERR_ALLOCATED ;
    }

    WOutDev[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
    
    memcpy(&WOutDev[wDevID].waveDesc, lpDesc, 		sizeof(WAVEOPENDESC));
    memcpy(&WOutDev[wDevID].format,   lpDesc->lpFormat, sizeof(PCMWAVEFORMAT));
    
    if (WOutDev[wDevID].format.wBitsPerSample == 0) {
	WOutDev[wDevID].format.wBitsPerSample = 8 *
	    (WOutDev[wDevID].format.wf.nAvgBytesPerSec /
	     WOutDev[wDevID].format.wf.nSamplesPerSec) /
	    WOutDev[wDevID].format.wf.nChannels;
    }
    
    /* shockwave player uses only 4 1k-fragments at a rate of 22050 bytes/sec
     * thus leading to 46ms per fragment, and a turnaround time of 185ms
     */
    /* 2^10=1024 bytes per fragment, 16 fragments max */
    audio_fragment = 0x000F000A;
    sample_size = WOutDev[wDevID].format.wBitsPerSample;
    sample_rate = WOutDev[wDevID].format.wf.nSamplesPerSec;
    dsp_stereo = (WOutDev[wDevID].format.wf.nChannels > 1) ? 1 : 0;

    IOCTL(audio, SNDCTL_DSP_SETFRAGMENT, audio_fragment);
    /* First size and stereo then samplerate */
    IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, sample_size);
    IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo);
    IOCTL(audio, SNDCTL_DSP_SPEED, sample_rate);

    /* paranoid checks */
    if (sample_size != WOutDev[wDevID].format.wBitsPerSample)
	ERR("Can't set sample_size to %u (%d)\n", 
	    WOutDev[wDevID].format.wBitsPerSample, sample_size);
    if (dsp_stereo != (WOutDev[wDevID].format.wf.nChannels > 1) ? 1 : 0) 
	ERR("Can't set stereo to %u (%d)\n", 
	    (WOutDev[wDevID].format.wf.nChannels > 1) ? 1 : 0, dsp_stereo);
    if (sample_rate != WOutDev[wDevID].format.wf.nSamplesPerSec)
	ERR("Can't set sample_rate to %lu (%d)\n", 
	    WOutDev[wDevID].format.wf.nSamplesPerSec, sample_rate);

    /* even if we set fragment size above, read it again, just in case */
    IOCTL(audio, SNDCTL_DSP_GETBLKSIZE, fragment_size);

    WOutDev[wDevID].unixdev = audio;
    WOutDev[wDevID].dwFragmentSize = fragment_size;
    WOutDev[wDevID].wMaxFragments = HIWORD(audio_fragment) + 1;

    WOutDev[wDevID].hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    WOutDev[wDevID].hThread = CreateThread(NULL, 0, wodPlayer, (LPVOID)(DWORD)wDevID, 0, &(WOutDev[wDevID].dwThreadID));
    WaitForSingleObject(WOutDev[wDevID].hEvent, INFINITE);

    TRACE("fd=%d fragmentSize=%ld\n", 
	  WOutDev[wDevID].unixdev, WOutDev[wDevID].dwFragmentSize);

    TRACE("wBitsPerSample=%u, nAvgBytesPerSec=%lu, nSamplesPerSec=%lu, nChannels=%u nBlockAlign=%u!\n", 
	  WOutDev[wDevID].format.wBitsPerSample, WOutDev[wDevID].format.wf.nAvgBytesPerSec, 
	  WOutDev[wDevID].format.wf.nSamplesPerSec, WOutDev[wDevID].format.wf.nChannels,
	  WOutDev[wDevID].format.wf.nBlockAlign);
    
    if (WAVE_NotifyClient(wDevID, WOM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN("can't notify client !\n");
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
    
    TRACE("(%u);\n", wDevID);
    
    if (wDevID > MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    if (WOutDev[wDevID].lpQueueHdr != NULL || WOutDev[wDevID].lpNotifyHdr != NULL) {
	WARN("buffers still playing !\n");
	ret = WAVERR_STILLPLAYING;
    } else {
	TRACE("imhere[3-close]\n");
	PostThreadMessageA(WOutDev[wDevID].dwThreadID, WINE_WM_CLOSING, 0, 0);
	WaitForSingleObject(WOutDev[wDevID].hEvent, INFINITE);
	CloseHandle(WOutDev[wDevID].hEvent);

	close(WOutDev[wDevID].unixdev);
	WOutDev[wDevID].unixdev = 0;
	WOutDev[wDevID].dwFragmentSize = 0;
	if (WAVE_NotifyClient(wDevID, WOM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
	    WARN("can't notify client !\n");
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
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    
    /* first, do the sanity checks... */
    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
        WARN("bad dev ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    if (lpWaveHdr->lpData == NULL || !(lpWaveHdr->dwFlags & WHDR_PREPARED)) 
	return WAVERR_UNPREPARED;
    
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE) 
	return WAVERR_STILLPLAYING;

    TRACE("imhere[3-HEADER]\n");
    PostThreadMessageA(WOutDev[wDevID].dwThreadID, WINE_WM_HEADER, 0, (DWORD)lpWaveHdr);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodPrepare			[internal]
 */
static DWORD wodPrepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    
    if (wDevID > MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN("bad device ID !\n");
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
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    
    if (wDevID > MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN("bad device ID !\n");
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
    TRACE("(%u);!\n", wDevID);
    
    if (wDevID > MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    TRACE("imhere[3-PAUSING]\n");
    PostThreadMessageA(WOutDev[wDevID].dwThreadID, WINE_WM_PAUSING, 0, 0);
    WaitForSingleObject(WOutDev[wDevID].hEvent, INFINITE);
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodRestart				[internal]
 */
static DWORD wodRestart(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    
    if (wDevID > MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    if (WOutDev[wDevID].state == WINE_WS_PAUSED) {
	TRACE("imhere[3-RESTARTING]\n");
	PostThreadMessageA(WOutDev[wDevID].dwThreadID, WINE_WM_RESTARTING, 0, 0);
	WaitForSingleObject(WOutDev[wDevID].hEvent, INFINITE);
    }
    
    /* FIXME: is NotifyClient with WOM_DONE right ? (Comet Busters 1.3.3 needs this notification) */
    /* FIXME: Myst crashes with this ... hmm -MM
       if (WAVE_NotifyClient(wDevID, WOM_DONE, 0L, 0L) != MMSYSERR_NOERROR) {
       WARN("can't notify client !\n");
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
    TRACE("(%u);\n", wDevID);
    
    if (wDevID > MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    TRACE("imhere[3-RESET]\n");
    PostThreadMessageA(WOutDev[wDevID].dwThreadID, WINE_WM_RESETTING, 0, 0);
    WaitForSingleObject(WOutDev[wDevID].hEvent, INFINITE);
    
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 * 				wodGetPosition			[internal]
 */
static DWORD wodGetPosition(WORD wDevID, LPMMTIME lpTime, DWORD uSize)
{
    int		time;
    DWORD	val;

    TRACE("(%u, %p, %lu);\n", wDevID, lpTime, uSize);
    
    if (wDevID > MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;

    val = WOutDev[wDevID].dwPlayedTotal + WOutDev[wDevID].dwPlayed;

    TRACE("wType=%04X wBitsPerSample=%u nSamplesPerSec=%lu nChannels=%u nAvgBytesPerSec=%lu\n", 
	  lpTime->wType, WOutDev[wDevID].format.wBitsPerSample, 
	  WOutDev[wDevID].format.wf.nSamplesPerSec, WOutDev[wDevID].format.wf.nChannels, 
	  WOutDev[wDevID].format.wf.nAvgBytesPerSec); 
    TRACE("dwTotalPlayed=%lu\n", val);
    
    switch (lpTime->wType) {
    case TIME_BYTES:
	lpTime->u.cb = val;
	TRACE("TIME_BYTES=%lu\n", lpTime->u.cb);
	break;
    case TIME_SAMPLES:
	lpTime->u.sample = val * 8 / WOutDev[wDevID].format.wBitsPerSample;
	TRACE("TIME_SAMPLES=%lu\n", lpTime->u.sample);
	break;
    case TIME_SMPTE:
	time = val / (WOutDev[wDevID].format.wf.nAvgBytesPerSec / 1000);
	lpTime->u.smpte.hour = time / 108000;
	time -= lpTime->u.smpte.hour * 108000;
	lpTime->u.smpte.min = time / 1800;
	time -= lpTime->u.smpte.min * 1800;
	lpTime->u.smpte.sec = time / 30;
	time -= lpTime->u.smpte.sec * 30;
	lpTime->u.smpte.frame = time;
	lpTime->u.smpte.fps = 30;
	TRACE("TIME_SMPTE=%02u:%02u:%02u:%02u\n",
	      lpTime->u.smpte.hour, lpTime->u.smpte.min,
	      lpTime->u.smpte.sec, lpTime->u.smpte.frame);
	break;
    default:
	FIXME("Format %d not supported ! use TIME_MS !\n", lpTime->wType);
	lpTime->wType = TIME_MS;
    case TIME_MS:
	lpTime->u.ms = val / (WOutDev[wDevID].format.wf.nAvgBytesPerSec / 1000);
	TRACE("TIME_MS=%lu\n", lpTime->u.ms);
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
    int		volume;
    DWORD	left, right;
    
    TRACE("(%u, %p);\n", wDevID, lpdwVol);
    
    if (lpdwVol == NULL) 
	return MMSYSERR_NOTENABLED;
    if ((mixer = open(MIXER_DEV, O_RDONLY)) < 0) {
	WARN("mixer device not available !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, SOUND_MIXER_READ_PCM, &volume) == -1) {
	WARN("unable read mixer !\n");
	return MMSYSERR_NOTENABLED;
    }
    close(mixer);
    left = LOBYTE(volume);
    right = HIBYTE(volume);
    TRACE("left=%ld right=%ld !\n", left, right);
    *lpdwVol = ((left * 0xFFFFl) / 100) + (((right * 0xFFFFl) / 100) << 16);
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 * 				wodSetVolume			[internal]
 */
static DWORD wodSetVolume(WORD wDevID, DWORD dwParam)
{
    int 	mixer;
    int		volume;
    DWORD	left, right;

    TRACE("(%u, %08lX);\n", wDevID, dwParam);

    left  = (LOWORD(dwParam) * 100) / 0xFFFFl;
    right = (HIWORD(dwParam) * 100) / 0xFFFFl;
    volume = left + (right << 8);
    
    if ((mixer = open(MIXER_DEV, O_WRONLY)) < 0) {
	WARN("mixer device not available !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, SOUND_MIXER_WRITE_PCM, &volume) == -1) {
	WARN("unable set mixer !\n");
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
 * 				OSS_wodMessage		[sample driver]
 */
DWORD WINAPI OSS_wodMessage(UINT wDevID, UINT wMsg, DWORD dwUser, 
			    DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%u, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);
    
    switch (wMsg) {
    case DRVM_INIT:
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
	/* FIXME: Pretend this is supported */
	return 0;
    case WODM_OPEN:	 	return wodOpen		(wDevID, (LPWAVEOPENDESC)dwParam1,	dwParam2);
    case WODM_CLOSE:	 	return wodClose		(wDevID);
    case WODM_WRITE:	 	return wodWrite		(wDevID, (LPWAVEHDR)dwParam1,		dwParam2);
    case WODM_PAUSE:	 	return wodPause		(wDevID);
    case WODM_GETPOS:	 	return wodGetPosition	(wDevID, (LPMMTIME)dwParam1, 		dwParam2);
    case WODM_BREAKLOOP: 	return MMSYSERR_NOTSUPPORTED;
    case WODM_PREPARE:	 	return wodPrepare	(wDevID, (LPWAVEHDR)dwParam1, 		dwParam2);
    case WODM_UNPREPARE: 	return wodUnprepare	(wDevID, (LPWAVEHDR)dwParam1, 		dwParam2);
    case WODM_GETDEVCAPS:	return wodGetDevCaps	(wDevID, (LPWAVEOUTCAPSA)dwParam1,	dwParam2);
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
	FIXME("unknown message %d!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*======================================================================*
 *                  Low level WAVE IN implemantation			*
 *======================================================================*/

/**************************************************************************
 * 			widGetDevCaps				[internal]
 */
static DWORD widGetDevCaps(WORD wDevID, LPWAVEINCAPSA lpCaps, DWORD dwSize)
{
    int 	audio, smplrate, samplesize=16, dsp_stereo=1, bytespersmpl;
    
    TRACE("(%u, %p, %lu);\n", wDevID, lpCaps, dwSize);
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
    TRACE("dwFormats = %08lX\n", lpCaps->dwFormats);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widOpen				[internal]
 */
static DWORD widOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    int 		audio, abuf_size, smplrate, samplesize, dsp_stereo;
    LPWAVEFORMAT	lpFormat;
    
    TRACE("(%u, %p, %08lX);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN("Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_WAVEINDRV) {
	TRACE("MAX_WAVINDRV reached !\n");
	return MMSYSERR_ALLOCATED;
    }

    /* only PCM format is supported so far... */
    if (lpDesc->lpFormat->wFormatTag != WAVE_FORMAT_PCM ||
	lpDesc->lpFormat->nChannels == 0 ||
	lpDesc->lpFormat->nSamplesPerSec == 0) {
	WARN("Bad format: tag=%04X nChannels=%d nSamplesPerSec=%ld !\n", 
	     lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
	     lpDesc->lpFormat->nSamplesPerSec);
	return WAVERR_BADFORMAT;
    }

    if (dwFlags & WAVE_FORMAT_QUERY) {
	TRACE("Query format: tag=%04X nChannels=%d nSamplesPerSec=%ld !\n", 
	     lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
	     lpDesc->lpFormat->nSamplesPerSec);
	return MMSYSERR_NOERROR;
    }

    WInDev[wDevID].unixdev = 0;
    if (access(SOUND_DEV,0) != 0) return MMSYSERR_NOTENABLED;
    audio = open(SOUND_DEV, O_RDONLY, 0);
    if (audio == -1) {
	WARN("can't open !\n");
	return MMSYSERR_ALLOCATED;
    }
    IOCTL(audio, SNDCTL_DSP_GETBLKSIZE, abuf_size);
    if (abuf_size < 1024 || abuf_size > 65536) {
	if (abuf_size == -1)
	    WARN("IOCTL can't 'SNDCTL_DSP_GETBLKSIZE' !\n");
	else
	    WARN("SNDCTL_DSP_GETBLKSIZE Invalid dwFragmentSize !\n");
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
    TRACE("wBitsPerSample=%u !\n", WInDev[wDevID].format.wBitsPerSample);
    TRACE("nSamplesPerSec=%lu !\n", WInDev[wDevID].format.wf.nSamplesPerSec);
    TRACE("nChannels=%u !\n", WInDev[wDevID].format.wf.nChannels);
    TRACE("nAvgBytesPerSec=%lu\n", WInDev[wDevID].format.wf.nAvgBytesPerSec); 
    if (WAVE_NotifyClient(wDevID, WIM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN("can't notify client !\n");
	return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widClose			[internal]
 */
static DWORD widClose(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    if (wDevID > MAX_WAVEINDRV) return MMSYSERR_INVALPARAM;
    if (WInDev[wDevID].unixdev == 0) {
	WARN("can't close !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (WInDev[wDevID].lpQueueHdr != NULL) {
	WARN("still buffers open !\n");
	return WAVERR_STILLPLAYING;
    }
    close(WInDev[wDevID].unixdev);
    WInDev[wDevID].unixdev = 0;
    WInDev[wDevID].dwFragmentSize = 0;
    if (WAVE_NotifyClient(wDevID, WIM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN("can't notify client !\n");
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
    
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    if (WInDev[wDevID].unixdev == 0) {
	WARN("can't do it !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (!(lpWaveHdr->dwFlags & WHDR_PREPARED)) {
	TRACE("never been prepared !\n");
	return WAVERR_UNPREPARED;
    }
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE) {
	TRACE("header already in use !\n");
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
    TRACE("buffer added ! (now %u in queue)\n", count);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widPrepare			[internal]
 */
static DWORD widPrepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    if (WInDev[wDevID].unixdev == 0) {
	WARN("can't prepare !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE)
	return WAVERR_STILLPLAYING;
    lpWaveHdr->dwFlags |= WHDR_PREPARED;
    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
    lpWaveHdr->dwFlags &= ~WHDR_DONE;
    lpWaveHdr->dwBytesRecorded = 0;
    TRACE("header prepared !\n");
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widUnprepare			[internal]
 */
static DWORD widUnprepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    if (WInDev[wDevID].unixdev == 0) {
	WARN("can't unprepare !\n");
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

    TRACE("(%u);\n", wDevID);
    if (WInDev[wDevID].unixdev == 0) {
	WARN("can't start recording !\n");
	return MMSYSERR_NOTENABLED;
    }
    
    lpWaveHdr = &(WInDev[wDevID].lpQueueHdr);
    TRACE("lpWaveHdr = %08lx\n",(DWORD)lpWaveHdr);

    if (!*lpWaveHdr || !(*lpWaveHdr)->lpData) {
	TRACE("never been prepared !\n");
	return WAVERR_UNPREPARED;
    }
    
    while (*lpWaveHdr != NULL) {
	lpData = (*lpWaveHdr)->lpData;
	TRACE("recording buf#%u=%p size=%lu \n",
	      count, lpData, (*lpWaveHdr)->dwBufferLength);

	bytesRead = read(WInDev[wDevID].unixdev, lpData, (*lpWaveHdr)->dwBufferLength);

	if (bytesRead == -1)
	    perror("read from audio device");

	TRACE("bytesread=%d (%ld)\n", bytesRead, (*lpWaveHdr)->dwBufferLength);
	(*lpWaveHdr)->dwBytesRecorded = bytesRead;
	WInDev[wDevID].dwTotalRecorded += (*lpWaveHdr)->dwBytesRecorded;
	(*lpWaveHdr)->dwFlags &= ~WHDR_INQUEUE;
	(*lpWaveHdr)->dwFlags |= WHDR_DONE;
	
	if (WAVE_NotifyClient(wDevID, WIM_DATA, (DWORD)*lpWaveHdr, (*lpWaveHdr)->dwBytesRecorded) != MMSYSERR_NOERROR) {
	    WARN("can't notify client !\n");
	    return MMSYSERR_INVALPARAM;
	}

	/* removes the current block from the queue */
	*lpWaveHdr = (*lpWaveHdr)->lpNext;
	count++;
    }
    TRACE("end of recording !\n");
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widStop					[internal]
 */
static DWORD widStop(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    if (WInDev[wDevID].unixdev == 0) {
	WARN("can't stop !\n");
	return MMSYSERR_NOTENABLED;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widReset				[internal]
 */
static DWORD widReset(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    if (WInDev[wDevID].unixdev == 0) {
	WARN("can't reset !\n");
	return MMSYSERR_NOTENABLED;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widGetPosition			[internal]
 */
static DWORD widGetPosition(WORD wDevID, LPMMTIME lpTime, DWORD uSize)
{
    int		time;
    
    TRACE("(%u, %p, %lu);\n", wDevID, lpTime, uSize);
    if (WInDev[wDevID].unixdev == 0) {
	WARN("can't get pos !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;
    TRACE("wType=%04X !\n", lpTime->wType);
    TRACE("wBitsPerSample=%u\n", WInDev[wDevID].format.wBitsPerSample); 
    TRACE("nSamplesPerSec=%lu\n", WInDev[wDevID].format.wf.nSamplesPerSec); 
    TRACE("nChannels=%u\n", WInDev[wDevID].format.wf.nChannels); 
    TRACE("nAvgBytesPerSec=%lu\n", WInDev[wDevID].format.wf.nAvgBytesPerSec); 
    switch (lpTime->wType) {
    case TIME_BYTES:
	lpTime->u.cb = WInDev[wDevID].dwTotalRecorded;
	TRACE("TIME_BYTES=%lu\n", lpTime->u.cb);
	break;
    case TIME_SAMPLES:
	lpTime->u.sample = WInDev[wDevID].dwTotalRecorded * 8 /
	    WInDev[wDevID].format.wBitsPerSample;
	TRACE("TIME_SAMPLES=%lu\n", lpTime->u.sample);
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
	TRACE("TIME_SMPTE=%02u:%02u:%02u:%02u\n",
	      lpTime->u.smpte.hour, lpTime->u.smpte.min,
	      lpTime->u.smpte.sec, lpTime->u.smpte.frame);
	break;
    case TIME_MS:
	lpTime->u.ms = WInDev[wDevID].dwTotalRecorded /
	    (WInDev[wDevID].format.wf.nAvgBytesPerSec / 1000);
	TRACE("TIME_MS=%lu\n", lpTime->u.ms);
	break;
    default:
	FIXME("format not supported (%u) ! use TIME_MS !\n", lpTime->wType);
	lpTime->wType = TIME_MS;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				OSS_widMessage			[sample driver]
 */
DWORD WINAPI OSS_widMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			    DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%u, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    switch (wMsg) {
    case DRVM_INIT:
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
	/* FIXME: Pretend this is supported */
	return 0;
    case WIDM_OPEN:		return widOpen(wDevID, (LPWAVEOPENDESC)dwParam1, dwParam2);
    case WIDM_CLOSE:		return widClose(wDevID);
    case WIDM_ADDBUFFER:	return widAddBuffer(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WIDM_PREPARE:		return widPrepare(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WIDM_UNPREPARE:	return widUnprepare(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WIDM_GETDEVCAPS:	return widGetDevCaps(wDevID, (LPWAVEINCAPSA)dwParam1, dwParam2);
    case WIDM_GETNUMDEVS:	return wodGetNumDevs();	/* same number of devices in output as in input */
    case WIDM_GETPOS:		return widGetPosition(wDevID, (LPMMTIME)dwParam1, dwParam2);
    case WIDM_RESET:		return widReset(wDevID);
    case WIDM_START:		return widStart(wDevID);
    case WIDM_STOP:		return widStop(wDevID);
    default:
	FIXME("unknown message %u!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

#else /* !HAVE_OSS */

/**************************************************************************
 * 				wodMessage			[sample driver]
 */
DWORD WINAPI OSS_wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			    DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08lX, %08lX, %08lX):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				widMessage			[sample driver]
 */
DWORD WINAPI OSS_widMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			    DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08lX, %08lX, %08lX):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

#endif /* HAVE_OSS */
