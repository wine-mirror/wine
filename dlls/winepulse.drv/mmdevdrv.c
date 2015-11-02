/*
 * Copyright 2011-2012 Maarten Lankhorst
 * Copyright 2010-2011 Maarten Lankhorst for CodeWeavers
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
#define _GNU_SOURCE

#include "config.h"
#include <poll.h>
#include <pthread.h>

#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>

#include <pulse/pulseaudio.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"

#include "ole2.h"
#include "dshow.h"
#include "dsound.h"
#include "propsys.h"

#include "initguid.h"
#include "ks.h"
#include "ksmedia.h"
#include "mmdeviceapi.h"
#include "audioclient.h"
#include "endpointvolume.h"
#include "audiopolicy.h"

WINE_DEFAULT_DEBUG_CHANNEL(pulse);

#define NULL_PTR_ERR MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER)

/* From <dlls/mmdevapi/mmdevapi.h> */
enum DriverPriority {
    Priority_Unavailable = 0,
    Priority_Low,
    Priority_Neutral,
    Priority_Preferred
};

static const REFERENCE_TIME MinimumPeriod = 30000;
static const REFERENCE_TIME DefaultPeriod = 100000;

static pa_context *pulse_ctx;
static pa_mainloop *pulse_ml;

static HANDLE pulse_thread;
static pthread_mutex_t pulse_lock;
static pthread_cond_t pulse_cond = PTHREAD_COND_INITIALIZER;

/* Mixer format + period times */
static WAVEFORMATEXTENSIBLE pulse_fmt[2];
static REFERENCE_TIME pulse_min_period[2], pulse_def_period[2];

static GUID pulse_render_guid =
{ 0xfd47d9cc, 0x4218, 0x4135, { 0x9c, 0xe2, 0x0c, 0x19, 0x5c, 0x87, 0x40, 0x5b } };
static GUID pulse_capture_guid =
{ 0x25da76d0, 0x033c, 0x4235, { 0x90, 0x02, 0x19, 0xf4, 0x88, 0x94, 0xac, 0x6f } };

BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, void *reserved)
{
    if (reason == DLL_PROCESS_ATTACH) {
        pthread_mutexattr_t attr;

        DisableThreadLibraryCalls(dll);

        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);

        if (pthread_mutex_init(&pulse_lock, &attr) != 0)
            pthread_mutex_init(&pulse_lock, NULL);
    } else if (reason == DLL_PROCESS_DETACH) {
        if (pulse_thread)
           SetThreadPriority(pulse_thread, 0);
        if (pulse_ctx) {
            pa_context_disconnect(pulse_ctx);
            pa_context_unref(pulse_ctx);
        }
        if (pulse_ml)
            pa_mainloop_quit(pulse_ml, 0);
        if (pulse_thread) {
            WaitForSingleObject(pulse_thread, INFINITE);
            CloseHandle(pulse_thread);
        }
    }
    return TRUE;
}

typedef struct ACImpl ACImpl;

typedef struct _ACPacket {
    struct list entry;
    UINT64 qpcpos;
    BYTE *data;
    UINT32 discont;
} ACPacket;

struct ACImpl {
    IAudioClient IAudioClient_iface;
    IAudioRenderClient IAudioRenderClient_iface;
    IAudioCaptureClient IAudioCaptureClient_iface;
    IAudioClock IAudioClock_iface;
    IAudioClock2 IAudioClock2_iface;
    IAudioStreamVolume IAudioStreamVolume_iface;
    IMMDevice *parent;
    struct list entry;
    float vol[PA_CHANNELS_MAX];

    LONG ref;
    EDataFlow dataflow;
    DWORD flags;
    AUDCLNT_SHAREMODE share;
    HANDLE event;

    UINT32 bufsize_frames, bufsize_bytes, locked, capture_period, pad, started, peek_ofs;
    void *locked_ptr, *tmp_buffer;

    pa_stream *stream;
    pa_sample_spec ss;
    pa_channel_map map;

    INT64 clock_lastpos, clock_written;

    struct list packet_free_head;
    struct list packet_filled_head;
};

static const WCHAR defaultW[] = {'P','u','l','s','e','a','u','d','i','o',0};

static const IAudioClientVtbl AudioClient_Vtbl;
static const IAudioRenderClientVtbl AudioRenderClient_Vtbl;
static const IAudioCaptureClientVtbl AudioCaptureClient_Vtbl;
static const IAudioClockVtbl AudioClock_Vtbl;
static const IAudioClock2Vtbl AudioClock2_Vtbl;
static const IAudioStreamVolumeVtbl AudioStreamVolume_Vtbl;

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

/* Following pulseaudio design here, mainloop has the lock taken whenever
 * it is handling something for pulse, and the lock is required whenever
 * doing any pa_* call that can affect the state in any way
 *
 * pa_cond_wait is used when waiting on results, because the mainloop needs
 * the same lock taken to affect the state
 *
 * This is basically the same as the pa_threaded_mainloop implementation,
 * but that cannot be used because it uses pthread_create directly
 *
 * pa_threaded_mainloop_(un)lock -> pthread_mutex_(un)lock
 * pa_threaded_mainloop_signal -> pthread_cond_signal
 * pa_threaded_mainloop_wait -> pthread_cond_wait
 */

static int pulse_poll_func(struct pollfd *ufds, unsigned long nfds, int timeout, void *userdata) {
    int r;
    pthread_mutex_unlock(&pulse_lock);
    r = poll(ufds, nfds, timeout);
    pthread_mutex_lock(&pulse_lock);
    return r;
}

static DWORD CALLBACK pulse_mainloop_thread(void *tmp) {
    int ret;
    pulse_ml = pa_mainloop_new();
    pa_mainloop_set_poll_func(pulse_ml, pulse_poll_func, NULL);
    pthread_mutex_lock(&pulse_lock);
    pthread_cond_signal(&pulse_cond);
    pa_mainloop_run(pulse_ml, &ret);
    pthread_mutex_unlock(&pulse_lock);
    pa_mainloop_free(pulse_ml);
    return ret;
}

static void pulse_contextcallback(pa_context *c, void *userdata)
{
    switch (pa_context_get_state(c)) {
        default:
            FIXME("Unhandled state: %i\n", pa_context_get_state(c));
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
        case PA_CONTEXT_TERMINATED:
            TRACE("State change to %i\n", pa_context_get_state(c));
            return;

        case PA_CONTEXT_READY:
            TRACE("Ready\n");
            break;

        case PA_CONTEXT_FAILED:
            ERR("Context failed: %s\n", pa_strerror(pa_context_errno(c)));
            break;
    }
    pthread_cond_signal(&pulse_cond);
}

static void pulse_stream_state(pa_stream *s, void *user)
{
    pa_stream_state_t state = pa_stream_get_state(s);
    TRACE("Stream state changed to %i\n", state);
    pthread_cond_signal(&pulse_cond);
}

static const enum pa_channel_position pulse_pos_from_wfx[] = {
    PA_CHANNEL_POSITION_FRONT_LEFT,
    PA_CHANNEL_POSITION_FRONT_RIGHT,
    PA_CHANNEL_POSITION_FRONT_CENTER,
    PA_CHANNEL_POSITION_LFE,
    PA_CHANNEL_POSITION_REAR_LEFT,
    PA_CHANNEL_POSITION_REAR_RIGHT,
    PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER,
    PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER,
    PA_CHANNEL_POSITION_REAR_CENTER,
    PA_CHANNEL_POSITION_SIDE_LEFT,
    PA_CHANNEL_POSITION_SIDE_RIGHT,
    PA_CHANNEL_POSITION_TOP_CENTER,
    PA_CHANNEL_POSITION_TOP_FRONT_LEFT,
    PA_CHANNEL_POSITION_TOP_FRONT_CENTER,
    PA_CHANNEL_POSITION_TOP_FRONT_RIGHT,
    PA_CHANNEL_POSITION_TOP_REAR_LEFT,
    PA_CHANNEL_POSITION_TOP_REAR_CENTER,
    PA_CHANNEL_POSITION_TOP_REAR_RIGHT
};

