/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Sample Wine Driver for Advanced Linux Sound System (ALSA)
 *      Based on version <final> of the ALSA API
 *
 * Copyright    2002 Eric Pouech
 *              2002 Marco Pietrobono
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

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "winuser.h"
#include "mmddk.h"
#include "dsound.h"
#include "dsdriver.h"
#include "alsa.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wave);


#if defined(HAVE_ALSA) && ((SND_LIB_MAJOR == 0 && SND_LIB_MINOR >= 9) || SND_LIB_MAJOR >= 1)

/* internal ALSALIB functions */
snd_pcm_uframes_t _snd_pcm_mmap_hw_ptr(snd_pcm_t *pcm);


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
enum win_wm_message {
    WINE_WM_PAUSING = WM_USER + 1, WINE_WM_RESTARTING, WINE_WM_RESETTING, WINE_WM_HEADER,
    WINE_WM_UPDATE, WINE_WM_BREAKLOOP, WINE_WM_CLOSING
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
#define ALSA_RING_BUFFER_INCREMENT      64
typedef struct {
    ALSA_MSG			* messages;
    int                         ring_buffer_size;
    int				msg_tosave;
    int				msg_toget;
    HANDLE			msg_event;
    CRITICAL_SECTION		msg_crst;
} ALSA_MSG_RING;

typedef struct {
    /* Windows information */
    volatile int		state;			/* one of the WINE_WS_ manifest constants */
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    PCMWAVEFORMAT		format;
    WAVEOUTCAPSA		caps;

    /* ALSA information (ALSA 0.9/1.x uses two different devices for playback/capture) */
    char *			device;
    snd_pcm_t*                  p_handle;                 /* handle to ALSA playback device */
    snd_pcm_t*                  c_handle;                 /* handle to ALSA capture device */
    snd_pcm_hw_params_t *	hw_params;		/* ALSA Hw params */

    snd_ctl_t *                 ctl;                    /* control handle for the playback volume */
    snd_ctl_elem_id_t *         playback_eid;		/* element id of the playback volume control */
    snd_ctl_elem_value_t *      playback_evalue;	/* element value of the playback volume control */
    snd_ctl_elem_info_t *       playback_einfo;         /* element info of the playback volume control */

    snd_pcm_sframes_t           (*write)(snd_pcm_t *, const void *, snd_pcm_uframes_t );

    struct pollfd		*ufds;
    int				count;

    DWORD                       dwBufferSize;           /* size of whole ALSA buffer in bytes */
    LPWAVEHDR			lpQueuePtr;		/* start of queued WAVEHDRs (waiting to be notified) */
    LPWAVEHDR			lpPlayPtr;		/* start of not yet fully played buffers */

    LPWAVEHDR			lpLoopPtr;              /* pointer of first buffer in loop, if any */
    DWORD			dwLoops;		/* private copy of loop counter */

    DWORD			dwPlayedTotal;

    /* synchronization stuff */
    HANDLE			hStartUpEvent;
    HANDLE			hThread;
    DWORD			dwThreadID;
    ALSA_MSG_RING		msgRing;

    /* DirectSound stuff */
    DSDRIVERDESC                ds_desc;
    GUID                        ds_guid;
} WINE_WAVEOUT;

static WINE_WAVEOUT	WOutDev   [MAX_WAVEOUTDRV];
static DWORD            ALSA_WodNumDevs;

static DWORD wodDsCreate(UINT wDevID, PIDSDRIVER* drv);
static DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc);
static DWORD wodDsGuid(UINT wDevID, LPGUID pGuid);

/* These strings used only for tracing */
#if 0
static const char *wodPlayerCmdString[] = {
    "WINE_WM_PAUSING",
    "WINE_WM_RESTARTING",
    "WINE_WM_RESETTING",
    "WINE_WM_HEADER",
    "WINE_WM_UPDATE",
    "WINE_WM_BREAKLOOP",
    "WINE_WM_CLOSING",
};
#endif

/*======================================================================*
 *                  Low level WAVE implementation			*
 *======================================================================*/

/**************************************************************************
 * 			ALSA_InitializeVolumeCtl		[internal]
 *
 * used to initialize the PCM Volume Control
 */
static int ALSA_InitializeVolumeCtl(WINE_WAVEOUT * wwo)
{
    snd_ctl_t *                 ctl = NULL;
    snd_ctl_card_info_t *	cardinfo;
    snd_ctl_elem_list_t *       elemlist;
    snd_ctl_elem_id_t *         e_id;
    snd_ctl_elem_info_t *       einfo;
    snd_hctl_t *                hctl = NULL;
    snd_hctl_elem_t *           elem;
    int                         nCtrls;
    int                         i;

    snd_ctl_card_info_alloca(&cardinfo);
    memset(cardinfo,0,snd_ctl_card_info_sizeof());

    snd_ctl_elem_list_alloca(&elemlist);
    memset(elemlist,0,snd_ctl_elem_list_sizeof());

    snd_ctl_elem_id_alloca(&e_id);
    memset(e_id,0,snd_ctl_elem_id_sizeof());

    snd_ctl_elem_info_alloca(&einfo);
    memset(einfo,0,snd_ctl_elem_info_sizeof());

#define EXIT_ON_ERROR(f,txt) do \
{ \
    int err; \
    if ( (err = (f) ) < 0) \
    { \
	ERR(txt ": %s\n", snd_strerror(err)); \
	if (hctl) \
	    snd_hctl_close(hctl); \
	if (ctl) \
	    snd_ctl_close(ctl); \
	return -1; \
    } \
} while(0)

    EXIT_ON_ERROR( snd_ctl_open(&ctl,"hw",0) , "ctl open failed" );
    EXIT_ON_ERROR( snd_ctl_card_info(ctl, cardinfo), "card info failed");
    EXIT_ON_ERROR( snd_ctl_elem_list(ctl, elemlist), "elem list failed");

    nCtrls = snd_ctl_elem_list_get_count(elemlist);

    EXIT_ON_ERROR( snd_hctl_open(&hctl,"hw",0), "hctl open failed");
    EXIT_ON_ERROR( snd_hctl_load(hctl), "hctl load failed" );

    elem=snd_hctl_first_elem(hctl);
    for ( i= 0; i<nCtrls; i++) {

	memset(e_id,0,snd_ctl_elem_id_sizeof());

	snd_hctl_elem_get_id(elem,e_id);
/*
	TRACE("ctl: #%d '%s'%d\n",
				   snd_ctl_elem_id_get_numid(e_id),
				   snd_ctl_elem_id_get_name(e_id),
				   snd_ctl_elem_id_get_index(e_id));
*/
	if ( !strcmp("PCM Playback Volume", snd_ctl_elem_id_get_name(e_id)) )
	{
	    EXIT_ON_ERROR( snd_hctl_elem_info(elem,einfo), "hctl elem info failed" );

	    /* few sanity checks... you'll never know... */
	    if ( snd_ctl_elem_info_get_type(einfo) != SND_CTL_ELEM_TYPE_INTEGER )
	    	WARN("playback volume control is not an integer\n");
	    if ( !snd_ctl_elem_info_is_readable(einfo) )
	    	WARN("playback volume control is readable\n");
	    if ( !snd_ctl_elem_info_is_writable(einfo) )
	    	WARN("playback volume control is readable\n");

	    TRACE("   ctrl range: min=%ld  max=%ld  step=%ld\n",
	         snd_ctl_elem_info_get_min(einfo),
	         snd_ctl_elem_info_get_max(einfo),
	         snd_ctl_elem_info_get_step(einfo));

	    EXIT_ON_ERROR( snd_ctl_elem_id_malloc(&wwo->playback_eid), "elem id malloc failed" );
	    EXIT_ON_ERROR( snd_ctl_elem_info_malloc(&wwo->playback_einfo), "elem info malloc failed" );
	    EXIT_ON_ERROR( snd_ctl_elem_value_malloc(&wwo->playback_evalue), "elem value malloc failed" );

	    /* ok, now we can safely save these objects for later */
	    snd_ctl_elem_id_copy(wwo->playback_eid, e_id);
	    snd_ctl_elem_info_copy(wwo->playback_einfo, einfo);
	    snd_ctl_elem_value_set_id(wwo->playback_evalue, wwo->playback_eid);
	    wwo->ctl = ctl;
	}

	elem=snd_hctl_elem_next(elem);
    }
    snd_hctl_close(hctl);
#undef EXIT_ON_ERROR
    return 0;
}

