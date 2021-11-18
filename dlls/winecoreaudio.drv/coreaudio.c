/*
 * Unixlib for winecoreaudio driver.
 *
 * Copyright 2011 Andrew Eikum for CodeWeavers
 * Copyright 2021 Huw Davies
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

#define LoadResource __carbon_LoadResource
#define CompareString __carbon_CompareString
#define GetCurrentThread __carbon_GetCurrentThread
#define GetCurrentProcess __carbon_GetCurrentProcess

#include <stdarg.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <fenv.h>
#include <unistd.h>

#include <libkern/OSAtomic.h>
#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioFormat.h>
#include <AudioToolbox/AudioConverter.h>
#include <AudioUnit/AudioUnit.h>

#undef LoadResource
#undef CompareString
#undef GetCurrentThread
#undef GetCurrentProcess
#undef _CDECL

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "mmdeviceapi.h"
#include "initguid.h"
#include "audioclient.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/unixlib.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(coreaudio);

static HRESULT osstatus_to_hresult(OSStatus sc)
{
    switch(sc){
    case kAudioFormatUnsupportedDataFormatError:
    case kAudioFormatUnknownFormatError:
    case kAudioDeviceUnsupportedFormatError:
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    case kAudioHardwareBadDeviceError:
        return AUDCLNT_E_DEVICE_INVALIDATED;
    }
    return E_FAIL;
}

/* copied from kernelbase */
static int muldiv( int a, int b, int c )
{
    LONGLONG ret;

    if (!c) return -1;

    /* We want to deal with a positive divisor to simplify the logic. */
    if (c < 0)
    {
        a = -a;
        c = -c;
    }

    /* If the result is positive, we "add" to round. else, we subtract to round. */
    if ((a < 0 && b < 0) || (a >= 0 && b >= 0))
        ret = (((LONGLONG)a * b) + (c / 2)) / c;
    else
        ret = (((LONGLONG)a * b) - (c / 2)) / c;

    if (ret > 2147483647 || ret < -2147483647) return -1;
    return ret;
}

static AudioObjectPropertyScope get_scope(EDataFlow flow)
{
    return (flow == eRender) ? kAudioDevicePropertyScopeOutput : kAudioDevicePropertyScopeInput;
}

static BOOL device_has_channels(AudioDeviceID device, EDataFlow flow)
{
    AudioObjectPropertyAddress addr;
    AudioBufferList *buffers;
    BOOL ret = FALSE;
    OSStatus sc;
    UInt32 size;
    int i;

    addr.mSelector = kAudioDevicePropertyStreamConfiguration;
    addr.mScope = get_scope(flow);
    addr.mElement = 0;

    sc = AudioObjectGetPropertyDataSize(device, &addr, 0, NULL, &size);
    if(sc != noErr){
        WARN("Unable to get _StreamConfiguration property size for device %u: %x\n",
             (unsigned int)device, (int)sc);
        return FALSE;
    }

    buffers = malloc(size);
    if(!buffers) return FALSE;

    sc = AudioObjectGetPropertyData(device, &addr, 0, NULL, &size, buffers);
    if(sc != noErr){
        WARN("Unable to get _StreamConfiguration property for device %u: %x\n",
             (unsigned int)device, (int)sc);
        free(buffers);
        return FALSE;
    }

    for(i = 0; i < buffers->mNumberBuffers; i++){
        if(buffers->mBuffers[i].mNumberChannels > 0){
            ret = TRUE;
            break;
        }
    }
    free(buffers);
    return ret;
}