static void pulse_probe_settings(int render, WAVEFORMATEXTENSIBLE *fmt) {
    WAVEFORMATEX *wfx = &fmt->Format;
    pa_stream *stream;
    pa_channel_map map;
    pa_sample_spec ss;
    pa_buffer_attr attr;
    int ret, i;
    unsigned int length = 0;

    pa_channel_map_init_auto(&map, 2, PA_CHANNEL_MAP_ALSA);
    ss.rate = 48000;
    ss.format = PA_SAMPLE_FLOAT32LE;
    ss.channels = map.channels;

    attr.maxlength = -1;
    attr.tlength = -1;
    attr.minreq = attr.fragsize = pa_frame_size(&ss);
    attr.prebuf = 0;

    stream = pa_stream_new(pulse_ctx, "format test stream", &ss, &map);
    if (stream)
        pa_stream_set_state_callback(stream, pulse_stream_state, NULL);
    if (!stream)
        ret = -1;
    else if (render)
        ret = pa_stream_connect_playback(stream, NULL, &attr,
        PA_STREAM_START_CORKED|PA_STREAM_FIX_RATE|PA_STREAM_FIX_CHANNELS|PA_STREAM_EARLY_REQUESTS, NULL, NULL);
    else
        ret = pa_stream_connect_record(stream, NULL, &attr, PA_STREAM_START_CORKED|PA_STREAM_FIX_RATE|PA_STREAM_FIX_CHANNELS|PA_STREAM_EARLY_REQUESTS);
    if (ret >= 0) {
        while (pa_mainloop_iterate(pulse_ml, 1, &ret) >= 0 &&
                pa_stream_get_state(stream) == PA_STREAM_CREATING)
        {}
        if (pa_stream_get_state(stream) == PA_STREAM_READY) {
            ss = *pa_stream_get_sample_spec(stream);
            map = *pa_stream_get_channel_map(stream);
            if (render)
                length = pa_stream_get_buffer_attr(stream)->minreq;
            else
                length = pa_stream_get_buffer_attr(stream)->fragsize;
            pa_stream_disconnect(stream);
            while (pa_mainloop_iterate(pulse_ml, 1, &ret) >= 0 &&
                    pa_stream_get_state(stream) == PA_STREAM_READY)
            {}
        }
    }
    if (stream)
        pa_stream_unref(stream);
    if (length)
        pulse_def_period[!render] = pulse_min_period[!render] = pa_bytes_to_usec(10 * length, &ss);
    else
        pulse_min_period[!render] = MinimumPeriod;
    if (pulse_def_period[!render] <= DefaultPeriod)
        pulse_def_period[!render] = DefaultPeriod;

    wfx->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfx->cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    wfx->nChannels = ss.channels;
    wfx->wBitsPerSample = 8 * pa_sample_size_of_format(ss.format);
    wfx->nSamplesPerSec = ss.rate;
    wfx->nBlockAlign = pa_frame_size(&ss);
    wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;
    if (ss.format != PA_SAMPLE_S24_32LE)
        fmt->Samples.wValidBitsPerSample = wfx->wBitsPerSample;
    else
        fmt->Samples.wValidBitsPerSample = 24;
    if (ss.format == PA_SAMPLE_FLOAT32LE)
        fmt->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    else
        fmt->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    fmt->dwChannelMask = 0;
    for (i = 0; i < map.channels; ++i)
        switch (map.map[i]) {
            default: FIXME("Unhandled channel %s\n", pa_channel_position_to_string(map.map[i])); break;
            case PA_CHANNEL_POSITION_FRONT_LEFT: fmt->dwChannelMask |= SPEAKER_FRONT_LEFT; break;
            case PA_CHANNEL_POSITION_MONO:
            case PA_CHANNEL_POSITION_FRONT_CENTER: fmt->dwChannelMask |= SPEAKER_FRONT_CENTER; break;
            case PA_CHANNEL_POSITION_FRONT_RIGHT: fmt->dwChannelMask |= SPEAKER_FRONT_RIGHT; break;
            case PA_CHANNEL_POSITION_REAR_LEFT: fmt->dwChannelMask |= SPEAKER_BACK_LEFT; break;
            case PA_CHANNEL_POSITION_REAR_CENTER: fmt->dwChannelMask |= SPEAKER_BACK_CENTER; break;
            case PA_CHANNEL_POSITION_REAR_RIGHT: fmt->dwChannelMask |= SPEAKER_BACK_RIGHT; break;
            case PA_CHANNEL_POSITION_LFE: fmt->dwChannelMask |= SPEAKER_LOW_FREQUENCY; break;
            case PA_CHANNEL_POSITION_SIDE_LEFT: fmt->dwChannelMask |= SPEAKER_SIDE_LEFT; break;
            case PA_CHANNEL_POSITION_SIDE_RIGHT: fmt->dwChannelMask |= SPEAKER_SIDE_RIGHT; break;
            case PA_CHANNEL_POSITION_TOP_CENTER: fmt->dwChannelMask |= SPEAKER_TOP_CENTER; break;
            case PA_CHANNEL_POSITION_TOP_FRONT_LEFT: fmt->dwChannelMask |= SPEAKER_TOP_FRONT_LEFT; break;
            case PA_CHANNEL_POSITION_TOP_FRONT_CENTER: fmt->dwChannelMask |= SPEAKER_TOP_FRONT_CENTER; break;
            case PA_CHANNEL_POSITION_TOP_FRONT_RIGHT: fmt->dwChannelMask |= SPEAKER_TOP_FRONT_RIGHT; break;
            case PA_CHANNEL_POSITION_TOP_REAR_LEFT: fmt->dwChannelMask |= SPEAKER_TOP_BACK_LEFT; break;
            case PA_CHANNEL_POSITION_TOP_REAR_CENTER: fmt->dwChannelMask |= SPEAKER_TOP_BACK_CENTER; break;
            case PA_CHANNEL_POSITION_TOP_REAR_RIGHT: fmt->dwChannelMask |= SPEAKER_TOP_BACK_RIGHT; break;
        }
}

static HRESULT pulse_connect(void)
{
    int len;
    WCHAR path[PATH_MAX], *name;
    char *str;

    if (!pulse_thread)
    {
        if (!(pulse_thread = CreateThread(NULL, 0, pulse_mainloop_thread, NULL, 0, NULL)))
        {
            ERR("Failed to create mainloop thread.");
            return E_FAIL;
        }
        SetThreadPriority(pulse_thread, THREAD_PRIORITY_TIME_CRITICAL);
        pthread_cond_wait(&pulse_cond, &pulse_lock);
    }

    if (pulse_ctx && PA_CONTEXT_IS_GOOD(pa_context_get_state(pulse_ctx)))
        return S_OK;
    if (pulse_ctx)
        pa_context_unref(pulse_ctx);

    GetModuleFileNameW(NULL, path, sizeof(path)/sizeof(*path));
    name = strrchrW(path, '\\');
    if (!name)
        name = path;
    else
        name++;
    len = WideCharToMultiByte(CP_UNIXCP, 0, name, -1, NULL, 0, NULL, NULL);
    str = pa_xmalloc(len);
    WideCharToMultiByte(CP_UNIXCP, 0, name, -1, str, len, NULL, NULL);
    TRACE("Name: %s\n", str);
    pulse_ctx = pa_context_new(pa_mainloop_get_api(pulse_ml), str);
    pa_xfree(str);
    if (!pulse_ctx) {
        ERR("Failed to create context\n");
        return E_FAIL;
    }

    pa_context_set_state_callback(pulse_ctx, pulse_contextcallback, NULL);

    TRACE("libpulse protocol version: %u. API Version %u\n", pa_context_get_protocol_version(pulse_ctx), PA_API_VERSION);
    if (pa_context_connect(pulse_ctx, NULL, 0, NULL) < 0)
        goto fail;

    /* Wait for connection */
    while (pthread_cond_wait(&pulse_cond, &pulse_lock)) {
        pa_context_state_t state = pa_context_get_state(pulse_ctx);

        if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED)
            goto fail;

        if (state == PA_CONTEXT_READY)
            break;
    }

    TRACE("Connected to server %s with protocol version: %i.\n",
        pa_context_get_server(pulse_ctx),
        pa_context_get_server_protocol_version(pulse_ctx));
    return S_OK;

