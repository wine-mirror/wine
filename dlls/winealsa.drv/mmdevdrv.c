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

#define COBJMACROS

#include <stdarg.h>
#include <wchar.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winternl.h"
#include "propsys.h"
#include "propkey.h"
#include "initguid.h"
#include "ole2.h"
#include "mmdeviceapi.h"
#include "devpkey.h"
#include "mmsystem.h"
#include "dsound.h"

#include "endpointvolume.h"
#include "audioclient.h"
#include "audiopolicy.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unixlib.h"

#include "unixlib.h"

#include "../mmdevapi/mmdevdrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(alsa);

#define NULL_PTR_ERR MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER)

static const REFERENCE_TIME DefaultPeriod = 100000;
static const REFERENCE_TIME MinimumPeriod = 50000;

static CRITICAL_SECTION g_sessions_lock;
static CRITICAL_SECTION_DEBUG g_sessions_lock_debug =
{
    0, 0, &g_sessions_lock,
    { &g_sessions_lock_debug.ProcessLocksList, &g_sessions_lock_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": g_sessions_lock") }
};
static CRITICAL_SECTION g_sessions_lock = { &g_sessions_lock_debug, -1, 0, 0, 0, 0 };
static struct list g_sessions = LIST_INIT(g_sessions);

static WCHAR drv_key_devicesW[256];
static const WCHAR guidW[] = {'g','u','i','d',0};

static const IAudioClient3Vtbl AudioClient3_Vtbl;
static const IAudioRenderClientVtbl AudioRenderClient_Vtbl;
static const IAudioCaptureClientVtbl AudioCaptureClient_Vtbl;
extern const IAudioSessionControl2Vtbl AudioSessionControl2_Vtbl;
extern const ISimpleAudioVolumeVtbl SimpleAudioVolume_Vtbl;
extern const IAudioClockVtbl AudioClock_Vtbl;
extern const IAudioClock2Vtbl AudioClock2_Vtbl;
extern const IAudioStreamVolumeVtbl AudioStreamVolume_Vtbl;
extern const IChannelAudioVolumeVtbl ChannelAudioVolume_Vtbl;

static AudioSessionWrapper *AudioSessionWrapper_Create(ACImpl *client);

void DECLSPEC_HIDDEN sessions_lock(void)
{
    EnterCriticalSection(&g_sessions_lock);
}

void DECLSPEC_HIDDEN sessions_unlock(void)
{
    LeaveCriticalSection(&g_sessions_lock);
}

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

BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, void *reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        WCHAR buf[MAX_PATH];
        WCHAR *filename;

        if(__wine_init_unix_call()) return FALSE;

        GetModuleFileNameW(dll, buf, ARRAY_SIZE(buf));

        filename = wcsrchr(buf, '\\');
        filename = filename ? filename + 1 : buf;

        swprintf(drv_key_devicesW, ARRAY_SIZE(drv_key_devicesW),
                 L"Software\\Wine\\Drivers\\%s\\devices", filename);

        break;
    }
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        DeleteCriticalSection(&g_sessions_lock);
        break;
    }
    return TRUE;
}

static HRESULT alsa_stream_release(stream_handle stream, HANDLE timer_thread)
{
    struct release_stream_params params;

    params.stream = stream;
    params.timer_thread = timer_thread;

    ALSA_CALL(release_stream, &params);

    return params.result;
}

