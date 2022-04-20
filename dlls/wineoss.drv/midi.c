/*
 * MIDI driver for OSS (PE-side)
 *
 * Copyright 1994       Martin Ayotte
 * Copyright 1998       Luiz Otavio L. Zorzella (init procedures)
 * Copyright 1998, 1999 Eric POUECH
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

/* TODO:
 *    + use better instrument definition for OPL/2 (midiPatch.c) or
 *      use existing instrument definition (from playmidi or kmid)
 *      with a .winerc option 
 *    + have a look at OPL/3 ?
 *    + implement asynchronous playback of MidiHdr
 *    + implement STREAM'ed MidiHdr (question: how shall we share the
 *      code between the midiStream functions in MMSYSTEM/WINMM and
 *      the code for the low level driver)
 *    + use a more accurate read mechanism than the one of snooping on
 *      timers (like select on fd)
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <sys/soundcard.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "mmddk.h"
#include "audioclient.h"

#include "wine/debug.h"
#include "wine/unixlib.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(midi);

static WINE_MIDIIN *MidiInDev;
static WINE_MIDIOUT *MidiOutDev;

/* this is the total number of MIDI out devices found (synth and port) */
static	int 		MODM_NumDevs = 0;
/* this is the number of FM synthesizers (index from 0 to NUMFMSYNTHDEVS - 1) */
static	int		MODM_NumFMSynthDevs = 0;
/* the Midi ports have index from NUMFMSYNTHDEVS to NumDevs - 1 */

/* this is the total number of MIDI out devices found */
static	int 		MIDM_NumDevs = 0;

static	int		numStartedMidiIn = 0;

static CRITICAL_SECTION crit_sect;   /* protects all MidiIn buffers queues */
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &crit_sect,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": crit_sect") }
};
static CRITICAL_SECTION crit_sect = { &critsect_debug, -1, 0, 0, 0, 0 };

static int end_thread;
static HANDLE hThread;

/*======================================================================*
 *                  Low level MIDI implementation			*
 *======================================================================*/

static int midiOpenSeq(void);
static int midiCloseSeq(int);

static int MIDI_loadcount;
/**************************************************************************
 * 			OSS_MidiInit				[internal]
 *
 * Initializes the MIDI devices information variables
 */
static LRESULT OSS_MidiInit(void)
{
    struct midi_init_params params;
    UINT err;

    TRACE("(%i)\n", MIDI_loadcount);
    if (MIDI_loadcount++)
        return 1;

    TRACE("Initializing the MIDI variables.\n");

    params.err = &err;
    OSS_CALL(midi_init, &params);

    if (!err)
    {
        MidiInDev = params.srcs;
        MidiOutDev = params.dests;
        MODM_NumDevs = params.num_dests;
        MODM_NumFMSynthDevs = params.num_synths;
        MIDM_NumDevs = params.num_srcs;
    }
    return err;
}

/**************************************************************************
 * 			OSS_MidiExit				[internal]
 *
 * Release the MIDI devices information variables
 */
static LRESULT OSS_MidiExit(void)
{
    TRACE("(%i)\n", MIDI_loadcount);

    if (--MIDI_loadcount)
        return 1;

    MidiInDev = NULL;
    MidiOutDev = NULL;

    MODM_NumDevs = 0;
    MODM_NumFMSynthDevs = 0;
    MIDM_NumDevs = 0;

    return 0;
}

static void notify_client(struct notify_context *notify)
{
    TRACE("dev_id = %d msg = %d param1 = %04lX param2 = %04lX\n",
          notify->dev_id, notify->msg, notify->param_1, notify->param_2);

    DriverCallback(notify->callback, notify->flags, notify->device, notify->msg,
                   notify->instance, notify->param_1, notify->param_2);
}

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
static int midiOpenSeq(void)
{
    struct midi_seq_open_params params;

    params.close = 0;
    params.fd = -1;
    OSS_CALL(midi_seq_open, &params);

    return params.fd;
}

/**************************************************************************
 * 			midiCloseSeq				[internal]
 */
