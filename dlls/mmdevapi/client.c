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

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include <wchar.h>

#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <winternl.h>

#include <wine/debug.h>
#include <wine/unixlib.h>

#include "mmdevapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmdevapi);

typedef struct tagLANGANDCODEPAGE
{
    WORD wLanguage;
    WORD wCodePage;
} LANGANDCODEPAGE;

extern void sessions_lock(void);
extern void sessions_unlock(void);

extern HRESULT get_audio_session(const GUID *sessionguid, IMMDevice *device, UINT channels,
                                 struct audio_session **out);
extern struct audio_session_wrapper *session_wrapper_create(struct audio_client *client);

static HANDLE main_loop_thread;

void main_loop_stop(void)
{
    if (main_loop_thread) {
        WaitForSingleObject(main_loop_thread, INFINITE);
        CloseHandle(main_loop_thread);
    }
}

void set_stream_volumes(struct audio_client *This)
{
    struct set_volumes_params params;

    params.stream          = This->stream;
    params.master_volume   = (This->session->mute ? 0.0f : This->session->master_vol);
    params.volumes         = This->vols;
    params.session_volumes = This->session->channel_vols;

    wine_unix_call(set_volumes, &params);
}

static inline struct audio_client *impl_from_IAudioCaptureClient(IAudioCaptureClient *iface)
{
    return CONTAINING_RECORD(iface, struct audio_client, IAudioCaptureClient_iface);
}

static inline struct audio_client *impl_from_IAudioClient3(IAudioClient3 *iface)
{
    return CONTAINING_RECORD(iface, struct audio_client, IAudioClient3_iface);
}

static inline struct audio_client *impl_from_IAudioClock(IAudioClock *iface)
{
    return CONTAINING_RECORD(iface, struct audio_client, IAudioClock_iface);
}

static inline struct audio_client *impl_from_IAudioClock2(IAudioClock2 *iface)
{
    return CONTAINING_RECORD(iface, struct audio_client, IAudioClock2_iface);
}

static inline struct audio_client *impl_from_IAudioClockAdjustment(IAudioClockAdjustment *iface)
{
    return CONTAINING_RECORD(iface, struct audio_client, IAudioClockAdjustment_iface);
}

static inline struct audio_client *impl_from_IAudioRenderClient(IAudioRenderClient *iface)
{
    return CONTAINING_RECORD(iface, struct audio_client, IAudioRenderClient_iface);
}

static inline struct audio_client *impl_from_IAudioStreamVolume(IAudioStreamVolume *iface)
{
    return CONTAINING_RECORD(iface, struct audio_client, IAudioStreamVolume_iface);
}

static HRESULT get_periods(struct audio_client *client,
                           REFERENCE_TIME *def_period, REFERENCE_TIME *min_period)
{
    static const REFERENCE_TIME min_def_period = 100000; /* 10 ms */
    struct get_device_period_params params;

    params.device     = client->device_name;
    params.flow       = client->dataflow;
    params.def_period = def_period;
    params.min_period = min_period;

    wine_unix_call(get_device_period, &params);

    if (def_period) *def_period = max(*def_period, min_def_period);

    return params.result;
}

static HRESULT adjust_timing(struct audio_client *client, const BOOLEAN force_def_period,
                             REFERENCE_TIME *duration, REFERENCE_TIME *period,
                             const AUDCLNT_SHAREMODE mode, const DWORD flags,
                             const WAVEFORMATEX *fmt)
{
    REFERENCE_TIME def_period, min_period;
    HRESULT hr;

    TRACE("Requested duration %lu and period %lu\n", (ULONG)*duration, (ULONG)*period);

    if (FAILED(hr = get_periods(client, &def_period, &min_period)))
        return hr;

    TRACE("Device periods: %lu default and %lu minimum\n", (ULONG)def_period, (ULONG)min_period);

    if (mode == AUDCLNT_SHAREMODE_SHARED) {
        if (*period == 0 || force_def_period)
            *period = def_period;
        else if (*period < min_period)
            return AUDCLNT_E_INVALID_DEVICE_PERIOD;
        if (*duration < 3 * *period)
            *duration = 3 * *period;
    } else {
        const WAVEFORMATEXTENSIBLE *fmtex = (WAVEFORMATEXTENSIBLE *)fmt;
        if (fmtex->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
           (fmtex->dwChannelMask == 0 || fmtex->dwChannelMask & SPEAKER_RESERVED))
            return AUDCLNT_E_UNSUPPORTED_FORMAT;
        else {
            if (*period == 0)
                *period = def_period;
            if (*period < min_period || *period > 5000000)
                return AUDCLNT_E_INVALID_DEVICE_PERIOD;
            else if (*duration > 20000000) /* The smaller the period, the lower this limit. */
                return AUDCLNT_E_BUFFER_SIZE_ERROR;
            else if (flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK) {
                if (*duration != *period)
                    return AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL;

                FIXME("EXCLUSIVE mode with EVENTCALLBACK\n");

                return AUDCLNT_E_DEVICE_IN_USE;
            } else if (*duration < 8 * *period)
                *duration = 8 * *period; /* May grow above 2s. */
        }
    }

    TRACE("Adjusted duration %lu and period %lu\n", (ULONG)*duration, (ULONG)*period);

    return hr;
}

