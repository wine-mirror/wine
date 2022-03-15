/*
 * MIDI driver for ALSA (PE-side)
 *
 * Copyright 1994       Martin Ayotte
 * Copyright 1998       Luiz Otavio L. Zorzella
 * Copyright 1998, 1999 Eric POUECH
 * Copyright 2003       Christian Costa
 * Copyright 2022       Huw Davies
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

#include "config.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winternl.h"
#include "mmddk.h"
#include "mmdeviceapi.h"

#include "wine/debug.h"
#include "wine/unixlib.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(midi);

static WINE_MIDIIN	*MidiInDev;
static WINE_MIDIOUT	*MidiOutDev;

/* this is the total number of MIDI out devices found (synth and port) */
static	int 		MODM_NumDevs = 0;
/* this is the total number of MIDI out devices found */
static	int 		MIDM_NumDevs = 0;

static	int		numStartedMidiIn = 0;

static CRITICAL_SECTION crit_sect;   /* protects all MidiIn buffer queues */
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &crit_sect,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": crit_sect") }
};
static CRITICAL_SECTION crit_sect = { &critsect_debug, -1, 0, 0, 0, 0 };

static int end_thread;
static HANDLE hThread;

static void seq_lock(void)
{
    ALSA_CALL(midi_seq_lock, (void *)(UINT_PTR)1);
}

static void seq_unlock(void)
{
    ALSA_CALL(midi_seq_lock, (void *)(UINT_PTR)0);
}

/*======================================================================*
 *                  Low level MIDI implementation			*
 *======================================================================*/

#if 0 /* Debug Purpose */
static void error_handler(const char* file, int line, const char* function, int err, const char* fmt, ...)
{
    va_list arg;
    if (err == ENOENT)
        return;
    va_start(arg, fmt);
    fprintf(stderr, "ALSA lib %s:%i:(%s) ", file, line, function);
    vfprintf(stderr, fmt, arg);
    if (err)
        fprintf(stderr, ": %s", snd_strerror(err));
    putc('\n', stderr);
    va_end(arg);
}
#endif

/**************************************************************************
 * 			MIDI_NotifyClient			[internal]
 */
static void MIDI_NotifyClient(UINT wDevID, WORD wMsg,
			      DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    DWORD_PTR 		dwCallBack;
    UINT 		uFlags;
    HANDLE		hDev;
    DWORD_PTR 		dwInstance;

    TRACE("wDevID = %04X wMsg = %d dwParm1 = %04lX dwParam2 = %04lX\n",
	  wDevID, wMsg, dwParam1, dwParam2);

    switch (wMsg) {
    case MOM_OPEN:
    case MOM_CLOSE:
    case MOM_DONE:
    case MOM_POSITIONCB:
	if (wDevID > MODM_NumDevs) return;

	dwCallBack = MidiOutDev[wDevID].midiDesc.dwCallback;
	uFlags = MidiOutDev[wDevID].wFlags;
	hDev = MidiOutDev[wDevID].midiDesc.hMidi;
	dwInstance = MidiOutDev[wDevID].midiDesc.dwInstance;
	break;

    case MIM_OPEN:
    case MIM_CLOSE:
    case MIM_DATA:
    case MIM_LONGDATA:
    case MIM_ERROR:
    case MIM_LONGERROR:
    case MIM_MOREDATA:
	if (wDevID > MIDM_NumDevs) return;

	dwCallBack = MidiInDev[wDevID].midiDesc.dwCallback;
	uFlags = MidiInDev[wDevID].wFlags;
	hDev = MidiInDev[wDevID].midiDesc.hMidi;
	dwInstance = MidiInDev[wDevID].midiDesc.dwInstance;
	break;
    default:
	ERR("Unsupported MSW-MIDI message %u\n", wMsg);
	return;
    }

    DriverCallback(dwCallBack, uFlags, hDev, wMsg, dwInstance, dwParam1, dwParam2);
}

/**************************************************************************
 * 			midiOpenSeq				[internal]
 */
static snd_seq_t *midiOpenSeq(int *port_in_ret)
{
    struct midi_seq_open_params params;

    params.port_in = port_in_ret;
    params.close = 0;
    ALSA_CALL(midi_seq_open, &params);

    return params.seq;
}

/**************************************************************************
 * 			midiCloseSeq				[internal]
 */
static int midiCloseSeq(void)
{
    struct midi_seq_open_params params;

    params.port_in = NULL;
    params.close = 1;
    ALSA_CALL(midi_seq_open, &params);

    return 0;
}

