/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Sample Wine Driver for Open Sound System (featured in Linux and FreeBSD)
 *
 * Copyright 1994 Martin Ayotte
 *           1999 Eric Pouech (async playing in waveOut/waveIn)
 *	     2000 Eric Pouech (loops in waveOut)
 *           2002 Eric Pouech (full duplex)
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
/*
 * FIXME:
 *	pause in waveOut does not work correctly in loop mode
 */

/*#define EMULATE_SB16*/

/* unless someone makes a wineserver kernel module, Unix pipes are faster than win32 events */
#define USE_PIPE_SYNC

/* an exact wodGetPosition is usually not worth the extra context switches,
 * as we're going to have near fragment accuracy anyway */
/* #define EXACT_WODPOSITION */

#include "config.h"

#include <stdlib.h>
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
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif

#include "windef.h"
#include "wingdi.h"
#include "winerror.h"
#include "wine/winuser16.h"
#include "mmddk.h"
#include "dsound.h"
#include "dsdriver.h"
#include "oss.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wave);

/* Allow 1% deviation for sample rates (some ES137x cards) */
#define NEAR_MATCH(rate1,rate2) (((100*((int)(rate1)-(int)(rate2)))/(rate1))==0)

#ifdef HAVE_OSS

#define MAX_WAVEDRV 	(3)

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
} OSS_MSG;

/* implement an in-process message ring for better performance
 * (compared to passing thru the server)
 * this ring will be used by the input (resp output) record (resp playback) routine
 */
typedef struct {
    /* FIXME: this could be made a dynamically growing array (if needed) */
    /* maybe it's needed, a Humongous game manages to transmit 128 messages at once at startup */
#define OSS_RING_BUFFER_SIZE	192
    OSS_MSG			messages[OSS_RING_BUFFER_SIZE];
    int				msg_tosave;
    int				msg_toget;
#ifdef USE_PIPE_SYNC
    int				msg_pipe[2];
#else
    HANDLE			msg_event;
#endif
    CRITICAL_SECTION		msg_crst;
} OSS_MSG_RING;

typedef struct tagOSS_DEVICE {
    const char*                 dev_name;
    const char*                 mixer_name;
    unsigned                    open_count;
    WAVEOUTCAPSA                out_caps;
    WAVEINCAPSA		        in_caps;
    unsigned                    open_access;
    int                         fd;
    DWORD                       owner_tid;
    unsigned                    sample_rate;
    unsigned                    stereo;
    unsigned                    format;
    unsigned                    audio_fragment;
    BOOL                        full_duplex;
    BOOL                        bTriggerSupport;
} OSS_DEVICE;

static OSS_DEVICE   OSS_Devices[MAX_WAVEDRV];

typedef struct {
    OSS_DEVICE*                 ossdev;
    volatile int		state;			/* one of the WINE_WS_ manifest constants */
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    PCMWAVEFORMAT		format;

    /* OSS information */
    DWORD			dwFragmentSize;		/* size of OSS buffer fragment */
    DWORD                       dwBufferSize;           /* size of whole OSS buffer in bytes */
    LPWAVEHDR			lpQueuePtr;		/* start of queued WAVEHDRs (waiting to be notified) */
    LPWAVEHDR			lpPlayPtr;		/* start of not yet fully played buffers */
    DWORD			dwPartialOffset;	/* Offset of not yet written bytes in lpPlayPtr */

    LPWAVEHDR			lpLoopPtr;              /* pointer of first buffer in loop, if any */
    DWORD			dwLoops;		/* private copy of loop counter */

    DWORD			dwPlayedTotal;		/* number of bytes actually played since opening */
    DWORD                       dwWrittenTotal;         /* number of bytes written to OSS buffer since opening */
    BOOL                        bNeedPost;              /* whether audio still needs to be physically started */

    /* synchronization stuff */
    HANDLE			hStartUpEvent;
    HANDLE			hThread;
    DWORD			dwThreadID;
    OSS_MSG_RING		msgRing;

    /* DirectSound stuff */
    LPBYTE			mapping;
    DWORD			maplen;
} WINE_WAVEOUT;

typedef struct {
    OSS_DEVICE*                 ossdev;
    volatile int		state;
    DWORD			dwFragmentSize;		/* OpenSound '/dev/dsp' give us that size */
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    PCMWAVEFORMAT		format;
    LPWAVEHDR			lpQueuePtr;
    DWORD			dwTotalRecorded;

    /* synchronization stuff */
    HANDLE			hThread;
    DWORD			dwThreadID;
    HANDLE			hStartUpEvent;
    OSS_MSG_RING		msgRing;
} WINE_WAVEIN;

static WINE_WAVEOUT	WOutDev   [MAX_WAVEDRV];
static WINE_WAVEIN	WInDev    [MAX_WAVEDRV];
static unsigned         numOutDev;
static unsigned         numInDev;

static DWORD wodDsCreate(UINT wDevID, PIDSDRIVER* drv);

/* These strings used only for tracing */
static const char *wodPlayerCmdString[] = {
    "WINE_WM_PAUSING",
    "WINE_WM_RESTARTING",
    "WINE_WM_RESETTING",
    "WINE_WM_HEADER",
    "WINE_WM_UPDATE",
    "WINE_WM_BREAKLOOP",
    "WINE_WM_CLOSING",
};

/*======================================================================*
 *                  Low level WAVE implementation			*
 *======================================================================*/

/******************************************************************
 *		OSS_RawOpenDevice
 *
 * Low level device opening (from values stored in ossdev)
 */
static DWORD      OSS_RawOpenDevice(OSS_DEVICE* ossdev, int strict_format)
{
    int fd, val, err;

    if ((fd = open(ossdev->dev_name, ossdev->open_access|O_NDELAY, 0)) == -1)
    {
        WARN("Couldn't open %s (%s)\n", ossdev->dev_name, strerror(errno));
        return (errno == EBUSY) ? MMSYSERR_ALLOCATED : MMSYSERR_ERROR;
    }
    fcntl(fd, F_SETFD, 1); /* set close on exec flag */
    /* turn full duplex on if it has been requested */
    if (ossdev->open_access == O_RDWR && ossdev->full_duplex)
        ioctl(fd, SNDCTL_DSP_SETDUPLEX, 0);

    if (ossdev->audio_fragment)
    {
        ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &ossdev->audio_fragment);
    }

    /* First size and stereo then samplerate */
    err=MMSYSERR_NOERROR;
    if (ossdev->format)
    {
        val = ossdev->format;
        ioctl(fd, SNDCTL_DSP_SETFMT, &ossdev->format);
        if (val != ossdev->format) {
            TRACE("Can't set format to %d (returned %d)\n", val, ossdev->format);
            err=WAVERR_BADFORMAT;
            if (strict_format)
                goto error;
        }
    }
    if (ossdev->stereo)
    {
        val = ossdev->stereo;
        ioctl(fd, SNDCTL_DSP_STEREO, &ossdev->stereo);
        if (val != ossdev->stereo) {
            TRACE("Can't set stereo to %u (returned %d)\n", val, ossdev->stereo);
            err=WAVERR_BADFORMAT;
            if (strict_format)
                goto error;
        }
    }
    if (ossdev->sample_rate)
    {
        val = ossdev->sample_rate;
        ioctl(fd, SNDCTL_DSP_SPEED, &ossdev->sample_rate);
        if (!NEAR_MATCH(val, ossdev->sample_rate)) {
            TRACE("Can't set sample_rate to %u (returned %d)\n", val, ossdev->sample_rate);
            err=WAVERR_BADFORMAT;
            if (strict_format)
                goto error;
        }
    }
    ossdev->fd = fd;
    return err;