static void dump_fmt(const WAVEFORMATEX *fmt)
{
    TRACE("wFormatTag: 0x%x (", fmt->wFormatTag);
    switch (fmt->wFormatTag) {
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
        WAVEFORMATEXTENSIBLE *fmtex = (void *)fmt;
        TRACE("dwChannelMask: %08lx\n", fmtex->dwChannelMask);
        TRACE("Samples: %04x\n", fmtex->Samples.wReserved);
        TRACE("SubFormat: %s\n", wine_dbgstr_guid(&fmtex->SubFormat));
    }
}

static DWORD CALLBACK main_loop_func(void *event)
{
    struct main_loop_params params;

    SetThreadDescription(GetCurrentThread(), L"audio_client_main");

    params.event = event;

    wine_unix_call(main_loop, &params);

    return 0;
}

HRESULT main_loop_start(void)
{
    if (!main_loop_thread) {
        HANDLE event = CreateEventW(NULL, TRUE, FALSE, NULL);
        if (!(main_loop_thread = CreateThread(NULL, 0, main_loop_func, event, 0, NULL))) {
            ERR("Failed to create main loop thread\n");
            CloseHandle(event);
            return E_FAIL;
        }

        SetThreadPriority(main_loop_thread, THREAD_PRIORITY_TIME_CRITICAL);
        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
    }

    return S_OK;
}

static DWORD CALLBACK timer_loop_func(void *user)
{
    struct timer_loop_params params;
    struct audio_client *This = user;

    SetThreadDescription(GetCurrentThread(), L"audio_client_timer");

    params.stream = This->stream;

    wine_unix_call(timer_loop, &params);

    return 0;
}

HRESULT stream_release(stream_handle stream, HANDLE timer_thread)
{
    struct release_stream_params params;

    params.stream       = stream;
    params.timer_thread = timer_thread;

    wine_unix_call(release_stream, &params);

    return params.result;
}

static BOOL query_productname(void *data, LANGANDCODEPAGE *lang, LPVOID *buffer, UINT *len)
{
    WCHAR pn[37];
    swprintf(pn, ARRAY_SIZE(pn), L"\\StringFileInfo\\%04x%04x\\ProductName", lang->wLanguage, lang->wCodePage);
    return VerQueryValueW(data, pn, buffer, len) && *len;
}

WCHAR *get_application_name(void)
{
    WCHAR path[MAX_PATH], *name;
    UINT translate_size, productname_size;
    LANGANDCODEPAGE *translate;
    LPVOID productname;
    BOOL found = FALSE;
    void *data = NULL;
    unsigned int i;
    LCID locale;
    DWORD size;

    GetModuleFileNameW(NULL, path, ARRAY_SIZE(path));

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

    /* No translations found. */
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
    if (found) {
        name = wcsdup(productname);
        free(data);
        return name;
    }

    free(data);

    name = wcsrchr(path, '\\');
    if (!name)
        name = path;
    else
        name++;

    return wcsdup(name);
}