/**************************************************************************
 * 			ALSA_XRUNRecovery		[internal]
 *
 * used to recovery from XRUN errors (buffer underflow/overflow)
 */
static int ALSA_XRUNRecovery(WINE_WAVEOUT * wwo, int err)
{
    if (err == -EPIPE) {    /* under-run */
        err = snd_pcm_prepare(wwo->p_handle);
        if (err < 0)
             ERR( "underrun recovery failed. prepare failed: %s\n", snd_strerror(err));
        return 0;
    } else if (err == -ESTRPIPE) {
        while ((err = snd_pcm_resume(wwo->p_handle)) == -EAGAIN)
            sleep(1);       /* wait until the suspend flag is released */
            if (err < 0) {
                err = snd_pcm_prepare(wwo->p_handle);
                if (err < 0)
                    ERR("recovery from suspend failed, prepare failed: %s\n", snd_strerror(err));
            }
            return 0;
    }
    return err;
}

/**************************************************************************
 * 			ALSA_TraceParameters		[internal]
 *
 * used to trace format changes, hw and sw parameters
 */
static void ALSA_TraceParameters(snd_pcm_hw_params_t * hw_params, snd_pcm_sw_params_t * sw, int full)
{
    snd_pcm_format_t   format = snd_pcm_hw_params_get_format(hw_params);
    snd_pcm_access_t   access = snd_pcm_hw_params_get_access(hw_params);

#define X(x) ((x)? "true" : "false")
    if (full)
	TRACE("FLAGS: sampleres=%s overrng=%s pause=%s resume=%s syncstart=%s batch=%s block=%s double=%s "
    	      "halfd=%s joint=%s \n",
	      X(snd_pcm_hw_params_can_mmap_sample_resolution(hw_params)),
	      X(snd_pcm_hw_params_can_overrange(hw_params)),
	      X(snd_pcm_hw_params_can_pause(hw_params)),
	      X(snd_pcm_hw_params_can_resume(hw_params)),
	      X(snd_pcm_hw_params_can_sync_start(hw_params)),
	      X(snd_pcm_hw_params_is_batch(hw_params)),
	      X(snd_pcm_hw_params_is_block_transfer(hw_params)),
	      X(snd_pcm_hw_params_is_double(hw_params)),
	      X(snd_pcm_hw_params_is_half_duplex(hw_params)),
	      X(snd_pcm_hw_params_is_joint_duplex(hw_params)));
#undef X

    if (access >= 0)
	TRACE("access=%s\n", snd_pcm_access_name(access));
    else
    {
	snd_pcm_access_mask_t * acmask;
	snd_pcm_access_mask_alloca(&acmask);
	snd_pcm_hw_params_get_access_mask(hw_params, acmask);
	for ( access = SND_PCM_ACCESS_MMAP_INTERLEAVED; access <= SND_PCM_ACCESS_LAST; access++)
	    if (snd_pcm_access_mask_test(acmask, access))
		TRACE("access=%s\n", snd_pcm_access_name(access));
    }

    if (format >= 0)
    {
	TRACE("format=%s\n", snd_pcm_format_name(format));

    }
    else
    {
	snd_pcm_format_mask_t *     fmask;

	snd_pcm_format_mask_alloca(&fmask);
	snd_pcm_hw_params_get_format_mask(hw_params, fmask);
	for ( format = SND_PCM_FORMAT_S8; format <= SND_PCM_FORMAT_LAST ; format++)
	    if ( snd_pcm_format_mask_test(fmask, format) )
		TRACE("format=%s\n", snd_pcm_format_name(format));
    }

#define X(x) do { \
int n = snd_pcm_hw_params_get_##x(hw_params); \
if (n<0) \
    TRACE(#x "_min=%ld " #x "_max=%ld\n", \
        (long int)snd_pcm_hw_params_get_##x##_min(hw_params), \
	(long int)snd_pcm_hw_params_get_##x##_max(hw_params)); \
else \
    TRACE(#x "=%d\n", n); \
} while(0)
    X(channels);
    X(buffer_size);
#undef X

#define X(x) do { \
int n = snd_pcm_hw_params_get_##x(hw_params,0); \
if (n<0) \
    TRACE(#x "_min=%ld " #x "_max=%ld\n", \
        (long int)snd_pcm_hw_params_get_##x##_min(hw_params,0), \
	(long int)snd_pcm_hw_params_get_##x##_max(hw_params,0)); \
else \
    TRACE(#x "=%d\n", n); \
} while(0)
    X(rate);
    X(buffer_time);
    X(periods);
    X(period_size);
    X(period_time);
    X(tick_time);
#undef X

    if (!sw)
	return;


}



/******************************************************************
 *		ALSA_WaveInit
 *
 * Initialize internal structures from ALSA information
 */
LONG ALSA_WaveInit(void)
{
    snd_pcm_t*                  h = NULL;
    snd_pcm_info_t *            info;
    snd_pcm_hw_params_t *       hw_params;
    WINE_WAVEOUT*	        wwo;

    wwo = &WOutDev[0];

    /* FIXME: use better values */
    wwo->device = "hw";
    wwo->caps.wMid = 0x0002;
    wwo->caps.wPid = 0x0104;
    strcpy(wwo->caps.szPname, "SB16 Wave Out");
    wwo->caps.vDriverVersion = 0x0100;
    wwo->caps.dwFormats = 0x00000000;
    wwo->caps.dwSupport = WAVECAPS_VOLUME;
    strcpy(wwo->ds_desc.szDesc, "WineALSA DirectSound Driver");
    strcpy(wwo->ds_desc.szDrvName, "winealsa.drv");
    wwo->ds_guid = DSDEVID_DefaultPlayback;

    if (!wine_dlopen("libasound.so.2", RTLD_LAZY|RTLD_GLOBAL, NULL, 0))
    {
        ERR("Error: ALSA lib needs to be loaded with flags RTLD_LAZY and RTLD_GLOBAL.\n");
        return -1;
    }

    snd_pcm_info_alloca(&info);
    snd_pcm_hw_params_alloca(&hw_params);

#define EXIT_ON_ERROR(f,txt) do { int err; if ( (err = (f) ) < 0) { ERR(txt ": %s\n", snd_strerror(err)); if (h) snd_pcm_close(h); return -1; } } while(0)

    ALSA_WodNumDevs = 0;
    EXIT_ON_ERROR( snd_pcm_open(&h, wwo->device, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) , "open pcm" );
    if (!h) return -1;
    ALSA_WodNumDevs++;

    EXIT_ON_ERROR( snd_pcm_info(h, info) , "pcm info" );

    TRACE("dev=%d id=%s name=%s subdev=%d subdev_name=%s subdev_avail=%d subdev_num=%d stream=%s subclass=%s \n",
       snd_pcm_info_get_device(info),
       snd_pcm_info_get_id(info),
       snd_pcm_info_get_name(info),
       snd_pcm_info_get_subdevice(info),
       snd_pcm_info_get_subdevice_name(info),
       snd_pcm_info_get_subdevices_avail(info),
       snd_pcm_info_get_subdevices_count(info),
       snd_pcm_stream_name(snd_pcm_info_get_stream(info)),
       (snd_pcm_info_get_subclass(info) == SND_PCM_SUBCLASS_GENERIC_MIX ? "GENERIC MIX": "MULTI MIX"));

    EXIT_ON_ERROR( snd_pcm_hw_params_any(h, hw_params) , "pcm hw params" );
#undef EXIT_ON_ERROR

    if (TRACE_ON(wave))
	ALSA_TraceParameters(hw_params, NULL, TRUE);

    {
	snd_pcm_format_mask_t *     fmask;
	int ratemin = snd_pcm_hw_params_get_rate_min(hw_params, 0);
	int ratemax = snd_pcm_hw_params_get_rate_max(hw_params, 0);
	int chmin = snd_pcm_hw_params_get_channels_min(hw_params); \
	int chmax = snd_pcm_hw_params_get_channels_max(hw_params); \

	snd_pcm_format_mask_alloca(&fmask);
	snd_pcm_hw_params_get_format_mask(hw_params, fmask);

#define X(r,v) \
       if ( (r) >= ratemin && ( (r) <= ratemax || ratemax == -1) ) \
       { \
          if (snd_pcm_format_mask_test( fmask, SND_PCM_FORMAT_U8)) \
          { \
              if (chmin <= 1 && 1 <= chmax) \
                  wwo->caps.dwFormats |= WAVE_FORMAT_##v##S08; \
              if (chmin <= 2 && 2 <= chmax) \
                  wwo->caps.dwFormats |= WAVE_FORMAT_##v##S08; \
          } \
          if (snd_pcm_format_mask_test( fmask, SND_PCM_FORMAT_S16_LE)) \
          { \
              if (chmin <= 1 && 1 <= chmax) \
                  wwo->caps.dwFormats |= WAVE_FORMAT_##v##S16; \
              if (chmin <= 2 && 2 <= chmax) \
                  wwo->caps.dwFormats |= WAVE_FORMAT_##v##S16; \
          } \
       }
       X(11025,1);
       X(22050,2);
       X(44100,4);
#undef X
    }

    if ( snd_pcm_hw_params_get_channels_min(hw_params) > 1) FIXME("-\n");
    wwo->caps.wChannels = (snd_pcm_hw_params_get_channels_max(hw_params) >= 2) ? 2 : 1;
    if (snd_pcm_hw_params_get_channels_min(hw_params) <= 2 && 2 <= snd_pcm_hw_params_get_channels_max(hw_params))
        wwo->caps.dwSupport |= WAVECAPS_LRVOLUME;

    /* FIXME: always true ? */
    wwo->caps.dwSupport |= WAVECAPS_SAMPLEACCURATE;

    {
	snd_pcm_access_mask_t *     acmask;
	snd_pcm_access_mask_alloca(&acmask);
	snd_pcm_hw_params_get_access_mask(hw_params, acmask);

	/* FIXME: NONITERLEAVED and COMPLEX are not supported right now */
	if ( snd_pcm_access_mask_test( acmask, SND_PCM_ACCESS_MMAP_INTERLEAVED ) )
            wwo->caps.dwSupport |= WAVECAPS_DIRECTSOUND;
    }

    TRACE("Configured with dwFmts=%08lx dwSupport=%08lx\n",
          wwo->caps.dwFormats, wwo->caps.dwSupport);

    snd_pcm_close(h);

    ALSA_InitializeVolumeCtl(wwo);

    return 0;
}

/******************************************************************
 *		ALSA_InitRingMessage
 *
 * Initialize the ring of messages for passing between driver's caller and playback/record
 * thread
 */
static int ALSA_InitRingMessage(ALSA_MSG_RING* omr)
{
    omr->msg_toget = 0;
    omr->msg_tosave = 0;
    omr->msg_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    omr->ring_buffer_size = ALSA_RING_BUFFER_INCREMENT;
    omr->messages = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,omr->ring_buffer_size * sizeof(ALSA_MSG));

    InitializeCriticalSection(&omr->msg_crst);
    return 0;
}

/******************************************************************
 *		ALSA_DestroyRingMessage
 *
 */
static int ALSA_DestroyRingMessage(ALSA_MSG_RING* omr)
{
    CloseHandle(omr->msg_event);
    HeapFree(GetProcessHeap(),0,omr->messages);
    DeleteCriticalSection(&omr->msg_crst);
    return 0;
}

/******************************************************************
 *		ALSA_AddRingMessage
 *
 * Inserts a new message into the ring (should be called from DriverProc derivated routines)
 */
static int ALSA_AddRingMessage(ALSA_MSG_RING* omr, enum win_wm_message msg, DWORD param, BOOL wait)
{
    HANDLE	hEvent = INVALID_HANDLE_VALUE;

    EnterCriticalSection(&omr->msg_crst);
    if ((omr->msg_toget == ((omr->msg_tosave + 1) % omr->ring_buffer_size)))
    {
	omr->ring_buffer_size += ALSA_RING_BUFFER_INCREMENT;
	TRACE("omr->ring_buffer_size=%d\n",omr->ring_buffer_size);
	omr->messages = HeapReAlloc(GetProcessHeap(),0,omr->messages, omr->ring_buffer_size * sizeof(ALSA_MSG));
    }
    if (wait)
    {
        hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
        if (hEvent == INVALID_HANDLE_VALUE)
        {
            ERR("can't create event !?\n");
            LeaveCriticalSection(&omr->msg_crst);
            return 0;
        }
        if (omr->msg_toget != omr->msg_tosave && omr->messages[omr->msg_toget].msg != WINE_WM_HEADER)
            FIXME("two fast messages in the queue!!!!\n");

        /* fast messages have to be added at the start of the queue */
        omr->msg_toget = (omr->msg_toget + omr->ring_buffer_size - 1) % omr->ring_buffer_size;

        omr->messages[omr->msg_toget].msg = msg;
        omr->messages[omr->msg_toget].param = param;
        omr->messages[omr->msg_toget].hEvent = hEvent;
    }
    else
    {
        omr->messages[omr->msg_tosave].msg = msg;
        omr->messages[omr->msg_tosave].param = param;
        omr->messages[omr->msg_tosave].hEvent = INVALID_HANDLE_VALUE;
        omr->msg_tosave = (omr->msg_tosave + 1) % omr->ring_buffer_size;
    }
    LeaveCriticalSection(&omr->msg_crst);
    /* signal a new message */
    SetEvent(omr->msg_event);
    if (wait)
    {
        /* wait for playback/record thread to have processed the message */
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
    }
    return 1;
}

/******************************************************************
 *		ALSA_RetrieveRingMessage
 *
 * Get a message from the ring. Should be called by the playback/record thread.
 */
static int ALSA_RetrieveRingMessage(ALSA_MSG_RING* omr,
                                   enum win_wm_message *msg, DWORD *param, HANDLE *hEvent)
{
    EnterCriticalSection(&omr->msg_crst);

    if (omr->msg_toget == omr->msg_tosave) /* buffer empty ? */
    {
        LeaveCriticalSection(&omr->msg_crst);
	return 0;
    }

    *msg = omr->messages[omr->msg_toget].msg;
    omr->messages[omr->msg_toget].msg = 0;
    *param = omr->messages[omr->msg_toget].param;
    *hEvent = omr->messages[omr->msg_toget].hEvent;
    omr->msg_toget = (omr->msg_toget + 1) % omr->ring_buffer_size;
    LeaveCriticalSection(&omr->msg_crst);
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
static BOOL wodUpdatePlayedTotal(WINE_WAVEOUT* wwo, snd_pcm_status_t* ps)
{
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

    wwo->lpPlayPtr->reserved = 0;

    if (lpWaveHdr->dwFlags & WHDR_BEGINLOOP) {
	if (wwo->lpLoopPtr) {
	    WARN("Already in a loop. Discarding loop on this header (%p)\n", lpWaveHdr);
	} else {
	    TRACE("Starting loop (%ldx) with %p\n", lpWaveHdr->dwLoops, lpWaveHdr);
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
	    wwo->lpPlayPtr->reserved = 0;
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
 * 			     wodPlayer_DSPWait			[internal]
 * Returns the number of milliseconds to wait for the DSP buffer to play a
 * period
 */
static DWORD wodPlayer_DSPWait(const WINE_WAVEOUT *wwo)
{
    /* time for one period to be played */
    return snd_pcm_hw_params_get_period_time(wwo->hw_params, 0) / 1000;
}

/**************************************************************************
 * 			     wodPllayer_NotifyWait               [internal]
 * Returns the number of milliseconds to wait before attempting to notify
 * completion of the specified wavehdr.
 * This is based on the number of bytes remaining to be written in the
 * wave.
 */
static DWORD wodPlayer_NotifyWait(const WINE_WAVEOUT* wwo, LPWAVEHDR lpWaveHdr)
{
    DWORD dwMillis;

    dwMillis = (lpWaveHdr->dwBufferLength - lpWaveHdr->reserved) * 1000 / wwo->format.wf.nAvgBytesPerSec;
    if (!dwMillis) dwMillis = 1;

    return dwMillis;
}


/**************************************************************************
 * 			     wodPlayer_WriteMaxFrags            [internal]
 * Writes the maximum number of frames possible to the DSP and returns
 * the number of frames written.
 */
static int wodPlayer_WriteMaxFrags(WINE_WAVEOUT* wwo, DWORD* frames)
{
    /* Only attempt to write to free frames */
    LPWAVEHDR lpWaveHdr = wwo->lpPlayPtr;
    DWORD dwLength = snd_pcm_bytes_to_frames(wwo->p_handle, lpWaveHdr->dwBufferLength - lpWaveHdr->reserved);
    int toWrite = min(dwLength, *frames);
    int written;

    TRACE("Writing wavehdr %p.%lu[%lu]\n", lpWaveHdr, lpWaveHdr->reserved, lpWaveHdr->dwBufferLength);

    written = (wwo->write)(wwo->p_handle, lpWaveHdr->lpData + lpWaveHdr->reserved, toWrite);
    if ( written < 0)
    {
    	/* XRUN occurred. let's try to recover */
        ALSA_XRUNRecovery(wwo, written);
	written = (wwo->write)(wwo->p_handle, lpWaveHdr->lpData + lpWaveHdr->reserved, toWrite);
    }
    if (written <= 0)
    {
        /* still in error */
        ERR("Error in writing wavehdr. Reason: %s\n", snd_strerror(written));
        return written;
    }

    lpWaveHdr->reserved += snd_pcm_frames_to_bytes(wwo->p_handle, written);
    if ( lpWaveHdr->reserved >= lpWaveHdr->dwBufferLength) {
	/* this will be used to check if the given wave header has been fully played or not... */
        lpWaveHdr->reserved = lpWaveHdr->dwBufferLength;
        /* If we wrote all current wavehdr, skip to the next one */
        wodPlayer_PlayPtrNext(wwo);
    }
    *frames -= written;
    wwo->dwPlayedTotal += snd_pcm_frames_to_bytes(wwo->p_handle, written);

    return written;
}


/**************************************************************************
 * 				wodPlayer_NotifyCompletions	[internal]
 *
 * Notifies and remove from queue all wavehdrs which have been played to
 * the speaker (ie. they have cleared the ALSA buffer).  If force is true,
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
    while ((lpWaveHdr = wwo->lpQueuePtr) &&
           (force ||
            (lpWaveHdr != wwo->lpPlayPtr &&
             lpWaveHdr != wwo->lpLoopPtr &&
             lpWaveHdr->reserved == lpWaveHdr->dwBufferLength))) {

	wwo->lpQueuePtr = lpWaveHdr->lpNext;

	lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
	lpWaveHdr->dwFlags |= WHDR_DONE;

	wodNotifyClient(wwo, WOM_DONE, (DWORD)lpWaveHdr, 0);
    }
    return  (lpWaveHdr && lpWaveHdr != wwo->lpPlayPtr && lpWaveHdr != wwo->lpLoopPtr) ?
        wodPlayer_NotifyWait(wwo, lpWaveHdr) : INFINITE;
}


void wait_for_poll(snd_pcm_t *handle, struct pollfd *ufds, unsigned int count)
{
    unsigned short revents;

    while (1) {
	poll(ufds, count, -1);
	snd_pcm_poll_descriptors_revents(handle, ufds, count, &revents);

	if (revents & POLLERR)
	    return;

	/*if (revents & POLLOUT)
		return 0;*/
    }
}


/**************************************************************************
 * 				wodPlayer_Reset			[internal]
 *
 * wodPlayer helper. Resets current output stream.
 */
static	void	wodPlayer_Reset(WINE_WAVEOUT* wwo)
{
    enum win_wm_message	msg;
    DWORD		        param;
    HANDLE		        ev;
    int                         err;

    /* flush all possible output */
    wait_for_poll(wwo->p_handle, wwo->ufds, wwo->count);

    /* updates current notify list */
    wodPlayer_NotifyCompletions(wwo, FALSE);

    if ( (err = snd_pcm_drop(wwo->p_handle)) < 0) {
	FIXME("flush: %s\n", snd_strerror(err));
	wwo->hThread = 0;
	wwo->state = WINE_WS_STOPPED;
	ExitThread(-1);
    }
    if ( (err = snd_pcm_prepare(wwo->p_handle)) < 0 )
        ERR("pcm prepare failed: %s\n", snd_strerror(err));

    /* remove any buffer */
    wodPlayer_NotifyCompletions(wwo, TRUE);

    wwo->lpPlayPtr = wwo->lpQueuePtr = wwo->lpLoopPtr = NULL;
    wwo->state = WINE_WS_STOPPED;

    /* remove any existing message in the ring */
    EnterCriticalSection(&wwo->msgRing.msg_crst);
    /* return all pending headers in queue */
    while (ALSA_RetrieveRingMessage(&wwo->msgRing, &msg, &param, &ev))
    {
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
    int                 err;

    while (ALSA_RetrieveRingMessage(&wwo->msgRing, &msg, &param, &ev)) {
    /* TRACE("Received %s %lx\n", wodPlayerCmdString[msg - WM_USER - 1], param); */

	switch (msg) {
	case WINE_WM_PAUSING:
	    if ( snd_pcm_state(wwo->p_handle) == SND_PCM_STATE_RUNNING )
	     {
		err = snd_pcm_pause(wwo->p_handle, 1);
		if ( err < 0 )
		    ERR("pcm_pause failed: %s\n", snd_strerror(err));
	     }
	    wwo->state = WINE_WS_PAUSED;
	    SetEvent(ev);
	    break;
	case WINE_WM_RESTARTING:
            if (wwo->state == WINE_WS_PAUSED)
            {
		if ( snd_pcm_state(wwo->p_handle) == SND_PCM_STATE_PAUSED )
		 {
		    err = snd_pcm_pause(wwo->p_handle, 0);
		    if ( err < 0 )
		        ERR("pcm_pause failed: %s\n", snd_strerror(err));
		 }
                wwo->state = WINE_WS_PLAYING;
            }
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
	    wodPlayer_Reset(wwo);
	    SetEvent(ev);
	    break;
        case WINE_WM_UPDATE:
            wodUpdatePlayedTotal(wwo, NULL);
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
    DWORD               availInQ = snd_pcm_avail_update(wwo->p_handle);

    /* no more room... no need to try to feed */
    while (wwo->lpPlayPtr && availInQ > 0)
        if ( wodPlayer_WriteMaxFrags(wwo, &availInQ) < 0 )
	    break;

    return wodPlayer_DSPWait(wwo);
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
    WINE_WAVEOUT*	        wwo;
    snd_pcm_hw_params_t *       hw_params;
    snd_pcm_sw_params_t *       sw_params;
    snd_pcm_access_t            access;
    snd_pcm_format_t            format;
    int                         rate;
    unsigned int                buffer_time = 500000;
    unsigned int                period_time = 10000;
    int                         buffer_size;
    snd_pcm_uframes_t           period_size;
    int                         flags;
    snd_pcm_t *                 pcm;
    int                         err;

    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_sw_params_alloca(&sw_params);

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

    if ((dwFlags & WAVE_DIRECTSOUND) && !(wwo->caps.dwSupport & WAVECAPS_DIRECTSOUND))
	/* not supported, ignore it */
	dwFlags &= ~WAVE_DIRECTSOUND;

    wwo->p_handle = 0;
    flags = SND_PCM_NONBLOCK;
    if ( dwFlags & WAVE_DIRECTSOUND )
    	flags |= SND_PCM_ASYNC;

    if (snd_pcm_open(&pcm, wwo->device, SND_PCM_STREAM_PLAYBACK, dwFlags))
    {
        ERR("Error open: %s\n", snd_strerror(errno));
	return MMSYSERR_NOTENABLED;
    }

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

    snd_pcm_hw_params_any(pcm, hw_params);

#define EXIT_ON_ERROR(f,e,txt) do \
{ \
    int err; \
    if ( (err = (f) ) < 0) \
    { \
	ERR(txt ": %s\n", snd_strerror(err)); \
	snd_pcm_close(pcm); \
	return e; \
    } \
} while(0)

    access = SND_PCM_ACCESS_MMAP_INTERLEAVED;
    if ( ( err = snd_pcm_hw_params_set_access(pcm, hw_params, access ) ) < 0) {
        WARN("mmap not available. switching to standard write.\n");
        access = SND_PCM_ACCESS_RW_INTERLEAVED;
	EXIT_ON_ERROR( snd_pcm_hw_params_set_access(pcm, hw_params, access ), MMSYSERR_INVALPARAM, "unable to set access for playback");
	wwo->write = snd_pcm_writei;
    }
    else
	wwo->write = snd_pcm_mmap_writei;

    EXIT_ON_ERROR( snd_pcm_hw_params_set_channels(pcm, hw_params, wwo->format.wf.nChannels), MMSYSERR_INVALPARAM, "unable to set required channels");

    format = (wwo->format.wBitsPerSample == 16) ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_U8;
    EXIT_ON_ERROR( snd_pcm_hw_params_set_format(pcm, hw_params, format), MMSYSERR_INVALPARAM, "unable to set required format");

    rate = snd_pcm_hw_params_set_rate_near(pcm, hw_params, wwo->format.wf.nSamplesPerSec, 0);
    if (rate < 0) {
	ERR("Rate %ld Hz not available for playback: %s\n", wwo->format.wf.nSamplesPerSec, snd_strerror(rate));
	snd_pcm_close(pcm);
        return WAVERR_BADFORMAT;
    }
    if (rate != wwo->format.wf.nSamplesPerSec) {
	ERR("Rate doesn't match (requested %ld Hz, got %d Hz)\n", wwo->format.wf.nSamplesPerSec, rate);
	snd_pcm_close(pcm);
        return WAVERR_BADFORMAT;
    }
    
    EXIT_ON_ERROR( snd_pcm_hw_params_set_buffer_time_near(pcm, hw_params, buffer_time, 0), MMSYSERR_INVALPARAM, "unable to set buffer time");
    EXIT_ON_ERROR( snd_pcm_hw_params_set_period_time_near(pcm, hw_params, period_time, 0), MMSYSERR_INVALPARAM, "unable to set period time");

    EXIT_ON_ERROR( snd_pcm_hw_params(pcm, hw_params), MMSYSERR_INVALPARAM, "unable to set hw params for playback");
    
    period_size = snd_pcm_hw_params_get_period_size(hw_params, 0);
    buffer_size = snd_pcm_hw_params_get_buffer_size(hw_params);

    snd_pcm_sw_params_current(pcm, sw_params);
    EXIT_ON_ERROR( snd_pcm_sw_params_set_start_threshold(pcm, sw_params, dwFlags & WAVE_DIRECTSOUND ? INT_MAX : 1 ), MMSYSERR_ERROR, "unable to set start threshold");
    EXIT_ON_ERROR( snd_pcm_sw_params_set_silence_size(pcm, sw_params, 0), MMSYSERR_ERROR, "unable to set silence size");
    EXIT_ON_ERROR( snd_pcm_sw_params_set_avail_min(pcm, sw_params, period_size), MMSYSERR_ERROR, "unable to set avail min");
    EXIT_ON_ERROR( snd_pcm_sw_params_set_xfer_align(pcm, sw_params, 1), MMSYSERR_ERROR, "unable to set xfer align");
    EXIT_ON_ERROR( snd_pcm_sw_params_set_silence_threshold(pcm, sw_params, 0), MMSYSERR_ERROR, "unable to set silence threshold");
    EXIT_ON_ERROR( snd_pcm_sw_params(pcm, sw_params), MMSYSERR_ERROR, "unable to set sw params for playback");
#undef EXIT_ON_ERROR

    snd_pcm_prepare(pcm);

    if (TRACE_ON(wave))
	ALSA_TraceParameters(hw_params, sw_params, FALSE);

    /* now, we can save all required data for later use... */
    if ( wwo->hw_params )
    	snd_pcm_hw_params_free(wwo->hw_params);
    snd_pcm_hw_params_malloc(&(wwo->hw_params));
    snd_pcm_hw_params_copy(wwo->hw_params, hw_params);

    wwo->dwBufferSize = buffer_size;
    wwo->lpQueuePtr = wwo->lpPlayPtr = wwo->lpLoopPtr = NULL;
    wwo->p_handle = pcm;

    ALSA_InitRingMessage(&wwo->msgRing);

    wwo->count = snd_pcm_poll_descriptors_count (wwo->p_handle);
    if (wwo->count <= 0) {
	ERR("Invalid poll descriptors count\n");
	return MMSYSERR_ERROR;
    }

    wwo->ufds = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(struct pollfd) * wwo->count);
    if (wwo->ufds == NULL) {
	ERR("No enough memory\n");
	return MMSYSERR_NOMEM;
    }
    if ((err = snd_pcm_poll_descriptors(wwo->p_handle, wwo->ufds, wwo->count)) < 0) {
	ERR("Unable to obtain poll descriptors for playback: %s\n", snd_strerror(err));
	return MMSYSERR_ERROR;
    }

    if (!(dwFlags & WAVE_DIRECTSOUND)) {
	wwo->hStartUpEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
	wwo->hThread = CreateThread(NULL, 0, wodPlayer, (LPVOID)(DWORD)wDevID, 0, &(wwo->dwThreadID));
	WaitForSingleObject(wwo->hStartUpEvent, INFINITE);
	CloseHandle(wwo->hStartUpEvent);
    } else {
	wwo->hThread = INVALID_HANDLE_VALUE;
	wwo->dwThreadID = 0;
    }
    wwo->hStartUpEvent = INVALID_HANDLE_VALUE;

    TRACE("handle=%08lx \n", (DWORD)wwo->p_handle);
/*    if (wwo->dwFragmentSize % wwo->format.wf.nBlockAlign)
	ERR("Fragment doesn't contain an integral number of data blocks\n");
*/
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

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].p_handle == NULL) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    wwo = &WOutDev[wDevID];
    if (wwo->lpQueuePtr) {
	WARN("buffers still playing !\n");
	ret = WAVERR_STILLPLAYING;
    } else {
	if (wwo->hThread != INVALID_HANDLE_VALUE) {
	    ALSA_AddRingMessage(&wwo->msgRing, WINE_WM_CLOSING, 0, TRUE);
	}
        ALSA_DestroyRingMessage(&wwo->msgRing);

	snd_pcm_hw_params_free(wwo->hw_params);
	wwo->hw_params = NULL;

        snd_pcm_close(wwo->p_handle);
	wwo->p_handle = NULL;

	ret = wodNotifyClient(wwo, WOM_CLOSE, 0L, 0L);
    }

    HeapFree(GetProcessHeap(), 0, wwo->ufds);
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
    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].p_handle == NULL) {
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

    ALSA_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_HEADER, (DWORD)lpWaveHdr, FALSE);

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

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].p_handle == NULL) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    ALSA_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_PAUSING, 0, TRUE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodRestart				[internal]
 */