fail:
    pa_context_unref(pulse_ctx);
    pulse_ctx = NULL;
    return E_FAIL;
}

/* some poorly-behaved applications call audio functions during DllMain, so we
 * have to do as much as possible without creating a new thread. this function
 * sets up a synchronous connection to verify the server is running and query
 * static data. */
static HRESULT pulse_test_connect(void)
{
    int len, ret;
    WCHAR path[PATH_MAX], *name;
    char *str;

    pulse_ml = pa_mainloop_new();

    pa_mainloop_set_poll_func(pulse_ml, pulse_poll_func, NULL);

    GetModuleFileNameW(NULL, path, sizeof(path)/sizeof(*path));
    name = strrchrW(path, '\\');
    if (!name)
        name = path;
    else
        name++;
    len = WideCharToMultiByte(CP_UNIXCP, 0, name, -1, NULL, 0, NULL, NULL);
    str = pa_xmalloc(len);
    WideCharToMultiByte(CP_UNIXCP, 0, name, -1, str, len, NULL, NULL);
    TRACE("Name: %s\n", str);
    pulse_ctx = pa_context_new(pa_mainloop_get_api(pulse_ml), str);
    pa_xfree(str);
    if (!pulse_ctx) {
        ERR("Failed to create context\n");
        pa_mainloop_free(pulse_ml);
        pulse_ml = NULL;
        return E_FAIL;
    }

    pa_context_set_state_callback(pulse_ctx, pulse_contextcallback, NULL);

    TRACE("libpulse protocol version: %u. API Version %u\n", pa_context_get_protocol_version(pulse_ctx), PA_API_VERSION);
    if (pa_context_connect(pulse_ctx, NULL, 0, NULL) < 0)
        goto fail;

    /* Wait for connection */
    while (pa_mainloop_iterate(pulse_ml, 1, &ret) >= 0) {
        pa_context_state_t state = pa_context_get_state(pulse_ctx);

        if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED)
            goto fail;

        if (state == PA_CONTEXT_READY)
            break;
    }

    TRACE("Test-connected to server %s with protocol version: %i.\n",
        pa_context_get_server(pulse_ctx),
        pa_context_get_server_protocol_version(pulse_ctx));

    pulse_probe_settings(1, &pulse_fmt[0]);
    pulse_probe_settings(0, &pulse_fmt[1]);

    pa_context_unref(pulse_ctx);
    pulse_ctx = NULL;
    pa_mainloop_free(pulse_ml);
    pulse_ml = NULL;

    return S_OK;

fail:
    pa_context_unref(pulse_ctx);
    pulse_ctx = NULL;
    pa_mainloop_free(pulse_ml);
    pulse_ml = NULL;

    return E_FAIL;
}

static HRESULT pulse_stream_valid(ACImpl *This) {
    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (!This->stream || pa_stream_get_state(This->stream) != PA_STREAM_READY)
        return AUDCLNT_E_DEVICE_INVALIDATED;
    return S_OK;
}

static void dump_attr(const pa_buffer_attr *attr) {
    TRACE("maxlength: %u\n", attr->maxlength);
    TRACE("minreq: %u\n", attr->minreq);
    TRACE("fragsize: %u\n", attr->fragsize);
    TRACE("tlength: %u\n", attr->tlength);
    TRACE("prebuf: %u\n", attr->prebuf);
}

static void pulse_op_cb(pa_stream *s, int success, void *user) {
    TRACE("Success: %i\n", success);
    *(int*)user = success;
    pthread_cond_signal(&pulse_cond);
}

static void pulse_attr_update(pa_stream *s, void *user) {
    const pa_buffer_attr *attr = pa_stream_get_buffer_attr(s);
    TRACE("New attributes or device moved:\n");
    dump_attr(attr);
}

static void pulse_wr_callback(pa_stream *s, size_t bytes, void *userdata)
{
    ACImpl *This = userdata;
    UINT32 oldpad = This->pad;

    if (bytes < This->bufsize_bytes)
        This->pad = This->bufsize_bytes - bytes;
    else
        This->pad = 0;

    if (oldpad == This->pad)
        return;

    assert(oldpad > This->pad);

    This->clock_written += oldpad - This->pad;
    TRACE("New pad: %zu (-%zu)\n", This->pad / pa_frame_size(&This->ss), (oldpad - This->pad) / pa_frame_size(&This->ss));

    if (This->event)
        SetEvent(This->event);
}

static void pulse_underflow_callback(pa_stream *s, void *userdata)
{
    WARN("Underflow\n");
}

/* Latency is periodically updated even when nothing is played,
 * because of PA_STREAM_AUTO_TIMING_UPDATE so use it as timer
 *
 * Perfect for passing all tests :)
 */
static void pulse_latency_callback(pa_stream *s, void *userdata)
{
    ACImpl *This = userdata;
    if (!This->pad && This->event)
        SetEvent(This->event);
}

static void pulse_started_callback(pa_stream *s, void *userdata)
{
    TRACE("(Re)started playing\n");
}

static void pulse_rd_loop(ACImpl *This, size_t bytes)
{
    while (bytes >= This->capture_period) {
        ACPacket *p, *next;
        LARGE_INTEGER stamp, freq;
        BYTE *dst, *src;
        size_t src_len, copy, rem = This->capture_period;
        if (!(p = (ACPacket*)list_head(&This->packet_free_head))) {
            p = (ACPacket*)list_head(&This->packet_filled_head);
            if (!p->discont) {
                next = (ACPacket*)p->entry.next;
                next->discont = 1;
            } else
                p = (ACPacket*)list_tail(&This->packet_filled_head);
            assert(This->pad == This->bufsize_bytes);
        } else {
            assert(This->pad < This->bufsize_bytes);
            This->pad += This->capture_period;
            assert(This->pad <= This->bufsize_bytes);
        }
        QueryPerformanceCounter(&stamp);
        QueryPerformanceFrequency(&freq);
        p->qpcpos = (stamp.QuadPart * (INT64)10000000) / freq.QuadPart;
        p->discont = 0;
        list_remove(&p->entry);
        list_add_tail(&This->packet_filled_head, &p->entry);

        dst = p->data;
        while (rem) {
            pa_stream_peek(This->stream, (const void**)&src, &src_len);
            assert(src_len);
            assert(This->peek_ofs < src_len);
            src += This->peek_ofs;
            src_len -= This->peek_ofs;
            assert(src_len <= bytes);

            copy = rem;
            if (copy > src_len)
                copy = src_len;
            memcpy(dst, src, rem);
            src += copy;
            src_len -= copy;
            dst += copy;
            rem -= copy;

            if (!src_len) {
                This->peek_ofs = 0;
                pa_stream_drop(This->stream);
            } else
                This->peek_ofs += copy;
        }
        bytes -= This->capture_period;
    }
}

static void pulse_rd_drop(ACImpl *This, size_t bytes)
{
    while (bytes >= This->capture_period) {
        size_t src_len, copy, rem = This->capture_period;
        while (rem) {
            const void *src;
            pa_stream_peek(This->stream, &src, &src_len);
            assert(src_len);
            assert(This->peek_ofs < src_len);
            src_len -= This->peek_ofs;
            assert(src_len <= bytes);

            copy = rem;
            if (copy > src_len)
                copy = src_len;

            src_len -= copy;
            rem -= copy;

            if (!src_len) {
                This->peek_ofs = 0;
                pa_stream_drop(This->stream);
            } else
                This->peek_ofs += copy;
            bytes -= copy;
        }
    }
}

static void pulse_rd_callback(pa_stream *s, size_t bytes, void *userdata)
{
    ACImpl *This = userdata;

    TRACE("Readable total: %zu, fragsize: %u\n", bytes, pa_stream_get_buffer_attr(s)->fragsize);
    assert(bytes >= This->peek_ofs);
    bytes -= This->peek_ofs;
    if (bytes < This->capture_period)
        return;

    if (This->started)
        pulse_rd_loop(This, bytes);
    else
        pulse_rd_drop(This, bytes);

    if (This->event)
        SetEvent(This->event);
}

