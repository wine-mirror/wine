/*
 * Sample MIDI Wine Driver for Mac OS X (based on OSS midi driver)
 *
 * Copyright 1994 	Martin Ayotte
 * Copyright 1998 	Luiz Otavio L. Zorzella (init procedures)
 * Copyright 1998/1999	Eric POUECH :
 * 		98/7 	changes for making this MIDI driver work on OSS
 * 			current support is limited to MIDI ports of OSS systems
 * 		98/9	rewriting MCI code for MIDI
 * 		98/11 	split in midi.c and mcimidi.c
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
#include "coreaudio.h"

WINE_DEFAULT_DEBUG_CHANNEL(midi);

#include <CoreAudio/CoreAudio.h>

#define WINE_DEFINITIONS
#include "coremidi.h"

static MIDIClientRef wineMIDIClient = NULL;

static DWORD MIDIOut_NumDevs = 0;
static DWORD MIDIIn_NumDevs = 0;

typedef struct tagMIDIDestination {
    /* graph and synth are only used for MIDI Synth */
    AUGraph graph;
    AudioUnit synth;

    MIDIEndpointRef dest;

    MIDIOUTCAPSW caps;
    MIDIOPENDESC midiDesc;
    WORD wFlags;
} MIDIDestination;

typedef struct tagMIDISource {
    MIDIEndpointRef source;

    WORD wDevID;
    int state; /* 0 is no recording started, 1 in recording, bit 2 set if in sys exclusive recording */
    MIDIINCAPSW caps;
    MIDIOPENDESC midiDesc;
    LPMIDIHDR lpQueueHdr;
    WORD wFlags;
    DWORD startTime;
} MIDISource;

typedef struct {
    UInt16 devID;
    UInt16 length;
    Byte data[];
} MIDIMessage;

static CRITICAL_SECTION midiInLock; /* Critical section for MIDI In */
static CFStringRef MIDIInThreadPortName = NULL;

static DWORD WINAPI MIDIIn_MessageThread(LPVOID p);

static MIDIPortRef MIDIInPort = NULL;
static MIDIPortRef MIDIOutPort = NULL;

#define MAX_MIDI_SYNTHS 1

MIDIDestination *destinations;
MIDISource *sources;

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
        return DRV_FAILURE;
    }
    CFRelease(name);

    MIDIOut_NumDevs = MAX_MIDI_SYNTHS;
    MIDIOut_NumDevs += numDest;

    MIDIIn_NumDevs = MIDIGetNumberOfSources();

    TRACE("MIDIOut_NumDevs %d MIDIIn_NumDevs %d\n", MIDIOut_NumDevs, MIDIIn_NumDevs);

    destinations = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MIDIOut_NumDevs * sizeof(MIDIDestination));
    sources = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MIDIIn_NumDevs * sizeof(MIDISource));

    if (MIDIIn_NumDevs > 0)
    {
        InitializeCriticalSection(&midiInLock);
        midiInLock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": midiInLock");
        MIDIInThreadPortName = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("MIDIInThreadPortName.%u"), getpid());
        CloseHandle( CreateThread(NULL, 0, MIDIIn_MessageThread, NULL, 0, NULL));

        name = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("WineInputPort.%u"), getpid());
        MIDIInputPortCreate(wineMIDIClient, name, MIDIIn_ReadProc, NULL, &MIDIInPort);
        CFRelease(name);
    }
    if (numDest > 0)
    {
        name = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("WineOutputPort.%u"), getpid());
        MIDIOutputPortCreate(wineMIDIClient, name, &MIDIOutPort);
        CFRelease(name);
    }

    /* initialize sources */
    for (i = 0; i < MIDIIn_NumDevs; i++)
    {
        sources[i].wDevID = i;
        sources[i].source = MIDIGetSource(i);

        CoreMIDI_GetObjectName(sources[i].source, szPname, sizeof(szPname));
        MultiByteToWideChar(CP_ACP, 0, szPname, -1, sources[i].caps.szPname, ARRAY_SIZE(sources[i].caps.szPname));

        MIDIPortConnectSource(MIDIInPort, sources[i].source, &sources[i].wDevID);

        sources[i].state = 0;
        /* FIXME */
        sources[i].caps.wMid = 0x00FF; 	/* Manufac ID */
        sources[i].caps.wPid = 0x0001; 	/* Product ID */
        sources[i].caps.vDriverVersion = 0x0001;
        sources[i].caps.dwSupport = 0;
    }

    /* initialise MIDI synths */
    for (i = 0; i < MAX_MIDI_SYNTHS; i++)
    {
        snprintf(szPname, sizeof(szPname), "CoreAudio MIDI Synth %d", i);
        MultiByteToWideChar(CP_ACP, 0, szPname, -1, destinations[i].caps.szPname, ARRAY_SIZE(destinations[i].caps.szPname));

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
        destinations[i].dest = MIDIGetDestination(i - MAX_MIDI_SYNTHS);

        CoreMIDI_GetObjectName(destinations[i].dest, szPname, sizeof(szPname));
        MultiByteToWideChar(CP_ACP, 0, szPname, -1, destinations[i].caps.szPname, ARRAY_SIZE(destinations[i].caps.szPname));

        destinations[i].caps.wTechnology = MOD_MIDIPORT;
        destinations[i].caps.wChannelMask = 0xFFFF;

        destinations[i].caps.wMid = 0x00FF; 	/* Manufac ID */
        destinations[i].caps.wPid = 0x0001;
        destinations[i].caps.vDriverVersion = 0x0001;
        destinations[i].caps.dwSupport = 0;
        destinations[i].caps.wVoices = 0;
        destinations[i].caps.wNotes = 0;
    }
    return DRV_SUCCESS;
}

