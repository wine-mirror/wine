/*
 * Wine Driver for Libaudioio
 * Derived from the Wine OSS Sample Driver
 * Copyright 1994 Martin Ayotte
 *           1999 Eric Pouech (async playing in waveOut/waveIn)
 *	     2000 Eric Pouech (loops in waveOut)
 *	     2002 Robert Lunnon (Modifications for libaudioio)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 *
 * Note large hacks done to effectively disable DSound altogether
 * Also Input is not yet working (Lots more work to be done there)
 * But this does make a reasonable output driver for solaris until the arts
 * driver comes up to speed
 */

/*
 * FIXME:
 *	pause in waveOut does not work correctly
 *	full duplex (in/out) is not working (device is opened twice for Out
 *	and In) (OSS is known for its poor duplex capabilities, alsa is
 *	better)
 */

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#ifdef HAVE_LIBAUDIOIO_H
#include <libaudioio.h>
#endif
#include "windef.h"
#include "winbase.h"
#include "wine/unicode.h"
#include "wingdi.h"
#include "winerror.h"
#include "winuser.h"
#include "mmddk.h"
#include "dsound.h"
#include "dsdriver.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wave);

#ifdef HAVE_LIBAUDIOIO

/* Allow 1% deviation for sample rates (some ES137x cards) */
#define NEAR_MATCH(rate1,rate2) (((100*((int)(rate1)-(int)(rate2)))/(rate1))==0)

#define SOUND_DEV "/dev/audio"
#define DEFAULT_FRAGMENT_SIZE 4096



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

#define WINE_WM_FIRST WINE_WM_PAUSING
#define WINE_WM_LAST WINE_WM_HEADER

typedef struct {
    int msg;
    DWORD param;
} WWO_MSG;

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
    DWORD			dwLoops;		/* private copy of loop counter */

    DWORD			dwLastFragDone;		/* time in ms, when last played fragment will be actually played */
    DWORD			dwPlayedTotal;		/* number of bytes played since opening */

    /* info on current lpQueueHdr->lpWaveHdr */
    DWORD			dwOffCurrHdr;		/* offset in lpPlayPtr->lpData for fragments */
    DWORD			dwRemain;		/* number of bytes to write to end the current fragment  */

    /* synchronization stuff */
    HANDLE			hThread;
    DWORD			dwThreadID;
    HANDLE			hEvent;
#define WWO_RING_BUFFER_SIZE	30
    WWO_MSG			messages[WWO_RING_BUFFER_SIZE];
    int				msg_tosave;
    int				msg_toget;
    HANDLE			msg_event;
    CRITICAL_SECTION		msg_crst;
    WAVEOUTCAPSW		caps;

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
    WAVEINCAPSW			caps;
    BOOL                        bTriggerSupport;

    /* synchronization stuff */
    HANDLE			hThread;
    DWORD			dwThreadID;
    HANDLE			hEvent;
} WINE_WAVEIN;

static WINE_WAVEOUT	WOutDev   [MAX_WAVEOUTDRV];
static WINE_WAVEIN	WInDev    [MAX_WAVEINDRV ];

static DWORD wodDsCreate(UINT wDevID, PIDSDRIVER* drv);
static DWORD widDsCreate(UINT wDevID, PIDSCDRIVER* drv);
static DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc);
static DWORD widDsDesc(UINT wDevID, PDSDRIVERDESC desc);

static DWORD bytes_to_mmtime(LPMMTIME lpTime, DWORD position,
                             PCMWAVEFORMAT* format)
{
    TRACE("wType=%04X wBitsPerSample=%u nSamplesPerSec=%u nChannels=%u nAvgBytesPerSec=%u\n",
          lpTime->wType, format->wBitsPerSample, format->wf.nSamplesPerSec,
          format->wf.nChannels, format->wf.nAvgBytesPerSec);
    TRACE("Position in bytes=%u\n", position);

    switch (lpTime->wType) {
    case TIME_SAMPLES:
        lpTime->u.sample = position / (format->wBitsPerSample / 8 * format->wf.nChannels);
        TRACE("TIME_SAMPLES=%u\n", lpTime->u.sample);
        break;
    case TIME_MS:
        lpTime->u.ms = 1000.0 * position / (format->wBitsPerSample / 8 * format->wf.nChannels * format->wf.nSamplesPerSec);
        TRACE("TIME_MS=%u\n", lpTime->u.ms);
        break;
    case TIME_SMPTE:
        lpTime->u.smpte.fps = 30;
        position = position / (format->wBitsPerSample / 8 * format->wf.nChannels);
        position += (format->wf.nSamplesPerSec / lpTime->u.smpte.fps) - 1; /* round up */
        lpTime->u.smpte.sec = position / format->wf.nSamplesPerSec;
        position -= lpTime->u.smpte.sec * format->wf.nSamplesPerSec;
        lpTime->u.smpte.min = lpTime->u.smpte.sec / 60;
        lpTime->u.smpte.sec -= 60 * lpTime->u.smpte.min;
        lpTime->u.smpte.hour = lpTime->u.smpte.min / 60;
        lpTime->u.smpte.min -= 60 * lpTime->u.smpte.hour;
        lpTime->u.smpte.fps = 30;
        lpTime->u.smpte.frame = position * lpTime->u.smpte.fps / format->wf.nSamplesPerSec;
        TRACE("TIME_SMPTE=%02u:%02u:%02u:%02u\n",
              lpTime->u.smpte.hour, lpTime->u.smpte.min,
              lpTime->u.smpte.sec, lpTime->u.smpte.frame);
        break;
    default:
        FIXME("Format %d not supported, using TIME_BYTES !\n", lpTime->wType);
        lpTime->wType = TIME_BYTES;
        /* fall through */
    case TIME_BYTES:
        lpTime->u.cb = position;
        TRACE("TIME_BYTES=%u\n", lpTime->u.cb);
        break;
    }
    return MMSYSERR_NOERROR;
}

/*======================================================================*
 *                  Low level WAVE implementation			*
 *======================================================================*/
SampleSpec_t spec[2];