static HRESULT stream_init(struct audio_client *client, const BOOLEAN force_def_period,
                           const AUDCLNT_SHAREMODE mode, const DWORD flags,
                           REFERENCE_TIME duration, REFERENCE_TIME period,
                           const WAVEFORMATEX *fmt, const GUID *sessionguid)
{
    struct create_stream_params params;
    UINT32 i, channel_count;
    stream_handle stream;
    WCHAR *name;

    if (!fmt)
        return E_POINTER;

    dump_fmt(fmt);

    if (mode != AUDCLNT_SHAREMODE_SHARED && mode != AUDCLNT_SHAREMODE_EXCLUSIVE)
        return E_INVALIDARG;

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

    if (mode == AUDCLNT_SHAREMODE_SHARED) {
        WAVEFORMATEX *mix_fmt;
        HRESULT hr;

        if (FAILED(hr = IAudioClient3_GetMixFormat(&client->IAudioClient3_iface, &mix_fmt)))
            return hr;

        if (fmt->nChannels != mix_fmt->nChannels || fmt->nSamplesPerSec != mix_fmt->nSamplesPerSec)
        {
            CoTaskMemFree(mix_fmt);
            return AUDCLNT_E_UNSUPPORTED_FORMAT;
        }

        CoTaskMemFree(mix_fmt);
    }

    sessions_lock();

    if (client->stream) {
        sessions_unlock();
        return AUDCLNT_E_ALREADY_INITIALIZED;
    }

    if (FAILED(params.result = main_loop_start())) {
        sessions_unlock();
        return params.result;
    }

    if (flags & AUDCLNT_STREAMFLAGS_LOOPBACK) {
        struct get_loopback_capture_device_params params;

        if (client->dataflow != eRender) {
            sessions_unlock();
            return AUDCLNT_E_WRONG_ENDPOINT_TYPE;
        }

        params.device = client->device_name;
        params.name = name = get_application_name();
        params.ret_device_len = 0;
        params.ret_device = NULL;
        params.result = E_NOTIMPL;
        wine_unix_call(get_loopback_capture_device, &params);
        while (params.result == STATUS_BUFFER_TOO_SMALL) {
            free(params.ret_device);
            params.ret_device = malloc(params.ret_device_len);
            wine_unix_call(get_loopback_capture_device, &params);
        }
        free(name);
        if (FAILED(params.result)) {
            sessions_unlock();
            free(params.ret_device);
            if (params.result == E_NOTIMPL)
                FIXME("get_loopback_capture_device is not supported by backend.\n");
            return params.result;
        }
        free(client->device_name);
        client->device_name = params.ret_device;
        client->dataflow = eCapture;
    }

    if (FAILED(params.result = adjust_timing(client, force_def_period, &duration, &period, mode, flags, fmt))) {
        sessions_unlock();
        return params.result;
    }

    params.name = name   = get_application_name();
    params.device        = client->device_name;
    params.flow          = client->dataflow;
    params.share         = mode;
    params.flags         = flags;
    params.duration      = duration;
    params.period        = period;
    params.fmt           = fmt;
    params.channel_count = &channel_count;
    params.stream        = &stream;

    wine_unix_call(create_stream, &params);

    free(name);

    if (FAILED(params.result)) {
        sessions_unlock();
        return params.result;
    }

    if (!(client->vols = malloc(channel_count * sizeof(*client->vols)))) {
        params.result = E_OUTOFMEMORY;
        goto exit;
    }

    for (i = 0; i < channel_count; i++)
        client->vols[i] = 1.f;

    params.result = get_audio_session(sessionguid, client->parent, channel_count, &client->session);

exit:
    if (FAILED(params.result)) {
        stream_release(stream, NULL);
        free(client->vols);
        client->vols = NULL;
    } else {
        list_add_tail(&client->session->clients, &client->entry);
        client->stream = stream;
        client->channel_count = channel_count;
        set_stream_volumes(client);
    }

    sessions_unlock();

    return params.result;
}

static HRESULT WINAPI capture_QueryInterface(IAudioCaptureClient *iface, REFIID riid, void **ppv)
{
    struct audio_client *This = impl_from_IAudioCaptureClient(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAudioCaptureClient))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IMarshal)) {
        return IUnknown_QueryInterface(This->marshal, riid, ppv);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);

    return S_OK;
}

static ULONG WINAPI capture_AddRef(IAudioCaptureClient *iface)
{
    struct audio_client *This = impl_from_IAudioCaptureClient(iface);
    return IAudioClient3_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI capture_Release(IAudioCaptureClient *iface)
{
    struct audio_client *This = impl_from_IAudioCaptureClient(iface);
    return IAudioClient3_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI capture_GetBuffer(IAudioCaptureClient *iface, BYTE **data, UINT32 *frames,
                                        DWORD *flags, UINT64 *devpos, UINT64 *qpcpos)
{
    struct audio_client *This = impl_from_IAudioCaptureClient(iface);
    struct get_capture_buffer_params params;

    TRACE("(%p)->(%p, %p, %p, %p, %p)\n", This, data, frames, flags, devpos, qpcpos);

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
    params.flags  = (UINT *)flags;
    params.devpos = devpos;
    params.qpcpos = qpcpos;

    wine_unix_call(get_capture_buffer, &params);

    return params.result;
}

static HRESULT WINAPI capture_ReleaseBuffer(IAudioCaptureClient *iface, UINT32 done)
{
    struct audio_client *This = impl_from_IAudioCaptureClient(iface);
    struct release_capture_buffer_params params;

    TRACE("(%p)->(%u)\n", This, done);

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    params.done   = done;

    wine_unix_call(release_capture_buffer, &params);

    return params.result;
}

static HRESULT WINAPI capture_GetNextPacketSize(IAudioCaptureClient *iface, UINT32 *frames)
{
    struct audio_client *This = impl_from_IAudioCaptureClient(iface);
    struct get_next_packet_size_params params;

    TRACE("(%p)->(%p)\n", This, frames);

    if (!frames)
        return E_POINTER;

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    params.frames = frames;

    wine_unix_call(get_next_packet_size, &params);

    return params.result;
}

const IAudioCaptureClientVtbl AudioCaptureClient_Vtbl =
{
    capture_QueryInterface,
    capture_AddRef,
    capture_Release,
    capture_GetBuffer,
    capture_ReleaseBuffer,
    capture_GetNextPacketSize
};

static HRESULT WINAPI client_QueryInterface(IAudioClient3 *iface, REFIID riid, void **ppv)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAudioClient) ||
        IsEqualIID(riid, &IID_IAudioClient2) ||
        IsEqualIID(riid, &IID_IAudioClient3))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IAudioClockAdjustment))
        *ppv = &This->IAudioClockAdjustment_iface;
    else if(IsEqualIID(riid, &IID_IMarshal)) {
        struct audio_client *This = impl_from_IAudioClient3(iface);
        return IUnknown_QueryInterface(This->marshal, riid, ppv);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);

    return S_OK;
}

