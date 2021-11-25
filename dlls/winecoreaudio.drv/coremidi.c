/*
 * MIDI driver for macOS (unixlib)
 *
 * Copyright 1994       Martin Ayotte
 * Copyright 1998       Luiz Otavio L. Zorzella
 * Copyright 1998, 1999 Eric POUECH
 * Copyright 2005, 2006 Emmanuel Maillard
 * Copyright 2021       Huw Davies
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
#if 0
#pragma makedep unix
#endif

#include "config.h"

#define ULONG __carbon_ULONG
#define E_INVALIDARG __carbon_E_INVALIDARG
#define E_OUTOFMEMORY __carbon_E_OUTOFMEMORY
#define E_HANDLE __carbon_E_HANDLE
#define E_ACCESSDENIED __carbon_E_ACCESSDENIED
#define E_UNEXPECTED __carbon_E_UNEXPECTED
#define E_FAIL __carbon_E_FAIL
#define E_ABORT __carbon_E_ABORT
#define E_POINTER __carbon_E_POINTER
#define E_NOINTERFACE __carbon_E_NOINTERFACE
#define E_NOTIMPL __carbon_E_NOTIMPL
#define S_FALSE __carbon_S_FALSE
#define S_OK __carbon_S_OK
#define HRESULT_FACILITY __carbon_HRESULT_FACILITY
#define IS_ERROR __carbon_IS_ERROR
#define FAILED __carbon_FAILED
#define SUCCEEDED __carbon_SUCCEEDED
#define MAKE_HRESULT __carbon_MAKE_HRESULT
#define HRESULT __carbon_HRESULT
#define STDMETHODCALLTYPE __carbon_STDMETHODCALLT
#include <mach/mach_time.h>
#include <CoreMIDI/CoreMIDI.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#undef ULONG
#undef E_INVALIDARG
#undef E_OUTOFMEMORY
#undef E_HANDLE
#undef E_ACCESSDENIED
#undef E_UNEXPECTED
#undef E_FAIL
#undef E_ABORT
#undef E_POINTER
#undef E_NOINTERFACE
#undef E_NOTIMPL
#undef S_FALSE
#undef S_OK
#undef HRESULT_FACILITY
#undef IS_ERROR
#undef FAILED
#undef SUCCEEDED
#undef MAKE_HRESULT
#undef HRESULT
#undef STDMETHODCALLTYPE

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "mmsystem.h"
#include "mmddk.h"
#include "mmdeviceapi.h"
#include "audioclient.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/unixlib.h"

#include "coreaudio.h"
#include "coremidi.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(midi);

static MIDIClientRef midi_client;
static MIDIPortRef midi_out_port, midi_in_port;
static UINT num_dests, num_srcs;
static struct midi_dest *dests;
static struct midi_src *srcs;
static CFStringRef midi_in_thread_port_name;

/*
 *  CoreMIDI IO threaded callback,
 *  we can't call Wine debug channels, critical section or anything using NtCurrentTeb here.
 */
static void midi_in_read_proc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
    CFMessagePortRef msg_port = CFMessagePortCreateRemote(kCFAllocatorDefault, midi_in_thread_port_name);
    MIDIPacket *packet = (MIDIPacket *)pktlist->packet;
    CFMutableDataRef data;
    MIDIMessage msg;
    unsigned int i;

    for (i = 0; i < pktlist->numPackets; ++i)
    {
        msg.devID = *(UInt16 *)connRefCon;
        msg.length = packet->length;
        data = CFDataCreateMutable(kCFAllocatorDefault, sizeof(msg) + packet->length);
        if (data)
        {
            CFDataAppendBytes(data, (UInt8 *)&msg, sizeof(msg));
            CFDataAppendBytes(data, packet->data, packet->length);
            CFMessagePortSendRequest(msg_port, 0, data, 0.0, 0.0, NULL, NULL);
            CFRelease(data);
        }
        packet = MIDIPacketNext(packet);
    }
    CFRelease(msg_port);
}

