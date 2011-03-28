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

#define ULONG CoreFoundation_ULONG
#define HRESULT CoreFoundation_HRESULT
#ifndef HAVE_AUDIOUNIT_AUDIOCOMPONENT_H
#include <CoreServices/CoreServices.h>
#endif
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#undef ULONG
#undef HRESULT

#undef DPRINTF
#undef STDMETHODCALLTYPE
#include "coreaudio.h"
#include "wine/debug.h"

#ifndef HAVE_AUDIOUNIT_AUDIOCOMPONENT_H
/* Define new AudioComponent Manager functions for compatibility's sake */
typedef Component AudioComponent;
typedef ComponentDescription AudioComponentDescription;
typedef ComponentInstance AudioComponentInstance;

static inline AudioComponent AudioComponentFindNext(AudioComponent ac, AudioComponentDescription *desc)
{
    return FindNextComponent(ac, desc);
}

static inline OSStatus AudioComponentInstanceNew(AudioComponent ac, AudioComponentInstance *aci)
{
    return OpenAComponent(ac, aci);
}

static inline OSStatus AudioComponentInstanceDispose(AudioComponentInstance aci)
{
    return CloseComponent(aci);
}
#endif

#ifndef HAVE_AUGRAPHADDNODE
static inline OSStatus AUGraphAddNode(AUGraph graph, const AudioComponentDescription *desc, AUNode *node)
{
    return AUGraphNewNode(graph, desc, 0, NULL, node);
}

static inline OSStatus AUGraphNodeInfo(AUGraph graph, AUNode node, AudioComponentDescription *desc, AudioUnit *au)
{
    return AUGraphGetNodeInfo(graph, node, desc, 0, NULL, au);
}
#endif

WINE_DEFAULT_DEBUG_CHANNEL(wave);
WINE_DECLARE_DEBUG_CHANNEL(midi);

static const char *streamDescription(const AudioStreamBasicDescription* stream)
{
    return wine_dbg_sprintf("\n mSampleRate : %f\n mFormatID : %s\n mFormatFlags : %lX\n mBytesPerPacket : %lu\n mFramesPerPacket : %lu\n mBytesPerFrame : %lu\n mChannelsPerFrame : %lu\n mBitsPerChannel : %lu\n",
        stream->mSampleRate,
        wine_dbgstr_fourcc(stream->mFormatID),
        stream->mFormatFlags,
        stream->mBytesPerPacket,
        stream->mFramesPerPacket,
        stream->mBytesPerFrame,
        stream->mChannelsPerFrame,
        stream->mBitsPerChannel);
}

extern OSStatus CoreAudio_woAudioUnitIOProc(void *inRefCon, 
				AudioUnitRenderActionFlags *ioActionFlags, 
				const AudioTimeStamp *inTimeStamp, 
				UInt32 inBusNumber, 
				UInt32 inNumberFrames, 
				AudioBufferList *ioData);

extern OSStatus CoreAudio_wiAudioUnitIOProc(void *inRefCon,
				AudioUnitRenderActionFlags *ioActionFlags,
				const AudioTimeStamp *inTimeStamp,
				UInt32 inBusNumber,
				UInt32 inNumberFrames,
				AudioBufferList *ioData);

int AudioUnit_CreateDefaultAudioUnit(void *wwo, AudioUnit *au)
{
    OSStatus err;
    AudioComponent comp;
    AudioComponentDescription desc;
    AURenderCallbackStruct callbackStruct;

    TRACE("\n");
    
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    comp = AudioComponentFindNext(NULL, &desc);
    if (comp == NULL)
        return 0;
    
    err = AudioComponentInstanceNew(comp, au);
    if (err != noErr || *au == NULL)
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
    OSStatus err = AudioComponentInstanceDispose(au);
    return (err == noErr);
}