error:
    close(fd);
    return err;
}

/******************************************************************
 *		OSS_OpenDevice
 *
 * since OSS has poor capabilities in full duplex, we try here to let a program
 * open the device for both waveout and wavein streams...
 * this is hackish, but it's the way OSS interface is done...
 */
static DWORD OSS_OpenDevice(OSS_DEVICE* ossdev, unsigned req_access,
                            int* frag, int strict_format,
                            int sample_rate, int stereo, int fmt)
{
    DWORD       ret;

    if (ossdev->full_duplex && (req_access == O_RDONLY || req_access == O_WRONLY))
        req_access = O_RDWR;

    /* FIXME: this should be protected, and it also contains a race with OSS_CloseDevice */
    if (ossdev->open_count == 0)
    {
	if (access(ossdev->dev_name, 0) != 0) return MMSYSERR_NODRIVER;

        ossdev->audio_fragment = (frag) ? *frag : 0;
        ossdev->sample_rate = sample_rate;
        ossdev->stereo = stereo;
        ossdev->format = fmt;
        ossdev->open_access = req_access;
        ossdev->owner_tid = GetCurrentThreadId();

        if ((ret = OSS_RawOpenDevice(ossdev,strict_format)) != MMSYSERR_NOERROR) return ret;
    }
    else
    {
        /* check we really open with the same parameters */
        if (ossdev->open_access != req_access)
        {
            WARN("Mismatch in access...\n");
            return WAVERR_BADFORMAT;
        }
        /* FIXME: if really needed, we could do, in this case, on the fly
         * PCM conversion (using the MSACM ad hoc driver)
         */
        if (ossdev->audio_fragment != (frag ? *frag : 0) ||
            ossdev->sample_rate != sample_rate ||
            ossdev->stereo != stereo ||
            ossdev->format != fmt)
        {
            WARN("FullDuplex: mismatch in PCM parameters for input and output\n"
                 "OSS doesn't allow us different parameters\n"
                 "audio_frag(%x/%x) sample_rate(%d/%d) stereo(%d/%d) fmt(%d/%d)\n",
                 ossdev->audio_fragment, frag ? *frag : 0,
                 ossdev->sample_rate, sample_rate,
                 ossdev->stereo, stereo,
                 ossdev->format, fmt);
            return WAVERR_BADFORMAT;
        }
        if (GetCurrentThreadId() != ossdev->owner_tid)
        {
            WARN("Another thread is trying to access audio...\n");
            return MMSYSERR_ERROR;
        }
    }

    ossdev->open_count++;

    return MMSYSERR_NOERROR;
}

/******************************************************************
 *		OSS_CloseDevice
 *
 *
 */
static void	OSS_CloseDevice(OSS_DEVICE* ossdev)
{
    if (--ossdev->open_count == 0)
    {
       /* reset the device before we close it in case it is in a bad state */
       ioctl(ossdev->fd, SNDCTL_DSP_RESET, 0);
       close(ossdev->fd);
    }
}

/******************************************************************
 *		OSS_ResetDevice
 *
 * Resets the device. OSS Commercial requires the device to be closed
 * after a SNDCTL_DSP_RESET ioctl call... this function implements
 * this behavior...
 */
static DWORD     OSS_ResetDevice(OSS_DEVICE* ossdev)
{
    DWORD       ret;

    if (ioctl(ossdev->fd, SNDCTL_DSP_RESET, NULL) == -1) 
    {
	perror("ioctl SNDCTL_DSP_RESET");
        return -1;
    }
    TRACE("Changing fd from %d to ", ossdev->fd);
    close(ossdev->fd);
    ret = OSS_RawOpenDevice(ossdev, 1);
    TRACE("%d\n", ossdev->fd);
    return ret;
}

const static int win_std_oss_fmts[2]={AFMT_U8,AFMT_S16_LE};
const static int win_std_rates[5]={96000,48000,44100,22050,11025};
const static int win_std_formats[2][2][5]=
    {{{WAVE_FORMAT_96M08, WAVE_FORMAT_48M08, WAVE_FORMAT_4M08,
       WAVE_FORMAT_2M08,  WAVE_FORMAT_1M08},
      {WAVE_FORMAT_96S08, WAVE_FORMAT_48S08, WAVE_FORMAT_4S08,
       WAVE_FORMAT_2S08,  WAVE_FORMAT_1S08}},
     {{WAVE_FORMAT_96M16, WAVE_FORMAT_48M16, WAVE_FORMAT_4M16,
       WAVE_FORMAT_2M16,  WAVE_FORMAT_1M16},
      {WAVE_FORMAT_96S16, WAVE_FORMAT_48S16, WAVE_FORMAT_4S16,
       WAVE_FORMAT_2S16,  WAVE_FORMAT_1S16}},
    };

/******************************************************************
 *		OSS_WaveOutInit
 *
 *
 */