static DWORD wodRestart(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].p_handle == NULL) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    if (WOutDev[wDevID].state == WINE_WS_PAUSED) {
	ALSA_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_RESTARTING, 0, TRUE);
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

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].p_handle == NULL) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    ALSA_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_RESETTING, 0, TRUE);

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

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].p_handle == NULL) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;

    wwo = &WOutDev[wDevID];
    ALSA_AddRingMessage(&wwo->msgRing, WINE_WM_UPDATE, 0, TRUE);
    val = wwo->dwPlayedTotal;

    TRACE("wType=%04X wBitsPerSample=%u nSamplesPerSec=%lu nChannels=%u nAvgBytesPerSec=%lu\n",
	  lpTime->wType, wwo->format.wBitsPerSample,
	  wwo->format.wf.nSamplesPerSec, wwo->format.wf.nChannels,
	  wwo->format.wf.nAvgBytesPerSec);
    TRACE("dwPlayedTotal=%lu\n", val);

    switch (lpTime->wType) {
    case TIME_BYTES:
	lpTime->u.cb = val;
	TRACE("TIME_BYTES=%lu\n", lpTime->u.cb);
	break;
    case TIME_SAMPLES:
	lpTime->u.sample = val * 8 / wwo->format.wBitsPerSample /wwo->format.wf.nChannels;
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
 * 				wodBreakLoop			[internal]
 */
static DWORD wodBreakLoop(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].p_handle == NULL) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    ALSA_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_BREAKLOOP, 0, TRUE);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodGetVolume			[internal]
 */