int AudioUnit_InitializeWithStreamDescription(AudioUnit au, AudioStreamBasicDescription *stream)
{
    OSStatus err = noErr;
        
    TRACE("input format: %s\n", streamDescription(stream));

    err = AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input,
                                0, stream, sizeof(*stream));

    if (err != noErr)
    {
        ERR("AudioUnitSetProperty return an error %s\n", wine_dbgstr_fourcc(err));
        return 0;
    }
    
    err = AudioUnitInitialize(au);
    if (err != noErr)
    {
        ERR("AudioUnitInitialize return an error %s\n", wine_dbgstr_fourcc(err));
        return 0;
    }
    return 1;
}

int AudioUnit_SetVolume(AudioUnit au, float left, float right)
{
    OSStatus err = noErr;
    static int once;

    if (!once++) FIXME("independent left/right volume not implemented (%f, %f)\n", left, right);
   
    err = AudioUnitSetParameter(au, kHALOutputParam_Volume, kAudioUnitParameterFlag_Output, 0, left, 0);
                                
    if (err != noErr)
    {
        ERR("AudioUnitSetParameter return an error %s\n", wine_dbgstr_fourcc(err));
        return 0;
    }
    return 1;
}

int AudioUnit_GetVolume(AudioUnit au, float *left, float *right)
{
    OSStatus err = noErr;
    static int once;

    if (!once++) FIXME("independent left/right volume not implemented\n");
    
    err = AudioUnitGetParameter(au, kHALOutputParam_Volume, kAudioUnitParameterFlag_Output, 0, left);
    if (err != noErr)
    {
        ERR("AudioUnitGetParameter return an error %s\n", wine_dbgstr_fourcc(err));
        return 0;
    }
    *right = *left;
    return 1;
}


/* FIXME: implement sample rate conversion on input */
int AudioUnit_GetInputDeviceSampleRate(void)
{
    AudioDeviceID               defaultInputDevice;
    UInt32                      param;
    AudioObjectPropertyAddress  propertyAddress;
    Float64                     sampleRate;
    OSStatus                    err;

    param = sizeof(defaultInputDevice);
    propertyAddress.mSelector = kAudioHardwarePropertyDefaultInputDevice;
    propertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
    propertyAddress.mElement = kAudioObjectPropertyElementMaster;
    err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &param, &defaultInputDevice);
    if (err != noErr || defaultInputDevice == kAudioDeviceUnknown)
    {
        ERR("Couldn't get the default audio input device ID: %08lx\n", err);
        return 0;
    }

    param = sizeof(sampleRate);
    propertyAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
    propertyAddress.mScope = kAudioDevicePropertyScopeInput;
    err = AudioObjectGetPropertyData(defaultInputDevice, &propertyAddress, 0, NULL, &param, &sampleRate);
    if (err != noErr)
    {
        ERR("Couldn't get the device sample rate: %08lx\n", err);
        return 0;
    }

    return sampleRate;
}