static BOOL OSS_WaveOutInit(OSS_DEVICE* ossdev)
{
    int rc,arg;
    int f,c,r;

    if (OSS_OpenDevice(ossdev, O_WRONLY, NULL, 0, 0, 0, 0) != 0) return FALSE;
    ioctl(ossdev->fd, SNDCTL_DSP_RESET, 0);

    /* FIXME: some programs compare this string against the content of the
     * registry for MM drivers. The names have to match in order for the
     * program to work (e.g. MS win9x mplayer.exe)
     */
#ifdef EMULATE_SB16
    ossdev->out_caps.wMid = 0x0002;
    ossdev->out_caps.wPid = 0x0104;
    strcpy(ossdev->out_caps.szPname, "SB16 Wave Out");
#else
    ossdev->out_caps.wMid = 0x00FF; /* Manufac ID */
    ossdev->out_caps.wPid = 0x0001; /* Product ID */
    /*    strcpy(ossdev->out_caps.szPname, "OpenSoundSystem WAVOUT Driver");*/
    strcpy(ossdev->out_caps.szPname, "CS4236/37/38");
#endif
    ossdev->out_caps.vDriverVersion = 0x0100;
    ossdev->out_caps.wChannels = 1;
    ossdev->out_caps.dwFormats = 0x00000000;
    ossdev->out_caps.wReserved1 = 0;
    ossdev->out_caps.dwSupport = WAVECAPS_VOLUME;

    if (WINE_TRACE_ON(wave)) {
        /* Note that this only reports the formats supported by the hardware.
         * The driver may support other formats and do the conversions in
         * software which is why we don't use this value
         */
        int oss_mask;
        ioctl(ossdev->fd, SNDCTL_DSP_GETFMTS, &oss_mask);
        TRACE("OSS dsp out mask=%08x\n", oss_mask);
    }

    /* We must first set the format and the stereo mode as some sound cards
     * may support 44kHz mono but not 44kHz stereo. Also we must
     * systematically check the return value of these ioctls as they will
     * always succeed (see OSS Linux) but will modify the parameter to match
     * whatever they support. The OSS specs also say we must first set the
     * sample size, then the stereo and then the sample rate.
     */
    for (f=0;f<2;f++) {
        arg=win_std_oss_fmts[f];
        rc=ioctl(ossdev->fd, SNDCTL_DSP_SAMPLESIZE, &arg);
        if (rc!=0 || arg!=win_std_oss_fmts[f])
            continue;

        for (c=0;c<2;c++) {
            arg=c;
            rc=ioctl(ossdev->fd, SNDCTL_DSP_STEREO, &arg);
            if (rc!=0 || arg!=c)
                continue;
            if (c==1) {
                ossdev->out_caps.wChannels=2;
                ossdev->out_caps.dwSupport|=WAVECAPS_LRVOLUME;
            }

            for (r=0;r<sizeof(win_std_rates)/sizeof(*win_std_rates);r++) {
                arg=win_std_rates[r];
                rc=ioctl(ossdev->fd, SNDCTL_DSP_SPEED, &arg);
                TRACE("DSP_SPEED: rc=%d returned %d for %dx%dx%d\n",
                      rc,arg,win_std_rates[r],win_std_oss_fmts[f],c+1);
                if (rc==0 && NEAR_MATCH(arg,win_std_rates[r]))
                    ossdev->out_caps.dwFormats|=win_std_formats[f][c][r];
            }
        }
    }

    if (ioctl(ossdev->fd, SNDCTL_DSP_GETCAPS, &arg) == 0) {
        TRACE("OSS dsp out caps=%08X\n", arg);
        if ((arg & DSP_CAP_REALTIME) && !(arg & DSP_CAP_BATCH)) {
            ossdev->out_caps.dwSupport |= WAVECAPS_SAMPLEACCURATE;
        }
        /* well, might as well use the DirectSound cap flag for something */
        if ((arg & DSP_CAP_TRIGGER) && (arg & DSP_CAP_MMAP) &&
            !(arg & DSP_CAP_BATCH))
            ossdev->out_caps.dwSupport |= WAVECAPS_DIRECTSOUND;
    }
    OSS_CloseDevice(ossdev);
    TRACE("out dwFormats = %08lX, dwSupport = %08lX\n",
          ossdev->out_caps.dwFormats, ossdev->out_caps.dwSupport);
    return TRUE;
}

/******************************************************************
 *		OSS_WaveInInit
 *
 *
 */
static BOOL OSS_WaveInInit(OSS_DEVICE* ossdev)
{
    int rc,arg;
    int f,c,r;

    if (OSS_OpenDevice(ossdev, O_RDONLY, NULL, 0, 0, 0, 0) != 0) return FALSE;
    ioctl(ossdev->fd, SNDCTL_DSP_RESET, 0);

    /* See comment in OSS_WaveOutInit */
#ifdef EMULATE_SB16
    ossdev->in_caps.wMid = 0x0002;
    ossdev->in_caps.wPid = 0x0004;
    strcpy(ossdev->in_caps.szPname, "SB16 Wave In");
#else
    ossdev->in_caps.wMid = 0x00FF; /* Manufac ID */
    ossdev->in_caps.wPid = 0x0001; /* Product ID */
    strcpy(ossdev->in_caps.szPname, "OpenSoundSystem WAVIN Driver");
#endif
    ossdev->in_caps.dwFormats = 0x00000000;
    ossdev->in_caps.wChannels = 1;
    ossdev->in_caps.wReserved1 = 0;
    ossdev->bTriggerSupport = FALSE;

    if (WINE_TRACE_ON(wave)) {
        /* Note that this only reports the formats supported by the hardware.
         * The driver may support other formats and do the conversions in
         * software which is why we don't use this value
         */
        int oss_mask;
        ioctl(ossdev->fd, SNDCTL_DSP_GETFMTS, &oss_mask);
        TRACE("OSS dsp out mask=%08x\n", oss_mask);
    }

    /* See the comment in OSS_WaveOutInit */
    for (f=0;f<2;f++) {
        arg=win_std_oss_fmts[f];
        rc=ioctl(ossdev->fd, SNDCTL_DSP_SAMPLESIZE, &arg);
        if (rc!=0 || arg!=win_std_oss_fmts[f])
            continue;

        for (c=0;c<2;c++) {
            arg=c;
            rc=ioctl(ossdev->fd, SNDCTL_DSP_STEREO, &arg);
            if (rc!=0 || arg!=c)
                continue;
            if (c==1) {
                ossdev->in_caps.wChannels=2;
            }

            for (r=0;r<sizeof(win_std_rates)/sizeof(*win_std_rates);r++) {
                arg=win_std_rates[r];
                rc=ioctl(ossdev->fd, SNDCTL_DSP_SPEED, &arg);
                TRACE("DSP_SPEED: rc=%d returned %d for %dx%dx%d\n",rc,arg,win_std_rates[r],win_std_oss_fmts[f],c+1);
                if (rc==0 && NEAR_MATCH(arg,win_std_rates[r]))
                    ossdev->in_caps.dwFormats|=win_std_formats[f][c][r];
            }
        }
    }

    if (ioctl(ossdev->fd, SNDCTL_DSP_GETCAPS, &arg) == 0) {
        TRACE("OSS dsp in caps=%08X\n", arg);
        if (arg & DSP_CAP_TRIGGER)
            ossdev->bTriggerSupport = TRUE;
    }
    OSS_CloseDevice(ossdev);
    TRACE("in dwFormats = %08lX\n", ossdev->in_caps.dwFormats);
    return TRUE;
}

/******************************************************************
 *		OSS_WaveFullDuplexInit
 *
 *
 */
static void OSS_WaveFullDuplexInit(OSS_DEVICE* ossdev)
{
    int 	caps;

    if (OSS_OpenDevice(ossdev, O_RDWR, NULL, 0, 0, 0, 0) != 0) return;
    if (ioctl(ossdev->fd, SNDCTL_DSP_GETCAPS, &caps) == 0)
    {
	ossdev->full_duplex = (caps & DSP_CAP_DUPLEX);
    }
    OSS_CloseDevice(ossdev);
}

/******************************************************************
 *		OSS_WaveInit
 *
 * Initialize internal structures from OSS information
 */
