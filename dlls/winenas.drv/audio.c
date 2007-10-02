/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Wine Driver for NAS Network Audio System
 *   http://radscan.com/nas.html
 *
 * Copyright 1994 Martin Ayotte
 *           1999 Eric Pouech (async playing in waveOut/waveIn)
 *	     2000 Eric Pouech (loops in waveOut)
 *	     2002 Chris Morgan (aRts version of this file)
 *           2002 Nicolas Escuder (NAS version of this file)
 *
 * Copyright 2002 Nicolas Escuder <n.escuder@alineanet.com>
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
 */
/* NOTE:
 *    with nas we cannot stop the audio that is already in
 *    the servers buffer.
 *
 * FIXME:
 *	pause in waveOut does not work correctly in loop mode
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <fcntl.h>
#include <math.h>

#define FRAG_SIZE  1024
#define FRAG_COUNT 10

/* avoid type conflicts */
#define INT8 X_INT8
#define INT16 X_INT16
#define INT32 X_INT32
#define INT64 X_INT64
#define BOOL X_BOOL
#define BYTE X_BYTE
#ifdef HAVE_AUDIO_AUDIOLIB_H
#include <audio/audiolib.h>
#endif
#ifdef HAVE_AUDIO_SOUNDLIB_H
#include <audio/soundlib.h>
#endif
#undef INT8
#undef INT16
#undef INT32
#undef INT64
#undef LONG64
#undef BOOL
#undef BYTE

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmddk.h"
#include "dsound.h"
#include "dsdriver.h"
#include "nas.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wave);

/* Allow 1% deviation for sample rates (some ES137x cards) */
#define NEAR_MATCH(rate1,rate2) (((100*((int)(rate1)-(int)(rate2)))/(rate1))==0)

#ifdef HAVE_NAS

static AuServer         *AuServ;

#define MAX_WAVEOUTDRV 	(1)

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
enum win_wm_message {
    WINE_WM_PAUSING = WM_USER + 1, WINE_WM_RESTARTING, WINE_WM_RESETTING, WINE_WM_HEADER,
    WINE_WM_UPDATE, WINE_WM_BREAKLOOP, WINE_WM_CLOSING
};

typedef struct {
    enum win_wm_message 	msg;	/* message identifier */
    DWORD	                param;  /* parameter for this message */
    HANDLE	                hEvent;	/* if message is synchronous, handle of event for synchro */
} RING_MSG;

/* implement an in-process message ring for better performance
 * (compared to passing thru the server)
 * this ring will be used by the input (resp output) record (resp playback) routine
 */
#define NAS_RING_BUFFER_INCREMENT      64
typedef struct {
    RING_MSG			* messages;
    int                         ring_buffer_size;
    int				msg_tosave;
    int				msg_toget;
    HANDLE			msg_event;
    CRITICAL_SECTION		msg_crst;
} MSG_RING;

typedef struct {
    volatile int		state;			/* one of the WINE_WS_ manifest constants */
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    PCMWAVEFORMAT		format;
    WAVEOUTCAPSW		caps;
    int				Id;

    int                         open;
    AuServer                    *AuServ;
    AuDeviceID                  AuDev;
    AuFlowID                    AuFlow;
    BOOL                        FlowStarted;

    DWORD                       writeBytes;
    DWORD                       freeBytes;
    DWORD                       sendBytes;

    DWORD                       BufferSize;           /* size of whole buffer in bytes */

    char*                       SoundBuffer;
    long                        BufferUsed;

    DWORD			volume_left;		/* volume control information */
    DWORD			volume_right;

    LPWAVEHDR			lpQueuePtr;		/* start of queued WAVEHDRs (waiting to be notified) */
    LPWAVEHDR			lpPlayPtr;		/* start of not yet fully played buffers */

    LPWAVEHDR			lpLoopPtr;              /* pointer of first buffer in loop, if any */
    DWORD			dwLoops;		/* private copy of loop counter */

    DWORD			PlayedTotal;		/* number of bytes actually played since opening */
    DWORD                       WrittenTotal;         /* number of bytes written to the audio device since opening */

    /* synchronization stuff */
    HANDLE			hStartUpEvent;
    HANDLE			hThread;
    DWORD			dwThreadID;
    MSG_RING			msgRing;
} WINE_WAVEOUT;

static WINE_WAVEOUT	WOutDev   [MAX_WAVEOUTDRV];

static DWORD wodDsCreate(UINT wDevID, PIDSDRIVER* drv);
static DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc);


/* NASFUNC */
static AuBool event_handler(AuServer* aud, AuEvent* ev, AuEventHandlerRec* hnd);
static int nas_init(void);
static int nas_end(void);

static int nas_finddev(WINE_WAVEOUT* wwo);
static int nas_open(WINE_WAVEOUT* wwo);
static int nas_free(WINE_WAVEOUT* wwo);
static int nas_close(WINE_WAVEOUT* wwo);
static void buffer_resize(WINE_WAVEOUT* wwo, int len);
static int nas_add_buffer(WINE_WAVEOUT* wwo);
static int nas_send_buffer(WINE_WAVEOUT* wwo);

/* These strings used only for tracing */
static const char * const wodPlayerCmdString[] = {
    "WINE_WM_PAUSING",
    "WINE_WM_RESTARTING",
    "WINE_WM_RESETTING",
    "WINE_WM_HEADER",
    "WINE_WM_UPDATE",
    "WINE_WM_BREAKLOOP",
    "WINE_WM_CLOSING",
};

