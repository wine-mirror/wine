/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Wine Driver for aRts Sound Server
 *   http://www.arts-project.org
 *
 * Copyright 1994 Martin Ayotte
 *           1999 Eric Pouech (async playing in waveOut/waveIn)
 *	     2000 Eric Pouech (loops in waveOut)
 *	     2002 Chris Morgan (aRts version of this file)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* NOTE:
 *    with arts we cannot stop the audio that is already in
 *    the servers buffer, so to reduce delays during starting
 *    and stoppping of audio streams adjust the
 *    audio buffer size in the kde control center or in the
 *    artsd startup script
 *
 * FIXME:
 *	pause in waveOut does not work correctly in loop mode
 *
 *	does something need to be done in for WaveIn DirectSound?
 */

#include "config.h"
#include "wine/port.h"

#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "winuser.h"
#include "mmddk.h"
#include "dsound.h"
#include "dsdriver.h"
#include "arts.h"
#include "wine/unicode.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wave);

#ifdef HAVE_ARTS

#include <artsc.h>

/* The following four #defines allow you to fine-tune the packet
 * settings in arts for better low-latency support. You must also
 * adjust the latency in the KDE arts control panel. I recommend 4
 * fragments, 1024 bytes.
 *
 * The following is from the ARTS documentation and explains what CCCC
 * and SSSS mean:
 * 
 * @li ARTS_P_PACKET_SETTINGS (rw) This is a way to configure packet
 * size & packet count at the same time.  The format is 0xCCCCSSSS,
 * where 2^SSSS is the packet size, and CCCC is the packet count. Note
 * that when writing this, you don't necessarily get the settings you
 * requested.
 */
#define WAVEOUT_PACKET_CCCC 0x000C
#define WAVEOUT_PACKET_SSSS 0x0008
#define WAVEIN_PACKET_CCCC  0x000C
#define WAVEIN_PACKET_SSSS  0x0008

#define BUFFER_REFILL_THRESHOLD 4

#define WAVEOUT_PACKET_SETTINGS ((WAVEOUT_PACKET_CCCC << 16) | (WAVEOUT_PACKET_SSSS))
#define WAVEIN_PACKET_SETTINGS  ((WAVEIN_PACKET_CCCC << 16) | (WAVEIN_PACKET_SSSS))

#define MAX_WAVEOUTDRV 	(10)
#define MAX_WAVEINDRV 	(10)

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
    WINE_WM_UPDATE, WINE_WM_BREAKLOOP, WINE_WM_CLOSING, WINE_WM_STARTING, WINE_WM_STOPPING
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
#define ARTS_RING_BUFFER_INCREMENT      64
typedef struct {
    RING_MSG			* messages;
    int                         ring_buffer_size;
    int				msg_tosave;
    int				msg_toget;
    HANDLE			msg_event;
    CRITICAL_SECTION		msg_crst;
} ARTS_MSG_RING;

typedef struct {
    volatile int		state;			/* one of the WINE_WS_ manifest constants */
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    PCMWAVEFORMAT		format;
    WAVEOUTCAPSW		caps;
    char                        interface_name[32];

    DWORD			dwSleepTime;		/* Num of milliseconds to sleep between filling the dsp buffers */

    /* arts information */
    arts_stream_t 		play_stream;		/* the stream structure we get from arts when opening a stream for playing */
    DWORD                       dwBufferSize;           /* size of whole buffer in bytes */
    int				packetSettings;

    char*			sound_buffer;
    long			buffer_size;

    DWORD			volume_left;		/* volume control information */
    DWORD			volume_right;

    LPWAVEHDR			lpQueuePtr;		/* start of queued WAVEHDRs (waiting to be notified) */
    LPWAVEHDR			lpPlayPtr;		/* start of not yet fully played buffers */
    DWORD			dwPartialOffset;	/* Offset of not yet written bytes in lpPlayPtr */

    LPWAVEHDR			lpLoopPtr;              /* pointer of first buffer in loop, if any */
    DWORD			dwLoops;		/* private copy of loop counter */

    DWORD			dwPlayedTotal;		/* number of bytes actually played since opening */
    DWORD                       dwWrittenTotal;         /* number of bytes written to the audio device since opening */

    /* synchronization stuff */
    HANDLE			hStartUpEvent;
    HANDLE			hThread;
    DWORD			dwThreadID;
    ARTS_MSG_RING		msgRing;
} WINE_WAVEOUT;

typedef struct {
    volatile int		state;			/* one of the WINE_WS_ manifest constants */
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    PCMWAVEFORMAT		format;
    WAVEINCAPSW			caps;
    char                        interface_name[32];

    /* arts information */
    arts_stream_t 		record_stream;		/* the stream structure we get from arts when opening a stream for recording */
    int				packetSettings;

    LPWAVEHDR			lpQueuePtr;
    DWORD			dwRecordedTotal;

    /* synchronization stuff */
    HANDLE			hStartUpEvent;
    HANDLE			hThread;
    DWORD			dwThreadID;
    ARTS_MSG_RING		msgRing;
} WINE_WAVEIN;

static BOOL		init;
static WINE_WAVEOUT	WOutDev   [MAX_WAVEOUTDRV];
static WINE_WAVEIN	WInDev    [MAX_WAVEINDRV];

static DWORD wodDsCreate(UINT wDevID, PIDSDRIVER* drv);
static DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc);

/* These strings used only for tracing */
static const char *wodPlayerCmdString[] = {
    "WINE_WM_PAUSING",
    "WINE_WM_RESTARTING",
    "WINE_WM_RESETTING",
    "WINE_WM_HEADER",
    "WINE_WM_UPDATE",
    "WINE_WM_BREAKLOOP",
    "WINE_WM_CLOSING",
    "WINE_WM_STARTING",
    "WINE_WM_STOPPING",
};

static DWORD bytes_to_mmtime(LPMMTIME lpTime, DWORD position,
                             PCMWAVEFORMAT* format)
{
    TRACE("wType=%04X wBitsPerSample=%u nSamplesPerSec=%lu nChannels=%u nAvgBytesPerSec=%lu\n",
          lpTime->wType, format->wBitsPerSample, format->wf.nSamplesPerSec,
          format->wf.nChannels, format->wf.nAvgBytesPerSec);
    TRACE("Position in bytes=%lu\n", position);

    switch (lpTime->wType) {
    case TIME_SAMPLES:
        lpTime->u.sample = position / (format->wBitsPerSample / 8 * format->wf.nChannels);
        TRACE("TIME_SAMPLES=%lu\n", lpTime->u.sample);
        break;
    case TIME_MS:
        lpTime->u.ms = 1000.0 * position / (format->wBitsPerSample / 8 * format->wf.nChannels * format->wf.nSamplesPerSec);
        TRACE("TIME_MS=%lu\n", lpTime->u.ms);
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
        TRACE("TIME_BYTES=%lu\n", lpTime->u.cb);
        break;
    }
    return MMSYSERR_NOERROR;
}

