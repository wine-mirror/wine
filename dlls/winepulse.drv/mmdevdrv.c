/*
 * Copyright 2011-2012 Maarten Lankhorst
 * Copyright 2010-2011 Maarten Lankhorst for CodeWeavers
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
#include <assert.h>
#include <wchar.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unixlib.h"

#include "ole2.h"
#include "mimeole.h"
#include "dshow.h"
#include "dsound.h"
#include "propsys.h"
#include "propkey.h"

#include "initguid.h"
#include "ks.h"
#include "ksmedia.h"
#include "mmdeviceapi.h"
#include "audioclient.h"
#include "endpointvolume.h"
#include "audiopolicy.h"

#include "../mmdevapi/unixlib.h"
#include "../mmdevapi/mmdevdrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(pulse);

#define MAX_PULSE_NAME_LEN 256

#define NULL_PTR_ERR MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER)

static HANDLE pulse_thread;
static struct list g_sessions = LIST_INIT(g_sessions);
static struct list g_devices_cache = LIST_INIT(g_devices_cache);

struct device_cache {
    struct list entry;
    GUID guid;
    EDataFlow dataflow;
    char pulse_name[0];
};

static GUID pulse_render_guid =
{ 0xfd47d9cc, 0x4218, 0x4135, { 0x9c, 0xe2, 0x0c, 0x19, 0x5c, 0x87, 0x40, 0x5b } };
static GUID pulse_capture_guid =
{ 0x25da76d0, 0x033c, 0x4235, { 0x90, 0x02, 0x19, 0xf4, 0x88, 0x94, 0xac, 0x6f } };

static WCHAR drv_key_devicesW[256];

static CRITICAL_SECTION session_cs;
static CRITICAL_SECTION_DEBUG session_cs_debug = {
    0, 0, &session_cs,
    { &session_cs_debug.ProcessLocksList,
      &session_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": session_cs") }
};
static CRITICAL_SECTION session_cs = { &session_cs_debug, -1, 0, 0, 0, 0 };

void DECLSPEC_HIDDEN sessions_lock(void)
{
    EnterCriticalSection(&session_cs);
}

void DECLSPEC_HIDDEN sessions_unlock(void)
{
    LeaveCriticalSection(&session_cs);
}

BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, void *reserved)
{
    if (reason == DLL_PROCESS_ATTACH) {
        WCHAR buf[MAX_PATH];
        WCHAR *filename;

        DisableThreadLibraryCalls(dll);
        if (__wine_init_unix_call())
            return FALSE;

        GetModuleFileNameW(dll, buf, ARRAY_SIZE(buf));

        filename = wcsrchr(buf, '\\');
        filename = filename ? filename + 1 : buf;

        swprintf(drv_key_devicesW, ARRAY_SIZE(drv_key_devicesW),
                 L"Software\\Wine\\Drivers\\%s\\devices", filename);
    } else if (reason == DLL_PROCESS_DETACH) {
        struct device_cache *device, *device_next;

        LIST_FOR_EACH_ENTRY_SAFE(device, device_next, &g_devices_cache, struct device_cache, entry)
            free(device);

        if (pulse_thread) {
            WaitForSingleObject(pulse_thread, INFINITE);
            CloseHandle(pulse_thread);
        }
    }
    return TRUE;
}

static const IAudioClient3Vtbl AudioClient3_Vtbl;
static const IAudioRenderClientVtbl AudioRenderClient_Vtbl;
static const IAudioCaptureClientVtbl AudioCaptureClient_Vtbl;
extern const IAudioSessionControl2Vtbl AudioSessionControl2_Vtbl;
static const ISimpleAudioVolumeVtbl SimpleAudioVolume_Vtbl;
static const IChannelAudioVolumeVtbl ChannelAudioVolume_Vtbl;
static const IAudioClockVtbl AudioClock_Vtbl;
static const IAudioClock2Vtbl AudioClock2_Vtbl;
static const IAudioStreamVolumeVtbl AudioStreamVolume_Vtbl;

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

static void pulse_call(enum unix_funcs code, void *params)
{
    NTSTATUS status;
    status = WINE_UNIX_CALL(code, params);
    assert(!status);
}

static void pulse_release_stream(stream_handle stream, HANDLE timer)
{
    struct release_stream_params params;
    params.stream       = stream;
    params.timer_thread = timer;
    pulse_call(release_stream, &params);
}

static DWORD CALLBACK pulse_mainloop_thread(void *event)
{
    struct main_loop_params params;
    params.event = event;
    SetThreadDescription(GetCurrentThread(), L"winepulse_mainloop");
    pulse_call(main_loop, &params);
    return 0;
}

typedef struct tagLANGANDCODEPAGE
{
    WORD wLanguage;
    WORD wCodePage;
} LANGANDCODEPAGE;

static BOOL query_productname(void *data, LANGANDCODEPAGE *lang, LPVOID *buffer, UINT *len)
{
    WCHAR pn[37];
    swprintf(pn, ARRAY_SIZE(pn), L"\\StringFileInfo\\%04x%04x\\ProductName", lang->wLanguage, lang->wCodePage);
    return VerQueryValueW(data, pn, buffer, len) && *len;
}

static WCHAR *get_application_name(BOOL query_app_name)
{
    WCHAR path[MAX_PATH], *name;

    GetModuleFileNameW(NULL, path, ARRAY_SIZE(path));

    if (query_app_name)
    {
        UINT translate_size, productname_size;
        LANGANDCODEPAGE *translate;
        LPVOID productname;
        BOOL found = FALSE;
        void *data = NULL;
        unsigned int i;
        LCID locale;
        DWORD size;

        size = GetFileVersionInfoSizeW(path, NULL);
        if (!size)
            goto skip;

        data = malloc(size);
        if (!data)
            goto skip;

        if (!GetFileVersionInfoW(path, 0, size, data))
            goto skip;

        if (!VerQueryValueW(data, L"\\VarFileInfo\\Translation", (LPVOID *)&translate, &translate_size))
            goto skip;

        /* no translations found */
        if (translate_size < sizeof(LANGANDCODEPAGE))
            goto skip;

        /* The following code will try to find the best translation. We first search for an
         * exact match of the language, then a match of the language PRIMARYLANGID, then we
         * search for a LANG_NEUTRAL match, and if that still doesn't work we pick the
         * first entry which contains a proper productname. */
        locale = GetThreadLocale();

        for (i = 0; i < translate_size / sizeof(LANGANDCODEPAGE); i++) {
            if (translate[i].wLanguage == locale &&
                    query_productname(data, &translate[i], &productname, &productname_size)) {
                found = TRUE;
                break;
            }
        }

        if (!found) {
            for (i = 0; i < translate_size / sizeof(LANGANDCODEPAGE); i++) {
                if (PRIMARYLANGID(translate[i].wLanguage) == PRIMARYLANGID(locale) &&
                        query_productname(data, &translate[i], &productname, &productname_size)) {
                    found = TRUE;
                    break;
                }
            }
        }

        if (!found) {
            for (i = 0; i < translate_size / sizeof(LANGANDCODEPAGE); i++) {
                if (PRIMARYLANGID(translate[i].wLanguage) == LANG_NEUTRAL &&
                        query_productname(data, &translate[i], &productname, &productname_size)) {
                    found = TRUE;
                    break;
                }
            }
        }

        if (!found) {
            for (i = 0; i < translate_size / sizeof(LANGANDCODEPAGE); i++) {
                if (query_productname(data, &translate[i], &productname, &productname_size)) {
                    found = TRUE;
                    break;
                }
            }
        }

    skip:
        free(data);
        if (found) return wcsdup(productname);
    }

    name = wcsrchr(path, '\\');
    if (!name)
        name = path;
    else
        name++;
    return wcsdup(name);
}

