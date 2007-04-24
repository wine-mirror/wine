/*
 * Sample MIDI Wine Driver for MacOSX (based on OSS midi driver)
 *
 * Copyright 1994 	Martin Ayotte
 * Copyright 1998 	Luiz Otavio L. Zorzella (init procedures)
 * Copyright 1998/1999	Eric POUECH :
 * 		98/7 	changes for making this MIDI driver work on OSS
 * 			current support is limited to MIDI ports of OSS systems
 * 		98/9	rewriting MCI code for MIDI
 * 		98/11 	splitted in midi.c and mcimidi.c
 * Copyright 2006       Emmanuel Maillard
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
#include "wine/port.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "mmddk.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(midi);

#if defined(HAVE_COREAUDIO_COREAUDIO_H)
#include <CoreAudio/CoreAudio.h>

#define WINE_DEFINITIONS
#include "coremidi.h"

static MIDIClientRef wineMIDIClient = NULL;

static DWORD MIDIOut_NumDevs = 0;

typedef struct tagMIDIDestination {
    /* graph and synth are only used for MIDI Synth */
    AUGraph graph;
    AudioUnit synth;

    MIDIPortRef port;
    MIDIOUTCAPSW caps;
    MIDIOPENDESC midiDesc;
    WORD wFlags;
} MIDIDestination;

#define MAX_MIDI_SYNTHS 1

MIDIDestination *destinations;

extern int SynthUnit_CreateDefaultSynthUnit(AUGraph *graph, AudioUnit *synth);
extern int SynthUnit_Initialize(AudioUnit synth, AUGraph graph);
extern int SynthUnit_Close(AUGraph graph);


LONG CoreAudio_MIDIInit(void)
{
    int i;
    CHAR szPname[MAXPNAMELEN] = {0};

    int numDest = MIDIGetNumberOfDestinations();
    CFStringRef name = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("wineMIDIClient.%d"), getpid());

    wineMIDIClient = CoreMIDI_CreateClient( name );
    if (wineMIDIClient == NULL)
    {
        CFRelease(name);
        ERR("can't create wineMIDIClient\n");
        return 0;
    }
    CFRelease(name);

    MIDIOut_NumDevs = MAX_MIDI_SYNTHS;
    MIDIOut_NumDevs += numDest;

    TRACE("MIDIOut_NumDevs %d\n", MIDIOut_NumDevs);

    destinations = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MIDIOut_NumDevs * sizeof(MIDIDestination));

    /* initialise MIDI synths */
    for (i = 0; i < MAX_MIDI_SYNTHS; i++)
    {
        snprintf(szPname, sizeof(szPname), "CoreAudio MIDI Synth %d", i);
        MultiByteToWideChar(CP_ACP, 0, szPname, -1, destinations[i].caps.szPname, sizeof(destinations[i].caps.szPname)/sizeof(WCHAR));

        destinations[i].caps.wTechnology = MOD_SYNTH;
        destinations[i].caps.wChannelMask = 0xFFFF;

        destinations[i].caps.wMid = 0x00FF; 	/* Manufac ID */
        destinations[i].caps.wPid = 0x0001; 	/* Product ID */
        destinations[i].caps.vDriverVersion = 0x0001;
        destinations[i].caps.dwSupport = MIDICAPS_VOLUME;
        destinations[i].caps.wVoices = 16;
        destinations[i].caps.wNotes = 16;
    }
    /* initialise available destinations */
    for (i = MAX_MIDI_SYNTHS; i < numDest + MAX_MIDI_SYNTHS; i++)
    {
        MIDIEndpointRef endpoint = MIDIGetDestination(i - MAX_MIDI_SYNTHS);

        CoreMIDI_GetObjectName(endpoint, szPname, sizeof(szPname));
        MultiByteToWideChar(CP_ACP, 0, szPname, -1, destinations[i].caps.szPname, sizeof(destinations[i].caps.szPname)/sizeof(WCHAR));

        name = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("WineOutputPort.%d.%u"), i, getpid());
        MIDIOutputPortCreate(wineMIDIClient, name, &destinations[i].port);
        CFRelease(name);

        destinations[i].caps.wTechnology = MOD_MIDIPORT;
        destinations[i].caps.wChannelMask = 0xFFFF;

        destinations[i].caps.wMid = 0x00FF; 	/* Manufac ID */
        destinations[i].caps.wPid = 0x0001;
        destinations[i].caps.vDriverVersion = 0x0001;
        destinations[i].caps.dwSupport = 0;
        destinations[i].caps.wVoices = 16;
        destinations[i].caps.wNotes = 16;
    }
    return 1;
}

LONG CoreAudio_MIDIRelease(void)
{
    TRACE("\n");
    if (wineMIDIClient) MIDIClientDispose(wineMIDIClient); /* MIDIClientDispose will close all ports */
    HeapFree(GetProcessHeap(), 0, destinations);
    return 1;
}


