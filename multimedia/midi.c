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
 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "windows.h"
#include "ldt.h"
#include "multimedia.h"
#include "user.h"
#include "driver.h"
#include "mmsystem.h"
#include "xmalloc.h"
#include "debug.h"
#include "callback.h"
#include "options.h"

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

typedef struct {
    DWORD		dwFirst;		/* offset in file of track */
    DWORD		dwLast;			/* number of bytes in file of track */
    DWORD		dwIndex;		/* current index in file (dwFirst <= dwIndex < dwLast) */
    DWORD		dwLength;		/* number of pulses in this track */
    DWORD		dwEventPulse;		/* current pulse # (event) pointed by dwIndex */
    DWORD		dwEventData;		/* current data    (event) pointed by dwIndex */
    WORD		wEventLength;		/* current length  (event) pointed by dwIndex */
    WORD		wStatus : 1,		/* 1 : playing, 0 : done */
	                wTrackNr : 7,
	                wLastCommand : 8;
} MCI_MIDITRACK;

typedef struct {
    int			nUseCount;          	/* Incremented for each shared open          */
    BOOL16		fShareable;         	/* TRUE if first open was shareable          */
    WORD		wNotifyDeviceID;    	/* MCI device ID with a pending notification */
    HANDLE16 		hCallback;         	/* Callback handle for pending notification  */
    HMMIO32		hFile;	            	/* mmio file handle open as Element          */
    WORD		wFormat;
    WORD		nTracks;
    WORD		nDivision;
    DWORD		dwTempo;
    MCI_OPEN_PARMS16 	openParms;
    WORD		dwStatus;
    MCI_MIDITRACK*     	tracks;
    DWORD		dwPulse;
    DWORD		dwPositionMS;
    DWORD		dwStartTicks;
    HLOCAL16		hMidiHdr;
} WINE_MCIMIDI;

static WINE_MIDIIN	MidiInDev [MAX_MIDIINDRV ];
static WINE_MIDIOUT	MidiOutDev[MAX_MIDIOUTDRV];
static WINE_MCIMIDI	MCIMidiDev[MAX_MCIMIDIDRV];

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

/**************************************************************************
 * 				MIDI_ReadByte			[internal]	
 */
static DWORD MIDI_ReadByte(UINT16 wDevID, BYTE *lpbyt)
{
    if (lpbyt != NULL) {
	if (mmioRead32(MCIMidiDev[wDevID].hFile, (HPSTR)lpbyt,
		       (long) sizeof(BYTE)) == (long) sizeof(BYTE)) {
	    return 0;
	}
    }
    WARN(midi, "error reading wDevID=%04X\n", wDevID);
    return MCIERR_INTERNAL;
    
}

/**************************************************************************
 * 				MIDI_ReadWord			[internal]	
 */
static DWORD MIDI_ReadWord(UINT16 wDevID, LPWORD lpw)
{
    BYTE	hibyte, lobyte;

    if (lpw != NULL) {
	if (MIDI_ReadByte(wDevID, &hibyte) == 0) {
	    if (MIDI_ReadByte(wDevID, &lobyte) == 0) {
		*lpw = ((WORD)hibyte << 8) + lobyte;
		return 0;
	    }
	}
    }
    WARN(midi, "error reading wDevID=%04X\n", wDevID);
    return MCIERR_INTERNAL;
}

/**************************************************************************
 * 				MIDI_ReadLong			[internal]	
 */
static DWORD MIDI_ReadLong(UINT16 wDevID, LPDWORD lpdw)
{
    WORD	hiword, loword;

    if (lpdw != NULL) {
	if (MIDI_ReadWord(wDevID, &hiword) == 0) {
	    if (MIDI_ReadWord(wDevID, &loword) == 0) {
		*lpdw = MAKELONG(loword, hiword);
		return 0;
	    }
	}
    }
    WARN(midi, "error reading wDevID=%04X\n", wDevID);
    return MCIERR_INTERNAL;
}

/**************************************************************************
 *  				MIDI_ReadVaryLen		[internal]	
 */
static WORD MIDI_ReadVaryLen(UINT16 wDevID, LPDWORD lpdw)
{
    BYTE	byte;
    DWORD	value = 0;
    WORD	ret = 0;

    if (lpdw == NULL) 
	return MCIERR_INTERNAL;

    do {
	if (MIDI_ReadByte(wDevID, &byte) != 0) {
	    WARN(midi, "error reading wDevID=%04X\n", wDevID);
	    return 0;
	}
	value = (value << 7) + (byte & 0x7F);
	ret++; 
    } while (byte & 0x80);
    *lpdw = value;
    /*
      TRACE(midi, "val=%08lX \n", value);
    */
    return ret;
}

/**************************************************************************
 * 				MIDI_ReadNextEvent		[internal]	
 */
static DWORD	MIDI_ReadNextEvent(UINT16 wDevID, MCI_MIDITRACK* mmt) 
{
    BYTE	b1, b2 = 0, b3;
    WORD	hw = 0;
    DWORD	evtPulse;
    DWORD	evtLength;
    DWORD	tmp;

    if (mmioSeek32(MCIMidiDev[wDevID].hFile, mmt->dwIndex, SEEK_SET) != mmt->dwIndex) {
	WARN(midi, "can't seek at %08lX \n", mmt->dwIndex);
	return MCIERR_INTERNAL;
    }
    evtLength = MIDI_ReadVaryLen(wDevID, &evtPulse) + 1;	/* > 0 */
    MIDI_ReadByte(wDevID, &b1);
    switch (b1) {
    case 0xF0:
    case 0xF7:
	evtLength += MIDI_ReadVaryLen(wDevID, &tmp);
	evtLength += tmp;
	break;
    case 0xFF:
	MIDI_ReadByte(wDevID, &b2);	evtLength++;
	
	evtLength += MIDI_ReadVaryLen(wDevID, &tmp);
	if (evtLength >= 0x10000u) {
	    /* this limitation shouldn't be a problem */
	    WARN(midi, "Ouch !! Implementation limitation to 64k bytes for a MIDI event is overflowed\n");
	    hw = 0xFFFF;
	} else {
	    hw = LOWORD(evtLength);
	}
	evtLength += tmp;
	break;
    default:
	if (b1 & 0x80) { // use running status ?
	    mmt->wLastCommand = b1;
	    MIDI_ReadByte(wDevID, &b2);	evtLength++;
	} else {
	    b2 = b1;
	    b1 = mmt->wLastCommand;
	}
	switch ((b1 >> 4) & 0x07) {
	case 0:	case 1:	case 2: case 3: case 6:
	    MIDI_ReadByte(wDevID, &b3);	evtLength++;
	    hw = b3;
	    break;
	case 4:	case 5:
	    break;
	case 7:
	    WARN(midi, "Strange indeed b1=0x%02x\n", b1);
	}
	break;
    }
    if (mmt->dwIndex + evtLength > mmt->dwLast)
	return MCIERR_INTERNAL;

    mmt->dwEventPulse += evtPulse;
    mmt->dwEventData   = (hw << 16) + (b2 << 8) + b1;
    mmt->wEventLength  = evtLength;

    /*
    TRACE(midi, "[%u] => pulse=%08lx(%08lx), data=%08lx, length=%u\n", 
	  mmt->wTrackNr, mmt->dwEventPulse, evtPulse, 
	  mmt->dwEventData, mmt->wEventLength);
    */
    return 0;
}

