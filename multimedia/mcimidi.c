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
    WORD		wNotifyDeviceID;    	/* MCI device ID with a pending notification */
    HANDLE16 		hCallback;         	/* Callback handle for pending notification  */
    HMMIO32		hFile;	            	/* mmio file handle open as Element          */
    MCI_OPEN_PARMS32A 	openParms;
    HLOCAL16		hMidiHdr;
    WORD		dwStatus;		/* one from MCI_MODE_xxxx */
    DWORD		dwMciTimeFormat;	/* One of the supported MCI_FORMAT_xxxx */	       
    WORD		wFormat;		/* Format of MIDI hFile (0, 1 or 2) */
    WORD		nTracks;		/* Number of tracks in hFile */
    WORD		nDivision;		/* Number of division in hFile PPQN or SMPTE */
    WORD		wStartedPlaying;
    DWORD		dwTempo;		/* Tempo (# of 1/4 note per second */
    MCI_MIDITRACK*     	tracks;			/* Content of each track */
    DWORD		dwPulse;		
    DWORD		dwPositionMS;
    DWORD		dwStartTicks;
} WINE_MCIMIDI;

#define MAX_MCIMIDIDRV 	(1)
static WINE_MCIMIDI	MCIMidiDev[MAX_MCIMIDIDRV];

/*======================================================================*
 *                  	    MCI MIDI implemantation			*
 *======================================================================*/

#ifdef SNDCTL_MIDI_INFO
/**************************************************************************
 * 				MIDI_mciGetOpenDev		[internal]	
 */
static WINE_MCIMIDI*  MIDI_mciGetOpenDev(UINT16 wDevID)
{
    if (wDevID >= MAX_MCIMIDIDRV || MCIMidiDev[wDevID].nUseCount == 0) {
	WARN(mcimidi, "Invalid wDevID=%u\n", wDevID);
	return 0;
    }
    return &MCIMidiDev[wDevID];
}

/**************************************************************************
 * 				MIDI_mciReadByte		[internal]	
 */
static DWORD MIDI_mciReadByte(WINE_MCIMIDI* wmm, BYTE *lpbyt)
{
    DWORD	ret = 0;
    
    if (lpbyt == NULL || 
	mmioRead32(wmm->hFile, (HPSTR)lpbyt, (long)sizeof(BYTE)) != (long)sizeof(BYTE)) {
	WARN(mcimidi, "Error reading wmm=%p\n", wmm);
	ret = MCIERR_INTERNAL;
    }
    
    return ret;
}

/**************************************************************************
 * 				MIDI_mciReadWord		[internal]	
 */
static DWORD MIDI_mciReadWord(WINE_MCIMIDI* wmm, LPWORD lpw)
{
    BYTE	hibyte, lobyte;
    DWORD	ret = MCIERR_INTERNAL;
    
    if (lpw != NULL && 
	MIDI_mciReadByte(wmm, &hibyte) == 0 && 
	MIDI_mciReadByte(wmm, &lobyte) == 0) {
	*lpw = ((WORD)hibyte << 8) + lobyte;
	ret = 0;
    }
    return ret;
}

/**************************************************************************
 * 				MIDI_mciReadLong		[internal]	
 */
static DWORD MIDI_mciReadLong(WINE_MCIMIDI* wmm, LPDWORD lpdw)
{
    WORD	hiword, loword;
    DWORD	ret = MCIERR_INTERNAL;
    
    if (lpdw != NULL &&
	MIDI_mciReadWord(wmm, &hiword) == 0 &&
	MIDI_mciReadWord(wmm, &loword) == 0) {
	*lpdw = MAKELONG(loword, hiword);
	ret = 0;
    }
    return ret;
}

/**************************************************************************
 *  				MIDI_mciReadVaryLen		[internal]	
 */
static WORD MIDI_mciReadVaryLen(WINE_MCIMIDI* wmm, LPDWORD lpdw)
{
    BYTE	byte;
    DWORD	value = 0;
    WORD	ret = 0;
    
    if (lpdw == NULL) {
	ret = MCIERR_INTERNAL;
    } else {
	do {
	    if (MIDI_mciReadByte(wmm, &byte) != 0) {
		return 0;
	    }
	    value = (value << 7) + (byte & 0x7F);
	    ret++; 
	} while (byte & 0x80);
	*lpdw = value;
	/*
	  TRACE(mcimidi, "val=%08lX \n", value);
	*/
    }
    return ret;
}

/**************************************************************************
 * 				MIDI_mciReadNextEvent		[internal]	
 */
static DWORD	MIDI_mciReadNextEvent(WINE_MCIMIDI* wmm, MCI_MIDITRACK* mmt) 
{
    BYTE	b1, b2 = 0, b3;
    WORD	hw = 0;
    DWORD	evtPulse;
    DWORD	evtLength;
    DWORD	tmp;
    
    if (mmioSeek32(wmm->hFile, mmt->dwIndex, SEEK_SET) != mmt->dwIndex) {
	WARN(mcimidi, "Can't seek at %08lX \n", mmt->dwIndex);
	return MCIERR_INTERNAL;
    }
    evtLength = MIDI_mciReadVaryLen(wmm, &evtPulse) + 1;	/* > 0 */
    MIDI_mciReadByte(wmm, &b1);
    switch (b1) {
    case 0xF0:
    case 0xF7:
	evtLength += MIDI_mciReadVaryLen(wmm, &tmp);
	evtLength += tmp;
	break;
    case 0xFF:
	MIDI_mciReadByte(wmm, &b2);	evtLength++;
	
	evtLength += MIDI_mciReadVaryLen(wmm, &tmp);
	if (evtLength >= 0x10000u) {
	    /* this limitation shouldn't be a problem */
	    WARN(mcimidi, "Ouch !! Implementation limitation to 64k bytes for a MIDI event is overflowed\n");
	    hw = 0xFFFF;
	} else {
	    hw = LOWORD(evtLength);
	}
	evtLength += tmp;
	break;
    default:
	if (b1 & 0x80) { /* use running status ? */
	    mmt->wLastCommand = b1;
	    MIDI_mciReadByte(wmm, &b2);	evtLength++;
	} else {
	    b2 = b1;
	    b1 = mmt->wLastCommand;
	}
	switch ((b1 >> 4) & 0x07) {
	case 0:	case 1:	case 2: case 3: case 6:
	    MIDI_mciReadByte(wmm, &b3);	evtLength++;
	    hw = b3;
	    break;
	case 4:	case 5:
	    break;
	case 7:
	    WARN(mcimidi, "Strange indeed b1=0x%02x\n", b1);
	}
	break;
    }
    if (mmt->dwIndex + evtLength > mmt->dwLast)
	return MCIERR_INTERNAL;
    
    mmt->dwEventPulse += evtPulse;
    mmt->dwEventData   = (hw << 16) + (b2 << 8) + b1;
    mmt->wEventLength  = evtLength;
    
    /*
      TRACE(mcimidi, "[%u] => pulse=%08lx(%08lx), data=%08lx, length=%u\n", 
      mmt->wTrackNr, mmt->dwEventPulse, evtPulse, 
      mmt->dwEventData, mmt->wEventLength);
    */
    return 0;
}

/**************************************************************************
 * 				MIDI_mciReadMTrk		[internal]	
 */
