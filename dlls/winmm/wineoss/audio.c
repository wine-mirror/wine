/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*				   
 * Sample Wine Driver for Open Sound System (featured in Linux and FreeBSD)
 *
 * Copyright 1994 Martin Ayotte
 *           1999 Eric Pouech (async playing in waveOut/waveIn)
 *	     2000 Eric Pouech (loops in waveOut)
 */
/*
 * FIXME:
 *	pause in waveOut does not work correctly
 *	full duplex (in/out) is not working (device is opened twice for Out 
 *	and In) (OSS is known for its poor duplex capabilities, alsa is
 *	better)
 */

/*#define EMULATE_SB16*/

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "windef.h"
#include "wingdi.h"
#include "winerror.h"
#include "wine/winuser16.h"
#include "driver.h"
#include "mmddk.h"
#include "dsound.h"
#include "dsdriver.h"
#include "oss.h"
#include "heap.h"
#include "ldt.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(wave);

/* Allow 1% deviation for sample rates (some ES137x cards) */
#define NEAR_MATCH(rate1,rate2) (((100*((int)(rate1)-(int)(rate2)))/(rate1))==0)

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
    LPWAVEHDR			lpQueuePtr;		/* start of queued WAVEHDRs (waiting to be notified) */
    LPWAVEHDR			lpPlayPtr;		/* start of not yet fully played buffers */
    LPWAVEHDR			lpLoopPtr;              /* pointer of first buffer in loop, if any */
    
    DWORD			dwLastFragDone;		/* time in ms, when last played fragment will be actually played */
    DWORD			dwPlayedTotal;		/* number of bytes played since opening */

    /* info on current lpQueueHdr->lpWaveHdr */
    DWORD			dwOffCurrHdr;		/* offset in lpPlayPtr->lpData for fragments */
    DWORD			dwRemain;		/* number of bytes to write to end the current fragment  */

    /* synchronization stuff */
    HANDLE			hThread;
    DWORD			dwThreadID;
    HANDLE			hEvent;
    WAVEOUTCAPSA		caps;

    /* DirectSound stuff */
    LPBYTE			mapping;
    DWORD			maplen;
} WINE_WAVEOUT;

typedef struct {
    int				unixdev;
    volatile int		state;
    DWORD			dwFragmentSize;		/* OpenSound '/dev/dsp' give us that size */
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    PCMWAVEFORMAT		format;
    LPWAVEHDR			lpQueuePtr;
    DWORD			dwTotalRecorded;
    WAVEINCAPSA			caps;

    /* synchronization stuff */
    HANDLE			hThread;
    DWORD			dwThreadID;
    HANDLE			hEvent;
} WINE_WAVEIN;

static WINE_WAVEOUT	WOutDev   [MAX_WAVEOUTDRV];
static WINE_WAVEIN	WInDev    [MAX_WAVEINDRV ];

static DWORD wodDsCreate(UINT wDevID, PIDSDRIVER* drv);

/*======================================================================*
 *                  Low level WAVE implemantation			*
 *======================================================================*/

LONG OSS_WaveInit(void)
{
    int 	audio;
    int		smplrate;
    int		samplesize = 16;
    int		dsp_stereo = 1;
    int		bytespersmpl;
    int 	caps;
    int		mask;

    /* start with output device */

    /* FIXME: only one device is supported */
    memset(&WOutDev[0].caps, 0, sizeof(WOutDev[0].caps));

    if (access(SOUND_DEV,0) != 0 ||
	(audio = open(SOUND_DEV, O_WRONLY|O_NDELAY, 0)) == -1) {
	TRACE("Couldn't open out %s (%s)\n", SOUND_DEV, strerror(errno));
	return -1;
    }

    ioctl(audio, SNDCTL_DSP_RESET, 0);

    /* FIXME: some programs compare this string against the content of the registry
     * for MM drivers. The name have to match in order the program to work 
     * (e.g. MS win9x mplayer.exe)
     */
#ifdef EMULATE_SB16
    WOutDev[0].caps.wMid = 0x0002;
    WOutDev[0].caps.wPid = 0x0104;
    strcpy(WOutDev[0].caps.szPname, "SB16 Wave Out");
#else
    WOutDev[0].caps.wMid = 0x00FF; 	/* Manufac ID */
    WOutDev[0].caps.wPid = 0x0001; 	/* Product ID */
    /*    strcpy(WOutDev[0].caps.szPname, "OpenSoundSystem WAVOUT Driver");*/
    strcpy(WOutDev[0].caps.szPname, "CS4236/37/38");
#endif
    WOutDev[0].caps.vDriverVersion = 0x0100;
    WOutDev[0].caps.dwFormats = 0x00000000;
    WOutDev[0].caps.dwSupport = WAVECAPS_VOLUME;
    
    IOCTL(audio, SNDCTL_DSP_GETFMTS, mask);
    TRACE("OSS dsp out mask=%08x\n", mask);

    /* First bytespersampl, then stereo */
    bytespersmpl = (IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, samplesize) != 0) ? 1 : 2;
    
    WOutDev[0].caps.wChannels = (IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo) != 0) ? 1 : 2;
    if (WOutDev[0].caps.wChannels > 1) WOutDev[0].caps.dwSupport |= WAVECAPS_LRVOLUME;
    
    smplrate = 44100;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	if (mask & AFMT_U8) {
	    WOutDev[0].caps.dwFormats |= WAVE_FORMAT_4M08;
	    if (WOutDev[0].caps.wChannels > 1)
		WOutDev[0].caps.dwFormats |= WAVE_FORMAT_4S08;
	}
	if ((mask & AFMT_S16_LE) && bytespersmpl > 1) {
	    WOutDev[0].caps.dwFormats |= WAVE_FORMAT_4M16;
	    if (WOutDev[0].caps.wChannels > 1)
		WOutDev[0].caps.dwFormats |= WAVE_FORMAT_4S16;
	}
    }
    smplrate = 22050;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	if (mask & AFMT_U8) {
	    WOutDev[0].caps.dwFormats |= WAVE_FORMAT_2M08;
	    if (WOutDev[0].caps.wChannels > 1)
		WOutDev[0].caps.dwFormats |= WAVE_FORMAT_2S08;
	}
	if ((mask & AFMT_S16_LE) && bytespersmpl > 1) {
	    WOutDev[0].caps.dwFormats |= WAVE_FORMAT_2M16;
	    if (WOutDev[0].caps.wChannels > 1)
		WOutDev[0].caps.dwFormats |= WAVE_FORMAT_2S16;
	}
    }
    smplrate = 11025;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	if (mask & AFMT_U8) {
	    WOutDev[0].caps.dwFormats |= WAVE_FORMAT_1M08;
	    if (WOutDev[0].caps.wChannels > 1)
		WOutDev[0].caps.dwFormats |= WAVE_FORMAT_1S08;
	}
	if ((mask & AFMT_S16_LE) && bytespersmpl > 1) {
	    WOutDev[0].caps.dwFormats |= WAVE_FORMAT_1M16;
	    if (WOutDev[0].caps.wChannels > 1)
		WOutDev[0].caps.dwFormats |= WAVE_FORMAT_1S16;
	}
    }
    if (IOCTL(audio, SNDCTL_DSP_GETCAPS, caps) == 0) {
	TRACE("OSS dsp out caps=%08X\n", caps);
	if ((caps & DSP_CAP_REALTIME) && !(caps & DSP_CAP_BATCH)) {
	    WOutDev[0].caps.dwSupport |= WAVECAPS_SAMPLEACCURATE;

	    /* well, might as well use the DirectSound cap flag for something */
	    if ((caps & DSP_CAP_TRIGGER) && (caps & DSP_CAP_MMAP))
		WOutDev[0].caps.dwSupport |= WAVECAPS_DIRECTSOUND;
	}
    }
    close(audio);
    TRACE("out dwFormats = %08lX, dwSupport = %08lX\n",
	  WOutDev[0].caps.dwFormats, WOutDev[0].caps.dwSupport);

    /* then do input device */
    samplesize = 16;
    dsp_stereo = 1;
    
    if (access(SOUND_DEV,0) != 0 ||
	(audio = open(SOUND_DEV, O_RDONLY|O_NDELAY, 0)) == -1) {
	TRACE("Couldn't open in %s (%s)\n", SOUND_DEV, strerror(errno));
	return -1;
    }

    ioctl(audio, SNDCTL_DSP_RESET, 0);