static ULONG WINAPI client_AddRef(IAudioClient3 *iface)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) Refcount now %lu\n", This, ref);
    return ref;
}

static ULONG WINAPI client_Release(IAudioClient3 *iface)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) Refcount now %lu\n", This, ref);

    if (!ref) {
        IAudioClient3_Stop(iface);
        IMMDevice_Release(This->parent);
        IUnknown_Release(This->marshal);

        if (This->session) {
            sessions_lock();
            list_remove(&This->entry);
            sessions_unlock();
        }

        free(This->vols);

        if (This->stream)
            stream_release(This->stream, This->timer_thread);

        free(This->device_name);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI client_Initialize(IAudioClient3 *iface, AUDCLNT_SHAREMODE mode, DWORD flags,
                                 REFERENCE_TIME duration, REFERENCE_TIME period,
                                 const WAVEFORMATEX *fmt, const GUID *sessionguid)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(%x, %lx, %s, %s, %p, %s)\n", This, mode, flags, wine_dbgstr_longlong(duration),
                                               wine_dbgstr_longlong(period), fmt,
                                               debugstr_guid(sessionguid));

    return stream_init(This, TRUE, mode, flags, duration, period, fmt, sessionguid);
}

static HRESULT WINAPI client_GetBufferSize(IAudioClient3 *iface, UINT32 *out)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    struct get_buffer_size_params params;

    TRACE("(%p)->(%p)\n", This, out);

    if (!out)
        return E_POINTER;

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    params.frames = out;

    wine_unix_call(get_buffer_size, &params);

    return params.result;
}

static HRESULT WINAPI client_GetStreamLatency(IAudioClient3 *iface, REFERENCE_TIME *latency)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    struct get_latency_params params;

    TRACE("(%p)->(%p)\n", This, latency);

    if (!latency)
        return E_POINTER;

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream  = This->stream;
    params.latency = latency;

    wine_unix_call(get_latency, &params);

    return params.result;
}

static HRESULT WINAPI client_GetCurrentPadding(IAudioClient3 *iface, UINT32 *out)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    struct get_current_padding_params params;

    TRACE("(%p)->(%p)\n", This, out);

    if (!out)
        return E_POINTER;

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream  = This->stream;
    params.padding = out;

    wine_unix_call(get_current_padding, &params);

    return params.result;
}

static HRESULT WINAPI client_IsFormatSupported(IAudioClient3 *iface, AUDCLNT_SHAREMODE mode,
                                        const WAVEFORMATEX *fmt, WAVEFORMATEX **out)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    struct is_format_supported_params params;

    TRACE("(%p)->(%x, %p, %p)\n", This, mode, fmt, out);

    if (fmt) {
        dump_fmt(fmt);

        if (mode == AUDCLNT_SHAREMODE_SHARED) {
            WAVEFORMATEX *mix_fmt;
            HRESULT hr;

            if (FAILED(hr = IAudioClient3_GetMixFormat(iface, &mix_fmt)))
                return hr;

            if (fmt->nChannels != mix_fmt->nChannels || fmt->nSamplesPerSec != mix_fmt->nSamplesPerSec) {
                *out = mix_fmt;
                return S_FALSE;
            }

            CoTaskMemFree(mix_fmt);
        }
    }

    params.device  = This->device_name;
    params.flow    = This->dataflow;
    params.share   = mode;
    params.fmt_in  = fmt;
    params.fmt_out = NULL;

    if (out) {
        *out = NULL;
        if (mode == AUDCLNT_SHAREMODE_SHARED)
            params.fmt_out = CoTaskMemAlloc(sizeof(*params.fmt_out));
    }

    wine_unix_call(is_format_supported, &params);

    if (params.result == S_FALSE)
        *out = &params.fmt_out->Format;
    else
        CoTaskMemFree(params.fmt_out);

    return params.result;
}

static HRESULT WINAPI client_GetMixFormat(IAudioClient3 *iface, WAVEFORMATEX **pwfx)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    struct get_mix_format_params params;

    TRACE("(%p)->(%p)\n", This, pwfx);

    if (!pwfx)
        return E_POINTER;

    *pwfx = NULL;

    params.device = This->device_name;
    params.flow   = This->dataflow;
    params.fmt    = CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
    if (!params.fmt)
        return E_OUTOFMEMORY;

    wine_unix_call(get_mix_format, &params);

    if (SUCCEEDED(params.result)) {
        *pwfx = &params.fmt->Format;
        dump_fmt(*pwfx);
    } else
        CoTaskMemFree(params.fmt);

    return params.result;
}

