/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * Sample MIDI Wine Driver for ALSA (basically Linux)
 *
 * Copyright 1994 	Martin Ayotte
 * Copyright 1998 	Luiz Otavio L. Zorzella (init procedures)
 * Copyright 1998/1999	Eric POUECH :
 * 		98/7 	changes for making this MIDI driver work on OSS
 * 			current support is limited to MIDI ports of OSS systems
 * 		98/9	rewriting MCI code for MIDI
 * 		98/11 	splitted in midi.c and mcimidi.c
 * Copyright 2003      Christian Costa :
 *                     ALSA port
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
 *
 * TODO: Finish midi record
 *
 */

#include "config.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include <errno.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "mmddk.h"
#ifdef HAVE_ALSA
# include "alsa.h"
#endif
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(midi);

#if defined(HAVE_ALSA) && ((SND_LIB_MAJOR == 0 && SND_LIB_MINOR >= 9) || SND_LIB_MAJOR >= 1)

typedef struct {
    int			state;                  /* -1 disabled, 0 is no recording started, 1 in recording, bit 2 set if in sys exclusive recording */
    DWORD		bufsize;
    MIDIOPENDESC	midiDesc;
    WORD		wFlags;
    LPMIDIHDR	 	lpQueueHdr;
    DWORD		dwTotalPlayed;
    unsigned char	incoming[3];
    unsigned char	incPrev;
    char		incLen;
    DWORD		startTime;
    MIDIINCAPSA         caps;
    snd_seq_addr_t      addr;
} WINE_MIDIIN;

typedef struct {
    BOOL                bEnabled;
    DWORD		bufsize;
    MIDIOPENDESC	midiDesc;
    WORD		wFlags;
    LPMIDIHDR	 	lpQueueHdr;
    DWORD		dwTotalPlayed;
    void*		lpExtra;	 	/* according to port type (MIDI, FM...), extra data when needed */
    MIDIOUTCAPSA        caps;
    snd_seq_addr_t      addr;
} WINE_MIDIOUT;

static WINE_MIDIIN	MidiInDev [MAX_MIDIINDRV ];
static WINE_MIDIOUT	MidiOutDev[MAX_MIDIOUTDRV];

/* this is the total number of MIDI out devices found (synth and port) */
static	int 		MODM_NumDevs = 0;
/* this is the total number of MIDI out devices found */
static	int 		MIDM_NumDevs = 0;

static	snd_seq_t*      midiSeq = NULL;
static	int		numOpenMidiSeq = 0;
static	UINT		midiInTimerID = 0;
static	int		numStartedMidiIn = 0;

int port_in;
int port_out;

/*======================================================================*
 *                  Low level MIDI implementation			*
 *======================================================================*/

static int midiOpenSeq(int);
static int midiCloseSeq(void);

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
 * 			MIDI_unixToWindowsDeviceType  		[internal]
 *
 * return the Windows equivalent to a Unix Device Type
 *
 */
static	int 	MIDI_AlsaToWindowsDeviceType(int type)
{
    /* MOD_MIDIPORT     output port
     * MOD_SYNTH        generic internal synth
     * MOD_SQSYNTH      square wave internal synth
     * MOD_FMSYNTH      FM internal synth
     * MOD_MAPPER       MIDI mapper
     * MOD_WAVETABLE    hardware watetable internal synth
     * MOD_SWSYNTH      software internal synth
     */

    /* FIXME Is this really the correct equivalence from ALSA to
       Windows Sound type */

    if (type & SND_SEQ_PORT_TYPE_SYNTH)
        return MOD_FMSYNTH;

    if (type & (SND_SEQ_PORT_TYPE_DIRECT_SAMPLE|SND_SEQ_PORT_TYPE_SAMPLE))
        return MOD_SYNTH;

    if (type & SND_SEQ_PORT_TYPE_MIDI_GENERIC)
        return MOD_MIDIPORT;
    
    ERR("Cannot determine the type of this midi device. Assuming FM Synth\n");
    return MOD_FMSYNTH;
}

/**************************************************************************
 * 			MIDI_NotifyClient			[internal]
 */