static DWORD MIDI_mciReadMTrk(WINE_MCIMIDI* wmm, MCI_MIDITRACK* mmt)
{
    DWORD		toberead;
    FOURCC		fourcc;
    
    if (mmioRead32(wmm->hFile, (HPSTR)&fourcc, (long)sizeof(FOURCC)) != 
	(long)sizeof(FOURCC)) {
	return MCIERR_INTERNAL;
    }
    
    if (fourcc != mmioFOURCC('M', 'T', 'r', 'k')) {
	WARN(mcimidi, "Can't synchronize on 'MTrk' !\n");
	return MCIERR_INTERNAL;
    }
    
    if (MIDI_mciReadLong(wmm, &toberead) != 0) {
	return MCIERR_INTERNAL;
    }
    mmt->dwFirst = mmioSeek32(wmm->hFile, 0, SEEK_CUR); /* >= 0 */
    mmt->dwLast = mmt->dwFirst + toberead;
    
    /* compute # of pulses in this track */
    mmt->dwIndex = mmt->dwFirst;
    mmt->dwEventPulse = 0;
    
    while (MIDI_mciReadNextEvent(wmm, mmt) == 0 && 
	   LOWORD(mmt->dwEventData) != 0x2FFF) {	
	mmt->dwIndex += mmt->wEventLength;
    }
    mmt->dwLength = mmt->dwEventPulse;
    
    TRACE(mcimidi, "Track %u has %lu bytes and %lu pulses\n", mmt->wTrackNr, toberead, mmt->dwLength);
    
    /* reset track data */
    mmt->wStatus = 1;	/* ok, playing */
    mmt->dwIndex = mmt->dwFirst;
    mmt->dwEventPulse = 0;
    
    if (mmioSeek32(wmm->hFile, 0, SEEK_CUR) != mmt->dwLast) {
	WARN(mcimidi, "Ouch, out of sync seek=%lu track=%lu\n", 
	     mmioSeek32(wmm->hFile, 0, SEEK_CUR), mmt->dwLast);
	/* position at end of this track, to be ready to read next track */
	mmioSeek32(wmm->hFile, mmt->dwLast, SEEK_SET);
    }
    
    return 0;
}

/**************************************************************************
 * 				MIDI_mciReadMThd		[internal]	
 */
static DWORD MIDI_mciReadMThd(WINE_MCIMIDI* wmm, DWORD dwOffset)
{
    DWORD	toberead;
    FOURCC	fourcc;
    WORD	nt;
    
    TRACE(mcimidi, "(%p, %08lX);\n", wmm, dwOffset);
    
    if (mmioSeek32(wmm->hFile, dwOffset, SEEK_SET) != dwOffset) {
	WARN(mcimidi, "Can't seek at %08lX begin of 'MThd' \n", dwOffset);
	return MCIERR_INTERNAL;
    }
    if (mmioRead32(wmm->hFile, (HPSTR)&fourcc,
		   (long) sizeof(FOURCC)) != (long) sizeof(FOURCC))
	return MCIERR_INTERNAL;
    
    if (fourcc != mmioFOURCC('M', 'T', 'h', 'd')) {
	WARN(mcimidi, "Can't synchronize on 'MThd' !\n");
	return MCIERR_INTERNAL;
    }
    
    if (MIDI_mciReadLong(wmm, &toberead) != 0 || toberead < 3 * sizeof(WORD))
	return MCIERR_INTERNAL;
    
    if (MIDI_mciReadWord(wmm, &wmm->wFormat) != 0 ||
	MIDI_mciReadWord(wmm, &wmm->nTracks) != 0 ||
	MIDI_mciReadWord(wmm, &wmm->nDivision) != 0) {
	return MCIERR_INTERNAL;
    }
    
    TRACE(mcimidi, "toberead=0x%08lX, wFormat=0x%04X nTracks=0x%04X nDivision=0x%04X\n",
	  toberead, wmm->wFormat, wmm->nTracks, wmm->nDivision);
    
    /* MS doc says that the MIDI MCI time format must be put by default to the format
     * stored in the MIDI file...
     */
    if (wmm->nDivision > 0x8000) {
	/* eric.pouech@lemel.fr 98/11
	 * In did not check this very code (pulses are expressed as SMPTE sub-frames).
	 * In about 40 MB of MIDI files I have, none was SMPTE based...
	 * I'm just wondering if this is widely used :-). So, if someone has one of 
	 * these files, I'd like to know about.
	 */
	FIXME(mcimidi, "Handling SMPTE time in MIDI files has not been tested\n"
	      "Please report to comp.emulators.ms-windows.wine with MIDI file !\n");
	
	switch (HIBYTE(wmm->nDivision)) {
	case 0xE8:	wmm->dwMciTimeFormat = MCI_FORMAT_SMPTE_24;	break;	/* -24 */
	case 0xE7:	wmm->dwMciTimeFormat = MCI_FORMAT_SMPTE_25;	break;	/* -25 */
	case 0xE3:	wmm->dwMciTimeFormat = MCI_FORMAT_SMPTE_30DROP;	break;	/* -29 */ /* is the MCI constant correct ? */
	case 0xE2:	wmm->dwMciTimeFormat = MCI_FORMAT_SMPTE_30;	break;	/* -30 */
	default:
	    WARN(mcimidi, "Unsupported number of frames %d\n", -(char)HIBYTE(wmm->nDivision));
	    return MCIERR_INTERNAL;
	}
	switch (LOBYTE(wmm->nDivision)) {
	case 4:	/* MIDI Time Code */
	case 8:
	case 10:
	case 80: /* SMPTE bit resolution */
	case 100:
	default:
	    WARN(mcimidi, "Unsupported number of sub-frames %d\n", LOBYTE(wmm->nDivision));
	    return MCIERR_INTERNAL;
	}
	return MCIERR_INTERNAL;
    } else if (wmm->nDivision == 0) {
	WARN(mcimidi, "Number of division is 0, can't support that !!\n");
	return MCIERR_INTERNAL;
    } else {
	wmm->dwMciTimeFormat = MCI_FORMAT_MILLISECONDS;
    }
    
    switch (wmm->wFormat) {
    case 0:
	if (wmm->nTracks != 1) {
	    WARN(mcimidi, "Got type 0 file whose number of track is not 1. Setting it to 1\n");
	    wmm->nTracks = 1;
	}
	break;
    case 1:
    case 2:
	break;
    default:
	WARN(mcimidi, "Handling MIDI files which format = %d is not (yet) supported\n"
	     "Please report with MIDI file !\n", wmm->wFormat);
	return MCIERR_INTERNAL;
    }
    
    if (wmm->nTracks & 0x8000) {
	/* this shouldn't be a problem... */
	WARN(mcimidi, "Ouch !! Implementation limitation to 32k tracks per MIDI file is overflowed\n");
	wmm->nTracks = 0x7FFF;
    }
    
    wmm->tracks = xmalloc(sizeof(MCI_MIDITRACK) * wmm->nTracks);
    
    toberead -= 3 * sizeof(WORD); 
    if (toberead > 0) {
	TRACE(mcimidi, "Size of MThd > 6, skipping %ld extra bytes\n", toberead);
	mmioSeek32(wmm->hFile, toberead, SEEK_CUR);
    }
    
    for (nt = 0; nt < wmm->nTracks; nt++) {
	wmm->tracks[nt].wTrackNr = nt;
	if (MIDI_mciReadMTrk(wmm, &wmm->tracks[nt]) != 0) {
	    WARN(mcimidi, "Can't read 'MTrk' header \n");
	    return MCIERR_INTERNAL;
	}
    }
    
    wmm->dwTempo = 500000;
    
    return 0;
}

/**************************************************************************
 * 			MIDI_ConvertPulseToMS			[internal]
 */
static	DWORD	MIDI_ConvertPulseToMS(WINE_MCIMIDI* wmm, DWORD pulse)
{
    DWORD	ret = 0;
    
    /* FIXME: this function may return false values since the tempo (wmm->dwTempo)
     * may change during file playing
     */
    if (wmm->nDivision == 0) {
	FIXME(mcimidi, "Shouldn't happen. wmm->nDivision = 0\n");
    } else if (wmm->nDivision > 0x8000) { /* SMPTE, unchecked FIXME? */
	int	nf = -(char)HIBYTE(wmm->nDivision);	/* number of frames     */
	int	nsf = LOBYTE(wmm->nDivision);		/* number of sub-frames */
	ret = (pulse * 1000) / (nf * nsf);
    } else {
	ret = (DWORD)((double)pulse * ((double)wmm->dwTempo / 1000) /	
		      (double)wmm->nDivision);
    }
    
    /*
      TRACE(mcimidi, "pulse=%lu tempo=%lu division=%u=0x%04x => ms=%lu\n", 
      pulse, wmm->dwTempo, wmm->nDivision, wmm->nDivision, ret);
    */
    
    return ret;
}