LONG CoreAudio_MIDIRelease(void)
{
    TRACE("\n");
    if (MIDIIn_NumDevs > 0)
    {
        CFMessagePortRef messagePort;
        /* Stop CFRunLoop in MIDIIn_MessageThread */
        messagePort = CFMessagePortCreateRemote(kCFAllocatorDefault, MIDIInThreadPortName);
        CFMessagePortSendRequest(messagePort, 1, NULL, 0.0, 0.0, NULL, NULL);
        CFRelease(messagePort);

        midiInLock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&midiInLock);
    }

    if (wineMIDIClient) MIDIClientDispose(wineMIDIClient); /* MIDIClientDispose will close all ports */

    HeapFree(GetProcessHeap(), 0, sources);
    HeapFree(GetProcessHeap(), 0, destinations);
    return DRV_SUCCESS;
}


/**************************************************************************
 * 			MIDI_NotifyClient			[internal]
 */
static void MIDI_NotifyClient(UINT wDevID, WORD wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    DWORD_PTR 		dwCallBack;
    UINT 		uFlags;
    HANDLE		hDev;
    DWORD_PTR 		dwInstance;

    TRACE("wDevID=%d wMsg=%d dwParm1=%04lX dwParam2=%04lX\n", wDevID, wMsg, dwParam1, dwParam2);

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
        dwCallBack = sources[wDevID].midiDesc.dwCallback;
	uFlags = sources[wDevID].wFlags;
	hDev = sources[wDevID].midiDesc.hMidi;
	dwInstance = sources[wDevID].midiDesc.dwInstance;
        break;
    default:
	ERR("Unsupported MSW-MIDI message %u\n", wMsg);
	return;
    }

    DriverCallback(dwCallBack, uFlags, hDev, wMsg, dwInstance, dwParam1, dwParam2);
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
    dest->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
    dest->midiDesc = *lpDesc;

    MIDI_NotifyClient(wDevID, MOM_OPEN, 0L, 0L);
    return MMSYSERR_NOERROR;
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

    destinations[wDevID].graph = 0;
    destinations[wDevID].synth = 0;

    MIDI_NotifyClient(wDevID, MOM_CLOSE, 0L, 0L);
    destinations[wDevID].midiDesc.hMidi = 0;

    return ret;
}

