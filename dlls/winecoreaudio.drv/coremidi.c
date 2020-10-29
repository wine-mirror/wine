/*
 * Wine Midi driver for Mac OS X
 *
 * Copyright 2006 Emmanuel Maillard
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
    unsigned int i;

    MIDIPacket *packet = (MIDIPacket *)pktlist->packet;
    for (i = 0; i < pktlist->numPackets; ++i) {
        UInt16 devID = *((UInt16 *)connRefCon);

        MIDIIn_SendMessage(devID, packet->data, packet->length);

        packet = MIDIPacketNext(packet);
    }
}

void MIDIOut_Send(MIDIPortRef port, MIDIEndpointRef dest, UInt8 *buffer, unsigned length)
{
    Byte packetBuff[512];
    MIDIPacketList *packetList = (MIDIPacketList *)packetBuff;

    MIDIPacket *packet = MIDIPacketListInit(packetList);

    packet = MIDIPacketListAdd(packetList, sizeof(packetBuff), packet, mach_absolute_time(), length, buffer);
    if (packet)
        MIDISend(port, dest, packetList);
}
