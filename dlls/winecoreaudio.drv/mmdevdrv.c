/*
 * Copyright 2011 Andrew Eikum for CodeWeavers
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

#define NONAMELESSUNION
#define COBJMACROS
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

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"

#include "ole2.h"
#include "mmdeviceapi.h"
#include "devpkey.h"
#include "dshow.h"
#include "dsound.h"

#include "initguid.h"
#include "endpointvolume.h"
#include "audioclient.h"
#include "audiopolicy.h"

WINE_DEFAULT_DEBUG_CHANNEL(coreaudio);

#ifndef HAVE_AUDIOUNIT_AUDIOCOMPONENT_H
/* Define new AudioComponent Manager functions for OSX 10.5 */
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

#define NULL_PTR_ERR MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER)

static const REFERENCE_TIME DefaultPeriod = 100000;
static const REFERENCE_TIME MinimumPeriod = 50000;

struct ACImpl;
typedef struct ACImpl ACImpl;

typedef struct _AudioSession {
    GUID guid;
    struct list clients;

    IMMDevice *device;

    float master_vol;
    UINT32 channel_count;
    float *channel_vols;
    BOOL mute;

    CRITICAL_SECTION lock;

    struct list entry;
} AudioSession;

typedef struct _AudioSessionWrapper {
    IAudioSessionControl2 IAudioSessionControl2_iface;
    IChannelAudioVolume IChannelAudioVolume_iface;
    ISimpleAudioVolume ISimpleAudioVolume_iface;

    LONG ref;

    ACImpl *client;
    AudioSession *session;
} AudioSessionWrapper;

struct ACImpl {
    IAudioClient IAudioClient_iface;
    IAudioRenderClient IAudioRenderClient_iface;
    IAudioCaptureClient IAudioCaptureClient_iface;
    IAudioClock IAudioClock_iface;
    IAudioClock2 IAudioClock2_iface;
    IAudioStreamVolume IAudioStreamVolume_iface;

    LONG ref;

    IMMDevice *parent;
    IUnknown *pUnkFTMarshal;

    WAVEFORMATEX *fmt;

    EDataFlow dataflow;
    DWORD flags;
    AUDCLNT_SHAREMODE share;
    HANDLE event;
    float *vols;

    BOOL initted;
    AudioDeviceID adevid;
    AudioObjectPropertyScope scope;
    AudioConverterRef converter;
    AudioComponentInstance unit;
    AudioStreamBasicDescription dev_desc; /* audio unit format, not necessarily the same as fmt */
    HANDLE timer;
    UINT32 period_ms, bufsize_frames, period_frames;
    UINT64 written_frames;
    UINT32 lcl_offs_frames, wri_offs_frames, held_frames, tmp_buffer_frames;
    UINT32 cap_bufsize_frames, cap_offs_frames, cap_held_frames, wrap_bufsize_frames, resamp_bufsize_frames;
    INT32 getbuf_last;
    BOOL playing;
    BYTE *cap_buffer, *wrap_buffer, *resamp_buffer, *local_buffer, *tmp_buffer;

    AudioSession *session;
    AudioSessionWrapper *session_wrapper;

    struct list entry;

    OSSpinLock lock;
};

static const IAudioClientVtbl AudioClient_Vtbl;
static const IAudioRenderClientVtbl AudioRenderClient_Vtbl;
static const IAudioCaptureClientVtbl AudioCaptureClient_Vtbl;
static const IAudioSessionControl2Vtbl AudioSessionControl2_Vtbl;
static const ISimpleAudioVolumeVtbl SimpleAudioVolume_Vtbl;
static const IAudioClockVtbl AudioClock_Vtbl;
static const IAudioClock2Vtbl AudioClock2_Vtbl;
static const IAudioStreamVolumeVtbl AudioStreamVolume_Vtbl;
static const IChannelAudioVolumeVtbl ChannelAudioVolume_Vtbl;
static const IAudioSessionManager2Vtbl AudioSessionManager2_Vtbl;

typedef struct _SessionMgr {
    IAudioSessionManager2 IAudioSessionManager2_iface;

    LONG ref;

    IMMDevice *device;
} SessionMgr;

static const WCHAR drv_key_devicesW[] = {'S','o','f','t','w','a','r','e','\\',
    'W','i','n','e','\\','D','r','i','v','e','r','s','\\',
    'w','i','n','e','c','o','r','e','a','u','d','i','o','.','d','r','v','\\','d','e','v','i','c','e','s',0};
static const WCHAR guidW[] = {'g','u','i','d',0};

static HANDLE g_timer_q;

static CRITICAL_SECTION g_sessions_lock;
static CRITICAL_SECTION_DEBUG g_sessions_lock_debug =
{
    0, 0, &g_sessions_lock,
    { &g_sessions_lock_debug.ProcessLocksList, &g_sessions_lock_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": g_sessions_lock") }
};
static CRITICAL_SECTION g_sessions_lock = { &g_sessions_lock_debug, -1, 0, 0, 0, 0 };
static struct list g_sessions = LIST_INIT(g_sessions);

static AudioSessionWrapper *AudioSessionWrapper_Create(ACImpl *client);
static HRESULT ca_setvol(ACImpl *This, UINT32 index);

static inline ACImpl *impl_from_IAudioClient(IAudioClient *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioClient_iface);
}

static inline ACImpl *impl_from_IAudioRenderClient(IAudioRenderClient *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioRenderClient_iface);
}

static inline ACImpl *impl_from_IAudioCaptureClient(IAudioCaptureClient *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioCaptureClient_iface);
}

static inline AudioSessionWrapper *impl_from_IAudioSessionControl2(IAudioSessionControl2 *iface)
{
    return CONTAINING_RECORD(iface, AudioSessionWrapper, IAudioSessionControl2_iface);
}

static inline AudioSessionWrapper *impl_from_ISimpleAudioVolume(ISimpleAudioVolume *iface)
{
    return CONTAINING_RECORD(iface, AudioSessionWrapper, ISimpleAudioVolume_iface);
}

static inline AudioSessionWrapper *impl_from_IChannelAudioVolume(IChannelAudioVolume *iface)
{
    return CONTAINING_RECORD(iface, AudioSessionWrapper, IChannelAudioVolume_iface);
}

static inline ACImpl *impl_from_IAudioClock(IAudioClock *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioClock_iface);
}

static inline ACImpl *impl_from_IAudioClock2(IAudioClock2 *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioClock2_iface);
}

static inline ACImpl *impl_from_IAudioStreamVolume(IAudioStreamVolume *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioStreamVolume_iface);
}

static inline SessionMgr *impl_from_IAudioSessionManager2(IAudioSessionManager2 *iface)
{
    return CONTAINING_RECORD(iface, SessionMgr, IAudioSessionManager2_iface);
}

BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, void *reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        g_timer_q = CreateTimerQueue();
        if(!g_timer_q)
            return FALSE;
        break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
        DeleteCriticalSection(&g_sessions_lock);
        break;
    }
    return TRUE;
}

/* From <dlls/mmdevapi/mmdevapi.h> */
enum DriverPriority {
    Priority_Unavailable = 0,
    Priority_Low,
    Priority_Neutral,
    Priority_Preferred
};

int WINAPI AUDDRV_GetPriority(void)
{
    return Priority_Neutral;
}

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

static void set_device_guid(EDataFlow flow, HKEY drv_key, const WCHAR *key_name,
        GUID *guid)
{
    HKEY key;
    BOOL opened = FALSE;
    LONG lr;

    if(!drv_key){
        lr = RegCreateKeyExW(HKEY_CURRENT_USER, drv_key_devicesW, 0, NULL, 0, KEY_WRITE,
                    NULL, &drv_key, NULL);
        if(lr != ERROR_SUCCESS){
            ERR("RegCreateKeyEx(drv_key) failed: %u\n", lr);
            return;
        }
        opened = TRUE;
    }

    lr = RegCreateKeyExW(drv_key, key_name, 0, NULL, 0, KEY_WRITE,
                NULL, &key, NULL);
    if(lr != ERROR_SUCCESS){
        ERR("RegCreateKeyEx(%s) failed: %u\n", wine_dbgstr_w(key_name), lr);
        goto exit;
    }

    lr = RegSetValueExW(key, guidW, 0, REG_BINARY, (BYTE*)guid,
                sizeof(GUID));
    if(lr != ERROR_SUCCESS)
        ERR("RegSetValueEx(%s\\guid) failed: %u\n", wine_dbgstr_w(key_name), lr);

    RegCloseKey(key);
exit:
    if(opened)
        RegCloseKey(drv_key);
}

static void get_device_guid(EDataFlow flow, AudioDeviceID device, GUID *guid)
{
    HKEY key = NULL, dev_key;
    DWORD type, size = sizeof(*guid);
    WCHAR key_name[256];

    static const WCHAR key_fmt[] = {'%','u',0};

    if(flow == eCapture)
        key_name[0] = '1';
    else
        key_name[0] = '0';
    key_name[1] = ',';

    sprintfW(key_name + 2, key_fmt, device);

    if(RegOpenKeyExW(HKEY_CURRENT_USER, drv_key_devicesW, 0, KEY_WRITE|KEY_READ, &key) == ERROR_SUCCESS){
        if(RegOpenKeyExW(key, key_name, 0, KEY_READ, &dev_key) == ERROR_SUCCESS){
            if(RegQueryValueExW(dev_key, guidW, 0, &type,
                        (BYTE*)guid, &size) == ERROR_SUCCESS){
                if(type == REG_BINARY){
                    RegCloseKey(dev_key);
                    RegCloseKey(key);
                    return;
                }
                ERR("Invalid type for device %s GUID: %u; ignoring and overwriting\n",
                        wine_dbgstr_w(key_name), type);
            }
            RegCloseKey(dev_key);
        }
    }

    CoCreateGuid(guid);

    set_device_guid(flow, key, key_name, guid);

    if(key)
        RegCloseKey(key);
}