static DWORD wodGetVolume(WORD wDevID, LPDWORD lpdwVol)
{
    WORD	       left, right;
    WINE_WAVEOUT*      wwo;
    int                count;
    long               min, max;

    TRACE("(%u, %p);\n", wDevID, lpdwVol);
    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].p_handle == NULL) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    wwo = &WOutDev[wDevID];
    count = snd_ctl_elem_info_get_count(wwo->playback_einfo);
    min = snd_ctl_elem_info_get_min(wwo->playback_einfo);
    max = snd_ctl_elem_info_get_max(wwo->playback_einfo);

#define VOLUME_ALSA_TO_WIN(x) (((x)-min) * 65536 /(max-min))
    if (lpdwVol == NULL)
	return MMSYSERR_NOTENABLED;

    switch (count)
    {
   	case 2:
	    left = VOLUME_ALSA_TO_WIN(snd_ctl_elem_value_get_integer(wwo->playback_evalue, 0));
	    right = VOLUME_ALSA_TO_WIN(snd_ctl_elem_value_get_integer(wwo->playback_evalue, 1));
	    break;
	case 1:
	    left = right = VOLUME_ALSA_TO_WIN(snd_ctl_elem_value_get_integer(wwo->playback_evalue, 0));
	    break;
	default:
	    WARN("%d channels mixer not supported\n", count);
	    return MMSYSERR_NOERROR;
     }
