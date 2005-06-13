/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Sample Wine Driver for Advanced Linux Sound System (ALSA)
 *      Based on version <final> of the ALSA API
 *
 * Copyright    2002 Eric Pouech
 *              2002 Marco Pietrobono
 *              2003 Christian Costa : WaveIn support
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

/* unless someone makes a wineserver kernel module, Unix pipes are faster than win32 events */
#define USE_PIPE_SYNC

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
#include "winnls.h"
#include "winreg.h"
#include "mmddk.h"
#include "mmreg.h"
#include "dsound.h"
#include "dsdriver.h"
#include "ks.h"
#include "ksguid.h"
#include "ksmedia.h"
#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include "alsa.h"
#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wave);


#ifdef HAVE_ALSA

/* internal ALSALIB functions */
snd_pcm_uframes_t _snd_pcm_mmap_hw_ptr(snd_pcm_t *pcm);


#define MAX_WAVEOUTDRV 	(6)
#define MAX_WAVEINDRV 	(6)

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
#define SIGNAL_OMR(omr) do { int x = 0; write((omr)->msg_pipe[1], &x, sizeof(x)); } while (0)
#define CLEAR_OMR(omr) do { int x = 0; read((omr)->msg_pipe[0], &x, sizeof(x)); } while (0)
#define RESET_OMR(omr) do { } while (0)
#define WAIT_OMR(omr, sleep) \
  do { struct pollfd pfd; pfd.fd = (omr)->msg_pipe[0]; \
       pfd.events = POLLIN; poll(&pfd, 1, sleep); } while (0)
#else
#define SIGNAL_OMR(omr) do { SetEvent((omr)->msg_event); } while (0)
#define CLEAR_OMR(omr) do { } while (0)
#define RESET_OMR(omr) do { ResetEvent((omr)->msg_event); } while (0)
#define WAIT_OMR(omr, sleep) \
  do { WaitForSingleObject((omr)->msg_event, sleep); } while (0)
#endif

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
#ifdef USE_PIPE_SYNC
    int                         msg_pipe[2];
#else
    HANDLE                      msg_event;
#endif
    CRITICAL_SECTION		msg_crst;
} ALSA_MSG_RING;

typedef struct {
    volatile int		state;			/* one of the WINE_WS_ manifest constants */
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    WAVEFORMATPCMEX		format;

    /* ALSA information (ALSA 0.9/1.x uses two different devices for playback/capture) */
    char*                       device;
    char                        interface_name[64];
    snd_pcm_t*                  handle;                 /* handle to ALSA capture device */
    snd_pcm_hw_params_t *	hw_params;		/* ALSA Hw params */

    DWORD                       dwBufferSize;           /* size of whole ALSA buffer in bytes */
    LPWAVEHDR			lpQueuePtr;		/* start of queued WAVEHDRs (waiting to be notified) */
    LPWAVEHDR			lpPlayPtr;		/* start of not yet fully played buffers */

    LPWAVEHDR			lpLoopPtr;              /* pointer of first buffer in loop, if any */
    DWORD			dwLoops;		/* private copy of loop counter */

    DWORD			dwPlayedTotal;		/* number of bytes actually played since opening */
    DWORD			dwWrittenTotal;		/* number of bytes written to ALSA buffer since opening */

    struct pollfd		*ufds;
    int				count;

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

    char*                       mixer;                  /* mixer device name: hw:# */
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



static WINE_WAVEDEV	WOutDev   [MAX_WAVEOUTDRV];
static DWORD            ALSA_WodNumDevs;
static WINE_WAVEDEV	WInDev   [MAX_WAVEINDRV];
static DWORD            ALSA_WidNumDevs;

static DWORD wodDsCreate(UINT wDevID, PIDSDRIVER* drv);
static DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc);

/* These strings used only for tracing */
static const char * getCmdString(enum win_wm_message msg)
{
    static char unknown[32];
#define MSG_TO_STR(x) case x: return #x
    switch(msg) {
    MSG_TO_STR(WINE_WM_PAUSING);
    MSG_TO_STR(WINE_WM_RESTARTING);
    MSG_TO_STR(WINE_WM_RESETTING);
    MSG_TO_STR(WINE_WM_HEADER);
    MSG_TO_STR(WINE_WM_UPDATE);
    MSG_TO_STR(WINE_WM_BREAKLOOP);
    MSG_TO_STR(WINE_WM_CLOSING);
    MSG_TO_STR(WINE_WM_STARTING);
    MSG_TO_STR(WINE_WM_STOPPING);
    }
#undef MSG_TO_STR
    sprintf(unknown, "UNKNOWN(0x%08x)", msg);
    return unknown;
}

static const char * getMessage(UINT msg)
{
    static char unknown[32];
#define MSG_TO_STR(x) case x: return #x
    switch(msg) {
    MSG_TO_STR(DRVM_INIT);
    MSG_TO_STR(DRVM_EXIT);
    MSG_TO_STR(DRVM_ENABLE);
    MSG_TO_STR(DRVM_DISABLE);
    MSG_TO_STR(WIDM_OPEN);
    MSG_TO_STR(WIDM_CLOSE);
    MSG_TO_STR(WIDM_ADDBUFFER);
    MSG_TO_STR(WIDM_PREPARE);
    MSG_TO_STR(WIDM_UNPREPARE);
    MSG_TO_STR(WIDM_GETDEVCAPS);
    MSG_TO_STR(WIDM_GETNUMDEVS);
    MSG_TO_STR(WIDM_GETPOS);
    MSG_TO_STR(WIDM_RESET);
    MSG_TO_STR(WIDM_START);
    MSG_TO_STR(WIDM_STOP);
    MSG_TO_STR(WODM_OPEN);
    MSG_TO_STR(WODM_CLOSE);
    MSG_TO_STR(WODM_WRITE);
    MSG_TO_STR(WODM_PAUSE);
    MSG_TO_STR(WODM_GETPOS);
    MSG_TO_STR(WODM_BREAKLOOP);
    MSG_TO_STR(WODM_PREPARE);
    MSG_TO_STR(WODM_UNPREPARE);
    MSG_TO_STR(WODM_GETDEVCAPS);
    MSG_TO_STR(WODM_GETNUMDEVS);
    MSG_TO_STR(WODM_GETPITCH);
    MSG_TO_STR(WODM_SETPITCH);
    MSG_TO_STR(WODM_GETPLAYBACKRATE);
    MSG_TO_STR(WODM_SETPLAYBACKRATE);
    MSG_TO_STR(WODM_GETVOLUME);
    MSG_TO_STR(WODM_SETVOLUME);
    MSG_TO_STR(WODM_RESTART);
    MSG_TO_STR(WODM_RESET);
    MSG_TO_STR(DRV_QUERYDEVICEINTERFACESIZE);
    MSG_TO_STR(DRV_QUERYDEVICEINTERFACE);
    MSG_TO_STR(DRV_QUERYDSOUNDIFACE);
    MSG_TO_STR(DRV_QUERYDSOUNDDESC);
    }
#undef MSG_TO_STR
    sprintf(unknown, "UNKNOWN(0x%04x)", msg);
    return unknown;
}

static const char * getFormat(WORD wFormatTag)
{
    static char unknown[32];
#define FMT_TO_STR(x) case x: return #x
    switch(wFormatTag) {
    FMT_TO_STR(WAVE_FORMAT_PCM);
    FMT_TO_STR(WAVE_FORMAT_EXTENSIBLE);
    FMT_TO_STR(WAVE_FORMAT_MULAW);
    FMT_TO_STR(WAVE_FORMAT_ALAW);
    FMT_TO_STR(WAVE_FORMAT_ADPCM);
    }
#undef FMT_TO_STR
    sprintf(unknown, "UNKNOWN(0x%04x)", wFormatTag);
    return unknown;
}

/* Allow 1% deviation for sample rates (some ES137x cards) */
static BOOL NearMatch(int rate1, int rate2)
{
    return (((100 * (rate1 - rate2)) / rate1) == 0);
}

static DWORD bytes_to_mmtime(LPMMTIME lpTime, DWORD position,
                             WAVEFORMATPCMEX* format)
{
    TRACE("wType=%04X wBitsPerSample=%u nSamplesPerSec=%lu nChannels=%u nAvgBytesPerSec=%lu\n",
          lpTime->wType, format->Format.wBitsPerSample, format->Format.nSamplesPerSec,
          format->Format.nChannels, format->Format.nAvgBytesPerSec);
    TRACE("Position in bytes=%lu\n", position);

    switch (lpTime->wType) {
    case TIME_SAMPLES:
        lpTime->u.sample = position / (format->Format.wBitsPerSample / 8 * format->Format.nChannels);
        TRACE("TIME_SAMPLES=%lu\n", lpTime->u.sample);
        break;
    case TIME_MS:
        lpTime->u.ms = 1000.0 * position / (format->Format.wBitsPerSample / 8 * format->Format.nChannels * format->Format.nSamplesPerSec);
        TRACE("TIME_MS=%lu\n", lpTime->u.ms);
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
        TRACE("TIME_BYTES=%lu\n", lpTime->u.cb);
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
        if (wf->nChannels==1||wf->nChannels==2) {
            if (wf->wBitsPerSample==8||wf->wBitsPerSample==16)
                return TRUE;
        }
    } else if (wf->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        WAVEFORMATEXTENSIBLE 	* wfex = (WAVEFORMATEXTENSIBLE *)wf;

        if (wf->cbSize == 22 &&
            (IsEqualGUID(&wfex->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM) ||
             IsEqualGUID(&wfex->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))) {
            if (wf->nChannels>=1 && wf->nChannels<=6) {
                if (wf->wBitsPerSample==wfex->Samples.wValidBitsPerSample) {
                    if (wf->wBitsPerSample==8||wf->wBitsPerSample==16||
                        wf->wBitsPerSample==24||wf->wBitsPerSample==32) {
                        return TRUE;
                    }
                } else
                    WARN("wBitsPerSample != wValidBitsPerSample not supported yet\n");
            }
        } else
            WARN("only KSDATAFORMAT_SUBTYPE_PCM and KSDATAFORMAT_SUBTYPE_IEEE_FLOAT "
                 "supported\n");
    } else if (wf->wFormatTag == WAVE_FORMAT_MULAW || wf->wFormatTag == WAVE_FORMAT_ALAW) {
        if (wf->wBitsPerSample==8)
            return TRUE;
        else
            ERR("WAVE_FORMAT_MULAW and WAVE_FORMAT_ALAW wBitsPerSample must = 8\n");

    } else if (wf->wFormatTag == WAVE_FORMAT_ADPCM) {
        if (wf->wBitsPerSample==4)
            return TRUE;
        else
            ERR("WAVE_FORMAT_ADPCM wBitsPerSample must = 4\n");
    } else
        WARN("only WAVE_FORMAT_PCM and WAVE_FORMAT_EXTENSIBLE supported\n");

    return FALSE;
}