#ifdef EMULATE_SB16
    WInDev[0].caps.wMid = 0x0002;
    WInDev[0].caps.wPid = 0x0004;
    strcpy(WInDev[0].caps.szPname, "SB16 Wave In");
#else
    WInDev[0].caps.wMid = 0x00FF; 	/* Manufac ID */
    WInDev[0].caps.wPid = 0x0001; 	/* Product ID */
    strcpy(WInDev[0].caps.szPname, "OpenSoundSystem WAVIN Driver");
#endif
    WInDev[0].caps.dwFormats = 0x00000000;
    WInDev[0].caps.wChannels = (IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo) != 0) ? 1 : 2;

    IOCTL(audio, SNDCTL_DSP_GETFMTS, mask);
    TRACE("OSS in dsp mask=%08x\n", mask);

    bytespersmpl = (IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, samplesize) != 0) ? 1 : 2;
    smplrate = 44100;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	if (mask & AFMT_U8) {
	    WInDev[0].caps.dwFormats |= WAVE_FORMAT_4M08;
	    if (WInDev[0].caps.wChannels > 1)
		WInDev[0].caps.dwFormats |= WAVE_FORMAT_4S08;
	}
	if ((mask & AFMT_S16_LE) && bytespersmpl > 1) {
	    WInDev[0].caps.dwFormats |= WAVE_FORMAT_4M16;
	    if (WInDev[0].caps.wChannels > 1)
		WInDev[0].caps.dwFormats |= WAVE_FORMAT_4S16;
	}
    }
    smplrate = 22050;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	if (mask & AFMT_U8) {
	    WInDev[0].caps.dwFormats |= WAVE_FORMAT_2M08;
	    if (WInDev[0].caps.wChannels > 1)
		WInDev[0].caps.dwFormats |= WAVE_FORMAT_2S08;
	}
	if ((mask & AFMT_S16_LE) && bytespersmpl > 1) {
	    WInDev[0].caps.dwFormats |= WAVE_FORMAT_2M16;
	    if (WInDev[0].caps.wChannels > 1)
		WInDev[0].caps.dwFormats |= WAVE_FORMAT_2S16;
	}
    }
    smplrate = 11025;
    if (IOCTL(audio, SNDCTL_DSP_SPEED, smplrate) == 0) {
	if (mask & AFMT_U8) {
	    WInDev[0].caps.dwFormats |= WAVE_FORMAT_1M08;
	    if (WInDev[0].caps.wChannels > 1)
		WInDev[0].caps.dwFormats |= WAVE_FORMAT_1S08;
	}
	if ((mask & AFMT_S16_LE) && bytespersmpl > 1) {
	    WInDev[0].caps.dwFormats |= WAVE_FORMAT_1M16;
	    if (WInDev[0].caps.wChannels > 1)
		WInDev[0].caps.dwFormats |= WAVE_FORMAT_1S16;
	}
    }
    close(audio);
    TRACE("in dwFormats = %08lX\n", WInDev[0].caps.dwFormats);

    return 0;
}

/**************************************************************************
 * 			OSS_NotifyClient			[internal]
 */
