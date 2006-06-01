/*
 * Wine Driver for CoreAudio / AudioUnit
 *
 * Copyright 2005, 2006 Emmanuel Maillard
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

WINE_DEFAULT_DEBUG_CHANNEL(wave);

#ifdef HAVE_AUDIOUNIT_AUDIOUNIT_H
#include <AudioUnit/AudioUnit.h>

extern OSStatus CoreAudio_woAudioUnitIOProc(void *inRefCon, 
				AudioUnitRenderActionFlags *ioActionFlags, 
				const AudioTimeStamp *inTimeStamp, 
				UInt32 inBusNumber, 
				UInt32 inNumberFrames, 
				AudioBufferList *ioData);

int AudioUnit_CreateDefaultAudioUnit(void *wwo, AudioUnit *au)
{
    OSStatus err;
    Component comp;
    ComponentDescription desc;
    AURenderCallbackStruct callbackStruct;
    
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    comp = FindNextComponent(NULL, &desc);
    if (comp == NULL)
        return 0;
    
    err = OpenAComponent(comp, au);
    if (comp == NULL)
        return 0;
        
    callbackStruct.inputProc = CoreAudio_woAudioUnitIOProc;
    callbackStruct.inputProcRefCon = wwo;

    err = AudioUnitSetProperty( *au, 
                                kAudioUnitProperty_SetRenderCallback, 
                                kAudioUnitScope_Input,
                                0, 
                                &callbackStruct, 
                                sizeof(callbackStruct));
    return (err == noErr);
}

int AudioUnit_CloseAudioUnit(AudioUnit au)
{
    OSStatus err = CloseComponent(au);
    return (err == noErr);
}

int AudioUnit_InitializeWithStreamDescription(AudioUnit au, AudioStreamBasicDescription *stream)
{
    OSStatus err = noErr;
        
    err = AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input,
                                0, stream, sizeof(*stream));

    if (err != noErr)
    {
        ERR("AudioUnitSetProperty return an error %c%c%c%c\n", (char) (err >> 24), (char) (err >> 16), (char) (err >> 8), (char) err);
        return 0;
    }
    
    err = AudioUnitInitialize(au);
    if (err != noErr)
    {
        ERR("AudioUnitInitialize return an error %c%c%c%c\n", (char) (err >> 24), (char) (err >> 16), (char) (err >> 8), (char) err);
        return 0;
    }
    return 1;
}

int AudioUnit_SetVolume(AudioUnit au, float left, float right)
{
    OSStatus err = noErr;
   
    err = AudioUnitSetParameter(au, kHALOutputParam_Volume, kAudioUnitParameterFlag_Output, 0, left, 0);
                                
    if (err != noErr)
    {
        ERR("AudioUnitSetParameter return an error %c%c%c%c\n", (char) (err >> 24), (char) (err >> 16), (char) (err >> 8), (char) err);
        return 0;
    }
    return 1;
}

int AudioUnit_GetVolume(AudioUnit au, float *left, float *right)
{
    OSStatus err = noErr;
    
    err = AudioUnitGetParameter(au, kHALOutputParam_Volume, kAudioUnitParameterFlag_Output, 0, left);
    if (err != noErr)
    {
        ERR("AudioUnitGetParameter return an error %c%c%c%c\n", (char) (err >> 24), (char) (err >> 16), (char) (err >> 8), (char) err);
        return 0;
    }
    *right = *left;
    return 1;
}
#endif
