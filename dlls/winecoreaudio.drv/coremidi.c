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


#include "config.h"

#include <CoreMIDI/CoreMIDI.h>
#include <mach/mach_time.h>

#include "coremidi.h"


MIDIClientRef CoreMIDI_CreateClient(CFStringRef name)
{
    MIDIClientRef client = 0;

    if (MIDIClientCreate(name, NULL /* FIXME use notify proc */, NULL, &client) != noErr)
        return 0;

    return client;
}

void CoreMIDI_GetObjectName(MIDIObjectRef obj, char *name, int size)
{
    OSStatus err = noErr;
    CFStringRef cfname;

    err = MIDIObjectGetStringProperty(obj, kMIDIPropertyName, &cfname);
    if (err == noErr)
    {
        CFStringGetCString(cfname, name, size, kCFStringEncodingASCII);
        CFRelease(cfname);
    }
}

/*
 *  CoreMIDI IO threaded callback,
 *  we can't call Wine debug channels, critical section or anything using NtCurrentTeb here.
 */
void MIDIIn_ReadProc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
    CFMessagePortRef msg_port = CFMessagePortCreateRemote(kCFAllocatorDefault, MIDIInThreadPortName);
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