static void handle_midi_event(snd_seq_event_t *ev)
{
    WORD wDevID;

    /* Find the target device */
    for (wDevID = 0; wDevID < MIDM_NumDevs; wDevID++)
        if ( (ev->source.client == MidiInDev[wDevID].addr.client) && (ev->source.port == MidiInDev[wDevID].addr.port) )
            break;
    if ((wDevID == MIDM_NumDevs) || (MidiInDev[wDevID].state != 1))
        FIXME("Unexpected event received, type = %x from %d:%d\n", ev->type, ev->source.client, ev->source.port);
    else {
        DWORD dwTime, toSend = 0;
        int value = 0;
        /* FIXME: Should use ev->time instead for better accuracy */
        dwTime = GetTickCount() - MidiInDev[wDevID].startTime;
        TRACE("Event received, type = %x, device = %d\n", ev->type, wDevID);
        switch(ev->type)
        {
        case SND_SEQ_EVENT_NOTEOFF:
            toSend = (ev->data.note.velocity << 16) | (ev->data.note.note << 8) | MIDI_CMD_NOTE_OFF | ev->data.control.channel;
            break;
        case SND_SEQ_EVENT_NOTEON:
            toSend = (ev->data.note.velocity << 16) | (ev->data.note.note << 8) | MIDI_CMD_NOTE_ON | ev->data.control.channel;
            break;
        case SND_SEQ_EVENT_KEYPRESS:
            toSend = (ev->data.note.velocity << 16) | (ev->data.note.note << 8) | MIDI_CMD_NOTE_PRESSURE | ev->data.control.channel;
            break;
        case SND_SEQ_EVENT_CONTROLLER:
            toSend = (ev->data.control.value << 16) | (ev->data.control.param << 8) | MIDI_CMD_CONTROL | ev->data.control.channel;
            break;
        case SND_SEQ_EVENT_PITCHBEND:
            value = ev->data.control.value + 0x2000;
            toSend = (((value >> 7) & 0x7f) << 16) | ((value & 0x7f) << 8) | MIDI_CMD_BENDER | ev->data.control.channel;
            break;
        case SND_SEQ_EVENT_PGMCHANGE:
            toSend = ((ev->data.control.value & 0x7f) << 8) | MIDI_CMD_PGM_CHANGE | ev->data.control.channel;
            break;
        case SND_SEQ_EVENT_CHANPRESS:
            toSend = ((ev->data.control.value & 0x7f) << 8) | MIDI_CMD_CHANNEL_PRESSURE | ev->data.control.channel;
            break;
        case SND_SEQ_EVENT_CLOCK:
            toSend = 0xF8;
            break;
        case SND_SEQ_EVENT_START:
            toSend = 0xFA;
            break;
        case SND_SEQ_EVENT_CONTINUE:
            toSend = 0xFB;
            break;
        case SND_SEQ_EVENT_STOP:
            toSend = 0xFC;
            break;
        case SND_SEQ_EVENT_SONGPOS:
            toSend = (((ev->data.control.value >> 7) & 0x7f) << 16) | ((ev->data.control.value & 0x7f) << 8) | MIDI_CMD_COMMON_SONG_POS;
            break;
        case SND_SEQ_EVENT_SONGSEL:
          toSend = ((ev->data.control.value & 0x7f) << 8) | MIDI_CMD_COMMON_SONG_SELECT;
            break;
        case SND_SEQ_EVENT_RESET:
            toSend = 0xFF;
            break;
        case SND_SEQ_EVENT_QFRAME:
          toSend = ((ev->data.control.value & 0x7f) << 8) | MIDI_CMD_COMMON_MTC_QUARTER;
            break;
        case SND_SEQ_EVENT_SYSEX:
            {
                int pos = 0;
                int len = ev->data.ext.len;
                LPBYTE ptr = ev->data.ext.ptr;
                LPMIDIHDR lpMidiHdr;

                EnterCriticalSection(&crit_sect);
                while (len) {
                    if ((lpMidiHdr = MidiInDev[wDevID].lpQueueHdr) != NULL) {
                        int copylen = min(len, lpMidiHdr->dwBufferLength - lpMidiHdr->dwBytesRecorded);
                        memcpy(lpMidiHdr->lpData + lpMidiHdr->dwBytesRecorded, ptr + pos, copylen);
                        lpMidiHdr->dwBytesRecorded += copylen;
                        len -= copylen;
                        pos += copylen;
                        /* We check if we reach the end of buffer or the end of sysex before notifying
                         * to handle the case where ALSA split the sysex into several events */
                        if ((lpMidiHdr->dwBytesRecorded == lpMidiHdr->dwBufferLength) ||
                            (*(BYTE*)(lpMidiHdr->lpData + lpMidiHdr->dwBytesRecorded - 1) == 0xF7)) {
                            MidiInDev[wDevID].lpQueueHdr = lpMidiHdr->lpNext;
                            lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
                            lpMidiHdr->dwFlags |= MHDR_DONE;
                            MIDI_NotifyClient(wDevID, MIM_LONGDATA, (DWORD_PTR)lpMidiHdr, dwTime);
                        }
                    } else {
                        FIXME("Sysex data received but no buffer to store it!\n");
                        break;
                    }
                }
                LeaveCriticalSection(&crit_sect);
            }
            break;
        case SND_SEQ_EVENT_SENSING:
            /* Noting to do */
            break;
        default:
            FIXME("Unhandled event received, type = %x\n", ev->type);
            break;
        }
        if (toSend != 0) {
            TRACE("Received event %08x from %d:%d\n", toSend, ev->source.client, ev->source.port);
            MIDI_NotifyClient(wDevID, MIM_DATA, toSend, dwTime);
        }
    }
}