#undef VOLUME_ALSA_TO_WIN

    TRACE("left=%d right=%d !\n", left, right);
    *lpdwVol = MAKELONG( left, right );
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodSetVolume			[internal]
 */
static DWORD wodSetVolume(WORD wDevID, DWORD dwParam)
{
    WORD	       left, right;
    WINE_WAVEOUT*      wwo;
    int                count, err;
    long               min, max;

    TRACE("(%u, %08lX);\n", wDevID, dwParam);
    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].p_handle == NULL) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    wwo = &WOutDev[wDevID];
    count=snd_ctl_elem_info_get_count(wwo->playback_einfo);
    min = snd_ctl_elem_info_get_min(wwo->playback_einfo);
    max = snd_ctl_elem_info_get_max(wwo->playback_einfo);

    left  = LOWORD(dwParam);
    right = HIWORD(dwParam);

#define VOLUME_WIN_TO_ALSA(x) ( (((x) * (max-min)) / 65536) + min )
    switch (count)
    {
   	case 2:
	    snd_ctl_elem_value_set_integer(wwo->playback_evalue, 0, VOLUME_WIN_TO_ALSA(left));
	    snd_ctl_elem_value_set_integer(wwo->playback_evalue, 1, VOLUME_WIN_TO_ALSA(right));
	    break;
	case 1:
	    snd_ctl_elem_value_set_integer(wwo->playback_evalue, 0, VOLUME_WIN_TO_ALSA(left));
	    break;
	default:
	    WARN("%d channels mixer not supported\n", count);
     }