/*======================================================================*
 *                  Low level WAVE implementation			*
 *======================================================================*/

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
                           int right, int 	nChannels)
{
  BYTE *d_out = (BYTE *)bufout;
  BYTE *d_in = (BYTE *)bufin;
  int i, v;

/*
  TRACE("length == %d, nChannels == %d\n", length, nChannels);
*/

  if (right == -1) right = left;

  for(i = 0; i < length; i+=(nChannels))
  {
    v = (BYTE) ((*(d_in++) * left) / 100);
    *(d_out++) = (v>255) ? 255 : ((v<0) ? 0 : v);
    if(nChannels == 2)
    {
      v = (BYTE) ((*(d_in++) * right) / 100);
      *(d_out++) = (v>255) ? 255 : ((v<0) ? 0 : v);
    }
  }
}

/******************************************************************
 *		ARTS_CloseWaveOutDevice
 *
 */
static void ARTS_CloseWaveOutDevice(WINE_WAVEOUT* wwo)
{
  arts_close_stream(wwo->play_stream); 	/* close the arts stream */
  wwo->play_stream = (arts_stream_t*)-1;

  /* free up the buffer we use for volume and reset the size */
  HeapFree(GetProcessHeap(), 0, wwo->sound_buffer);
  wwo->sound_buffer = NULL;
  wwo->buffer_size = 0;
}

/******************************************************************
 *		ARTS_CloseWaveInDevice
 *
 */
static void ARTS_CloseWaveInDevice(WINE_WAVEIN* wwi)
{
  arts_close_stream(wwi->record_stream); 	/* close the arts stream */
  wwi->record_stream = (arts_stream_t*)-1;
}

/******************************************************************
 *		ARTS_Init
 */
static int	ARTS_Init(void)
{
  return arts_init();  /* initialize arts and return errorcode */
}

/******************************************************************
 *		ARTS_WaveClose
 */
LONG		ARTS_WaveClose(void)
{
    int iDevice;

    /* close all open devices */
    for(iDevice = 0; iDevice < MAX_WAVEOUTDRV; iDevice++)
    {
      if(WOutDev[iDevice].play_stream != (arts_stream_t*)-1)
      {
        ARTS_CloseWaveOutDevice(&WOutDev[iDevice]);
      }
    }

    for(iDevice = 0; iDevice < MAX_WAVEINDRV; iDevice++)
    {
      if(WInDev[iDevice].record_stream != (arts_stream_t*)-1)
      {
        ARTS_CloseWaveInDevice(&WInDev[iDevice]);
      }
    }

    if (init)
        arts_free();    /* free up arts */
    return 1;
}

/******************************************************************
 *		ARTS_WaveInit
 *
 * Initialize internal structures from ARTS server info
 */