static DWORD WINAPI pulse_timer_cb(void *user)
{
    struct timer_loop_params params;
    ACImpl *This = user;
    params.stream = This->stream;
    SetThreadDescription(GetCurrentThread(), L"winepulse_timer_loop");
    pulse_call(timer_loop, &params);
    return 0;
}

static void set_stream_volumes(ACImpl *This)
{
    struct set_volumes_params params;
    params.stream          = This->stream;
    params.master_volume   = This->session->mute ? 0.0f : This->session->master_vol;
    params.volumes         = This->vols;
    params.session_volumes = This->session->channel_vols;
    params.channel         = 0;
    pulse_call(set_volumes, &params);
}

static void get_device_guid(HKEY drv_key, EDataFlow flow, const char *pulse_name, GUID *guid)
{
    WCHAR key_name[MAX_PULSE_NAME_LEN + 2];
    DWORD type, size = sizeof(*guid);
    LSTATUS status;
    HKEY dev_key;

    if (!pulse_name[0]) {
        *guid = (flow == eRender) ? pulse_render_guid : pulse_capture_guid;
        return;
    }

    if (!drv_key) {
        CoCreateGuid(guid);
        return;
    }

    key_name[0] = (flow == eRender) ? '0' : '1';
    key_name[1] = ',';
    MultiByteToWideChar(CP_UNIXCP, 0, pulse_name, -1, key_name + 2, ARRAY_SIZE(key_name) - 2);

    status = RegCreateKeyExW(drv_key, key_name, 0, NULL, 0, KEY_READ | KEY_WRITE | KEY_WOW64_64KEY,
                             NULL, &dev_key, NULL);
    if (status != ERROR_SUCCESS) {
        ERR("Failed to open registry key for device %s: %lu\n", pulse_name, status);
        CoCreateGuid(guid);
        return;
    }

    status = RegQueryValueExW(dev_key, L"guid", 0, &type, (BYTE*)guid, &size);
    if (status != ERROR_SUCCESS || type != REG_BINARY || size != sizeof(*guid)) {
        CoCreateGuid(guid);
        status = RegSetValueExW(dev_key, L"guid", 0, REG_BINARY, (BYTE*)guid, sizeof(*guid));
        if (status != ERROR_SUCCESS)
            ERR("Failed to store device GUID for %s to registry: %lu\n", pulse_name, status);
    }
    RegCloseKey(dev_key);
}