int AudioUnit_CreateInputUnit(void* wwi, AudioUnit* out_au,
        WORD nChannels, DWORD nSamplesPerSec, WORD wBitsPerSample,
        UInt32* outFrameCount)
{
    OSStatus                    err = noErr;
    AudioComponentDescription   description;
    AudioComponent              component;
    AudioUnit                   au;
    UInt32                      param;
    AudioObjectPropertyAddress  propertyAddress;
    AURenderCallbackStruct      callback;
    AudioDeviceID               defaultInputDevice;
    AudioStreamBasicDescription desiredFormat;


    if (!outFrameCount)
    {
        ERR("Invalid parameter\n");
        return 0;
    }

    /* Open the AudioOutputUnit */
    description.componentType           = kAudioUnitType_Output;
    description.componentSubType        = kAudioUnitSubType_HALOutput;
    description.componentManufacturer   = kAudioUnitManufacturer_Apple;
    description.componentFlags          = 0;
    description.componentFlagsMask      = 0;

    component = AudioComponentFindNext(NULL, &description);
    if (!component)
    {
        ERR("AudioComponentFindNext(kAudioUnitSubType_HALOutput) failed\n");
        return 0;
    }

    err = AudioComponentInstanceNew(component, &au);
    if (err != noErr || au == NULL)
    {
        ERR("AudioComponentInstanceNew failed: %08lx\n", err);
        return 0;
    }

    /* Configure the AudioOutputUnit */
    /* The AUHAL has two buses (AKA elements).  Bus 0 is output from the app
     * to the device.  Bus 1 is input from the device to the app.  Each bus
     * has two ends (AKA scopes).  Data goes from the input scope to the
     * output scope.  The terminology is somewhat confusing because the terms
     * "input" and "output" have two meanings.  Here's a summary:
     *
     *      Bus 0, input scope: refers to the source of data to be output as sound
     *      Bus 0, output scope: refers to the actual sound output device
     *      Bus 1, input scope: refers to the actual sound input device
     *      Bus 1, output scope: refers to the destination of data received by the input device
     */

    /* Enable input on the AUHAL */
    param = 1;
    err = AudioUnitSetProperty(au, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1, &param, sizeof(param));
    if (err != noErr)
    {
        ERR("Couldn't enable input on AUHAL: %08lx\n", err);
        goto error;
    }

    /* Disable Output on the AUHAL */
    param = 0;
    err = AudioUnitSetProperty(au, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, 0, &param, sizeof(param));
    if (err != noErr)
    {
        ERR("Couldn't disable output on AUHAL: %08lx\n", err);
        goto error;
    }

    /* Find the default input device */
    param = sizeof(defaultInputDevice);
    propertyAddress.mSelector = kAudioHardwarePropertyDefaultInputDevice;
    propertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
    propertyAddress.mElement = kAudioObjectPropertyElementMaster;
    err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &param, &defaultInputDevice);
    if (err != noErr || defaultInputDevice == kAudioDeviceUnknown)
    {
        ERR("Couldn't get the default audio device ID: %08lx\n", err);
        goto error;
    }

    /* Set the current device to the default input device. */
    err = AudioUnitSetProperty(au, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &defaultInputDevice, sizeof(defaultInputDevice));
    if (err != noErr)
    {
        ERR("Couldn't set current device of AUHAL to default input device: %08lx\n", err);
        goto error;
    }

    /* Setup render callback */
    /* This will be called when the AUHAL has input data.  However, it won't
     * be passed the data itself.  The callback will have to all AudioUnitRender. */
    callback.inputProc = CoreAudio_wiAudioUnitIOProc;
    callback.inputProcRefCon = wwi;
    err = AudioUnitSetProperty(au, kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, 0, &callback, sizeof(callback));
    if (err != noErr)
    {
        ERR("Couldn't set input callback of AUHAL: %08lx\n", err);
        goto error;
    }

    /* Setup the desired data format. */
    /* FIXME: implement sample rate conversion on input.  We shouldn't set
     * the mSampleRate of this to the desired sample rate.  We need to query
     * the input device and use that.  If they don't match, we need to set up
     * an AUConverter to do the sample rate conversion on a separate thread. */
    desiredFormat.mFormatID         = kAudioFormatLinearPCM;
    desiredFormat.mFormatFlags      = kLinearPCMFormatFlagIsPacked;
    if (wBitsPerSample != 8)
        desiredFormat.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
    desiredFormat.mSampleRate       = nSamplesPerSec;
    desiredFormat.mChannelsPerFrame = nChannels;
    desiredFormat.mFramesPerPacket  = 1;
    desiredFormat.mBitsPerChannel   = wBitsPerSample;
    desiredFormat.mBytesPerFrame    = desiredFormat.mBitsPerChannel * desiredFormat.mChannelsPerFrame / 8;
    desiredFormat.mBytesPerPacket   = desiredFormat.mBytesPerFrame * desiredFormat.mFramesPerPacket;

    /* Set the AudioOutputUnit output data format */
    err = AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &desiredFormat, sizeof(desiredFormat));
    if (err != noErr)
    {
        ERR("Couldn't set desired input format of AUHAL: %08lx\n", err);
        goto error;
    }

    /* Get the number of frames in the IO buffer(s) */
    param = sizeof(*outFrameCount);
    err = AudioUnitGetProperty(au, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Global, 0, outFrameCount, &param);
    if (err != noErr)
    {
        ERR("Failed to get audio sample size: %08lx\n", err);
        goto error;
    }

    TRACE("Frame count: %lu\n", *outFrameCount);

    /* Initialize the AU */
    err = AudioUnitInitialize(au);
    if (err != noErr)
    {
        ERR("Failed to initialize AU: %08lx\n", err);
        goto error;
    }

    *out_au = au;

    return 1;