static void copy_format(LPWAVEFORMATEX wf1, LPWAVEFORMATPCMEX wf2)
{
    ZeroMemory(wf2, sizeof(wf2));
    if (wf1->wFormatTag == WAVE_FORMAT_PCM)
        memcpy(wf2, wf1, sizeof(PCMWAVEFORMAT));
    else if (wf1->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        memcpy(wf2, wf1, sizeof(WAVEFORMATPCMEX));
    else
        memcpy(wf2, wf1, sizeof(WAVEFORMATEX) + wf1->cbSize);
}

/*======================================================================*
 *                  Low level WAVE implementation			*
 *======================================================================*/

/**************************************************************************
 * 			ALSA_CheckSetVolume		[internal]
 *
 *  Helper function for Alsa volume queries.  This tries to simplify 
 * the process of managing the volume.  All parameters are optional
 * (pass NULL to ignore or not use).
 *  Return values are MMSYSERR_NOERROR on success, or !0 on failure;
 * error codes are normalized into the possible documented return
 * values from waveOutGetVolume.
 */
static int ALSA_CheckSetVolume(snd_hctl_t *hctl, int *out_left, int *out_right, 
            int *out_min, int *out_max, int *out_step,
            int *new_left, int *new_right)
{
    int rc = MMSYSERR_NOERROR;
    int value_count = 0;
    snd_hctl_elem_t *           elem = NULL;
    snd_ctl_elem_info_t *       eleminfop = NULL;
    snd_ctl_elem_value_t *      elemvaluep = NULL;
    snd_ctl_elem_id_t *         elemidp = NULL;


#define EXIT_ON_ERROR(f,txt,exitcode) do \
{ \
    int err; \
    if ( (err = (f) ) < 0) \
    { \
	ERR(txt " failed: %s\n", snd_strerror(err)); \
        rc = exitcode; \
        goto out; \
    } \
} while(0)

    if (! hctl)
        return MMSYSERR_NOTSUPPORTED;

    /* Allocate areas to return information about the volume */
    EXIT_ON_ERROR(snd_ctl_elem_id_malloc(&elemidp), "snd_ctl_elem_id_malloc", MMSYSERR_NOMEM);
    EXIT_ON_ERROR(snd_ctl_elem_value_malloc (&elemvaluep), "snd_ctl_elem_value_malloc", MMSYSERR_NOMEM);
    EXIT_ON_ERROR(snd_ctl_elem_info_malloc (&eleminfop), "snd_ctl_elem_info_malloc", MMSYSERR_NOMEM);
    snd_ctl_elem_id_clear(elemidp);
    snd_ctl_elem_value_clear(elemvaluep);
    snd_ctl_elem_info_clear(eleminfop);

    /* Setup and find an element id that exactly matches the characteristic we want
    ** FIXME:  It is probably short sighted to hard code and fixate on PCM Playback Volume */
    snd_ctl_elem_id_set_name(elemidp, "PCM Playback Volume");
    snd_ctl_elem_id_set_interface(elemidp, SND_CTL_ELEM_IFACE_MIXER);
    elem = snd_hctl_find_elem(hctl, elemidp);
    if (elem)
    {
        /* Read and return volume information */
        EXIT_ON_ERROR(snd_hctl_elem_info(elem, eleminfop), "snd_hctl_elem_info", MMSYSERR_NOTSUPPORTED);
        value_count = snd_ctl_elem_info_get_count(eleminfop);
        if (out_min || out_max || out_step)
        {
	    if (!snd_ctl_elem_info_is_readable(eleminfop))
            {
                ERR("snd_ctl_elem_info_is_readable returned false; cannot return info\n");
                rc = MMSYSERR_NOTSUPPORTED;
                goto out;
            }

            if (out_min)
                *out_min = snd_ctl_elem_info_get_min(eleminfop);

            if (out_max)
                *out_max = snd_ctl_elem_info_get_max(eleminfop);

            if (out_step)
                *out_step = snd_ctl_elem_info_get_step(eleminfop);
        }

        if (out_left || out_right)
        {
            EXIT_ON_ERROR(snd_hctl_elem_read(elem, elemvaluep), "snd_hctl_elem_read", MMSYSERR_NOTSUPPORTED);

            if (out_left)
                *out_left = snd_ctl_elem_value_get_integer(elemvaluep, 0);

            if (out_right)
            {
                if (value_count == 1)
                    *out_right = snd_ctl_elem_value_get_integer(elemvaluep, 0);
                else if (value_count == 2)
                    *out_right = snd_ctl_elem_value_get_integer(elemvaluep, 1);
                else
                {
                    ERR("Unexpected value count %d from snd_ctl_elem_info_get_count while getting volume info\n", value_count);
                    rc = -1;
                    goto out;
                }
            }
        }

        /* Set the volume */
        if (new_left || new_right)
        {
            EXIT_ON_ERROR(snd_hctl_elem_read(elem, elemvaluep), "snd_hctl_elem_read", MMSYSERR_NOTSUPPORTED);
            if (new_left)
	        snd_ctl_elem_value_set_integer(elemvaluep, 0, *new_left);
            if (new_right)
            {
                if (value_count == 1)
	            snd_ctl_elem_value_set_integer(elemvaluep, 0, *new_right);
                else if (value_count == 2)
	            snd_ctl_elem_value_set_integer(elemvaluep, 1, *new_right);
                else
                {
                    ERR("Unexpected value count %d from snd_ctl_elem_info_get_count while setting volume info\n", value_count);
                    rc = -1;
                    goto out;
                }
            }

            EXIT_ON_ERROR(snd_hctl_elem_write(elem, elemvaluep), "snd_hctl_elem_write", MMSYSERR_NOTSUPPORTED);
        }
    }
    else
    {
        ERR("Could not find 'PCM Playback Volume' element\n");
        rc = MMSYSERR_NOTSUPPORTED;
    }


#undef EXIT_ON_ERROR

out:

    if (elemvaluep)
        snd_ctl_elem_value_free(elemvaluep);
    if (eleminfop)
        snd_ctl_elem_info_free(eleminfop);
    if (elemidp)
        snd_ctl_elem_id_free(elemidp);

    return rc;
}


/**************************************************************************
 * 			ALSA_XRUNRecovery		[internal]
 *
 * used to recovery from XRUN errors (buffer underflow/overflow)
 */
static int ALSA_XRUNRecovery(WINE_WAVEDEV * wwo, int err)
{
    if (err == -EPIPE) {    /* under-run */
        err = snd_pcm_prepare(wwo->handle);
        if (err < 0)
             ERR( "underrun recovery failed. prepare failed: %s\n", snd_strerror(err));
        return 0;
    } else if (err == -ESTRPIPE) {
        while ((err = snd_pcm_resume(wwo->handle)) == -EAGAIN)
            sleep(1);       /* wait until the suspend flag is released */
        if (err < 0) {
            err = snd_pcm_prepare(wwo->handle);
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
    int err;
    snd_pcm_format_t   format;
    snd_pcm_access_t   access;

    err = snd_pcm_hw_params_get_access(hw_params, &access);
    err = snd_pcm_hw_params_get_format(hw_params, &format);

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

    do {
      int err=0;
      unsigned int val=0;
      err = snd_pcm_hw_params_get_channels(hw_params, &val); 
      if (err<0) {
        unsigned int min = 0;
        unsigned int max = 0;
        err = snd_pcm_hw_params_get_channels_min(hw_params, &min), 
	err = snd_pcm_hw_params_get_channels_max(hw_params, &max); 
        TRACE("channels_min=%u, channels_min_max=%u\n", min, max);
      } else {
        TRACE("channels=%d\n", val);
      }
    } while(0);
    do {
      int err=0;
      snd_pcm_uframes_t val=0;
      err = snd_pcm_hw_params_get_buffer_size(hw_params, &val); 
      if (err<0) {
        snd_pcm_uframes_t min = 0;
        snd_pcm_uframes_t max = 0;
        err = snd_pcm_hw_params_get_buffer_size_min(hw_params, &min), 
	err = snd_pcm_hw_params_get_buffer_size_max(hw_params, &max); 
        TRACE("buffer_size_min=%lu, buffer_size_min_max=%lu\n", min, max);
      } else {
        TRACE("buffer_size=%lu\n", val);
      }
    } while(0);

#define X(x) do { \
int err=0; \
int dir=0; \
unsigned int val=0; \
err = snd_pcm_hw_params_get_##x(hw_params,&val, &dir); \
if (err<0) { \
  unsigned int min = 0; \
  unsigned int max = 0; \
  err = snd_pcm_hw_params_get_##x##_min(hw_params, &min, &dir); \
  err = snd_pcm_hw_params_get_##x##_max(hw_params, &max, &dir); \
  TRACE(#x "_min=%u " #x "_max=%u\n", min, max); \
} else \
    TRACE(#x "=%d\n", val); \
} while(0)

    X(rate);
    X(buffer_time);
    X(periods);
    do {
      int err=0;
      int dir=0;
      snd_pcm_uframes_t val=0;
      err = snd_pcm_hw_params_get_period_size(hw_params, &val, &dir); 
      if (err<0) {
        snd_pcm_uframes_t min = 0;
        snd_pcm_uframes_t max = 0;
        err = snd_pcm_hw_params_get_period_size_min(hw_params, &min, &dir), 
	err = snd_pcm_hw_params_get_period_size_max(hw_params, &max, &dir); 
        TRACE("period_size_min=%lu, period_size_min_max=%lu\n", min, max);
      } else {
        TRACE("period_size=%lu\n", val);
      }
    } while(0);

    X(period_time);
    X(tick_time);
#undef X

    if (!sw)
	return;
}

/* return a string duplicated on the win32 process heap, free with HeapFree */
static char* ALSA_strdup(char *s) {
    char *result = HeapAlloc(GetProcessHeap(), 0, strlen(s)+1);
    strcpy(result, s);
    return result;
}

/******************************************************************
 *             ALSA_GetDeviceFromReg
 *
 * Returns either "plug:hw" or reads the registry so the user can
 * override the playback/record device used.
 */
static char *ALSA_GetDeviceFromReg(const char *value)
{
    DWORD res;
    DWORD type;
    HKEY key = 0;
    char *result = NULL;
    DWORD resultSize;

    /* @@ Wine registry key: HKLM\Software\Wine\Wine\Config\alsa */
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\ALSA", 0, KEY_QUERY_VALUE, &key);
    if (res != ERROR_SUCCESS) goto end;

    res = RegQueryValueExA(key, value, NULL, &type, NULL, &resultSize);
    if (res != ERROR_SUCCESS) goto end;

    if (type != REG_SZ) {
       ERR("Registry key [HKEY_LOCAL_MACHINE\\Software\\Wine\\Wine\\ALSA\\%s] must be a string\n", value);
       goto end;
    }

    result = HeapAlloc(GetProcessHeap(), 0, resultSize);
    res = RegQueryValueExA(key, value, NULL, NULL, result, &resultSize);

end:
    if (!result)
        result = ALSA_strdup("plug:hw");

    if (key)
        RegCloseKey(key);

    return result;
}

/******************************************************************
 *		ALSA_WaveInit
 *
 * Initialize internal structures from ALSA information
 */
LONG ALSA_WaveInit(void)
{
    snd_pcm_t*                  h;
    snd_pcm_info_t *            info;
    snd_pcm_hw_params_t *       hw_params;
    unsigned int ratemin=0;
    unsigned int ratemax=0;
    unsigned int chmin=0;
    unsigned int chmax=0;
    int dir=0;
    int err=0;
    WINE_WAVEDEV*	        wwo;
    WINE_WAVEDEV*	        wwi;
    int i;

    if (!wine_dlopen("libasound.so.2", RTLD_LAZY|RTLD_GLOBAL, NULL, 0))
    {
        ERR("Error: ALSA lib needs to be loaded with flags RTLD_LAZY and RTLD_GLOBAL.\n");
        return -1;
    }

    ALSA_WodNumDevs = 0;

    for (i = 0; i < MAX_WAVEOUTDRV; i++)
    {
        char device[64];
        char mixer[16];
        char * regdev;
        WCHAR nameW[64];
	snd_pcm_format_mask_t * fmask;
	snd_pcm_access_mask_t * acmask;

        wwo = &WOutDev[ALSA_WodNumDevs];

        regdev = ALSA_GetDeviceFromReg("PlaybackDevice");
        sprintf(device, "%s:%d", regdev, i);
        HeapFree(GetProcessHeap(), 0, regdev);
        wwo->device = HeapAlloc(GetProcessHeap(), 0, strlen(device));
        strcpy(wwo->device, device);
        TRACE("using waveout device \"%s\"\n", wwo->device);

        snprintf(wwo->interface_name, sizeof(wwo->interface_name), "winealsa: %s", wwo->device);

        wwo->outcaps.wMid = 0x0002;
        wwo->outcaps.wPid = 0x0104;
        wwo->outcaps.vDriverVersion = 0x0100;
        wwo->outcaps.dwFormats = 0x00000000;
        wwo->outcaps.dwSupport = 0;
        strcpy(wwo->ds_desc.szDrvname, "winealsa.drv");

        snd_pcm_info_alloca(&info);
        snd_pcm_hw_params_alloca(&hw_params);

#define EXIT_ON_ERROR(f,txt) do { int err; if ( (err = (f) ) < 0) { ERR(txt ": %s\n", snd_strerror(err)); if (h) snd_pcm_close(h); return -1; } } while(0)

        h = NULL;
        snd_pcm_open(&h, wwo->device, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
        if (!h)
            continue;

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

        strcpy(wwo->ds_desc.szDesc, snd_pcm_info_get_name(info));
        MultiByteToWideChar(CP_ACP, 0, wwo->ds_desc.szDesc, -1, nameW, sizeof(nameW)/sizeof(WCHAR));
        strcpyW(wwo->outcaps.szPname, nameW);
        EXIT_ON_ERROR( snd_pcm_hw_params_any(h, hw_params) , "pcm hw params" );
#undef EXIT_ON_ERROR

        err = snd_pcm_hw_params_get_rate_min(hw_params, &ratemin, &dir);
        err = snd_pcm_hw_params_get_rate_max(hw_params, &ratemax, &dir);
        err = snd_pcm_hw_params_get_channels_min(hw_params, &chmin);
        err = snd_pcm_hw_params_get_channels_max(hw_params, &chmax);
        if (TRACE_ON(wave))
            ALSA_TraceParameters(hw_params, NULL, TRUE);

	snd_pcm_format_mask_alloca(&fmask);
	snd_pcm_hw_params_get_format_mask(hw_params, fmask);

#define X(r,v) \
        if ( (r) >= ratemin && ( (r) <= ratemax || ratemax == -1) ) \
        { \
           if (snd_pcm_format_mask_test( fmask, SND_PCM_FORMAT_U8)) \
           { \
              if (chmin <= 1 && 1 <= chmax) \
                  wwo->outcaps.dwFormats |= WAVE_FORMAT_##v##M08; \
              if (chmin <= 2 && 2 <= chmax) \
                  wwo->outcaps.dwFormats |= WAVE_FORMAT_##v##S08; \
           } \
           if (snd_pcm_format_mask_test( fmask, SND_PCM_FORMAT_S16_LE)) \
           { \
              if (chmin <= 1 && 1 <= chmax) \
                  wwo->outcaps.dwFormats |= WAVE_FORMAT_##v##M16; \
              if (chmin <= 2 && 2 <= chmax) \
                  wwo->outcaps.dwFormats |= WAVE_FORMAT_##v##S16; \
           } \
        }
        X(11025,1);
        X(22050,2);
        X(44100,4);
        X(48000,48);
        X(96000,96);
#undef X

        if (chmin > 1)
            FIXME("-\n");
        wwo->outcaps.wChannels = chmax;

        /* FIXME: always true ? */
        wwo->outcaps.dwSupport |= WAVECAPS_SAMPLEACCURATE;

	snd_pcm_access_mask_alloca(&acmask);
	snd_pcm_hw_params_get_access_mask(hw_params, acmask);

	/* FIXME: NONITERLEAVED and COMPLEX are not supported right now */
	if ( snd_pcm_access_mask_test( acmask, SND_PCM_ACCESS_MMAP_INTERLEAVED ) )
            wwo->outcaps.dwSupport |= WAVECAPS_DIRECTSOUND;

        TRACE("Configured with dwFmts=%08lx dwSupport=%08lx\n",
              wwo->outcaps.dwFormats, wwo->outcaps.dwSupport);

        snd_pcm_close(h);

        /* Get a high level control handle for volume operations */
        /* FIXME:  This is never freed! (there are other things done in this function similarly not freed) */
        sprintf(mixer, "hw:%d", i);
        wwo->mixer = HeapAlloc(GetProcessHeap(), 0, strlen(mixer));
        strcpy(wwo->mixer, mixer);
        TRACE("using mixer device \"%s\"\n", wwo->mixer);

        if (snd_hctl_open(&wwo->hctl, wwo->mixer, 0) >= 0)
            snd_hctl_load(wwo->hctl);
        else
            wwo->hctl = NULL;

        /* check for volume control support */
        if (wwo->hctl) {
            wwo->outcaps.dwSupport |= WAVECAPS_VOLUME;

            if (chmin <= 2 && 2 <= chmax)
                wwo->outcaps.dwSupport |= WAVECAPS_LRVOLUME;
        }

        if (wwo->outcaps.dwFormats & (WAVE_FORMAT_1M08  | WAVE_FORMAT_2M08  |
                                   WAVE_FORMAT_4M08  | WAVE_FORMAT_48M08 |
                                   WAVE_FORMAT_96M08 | WAVE_FORMAT_1M16  |
                                   WAVE_FORMAT_2M16  | WAVE_FORMAT_4M16  |
                                   WAVE_FORMAT_48M16 | WAVE_FORMAT_96M16) )
            wwo->ds_caps.dwFlags |= DSCAPS_PRIMARYMONO;

        if (wwo->outcaps.dwFormats & (WAVE_FORMAT_1S08  | WAVE_FORMAT_2S08  |
                                   WAVE_FORMAT_4S08  | WAVE_FORMAT_48S08 |
                                   WAVE_FORMAT_96S08 | WAVE_FORMAT_1S16  |
                                   WAVE_FORMAT_2S16  | WAVE_FORMAT_4S16  |
                                   WAVE_FORMAT_48S16 | WAVE_FORMAT_96S16) )
            wwo->ds_caps.dwFlags |= DSCAPS_PRIMARYSTEREO;

        if (wwo->outcaps.dwFormats & (WAVE_FORMAT_1M08  | WAVE_FORMAT_2M08  |
                                   WAVE_FORMAT_4M08  | WAVE_FORMAT_48M08 |
                                   WAVE_FORMAT_96M08 | WAVE_FORMAT_1S08  |
                                   WAVE_FORMAT_2S08  | WAVE_FORMAT_4S08  |
                                   WAVE_FORMAT_48S08 | WAVE_FORMAT_96S08) )
            wwo->ds_caps.dwFlags |= DSCAPS_PRIMARY8BIT;

        if (wwo->outcaps.dwFormats & (WAVE_FORMAT_1M16  | WAVE_FORMAT_2M16  |
                                   WAVE_FORMAT_4M16  | WAVE_FORMAT_48M16 |
                                   WAVE_FORMAT_96M16 | WAVE_FORMAT_1S16  |
                                   WAVE_FORMAT_2S16  | WAVE_FORMAT_4S16  |
                                   WAVE_FORMAT_48S16 | WAVE_FORMAT_96S16) )
            wwo->ds_caps.dwFlags |= DSCAPS_PRIMARY16BIT;

        wwo->ds_caps.dwMinSecondarySampleRate = DSBFREQUENCY_MIN;
        wwo->ds_caps.dwMaxSecondarySampleRate = DSBFREQUENCY_MAX;
        wwo->ds_caps.dwPrimaryBuffers = 1;

        ALSA_WodNumDevs++;
    }

    ALSA_WidNumDevs = 0;

    for (i = 0; i < MAX_WAVEINDRV; i++)
    {
        char device[64];
        char * regdev;
        WCHAR nameW[64];
	snd_pcm_format_mask_t * fmask;
	snd_pcm_access_mask_t * acmask;

        wwi = &WInDev[ALSA_WidNumDevs];

        regdev = ALSA_GetDeviceFromReg("CaptureDevice");
        sprintf(device, "%s:%d", regdev, i);
        HeapFree(GetProcessHeap(), 0, regdev);
        wwi->device = HeapAlloc(GetProcessHeap(), 0, strlen(device));
        strcpy(wwi->device, device);

        TRACE("using wavein device \"%s\"\n", wwi->device);

        snprintf(wwi->interface_name, sizeof(wwi->interface_name), "winealsa: %s", wwi->device);

        wwi->incaps.wMid = 0x0002;
        wwi->incaps.wPid = 0x0104;
        wwi->incaps.vDriverVersion = 0x0100;
        wwi->incaps.dwFormats = 0x00000000;
        strcpy(wwi->ds_desc.szDrvname, "winealsa.drv");
        wwi->dwSupport = 0;

        snd_pcm_info_alloca(&info);
        snd_pcm_hw_params_alloca(&hw_params);

#define EXIT_ON_ERROR(f,txt) do { int err; if ( (err = (f) ) < 0) { ERR(txt ": %s\n", snd_strerror(err)); if (h) snd_pcm_close(h); return -1; } } while(0)

        h = NULL;
        snd_pcm_open(&h, wwi->device, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
        if (!h)
            continue;

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

        strcpy(wwi->ds_desc.szDesc, snd_pcm_info_get_name(info));
        MultiByteToWideChar(CP_ACP, 0, wwi->ds_desc.szDesc, -1, nameW, sizeof(nameW)/sizeof(WCHAR));
        strcpyW(wwi->incaps.szPname, nameW);
        EXIT_ON_ERROR( snd_pcm_hw_params_any(h, hw_params) , "pcm hw params" );
#undef EXIT_ON_ERROR
        err = snd_pcm_hw_params_get_rate_min(hw_params, &ratemin, &dir);
        err = snd_pcm_hw_params_get_rate_max(hw_params, &ratemax, &dir);
        err = snd_pcm_hw_params_get_channels_min(hw_params, &chmin);
        err = snd_pcm_hw_params_get_channels_max(hw_params, &chmax);

        if (TRACE_ON(wave))
            ALSA_TraceParameters(hw_params, NULL, TRUE);

	snd_pcm_format_mask_alloca(&fmask);
	snd_pcm_hw_params_get_format_mask(hw_params, fmask);

#define X(r,v) \
        if ( (r) >= ratemin && ( (r) <= ratemax || ratemax == -1) ) \
        { \
           if (snd_pcm_format_mask_test( fmask, SND_PCM_FORMAT_U8)) \
           { \
              if (chmin <= 1 && 1 <= chmax) \
                  wwi->incaps.dwFormats |= WAVE_FORMAT_##v##M08; \
              if (chmin <= 2 && 2 <= chmax) \
                  wwi->incaps.dwFormats |= WAVE_FORMAT_##v##S08; \
           } \
           if (snd_pcm_format_mask_test( fmask, SND_PCM_FORMAT_S16_LE)) \
           { \
              if (chmin <= 1 && 1 <= chmax) \
                  wwi->incaps.dwFormats |= WAVE_FORMAT_##v##M16; \
              if (chmin <= 2 && 2 <= chmax) \
                  wwi->incaps.dwFormats |= WAVE_FORMAT_##v##S16; \
           } \
        }
        X(11025,1);
        X(22050,2);
        X(44100,4);
        X(48000,48);
        X(96000,96);
#undef X

        if (chmin > 1)
            FIXME("-\n");
        wwi->incaps.wChannels = chmax;

	snd_pcm_access_mask_alloca(&acmask);
	snd_pcm_hw_params_get_access_mask(hw_params, acmask);

	/* FIXME: NONITERLEAVED and COMPLEX are not supported right now */
	if ( snd_pcm_access_mask_test( acmask, SND_PCM_ACCESS_MMAP_INTERLEAVED ) ) {
#if 0
            wwi->dwSupport |= WAVECAPS_DIRECTSOUND;
#endif
        }

        TRACE("Configured with dwFmts=%08lx\n", wwi->incaps.dwFormats);

        snd_pcm_close(h);

        ALSA_WidNumDevs++;
    }

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
#ifdef USE_PIPE_SYNC
    if (pipe(omr->msg_pipe) < 0) {
        omr->msg_pipe[0] = -1;
        omr->msg_pipe[1] = -1;
        ERR("could not create pipe, error=%s\n", strerror(errno));
    }
#else
    omr->msg_event = CreateEventW(NULL, FALSE, FALSE, NULL);
#endif
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
#ifdef USE_PIPE_SYNC
    close(omr->msg_pipe[0]);
    close(omr->msg_pipe[1]);
#else
    CloseHandle(omr->msg_event);
#endif
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
	int old_ring_buffer_size = omr->ring_buffer_size;
	omr->ring_buffer_size += ALSA_RING_BUFFER_INCREMENT;
	TRACE("omr->ring_buffer_size=%d\n",omr->ring_buffer_size);
	omr->messages = HeapReAlloc(GetProcessHeap(),0,omr->messages, omr->ring_buffer_size * sizeof(ALSA_MSG));
	/* Now we need to rearrange the ring buffer so that the new
	   buffers just allocated are in between omr->msg_tosave and
	   omr->msg_toget.
	*/
	if (omr->msg_tosave < omr->msg_toget)
	{
	    memmove(&(omr->messages[omr->msg_toget + ALSA_RING_BUFFER_INCREMENT]),
		    &(omr->messages[omr->msg_toget]),
		    sizeof(ALSA_MSG)*(old_ring_buffer_size - omr->msg_toget)
		    );
	    omr->msg_toget += ALSA_RING_BUFFER_INCREMENT;
	}
    }
    if (wait)
    {
        hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (hEvent == INVALID_HANDLE_VALUE)
        {
            ERR("can't create event !?\n");
            LeaveCriticalSection(&omr->msg_crst);
            return 0;
        }
        if (omr->msg_toget != omr->msg_tosave && omr->messages[omr->msg_toget].msg != WINE_WM_HEADER)
            FIXME("two fast messages in the queue!!!! toget = %d(%s), tosave=%d(%s)\n",
                  omr->msg_toget,getCmdString(omr->messages[omr->msg_toget].msg),
                  omr->msg_tosave,getCmdString(omr->messages[omr->msg_tosave].msg));

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
    SIGNAL_OMR(omr);
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
    CLEAR_OMR(omr);
    LeaveCriticalSection(&omr->msg_crst);
    return 1;
}

/******************************************************************
 *              ALSA_PeekRingMessage
 *
 * Peek at a message from the ring but do not remove it.
 * Should be called by the playback/record thread.
 */
static int ALSA_PeekRingMessage(ALSA_MSG_RING* omr,
                               enum win_wm_message *msg,
                               DWORD *param, HANDLE *hEvent)
{
    EnterCriticalSection(&omr->msg_crst);

    if (omr->msg_toget == omr->msg_tosave) /* buffer empty ? */
    {
	LeaveCriticalSection(&omr->msg_crst);
	return 0;
    }

    *msg = omr->messages[omr->msg_toget].msg;
    *param = omr->messages[omr->msg_toget].param;
    *hEvent = omr->messages[omr->msg_toget].hEvent;
    LeaveCriticalSection(&omr->msg_crst);
    return 1;
}

/*======================================================================*
 *                  Low level WAVE OUT implementation			*
 *======================================================================*/

/**************************************************************************
 * 			wodNotifyClient			[internal]
 */
static DWORD wodNotifyClient(WINE_WAVEDEV* wwo, WORD wMsg, DWORD dwParam1, DWORD dwParam2)
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
static BOOL wodUpdatePlayedTotal(WINE_WAVEDEV* wwo, snd_pcm_status_t* ps)
{
    snd_pcm_sframes_t delay = 0;
    snd_pcm_delay(wwo->handle, &delay);
    if (snd_pcm_state(wwo->handle) != SND_PCM_STATE_RUNNING)
        delay=0;
    wwo->dwPlayedTotal = wwo->dwWrittenTotal - snd_pcm_frames_to_bytes(wwo->handle, delay);
    return TRUE;
}

/**************************************************************************
 * 				wodPlayer_BeginWaveHdr          [internal]
 *
 * Makes the specified lpWaveHdr the currently playing wave header.
 * If the specified wave header is a begin loop and we're not already in
 * a loop, setup the loop.
 */
static void wodPlayer_BeginWaveHdr(WINE_WAVEDEV* wwo, LPWAVEHDR lpWaveHdr)
{
    wwo->lpPlayPtr = lpWaveHdr;

    if (!lpWaveHdr) return;

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
    wwo->dwPartialOffset = 0;
}

/**************************************************************************
 * 				wodPlayer_PlayPtrNext	        [internal]
 *
 * Advance the play pointer to the next waveheader, looping if required.
 */
static LPWAVEHDR wodPlayer_PlayPtrNext(WINE_WAVEDEV* wwo)
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
 * 			     wodPlayer_DSPWait			[internal]
 * Returns the number of milliseconds to wait for the DSP buffer to play a
 * period
 */
static DWORD wodPlayer_DSPWait(const WINE_WAVEDEV *wwo)
{
    /* time for one period to be played */
    unsigned int val=0;
    int dir=0;
    int err=0;
    err = snd_pcm_hw_params_get_period_time(wwo->hw_params, &val, &dir);
    return val / 1000;
}

/**************************************************************************
 * 			     wodPlayer_NotifyWait               [internal]
 * Returns the number of milliseconds to wait before attempting to notify
 * completion of the specified wavehdr.
 * This is based on the number of bytes remaining to be written in the
 * wave.
 */
static DWORD wodPlayer_NotifyWait(const WINE_WAVEDEV* wwo, LPWAVEHDR lpWaveHdr)
{
    DWORD dwMillis;

    if (lpWaveHdr->reserved < wwo->dwPlayedTotal) {
        dwMillis = 1;
    } else {
        dwMillis = (lpWaveHdr->reserved - wwo->dwPlayedTotal) * 1000 / wwo->format.Format.nAvgBytesPerSec;
        if (!dwMillis) dwMillis = 1;
    }

    return dwMillis;
}


/**************************************************************************
 * 			     wodPlayer_WriteMaxFrags            [internal]
 * Writes the maximum number of frames possible to the DSP and returns
 * the number of frames written.
 */
static int wodPlayer_WriteMaxFrags(WINE_WAVEDEV* wwo, DWORD* frames)
{
    /* Only attempt to write to free frames */
    LPWAVEHDR lpWaveHdr = wwo->lpPlayPtr;
    DWORD dwLength = snd_pcm_bytes_to_frames(wwo->handle, lpWaveHdr->dwBufferLength - wwo->dwPartialOffset);
    int toWrite = min(dwLength, *frames);
    int written;

    TRACE("Writing wavehdr %p.%lu[%lu]\n", lpWaveHdr, wwo->dwPartialOffset, lpWaveHdr->dwBufferLength);

    if (toWrite > 0) {
	written = (wwo->write)(wwo->handle, lpWaveHdr->lpData + wwo->dwPartialOffset, toWrite);
	if ( written < 0) {
	    /* XRUN occurred. let's try to recover */
	    ALSA_XRUNRecovery(wwo, written);
	    written = (wwo->write)(wwo->handle, lpWaveHdr->lpData + wwo->dwPartialOffset, toWrite);
	}
	if (written <= 0) {
	    /* still in error */
	    ERR("Error in writing wavehdr. Reason: %s\n", snd_strerror(written));
	    return written;
	}
    } else
	written = 0;

    wwo->dwPartialOffset += snd_pcm_frames_to_bytes(wwo->handle, written);
    if ( wwo->dwPartialOffset >= lpWaveHdr->dwBufferLength) {
	/* this will be used to check if the given wave header has been fully played or not... */
	wwo->dwPartialOffset = lpWaveHdr->dwBufferLength;
	/* If we wrote all current wavehdr, skip to the next one */
	wodPlayer_PlayPtrNext(wwo);
    }
    *frames -= written;
    wwo->dwWrittenTotal += snd_pcm_frames_to_bytes(wwo->handle, written);
    TRACE("dwWrittenTotal=%lu\n", wwo->dwWrittenTotal);

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
static DWORD wodPlayer_NotifyCompletions(WINE_WAVEDEV* wwo, BOOL force)
{
    LPWAVEHDR		lpWaveHdr;

    /* Start from lpQueuePtr and keep notifying until:
     * - we hit an unwritten wavehdr
     * - we hit the beginning of a running loop
     * - we hit a wavehdr which hasn't finished playing
     */
#if 0
    while ((lpWaveHdr = wwo->lpQueuePtr) &&
           (force ||
            (lpWaveHdr != wwo->lpPlayPtr &&
             lpWaveHdr != wwo->lpLoopPtr &&
             lpWaveHdr->reserved <= wwo->dwPlayedTotal))) {

	wwo->lpQueuePtr = lpWaveHdr->lpNext;

	lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
	lpWaveHdr->dwFlags |= WHDR_DONE;

	wodNotifyClient(wwo, WOM_DONE, (DWORD)lpWaveHdr, 0);
    }
#else
    for (;;)
    {
        lpWaveHdr = wwo->lpQueuePtr;
        if (!lpWaveHdr) {TRACE("Empty queue\n"); break;}
        if (!force)
        {
            if (lpWaveHdr == wwo->lpPlayPtr) {TRACE("play %p\n", lpWaveHdr); break;}
            if (lpWaveHdr == wwo->lpLoopPtr) {TRACE("loop %p\n", lpWaveHdr); break;}
            if (lpWaveHdr->reserved > wwo->dwPlayedTotal){TRACE("still playing %p (%lu/%lu)\n", lpWaveHdr, lpWaveHdr->reserved, wwo->dwPlayedTotal);break;}
        }
	wwo->lpQueuePtr = lpWaveHdr->lpNext;

	lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
	lpWaveHdr->dwFlags |= WHDR_DONE;

	wodNotifyClient(wwo, WOM_DONE, (DWORD)lpWaveHdr, 0);
    }
#endif
    return  (lpWaveHdr && lpWaveHdr != wwo->lpPlayPtr && lpWaveHdr != wwo->lpLoopPtr) ?
        wodPlayer_NotifyWait(wwo, lpWaveHdr) : INFINITE;
}


static void wait_for_poll(snd_pcm_t *handle, struct pollfd *ufds, unsigned int count)
{
    unsigned short revents;

    if (snd_pcm_state(handle) != SND_PCM_STATE_RUNNING)
	return;

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
static	void	wodPlayer_Reset(WINE_WAVEDEV* wwo)
{
    enum win_wm_message	msg;
    DWORD		        param;
    HANDLE		        ev;
    int                         err;
    TRACE("(%p)\n", wwo);

    /* flush all possible output */
    wait_for_poll(wwo->handle, wwo->ufds, wwo->count);

    wodUpdatePlayedTotal(wwo, NULL);
    /* updates current notify list */
    wodPlayer_NotifyCompletions(wwo, FALSE);

    if ( (err = snd_pcm_drop(wwo->handle)) < 0) {
	FIXME("flush: %s\n", snd_strerror(err));
	wwo->hThread = 0;
	wwo->state = WINE_WS_STOPPED;
	ExitThread(-1);
    }
    if ( (err = snd_pcm_prepare(wwo->handle)) < 0 )
        ERR("pcm prepare failed: %s\n", snd_strerror(err));

    /* remove any buffer */
    wodPlayer_NotifyCompletions(wwo, TRUE);

    wwo->lpPlayPtr = wwo->lpQueuePtr = wwo->lpLoopPtr = NULL;
    wwo->state = WINE_WS_STOPPED;
    wwo->dwPlayedTotal = wwo->dwWrittenTotal = 0;
    /* Clear partial wavehdr */
    wwo->dwPartialOffset = 0;

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
    RESET_OMR(&wwo->msgRing);
    LeaveCriticalSection(&wwo->msgRing.msg_crst);
}

/**************************************************************************
 * 		      wodPlayer_ProcessMessages			[internal]
 */
static void wodPlayer_ProcessMessages(WINE_WAVEDEV* wwo)
{
    LPWAVEHDR           lpWaveHdr;
    enum win_wm_message	msg;
    DWORD		param;
    HANDLE		ev;
    int                 err;

    while (ALSA_RetrieveRingMessage(&wwo->msgRing, &msg, &param, &ev)) {
     TRACE("Received %s %lx\n", getCmdString(msg), param); 

	switch (msg) {
	case WINE_WM_PAUSING:
	    if ( snd_pcm_state(wwo->handle) == SND_PCM_STATE_RUNNING )
	     {
		err = snd_pcm_pause(wwo->handle, 1);
		if ( err < 0 )
		    ERR("pcm_pause failed: %s\n", snd_strerror(err));
	     }
	    wwo->state = WINE_WS_PAUSED;
	    SetEvent(ev);
	    break;
	case WINE_WM_RESTARTING:
            if (wwo->state == WINE_WS_PAUSED)
            {
		if ( snd_pcm_state(wwo->handle) == SND_PCM_STATE_PAUSED )
		 {
		    err = snd_pcm_pause(wwo->handle, 0);
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
static DWORD wodPlayer_FeedDSP(WINE_WAVEDEV* wwo)
{
    DWORD               availInQ;

    wodUpdatePlayedTotal(wwo, NULL);
    availInQ = snd_pcm_avail_update(wwo->handle);

#if 0
    /* input queue empty and output buffer with less than one fragment to play */
    if (!wwo->lpPlayPtr && wwo->dwBufferSize < availInQ + wwo->dwFragmentSize) {
	TRACE("Run out of wavehdr:s...\n");
        return INFINITE;
    }
#endif

    /* no more room... no need to try to feed */
    if (availInQ > 0) {
        /* Feed from partial wavehdr */
        if (wwo->lpPlayPtr && wwo->dwPartialOffset != 0) {
            wodPlayer_WriteMaxFrags(wwo, &availInQ);
        }

        /* Feed wavehdrs until we run out of wavehdrs or DSP space */
        if (wwo->dwPartialOffset == 0 && wwo->lpPlayPtr) {
            do {
                TRACE("Setting time to elapse for %p to %lu\n",
                      wwo->lpPlayPtr, wwo->dwWrittenTotal + wwo->lpPlayPtr->dwBufferLength);
                /* note the value that dwPlayedTotal will return when this wave finishes playing */
                wwo->lpPlayPtr->reserved = wwo->dwWrittenTotal + wwo->lpPlayPtr->dwBufferLength;
            } while (wodPlayer_WriteMaxFrags(wwo, &availInQ) && wwo->lpPlayPtr && availInQ > 0);
        }
    }

    return wodPlayer_DSPWait(wwo);
}

/**************************************************************************
 * 				wodPlayer			[internal]
 */
static	DWORD	CALLBACK	wodPlayer(LPVOID pmt)
{
    WORD	  uDevID = (DWORD)pmt;
    WINE_WAVEDEV* wwo = (WINE_WAVEDEV*)&WOutDev[uDevID];
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
        WAIT_OMR(&wwo->msgRing, dwSleepTime);
	wodPlayer_ProcessMessages(wwo);
	if (wwo->state == WINE_WS_PLAYING) {
	    dwNextFeedTime = wodPlayer_FeedDSP(wwo);
	    dwNextNotifyTime = wodPlayer_NotifyCompletions(wwo, FALSE);
	    if (dwNextFeedTime == INFINITE) {
		/* FeedDSP ran out of data, but before giving up, */
		/* check that a notification didn't give us more */
		wodPlayer_ProcessMessages(wwo);
		if (wwo->lpPlayPtr) {
		    TRACE("recovering\n");
		    dwNextFeedTime = wodPlayer_FeedDSP(wwo);
		}
	    }
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

    memcpy(lpCaps, &WOutDev[wDevID].outcaps, min(dwSize, sizeof(*lpCaps)));
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodOpen				[internal]
 */
static DWORD wodOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    WINE_WAVEDEV*	        wwo;
    snd_pcm_hw_params_t *       hw_params;
    snd_pcm_sw_params_t *       sw_params;
    snd_pcm_access_t            access;
    snd_pcm_format_t            format = -1;
    unsigned int                rate;
    unsigned int                buffer_time = 500000;
    unsigned int                period_time = 10000;
    snd_pcm_uframes_t           buffer_size;
    snd_pcm_uframes_t           period_size;
    int                         flags;
    snd_pcm_t *                 pcm;
    int                         err=0;
    int                         dir=0;

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
    if (!supportedFormat(lpDesc->lpFormat)) {
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

    if (wwo->handle != NULL) {
        WARN("already allocated\n");
        return MMSYSERR_ALLOCATED;
    }

    if ((dwFlags & WAVE_DIRECTSOUND) && !(wwo->outcaps.dwSupport & WAVECAPS_DIRECTSOUND))
	/* not supported, ignore it */
	dwFlags &= ~WAVE_DIRECTSOUND;

    wwo->handle = 0;
    flags = SND_PCM_NONBLOCK;
#if 0
    if ( dwFlags & WAVE_DIRECTSOUND )
	flags |= SND_PCM_ASYNC;
#endif

    if ( (err = snd_pcm_open(&pcm, wwo->device, SND_PCM_STREAM_PLAYBACK, flags)) < 0)
    {
        ERR("Error open: %s\n", snd_strerror(err));
	return MMSYSERR_NOTENABLED;
    }

    wwo->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    memcpy(&wwo->waveDesc, lpDesc, sizeof(WAVEOPENDESC));
    copy_format(lpDesc->lpFormat, &wwo->format);

    TRACE("Requested this format: %ldx%dx%d %s\n",
          wwo->format.Format.nSamplesPerSec,
          wwo->format.Format.wBitsPerSample,
          wwo->format.Format.nChannels,
          getFormat(wwo->format.Format.wFormatTag));

    if (wwo->format.Format.wBitsPerSample == 0) {
	WARN("Resetting zeroed wBitsPerSample\n");
	wwo->format.Format.wBitsPerSample = 8 *
	    (wwo->format.Format.nAvgBytesPerSec /
	     wwo->format.Format.nSamplesPerSec) /
	    wwo->format.Format.nChannels;
    }

    snd_pcm_hw_params_any(pcm, hw_params);

#define EXIT_ON_ERROR(f,e,txt) do \
{ \
    int err; \
    if ( (err = (f) ) < 0) \
    { \
	WARN(txt ": %s\n", snd_strerror(err)); \
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

    if ((err = snd_pcm_hw_params_set_channels(pcm, hw_params, wwo->format.Format.nChannels)) < 0) {
        WARN("unable to set required channels: %d\n", wwo->format.Format.nChannels);
        if (dwFlags & WAVE_DIRECTSOUND) {
            if (wwo->format.Format.nChannels > 2)
                wwo->format.Format.nChannels = 2;
            else if (wwo->format.Format.nChannels == 2)
                wwo->format.Format.nChannels = 1;
            else if (wwo->format.Format.nChannels == 1)
                wwo->format.Format.nChannels = 2;
            /* recalculate block align and bytes per second */
            wwo->format.Format.nBlockAlign = (wwo->format.Format.wBitsPerSample * wwo->format.Format.nChannels) / 8;
            wwo->format.Format.nAvgBytesPerSec = wwo->format.Format.nSamplesPerSec * wwo->format.Format.nBlockAlign;
            WARN("changed number of channels from %d to %d\n", lpDesc->lpFormat->nChannels, wwo->format.Format.nChannels);
        }
        EXIT_ON_ERROR( snd_pcm_hw_params_set_channels(pcm, hw_params, wwo->format.Format.nChannels ), WAVERR_BADFORMAT, "unable to set required channels" );
    }

    if ((wwo->format.Format.wFormatTag == WAVE_FORMAT_PCM) ||
        ((wwo->format.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) &&
        IsEqualGUID(&wwo->format.SubFormat, &KSDATAFORMAT_SUBTYPE_PCM))) {
        format = (wwo->format.Format.wBitsPerSample == 8) ? SND_PCM_FORMAT_U8 :
                 (wwo->format.Format.wBitsPerSample == 16) ? SND_PCM_FORMAT_S16_LE :
                 (wwo->format.Format.wBitsPerSample == 24) ? SND_PCM_FORMAT_S24_LE :
                 (wwo->format.Format.wBitsPerSample == 32) ? SND_PCM_FORMAT_S32_LE : -1;
    } else if ((wwo->format.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) &&
        IsEqualGUID(&wwo->format.SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)){
        format = (wwo->format.Format.wBitsPerSample == 32) ? SND_PCM_FORMAT_FLOAT_LE : -1;
    } else if (wwo->format.Format.wFormatTag == WAVE_FORMAT_MULAW) {
        FIXME("unimplemented format: WAVE_FORMAT_MULAW\n");
        snd_pcm_close(pcm);
        return WAVERR_BADFORMAT;
    } else if (wwo->format.Format.wFormatTag == WAVE_FORMAT_ALAW) {
        FIXME("unimplemented format: WAVE_FORMAT_ALAW\n");
        snd_pcm_close(pcm);
        return WAVERR_BADFORMAT;
    } else if (wwo->format.Format.wFormatTag == WAVE_FORMAT_ADPCM) {
        FIXME("unimplemented format: WAVE_FORMAT_ADPCM\n");
        snd_pcm_close(pcm);
        return WAVERR_BADFORMAT;
    } else {
        ERR("invalid format: %0x04x\n", wwo->format.Format.wFormatTag);
        snd_pcm_close(pcm);
        return WAVERR_BADFORMAT;
    }

    if ((err = snd_pcm_hw_params_set_format(pcm, hw_params, format)) < 0) {
        WARN("unable to set required format: %s\n", snd_pcm_format_name(format));
        if (dwFlags & WAVE_DIRECTSOUND) {
            if ((wwo->format.Format.wFormatTag == WAVE_FORMAT_PCM) ||
               ((wwo->format.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) &&
               IsEqualGUID(&wwo->format.SubFormat, & KSDATAFORMAT_SUBTYPE_PCM))) {
                if (wwo->format.Format.wBitsPerSample != 16) {
                    wwo->format.Format.wBitsPerSample = 16;
                    format = SND_PCM_FORMAT_S16_LE;
                } else {
                    wwo->format.Format.wBitsPerSample = 8;
                    format = SND_PCM_FORMAT_U8;
                }
                /* recalculate block align and bytes per second */
                wwo->format.Format.nBlockAlign = (wwo->format.Format.wBitsPerSample * wwo->format.Format.nChannels) / 8;
                wwo->format.Format.nAvgBytesPerSec = wwo->format.Format.nSamplesPerSec * wwo->format.Format.nBlockAlign;
                WARN("changed bits per sample from %d to %d\n", lpDesc->lpFormat->wBitsPerSample, wwo->format.Format.wBitsPerSample);
            }
        }
        EXIT_ON_ERROR( snd_pcm_hw_params_set_format(pcm, hw_params, format), WAVERR_BADFORMAT, "unable to set required format" );
    }

    rate = wwo->format.Format.nSamplesPerSec;
    dir=0;
    err = snd_pcm_hw_params_set_rate_near(pcm, hw_params, &rate, &dir);
    if (err < 0) {
	WARN("Rate %ld Hz not available for playback: %s\n", wwo->format.Format.nSamplesPerSec, snd_strerror(rate));
	snd_pcm_close(pcm);
        return WAVERR_BADFORMAT;
    }
    if (!NearMatch(rate, wwo->format.Format.nSamplesPerSec)) {
        if (dwFlags & WAVE_DIRECTSOUND) {
            WARN("changed sample rate from %ld Hz to %d Hz\n", wwo->format.Format.nSamplesPerSec, rate);
            wwo->format.Format.nSamplesPerSec = rate;
            /* recalculate bytes per second */
            wwo->format.Format.nAvgBytesPerSec = wwo->format.Format.nSamplesPerSec * wwo->format.Format.nBlockAlign;
        } else {
            WARN("Rate doesn't match (requested %ld Hz, got %d Hz)\n", wwo->format.Format.nSamplesPerSec, rate);
            snd_pcm_close(pcm);
            return WAVERR_BADFORMAT;
        }
    }

    /* give the new format back to direct sound */
    if (dwFlags & WAVE_DIRECTSOUND) {
        lpDesc->lpFormat->wFormatTag = wwo->format.Format.wFormatTag;
        lpDesc->lpFormat->nChannels = wwo->format.Format.nChannels;
        lpDesc->lpFormat->nSamplesPerSec = wwo->format.Format.nSamplesPerSec;
        lpDesc->lpFormat->wBitsPerSample = wwo->format.Format.wBitsPerSample;
        lpDesc->lpFormat->nBlockAlign = wwo->format.Format.nBlockAlign;
        lpDesc->lpFormat->nAvgBytesPerSec = wwo->format.Format.nAvgBytesPerSec;
    }

    TRACE("Got this format: %ldx%dx%d %s\n",
          wwo->format.Format.nSamplesPerSec,
          wwo->format.Format.wBitsPerSample,
          wwo->format.Format.nChannels,
          getFormat(wwo->format.Format.wFormatTag));

    dir=0; 
    EXIT_ON_ERROR( snd_pcm_hw_params_set_buffer_time_near(pcm, hw_params, &buffer_time, &dir), MMSYSERR_INVALPARAM, "unable to set buffer time");
    dir=0; 
    EXIT_ON_ERROR( snd_pcm_hw_params_set_period_time_near(pcm, hw_params, &period_time, &dir), MMSYSERR_INVALPARAM, "unable to set period time");

    EXIT_ON_ERROR( snd_pcm_hw_params(pcm, hw_params), MMSYSERR_INVALPARAM, "unable to set hw params for playback");
    
    err = snd_pcm_hw_params_get_period_size(hw_params, &period_size, &dir);
    err = snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);

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

    wwo->dwBufferSize = snd_pcm_frames_to_bytes(pcm, buffer_size);
    wwo->lpQueuePtr = wwo->lpPlayPtr = wwo->lpLoopPtr = NULL;
    wwo->handle = pcm;
    wwo->dwPlayedTotal = wwo->dwWrittenTotal = 0;
    wwo->dwPartialOffset = 0;

    ALSA_InitRingMessage(&wwo->msgRing);

    wwo->count = snd_pcm_poll_descriptors_count (wwo->handle);
    if (wwo->count <= 0) {
	ERR("Invalid poll descriptors count\n");
	return MMSYSERR_ERROR;
    }

    wwo->ufds = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(struct pollfd) * wwo->count);
    if (wwo->ufds == NULL) {
	ERR("No enough memory\n");
	return MMSYSERR_NOMEM;
    }
    if ((err = snd_pcm_poll_descriptors(wwo->handle, wwo->ufds, wwo->count)) < 0) {
	ERR("Unable to obtain poll descriptors for playback: %s\n", snd_strerror(err));
	return MMSYSERR_ERROR;
    }

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

    TRACE("handle=%08lx \n", (DWORD)wwo->handle);
/*    if (wwo->dwFragmentSize % wwo->format.Format.nBlockAlign)
	ERR("Fragment doesn't contain an integral number of data blocks\n");
*/
    TRACE("wBitsPerSample=%u, nAvgBytesPerSec=%lu, nSamplesPerSec=%lu, nChannels=%u nBlockAlign=%u!\n",
	  wwo->format.Format.wBitsPerSample, wwo->format.Format.nAvgBytesPerSec,
	  wwo->format.Format.nSamplesPerSec, wwo->format.Format.nChannels,
	  wwo->format.Format.nBlockAlign);

    return wodNotifyClient(wwo, WOM_OPEN, 0L, 0L);
}


/**************************************************************************
 * 				wodClose			[internal]
 */
static DWORD wodClose(WORD wDevID)
{
    DWORD		ret = MMSYSERR_NOERROR;
    WINE_WAVEDEV*	wwo;

    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].handle == NULL) {
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

        snd_pcm_close(wwo->handle);
	wwo->handle = NULL;

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
    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].handle == NULL) {
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
 * 			wodPause				[internal]
 */
static DWORD wodPause(WORD wDevID)
{
    TRACE("(%u);!\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].handle == NULL) {
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

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].handle == NULL) {
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

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].handle == NULL) {
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
    WINE_WAVEDEV*	wwo;

    TRACE("(%u, %p, %lu);\n", wDevID, lpTime, uSize);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].handle == NULL) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;

    wwo = &WOutDev[wDevID];
    ALSA_AddRingMessage(&wwo->msgRing, WINE_WM_UPDATE, 0, TRUE);

    return bytes_to_mmtime(lpTime, wwo->dwPlayedTotal, &wwo->format);
}

/**************************************************************************
 * 				wodBreakLoop			[internal]
 */
static DWORD wodBreakLoop(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || WOutDev[wDevID].handle == NULL) {
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
    WORD	       wleft, wright;
    WINE_WAVEDEV*      wwo;
    int                min, max;
    int                left, right;
    DWORD              rc;

    TRACE("(%u, %p);\n", wDevID, lpdwVol);
    if (wDevID >= MAX_WAVEOUTDRV) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    if (lpdwVol == NULL)
	return MMSYSERR_NOTENABLED;

    wwo = &WOutDev[wDevID];

    if (lpdwVol == NULL)
	return MMSYSERR_NOTENABLED;

    rc = ALSA_CheckSetVolume(wwo->hctl, &left, &right, &min, &max, NULL, NULL, NULL);
    if (rc == MMSYSERR_NOERROR)
    {
#define VOLUME_ALSA_TO_WIN(x) (  ( (((x)-min) * 65535) + (max-min)/2 ) /(max-min))
        wleft = VOLUME_ALSA_TO_WIN(left);
        wright = VOLUME_ALSA_TO_WIN(right);
#undef VOLUME_ALSA_TO_WIN
        TRACE("left=%d,right=%d,converted to windows left %d, right %d\n", left, right, wleft, wright);
        *lpdwVol = MAKELONG( wleft, wright );
    }
    else
        TRACE("CheckSetVolume failed; rc %ld\n", rc);

    return rc;
}

/**************************************************************************
 * 				wodSetVolume			[internal]
 */
static DWORD wodSetVolume(WORD wDevID, DWORD dwParam)
{
    WORD	       wleft, wright;
    WINE_WAVEDEV*      wwo;
    int                min, max;
    int                left, right;
    DWORD              rc;

    TRACE("(%u, %08lX);\n", wDevID, dwParam);
    if (wDevID >= MAX_WAVEOUTDRV) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    wwo = &WOutDev[wDevID];

    rc = ALSA_CheckSetVolume(wwo->hctl, NULL, NULL, &min, &max, NULL, NULL, NULL);
    if (rc == MMSYSERR_NOERROR)
    {
        wleft  = LOWORD(dwParam);
        wright = HIWORD(dwParam);
#define VOLUME_WIN_TO_ALSA(x) ( (  ( ((x) * (max-min)) + 32767) / 65535) + min )
        left = VOLUME_WIN_TO_ALSA(wleft);
        right = VOLUME_WIN_TO_ALSA(wright);
#undef VOLUME_WIN_TO_ALSA
        rc = ALSA_CheckSetVolume(wwo->hctl, NULL, NULL, NULL, NULL, NULL, &left, &right);
        if (rc == MMSYSERR_NOERROR)
            TRACE("set volume:  wleft=%d, wright=%d, converted to alsa left %d, right %d\n", wleft, wright, left, right);
        else
            TRACE("SetVolume failed; rc %ld\n", rc);
    }

    return rc;
}

/**************************************************************************
 * 				wodGetNumDevs			[internal]
 */
static	DWORD	wodGetNumDevs(void)
{
    return ALSA_WodNumDevs;
}

/**************************************************************************
 * 				wodDevInterfaceSize		[internal]
 */
static DWORD wodDevInterfaceSize(UINT wDevID, LPDWORD dwParam1)
{
    TRACE("(%u, %p)\n", wDevID, dwParam1);

    *dwParam1 = MultiByteToWideChar(CP_ACP, 0, WOutDev[wDevID].interface_name, -1,
                                    NULL, 0 ) * sizeof(WCHAR);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodDevInterface			[internal]
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
 * 				wodMessage (WINEALSA.@)
 */
DWORD WINAPI ALSA_wodMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
                             DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%u, %s, %08lX, %08lX, %08lX);\n",
	  wDevID, getMessage(wMsg), dwUser, dwParam1, dwParam2);

    switch (wMsg) {
    case DRVM_INIT:
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
	/* FIXME: Pretend this is supported */
	return 0;
    case WODM_OPEN:	 	return wodOpen		(wDevID, (LPWAVEOPENDESC)dwParam1,	dwParam2);
    case WODM_CLOSE:	 	return wodClose		(wDevID);
    case WODM_GETDEVCAPS:	return wodGetDevCaps	(wDevID, (LPWAVEOUTCAPSW)dwParam1,	dwParam2);
    case WODM_GETNUMDEVS:	return wodGetNumDevs	();
    case WODM_GETPITCH:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPITCH:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
    case WODM_WRITE:	 	return wodWrite		(wDevID, (LPWAVEHDR)dwParam1,		dwParam2);
    case WODM_PAUSE:	 	return wodPause		(wDevID);
    case WODM_GETPOS:	 	return wodGetPosition	(wDevID, (LPMMTIME)dwParam1, 		dwParam2);
    case WODM_BREAKLOOP: 	return wodBreakLoop     (wDevID);
    case WODM_PREPARE:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_UNPREPARE: 	return MMSYSERR_NOTSUPPORTED;
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
 *                  Low level DSOUND implementation			*
 *======================================================================*/

typedef struct IDsDriverImpl IDsDriverImpl;
typedef struct IDsDriverBufferImpl IDsDriverBufferImpl;

struct IDsDriverImpl
{
    /* IUnknown fields */
    const IDsDriverVtbl *lpVtbl;
    DWORD		ref;
    /* IDsDriverImpl fields */
    UINT		wDevID;
    IDsDriverBufferImpl*primary;
};

struct IDsDriverBufferImpl
{
    /* IUnknown fields */
    const IDsDriverBufferVtbl *lpVtbl;
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
    WINE_WAVEDEV *     wwo = &(WOutDev[pdbi->drv->wDevID]);
    snd_pcm_state_t    state = snd_pcm_state(wwo->handle);

    if ( state == SND_PCM_STATE_XRUN )
    {
	int            err = snd_pcm_prepare(wwo->handle);
	TRACE("xrun occurred\n");
	if ( err < 0 )
            ERR("recovery from xrun failed, prepare failed: %s\n", snd_strerror(err));
    }
    else if ( state == SND_PCM_STATE_SUSPENDED )
    {
	int            err = snd_pcm_resume(wwo->handle);
	TRACE("recovery from suspension occurred\n");
        if (err < 0 && err != -EAGAIN){
            err = snd_pcm_prepare(wwo->handle);
            if (err < 0)
                ERR("recovery from suspend failed, prepare failed: %s\n", snd_strerror(err));
        }
    }
}

static void DSDB_MMAPCopy(IDsDriverBufferImpl* pdbi)
{
    WINE_WAVEDEV *     wwo = &(WOutDev[pdbi->drv->wDevID]);
    unsigned int       channels;
    snd_pcm_format_t   format;
    snd_pcm_uframes_t  period_size;
    snd_pcm_sframes_t  avail;
    int err;
    int dir=0;

    if ( !pdbi->mmap_buffer || !wwo->hw_params || !wwo->handle)
    	return;

    err = snd_pcm_hw_params_get_channels(wwo->hw_params, &channels);
    err = snd_pcm_hw_params_get_format(wwo->hw_params, &format);
    dir=0;
    err = snd_pcm_hw_params_get_period_size(wwo->hw_params, &period_size, &dir);
    avail = snd_pcm_avail_update(wwo->handle);

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

	snd_pcm_mmap_begin(wwo->handle, &areas, &ofs, &frames);
	if (areas != pdbi->mmap_areas || areas->addr != pdbi->mmap_areas->addr)
	    FIXME("Can't access sound driver's buffer directly.\n");
	err = snd_pcm_mmap_commit(wwo->handle, ofs, frames);

	LeaveCriticalSection(&pdbi->mmap_crst);

	if ( err != (snd_pcm_sframes_t) frames)
	    ERR("mmap partially failed.\n");

	avail = snd_pcm_avail_update(wwo->handle);
    }

    if (avail > 0)
    {
	const snd_pcm_channel_area_t *areas;
	snd_pcm_uframes_t     ofs;
	snd_pcm_uframes_t     frames;
	int                   err;

	frames = avail;

	EnterCriticalSection(&pdbi->mmap_crst);

	snd_pcm_mmap_begin(wwo->handle, &areas, &ofs, &frames);
	if (areas != pdbi->mmap_areas || areas->addr != pdbi->mmap_areas->addr)
	    FIXME("Can't access sound driver's buffer directly.\n");
	err = snd_pcm_mmap_commit(wwo->handle, ofs, frames);

	LeaveCriticalSection(&pdbi->mmap_crst);

	if ( err != (snd_pcm_sframes_t) frames)
	    ERR("mmap partially failed.\n");

	avail = snd_pcm_avail_update(wwo->handle);
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
    WINE_WAVEDEV *            wwo = &(WOutDev[pdbi->drv->wDevID]);
    snd_pcm_format_t          format;
    snd_pcm_uframes_t         frames;
    snd_pcm_uframes_t         ofs;
    snd_pcm_uframes_t         avail;
    unsigned int              channels;
    unsigned int              bits_per_sample;
    unsigned int              bits_per_frame;
    int                       err;

    err = snd_pcm_hw_params_get_format(wwo->hw_params, &format);
    err = snd_pcm_hw_params_get_buffer_size(wwo->hw_params, &frames);
    err = snd_pcm_hw_params_get_channels(wwo->hw_params, &channels);
    bits_per_sample = snd_pcm_format_physical_width(format);
    bits_per_frame = bits_per_sample * channels;


    if (TRACE_ON(wave))
	ALSA_TraceParameters(wwo->hw_params, NULL, FALSE);

    TRACE("format=%s  frames=%ld  channels=%d  bits_per_sample=%d  bits_per_frame=%d\n",
          snd_pcm_format_name(format), frames, channels, bits_per_sample, bits_per_frame);

    pdbi->mmap_buflen_frames = frames;
    pdbi->mmap_buflen_bytes = snd_pcm_frames_to_bytes( wwo->handle, frames );

    avail = snd_pcm_avail_update(wwo->handle);
    if (avail < 0)
    {
	ERR("No buffer is available: %s.", snd_strerror(avail));
	return DSERR_GENERIC;
    }
    err = snd_pcm_mmap_begin(wwo->handle, (const snd_pcm_channel_area_t **)&pdbi->mmap_areas, &ofs, &avail);
    if ( err < 0 )
    {
	ERR("Can't map sound device for direct access: %s\n", snd_strerror(err));
	return DSERR_GENERIC;
    }
    avail = 0;/* We don't have any data to commit yet */
    err = snd_pcm_mmap_commit(wwo->handle, ofs, avail);
    if (ofs > 0)
	err = snd_pcm_rewind(wwo->handle, ofs);
    pdbi->mmap_buffer = pdbi->mmap_areas->addr;

    snd_pcm_format_set_silence(format, pdbi->mmap_buffer, frames );

    TRACE("created mmap buffer of %ld frames (%ld bytes) at %p\n",
        frames, pdbi->mmap_buflen_bytes, pdbi->mmap_buffer);

    InitializeCriticalSection(&pdbi->mmap_crst);

    err = snd_async_add_pcm_handler(&pdbi->mmap_async_handler, wwo->handle, DSDB_PCMCallback, pdbi);
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
    pdbi->mmap_areas = NULL;
    pdbi->mmap_buffer = NULL;
    DeleteCriticalSection(&pdbi->mmap_crst);
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
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%lu)\n",This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDsDriverBufferImpl_Release(PIDSDRIVERBUFFER iface)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%lu)\n",This, refCount + 1);

    if (refCount)
	return refCount;
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
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    TRACE("(%p)\n",iface);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_Unlock(PIDSDRIVERBUFFER iface,
						 LPVOID pvAudio1,DWORD dwLen1,
						 LPVOID pvAudio2,DWORD dwLen2)
{
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    TRACE("(%p)\n",iface);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetFormat(PIDSDRIVERBUFFER iface,
						    LPWAVEFORMATEX pwfx)
{
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    TRACE("(%p,%p)\n",iface,pwfx);
    return DSERR_BUFFERLOST;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetFrequency(PIDSDRIVERBUFFER iface, DWORD dwFreq)
{
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    TRACE("(%p,%ld): stub\n",iface,dwFreq);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetVolumePan(PIDSDRIVERBUFFER iface, PDSVOLUMEPAN pVolPan)
{
    DWORD vol;
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    TRACE("(%p,%p)\n",iface,pVolPan);
    vol = pVolPan->dwTotalLeftAmpFactor | (pVolPan->dwTotalRightAmpFactor << 16);
                                                                                
    if (wodSetVolume(This->drv->wDevID, vol) != MMSYSERR_NOERROR) {
	WARN("wodSetVolume failed\n");
	return DSERR_INVALIDPARAM;
    }

    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetPosition(PIDSDRIVERBUFFER iface, DWORD dwNewPos)
{
    /* IDsDriverImpl *This = (IDsDriverImpl *)iface; */
    TRACE("(%p,%ld): stub\n",iface,dwNewPos);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_GetPosition(PIDSDRIVERBUFFER iface,
						      LPDWORD lpdwPlay, LPDWORD lpdwWrite)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    WINE_WAVEDEV *      wwo = &(WOutDev[This->drv->wDevID]);
    snd_pcm_uframes_t   hw_ptr;
    snd_pcm_uframes_t   period_size;
    int dir;
    int err;

    if (wwo->hw_params == NULL) return DSERR_GENERIC;

    dir=0;
    err = snd_pcm_hw_params_get_period_size(wwo->hw_params, &period_size, &dir);

    if (wwo->handle == NULL) return DSERR_GENERIC;
    /** we need to track down buffer underruns */
    DSDB_CheckXRUN(This);

    EnterCriticalSection(&This->mmap_crst);
    /* FIXME: snd_pcm_mmap_hw_ptr() should not be accessed by a user app. */
    /*        It will NOT return what why want anyway. */
    hw_ptr = _snd_pcm_mmap_hw_ptr(wwo->handle);
    if (lpdwPlay)
	*lpdwPlay = snd_pcm_frames_to_bytes(wwo->handle, hw_ptr/ period_size  * period_size) % This->mmap_buflen_bytes;
    if (lpdwWrite)
	*lpdwWrite = snd_pcm_frames_to_bytes(wwo->handle, (hw_ptr / period_size + 1) * period_size ) % This->mmap_buflen_bytes;
    LeaveCriticalSection(&This->mmap_crst);

    TRACE("hw_ptr=0x%08x, playpos=%ld, writepos=%ld\n", (unsigned int)hw_ptr, lpdwPlay?*lpdwPlay:-1, lpdwWrite?*lpdwWrite:-1);
    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_Play(PIDSDRIVERBUFFER iface, DWORD dwRes1, DWORD dwRes2, DWORD dwFlags)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    WINE_WAVEDEV *       wwo = &(WOutDev[This->drv->wDevID]);
    snd_pcm_state_t      state;
    int                  err;

    TRACE("(%p,%lx,%lx,%lx)\n",iface,dwRes1,dwRes2,dwFlags);

    if (wwo->handle == NULL) return DSERR_GENERIC;

    state = snd_pcm_state(wwo->handle);
    if ( state == SND_PCM_STATE_SETUP )
    {
	err = snd_pcm_prepare(wwo->handle);
        state = snd_pcm_state(wwo->handle);
    }
    if ( state == SND_PCM_STATE_PREPARED )
     {
	DSDB_MMAPCopy(This);
	err = snd_pcm_start(wwo->handle);
     }
    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_Stop(PIDSDRIVERBUFFER iface)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    WINE_WAVEDEV *    wwo = &(WOutDev[This->drv->wDevID]);
    int               err;
    DWORD             play;
    DWORD             write;

    TRACE("(%p)\n",iface);

    if (wwo->handle == NULL) return DSERR_GENERIC;

    /* ring buffer wrap up detection */
    IDsDriverBufferImpl_GetPosition(iface, &play, &write);
    if ( play > write)
    {
	TRACE("writepos wrapper up\n");
    	return DS_OK;
    }

    if ( ( err = snd_pcm_drop(wwo->handle)) < 0 )
    {
   	ERR("error while stopping pcm: %s\n", snd_strerror(err));
	return DSERR_GENERIC;
    }
    return DS_OK;
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
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%lu)\n",This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDsDriverImpl_Release(PIDSDRIVER iface)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%lu)\n",This, refCount + 1);

    if (refCount)
	return refCount;
    HeapFree(GetProcessHeap(),0,This);
    return 0;
}

static HRESULT WINAPI IDsDriverImpl_GetDriverDesc(PIDSDRIVER iface, PDSDRIVERDESC pDesc)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    TRACE("(%p,%p)\n",iface,pDesc);
    memcpy(pDesc, &(WOutDev[This->wDevID].ds_desc), sizeof(DSDRIVERDESC));
    pDesc->dwFlags = DSDDESC_DOMMSYSTEMOPEN | DSDDESC_DOMMSYSTEMSETFORMAT |
	DSDDESC_USESYSTEMMEMORY | DSDDESC_DONTNEEDPRIMARYLOCK;
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
    /* IDsDriverImpl *This = (IDsDriverImpl *)iface; */
    TRACE("(%p)\n",iface);
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_Close(PIDSDRIVER iface)
{
    /* IDsDriverImpl *This = (IDsDriverImpl *)iface; */
    TRACE("(%p)\n",iface);
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_GetCaps(PIDSDRIVER iface, PDSDRIVERCAPS pCaps)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    TRACE("(%p,%p)\n",iface,pCaps);
    memcpy(pCaps, &(WOutDev[This->wDevID].ds_caps), sizeof(DSDRIVERCAPS));
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
    int err;

    TRACE("(%p,%p,%lx,%lx)\n",iface,pwfx,dwFlags,dwCardAddress);
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

    TRACE("driver created\n");

    /* the HAL isn't much better than the HEL if we can't do mmap() */
    if (!(WOutDev[wDevID].outcaps.dwSupport & WAVECAPS_DIRECTSOUND)) {
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
    memcpy(desc, &(WOutDev[wDevID].ds_desc), sizeof(DSDRIVERDESC));
    return MMSYSERR_NOERROR;
}

/*======================================================================*
*                  Low level WAVE IN implementation			*
*======================================================================*/

/**************************************************************************
* 			widNotifyClient			[internal]
*/
static DWORD widNotifyClient(WINE_WAVEDEV* wwi, WORD wMsg, DWORD dwParam1, DWORD dwParam2)
{
   TRACE("wMsg = 0x%04x dwParm1 = %04lX dwParam2 = %04lX\n", wMsg, dwParam1, dwParam2);

   switch (wMsg) {
   case WIM_OPEN:
   case WIM_CLOSE:
   case WIM_DATA:
       if (wwi->wFlags != DCB_NULL &&
	   !DriverCallback(wwi->waveDesc.dwCallback, wwi->wFlags, (HDRVR)wwi->waveDesc.hWave,
			   wMsg, wwi->waveDesc.dwInstance, dwParam1, dwParam2)) {
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
static DWORD widGetDevCaps(WORD wDevID, LPWAVEOUTCAPSW lpCaps, DWORD dwSize)
{
    TRACE("(%u, %p, %lu);\n", wDevID, lpCaps, dwSize);

    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;

    if (wDevID >= MAX_WAVEINDRV) {
	TRACE("MAX_WAVOUTDRV reached !\n");
	return MMSYSERR_BADDEVICEID;
    }

    memcpy(lpCaps, &WInDev[wDevID].incaps, min(dwSize, sizeof(*lpCaps)));
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widRecorder_ReadHeaders		[internal]
 */
static void widRecorder_ReadHeaders(WINE_WAVEDEV * wwi)
{
    enum win_wm_message tmp_msg;
    DWORD		tmp_param;
    HANDLE		tmp_ev;
    WAVEHDR*		lpWaveHdr;

    while (ALSA_RetrieveRingMessage(&wwi->msgRing, &tmp_msg, &tmp_param, &tmp_ev)) {
        if (tmp_msg == WINE_WM_HEADER) {
	    LPWAVEHDR*	wh;
	    lpWaveHdr = (LPWAVEHDR)tmp_param;
	    lpWaveHdr->lpNext = 0;

	    if (wwi->lpQueuePtr == 0)
		wwi->lpQueuePtr = lpWaveHdr;
	    else {
	        for (wh = &(wwi->lpQueuePtr); *wh; wh = &((*wh)->lpNext));
	        *wh = lpWaveHdr;
	    }
	} else {
            ERR("should only have headers left\n");
        }
    }
}

/**************************************************************************
 * 				widRecorder			[internal]
 */
static	DWORD	CALLBACK	widRecorder(LPVOID pmt)
{
    WORD		uDevID = (DWORD)pmt;
    WINE_WAVEDEV*	wwi = (WINE_WAVEDEV*)&WInDev[uDevID];
    WAVEHDR*		lpWaveHdr;
    DWORD		dwSleepTime;
    DWORD		bytesRead;
    LPVOID		buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, wwi->dwPeriodSize);
    char               *pOffset = buffer;
    enum win_wm_message msg;
    DWORD		param;
    HANDLE		ev;
    DWORD               frames_per_period;

    wwi->state = WINE_WS_STOPPED;
    wwi->dwTotalRecorded = 0;
    wwi->lpQueuePtr = NULL;

    SetEvent(wwi->hStartUpEvent);

    /* make sleep time to be # of ms to output a period */
    dwSleepTime = (1024/*wwi-dwPeriodSize => overrun!*/ * 1000) / wwi->format.Format.nAvgBytesPerSec;
    frames_per_period = snd_pcm_bytes_to_frames(wwi->handle, wwi->dwPeriodSize); 
    TRACE("sleeptime=%ld ms\n", dwSleepTime);

    for (;;) {
	/* wait for dwSleepTime or an event in thread's queue */
	/* FIXME: could improve wait time depending on queue state,
	 * ie, number of queued fragments
	 */
	if (wwi->lpQueuePtr != NULL && wwi->state == WINE_WS_PLAYING)
        {
	    int periods;
	    DWORD frames;
	    DWORD bytes;
	    DWORD read;

            lpWaveHdr = wwi->lpQueuePtr;
            /* read all the fragments accumulated so far */
	    frames = snd_pcm_avail_update(wwi->handle);
	    bytes = snd_pcm_frames_to_bytes(wwi->handle, frames);
	    TRACE("frames = %ld  bytes = %ld\n", frames, bytes);
	    periods = bytes / wwi->dwPeriodSize;
            while ((periods > 0) && (wwi->lpQueuePtr))
            {
		periods--;
		bytes = wwi->dwPeriodSize;
	        TRACE("bytes = %ld\n",bytes);
                if (lpWaveHdr->dwBufferLength - lpWaveHdr->dwBytesRecorded >= wwi->dwPeriodSize)
                {
                    /* directly read fragment in wavehdr */
                    read = wwi->read(wwi->handle, lpWaveHdr->lpData + lpWaveHdr->dwBytesRecorded, frames_per_period);
		    bytesRead = snd_pcm_frames_to_bytes(wwi->handle, read);
			
                    TRACE("bytesRead=%ld (direct)\n", bytesRead);
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

			    wwi->lpQueuePtr = lpNext;
			    widNotifyClient(wwi, WIM_DATA, (DWORD)lpWaveHdr, 0);
			    lpWaveHdr = lpNext;
			}
                    } else {
                        TRACE("read(%s, %p, %ld) failed (%s)\n", wwi->device,
                            lpWaveHdr->lpData + lpWaveHdr->dwBytesRecorded,
                            frames_per_period, strerror(errno));
                    }
                }
		else
		{
                    /* read the fragment in a local buffer */
		    read = wwi->read(wwi->handle, buffer, frames_per_period);
		    bytesRead = snd_pcm_frames_to_bytes(wwi->handle, read);
                    pOffset = buffer;

                    TRACE("bytesRead=%ld (local)\n", bytesRead);

		    if (bytesRead == (DWORD) -1) {
			TRACE("read(%s, %p, %ld) failed (%s)\n", wwi->device,
			      buffer, frames_per_period, strerror(errno));
			continue;
		    }	

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

			    wwi->lpQueuePtr = lpNext;
                            widNotifyClient(wwi, WIM_DATA, (DWORD)lpWaveHdr, 0);

			    lpWaveHdr = lpNext;
			    if (!lpNext && bytesRead) {
				/* before we give up, check for more header messages */
				while (ALSA_PeekRingMessage(&wwi->msgRing, &msg, &param, &ev))
				{
				    if (msg == WINE_WM_HEADER) {
					LPWAVEHDR hdr;
					ALSA_RetrieveRingMessage(&wwi->msgRing, &msg, &param, &ev);
					hdr = ((LPWAVEHDR)param);
					TRACE("msg = %s, hdr = %p, ev = %p\n", getCmdString(msg), hdr, ev);
					hdr->lpNext = 0;
					if (lpWaveHdr == 0) {
					    /* new head of queue */
					    wwi->lpQueuePtr = lpWaveHdr = hdr;
					} else {
					    /* insert buffer at the end of queue */
					    LPWAVEHDR*  wh;
					    for (wh = &(wwi->lpQueuePtr); *wh; wh = &((*wh)->lpNext));
					    *wh = hdr;
					}
				    } else
					break;
				}

				if (lpWaveHdr == 0) {
                                    /* no more buffer to copy data to, but we did read more.
                                     * what hasn't been copied will be dropped
                                     */
                                    WARN("buffer under run! %lu bytes dropped.\n", bytesRead);
                                    wwi->lpQueuePtr = NULL;
                                    break;
				}
                            }
                        }
		    }
		}
            }
	}

        WAIT_OMR(&wwi->msgRing, dwSleepTime);

	while (ALSA_RetrieveRingMessage(&wwi->msgRing, &msg, &param, &ev))
	{
            TRACE("msg=%s param=0x%lx\n", getCmdString(msg), param);
	    switch (msg) {
	    case WINE_WM_PAUSING:
		wwi->state = WINE_WS_PAUSED;
                /*FIXME("Device should stop recording\n");*/
		SetEvent(ev);
		break;
	    case WINE_WM_STARTING:
		wwi->state = WINE_WS_PLAYING;
		snd_pcm_start(wwi->handle);
		SetEvent(ev);
		break;
	    case WINE_WM_HEADER:
		lpWaveHdr = (LPWAVEHDR)param;
		lpWaveHdr->lpNext = 0;

		/* insert buffer at the end of queue */
		{
		    LPWAVEHDR*	wh;
		    for (wh = &(wwi->lpQueuePtr); *wh; wh = &((*wh)->lpNext));
		    *wh = lpWaveHdr;
		}
		break;
	    case WINE_WM_STOPPING:
		if (wwi->state != WINE_WS_STOPPED)
		{
		    snd_pcm_drain(wwi->handle);

		    /* read any headers in queue */
		    widRecorder_ReadHeaders(wwi);

		    /* return current buffer to app */
		    lpWaveHdr = wwi->lpQueuePtr;
		    if (lpWaveHdr)
		    {
		        LPWAVEHDR	lpNext = lpWaveHdr->lpNext;
		        TRACE("stop %p %p\n", lpWaveHdr, lpWaveHdr->lpNext);
		        lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
		        lpWaveHdr->dwFlags |= WHDR_DONE;
		        wwi->lpQueuePtr = lpNext;
		        widNotifyClient(wwi, WIM_DATA, (DWORD)lpWaveHdr, 0);
		    }
		}
		wwi->state = WINE_WS_STOPPED;
		SetEvent(ev);
		break;
	    case WINE_WM_RESETTING:
		if (wwi->state != WINE_WS_STOPPED)
		{
		    snd_pcm_drain(wwi->handle);
		}
		wwi->state = WINE_WS_STOPPED;
    		wwi->dwTotalRecorded = 0;

		/* read any headers in queue */
		widRecorder_ReadHeaders(wwi);

		/* return all buffers to the app */
		for (lpWaveHdr = wwi->lpQueuePtr; lpWaveHdr; lpWaveHdr = lpWaveHdr->lpNext) {
		    TRACE("reset %p %p\n", lpWaveHdr, lpWaveHdr->lpNext);
		    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
		    lpWaveHdr->dwFlags |= WHDR_DONE;
                    wwi->lpQueuePtr = lpWaveHdr->lpNext;
		    widNotifyClient(wwi, WIM_DATA, (DWORD)lpWaveHdr, 0);
		}

		wwi->lpQueuePtr = NULL;
		SetEvent(ev);
		break;
	    case WINE_WM_CLOSING:
		wwi->hThread = 0;
		wwi->state = WINE_WS_CLOSED;
		SetEvent(ev);
		HeapFree(GetProcessHeap(), 0, buffer);
		ExitThread(0);
		/* shouldn't go here */
	    case WINE_WM_UPDATE:
		SetEvent(ev);
		break;

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
    WINE_WAVEDEV*	        wwi;
    snd_pcm_hw_params_t *       hw_params;
    snd_pcm_sw_params_t *       sw_params;
    snd_pcm_access_t            access;
    snd_pcm_format_t            format;
    unsigned int                rate;
    unsigned int                buffer_time = 500000;
    unsigned int                period_time = 10000;
    snd_pcm_uframes_t           buffer_size;
    snd_pcm_uframes_t           period_size;
    int                         flags;
    snd_pcm_t *                 pcm;
    int                         err;
    int                         dir;

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
    if (!supportedFormat(lpDesc->lpFormat)) {
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

    if (wwi->handle != NULL) {
        WARN("already allocated\n");
        return MMSYSERR_ALLOCATED;
    }

    if ((dwFlags & WAVE_DIRECTSOUND) && !(wwi->dwSupport & WAVECAPS_DIRECTSOUND))
	/* not supported, ignore it */
	dwFlags &= ~WAVE_DIRECTSOUND;

    wwi->handle = 0;
    flags = SND_PCM_NONBLOCK;
#if 0
    if ( dwFlags & WAVE_DIRECTSOUND )
	flags |= SND_PCM_ASYNC;
#endif

    if ( (err=snd_pcm_open(&pcm, wwi->device, SND_PCM_STREAM_CAPTURE, flags)) < 0 )
    {
        ERR("Error open: %s\n", snd_strerror(err));
	return MMSYSERR_NOTENABLED;
    }

    wwi->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    memcpy(&wwi->waveDesc, lpDesc, sizeof(WAVEOPENDESC));
    copy_format(lpDesc->lpFormat, &wwi->format);

    if (wwi->format.Format.wBitsPerSample == 0) {
	WARN("Resetting zeroed wBitsPerSample\n");
	wwi->format.Format.wBitsPerSample = 8 *
	    (wwi->format.Format.nAvgBytesPerSec /
	     wwi->format.Format.nSamplesPerSec) /
	    wwi->format.Format.nChannels;
    }

    snd_pcm_hw_params_any(pcm, hw_params);

#define EXIT_ON_ERROR(f,e,txt) do \
{ \
    int err; \
    if ( (err = (f) ) < 0) \
    { \
	WARN(txt ": %s\n", snd_strerror(err)); \
	snd_pcm_close(pcm); \
	return e; \
    } \
} while(0)

    access = SND_PCM_ACCESS_MMAP_INTERLEAVED;
    if ( ( err = snd_pcm_hw_params_set_access(pcm, hw_params, access ) ) < 0) {
        WARN("mmap not available. switching to standard write.\n");
        access = SND_PCM_ACCESS_RW_INTERLEAVED;
	EXIT_ON_ERROR( snd_pcm_hw_params_set_access(pcm, hw_params, access ), MMSYSERR_INVALPARAM, "unable to set access for playback");
	wwi->read = snd_pcm_readi;
    }
    else
	wwi->read = snd_pcm_mmap_readi;

    EXIT_ON_ERROR( snd_pcm_hw_params_set_channels(pcm, hw_params, wwi->format.Format.nChannels), WAVERR_BADFORMAT, "unable to set required channels");

    if ((wwi->format.Format.wFormatTag == WAVE_FORMAT_PCM) ||
        ((wwi->format.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) &&
        IsEqualGUID(&wwi->format.SubFormat, &KSDATAFORMAT_SUBTYPE_PCM))) {
        format = (wwi->format.Format.wBitsPerSample == 8) ? SND_PCM_FORMAT_U8 :
                 (wwi->format.Format.wBitsPerSample == 16) ? SND_PCM_FORMAT_S16_LE :
                 (wwi->format.Format.wBitsPerSample == 24) ? SND_PCM_FORMAT_S24_LE :
                 (wwi->format.Format.wBitsPerSample == 32) ? SND_PCM_FORMAT_S32_LE : -1;
    } else if ((wwi->format.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) &&
        IsEqualGUID(&wwi->format.SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)){
        format = (wwi->format.Format.wBitsPerSample == 32) ? SND_PCM_FORMAT_FLOAT_LE : -1;
    } else if (wwi->format.Format.wFormatTag == WAVE_FORMAT_MULAW) {
        FIXME("unimplemented format: WAVE_FORMAT_MULAW\n");
        snd_pcm_close(pcm);
        return WAVERR_BADFORMAT;
    } else if (wwi->format.Format.wFormatTag == WAVE_FORMAT_ALAW) {
        FIXME("unimplemented format: WAVE_FORMAT_ALAW\n");
        snd_pcm_close(pcm);
        return WAVERR_BADFORMAT;
    } else if (wwi->format.Format.wFormatTag == WAVE_FORMAT_ADPCM) {
        FIXME("unimplemented format: WAVE_FORMAT_ADPCM\n");
        snd_pcm_close(pcm);
        return WAVERR_BADFORMAT;
    } else {
        ERR("invalid format: %0x04x\n", wwi->format.Format.wFormatTag);
        snd_pcm_close(pcm);
        return WAVERR_BADFORMAT;
    }

    EXIT_ON_ERROR( snd_pcm_hw_params_set_format(pcm, hw_params, format), WAVERR_BADFORMAT, "unable to set required format");

    rate = wwi->format.Format.nSamplesPerSec;
    dir = 0;
    err = snd_pcm_hw_params_set_rate_near(pcm, hw_params, &rate, &dir);
    if (err < 0) {
	WARN("Rate %ld Hz not available for playback: %s\n", wwi->format.Format.nSamplesPerSec, snd_strerror(rate));
	snd_pcm_close(pcm);
        return WAVERR_BADFORMAT;
    }
    if (!NearMatch(rate, wwi->format.Format.nSamplesPerSec)) {
	WARN("Rate doesn't match (requested %ld Hz, got %d Hz)\n", wwi->format.Format.nSamplesPerSec, rate);
	snd_pcm_close(pcm);
        return WAVERR_BADFORMAT;
    }
    
    dir=0; 
    EXIT_ON_ERROR( snd_pcm_hw_params_set_buffer_time_near(pcm, hw_params, &buffer_time, &dir), MMSYSERR_INVALPARAM, "unable to set buffer time");
    dir=0; 
    EXIT_ON_ERROR( snd_pcm_hw_params_set_period_time_near(pcm, hw_params, &period_time, &dir), MMSYSERR_INVALPARAM, "unable to set period time");

    EXIT_ON_ERROR( snd_pcm_hw_params(pcm, hw_params), MMSYSERR_INVALPARAM, "unable to set hw params for playback");
    
    dir=0;
    err = snd_pcm_hw_params_get_period_size(hw_params, &period_size, &dir);
    err = snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);

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
    if ( wwi->hw_params )
    	snd_pcm_hw_params_free(wwi->hw_params);
    snd_pcm_hw_params_malloc(&(wwi->hw_params));
    snd_pcm_hw_params_copy(wwi->hw_params, hw_params);

    wwi->dwBufferSize = snd_pcm_frames_to_bytes(pcm, buffer_size);
    wwi->lpQueuePtr = wwi->lpPlayPtr = wwi->lpLoopPtr = NULL;
    wwi->handle = pcm;

    ALSA_InitRingMessage(&wwi->msgRing);

    wwi->count = snd_pcm_poll_descriptors_count (wwi->handle);
    if (wwi->count <= 0) {
	ERR("Invalid poll descriptors count\n");
	return MMSYSERR_ERROR;
    }

    wwi->ufds = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(struct pollfd) * wwi->count);
    if (wwi->ufds == NULL) {
	ERR("No enough memory\n");
	return MMSYSERR_NOMEM;
    }
    if ((err = snd_pcm_poll_descriptors(wwi->handle, wwi->ufds, wwi->count)) < 0) {
	ERR("Unable to obtain poll descriptors for playback: %s\n", snd_strerror(err));
	return MMSYSERR_ERROR;
    }

    wwi->dwPeriodSize = period_size;
    /*if (wwi->dwFragmentSize % wwi->format.Format.nBlockAlign)
	ERR("Fragment doesn't contain an integral number of data blocks\n");
    */
    TRACE("dwPeriodSize=%lu\n", wwi->dwPeriodSize);
    TRACE("wBitsPerSample=%u, nAvgBytesPerSec=%lu, nSamplesPerSec=%lu, nChannels=%u nBlockAlign=%u!\n",
	  wwi->format.Format.wBitsPerSample, wwi->format.Format.nAvgBytesPerSec,
	  wwi->format.Format.nSamplesPerSec, wwi->format.Format.nChannels,
	  wwi->format.Format.nBlockAlign);

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

    return widNotifyClient(wwi, WIM_OPEN, 0L, 0L);
}


/**************************************************************************
 * 				widClose			[internal]
 */
static DWORD widClose(WORD wDevID)
{
    DWORD		ret = MMSYSERR_NOERROR;
    WINE_WAVEDEV*	wwi;

    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].handle == NULL) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    wwi = &WInDev[wDevID];
    if (wwi->lpQueuePtr) {
	WARN("buffers still playing !\n");
	ret = WAVERR_STILLPLAYING;
    } else {
	if (wwi->hThread != INVALID_HANDLE_VALUE) {
	    ALSA_AddRingMessage(&wwi->msgRing, WINE_WM_CLOSING, 0, TRUE);
	}
        ALSA_DestroyRingMessage(&wwi->msgRing);

	snd_pcm_hw_params_free(wwi->hw_params);
	wwi->hw_params = NULL;

        snd_pcm_close(wwi->handle);
	wwi->handle = NULL;

	ret = widNotifyClient(wwi, WIM_CLOSE, 0L, 0L);
    }

    HeapFree(GetProcessHeap(), 0, wwi->ufds);
    return ret;
}

/**************************************************************************
 * 				widAddBuffer			[internal]
 *
 */
static DWORD widAddBuffer(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);

    /* first, do the sanity checks... */
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].handle == NULL) {
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

    ALSA_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_HEADER, (DWORD)lpWaveHdr, FALSE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widStart			[internal]
 *
 */
static DWORD widStart(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);

    /* first, do the sanity checks... */
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].handle == NULL) {
        WARN("bad dev ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    ALSA_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_STARTING, 0, TRUE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widStop			[internal]
 *
 */
static DWORD widStop(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);

    /* first, do the sanity checks... */
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].handle == NULL) {
        WARN("bad dev ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    ALSA_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_STOPPING, 0, TRUE);

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
    ALSA_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_RESETTING, 0, TRUE);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widGetPosition			[internal]
 */
static DWORD widGetPosition(WORD wDevID, LPMMTIME lpTime, DWORD uSize)
{
    WINE_WAVEDEV*	wwi;

    TRACE("(%u, %p, %lu);\n", wDevID, lpTime, uSize);

    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("can't get pos !\n");
	return MMSYSERR_INVALHANDLE;
    }

    if (lpTime == NULL)	{
        WARN("invalid parameter: lpTime = NULL\n");
        return MMSYSERR_INVALPARAM;
    }

    wwi = &WInDev[wDevID];
    ALSA_AddRingMessage(&wwi->msgRing, WINE_WM_UPDATE, 0, TRUE);

    return bytes_to_mmtime(lpTime, wwi->dwTotalRecorded, &wwi->format);
}

/**************************************************************************
 * 				widGetNumDevs			[internal]
 */
static	DWORD	widGetNumDevs(void)
{
    return ALSA_WidNumDevs;
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
 *                              widDsCreate                     [internal]
 */
static DWORD widDsCreate(UINT wDevID, PIDSCDRIVER* drv)
{
    TRACE("(%d,%p)\n",wDevID,drv);

    /* the HAL isn't much better than the HEL if we can't do mmap() */
    FIXME("DirectSoundCapture not implemented\n");
    MESSAGE("The (slower) DirectSound HEL mode will be used instead.\n");
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 *                              widDsDesc                       [internal]
 */
static DWORD widDsDesc(UINT wDevID, PDSDRIVERDESC desc)
{
    memcpy(desc, &(WInDev[wDevID].ds_desc), sizeof(DSDRIVERDESC));
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widMessage (WINEALSA.@)
 */
DWORD WINAPI ALSA_widMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
                             DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%u, %s, %08lX, %08lX, %08lX);\n",
	  wDevID, getMessage(wMsg), dwUser, dwParam1, dwParam2);

    switch (wMsg) {
    case DRVM_INIT:
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
	/* FIXME: Pretend this is supported */
	return 0;
    case WIDM_OPEN:	 	return widOpen		(wDevID, (LPWAVEOPENDESC)dwParam1,	dwParam2);
    case WIDM_CLOSE:	 	return widClose		(wDevID);
    case WIDM_ADDBUFFER:	return widAddBuffer	(wDevID, (LPWAVEHDR)dwParam1,		dwParam2);
    case WIDM_PREPARE:	 	return MMSYSERR_NOTSUPPORTED;
    case WIDM_UNPREPARE: 	return MMSYSERR_NOTSUPPORTED;
    case WIDM_GETDEVCAPS:	return widGetDevCaps	(wDevID, (LPWAVEOUTCAPSW)dwParam1,	dwParam2);
    case WIDM_GETNUMDEVS:	return widGetNumDevs	();
    case WIDM_GETPOS:	 	return widGetPosition	(wDevID, (LPMMTIME)dwParam1, 		dwParam2);
    case WIDM_RESET:		return widReset		(wDevID);
    case WIDM_START: 		return widStart	(wDevID, (LPWAVEHDR)dwParam1, 		dwParam2);
    case WIDM_STOP: 		return widStop	(wDevID, (LPWAVEHDR)dwParam1, 		dwParam2);
    case DRV_QUERYDEVICEINTERFACESIZE: return widDevInterfaceSize       (wDevID, (LPDWORD)dwParam1);
    case DRV_QUERYDEVICEINTERFACE:     return widDevInterface           (wDevID, (PWCHAR)dwParam1, dwParam2);
    case DRV_QUERYDSOUNDIFACE:	return widDsCreate   (wDevID, (PIDSCDRIVER*)dwParam1);
    case DRV_QUERYDSOUNDDESC:	return widDsDesc     (wDevID, (PDSDRIVERDESC)dwParam1);
    default:
	FIXME("unknown message %d!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

#else

/**************************************************************************
 * 				widMessage (WINEALSA.@)
 */
DWORD WINAPI ALSA_widMessage(WORD wDevID, WORD wMsg, DWORD dwUser,
                             DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08lX, %08lX, %08lX):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

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
