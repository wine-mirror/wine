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
 *                  Low level WAVE IN implementation			*
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

WINE_WAVEDEV	*WInDev;
DWORD            ALSA_WidNumMallocedDevs;
DWORD            ALSA_WidNumDevs;

/**************************************************************************
* 			widNotifyClient			[internal]
*/
static DWORD widNotifyClient(WINE_WAVEDEV* wwi, WORD wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
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
static DWORD widGetDevCaps(WORD wDevID, LPWAVEINCAPSW lpCaps, DWORD dwSize)
{
    TRACE("(%u, %p, %u);\n", wDevID, lpCaps, dwSize);

    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;

    if (wDevID >= ALSA_WidNumDevs) {
	TRACE("Requested device %d, but only %d are known!\n", wDevID, ALSA_WidNumDevs);
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
    DWORD_PTR		tmp_param;
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
    WORD		uDevID = (DWORD_PTR)pmt;
    WINE_WAVEDEV*	wwi = &WInDev[uDevID];
    WAVEHDR*		lpWaveHdr;
    DWORD		dwSleepTime;
    DWORD		bytesRead;
    enum win_wm_message msg;
    DWORD_PTR		param;
    HANDLE		ev;

    wwi->state = WINE_WS_STOPPED;
    InterlockedExchange((LONG*)&wwi->dwTotalRecorded, 0);
    wwi->lpQueuePtr = NULL;

    SetEvent(wwi->hStartUpEvent);

    /* make sleep time to be # of ms to output a period */
    dwSleepTime = (wwi->dwPeriodSize * 1000) / wwi->format.Format.nAvgBytesPerSec;
    TRACE("sleeptime=%d ms, total buffer length=%d ms (%d bytes)\n", dwSleepTime, wwi->dwBufferSize * 1000 / wwi->format.Format.nAvgBytesPerSec, wwi->dwBufferSize);

    for (;;) {
	/* wait for dwSleepTime or an event in thread's queue */
	if (wwi->lpQueuePtr != NULL && wwi->state == WINE_WS_PLAYING)
        {
            DWORD frames;
            DWORD bytes;
            DWORD read;

            lpWaveHdr = wwi->lpQueuePtr;
            /* read all the fragments accumulated so far */
            frames = snd_pcm_avail_update(wwi->pcm);
            bytes = snd_pcm_frames_to_bytes(wwi->pcm, frames);

            TRACE("frames = %d  bytes = %d state=%d\n", frames, bytes, snd_pcm_state(wwi->pcm));
            if (snd_pcm_state(wwi->pcm) == SND_PCM_STATE_XRUN)
            {
                FIXME("Recovering from XRUN!\n");
                snd_pcm_prepare(wwi->pcm);
                frames = snd_pcm_avail_update(wwi->pcm);
                bytes = snd_pcm_frames_to_bytes(wwi->pcm, frames);
                snd_pcm_start(wwi->pcm);
                snd_pcm_forward(wwi->pcm, frames - snd_pcm_bytes_to_frames(wwi->pcm, wwi->dwPeriodSize));
                continue;
            }
            while (frames > 0 && wwi->lpQueuePtr)
            {
                TRACE("bytes = %d\n", bytes);
                if (lpWaveHdr->dwBufferLength - lpWaveHdr->dwBytesRecorded < bytes)
                {
                    bytes = lpWaveHdr->dwBufferLength - lpWaveHdr->dwBytesRecorded;
                    frames = snd_pcm_bytes_to_frames(wwi->pcm, bytes);
                }
                /* directly read fragment in wavehdr */
                read = wwi->read(wwi->pcm, lpWaveHdr->lpData + lpWaveHdr->dwBytesRecorded, frames);
                bytesRead = snd_pcm_frames_to_bytes(wwi->pcm, read);

                TRACE("bytesRead=(%d(%d)/(%d)) -> (%d/%d)\n", bytesRead, read, frames, lpWaveHdr->dwBufferLength, lpWaveHdr->dwBufferLength - lpWaveHdr->dwBytesRecorded);
                if (read != (DWORD) -1)
                {
                    /* update number of bytes recorded in current buffer and by this device */
                    lpWaveHdr->dwBytesRecorded += bytesRead;
                    InterlockedExchangeAdd((LONG*)&wwi->dwTotalRecorded, bytesRead);
                    frames -= read;
                    bytes -= bytesRead;

                    /* buffer is full. notify client */
                    if (!snd_pcm_bytes_to_frames(wwi->pcm, lpWaveHdr->dwBytesRecorded - lpWaveHdr->dwBufferLength))
                    {
                        /* must copy the value of next waveHdr, because we have no idea of what
                         * will be done with the content of lpWaveHdr in callback
                         */
                        LPWAVEHDR	lpNext = lpWaveHdr->lpNext;

                        lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
                        lpWaveHdr->dwFlags |=  WHDR_DONE;

                        wwi->lpQueuePtr = lpNext;
                        widNotifyClient(wwi, WIM_DATA, (DWORD_PTR)lpWaveHdr, 0);
                        lpWaveHdr = lpNext;
                    }
                } else {
                    WARN("read(%s, %p, %d) failed (%d/%s)\n", wwi->pcmname,
                        lpWaveHdr->lpData + lpWaveHdr->dwBytesRecorded,
                        frames, frames, snd_strerror(read));
                }
            }
        }

        ALSA_WaitRingMessage(&wwi->msgRing, dwSleepTime);

	while (ALSA_RetrieveRingMessage(&wwi->msgRing, &msg, &param, &ev))
	{
            TRACE("msg=%s param=0x%lx\n", ALSA_getCmdString(msg), param);
	    switch (msg) {
	    case WINE_WM_PAUSING:
		wwi->state = WINE_WS_PAUSED;
                /*FIXME("Device should stop recording\n");*/
		SetEvent(ev);
		break;
	    case WINE_WM_STARTING:
		wwi->state = WINE_WS_PLAYING;
		snd_pcm_start(wwi->pcm);
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
		    snd_pcm_drain(wwi->pcm);

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
		        widNotifyClient(wwi, WIM_DATA, (DWORD_PTR)lpWaveHdr, 0);
		    }
		}
		wwi->state = WINE_WS_STOPPED;
		SetEvent(ev);
		break;
	    case WINE_WM_RESETTING:
		if (wwi->state != WINE_WS_STOPPED)
		{
		    snd_pcm_drain(wwi->pcm);
		}
		wwi->state = WINE_WS_STOPPED;
		wwi->dwTotalRecorded = 0;

		/* read any headers in queue */
		widRecorder_ReadHeaders(wwi);

		/* return all buffers to the app */
		while (wwi->lpQueuePtr) {
		    lpWaveHdr = wwi->lpQueuePtr;
		    TRACE("reset %p %p\n", lpWaveHdr, lpWaveHdr->lpNext);
                    wwi->lpQueuePtr = lpWaveHdr->lpNext;
		    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
		    lpWaveHdr->dwFlags |= WHDR_DONE;
		    widNotifyClient(wwi, WIM_DATA, (DWORD_PTR)lpWaveHdr, 0);
		}

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
    DWORD                       ret;

    /* JPW TODO - review this code */
    TRACE("(%u, %p, %08X);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN("Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= ALSA_WidNumDevs) {
	TRACE("Requested device %d, but only %d are known!\n", wDevID, ALSA_WidNumDevs);
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

    wwi = &WInDev[wDevID];

    if (wwi->pcm != NULL) {
        WARN("already allocated\n");
        return MMSYSERR_ALLOCATED;
    }

    wwi->pcm = 0;
    flags = SND_PCM_NONBLOCK;

    if ( (err=snd_pcm_open(&pcm, wwi->pcmname, SND_PCM_STREAM_CAPTURE, flags)) < 0 )
    {
        ERR("Error open: %s\n", snd_strerror(err));
	return MMSYSERR_NOTENABLED;
    }

    wwi->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    wwi->waveDesc = *lpDesc;
    ALSA_copyFormat(lpDesc->lpFormat, &wwi->format);

    if (wwi->format.Format.wBitsPerSample == 0) {
	WARN("Resetting zeroed wBitsPerSample\n");
	wwi->format.Format.wBitsPerSample = 8 *
	    (wwi->format.Format.nAvgBytesPerSec /
	     wwi->format.Format.nSamplesPerSec) /
	    wwi->format.Format.nChannels;
    }

    hw_params = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, snd_pcm_hw_params_sizeof() );
    sw_params = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, snd_pcm_sw_params_sizeof() );

    snd_pcm_hw_params_any(pcm, hw_params);

#define EXIT_ON_ERROR(f,e,txt) do \
{ \
    int err; \
    if ( (err = (f) ) < 0) \
    { \
	WARN(txt ": %s\n", snd_strerror(err)); \
	ret = (e); \
        goto error; \
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
                 (wwi->format.Format.wBitsPerSample == 24) ? SND_PCM_FORMAT_S24_3LE :
                 (wwi->format.Format.wBitsPerSample == 32) ? SND_PCM_FORMAT_S32_LE : -1;
    } else if ((wwi->format.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) &&
        IsEqualGUID(&wwi->format.SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)){
        format = (wwi->format.Format.wBitsPerSample == 32) ? SND_PCM_FORMAT_FLOAT_LE : -1;
    } else if (wwi->format.Format.wFormatTag == WAVE_FORMAT_MULAW) {
        FIXME("unimplemented format: WAVE_FORMAT_MULAW\n");
        ret = WAVERR_BADFORMAT;
        goto error;
    } else if (wwi->format.Format.wFormatTag == WAVE_FORMAT_ALAW) {
        FIXME("unimplemented format: WAVE_FORMAT_ALAW\n");
        ret = WAVERR_BADFORMAT;
        goto error;
    } else if (wwi->format.Format.wFormatTag == WAVE_FORMAT_ADPCM) {
        FIXME("unimplemented format: WAVE_FORMAT_ADPCM\n");
        ret = WAVERR_BADFORMAT;
        goto error;
    } else {
        ERR("invalid format: %0x04x\n", wwi->format.Format.wFormatTag);
        ret = WAVERR_BADFORMAT;
        goto error;
    }

    EXIT_ON_ERROR( snd_pcm_hw_params_set_format(pcm, hw_params, format), WAVERR_BADFORMAT, "unable to set required format");

    rate = wwi->format.Format.nSamplesPerSec;
    dir = 0;
    err = snd_pcm_hw_params_set_rate_near(pcm, hw_params, &rate, &dir);
    if (err < 0) {
	WARN("Rate %d Hz not available for playback: %s\n", wwi->format.Format.nSamplesPerSec, snd_strerror(rate));
        ret = WAVERR_BADFORMAT;
        goto error;
    }
    if (!ALSA_NearMatch(rate, wwi->format.Format.nSamplesPerSec)) {
	WARN("Rate doesn't match (requested %d Hz, got %d Hz)\n", wwi->format.Format.nSamplesPerSec, rate);
        ret = WAVERR_BADFORMAT;
        goto error;
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
    EXIT_ON_ERROR( snd_pcm_sw_params_set_start_threshold(pcm, sw_params, 1), MMSYSERR_ERROR, "unable to set start threshold");
    EXIT_ON_ERROR( snd_pcm_sw_params_set_silence_size(pcm, sw_params, 0), MMSYSERR_ERROR, "unable to set silence size");
    EXIT_ON_ERROR( snd_pcm_sw_params_set_avail_min(pcm, sw_params, period_size), MMSYSERR_ERROR, "unable to set avail min");
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
    wwi->pcm = pcm;

    ALSA_InitRingMessage(&wwi->msgRing);

    wwi->dwPeriodSize = snd_pcm_frames_to_bytes(pcm, period_size);
    TRACE("dwPeriodSize=%u\n", wwi->dwPeriodSize);
    TRACE("wBitsPerSample=%u, nAvgBytesPerSec=%u, nSamplesPerSec=%u, nChannels=%u nBlockAlign=%u!\n",
	  wwi->format.Format.wBitsPerSample, wwi->format.Format.nAvgBytesPerSec,
	  wwi->format.Format.nSamplesPerSec, wwi->format.Format.nChannels,
	  wwi->format.Format.nBlockAlign);

    wwi->hStartUpEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    wwi->hThread = CreateThread(NULL, 0, widRecorder, (LPVOID)(DWORD_PTR)wDevID, 0, &(wwi->dwThreadID));
    if (wwi->hThread)
        SetThreadPriority(wwi->hThread, THREAD_PRIORITY_TIME_CRITICAL);
    WaitForSingleObject(wwi->hStartUpEvent, INFINITE);
    CloseHandle(wwi->hStartUpEvent);
    wwi->hStartUpEvent = INVALID_HANDLE_VALUE;

    HeapFree( GetProcessHeap(), 0, hw_params );
    HeapFree( GetProcessHeap(), 0, sw_params );
    return widNotifyClient(wwi, WIM_OPEN, 0L, 0L);

error:
    snd_pcm_close(pcm);
    HeapFree( GetProcessHeap(), 0, hw_params );
    HeapFree( GetProcessHeap(), 0, sw_params );
    return ret;
}


/**************************************************************************
 * 				widClose			[internal]
 */
static DWORD widClose(WORD wDevID)
{
    DWORD		ret = MMSYSERR_NOERROR;
    WINE_WAVEDEV*	wwi;

    TRACE("(%u);\n", wDevID);

    if (wDevID >= ALSA_WidNumDevs) {
	TRACE("Requested device %d, but only %d are known!\n", wDevID, ALSA_WidNumDevs);
	return MMSYSERR_BADDEVICEID;
    }

    if (WInDev[wDevID].pcm == NULL) {
	WARN("Requested to close already closed device %d!\n", wDevID);
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

        snd_pcm_close(wwi->pcm);
	wwi->pcm = NULL;

	ret = widNotifyClient(wwi, WIM_CLOSE, 0L, 0L);
    }

    return ret;
}

/**************************************************************************
 * 				widAddBuffer			[internal]
 *
 */
static DWORD widAddBuffer(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08X);\n", wDevID, lpWaveHdr, dwSize);

    /* first, do the sanity checks... */
    if (wDevID >= ALSA_WidNumDevs) {
	TRACE("Requested device %d, but only %d are known!\n", wDevID, ALSA_WidNumDevs);
	return MMSYSERR_BADDEVICEID;
    }

    if (WInDev[wDevID].pcm == NULL) {
	WARN("Requested to add buffer to already closed device %d!\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }

    if (lpWaveHdr->lpData == NULL || !(lpWaveHdr->dwFlags & WHDR_PREPARED))
	return WAVERR_UNPREPARED;

    if (lpWaveHdr->dwFlags & WHDR_INQUEUE)
	return WAVERR_STILLPLAYING;

    lpWaveHdr->dwFlags &= ~WHDR_DONE;
    lpWaveHdr->dwFlags |= WHDR_INQUEUE;
    lpWaveHdr->dwBytesRecorded = 0;
    lpWaveHdr->lpNext = 0;

    ALSA_AddRingMessage(&WInDev[wDevID].msgRing, WINE_WM_HEADER, (DWORD_PTR)lpWaveHdr, FALSE);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widStart			[internal]
 *
 */
static DWORD widStart(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08X);\n", wDevID, lpWaveHdr, dwSize);

    /* first, do the sanity checks... */
    if (wDevID >= ALSA_WidNumDevs) {
	TRACE("Requested device %d, but only %d are known!\n", wDevID, ALSA_WidNumDevs);
	return MMSYSERR_BADDEVICEID;
    }

    if (WInDev[wDevID].pcm == NULL) {
	WARN("Requested to start closed device %d!\n", wDevID);
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
    TRACE("(%u, %p, %08X);\n", wDevID, lpWaveHdr, dwSize);

    /* first, do the sanity checks... */
    if (wDevID >= ALSA_WidNumDevs) {
	TRACE("Requested device %d, but only %d are known!\n", wDevID, ALSA_WidNumDevs);
	return MMSYSERR_BADDEVICEID;
    }

    if (WInDev[wDevID].pcm == NULL) {
	WARN("Requested to stop closed device %d!\n", wDevID);
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
    if (wDevID >= ALSA_WidNumDevs) {
	TRACE("Requested device %d, but only %d are known!\n", wDevID, ALSA_WidNumDevs);
	return MMSYSERR_BADDEVICEID;
    }

    if (WInDev[wDevID].pcm == NULL) {
	WARN("Requested to reset closed device %d!\n", wDevID);
	return MMSYSERR_BADDEVICEID;
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

    TRACE("(%u, %p, %u);\n", wDevID, lpTime, uSize);

    if (wDevID >= ALSA_WidNumDevs) {
	TRACE("Requested device %d, but only %d are known!\n", wDevID, ALSA_WidNumDevs);
	return MMSYSERR_BADDEVICEID;
    }

    if (WInDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("Requested position of closed device %d!\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }

    if (lpTime == NULL)	{
        WARN("invalid parameter: lpTime = NULL\n");
        return MMSYSERR_INVALPARAM;
    }

    wwi = &WInDev[wDevID];
    return ALSA_bytes_to_mmtime(lpTime, wwi->dwTotalRecorded, &wwi->format);
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

    *dwParam1 = MultiByteToWideChar(CP_UNIXCP, 0, WInDev[wDevID].interface_name, -1,
                                    NULL, 0 ) * sizeof(WCHAR);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 *                              widDevInterface                 [internal]
 */
static DWORD widDevInterface(UINT wDevID, PWCHAR dwParam1, DWORD dwParam2)
{
    if (dwParam2 >= MultiByteToWideChar(CP_UNIXCP, 0, WInDev[wDevID].interface_name, -1,
                                        NULL, 0 ) * sizeof(WCHAR))
    {
        MultiByteToWideChar(CP_UNIXCP, 0, WInDev[wDevID].interface_name, -1,
                            dwParam1, dwParam2 / sizeof(WCHAR));
        return MMSYSERR_NOERROR;
    }
    return MMSYSERR_INVALPARAM;
}

/**************************************************************************
 * 				widMessage (WINEALSA.@)
 */
DWORD WINAPI ALSA_widMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser,
                             DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    TRACE("(%u, %s, %08lX, %08lX, %08lX);\n",
	  wDevID, ALSA_getMessage(wMsg), dwUser, dwParam1, dwParam2);

    switch (wMsg) {
    case DRVM_INIT:
        ALSA_WaveInit();
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
    case WIDM_GETDEVCAPS:	return widGetDevCaps	(wDevID, (LPWAVEINCAPSW)dwParam1,	dwParam2);
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

#else /* HAVE_ALSA */

/**************************************************************************
 * 				widMessage (WINEALSA.@)
 */
DWORD WINAPI ALSA_widMessage(WORD wDevID, WORD wMsg, DWORD_PTR dwUser,
                             DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    FIXME("(%u, %04X, %08lX, %08lX, %08lX):stub\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

#endif /* HAVE_ALSA */
