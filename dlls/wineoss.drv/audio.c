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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
/*
 * FIXME:
 *	pause in waveOut does not work correctly in loop mode
 *	Direct Sound Capture driver does not work (not complete yet)
 */

/* an exact wodGetPosition is usually not worth the extra context switches,
 * as we're going to have near fragment accuracy anyway */
#define EXACT_WODPOSITION
#define EXACT_WIDPOSITION

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
#include <fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winerror.h"
#include "mmddk.h"
#include "mmreg.h"
#include "dsound.h"
#include "ks.h"
#include "ksguid.h"
#include "ksmedia.h"
#include "initguid.h"
#include "dsdriver.h"
#include "oss.h"
#include "wine/debug.h"

#include "audio.h"

WINE_DEFAULT_DEBUG_CHANNEL(wave);

/* Allow 1% deviation for sample rates (some ES137x cards) */
#define NEAR_MATCH(rate1,rate2) (((100*((int)(rate1)-(int)(rate2)))/(rate1))==0)

#ifdef HAVE_OSS

WINE_WAVEOUT    WOutDev[MAX_WAVEDRV];
WINE_WAVEIN     WInDev[MAX_WAVEDRV];
unsigned        numOutDev;
unsigned        numInDev;

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

int getEnables(OSS_DEVICE *ossdev)
{
    return ( (ossdev->bOutputEnabled ? PCM_ENABLE_OUTPUT : 0) |
             (ossdev->bInputEnabled  ? PCM_ENABLE_INPUT  : 0) );
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

static DWORD wodDevInterfaceSize(UINT wDevID, LPDWORD dwParam1)
{
    TRACE("(%u, %p)\n", wDevID, dwParam1);

    *dwParam1 = MultiByteToWideChar(CP_ACP, 0, WOutDev[wDevID].ossdev.interface_name, -1,
                                    NULL, 0 ) * sizeof(WCHAR);
    return MMSYSERR_NOERROR;
}

static DWORD wodDevInterface(UINT wDevID, PWCHAR dwParam1, DWORD dwParam2)
{
    if (dwParam2 >= MultiByteToWideChar(CP_ACP, 0, WOutDev[wDevID].ossdev.interface_name, -1,
                                        NULL, 0 ) * sizeof(WCHAR))
    {
        MultiByteToWideChar(CP_ACP, 0, WOutDev[wDevID].ossdev.interface_name, -1,
                            dwParam1, dwParam2 / sizeof(WCHAR));
	return MMSYSERR_NOERROR;
    }

    return MMSYSERR_INVALPARAM;
}

static DWORD widDevInterfaceSize(UINT wDevID, LPDWORD dwParam1)
{
    TRACE("(%u, %p)\n", wDevID, dwParam1);

    *dwParam1 = MultiByteToWideChar(CP_ACP, 0, WInDev[wDevID].ossdev.interface_name, -1,
                                    NULL, 0 ) * sizeof(WCHAR);
    return MMSYSERR_NOERROR;
}

static DWORD widDevInterface(UINT wDevID, PWCHAR dwParam1, DWORD dwParam2)
{
    if (dwParam2 >= MultiByteToWideChar(CP_ACP, 0, WInDev[wDevID].ossdev.interface_name, -1,
                                        NULL, 0 ) * sizeof(WCHAR))
    {
        MultiByteToWideChar(CP_ACP, 0, WInDev[wDevID].ossdev.interface_name, -1,
                            dwParam1, dwParam2 / sizeof(WCHAR));
	return MMSYSERR_NOERROR;
    }

    return MMSYSERR_INVALPARAM;
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
    int fd, val, rc;
    TRACE("(%p,%d)\n",ossdev,strict_format);

    TRACE("open_access=%s\n",
        ossdev->open_access == O_RDONLY ? "O_RDONLY" :
        ossdev->open_access == O_WRONLY ? "O_WRONLY" :
        ossdev->open_access == O_RDWR ? "O_RDWR" : "Unknown");

    if ((fd = open(ossdev->dev_name, ossdev->open_access|O_NDELAY, 0)) == -1)
    {
        WARN("Couldn't open %s (%s)\n", ossdev->dev_name, strerror(errno));
        return (errno == EBUSY) ? MMSYSERR_ALLOCATED : MMSYSERR_ERROR;
    }
    fcntl(fd, F_SETFD, 1); /* set close on exec flag */
    /* turn full duplex on if it has been requested */
    if (ossdev->open_access == O_RDWR && ossdev->full_duplex) {
        rc = ioctl(fd, SNDCTL_DSP_SETDUPLEX, 0);
        /* on *BSD, as full duplex is always enabled by default, this ioctl
         * will fail with EINVAL
         * so, we don't consider EINVAL an error here
         */
        if (rc != 0 && errno != EINVAL) {
            WARN("ioctl(%s, SNDCTL_DSP_SETDUPLEX) failed (%s)\n", ossdev->dev_name, strerror(errno));
            goto error2;
	}
    }

    if (ossdev->audio_fragment) {
        rc = ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &ossdev->audio_fragment);
        if (rc != 0) {
	    ERR("ioctl(%s, SNDCTL_DSP_SETFRAGMENT) failed (%s)\n", ossdev->dev_name, strerror(errno));
            goto error2;
	}
    }

    /* First size and channels then samplerate */
    if (ossdev->format>=0)
    {
        val = ossdev->format;
        rc = ioctl(fd, SNDCTL_DSP_SETFMT, &ossdev->format);
        if (rc != 0 || val != ossdev->format) {
            TRACE("Can't set format to %d (returned %d)\n", val, ossdev->format);
            if (strict_format)
                goto error;
        }
    }
    if (ossdev->channels>=0)
    {
        val = ossdev->channels;
        rc = ioctl(fd, SNDCTL_DSP_CHANNELS, &ossdev->channels);
        if (rc != 0 || val != ossdev->channels) {
            TRACE("Can't set channels to %u (returned %d)\n", val, ossdev->channels);
            if (strict_format)
                goto error;
        }
    }
    if (ossdev->sample_rate>=0)
    {
        val = ossdev->sample_rate;
        rc = ioctl(fd, SNDCTL_DSP_SPEED, &ossdev->sample_rate);
        if (rc != 0 || !NEAR_MATCH(val, ossdev->sample_rate)) {
            TRACE("Can't set sample_rate to %u (returned %d)\n", val, ossdev->sample_rate);
            if (strict_format)
                goto error;
        }
    }
    ossdev->fd = fd;

    ossdev->bOutputEnabled = TRUE;	/* OSS enables by default */
    ossdev->bInputEnabled  = TRUE;	/* OSS enables by default */
    if (ossdev->open_access == O_RDONLY)
        ossdev->bOutputEnabled = FALSE;
    if (ossdev->open_access == O_WRONLY)
        ossdev->bInputEnabled = FALSE;

    if (ossdev->bTriggerSupport) {
	int trigger;
        trigger = getEnables(ossdev);
        /* If we do not have full duplex, but they opened RDWR 
        ** (as you have to in order for an mmap to succeed)
        ** then we start out with input off
        */
        if (ossdev->open_access == O_RDWR && !ossdev->full_duplex && 
            ossdev->bInputEnabled && ossdev->bOutputEnabled) {
    	    ossdev->bInputEnabled  = FALSE;
            trigger &= ~PCM_ENABLE_INPUT;
	    ioctl(fd, SNDCTL_DSP_SETTRIGGER, &trigger);
        }
    }

    return MMSYSERR_NOERROR;

error:
    close(fd);
    return WAVERR_BADFORMAT;
error2:
    close(fd);
    return MMSYSERR_ERROR;
}

/******************************************************************
 *		OSS_OpenDevice
 *
 * since OSS has poor capabilities in full duplex, we try here to let a program
 * open the device for both waveout and wavein streams...
 * this is hackish, but it's the way OSS interface is done...
 */