/**************************************************************************
 * 			MIDI_NotifyClient			[internal]
 */
static DWORD MIDI_NotifyClient(UINT wDevID, WORD wMsg, DWORD dwParam1, DWORD dwParam2)
{
    DWORD 		dwCallBack;
    UINT 		uFlags;
    HANDLE		hDev;
    DWORD 		dwInstance;

    TRACE("wDevID=%d wMsg=%d dwParm1=%04X dwParam2=%04X\n", wDevID, wMsg, dwParam1, dwParam2);

    switch (wMsg) {
    case MOM_OPEN:
    case MOM_CLOSE:
    case MOM_DONE:
    case MOM_POSITIONCB:
	dwCallBack = destinations[wDevID].midiDesc.dwCallback;
	uFlags = destinations[wDevID].wFlags;
	hDev = destinations[wDevID].midiDesc.hMidi;
	dwInstance = destinations[wDevID].midiDesc.dwInstance;
	break;

    case MIM_OPEN:
    case MIM_CLOSE:
    case MIM_DATA:
    case MIM_LONGDATA:
    case MIM_ERROR:
    case MIM_LONGERROR:
    case MIM_MOREDATA:
    default:
	WARN("Unsupported MSW-MIDI message %u\n", wMsg);
	return MMSYSERR_ERROR;
    }

    return DriverCallback(dwCallBack, uFlags, hDev, wMsg, dwInstance, dwParam1, dwParam2) ?
        MMSYSERR_NOERROR : MMSYSERR_ERROR;
}

static DWORD MIDIOut_Open(WORD wDevID, LPMIDIOPENDESC lpDesc, DWORD dwFlags)
{
    MIDIDestination *dest;

    TRACE("wDevID=%d lpDesc=%p dwFlags=%08x\n", wDevID, lpDesc, dwFlags);

    if (lpDesc == NULL) {
	WARN("Invalid Parameter\n");
	return MMSYSERR_INVALPARAM;
    }

    if (wDevID >= MIDIOut_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }

    if (destinations[wDevID].midiDesc.hMidi != 0) {
	WARN("device already open !\n");
	return MMSYSERR_ALLOCATED;
    }

    if ((dwFlags & ~CALLBACK_TYPEMASK) != 0) {
	WARN("bad dwFlags\n");
	return MMSYSERR_INVALFLAG;
    }
    dest = &destinations[wDevID];

    if (dest->caps.wTechnology == MOD_SYNTH)
    {
        if (!SynthUnit_CreateDefaultSynthUnit(&dest->graph, &dest->synth))
        {
            ERR("SynthUnit_CreateDefaultSynthUnit dest=%p failed\n", dest);
            return MMSYSERR_ERROR;
        }

        if (!SynthUnit_Initialize(dest->synth, dest->graph))
        {
            ERR("SynthUnit_Initialise dest=%p failed\n", dest);
            return MMSYSERR_ERROR;
        }
    }
    else
    {
        FIXME("MOD_MIDIPORT\n");
    }
    dest->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
    dest->midiDesc = *lpDesc;

    return MIDI_NotifyClient(wDevID, MOM_OPEN, 0L, 0L);
}

static DWORD MIDIOut_Close(WORD wDevID)
{
    DWORD ret = MMSYSERR_NOERROR;

    TRACE("wDevID=%d\n", wDevID);

    if (wDevID >= MIDIOut_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }

    if (destinations[wDevID].caps.wTechnology == MOD_SYNTH)
        SynthUnit_Close(destinations[wDevID].graph);
    else
        FIXME("MOD_MIDIPORT\n");

    destinations[wDevID].graph = 0;
    destinations[wDevID].synth = 0;

    if (MIDI_NotifyClient(wDevID, MOM_CLOSE, 0L, 0L) != MMSYSERR_NOERROR) {
	WARN("can't notify client !\n");
	ret = MMSYSERR_INVALPARAM;
    }
    destinations[wDevID].midiDesc.hMidi = 0;

    return ret;
}