HRESULT WINAPI AUDDRV_GetEndpointIDs(EDataFlow flow, WCHAR ***ids_out, GUID **keys,
        UINT *num, UINT *def_index)
{
    struct get_endpoint_ids_params params;
    GUID *guids = NULL;
    WCHAR **ids = NULL;
    unsigned int i = 0;
    LSTATUS status;
    HKEY drv_key;

    TRACE("%d %p %p %p\n", flow, ids_out, num, def_index);

    params.flow = flow;
    params.size = MAX_PULSE_NAME_LEN * 4;
    params.endpoints = NULL;
    do {
        HeapFree(GetProcessHeap(), 0, params.endpoints);
        params.endpoints = HeapAlloc(GetProcessHeap(), 0, params.size);
        pulse_call(get_endpoint_ids, &params);
    } while(params.result == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

    if (FAILED(params.result))
        goto end;

    ids = HeapAlloc(GetProcessHeap(), 0, params.num * sizeof(*ids));
    guids = HeapAlloc(GetProcessHeap(), 0, params.num * sizeof(*guids));
    if (!ids || !guids) {
        params.result = E_OUTOFMEMORY;
        goto end;
    }

    status = RegCreateKeyExW(HKEY_CURRENT_USER, drv_key_devicesW, 0, NULL, 0,
                             KEY_WRITE | KEY_WOW64_64KEY, NULL, &drv_key, NULL);
    if (status != ERROR_SUCCESS) {
        ERR("Failed to open devices registry key: %lu\n", status);
        drv_key = NULL;
    }

    for (i = 0; i < params.num; i++) {
        WCHAR *name = (WCHAR *)((char *)params.endpoints + params.endpoints[i].name);
        char *pulse_name = (char *)params.endpoints + params.endpoints[i].device;
        unsigned int size = (wcslen(name) + 1) * sizeof(WCHAR);

        if (!(ids[i] = HeapAlloc(GetProcessHeap(), 0, size))) {
            params.result = E_OUTOFMEMORY;
            break;
        }
        memcpy(ids[i], name, size);
        get_device_guid(drv_key, flow, pulse_name, &guids[i]);
    }
    if (drv_key)
        RegCloseKey(drv_key);

end:
    HeapFree(GetProcessHeap(), 0, params.endpoints);
    if (FAILED(params.result)) {
        HeapFree(GetProcessHeap(), 0, guids);
        while (i--) HeapFree(GetProcessHeap(), 0, ids[i]);
        HeapFree(GetProcessHeap(), 0, ids);
    } else {
        *ids_out = ids;
        *keys = guids;
        *num = params.num;
        *def_index = params.default_idx;
    }
    return params.result;
}

static BOOL get_pulse_name_by_guid(const GUID *guid, char pulse_name[MAX_PULSE_NAME_LEN], EDataFlow *flow)
{
    struct device_cache *device;
    WCHAR key_name[MAX_PULSE_NAME_LEN + 2];
    DWORD key_name_size;
    DWORD index = 0;
    HKEY key;

    /* Return empty string for default PulseAudio device */
    pulse_name[0] = 0;
    if (IsEqualGUID(guid, &pulse_render_guid)) {
        *flow = eRender;
        return TRUE;
    } else if (IsEqualGUID(guid, &pulse_capture_guid)) {
        *flow = eCapture;
        return TRUE;
    }

    /* Check the cache first */
    LIST_FOR_EACH_ENTRY(device, &g_devices_cache, struct device_cache, entry) {
        if (!IsEqualGUID(guid, &device->guid))
            continue;
        *flow = device->dataflow;
        strcpy(pulse_name, device->pulse_name);
        return TRUE;
    }

    if (RegOpenKeyExW(HKEY_CURRENT_USER, drv_key_devicesW, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) {
        WARN("No devices found in registry\n");
        return FALSE;
    }

    for (;;) {
        DWORD size, type;
        LSTATUS status;
        GUID reg_guid;
        HKEY dev_key;
        int len;

        key_name_size = ARRAY_SIZE(key_name);
        if (RegEnumKeyExW(key, index++, key_name, &key_name_size, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        if (RegOpenKeyExW(key, key_name, 0, KEY_READ | KEY_WOW64_64KEY, &dev_key) != ERROR_SUCCESS) {
            ERR("Couldn't open key: %s\n", wine_dbgstr_w(key_name));
            continue;
        }

        size = sizeof(reg_guid);
        status = RegQueryValueExW(dev_key, L"guid", 0, &type, (BYTE *)&reg_guid, &size);
        RegCloseKey(dev_key);

        if (status == ERROR_SUCCESS && type == REG_BINARY && size == sizeof(reg_guid) && IsEqualGUID(&reg_guid, guid)) {
            RegCloseKey(key);

            TRACE("Found matching device key: %s\n", wine_dbgstr_w(key_name));

            if (key_name[0] == '0')
                *flow = eRender;
            else if (key_name[0] == '1')
                *flow = eCapture;
            else {
                WARN("Unknown device type: %c\n", key_name[0]);
                return FALSE;
            }

            if (!(len = WideCharToMultiByte(CP_UNIXCP, 0, key_name + 2, -1, pulse_name, MAX_PULSE_NAME_LEN, NULL, NULL)))
                return FALSE;

            if ((device = malloc(FIELD_OFFSET(struct device_cache, pulse_name[len])))) {
                device->guid = reg_guid;
                device->dataflow = *flow;
                strcpy(device->pulse_name, pulse_name);
                list_add_tail(&g_devices_cache, &device->entry);
            }
            return TRUE;
        }
    }

    RegCloseKey(key);
    WARN("No matching device in registry for GUID %s\n", debugstr_guid(guid));
    return FALSE;
}

HRESULT WINAPI AUDDRV_GetAudioEndpoint(GUID *guid, IMMDevice *dev, IAudioClient **out)
{
    ACImpl *This;
    char pulse_name[MAX_PULSE_NAME_LEN];
    EDataFlow dataflow;
    unsigned len;
    HRESULT hr;

    TRACE("%s %p %p\n", debugstr_guid(guid), dev, out);

    if (!get_pulse_name_by_guid(guid, pulse_name, &dataflow))
        return AUDCLNT_E_DEVICE_INVALIDATED;

    *out = NULL;

    len = strlen(pulse_name) + 1;
    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FIELD_OFFSET(ACImpl, device_name[len]));
    if (!This)
        return E_OUTOFMEMORY;

    This->IAudioClient3_iface.lpVtbl = &AudioClient3_Vtbl;
    This->IAudioRenderClient_iface.lpVtbl = &AudioRenderClient_Vtbl;
    This->IAudioCaptureClient_iface.lpVtbl = &AudioCaptureClient_Vtbl;
    This->IAudioClock_iface.lpVtbl = &AudioClock_Vtbl;
    This->IAudioClock2_iface.lpVtbl = &AudioClock2_Vtbl;
    This->IAudioStreamVolume_iface.lpVtbl = &AudioStreamVolume_Vtbl;
    This->dataflow = dataflow;
    This->parent = dev;
    memcpy(This->device_name, pulse_name, len);

    hr = CoCreateFreeThreadedMarshaler((IUnknown*)&This->IAudioClient3_iface, &This->marshal);
    if (FAILED(hr)) {
        HeapFree(GetProcessHeap(), 0, This);
        return hr;
    }
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

    if (!ppv)
        return E_POINTER;

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioClient) ||
            IsEqualIID(riid, &IID_IAudioClient2) ||
            IsEqualIID(riid, &IID_IAudioClient3))
        *ppv = iface;
    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    if (IsEqualIID(riid, &IID_IMarshal))
        return IUnknown_QueryInterface(This->marshal, riid, ppv);

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
    if (!ref) {
        if (This->stream) {
            pulse_release_stream(This->stream, This->timer_thread);
            This->stream = 0;
            sessions_lock();
            list_remove(&This->entry);
            sessions_unlock();
        }
        IUnknown_Release(This->marshal);
        IMMDevice_Release(This->parent);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static void dump_fmt(const WAVEFORMATEX *fmt)
{
    TRACE("wFormatTag: 0x%x (", fmt->wFormatTag);
    switch(fmt->wFormatTag) {
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

    if (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        WAVEFORMATEXTENSIBLE *fmtex = (void*)fmt;
        TRACE("dwChannelMask: %08lx\n", fmtex->dwChannelMask);
        TRACE("Samples: %04x\n", fmtex->Samples.wReserved);
        TRACE("SubFormat: %s\n", wine_dbgstr_guid(&fmtex->SubFormat));
    }
}

static WAVEFORMATEX *clone_format(const WAVEFORMATEX *fmt)
{
    WAVEFORMATEX *ret;
    size_t size;

    if (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        size = sizeof(WAVEFORMATEXTENSIBLE);
    else
        size = sizeof(WAVEFORMATEX);

    ret = CoTaskMemAlloc(size);
    if (!ret)
        return NULL;

    memcpy(ret, fmt, size);

    ret->cbSize = size - sizeof(WAVEFORMATEX);

    return ret;
}

static void session_init_vols(AudioSession *session, UINT channels)
{
    if (session->channel_count < channels) {
        UINT i;

        if (session->channel_vols)
            session->channel_vols = HeapReAlloc(GetProcessHeap(), 0,
                    session->channel_vols, sizeof(float) * channels);
        else
            session->channel_vols = HeapAlloc(GetProcessHeap(), 0,
                    sizeof(float) * channels);
        if (!session->channel_vols)
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
    if (!ret)
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

    if (!sessionguid || IsEqualGUID(sessionguid, &GUID_NULL)) {
        *out = create_session(&GUID_NULL, device, channels);
        if (!*out)
            return E_OUTOFMEMORY;

        return S_OK;
    }

    *out = NULL;
    LIST_FOR_EACH_ENTRY(session, &g_sessions, AudioSession, entry) {
        if (session->device == device &&
            IsEqualGUID(sessionguid, &session->guid)) {
            session_init_vols(session, channels);
            *out = session;
            break;
        }
    }

    if (!*out) {
        *out = create_session(sessionguid, device, channels);
        if (!*out)
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
    unsigned int i, channel_count;
    stream_handle stream;
    WCHAR *name;
    HRESULT hr;

    TRACE("(%p)->(%x, %lx, %s, %s, %p, %s)\n", This, mode, flags,
          wine_dbgstr_longlong(duration), wine_dbgstr_longlong(period), fmt, debugstr_guid(sessionguid));

    if (!fmt)
        return E_POINTER;
    dump_fmt(fmt);

    if (mode != AUDCLNT_SHAREMODE_SHARED && mode != AUDCLNT_SHAREMODE_EXCLUSIVE)
        return E_INVALIDARG;
    if (mode == AUDCLNT_SHAREMODE_EXCLUSIVE)
        return AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED;

    if (flags & ~(AUDCLNT_STREAMFLAGS_CROSSPROCESS |
                AUDCLNT_STREAMFLAGS_LOOPBACK |
                AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
                AUDCLNT_STREAMFLAGS_NOPERSIST |
                AUDCLNT_STREAMFLAGS_RATEADJUST |
                AUDCLNT_SESSIONFLAGS_EXPIREWHENUNOWNED |
                AUDCLNT_SESSIONFLAGS_DISPLAY_HIDE |
                AUDCLNT_SESSIONFLAGS_DISPLAY_HIDEWHENEXPIRED |
                AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY |
                AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM)) {
        FIXME("Unknown flags: %08lx\n", flags);
        return E_INVALIDARG;
    }

    sessions_lock();

    if (This->stream) {
        sessions_unlock();
        return AUDCLNT_E_ALREADY_INITIALIZED;
    }

    if (!pulse_thread)
    {
        HANDLE event = CreateEventW(NULL, TRUE, FALSE, NULL);
        if (!(pulse_thread = CreateThread(NULL, 0, pulse_mainloop_thread, event, 0, NULL)))
        {
            ERR("Failed to create mainloop thread.\n");
            sessions_unlock();
            CloseHandle(event);
            return E_FAIL;
        }
        SetThreadPriority(pulse_thread, THREAD_PRIORITY_TIME_CRITICAL);
        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
    }

    params.name = name = get_application_name(TRUE);
    params.device   = This->device_name;
    params.flow     = This->dataflow;
    params.share    = mode;
    params.flags    = flags;
    params.duration = duration;
    params.period   = period;
    params.fmt      = fmt;
    params.stream   = &stream;
    params.channel_count = &channel_count;
    pulse_call(create_stream, &params);
    free(name);
    if (FAILED(hr = params.result))
    {
        sessions_unlock();
        return hr;
    }

    if (!(This->vols = malloc(channel_count * sizeof(*This->vols))))
    {
        pulse_release_stream(stream, NULL);
        sessions_unlock();
        return E_OUTOFMEMORY;
    }
    for (i = 0; i < channel_count; i++)
        This->vols[i] = 1.f;

    hr = get_audio_session(sessionguid, This->parent, channel_count, &This->session);
    if (FAILED(hr))
    {
        free(This->vols);
        This->vols = NULL;
        sessions_unlock();
        pulse_release_stream(stream, NULL);
        return E_OUTOFMEMORY;
    }

    This->stream = stream;
    This->channel_count = channel_count;
    list_add_tail(&This->session->clients, &This->entry);
    set_stream_volumes(This);

    sessions_unlock();
    return S_OK;
}

static HRESULT WINAPI AudioClient_GetBufferSize(IAudioClient3 *iface,
        UINT32 *out)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct get_buffer_size_params params;

    TRACE("(%p)->(%p)\n", This, out);

    if (!out)
        return E_POINTER;
    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    params.frames = out;
    pulse_call(get_buffer_size, &params);
    return params.result;
}

static HRESULT WINAPI AudioClient_GetStreamLatency(IAudioClient3 *iface,
        REFERENCE_TIME *latency)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct get_latency_params params;

    TRACE("(%p)->(%p)\n", This, latency);

    if (!latency)
        return E_POINTER;
    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream  = This->stream;
    params.latency = latency;
    pulse_call(get_latency, &params);
    return params.result;
}

static HRESULT WINAPI AudioClient_GetCurrentPadding(IAudioClient3 *iface,
        UINT32 *out)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct get_current_padding_params params;

    TRACE("(%p)->(%p)\n", This, out);

    if (!out)
        return E_POINTER;
    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream  = This->stream;
    params.padding = out;
    pulse_call(get_current_padding, &params);
    return params.result;
}

static HRESULT WINAPI AudioClient_IsFormatSupported(IAudioClient3 *iface,
        AUDCLNT_SHAREMODE mode, const WAVEFORMATEX *fmt,
        WAVEFORMATEX **out)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    HRESULT hr = S_OK;
    WAVEFORMATEX *closest = NULL;
    BOOL exclusive;

    TRACE("(%p)->(%x, %p, %p)\n", This, mode, fmt, out);

    if (!fmt)
        return E_POINTER;

    if (out)
        *out = NULL;

    if (mode == AUDCLNT_SHAREMODE_EXCLUSIVE) {
        exclusive = 1;
        out = NULL;
    } else if (mode == AUDCLNT_SHAREMODE_SHARED) {
        exclusive = 0;
        if (!out)
            return E_POINTER;
    } else
        return E_INVALIDARG;

    if (fmt->nChannels == 0)
        return AUDCLNT_E_UNSUPPORTED_FORMAT;

    closest = clone_format(fmt);
    if (!closest)
        return E_OUTOFMEMORY;

    dump_fmt(fmt);

    switch (fmt->wFormatTag) {
    case WAVE_FORMAT_EXTENSIBLE: {
        WAVEFORMATEXTENSIBLE *ext = (WAVEFORMATEXTENSIBLE*)closest;

        if ((fmt->cbSize != sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX) &&
             fmt->cbSize != sizeof(WAVEFORMATEXTENSIBLE)) ||
            fmt->nBlockAlign != fmt->wBitsPerSample / 8 * fmt->nChannels ||
            ext->Samples.wValidBitsPerSample > fmt->wBitsPerSample ||
            fmt->nAvgBytesPerSec != fmt->nBlockAlign * fmt->nSamplesPerSec) {
            hr = E_INVALIDARG;
            break;
        }

        if (exclusive) {
            UINT32 mask = 0, i, channels = 0;

            if (!(ext->dwChannelMask & (SPEAKER_ALL | SPEAKER_RESERVED))) {
                for (i = 1; !(i & SPEAKER_RESERVED); i <<= 1) {
                    if (i & ext->dwChannelMask) {
                        mask |= i;
                        channels++;
                    }
                }

                if (channels != fmt->nChannels || (ext->dwChannelMask & ~mask)) {
                    hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
                    break;
                }
            } else {
                hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
                break;
            }
        }

        if (IsEqualGUID(&ext->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
            if (fmt->wBitsPerSample != 32) {
                hr = E_INVALIDARG;
                break;
            }

            if (ext->Samples.wValidBitsPerSample != fmt->wBitsPerSample) {
                hr = S_FALSE;
                ext->Samples.wValidBitsPerSample = fmt->wBitsPerSample;
            }
        } else if (IsEqualGUID(&ext->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)) {
            if (!fmt->wBitsPerSample || fmt->wBitsPerSample > 32 || fmt->wBitsPerSample % 8) {
                hr = E_INVALIDARG;
                break;
            }

            if (ext->Samples.wValidBitsPerSample != fmt->wBitsPerSample &&
                !(fmt->wBitsPerSample == 32 &&
                  ext->Samples.wValidBitsPerSample == 24)) {
                hr = S_FALSE;
                ext->Samples.wValidBitsPerSample = fmt->wBitsPerSample;
                break;
            }
        } else {
            hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
            break;
        }

        break;
    }

    case WAVE_FORMAT_ALAW:
    case WAVE_FORMAT_MULAW:
        if (fmt->wBitsPerSample != 8) {
            hr = E_INVALIDARG;
            break;
        }
        /* Fall-through */
    case WAVE_FORMAT_IEEE_FLOAT:
        if (fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT && fmt->wBitsPerSample != 32) {
            hr = E_INVALIDARG;
            break;
        }
        /* Fall-through */
    case WAVE_FORMAT_PCM:
        if (fmt->wFormatTag == WAVE_FORMAT_PCM &&
            (!fmt->wBitsPerSample || fmt->wBitsPerSample > 32 || fmt->wBitsPerSample % 8)) {
            hr = E_INVALIDARG;
            break;
        }

        if (fmt->nChannels > 2) {
            hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
            break;
        }
        /*
         * fmt->cbSize, fmt->nBlockAlign and fmt->nAvgBytesPerSec seem to be
         * ignored, invalid values are happily accepted.
         */
        break;
    default:
        hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
        break;
    }

    if (exclusive && hr != S_OK) {
        hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
        CoTaskMemFree(closest);
    } else if (hr != S_FALSE)
        CoTaskMemFree(closest);
    else
        *out = closest;

    /* Winepulse does not currently support exclusive mode, if you know of an
     * application that uses it, I will correct this..
     */
    if (hr == S_OK && exclusive)
        return This->dataflow == eCapture ? AUDCLNT_E_UNSUPPORTED_FORMAT : AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED;

    TRACE("returning: %08lx %p\n", hr, out ? *out : NULL);
    return hr;
}

static HRESULT WINAPI AudioClient_GetMixFormat(IAudioClient3 *iface,
        WAVEFORMATEX **pwfx)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct get_mix_format_params params;

    TRACE("(%p)->(%p)\n", This, pwfx);

    if (!pwfx)
        return E_POINTER;
    *pwfx = NULL;

    params.device = This->device_name;
    params.flow = This->dataflow;
    params.fmt = CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
    if (!params.fmt)
        return E_OUTOFMEMORY;

    pulse_call(get_mix_format, &params);

    if (SUCCEEDED(params.result)) {
        *pwfx = &params.fmt->Format;
        dump_fmt(*pwfx);
    } else {
        CoTaskMemFree(params.fmt);
    }

    return params.result;
}

static HRESULT WINAPI AudioClient_GetDevicePeriod(IAudioClient3 *iface,
        REFERENCE_TIME *defperiod, REFERENCE_TIME *minperiod)
{
    struct get_device_period_params params;
    ACImpl *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(%p, %p)\n", This, defperiod, minperiod);

    if (!defperiod && !minperiod)
        return E_POINTER;

    params.flow = This->dataflow;
    params.device = This->device_name;
    params.def_period = defperiod;
    params.min_period = minperiod;

    pulse_call(get_device_period, &params);

    return params.result;
}

static HRESULT WINAPI AudioClient_Start(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct start_params params;
    HRESULT hr;

    TRACE("(%p)\n", This);

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    pulse_call(start, &params);
    if (FAILED(hr = params.result))
        return hr;

    if (!This->timer_thread) {
        This->timer_thread = CreateThread(NULL, 0, pulse_timer_cb, This, 0, NULL);
        SetThreadPriority(This->timer_thread, THREAD_PRIORITY_TIME_CRITICAL);
    }

    return S_OK;
}

static HRESULT WINAPI AudioClient_Stop(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct stop_params params;

    TRACE("(%p)\n", This);

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    pulse_call(stop, &params);
    return params.result;
}

static HRESULT WINAPI AudioClient_Reset(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct reset_params params;

    TRACE("(%p)\n", This);

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    pulse_call(reset, &params);
    return params.result;
}

static HRESULT WINAPI AudioClient_SetEventHandle(IAudioClient3 *iface,
        HANDLE event)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    struct set_event_handle_params params;

    TRACE("(%p)->(%p)\n", This, event);

    if (!event)
        return E_INVALIDARG;
    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    params.event  = event;
    pulse_call(set_event_handle, &params);
    return params.result;
}

static HRESULT WINAPI AudioClient_GetService(IAudioClient3 *iface, REFIID riid,
        void **ppv)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    if (IsEqualIID(riid, &IID_IAudioRenderClient)) {
        if (This->dataflow != eRender)
            return AUDCLNT_E_WRONG_ENDPOINT_TYPE;
        *ppv = &This->IAudioRenderClient_iface;
    } else if (IsEqualIID(riid, &IID_IAudioCaptureClient)) {
        if (This->dataflow != eCapture)
            return AUDCLNT_E_WRONG_ENDPOINT_TYPE;
        *ppv = &This->IAudioCaptureClient_iface;
    } else if (IsEqualIID(riid, &IID_IAudioClock)) {
        *ppv = &This->IAudioClock_iface;
    } else if (IsEqualIID(riid, &IID_IAudioStreamVolume)) {
        *ppv = &This->IAudioStreamVolume_iface;
    } else if (IsEqualIID(riid, &IID_IAudioSessionControl) ||
               IsEqualIID(riid, &IID_IChannelAudioVolume) ||
               IsEqualIID(riid, &IID_ISimpleAudioVolume)) {
        if (!This->session_wrapper) {
            This->session_wrapper = AudioSessionWrapper_Create(This);
            if (!This->session_wrapper)
                return E_OUTOFMEMORY;
        }
        if (IsEqualIID(riid, &IID_IAudioSessionControl))
            *ppv = &This->session_wrapper->IAudioSessionControl2_iface;
        else if (IsEqualIID(riid, &IID_IChannelAudioVolume))
            *ppv = &This->session_wrapper->IChannelAudioVolume_iface;
        else if (IsEqualIID(riid, &IID_ISimpleAudioVolume))
            *ppv = &This->session_wrapper->ISimpleAudioVolume_iface;
    }

    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

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

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAudioRenderClient))
        *ppv = iface;
    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    if (IsEqualIID(riid, &IID_IMarshal))
        return IUnknown_QueryInterface(This->marshal, riid, ppv);

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

    if (!data)
        return E_POINTER;
    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;
    *data = NULL;

    params.stream = This->stream;
    params.frames = frames;
    params.data   = data;
    pulse_call(get_render_buffer, &params);
    return params.result;
}