LONG LIBAUDIOIO_WaveInit(void)
{
    int 	audio;
    int		smplrate;
    int		samplesize = 16;
    int		dsp_stereo = 1;
    int		bytespersmpl;
    int 	caps;
    int		mask;
    int 	i;

    int audio_fd;
    int mode;

    static const WCHAR ini_out[] = {'A','u','d','i','o','I','O',' ','W','a','v','e','O','u','t',' ','D','r','i','v','e','r',0};
    static const WCHAR ini_in [] = {'A','u','d','i','o','I','O',' ','W','a','v','e','I','n',' ',' ','D','r','i','v','e','r',0};

    TRACE("Init ENTERED (%d) (%d) rate = %d\n",CLIENT_PLAYBACK,CLIENT_RECORD,spec[CLIENT_PLAYBACK].rate);
    spec[CLIENT_RECORD].channels=spec[CLIENT_PLAYBACK].channels=2;
    spec[CLIENT_RECORD].max_blocks=spec[CLIENT_PLAYBACK].max_blocks=16;
    spec[CLIENT_RECORD].rate=spec[CLIENT_PLAYBACK].rate=44100;
    spec[CLIENT_RECORD].encoding=spec[CLIENT_PLAYBACK].encoding= ENCODE_PCM;
    spec[CLIENT_RECORD].precision=spec[CLIENT_PLAYBACK].precision=16 ;
    spec[CLIENT_RECORD].endian=spec[CLIENT_PLAYBACK].endian=ENDIAN_INTEL;
    spec[CLIENT_RECORD].disable_threads=spec[CLIENT_PLAYBACK].disable_threads=1;
    spec[CLIENT_RECORD].type=spec[CLIENT_PLAYBACK].type=TYPE_SIGNED; /* in 16 bit mode this is what typical PC hardware expects */

    mode = O_WRONLY|O_NDELAY;

    /* start with output device */

    /* initialize all device handles to -1 */
    for (i = 0; i < MAX_WAVEOUTDRV; ++i)
    {
        WOutDev[i].unixdev = -1;
    }

    /* FIXME: only one device is supported */
    memset(&WOutDev[0].caps, 0, sizeof(WOutDev[0].caps));

    WOutDev[0].caps.wMid = 0x00FF; 	/* Manufac ID */
    WOutDev[0].caps.wPid = 0x0001; 	/* Product ID */
    strcpyW(WOutDev[0].caps.szPname, ini_out);
    WOutDev[0].caps.vDriverVersion = 0x0100;
    WOutDev[0].caps.dwFormats = 0x00000000;
    WOutDev[0].caps.dwSupport = WAVECAPS_VOLUME;

    /*
     * Libaudioio works differently, you tell it what spec audio you want to write
     * and it guarantees to match it by converting to the final format on the fly.
     * So we don't need to read  back and compare.
     */

    bytespersmpl = spec[CLIENT_PLAYBACK].precision/8;

    WOutDev[0].caps.wChannels = spec[CLIENT_PLAYBACK].channels;


/* Fixme: Libaudioio 0.2 doesn't support balance yet (Libaudioio 0.3 does so this must be fixed later)*/

/*    if (WOutDev[0].caps.wChannels > 1) WOutDev[0].caps.dwSupport |= WAVECAPS_LRVOLUME;*/
    TRACE("Init sammplerate= %d\n",spec[CLIENT_PLAYBACK].rate);

    smplrate = spec[CLIENT_PLAYBACK].rate;
/*
 * We have set up the data format to be 16 bit signed in intel format
 * For Big Endian machines libaudioio will convert the data to bigendian for us
 */

    WOutDev[0].caps.dwFormats |= WAVE_FORMAT_4M16;
    if (WOutDev[0].caps.wChannels > 1)
        WOutDev[0].caps.dwFormats |= WAVE_FORMAT_4S16;

/* Don't understand this yet, but I don't think this functionality is portable, leave it here for future evaluation
 *   if (IOCTL(audio, SNDCTL_DSP_GETCAPS, caps) == 0) {
 *	TRACE("OSS dsp out caps=%08X\n", caps);
 *	if ((caps & DSP_CAP_REALTIME) && !(caps & DSP_CAP_BATCH)) {
 *	    WOutDev[0].caps.dwSupport |= WAVECAPS_SAMPLEACCURATE;
 *	}
 */	/* well, might as well use the DirectSound cap flag for something */
/*	if ((caps & DSP_CAP_TRIGGER) && (caps & DSP_CAP_MMAP) &&
 *	    !(caps & DSP_CAP_BATCH))
 *	    WOutDev[0].caps.dwSupport |= WAVECAPS_DIRECTSOUND;
 *   }
 */


/*
 * Note that libaudioio audio capture is not proven yet, in our open call
 * set the spec for input audio the same as for output
 */
    TRACE("out dwFormats = %08X, dwSupport = %08X\n",
	  WOutDev[0].caps.dwFormats, WOutDev[0].caps.dwSupport);

    for (i = 0; i < MAX_WAVEINDRV; ++i)
    {
        WInDev[i].unixdev = -1;
    }

    memset(&WInDev[0].caps, 0, sizeof(WInDev[0].caps));

    WInDev[0].caps.wMid = 0x00FF; 	/* Manufac ID */
    WInDev[0].caps.wPid = 0x0001; 	/* Product ID */
    strcpyW(WInDev[0].caps.szPname, ini_in);
    WInDev[0].caps.dwFormats = 0x00000000;
    WInDev[0].caps.wChannels = spec[CLIENT_RECORD].channels;

    WInDev[0].bTriggerSupport = TRUE;    /* Maybe :-) */

    bytespersmpl = spec[CLIENT_RECORD].precision/8;
    smplrate = spec[CLIENT_RECORD].rate;

	    WInDev[0].caps.dwFormats |= WAVE_FORMAT_4M16;
	    if (WInDev[0].caps.wChannels > 1)
		WInDev[0].caps.dwFormats |= WAVE_FORMAT_4S16;


    TRACE("in dwFormats = %08X\n", WInDev[0].caps.dwFormats);

    return 0;
}

/**************************************************************************
 * 			LIBAUDIOIO_NotifyClient			[internal]
 */