static NTSTATUS get_endpoint_ids(void *args)
{
    struct get_endpoint_ids_params *params = args;
    unsigned int num_devices, i, needed;
    AudioDeviceID *devices, default_id;
    AudioObjectPropertyAddress addr;
    struct endpoint *endpoint;
    UInt32 devsize, size;
    struct endpoint_info
    {
        CFStringRef name;
        AudioDeviceID id;
    } *info;
    OSStatus sc;
    WCHAR *ptr;

    params->num = 0;
    params->default_idx = 0;

    addr.mScope = kAudioObjectPropertyScopeGlobal;
    addr.mElement = kAudioObjectPropertyElementMaster;
    if(params->flow == eRender) addr.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
    else if(params->flow == eCapture) addr.mSelector = kAudioHardwarePropertyDefaultInputDevice;
    else{
        params->result = E_INVALIDARG;
        return STATUS_SUCCESS;
    }

    size = sizeof(default_id);
    sc = AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr, 0, NULL, &size, &default_id);
    if(sc != noErr){
        WARN("Getting _DefaultInputDevice property failed: %x\n", (int)sc);
        default_id = -1;
    }

    addr.mSelector = kAudioHardwarePropertyDevices;
    sc = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &addr, 0, NULL, &devsize);
    if(sc != noErr){
        WARN("Getting _Devices property size failed: %x\n", (int)sc);
        params->result = osstatus_to_hresult(sc);
        return STATUS_SUCCESS;
    }

    num_devices = devsize / sizeof(AudioDeviceID);
    devices = malloc(devsize);
    info = malloc(num_devices * sizeof(*info));
    if(!devices || !info){
        free(info);
        free(devices);
        params->result = E_OUTOFMEMORY;
        return STATUS_SUCCESS;
    }

    sc = AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr, 0, NULL, &devsize, devices);
    if(sc != noErr){
        WARN("Getting _Devices property failed: %x\n", (int)sc);
        free(info);
        free(devices);
        params->result = osstatus_to_hresult(sc);
        return STATUS_SUCCESS;
    }

    addr.mSelector = kAudioObjectPropertyName;
    addr.mScope = get_scope(params->flow);
    addr.mElement = 0;

    for(i = 0; i < num_devices; i++){
        if(!device_has_channels(devices[i], params->flow)) continue;

        size = sizeof(CFStringRef);
        sc = AudioObjectGetPropertyData(devices[i], &addr, 0, NULL, &size, &info[params->num].name);
        if(sc != noErr){
            WARN("Unable to get _Name property for device %u: %x\n",
                 (unsigned int)devices[i], (int)sc);
            continue;
        }
        info[params->num++].id = devices[i];
    }
    free(devices);

    needed = sizeof(*endpoint) * params->num;
    endpoint = params->endpoints;
    ptr = (WCHAR *)(endpoint + params->num);

    for(i = 0; i < params->num; i++){
        SIZE_T len = CFStringGetLength(info[i].name);
        needed += (len + 1) * sizeof(WCHAR);

        if(needed <= params->size){
            endpoint->name = ptr;
            CFStringGetCharacters(info[i].name, CFRangeMake(0, len), (UniChar*)endpoint->name);
            ptr[len] = 0;
            endpoint->id = info[i].id;
            endpoint++;
            ptr += len + 1;
        }
        CFRelease(info[i].name);
        if(info[i].id == default_id) params->default_idx = i;
    }
    free(info);

    if(needed > params->size){
        params->size = needed;
        params->result = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }
    else params->result = S_OK;

    return STATUS_SUCCESS;
}