HRESULT WINAPI AUDDRV_GetEndpointIDs(EDataFlow flow, WCHAR ***ids,
        GUID **guids, UINT *num, UINT *def_index)
{
    UInt32 devsize, size;
    AudioDeviceID *devices;
    AudioDeviceID default_id;
    AudioObjectPropertyAddress addr;
    OSStatus sc;
    int i, ndevices;

    TRACE("%d %p %p %p\n", flow, ids, num, def_index);

    addr.mScope = kAudioObjectPropertyScopeGlobal;
    addr.mElement = kAudioObjectPropertyElementMaster;
    if(flow == eRender)
        addr.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
    else if(flow == eCapture)
        addr.mSelector = kAudioHardwarePropertyDefaultInputDevice;
    else
        return E_INVALIDARG;

    size = sizeof(default_id);
    sc = AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr, 0,
            NULL, &size, &default_id);
    if(sc != noErr){
        WARN("Getting _DefaultInputDevice property failed: %x\n", (int)sc);
        default_id = -1;
    }

    addr.mSelector = kAudioHardwarePropertyDevices;
    sc = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &addr, 0,
            NULL, &devsize);
    if(sc != noErr){
        WARN("Getting _Devices property size failed: %x\n", (int)sc);
        return osstatus_to_hresult(sc);
    }

    devices = HeapAlloc(GetProcessHeap(), 0, devsize);
    if(!devices)
        return E_OUTOFMEMORY;

    sc = AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr, 0, NULL,
            &devsize, devices);
    if(sc != noErr){
        WARN("Getting _Devices property failed: %x\n", (int)sc);
        HeapFree(GetProcessHeap(), 0, devices);
        return osstatus_to_hresult(sc);
    }

    ndevices = devsize / sizeof(AudioDeviceID);

    *ids = HeapAlloc(GetProcessHeap(), 0, ndevices * sizeof(WCHAR *));
    if(!*ids){
        HeapFree(GetProcessHeap(), 0, devices);
        return E_OUTOFMEMORY;
    }

    *guids = HeapAlloc(GetProcessHeap(), 0, ndevices * sizeof(GUID));
    if(!*guids){
        HeapFree(GetProcessHeap(), 0, *ids);
        HeapFree(GetProcessHeap(), 0, devices);
        return E_OUTOFMEMORY;
    }

    *num = 0;
    *def_index = (UINT)-1;
    for(i = 0; i < ndevices; ++i){
        AudioBufferList *buffers;
        CFStringRef name;
        SIZE_T len;
        int j;

        addr.mSelector = kAudioDevicePropertyStreamConfiguration;
        if(flow == eRender)
            addr.mScope = kAudioDevicePropertyScopeOutput;
        else
            addr.mScope = kAudioDevicePropertyScopeInput;
        addr.mElement = 0;
        sc = AudioObjectGetPropertyDataSize(devices[i], &addr, 0, NULL, &size);
        if(sc != noErr){
            WARN("Unable to get _StreamConfiguration property size for "
                    "device %u: %x\n", (unsigned int)devices[i], (int)sc);
            continue;
        }

        buffers = HeapAlloc(GetProcessHeap(), 0, size);
        if(!buffers){
            HeapFree(GetProcessHeap(), 0, devices);
            for(j = 0; j < *num; ++j)
                HeapFree(GetProcessHeap(), 0, (*ids)[j]);
            HeapFree(GetProcessHeap(), 0, *guids);
            HeapFree(GetProcessHeap(), 0, *ids);
            return E_OUTOFMEMORY;
        }

        sc = AudioObjectGetPropertyData(devices[i], &addr, 0, NULL,
                &size, buffers);
        if(sc != noErr){
            WARN("Unable to get _StreamConfiguration property for "
                    "device %u: %x\n", (unsigned int)devices[i], (int)sc);
            HeapFree(GetProcessHeap(), 0, buffers);
            continue;
        }

        /* check that there's at least one channel in this device before
         * we claim it as usable */
        for(j = 0; j < buffers->mNumberBuffers; ++j)
            if(buffers->mBuffers[j].mNumberChannels > 0)
                break;
        if(j >= buffers->mNumberBuffers){
            HeapFree(GetProcessHeap(), 0, buffers);
            continue;
        }

        HeapFree(GetProcessHeap(), 0, buffers);

        size = sizeof(name);
        addr.mSelector = kAudioObjectPropertyName;
        sc = AudioObjectGetPropertyData(devices[i], &addr, 0, NULL,
                &size, &name);
        if(sc != noErr){
            WARN("Unable to get _Name property for device %u: %x\n",
                    (unsigned int)devices[i], (int)sc);
            continue;
        }

        len = CFStringGetLength(name) + 1;
        (*ids)[*num] = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if(!(*ids)[*num]){
            CFRelease(name);
            HeapFree(GetProcessHeap(), 0, devices);
            for(j = 0; j < *num; ++j)
                HeapFree(GetProcessHeap(), 0, (*ids)[j]);
            HeapFree(GetProcessHeap(), 0, *ids);
            HeapFree(GetProcessHeap(), 0, *guids);
            return E_OUTOFMEMORY;
        }
        CFStringGetCharacters(name, CFRangeMake(0, len - 1), (UniChar*)(*ids)[*num]);
        ((*ids)[*num])[len - 1] = 0;
        CFRelease(name);

        get_device_guid(flow, devices[i], &(*guids)[*num]);

        if(*def_index == (UINT)-1 && devices[i] == default_id)
            *def_index = *num;

        TRACE("device %u: id %s key %u%s\n", *num, debugstr_w((*ids)[*num]),
              (unsigned int)devices[i], (*def_index == *num) ? " (default)" : "");

        (*num)++;
    }

    if(*def_index == (UINT)-1)
        *def_index = 0;

    HeapFree(GetProcessHeap(), 0, devices);

    return S_OK;
}

static BOOL get_deviceid_by_guid(GUID *guid, AudioDeviceID *id, EDataFlow *flow)
{
    HKEY devices_key;
    UINT i = 0;
    WCHAR key_name[256];
    DWORD key_name_size;

    if(RegOpenKeyExW(HKEY_CURRENT_USER, drv_key_devicesW, 0, KEY_READ, &devices_key) != ERROR_SUCCESS){
        ERR("No devices in registry?\n");
        return FALSE;
    }

    while(1){
        HKEY key;
        DWORD size, type;
        GUID reg_guid;

        key_name_size = sizeof(key_name);
        if(RegEnumKeyExW(devices_key, i++, key_name, &key_name_size, NULL,
                NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        if(RegOpenKeyExW(devices_key, key_name, 0, KEY_READ, &key) != ERROR_SUCCESS){
            WARN("Couldn't open key: %s\n", wine_dbgstr_w(key_name));
            continue;
        }

        size = sizeof(reg_guid);
        if(RegQueryValueExW(key, guidW, 0, &type,
                    (BYTE*)&reg_guid, &size) == ERROR_SUCCESS){
            if(IsEqualGUID(&reg_guid, guid)){
                RegCloseKey(key);
                RegCloseKey(devices_key);

                TRACE("Found matching device key: %s\n", wine_dbgstr_w(key_name));

                if(key_name[0] == '0')
                    *flow = eRender;
                else if(key_name[0] == '1')
                    *flow = eCapture;
                else{
                    ERR("Unknown device type: %c\n", key_name[0]);
                    return FALSE;
                }

                *id = strtoulW(key_name + 2, NULL, 10);

                return TRUE;
            }
        }

        RegCloseKey(key);
    }

    RegCloseKey(devices_key);

    WARN("No matching device in registry for GUID %s\n", debugstr_guid(guid));

    return FALSE;
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

HRESULT WINAPI AUDDRV_GetAudioEndpoint(GUID *guid, IMMDevice *dev, IAudioClient **out)
{
    ACImpl *This;
    AudioDeviceID adevid;
    EDataFlow dataflow;
    HRESULT hr;

    TRACE("%s %p %p\n", debugstr_guid(guid), dev, out);

    if(!get_deviceid_by_guid(guid, &adevid, &dataflow))
        return AUDCLNT_E_DEVICE_INVALIDATED;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ACImpl));
    if(!This)
        return E_OUTOFMEMORY;

    This->IAudioClient_iface.lpVtbl = &AudioClient_Vtbl;
    This->IAudioRenderClient_iface.lpVtbl = &AudioRenderClient_Vtbl;
    This->IAudioCaptureClient_iface.lpVtbl = &AudioCaptureClient_Vtbl;
    This->IAudioClock_iface.lpVtbl = &AudioClock_Vtbl;
    This->IAudioClock2_iface.lpVtbl = &AudioClock2_Vtbl;
    This->IAudioStreamVolume_iface.lpVtbl = &AudioStreamVolume_Vtbl;

    This->dataflow = dataflow;

    if(dataflow == eRender)
        This->scope = kAudioDevicePropertyScopeOutput;
    else if(dataflow == eCapture)
        This->scope = kAudioDevicePropertyScopeInput;
    else{
        HeapFree(GetProcessHeap(), 0, This);
        return E_INVALIDARG;
    }

    This->lock = 0;

    hr = CoCreateFreeThreadedMarshaler((IUnknown *)&This->IAudioClient_iface, &This->pUnkFTMarshal);
    if (FAILED(hr)) {
        HeapFree(GetProcessHeap(), 0, This);
        return hr;
    }

    This->parent = dev;
    IMMDevice_AddRef(This->parent);

    This->adevid = adevid;

    if(!(This->unit = get_audiounit(This->dataflow, This->adevid))){
        HeapFree(GetProcessHeap(), 0, This);
        return AUDCLNT_E_DEVICE_INVALIDATED;
    }

    *out = &This->IAudioClient_iface;
    IAudioClient_AddRef(&This->IAudioClient_iface);

    return S_OK;
}

static HRESULT WINAPI AudioClient_QueryInterface(IAudioClient *iface,
        REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;
    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IAudioClient))
        *ppv = iface;
    else if(IsEqualIID(riid, &IID_IMarshal))
        return IUnknown_QueryInterface(This->pUnkFTMarshal, riid, ppv);

    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioClient_AddRef(IAudioClient *iface)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    return ref;
}