#define TIME_MS_IN_ONE_HOUR	(60*60*1000)
#define TIME_MS_IN_ONE_MINUTE	(60*1000)
#define TIME_MS_IN_ONE_SECOND	(1000)

/**************************************************************************
 * 			MIDI_ConvertTimeFormatToMS		[internal]
 */
static	DWORD	MIDI_ConvertTimeFormatToMS(WINE_MCIMIDI* wmm, DWORD val)
{
    DWORD	ret = 0;
    
    switch (wmm->dwMciTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:	
	ret = val;		
	break;
    case MCI_FORMAT_SMPTE_24:
	ret = 
	    (HIBYTE(HIWORD(val)) * 125) / 3 +             LOBYTE(HIWORD(val)) * TIME_MS_IN_ONE_SECOND +
	    HIBYTE(LOWORD(val)) * TIME_MS_IN_ONE_MINUTE + LOBYTE(LOWORD(val)) * TIME_MS_IN_ONE_HOUR;
	break;
    case MCI_FORMAT_SMPTE_25:		
	ret = 
	    HIBYTE(HIWORD(val)) * 40 + 		  	  LOBYTE(HIWORD(val)) * TIME_MS_IN_ONE_SECOND +
	    HIBYTE(LOWORD(val)) * TIME_MS_IN_ONE_MINUTE + LOBYTE(LOWORD(val)) * TIME_MS_IN_ONE_HOUR;
	break;
    case MCI_FORMAT_SMPTE_30:		
	ret = 
	    (HIBYTE(HIWORD(val)) * 100) / 3 + 		  LOBYTE(HIWORD(val)) * TIME_MS_IN_ONE_SECOND +
	    HIBYTE(LOWORD(val)) * TIME_MS_IN_ONE_MINUTE + LOBYTE(LOWORD(val)) * TIME_MS_IN_ONE_HOUR;
	break;
    default:
	WARN(mcimidi, "Bad time format %lu!\n", wmm->dwMciTimeFormat);
    }
    /*
      TRACE(mcimidi, "val=%lu=0x%08lx [tf=%lu] => ret=%lu\n", val, val, wmm->dwMciTimeFormat, ret);
    */
    return ret;
}	

/**************************************************************************
 * 			MIDI_ConvertMSToTimeFormat		[internal]
 */
static	DWORD	MIDI_ConvertMSToTimeFormat(WINE_MCIMIDI* wmm, DWORD _val)
{
    DWORD	ret = 0, val = _val;
    DWORD	h, m, s, f;
    
    switch (wmm->dwMciTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:	
	ret = val;		
	break;
    case MCI_FORMAT_SMPTE_24:
    case MCI_FORMAT_SMPTE_25:		
    case MCI_FORMAT_SMPTE_30:		
	h = val / TIME_MS_IN_ONE_HOUR;	
	m = (val -= h * TIME_MS_IN_ONE_HOUR)   / TIME_MS_IN_ONE_MINUTE;	
	s = (val -= m * TIME_MS_IN_ONE_MINUTE) / TIME_MS_IN_ONE_SECOND;
	switch (wmm->dwMciTimeFormat) {
	case MCI_FORMAT_SMPTE_24:
	    /* one frame is 1000/24 val long, 1000/24 == 125/3 */
	    f = (val * 3) / 125; 	val -= (f * 125) / 3;
	    break;
	case MCI_FORMAT_SMPTE_25:		
	    /* one frame is 1000/25 ms long, 1000/25 == 40 */
	    f = val / 40; 		val -= f * 40;
	    break;
	case MCI_FORMAT_SMPTE_30:		
	    /* one frame is 1000/30 ms long, 1000/30 == 100/3 */
	    f = (val * 3) / 100; 	val -= (f * 100) / 3;
	    break;
	default:
	    FIXME(mcimidi, "There must be some bad bad programmer\n");
	    f = 0;
	}
	/* val contains the number of ms which cannot make a complete frame */
	/* FIXME: is this correct ? programs seem to be happy with that */
	ret = (f << 24) | (s << 16) | (m << 8) | (h << 0);
	break;
    default:
	WARN(mcimidi, "Bad time format %lu!\n", wmm->dwMciTimeFormat);
    }
    /*
      TRACE(mcimidi, "val=%lu [tf=%lu] => ret=%lu=0x%08lx\n", _val, wmm->dwMciTimeFormat, ret, ret);
    */
    return ret;
}	

/**************************************************************************
 * 			MIDI_GetMThdLengthMS			[internal]
 */
static	DWORD	MIDI_GetMThdLengthMS(WINE_MCIMIDI* wmm)
{
    WORD	nt;
    DWORD	ret = 0;
    
    for (nt = 0; nt < wmm->nTracks; nt++) {
	if (wmm->wFormat == 2) {
	    ret += wmm->tracks[nt].dwLength;
	} else if (wmm->tracks[nt].dwLength > ret) {
	    ret = wmm->tracks[nt].dwLength;
	}
    }
    /* FIXME: this is wrong if there is a tempo change inside the file */
    return MIDI_ConvertPulseToMS(wmm, ret);
}

/**************************************************************************
 * 				MIDI_mciOpen			[internal]	
 */
static DWORD MIDI_mciOpen(UINT16 wDevID, DWORD dwFlags, LPMCI_OPEN_PARMS32A lpParms)
{
    MIDIOPENDESC 	midiOpenDesc;
    DWORD		dwRet = 0;
    DWORD		dwDeviceID;
    WINE_MCIMIDI*	wmm;
    
    TRACE(mcimidi, "(%04x, %08lX, %p)\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL) 	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wDevID >= MAX_MCIMIDIDRV) {
	WARN(mcimidi, "Invalid wDevID=%u\n", wDevID);
	return MCIERR_INVALID_DEVICE_ID;
    }
    wmm = &MCIMidiDev[wDevID];
    
    if (wmm->nUseCount > 0) {
	/* The driver is already opened on this channel
	 * MIDI sequencer cannot be shared
	 */
	return MCIERR_DEVICE_OPEN;
    }
    wmm->nUseCount++;
    
    wmm->hFile = 0;
    dwDeviceID = lpParms->wDeviceID;
    
    TRACE(mcimidi, "wDevID=%04X (lpParams->wDeviceID=%08lX)\n", wDevID, dwDeviceID);
    /*	lpParms->wDeviceID = wDevID;*/
    
    if (dwFlags & MCI_OPEN_ELEMENT) {
	LPSTR		lpstrElementName;
	
	lpstrElementName = lpParms->lpstrElementName;
	
	TRACE(mcimidi, "MCI_OPEN_ELEMENT '%s' !\n", lpstrElementName);
	if (lpstrElementName && strlen(lpstrElementName) > 0) {
	    wmm->hFile = mmioOpen32A(lpstrElementName, NULL, 
				     MMIO_ALLOCBUF | MMIO_READWRITE | MMIO_EXCLUSIVE);
	    if (wmm->hFile == 0) {
		WARN(mcimidi, "Can't find file='%s' !\n", lpstrElementName);
		wmm->nUseCount--;
		return MCIERR_FILE_NOT_FOUND;
	    }
	} else {
	    wmm->hFile = 0;
	}
    }
    TRACE(mcimidi, "hFile=%u\n", wmm->hFile);
    
    memcpy(&wmm->openParms, lpParms, sizeof(MCI_OPEN_PARMS32A));
    
    wmm->wNotifyDeviceID = dwDeviceID;
    wmm->dwStatus = MCI_MODE_NOT_READY;	/* while loading file contents */
    /* spec says it should be the default format from the MIDI file... */
    wmm->dwMciTimeFormat = MCI_FORMAT_MILLISECONDS;
    
    midiOpenDesc.hMidi = 0;
    
    if (wmm->hFile != 0) {
	MMCKINFO	ckMainRIFF;
	MMCKINFO	mmckInfo;
	DWORD		dwOffset = 0;
	
	if (mmioDescend(wmm->hFile, &ckMainRIFF, NULL, 0) != 0) {
	    dwRet = MCIERR_INVALID_FILE;
	} else {
	    TRACE(mcimidi,"ParentChunk ckid=%.4s fccType=%.4s cksize=%08lX \n",
		  (LPSTR)&ckMainRIFF.ckid, (LPSTR)&ckMainRIFF.fccType, ckMainRIFF.cksize);
	    
	    if (ckMainRIFF.ckid == FOURCC_RIFF && ckMainRIFF.fccType == mmioFOURCC('R', 'M', 'I', 'D')) {
		mmckInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
		if (mmioDescend(wmm->hFile, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK) == 0) {
		    TRACE(mcimidi, "... is a 'RMID' file \n");
		    dwOffset = mmckInfo.dwDataOffset;
		} else {
		    dwRet = MCIERR_INVALID_FILE;
		}
	    }
	    if (dwRet == 0 && MIDI_mciReadMThd(wmm, dwOffset) != 0) {
		WARN(mcimidi, "Can't read 'MThd' header \n");
		dwRet = MCIERR_INVALID_FILE;
	    }
	}
    } else {
	TRACE(mcimidi, "hFile==0, setting #tracks to 0; is this correct ?\n");
	wmm->nTracks = 0;
	wmm->wFormat = 0;
	wmm->nDivision = 1;
    }
    if (dwRet != 0) {
	wmm->nUseCount--;
	if (wmm->hFile != 0)
	    mmioClose32(wmm->hFile, 0);
	wmm->hFile = 0;
    } else {
	wmm->dwPositionMS = 0;
	wmm->dwStatus = MCI_MODE_STOP;
	wmm->hMidiHdr = USER_HEAP_ALLOC(sizeof(MIDIHDR));
	
	dwRet = modMessage(wDevID, MODM_OPEN, 0, (DWORD)&midiOpenDesc, CALLBACK_NULL);
	/*	dwRet = midMessage(wDevID, MIDM_OPEN, 0, (DWORD)&midiOpenDesc, CALLBACK_NULL);*/
    }
    return dwRet;
}