#undef VOLUME_WIN_TO_ALSA
    if ( (err = snd_ctl_elem_write(wwo->ctl, wwo->playback_evalue)) < 0)
    {
	ERR("error writing snd_ctl_elem_value: %s\n", snd_strerror(err));
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodGetNumDevs			[internal]
 */
static	DWORD	wodGetNumDevs(void)
{
    return ALSA_WodNumDevs;
}


/**************************************************************************
 * 				wodMessage (WINEALSA.@)
 */
DWORD WINAPI ALSA_wodMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
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
    case WODM_GETDEVCAPS:	return wodGetDevCaps	(wDevID, (LPWAVEOUTCAPSA)dwParam1,	dwParam2);
    case WODM_GETNUMDEVS:	return wodGetNumDevs	();
    case WODM_GETPITCH:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPITCH:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
    case WODM_WRITE:	 	return wodWrite		(wDevID, (LPWAVEHDR)dwParam1,		dwParam2);
    case WODM_PAUSE:	 	return wodPause		(wDevID);
    case WODM_GETPOS:	 	return wodGetPosition	(wDevID, (LPMMTIME)dwParam1, 		dwParam2);
    case WODM_BREAKLOOP: 	return wodBreakLoop     (wDevID);
    case WODM_PREPARE:	 	return wodPrepare	(wDevID, (LPWAVEHDR)dwParam1, 		dwParam2);
    case WODM_UNPREPARE: 	return wodUnprepare	(wDevID, (LPWAVEHDR)dwParam1, 		dwParam2);
    case WODM_GETVOLUME:	return wodGetVolume	(wDevID, (LPDWORD)dwParam1);
    case WODM_SETVOLUME:	return wodSetVolume	(wDevID, dwParam1);
    case WODM_RESTART:		return wodRestart	(wDevID);
    case WODM_RESET:		return wodReset		(wDevID);
    case DRV_QUERYDSOUNDIFACE:	return wodDsCreate	(wDevID, (PIDSDRIVER*)dwParam1);
    case DRV_QUERYDSOUNDDESC:	return wodDsDesc	(wDevID, (PDSDRIVERDESC)dwParam1);
    case DRV_QUERYDSOUNDGUID:	return wodDsGuid	(wDevID, (LPGUID)dwParam1);

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
    DWORD		      ref;
    /* IDsDriverBufferImpl fields */
    IDsDriverImpl*	      drv;

    CRITICAL_SECTION          mmap_crst;
    LPVOID                    mmap_buffer;
    DWORD                     mmap_buflen_bytes;
    snd_pcm_uframes_t         mmap_buflen_frames;
    snd_pcm_channel_area_t *  mmap_areas;
    snd_async_handler_t *     mmap_async_handler;
};