static HRESULT WINAPI client_GetDevicePeriod(IAudioClient3 *iface, REFERENCE_TIME *defperiod,
                                      REFERENCE_TIME *minperiod)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(%p, %p)\n", This, defperiod, minperiod);

    if (!defperiod && !minperiod)
        return E_POINTER;

    return get_periods(This, defperiod, minperiod);
}

static HRESULT WINAPI client_Start(IAudioClient3 *iface)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    struct start_params params;

    TRACE("(%p)\n", This);

    sessions_lock();

    if (!This->stream) {
        sessions_unlock();
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    params.stream = This->stream;
    wine_unix_call(start, &params);

    if (SUCCEEDED(params.result) && !This->timer_thread) {
        if ((This->timer_thread = CreateThread(NULL, 0, timer_loop_func, This, 0, NULL)))
            SetThreadPriority(This->timer_thread, THREAD_PRIORITY_TIME_CRITICAL);
        else {
            IAudioClient3_Stop(&This->IAudioClient3_iface);
            params.result = E_FAIL;
        }
    }

    sessions_unlock();

    return params.result;
}

static HRESULT WINAPI client_Stop(IAudioClient3 *iface)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    struct stop_params params;

    TRACE("(%p)\n", This);

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;

    wine_unix_call(stop, &params);

    return params.result;
}

static HRESULT WINAPI client_Reset(IAudioClient3 *iface)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    struct reset_params params;

    TRACE("(%p)\n", This);

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;

    wine_unix_call(reset, &params);

    return params.result;
}

static HRESULT WINAPI client_SetEventHandle(IAudioClient3 *iface, HANDLE event)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    struct set_event_handle_params params;

    TRACE("(%p)->(%p)\n", This, event);

    if (!event)
        return E_INVALIDARG;

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    params.event  = event;

    wine_unix_call(set_event_handle, &params);

    return params.result;
}

static HRESULT WINAPI client_GetService(IAudioClient3 *iface, REFIID riid, void **ppv)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    HRESULT hr;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    *ppv = NULL;

    sessions_lock();

    if (!This->stream) {
        hr = AUDCLNT_E_NOT_INITIALIZED;
        goto exit;
    }

    if (IsEqualIID(riid, &IID_IAudioRenderClient)) {
        if (This->dataflow != eRender) {
            hr = AUDCLNT_E_WRONG_ENDPOINT_TYPE;
            goto exit;
        }

        IAudioRenderClient_AddRef(&This->IAudioRenderClient_iface);
        *ppv = &This->IAudioRenderClient_iface;
    } else if (IsEqualIID(riid, &IID_IAudioCaptureClient)) {
        if (This->dataflow != eCapture) {
            hr = AUDCLNT_E_WRONG_ENDPOINT_TYPE;
            goto exit;
        }

        IAudioCaptureClient_AddRef(&This->IAudioCaptureClient_iface);
        *ppv = &This->IAudioCaptureClient_iface;
    } else if (IsEqualIID(riid, &IID_IAudioClock)) {
        IAudioClock_AddRef(&This->IAudioClock_iface);
        *ppv = &This->IAudioClock_iface;
    } else if (IsEqualIID(riid, &IID_IAudioStreamVolume)) {
        IAudioStreamVolume_AddRef(&This->IAudioStreamVolume_iface);
        *ppv = &This->IAudioStreamVolume_iface;
    } else if (IsEqualIID(riid, &IID_IAudioSessionControl) ||
               IsEqualIID(riid, &IID_IChannelAudioVolume) ||
               IsEqualIID(riid, &IID_ISimpleAudioVolume)) {
        const BOOLEAN new_session = !This->session_wrapper;
        if (new_session) {
            This->session_wrapper = session_wrapper_create(This);
            if (!This->session_wrapper) {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
        }

        if (IsEqualIID(riid, &IID_IAudioSessionControl))
            *ppv = &This->session_wrapper->IAudioSessionControl2_iface;
        else if (IsEqualIID(riid, &IID_IChannelAudioVolume))
            *ppv = &This->session_wrapper->IChannelAudioVolume_iface;
        else if (IsEqualIID(riid, &IID_ISimpleAudioVolume))
            *ppv = &This->session_wrapper->ISimpleAudioVolume_iface;

        if (!new_session)
            IUnknown_AddRef((IUnknown *)*ppv);
    } else if (IsEqualIID(riid, &IID_IAudioClockAdjustment)) {
        IAudioClockAdjustment_AddRef(&This->IAudioClockAdjustment_iface);
        *ppv = &This->IAudioClockAdjustment_iface;
    } else {
            FIXME("stub %s\n", debugstr_guid(riid));
            hr = E_NOINTERFACE;
            goto exit;
    }

    hr = S_OK;
exit:
    sessions_unlock();

    return hr;
}

static HRESULT WINAPI client_IsOffloadCapable(IAudioClient3 *iface, AUDIO_STREAM_CATEGORY category,
                                       BOOL *offload_capable)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(0x%x, %p)\n", This, category, offload_capable);

    if (!offload_capable)
        return E_INVALIDARG;

    *offload_capable = FALSE;

    return S_OK;
}