/**************************************************************************
 * 				MIDI_mciStop			[internal]
 */
static DWORD MIDI_mciStop(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{    
    WINE_MCIMIDI*	wmm = MIDI_mciGetOpenDev(wDevID);
    
    TRACE(mcimidi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmm == NULL)	return MCIERR_INVALID_DEVICE_ID;
    
    wmm->dwStatus = MCI_MODE_STOP;
    TRACE(mcimidi, "wmm->dwStatus=%d\n", wmm->dwStatus);
    
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE(mcimidi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmm->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				MIDI_mciClose			[internal]
 */
static DWORD MIDI_mciClose(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD		dwRet;
    WINE_MCIMIDI*	wmm = MIDI_mciGetOpenDev(wDevID);
    
    TRACE(mcimidi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmm == NULL)	return MCIERR_INVALID_DEVICE_ID;
    
    if (wmm->dwStatus != MCI_MODE_STOP) {
	MIDI_mciStop(wDevID, MCI_WAIT, lpParms);
    }
    
    wmm->dwStatus = MCI_MODE_STOP;
    wmm->nUseCount--;
    if (wmm->nUseCount == 0) {
	if (wmm->hFile != 0) {
	    mmioClose32(wmm->hFile, 0);
	    wmm->hFile = 0;
	    TRACE(mcimidi, "hFile closed !\n");
	}
	USER_HEAP_FREE(wmm->hMidiHdr);
	free(wmm->tracks);
	dwRet = modMessage(wDevID, MODM_CLOSE, 0, 0L, 0L);
	if (dwRet != MMSYSERR_NOERROR) return MCIERR_INTERNAL;
	/*
	  dwRet = midMessage(wDevID, MIDM_CLOSE, 0, 0L, 0L);
	  if (dwRet != MMSYSERR_NOERROR) return MCIERR_INTERNAL;
	*/
    } else {
	TRACE(mcimidi, "Shouldn't happen... nUseCount=%d\n", wmm->nUseCount);
	return MCIERR_INTERNAL;
    }
    
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE(mcimidi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmm->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				MIDI_mciFindNextEvent		[internal]
 */
static MCI_MIDITRACK*	MIDI_mciFindNextEvent(WINE_MCIMIDI* wmm, LPDWORD hiPulse) 
{
    WORD		cnt, nt;
    MCI_MIDITRACK*	mmt;
    
    *hiPulse = 0xFFFFFFFFul;
    cnt = 0xFFFFu;
    for (nt = 0; nt < wmm->nTracks; nt++) {
	mmt = &wmm->tracks[nt];
	
	if (mmt->wStatus == 0)
	    continue;
	if (mmt->dwEventPulse < *hiPulse) {
	    *hiPulse = mmt->dwEventPulse;
	    cnt = nt;
	}
    }
    return (cnt == 0xFFFFu) ? 0 /* no more event on all tracks */
	: &wmm->tracks[cnt];
}

/**************************************************************************
 * 				MIDI_mciPlay			[internal]
 */
static DWORD MIDI_mciPlay(UINT16 wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
    DWORD		dwStartMS, dwEndMS, dwRet;
    WORD		doPlay, nt;
    MCI_MIDITRACK*	mmt;
    DWORD		hiPulse;
    WINE_MCIMIDI*	wmm = MIDI_mciGetOpenDev(wDevID);
    
    TRACE(mcimidi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmm == NULL)	return MCIERR_INVALID_DEVICE_ID;
    
    if (wmm->hFile == 0) {
	WARN(mcimidi, "Can't play: no file '%s' !\n", wmm->openParms.lpstrElementName);
	return MCIERR_FILE_NOT_FOUND;
    }
    
    if (wmm->dwStatus != MCI_MODE_STOP) {
	WARN(mcimidi, "Can't play: device is not stopped !\n");
	return MCIERR_INTERNAL;
    }
    
    if (!(dwFlags & MCI_WAIT)) {	
	/** FIXME: I'm not 100% sure that wNotifyDeviceID is the right value in all cases ??? */
	return MCI_SendCommandAsync32(wmm->wNotifyDeviceID, MCI_PLAY, dwFlags, (DWORD)lpParms);
    }
    
    if (lpParms && (dwFlags & MCI_FROM)) {
	dwStartMS = MIDI_ConvertTimeFormatToMS(wmm, lpParms->dwFrom); 
    } else {
	dwStartMS = wmm->dwPositionMS;
    }
    
    if (lpParms && (dwFlags & MCI_TO)) {
	dwEndMS = MIDI_ConvertTimeFormatToMS(wmm, lpParms->dwTo);
    } else {
	dwEndMS = 0xFFFFFFFFul;
    }
    
    TRACE(mcimidi, "Playing from %lu to %lu\n", dwStartMS, dwEndMS);
    
    /* init tracks */
    for (nt = 0; nt < wmm->nTracks; nt++) {
	mmt = &wmm->tracks[nt];
	
	mmt->wStatus = 1;	/* ok, playing */
	mmt->dwIndex = mmt->dwFirst;
	if (wmm->wFormat == 2 && nt > 0) {
	    mmt->dwEventPulse = wmm->tracks[nt - 1].dwLength;
	} else {
	    mmt->dwEventPulse = 0;
	}
	MIDI_mciReadNextEvent(wmm, mmt); /* FIXME == 0 */
    }
    
    wmm->dwPulse = 0;
    wmm->dwTempo = 500000;
    wmm->dwStatus = MCI_MODE_PLAY;
    wmm->dwPositionMS = 0;
    wmm->wStartedPlaying = FALSE;
    
    while (wmm->dwStatus != MCI_MODE_STOP) {
	/* it seems that in case of multi-threading, gcc is optimizing just a little bit 
	 * too much. Tell gcc not to optimize status value using volatile. 
	 */
	while (((volatile WINE_MCIMIDI*)wmm)->dwStatus == MCI_MODE_PAUSE);
	
	doPlay = (wmm->dwPositionMS >= dwStartMS && wmm->dwPositionMS <= dwEndMS);
	
	TRACE(mcimidi, "wmm->dwStatus=%d, doPlay=%s\n", wmm->dwStatus, doPlay ? "T" : "F");
	
	if ((mmt = MIDI_mciFindNextEvent(wmm, &hiPulse)) == NULL)
	    break;  /* no more event on tracks */
	
	/* if starting playing, then set StartTicks to the value it would have had
	 * if play had started at position 0
	 */
	if (doPlay && !wmm->wStartedPlaying) {
	    wmm->dwStartTicks = GetTickCount() - MIDI_ConvertPulseToMS(wmm, wmm->dwPulse);
	    wmm->wStartedPlaying = TRUE;
	    TRACE(mcimidi, "Setting dwStartTicks to %lu\n", wmm->dwStartTicks);
	}
	
	if (hiPulse > wmm->dwPulse) {
	    if (doPlay) {
		DWORD	togo = wmm->dwStartTicks + wmm->dwPositionMS + MIDI_ConvertPulseToMS(wmm, hiPulse - wmm->dwPulse);
		DWORD	tc = GetTickCount();
		
		TRACE(mcimidi, "Pulses hi=0x%08lx <> cur=0x%08lx\n", hiPulse, wmm->dwPulse);
		TRACE(mcimidi, "Wait until %lu => %lu ms\n", 	
		      tc - wmm->dwStartTicks, togo - wmm->dwStartTicks);
		if (tc < togo)	
		    Sleep(togo - tc);
	    }
	    wmm->dwPositionMS += MIDI_ConvertPulseToMS(wmm, hiPulse - wmm->dwPulse);
	    wmm->dwPulse = hiPulse;
	}
	
	switch (LOBYTE(LOWORD(mmt->dwEventData))) {
	case 0xF0:
	case 0xF7:	/* sysex events */
	    {
		FIXME(mcimidi, "Not handling SysEx events (yet)\n");
	    }
	    break;
	case 0xFF:
	    /* position after meta data header */
	    mmioSeek32(wmm->hFile, mmt->dwIndex + HIWORD(mmt->dwEventData), SEEK_SET);
	    switch (HIBYTE(LOWORD(mmt->dwEventData))) {
	    case 0x00: /* 16-bit sequence number */
		if (TRACE_ON(midi)) {
		    WORD	twd;
		    
		    MIDI_mciReadWord(wmm, &twd);	/* == 0 */
		    TRACE(mcimidi, "Got sequence number %u\n", twd);
		}
		break;
	    case 0x01: /* any text */
	    case 0x02: /* Copyright Message text */
	    case 0x03: /* Sequence/Track Name text */
	    case 0x04: /* Instrument Name text */
	    case 0x05: /* Lyric text */
	    case 0x06: /* Marker text */
	    case 0x07: /* Cue-point text */
		if (TRACE_ON(mcimidi)) {
		    char	buf[1024];
		    WORD	len = mmt->wEventLength - HIWORD(mmt->dwEventData);
		    static	char*	info[8] = {"", "Text", "Copyright", "Seq/Trk name", 
						   "Instrument", "Lyric", "Marker", "Cue-point"};
		    WORD	idx = HIBYTE(LOWORD(mmt->dwEventData));
		    
		    if (len >= sizeof(buf)) {
			WARN(mcimidi, "Buffer for text is too small (%d bytes, when %u are needed)\n", sizeof(buf) - 1, len);
			len = sizeof(buf) - 1;
		    }
		    if (mmioRead32(wmm->hFile, (HPSTR)buf, len) == len) {
			buf[len] = 0;	/* end string in case */
			TRACE(mcimidi, "%s => \"%s\"\n", (idx < 8 ) ? info[idx] : "", buf);
		    } else {
			WARN(mcimidi, "Couldn't read data for %s\n", (idx < 8 ) ? info[idx] : "");
		    }
		}
		break;
	    case 0x20: 
		/* MIDI channel (cc) */
		if (FIXME_ON(mcimidi)) {
		    BYTE	bt;
		    
		    MIDI_mciReadByte(wmm, &bt);	/* == 0 */
		    FIXME(mcimidi, "NIY: MIDI channel=%u, track=%u\n", bt, mmt->wTrackNr);
		}
		break;
	    case 0x21:
		/* MIDI port (pp) */
		if (FIXME_ON(mcimidi)) {
		    BYTE	bt;
		    
		    MIDI_mciReadByte(wmm, &bt);	/* == 0 */
		    FIXME(mcimidi, "NIY: MIDI port=%u, track=%u\n", bt, mmt->wTrackNr);
		}
		break;
	    case 0x2F: /* end of track */
		mmt->wStatus = 0;
		break;
	    case 0x51:/* set tempo */
		/* Tempo is expressed in µ-seconds per midi quarter note
		 * for format 1 MIDI files, this can only be present on track #0
		 */
		if (mmt->wTrackNr != 0 && wmm->wFormat == 1) {
		    WARN(mcimidi, "For format #1 MIDI files, tempo can only be changed on track #0 (%u)\n", mmt->wTrackNr);
		} else { 
		    BYTE	tbt;
		    DWORD	value = 0;
		    
		    MIDI_mciReadByte(wmm, &tbt);	value  = ((DWORD)tbt) << 16;
		    MIDI_mciReadByte(wmm, &tbt);	value |= ((DWORD)tbt) << 8;
		    MIDI_mciReadByte(wmm, &tbt);	value |= ((DWORD)tbt) << 0;
		    TRACE(mcimidi, "Setting tempo to %ld (BPM=%ld)\n", wmm->dwTempo, (value) ? (60000000l / value) : 0);
		    wmm->dwTempo = value;
		}
		break;
	    case 0x54: /* (hour) (min) (second) (frame) (fractional-frame) - SMPTE track start */
		if (mmt->wTrackNr != 0 && wmm->wFormat == 1) {
		    WARN(mcimidi, "For format #1 MIDI files, SMPTE track start can only be expressed on track #0 (%u)\n", mmt->wTrackNr);
		} if (mmt->dwEventPulse != 0) {
		    WARN(mcimidi, "SMPTE track start can only be expressed at start of track (%lu)\n", mmt->dwEventPulse);
		} else {
		    BYTE	h, m, s, f, ff;
		    
		    MIDI_mciReadByte(wmm, &h);
		    MIDI_mciReadByte(wmm, &m);
		    MIDI_mciReadByte(wmm, &s);
		    MIDI_mciReadByte(wmm, &f);
		    MIDI_mciReadByte(wmm, &ff);
		    FIXME(mcimidi, "NIY: SMPTE track start %u:%u:%u %u.%u\n", h, m, s, f, ff);
		}
		break;
	    case 0x58: /* file rythm */
		if (TRACE_ON(mcimidi)) {
		    BYTE	num, den, cpmc, _32npqn;
		    
		    MIDI_mciReadByte(wmm, &num);	
		    MIDI_mciReadByte(wmm, &den);		/* to notate e.g. 6/8 */
		    MIDI_mciReadByte(wmm, &cpmc);		/* number of MIDI clocks per metronome click */ 
		    MIDI_mciReadByte(wmm, &_32npqn);		/* number of notated 32nd notes per MIDI quarter note */
		    
		    TRACE(mcimidi, "%u/%u, clock per metronome click=%u, 32nd notes by 1/4 note=%u\n", num, 1 << den, cpmc, _32npqn);
		}
		break;
	    case 0x59: /* key signature */
		if (TRACE_ON(mcimidi)) {
		    BYTE	sf, mm;
		    
		    MIDI_mciReadByte(wmm, &sf);		
		    MIDI_mciReadByte(wmm, &mm);	
		    
		    if (sf >= 0x80) 	TRACE(mcimidi, "%d flats\n", -(char)sf);	 
		    else if (sf > 0) 	TRACE(mcimidi, "%d sharps\n", (char)sf);	 
		    else 		TRACE(mcimidi, "Key of C\n");
		    TRACE(mcimidi, "Mode: %s\n", (mm = 0) ? "major" : "minor");
		}
		break;
	    default:
		WARN(mcimidi, "Unknown MIDI meta event %02x. Skipping...\n", HIBYTE(LOWORD(mmt->dwEventData)));
		break;
	    }
	    break;
	default:
	    if (doPlay) {
		dwRet = modMessage(wDevID, MODM_DATA, 0, mmt->dwEventData, 0);
	    } else {
		switch (LOBYTE(LOWORD(mmt->dwEventData)) & 0xF0) {
		case MIDI_NOTEON:
		case MIDI_NOTEOFF:
		    dwRet = 0;	
		    break;
		default:
		    dwRet = modMessage(wDevID, MODM_DATA, 0, mmt->dwEventData, 0);
		}
	    }
	}
	mmt->dwIndex += mmt->wEventLength;
	if (mmt->dwIndex < mmt->dwFirst || mmt->dwIndex >= mmt->dwLast) {
	    mmt->wStatus = 0;
	} 
	if (mmt->wStatus) {	
	    MIDI_mciReadNextEvent(wmm, mmt);
	}
    }
    
    /* stop all notes */
    /* should be (0x7800 | MIDI_CTL_CHANGE) instead of 0x78B0,
     * but MIDI_CTL_CHANGE is defined in OSS's soundcard.h and MCI must compile
     * without any reference to OSS
     */
    /* FIXME: check if 0x78B0 is channel dependant or not. I coded it so that 
     * it's channel dependent...
     */
    {
	unsigned chn;
	for (chn = 0; chn < 16; chn++)
	    modMessage(wDevID, MODM_DATA, 0, 0x78B0 | chn, 0);
    }
    
    wmm->dwStatus = MCI_MODE_STOP;
    
    /* to restart playing at beginning when it's over */
    wmm->dwPositionMS = 0;
    
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE(mcimidi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmm->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
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
    WINE_MCIMIDI*	wmm = MIDI_mciGetOpenDev(wDevID);
    
    TRACE(mcimidi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmm == 0)	return MCIERR_INVALID_DEVICE_ID;
    
    if (wmm->hFile == 0) {
	WARN(mcimidi, "Can't find file='%08lx' !\n", 
	     (DWORD)wmm->openParms.lpstrElementName);
	return MCIERR_FILE_NOT_FOUND;
    }
    start = 1; 		end = 99999;
    if (lpParms && (dwFlags & MCI_FROM)) {
	start = lpParms->dwFrom; 
	TRACE(mcimidi, "MCI_FROM=%d \n", start);
    }
    if (lpParms && (dwFlags & MCI_TO)) {
	end = lpParms->dwTo;
	TRACE(mcimidi, "MCI_TO=%d \n", end);
    }
    lpMidiHdr = USER_HEAP_LIN_ADDR(wmm->hMidiHdr);
    lpMidiHdr->lpData = (LPSTR) xmalloc(1200);
    lpMidiHdr->dwBufferLength = 1024;
    lpMidiHdr->dwUser = 0L;
    lpMidiHdr->dwFlags = 0L;
    dwRet = midMessage(wDevID, MIDM_PREPARE, 0, (DWORD)lpMidiHdr, sizeof(MIDIHDR));
    TRACE(mcimidi, "After MIDM_PREPARE \n");
    wmm->dwStatus = MCI_MODE_RECORD;
    while (wmm->dwStatus != MCI_MODE_STOP) {
	TRACE(mcimidi, "wmm->dwStatus=%p %d\n",
	      &wmm->dwStatus, wmm->dwStatus);
	lpMidiHdr->dwBytesRecorded = 0;
	dwRet = midMessage(wDevID, MIDM_START, 0, 0L, 0L);
	TRACE(mcimidi, "After MIDM_START lpMidiHdr=%p dwBytesRecorded=%lu\n",
	      lpMidiHdr, lpMidiHdr->dwBytesRecorded);
	if (lpMidiHdr->dwBytesRecorded == 0) break;
    }
    TRACE(mcimidi, "Before MIDM_UNPREPARE \n");
    dwRet = midMessage(wDevID, MIDM_UNPREPARE, 0, (DWORD)lpMidiHdr, sizeof(MIDIHDR));
    TRACE(mcimidi, "After MIDM_UNPREPARE \n");
    if (lpMidiHdr->lpData != NULL) {
	free(lpMidiHdr->lpData);
	lpMidiHdr->lpData = NULL;
    }
    wmm->dwStatus = MCI_MODE_STOP;
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE(mcimidi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmm->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				MIDI_mciPause			[internal]
 */
static DWORD MIDI_mciPause(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIMIDI*	wmm = MIDI_mciGetOpenDev(wDevID);
    
    TRACE(mcimidi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmm == NULL)	return MCIERR_INVALID_DEVICE_ID;
    
    if (wmm->dwStatus == MCI_MODE_PLAY) {
	/* stop all notes */
	/* see note in MIDI_mciPlay */
	unsigned chn;
	for (chn = 0; chn < 16; chn++)
	    modMessage(wDevID, MODM_DATA, 0, 0x78B0 | chn, 0);
	wmm->dwStatus = MCI_MODE_PAUSE;
    } 
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE(mcimidi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmm->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    
    return 0;
}

/**************************************************************************
 * 				MIDI_mciResume			[internal]
 */
static DWORD MIDI_mciResume(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIMIDI*	wmm = MIDI_mciGetOpenDev(wDevID);
    
    TRACE(mcimidi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wmm == NULL)	return MCIERR_INVALID_DEVICE_ID;
    
    if (wmm->dwStatus == MCI_MODE_PAUSE) {
	wmm->wStartedPlaying = FALSE;
	wmm->dwStatus = MCI_MODE_PLAY;
    } 
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE(mcimidi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmm->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				MIDI_mciSet			[internal]
 */
static DWORD MIDI_mciSet(UINT16 wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
{
    WINE_MCIMIDI*	wmm = MIDI_mciGetOpenDev(wDevID);
    
    TRACE(mcimidi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmm == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (dwFlags & MCI_SET_TIME_FORMAT) {
	switch (lpParms->dwTimeFormat) {
	case MCI_FORMAT_MILLISECONDS:
	    TRACE(mcimidi, "MCI_FORMAT_MILLISECONDS !\n");
	    wmm->dwMciTimeFormat = MCI_FORMAT_MILLISECONDS;
	    break;
	case MCI_FORMAT_SMPTE_24:
	    TRACE(mcimidi, "MCI_FORMAT_SMPTE_24 !\n");
	    wmm->dwMciTimeFormat = MCI_FORMAT_SMPTE_24;
	    break;
	case MCI_FORMAT_SMPTE_25:
	    TRACE(mcimidi, "MCI_FORMAT_SMPTE_25 !\n");
	    wmm->dwMciTimeFormat = MCI_FORMAT_SMPTE_25;
	    break;
	case MCI_FORMAT_SMPTE_30:
	    TRACE(mcimidi, "MCI_FORMAT_SMPTE_30 !\n");
	    wmm->dwMciTimeFormat = MCI_FORMAT_SMPTE_30;
	    break;
	default:
	    WARN(mcimidi, "Bad time format %lu!\n", lpParms->dwTimeFormat);
	    return MCIERR_BAD_TIME_FORMAT;
	}
    }
    if (dwFlags & MCI_SET_VIDEO) {
	TRACE(mcimidi, "No support for video !\n");
	return MCIERR_UNSUPPORTED_FUNCTION;
    }
    if (dwFlags & MCI_SET_DOOR_OPEN) {
	TRACE(mcimidi, "No support for door open !\n");
	return MCIERR_UNSUPPORTED_FUNCTION;
    }
    if (dwFlags & MCI_SET_DOOR_CLOSED) {
	TRACE(mcimidi, "No support for door close !\n");
	return MCIERR_UNSUPPORTED_FUNCTION;
    }
    if (dwFlags & MCI_SET_AUDIO) {
	if (dwFlags & MCI_SET_ON) {
	    TRACE(mcimidi, "MCI_SET_ON audio !\n");
	} else if (dwFlags & MCI_SET_OFF) {
	    TRACE(mcimidi, "MCI_SET_OFF audio !\n");
	} else {
	    WARN(mcimidi, "MCI_SET_AUDIO without SET_ON or SET_OFF\n");
	    return MCIERR_BAD_INTEGER;
	}
	
	if (lpParms->dwAudio & MCI_SET_AUDIO_ALL)
	    TRACE(mcimidi, "MCI_SET_AUDIO_ALL !\n");
	if (lpParms->dwAudio & MCI_SET_AUDIO_LEFT)
	    TRACE(mcimidi, "MCI_SET_AUDIO_LEFT !\n");
	if (lpParms->dwAudio & MCI_SET_AUDIO_RIGHT)
	    TRACE(mcimidi, "MCI_SET_AUDIO_RIGHT !\n");
    }
    
    if (dwFlags & MCI_SEQ_SET_MASTER)
	TRACE(mcimidi, "MCI_SEQ_SET_MASTER !\n");
    if (dwFlags & MCI_SEQ_SET_SLAVE)
	TRACE(mcimidi, "MCI_SEQ_SET_SLAVE !\n");
    if (dwFlags & MCI_SEQ_SET_OFFSET)
	TRACE(mcimidi, "MCI_SEQ_SET_OFFSET !\n");
    if (dwFlags & MCI_SEQ_SET_PORT)
	TRACE(mcimidi, "MCI_SEQ_SET_PORT !\n");
    if (dwFlags & MCI_SEQ_SET_TEMPO)
	TRACE(mcimidi, "MCI_SEQ_SET_TEMPO !\n");
    return 0;
}

/**************************************************************************
 * 				MIDI_mciStatus			[internal]
 */
static DWORD MIDI_mciStatus(UINT16 wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
    WINE_MCIMIDI*	wmm = MIDI_mciGetOpenDev(wDevID);
    
    TRACE(mcimidi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL) 	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmm == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (dwFlags & MCI_STATUS_ITEM) {
	switch (lpParms->dwItem) {
	case MCI_STATUS_CURRENT_TRACK:
	    /* FIXME in Format 2 */
	    lpParms->dwReturn = 1;
	    TRACE(mcimidi, "MCI_STATUS_CURRENT_TRACK => %lu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_LENGTH:
	    if ((dwFlags & MCI_TRACK) && wmm->wFormat == 2) {
		if (lpParms->dwTrack >= wmm->nTracks)
		    return MCIERR_BAD_INTEGER;
		/* FIXME: this is wrong if there is a tempo change inside the file */
		lpParms->dwReturn = MIDI_ConvertPulseToMS(wmm, wmm->tracks[lpParms->dwTrack].dwLength);
	    } else {
		lpParms->dwReturn = MIDI_GetMThdLengthMS(wmm);
	    }
	    lpParms->dwReturn = MIDI_ConvertMSToTimeFormat(wmm, lpParms->dwReturn);
	    TRACE(mcimidi, "MCI_STATUS_LENGTH => %lu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_MODE:
 	    lpParms->dwReturn = wmm->dwStatus;
	    TRACE(mcimidi, "MCI_STATUS_MODE => %lu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_MEDIA_PRESENT:
	    TRACE(mcimidi, "MCI_STATUS_MEDIA_PRESENT => TRUE\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_STATUS_NUMBER_OF_TRACKS:
	    lpParms->dwReturn = (wmm->wFormat == 2) ? wmm->nTracks : 1;
	    TRACE(mcimidi, "MCI_STATUS_NUMBER_OF_TRACKS => %lu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_POSITION:
	    /* FIXME: do I need to use MCI_TRACK ? */
	    lpParms->dwReturn = MIDI_ConvertMSToTimeFormat(wmm, 
							   (dwFlags & MCI_STATUS_START) ? 0 : wmm->dwPositionMS);
	    TRACE(mcimidi, "MCI_STATUS_POSITION %s => %lu\n", 
		  (dwFlags & MCI_STATUS_START) ? "start" : "current", lpParms->dwReturn);
	    break;
	case MCI_STATUS_READY:
	    lpParms->dwReturn = (wmm->dwStatus != MCI_MODE_NOT_READY);
	    TRACE(mcimidi, "MCI_STATUS_READY = %lu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_TIME_FORMAT:
	    lpParms->dwReturn = wmm->dwMciTimeFormat;
	    TRACE(mcimidi, "MCI_STATUS_TIME_FORMAT => %lu\n", lpParms->dwReturn);
	    break;
	case MCI_SEQ_STATUS_DIVTYPE:
	    TRACE(mcimidi, "MCI_SEQ_STATUS_DIVTYPE !\n");
	    if (wmm->nDivision > 0x8000) {
		switch (wmm->nDivision) {
		case 0xE8:	lpParms->dwReturn = MCI_SEQ_DIV_SMPTE_24;	break;	/* -24 */
		case 0xE7:	lpParms->dwReturn = MCI_SEQ_DIV_SMPTE_25;	break;	/* -25 */
		case 0xE3:	lpParms->dwReturn = MCI_SEQ_DIV_SMPTE_30DROP;	break;	/* -29 */ /* is the MCI constant correct ? */
		case 0xE2:	lpParms->dwReturn = MCI_SEQ_DIV_SMPTE_30;	break;	/* -30 */
		default:	FIXME(mcimidi, "There is a bad bad programmer\n");
		}
	    } else {
		lpParms->dwReturn = MCI_SEQ_DIV_PPQN;
	    }
	    break;
	case MCI_SEQ_STATUS_MASTER:
	    TRACE(mcimidi, "MCI_SEQ_STATUS_MASTER !\n");
	    lpParms->dwReturn = 0;
	    break;
	case MCI_SEQ_STATUS_SLAVE:
	    TRACE(mcimidi, "MCI_SEQ_STATUS_SLAVE !\n");
	    lpParms->dwReturn = 0;
	    break;
	case MCI_SEQ_STATUS_OFFSET:
	    TRACE(mcimidi, "MCI_SEQ_STATUS_OFFSET !\n");
	    lpParms->dwReturn = 0;
	    break;
	case MCI_SEQ_STATUS_PORT:
	    TRACE(mcimidi, "MCI_SEQ_STATUS_PORT !\n");
	    lpParms->dwReturn = 0;
	    break;
	case MCI_SEQ_STATUS_TEMPO:
	    TRACE(mcimidi, "MCI_SEQ_STATUS_TEMPO !\n");
	    lpParms->dwReturn = wmm->dwTempo;
	    break;
	default:
	    WARN(mcimidi, "Unknowm command %08lX !\n", lpParms->dwItem);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    } else {
	WARN(mcimidi, "No Status-Item!\n");
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_NOTIFY) {
	TRACE(mcimidi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wmm->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				MIDI_mciGetDevCaps		[internal]
 */
static DWORD MIDI_mciGetDevCaps(UINT16 wDevID, DWORD dwFlags, 
				LPMCI_GETDEVCAPS_PARMS lpParms)
{
    WINE_MCIMIDI*	wmm = MIDI_mciGetOpenDev(wDevID);
    
    TRACE(mcimidi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL) 	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmm == NULL)		return MCIERR_INVALID_DEVICE_ID;
    
    if (dwFlags & MCI_GETDEVCAPS_ITEM) {
	switch (lpParms->dwItem) {
	case MCI_GETDEVCAPS_DEVICE_TYPE:
	    TRACE(mcimidi, "MCI_GETDEVCAPS_DEVICE_TYPE !\n");
	    lpParms->dwReturn = MCI_DEVTYPE_SEQUENCER;
	    break;
	case MCI_GETDEVCAPS_HAS_AUDIO:
	    TRACE(mcimidi, "MCI_GETDEVCAPS_HAS_AUDIO !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_HAS_VIDEO:
	    TRACE(mcimidi, "MCI_GETDEVCAPS_HAS_VIDEO !\n");
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_USES_FILES:
	    TRACE(mcimidi, "MCI_GETDEVCAPS_USES_FILES !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_COMPOUND_DEVICE:
	    TRACE(mcimidi, "MCI_GETDEVCAPS_COMPOUND_DEVICE !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_CAN_EJECT:
	    TRACE(mcimidi, "MCI_GETDEVCAPS_CAN_EJECT !\n");
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_CAN_PLAY:
	    TRACE(mcimidi, "MCI_GETDEVCAPS_CAN_PLAY !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_CAN_RECORD:
	    TRACE(mcimidi, "MCI_GETDEVCAPS_CAN_RECORD !\n");
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_CAN_SAVE:
	    TRACE(mcimidi, "MCI_GETDEVCAPS_CAN_SAVE !\n");
	    lpParms->dwReturn = FALSE;
	    break;
	default:
	    TRACE(mcimidi, "Unknown capability (%08lx) !\n", lpParms->dwItem);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    } else {
	TRACE(mcimidi, "No GetDevCaps-Item !\n");
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    return 0;
}

/**************************************************************************
 * 				MIDI_mciInfo			[internal]
 */
static DWORD MIDI_mciInfo(UINT16 wDevID, DWORD dwFlags, LPMCI_INFO_PARMS32A lpParms)
{
    DWORD		ret = 0;
    LPSTR		str = 0;
    WINE_MCIMIDI*	wmm = MIDI_mciGetOpenDev(wDevID);
    
    TRACE(mcimidi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL || lpParms->lpstrReturn == NULL) {
	ret = MCIERR_NULL_PARAMETER_BLOCK;
    } else if (wmm == NULL) {
	ret = MCIERR_INVALID_DEVICE_ID;
    } else {
	TRACE(mcimidi, "buf=%p, len=%lu\n", lpParms->lpstrReturn, lpParms->dwRetSize);
	
	switch (dwFlags) {
	case MCI_INFO_PRODUCT:
	    str = "Wine's MIDI sequencer";
	    break;
	case MCI_INFO_FILE:
	    str = wmm->openParms.lpstrElementName;
	    break;
#if 0
	    /* FIXME: the following manifest constants are not defined in <WINE>/include/mmsystem.h */
	case MCI_INFO_COPYRIGHT:
	    break;
	case MCI_INFO_NAME:
	    break;
#endif
	default:
	    WARN(mcimidi, "Don't know this info command (%lu)\n", dwFlags);
	    ret = MCIERR_UNRECOGNIZED_COMMAND;
	}
    }
    if (str) {
	ret = MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize, str);
    } else {
	lpParms->lpstrReturn[0] = 0;
    }
    
    return ret;
}

/**************************************************************************
 * 				MIDI_mciSeek			[internal]
 */
static DWORD MIDI_mciSeek(UINT16 wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
    DWORD		ret = 0;
    WINE_MCIMIDI*	wmm = MIDI_mciGetOpenDev(wDevID);
    
    TRACE(mcimidi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL) {
	ret = MCIERR_NULL_PARAMETER_BLOCK;
    } else if (wmm == NULL) {
	ret = MCIERR_INVALID_DEVICE_ID;
    } else {
	MIDI_mciStop(wDevID, MCI_WAIT, 0);
	
	if (dwFlags & MCI_SEEK_TO_START) {
	    wmm->dwPositionMS = 0;
	} else if (dwFlags & MCI_SEEK_TO_END) {
	    wmm->dwPositionMS = 0xFFFFFFFF; /* fixme */
	} else if (dwFlags & MCI_TO) {
	    wmm->dwPositionMS = MIDI_ConvertTimeFormatToMS(wmm, lpParms->dwTo);
	} else {
	    WARN(mcimidi, "dwFlag doesn't tell where to seek to...\n");
	    return MCIERR_MISSING_PARAMETER;
	}
	
	TRACE(mcimidi, "Seeking to position=%lu ms\n", wmm->dwPositionMS);
	
	if (dwFlags & MCI_NOTIFY) {
	    TRACE(mcimidi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	    mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			      wmm->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
	}
    }
    return ret;	
}    
#endif

/*======================================================================*
 *                  	    MIDI entry points 				*
 *======================================================================*/

/**************************************************************************
 * 				MCIMIDI_DriverProc32	[sample driver]
 */
LONG WINAPI MCIMIDI_DriverProc32(DWORD dwDevID, HDRVR16 hDriv, DWORD wMsg, 
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
    case DRV_CONFIGURE:		MessageBox32A(0, "Sample Midi Linux Driver !", "MMLinux Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
#ifdef SNDCTL_MIDI_INFO
    case MCI_OPEN_DRIVER:	return MIDI_mciOpen      (dwDevID, dwParam1, (LPMCI_OPEN_PARMS32A)   dwParam2);
    case MCI_CLOSE_DRIVER:	return MIDI_mciClose     (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)   dwParam2);
    case MCI_PLAY:		return MIDI_mciPlay      (dwDevID, dwParam1, (LPMCI_PLAY_PARMS)      dwParam2);
    case MCI_RECORD:		return MIDI_mciRecord    (dwDevID, dwParam1, (LPMCI_RECORD_PARMS)    dwParam2);
    case MCI_STOP:		return MIDI_mciStop      (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)   dwParam2);
    case MCI_SET:		return MIDI_mciSet       (dwDevID, dwParam1, (LPMCI_SET_PARMS)       dwParam2);
    case MCI_PAUSE:		return MIDI_mciPause     (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)   dwParam2);
    case MCI_RESUME:		return MIDI_mciResume    (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)   dwParam2);
    case MCI_STATUS:		return MIDI_mciStatus    (dwDevID, dwParam1, (LPMCI_STATUS_PARMS)    dwParam2);
    case MCI_GETDEVCAPS:	return MIDI_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)dwParam2);
    case MCI_INFO:		return MIDI_mciInfo      (dwDevID, dwParam1, (LPMCI_INFO_PARMS32A)   dwParam2);
    case MCI_SEEK:		return MIDI_mciSeek      (dwDevID, dwParam1, (LPMCI_SEEK_PARMS)      dwParam2);
#else
    case MCI_OPEN_DRIVER:
    case MCI_CLOSE_DRIVER:
    case MCI_PLAY:
    case MCI_RECORD:
    case MCI_STOP:
    case MCI_SET:
    case MCI_PAUSE:
    case MCI_RESUME:
    case MCI_STATUS:
    case MCI_GETDEVCAPS:
    case MCI_INFO:
    case MCI_SEEK:
#endif
    case MCI_LOAD:		
    case MCI_SAVE:		
    case MCI_FREEZE:		
    case MCI_PUT:		
    case MCI_REALIZE:		
    case MCI_UNFREEZE:		
    case MCI_UPDATE:		
    case MCI_WHERE:		
    case MCI_WINDOW:		
    case MCI_STEP:		
    case MCI_SPIN:		
    case MCI_ESCAPE:		
    case MCI_COPY:		
    case MCI_CUT:		
    case MCI_DELETE:		
    case MCI_PASTE:		
	WARN(mcimidi, "Unsupported command=%s\n", MCI_CommandToString(wMsg));
	break;
    case MCI_OPEN:
    case MCI_CLOSE:
	FIXME(mcimidi, "Shouldn't receive a MCI_OPEN or CLOSE message\n");
	break;
    default:			
	TRACE(mcimidi, "Sending msg=%s to default driver proc\n", MCI_CommandToString(wMsg));
	return DefDriverProc32(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return MCIERR_UNRECOGNIZED_COMMAND;
}

    