static DWORD MIDIOut_Data(WORD wDevID, DWORD dwParam)
{
    WORD evt = LOBYTE(LOWORD(dwParam));
    UInt8 chn = (evt & 0x0F);

    TRACE("wDevID=%d dwParam=%08X\n", wDevID, dwParam);

    if (wDevID >= MIDIOut_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }

    if (destinations[wDevID].caps.wTechnology == MOD_SYNTH)
    {
        WORD d1  = HIBYTE(LOWORD(dwParam));
        WORD d2  = LOBYTE(HIWORD(dwParam));
        OSStatus err = noErr;

        err = MusicDeviceMIDIEvent(destinations[wDevID].synth, (evt & 0xF0) | chn, d1, d2, 0);
        if (err != noErr)
        {
            ERR("MusicDeviceMIDIEvent(%p, %04x, %04x, %04x, %d) return %s\n", destinations[wDevID].synth, (evt & 0xF0) | chn, d1, d2, 0, wine_dbgstr_fourcc(err));
            return MMSYSERR_ERROR;
        }
    }
    else
    {
        UInt8 buffer[3];
        buffer[0] = (evt & 0xF0) | chn;
        buffer[1] = HIBYTE(LOWORD(dwParam));
        buffer[2] = LOBYTE(HIWORD(dwParam));

        MIDIOut_Send(MIDIOutPort, destinations[wDevID].dest, buffer, 3);
    }

    return MMSYSERR_NOERROR;
}

