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
        case MODM_CLOSE:
        case MODM_DATA:
        case MODM_LONGDATA:
        case MODM_PREPARE:
        case MODM_UNPREPARE:
        case MODM_GETDEVCAPS:
        case MODM_GETNUMDEVS:
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