static const char * const nas_elementnotify_kinds[] = {
        "LowWater",
        "HighWater",
        "State",
        "Unknown"
};

static const char * const nas_states[] = {
        "Stop",
        "Start",
        "Pause",
        "Any"
};

static const char * const nas_reasons[] = {
        "User",
        "Underrun",
        "Overrun",
        "EOF",
        "Watermark",
        "Hardware",
        "Any"
};

static const char* nas_reason(unsigned int reason)
{
        if (reason > 6) reason = 6;
        return nas_reasons[reason];
}

static const char* nas_elementnotify_kind(unsigned int kind)
{
        if (kind > 2) kind = 3;
        return nas_elementnotify_kinds[kind];
}


#if 0
static const char* nas_event_type(unsigned int type)
{
        static const char * const nas_event_types[] =
        {
            "Undefined",
            "Undefined",
            "ElementNotify",
            "GrabNotify",
            "MonitorNotify",
            "BucketNotify",
            "DeviceNotify"
        };

        if (type > 6) type = 0;
        return nas_event_types[type];
}
#endif


static const char* nas_state(unsigned int state)
{
        if (state > 3) state = 3;
        return nas_states[state];
}

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
        WARN("Format %d not supported, using TIME_BYTES !\n", lpTime->wType);
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
#if 0
/* Volume functions derived from Alsaplayer source */
/* length is the number of 16 bit samples */
static void volume_effect16(void *bufin, void* bufout, int length, int left,
                            int right, int nChannels)
{
  short *d_out = (short *)bufout;
  short *d_in = (short *)bufin;
  int i, v;

/*
  TRACE("length == %d, nChannels == %d\n", length, nChannels);
*/

  if (right == -1) right = left;

  for(i = 0; i < length; i+=(nChannels))
  {
    v = (int) ((*(d_in++) * left) / 100);
    *(d_out++) = (v>32767) ? 32767 : ((v<-32768) ? -32768 : v);
    if(nChannels == 2)
    {
      v = (int) ((*(d_in++) * right) / 100);
      *(d_out++) = (v>32767) ? 32767 : ((v<-32768) ? -32768 : v);
    }
  }
}

/* length is the number of 8 bit samples */
static void volume_effect8(void *bufin, void* bufout, int length, int left,
                           int right, int nChannels)
{
  char *d_out = (char *)bufout;
  char *d_in = (char *)bufin;
  int i, v;

/*
  TRACE("length == %d, nChannels == %d\n", length, nChannels);
*/

  if (right == -1) right = left;

  for(i = 0; i < length; i+=(nChannels))
  {
    v = (char) ((*(d_in++) * left) / 100);
    *(d_out++) = (v>255) ? 255 : ((v<0) ? 0 : v);
    if(nChannels == 2)
    {
      v = (char) ((*(d_in++) * right) / 100);
      *(d_out++) = (v>255) ? 255 : ((v<0) ? 0 : v);
    }
  }
}
#endif

/******************************************************************
 *		NAS_CloseDevice
 *
 */
static void NAS_CloseDevice(WINE_WAVEOUT* wwo)
{
  TRACE("NAS_CloseDevice\n");
  nas_close(wwo);
}

/******************************************************************
 *		NAS_WaveClose
 */
LONG		NAS_WaveClose(void)
{
    nas_end();    /* free up nas server */
    return 1;
}

/******************************************************************
 *		NAS_WaveInit
 *
 * Initialize internal structures from NAS server info
 */
LONG NAS_WaveInit(void)
{
    int 	i;
    if (!nas_init()) return MMSYSERR_ERROR;

    /* initialize all device handles to -1 */
    for (i = 0; i < MAX_WAVEOUTDRV; ++i)
    {
        static const WCHAR ini[] = {'N','A','S',' ','W','A','V','E','O','U','T',' ','D','r','i','v','e','r',0};
	memset(&WOutDev[i].caps, 0, sizeof(WOutDev[i].caps)); /* zero out caps values */

        WOutDev[i].AuServ = AuServ;
        WOutDev[i].AuDev = AuNone;
	WOutDev[i].Id = i;
    	WOutDev[i].caps.wMid = 0x00FF; 	/* Manufac ID */
    	WOutDev[i].caps.wPid = 0x0001; 	/* Product ID */
        strcpyW(WOutDev[i].caps.szPname, ini);
        WOutDev[i].AuFlow = 0;
    	WOutDev[i].caps.vDriverVersion = 0x0100;
    	WOutDev[i].caps.dwFormats = 0x00000000;
    	WOutDev[i].caps.dwSupport = WAVECAPS_VOLUME;

    	WOutDev[i].caps.wChannels = 2;
    	WOutDev[i].caps.dwSupport |= WAVECAPS_LRVOLUME;

    	WOutDev[i].caps.dwFormats |= WAVE_FORMAT_4M08;
    	WOutDev[i].caps.dwFormats |= WAVE_FORMAT_4S08;
    	WOutDev[i].caps.dwFormats |= WAVE_FORMAT_4S16;
    	WOutDev[i].caps.dwFormats |= WAVE_FORMAT_4M16;
    	WOutDev[i].caps.dwFormats |= WAVE_FORMAT_2M08;
    	WOutDev[i].caps.dwFormats |= WAVE_FORMAT_2S08;
    	WOutDev[i].caps.dwFormats |= WAVE_FORMAT_2M16;
    	WOutDev[i].caps.dwFormats |= WAVE_FORMAT_2S16;
    	WOutDev[i].caps.dwFormats |= WAVE_FORMAT_1M08;
    	WOutDev[i].caps.dwFormats |= WAVE_FORMAT_1S08;
	WOutDev[i].caps.dwFormats |= WAVE_FORMAT_1M16;
	WOutDev[i].caps.dwFormats |= WAVE_FORMAT_1S16;
    }


    return 0;
}