static DWORD WINAPI alsa_timer_thread(void *user)
{
    struct timer_loop_params params;
    ACImpl *This = user;

    SetThreadDescription(GetCurrentThread(), L"winealsa_timer");

    params.stream = This->stream;

    ALSA_CALL(timer_loop, &params);

    return 0;
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
            ERR("RegCreateKeyEx(drv_key) failed: %lu\n", lr);
            return;
        }
        opened = TRUE;
    }

    lr = RegCreateKeyExW(drv_key, key_name, 0, NULL, 0, KEY_WRITE,
                NULL, &key, NULL);
    if(lr != ERROR_SUCCESS){
        ERR("RegCreateKeyEx(%s) failed: %lu\n", wine_dbgstr_w(key_name), lr);
        goto exit;
    }

    lr = RegSetValueExW(key, guidW, 0, REG_BINARY, (BYTE*)guid,
                sizeof(GUID));
    if(lr != ERROR_SUCCESS)
        ERR("RegSetValueEx(%s\\guid) failed: %lu\n", wine_dbgstr_w(key_name), lr);

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
                ERR("Invalid type for device %s GUID: %lu; ignoring and overwriting\n",
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
    struct set_volumes_params params;

    params.stream = This->stream;
    params.master_volume = (This->session->mute ? 0.0f : This->session->master_vol);
    params.volumes = This->vols;
    params.session_volumes = This->session->channel_vols;

    ALSA_CALL(set_volumes, &params);
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
        WCHAR *name = (WCHAR *)((char *)params.endpoints + params.endpoints[i].name);
        char *device = (char *)params.endpoints + params.endpoints[i].device;
        unsigned int size = (wcslen(name) + 1) * sizeof(WCHAR);

        ids[i] = HeapAlloc(GetProcessHeap(), 0, size);
        if(!ids[i]){
            params.result = E_OUTOFMEMORY;
            goto end;
        }
        memcpy(ids[i], name, size);
        get_device_guid(flow, device, guids + i);
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
    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, offsetof(ACImpl, device_name[len + 1]));
    if(!This)
        return E_OUTOFMEMORY;

    This->IAudioClient3_iface.lpVtbl = &AudioClient3_Vtbl;
    This->IAudioRenderClient_iface.lpVtbl = &AudioRenderClient_Vtbl;
    This->IAudioCaptureClient_iface.lpVtbl = &AudioCaptureClient_Vtbl;
    This->IAudioClock_iface.lpVtbl = &AudioClock_Vtbl;
    This->IAudioClock2_iface.lpVtbl = &AudioClock2_Vtbl;
    This->IAudioStreamVolume_iface.lpVtbl = &AudioStreamVolume_Vtbl;

    hr = CoCreateFreeThreadedMarshaler((IUnknown *)&This->IAudioClient3_iface, &This->marshal);
    if (FAILED(hr)) {
        HeapFree(GetProcessHeap(), 0, This);
        return hr;
    }

    This->dataflow = dataflow;
    memcpy(This->device_name, alsa_name, len + 1);

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
        return IUnknown_QueryInterface(This->marshal, riid, ppv);

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
    TRACE("(%p) Refcount now %lu\n", This, ref);
    return ref;
}