static ULONG WINAPI AudioClient_Release(IAudioClient *iface)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    if(!ref){
        if(This->timer){
            HANDLE event;
            BOOL wait;
            event = CreateEventW(NULL, TRUE, FALSE, NULL);
            wait = !DeleteTimerQueueTimer(g_timer_q, This->timer, event);
            wait = wait && GetLastError() == ERROR_IO_PENDING;
            if(event && wait)
                WaitForSingleObject(event, INFINITE);
            CloseHandle(event);
        }
        AudioOutputUnitStop(This->unit);
        AudioComponentInstanceDispose(This->unit);
        if(This->converter)
            AudioConverterDispose(This->converter);
        if(This->session){
            EnterCriticalSection(&g_sessions_lock);
            list_remove(&This->entry);
            LeaveCriticalSection(&g_sessions_lock);
        }
        HeapFree(GetProcessHeap(), 0, This->vols);
        HeapFree(GetProcessHeap(), 0, This->tmp_buffer);
        HeapFree(GetProcessHeap(), 0, This->cap_buffer);
        HeapFree(GetProcessHeap(), 0, This->local_buffer);
        free(This->wrap_buffer);
        HeapFree(GetProcessHeap(), 0, This->resamp_buffer);
        CoTaskMemFree(This->fmt);
        IMMDevice_Release(This->parent);
        IUnknown_Release(This->pUnkFTMarshal);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static void dump_fmt(const WAVEFORMATEX *fmt)
{
    TRACE("wFormatTag: 0x%x (", fmt->wFormatTag);
    switch(fmt->wFormatTag){
    case WAVE_FORMAT_PCM:
        TRACE("WAVE_FORMAT_PCM");
        break;
    case WAVE_FORMAT_IEEE_FLOAT:
        TRACE("WAVE_FORMAT_IEEE_FLOAT");
        break;
    case WAVE_FORMAT_EXTENSIBLE:
        TRACE("WAVE_FORMAT_EXTENSIBLE");
        break;
    default:
        TRACE("Unknown");
        break;
    }
    TRACE(")\n");

    TRACE("nChannels: %u\n", fmt->nChannels);
    TRACE("nSamplesPerSec: %u\n", fmt->nSamplesPerSec);
    TRACE("nAvgBytesPerSec: %u\n", fmt->nAvgBytesPerSec);
    TRACE("nBlockAlign: %u\n", fmt->nBlockAlign);
    TRACE("wBitsPerSample: %u\n", fmt->wBitsPerSample);
    TRACE("cbSize: %u\n", fmt->cbSize);

    if(fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE){
        WAVEFORMATEXTENSIBLE *fmtex = (void*)fmt;
        TRACE("dwChannelMask: %08x\n", fmtex->dwChannelMask);
        TRACE("Samples: %04x\n", fmtex->Samples.wReserved);
        TRACE("SubFormat: %s\n", wine_dbgstr_guid(&fmtex->SubFormat));
    }
}

static DWORD get_channel_mask(unsigned int channels)
{
    switch(channels){
    case 0:
        return 0;
    case 1:
        return KSAUDIO_SPEAKER_MONO;
    case 2:
        return KSAUDIO_SPEAKER_STEREO;
    case 3:
        return KSAUDIO_SPEAKER_STEREO | SPEAKER_LOW_FREQUENCY;
    case 4:
        return KSAUDIO_SPEAKER_QUAD;    /* not _SURROUND */
    case 5:
        return KSAUDIO_SPEAKER_QUAD | SPEAKER_LOW_FREQUENCY;
    case 6:
        return KSAUDIO_SPEAKER_5POINT1; /* not 5POINT1_SURROUND */
    case 7:
        return KSAUDIO_SPEAKER_5POINT1 | SPEAKER_BACK_CENTER;
    case 8:
        return KSAUDIO_SPEAKER_7POINT1_SURROUND; /* Vista deprecates 7POINT1 */
    }
    FIXME("Unknown speaker configuration: %u\n", channels);
    return 0;
}

static WAVEFORMATEX *clone_format(const WAVEFORMATEX *fmt)
{
    WAVEFORMATEX *ret;
    size_t size;

    if(fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        size = sizeof(WAVEFORMATEXTENSIBLE);
    else
        size = sizeof(WAVEFORMATEX);

    ret = CoTaskMemAlloc(size);
    if(!ret)
        return NULL;

    memcpy(ret, fmt, size);

    ret->cbSize = size - sizeof(WAVEFORMATEX);

    return ret;
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

static void session_init_vols(AudioSession *session, UINT channels)
{
    if(session->channel_count < channels){
        UINT i;

        if(session->channel_vols)
            session->channel_vols = HeapReAlloc(GetProcessHeap(), 0,
                    session->channel_vols, sizeof(float) * channels);
        else
            session->channel_vols = HeapAlloc(GetProcessHeap(), 0,
                    sizeof(float) * channels);
        if(!session->channel_vols)
            return;

        for(i = session->channel_count; i < channels; ++i)
            session->channel_vols[i] = 1.f;

        session->channel_count = channels;
    }
}

static AudioSession *create_session(const GUID *guid, IMMDevice *device,
        UINT num_channels)
{
    AudioSession *ret;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(AudioSession));
    if(!ret)
        return NULL;

    memcpy(&ret->guid, guid, sizeof(GUID));

    ret->device = device;

    list_init(&ret->clients);

    list_add_head(&g_sessions, &ret->entry);

    InitializeCriticalSection(&ret->lock);
    ret->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": AudioSession.lock");

    session_init_vols(ret, num_channels);

    ret->master_vol = 1.f;

    return ret;
}

/* if channels == 0, then this will return or create a session with
 * matching dataflow and GUID. otherwise, channels must also match */
static HRESULT get_audio_session(const GUID *sessionguid,
        IMMDevice *device, UINT channels, AudioSession **out)
{
    AudioSession *session;

    if(!sessionguid || IsEqualGUID(sessionguid, &GUID_NULL)){
        *out = create_session(&GUID_NULL, device, channels);
        if(!*out)
            return E_OUTOFMEMORY;

        return S_OK;
    }

    *out = NULL;
    LIST_FOR_EACH_ENTRY(session, &g_sessions, AudioSession, entry){
        if(session->device == device &&
                IsEqualGUID(sessionguid, &session->guid)){
            session_init_vols(session, channels);
            *out = session;
            break;
        }
    }

    if(!*out){
        *out = create_session(sessionguid, device, channels);
        if(!*out)
            return E_OUTOFMEMORY;
    }

    return S_OK;
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

static void silence_buffer(ACImpl *This, BYTE *buffer, UINT32 frames)
{
    WAVEFORMATEXTENSIBLE *fmtex = (WAVEFORMATEXTENSIBLE*)This->fmt;
    if((This->fmt->wFormatTag == WAVE_FORMAT_PCM ||
            (This->fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&fmtex->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM))) &&
            This->fmt->wBitsPerSample == 8)
        memset(buffer, 128, frames * This->fmt->nBlockAlign);
    else
        memset(buffer, 0, frames * This->fmt->nBlockAlign);
}

/* CA is pulling data from us */
static OSStatus ca_render_cb(void *user, AudioUnitRenderActionFlags *flags,
        const AudioTimeStamp *ts, UInt32 bus, UInt32 nframes,
        AudioBufferList *data)
{
    ACImpl *This = user;
    UINT32 to_copy_bytes, to_copy_frames, chunk_bytes, lcl_offs_bytes;

    OSSpinLockLock(&This->lock);

    if(This->playing){
        lcl_offs_bytes = This->lcl_offs_frames * This->fmt->nBlockAlign;
        to_copy_frames = min(nframes, This->held_frames);
        to_copy_bytes = to_copy_frames * This->fmt->nBlockAlign;

        chunk_bytes = (This->bufsize_frames - This->lcl_offs_frames) * This->fmt->nBlockAlign;

        if(to_copy_bytes > chunk_bytes){
            memcpy(data->mBuffers[0].mData, This->local_buffer + lcl_offs_bytes, chunk_bytes);
            memcpy(((BYTE *)data->mBuffers[0].mData) + chunk_bytes, This->local_buffer, to_copy_bytes - chunk_bytes);
        }else
            memcpy(data->mBuffers[0].mData, This->local_buffer + lcl_offs_bytes, to_copy_bytes);

        This->lcl_offs_frames += to_copy_frames;
        This->lcl_offs_frames %= This->bufsize_frames;
        This->held_frames -= to_copy_frames;
    }else
        to_copy_bytes = to_copy_frames = 0;

    if(nframes > to_copy_frames)
        silence_buffer(This, ((BYTE *)data->mBuffers[0].mData) + to_copy_bytes, nframes - to_copy_frames);

    OSSpinLockUnlock(&This->lock);

    return noErr;
}

static UINT buf_ptr_diff(UINT left, UINT right, UINT bufsize)
{
    if(left <= right)
        return right - left;
    return bufsize - (left - right);
}

/* place data from cap_buffer into provided AudioBufferList */
static OSStatus feed_cb(AudioConverterRef converter, UInt32 *nframes, AudioBufferList *data,
        AudioStreamPacketDescription **packets, void *user)
{
    ACImpl *This = user;

    *nframes = min(*nframes, This->cap_held_frames);
    if(!*nframes){
        data->mBuffers[0].mData = NULL;
        data->mBuffers[0].mDataByteSize = 0;
        data->mBuffers[0].mNumberChannels = This->fmt->nChannels;
        return noErr;
    }

    data->mBuffers[0].mDataByteSize = *nframes * This->fmt->nBlockAlign;
    data->mBuffers[0].mNumberChannels = This->fmt->nChannels;

    if(This->cap_offs_frames + *nframes > This->cap_bufsize_frames){
        UINT32 chunk_frames = This->cap_bufsize_frames - This->cap_offs_frames;

        if(This->wrap_bufsize_frames < *nframes){
            free(This->wrap_buffer);
            This->wrap_buffer = malloc(data->mBuffers[0].mDataByteSize);
            This->wrap_bufsize_frames = *nframes;
        }

        memcpy(This->wrap_buffer, This->cap_buffer + This->cap_offs_frames * This->fmt->nBlockAlign,
                chunk_frames * This->fmt->nBlockAlign);
        memcpy(This->wrap_buffer + chunk_frames * This->fmt->nBlockAlign, This->cap_buffer,
                (*nframes - chunk_frames) * This->fmt->nBlockAlign);

        data->mBuffers[0].mData = This->wrap_buffer;
    }else
        data->mBuffers[0].mData = This->cap_buffer + This->cap_offs_frames * This->fmt->nBlockAlign;

    This->cap_offs_frames += *nframes;
    This->cap_offs_frames %= This->cap_bufsize_frames;
    This->cap_held_frames -= *nframes;

    if(packets)
        *packets = NULL;

    return noErr;
}

static void capture_resample(ACImpl *This)
{
    UINT32 resamp_period_frames = MulDiv(This->period_frames, This->dev_desc.mSampleRate, This->fmt->nSamplesPerSec);
    OSStatus sc;

    /* the resampling process often needs more source frames than we'd
     * guess from a straight conversion using the sample rate ratio. so
     * only convert if we have extra source data. */
    while(This->cap_held_frames > resamp_period_frames * 2){
        AudioBufferList converted_list;
        UInt32 wanted_frames = This->period_frames;

        converted_list.mNumberBuffers = 1;
        converted_list.mBuffers[0].mNumberChannels = This->fmt->nChannels;
        converted_list.mBuffers[0].mDataByteSize = wanted_frames * This->fmt->nBlockAlign;

        if(This->resamp_bufsize_frames < wanted_frames){
            HeapFree(GetProcessHeap(), 0, This->resamp_buffer);
            This->resamp_buffer = HeapAlloc(GetProcessHeap(), 0, converted_list.mBuffers[0].mDataByteSize);
            This->resamp_bufsize_frames = wanted_frames;
        }

        converted_list.mBuffers[0].mData = This->resamp_buffer;

        sc = AudioConverterFillComplexBuffer(This->converter, feed_cb,
                This, &wanted_frames, &converted_list, NULL);
        if(sc != noErr){
            WARN("AudioConverterFillComplexBuffer failed: %x\n", (int)sc);
            break;
        }

        ca_wrap_buffer(This->local_buffer,
                This->wri_offs_frames * This->fmt->nBlockAlign,
                This->bufsize_frames * This->fmt->nBlockAlign,
                This->resamp_buffer, wanted_frames * This->fmt->nBlockAlign);

        This->wri_offs_frames += wanted_frames;
        This->wri_offs_frames %= This->bufsize_frames;
        if(This->held_frames + wanted_frames > This->bufsize_frames){
            This->lcl_offs_frames += buf_ptr_diff(This->lcl_offs_frames,
                    This->wri_offs_frames, This->bufsize_frames);
            This->held_frames = This->bufsize_frames;
        }else
            This->held_frames += wanted_frames;
    }
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
    ACImpl *This = user;
    AudioBufferList list;
    OSStatus sc;
    UINT32 cap_wri_offs_frames;

    OSSpinLockLock(&This->lock);

    cap_wri_offs_frames = (This->cap_offs_frames + This->cap_held_frames) % This->cap_bufsize_frames;

    list.mNumberBuffers = 1;
    list.mBuffers[0].mNumberChannels = This->fmt->nChannels;
    list.mBuffers[0].mDataByteSize = nframes * This->fmt->nBlockAlign;

    if(!This->playing || cap_wri_offs_frames + nframes > This->cap_bufsize_frames){
        if(This->wrap_bufsize_frames < nframes){
            free(This->wrap_buffer);
            This->wrap_buffer = malloc(list.mBuffers[0].mDataByteSize);
            This->wrap_bufsize_frames = nframes;
        }

        list.mBuffers[0].mData = This->wrap_buffer;
    }else
        list.mBuffers[0].mData = This->cap_buffer + cap_wri_offs_frames * This->fmt->nBlockAlign;

    sc = AudioUnitRender(This->unit, flags, ts, bus, nframes, &list);
    if(sc != noErr){
        OSSpinLockUnlock(&This->lock);
        return sc;
    }

    if(This->playing){
        if(list.mBuffers[0].mData == This->wrap_buffer){
            ca_wrap_buffer(This->cap_buffer,
                    cap_wri_offs_frames * This->fmt->nBlockAlign,
                    This->cap_bufsize_frames * This->fmt->nBlockAlign,
                    This->wrap_buffer, list.mBuffers[0].mDataByteSize);
        }

        This->cap_held_frames += list.mBuffers[0].mDataByteSize / This->fmt->nBlockAlign;
        if(This->cap_held_frames > This->cap_bufsize_frames){
            This->cap_offs_frames += This->cap_held_frames % This->cap_bufsize_frames;
            This->cap_offs_frames %= This->cap_bufsize_frames;
            This->cap_held_frames = This->cap_bufsize_frames;
        }
    }

    OSSpinLockUnlock(&This->lock);
    return noErr;
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

static HRESULT WINAPI AudioClient_Initialize(IAudioClient *iface,
        AUDCLNT_SHAREMODE mode, DWORD flags, REFERENCE_TIME duration,
        REFERENCE_TIME period, const WAVEFORMATEX *fmt,
        const GUID *sessionguid)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    HRESULT hr;
    OSStatus sc;
    int i;

    TRACE("(%p)->(%x, %x, %s, %s, %p, %s)\n", This, mode, flags,
          wine_dbgstr_longlong(duration), wine_dbgstr_longlong(period), fmt, debugstr_guid(sessionguid));

    if(!fmt)
        return E_POINTER;

    dump_fmt(fmt);

    if(mode != AUDCLNT_SHAREMODE_SHARED && mode != AUDCLNT_SHAREMODE_EXCLUSIVE)
        return AUDCLNT_E_NOT_INITIALIZED;

    if(flags & ~(AUDCLNT_STREAMFLAGS_CROSSPROCESS |
                AUDCLNT_STREAMFLAGS_LOOPBACK |
                AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
                AUDCLNT_STREAMFLAGS_NOPERSIST |
                AUDCLNT_STREAMFLAGS_RATEADJUST |
                AUDCLNT_SESSIONFLAGS_EXPIREWHENUNOWNED |
                AUDCLNT_SESSIONFLAGS_DISPLAY_HIDE |
                AUDCLNT_SESSIONFLAGS_DISPLAY_HIDEWHENEXPIRED)){
        TRACE("Unknown flags: %08x\n", flags);
        return E_INVALIDARG;
    }

    if(mode == AUDCLNT_SHAREMODE_SHARED){
        period = DefaultPeriod;
        if( duration < 3 * period)
            duration = 3 * period;
    }else{
        if(fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE){
            if(((WAVEFORMATEXTENSIBLE*)fmt)->dwChannelMask == 0 ||
                    ((WAVEFORMATEXTENSIBLE*)fmt)->dwChannelMask & SPEAKER_RESERVED)
                return AUDCLNT_E_UNSUPPORTED_FORMAT;
        }

        if(!period)
            period = DefaultPeriod; /* not minimum */
        if(period < MinimumPeriod || period > 5000000)
            return AUDCLNT_E_INVALID_DEVICE_PERIOD;
        if(duration > 20000000) /* the smaller the period, the lower this limit */
            return AUDCLNT_E_BUFFER_SIZE_ERROR;
        if(flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK){
            if(duration != period)
                return AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL;
            FIXME("EXCLUSIVE mode with EVENTCALLBACK\n");
            return AUDCLNT_E_DEVICE_IN_USE;
        }else{
            if( duration < 8 * period)
                duration = 8 * period; /* may grow above 2s */
        }
    }

    OSSpinLockLock(&This->lock);

    if(This->initted){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_ALREADY_INITIALIZED;
    }

    This->fmt = clone_format(fmt);
    if(!This->fmt){
        OSSpinLockUnlock(&This->lock);
        return E_OUTOFMEMORY;
    }

    This->period_ms = period / 10000;
    This->period_frames = MulDiv(period, This->fmt->nSamplesPerSec, 10000000);

    This->bufsize_frames = MulDiv(duration, fmt->nSamplesPerSec, 10000000);
    if(mode == AUDCLNT_SHAREMODE_EXCLUSIVE)
        This->bufsize_frames -= This->bufsize_frames % This->period_frames;

    hr = ca_setup_audiounit(This->dataflow, This->unit, This->fmt, &This->dev_desc, &This->converter);
    if(FAILED(hr)){
        CoTaskMemFree(This->fmt);
        This->fmt = NULL;
        OSSpinLockUnlock(&This->lock);
        return hr;
    }

    if(This->dataflow == eCapture){
        AURenderCallbackStruct input;

        memset(&input, 0, sizeof(input));
        input.inputProc = &ca_capture_cb;
        input.inputProcRefCon = This;

        sc = AudioUnitSetProperty(This->unit, kAudioOutputUnitProperty_SetInputCallback,
                kAudioUnitScope_Output, 1, &input, sizeof(input));
        if(sc != noErr){
            WARN("Couldn't set callback: %x\n", (int)sc);
            AudioConverterDispose(This->converter);
            This->converter = NULL;
            CoTaskMemFree(This->fmt);
            This->fmt = NULL;
            OSSpinLockUnlock(&This->lock);
            return osstatus_to_hresult(sc);
        }
    }else{
        AURenderCallbackStruct input;

        memset(&input, 0, sizeof(input));
        input.inputProc = &ca_render_cb;
        input.inputProcRefCon = This;

        sc = AudioUnitSetProperty(This->unit, kAudioUnitProperty_SetRenderCallback,
                kAudioUnitScope_Input, 0, &input, sizeof(input));
        if(sc != noErr){
            WARN("Couldn't set callback: %x\n", (int)sc);
            CoTaskMemFree(This->fmt);
            This->fmt = NULL;
            OSSpinLockUnlock(&This->lock);
            return osstatus_to_hresult(sc);
        }
    }

    sc = AudioUnitInitialize(This->unit);
    if(sc != noErr){
        WARN("Couldn't initialize: %x\n", (int)sc);
        if(This->converter){
            AudioConverterDispose(This->converter);
            This->converter = NULL;
        }
        CoTaskMemFree(This->fmt);
        This->fmt = NULL;
        OSSpinLockUnlock(&This->lock);
        return osstatus_to_hresult(sc);
    }

    /* we play audio continuously because AudioOutputUnitStart sometimes takes
     * a while to return */
    sc = AudioOutputUnitStart(This->unit);
    if(sc != noErr){
        WARN("Unit failed to start: %x\n", (int)sc);
        if(This->converter){
            AudioConverterDispose(This->converter);
            This->converter = NULL;
        }
        CoTaskMemFree(This->fmt);
        This->fmt = NULL;
        OSSpinLockUnlock(&This->lock);
        return osstatus_to_hresult(sc);
    }

    This->local_buffer = HeapAlloc(GetProcessHeap(), 0, This->bufsize_frames * fmt->nBlockAlign);
    silence_buffer(This, This->local_buffer, This->bufsize_frames);

    if(This->dataflow == eCapture){
        This->cap_bufsize_frames = MulDiv(duration, This->dev_desc.mSampleRate, 10000000);
        This->cap_buffer = HeapAlloc(GetProcessHeap(), 0, This->cap_bufsize_frames * This->fmt->nBlockAlign);
    }

    This->vols = HeapAlloc(GetProcessHeap(), 0, fmt->nChannels * sizeof(float));
    if(!This->vols){
        CoTaskMemFree(This->fmt);
        This->fmt = NULL;
        OSSpinLockUnlock(&This->lock);
        return E_OUTOFMEMORY;
    }

    for(i = 0; i < fmt->nChannels; ++i)
        This->vols[i] = 1.f;

    This->share = mode;
    This->flags = flags;

    EnterCriticalSection(&g_sessions_lock);

    hr = get_audio_session(sessionguid, This->parent, fmt->nChannels,
            &This->session);
    if(FAILED(hr)){
        LeaveCriticalSection(&g_sessions_lock);
        CoTaskMemFree(This->fmt);
        This->fmt = NULL;
        HeapFree(GetProcessHeap(), 0, This->vols);
        This->vols = NULL;
        OSSpinLockUnlock(&This->lock);
        return E_INVALIDARG;
    }

    list_add_tail(&This->session->clients, &This->entry);

    LeaveCriticalSection(&g_sessions_lock);

    ca_setvol(This, -1);

    This->initted = TRUE;

    OSSpinLockUnlock(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_GetBufferSize(IAudioClient *iface,
        UINT32 *frames)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)->(%p)\n", This, frames);

    if(!frames)
        return E_POINTER;

    OSSpinLockLock(&This->lock);

    if(!This->initted){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    *frames = This->bufsize_frames;

    OSSpinLockUnlock(&This->lock);

    return S_OK;
}

static HRESULT ca_get_max_stream_latency(ACImpl *This, UInt32 *max)
{
    AudioObjectPropertyAddress addr;
    AudioStreamID *ids;
    UInt32 size;
    OSStatus sc;
    int nstreams, i;

    addr.mScope = This->scope;
    addr.mElement = 0;
    addr.mSelector = kAudioDevicePropertyStreams;

    sc = AudioObjectGetPropertyDataSize(This->adevid, &addr, 0, NULL,
            &size);
    if(sc != noErr){
        WARN("Unable to get size for _Streams property: %x\n", (int)sc);
        return osstatus_to_hresult(sc);
    }

    ids = HeapAlloc(GetProcessHeap(), 0, size);
    if(!ids)
        return E_OUTOFMEMORY;

    sc = AudioObjectGetPropertyData(This->adevid, &addr, 0, NULL, &size, ids);
    if(sc != noErr){
        WARN("Unable to get _Streams property: %x\n", (int)sc);
        HeapFree(GetProcessHeap(), 0, ids);
        return osstatus_to_hresult(sc);
    }

    nstreams = size / sizeof(AudioStreamID);
    *max = 0;

    addr.mSelector = kAudioStreamPropertyLatency;
    for(i = 0; i < nstreams; ++i){
        UInt32 latency;

        size = sizeof(latency);
        sc = AudioObjectGetPropertyData(ids[i], &addr, 0, NULL,
                &size, &latency);
        if(sc != noErr){
            WARN("Unable to get _Latency property: %x\n", (int)sc);
            continue;
        }

        if(latency > *max)
            *max = latency;
    }

    HeapFree(GetProcessHeap(), 0, ids);

    return S_OK;
}

static HRESULT WINAPI AudioClient_GetStreamLatency(IAudioClient *iface,
        REFERENCE_TIME *out)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    UInt32 latency, stream_latency, size;
    AudioObjectPropertyAddress addr;
    OSStatus sc;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, out);

    if(!out)
        return E_POINTER;

    OSSpinLockLock(&This->lock);

    if(!This->initted){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    addr.mScope = This->scope;
    addr.mSelector = kAudioDevicePropertyLatency;
    addr.mElement = 0;

    size = sizeof(latency);
    sc = AudioObjectGetPropertyData(This->adevid, &addr, 0, NULL,
            &size, &latency);
    if(sc != noErr){
        WARN("Couldn't get _Latency property: %x\n", (int)sc);
        OSSpinLockUnlock(&This->lock);
        return osstatus_to_hresult(sc);
    }

    hr = ca_get_max_stream_latency(This, &stream_latency);
    if(FAILED(hr)){
        OSSpinLockUnlock(&This->lock);
        return hr;
    }

    latency += stream_latency;
    /* pretend we process audio in Period chunks, so max latency includes
     * the period time */
    *out = MulDiv(latency, 10000000, This->fmt->nSamplesPerSec)
         + This->period_ms * 10000;

    OSSpinLockUnlock(&This->lock);

    return S_OK;
}

static HRESULT AudioClient_GetCurrentPadding_nolock(ACImpl *This,
        UINT32 *numpad)
{
    if(!This->initted)
        return AUDCLNT_E_NOT_INITIALIZED;

    if(This->dataflow == eCapture)
        capture_resample(This);

    *numpad = This->held_frames;

    return S_OK;
}

static HRESULT WINAPI AudioClient_GetCurrentPadding(IAudioClient *iface,
        UINT32 *numpad)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, numpad);

    if(!numpad)
        return E_POINTER;

    OSSpinLockLock(&This->lock);

    hr = AudioClient_GetCurrentPadding_nolock(This, numpad);

    OSSpinLockUnlock(&This->lock);

    return hr;
}