static DWORD MIDI_NotifyClient(UINT wDevID, WORD wMsg,
			       DWORD dwParam1, DWORD dwParam2)
{
    DWORD 		dwCallBack;
    UINT 		uFlags;
    HANDLE		hDev;
    DWORD 		dwInstance;

    TRACE("wDevID = %04X wMsg = %d dwParm1 = %04lX dwParam2 = %04lX\n",
	  wDevID, wMsg, dwParam1, dwParam2);

    switch (wMsg) {
    case MOM_OPEN:
    case MOM_CLOSE:
    case MOM_DONE:
	if (wDevID > MODM_NumDevs)
	    return MMSYSERR_BADDEVICEID;

	dwCallBack = MidiOutDev[wDevID].midiDesc.dwCallback;
	uFlags = MidiOutDev[wDevID].wFlags;
	hDev = MidiOutDev[wDevID].midiDesc.hMidi;
	dwInstance = MidiOutDev[wDevID].midiDesc.dwInstance;
	break;

    case MIM_OPEN:
    case MIM_CLOSE:
    case MIM_DATA:
    case MIM_ERROR:
	if (wDevID > MIDM_NumDevs)
	    return MMSYSERR_BADDEVICEID;

	dwCallBack = MidiInDev[wDevID].midiDesc.dwCallback;
	uFlags = MidiInDev[wDevID].wFlags;
	hDev = MidiInDev[wDevID].midiDesc.hMidi;
	dwInstance = MidiInDev[wDevID].midiDesc.dwInstance;
	break;
    default:
	WARN("Unsupported MSW-MIDI message %u\n", wMsg);
	return MMSYSERR_ERROR;
    }

    return DriverCallback(dwCallBack, uFlags, hDev, wMsg, dwInstance, dwParam1, dwParam2) ?
	0 : MMSYSERR_ERROR;
}

static int midi_warn = 1;
/**************************************************************************
 * 			midiOpenSeq				[internal]
 */