static HRESULT pulse_stream_connect(ACImpl *This, UINT32 period_bytes) {
    int ret;
    char buffer[64];
    static LONG number;
    pa_buffer_attr attr;
    if (This->stream) {
        pa_stream_disconnect(This->stream);
        while (pa_stream_get_state(This->stream) == PA_STREAM_READY)
            pthread_cond_wait(&pulse_cond, &pulse_lock);
        pa_stream_unref(This->stream);
    }
    ret = InterlockedIncrement(&number);
    sprintf(buffer, "audio stream #%i", ret);
    This->stream = pa_stream_new(pulse_ctx, buffer, &This->ss, &This->map);
    pa_stream_set_state_callback(This->stream, pulse_stream_state, This);
    pa_stream_set_buffer_attr_callback(This->stream, pulse_attr_update, This);
    pa_stream_set_moved_callback(This->stream, pulse_attr_update, This);

    /* Pulseaudio will fill in correct values */
    attr.minreq = attr.fragsize = period_bytes;
    attr.maxlength = attr.tlength = This->bufsize_bytes;
    attr.prebuf = pa_frame_size(&This->ss);
    dump_attr(&attr);
    if (This->dataflow == eRender)
        ret = pa_stream_connect_playback(This->stream, NULL, &attr,
        PA_STREAM_START_CORKED|PA_STREAM_START_UNMUTED|PA_STREAM_AUTO_TIMING_UPDATE|PA_STREAM_INTERPOLATE_TIMING|PA_STREAM_EARLY_REQUESTS, NULL, NULL);
    else
        ret = pa_stream_connect_record(This->stream, NULL, &attr,
        PA_STREAM_START_CORKED|PA_STREAM_START_UNMUTED|PA_STREAM_AUTO_TIMING_UPDATE|PA_STREAM_INTERPOLATE_TIMING|PA_STREAM_EARLY_REQUESTS);
    if (ret < 0) {
        WARN("Returns %i\n", ret);
        return AUDCLNT_E_ENDPOINT_CREATE_FAILED;
    }
    while (pa_stream_get_state(This->stream) == PA_STREAM_CREATING)
        pthread_cond_wait(&pulse_cond, &pulse_lock);
    if (pa_stream_get_state(This->stream) != PA_STREAM_READY)
        return AUDCLNT_E_ENDPOINT_CREATE_FAILED;

    if (This->dataflow == eRender) {
        pa_stream_set_write_callback(This->stream, pulse_wr_callback, This);
        pa_stream_set_underflow_callback(This->stream, pulse_underflow_callback, This);
        pa_stream_set_started_callback(This->stream, pulse_started_callback, This);
    } else
        pa_stream_set_read_callback(This->stream, pulse_rd_callback, This);
    return S_OK;
}

HRESULT WINAPI AUDDRV_GetEndpointIDs(EDataFlow flow, const WCHAR ***ids, GUID **keys,
        UINT *num, UINT *def_index)
{
    WCHAR *id;

    TRACE("%d %p %p %p\n", flow, ids, num, def_index);

    *num = 1;
    *def_index = 0;

    *ids = HeapAlloc(GetProcessHeap(), 0, sizeof(**ids));
    *keys = NULL;
    if (!*ids)
        return E_OUTOFMEMORY;

    (*ids)[0] = id = HeapAlloc(GetProcessHeap(), 0, sizeof(defaultW));
    *keys = HeapAlloc(GetProcessHeap(), 0, sizeof(**keys));
    if (!*keys || !id) {
        HeapFree(GetProcessHeap(), 0, id);
        HeapFree(GetProcessHeap(), 0, *keys);
        HeapFree(GetProcessHeap(), 0, *ids);
        *ids = NULL;
        *keys = NULL;
        return E_OUTOFMEMORY;
    }
    memcpy(id, defaultW, sizeof(defaultW));

    if (flow == eRender)
        (*keys)[0] = pulse_render_guid;
    else
        (*keys)[0] = pulse_capture_guid;

    return S_OK;
}

int WINAPI AUDDRV_GetPriority(void)
{
    HRESULT hr;
    pthread_mutex_lock(&pulse_lock);
    hr = pulse_test_connect();
    pthread_mutex_unlock(&pulse_lock);
    return SUCCEEDED(hr) ? Priority_Low : Priority_Unavailable;
}

HRESULT WINAPI AUDDRV_GetAudioEndpoint(GUID *guid, IMMDevice *dev, IAudioClient **out)
{
    ACImpl *This;
    int i;
    EDataFlow dataflow;

    TRACE("%s %p %p\n", debugstr_guid(guid), dev, out);
    if (IsEqualGUID(guid, &pulse_render_guid))
        dataflow = eRender;
    else if (IsEqualGUID(guid, &pulse_capture_guid))
        dataflow = eCapture;
    else
        return E_UNEXPECTED;

    *out = NULL;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->IAudioClient_iface.lpVtbl = &AudioClient_Vtbl;
    This->IAudioRenderClient_iface.lpVtbl = &AudioRenderClient_Vtbl;
    This->IAudioCaptureClient_iface.lpVtbl = &AudioCaptureClient_Vtbl;
    This->IAudioClock_iface.lpVtbl = &AudioClock_Vtbl;
    This->IAudioClock2_iface.lpVtbl = &AudioClock2_Vtbl;
    This->IAudioStreamVolume_iface.lpVtbl = &AudioStreamVolume_Vtbl;
    This->dataflow = dataflow;
    This->parent = dev;
    for (i = 0; i < PA_CHANNELS_MAX; ++i)
        This->vol[i] = 1.f;
    IMMDevice_AddRef(This->parent);

    *out = &This->IAudioClient_iface;
    IAudioClient_AddRef(&This->IAudioClient_iface);

    return S_OK;
}

