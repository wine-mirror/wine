/*
 * Wine Driver for EsounD Sound Server
 * http://www.tux.org/~ricdude/EsounD.html
 *
 * Copyright 1994 Martin Ayotte
 *           1999 Eric Pouech (async playing in waveOut/waveIn)
 *	     2000 Eric Pouech (loops in waveOut)
 *	     2004 Zhangrong Huang (EsounD version of this file)
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
 *    with esd we cannot stop the audio that is already in
 *    the server's buffer.
 *
 * FIXME:
 *	pause in waveOut does not work correctly in loop mode
 *
 *	does something need to be done in for WaveIn DirectSound?
 *
 */

#include "config.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "wine/winuser16.h"
#include "mmddk.h"
#include "mmreg.h"
#include "dsound.h"
#include "dsdriver.h"
#include "ks.h"
#include "ksguid.h"
#include "ksmedia.h"
#include "esound.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wave);

#ifdef HAVE_ESD

#include <esd.h>

/* unless someone makes a wineserver kernel module, Unix pipes are faster than win32 events */
#define USE_PIPE_SYNC

/* define if you want to use esd_monitor_stream instead of
 * esd_record_stream for waveIn stream
 */
/*#define WID_USE_ESDMON*/

#define BUFFER_REFILL_THRESHOLD 4

#define MAX_WAVEOUTDRV 	(10)
#define MAX_WAVEINDRV 	(10)
#define MAX_CHANNELS	2

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

#ifdef USE_PIPE_SYNC
#define SIGNAL_OMR(mr) do { int x = 0; write((mr)->msg_pipe[1], &x, sizeof(x)); } while (0)
#define CLEAR_OMR(mr) do { int x = 0; read((mr)->msg_pipe[0], &x, sizeof(x)); } while (0)
#define RESET_OMR(mr) do { } while (0)
#define WAIT_OMR(mr, sleep) \
  do { struct pollfd pfd; pfd.fd = (mr)->msg_pipe[0]; \
       pfd.events = POLLIN; poll(&pfd, 1, sleep); } while (0)
#else
#define SIGNAL_OMR(mr) do { SetEvent((mr)->msg_event); } while (0)
#define CLEAR_OMR(mr) do { } while (0)
#define RESET_OMR(mr) do { ResetEvent((mr)->msg_event); } while (0)
#define WAIT_OMR(mr, sleep) \
  do { WaitForSingleObject((mr)->msg_event, sleep); } while (0)
#endif

typedef struct {
    enum win_wm_message 	msg;	/* message identifier */
    DWORD	                param;  /* parameter for this message */
    HANDLE	                hEvent;	/* if message is synchronous, handle of event for synchro */
} RING_MSG;

/* implement an in-process message ring for better performance
 * (compared to passing thru the server)
 * this ring will be used by the input (resp output) record (resp playback) routine
 */
#define ESD_RING_BUFFER_INCREMENT      64
typedef struct {
    RING_MSG			* messages;
    int                         ring_buffer_size;
    int				msg_tosave;
    int				msg_toget;
#ifdef USE_PIPE_SYNC
    int                         msg_pipe[2];
#else
    HANDLE                      msg_event;
#endif
    CRITICAL_SECTION		msg_crst;
} ESD_MSG_RING;

typedef struct {
    volatile int		state;			/* one of the WINE_WS_ manifest constants */
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    WAVEFORMATPCMEX             waveFormat;
    WAVEOUTCAPSW		caps;
    char                        interface_name[32];

    DWORD			dwSleepTime;		/* Num of milliseconds to sleep between filling the dsp buffers */

    /* esd information */
    int				esd_fd;		/* the socket fd we get from esd when opening a stream for playing */
    int				bytes_per_frame;
    DWORD                       dwBufferSize;           /* size of whole buffer in bytes */

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
    ESD_MSG_RING		msgRing;
} WINE_WAVEOUT;

typedef struct {
    volatile int		state;			/* one of the WINE_WS_ manifest constants */
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    WAVEFORMATPCMEX             waveFormat;
    WAVEINCAPSW			caps;
    char                        interface_name[32];

    /* esd information */
    int				esd_fd;		/* the socket fd we get from esd when opening a stream for recording */
    int				bytes_per_frame;

    LPWAVEHDR			lpQueuePtr;
    DWORD			dwRecordedTotal;

    /* synchronization stuff */
    HANDLE			hStartUpEvent;
    HANDLE			hThread;
    DWORD			dwThreadID;
    ESD_MSG_RING		msgRing;
} WINE_WAVEIN;