static DWORD LIBAUDIOIO_NotifyClient(UINT wDevID, WORD wMsg, DWORD dwParam1, DWORD dwParam2)
{
    TRACE("wDevID = %04X wMsg = 0x%04x dwParm1 = %04X dwParam2 = %04X\n",wDevID, wMsg, dwParam1, dwParam2);

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
 *                  Low level WAVE OUT implementation			*
 *======================================================================*/

/**************************************************************************
 * 				wodPlayer_WriteFragments	[internal]
 *
 * wodPlayer helper. Writes as many fragments as it can to unixdev.
 * Returns TRUE in case of buffer underrun.
 */
static	BOOL	wodPlayer_WriteFragments(WINE_WAVEOUT* wwo)
{
    LPWAVEHDR		lpWaveHdr;
    LPBYTE		lpData;
    int			count;

    TRACE("wodPlayer_WriteFragments sammplerate= %d\n",spec[CLIENT_PLAYBACK].rate);
    for (;;) {


/*
 * Libaudioio doesn't buffer the same way as linux, you can write data is any block size
 *And libaudioio just tracks the number of blocks in the streams queue to control latency
 */

	if (!AudioIOCheckWriteReady()) return FALSE;   /*  Returns false if you have execeeded your specified latency (No space left)*/

	lpWaveHdr = wwo->lpPlayPtr;
	if (!lpWaveHdr) {
	    if (wwo->dwRemain > 0 &&		/* still data to send to complete current fragment */
		wwo->dwLastFragDone &&  	/* first fragment has been played */
		AudioIOCheckUnderrun()) {   /* done with all waveOutWrite()' fragments */
		/* FIXME: should do better handling here */
		WARN("Oooch, buffer underrun !\n");
		return TRUE; /* force resetting of waveOut device */
	    }
	    return FALSE;	/* wait a bit */
	}

	if (wwo->dwOffCurrHdr == 0) {
	    TRACE("Starting a new wavehdr %p of %d bytes\n", lpWaveHdr, lpWaveHdr->dwBufferLength);
	    if (lpWaveHdr->dwFlags & WHDR_BEGINLOOP) {
		if (wwo->lpLoopPtr) {
		    WARN("Already in a loop. Discarding loop on this header (%p)\n", lpWaveHdr);
		} else {
		    TRACE("Starting loop (%dx) with %p\n", lpWaveHdr->dwLoops, lpWaveHdr);
		    wwo->lpLoopPtr = lpWaveHdr;
		    /* Windows does not touch WAVEHDR.dwLoops,
		     * so we need to make an internal copy */
		    wwo->dwLoops = lpWaveHdr->dwLoops;
		}
	    }
	}

	lpData = lpWaveHdr->lpData;

	/* finish current wave hdr ? */
	if (wwo->dwOffCurrHdr + wwo->dwRemain >= lpWaveHdr->dwBufferLength) {
	    DWORD	toWrite = lpWaveHdr->dwBufferLength - wwo->dwOffCurrHdr;

	    /* write end of current wave hdr */
	    count = AudioIOWrite(lpData + wwo->dwOffCurrHdr, toWrite);
	    TRACE("write(%p[%5u], %5u) => %d\n", lpData, wwo->dwOffCurrHdr, toWrite, count);

	    if (count > 0 || toWrite == 0) {
		DWORD	tc = GetTickCount();

		if (wwo->dwLastFragDone /* + guard time ?? */ < tc)
		    wwo->dwLastFragDone = tc;
		wwo->dwLastFragDone += (toWrite * 1000) / wwo->format.wf.nAvgBytesPerSec;

		lpWaveHdr->reserved = wwo->dwLastFragDone;
		TRACE("Tagging hdr %p with %08x\n", lpWaveHdr, wwo->dwLastFragDone);

		/* WAVEHDR written, go to next one */
		if ((lpWaveHdr->dwFlags & WHDR_ENDLOOP) && wwo->lpLoopPtr) {
		    if (--wwo->dwLoops > 0) {
			wwo->lpPlayPtr = wwo->lpLoopPtr;
		    } else {
			/* last one played */
			if (wwo->lpLoopPtr != lpWaveHdr && (lpWaveHdr->dwFlags & WHDR_BEGINLOOP)) {
			    FIXME("Correctly handled case ? (ending loop buffer also starts a new loop)\n");
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
	    count = AudioIOWrite( lpData + wwo->dwOffCurrHdr, wwo->dwRemain);
	    TRACE("write(%p[%5u], %5u) => %d\n", lpData, wwo->dwOffCurrHdr, wwo->dwRemain, count);
	    if (count > 0) {
		DWORD	tc = GetTickCount();

		if (wwo->dwLastFragDone /* + guard time ?? */ < tc)
		    wwo->dwLastFragDone = tc;
		wwo->dwLastFragDone += (wwo->dwRemain * 1000) / wwo->format.wf.nAvgBytesPerSec;

		TRACE("Tagging frag with %08x\n", wwo->dwLastFragDone);

		wwo->dwOffCurrHdr += count;
		wwo->dwRemain = wwo->dwFragmentSize;
	    }
	}
    }

}

int wodPlayer_Message(WINE_WAVEOUT *wwo, int msg, DWORD param)
{
    TRACE("wodPlayerMessage sammplerate= %d  msg=%d\n",spec[CLIENT_PLAYBACK].rate,msg);
    EnterCriticalSection(&wwo->msg_crst);
    if ((wwo->msg_tosave == wwo->msg_toget) /* buffer overflow ? */
    &&  (wwo->messages[wwo->msg_toget].msg))
    {
	ERR("buffer overflow !?\n");
        LeaveCriticalSection(&wwo->msg_crst);
	return 0;
    }

    wwo->messages[wwo->msg_tosave].msg = msg;
    wwo->messages[wwo->msg_tosave].param = param;
    wwo->msg_tosave++;
    if (wwo->msg_tosave > WWO_RING_BUFFER_SIZE-1)
	wwo->msg_tosave = 0;
    LeaveCriticalSection(&wwo->msg_crst);
    /* signal a new message */
    SetEvent(wwo->msg_event);
    return 1;
}

int wodPlayer_RetrieveMessage(WINE_WAVEOUT *wwo, int *msg, DWORD *param)
{
    EnterCriticalSection(&wwo->msg_crst);

    if (wwo->msg_toget == wwo->msg_tosave) /* buffer empty ? */
    {
        LeaveCriticalSection(&wwo->msg_crst);
	return 0;
    }

    *msg = wwo->messages[wwo->msg_toget].msg;
    wwo->messages[wwo->msg_toget].msg = 0;
    *param = wwo->messages[wwo->msg_toget].param;
    wwo->msg_toget++;
    if (wwo->msg_toget > WWO_RING_BUFFER_SIZE-1)
	wwo->msg_toget = 0;
    LeaveCriticalSection(&wwo->msg_crst);
    return 1;
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
	if (LIBAUDIOIO_NotifyClient(uDevID, WOM_DONE, (DWORD)lpWaveHdr, 0) != MMSYSERR_NOERROR) {
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
/*
 *FIXME In the original code I think this aborted IO. With Libaudioio you have to wait
 * The following function just blocks until I/O is complete
 * There is possibly a way to abort the blocks buffered in streams
 * but this approach seems to work OK
 */
 	AudioIOFlush();



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
    int			msg;
    DWORD		param;
    DWORD		tc;
    BOOL                had_msg;

    wwo->state = WINE_WS_STOPPED;

    wwo->dwLastFragDone = 0;
    wwo->dwOffCurrHdr = 0;
    wwo->dwRemain = wwo->dwFragmentSize;
    wwo->lpQueuePtr = wwo->lpPlayPtr = wwo->lpLoopPtr = NULL;
    wwo->dwPlayedTotal = 0;

    TRACE("imhere[0]\n");
    SetEvent(wwo->hEvent);

    for (;;) {
	/* wait for dwSleepTime or an event in thread's queue
	 * FIXME:
	 * - is wait time calculation optimal ?
	 * - these 100 ms parts should be changed, but Eric reports
	 *   that the wodPlayer thread might lock up if we use INFINITE
	 *   (strange !), so I better don't change that now... */
	if (wwo->state != WINE_WS_PLAYING)
	    dwSleepTime = 100;
	else
	{
	    tc = GetTickCount();
	    if (tc < wwo->dwLastFragDone)
	    {
		/* calculate sleep time depending on when the last fragment
		   will be played */
	        dwSleepTime = (wwo->dwLastFragDone - tc)*7/10;
		if (dwSleepTime > 100)
		    dwSleepTime = 100;
	    }
	    else
	        dwSleepTime = 0;
	}

	TRACE("imhere[1] tc = %08x\n", GetTickCount());
	if (dwSleepTime)
	    WaitForSingleObject(wwo->msg_event, dwSleepTime);
	TRACE("imhere[2] (q=%p p=%p) tc = %08x\n", wwo->lpQueuePtr,
	      wwo->lpPlayPtr, GetTickCount());
	had_msg = FALSE;
	    	TRACE("Looking for message\n");
	while (wodPlayer_RetrieveMessage(wwo, &msg, &param)) {
	    had_msg = TRUE;
	    	TRACE("Processing message\n");
	    switch (msg) {
	    case WINE_WM_PAUSING:
	    	TRACE("WINE_WM_PAUSING\n");
		wodPlayer_Reset(wwo, uDevID, FALSE);
		wwo->state = WINE_WS_PAUSED;
		SetEvent(wwo->hEvent);
		break;
	    case WINE_WM_RESTARTING:
	    	TRACE("WINE_WM_RESTARTING\n");
		wwo->state = WINE_WS_PLAYING;
		SetEvent(wwo->hEvent);
		break;
	    case WINE_WM_HEADER:
	    	TRACE("WINE_WM_HEADER\n");
		lpWaveHdr = (LPWAVEHDR)param;

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
	    	TRACE("WINE_WM_RESETTING\n");	    
		wodPlayer_Reset(wwo, uDevID, TRUE);
		SetEvent(wwo->hEvent);
		break;
	    case WINE_WM_CLOSING:
	    	TRACE("WINE_WM_CLOSING\n");	    
		/* sanity check: this should not happen since the device must have been reset before */
		if (wwo->lpQueuePtr || wwo->lpPlayPtr) ERR("out of sync\n");
		wwo->hThread = 0;
		wwo->state = WINE_WS_CLOSED;
		SetEvent(wwo->hEvent);
		ExitThread(0);
		/* shouldn't go here */
	    default:
		FIXME("unknown message %d\n", msg);
		break;
	    }
	    if (wwo->state == WINE_WS_PLAYING) {
	    	TRACE("Writing Fragment\n");
		wodPlayer_WriteFragments(wwo);
	    }
	    wodPlayer_Notify(wwo, uDevID, FALSE);
	}

	if (!had_msg) { /* if we've received a msg we've just done this so we
			   won't repeat it */
	    if (wwo->state == WINE_WS_PLAYING) {
	    	TRACE("Writing Fragment (entry 2)\n");
		wodPlayer_WriteFragments(wwo);
	    }
	    wodPlayer_Notify(wwo, uDevID, FALSE);
	}
    }
    ExitThread(0);
    /* just for not generating compilation warnings... should never be executed */
    return 0;
}

/**************************************************************************
 * 			wodGetDevCaps				[internal]
 */
static DWORD wodGetDevCaps(WORD wDevID, LPWAVEOUTCAPSW lpCaps, DWORD dwSize)
{
    TRACE("(%u, %p, %u);\n", wDevID, lpCaps, dwSize);

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
    int			fragment_size;
    int			audio_fragment;
    WINE_WAVEOUT*	wwo;

    TRACE("(%u, %p, %08X);\n", wDevID, lpDesc, dwFlags);
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
	lpDesc->lpFormat->nSamplesPerSec == 0 ||
        (lpDesc->lpFormat->wBitsPerSample!=8 && lpDesc->lpFormat->wBitsPerSample!=16)) {
	WARN("Bad format: tag=%04X nChannels=%d nSamplesPerSec=%d !\n",
	     lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
	     lpDesc->lpFormat->nSamplesPerSec);
	return WAVERR_BADFORMAT;
    }

    if (dwFlags & WAVE_FORMAT_QUERY) {
	TRACE("Query format: tag=%04X nChannels=%d nSamplesPerSec=%d !\n",
	     lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
	     lpDesc->lpFormat->nSamplesPerSec);
	return MMSYSERR_NOERROR;
    }

    wwo = &WOutDev[wDevID];

    if ((dwFlags & WAVE_DIRECTSOUND) && !(wwo->caps.dwSupport & WAVECAPS_DIRECTSOUND))
	/* not supported, ignore it */
	dwFlags &= ~WAVE_DIRECTSOUND;

    if (access(SOUND_DEV, 0) != 0)
	return MMSYSERR_NOTENABLED;

	audio = AudioIOOpenX( O_WRONLY|O_NDELAY,&spec[CLIENT_PLAYBACK],&spec[CLIENT_PLAYBACK]);

    if (audio == -1) {
	WARN("can't open sound device %s (%s)!\n", SOUND_DEV, strerror(errno));
	return MMSYSERR_ALLOCATED;
    }
 /*   fcntl(audio, F_SETFD, 1); *//* set close on exec flag - Dunno about this (RL)*/
    wwo->unixdev = audio;
    wwo->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    wwo->waveDesc = *lpDesc;
    memcpy(&wwo->format,   lpDesc->lpFormat, sizeof(PCMWAVEFORMAT));

    if (wwo->format.wBitsPerSample == 0) {
	WARN("Resetting zeroed wBitsPerSample\n");
	wwo->format.wBitsPerSample = 8 *
	    (wwo->format.wf.nAvgBytesPerSec /
	     wwo->format.wf.nSamplesPerSec) /
	    wwo->format.wf.nChannels;
    }

    if (dwFlags & WAVE_DIRECTSOUND) {
        if (wwo->caps.dwSupport & WAVECAPS_SAMPLEACCURATE)
	    /* we have realtime DirectSound, fragments just waste our time,
	     * but a large buffer is good, so choose 64KB (32 * 2^11) */
	    audio_fragment = 0x0020000B;
	else
	    /* to approximate realtime, we must use small fragments,
	     * let's try to fragment the above 64KB (256 * 2^8) */
	    audio_fragment = 0x01000008;
    } else {
	/* shockwave player uses only 4 1k-fragments at a rate of 22050 bytes/sec
	 * thus leading to 46ms per fragment, and a turnaround time of 185ms
	 */
	/* 16 fragments max, 2^10=1024 bytes per fragment */
	audio_fragment = 0x000F000A;
    }
    sample_rate = wwo->format.wf.nSamplesPerSec;
    dsp_stereo = (wwo->format.wf.nChannels > 1) ? 1 : 0;



    /*Set the sample rate*/
    spec[CLIENT_PLAYBACK].rate=sample_rate;

    /*And the size and signedness*/
    spec[CLIENT_PLAYBACK].precision=(wwo->format.wBitsPerSample );
    if (spec[CLIENT_PLAYBACK].precision==16) spec[CLIENT_PLAYBACK].type=TYPE_SIGNED; else spec[CLIENT_PLAYBACK].type=TYPE_UNSIGNED;
    spec[CLIENT_PLAYBACK].channels=(wwo->format.wf.nChannels);
    spec[CLIENT_PLAYBACK].encoding=ENCODE_PCM;
    spec[CLIENT_PLAYBACK].endian=ENDIAN_INTEL;
    spec[CLIENT_PLAYBACK].max_blocks=16;  /*FIXME This is the libaudioio equivalent to fragments, it controls latency*/

	audio = AudioIOOpenX( O_WRONLY|O_NDELAY,&spec[CLIENT_PLAYBACK],&spec[CLIENT_PLAYBACK]);

    if (audio == -1) {
	WARN("can't open sound device %s (%s)!\n", SOUND_DEV, strerror(errno));
	return MMSYSERR_ALLOCATED;
    }
 /*   fcntl(audio, F_SETFD, 1); *//* set close on exec flag */
    wwo->unixdev = audio;


    /* even if we set fragment size above, read it again, just in case */

    wwo->dwFragmentSize = DEFAULT_FRAGMENT_SIZE;

    wwo->msg_toget = 0;
    wwo->msg_tosave = 0;
    wwo->msg_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    memset(wwo->messages, 0, sizeof(WWO_MSG)*WWO_RING_BUFFER_SIZE);
    InitializeCriticalSection(&wwo->msg_crst);
    wwo->msg_crst.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": WINE_WAVEOUT.msg_crst");

    if (!(dwFlags & WAVE_DIRECTSOUND)) {
    	TRACE("Starting wodPlayer Thread\n");
	wwo->hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
	wwo->hThread = CreateThread(NULL, 0, wodPlayer, (LPVOID)(DWORD)wDevID, 0, &(wwo->dwThreadID));
        if (wwo->hThread)
            SetThreadPriority(wwo->hThread, THREAD_PRIORITY_TIME_CRITICAL);
	WaitForSingleObject(wwo->hEvent, INFINITE);
    } else {
	wwo->hEvent = INVALID_HANDLE_VALUE;
	wwo->hThread = INVALID_HANDLE_VALUE;
	wwo->dwThreadID = 0;
    }

    TRACE("fd=%d fragmentSize=%d\n",
	  wwo->unixdev, wwo->dwFragmentSize);
    if (wwo->dwFragmentSize % wwo->format.wf.nBlockAlign)
	ERR("Fragment doesn't contain an integral number of data blocks\n");

    TRACE("wBitsPerSample=%u, nAvgBytesPerSec=%u, nSamplesPerSec=%u, nChannels=%u nBlockAlign=%u!\n",
	  wwo->format.wBitsPerSample, wwo->format.wf.nAvgBytesPerSec,
	  wwo->format.wf.nSamplesPerSec, wwo->format.wf.nChannels,
	  wwo->format.wf.nBlockAlign);

    if (LIBAUDIOIO_NotifyClient(wDevID, WOM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
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

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == -1) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    wwo = &WOutDev[wDevID];
    if (wwo->lpQueuePtr) {
	WARN("buffers still playing !\n");
	ret = WAVERR_STILLPLAYING;
    } else {
	TRACE("imhere[3-close]\n");

	wwo->msg_crst.DebugInfo->Spare[0] = 0;
	DeleteCriticalSection(&wwo->msg_crst);

	if (wwo->hEvent != INVALID_HANDLE_VALUE) {
	    wodPlayer_Message(wwo, WINE_WM_CLOSING, 0);
	    WaitForSingleObject(wwo->hEvent, INFINITE);
	    CloseHandle(wwo->hEvent);
	}
	if (wwo->mapping) {
	    munmap(wwo->mapping, wwo->maplen);
	    wwo->mapping = NULL;
	}

	AudioIOClose();
	wwo->unixdev = -1;
	wwo->dwFragmentSize = 0;
	if (LIBAUDIOIO_NotifyClient(wDevID, WOM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
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
    TRACE("(%u, %p, %08X);\n", wDevID, lpWaveHdr, dwSize);

    /* first, do the sanity checks... */
    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == -1) {
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
    wodPlayer_Message(&WOutDev[wDevID], WINE_WM_HEADER, (DWORD)lpWaveHdr);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodPause				[internal]
 */
static DWORD wodPause(WORD wDevID)
{
    TRACE("(%u);!\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == -1) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    TRACE("imhere[3-PAUSING]\n");
    wodPlayer_Message(&WOutDev[wDevID], WINE_WM_PAUSING, 0);
    WaitForSingleObject(WOutDev[wDevID].hEvent, INFINITE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodRestart				[internal]
 */
static DWORD wodRestart(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == -1) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    if (WOutDev[wDevID].state == WINE_WS_PAUSED) {
	TRACE("imhere[3-RESTARTING]\n");
	wodPlayer_Message(&WOutDev[wDevID], WINE_WM_RESTARTING, 0);
	WaitForSingleObject(WOutDev[wDevID].hEvent, INFINITE);
    }

    /* FIXME: is NotifyClient with WOM_DONE right ? (Comet Busters 1.3.3 needs this notification) */
    /* FIXME: Myst crashes with this ... hmm -MM
       if (LIBAUDIOIO_NotifyClient(wDevID, WOM_DONE, 0L, 0L) != MMSYSERR_NOERROR) {
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

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == -1) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    TRACE("imhere[3-RESET]\n");
    wodPlayer_Message(&WOutDev[wDevID], WINE_WM_RESETTING, 0);
    WaitForSingleObject(WOutDev[wDevID].hEvent, INFINITE);

    return MMSYSERR_NOERROR;
}


/**************************************************************************
 * 				wodGetPosition			[internal]
 */
static DWORD wodGetPosition(WORD wDevID, LPMMTIME lpTime, DWORD uSize)
{
    WINE_WAVEOUT*	wwo;

    TRACE("(%u, %p, %u);\n", wDevID, lpTime, uSize);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].unixdev == -1) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;

    wwo = &WOutDev[wDevID];

    return bytes_to_mmtime(lpTime, wwo->dwPlayedTotal, &wwo->format);
}

/**************************************************************************
 * 				wodGetVolume			[internal]
 */
static DWORD wodGetVolume(WORD wDevID, LPDWORD lpdwVol)
{
    int		vol,bal;
    DWORD	left, right;

    TRACE("(%u, %p);\n", wDevID, lpdwVol);

    if (lpdwVol == NULL)
	return MMSYSERR_NOTENABLED;

     vol=AudioIOGetPlaybackVolume();
     bal=AudioIOGetPlaybackBalance();


     if(bal<0) {
     	left = vol;
     	right=-(vol*(-100+bal)/100);
     }
      else
     {
     	right = vol;
     	left=(vol*(100-bal)/100);
     }

    *lpdwVol = ((left * 0xFFFFl) / 100) + (((right * 0xFFFFl) / 100) << 16);
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 * 				wodSetVolume			[internal]
 */
static DWORD wodSetVolume(WORD wDevID, DWORD dwParam)
{
    int		volume,bal;
    DWORD	left, right;

    TRACE("(%u, %08X);\n", wDevID, dwParam);

    left  = (LOWORD(dwParam) * 100) / 0xFFFFl;
    right = (HIWORD(dwParam) * 100) / 0xFFFFl;
    volume = max(left , right );
    bal=min(left,right);
    bal=bal*100/volume;
    if(right>left) bal=-100+bal; else bal=100-bal;

    AudioIOSetPlaybackVolume(volume);
    AudioIOSetPlaybackBalance(bal);

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
    TRACE("NumDrivers = %d\n",ret);
    return ret;
}

/**************************************************************************
 * 				wodMessage (WINEAUDIOIO.@)
 */
DWORD WINAPI LIBAUDIOIO_wodMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%u, %04X, %08X, %08X, %08X);\n",
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
    case WODM_PREPARE:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_UNPREPARE: 	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETDEVCAPS:	return wodGetDevCaps	(wDevID, (LPWAVEOUTCAPSW)dwParam1,	dwParam2);
    case WODM_GETNUMDEVS:	return wodGetNumDevs	();
    case WODM_GETPITCH:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPITCH:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETVOLUME:	return wodGetVolume	(wDevID, (LPDWORD)dwParam1);
    case WODM_SETVOLUME:	return wodSetVolume	(wDevID, dwParam1);
    case WODM_RESTART:		return wodRestart	(wDevID);
    case WODM_RESET:		return wodReset		(wDevID);

    case DRV_QUERYDSOUNDIFACE:	return wodDsCreate	(wDevID, (PIDSDRIVER*)dwParam1);
    case DRV_QUERYDSOUNDDESC:	return wodDsDesc	(wDevID, (PDSDRIVERDESC)dwParam1);
    default:
	FIXME("unknown message %d!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*======================================================================*
 *                  Low level DSOUND implementation			*
 *		While I have tampered somewhat with this code it is wholely unlikely that it works
 *		Elsewhere the driver returns Not Implemented for DIrectSound
 *		While it may be possible to map the sound device on Solaris
 *		Doing so would bypass the libaudioio library and therefore break any conversions
 *		that the libaudioio sample specification converter is doing
 *		**** All this is untested so far
 *======================================================================*/

typedef struct IDsDriverImpl IDsDriverImpl;
typedef struct IDsDriverBufferImpl IDsDriverBufferImpl;

struct IDsDriverImpl
{
    /* IUnknown fields */
    const IDsDriverVtbl *lpVtbl;
    LONG		ref;
    /* IDsDriverImpl fields */
    UINT		wDevID;
    IDsDriverBufferImpl*primary;
};

struct IDsDriverBufferImpl
{
    /* IUnknown fields */
    const IDsDriverBufferVtbl *lpVtbl;
    LONG		ref;
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
	TRACE("(%p): sound device has been mapped for direct access at %p, size=%d\n", dsdb, wwo->mapping, wwo->maplen);

	/* for some reason, es1371 and sblive! sometimes have junk in here. */
	memset(wwo->mapping,0,wwo->maplen); /* clear it, or we get junk noise */
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
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    FIXME("(): stub!\n");
    return DSERR_UNSUPPORTED;
}

static ULONG WINAPI IDsDriverBufferImpl_AddRef(PIDSDRIVERBUFFER iface)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IDsDriverBufferImpl_Release(PIDSDRIVERBUFFER iface)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (refCount)
	return refCount;
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
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    /* since we (GetDriverDesc flags) have specified DSDDESC_DONTNEEDPRIMARYLOCK,
     * and that we don't support secondary buffers, this method will never be called */
    TRACE("(%p): stub\n",iface);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_Unlock(PIDSDRIVERBUFFER iface,
						 LPVOID pvAudio1,DWORD dwLen1,
						 LPVOID pvAudio2,DWORD dwLen2)
{
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    TRACE("(%p): stub\n",iface);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetFormat(PIDSDRIVERBUFFER iface,
						    LPWAVEFORMATEX pwfx)
{
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */

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
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    TRACE("(%p,%d): stub\n",iface,dwFreq);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetVolumePan(PIDSDRIVERBUFFER iface, PDSVOLUMEPAN pVolPan)
{
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    FIXME("(%p,%p): stub!\n",iface,pVolPan);
    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetPosition(PIDSDRIVERBUFFER iface, DWORD dwNewPos)
{
    /* IDsDriverImpl *This = (IDsDriverImpl *)iface; */
    TRACE("(%p,%d): stub\n",iface,dwNewPos);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_GetPosition(PIDSDRIVERBUFFER iface,
						      LPDWORD lpdwPlay, LPDWORD lpdwWrite)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
#if 0
    count_info info;
#endif

    TRACE("(%p)\n",iface);
    if (WOutDev[This->drv->wDevID].unixdev == -1) {
	ERR("device not open, but accessing?\n");
	return DSERR_UNINITIALIZED;
    }
    /*Libaudioio doesn't support this (Yet anyway)*/
     return DSERR_UNSUPPORTED;

}

static HRESULT WINAPI IDsDriverBufferImpl_Play(PIDSDRIVERBUFFER iface, DWORD dwRes1, DWORD dwRes2, DWORD dwFlags)
{
#if 0
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    int enable = PCM_ENABLE_OUTPUT;
    TRACE("(%p,%lx,%lx,%lx)\n",iface,dwRes1,dwRes2,dwFlags);
    if (ioctl(WOutDev[This->drv->wDevID].unixdev, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
	ERR("ioctl failed (%d)\n", errno);
	return DSERR_GENERIC;
    }
#endif
    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_Stop(PIDSDRIVERBUFFER iface)
{
#if 0
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    int enable = 0;
    TRACE("(%p)\n",iface);
    /* no more playing */
    if (ioctl(WOutDev[This->drv->wDevID].unixdev, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
	ERR("ioctl failed (%d)\n", errno);
	return DSERR_GENERIC;
    }
#endif
#if 0
    /* the play position must be reset to the beginning of the buffer */
    if (ioctl(WOutDev[This->drv->wDevID].unixdev, SNDCTL_DSP_RESET, 0) < 0) {
	ERR("ioctl failed (%d)\n", errno);
	return DSERR_GENERIC;
    }
#endif
    /* Most OSS drivers just can't stop the playback without closing the device...
     * so we need to somehow signal to our DirectSound implementation
     * that it should completely recreate this HW buffer...
     * this unexpected error code should do the trick... */
    return DSERR_BUFFERLOST;
}

static const IDsDriverBufferVtbl dsdbvt =
{
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
    /* IDsDriverImpl *This = (IDsDriverImpl *)iface; */
    FIXME("(%p): stub!\n",iface);
    return DSERR_UNSUPPORTED;
}

static ULONG WINAPI IDsDriverImpl_AddRef(PIDSDRIVER iface)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IDsDriverImpl_Release(PIDSDRIVER iface)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (refCount)
	return refCount;
    HeapFree(GetProcessHeap(),0,This);
    return 0;
}

static HRESULT WINAPI IDsDriverImpl_GetDriverDesc(PIDSDRIVER iface, PDSDRIVERDESC pDesc)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    TRACE("(%p,%p)\n",iface,pDesc);
    pDesc->dwFlags = DSDDESC_DOMMSYSTEMOPEN | DSDDESC_DOMMSYSTEMSETFORMAT |
	DSDDESC_USESYSTEMMEMORY | DSDDESC_DONTNEEDPRIMARYLOCK;
    strcpy(pDesc->szDesc,"Wine AudioIO DirectSound Driver");
    strcpy(pDesc->szDrvname,"wineaudioio.drv");
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
#if 0
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    int enable = 0;

    TRACE("(%p)\n",iface);
    /* make sure the card doesn't start playing before we want it to */
    if (ioctl(WOutDev[This->wDevID].unixdev, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
	ERR("ioctl failed (%d)\n", errno);
	return DSERR_GENERIC;
    }
#endif
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_Close(PIDSDRIVER iface)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    TRACE("(%p)\n",iface);
    if (This->primary) {
	ERR("problem with DirectSound: primary not released\n");
	return DSERR_GENERIC;
    }
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_GetCaps(PIDSDRIVER iface, PDSDRIVERCAPS pCaps)
{
    /* IDsDriverImpl *This = (IDsDriverImpl *)iface; */
    TRACE("(%p,%p)\n",iface,pCaps);
    memset(pCaps, 0, sizeof(*pCaps));
    /* FIXME: need to check actual capabilities */
    pCaps->dwFlags = DSCAPS_PRIMARYMONO | DSCAPS_PRIMARYSTEREO |
	DSCAPS_PRIMARY8BIT | DSCAPS_PRIMARY16BIT;
    pCaps->dwPrimaryBuffers = 1;
    pCaps->dwMinSecondarySampleRate = DSBFREQUENCY_MIN;
    pCaps->dwMaxSecondarySampleRate = DSBFREQUENCY_MAX;
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
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    IDsDriverBufferImpl** ippdsdb = (IDsDriverBufferImpl**)ppvObj;
    HRESULT err;
#if 0
    audio_buf_info info;
    int enable = 0;
#endif

    TRACE("(%p,%p,%x,%x)\n",iface,pwfx,dwFlags,dwCardAddress);
    /* we only support primary buffers */
    if (!(dwFlags & DSBCAPS_PRIMARYBUFFER))
	return DSERR_UNSUPPORTED;
    if (This->primary)
	return DSERR_ALLOCATED;
    if (dwFlags & (DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN))
	return DSERR_CONTROLUNAVAIL;

    *ippdsdb = HeapAlloc(GetProcessHeap(),0,sizeof(IDsDriverBufferImpl));
    if (*ippdsdb == NULL)
	return DSERR_OUTOFMEMORY;
    (*ippdsdb)->lpVtbl  = &dsdbvt;
    (*ippdsdb)->ref	= 1;
    (*ippdsdb)->drv	= This;

    /* check how big the DMA buffer is now */
#if 0
    if (ioctl(WOutDev[This->wDevID].unixdev, SNDCTL_DSP_GETOSPACE, &info) < 0) {
	ERR("ioctl failed (%d)\n", errno);
	HeapFree(GetProcessHeap(),0,*ippdsdb);
	*ippdsdb = NULL;
	return DSERR_GENERIC;
    }
#endif
    WOutDev[This->wDevID].maplen =64*1024; /* Map 64 K at a time */

#if 0
    (*ippdsdb)->buflen = info.fragstotal * info.fragsize;
#endif
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

    /* some drivers need some extra nudging after mapping */
#if 0
    if (ioctl(WOutDev[This->wDevID].unixdev, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
	ERR("ioctl failed (%d)\n", errno);
	return DSERR_GENERIC;
    }
#endif

    This->primary = *ippdsdb;

    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_DuplicateSoundBuffer(PIDSDRIVER iface,
							 PIDSDRIVERBUFFER pBuffer,
							 LPVOID *ppvObj)
{
    /* IDsDriverImpl *This = (IDsDriverImpl *)iface; */
    TRACE("(%p,%p): stub\n",iface,pBuffer);
    return DSERR_INVALIDCALL;
}

static const IDsDriverVtbl dsdvt =
{
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

    *idrv = HeapAlloc(GetProcessHeap(),0,sizeof(IDsDriverImpl));
    if (!*idrv)
	return MMSYSERR_NOMEM;
    (*idrv)->lpVtbl	= &dsdvt;
    (*idrv)->ref	= 1;

    (*idrv)->wDevID	= wDevID;
    (*idrv)->primary	= NULL;
    return MMSYSERR_NOERROR;
}

static DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc)
{
    memset(desc, 0, sizeof(*desc));
    strcpy(desc->szDesc, "Wine LIBAUDIOIO DirectSound Driver");
    strcpy(desc->szDrvname, "wineaudioio.drv");
    return MMSYSERR_NOERROR;
}

/*======================================================================*
 *                  Low level WAVE IN implementation			*
 *======================================================================*/

/**************************************************************************
 * 			widGetDevCaps				[internal]
 */
static DWORD widGetDevCaps(WORD wDevID, LPWAVEINCAPSW lpCaps, DWORD dwSize)
{
    TRACE("(%u, %p, %u);\n", wDevID, lpCaps, dwSize);

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


	int fragments;
	int fragsize;
	int fragstotal;
	int bytes;

	LPVOID		buffer = HeapAlloc(GetProcessHeap(),
		                           HEAP_ZERO_MEMORY,
       		                       wwi->dwFragmentSize);

    BYTE *pOffset = buffer;

    PeekMessageA(&msg, 0, 0, 0, 0);
    wwi->state = WINE_WS_STOPPED;
    wwi->dwTotalRecorded = 0;

    SetEvent(wwi->hEvent);


	/* make sleep time to be # of ms to output a fragment */
    dwSleepTime = (wwi->dwFragmentSize * 1000) / wwi->format.wf.nAvgBytesPerSec;
    TRACE("sleeptime=%d ms\n", dwSleepTime);

    for (; ; ) {
	/* wait for dwSleepTime or an event in thread's queue */
	/* FIXME: could improve wait time depending on queue state,
	 * ie, number of queued fragments
	 */

	if (wwi->lpQueuePtr != NULL && wwi->state == WINE_WS_PLAYING)
        {
            lpWaveHdr = wwi->lpQueuePtr;

	    bytes=fragsize=AudioIORecordingAvailable();
            fragments=fragstotal=1;

            TRACE("info={frag=%d fsize=%d ftotal=%d bytes=%d}\n", fragments, fragsize, fragstotal, bytes);


            /* read all the fragments accumulated so far */
            while ((fragments > 0) && (wwi->lpQueuePtr))
            {
                fragments --;

                if (lpWaveHdr->dwBufferLength - lpWaveHdr->dwBytesRecorded >= wwi->dwFragmentSize)
                {
                    /* directly read fragment in wavehdr */
                    bytesRead = AudioIORead(
		      		     lpWaveHdr->lpData + lpWaveHdr->dwBytesRecorded,
                                     wwi->dwFragmentSize);

                    TRACE("bytesRead=%d (direct)\n", bytesRead);
		    if (bytesRead != (DWORD) -1)
		    {
			/* update number of bytes recorded in current buffer and by this device */
                        lpWaveHdr->dwBytesRecorded += bytesRead;
			wwi->dwTotalRecorded       += bytesRead;

			/* buffer is full. notify client */
			if (lpWaveHdr->dwBytesRecorded == lpWaveHdr->dwBufferLength)
			{
			    /* must copy the value of next waveHdr, because we have no idea of what
			     * will be done with the content of lpWaveHdr in callback
			     */
			    LPWAVEHDR	lpNext = lpWaveHdr->lpNext;

			    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
			    lpWaveHdr->dwFlags |=  WHDR_DONE;

			    if (LIBAUDIOIO_NotifyClient(uDevID, WIM_DATA,
						 (DWORD)lpWaveHdr, 0) != MMSYSERR_NOERROR)
			    {
				WARN("can't notify client !\n");
			    }
			    lpWaveHdr = wwi->lpQueuePtr = lpNext;
			}
                    }
                }
                else
		{
                    /* read the fragment in a local buffer */
                    bytesRead = AudioIORead( buffer, wwi->dwFragmentSize);
                    pOffset = buffer;

                    TRACE("bytesRead=%d (local)\n", bytesRead);

                    /* copy data in client buffers */
                    while (bytesRead != (DWORD) -1 && bytesRead > 0)
                    {
                        DWORD dwToCopy = min (bytesRead, lpWaveHdr->dwBufferLength - lpWaveHdr->dwBytesRecorded);

                        memcpy(lpWaveHdr->lpData + lpWaveHdr->dwBytesRecorded,
                               pOffset,
                               dwToCopy);

                        /* update number of bytes recorded in current buffer and by this device */
                        lpWaveHdr->dwBytesRecorded += dwToCopy;
                        wwi->dwTotalRecorded += dwToCopy;
                        bytesRead -= dwToCopy;
                        pOffset   += dwToCopy;

                        /* client buffer is full. notify client */
                        if (lpWaveHdr->dwBytesRecorded == lpWaveHdr->dwBufferLength)
                        {
			    /* must copy the value of next waveHdr, because we have no idea of what
			     * will be done with the content of lpWaveHdr in callback
			     */
			    LPWAVEHDR	lpNext = lpWaveHdr->lpNext;
			    TRACE("lpNext=%p\n", lpNext);

                            lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
                            lpWaveHdr->dwFlags |=  WHDR_DONE;

                            if (LIBAUDIOIO_NotifyClient(uDevID, WIM_DATA,
                                                 (DWORD)lpWaveHdr, 0) != MMSYSERR_NOERROR)
                            {
                                WARN("can't notify client !\n");
                            }

			    wwi->lpQueuePtr = lpWaveHdr = lpNext;
			    if (!lpNext && bytesRead) {
                                /* no more buffer to copy data to, but we did read more.
                                 * what hasn't been copied will be dropped
                                 */
                                WARN("buffer under run! %u bytes dropped.\n", bytesRead);
                                wwi->lpQueuePtr = NULL;
                                break;
                            }
                        }
                    }
                }
            }
	}

	MsgWaitForMultipleObjects(0, NULL, FALSE, dwSleepTime, QS_POSTMESSAGE);

	while (PeekMessageA(&msg, 0, WINE_WM_FIRST, WINE_WM_LAST, PM_REMOVE)) {

            TRACE("msg=0x%x wParam=0x%x lParam=0x%lx\n", msg.message, msg.wParam, msg.lParam);
	    switch (msg.message) {
	    case WINE_WM_PAUSING:
		wwi->state = WINE_WS_PAUSED;

                AudioIORecordingPause();
		SetEvent(wwi->hEvent);
		break;
	    case WINE_WM_RESTARTING:
            {

		wwi->state = WINE_WS_PLAYING;

                if (wwi->bTriggerSupport)
                {
                    /* start the recording */
                    AudioIORecordingResume();
                }
                else
                {
                    unsigned char data[4];
                    /* read 4 bytes to start the recording */
                    AudioIORead( data, 4);
                }

		SetEvent(wwi->hEvent);
		break;
            }
	    case WINE_WM_HEADER:
		lpWaveHdr = (LPWAVEHDR)msg.lParam;
		lpWaveHdr->lpNext = 0;

		/* insert buffer at the end of queue */
		{
		    LPWAVEHDR*	wh;
		    for (wh = &(wwi->lpQueuePtr); *wh; wh = &((*wh)->lpNext));
		    *wh = lpWaveHdr;
		}
		break;
	    case WINE_WM_RESETTING:
		wwi->state = WINE_WS_STOPPED;
		/* return all buffers to the app */
		for (lpWaveHdr = wwi->lpQueuePtr; lpWaveHdr; lpWaveHdr = lpWaveHdr->lpNext) {
		    TRACE("reset %p %p\n", lpWaveHdr, lpWaveHdr->lpNext);
		    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
		    lpWaveHdr->dwFlags |= WHDR_DONE;

		    if (LIBAUDIOIO_NotifyClient(uDevID, WIM_DATA,
					 (DWORD)lpWaveHdr, 0) != MMSYSERR_NOERROR) {
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
		HeapFree(GetProcessHeap(), 0, buffer);
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
    int 		audio;
    int			fragment_size;
    int			sample_rate;
    int			format;
    int			dsp_stereo;
    WINE_WAVEIN*	wwi;
    int			audio_fragment;

    TRACE("(%u, %p, %08X);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN("Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_WAVEINDRV) return MMSYSERR_BADDEVICEID;

    /* only PCM format is supported so far... */
    if (lpDesc->lpFormat->wFormatTag != WAVE_FORMAT_PCM ||
	lpDesc->lpFormat->nChannels == 0 ||
	lpDesc->lpFormat->nSamplesPerSec == 0 ||
        (lpDesc->lpFormat->wBitsPerSample!=8 && lpDesc->lpFormat->wBitsPerSample!=16)) {
	WARN("Bad format: tag=%04X nChannels=%d nSamplesPerSec=%d !\n",
	     lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
	     lpDesc->lpFormat->nSamplesPerSec);
	return WAVERR_BADFORMAT;
    }

    if (dwFlags & WAVE_FORMAT_QUERY) {
	TRACE("Query format: tag=%04X nChannels=%d nSamplesPerSec=%d !\n",
	     lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
	     lpDesc->lpFormat->nSamplesPerSec);
	return MMSYSERR_NOERROR;
    }

    if (access(SOUND_DEV,0) != 0) return MMSYSERR_NOTENABLED;
    audio = AudioIOOpenX( O_RDONLY|O_NDELAY, &spec[CLIENT_RECORD],&spec[CLIENT_RECORD]);
    if (audio == -1) {
	WARN("can't open sound device %s (%s)!\n", SOUND_DEV, strerror(errno));
	return MMSYSERR_ALLOCATED;
    }
    fcntl(audio, F_SETFD, 1); /* set close on exec flag */

    wwi = &WInDev[wDevID];
    if (wwi->lpQueuePtr) {
	WARN("Should have an empty queue (%p)\n", wwi->lpQueuePtr);
	wwi->lpQueuePtr = NULL;
    }
    wwi->unixdev = audio;
    wwi->dwTotalRecorded = 0;
    wwi->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    wwi->waveDesc = *lpDesc;
    memcpy(&wwi->format,   lpDesc->lpFormat, sizeof(PCMWAVEFORMAT));

    if (wwi->format.wBitsPerSample == 0) {
	WARN("Resetting zeroed wBitsPerSample\n");
	wwi->format.wBitsPerSample = 8 *
	    (wwi->format.wf.nAvgBytesPerSec /
	     wwi->format.wf.nSamplesPerSec) /
	    wwi->format.wf.nChannels;
    }

    spec[CLIENT_RECORD].rate=sample_rate = wwi->format.wf.nSamplesPerSec;
    dsp_stereo = ((spec[CLIENT_RECORD].channels=wwi->format.wf.nChannels) > 1) ? TRUE : FALSE;
    spec[CLIENT_RECORD].precision= wwi->format.wBitsPerSample;
    spec[CLIENT_RECORD].type=(spec[CLIENT_RECORD].precision==16)?TYPE_SIGNED:TYPE_UNSIGNED;

    /* This is actually hand tuned to work so that my SB Live:
     * - does not skip
     * - does not buffer too much
     * when sending with the Shoutcast winamp plugin
     */
    /* 7 fragments max, 2^10 = 1024 bytes per fragment */
    audio_fragment = 0x0007000A;
    fragment_size=4096;
    if (fragment_size == -1) {
	AudioIOClose();
	wwi->unixdev = -1;
	return MMSYSERR_NOTENABLED;
    }
    wwi->dwFragmentSize = fragment_size;

    TRACE("wBitsPerSample=%u, nAvgBytesPerSec=%u, nSamplesPerSec=%u, nChannels=%u nBlockAlign=%u!\n",
	  wwi->format.wBitsPerSample, wwi->format.wf.nAvgBytesPerSec,
	  wwi->format.wf.nSamplesPerSec, wwi->format.wf.nChannels,
	  wwi->format.wf.nBlockAlign);

    wwi->hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    wwi->hThread = CreateThread(NULL, 0, widRecorder, (LPVOID)(DWORD)wDevID, 0, &(wwi->dwThreadID));
    if (wwi->hThread)
        SetThreadPriority(wwi->hThread, THREAD_PRIORITY_TIME_CRITICAL);
    WaitForSingleObject(wwi->hEvent, INFINITE);

   if (LIBAUDIOIO_NotifyClient(wDevID, WIM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
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
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].unixdev == -1) {
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
    AudioIOClose();
    wwi->unixdev = -1;
    wwi->dwFragmentSize = 0;
    if (LIBAUDIOIO_NotifyClient(wDevID, WIM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
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
    TRACE("(%u, %p, %08X);\n", wDevID, lpWaveHdr, dwSize);

    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].unixdev == -1) {
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
 * 			widStart				[internal]
 */
static DWORD widStart(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].unixdev == -1) {
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
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].unixdev == -1) {
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
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].unixdev == -1) {
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
    WINE_WAVEIN*	wwi;

    TRACE("(%u, %p, %u);\n", wDevID, lpTime, uSize);

    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].unixdev == -1) {
	WARN("can't get pos !\n");
	return MMSYSERR_INVALHANDLE;
    }
    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;

    wwi = &WInDev[wDevID];

    return bytes_to_mmtime(lpTime, wwi->dwTotalRecorded, &wwi->format);
}

/**************************************************************************
 * 				widMessage (WINEAUDIOIO.@)
 */
DWORD WINAPI LIBAUDIOIO_widMessage(WORD wDevID, WORD wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%u, %04X, %08X, %08X, %08X);\n",
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
    case WIDM_PREPARE:		return MMSYSERR_NOTSUPPORTED;
    case WIDM_UNPREPARE:	return MMSYSERR_NOTSUPPORTED;
    case WIDM_GETDEVCAPS:	return widGetDevCaps (wDevID, (LPWAVEINCAPSW)dwParam1, dwParam2);
    case WIDM_GETNUMDEVS:	return wodGetNumDevs ();	/* same number of devices in output as in input */
    case WIDM_GETPOS:		return widGetPosition(wDevID, (LPMMTIME)dwParam1, dwParam2);
    case WIDM_RESET:		return widReset      (wDevID);
    case WIDM_START:		return widStart      (wDevID);
    case WIDM_STOP:		return widStop       (wDevID);
    case DRV_QUERYDSOUNDIFACE:	return widDsCreate   (wDevID, (PIDSCDRIVER*)dwParam1);
    case DRV_QUERYDSOUNDDESC:	return widDsDesc     (wDevID, (PDSDRIVERDESC)dwParam1);
    default:
	FIXME("unknown message %u!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*======================================================================*
 *                  Low level DSOUND capture implementation		*
 *======================================================================*/
static DWORD widDsCreate(UINT wDevID, PIDSCDRIVER* drv)
{
    /* we can't perform memory mapping as we don't have a file stream
	interface with arts like we do with oss */
    MESSAGE("This sound card's driver does not support direct access\n");
    MESSAGE("The (slower) DirectSound HEL mode will be used instead.\n");
    return MMSYSERR_NOTSUPPORTED;
}

static DWORD widDsDesc(UINT wDevID, PDSDRIVERDESC desc)
{
    memset(desc, 0, sizeof(*desc));
    strcpy(desc->szDesc, "Wine LIBAUDIOIO DirectSound Driver");
    strcpy(desc->szDrvname, "wineaudioio.drv");
    return MMSYSERR_NOERROR;
}

#else /* HAVE_LIBAUDIOIO */

/**************************************************************************
 * 				wodMessage (WINEAUDIOIO.@)
 */
DWORD WINAPI LIBAUDIOIO_wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08X, %08X, %08X):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				widMessage (WINEAUDIOIO.@)
 */
DWORD WINAPI LIBAUDIOIO_widMessage(WORD wDevID, WORD wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08X, %08X, %08X):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

#endif /* HAVE_LIBAUDIOIO */