static int midiOpenSeq(int create_client)
{
    if (numOpenMidiSeq == 0) {
	if (snd_seq_open(&midiSeq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0)
        {
	    if (midi_warn)
	    {
		WARN("Error opening ALSA sequencer.\n");
	    }
	    midi_warn = 0;
	    return -1;
	}

	if (create_client) {
	    /* Setting the client name is the only init to do */
	    snd_seq_set_client_name(midiSeq, "WINE midi driver");

#if 0 /* FIXME: Is it possible to use a port for READ & WRITE ops */
            port_in = port_out = snd_seq_create_simple_port(midiSeq, "WINE ALSA Input/Output", SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_WRITE,
	             	                                                 SND_SEQ_PORT_TYPE_APPLICATION);
            if (port_out < 0)
               TRACE("Unable to create output port\n");
            else
	       TRACE("Outport port created successfully (%d)\n", port_out);
#else
            port_out = snd_seq_create_simple_port(midiSeq, "WINE ALSA Output", SND_SEQ_PORT_CAP_READ,
	                 	                                                 SND_SEQ_PORT_TYPE_APPLICATION);
	    if (port_out < 0)
		TRACE("Unable to create output port\n");
	    else
		TRACE("Outport port created successfully (%d)\n", port_out);

	    port_in = snd_seq_create_simple_port(midiSeq, "WINE ALSA Input", SND_SEQ_PORT_CAP_WRITE,
	             	                                               SND_SEQ_PORT_TYPE_APPLICATION);
            if (port_in < 0)
                TRACE("Unable to create input port\n");
            else
	        TRACE("Input port created successfully (%d)\n", port_in);
#endif
       }
    }
    numOpenMidiSeq++;
    return 0;
}

/**************************************************************************
 * 			midiCloseSeq				[internal]
 */
static int midiCloseSeq(void)
{
    if (--numOpenMidiSeq == 0) {
	snd_seq_delete_simple_port(midiSeq, port_out);
	snd_seq_delete_simple_port(midiSeq, port_in);
	snd_seq_close(midiSeq);
	midiSeq = NULL;
    }
    return 0;
}

static VOID WINAPI midTimeCallback(HWND hwnd, UINT msg, UINT id, DWORD dwTime)
{
    TRACE("(%p, %d, %d, %lu)\n", hwnd, msg, id, dwTime);

    while(snd_seq_event_input_pending(midiSeq, 0) > 0) {
	snd_seq_event_t* ev;
	TRACE("An event is pending\n");
	snd_seq_event_input(midiSeq, &ev);
	TRACE("Event received, type = %d\n", ev->type);
	snd_seq_free_event(ev);
    }
}

/**************************************************************************
 * 				midGetDevCaps			[internal]
 */
static DWORD midGetDevCaps(WORD wDevID, LPMIDIINCAPSA lpCaps, DWORD dwSize)
{
    TRACE("(%04X, %p, %08lX);\n", wDevID, lpCaps, dwSize);

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
    TRACE("(%04X, %p, %08lX);\n", wDevID, lpDesc, dwFlags);

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

    if (midiOpenSeq(1) < 0) {
	return MMSYSERR_ERROR;
    }

    /* Connect our app port to the device port */
    if (snd_seq_connect_from(midiSeq, port_in, MidiInDev[wDevID].addr.client, MidiInDev[wDevID].addr.port) < 0)
	return MMSYSERR_NOTENABLED;

    TRACE("input port connected %d %d %d\n",port_in,MidiInDev[wDevID].addr.client,MidiInDev[wDevID].addr.port);

   if (numStartedMidiIn++ == 0) {
	midiInTimerID = SetTimer(0, 0, 250, midTimeCallback);
	if (!midiInTimerID) {
	    numStartedMidiIn = 0;
	    WARN("Couldn't start timer for midi-in\n");
	    midiCloseSeq();
	    return MMSYSERR_ERROR;
	}
	TRACE("Starting timer (%u) for midi-in\n", midiInTimerID);
    }

    MidiInDev[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    MidiInDev[wDevID].lpQueueHdr = NULL;
    MidiInDev[wDevID].dwTotalPlayed = 0;
    MidiInDev[wDevID].bufsize = 0x3FFF;
    MidiInDev[wDevID].midiDesc = *lpDesc;
    MidiInDev[wDevID].state = 0;
    MidiInDev[wDevID].incLen = 0;
    MidiInDev[wDevID].startTime = 0;

    if (MIDI_NotifyClient(wDevID, MIM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN("can't notify client !\n");
	return MMSYSERR_INVALPARAM;
    }
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

    if (midiSeq == NULL) {
	WARN("ooops !\n");
	return MMSYSERR_ERROR;
    }
    if (--numStartedMidiIn == 0) {
	TRACE("Stopping timer for midi-in\n");
	if (!KillTimer(0, midiInTimerID)) {
	    WARN("Couldn't stop timer for midi-in\n");
	}
	midiInTimerID = 0;
    }

    snd_seq_disconnect_from(midiSeq, port_in, MidiInDev[wDevID].addr.client, MidiInDev[wDevID].addr.port);
    midiCloseSeq();

    MidiInDev[wDevID].bufsize = 0;
    if (MIDI_NotifyClient(wDevID, MIM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN("can't notify client !\n");
	ret = MMSYSERR_INVALPARAM;
    }
    MidiInDev[wDevID].midiDesc.hMidi = 0;

    return ret;
}


/**************************************************************************
 * 				midAddBuffer			[internal]
 */
static DWORD midAddBuffer(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);

    if (wDevID >= MIDM_NumDevs) return MMSYSERR_BADDEVICEID;
    if (MidiInDev[wDevID].state == -1) return MIDIERR_NODEVICE;

    if (lpMidiHdr == NULL)	return MMSYSERR_INVALPARAM;
    if (sizeof(MIDIHDR) > dwSize) return MMSYSERR_INVALPARAM;
    if (lpMidiHdr->dwBufferLength == 0) return MMSYSERR_INVALPARAM;
    if (lpMidiHdr->dwFlags & MHDR_INQUEUE) return MIDIERR_STILLPLAYING;
    if (!(lpMidiHdr->dwFlags & MHDR_PREPARED)) return MIDIERR_UNPREPARED;

    if (MidiInDev[wDevID].lpQueueHdr == 0) {
	MidiInDev[wDevID].lpQueueHdr = lpMidiHdr;
    } else {
	LPMIDIHDR	ptr;

	for (ptr = MidiInDev[wDevID].lpQueueHdr;
	     ptr->lpNext != 0;
	     ptr = (LPMIDIHDR)ptr->lpNext);
	ptr->lpNext = (struct midihdr_tag*)lpMidiHdr;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midPrepare			[internal]
 */
static DWORD midPrepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);

    if (dwSize < sizeof(MIDIHDR) || lpMidiHdr == 0 ||
	lpMidiHdr->lpData == 0 || lpMidiHdr->dwFlags != 0 ||
	lpMidiHdr->dwBufferLength >= 0x10000ul)
	return MMSYSERR_INVALPARAM;

    lpMidiHdr->lpNext = 0;
    lpMidiHdr->dwFlags |= MHDR_PREPARED;
    lpMidiHdr->dwBytesRecorded = 0;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midUnprepare			[internal]
 */
static DWORD midUnprepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);

    if (wDevID >= MIDM_NumDevs) return MMSYSERR_BADDEVICEID;
    if (MidiInDev[wDevID].state == -1) return MIDIERR_NODEVICE;

    if (dwSize < sizeof(MIDIHDR) || lpMidiHdr == 0 ||
	lpMidiHdr->lpData == 0 || lpMidiHdr->dwBufferLength >= 0x10000ul)
	return MMSYSERR_INVALPARAM;

    if (!(lpMidiHdr->dwFlags & MHDR_PREPARED)) return MIDIERR_UNPREPARED;
    if (lpMidiHdr->dwFlags & MHDR_INQUEUE) return MIDIERR_STILLPLAYING;

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

    while (MidiInDev[wDevID].lpQueueHdr) {
	MidiInDev[wDevID].lpQueueHdr->dwFlags &= ~MHDR_INQUEUE;
	MidiInDev[wDevID].lpQueueHdr->dwFlags |= MHDR_DONE;
	/* FIXME: when called from 16 bit, lpQueueHdr needs to be a segmented ptr */
	if (MIDI_NotifyClient(wDevID, MIM_LONGDATA,
			      (DWORD)MidiInDev[wDevID].lpQueueHdr, dwTime) != MMSYSERR_NOERROR) {
	    WARN("Couldn't notify client\n");
	}
	MidiInDev[wDevID].lpQueueHdr = (LPMIDIHDR)MidiInDev[wDevID].lpQueueHdr->lpNext;
    }

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
static DWORD modGetDevCaps(WORD wDevID, LPMIDIOUTCAPSA lpCaps, DWORD dwSize)
{
    TRACE("(%04X, %p, %08lX);\n", wDevID, lpCaps, dwSize);

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
    TRACE("(%04X, %p, %08lX);\n", wDevID, lpDesc, dwFlags);
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
    if (!MidiOutDev[wDevID].bEnabled) {
	TRACE("disabled wDevID\n");
	return MMSYSERR_NOTENABLED;
    }

    MidiOutDev[wDevID].lpExtra = 0;

    switch (MidiOutDev[wDevID].caps.wTechnology) {
    case MOD_FMSYNTH:
    case MOD_MIDIPORT:
    case MOD_SYNTH:
	if (midiOpenSeq(1) < 0) {
	    return MMSYSERR_ALLOCATED;
	}
	break;
    default:
	WARN("Technology not supported (yet) %d !\n",
	     MidiOutDev[wDevID].caps.wTechnology);
	return MMSYSERR_NOTENABLED;
    }

    MidiOutDev[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    MidiOutDev[wDevID].lpQueueHdr = NULL;
    MidiOutDev[wDevID].dwTotalPlayed = 0;
    MidiOutDev[wDevID].bufsize = 0x3FFF;
    MidiOutDev[wDevID].midiDesc = *lpDesc;

    /* Connect our app port to the device port */
    if (snd_seq_connect_to(midiSeq, port_out, MidiOutDev[wDevID].addr.client, MidiOutDev[wDevID].addr.port) < 0)
	return MMSYSERR_NOTENABLED;
    
    if (MIDI_NotifyClient(wDevID, MOM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN("can't notify client !\n");
	return MMSYSERR_INVALPARAM;
    }
    TRACE("Successful !\n");
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

    if (midiSeq == NULL) {
	WARN("can't close !\n");
	return MMSYSERR_ERROR;
    }

    switch (MidiOutDev[wDevID].caps.wTechnology) {
    case MOD_FMSYNTH:
    case MOD_MIDIPORT:
    case MOD_SYNTH:
        snd_seq_disconnect_to(midiSeq, port_out, MidiOutDev[wDevID].addr.client, MidiOutDev[wDevID].addr.port);
	midiCloseSeq();
	break;
    default:
	WARN("Technology not supported (yet) %d !\n",
	     MidiOutDev[wDevID].caps.wTechnology);
	return MMSYSERR_NOTENABLED;
    }

    if (MidiOutDev[wDevID].lpExtra != 0) {
        HeapFree(GetProcessHeap(), 0, MidiOutDev[wDevID].lpExtra);
	MidiOutDev[wDevID].lpExtra = 0;
    }
 
    MidiOutDev[wDevID].bufsize = 0;
    if (MIDI_NotifyClient(wDevID, MOM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN("can't notify client !\n");
	ret = MMSYSERR_INVALPARAM;
    }
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
    
    TRACE("(%04X, %08lX);\n", wDevID, dwParam);

    if (wDevID >= MODM_NumDevs) return MMSYSERR_BADDEVICEID;
    if (!MidiOutDev[wDevID].bEnabled) return MIDIERR_NODEVICE;

    if (midiSeq == NULL) {
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
            snd_seq_ev_set_source(&event, port_out);
            snd_seq_ev_set_dest(&event, MidiOutDev[wDevID].addr.client, MidiOutDev[wDevID].addr.port);
	    
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
		snd_seq_ev_set_pitchbend(&event, evt&0x0F, ((WORD)d1 << 7) | (WORD)d2);
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
		case 0x01:	/* Undefined */
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
		    /* FIXME: Is this function suitable for these purposes
		       (and also Song Select and Song Position Pointer) */
	            snd_seq_ev_set_sysex(&event, 1, &evt);
		    break;
		case 0x0F: 	/* Reset */
				/* snd_seq_ev_set_sysex(&event, 1, &evt);
				   this other way may be better */
		    {
			BYTE reset_sysex_seq[] = {MIDI_CMD_COMMON_SYSEX, 0x7e, 0x7f, 0x09, 0x01, 0xf7};
			snd_seq_ev_set_sysex(&event, sizeof(reset_sysex_seq), reset_sysex_seq);
		    }
		    break;
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
	    if (handled)
                snd_seq_event_output_direct(midiSeq, &event);
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
    int		len_add = 0;
    LPBYTE	lpData, lpNewData = NULL;
    snd_seq_event_t event;

    TRACE("(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);

    /* Note: MS doc does not say much about the dwBytesRecorded member of the MIDIHDR structure
     * but it seems to be used only for midi input.
     * Taking a look at the WAVEHDR structure (which is quite similar) confirms this assumption.
     */
    
    if (wDevID >= MODM_NumDevs) return MMSYSERR_BADDEVICEID;
    if (!MidiOutDev[wDevID].bEnabled) return MIDIERR_NODEVICE;

    if (midiSeq == NULL) {
	WARN("can't play !\n");
	return MIDIERR_NODEVICE;
    }

    lpData = lpMidiHdr->lpData;
    
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
	WARN("Alledged system exclusive buffer is not correct\n\tPlease report with MIDI file\n");
	lpNewData = HeapAlloc(GetProcessHeap, 0, lpMidiHdr->dwBufferLength + 2);
    }

    TRACE("dwBufferLength=%lu !\n", lpMidiHdr->dwBufferLength);
    TRACE("                 %02X %02X %02X ... %02X %02X %02X\n",
	  lpData[0], lpData[1], lpData[2], lpData[lpMidiHdr->dwBufferLength-3],
	  lpData[lpMidiHdr->dwBufferLength-2], lpData[lpMidiHdr->dwBufferLength-1]);

    switch (MidiOutDev[wDevID].caps.wTechnology) {
    case MOD_FMSYNTH:
	/* FIXME: I don't think there is much to do here */
	break;
    case MOD_MIDIPORT:
	if (lpData[0] != 0xF0) {
	    /* Send start of System Exclusive */
	    len_add = 1;
	    lpData[0] = 0xF0;
	    memcpy(lpNewData, lpData, lpMidiHdr->dwBufferLength);
	    WARN("Adding missing 0xF0 marker at the beginning of "
		 "system exclusive byte stream\n");
	}
	if (lpData[lpMidiHdr->dwBufferLength-1] != 0xF7) {
	    /* Send end of System Exclusive */
	    memcpy(lpData + len_add, lpData, lpMidiHdr->dwBufferLength);
            lpNewData[lpMidiHdr->dwBufferLength + len_add - 1] = 0xF0;
	    len_add++;
	    WARN("Adding missing 0xF7 marker at the end of "
		 "system exclusive byte stream\n");
	}
	snd_seq_ev_clear(&event);
	snd_seq_ev_set_direct(&event);
	snd_seq_ev_set_source(&event, port_out);
	snd_seq_ev_set_dest(&event, MidiOutDev[wDevID].addr.client, MidiOutDev[wDevID].addr.port);
	TRACE("client = %d port = %d\n", MidiOutDev[wDevID].addr.client, MidiOutDev[wDevID].addr.port);
	snd_seq_ev_set_sysex(&event, lpMidiHdr->dwBufferLength + len_add, lpNewData ? lpNewData : lpData);
	snd_seq_event_output_direct(midiSeq, &event);
	if (lpNewData)
		HeapFree(GetProcessHeap(), 0, lpData);
	break;
    default:
	WARN("Technology not supported (yet) %d !\n",
	     MidiOutDev[wDevID].caps.wTechnology);
	return MMSYSERR_NOTENABLED;
    }

    lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
    lpMidiHdr->dwFlags |= MHDR_DONE;
    if (MIDI_NotifyClient(wDevID, MOM_DONE, (DWORD)lpMidiHdr, 0L) != MMSYSERR_NOERROR) {
	WARN("can't notify client !\n");
	return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			modPrepare				[internal]
 */
static DWORD modPrepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);

    if (midiSeq == NULL) {
	WARN("can't prepare !\n");
	return MMSYSERR_NOTENABLED;
    }

    /* MS doc says that dwFlags must be set to zero, but (kinda funny) MS mciseq drivers
     * asks to prepare MIDIHDR which dwFlags != 0.
     * So at least check for the inqueue flag
     */
    if (dwSize < sizeof(MIDIHDR) || lpMidiHdr == 0 ||
	lpMidiHdr->lpData == 0 || (lpMidiHdr->dwFlags & MHDR_INQUEUE) != 0 ||
	lpMidiHdr->dwBufferLength >= 0x10000ul) {
	WARN("%p %p %08lx %d/%ld\n", lpMidiHdr, lpMidiHdr->lpData,
	           lpMidiHdr->dwFlags, sizeof(MIDIHDR), dwSize);
	return MMSYSERR_INVALPARAM;
    }

    lpMidiHdr->lpNext = 0;
    lpMidiHdr->dwFlags |= MHDR_PREPARED;
    lpMidiHdr->dwFlags &= ~MHDR_DONE;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				modUnprepare			[internal]
 */
static DWORD modUnprepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);

    if (midiSeq == NULL) {
	WARN("can't unprepare !\n");
	return MMSYSERR_NOTENABLED;
    }

    if (dwSize < sizeof(MIDIHDR) || lpMidiHdr == 0)
	return MMSYSERR_INVALPARAM;
    if (lpMidiHdr->dwFlags & MHDR_INQUEUE)
	return MIDIERR_STILLPLAYING;
    lpMidiHdr->dwFlags &= ~MHDR_PREPARED;
    return MMSYSERR_NOERROR;
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

#endif /* defined(HAVE_ALSA) && ((SND_LIB_MAJOR == 0 && SND_LIB_MINOR >= 9) || SND_LIB_MAJOR >= 1) */


/*======================================================================*
 *                  	    MIDI entry points 				*
 *======================================================================*/

/**************************************************************************
 * ALSA_MidiInit				[internal]
 *
 * Initializes the MIDI devices information variables
 */
LONG ALSA_MidiInit(void)
{
#if defined(HAVE_ALSA) && ((SND_LIB_MAJOR == 0 && SND_LIB_MINOR >= 9) || SND_LIB_MAJOR >= 1)
    static	BOOL	bInitDone = FALSE;
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    int count;

    if (bInitDone)
	return TRUE;

    TRACE("Initializing the MIDI variables.\n");
    bInitDone = TRUE;

    /* try to open device */
    if (midiOpenSeq(0) == -1) {
	return TRUE;
    }

#if 0 /* Debug purpose */
    snd_lib_error_set_handler(error_handler);
#endif
    
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    while(snd_seq_query_next_client(midiSeq, cinfo) >= 0) {
        snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
	snd_seq_port_info_set_port(pinfo, -1);
	count = 0;
	while (snd_seq_query_next_port(midiSeq, pinfo) >= 0) {
            int cap = snd_seq_port_info_get_capability(pinfo);
	    int type = snd_seq_port_info_get_type(pinfo);
	    if (cap & SND_SEQ_PORT_CAP_WRITE) {
                TRACE("OUT (%d:%s:%s:%d:%s:%x)\n",snd_seq_client_info_get_client(cinfo),
				                snd_seq_client_info_get_name(cinfo),
						snd_seq_client_info_get_type(cinfo) == SND_SEQ_USER_CLIENT ? "user" : "kernel",
						snd_seq_port_info_get_port(pinfo),
						snd_seq_port_info_get_name(pinfo),
						type);
		
		if (MODM_NumDevs >= MAX_MIDIOUTDRV)
		    continue;
		if (!type)
	            continue;

		memcpy(&MidiOutDev[MODM_NumDevs].addr, snd_seq_port_info_get_addr(pinfo), sizeof(snd_seq_addr_t));
		
		/* Manufac ID. We do not have access to this with soundcard.h
		 * Does not seem to be a problem, because in mmsystem.h only
		 * Microsoft's ID is listed.
		 */
		MidiOutDev[MODM_NumDevs].caps.wMid = 0x00FF;
		MidiOutDev[MODM_NumDevs].caps.wPid = 0x0001; 	/* FIXME Product ID  */
		/* Product Version. We simply say "1" */
		MidiOutDev[MODM_NumDevs].caps.vDriverVersion = 0x001;
		MidiOutDev[MODM_NumDevs].caps.wChannelMask   = 0xFFFF;

		/* FIXME Do we have this information?
		 * Assuming the soundcards can handle
		 * MIDICAPS_VOLUME and MIDICAPS_LRVOLUME but
		 * not MIDICAPS_CACHE.
		 */
		MidiOutDev[MODM_NumDevs].caps.dwSupport      = MIDICAPS_VOLUME|MIDICAPS_LRVOLUME;
                strcpy(MidiOutDev[MODM_NumDevs].caps.szPname, snd_seq_client_info_get_name(cinfo));

                MidiOutDev[MODM_NumDevs].caps.wTechnology = MIDI_AlsaToWindowsDeviceType(type);
                MidiOutDev[MODM_NumDevs].caps.wVoices     = 16;

                /* FIXME Is it possible to know the maximum
                 * number of simultaneous notes of a soundcard ?
                 * I believe we don't have this information, but
                 * it's probably equal or more than wVoices
                 */
                MidiOutDev[MODM_NumDevs].caps.wNotes = 16;
                MidiOutDev[MODM_NumDevs].bEnabled    = TRUE;

	        TRACE("MidiOut[%d]\tname='%s' techn=%d voices=%d notes=%d chnMsk=%04x support=%ld\n"
	      	      "\tALSA info: midi dev-type=%lx, capa=%lx\n",
	      	      MODM_NumDevs, MidiOutDev[MODM_NumDevs].caps.szPname, MidiOutDev[MODM_NumDevs].caps.wTechnology,
	      	      MidiOutDev[MODM_NumDevs].caps.wVoices, MidiOutDev[MODM_NumDevs].caps.wNotes,
	              MidiOutDev[MODM_NumDevs].caps.wChannelMask, MidiOutDev[MODM_NumDevs].caps.dwSupport,
	              (long)type, (long)0);

		
		MODM_NumDevs++;
            }
	    if (cap & SND_SEQ_PORT_CAP_READ) {
                TRACE("IN  (%d:%s:%s:%d:%s:%x)\n",snd_seq_client_info_get_client(cinfo),
				                snd_seq_client_info_get_name(cinfo),
						snd_seq_client_info_get_type(cinfo) == SND_SEQ_USER_CLIENT ? "user" : "kernel",
						snd_seq_port_info_get_port(pinfo),
						snd_seq_port_info_get_name(pinfo),
						type);
		
		if (MIDM_NumDevs >= MAX_MIDIINDRV)
		    continue;
		if (!type)
		    continue;

		memcpy(&MidiInDev[MIDM_NumDevs].addr, snd_seq_port_info_get_addr(pinfo), sizeof(snd_seq_addr_t));
		
		/* Manufac ID. We do not have access to this with soundcard.h
		 * Does not seem to be a problem, because in mmsystem.h only
		 * Microsoft's ID is listed.
		 */
		MidiInDev[MIDM_NumDevs].caps.wMid = 0x00FF;
		MidiInDev[MIDM_NumDevs].caps.wPid = 0x0001; 	/* FIXME Product ID  */
		/* Product Version. We simply say "1" */
		MidiInDev[MIDM_NumDevs].caps.vDriverVersion = 0x001;

		/* FIXME Do we have this information?
		 * Assuming the soundcards can handle
		 * MIDICAPS_VOLUME and MIDICAPS_LRVOLUME but
		 * not MIDICAPS_CACHE.
		 */
		MidiInDev[MIDM_NumDevs].caps.dwSupport      = MIDICAPS_VOLUME|MIDICAPS_LRVOLUME;
                strcpy(MidiInDev[MIDM_NumDevs].caps.szPname, snd_seq_client_info_get_name(cinfo));

                MidiInDev[MIDM_NumDevs].state = 0;

                TRACE("MidiIn [%d]\tname='%s' support=%ld\n"
	      	      "\tALSA info: midi dev-type=%lx, capa=%lx\n",
	              MIDM_NumDevs, MidiInDev[MIDM_NumDevs].caps.szPname, MidiInDev[MIDM_NumDevs].caps.dwSupport,
	              (long)type, (long)0);

		MIDM_NumDevs++;
	    }
	}
    }

    /* close file and exit */
    midiCloseSeq();

    TRACE("End\n");
#endif
    return TRUE;
}

/**************************************************************************
 * 			midMessage (WINEOSS.4)
 */
DWORD WINAPI ALSA_midMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%04X, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);
    switch (wMsg) {
#if defined(HAVE_ALSA) && ((SND_LIB_MAJOR == 0 && SND_LIB_MINOR >= 9) || SND_LIB_MAJOR >= 1)
    case DRVM_INIT:
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
	return midGetDevCaps(wDevID, (LPMIDIINCAPSA)dwParam1,dwParam2);
    case MIDM_GETNUMDEVS:
	return MIDM_NumDevs;
    case MIDM_RESET:
	return midReset(wDevID);
    case MIDM_START:
	return midStart(wDevID);
    case MIDM_STOP:
	return midStop(wDevID);
#endif
    default:
	TRACE("Unsupported message\n");
    }
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				modMessage (WINEOSS.5)
 */
DWORD WINAPI ALSA_modMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%04X, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    switch (wMsg) {
#if defined(HAVE_ALSA) && ((SND_LIB_MAJOR == 0 && SND_LIB_MINOR >= 9) || SND_LIB_MAJOR >= 1)
    case DRVM_INIT:
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
	return modGetDevCaps(wDevID, (LPMIDIOUTCAPSA)dwParam1, dwParam2);
    case MODM_GETNUMDEVS:
	return MODM_NumDevs;
    case MODM_GETVOLUME:
	return 0;
    case MODM_SETVOLUME:
	return 0;
    case MODM_RESET:
	return modReset(wDevID);
#endif
    default:
	TRACE("Unsupported message\n");
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*-----------------------------------------------------------------------*/