static HRESULT WINAPI AudioClient_QueryInterface(IAudioClient *iface,
        REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IAudioClient))
        *ppv = iface;
    if (*ppv) {
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
    if (!ref) {
        if (This->stream) {
            pthread_mutex_lock(&pulse_lock);
            if (PA_STREAM_IS_GOOD(pa_stream_get_state(This->stream))) {
                pa_stream_disconnect(This->stream);
                while (PA_STREAM_IS_GOOD(pa_stream_get_state(This->stream)))
                    pthread_cond_wait(&pulse_cond, &pulse_lock);
            }
            pa_stream_unref(This->stream);
            This->stream = NULL;
            list_remove(&This->entry);
            pthread_mutex_unlock(&pulse_lock);
        }
        IMMDevice_Release(This->parent);
        HeapFree(GetProcessHeap(), 0, This->tmp_buffer);
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
    TRACE("nSamplesPerSec: %u\n", fmt->nSamplesPerSec);
    TRACE("nAvgBytesPerSec: %u\n", fmt->nAvgBytesPerSec);
    TRACE("nBlockAlign: %u\n", fmt->nBlockAlign);
    TRACE("wBitsPerSample: %u\n", fmt->wBitsPerSample);
    TRACE("cbSize: %u\n", fmt->cbSize);

    if (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        WAVEFORMATEXTENSIBLE *fmtex = (void*)fmt;
        TRACE("dwChannelMask: %08x\n", fmtex->dwChannelMask);
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

static DWORD get_channel_mask(unsigned int channels)
{
    switch(channels) {
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

static HRESULT pulse_spec_from_waveformat(ACImpl *This, const WAVEFORMATEX *fmt)
{
    pa_channel_map_init(&This->map);
    This->ss.rate = fmt->nSamplesPerSec;
    This->ss.format = PA_SAMPLE_INVALID;

    switch(fmt->wFormatTag) {
    case WAVE_FORMAT_IEEE_FLOAT:
        if (!fmt->nChannels || fmt->nChannels > 2 || fmt->wBitsPerSample != 32)
            break;
        This->ss.format = PA_SAMPLE_FLOAT32LE;
        pa_channel_map_init_auto(&This->map, fmt->nChannels, PA_CHANNEL_MAP_ALSA);
        break;
    case WAVE_FORMAT_PCM:
        if (!fmt->nChannels || fmt->nChannels > 2)
            break;
        if (fmt->wBitsPerSample == 8)
            This->ss.format = PA_SAMPLE_U8;
        else if (fmt->wBitsPerSample == 16)
            This->ss.format = PA_SAMPLE_S16LE;
        else
            return AUDCLNT_E_UNSUPPORTED_FORMAT;
        pa_channel_map_init_auto(&This->map, fmt->nChannels, PA_CHANNEL_MAP_ALSA);
        break;
    case WAVE_FORMAT_EXTENSIBLE: {
        WAVEFORMATEXTENSIBLE *wfe = (WAVEFORMATEXTENSIBLE*)fmt;
        DWORD mask = wfe->dwChannelMask;
        DWORD i = 0, j;
        if (fmt->cbSize != (sizeof(*wfe) - sizeof(*fmt)) && fmt->cbSize != sizeof(*wfe))
            break;
        if (IsEqualGUID(&wfe->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) &&
            (!wfe->Samples.wValidBitsPerSample || wfe->Samples.wValidBitsPerSample == 32) &&
            fmt->wBitsPerSample == 32)
            This->ss.format = PA_SAMPLE_FLOAT32LE;
        else if (IsEqualGUID(&wfe->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)) {
            DWORD valid = wfe->Samples.wValidBitsPerSample;
            if (!valid)
                valid = fmt->wBitsPerSample;
            if (!valid || valid > fmt->wBitsPerSample)
                break;
            switch (fmt->wBitsPerSample) {
                case 8:
                    if (valid == 8)
                        This->ss.format = PA_SAMPLE_U8;
                    break;
                case 16:
                    if (valid == 16)
                        This->ss.format = PA_SAMPLE_S16LE;
                    break;
                case 24:
                    if (valid == 24)
                        This->ss.format = PA_SAMPLE_S24LE;
                    break;
                case 32:
                    if (valid == 24)
                        This->ss.format = PA_SAMPLE_S24_32LE;
                    else if (valid == 32)
                        This->ss.format = PA_SAMPLE_S32LE;
                    break;
                default:
                    return AUDCLNT_E_UNSUPPORTED_FORMAT;
            }
        }
        This->map.channels = fmt->nChannels;
        if (!mask || mask == SPEAKER_ALL)
            mask = get_channel_mask(fmt->nChannels);
        else if (mask == ~0U && fmt->nChannels == 1)
            mask = SPEAKER_FRONT_CENTER;
        for (j = 0; j < sizeof(pulse_pos_from_wfx)/sizeof(*pulse_pos_from_wfx) && i < fmt->nChannels; ++j) {
            if (mask & (1 << j))
                This->map.map[i++] = pulse_pos_from_wfx[j];
        }

        /* Special case for mono since pulse appears to map it differently */
        if (mask == SPEAKER_FRONT_CENTER)
            This->map.map[0] = PA_CHANNEL_POSITION_MONO;

        if (i < fmt->nChannels || (mask & SPEAKER_RESERVED)) {
            This->map.channels = 0;
            ERR("Invalid channel mask: %i/%i and %x(%x)\n", i, fmt->nChannels, mask, wfe->dwChannelMask);
            break;
        }
        break;
        }
    case WAVE_FORMAT_ALAW:
    case WAVE_FORMAT_MULAW:
        if (fmt->wBitsPerSample != 8) {
            FIXME("Unsupported bpp %u for LAW\n", fmt->wBitsPerSample);
            return AUDCLNT_E_UNSUPPORTED_FORMAT;
        }
        if (fmt->nChannels != 1 && fmt->nChannels != 2) {
            FIXME("Unsupported channels %u for LAW\n", fmt->nChannels);
            return AUDCLNT_E_UNSUPPORTED_FORMAT;
        }
        This->ss.format = fmt->wFormatTag == WAVE_FORMAT_MULAW ? PA_SAMPLE_ULAW : PA_SAMPLE_ALAW;
        pa_channel_map_init_auto(&This->map, fmt->nChannels, PA_CHANNEL_MAP_ALSA);
        break;
    default:
        WARN("Unhandled tag %x\n", fmt->wFormatTag);
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    }
    This->ss.channels = This->map.channels;
    if (!pa_channel_map_valid(&This->map) || This->ss.format == PA_SAMPLE_INVALID) {
        ERR("Invalid format! Channel spec valid: %i, format: %i\n", pa_channel_map_valid(&This->map), This->ss.format);
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    }
    return S_OK;
}

static HRESULT WINAPI AudioClient_Initialize(IAudioClient *iface,
        AUDCLNT_SHAREMODE mode, DWORD flags, REFERENCE_TIME duration,
        REFERENCE_TIME period, const WAVEFORMATEX *fmt,
        const GUID *sessionguid)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    HRESULT hr = S_OK;
    UINT period_bytes;

    TRACE("(%p)->(%x, %x, %s, %s, %p, %s)\n", This, mode, flags,
          wine_dbgstr_longlong(duration), wine_dbgstr_longlong(period), fmt, debugstr_guid(sessionguid));

    if (!fmt)
        return E_POINTER;

    if (mode != AUDCLNT_SHAREMODE_SHARED && mode != AUDCLNT_SHAREMODE_EXCLUSIVE)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (mode == AUDCLNT_SHAREMODE_EXCLUSIVE)
        return AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED;

    if (flags & ~(AUDCLNT_STREAMFLAGS_CROSSPROCESS |
                AUDCLNT_STREAMFLAGS_LOOPBACK |
                AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
                AUDCLNT_STREAMFLAGS_NOPERSIST |
                AUDCLNT_STREAMFLAGS_RATEADJUST |
                AUDCLNT_SESSIONFLAGS_EXPIREWHENUNOWNED |
                AUDCLNT_SESSIONFLAGS_DISPLAY_HIDE |
                AUDCLNT_SESSIONFLAGS_DISPLAY_HIDEWHENEXPIRED)) {
        TRACE("Unknown flags: %08x\n", flags);
        return E_INVALIDARG;
    }

    pthread_mutex_lock(&pulse_lock);

    hr = pulse_connect();
    if (FAILED(hr)) {
        pthread_mutex_unlock(&pulse_lock);
        return hr;
    }

    if (This->stream) {
        pthread_mutex_unlock(&pulse_lock);
        return AUDCLNT_E_ALREADY_INITIALIZED;
    }

    hr = pulse_spec_from_waveformat(This, fmt);
    TRACE("Obtaining format returns %08x\n", hr);
    dump_fmt(fmt);

    if (FAILED(hr))
        goto exit;

    if (mode == AUDCLNT_SHAREMODE_SHARED) {
        REFERENCE_TIME def = pulse_def_period[This->dataflow == eCapture];
        REFERENCE_TIME min = pulse_min_period[This->dataflow == eCapture];

        /* Switch to low latency mode if below 2 default periods,
         * which is 20 ms by default, this will increase the amount
         * of interrupts but allows very low latency. In dsound I
         * managed to get a total latency of ~8ms, which is well below
         * default
         */
        if (duration < 2 * def)
            period = min;
        else
            period = def;
        if (duration < 2 * period)
            duration = 2 * period;

        /* Uh oh, really low latency requested.. */
        if (duration <= 2 * period)
            period /= 2;
    }
    period_bytes = pa_frame_size(&This->ss) * MulDiv(period, This->ss.rate, 10000000);

    if (duration < 20000000)
        This->bufsize_frames = ceil((duration / 10000000.) * fmt->nSamplesPerSec);
    else
        This->bufsize_frames = 2 * fmt->nSamplesPerSec;
    This->bufsize_bytes = This->bufsize_frames * pa_frame_size(&This->ss);

    This->share = mode;
    This->flags = flags;
    hr = pulse_stream_connect(This, period_bytes);
    if (SUCCEEDED(hr)) {
        UINT32 unalign;
        const pa_buffer_attr *attr = pa_stream_get_buffer_attr(This->stream);
        /* Update frames according to new size */
        dump_attr(attr);
        if (This->dataflow == eRender)
            This->bufsize_bytes = attr->tlength;
        else {
            This->capture_period = period_bytes = attr->fragsize;
            if ((unalign = This->bufsize_bytes % period_bytes))
                This->bufsize_bytes += period_bytes - unalign;
        }
        This->bufsize_frames = This->bufsize_bytes / pa_frame_size(&This->ss);
    }
    if (SUCCEEDED(hr)) {
        UINT32 i, capture_packets = This->capture_period ? This->bufsize_bytes / This->capture_period : 0;
        This->tmp_buffer = HeapAlloc(GetProcessHeap(), 0, This->bufsize_bytes + capture_packets * sizeof(ACPacket));
        if (!This->tmp_buffer)
            hr = E_OUTOFMEMORY;
        else {
            ACPacket *cur_packet = (ACPacket*)((char*)This->tmp_buffer + This->bufsize_bytes);
            BYTE *data = This->tmp_buffer;
            memset(This->tmp_buffer, This->ss.format == PA_SAMPLE_U8 ? 0x80 : 0, This->bufsize_bytes);
            list_init(&This->packet_free_head);
            list_init(&This->packet_filled_head);
            for (i = 0; i < capture_packets; ++i, ++cur_packet) {
                list_add_tail(&This->packet_free_head, &cur_packet->entry);
                cur_packet->data = data;
                data += This->capture_period;
            }
            assert(!This->capture_period || This->bufsize_bytes == This->capture_period * capture_packets);
            assert(!capture_packets || data - This->bufsize_bytes == This->tmp_buffer);
        }
    }

exit:
    if (FAILED(hr)) {
        HeapFree(GetProcessHeap(), 0, This->tmp_buffer);
        This->tmp_buffer = NULL;
        if (This->stream) {
            pa_stream_disconnect(This->stream);
            pa_stream_unref(This->stream);
            This->stream = NULL;
        }
    }
    pthread_mutex_unlock(&pulse_lock);
    return hr;
}

static HRESULT WINAPI AudioClient_GetBufferSize(IAudioClient *iface,
        UINT32 *out)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, out);

    if (!out)
        return E_POINTER;

    pthread_mutex_lock(&pulse_lock);
    hr = pulse_stream_valid(This);
    if (SUCCEEDED(hr))
        *out = This->bufsize_frames;
    pthread_mutex_unlock(&pulse_lock);

    return hr;
}

static HRESULT WINAPI AudioClient_GetStreamLatency(IAudioClient *iface,
        REFERENCE_TIME *latency)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    const pa_buffer_attr *attr;
    REFERENCE_TIME lat;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, latency);

    if (!latency)
        return E_POINTER;

    pthread_mutex_lock(&pulse_lock);
    hr = pulse_stream_valid(This);
    if (FAILED(hr)) {
        pthread_mutex_unlock(&pulse_lock);
        return hr;
    }
    attr = pa_stream_get_buffer_attr(This->stream);
    if (This->dataflow == eRender)
        lat = attr->minreq / pa_frame_size(&This->ss);
    else
        lat = attr->fragsize / pa_frame_size(&This->ss);
    *latency = 10000000;
    *latency *= lat;
    *latency /= This->ss.rate;
    pthread_mutex_unlock(&pulse_lock);
    TRACE("Latency: %u ms\n", (DWORD)(*latency / 10000));
    return S_OK;
}