/**************************************************************************
 * 				MIDI_ReadMTrk			[internal]	
 */
static DWORD MIDI_ReadMTrk(UINT16 wDevID, MCI_MIDITRACK* mmt)
{
    DWORD	toberead;
    FOURCC	fourcc;

    if (mmioRead32(MCIMidiDev[wDevID].hFile, (HPSTR)&fourcc,
		   (long) sizeof(FOURCC)) != (long) sizeof(FOURCC)) {
	return MCIERR_INTERNAL;
    }

    if (fourcc != mmioFOURCC('M', 'T', 'r', 'k')) {
	WARN(midi, "cannot synchronize on MTrk !\n");
	return MCIERR_INTERNAL;
    }

    if (MIDI_ReadLong(wDevID, &toberead) != 0) {
	return MCIERR_INTERNAL;
    }
    mmt->dwFirst = mmioSeek32(MCIMidiDev[wDevID].hFile, 0, SEEK_CUR); /* >= 0 */
    mmt->dwLast = mmt->dwFirst + toberead;

    /* compute # of pulses in this track */
    mmt->dwIndex = mmt->dwFirst;
    mmt->dwEventPulse = 0;
    while (MIDI_ReadNextEvent(wDevID, mmt) == 0 && 
	   LOWORD(mmt->dwEventData) != 0x2FFF) {	
	mmt->dwIndex += mmt->wEventLength;
    }
    mmt->dwLength = mmt->dwEventPulse;

    TRACE(midi, "Track %u has %lu bytes and %lu pulses\n", 
	  mmt->wTrackNr, toberead, mmt->dwLength);

    /* reset track data */
    mmt->wStatus = 1;	/* ok, playing */
    mmt->dwIndex = mmt->dwFirst;
    mmt->dwEventPulse = 0;

    if (mmioSeek32(MCIMidiDev[wDevID].hFile, 0, SEEK_CUR) != mmt->dwLast) {
	WARN(midi, "Ouch, out of sync seek=%lu track=%lu\n", 
	     mmioSeek32(MCIMidiDev[wDevID].hFile, 0, SEEK_CUR), mmt->dwLast);
	/* position at end of this track, to be ready to read next track */
	mmioSeek32(MCIMidiDev[wDevID].hFile, mmt->dwLast, SEEK_SET);
    }

    return 0;
}

/**************************************************************************
 * 				MIDI_ReadMThd			[internal]	
 */
static DWORD MIDI_ReadMThd(UINT16 wDevID, DWORD dwOffset)
{
    DWORD	toberead;
    FOURCC	fourcc;
    WORD	nt;

    TRACE(midi, "(%04X, %08lX);\n", wDevID, dwOffset);
    if (mmioSeek32(MCIMidiDev[wDevID].hFile, dwOffset, SEEK_SET) != dwOffset) {
	WARN(midi, "can't seek at %08lX begin of 'MThd' \n", dwOffset);
	return MCIERR_INTERNAL;
    }
    if (mmioRead32(MCIMidiDev[wDevID].hFile, (HPSTR)&fourcc,
		   (long) sizeof(FOURCC)) != (long) sizeof(FOURCC))
	return MCIERR_INTERNAL;

    if (fourcc != mmioFOURCC('M', 'T', 'h', 'd')) {
	WARN(midi, "cannot synchronize on MThd !\n");
	return MCIERR_INTERNAL;
    }

    if (MIDI_ReadLong(wDevID, &toberead) != 0 || toberead < 3 * sizeof(WORD))
	return MCIERR_INTERNAL;
    if (MIDI_ReadWord(wDevID, &MCIMidiDev[wDevID].wFormat) != 0)
	return MCIERR_INTERNAL;
    if (MIDI_ReadWord(wDevID, &MCIMidiDev[wDevID].nTracks) != 0)
	return MCIERR_INTERNAL;
    if (MIDI_ReadWord(wDevID, &MCIMidiDev[wDevID].nDivision) != 0)
	return MCIERR_INTERNAL;

    TRACE(midi, "toberead=0x%08lX, wFormat=0x%04X nTracks=0x%04X nDivision=0x%04X\n",
	  toberead, MCIMidiDev[wDevID].wFormat,
	  MCIMidiDev[wDevID].nTracks,
	  MCIMidiDev[wDevID].nDivision);

    if (MCIMidiDev[wDevID].nDivision > 0x8000) {
	WARN(midi, "Handling SMPTE time in MIDI files is not (yet) supported\n"
	           "Please report with MIDI file !\n");
	return MCIERR_INTERNAL;
    }

    switch (MCIMidiDev[wDevID].wFormat) {
    case 0:
	if (MCIMidiDev[wDevID].nTracks != 1) {
	    WARN(midi, "Got type 0 file whose number of track is not 1. Setting it to 1\n");
	    MCIMidiDev[wDevID].nTracks = 1;
	}
	break;
    case 1:
    case 2:
	break;
    default:
	WARN(midi, "Handling MIDI files which format = %d is not (yet) supported\n"
	           "Please report with MIDI file !\n", MCIMidiDev[wDevID].wFormat);
	return MCIERR_INTERNAL;
    }

    if (MCIMidiDev[wDevID].nTracks & 0x8000) {
	/* this shouldn't be a problem... */
	WARN(midi, "Ouch !! Implementation limitation to 32k tracks per MIDI file is overflowed\n");
	MCIMidiDev[wDevID].nTracks = 0x7FFF;
    }
    MCIMidiDev[wDevID].tracks = xmalloc(sizeof(MCI_MIDITRACK) * MCIMidiDev[wDevID].nTracks);

    toberead -= 3 * sizeof(WORD); 
    if (toberead > 0) {
	TRACE(midi, "Size of MThd > 6, skipping %ld extra bytes\n", toberead);
	mmioSeek32(MCIMidiDev[wDevID].hFile, toberead, SEEK_CUR);
    }

    for (nt = 0; nt < MCIMidiDev[wDevID].nTracks; nt++) {
	MCIMidiDev[wDevID].tracks[nt].wTrackNr = nt;
	if (MIDI_ReadMTrk(wDevID, &MCIMidiDev[wDevID].tracks[nt]) != 0) {
	    WARN(midi, "can't read 'MTrk' header \n");
	    return MCIERR_INTERNAL;
	}
    }

    return 0;
}

