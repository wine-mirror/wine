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

#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winternl.h"
#include "mmsystem.h"
#include "mmddk.h"
#include "mmdeviceapi.h"
#include "audioclient.h"
#include "wine/debug.h"
#include "wine/unixlib.h"

#include "coreaudio.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(midi);

struct midi_dest
{
    /* graph and synth are only used for MIDI Synth */
    AUGraph graph;
    AudioUnit synth;

    MIDIEndpointRef dest;

    MIDIOUTCAPSW caps;
    MIDIOPENDESC midiDesc;
    BYTE runningStatus;
    WORD wFlags;
};

struct midi_src
{
    MIDIEndpointRef source;

    WORD wDevID;
    int state; /* 0 is no recording started, 1 in recording, bit 2 set if in sys exclusive recording */
    MIDIINCAPSW caps;
    MIDIOPENDESC midiDesc;
    LPMIDIHDR lpQueueHdr;
    WORD wFlags;
    UINT startTime;
};

static MIDIClientRef midi_client;
static MIDIPortRef midi_out_port, midi_in_port;
static UINT num_dests, num_srcs;
static struct midi_dest *dests;
static struct midi_src *srcs;

static pthread_mutex_t midi_in_mutex = PTHREAD_MUTEX_INITIALIZER;

#define NOTIFY_BUFFER_SIZE 64 + 1 /* + 1 for the sentinel */
static pthread_mutex_t notify_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t notify_cond = PTHREAD_COND_INITIALIZER;
static BOOL notify_quit;
static struct notify_context notify_buffer[NOTIFY_BUFFER_SIZE];
static struct notify_context *notify_read, *notify_write;

#define MAX_MIDI_SYNTHS 1

static void midi_in_lock(BOOL lock)
{
    if (lock) pthread_mutex_lock(&midi_in_mutex);
    else pthread_mutex_unlock(&midi_in_mutex);
}

static void set_in_notify(struct notify_context *notify, struct midi_src *src, WORD dev_id, WORD msg,
                          UINT_PTR param_1, UINT_PTR param_2)
{
    notify->send_notify = TRUE;
    notify->dev_id = dev_id;
    notify->msg = msg;
    notify->param_1 = param_1;
    notify->param_2 = param_2;
    notify->callback = src->midiDesc.dwCallback;
    notify->flags = src->wFlags;
    notify->device = src->midiDesc.hMidi;
    notify->instance = src->midiDesc.dwInstance;
}

/*
 * notify buffer: The notification ring buffer is implemented so that
 * there is always at least one unused sentinel before the current
 * read position in order to allow detection of the full vs empty
 * state.
 */
static struct notify_context *notify_buffer_next(struct notify_context *notify)
{
    if (++notify >= notify_buffer + ARRAY_SIZE(notify_buffer))
        notify = notify_buffer;

    return notify;
}

static void notify_buffer_add(struct notify_context *notify)
{
    struct notify_context *next = notify_buffer_next(notify_write);

    if (next == notify_read) /* buffer is full - we can't issue a WARN() in a non-Win32 thread */
        notify_read = notify_buffer_next(notify_read); /* drop the oldest notification */
    *notify_write = *notify;
    notify_write = next;
}

static BOOL notify_buffer_empty(void)
{
    return notify_read == notify_write;
}

static BOOL notify_buffer_remove(struct notify_context *notify)
{
    if (notify_buffer_empty()) return FALSE;

    *notify = *notify_read;
    notify_read = notify_buffer_next(notify_read);
    return TRUE;
}

static void notify_post(struct notify_context *notify)
{
    pthread_mutex_lock(&notify_mutex);

    if (notify) notify_buffer_add(notify);
    else notify_quit = TRUE;
    pthread_cond_signal(&notify_cond);

    pthread_mutex_unlock(&notify_mutex);
}

/*
 *  CoreMIDI IO threaded callback,
 *  we can't call Wine debug channels, critical section or anything using NtCurrentTeb here.
 */
static uint64_t get_time_ms(void)
{
    static mach_timebase_info_data_t timebase;

    if (!timebase.denom) mach_timebase_info(&timebase);
    return mach_absolute_time() / 1000000 * timebase.numer / timebase.denom;
}

static void process_sysex_packet(struct midi_src *src, MIDIPacket *packet)
{
    unsigned int pos = 0, len = packet->length, copy_len;
    UINT current_time = get_time_ms() - src->startTime;
    struct notify_context notify;

    src->state |= 2;

    midi_in_lock(TRUE);

    while (len)
    {
        MIDIHDR *hdr = src->lpQueueHdr;
        if (!hdr) break;

        copy_len = min(len, hdr->dwBufferLength - hdr->dwBytesRecorded);
        memcpy(hdr->lpData + hdr->dwBytesRecorded, packet->data + pos, copy_len);
        hdr->dwBytesRecorded += copy_len;
        len -= copy_len;
        pos += copy_len;

        if ((hdr->dwBytesRecorded == hdr->dwBufferLength) ||
            (*(BYTE*)(hdr->lpData + hdr->dwBytesRecorded - 1) == 0xf7))
        { /* buffer full or end of sysex message */
            src->lpQueueHdr = hdr->lpNext;
            hdr->dwFlags &= ~MHDR_INQUEUE;
            hdr->dwFlags |= MHDR_DONE;
            set_in_notify(&notify, src, src->wDevID, MIM_LONGDATA, (UINT_PTR)hdr, current_time);
            notify_post(&notify);
            src->state &= ~2;
        }
    }

    midi_in_lock(FALSE);
}