static HRESULT WINAPI AudioClient_IsFormatSupported(IAudioClient *iface,
        AUDCLNT_SHAREMODE mode, const WAVEFORMATEX *pwfx,
        WAVEFORMATEX **outpwfx)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    AudioStreamBasicDescription dev_desc;
    AudioConverterRef converter;
    AudioComponentInstance unit;
    WAVEFORMATEXTENSIBLE *fmtex = (WAVEFORMATEXTENSIBLE*)pwfx;
    HRESULT hr;

    TRACE("(%p)->(%x, %p, %p)\n", This, mode, pwfx, outpwfx);

    if(!pwfx || (mode == AUDCLNT_SHAREMODE_SHARED && !outpwfx))
        return E_POINTER;

    if(mode != AUDCLNT_SHAREMODE_SHARED && mode != AUDCLNT_SHAREMODE_EXCLUSIVE)
        return E_INVALIDARG;

    if(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
            pwfx->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
        return E_INVALIDARG;

    dump_fmt(pwfx);

    if(outpwfx){
        *outpwfx = NULL;
        if(mode != AUDCLNT_SHAREMODE_SHARED)
            outpwfx = NULL;
    }

    if(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE){
        if(pwfx->nAvgBytesPerSec == 0 ||
                pwfx->nBlockAlign == 0 ||
                fmtex->Samples.wValidBitsPerSample > pwfx->wBitsPerSample)
            return E_INVALIDARG;
        if(fmtex->Samples.wValidBitsPerSample < pwfx->wBitsPerSample)
            goto unsupported;
        if(mode == AUDCLNT_SHAREMODE_EXCLUSIVE){
            if(fmtex->dwChannelMask == 0 ||
                    fmtex->dwChannelMask & SPEAKER_RESERVED)
                goto unsupported;
        }
    }

    if(pwfx->nBlockAlign != pwfx->nChannels * pwfx->wBitsPerSample / 8 ||
            pwfx->nAvgBytesPerSec != pwfx->nBlockAlign * pwfx->nSamplesPerSec)
        goto unsupported;

    if(pwfx->nChannels == 0)
        return AUDCLNT_E_UNSUPPORTED_FORMAT;

    unit = get_audiounit(This->dataflow, This->adevid);

    converter = NULL;
    hr = ca_setup_audiounit(This->dataflow, unit, pwfx, &dev_desc, &converter);
    AudioComponentInstanceDispose(unit);
    if(FAILED(hr))
        goto unsupported;

    if(converter)
        AudioConverterDispose(converter);

    return S_OK;

unsupported:
    if(outpwfx){
        hr = IAudioClient_GetMixFormat(&This->IAudioClient_iface, outpwfx);
        if(FAILED(hr))
            return hr;
        return S_FALSE;
    }

    return AUDCLNT_E_UNSUPPORTED_FORMAT;
}