/******************************************************************
 *		NAS_InitRingMessage
 *
 * Initialize the ring of messages for passing between driver's caller and playback/record
 * thread
 */
static int NAS_InitRingMessage(MSG_RING* mr)
{
    mr->msg_toget = 0;
    mr->msg_tosave = 0;
    mr->msg_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    mr->ring_buffer_size = NAS_RING_BUFFER_INCREMENT;
    mr->messages = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,mr->ring_buffer_size * sizeof(RING_MSG));
    InitializeCriticalSection(&mr->msg_crst);
    mr->msg_crst.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": MSG_RING.msg_crst");
    return 0;
}

/******************************************************************
 *		NAS_DestroyRingMessage
 *
 */
static int NAS_DestroyRingMessage(MSG_RING* mr)
{
    CloseHandle(mr->msg_event);
    HeapFree(GetProcessHeap(),0,mr->messages);
    mr->msg_crst.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&mr->msg_crst);
    return 0;
}

/******************************************************************
 *		NAS_AddRingMessage
 *
 * Inserts a new message into the ring (should be called from DriverProc derivated routines)
 */
static int NAS_AddRingMessage(MSG_RING* mr, enum win_wm_message msg, DWORD param, BOOL wait)
{
    HANDLE      hEvent = INVALID_HANDLE_VALUE;

    EnterCriticalSection(&mr->msg_crst);
    if ((mr->msg_toget == ((mr->msg_tosave + 1) % mr->ring_buffer_size)))
    {
	int old_ring_buffer_size = mr->ring_buffer_size;
	mr->ring_buffer_size += NAS_RING_BUFFER_INCREMENT;
	TRACE("omr->ring_buffer_size=%d\n",mr->ring_buffer_size);
	mr->messages = HeapReAlloc(GetProcessHeap(),0,mr->messages, mr->ring_buffer_size * sizeof(RING_MSG));
	/* Now we need to rearrange the ring buffer so that the new
	   buffers just allocated are in between mr->msg_tosave and
	   mr->msg_toget.
	*/
	if (mr->msg_tosave < mr->msg_toget)
	{
	    memmove(&(mr->messages[mr->msg_toget + NAS_RING_BUFFER_INCREMENT]),
		    &(mr->messages[mr->msg_toget]),
		    sizeof(RING_MSG)*(old_ring_buffer_size - mr->msg_toget)
		    );
	    mr->msg_toget += NAS_RING_BUFFER_INCREMENT;
	}
    }
    if (wait)
    {
        hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (hEvent == INVALID_HANDLE_VALUE)
        {
            ERR("can't create event !?\n");
            LeaveCriticalSection(&mr->msg_crst);
            return 0;
        }
        if (mr->msg_toget != mr->msg_tosave && mr->messages[mr->msg_toget].msg != WINE_WM_HEADER)
            FIXME("two fast messages in the queue!!!!\n");

        /* fast messages have to be added at the start of the queue */
        mr->msg_toget = (mr->msg_toget + mr->ring_buffer_size - 1) % mr->ring_buffer_size;

        mr->messages[mr->msg_toget].msg = msg;
        mr->messages[mr->msg_toget].param = param;
        mr->messages[mr->msg_toget].hEvent = hEvent;
    }
    else
    {
        mr->messages[mr->msg_tosave].msg = msg;
        mr->messages[mr->msg_tosave].param = param;
        mr->messages[mr->msg_tosave].hEvent = INVALID_HANDLE_VALUE;
        mr->msg_tosave = (mr->msg_tosave + 1) % mr->ring_buffer_size;
    }

    LeaveCriticalSection(&mr->msg_crst);

    SetEvent(mr->msg_event);    /* signal a new message */

    if (wait)
    {
        /* wait for playback/record thread to have processed the message */
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
    }

    return 1;
}

/******************************************************************
 *		NAS_RetrieveRingMessage
 *
 * Get a message from the ring. Should be called by the playback/record thread.
 */
static int NAS_RetrieveRingMessage(MSG_RING* mr,
                                   enum win_wm_message *msg, DWORD *param, HANDLE *hEvent)
{
    EnterCriticalSection(&mr->msg_crst);

    if (mr->msg_toget == mr->msg_tosave) /* buffer empty ? */
    {
        LeaveCriticalSection(&mr->msg_crst);
	return 0;
    }

    *msg = mr->messages[mr->msg_toget].msg;
    mr->messages[mr->msg_toget].msg = 0;
    *param = mr->messages[mr->msg_toget].param;
    *hEvent = mr->messages[mr->msg_toget].hEvent;
    mr->msg_toget = (mr->msg_toget + 1) % mr->ring_buffer_size;
    LeaveCriticalSection(&mr->msg_crst);
    return 1;
}

/*======================================================================*
 *                  Low level WAVE OUT implementation			*
 *======================================================================*/

/**************************************************************************
 * 			wodNotifyClient			[internal]
 */
