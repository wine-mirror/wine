/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * Sample MIDI Wine Driver for Linux
 *
 * Copyright 1994 Martin Ayotte
 */

/* 
 * Eric POUECH : 
 * 98/7 changes for making this MIDI driver work on OSS
 * 	current support is limited to MIDI ports of OSS systems
 * 98/9	rewriting MCI code for MIDI
 * 98/11 splitted in midi.c and mcimidi.c
 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "wine/winuser16.h"
#include "ldt.h"
#include "multimedia.h"
#include "user.h"
#include "driver.h"
#include "xmalloc.h"
#include "debug.h"
#include "callback.h"
#include "options.h"
#include "heap.h"

typedef struct {
#ifndef HAVE_OSS
    int			unixdev;
#endif
    int			state;
    DWORD		bufsize;
    LPMIDIOPENDESC	midiDesc;
    WORD		wFlags;
    LPMIDIHDR 		lpQueueHdr;
    DWORD		dwTotalPlayed;
#ifdef HAVE_OSS
    unsigned char	incoming[3];
    unsigned char	incPrev;
    char		incLen;
    DWORD		startTime;
#endif
} WINE_MIDIIN;

typedef struct {
#ifndef HAVE_OSS
    int			unixdev;
#endif
    int			state;
    DWORD		bufsize;
    LPMIDIOPENDESC	midiDesc;
    WORD		wFlags;
    LPMIDIHDR 		lpQueueHdr;
    DWORD		dwTotalPlayed;
#ifdef HAVE_OSS
    void*		lpExtra;	 	/* according to port type (MIDI, FM...), extra data when needed */
#endif
} WINE_MIDIOUT;

static WINE_MIDIIN	MidiInDev [MAX_MIDIINDRV ];
static WINE_MIDIOUT	MidiOutDev[MAX_MIDIOUTDRV];

/* this is the total number of MIDI out devices found */
int 	MODM_NUMDEVS = 0;				
/* this is the number of FM synthetizers (index from 0 to 
   NUMFMSYNTHDEVS - 1) */
int	MODM_NUMFMSYNTHDEVS = 0;	
/* this is the number of Midi ports (index from NUMFMSYNTHDEVS to 
   NUMFMSYNTHDEVS + NUMMIDIDEVS - 1) */
int	MODM_NUMMIDIDEVS = 0;		

/* this is the total number of MIDI out devices found */
int 	MIDM_NUMDEVS = 0;				

#ifdef HAVE_OSS
static	int		midiSeqFD = -1;
static	int		numOpenMidiSeq = 0;
static	UINT32		midiInTimerID = 0;
static	int		numStartedMidiIn = 0;
#endif /* HAVE_OSS */

/* this structure holds pointers with information for each MIDI
 * out device found.
 */
LPMIDIOUTCAPS16 midiOutDevices[MAX_MIDIOUTDRV];

/* this structure holds pointers with information for each MIDI
 * in device found.
 */
LPMIDIINCAPS16  midiInDevices [MAX_MIDIINDRV];

/* 
 * FIXME : all tests on device ID for midXXX and modYYY are made against 
 * MAX_MIDIxxDRV (when they are made) but should be done against the actual
 * number of midi devices found...
 * check also when HAVE_OSS is defined that midiDesc is not NULL
 */

/*======================================================================*
 *                  Low level MIDI implemantation			*
 *======================================================================*/
#ifdef SNDCTL_MIDI_INFO
/**************************************************************************
 * 			MIDI_NotifyClient			[internal]
 */
static DWORD MIDI_NotifyClient(UINT16 wDevID, WORD wMsg, 
			       DWORD dwParam1, DWORD dwParam2)
{
    DWORD 		dwCallBack;
    UINT16 		uFlags;
    HANDLE16 		hDev;
    DWORD 		dwInstance;
    
    TRACE(midi,"wDevID = %04X wMsg = %d dwParm1 = %04lX dwParam2 = %04lX\n", 
	  wDevID, wMsg, dwParam1, dwParam2);
    
    switch (wMsg) {
    case MOM_OPEN:
    case MOM_CLOSE:
    case MOM_DONE:
	if (wDevID > MAX_MIDIOUTDRV) 
	    return MCIERR_INTERNAL;
	
	dwCallBack = MidiOutDev[wDevID].midiDesc->dwCallback;
	uFlags = MidiOutDev[wDevID].wFlags;
	hDev = MidiOutDev[wDevID].midiDesc->hMidi;
	dwInstance = MidiOutDev[wDevID].midiDesc->dwInstance;
	break;
	
    case MIM_OPEN:
    case MIM_CLOSE:
    case MIM_DATA:
    case MIM_ERROR:
	if (wDevID > MAX_MIDIINDRV) 
	    return MCIERR_INTERNAL;
	
	dwCallBack = MidiInDev[wDevID].midiDesc->dwCallback;
	uFlags = MidiInDev[wDevID].wFlags;
	hDev = MidiInDev[wDevID].midiDesc->hMidi;
	dwInstance = MidiInDev[wDevID].midiDesc->dwInstance;
	break;
    default:
	WARN(midi, "Unsupported MSW-MIDI message %u\n", wMsg);
	return MCIERR_INTERNAL;
    }
    
    return DriverCallback(dwCallBack, uFlags, hDev, wMsg, dwInstance, dwParam1, dwParam2) ?
	0 : MCIERR_INTERNAL;
}

#ifdef HAVE_OSS
/**************************************************************************
 * 			midiOpenSeq				[internal]
 */