static HRESULT WINAPI AudioRenderClient_ReleaseBuffer(
        IAudioRenderClient *iface, UINT32 written_frames, DWORD flags)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    struct release_render_buffer_params params;

    TRACE("(%p)->(%u, %lx)\n", This, written_frames, flags);

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream         = This->stream;
    params.written_frames = written_frames;
    params.flags          = flags;
    pulse_call(release_render_buffer, &params);
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

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAudioCaptureClient))
        *ppv = iface;
    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    if (IsEqualIID(riid, &IID_IMarshal))
        return IUnknown_QueryInterface(This->marshal, riid, ppv);

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

    if (!data)
       return E_POINTER;
    *data = NULL;
    if (!frames || !flags)
        return E_POINTER;
    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    params.data   = data;
    params.frames = frames;
    params.flags  = (UINT*)flags;
    params.devpos = devpos;
    params.qpcpos = qpcpos;
    pulse_call(get_capture_buffer, &params);
    return params.result;
}

static HRESULT WINAPI AudioCaptureClient_ReleaseBuffer(
        IAudioCaptureClient *iface, UINT32 done)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    struct release_capture_buffer_params params;

    TRACE("(%p)->(%u)\n", This, done);

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    params.done   = done;
    pulse_call(release_capture_buffer, &params);
    return params.result;
}