LONG OSS_WaveInit(void)
{
    int 	i;

    /* FIXME: only one device is supported */
    memset(&OSS_Devices, 0, sizeof(OSS_Devices));
    /* FIXME: should check that dsp actually points to dsp0, or that dsp0 exists
     * we should also be able to configure (bitmap) which devices we want to use...
     * - or even store the name of all drivers in our configuration
     */
    OSS_Devices[0].dev_name   = "/dev/dsp";
    OSS_Devices[0].mixer_name = "/dev/mixer";
    OSS_Devices[1].dev_name   = "/dev/dsp1";
    OSS_Devices[1].mixer_name = "/dev/mixer1";
    OSS_Devices[2].dev_name   = "/dev/dsp2";
    OSS_Devices[2].mixer_name = "/dev/mixer2";

    /* start with output device */
    for (i = 0; i < MAX_WAVEDRV; ++i)
    {
        if (OSS_WaveOutInit(&OSS_Devices[i]))
        {
            WOutDev[numOutDev].state = WINE_WS_CLOSED;
            WOutDev[numOutDev].ossdev = &OSS_Devices[i];
            numOutDev++;
        }
    }

    /* then do input device */
    for (i = 0; i < MAX_WAVEDRV; ++i)
    {
        if (OSS_WaveInInit(&OSS_Devices[i]))
        {
            WInDev[numInDev].state = WINE_WS_CLOSED;
            WInDev[numInDev].ossdev = &OSS_Devices[i];
            numInDev++;
        }
    }

    /* finish with the full duplex bits */
    for (i = 0; i < MAX_WAVEDRV; i++)
        OSS_WaveFullDuplexInit(&OSS_Devices[i]);

    return 0;
}

/******************************************************************
 *		OSS_InitRingMessage
 *
 * Initialize the ring of messages for passing between driver's caller and playback/record
 * thread
 */
static int OSS_InitRingMessage(OSS_MSG_RING* omr)
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
    omr->msg_event = CreateEventA(NULL, FALSE, FALSE, NULL);
#endif
    memset(omr->messages, 0, sizeof(OSS_MSG) * OSS_RING_BUFFER_SIZE);
    InitializeCriticalSection(&omr->msg_crst);
    return 0;
}

/******************************************************************
 *		OSS_DestroyRingMessage
 *
 */
static int OSS_DestroyRingMessage(OSS_MSG_RING* omr)
{
#ifdef USE_PIPE_SYNC
    close(omr->msg_pipe[0]);
    close(omr->msg_pipe[1]);
#else
    CloseHandle(omr->msg_event);
#endif
    DeleteCriticalSection(&omr->msg_crst);
    return 0;
}

/******************************************************************
 *		OSS_AddRingMessage
 *
 * Inserts a new message into the ring (should be called from DriverProc derivated routines)
 */