static void process_small_packet(struct midi_src *src, MIDIPacket *packet)
{
    UINT current_time = get_time_ms() - src->startTime, data;
    struct notify_context notify;
    unsigned int pos = 0;

    while (pos < packet->length)
    {
        data = 0;
        switch (packet->data[pos] & 0xf0)
        {
        case 0xf0:
            data = packet->data[pos];
            pos++;
            break;
        case 0xc0:
        case 0xd0:
            data = (packet->data[pos + 1] <<  8) | packet->data[pos];
            pos += 2;
            break;
        default:
            data = (packet->data[pos + 2] << 16) | (packet->data[pos + 1] <<  8) |
                packet->data[pos];
            pos += 3;
            break;
        }
        set_in_notify(&notify, src, src->wDevID, MIM_DATA, data, current_time);
        notify_post(&notify);
    }
}

static void midi_in_read_proc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
    MIDIPacket *packet = (MIDIPacket *)pktlist->packet;
    WORD dev_id = *(WORD *)connRefCon;
    struct midi_src *src;
    unsigned int i;

    if (dev_id >= num_srcs) return;
    src = srcs + dev_id;
    if (src->state < 1) /* input not started */
        return;

    for (i = 0; i < pktlist->numPackets; ++i)
    {
        if (packet->data[0] == 0xf0 || src->state & 2)
            process_sysex_packet(src, packet);
        else
            process_small_packet(src, packet);

        packet = MIDIPacketNext(packet);
    }
}

NTSTATUS unix_midi_init(void *args)
{
    CFStringRef name = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("wineMIDIClient.%d"), getpid());
    struct midi_init_params *params = args;
    OSStatus sc;
    UINT i;

    pthread_mutex_lock(&notify_mutex);
    notify_quit = FALSE;
    notify_read = notify_write = notify_buffer;
    pthread_mutex_unlock(&notify_mutex);

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

    *params->err = DRV_SUCCESS;
    return STATUS_SUCCESS;
}

NTSTATUS unix_midi_release(void *args)
{
    /* stop the notify_wait thread */
    notify_post(NULL);

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
        ERR("NewAUGraph return %s\n", coreaudio_dbgstr_fourcc(sc));
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
        ERR("AUGraphAddNode cannot create synthNode : %s\n", coreaudio_dbgstr_fourcc(sc));
        return FALSE;
    }

    /* create out node */
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;

    sc = AUGraphAddNode(*graph, &desc, &out_node);
    if (sc != noErr)
    {
        ERR("AUGraphAddNode cannot create outNode %s\n", coreaudio_dbgstr_fourcc(sc));
        return FALSE;
    }

    sc = AUGraphOpen(*graph);
    if (sc != noErr)
    {
        ERR("AUGraphOpen returns %s\n", coreaudio_dbgstr_fourcc(sc));
        return FALSE;
    }

    /* connecting the nodes */
    sc = AUGraphConnectNodeInput(*graph, synth_node, 0, out_node, 0);
    if (sc != noErr)
    {
        ERR("AUGraphConnectNodeInput cannot connect synthNode to outNode : %s\n",
            coreaudio_dbgstr_fourcc(sc));
        return FALSE;
    }

    /* Get the synth unit */
    sc = AUGraphNodeInfo(*graph, synth_node, 0, synth);
    if (sc != noErr)
    {
        ERR("AUGraphNodeInfo return %s\n", coreaudio_dbgstr_fourcc(sc));
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
        ERR("AUGraphInitialize(%p) returns %s\n", graph, coreaudio_dbgstr_fourcc(sc));
        return FALSE;
    }

    sc = AUGraphStart(graph);
    if (sc != noErr)
    {
        ERR("AUGraphStart(%p) returns %s\n", graph, coreaudio_dbgstr_fourcc(sc));
        return FALSE;
    }

    return TRUE;
}

static BOOL synth_unit_close(AUGraph graph)
{
    OSStatus sc;

    sc = AUGraphStop(graph);
    if (sc != noErr)
    {
        ERR("AUGraphStop(%p) returns %s\n", graph, coreaudio_dbgstr_fourcc(sc));
        return FALSE;
    }

    sc = DisposeAUGraph(graph);
    if (sc != noErr)
    {
        ERR("DisposeAUGraph(%p) returns %s\n", graph, coreaudio_dbgstr_fourcc(sc));
        return FALSE;
    }

    return TRUE;
}

static void set_out_notify(struct notify_context *notify, struct midi_dest *dest, WORD dev_id, WORD msg,
                           UINT_PTR param_1, UINT_PTR param_2)
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

static UINT midi_out_open(WORD dev_id, MIDIOPENDESC *midi_desc, UINT flags, struct notify_context *notify)
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
    dest->runningStatus = 0;
    dest->wFlags = HIWORD(flags & CALLBACK_TYPEMASK);
    dest->midiDesc = *midi_desc;

    set_out_notify(notify, dest, dev_id, MOM_OPEN, 0, 0);

    return MMSYSERR_NOERROR;
}