static WAVEFORMATEX *clone_format(const WAVEFORMATEX *fmt)
{
    WAVEFORMATEX *ret;
    size_t size;

    if(fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        size = sizeof(WAVEFORMATEXTENSIBLE);
    else
        size = sizeof(WAVEFORMATEX);

    ret = malloc(size);
    if(!ret)
        return NULL;

    memcpy(ret, fmt, size);

    ret->cbSize = size - sizeof(WAVEFORMATEX);

    return ret;
}

static void silence_buffer(struct coreaudio_stream *stream, BYTE *buffer, UINT32 frames)
{
    WAVEFORMATEXTENSIBLE *fmtex = (WAVEFORMATEXTENSIBLE*)stream->fmt;
    if((stream->fmt->wFormatTag == WAVE_FORMAT_PCM ||
        (stream->fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
         IsEqualGUID(&fmtex->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM))) &&
       stream->fmt->wBitsPerSample == 8)
        memset(buffer, 128, frames * stream->fmt->nBlockAlign);
    else
        memset(buffer, 0, frames * stream->fmt->nBlockAlign);
}

/* CA is pulling data from us */
static OSStatus ca_render_cb(void *user, AudioUnitRenderActionFlags *flags,
        const AudioTimeStamp *ts, UInt32 bus, UInt32 nframes,
        AudioBufferList *data)
{
    struct coreaudio_stream *stream = user;
    UINT32 to_copy_bytes, to_copy_frames, chunk_bytes, lcl_offs_bytes;

    OSSpinLockLock(&stream->lock);

    if(stream->playing){
        lcl_offs_bytes = stream->lcl_offs_frames * stream->fmt->nBlockAlign;
        to_copy_frames = min(nframes, stream->held_frames);
        to_copy_bytes = to_copy_frames * stream->fmt->nBlockAlign;

        chunk_bytes = (stream->bufsize_frames - stream->lcl_offs_frames) * stream->fmt->nBlockAlign;

        if(to_copy_bytes > chunk_bytes){
            memcpy(data->mBuffers[0].mData, stream->local_buffer + lcl_offs_bytes, chunk_bytes);
            memcpy(((BYTE *)data->mBuffers[0].mData) + chunk_bytes, stream->local_buffer, to_copy_bytes - chunk_bytes);
        }else
            memcpy(data->mBuffers[0].mData, stream->local_buffer + lcl_offs_bytes, to_copy_bytes);

        stream->lcl_offs_frames += to_copy_frames;
        stream->lcl_offs_frames %= stream->bufsize_frames;
        stream->held_frames -= to_copy_frames;
    }else
        to_copy_bytes = to_copy_frames = 0;

    if(nframes > to_copy_frames)
        silence_buffer(stream, ((BYTE *)data->mBuffers[0].mData) + to_copy_bytes, nframes - to_copy_frames);

    OSSpinLockUnlock(&stream->lock);

    return noErr;
}

static void ca_wrap_buffer(BYTE *dst, UINT32 dst_offs, UINT32 dst_bytes,
                           BYTE *src, UINT32 src_bytes)
{
    UINT32 chunk_bytes = dst_bytes - dst_offs;

    if(chunk_bytes < src_bytes){
        memcpy(dst + dst_offs, src, chunk_bytes);
        memcpy(dst, src + chunk_bytes, src_bytes - chunk_bytes);
    }else
        memcpy(dst + dst_offs, src, src_bytes);
}

/* we need to trigger CA to pull data from the device and give it to us
 *
 * raw data from CA is stored in cap_buffer, possibly via wrap_buffer
 *
 * raw data is resampled from cap_buffer into resamp_buffer in period-size
 * chunks and copied to local_buffer
 */
static OSStatus ca_capture_cb(void *user, AudioUnitRenderActionFlags *flags,
                              const AudioTimeStamp *ts, UInt32 bus, UInt32 nframes,
                              AudioBufferList *data)
{
    struct coreaudio_stream *stream = user;
    AudioBufferList list;
    OSStatus sc;
    UINT32 cap_wri_offs_frames;

    OSSpinLockLock(&stream->lock);

    cap_wri_offs_frames = (stream->cap_offs_frames + stream->cap_held_frames) % stream->cap_bufsize_frames;

    list.mNumberBuffers = 1;
    list.mBuffers[0].mNumberChannels = stream->fmt->nChannels;
    list.mBuffers[0].mDataByteSize = nframes * stream->fmt->nBlockAlign;

    if(!stream->playing || cap_wri_offs_frames + nframes > stream->cap_bufsize_frames){
        if(stream->wrap_bufsize_frames < nframes){
            free(stream->wrap_buffer);
            stream->wrap_buffer = malloc(list.mBuffers[0].mDataByteSize);
            stream->wrap_bufsize_frames = nframes;
        }

        list.mBuffers[0].mData = stream->wrap_buffer;
    }else
        list.mBuffers[0].mData = stream->cap_buffer + cap_wri_offs_frames * stream->fmt->nBlockAlign;

    sc = AudioUnitRender(stream->unit, flags, ts, bus, nframes, &list);
    if(sc != noErr){
        OSSpinLockUnlock(&stream->lock);
        return sc;
    }

    if(stream->playing){
        if(list.mBuffers[0].mData == stream->wrap_buffer){
            ca_wrap_buffer(stream->cap_buffer,
                    cap_wri_offs_frames * stream->fmt->nBlockAlign,
                    stream->cap_bufsize_frames * stream->fmt->nBlockAlign,
                    stream->wrap_buffer, list.mBuffers[0].mDataByteSize);
        }

        stream->cap_held_frames += list.mBuffers[0].mDataByteSize / stream->fmt->nBlockAlign;
        if(stream->cap_held_frames > stream->cap_bufsize_frames){
            stream->cap_offs_frames += stream->cap_held_frames % stream->cap_bufsize_frames;
            stream->cap_offs_frames %= stream->cap_bufsize_frames;
            stream->cap_held_frames = stream->cap_bufsize_frames;
        }
    }

    OSSpinLockUnlock(&stream->lock);
    return noErr;
}

static AudioComponentInstance get_audiounit(EDataFlow dataflow, AudioDeviceID adevid)
{
    AudioComponentInstance unit;
    AudioComponent comp;
    AudioComponentDescription desc;
    OSStatus sc;

    memset(&desc, 0, sizeof(desc));
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;

    if(!(comp = AudioComponentFindNext(NULL, &desc))){
        WARN("AudioComponentFindNext failed\n");
        return NULL;
    }

    sc = AudioComponentInstanceNew(comp, &unit);
    if(sc != noErr){
        WARN("AudioComponentInstanceNew failed: %x\n", (int)sc);
        return NULL;
    }

    if(dataflow == eCapture){
        UInt32 enableio;

        enableio = 1;
        sc = AudioUnitSetProperty(unit, kAudioOutputUnitProperty_EnableIO,
                kAudioUnitScope_Input, 1, &enableio, sizeof(enableio));
        if(sc != noErr){
            WARN("Couldn't enable I/O on input element: %x\n", (int)sc);
            AudioComponentInstanceDispose(unit);
            return NULL;
        }

        enableio = 0;
        sc = AudioUnitSetProperty(unit, kAudioOutputUnitProperty_EnableIO,
                kAudioUnitScope_Output, 0, &enableio, sizeof(enableio));
        if(sc != noErr){
            WARN("Couldn't disable I/O on output element: %x\n", (int)sc);
            AudioComponentInstanceDispose(unit);
            return NULL;
        }
    }

    sc = AudioUnitSetProperty(unit, kAudioOutputUnitProperty_CurrentDevice,
            kAudioUnitScope_Global, 0, &adevid, sizeof(adevid));
    if(sc != noErr){
        WARN("Couldn't set audio unit device\n");
        AudioComponentInstanceDispose(unit);
        return NULL;
    }

    return unit;
}

static void dump_adesc(const char *aux, AudioStreamBasicDescription *desc)
{
    TRACE("%s: mSampleRate: %f\n", aux, desc->mSampleRate);
    TRACE("%s: mBytesPerPacket: %u\n", aux, (unsigned int)desc->mBytesPerPacket);
    TRACE("%s: mFramesPerPacket: %u\n", aux, (unsigned int)desc->mFramesPerPacket);
    TRACE("%s: mBytesPerFrame: %u\n", aux, (unsigned int)desc->mBytesPerFrame);
    TRACE("%s: mChannelsPerFrame: %u\n", aux, (unsigned int)desc->mChannelsPerFrame);
    TRACE("%s: mBitsPerChannel: %u\n", aux, (unsigned int)desc->mBitsPerChannel);
}

static HRESULT ca_get_audiodesc(AudioStreamBasicDescription *desc,
                                const WAVEFORMATEX *fmt)
{
    const WAVEFORMATEXTENSIBLE *fmtex = (const WAVEFORMATEXTENSIBLE *)fmt;

    desc->mFormatFlags = 0;

    if(fmt->wFormatTag == WAVE_FORMAT_PCM ||
            (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&fmtex->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM))){
        desc->mFormatID = kAudioFormatLinearPCM;
        if(fmt->wBitsPerSample > 8)
            desc->mFormatFlags = kAudioFormatFlagIsSignedInteger;
    }else if(fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
            (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&fmtex->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))){
        desc->mFormatID = kAudioFormatLinearPCM;
        desc->mFormatFlags = kAudioFormatFlagIsFloat;
    }else if(fmt->wFormatTag == WAVE_FORMAT_MULAW ||
            (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&fmtex->SubFormat, &KSDATAFORMAT_SUBTYPE_MULAW))){
        desc->mFormatID = kAudioFormatULaw;
    }else if(fmt->wFormatTag == WAVE_FORMAT_ALAW ||
            (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&fmtex->SubFormat, &KSDATAFORMAT_SUBTYPE_ALAW))){
        desc->mFormatID = kAudioFormatALaw;
    }else
        return AUDCLNT_E_UNSUPPORTED_FORMAT;

    desc->mSampleRate = fmt->nSamplesPerSec;
    desc->mBytesPerPacket = fmt->nBlockAlign;
    desc->mFramesPerPacket = 1;
    desc->mBytesPerFrame = fmt->nBlockAlign;
    desc->mChannelsPerFrame = fmt->nChannels;
    desc->mBitsPerChannel = fmt->wBitsPerSample;
    desc->mReserved = 0;

    return S_OK;
}