static int midiOpenSeq(void)
{
    if (numOpenMidiSeq == 0) {
	midiSeqFD = open(MIDI_SEQ, O_RDWR, 0);
	if (midiSeqFD == -1) {
	    ERR(midi, "can't open '%s' ! (%d)\n", MIDI_SEQ, errno);
	    return -1;
	}
	if (fcntl(midiSeqFD, F_SETFL, O_NONBLOCK) < 0) {
	    WARN(midi, "can't set sequencer fd to non blocking (%d)\n", errno);
	    close(midiSeqFD);
	    midiSeqFD = -1;
	    return -1;
	}
	ioctl(midiSeqFD, SNDCTL_SEQ_RESET);
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
	close(midiSeqFD);
	midiSeqFD = -1;
    }
    return 0;
}

/* FIXME: this is a bad idea, it's even not static... */
SEQ_DEFINEBUF(1024);

/* FIXME: this is not reentrant, not static - because of global variable 
 * _seqbuf and al. */
/**************************************************************************
 * 			seqbuf_dump				[internal]
 */
void seqbuf_dump(void)
{
    if (_seqbufptr) {
	if (write(midiSeqFD, _seqbuf, _seqbufptr) == -1) {
	    WARN(midi, "Can't write data to sequencer (%d/%d)!\n", 
		 midiSeqFD, errno);
	}
	/* FIXME:
	 *	in any case buffer is lost so that if many errors occur the buffer 
	 * will not overrun 
	 */
	_seqbufptr = 0;
    }
}
#endif /* HAVE_OSS */

#ifdef HAVE_OSS

