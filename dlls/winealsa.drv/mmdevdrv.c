/*
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
 * Copyright 2011 Andrew Eikum for CodeWeavers
 * Copyright 2022 Huw Davies
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

#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"
#include "wine/unixlib.h"

#include "propsys.h"
#include "initguid.h"
#include "ole2.h"
#include "propkey.h"
#include "mmdeviceapi.h"
#include "devpkey.h"
#include "mmsystem.h"
#include "dsound.h"

#include "initguid.h"
#include "endpointvolume.h"
#include "audioclient.h"
#include "audiopolicy.h"

#include <alsa/asoundlib.h>

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(alsa);

unixlib_handle_t alsa_handle = 0;

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
    IAudioClient3 IAudioClient3_iface;
    IAudioRenderClient IAudioRenderClient_iface;
    IAudioCaptureClient IAudioCaptureClient_iface;
    IAudioClock IAudioClock_iface;
    IAudioClock2 IAudioClock2_iface;
    IAudioStreamVolume IAudioStreamVolume_iface;

    LONG ref;

    IMMDevice *parent;
    IUnknown *pUnkFTMarshal;

    EDataFlow dataflow;
    float *vols;
    UINT32 channel_count;
    struct alsa_stream *stream;

    HANDLE timer;

    AudioSession *session;
    AudioSessionWrapper *session_wrapper;

    struct list entry;

    /* Keep at end */
    char alsa_name[1];
};

typedef struct _SessionMgr {
    IAudioSessionManager2 IAudioSessionManager2_iface;

    LONG ref;

    IMMDevice *device;
} SessionMgr;

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

static const WCHAR drv_key_devicesW[] = {'S','o','f','t','w','a','r','e','\\',
    'W','i','n','e','\\','D','r','i','v','e','r','s','\\',
    'w','i','n','e','a','l','s','a','.','d','r','v','\\','d','e','v','i','c','e','s',0};
static const WCHAR guidW[] = {'g','u','i','d',0};

static const IAudioClient3Vtbl AudioClient3_Vtbl;
static const IAudioRenderClientVtbl AudioRenderClient_Vtbl;
static const IAudioCaptureClientVtbl AudioCaptureClient_Vtbl;
static const IAudioSessionControl2Vtbl AudioSessionControl2_Vtbl;
static const ISimpleAudioVolumeVtbl SimpleAudioVolume_Vtbl;
static const IAudioClockVtbl AudioClock_Vtbl;
static const IAudioClock2Vtbl AudioClock2_Vtbl;
static const IAudioStreamVolumeVtbl AudioStreamVolume_Vtbl;
static const IChannelAudioVolumeVtbl ChannelAudioVolume_Vtbl;
static const IAudioSessionManager2Vtbl AudioSessionManager2_Vtbl;

static AudioSessionWrapper *AudioSessionWrapper_Create(ACImpl *client);

static inline ACImpl *impl_from_IAudioClient3(IAudioClient3 *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioClient3_iface);
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
        if(NtQueryVirtualMemory(GetCurrentProcess(), dll, MemoryWineUnixFuncs,
                                &alsa_handle, sizeof(alsa_handle), NULL))
            return FALSE;
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

static void alsa_lock(struct alsa_stream *stream)
{
    pthread_mutex_lock(&stream->lock);
}

static void alsa_unlock(struct alsa_stream *stream)
{
    pthread_mutex_unlock(&stream->lock);
}

static HRESULT alsa_stream_release(struct alsa_stream *stream)
{
    struct release_stream_params params;

    /* FIXME: to be moved with remap_channels() */
    HeapFree(GetProcessHeap(), 0, stream->remapping_buf);

    params.stream = stream;

    ALSA_CALL(release_stream, &params);

    return params.result;
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

static void get_device_guid(EDataFlow flow, const char *device, GUID *guid)
{
    HKEY key = NULL, dev_key;
    DWORD type, size = sizeof(*guid);
    WCHAR key_name[256];

    if(flow == eCapture)
        key_name[0] = '1';
    else
        key_name[0] = '0';
    key_name[1] = ',';
    MultiByteToWideChar(CP_UNIXCP, 0, device, -1, key_name + 2, ARRAY_SIZE(key_name) - 2);

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

static void set_stream_volumes(ACImpl *This)
{
    struct alsa_stream *stream = This->stream;
    unsigned int i;

    for(i = 0; i < stream->fmt->nChannels; i++)
        stream->vols[i] = This->session->mute ? 0.0f :
            This->vols[i] * This->session->channel_vols[i] * This->session->master_vol;
}

HRESULT WINAPI AUDDRV_GetEndpointIDs(EDataFlow flow, WCHAR ***ids_out, GUID **guids_out,
        UINT *num, UINT *def_index)
{
    struct get_endpoint_ids_params params;
    unsigned int i;
    GUID *guids = NULL;
    WCHAR **ids = NULL;

    TRACE("%d %p %p %p %p\n", flow, ids, guids, num, def_index);

    params.flow = flow;
    params.size = 1000;
    params.endpoints = NULL;
    do{
        HeapFree(GetProcessHeap(), 0, params.endpoints);
        params.endpoints = HeapAlloc(GetProcessHeap(), 0, params.size);
        ALSA_CALL(get_endpoint_ids, &params);
    }while(params.result == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

    if(FAILED(params.result)) goto end;

    ids = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, params.num * sizeof(*ids));
    guids = HeapAlloc(GetProcessHeap(), 0, params.num * sizeof(*guids));
    if(!ids || !guids){
        params.result = E_OUTOFMEMORY;
        goto end;
    }

    for(i = 0; i < params.num; i++){
        unsigned int size = (strlenW(params.endpoints[i].name) + 1) * sizeof(WCHAR);
        ids[i] = HeapAlloc(GetProcessHeap(), 0, size);
        if(!ids[i]){
            params.result = E_OUTOFMEMORY;
            goto end;
        }
        memcpy(ids[i], params.endpoints[i].name, size);
        get_device_guid(flow, params.endpoints[i].device, guids + i);
    }
    *def_index = params.default_idx;

end:
    HeapFree(GetProcessHeap(), 0, params.endpoints);
    if(FAILED(params.result)){
        HeapFree(GetProcessHeap(), 0, guids);
        if(ids){
            for(i = 0; i < params.num; i++)
                HeapFree(GetProcessHeap(), 0, ids[i]);
            HeapFree(GetProcessHeap(), 0, ids);
        }
    }else{
        *ids_out = ids;
        *guids_out = guids;
        *num = params.num;
    }

    return params.result;
}

static BOOL get_alsa_name_by_guid(GUID *guid, char *name, DWORD name_size, EDataFlow *flow)
{
    HKEY devices_key;
    UINT i = 0;
    WCHAR key_name[256];
    DWORD key_name_size;

    if(RegOpenKeyExW(HKEY_CURRENT_USER, drv_key_devicesW, 0, KEY_READ, &devices_key) != ERROR_SUCCESS){
        ERR("No devices found in registry?\n");
        return FALSE;
    }

    while(1){
        HKEY key;
        DWORD size, type;
        GUID reg_guid;

        key_name_size = ARRAY_SIZE(key_name);
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

                WideCharToMultiByte(CP_UNIXCP, 0, key_name + 2, -1, name, name_size, NULL, NULL);

                return TRUE;
            }
        }

        RegCloseKey(key);
    }

    RegCloseKey(devices_key);

    WARN("No matching device in registry for GUID %s\n", debugstr_guid(guid));

    return FALSE;
}

HRESULT WINAPI AUDDRV_GetAudioEndpoint(GUID *guid, IMMDevice *dev, IAudioClient **out)
{
    ACImpl *This;
    char alsa_name[256];
    EDataFlow dataflow;
    HRESULT hr;
    int len;

    TRACE("%s %p %p\n", debugstr_guid(guid), dev, out);

    if(!get_alsa_name_by_guid(guid, alsa_name, sizeof(alsa_name), &dataflow))
        return AUDCLNT_E_DEVICE_INVALIDATED;

    if(dataflow != eRender && dataflow != eCapture)
        return E_UNEXPECTED;

    len = strlen(alsa_name);
    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, offsetof(ACImpl, alsa_name[len + 1]));
    if(!This)
        return E_OUTOFMEMORY;

    This->IAudioClient3_iface.lpVtbl = &AudioClient3_Vtbl;
    This->IAudioRenderClient_iface.lpVtbl = &AudioRenderClient_Vtbl;
    This->IAudioCaptureClient_iface.lpVtbl = &AudioCaptureClient_Vtbl;
    This->IAudioClock_iface.lpVtbl = &AudioClock_Vtbl;
    This->IAudioClock2_iface.lpVtbl = &AudioClock2_Vtbl;
    This->IAudioStreamVolume_iface.lpVtbl = &AudioStreamVolume_Vtbl;

    hr = CoCreateFreeThreadedMarshaler((IUnknown *)&This->IAudioClient3_iface, &This->pUnkFTMarshal);
    if (FAILED(hr)) {
        HeapFree(GetProcessHeap(), 0, This);
        return hr;
    }

    This->dataflow = dataflow;
    memcpy(This->alsa_name, alsa_name, len + 1);

    This->parent = dev;
    IMMDevice_AddRef(This->parent);

    *out = (IAudioClient *)&This->IAudioClient3_iface;
    IAudioClient3_AddRef(&This->IAudioClient3_iface);

    return S_OK;
}

