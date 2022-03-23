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

/* this is the total number of MIDI out devices found */
static	int 		MIDM_NumDevs = 0;

static	int		numStartedMidiIn = 0;

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

static void notify_client(struct notify_context *notify)
{
    TRACE("dev_id = %d msg = %d param1 = %04lX param2 = %04lX\n", notify->dev_id, notify->msg, notify->param_1, notify->param_2);

    DriverCallback(notify->callback, notify->flags, notify->device, notify->msg,
                   notify->instance, notify->param_1, notify->param_2);
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
    case MIM_OPEN:
    case MIM_CLOSE:
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
                ALSA_CALL(midi_handle_event, ev);
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
        MIDM_NumDevs = params.num_srcs;
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
    struct midi_in_message_params params;
    struct notify_context notify;
    UINT err;

    TRACE("(%04X, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);
    switch (wMsg) {
    case DRVM_INIT:
        ALSA_MidiInit();
        return 0;
    case MIDM_OPEN:
	return midOpen(wDevID, (LPMIDIOPENDESC)dwParam1, dwParam2);
    case MIDM_CLOSE:
	return midClose(wDevID);
    }

    params.dev_id = wDevID;
    params.msg = wMsg;
    params.user = dwUser;
    params.param_1 = dwParam1;
    params.param_2 = dwParam2;
    params.err = &err;
    params.notify = &notify;

    do
    {
        ALSA_CALL(midi_in_message, &params);
        if ((!err || err == ERROR_RETRY) && notify.send_notify) notify_client(&notify);
    } while (err == ERROR_RETRY);

    return err;
}

/**************************************************************************
 * 				modMessage (WINEALSA.@)
 */
DWORD WINAPI ALSA_modMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser,
                             DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    struct midi_out_message_params params;
    struct notify_context notify;
    UINT err;

    TRACE("(%04X, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    switch (wMsg) {
    case DRVM_INIT:
        ALSA_MidiInit();
        return 0;
    }

    params.dev_id = wDevID;
    params.msg = wMsg;
    params.user = dwUser;
    params.param_1 = dwParam1;
    params.param_2 = dwParam2;
    params.err = &err;
    params.notify = &notify;

    ALSA_CALL(midi_out_message, &params);

    if (!err && notify.send_notify) notify_client(&notify);

    return err;
}

static DWORD WINAPI notify_thread(void *p)
{
    struct midi_notify_wait_params params;
    struct notify_context notify;
    BOOL quit;

    params.notify = &notify;
    params.quit = &quit;

    while (1)
    {
        ALSA_CALL(midi_notify_wait, &params);
        if (quit) break;
        if (notify.send_notify) notify_client(&notify);
    }
    return 0;
}

/**************************************************************************
 * 				DriverProc (WINEALSA.@)
 */
LRESULT CALLBACK ALSA_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
                                 LPARAM dwParam1, LPARAM dwParam2)
{
    switch(wMsg) {
    case DRV_LOAD:
        CloseHandle(CreateThread(NULL, 0, notify_thread, NULL, 0, NULL));
        return 1;
    case DRV_FREE:
        ALSA_CALL(midi_release, NULL);
        return 1;
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