static UINT midi_out_close(WORD dev_id, struct notify_context *notify)
{
    struct midi_dest *dest;

    TRACE("dev_id = %d\n", dev_id);

    if (dev_id >= num_dests)
    {
        WARN("bad device ID : %d\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }

    dest = dests + dev_id;

    if (dest->caps.wTechnology == MOD_SYNTH)
        synth_unit_close(dest->graph);
    dest->graph = 0;
    dest->synth = 0;

    set_out_notify(notify, dest, dev_id, MOM_CLOSE, 0, 0);

    dest->midiDesc.hMidi = 0;

    return MMSYSERR_NOERROR;
}

static void midi_send(MIDIPortRef port, MIDIEndpointRef dest, UInt8 *buffer, unsigned len)
{
    Byte packet_buf[512];
    MIDIPacketList *packet_list = (MIDIPacketList *)packet_buf;
    MIDIPacket *packet = MIDIPacketListInit(packet_list);

    packet = MIDIPacketListAdd(packet_list, sizeof(packet_buf), packet, mach_absolute_time(), len, buffer);
    if (packet) MIDISend(port, dest, packet_list);
}

static UINT midi_out_data(WORD dev_id, UINT data)
{
    struct midi_dest *dest;
    UInt8 bytes[3];
    size_t count = sizeof(bytes);
    OSStatus sc;

    TRACE("dev_id = %d data = %08x\n", dev_id, data);

    if (dev_id >= num_dests)
    {
        WARN("bad device ID : %d\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }
    dest = dests + dev_id;

    bytes[0] = data & 0xff;
    if (bytes[0] & 0x80)
    {
        bytes[1] = (data >> 8) & 0xff;
        bytes[2] = (data >> 16) & 0xff;
        if (bytes[0] < 0xF0)
            dest->runningStatus = bytes[0];
        else if (bytes[0] <= 0xF7)
            dest->runningStatus = 0;
    }
    else if (dest->runningStatus)
    {
        bytes[0] = dest->runningStatus;
        bytes[1] = data & 0xff;
        bytes[2] = (data >> 8) & 0xff;
    }
    else
    {
        FIXME("ooch %x\n", data);
        return MMSYSERR_NOERROR;
    }

    if (dest->caps.wTechnology == MOD_SYNTH)
    {
        sc = MusicDeviceMIDIEvent(dest->synth, bytes[0], bytes[1], bytes[2], 0);
        if (sc != noErr)
        {
            ERR("MusicDeviceMIDIEvent returns %s\n", coreaudio_dbgstr_fourcc(sc));
            return MMSYSERR_ERROR;
        }
    }
    else
    {
        /* MODM_DATA always includes 3 bytes, but some message types are only
         * 1 or 2 bytes long. The docs say that "The driver must parse the event
         * to determine how many bytes to transfer"
         *
         * Sending 3 bytes seems to work for most 1/2 byte messages
         * (Core MIDI must be doing some parsing), but some 2 byte messages
         * result in duplicates if all 3 bytes are sent.
         */
        switch (bytes[0] & 0xF0)
        {
            case 0xC0:  /* Program Change */
            case 0xD0:  /* Channel Pressure (After-touch) */
                count = 2;
                break;
        }

        midi_send(midi_out_port, dest->dest, bytes, count);
    }

    return MMSYSERR_NOERROR;
}

static UINT midi_out_long_data(WORD dev_id, MIDIHDR *hdr, UINT hdr_size, struct notify_context *notify)
{
    struct midi_dest *dest;
    OSStatus sc;

    TRACE("dev_id = %d midi_hdr = %p hdr_size = %d\n", dev_id, hdr, hdr_size);

    if (dev_id >= num_dests)
    {
        WARN("bad device ID : %d\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }
    if (!hdr)
    {
        WARN("Invalid Parameter\n");
        return MMSYSERR_INVALPARAM;
    }
    if (!hdr->lpData || !(hdr->dwFlags & MHDR_PREPARED))
        return MIDIERR_UNPREPARED;

    if (hdr->dwFlags & MHDR_INQUEUE)
        return MIDIERR_STILLPLAYING;

    hdr->dwFlags &= ~MHDR_DONE;
    hdr->dwFlags |= MHDR_INQUEUE;

    if ((UInt8)hdr->lpData[0] != 0xf0)
        /* System Exclusive */
        ERR("Add missing 0xf0 marker at the beginning of system exclusive byte stream\n");

    if ((UInt8)hdr->lpData[hdr->dwBufferLength - 1] != 0xF7)
        /* Send end of System Exclusive */
        ERR("Add missing 0xf7 marker at the end of system exclusive byte stream\n");

    dest = dests + dev_id;

    if (dest->caps.wTechnology == MOD_SYNTH) /* FIXME */
    {
        sc = MusicDeviceSysEx(dest->synth, (const UInt8 *)hdr->lpData, hdr->dwBufferLength);
        if (sc != noErr)
        {
            ERR("MusicDeviceSysEx returns %s\n", coreaudio_dbgstr_fourcc(sc));
            return MMSYSERR_ERROR;
        }
    }
    else if (dest->caps.wTechnology == MOD_MIDIPORT)
        midi_send(midi_out_port, dest->dest, (UInt8 *)hdr->lpData, hdr->dwBufferLength);

    dest->runningStatus = 0;
    hdr->dwFlags &= ~MHDR_INQUEUE;
    hdr->dwFlags |= MHDR_DONE;

    set_out_notify(notify, dest, dev_id, MOM_DONE, (UINT_PTR)hdr, 0);

    return MMSYSERR_NOERROR;
}

static UINT midi_out_prepare(WORD dev_id, MIDIHDR *hdr, UINT hdr_size)
{
    TRACE("dev_id = %d midi_hdr = %p hdr_size = %d\n", dev_id, hdr, hdr_size);

    if (hdr_size < offsetof(MIDIHDR, dwOffset) || !hdr || !hdr->lpData)
	return MMSYSERR_INVALPARAM;
    if (hdr->dwFlags & MHDR_PREPARED)
	return MMSYSERR_NOERROR;

    hdr->lpNext = 0;
    hdr->dwFlags |= MHDR_PREPARED;
    hdr->dwFlags &= ~(MHDR_DONE | MHDR_INQUEUE);
    return MMSYSERR_NOERROR;
}

static UINT midi_out_unprepare(WORD dev_id, MIDIHDR *hdr, UINT hdr_size)
{
    TRACE("dev_id = %d midi_hdr = %p hdr_size = %d\n", dev_id, hdr, hdr_size);

    if (hdr_size < offsetof(MIDIHDR, dwOffset) || !hdr || !hdr->lpData)
	return MMSYSERR_INVALPARAM;
    if (!(hdr->dwFlags & MHDR_PREPARED))
	return MMSYSERR_NOERROR;
    if (hdr->dwFlags & MHDR_INQUEUE)
	return MIDIERR_STILLPLAYING;

    hdr->dwFlags &= ~MHDR_PREPARED;
    return MMSYSERR_NOERROR;
}

static UINT midi_out_get_devcaps(WORD dev_id, MIDIOUTCAPSW *caps, UINT size)
{
    TRACE("dev_id = %d caps = %p size = %d\n", dev_id, caps, size);

    if (!caps)
    {
        WARN("Invalid Parameter\n");
        return MMSYSERR_INVALPARAM;
    }

    if (dev_id >= num_dests)
    {
        WARN("bad device ID : %d\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }
    memcpy(caps, &dests[dev_id].caps, min(size, sizeof(*caps)));
    return MMSYSERR_NOERROR;
}

static UINT midi_out_get_num_devs(void)
{
    TRACE("\n");
    return num_dests;
}

static UINT midi_out_get_volume(WORD dev_id, UINT *volume)
{
    TRACE("%d\n", dev_id);

    if (dev_id >= num_dests)
    {
        WARN("bad device ID : %d\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }
    if (!volume)
    {
        WARN("Invalid Parameter\n");
        return MMSYSERR_INVALPARAM;
    }

    if (dests[dev_id].caps.wTechnology == MOD_SYNTH)
    {
        static int once;
        float left;

        if (!once++) FIXME("independent left/right volume not implemented\n");
        AudioUnitGetParameter(dests[dev_id].synth, kHALOutputParam_Volume, kAudioUnitParameterFlag_Output, 0, &left);
        *volume = (WORD)(left * 0xffff) + ((WORD)(left * 0xffff) << 16);
        return MMSYSERR_NOERROR;
    }

    return MMSYSERR_NOTSUPPORTED;
}

static UINT midi_out_set_volume(WORD dev_id, UINT volume)
{
    TRACE("dev_id = %d vol = %08x\n", dev_id, volume);

    if (dev_id >= num_dests)
    {
        WARN("bad device ID : %d\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }
    if (dests[dev_id].caps.wTechnology == MOD_SYNTH)
    {
        float left, right;
        static int once;

        if (!once++) FIXME("independent left/right volume not implemented\n");
        left  = LOWORD(volume) / 65535.0f;
        right = HIWORD(volume) / 65535.0f;

        AudioUnitSetParameter(dests[dev_id].synth, kHALOutputParam_Volume, kAudioUnitParameterFlag_Output,
                              0, fmaxf(left, right), 0);
        return MMSYSERR_NOERROR;
    }

    return MMSYSERR_NOTSUPPORTED;
}

static UINT midi_out_reset(WORD dev_id)
{
    unsigned chn;

    TRACE("%d\n", dev_id);

    if (dev_id >= num_dests)
    {
        WARN("bad device ID : %d\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }
    if (dests[dev_id].caps.wTechnology == MOD_SYNTH)
    {
        for (chn = 0; chn < 16; chn++)
        {
            /* turn off every note */
            MusicDeviceMIDIEvent(dests[dev_id].synth, 0xB0 | chn, 0x7B, 0, 0);
            /* remove sustain on channel */
            MusicDeviceMIDIEvent(dests[dev_id].synth, 0xB0 | chn, 0x40, 0, 0);
        }
    }
    else FIXME("MOD_MIDIPORT\n");

    dests[dev_id].runningStatus = 0;

    /* FIXME: the LongData buffers must also be returned to the app */
    return MMSYSERR_NOERROR;
}

static UINT midi_in_open(WORD dev_id, MIDIOPENDESC *midi_desc, UINT flags, struct notify_context *notify)
{
    struct midi_src *src;

    TRACE("dev_id = %d desc = %p flags = %08x\n", dev_id, midi_desc, flags);

    if (!midi_desc)
    {
        WARN("Invalid Parameter\n");
        return MMSYSERR_INVALPARAM;
    }
    if (dev_id >= num_srcs)
    {
        WARN("bad device ID : %d\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }
    src = srcs + dev_id;

    if (src->midiDesc.hMidi)
    {
        WARN("device already open !\n");
        return MMSYSERR_ALLOCATED;
    }
    if (flags & MIDI_IO_STATUS)
    {
        FIXME("No support for MIDI_IO_STATUS in flags yet, ignoring it\n");
        flags &= ~MIDI_IO_STATUS;
    }
    if (flags & ~CALLBACK_TYPEMASK)
    {
        FIXME("Bad flags\n");
        return MMSYSERR_INVALFLAG;
    }

    src->wFlags = HIWORD(flags & CALLBACK_TYPEMASK);
    src->lpQueueHdr = NULL;
    src->midiDesc = *midi_desc;
    src->startTime = 0;
    src->state = 0;

    set_in_notify(notify, src, dev_id, MIM_OPEN, 0, 0);

    return MMSYSERR_NOERROR;
}

static UINT midi_in_close(WORD dev_id, struct notify_context *notify)
{
    struct midi_src *src;

    TRACE("dev_id = %d\n", dev_id);

    if (dev_id >= num_srcs)
    {
        WARN("bad device ID : %d\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }
    src = srcs + dev_id;

    if (!src->midiDesc.hMidi)
    {
        WARN("device not opened !\n");
        return MMSYSERR_ERROR;
    }
    if (src->lpQueueHdr) return MIDIERR_STILLPLAYING;

    set_in_notify(notify, src, dev_id, MIM_CLOSE, 0, 0);
    src->midiDesc.hMidi = 0;

    return MMSYSERR_NOERROR;
}

static UINT midi_in_add_buffer(WORD dev_id, MIDIHDR *hdr, UINT hdr_size)
{
    MIDIHDR **next;

    TRACE("dev_id = %d hdr = %p hdr_size = %d\n", dev_id, hdr, hdr_size);

    if (dev_id >= num_srcs)
    {
        WARN("bad device ID : %d\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }
    if (!hdr || hdr_size < offsetof(MIDIHDR, dwOffset) || !hdr->dwBufferLength)
    {
        WARN("Invalid Parameter\n");
        return MMSYSERR_INVALPARAM;
    }
    if (hdr->dwFlags & MHDR_INQUEUE)
    {
        WARN("Still playing\n");
        return MIDIERR_STILLPLAYING;
    }
    if (!(hdr->dwFlags & MHDR_PREPARED))
    {
        WARN("Unprepared\n");
        return MIDIERR_UNPREPARED;
    }

    hdr->dwFlags &= ~WHDR_DONE;
    hdr->dwFlags |= MHDR_INQUEUE;
    hdr->dwBytesRecorded = 0;
    hdr->lpNext = NULL;

    midi_in_lock(TRUE);

    next = &srcs[dev_id].lpQueueHdr;
    while (*next) next = &(*next)->lpNext;
    *next = hdr;

    midi_in_lock(FALSE);

    return MMSYSERR_NOERROR;
}

static UINT midi_in_prepare(WORD dev_id, MIDIHDR *hdr, UINT hdr_size)
{
    TRACE("dev_id = %d hdr = %p hdr_size = %d\n", dev_id, hdr, hdr_size);

    if (hdr_size < offsetof(MIDIHDR, dwOffset) || !hdr || !hdr->lpData)
        return MMSYSERR_INVALPARAM;
    if (hdr->dwFlags & MHDR_PREPARED)
        return MMSYSERR_NOERROR;

    hdr->lpNext = NULL;
    hdr->dwFlags |= MHDR_PREPARED;
    hdr->dwFlags &= ~(MHDR_DONE | MHDR_INQUEUE);
    return MMSYSERR_NOERROR;
}

static UINT midi_in_unprepare(WORD dev_id, MIDIHDR *hdr, UINT hdr_size)
{
    TRACE("dev_id = %d hdr = %p hdr_size = %d\n", dev_id, hdr, hdr_size);

    if (hdr_size < offsetof(MIDIHDR, dwOffset) || !hdr || !hdr->lpData)
        return MMSYSERR_INVALPARAM;
    if (!(hdr->dwFlags & MHDR_PREPARED))
        return MMSYSERR_NOERROR;
    if (hdr->dwFlags & MHDR_INQUEUE)
        return MIDIERR_STILLPLAYING;

    hdr->dwFlags &= ~MHDR_PREPARED;
    return MMSYSERR_NOERROR;
}

static UINT midi_in_get_devcaps(WORD dev_id, MIDIINCAPSW *caps, UINT size)
{
    TRACE("dev_id = %d caps = %p size = %d\n", dev_id, caps, size);

    if (!caps)
    {
        WARN("Invalid Parameter\n");
        return MMSYSERR_INVALPARAM;
    }
    if (dev_id >= num_srcs)
    {
        WARN("bad device ID : %d\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }

    memcpy(caps, &srcs[dev_id].caps, min(size, sizeof(*caps)));
    return MMSYSERR_NOERROR;
}

static UINT midi_in_get_num_devs(void)
{
    TRACE("\n");
    return num_srcs;
}

static UINT midi_in_start(WORD dev_id)
{
    TRACE("%d\n", dev_id);

    if (dev_id >= num_srcs)
    {
        WARN("bad device ID : %d\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }
    srcs[dev_id].state = 1;
    srcs[dev_id].startTime = get_time_ms();
    return MMSYSERR_NOERROR;
}

static UINT midi_in_stop(WORD dev_id)
{
    TRACE("%d\n", dev_id);

    if (dev_id >= num_srcs)
    {
        WARN("bad device ID : %d\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }
    srcs[dev_id].state = 0;
    return MMSYSERR_NOERROR;
}

static UINT midi_in_reset(WORD dev_id, struct notify_context *notify)
{
    UINT cur_time = get_time_ms();
    UINT err = MMSYSERR_NOERROR;
    struct midi_src *src;
    MIDIHDR *hdr;

    TRACE("%d\n", dev_id);

    if (dev_id >= num_srcs)
    {
        WARN("bad device ID : %d\n", dev_id);
        return MMSYSERR_BADDEVICEID;
    }
    src = srcs + dev_id;

    midi_in_lock(TRUE);

    if (src->lpQueueHdr)
    {
        hdr = src->lpQueueHdr;
        src->lpQueueHdr = hdr->lpNext;
        hdr->dwFlags &= ~MHDR_INQUEUE;
        hdr->dwFlags |= MHDR_DONE;
        set_in_notify(notify, src, dev_id, MIM_LONGDATA, (UINT_PTR)hdr, cur_time - src->startTime);
        if (src->lpQueueHdr) err = ERROR_RETRY; /* ask the client to call again */
    }

    midi_in_lock(FALSE);

    return err;
}

NTSTATUS unix_midi_out_message(void *args)
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
    case MODM_CLOSE:
        *params->err = midi_out_close(params->dev_id, params->notify);
        break;
    case MODM_DATA:
        *params->err = midi_out_data(params->dev_id, params->param_1);
        break;
    case MODM_LONGDATA:
        *params->err = midi_out_long_data(params->dev_id, (MIDIHDR *)params->param_1, params->param_2, params->notify);
        break;
    case MODM_PREPARE:
        *params->err = midi_out_prepare(params->dev_id, (MIDIHDR *)params->param_1, params->param_2);
        break;
    case MODM_UNPREPARE:
        *params->err = midi_out_unprepare(params->dev_id, (MIDIHDR *)params->param_1, params->param_2);
        break;
    case MODM_GETDEVCAPS:
        *params->err = midi_out_get_devcaps(params->dev_id, (MIDIOUTCAPSW *)params->param_1, params->param_2);
        break;
    case MODM_GETNUMDEVS:
        *params->err = midi_out_get_num_devs();
        break;
    case MODM_GETVOLUME:
        *params->err = midi_out_get_volume(params->dev_id, (UINT *)params->param_1);
        break;
    case MODM_SETVOLUME:
        *params->err = midi_out_set_volume(params->dev_id, params->param_1);
        break;
    case MODM_RESET:
        *params->err = midi_out_reset(params->dev_id);
        break;
    default:
        TRACE("Unsupported message\n");
        *params->err = MMSYSERR_NOTSUPPORTED;
    }

    return STATUS_SUCCESS;
}

NTSTATUS unix_midi_in_message(void *args)
{
    struct midi_in_message_params *params = args;

    params->notify->send_notify = FALSE;

    switch (params->msg)
    {
    case DRVM_INIT:
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
        *params->err = MMSYSERR_NOERROR;
        break;
    case MIDM_OPEN:
        *params->err = midi_in_open(params->dev_id, (MIDIOPENDESC *)params->param_1, params->param_2, params->notify);
        break;
    case MIDM_CLOSE:
        *params->err = midi_in_close(params->dev_id, params->notify);
        break;
    case MIDM_ADDBUFFER:
        *params->err = midi_in_add_buffer(params->dev_id, (MIDIHDR *)params->param_1, params->param_2);
        break;
    case MIDM_PREPARE:
        *params->err = midi_in_prepare(params->dev_id, (MIDIHDR *)params->param_1, params->param_2);
        break;
    case MIDM_UNPREPARE:
        *params->err = midi_in_unprepare(params->dev_id, (MIDIHDR *)params->param_1, params->param_2);
        break;
    case MIDM_GETDEVCAPS:
        *params->err = midi_in_get_devcaps(params->dev_id, (MIDIINCAPSW *)params->param_1, params->param_2);
        break;
    case MIDM_GETNUMDEVS:
        *params->err = midi_in_get_num_devs();
        break;
    case MIDM_START:
        *params->err = midi_in_start(params->dev_id);
        break;
    case MIDM_STOP:
        *params->err = midi_in_stop(params->dev_id);
        break;
    case MIDM_RESET:
        *params->err = midi_in_reset(params->dev_id, params->notify);
        break;
    default:
        TRACE("Unsupported message\n");
        *params->err = MMSYSERR_NOTSUPPORTED;
    }

    return STATUS_SUCCESS;
}

NTSTATUS unix_midi_notify_wait(void *args)
{
    struct midi_notify_wait_params *params = args;

    pthread_mutex_lock(&notify_mutex);

    while (!notify_quit && notify_buffer_empty())
        pthread_cond_wait(&notify_cond, &notify_mutex);

    *params->quit = notify_quit;
    if (!notify_quit) notify_buffer_remove(params->notify);

    pthread_mutex_unlock(&notify_mutex);

    return STATUS_SUCCESS;
}

#ifdef _WIN64

typedef UINT PTR32;

NTSTATUS unix_wow64_midi_init(void *args)
{
    struct
    {
        PTR32 err;
    } *params32 = args;
    struct midi_init_params params =
    {
        .err = ULongToPtr(params32->err)
    };
    return unix_midi_init(&params);
}

struct notify_context32
{
    BOOL send_notify;
    WORD dev_id;
    WORD msg;
    UINT param_1;
    UINT param_2;
    UINT callback;
    UINT flags;
    PTR32 device;
    UINT instance;
};

static void notify_to_notify32(struct notify_context32 *notify32,
                               const struct notify_context *notify)
{
    notify32->send_notify = notify->send_notify;
    notify32->dev_id = notify->dev_id;
    notify32->msg = notify->msg;
    notify32->param_1 = notify->param_1;
    notify32->param_2 = notify->param_2;
    notify32->callback = notify->callback;
    notify32->flags = notify->flags;
    notify32->device = PtrToUlong(notify->device);
    notify32->instance = notify->instance;
}

struct midi_open_desc32
{
    PTR32 hMidi;
    UINT dwCallback;
    UINT dwInstance;
    UINT dnDevNode;
    UINT cIds;
    MIDIOPENSTRMID rgIds;
};

struct midi_hdr32
{
    PTR32 lpData;
    UINT dwBufferLength;
    UINT dwBytesRecorded;
    UINT dwUser;
    UINT dwFlags;
    PTR32 lpNext;
    UINT reserved;
    UINT dwOffset;
    UINT dwReserved[8];
};

static UINT wow64_midi_out_prepare(WORD dev_id, struct midi_hdr32 *hdr, UINT hdr_size)
{
    TRACE("(%04X, %p, %d);\n", dev_id, hdr, hdr_size);

    if (hdr_size < offsetof(struct midi_hdr32, dwOffset) || !hdr || !hdr->lpData)
        return MMSYSERR_INVALPARAM;
    if (hdr->dwFlags & MHDR_PREPARED)
        return MMSYSERR_NOERROR;

    hdr->lpNext = 0;
    hdr->dwFlags |= MHDR_PREPARED;
    hdr->dwFlags &= ~(MHDR_DONE | MHDR_INQUEUE);
    return MMSYSERR_NOERROR;
}

static UINT wow64_midi_out_unprepare(WORD dev_id, struct midi_hdr32 *hdr, UINT hdr_size)
{
    TRACE("(%04X, %p, %d);\n", dev_id, hdr, hdr_size);

    if (hdr_size < offsetof(struct midi_hdr32, dwOffset) || !hdr || !hdr->lpData)
        return MMSYSERR_INVALPARAM;
    if (!(hdr->dwFlags & MHDR_PREPARED))
        return MMSYSERR_NOERROR;
    if (hdr->dwFlags & MHDR_INQUEUE)
        return MIDIERR_STILLPLAYING;

    hdr->dwFlags &= ~MHDR_PREPARED;
    return MMSYSERR_NOERROR;
}

NTSTATUS unix_wow64_midi_out_message(void *args)
{
    struct
    {
        UINT dev_id;
        UINT msg;
        UINT user;
        UINT param_1;
        UINT param_2;
        PTR32 err;
        PTR32 notify;
    } *params32 = args;
    struct notify_context32 *notify32 = ULongToPtr(params32->notify);
    struct midi_open_desc32 *desc32;
    struct midi_hdr32 *hdr32;
    struct notify_context notify;
    MIDIOPENDESC open_desc;
    MIDIHDR hdr;
    struct midi_out_message_params params =
    {
        .dev_id = params32->dev_id,
        .msg = params32->msg,
        .user = params32->user,
        .param_1 = params32->param_1,
        .param_2 = params32->param_2,
        .err = ULongToPtr(params32->err),
        .notify = &notify
    };
    notify32->send_notify = FALSE;

    switch (params32->msg)
    {
    case MODM_OPEN:
        desc32 = ULongToPtr(params32->param_1);

        open_desc.hMidi = ULongToPtr(desc32->hMidi);
        open_desc.dwCallback = desc32->dwCallback;
        open_desc.dwInstance = desc32->dwInstance;
        open_desc.dnDevNode = desc32->dnDevNode;
        open_desc.cIds = desc32->cIds;
        open_desc.rgIds.dwStreamID = desc32->rgIds.dwStreamID;
        open_desc.rgIds.wDeviceID = desc32->rgIds.wDeviceID;

        params.param_1 = (UINT_PTR)&open_desc;
        break;

    case MODM_LONGDATA:
        hdr32 = ULongToPtr(params32->param_1);

        memset(&hdr, 0, sizeof(hdr));
        hdr.lpData = ULongToPtr(hdr32->lpData);
        hdr.dwBufferLength = hdr32->dwBufferLength;
        hdr.dwFlags = hdr32->dwFlags;

        params.param_1 = (UINT_PTR)&hdr;
        params.param_2 = sizeof(hdr);
        break;

    case MODM_PREPARE: /* prepare and unprepare are easier to handle explicitly */
        hdr32 = ULongToPtr(params32->param_1);

        *params.err = wow64_midi_out_prepare(params32->dev_id, hdr32, params32->param_2);
        return STATUS_SUCCESS;

    case MODM_UNPREPARE:
        hdr32 = ULongToPtr(params32->param_1);

        *params.err = wow64_midi_out_unprepare(params32->dev_id, hdr32, params32->param_2);
        return STATUS_SUCCESS;
    }

    unix_midi_out_message(&params);

    switch (params32->msg)
    {
    case MODM_LONGDATA:
        hdr32 = ULongToPtr(params32->param_1);

        hdr32->dwFlags = hdr.dwFlags;
        break;
    }

    if (notify.send_notify)
    {
        notify_to_notify32(notify32, &notify);

        if (notify.msg == MOM_DONE)
            notify32->param_1 = params32->param_1; /* restore the 32-bit hdr */
    }
    return STATUS_SUCCESS;
}

static UINT wow64_midi_in_prepare(WORD dev_id, struct midi_hdr32 *hdr, UINT hdr_size)
{
    TRACE("(%04X, %p, %d);\n", dev_id, hdr, hdr_size);

    if (hdr_size < offsetof(struct midi_hdr32, dwOffset) || !hdr || !hdr->lpData)
        return MMSYSERR_INVALPARAM;
    if (hdr->dwFlags & MHDR_PREPARED)
        return MMSYSERR_NOERROR;

    hdr->lpNext = 0;
    hdr->dwFlags |= MHDR_PREPARED;
    hdr->dwFlags &= ~(MHDR_DONE | MHDR_INQUEUE);

    return MMSYSERR_NOERROR;
}

static UINT wow64_midi_in_unprepare(WORD dev_id, struct midi_hdr32 *hdr, UINT hdr_size)
{
    TRACE("(%04X, %p, %d);\n", dev_id, hdr, hdr_size);

    if (hdr_size < offsetof(struct midi_hdr32, dwOffset) || !hdr || !hdr->lpData)
        return MMSYSERR_INVALPARAM;
    if (!(hdr->dwFlags & MHDR_PREPARED))
        return MMSYSERR_NOERROR;
    if (hdr->dwFlags & MHDR_INQUEUE)
        return MIDIERR_STILLPLAYING;

    hdr->dwFlags &= ~MHDR_PREPARED;

    return MMSYSERR_NOERROR;
}

NTSTATUS unix_wow64_midi_in_message(void *args)
{
    struct
    {
        UINT dev_id;
        UINT msg;
        UINT user;
        UINT param_1;
        UINT param_2;
        PTR32 err;
        PTR32 notify;
    } *params32 = args;
    struct notify_context32 *notify32 = ULongToPtr(params32->notify);
    struct midi_open_desc32 *desc32;
    struct midi_hdr32 *hdr32;
    struct notify_context notify;
    MIDIOPENDESC open_desc;
    MIDIHDR *hdr = NULL;
    struct midi_in_message_params params =
    {
        .dev_id = params32->dev_id,
        .msg = params32->msg,
        .user = params32->user,
        .param_1 = params32->param_1,
        .param_2 = params32->param_2,
        .err = ULongToPtr(params32->err),
        .notify = &notify
    };
    notify32->send_notify = FALSE;

    switch (params32->msg)
    {
    case MIDM_OPEN:
        desc32 = ULongToPtr(params32->param_1);

        open_desc.hMidi = ULongToPtr(desc32->hMidi);
        open_desc.dwCallback = desc32->dwCallback;
        open_desc.dwInstance = desc32->dwInstance;
        open_desc.dnDevNode = desc32->dnDevNode;
        open_desc.cIds = desc32->cIds;
        open_desc.rgIds.dwStreamID = desc32->rgIds.dwStreamID;
        open_desc.rgIds.wDeviceID = desc32->rgIds.wDeviceID;

        params.param_1 = (UINT_PTR)&open_desc;
        break;

    case MIDM_ADDBUFFER:
        hdr32 = ULongToPtr(params32->param_1);

        hdr = calloc(1, sizeof(*hdr));
        hdr->lpData = ULongToPtr(hdr32->lpData);
        hdr->dwBufferLength = hdr32->dwBufferLength;
        hdr->dwFlags = hdr32->dwFlags;
        hdr->dwReserved[7] = params32->param_1; /* keep hdr32 for MIM_LONGDATA notification */

        params.param_1 = (UINT_PTR)hdr;
        params.param_2 = sizeof(*hdr);
        break;

    case MIDM_PREPARE: /* prepare and unprepare are easier to handle explicitly */
        hdr32 = ULongToPtr(params32->param_1);

        *params.err = wow64_midi_in_prepare(params32->dev_id, hdr32, params32->param_2);
        return STATUS_SUCCESS;

    case MIDM_UNPREPARE:
        hdr32 = ULongToPtr(params32->param_1);

        *params.err = wow64_midi_in_unprepare(params32->dev_id, hdr32, params32->param_2);
        return STATUS_SUCCESS;
    }

    unix_midi_in_message(&params);

    switch (params32->msg)
    {
    case MIDM_ADDBUFFER:
        hdr32 = ULongToPtr(params32->param_1);

        if (!*params.err)
        {
            hdr32->dwFlags = hdr->dwFlags;
            hdr32->dwBytesRecorded = hdr->dwBytesRecorded;
            hdr32->lpNext = 0;
        }
        else
            free(hdr);
        break;
    }

    if (notify.send_notify)
    {
        notify_to_notify32(notify32, &notify);

        if (notify.msg == MIM_LONGDATA)
        {
            hdr = (MIDIHDR *)notify.param_1;
            notify32->param_1 = hdr->dwReserved[7];
            hdr32 = ULongToPtr(notify32->param_1);
            hdr32->dwBytesRecorded = hdr->dwBytesRecorded;
            hdr32->dwFlags = hdr->dwFlags;
            free(hdr);
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS unix_wow64_midi_notify_wait(void *args)
{
    struct
    {
        PTR32 quit;
        PTR32 notify;
    } *params32 = args;
    struct notify_context32 *notify32 = ULongToPtr(params32->notify);
    struct midi_hdr32 *hdr32;
    struct notify_context notify;
    MIDIHDR *hdr;
    struct midi_notify_wait_params params =
    {
        .quit = ULongToPtr(params32->quit),
        .notify = &notify
    };
    notify32->send_notify = FALSE;

    unix_midi_notify_wait(&params);

    if (!*params.quit && notify.send_notify)
    {
        notify_to_notify32(notify32, &notify);

        if (notify.msg == MIM_LONGDATA)
        {
            hdr = (MIDIHDR *)notify.param_1;
            notify32->param_1 = hdr->dwReserved[7];
            hdr32 = ULongToPtr(notify32->param_1);
            hdr32->dwBytesRecorded = hdr->dwBytesRecorded;
            hdr32->dwFlags = hdr->dwFlags;
            free(hdr);
        }
    }
    return STATUS_SUCCESS;
}

#endif /* _WIN64 */