static HRESULT WINAPI AudioClient_QueryInterface(IAudioClient3 *iface,
        REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;
    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioClient) ||
            IsEqualIID(riid, &IID_IAudioClient2) ||
            IsEqualIID(riid, &IID_IAudioClient3))
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

static ULONG WINAPI AudioClient_AddRef(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    return ref;
}

static ULONG WINAPI AudioClient_Release(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    ULONG ref;

    ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    if(!ref){
        if(This->timer){
            HANDLE event;
            DWORD wait;
            event = CreateEventW(NULL, TRUE, FALSE, NULL);
            wait = !DeleteTimerQueueTimer(g_timer_q, This->timer, event);
            wait = wait && GetLastError() == ERROR_IO_PENDING;
            if(event && wait)
                WaitForSingleObject(event, INFINITE);
            CloseHandle(event);
        }

        IAudioClient3_Stop(iface);
        IMMDevice_Release(This->parent);
        IUnknown_Release(This->pUnkFTMarshal);
        if(This->session){
            EnterCriticalSection(&g_sessions_lock);
            list_remove(&This->entry);
            LeaveCriticalSection(&g_sessions_lock);
        }
        HeapFree(GetProcessHeap(), 0, This->vols);
        if (This->stream)
            alsa_stream_release(This->stream);
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

static void silence_buffer(struct alsa_stream *stream, BYTE *buffer, UINT32 frames)
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

static HRESULT WINAPI AudioClient_Initialize(IAudioClient3 *iface,
        AUDCLNT_SHAREMODE mode, DWORD flags, REFERENCE_TIME duration,
        REFERENCE_TIME period, const WAVEFORMATEX *fmt,
        const GUID *sessionguid)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct create_stream_params params;
    struct alsa_stream *stream;
    unsigned int i;

    TRACE("(%p)->(%x, %x, %s, %s, %p, %s)\n", This, mode, flags,
          wine_dbgstr_longlong(duration), wine_dbgstr_longlong(period), fmt, debugstr_guid(sessionguid));

    if(!fmt)
        return E_POINTER;

    if(mode != AUDCLNT_SHAREMODE_SHARED && mode != AUDCLNT_SHAREMODE_EXCLUSIVE)
        return E_INVALIDARG;

    if(flags & ~(AUDCLNT_STREAMFLAGS_CROSSPROCESS |
                AUDCLNT_STREAMFLAGS_LOOPBACK |
                AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
                AUDCLNT_STREAMFLAGS_NOPERSIST |
                AUDCLNT_STREAMFLAGS_RATEADJUST |
                AUDCLNT_SESSIONFLAGS_EXPIREWHENUNOWNED |
                AUDCLNT_SESSIONFLAGS_DISPLAY_HIDE |
                AUDCLNT_SESSIONFLAGS_DISPLAY_HIDEWHENEXPIRED |
                AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY |
                AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM)){
        FIXME("Unknown flags: %08x\n", flags);
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

    EnterCriticalSection(&g_sessions_lock);

    if(This->stream){
        LeaveCriticalSection(&g_sessions_lock);
        return AUDCLNT_E_ALREADY_INITIALIZED;
    }

    dump_fmt(fmt);

    params.alsa_name = This->alsa_name;
    params.flow = This->dataflow;
    params.share = mode;
    params.flags = flags;
    params.duration = duration;
    params.period = period;
    params.fmt = fmt;
    params.stream = &stream;

    ALSA_CALL(create_stream, &params);
    if(FAILED(params.result)){
        LeaveCriticalSection(&g_sessions_lock);
        return params.result;
    }

    This->channel_count = fmt->nChannels;
    This->vols = HeapAlloc(GetProcessHeap(), 0, This->channel_count * sizeof(float));
    if(!This->vols){
        params.result = E_OUTOFMEMORY;
        goto exit;
    }
    for(i = 0; i < This->channel_count; ++i)
        This->vols[i] = 1.f;

    params.result = get_audio_session(sessionguid, This->parent, This->channel_count,
                                      &This->session);
    if(FAILED(params.result))
        goto exit;

    list_add_tail(&This->session->clients, &This->entry);

exit:
    if(FAILED(params.result)){
        alsa_stream_release(stream);
        HeapFree(GetProcessHeap(), 0, This->vols);
        This->vols = NULL;
    }else{
        This->stream = stream;
        set_stream_volumes(This);
    }

    LeaveCriticalSection(&g_sessions_lock);

    return params.result;
}

static HRESULT WINAPI AudioClient_GetBufferSize(IAudioClient3 *iface,
        UINT32 *out)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct get_buffer_size_params params;

    TRACE("(%p)->(%p)\n", This, out);

    if(!out)
        return E_POINTER;

    if(!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    params.size = out;

    ALSA_CALL(get_buffer_size, &params);

    return params.result;
}

static HRESULT WINAPI AudioClient_GetStreamLatency(IAudioClient3 *iface,
        REFERENCE_TIME *latency)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct get_latency_params params;

    TRACE("(%p)->(%p)\n", This, latency);

    if(!latency)
        return E_POINTER;

    if(!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    params.latency = latency;

    ALSA_CALL(get_latency, &params);

    return params.result;
}

static HRESULT WINAPI AudioClient_GetCurrentPadding(IAudioClient3 *iface,
        UINT32 *out)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct get_current_padding_params params;

    TRACE("(%p)->(%p)\n", This, out);

    if(!out)
        return E_POINTER;

    if(!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    params.padding = out;

    ALSA_CALL(get_current_padding, &params);

    TRACE("pad: %u\n", *out);

    return params.result;
}

static HRESULT WINAPI AudioClient_IsFormatSupported(IAudioClient3 *iface,
        AUDCLNT_SHAREMODE mode, const WAVEFORMATEX *fmt,
        WAVEFORMATEX **out)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct is_format_supported_params params;

    TRACE("(%p)->(%x, %p, %p)\n", This, mode, fmt, out);
    if(fmt) dump_fmt(fmt);

    params.alsa_name = This->alsa_name;
    params.flow = This->dataflow;
    params.share = mode;
    params.fmt_in = fmt;
    params.fmt_out = NULL;

    if(out){
        *out = NULL;
        if(mode == AUDCLNT_SHAREMODE_SHARED)
            params.fmt_out = CoTaskMemAlloc(sizeof(*params.fmt_out));
    }
    ALSA_CALL(is_format_supported, &params);

    if(params.result == S_FALSE)
        *out = &params.fmt_out->Format;
    else
        CoTaskMemFree(params.fmt_out);

    return params.result;
}

static HRESULT WINAPI AudioClient_GetMixFormat(IAudioClient3 *iface,
        WAVEFORMATEX **pwfx)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct get_mix_format_params params;

    TRACE("(%p)->(%p)\n", This, pwfx);

    if(!pwfx)
        return E_POINTER;
    *pwfx = NULL;

    params.alsa_name = This->alsa_name;
    params.flow = This->dataflow;
    params.fmt = CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
    if(!params.fmt)
        return E_OUTOFMEMORY;

    ALSA_CALL(get_mix_format, &params);

    if(SUCCEEDED(params.result)){
        *pwfx = &params.fmt->Format;
        dump_fmt(*pwfx);
    } else
        CoTaskMemFree(params.fmt);

    return params.result;
}

static HRESULT WINAPI AudioClient_GetDevicePeriod(IAudioClient3 *iface,
        REFERENCE_TIME *defperiod, REFERENCE_TIME *minperiod)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(%p, %p)\n", This, defperiod, minperiod);

    if(!defperiod && !minperiod)
        return E_POINTER;

    if(defperiod)
        *defperiod = DefaultPeriod;
    if(minperiod)
        *minperiod = DefaultPeriod;

    return S_OK;
}