static int midiCloseSeq(int fd)
{
    struct midi_seq_open_params params;

    params.close = 1;
    params.fd = fd;
    OSS_CALL(midi_seq_open, &params);

    return 0;
}

/* FIXME: this is a bad idea, it's even not static... */
SEQ_DEFINEBUF(1024);

/* FIXME: this is not reentrant, not static - because of global variable
 * _seqbuf and al.
 */
/**************************************************************************
 * 			seqbuf_dump				[internal]
 *
 * Used by SEQ_DUMPBUF to flush the buffer.
 *
 */
void seqbuf_dump(void)
{
    int fd;

    /* The device is already open, but there's no way to pass the
       fd to this function.  Rather than rely on a global variable
       we pretend to open the seq again. */
    fd = midiOpenSeq();
    if (_seqbufptr) {
	if (write(fd, _seqbuf, _seqbufptr) == -1) {
	    WARN("Can't write data to sequencer %d, errno %d (%s)!\n",
		fd, errno, strerror(errno));
	}
	/* FIXME:
	 *	in any case buffer is lost so that if many errors occur the buffer
	 * will not overrun
	 */
	_seqbufptr = 0;
    }
    midiCloseSeq(fd);
}

/**************************************************************************
 * 			midReceiveChar				[internal]
 */
static void midReceiveChar(WORD wDevID, unsigned char value, DWORD dwTime)
{
    DWORD		toSend = 0;

    TRACE("Adding %02xh to %d[%d]\n", value, wDevID, MidiInDev[wDevID].incLen);

    if (wDevID >= MIDM_NumDevs) {
	WARN("bad devID\n");
	return;
    }
    if (MidiInDev[wDevID].state <= 0) {
	TRACE("disabled or input not started, thrown away\n");
	return;
    }

    if (MidiInDev[wDevID].state & 2) { /* system exclusive */
	LPMIDIHDR	lpMidiHdr;
        BOOL            sbfb = FALSE;

	EnterCriticalSection(&crit_sect);
	if ((lpMidiHdr = MidiInDev[wDevID].lpQueueHdr) != NULL) {
	    LPBYTE	lpData = (LPBYTE) lpMidiHdr->lpData;

	    lpData[lpMidiHdr->dwBytesRecorded++] = value;
	    if (lpMidiHdr->dwBytesRecorded == lpMidiHdr->dwBufferLength) {
		sbfb = TRUE;
	    }
	}
	if (value == 0xF7) { /* then end */
	    MidiInDev[wDevID].state &= ~2;
	    sbfb = TRUE;
	}
	if (sbfb && lpMidiHdr != NULL) {
	    lpMidiHdr = MidiInDev[wDevID].lpQueueHdr;
	    MidiInDev[wDevID].lpQueueHdr = lpMidiHdr->lpNext;
	    lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
	    lpMidiHdr->dwFlags |= MHDR_DONE;
	    MIDI_NotifyClient(wDevID, MIM_LONGDATA, (DWORD_PTR)lpMidiHdr, dwTime);
	}
	LeaveCriticalSection(&crit_sect);
	return;
    }

#define IS_CMD(_x)	(((_x) & 0x80) == 0x80)
#define IS_SYS_CMD(_x)	(((_x) & 0xF0) == 0xF0)

    if (!IS_CMD(value) && MidiInDev[wDevID].incLen == 0) { /* try to reuse old cmd */
	if (IS_CMD(MidiInDev[wDevID].incPrev) && !IS_SYS_CMD(MidiInDev[wDevID].incPrev)) {
	    MidiInDev[wDevID].incoming[0] = MidiInDev[wDevID].incPrev;
	    MidiInDev[wDevID].incLen = 1;
	    TRACE("Reusing old command %02xh\n", MidiInDev[wDevID].incPrev);
	} else {
	    FIXME("error for midi-in, should generate MIM_ERROR notification:"
		  " prev=%02Xh, incLen=%02Xh\n",
		  MidiInDev[wDevID].incPrev, MidiInDev[wDevID].incLen);
	    return;
	}
    }
    MidiInDev[wDevID].incoming[(int)(MidiInDev[wDevID].incLen++)] = value;
    if (MidiInDev[wDevID].incLen == 1 && !IS_SYS_CMD(MidiInDev[wDevID].incoming[0])) {
	/* store new cmd, just in case */
	MidiInDev[wDevID].incPrev = MidiInDev[wDevID].incoming[0];
    }

#undef IS_CMD
#undef IS_SYS_CMD

    switch (MidiInDev[wDevID].incoming[0] & 0xF0) {
    case MIDI_NOTEOFF:
    case MIDI_NOTEON:
    case MIDI_KEY_PRESSURE:
    case MIDI_CTL_CHANGE:
    case MIDI_PITCH_BEND:
	if (MidiInDev[wDevID].incLen == 3) {
	    toSend = (MidiInDev[wDevID].incoming[2] << 16) |
		(MidiInDev[wDevID].incoming[1] <<  8) |
		(MidiInDev[wDevID].incoming[0] <<  0);
	}
	break;
    case MIDI_PGM_CHANGE:
    case MIDI_CHN_PRESSURE:
	if (MidiInDev[wDevID].incLen == 2) {
	    toSend = (MidiInDev[wDevID].incoming[1] <<  8) |
		(MidiInDev[wDevID].incoming[0] <<  0);
	}
	break;
    case MIDI_SYSTEM_PREFIX:
	if (MidiInDev[wDevID].incoming[0] == 0xF0) {
	    MidiInDev[wDevID].state |= 2;
	    MidiInDev[wDevID].incLen = 0;
	} else {
	    if (MidiInDev[wDevID].incLen == 1) {
		toSend = (MidiInDev[wDevID].incoming[0] <<  0);
	    }
	}
	break;
    default:
	WARN("This shouldn't happen (%02X)\n", MidiInDev[wDevID].incoming[0]);
    }
    if (toSend != 0) {
	TRACE("Sending event %08x\n", toSend);
	MidiInDev[wDevID].incLen =	0;
	dwTime -= MidiInDev[wDevID].startTime;
	MIDI_NotifyClient(wDevID, MIM_DATA, toSend, dwTime);
    }
}