/**************************************************************************
 * 				MIDI_mciOpen			[internal]	
 */
static DWORD MIDI_mciOpen(UINT16 wDevID, DWORD dwFlags, void* lp, BOOL32 is32)
{
    MIDIOPENDESC 	MidiDesc;
    DWORD		dwRet;
    DWORD		dwDeviceID;

    TRACE(midi, "(%08lX, %p)\n", dwFlags, lp);
     
    if (lp == NULL) return MCIERR_INTERNAL;
    
    if (MCIMidiDev[wDevID].nUseCount > 0) {
	/* The driver already open on this channel */
	/* If the driver was opened shareable before and this open specifies */
	/* shareable then increment the use count */
	if (MCIMidiDev[wDevID].fShareable && (dwFlags & MCI_OPEN_SHAREABLE))
	    ++MCIMidiDev[wDevID].nUseCount;
	else
	    return MCIERR_MUST_USE_SHAREABLE;
    } else {
	MCIMidiDev[wDevID].nUseCount = 1;
	MCIMidiDev[wDevID].fShareable = dwFlags & MCI_OPEN_SHAREABLE;
	MCIMidiDev[wDevID].hMidiHdr = USER_HEAP_ALLOC(sizeof(MIDIHDR));
    }

    if (is32) 	dwDeviceID = ((LPMCI_OPEN_PARMS32A)lp)->wDeviceID;
    else	dwDeviceID = ((LPMCI_OPEN_PARMS16)lp)->wDeviceID;
    
    TRACE(midi, "wDevID=%04X (lpParams->wDeviceID=%08lX)\n", wDevID, dwDeviceID);
    /*	lpParms->wDeviceID = wDevID;*/
    TRACE(midi, "before OPEN_ELEMENT\n");
    if (dwFlags & MCI_OPEN_ELEMENT) {
	LPSTR		lpstrElementName;

	if (is32)	lpstrElementName = ((LPMCI_OPEN_PARMS32A)lp)->lpstrElementName;
	else		lpstrElementName = (LPSTR)PTR_SEG_TO_LIN(((LPMCI_OPEN_PARMS16)lp)->lpstrElementName);

	TRACE(midi, "MCI_OPEN_ELEMENT '%s' !\n", lpstrElementName);
	if (strlen(lpstrElementName) > 0) {
	    MCIMidiDev[wDevID].hFile = mmioOpen32A(lpstrElementName, NULL, 
						   MMIO_ALLOCBUF | MMIO_READWRITE | MMIO_EXCLUSIVE);
	    if (MCIMidiDev[wDevID].hFile == 0) {
		WARN(midi, "can't find file='%s' !\n", lpstrElementName);
		return MCIERR_FILE_NOT_FOUND;
	    }
	} else 
	    MCIMidiDev[wDevID].hFile = 0;
    }
    TRACE(midi, "hFile=%u\n", MCIMidiDev[wDevID].hFile);

    /* should be of same size in all cases */
    memcpy(&MCIMidiDev[wDevID].openParms, lp, sizeof(MCI_OPEN_PARMS16));

    MCIMidiDev[wDevID].wNotifyDeviceID = dwDeviceID;
    MCIMidiDev[wDevID].dwStatus = MCI_MODE_STOP;
    MidiDesc.hMidi = 0;

    if (MCIMidiDev[wDevID].hFile != 0) {
	MMCKINFO	ckMainRIFF;
	DWORD		dwOffset = 0;

	if (mmioDescend(MCIMidiDev[wDevID].hFile, &ckMainRIFF, NULL, 0) != 0) {
	    return MCIERR_INTERNAL;
	}
	TRACE(midi,"ParentChunk ckid=%.4s fccType=%.4s cksize=%08lX \n",
	      (LPSTR)&ckMainRIFF.ckid, (LPSTR)&ckMainRIFF.fccType,
	      ckMainRIFF.cksize);

	if (ckMainRIFF.ckid == mmioFOURCC('R', 'M', 'I', 'D')) {
	    TRACE(midi, "is a 'RMID' file \n");
	    dwOffset = ckMainRIFF.dwDataOffset;
	    FIXME(midi, "Setting #tracks for RMID to 1: is this correct ?\n");
	    MCIMidiDev[wDevID].nTracks = 1;
	}
	if (MIDI_ReadMThd(wDevID, dwOffset) != 0) {
	    WARN(midi, "can't read 'MThd' header \n");
	    return MCIERR_INTERNAL;
	}

	TRACE(midi, "Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX \n",
	      (LPSTR)&ckMainRIFF.ckid, (LPSTR)&ckMainRIFF.fccType,
	      ckMainRIFF.cksize);
    }
    
    dwRet = modMessage(wDevID, MODM_OPEN, 0, (DWORD)&MidiDesc, CALLBACK_NULL);
    /*	dwRet = midMessage(wDevID, MIDM_OPEN, 0, (DWORD)&MidiDesc, CALLBACK_NULL);*/
    
    return 0;
}

static	DWORD	MIDI_ConvertPulseToMS(UINT16 wDevID, DWORD pulse)
{
    DWORD	ret = (DWORD)((double)pulse * ((double)MCIMidiDev[wDevID].dwTempo / 1000) /	
			      (double)MCIMidiDev[wDevID].nDivision);

    /*
    TRACE(midi, "pulse=%lu tempo=%lu division=%u => ms=%lu\n", 
          pulse, MCIMidiDev[wDevID].dwTempo, MCIMidiDev[wDevID].nDivision, ret);
    */
    return ret;
}

/**************************************************************************
 * 				MIDI_mciStop			[internal]
 */