static ULONG WINAPI AudioClient_Release(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    ULONG ref;

    ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) Refcount now %lu\n", This, ref);
    if(!ref){
        IAudioClient3_Stop(iface);
        IMMDevice_Release(This->parent);
        IUnknown_Release(This->marshal);
        if(This->session){
            sessions_lock();
            list_remove(&This->entry);
            sessions_unlock();
        }
        HeapFree(GetProcessHeap(), 0, This->vols);
        if (This->stream)
            alsa_stream_release(This->stream, This->timer_thread);
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
    TRACE("nSamplesPerSec: %lu\n", fmt->nSamplesPerSec);
    TRACE("nAvgBytesPerSec: %lu\n", fmt->nAvgBytesPerSec);
    TRACE("nBlockAlign: %u\n", fmt->nBlockAlign);
    TRACE("wBitsPerSample: %u\n", fmt->wBitsPerSample);
    TRACE("cbSize: %u\n", fmt->cbSize);

    if(fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE){
        WAVEFORMATEXTENSIBLE *fmtex = (void*)fmt;
        TRACE("dwChannelMask: %08lx\n", fmtex->dwChannelMask);
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

static HRESULT WINAPI AudioClient_Initialize(IAudioClient3 *iface,
        AUDCLNT_SHAREMODE mode, DWORD flags, REFERENCE_TIME duration,
        REFERENCE_TIME period, const WAVEFORMATEX *fmt,
        const GUID *sessionguid)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct create_stream_params params;
    stream_handle stream;
    unsigned int i;

    TRACE("(%p)->(%x, %lx, %s, %s, %p, %s)\n", This, mode, flags,
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
        FIXME("Unknown flags: %08lx\n", flags);
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

    sessions_lock();

    if(This->stream){
        sessions_unlock();
        return AUDCLNT_E_ALREADY_INITIALIZED;
    }

    dump_fmt(fmt);

    params.name = NULL;
    params.device = This->device_name;
    params.flow = This->dataflow;
    params.share = mode;
    params.flags = flags;
    params.duration = duration;
    params.period = period;
    params.fmt = fmt;
    params.channel_count = NULL;
    params.stream = &stream;

    ALSA_CALL(create_stream, &params);
    if(FAILED(params.result)){
        sessions_unlock();
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
        alsa_stream_release(stream, NULL);
        HeapFree(GetProcessHeap(), 0, This->vols);
        This->vols = NULL;
    }else{
        This->stream = stream;
        set_stream_volumes(This);
    }

    sessions_unlock();

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
    params.frames = out;

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

    params.device = This->device_name;
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

    params.device = This->device_name;
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

static HRESULT WINAPI AudioClient_Start(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct start_params params;

    TRACE("(%p)\n", This);

    sessions_lock();

    if(!This->stream){
        sessions_unlock();
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    params.stream = This->stream;

    ALSA_CALL(start, &params);

    if(SUCCEEDED(params.result) && !This->timer_thread){
        This->timer_thread = CreateThread(NULL, 0, alsa_timer_thread, This, 0, NULL);
        SetThreadPriority(This->timer_thread, THREAD_PRIORITY_TIME_CRITICAL);
    }

    sessions_unlock();

    return params.result;
}

static HRESULT WINAPI AudioClient_Stop(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct stop_params params;

    TRACE("(%p)\n", This);

    if(!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;

    ALSA_CALL(stop, &params);

    return params.result;
}

static HRESULT WINAPI AudioClient_Reset(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct reset_params params;

    TRACE("(%p)\n", This);

    if(!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;

    ALSA_CALL(reset, &params);

    return params.result;
}

static HRESULT WINAPI AudioClient_SetEventHandle(IAudioClient3 *iface,
        HANDLE event)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct set_event_handle_params params;

    TRACE("(%p)->(%p)\n", This, event);

    if(!event)
        return E_INVALIDARG;

    if(!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    params.event = event;

    ALSA_CALL(set_event_handle, &params);

    return params.result;
}

static HRESULT WINAPI AudioClient_GetService(IAudioClient3 *iface, REFIID riid,
        void **ppv)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    sessions_lock();

    if(!This->stream){
        sessions_unlock();
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if(IsEqualIID(riid, &IID_IAudioRenderClient)){
        if(This->dataflow != eRender){
            sessions_unlock();
            return AUDCLNT_E_WRONG_ENDPOINT_TYPE;
        }
        IAudioRenderClient_AddRef(&This->IAudioRenderClient_iface);
        *ppv = &This->IAudioRenderClient_iface;
    }else if(IsEqualIID(riid, &IID_IAudioCaptureClient)){
        if(This->dataflow != eCapture){
            sessions_unlock();
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
                sessions_unlock();
                return E_OUTOFMEMORY;
            }
        }else
            IAudioSessionControl2_AddRef(&This->session_wrapper->IAudioSessionControl2_iface);

        *ppv = &This->session_wrapper->IAudioSessionControl2_iface;
    }else if(IsEqualIID(riid, &IID_IChannelAudioVolume)){
        if(!This->session_wrapper){
            This->session_wrapper = AudioSessionWrapper_Create(This);
            if(!This->session_wrapper){
                sessions_unlock();
                return E_OUTOFMEMORY;
            }
        }else
            IChannelAudioVolume_AddRef(&This->session_wrapper->IChannelAudioVolume_iface);

        *ppv = &This->session_wrapper->IChannelAudioVolume_iface;
    }else if(IsEqualIID(riid, &IID_ISimpleAudioVolume)){
        if(!This->session_wrapper){
            This->session_wrapper = AudioSessionWrapper_Create(This);
            if(!This->session_wrapper){
                sessions_unlock();
                return E_OUTOFMEMORY;
            }
        }else
            ISimpleAudioVolume_AddRef(&This->session_wrapper->ISimpleAudioVolume_iface);

        *ppv = &This->session_wrapper->ISimpleAudioVolume_iface;
    }

    if(*ppv){
        sessions_unlock();
        return S_OK;
    }

    sessions_unlock();

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

    FIXME("(%p)->(0x%lx, %u, %p, %s)\n", This, flags, period_frames, format, debugstr_guid(session_guid));

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
        return IUnknown_QueryInterface(This->marshal, riid, ppv);

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
    return IAudioClient3_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI AudioRenderClient_Release(IAudioRenderClient *iface)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    return IAudioClient3_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI AudioRenderClient_GetBuffer(IAudioRenderClient *iface,
        UINT32 frames, BYTE **data)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    struct get_render_buffer_params params;

    TRACE("(%p)->(%u, %p)\n", This, frames, data);

    if(!data)
        return E_POINTER;
    *data = NULL;

    params.stream = This->stream;
    params.frames = frames;
    params.data = data;

    ALSA_CALL(get_render_buffer, &params);

    return params.result;
}

static HRESULT WINAPI AudioRenderClient_ReleaseBuffer(
        IAudioRenderClient *iface, UINT32 written_frames, DWORD flags)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    struct release_render_buffer_params params;

    TRACE("(%p)->(%u, %lx)\n", This, written_frames, flags);

    params.stream = This->stream;
    params.written_frames = written_frames;
    params.flags = flags;

    ALSA_CALL(release_render_buffer, &params);

    return params.result;
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
        return IUnknown_QueryInterface(This->marshal, riid, ppv);

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
    struct get_capture_buffer_params params;

    TRACE("(%p)->(%p, %p, %p, %p, %p)\n", This, data, frames, flags,
            devpos, qpcpos);

    if(!data)
        return E_POINTER;

    *data = NULL;

    if(!frames || !flags)
        return E_POINTER;

    params.stream = This->stream;
    params.data = data;
    params.frames = frames;
    params.flags = (UINT*)flags;
    params.devpos = devpos;
    params.qpcpos = qpcpos;

    ALSA_CALL(get_capture_buffer, &params);

    return params.result;
}

static HRESULT WINAPI AudioCaptureClient_ReleaseBuffer(
        IAudioCaptureClient *iface, UINT32 done)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    struct release_capture_buffer_params params;

    TRACE("(%p)->(%u)\n", This, done);

    params.stream = This->stream;
    params.done = done;

    ALSA_CALL(release_capture_buffer, &params);

    return params.result;
}

static HRESULT WINAPI AudioCaptureClient_GetNextPacketSize(
        IAudioCaptureClient *iface, UINT32 *frames)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    struct get_next_packet_size_params params;

    TRACE("(%p)->(%p)\n", This, frames);

    if(!frames)
        return E_POINTER;

    params.stream = This->stream;
    params.frames = frames;

    ALSA_CALL(get_next_packet_size, &params);

    return params.result;
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
        IAudioClient3_AddRef(&client->IAudioClient3_iface);
    }

    return ret;
}

HRESULT WINAPI AUDDRV_GetAudioSessionWrapper(const GUID *guid, IMMDevice *device,
                                             AudioSessionWrapper **out)
{
    AudioSession *session;

    HRESULT hr = get_audio_session(guid, device, 0, &session);
    if(FAILED(hr))
        return hr;

    *out = AudioSessionWrapper_Create(NULL);
    if(!*out)
        return E_OUTOFMEMORY;

    (*out)->session = session;

    return S_OK;
}

HRESULT WINAPI AUDDRV_GetPropValue(GUID *guid, const PROPERTYKEY *prop, PROPVARIANT *out)
{
    struct get_prop_value_params params;
    char name[256];
    EDataFlow flow;
    unsigned int size = 0;

    TRACE("%s, (%s,%lu), %p\n", wine_dbgstr_guid(guid), wine_dbgstr_guid(&prop->fmtid), prop->pid, out);

    if(!get_alsa_name_by_guid(guid, name, sizeof(name), &flow))
    {
        WARN("Unknown interface %s\n", debugstr_guid(guid));
        return E_NOINTERFACE;
    }

    params.device = name;
    params.flow = flow;
    params.guid = guid;
    params.prop = prop;
    params.value = out;
    params.buffer = NULL;
    params.buffer_size = &size;

    while(1) {
        ALSA_CALL(get_prop_value, &params);

        if(params.result != E_NOT_SUFFICIENT_BUFFER)
            break;

        CoTaskMemFree(params.buffer);
        params.buffer = CoTaskMemAlloc(*params.buffer_size);
        if(!params.buffer)
            return E_OUTOFMEMORY;
    }
    if(FAILED(params.result))
        CoTaskMemFree(params.buffer);

    return params.result;
}