static DWORD MIDIOut_LongData(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    LPBYTE lpData;
    OSStatus err = noErr;

    TRACE("wDevID=%d lpMidiHdr=%p dwSize=%d\n", wDevID, lpMidiHdr, dwSize);

    /* Note: MS doc does not say much about the dwBytesRecorded member of the MIDIHDR structure
     * but it seems to be used only for midi input.
     * Taking a look at the WAVEHDR structure (which is quite similar) confirms this assumption.
     */

    if (wDevID >= MIDIOut_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }

    if (lpMidiHdr == NULL) {
	WARN("Invalid Parameter\n");
	return MMSYSERR_INVALPARAM;
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


    if (lpData[0] != 0xF0) {
        /* System Exclusive */
        ERR("Add missing 0xF0 marker at the beginning of system exclusive byte stream\n");
    }
    if (lpData[lpMidiHdr->dwBufferLength - 1] != 0xF7) {
        /* Send end of System Exclusive */
        ERR("Add missing 0xF7 marker at the end of system exclusive byte stream\n");
    }
    if (destinations[wDevID].caps.wTechnology == MOD_SYNTH) /* FIXME */
    {
        err = MusicDeviceSysEx(destinations[wDevID].synth, (const UInt8 *) lpData, lpMidiHdr->dwBufferLength);
        if (err != noErr)
        {
            ERR("MusicDeviceSysEx(%p, %p, %d) return %s\n", destinations[wDevID].synth, lpData, lpMidiHdr->dwBufferLength, wine_dbgstr_fourcc(err));
            return MMSYSERR_ERROR;
        }
    }
    else if (destinations[wDevID].caps.wTechnology == MOD_MIDIPORT) {
        MIDIOut_Send(MIDIOutPort, destinations[wDevID].dest, lpData, lpMidiHdr->dwBufferLength);
    }

    lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
    lpMidiHdr->dwFlags |= MHDR_DONE;
    MIDI_NotifyClient(wDevID, MOM_DONE, (DWORD_PTR)lpMidiHdr, 0L);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			MIDIOut_Prepare				[internal]
 */
static DWORD MIDIOut_Prepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("wDevID=%d lpMidiHdr=%p dwSize=%d\n", wDevID, lpMidiHdr, dwSize);

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
 * 				MIDIOut_Unprepare			[internal]
 */
static DWORD MIDIOut_Unprepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("wDevID=%d lpMidiHdr=%p dwSize=%d\n", wDevID, lpMidiHdr, dwSize);

    if (dwSize < offsetof(MIDIHDR,dwOffset) || lpMidiHdr == 0 || lpMidiHdr->lpData == 0)
	return MMSYSERR_INVALPARAM;
    if (!(lpMidiHdr->dwFlags & MHDR_PREPARED))
	return MMSYSERR_NOERROR;
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

static DWORD MIDIOut_GetVolume(WORD wDevID, DWORD *lpdwVolume)
{
    TRACE("%d\n", wDevID);

    if (wDevID >= MIDIOut_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }
    if (lpdwVolume == NULL) {
	WARN("Invalid Parameter\n");
	return MMSYSERR_INVALPARAM;
    }

    if (destinations[wDevID].caps.wTechnology == MOD_SYNTH)
    {
        float left;
        float right;
        AudioUnit_GetVolume(destinations[wDevID].synth, &left, &right);

        *lpdwVolume = (WORD) (left * 0xFFFF) + ((WORD) (right * 0xFFFF) << 16);

        return MMSYSERR_NOERROR;
    }

    return MMSYSERR_NOTSUPPORTED;
}

static DWORD MIDIOut_SetVolume(WORD wDevID, DWORD dwVolume)
{
    TRACE("%d\n", wDevID);

    if (wDevID >= MIDIOut_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }
    if (destinations[wDevID].caps.wTechnology == MOD_SYNTH)
    {
        float left;
        float right;

        left  = LOWORD(dwVolume) / 65535.0f;
        right = HIWORD(dwVolume) / 65535.0f;
        AudioUnit_SetVolume(destinations[wDevID].synth, left, right);

        return MMSYSERR_NOERROR;
    }

    return MMSYSERR_NOTSUPPORTED;
}

static DWORD MIDIOut_Reset(WORD wDevID)
{
    unsigned chn;

    TRACE("%d\n", wDevID);

    if (wDevID >= MIDIOut_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }
    if (destinations[wDevID].caps.wTechnology == MOD_SYNTH)
    {
        for (chn = 0; chn < 16; chn++) {
            /* turn off every note */
            MusicDeviceMIDIEvent(destinations[wDevID].synth, 0xB0 | chn, 0x7B, 0, 0);
            /* remove sustain on channel */
            MusicDeviceMIDIEvent(destinations[wDevID].synth, 0xB0 | chn, 0x40, 0, 0);
        }
    }
    else FIXME("MOD_MIDIPORT\n");

    /* FIXME: the LongData buffers must also be returned to the app */
    return MMSYSERR_NOERROR;
}

static DWORD MIDIIn_Open(WORD wDevID, LPMIDIOPENDESC lpDesc, DWORD dwFlags)
{
    TRACE("wDevID=%d lpDesc=%p dwFlags=%08x\n", wDevID, lpDesc, dwFlags);

    if (lpDesc == NULL) {
	WARN("Invalid Parameter\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MIDIIn_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }
    if (sources[wDevID].midiDesc.hMidi != 0) {
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

    sources[wDevID].wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
    sources[wDevID].lpQueueHdr = NULL;
    sources[wDevID].midiDesc = *lpDesc;
    sources[wDevID].startTime = 0;
    sources[wDevID].state = 0;

    MIDI_NotifyClient(wDevID, MIM_OPEN, 0L, 0L);
    return MMSYSERR_NOERROR;
}

static DWORD MIDIIn_Close(WORD wDevID)
{
    DWORD ret = MMSYSERR_NOERROR;

    TRACE("wDevID=%d\n", wDevID);

    if (wDevID >= MIDIIn_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }

    if (sources[wDevID].midiDesc.hMidi == 0) {
	WARN("device not opened !\n");
	return MMSYSERR_ERROR;
    }
    if (sources[wDevID].lpQueueHdr != 0) {
	return MIDIERR_STILLPLAYING;
    }

    MIDI_NotifyClient(wDevID, MIM_CLOSE, 0L, 0L);
    sources[wDevID].midiDesc.hMidi = 0;
    return ret;
}

static DWORD MIDIIn_AddBuffer(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("wDevID=%d lpMidiHdr=%p dwSize=%d\n", wDevID, lpMidiHdr, dwSize);

    if (wDevID >= MIDIIn_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }
    if (lpMidiHdr == NULL) {
	WARN("Invalid Parameter\n");
	return MMSYSERR_INVALPARAM;
    }
    if (dwSize < offsetof(MIDIHDR,dwOffset)) {
	WARN("Invalid Parameter\n");
	return MMSYSERR_INVALPARAM;
    }
    if (lpMidiHdr->dwBufferLength == 0) {
	WARN("Invalid Parameter\n");
	return MMSYSERR_INVALPARAM;
    }
    if (lpMidiHdr->dwFlags & MHDR_INQUEUE) {
	WARN("Still playing\n");
	return MIDIERR_STILLPLAYING;
    }
    if (!(lpMidiHdr->dwFlags & MHDR_PREPARED)) {
	WARN("Unprepared\n");
	return MIDIERR_UNPREPARED;
    }

    EnterCriticalSection(&midiInLock);
    lpMidiHdr->dwFlags &= ~WHDR_DONE;
    lpMidiHdr->dwFlags |= MHDR_INQUEUE;
    lpMidiHdr->dwBytesRecorded = 0;
    lpMidiHdr->lpNext = 0;
    if (sources[wDevID].lpQueueHdr == 0) {
	sources[wDevID].lpQueueHdr = lpMidiHdr;
    } else {
	LPMIDIHDR ptr;
	for (ptr = sources[wDevID].lpQueueHdr;
	     ptr->lpNext != 0;
	     ptr = ptr->lpNext);
	ptr->lpNext = lpMidiHdr;
    }
    LeaveCriticalSection(&midiInLock);

    return MMSYSERR_NOERROR;
}

static DWORD MIDIIn_Prepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("wDevID=%d lpMidiHdr=%p dwSize=%d\n", wDevID, lpMidiHdr, dwSize);

    if (dwSize < offsetof(MIDIHDR,dwOffset) || lpMidiHdr == 0 || lpMidiHdr->lpData == 0)
	return MMSYSERR_INVALPARAM;
    if (lpMidiHdr->dwFlags & MHDR_PREPARED)
	return MMSYSERR_NOERROR;

    lpMidiHdr->lpNext = 0;
    lpMidiHdr->dwFlags |= MHDR_PREPARED;
    lpMidiHdr->dwFlags &= ~(MHDR_DONE|MHDR_INQUEUE); /* flags cleared since w2k */
    return MMSYSERR_NOERROR;
}

static DWORD MIDIIn_Unprepare(WORD wDevID, LPMIDIHDR lpMidiHdr, DWORD dwSize)
{
    TRACE("wDevID=%d lpMidiHdr=%p dwSize=%d\n", wDevID, lpMidiHdr, dwSize);

    if (dwSize < offsetof(MIDIHDR,dwOffset) || lpMidiHdr == 0 || lpMidiHdr->lpData == 0)
	return MMSYSERR_INVALPARAM;
    if (!(lpMidiHdr->dwFlags & MHDR_PREPARED))
	return MMSYSERR_NOERROR;
    if (lpMidiHdr->dwFlags & MHDR_INQUEUE)
	return MIDIERR_STILLPLAYING;

    lpMidiHdr->dwFlags &= ~MHDR_PREPARED;
    return MMSYSERR_NOERROR;
}

static DWORD MIDIIn_GetDevCaps(WORD wDevID, LPMIDIINCAPSW lpCaps, DWORD dwSize)
{
    TRACE("wDevID=%d lpCaps=%p dwSize=%d\n", wDevID, lpCaps, dwSize);

    if (lpCaps == NULL) {
	WARN("Invalid Parameter\n");
	return MMSYSERR_INVALPARAM;
    }

    if (wDevID >= MIDIIn_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }
    memcpy(lpCaps, &sources[wDevID].caps, min(dwSize, sizeof(*lpCaps)));
    return MMSYSERR_NOERROR;
}

static DWORD MIDIIn_GetNumDevs(void)
{
    TRACE("\n");
    return MIDIIn_NumDevs;
}

static DWORD MIDIIn_Start(WORD wDevID)
{
    TRACE("%d\n", wDevID);

    if (wDevID >= MIDIIn_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }
    sources[wDevID].state = 1;
    sources[wDevID].startTime = GetTickCount();
    return MMSYSERR_NOERROR;
}

static DWORD MIDIIn_Stop(WORD wDevID)
{
    TRACE("%d\n", wDevID);
    if (wDevID >= MIDIIn_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }
    sources[wDevID].state = 0;
    return MMSYSERR_NOERROR;
}

static DWORD MIDIIn_Reset(WORD wDevID)
{
    DWORD dwTime = GetTickCount();

    TRACE("%d\n", wDevID);
    if (wDevID >= MIDIIn_NumDevs) {
        WARN("bad device ID : %d\n", wDevID);
	return MMSYSERR_BADDEVICEID;
    }

    EnterCriticalSection(&midiInLock);
    while (sources[wDevID].lpQueueHdr) {
	LPMIDIHDR lpMidiHdr = sources[wDevID].lpQueueHdr;
	sources[wDevID].lpQueueHdr = lpMidiHdr->lpNext;
	lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
	lpMidiHdr->dwFlags |= MHDR_DONE;
	/* FIXME: when called from 16 bit, lpQueueHdr needs to be a segmented ptr */
	MIDI_NotifyClient(wDevID, MIM_LONGDATA, (DWORD_PTR)lpMidiHdr, dwTime);
    }
    LeaveCriticalSection(&midiInLock);

    return MMSYSERR_NOERROR;
}

/*
 * MIDI In Mach message handling
 */

/*
 *  Call from CoreMIDI IO threaded callback,
 *  we can't call Wine debug channels, critical section or anything using NtCurrentTeb here.
 */
void MIDIIn_SendMessage(UInt16 devID, const void *buffer, UInt16 length)
{
    MIDIMessage msg;
    CFMutableDataRef data;

    CFMessagePortRef messagePort;
    messagePort = CFMessagePortCreateRemote(kCFAllocatorDefault, MIDIInThreadPortName);

    msg.devID = devID;
    msg.length = length;

    data = CFDataCreateMutable(kCFAllocatorDefault, sizeof(msg) + length);
    if (data)
    {
        CFDataAppendBytes(data, (UInt8 *) &msg, sizeof(msg));
        CFDataAppendBytes(data, buffer, length);
        CFMessagePortSendRequest(messagePort, 0, data, 0.0, 0.0, NULL, NULL);
        CFRelease(data);
    }
    CFRelease(messagePort);
}

static CFDataRef MIDIIn_MessageHandler(CFMessagePortRef local, SInt32 msgid, CFDataRef data, void *info)
{
    MIDIMessage *msg = NULL;
    int i = 0;
    MIDISource *src = NULL;
    DWORD sendData = 0;
    int pos = 0;
    DWORD currentTime;
    BOOL sysexStart;

    switch (msgid)
    {
        case 0:
            msg = (MIDIMessage *) CFDataGetBytePtr(data);
            TRACE("devID=%d\n", msg->devID);
             for (i = 0; i < msg->length; ++i) {
                TRACE("%02X ", msg->data[i]);
            }
            TRACE("\n");
            src = &sources[msg->devID];
            if (src->state < 1)
            {
                TRACE("input not started, thrown away\n");
                return NULL;
            }

            sysexStart = (msg->data[0] == 0xF0);

            if (sysexStart || src->state & 2) {
                int pos = 0;
                int len = msg->length;

                if (sysexStart) {
                    TRACE("Receiving sysex message\n");
                    src->state |= 2;
                }

                EnterCriticalSection(&midiInLock);
                currentTime = GetTickCount() - src->startTime;

                while (len) {
                    LPMIDIHDR lpMidiHdr = src->lpQueueHdr;

                    if (lpMidiHdr != NULL) {
                        int copylen = min(len, lpMidiHdr->dwBufferLength - lpMidiHdr->dwBytesRecorded);
                        memcpy(lpMidiHdr->lpData + lpMidiHdr->dwBytesRecorded, msg->data + pos, copylen);
                        lpMidiHdr->dwBytesRecorded += copylen;
                        len -= copylen;
                        pos += copylen;

                        TRACE("Copied %d bytes of sysex message\n", copylen);

                        if ((lpMidiHdr->dwBytesRecorded == lpMidiHdr->dwBufferLength) ||
                            (*(BYTE*)(lpMidiHdr->lpData + lpMidiHdr->dwBytesRecorded - 1) == 0xF7)) {
                            TRACE("Sysex message complete (or buffer limit reached), dispatching %d bytes\n", lpMidiHdr->dwBytesRecorded);
                            src->lpQueueHdr = lpMidiHdr->lpNext;
                            lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
                            lpMidiHdr->dwFlags |= MHDR_DONE;
                            MIDI_NotifyClient(msg->devID, MIM_LONGDATA, (DWORD_PTR)lpMidiHdr, currentTime);
                            src->state &= ~2;
                        }
                    }
                    else {
                        FIXME("Sysex data received but no buffer to store it!\n");
                        break;
                    }
                }

                LeaveCriticalSection(&midiInLock);
                return NULL;
            }

            EnterCriticalSection(&midiInLock);
            currentTime = GetTickCount() - src->startTime;

            while (pos < msg->length)
            {
                sendData = 0;
                switch (msg->data[pos] & 0xF0)
                {
                    case 0xF0:
                        sendData = (msg->data[pos] <<  0);
                        pos++;
                        break;

                    case 0xC0:
                    case 0xD0:
                        sendData = (msg->data[pos + 1] <<  8) | (msg->data[pos] <<  0);
                        pos += 2;
                        break;
                    default:
                        sendData = (msg->data[pos + 2] << 16) |
                                    (msg->data[pos + 1] <<  8) |
                                    (msg->data[pos] <<  0);
                        pos += 3;
                        break;
                }
                MIDI_NotifyClient(msg->devID, MIM_DATA, sendData, currentTime);
            }
            LeaveCriticalSection(&midiInLock);
            break;
        default:
            CFRunLoopStop(CFRunLoopGetCurrent());
            break;
    }
    return NULL;
}

static DWORD WINAPI MIDIIn_MessageThread(LPVOID p)
{
    CFMessagePortRef local;
    CFRunLoopSourceRef source;
    Boolean info;

    local = CFMessagePortCreateLocal(kCFAllocatorDefault, MIDIInThreadPortName, &MIDIIn_MessageHandler, NULL, &info);

    source = CFMessagePortCreateRunLoopSource(kCFAllocatorDefault, local, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopDefaultMode);

    CFRunLoopRun();

    CFRunLoopSourceInvalidate(source);
    CFRelease(source);
    CFRelease(local);
    CFRelease(MIDIInThreadPortName);
    MIDIInThreadPortName = NULL;

    return 0;
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
            return MIDIOut_LongData(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
        case MODM_PREPARE:
            return MIDIOut_Prepare(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
        case MODM_UNPREPARE:
            return MIDIOut_Unprepare(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
        case MODM_GETDEVCAPS:
            return MIDIOut_GetDevCaps(wDevID, (LPMIDIOUTCAPSW) dwParam1, dwParam2);
        case MODM_GETNUMDEVS:
            return MIDIOut_GetNumDevs();
        case MODM_GETVOLUME:
            return MIDIOut_GetVolume(wDevID, (DWORD*)dwParam1);
        case MODM_SETVOLUME:
            return MIDIOut_SetVolume(wDevID, dwParam1);
        case MODM_RESET:
            return MIDIOut_Reset(wDevID);
        default:
            TRACE("Unsupported message (08%x)\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
* 			midMessage
*/
DWORD WINAPI CoreAudio_midMessage(UINT wDevID, UINT wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2)
{
    TRACE("%d %08x %08x %08x %08x\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    switch (wMsg) {
        case DRVM_INIT:
        case DRVM_EXIT:
        case DRVM_ENABLE:
        case DRVM_DISABLE:
            return 0;
        case MIDM_OPEN:
            return MIDIIn_Open(wDevID, (LPMIDIOPENDESC)dwParam1, dwParam2);
        case MIDM_CLOSE:
            return MIDIIn_Close(wDevID);
        case MIDM_ADDBUFFER:
            return MIDIIn_AddBuffer(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
        case MIDM_PREPARE:
            return MIDIIn_Prepare(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
        case MIDM_UNPREPARE:
            return MIDIIn_Unprepare(wDevID, (LPMIDIHDR)dwParam1, dwParam2);
        case MIDM_GETDEVCAPS:
            return MIDIIn_GetDevCaps(wDevID, (LPMIDIINCAPSW) dwParam1, dwParam2);
        case MIDM_GETNUMDEVS:
            return MIDIIn_GetNumDevs();
        case MIDM_START:
            return MIDIIn_Start(wDevID);
        case MIDM_STOP:
            return MIDIIn_Stop(wDevID);
        case MIDM_RESET:
            return MIDIIn_Reset(wDevID);
        default:
            TRACE("Unsupported message\n");
    }
    return MMSYSERR_NOTSUPPORTED;
}
