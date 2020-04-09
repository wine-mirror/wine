/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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

#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"
#include "mf_private.h"
#include "initguid.h"
#include "mmdeviceapi.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct audio_renderer;

struct audio_renderer_stream
{
    IMFStreamSink IMFStreamSink_iface;
    IMFMediaTypeHandler IMFMediaTypeHandler_iface;
    LONG refcount;
    struct audio_renderer *sink;
    IMFMediaEventQueue *event_queue;
    CRITICAL_SECTION cs;
};

struct audio_renderer
{
    IMFMediaSink IMFMediaSink_iface;
    IMFMediaSinkPreroll IMFMediaSinkPreroll_iface;
    IMFClockStateSink IMFClockStateSink_iface;
    IMFMediaEventGenerator IMFMediaEventGenerator_iface;
    IMFGetService IMFGetService_iface;
    IMFSimpleAudioVolume IMFSimpleAudioVolume_iface;
    IMFAudioStreamVolume IMFAudioStreamVolume_iface;
    IMFAudioPolicy IMFAudioPolicy_iface;
    LONG refcount;
    IMFMediaEventQueue *event_queue;
    IMFPresentationClock *clock;
    BOOL is_shut_down;
    struct audio_renderer_stream *stream;
    CRITICAL_SECTION cs;
};

static struct audio_renderer *impl_from_IMFMediaSink(IMFMediaSink *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer, IMFMediaSink_iface);
}

static struct audio_renderer *impl_from_IMFMediaSinkPreroll(IMFMediaSinkPreroll *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer, IMFMediaSinkPreroll_iface);
}

static struct audio_renderer *impl_from_IMFClockStateSink(IMFClockStateSink *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer, IMFClockStateSink_iface);
}

static struct audio_renderer *impl_from_IMFMediaEventGenerator(IMFMediaEventGenerator *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer, IMFMediaEventGenerator_iface);
}

static struct audio_renderer *impl_from_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer, IMFGetService_iface);
}

static struct audio_renderer *impl_from_IMFSimpleAudioVolume(IMFSimpleAudioVolume *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer, IMFSimpleAudioVolume_iface);
}

static struct audio_renderer *impl_from_IMFAudioStreamVolume(IMFAudioStreamVolume *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer, IMFAudioStreamVolume_iface);
}

static struct audio_renderer *impl_from_IMFAudioPolicy(IMFAudioPolicy *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer, IMFAudioPolicy_iface);
}

static struct audio_renderer_stream *impl_from_IMFStreamSink(IMFStreamSink *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer_stream, IMFStreamSink_iface);
}

static struct audio_renderer_stream *impl_from_IMFMediaTypeHandler(IMFMediaTypeHandler *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer_stream, IMFMediaTypeHandler_iface);
}

static HRESULT WINAPI audio_renderer_sink_QueryInterface(IMFMediaSink *iface, REFIID riid, void **obj)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFMediaSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaSinkPreroll))
    {
        *obj = &renderer->IMFMediaSinkPreroll_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFClockStateSink))
    {
        *obj = &renderer->IMFClockStateSink_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaEventGenerator))
    {
        *obj = &renderer->IMFMediaEventGenerator_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFGetService))
    {
        *obj = &renderer->IMFGetService_iface;
    }
    else
    {
        WARN("Unsupported %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);

    return S_OK;
}

static ULONG WINAPI audio_renderer_sink_AddRef(IMFMediaSink *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);
    ULONG refcount = InterlockedIncrement(&renderer->refcount);
    TRACE("%p, refcount %u.\n", iface, refcount);
    return refcount;
}

static ULONG WINAPI audio_renderer_sink_Release(IMFMediaSink *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);
    ULONG refcount = InterlockedDecrement(&renderer->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (renderer->event_queue)
            IMFMediaEventQueue_Release(renderer->event_queue);
        if (renderer->clock)
            IMFPresentationClock_Release(renderer->clock);
        DeleteCriticalSection(&renderer->cs);
        heap_free(renderer);
    }

    return refcount;
}

