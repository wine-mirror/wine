/* Definition for ALSA drivers : wine multimedia system
 *
 * Copyright (C) 2002 Erich Pouech
 * Copyright (C) 2002 Marco Pietrobono
 * Copyright (C) 2003 Christian Costa
 * Copyright (C) 2007 Maarten Lankhorst
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

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#if defined(HAVE_ALSA) && !defined(__ALSA_H)
#define __ALSA_H

#ifdef interface
#undef interface
#endif

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

#ifdef HAVE_ALSA_ASOUNDLIB_H
#include <alsa/asoundlib.h>
#elif defined(HAVE_SYS_ASOUNDLIB_H)
#include <sys/asoundlib.h>
#endif
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif

#include "mmreg.h"
#include "dsound.h"
#include "dsdriver.h"

#include "ks.h"
#include "ksmedia.h"
#include "ksguid.h"

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
} ALSA_MSG;

/* implement an in-process message ring for better performance
 * (compared to passing thru the server)
 * this ring will be used by the input (resp output) record (resp playback) routine
 */
typedef struct {
    ALSA_MSG			* messages;
    int                         ring_buffer_size;
    int				msg_tosave;
    int				msg_toget;
/* Either pipe or event is used, but that is defined in alsa.c,
 * since this is a global header we define both here */
    int                         msg_pipe[2];
    HANDLE                      msg_event;
    CRITICAL_SECTION		msg_crst;
} ALSA_MSG_RING;

typedef struct {
    volatile int		state;			/* one of the WINE_WS_ manifest constants */
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    WAVEFORMATPCMEX		format;

    char*                       pcmname;                /* string name of alsa PCM device */
    char*                       ctlname;                /* string name of alsa control device */
    char                        interface_name[MAXPNAMELEN * 2];

    snd_pcm_t*                  pcm;                    /* handle to ALSA playback device */

    snd_pcm_hw_params_t *       hw_params;

    DWORD                       dwBufferSize;           /* size of whole ALSA buffer in bytes */
    LPWAVEHDR			lpQueuePtr;		/* start of queued WAVEHDRs (waiting to be notified) */
    LPWAVEHDR			lpPlayPtr;		/* start of not yet fully played buffers */

    LPWAVEHDR			lpLoopPtr;              /* pointer of first buffer in loop, if any */
    DWORD			dwLoops;		/* private copy of loop counter */

    DWORD			dwPlayedTotal;		/* number of bytes actually played since opening */
    DWORD			dwWrittenTotal;		/* number of bytes written to ALSA buffer since opening */

    /* synchronization stuff */
    HANDLE			hStartUpEvent;
    HANDLE			hThread;
    DWORD			dwThreadID;
    ALSA_MSG_RING		msgRing;

    /* DirectSound stuff */
    DSDRIVERDESC                ds_desc;
    DSDRIVERCAPS                ds_caps;

    /* Waveout only fields */
    WAVEOUTCAPSW		outcaps;

    snd_hctl_t *                hctl;                    /* control handle for the playback volume */

    snd_pcm_sframes_t           (*write)(snd_pcm_t *, const void *, snd_pcm_uframes_t );

    DWORD			dwPartialOffset;	/* Offset of not yet written bytes in lpPlayPtr */

    /* Wavein only fields */

    WAVEINCAPSW                 incaps;
    DWORD                       dwSupport;

    snd_pcm_sframes_t           (*read)(snd_pcm_t *, void *, snd_pcm_uframes_t );

    DWORD			dwPeriodSize;		/* size of OSS buffer period */
    DWORD			dwTotalRecorded;

}   WINE_WAVEDEV;

/*----------------------------------------------------------------------------
**  Global array of output and input devices, initialized via ALSA_WaveInit
*/
#define WAVEDEV_ALLOC_EXTENT_SIZE       10

/* wavein.c */
extern WINE_WAVEDEV	*WInDev;
extern DWORD		ALSA_WidNumMallocedDevs;
extern DWORD		ALSA_WidNumDevs;

/* waveout.c */
extern WINE_WAVEDEV	*WOutDev;
extern DWORD		ALSA_WodNumMallocedDevs;
extern DWORD		ALSA_WodNumDevs;
DWORD wodSetVolume(WORD wDevID, DWORD dwParam);

/* alsa.c */
int	ALSA_InitRingMessage(ALSA_MSG_RING* omr);
int	ALSA_DestroyRingMessage(ALSA_MSG_RING* omr);
void	ALSA_ResetRingMessage(ALSA_MSG_RING* omr);
void	ALSA_WaitRingMessage(ALSA_MSG_RING* omr, DWORD sleep);
int	ALSA_AddRingMessage(ALSA_MSG_RING* omr, enum win_wm_message msg, DWORD param, BOOL wait);
int	ALSA_RetrieveRingMessage(ALSA_MSG_RING* omr, enum win_wm_message *msg, DWORD *param, HANDLE *hEvent);
int	ALSA_PeekRingMessage(ALSA_MSG_RING* omr, enum win_wm_message *msg, DWORD *param, HANDLE *hEvent);
int	ALSA_CheckSetVolume(snd_hctl_t *hctl, int *out_left, int *out_right, int *out_min, int *out_max, int *out_step, int *new_left, int *new_right);

const char * ALSA_getCmdString(enum win_wm_message msg);
const char * ALSA_getMessage(UINT msg);
const char * ALSA_getFormat(WORD wFormatTag);
BOOL	ALSA_NearMatch(int rate1, int rate2);
DWORD	ALSA_bytes_to_mmtime(LPMMTIME lpTime, DWORD position, WAVEFORMATPCMEX* format);
void	ALSA_TraceParameters(snd_pcm_hw_params_t * hw_params, snd_pcm_sw_params_t * sw, int full);
int	ALSA_XRUNRecovery(WINE_WAVEDEV * wwo, int err);
void	ALSA_copyFormat(LPWAVEFORMATEX wf1, LPWAVEFORMATPCMEX wf2);
BOOL	ALSA_supportedFormat(LPWAVEFORMATEX wf);

/* dscapture.c */
DWORD widDsCreate(UINT wDevID, PIDSCDRIVER* drv);
DWORD widDsDesc(UINT wDevID, PDSDRIVERDESC desc);

/* dsoutput.c */
DWORD wodDsCreate(UINT wDevID, PIDSDRIVER* drv);
DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc);

/* midi.c */
extern LONG ALSA_MidiInit(void);

/* waveinit.c */
extern LONG ALSA_WaveInit(void);

#endif /* __ALSA_H */