static DWORD WINAPI midRecThread(void *arg)
{
    int fd = (int)(INT_PTR)arg;
    unsigned char buffer[256];
    int len, idx;
    DWORD dwTime;
    struct pollfd pfd;

    TRACE("Thread startup\n");

    pfd.fd = fd;
    pfd.events = POLLIN;
    
    while(!end_thread) {
	TRACE("Thread loop\n");

	/* Check if an event is present */
	if (poll(&pfd, 1, 250) <= 0)
	    continue;
	
	len = read(fd, buffer, sizeof(buffer));
	TRACE("Received %d bytes\n", len);

	if (len < 0) continue;
	if ((len % 4) != 0) {
	    WARN("Bad length %d, errno %d (%s)\n", len, errno, strerror(errno));
	    continue;
	}

	dwTime = GetTickCount();
	
	for (idx = 0; idx < len; ) {
	    if (buffer[idx] & 0x80) {
		TRACE(
		      "Reading<8> %02x %02x %02x %02x %02x %02x %02x %02x\n",
		      buffer[idx + 0], buffer[idx + 1],
		      buffer[idx + 2], buffer[idx + 3],
		      buffer[idx + 4], buffer[idx + 5],
		      buffer[idx + 6], buffer[idx + 7]);
		      idx += 8;
	    } else {
		switch (buffer[idx + 0]) {
		case SEQ_WAIT:
		case SEQ_ECHO:
		    break;
		case SEQ_MIDIPUTC:
		    midReceiveChar(buffer[idx + 2], buffer[idx + 1], dwTime);
		    break;
		default:
		    TRACE("Unsupported event %d\n", buffer[idx + 0]);
		    break;
		}
		idx += 4;
	    }
	}
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
    int fd;

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

    fd = midiOpenSeq();
    if (fd < 0) {
	return MMSYSERR_ERROR;
    }

    if (numStartedMidiIn++ == 0) {
	end_thread = 0;
	hThread = CreateThread(NULL, 0, midRecThread, (void *)(INT_PTR)fd, 0, NULL);
	if (!hThread) {
	    numStartedMidiIn = 0;
	    WARN("Couldn't create thread for midi-in\n");
	    midiCloseSeq(fd);
	    return MMSYSERR_ERROR;
	}
        SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
	TRACE("Created thread for midi-in\n");
    }

    MidiInDev[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    MidiInDev[wDevID].lpQueueHdr = NULL;
    MidiInDev[wDevID].midiDesc = *lpDesc;
    MidiInDev[wDevID].state = 0;
    MidiInDev[wDevID].incLen = 0;
    MidiInDev[wDevID].startTime = 0;
    MidiInDev[wDevID].fd = fd;

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

    if (MidiInDev[wDevID].fd == -1) {
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
    midiCloseSeq(MidiInDev[wDevID].fd);
    MidiInDev[wDevID].fd = -1;

    MIDI_NotifyClient(wDevID, MIM_CLOSE, 0L, 0L);
    MidiInDev[wDevID].midiDesc.hMidi = 0;
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

	for (ptr = MidiInDev[wDevID].lpQueueHdr;
	     ptr->lpNext != 0;
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

/*-----------------------------------------------------------------------*/

typedef struct sVoice {
    int			note;			/* 0 means not used */
    int			channel;
    unsigned		cntMark : 30,
	                status : 2;
#define sVS_UNUSED	0
#define sVS_PLAYING	1
#define sVS_SUSTAINED	2
} sVoice;

typedef struct sChannel {
    int			program;

    int			bender;
    int			benderRange;
    /* controllers */
    int			bank;		/* CTL_BANK_SELECT */
    int			volume;		/* CTL_MAIN_VOLUME */
    int			balance;	/* CTL_BALANCE     */
    int			expression;	/* CTL_EXPRESSION  */
    int			sustain;	/* CTL_SUSTAIN     */

    unsigned char	nrgPmtMSB;	/* Non register Parameters */
    unsigned char	nrgPmtLSB;
    unsigned char	regPmtMSB;	/* Non register Parameters */
    unsigned char	regPmtLSB;
} sChannel;

typedef struct sFMextra {
    unsigned		counter;
    int			drumSetMask;
    sChannel		channel[16];	/* MIDI has only 16 channels */
    sVoice		voice[1];	/* dyn allocated according to sound card */
    /* do not append fields below voice[1] since the size of this structure
     * depends on the number of available voices on the FM synth...
     */
} sFMextra;

#define		IS_DRUM_CHANNEL(_xtra, _chn)	((_xtra)->drumSetMask & (1 << (_chn)))

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
 * 			modData					[internal]
 */
static DWORD modData(WORD wDevID, DWORD dwParam)
{
    WORD	evt = LOBYTE(LOWORD(dwParam));
    WORD	d1  = HIBYTE(LOWORD(dwParam));
    WORD	d2  = LOBYTE(HIWORD(dwParam));

    TRACE("(%04X, %08X);\n", wDevID, dwParam);

    if (wDevID >= MODM_NumDevs) return MMSYSERR_BADDEVICEID;
    if (!MidiOutDev[wDevID].bEnabled) return MIDIERR_NODEVICE;

    if (MidiOutDev[wDevID].fd == -1) {
	WARN("can't play !\n");
	return MIDIERR_NODEVICE;
    }
    switch (MidiOutDev[wDevID].caps.wTechnology) {
    case MOD_FMSYNTH:
	/* FIXME:
	 *	- chorus depth controller is not used
	 */
	{
            sFMextra*   extra   = MidiOutDev[wDevID].lpExtra;
	    sVoice* 	voice   = extra->voice;
	    sChannel*	channel = extra->channel;
	    int		chn = (evt & 0x0F);
	    int		i, nv;

	    switch (evt & 0xF0) {
	    case MIDI_NOTEOFF:
		for (i = 0; i < MidiOutDev[wDevID].caps.wVoices; i++) {
				/* don't stop sustained notes */
		    if (voice[i].status == sVS_PLAYING && voice[i].channel == chn && voice[i].note == d1) {
			voice[i].status = sVS_UNUSED;
			SEQ_STOP_NOTE(wDevID, i, d1, d2);
		    }
		}
		break;
	    case MIDI_NOTEON:
		if (d2 == 0) { /* note off if velocity == 0 */
		    for (i = 0; i < MidiOutDev[wDevID].caps.wVoices; i++) {
			/* don't stop sustained notes */
			if (voice[i].status == sVS_PLAYING && voice[i].channel == chn && voice[i].note == d1) {
			    voice[i].status = sVS_UNUSED;
			    SEQ_STOP_NOTE(wDevID, i, d1, 64);
			}
		    }
		    break;
		}
		/* finding out in this order :
		 *	- an empty voice
		 *	- if replaying the same note on the same channel
		 *	- the older voice (LRU)
		 */
		for (i = nv = 0; i < MidiOutDev[wDevID].caps.wVoices; i++) {
		    if (voice[i].status == sVS_UNUSED ||
			(voice[i].note == d1 && voice[i].channel == chn)) {
			nv = i;
			break;
		    }
		    if (voice[i].cntMark < voice[0].cntMark) {
			nv = i;
		    }
		}
		TRACE(
		      "playing on voice=%d, pgm=%d, pan=0x%02X, vol=0x%02X, "
		      "bender=0x%02X, note=0x%02X, vel=0x%02X\n",
		      nv, channel[chn].program,
		      channel[chn].balance,
		      channel[chn].volume,
		      channel[chn].bender, d1, d2);

		SEQ_SET_PATCH(wDevID, nv, IS_DRUM_CHANNEL(extra, chn) ?
			      (128 + d1) : channel[chn].program);
		SEQ_BENDER_RANGE(wDevID, nv, channel[chn].benderRange * 100);
		SEQ_BENDER(wDevID, nv, channel[chn].bender);
		SEQ_CONTROL(wDevID, nv, CTL_PAN, channel[chn].balance);
		SEQ_CONTROL(wDevID, nv, CTL_EXPRESSION, channel[chn].expression);
#if 0
		/* FIXME: does not really seem to work on my SB card and
		 * screws everything up... so lay it down
		 */
		SEQ_CONTROL(wDevID, nv, CTL_MAIN_VOLUME, channel[chn].volume);
#endif
		SEQ_START_NOTE(wDevID, nv, d1, d2);
		voice[nv].status = channel[chn].sustain ? sVS_SUSTAINED : sVS_PLAYING;
		voice[nv].note = d1;
		voice[nv].channel = chn;
		voice[nv].cntMark = extra->counter++;
		break;
	    case MIDI_KEY_PRESSURE:
		for (i = 0; i < MidiOutDev[wDevID].caps.wVoices; i++) {
		    if (voice[i].status != sVS_UNUSED && voice[i].channel == chn && voice[i].note == d1) {
			SEQ_KEY_PRESSURE(wDevID, i, d1, d2);
		    }
		}
		break;
	    case MIDI_CTL_CHANGE:
		switch (d1) {
		case CTL_BANK_SELECT:	channel[chn].bank = d2;		break;
		case CTL_MAIN_VOLUME:	channel[chn].volume = d2;	break;
		case CTL_PAN:		channel[chn].balance = d2;	break;
		case CTL_EXPRESSION:	channel[chn].expression = d2;	break;
		case CTL_SUSTAIN:	channel[chn].sustain = d2;
		    if (d2) {
			for (i = 0; i < MidiOutDev[wDevID].caps.wVoices; i++) {
			    if (voice[i].status == sVS_PLAYING && voice[i].channel == chn) {
				voice[i].status = sVS_SUSTAINED;
			    }
			}
		    } else {
			for (i = 0; i < MidiOutDev[wDevID].caps.wVoices; i++) {
			    if (voice[i].status == sVS_SUSTAINED && voice[i].channel == chn) {
				voice[i].status = sVS_UNUSED;
				SEQ_STOP_NOTE(wDevID, i, voice[i].note, 64);
			    }
			}
		    }
		    break;
		case CTL_NONREG_PARM_NUM_LSB:	channel[chn].nrgPmtLSB = d2;	break;
		case CTL_NONREG_PARM_NUM_MSB:	channel[chn].nrgPmtMSB = d2;	break;
		case CTL_REGIST_PARM_NUM_LSB:	channel[chn].regPmtLSB = d2;	break;
		case CTL_REGIST_PARM_NUM_MSB:	channel[chn].regPmtMSB = d2;	break;
		case CTL_DATA_ENTRY:
		    switch ((channel[chn].regPmtMSB << 8) | channel[chn].regPmtLSB) {
		    case 0x0000:
			if (channel[chn].benderRange != d2) {
			    channel[chn].benderRange = d2;
			    for (i = 0; i < MidiOutDev[wDevID].caps.wVoices; i++) {
				if (voice[i].channel == chn) {
				    SEQ_BENDER_RANGE(wDevID, i, channel[chn].benderRange);
				}
			    }
			}
			break;

		    case 0x7F7F:
			channel[chn].benderRange = 2;
			for (i = 0; i < MidiOutDev[wDevID].caps.wVoices; i++) {
			    if (voice[i].channel == chn) {
				SEQ_BENDER_RANGE(wDevID, i, channel[chn].benderRange);
			    }
			}
			break;
		    default:
			TRACE("Data entry: regPmt=0x%02x%02x, nrgPmt=0x%02x%02x with %x\n",
			      channel[chn].regPmtMSB, channel[chn].regPmtLSB,
			      channel[chn].nrgPmtMSB, channel[chn].nrgPmtLSB,
			      d2);
			break;
		    }
		    break;

		case 0x78: /* all sounds off */
		    /* FIXME: I don't know if I have to take care of the channel
		     * for this control ?
		     */
		    for (i = 0; i < MidiOutDev[wDevID].caps.wVoices; i++) {
			if (voice[i].status != sVS_UNUSED && voice[i].channel == chn) {
			    voice[i].status = sVS_UNUSED;
			    SEQ_STOP_NOTE(wDevID, i, voice[i].note, 64);
			}
		    }
		    break;
		case 0x7B: /* all notes off */
		    /* FIXME: I don't know if I have to take care of the channel
		     * for this control ?
		     */
		    for (i = 0; i < MidiOutDev[wDevID].caps.wVoices; i++) {
			if (voice[i].status == sVS_PLAYING && voice[i].channel == chn) {
			    voice[i].status = sVS_UNUSED;
			    SEQ_STOP_NOTE(wDevID, i, voice[i].note, 64);
			}
		    }
		    break;
		default:
		    TRACE("Dropping MIDI control event 0x%02x(%02x) on channel %d\n",
			  d1, d2, chn);
		    break;
		}
		break;
	    case MIDI_PGM_CHANGE:
		channel[chn].program = d1;
		break;
	    case MIDI_CHN_PRESSURE:
		for (i = 0; i < MidiOutDev[wDevID].caps.wVoices; i++) {
		    if (voice[i].status != sVS_UNUSED && voice[i].channel == chn) {
			SEQ_KEY_PRESSURE(wDevID, i, voice[i].note, d1);
		    }
		}
		break;
	    case MIDI_PITCH_BEND:
		channel[chn].bender = (d2 << 7) + d1;
		for (i = 0; i < MidiOutDev[wDevID].caps.wVoices; i++) {
		    if (voice[i].channel == chn) {
			SEQ_BENDER(wDevID, i, channel[chn].bender);
		    }
		}
		break;
	    case MIDI_SYSTEM_PREFIX:
		switch (evt & 0x0F) {
		case 0x0F: 	/* Reset */
		    OSS_CALL(midi_out_fm_reset, (void *)(UINT_PTR)wDevID);
		    break;
		default:
		    WARN("Unsupported (yet) system event %02x\n", evt & 0x0F);
		}
		break;
	    default:
		WARN("Internal error, shouldn't happen (event=%08x)\n", evt & 0xF0);
		return MMSYSERR_NOTENABLED;
	    }
	}
	break;
    case MOD_MIDIPORT:
	{
	    int	dev = wDevID - MODM_NumFMSynthDevs;
	    if (dev < 0) {
		WARN("Internal error on devID (%u) !\n", wDevID);
		return MIDIERR_NODEVICE;
	    }

	    switch (evt & 0xF0) {
	    case MIDI_NOTEOFF:
	    case MIDI_NOTEON:
	    case MIDI_KEY_PRESSURE:
	    case MIDI_CTL_CHANGE:
	    case MIDI_PITCH_BEND:
		SEQ_MIDIOUT(dev, evt);
		SEQ_MIDIOUT(dev, d1);
		SEQ_MIDIOUT(dev, d2);
		break;
	    case MIDI_PGM_CHANGE:
	    case MIDI_CHN_PRESSURE:
		SEQ_MIDIOUT(dev, evt);
		SEQ_MIDIOUT(dev, d1);
		break;
	    case MIDI_SYSTEM_PREFIX:
		switch (evt & 0x0F) {
		case 0x00:	/* System Exclusive, don't do it on modData,
				 * should require modLongData*/
		case 0x04:	/* Undefined. */
		case 0x05:	/* Undefined. */
		case 0x07:	/* End of Exclusive. */
		case 0x09:	/* Undefined. */
		case 0x0D:	/* Undefined. */
		    break;
		case 0x06:	/* Tune Request */
		case 0x08:	/* Timing Clock. */
		case 0x0A:	/* Start. */
		case 0x0B:	/* Continue */
		case 0x0C:	/* Stop */
		case 0x0E: 	/* Active Sensing. */
		    SEQ_MIDIOUT(dev, evt);
		    break;
		case 0x0F: 	/* Reset */
				/* SEQ_MIDIOUT(dev, evt);
				   this other way may be better */
		    SEQ_MIDIOUT(dev, MIDI_SYSTEM_PREFIX);
		    SEQ_MIDIOUT(dev, 0x7e);
		    SEQ_MIDIOUT(dev, 0x7f);
		    SEQ_MIDIOUT(dev, 0x09);
		    SEQ_MIDIOUT(dev, 0x01);
		    SEQ_MIDIOUT(dev, 0xf7);
		    break;
		case 0x01:	/* MTC Quarter frame */
		case 0x03:	/* Song Select. */
		    SEQ_MIDIOUT(dev, evt);
		    SEQ_MIDIOUT(dev, d1);
		case 0x02:	/* Song Position Pointer. */
		    SEQ_MIDIOUT(dev, evt);
		    SEQ_MIDIOUT(dev, d1);
		    SEQ_MIDIOUT(dev, d2);
		}
		break;
	    }
	}
	break;
    default:
	WARN("Technology not supported (yet) %d !\n",
	     MidiOutDev[wDevID].caps.wTechnology);
	return MMSYSERR_NOTENABLED;
    }

    SEQ_DUMPBUF();

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 *		modLongData					[internal]
 */
static DWORD modLongData(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    int		count;
    LPBYTE	lpData;

    TRACE("(%04X, %p, %08X);\n", wDevID, lpMidiHdr, dwSize);

    /* Note: MS doc does not say much about the dwBytesRecorded member of the MIDIHDR structure
     * but it seems to be used only for midi input.
     * Taking a look at the WAVEHDR structure (which is quite similar) confirms this assumption.
     */
    
    if (wDevID >= MODM_NumDevs) return MMSYSERR_BADDEVICEID;
    if (!MidiOutDev[wDevID].bEnabled) return MIDIERR_NODEVICE;

    if (MidiOutDev[wDevID].fd == -1) {
	WARN("can't play !\n");
	return MIDIERR_NODEVICE;
    }

    lpData = (LPBYTE) lpMidiHdr->lpData;

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
     * If the latter is true, then the following WARNing will fire up
     */
    if (lpData[0] != 0xF0 || lpData[lpMidiHdr->dwBufferLength - 1] != 0xF7) {
	WARN("The allegedly system exclusive buffer is not correct\n\tPlease report with MIDI file\n");
    }

    TRACE("dwBufferLength=%u !\n", lpMidiHdr->dwBufferLength);
    TRACE("                 %02X %02X %02X ... %02X %02X %02X\n",
	  lpData[0], lpData[1], lpData[2], lpData[lpMidiHdr->dwBufferLength-3],
	  lpData[lpMidiHdr->dwBufferLength-2], lpData[lpMidiHdr->dwBufferLength-1]);

    switch (MidiOutDev[wDevID].caps.wTechnology) {
    case MOD_FMSYNTH:
	/* FIXME: I don't think there is much to do here */
	break;
    case MOD_MIDIPORT:
	if (lpData[0] != 0xF0) {
	    /* Send end of System Exclusive */
	    SEQ_MIDIOUT(wDevID - MODM_NumFMSynthDevs, 0xF0);
	    WARN("Adding missing 0xF0 marker at the beginning of "
		 "system exclusive byte stream\n");
	}
	for (count = 0; count < lpMidiHdr->dwBufferLength; count++) {
	    SEQ_MIDIOUT(wDevID - MODM_NumFMSynthDevs, lpData[count]);
	}
	if (lpData[count - 1] != 0xF7) {
	    /* Send end of System Exclusive */
	    SEQ_MIDIOUT(wDevID - MODM_NumFMSynthDevs, 0xF7);
	    WARN("Adding missing 0xF7 marker at the end of "
		 "system exclusive byte stream\n");
	}
	SEQ_DUMPBUF();
	break;
    default:
	WARN("Technology not supported (yet) %d !\n",
	     MidiOutDev[wDevID].caps.wTechnology);
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
	modData(wDevID, 0x7800 | MIDI_CTL_CHANGE | chn);
	/* remove sustain on all channels */
	modData(wDevID, (CTL_SUSTAIN << 8) | MIDI_CTL_CHANGE | chn);
    }
    /* FIXME: the LongData buffers must also be returned to the app */
    return MMSYSERR_NOERROR;
}

/*======================================================================*
 *                  	    MIDI entry points 				*
 *======================================================================*/

/**************************************************************************
 * 			midMessage (WINEOSS.@)
 */
DWORD WINAPI OSS_midMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser,
			    DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    TRACE("(%04X, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);
    switch (wMsg) {
    case DRVM_INIT:
        return OSS_MidiInit();
    case DRVM_EXIT:
        return OSS_MidiExit();
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
 * 				modMessage (WINEOSS.@)
 */
DWORD WINAPI OSS_modMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser,
			    DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    struct midi_out_message_params params;
    struct notify_context notify;
    UINT err;

    TRACE("(%04X, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    switch (wMsg) {
    case DRVM_INIT:
        return OSS_MidiInit();
    case DRVM_EXIT:
        return OSS_MidiExit();
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
    }

    params.dev_id = wDevID;
    params.msg = wMsg;
    params.user = dwUser;
    params.param_1 = dwParam1;
    params.param_2 = dwParam2;
    params.err = &err;
    params.notify = &notify;

    OSS_CALL(midi_out_message, &params);

    if (!err && notify.send_notify) notify_client(&notify);

    return err;
}

/**************************************************************************
 * 				DriverProc (WINEOSS.1)
 */
LRESULT CALLBACK OSS_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
                                LPARAM dwParam1, LPARAM dwParam2)
{
     TRACE("(%08lX, %p, %08X, %08lX, %08lX)\n",
           dwDevID, hDriv, wMsg, dwParam1, dwParam2);

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