static HRESULT WINAPI audio_renderer_sink_GetCharacteristics(IMFMediaSink *iface, DWORD *flags)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p, %p.\n", iface, flags);

    if (renderer->is_shut_down)
        return MF_E_SHUTDOWN;

    *flags = MEDIASINK_FIXED_STREAMS | MEDIASINK_CAN_PREROLL;

    return S_OK;
}

static HRESULT WINAPI audio_renderer_sink_AddStreamSink(IMFMediaSink *iface, DWORD stream_sink_id,
    IMFMediaType *media_type, IMFStreamSink **stream_sink)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p, %#x, %p, %p.\n", iface, stream_sink_id, media_type, stream_sink);

    return renderer->is_shut_down ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI audio_renderer_sink_RemoveStreamSink(IMFMediaSink *iface, DWORD stream_sink_id)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p, %#x.\n", iface, stream_sink_id);

    return renderer->is_shut_down ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI audio_renderer_sink_GetStreamSinkCount(IMFMediaSink *iface, DWORD *count)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_POINTER;

    if (renderer->is_shut_down)
        return MF_E_SHUTDOWN;

    *count = 1;

    return S_OK;
}

static HRESULT WINAPI audio_renderer_sink_GetStreamSinkByIndex(IMFMediaSink *iface, DWORD index,
        IMFStreamSink **stream)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p.\n", iface, index, stream);

    if (renderer->is_shut_down)
        return MF_E_SHUTDOWN;

    EnterCriticalSection(&renderer->cs);

    if (renderer->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else if (index > 0)
        hr = MF_E_INVALIDINDEX;
    else
    {
       *stream = &renderer->stream->IMFStreamSink_iface;
       IMFStreamSink_AddRef(*stream);
    }

    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_sink_GetStreamSinkById(IMFMediaSink *iface, DWORD stream_sink_id,
        IMFStreamSink **stream)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %#x, %p.\n", iface, stream_sink_id, stream);

    EnterCriticalSection(&renderer->cs);

    if (renderer->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else if (stream_sink_id > 0)
        hr = MF_E_INVALIDSTREAMNUMBER;
    else
    {
        *stream = &renderer->stream->IMFStreamSink_iface;
        IMFStreamSink_AddRef(*stream);
    }

    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_sink_SetPresentationClock(IMFMediaSink *iface, IMFPresentationClock *clock)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, clock);

    EnterCriticalSection(&renderer->cs);

    if (renderer->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else
    {
        if (renderer->clock)
        {
            IMFPresentationClock_RemoveClockStateSink(renderer->clock, &renderer->IMFClockStateSink_iface);
            IMFPresentationClock_Release(renderer->clock);
        }
        renderer->clock = clock;
        if (renderer->clock)
        {
            IMFPresentationClock_AddRef(renderer->clock);
            IMFPresentationClock_AddClockStateSink(renderer->clock, &renderer->IMFClockStateSink_iface);
        }
    }

    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_sink_GetPresentationClock(IMFMediaSink *iface, IMFPresentationClock **clock)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, clock);

    if (!clock)
        return E_POINTER;

    EnterCriticalSection(&renderer->cs);

    if (renderer->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else if (renderer->clock)
    {
        *clock = renderer->clock;
        IMFPresentationClock_AddRef(*clock);
    }
    else
        hr = MF_E_NO_CLOCK;

    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_sink_Shutdown(IMFMediaSink *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p.\n", iface);

    if (renderer->is_shut_down)
        return MF_E_SHUTDOWN;

    EnterCriticalSection(&renderer->cs);

    renderer->is_shut_down = TRUE;
    IMFMediaEventQueue_Shutdown(renderer->event_queue);

    /* Detach stream. */
    IMFMediaSink_Release(&renderer->stream->sink->IMFMediaSink_iface);
    EnterCriticalSection(&renderer->stream->cs);
    renderer->stream->sink = NULL;
    IMFMediaEventQueue_Shutdown(renderer->stream->event_queue);
    LeaveCriticalSection(&renderer->stream->cs);

    IMFStreamSink_Release(&renderer->stream->IMFStreamSink_iface);
    renderer->stream = NULL;

    LeaveCriticalSection(&renderer->cs);

    return S_OK;
}

static const IMFMediaSinkVtbl audio_renderer_sink_vtbl =
{
    audio_renderer_sink_QueryInterface,
    audio_renderer_sink_AddRef,
    audio_renderer_sink_Release,
    audio_renderer_sink_GetCharacteristics,
    audio_renderer_sink_AddStreamSink,
    audio_renderer_sink_RemoveStreamSink,
    audio_renderer_sink_GetStreamSinkCount,
    audio_renderer_sink_GetStreamSinkByIndex,
    audio_renderer_sink_GetStreamSinkById,
    audio_renderer_sink_SetPresentationClock,
    audio_renderer_sink_GetPresentationClock,
    audio_renderer_sink_Shutdown,
};

static HRESULT WINAPI audio_renderer_preroll_QueryInterface(IMFMediaSinkPreroll *iface, REFIID riid, void **obj)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSinkPreroll(iface);
    return IMFMediaSink_QueryInterface(&renderer->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI audio_renderer_preroll_AddRef(IMFMediaSinkPreroll *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSinkPreroll(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI audio_renderer_preroll_Release(IMFMediaSinkPreroll *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSinkPreroll(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI audio_renderer_preroll_NotifyPreroll(IMFMediaSinkPreroll *iface, MFTIME start_time)
{
    FIXME("%p, %s.\n", iface, debugstr_time(start_time));

    return E_NOTIMPL;
}

static const IMFMediaSinkPrerollVtbl audio_renderer_preroll_vtbl =
{
    audio_renderer_preroll_QueryInterface,
    audio_renderer_preroll_AddRef,
    audio_renderer_preroll_Release,
    audio_renderer_preroll_NotifyPreroll,
};

static HRESULT WINAPI audio_renderer_events_QueryInterface(IMFMediaEventGenerator *iface, REFIID riid, void **obj)
{
    struct audio_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_QueryInterface(&renderer->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI audio_renderer_events_AddRef(IMFMediaEventGenerator *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI audio_renderer_events_Release(IMFMediaEventGenerator *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI audio_renderer_events_GetEvent(IMFMediaEventGenerator *iface, DWORD flags, IMFMediaEvent **event)
{
    struct audio_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %#x, %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(renderer->event_queue, flags, event);
}

static HRESULT WINAPI audio_renderer_events_BeginGetEvent(IMFMediaEventGenerator *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct audio_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(renderer->event_queue, callback, state);
}

static HRESULT WINAPI audio_renderer_events_EndGetEvent(IMFMediaEventGenerator *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct audio_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(renderer->event_queue, result, event);
}

static HRESULT WINAPI audio_renderer_events_QueueEvent(IMFMediaEventGenerator *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct audio_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %u, %s, %#x, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(renderer->event_queue, event_type, ext_type, hr, value);
}

static const IMFMediaEventGeneratorVtbl audio_renderer_events_vtbl =
{
    audio_renderer_events_QueryInterface,
    audio_renderer_events_AddRef,
    audio_renderer_events_Release,
    audio_renderer_events_GetEvent,
    audio_renderer_events_BeginGetEvent,
    audio_renderer_events_EndGetEvent,
    audio_renderer_events_QueueEvent,
};

static HRESULT WINAPI audio_renderer_clock_sink_QueryInterface(IMFClockStateSink *iface, REFIID riid, void **obj)
{
    struct audio_renderer *renderer = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_QueryInterface(&renderer->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI audio_renderer_clock_sink_AddRef(IMFClockStateSink *iface)
{
    struct audio_renderer *renderer = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI audio_renderer_clock_sink_Release(IMFClockStateSink *iface)
{
    struct audio_renderer *renderer = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI audio_renderer_clock_sink_OnClockStart(IMFClockStateSink *iface, MFTIME systime, LONGLONG offset)
{
    FIXME("%p, %s, %s.\n", iface, debugstr_time(systime), debugstr_time(offset));

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_clock_sink_OnClockStop(IMFClockStateSink *iface, MFTIME systime)
{
    FIXME("%p, %s.\n", iface, debugstr_time(systime));

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_clock_sink_OnClockPause(IMFClockStateSink *iface, MFTIME systime)
{
    FIXME("%p, %s.\n", iface, debugstr_time(systime));

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_clock_sink_OnClockRestart(IMFClockStateSink *iface, MFTIME systime)
{
    FIXME("%p, %s.\n", iface, debugstr_time(systime));

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_clock_sink_OnClockSetRate(IMFClockStateSink *iface, MFTIME systime, float rate)
{
    FIXME("%p, %s, %f.\n", iface, debugstr_time(systime), rate);

    return E_NOTIMPL;
}

static const IMFClockStateSinkVtbl audio_renderer_clock_sink_vtbl =
{
    audio_renderer_clock_sink_QueryInterface,
    audio_renderer_clock_sink_AddRef,
    audio_renderer_clock_sink_Release,
    audio_renderer_clock_sink_OnClockStart,
    audio_renderer_clock_sink_OnClockStop,
    audio_renderer_clock_sink_OnClockPause,
    audio_renderer_clock_sink_OnClockRestart,
    audio_renderer_clock_sink_OnClockSetRate,
};

static HRESULT WINAPI audio_renderer_get_service_QueryInterface(IMFGetService *iface, REFIID riid, void **obj)
{
    struct audio_renderer *renderer = impl_from_IMFGetService(iface);
    return IMFMediaSink_QueryInterface(&renderer->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI audio_renderer_get_service_AddRef(IMFGetService *iface)
{
    struct audio_renderer *renderer = impl_from_IMFGetService(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI audio_renderer_get_service_Release(IMFGetService *iface)
{
    struct audio_renderer *renderer = impl_from_IMFGetService(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI audio_renderer_get_service_GetService(IMFGetService *iface, REFGUID service, REFIID riid, void **obj)
{
    struct audio_renderer *renderer = impl_from_IMFGetService(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(service), debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualGUID(service, &MR_POLICY_VOLUME_SERVICE) && IsEqualIID(riid, &IID_IMFSimpleAudioVolume))
    {
        *obj = &renderer->IMFSimpleAudioVolume_iface;
    }
    else if (IsEqualGUID(service, &MR_STREAM_VOLUME_SERVICE) && IsEqualIID(riid, &IID_IMFAudioStreamVolume))
    {
        *obj = &renderer->IMFAudioStreamVolume_iface;
    }
    else if (IsEqualGUID(service, &MR_AUDIO_POLICY_SERVICE) && IsEqualIID(riid, &IID_IMFAudioPolicy))
    {
        *obj = &renderer->IMFAudioPolicy_iface;
    }
    else
        FIXME("Unsupported service %s, interface %s.\n", debugstr_guid(service), debugstr_guid(riid));

    if (*obj)
        IUnknown_AddRef((IUnknown *)*obj);

    return *obj ? S_OK : E_NOINTERFACE;
}

static const IMFGetServiceVtbl audio_renderer_get_service_vtbl =
{
    audio_renderer_get_service_QueryInterface,
    audio_renderer_get_service_AddRef,
    audio_renderer_get_service_Release,
    audio_renderer_get_service_GetService,
};

static HRESULT WINAPI audio_renderer_simple_volume_QueryInterface(IMFSimpleAudioVolume *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFSimpleAudioVolume) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFSimpleAudioVolume_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI audio_renderer_simple_volume_AddRef(IMFSimpleAudioVolume *iface)
{
    struct audio_renderer *renderer = impl_from_IMFSimpleAudioVolume(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI audio_renderer_simple_volume_Release(IMFSimpleAudioVolume *iface)
{
    struct audio_renderer *renderer = impl_from_IMFSimpleAudioVolume(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI audio_renderer_simple_volume_SetMasterVolume(IMFSimpleAudioVolume *iface, float level)
{
    FIXME("%p, %f.\n", iface, level);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_simple_volume_GetMasterVolume(IMFSimpleAudioVolume *iface, float *level)
{
    FIXME("%p, %p.\n", iface, level);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_simple_volume_SetMute(IMFSimpleAudioVolume *iface, BOOL mute)
{
    FIXME("%p, %d.\n", iface, mute);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_simple_volume_GetMute(IMFSimpleAudioVolume *iface, BOOL *mute)
{
    FIXME("%p, %p.\n", iface, mute);

    return E_NOTIMPL;
}

static const IMFSimpleAudioVolumeVtbl audio_renderer_simple_volume_vtbl =
{
    audio_renderer_simple_volume_QueryInterface,
    audio_renderer_simple_volume_AddRef,
    audio_renderer_simple_volume_Release,
    audio_renderer_simple_volume_SetMasterVolume,
    audio_renderer_simple_volume_GetMasterVolume,
    audio_renderer_simple_volume_SetMute,
    audio_renderer_simple_volume_GetMute,
};

static HRESULT WINAPI audio_renderer_stream_volume_QueryInterface(IMFAudioStreamVolume *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFAudioStreamVolume) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAudioStreamVolume_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI audio_renderer_stream_volume_AddRef(IMFAudioStreamVolume *iface)
{
    struct audio_renderer *renderer = impl_from_IMFAudioStreamVolume(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI audio_renderer_stream_volume_Release(IMFAudioStreamVolume *iface)
{
    struct audio_renderer *renderer = impl_from_IMFAudioStreamVolume(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI audio_renderer_stream_volume_GetChannelCount(IMFAudioStreamVolume *iface, UINT32 *count)
{
    FIXME("%p, %p.\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_stream_volume_SetChannelVolume(IMFAudioStreamVolume *iface, UINT32 index, float level)
{
    FIXME("%p, %u, %f.\n", iface, index, level);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_stream_volume_GetChannelVolume(IMFAudioStreamVolume *iface, UINT32 index, float *level)
{
    FIXME("%p, %u, %p.\n", iface, index, level);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_stream_volume_SetAllVolumes(IMFAudioStreamVolume *iface, UINT32 count,
        const float *volumes)
{
    FIXME("%p, %u, %p.\n", iface, count, volumes);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_stream_volume_GetAllVolumes(IMFAudioStreamVolume *iface, UINT32 count, float *volumes)
{
    FIXME("%p, %u, %p.\n", iface, count, volumes);

    return E_NOTIMPL;
}

static const IMFAudioStreamVolumeVtbl audio_renderer_stream_volume_vtbl =
{
    audio_renderer_stream_volume_QueryInterface,
    audio_renderer_stream_volume_AddRef,
    audio_renderer_stream_volume_Release,
    audio_renderer_stream_volume_GetChannelCount,
    audio_renderer_stream_volume_SetChannelVolume,
    audio_renderer_stream_volume_GetChannelVolume,
    audio_renderer_stream_volume_SetAllVolumes,
    audio_renderer_stream_volume_GetAllVolumes,
};

static HRESULT WINAPI audio_renderer_policy_QueryInterface(IMFAudioPolicy *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFAudioPolicy) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAudioPolicy_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI audio_renderer_policy_AddRef(IMFAudioPolicy *iface)
{
    struct audio_renderer *renderer = impl_from_IMFAudioPolicy(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI audio_renderer_policy_Release(IMFAudioPolicy *iface)
{
    struct audio_renderer *renderer = impl_from_IMFAudioPolicy(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI audio_renderer_policy_SetGroupingParam(IMFAudioPolicy *iface, REFGUID param)
{
    FIXME("%p, %s.\n", iface, debugstr_guid(param));

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_policy_GetGroupingParam(IMFAudioPolicy *iface, GUID *param)
{
    FIXME("%p, %p.\n", iface, param);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_policy_SetDisplayName(IMFAudioPolicy *iface, const WCHAR *name)
{
    FIXME("%p, %s.\n", iface, debugstr_w(name));

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_policy_GetDisplayName(IMFAudioPolicy *iface, WCHAR **name)
{
    FIXME("%p, %p.\n", iface, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_policy_SetIconPath(IMFAudioPolicy *iface, const WCHAR *path)
{
    FIXME("%p, %s.\n", iface, debugstr_w(path));

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_policy_GetIconPath(IMFAudioPolicy *iface, WCHAR **path)
{
    FIXME("%p, %p.\n", iface, path);

    return E_NOTIMPL;
}

static const IMFAudioPolicyVtbl audio_renderer_policy_vtbl =
{
    audio_renderer_policy_QueryInterface,
    audio_renderer_policy_AddRef,
    audio_renderer_policy_Release,
    audio_renderer_policy_SetGroupingParam,
    audio_renderer_policy_GetGroupingParam,
    audio_renderer_policy_SetDisplayName,
    audio_renderer_policy_GetDisplayName,
    audio_renderer_policy_SetIconPath,
    audio_renderer_policy_GetIconPath,
};

static HRESULT sar_create_mmdevice(IMFAttributes *attributes, IMMDevice **device)
{
    WCHAR *endpoint;
    unsigned int length, role = eMultimedia;
    IMMDeviceEnumerator *devenum;
    HRESULT hr;

    if (attributes)
    {
        /* Mutually exclusive attributes. */
        if (SUCCEEDED(IMFAttributes_GetItem(attributes, &MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ROLE, NULL)) &&
                SUCCEEDED(IMFAttributes_GetItem(attributes, &MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ID, NULL)))
        {
            return E_INVALIDARG;
        }
    }

    if (FAILED(hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator,
            (void **)&devenum)))
    {
        return hr;
    }

    role = eMultimedia;
    if (attributes && SUCCEEDED(IMFAttributes_GetUINT32(attributes, &MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ROLE, &role)))
        TRACE("Specified role %d.\n", role);

    if (attributes && SUCCEEDED(IMFAttributes_GetAllocatedString(attributes, &MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ID,
            &endpoint, &length)))
    {
        TRACE("Specified end point %s.\n", debugstr_w(endpoint));
        hr = IMMDeviceEnumerator_GetDevice(devenum, endpoint, device);
        CoTaskMemFree(endpoint);
    }
    else
        hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(devenum, eRender, role, device);

    if (FAILED(hr))
        hr = MF_E_NO_AUDIO_PLAYBACK_DEVICE;

    IMMDeviceEnumerator_Release(devenum);

    return hr;
}

static HRESULT WINAPI audio_renderer_stream_QueryInterface(IMFStreamSink *iface, REFIID riid, void **obj)
{
    struct audio_renderer_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFStreamSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &stream->IMFStreamSink_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaTypeHandler))
    {
        *obj = &stream->IMFMediaTypeHandler_iface;
    }
    else
    {
        WARN("Unsupported %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);

    return S_OK;
}

static ULONG WINAPI audio_renderer_stream_AddRef(IMFStreamSink *iface)
{
    struct audio_renderer_stream *stream = impl_from_IMFStreamSink(iface);
    ULONG refcount = InterlockedIncrement(&stream->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI audio_renderer_stream_Release(IMFStreamSink *iface)
{
    struct audio_renderer_stream *stream = impl_from_IMFStreamSink(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (stream->sink)
            IMFMediaSink_Release(&stream->sink->IMFMediaSink_iface);
        if (stream->event_queue)
        {
            IMFMediaEventQueue_Shutdown(stream->event_queue);
            IMFMediaEventQueue_Release(stream->event_queue);
        }
        DeleteCriticalSection(&stream->cs);
        heap_free(stream);
    }

    return refcount;
}

static HRESULT WINAPI audio_renderer_stream_GetEvent(IMFStreamSink *iface, DWORD flags, IMFMediaEvent **event)
{
    struct audio_renderer_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %#x, %p.\n", iface, flags, event);

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_GetEvent(stream->event_queue, flags, event);
}

static HRESULT WINAPI audio_renderer_stream_BeginGetEvent(IMFStreamSink *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct audio_renderer_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_BeginGetEvent(stream->event_queue, callback, state);
}

static HRESULT WINAPI audio_renderer_stream_EndGetEvent(IMFStreamSink *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct audio_renderer_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_EndGetEvent(stream->event_queue, result, event);
}

static HRESULT WINAPI audio_renderer_stream_QueueEvent(IMFStreamSink *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct audio_renderer_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %u, %s, %#x, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_QueueEventParamVar(stream->event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI audio_renderer_stream_GetMediaSink(IMFStreamSink *iface, IMFMediaSink **sink)
{
    struct audio_renderer_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, sink);

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    *sink = &stream->sink->IMFMediaSink_iface;
    IMFMediaSink_AddRef(*sink);

    return S_OK;
}

static HRESULT WINAPI audio_renderer_stream_GetIdentifier(IMFStreamSink *iface, DWORD *identifier)
{
    struct audio_renderer_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, identifier);

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    *identifier = 0;

    return S_OK;
}

static HRESULT WINAPI audio_renderer_stream_GetMediaTypeHandler(IMFStreamSink *iface, IMFMediaTypeHandler **handler)
{
    struct audio_renderer_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, handler);

    if (!handler)
        return E_POINTER;

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    *handler = &stream->IMFMediaTypeHandler_iface;
    IMFMediaTypeHandler_AddRef(*handler);

    return S_OK;
}

static HRESULT WINAPI audio_renderer_stream_ProcessSample(IMFStreamSink *iface, IMFSample *sample)
{
    FIXME("%p, %p.\n", iface, sample);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_stream_PlaceMarker(IMFStreamSink *iface, MFSTREAMSINK_MARKER_TYPE marker_type,
        const PROPVARIANT *marker_value, const PROPVARIANT *context_value)
{
    FIXME("%p, %d, %p, %p.\n", iface, marker_type, marker_value, context_value);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_stream_Flush(IMFStreamSink *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static const IMFStreamSinkVtbl audio_renderer_stream_vtbl =
{
    audio_renderer_stream_QueryInterface,
    audio_renderer_stream_AddRef,
    audio_renderer_stream_Release,
    audio_renderer_stream_GetEvent,
    audio_renderer_stream_BeginGetEvent,
    audio_renderer_stream_EndGetEvent,
    audio_renderer_stream_QueueEvent,
    audio_renderer_stream_GetMediaSink,
    audio_renderer_stream_GetIdentifier,
    audio_renderer_stream_GetMediaTypeHandler,
    audio_renderer_stream_ProcessSample,
    audio_renderer_stream_PlaceMarker,
    audio_renderer_stream_Flush,
};

static HRESULT WINAPI audio_renderer_stream_type_handler_QueryInterface(IMFMediaTypeHandler *iface, REFIID riid,
        void **obj)
{
    struct audio_renderer_stream *stream = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_QueryInterface(&stream->IMFStreamSink_iface, riid, obj);
}

static ULONG WINAPI audio_renderer_stream_type_handler_AddRef(IMFMediaTypeHandler *iface)
{
    struct audio_renderer_stream *stream = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_AddRef(&stream->IMFStreamSink_iface);
}

static ULONG WINAPI audio_renderer_stream_type_handler_Release(IMFMediaTypeHandler *iface)
{
    struct audio_renderer_stream *stream = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_Release(&stream->IMFStreamSink_iface);
}

static HRESULT WINAPI audio_renderer_stream_type_handler_IsMediaTypeSupported(IMFMediaTypeHandler *iface,
        IMFMediaType *in_type, IMFMediaType **out_type)
{
    FIXME("%p, %p, %p.\n", iface, in_type, out_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_stream_type_handler_GetMediaTypeCount(IMFMediaTypeHandler *iface, DWORD *count)
{
    FIXME("%p, %p.\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_stream_type_handler_GetMediaTypeByIndex(IMFMediaTypeHandler *iface, DWORD index,
        IMFMediaType **media_type)
{
    FIXME("%p, %u, %p.\n", iface, index, media_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_stream_type_handler_SetCurrentMediaType(IMFMediaTypeHandler *iface,
        IMFMediaType *media_type)
{
    FIXME("%p, %p.\n", iface, media_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_stream_type_handler_GetCurrentMediaType(IMFMediaTypeHandler *iface,
        IMFMediaType **media_type)
{
    FIXME("%p, %p.\n", iface, media_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_renderer_stream_type_handler_GetMajorType(IMFMediaTypeHandler *iface, GUID *type)
{
    struct audio_renderer_stream *stream = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %p.\n", iface, type);

    if (!type)
        return E_POINTER;

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    memcpy(type, &MFMediaType_Audio, sizeof(*type));
    return S_OK;
}

static const IMFMediaTypeHandlerVtbl audio_renderer_stream_type_handler_vtbl =
{
    audio_renderer_stream_type_handler_QueryInterface,
    audio_renderer_stream_type_handler_AddRef,
    audio_renderer_stream_type_handler_Release,
    audio_renderer_stream_type_handler_IsMediaTypeSupported,
    audio_renderer_stream_type_handler_GetMediaTypeCount,
    audio_renderer_stream_type_handler_GetMediaTypeByIndex,
    audio_renderer_stream_type_handler_SetCurrentMediaType,
    audio_renderer_stream_type_handler_GetCurrentMediaType,
    audio_renderer_stream_type_handler_GetMajorType,
};

static HRESULT audio_renderer_create_stream(struct audio_renderer *sink, struct audio_renderer_stream **stream)
{
    struct audio_renderer_stream *object;
    HRESULT hr;

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFStreamSink_iface.lpVtbl = &audio_renderer_stream_vtbl;
    object->IMFMediaTypeHandler_iface.lpVtbl = &audio_renderer_stream_type_handler_vtbl;
    object->refcount = 1;
    object->sink = sink;
    IMFMediaSink_AddRef(&object->sink->IMFMediaSink_iface);
    InitializeCriticalSection(&object->cs);

    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
        goto failed;

    *stream = object;

    return S_OK;

failed:
    IMFStreamSink_Release(&object->IMFStreamSink_iface);

    return hr;
}

static HRESULT sar_create_object(IMFAttributes *attributes, void *user_context, IUnknown **obj)
{
    struct audio_renderer *renderer;
    IMMDevice *device;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", attributes, user_context, obj);

    if (!(renderer = heap_alloc_zero(sizeof(*renderer))))
        return E_OUTOFMEMORY;

    renderer->IMFMediaSink_iface.lpVtbl = &audio_renderer_sink_vtbl;
    renderer->IMFMediaSinkPreroll_iface.lpVtbl = &audio_renderer_preroll_vtbl;
    renderer->IMFClockStateSink_iface.lpVtbl = &audio_renderer_clock_sink_vtbl;
    renderer->IMFMediaEventGenerator_iface.lpVtbl = &audio_renderer_events_vtbl;
    renderer->IMFGetService_iface.lpVtbl = &audio_renderer_get_service_vtbl;
    renderer->IMFSimpleAudioVolume_iface.lpVtbl = &audio_renderer_simple_volume_vtbl;
    renderer->IMFAudioStreamVolume_iface.lpVtbl = &audio_renderer_stream_volume_vtbl;
    renderer->IMFAudioPolicy_iface.lpVtbl = &audio_renderer_policy_vtbl;
    renderer->refcount = 1;
    InitializeCriticalSection(&renderer->cs);

    if (FAILED(hr = audio_renderer_create_stream(renderer, &renderer->stream)))
        goto failed;

    if (FAILED(hr = MFCreateEventQueue(&renderer->event_queue)))
        goto failed;

    if (FAILED(hr = sar_create_mmdevice(attributes, &device)))
        goto failed;

    IMMDevice_Release(device);

    *obj = (IUnknown *)&renderer->IMFMediaSink_iface;

    return S_OK;

failed:

    IMFMediaSink_Release(&renderer->IMFMediaSink_iface);

    return hr;
}

static void sar_shutdown_object(void *user_context, IUnknown *obj)
{
    /* FIXME: shut down sink */
}

static void sar_free_private(void *user_context)
{
}

static const struct activate_funcs sar_activate_funcs =
{
    sar_create_object,
    sar_shutdown_object,
    sar_free_private,
};

/***********************************************************************
 *      MFCreateAudioRendererActivate (mf.@)
 */
HRESULT WINAPI MFCreateAudioRendererActivate(IMFActivate **activate)
{
    TRACE("%p.\n", activate);

    if (!activate)
        return E_POINTER;

    return create_activation_object(NULL, &sar_activate_funcs, activate);
}

/***********************************************************************
 *      MFCreateAudioRenderer (mf.@)
 */
HRESULT WINAPI MFCreateAudioRenderer(IMFAttributes *attributes, IMFMediaSink **sink)
{
    IUnknown *object;
    HRESULT hr;

    TRACE("%p, %p.\n", attributes, sink);

    if (SUCCEEDED(hr = sar_create_object(attributes, NULL, &object)))
    {
        hr = IUnknown_QueryInterface(object, &IID_IMFMediaSink, (void **)sink);
        IUnknown_Release(object);
    }

    return hr;
}