static void ACImpl_GetRenderPad(ACImpl *This, UINT32 *out)
{
    *out = This->pad / pa_frame_size(&This->ss);
}

static void ACImpl_GetCapturePad(ACImpl *This, UINT32 *out)
{
    ACPacket *packet = This->locked_ptr;
    if (!packet && !list_empty(&This->packet_filled_head)) {
        packet = (ACPacket*)list_head(&This->packet_filled_head);
        This->locked_ptr = packet;
        list_remove(&packet->entry);
    }
    if (out)
        *out = This->pad / pa_frame_size(&This->ss);
}

static HRESULT WINAPI AudioClient_GetCurrentPadding(IAudioClient *iface,
        UINT32 *out)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, out);

    if (!out)
        return E_POINTER;

    pthread_mutex_lock(&pulse_lock);
    hr = pulse_stream_valid(This);
    if (FAILED(hr)) {
        pthread_mutex_unlock(&pulse_lock);
        return hr;
    }

    if (This->dataflow == eRender)
        ACImpl_GetRenderPad(This, out);
    else
        ACImpl_GetCapturePad(This, out);
    pthread_mutex_unlock(&pulse_lock);

    TRACE("%p Pad: %u ms (%u)\n", This, MulDiv(*out, 1000, This->ss.rate), *out);
    return S_OK;
}

static HRESULT WINAPI AudioClient_IsFormatSupported(IAudioClient *iface,
        AUDCLNT_SHAREMODE mode, const WAVEFORMATEX *fmt,
        WAVEFORMATEX **out)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    HRESULT hr = S_OK;
    WAVEFORMATEX *closest = NULL;

    TRACE("(%p)->(%x, %p, %p)\n", This, mode, fmt, out);

    if (!fmt || (mode == AUDCLNT_SHAREMODE_SHARED && !out))
        return E_POINTER;

    if (out)
        *out = NULL;
    if (mode != AUDCLNT_SHAREMODE_SHARED && mode != AUDCLNT_SHAREMODE_EXCLUSIVE)
        return E_INVALIDARG;
    if (mode == AUDCLNT_SHAREMODE_EXCLUSIVE)
        return This->dataflow == eCapture ? AUDCLNT_E_UNSUPPORTED_FORMAT : AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED;
    switch (fmt->wFormatTag) {
    case WAVE_FORMAT_EXTENSIBLE:
        if (fmt->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
            return E_INVALIDARG;
        dump_fmt(fmt);
        break;
    case WAVE_FORMAT_ALAW:
    case WAVE_FORMAT_MULAW:
    case WAVE_FORMAT_IEEE_FLOAT:
    case WAVE_FORMAT_PCM:
        dump_fmt(fmt);
        break;
    default:
        dump_fmt(fmt);
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    }
    if (fmt->nChannels == 0)
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    closest = clone_format(fmt);
    if (!closest) {
        if (out)
            *out = NULL;
        return E_OUTOFMEMORY;
    }

    if (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        UINT32 mask = 0, i, channels = 0;
        WAVEFORMATEXTENSIBLE *ext = (WAVEFORMATEXTENSIBLE*)closest;

        if ((fmt->nChannels > 1 && ext->dwChannelMask == SPEAKER_ALL) ||
            (fmt->nChannels == 1 && ext->dwChannelMask == ~0U)) {
            mask = ext->dwChannelMask;
            channels = fmt->nChannels;
        } else if (ext->dwChannelMask) {
            for (i = 1; !(i & SPEAKER_RESERVED); i <<= 1) {
                if (i & ext->dwChannelMask) {
                    mask |= i;
                    channels++;
                }
            }
            if (channels < fmt->nChannels)
                mask = get_channel_mask(fmt->nChannels);
        } else
            mask = ext->dwChannelMask;
        if (ext->dwChannelMask != mask) {
            ext->dwChannelMask = mask;
            hr = S_FALSE;
        }
    }

    if (hr == S_OK || !out) {
        CoTaskMemFree(closest);
        if (out)
            *out = NULL;
    } else if (closest) {
        closest->nBlockAlign =
            closest->nChannels * closest->wBitsPerSample / 8;
        closest->nAvgBytesPerSec =
            closest->nBlockAlign * closest->nSamplesPerSec;
        *out = closest;
    }

    TRACE("returning: %08x %p\n", hr, out ? *out : NULL);
    return hr;
}

static HRESULT WINAPI AudioClient_GetMixFormat(IAudioClient *iface,
        WAVEFORMATEX **pwfx)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    WAVEFORMATEXTENSIBLE *fmt = &pulse_fmt[This->dataflow == eCapture];

    TRACE("(%p)->(%p)\n", This, pwfx);

    if (!pwfx)
        return E_POINTER;

    *pwfx = clone_format(&fmt->Format);
    if (!*pwfx)
        return E_OUTOFMEMORY;
    dump_fmt(*pwfx);
    return S_OK;
}