static HRESULT WINAPI client_SetClientProperties(IAudioClient3 *iface,
                                          const AudioClientProperties *prop)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    const Win8AudioClientProperties *legacy_prop = (const Win8AudioClientProperties *)prop;

    TRACE("(%p)->(%p)\n", This, prop);

    if (!legacy_prop)
        return E_POINTER;

    if (legacy_prop->cbSize == sizeof(AudioClientProperties)) {
        TRACE("{ bIsOffload: %u, eCategory: 0x%x, Options: 0x%x }\n", legacy_prop->bIsOffload,
                                                                      legacy_prop->eCategory,
                                                                      prop->Options);
    } else if(legacy_prop->cbSize == sizeof(Win8AudioClientProperties)) {
        TRACE("{ bIsOffload: %u, eCategory: 0x%x }\n", legacy_prop->bIsOffload,
                                                       legacy_prop->eCategory);
    } else {
        WARN("Unsupported Size = %d\n", legacy_prop->cbSize);
        return E_INVALIDARG;
    }

    if (legacy_prop->bIsOffload)
        return AUDCLNT_E_ENDPOINT_OFFLOAD_NOT_CAPABLE;

    return S_OK;
}

static HRESULT WINAPI client_GetBufferSizeLimits(IAudioClient3 *iface, const WAVEFORMATEX *format,
                                          BOOL event_driven, REFERENCE_TIME *min_duration,
                                          REFERENCE_TIME *max_duration)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    FIXME("(%p)->(%p, %u, %p, %p) - stub\n", This, format, event_driven, min_duration, max_duration);
    return E_NOTIMPL;
}

static HRESULT WINAPI client_GetSharedModeEnginePeriod(IAudioClient3 *iface,
                                                const WAVEFORMATEX *format,
                                                UINT32 *default_period_frames,
                                                UINT32 *unit_period_frames,
                                                UINT32 *min_period_frames,
                                                UINT32 *max_period_frames)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    REFERENCE_TIME def_period, min_period;
    HRESULT hr;

    TRACE("(%p)->(%p, %p, %p, %p, %p)\n",
          This, format, default_period_frames,
          unit_period_frames, min_period_frames,
          max_period_frames);

    if (FAILED(hr = get_periods(This, &def_period, &min_period)))
        return hr;

    *default_period_frames = def_period * format->nSamplesPerSec / (REFERENCE_TIME)10000000;
    *min_period_frames     = min_period * format->nSamplesPerSec / (REFERENCE_TIME)10000000;
    *max_period_frames     = *default_period_frames;
    *unit_period_frames    = 1;

    return hr;
}

static HRESULT WINAPI client_GetCurrentSharedModeEnginePeriod(IAudioClient3 *iface,
                                                       WAVEFORMATEX **cur_format,
                                                       UINT32 *cur_period_frames)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    UINT32 dummy;
    HRESULT hr;

    TRACE("(%p)->(%p, %p)\n", This, cur_format, cur_period_frames);

    if (!cur_format || !cur_period_frames)
        return E_POINTER;

    if (FAILED(hr = client_GetMixFormat(iface, cur_format)))
        return hr;

    return client_GetSharedModeEnginePeriod(iface, *cur_format, cur_period_frames, &dummy, &dummy, &dummy);
}

static HRESULT WINAPI client_InitializeSharedAudioStream(IAudioClient3 *iface, DWORD flags,
                                                  UINT32 period_frames,
                                                  const WAVEFORMATEX *format,
                                                  const GUID *session_guid)
{
    struct audio_client *This = impl_from_IAudioClient3(iface);
    REFERENCE_TIME period;

    TRACE("(%p)->(0x%lx, %u, %p, %s)\n", This, flags, period_frames, format, debugstr_guid(session_guid));

    if (!format)
        return E_POINTER;

    period = period_frames * (REFERENCE_TIME)10000000 / format->nSamplesPerSec;

    return stream_init(This, FALSE, AUDCLNT_SHAREMODE_SHARED, flags, 0, period, format, session_guid);
}

const IAudioClient3Vtbl AudioClient3_Vtbl =
{
    client_QueryInterface,
    client_AddRef,
    client_Release,
    client_Initialize,
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

static HRESULT WINAPI clock_QueryInterface(IAudioClock *iface, REFIID riid, void **ppv)
{
    struct audio_client *This = impl_from_IAudioClock(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAudioClock))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IAudioClock2))
        *ppv = &This->IAudioClock2_iface;
    else if (IsEqualIID(riid, &IID_IMarshal)) {
        return IUnknown_QueryInterface(This->marshal, riid, ppv);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);

    return S_OK;
}

