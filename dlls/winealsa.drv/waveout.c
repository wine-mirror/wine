/*
 * Sample Wine Driver for Advanced Linux Sound System (ALSA)
 *      Based on version <final> of the ALSA API
 *
 * Copyright    2002 Eric Pouech
 *              2002 Marco Pietrobono
 *              2003 Christian Costa : WaveIn support
 *              2006-2007 Maarten Lankhorst
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

/*======================================================================*
 *                  Low level WAVE OUT implementation			*
 *======================================================================*/

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
#include "winuser.h"
#include "winnls.h"
#include "mmddk.h"

#include "alsa.h"

#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wave);

#ifdef HAVE_ALSA

WINE_WAVEDEV	*WOutDev;
DWORD            ALSA_WodNumMallocedDevs;
DWORD            ALSA_WodNumDevs;

/**************************************************************************
 * 			wodNotifyClient			[internal]
 */
static DWORD wodNotifyClient(WINE_WAVEDEV* wwo, WORD wMsg, DWORD dwParam1, DWORD dwParam2)
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
static BOOL wodUpdatePlayedTotal(WINE_WAVEDEV* wwo, snd_pcm_status_t* ps)
{
    snd_pcm_sframes_t delay = 0;
    snd_pcm_state_t state;

    state = snd_pcm_state(wwo->pcm);
    snd_pcm_delay(wwo->pcm, &delay);

    /* A delay < 0 indicates an underrun; for our purposes that's 0.  */
    if ( (state != SND_PCM_STATE_RUNNING && state != SND_PCM_STATE_PREPARED) || (delay < 0))
    {
        WARN("Unexpected state (%d) or delay (%ld) while updating Total Played, resetting\n", state, delay);
        delay=0;
    }
    InterlockedExchange((LONG*)&wwo->dwPlayedTotal, wwo->dwWrittenTotal - snd_pcm_frames_to_bytes(wwo->pcm, delay));
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
    DWORD dwLength = snd_pcm_bytes_to_frames(wwo->pcm, lpWaveHdr->dwBufferLength - wwo->dwPartialOffset);
    int toWrite = min(dwLength, *frames);
    int written;

    TRACE("Writing wavehdr %p.%u[%u]\n", lpWaveHdr, wwo->dwPartialOffset, lpWaveHdr->dwBufferLength);

    if (toWrite > 0) {
	written = (wwo->write)(wwo->pcm, lpWaveHdr->lpData + wwo->dwPartialOffset, toWrite);
	if ( written < 0) {
	    /* XRUN occurred. let's try to recover */
	    ALSA_XRUNRecovery(wwo, written);
	    written = (wwo->write)(wwo->pcm, lpWaveHdr->lpData + wwo->dwPartialOffset, toWrite);
	}
	if (written <= 0) {
	    /* still in error */
	    ERR("Error in writing wavehdr. Reason: %s\n", snd_strerror(written));
	    return written;
	}
    } else
	written = 0;

    wwo->dwPartialOffset += snd_pcm_frames_to_bytes(wwo->pcm, written);
    if ( wwo->dwPartialOffset >= lpWaveHdr->dwBufferLength) {
	/* this will be used to check if the given wave header has been fully played or not... */
	wwo->dwPartialOffset = lpWaveHdr->dwBufferLength;
	/* If we wrote all current wavehdr, skip to the next one */
	wodPlayer_PlayPtrNext(wwo);
    }
    *frames -= written;
    wwo->dwWrittenTotal += snd_pcm_frames_to_bytes(wwo->pcm, written);
    TRACE("dwWrittenTotal=%u\n", wwo->dwWrittenTotal);

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
    return  (lpWaveHdr && lpWaveHdr != wwo->lpPlayPtr && lpWaveHdr != wwo->lpLoopPtr) ?
        wodPlayer_NotifyWait(wwo, lpWaveHdr) : INFINITE;
}


/**************************************************************************
 * 				wodPlayer_Reset			[internal]
 *
 * wodPlayer helper. Resets current output stream.
 */