static DWORD OSS_NotifyClient(UINT wDevID, WORD wMsg, DWORD dwParam1, 
			      DWORD dwParam2)
{
    TRACE("wDevID = %04X wMsg = %d dwParm1 = %04lX dwParam2 = %04lX\n",wDevID, wMsg, dwParam1, dwParam2);
    
    switch (wMsg) {
    case WOM_OPEN:
    case WOM_CLOSE:
    case WOM_DONE:
	if (wDevID >= MAX_WAVEOUTDRV) return MCIERR_INTERNAL;
	
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
	if (wDevID >= MAX_WAVEINDRV) return MCIERR_INTERNAL;
	
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
    audio_buf_info 	info;

    for (;;) {
	if (ioctl(wwo->unixdev, SNDCTL_DSP_GETOSPACE, &info) < 0) {
	    ERR("ioctl failed (%s)\n", strerror(errno));
	    return FALSE;
	}
	
	TRACE("Fragments %d/%d\n", info.fragments, info.fragstotal);

	if (!info.fragments)	/* output queue is full, wait a bit */
	    return FALSE;

	lpWaveHdr = wwo->lpPlayPtr;
	if (!lpWaveHdr) {
	    if (wwo->dwRemain > 0 &&		/* still data to send to complete current fragment */
		wwo->dwLastFragDone &&  	/* first fragment has been played */
		info.fragments + 2 > info.fragstotal) {   /* done with all waveOutWrite()' fragments */
		/* FIXME: should do better handling here */
		TRACE("Oooch, buffer underrun !\n");
		return TRUE; /* force resetting of waveOut device */
	    }
	    return FALSE;	/* wait a bit */
	}
	
	if (wwo->dwOffCurrHdr == 0) {
	    TRACE("Starting a new wavehdr %p of %ld bytes\n", lpWaveHdr, lpWaveHdr->dwBufferLength);
	    if (lpWaveHdr->dwFlags & WHDR_BEGINLOOP) {
		if (wwo->lpLoopPtr) {
		    WARN("Already in a loop. Discarding loop on this header (%p)\n", lpWaveHdr);
		} else {
		    wwo->lpLoopPtr = lpWaveHdr;
		}
	    }
	}
	
	lpData = lpWaveHdr->lpData;

	/* finish current wave hdr ? */
	if (wwo->dwOffCurrHdr + wwo->dwRemain >= lpWaveHdr->dwBufferLength) { 
	    DWORD	toWrite = lpWaveHdr->dwBufferLength - wwo->dwOffCurrHdr;
	    
	    /* write end of current wave hdr */
	    count = write(wwo->unixdev, lpData + wwo->dwOffCurrHdr, toWrite);
	    TRACE("write(%p[%5lu], %5lu) => %d\n", lpData, wwo->dwOffCurrHdr, toWrite, count);
	    
	    if (count > 0 || toWrite == 0) {
		DWORD	tc = GetTickCount();

		if (wwo->dwLastFragDone /* + guard time ?? */ < tc) 
		    wwo->dwLastFragDone = tc;
		wwo->dwLastFragDone += (toWrite * 1000) / wwo->format.wf.nAvgBytesPerSec;

		lpWaveHdr->reserved = wwo->dwLastFragDone;
		TRACE("Tagging hdr %p with %08lx\n", lpWaveHdr, wwo->dwLastFragDone);

		/* WAVEHDR written, go to next one */
		if ((lpWaveHdr->dwFlags & WHDR_ENDLOOP) && wwo->lpLoopPtr) {
		    if (--wwo->lpLoopPtr->dwLoops > 0) {
			wwo->lpPlayPtr = wwo->lpLoopPtr;
		    } else {
			/* last one played */
			if (wwo->lpLoopPtr != lpWaveHdr && (lpWaveHdr->dwFlags & WHDR_BEGINLOOP)) {
			    FIXME("Correctly handled case ? (ending loop buffer also starts a new loop\n");
			    /* shall we consider the END flag for the closing loop or for
			     * the opening one or for both ???
			     * code assumes for closing loop only
			     */
			    wwo->lpLoopPtr = lpWaveHdr;
			} else {
			    wwo->lpLoopPtr = NULL;
			}
			wwo->lpPlayPtr = lpWaveHdr->lpNext;
		    }
		} else {
		    wwo->lpPlayPtr = lpWaveHdr->lpNext;
		}
		wwo->dwOffCurrHdr = 0;
		if ((wwo->dwRemain -= count) == 0) {
		    wwo->dwRemain = wwo->dwFragmentSize;
		}
	    }
	    continue; /* try to go to use next wavehdr */
	}  else	{
	    count = write(wwo->unixdev, lpData + wwo->dwOffCurrHdr, wwo->dwRemain);
	    TRACE("write(%p[%5lu], %5lu) => %d\n", lpData, wwo->dwOffCurrHdr, wwo->dwRemain, count);
	    if (count > 0) {
		DWORD	tc = GetTickCount();

		if (wwo->dwLastFragDone /* + guard time ?? */ < tc) 
		    wwo->dwLastFragDone = tc;
		wwo->dwLastFragDone += (wwo->dwRemain * 1000) / wwo->format.wf.nAvgBytesPerSec;

		TRACE("Tagging frag with %08lx\n", wwo->dwLastFragDone);

		wwo->dwOffCurrHdr += count;
		wwo->dwRemain = wwo->dwFragmentSize;
	    }
	}
    }
}

/**************************************************************************
 * 				wodPlayer_Notify		[internal]
 *
 * wodPlayer helper. Notifies (and remove from queue) all the wavehdr which content
 * have been played (actually to speaker, not to unixdev fd).
 */
static	void	wodPlayer_Notify(WINE_WAVEOUT* wwo, WORD uDevID, BOOL force)
{
    LPWAVEHDR		lpWaveHdr;
    DWORD		tc = GetTickCount();

    while (wwo->lpQueuePtr && 
	   (force || 
	    (wwo->lpQueuePtr != wwo->lpPlayPtr && wwo->lpQueuePtr != wwo->lpLoopPtr))) {
	lpWaveHdr = wwo->lpQueuePtr;
	    
	if (lpWaveHdr->reserved > tc && !force) break;

	wwo->dwPlayedTotal += lpWaveHdr->dwBufferLength;
	wwo->lpQueuePtr = lpWaveHdr->lpNext;

	lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
	lpWaveHdr->dwFlags |= WHDR_DONE;

	TRACE("Notifying client with %p\n", lpWaveHdr);
	if (OSS_NotifyClient(uDevID, WOM_DONE, (DWORD)lpWaveHdr, 0) != MMSYSERR_NOERROR) {
	    WARN("can't notify client !\n");
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
    wwo->dwRemain = wwo->dwFragmentSize;
    if (reset) {
	/* empty notify list */
	wodPlayer_Notify(wwo, uDevID, TRUE);

	wwo->lpPlayPtr = wwo->lpQueuePtr = wwo->lpLoopPtr = NULL;
	wwo->state = WINE_WS_STOPPED;
	wwo->dwPlayedTotal = 0;
    } else {
	/* FIXME: this is not accurate when looping, but can be do better ? */
	wwo->lpPlayPtr = (wwo->lpLoopPtr) ? wwo->lpLoopPtr : wwo->lpQueuePtr;
	wwo->state = WINE_WS_PAUSED;
    }
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

    wwo->dwLastFragDone = 0;
    wwo->dwOffCurrHdr = 0;
    wwo->dwRemain = wwo->dwFragmentSize;
    wwo->lpQueuePtr = wwo->lpPlayPtr = wwo->lpLoopPtr = NULL;
    wwo->dwPlayedTotal = 0;

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
				     2 * dwSleepTime : /*INFINITE*/100, 
				  QS_POSTMESSAGE);
	TRACE("imhere[2] (q=%p p=%p)\n", wwo->lpQueuePtr, wwo->lpPlayPtr);
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
		
		/* insert buffer at the end of queue */
		{
		    LPWAVEHDR*	wh;
		    for (wh = &(wwo->lpQueuePtr); *wh; wh = &((*wh)->lpNext));
		    *wh = lpWaveHdr;
		}
		if (!wwo->lpPlayPtr) wwo->lpPlayPtr = lpWaveHdr;
		if (wwo->state == WINE_WS_STOPPED)
		    wwo->state = WINE_WS_PLAYING;
		break;
	    case WINE_WM_RESETTING:
		wodPlayer_Reset(wwo, uDevID, TRUE);
		SetEvent(wwo->hEvent);
		break;
	    case WINE_WM_CLOSING:
		/* sanity check: this should not happen since the device must have been reset before */
		if (wwo->lpQueuePtr || wwo->lpPlayPtr) ERR("out of sync\n");
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
    TRACE("(%u, %p, %lu);\n", wDevID, lpCaps, dwSize);
    
    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;
    
    if (wDevID >= MAX_WAVEOUTDRV) {
	TRACE("MAX_WAVOUTDRV reached !\n");
	return MMSYSERR_BADDEVICEID;
    }

    memcpy(lpCaps, &WOutDev[wDevID].caps, min(dwSize, sizeof(*lpCaps)));
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodOpen				[internal]
 */
static DWORD wodOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    int 	 	audio;
    int			format;
    int			sample_rate;
    int			dsp_stereo;
    int			audio_fragment;
    int			fragment_size;
    WINE_WAVEOUT*	wwo;

    TRACE("(%u, %p, %08lX);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN("Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_WAVEOUTDRV) {
	TRACE("MAX_WAVOUTDRV reached !\n");
	return MMSYSERR_BADDEVICEID;
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

    wwo = &WOutDev[wDevID];

    if (dwFlags & WAVE_DIRECTSOUND) {
	if (!(wwo->caps.dwSupport & WAVECAPS_DIRECTSOUND))
	    /* not supported, ignore it */
	    dwFlags &= ~WAVE_DIRECTSOUND;
    }

    
    wwo->unixdev = 0;
    if (access(SOUND_DEV, 0) != 0) 
	return MMSYSERR_NOTENABLED;
    if (dwFlags & WAVE_DIRECTSOUND)
	/* we want to be able to mmap() the device, which means it must be opened readable,
	 * otherwise mmap() will fail (at least under Linux) */
	audio = open(SOUND_DEV, O_RDWR|O_NDELAY, 0);
    else
	audio = open(SOUND_DEV, O_WRONLY|O_NDELAY, 0);
    if (audio == -1) {
	WARN("can't open (%s)!\n", strerror(errno));
	return MMSYSERR_ALLOCATED;
    }

    wwo->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
    
    memcpy(&wwo->waveDesc, lpDesc, 		sizeof(WAVEOPENDESC));
    memcpy(&wwo->format,   lpDesc->lpFormat, sizeof(PCMWAVEFORMAT));
    
    if (WOutDev[wDevID].format.wBitsPerSample == 0) {
	WOutDev[wDevID].format.wBitsPerSample = 8 *
	    (WOutDev[wDevID].format.wf.nAvgBytesPerSec /
	     WOutDev[wDevID].format.wf.nSamplesPerSec) /
	    WOutDev[wDevID].format.wf.nChannels;
    }
    
    if (dwFlags & WAVE_DIRECTSOUND) {
	/* with DirectSound, fragments are irrelevant, but a large buffer isn't...
	 * so let's choose a full 64KB for DirectSound */
	audio_fragment = 0x0020000B;
    } else {
	/* shockwave player uses only 4 1k-fragments at a rate of 22050 bytes/sec
	 * thus leading to 46ms per fragment, and a turnaround time of 185ms
	 */
	/* 2^10=1024 bytes per fragment, 16 fragments max */
	audio_fragment = 0x000F000A;
    }
    sample_rate = wwo->format.wf.nSamplesPerSec;
    dsp_stereo = (wwo->format.wf.nChannels > 1) ? 1 : 0;
    format = (wwo->format.wBitsPerSample == 16) ? AFMT_S16_LE : AFMT_U8;

    IOCTL(audio, SNDCTL_DSP_SETFRAGMENT, audio_fragment);
    /* First size and stereo then samplerate */
    IOCTL(audio, SNDCTL_DSP_SETFMT, format);
    IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo);
    IOCTL(audio, SNDCTL_DSP_SPEED, sample_rate);

    /* paranoid checks */
    if (format != ((wwo->format.wBitsPerSample == 16) ? AFMT_S16_LE : AFMT_U8))
	ERR("Can't set format to %d (%d)\n", 
	    (wwo->format.wBitsPerSample == 16) ? AFMT_S16_LE : AFMT_U8, format);
    if (dsp_stereo != (wwo->format.wf.nChannels > 1) ? 1 : 0) 
	ERR("Can't set stereo to %u (%d)\n", 
	    (wwo->format.wf.nChannels > 1) ? 1 : 0, dsp_stereo);
    if (!NEAR_MATCH(sample_rate,wwo->format.wf.nSamplesPerSec))
	ERR("Can't set sample_rate to %lu (%d)\n", 
	    wwo->format.wf.nSamplesPerSec, sample_rate);

    /* even if we set fragment size above, read it again, just in case */
    IOCTL(audio, SNDCTL_DSP_GETBLKSIZE, fragment_size);

    wwo->unixdev = audio;
    wwo->dwFragmentSize = fragment_size;

    if (!(dwFlags & WAVE_DIRECTSOUND)) {
	wwo->hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
	wwo->hThread = CreateThread(NULL, 0, wodPlayer, (LPVOID)(DWORD)wDevID, 0, &(wwo->dwThreadID));
	WaitForSingleObject(wwo->hEvent, INFINITE);
    } else {
	wwo->hEvent = INVALID_HANDLE_VALUE;
	wwo->hThread = INVALID_HANDLE_VALUE;
	wwo->dwThreadID = 0;
    }

    TRACE("fd=%d fragmentSize=%ld\n", 
	  wwo->unixdev, wwo->dwFragmentSize);
    if (wwo->dwFragmentSize % wwo->format.wf.nBlockAlign)
	ERR("Fragment doesn't contain an integral number of data blocks\n");

    TRACE("wBitsPerSample=%u, nAvgBytesPerSec=%lu, nSamplesPerSec=%lu, nChannels=%u nBlockAlign=%u!\n", 
	  wwo->format.wBitsPerSample, wwo->format.wf.nAvgBytesPerSec, 
	  wwo->format.wf.nSamplesPerSec, wwo->format.wf.nChannels,
	  wwo->format.wf.nBlockAlign);
    
    if (OSS_NotifyClient(wDevID, WOM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
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
    DWORD		ret = MMSYSERR_NOERROR;
    WINE_WAVEOUT*	wwo;

    TRACE("(%u);\n", wDevID);
    
    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    wwo = &WOutDev[wDevID];
    if (wwo->lpQueuePtr) {
	WARN("buffers still playing !\n");
	ret = WAVERR_STILLPLAYING;
    } else {
	TRACE("imhere[3-close]\n");
	if (wwo->hEvent != INVALID_HANDLE_VALUE) {
	    PostThreadMessageA(wwo->dwThreadID, WINE_WM_CLOSING, 0, 0);
	    WaitForSingleObject(wwo->hEvent, INFINITE);
	    CloseHandle(wwo->hEvent);
	}
	if (wwo->mapping) {
	    munmap(wwo->mapping, wwo->maplen);
	    wwo->mapping = NULL;
	}

	close(wwo->unixdev);
	wwo->unixdev = 0;
	wwo->dwFragmentSize = 0;
	if (OSS_NotifyClient(wDevID, WOM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
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

    lpWaveHdr->dwFlags &= ~WHDR_DONE;
    lpWaveHdr->dwFlags |= WHDR_INQUEUE;
    lpWaveHdr->lpNext = 0;

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
    
    if (wDevID >= MAX_WAVEOUTDRV) {
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
    
    if (wDevID >= MAX_WAVEOUTDRV) {
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
    
    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
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
    
    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
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
       if (OSS_NotifyClient(wDevID, WOM_DONE, 0L, 0L) != MMSYSERR_NOERROR) {
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
    
    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
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
    int			time;
    DWORD		val;
    WINE_WAVEOUT*	wwo;

    TRACE("(%u, %p, %lu);\n", wDevID, lpTime, uSize);
    
    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == 0) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;

    wwo = &WOutDev[wDevID];
    val = wwo->dwPlayedTotal;

    TRACE("wType=%04X wBitsPerSample=%u nSamplesPerSec=%lu nChannels=%u nAvgBytesPerSec=%lu\n", 
	  lpTime->wType, wwo->format.wBitsPerSample, 
	  wwo->format.wf.nSamplesPerSec, wwo->format.wf.nChannels, 
	  wwo->format.wf.nAvgBytesPerSec); 
    TRACE("dwTotalPlayed=%lu\n", val);
    
    switch (lpTime->wType) {
    case TIME_BYTES:
	lpTime->u.cb = val;
	TRACE("TIME_BYTES=%lu\n", lpTime->u.cb);
	break;
    case TIME_SAMPLES:
	lpTime->u.sample = val * 8 / wwo->format.wBitsPerSample;
	TRACE("TIME_SAMPLES=%lu\n", lpTime->u.sample);
	break;
    case TIME_SMPTE:
	time = val / (wwo->format.wf.nAvgBytesPerSec / 1000);
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
	lpTime->u.ms = val / (wwo->format.wf.nAvgBytesPerSec / 1000);
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
    if ((mixer = open(MIXER_DEV, O_RDONLY|O_NDELAY)) < 0) {
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
    
    if ((mixer = open(MIXER_DEV, O_WRONLY|O_NDELAY)) < 0) {
	WARN("mixer device not available !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, SOUND_MIXER_WRITE_PCM, &volume) == -1) {
	WARN("unable set mixer !\n");
	return MMSYSERR_NOTENABLED;
    } else {
	TRACE("volume=%04x\n", (unsigned)volume);
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
    int audio = open(SOUND_DEV, O_WRONLY|O_NDELAY, 0);
    
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
    case 0x810:			return wodDsCreate(wDevID, (PIDSDRIVER*)dwParam1);
    default:
	FIXME("unknown message %d!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*======================================================================*
 *                  Low level DSOUND implementation			*
 *======================================================================*/

typedef struct IDsDriverImpl IDsDriverImpl;
typedef struct IDsDriverBufferImpl IDsDriverBufferImpl;

struct IDsDriverImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDsDriver);
    DWORD		ref;
    /* IDsDriverImpl fields */
    UINT		wDevID;
    IDsDriverBufferImpl*primary;
};

struct IDsDriverBufferImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDsDriverBuffer);
    DWORD		ref;
    /* IDsDriverBufferImpl fields */
    IDsDriverImpl*	drv;
    DWORD		buflen;
};

static HRESULT DSDB_MapPrimary(IDsDriverBufferImpl *dsdb)
{
    WINE_WAVEOUT *wwo = &(WOutDev[dsdb->drv->wDevID]);
    if (!wwo->mapping) {
	wwo->mapping = mmap(NULL, wwo->maplen, PROT_WRITE, MAP_SHARED,
			    wwo->unixdev, 0);
	if (wwo->mapping == (LPBYTE)-1) {
	    ERR("(%p): Could not map sound device for direct access (errno=%d)\n", dsdb, errno);
	    return DSERR_GENERIC;
	}
	TRACE("(%p): sound device has been mapped for direct access at %p, size=%ld\n", dsdb, wwo->mapping, wwo->maplen);
    }
    return DS_OK;
}

static HRESULT DSDB_UnmapPrimary(IDsDriverBufferImpl *dsdb)
{
    WINE_WAVEOUT *wwo = &(WOutDev[dsdb->drv->wDevID]);
    if (wwo->mapping) {
	if (munmap(wwo->mapping, wwo->maplen) < 0) {
	    ERR("(%p): Could not unmap sound device (errno=%d)\n", dsdb, errno);
	    return DSERR_GENERIC;
	}
	wwo->mapping = NULL;
	TRACE("(%p): sound device unmapped\n", dsdb);
    }
    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_QueryInterface(PIDSDRIVERBUFFER iface, REFIID riid, LPVOID *ppobj)
{
    /* ICOM_THIS(IDsDriverBufferImpl,iface); */
    FIXME("(): stub!\n");
    return DSERR_UNSUPPORTED;
}

static ULONG WINAPI IDsDriverBufferImpl_AddRef(PIDSDRIVERBUFFER iface)
{
    ICOM_THIS(IDsDriverBufferImpl,iface);
    This->ref++;
    return This->ref;
}

static ULONG WINAPI IDsDriverBufferImpl_Release(PIDSDRIVERBUFFER iface)
{
    ICOM_THIS(IDsDriverBufferImpl,iface);
    if (--This->ref)
	return This->ref;
    if (This == This->drv->primary)
	This->drv->primary = NULL;
    DSDB_UnmapPrimary(This);
    HeapFree(GetProcessHeap(),0,This);
    return 0;
}

static HRESULT WINAPI IDsDriverBufferImpl_Lock(PIDSDRIVERBUFFER iface,
					       LPVOID*ppvAudio1,LPDWORD pdwLen1,
					       LPVOID*ppvAudio2,LPDWORD pdwLen2,
					       DWORD dwWritePosition,DWORD dwWriteLen,
					       DWORD dwFlags)
{
    /* ICOM_THIS(IDsDriverBufferImpl,iface); */
    /* since we (GetDriverDesc flags) have specified DSDDESC_DONTNEEDPRIMARYLOCK,
     * and that we don't support secondary buffers, this method will never be called */
    TRACE("(%p): stub\n",iface);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_Unlock(PIDSDRIVERBUFFER iface,
						 LPVOID pvAudio1,DWORD dwLen1,
						 LPVOID pvAudio2,DWORD dwLen2)
{
    /* ICOM_THIS(IDsDriverBufferImpl,iface); */
    TRACE("(%p): stub\n",iface);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetFormat(PIDSDRIVERBUFFER iface,
						    LPWAVEFORMATEX pwfx)
{
    /* ICOM_THIS(IDsDriverBufferImpl,iface); */

    TRACE("(%p,%p)\n",iface,pwfx);
    /* On our request (GetDriverDesc flags), DirectSound has by now used
     * waveOutClose/waveOutOpen to set the format...
     * unfortunately, this means our mmap() is now gone...
     * so we need to somehow signal to our DirectSound implementation
     * that it should completely recreate this HW buffer...
     * this unexpected error code should do the trick... */
    return DSERR_BUFFERLOST;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetFrequency(PIDSDRIVERBUFFER iface, DWORD dwFreq)
{
    /* ICOM_THIS(IDsDriverBufferImpl,iface); */
    TRACE("(%p,%ld): stub\n",iface,dwFreq);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetVolumePan(PIDSDRIVERBUFFER iface, PDSVOLUMEPAN pVolPan)
{
    /* ICOM_THIS(IDsDriverBufferImpl,iface); */
    FIXME("(%p,%p): stub!\n",iface,pVolPan);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetPosition(PIDSDRIVERBUFFER iface, DWORD dwNewPos)
{
    /* ICOM_THIS(IDsDriverImpl,iface); */
    TRACE("(%p,%ld): stub\n",iface,dwNewPos);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_GetPosition(PIDSDRIVERBUFFER iface,
						      LPDWORD lpdwPlay, LPDWORD lpdwWrite)
{
    ICOM_THIS(IDsDriverBufferImpl,iface);
    count_info info;
    DWORD ptr;

    TRACE("(%p)\n",iface);
    if (ioctl(WOutDev[This->drv->wDevID].unixdev, SNDCTL_DSP_GETOPTR, &info) < 0) {
	ERR("ioctl failed (%d)\n", errno);
	return DSERR_GENERIC;
    }
    ptr = info.ptr & ~3; /* align the pointer, just in case */
    if (lpdwPlay) *lpdwPlay = ptr;
    if (lpdwWrite) {
	/* add some safety margin (not strictly necessary, but...) */
	*lpdwWrite = ptr + 32;
	while (*lpdwWrite > This->buflen)
	    *lpdwWrite -= This->buflen;
    }
    TRACE("playpos=%ld, writepos=%ld\n", lpdwPlay?*lpdwPlay:0, lpdwWrite?*lpdwWrite:0);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_Play(PIDSDRIVERBUFFER iface, DWORD dwRes1, DWORD dwRes2, DWORD dwFlags)
{
    ICOM_THIS(IDsDriverBufferImpl,iface);
    int enable = PCM_ENABLE_OUTPUT;
    TRACE("(%p,%lx,%lx,%lx)\n",iface,dwRes1,dwRes2,dwFlags);
    if (ioctl(WOutDev[This->drv->wDevID].unixdev, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
	ERR("ioctl failed (%d)\n", errno);
	return DSERR_GENERIC;
    }
    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_Stop(PIDSDRIVERBUFFER iface)
{
    ICOM_THIS(IDsDriverBufferImpl,iface);
    int enable = 0;
    TRACE("(%p)\n",iface);
    /* no more playing */
    if (ioctl(WOutDev[This->drv->wDevID].unixdev, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
	ERR("ioctl failed (%d)\n", errno);
	return DSERR_GENERIC;
    }
#if 0
    /* the play position must be reset to the beginning of the buffer */
    if (ioctl(WOutDev[This->drv->wDevID].unixdev, SNDCTL_DSP_RESET, 0) < 0) {
	ERR("ioctl failed (%d)\n", errno);
	return DSERR_GENERIC;
    }
#endif
    return DS_OK;
}

static ICOM_VTABLE(IDsDriverBuffer) dsdbvt =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDsDriverBufferImpl_QueryInterface,
    IDsDriverBufferImpl_AddRef,
    IDsDriverBufferImpl_Release,
    IDsDriverBufferImpl_Lock,
    IDsDriverBufferImpl_Unlock,
    IDsDriverBufferImpl_SetFormat,
    IDsDriverBufferImpl_SetFrequency,
    IDsDriverBufferImpl_SetVolumePan,
    IDsDriverBufferImpl_SetPosition,
    IDsDriverBufferImpl_GetPosition,
    IDsDriverBufferImpl_Play,
    IDsDriverBufferImpl_Stop
};

static HRESULT WINAPI IDsDriverImpl_QueryInterface(PIDSDRIVER iface, REFIID riid, LPVOID *ppobj)
{
    /* ICOM_THIS(IDsDriverImpl,iface); */
    FIXME("(%p): stub!\n",iface);
    return DSERR_UNSUPPORTED;
}

static ULONG WINAPI IDsDriverImpl_AddRef(PIDSDRIVER iface)
{
    ICOM_THIS(IDsDriverImpl,iface);
    This->ref++;
    return This->ref;
}

static ULONG WINAPI IDsDriverImpl_Release(PIDSDRIVER iface)
{
    ICOM_THIS(IDsDriverImpl,iface);
    if (--This->ref)
	return This->ref;
    HeapFree(GetProcessHeap(),0,This);
    return 0;
}

static HRESULT WINAPI IDsDriverImpl_GetDriverDesc(PIDSDRIVER iface, PDSDRIVERDESC pDesc)
{
    ICOM_THIS(IDsDriverImpl,iface);
    TRACE("(%p,%p)\n",iface,pDesc);
    pDesc->dwFlags = DSDDESC_DOMMSYSTEMOPEN | DSDDESC_DOMMSYSTEMSETFORMAT |
	DSDDESC_USESYSTEMMEMORY | DSDDESC_DONTNEEDPRIMARYLOCK;
    strcpy(pDesc->szDesc,"WineOSS DirectSound Driver");
    strcpy(pDesc->szDrvName,"wineoss.drv");
    pDesc->dnDevNode		= WOutDev[This->wDevID].waveDesc.dnDevNode;
    pDesc->wVxdId		= 0; 
    pDesc->wReserved		= 0;
    pDesc->ulDeviceNum		= This->wDevID;
    pDesc->dwHeapType		= DSDHEAP_NOHEAP;
    pDesc->pvDirectDrawHeap	= NULL;
    pDesc->dwMemStartAddress	= 0;
    pDesc->dwMemEndAddress	= 0;
    pDesc->dwMemAllocExtra	= 0;
    pDesc->pvReserved1		= NULL;
    pDesc->pvReserved2		= NULL;
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_Open(PIDSDRIVER iface)
{
    ICOM_THIS(IDsDriverImpl,iface);
    int enable;

    TRACE("(%p)\n",iface);
    /* make sure the card doesn't start playing before we want it to */
    if (ioctl(WOutDev[This->wDevID].unixdev, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
	ERR("ioctl failed (%d)\n", errno);
	return DSERR_GENERIC;
    }
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_Close(PIDSDRIVER iface)
{
    ICOM_THIS(IDsDriverImpl,iface);
    TRACE("(%p)\n",iface);
    if (This->primary) {
	ERR("problem with DirectSound: primary not released\n");
	return DSERR_GENERIC;
    }
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_GetCaps(PIDSDRIVER iface, PDSDRIVERCAPS pCaps)
{
    /* ICOM_THIS(IDsDriverImpl,iface); */
    TRACE("(%p,%p)\n",iface,pCaps);
    memset(pCaps, 0, sizeof(*pCaps));
    /* FIXME: need to check actual capabilities */
    pCaps->dwFlags = DSCAPS_PRIMARYMONO | DSCAPS_PRIMARYSTEREO |
	DSCAPS_PRIMARY8BIT | DSCAPS_PRIMARY16BIT;
    pCaps->dwPrimaryBuffers = 1;
    /* the other fields only apply to secondary buffers, which we don't support
     * (unless we want to mess with wavetable synthesizers and MIDI) */
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_CreateSoundBuffer(PIDSDRIVER iface,
						      LPWAVEFORMATEX pwfx,
						      DWORD dwFlags, DWORD dwCardAddress,
						      LPDWORD pdwcbBufferSize,
						      LPBYTE *ppbBuffer,
						      LPVOID *ppvObj)
{
    ICOM_THIS(IDsDriverImpl,iface);
    IDsDriverBufferImpl** ippdsdb = (IDsDriverBufferImpl**)ppvObj;
    HRESULT err;
    audio_buf_info info;

    TRACE("(%p,%p,%lx,%lx)\n",iface,pwfx,dwFlags,dwCardAddress);
    /* we only support primary buffers */
    if (!(dwFlags & DSBCAPS_PRIMARYBUFFER))
	return DSERR_UNSUPPORTED;
    if (This->primary)
	return DSERR_ALLOCATED;
    if (dwFlags & (DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN))
	return DSERR_CONTROLUNAVAIL;

    *ippdsdb = (IDsDriverBufferImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IDsDriverBufferImpl));
    if (*ippdsdb == NULL)
	return DSERR_OUTOFMEMORY;
    ICOM_VTBL(*ippdsdb) = &dsdbvt;
    (*ippdsdb)->ref	= 1;
    (*ippdsdb)->drv	= This;

    /* check how big the DMA buffer is now */
    if (ioctl(WOutDev[This->wDevID].unixdev, SNDCTL_DSP_GETOSPACE, &info) < 0) {
	ERR("ioctl failed (%d)\n", errno);
	HeapFree(GetProcessHeap(),0,*ippdsdb);
	*ippdsdb = NULL;
	return DSERR_GENERIC;
    }
    WOutDev[This->wDevID].maplen = (*ippdsdb)->buflen = info.fragstotal * info.fragsize;

    /* map the DMA buffer */
    err = DSDB_MapPrimary(*ippdsdb);
    if (err != DS_OK) {
	HeapFree(GetProcessHeap(),0,*ippdsdb);
	*ippdsdb = NULL;
	return err;
    }

    /* primary buffer is ready to go */
    *pdwcbBufferSize	= WOutDev[This->wDevID].maplen;
    *ppbBuffer		= WOutDev[This->wDevID].mapping;

    This->primary = *ippdsdb;

    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_DuplicateSoundBuffer(PIDSDRIVER iface,
							 PIDSDRIVERBUFFER pBuffer,
							 LPVOID *ppvObj)
{
    /* ICOM_THIS(IDsDriverImpl,iface); */
    TRACE("(%p,%p): stub\n",iface,pBuffer);
    return DSERR_INVALIDCALL;
}

static ICOM_VTABLE(IDsDriver) dsdvt =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDsDriverImpl_QueryInterface,
    IDsDriverImpl_AddRef,
    IDsDriverImpl_Release,
    IDsDriverImpl_GetDriverDesc,
    IDsDriverImpl_Open,
    IDsDriverImpl_Close,
    IDsDriverImpl_GetCaps,
    IDsDriverImpl_CreateSoundBuffer,
    IDsDriverImpl_DuplicateSoundBuffer
};

static DWORD wodDsCreate(UINT wDevID, PIDSDRIVER* drv)
{
    IDsDriverImpl** idrv = (IDsDriverImpl**)drv;

    /* the HAL isn't much better than the HEL if we can't do mmap() */
    if (!(WOutDev[wDevID].caps.dwSupport & WAVECAPS_DIRECTSOUND)) {
	ERR("DirectSound flag not set\n");
	MESSAGE("This sound card's driver does not support direct access\n");
	MESSAGE("The (slower) DirectSound HEL mode will be used instead.\n");
	return MMSYSERR_NOTSUPPORTED;
    }

    *idrv = (IDsDriverImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IDsDriverImpl));
    if (!*idrv)
	return MMSYSERR_NOMEM;
    ICOM_VTBL(*idrv)	= &dsdvt;
    (*idrv)->ref	= 1;

    (*idrv)->wDevID	= wDevID;
    (*idrv)->primary	= NULL;
    return MMSYSERR_NOERROR;
}

/*======================================================================*
 *                  Low level WAVE IN implemantation			*
 *======================================================================*/

/*======================================================================*
 *                  Low level WAVE IN implemantation			*
 *======================================================================*/

/**************************************************************************
 * 			widGetDevCaps				[internal]
 */
static DWORD widGetDevCaps(WORD wDevID, LPWAVEINCAPSA lpCaps, DWORD dwSize)
{
    TRACE("(%u, %p, %lu);\n", wDevID, lpCaps, dwSize);
    
    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;
    
    if (wDevID >= MAX_WAVEINDRV) {
	TRACE("MAX_WAVINDRV reached !\n");
	return MMSYSERR_BADDEVICEID;
    }

    memcpy(lpCaps, &WInDev[wDevID].caps, min(dwSize, sizeof(*lpCaps)));
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widRecorder			[internal]
 */
static	DWORD	CALLBACK	widRecorder(LPVOID pmt)
{
    WORD		uDevID = (DWORD)pmt;
    WINE_WAVEIN*	wwi = (WINE_WAVEIN*)&WInDev[uDevID];
    WAVEHDR*		lpWaveHdr;
    DWORD		dwSleepTime;
    MSG			msg;
    DWORD		bytesRead;

    PeekMessageA(&msg, 0, 0, 0, 0);
    wwi->state = WINE_WS_STOPPED;
    wwi->dwTotalRecorded = 0;

    TRACE("imhere[0]\n");
    SetEvent(wwi->hEvent);

    /* make sleep time to be # of ms to output a fragment */
    dwSleepTime = (wwi->dwFragmentSize * 1000) / wwi->format.wf.nAvgBytesPerSec;

    for (;;) {
	/* wait for dwSleepTime or an event in thread's queue */
	/* FIXME: could improve wait time depending on queue state,
	 * ie, number of queued fragments
	 */
	TRACE("imhere[1]\n");

	if (wwi->lpQueuePtr != NULL) {
	    lpWaveHdr = wwi->lpQueuePtr;
	    TRACE("recording buf=%p size=%lu/read=%lu \n",
		  lpWaveHdr->lpData, wwi->lpQueuePtr->dwBufferLength, lpWaveHdr->dwBytesRecorded);

	    bytesRead = read(wwi->unixdev, lpWaveHdr->lpData + lpWaveHdr->dwBytesRecorded, 
			     lpWaveHdr->dwBufferLength - lpWaveHdr->dwBytesRecorded);

	    if (bytesRead > 0) {
		TRACE("Read=%lu (%ld)\n", bytesRead, lpWaveHdr->dwBufferLength);
		lpWaveHdr->dwBytesRecorded += bytesRead;
		wwi->dwTotalRecorded += bytesRead;
		if (lpWaveHdr->dwBytesRecorded == lpWaveHdr->dwBufferLength) {
		    /* removes the current block from the queue */
		    wwi->lpQueuePtr = lpWaveHdr->lpNext;

		    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
		    lpWaveHdr->dwFlags |= WHDR_DONE;
	
		    if (OSS_NotifyClient(uDevID, WIM_DATA, (DWORD)lpWaveHdr, lpWaveHdr->dwBytesRecorded) != MMSYSERR_NOERROR) {
			WARN("can't notify client !\n");
		    }

		}
	    } else {
		TRACE("No data (%s)\n", strerror(errno));
	    }
	}

	MsgWaitForMultipleObjects(0, NULL, FALSE, dwSleepTime, QS_POSTMESSAGE);
	TRACE("imhere[2] (q=%p)\n", wwi->lpQueuePtr);

	while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
	    switch (msg.message) {
	    case WINE_WM_PAUSING:
		wwi->state = WINE_WS_PAUSED;
		SetEvent(wwi->hEvent);
		break;
	    case WINE_WM_RESTARTING:
		wwi->state = WINE_WS_PLAYING;
		SetEvent(wwi->hEvent);
		break;
	    case WINE_WM_HEADER:
		lpWaveHdr = (LPWAVEHDR)msg.lParam;
		lpWaveHdr->lpNext = 0;

		/* insert buffer at the end of queue */
		{
		    LPWAVEHDR*	wh;
		    for (wh = &(wwi->lpQueuePtr); *wh; wh = &((*wh)->lpNext));
		    *wh = lpWaveHdr;
		}
		if (wwi->state == WINE_WS_STOPPED)
		    wwi->state = WINE_WS_PLAYING;
		break;
	    case WINE_WM_RESETTING:
		wwi->state = WINE_WS_STOPPED;
		/* return all buffers to the app */
		for (lpWaveHdr = wwi->lpQueuePtr; lpWaveHdr; lpWaveHdr = lpWaveHdr->lpNext) {
		    TRACE("reset %p %p\n", lpWaveHdr, lpWaveHdr->lpNext);
		    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
		    lpWaveHdr->dwFlags |= WHDR_DONE;
	
		    if (OSS_NotifyClient(uDevID, WIM_DATA, (DWORD)lpWaveHdr, 
					 lpWaveHdr->dwBytesRecorded) != MMSYSERR_NOERROR) {
			WARN("can't notify client !\n");
		    }
		}
		wwi->lpQueuePtr = NULL;
		SetEvent(wwi->hEvent);
		break;
	    case WINE_WM_CLOSING:
		wwi->hThread = 0;
		wwi->state = WINE_WS_CLOSED;
		SetEvent(wwi->hEvent);
		ExitThread(0);
		/* shouldn't go here */
	    default:
		FIXME("unknown message %d\n", msg.message);
		break;
	    }
	}
    }
    ExitThread(0);
    /* just for not generating compilation warnings... should never be executed */
    return 0; 
}


/**************************************************************************
 * 				widOpen				[internal]
 */
static DWORD widOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    int 		audio, abuf_size, smplrate, samplesize, dsp_stereo;
    LPWAVEFORMAT	lpFormat;
    WINE_WAVEIN*	wwi;

    TRACE("(%u, %p, %08lX);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN("Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_WAVEINDRV) return MMSYSERR_BADDEVICEID;

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

    wwi = &WInDev[wDevID];
    wwi->unixdev = 0;
    if (access(SOUND_DEV,0) != 0) return MMSYSERR_NOTENABLED;
    audio = open(SOUND_DEV, O_RDONLY|O_NDELAY, 0);
    if (audio == -1) {
	WARN("can't open (%s)!\n", strerror(errno));
	return MMSYSERR_ALLOCATED;
    }
    IOCTL(audio, SNDCTL_DSP_GETBLKSIZE, abuf_size);
    if (abuf_size < 1024 || abuf_size > 65536) {
	if (abuf_size == -1)
        {
            WARN("IOCTL can't 'SNDCTL_DSP_GETBLKSIZE' !\n");
	    close(audio);
            return MMSYSERR_NOTENABLED;
        }
        WARN("SNDCTL_DSP_GETBLKSIZE Invalid dwFragmentSize %d!\n",abuf_size);
    }
    wwi->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    if (wwi->lpQueuePtr) {
	WARN("Should have an empty queue (%p)\n", wwi->lpQueuePtr);
	wwi->lpQueuePtr = NULL;
    }
    wwi->unixdev = audio;
    wwi->dwFragmentSize = abuf_size;
    wwi->dwTotalRecorded = 0;
    memcpy(&wwi->waveDesc, lpDesc, sizeof(WAVEOPENDESC));
    lpFormat = (LPWAVEFORMAT) lpDesc->lpFormat; 

    memcpy(&wwi->format, lpFormat, sizeof(PCMWAVEFORMAT));
    wwi->format.wBitsPerSample = 8; /* <-------------- */
    if (wwi->format.wf.nChannels == 0) return WAVERR_BADFORMAT;
    if (wwi->format.wf.nSamplesPerSec == 0) return WAVERR_BADFORMAT;
    if (wwi->format.wBitsPerSample == 0) {
	wwi->format.wBitsPerSample = 8 *
	    (wwi->format.wf.nAvgBytesPerSec /
	     wwi->format.wf.nSamplesPerSec) /
	    wwi->format.wf.nChannels;
    }
    samplesize = wwi->format.wBitsPerSample;
    smplrate = wwi->format.wf.nSamplesPerSec;
    dsp_stereo = (wwi->format.wf.nChannels > 1) ? TRUE : FALSE;
    IOCTL(audio, SNDCTL_DSP_SPEED, smplrate);
    IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, samplesize);
    IOCTL(audio, SNDCTL_DSP_STEREO, dsp_stereo);
    TRACE("wBitsPerSample=%u !\n", wwi->format.wBitsPerSample);
    TRACE("nSamplesPerSec=%lu !\n", wwi->format.wf.nSamplesPerSec);
    TRACE("nChannels=%u !\n", wwi->format.wf.nChannels);
    TRACE("nAvgBytesPerSec=%lu\n", wwi->format.wf.nAvgBytesPerSec); 

    wwi->hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    wwi->hThread = CreateThread(NULL, 0, widRecorder, (LPVOID)(DWORD)wDevID, 0, &(wwi->dwThreadID));
    WaitForSingleObject(wwi->hEvent, INFINITE);

   if (OSS_NotifyClient(wDevID, WIM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
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
    WINE_WAVEIN*	wwi;

    TRACE("(%u);\n", wDevID);
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].unixdev == 0) {
	WARN("can't close !\n");
	return MMSYSERR_INVALHANDLE;
    }

    wwi = &WInDev[wDevID];

    if (wwi->lpQueuePtr != NULL) {
	WARN("still buffers open !\n");
	return WAVERR_STILLPLAYING;
    }

    PostThreadMessageA(wwi->dwThreadID, WINE_WM_CLOSING, 0, 0);
    WaitForSingleObject(wwi->hEvent, INFINITE);
    CloseHandle(wwi->hEvent);
    close(wwi->unixdev);
    wwi->unixdev = 0;
    wwi->dwFragmentSize = 0;
    if (OSS_NotifyClient(wDevID, WIM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
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
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);

    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].unixdev == 0) {
	WARN("can't do it !\n");
	return MMSYSERR_INVALHANDLE;
    }
    if (!(lpWaveHdr->dwFlags & WHDR_PREPARED)) {
	TRACE("never been prepared !\n");
	return WAVERR_UNPREPARED;
    }
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE) {
	TRACE("header already in use !\n");
	return WAVERR_STILLPLAYING;
    }
    lpWaveHdr->dwFlags |= WHDR_INQUEUE;
    lpWaveHdr->dwFlags &= ~WHDR_DONE;
    lpWaveHdr->dwBytesRecorded = 0;
    lpWaveHdr->lpNext = NULL;
    PostThreadMessageA(WInDev[wDevID].dwThreadID, WINE_WM_HEADER, 0, (DWORD)lpWaveHdr);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widPrepare			[internal]
 */
static DWORD widPrepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);

    if (wDevID >= MAX_WAVEINDRV) return MMSYSERR_INVALHANDLE;

    if (lpWaveHdr->dwFlags & WHDR_INQUEUE)
	return WAVERR_STILLPLAYING;

    lpWaveHdr->dwFlags |= WHDR_PREPARED;
    lpWaveHdr->dwFlags &= ~(WHDR_INQUEUE|WHDR_DONE);
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
    if (wDevID >= MAX_WAVEINDRV) return MMSYSERR_INVALHANDLE;

    if (lpWaveHdr->dwFlags & WHDR_INQUEUE)
	return WAVERR_STILLPLAYING;

    lpWaveHdr->dwFlags &= ~(WHDR_PREPARED|WHDR_INQUEUE);
    lpWaveHdr->dwFlags |= WHDR_DONE;
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widStart				[internal]
 */
static DWORD widStart(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].unixdev == 0) {
	WARN("can't start recording !\n");
	return MMSYSERR_INVALHANDLE;
    }
    
    PostThreadMessageA(WInDev[wDevID].dwThreadID, WINE_WM_RESTARTING, 0, 0);
    WaitForSingleObject(WInDev[wDevID].hEvent, INFINITE);
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widStop					[internal]
 */
static DWORD widStop(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].unixdev == 0) {
	WARN("can't stop !\n");
	return MMSYSERR_INVALHANDLE;
    }
    /* FIXME: reset aint stop */
    PostThreadMessageA(WInDev[wDevID].dwThreadID, WINE_WM_RESETTING, 0, 0);
    WaitForSingleObject(WInDev[wDevID].hEvent, INFINITE);
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widReset				[internal]
 */
static DWORD widReset(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].unixdev == 0) {
	WARN("can't reset !\n");
	return MMSYSERR_INVALHANDLE;
    }
    PostThreadMessageA(WInDev[wDevID].dwThreadID, WINE_WM_RESETTING, 0, 0);
    WaitForSingleObject(WInDev[wDevID].hEvent, INFINITE);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widGetPosition			[internal]
 */
static DWORD widGetPosition(WORD wDevID, LPMMTIME lpTime, DWORD uSize)
{
    int			time;
    WINE_WAVEIN*	wwi;
    
    TRACE("(%u, %p, %lu);\n", wDevID, lpTime, uSize);

    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].unixdev == 0) {
	WARN("can't get pos !\n");
	return MMSYSERR_INVALHANDLE;
    }
    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;

    wwi = &WInDev[wDevID];

    TRACE("wType=%04X !\n", lpTime->wType);
    TRACE("wBitsPerSample=%u\n", wwi->format.wBitsPerSample); 
    TRACE("nSamplesPerSec=%lu\n", wwi->format.wf.nSamplesPerSec); 
    TRACE("nChannels=%u\n", wwi->format.wf.nChannels); 
    TRACE("nAvgBytesPerSec=%lu\n", wwi->format.wf.nAvgBytesPerSec); 
    switch (lpTime->wType) {
    case TIME_BYTES:
	lpTime->u.cb = wwi->dwTotalRecorded;
	TRACE("TIME_BYTES=%lu\n", lpTime->u.cb);
	break;
    case TIME_SAMPLES:
	lpTime->u.sample = wwi->dwTotalRecorded * 8 /
	    wwi->format.wBitsPerSample;
	TRACE("TIME_SAMPLES=%lu\n", lpTime->u.sample);
	break;
    case TIME_SMPTE:
	time = wwi->dwTotalRecorded /
	    (wwi->format.wf.nAvgBytesPerSec / 1000);
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
	lpTime->u.ms = wwi->dwTotalRecorded /
	    (wwi->format.wf.nAvgBytesPerSec / 1000);
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
    case WIDM_OPEN:		return widOpen       (wDevID, (LPWAVEOPENDESC)dwParam1, dwParam2);
    case WIDM_CLOSE:		return widClose      (wDevID);
    case WIDM_ADDBUFFER:	return widAddBuffer  (wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WIDM_PREPARE:		return widPrepare    (wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WIDM_UNPREPARE:	return widUnprepare  (wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WIDM_GETDEVCAPS:	return widGetDevCaps (wDevID, (LPWAVEINCAPSA)dwParam1, dwParam2);
    case WIDM_GETNUMDEVS:	return wodGetNumDevs ();	/* same number of devices in output as in input */
    case WIDM_GETPOS:		return widGetPosition(wDevID, (LPMMTIME)dwParam1, dwParam2);
    case WIDM_RESET:		return widReset      (wDevID);
    case WIDM_START:		return widStart      (wDevID);
    case WIDM_STOP:		return widStop       (wDevID);
    default:
	FIXME("unknown message %u!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

#else /* !HAVE_OSS */

/**************************************************************************
 * 				OSS_wodMessage			[sample driver]
 */
DWORD WINAPI OSS_wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			    DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08lX, %08lX, %08lX):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				OSS_widMessage			[sample driver]
 */
DWORD WINAPI OSS_widMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			    DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08lX, %08lX, %08lX):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

#endif /* HAVE_OSS */