static void DSDB_CheckXRUN(IDsDriverBufferImpl* pdbi)
{
    WINE_WAVEOUT *     wwo = &(WOutDev[pdbi->drv->wDevID]);
    snd_pcm_state_t    state = snd_pcm_state(wwo->p_handle);

    if ( state == SND_PCM_STATE_XRUN )
    {
	int            err = snd_pcm_prepare(wwo->p_handle);
	TRACE("xrun occurred\n");
	if ( err < 0 )
            ERR("recovery from xrun failed, prepare failed: %s\n", snd_strerror(err));
    }
    else if ( state == SND_PCM_STATE_SUSPENDED )
    {
	int            err = snd_pcm_resume(wwo->p_handle);
	TRACE("recovery from suspension occurred\n");
        if (err < 0 && err != -EAGAIN){
            err = snd_pcm_prepare(wwo->p_handle);
            if (err < 0)
                ERR("recovery from suspend failed, prepare failed: %s\n", snd_strerror(err));
        }
    }
}

static void DSDB_MMAPCopy(IDsDriverBufferImpl* pdbi)
{
    WINE_WAVEOUT *     wwo = &(WOutDev[pdbi->drv->wDevID]);
    int                channels;
    snd_pcm_format_t   format;
    snd_pcm_uframes_t  period_size;
    snd_pcm_sframes_t  avail;

    if ( !pdbi->mmap_buffer || !wwo->hw_params || !wwo->p_handle)
    	return;

    channels = snd_pcm_hw_params_get_channels(wwo->hw_params);
    format = snd_pcm_hw_params_get_format(wwo->hw_params);
    period_size = snd_pcm_hw_params_get_period_size(wwo->hw_params, 0);
    avail = snd_pcm_avail_update(wwo->p_handle);

    DSDB_CheckXRUN(pdbi);

    TRACE("avail=%d format=%s channels=%d\n", (int)avail, snd_pcm_format_name(format), channels );

    while (avail >= period_size)
    {
	const snd_pcm_channel_area_t *areas;
	snd_pcm_uframes_t     ofs;
	snd_pcm_uframes_t     frames;
	int                   err;

	frames = avail / period_size * period_size; /* round down to a multiple of period_size */

	EnterCriticalSection(&pdbi->mmap_crst);

	snd_pcm_mmap_begin(wwo->p_handle, &areas, &ofs, &frames);
	snd_pcm_areas_copy(areas, ofs, pdbi->mmap_areas, ofs, channels, frames, format);
	err = snd_pcm_mmap_commit(wwo->p_handle, ofs, frames);

	LeaveCriticalSection(&pdbi->mmap_crst);

	if ( err != (snd_pcm_sframes_t) frames)
	    ERR("mmap partially failed.\n");

	avail = snd_pcm_avail_update(wwo->p_handle);
    }
 }

static void DSDB_PCMCallback(snd_async_handler_t *ahandler)
{
    /* snd_pcm_t *               handle = snd_async_handler_get_pcm(ahandler); */
    IDsDriverBufferImpl*      pdbi = snd_async_handler_get_callback_private(ahandler);
    TRACE("callback called\n");
    DSDB_MMAPCopy(pdbi);
}

static int DSDB_CreateMMAP(IDsDriverBufferImpl* pdbi)
 {
    WINE_WAVEOUT *            wwo = &(WOutDev[pdbi->drv->wDevID]);
    snd_pcm_format_t          format = snd_pcm_hw_params_get_format(wwo->hw_params);
    snd_pcm_uframes_t         frames = snd_pcm_hw_params_get_buffer_size(wwo->hw_params);
    int                       channels = snd_pcm_hw_params_get_channels(wwo->hw_params);
    unsigned int              bits_per_sample = snd_pcm_format_physical_width(format);
    unsigned int              bits_per_frame = bits_per_sample * channels;
    snd_pcm_channel_area_t *  a;
    unsigned int              c;
    int                       err;

    if (TRACE_ON(wave))
	ALSA_TraceParameters(wwo->hw_params, NULL, FALSE);

    TRACE("format=%s  frames=%ld  channels=%d  bits_per_sample=%d  bits_per_frame=%d\n",
          snd_pcm_format_name(format), frames, channels, bits_per_sample, bits_per_frame);

    pdbi->mmap_buflen_frames = frames;
    pdbi->mmap_buflen_bytes = snd_pcm_frames_to_bytes( wwo->p_handle, frames );
    pdbi->mmap_buffer = HeapAlloc(GetProcessHeap(),0,pdbi->mmap_buflen_bytes);
    if (!pdbi->mmap_buffer)
	return DSERR_OUTOFMEMORY;

    snd_pcm_format_set_silence(format, pdbi->mmap_buffer, frames );

    TRACE("created mmap buffer of %ld frames (%ld bytes) at %p\n",
        frames, pdbi->mmap_buflen_bytes, pdbi->mmap_buffer);

    pdbi->mmap_areas = HeapAlloc(GetProcessHeap(),0,channels*sizeof(snd_pcm_channel_area_t));
    if (!pdbi->mmap_areas)
	return DSERR_OUTOFMEMORY;

    a = pdbi->mmap_areas;
    for (c = 0; c < channels; c++, a++)
    {
	a->addr = pdbi->mmap_buffer;
	a->first = bits_per_sample * c;
	a->step = bits_per_frame;
	TRACE("Area %d: addr=%p  first=%d  step=%d\n", c, a->addr, a->first, a->step);
    }

    InitializeCriticalSection(&pdbi->mmap_crst);

    err = snd_async_add_pcm_handler(&pdbi->mmap_async_handler, wwo->p_handle, DSDB_PCMCallback, pdbi);
    if ( err < 0 )
     {
 	ERR("add_pcm_handler failed. reason: %s\n", snd_strerror(err));
	return DSERR_GENERIC;
     }

    return DS_OK;
 }

static void DSDB_DestroyMMAP(IDsDriverBufferImpl* pdbi)
{
    TRACE("mmap buffer %p destroyed\n", pdbi->mmap_buffer);
    HeapFree(GetProcessHeap(), 0, pdbi->mmap_areas);
    HeapFree(GetProcessHeap(), 0, pdbi->mmap_buffer);
    pdbi->mmap_areas = NULL;
    pdbi->mmap_buffer = NULL;
    DeleteCriticalSection(&pdbi->mmap_crst);
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
    TRACE("(%p)\n",iface);
    return ++This->ref;
}

static ULONG WINAPI IDsDriverBufferImpl_Release(PIDSDRIVERBUFFER iface)
{
    ICOM_THIS(IDsDriverBufferImpl,iface);
    TRACE("(%p)\n",iface);
    if (--This->ref)
	return This->ref;
    if (This == This->drv->primary)
	This->drv->primary = NULL;
    DSDB_DestroyMMAP(This);
    HeapFree(GetProcessHeap(), 0, This);
    return 0;
}