static int OSS_AddRingMessage(OSS_MSG_RING* omr, enum win_wm_message msg, DWORD param, BOOL wait)
{
    HANDLE	hEvent = INVALID_HANDLE_VALUE;

    EnterCriticalSection(&omr->msg_crst);
    if ((omr->msg_toget == ((omr->msg_tosave + 1) % OSS_RING_BUFFER_SIZE))) /* buffer overflow ? */
    {
	ERR("buffer overflow !?\n");
        LeaveCriticalSection(&omr->msg_crst);
	return 0;
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
        omr->msg_toget = (omr->msg_toget + OSS_RING_BUFFER_SIZE - 1) % OSS_RING_BUFFER_SIZE;

        omr->messages[omr->msg_toget].msg = msg;
        omr->messages[omr->msg_toget].param = param;
        omr->messages[omr->msg_toget].hEvent = hEvent;
    }
    else
    {
        omr->messages[omr->msg_tosave].msg = msg;
        omr->messages[omr->msg_tosave].param = param;
        omr->messages[omr->msg_tosave].hEvent = INVALID_HANDLE_VALUE;
        omr->msg_tosave = (omr->msg_tosave + 1) % OSS_RING_BUFFER_SIZE;
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
 *		OSS_RetrieveRingMessage
 *
 * Get a message from the ring. Should be called by the playback/record thread.
 */
static int OSS_RetrieveRingMessage(OSS_MSG_RING* omr,
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
    omr->msg_toget = (omr->msg_toget + 1) % OSS_RING_BUFFER_SIZE;
    CLEAR_OMR(omr);
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
	    !DriverCallback(wwo->waveDesc.dwCallback, wwo->wFlags,
			    (HDRVR)wwo->waveDesc.hWave, wMsg,
			    wwo->waveDesc.dwInstance, dwParam1, dwParam2)) {
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
static BOOL wodUpdatePlayedTotal(WINE_WAVEOUT* wwo, audio_buf_info* info)
{
    audio_buf_info dspspace;
    if (!info) info = &dspspace;

    if (ioctl(wwo->ossdev->fd, SNDCTL_DSP_GETOSPACE, info) < 0) {
        ERR("ioctl can't 'SNDCTL_DSP_GETOSPACE' !\n");
        return FALSE;
    }
    wwo->dwPlayedTotal = wwo->dwWrittenTotal - (wwo->dwBufferSize - info->bytes);
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
 * 			     wodPlayer_DSPWait			[internal]
 * Returns the number of milliseconds to wait for the DSP buffer to write
 * one fragment.
 */
static DWORD wodPlayer_DSPWait(const WINE_WAVEOUT *wwo)
{
    /* time for one fragment to be played */
    return wwo->dwFragmentSize * 1000 / wwo->format.wf.nAvgBytesPerSec;
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

    if (lpWaveHdr->reserved < wwo->dwPlayedTotal) {
	dwMillis = 1;
    } else {
	dwMillis = (lpWaveHdr->reserved - wwo->dwPlayedTotal) * 1000 / wwo->format.wf.nAvgBytesPerSec;
	if (!dwMillis) dwMillis = 1;
    }

    return dwMillis;
}


/**************************************************************************
 * 			     wodPlayer_WriteMaxFrags            [internal]
 * Writes the maximum number of bytes possible to the DSP and returns
 * TRUE iff the current playPtr has been fully played
 */
static BOOL wodPlayer_WriteMaxFrags(WINE_WAVEOUT* wwo, DWORD* bytes)
{
    DWORD       dwLength = wwo->lpPlayPtr->dwBufferLength - wwo->dwPartialOffset;
    DWORD       toWrite = min(dwLength, *bytes);
    int         written;
    BOOL        ret = FALSE;

    TRACE("Writing wavehdr %p.%lu[%lu]/%lu\n",
          wwo->lpPlayPtr, wwo->dwPartialOffset, wwo->lpPlayPtr->dwBufferLength, toWrite);

    if (toWrite > 0)
    {
        written = write(wwo->ossdev->fd, wwo->lpPlayPtr->lpData + wwo->dwPartialOffset, toWrite);
        if (written <= 0) return FALSE;
    }
    else
        written = 0;

    if (written >= dwLength) {
        /* If we wrote all current wavehdr, skip to the next one */
        wodPlayer_PlayPtrNext(wwo);
        ret = TRUE;
    } else {
        /* Remove the amount written */
        wwo->dwPartialOffset += written;
    }
    *bytes -= written;
    wwo->dwWrittenTotal += written;

    return ret;
}


/**************************************************************************
 * 				wodPlayer_NotifyCompletions	[internal]
 *
 * Notifies and remove from queue all wavehdrs which have been played to
 * the speaker (ie. they have cleared the OSS buffer).  If force is true,
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
             lpWaveHdr->reserved <= wwo->dwPlayedTotal))) {

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
    wodUpdatePlayedTotal(wwo, NULL);
    /* updates current notify list */
    wodPlayer_NotifyCompletions(wwo, FALSE);

    /* flush all possible output */
    if (OSS_ResetDevice(wwo->ossdev) != MMSYSERR_NOERROR)
    {
	wwo->hThread = 0;
	wwo->state = WINE_WS_STOPPED;
	ExitThread(-1);
    }

    if (reset) {
        enum win_wm_message	msg;
        DWORD		        param;
        HANDLE		        ev;

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
        while (OSS_RetrieveRingMessage(&wwo->msgRing, &msg, &param, &ev))
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
    } else {
        if (wwo->lpLoopPtr) {
            /* complicated case, not handled yet (could imply modifying the loop counter */
            FIXME("Pausing while in loop isn't correctly handled yet, except strange results\n");
            wwo->lpPlayPtr = wwo->lpLoopPtr;
            wwo->dwPartialOffset = 0;
            wwo->dwWrittenTotal = wwo->dwPlayedTotal; /* this is wrong !!! */
        } else {
            LPWAVEHDR   ptr;
            DWORD       sz = wwo->dwPartialOffset;

            /* reset all the data as if we had written only up to lpPlayedTotal bytes */
            /* compute the max size playable from lpQueuePtr */
            for (ptr = wwo->lpQueuePtr; ptr != wwo->lpPlayPtr; ptr = ptr->lpNext) {
                sz += ptr->dwBufferLength;
            }
            /* because the reset lpPlayPtr will be lpQueuePtr */
            if (wwo->dwWrittenTotal > wwo->dwPlayedTotal + sz) ERR("grin\n");
            wwo->dwPartialOffset = sz - (wwo->dwWrittenTotal - wwo->dwPlayedTotal);
            wwo->dwWrittenTotal = wwo->dwPlayedTotal;
            wwo->lpPlayPtr = wwo->lpQueuePtr;
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

    while (OSS_RetrieveRingMessage(&wwo->msgRing, &msg, &param, &ev)) {
	TRACE("Received %s %lx\n", wodPlayerCmdString[msg - WM_USER - 1], param);
	switch (msg) {
	case WINE_WM_PAUSING:
	    wodPlayer_Reset(wwo, FALSE);
	    SetEvent(ev);
	    break;
	case WINE_WM_RESTARTING:
            if (wwo->state == WINE_WS_PAUSED)
            {
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
	    wodPlayer_Reset(wwo, TRUE);
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
    audio_buf_info dspspace;
    DWORD       availInQ;

    wodUpdatePlayedTotal(wwo, &dspspace);
    availInQ = dspspace.bytes;
    TRACE("fragments=%d/%d, fragsize=%d, bytes=%d\n",
	  dspspace.fragments, dspspace.fragstotal, dspspace.fragsize, dspspace.bytes);

    /* input queue empty and output buffer with less than one fragment to play 
     * actually some cards do not play the fragment before the last if this one is partially feed
     * so we need to test for full the availability of 2 fragments
     */
    if (!wwo->lpPlayPtr && wwo->dwBufferSize < availInQ + 2 * wwo->dwFragmentSize && 
        !wwo->bNeedPost) {
	TRACE("Run out of wavehdr:s...\n");
        return INFINITE;
    }

    /* no more room... no need to try to feed */
    if (dspspace.fragments != 0) {
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

        if (wwo->bNeedPost) {
            /* OSS doesn't start before it gets either 2 fragments or a SNDCTL_DSP_POST;
             * if it didn't get one, we give it the other */
            if (wwo->dwBufferSize < availInQ + 2 * wwo->dwFragmentSize)
                ioctl(wwo->ossdev->fd, SNDCTL_DSP_POST, 0);
            wwo->bNeedPost = FALSE;
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
	WAIT_OMR(&wwo->msgRing, dwSleepTime);
	wodPlayer_ProcessMessages(wwo);
	if (wwo->state == WINE_WS_PLAYING) {
	    dwNextFeedTime = wodPlayer_FeedDSP(wwo);
	    dwNextNotifyTime = wodPlayer_NotifyCompletions(wwo, FALSE);
	    if (dwNextFeedTime == INFINITE) {
		/* FeedDSP ran out of data, but before flushing, */
		/* check that a notification didn't give us more */
		wodPlayer_ProcessMessages(wwo);
		if (!wwo->lpPlayPtr) {
		    TRACE("flushing\n");
		    ioctl(wwo->ossdev->fd, SNDCTL_DSP_SYNC, 0);
		    wwo->dwPlayedTotal = wwo->dwWrittenTotal;
                    dwNextNotifyTime = wodPlayer_NotifyCompletions(wwo, FALSE);
		} else {
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
static DWORD wodGetDevCaps(WORD wDevID, LPWAVEOUTCAPSA lpCaps, DWORD dwSize)
{
    TRACE("(%u, %p, %lu);\n", wDevID, lpCaps, dwSize);

    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;

    if (wDevID >= numOutDev) {
	TRACE("numOutDev reached !\n");
	return MMSYSERR_BADDEVICEID;
    }

    memcpy(lpCaps, &WOutDev[wDevID].ossdev->out_caps, min(dwSize, sizeof(*lpCaps)));
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodOpen				[internal]
 */
static DWORD wodOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    int			audio_fragment;
    WINE_WAVEOUT*	wwo;
    audio_buf_info      info;
    DWORD               ret;

    TRACE("(%u, %p, %08lX);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN("Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= numOutDev) {
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

    if ((dwFlags & WAVE_DIRECTSOUND) && 
        !(wwo->ossdev->out_caps.dwSupport & WAVECAPS_DIRECTSOUND))
	/* not supported, ignore it */
	dwFlags &= ~WAVE_DIRECTSOUND;

    if (dwFlags & WAVE_DIRECTSOUND) {
        if (wwo->ossdev->out_caps.dwSupport & WAVECAPS_SAMPLEACCURATE)
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
    if (wwo->state != WINE_WS_CLOSED) return MMSYSERR_ALLOCATED;
    /* we want to be able to mmap() the device, which means it must be opened readable,
     * otherwise mmap() will fail (at least under Linux) */
    ret = OSS_OpenDevice(wwo->ossdev,
                         (dwFlags & WAVE_DIRECTSOUND) ? O_RDWR : O_WRONLY,
                         &audio_fragment,
                         (dwFlags & WAVE_DIRECTSOUND) ? 0 : 1,
                         lpDesc->lpFormat->nSamplesPerSec,
                         (lpDesc->lpFormat->nChannels > 1) ? 1 : 0,
                         (lpDesc->lpFormat->wBitsPerSample == 16)
                             ? AFMT_S16_LE : AFMT_U8);
    if ((ret==WAVERR_BADFORMAT) && (dwFlags & WAVE_DIRECTSOUND)) {
        lpDesc->lpFormat->nSamplesPerSec=wwo->ossdev->sample_rate;
        lpDesc->lpFormat->nChannels=(wwo->ossdev->stereo ? 2 : 1);
        lpDesc->lpFormat->wBitsPerSample=(wwo->ossdev->format == AFMT_U8 ? 8 : 16);
        lpDesc->lpFormat->nBlockAlign=lpDesc->lpFormat->nChannels*lpDesc->lpFormat->wBitsPerSample/8;
        lpDesc->lpFormat->nAvgBytesPerSec=lpDesc->lpFormat->nSamplesPerSec*lpDesc->lpFormat->nBlockAlign;
        ret=MMSYSERR_NOERROR;
        TRACE("OSS_OpenDevice returned this format: %ldx%dx%d\n",
              lpDesc->lpFormat->nSamplesPerSec,
              lpDesc->lpFormat->wBitsPerSample,
              lpDesc->lpFormat->nChannels);
    }
    if (ret != 0) return ret;
    wwo->state = WINE_WS_STOPPED;

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
    /* Read output space info for future reference */
    if (ioctl(wwo->ossdev->fd, SNDCTL_DSP_GETOSPACE, &info) < 0) {
	ERR("ioctl can't 'SNDCTL_DSP_GETOSPACE' !\n");
        OSS_CloseDevice(wwo->ossdev);
	wwo->state = WINE_WS_CLOSED;
	return MMSYSERR_NOTENABLED;
    }

    /* Check that fragsize is correct per our settings above */
    if ((info.fragsize > 1024) && (LOWORD(audio_fragment) <= 10)) {
	/* we've tried to set 1K fragments or less, but it didn't work */
	ERR("fragment size set failed, size is now %d\n", info.fragsize);
	MESSAGE("Your Open Sound System driver did not let us configure small enough sound fragments.\n");
	MESSAGE("This may cause delays and other problems in audio playback with certain applications.\n");
    }

    /* Remember fragsize and total buffer size for future use */
    wwo->dwFragmentSize = info.fragsize;
    wwo->dwBufferSize = info.fragstotal * info.fragsize;
    wwo->dwPlayedTotal = 0;
    wwo->dwWrittenTotal = 0;
    wwo->bNeedPost = TRUE;

    OSS_InitRingMessage(&wwo->msgRing);

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

    TRACE("fd=%d fragmentSize=%ld\n",
	  wwo->ossdev->fd, wwo->dwFragmentSize);
    if (wwo->dwFragmentSize % wwo->format.wf.nBlockAlign)
	ERR("Fragment doesn't contain an integral number of data blocks\n");

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

    if (wDevID >= numOutDev || WOutDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    wwo = &WOutDev[wDevID];
    if (wwo->lpQueuePtr) {
	WARN("buffers still playing !\n");
	ret = WAVERR_STILLPLAYING;
    } else {
	if (wwo->hThread != INVALID_HANDLE_VALUE) {
	    OSS_AddRingMessage(&wwo->msgRing, WINE_WM_CLOSING, 0, TRUE);
	}
	if (wwo->mapping) {
	    munmap(wwo->mapping, wwo->maplen);
	    wwo->mapping = NULL;
	}

        OSS_DestroyRingMessage(&wwo->msgRing);

        OSS_CloseDevice(wwo->ossdev);
	wwo->state = WINE_WS_CLOSED;
	wwo->dwFragmentSize = 0;
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
    if (wDevID >= numOutDev || WOutDev[wDevID].state == WINE_WS_CLOSED) {
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

    if ((lpWaveHdr->dwBufferLength & (WOutDev[wDevID].format.wf.nBlockAlign - 1)) != 0)
    {
        WARN("WaveHdr length isn't a multiple of the PCM block size: %ld %% %d\n",lpWaveHdr->dwBufferLength,WOutDev[wDevID].format.wf.nBlockAlign);
        lpWaveHdr->dwBufferLength &= ~(WOutDev[wDevID].format.wf.nBlockAlign - 1);
    }

    OSS_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_HEADER, (DWORD)lpWaveHdr, FALSE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodPrepare			[internal]
 */
static DWORD wodPrepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);

    if (wDevID >= numOutDev) {
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

    if (wDevID >= numOutDev) {
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

    if (wDevID >= numOutDev || WOutDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    OSS_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_PAUSING, 0, TRUE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodRestart				[internal]
 */
static DWORD wodRestart(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);

    if (wDevID >= numOutDev || WOutDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    OSS_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_RESTARTING, 0, TRUE);

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

    if (wDevID >= numOutDev || WOutDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    OSS_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_RESETTING, 0, TRUE);

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

    if (wDevID >= numOutDev || WOutDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;

    wwo = &WOutDev[wDevID];
#ifdef EXACT_WODPOSITION
    OSS_AddRingMessage(&wwo->msgRing, WINE_WM_UPDATE, 0, TRUE);
#endif
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

    if (wDevID >= numOutDev || WOutDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    OSS_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_BREAKLOOP, 0, TRUE);
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
    if (wDevID >= numOutDev) return MMSYSERR_INVALPARAM;

    if ((mixer = open(WOutDev[wDevID].ossdev->mixer_name, O_RDONLY|O_NDELAY)) < 0) {
	WARN("mixer device not available !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, SOUND_MIXER_READ_PCM, &volume) == -1) {
	WARN("unable to read mixer !\n");
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

    if (wDevID >= numOutDev) return MMSYSERR_INVALPARAM;

    if ((mixer = open(WOutDev[wDevID].ossdev->mixer_name, O_WRONLY|O_NDELAY)) < 0) {
	WARN("mixer device not available !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, SOUND_MIXER_WRITE_PCM, &volume) == -1) {
	WARN("unable to set mixer !\n");
	return MMSYSERR_NOTENABLED;
    } else {
	TRACE("volume=%04x\n", (unsigned)volume);
    }
    close(mixer);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodMessage (WINEOSS.7)
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
    case WODM_BREAKLOOP: 	return wodBreakLoop     (wDevID);
    case WODM_PREPARE:	 	return wodPrepare	(wDevID, (LPWAVEHDR)dwParam1, 		dwParam2);
    case WODM_UNPREPARE: 	return wodUnprepare	(wDevID, (LPWAVEHDR)dwParam1, 		dwParam2);
    case WODM_GETDEVCAPS:	return wodGetDevCaps	(wDevID, (LPWAVEOUTCAPSA)dwParam1,	dwParam2);
    case WODM_GETNUMDEVS:	return numOutDev;
    case WODM_GETPITCH:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPITCH:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETVOLUME:	return wodGetVolume	(wDevID, (LPDWORD)dwParam1);
    case WODM_SETVOLUME:	return wodSetVolume	(wDevID, dwParam1);
    case WODM_RESTART:		return wodRestart	(wDevID);
    case WODM_RESET:		return wodReset		(wDevID);

    case DRV_QUERYDSOUNDIFACE:	return wodDsCreate(wDevID, (PIDSDRIVER*)dwParam1);
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
			    wwo->ossdev->fd, 0);
	if (wwo->mapping == (LPBYTE)-1) {
	    ERR("(%p): Could not map sound device for direct access (%s)\n", dsdb, strerror(errno));
	    return DSERR_GENERIC;
	}
	TRACE("(%p): sound device has been mapped for direct access at %p, size=%ld\n", dsdb, wwo->mapping, wwo->maplen);

	/* for some reason, es1371 and sblive! sometimes have junk in here.
	 * clear it, or we get junk noise */
	/* some libc implementations are buggy: their memset reads from the buffer...
	 * to work around it, we have to zero the block by hand. We don't do the expected:
	 * memset(wwo->mapping,0, wwo->maplen);
	 */
	{
	    char*	p1 = wwo->mapping;
	    unsigned	len = wwo->maplen;

	    if (len >= 16) /* so we can have at least a 4 long area to store... */
	    {
		/* the mmap:ed value is (at least) dword aligned
		 * so, start filling the complete unsigned long:s
		 */
		int		b = len >> 2;
		unsigned long*	p4 = (unsigned long*)p1;

		while (b--) *p4++ = 0;
		/* prepare for filling the rest */
		len &= 3;
		p1 = (unsigned char*)p4;
	    }
	    /* in all cases, fill the remaining bytes */
	    while (len-- != 0) *p1++ = 0;
	}
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
    if (WOutDev[This->drv->wDevID].state == WINE_WS_CLOSED) {
	ERR("device not open, but accessing?\n");
	return DSERR_UNINITIALIZED;
    }
    if (ioctl(WOutDev[This->drv->wDevID].ossdev->fd, SNDCTL_DSP_GETOPTR, &info) < 0) {
	ERR("ioctl failed (%d)\n", errno);
	return DSERR_GENERIC;
    }
    ptr = info.ptr & ~3; /* align the pointer, just in case */
    if (lpdwPlay) *lpdwPlay = ptr;
    if (lpdwWrite) {
	/* add some safety margin (not strictly necessary, but...) */
	if (WOutDev[This->drv->wDevID].ossdev->out_caps.dwSupport & WAVECAPS_SAMPLEACCURATE)
	    *lpdwWrite = ptr + 32;
	else
	    *lpdwWrite = ptr + WOutDev[This->drv->wDevID].dwFragmentSize;
	while (*lpdwWrite > This->buflen)
	    *lpdwWrite -= This->buflen;
    }
    TRACE("playpos=%ld, writepos=%ld\n", lpdwPlay?*lpdwPlay:0, lpdwWrite?*lpdwWrite:0);
    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_Play(PIDSDRIVERBUFFER iface, DWORD dwRes1, DWORD dwRes2, DWORD dwFlags)
{
    ICOM_THIS(IDsDriverBufferImpl,iface);
    int enable = PCM_ENABLE_OUTPUT;
    TRACE("(%p,%lx,%lx,%lx)\n",iface,dwRes1,dwRes2,dwFlags);
    if (ioctl(WOutDev[This->drv->wDevID].ossdev->fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
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
    if (ioctl(WOutDev[This->drv->wDevID].ossdev->fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
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
    /* Most OSS drivers just can't stop the playback without closing the device...
     * so we need to somehow signal to our DirectSound implementation
     * that it should completely recreate this HW buffer...
     * this unexpected error code should do the trick... */
    return DSERR_BUFFERLOST;
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
    int enable = 0;

    TRACE("(%p)\n",iface);
    /* make sure the card doesn't start playing before we want it to */
    if (ioctl(WOutDev[This->wDevID].ossdev->fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
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
    int enable = 0;

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
    if (ioctl(WOutDev[This->wDevID].ossdev->fd, SNDCTL_DSP_GETOSPACE, &info) < 0) {
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

    /* some drivers need some extra nudging after mapping */
    if (ioctl(WOutDev[This->wDevID].ossdev->fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
	ERR("ioctl failed (%d)\n", errno);
	return DSERR_GENERIC;
    }

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
    if (!(WOutDev[wDevID].ossdev->out_caps.dwSupport & WAVECAPS_DIRECTSOUND)) {
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
 *                  Low level WAVE IN implementation			*
 *======================================================================*/

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
static DWORD widGetDevCaps(WORD wDevID, LPWAVEINCAPSA lpCaps, DWORD dwSize)
{
    TRACE("(%u, %p, %lu);\n", wDevID, lpCaps, dwSize);

    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;

    if (wDevID >= numInDev) {
	TRACE("numOutDev reached !\n");
	return MMSYSERR_BADDEVICEID;
    }

    memcpy(lpCaps, &WInDev[wDevID].ossdev->in_caps, min(dwSize, sizeof(*lpCaps)));
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
    LPVOID		buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, wwi->dwFragmentSize);
    LPVOID		pOffset = buffer;
    audio_buf_info 	info;
    int 		xs;
    enum win_wm_message msg;
    DWORD		param;
    HANDLE		ev;

    wwi->state = WINE_WS_STOPPED;
    wwi->dwTotalRecorded = 0;

    SetEvent(wwi->hStartUpEvent);

    /* the soundblaster live needs a micro wake to get its recording started
     * (or GETISPACE will have 0 frags all the time)
     */
    read(wwi->ossdev->fd, &xs, 4);

	/* make sleep time to be # of ms to output a fragment */
    dwSleepTime = (wwi->dwFragmentSize * 1000) / wwi->format.wf.nAvgBytesPerSec;
    TRACE("sleeptime=%ld ms\n", dwSleepTime);

    for (;;) {
	/* wait for dwSleepTime or an event in thread's queue */
	/* FIXME: could improve wait time depending on queue state,
	 * ie, number of queued fragments
	 */

	if (wwi->lpQueuePtr != NULL && wwi->state == WINE_WS_PLAYING)
        {
            lpWaveHdr = wwi->lpQueuePtr;

	    ioctl(wwi->ossdev->fd, SNDCTL_DSP_GETISPACE, &info);
            TRACE("info={frag=%d fsize=%d ftotal=%d bytes=%d}\n", info.fragments, info.fragsize, info.fragstotal, info.bytes);

            /* read all the fragments accumulated so far */
            while ((info.fragments > 0) && (wwi->lpQueuePtr))
            {
                info.fragments --;

                if (lpWaveHdr->dwBufferLength - lpWaveHdr->dwBytesRecorded >= wwi->dwFragmentSize)
                {
                    /* directly read fragment in wavehdr */
                    bytesRead = read(wwi->ossdev->fd,
		      		     lpWaveHdr->lpData + lpWaveHdr->dwBytesRecorded,
                                     wwi->dwFragmentSize);

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

			    widNotifyClient(wwi, WIM_DATA, (DWORD)lpWaveHdr, 0);
			    lpWaveHdr = wwi->lpQueuePtr = lpNext;
			}
                    }
                }
                else
		{
                    /* read the fragment in a local buffer */
                    bytesRead = read(wwi->ossdev->fd, buffer, wwi->dwFragmentSize);
                    pOffset = buffer;

                    TRACE("bytesRead=%ld (local)\n", bytesRead);

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

                            widNotifyClient(wwi, WIM_DATA, (DWORD)lpWaveHdr, 0);

			    wwi->lpQueuePtr = lpWaveHdr = lpNext;
			    if (!lpNext && bytesRead) {
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

	WAIT_OMR(&wwi->msgRing, dwSleepTime);

	while (OSS_RetrieveRingMessage(&wwi->msgRing, &msg, &param, &ev))
	{

            TRACE("msg=0x%x param=0x%lx\n", msg, param);
	    switch (msg) {
	    case WINE_WM_PAUSING:
		wwi->state = WINE_WS_PAUSED;
                /*FIXME("Device should stop recording\n");*/
		SetEvent(ev);
		break;
	    case WINE_WM_RESTARTING:
            {
                int enable = PCM_ENABLE_INPUT;
		wwi->state = WINE_WS_PLAYING;

                if (wwi->ossdev->bTriggerSupport)
                {
                    /* start the recording */
                    if (ioctl(wwi->ossdev->fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0)
                    {
                        ERR("ioctl(SNDCTL_DSP_SETTRIGGER) failed (%d)\n", errno);
                    }
                }
                else
                {
                    unsigned char data[4];
                    /* read 4 bytes to start the recording */
                    read(wwi->ossdev->fd, data, 4);
                }

		SetEvent(ev);
		break;
            }
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
	    case WINE_WM_RESETTING:
		wwi->state = WINE_WS_STOPPED;
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
		HeapFree(GetProcessHeap(), 0, buffer);
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
    int                 fragment_size;
    int                 audio_fragment;
    DWORD               ret;

    TRACE("(%u, %p, %08lX);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN("Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= numInDev) return MMSYSERR_BADDEVICEID;

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

    if (wwi->state != WINE_WS_CLOSED) return MMSYSERR_ALLOCATED;
    /* This is actually hand tuned to work so that my SB Live:
     * - does not skip
     * - does not buffer too much
     * when sending with the Shoutcast winamp plugin
     */
    /* 15 fragments max, 2^10 = 1024 bytes per fragment */
    audio_fragment = 0x000F000A;
    ret = OSS_OpenDevice(wwi->ossdev, O_RDONLY, &audio_fragment,
                         1,
                         lpDesc->lpFormat->nSamplesPerSec,
                         (lpDesc->lpFormat->nChannels > 1) ? 1 : 0,
                         (lpDesc->lpFormat->wBitsPerSample == 16)
                         ? AFMT_S16_LE : AFMT_U8);
    if (ret != 0) return ret;
    wwi->state = WINE_WS_STOPPED;

    if (wwi->lpQueuePtr) {
	WARN("Should have an empty queue (%p)\n", wwi->lpQueuePtr);
	wwi->lpQueuePtr = NULL;
    }
    wwi->dwTotalRecorded = 0;
    wwi->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    memcpy(&wwi->waveDesc, lpDesc,           sizeof(WAVEOPENDESC));
    memcpy(&wwi->format,   lpDesc->lpFormat, sizeof(PCMWAVEFORMAT));

    if (wwi->format.wBitsPerSample == 0) {
	WARN("Resetting zeroed wBitsPerSample\n");
	wwi->format.wBitsPerSample = 8 *
	    (wwi->format.wf.nAvgBytesPerSec /
	     wwi->format.wf.nSamplesPerSec) /
	    wwi->format.wf.nChannels;
    }

    ioctl(wwi->ossdev->fd, SNDCTL_DSP_GETBLKSIZE, &fragment_size);
    if (fragment_size == -1) {
	WARN("IOCTL can't 'SNDCTL_DSP_GETBLKSIZE' !\n");
        OSS_CloseDevice(wwi->ossdev);
	wwi->state = WINE_WS_CLOSED;
	return MMSYSERR_NOTENABLED;
    }
    wwi->dwFragmentSize = fragment_size;

    TRACE("wBitsPerSample=%u, nAvgBytesPerSec=%lu, nSamplesPerSec=%lu, nChannels=%u nBlockAlign=%u!\n",
	  wwi->format.wBitsPerSample, wwi->format.wf.nAvgBytesPerSec,
	  wwi->format.wf.nSamplesPerSec, wwi->format.wf.nChannels,
	  wwi->format.wf.nBlockAlign);

    OSS_InitRingMessage(&wwi->msgRing);

    wwi->hStartUpEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    wwi->hThread = CreateThread(NULL, 0, widRecorder, (LPVOID)(DWORD)wDevID, 0, &(wwi->dwThreadID));
    WaitForSingleObject(wwi->hStartUpEvent, INFINITE);
    CloseHandle(wwi->hStartUpEvent);
    wwi->hStartUpEvent = INVALID_HANDLE_VALUE;

    return widNotifyClient(wwi, WIM_OPEN, 0L, 0L);
}

/**************************************************************************
 * 				widClose			[internal]
 */
static DWORD widClose(WORD wDevID)
{
    WINE_WAVEIN*	wwi;

    TRACE("(%u);\n", wDevID);
    if (wDevID >= numInDev || WInDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("can't close !\n");
	return MMSYSERR_INVALHANDLE;
    }

    wwi = &WInDev[wDevID];

    if (wwi->lpQueuePtr != NULL) {
	WARN("still buffers open !\n");
	return WAVERR_STILLPLAYING;
    }

    OSS_AddRingMessage(&wwi->msgRing, WINE_WM_CLOSING, 0, TRUE);
    OSS_CloseDevice(wwi->ossdev);
    wwi->state = WINE_WS_CLOSED;
    wwi->dwFragmentSize = 0;
    OSS_DestroyRingMessage(&wwi->msgRing);
    return widNotifyClient(wwi, WIM_CLOSE, 0L, 0L);
}

/**************************************************************************
 * 				widAddBuffer		[internal]
 */
static DWORD widAddBuffer(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);

    if (wDevID >= numInDev || WInDev[wDevID].state == WINE_WS_CLOSED) {
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

    OSS_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_HEADER, (DWORD)lpWaveHdr, FALSE);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widPrepare			[internal]
 */
static DWORD widPrepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);

    if (wDevID >= numInDev) return MMSYSERR_INVALHANDLE;

    if (lpWaveHdr->dwFlags & WHDR_INQUEUE)
	return WAVERR_STILLPLAYING;

    lpWaveHdr->dwFlags |= WHDR_PREPARED;
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
    if (wDevID >= numInDev) return MMSYSERR_INVALHANDLE;

    if (lpWaveHdr->dwFlags & WHDR_INQUEUE)
	return WAVERR_STILLPLAYING;

    lpWaveHdr->dwFlags &= ~WHDR_PREPARED;
    lpWaveHdr->dwFlags |= WHDR_DONE;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widStart				[internal]
 */
static DWORD widStart(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    if (wDevID >= numInDev || WInDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("can't start recording !\n");
	return MMSYSERR_INVALHANDLE;
    }

    OSS_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_RESTARTING, 0, TRUE);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widStop					[internal]
 */
static DWORD widStop(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    if (wDevID >= numInDev || WInDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("can't stop !\n");
	return MMSYSERR_INVALHANDLE;
    }
    /* FIXME: reset aint stop */
    OSS_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_RESETTING, 0, TRUE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widReset				[internal]
 */
static DWORD widReset(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    if (wDevID >= numInDev || WInDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("can't reset !\n");
	return MMSYSERR_INVALHANDLE;
    }
    OSS_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_RESETTING, 0, TRUE);
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

    if (wDevID >= numInDev || WInDev[wDevID].state == WINE_WS_CLOSED) {
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
 * 				widMessage (WINEOSS.6)
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
    case WIDM_GETNUMDEVS:	return numInDev;
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
 * 				wodMessage (WINEOSS.7)
 */
DWORD WINAPI OSS_wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08lX, %08lX, %08lX):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				widMessage (WINEOSS.6)
 */
DWORD WINAPI OSS_widMessage(WORD wDevID, WORD wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08lX, %08lX, %08lX):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

#endif /* HAVE_OSS */