static HRESULT ca_setup_audiounit(EDataFlow dataflow, AudioComponentInstance unit,
                                  const WAVEFORMATEX *fmt, AudioStreamBasicDescription *dev_desc,
                                  AudioConverterRef *converter)
{
    OSStatus sc;
    HRESULT hr;

    if(dataflow == eCapture){
        AudioStreamBasicDescription desc;
        UInt32 size;
        Float64 rate;
        fenv_t fenv;
        BOOL fenv_stored = TRUE;

        hr = ca_get_audiodesc(&desc, fmt);
        if(FAILED(hr))
            return hr;
        dump_adesc("requested", &desc);

        /* input-only units can't perform sample rate conversion, so we have to
         * set up our own AudioConverter to support arbitrary sample rates. */
        size = sizeof(*dev_desc);
        sc = AudioUnitGetProperty(unit, kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input, 1, dev_desc, &size);
        if(sc != noErr){
            WARN("Couldn't get unit format: %x\n", (int)sc);
            return osstatus_to_hresult(sc);
        }
        dump_adesc("hardware", dev_desc);

        rate = dev_desc->mSampleRate;
        *dev_desc = desc;
        dev_desc->mSampleRate = rate;

        dump_adesc("final", dev_desc);
        sc = AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Output, 1, dev_desc, sizeof(*dev_desc));
        if(sc != noErr){
            WARN("Couldn't set unit format: %x\n", (int)sc);
            return osstatus_to_hresult(sc);
        }

        /* AudioConverterNew requires divide-by-zero SSE exceptions to be masked */
        if(feholdexcept(&fenv)){
            WARN("Failed to store fenv state\n");
            fenv_stored = FALSE;
        }

        sc = AudioConverterNew(dev_desc, &desc, converter);

        if(fenv_stored && fesetenv(&fenv))
            WARN("Failed to restore fenv state\n");

        if(sc != noErr){
            WARN("Couldn't create audio converter: %x\n", (int)sc);
            return osstatus_to_hresult(sc);
        }
    }else{
        hr = ca_get_audiodesc(dev_desc, fmt);
        if(FAILED(hr))
            return hr;

        dump_adesc("final", dev_desc);
        sc = AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input, 0, dev_desc, sizeof(*dev_desc));
        if(sc != noErr){
            WARN("Couldn't set format: %x\n", (int)sc);
            return osstatus_to_hresult(sc);
        }
    }

    return S_OK;
}