static HRESULT WINAPI AudioCaptureClient_GetNextPacketSize(
        IAudioCaptureClient *iface, UINT32 *frames)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    struct get_next_packet_size_params params;

    TRACE("(%p)->(%p)\n", This, frames);

    if (!frames)
        return E_POINTER;
    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    params.frames = frames;
    pulse_call(get_next_packet_size, &params);
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

static HRESULT WINAPI AudioClock_QueryInterface(IAudioClock *iface,
        REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioClock(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IAudioClock))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IAudioClock2))
        *ppv = &This->IAudioClock2_iface;
    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    if (IsEqualIID(riid, &IID_IMarshal))
        return IUnknown_QueryInterface(This->marshal, riid, ppv);

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
    struct get_frequency_params params;

    TRACE("(%p)->(%p)\n", This, freq);

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    params.freq   = freq;
    pulse_call(get_frequency, &params);
    return params.result;
}

static HRESULT WINAPI AudioClock_GetPosition(IAudioClock *iface, UINT64 *pos,
        UINT64 *qpctime)
{
    ACImpl *This = impl_from_IAudioClock(iface);
    struct get_position_params params;

    TRACE("(%p)->(%p, %p)\n", This, pos, qpctime);

    if (!pos)
        return E_POINTER;
    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream  = This->stream;
    params.device  = FALSE;
    params.pos     = pos;
    params.qpctime = qpctime;
    pulse_call(get_position, &params);
    return params.result;
}