static DWORD MIDI_mciStop(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wDevID >= MAX_MCIMIDIDRV || MCIMidiDev[wDevID].nUseCount == 0) {
	WARN(midi, "Invalid wDevID=%u\n", wDevID);
	return MCIERR_INVALID_DEVICE_ID;
    }

    MCIMidiDev[wDevID].dwStatus = MCI_MODE_STOP;
    TRACE(midi, "MCIMidiDev[wDevID].dwStatus=%d\n", MCIMidiDev[wDevID].dwStatus);
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE(midi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
			MCIMidiDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				MIDI_mciClose			[internal]
 */
static DWORD MIDI_mciClose(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD		dwRet;
    
    TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wDevID >= MAX_MCIMIDIDRV || MCIMidiDev[wDevID].nUseCount == 0) {
	WARN(midi, "Invalid wDevID=%u\n", wDevID);
	return MCIERR_INVALID_DEVICE_ID;
    }

    if (MCIMidiDev[wDevID].dwStatus != MCI_MODE_STOP) {
	MIDI_mciStop(wDevID, MCI_WAIT, lpParms);
    }

    MCIMidiDev[wDevID].dwStatus = MCI_MODE_STOP;
    MCIMidiDev[wDevID].nUseCount--;
    if (MCIMidiDev[wDevID].nUseCount == 0) {
	if (MCIMidiDev[wDevID].hFile != 0) {
	    mmioClose32(MCIMidiDev[wDevID].hFile, 0);
	    MCIMidiDev[wDevID].hFile = 0;
	    TRACE(midi, "hFile closed !\n");
	}
	USER_HEAP_FREE(MCIMidiDev[wDevID].hMidiHdr);
	free(MCIMidiDev[wDevID].tracks);
	dwRet = modMessage(wDevID, MODM_CLOSE, 0, 0L, 0L);
	if (dwRet != MMSYSERR_NOERROR) return MCIERR_INTERNAL;
	/*
	  dwRet = midMessage(wDevID, MIDM_CLOSE, 0, 0L, 0L);
	  if (dwRet != MMSYSERR_NOERROR) return MCIERR_INTERNAL;
	*/
    }
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE(midi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
			MCIMidiDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				MIDI_mciPlay			[internal]
 */
static DWORD MIDI_mciPlay(UINT16 wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
    DWORD		start = 0, end = 0xFFFFFFFFul;
    DWORD		dwRet;
    WORD		nt, cnt;

    TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wDevID >= MAX_MCIMIDIDRV || MCIMidiDev[wDevID].nUseCount == 0) {
	WARN(midi, "Invalid wDevID=%u\n", wDevID);
	return MCIERR_INVALID_DEVICE_ID;
    }

    if (MCIMidiDev[wDevID].hFile == 0) {
	WARN(midi, "Cannot play : can't find file='%08lx' !\n", 
	     (DWORD)MCIMidiDev[wDevID].openParms.lpstrElementName);
	return MCIERR_FILE_NOT_FOUND;
    }

    if (MCIMidiDev[wDevID].dwStatus != MCI_MODE_STOP) {
	WARN(midi, "Cannot play : device is not stopped !\n");
	return MCIERR_INTERNAL;
    }

    if (lpParms && (dwFlags & MCI_FROM)) {
	start = lpParms->dwFrom; 
	FIXME(midi, "MCI_FROM=%lu \n", start);
    }
    if (lpParms && (dwFlags & MCI_TO)) {
	end = lpParms->dwTo;
	FIXME(midi, "MCI_TO=%lu \n", end);
    }

    if (!(dwFlags & MCI_WAIT)) {	
	/** FIXME: I'm not 100% sure that wNotifyDeviceID is the right value in all cases ??? */
	return mciSendCommandAsync32(MCIMidiDev[wDevID].wNotifyDeviceID, MCI_PLAY, dwFlags, (DWORD)lpParms);
    }

    /* init tracks */
    for (nt = 0; nt < MCIMidiDev[wDevID].nTracks; nt++) {
	MCI_MIDITRACK*	mmt = &MCIMidiDev[wDevID].tracks[nt];

	mmt->wStatus = 1;	/* ok, playing */
	mmt->dwIndex = mmt->dwFirst;
	if (MCIMidiDev[wDevID].wFormat == 2 && nt > 0) {
	    mmt->dwEventPulse = MCIMidiDev[wDevID].tracks[nt - 1].dwLength;
	} else {
	    mmt->dwEventPulse = 0;
	}
	MIDI_ReadNextEvent(wDevID, mmt); /* FIXME == 0 */
    }
	
    MCIMidiDev[wDevID].dwPulse = 0;
    MCIMidiDev[wDevID].dwPositionMS = 0;
    MCIMidiDev[wDevID].dwStartTicks = GetTickCount();
    MCIMidiDev[wDevID].dwTempo = 500000;
    MCIMidiDev[wDevID].dwStatus = MCI_MODE_PLAY;

    while (MCIMidiDev[wDevID].dwStatus != MCI_MODE_STOP) {
	MCI_MIDITRACK*	mmt;
	DWORD		hiPulse;

	TRACE(midi, "MCIMidiDev[wDevID].dwStatus=%p %d\n",
	      &MCIMidiDev[wDevID].dwStatus, MCIMidiDev[wDevID].dwStatus);

	while (MCIMidiDev[wDevID].dwStatus == MCI_MODE_PAUSE);

	/* find first event */
	hiPulse = 0xFFFFFFFFul;
	cnt = 0xFFFFu;
	for (nt = 0; nt < MCIMidiDev[wDevID].nTracks; nt++) {
	    mmt = &MCIMidiDev[wDevID].tracks[nt];
	    
	    if (mmt->wStatus == 0)
		continue;
	    if (mmt->dwEventPulse < hiPulse) {
		hiPulse = mmt->dwEventPulse;
		cnt = nt;
	    }
	}
	if (cnt == 0xFFFFu)  /* no more event on tracks */
	    break;

	mmt = &MCIMidiDev[wDevID].tracks[cnt];
	if (hiPulse > MCIMidiDev[wDevID].dwPulse) {
	    DWORD	togo = MCIMidiDev[wDevID].dwStartTicks + 
		MIDI_ConvertPulseToMS(wDevID, hiPulse);
	    DWORD	tc = GetTickCount();

	    TRACE(midi, "Pulses hi=0x%08lx <> cur=0x%08lx\n", hiPulse, MCIMidiDev[wDevID].dwPulse);
	    TRACE(midi, "Wait til %08lx -> %08lx ms\n", 	
		  tc - MCIMidiDev[wDevID].dwStartTicks, 
		  togo - MCIMidiDev[wDevID].dwStartTicks);
	    if (tc < togo)	Sleep(togo - tc);
	    MCIMidiDev[wDevID].dwPositionMS += MIDI_ConvertPulseToMS(wDevID, hiPulse - MCIMidiDev[wDevID].dwPulse);
	    MCIMidiDev[wDevID].dwPulse = hiPulse;
	}
	    
	switch (LOBYTE(LOWORD(mmt->dwEventData))) {
	case 0xF0:
	case 0xF7:	/* sysex events */
	    FIXME(midi, "Not handling SysEx events (yet)\n");
	    break;
	case 0xFF:
	    /* position after meta data header */
	    mmioSeek32(MCIMidiDev[wDevID].hFile, mmt->dwIndex + HIWORD(mmt->dwEventData), SEEK_SET);
	    switch (HIBYTE(LOWORD(mmt->dwEventData))) {
	    case 0x00: /* 16-bit sequence number */
		if (TRACE_ON(midi)) {
		    WORD	twd;

		    MIDI_ReadWord(wDevID, &twd);	/* == 0 */
		    TRACE(midi, "Got sequence number %u\n", twd);
		}
		break;
	    case 0x01: /* any text */
	    case 0x02: /* Copyright Message text */
	    case 0x03: /* Sequence/Track Name text */
	    case 0x04: /* Instrument Name text */
	    case 0x05: /* Lyric text */
	    case 0x06: /* Marker text */
	    case 0x07: /* Cue-point text */
		if (TRACE_ON(midi)) {
		    char	buf[1024];
		    WORD	len = mmt->wEventLength - HIWORD(mmt->dwEventData);
		    static	char*	info[8] = {"", "Text", "Copyright", "Seq/Trk name", 
						   "Instrument", "Lyric", "Marker", "Cue-point"};
		    WORD	idx = HIBYTE(LOWORD(mmt->dwEventData));

		    if (len >= sizeof(buf)) {
			WARN(midi, "Buffer for text is too small (%d bytes, when %u are needed)\n", sizeof(buf) - 1, len);
			len = sizeof(buf) - 1;
		    }
		    if (mmioRead32(MCIMidiDev[wDevID].hFile, (HPSTR)buf, len) == len) {
			buf[len] = 0;	/* end string in case */
			TRACE(midi, "%s => \"%s\"\n", (idx < 8 ) ? info[idx] : "", buf);
		    } else {
			WARN(midi, "Couldn't read data for %s\n", (idx < 8 ) ? info[idx] : "");
		    }
		}
		break;
	    case 0x20: 
		/* MIDI channel
		 * cc 
		 */
		if (TRACE_ON(midi)) {
		    BYTE	bt;
		    
		    MIDI_ReadByte(wDevID, &bt);	/* == 0 */
		    FIXME(midi, "NIY: MIDI channel %u\n", bt);

		} else {
		    FIXME(midi, "NIY: MIDI channel\n");
		}
		break;
	    case 0x21:
		/* MIDI port
		 * pp
		 */
		if (TRACE_ON(midi)) {
		    BYTE	bt;
		    
		    MIDI_ReadByte(wDevID, &bt);	/* == 0 */
		    FIXME(midi, "NIY: MIDI port %u\n", bt);
		} else {
		    FIXME(midi, "NIY: MIDI port\n");
		}
		break;
	    case 0x2F: /* end of track */
		mmt->wStatus = 0;
		break;
	    case 0x51:/* set tempo */
		/* Tempo is expressed in µ-seconds per midi quarter note
		 * for format 1 MIDI files, this can only be present on track #0
		 */
		if (mmt->wTrackNr != 0 && MCIMidiDev[wDevID].wFormat == 1) {
		    WARN(midi, "For format #1 MIDI files, tempo can only be changed on track #0 (%u)\n", mmt->wTrackNr);
		} else { 
		    BYTE	tbt;
		    
		    MIDI_ReadByte(wDevID, &tbt);	MCIMidiDev[wDevID].dwTempo  = ((DWORD)tbt) << 16;
		    MIDI_ReadByte(wDevID, &tbt);	MCIMidiDev[wDevID].dwTempo |= ((DWORD)tbt) << 8;
		    MIDI_ReadByte(wDevID, &tbt);	MCIMidiDev[wDevID].dwTempo |= ((DWORD)tbt) << 0;
		    TRACE(midi, "Setting tempo to %ld (BPM=%ld)\n", MCIMidiDev[wDevID].dwTempo, 60000000l / MCIMidiDev[wDevID].dwTempo);
		}
		
		break;
	    case 0x54: /* (hour) (min) (second) (frame) (fractional-frame) - SMPTE track start */
		FIXME(midi, "NIY: SMPTE track start\n");
		break;
	    case 0x58: 
		if (TRACE_ON(midi)) {
		    BYTE	num, den, cpmc, _32npqn;
		    
		    MIDI_ReadByte(wDevID, &num);	
		    MIDI_ReadByte(wDevID, &den);	/* to notate e.g. 6/8 */
		    MIDI_ReadByte(wDevID, &cpmc);	/* number of MIDI clocks per metronome click */ 
		    MIDI_ReadByte(wDevID, &_32npqn);	/* number of notated 32nd notes per MIDI quarter note */

		    TRACE(midi, "%u/%u, clock per metronome click=%u, 32nd notes by 1/4 note=%u\n", num, 1 << den, cpmc, _32npqn);
		}
		break;
	    case 0x59: 
		if (TRACE_ON(midi)) {
		    BYTE	sf, mm;

		    MIDI_ReadByte(wDevID, &sf);		MIDI_ReadByte(wDevID, &mm);	

		    if (sf >= 0x80) 	TRACE(midi, "%d flats\n", -(char)sf);	 
		    else if (sf > 0) 	TRACE(midi, "%d sharps\n", (char)sf);	 
		    else 		TRACE(midi, "Key of C\n");
		    TRACE(midi, "Mode: %s\n", (mm = 0) ? "major" : "minor");
		}
		break;
	    default:
		WARN(midi, "Unknown MIDI meta event %02x. Skipping...\n", HIBYTE(LOWORD(mmt->dwEventData)));
		break;
	    }
	    break;
	default:		
	    dwRet = modMessage(wDevID, MODM_DATA, 0, mmt->dwEventData, 0);
	}
	mmt->dwIndex += mmt->wEventLength;
	if (mmt->dwIndex < mmt->dwFirst || mmt->dwIndex >= mmt->dwLast) {
	    mmt->wStatus = 0;
	} 
	if (mmt->wStatus) {	
	    MIDI_ReadNextEvent(wDevID, mmt);
	}
    }

    /* stop all notes */
#ifdef HAVE_OSS
    modMessage(wDevID, MODM_DATA, 0, (MIDI_CTL_CHANGE << 8) | 0x78, 0);
#endif
    MCIMidiDev[wDevID].dwStatus = MCI_MODE_STOP;
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE(midi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
			MCIMidiDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				MIDI_mciRecord			[internal]
 */
static DWORD MIDI_mciRecord(UINT16 wDevID, DWORD dwFlags, LPMCI_RECORD_PARMS lpParms)
{
    int			start, end;
    LPMIDIHDR		lpMidiHdr;
    DWORD		dwRet;
    
    TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wDevID >= MAX_MCIMIDIDRV || MCIMidiDev[wDevID].nUseCount == 0) {
	WARN(midi, "Invalid wDevID=%u\n", wDevID);
	return MCIERR_INVALID_DEVICE_ID;
    }

    if (MCIMidiDev[wDevID].hFile == 0) {
	WARN(midi, "can't find file='%08lx' !\n", 
	     (DWORD)MCIMidiDev[wDevID].openParms.lpstrElementName);
	return MCIERR_FILE_NOT_FOUND;
    }
    start = 1; 		end = 99999;
    if (lpParms && (dwFlags & MCI_FROM)) {
	start = lpParms->dwFrom; 
	TRACE(midi, "MCI_FROM=%d \n", start);
    }
    if (lpParms && (dwFlags & MCI_TO)) {
	end = lpParms->dwTo;
	TRACE(midi, "MCI_TO=%d \n", end);
    }
    lpMidiHdr = USER_HEAP_LIN_ADDR(MCIMidiDev[wDevID].hMidiHdr);
    lpMidiHdr->lpData = (LPSTR) xmalloc(1200);
    lpMidiHdr->dwBufferLength = 1024;
    lpMidiHdr->dwUser = 0L;
    lpMidiHdr->dwFlags = 0L;
    dwRet = midMessage(wDevID, MIDM_PREPARE, 0, (DWORD)lpMidiHdr, sizeof(MIDIHDR));
    TRACE(midi, "after MIDM_PREPARE \n");
    MCIMidiDev[wDevID].dwStatus = MCI_MODE_RECORD;
    while (MCIMidiDev[wDevID].dwStatus != MCI_MODE_STOP) {
	TRACE(midi, "MCIMidiDev[wDevID].dwStatus=%p %d\n",
	      &MCIMidiDev[wDevID].dwStatus, MCIMidiDev[wDevID].dwStatus);
	lpMidiHdr->dwBytesRecorded = 0;
	dwRet = midMessage(wDevID, MIDM_START, 0, 0L, 0L);
	TRACE(midi, "after MIDM_START lpMidiHdr=%p dwBytesRecorded=%lu\n",
	      lpMidiHdr, lpMidiHdr->dwBytesRecorded);
	if (lpMidiHdr->dwBytesRecorded == 0) break;
    }
    TRACE(midi, "before MIDM_UNPREPARE \n");
    dwRet = midMessage(wDevID, MIDM_UNPREPARE, 0, (DWORD)lpMidiHdr, sizeof(MIDIHDR));
    TRACE(midi, "after MIDM_UNPREPARE \n");
    if (lpMidiHdr->lpData != NULL) {
	free(lpMidiHdr->lpData);
	lpMidiHdr->lpData = NULL;
    }
    MCIMidiDev[wDevID].dwStatus = MCI_MODE_STOP;
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE(midi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
			MCIMidiDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				MIDI_mciPause			[internal]
 */
static DWORD MIDI_mciPause(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wDevID >= MAX_MCIMIDIDRV || MCIMidiDev[wDevID].nUseCount == 0) {
	WARN(midi, "Invalid wDevID=%u\n", wDevID);
	return MCIERR_INVALID_DEVICE_ID;
    }

    if (MCIMidiDev[wDevID].dwStatus == MCI_MODE_PLAY) {
	MCIMidiDev[wDevID].dwStatus = MCI_MODE_PAUSE;
    } 
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE(midi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
			MCIMidiDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
	
    return 0;
}

/**************************************************************************
 * 				MIDI_mciResume			[internal]
 */
static DWORD MIDI_mciResume(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wDevID >= MAX_MCIMIDIDRV || MCIMidiDev[wDevID].nUseCount == 0) {
	WARN(midi, "Invalid wDevID=%u\n", wDevID);
	return MCIERR_INVALID_DEVICE_ID;
    }

    if (MCIMidiDev[wDevID].dwStatus == MCI_MODE_PAUSE) {
	MCIMidiDev[wDevID].dwStatus = MCI_MODE_PLAY;
    } 
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE(midi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
			MCIMidiDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				MIDI_mciSet			[internal]
 */
static DWORD MIDI_mciSet(UINT16 wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
{
    TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) return MCIERR_INTERNAL;

    if (wDevID >= MAX_MCIMIDIDRV || MCIMidiDev[wDevID].nUseCount == 0) {
	WARN(midi, "Invalid wDevID=%u\n", wDevID);
	return MCIERR_INVALID_DEVICE_ID;
    }

    TRACE(midi, "dwTimeFormat=%08lX\n", lpParms->dwTimeFormat);
    TRACE(midi, "dwAudio=%08lX\n", lpParms->dwAudio);

    if (dwFlags & MCI_SET_TIME_FORMAT) {
	switch (lpParms->dwTimeFormat) {
	case MCI_FORMAT_MILLISECONDS:
	    TRACE(midi, "MCI_FORMAT_MILLISECONDS !\n");
	    break;
	case MCI_FORMAT_BYTES:
	    TRACE(midi, "MCI_FORMAT_BYTES !\n");
	    break;
	case MCI_FORMAT_SAMPLES:
	    TRACE(midi, "MCI_FORMAT_SAMPLES !\n");
	    break;
	default:
	    WARN(midi, "bad time format !\n");
	    return MCIERR_BAD_TIME_FORMAT;
	}
    }
    if (dwFlags & MCI_SET_VIDEO) {
	TRACE(midi, "No support for video !\n");
	return MCIERR_UNSUPPORTED_FUNCTION;
    }
    if (dwFlags & MCI_SET_DOOR_OPEN) {
	TRACE(midi, "No support for door open !\n");
	return MCIERR_UNSUPPORTED_FUNCTION;
    }
    if (dwFlags & MCI_SET_DOOR_CLOSED) {
	TRACE(midi, "No support for door close !\n");
	return MCIERR_UNSUPPORTED_FUNCTION;
    }

    if (dwFlags && MCI_SET_ON) {
	TRACE(midi, "MCI_SET_ON !\n");
	if (dwFlags & MCI_SET_AUDIO) {
	    TRACE(midi, "MCI_SET_AUDIO !\n");
	}
	if (dwFlags && MCI_SET_AUDIO_LEFT)
	    TRACE(midi, "MCI_SET_AUDIO_LEFT !\n");
	if (dwFlags && MCI_SET_AUDIO_RIGHT)
	    TRACE(midi, "MCI_SET_AUDIO_RIGHT !\n");
    }
    if (dwFlags & MCI_SET_OFF) {
	TRACE(midi, "MCI_SET_OFF !\n");
	if (dwFlags & MCI_SET_AUDIO) {
	    TRACE(midi, "MCI_SET_AUDIO !\n");
	}    
	if (dwFlags && MCI_SET_AUDIO_LEFT)
	    TRACE(midi, "MCI_SET_AUDIO_LEFT !\n");
	if (dwFlags && MCI_SET_AUDIO_RIGHT)
	    TRACE(midi, "MCI_SET_AUDIO_RIGHT !\n");
    }
    if (dwFlags & MCI_SEQ_SET_MASTER)
	TRACE(midi, "MCI_SEQ_SET_MASTER !\n");
    if (dwFlags & MCI_SEQ_SET_SLAVE)
	TRACE(midi, "MCI_SEQ_SET_SLAVE !\n");
    if (dwFlags & MCI_SEQ_SET_OFFSET)
	TRACE(midi, "MCI_SEQ_SET_OFFSET !\n");
    if (dwFlags & MCI_SEQ_SET_PORT)
	TRACE(midi, "MCI_SEQ_SET_PORT !\n");
    if (dwFlags & MCI_SEQ_SET_TEMPO)
	TRACE(midi, "MCI_SEQ_SET_TEMPO !\n");
    return 0;
}

/**************************************************************************
 * 				MIDI_mciStatus			[internal]
 */
static DWORD MIDI_mciStatus(UINT16 wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
    TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) return MCIERR_INTERNAL;

    if (wDevID >= MAX_MCIMIDIDRV || MCIMidiDev[wDevID].nUseCount == 0) {
	WARN(midi, "Invalid wDevID=%u\n", wDevID);
	return MCIERR_INVALID_DEVICE_ID;
    }

    if (dwFlags & MCI_STATUS_ITEM) {
	switch (lpParms->dwItem) {
	case MCI_STATUS_CURRENT_TRACK:
	    TRACE(midi, "MCI_STATUS_CURRENT_TRACK !\n");
	    lpParms->dwReturn = 1;
	    break;
	case MCI_STATUS_LENGTH:
	    TRACE(midi, "MCI_STATUS_LENGTH !\n");
	    {
		WORD	nt;
		DWORD	ret = 0;

		for (nt = 0; nt < MCIMidiDev[wDevID].nTracks; nt++) {
		    if (MCIMidiDev[wDevID].wFormat == 2) {
			ret += MCIMidiDev[wDevID].tracks[nt].dwLength;
		    } else {
			if (MCIMidiDev[wDevID].tracks[nt].dwLength > ret)
			    ret = MCIMidiDev[wDevID].tracks[nt].dwLength;
		    }
		}
		lpParms->dwReturn = MIDI_ConvertPulseToMS(wDevID, ret);
	    }
	    break;
	case MCI_STATUS_MODE:
	    TRACE(midi, "MCI_STATUS_MODE !\n");
	    lpParms->dwReturn = MCIMidiDev[wDevID].dwStatus;
	    break;
	case MCI_STATUS_MEDIA_PRESENT:
	    TRACE(midi, "MCI_STATUS_MEDIA_PRESENT !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_STATUS_NUMBER_OF_TRACKS:
	    TRACE(midi, "MCI_STATUS_NUMBER_OF_TRACKS !\n");
	    lpParms->dwReturn = 1; /* FIXME: except in format 2 */
	    break;
	case MCI_STATUS_POSITION:
	    TRACE(midi, "MCI_STATUS_POSITION !\n");
	    if (dwFlags & MCI_STATUS_START) {
		lpParms->dwReturn = 0;
	    } else {
		lpParms->dwReturn = MCIMidiDev[wDevID].dwPositionMS;
	    }
	    break;
	case MCI_STATUS_READY:
	    TRACE(midi, "MCI_STATUS_READY !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_STATUS_TIME_FORMAT:
	    TRACE(midi, "MCI_STATUS_TIME_FORMAT !\n");
	    lpParms->dwReturn = MCI_FORMAT_MILLISECONDS;
	    break;
	case MCI_SEQ_STATUS_DIVTYPE:
	    TRACE(midi, "MCI_SEQ_STATUS_DIVTYPE !\n");
	    lpParms->dwReturn = MCI_SEQ_DIV_PPQN;
	    break;
	case MCI_SEQ_STATUS_MASTER:
	    TRACE(midi, "MCI_SEQ_STATUS_MASTER !\n");
	    lpParms->dwReturn = 0;
	    break;
	case MCI_SEQ_STATUS_SLAVE:
	    TRACE(midi, "MCI_SEQ_STATUS_SLAVE !\n");
	    lpParms->dwReturn = 0;
	    break;
	case MCI_SEQ_STATUS_OFFSET:
	    TRACE(midi, "MCI_SEQ_STATUS_OFFSET !\n");
	    lpParms->dwReturn = 0;
	    break;
	case MCI_SEQ_STATUS_PORT:
	    TRACE(midi, "MCI_SEQ_STATUS_PORT !\n");
	    lpParms->dwReturn = 0;
	    break;
	case MCI_SEQ_STATUS_TEMPO:
	    TRACE(midi, "MCI_SEQ_STATUS_TEMPO !\n");
	    lpParms->dwReturn = MCIMidiDev[wDevID].dwTempo;
	    break;
	default:
	    WARN(midi, "unknowm command %08lX !\n", lpParms->dwItem);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    } else {
	WARN(midi, "No Status-Item!\n");
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_NOTIFY) {
	TRACE(midi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
			MCIMidiDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				MIDI_mciGetDevCaps		[internal]
 */
static DWORD MIDI_mciGetDevCaps(UINT16 wDevID, DWORD dwFlags, 
				LPMCI_GETDEVCAPS_PARMS lpParms)
{
    TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) return MCIERR_INTERNAL;

    if (wDevID >= MAX_MCIMIDIDRV || MCIMidiDev[wDevID].nUseCount == 0) {
	WARN(midi, "Invalid wDevID=%u\n", wDevID);
	return MCIERR_INVALID_DEVICE_ID;
    }

    if (dwFlags & MCI_GETDEVCAPS_ITEM) {
	switch (lpParms->dwItem) {
	case MCI_GETDEVCAPS_CAN_RECORD:
	    TRACE(midi, "MCI_GETDEVCAPS_CAN_RECORD !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_HAS_AUDIO:
	    TRACE(midi, "MCI_GETDEVCAPS_HAS_AUDIO !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_HAS_VIDEO:
	    TRACE(midi, "MCI_GETDEVCAPS_HAS_VIDEO !\n");
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_DEVICE_TYPE:
	    TRACE(midi, "MCI_GETDEVCAPS_DEVICE_TYPE !\n");
	    lpParms->dwReturn = MCI_DEVTYPE_SEQUENCER;
	    break;
	case MCI_GETDEVCAPS_USES_FILES:
	    TRACE(midi, "MCI_GETDEVCAPS_USES_FILES !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_COMPOUND_DEVICE:
	    TRACE(midi, "MCI_GETDEVCAPS_COMPOUND_DEVICE !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_CAN_EJECT:
	    TRACE(midi, "MCI_GETDEVCAPS_CAN_EJECT !\n");
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_CAN_PLAY:
	    TRACE(midi, "MCI_GETDEVCAPS_CAN_PLAY !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_CAN_SAVE:
	    TRACE(midi, "MCI_GETDEVCAPS_CAN_SAVE !\n");
	    lpParms->dwReturn = FALSE;
	    break;
	default:
	    TRACE(midi, "Unknown capability (%08lx) !\n", lpParms->dwItem);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    } else {
	TRACE(midi, "No GetDevCaps-Item !\n");
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    return 0;
}

/**************************************************************************
 * 				MIDI_mciInfo			[internal]
 */
static DWORD MIDI_mciInfo(UINT16 wDevID, DWORD dwFlags, LPMCI_INFO_PARMS16 lpParms)
{
    TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) return MCIERR_INTERNAL;

    if (wDevID >= MAX_MCIMIDIDRV || MCIMidiDev[wDevID].nUseCount == 0) {
	WARN(midi, "Invalid wDevID=%u\n", wDevID);
	return MCIERR_INVALID_DEVICE_ID;
    }

    lpParms->lpstrReturn = NULL;

    switch (dwFlags) {
    case MCI_INFO_PRODUCT:
	lpParms->lpstrReturn = "Linux Sound System 0.5";
	break;
    case MCI_INFO_FILE:
	lpParms->lpstrReturn = "FileName";
	break;
    default:
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    lpParms->dwRetSize = (lpParms->lpstrReturn != NULL) ? strlen(lpParms->lpstrReturn) : 0;
    return 0;
}

/*-----------------------------------------------------------------------*/

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
		    for (i = 0; i < midiOutDevices[wDevID]->wVoices; i++) {
			if (voice[i].status != sVS_UNUSED && voice[i].channel == chn) {
			    voice[i].status = sVS_UNUSED;
			    SEQ_STOP_NOTE(wDevID, i, voice[i].note, 0);
			}
		    }
		    break;
		case 0x7B: /* all notes off */
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
		    WARN(midi, 
			 "Unsupported (yet) system event %02x\n", 
			 evt & 0x0F);
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

/**************************************************************************
 * 				modMessage		[sample driver]
 */
DWORD WINAPI modMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			DWORD dwParam1, DWORD dwParam2)
{
    TRACE(midi, "(%04X, %04X, %08lX, %08lX, %08lX);\n", 
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    switch (wMsg) {
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
    default:
	TRACE(midi, "Unsupported message\n");
    }
    return MMSYSERR_NOTSUPPORTED;
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
    case MCI_OPEN_DRIVER:	
    case MCI_OPEN:		return MIDI_mciOpen(dwDevID, dwParam1, PTR_SEG_TO_LIN(dwParam2), FALSE);
    case MCI_CLOSE_DRIVER:	
    case MCI_CLOSE:		return MIDI_mciClose(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_PLAY:		return MIDI_mciPlay(dwDevID, dwParam1, (LPMCI_PLAY_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_RECORD:		return MIDI_mciRecord(dwDevID, dwParam1, (LPMCI_RECORD_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_STOP:		return MIDI_mciStop(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_SET:		return MIDI_mciSet(dwDevID, dwParam1, (LPMCI_SET_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_PAUSE:		return MIDI_mciPause(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_RESUME:		return MIDI_mciResume(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_STATUS:		return MIDI_mciStatus(dwDevID, dwParam1, (LPMCI_STATUS_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_GETDEVCAPS:	return MIDI_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_INFO:		return MIDI_mciInfo(dwDevID, dwParam1, (LPMCI_INFO_PARMS16)PTR_SEG_TO_LIN(dwParam2));
    default:			return DefDriverProc16(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
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
    case MCI_OPEN_DRIVER:	
    case MCI_OPEN:		return MIDI_mciOpen(dwDevID, dwParam1, (void*)dwParam2, TRUE);
    case MCI_CLOSE_DRIVER:	
    case MCI_CLOSE:		return MIDI_mciClose(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_PLAY:		return MIDI_mciPlay(dwDevID, dwParam1, (LPMCI_PLAY_PARMS)dwParam2);
    case MCI_RECORD:		return MIDI_mciRecord(dwDevID, dwParam1, (LPMCI_RECORD_PARMS)dwParam2);
    case MCI_STOP:		return MIDI_mciStop(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_SET:		return MIDI_mciSet(dwDevID, dwParam1, (LPMCI_SET_PARMS)dwParam2);
    case MCI_PAUSE:		return MIDI_mciPause(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_RESUME:		return MIDI_mciResume(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_STATUS:		return MIDI_mciStatus(dwDevID, dwParam1, (LPMCI_STATUS_PARMS)dwParam2);
    case MCI_GETDEVCAPS:	return MIDI_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)dwParam2);
    case MCI_INFO:		return MIDI_mciInfo(dwDevID, dwParam1, (LPMCI_INFO_PARMS16)dwParam2);
    default:			return DefDriverProc32(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}
/*-----------------------------------------------------------------------*/