static NTSTATUS create_stream(void *args)
{
    struct create_stream_params *params = args;
    struct coreaudio_stream *stream = calloc(1, sizeof(*stream));
    AURenderCallbackStruct input;
    OSStatus sc;

    if(!stream){
        params->result = E_OUTOFMEMORY;
        return STATUS_SUCCESS;
    }

    stream->fmt = clone_format(params->fmt);
    if(!stream->fmt){
        params->result = E_OUTOFMEMORY;
        goto end;
    }

    stream->period_ms = params->period / 10000;
    stream->period_frames = muldiv(params->period, stream->fmt->nSamplesPerSec, 10000000);
    stream->dev_id = params->dev_id;
    stream->flow = params->flow;
    stream->share = params->share;

    stream->bufsize_frames = muldiv(params->duration, stream->fmt->nSamplesPerSec, 10000000);
    if(params->share == AUDCLNT_SHAREMODE_EXCLUSIVE)
        stream->bufsize_frames -= stream->bufsize_frames % stream->period_frames;

    if(!(stream->unit = get_audiounit(stream->flow, stream->dev_id))){
        params->result = AUDCLNT_E_DEVICE_INVALIDATED;
        goto end;
    }

    params->result = ca_setup_audiounit(stream->flow, stream->unit, stream->fmt, &stream->dev_desc, &stream->converter);
    if(FAILED(params->result)) goto end;

    input.inputProcRefCon = stream;
    if(stream->flow == eCapture){
        input.inputProc = ca_capture_cb;
        sc = AudioUnitSetProperty(stream->unit, kAudioOutputUnitProperty_SetInputCallback,
                                  kAudioUnitScope_Output, 1, &input, sizeof(input));
    }else{
        input.inputProc = ca_render_cb;
        sc = AudioUnitSetProperty(stream->unit, kAudioUnitProperty_SetRenderCallback,
                                  kAudioUnitScope_Input, 0, &input, sizeof(input));
    }
    if(sc != noErr){
        WARN("Couldn't set callback: %x\n", (int)sc);
        params->result = osstatus_to_hresult(sc);
        goto end;
    }

    sc = AudioUnitInitialize(stream->unit);
    if(sc != noErr){
        WARN("Couldn't initialize: %x\n", (int)sc);
        params->result = osstatus_to_hresult(sc);
        goto end;
    }

    /* we play audio continuously because AudioOutputUnitStart sometimes takes
     * a while to return */
    sc = AudioOutputUnitStart(stream->unit);
    if(sc != noErr){
        WARN("Unit failed to start: %x\n", (int)sc);
        params->result = osstatus_to_hresult(sc);
        goto end;
    }

    stream->local_buffer_size = stream->bufsize_frames * stream->fmt->nBlockAlign;
    if(NtAllocateVirtualMemory(GetCurrentProcess(), (void **)&stream->local_buffer, 0, &stream->local_buffer_size,
                               MEM_COMMIT, PAGE_READWRITE)){
        params->result = E_OUTOFMEMORY;
        goto end;
    }
    silence_buffer(stream, stream->local_buffer, stream->bufsize_frames);

    if(stream->flow == eCapture){
        stream->cap_bufsize_frames = muldiv(params->duration, stream->dev_desc.mSampleRate, 10000000);
        stream->cap_buffer = malloc(stream->cap_bufsize_frames * stream->fmt->nBlockAlign);
    }
    params->result = S_OK;

end:
    if(FAILED(params->result)){
        if(stream->converter) AudioConverterDispose(stream->converter);
        if(stream->unit) AudioComponentInstanceDispose(stream->unit);
        free(stream->fmt);
        free(stream);
    } else
        params->stream = stream;

    return STATUS_SUCCESS;
}

static NTSTATUS release_stream( void *args )
{
    struct release_stream_params *params = args;
    struct coreaudio_stream *stream = params->stream;

    if(stream->unit){
        AudioOutputUnitStop(stream->unit);
        AudioComponentInstanceDispose(stream->unit);
    }

    if(stream->converter) AudioConverterDispose(stream->converter);
    free(stream->wrap_buffer);
    free(stream->cap_buffer);
    if(stream->local_buffer)
        NtFreeVirtualMemory(GetCurrentProcess(), (void **)&stream->local_buffer,
                            &stream->local_buffer_size, MEM_RELEASE);
    free(stream->fmt);
    params->result = S_OK;
    return STATUS_SUCCESS;
}

unixlib_entry_t __wine_unix_call_funcs[] =
{
    get_endpoint_ids,
    create_stream,
    release_stream,
};
