/*
 * Sample MIDI Wine Driver for Linux
 *
 * Copyright 1994 Martin Ayotte
 */

#include <stdio.h>
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

#if defined (__HAS_SOUNDCARD_H__)

static LINUX_MIDIIN	MidiInDev[MAX_MIDIINDRV];
static LINUX_MIDIOUT	MidiOutDev[MAX_MIDIOUTDRV];
static LINUX_MCIMIDI	MCIMidiDev[MAX_MCIMIDIDRV];

#endif

/* this is the total number of MIDI devices found */
int MODM_NUMDEVS = 0;

/* this structure holds pointers with information for each MIDI
 * device found.
 */
LPMIDIOUTCAPS16 midiDevices[MAX_MIDIOUTDRV];

/**************************************************************************
 * 			MIDI_NotifyClient			[internal]
 */
static DWORD MIDI_NotifyClient(UINT16 wDevID, WORD wMsg, 
				DWORD dwParam1, DWORD dwParam2)
{
	TRACE(midi,"wDevID = %04X wMsg = %d dwParm1 = %04lX dwParam2 = %04lX\n",wDevID, wMsg, dwParam1, dwParam2);

#if defined(linux) || defined(__FreeBSD__)

	switch (wMsg) {
	case MOM_OPEN:
	case MOM_CLOSE:
	case MOM_DONE:
	  if (wDevID > MAX_MIDIOUTDRV) return MCIERR_INTERNAL;
	  
	  if (MidiOutDev[wDevID].wFlags != DCB_NULL && !DriverCallback(
		MidiOutDev[wDevID].midiDesc.dwCallback, 
		MidiOutDev[wDevID].wFlags, 
		MidiOutDev[wDevID].midiDesc.hMidi, 
                wMsg, 
		MidiOutDev[wDevID].midiDesc.dwInstance, 
                dwParam1, 
                dwParam2)) {
	    WARN(midi,"can't notify client !\n");
	    return MMSYSERR_NOERROR;
	  }
	  break;

	case MIM_OPEN:
	case MIM_CLOSE:
	  if (wDevID > MAX_MIDIINDRV) return MCIERR_INTERNAL;
	  
	if (MidiInDev[wDevID].wFlags != DCB_NULL && !DriverCallback(
		MidiInDev[wDevID].midiDesc.dwCallback, MidiInDev[wDevID].wFlags, 
		MidiInDev[wDevID].midiDesc.hMidi, wMsg, 
		MidiInDev[wDevID].midiDesc.dwInstance, dwParam1, dwParam2)) {
	    WARN(mciwave,"can't notify client !\n");
		return MMSYSERR_NOERROR;
		}
	  break;
	}
        return 0;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
 * 				MIDI_ReadByte			[internal]	
 */
static DWORD MIDI_ReadByte(UINT16 wDevID, BYTE *lpbyt)
{
#if defined(linux) || defined(__FreeBSD__)
	if (lpbyt != NULL) {
		if (mmioRead(MCIMidiDev[wDevID].hFile, (HPSTR)lpbyt,
			(long) sizeof(BYTE)) == (long) sizeof(BYTE)) {
			return 0;
		}
	}
	WARN(midi, "error reading wDevID=%04X\n", wDevID);
	return MCIERR_INTERNAL;

#else
        return MMSYSERR_NOTENABLED;
#endif
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
static DWORD MIDI_ReadVaryLen(UINT16 wDevID, LPDWORD lpdw)
{
	BYTE	byte;
	DWORD	value;
	if (lpdw == NULL) return MCIERR_INTERNAL;
	if (MIDI_ReadByte(wDevID, &byte) != 0) {
		WARN(midi, "error reading wDevID=%04X\n", wDevID);
		return MCIERR_INTERNAL;
	}
	value = (DWORD)(byte & 0x7F);
	while (byte & 0x80) {
		if (MIDI_ReadByte(wDevID, &byte) != 0) {
			WARN(midi, "error reading wDevID=%04X\n", wDevID);
			return MCIERR_INTERNAL;
		}
		value = (value << 7) + (byte & 0x7F);
	}
	*lpdw = value;
/*
	TRACE(midi, "val=%08lX \n", value);
*/
	return 0;
}


/**************************************************************************
 * 				MIDI_ReadMThd			[internal]	
 */
static DWORD MIDI_ReadMThd(UINT16 wDevID, DWORD dwOffset)
{
#if defined(linux) || defined(__FreeBSD__)
	DWORD	toberead;
	FOURCC	fourcc;
	TRACE(midi, "(%04X, %08lX);\n", wDevID, dwOffset);
	if (mmioSeek(MCIMidiDev[wDevID].hFile, dwOffset, SEEK_SET) != dwOffset) {
		WARN(midi, "can't seek at %08lX begin of 'MThd' \n", dwOffset);
		return MCIERR_INTERNAL;
	}
	if (mmioRead(MCIMidiDev[wDevID].hFile, (HPSTR)&fourcc,
		(long) sizeof(FOURCC)) != (long) sizeof(FOURCC))
		return MCIERR_INTERNAL;
	if (MIDI_ReadLong(wDevID, &toberead) != 0)
		return MCIERR_INTERNAL;
	if (MIDI_ReadWord(wDevID, &MCIMidiDev[wDevID].wFormat) != 0)
		return MCIERR_INTERNAL;
	if (MIDI_ReadWord(wDevID, &MCIMidiDev[wDevID].nTracks) != 0)
		return MCIERR_INTERNAL;
	if (MIDI_ReadWord(wDevID, &MCIMidiDev[wDevID].nTempo) != 0)
		return MCIERR_INTERNAL;
	TRACE(midi, "toberead=%08lX, wFormat=%04X nTracks=%04X nTempo=%04X\n",
		toberead, MCIMidiDev[wDevID].wFormat,
		MCIMidiDev[wDevID].nTracks,
		MCIMidiDev[wDevID].nTempo);
	toberead -= 3 * sizeof(WORD);
/*
		ntrks = read16bit ();
		Mf_division = division = read16bit ();
*/
	return 0;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


static DWORD MIDI_ReadMTrk(UINT16 wDevID, DWORD dwOffset)
{
#if defined(linux) || defined(__FreeBSD__)
	DWORD	toberead;
	FOURCC	fourcc;
	if (mmioSeek(MCIMidiDev[wDevID].hFile, dwOffset, SEEK_SET) != dwOffset) {
		WARN(midi, "can't seek at %08lX begin of 'MThd' \n", dwOffset);
		}
	if (mmioRead(MCIMidiDev[wDevID].hFile, (HPSTR)&fourcc,
		(long) sizeof(FOURCC)) != (long) sizeof(FOURCC)) {
		return MCIERR_INTERNAL;
		}
	if (MIDI_ReadLong(wDevID, &toberead) != 0) {
		return MCIERR_INTERNAL;
		}
	TRACE(midi, "toberead=%08lX\n", toberead);
	toberead -= 3 * sizeof(WORD);
	MCIMidiDev[wDevID].dwTotalLen = toberead;
	return 0;
#else
         return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
 * 				MIDI_mciOpen			[internal]	
 */
static DWORD MIDI_mciOpen(UINT16 wDevID, DWORD dwFlags, LPMCI_OPEN_PARMS16 lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
	MIDIOPENDESC 	MidiDesc;
	DWORD		dwRet;
	DWORD		dwOffset;
	LPSTR		lpstrElementName;
	char		str[128];

	TRACE(midi, "(%08lX, %p)\n", dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;

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

	TRACE(midi, "wDevID=%04X\n", wDevID);
/*	lpParms->wDeviceID = wDevID;*/
	TRACE(midi, "lpParms->wDevID=%04X\n", lpParms->wDeviceID);
	TRACE(midi, "before OPEN_ELEMENT\n");
	if (dwFlags & MCI_OPEN_ELEMENT) {
		lpstrElementName = (LPSTR)PTR_SEG_TO_LIN(lpParms->lpstrElementName);
		TRACE(midi, "MCI_OPEN_ELEMENT '%s' !\n", lpstrElementName);
		if (strlen(lpstrElementName) > 0) {
			strcpy(str, lpstrElementName);
			CharUpper32A(str);
			MCIMidiDev[wDevID].hFile = mmioOpen16(str, NULL, 
				MMIO_ALLOCBUF | MMIO_READWRITE | MMIO_EXCLUSIVE);
			if (MCIMidiDev[wDevID].hFile == 0) {
				WARN(midi, "can't find file='%s' !\n", str);
				return MCIERR_FILE_NOT_FOUND;
			}
		} else 
			MCIMidiDev[wDevID].hFile = 0;
	}
	TRACE(midi, "hFile=%u\n", MCIMidiDev[wDevID].hFile);
	memcpy(&MCIMidiDev[wDevID].openParms, lpParms, sizeof(MCI_OPEN_PARMS16));
	MCIMidiDev[wDevID].wNotifyDeviceID = lpParms->wDeviceID;
	MCIMidiDev[wDevID].dwStatus = MCI_MODE_STOP;
	MCIMidiDev[wDevID].dwBeginData = 0;
	MCIMidiDev[wDevID].dwTotalLen = 0;
	MidiDesc.hMidi = 0;
	if (MCIMidiDev[wDevID].hFile != 0) {
		MMCKINFO	ckMainRIFF;
		if (mmioDescend(MCIMidiDev[wDevID].hFile, &ckMainRIFF, NULL, 0) != 0) {
			return MCIERR_INTERNAL;
		}
		TRACE(midi,"ParentChunk ckid=%.4s fccType=%.4s cksize=%08lX \n",
				(LPSTR)&ckMainRIFF.ckid, (LPSTR)&ckMainRIFF.fccType,
				ckMainRIFF.cksize);
		dwOffset = 0;
		if (ckMainRIFF.ckid == mmioFOURCC('R', 'M', 'I', 'D')) {
			TRACE(midi, "is a 'RMID' file \n");
			dwOffset = ckMainRIFF.dwDataOffset;
		}
		if (ckMainRIFF.ckid != mmioFOURCC('M', 'T', 'h', 'd')) {
			WARN(midi, "unknown format !\n");
			return MCIERR_INTERNAL;
		}
		if (MIDI_ReadMThd(wDevID, dwOffset) != 0) {
			WARN(midi, "can't read 'MThd' header \n");
			return MCIERR_INTERNAL;
		}
		dwOffset = mmioSeek(MCIMidiDev[wDevID].hFile, 0, SEEK_CUR);
		if (MIDI_ReadMTrk(wDevID, dwOffset) != 0) {
			WARN(midi, "can't read 'MTrk' header \n");
			return MCIERR_INTERNAL;
		}
		dwOffset = mmioSeek(MCIMidiDev[wDevID].hFile, 0, SEEK_CUR);
		MCIMidiDev[wDevID].dwBeginData = dwOffset;
		TRACE(midi, "Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX \n",
				(LPSTR)&ckMainRIFF.ckid, (LPSTR)&ckMainRIFF.fccType,
				ckMainRIFF.cksize);
	}

	dwRet = modMessage(wDevID, MODM_OPEN, 0, (DWORD)&MidiDesc, CALLBACK_NULL);
/*	dwRet = midMessage(wDevID, MIDM_OPEN, 0, (DWORD)&MidiDesc, CALLBACK_NULL);*/

	return 0;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
 * 				MIDI_mciStop			[internal]
 */
static DWORD MIDI_mciStop(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
	TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	MCIMidiDev[wDevID].dwStatus = MCI_MODE_STOP;
	TRACE(midi, "MCIMidiDev[wDevID].dwStatus=%p %d\n",
			&MCIMidiDev[wDevID].dwStatus, MCIMidiDev[wDevID].dwStatus);
	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}


/**************************************************************************
 * 				MIDI_mciClose		[internal]
 */
static DWORD MIDI_mciClose(UINT16 wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
	DWORD		dwRet;

	TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwParam, lpParms);
	if (MCIMidiDev[wDevID].dwStatus != MCI_MODE_STOP) {
		MIDI_mciStop(wDevID, MCI_WAIT, lpParms);
		}
	MCIMidiDev[wDevID].dwStatus = MCI_MODE_STOP;
	MCIMidiDev[wDevID].nUseCount--;
	if (MCIMidiDev[wDevID].nUseCount == 0) {
		if (MCIMidiDev[wDevID].hFile != 0) {
			mmioClose(MCIMidiDev[wDevID].hFile, 0);
			MCIMidiDev[wDevID].hFile = 0;
			TRACE(midi, "hFile closed !\n");
			}
		USER_HEAP_FREE(MCIMidiDev[wDevID].hMidiHdr);
		dwRet = modMessage(wDevID, MODM_CLOSE, 0, 0L, 0L);
		if (dwRet != MMSYSERR_NOERROR) return MCIERR_INTERNAL;
/*
		dwRet = midMessage(wDevID, MIDM_CLOSE, 0, 0L, 0L);
		if (dwRet != MMSYSERR_NOERROR) return MCIERR_INTERNAL;
*/
		}
	return 0;
#else
	return 0;
#endif
}


/**************************************************************************
 * 				MIDI_mciPlay		[internal]
 */
static DWORD MIDI_mciPlay(UINT16 wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
	int		count,start,end;
	LPMIDIHDR	lpMidiHdr;
	DWORD		dwData,dwRet;
	LPWORD		ptr;

	TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
	if (MCIMidiDev[wDevID].hFile == 0) {
		WARN(midi, "can't find file='%08lx' !\n", 
				(DWORD)MCIMidiDev[wDevID].openParms.lpstrElementName);
		return MCIERR_FILE_NOT_FOUND;
	}
	start = 1; 		end = 99999;
	if (dwFlags & MCI_FROM) {
		start = lpParms->dwFrom; 
		TRACE(midi, "MCI_FROM=%d \n", start);
	}
	if (dwFlags & MCI_TO) {
		end = lpParms->dwTo;
		TRACE(midi, "MCI_TO=%d \n", end);
	}
#if 0
	if (dwFlags & MCI_NOTIFY) {
		TRACE(midi, "MCI_NOTIFY %08lX !\n", lpParms->dwCallback);
		switch(fork()) {
		case -1:
			WARN(midi, "Can't 'fork' process !\n");
			break;
		case 0:
			TRACE(midi, "process started ! play in background ...\n");
			break;
		default:
			TRACE(midi, "process started ! return to caller...\n");
			return 0;
		}
	}
#endif

	lpMidiHdr = USER_HEAP_LIN_ADDR(MCIMidiDev[wDevID].hMidiHdr);

	lpMidiHdr->lpData = (LPSTR)xmalloc(1200);
	if (lpMidiHdr->lpData == NULL) return MCIERR_INTERNAL;
	lpMidiHdr->dwBufferLength = 1024;
	lpMidiHdr->dwUser = 0L;
	lpMidiHdr->dwFlags = 0L;
	dwRet = modMessage(wDevID, MODM_PREPARE, 0, (DWORD)lpMidiHdr, sizeof(MIDIHDR));

/*	TRACE(midi, "after MODM_PREPARE \n"); */

	MCIMidiDev[wDevID].dwStatus = MCI_MODE_PLAY;
	while(MCIMidiDev[wDevID].dwStatus != MCI_MODE_STOP) {
		TRACE(midi, "MCIMidiDev[wDevID].dwStatus=%p %d\n",
			&MCIMidiDev[wDevID].dwStatus, MCIMidiDev[wDevID].dwStatus);

		ptr = (LPWORD)lpMidiHdr->lpData;
		for (count = 0; count < lpMidiHdr->dwBufferLength; count++) {
			if (MIDI_ReadVaryLen(wDevID, &dwData) != 0) break;
			*ptr = LOWORD(dwData);
		}
/*
		count = mmioRead(MCIMidiDev[wDevID].hFile, lpMidiHdr->lpData, lpMidiHdr->dwBufferLength);
*/
		TRACE(midi, "after read count = %d\n",count);

		if (count < 1) break;
		lpMidiHdr->dwBytesRecorded = count;
		TRACE(midi, "before MODM_LONGDATA lpMidiHdr=%p dwBytesRecorded=%lu\n",
					lpMidiHdr, lpMidiHdr->dwBytesRecorded);
		dwRet = modMessage(wDevID, MODM_LONGDATA, 0, (DWORD)lpMidiHdr, sizeof(MIDIHDR));
		if (dwRet != MMSYSERR_NOERROR) {
		  switch (dwRet) {
		  case MMSYSERR_NOTENABLED:
		    return MCIERR_DEVICE_NOT_READY;
		    
		  case MIDIERR_NODEVICE:
		    return MCIERR_INVALID_DEVICE_ID;

		  case MIDIERR_UNPREPARED:
		    return MCIERR_DRIVER_INTERNAL;

		  case MIDIERR_STILLPLAYING:
		    return MCIERR_SEQ_PORT_INUSE;

		  case MMSYSERR_INVALPARAM:
		    return MCIERR_CANNOT_LOAD_DRIVER;
		  
		  default:
		    return MCIERR_DRIVER;
		  }	
		}
	      }
	dwRet = modMessage(wDevID, MODM_UNPREPARE, 0, (DWORD)lpMidiHdr, sizeof(MIDIHDR));
	if (lpMidiHdr->lpData != NULL) {
		free(lpMidiHdr->lpData);
		lpMidiHdr->lpData = NULL;
	}
	MCIMidiDev[wDevID].dwStatus = MCI_MODE_STOP;
	if (dwFlags & MCI_NOTIFY) {
		TRACE(midi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
		mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
			MCIMidiDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
#if 0
		exit(1);
#endif
	}
	return 0;
#else
	return MMSYSERR_NOTENABLED;
#endif
      }


/**************************************************************************
 * 				MIDI_mciRecord			[internal]
 */
static DWORD MIDI_mciRecord(UINT16 wDevID, DWORD dwFlags, LPMCI_RECORD_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
	int			start, end;
	LPMIDIHDR	lpMidiHdr;
	DWORD		dwRet;

	TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
	if (MCIMidiDev[wDevID].hFile == 0) {
		WARN(midi, "can't find file='%08lx' !\n", 
			(DWORD)MCIMidiDev[wDevID].openParms.lpstrElementName);
		return MCIERR_FILE_NOT_FOUND;
	}
	start = 1; 		end = 99999;
	if (dwFlags & MCI_FROM) {
		start = lpParms->dwFrom; 
		TRACE(midi, "MCI_FROM=%d \n", start);
	}
	if (dwFlags & MCI_TO) {
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
	while(MCIMidiDev[wDevID].dwStatus != MCI_MODE_STOP) {
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
	if (dwFlags & MCI_NOTIFY) {
		TRACE(midi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
		mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
			MCIMidiDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
	}
	return 0;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
 * 				MIDI_mciPause			[internal]
 */
static DWORD MIDI_mciPause(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
	TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}


/**************************************************************************
 * 				MIDI_mciResume			[internal]
 */
static DWORD MIDI_mciResume(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
	TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}


/**************************************************************************
 * 				MIDI_mciSet			[internal]
 */
static DWORD MIDI_mciSet(UINT16 wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
	TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
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
	if (dwFlags & MCI_SET_VIDEO) return MCIERR_UNSUPPORTED_FUNCTION;
	if (dwFlags & MCI_SET_DOOR_OPEN) return MCIERR_UNSUPPORTED_FUNCTION;
	if (dwFlags & MCI_SET_DOOR_CLOSED) return MCIERR_UNSUPPORTED_FUNCTION;
	if (dwFlags & MCI_SET_AUDIO)
		TRACE(midi, "MCI_SET_AUDIO !\n");
	if (dwFlags && MCI_SET_ON) {
		TRACE(midi, "MCI_SET_ON !\n");
		if (dwFlags && MCI_SET_AUDIO_LEFT)
			TRACE(midi, "MCI_SET_AUDIO_LEFT !\n");
		if (dwFlags && MCI_SET_AUDIO_RIGHT)
			TRACE(midi, "MCI_SET_AUDIO_RIGHT !\n");
	}
	if (dwFlags & MCI_SET_OFF)
		TRACE(midi, "MCI_SET_OFF !\n");
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
#else
	return MCIERR_INTERNAL;
#endif
}


/**************************************************************************
 * 				MIDI_mciStatus		[internal]
 */
static DWORD MIDI_mciStatus(UINT16 wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
	TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	if (dwFlags & MCI_STATUS_ITEM) {
		switch(lpParms->dwItem) {
		case MCI_STATUS_CURRENT_TRACK:
			lpParms->dwReturn = 1;
			break;
		case MCI_STATUS_LENGTH:
			lpParms->dwReturn = 5555;
			if (dwFlags & MCI_TRACK) {
				lpParms->dwTrack = 1;
				lpParms->dwReturn = 2222;
			}
			break;
		case MCI_STATUS_MODE:
			lpParms->dwReturn = MCI_MODE_STOP;
			break;
		case MCI_STATUS_MEDIA_PRESENT:
			TRACE(midi, "MCI_STATUS_MEDIA_PRESENT !\n");
			lpParms->dwReturn = TRUE;
			break;
		case MCI_STATUS_NUMBER_OF_TRACKS:
			lpParms->dwReturn = 1;
			break;
		case MCI_STATUS_POSITION:
			lpParms->dwReturn = 3333;
			if (dwFlags & MCI_STATUS_START)
				lpParms->dwItem = 1;
			if (dwFlags & MCI_TRACK) {
				lpParms->dwTrack = 1;
				lpParms->dwReturn = 777;
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
			lpParms->dwReturn = 0;
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
			lpParms->dwReturn = 0;
			break;
		default:
			WARN(midi, "unknowm command %08lX !\n", lpParms->dwItem);
			return MCIERR_UNRECOGNIZED_COMMAND;
		}
	}
	if (dwFlags & MCI_NOTIFY) {
		TRACE(midi, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
		mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
			MCIMidiDev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
	}
 	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}

/**************************************************************************
 * 				MIDI_mciGetDevCaps		[internal]
 */
static DWORD MIDI_mciGetDevCaps(UINT16 wDevID, DWORD dwFlags, 
					LPMCI_GETDEVCAPS_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
	TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	if (dwFlags & MCI_GETDEVCAPS_ITEM) {
		switch(lpParms->dwItem) {
		case MCI_GETDEVCAPS_CAN_RECORD:
			lpParms->dwReturn = TRUE;
			break;
		case MCI_GETDEVCAPS_HAS_AUDIO:
			lpParms->dwReturn = TRUE;
			break;
		case MCI_GETDEVCAPS_HAS_VIDEO:
			lpParms->dwReturn = FALSE;
			break;
		case MCI_GETDEVCAPS_DEVICE_TYPE:
			lpParms->dwReturn = MCI_DEVTYPE_SEQUENCER;
			break;
		case MCI_GETDEVCAPS_USES_FILES:
			lpParms->dwReturn = TRUE;
			break;
		case MCI_GETDEVCAPS_COMPOUND_DEVICE:
			lpParms->dwReturn = TRUE;
			break;
		case MCI_GETDEVCAPS_CAN_EJECT:
			lpParms->dwReturn = FALSE;
			break;
		case MCI_GETDEVCAPS_CAN_PLAY:
			lpParms->dwReturn = TRUE;
			break;
		case MCI_GETDEVCAPS_CAN_SAVE:
			lpParms->dwReturn = FALSE;
			break;
		default:
			return MCIERR_UNRECOGNIZED_COMMAND;
		}
	}
 	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}


/**************************************************************************
 * 				MIDI_mciInfo			[internal]
 */
static DWORD MIDI_mciInfo(UINT16 wDevID, DWORD dwFlags, LPMCI_INFO_PARMS16 lpParms)
{
# if defined(__FreeBSD__) || defined (linux)
	TRACE(midi, "(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
	if (lpParms == NULL) return MCIERR_INTERNAL;
	lpParms->lpstrReturn = NULL;
	switch(dwFlags) {
	case MCI_INFO_PRODUCT:
		lpParms->lpstrReturn = "Linux Sound System 0.5";
		break;
	case MCI_INFO_FILE:
		lpParms->lpstrReturn = "FileName";
		break;
	default:
		return MCIERR_UNRECOGNIZED_COMMAND;
	}
	if (lpParms->lpstrReturn != NULL)
		lpParms->dwRetSize = strlen(lpParms->lpstrReturn);
	else
		lpParms->dwRetSize = 0;
 	return 0;
#else
	return MCIERR_INTERNAL;
#endif
}


/*-----------------------------------------------------------------------*/


/**************************************************************************
 * 				midGetDevCaps			[internal]
 */
static DWORD midGetDevCaps(WORD wDevID, LPMIDIINCAPS16 lpCaps, DWORD dwSize)
{
	TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpCaps, dwSize);
	lpCaps->wMid = 0x00FF; 	        /* Manufac ID */
	lpCaps->wPid = 0x0001; 	        /* Product ID */
	lpCaps->vDriverVersion = 0x001; /* Product Version */
	strcpy(lpCaps->szPname, "Linux MIDIIN Driver");

	return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			midOpen					[internal]
 */
static DWORD midOpen(WORD wDevID, LPMIDIOPENDESC lpDesc, DWORD dwFlags)
{
#if defined(linux) || defined(__FreeBSD__)
	int		midi;
	TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpDesc, dwFlags);
	if (lpDesc == NULL) {
		WARN(midi, "Invalid Parameter !\n");
		return MMSYSERR_INVALPARAM;
	}
	if (wDevID >= MAX_MIDIINDRV) {
		TRACE(midi,"MAX_MIDIINDRV reached !\n");
		return MMSYSERR_ALLOCATED;
	}
	MidiInDev[wDevID].unixdev = 0;
	midi = open (MIDI_DEV, O_RDONLY, 0);
	if (midi == -1) {
		WARN(midi,"can't open !\n");
		return MMSYSERR_NOTENABLED;
	}
	MidiInDev[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
	switch(MidiInDev[wDevID].wFlags) {
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
	MidiInDev[wDevID].lpQueueHdr = NULL;
	MidiInDev[wDevID].unixdev = midi;
	MidiInDev[wDevID].dwTotalPlayed = 0;
	MidiInDev[wDevID].bufsize = 0x3FFF;
	if (MIDI_NotifyClient(wDevID, MIM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
		WARN(midi,"can't notify client !\n");
		return MMSYSERR_INVALPARAM;
	}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
 * 			midClose				[internal]
 */
static DWORD midClose(WORD wDevID)
{
#if defined(linux) || defined(__FreeBSD__)
	TRACE(midi, "(%04X);\n", wDevID);
	if (MidiInDev[wDevID].unixdev == 0) {
		WARN(midi,"can't close !\n");
		return MMSYSERR_NOTENABLED;
	}
	close(MidiInDev[wDevID].unixdev);
	MidiInDev[wDevID].unixdev = 0;
	MidiInDev[wDevID].bufsize = 0;
	if (MIDI_NotifyClient(wDevID, MIM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
		WARN(midi,"can't notify client !\n");
		return MMSYSERR_INVALPARAM;
	}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
 * 				midAddBuffer		[internal]
 */
static DWORD midAddBuffer(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
	TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);
	return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				midPrepare			[internal]
 */
static DWORD midPrepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
	TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);
	return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				midUnprepare			[internal]
 */
static DWORD midUnprepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
	TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);
	return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 			midReset				[internal]
 */
static DWORD midReset(WORD wDevID)
{
	TRACE(midi, "(%04X);\n", wDevID);
	return MMSYSERR_NOTENABLED;
}


/**************************************************************************
 * 			midStart				[internal]
 */
static DWORD midStart(WORD wDevID)
{
	TRACE(midi, "(%04X);\n", wDevID);
	return MMSYSERR_NOTENABLED;
}


/**************************************************************************
 *			midStop					[internal]
 */
static DWORD midStop(WORD wDevID)
{
	TRACE(midi, "(%04X);\n", wDevID);
	return MMSYSERR_NOTENABLED;
}


/**************************************************************************
 * 			midMessage				[sample driver]
 */
DWORD midMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2)
{
	TRACE(midi, "(%04X, %04X, %08lX, %08lX, %08lX);\n", 
			wDevID, wMsg, dwUser, dwParam1, dwParam2);
	switch(wMsg) {
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
		return 0;
	case MIDM_RESET:
		return midReset(wDevID);
	case MIDM_START:
		return midStart(wDevID);
	case MIDM_STOP:
		return midStop(wDevID);
	}
	return MMSYSERR_NOTSUPPORTED;
}

/*-----------------------------------------------------------------------*/

/**************************************************************************
 * 				modGetDevCaps			[internal]
 */
static DWORD modGetDevCaps(WORD wDevID, LPMIDIOUTCAPS16 lpCaps, DWORD dwSize)
{
  LPMIDIOUTCAPS16 tmplpCaps;

	TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpCaps, dwSize);
  if (wDevID == (WORD) MIDI_MAPPER) { 
	lpCaps->wMid = 0x00FF; 	/* Manufac ID */
	lpCaps->wPid = 0x0001; 	/* Product ID */
	lpCaps->vDriverVersion = 0x001; /* Product Version */
    strcpy(lpCaps->szPname, "MIDI Maper (not functional yet)");
    lpCaps->wTechnology = MOD_FMSYNTH; /* FIXME Does it make any difference ? */
    lpCaps->wVoices     = 14;       /* FIXME */
    lpCaps->wNotes      = 14;       /* FIXME */
    lpCaps->dwSupport   = MIDICAPS_VOLUME|MIDICAPS_LRVOLUME; /* FIXME Does it make any difference ? */
  } else {
    /* FIXME There is a way to do it so easily, but I'm too
     * sleepy to think and I want to test
*/
    tmplpCaps = midiDevices [wDevID];
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
#if defined(linux) || defined(__FreeBSD__)
	int		midi;

	TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpDesc, dwFlags);
	if (lpDesc == NULL) {
		WARN(midi, "Invalid Parameter !\n");
		return MMSYSERR_INVALPARAM;
	}
	if (wDevID>= MAX_MIDIOUTDRV) {
		TRACE(midi,"MAX_MIDIOUTDRV reached !\n");
		return MMSYSERR_ALLOCATED; /* FIXME isn't MMSYSERR_BADDEVICEID the right answer ? */
	}
	MidiOutDev[wDevID].unixdev = 0;
	midi = open (MIDI_DEV, O_WRONLY, 0);
	if (midi == -1) {
		WARN(midi, "can't open !\n");
		return MMSYSERR_NOTENABLED;
	}
	MidiOutDev[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
	switch(MidiOutDev[wDevID].wFlags) {
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
	MidiOutDev[wDevID].unixdev = midi;
	MidiOutDev[wDevID].dwTotalPlayed = 0;
	MidiOutDev[wDevID].bufsize = 0x3FFF;
	if (MIDI_NotifyClient(wDevID, MOM_OPEN, 0L, 0L) != MMSYSERR_NOERROR) {
		WARN(midi,"can't notify client !\n");
		return MMSYSERR_INVALPARAM;
	}
	TRACE(midi, "Succesful unixdev=%d !\n", midi);
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
 * 			modClose				[internal]
 */
static DWORD modClose(WORD wDevID)
{
#if defined(linux) || defined(__FreeBSD__)
	TRACE(midi, "(%04X);\n", wDevID);
	if (MidiOutDev[wDevID].unixdev == 0) {
		WARN(midi,"can't close !\n");
		return MMSYSERR_NOTENABLED;
	}
	close(MidiOutDev[wDevID].unixdev);
	MidiOutDev[wDevID].unixdev = 0;
	MidiOutDev[wDevID].bufsize = 0;
	if (MIDI_NotifyClient(wDevID, MOM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
		WARN(midi,"can't notify client !\n");
		return MMSYSERR_INVALPARAM;
	}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
 * 			modData					[internal]
 */
static DWORD modData(WORD wDevID, DWORD dwParam)
{
#if defined(linux) || defined(__FreeBSD__)
	WORD	event;

	TRACE(midi, "(%04X, %08lX);\n", wDevID, dwParam);
	if (MidiOutDev[wDevID].unixdev == 0) {
        	WARN(midi,"can't play !\n");
		return MIDIERR_NODEVICE;
	}
	event = LOWORD(dwParam);
	if (write (MidiOutDev[wDevID].unixdev, 
		&event, sizeof(WORD)) != sizeof(WORD)) {
		WARN(midi, "error writting unixdev !\n");
	}
	return MMSYSERR_NOTENABLED;
#else
        return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
 *		modLongData					[internal]
 */
static DWORD modLongData(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
#if defined(linux) || defined(__FreeBSD__)
	int		count;
	LPWORD	ptr;
	int     en;

	TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);
	if (MidiOutDev[wDevID].unixdev == 0) {
        WARN(midi,"can't play !\n");
		return MIDIERR_NODEVICE;
		}
	if (lpMidiHdr->lpData == NULL) return MIDIERR_UNPREPARED;
	if (!(lpMidiHdr->dwFlags & MHDR_PREPARED)) return MIDIERR_UNPREPARED;
	if (lpMidiHdr->dwFlags & MHDR_INQUEUE) return MIDIERR_STILLPLAYING;
	lpMidiHdr->dwFlags &= ~MHDR_DONE;
	lpMidiHdr->dwFlags |= MHDR_INQUEUE;
	TRACE(midi, "dwBytesRecorded %lu !\n", lpMidiHdr->dwBytesRecorded);
	TRACE(midi, "                 %02X %02X %02X %02X\n",
		     lpMidiHdr->lpData[0], lpMidiHdr->lpData[1],
		     lpMidiHdr->lpData[2], lpMidiHdr->lpData[3]);
/*
	count = write (MidiOutDev[wDevID].unixdev, 
		lpMidiHdr->lpData, lpMidiHdr->dwBytesRecorded);
*/
	ptr = (LPWORD)lpMidiHdr->lpData;
	for (count = 0; count < lpMidiHdr->dwBytesRecorded; count++) {
		if (write (MidiOutDev[wDevID].unixdev, ptr, 
			sizeof(WORD)) != sizeof(WORD)) break;
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
	lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
	lpMidiHdr->dwFlags |= MHDR_DONE;
	if (MIDI_NotifyClient(wDevID, MOM_DONE, 0L, 0L) != MMSYSERR_NOERROR) {
		WARN(midi,"can't notify client !\n");
		return MMSYSERR_INVALPARAM;
	}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
 * 			modPrepare				[internal]
 */
static DWORD modPrepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
#if defined(linux) || defined(__FreeBSD__)
	TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);
	if (MidiOutDev[wDevID].unixdev == 0) {
		WARN(midi,"can't prepare !\n");
		return MMSYSERR_NOTENABLED;
	}
	if (MidiOutDev[wDevID].lpQueueHdr != NULL) {
		TRACE(midi,"already prepare !\n");
		return MMSYSERR_NOTENABLED;
	}
	MidiOutDev[wDevID].dwTotalPlayed = 0;
	MidiOutDev[wDevID].lpQueueHdr = PTR_SEG_TO_LIN(lpMidiHdr);
	if (lpMidiHdr->dwFlags & MHDR_INQUEUE) return MIDIERR_STILLPLAYING;
	lpMidiHdr->dwFlags |= MHDR_PREPARED;
	lpMidiHdr->dwFlags &= ~MHDR_DONE;
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
 * 				modUnprepare			[internal]
 */
static DWORD modUnprepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
#if defined(linux) || defined(__FreeBSD__)
	TRACE(midi, "(%04X, %p, %08lX);\n", wDevID, lpMidiHdr, dwSize);
	if (MidiOutDev[wDevID].unixdev == 0) {
		WARN(midi,"can't unprepare !\n");
		return MMSYSERR_NOTENABLED;
	}
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
 * 			modReset				[internal]
 */
static DWORD modReset(WORD wDevID)
{
	TRACE(midi, "(%04X);\n", wDevID);
	return MMSYSERR_NOTENABLED;
}


/**************************************************************************
 * 				modMessage			[sample driver]
 */
DWORD modMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2)
{
	TRACE(midi, "(%04X, %04X, %08lX, %08lX, %08lX);\n", 
			wDevID, wMsg, dwUser, dwParam1, dwParam2);
	switch(wMsg) {
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
	}
	return MMSYSERR_NOTSUPPORTED;
}


/**************************************************************************
 * 				MIDI_DriverProc		[sample driver]
 */
LONG MIDI_DriverProc(DWORD dwDevID, HDRVR16 hDriv, WORD wMsg, 
		     DWORD dwParam1, DWORD dwParam2)
{
#if defined(linux) || defined(__FreeBSD__)
	switch(wMsg) {
	case DRV_LOAD:
		return 1;
	case DRV_FREE:
		return 1;
	case DRV_OPEN:
		return 1;
	case DRV_CLOSE:
		return 1;
	case DRV_ENABLE:
		return 1;
	case DRV_DISABLE:
		return 1;
	case DRV_QUERYCONFIGURE:
		return 1;
	case DRV_CONFIGURE:
		MessageBox16(0, "Sample Midi Linux Driver !", 
			     "MMLinux Driver", MB_OK);
		return 1;
	case DRV_INSTALL:
		return DRVCNF_RESTART;
	case DRV_REMOVE:
		return DRVCNF_RESTART;
	case MCI_OPEN_DRIVER:
	case MCI_OPEN:
		return MIDI_mciOpen(dwDevID, dwParam1, (LPMCI_OPEN_PARMS16)PTR_SEG_TO_LIN(dwParam2));
	case MCI_CLOSE_DRIVER:
	case MCI_CLOSE:
		return MIDI_mciClose(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
	case MCI_PLAY:
		return MIDI_mciPlay(dwDevID, dwParam1, (LPMCI_PLAY_PARMS)PTR_SEG_TO_LIN(dwParam2));
	case MCI_RECORD:
		return MIDI_mciRecord(dwDevID, dwParam1, (LPMCI_RECORD_PARMS)PTR_SEG_TO_LIN(dwParam2));
	case MCI_STOP:
		return MIDI_mciStop(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
	case MCI_SET:
		return MIDI_mciSet(dwDevID, dwParam1, (LPMCI_SET_PARMS)PTR_SEG_TO_LIN(dwParam2));
	case MCI_PAUSE:
		return MIDI_mciPause(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
	case MCI_RESUME:
		return MIDI_mciResume(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
	case MCI_STATUS:
		return MIDI_mciStatus(dwDevID, dwParam1, (LPMCI_STATUS_PARMS)PTR_SEG_TO_LIN(dwParam2));
	case MCI_GETDEVCAPS:
		return MIDI_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)PTR_SEG_TO_LIN(dwParam2));
	case MCI_INFO:
		return MIDI_mciInfo(dwDevID, dwParam1, (LPMCI_INFO_PARMS16)PTR_SEG_TO_LIN(dwParam2));
	default:
		return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
	}
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/*-----------------------------------------------------------------------*/