error:
    if (au)
        AudioComponentInstanceDispose(au);
    return 0;
}

/*
 *  MIDI Synth Unit
 */
int SynthUnit_CreateDefaultSynthUnit(AUGraph *graph, AudioUnit *synth)
{
    OSStatus err;
    AudioComponentDescription desc;
    AUNode synthNode;
    AUNode outNode;

    err = NewAUGraph(graph);
    if (err != noErr)
    {
        ERR_(midi)("NewAUGraph return %s\n", wine_dbgstr_fourcc(err));
        return 0;
    }

    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    /* create synth node */
    desc.componentType = kAudioUnitType_MusicDevice;
    desc.componentSubType = kAudioUnitSubType_DLSSynth;

    err = AUGraphAddNode(*graph, &desc, &synthNode);
    if (err != noErr)
    {
        ERR_(midi)("AUGraphAddNode cannot create synthNode : %s\n", wine_dbgstr_fourcc(err));
        return 0;
    }

    /* create out node */
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;

    err = AUGraphAddNode(*graph, &desc, &outNode);
    if (err != noErr)
    {
        ERR_(midi)("AUGraphAddNode cannot create outNode %s\n", wine_dbgstr_fourcc(err));
        return 0;
    }

    err = AUGraphOpen(*graph);
    if (err != noErr)
    {
        ERR_(midi)("AUGraphOpen return %s\n", wine_dbgstr_fourcc(err));
        return 0;
    }

    /* connecting the nodes */
    err = AUGraphConnectNodeInput(*graph, synthNode, 0, outNode, 0);
    if (err != noErr)
    {
        ERR_(midi)("AUGraphConnectNodeInput cannot connect synthNode to outNode : %s\n", wine_dbgstr_fourcc(err));
        return 0;
    }

    /* Get the synth unit */
    err = AUGraphNodeInfo(*graph, synthNode, 0, synth);
    if (err != noErr)
    {
        ERR_(midi)("AUGraphNodeInfo return %s\n", wine_dbgstr_fourcc(err));
        return 0;
    }

    return 1;
}

int SynthUnit_Initialize(AudioUnit synth, AUGraph graph)
{
    OSStatus err = noErr;

    err = AUGraphInitialize(graph);
    if (err != noErr)
    {
        ERR_(midi)("AUGraphInitialize(%p) return %s\n", graph, wine_dbgstr_fourcc(err));
        return 0;
    }

    err = AUGraphStart(graph);
    if (err != noErr)
    {
        ERR_(midi)("AUGraphStart(%p) return %s\n", graph, wine_dbgstr_fourcc(err));
        return 0;
    }

    return 1;
}

int SynthUnit_Close(AUGraph graph)
{
    OSStatus err = noErr;

    err = AUGraphStop(graph);
    if (err != noErr)
    {
        ERR_(midi)("AUGraphStop(%p) return %s\n", graph, wine_dbgstr_fourcc(err));
        return 0;
    }

    err = DisposeAUGraph(graph);
    if (err != noErr)
    {
        ERR_(midi)("DisposeAUGraph(%p) return %s\n", graph, wine_dbgstr_fourcc(err));
        return 0;
    }

    return 1;
}
