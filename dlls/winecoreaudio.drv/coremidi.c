/*
 * Wine Midi driver for MacOSX
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(midi);

#ifdef HAVE_COREAUDIO_COREAUDIO_H
#include <CoreMIDI/CoreMIDI.h>

#include "coremidi.h"

MIDIClientRef CoreMIDI_CreateClient(CFStringRef name)
{
    MIDIClientRef client = NULL;

    if (MIDIClientCreate(name, NULL /* FIXME use notify proc */, NULL, &client) != noErr)
        return NULL;

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

#endif /* HAVE_COREAUDIO_COREAUDIO_H */