static HRESULT WINAPI AudioClient_GetDevicePeriod(IAudioClient *iface,
        REFERENCE_TIME *defperiod, REFERENCE_TIME *minperiod)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)->(%p, %p)\n", This, defperiod, minperiod);

    if (!defperiod && !minperiod)
        return E_POINTER;

    if (defperiod)
        *defperiod = pulse_def_period[This->dataflow == eCapture];
    if (minperiod)
        *minperiod = pulse_min_period[This->dataflow == eCapture];

    return S_OK;
}

static HRESULT WINAPI AudioClient_Start(IAudioClient *iface)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    HRESULT hr = S_OK;
    int success;
    pa_operation *o;

    TRACE("(%p)\n", This);

    pthread_mutex_lock(&pulse_lock);
    hr = pulse_stream_valid(This);
    if (FAILED(hr)) {
        pthread_mutex_unlock(&pulse_lock);
        return hr;
    }

    if ((This->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK) && !This->event) {
        pthread_mutex_unlock(&pulse_lock);
        return AUDCLNT_E_EVENTHANDLE_NOT_SET;
    }

    if (This->started) {
        pthread_mutex_unlock(&pulse_lock);
        return AUDCLNT_E_NOT_STOPPED;
    }

    if (pa_stream_is_corked(This->stream)) {
        o = pa_stream_cork(This->stream, 0, pulse_op_cb, &success);
        if (o) {
            while(pa_operation_get_state(o) == PA_OPERATION_RUNNING)
                pthread_cond_wait(&pulse_cond, &pulse_lock);
            pa_operation_unref(o);
        } else
            success = 0;
        if (!success)
            hr = E_FAIL;
    }
    if (SUCCEEDED(hr)) {
        This->started = TRUE;
        if (This->dataflow == eRender && This->event)
            pa_stream_set_latency_update_callback(This->stream, pulse_latency_callback, This);
    }
    pthread_mutex_unlock(&pulse_lock);
    return hr;
}

static HRESULT WINAPI AudioClient_Stop(IAudioClient *iface)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    HRESULT hr = S_OK;
    pa_operation *o;
    int success;

    TRACE("(%p)\n", This);

    pthread_mutex_lock(&pulse_lock);
    hr = pulse_stream_valid(This);
    if (FAILED(hr)) {
        pthread_mutex_unlock(&pulse_lock);
        return hr;
    }

    if (!This->started) {
        pthread_mutex_unlock(&pulse_lock);
        return S_FALSE;
    }

    if (This->dataflow == eRender) {
        o = pa_stream_cork(This->stream, 1, pulse_op_cb, &success);
        if (o) {
            while(pa_operation_get_state(o) == PA_OPERATION_RUNNING)
                pthread_cond_wait(&pulse_cond, &pulse_lock);
            pa_operation_unref(o);
        } else
            success = 0;
        if (!success)
            hr = E_FAIL;
    }
    if (SUCCEEDED(hr)) {
        This->started = FALSE;
    }
    pthread_mutex_unlock(&pulse_lock);
    return hr;
}

static HRESULT WINAPI AudioClient_Reset(IAudioClient *iface)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    HRESULT hr = S_OK;

    TRACE("(%p)\n", This);

    pthread_mutex_lock(&pulse_lock);
    hr = pulse_stream_valid(This);
    if (FAILED(hr)) {
        pthread_mutex_unlock(&pulse_lock);
        return hr;
    }

    if (This->started) {
        pthread_mutex_unlock(&pulse_lock);
        return AUDCLNT_E_NOT_STOPPED;
    }

    if (This->locked) {
        pthread_mutex_unlock(&pulse_lock);
        return AUDCLNT_E_BUFFER_OPERATION_PENDING;
    }

    if (This->dataflow == eRender) {
        /* If there is still data in the render buffer it needs to be removed from the server */
        int success = 0;
        if (This->pad) {
            pa_operation *o = pa_stream_flush(This->stream, pulse_op_cb, &success);
            if (o) {
                while(pa_operation_get_state(o) == PA_OPERATION_RUNNING)
                    pthread_cond_wait(&pulse_cond, &pulse_lock);
                pa_operation_unref(o);
            }
        }
        if (success || !This->pad)
            This->clock_lastpos = This->clock_written = This->pad = 0;
    } else {
        ACPacket *p;
        This->clock_written += This->pad;
        This->pad = 0;

        if ((p = This->locked_ptr)) {
            This->locked_ptr = NULL;
            list_add_tail(&This->packet_free_head, &p->entry);
        }
        list_move_tail(&This->packet_free_head, &This->packet_filled_head);
    }
    pthread_mutex_unlock(&pulse_lock);

    return hr;
}

static HRESULT WINAPI AudioClient_SetEventHandle(IAudioClient *iface,
        HANDLE event)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, event);

    if (!event)
        return E_INVALIDARG;

    pthread_mutex_lock(&pulse_lock);
    hr = pulse_stream_valid(This);
    if (FAILED(hr)) {
        pthread_mutex_unlock(&pulse_lock);
        return hr;
    }

    if (!(This->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK))
        hr = AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED;
    else if (This->event)
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
    else
        This->event = event;
    pthread_mutex_unlock(&pulse_lock);
    return hr;
}

static HRESULT WINAPI AudioClient_GetService(IAudioClient *iface, REFIID riid,
        void **ppv)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    HRESULT hr;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    pthread_mutex_lock(&pulse_lock);
    hr = pulse_stream_valid(This);
    pthread_mutex_unlock(&pulse_lock);
    if (FAILED(hr))
        return hr;

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
    }

    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

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
    size_t avail, req, bytes = frames * pa_frame_size(&This->ss);
    UINT32 pad;
    HRESULT hr = S_OK;
    int ret = -1;

    TRACE("(%p)->(%u, %p)\n", This, frames, data);

    if (!data)
        return E_POINTER;
    *data = NULL;

    pthread_mutex_lock(&pulse_lock);
    hr = pulse_stream_valid(This);
    if (FAILED(hr) || This->locked) {
        pthread_mutex_unlock(&pulse_lock);
        return FAILED(hr) ? hr : AUDCLNT_E_OUT_OF_ORDER;
    }
    if (!frames) {
        pthread_mutex_unlock(&pulse_lock);
        return S_OK;
    }

    ACImpl_GetRenderPad(This, &pad);
    avail = This->bufsize_frames - pad;
    if (avail < frames || bytes > This->bufsize_bytes) {
        pthread_mutex_unlock(&pulse_lock);
        WARN("Wanted to write %u, but only %zu available\n", frames, avail);
        return AUDCLNT_E_BUFFER_TOO_LARGE;
    }

    This->locked = frames;
    req = bytes;
    ret = pa_stream_begin_write(This->stream, &This->locked_ptr, &req);
    if (ret < 0 || req < bytes) {
        FIXME("%p Not using pulse locked data: %i %zu/%u %u/%u\n", This, ret, req/pa_frame_size(&This->ss), frames, pad, This->bufsize_frames);
        if (ret >= 0)
            pa_stream_cancel_write(This->stream);
        *data = This->tmp_buffer;
        This->locked_ptr = NULL;
    } else
        *data = This->locked_ptr;
    pthread_mutex_unlock(&pulse_lock);
    return hr;
}

static void pulse_free_noop(void *buf)
{
}

