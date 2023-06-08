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
extern const IAudioRenderClientVtbl AudioRenderClient_Vtbl;
extern const IAudioCaptureClientVtbl AudioCaptureClient_Vtbl;
extern const IAudioSessionControl2Vtbl AudioSessionControl2_Vtbl;
extern const ISimpleAudioVolumeVtbl SimpleAudioVolume_Vtbl;
extern const IChannelAudioVolumeVtbl ChannelAudioVolume_Vtbl;
extern const IAudioClockVtbl AudioClock_Vtbl;
extern const IAudioClock2Vtbl AudioClock2_Vtbl;
extern const IAudioStreamVolumeVtbl AudioStreamVolume_Vtbl;

extern struct audio_session_wrapper *session_wrapper_create(
    struct audio_client *client) DECLSPEC_HIDDEN;

static inline ACImpl *impl_from_IAudioClient3(IAudioClient3 *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioClient3_iface);
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
        if (found)
        {
            name = wcsdup(productname);
            free(data);
            return name;
        }
        free(data);
    }

    name = wcsrchr(path, '\\');
    if (!name)
        name = path;
    else
        name++;
    return wcsdup(name);
}

static void set_stream_volumes(ACImpl *This)
{
    struct set_volumes_params params;
    params.stream          = This->stream;
    params.master_volume   = This->session->mute ? 0.0f : This->session->master_vol;
    params.volumes         = This->vols;
    params.session_volumes = This->session->channel_vols;
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

extern HRESULT WINAPI client_GetBufferSize(IAudioClient3 *iface,
        UINT32 *out);

extern HRESULT WINAPI client_GetStreamLatency(IAudioClient3 *iface,
        REFERENCE_TIME *latency);

extern HRESULT WINAPI client_GetCurrentPadding(IAudioClient3 *iface,
        UINT32 *out);

extern HRESULT WINAPI client_IsFormatSupported(IAudioClient3 *iface,
        AUDCLNT_SHAREMODE mode, const WAVEFORMATEX *fmt,
        WAVEFORMATEX **out);

extern HRESULT WINAPI client_GetMixFormat(IAudioClient3 *iface,
        WAVEFORMATEX **pwfx);

extern HRESULT WINAPI client_GetDevicePeriod(IAudioClient3 *iface,
        REFERENCE_TIME *defperiod, REFERENCE_TIME *minperiod);

extern HRESULT WINAPI client_Start(IAudioClient3 *iface);

extern HRESULT WINAPI client_Stop(IAudioClient3 *iface);

extern HRESULT WINAPI client_Reset(IAudioClient3 *iface);

extern HRESULT WINAPI client_SetEventHandle(IAudioClient3 *iface,
        HANDLE event);

extern HRESULT WINAPI client_GetService(IAudioClient3 *iface, REFIID riid,
        void **ppv);

extern HRESULT WINAPI client_IsOffloadCapable(IAudioClient3 *iface,
        AUDIO_STREAM_CATEGORY category, BOOL *offload_capable);

extern HRESULT WINAPI client_SetClientProperties(IAudioClient3 *iface,
        const AudioClientProperties *prop);

extern HRESULT WINAPI client_GetBufferSizeLimits(IAudioClient3 *iface,
        const WAVEFORMATEX *format, BOOL event_driven, REFERENCE_TIME *min_duration,
        REFERENCE_TIME *max_duration);

extern HRESULT WINAPI client_GetSharedModeEnginePeriod(IAudioClient3 *iface,
        const WAVEFORMATEX *format, UINT32 *default_period_frames, UINT32 *unit_period_frames,
        UINT32 *min_period_frames, UINT32 *max_period_frames);

extern HRESULT WINAPI client_GetCurrentSharedModeEnginePeriod(IAudioClient3 *iface,
        WAVEFORMATEX **cur_format, UINT32 *cur_period_frames);

extern HRESULT WINAPI client_InitializeSharedAudioStream(IAudioClient3 *iface,
        DWORD flags, UINT32 period_frames, const WAVEFORMATEX *format,
        const GUID *session_guid);

static const IAudioClient3Vtbl AudioClient3_Vtbl =
{
    AudioClient_QueryInterface,
    AudioClient_AddRef,
    AudioClient_Release,
    AudioClient_Initialize,
    client_GetBufferSize,
    client_GetStreamLatency,
    client_GetCurrentPadding,
    client_IsFormatSupported,
    client_GetMixFormat,
    client_GetDevicePeriod,
    client_Start,
    client_Stop,
    client_Reset,
    client_SetEventHandle,
    client_GetService,
    client_IsOffloadCapable,
    client_SetClientProperties,
    client_GetBufferSizeLimits,
    client_GetSharedModeEnginePeriod,
    client_GetCurrentSharedModeEnginePeriod,
    client_InitializeSharedAudioStream,
};

HRESULT WINAPI AUDDRV_GetAudioSessionWrapper(const GUID *guid, IMMDevice *device,
                                             AudioSessionWrapper **out)
{
    AudioSession *session;

    HRESULT hr = get_audio_session(guid, device, 0, &session);
    if(FAILED(hr))
        return hr;

    *out = session_wrapper_create(NULL);
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
