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

unixlib_entry_t __wine_unix_call_funcs[] =
{
    get_endpoint_ids,
};