static DWORD wodNotifyClient(WINE_WAVEOUT* wwo, WORD wMsg, DWORD dwParam1, DWORD dwParam2)
{
    TRACE("wMsg = 0x%04x dwParm1 = %04X dwParam2 = %04X\n", wMsg, dwParam1, dwParam2);

    switch (wMsg) {
    case WOM_OPEN:
    case WOM_CLOSE:
    case WOM_DONE:
	if (wwo->wFlags != DCB_NULL &&
	    !DriverCallback(wwo->waveDesc.dwCallback, wwo->wFlags, (HDRVR)wwo->waveDesc.hWave,
			    wMsg, wwo->waveDesc.dwInstance, dwParam1, dwParam2)) {
	    WARN("can't notify client !\n");
	    return MMSYSERR_ERROR;
	}
	break;
    default:
	FIXME("Unknown callback message %u\n", wMsg);
        return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodUpdatePlayedTotal	[internal]
 *
 */
static BOOL wodUpdatePlayedTotal(WINE_WAVEOUT* wwo)
{
    wwo->PlayedTotal = wwo->WrittenTotal;
    return TRUE;
}

/**************************************************************************
 * 				wodPlayer_BeginWaveHdr          [internal]
 *
 * Makes the specified lpWaveHdr the currently playing wave header.
 * If the specified wave header is a begin loop and we're not already in
 * a loop, setup the loop.
 */
static void wodPlayer_BeginWaveHdr(WINE_WAVEOUT* wwo, LPWAVEHDR lpWaveHdr)
{
    wwo->lpPlayPtr = lpWaveHdr;

    if (!lpWaveHdr) return;

    if (lpWaveHdr->dwFlags & WHDR_BEGINLOOP) {
	if (wwo->lpLoopPtr) {
	    WARN("Already in a loop. Discarding loop on this header (%p)\n", lpWaveHdr);
	    TRACE("Already in a loop. Discarding loop on this header (%p)\n", lpWaveHdr);
	} else {
            TRACE("Starting loop (%dx) with %p\n", lpWaveHdr->dwLoops, lpWaveHdr);
	    wwo->lpLoopPtr = lpWaveHdr;
	    /* Windows does not touch WAVEHDR.dwLoops,
	     * so we need to make an internal copy */
	    wwo->dwLoops = lpWaveHdr->dwLoops;
	}
    }
}

/**************************************************************************
 * 				wodPlayer_PlayPtrNext	        [internal]
 *
 * Advance the play pointer to the next waveheader, looping if required.
 */
static LPWAVEHDR wodPlayer_PlayPtrNext(WINE_WAVEOUT* wwo)
{
    LPWAVEHDR lpWaveHdr = wwo->lpPlayPtr;

    if ((lpWaveHdr->dwFlags & WHDR_ENDLOOP) && wwo->lpLoopPtr) {
	/* We're at the end of a loop, loop if required */
	if (--wwo->dwLoops > 0) {
	    wwo->lpPlayPtr = wwo->lpLoopPtr;
	} else {
	    /* Handle overlapping loops correctly */
	    if (wwo->lpLoopPtr != lpWaveHdr && (lpWaveHdr->dwFlags & WHDR_BEGINLOOP)) {
		FIXME("Correctly handled case ? (ending loop buffer also starts a new loop)\n");
		/* shall we consider the END flag for the closing loop or for
		 * the opening one or for both ???
		 * code assumes for closing loop only
		 */
	    } else {
                lpWaveHdr = lpWaveHdr->lpNext;
            }
            wwo->lpLoopPtr = NULL;
            wodPlayer_BeginWaveHdr(wwo, lpWaveHdr);
	}
    } else {
	/* We're not in a loop.  Advance to the next wave header */
	wodPlayer_BeginWaveHdr(wwo, lpWaveHdr = lpWaveHdr->lpNext);
    }
    return lpWaveHdr;
}

/**************************************************************************
 * 				wodPlayer_NotifyCompletions	[internal]
 *
 * Notifies and remove from queue all wavehdrs which have been played to
 * the speaker (ie. they have cleared the audio device).  If force is true,
 * we notify all wavehdrs and remove them all from the queue even if they
 * are unplayed or part of a loop.
 */
static DWORD wodPlayer_NotifyCompletions(WINE_WAVEOUT* wwo, BOOL force)
{
    LPWAVEHDR		lpWaveHdr;

    /* Start from lpQueuePtr and keep notifying until:
     * - we hit an unwritten wavehdr
     * - we hit the beginning of a running loop
     * - we hit a wavehdr which hasn't finished playing
     */
    wodUpdatePlayedTotal(wwo);

    while ((lpWaveHdr = wwo->lpQueuePtr) && (force || (lpWaveHdr != wwo->lpPlayPtr &&
            lpWaveHdr != wwo->lpLoopPtr && lpWaveHdr->reserved <= wwo->PlayedTotal))) {

	wwo->lpQueuePtr = lpWaveHdr->lpNext;

	lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
	lpWaveHdr->dwFlags |= WHDR_DONE;

	wodNotifyClient(wwo, WOM_DONE, (DWORD)lpWaveHdr, 0);
    }
    return  (lpWaveHdr && lpWaveHdr != wwo->lpPlayPtr && lpWaveHdr != wwo->lpLoopPtr) ?
            1 : 1;
}

/**************************************************************************
 * 				wodPlayer_Reset			[internal]
 *
 * wodPlayer helper. Resets current output stream.
 */
static	void	wodPlayer_Reset(WINE_WAVEOUT* wwo, BOOL reset)
{
    wodUpdatePlayedTotal(wwo);
    wodPlayer_NotifyCompletions(wwo, FALSE); /* updates current notify list */

    /* we aren't able to flush any data that has already been written */
    /* to nas, otherwise we would do the flushing here */

    nas_free(wwo);

    if (reset) {
        enum win_wm_message     msg;
        DWORD                   param;
        HANDLE                  ev;

	/* remove any buffer */
	wodPlayer_NotifyCompletions(wwo, TRUE);

	wwo->lpPlayPtr = wwo->lpQueuePtr = wwo->lpLoopPtr = NULL;
	wwo->state = WINE_WS_STOPPED;
	wwo->PlayedTotal = wwo->WrittenTotal = 0;

        /* remove any existing message in the ring */
        EnterCriticalSection(&wwo->msgRing.msg_crst);

        /* return all pending headers in queue */
        while (NAS_RetrieveRingMessage(&wwo->msgRing, &msg, &param, &ev))
        {
	    TRACE("flushing msg\n");
            if (msg != WINE_WM_HEADER)
            {
                FIXME("shouldn't have headers left\n");
                SetEvent(ev);
                continue;
            }
            ((LPWAVEHDR)param)->dwFlags &= ~WHDR_INQUEUE;
            ((LPWAVEHDR)param)->dwFlags |= WHDR_DONE;

            wodNotifyClient(wwo, WOM_DONE, param, 0);
        }
        ResetEvent(wwo->msgRing.msg_event);
        LeaveCriticalSection(&wwo->msgRing.msg_crst);
    } else {
        if (wwo->lpLoopPtr) {
            /* complicated case, not handled yet (could imply modifying the loop counter */
            FIXME("Pausing while in loop isn't correctly handled yet, except strange results\n");
            wwo->lpPlayPtr = wwo->lpLoopPtr;
            wwo->WrittenTotal = wwo->PlayedTotal; /* this is wrong !!! */
        } else {
	    /* the data already written is going to be played, so take */
	    /* this fact into account here */
	    wwo->PlayedTotal = wwo->WrittenTotal;
        }
	wwo->state = WINE_WS_PAUSED;
    }
}

/**************************************************************************
 * 		      wodPlayer_ProcessMessages			[internal]
 */
static void wodPlayer_ProcessMessages(WINE_WAVEOUT* wwo)
{
    LPWAVEHDR           lpWaveHdr;
    enum win_wm_message	msg;
    DWORD		param;
    HANDLE		ev;

    while (NAS_RetrieveRingMessage(&wwo->msgRing, &msg, &param, &ev)) {
        TRACE("Received %s %x\n", wodPlayerCmdString[msg - WM_USER - 1], param);
	switch (msg) {
	case WINE_WM_PAUSING:
	    wodPlayer_Reset(wwo, FALSE);
	    SetEvent(ev);
	    break;
	case WINE_WM_RESTARTING:
	    wwo->state = WINE_WS_PLAYING;
	    SetEvent(ev);
	    break;
	case WINE_WM_HEADER:
	    lpWaveHdr = (LPWAVEHDR)param;

	    /* insert buffer at the end of queue */
	    {
		LPWAVEHDR*	wh;
		for (wh = &(wwo->lpQueuePtr); *wh; wh = &((*wh)->lpNext));
		*wh = lpWaveHdr;
	    }
            if (!wwo->lpPlayPtr)
                wodPlayer_BeginWaveHdr(wwo,lpWaveHdr);
	    if (wwo->state == WINE_WS_STOPPED)
		wwo->state = WINE_WS_PLAYING;
	    break;
	case WINE_WM_RESETTING:
	    wodPlayer_Reset(wwo, TRUE);
	    SetEvent(ev);
	    break;
        case WINE_WM_UPDATE:
            wodUpdatePlayedTotal(wwo);
	    SetEvent(ev);
            break;
        case WINE_WM_BREAKLOOP:
            if (wwo->state == WINE_WS_PLAYING && wwo->lpLoopPtr != NULL) {
                /* ensure exit at end of current loop */
                wwo->dwLoops = 1;
            }
	    SetEvent(ev);
            break;
	case WINE_WM_CLOSING:
	    /* sanity check: this should not happen since the device must have been reset before */
	    if (wwo->lpQueuePtr || wwo->lpPlayPtr) ERR("out of sync\n");
	    wwo->hThread = 0;
	    wwo->state = WINE_WS_CLOSED;
	    SetEvent(ev);
	    ExitThread(0);
	    /* shouldn't go here */
	default:
	    FIXME("unknown message %d\n", msg);
	    break;
	}
    }
}

/**************************************************************************
 * 				wodPlayer			[internal]
 */
static	DWORD	CALLBACK	wodPlayer(LPVOID pmt)
{
    WORD	  uDevID = (DWORD)pmt;
    WINE_WAVEOUT* wwo = (WINE_WAVEOUT*)&WOutDev[uDevID];

    wwo->state = WINE_WS_STOPPED;
    SetEvent(wwo->hStartUpEvent);

    for (;;) {

        if (wwo->FlowStarted) {
           AuHandleEvents(wwo->AuServ);

           if (wwo->state == WINE_WS_PLAYING && wwo->freeBytes && wwo->BufferUsed)
              nas_send_buffer(wwo);
        }

        if (wwo->BufferUsed <= FRAG_SIZE && wwo->writeBytes > 0)
           wodPlayer_NotifyCompletions(wwo, FALSE);

        WaitForSingleObject(wwo->msgRing.msg_event, 20);
        wodPlayer_ProcessMessages(wwo);

        while(wwo->lpPlayPtr) {
           wwo->lpPlayPtr->reserved = wwo->WrittenTotal + wwo->lpPlayPtr->dwBufferLength;
           nas_add_buffer(wwo);
           wodPlayer_PlayPtrNext(wwo);
        }

    }
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
    WINE_WAVEOUT*	wwo;

    TRACE("wodOpen (%u, %p, %08X);\n", wDevID, lpDesc, dwFlags);

    if (lpDesc == NULL) {
	WARN("Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_WAVEOUTDRV) {
	TRACE("MAX_WAVOUTDRV reached !\n");
	return MMSYSERR_BADDEVICEID;
    }

    /* if this device is already open tell the app that it is allocated */

    wwo = &WOutDev[wDevID];

    if(wwo->open)
    {
      TRACE("device already allocated\n");
      return MMSYSERR_ALLOCATED;
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

    /* direct sound not supported, ignore the flag */
    dwFlags &= ~WAVE_DIRECTSOUND;

    wwo->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    memcpy(&wwo->waveDesc, lpDesc, 	     sizeof(WAVEOPENDESC));
    memcpy(&wwo->format,   lpDesc->lpFormat, sizeof(PCMWAVEFORMAT));

    if (wwo->format.wBitsPerSample == 0) {
	WARN("Resetting zeroed wBitsPerSample\n");
	wwo->format.wBitsPerSample = 8 *
	    (wwo->format.wf.nAvgBytesPerSec /
	     wwo->format.wf.nSamplesPerSec) /
	    wwo->format.wf.nChannels;
    }

    if (!nas_open(wwo))
       return MMSYSERR_ALLOCATED;

    NAS_InitRingMessage(&wwo->msgRing);

    /* create player thread */
    if (!(dwFlags & WAVE_DIRECTSOUND)) {
	wwo->hStartUpEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
	wwo->hThread = CreateThread(NULL, 0, wodPlayer, (LPVOID)(DWORD)wDevID, 0, &(wwo->dwThreadID));
        if (wwo->hThread)
            SetThreadPriority(wwo->hThread, THREAD_PRIORITY_TIME_CRITICAL);
	WaitForSingleObject(wwo->hStartUpEvent, INFINITE);
	CloseHandle(wwo->hStartUpEvent);
    } else {
	wwo->hThread = INVALID_HANDLE_VALUE;
	wwo->dwThreadID = 0;
    }
    wwo->hStartUpEvent = INVALID_HANDLE_VALUE;

    TRACE("stream=0x%lx, BufferSize=%d\n", (long)wwo->AuServ, wwo->BufferSize);

    TRACE("wBitsPerSample=%u nAvgBytesPerSec=%u nSamplesPerSec=%u nChannels=%u nBlockAlign=%u\n",
	  wwo->format.wBitsPerSample, wwo->format.wf.nAvgBytesPerSec,
	  wwo->format.wf.nSamplesPerSec, wwo->format.wf.nChannels,
	  wwo->format.wf.nBlockAlign);

    return wodNotifyClient(wwo, WOM_OPEN, 0L, 0L);
}

/**************************************************************************
 * 				wodClose			[internal]
 */
static DWORD wodClose(WORD wDevID)
{
    DWORD		ret = MMSYSERR_NOERROR;
    WINE_WAVEOUT*	wwo;

    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || AuServ  == NULL)
    {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    wwo = &WOutDev[wDevID];
    if (wwo->lpQueuePtr) {
	WARN("buffers still playing !\n");
	ret = WAVERR_STILLPLAYING;
    } else {
	TRACE("imhere[3-close]\n");
	if (wwo->hThread != INVALID_HANDLE_VALUE) {
	    NAS_AddRingMessage(&wwo->msgRing, WINE_WM_CLOSING, 0, TRUE);
	}

        NAS_DestroyRingMessage(&wwo->msgRing);

	NAS_CloseDevice(wwo);	/* close the stream and clean things up */

	ret = wodNotifyClient(wwo, WOM_CLOSE, 0L, 0L);
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
    if (wDevID >= MAX_WAVEOUTDRV || AuServ == NULL)
    {
        WARN("bad dev ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    if (lpWaveHdr->lpData == NULL || !(lpWaveHdr->dwFlags & WHDR_PREPARED))
    {
	TRACE("unprepared\n");
	return WAVERR_UNPREPARED;
    }

    if (lpWaveHdr->dwFlags & WHDR_INQUEUE)
    {
	TRACE("still playing\n");
	return WAVERR_STILLPLAYING;
    }

    lpWaveHdr->dwFlags &= ~WHDR_DONE;
    lpWaveHdr->dwFlags |= WHDR_INQUEUE;
    lpWaveHdr->lpNext = 0;

    TRACE("adding ring message\n");
    NAS_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_HEADER, (DWORD)lpWaveHdr, FALSE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodPause				[internal]
 */
static DWORD wodPause(WORD wDevID)
{
    TRACE("(%u);!\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || AuServ == NULL)
    {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    TRACE("imhere[3-PAUSING]\n");
    NAS_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_PAUSING, 0, TRUE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodRestart				[internal]
 */
static DWORD wodRestart(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || AuServ == NULL)
    {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    if (WOutDev[wDevID].state == WINE_WS_PAUSED) {
	TRACE("imhere[3-RESTARTING]\n");
	NAS_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_RESTARTING, 0, TRUE);
    }

    /* FIXME: is NotifyClient with WOM_DONE right ? (Comet Busters 1.3.3 needs this notification) */
    /* FIXME: Myst crashes with this ... hmm -MM
       return wodNotifyClient(wwo, WOM_DONE, 0L, 0L);
    */

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodReset				[internal]
 */
static DWORD wodReset(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || AuServ == NULL)
    {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    TRACE("imhere[3-RESET]\n");
    NAS_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_RESETTING, 0, TRUE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodGetPosition			[internal]
 */
static DWORD wodGetPosition(WORD wDevID, LPMMTIME lpTime, DWORD uSize)
{
    WINE_WAVEOUT*	wwo;

    TRACE("%u, %p, %u);\n", wDevID, lpTime, uSize);

    if (wDevID >= MAX_WAVEOUTDRV || AuServ == NULL)
    {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;

    wwo = &WOutDev[wDevID];
#if 0
    NAS_AddRingMessage(&wwo->msgRing, WINE_WM_UPDATE, 0, TRUE);
#endif

    return bytes_to_mmtime(lpTime, wwo->WrittenTotal, &wwo->format);
}

/**************************************************************************
 * 				wodBreakLoop			[internal]
 */
static DWORD wodBreakLoop(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || AuServ == NULL)
    {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    NAS_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_BREAKLOOP, 0, TRUE);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodGetVolume			[internal]
 */
static DWORD wodGetVolume(WORD wDevID, LPDWORD lpdwVol)
{
    DWORD left, right;

    left = WOutDev[wDevID].volume_left;
    right = WOutDev[wDevID].volume_right;

    TRACE("(%u, %p);\n", wDevID, lpdwVol);

    *lpdwVol = ((left * 0xFFFFl) / 100) + (((right * 0xFFFFl) / 100) << 16);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodSetVolume			[internal]
 */
static DWORD wodSetVolume(WORD wDevID, DWORD dwParam)
{
    DWORD left, right;

    left  = (LOWORD(dwParam) * 100) / 0xFFFFl;
    right = (HIWORD(dwParam) * 100) / 0xFFFFl;

    TRACE("(%u, %08X);\n", wDevID, dwParam);

    WOutDev[wDevID].volume_left = left;
    WOutDev[wDevID].volume_right = right;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodGetNumDevs			[internal]
 */
static	DWORD	wodGetNumDevs(void)
{
    return MAX_WAVEOUTDRV;
}

/**************************************************************************
 * 				wodMessage (WINENAS.@)
 */
DWORD WINAPI NAS_wodMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%u, %04X, %08X, %08X, %08X);\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);

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
    case WODM_BREAKLOOP: 	return wodBreakLoop     (wDevID);
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
 *======================================================================*/
static DWORD wodDsCreate(UINT wDevID, PIDSDRIVER* drv)
{
    /* we can't perform memory mapping as we don't have a file stream
	interface with nas like we do with oss */
    MESSAGE("This sound card s driver does not support direct access\n");
    MESSAGE("The (slower) DirectSound HEL mode will be used instead.\n");
    return MMSYSERR_NOTSUPPORTED;
}

static DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc)
{
    memset(desc, 0, sizeof(*desc));
    strcpy(desc->szDesc, "Wine NAS DirectSound Driver");
    strcpy(desc->szDrvname, "winenas.drv");
    return MMSYSERR_NOERROR;
}

static int nas_init(void) {
    TRACE("NAS INIT\n");
    if (!(AuServ = AuOpenServer(NULL, 0, NULL, 0, NULL, NULL)))
       return 0;

    return 1;
}

static int nas_finddev(WINE_WAVEOUT* wwo) {
   int i;

    for (i = 0; i < AuServerNumDevices(wwo->AuServ); i++) {
        if ((AuDeviceKind(AuServerDevice(wwo->AuServ, i)) ==
             AuComponentKindPhysicalOutput) &&
             AuDeviceNumTracks(AuServerDevice(wwo->AuServ, i)) == wwo->format.wf.nChannels)
        {
            wwo->AuDev = AuDeviceIdentifier(AuServerDevice(wwo->AuServ, i));
            break;
        }
    }

    if (wwo->AuDev == AuNone)
       return 0;
    return 1;
}

static int nas_open(WINE_WAVEOUT* wwo) {
    AuElement elements[3];

    if (!wwo->AuServ)
       return 0;

    if (!nas_finddev(wwo))
       return 0;

    if (!(wwo->AuFlow = AuCreateFlow(wwo->AuServ, NULL)))
       return 0;

    wwo->BufferSize = FRAG_SIZE * FRAG_COUNT;

    AuMakeElementImportClient(&elements[0], wwo->format.wf.nSamplesPerSec,
           wwo->format.wBitsPerSample == 16 ? AuFormatLinearSigned16LSB : AuFormatLinearUnsigned8,
           wwo->format.wf.nChannels, AuTrue, wwo->BufferSize, wwo->BufferSize / 2, 0, NULL);

    AuMakeElementExportDevice(&elements[1], 0, wwo->AuDev, wwo->format.wf.nSamplesPerSec,
                              AuUnlimitedSamples, 0, NULL);

    AuSetElements(wwo->AuServ, wwo->AuFlow, AuTrue, 2, elements, NULL);

    AuRegisterEventHandler(wwo->AuServ, AuEventHandlerIDMask, 0, wwo->AuFlow,
                           event_handler, (AuPointer) wwo);


    wwo->PlayedTotal = 0;
    wwo->WrittenTotal = 0;
    wwo->open = 1;

    wwo->BufferUsed = 0;
    wwo->writeBytes = 0;
    wwo->freeBytes = 0;
    wwo->sendBytes = 0;
    wwo->SoundBuffer = NULL;
    wwo->FlowStarted = 0;

    AuStartFlow(wwo->AuServ, wwo->AuFlow, NULL);
    AuPauseFlow(wwo->AuServ, wwo->AuFlow, NULL);
    wwo->FlowStarted = 1;

    return 1;
}

static AuBool
event_handler(AuServer* aud, AuEvent* ev, AuEventHandlerRec* hnd)
{
  WINE_WAVEOUT *wwo = (WINE_WAVEOUT *)hnd->data;
        switch (ev->type) {

        case AuEventTypeElementNotify: {
                AuElementNotifyEvent* event = (AuElementNotifyEvent *)ev;


                switch (event->kind) {
                   case AuElementNotifyKindLowWater:
                     wwo->freeBytes += event->num_bytes;
                     if (wwo->writeBytes > 0)
                        wwo->sendBytes += event->num_bytes;
                    if (wwo->freeBytes && wwo->BufferUsed)
                        nas_send_buffer(wwo);
                   break;

                   case AuElementNotifyKindState:
                     TRACE("ev: kind %s state %s->%s reason %s numbytes %ld freeB %u\n",
                                     nas_elementnotify_kind(event->kind),
                                     nas_state(event->prev_state),
                                     nas_state(event->cur_state),
                                     nas_reason(event->reason),
                                     event->num_bytes, wwo->freeBytes);

                     if (event->cur_state ==  AuStatePause && event->reason != AuReasonUser) {
                        wwo->freeBytes += event->num_bytes;
                        if (wwo->writeBytes > 0)
                           wwo->sendBytes += event->num_bytes;
                        if (wwo->sendBytes > wwo->writeBytes)
                           wwo->sendBytes = wwo->writeBytes;
                       if (wwo->freeBytes && wwo->BufferUsed)
                           nas_send_buffer(wwo);
                     }
                   break;
                }
           }
        }
        return AuTrue;
}

static void
buffer_resize(WINE_WAVEOUT* wwo, int len)
{
        void *newbuf = malloc(wwo->BufferUsed + len);
        void *oldbuf = wwo->SoundBuffer;
        memcpy(newbuf, oldbuf, wwo->BufferUsed);
        wwo->SoundBuffer = newbuf;
        free(oldbuf);
}

static int nas_add_buffer(WINE_WAVEOUT* wwo) {
    int len = wwo->lpPlayPtr->dwBufferLength;

    buffer_resize(wwo, len);
    memcpy(wwo->SoundBuffer + wwo->BufferUsed, wwo->lpPlayPtr->lpData, len);
    wwo->BufferUsed += len;
    wwo->WrittenTotal += len;
    return len;
}

static int nas_send_buffer(WINE_WAVEOUT* wwo) {
  int oldb , len;
  char *ptr, *newdata;
  newdata = NULL;
  oldb = len = 0;

  if (wwo->freeBytes <= 0)
     return 0;

  if (wwo->SoundBuffer == NULL || wwo->BufferUsed == 0) {
     return 0;
  }

  if (wwo->BufferUsed <= wwo->freeBytes) {
     len = wwo->BufferUsed;
     ptr = wwo->SoundBuffer;
  } else {
     len = wwo->freeBytes;
     ptr = malloc(len);
     memcpy(ptr,wwo->SoundBuffer,len);
     newdata = malloc(wwo->BufferUsed - len);
     memcpy(newdata, wwo->SoundBuffer + len, wwo->BufferUsed - len);
  }

 TRACE("envoye de %d bytes / %lu bytes / freeBytes %u\n", len, wwo->BufferUsed, wwo->freeBytes);

 AuWriteElement(wwo->AuServ, wwo->AuFlow, 0, len, ptr, AuFalse, NULL);

 wwo->BufferUsed -= len;
 wwo->freeBytes -= len;
 wwo->writeBytes += len;

 free(ptr);

 wwo->SoundBuffer = NULL;

 if (newdata != NULL)
    wwo->SoundBuffer = newdata;

 return len;
}

static int nas_free(WINE_WAVEOUT* wwo)
{

  if (!wwo->FlowStarted && wwo->BufferUsed) {
     AuStartFlow(wwo->AuServ, wwo->AuFlow, NULL);
     wwo->FlowStarted = 1;
  }

  while (wwo->BufferUsed || wwo->writeBytes != wwo->sendBytes) {
    if (wwo->freeBytes)
       nas_send_buffer(wwo);
    AuHandleEvents(wwo->AuServ);
  }

  AuFlush(wwo->AuServ);
  return TRUE;
}

static int nas_close(WINE_WAVEOUT* wwo)
{
  AuEvent ev;

  nas_free(wwo);

  AuStopFlow(wwo->AuServ, wwo->AuFlow, NULL);
  AuDestroyFlow(wwo->AuServ, wwo->AuFlow, NULL);
  AuFlush(wwo->AuServ);
  AuNextEvent(wwo->AuServ, AuTrue, &ev);
  AuDispatchEvent(wwo->AuServ, &ev);

  wwo->AuFlow = 0;
  wwo->open = 0;
  wwo->BufferUsed = 0;
  wwo->freeBytes = 0;
  wwo->SoundBuffer = NULL;
  return 1;
}

static int nas_end(void)
{
  if (AuServ)
  {
    AuCloseServer(AuServ);
    AuServ = 0;
  }
  return 1;
}

#else /* !HAVE_NAS */

/**************************************************************************
 * 				wodMessage (WINENAS.@)
 */
DWORD WINAPI NAS_wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08X, %08X, %08X):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}
#endif