static BYTE *remap_channels(struct alsa_stream *stream, BYTE *buf, snd_pcm_uframes_t frames)
{
    snd_pcm_uframes_t i;
    UINT c;
    UINT bytes_per_sample = stream->fmt->wBitsPerSample / 8;

    if(!stream->need_remapping)
        return buf;

    if(!stream->remapping_buf){
        stream->remapping_buf = HeapAlloc(GetProcessHeap(), 0,
                bytes_per_sample * stream->alsa_channels * frames);
        stream->remapping_buf_frames = frames;
    }else if(stream->remapping_buf_frames < frames){
        stream->remapping_buf = HeapReAlloc(GetProcessHeap(), 0, stream->remapping_buf,
                bytes_per_sample * stream->alsa_channels * frames);
        stream->remapping_buf_frames = frames;
    }

    snd_pcm_format_set_silence(stream->alsa_format, stream->remapping_buf,
            frames * stream->alsa_channels);

    switch(stream->fmt->wBitsPerSample){
    case 8: {
            UINT8 *tgt_buf, *src_buf;
            tgt_buf = stream->remapping_buf;
            src_buf = buf;
            for(i = 0; i < frames; ++i){
                for(c = 0; c < stream->fmt->nChannels; ++c)
                    tgt_buf[stream->alsa_channel_map[c]] = src_buf[c];
                tgt_buf += stream->alsa_channels;
                src_buf += stream->fmt->nChannels;
            }
            break;
        }
    case 16: {
            UINT16 *tgt_buf, *src_buf;
            tgt_buf = (UINT16*)stream->remapping_buf;
            src_buf = (UINT16*)buf;
            for(i = 0; i < frames; ++i){
                for(c = 0; c < stream->fmt->nChannels; ++c)
                    tgt_buf[stream->alsa_channel_map[c]] = src_buf[c];
                tgt_buf += stream->alsa_channels;
                src_buf += stream->fmt->nChannels;
            }
        }
        break;
    case 32: {
            UINT32 *tgt_buf, *src_buf;
            tgt_buf = (UINT32*)stream->remapping_buf;
            src_buf = (UINT32*)buf;
            for(i = 0; i < frames; ++i){
                for(c = 0; c < stream->fmt->nChannels; ++c)
                    tgt_buf[stream->alsa_channel_map[c]] = src_buf[c];
                tgt_buf += stream->alsa_channels;
                src_buf += stream->fmt->nChannels;
            }
        }
        break;
    default: {
            BYTE *tgt_buf, *src_buf;
            tgt_buf = stream->remapping_buf;
            src_buf = buf;
            for(i = 0; i < frames; ++i){
                for(c = 0; c < stream->fmt->nChannels; ++c)
                    memcpy(&tgt_buf[stream->alsa_channel_map[c] * bytes_per_sample],
                            &src_buf[c * bytes_per_sample], bytes_per_sample);
                tgt_buf += stream->alsa_channels * bytes_per_sample;
                src_buf += stream->fmt->nChannels * bytes_per_sample;
            }
        }
        break;
    }

    return stream->remapping_buf;
}

static void adjust_buffer_volume(const struct alsa_stream *stream, BYTE *buf, snd_pcm_uframes_t frames)
{
    BOOL adjust = FALSE;
    UINT32 i, channels, mute = 0;
    BYTE *end;

    if (stream->vol_adjusted_frames >= frames)
        return;
    channels = stream->fmt->nChannels;

    /* Adjust the buffer based on the volume for each channel */
    for (i = 0; i < channels; i++)
    {
        adjust |= stream->vols[i] != 1.0f;
        if (stream->vols[i] == 0.0f)
            mute++;
    }

    if (mute == channels)
    {
        int err = snd_pcm_format_set_silence(stream->alsa_format, buf, frames * channels);
        if (err < 0)
            WARN("Setting buffer to silence failed: %d (%s)\n", err, snd_strerror(err));
        return;
    }
    if (!adjust) return;

    /* Skip the frames we've already adjusted before */
    end = buf + frames * stream->fmt->nBlockAlign;
    buf += stream->vol_adjusted_frames * stream->fmt->nBlockAlign;

    switch (stream->alsa_format)
    {
#ifndef WORDS_BIGENDIAN
#define PROCESS_BUFFER(type) do         \
{                                       \
    type *p = (type*)buf;               \
    do                                  \
    {                                   \
        for (i = 0; i < channels; i++)  \
            p[i] = p[i] * stream->vols[i];       \
        p += i;                         \
    } while ((BYTE*)p != end);          \
} while (0)
    case SND_PCM_FORMAT_S16_LE:
        PROCESS_BUFFER(INT16);
        break;
    case SND_PCM_FORMAT_S32_LE:
        PROCESS_BUFFER(INT32);
        break;
    case SND_PCM_FORMAT_FLOAT_LE:
        PROCESS_BUFFER(float);
        break;
    case SND_PCM_FORMAT_FLOAT64_LE:
        PROCESS_BUFFER(double);
        break;
#undef PROCESS_BUFFER
    case SND_PCM_FORMAT_S20_3LE:
    case SND_PCM_FORMAT_S24_3LE:
    {
        /* Do it 12 bytes at a time until it is no longer possible */
        UINT32 *q = (UINT32*)buf, mask = ~0xff;
        BYTE *p;

        /* After we adjust the volume, we need to mask out low bits */
        if (stream->alsa_format == SND_PCM_FORMAT_S20_3LE)
            mask = ~0x0fff;

        i = 0;
        while (end - (BYTE*)q >= 12)
        {
            UINT32 v[4], k;
            v[0] = q[0] << 8;
            v[1] = q[1] << 16 | (q[0] >> 16 & ~0xff);
            v[2] = q[2] << 24 | (q[1] >> 8  & ~0xff);
            v[3] = q[2] & ~0xff;
            for (k = 0; k < 4; k++)
            {
                v[k] = (INT32)((INT32)v[k] * stream->vols[i]);
                v[k] &= mask;
                if (++i == channels) i = 0;
            }
            *q++ = v[0] >> 8  | v[1] << 16;
            *q++ = v[1] >> 16 | v[2] << 8;
            *q++ = v[2] >> 24 | v[3];
        }
        p = (BYTE*)q;
        while (p != end)
        {
            UINT32 v = (INT32)((INT32)(p[0] << 8 | p[1] << 16 | p[2] << 24) * stream->vols[i]);
            v &= mask;
            *p++ = v >> 8  & 0xff;
            *p++ = v >> 16 & 0xff;
            *p++ = v >> 24;
            if (++i == channels) i = 0;
        }
        break;
    }
#endif
    case SND_PCM_FORMAT_U8:
    {
        UINT8 *p = (UINT8*)buf;
        do
        {
            for (i = 0; i < channels; i++)
                p[i] = (int)((p[i] - 128) * stream->vols[i]) + 128;
            p += i;
        } while ((BYTE*)p != end);
        break;
    }
    default:
        TRACE("Unhandled format %i, not adjusting volume.\n", stream->alsa_format);
        break;
    }
}

static snd_pcm_sframes_t alsa_write_best_effort(struct alsa_stream *stream, BYTE *buf, snd_pcm_uframes_t frames)
{
    snd_pcm_sframes_t written;

    adjust_buffer_volume(stream, buf, frames);

    /* Mark the frames we've already adjusted */
    if (stream->vol_adjusted_frames < frames)
        stream->vol_adjusted_frames = frames;

    buf = remap_channels(stream, buf, frames);

    written = snd_pcm_writei(stream->pcm_handle, buf, frames);
    if(written < 0){
        int ret;

        if(written == -EAGAIN)
            /* buffer full */
            return 0;

        WARN("writei failed, recovering: %ld (%s)\n", written,
                snd_strerror(written));

        ret = snd_pcm_recover(stream->pcm_handle, written, 0);
        if(ret < 0){
            WARN("Could not recover: %d (%s)\n", ret, snd_strerror(ret));
            return ret;
        }

        written = snd_pcm_writei(stream->pcm_handle, buf, frames);
    }

    if (written > 0)
        stream->vol_adjusted_frames -= written;
    return written;
}