static HRESULT WINAPI AudioRenderClient_ReleaseBuffer(
        IAudioRenderClient *iface, UINT32 written_frames, DWORD flags)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    UINT32 written_bytes = written_frames * pa_frame_size(&This->ss);

    TRACE("(%p)->(%u, %x)\n", This, written_frames, flags);

    pthread_mutex_lock(&pulse_lock);
    if (!This->locked || !written_frames) {
        if (This->locked_ptr)
            pa_stream_cancel_write(This->stream);
        This->locked = 0;
        This->locked_ptr = NULL;
        pthread_mutex_unlock(&pulse_lock);
        return written_frames ? AUDCLNT_E_OUT_OF_ORDER : S_OK;
    }

    if (This->locked < written_frames) {
        pthread_mutex_unlock(&pulse_lock);
        return AUDCLNT_E_INVALID_SIZE;
    }

    if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
        if (This->ss.format == PA_SAMPLE_U8)
            memset(This->tmp_buffer, 128, written_bytes);
        else
            memset(This->tmp_buffer, 0, written_bytes);
    }

    This->locked = 0;
    if (This->locked_ptr)
        pa_stream_write(This->stream, This->locked_ptr, written_bytes, NULL, 0, PA_SEEK_RELATIVE);
    else
        pa_stream_write(This->stream, This->tmp_buffer, written_bytes, pulse_free_noop, 0, PA_SEEK_RELATIVE);
    This->pad += written_bytes;
    This->locked_ptr = NULL;
    TRACE("Released %u, pad %zu\n", written_frames, This->pad / pa_frame_size(&This->ss));
    assert(This->pad <= This->bufsize_bytes);

    pthread_mutex_unlock(&pulse_lock);
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
    HRESULT hr;
    ACPacket *packet;

    TRACE("(%p)->(%p, %p, %p, %p, %p)\n", This, data, frames, flags,
            devpos, qpcpos);

    if (!data || !frames || !flags)
        return E_POINTER;

    pthread_mutex_lock(&pulse_lock);
    hr = pulse_stream_valid(This);
    if (FAILED(hr) || This->locked) {
        pthread_mutex_unlock(&pulse_lock);
        return FAILED(hr) ? hr : AUDCLNT_E_OUT_OF_ORDER;
    }

    ACImpl_GetCapturePad(This, NULL);
    if ((packet = This->locked_ptr)) {
        *frames = This->capture_period / pa_frame_size(&This->ss);
        *flags = 0;
        if (packet->discont)
            *flags |= AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY;
        if (devpos) {
            if (packet->discont)
                *devpos = (This->clock_written + This->capture_period) / pa_frame_size(&This->ss);
            else
                *devpos = This->clock_written / pa_frame_size(&This->ss);
        }
        if (qpcpos)
            *qpcpos = packet->qpcpos;
        *data = packet->data;
    }
    else
        *frames = 0;
    This->locked = *frames;
    pthread_mutex_unlock(&pulse_lock);
    return *frames ? S_OK : AUDCLNT_S_BUFFER_EMPTY;
}

static HRESULT WINAPI AudioCaptureClient_ReleaseBuffer(
        IAudioCaptureClient *iface, UINT32 done)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);

    TRACE("(%p)->(%u)\n", This, done);

    pthread_mutex_lock(&pulse_lock);
    if (!This->locked && done) {
        pthread_mutex_unlock(&pulse_lock);
        return AUDCLNT_E_OUT_OF_ORDER;
    }
    if (done && This->locked != done) {
        pthread_mutex_unlock(&pulse_lock);
        return AUDCLNT_E_INVALID_SIZE;
    }
    if (done) {
        ACPacket *packet = This->locked_ptr;
        This->locked_ptr = NULL;
        This->pad -= This->capture_period;
        if (packet->discont)
            This->clock_written += 2 * This->capture_period;
        else
            This->clock_written += This->capture_period;
        list_add_tail(&This->packet_free_head, &packet->entry);
    }
    This->locked = 0;
    pthread_mutex_unlock(&pulse_lock);
    return S_OK;
}

static HRESULT WINAPI AudioCaptureClient_GetNextPacketSize(
        IAudioCaptureClient *iface, UINT32 *frames)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);

    TRACE("(%p)->(%p)\n", This, frames);
    if (!frames)
        return E_POINTER;

    pthread_mutex_lock(&pulse_lock);
    ACImpl_GetCapturePad(This, NULL);
    if (This->locked_ptr)
        *frames = This->capture_period / pa_frame_size(&This->ss);
    else
        *frames = 0;
    pthread_mutex_unlock(&pulse_lock);
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
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, freq);

    pthread_mutex_lock(&pulse_lock);
    hr = pulse_stream_valid(This);
    if (SUCCEEDED(hr))
        *freq = This->ss.rate * pa_frame_size(&This->ss);
    pthread_mutex_unlock(&pulse_lock);
    return hr;
}

static HRESULT WINAPI AudioClock_GetPosition(IAudioClock *iface, UINT64 *pos,
        UINT64 *qpctime)
{
    ACImpl *This = impl_from_IAudioClock(iface);
    HRESULT hr;

    TRACE("(%p)->(%p, %p)\n", This, pos, qpctime);

    if (!pos)
        return E_POINTER;

    pthread_mutex_lock(&pulse_lock);
    hr = pulse_stream_valid(This);
    if (FAILED(hr)) {
        pthread_mutex_unlock(&pulse_lock);
        return hr;
    }

    *pos = This->clock_written;

    /* Make time never go backwards */
    if (*pos < This->clock_lastpos)
        *pos = This->clock_lastpos;
    else
        This->clock_lastpos = *pos;
    pthread_mutex_unlock(&pulse_lock);

    TRACE("%p Position: %u\n", This, (unsigned)*pos);

    if (qpctime) {
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
    HRESULT hr = AudioClock_GetPosition(&This->IAudioClock_iface, pos, qpctime);
    if (SUCCEEDED(hr))
        *pos /= pa_frame_size(&This->ss);
    return hr;
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

    if (!out)
        return E_POINTER;

    *out = This->ss.channels;

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
    HRESULT hr;
    int i;

    TRACE("(%p)->(%d, %p)\n", This, count, levels);

    if (!levels)
        return E_POINTER;

    if (count != This->ss.channels)
        return E_INVALIDARG;

    pthread_mutex_lock(&pulse_lock);
    hr = pulse_stream_valid(This);
    if (FAILED(hr))
        goto out;

    for (i = 0; i < count; ++i)
        This->vol[i] = levels[i];

out:
    pthread_mutex_unlock(&pulse_lock);
    return hr;
}

static HRESULT WINAPI AudioStreamVolume_GetAllVolumes(
        IAudioStreamVolume *iface, UINT32 count, float *levels)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    HRESULT hr;
    int i;

    TRACE("(%p)->(%d, %p)\n", This, count, levels);

    if (!levels)
        return E_POINTER;

    if (count != This->ss.channels)
        return E_INVALIDARG;

    pthread_mutex_lock(&pulse_lock);
    hr = pulse_stream_valid(This);
    if (FAILED(hr))
        goto out;

    for (i = 0; i < count; ++i)
        levels[i] = This->vol[i];

out:
    pthread_mutex_unlock(&pulse_lock);
    return hr;
}

static HRESULT WINAPI AudioStreamVolume_SetChannelVolume(
        IAudioStreamVolume *iface, UINT32 index, float level)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    HRESULT hr;
    float volumes[PA_CHANNELS_MAX];

    TRACE("(%p)->(%d, %f)\n", This, index, level);

    if (level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if (index >= This->ss.channels)
        return E_INVALIDARG;

    hr = AudioStreamVolume_GetAllVolumes(iface, This->ss.channels, volumes);
    volumes[index] = level;
    if (SUCCEEDED(hr))
        hr = AudioStreamVolume_SetAllVolumes(iface, This->ss.channels, volumes);
    return hr;
}

static HRESULT WINAPI AudioStreamVolume_GetChannelVolume(
        IAudioStreamVolume *iface, UINT32 index, float *level)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    float volumes[PA_CHANNELS_MAX];
    HRESULT hr;

    TRACE("(%p)->(%d, %p)\n", This, index, level);

    if (!level)
        return E_POINTER;

    if (index >= This->ss.channels)
        return E_INVALIDARG;

    hr = AudioStreamVolume_GetAllVolumes(iface, This->ss.channels, volumes);
    if (SUCCEEDED(hr))
        *level = volumes[index];
    return hr;
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

HRESULT WINAPI AUDDRV_GetAudioSessionManager(IMMDevice *device,
        IAudioSessionManager2 **out)
{
    *out = NULL;
    return E_NOTIMPL;
}