static DWORD WINAPI midRecThread(void *arg)
{
    snd_seq_t *midi_seq = arg;
    int npfd;
    struct pollfd *pfd;
    int ret;

    TRACE("Thread startup\n");

    while(!end_thread) {
	TRACE("Thread loop\n");
        seq_lock();
	npfd = snd_seq_poll_descriptors_count(midi_seq, POLLIN);
	pfd = HeapAlloc(GetProcessHeap(), 0, npfd * sizeof(struct pollfd));
	snd_seq_poll_descriptors(midi_seq, pfd, npfd, POLLIN);
        seq_unlock();

	/* Check if an event is present */
	if (poll(pfd, npfd, 250) <= 0) {
	    HeapFree(GetProcessHeap(), 0, pfd);
	    continue;
	}

	/* Note: This definitely does not work.  
	 * while(snd_seq_event_input_pending(midi_seq, 0) > 0) {
	       snd_seq_event_t* ev;
	       snd_seq_event_input(midi_seq, &ev);
	       ....................
	       snd_seq_free_event(ev);
	   }*/

	do {
            snd_seq_event_t *ev;

            seq_lock();
            snd_seq_event_input(midi_seq, &ev);
            seq_unlock();

            if (ev) {
                handle_midi_event(ev);
                snd_seq_free_event(ev);
            }

            seq_lock();
            ret = snd_seq_event_input_pending(midi_seq, 0);
            seq_unlock();
	} while(ret > 0);

	HeapFree(GetProcessHeap(), 0, pfd);
    }
    return 0;
}

/**************************************************************************
 * 				midGetDevCaps			[internal]
 */
static DWORD midGetDevCaps(WORD wDevID, LPMIDIINCAPSW lpCaps, DWORD dwSize)
{
    TRACE("(%04X, %p, %08X);\n", wDevID, lpCaps, dwSize);

    if (wDevID >= MIDM_NumDevs) return MMSYSERR_BADDEVICEID;
    if (lpCaps == NULL)		return MMSYSERR_INVALPARAM;

    memcpy(lpCaps, &MidiInDev[wDevID].caps, min(dwSize, sizeof(*lpCaps)));

    return MMSYSERR_NOERROR;
}


/**************************************************************************
 * 			midOpen					[internal]
 */