static snd_pcm_sframes_t alsa_write_buffer_wrap(struct alsa_stream *stream, BYTE *buf,
        snd_pcm_uframes_t buflen, snd_pcm_uframes_t offs,
        snd_pcm_uframes_t to_write)
{
    snd_pcm_sframes_t ret = 0;

    while(to_write){
        snd_pcm_uframes_t chunk;
        snd_pcm_sframes_t tmp;

        if(offs + to_write > buflen)
            chunk = buflen - offs;
        else
            chunk = to_write;

        tmp = alsa_write_best_effort(stream, buf + offs * stream->fmt->nBlockAlign, chunk);
        if(tmp < 0)
            return ret;
        if(!tmp)
            break;

        ret += tmp;
        to_write -= tmp;
        offs += tmp;
        offs %= buflen;
    }

    return ret;
}

static UINT buf_ptr_diff(UINT left, UINT right, UINT bufsize)
{
    if(left <= right)
        return right - left;
    return bufsize - (left - right);
}

static UINT data_not_in_alsa(struct alsa_stream *stream)
{
    UINT32 diff;

    diff = buf_ptr_diff(stream->lcl_offs_frames, stream->wri_offs_frames, stream->bufsize_frames);
    if(diff)
        return diff;

    return stream->held_frames - stream->data_in_alsa_frames;
}
/* Here's the buffer setup:
 *
 *  vvvvvvvv sent to HW already
 *          vvvvvvvv in ALSA buffer but rewindable
 * [dddddddddddddddd] ALSA buffer
 *         [dddddddddddddddd--------] mmdevapi buffer
 *          ^^^^^^^^ data_in_alsa_frames
 *          ^^^^^^^^^^^^^^^^ held_frames
 *                  ^ lcl_offs_frames
 *                          ^ wri_offs_frames
 *
 * GetCurrentPadding is held_frames
 *
 * During period callback, we decrement held_frames, fill ALSA buffer, and move
 *   lcl_offs forward
 *
 * During Stop, we rewind the ALSA buffer
 */
static void alsa_write_data(struct alsa_stream *stream)
{
    snd_pcm_sframes_t written;
    snd_pcm_uframes_t avail, max_copy_frames, data_frames_played;
    int err;

    /* this call seems to be required to get an accurate snd_pcm_state() */
    avail = snd_pcm_avail_update(stream->pcm_handle);

    if(snd_pcm_state(stream->pcm_handle) == SND_PCM_STATE_XRUN){
        TRACE("XRun state, recovering\n");

        avail = stream->alsa_bufsize_frames;

        if((err = snd_pcm_recover(stream->pcm_handle, -EPIPE, 1)) < 0)
            WARN("snd_pcm_recover failed: %d (%s)\n", err, snd_strerror(err));

        if((err = snd_pcm_reset(stream->pcm_handle)) < 0)
            WARN("snd_pcm_reset failed: %d (%s)\n", err, snd_strerror(err));

        if((err = snd_pcm_prepare(stream->pcm_handle)) < 0)
            WARN("snd_pcm_prepare failed: %d (%s)\n", err, snd_strerror(err));
    }

    TRACE("avail: %ld\n", avail);

    /* Add a lead-in when starting with too few frames to ensure
     * continuous rendering.  Additional benefit: Force ALSA to start. */
    if(stream->data_in_alsa_frames == 0 && stream->held_frames < stream->alsa_period_frames)
    {
        alsa_write_best_effort(stream, stream->silence_buf,
                               stream->alsa_period_frames - stream->held_frames);
        stream->vol_adjusted_frames = 0;
    }

    if(stream->started)
        max_copy_frames = data_not_in_alsa(stream);
    else
        max_copy_frames = 0;

    data_frames_played = min(stream->data_in_alsa_frames, avail);
    stream->data_in_alsa_frames -= data_frames_played;

    if(stream->held_frames > data_frames_played){
        if(stream->started)
            stream->held_frames -= data_frames_played;
    }else
        stream->held_frames = 0;

    while(avail && max_copy_frames){
        snd_pcm_uframes_t to_write;

        to_write = min(avail, max_copy_frames);

        written = alsa_write_buffer_wrap(stream, stream->local_buffer,
                stream->bufsize_frames, stream->lcl_offs_frames, to_write);
        if(written <= 0)
            break;

        avail -= written;
        stream->lcl_offs_frames += written;
        stream->lcl_offs_frames %= stream->bufsize_frames;
        stream->data_in_alsa_frames += written;
        max_copy_frames -= written;
    }

    if(stream->event)
        SetEvent(stream->event);
}

static void alsa_read_data(struct alsa_stream *stream)
{
    snd_pcm_sframes_t nread;
    UINT32 pos = stream->wri_offs_frames, limit = stream->held_frames;
    unsigned int i;

    if(!stream->started)
        goto exit;

    /* FIXME: Detect overrun and signal DATA_DISCONTINUITY
     * How to count overrun frames and report them as position increase? */
    limit = stream->bufsize_frames - max(limit, pos);

    nread = snd_pcm_readi(stream->pcm_handle,
            stream->local_buffer + pos * stream->fmt->nBlockAlign, limit);
    TRACE("read %ld from %u limit %u\n", nread, pos, limit);
    if(nread < 0){
        int ret;

        if(nread == -EAGAIN) /* no data yet */
            return;

        WARN("read failed, recovering: %ld (%s)\n", nread, snd_strerror(nread));

        ret = snd_pcm_recover(stream->pcm_handle, nread, 0);
        if(ret < 0){
            WARN("Recover failed: %d (%s)\n", ret, snd_strerror(ret));
            return;
        }

        nread = snd_pcm_readi(stream->pcm_handle,
                stream->local_buffer + pos * stream->fmt->nBlockAlign, limit);
        if(nread < 0){
            WARN("read failed: %ld (%s)\n", nread, snd_strerror(nread));
            return;
        }
    }

    for(i = 0; i < stream->fmt->nChannels; i++)
        if(stream->vols[i] != 0.0f)
            break;
    if(i == stream->fmt->nChannels){ /* mute */
        int err;
        if((err = snd_pcm_format_set_silence(stream->alsa_format,
                        stream->local_buffer + pos * stream->fmt->nBlockAlign,
                        nread)) < 0)
            WARN("Setting buffer to silence failed: %d (%s)\n", err,
                    snd_strerror(err));
    }

    stream->wri_offs_frames += nread;
    stream->wri_offs_frames %= stream->bufsize_frames;
    stream->held_frames += nread;

exit:
    if(stream->event)
        SetEvent(stream->event);
}

static void CALLBACK alsa_push_buffer_data(void *user, BOOLEAN timer)
{
    ACImpl *This = user;
    struct alsa_stream *stream = This->stream;

    alsa_lock(stream);

    QueryPerformanceCounter(&stream->last_period_time);

    if(stream->flow == eRender)
        alsa_write_data(stream);
    else if(stream->flow == eCapture)
        alsa_read_data(stream);

    alsa_unlock(stream);
}

static snd_pcm_uframes_t interp_elapsed_frames(struct alsa_stream *stream)
{
    LARGE_INTEGER time_freq, current_time, time_diff;
    QueryPerformanceFrequency(&time_freq);
    QueryPerformanceCounter(&current_time);
    time_diff.QuadPart = current_time.QuadPart - stream->last_period_time.QuadPart;
    return MulDiv(time_diff.QuadPart, stream->fmt->nSamplesPerSec, time_freq.QuadPart);
}

static int alsa_rewind_best_effort(ACImpl *This)
{
    struct alsa_stream *stream = This->stream;
    snd_pcm_uframes_t len, leave;

    /* we can't use snd_pcm_rewindable, some PCM devices crash. so follow
     * PulseAudio's example and rewind as much data as we believe is in the
     * buffer, minus 1.33ms for safety. */

    /* amount of data to leave in ALSA buffer */
    leave = interp_elapsed_frames(stream) + stream->safe_rewind_frames;

    if(stream->held_frames < leave)
        stream->held_frames = 0;
    else
        stream->held_frames -= leave;

    if(stream->data_in_alsa_frames < leave)
        len = 0;
    else
        len = stream->data_in_alsa_frames - leave;

    TRACE("rewinding %lu frames, now held %u\n", len, stream->held_frames);

    if(len)
        /* snd_pcm_rewind return value is often broken, assume it succeeded */
        snd_pcm_rewind(stream->pcm_handle, len);

    stream->data_in_alsa_frames = 0;

    return len;
}