static DWORD ca_channel_layout_to_channel_mask(const AudioChannelLayout *layout)
{
    int i;
    DWORD mask = 0;

    for (i = 0; i < layout->mNumberChannelDescriptions; ++i) {
        switch (layout->mChannelDescriptions[i].mChannelLabel) {
            default: FIXME("Unhandled channel 0x%x\n", layout->mChannelDescriptions[i].mChannelLabel); break;
            case kAudioChannelLabel_Left: mask |= SPEAKER_FRONT_LEFT; break;
            case kAudioChannelLabel_Mono:
            case kAudioChannelLabel_Center: mask |= SPEAKER_FRONT_CENTER; break;
            case kAudioChannelLabel_Right: mask |= SPEAKER_FRONT_RIGHT; break;
            case kAudioChannelLabel_LeftSurround: mask |= SPEAKER_BACK_LEFT; break;
            case kAudioChannelLabel_CenterSurround: mask |= SPEAKER_BACK_CENTER; break;
            case kAudioChannelLabel_RightSurround: mask |= SPEAKER_BACK_RIGHT; break;
            case kAudioChannelLabel_LFEScreen: mask |= SPEAKER_LOW_FREQUENCY; break;
            case kAudioChannelLabel_LeftSurroundDirect: mask |= SPEAKER_SIDE_LEFT; break;
            case kAudioChannelLabel_RightSurroundDirect: mask |= SPEAKER_SIDE_RIGHT; break;
            case kAudioChannelLabel_TopCenterSurround: mask |= SPEAKER_TOP_CENTER; break;
            case kAudioChannelLabel_VerticalHeightLeft: mask |= SPEAKER_TOP_FRONT_LEFT; break;
            case kAudioChannelLabel_VerticalHeightCenter: mask |= SPEAKER_TOP_FRONT_CENTER; break;
            case kAudioChannelLabel_VerticalHeightRight: mask |= SPEAKER_TOP_FRONT_RIGHT; break;
            case kAudioChannelLabel_TopBackLeft: mask |= SPEAKER_TOP_BACK_LEFT; break;
            case kAudioChannelLabel_TopBackCenter: mask |= SPEAKER_TOP_BACK_CENTER; break;
            case kAudioChannelLabel_TopBackRight: mask |= SPEAKER_TOP_BACK_RIGHT; break;
            case kAudioChannelLabel_LeftCenter: mask |= SPEAKER_FRONT_LEFT_OF_CENTER; break;
            case kAudioChannelLabel_RightCenter: mask |= SPEAKER_FRONT_RIGHT_OF_CENTER; break;
        }
    }

    return mask;
}

/* For most hardware on Windows, users must choose a configuration with an even
 * number of channels (stereo, quad, 5.1, 7.1). Users can then disable
 * channels, but those channels are still reported to applications from
 * GetMixFormat! Some applications behave badly if given an odd number of
 * channels (e.g. 2.1).  Here, we find the nearest configuration that Windows
 * would report for a given channel layout. */
static void convert_channel_layout(const AudioChannelLayout *ca_layout, WAVEFORMATEXTENSIBLE *fmt)
{
    DWORD ca_mask = ca_channel_layout_to_channel_mask(ca_layout);

    TRACE("Got channel mask for CA: 0x%x\n", ca_mask);

    if (ca_layout->mNumberChannelDescriptions == 1)
    {
        fmt->Format.nChannels = 1;
        fmt->dwChannelMask = ca_mask;
        return;
    }

    /* compare against known configurations and find smallest configuration
     * which is a superset of the given speakers */

    if (ca_layout->mNumberChannelDescriptions <= 2 &&
            (ca_mask & ~KSAUDIO_SPEAKER_STEREO) == 0)
    {
        fmt->Format.nChannels = 2;
        fmt->dwChannelMask = KSAUDIO_SPEAKER_STEREO;
        return;
    }

    if (ca_layout->mNumberChannelDescriptions <= 4 &&
            (ca_mask & ~KSAUDIO_SPEAKER_QUAD) == 0)
    {
        fmt->Format.nChannels = 4;
        fmt->dwChannelMask = KSAUDIO_SPEAKER_QUAD;
        return;
    }

    if (ca_layout->mNumberChannelDescriptions <= 4 &&
            (ca_mask & ~KSAUDIO_SPEAKER_SURROUND) == 0)
    {
        fmt->Format.nChannels = 4;
        fmt->dwChannelMask = KSAUDIO_SPEAKER_SURROUND;
        return;
    }

    if (ca_layout->mNumberChannelDescriptions <= 6 &&
            (ca_mask & ~KSAUDIO_SPEAKER_5POINT1) == 0)
    {
        fmt->Format.nChannels = 6;
        fmt->dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
        return;
    }

    if (ca_layout->mNumberChannelDescriptions <= 6 &&
            (ca_mask & ~KSAUDIO_SPEAKER_5POINT1_SURROUND) == 0)
    {
        fmt->Format.nChannels = 6;
        fmt->dwChannelMask = KSAUDIO_SPEAKER_5POINT1_SURROUND;
        return;
    }

    if (ca_layout->mNumberChannelDescriptions <= 8 &&
            (ca_mask & ~KSAUDIO_SPEAKER_7POINT1) == 0)
    {
        fmt->Format.nChannels = 8;
        fmt->dwChannelMask = KSAUDIO_SPEAKER_7POINT1;
        return;
    }

    if (ca_layout->mNumberChannelDescriptions <= 8 &&
            (ca_mask & ~KSAUDIO_SPEAKER_7POINT1_SURROUND) == 0)
    {
        fmt->Format.nChannels = 8;
        fmt->dwChannelMask = KSAUDIO_SPEAKER_7POINT1_SURROUND;
        return;
    }

    /* oddball format, report truthfully */
    fmt->Format.nChannels = ca_layout->mNumberChannelDescriptions;
    fmt->dwChannelMask = ca_mask;
}

static HRESULT WINAPI AudioClient_GetMixFormat(IAudioClient *iface,
        WAVEFORMATEX **pwfx)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    WAVEFORMATEXTENSIBLE *fmt;
    OSStatus sc;
    UInt32 size;
    Float64 rate;
    AudioBufferList *buffers;
    AudioChannelLayout *layout;
    AudioObjectPropertyAddress addr;
    int i;

    TRACE("(%p)->(%p)\n", This, pwfx);

    if(!pwfx)
        return E_POINTER;
    *pwfx = NULL;

    fmt = CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
    if(!fmt)
        return E_OUTOFMEMORY;

    fmt->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;

    addr.mScope = This->scope;
    addr.mElement = 0;
    addr.mSelector = kAudioDevicePropertyPreferredChannelLayout;

    sc = AudioObjectGetPropertyDataSize(This->adevid, &addr, 0, NULL, &size);
    if(sc == noErr){
        layout = HeapAlloc(GetProcessHeap(), 0, size);

        sc = AudioObjectGetPropertyData(This->adevid, &addr, 0, NULL, &size, layout);
        if(sc == noErr){
            TRACE("Got channel layout: {tag: 0x%x, bitmap: 0x%x, num_descs: %u}\n",
                    layout->mChannelLayoutTag, layout->mChannelBitmap, layout->mNumberChannelDescriptions);

            if(layout->mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions){
                convert_channel_layout(layout, fmt);
            }else{
                WARN("Haven't implemented support for this layout tag: 0x%x, guessing at layout\n", layout->mChannelLayoutTag);
                fmt->Format.nChannels = 0;
            }
        }else{
            TRACE("Unable to get _PreferredChannelLayout property: %x, guessing at layout\n", (int)sc);
            fmt->Format.nChannels = 0;
        }

        HeapFree(GetProcessHeap(), 0, layout);
    }else{
        TRACE("Unable to get size for _PreferredChannelLayout property: %x, guessing at layout\n", (int)sc);
        fmt->Format.nChannels = 0;
    }

    if(fmt->Format.nChannels == 0){
        addr.mScope = This->scope;
        addr.mElement = 0;
        addr.mSelector = kAudioDevicePropertyStreamConfiguration;

        sc = AudioObjectGetPropertyDataSize(This->adevid, &addr, 0, NULL, &size);
        if(sc != noErr){
            CoTaskMemFree(fmt);
            WARN("Unable to get size for _StreamConfiguration property: %x\n", (int)sc);
            return osstatus_to_hresult(sc);
        }

        buffers = HeapAlloc(GetProcessHeap(), 0, size);
        if(!buffers){
            CoTaskMemFree(fmt);
            return E_OUTOFMEMORY;
        }

        sc = AudioObjectGetPropertyData(This->adevid, &addr, 0, NULL,
                &size, buffers);
        if(sc != noErr){
            CoTaskMemFree(fmt);
            HeapFree(GetProcessHeap(), 0, buffers);
            WARN("Unable to get _StreamConfiguration property: %x\n", (int)sc);
            return osstatus_to_hresult(sc);
        }

        fmt->Format.nChannels = 0;
        for(i = 0; i < buffers->mNumberBuffers; ++i)
            fmt->Format.nChannels += buffers->mBuffers[i].mNumberChannels;

        HeapFree(GetProcessHeap(), 0, buffers);

        fmt->dwChannelMask = get_channel_mask(fmt->Format.nChannels);
    }

    addr.mSelector = kAudioDevicePropertyNominalSampleRate;
    size = sizeof(Float64);
    sc = AudioObjectGetPropertyData(This->adevid, &addr, 0, NULL, &size, &rate);
    if(sc != noErr){
        CoTaskMemFree(fmt);
        WARN("Unable to get _NominalSampleRate property: %x\n", (int)sc);
        return osstatus_to_hresult(sc);
    }
    fmt->Format.nSamplesPerSec = rate;

    fmt->Format.wBitsPerSample = 32;
    fmt->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

    fmt->Format.nBlockAlign = (fmt->Format.wBitsPerSample *
            fmt->Format.nChannels) / 8;
    fmt->Format.nAvgBytesPerSec = fmt->Format.nSamplesPerSec *
        fmt->Format.nBlockAlign;

    fmt->Samples.wValidBitsPerSample = fmt->Format.wBitsPerSample;
    fmt->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

    *pwfx = (WAVEFORMATEX*)fmt;
    dump_fmt(*pwfx);

    return S_OK;
}

static HRESULT WINAPI AudioClient_GetDevicePeriod(IAudioClient *iface,
        REFERENCE_TIME *defperiod, REFERENCE_TIME *minperiod)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)->(%p, %p)\n", This, defperiod, minperiod);

    if(!defperiod && !minperiod)
        return E_POINTER;

    if(defperiod)
        *defperiod = DefaultPeriod;
    if(minperiod)
        *minperiod = MinimumPeriod;

    return S_OK;
}

void CALLBACK ca_period_cb(void *user, BOOLEAN timer)
{
    ACImpl *This = user;

    if(This->event)
        SetEvent(This->event);
}