static HRESULT WINAPI AudioClock_GetCharacteristics(IAudioClock *iface,
        DWORD *chars)
{
    ACImpl *This = impl_from_IAudioClock(iface);

    TRACE("(%p)->(%p)\n", This, chars);

    if (!chars)
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
    struct get_position_params params;

    TRACE("(%p)->(%p, %p)\n", This, pos, qpctime);

    if (!pos)
        return E_POINTER;
    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream  = This->stream;
    params.device  = TRUE;
    params.pos     = pos;
    params.qpctime = qpctime;
    pulse_call(get_position, &params);
    return params.result;
}

static const IAudioClock2Vtbl AudioClock2_Vtbl =
{
    AudioClock2_QueryInterface,
    AudioClock2_AddRef,
    AudioClock2_Release,
    AudioClock2_GetDevicePosition
};

static HRESULT WINAPI AudioStreamVolume_QueryInterface(
        IAudioStreamVolume *iface, REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAudioStreamVolume))
        *ppv = iface;
    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    if (IsEqualIID(riid, &IID_IMarshal))
        return IUnknown_QueryInterface(This->marshal, riid, ppv);

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

    if (!out)
        return E_POINTER;

    *out = This->channel_count;

    return S_OK;
}

struct pulse_info_cb_data {
    UINT32 n;
    float *levels;
};