NTSTATUS midi_init(void *args)
{
    CFStringRef name = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("wineMIDIClient.%d"), getpid());
    struct midi_init_params *params = args;
    OSStatus sc;
    UINT i;

    sc = MIDIClientCreate(name, NULL /* FIXME use notify proc */, NULL, &midi_client);
    CFRelease(name);
    if (sc)
    {
        ERR("can't create MIDI Client\n");
        *params->err = DRV_FAILURE;
        return STATUS_SUCCESS;
    }

    num_dests = MAX_MIDI_SYNTHS + MIDIGetNumberOfDestinations();
    num_srcs = MIDIGetNumberOfSources();

    TRACE("num_dests %d num_srcs %d\n", num_dests, num_srcs);

    dests = calloc(num_dests, sizeof(*dests));
    srcs = calloc(num_srcs, sizeof(*srcs));

    if (num_srcs > 0)
    {
        midi_in_thread_port_name = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("MIDIInThreadPortName.%u"), getpid());
        name = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("WineInputPort.%u"), getpid());
        MIDIInputPortCreate(midi_client, name, midi_in_read_proc, NULL, &midi_in_port);
        CFRelease(name);
    }

    if (num_dests > MAX_MIDI_SYNTHS)
    {
        name = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("WineOutputPort.%u"), getpid());
        MIDIOutputPortCreate(midi_client, name, &midi_out_port);
        CFRelease(name);
    }

    /* initialize sources */
    for (i = 0; i < num_srcs; i++)
    {
        srcs[i].wDevID = i;
        srcs[i].source = MIDIGetSource(i);

        sc = MIDIObjectGetStringProperty(srcs[i].source, kMIDIPropertyName, &name);
        if (!sc)
        {
            int len = min(CFStringGetLength(name), ARRAY_SIZE(srcs[i].caps.szPname) - 1);
            CFStringGetCharacters(name, CFRangeMake(0, len), srcs[i].caps.szPname);
            srcs[i].caps.szPname[len] = '\0';
        }
        MIDIPortConnectSource(midi_in_port, srcs[i].source, &srcs[i].wDevID);

        srcs[i].state = 0;
        /* FIXME */
        srcs[i].caps.wMid = 0x00FF; 	/* Manufac ID */
        srcs[i].caps.wPid = 0x0001; 	/* Product ID */
        srcs[i].caps.vDriverVersion = 0x0001;
        srcs[i].caps.dwSupport = 0;
    }

    /* initialise MIDI synths */
    for (i = 0; i < MAX_MIDI_SYNTHS; i++)
    {
        static const WCHAR synth_name[] = {'C','o','r','e','A','u','d','i','o',' ','M','I','D','I',' ','S','y','n','t','h',' '};

        C_ASSERT(MAX_MIDI_SYNTHS < 10);
        memcpy(dests[i].caps.szPname, synth_name, sizeof(synth_name));
        dests[i].caps.szPname[ARRAY_SIZE(synth_name)] = '1' + i;
        dests[i].caps.szPname[ARRAY_SIZE(synth_name) + 1] = '\0';

        dests[i].caps.wTechnology = MOD_SYNTH;
        dests[i].caps.wChannelMask = 0xFFFF;

        dests[i].caps.wMid = 0x00FF; 	/* Manufac ID */
        dests[i].caps.wPid = 0x0001; 	/* Product ID */
        dests[i].caps.vDriverVersion = 0x0001;
        dests[i].caps.dwSupport = MIDICAPS_VOLUME;
        dests[i].caps.wVoices = 16;
        dests[i].caps.wNotes = 16;
    }
    /* initialise available destinations */
    for (i = MAX_MIDI_SYNTHS; i < num_dests; i++)
    {
        dests[i].dest = MIDIGetDestination(i - MAX_MIDI_SYNTHS);

        sc = MIDIObjectGetStringProperty(dests[i].dest, kMIDIPropertyName, &name);
        if (!sc)
        {
            int len = min(CFStringGetLength(name), ARRAY_SIZE(dests[i].caps.szPname) - 1);
            CFStringGetCharacters(name, CFRangeMake(0, len), dests[i].caps.szPname);
            dests[i].caps.szPname[len] = '\0';
        }

        dests[i].caps.wTechnology = MOD_MIDIPORT;
        dests[i].caps.wChannelMask = 0xFFFF;

        dests[i].caps.wMid = 0x00FF; 	/* Manufac ID */
        dests[i].caps.wPid = 0x0001;
        dests[i].caps.vDriverVersion = 0x0001;
        dests[i].caps.dwSupport = 0;
        dests[i].caps.wVoices = 0;
        dests[i].caps.wNotes = 0;
    }

    params->num_dests = num_dests;
    params->num_srcs = num_srcs;
    params->dests = dests;
    params->srcs = srcs;
    params->midi_out_port = (void *)midi_out_port;
    params->midi_in_port = (void *)midi_in_port;

    *params->err = DRV_SUCCESS;
    return STATUS_SUCCESS;
}

NTSTATUS midi_release(void *args)
{
    CFMessagePortRef msg_port;

    if (num_srcs)
    {
        /* Stop CFRunLoop in MIDIIn_MessageThread */
        msg_port = CFMessagePortCreateRemote(kCFAllocatorDefault, midi_in_thread_port_name);
        CFMessagePortSendRequest(msg_port, 1, NULL, 0.0, 0.0, NULL, NULL);
        CFRelease(msg_port);
    }

    if (midi_client) MIDIClientDispose(midi_client); /* MIDIClientDispose will close all ports */

    free(srcs);
    free(dests);

    return STATUS_SUCCESS;
}

/*
 *  MIDI Synth Unit
 */