DWORD OSS_OpenDevice(OSS_DEVICE* ossdev, unsigned req_access,
                            int* frag, int strict_format,
                            int sample_rate, int channels, int fmt)
{
    DWORD       ret;
    DWORD open_access;
    TRACE("(%p,%u,%p,%d,%d,%d,%x)\n",ossdev,req_access,frag,strict_format,sample_rate,channels,fmt);

    if (ossdev->full_duplex && (req_access == O_RDONLY || req_access == O_WRONLY))
    {
        TRACE("Opening RDWR because full_duplex=%d and req_access=%d\n",
              ossdev->full_duplex,req_access);
        open_access = O_RDWR;
    }
    else
    {
        open_access=req_access;
    }

    /* FIXME: this should be protected, and it also contains a race with OSS_CloseDevice */
    if (ossdev->open_count == 0)
    {
	if (access(ossdev->dev_name, 0) != 0) return MMSYSERR_NODRIVER;

        ossdev->audio_fragment = (frag) ? *frag : 0;
        ossdev->sample_rate = sample_rate;
        ossdev->channels = channels;
        ossdev->format = fmt;
        ossdev->open_access = open_access;
        ossdev->owner_tid = GetCurrentThreadId();

        if ((ret = OSS_RawOpenDevice(ossdev,strict_format)) != MMSYSERR_NOERROR) return ret;
        if (ossdev->full_duplex && ossdev->bTriggerSupport &&
            (req_access == O_RDONLY || req_access == O_WRONLY))
        {
            int enable;
            if (req_access == O_WRONLY)
                ossdev->bInputEnabled=0;
            else
                ossdev->bOutputEnabled=0;
            enable = getEnables(ossdev);
            TRACE("Calling SNDCTL_DSP_SETTRIGGER with %x\n",enable);
            if (ioctl(ossdev->fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0)
                ERR("ioctl(%s, SNDCTL_DSP_SETTRIGGER, %d) failed (%s)\n",ossdev->dev_name, enable, strerror(errno));
        }
    }
    else
    {
        /* check we really open with the same parameters */
        if (ossdev->open_access != open_access)
        {
            ERR("FullDuplex: Mismatch in access. Your sound device is not full duplex capable.\n");
            return WAVERR_BADFORMAT;
        }

	/* check if the audio parameters are the same */
        if (ossdev->sample_rate != sample_rate ||
            ossdev->channels != channels ||
            ossdev->format != fmt)
        {
	    /* This is not a fatal error because MSACM might do the remapping */
            WARN("FullDuplex: mismatch in PCM parameters for input and output\n"
                 "OSS doesn't allow us different parameters\n"
                 "audio_frag(%x/%x) sample_rate(%d/%d) channels(%d/%d) fmt(%d/%d)\n",
                 ossdev->audio_fragment, frag ? *frag : 0,
                 ossdev->sample_rate, sample_rate,
                 ossdev->channels, channels,
                 ossdev->format, fmt);
            return WAVERR_BADFORMAT;
        }
	/* check if the fragment sizes are the same */
        if (ossdev->audio_fragment != (frag ? *frag : 0) )
        {
	    ERR("FullDuplex: Playback and Capture hardware acceleration levels are different.\n"
	        "Please run winecfg, open \"Audio\" page and set\n"
                "\"Hardware Acceleration\" to \"Emulation\".\n");
	    return WAVERR_BADFORMAT;
	}
        if (GetCurrentThreadId() != ossdev->owner_tid)
        {
            WARN("Another thread is trying to access audio...\n");
            return MMSYSERR_ERROR;
        }
        if (ossdev->full_duplex && ossdev->bTriggerSupport &&
            (req_access == O_RDONLY || req_access == O_WRONLY))
        {
            int enable;
            if (req_access == O_WRONLY)
                ossdev->bOutputEnabled=1;
            else
                ossdev->bInputEnabled=1;
            enable = getEnables(ossdev);
            TRACE("Calling SNDCTL_DSP_SETTRIGGER with %x\n",enable);
            if (ioctl(ossdev->fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0)
                ERR("ioctl(%s, SNDCTL_DSP_SETTRIGGER, %d) failed (%s)\n",ossdev->dev_name, enable, strerror(errno));
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
void	OSS_CloseDevice(OSS_DEVICE* ossdev)
{
    TRACE("(%p)\n",ossdev);
    if (ossdev->open_count>0) {
        ossdev->open_count--;
    } else {
        WARN("OSS_CloseDevice called too many times\n");
    }
    if (ossdev->open_count == 0)
    {
        fcntl(ossdev->fd, F_SETFL, fcntl(ossdev->fd, F_GETFL) & ~O_NDELAY);
        /* reset the device before we close it in case it is in a bad state */
        ioctl(ossdev->fd, SNDCTL_DSP_RESET, 0);
        if (close(ossdev->fd) != 0) FIXME("Cannot close %d: %s\n", ossdev->fd, strerror(errno));
    }
}

/******************************************************************
 *		OSS_ResetDevice
 *
 * Resets the device. OSS Commercial requires the device to be closed
 * after a SNDCTL_DSP_RESET ioctl call... this function implements
 * this behavior...
 * FIXME: This causes problems when doing full duplex so we really
 * only reset when not doing full duplex. We need to do this better
 * someday.
 */
static DWORD     OSS_ResetDevice(OSS_DEVICE* ossdev)
{
    DWORD       ret = MMSYSERR_NOERROR;
    int         old_fd = ossdev->fd;
    TRACE("(%p)\n", ossdev);

    if (ossdev->open_count == 1) {
	if (ioctl(ossdev->fd, SNDCTL_DSP_RESET, NULL) == -1)
	{
	    perror("ioctl SNDCTL_DSP_RESET");
            return -1;
	}
	close(ossdev->fd);
	ret = OSS_RawOpenDevice(ossdev, 1);
	TRACE("Changing fd from %d to %d\n", old_fd, ossdev->fd);
    } else
	WARN("Not resetting device because it is in full duplex mode!\n");

    return ret;
}

static const int win_std_oss_fmts[2]={AFMT_U8,AFMT_S16_LE};
static const int win_std_rates[5]={96000,48000,44100,22050,11025};
static const int win_std_formats[2][2][5]=
    {{{WAVE_FORMAT_96M08, WAVE_FORMAT_48M08, WAVE_FORMAT_4M08,
       WAVE_FORMAT_2M08,  WAVE_FORMAT_1M08},
      {WAVE_FORMAT_96S08, WAVE_FORMAT_48S08, WAVE_FORMAT_4S08,
       WAVE_FORMAT_2S08,  WAVE_FORMAT_1S08}},
     {{WAVE_FORMAT_96M16, WAVE_FORMAT_48M16, WAVE_FORMAT_4M16,
       WAVE_FORMAT_2M16,  WAVE_FORMAT_1M16},
      {WAVE_FORMAT_96S16, WAVE_FORMAT_48S16, WAVE_FORMAT_4S16,
       WAVE_FORMAT_2S16,  WAVE_FORMAT_1S16}},
    };

static void OSS_Info(int fd)
{
    /* Note that this only reports the formats supported by the hardware.
     * The driver may support other formats and do the conversions in
     * software which is why we don't use this value
     */
    int oss_mask, oss_caps;
    if (ioctl(fd, SNDCTL_DSP_GETFMTS, &oss_mask) >= 0) {
        TRACE("Formats=%08x ( ", oss_mask);
        if (oss_mask & AFMT_MU_LAW) TRACE("AFMT_MU_LAW ");
        if (oss_mask & AFMT_A_LAW) TRACE("AFMT_A_LAW ");
        if (oss_mask & AFMT_IMA_ADPCM) TRACE("AFMT_IMA_ADPCM ");
        if (oss_mask & AFMT_U8) TRACE("AFMT_U8 ");
        if (oss_mask & AFMT_S16_LE) TRACE("AFMT_S16_LE ");
        if (oss_mask & AFMT_S16_BE) TRACE("AFMT_S16_BE ");
        if (oss_mask & AFMT_S8) TRACE("AFMT_S8 ");
        if (oss_mask & AFMT_U16_LE) TRACE("AFMT_U16_LE ");
        if (oss_mask & AFMT_U16_BE) TRACE("AFMT_U16_BE ");
        if (oss_mask & AFMT_MPEG) TRACE("AFMT_MPEG ");
#ifdef AFMT_AC3
        if (oss_mask & AFMT_AC3) TRACE("AFMT_AC3 ");
#endif
#ifdef AFMT_VORBIS
        if (oss_mask & AFMT_VORBIS) TRACE("AFMT_VORBIS ");
#endif
#ifdef AFMT_S32_LE
        if (oss_mask & AFMT_S32_LE) TRACE("AFMT_S32_LE ");
#endif
#ifdef AFMT_S32_BE
        if (oss_mask & AFMT_S32_BE) TRACE("AFMT_S32_BE ");
#endif
#ifdef AFMT_FLOAT
        if (oss_mask & AFMT_FLOAT) TRACE("AFMT_FLOAT ");
#endif
#ifdef AFMT_S24_LE
        if (oss_mask & AFMT_S24_LE) TRACE("AFMT_S24_LE ");
#endif
#ifdef AFMT_S24_BE
        if (oss_mask & AFMT_S24_BE) TRACE("AFMT_S24_BE ");
#endif
#ifdef AFMT_SPDIF_RAW
        if (oss_mask & AFMT_SPDIF_RAW) TRACE("AFMT_SPDIF_RAW ");
#endif
        TRACE(")\n");
    }
    if (ioctl(fd, SNDCTL_DSP_GETCAPS, &oss_caps) >= 0) {
        TRACE("Caps=%08x\n",oss_caps);
        TRACE("\tRevision: %d\n", oss_caps&DSP_CAP_REVISION);
        TRACE("\tDuplex: %s\n", oss_caps & DSP_CAP_DUPLEX ? "true" : "false");
        TRACE("\tRealtime: %s\n", oss_caps & DSP_CAP_REALTIME ? "true" : "false");
        TRACE("\tBatch: %s\n", oss_caps & DSP_CAP_BATCH ? "true" : "false");
        TRACE("\tCoproc: %s\n", oss_caps & DSP_CAP_COPROC ? "true" : "false");
        TRACE("\tTrigger: %s\n", oss_caps & DSP_CAP_TRIGGER ? "true" : "false");
        TRACE("\tMmap: %s\n", oss_caps & DSP_CAP_MMAP ? "true" : "false");
#ifdef DSP_CAP_MULTI
        TRACE("\tMulti: %s\n", oss_caps & DSP_CAP_MULTI ? "true" : "false");
#endif
#ifdef DSP_CAP_BIND
        TRACE("\tBind: %s\n", oss_caps & DSP_CAP_BIND ? "true" : "false");
#endif
#ifdef DSP_CAP_INPUT
        TRACE("\tInput: %s\n", oss_caps & DSP_CAP_INPUT ? "true" : "false");
#endif
#ifdef DSP_CAP_OUTPUT
        TRACE("\tOutput: %s\n", oss_caps & DSP_CAP_OUTPUT ? "true" : "false");
#endif
#ifdef DSP_CAP_VIRTUAL
        TRACE("\tVirtual: %s\n", oss_caps & DSP_CAP_VIRTUAL ? "true" : "false");
#endif
#ifdef DSP_CAP_ANALOGOUT
        TRACE("\tAnalog Out: %s\n", oss_caps & DSP_CAP_ANALOGOUT ? "true" : "false");
#endif
#ifdef DSP_CAP_ANALOGIN
        TRACE("\tAnalog In: %s\n", oss_caps & DSP_CAP_ANALOGIN ? "true" : "false");
#endif
#ifdef DSP_CAP_DIGITALOUT
        TRACE("\tDigital Out: %s\n", oss_caps & DSP_CAP_DIGITALOUT ? "true" : "false");
#endif
#ifdef DSP_CAP_DIGITALIN
        TRACE("\tDigital In: %s\n", oss_caps & DSP_CAP_DIGITALIN ? "true" : "false");
#endif
#ifdef DSP_CAP_ADMASK
        TRACE("\tA/D Mask: %s\n", oss_caps & DSP_CAP_ADMASK ? "true" : "false");
#endif
#ifdef DSP_CAP_SHADOW
        TRACE("\tShadow: %s\n", oss_caps & DSP_CAP_SHADOW ? "true" : "false");
#endif
#ifdef DSP_CH_MASK
        TRACE("\tChannel Mask: %x\n", oss_caps & DSP_CH_MASK);
#endif
#ifdef DSP_CAP_SLAVE
        TRACE("\tSlave: %s\n", oss_caps & DSP_CAP_SLAVE ? "true" : "false");
#endif
    }
}

/******************************************************************
 *		OSS_WaveOutInit
 *
 *
 */
static BOOL OSS_WaveOutInit(OSS_DEVICE* ossdev)
{
    int rc,arg;
    int f,c,r;
    BOOL has_mixer = FALSE;
    TRACE("(%p) %s\n", ossdev, ossdev->dev_name);

    if (OSS_OpenDevice(ossdev, O_WRONLY, NULL, 0,-1,-1,-1) != 0)
        return FALSE;

    ioctl(ossdev->fd, SNDCTL_DSP_RESET, 0);

#if defined(SNDCTL_MIXERINFO)
    {
        int mixer;
        if ((mixer = open(ossdev->mixer_name, O_RDONLY|O_NDELAY)) >= 0) {
            oss_mixerinfo info;
            info.dev = 0;
            if (ioctl(mixer, SNDCTL_MIXERINFO, &info) >= 0) {
                lstrcpynA(ossdev->ds_desc.szDesc, info.name, sizeof(info.name));
                strcpy(ossdev->ds_desc.szDrvname, "wineoss.drv");
                MultiByteToWideChar(CP_ACP, 0, info.name, sizeof(info.name),
                                    ossdev->out_caps.szPname,
                                    sizeof(ossdev->out_caps.szPname) / sizeof(WCHAR));
                TRACE("%s: %s\n", ossdev->mixer_name, ossdev->ds_desc.szDesc);
                has_mixer = TRUE;
            } else {
                WARN("%s: cannot read SNDCTL_MIXERINFO!\n", ossdev->mixer_name);
            }
            close(mixer);
        } else {
            WARN("open(%s) failed (%s)\n", ossdev->mixer_name , strerror(errno));
        }
    }
#elif defined(SOUND_MIXER_INFO)
    {
        int mixer;
        if ((mixer = open(ossdev->mixer_name, O_RDONLY|O_NDELAY)) >= 0) {
            mixer_info info;
            if (ioctl(mixer, SOUND_MIXER_INFO, &info) >= 0) {
                lstrcpynA(ossdev->ds_desc.szDesc, info.name, sizeof(info.name));
                strcpy(ossdev->ds_desc.szDrvname, "wineoss.drv");
                MultiByteToWideChar(CP_ACP, 0, info.name, sizeof(info.name), 
                                    ossdev->out_caps.szPname, 
                                    sizeof(ossdev->out_caps.szPname) / sizeof(WCHAR));
                TRACE("%s: %s\n", ossdev->mixer_name, ossdev->ds_desc.szDesc);
                has_mixer = TRUE;
            } else {
                /* FreeBSD up to at least 5.2 provides this ioctl, but does not
                 * implement it properly, and there are probably similar issues
                 * on other platforms, so we warn but try to go ahead.
                 */
                WARN("%s: cannot read SOUND_MIXER_INFO!\n", ossdev->mixer_name);
            }
            close(mixer);
        } else {
            WARN("open(%s) failed (%s)\n", ossdev->mixer_name , strerror(errno));
        }
    }
#endif /* SOUND_MIXER_INFO */

    if (WINE_TRACE_ON(wave))
        OSS_Info(ossdev->fd);

    ossdev->out_caps.wMid = 0x00FF; /* Manufac ID */
    ossdev->out_caps.wPid = 0x0001; /* Product ID */

    ossdev->out_caps.vDriverVersion = 0x0100;
    ossdev->out_caps.wChannels = 1;
    ossdev->out_caps.dwFormats = 0x00000000;
    ossdev->out_caps.wReserved1 = 0;
    ossdev->out_caps.dwSupport = has_mixer ? WAVECAPS_VOLUME : 0;

    /* direct sound caps */
    ossdev->ds_caps.dwFlags = DSCAPS_CERTIFIED;
    ossdev->ds_caps.dwFlags |= DSCAPS_SECONDARY8BIT;
    ossdev->ds_caps.dwFlags |= DSCAPS_SECONDARY16BIT;
    ossdev->ds_caps.dwFlags |= DSCAPS_SECONDARYMONO;
    ossdev->ds_caps.dwFlags |= DSCAPS_SECONDARYSTEREO;
    ossdev->ds_caps.dwFlags |= DSCAPS_CONTINUOUSRATE;

    ossdev->ds_caps.dwPrimaryBuffers = 1;
    ossdev->ds_caps.dwMinSecondarySampleRate = DSBFREQUENCY_MIN;
    ossdev->ds_caps.dwMaxSecondarySampleRate = DSBFREQUENCY_MAX;

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
        if (rc!=0 || arg!=win_std_oss_fmts[f]) {
            TRACE("DSP_SAMPLESIZE: rc=%d returned %d for %d\n",
                  rc,arg,win_std_oss_fmts[f]);
            continue;
        }
	if (f == 0)
	    ossdev->ds_caps.dwFlags |= DSCAPS_PRIMARY8BIT;
	else if (f == 1)
	    ossdev->ds_caps.dwFlags |= DSCAPS_PRIMARY16BIT;

        for (c = 1; c <= MAX_CHANNELS; c++) {
            arg=c;
            rc=ioctl(ossdev->fd, SNDCTL_DSP_CHANNELS, &arg);
            if( rc == -1) break;
            if (rc!=0 || arg!=c) {
                TRACE("DSP_CHANNELS: rc=%d returned %d for %d\n",rc,arg,c);
                continue;
            }
	    if (c == 1) {
		ossdev->ds_caps.dwFlags |= DSCAPS_PRIMARYMONO;
	    } else if (c == 2) {
                ossdev->out_caps.wChannels = 2;
                if (has_mixer)
                    ossdev->out_caps.dwSupport|=WAVECAPS_LRVOLUME;
		ossdev->ds_caps.dwFlags |= DSCAPS_PRIMARYSTEREO;
            } else
                ossdev->out_caps.wChannels = c;

            for (r=0;r<sizeof(win_std_rates)/sizeof(*win_std_rates);r++) {
                arg=win_std_rates[r];
                rc=ioctl(ossdev->fd, SNDCTL_DSP_SPEED, &arg);
                TRACE("DSP_SPEED: rc=%d returned %d for %dx%dx%d\n",
                      rc,arg,win_std_rates[r],win_std_oss_fmts[f],c);
                if (rc==0 && arg!=0 && NEAR_MATCH(arg,win_std_rates[r]) && c < 3)
                    ossdev->out_caps.dwFormats|=win_std_formats[f][c-1][r];
            }
        }
    }

    if (ioctl(ossdev->fd, SNDCTL_DSP_GETCAPS, &arg) == 0) {
        if (arg & DSP_CAP_TRIGGER)
            ossdev->bTriggerSupport = TRUE;
        if ((arg & DSP_CAP_REALTIME) && !(arg & DSP_CAP_BATCH)) {
            ossdev->out_caps.dwSupport |= WAVECAPS_SAMPLEACCURATE;
        }
        /* well, might as well use the DirectSound cap flag for something */
        if ((arg & DSP_CAP_TRIGGER) && (arg & DSP_CAP_MMAP) &&
            !(arg & DSP_CAP_BATCH)) {
            ossdev->out_caps.dwSupport |= WAVECAPS_DIRECTSOUND;
	} else {
	    ossdev->ds_caps.dwFlags |= DSCAPS_EMULDRIVER;
	}
#ifdef DSP_CAP_MULTI    /* not every oss has this */
        /* check for hardware secondary buffer support (multi open) */
        if ((arg & DSP_CAP_MULTI) &&
            (ossdev->out_caps.dwSupport & WAVECAPS_DIRECTSOUND)) {
            TRACE("hardware secondary buffer support available\n");

            ossdev->ds_caps.dwMaxHwMixingAllBuffers = 16;
            ossdev->ds_caps.dwMaxHwMixingStaticBuffers = 0;
            ossdev->ds_caps.dwMaxHwMixingStreamingBuffers = 16;

            ossdev->ds_caps.dwFreeHwMixingAllBuffers = 16;
            ossdev->ds_caps.dwFreeHwMixingStaticBuffers = 0;
            ossdev->ds_caps.dwFreeHwMixingStreamingBuffers = 16;
        }
#endif
    }
    OSS_CloseDevice(ossdev);
    TRACE("out wChannels = %d, dwFormats = %08X, dwSupport = %08X\n",
          ossdev->out_caps.wChannels, ossdev->out_caps.dwFormats,
          ossdev->out_caps.dwSupport);
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
    TRACE("(%p) %s\n", ossdev, ossdev->dev_name);

    if (OSS_OpenDevice(ossdev, O_RDONLY, NULL, 0,-1,-1,-1) != 0)
        return FALSE;

    ioctl(ossdev->fd, SNDCTL_DSP_RESET, 0);

#if defined(SNDCTL_MIXERINFO)
    {
        int mixer;
        if ((mixer = open(ossdev->mixer_name, O_RDONLY|O_NDELAY)) >= 0) {
            oss_mixerinfo info;
            info.dev = 0;
            if (ioctl(mixer, SNDCTL_MIXERINFO, &info) >= 0) {
                MultiByteToWideChar(CP_ACP, 0, info.name, -1,
                                    ossdev->in_caps.szPname,
                                    sizeof(ossdev->in_caps.szPname) / sizeof(WCHAR));
                TRACE("%s: %s\n", ossdev->mixer_name, ossdev->ds_desc.szDesc);
            } else {
                WARN("%s: cannot read SNDCTL_MIXERINFO!\n", ossdev->mixer_name);
            }
            close(mixer);
        } else {
            WARN("open(%s) failed (%s)\n", ossdev->mixer_name, strerror(errno));
        }
    }
#elif defined(SOUND_MIXER_INFO)
    {
        int mixer;
        if ((mixer = open(ossdev->mixer_name, O_RDONLY|O_NDELAY)) >= 0) {
            mixer_info info;
            if (ioctl(mixer, SOUND_MIXER_INFO, &info) >= 0) {
                MultiByteToWideChar(CP_ACP, 0, info.name, -1, 
                                    ossdev->in_caps.szPname, 
                                    sizeof(ossdev->in_caps.szPname) / sizeof(WCHAR));
                TRACE("%s: %s\n", ossdev->mixer_name, ossdev->ds_desc.szDesc);
            } else {
                /* FreeBSD up to at least 5.2 provides this ioctl, but does not
                 * implement it properly, and there are probably similar issues
                 * on other platforms, so we warn but try to go ahead.
                 */
                WARN("%s: cannot read SOUND_MIXER_INFO!\n", ossdev->mixer_name);
            }
            close(mixer);
        } else {
            WARN("open(%s) failed (%s)\n", ossdev->mixer_name, strerror(errno));
        }
    }
#endif /* SOUND_MIXER_INFO */

    if (WINE_TRACE_ON(wave))
        OSS_Info(ossdev->fd);

    ossdev->in_caps.wMid = 0x00FF; /* Manufac ID */
    ossdev->in_caps.wPid = 0x0001; /* Product ID */

    ossdev->in_caps.dwFormats = 0x00000000;
    ossdev->in_caps.wChannels = 1;
    ossdev->in_caps.wReserved1 = 0;

    /* direct sound caps */
    ossdev->dsc_caps.dwSize = sizeof(ossdev->dsc_caps);
    ossdev->dsc_caps.dwFlags = 0;
    ossdev->dsc_caps.dwFormats = 0x00000000;
    ossdev->dsc_caps.dwChannels = 1;

    /* See the comment in OSS_WaveOutInit for the loop order */
    for (f=0;f<2;f++) {
        arg=win_std_oss_fmts[f];
        rc=ioctl(ossdev->fd, SNDCTL_DSP_SAMPLESIZE, &arg);
        if (rc!=0 || arg!=win_std_oss_fmts[f]) {
            TRACE("DSP_SAMPLESIZE: rc=%d returned 0x%x for 0x%x\n",
                  rc,arg,win_std_oss_fmts[f]);
            continue;
        }

        for (c = 1; c <= MAX_CHANNELS; c++) {
            arg=c;
            rc=ioctl(ossdev->fd, SNDCTL_DSP_CHANNELS, &arg);
            if( rc == -1) break;
            if (rc!=0 || arg!=c) {
                TRACE("DSP_CHANNELS: rc=%d returned %d for %d\n",rc,arg,c);
                continue;
            }
            if (c > 1) {
                ossdev->in_caps.wChannels = c;
    		ossdev->dsc_caps.dwChannels = c;
            }

            for (r=0;r<sizeof(win_std_rates)/sizeof(*win_std_rates);r++) {
                arg=win_std_rates[r];
                rc=ioctl(ossdev->fd, SNDCTL_DSP_SPEED, &arg);
                TRACE("DSP_SPEED: rc=%d returned %d for %dx%dx%d\n",rc,arg,win_std_rates[r],win_std_oss_fmts[f],c);
                if (rc==0 && NEAR_MATCH(arg,win_std_rates[r]) && c < 3)
                    ossdev->in_caps.dwFormats|=win_std_formats[f][c-1][r];
		    ossdev->dsc_caps.dwFormats|=win_std_formats[f][c-1][r];
            }
        }
    }

    if (ioctl(ossdev->fd, SNDCTL_DSP_GETCAPS, &arg) == 0) {
        if (arg & DSP_CAP_TRIGGER)
            ossdev->bTriggerSupport = TRUE;
        if ((arg & DSP_CAP_TRIGGER) && (arg & DSP_CAP_MMAP) &&
            !(arg & DSP_CAP_BATCH)) {
	    /* FIXME: enable the next statement if you want to work on the driver */
#if 0
            ossdev->in_caps_support |= WAVECAPS_DIRECTSOUND;
#endif
	}
	if ((arg & DSP_CAP_REALTIME) && !(arg & DSP_CAP_BATCH))
	    ossdev->in_caps_support |= WAVECAPS_SAMPLEACCURATE;
    }
    OSS_CloseDevice(ossdev);
    TRACE("in wChannels = %d, dwFormats = %08X, in_caps_support = %08X\n",
        ossdev->in_caps.wChannels, ossdev->in_caps.dwFormats, ossdev->in_caps_support);
    return TRUE;
}

/******************************************************************
 *		OSS_WaveFullDuplexInit
 *
 *
 */
static void OSS_WaveFullDuplexInit(OSS_DEVICE* ossdev)
{
    int rc,arg;
    int f,c,r;
    int caps;
    BOOL has_mixer = FALSE;
    TRACE("(%p) %s\n", ossdev, ossdev->dev_name);

    /* The OSS documentation says we must call SNDCTL_SETDUPLEX
     * *before* checking for SNDCTL_DSP_GETCAPS otherwise we may
     * get the wrong result. This ioctl must even be done before
     * setting the fragment size so that only OSS_RawOpenDevice is
     * in a position to do it. So we set full_duplex speculatively
     * and adjust right after.
     */
    ossdev->full_duplex=1;
    rc=OSS_OpenDevice(ossdev, O_RDWR, NULL, 0,-1,-1,-1);
    ossdev->full_duplex=0;
    if (rc != 0)
        return;

    ioctl(ossdev->fd, SNDCTL_DSP_RESET, 0);

#if defined(SNDCTL_MIXERINFO)
    {
        int mixer;
        if ((mixer = open(ossdev->mixer_name, O_RDWR|O_NDELAY)) >= 0) {
            oss_mixerinfo info;
            info.dev = 0;
            if (ioctl(mixer, SNDCTL_MIXERINFO, &info) >= 0) {
                has_mixer = TRUE;
            } else {
                WARN("%s: cannot read SNDCTL_MIXERINFO!\n", ossdev->mixer_name);
            }
            close(mixer);
        } else {
            WARN("open(%s) failed (%s)\n", ossdev->mixer_name , strerror(errno));
        }
    }
#elif defined(SOUND_MIXER_INFO)
    {
        int mixer;
        if ((mixer = open(ossdev->mixer_name, O_RDWR|O_NDELAY)) >= 0) {
            mixer_info info;
            if (ioctl(mixer, SOUND_MIXER_INFO, &info) >= 0) {
                has_mixer = TRUE;
            } else {
                /* FreeBSD up to at least 5.2 provides this ioctl, but does not
                 * implement it properly, and there are probably similar issues
                 * on other platforms, so we warn but try to go ahead.
                 */
                WARN("%s: cannot read SOUND_MIXER_INFO!\n", ossdev->mixer_name);
            }
            close(mixer);
        } else {
            WARN("open(%s) failed (%s)\n", ossdev->mixer_name , strerror(errno));
        }
    }
#endif /* SOUND_MIXER_INFO */

    TRACE("%s\n", ossdev->ds_desc.szDesc);

    if (ioctl(ossdev->fd, SNDCTL_DSP_GETCAPS, &caps) == 0)
        ossdev->full_duplex = (caps & DSP_CAP_DUPLEX);

    ossdev->duplex_out_caps = ossdev->out_caps;

    ossdev->duplex_out_caps.wChannels = 1;
    ossdev->duplex_out_caps.dwFormats = 0x00000000;
    ossdev->duplex_out_caps.dwSupport = has_mixer ? WAVECAPS_VOLUME : 0;

    if (WINE_TRACE_ON(wave))
        OSS_Info(ossdev->fd);

    /* See the comment in OSS_WaveOutInit for the loop order */
    for (f=0;f<2;f++) {
        arg=win_std_oss_fmts[f];
        rc=ioctl(ossdev->fd, SNDCTL_DSP_SAMPLESIZE, &arg);
        if (rc!=0 || arg!=win_std_oss_fmts[f]) {
            TRACE("DSP_SAMPLESIZE: rc=%d returned 0x%x for 0x%x\n",
                  rc,arg,win_std_oss_fmts[f]);
            continue;
        }

        for (c = 1; c <= MAX_CHANNELS; c++) {
            arg=c;
            rc=ioctl(ossdev->fd, SNDCTL_DSP_CHANNELS, &arg);
            if( rc == -1) break;
            if (rc!=0 || arg!=c) {
                TRACE("DSP_CHANNELS: rc=%d returned %d for %d\n",rc,arg,c);
                continue;
            }
	    if (c == 1) {
		ossdev->ds_caps.dwFlags |= DSCAPS_PRIMARYMONO;
	    } else if (c == 2) {
                ossdev->duplex_out_caps.wChannels = 2;
                if (has_mixer)
                    ossdev->duplex_out_caps.dwSupport|=WAVECAPS_LRVOLUME;
		ossdev->ds_caps.dwFlags |= DSCAPS_PRIMARYSTEREO;
            } else
                ossdev->duplex_out_caps.wChannels = c;

            for (r=0;r<sizeof(win_std_rates)/sizeof(*win_std_rates);r++) {
                arg=win_std_rates[r];
                rc=ioctl(ossdev->fd, SNDCTL_DSP_SPEED, &arg);
                TRACE("DSP_SPEED: rc=%d returned %d for %dx%dx%d\n",
                      rc,arg,win_std_rates[r],win_std_oss_fmts[f],c);
                if (rc==0 && arg!=0 && NEAR_MATCH(arg,win_std_rates[r]) && c < 3)
                    ossdev->duplex_out_caps.dwFormats|=win_std_formats[f][c-1][r];
            }
        }
    }

    if (ioctl(ossdev->fd, SNDCTL_DSP_GETCAPS, &arg) == 0) {
        if ((arg & DSP_CAP_REALTIME) && !(arg & DSP_CAP_BATCH)) {
            ossdev->duplex_out_caps.dwSupport |= WAVECAPS_SAMPLEACCURATE;
        }
        /* well, might as well use the DirectSound cap flag for something */
        if ((arg & DSP_CAP_TRIGGER) && (arg & DSP_CAP_MMAP) &&
            !(arg & DSP_CAP_BATCH)) {
            ossdev->duplex_out_caps.dwSupport |= WAVECAPS_DIRECTSOUND;
	}
    }
    OSS_CloseDevice(ossdev);
    TRACE("duplex wChannels = %d, dwFormats = %08X, dwSupport = %08X\n",
          ossdev->duplex_out_caps.wChannels,
          ossdev->duplex_out_caps.dwFormats,
          ossdev->duplex_out_caps.dwSupport);
}

static char* StrDup(const char* str, const char* def)
{
    char* dst;
    if (str==NULL)
        str=def;
    dst=HeapAlloc(GetProcessHeap(),0,strlen(str)+1);
    strcpy(dst, str);
    return dst;
}

/******************************************************************
 *		OSS_WaveInit
 *
 * Initialize internal structures from OSS information
 */
LRESULT OSS_WaveInit(void)
{
    char* str;
    int i;

    FIXME("() stub\n");

    str=getenv("AUDIODEV");
    if (str!=NULL)
    {
        WOutDev[0].ossdev.dev_name = WInDev[0].ossdev.dev_name = StrDup(str,"");
        WOutDev[0].ossdev.mixer_name = WInDev[0].ossdev.mixer_name = StrDup(getenv("MIXERDEV"),"/dev/mixer");
        for (i = 1; i < MAX_WAVEDRV; ++i)
        {
            WOutDev[i].ossdev.dev_name = WInDev[i].ossdev.dev_name = StrDup("",NULL);
            WOutDev[i].ossdev.mixer_name = WInDev[i].ossdev.mixer_name = StrDup("",NULL);
        }
    }
    else
    {
        WOutDev[0].ossdev.dev_name = WInDev[0].ossdev.dev_name = StrDup("/dev/dsp",NULL);
        WOutDev[0].ossdev.mixer_name = WInDev[0].ossdev.mixer_name = StrDup("/dev/mixer",NULL);
        for (i = 1; i < MAX_WAVEDRV; ++i)
        {
            WOutDev[i].ossdev.dev_name = WInDev[i].ossdev.dev_name = HeapAlloc(GetProcessHeap(),0,11);
            sprintf(WOutDev[i].ossdev.dev_name, "/dev/dsp%d", i);
            WOutDev[i].ossdev.mixer_name = WInDev[i].ossdev.mixer_name = HeapAlloc(GetProcessHeap(),0,13);
            sprintf(WOutDev[i].ossdev.mixer_name, "/dev/mixer%d", i);
        }
    }

    for (i = 0; i < MAX_WAVEDRV; ++i)
    {
        WOutDev[i].ossdev.interface_name = WInDev[i].ossdev.interface_name =
            HeapAlloc(GetProcessHeap(),0,9+strlen(WOutDev[i].ossdev.dev_name)+1);
        sprintf(WOutDev[i].ossdev.interface_name, "wineoss: %s", WOutDev[i].ossdev.dev_name);
    }

    /* start with output devices */
    for (i = 0; i < MAX_WAVEDRV; ++i)
    {
        if (*WOutDev[i].ossdev.dev_name == '\0' || OSS_WaveOutInit(&WOutDev[i].ossdev))
        {
            WOutDev[numOutDev].state = WINE_WS_CLOSED;
            WOutDev[numOutDev].volume = 0xffffffff;
            numOutDev++;
        }
    }

    /* then do input devices */
    for (i = 0; i < MAX_WAVEDRV; ++i)
    {
        if (*WInDev[i].ossdev.dev_name=='\0' || OSS_WaveInInit(&WInDev[i].ossdev))
        {
            WInDev[numInDev].state = WINE_WS_CLOSED;
            numInDev++;
        }
    }

    /* finish with the full duplex bits */
    for (i = 0; i < MAX_WAVEDRV; i++)
        if (*WOutDev[i].ossdev.dev_name!='\0')
            OSS_WaveFullDuplexInit(&WOutDev[i].ossdev);

    TRACE("%d wave out devices\n", numOutDev);
    for (i = 0; i < numOutDev; i++) {
        TRACE("%d: %s, %s, %s\n", i, WOutDev[i].ossdev.dev_name,
              WOutDev[i].ossdev.mixer_name, WOutDev[i].ossdev.interface_name);
    }

    TRACE("%d wave in devices\n", numInDev);
    for (i = 0; i < numInDev; i++) {
        TRACE("%d: %s, %s, %s\n", i, WInDev[i].ossdev.dev_name,
              WInDev[i].ossdev.mixer_name, WInDev[i].ossdev.interface_name);
    }

    return 0;
}

/******************************************************************
 *		OSS_WaveExit
 *
 * Delete/clear internal structures of OSS information
 */
LRESULT OSS_WaveExit(void)
{
    int i;
    TRACE("()\n");

    for (i = 0; i < MAX_WAVEDRV; ++i)
    {
        HeapFree(GetProcessHeap(), 0, WOutDev[i].ossdev.dev_name);
        HeapFree(GetProcessHeap(), 0, WOutDev[i].ossdev.mixer_name);
        HeapFree(GetProcessHeap(), 0, WOutDev[i].ossdev.interface_name);
    }

    ZeroMemory(WOutDev, sizeof(WOutDev));
    ZeroMemory(WInDev, sizeof(WInDev));

    numOutDev = 0;
    numInDev = 0;

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
    omr->msg_event = CreateEventW(NULL, FALSE, FALSE, NULL);
#endif
    omr->ring_buffer_size = OSS_RING_BUFFER_INCREMENT;
    omr->messages = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,omr->ring_buffer_size * sizeof(OSS_MSG));
    InitializeCriticalSection(&omr->msg_crst);
    omr->msg_crst.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": OSS_MSG_RING.msg_crst");
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
    HeapFree(GetProcessHeap(),0,omr->messages);
    omr->msg_crst.DebugInfo->Spare[0] = 0;
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
    if ((omr->msg_toget == ((omr->msg_tosave + 1) % omr->ring_buffer_size)))
    {
	int old_ring_buffer_size = omr->ring_buffer_size;
	omr->ring_buffer_size += OSS_RING_BUFFER_INCREMENT;
	TRACE("omr->ring_buffer_size=%d\n",omr->ring_buffer_size);
	omr->messages = HeapReAlloc(GetProcessHeap(),0,omr->messages, omr->ring_buffer_size * sizeof(OSS_MSG));
	/* Now we need to rearrange the ring buffer so that the new
	   buffers just allocated are in between omr->msg_tosave and
	   omr->msg_toget.
	*/
	if (omr->msg_tosave < omr->msg_toget)
	{
	    memmove(&(omr->messages[omr->msg_toget + OSS_RING_BUFFER_INCREMENT]),
		    &(omr->messages[omr->msg_toget]),
		    sizeof(OSS_MSG)*(old_ring_buffer_size - omr->msg_toget)
		    );
	    omr->msg_toget += OSS_RING_BUFFER_INCREMENT;
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
    omr->msg_toget = (omr->msg_toget + 1) % omr->ring_buffer_size;
    CLEAR_OMR(omr);
    LeaveCriticalSection(&omr->msg_crst);
    return 1;
}

/******************************************************************
 *              OSS_PeekRingMessage
 *
 * Peek at a message from the ring but do not remove it.
 * Should be called by the playback/record thread.
 */
static int OSS_PeekRingMessage(OSS_MSG_RING* omr,
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
static DWORD wodNotifyClient(WINE_WAVEOUT* wwo, WORD wMsg, DWORD dwParam1, DWORD dwParam2)
{
    TRACE("wMsg = 0x%04x (%s) dwParm1 = %04X dwParam2 = %04X\n", wMsg,
        wMsg == WOM_OPEN ? "WOM_OPEN" : wMsg == WOM_CLOSE ? "WOM_CLOSE" :
        wMsg == WOM_DONE ? "WOM_DONE" : "Unknown", dwParam1, dwParam2);

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
    DWORD notplayed;
    if (!info) info = &dspspace;

    if (ioctl(wwo->ossdev.fd, SNDCTL_DSP_GETOSPACE, info) < 0) {
        ERR("ioctl(%s, SNDCTL_DSP_GETOSPACE) failed (%s)\n", wwo->ossdev.dev_name, strerror(errno));
        return FALSE;
    }

    /* GETOSPACE is not always accurate when we're down to the last fragment or two;
    **   we try to accommodate that here by assuming that the dsp is empty by looking
    **   at the clock rather than the result of GETOSPACE */
    notplayed = wwo->dwBufferSize - info->bytes;
    if (notplayed > 0 && notplayed < (info->fragsize * 2))
    {
        if (wwo->dwProjectedFinishTime && GetTickCount() >= wwo->dwProjectedFinishTime)
        {
            TRACE("Adjusting for a presumed OSS bug and assuming all data has been played.\n");
            wwo->dwPlayedTotal = wwo->dwWrittenTotal;
            return TRUE;
        }
        else
            /* Some OSS drivers will clean up nicely if given a POST, so give 'em the chance... */
            ioctl(wwo->ossdev.fd, SNDCTL_DSP_POST, 0);
    }

    wwo->dwPlayedTotal = wwo->dwWrittenTotal - notplayed;
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
 * 			     wodPlayer_TicksTillEmpty		[internal]
 * Returns the number of ticks until we think the DSP should be empty
 */
static DWORD wodPlayer_TicksTillEmpty(const WINE_WAVEOUT *wwo)
{
    return ((wwo->dwWrittenTotal - wwo->dwPlayedTotal) * 1000)
        / wwo->waveFormat.Format.nAvgBytesPerSec;
}

/**************************************************************************
 * 			     wodPlayer_DSPWait			[internal]
 * Returns the number of milliseconds to wait for the DSP buffer to write
 * one fragment.
 */
static DWORD wodPlayer_DSPWait(const WINE_WAVEOUT *wwo)
{
    /* time for one fragment to be played */
    return wwo->dwFragmentSize * 1000 / wwo->waveFormat.Format.nAvgBytesPerSec;
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
	dwMillis = (lpWaveHdr->reserved - wwo->dwPlayedTotal) * 1000 / wwo->waveFormat.Format.nAvgBytesPerSec;
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

    TRACE("Writing wavehdr %p.%u[%u]/%u\n",
          wwo->lpPlayPtr, wwo->dwPartialOffset, wwo->lpPlayPtr->dwBufferLength, toWrite);

    if (toWrite > 0)
    {
        written = write(wwo->ossdev.fd, wwo->lpPlayPtr->lpData + wwo->dwPartialOffset, toWrite);
        if (written <= 0) {
            TRACE("write(%s, %p, %d) failed (%s) returned %d\n", wwo->ossdev.dev_name,
                wwo->lpPlayPtr->lpData + wwo->dwPartialOffset, toWrite, strerror(errno), written);
            return FALSE;
        }
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
    TRACE("dwWrittenTotal=%u\n", wwo->dwWrittenTotal);
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
            if (lpWaveHdr->reserved > wwo->dwPlayedTotal) {TRACE("still playing %p (%u/%u)\n", lpWaveHdr, lpWaveHdr->reserved, wwo->dwPlayedTotal);break;}
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
    if (OSS_ResetDevice(&wwo->ossdev) != MMSYSERR_NOERROR)
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
	TRACE("Received %s %x\n", getCmdString(msg), param);
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

    if (!wodUpdatePlayedTotal(wwo, &dspspace)) return INFINITE;
    availInQ = dspspace.bytes;
    TRACE("fragments=%d/%d, fragsize=%d, bytes=%d\n",
	  dspspace.fragments, dspspace.fragstotal, dspspace.fragsize, dspspace.bytes);

    /* no more room... no need to try to feed */
    if (dspspace.fragments != 0) {
        /* Feed from partial wavehdr */
        if (wwo->lpPlayPtr && wwo->dwPartialOffset != 0) {
            wodPlayer_WriteMaxFrags(wwo, &availInQ);
        }

        /* Feed wavehdrs until we run out of wavehdrs or DSP space */
        if (wwo->dwPartialOffset == 0 && wwo->lpPlayPtr) {
            do {
                TRACE("Setting time to elapse for %p to %u\n",
                      wwo->lpPlayPtr, wwo->dwWrittenTotal + wwo->lpPlayPtr->dwBufferLength);
                /* note the value that dwPlayedTotal will return when this wave finishes playing */
                wwo->lpPlayPtr->reserved = wwo->dwWrittenTotal + wwo->lpPlayPtr->dwBufferLength;
            } while (wodPlayer_WriteMaxFrags(wwo, &availInQ) && wwo->lpPlayPtr && availInQ > 0);
        }

        if (wwo->bNeedPost) {
            /* OSS doesn't start before it gets either 2 fragments or a SNDCTL_DSP_POST;
             * if it didn't get one, we give it the other */
            if (wwo->dwBufferSize < availInQ + 2 * wwo->dwFragmentSize)
                ioctl(wwo->ossdev.fd, SNDCTL_DSP_POST, 0);
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
        TRACE("waiting %ums (%u,%u)\n", dwSleepTime, dwNextFeedTime, dwNextNotifyTime);
	WAIT_OMR(&wwo->msgRing, dwSleepTime);
	wodPlayer_ProcessMessages(wwo);
	if (wwo->state == WINE_WS_PLAYING) {
	    dwNextFeedTime = wodPlayer_FeedDSP(wwo);
            if (dwNextFeedTime != INFINITE)
                wwo->dwProjectedFinishTime = GetTickCount() + wodPlayer_TicksTillEmpty(wwo);
            else
                wwo->dwProjectedFinishTime = 0;

	    dwNextNotifyTime = wodPlayer_NotifyCompletions(wwo, FALSE);
	    if (dwNextFeedTime == INFINITE) {
		/* FeedDSP ran out of data, but before flushing, */
		/* check that a notification didn't give us more */
		wodPlayer_ProcessMessages(wwo);
		if (!wwo->lpPlayPtr) {
		    TRACE("flushing\n");
		    ioctl(wwo->ossdev.fd, SNDCTL_DSP_SYNC, 0);
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
static DWORD wodGetDevCaps(WORD wDevID, LPWAVEOUTCAPSW lpCaps, DWORD dwSize)
{
    TRACE("(%u, %p, %u);\n", wDevID, lpCaps, dwSize);

    if (lpCaps == NULL) {
        WARN("not enabled\n");
        return MMSYSERR_NOTENABLED;
    }

    if (wDevID >= numOutDev) {
	WARN("numOutDev reached !\n");
	return MMSYSERR_BADDEVICEID;
    }

    if (WOutDev[wDevID].ossdev.open_access == O_RDWR)
        memcpy(lpCaps, &WOutDev[wDevID].ossdev.duplex_out_caps, min(dwSize, sizeof(*lpCaps)));
    else
        memcpy(lpCaps, &WOutDev[wDevID].ossdev.out_caps, min(dwSize, sizeof(*lpCaps)));

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodOpen				[internal]
 */
DWORD wodOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    int			audio_fragment;
    WINE_WAVEOUT*	wwo;
    audio_buf_info      info;
    DWORD               ret;

    TRACE("(%u, %p[cb=%08x], %08X);\n", wDevID, lpDesc, lpDesc->dwCallback, dwFlags);
    if (lpDesc == NULL) {
	WARN("Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= numOutDev) {
	TRACE("MAX_WAVOUTDRV reached !\n");
	return MMSYSERR_BADDEVICEID;
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

    TRACE("OSS_OpenDevice requested this format: %dx%dx%d %s\n",
          lpDesc->lpFormat->nSamplesPerSec,
          lpDesc->lpFormat->wBitsPerSample,
          lpDesc->lpFormat->nChannels,
          lpDesc->lpFormat->wFormatTag == WAVE_FORMAT_PCM ? "WAVE_FORMAT_PCM" :
          lpDesc->lpFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE ? "WAVE_FORMAT_EXTENSIBLE" :
          "UNSUPPORTED");

    wwo = &WOutDev[wDevID];

    if ((dwFlags & WAVE_DIRECTSOUND) &&
        !(wwo->ossdev.duplex_out_caps.dwSupport & WAVECAPS_DIRECTSOUND))
	/* not supported, ignore it */
	dwFlags &= ~WAVE_DIRECTSOUND;

    if (dwFlags & WAVE_DIRECTSOUND) {
        if (wwo->ossdev.duplex_out_caps.dwSupport & WAVECAPS_SAMPLEACCURATE)
	    /* we have realtime DirectSound, fragments just waste our time,
	     * but a large buffer is good, so choose 64KB (32 * 2^11) */
	    audio_fragment = 0x0020000B;
	else
	    /* to approximate realtime, we must use small fragments,
	     * let's try to fragment the above 64KB (256 * 2^8) */
	    audio_fragment = 0x01000008;
    } else {
	/* A wave device must have a worst case latency of 10 ms so calculate
	 * the largest fragment size less than 10 ms long.
	 */
	int	fsize = lpDesc->lpFormat->nAvgBytesPerSec / 100;	/* 10 ms chunk */
	int	shift = 0;
	while ((1 << shift) <= fsize)
	    shift++;
	shift--;
	audio_fragment = 0x00100000 + shift;	/* 16 fragments of 2^shift */
    }

    TRACE("requesting %d %d byte fragments (%d ms/fragment)\n",
        audio_fragment >> 16, 1 << (audio_fragment & 0xffff),
	((1 << (audio_fragment & 0xffff)) * 1000) / lpDesc->lpFormat->nAvgBytesPerSec);

    if (wwo->state != WINE_WS_CLOSED) {
        WARN("already allocated\n");
        return MMSYSERR_ALLOCATED;
    }

    /* we want to be able to mmap() the device, which means it must be opened readable,
     * otherwise mmap() will fail (at least under Linux) */
    ret = OSS_OpenDevice(&wwo->ossdev,
                         (dwFlags & WAVE_DIRECTSOUND) ? O_RDWR : O_WRONLY,
                         &audio_fragment,
                         (dwFlags & WAVE_DIRECTSOUND) ? 0 : 1,
                         lpDesc->lpFormat->nSamplesPerSec,
                         lpDesc->lpFormat->nChannels,
                         (lpDesc->lpFormat->wBitsPerSample == 16)
                             ? AFMT_S16_LE : AFMT_U8);
    if ((ret==MMSYSERR_NOERROR) && (dwFlags & WAVE_DIRECTSOUND)) {
        lpDesc->lpFormat->nSamplesPerSec=wwo->ossdev.sample_rate;
        lpDesc->lpFormat->nChannels=wwo->ossdev.channels;
        lpDesc->lpFormat->wBitsPerSample=(wwo->ossdev.format == AFMT_U8 ? 8 : 16);
        lpDesc->lpFormat->nBlockAlign=lpDesc->lpFormat->nChannels*lpDesc->lpFormat->wBitsPerSample/8;
        lpDesc->lpFormat->nAvgBytesPerSec=lpDesc->lpFormat->nSamplesPerSec*lpDesc->lpFormat->nBlockAlign;
        TRACE("OSS_OpenDevice returned this format: %dx%dx%d\n",
              lpDesc->lpFormat->nSamplesPerSec,
              lpDesc->lpFormat->wBitsPerSample,
              lpDesc->lpFormat->nChannels);
    }
    if (ret != 0) return ret;
    wwo->state = WINE_WS_STOPPED;

    wwo->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    memcpy(&wwo->waveDesc, lpDesc, sizeof(WAVEOPENDESC));
    copy_format(lpDesc->lpFormat, &wwo->waveFormat);

    if (wwo->waveFormat.Format.wBitsPerSample == 0) {
	WARN("Resetting zeroed wBitsPerSample\n");
	wwo->waveFormat.Format.wBitsPerSample = 8 *
	    (wwo->waveFormat.Format.nAvgBytesPerSec /
	     wwo->waveFormat.Format.nSamplesPerSec) /
	    wwo->waveFormat.Format.nChannels;
    }
    /* Read output space info for future reference */
    if (ioctl(wwo->ossdev.fd, SNDCTL_DSP_GETOSPACE, &info) < 0) {
	ERR("ioctl(%s, SNDCTL_DSP_GETOSPACE) failed (%s)\n", wwo->ossdev.dev_name, strerror(errno));
        OSS_CloseDevice(&wwo->ossdev);
	wwo->state = WINE_WS_CLOSED;
	return MMSYSERR_NOTENABLED;
    }

    TRACE("got %d %d byte fragments (%d ms/fragment)\n", info.fragstotal,
        info.fragsize, (info.fragsize * 1000) / (wwo->ossdev.sample_rate *
        wwo->ossdev.channels * (wwo->ossdev.format == AFMT_U8 ? 1 : 2)));

    /* Check that fragsize is correct per our settings above */
    if ((info.fragsize > 1024) && (LOWORD(audio_fragment) <= 10)) {
        /* we've tried to set 1K fragments or less, but it didn't work */
        WARN("fragment size set failed, size is now %d\n", info.fragsize);
    }

    /* Remember fragsize and total buffer size for future use */
    wwo->dwFragmentSize = info.fragsize;
    wwo->dwBufferSize = info.fragstotal * info.fragsize;
    wwo->dwPlayedTotal = 0;
    wwo->dwWrittenTotal = 0;
    wwo->bNeedPost = TRUE;

    TRACE("fd=%d fragstotal=%d fragsize=%d BufferSize=%d\n",
          wwo->ossdev.fd, info.fragstotal, info.fragsize, wwo->dwBufferSize);
    if (wwo->dwFragmentSize % wwo->waveFormat.Format.nBlockAlign) {
        ERR("Fragment doesn't contain an integral number of data blocks fragsize=%d BlockAlign=%d\n",wwo->dwFragmentSize,wwo->waveFormat.Format.nBlockAlign);
        /* Some SoundBlaster 16 cards return an incorrect (odd) fragment
         * size for 16 bit sound. This will cause a system crash when we try
         * to write just the specified odd number of bytes. So if we
         * detect something is wrong we'd better fix it.
         */
        wwo->dwFragmentSize-=wwo->dwFragmentSize % wwo->waveFormat.Format.nBlockAlign;
    }

    OSS_InitRingMessage(&wwo->msgRing);

    wwo->hStartUpEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    wwo->hThread = CreateThread(NULL, 0, wodPlayer, (LPVOID)(DWORD)wDevID, 0, &(wwo->dwThreadID));
    if (wwo->hThread)
        SetThreadPriority(wwo->hThread, THREAD_PRIORITY_TIME_CRITICAL);
    WaitForSingleObject(wwo->hStartUpEvent, INFINITE);
    CloseHandle(wwo->hStartUpEvent);
    wwo->hStartUpEvent = INVALID_HANDLE_VALUE;

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

        OSS_DestroyRingMessage(&wwo->msgRing);

        OSS_CloseDevice(&wwo->ossdev);
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
    TRACE("(%u, %p, %08X);\n", wDevID, lpWaveHdr, dwSize);

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

    if ((lpWaveHdr->dwBufferLength & (WOutDev[wDevID].waveFormat.Format.nBlockAlign - 1)) != 0)
    {
        WARN("WaveHdr length isn't a multiple of the PCM block size: %d %% %d\n",lpWaveHdr->dwBufferLength,WOutDev[wDevID].waveFormat.Format.nBlockAlign);
        lpWaveHdr->dwBufferLength &= ~(WOutDev[wDevID].waveFormat.Format.nBlockAlign - 1);
    }

    OSS_AddRingMessage(&WOutDev[wDevID].msgRing, WINE_WM_HEADER, (DWORD)lpWaveHdr, FALSE);

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
    WINE_WAVEOUT*	wwo;

    TRACE("(%u, %p, %u);\n", wDevID, lpTime, uSize);

    if (wDevID >= numOutDev || WOutDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }

    if (lpTime == NULL) {
	WARN("invalid parameter: lpTime == NULL\n");
	return MMSYSERR_INVALPARAM;
    }

    wwo = &WOutDev[wDevID];
#ifdef EXACT_WODPOSITION
    if (wwo->ossdev.open_access == O_RDWR) {
        if (wwo->ossdev.duplex_out_caps.dwSupport & WAVECAPS_SAMPLEACCURATE)
	    OSS_AddRingMessage(&wwo->msgRing, WINE_WM_UPDATE, 0, TRUE);
    } else {
        if (wwo->ossdev.out_caps.dwSupport & WAVECAPS_SAMPLEACCURATE)
	    OSS_AddRingMessage(&wwo->msgRing, WINE_WM_UPDATE, 0, TRUE);
    }
#endif

    return bytes_to_mmtime(lpTime, wwo->dwPlayedTotal, &wwo->waveFormat);
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
    int		mixer;
    int		volume;
    DWORD	left, right;
    DWORD	last_left, last_right;

    TRACE("(%u, %p);\n", wDevID, lpdwVol);

    if (lpdwVol == NULL) {
        WARN("not enabled\n");
        return MMSYSERR_NOTENABLED;
    }
    if (wDevID >= numOutDev) {
        WARN("invalid parameter\n");
        return MMSYSERR_INVALPARAM;
    }
    if (WOutDev[wDevID].ossdev.open_access == O_RDWR) {
        if (!(WOutDev[wDevID].ossdev.duplex_out_caps.dwSupport & WAVECAPS_VOLUME)) {
            TRACE("Volume not supported\n");
            return MMSYSERR_NOTSUPPORTED;
        }
    } else {
        if (!(WOutDev[wDevID].ossdev.out_caps.dwSupport & WAVECAPS_VOLUME)) {
            TRACE("Volume not supported\n");
            return MMSYSERR_NOTSUPPORTED;
        }
    }

    if ((mixer = open(WOutDev[wDevID].ossdev.mixer_name, O_RDONLY|O_NDELAY)) < 0) {
        WARN("mixer device not available !\n");
        return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, SOUND_MIXER_READ_PCM, &volume) == -1) {
        close(mixer);
        WARN("ioctl(%s, SOUND_MIXER_READ_PCM) failed (%s)\n",
             WOutDev[wDevID].ossdev.mixer_name, strerror(errno));
        return MMSYSERR_NOTENABLED;
    }
    close(mixer);

    left = LOBYTE(volume);
    right = HIBYTE(volume);
    TRACE("left=%d right=%d !\n", left, right);
    last_left  = (LOWORD(WOutDev[wDevID].volume) * 100) / 0xFFFFl;
    last_right = (HIWORD(WOutDev[wDevID].volume) * 100) / 0xFFFFl;
    TRACE("last_left=%d last_right=%d !\n", last_left, last_right);
    if (last_left == left && last_right == right)
	*lpdwVol = WOutDev[wDevID].volume;
    else
	*lpdwVol = ((left * 0xFFFFl) / 100) + (((right * 0xFFFFl) / 100) << 16);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodSetVolume			[internal]
 */
DWORD wodSetVolume(WORD wDevID, DWORD dwParam)
{
    int		mixer;
    int		volume;
    DWORD	left, right;

    TRACE("(%u, %08X);\n", wDevID, dwParam);

    left  = (LOWORD(dwParam) * 100) / 0xFFFFl;
    right = (HIWORD(dwParam) * 100) / 0xFFFFl;
    volume = left + (right << 8);

    if (wDevID >= numOutDev) {
        WARN("invalid parameter: wDevID > %d\n", numOutDev);
        return MMSYSERR_INVALPARAM;
    }
    if (WOutDev[wDevID].ossdev.open_access == O_RDWR) {
        if (!(WOutDev[wDevID].ossdev.duplex_out_caps.dwSupport & WAVECAPS_VOLUME)) {
            TRACE("Volume not supported\n");
            return MMSYSERR_NOTSUPPORTED;
        }
    } else {
        if (!(WOutDev[wDevID].ossdev.out_caps.dwSupport & WAVECAPS_VOLUME)) {
            TRACE("Volume not supported\n");
            return MMSYSERR_NOTSUPPORTED;
        }
    }
    if ((mixer = open(WOutDev[wDevID].ossdev.mixer_name, O_WRONLY|O_NDELAY)) < 0) {
        WARN("open(%s) failed (%s)\n", WOutDev[wDevID].ossdev.mixer_name, strerror(errno));
        return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, SOUND_MIXER_WRITE_PCM, &volume) == -1) {
        close(mixer);
        WARN("ioctl(%s, SOUND_MIXER_WRITE_PCM) failed (%s)\n",
            WOutDev[wDevID].ossdev.mixer_name, strerror(errno));
        return MMSYSERR_NOTENABLED;
    }
    TRACE("volume=%04x\n", (unsigned)volume);
    close(mixer);

    /* save requested volume */
    WOutDev[wDevID].volume = dwParam;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodMessage (WINEOSS.7)
 */
DWORD WINAPI OSS_wodMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%u, %s, %08X, %08X, %08X);\n",
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
    case WODM_WRITE:	 	return wodWrite		(wDevID, (LPWAVEHDR)dwParam1,		dwParam2);
    case WODM_PAUSE:	 	return wodPause		(wDevID);
    case WODM_GETPOS:	 	return wodGetPosition	(wDevID, (LPMMTIME)dwParam1, 		dwParam2);
    case WODM_BREAKLOOP: 	return wodBreakLoop     (wDevID);
    case WODM_PREPARE:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_UNPREPARE: 	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETDEVCAPS:	return wodGetDevCaps	(wDevID, (LPWAVEOUTCAPSW)dwParam1,	dwParam2);
    case WODM_GETNUMDEVS:	return numOutDev;
    case WODM_GETPITCH:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPITCH:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETVOLUME:	return wodGetVolume	(wDevID, (LPDWORD)dwParam1);
    case WODM_SETVOLUME:	return wodSetVolume	(wDevID, dwParam1);
    case WODM_RESTART:		return wodRestart	(wDevID);
    case WODM_RESET:		return wodReset		(wDevID);

    case DRV_QUERYDEVICEINTERFACESIZE: return wodDevInterfaceSize      (wDevID, (LPDWORD)dwParam1);
    case DRV_QUERYDEVICEINTERFACE:     return wodDevInterface          (wDevID, (PWCHAR)dwParam1, dwParam2);
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
 * 			widNotifyClient			[internal]
 */
static DWORD widNotifyClient(WINE_WAVEIN* wwi, WORD wMsg, DWORD dwParam1, DWORD dwParam2)
{
    TRACE("wMsg = 0x%04x (%s) dwParm1 = %04X dwParam2 = %04X\n", wMsg,
        wMsg == WIM_OPEN ? "WIM_OPEN" : wMsg == WIM_CLOSE ? "WIM_CLOSE" :
        wMsg == WIM_DATA ? "WIM_DATA" : "Unknown", dwParam1, dwParam2);

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

    if (wDevID >= numInDev) {
	TRACE("numOutDev reached !\n");
	return MMSYSERR_BADDEVICEID;
    }

    memcpy(lpCaps, &WInDev[wDevID].ossdev.in_caps, min(dwSize, sizeof(*lpCaps)));
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widRecorder_ReadHeaders		[internal]
 */
static void widRecorder_ReadHeaders(WINE_WAVEIN * wwi)
{
    enum win_wm_message tmp_msg;
    DWORD		tmp_param;
    HANDLE		tmp_ev;
    WAVEHDR*		lpWaveHdr;

    while (OSS_RetrieveRingMessage(&wwi->msgRing, &tmp_msg, &tmp_param, &tmp_ev)) {
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
    WINE_WAVEIN*	wwi = (WINE_WAVEIN*)&WInDev[uDevID];
    WAVEHDR*		lpWaveHdr;
    DWORD		dwSleepTime;
    DWORD		bytesRead;
    LPVOID		buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, wwi->dwFragmentSize);
    char               *pOffset = buffer;
    audio_buf_info 	info;
    int 		xs;
    enum win_wm_message msg;
    DWORD		param;
    HANDLE		ev;
    int                 enable;

    wwi->state = WINE_WS_STOPPED;
    wwi->dwTotalRecorded = 0;
    wwi->dwTotalRead = 0;
    wwi->lpQueuePtr = NULL;

    SetEvent(wwi->hStartUpEvent);

    /* disable input so capture will begin when triggered */
    wwi->ossdev.bInputEnabled = FALSE;
    enable = getEnables(&wwi->ossdev);
    if (ioctl(wwi->ossdev.fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0)
	ERR("ioctl(%s, SNDCTL_DSP_SETTRIGGER) failed (%s)\n", wwi->ossdev.dev_name, strerror(errno));

    /* the soundblaster live needs a micro wake to get its recording started
     * (or GETISPACE will have 0 frags all the time)
     */
    read(wwi->ossdev.fd, &xs, 4);

    /* make sleep time to be # of ms to output a fragment */
    dwSleepTime = (wwi->dwFragmentSize * 1000) / wwi->waveFormat.Format.nAvgBytesPerSec;
    TRACE("sleeptime=%d ms\n", dwSleepTime);

    for (;;) {
	/* wait for dwSleepTime or an event in thread's queue */
	/* FIXME: could improve wait time depending on queue state,
	 * ie, number of queued fragments
	 */

	if (wwi->lpQueuePtr != NULL && wwi->state == WINE_WS_PLAYING)
        {
            lpWaveHdr = wwi->lpQueuePtr;

	    ioctl(wwi->ossdev.fd, SNDCTL_DSP_GETISPACE, &info);
            TRACE("info={frag=%d fsize=%d ftotal=%d bytes=%d}\n", info.fragments, info.fragsize, info.fragstotal, info.bytes);

            /* read all the fragments accumulated so far */
            while ((info.fragments > 0) && (wwi->lpQueuePtr))
            {
                info.fragments --;

                if (lpWaveHdr->dwBufferLength - lpWaveHdr->dwBytesRecorded >= wwi->dwFragmentSize)
                {
                    /* directly read fragment in wavehdr */
                    bytesRead = read(wwi->ossdev.fd,
		      		     lpWaveHdr->lpData + lpWaveHdr->dwBytesRecorded,
                                     wwi->dwFragmentSize);

                    TRACE("bytesRead=%d (direct)\n", bytesRead);
                    if (bytesRead != (DWORD) -1)
		    {
			/* update number of bytes recorded in current buffer and by this device */
                        lpWaveHdr->dwBytesRecorded += bytesRead;
			wwi->dwTotalRead           += bytesRead;
			wwi->dwTotalRecorded = wwi->dwTotalRead;

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
                        TRACE("read(%s, %p, %d) failed (%s)\n", wwi->ossdev.dev_name,
                            lpWaveHdr->lpData + lpWaveHdr->dwBytesRecorded,
                            wwi->dwFragmentSize, strerror(errno));
                    }
                }
                else
		{
                    /* read the fragment in a local buffer */
                    bytesRead = read(wwi->ossdev.fd, buffer, wwi->dwFragmentSize);
                    pOffset = buffer;

                    TRACE("bytesRead=%d (local)\n", bytesRead);

                    if (bytesRead == (DWORD) -1) {
                        TRACE("read(%s, %p, %d) failed (%s)\n", wwi->ossdev.dev_name,
                            buffer, wwi->dwFragmentSize, strerror(errno));
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
                        wwi->dwTotalRead           += dwToCopy;
			wwi->dwTotalRecorded = wwi->dwTotalRead;
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
				while (OSS_PeekRingMessage(&wwi->msgRing, &msg, &param, &ev))
				{
				    if (msg == WINE_WM_HEADER) {
					LPWAVEHDR hdr;
					OSS_RetrieveRingMessage(&wwi->msgRing, &msg, &param, &ev);
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
                                    WARN("buffer under run! %u bytes dropped.\n", bytesRead);
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

	while (OSS_RetrieveRingMessage(&wwi->msgRing, &msg, &param, &ev))
	{
            TRACE("msg=%s param=0x%x\n", getCmdString(msg), param);
	    switch (msg) {
	    case WINE_WM_PAUSING:
		wwi->state = WINE_WS_PAUSED;
                /*FIXME("Device should stop recording\n");*/
		SetEvent(ev);
		break;
	    case WINE_WM_STARTING:
		wwi->state = WINE_WS_PLAYING;

                if (wwi->ossdev.bTriggerSupport)
                {
                    /* start the recording */
		    wwi->ossdev.bInputEnabled = TRUE;
                    enable = getEnables(&wwi->ossdev);
                    if (ioctl(wwi->ossdev.fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
		        wwi->ossdev.bInputEnabled = FALSE;
                        ERR("ioctl(%s, SNDCTL_DSP_SETTRIGGER) failed (%s)\n", wwi->ossdev.dev_name, strerror(errno));
		    }
                }
                else
                {
                    unsigned char data[4];
                    /* read 4 bytes to start the recording */
                    read(wwi->ossdev.fd, data, 4);
                }

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
                    if (wwi->ossdev.bTriggerSupport)
                    {
                        /* stop the recording */
		        wwi->ossdev.bInputEnabled = FALSE;
                        enable = getEnables(&wwi->ossdev);
                        if (ioctl(wwi->ossdev.fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
		            wwi->ossdev.bInputEnabled = FALSE;
                            ERR("ioctl(%s, SNDCTL_DSP_SETTRIGGER) failed (%s)\n", wwi->ossdev.dev_name, strerror(errno));
		        }
                    }

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
                    if (wwi->ossdev.bTriggerSupport)
                    {
                        /* stop the recording */
		        wwi->ossdev.bInputEnabled = FALSE;
                        enable = getEnables(&wwi->ossdev);
                        if (ioctl(wwi->ossdev.fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
		            wwi->ossdev.bInputEnabled = FALSE;
                            ERR("ioctl(%s, SNDCTL_DSP_SETTRIGGER) failed (%s)\n", wwi->ossdev.dev_name, strerror(errno));
		        }
                    }
		}
		wwi->state = WINE_WS_STOPPED;
    		wwi->dwTotalRecorded = 0;
		wwi->dwTotalRead = 0;

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
	    case WINE_WM_UPDATE:
		if (wwi->state == WINE_WS_PLAYING) {
		    audio_buf_info tmp_info;
		    if (ioctl(wwi->ossdev.fd, SNDCTL_DSP_GETISPACE, &tmp_info) < 0)
			ERR("ioctl(%s, SNDCTL_DSP_GETISPACE) failed (%s)\n", wwi->ossdev.dev_name, strerror(errno));
		    else
			wwi->dwTotalRecorded = wwi->dwTotalRead + tmp_info.bytes;
		}
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
DWORD widOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    WINE_WAVEIN*	wwi;
    audio_buf_info      info;
    int                 audio_fragment;
    DWORD               ret;

    TRACE("(%u, %p, %08X);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN("Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= numInDev) {
        WARN("bad device id: %d >= %d\n", wDevID, numInDev);
        return MMSYSERR_BADDEVICEID;
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

    TRACE("OSS_OpenDevice requested this format: %dx%dx%d %s\n",
          lpDesc->lpFormat->nSamplesPerSec,
          lpDesc->lpFormat->wBitsPerSample,
          lpDesc->lpFormat->nChannels,
          lpDesc->lpFormat->wFormatTag == WAVE_FORMAT_PCM ? "WAVE_FORMAT_PCM" :
          lpDesc->lpFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE ? "WAVE_FORMAT_EXTENSIBLE" :
          "UNSUPPORTED");

    wwi = &WInDev[wDevID];

    if (wwi->state != WINE_WS_CLOSED) return MMSYSERR_ALLOCATED;

    if ((dwFlags & WAVE_DIRECTSOUND) &&
        !(wwi->ossdev.in_caps_support & WAVECAPS_DIRECTSOUND))
	/* not supported, ignore it */
	dwFlags &= ~WAVE_DIRECTSOUND;

    if (dwFlags & WAVE_DIRECTSOUND) {
	TRACE("has DirectSoundCapture driver\n");
        if (wwi->ossdev.in_caps_support & WAVECAPS_SAMPLEACCURATE)
	    /* we have realtime DirectSound, fragments just waste our time,
	     * but a large buffer is good, so choose 64KB (32 * 2^11) */
	    audio_fragment = 0x0020000B;
	else
	    /* to approximate realtime, we must use small fragments,
	     * let's try to fragment the above 64KB (256 * 2^8) */
	    audio_fragment = 0x01000008;
    } else {
	TRACE("doesn't have DirectSoundCapture driver\n");
	if (wwi->ossdev.open_count > 0) {
	    TRACE("Using output device audio_fragment\n");
	    /* FIXME: This may not be optimal for capture but it allows us
	     * to do hardware playback without hardware capture. */
	    audio_fragment = wwi->ossdev.audio_fragment;
	} else {
	    /* A wave device must have a worst case latency of 10 ms so calculate
	     * the largest fragment size less than 10 ms long.
	     */
	    int	fsize = lpDesc->lpFormat->nAvgBytesPerSec / 100;	/* 10 ms chunk */
	    int	shift = 0;
	    while ((1 << shift) <= fsize)
		shift++;
	    shift--;
	    audio_fragment = 0x00100000 + shift;	/* 16 fragments of 2^shift */
	}
    }

    TRACE("requesting %d %d byte fragments (%d ms)\n", audio_fragment >> 16,
	1 << (audio_fragment & 0xffff),
	((1 << (audio_fragment & 0xffff)) * 1000) / lpDesc->lpFormat->nAvgBytesPerSec);

    ret = OSS_OpenDevice(&wwi->ossdev, O_RDONLY, &audio_fragment,
                         1,
                         lpDesc->lpFormat->nSamplesPerSec,
                         lpDesc->lpFormat->nChannels,
                         (lpDesc->lpFormat->wBitsPerSample == 16)
                         ? AFMT_S16_LE : AFMT_U8);
    if (ret != 0) return ret;
    wwi->state = WINE_WS_STOPPED;

    if (wwi->lpQueuePtr) {
	WARN("Should have an empty queue (%p)\n", wwi->lpQueuePtr);
	wwi->lpQueuePtr = NULL;
    }
    wwi->dwTotalRecorded = 0;
    wwi->dwTotalRead = 0;
    wwi->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    memcpy(&wwi->waveDesc, lpDesc, sizeof(WAVEOPENDESC));
    copy_format(lpDesc->lpFormat, &wwi->waveFormat);

    if (wwi->waveFormat.Format.wBitsPerSample == 0) {
	WARN("Resetting zeroed wBitsPerSample\n");
	wwi->waveFormat.Format.wBitsPerSample = 8 *
	    (wwi->waveFormat.Format.nAvgBytesPerSec /
	     wwi->waveFormat.Format.nSamplesPerSec) /
	    wwi->waveFormat.Format.nChannels;
    }

    if (ioctl(wwi->ossdev.fd, SNDCTL_DSP_GETISPACE, &info) < 0) {
        ERR("ioctl(%s, SNDCTL_DSP_GETISPACE) failed (%s)\n",
            wwi->ossdev.dev_name, strerror(errno));
        OSS_CloseDevice(&wwi->ossdev);
	wwi->state = WINE_WS_CLOSED;
	return MMSYSERR_NOTENABLED;
    }

    TRACE("got %d %d byte fragments (%d ms/fragment)\n", info.fragstotal,
        info.fragsize, (info.fragsize * 1000) / (wwi->ossdev.sample_rate *
        wwi->ossdev.channels * (wwi->ossdev.format == AFMT_U8 ? 1 : 2)));

    wwi->dwFragmentSize = info.fragsize;

    TRACE("dwFragmentSize=%u\n", wwi->dwFragmentSize);
    TRACE("wBitsPerSample=%u, nAvgBytesPerSec=%u, nSamplesPerSec=%u, nChannels=%u nBlockAlign=%u!\n",
	  wwi->waveFormat.Format.wBitsPerSample, wwi->waveFormat.Format.nAvgBytesPerSec,
	  wwi->waveFormat.Format.nSamplesPerSec, wwi->waveFormat.Format.nChannels,
	  wwi->waveFormat.Format.nBlockAlign);

    OSS_InitRingMessage(&wwi->msgRing);

    wwi->hStartUpEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    wwi->hThread = CreateThread(NULL, 0, widRecorder, (LPVOID)(DWORD)wDevID, 0, &(wwi->dwThreadID));
    if (wwi->hThread)
        SetThreadPriority(wwi->hThread, THREAD_PRIORITY_TIME_CRITICAL);
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
    OSS_CloseDevice(&wwi->ossdev);
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
    TRACE("(%u, %p, %08X);\n", wDevID, lpWaveHdr, dwSize);

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
 * 			widStart				[internal]
 */
static DWORD widStart(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    if (wDevID >= numInDev || WInDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("can't start recording !\n");
	return MMSYSERR_INVALHANDLE;
    }

    OSS_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_STARTING, 0, TRUE);
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

    OSS_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_STOPPING, 0, TRUE);

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
    WINE_WAVEIN*	wwi;

    TRACE("(%u, %p, %u);\n", wDevID, lpTime, uSize);

    if (wDevID >= numInDev || WInDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("can't get pos !\n");
	return MMSYSERR_INVALHANDLE;
    }

    if (lpTime == NULL) {
	WARN("invalid parameter: lpTime == NULL\n");
	return MMSYSERR_INVALPARAM;
    }

    wwi = &WInDev[wDevID];
#ifdef EXACT_WIDPOSITION
    if (wwi->ossdev.in_caps_support & WAVECAPS_SAMPLEACCURATE)
        OSS_AddRingMessage(&(wwi->msgRing), WINE_WM_UPDATE, 0, TRUE);
#endif

    return bytes_to_mmtime(lpTime, wwi->dwTotalRecorded, &wwi->waveFormat);
}

/**************************************************************************
 * 				widMessage (WINEOSS.6)
 */
DWORD WINAPI OSS_widMessage(WORD wDevID, WORD wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%u, %s, %08X, %08X, %08X);\n",
	  wDevID, getMessage(wMsg), dwUser, dwParam1, dwParam2);

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
    case WIDM_GETNUMDEVS:	return numInDev;
    case WIDM_GETPOS:		return widGetPosition(wDevID, (LPMMTIME)dwParam1, dwParam2);
    case WIDM_RESET:		return widReset      (wDevID);
    case WIDM_START:		return widStart      (wDevID);
    case WIDM_STOP:		return widStop       (wDevID);
    case DRV_QUERYDEVICEINTERFACESIZE: return widDevInterfaceSize      (wDevID, (LPDWORD)dwParam1);
    case DRV_QUERYDEVICEINTERFACE:     return widDevInterface          (wDevID, (PWCHAR)dwParam1, dwParam2);
    case DRV_QUERYDSOUNDIFACE:	return widDsCreate   (wDevID, (PIDSCDRIVER*)dwParam1);
    case DRV_QUERYDSOUNDDESC:	return widDsDesc     (wDevID, (PDSDRIVERDESC)dwParam1);
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
    FIXME("(%u, %04X, %08X, %08X, %08X):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				widMessage (WINEOSS.6)
 */
DWORD WINAPI OSS_widMessage(WORD wDevID, WORD wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08X, %08X, %08X):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

#endif /* HAVE_OSS */