static HRESULT WINAPI AudioStreamVolume_SetAllVolumes(
        IAudioStreamVolume *iface, UINT32 count, const float *levels)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    int i;

    TRACE("(%p)->(%d, %p)\n", This, count, levels);

    if (!levels)
        return E_POINTER;

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (count != This->channel_count)
        return E_INVALIDARG;

    sessions_lock();
    for (i = 0; i < count; ++i)
        This->vols[i] = levels[i];

    set_stream_volumes(This);
    sessions_unlock();
    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_GetAllVolumes(
        IAudioStreamVolume *iface, UINT32 count, float *levels)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    int i;

    TRACE("(%p)->(%d, %p)\n", This, count, levels);

    if (!levels)
        return E_POINTER;

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (count != This->channel_count)
        return E_INVALIDARG;

    sessions_lock();
    for (i = 0; i < count; ++i)
        levels[i] = This->vols[i];
    sessions_unlock();
    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_SetChannelVolume(
        IAudioStreamVolume *iface, UINT32 index, float level)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%d, %f)\n", This, index, level);

    if (level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (index >= This->channel_count)
        return E_INVALIDARG;

    sessions_lock();
    This->vols[index] = level;
    set_stream_volumes(This);
    sessions_unlock();
    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_GetChannelVolume(
        IAudioStreamVolume *iface, UINT32 index, float *level)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%d, %p)\n", This, index, level);

    if (!level)
        return E_POINTER;

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (index >= This->channel_count)
        return E_INVALIDARG;

    sessions_lock();
    *level = This->vols[index];
    sessions_unlock();
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