static ULONG WINAPI clock_AddRef(IAudioClock *iface)
{
    struct audio_client *This = impl_from_IAudioClock(iface);
    return IAudioClient3_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI clock_Release(IAudioClock *iface)
{
    struct audio_client *This = impl_from_IAudioClock(iface);
    return IAudioClient3_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI clock_GetFrequency(IAudioClock *iface, UINT64 *freq)
{
    struct audio_client *This = impl_from_IAudioClock(iface);
    struct get_frequency_params params;

    TRACE("(%p)->(%p)\n", This, freq);

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    params.freq   = freq;

    wine_unix_call(get_frequency, &params);

    return params.result;
}

static HRESULT WINAPI clock_GetPosition(IAudioClock *iface, UINT64 *pos, UINT64 *qpctime)
{
    struct audio_client *This = impl_from_IAudioClock(iface);
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

    wine_unix_call(get_position, &params);

    return params.result;
}

static HRESULT WINAPI clock_GetCharacteristics(IAudioClock *iface, DWORD *chars)
{
    struct audio_client *This = impl_from_IAudioClock(iface);

    TRACE("(%p)->(%p)\n", This, chars);

    if (!chars)
        return E_POINTER;

    *chars = AUDIOCLOCK_CHARACTERISTIC_FIXED_FREQ;

    return S_OK;
}

const IAudioClockVtbl AudioClock_Vtbl =
{
    clock_QueryInterface,
    clock_AddRef,
    clock_Release,
    clock_GetFrequency,
    clock_GetPosition,
    clock_GetCharacteristics
};

static HRESULT WINAPI clock2_QueryInterface(IAudioClock2 *iface, REFIID riid, void **ppv)
{
    struct audio_client *This = impl_from_IAudioClock2(iface);
    return IAudioClock_QueryInterface(&This->IAudioClock_iface, riid, ppv);
}

static ULONG WINAPI clock2_AddRef(IAudioClock2 *iface)
{
    struct audio_client *This = impl_from_IAudioClock2(iface);
    return IAudioClient3_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI clock2_Release(IAudioClock2 *iface)
{
    struct audio_client *This = impl_from_IAudioClock2(iface);
    return IAudioClient3_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI clock2_GetDevicePosition(IAudioClock2 *iface, UINT64 *pos, UINT64 *qpctime)
{
    struct audio_client *This = impl_from_IAudioClock2(iface);
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

    wine_unix_call(get_position, &params);

    return params.result;
}

const IAudioClock2Vtbl AudioClock2_Vtbl =
{
    clock2_QueryInterface,
    clock2_AddRef,
    clock2_Release,
    clock2_GetDevicePosition
};

static HRESULT WINAPI AudioClockAdjustment_QueryInterface(IAudioClockAdjustment *iface,
        REFIID riid, void **ppv)
{
    struct audio_client *This = impl_from_IAudioClockAdjustment(iface);
    return IAudioClient3_QueryInterface(&This->IAudioClient3_iface, riid, ppv);
}

static ULONG WINAPI AudioClockAdjustment_AddRef(IAudioClockAdjustment *iface)
{
    struct audio_client *This = impl_from_IAudioClockAdjustment(iface);
    return IAudioClient3_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI AudioClockAdjustment_Release(IAudioClockAdjustment *iface)
{
    struct audio_client *This = impl_from_IAudioClockAdjustment(iface);
    return IAudioClient3_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI AudioClockAdjustment_SetSampleRate(IAudioClockAdjustment *iface, float rate)
{
    struct audio_client *This = impl_from_IAudioClockAdjustment(iface);
    struct set_sample_rate_params params;

    TRACE("(%p)->(%f)\n", This, rate);

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream = This->stream;
    params.rate   = rate;
    params.result = E_NOTIMPL;

    wine_unix_call(set_sample_rate, &params);

    return params.result;
}

const IAudioClockAdjustmentVtbl AudioClockAdjustment_Vtbl =
{
    AudioClockAdjustment_QueryInterface,
    AudioClockAdjustment_AddRef,
    AudioClockAdjustment_Release,
    AudioClockAdjustment_SetSampleRate
};

static HRESULT WINAPI render_QueryInterface(IAudioRenderClient *iface, REFIID riid, void **ppv)
{
    struct audio_client *This = impl_from_IAudioRenderClient(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAudioRenderClient))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IMarshal)) {
        return IUnknown_QueryInterface(This->marshal, riid, ppv);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);

    return S_OK;
}

static ULONG WINAPI render_AddRef(IAudioRenderClient *iface)
{
    struct audio_client *This = impl_from_IAudioRenderClient(iface);
    return IAudioClient3_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI render_Release(IAudioRenderClient *iface)
{
    struct audio_client *This = impl_from_IAudioRenderClient(iface);
    return IAudioClient3_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI render_GetBuffer(IAudioRenderClient *iface, UINT32 frames, BYTE **data)
{
    struct audio_client *This = impl_from_IAudioRenderClient(iface);
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

    wine_unix_call(get_render_buffer, &params);

    return params.result;
}

static HRESULT WINAPI render_ReleaseBuffer(IAudioRenderClient *iface, UINT32 written_frames,
                                           DWORD flags)
{
    struct audio_client *This = impl_from_IAudioRenderClient(iface);
    struct release_render_buffer_params params;

    TRACE("(%p)->(%u, %lx)\n", This, written_frames, flags);

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    params.stream         = This->stream;
    params.written_frames = written_frames;
    params.flags          = flags;

    wine_unix_call(release_render_buffer, &params);

    return params.result;
}

const IAudioRenderClientVtbl AudioRenderClient_Vtbl = {
    render_QueryInterface,
    render_AddRef,
    render_Release,
    render_GetBuffer,
    render_ReleaseBuffer
};

static HRESULT WINAPI streamvolume_QueryInterface(IAudioStreamVolume *iface, REFIID riid,
                                                  void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAudioStreamVolume))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IMarshal)) {
        struct audio_client *This = impl_from_IAudioStreamVolume(iface);
        return IUnknown_QueryInterface(This->marshal, riid, ppv);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);

    return S_OK;
}

static ULONG WINAPI streamvolume_AddRef(IAudioStreamVolume *iface)
{
    struct audio_client *This = impl_from_IAudioStreamVolume(iface);
    return IAudioClient3_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI streamvolume_Release(IAudioStreamVolume *iface)
{
    struct audio_client *This = impl_from_IAudioStreamVolume(iface);
    return IAudioClient3_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI streamvolume_GetChannelCount(IAudioStreamVolume *iface, UINT32 *out)
{
    struct audio_client *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%p)\n", This, out);

    if (!out)
        return E_POINTER;

    *out = This->channel_count;

    return S_OK;
}

static HRESULT WINAPI streamvolume_SetChannelVolume(IAudioStreamVolume *iface, UINT32 index,
                                                    float level)
{
    struct audio_client *This = impl_from_IAudioStreamVolume(iface);

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

static HRESULT WINAPI streamvolume_GetChannelVolume(IAudioStreamVolume *iface, UINT32 index,
                                                    float *level)
{
    struct audio_client *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%d, %p)\n", This, index, level);

    if (!level)
        return E_POINTER;

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    if (index >= This->channel_count)
        return E_INVALIDARG;

    *level = This->vols[index];

    return S_OK;
}

static HRESULT WINAPI streamvolume_SetAllVolumes(IAudioStreamVolume *iface, UINT32 count,
                                                 const float *levels)
{
    struct audio_client *This = impl_from_IAudioStreamVolume(iface);
    unsigned int i;

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

static HRESULT WINAPI streamvolume_GetAllVolumes(IAudioStreamVolume *iface, UINT32 count,
                                                 float *levels)
{
    struct audio_client *This = impl_from_IAudioStreamVolume(iface);
    unsigned int i;

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

const IAudioStreamVolumeVtbl AudioStreamVolume_Vtbl =
{
    streamvolume_QueryInterface,
    streamvolume_AddRef,
    streamvolume_Release,
    streamvolume_GetChannelCount,
    streamvolume_SetChannelVolume,
    streamvolume_GetChannelVolume,
    streamvolume_SetAllVolumes,
    streamvolume_GetAllVolumes
};

HRESULT AudioClient_Create(GUID *guid, IMMDevice *device, IAudioClient **out)
{
    struct audio_client *This;
    char *name;
    EDataFlow dataflow;
    HRESULT hr;

    TRACE("%s %p %p\n", debugstr_guid(guid), device, out);

    *out = NULL;

    if (!get_device_name_from_guid(guid, &name, &dataflow))
        return AUDCLNT_E_DEVICE_INVALIDATED;

    if (dataflow != eRender && dataflow != eCapture) {
        free(name);
        return E_UNEXPECTED;
    }

    This = calloc(1, sizeof(*This));
    if (!This) {
        free(name);
        return E_OUTOFMEMORY;
    }

    This->device_name = name;

    This->IAudioCaptureClient_iface.lpVtbl   = &AudioCaptureClient_Vtbl;
    This->IAudioClient3_iface.lpVtbl         = &AudioClient3_Vtbl;
    This->IAudioClock_iface.lpVtbl           = &AudioClock_Vtbl;
    This->IAudioClock2_iface.lpVtbl          = &AudioClock2_Vtbl;
    This->IAudioClockAdjustment_iface.lpVtbl = &AudioClockAdjustment_Vtbl;
    This->IAudioRenderClient_iface.lpVtbl    = &AudioRenderClient_Vtbl;
    This->IAudioStreamVolume_iface.lpVtbl    = &AudioStreamVolume_Vtbl;

    This->dataflow = dataflow;
    This->parent   = device;

    hr = CoCreateFreeThreadedMarshaler((IUnknown *)&This->IAudioClient3_iface, &This->marshal);
    if (FAILED(hr)) {
        free(This->device_name);
        free(This);
        return hr;
    }

    IMMDevice_AddRef(This->parent);

    *out = (IAudioClient *)&This->IAudioClient3_iface;
    IAudioClient3_AddRef(&This->IAudioClient3_iface);

    return S_OK;
}