static HRESULT WINAPI IDsDriverBufferImpl_Lock(PIDSDRIVERBUFFER iface,
					       LPVOID*ppvAudio1,LPDWORD pdwLen1,
					       LPVOID*ppvAudio2,LPDWORD pdwLen2,
					       DWORD dwWritePosition,DWORD dwWriteLen,
					       DWORD dwFlags)
{
    /* ICOM_THIS(IDsDriverBufferImpl,iface); */
    TRACE("(%p)\n",iface);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_Unlock(PIDSDRIVERBUFFER iface,
						 LPVOID pvAudio1,DWORD dwLen1,
						 LPVOID pvAudio2,DWORD dwLen2)
{
    /* ICOM_THIS(IDsDriverBufferImpl,iface); */
    TRACE("(%p)\n",iface);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetFormat(PIDSDRIVERBUFFER iface,
						    LPWAVEFORMATEX pwfx)
{
    /* ICOM_THIS(IDsDriverBufferImpl,iface); */
    TRACE("(%p,%p)\n",iface,pwfx);
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
    WINE_WAVEOUT *      wwo = &(WOutDev[This->drv->wDevID]);
    snd_pcm_uframes_t   hw_ptr;
    snd_pcm_uframes_t   period_size;

    if (wwo->hw_params == NULL) return DSERR_GENERIC;

    period_size  = snd_pcm_hw_params_get_period_size(wwo->hw_params, 0);

    if (wwo->p_handle == NULL) return DSERR_GENERIC;
    /** we need to track down buffer underruns */
    DSDB_CheckXRUN(This);

    EnterCriticalSection(&This->mmap_crst);
    hw_ptr = _snd_pcm_mmap_hw_ptr(wwo->p_handle);
    if (lpdwPlay)
	*lpdwPlay = snd_pcm_frames_to_bytes(wwo->p_handle, hw_ptr/ period_size  * period_size) % This->mmap_buflen_bytes;
    if (lpdwWrite)
	*lpdwWrite = snd_pcm_frames_to_bytes(wwo->p_handle, (hw_ptr / period_size + 1) * period_size ) % This->mmap_buflen_bytes;
    LeaveCriticalSection(&This->mmap_crst);

    TRACE("hw_ptr=0x%08x, playpos=%ld, writepos=%ld\n", (unsigned int)hw_ptr, lpdwPlay?*lpdwPlay:-1, lpdwWrite?*lpdwWrite:-1);
    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_Play(PIDSDRIVERBUFFER iface, DWORD dwRes1, DWORD dwRes2, DWORD dwFlags)
{
    ICOM_THIS(IDsDriverBufferImpl,iface);
    WINE_WAVEOUT *       wwo = &(WOutDev[This->drv->wDevID]);
    snd_pcm_state_t      state;
    int                  err;

    TRACE("(%p,%lx,%lx,%lx)\n",iface,dwRes1,dwRes2,dwFlags);

    if (wwo->p_handle == NULL) return DSERR_GENERIC;

    state = snd_pcm_state(wwo->p_handle);
    if ( state == SND_PCM_STATE_SETUP )
    {
	err = snd_pcm_prepare(wwo->p_handle);
        state = snd_pcm_state(wwo->p_handle);
    }
    if ( state == SND_PCM_STATE_PREPARED )
     {
	DSDB_MMAPCopy(This);
	err = snd_pcm_start(wwo->p_handle);
     }
    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_Stop(PIDSDRIVERBUFFER iface)
{
    ICOM_THIS(IDsDriverBufferImpl,iface);
    WINE_WAVEOUT *    wwo = &(WOutDev[This->drv->wDevID]);
    int               err;
    DWORD             play;
    DWORD             write;

    TRACE("(%p)\n",iface);

    if (wwo->p_handle == NULL) return DSERR_GENERIC;

    /* ring buffer wrap up detection */
    IDsDriverBufferImpl_GetPosition(iface, &play, &write);
    if ( play > write)
    {
	TRACE("writepos wrapper up\n");
    	return DS_OK;
    }

    if ( ( err = snd_pcm_drop(wwo->p_handle)) < 0 )
    {
   	ERR("error while stopping pcm: %s\n", snd_strerror(err));
	return DSERR_GENERIC;
    }
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
    TRACE("(%p)\n",iface);
    This->ref++;
    return This->ref;
}

static ULONG WINAPI IDsDriverImpl_Release(PIDSDRIVER iface)
{
    ICOM_THIS(IDsDriverImpl,iface);
    TRACE("(%p)\n",iface);
    if (--This->ref)
	return This->ref;
    HeapFree(GetProcessHeap(),0,This);
    return 0;
}

static HRESULT WINAPI IDsDriverImpl_GetDriverDesc(PIDSDRIVER iface, PDSDRIVERDESC pDesc)
{
    ICOM_THIS(IDsDriverImpl,iface);
    TRACE("(%p,%p)\n",iface,pDesc);
    memcpy(pDesc, &(WOutDev[This->wDevID].ds_desc), sizeof(DSDRIVERDESC));
    pDesc->dwFlags = DSDDESC_DOMMSYSTEMOPEN | DSDDESC_DOMMSYSTEMSETFORMAT |
	DSDDESC_USESYSTEMMEMORY;
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
    /* ICOM_THIS(IDsDriverImpl,iface); */
    TRACE("(%p)\n",iface);
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_Close(PIDSDRIVER iface)
{
    /* ICOM_THIS(IDsDriverImpl,iface); */
    TRACE("(%p)\n",iface);
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_GetCaps(PIDSDRIVER iface, PDSDRIVERCAPS pCaps)
{
    ICOM_THIS(IDsDriverImpl,iface);
    TRACE("(%p,%p)\n",iface,pCaps);
    memset(pCaps, 0, sizeof(*pCaps));

    pCaps->dwFlags = DSCAPS_PRIMARYMONO;
    if ( WOutDev[This->wDevID].caps.wChannels == 2 )
        pCaps->dwFlags |= DSCAPS_PRIMARYSTEREO;

    if ( WOutDev[This->wDevID].caps.dwFormats & (WAVE_FORMAT_1S08 | WAVE_FORMAT_2S08 | WAVE_FORMAT_4S08 ) )
	pCaps->dwFlags |= DSCAPS_PRIMARY8BIT;

    if ( WOutDev[This->wDevID].caps.dwFormats & (WAVE_FORMAT_1S16 | WAVE_FORMAT_2S16 | WAVE_FORMAT_4S16))
	pCaps->dwFlags |= DSCAPS_PRIMARY16BIT;

    pCaps->dwPrimaryBuffers = 1;
    TRACE("caps=0x%X\n",(unsigned int)pCaps->dwFlags);
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
    ICOM_THIS(IDsDriverImpl,iface);
    IDsDriverBufferImpl** ippdsdb = (IDsDriverBufferImpl**)ppvObj;
    int err;

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
    (*ippdsdb)->lpVtbl  = &dsdbvt;
    (*ippdsdb)->ref	= 1;
    (*ippdsdb)->drv	= This;

    err = DSDB_CreateMMAP((*ippdsdb));
    if ( err != DS_OK )
     {
	HeapFree(GetProcessHeap(), 0, *ippdsdb);
	*ippdsdb = NULL;
	return err;
     }
    *ppbBuffer = (*ippdsdb)->mmap_buffer;
    *pdwcbBufferSize = (*ippdsdb)->mmap_buflen_bytes;

    This->primary = *ippdsdb;

    /* buffer is ready to go */
    TRACE("buffer created at %p\n", *ippdsdb);
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

    TRACE("driver created\n");

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
    (*idrv)->lpVtbl	= &dsdvt;
    (*idrv)->ref	= 1;

    (*idrv)->wDevID	= wDevID;
    (*idrv)->primary	= NULL;
    return MMSYSERR_NOERROR;
}

static DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc)
{
    memcpy(desc, &(WOutDev[wDevID].ds_desc), sizeof(DSDRIVERDESC));
    return MMSYSERR_NOERROR;
}

static DWORD wodDsGuid(UINT wDevID, LPGUID pGuid)
{
    TRACE("(%d,%p)\n",wDevID,pGuid);

    memcpy(pGuid, &(WOutDev[wDevID].ds_guid), sizeof(GUID));

    return MMSYSERR_NOERROR;
}

#endif

#ifndef HAVE_ALSA

/**************************************************************************
 * 				wodMessage (WINEALSA.@)
 */
DWORD WINAPI ALSA_wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser,
                             DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08lX, %08lX, %08lX):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

#endif /* HAVE_ALSA */