static AudioSessionWrapper *AudioSessionWrapper_Create(ACImpl *client)
{
    AudioSessionWrapper *ret;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(AudioSessionWrapper));
    if (!ret)
        return NULL;

    ret->IAudioSessionControl2_iface.lpVtbl = &AudioSessionControl2_Vtbl;
    ret->ISimpleAudioVolume_iface.lpVtbl = &SimpleAudioVolume_Vtbl;
    ret->IChannelAudioVolume_iface.lpVtbl = &ChannelAudioVolume_Vtbl;

    ret->ref = !client;

    ret->client = client;
    if (client) {
        ret->session = client->session;
        IAudioClient3_AddRef(&client->IAudioClient3_iface);
    }

    return ret;
}

static HRESULT WINAPI SimpleAudioVolume_QueryInterface(
        ISimpleAudioVolume *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_ISimpleAudioVolume))
        *ppv = iface;
    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI SimpleAudioVolume_AddRef(ISimpleAudioVolume *iface)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    return IAudioSessionControl2_AddRef(&This->IAudioSessionControl2_iface);
}

static ULONG WINAPI SimpleAudioVolume_Release(ISimpleAudioVolume *iface)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    return IAudioSessionControl2_Release(&This->IAudioSessionControl2_iface);
}

static HRESULT WINAPI SimpleAudioVolume_SetMasterVolume(
        ISimpleAudioVolume *iface, float level, const GUID *context)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    AudioSession *session = This->session;
    ACImpl *client;

    TRACE("(%p)->(%f, %s)\n", session, level, wine_dbgstr_guid(context));

    if (level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if (context)
        FIXME("Notifications not supported yet\n");

    TRACE("PulseAudio does not support session volume control\n");

    sessions_lock();
    session->master_vol = level;
    LIST_FOR_EACH_ENTRY(client, &This->session->clients, ACImpl, entry)
        set_stream_volumes(client);
    sessions_unlock();

    return S_OK;
}

static HRESULT WINAPI SimpleAudioVolume_GetMasterVolume(
        ISimpleAudioVolume *iface, float *level)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%p)\n", session, level);

    if (!level)
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

    if (context)
        FIXME("Notifications not supported yet\n");

    sessions_lock();
    session->mute = mute;
    LIST_FOR_EACH_ENTRY(client, &This->session->clients, ACImpl, entry)
        set_stream_volumes(client);
    sessions_unlock();

    return S_OK;
}

static HRESULT WINAPI SimpleAudioVolume_GetMute(ISimpleAudioVolume *iface,
        BOOL *mute)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%p)\n", session, mute);

    if (!mute)
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

static HRESULT WINAPI ChannelAudioVolume_QueryInterface(
        IChannelAudioVolume *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IChannelAudioVolume))
        *ppv = iface;
    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ChannelAudioVolume_AddRef(IChannelAudioVolume *iface)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    return IAudioSessionControl2_AddRef(&This->IAudioSessionControl2_iface);
}

static ULONG WINAPI ChannelAudioVolume_Release(IChannelAudioVolume *iface)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    return IAudioSessionControl2_Release(&This->IAudioSessionControl2_iface);
}

static HRESULT WINAPI ChannelAudioVolume_GetChannelCount(
        IChannelAudioVolume *iface, UINT32 *out)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%p)\n", session, out);

    if (!out)
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

    if (level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if (index >= session->channel_count)
        return E_INVALIDARG;

    if (context)
        FIXME("Notifications not supported yet\n");

    TRACE("PulseAudio does not support session volume control\n");

    sessions_lock();
    session->channel_vols[index] = level;
    LIST_FOR_EACH_ENTRY(client, &This->session->clients, ACImpl, entry)
        set_stream_volumes(client);
    sessions_unlock();

    return S_OK;
}

static HRESULT WINAPI ChannelAudioVolume_GetChannelVolume(
        IChannelAudioVolume *iface, UINT32 index, float *level)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%d, %p)\n", session, index, level);

    if (!level)
        return NULL_PTR_ERR;

    if (index >= session->channel_count)
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
    ACImpl *client;
    int i;

    TRACE("(%p)->(%d, %p, %s)\n", session, count, levels,
            wine_dbgstr_guid(context));

    if (!levels)
        return NULL_PTR_ERR;

    if (count != session->channel_count)
        return E_INVALIDARG;

    if (context)
        FIXME("Notifications not supported yet\n");

    TRACE("PulseAudio does not support session volume control\n");

    sessions_lock();
    for(i = 0; i < count; ++i)
        session->channel_vols[i] = levels[i];
    LIST_FOR_EACH_ENTRY(client, &This->session->clients, ACImpl, entry)
        set_stream_volumes(client);
    sessions_unlock();
    return S_OK;
}

static HRESULT WINAPI ChannelAudioVolume_GetAllVolumes(
        IChannelAudioVolume *iface, UINT32 count, float *levels)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;
    int i;

    TRACE("(%p)->(%d, %p)\n", session, count, levels);

    if (!levels)
        return NULL_PTR_ERR;

    if (count != session->channel_count)
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
    char pulse_name[MAX_PULSE_NAME_LEN];
    unsigned int size = 0;

    TRACE("%s, (%s,%lu), %p\n", wine_dbgstr_guid(guid), wine_dbgstr_guid(&prop->fmtid), prop->pid, out);

    if (!get_pulse_name_by_guid(guid, pulse_name, &params.flow))
        return E_FAIL;

    params.device = pulse_name;
    params.guid = guid;
    params.prop = prop;
    params.value = out;
    params.buffer = NULL;
    params.buffer_size = &size;

    while(1) {
        pulse_call(get_prop_value, &params);

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