static	void	wodPlayer_Reset(WINE_WAVEDEV* wwo, BOOL reset)
{
    int                         err;
    TRACE("(%p)\n", wwo);

    /* flush all possible output */
    snd_pcm_drain(wwo->pcm);

    wodUpdatePlayedTotal(wwo, NULL);
    /* updates current notify list */
    wodPlayer_NotifyCompletions(wwo, FALSE);

    if ( (err = snd_pcm_drop(wwo->pcm)) < 0) {
	FIXME("flush: %s\n", snd_strerror(err));
	wwo->hThread = 0;
	wwo->state = WINE_WS_STOPPED;
	ExitThread(-1);
    }
    if ( (err = snd_pcm_prepare(wwo->pcm)) < 0 )
        ERR("pcm prepare failed: %s\n", snd_strerror(err));

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
        ALSA_ResetRingMessage(&wwo->msgRing);
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
static void wodPlayer_ProcessMessages(WINE_WAVEDEV* wwo)
{
    LPWAVEHDR           lpWaveHdr;
    enum win_wm_message	msg;
    DWORD		param;
    HANDLE		ev;
    int                 err;

    while (ALSA_RetrieveRingMessage(&wwo->msgRing, &msg, &param, &ev)) {
     TRACE("Received %s %x\n", ALSA_getCmdString(msg), param);

	switch (msg) {
	case WINE_WM_PAUSING:
	    if ( snd_pcm_state(wwo->pcm) == SND_PCM_STATE_RUNNING )
	     {
                if ( snd_pcm_hw_params_can_pause(wwo->hw_params) )
		{
		    err = snd_pcm_pause(wwo->pcm, 1);
		    if ( err < 0 )
			ERR("pcm_pause failed: %s\n", snd_strerror(err));
		    wwo->state = WINE_WS_PAUSED;
		}
		else
		{
		    wodPlayer_Reset(wwo,FALSE);
		}
	     }
	    SetEvent(ev);
	    break;
	case WINE_WM_RESTARTING:
            if (wwo->state == WINE_WS_PAUSED)
            {
		if ( snd_pcm_state(wwo->pcm) == SND_PCM_STATE_PAUSED )
		 {
		    err = snd_pcm_pause(wwo->pcm, 0);
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
	    wodPlayer_Reset(wwo,TRUE);
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
    availInQ = snd_pcm_avail_update(wwo->pcm);

    /* no more room... no need to try to feed */
    if (availInQ > 0) {
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
        TRACE("waiting %ums (%u,%u)\n", dwSleepTime, dwNextFeedTime, dwNextNotifyTime);
        ALSA_WaitRingMessage(&wwo->msgRing, dwSleepTime);
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
    TRACE("(%u, %p, %u);\n", wDevID, lpCaps, dwSize);

    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;

    if (wDevID >= ALSA_WodNumDevs) {
	TRACE("Asked for device %d, but only %d known!\n", wDevID, ALSA_WodNumDevs);
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
    snd_pcm_t *                 pcm = NULL;
    snd_hctl_t *                hctl = NULL;
    snd_pcm_hw_params_t *       hw_params = NULL;
    snd_pcm_sw_params_t *       sw_params;
    snd_pcm_access_t            access;
    snd_pcm_format_t            format = -1;
    unsigned int                rate;
    unsigned int                buffer_time = 500000;
    unsigned int                period_time = 10000;
    snd_pcm_uframes_t           buffer_size;
    snd_pcm_uframes_t           period_size;
    int                         flags;
    int                         err=0;
    int                         dir=0;
    DWORD                       retcode = 0;

    snd_pcm_sw_params_alloca(&sw_params);

    TRACE("(%u, %p, %08X);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN("Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= ALSA_WodNumDevs) {
	TRACE("Asked for device %d, but only %d known!\n", wDevID, ALSA_WodNumDevs);
	return MMSYSERR_BADDEVICEID;
    }

    /* only PCM format is supported so far... */
    if (!ALSA_supportedFormat(lpDesc->lpFormat)) {
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

    if (wwo->pcm != NULL) {
        WARN("%d already allocated\n", wDevID);
        return MMSYSERR_ALLOCATED;
    }

    if (dwFlags & WAVE_DIRECTSOUND)
        FIXME("Why are we called with DirectSound flag? It doesn't use MMSYSTEM any more\n");
        /* not supported, ignore it */
    dwFlags &= ~WAVE_DIRECTSOUND;

    flags = SND_PCM_NONBLOCK;

    if ( (err = snd_pcm_open(&pcm, wwo->pcmname, SND_PCM_STREAM_PLAYBACK, flags)) < 0)
    {
        ERR("Error open: %s\n", snd_strerror(err));
	return MMSYSERR_NOTENABLED;
    }

    if (wwo->ctlname)
    {
        err = snd_hctl_open(&hctl, wwo->ctlname, 0);
        if (err >= 0)
        {
            snd_hctl_load(hctl);
        }
        else
        {
            WARN("Could not open hctl for [%s]: %s\n", wwo->ctlname, snd_strerror(err));
            hctl = NULL;
        }
    }

    wwo->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    memcpy(&wwo->waveDesc, lpDesc, sizeof(WAVEOPENDESC));
    ALSA_copyFormat(lpDesc->lpFormat, &wwo->format);

    TRACE("Requested this format: %dx%dx%d %s\n",
          wwo->format.Format.nSamplesPerSec,
          wwo->format.Format.wBitsPerSample,
          wwo->format.Format.nChannels,
          ALSA_getFormat(wwo->format.Format.wFormatTag));

    if (wwo->format.Format.wBitsPerSample == 0) {
	WARN("Resetting zeroed wBitsPerSample\n");
	wwo->format.Format.wBitsPerSample = 8 *
	    (wwo->format.Format.nAvgBytesPerSec /
	     wwo->format.Format.nSamplesPerSec) /
	    wwo->format.Format.nChannels;
    }

#define EXIT_ON_ERROR(f,e,txt) do \
{ \
    int err; \
    if ( (err = (f) ) < 0) \
    { \
	WARN(txt ": %s\n", snd_strerror(err)); \
	retcode=e; \
	goto errexit; \
    } \
} while(0)

    snd_pcm_hw_params_malloc(&hw_params);
    if (! hw_params)
    {
        retcode = MMSYSERR_NOMEM;
        goto errexit;
    }
    snd_pcm_hw_params_any(pcm, hw_params);

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
        retcode = WAVERR_BADFORMAT;
        goto errexit;
    } else if (wwo->format.Format.wFormatTag == WAVE_FORMAT_ALAW) {
        FIXME("unimplemented format: WAVE_FORMAT_ALAW\n");
        retcode = WAVERR_BADFORMAT;
        goto errexit;
    } else if (wwo->format.Format.wFormatTag == WAVE_FORMAT_ADPCM) {
        FIXME("unimplemented format: WAVE_FORMAT_ADPCM\n");
        retcode = WAVERR_BADFORMAT;
        goto errexit;
    } else {
        ERR("invalid format: %0x04x\n", wwo->format.Format.wFormatTag);
        retcode = WAVERR_BADFORMAT;
        goto errexit;
    }

    if ((err = snd_pcm_hw_params_set_format(pcm, hw_params, format)) < 0) {
        WARN("unable to set required format: %s\n", snd_pcm_format_name(format));
        EXIT_ON_ERROR( snd_pcm_hw_params_set_format(pcm, hw_params, format), WAVERR_BADFORMAT, "unable to set required format" );
    }

    rate = wwo->format.Format.nSamplesPerSec;
    dir=0;
    err = snd_pcm_hw_params_set_rate_near(pcm, hw_params, &rate, &dir);
    if (err < 0) {
	WARN("Rate %d Hz not available for playback: %s\n", wwo->format.Format.nSamplesPerSec, snd_strerror(rate));
        retcode = WAVERR_BADFORMAT;
        goto errexit;
    }
    if (!ALSA_NearMatch(rate, wwo->format.Format.nSamplesPerSec)) {
        WARN("Rate doesn't match (requested %d Hz, got %d Hz)\n", wwo->format.Format.nSamplesPerSec, rate);
        retcode = WAVERR_BADFORMAT;
        goto errexit;
    }

    TRACE("Got this format: %dx%dx%d %s\n",
          wwo->format.Format.nSamplesPerSec,
          wwo->format.Format.wBitsPerSample,
          wwo->format.Format.nChannels,
          ALSA_getFormat(wwo->format.Format.wFormatTag));

    dir=0;
    EXIT_ON_ERROR( snd_pcm_hw_params_set_buffer_time_near(pcm, hw_params, &buffer_time, &dir), MMSYSERR_INVALPARAM, "unable to set buffer time");
    dir=0;
    EXIT_ON_ERROR( snd_pcm_hw_params_set_period_time_near(pcm, hw_params, &period_time, &dir), MMSYSERR_INVALPARAM, "unable to set period time");

    EXIT_ON_ERROR( snd_pcm_hw_params(pcm, hw_params), MMSYSERR_INVALPARAM, "unable to set hw params for playback");

    err = snd_pcm_hw_params_get_period_size(hw_params, &period_size, &dir);
    err = snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);

    snd_pcm_sw_params_current(pcm, sw_params);
    EXIT_ON_ERROR( snd_pcm_sw_params_set_start_threshold(pcm, sw_params, 1), MMSYSERR_ERROR, "unable to set start threshold");
    EXIT_ON_ERROR( snd_pcm_sw_params_set_silence_size(pcm, sw_params, 0), MMSYSERR_ERROR, "unable to set silence size");
    EXIT_ON_ERROR( snd_pcm_sw_params_set_avail_min(pcm, sw_params, period_size), MMSYSERR_ERROR, "unable to set avail min");
    EXIT_ON_ERROR( snd_pcm_sw_params_set_xfer_align(pcm, sw_params, 1), MMSYSERR_ERROR, "unable to set xfer align");
    EXIT_ON_ERROR( snd_pcm_sw_params_set_silence_threshold(pcm, sw_params, 0), MMSYSERR_ERROR, "unable to set silence threshold");
    EXIT_ON_ERROR( snd_pcm_sw_params_set_xrun_mode(pcm, sw_params, SND_PCM_XRUN_NONE), MMSYSERR_ERROR, "unable to set xrun mode");
    EXIT_ON_ERROR( snd_pcm_sw_params(pcm, sw_params), MMSYSERR_ERROR, "unable to set sw params for playback");
#undef EXIT_ON_ERROR

    snd_pcm_prepare(pcm);

    if (TRACE_ON(wave))
	ALSA_TraceParameters(hw_params, sw_params, FALSE);

    /* now, we can save all required data for later use... */

    wwo->dwBufferSize = snd_pcm_frames_to_bytes(pcm, buffer_size);
    wwo->lpQueuePtr = wwo->lpPlayPtr = wwo->lpLoopPtr = NULL;
    wwo->dwPlayedTotal = wwo->dwWrittenTotal = 0;
    wwo->dwPartialOffset = 0;

    ALSA_InitRingMessage(&wwo->msgRing);

    wwo->hStartUpEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    wwo->hThread = CreateThread(NULL, 0, wodPlayer, (LPVOID)(DWORD)wDevID, 0, &(wwo->dwThreadID));
    if (wwo->hThread)
        SetThreadPriority(wwo->hThread, THREAD_PRIORITY_TIME_CRITICAL);
    else
    {
        ERR("Thread creation for the wodPlayer failed!\n");
        CloseHandle(wwo->hStartUpEvent);
        retcode = MMSYSERR_NOMEM;
        goto errexit;
    }
    WaitForSingleObject(wwo->hStartUpEvent, INFINITE);
    CloseHandle(wwo->hStartUpEvent);
    wwo->hStartUpEvent = INVALID_HANDLE_VALUE;

    TRACE("handle=%p\n", pcm);
    TRACE("wBitsPerSample=%u, nAvgBytesPerSec=%u, nSamplesPerSec=%u, nChannels=%u nBlockAlign=%u!\n",
	  wwo->format.Format.wBitsPerSample, wwo->format.Format.nAvgBytesPerSec,
	  wwo->format.Format.nSamplesPerSec, wwo->format.Format.nChannels,
	  wwo->format.Format.nBlockAlign);

    wwo->pcm = pcm;
    wwo->hctl = hctl;
    if ( wwo->hw_params )
	snd_pcm_hw_params_free(wwo->hw_params);
    wwo->hw_params = hw_params;

    return wodNotifyClient(wwo, WOM_OPEN, 0L, 0L);

errexit:
    if (pcm)
        snd_pcm_close(pcm);

    if (hctl)
    {
        snd_hctl_free(hctl);
        snd_hctl_close(hctl);
    }

    if ( hw_params )
	snd_pcm_hw_params_free(hw_params);

    if (wwo->msgRing.ring_buffer_size > 0)
        ALSA_DestroyRingMessage(&wwo->msgRing);

    return retcode;
}


/**************************************************************************
 * 				wodClose			[internal]
 */
static DWORD wodClose(WORD wDevID)
{
    DWORD		ret = MMSYSERR_NOERROR;
    WINE_WAVEDEV*	wwo;

    TRACE("(%u);\n", wDevID);

    if (wDevID >= ALSA_WodNumDevs) {
	TRACE("Asked for device %d, but only %d known!\n", wDevID, ALSA_WodNumDevs);
	return MMSYSERR_BADDEVICEID;
    }

    if (WOutDev[wDevID].pcm == NULL) {
	WARN("Requested to close already closed device %d!\n", wDevID);
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

        if (wwo->hw_params)
	    snd_pcm_hw_params_free(wwo->hw_params);
	wwo->hw_params = NULL;

        if (wwo->pcm)
            snd_pcm_close(wwo->pcm);
	wwo->pcm = NULL;

        if (wwo->hctl)
        {
            snd_hctl_free(wwo->hctl);
            snd_hctl_close(wwo->hctl);
        }
	wwo->hctl = NULL;

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

    if (wDevID >= ALSA_WodNumDevs) {
	TRACE("Asked for device %d, but only %d known!\n", wDevID, ALSA_WodNumDevs);
	return MMSYSERR_BADDEVICEID;
    }

    if (WOutDev[wDevID].pcm == NULL) {
	WARN("Requested to write to closed device %d!\n", wDevID);
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

    if (wDevID >= ALSA_WodNumDevs) {
	TRACE("Asked for device %d, but only %d known!\n", wDevID, ALSA_WodNumDevs);
	return MMSYSERR_BADDEVICEID;
    }

    if (WOutDev[wDevID].pcm == NULL) {
	WARN("Requested to pause closed device %d!\n", wDevID);
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

    if (wDevID >= ALSA_WodNumDevs) {
	TRACE("Asked for device %d, but only %d known!\n", wDevID, ALSA_WodNumDevs);
	return MMSYSERR_BADDEVICEID;
    }

    if (WOutDev[wDevID].pcm == NULL) {
	WARN("Requested to restart closed device %d!\n", wDevID);
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

    if (wDevID >= ALSA_WodNumDevs) {
	TRACE("Asked for device %d, but only %d known!\n", wDevID, ALSA_WodNumDevs);
	return MMSYSERR_BADDEVICEID;
    }

    if (WOutDev[wDevID].pcm == NULL) {
	WARN("Requested to reset closed device %d!\n", wDevID);
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

    TRACE("(%u, %p, %u);\n", wDevID, lpTime, uSize);

    if (wDevID >= ALSA_WodNumDevs) {
	TRACE("Asked for device %d, but only %d known!\n", wDevID, ALSA_WodNumDevs);
	return MMSYSERR_BADDEVICEID;
    }

    if (WOutDev[wDevID].pcm == NULL) {
	WARN("Requested to get position of closed device %d!\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }

    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;

    wwo = &WOutDev[wDevID];
    return ALSA_bytes_to_mmtime(lpTime, wwo->dwPlayedTotal, &wwo->format);
}

/**************************************************************************
 * 				wodBreakLoop			[internal]
 */
static DWORD wodBreakLoop(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);

    if (wDevID >= ALSA_WodNumDevs) {
	TRACE("Asked for device %d, but only %d known!\n", wDevID, ALSA_WodNumDevs);
	return MMSYSERR_BADDEVICEID;
    }

    if (WOutDev[wDevID].pcm == NULL) {
	WARN("Requested to breakloop of closed device %d!\n", wDevID);
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
    if (wDevID >= ALSA_WodNumDevs) {
	TRACE("Asked for device %d, but only %d known!\n", wDevID, ALSA_WodNumDevs);
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
        TRACE("CheckSetVolume failed; rc %d\n", rc);

    return rc;
}

/**************************************************************************
 * 				wodSetVolume			[internal]
 */
DWORD wodSetVolume(WORD wDevID, DWORD dwParam)
{
    WORD	       wleft, wright;
    WINE_WAVEDEV*      wwo;
    int                min, max;
    int                left, right;
    DWORD              rc;

    TRACE("(%u, %08X);\n", wDevID, dwParam);
    if (wDevID >= ALSA_WodNumDevs) {
	TRACE("Asked for device %d, but only %d known!\n", wDevID, ALSA_WodNumDevs);
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
            TRACE("SetVolume failed; rc %d\n", rc);
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
    TRACE("(%u, %s, %08X, %08X, %08X);\n",
	  wDevID, ALSA_getMessage(wMsg), dwUser, dwParam1, dwParam2);

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

#else /* HAVE_ALSA */

/**************************************************************************
 * 				wodMessage (WINEALSA.@)
 */
DWORD WINAPI ALSA_wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser,
                             DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08X, %08X, %08X):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

#endif /* HAVE_ALSA */