static DWORD midOpen(WORD wDevID, LPMIDIOPENDESC lpDesc, DWORD dwFlags)
{
    int ret = 0, port_in;
    snd_seq_t *midi_seq;

    TRACE("(%04X, %p, %08X);\n", wDevID, lpDesc, dwFlags);

    if (lpDesc == NULL) {
	WARN("Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }

    /* FIXME :
     *	how to check that content of lpDesc is correct ?
     */
    if (wDevID >= MIDM_NumDevs) {
	WARN("wDevID too large (%u) !\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }
    if (MidiInDev[wDevID].state == -1) {        
        WARN("device disabled\n");
        return MIDIERR_NODEVICE;
    }
    if (MidiInDev[wDevID].midiDesc.hMidi != 0) {
	WARN("device already open !\n");
	return MMSYSERR_ALLOCATED;
    }
    if ((dwFlags & MIDI_IO_STATUS) != 0) {
	WARN("No support for MIDI_IO_STATUS in dwFlags yet, ignoring it\n");
	dwFlags &= ~MIDI_IO_STATUS;
    }
    if ((dwFlags & ~CALLBACK_TYPEMASK) != 0) {
	FIXME("Bad dwFlags\n");
	return MMSYSERR_INVALFLAG;
    }

    if (!(midi_seq = midiOpenSeq(&port_in))) {
	return MMSYSERR_ERROR;
    }

    MidiInDev[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    MidiInDev[wDevID].lpQueueHdr = NULL;
    MidiInDev[wDevID].midiDesc = *lpDesc;
    MidiInDev[wDevID].state = 0;
    MidiInDev[wDevID].startTime = 0;
    MidiInDev[wDevID].seq = midi_seq;
    MidiInDev[wDevID].port_in = port_in;

    /* Connect our app port to the device port */
    seq_lock();
    ret = snd_seq_connect_from(midi_seq, port_in, MidiInDev[wDevID].addr.client,
                               MidiInDev[wDevID].addr.port);
    seq_unlock();
    if (ret < 0)
	return MMSYSERR_NOTENABLED;

    TRACE("Input port :%d connected %d:%d\n",port_in,MidiInDev[wDevID].addr.client,MidiInDev[wDevID].addr.port);

    if (numStartedMidiIn++ == 0) {
	end_thread = 0;
	hThread = CreateThread(NULL, 0, midRecThread, midi_seq, 0, NULL);
	if (!hThread) {
	    numStartedMidiIn = 0;
	    WARN("Couldn't create thread for midi-in\n");
	    midiCloseSeq();
	    return MMSYSERR_ERROR;
	}
        SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
	TRACE("Created thread for midi-in\n");
    }

    MIDI_NotifyClient(wDevID, MIM_OPEN, 0L, 0L);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			midClose				[internal]
 */
static DWORD midClose(WORD wDevID)
{
    int		ret = MMSYSERR_NOERROR;

    TRACE("(%04X);\n", wDevID);

    if (wDevID >= MIDM_NumDevs) {
	WARN("wDevID too big (%u) !\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }
    if (MidiInDev[wDevID].midiDesc.hMidi == 0) {
	WARN("device not opened !\n");
	return MMSYSERR_ERROR;
    }
    if (MidiInDev[wDevID].lpQueueHdr != 0) {
	return MIDIERR_STILLPLAYING;
    }

    if (MidiInDev[wDevID].seq == NULL) {
	WARN("ooops !\n");
	return MMSYSERR_ERROR;
    }
    if (--numStartedMidiIn == 0) {
	TRACE("Stopping thread for midi-in\n");
	end_thread = 1;
	if (WaitForSingleObject(hThread, 5000) != WAIT_OBJECT_0) {
	    WARN("Thread end not signaled, force termination\n");
	    TerminateThread(hThread, 0);
	}
    	TRACE("Stopped thread for midi-in\n");
    }

    seq_lock();
    snd_seq_disconnect_from(MidiInDev[wDevID].seq, MidiInDev[wDevID].port_in, MidiInDev[wDevID].addr.client, MidiInDev[wDevID].addr.port);
    seq_unlock();
    midiCloseSeq();

    MIDI_NotifyClient(wDevID, MIM_CLOSE, 0L, 0L);
    MidiInDev[wDevID].midiDesc.hMidi = 0;
    MidiInDev[wDevID].seq = NULL;

    return ret;
}


/**************************************************************************
 * 				midAddBuffer			[internal]
 */
static DWORD midAddBuffer(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("(%04X, %p, %d);\n", wDevID, lpMidiHdr, dwSize);

    if (wDevID >= MIDM_NumDevs) return MMSYSERR_BADDEVICEID;
    if (MidiInDev[wDevID].state == -1) return MIDIERR_NODEVICE;

    if (lpMidiHdr == NULL)	return MMSYSERR_INVALPARAM;
    if (dwSize < offsetof(MIDIHDR,dwOffset)) return MMSYSERR_INVALPARAM;
    if (lpMidiHdr->dwBufferLength == 0) return MMSYSERR_INVALPARAM;
    if (lpMidiHdr->dwFlags & MHDR_INQUEUE) return MIDIERR_STILLPLAYING;
    if (!(lpMidiHdr->dwFlags & MHDR_PREPARED)) return MIDIERR_UNPREPARED;

    EnterCriticalSection(&crit_sect);
    lpMidiHdr->dwFlags &= ~WHDR_DONE;
    lpMidiHdr->dwFlags |= MHDR_INQUEUE;
    lpMidiHdr->dwBytesRecorded = 0;
    lpMidiHdr->lpNext = 0;
    if (MidiInDev[wDevID].lpQueueHdr == 0) {
	MidiInDev[wDevID].lpQueueHdr = lpMidiHdr;
    } else {
	LPMIDIHDR	ptr;

        for (ptr = MidiInDev[wDevID].lpQueueHdr; ptr->lpNext != 0;
             ptr = ptr->lpNext);
        ptr->lpNext = lpMidiHdr;
    }
    LeaveCriticalSection(&crit_sect);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midPrepare			[internal]
 */
static DWORD midPrepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("(%04X, %p, %d);\n", wDevID, lpMidiHdr, dwSize);

    if (dwSize < offsetof(MIDIHDR,dwOffset) || lpMidiHdr == 0 || lpMidiHdr->lpData == 0)
	return MMSYSERR_INVALPARAM;
    if (lpMidiHdr->dwFlags & MHDR_PREPARED)
	return MMSYSERR_NOERROR;

    lpMidiHdr->lpNext = 0;
    lpMidiHdr->dwFlags |= MHDR_PREPARED;
    lpMidiHdr->dwFlags &= ~(MHDR_DONE|MHDR_INQUEUE); /* flags cleared since w2k */

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midUnprepare			[internal]
 */
static DWORD midUnprepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("(%04X, %p, %d);\n", wDevID, lpMidiHdr, dwSize);

    if (dwSize < offsetof(MIDIHDR,dwOffset) || lpMidiHdr == 0 || lpMidiHdr->lpData == 0)
	return MMSYSERR_INVALPARAM;
    if (!(lpMidiHdr->dwFlags & MHDR_PREPARED))
	return MMSYSERR_NOERROR;
    if (lpMidiHdr->dwFlags & MHDR_INQUEUE)
	return MIDIERR_STILLPLAYING;

    lpMidiHdr->dwFlags &= ~MHDR_PREPARED;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			midReset				[internal]
 */
static DWORD midReset(WORD wDevID)
{
    DWORD		dwTime = GetTickCount();

    TRACE("(%04X);\n", wDevID);

    if (wDevID >= MIDM_NumDevs) return MMSYSERR_BADDEVICEID;
    if (MidiInDev[wDevID].state == -1) return MIDIERR_NODEVICE;

    EnterCriticalSection(&crit_sect);
    while (MidiInDev[wDevID].lpQueueHdr) {
	LPMIDIHDR lpMidiHdr = MidiInDev[wDevID].lpQueueHdr;
	MidiInDev[wDevID].lpQueueHdr = lpMidiHdr->lpNext;
	lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
	lpMidiHdr->dwFlags |= MHDR_DONE;
	MIDI_NotifyClient(wDevID, MIM_LONGDATA, (DWORD_PTR)lpMidiHdr, dwTime);
    }
    LeaveCriticalSection(&crit_sect);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			midStart				[internal]
 */
static DWORD midStart(WORD wDevID)
{
    TRACE("(%04X);\n", wDevID);

    if (wDevID >= MIDM_NumDevs) return MMSYSERR_BADDEVICEID;
    if (MidiInDev[wDevID].state == -1) return MIDIERR_NODEVICE;

    MidiInDev[wDevID].state = 1;
    MidiInDev[wDevID].startTime = GetTickCount();
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 *			midStop					[internal]
 */
static DWORD midStop(WORD wDevID)
{
    TRACE("(%04X);\n", wDevID);

    if (wDevID >= MIDM_NumDevs) return MMSYSERR_BADDEVICEID;
    if (MidiInDev[wDevID].state == -1) return MIDIERR_NODEVICE;

    MidiInDev[wDevID].state = 0;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				modGetDevCaps			[internal]
 */
static DWORD modGetDevCaps(WORD wDevID, LPMIDIOUTCAPSW lpCaps, DWORD dwSize)
{
    TRACE("(%04X, %p, %08X);\n", wDevID, lpCaps, dwSize);

    if (wDevID >= MODM_NumDevs)	return MMSYSERR_BADDEVICEID;
    if (lpCaps == NULL) 	return MMSYSERR_INVALPARAM;

    memcpy(lpCaps, &MidiOutDev[wDevID].caps, min(dwSize, sizeof(*lpCaps)));

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			modOpen					[internal]
 */
static DWORD modOpen(WORD wDevID, LPMIDIOPENDESC lpDesc, DWORD dwFlags)
{
    int ret;
    int port_out;
    char port_out_name[32];
    snd_seq_t *midi_seq;

    TRACE("(%04X, %p, %08X);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN("Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MODM_NumDevs) {
	TRACE("MAX_MIDIOUTDRV reached !\n");
	return MMSYSERR_BADDEVICEID;
    }
    if (MidiOutDev[wDevID].midiDesc.hMidi != 0) {
	WARN("device already open !\n");
	return MMSYSERR_ALLOCATED;
    }
    if (!MidiOutDev[wDevID].bEnabled) {
	WARN("device disabled !\n");
	return MIDIERR_NODEVICE;
    }
    if ((dwFlags & ~CALLBACK_TYPEMASK) != 0) {
	WARN("bad dwFlags\n");
	return MMSYSERR_INVALFLAG;
    }

    switch (MidiOutDev[wDevID].caps.wTechnology) {
    case MOD_FMSYNTH:
    case MOD_MIDIPORT:
    case MOD_SYNTH:
        if (!(midi_seq = midiOpenSeq(NULL))) {
	    return MMSYSERR_ALLOCATED;
	}
	break;
    default:
	WARN("Technology not supported (yet) %d !\n",
	     MidiOutDev[wDevID].caps.wTechnology);
	return MMSYSERR_NOTENABLED;
    }

    MidiOutDev[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
    MidiOutDev[wDevID].midiDesc = *lpDesc;
    MidiOutDev[wDevID].seq = midi_seq;

    seq_lock();
    /* Create a port dedicated to a specific device */
    /* Keep the old name without a number for the first port */
    if (wDevID)
	sprintf(port_out_name, "WINE ALSA Output #%d", wDevID);

    port_out = snd_seq_create_simple_port(midi_seq, wDevID?port_out_name:"WINE ALSA Output",
	    SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ|SND_SEQ_PORT_CAP_SUBS_WRITE,
	    SND_SEQ_PORT_TYPE_MIDI_GENERIC|SND_SEQ_PORT_TYPE_APPLICATION);

    if (port_out < 0) {
	TRACE("Unable to create output port\n");
	MidiOutDev[wDevID].port_out = -1;
    } else {
	TRACE("Output port %d created successfully\n", port_out);
	MidiOutDev[wDevID].port_out = port_out;

	/* Connect our app port to the device port */
	ret = snd_seq_connect_to(midi_seq, port_out, MidiOutDev[wDevID].addr.client,
	                         MidiOutDev[wDevID].addr.port);

	/* usually will happen when the port is already connected */
	/* other errors should not be fatal either */
	if (ret < 0)
	    WARN("Could not connect port %d to %d:%d: %s\n",
		 wDevID, MidiOutDev[wDevID].addr.client,
		 MidiOutDev[wDevID].addr.port, snd_strerror(ret));
    }
    seq_unlock();

    if (port_out < 0)
	return MMSYSERR_NOTENABLED;
    
    TRACE("Output port :%d connected %d:%d\n",port_out,MidiOutDev[wDevID].addr.client,MidiOutDev[wDevID].addr.port);

    MIDI_NotifyClient(wDevID, MOM_OPEN, 0L, 0L);
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 * 			modClose				[internal]
 */
static DWORD modClose(WORD wDevID)
{
    int	ret = MMSYSERR_NOERROR;

    TRACE("(%04X);\n", wDevID);

    if (MidiOutDev[wDevID].midiDesc.hMidi == 0) {
	WARN("device not opened !\n");
	return MMSYSERR_ERROR;
    }
    /* FIXME: should test that no pending buffer is still in the queue for
     * playing */

    if (MidiOutDev[wDevID].seq == NULL) {
	WARN("can't close !\n");
	return MMSYSERR_ERROR;
    }

    switch (MidiOutDev[wDevID].caps.wTechnology) {
    case MOD_FMSYNTH:
    case MOD_MIDIPORT:
    case MOD_SYNTH:
        seq_lock();
        TRACE("Deleting port :%d, connected to %d:%d\n", MidiOutDev[wDevID].port_out, MidiOutDev[wDevID].addr.client, MidiOutDev[wDevID].addr.port);
        snd_seq_delete_simple_port(MidiOutDev[wDevID].seq, MidiOutDev[wDevID].port_out);
        MidiOutDev[wDevID].port_out = -1;
        seq_unlock();
        midiCloseSeq();
        MidiOutDev[wDevID].seq = NULL;
	break;
    default:
	WARN("Technology not supported (yet) %d !\n",
	     MidiOutDev[wDevID].caps.wTechnology);
	return MMSYSERR_NOTENABLED;
    }

    MIDI_NotifyClient(wDevID, MOM_CLOSE, 0L, 0L);
    MidiOutDev[wDevID].midiDesc.hMidi = 0;
    return ret;
}

/**************************************************************************
 * 			modData					[internal]
 */
static DWORD modData(WORD wDevID, DWORD dwParam)
{
    BYTE	evt = LOBYTE(LOWORD(dwParam));
    BYTE	d1  = HIBYTE(LOWORD(dwParam));
    BYTE	d2  = LOBYTE(HIWORD(dwParam));
    
    TRACE("(%04X, %08X);\n", wDevID, dwParam);

    if (wDevID >= MODM_NumDevs) return MMSYSERR_BADDEVICEID;
    if (!MidiOutDev[wDevID].bEnabled) return MIDIERR_NODEVICE;

    if (MidiOutDev[wDevID].seq == NULL) {
	WARN("can't play !\n");
	return MIDIERR_NODEVICE;
    }
    switch (MidiOutDev[wDevID].caps.wTechnology) {
    case MOD_SYNTH:
    case MOD_MIDIPORT:
	{
	    int handled = 1; /* Assume event is handled */
            snd_seq_event_t event;
            snd_seq_ev_clear(&event);
            snd_seq_ev_set_direct(&event);
            snd_seq_ev_set_source(&event, MidiOutDev[wDevID].port_out);
            snd_seq_ev_set_subs(&event);
	    
	    switch (evt & 0xF0) {
	    case MIDI_CMD_NOTE_OFF:
		snd_seq_ev_set_noteoff(&event, evt&0x0F, d1, d2);
		break;
	    case MIDI_CMD_NOTE_ON:
		snd_seq_ev_set_noteon(&event, evt&0x0F, d1, d2);
		break;
	    case MIDI_CMD_NOTE_PRESSURE:
		snd_seq_ev_set_keypress(&event, evt&0x0F, d1, d2);
		break;
	    case MIDI_CMD_CONTROL:
		snd_seq_ev_set_controller(&event, evt&0x0F, d1, d2);
		break;
	    case MIDI_CMD_BENDER:
		snd_seq_ev_set_pitchbend(&event, evt&0x0F, ((WORD)d2 << 7 | (WORD)d1) - 0x2000);
		break;
	    case MIDI_CMD_PGM_CHANGE:
		snd_seq_ev_set_pgmchange(&event, evt&0x0F, d1);
		break;
	    case MIDI_CMD_CHANNEL_PRESSURE:
		snd_seq_ev_set_chanpress(&event, evt&0x0F, d1);
		break;
	    case MIDI_CMD_COMMON_SYSEX:
		switch (evt & 0x0F) {
		case 0x00:	/* System Exclusive, don't do it on modData,
				 * should require modLongData*/
		case 0x04:	/* Undefined. */
		case 0x05:	/* Undefined. */
		case 0x07:	/* End of Exclusive. */
		case 0x09:	/* Undefined. */
		case 0x0D:	/* Undefined. */
		    handled = 0;
		    break;
		case 0x06:	/* Tune Request */
		case 0x08:	/* Timing Clock. */
		case 0x0A:	/* Start. */
		case 0x0B:	/* Continue */
		case 0x0C:	/* Stop */
		case 0x0E: 	/* Active Sensing. */
		{
		    snd_midi_event_t *midi_event;

		    snd_midi_event_new(1, &midi_event);
		    snd_midi_event_init(midi_event);
		    snd_midi_event_encode_byte(midi_event, evt, &event);
		    snd_midi_event_free(midi_event);
		    break;
		}
		case 0x0F: 	/* Reset */
				/* snd_seq_ev_set_sysex(&event, 1, &evt);
				   this other way may be better */
		    {
			BYTE reset_sysex_seq[] = {MIDI_CMD_COMMON_SYSEX, 0x7e, 0x7f, 0x09, 0x01, 0xf7};
			snd_seq_ev_set_sysex(&event, sizeof(reset_sysex_seq), reset_sysex_seq);
		    }
		    break;
		case 0x01:	/* MTC Quarter frame */
		case 0x03:	/* Song Select. */
		    {
			BYTE buf[2];
			buf[0] = evt;
			buf[1] = d1;
			snd_seq_ev_set_sysex(&event, sizeof(buf), buf);
	            }
	            break;
		case 0x02:	/* Song Position Pointer. */
		    {
			BYTE buf[3];
			buf[0] = evt;
			buf[1] = d1;
			buf[2] = d2;
			snd_seq_ev_set_sysex(&event, sizeof(buf), buf);
	            }
		    break;
		}
		break;
	    }
	    if (handled) {
                seq_lock();
                snd_seq_event_output_direct(MidiOutDev[wDevID].seq, &event);
                seq_unlock();
            }
	}
	break;
    default:
	WARN("Technology not supported (yet) %d !\n",
	     MidiOutDev[wDevID].caps.wTechnology);
	return MMSYSERR_NOTENABLED;
    }

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 *		modLongData					[internal]
 */
static DWORD modLongData(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    int len_add = 0;
    BYTE *lpData, *lpNewData = NULL;
    snd_seq_event_t event;

    TRACE("(%04X, %p, %08X);\n", wDevID, lpMidiHdr, dwSize);

    /* Note: MS doc does not say much about the dwBytesRecorded member of the MIDIHDR structure
     * but it seems to be used only for midi input.
     * Taking a look at the WAVEHDR structure (which is quite similar) confirms this assumption.
     */

    if (wDevID >= MODM_NumDevs) return MMSYSERR_BADDEVICEID;
    if (!MidiOutDev[wDevID].bEnabled) return MIDIERR_NODEVICE;

    if (MidiOutDev[wDevID].seq == NULL) {
	WARN("can't play !\n");
	return MIDIERR_NODEVICE;
    }

    lpData = (BYTE*)lpMidiHdr->lpData;

    if (lpData == NULL)
	return MIDIERR_UNPREPARED;
    if (!(lpMidiHdr->dwFlags & MHDR_PREPARED))
	return MIDIERR_UNPREPARED;
    if (lpMidiHdr->dwFlags & MHDR_INQUEUE)
	return MIDIERR_STILLPLAYING;
    lpMidiHdr->dwFlags &= ~MHDR_DONE;
    lpMidiHdr->dwFlags |= MHDR_INQUEUE;

    /* FIXME: MS doc is not 100% clear. Will lpData only contain system exclusive
     * data, or can it also contain raw MIDI data, to be split up and sent to
     * modShortData() ?
     * If the latest is true, then the following WARNing will fire up
     */
    if (lpData[0] != 0xF0 || lpData[lpMidiHdr->dwBufferLength - 1] != 0xF7) {
	WARN("Alleged system exclusive buffer is not correct\n\tPlease report with MIDI file\n");
	lpNewData = HeapAlloc(GetProcessHeap(), 0, lpMidiHdr->dwBufferLength + 2);
    }

    TRACE("dwBufferLength=%u !\n", lpMidiHdr->dwBufferLength);
    TRACE("                 %02X %02X %02X ... %02X %02X %02X\n",
	  lpData[0], lpData[1], lpData[2], lpData[lpMidiHdr->dwBufferLength-3],
	  lpData[lpMidiHdr->dwBufferLength-2], lpData[lpMidiHdr->dwBufferLength-1]);

    switch (MidiOutDev[wDevID].caps.wTechnology) {
    case MOD_FMSYNTH:
        /* FIXME: I don't think there is much to do here */
        HeapFree(GetProcessHeap(), 0, lpNewData);
        break;
    case MOD_MIDIPORT:
        if (lpData[0] != 0xF0) {
            /* Send start of System Exclusive */
            len_add = 1;
            lpNewData[0] = 0xF0;
            memcpy(lpNewData + 1, lpData, lpMidiHdr->dwBufferLength);
            WARN("Adding missing 0xF0 marker at the beginning of system exclusive byte stream\n");
        }
        if (lpData[lpMidiHdr->dwBufferLength-1] != 0xF7) {
            /* Send end of System Exclusive */
            if (!len_add)
                memcpy(lpNewData, lpData, lpMidiHdr->dwBufferLength);
            lpNewData[lpMidiHdr->dwBufferLength + len_add] = 0xF7;
            len_add++;
            WARN("Adding missing 0xF7 marker at the end of system exclusive byte stream\n");
        }
	snd_seq_ev_clear(&event);
	snd_seq_ev_set_direct(&event);
	snd_seq_ev_set_source(&event, MidiOutDev[wDevID].port_out);
	snd_seq_ev_set_subs(&event);
	snd_seq_ev_set_sysex(&event, lpMidiHdr->dwBufferLength + len_add, lpNewData ? lpNewData : lpData);
        seq_lock();
        snd_seq_event_output_direct(MidiOutDev[wDevID].seq, &event);
        seq_unlock();
        HeapFree(GetProcessHeap(), 0, lpNewData);
        break;
    default:
	WARN("Technology not supported (yet) %d !\n",
	     MidiOutDev[wDevID].caps.wTechnology);
	HeapFree(GetProcessHeap(), 0, lpNewData);
	return MMSYSERR_NOTENABLED;
    }

    lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
    lpMidiHdr->dwFlags |= MHDR_DONE;
    MIDI_NotifyClient(wDevID, MOM_DONE, (DWORD_PTR)lpMidiHdr, 0L);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			modPrepare				[internal]
 */
static DWORD modPrepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("(%04X, %p, %d);\n", wDevID, lpMidiHdr, dwSize);

    if (dwSize < offsetof(MIDIHDR,dwOffset) || lpMidiHdr == 0 || lpMidiHdr->lpData == 0)
	return MMSYSERR_INVALPARAM;
    if (lpMidiHdr->dwFlags & MHDR_PREPARED)
	return MMSYSERR_NOERROR;

    lpMidiHdr->lpNext = 0;
    lpMidiHdr->dwFlags |= MHDR_PREPARED;
    lpMidiHdr->dwFlags &= ~(MHDR_DONE|MHDR_INQUEUE); /* flags cleared since w2k */
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				modUnprepare			[internal]
 */
static DWORD modUnprepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("(%04X, %p, %d);\n", wDevID, lpMidiHdr, dwSize);

    if (dwSize < offsetof(MIDIHDR,dwOffset) || lpMidiHdr == 0 || lpMidiHdr->lpData == 0)
	return MMSYSERR_INVALPARAM;
    if (!(lpMidiHdr->dwFlags & MHDR_PREPARED))
	return MMSYSERR_NOERROR;
    if (lpMidiHdr->dwFlags & MHDR_INQUEUE)
	return MIDIERR_STILLPLAYING;
    lpMidiHdr->dwFlags &= ~MHDR_PREPARED;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			modGetVolume				[internal]
 */
static DWORD modGetVolume(WORD wDevID, DWORD* lpdwVolume)
{
    if (!lpdwVolume) return MMSYSERR_INVALPARAM;
    if (wDevID >= MODM_NumDevs) return MMSYSERR_BADDEVICEID;
    *lpdwVolume = 0xFFFFFFFF;
    return (MidiOutDev[wDevID].caps.dwSupport & MIDICAPS_VOLUME) ? 0 : MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 			modReset				[internal]
 */
static DWORD modReset(WORD wDevID)
{
    unsigned chn;

    TRACE("(%04X);\n", wDevID);

    if (wDevID >= MODM_NumDevs) return MMSYSERR_BADDEVICEID;
    if (!MidiOutDev[wDevID].bEnabled) return MIDIERR_NODEVICE;

    /* stop all notes */
    /* FIXME: check if 0x78B0 is channel dependent or not. I coded it so that
     * it's channel dependent...
     */
    for (chn = 0; chn < 16; chn++) {
	/* turn off every note */
	modData(wDevID, 0x7800 | MIDI_CMD_CONTROL | chn);
	/* remove sustain on all channels */
	modData(wDevID, (MIDI_CTL_SUSTAIN << 8) | MIDI_CMD_CONTROL | chn);
    }
    /* FIXME: the LongData buffers must also be returned to the app */
    return MMSYSERR_NOERROR;
}


/*======================================================================*
 *                  	    MIDI entry points 				*
 *======================================================================*/

/**************************************************************************
 * ALSA_MidiInit				[internal]
 *
 * Initializes the MIDI devices information variables
 */
static BOOL ALSA_MidiInit(void)
{
    struct midi_init_params params;
    UINT err;

    params.err = &err;
    ALSA_CALL(midi_init, &params);

    if (!err)
    {
        MODM_NumDevs = params.num_dests;
        MIDM_NumDevs = params.num_srcs;
        MidiOutDev = params.dests;
        MidiInDev = params.srcs;
    }
    return TRUE;
}

/**************************************************************************
 * 			midMessage (WINEALSA.@)
 */
DWORD WINAPI ALSA_midMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser,
			    DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    TRACE("(%04X, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);
    switch (wMsg) {
    case DRVM_INIT:
        ALSA_MidiInit();
        return 0;
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
	/* FIXME: Pretend this is supported */
	return 0;
    case MIDM_OPEN:
	return midOpen(wDevID, (LPMIDIOPENDESC)dwParam1, dwParam2);
    case MIDM_CLOSE:
	return midClose(wDevID);
    case MIDM_ADDBUFFER:
	return midAddBuffer(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
    case MIDM_PREPARE:
	return midPrepare(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
    case MIDM_UNPREPARE:
	return midUnprepare(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
    case MIDM_GETDEVCAPS:
	return midGetDevCaps(wDevID, (LPMIDIINCAPSW)dwParam1,dwParam2);
    case MIDM_GETNUMDEVS:
	return MIDM_NumDevs;
    case MIDM_RESET:
	return midReset(wDevID);
    case MIDM_START:
	return midStart(wDevID);
    case MIDM_STOP:
	return midStop(wDevID);
    default:
	TRACE("Unsupported message\n");
    }
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				modMessage (WINEALSA.@)
 */
DWORD WINAPI ALSA_modMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser,
                             DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    TRACE("(%04X, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    switch (wMsg) {
    case DRVM_INIT:
        ALSA_MidiInit();
        return 0;
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
	/* FIXME: Pretend this is supported */
	return 0;
    case MODM_OPEN:
	return modOpen(wDevID, (LPMIDIOPENDESC)dwParam1, dwParam2);
    case MODM_CLOSE:
	return modClose(wDevID);
    case MODM_DATA:
	return modData(wDevID, dwParam1);
    case MODM_LONGDATA:
	return modLongData(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
    case MODM_PREPARE:
	return modPrepare(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
    case MODM_UNPREPARE:
	return modUnprepare(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
    case MODM_GETDEVCAPS:
	return modGetDevCaps(wDevID, (LPMIDIOUTCAPSW)dwParam1, dwParam2);
    case MODM_GETNUMDEVS:
	return MODM_NumDevs;
    case MODM_GETVOLUME:
	return modGetVolume(wDevID, (DWORD*)dwParam1);
    case MODM_SETVOLUME:
	return 0;
    case MODM_RESET:
	return modReset(wDevID);
    default:
	TRACE("Unsupported message\n");
    }
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				DriverProc (WINEALSA.@)
 */
LRESULT CALLBACK ALSA_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
                                 LPARAM dwParam1, LPARAM dwParam2)
{
/* EPP     TRACE("(%08lX, %04X, %08lX, %08lX, %08lX)\n",  */
/* EPP 	  dwDevID, hDriv, wMsg, dwParam1, dwParam2); */

    switch(wMsg) {
    case DRV_LOAD:
    case DRV_FREE:
    case DRV_OPEN:
    case DRV_CLOSE:
    case DRV_ENABLE:
    case DRV_DISABLE:
    case DRV_QUERYCONFIGURE:
    case DRV_CONFIGURE:
        return 1;
    case DRV_INSTALL:
    case DRV_REMOVE:
        return DRV_SUCCESS;
    default:
	return 0;
    }
}