static HRESULT WINAPI AudioClient_Start(IAudioClient *iface)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)\n", This);

    OSSpinLockLock(&This->lock);

    if(!This->initted){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if(This->playing){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_NOT_STOPPED;
    }

    if((This->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK) && !This->event){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_EVENTHANDLE_NOT_SET;
    }

    if(This->event && !This->timer)
        if(!CreateTimerQueueTimer(&This->timer, g_timer_q, ca_period_cb,
                This, 0, This->period_ms, WT_EXECUTEINTIMERTHREAD)){
            This->timer = NULL;
            OSSpinLockUnlock(&This->lock);
            WARN("Unable to create timer: %u\n", GetLastError());
            return E_OUTOFMEMORY;
        }

    This->playing = TRUE;

    OSSpinLockUnlock(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_Stop(IAudioClient *iface)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)\n", This);

    OSSpinLockLock(&This->lock);

    if(!This->initted){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if(!This->playing){
        OSSpinLockUnlock(&This->lock);
        return S_FALSE;
    }

    This->playing = FALSE;

    OSSpinLockUnlock(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_Reset(IAudioClient *iface)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)\n", This);

    OSSpinLockLock(&This->lock);

    if(!This->initted){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if(This->playing){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_NOT_STOPPED;
    }

    if(This->getbuf_last){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_BUFFER_OPERATION_PENDING;
    }

    if(This->dataflow == eRender){
        This->written_frames = 0;
    }else{
        This->written_frames += This->held_frames;
    }

    This->held_frames = 0;
    This->lcl_offs_frames = 0;
    This->wri_offs_frames = 0;
    This->cap_offs_frames = 0;
    This->cap_held_frames = 0;

    OSSpinLockUnlock(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_SetEventHandle(IAudioClient *iface,
        HANDLE event)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)->(%p)\n", This, event);

    if(!event)
        return E_INVALIDARG;

    OSSpinLockLock(&This->lock);

    if(!This->initted){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if(!(This->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK)){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED;
    }

    if (This->event){
        OSSpinLockUnlock(&This->lock);
        FIXME("called twice\n");
        return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
    }

    This->event = event;

    OSSpinLockUnlock(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_GetService(IAudioClient *iface, REFIID riid,
        void **ppv)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    OSSpinLockLock(&This->lock);

    if(!This->initted){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if(IsEqualIID(riid, &IID_IAudioRenderClient)){
        if(This->dataflow != eRender){
            OSSpinLockUnlock(&This->lock);
            return AUDCLNT_E_WRONG_ENDPOINT_TYPE;
        }
        IAudioRenderClient_AddRef(&This->IAudioRenderClient_iface);
        *ppv = &This->IAudioRenderClient_iface;
    }else if(IsEqualIID(riid, &IID_IAudioCaptureClient)){
        if(This->dataflow != eCapture){
            OSSpinLockUnlock(&This->lock);
            return AUDCLNT_E_WRONG_ENDPOINT_TYPE;
        }
        IAudioCaptureClient_AddRef(&This->IAudioCaptureClient_iface);
        *ppv = &This->IAudioCaptureClient_iface;
    }else if(IsEqualIID(riid, &IID_IAudioClock)){
        IAudioClock_AddRef(&This->IAudioClock_iface);
        *ppv = &This->IAudioClock_iface;
    }else if(IsEqualIID(riid, &IID_IAudioStreamVolume)){
        IAudioStreamVolume_AddRef(&This->IAudioStreamVolume_iface);
        *ppv = &This->IAudioStreamVolume_iface;
    }else if(IsEqualIID(riid, &IID_IAudioSessionControl)){
        if(!This->session_wrapper){
            This->session_wrapper = AudioSessionWrapper_Create(This);
            if(!This->session_wrapper){
                OSSpinLockUnlock(&This->lock);
                return E_OUTOFMEMORY;
            }
        }else
            IAudioSessionControl2_AddRef(&This->session_wrapper->IAudioSessionControl2_iface);

        *ppv = &This->session_wrapper->IAudioSessionControl2_iface;
    }else if(IsEqualIID(riid, &IID_IChannelAudioVolume)){
        if(!This->session_wrapper){
            This->session_wrapper = AudioSessionWrapper_Create(This);
            if(!This->session_wrapper){
                OSSpinLockUnlock(&This->lock);
                return E_OUTOFMEMORY;
            }
        }else
            IChannelAudioVolume_AddRef(&This->session_wrapper->IChannelAudioVolume_iface);

        *ppv = &This->session_wrapper->IChannelAudioVolume_iface;
    }else if(IsEqualIID(riid, &IID_ISimpleAudioVolume)){
        if(!This->session_wrapper){
            This->session_wrapper = AudioSessionWrapper_Create(This);
            if(!This->session_wrapper){
                OSSpinLockUnlock(&This->lock);
                return E_OUTOFMEMORY;
            }
        }else
            ISimpleAudioVolume_AddRef(&This->session_wrapper->ISimpleAudioVolume_iface);

        *ppv = &This->session_wrapper->ISimpleAudioVolume_iface;
    }

    if(*ppv){
        OSSpinLockUnlock(&This->lock);
        return S_OK;
    }

    OSSpinLockUnlock(&This->lock);

    FIXME("stub %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static const IAudioClientVtbl AudioClient_Vtbl =
{
    AudioClient_QueryInterface,
    AudioClient_AddRef,
    AudioClient_Release,
    AudioClient_Initialize,
    AudioClient_GetBufferSize,
    AudioClient_GetStreamLatency,
    AudioClient_GetCurrentPadding,
    AudioClient_IsFormatSupported,
    AudioClient_GetMixFormat,
    AudioClient_GetDevicePeriod,
    AudioClient_Start,
    AudioClient_Stop,
    AudioClient_Reset,
    AudioClient_SetEventHandle,
    AudioClient_GetService
};

static HRESULT WINAPI AudioRenderClient_QueryInterface(
        IAudioRenderClient *iface, REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioRenderClient))
        *ppv = iface;
    else if(IsEqualIID(riid, &IID_IMarshal))
        return IUnknown_QueryInterface(This->pUnkFTMarshal, riid, ppv);

    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioRenderClient_AddRef(IAudioRenderClient *iface)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    return AudioClient_AddRef(&This->IAudioClient_iface);
}

static ULONG WINAPI AudioRenderClient_Release(IAudioRenderClient *iface)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    return AudioClient_Release(&This->IAudioClient_iface);
}

static HRESULT WINAPI AudioRenderClient_GetBuffer(IAudioRenderClient *iface,
        UINT32 frames, BYTE **data)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    UINT32 pad;
    HRESULT hr;

    TRACE("(%p)->(%u, %p)\n", This, frames, data);

    if(!data)
        return E_POINTER;
    *data = NULL;

    OSSpinLockLock(&This->lock);

    if(This->getbuf_last){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_OUT_OF_ORDER;
    }

    if(!frames){
        OSSpinLockUnlock(&This->lock);
        return S_OK;
    }

    hr = AudioClient_GetCurrentPadding_nolock(This, &pad);
    if(FAILED(hr)){
        OSSpinLockUnlock(&This->lock);
        return hr;
    }

    if(pad + frames > This->bufsize_frames){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_BUFFER_TOO_LARGE;
    }

    if(This->wri_offs_frames + frames > This->bufsize_frames){
        if(This->tmp_buffer_frames < frames){
            HeapFree(GetProcessHeap(), 0, This->tmp_buffer);
            This->tmp_buffer = HeapAlloc(GetProcessHeap(), 0, frames * This->fmt->nBlockAlign);
            if(!This->tmp_buffer){
                OSSpinLockUnlock(&This->lock);
                return E_OUTOFMEMORY;
            }
            This->tmp_buffer_frames = frames;
        }
        *data = This->tmp_buffer;
        This->getbuf_last = -frames;
    }else{
        *data = This->local_buffer + This->wri_offs_frames * This->fmt->nBlockAlign;
        This->getbuf_last = frames;
    }

    silence_buffer(This, *data, frames);

    OSSpinLockUnlock(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioRenderClient_ReleaseBuffer(
        IAudioRenderClient *iface, UINT32 frames, DWORD flags)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    BYTE *buffer;

    TRACE("(%p)->(%u, %x)\n", This, frames, flags);

    OSSpinLockLock(&This->lock);

    if(!frames){
        This->getbuf_last = 0;
        OSSpinLockUnlock(&This->lock);
        return S_OK;
    }

    if(!This->getbuf_last){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_OUT_OF_ORDER;
    }

    if(frames > (This->getbuf_last >= 0 ? This->getbuf_last : -This->getbuf_last)){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_INVALID_SIZE;
    }

    if(This->getbuf_last >= 0)
        buffer = This->local_buffer + This->wri_offs_frames * This->fmt->nBlockAlign;
    else
        buffer = This->tmp_buffer;

    if(flags & AUDCLNT_BUFFERFLAGS_SILENT)
        silence_buffer(This, buffer, frames);

    if(This->getbuf_last < 0)
        ca_wrap_buffer(This->local_buffer,
                This->wri_offs_frames * This->fmt->nBlockAlign,
                This->bufsize_frames * This->fmt->nBlockAlign,
                buffer, frames * This->fmt->nBlockAlign);


    This->wri_offs_frames += frames;
    This->wri_offs_frames %= This->bufsize_frames;
    This->held_frames += frames;
    This->written_frames += frames;
    This->getbuf_last = 0;

    OSSpinLockUnlock(&This->lock);

    return S_OK;
}

static const IAudioRenderClientVtbl AudioRenderClient_Vtbl = {
    AudioRenderClient_QueryInterface,
    AudioRenderClient_AddRef,
    AudioRenderClient_Release,
    AudioRenderClient_GetBuffer,
    AudioRenderClient_ReleaseBuffer
};

static HRESULT WINAPI AudioCaptureClient_QueryInterface(
        IAudioCaptureClient *iface, REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioCaptureClient))
        *ppv = iface;
    else if(IsEqualIID(riid, &IID_IMarshal))
        return IUnknown_QueryInterface(This->pUnkFTMarshal, riid, ppv);

    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioCaptureClient_AddRef(IAudioCaptureClient *iface)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    return IAudioClient_AddRef(&This->IAudioClient_iface);
}

static ULONG WINAPI AudioCaptureClient_Release(IAudioCaptureClient *iface)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    return IAudioClient_Release(&This->IAudioClient_iface);
}

static HRESULT WINAPI AudioCaptureClient_GetBuffer(IAudioCaptureClient *iface,
        BYTE **data, UINT32 *frames, DWORD *flags, UINT64 *devpos,
        UINT64 *qpcpos)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    UINT32 chunk_bytes, chunk_frames;

    TRACE("(%p)->(%p, %p, %p, %p, %p)\n", This, data, frames, flags,
            devpos, qpcpos);

    if(!data || !frames || !flags)
        return E_POINTER;

    OSSpinLockLock(&This->lock);

    if(This->getbuf_last){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_OUT_OF_ORDER;
    }

    capture_resample(This);

    if(This->held_frames < This->period_frames){
        *frames = 0;
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_S_BUFFER_EMPTY;
    }

    *flags = 0;

    chunk_frames = This->bufsize_frames - This->lcl_offs_frames;
    if(chunk_frames < This->period_frames){
        chunk_bytes = chunk_frames * This->fmt->nBlockAlign;
        if(!This->tmp_buffer)
            This->tmp_buffer = HeapAlloc(GetProcessHeap(), 0, This->period_frames * This->fmt->nBlockAlign);
        *data = This->tmp_buffer;
        memcpy(*data, This->local_buffer + This->lcl_offs_frames * This->fmt->nBlockAlign, chunk_bytes);
        memcpy((*data) + chunk_bytes, This->local_buffer, This->period_frames * This->fmt->nBlockAlign - chunk_bytes);
    }else
        *data = This->local_buffer + This->lcl_offs_frames * This->fmt->nBlockAlign;

    This->getbuf_last = *frames = This->period_frames;

    if(devpos)
        *devpos = This->written_frames;
    if(qpcpos){ /* fixme: qpc of recording time */
        LARGE_INTEGER stamp, freq;
        QueryPerformanceCounter(&stamp);
        QueryPerformanceFrequency(&freq);
        *qpcpos = (stamp.QuadPart * (INT64)10000000) / freq.QuadPart;
    }

    OSSpinLockUnlock(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioCaptureClient_ReleaseBuffer(
        IAudioCaptureClient *iface, UINT32 done)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);

    TRACE("(%p)->(%u)\n", This, done);

    OSSpinLockLock(&This->lock);

    if(!done){
        This->getbuf_last = 0;
        OSSpinLockUnlock(&This->lock);
        return S_OK;
    }

    if(!This->getbuf_last){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_OUT_OF_ORDER;
    }

    if(This->getbuf_last != done){
        OSSpinLockUnlock(&This->lock);
        return AUDCLNT_E_INVALID_SIZE;
    }

    This->written_frames += done;
    This->held_frames -= done;
    This->lcl_offs_frames += done;
    This->lcl_offs_frames %= This->bufsize_frames;
    This->getbuf_last = 0;

    OSSpinLockUnlock(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioCaptureClient_GetNextPacketSize(
        IAudioCaptureClient *iface, UINT32 *frames)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);

    TRACE("(%p)->(%p)\n", This, frames);

    if(!frames)
        return E_POINTER;

    OSSpinLockLock(&This->lock);

    capture_resample(This);

    if(This->held_frames >= This->period_frames)
        *frames = This->period_frames;
    else
        *frames = 0;

    OSSpinLockUnlock(&This->lock);

    return S_OK;
}