static BOOL synth_unit_create_default(AUGraph *graph, AudioUnit *synth)
{
    AudioComponentDescription desc;
    AUNode synth_node;
    AUNode out_node;
    OSStatus sc;

    sc = NewAUGraph(graph);
    if (sc != noErr)
    {
        ERR("NewAUGraph return %s\n", wine_dbgstr_fourcc(sc));
        return FALSE;
    }

    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    /* create synth node */
    desc.componentType = kAudioUnitType_MusicDevice;
    desc.componentSubType = kAudioUnitSubType_DLSSynth;

    sc = AUGraphAddNode(*graph, &desc, &synth_node);
    if (sc != noErr)
    {
        ERR("AUGraphAddNode cannot create synthNode : %s\n", wine_dbgstr_fourcc(sc));
        return FALSE;
    }

    /* create out node */
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;

    sc = AUGraphAddNode(*graph, &desc, &out_node);
    if (sc != noErr)
    {
        ERR("AUGraphAddNode cannot create outNode %s\n", wine_dbgstr_fourcc(sc));
        return FALSE;
    }

    sc = AUGraphOpen(*graph);
    if (sc != noErr)
    {
        ERR("AUGraphOpen returns %s\n", wine_dbgstr_fourcc(sc));
        return FALSE;
    }

    /* connecting the nodes */
    sc = AUGraphConnectNodeInput(*graph, synth_node, 0, out_node, 0);
    if (sc != noErr)
    {
        ERR("AUGraphConnectNodeInput cannot connect synthNode to outNode : %s\n",
            wine_dbgstr_fourcc(sc));
        return FALSE;
    }

    /* Get the synth unit */
    sc = AUGraphNodeInfo(*graph, synth_node, 0, synth);
    if (sc != noErr)
    {
        ERR("AUGraphNodeInfo return %s\n", wine_dbgstr_fourcc(sc));
        return FALSE;
    }

    return TRUE;
}

static BOOL synth_unit_init(AudioUnit synth, AUGraph graph)
{
    OSStatus sc;

    sc = AUGraphInitialize(graph);
    if (sc != noErr)
    {
        ERR("AUGraphInitialize(%p) returns %s\n", graph, wine_dbgstr_fourcc(sc));
        return FALSE;
    }

    sc = AUGraphStart(graph);
    if (sc != noErr)
    {
        ERR("AUGraphStart(%p) returns %s\n", graph, wine_dbgstr_fourcc(sc));
        return FALSE;
    }

    return TRUE;
}

static void set_out_notify(struct notify_context *notify, struct midi_dest *dest, WORD dev_id, WORD msg,
                           DWORD_PTR param_1, DWORD_PTR param_2)
{
    notify->send_notify = TRUE;
    notify->dev_id = dev_id;
    notify->msg = msg;
    notify->param_1 = param_1;
    notify->param_2 = param_2;
    notify->callback = dest->midiDesc.dwCallback;
    notify->flags = dest->wFlags;
    notify->device = dest->midiDesc.hMidi;
    notify->instance = dest->midiDesc.dwInstance;
}

static DWORD midi_out_open(WORD dev_id, MIDIOPENDESC *midi_desc, DWORD flags, struct notify_context *notify)
{
    struct midi_dest *dest;

    TRACE("dev_id = %d desc = %p flags = %08x\n", dev_id, midi_desc, flags);

    if (!midi_desc) return MMSYSERR_INVALPARAM;

    if (dev_id >= num_dests)
    {
        WARN("bad device ID : %d\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }
    if (dests[dev_id].midiDesc.hMidi != 0)
    {
        WARN("device already open!\n");
        return MMSYSERR_ALLOCATED;
    }
    if ((flags & ~CALLBACK_TYPEMASK) != 0)
    {
        WARN("bad flags\n");
        return MMSYSERR_INVALFLAG;
    }

    dest = dests + dev_id;
    if (dest->caps.wTechnology == MOD_SYNTH)
    {
        if (!synth_unit_create_default(&dest->graph, &dest->synth))
        {
            ERR("SynthUnit_CreateDefaultSynthUnit dest=%p failed\n", dest);
            return MMSYSERR_ERROR;
        }
        if (!synth_unit_init(dest->synth, dest->graph))
        {
            ERR("SynthUnit_Initialise dest=%p failed\n", dest);
            return MMSYSERR_ERROR;
        }
    }
    dest->wFlags = HIWORD(flags & CALLBACK_TYPEMASK);
    dest->midiDesc = *midi_desc;

    set_out_notify(notify, dest, dev_id, MOM_OPEN, 0, 0);

    return MMSYSERR_NOERROR;
}

NTSTATUS midi_out_message(void *args)
{
    struct midi_out_message_params *params = args;

    params->notify->send_notify = FALSE;

    switch (params->msg)
    {
    case DRVM_INIT:
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
        *params->err = MMSYSERR_NOERROR;
        break;
    case MODM_OPEN:
        *params->err = midi_out_open(params->dev_id, (MIDIOPENDESC *)params->param_1, params->param_2, params->notify);
        break;
    default:
        TRACE("Unsupported message\n");
        *params->err = MMSYSERR_NOTSUPPORTED;
    }

    return STATUS_SUCCESS;
}