static DWORD MIDIOut_Data(WORD wDevID, DWORD dwParam)
{
    WORD evt = LOBYTE(LOWORD(dwParam));
    WORD d1  = HIBYTE(LOWORD(dwParam));
    WORD d2  = LOBYTE(HIWORD(dwParam));
    UInt8 chn = (evt & 0x0F);
    OSStatus err = noErr;

    TRACE("wDevID=%d dwParam=%08X\n", wDevID, dwParam);

    if (wDevID >= MIDIOut_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }

    TRACE("evt=%08x d1=%04x d2=%04x (evt & 0xF0)=%04x chn=%d\n", evt, d1, d2, (evt & 0xF0), chn);

    if (destinations[wDevID].caps.wTechnology == MOD_SYNTH)
    {
        err = MusicDeviceMIDIEvent(destinations[wDevID].synth, (evt & 0xF0) | chn, d1, d2, 0);
        if (err != noErr)
        {
            ERR("MusicDeviceMIDIEvent(%p, %04x, %04x, %04x, %d) return %c%c%c%c\n", destinations[wDevID].synth, (evt & 0xF0) | chn, d1, d2, 0, (char) (err >> 24), (char) (err >> 16), (char) (err >> 8), (char) err);
            return MMSYSERR_ERROR;
        }
    }
    else FIXME("MOD_MIDIPORT\n");

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			MIDIOut_Prepare				[internal]
 */
static DWORD MIDIOut_Prepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("wDevID=%d lpMidiHdr=%p dwSize=%d\n", wDevID, lpMidiHdr, dwSize);

    if (wDevID >= MIDIOut_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }

    /* MS doc says that dwFlags must be set to zero, but (kinda funny) MS mciseq drivers
     * asks to prepare MIDIHDR which dwFlags != 0.
     * So at least check for the inqueue flag
     */
    if (dwSize < sizeof(MIDIHDR) || lpMidiHdr == 0 ||
	lpMidiHdr->lpData == 0 || (lpMidiHdr->dwFlags & MHDR_INQUEUE) != 0 ||
	lpMidiHdr->dwBufferLength >= 0x10000ul) {
	WARN("%p %p %08x %lu/%d\n", lpMidiHdr, lpMidiHdr->lpData,
	           lpMidiHdr->dwFlags, sizeof(MIDIHDR), dwSize);
	return MMSYSERR_INVALPARAM;
    }

    lpMidiHdr->lpNext = 0;
    lpMidiHdr->dwFlags |= MHDR_PREPARED;
    lpMidiHdr->dwFlags &= ~MHDR_DONE;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				MIDIOut_Unprepare			[internal]
 */
static DWORD MIDIOut_Unprepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("wDevID=%d lpMidiHdr=%p dwSize=%d\n", wDevID, lpMidiHdr, dwSize);

    if (wDevID >= MIDIOut_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }
    if (dwSize < sizeof(MIDIHDR) || lpMidiHdr == 0)
	return MMSYSERR_INVALPARAM;
    if (lpMidiHdr->dwFlags & MHDR_INQUEUE)
	return MIDIERR_STILLPLAYING;

    lpMidiHdr->dwFlags &= ~MHDR_PREPARED;

    return MMSYSERR_NOERROR;
}

static DWORD MIDIOut_GetDevCaps(WORD wDevID, LPMIDIOUTCAPSW lpCaps, DWORD dwSize)
{
    TRACE("wDevID=%d lpCaps=%p dwSize=%d\n", wDevID, lpCaps, dwSize);

    if (lpCaps == NULL) {
	WARN("Invalid Parameter\n");
	return MMSYSERR_INVALPARAM;
    }

    if (wDevID >= MIDIOut_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }
    memcpy(lpCaps, &destinations[wDevID].caps, min(dwSize, sizeof(*lpCaps)));
    return MMSYSERR_NOERROR;
}

static DWORD MIDIOut_GetNumDevs(void)
{
    TRACE("\n");
    return MIDIOut_NumDevs;
}

/**************************************************************************
* 				modMessage
*/
DWORD WINAPI CoreAudio_modMessage(UINT wDevID, UINT wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2)
{
    TRACE("%d %08x %08x %08x %08x\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);

    switch (wMsg) {
        case DRVM_INIT:
        case DRVM_EXIT:
        case DRVM_ENABLE:
        case DRVM_DISABLE:
            return 0;
        case MODM_OPEN:
            return MIDIOut_Open(wDevID, (LPMIDIOPENDESC)dwParam1, dwParam2);
        case MODM_CLOSE:
            return MIDIOut_Close(wDevID);
        case MODM_DATA:
            return MIDIOut_Data(wDevID, dwParam1);
        case MODM_LONGDATA:
            TRACE("Unsupported message (08%x)\n", wMsg);
            return MMSYSERR_NOTSUPPORTED;
        case MODM_PREPARE:
            return MIDIOut_Prepare(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
        case MODM_UNPREPARE:
            return MIDIOut_Unprepare(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
        case MODM_GETDEVCAPS:
            return MIDIOut_GetDevCaps(wDevID, (LPMIDIOUTCAPSW) dwParam1, dwParam2);
        case MODM_GETNUMDEVS:
            return MIDIOut_GetNumDevs();
        case MODM_GETVOLUME:
        case MODM_SETVOLUME:
        case MODM_RESET:
        default:
            TRACE("Unsupported message (08%x)\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

#else

DWORD WINAPI CoreAudio_modMessage(UINT wDevID, UINT wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2)
{
    TRACE("%08x, %08x, %08x, %08x, %08x\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

#endif