static const IAudioCaptureClientVtbl AudioCaptureClient_Vtbl =
{
    AudioCaptureClient_QueryInterface,
    AudioCaptureClient_AddRef,
    AudioCaptureClient_Release,
    AudioCaptureClient_GetBuffer,
    AudioCaptureClient_ReleaseBuffer,
    AudioCaptureClient_GetNextPacketSize
};

static HRESULT WINAPI AudioClock_QueryInterface(IAudioClock *iface,
        REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioClock(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IAudioClock))
        *ppv = iface;
    else if(IsEqualIID(riid, &IID_IAudioClock2))
        *ppv = &This->IAudioClock2_iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioClock_AddRef(IAudioClock *iface)
{
    ACImpl *This = impl_from_IAudioClock(iface);
    return IAudioClient_AddRef(&This->IAudioClient_iface);
}

static ULONG WINAPI AudioClock_Release(IAudioClock *iface)
{
    ACImpl *This = impl_from_IAudioClock(iface);
    return IAudioClient_Release(&This->IAudioClient_iface);
}

static HRESULT WINAPI AudioClock_GetFrequency(IAudioClock *iface, UINT64 *freq)
{
    ACImpl *This = impl_from_IAudioClock(iface);

    TRACE("(%p)->(%p)\n", This, freq);

    if(This->share == AUDCLNT_SHAREMODE_SHARED)
        *freq = (UINT64)This->fmt->nSamplesPerSec * This->fmt->nBlockAlign;
    else
        *freq = This->fmt->nSamplesPerSec;

    return S_OK;
}

static HRESULT AudioClock_GetPosition_nolock(ACImpl *This,
        UINT64 *pos, UINT64 *qpctime)
{
    *pos = This->written_frames - This->held_frames;

    if(This->share == AUDCLNT_SHAREMODE_SHARED)
        *pos *= This->fmt->nBlockAlign;

    if(qpctime){
        LARGE_INTEGER stamp, freq;
        QueryPerformanceCounter(&stamp);
        QueryPerformanceFrequency(&freq);
        *qpctime = (stamp.QuadPart * (INT64)10000000) / freq.QuadPart;
    }

    return S_OK;
}

static HRESULT WINAPI AudioClock_GetPosition(IAudioClock *iface, UINT64 *pos,
        UINT64 *qpctime)
{
    ACImpl *This = impl_from_IAudioClock(iface);
    HRESULT hr;

    TRACE("(%p)->(%p, %p)\n", This, pos, qpctime);

    if(!pos)
        return E_POINTER;

    OSSpinLockLock(&This->lock);

    hr = AudioClock_GetPosition_nolock(This, pos, qpctime);

    OSSpinLockUnlock(&This->lock);

    return hr;
}

static HRESULT WINAPI AudioClock_GetCharacteristics(IAudioClock *iface,
        DWORD *chars)
{
    ACImpl *This = impl_from_IAudioClock(iface);

    TRACE("(%p)->(%p)\n", This, chars);

    if(!chars)
        return E_POINTER;

    *chars = AUDIOCLOCK_CHARACTERISTIC_FIXED_FREQ;

    return S_OK;
}

static const IAudioClockVtbl AudioClock_Vtbl =
{
    AudioClock_QueryInterface,
    AudioClock_AddRef,
    AudioClock_Release,
    AudioClock_GetFrequency,
    AudioClock_GetPosition,
    AudioClock_GetCharacteristics
};

static HRESULT WINAPI AudioClock2_QueryInterface(IAudioClock2 *iface,
        REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioClock2(iface);
    return IAudioClock_QueryInterface(&This->IAudioClock_iface, riid, ppv);
}

static ULONG WINAPI AudioClock2_AddRef(IAudioClock2 *iface)
{
    ACImpl *This = impl_from_IAudioClock2(iface);
    return IAudioClient_AddRef(&This->IAudioClient_iface);
}

static ULONG WINAPI AudioClock2_Release(IAudioClock2 *iface)
{
    ACImpl *This = impl_from_IAudioClock2(iface);
    return IAudioClient_Release(&This->IAudioClient_iface);
}

static HRESULT WINAPI AudioClock2_GetDevicePosition(IAudioClock2 *iface,
        UINT64 *pos, UINT64 *qpctime)
{
    ACImpl *This = impl_from_IAudioClock2(iface);

    FIXME("(%p)->(%p, %p)\n", This, pos, qpctime);

    return E_NOTIMPL;
}

static const IAudioClock2Vtbl AudioClock2_Vtbl =
{
    AudioClock2_QueryInterface,
    AudioClock2_AddRef,
    AudioClock2_Release,
    AudioClock2_GetDevicePosition
};

static AudioSessionWrapper *AudioSessionWrapper_Create(ACImpl *client)
{
    AudioSessionWrapper *ret;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(AudioSessionWrapper));
    if(!ret)
        return NULL;

    ret->IAudioSessionControl2_iface.lpVtbl = &AudioSessionControl2_Vtbl;
    ret->ISimpleAudioVolume_iface.lpVtbl = &SimpleAudioVolume_Vtbl;
    ret->IChannelAudioVolume_iface.lpVtbl = &ChannelAudioVolume_Vtbl;

    ret->ref = 1;

    ret->client = client;
    if(client){
        ret->session = client->session;
        AudioClient_AddRef(&client->IAudioClient_iface);
    }

    return ret;
}

static HRESULT WINAPI AudioSessionControl_QueryInterface(
        IAudioSessionControl2 *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioSessionControl) ||
            IsEqualIID(riid, &IID_IAudioSessionControl2))
        *ppv = iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioSessionControl_AddRef(IAudioSessionControl2 *iface)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    return ref;
}

static ULONG WINAPI AudioSessionControl_Release(IAudioSessionControl2 *iface)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    if(!ref){
        if(This->client){
            OSSpinLockLock(&This->client->lock);
            This->client->session_wrapper = NULL;
            OSSpinLockUnlock(&This->client->lock);
            AudioClient_Release(&This->client->IAudioClient_iface);
        }
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI AudioSessionControl_GetState(IAudioSessionControl2 *iface,
        AudioSessionState *state)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);
    ACImpl *client;

    TRACE("(%p)->(%p)\n", This, state);

    if(!state)
        return NULL_PTR_ERR;

    EnterCriticalSection(&g_sessions_lock);

    if(list_empty(&This->session->clients)){
        *state = AudioSessionStateExpired;
        LeaveCriticalSection(&g_sessions_lock);
        return S_OK;
    }

    LIST_FOR_EACH_ENTRY(client, &This->session->clients, ACImpl, entry){
        OSSpinLockLock(&client->lock);
        if(client->playing){
            *state = AudioSessionStateActive;
            OSSpinLockUnlock(&client->lock);
            LeaveCriticalSection(&g_sessions_lock);
            return S_OK;
        }
        OSSpinLockUnlock(&client->lock);
    }

    LeaveCriticalSection(&g_sessions_lock);

    *state = AudioSessionStateInactive;

    return S_OK;
}

static HRESULT WINAPI AudioSessionControl_GetDisplayName(
        IAudioSessionControl2 *iface, WCHAR **name)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_SetDisplayName(
        IAudioSessionControl2 *iface, const WCHAR *name, const GUID *session)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p, %s) - stub\n", This, name, debugstr_guid(session));

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_GetIconPath(
        IAudioSessionControl2 *iface, WCHAR **path)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, path);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_SetIconPath(
        IAudioSessionControl2 *iface, const WCHAR *path, const GUID *session)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p, %s) - stub\n", This, path, debugstr_guid(session));

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_GetGroupingParam(
        IAudioSessionControl2 *iface, GUID *group)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, group);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_SetGroupingParam(
        IAudioSessionControl2 *iface, const GUID *group, const GUID *session)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%s, %s) - stub\n", This, debugstr_guid(group),
            debugstr_guid(session));

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_RegisterAudioSessionNotification(
        IAudioSessionControl2 *iface, IAudioSessionEvents *events)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, events);

    return S_OK;
}

static HRESULT WINAPI AudioSessionControl_UnregisterAudioSessionNotification(
        IAudioSessionControl2 *iface, IAudioSessionEvents *events)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, events);

    return S_OK;
}

static HRESULT WINAPI AudioSessionControl_GetSessionIdentifier(
        IAudioSessionControl2 *iface, WCHAR **id)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_GetSessionInstanceIdentifier(
        IAudioSessionControl2 *iface, WCHAR **id)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_GetProcessId(
        IAudioSessionControl2 *iface, DWORD *pid)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    TRACE("(%p)->(%p)\n", This, pid);

    if(!pid)
        return E_POINTER;

    *pid = GetCurrentProcessId();

    return S_OK;
}

static HRESULT WINAPI AudioSessionControl_IsSystemSoundsSession(
        IAudioSessionControl2 *iface)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    TRACE("(%p)\n", This);

    return S_FALSE;
}

static HRESULT WINAPI AudioSessionControl_SetDuckingPreference(
        IAudioSessionControl2 *iface, BOOL optout)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    TRACE("(%p)->(%d)\n", This, optout);

    return S_OK;
}

static const IAudioSessionControl2Vtbl AudioSessionControl2_Vtbl =
{
    AudioSessionControl_QueryInterface,
    AudioSessionControl_AddRef,
    AudioSessionControl_Release,
    AudioSessionControl_GetState,
    AudioSessionControl_GetDisplayName,
    AudioSessionControl_SetDisplayName,
    AudioSessionControl_GetIconPath,
    AudioSessionControl_SetIconPath,
    AudioSessionControl_GetGroupingParam,
    AudioSessionControl_SetGroupingParam,
    AudioSessionControl_RegisterAudioSessionNotification,
    AudioSessionControl_UnregisterAudioSessionNotification,
    AudioSessionControl_GetSessionIdentifier,
    AudioSessionControl_GetSessionInstanceIdentifier,
    AudioSessionControl_GetProcessId,
    AudioSessionControl_IsSystemSoundsSession,
    AudioSessionControl_SetDuckingPreference
};

/* index == -1 means set all channels, otherwise sets only the given channel */
static HRESULT ca_setvol(ACImpl *This, UINT32 index)
{
    Float32 level;
    OSStatus sc;

    if(This->session->mute)
        level = 0.;
    else{
        if(index == (UINT32)-1){
            UINT32 i;
            level = 1.;
            for(i = 0; i < This->fmt->nChannels; ++i){
                Float32 tmp;
                tmp = This->session->master_vol *
                    This->session->channel_vols[i] * This->vols[i];
                level = tmp < level ? tmp : level;
            }
        }else
            level = This->session->master_vol *
                This->session->channel_vols[index] * This->vols[index];
    }

    sc = AudioUnitSetParameter(This->unit, kHALOutputParam_Volume,
            kAudioUnitScope_Global, 0, level, 0);
    if(sc != noErr)
        WARN("Couldn't set volume: %x\n", (int)sc);

    return S_OK;
}

static HRESULT ca_session_setvol(AudioSession *session, UINT32 index)
{
    HRESULT ret = S_OK;
    ACImpl *client;

    LIST_FOR_EACH_ENTRY(client, &session->clients, ACImpl, entry){
        HRESULT hr;
        hr = ca_setvol(client, index);
        if(FAILED(hr))
            ret = hr;
    }

    return ret;
}

static HRESULT WINAPI SimpleAudioVolume_QueryInterface(
        ISimpleAudioVolume *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_ISimpleAudioVolume))
        *ppv = iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI SimpleAudioVolume_AddRef(ISimpleAudioVolume *iface)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    return AudioSessionControl_AddRef(&This->IAudioSessionControl2_iface);
}

static ULONG WINAPI SimpleAudioVolume_Release(ISimpleAudioVolume *iface)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    return AudioSessionControl_Release(&This->IAudioSessionControl2_iface);
}