LONG ARTS_WaveInit(void)
{
    int 	i;
    int		errorcode;
    LONG	ret = 0;

    TRACE("called\n");

    __TRY
    {
        if ((errorcode = ARTS_Init()) < 0)
        {
            WARN("arts_init() failed (%d)\n", errorcode);
            ret = -1;
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        ERR("arts_init() crashed\n");
        ret = -1;
    }
    __ENDTRY

    if (ret)
        return ret;

    init = TRUE;

    /* initialize all device handles to -1 */
    for (i = 0; i < MAX_WAVEOUTDRV; ++i)
    {
        static const WCHAR ini[] = {'a','R','t','s',' ','W','a','v','e','O','u','t','D','r','i','v','e','r',0};

	WOutDev[i].play_stream = (arts_stream_t*)-1;
	memset(&WOutDev[i].caps, 0, sizeof(WOutDev[i].caps)); /* zero out
							caps values */
    	WOutDev[i].caps.wMid = 0x00FF; 	/* Manufac ID */
    	WOutDev[i].caps.wPid = 0x0001; 	/* Product ID */
    	strcpyW(WOutDev[i].caps.szPname, ini);
        snprintf(WOutDev[i].interface_name, sizeof(WOutDev[i].interface_name), "winearts: %d", i);

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

    for (i = 0; i < MAX_WAVEINDRV; ++i)
    {
        static const WCHAR ini[] = {'a','R','t','s',' ','W','a','v','e','I','n',' ','D','r','i','v','e','r',0};

	WInDev[i].record_stream = (arts_stream_t*)-1;
	memset(&WInDev[i].caps, 0, sizeof(WInDev[i].caps)); /* zero out
							caps values */
	WInDev[i].caps.wMid = 0x00FF;
	WInDev[i].caps.wPid = 0x0001;
	strcpyW(WInDev[i].caps.szPname, ini);
        snprintf(WInDev[i].interface_name, sizeof(WInDev[i].interface_name), "winearts: %d", i);

	WInDev[i].caps.vDriverVersion = 0x0100;
   	WInDev[i].caps.dwFormats = 0x00000000;

    	WInDev[i].caps.wChannels = 2;

    	WInDev[i].caps.dwFormats |= WAVE_FORMAT_4M08;
    	WInDev[i].caps.dwFormats |= WAVE_FORMAT_4S08;
    	WInDev[i].caps.dwFormats |= WAVE_FORMAT_4S16;
    	WInDev[i].caps.dwFormats |= WAVE_FORMAT_4M16;
    	WInDev[i].caps.dwFormats |= WAVE_FORMAT_2M08;
    	WInDev[i].caps.dwFormats |= WAVE_FORMAT_2S08;
    	WInDev[i].caps.dwFormats |= WAVE_FORMAT_2M16;
    	WInDev[i].caps.dwFormats |= WAVE_FORMAT_2S16;
    	WInDev[i].caps.dwFormats |= WAVE_FORMAT_1M08;
    	WInDev[i].caps.dwFormats |= WAVE_FORMAT_1S08;
	WInDev[i].caps.dwFormats |= WAVE_FORMAT_1M16;
	WInDev[i].caps.dwFormats |= WAVE_FORMAT_1S16;

	WInDev[i].caps.wReserved1 = 0;
    }
    return 0;
}

/******************************************************************
 *		ARTS_InitRingMessage
 *
 * Initialize the ring of messages for passing between driver's caller and playback/record
 * thread
 */
static int ARTS_InitRingMessage(ARTS_MSG_RING* mr)
{
    mr->msg_toget = 0;
    mr->msg_tosave = 0;
    mr->msg_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    mr->ring_buffer_size = ARTS_RING_BUFFER_INCREMENT;
    mr->messages = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,mr->ring_buffer_size * sizeof(RING_MSG));
    InitializeCriticalSection(&mr->msg_crst);
    return 0;
}

/******************************************************************
 *		ARTS_DestroyRingMessage
 *
 */
static int ARTS_DestroyRingMessage(ARTS_MSG_RING* mr)
{
    CloseHandle(mr->msg_event);
    HeapFree(GetProcessHeap(),0,mr->messages);
    mr->messages=NULL;
    DeleteCriticalSection(&mr->msg_crst);
    return 0;
}

/******************************************************************
 *		ARTS_AddRingMessage
 *
 * Inserts a new message into the ring (should be called from DriverProc derivated routines)
 */
static int ARTS_AddRingMessage(ARTS_MSG_RING* mr, enum win_wm_message msg, DWORD param, BOOL wait)
{
    HANDLE      hEvent = INVALID_HANDLE_VALUE;

    EnterCriticalSection(&mr->msg_crst);
    if ((mr->msg_toget == ((mr->msg_tosave + 1) % mr->ring_buffer_size)))
    {
	int old_ring_buffer_size = mr->ring_buffer_size;
	mr->ring_buffer_size += ARTS_RING_BUFFER_INCREMENT;
	TRACE("mr->ring_buffer_size=%d\n",mr->ring_buffer_size);
	mr->messages = HeapReAlloc(GetProcessHeap(),0,mr->messages, mr->ring_buffer_size * sizeof(RING_MSG));
	/* Now we need to rearrange the ring buffer so that the new
	   buffers just allocated are in between mr->msg_tosave and
	   mr->msg_toget.
	*/
	if (mr->msg_tosave < mr->msg_toget)
	{
	    memmove(&(mr->messages[mr->msg_toget + ARTS_RING_BUFFER_INCREMENT]),
		    &(mr->messages[mr->msg_toget]),
		    sizeof(RING_MSG)*(old_ring_buffer_size - mr->msg_toget)
		    );
	    mr->msg_toget += ARTS_RING_BUFFER_INCREMENT;
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
 *		ARTS_RetrieveRingMessage
 *
 * Get a message from the ring. Should be called by the playback/record thread.
 */
static int ARTS_RetrieveRingMessage(ARTS_MSG_RING* mr,
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
    TRACE("wMsg = 0x%04x dwParm1 = %04lX dwParam2 = %04lX\n", wMsg, dwParam1, dwParam2);

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
    /* total played is the bytes written less the bytes to write ;-) */
    wwo->dwPlayedTotal = wwo->dwWrittenTotal -
	(wwo->dwBufferSize -
	arts_stream_get(wwo->play_stream, ARTS_P_BUFFER_SPACE));

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
	    TRACE("Starting loop (%ldx) with %p\n", lpWaveHdr->dwLoops, lpWaveHdr);
	    wwo->lpLoopPtr = lpWaveHdr;
	    /* Windows does not touch WAVEHDR.dwLoops,
	     * so we need to make an internal copy */
	    wwo->dwLoops = lpWaveHdr->dwLoops;
	}
    }
    wwo->dwPartialOffset = 0;
}

/**************************************************************************
 * 				wodPlayer_PlayPtrNext	        [internal]
 *
 * Advance the play pointer to the next waveheader, looping if required.
 */
static LPWAVEHDR wodPlayer_PlayPtrNext(WINE_WAVEOUT* wwo)
{
    LPWAVEHDR lpWaveHdr = wwo->lpPlayPtr;

    wwo->dwPartialOffset = 0;
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
 * 			     wodPlayer_NotifyWait               [internal]
 * Returns the number of milliseconds to wait before attempting to notify
 * completion of the specified wavehdr.
 * This is based on the number of bytes remaining to be written in the
 * wave.
 */
static DWORD wodPlayer_NotifyWait(const WINE_WAVEOUT* wwo, LPWAVEHDR lpWaveHdr)
{
    DWORD dwMillis;

    if(lpWaveHdr->reserved < wwo->dwPlayedTotal)
    {
	dwMillis = 1;
    }
    else
    {
        dwMillis = (lpWaveHdr->reserved - wwo->dwPlayedTotal) * 1000 / wwo->format.wf.nAvgBytesPerSec;
	if(!dwMillis) dwMillis = 1;
    }

    TRACE("dwMillis = %ld\n", dwMillis);

    return dwMillis;
}


/**************************************************************************
 * 			     wodPlayer_WriteMaxFrags            [internal]
 * Writes the maximum number of bytes possible to the DSP and returns
 * the number of bytes written.
 */
static int wodPlayer_WriteMaxFrags(WINE_WAVEOUT* wwo, DWORD* bytes)
{
    /* Only attempt to write to free bytes */
    DWORD dwLength = wwo->lpPlayPtr->dwBufferLength - wwo->dwPartialOffset;
    int toWrite = min(dwLength, *bytes);
    int written;

    TRACE("Writing wavehdr %p.%lu[%lu]\n",
          wwo->lpPlayPtr, wwo->dwPartialOffset, wwo->lpPlayPtr->dwBufferLength);

    if (dwLength == 0)
    {
        wodPlayer_PlayPtrNext(wwo);
        return 0;
    }

    /* see if our buffer isn't large enough for the data we are writing */
    if(wwo->buffer_size < toWrite)
    {
      if(wwo->sound_buffer)
      {
	wwo->sound_buffer = HeapReAlloc(GetProcessHeap(), 0, wwo->sound_buffer, toWrite);
	wwo->buffer_size = toWrite;
      }
    }

    /* if we don't have a buffer then get one */
    if(!wwo->sound_buffer)
    {
      /* allocate some memory for the buffer */
      wwo->sound_buffer = HeapAlloc(GetProcessHeap(), 0, toWrite);
      wwo->buffer_size = toWrite;
    }

    /* if we don't have a buffer then error out */
    if(!wwo->sound_buffer)
    {
      ERR("error allocating sound_buffer memory\n");
      return 0;
    }

    TRACE("toWrite == %d\n", toWrite);

    /* apply volume to the bits */
    /* for single channel audio streams we only use the LEFT volume */
    if(wwo->format.wBitsPerSample == 16)
    {
      /* apply volume to the buffer we are about to send */
      /* divide toWrite(bytes) by 2 as volume processes by 16 bits */
      volume_effect16(wwo->lpPlayPtr->lpData + wwo->dwPartialOffset,
                wwo->sound_buffer, toWrite>>1, wwo->volume_left,
		wwo->volume_right, wwo->format.wf.nChannels);
    } else if(wwo->format.wBitsPerSample == 8)
    {
      /* apply volume to the buffer we are about to send */
      volume_effect8(wwo->lpPlayPtr->lpData + wwo->dwPartialOffset,
                wwo->sound_buffer, toWrite, wwo->volume_left,
		wwo->volume_right, wwo->format.wf.nChannels);
    } else
    {
      FIXME("unsupported wwo->format.wBitsPerSample of %d\n",
        wwo->format.wBitsPerSample);
    }

    /* send the audio data to arts for playing */
    written = arts_write(wwo->play_stream, wwo->sound_buffer, toWrite);

    TRACE("written = %d\n", written);

    if (written <= 0) 
    {
      *bytes = 0; /* apparently arts is actually full */
      return written; /* if we wrote nothing just return */
    }

    if (written >= dwLength)
        wodPlayer_PlayPtrNext(wwo);   /* If we wrote all current wavehdr, skip to the next one */
    else
        wwo->dwPartialOffset += written;    /* Remove the amount written */

    if (written < toWrite)
	*bytes = 0;
    else
	*bytes -= written;

    wwo->dwWrittenTotal += written; /* update stats on this wave device */

    return written; /* return the number of bytes written */
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

    if (wwo->lpQueuePtr) {
	TRACE("lpWaveHdr=(%p), lpPlayPtr=(%p), lpLoopPtr=(%p), reserved=(%ld), dwWrittenTotal=(%ld), force=(%d)\n", 
	      wwo->lpQueuePtr,
	      wwo->lpPlayPtr,
	      wwo->lpLoopPtr,
	      wwo->lpQueuePtr->reserved,
	      wwo->dwWrittenTotal,
	      force);
    } else {
	TRACE("lpWaveHdr=(%p), lpPlayPtr=(%p), lpLoopPtr=(%p),  dwWrittenTotal=(%ld), force=(%d)\n", 
	      wwo->lpQueuePtr,
	      wwo->lpPlayPtr,
	      wwo->lpLoopPtr,
	      wwo->dwWrittenTotal,
	      force);
    }

    /* Start from lpQueuePtr and keep notifying until:
     * - we hit an unwritten wavehdr
     * - we hit the beginning of a running loop
     * - we hit a wavehdr which hasn't finished playing
     */
    while ((lpWaveHdr = wwo->lpQueuePtr) &&
           (force ||
            (lpWaveHdr != wwo->lpPlayPtr &&
             lpWaveHdr != wwo->lpLoopPtr &&
	     lpWaveHdr->reserved <= wwo->dwWrittenTotal))) {

	wwo->lpQueuePtr = lpWaveHdr->lpNext;

	lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
	lpWaveHdr->dwFlags |= WHDR_DONE;

	wodNotifyClient(wwo, WOM_DONE, (DWORD)lpWaveHdr, 0);
    }
    return  (lpWaveHdr && lpWaveHdr != wwo->lpPlayPtr && lpWaveHdr != wwo->lpLoopPtr) ?
        wodPlayer_NotifyWait(wwo, lpWaveHdr) : INFINITE;
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
    /* to arts, otherwise we would do the flushing here */

    if (reset) {
        enum win_wm_message     msg;
        DWORD                   param;
        HANDLE                  ev;

	/* remove any buffer */
	wodPlayer_NotifyCompletions(wwo, TRUE);

	wwo->lpPlayPtr = wwo->lpQueuePtr = wwo->lpLoopPtr = NULL;
	wwo->state = WINE_WS_STOPPED;
	wwo->dwPlayedTotal = wwo->dwWrittenTotal = 0;

        wwo->dwPartialOffset = 0;        /* Clear partial wavehdr */

        /* remove any existing message in the ring */
        EnterCriticalSection(&wwo->msgRing.msg_crst);

        /* return all pending headers in queue */
        while (ARTS_RetrieveRingMessage(&wwo->msgRing, &msg, &param, &ev))
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
            wwo->dwPartialOffset = 0;
            wwo->dwWrittenTotal = wwo->dwPlayedTotal; /* this is wrong !!! */
        } else {
	    /* the data already written is going to be played, so take */
	    /* this fact into account here */
	    wwo->dwPlayedTotal = wwo->dwWrittenTotal;
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

    while (ARTS_RetrieveRingMessage(&wwo->msgRing, &msg, &param, &ev)) {
	TRACE("Received %s %lx\n", wodPlayerCmdString[msg - WM_USER - 1], param);
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
 * 			     wodPlayer_FeedDSP			[internal]
 * Feed as much sound data as we can into the DSP and return the number of
 * milliseconds before it will be necessary to feed the DSP again.
 */
static DWORD wodPlayer_FeedDSP(WINE_WAVEOUT* wwo)
{
    DWORD       availInQ;

    wodUpdatePlayedTotal(wwo);
    availInQ = arts_stream_get(wwo->play_stream, ARTS_P_BUFFER_SPACE);
    TRACE("availInQ = %ld\n", availInQ);

    /* input queue empty */
    if (!wwo->lpPlayPtr) {
        TRACE("Run out of wavehdr:s... flushing\n");
        return INFINITE;
    }

    /* no more room... no need to try to feed */
    if(!availInQ)
    {
	TRACE("no more room, no need to try to feed\n");
	return wwo->dwSleepTime;
    }

    /* Feed from partial wavehdr */
    if (wwo->lpPlayPtr && wwo->dwPartialOffset != 0)
    {
        TRACE("feeding from partial wavehdr\n");
        wodPlayer_WriteMaxFrags(wwo, &availInQ);
    }

    /* Feed wavehdrs until we run out of wavehdrs or DSP space */
    if (!wwo->dwPartialOffset)
    {
	while(wwo->lpPlayPtr && availInQ)
	{
	    TRACE("feeding waveheaders until we run out of space\n");
	    /* note the value that dwPlayedTotal will return when this wave finishes playing */
	    wwo->lpPlayPtr->reserved = wwo->dwWrittenTotal + wwo->lpPlayPtr->dwBufferLength;
	    TRACE("reserved=(%ld) dwWrittenTotal=(%ld) dwBufferLength=(%ld)\n",
		  wwo->lpPlayPtr->reserved,
		  wwo->dwWrittenTotal,
		  wwo->lpPlayPtr->dwBufferLength
		);
	    wodPlayer_WriteMaxFrags(wwo, &availInQ);
	}
    }

    if (!wwo->lpPlayPtr) {
        TRACE("Ran out of wavehdrs\n");
        return INFINITE;
    }

    return wwo->dwSleepTime;
}


/**************************************************************************
 * 				wodPlayer			[internal]
 */
static	DWORD	CALLBACK	wodPlayer(LPVOID pmt)
{
    WORD	  uDevID = (DWORD)pmt;
    WINE_WAVEOUT* wwo = (WINE_WAVEOUT*)&WOutDev[uDevID];
    DWORD         dwNextFeedTime = INFINITE;   /* Time before DSP needs feeding */
    DWORD         dwNextNotifyTime = INFINITE; /* Time before next wave completion */
    DWORD         dwSleepTime;

    wwo->state = WINE_WS_STOPPED;
    SetEvent(wwo->hStartUpEvent);

    for (;;) {
        /** Wait for the shortest time before an action is required.  If there
         *  are no pending actions, wait forever for a command.
         */
        dwSleepTime = min(dwNextFeedTime, dwNextNotifyTime);
        TRACE("waiting %lums (%lu,%lu)\n", dwSleepTime, dwNextFeedTime, dwNextNotifyTime);
        WaitForSingleObject(wwo->msgRing.msg_event, dwSleepTime);
	wodPlayer_ProcessMessages(wwo);
	if (wwo->state == WINE_WS_PLAYING) {
	    dwNextFeedTime = wodPlayer_FeedDSP(wwo);
	    dwNextNotifyTime = wodPlayer_NotifyCompletions(wwo, FALSE);
	} else {
	    dwNextFeedTime = dwNextNotifyTime = INFINITE;
	}
    }
}

/**************************************************************************
 * 			wodGetDevCaps				[internal]
 */
static DWORD wodGetDevCaps(WORD wDevID, LPWAVEOUTCAPSW lpCaps, DWORD dwSize)
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

    /* if this device is already open tell the app that it is allocated */
    if(WOutDev[wDevID].play_stream != (arts_stream_t*)-1)
    {
      TRACE("device already allocated\n");
      return MMSYSERR_ALLOCATED;
    }

    /* only PCM format is supported so far... */
    if (lpDesc->lpFormat->wFormatTag != WAVE_FORMAT_PCM ||
	lpDesc->lpFormat->nChannels == 0 ||
        lpDesc->lpFormat->nSamplesPerSec < DSBFREQUENCY_MIN ||
        lpDesc->lpFormat->nSamplesPerSec > DSBFREQUENCY_MAX ||
        (lpDesc->lpFormat->wBitsPerSample!=8 && lpDesc->lpFormat->wBitsPerSample!=16)) {
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

    wwo->play_stream = arts_play_stream(wwo->format.wf.nSamplesPerSec,
	wwo->format.wBitsPerSample, wwo->format.wf.nChannels, "winearts");

    /* clear these so we don't have any confusion ;-) */
    wwo->sound_buffer = 0;
    wwo->buffer_size = 0;

    arts_stream_set(wwo->play_stream, ARTS_P_BLOCKING, 0);    /* disable blocking on this stream */

    if(!wwo->play_stream) return MMSYSERR_ALLOCATED;

    /* Try to set the packet settings from constant and store the value that it
       was actually set to for future use */
    wwo->packetSettings = arts_stream_set(wwo->play_stream, ARTS_P_PACKET_SETTINGS, WAVEOUT_PACKET_SETTINGS);
    TRACE("Tried to set ARTS_P_PACKET_SETTINGS to (%x), actually set to (%x)\n", WAVEOUT_PACKET_SETTINGS, wwo->packetSettings);

    wwo->dwBufferSize = arts_stream_get(wwo->play_stream, ARTS_P_BUFFER_SIZE);
    TRACE("Buffer size is now (%ld)\n",wwo->dwBufferSize);

    wwo->dwPlayedTotal = 0;
    wwo->dwWrittenTotal = 0;

    wwo->dwSleepTime = ((1 << (wwo->packetSettings & 0xFFFF)) * 1000 * BUFFER_REFILL_THRESHOLD) / wwo->format.wf.nAvgBytesPerSec;

    /* Initialize volume to full level */
    wwo->volume_left = 100;
    wwo->volume_right = 100;

    ARTS_InitRingMessage(&wwo->msgRing);

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

    TRACE("stream=0x%lx, dwBufferSize=%ld\n",
	  (long)wwo->play_stream, wwo->dwBufferSize);

    TRACE("wBitsPerSample=%u, nAvgBytesPerSec=%lu, nSamplesPerSec=%lu, nChannels=%u nBlockAlign=%u!\n",
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

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].play_stream  ==
	(arts_stream_t*)-1)
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
	    ARTS_AddRingMessage(&wwo->msgRing, WINE_WM_CLOSING, 0, TRUE);
	}

        ARTS_DestroyRingMessage(&wwo->msgRing);

	ARTS_CloseWaveOutDevice(wwo);	/* close the stream and clean things up */

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
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);

    /* first, do the sanity checks... */
    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].play_stream ==
		(arts_stream_t*)-1)
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
    ARTS_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_HEADER, (DWORD)lpWaveHdr, FALSE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodPause				[internal]
 */
static DWORD wodPause(WORD wDevID)
{
    TRACE("(%u);!\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].play_stream ==
		(arts_stream_t*)-1)
    {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    TRACE("imhere[3-PAUSING]\n");
    ARTS_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_PAUSING, 0, TRUE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodRestart				[internal]
 */
static DWORD wodRestart(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].play_stream ==
		(arts_stream_t*)-1)
    {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    if (WOutDev[wDevID].state == WINE_WS_PAUSED) {
	TRACE("imhere[3-RESTARTING]\n");
	ARTS_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_RESTARTING, 0, TRUE);
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

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].play_stream ==
	(arts_stream_t*)-1)
    {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    TRACE("imhere[3-RESET]\n");
    ARTS_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_RESETTING, 0, TRUE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodGetPosition			[internal]
 */
static DWORD wodGetPosition(WORD wDevID, LPMMTIME lpTime, DWORD uSize)
{
    WINE_WAVEOUT*	wwo;

    TRACE("(%u, %p, %lu);\n", wDevID, lpTime, uSize);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].play_stream ==
	(arts_stream_t*)-1)
    {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;

    wwo = &WOutDev[wDevID];
    ARTS_AddRingMessage(&wwo->msgRing, WINE_WM_UPDATE, 0, TRUE);

    return bytes_to_mmtime(lpTime, wwo->dwPlayedTotal, &wwo->format);
}

/**************************************************************************
 * 				wodBreakLoop			[internal]
 */
static DWORD wodBreakLoop(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].play_stream ==
		(arts_stream_t*)-1)
    {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    ARTS_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_BREAKLOOP, 0, TRUE);
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

    *lpdwVol = ((left * 0xFFFFl) / 100) + (((right * 0xFFFFl) / 100) <<
		16);

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

    TRACE("(%u, %08lX);\n", wDevID, dwParam);

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
 *                              wodDevInterfaceSize             [internal]
 */
static DWORD wodDevInterfaceSize(UINT wDevID, LPDWORD dwParam1)
{
    TRACE("(%u, %p)\n", wDevID, dwParam1);
                                                                                                       
    *dwParam1 = MultiByteToWideChar(CP_ACP, 0, WOutDev[wDevID].interface_name, -1,
                                    NULL, 0 ) * sizeof(WCHAR);
    return MMSYSERR_NOERROR;
}
                                                                                                       
/**************************************************************************
 *                              wodDevInterface                 [internal]
 */
static DWORD wodDevInterface(UINT wDevID, PWCHAR dwParam1, DWORD dwParam2)
{
    if (dwParam2 >= MultiByteToWideChar(CP_ACP, 0, WOutDev[wDevID].interface_name, -1,
                                        NULL, 0 ) * sizeof(WCHAR))
    {
        MultiByteToWideChar(CP_ACP, 0, WOutDev[wDevID].interface_name, -1,
                            dwParam1, dwParam2 / sizeof(WCHAR));
        return MMSYSERR_NOERROR;
    }
    return MMSYSERR_INVALPARAM;
}
                                                                                                       
/**************************************************************************
 * 				wodMessage (WINEARTS.@)
 */
DWORD WINAPI ARTS_wodMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
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

    case DRV_QUERYDEVICEINTERFACESIZE: return wodDevInterfaceSize       (wDevID, (LPDWORD)dwParam1);
    case DRV_QUERYDEVICEINTERFACE:     return wodDevInterface           (wDevID, (PWCHAR)dwParam1, dwParam2);
    case DRV_QUERYDSOUNDIFACE:	return wodDsCreate	(wDevID, (PIDSDRIVER*)dwParam1);
    case DRV_QUERYDSOUNDDESC:	return wodDsDesc	(wDevID, (PDSDRIVERDESC)dwParam1);
    default:
	FIXME("unknown message %d!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*======================================================================*
 *                  Low level WAVE IN implementation			*
 *======================================================================*/

/**************************************************************************
 * 				widGetNumDevs			[internal]
 */
static	DWORD	widGetNumDevs(void)
{
    TRACE("%d\n", MAX_WAVEINDRV);
    return MAX_WAVEINDRV;
}

/**************************************************************************
 *                              widDevInterfaceSize             [internal]
 */
static DWORD widDevInterfaceSize(UINT wDevID, LPDWORD dwParam1)
{
    TRACE("(%u, %p)\n", wDevID, dwParam1);
                                                                                                       
                                                                                                       
    *dwParam1 = MultiByteToWideChar(CP_ACP, 0, WInDev[wDevID].interface_name, -1,
                                    NULL, 0 ) * sizeof(WCHAR);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 *                              widDevInterface                 [internal]
 */
static DWORD widDevInterface(UINT wDevID, PWCHAR dwParam1, DWORD dwParam2)
{
    if (dwParam2 >= MultiByteToWideChar(CP_ACP, 0, WInDev[wDevID].interface_name, -1,
                                        NULL, 0 ) * sizeof(WCHAR))
    {
        MultiByteToWideChar(CP_ACP, 0, WInDev[wDevID].interface_name, -1,
                            dwParam1, dwParam2 / sizeof(WCHAR));
        return MMSYSERR_NOERROR;
    }
    return MMSYSERR_INVALPARAM;
}

/**************************************************************************
 * 			widNotifyClient			[internal]
 */
static DWORD widNotifyClient(WINE_WAVEIN* wwi, WORD wMsg, DWORD dwParam1, DWORD dwParam2)
{
    TRACE("wMsg = 0x%04x dwParm1 = %04lX dwParam2 = %04lX\n", wMsg, dwParam1, dwParam2);

    switch (wMsg) {
    case WIM_OPEN:
    case WIM_CLOSE:
    case WIM_DATA:
	if (wwi->wFlags != DCB_NULL &&
	    !DriverCallback(wwi->waveDesc.dwCallback, wwi->wFlags,
			    (HDRVR)wwi->waveDesc.hWave, wMsg,
			    wwi->waveDesc.dwInstance, dwParam1, dwParam2)) {
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
 * 			widGetDevCaps				[internal]
 */
static DWORD widGetDevCaps(WORD wDevID, LPWAVEINCAPSW lpCaps, DWORD dwSize)
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
    DWORD		bytesRead;
    int			dwBufferSpace;
    enum win_wm_message msg;
    DWORD		param;
    HANDLE		ev;

    SetEvent(wwi->hStartUpEvent);

    /* make sleep time to be # of ms to record one packet */
    dwSleepTime = ((1 << (wwi->packetSettings & 0xFFFF)) * 1000) / wwi->format.wf.nAvgBytesPerSec;
    TRACE("sleeptime=%ld ms\n", dwSleepTime);

    for(;;) {
	/* Oddly enough, dwBufferSpace is sometimes negative.... 
	 * 
	 * NOTE: If you remove this call to arts_stream_get() and
	 * remove the && (dwBufferSpace > 0) the code will still
	 * function correctly. I don't know which way is
	 * faster/better.
	 */
	dwBufferSpace = arts_stream_get(wwi->record_stream, ARTS_P_BUFFER_SPACE);
	TRACE("wwi->lpQueuePtr=(%p), wwi->state=(%d), dwBufferSpace=(%d)\n",wwi->lpQueuePtr,wwi->state,dwBufferSpace);

	/* read all data is arts input buffer. */
	if ((wwi->lpQueuePtr != NULL) && (wwi->state == WINE_WS_PLAYING) && (dwBufferSpace > 0))
	{
	    lpWaveHdr = wwi->lpQueuePtr;
		
	    TRACE("read as much as we can\n");
	    while(wwi->lpQueuePtr)
	    {
		TRACE("attempt to read %ld bytes\n",lpWaveHdr->dwBufferLength - lpWaveHdr->dwBytesRecorded);
		bytesRead = arts_read(wwi->record_stream,
				      lpWaveHdr->lpData + lpWaveHdr->dwBytesRecorded,
				      lpWaveHdr->dwBufferLength - lpWaveHdr->dwBytesRecorded);
		TRACE("bytesRead=%ld\n",bytesRead);
		if (bytesRead==0) break;
		
		lpWaveHdr->dwBytesRecorded	+= bytesRead;
		wwi->dwRecordedTotal		+= bytesRead;

		/* buffer full. notify client */
		if (lpWaveHdr->dwBytesRecorded >= lpWaveHdr->dwBufferLength)
		{
		    /* must copy the value of next waveHdr, because we have no idea of what
		     * will be done with the content of lpWaveHdr in callback
		     */
		    LPWAVEHDR	lpNext = lpWaveHdr->lpNext;

		    TRACE("waveHdr full.\n");
		    
		    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
		    lpWaveHdr->dwFlags |=  WHDR_DONE;
		    
		    widNotifyClient(wwi, WIM_DATA, (DWORD)lpWaveHdr, 0);
		    lpWaveHdr = wwi->lpQueuePtr = lpNext;
		}
	    }
	}

	/* wait for dwSleepTime or an event in thread's queue */
	WaitForSingleObject(wwi->msgRing.msg_event, dwSleepTime);

	while (ARTS_RetrieveRingMessage(&wwi->msgRing, &msg, &param, &ev))
	{
	    TRACE("msg=%s param=0x%lx\n",wodPlayerCmdString[msg - WM_USER - 1], param);
	    switch(msg) {
	    case WINE_WM_PAUSING:
		wwi->state = WINE_WS_PAUSED;

		/* Put code here to "pause" arts recording
		 */

		SetEvent(ev);
		break;
	    case WINE_WM_STARTING:
		wwi->state = WINE_WS_PLAYING;

		/* Put code here to "start" arts recording
		 */

		SetEvent(ev);
		break;
	    case WINE_WM_HEADER:
		lpWaveHdr = (LPWAVEHDR)param;
		/* insert buffer at end of queue */
		{
		    LPWAVEHDR* wh;
		    int num_headers = 0;
		    for (wh = &(wwi->lpQueuePtr); *wh; wh = &((*wh)->lpNext))
		    {
			num_headers++;

		    }
		    *wh=lpWaveHdr;
		}
		break;
	    case WINE_WM_STOPPING:
		if (wwi->state != WINE_WS_STOPPED)
		{

		    /* Put code here to "stop" arts recording
		     */

		    /* return current buffer to app */
		    lpWaveHdr = wwi->lpQueuePtr;
		    if (lpWaveHdr)
		    {
			LPWAVEHDR lpNext = lpWaveHdr->lpNext;
		        TRACE("stop %p %p\n", lpWaveHdr, lpWaveHdr->lpNext);
		        lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
		        lpWaveHdr->dwFlags |= WHDR_DONE;
		        widNotifyClient(wwi, WIM_DATA, (DWORD)lpWaveHdr, 0);
		        wwi->lpQueuePtr = lpNext;
		    }
		}
		wwi->state = WINE_WS_STOPPED;
		SetEvent(ev);
		break;
	    case WINE_WM_RESETTING:
		wwi->state = WINE_WS_STOPPED;
		wwi->dwRecordedTotal = 0;

		/* return all buffers to the app */
		for (lpWaveHdr = wwi->lpQueuePtr; lpWaveHdr; lpWaveHdr = lpWaveHdr->lpNext) {
		    TRACE("reset %p %p\n", lpWaveHdr, lpWaveHdr->lpNext);
		    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
		    lpWaveHdr->dwFlags |= WHDR_DONE;

		    widNotifyClient(wwi, WIM_DATA, (DWORD)lpWaveHdr, 0);
		}
		wwi->lpQueuePtr = NULL; 
		SetEvent(ev);
		break;
	    case WINE_WM_CLOSING:
		wwi->hThread = 0;
		wwi->state = WINE_WS_CLOSED;
		SetEvent(ev);
		ExitThread(0);
		/* shouldn't go here */
	    default:
		FIXME("unknown message %d\n", msg);
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
    WINE_WAVEIN*	wwi;

    TRACE("(%u, %p %08lX);\n",wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN("Invalid Parametr (lpDesc == NULL)!\n");
	return MMSYSERR_INVALPARAM;
    }

    if (wDevID >= MAX_WAVEINDRV) {
	TRACE ("MAX_WAVEINDRV reached !\n");
	return MMSYSERR_BADDEVICEID;
    }

    /* if this device is already open tell the app that it is allocated */
    if(WInDev[wDevID].record_stream != (arts_stream_t*)-1)
    {
	TRACE("device already allocated\n");
	return MMSYSERR_ALLOCATED;
    }

    /* only PCM format is support so far... */
    if (lpDesc->lpFormat->wFormatTag != WAVE_FORMAT_PCM ||
	lpDesc->lpFormat->nChannels == 0 ||
        lpDesc->lpFormat->nSamplesPerSec < DSBFREQUENCY_MIN ||
        lpDesc->lpFormat->nSamplesPerSec > DSBFREQUENCY_MAX ||
        (lpDesc->lpFormat->wBitsPerSample!=8 && lpDesc->lpFormat->wBitsPerSample!=16)) {
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

    /* direct sound not supported, ignore the flag */
    dwFlags &= ~WAVE_DIRECTSOUND;

    wwi->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
    
    memcpy(&wwi->waveDesc, lpDesc,           sizeof(WAVEOPENDESC));
    memcpy(&wwi->format,   lpDesc->lpFormat, sizeof(PCMWAVEFORMAT));

    if (wwi->format.wBitsPerSample == 0) {
	WARN("Resetting zerod wBitsPerSample\n");
	wwi->format.wBitsPerSample = 8 *
	    (wwi->format.wf.nAvgBytesPerSec /
	     wwi->format.wf.nSamplesPerSec) /
	    wwi->format.wf.nChannels;
    }

    wwi->record_stream = arts_record_stream(wwi->format.wf.nSamplesPerSec,
					    wwi->format.wBitsPerSample,
					    wwi->format.wf.nChannels,
					    "winearts");
    TRACE("(wwi->record_stream=%p)\n",wwi->record_stream);
    wwi->state = WINE_WS_STOPPED;

    wwi->packetSettings = arts_stream_set(wwi->record_stream, ARTS_P_PACKET_SETTINGS, WAVEIN_PACKET_SETTINGS);
    TRACE("Tried to set ARTS_P_PACKET_SETTINGS to (%x), actually set to (%x)\n", WAVEIN_PACKET_SETTINGS, wwi->packetSettings);
    TRACE("Buffer size is now (%d)\n", arts_stream_get(wwi->record_stream, ARTS_P_BUFFER_SIZE));

    if (wwi->lpQueuePtr) {
	WARN("Should have an empty queue (%p)\n", wwi->lpQueuePtr);
	wwi->lpQueuePtr = NULL;
    }
    arts_stream_set(wwi->record_stream, ARTS_P_BLOCKING, 0);    /* disable blocking on this stream */

    if(!wwi->record_stream) return MMSYSERR_ALLOCATED;

    wwi->dwRecordedTotal = 0;
    wwi->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    ARTS_InitRingMessage(&wwi->msgRing);

    /* create recorder thread */
    if (!(dwFlags & WAVE_DIRECTSOUND)) {
	wwi->hStartUpEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
	wwi->hThread = CreateThread(NULL, 0, widRecorder, (LPVOID)(DWORD)wDevID, 0, &(wwi->dwThreadID));
        if (wwi->hThread)
            SetThreadPriority(wwi->hThread, THREAD_PRIORITY_TIME_CRITICAL);
	WaitForSingleObject(wwi->hStartUpEvent, INFINITE);
	CloseHandle(wwi->hStartUpEvent);
    } else {
	wwi->hThread = INVALID_HANDLE_VALUE;
	wwi->dwThreadID = 0;
    }
    wwi->hStartUpEvent = INVALID_HANDLE_VALUE;

    TRACE("wBitsPerSample=%u, nAvgBytesPerSec=%lu, nSamplesPerSec=%lu, nChannels=%u nBlockAlign=%u!\n",
	  wwi->format.wBitsPerSample, wwi->format.wf.nAvgBytesPerSec,
	  wwi->format.wf.nSamplesPerSec, wwi->format.wf.nChannels,
	  wwi->format.wf.nBlockAlign);
    return widNotifyClient(wwi, WIM_OPEN, 0L, 0L);
}

/**************************************************************************
 * 				widClose			[internal]
 */
static DWORD widClose(WORD wDevID)
{
    WINE_WAVEIN*	wwi;

    TRACE("(%u);\n", wDevID);
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("can't close !\n");
	return MMSYSERR_INVALHANDLE;
    }

    wwi = &WInDev[wDevID];

    if (wwi->lpQueuePtr != NULL) {
	WARN("still buffers open !\n");
	return WAVERR_STILLPLAYING;
    }

    ARTS_AddRingMessage(&wwi->msgRing, WINE_WM_CLOSING, 0, TRUE);
    ARTS_CloseWaveInDevice(wwi);
    wwi->state = WINE_WS_CLOSED;
    ARTS_DestroyRingMessage(&wwi->msgRing);
    return widNotifyClient(wwi, WIM_CLOSE, 0L, 0L);
}

/**************************************************************************
 * 				widAddBuffer		[internal]
 */
static DWORD widAddBuffer(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);

    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].state == WINE_WS_CLOSED) {
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

    ARTS_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_HEADER, (DWORD)lpWaveHdr, FALSE);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widStart				[internal]
 */
static DWORD widStart(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("can't start recording !\n");
	return MMSYSERR_INVALHANDLE;
    }

    ARTS_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_STARTING, 0, TRUE);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widStop					[internal]
 */
static DWORD widStop(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("can't stop !\n");
	return MMSYSERR_INVALHANDLE;
    }

    ARTS_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_STOPPING, 0, TRUE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widReset				[internal]
 */
static DWORD widReset(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("can't reset !\n");
	return MMSYSERR_INVALHANDLE;
    }
    ARTS_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_RESETTING, 0, TRUE);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widMessage (WINEARTS.6)
 */
DWORD WINAPI ARTS_widMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
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
    case WIDM_OPEN:	 	return widOpen		(wDevID, (LPWAVEOPENDESC)dwParam1,	dwParam2);
    case WIDM_CLOSE:		return widClose		(wDevID);
    case WIDM_ADDBUFFER:	return widAddBuffer	(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WIDM_PREPARE:		return MMSYSERR_NOTSUPPORTED;
    case WIDM_UNPREPARE:	return MMSYSERR_NOTSUPPORTED;
    case WIDM_GETDEVCAPS:	return widGetDevCaps	(wDevID, (LPWAVEINCAPSW)dwParam1,	dwParam2);
    case WIDM_GETNUMDEVS:	return widGetNumDevs	();
    case WIDM_RESET:		return widReset		(wDevID);
    case WIDM_START:		return widStart		(wDevID);
    case WIDM_STOP:		return widStop		(wDevID);
    case DRV_QUERYDEVICEINTERFACESIZE: return widDevInterfaceSize       (wDevID, (LPDWORD)dwParam1);
    case DRV_QUERYDEVICEINTERFACE:     return widDevInterface           (wDevID, (PWCHAR)dwParam1, dwParam2);
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
	interface with arts like we do with oss */
    MESSAGE("This sound card's driver does not support direct access\n");
    MESSAGE("The (slower) DirectSound HEL mode will be used instead.\n");
    return MMSYSERR_NOTSUPPORTED;
}

static DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc)
{
    memset(desc, 0, sizeof(*desc));
    strcpy(desc->szDesc, "Wine aRts DirectSound Driver");
    strcpy(desc->szDrvname, "winearts.drv");
    return MMSYSERR_NOERROR;
}

#else /* !HAVE_ARTS */

/**************************************************************************
 * 				wodMessage (WINEARTS.@)
 */
DWORD WINAPI ARTS_wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08lX, %08lX, %08lX):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				widMessage (WINEARTS.6)
 */
DWORD WINAPI ARTS_widMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08lX, %08lX, %08lX):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

#endif /* HAVE_ARTS */