static void midReceiveChar(WORD wDevID, unsigned char value, DWORD dwTime)
{
    DWORD		toSend = 0;
    
    TRACE(midi, "Adding %02xh to %d[%d]\n", value, wDevID, MidiInDev[wDevID].incLen);
    
    if (wDevID >= MAX_MIDIINDRV) {
	WARN(midi, "bad devID\n");
	return;
    }
    if (MidiInDev[wDevID].state == 0) {
	TRACE(midi, "input not started, thrown away\n");
	return;
    }
    
    if (MidiInDev[wDevID].state & 2) { /* system exclusive */
	LPMIDIHDR	ptr = MidiInDev[wDevID].lpQueueHdr;
	WORD 			sbfb = FALSE;
	
	if (ptr) {
	    ((LPSTR)(ptr->reserved))[ptr->dwBytesRecorded++] = value;
	    if (ptr->dwBytesRecorded == ptr->dwBufferLength) {
		sbfb = TRUE;
	    } 
	}
	if (value == 0xF7) { /* then end */
	    MidiInDev[wDevID].state &= ~2;
	    sbfb = TRUE;
	}
	if (sbfb && ptr != NULL) {
	    ptr = MidiInDev[wDevID].lpQueueHdr;
	    ptr->dwFlags &= ~MHDR_INQUEUE;
	    ptr->dwFlags |= MHDR_DONE;
	    MidiInDev[wDevID].lpQueueHdr = (LPMIDIHDR)ptr->lpNext;
	    if (MIDI_NotifyClient(wDevID, MIM_LONGDATA, (DWORD)ptr, dwTime) != MMSYSERR_NOERROR) {
		WARN(midi, "Couldn't notify client\n");
	    }
	}
	return;
    }
    
#define IS_CMD(_x)	(((_x) & 0x80) == 0x80)
#define IS_SYS_CMD(_x)	(((_x) & 0xF0) == 0xF0)
    
    if (!IS_CMD(value) && MidiInDev[wDevID].incLen == 0) { /* try to reuse old cmd */
	if (IS_CMD(MidiInDev[wDevID].incPrev) && !IS_SYS_CMD(MidiInDev[wDevID].incPrev)) {
	    MidiInDev[wDevID].incoming[0] = MidiInDev[wDevID].incPrev;
	    MidiInDev[wDevID].incLen = 1;
	    TRACE(midi, "Reusing old command %02xh\n", MidiInDev[wDevID].incPrev);
	} else {
	    FIXME(midi, "error for midi-in, should generate MIM_ERROR notification:"
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
    
#undef IS_CMD(_x)
#undef IS_SYS_CMD(_x)
    
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
	WARN(midi, "This shouldn't happen (%02X)\n", MidiInDev[wDevID].incoming[0]);
    }
    if (toSend != 0) {
	TRACE(midi, "Sending event %08lx\n", toSend);
	MidiInDev[wDevID].incLen =	0;
	dwTime -= MidiInDev[wDevID].startTime;
	if (MIDI_NotifyClient(wDevID, MIM_DATA, toSend, dwTime) != MMSYSERR_NOERROR) {
	    WARN(midi, "Couldn't notify client\n");
	}
    }
}

static VOID WINAPI midTimeCallback(HWND32 hwnd, UINT32 msg, UINT32 id, DWORD dwTime)
{
    unsigned	char		buffer[256];
    int				len, idx;
    
    TRACE(midi, "(%04X, %d, %d, %lu)\n", hwnd, msg, id, dwTime);
    
    len = read(midiSeqFD, buffer, sizeof(buffer));
    
    if ((len % 4) != 0) {
	WARN(midi, "bad length %d (%d)\n", len, errno);
	return;
    }
    
    for (idx = 0; idx < len; ) {
	if (buffer[idx] & 0x80) {
	    TRACE(midi, 
		  "reading<8> %02x %02x %02x %02x %02x %02x %02x %02x\n", 
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
		TRACE(midi, "Unsupported event %d\n", buffer[idx + 0]);
		break;
	    }
	    idx += 4;
	}				
    }
}
#endif /* HAVE_OSS */

/**************************************************************************
 * 				midGetDevCaps			[internal]
 */
static DWORD midGetDevCaps(WORD wDevID, LPMIDIINCAPS16 lpCaps, DWORD dwSize)
{
    LPMIDIINCAPS16	tmplpCaps;
    
    TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpCaps, dwSize);
    
    if (wDevID >= MIDM_NUMDEVS) {
	return MMSYSERR_BADDEVICEID;
    }
    if (lpCaps == NULL) {
	return MMSYSERR_INVALPARAM;
    }
    
    tmplpCaps = midiInDevices[wDevID];
    lpCaps->wMid = tmplpCaps->wMid;  
    lpCaps->wPid = tmplpCaps->wPid;  
    lpCaps->vDriverVersion = tmplpCaps->vDriverVersion;  
    strcpy(lpCaps->szPname, tmplpCaps->szPname);    
    if (dwSize == sizeof(MIDIINCAPS16)) { 
	/* we should run win 95, so make use of dwSupport */
	lpCaps->dwSupport = tmplpCaps->dwSupport;
    } else if (dwSize != sizeof(MIDIINCAPS16) - sizeof(DWORD)) {
	TRACE(midi, "bad size for lpCaps\n");
	return MMSYSERR_INVALPARAM;
    }
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			midOpen					[internal]
 */
static DWORD midOpen(WORD wDevID, LPMIDIOPENDESC lpDesc, DWORD dwFlags)
{
    TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpDesc, dwFlags);
    
    if (lpDesc == NULL) {
	WARN(midi, "Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    /* FIXME :
     *	how to check that content of lpDesc is correct ?
     */
    if (wDevID >= MAX_MIDIINDRV) {
	WARN(midi,"wDevID too large (%u) !\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }
    if (MidiInDev[wDevID].midiDesc != 0) {
	WARN(midi, "device already open !\n");
	return MMSYSERR_ALLOCATED;
    }
    if ((dwFlags & ~CALLBACK_TYPEMASK) != 0) { 
	FIXME(midi, "No support for MIDI_IO_STATUS in dwFlags\n");
	return MMSYSERR_INVALFLAG;
    }
    
#ifdef HAVE_OSS
    if (midiOpenSeq() < 0) {
	return MMSYSERR_ERROR;
    }
    
    if (numStartedMidiIn++ == 0) {
	midiInTimerID = SetTimer32(0, 0, 250, midTimeCallback);
	if (!midiInTimerID) {
	    numStartedMidiIn = 0;
	    WARN(midi, "Couldn't start timer for midi-in\n");
	    midiCloseSeq();
	    return MMSYSERR_ERROR;
	}
	TRACE(midi, "Starting timer (%u) for midi-in\n", midiInTimerID);
    }
#else /* HAVE_OSS */
    {
	int		midi = open(MIDI_DEV, O_WRONLY, 0);
	
	MidiInDev[wDevID].unixdev = 0;
	if (midi == -1) {
	    WARN(midi,"can't open '%s' (%d)!\n", MIDI_DEV, errno);			
	    return MMSYSERR_ALLOCATED;
	}
	MidiInDev[wDevID].unixdev = midi;
    }
#endif /* HAVE_OSS */
    
    MidiInDev[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
    switch (MidiInDev[wDevID].wFlags) {
    case DCB_NULL:
	TRACE(midi,"CALLBACK_NULL !\n");
	break;
    case DCB_WINDOW:
	TRACE(midi, "CALLBACK_WINDOW !\n");
	break;
    case DCB_TASK:
	TRACE(midi, "CALLBACK_TASK !\n");
	break;
    case DCB_FUNCTION:
	TRACE(midi, "CALLBACK_FUNCTION (%08lX)!\n", lpDesc->dwCallback);
	break;
    }
    MidiInDev[wDevID].lpQueueHdr = NULL;
    MidiInDev[wDevID].dwTotalPlayed = 0;
    MidiInDev[wDevID].bufsize = 0x3FFF;
    MidiInDev[wDevID].midiDesc = lpDesc;
    MidiInDev[wDevID].state = 0;
#ifdef HAVE_OSS
    MidiInDev[wDevID].incLen = 0;
    MidiInDev[wDevID].startTime = 0;
#endif /* HAVE_OSS */
    if (MIDI_NotifyClient(wDevID, MIM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN(midi,"can't notify client !\n");
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
    
    TRACE(midi, "(%04X);\n", wDevID);
    
    if (wDevID >= MAX_MIDIINDRV) {
	WARN(midi,"wDevID too bif (%u) !\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }
    if (MidiInDev[wDevID].midiDesc == 0) {
	WARN(midi, "device not opened !\n");
	return MMSYSERR_ERROR;
    }
    if (MidiInDev[wDevID].lpQueueHdr != 0) {
	return MIDIERR_STILLPLAYING;
    }
    
#ifdef HAVE_OSS
    if (midiSeqFD == -1) {
	WARN(midi,"ooops !\n");
	return MMSYSERR_ERROR;
    }
    if (--numStartedMidiIn == 0) {
	TRACE(midi, "Stopping timer for midi-in\n");
	if (!KillTimer32(0, midiInTimerID)) {
	    WARN(midi, "Couldn't stop timer for midi-in\n");
	}			
	midiInTimerID = 0;
    }
    midiCloseSeq();
#else /* HAVE_OSS */
    if (MidiInDev[wDevID].unixdev == 0) {
	WARN(midi,"ooops !\n");
	return MMSYSERR_ERROR;
    }
    close(MidiInDev[wDevID].unixdev);
    MidiInDev[wDevID].unixdev = 0;
#endif /* HAVE_OSS */
    MidiInDev[wDevID].bufsize = 0;
    if (MIDI_NotifyClient(wDevID, MIM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN(midi,"can't notify client !\n");
	ret = MMSYSERR_INVALPARAM;
    }
    MidiInDev[wDevID].midiDesc = 0;
    return ret;
}

/**************************************************************************
 * 				midAddBuffer			[internal]
 */
static DWORD midAddBuffer(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);
    
    if (lpMidiHdr == NULL)	return MMSYSERR_INVALPARAM;
    if (sizeof(MIDIHDR) != dwSize) return MMSYSERR_INVALPARAM;
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
    TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);
    
    if (dwSize != sizeof(MIDIHDR) || lpMidiHdr == 0 || 
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
    TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);
    
    if (dwSize != sizeof(MIDIHDR) || lpMidiHdr == 0 || 
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
    
    TRACE(midi, "(%04X);\n", wDevID);
    
    while (MidiInDev[wDevID].lpQueueHdr) {
	MidiInDev[wDevID].lpQueueHdr->dwFlags &= ~MHDR_INQUEUE;
	MidiInDev[wDevID].lpQueueHdr->dwFlags |= MHDR_DONE;
	if (MIDI_NotifyClient(wDevID, MIM_LONGDATA, 
			      (DWORD)MidiInDev[wDevID].lpQueueHdr, dwTime) != MMSYSERR_NOERROR) {
	    WARN(midi, "Couldn't notify client\n");
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
    TRACE(midi, "(%04X);\n", wDevID);
    
    /* FIXME : should test value of wDevID */
    
#ifdef HAVE_OSS
    MidiInDev[wDevID].state = 1;
    MidiInDev[wDevID].startTime = GetTickCount();
    return MMSYSERR_NOERROR;
#else
    return MMSYSERR_NOTENABLED;
#endif /* HAVE_OSS */
}

/**************************************************************************
 *			midStop					[internal]
 */
static DWORD midStop(WORD wDevID)
{
    TRACE(midi, "(%04X);\n", wDevID);
    
    /* FIXME : should test value of wDevID */
    
#ifdef HAVE_OSS
    MidiInDev[wDevID].state = 0;
    return MMSYSERR_NOERROR;
#else /* HAVE_OSS */
    return MMSYSERR_NOTENABLED;
#endif /* HAVE_OSS */
}

/**************************************************************************
 * 			midMessage				[sample driver]
 */
DWORD WINAPI midMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			DWORD dwParam1, DWORD dwParam2)
{
    TRACE(midi, "(%04X, %04X, %08lX, %08lX, %08lX);\n", 
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);
    switch (wMsg) {
    case MIDM_OPEN:
	return midOpen(wDevID,(LPMIDIOPENDESC)dwParam1, dwParam2);
    case MIDM_CLOSE:
	return midClose(wDevID);
    case MIDM_ADDBUFFER:
	return midAddBuffer(wDevID,(LPMIDIHDR)dwParam1, dwParam2);
    case MIDM_PREPARE:
	return midPrepare(wDevID,(LPMIDIHDR)dwParam1, dwParam2);
    case MIDM_UNPREPARE:
	return midUnprepare(wDevID,(LPMIDIHDR)dwParam1, dwParam2);
    case MIDM_GETDEVCAPS:
	return midGetDevCaps(wDevID,(LPMIDIINCAPS16)dwParam1,dwParam2);
    case MIDM_GETNUMDEVS:
	return MIDM_NUMDEVS; 
    case MIDM_RESET:
	return midReset(wDevID);
    case MIDM_START:
	return midStart(wDevID);
    case MIDM_STOP:
	return midStop(wDevID);
    default:
	TRACE(midi, "Unsupported message\n");
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*-----------------------------------------------------------------------*/

#ifdef HAVE_OSS

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
    /* controlers */
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

extern	unsigned char midiFMInstrumentPatches[16 * 128];
extern	unsigned char midiFMDrumsPatches     [16 * 128];

/**************************************************************************
 * 			modFMLoad				[internal]
 */
static int modFMLoad(int dev)
{
    int				i;
    struct sbi_instrument	sbi;
    
    sbi.device = dev;
    sbi.key = FM_PATCH;
    
    memset(sbi.operators + 16, 0, 16);
    for (i = 0; i < 128; i++) {
	sbi.channel = i;
	memcpy(sbi.operators, midiFMInstrumentPatches + i * 16, 16);
	
	if (write(midiSeqFD, (char*)&sbi, sizeof(sbi)) == -1) {
	    WARN(midi, "Couldn't write patch for instrument %d (%d)!\n", sbi.channel, errno);
	    return -1;
	}
    } 
    for (i = 0; i < 128; i++) {
	sbi.channel = 128 + i;
	memcpy(sbi.operators, midiFMDrumsPatches + i * 16, 16);
	
	if (write(midiSeqFD, (char*)&sbi, sizeof(sbi)) == -1) {
	    WARN(midi, "Couldn't write patch for drum %d (%d)!\n", sbi.channel, errno);
	    return -1;
	}
    } 
    return 0;
}

/**************************************************************************
 * 			modFMReset				[internal]
 */
static	void modFMReset(WORD wDevID)
{
    sFMextra*	extra   = (sFMextra*)MidiOutDev[wDevID].lpExtra;
    sVoice* 	voice   = extra->voice;
    sChannel*	channel = extra->channel;
    int		i;
    
    for (i = 0; i < midiOutDevices[wDevID]->wVoices; i++) {
	if (voice[i].status != sVS_UNUSED) {
	    SEQ_STOP_NOTE(wDevID, i, voice[i].note, 64);
	}
	SEQ_KEY_PRESSURE(wDevID, i, 127, 0);
	SEQ_CONTROL(wDevID, i, SEQ_VOLMODE, VOL_METHOD_LINEAR);
	voice[i].note = 0;
	voice[i].channel = -1;
	voice[i].cntMark = 0;
	voice[i].status = sVS_UNUSED;
    }
    for (i = 0; i < 16; i++) {
	channel[i].program = 0;
	channel[i].bender = 8192;
	channel[i].benderRange = 2;
	channel[i].bank = 0;
	channel[i].volume = 127;
	channel[i].balance = 64;
	channel[i].expression = 0;	
	channel[i].sustain = 0;	
    }
    extra->counter = 0;
    extra->drumSetMask = 1 << 9; /* channel 10 is normally drums, sometimes 16 also */
    SEQ_DUMPBUF();
}

#define		IS_DRUM_CHANNEL(_xtra, _chn)	((_xtra)->drumSetMask & (1 << (_chn)))

#endif /* HAVE_OSS */

/**************************************************************************
 * 				modGetDevCaps			[internal]
 */
static DWORD modGetDevCaps(WORD wDevID, LPMIDIOUTCAPS16 lpCaps, 
			   DWORD dwSize)
{
    TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpCaps, dwSize);
    if (wDevID == (WORD) MIDI_MAPPER) { 
	lpCaps->wMid = 0x00FF; 	/* Manufac ID */
	lpCaps->wPid = 0x0001; 	/* Product ID */
	lpCaps->vDriverVersion = 0x001; /* Product Version */
	strcpy(lpCaps->szPname, "MIDI Mapper (not functional yet)");
	/* FIXME Does it make any difference ? */
	lpCaps->wTechnology = MOD_FMSYNTH; 
	lpCaps->wVoices     = 14;       /* FIXME */
	lpCaps->wNotes      = 14;       /* FIXME */
	/* FIXME Does it make any difference ? */
	lpCaps->dwSupport   = MIDICAPS_VOLUME|MIDICAPS_LRVOLUME; 
    } else {
	LPMIDIOUTCAPS16	tmplpCaps;
	
	if (wDevID >= MODM_NUMDEVS) {
	    TRACE(midi, "MAX_MIDIOUTDRV reached !\n");
	    return MMSYSERR_BADDEVICEID;
	}
	/* FIXME There is a way to do it so easily, but I'm too
	 * sleepy to think and I want to test
	 */
	
	tmplpCaps = midiOutDevices[wDevID];
	lpCaps->wMid = tmplpCaps->wMid;  
	lpCaps->wPid = tmplpCaps->wPid;  
	lpCaps->vDriverVersion = tmplpCaps->vDriverVersion;  
	strcpy(lpCaps->szPname, tmplpCaps->szPname);    
	lpCaps->wTechnology = tmplpCaps->wTechnology;
	lpCaps->wVoices = tmplpCaps->wVoices;  
	lpCaps->wNotes = tmplpCaps->wNotes;  
	lpCaps->dwSupport = tmplpCaps->dwSupport;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			modOpen					[internal]
 */
static DWORD modOpen(WORD wDevID, LPMIDIOPENDESC lpDesc, DWORD dwFlags)
{
    TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL) {
	WARN(midi, "Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_MIDIOUTDRV) {
	TRACE(midi,"MAX_MIDIOUTDRV reached !\n");
	return MMSYSERR_BADDEVICEID;
    }
    if (MidiOutDev[wDevID].midiDesc != 0) {
	WARN(midi, "device already open !\n");
	return MMSYSERR_ALLOCATED;
    }
    if ((dwFlags & ~CALLBACK_TYPEMASK) != 0) { 
	WARN(midi, "bad dwFlags\n");
	return MMSYSERR_INVALFLAG;
    }
    if (midiOutDevices[wDevID] == NULL) {
	TRACE(midi, "un-allocated wDevID\n");
	return MMSYSERR_BADDEVICEID;
    }
    
#ifdef HAVE_OSS
    MidiOutDev[wDevID].lpExtra = 0;
    
    switch (midiOutDevices[wDevID]->wTechnology) {
    case MOD_FMSYNTH:
	{
	    void*	extra = xmalloc(sizeof(struct sFMextra) + 
					sizeof(struct sVoice) * 
					(midiOutDevices[wDevID]->wVoices - 1));
	    
	    if (extra == 0) {
		WARN(midi, "can't alloc extra data !\n");
		return MMSYSERR_NOMEM;
	    }
	    MidiOutDev[wDevID].lpExtra = extra;
	    if (midiOpenSeq() < 0) {
		MidiOutDev[wDevID].lpExtra = 0;
		free(extra);
		return MMSYSERR_ERROR;
	    }
	    if (modFMLoad(wDevID) < 0) {
		midiCloseSeq();
		MidiOutDev[wDevID].lpExtra = 0;
		free(extra);
		return MMSYSERR_ERROR;
	    }
	    modFMReset(wDevID);
	}
	break;
    case MOD_MIDIPORT:
	if (midiOpenSeq() < 0) {
	    return MMSYSERR_ALLOCATED;
	}
	break;
    default:
	WARN(midi,"Technology not supported (yet) %d !\n", 
	     midiOutDevices[wDevID]->wTechnology);
	return MMSYSERR_NOTENABLED;
    }
#else /* HAVE_OSS */
    {	
	int	midi = open (MIDI_DEV, O_WRONLY, 0);
	MidiOutDev[wDevID].unixdev = 0;
	if (midi == -1) {
	    WARN(midi, "can't open !\n");
	    return MMSYSERR_ALLOCATED;
	}
	MidiOutDev[wDevID].unixdev = midi;
    }
#endif /* HAVE_OSS */	
    
    MidiOutDev[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
    switch (MidiOutDev[wDevID].wFlags) {
    case DCB_NULL:
	TRACE(midi,"CALLBACK_NULL !\n");
	break;
    case DCB_WINDOW:
	TRACE(midi, "CALLBACK_WINDOW !\n");
	break;
    case DCB_TASK:
	TRACE(midi, "CALLBACK_TASK !\n");
	break;
    case DCB_FUNCTION:
	TRACE(midi, "CALLBACK_FUNCTION !\n");
	break;
    }
    MidiOutDev[wDevID].lpQueueHdr = NULL;
    MidiOutDev[wDevID].dwTotalPlayed = 0;
    MidiOutDev[wDevID].bufsize = 0x3FFF;
    MidiOutDev[wDevID].midiDesc = lpDesc;
    
    if (MIDI_NotifyClient(wDevID, MOM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN(midi,"can't notify client !\n");
	return MMSYSERR_INVALPARAM;
    }
    TRACE(midi, "Succesful !\n");
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 * 			modClose				[internal]
 */
static DWORD modClose(WORD wDevID)
{
    int	ret = MMSYSERR_NOERROR;

    TRACE(midi, "(%04X);\n", wDevID);
    
    if (MidiOutDev[wDevID].midiDesc == 0) {
	WARN(midi, "device not opened !\n");
	return MMSYSERR_ERROR;
    }
    /* FIXME: should test that no pending buffer is still in the queue for
     * playing */
    
#ifdef HAVE_OSS	
    if (midiSeqFD == -1) {
	WARN(midi,"can't close !\n");
	return MMSYSERR_ERROR;
    }
    
    switch (midiOutDevices[wDevID]->wTechnology) {
    case MOD_FMSYNTH:
    case MOD_MIDIPORT:
	midiCloseSeq();
	break;
    default:
	WARN(midi,"Technology not supported (yet) %d !\n", 
	     midiOutDevices[wDevID]->wTechnology);
	return MMSYSERR_NOTENABLED;
    }
    
    if (MidiOutDev[wDevID].lpExtra != 0) {
	free(MidiOutDev[wDevID].lpExtra);
	MidiOutDev[wDevID].lpExtra = 0;
    }
#else
    if (MidiOutDev[wDevID].unixdev == 0) {
	WARN(midi,"can't close !\n");
	return MMSYSERR_NOTENABLED;
    }
    close(MidiOutDev[wDevID].unixdev);
    MidiOutDev[wDevID].unixdev = 0;
#endif /* HAVE_OSS */
    
    MidiOutDev[wDevID].bufsize = 0;
    if (MIDI_NotifyClient(wDevID, MOM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN(midi,"can't notify client !\n");
	ret = MMSYSERR_INVALPARAM;
    }
    MidiOutDev[wDevID].midiDesc = 0;
    return ret;
}

/**************************************************************************
 * 			modData					[internal]
 */
static DWORD modData(WORD wDevID, DWORD dwParam)
{
    WORD	evt = LOBYTE(LOWORD(dwParam));
    WORD	d1  = HIBYTE(LOWORD(dwParam));
    WORD	d2  = LOBYTE(HIWORD(dwParam));
    
    TRACE(midi, "(%04X, %08lX);\n", wDevID, dwParam);
    
#ifdef HAVE_OSS
    if (midiSeqFD == -1) {
	WARN(midi,"can't play !\n");
	return MIDIERR_NODEVICE;
    }
    switch (midiOutDevices[wDevID]->wTechnology) {
    case MOD_FMSYNTH:
	/* FIXME:
	 *	- chorus depth controller is not used
	 */
	{
	    sFMextra*	extra   = (sFMextra*)MidiOutDev[wDevID].lpExtra;
	    sVoice* 	voice   = extra->voice;
	    sChannel*	channel = extra->channel;
	    int		chn = (evt & 0x0F);
	    int		i, nv;
	    
	    switch (evt & 0xF0) {
	    case MIDI_NOTEOFF:
		for (i = 0; i < midiOutDevices[wDevID]->wVoices; i++) {
				/* don't stop sustained notes */
		    if (voice[i].status == sVS_PLAYING && voice[i].channel == chn && voice[i].note == d1) {
			voice[i].status = sVS_UNUSED;
			SEQ_STOP_NOTE(wDevID, i, d1, d2);
		    }
		}
		break;	
	    case MIDI_NOTEON:
		if (d2 == 0) { /* note off if velocity == 0 */
		    for (i = 0; i < midiOutDevices[wDevID]->wVoices; i++) {
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
		for (i = nv = 0; i < midiOutDevices[wDevID]->wVoices; i++) {
		    if (voice[i].status == sVS_UNUSED || 
			(voice[i].note == d1 && voice[i].channel == chn)) {
			nv = i;
			break;
		    }
		    if (voice[i].cntMark < voice[0].cntMark) {
			nv = i;
		    }
		}
		TRACE(midi, 
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
		for (i = 0; i < midiOutDevices[wDevID]->wVoices; i++) {
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
			for (i = 0; i < midiOutDevices[wDevID]->wVoices; i++) {
			    if (voice[i].status == sVS_PLAYING && voice[i].channel == chn) {
				voice[i].status = sVS_SUSTAINED;
			    }
			}
		    } else {
			for (i = 0; i < midiOutDevices[wDevID]->wVoices; i++) {
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
			    for (i = 0; i < midiOutDevices[wDevID]->wVoices; i++) {
				if (voice[i].channel == chn) {
				    SEQ_BENDER_RANGE(wDevID, i, channel[chn].benderRange);
				}
			    }
			}
			break;
			
		    case 0x7F7F:
			channel[chn].benderRange = 2;
			for (i = 0; i < midiOutDevices[wDevID]->wVoices; i++) {
			    if (voice[i].channel == chn) {
				SEQ_BENDER_RANGE(wDevID, i, channel[chn].benderRange);
			    }
			}
			break;
		    default:
			TRACE(midi, "Data entry: regPmt=0x%02x%02x, nrgPmt=0x%02x%02x with %x\n",
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
		    for (i = 0; i < midiOutDevices[wDevID]->wVoices; i++) {
			if (voice[i].status != sVS_UNUSED && voice[i].channel == chn) {
			    voice[i].status = sVS_UNUSED;
			    SEQ_STOP_NOTE(wDevID, i, voice[i].note, 0);
			}
		    }
		    break;
		case 0x7B: /* all notes off */
		    /* FIXME: I don't know if I have to take care of the channel 
		     * for this control ?
		     */
		    for (i = 0; i < midiOutDevices[wDevID]->wVoices; i++) {
			if (voice[i].status == sVS_PLAYING && voice[i].channel == chn) {
			    voice[i].status = sVS_UNUSED;
			    SEQ_STOP_NOTE(wDevID, i, voice[i].note, 0);
			}
		    }
		    break;	
		default:
		    TRACE(midi, "Dropping MIDI control event 0x%02x(%02x) on channel %d\n", 
			  d1, d2, chn);
		    break;
		}
		break;
	    case MIDI_PGM_CHANGE:
		channel[chn].program = d1;
		break;
	    case MIDI_CHN_PRESSURE:
		for (i = 0; i < midiOutDevices[wDevID]->wVoices; i++) {
		    if (voice[i].status != sVS_UNUSED && voice[i].channel == chn) {
			SEQ_KEY_PRESSURE(wDevID, i, voice[i].note, d1);
		    }
		}
		break;
	    case MIDI_PITCH_BEND:
		channel[chn].bender = (d2 << 7) + d1;
		for (i = 0; i < midiOutDevices[wDevID]->wVoices; i++) {
		    if (voice[i].channel == chn) {
			SEQ_BENDER(wDevID, i, channel[chn].bender);
		    }
		}
		break;
	    case MIDI_SYSTEM_PREFIX:
		switch (evt & 0x0F) {
		case 0x0F: 	/* Reset */
		    modFMReset(wDevID);
		    break; 
		default:
		    WARN(midi, "Unsupported (yet) system event %02x\n", evt & 0x0F);
		}
		break;
	    default:	
		WARN(midi, "Internal error, shouldn't happen (event=%08x)\n", evt & 0xF0);
		return MMSYSERR_NOTENABLED;
	    }
	}
	break;
    case MOD_MIDIPORT:
	{
	    int	dev = wDevID - MODM_NUMFMSYNTHDEVS;
	    if (dev < 0) {
		WARN(midi, "Internal error on devID (%u) !\n", wDevID);
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
		case 0x01:	/* Undefined */
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
	WARN(midi, "Technology not supported (yet) %d !\n", 
	     midiOutDevices[wDevID]->wTechnology);
	return MMSYSERR_NOTENABLED;
    }
    
    SEQ_DUMPBUF();
#else
    if (MidiOutDev[wDevID].unixdev == 0) {
	WARN(midi,"can't play !\n");
	return MIDIERR_NODEVICE;
    }
    {
	WORD	event = LOWORD(dwParam);
	if (write (MidiOutDev[wDevID].unixdev, 
		   &event, sizeof(event)) != sizeof(WORD)) {
	    WARN(midi, "error writting unixdev !\n");
	}
    }
#endif /* HAVE_OSS */
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 *		modLongData					[internal]
 */
static DWORD modLongData(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    int		count;
    
    TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);
    
#ifdef HAVE_OSS
    if (midiSeqFD == -1) {
	WARN(midi,"can't play !\n");
	return MIDIERR_NODEVICE;
    }
#else /* HAVE_OSS */
    if (MidiOutDev[wDevID].unixdev == 0) {
	WARN(midi,"can't play !\n");
	return MIDIERR_NODEVICE;
    }
#endif /* HAVE_OSS */
    
    if (lpMidiHdr->lpData == NULL) 
	return MIDIERR_UNPREPARED;
    if (!(lpMidiHdr->dwFlags & MHDR_PREPARED)) 
	return MIDIERR_UNPREPARED;
    if (lpMidiHdr->dwFlags & MHDR_INQUEUE) 
	return MIDIERR_STILLPLAYING;
    lpMidiHdr->dwFlags &= ~MHDR_DONE;
    lpMidiHdr->dwFlags |= MHDR_INQUEUE;

    TRACE(midi, "dwBytesRecorded %lu !\n", lpMidiHdr->dwBytesRecorded);
    TRACE(midi, "                 %02X %02X %02X %02X\n",
	  ((LPBYTE)(lpMidiHdr->reserved))[0],
	  ((LPBYTE)(lpMidiHdr->reserved))[1],
	  ((LPBYTE)(lpMidiHdr->reserved))[2],
	  ((LPBYTE)(lpMidiHdr->reserved))[3]);
#ifdef HAVE_OSS
    switch (midiOutDevices[wDevID]->wTechnology) {
    case MOD_FMSYNTH:
	/* FIXME: I don't think there is much to do here */
	break;
    case MOD_MIDIPORT:
	if (((LPBYTE)lpMidiHdr->reserved)[0] != 0xF0) {
	    /* Send end of System Exclusive */
	    SEQ_MIDIOUT(wDevID - MODM_NUMFMSYNTHDEVS, 0xF0);
	    TRACE(midi, "Adding missing 0xF0 marker at the begining of "
		  "system exclusive byte stream\n");
	}
	for (count = 0; count < lpMidiHdr->dwBytesRecorded; count++) {
	    SEQ_MIDIOUT(wDevID - MODM_NUMFMSYNTHDEVS, 
			((LPBYTE)lpMidiHdr->reserved)[count]);	
	}
	if (((LPBYTE)lpMidiHdr->reserved)[count - 1] != 0xF7) {
	    /* Send end of System Exclusive */
	    SEQ_MIDIOUT(wDevID - MODM_NUMFMSYNTHDEVS, 0xF7);
	    TRACE(midi, "Adding missing 0xF7 marker at the end of "
		  "system exclusive byte stream\n");
	}
	SEQ_DUMPBUF();
	break;
    default:
	WARN(midi, "Technology not supported (yet) %d !\n", 
	     midiOutDevices[wDevID]->wTechnology);
	return MMSYSERR_NOTENABLED;
    }
#else /* HAVE_OSS */
    {
	LPWORD	ptr = (LPWORD)lpMidiHdr->reserved;
	int		en;
	
	for (count = 0; count < lpMidiHdr->dwBytesRecorded; count++) {
	    if (write(MidiOutDev[wDevID].unixdev, 
		      ptr, sizeof(WORD)) != sizeof(WORD)) 
		break;
	    ptr++;
	}
	
	en = errno;
	TRACE(midi, "after write count = %d\n",count);
	if (count != lpMidiHdr->dwBytesRecorded) {
	    WARN(midi, "error writting unixdev #%d ! (%d != %ld)\n",
		 MidiOutDev[wDevID].unixdev, count, 
		 lpMidiHdr->dwBytesRecorded);
	    TRACE(midi, "\terrno = %d error = %s\n",en,strerror(en));
	    return MMSYSERR_NOTENABLED;
	}
    }
#endif /* HAVE_OSS */
    
    lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
    lpMidiHdr->dwFlags |= MHDR_DONE;
    if (MIDI_NotifyClient(wDevID, MOM_DONE, (DWORD)lpMidiHdr, 0L) != MMSYSERR_NOERROR) {
	WARN(midi,"can't notify client !\n");
	return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			modPrepare				[internal]
 */
static DWORD modPrepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);
    
#ifdef HAVE_OSS
    if (midiSeqFD == -1) {
	WARN(midi,"can't prepare !\n");
	return MMSYSERR_NOTENABLED;
    }
#else /* HAVE_OSS */
    if (MidiOutDev[wDevID].unixdev == 0) {
	WARN(midi,"can't prepare !\n");
	return MMSYSERR_NOTENABLED;
    }
#endif /* HAVE_OSS */
    if (dwSize != sizeof(MIDIHDR) || lpMidiHdr == 0 || 
	lpMidiHdr->lpData == 0 || lpMidiHdr->dwFlags != 0 || 
	lpMidiHdr->dwBufferLength >= 0x10000ul)
	return MMSYSERR_INVALPARAM;
    
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
    TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);

#ifdef HAVE_OSS
    if (midiSeqFD == -1) {
	WARN(midi,"can't unprepare !\n");
	return MMSYSERR_NOTENABLED;
    }
#else /* HAVE_OSS */
    if (MidiOutDev[wDevID].unixdev == 0) {
	WARN(midi,"can't unprepare !\n");
	return MMSYSERR_NOTENABLED;
    }
#endif /* HAVE_OSS */
    if (dwSize != sizeof(MIDIHDR) || lpMidiHdr == 0)
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
    TRACE(midi, "(%04X);\n", wDevID);
    /* FIXME: this function should :
     *	turn off every note, remove sustain on all channels
     *	remove any pending buffers
     */
    return MMSYSERR_NOTENABLED;
}
#endif

/*======================================================================*
 *                  	    MIDI entry points 				*
 *======================================================================*/

/**************************************************************************
 * 				modMessage		[sample driver]
 */
DWORD WINAPI modMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			DWORD dwParam1, DWORD dwParam2)
{
    TRACE(midi, "(%04X, %04X, %08lX, %08lX, %08lX);\n", 
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    switch (wMsg) {
#ifdef SNDCTL_MIDI_INFO
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
	return modGetDevCaps(wDevID,(LPMIDIOUTCAPS16)dwParam1,dwParam2);
    case MODM_GETNUMDEVS:
	return MODM_NUMDEVS;
    case MODM_GETVOLUME:
	return 0;
    case MODM_SETVOLUME:
	return 0;
    case MODM_RESET:
	return modReset(wDevID);
#endif
    default:
	TRACE(midi, "Unsupported message\n");
    }
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				MIDI_DriverProc32	[sample driver]
 */
LONG MIDI_DriverProc32(DWORD dwDevID, HDRVR16 hDriv, DWORD wMsg, 
		       DWORD dwParam1, DWORD dwParam2)
{
    switch (wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return 1;
    case DRV_CLOSE:		return 1;
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBox16(0, "Sample Midi Linux Driver !", "MMLinux Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    default:			
	TRACE(midi, "Sending msg=%lu to default driver proc\n", wMsg);
	return DefDriverProc32(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}

/**************************************************************************
 * 				MIDI_DriverProc16	[sample driver]
 */
LONG MIDI_DriverProc16(DWORD dwDevID, HDRVR16 hDriv, WORD wMsg, 
		       DWORD dwParam1, DWORD dwParam2)
{
    switch (wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return 1;
    case DRV_CLOSE:		return 1;
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBox16(0, "Sample Midi Linux Driver !", "MMLinux Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    default:			
	TRACE(midi, "Sending msg=%u to default driver proc\n", wMsg);
	return DefDriverProc32(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}

/*-----------------------------------------------------------------------*/