static HRESULT WINAPI SimpleAudioVolume_SetMasterVolume(
        ISimpleAudioVolume *iface, float level, const GUID *context)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    AudioSession *session = This->session;
    HRESULT ret;

    TRACE("(%p)->(%f, %s)\n", session, level, wine_dbgstr_guid(context));

    if(level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if(context)
        FIXME("Notifications not supported yet\n");

    EnterCriticalSection(&session->lock);

    session->master_vol = level;

    ret = ca_session_setvol(session, -1);

    LeaveCriticalSection(&session->lock);

    return ret;
}

static HRESULT WINAPI SimpleAudioVolume_GetMasterVolume(
        ISimpleAudioVolume *iface, float *level)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%p)\n", session, level);

    if(!level)
        return NULL_PTR_ERR;

    *level = session->master_vol;

    return S_OK;
}

static HRESULT WINAPI SimpleAudioVolume_SetMute(ISimpleAudioVolume *iface,
        BOOL mute, const GUID *context)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%u, %s)\n", session, mute, debugstr_guid(context));

    if(context)
        FIXME("Notifications not supported yet\n");

    EnterCriticalSection(&session->lock);

    session->mute = mute;

    ca_session_setvol(session, -1);

    LeaveCriticalSection(&session->lock);

    return S_OK;
}

static HRESULT WINAPI SimpleAudioVolume_GetMute(ISimpleAudioVolume *iface,
        BOOL *mute)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%p)\n", session, mute);

    if(!mute)
        return NULL_PTR_ERR;

    *mute = session->mute;

    return S_OK;
}

static const ISimpleAudioVolumeVtbl SimpleAudioVolume_Vtbl  =
{
    SimpleAudioVolume_QueryInterface,
    SimpleAudioVolume_AddRef,
    SimpleAudioVolume_Release,
    SimpleAudioVolume_SetMasterVolume,
    SimpleAudioVolume_GetMasterVolume,
    SimpleAudioVolume_SetMute,
    SimpleAudioVolume_GetMute
};

static HRESULT WINAPI AudioStreamVolume_QueryInterface(
        IAudioStreamVolume *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioStreamVolume))
        *ppv = iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioStreamVolume_AddRef(IAudioStreamVolume *iface)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    return IAudioClient_AddRef(&This->IAudioClient_iface);
}

static ULONG WINAPI AudioStreamVolume_Release(IAudioStreamVolume *iface)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    return IAudioClient_Release(&This->IAudioClient_iface);
}

static HRESULT WINAPI AudioStreamVolume_GetChannelCount(
        IAudioStreamVolume *iface, UINT32 *out)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%p)\n", This, out);

    if(!out)
        return E_POINTER;

    *out = This->fmt->nChannels;

    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_SetChannelVolume(
        IAudioStreamVolume *iface, UINT32 index, float level)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    HRESULT ret;

    TRACE("(%p)->(%d, %f)\n", This, index, level);

    if(level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if(index >= This->fmt->nChannels)
        return E_INVALIDARG;

    OSSpinLockLock(&This->lock);

    This->vols[index] = level;

    WARN("CoreAudio doesn't support per-channel volume control\n");
    ret = ca_setvol(This, index);

    OSSpinLockUnlock(&This->lock);

    return ret;
}

static HRESULT WINAPI AudioStreamVolume_GetChannelVolume(
        IAudioStreamVolume *iface, UINT32 index, float *level)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%d, %p)\n", This, index, level);

    if(!level)
        return E_POINTER;

    if(index >= This->fmt->nChannels)
        return E_INVALIDARG;

    *level = This->vols[index];

    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_SetAllVolumes(
        IAudioStreamVolume *iface, UINT32 count, const float *levels)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    int i;
    HRESULT ret;

    TRACE("(%p)->(%d, %p)\n", This, count, levels);

    if(!levels)
        return E_POINTER;

    if(count != This->fmt->nChannels)
        return E_INVALIDARG;

    OSSpinLockLock(&This->lock);

    for(i = 0; i < count; ++i)
        This->vols[i] = levels[i];

    ret = ca_setvol(This, -1);

    OSSpinLockUnlock(&This->lock);

    return ret;
}

static HRESULT WINAPI AudioStreamVolume_GetAllVolumes(
        IAudioStreamVolume *iface, UINT32 count, float *levels)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    int i;

    TRACE("(%p)->(%d, %p)\n", This, count, levels);

    if(!levels)
        return E_POINTER;

    if(count != This->fmt->nChannels)
        return E_INVALIDARG;

    OSSpinLockLock(&This->lock);

    for(i = 0; i < count; ++i)
        levels[i] = This->vols[i];

    OSSpinLockUnlock(&This->lock);

    return S_OK;
}

static const IAudioStreamVolumeVtbl AudioStreamVolume_Vtbl =
{
    AudioStreamVolume_QueryInterface,
    AudioStreamVolume_AddRef,
    AudioStreamVolume_Release,
    AudioStreamVolume_GetChannelCount,
    AudioStreamVolume_SetChannelVolume,
    AudioStreamVolume_GetChannelVolume,
    AudioStreamVolume_SetAllVolumes,
    AudioStreamVolume_GetAllVolumes
};

static HRESULT WINAPI ChannelAudioVolume_QueryInterface(
        IChannelAudioVolume *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IChannelAudioVolume))
        *ppv = iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ChannelAudioVolume_AddRef(IChannelAudioVolume *iface)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    return AudioSessionControl_AddRef(&This->IAudioSessionControl2_iface);
}

static ULONG WINAPI ChannelAudioVolume_Release(IChannelAudioVolume *iface)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    return AudioSessionControl_Release(&This->IAudioSessionControl2_iface);
}

static HRESULT WINAPI ChannelAudioVolume_GetChannelCount(
        IChannelAudioVolume *iface, UINT32 *out)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%p)\n", session, out);

    if(!out)
        return NULL_PTR_ERR;

    *out = session->channel_count;

    return S_OK;
}

static HRESULT WINAPI ChannelAudioVolume_SetChannelVolume(
        IChannelAudioVolume *iface, UINT32 index, float level,
        const GUID *context)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;
    HRESULT ret;

    TRACE("(%p)->(%d, %f, %s)\n", session, index, level,
            wine_dbgstr_guid(context));

    if(level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if(index >= session->channel_count)
        return E_INVALIDARG;

    if(context)
        FIXME("Notifications not supported yet\n");

    EnterCriticalSection(&session->lock);

    session->channel_vols[index] = level;

    WARN("CoreAudio doesn't support per-channel volume control\n");
    ret = ca_session_setvol(session, index);

    LeaveCriticalSection(&session->lock);

    return ret;
}

static HRESULT WINAPI ChannelAudioVolume_GetChannelVolume(
        IChannelAudioVolume *iface, UINT32 index, float *level)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%d, %p)\n", session, index, level);

    if(!level)
        return NULL_PTR_ERR;

    if(index >= session->channel_count)
        return E_INVALIDARG;

    *level = session->channel_vols[index];

    return S_OK;
}

static HRESULT WINAPI ChannelAudioVolume_SetAllVolumes(
        IChannelAudioVolume *iface, UINT32 count, const float *levels,
        const GUID *context)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;
    int i;
    HRESULT ret;

    TRACE("(%p)->(%d, %p, %s)\n", session, count, levels,
            wine_dbgstr_guid(context));

    if(!levels)
        return NULL_PTR_ERR;

    if(count != session->channel_count)
        return E_INVALIDARG;

    if(context)
        FIXME("Notifications not supported yet\n");

    EnterCriticalSection(&session->lock);

    for(i = 0; i < count; ++i)
        session->channel_vols[i] = levels[i];

    ret = ca_session_setvol(session, -1);

    LeaveCriticalSection(&session->lock);

    return ret;
}

static HRESULT WINAPI ChannelAudioVolume_GetAllVolumes(
        IChannelAudioVolume *iface, UINT32 count, float *levels)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;
    int i;

    TRACE("(%p)->(%d, %p)\n", session, count, levels);

    if(!levels)
        return NULL_PTR_ERR;

    if(count != session->channel_count)
        return E_INVALIDARG;

    for(i = 0; i < count; ++i)
        levels[i] = session->channel_vols[i];

    return S_OK;
}

static const IChannelAudioVolumeVtbl ChannelAudioVolume_Vtbl =
{
    ChannelAudioVolume_QueryInterface,
    ChannelAudioVolume_AddRef,
    ChannelAudioVolume_Release,
    ChannelAudioVolume_GetChannelCount,
    ChannelAudioVolume_SetChannelVolume,
    ChannelAudioVolume_GetChannelVolume,
    ChannelAudioVolume_SetAllVolumes,
    ChannelAudioVolume_GetAllVolumes
};

static HRESULT WINAPI AudioSessionManager_QueryInterface(IAudioSessionManager2 *iface,
        REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioSessionManager) ||
            IsEqualIID(riid, &IID_IAudioSessionManager2))
        *ppv = iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioSessionManager_AddRef(IAudioSessionManager2 *iface)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    return ref;
}

static ULONG WINAPI AudioSessionManager_Release(IAudioSessionManager2 *iface)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    if(!ref)
        HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

static HRESULT WINAPI AudioSessionManager_GetAudioSessionControl(
        IAudioSessionManager2 *iface, const GUID *session_guid, DWORD flags,
        IAudioSessionControl **out)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    AudioSession *session;
    AudioSessionWrapper *wrapper;
    HRESULT hr;

    TRACE("(%p)->(%s, %x, %p)\n", This, debugstr_guid(session_guid),
            flags, out);

    hr = get_audio_session(session_guid, This->device, 0, &session);
    if(FAILED(hr))
        return hr;

    wrapper = AudioSessionWrapper_Create(NULL);
    if(!wrapper)
        return E_OUTOFMEMORY;

    wrapper->session = session;

    *out = (IAudioSessionControl*)&wrapper->IAudioSessionControl2_iface;

    return S_OK;
}

static HRESULT WINAPI AudioSessionManager_GetSimpleAudioVolume(
        IAudioSessionManager2 *iface, const GUID *session_guid, DWORD flags,
        ISimpleAudioVolume **out)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    AudioSession *session;
    AudioSessionWrapper *wrapper;
    HRESULT hr;

    TRACE("(%p)->(%s, %x, %p)\n", This, debugstr_guid(session_guid),
            flags, out);

    hr = get_audio_session(session_guid, This->device, 0, &session);
    if(FAILED(hr))
        return hr;

    wrapper = AudioSessionWrapper_Create(NULL);
    if(!wrapper)
        return E_OUTOFMEMORY;

    wrapper->session = session;

    *out = &wrapper->ISimpleAudioVolume_iface;

    return S_OK;
}

static HRESULT WINAPI AudioSessionManager_GetSessionEnumerator(
        IAudioSessionManager2 *iface, IAudioSessionEnumerator **out)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, out);
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionManager_RegisterSessionNotification(
        IAudioSessionManager2 *iface, IAudioSessionNotification *notification)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, notification);
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionManager_UnregisterSessionNotification(
        IAudioSessionManager2 *iface, IAudioSessionNotification *notification)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, notification);
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionManager_RegisterDuckNotification(
        IAudioSessionManager2 *iface, const WCHAR *session_id,
        IAudioVolumeDuckNotification *notification)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, notification);
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionManager_UnregisterDuckNotification(
        IAudioSessionManager2 *iface,
        IAudioVolumeDuckNotification *notification)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, notification);
    return E_NOTIMPL;
}

static const IAudioSessionManager2Vtbl AudioSessionManager2_Vtbl =
{
    AudioSessionManager_QueryInterface,
    AudioSessionManager_AddRef,
    AudioSessionManager_Release,
    AudioSessionManager_GetAudioSessionControl,
    AudioSessionManager_GetSimpleAudioVolume,
    AudioSessionManager_GetSessionEnumerator,
    AudioSessionManager_RegisterSessionNotification,
    AudioSessionManager_UnregisterSessionNotification,
    AudioSessionManager_RegisterDuckNotification,
    AudioSessionManager_UnregisterDuckNotification
};

HRESULT WINAPI AUDDRV_GetAudioSessionManager(IMMDevice *device,
        IAudioSessionManager2 **out)
{
    SessionMgr *This;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SessionMgr));
    if(!This)
        return E_OUTOFMEMORY;

    This->IAudioSessionManager2_iface.lpVtbl = &AudioSessionManager2_Vtbl;
    This->device = device;
    This->ref = 1;

    *out = &This->IAudioSessionManager2_iface;

    return S_OK;
}