static HRESULT WINAPI AudioClient_Start(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct alsa_stream *stream = This->stream;

    TRACE("(%p)\n", This);

    EnterCriticalSection(&g_sessions_lock);

    if(!This->stream){
        LeaveCriticalSection(&g_sessions_lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    alsa_lock(stream);

    if((stream->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK) && !stream->event){
        alsa_unlock(stream);
        LeaveCriticalSection(&g_sessions_lock);
        return AUDCLNT_E_EVENTHANDLE_NOT_SET;
    }

    if(stream->started){
        alsa_unlock(stream);
        LeaveCriticalSection(&g_sessions_lock);
        return AUDCLNT_E_NOT_STOPPED;
    }

    if(stream->flow == eCapture){
        /* dump any data that might be leftover in the ALSA capture buffer */
        snd_pcm_readi(stream->pcm_handle, stream->local_buffer,
                stream->bufsize_frames);
    }else{
        snd_pcm_sframes_t avail, written;
        snd_pcm_uframes_t offs;

        avail = snd_pcm_avail_update(stream->pcm_handle);
        avail = min(avail, stream->held_frames);

        if(stream->wri_offs_frames < stream->held_frames)
            offs = stream->bufsize_frames - stream->held_frames + stream->wri_offs_frames;
        else
            offs = stream->wri_offs_frames - stream->held_frames;

        /* fill it with data */
        written = alsa_write_buffer_wrap(stream, stream->local_buffer,
                stream->bufsize_frames, offs, avail);

        if(written > 0){
            stream->lcl_offs_frames = (offs + written) % stream->bufsize_frames;
            stream->data_in_alsa_frames = written;
        }else{
            stream->lcl_offs_frames = offs;
            stream->data_in_alsa_frames = 0;
        }
    }

    if(!This->timer){
        if(!CreateTimerQueueTimer(&This->timer, g_timer_q, alsa_push_buffer_data,
                This, 0, stream->mmdev_period_rt / 10000, WT_EXECUTEINTIMERTHREAD)){
            alsa_unlock(stream);
            LeaveCriticalSection(&g_sessions_lock);
            WARN("Unable to create timer: %u\n", GetLastError());
            return E_OUTOFMEMORY;
        }
    }

    stream->started = TRUE;

    alsa_unlock(stream);
    LeaveCriticalSection(&g_sessions_lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_Stop(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct alsa_stream *stream = This->stream;

    TRACE("(%p)\n", This);

    if(!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    alsa_lock(stream);

    if(!stream->started){
        alsa_unlock(stream);
        return S_FALSE;
    }

    if(stream->flow == eRender)
        alsa_rewind_best_effort(This);

    stream->started = FALSE;

    alsa_unlock(stream);

    return S_OK;
}

static HRESULT WINAPI AudioClient_Reset(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct alsa_stream *stream = This->stream;

    TRACE("(%p)\n", This);

    if(!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    alsa_lock(stream);

    if(stream->started){
        alsa_unlock(stream);
        return AUDCLNT_E_NOT_STOPPED;
    }

    if(stream->getbuf_last){
        alsa_unlock(stream);
        return AUDCLNT_E_BUFFER_OPERATION_PENDING;
    }

    if(snd_pcm_drop(stream->pcm_handle) < 0)
        WARN("snd_pcm_drop failed\n");

    if(snd_pcm_reset(stream->pcm_handle) < 0)
        WARN("snd_pcm_reset failed\n");

    if(snd_pcm_prepare(stream->pcm_handle) < 0)
        WARN("snd_pcm_prepare failed\n");

    if(stream->flow == eRender){
        stream->written_frames = 0;
        stream->last_pos_frames = 0;
    }else{
        stream->written_frames += stream->held_frames;
    }
    stream->held_frames = 0;
    stream->lcl_offs_frames = 0;
    stream->wri_offs_frames = 0;

    alsa_unlock(stream);

    return S_OK;
}

static HRESULT WINAPI AudioClient_SetEventHandle(IAudioClient3 *iface,
        HANDLE event)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct alsa_stream *stream = This->stream;

    TRACE("(%p)->(%p)\n", This, event);

    if(!event)
        return E_INVALIDARG;

    if(!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    alsa_lock(stream);

    if(!(stream->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK)){
        alsa_unlock(stream);
        return AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED;
    }

    if (stream->event){
        alsa_unlock(stream);
        FIXME("called twice\n");
        return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
    }

    stream->event = event;

    alsa_unlock(stream);

    return S_OK;
}

static HRESULT WINAPI AudioClient_GetService(IAudioClient3 *iface, REFIID riid,
        void **ppv)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    EnterCriticalSection(&g_sessions_lock);

    if(!This->stream){
        LeaveCriticalSection(&g_sessions_lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if(IsEqualIID(riid, &IID_IAudioRenderClient)){
        if(This->dataflow != eRender){
            LeaveCriticalSection(&g_sessions_lock);
            return AUDCLNT_E_WRONG_ENDPOINT_TYPE;
        }
        IAudioRenderClient_AddRef(&This->IAudioRenderClient_iface);
        *ppv = &This->IAudioRenderClient_iface;
    }else if(IsEqualIID(riid, &IID_IAudioCaptureClient)){
        if(This->dataflow != eCapture){
            LeaveCriticalSection(&g_sessions_lock);
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
                LeaveCriticalSection(&g_sessions_lock);
                return E_OUTOFMEMORY;
            }
        }else
            IAudioSessionControl2_AddRef(&This->session_wrapper->IAudioSessionControl2_iface);

        *ppv = &This->session_wrapper->IAudioSessionControl2_iface;
    }else if(IsEqualIID(riid, &IID_IChannelAudioVolume)){
        if(!This->session_wrapper){
            This->session_wrapper = AudioSessionWrapper_Create(This);
            if(!This->session_wrapper){
                LeaveCriticalSection(&g_sessions_lock);
                return E_OUTOFMEMORY;
            }
        }else
            IChannelAudioVolume_AddRef(&This->session_wrapper->IChannelAudioVolume_iface);

        *ppv = &This->session_wrapper->IChannelAudioVolume_iface;
    }else if(IsEqualIID(riid, &IID_ISimpleAudioVolume)){
        if(!This->session_wrapper){
            This->session_wrapper = AudioSessionWrapper_Create(This);
            if(!This->session_wrapper){
                LeaveCriticalSection(&g_sessions_lock);
                return E_OUTOFMEMORY;
            }
        }else
            ISimpleAudioVolume_AddRef(&This->session_wrapper->ISimpleAudioVolume_iface);

        *ppv = &This->session_wrapper->ISimpleAudioVolume_iface;
    }

    if(*ppv){
        LeaveCriticalSection(&g_sessions_lock);
        return S_OK;
    }

    LeaveCriticalSection(&g_sessions_lock);

    FIXME("stub %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static HRESULT WINAPI AudioClient_IsOffloadCapable(IAudioClient3 *iface,
        AUDIO_STREAM_CATEGORY category, BOOL *offload_capable)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(0x%x, %p)\n", This, category, offload_capable);

    if(!offload_capable)
        return E_INVALIDARG;

    *offload_capable = FALSE;

    return S_OK;
}

static HRESULT WINAPI AudioClient_SetClientProperties(IAudioClient3 *iface,
        const AudioClientProperties *prop)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    const Win8AudioClientProperties *legacy_prop = (const Win8AudioClientProperties *)prop;

    TRACE("(%p)->(%p)\n", This, prop);

    if(!legacy_prop)
        return E_POINTER;

    if(legacy_prop->cbSize == sizeof(AudioClientProperties)){
        TRACE("{ bIsOffload: %u, eCategory: 0x%x, Options: 0x%x }\n",
                legacy_prop->bIsOffload,
                legacy_prop->eCategory,
                prop->Options);
    }else if(legacy_prop->cbSize == sizeof(Win8AudioClientProperties)){
        TRACE("{ bIsOffload: %u, eCategory: 0x%x }\n",
                legacy_prop->bIsOffload,
                legacy_prop->eCategory);
    }else{
        WARN("Unsupported Size = %d\n", legacy_prop->cbSize);
        return E_INVALIDARG;
    }


    if(legacy_prop->bIsOffload)
        return AUDCLNT_E_ENDPOINT_OFFLOAD_NOT_CAPABLE;

    return S_OK;
}

static HRESULT WINAPI AudioClient_GetBufferSizeLimits(IAudioClient3 *iface,
        const WAVEFORMATEX *format, BOOL event_driven, REFERENCE_TIME *min_duration,
        REFERENCE_TIME *max_duration)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    FIXME("(%p)->(%p, %u, %p, %p)\n", This, format, event_driven, min_duration, max_duration);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioClient_GetSharedModeEnginePeriod(IAudioClient3 *iface,
        const WAVEFORMATEX *format, UINT32 *default_period_frames, UINT32 *unit_period_frames,
        UINT32 *min_period_frames, UINT32 *max_period_frames)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    FIXME("(%p)->(%p, %p, %p, %p, %p)\n", This, format, default_period_frames, unit_period_frames,
            min_period_frames, max_period_frames);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioClient_GetCurrentSharedModeEnginePeriod(IAudioClient3 *iface,
        WAVEFORMATEX **cur_format, UINT32 *cur_period_frames)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    FIXME("(%p)->(%p, %p)\n", This, cur_format, cur_period_frames);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioClient_InitializeSharedAudioStream(IAudioClient3 *iface,
        DWORD flags, UINT32 period_frames, const WAVEFORMATEX *format,
        const GUID *session_guid)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    FIXME("(%p)->(0x%x, %u, %p, %s)\n", This, flags, period_frames, format, debugstr_guid(session_guid));

    return E_NOTIMPL;
}

static const IAudioClient3Vtbl AudioClient3_Vtbl =
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
    AudioClient_GetService,
    AudioClient_IsOffloadCapable,
    AudioClient_SetClientProperties,
    AudioClient_GetBufferSizeLimits,
    AudioClient_GetSharedModeEnginePeriod,
    AudioClient_GetCurrentSharedModeEnginePeriod,
    AudioClient_InitializeSharedAudioStream,
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
    return AudioClient_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI AudioRenderClient_Release(IAudioRenderClient *iface)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    return AudioClient_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI AudioRenderClient_GetBuffer(IAudioRenderClient *iface,
        UINT32 frames, BYTE **data)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    struct alsa_stream *stream = This->stream;
    UINT32 write_pos;
    SIZE_T size;

    TRACE("(%p)->(%u, %p)\n", This, frames, data);

    if(!data)
        return E_POINTER;
    *data = NULL;

    alsa_lock(stream);

    if(stream->getbuf_last){
        alsa_unlock(stream);
        return AUDCLNT_E_OUT_OF_ORDER;
    }

    if(!frames){
        alsa_unlock(stream);
        return S_OK;
    }

    /* held_frames == GetCurrentPadding_nolock(); */
    if(stream->held_frames + frames > stream->bufsize_frames){
        alsa_unlock(stream);
        return AUDCLNT_E_BUFFER_TOO_LARGE;
    }

    write_pos = stream->wri_offs_frames;
    if(write_pos + frames > stream->bufsize_frames){
        if(stream->tmp_buffer_frames < frames){
            if(stream->tmp_buffer){
                size = 0;
                NtFreeVirtualMemory(GetCurrentProcess(), (void **)&stream->tmp_buffer, &size, MEM_RELEASE);
                stream->tmp_buffer = NULL;
            }
            size = frames * stream->fmt->nBlockAlign;
            if(NtAllocateVirtualMemory(GetCurrentProcess(), (void **)&stream->tmp_buffer, 0, &size,
                                       MEM_COMMIT, PAGE_READWRITE)){
                stream->tmp_buffer_frames = 0;
                alsa_unlock(stream);
                return E_OUTOFMEMORY;
            }
            stream->tmp_buffer_frames = frames;
        }
        *data = stream->tmp_buffer;
        stream->getbuf_last = -frames;
    }else{
        *data = stream->local_buffer + write_pos * stream->fmt->nBlockAlign;
        stream->getbuf_last = frames;
    }

    silence_buffer(stream, *data, frames);

    alsa_unlock(stream);

    return S_OK;
}

static void alsa_wrap_buffer(struct alsa_stream *stream, BYTE *buffer, UINT32 written_frames)
{
    snd_pcm_uframes_t write_offs_frames = stream->wri_offs_frames;
    UINT32 write_offs_bytes = write_offs_frames * stream->fmt->nBlockAlign;
    snd_pcm_uframes_t chunk_frames = stream->bufsize_frames - write_offs_frames;
    UINT32 chunk_bytes = chunk_frames * stream->fmt->nBlockAlign;
    UINT32 written_bytes = written_frames * stream->fmt->nBlockAlign;

    if(written_bytes <= chunk_bytes){
        memcpy(stream->local_buffer + write_offs_bytes, buffer, written_bytes);
    }else{
        memcpy(stream->local_buffer + write_offs_bytes, buffer, chunk_bytes);
        memcpy(stream->local_buffer, buffer + chunk_bytes,
                written_bytes - chunk_bytes);
    }
}

static HRESULT WINAPI AudioRenderClient_ReleaseBuffer(
        IAudioRenderClient *iface, UINT32 written_frames, DWORD flags)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    struct alsa_stream *stream = This->stream;
    BYTE *buffer;

    TRACE("(%p)->(%u, %x)\n", This, written_frames, flags);

    alsa_lock(stream);

    if(!written_frames){
        stream->getbuf_last = 0;
        alsa_unlock(stream);
        return S_OK;
    }

    if(!stream->getbuf_last){
        alsa_unlock(stream);
        return AUDCLNT_E_OUT_OF_ORDER;
    }

    if(written_frames > (stream->getbuf_last >= 0 ? stream->getbuf_last : -stream->getbuf_last)){
        alsa_unlock(stream);
        return AUDCLNT_E_INVALID_SIZE;
    }

    if(stream->getbuf_last >= 0)
        buffer = stream->local_buffer + stream->wri_offs_frames * stream->fmt->nBlockAlign;
    else
        buffer = stream->tmp_buffer;

    if(flags & AUDCLNT_BUFFERFLAGS_SILENT)
        silence_buffer(stream, buffer, written_frames);

    if(stream->getbuf_last < 0)
        alsa_wrap_buffer(stream, buffer, written_frames);

    stream->wri_offs_frames += written_frames;
    stream->wri_offs_frames %= stream->bufsize_frames;
    stream->held_frames += written_frames;
    stream->written_frames += written_frames;
    stream->getbuf_last = 0;

    alsa_unlock(stream);

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
    return IAudioClient3_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI AudioCaptureClient_Release(IAudioCaptureClient *iface)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    return IAudioClient3_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI AudioCaptureClient_GetBuffer(IAudioCaptureClient *iface,
        BYTE **data, UINT32 *frames, DWORD *flags, UINT64 *devpos,
        UINT64 *qpcpos)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    struct alsa_stream *stream = This->stream;
    SIZE_T size;

    TRACE("(%p)->(%p, %p, %p, %p, %p)\n", This, data, frames, flags,
            devpos, qpcpos);

    if(!data)
        return E_POINTER;

    *data = NULL;

    if(!frames || !flags)
        return E_POINTER;

    alsa_lock(stream);

    if(stream->getbuf_last){
        alsa_unlock(stream);
        return AUDCLNT_E_OUT_OF_ORDER;
    }

    /* hr = GetNextPacketSize(iface, frames); */
    if(stream->held_frames < stream->mmdev_period_frames){
        *frames = 0;
        alsa_unlock(stream);
        return AUDCLNT_S_BUFFER_EMPTY;
    }
    *frames = stream->mmdev_period_frames;

    if(stream->lcl_offs_frames + *frames > stream->bufsize_frames){
        UINT32 chunk_bytes, offs_bytes, frames_bytes;
        if(stream->tmp_buffer_frames < *frames){
            if(stream->tmp_buffer){
                size = 0;
                NtFreeVirtualMemory(GetCurrentProcess(), (void **)&stream->tmp_buffer, &size, MEM_RELEASE);
                stream->tmp_buffer = NULL;
            }
            size = *frames * stream->fmt->nBlockAlign;
            if(NtAllocateVirtualMemory(GetCurrentProcess(), (void **)&stream->tmp_buffer, 0, &size,
                                       MEM_COMMIT, PAGE_READWRITE)){
                stream->tmp_buffer_frames = 0;
                alsa_unlock(stream);
                return E_OUTOFMEMORY;
            }
            stream->tmp_buffer_frames = *frames;
        }

        *data = stream->tmp_buffer;
        chunk_bytes = (stream->bufsize_frames - stream->lcl_offs_frames) *
            stream->fmt->nBlockAlign;
        offs_bytes = stream->lcl_offs_frames * stream->fmt->nBlockAlign;
        frames_bytes = *frames * stream->fmt->nBlockAlign;
        memcpy(stream->tmp_buffer, stream->local_buffer + offs_bytes, chunk_bytes);
        memcpy(stream->tmp_buffer + chunk_bytes, stream->local_buffer,
                frames_bytes - chunk_bytes);
    }else
        *data = stream->local_buffer +
            stream->lcl_offs_frames * stream->fmt->nBlockAlign;

    stream->getbuf_last = *frames;
    *flags = 0;

    if(devpos)
      *devpos = stream->written_frames;
    if(qpcpos){ /* fixme: qpc of recording time */
        LARGE_INTEGER stamp, freq;
        QueryPerformanceCounter(&stamp);
        QueryPerformanceFrequency(&freq);
        *qpcpos = (stamp.QuadPart * (INT64)10000000) / freq.QuadPart;
    }

    alsa_unlock(stream);

    return *frames ? S_OK : AUDCLNT_S_BUFFER_EMPTY;
}

static HRESULT WINAPI AudioCaptureClient_ReleaseBuffer(
        IAudioCaptureClient *iface, UINT32 done)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    struct alsa_stream *stream = This->stream;

    TRACE("(%p)->(%u)\n", This, done);

    alsa_lock(stream);

    if(!done){
        stream->getbuf_last = 0;
        alsa_unlock(stream);
        return S_OK;
    }

    if(!stream->getbuf_last){
        alsa_unlock(stream);
        return AUDCLNT_E_OUT_OF_ORDER;
    }

    if(stream->getbuf_last != done){
        alsa_unlock(stream);
        return AUDCLNT_E_INVALID_SIZE;
    }

    stream->written_frames += done;
    stream->held_frames -= done;
    stream->lcl_offs_frames += done;
    stream->lcl_offs_frames %= stream->bufsize_frames;
    stream->getbuf_last = 0;

    alsa_unlock(stream);

    return S_OK;
}

static HRESULT WINAPI AudioCaptureClient_GetNextPacketSize(
        IAudioCaptureClient *iface, UINT32 *frames)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    struct alsa_stream *stream = This->stream;

    TRACE("(%p)->(%p)\n", This, frames);

    if(!frames)
        return E_POINTER;

    alsa_lock(stream);

    *frames = stream->held_frames < stream->mmdev_period_frames ? 0 : stream->mmdev_period_frames;

    alsa_unlock(stream);

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
    return IAudioClient3_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI AudioClock_Release(IAudioClock *iface)
{
    ACImpl *This = impl_from_IAudioClock(iface);
    return IAudioClient3_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI AudioClock_GetFrequency(IAudioClock *iface, UINT64 *freq)
{
    ACImpl *This = impl_from_IAudioClock(iface);
    struct alsa_stream *stream = This->stream;

    TRACE("(%p)->(%p)\n", This, freq);

    if(stream->share == AUDCLNT_SHAREMODE_SHARED)
        *freq = (UINT64)stream->fmt->nSamplesPerSec * stream->fmt->nBlockAlign;
    else
        *freq = stream->fmt->nSamplesPerSec;

    return S_OK;
}

static HRESULT WINAPI AudioClock_GetPosition(IAudioClock *iface, UINT64 *pos,
        UINT64 *qpctime)
{
    ACImpl *This = impl_from_IAudioClock(iface);
    struct alsa_stream *stream = This->stream;
    UINT64 position;
    snd_pcm_state_t alsa_state;

    TRACE("(%p)->(%p, %p)\n", This, pos, qpctime);

    if(!pos)
        return E_POINTER;

    alsa_lock(stream);

    /* avail_update required to get accurate snd_pcm_state() */
    snd_pcm_avail_update(stream->pcm_handle);
    alsa_state = snd_pcm_state(stream->pcm_handle);

    if(stream->flow == eRender){
        position = stream->written_frames - stream->held_frames;

        if(stream->started && alsa_state == SND_PCM_STATE_RUNNING && stream->held_frames)
            /* we should be using snd_pcm_delay here, but it is broken
             * especially during ALSA device underrun. instead, let's just
             * interpolate between periods with the system timer. */
            position += interp_elapsed_frames(stream);

        position = min(position, stream->written_frames - stream->held_frames + stream->mmdev_period_frames);

        position = min(position, stream->written_frames);
    }else
        position = stream->written_frames + stream->held_frames;

    /* ensure monotic growth */
    if(position < stream->last_pos_frames)
        position = stream->last_pos_frames;
    else
        stream->last_pos_frames = position;

    TRACE("frames written: %u, held: %u, state: 0x%x, position: %u\n",
            (UINT32)(stream->written_frames%1000000000), stream->held_frames,
            alsa_state, (UINT32)(position%1000000000));

    alsa_unlock(stream);

    if(stream->share == AUDCLNT_SHAREMODE_SHARED)
        *pos = position * stream->fmt->nBlockAlign;
    else
        *pos = position;

    if(qpctime){
        LARGE_INTEGER stamp, freq;
        QueryPerformanceCounter(&stamp);
        QueryPerformanceFrequency(&freq);
        *qpctime = (stamp.QuadPart * (INT64)10000000) / freq.QuadPart;
    }

    return S_OK;
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
    return IAudioClient3_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI AudioClock2_Release(IAudioClock2 *iface)
{
    ACImpl *This = impl_from_IAudioClock2(iface);
    return IAudioClient3_Release(&This->IAudioClient3_iface);
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
        AudioClient_AddRef(&client->IAudioClient3_iface);
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
            EnterCriticalSection(&g_sessions_lock);
            This->client->session_wrapper = NULL;
            LeaveCriticalSection(&g_sessions_lock);
            AudioClient_Release(&This->client->IAudioClient3_iface);
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
        alsa_lock(client->stream);
        if(client->stream->started){
            *state = AudioSessionStateActive;
            alsa_unlock(client->stream);
            LeaveCriticalSection(&g_sessions_lock);
            return S_OK;
        }
        alsa_unlock(client->stream);
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
    ACImpl *client;

    TRACE("(%p)->(%f, %s)\n", session, level, wine_dbgstr_guid(context));

    if(level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if(context)
        FIXME("Notifications not supported yet\n");

    TRACE("ALSA does not support volume control\n");

    EnterCriticalSection(&g_sessions_lock);

    session->master_vol = level;

    LIST_FOR_EACH_ENTRY(client, &session->clients, ACImpl, entry)
        set_stream_volumes(client);

    LeaveCriticalSection(&g_sessions_lock);

    return S_OK;
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
    ACImpl *client;

    TRACE("(%p)->(%u, %s)\n", session, mute, debugstr_guid(context));

    if(context)
        FIXME("Notifications not supported yet\n");

    EnterCriticalSection(&g_sessions_lock);

    session->mute = mute;
    LIST_FOR_EACH_ENTRY(client, &session->clients, ACImpl, entry)
        set_stream_volumes(client);

    LeaveCriticalSection(&g_sessions_lock);

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
    return IAudioClient3_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI AudioStreamVolume_Release(IAudioStreamVolume *iface)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    return IAudioClient3_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI AudioStreamVolume_GetChannelCount(
        IAudioStreamVolume *iface, UINT32 *out)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%p)\n", This, out);

    if(!out)
        return E_POINTER;

    *out = This->channel_count;

    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_SetChannelVolume(
        IAudioStreamVolume *iface, UINT32 index, float level)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%d, %f)\n", This, index, level);

    if(level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if(index >= This->channel_count)
        return E_INVALIDARG;

    TRACE("ALSA does not support volume control\n");

    EnterCriticalSection(&g_sessions_lock);

    This->vols[index] = level;
    set_stream_volumes(This);

    LeaveCriticalSection(&g_sessions_lock);

    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_GetChannelVolume(
        IAudioStreamVolume *iface, UINT32 index, float *level)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%d, %p)\n", This, index, level);

    if(!level)
        return E_POINTER;

    if(index >= This->channel_count)
        return E_INVALIDARG;

    *level = This->vols[index];

    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_SetAllVolumes(
        IAudioStreamVolume *iface, UINT32 count, const float *levels)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    unsigned int i;

    TRACE("(%p)->(%d, %p)\n", This, count, levels);

    if(!levels)
        return E_POINTER;

    if(count != This->channel_count)
        return E_INVALIDARG;

    TRACE("ALSA does not support volume control\n");

    EnterCriticalSection(&g_sessions_lock);

    for(i = 0; i < count; ++i)
        This->vols[i] = levels[i];
    set_stream_volumes(This);

    LeaveCriticalSection(&g_sessions_lock);

    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_GetAllVolumes(
        IAudioStreamVolume *iface, UINT32 count, float *levels)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    unsigned int i;

    TRACE("(%p)->(%d, %p)\n", This, count, levels);

    if(!levels)
        return E_POINTER;

    if(count != This->channel_count)
        return E_INVALIDARG;

    EnterCriticalSection(&g_sessions_lock);

    for(i = 0; i < count; ++i)
        levels[i] = This->vols[i];

    LeaveCriticalSection(&g_sessions_lock);

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
    ACImpl *client;

    TRACE("(%p)->(%d, %f, %s)\n", session, index, level,
            wine_dbgstr_guid(context));

    if(level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if(index >= session->channel_count)
        return E_INVALIDARG;

    if(context)
        FIXME("Notifications not supported yet\n");

    TRACE("ALSA does not support volume control\n");

    EnterCriticalSection(&g_sessions_lock);

    session->channel_vols[index] = level;

    LIST_FOR_EACH_ENTRY(client, &session->clients, ACImpl, entry)
        set_stream_volumes(client);

    LeaveCriticalSection(&g_sessions_lock);

    return S_OK;
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
    unsigned int i;
    ACImpl *client;

    TRACE("(%p)->(%d, %p, %s)\n", session, count, levels,
            wine_dbgstr_guid(context));

    if(!levels)
        return NULL_PTR_ERR;

    if(count != session->channel_count)
        return E_INVALIDARG;

    if(context)
        FIXME("Notifications not supported yet\n");

    TRACE("ALSA does not support volume control\n");

    EnterCriticalSection(&g_sessions_lock);

    for(i = 0; i < count; ++i)
        session->channel_vols[i] = levels[i];

    LIST_FOR_EACH_ENTRY(client, &session->clients, ACImpl, entry)
        set_stream_volumes(client);

    LeaveCriticalSection(&g_sessions_lock);

    return S_OK;
}

static HRESULT WINAPI ChannelAudioVolume_GetAllVolumes(
        IChannelAudioVolume *iface, UINT32 count, float *levels)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;
    unsigned int i;

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

static unsigned int alsa_probe_num_speakers(char *name) {
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    int err;
    unsigned int max_channels = 0;

    if ((err = snd_pcm_open(&handle, name, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0) {
        WARN("The device \"%s\" failed to open: %d (%s).\n",
                name, err, snd_strerror(err));
        return 0;
    }

    params = HeapAlloc(GetProcessHeap(), 0, snd_pcm_hw_params_sizeof());
    if (!params) {
        WARN("Out of memory.\n");
        snd_pcm_close(handle);
        return 0;
    }

    if ((err = snd_pcm_hw_params_any(handle, params)) < 0) {
        WARN("snd_pcm_hw_params_any failed for \"%s\": %d (%s).\n",
                name, err, snd_strerror(err));
        goto exit;
    }

    if ((err = snd_pcm_hw_params_get_channels_max(params,
                    &max_channels)) < 0){
        WARN("Unable to get max channels: %d (%s)\n", err, snd_strerror(err));
        goto exit;
    }

exit:
    HeapFree(GetProcessHeap(), 0, params);
    snd_pcm_close(handle);

    return max_channels;
}

enum AudioDeviceConnectionType {
    AudioDeviceConnectionType_Unknown = 0,
    AudioDeviceConnectionType_PCI,
    AudioDeviceConnectionType_USB
};

HRESULT WINAPI AUDDRV_GetPropValue(GUID *guid, const PROPERTYKEY *prop, PROPVARIANT *out)
{
    char name[256];
    EDataFlow flow;

    static const PROPERTYKEY devicepath_key = { /* undocumented? - {b3f8fa53-0004-438e-9003-51a46e139bfc},2 */
        {0xb3f8fa53, 0x0004, 0x438e, {0x90, 0x03, 0x51, 0xa4, 0x6e, 0x13, 0x9b, 0xfc}}, 2
    };

    TRACE("%s, (%s,%u), %p\n", wine_dbgstr_guid(guid), wine_dbgstr_guid(&prop->fmtid), prop->pid, out);

    if(!get_alsa_name_by_guid(guid, name, sizeof(name), &flow))
    {
        WARN("Unknown interface %s\n", debugstr_guid(guid));
        return E_NOINTERFACE;
    }

    if(IsEqualPropertyKey(*prop, devicepath_key))
    {
        char uevent[MAX_PATH];
        FILE *fuevent;
        int card, device;

        /* only implemented for identifiable devices, i.e. not "default" */
        if(!sscanf(name, "plughw:%u,%u", &card, &device))
            return E_NOTIMPL;

        sprintf(uevent, "/sys/class/sound/card%u/device/uevent", card);
        fuevent = fopen(uevent, "r");

        if(fuevent){
            enum AudioDeviceConnectionType connection = AudioDeviceConnectionType_Unknown;
            USHORT vendor_id = 0, product_id = 0;
            char line[256];

            while (fgets(line, sizeof(line), fuevent)) {
                char *val;
                size_t val_len;

                if((val = strchr(line, '='))) {
                    val[0] = 0;
                    val++;

                    val_len = strlen(val);
                    if(val_len > 0 && val[val_len - 1] == '\n') { val[val_len - 1] = 0; }

                    if(!strcmp(line, "PCI_ID")){
                        connection = AudioDeviceConnectionType_PCI;
                        if(sscanf(val, "%hX:%hX", &vendor_id, &product_id)<2){
                            WARN("Unexpected input when reading PCI_ID in uevent file.\n");
                            connection = AudioDeviceConnectionType_Unknown;
                            break;
                        }
                    }else if(!strcmp(line, "DEVTYPE") && !strcmp(val,"usb_interface"))
                        connection = AudioDeviceConnectionType_USB;
                    else if(!strcmp(line, "PRODUCT"))
                        if(sscanf(val, "%hx/%hx/", &vendor_id, &product_id)<2){
                            WARN("Unexpected input when reading PRODUCT in uevent file.\n");
                            connection = AudioDeviceConnectionType_Unknown;
                            break;
                        }
                }
            }

            fclose(fuevent);

            if(connection == AudioDeviceConnectionType_USB || connection == AudioDeviceConnectionType_PCI){
                static const WCHAR usbformatW[] = { '{','1','}','.','U','S','B','\\','V','I','D','_',
                    '%','0','4','X','&','P','I','D','_','%','0','4','X','\\',
                    '%','u','&','%','0','8','X',0 }; /* "{1}.USB\VID_%04X&PID_%04X\%u&%08X" */
                static const WCHAR pciformatW[] = { '{','1','}','.','H','D','A','U','D','I','O','\\','F','U','N','C','_','0','1','&',
                    'V','E','N','_','%','0','4','X','&','D','E','V','_',
                    '%','0','4','X','\\','%','u','&','%','0','8','X',0 }; /* "{1}.HDAUDIO\FUNC_01&VEN_%04X&DEV_%04X\%u&%08X" */
                UINT serial_number;

                /* As hardly any audio devices have serial numbers, Windows instead
                appears to use a persistent random number. We emulate this here
                by instead using the last 8 hex digits of the GUID. */
                serial_number = (guid->Data4[4] << 24) | (guid->Data4[5] << 16) | (guid->Data4[6] << 8) | guid->Data4[7];

                out->vt = VT_LPWSTR;
                out->pwszVal = CoTaskMemAlloc(128 * sizeof(WCHAR));

                if(!out->pwszVal)
                    return E_OUTOFMEMORY;

                if(connection == AudioDeviceConnectionType_USB)
                    sprintfW( out->pwszVal, usbformatW, vendor_id, product_id, device, serial_number);
                else if(connection == AudioDeviceConnectionType_PCI)
                    sprintfW( out->pwszVal, pciformatW, vendor_id, product_id, device, serial_number);

                return S_OK;
            }
        }else{
            WARN("Could not open %s for reading\n", uevent);
            return E_NOTIMPL;
        }
    } else if (flow != eCapture && IsEqualPropertyKey(*prop, PKEY_AudioEndpoint_PhysicalSpeakers)) {
        unsigned int num_speakers, card, device;
        char hwname[255];

        if (sscanf(name, "plughw:%u,%u", &card, &device))
            sprintf(hwname, "hw:%u,%u", card, device); /* must be hw rather than plughw to work */
        else
            strcpy(hwname, name);

        num_speakers = alsa_probe_num_speakers(hwname);
        if (num_speakers == 0)
            return E_FAIL;

        out->vt = VT_UI4;

        if (num_speakers > 6)
            out->ulVal = KSAUDIO_SPEAKER_STEREO;
        else if (num_speakers == 6)
            out->ulVal = KSAUDIO_SPEAKER_5POINT1;
        else if (num_speakers >= 4)
            out->ulVal = KSAUDIO_SPEAKER_QUAD;
        else if (num_speakers >= 2)
            out->ulVal = KSAUDIO_SPEAKER_STEREO;
        else if (num_speakers == 1)
            out->ulVal = KSAUDIO_SPEAKER_MONO;

        return S_OK;
    }

    TRACE("Unimplemented property %s,%u\n", wine_dbgstr_guid(&prop->fmtid), prop->pid);

    return E_NOTIMPL;
}