static char* esd_host;	/* the esd host */

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

/*======================================================================*
 *                  Low level WAVE implementation			*
 *======================================================================*/

/* Volume functions derived from Alsaplayer source */
/* length is the number of 16 bit samples */
void volume_effect16(void *bufin, void* bufout, int length, int left,
		int right, int 	nChannels)
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

static DWORD bytes_to_mmtime(LPMMTIME lpTime, DWORD position,
                             WAVEFORMATPCMEX* format)
{
    TRACE("wType=%04X wBitsPerSample=%u nSamplesPerSec=%u nChannels=%u nAvgBytesPerSec=%u\n",
          lpTime->wType, format->Format.wBitsPerSample, format->Format.nSamplesPerSec,
          format->Format.nChannels, format->Format.nAvgBytesPerSec);
    TRACE("Position in bytes=%u\n", position);

    switch (lpTime->wType) {
    case TIME_SAMPLES:
        lpTime->u.sample = position / (format->Format.wBitsPerSample / 8 * format->Format.nChannels);
        TRACE("TIME_SAMPLES=%u\n", lpTime->u.sample);
        break;
    case TIME_MS:
        lpTime->u.ms = 1000.0 * position / (format->Format.wBitsPerSample / 8 * format->Format.nChannels * format->Format.nSamplesPerSec);
        TRACE("TIME_MS=%u\n", lpTime->u.ms);
        break;
    case TIME_SMPTE:
        lpTime->u.smpte.fps = 30;
        position = position / (format->Format.wBitsPerSample / 8 * format->Format.nChannels);
        position += (format->Format.nSamplesPerSec / lpTime->u.smpte.fps) - 1; /* round up */
        lpTime->u.smpte.sec = position / format->Format.nSamplesPerSec;
        position -= lpTime->u.smpte.sec * format->Format.nSamplesPerSec;
        lpTime->u.smpte.min = lpTime->u.smpte.sec / 60;
        lpTime->u.smpte.sec -= 60 * lpTime->u.smpte.min;
        lpTime->u.smpte.hour = lpTime->u.smpte.min / 60;
        lpTime->u.smpte.min -= 60 * lpTime->u.smpte.hour;
        lpTime->u.smpte.fps = 30;
        lpTime->u.smpte.frame = position * lpTime->u.smpte.fps / format->Format.nSamplesPerSec;
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

static BOOL supportedFormat(LPWAVEFORMATEX wf)
{
    TRACE("(%p)\n",wf);
                                                                                
    if (wf->nSamplesPerSec<DSBFREQUENCY_MIN||wf->nSamplesPerSec>DSBFREQUENCY_MAX)
        return FALSE;
                                                                                
    if (wf->wFormatTag == WAVE_FORMAT_PCM) {
        if (wf->nChannels >= 1 && wf->nChannels <= MAX_CHANNELS) {
            if (wf->wBitsPerSample==8||wf->wBitsPerSample==16)
                return TRUE;
        }
    } else if (wf->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        WAVEFORMATEXTENSIBLE * wfex = (WAVEFORMATEXTENSIBLE *)wf;
                                                                                
        if (wf->cbSize == 22 && IsEqualGUID(&wfex->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)) {
            if (wf->nChannels >=1 && wf->nChannels <= MAX_CHANNELS) {
                if (wf->wBitsPerSample==wfex->Samples.wValidBitsPerSample) {
                    if (wf->wBitsPerSample==8||wf->wBitsPerSample==16)
                        return TRUE;
                } else
                    WARN("wBitsPerSample != wValidBitsPerSample not supported yet\n");
            }
        } else
            WARN("only KSDATAFORMAT_SUBTYPE_PCM supported\n");
    } else
        WARN("only WAVE_FORMAT_PCM and WAVE_FORMAT_EXTENSIBLE supported\n");
                                                                                
    return FALSE;
}

void copy_format(LPWAVEFORMATEX wf1, LPWAVEFORMATPCMEX wf2)
{
    ZeroMemory(wf2, sizeof(wf2));
    if (wf1->wFormatTag == WAVE_FORMAT_PCM)
        memcpy(wf2, wf1, sizeof(PCMWAVEFORMAT));
    else if (wf1->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        memcpy(wf2, wf1, sizeof(WAVEFORMATPCMEX));
    else
        memcpy(wf2, wf1, sizeof(WAVEFORMATEX) + wf1->cbSize);
}

/******************************************************************
 *		ESD_CloseWaveOutDevice
 *
 */
void		ESD_CloseWaveOutDevice(WINE_WAVEOUT* wwo)
{
	esd_close(wwo->esd_fd); 	/* close the esd socket fd */
	wwo->esd_fd = -1;

  /* free up the buffer we use for volume and reset the size */
  HeapFree(GetProcessHeap(), 0, wwo->sound_buffer);
  wwo->sound_buffer = NULL;
  wwo->buffer_size = 0;
}

/******************************************************************
 *		ESD_CloseWaveInDevice
 *
 */
void		ESD_CloseWaveInDevice(WINE_WAVEIN* wwi)
{
	esd_close(wwi->esd_fd); 	/* close the esd socket fd */
	wwi->esd_fd = -1;
}

/******************************************************************
 *		ESD_WaveClose
 */
LONG		ESD_WaveClose(void)
{
    int iDevice;

    /* close all open devices */
    for(iDevice = 0; iDevice < MAX_WAVEOUTDRV; iDevice++)
    {
      if(WOutDev[iDevice].esd_fd != -1)
      {
        ESD_CloseWaveOutDevice(&WOutDev[iDevice]);
      }
    }

    for(iDevice = 0; iDevice < MAX_WAVEINDRV; iDevice++)
    {
      if(WInDev[iDevice].esd_fd != -1)
      {
        ESD_CloseWaveInDevice(&WInDev[iDevice]);
      }
    }

    return 1;
}

/******************************************************************
 *		ESD_WaveInit
 *
 * Initialize internal structures from ESD server info
 */
LONG ESD_WaveInit(void)
{
    int 	i;
	int 	fd;

    TRACE("called\n");

    /* FIXME: Maybe usefully to set the esd host. */
    esd_host = NULL;

    /* Testing whether the esd host is alive. */
    if ((fd = esd_open_sound(esd_host)) < 0)
    {
	WARN("esd_open_sound() failed (%d)\n", errno);
	return -1;
    }
    esd_close(fd);

    /* initialize all device handles to -1 */
    for (i = 0; i < MAX_WAVEOUTDRV; ++i)
    {
        static const WCHAR ini[] = {'E','s','o','u','n','D',' ','W','a','v','e','O','u','t','D','r','i','v','e','r',0};

	WOutDev[i].esd_fd = -1;
	memset(&WOutDev[i].caps, 0, sizeof(WOutDev[i].caps)); /* zero out
							caps values */
	WOutDev[i].caps.wMid = 0x00FF; 	/* Manufacturer ID */
    	WOutDev[i].caps.wPid = 0x0001; 	/* Product ID */
    	lstrcpyW(WOutDev[i].caps.szPname, ini);
        snprintf(WOutDev[i].interface_name, sizeof(WOutDev[i].interface_name), "wineesd: %d", i);

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
        static const WCHAR ini[] = {'E','s','o','u','n','D',' ','W','a','v','e','I','n','D','r','i','v','e','r',0};

	WInDev[i].esd_fd = -1;
	memset(&WInDev[i].caps, 0, sizeof(WInDev[i].caps)); /* zero out
							caps values */
	WInDev[i].caps.wMid = 0x00FF;
	WInDev[i].caps.wPid = 0x0001;
	lstrcpyW(WInDev[i].caps.szPname, ini);
        snprintf(WInDev[i].interface_name, sizeof(WInDev[i].interface_name), "wineesd: %d", i);

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
 *		ESD_InitRingMessage
 *
 * Initialize the ring of messages for passing between driver's caller and playback/record
 * thread
 */
static int ESD_InitRingMessage(ESD_MSG_RING* mr)
{
    mr->msg_toget = 0;
    mr->msg_tosave = 0;
#ifdef USE_PIPE_SYNC
    if (pipe(mr->msg_pipe) < 0) {
        mr->msg_pipe[0] = -1;
        mr->msg_pipe[1] = -1;
        ERR("could not create pipe, error=%s\n", strerror(errno));
    }
#else
    mr->msg_event = CreateEventW(NULL, FALSE, FALSE, NULL);
#endif
    mr->ring_buffer_size = ESD_RING_BUFFER_INCREMENT;
    mr->messages = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,mr->ring_buffer_size * sizeof(RING_MSG));
    InitializeCriticalSection(&mr->msg_crst);
    mr->msg_crst.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ESD_MSG_RING.msg_crst");
    return 0;
}

/******************************************************************
 *		ESD_DestroyRingMessage
 *
 */
static int ESD_DestroyRingMessage(ESD_MSG_RING* mr)
{
#ifdef USE_PIPE_SYNC
    close(mr->msg_pipe[0]);
    close(mr->msg_pipe[1]);
#else
    CloseHandle(mr->msg_event);
#endif
    HeapFree(GetProcessHeap(),0,mr->messages);
    mr->messages=NULL;
    mr->msg_crst.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&mr->msg_crst);
    return 0;
}

/******************************************************************
 *		ESD_AddRingMessage
 *
 * Inserts a new message into the ring (should be called from DriverProc derived routines)
 */
static int ESD_AddRingMessage(ESD_MSG_RING* mr, enum win_wm_message msg, DWORD param, BOOL wait)
{
    HANDLE      hEvent = INVALID_HANDLE_VALUE;

    EnterCriticalSection(&mr->msg_crst);
    if ((mr->msg_toget == ((mr->msg_tosave + 1) % mr->ring_buffer_size)))
    {
	int old_ring_buffer_size = mr->ring_buffer_size;
	mr->ring_buffer_size += ESD_RING_BUFFER_INCREMENT;
	TRACE("mr->ring_buffer_size=%d\n",mr->ring_buffer_size);
	mr->messages = HeapReAlloc(GetProcessHeap(),0,mr->messages, mr->ring_buffer_size * sizeof(RING_MSG));
	/* Now we need to rearrange the ring buffer so that the new
	   buffers just allocated are in between mr->msg_tosave and
	   mr->msg_toget.
	*/
	if (mr->msg_tosave < mr->msg_toget)
	{
	    memmove(&(mr->messages[mr->msg_toget + ESD_RING_BUFFER_INCREMENT]),
		    &(mr->messages[mr->msg_toget]),
		    sizeof(RING_MSG)*(old_ring_buffer_size - mr->msg_toget)
		    );
	    mr->msg_toget += ESD_RING_BUFFER_INCREMENT;
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

    /* signal a new message */
    SIGNAL_OMR(mr);
    if (wait)
    {
        /* wait for playback/record thread to have processed the message */
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
    }

    return 1;
}

/******************************************************************
 *		ESD_RetrieveRingMessage
 *
 * Get a message from the ring. Should be called by the playback/record thread.
 */
static int ESD_RetrieveRingMessage(ESD_MSG_RING* mr,
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
    CLEAR_OMR(mr);
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
    /* total played is the bytes written less the bytes to write ;-) */
    wwo->dwPlayedTotal = wwo->dwWrittenTotal;

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
        dwMillis = (lpWaveHdr->reserved - wwo->dwPlayedTotal) * 1000 / wwo->waveFormat.Format.nAvgBytesPerSec;
	if(!dwMillis) dwMillis = 1;
    }

    TRACE("dwMillis = %d\n", dwMillis);

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

    TRACE("Writing wavehdr %p.%u[%u]\n",
          wwo->lpPlayPtr, wwo->dwPartialOffset, wwo->lpPlayPtr->dwBufferLength);

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
    if(wwo->waveFormat.Format.wBitsPerSample == 16)
    {
      /* apply volume to the buffer we are about to send */
      /* divide toWrite(bytes) by 2 as volume processes by 16 bits */
      volume_effect16(wwo->lpPlayPtr->lpData + wwo->dwPartialOffset,
                wwo->sound_buffer, toWrite>>1, wwo->volume_left,
		wwo->volume_right, wwo->waveFormat.Format.nChannels);
    } else if(wwo->waveFormat.Format.wBitsPerSample == 8)
    {
      /* apply volume to the buffer we are about to send */
      volume_effect8(wwo->lpPlayPtr->lpData + wwo->dwPartialOffset,
                wwo->sound_buffer, toWrite, wwo->volume_left,
		wwo->volume_right, wwo->waveFormat.Format.nChannels);
    } else
    {
      FIXME("unsupported wwo->format.wBitsPerSample of %d\n",
        wwo->waveFormat.Format.wBitsPerSample);
    }

    /* send the audio data to esd for playing */
    written = write(wwo->esd_fd, wwo->sound_buffer, toWrite);

    TRACE("written = %d\n", written);

    if (written <= 0) 
    {
      *bytes = 0; /* apparently esd is actually full */
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
	TRACE("lpWaveHdr=(%p), lpPlayPtr=(%p), lpLoopPtr=(%p), reserved=(%d), dwWrittenTotal=(%d), force=(%d)\n",
	      wwo->lpQueuePtr,
	      wwo->lpPlayPtr,
	      wwo->lpLoopPtr,
	      wwo->lpQueuePtr->reserved,
	      wwo->dwWrittenTotal,
	      force);
    } else {
	TRACE("lpWaveHdr=(%p), lpPlayPtr=(%p), lpLoopPtr=(%p),  dwWrittenTotal=(%d), force=(%d)\n",
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
    /* to esd, otherwise we would do the flushing here */

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
        while (ESD_RetrieveRingMessage(&wwo->msgRing, &msg, &param, &ev))
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
        RESET_OMR(&wwo->msgRing);
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

    while (ESD_RetrieveRingMessage(&wwo->msgRing, &msg, &param, &ev)) {
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
 * 			     wodPlayer_FeedDSP			[internal]
 * Feed as much sound data as we can into the DSP and return the number of
 * milliseconds before it will be necessary to feed the DSP again.
 */
static DWORD wodPlayer_FeedDSP(WINE_WAVEOUT* wwo)
{
    DWORD       availInQ;

    wodUpdatePlayedTotal(wwo);
    /* better way to set availInQ? */
    availInQ = ESD_BUF_SIZE;
    TRACE("availInQ = %d\n", availInQ);

    /* input queue empty */
    if (!wwo->lpPlayPtr) {
        TRACE("Run out of wavehdr:s... flushing\n");
        return INFINITE;
    }

#if 0
    /* no more room... no need to try to feed */
    if(!availInQ)
    {
	TRACE("no more room, no need to try to feed\n");
	return wwo->dwSleepTime;
    }
#endif

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
            TRACE("reserved=(%d) dwWrittenTotal=(%d) dwBufferLength=(%d)\n",
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
        TRACE("waiting %ums (%u,%u)\n", dwSleepTime, dwNextFeedTime, dwNextNotifyTime);
        WAIT_OMR(&wwo->msgRing, dwSleepTime);
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
    /* output to esound... */
    int			out_bits = ESD_BITS8, out_channels = ESD_MONO, out_rate;
    int			out_mode = ESD_STREAM, out_func = ESD_PLAY;
    esd_format_t	out_format;

    TRACE("(%u, %p, %08X);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN("Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_WAVEOUTDRV) {
	TRACE("MAX_WAVOUTDRV reached !\n");
	return MMSYSERR_BADDEVICEID;
    }

    /* if this device is already open tell the app that it is allocated */
    if(WOutDev[wDevID].esd_fd != -1)
    {
      TRACE("device already allocated\n");
      return MMSYSERR_ALLOCATED;
    }

    /* only PCM format is supported so far... */
    if (!supportedFormat(lpDesc->lpFormat)) {
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

    /* direct sound not supported, ignore the flag */
    dwFlags &= ~WAVE_DIRECTSOUND;

    wwo->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    wwo->waveDesc = *lpDesc;
    copy_format(lpDesc->lpFormat, &wwo->waveFormat);

    if (wwo->waveFormat.Format.wBitsPerSample == 0) {
	WARN("Resetting zeroed wBitsPerSample\n");
	wwo->waveFormat.Format.wBitsPerSample = 8 *
	    (wwo->waveFormat.Format.nAvgBytesPerSec /
	     wwo->waveFormat.Format.nSamplesPerSec) /
	    wwo->waveFormat.Format.nChannels;
    }

    if (wwo->waveFormat.Format.wBitsPerSample == 8)
	out_bits = ESD_BITS8;
    else if (wwo->waveFormat.Format.wBitsPerSample == 16)
	out_bits = ESD_BITS16;

    wwo->bytes_per_frame = (wwo->waveFormat.Format.wBitsPerSample * wwo->waveFormat.Format.nChannels) / 8;

    if (wwo->waveFormat.Format.nChannels == 1)
	out_channels = ESD_MONO;
    else if (wwo->waveFormat.Format.nChannels == 2)
	out_channels = ESD_STEREO;

    out_format = out_bits | out_channels | out_mode | out_func;
    out_rate = (int) wwo->waveFormat.Format.nSamplesPerSec;
	TRACE("esd output format = 0x%08x, rate = %d\n", out_format, out_rate);

    wwo->esd_fd = esd_play_stream(out_format, out_rate, esd_host, "wineesd");

    /* clear these so we don't have any confusion ;-) */
    wwo->sound_buffer = 0;
    wwo->buffer_size = 0;

    if(wwo->esd_fd < 0) return MMSYSERR_ALLOCATED;

    wwo->dwBufferSize = ESD_BUF_SIZE;
    TRACE("Buffer size is now (%d)\n",wwo->dwBufferSize);

    wwo->dwPlayedTotal = 0;
    wwo->dwWrittenTotal = 0;

    wwo->dwSleepTime = (1024 * 1000 * BUFFER_REFILL_THRESHOLD) / wwo->waveFormat.Format.nAvgBytesPerSec;

    /* Initialize volume to full level */
    wwo->volume_left = 100;
    wwo->volume_right = 100;

    ESD_InitRingMessage(&wwo->msgRing);

    /* create player thread */
    if (!(dwFlags & WAVE_DIRECTSOUND)) {
	wwo->hStartUpEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
	wwo->hThread = CreateThread(NULL, 0, wodPlayer, (LPVOID)(DWORD)wDevID, 0, &(wwo->dwThreadID));
	WaitForSingleObject(wwo->hStartUpEvent, INFINITE);
	CloseHandle(wwo->hStartUpEvent);
    } else {
	wwo->hThread = INVALID_HANDLE_VALUE;
	wwo->dwThreadID = 0;
    }
    wwo->hStartUpEvent = INVALID_HANDLE_VALUE;

    TRACE("esd=0x%lx, dwBufferSize=%d\n",
	  (long)wwo->esd_fd, wwo->dwBufferSize);

    TRACE("wBitsPerSample=%u, nAvgBytesPerSec=%u, nSamplesPerSec=%u, nChannels=%u nBlockAlign=%u!\n",
	  wwo->waveFormat.Format.wBitsPerSample, wwo->waveFormat.Format.nAvgBytesPerSec,
	  wwo->waveFormat.Format.nSamplesPerSec, wwo->waveFormat.Format.nChannels,
	  wwo->waveFormat.Format.nBlockAlign);

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

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].esd_fd == -1) {
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
	    ESD_AddRingMessage(&wwo->msgRing, WINE_WM_CLOSING, 0, TRUE);
	}

        ESD_DestroyRingMessage(&wwo->msgRing);

	ESD_CloseWaveOutDevice(wwo);	/* close the stream and clean things up */

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
    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].esd_fd == -1) {
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
    ESD_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_HEADER, (DWORD)lpWaveHdr, FALSE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodPause				[internal]
 */
static DWORD wodPause(WORD wDevID)
{
    TRACE("(%u);!\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].esd_fd == -1) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    TRACE("imhere[3-PAUSING]\n");
    ESD_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_PAUSING, 0, TRUE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodRestart				[internal]
 */
static DWORD wodRestart(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].esd_fd == -1) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    if (WOutDev[wDevID].state == WINE_WS_PAUSED) {
	TRACE("imhere[3-RESTARTING]\n");
	ESD_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_RESTARTING, 0, TRUE);
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

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].esd_fd == -1) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    TRACE("imhere[3-RESET]\n");
    ESD_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_RESETTING, 0, TRUE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodGetPosition			[internal]
 */
static DWORD wodGetPosition(WORD wDevID, LPMMTIME lpTime, DWORD uSize)
{
    WINE_WAVEOUT*	wwo;

    TRACE("(%u, %p, %u);\n", wDevID, lpTime, uSize);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].esd_fd == -1) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    if (lpTime == NULL)	{
        WARN("invalid parameter: lpTime == NULL\n");
        return MMSYSERR_INVALPARAM;
    }

    wwo = &WOutDev[wDevID];
    ESD_AddRingMessage(&wwo->msgRing, WINE_WM_UPDATE, 0, TRUE);

    return bytes_to_mmtime(lpTime, wwo->dwPlayedTotal, &wwo->waveFormat);
}

/**************************************************************************
 * 				wodBreakLoop			[internal]
 */
static DWORD wodBreakLoop(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].esd_fd == -1) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    ESD_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_BREAKLOOP, 0, TRUE);
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
 * 				wodMessage (WINEESD.@)
 */
DWORD WINAPI ESD_wodMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
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
    TRACE("wMsg = 0x%04x dwParm1 = %04X dwParam2 = %04X\n", wMsg, dwParam1, dwParam2);

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
    DWORD		bytesRead;
    enum win_wm_message msg;
    DWORD		param;
    HANDLE		ev;

    SetEvent(wwi->hStartUpEvent);

    /* make sleep time to be # of ms to record one packet */
    dwSleepTime = (1024 * 1000) / wwi->waveFormat.Format.nAvgBytesPerSec;
    TRACE("sleeptime=%d ms\n", dwSleepTime);

    for(;;) {
	TRACE("wwi->lpQueuePtr=(%p), wwi->state=(%d)\n",wwi->lpQueuePtr,wwi->state);

	/* read all data is esd input buffer. */
	if ((wwi->lpQueuePtr != NULL) && (wwi->state == WINE_WS_PLAYING))
	{
	    lpWaveHdr = wwi->lpQueuePtr;
 
	    TRACE("read as much as we can\n");
	    while(wwi->lpQueuePtr)
	    {
		TRACE("attempt to read %d bytes\n",lpWaveHdr->dwBufferLength - lpWaveHdr->dwBytesRecorded);
		bytesRead = read(wwi->esd_fd,
			      lpWaveHdr->lpData + lpWaveHdr->dwBytesRecorded,
			      lpWaveHdr->dwBufferLength - lpWaveHdr->dwBytesRecorded);
		TRACE("bytesRead=%d\n",bytesRead);
		if (bytesRead == -1 && errno == EAGAIN)
			bytesRead = 0;
		if (bytesRead==0) break; /* So we can stop recording smoothly */
		if (bytesRead < 0)
			bytesRead = 0;
 
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
	WAIT_OMR(&wwi->msgRing, dwSleepTime);

	while (ESD_RetrieveRingMessage(&wwi->msgRing, &msg, &param, &ev))
	{
            TRACE("msg=%s param=0x%x\n",wodPlayerCmdString[msg - WM_USER - 1], param);
	    switch(msg) {
	    case WINE_WM_PAUSING:
		wwi->state = WINE_WS_PAUSED;

		/* Put code here to "pause" esd recording
		 */

		SetEvent(ev);
		break;
	    case WINE_WM_STARTING:
		wwi->state = WINE_WS_PLAYING;

		/* Put code here to "start" esd recording
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

		    /* Put code here to "stop" esd recording
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
    /* input esound... */
    int			in_bits = ESD_BITS16, in_channels = ESD_STEREO, in_rate;
#ifdef WID_USE_ESDMON
    int			in_mode = ESD_STREAM, in_func = ESD_PLAY;
#else
    int			in_mode = ESD_STREAM, in_func = ESD_RECORD;
#endif
    esd_format_t	in_format;
    int			mode;

    TRACE("(%u, %p %08X);\n",wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN("Invalid Parametr (lpDesc == NULL)!\n");
	return MMSYSERR_INVALPARAM;
    }

    if (wDevID >= MAX_WAVEINDRV) {
	TRACE ("MAX_WAVEINDRV reached !\n");
	return MMSYSERR_BADDEVICEID;
    }

    /* if this device is already open tell the app that it is allocated */
    if(WInDev[wDevID].esd_fd != -1)
    {
	TRACE("device already allocated\n");
	return MMSYSERR_ALLOCATED;
    }

    /* only PCM format is support so far... */
    if (!supportedFormat(lpDesc->lpFormat)) {
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

    wwi = &WInDev[wDevID];

    /* direct sound not supported, ignore the flag */
    dwFlags &= ~WAVE_DIRECTSOUND;

    wwi->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    wwi->waveDesc = *lpDesc;
    copy_format(lpDesc->lpFormat, &wwi->waveFormat);

    if (wwi->waveFormat.Format.wBitsPerSample == 0) {
	WARN("Resetting zerod wBitsPerSample\n");
	wwi->waveFormat.Format.wBitsPerSample = 8 *
	    (wwi->waveFormat.Format.nAvgBytesPerSec /
	     wwi->waveFormat.Format.nSamplesPerSec) /
	    wwi->waveFormat.Format.nChannels;
    }

    if (wwi->waveFormat.Format.wBitsPerSample == 8)
	in_bits = ESD_BITS8;
    else if (wwi->waveFormat.Format.wBitsPerSample == 16)
	in_bits = ESD_BITS16;

    wwi->bytes_per_frame = (wwi->waveFormat.Format.wBitsPerSample * wwi->waveFormat.Format.nChannels) / 8;

    if (wwi->waveFormat.Format.nChannels == 1)
	in_channels = ESD_MONO;
    else if (wwi->waveFormat.Format.nChannels == 2)
	in_channels = ESD_STEREO;

    in_format = in_bits | in_channels | in_mode | in_func;
    in_rate = (int) wwi->waveFormat.Format.nSamplesPerSec;
	TRACE("esd input format = 0x%08x, rate = %d\n", in_format, in_rate);

#ifdef WID_USE_ESDMON
    wwi->esd_fd = esd_monitor_stream(in_format, in_rate, esd_host, "wineesd");
#else
    wwi->esd_fd = esd_record_stream(in_format, in_rate, esd_host, "wineesd");
#endif
    TRACE("(wwi->esd_fd=%d)\n",wwi->esd_fd);
    wwi->state = WINE_WS_STOPPED;

    if (wwi->lpQueuePtr) {
	WARN("Should have an empty queue (%p)\n", wwi->lpQueuePtr);
	wwi->lpQueuePtr = NULL;
    }

    if(wwi->esd_fd < 0) return MMSYSERR_ALLOCATED;

    /* Set the esd socket O_NONBLOCK, so we can stop recording smoothly */
    mode = fcntl(wwi->esd_fd, F_GETFL);
    mode |= O_NONBLOCK;
    fcntl(wwi->esd_fd, F_SETFL, mode);

    wwi->dwRecordedTotal = 0;
    wwi->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    ESD_InitRingMessage(&wwi->msgRing);

    /* create recorder thread */
    if (!(dwFlags & WAVE_DIRECTSOUND)) {
	wwi->hStartUpEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
	wwi->hThread = CreateThread(NULL, 0, widRecorder, (LPVOID)(DWORD)wDevID, 0, &(wwi->dwThreadID));
	WaitForSingleObject(wwi->hStartUpEvent, INFINITE);
	CloseHandle(wwi->hStartUpEvent);
    } else {
	wwi->hThread = INVALID_HANDLE_VALUE;
	wwi->dwThreadID = 0;
    }
    wwi->hStartUpEvent = INVALID_HANDLE_VALUE;

    TRACE("wBitsPerSample=%u, nAvgBytesPerSec=%u, nSamplesPerSec=%u, nChannels=%u nBlockAlign=%u!\n",
	  wwi->waveFormat.Format.wBitsPerSample, wwi->waveFormat.Format.nAvgBytesPerSec,
	  wwi->waveFormat.Format.nSamplesPerSec, wwi->waveFormat.Format.nChannels,
	  wwi->waveFormat.Format.nBlockAlign);
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

    ESD_AddRingMessage(&wwi->msgRing, WINE_WM_CLOSING, 0, TRUE);
    ESD_CloseWaveInDevice(wwi);
    wwi->state = WINE_WS_CLOSED;
    ESD_DestroyRingMessage(&wwi->msgRing);
    return widNotifyClient(wwi, WIM_CLOSE, 0L, 0L);
}

/**************************************************************************
 * 				widAddBuffer		[internal]
 */
static DWORD widAddBuffer(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08X);\n", wDevID, lpWaveHdr, dwSize);

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

    ESD_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_HEADER, (DWORD)lpWaveHdr, FALSE);
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

    ESD_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_STARTING, 0, TRUE);
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

    ESD_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_STOPPING, 0, TRUE);

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
    ESD_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_RESETTING, 0, TRUE);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widMessage (WINEESD.6)
 */
DWORD WINAPI ESD_widMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
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
	interface with esd like we do with oss */
    MESSAGE("This sound card's driver does not support direct access\n");
    MESSAGE("The (slower) DirectSound HEL mode will be used instead.\n");
    return MMSYSERR_NOTSUPPORTED;
}

static DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc)
{
    memset(desc, 0, sizeof(*desc));
    strcpy(desc->szDesc, "Wine EsounD DirectSound Driver");
    strcpy(desc->szDrvname, "wineesd.drv");
    return MMSYSERR_NOERROR;
}

#else /* !HAVE_ESD */

/**************************************************************************
 * 				wodMessage (WINEESD.@)
 */
DWORD WINAPI ESD_wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08X, %08X, %08X):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				widMessage (WINEESD.6)
 */
DWORD WINAPI ESD_widMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08X, %08X, %08X):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

#endif /* HAVE_ESD */
