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
#include "mf_private.h"
#include "initguid.h"
#include "mmdeviceapi.h"
#include "audioclient.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

enum stream_state
{
    STREAM_STATE_STOPPED = 0,
    STREAM_STATE_RUNNING,
    STREAM_STATE_PAUSED,
};

enum audio_renderer_flags
{
    SAR_SHUT_DOWN = 0x1,
    SAR_PREROLLED = 0x2,
    SAR_SAMPLE_REQUESTED = 0x4,
};

enum queued_object_type
{
    OBJECT_TYPE_SAMPLE,
    OBJECT_TYPE_MARKER,
};

struct queued_object
{
    struct list entry;
    enum queued_object_type type;
    union
    {
        struct
        {
            IMFSample *sample;
            unsigned int frame_offset;
        } sample;
        struct
        {
            MFSTREAMSINK_MARKER_TYPE type;
            PROPVARIANT context;
        } marker;
    } u;
};

struct audio_renderer
{
    IMFMediaSink IMFMediaSink_iface;
    IMFMediaSinkPreroll IMFMediaSinkPreroll_iface;
    IMFStreamSink IMFStreamSink_iface;
    IMFMediaTypeHandler IMFMediaTypeHandler_iface;
    IMFClockStateSink IMFClockStateSink_iface;
    IMFMediaEventGenerator IMFMediaEventGenerator_iface;
    IMFGetService IMFGetService_iface;
    IMFSimpleAudioVolume IMFSimpleAudioVolume_iface;
    IMFAudioStreamVolume IMFAudioStreamVolume_iface;
    IMFAudioPolicy IMFAudioPolicy_iface;
    IMFAsyncCallback render_callback;
    LONG refcount;
    IMFMediaEventQueue *event_queue;
    IMFMediaEventQueue *stream_event_queue;
    IMFPresentationClock *clock;
    IMFMediaType *media_type;
    IMFMediaType *current_media_type;
    IMMDevice *device;
    IAudioClient *audio_client;
    IAudioRenderClient *audio_render_client;
    IAudioStreamVolume *stream_volume;
    ISimpleAudioVolume *audio_volume;
    struct
    {
        unsigned int flags;
        GUID session_id;
    } stream_config;
    HANDLE buffer_ready_event;
    MFWORKITEM_KEY buffer_ready_key;
    unsigned int frame_size;
    unsigned int queued_frames;
    unsigned int max_frames;
    struct list queue;
    enum stream_state state;
    unsigned int flags;
    CRITICAL_SECTION cs;
};

static void release_pending_object(struct queued_object *object)
{
    list_remove(&object->entry);
    switch (object->type)
    {
        case OBJECT_TYPE_SAMPLE:
            if (object->u.sample.sample)
                IMFSample_Release(object->u.sample.sample);
            break;
        case OBJECT_TYPE_MARKER:
            PropVariantClear(&object->u.marker.context);
            break;
    }
    free(object);
}

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

static struct audio_renderer *impl_from_IMFStreamSink(IMFStreamSink *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer, IMFStreamSink_iface);
}

static struct audio_renderer *impl_from_IMFMediaTypeHandler(IMFMediaTypeHandler *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer, IMFMediaTypeHandler_iface);
}

static struct audio_renderer *impl_from_render_callback_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct audio_renderer, render_callback);
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
    TRACE("%p, refcount %lu.\n", iface, refcount);
    return refcount;
}

static void audio_renderer_release_audio_client(struct audio_renderer *renderer)
{
    struct queued_object *obj, *obj2;

    MFCancelWorkItem(renderer->buffer_ready_key);
    LIST_FOR_EACH_ENTRY_SAFE(obj, obj2, &renderer->queue, struct queued_object, entry)
    {
        release_pending_object(obj);
    }
    renderer->queued_frames = 0;
    renderer->buffer_ready_key = 0;
    if (renderer->audio_client)
    {
        IAudioClient_Stop(renderer->audio_client);
        IAudioClient_Reset(renderer->audio_client);
        IAudioClient_Release(renderer->audio_client);
    }
    renderer->audio_client = NULL;
    if (renderer->audio_render_client)
        IAudioRenderClient_Release(renderer->audio_render_client);
    renderer->audio_render_client = NULL;
    if (renderer->stream_volume)
        IAudioStreamVolume_Release(renderer->stream_volume);
    renderer->stream_volume = NULL;
    if (renderer->audio_volume)
        ISimpleAudioVolume_Release(renderer->audio_volume);
    renderer->audio_volume = NULL;
    renderer->flags &= ~SAR_PREROLLED;
}

static ULONG WINAPI audio_renderer_sink_Release(IMFMediaSink *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);
    ULONG refcount = InterlockedDecrement(&renderer->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (renderer->event_queue)
            IMFMediaEventQueue_Release(renderer->event_queue);
        if (renderer->stream_event_queue)
            IMFMediaEventQueue_Release(renderer->stream_event_queue);
        if (renderer->clock)
            IMFPresentationClock_Release(renderer->clock);
        if (renderer->device)
            IMMDevice_Release(renderer->device);
        if (renderer->media_type)
            IMFMediaType_Release(renderer->media_type);
        if (renderer->current_media_type)
            IMFMediaType_Release(renderer->current_media_type);
        audio_renderer_release_audio_client(renderer);
        CloseHandle(renderer->buffer_ready_event);
        DeleteCriticalSection(&renderer->cs);
        free(renderer);
    }

    return refcount;
}

static HRESULT WINAPI audio_renderer_sink_GetCharacteristics(IMFMediaSink *iface, DWORD *flags)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p, %p.\n", iface, flags);

    if (renderer->flags & SAR_SHUT_DOWN)
        return MF_E_SHUTDOWN;

    *flags = MEDIASINK_FIXED_STREAMS | MEDIASINK_CAN_PREROLL;

    return S_OK;
}

static HRESULT WINAPI audio_renderer_sink_AddStreamSink(IMFMediaSink *iface, DWORD stream_sink_id,
    IMFMediaType *media_type, IMFStreamSink **stream_sink)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p, %#lx, %p, %p.\n", iface, stream_sink_id, media_type, stream_sink);

    return renderer->flags & SAR_SHUT_DOWN ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI audio_renderer_sink_RemoveStreamSink(IMFMediaSink *iface, DWORD stream_sink_id)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p, %#lx.\n", iface, stream_sink_id);

    return renderer->flags & SAR_SHUT_DOWN ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI audio_renderer_sink_GetStreamSinkCount(IMFMediaSink *iface, DWORD *count)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_POINTER;

    if (renderer->flags & SAR_SHUT_DOWN)
        return MF_E_SHUTDOWN;

    *count = 1;

    return S_OK;
}

static HRESULT WINAPI audio_renderer_sink_GetStreamSinkByIndex(IMFMediaSink *iface, DWORD index,
        IMFStreamSink **stream)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %lu, %p.\n", iface, index, stream);

    EnterCriticalSection(&renderer->cs);

    if (renderer->flags & SAR_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (index > 0)
        hr = MF_E_INVALIDINDEX;
    else
    {
       *stream = &renderer->IMFStreamSink_iface;
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

    TRACE("%p, %#lx, %p.\n", iface, stream_sink_id, stream);

    EnterCriticalSection(&renderer->cs);

    if (renderer->flags & SAR_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (stream_sink_id > 0)
        hr = MF_E_INVALIDSTREAMNUMBER;
    else
    {
        *stream = &renderer->IMFStreamSink_iface;
        IMFStreamSink_AddRef(*stream);
    }

    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static void audio_renderer_set_presentation_clock(struct audio_renderer *renderer, IMFPresentationClock *clock)
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

static HRESULT WINAPI audio_renderer_sink_SetPresentationClock(IMFMediaSink *iface, IMFPresentationClock *clock)
{
    struct audio_renderer *renderer = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, clock);

    EnterCriticalSection(&renderer->cs);

    if (renderer->flags & SAR_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        audio_renderer_set_presentation_clock(renderer, clock);

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

    if (renderer->flags & SAR_SHUT_DOWN)
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
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&renderer->cs);

    if (renderer->flags & SAR_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        renderer->flags |= SAR_SHUT_DOWN;
        IMFMediaEventQueue_Shutdown(renderer->event_queue);
        IMFMediaEventQueue_Shutdown(renderer->stream_event_queue);
        audio_renderer_set_presentation_clock(renderer, NULL);
        audio_renderer_release_audio_client(renderer);
    }

    LeaveCriticalSection(&renderer->cs);

    return hr;
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

static void audio_renderer_preroll(struct audio_renderer *renderer)
{
    unsigned int i;

    if (renderer->flags & SAR_PREROLLED)
        return;

    for (i = 0; i < 2; ++i)
        IMFMediaEventQueue_QueueEventParamVar(renderer->stream_event_queue, MEStreamSinkRequestSample, &GUID_NULL, S_OK, NULL);
    renderer->flags |= SAR_PREROLLED;
}

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
    struct audio_renderer *renderer = impl_from_IMFMediaSinkPreroll(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(start_time));

    if (renderer->flags & SAR_SHUT_DOWN)
        return MF_E_SHUTDOWN;

    audio_renderer_preroll(renderer);
    return IMFMediaEventQueue_QueueEventParamVar(renderer->stream_event_queue, MEStreamSinkPrerolled, &GUID_NULL, S_OK, NULL);
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

    TRACE("%p, %#lx, %p.\n", iface, flags, event);

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

    TRACE("%p, %lu, %s, %#lx, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

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
    struct audio_renderer *renderer = impl_from_IMFClockStateSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %s, %s.\n", iface, debugstr_time(systime), debugstr_time(offset));

    EnterCriticalSection(&renderer->cs);
    if (renderer->audio_client)
    {
        if (renderer->state != STREAM_STATE_RUNNING)
        {
            if (FAILED(hr = IAudioClient_Start(renderer->audio_client)))
                WARN("Failed to start audio client, hr %#lx.\n", hr);
            renderer->state = STREAM_STATE_RUNNING;
        }
    }
    else
        hr = MF_E_NOT_INITIALIZED;

    IMFMediaEventQueue_QueueEventParamVar(renderer->stream_event_queue, MEStreamSinkStarted, &GUID_NULL, hr, NULL);
    if (SUCCEEDED(hr))
        audio_renderer_preroll(renderer);
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_clock_sink_OnClockStop(IMFClockStateSink *iface, MFTIME systime)
{
    struct audio_renderer *renderer = impl_from_IMFClockStateSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    EnterCriticalSection(&renderer->cs);
    if (renderer->audio_client)
    {
        if (renderer->state != STREAM_STATE_STOPPED)
        {
            if (SUCCEEDED(hr = IAudioClient_Stop(renderer->audio_client)))
            {
                if (FAILED(hr = IAudioClient_Reset(renderer->audio_client)))
                    WARN("Failed to reset audio client, hr %#lx.\n", hr);
            }
            else
                WARN("Failed to stop audio client, hr %#lx.\n", hr);
            renderer->state = STREAM_STATE_STOPPED;
            renderer->flags &= ~SAR_PREROLLED;
        }
    }
    else
        hr = MF_E_NOT_INITIALIZED;

    IMFMediaEventQueue_QueueEventParamVar(renderer->stream_event_queue, MEStreamSinkStopped, &GUID_NULL, hr, NULL);
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_clock_sink_OnClockPause(IMFClockStateSink *iface, MFTIME systime)
{
    struct audio_renderer *renderer = impl_from_IMFClockStateSink(iface);
    HRESULT hr;

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    EnterCriticalSection(&renderer->cs);
    if (renderer->state == STREAM_STATE_RUNNING)
    {
        if (renderer->audio_client)
        {
            if (FAILED(hr = IAudioClient_Stop(renderer->audio_client)))
                WARN("Failed to stop audio client, hr %#lx.\n", hr);
            renderer->state = STREAM_STATE_PAUSED;
        }
        else
            hr = MF_E_NOT_INITIALIZED;

        IMFMediaEventQueue_QueueEventParamVar(renderer->stream_event_queue, MEStreamSinkPaused, &GUID_NULL, hr, NULL);
    }
    else
        hr = MF_E_INVALID_STATE_TRANSITION;
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_clock_sink_OnClockRestart(IMFClockStateSink *iface, MFTIME systime)
{
    struct audio_renderer *renderer = impl_from_IMFClockStateSink(iface);
    BOOL preroll = FALSE;
    HRESULT hr = S_OK;

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    EnterCriticalSection(&renderer->cs);
    if (renderer->audio_client)
    {
        if ((preroll = (renderer->state != STREAM_STATE_RUNNING)))
        {
            if (FAILED(hr = IAudioClient_Start(renderer->audio_client)))
                WARN("Failed to start audio client, hr %#lx.\n", hr);
            renderer->state = STREAM_STATE_RUNNING;
        }
    }
    else
        hr = MF_E_NOT_INITIALIZED;

    IMFMediaEventQueue_QueueEventParamVar(renderer->stream_event_queue, MEStreamSinkStarted, &GUID_NULL, hr, NULL);
    if (preroll)
        audio_renderer_preroll(renderer);

    LeaveCriticalSection(&renderer->cs);

    return hr;
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
    struct audio_renderer *renderer = impl_from_IMFSimpleAudioVolume(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %f.\n", iface, level);

    EnterCriticalSection(&renderer->cs);
    if (renderer->audio_volume)
        hr = ISimpleAudioVolume_SetMasterVolume(renderer->audio_volume, level, NULL);
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_simple_volume_GetMasterVolume(IMFSimpleAudioVolume *iface, float *level)
{
    struct audio_renderer *renderer = impl_from_IMFSimpleAudioVolume(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, level);

    if (!level)
        return E_POINTER;

    *level = 0.0f;

    EnterCriticalSection(&renderer->cs);
    if (renderer->audio_volume)
        hr = ISimpleAudioVolume_GetMasterVolume(renderer->audio_volume, level);
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_simple_volume_SetMute(IMFSimpleAudioVolume *iface, BOOL mute)
{
    struct audio_renderer *renderer = impl_from_IMFSimpleAudioVolume(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %d.\n", iface, mute);

    EnterCriticalSection(&renderer->cs);
    if (renderer->audio_volume)
        hr = ISimpleAudioVolume_SetMute(renderer->audio_volume, mute, NULL);
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_simple_volume_GetMute(IMFSimpleAudioVolume *iface, BOOL *mute)
{
    struct audio_renderer *renderer = impl_from_IMFSimpleAudioVolume(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, mute);

    if (!mute)
        return E_POINTER;

    *mute = FALSE;

    EnterCriticalSection(&renderer->cs);
    if (renderer->audio_volume)
        hr = ISimpleAudioVolume_GetMute(renderer->audio_volume, mute);
    LeaveCriticalSection(&renderer->cs);

    return hr;
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
    struct audio_renderer *renderer = impl_from_IMFAudioStreamVolume(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_POINTER;

    *count = 0;

    EnterCriticalSection(&renderer->cs);
    if (renderer->stream_volume)
        hr = IAudioStreamVolume_GetChannelCount(renderer->stream_volume, count);
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_stream_volume_SetChannelVolume(IMFAudioStreamVolume *iface, UINT32 index, float level)
{
    struct audio_renderer *renderer = impl_from_IMFAudioStreamVolume(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %f.\n", iface, index, level);

    EnterCriticalSection(&renderer->cs);
    if (renderer->stream_volume)
        hr = IAudioStreamVolume_SetChannelVolume(renderer->stream_volume, index, level);
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_stream_volume_GetChannelVolume(IMFAudioStreamVolume *iface, UINT32 index, float *level)
{
    struct audio_renderer *renderer = impl_from_IMFAudioStreamVolume(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p.\n", iface, index, level);

    if (!level)
        return E_POINTER;

    *level = 0.0f;

    EnterCriticalSection(&renderer->cs);
    if (renderer->stream_volume)
        hr = IAudioStreamVolume_GetChannelVolume(renderer->stream_volume, index, level);
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_stream_volume_SetAllVolumes(IMFAudioStreamVolume *iface, UINT32 count,
        const float *volumes)
{
    struct audio_renderer *renderer = impl_from_IMFAudioStreamVolume(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p.\n", iface, count, volumes);

    EnterCriticalSection(&renderer->cs);
    if (renderer->stream_volume)
        hr = IAudioStreamVolume_SetAllVolumes(renderer->stream_volume, count, volumes);
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_stream_volume_GetAllVolumes(IMFAudioStreamVolume *iface, UINT32 count, float *volumes)
{
    struct audio_renderer *renderer = impl_from_IMFAudioStreamVolume(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p.\n", iface, count, volumes);

    if (!volumes)
        return E_POINTER;

    if (count)
        memset(volumes, 0, sizeof(*volumes) * count);

    EnterCriticalSection(&renderer->cs);
    if (renderer->stream_volume)
        hr = IAudioStreamVolume_GetAllVolumes(renderer->stream_volume, count, volumes);
    LeaveCriticalSection(&renderer->cs);

    return hr;
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

static HRESULT sar_create_mmdevice(IMFAttributes *attributes, struct audio_renderer *renderer)
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
        hr = IMMDeviceEnumerator_GetDevice(devenum, endpoint, &renderer->device);
        CoTaskMemFree(endpoint);
    }
    else
        hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(devenum, eRender, role, &renderer->device);

    /* Configuration attributes to be used later for audio client initialization. */
    if (attributes)
    {
        IMFAttributes_GetUINT32(attributes, &MF_AUDIO_RENDERER_ATTRIBUTE_FLAGS, &renderer->stream_config.flags);
        IMFAttributes_GetGUID(attributes, &MF_AUDIO_RENDERER_ATTRIBUTE_SESSION_ID, &renderer->stream_config.session_id);
    }

    if (FAILED(hr))
        hr = MF_E_NO_AUDIO_PLAYBACK_DEVICE;

    IMMDeviceEnumerator_Release(devenum);

    return hr;
}

static HRESULT WINAPI audio_renderer_stream_QueryInterface(IMFStreamSink *iface, REFIID riid, void **obj)
{
    struct audio_renderer *renderer = impl_from_IMFStreamSink(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFStreamSink) ||
            IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &renderer->IMFStreamSink_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaTypeHandler))
    {
        *obj = &renderer->IMFMediaTypeHandler_iface;
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
    struct audio_renderer *renderer = impl_from_IMFStreamSink(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI audio_renderer_stream_Release(IMFStreamSink *iface)
{
    struct audio_renderer *renderer = impl_from_IMFStreamSink(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI audio_renderer_stream_GetEvent(IMFStreamSink *iface, DWORD flags, IMFMediaEvent **event)
{
    struct audio_renderer *renderer = impl_from_IMFStreamSink(iface);

    TRACE("%p, %#lx, %p.\n", iface, flags, event);

    if (renderer->flags & SAR_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_GetEvent(renderer->stream_event_queue, flags, event);
}

static HRESULT WINAPI audio_renderer_stream_BeginGetEvent(IMFStreamSink *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct audio_renderer *renderer = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    if (renderer->flags & SAR_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_BeginGetEvent(renderer->stream_event_queue, callback, state);
}

static HRESULT WINAPI audio_renderer_stream_EndGetEvent(IMFStreamSink *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct audio_renderer *renderer = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    if (renderer->flags & SAR_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_EndGetEvent(renderer->stream_event_queue, result, event);
}

static HRESULT WINAPI audio_renderer_stream_QueueEvent(IMFStreamSink *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct audio_renderer *renderer = impl_from_IMFStreamSink(iface);

    TRACE("%p, %lu, %s, %#lx, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    if (renderer->flags & SAR_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_QueueEventParamVar(renderer->stream_event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI audio_renderer_stream_GetMediaSink(IMFStreamSink *iface, IMFMediaSink **sink)
{
    struct audio_renderer *renderer = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, sink);

    if (renderer->flags & SAR_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    *sink = &renderer->IMFMediaSink_iface;
    IMFMediaSink_AddRef(*sink);

    return S_OK;
}

static HRESULT WINAPI audio_renderer_stream_GetIdentifier(IMFStreamSink *iface, DWORD *identifier)
{
    struct audio_renderer *renderer = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, identifier);

    if (renderer->flags & SAR_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    *identifier = 0;

    return S_OK;
}

static HRESULT WINAPI audio_renderer_stream_GetMediaTypeHandler(IMFStreamSink *iface, IMFMediaTypeHandler **handler)
{
    struct audio_renderer *renderer = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, handler);

    if (!handler)
        return E_POINTER;

    if (renderer->flags & SAR_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    *handler = &renderer->IMFMediaTypeHandler_iface;
    IMFMediaTypeHandler_AddRef(*handler);

    return S_OK;
}

static HRESULT stream_queue_sample(struct audio_renderer *renderer, IMFSample *sample)
{
    struct queued_object *object;
    DWORD sample_len, sample_frames;
    HRESULT hr;

    if (FAILED(hr = IMFSample_GetTotalLength(sample, &sample_len)))
        return hr;

    sample_frames = sample_len / renderer->frame_size;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->type = OBJECT_TYPE_SAMPLE;
    object->u.sample.sample = sample;
    IMFSample_AddRef(object->u.sample.sample);

    list_add_tail(&renderer->queue, &object->entry);
    renderer->queued_frames += sample_frames;

    return S_OK;
}

static HRESULT WINAPI audio_renderer_stream_ProcessSample(IMFStreamSink *iface, IMFSample *sample)
{
    struct audio_renderer *renderer = impl_from_IMFStreamSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, sample);

    if (!sample)
        return E_POINTER;

    EnterCriticalSection(&renderer->cs);

    if (renderer->flags & SAR_SHUT_DOWN)
        hr = MF_E_STREAMSINK_REMOVED;
    else
    {
        if (renderer->state == STREAM_STATE_RUNNING)
            hr = stream_queue_sample(renderer, sample);
        renderer->flags &= ~SAR_SAMPLE_REQUESTED;

        if (renderer->queued_frames < renderer->max_frames && renderer->state == STREAM_STATE_RUNNING)
        {
            IMFMediaEventQueue_QueueEventParamVar(renderer->stream_event_queue, MEStreamSinkRequestSample,
                    &GUID_NULL, S_OK, NULL);
            renderer->flags |= SAR_SAMPLE_REQUESTED;
        }
    }

    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT stream_place_marker(struct audio_renderer *renderer, MFSTREAMSINK_MARKER_TYPE marker_type,
        const PROPVARIANT *context_value)
{
    struct queued_object *marker;
    HRESULT hr = S_OK;

    if (!(marker = calloc(1, sizeof(*marker))))
        return E_OUTOFMEMORY;

    marker->type = OBJECT_TYPE_MARKER;
    marker->u.marker.type = marker_type;
    PropVariantInit(&marker->u.marker.context);
    if (context_value)
        hr = PropVariantCopy(&marker->u.marker.context, context_value);
    if (SUCCEEDED(hr))
        list_add_tail(&renderer->queue, &marker->entry);
    else
        release_pending_object(marker);

    return hr;
}

static HRESULT WINAPI audio_renderer_stream_PlaceMarker(IMFStreamSink *iface, MFSTREAMSINK_MARKER_TYPE marker_type,
        const PROPVARIANT *marker_value, const PROPVARIANT *context_value)
{
    struct audio_renderer *renderer = impl_from_IMFStreamSink(iface);
    HRESULT hr;

    TRACE("%p, %d, %p, %p.\n", iface, marker_type, marker_value, context_value);

    EnterCriticalSection(&renderer->cs);

    if (renderer->flags & SAR_SHUT_DOWN)
        hr = MF_E_STREAMSINK_REMOVED;
    else
        hr = stream_place_marker(renderer, marker_type, context_value);

    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_stream_Flush(IMFStreamSink *iface)
{
    struct audio_renderer *renderer = impl_from_IMFStreamSink(iface);
    struct queued_object *obj, *obj2;
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&renderer->cs);
    if (renderer->flags & SAR_SHUT_DOWN)
        hr = MF_E_STREAMSINK_REMOVED;
    else
    {
        LIST_FOR_EACH_ENTRY_SAFE(obj, obj2, &renderer->queue, struct queued_object, entry)
        {
            if (obj->type == OBJECT_TYPE_MARKER)
            {
                IMFMediaEventQueue_QueueEventParamVar(renderer->stream_event_queue, MEStreamSinkMarker,
                        &GUID_NULL, S_OK, &obj->u.marker.context);
            }
            release_pending_object(obj);
        }
    }
    renderer->queued_frames = 0;
    LeaveCriticalSection(&renderer->cs);

    return hr;
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
    struct audio_renderer *renderer = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_QueryInterface(&renderer->IMFStreamSink_iface, riid, obj);
}

static ULONG WINAPI audio_renderer_stream_type_handler_AddRef(IMFMediaTypeHandler *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_AddRef(&renderer->IMFStreamSink_iface);
}

static ULONG WINAPI audio_renderer_stream_type_handler_Release(IMFMediaTypeHandler *iface)
{
    struct audio_renderer *renderer = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_Release(&renderer->IMFStreamSink_iface);
}

static HRESULT check_media_type(IMFMediaType *type, IMFMediaType *current)
{
    static const GUID *required_attrs[] =
    {
        &MF_MT_AUDIO_SAMPLES_PER_SECOND,
        &MF_MT_AUDIO_NUM_CHANNELS,
        &MF_MT_AUDIO_BITS_PER_SAMPLE,
        &MF_MT_AUDIO_BLOCK_ALIGNMENT,
        &MF_MT_AUDIO_AVG_BYTES_PER_SECOND,
    };
    PROPVARIANT value;
    BOOL result;
    HRESULT hr;
    GUID major;
    int i;

    if (FAILED(hr = IMFMediaType_GetGUID(type, &MF_MT_MAJOR_TYPE, &major)))
        return hr;
    if (!IsEqualGUID(&major, &MFMediaType_Audio))
        return MF_E_INVALIDMEDIATYPE;

    for (i = 0; SUCCEEDED(hr) && i < ARRAY_SIZE(required_attrs); ++i)
    {
        PropVariantInit(&value);
        hr = IMFMediaType_GetItem(type, required_attrs[i], &value);
        if (SUCCEEDED(hr))
            hr = IMFMediaType_CompareItem(current, required_attrs[i], &value, &result);
        if (SUCCEEDED(hr) && !result)
            hr = MF_E_INVALIDMEDIATYPE;
        PropVariantClear(&value);
    }

    if (FAILED(hr))
        return MF_E_INVALIDMEDIATYPE;

    return S_OK;
}

static HRESULT WINAPI audio_renderer_stream_type_handler_IsMediaTypeSupported(IMFMediaTypeHandler *iface,
        IMFMediaType *in_type, IMFMediaType **out_type)
{
    struct audio_renderer *renderer = impl_from_IMFMediaTypeHandler(iface);
    HRESULT hr;

    TRACE("%p, %p, %p.\n", iface, in_type, out_type);

    EnterCriticalSection(&renderer->cs);
    hr = check_media_type(in_type, renderer->media_type);
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_stream_type_handler_GetMediaTypeCount(IMFMediaTypeHandler *iface, DWORD *count)
{
    TRACE("%p, %p.\n", iface, count);

    *count = 1;

    return S_OK;
}

static HRESULT WINAPI audio_renderer_stream_type_handler_GetMediaTypeByIndex(IMFMediaTypeHandler *iface, DWORD index,
        IMFMediaType **media_type)
{
    struct audio_renderer *renderer = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %lu, %p.\n", iface, index, media_type);

    if (index > 0)
        return MF_E_NO_MORE_TYPES;

    *media_type = renderer->media_type;
    IMFMediaType_AddRef(*media_type);
    return S_OK;
}

static HRESULT audio_renderer_create_audio_client(struct audio_renderer *renderer)
{
    IMFAsyncResult *result;
    unsigned int flags;
    WAVEFORMATEX *wfx;
    HRESULT hr;

    audio_renderer_release_audio_client(renderer);

    hr = IMMDevice_Activate(renderer->device, &IID_IAudioClient, CLSCTX_INPROC_SERVER, NULL,
            (void **)&renderer->audio_client);
    if (FAILED(hr))
    {
        WARN("Failed to create audio client, hr %#lx.\n", hr);
        return hr;
    }

    /* FIXME: for now always use default format. */
    if (FAILED(hr = IAudioClient_GetMixFormat(renderer->audio_client, &wfx)))
    {
        WARN("Failed to get audio format, hr %#lx.\n", hr);
        return hr;
    }

    renderer->frame_size = wfx->wBitsPerSample * wfx->nChannels / 8;

    flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
    if (renderer->stream_config.flags & MF_AUDIO_RENDERER_ATTRIBUTE_FLAGS_CROSSPROCESS)
        flags |= AUDCLNT_STREAMFLAGS_CROSSPROCESS;
    if (renderer->stream_config.flags & MF_AUDIO_RENDERER_ATTRIBUTE_FLAGS_NOPERSIST)
        flags |= AUDCLNT_STREAMFLAGS_NOPERSIST;
    hr = IAudioClient_Initialize(renderer->audio_client, AUDCLNT_SHAREMODE_SHARED, flags, 1000000, 0, wfx,
            &renderer->stream_config.session_id);
    CoTaskMemFree(wfx);
    if (FAILED(hr))
    {
        WARN("Failed to initialize audio client, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = IAudioClient_GetService(renderer->audio_client, &IID_IAudioStreamVolume,
            (void **)&renderer->stream_volume)))
    {
        WARN("Failed to get stream volume control, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = IAudioClient_GetService(renderer->audio_client, &IID_ISimpleAudioVolume, (void **)&renderer->audio_volume)))
    {
        WARN("Failed to get audio volume control, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = IAudioClient_GetService(renderer->audio_client, &IID_IAudioRenderClient,
            (void **)&renderer->audio_render_client)))
    {
        WARN("Failed to get audio render client, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = IAudioClient_SetEventHandle(renderer->audio_client, renderer->buffer_ready_event)))
    {
        WARN("Failed to set event handle, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = IAudioClient_GetBufferSize(renderer->audio_client, &renderer->max_frames)))
    {
        WARN("Failed to get buffer size, hr %#lx.\n", hr);
        return hr;
    }

    if (SUCCEEDED(hr = MFCreateAsyncResult(NULL, &renderer->render_callback, NULL, &result)))
    {
        if (FAILED(hr = MFPutWaitingWorkItem(renderer->buffer_ready_event, 0, result, &renderer->buffer_ready_key)))
            WARN("Failed to submit wait item, hr %#lx.\n", hr);
        IMFAsyncResult_Release(result);
    }

    return hr;
}

static HRESULT WINAPI audio_renderer_stream_type_handler_SetCurrentMediaType(IMFMediaTypeHandler *iface,
        IMFMediaType *media_type)
{
    struct audio_renderer *renderer = impl_from_IMFMediaTypeHandler(iface);
    BOOL compare_result;
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, media_type);

    if (!media_type)
        return E_POINTER;

    EnterCriticalSection(&renderer->cs);
    if (SUCCEEDED(hr = check_media_type(media_type, renderer->media_type)))
    {
        if (renderer->current_media_type)
            IMFMediaType_Release(renderer->current_media_type);
        renderer->current_media_type = media_type;
        IMFMediaType_AddRef(renderer->current_media_type);

        if (SUCCEEDED(hr = audio_renderer_create_audio_client(renderer)))
        {
            if (SUCCEEDED(IMFMediaType_Compare(renderer->media_type, (IMFAttributes *)media_type, MF_ATTRIBUTES_MATCH_OUR_ITEMS,
                    &compare_result)) && !compare_result)
            {
                IMFMediaEventQueue_QueueEventParamVar(renderer->stream_event_queue, MEStreamSinkFormatInvalidated, &GUID_NULL,
                        S_OK, NULL);
                audio_renderer_preroll(renderer);
            }
        }
    }
    else
        hr = MF_E_INVALIDMEDIATYPE;
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_stream_type_handler_GetCurrentMediaType(IMFMediaTypeHandler *iface,
        IMFMediaType **media_type)
{
    struct audio_renderer *renderer = impl_from_IMFMediaTypeHandler(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, media_type);

    EnterCriticalSection(&renderer->cs);
    if (renderer->current_media_type)
    {
        *media_type = renderer->current_media_type;
        IMFMediaType_AddRef(*media_type);
    }
    else
        hr = MF_E_NOT_INITIALIZED;
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI audio_renderer_stream_type_handler_GetMajorType(IMFMediaTypeHandler *iface, GUID *type)
{
    struct audio_renderer *renderer = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %p.\n", iface, type);

    if (!type)
        return E_POINTER;

    if (renderer->flags & SAR_SHUT_DOWN)
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

static HRESULT audio_renderer_collect_supported_types(struct audio_renderer *renderer)
{
    IAudioClient *client;
    WAVEFORMATEX *format;
    HRESULT hr;

    if (FAILED(hr = MFCreateMediaType(&renderer->media_type)))
        return hr;

    hr = IMMDevice_Activate(renderer->device, &IID_IAudioClient, CLSCTX_INPROC_SERVER, NULL, (void **)&client);
    if (FAILED(hr))
    {
        WARN("Failed to create audio client, hr %#lx.\n", hr);
        return hr;
    }

    /* FIXME:  */

    hr = IAudioClient_GetMixFormat(client, &format);
    IAudioClient_Release(client);
    if (FAILED(hr))
    {
        WARN("Failed to get device audio format, hr %#lx.\n", hr);
        return hr;
    }

    hr = MFInitMediaTypeFromWaveFormatEx(renderer->media_type, format, format->cbSize + sizeof(*format));
    CoTaskMemFree(format);
    if (FAILED(hr))
    {
        WARN("Failed to initialize media type, hr %#lx.\n", hr);
        return hr;
    }

    return hr;
}

static HRESULT WINAPI audio_renderer_render_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI audio_renderer_render_callback_AddRef(IMFAsyncCallback *iface)
{
    struct audio_renderer *renderer = impl_from_render_callback_IMFAsyncCallback(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI audio_renderer_render_callback_Release(IMFAsyncCallback *iface)
{
    struct audio_renderer *renderer = impl_from_render_callback_IMFAsyncCallback(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI audio_renderer_render_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static void audio_renderer_render(struct audio_renderer *renderer, IMFAsyncResult *result)
{
    unsigned int src_frames, dst_frames, max_frames, pad_frames;
    struct queued_object *obj, *obj2;
    BOOL keep_sample = FALSE;
    IMFMediaBuffer *buffer;
    BYTE *dst, *src;
    DWORD src_len;
    HRESULT hr;

    LIST_FOR_EACH_ENTRY_SAFE(obj, obj2, &renderer->queue, struct queued_object, entry)
    {
        if (obj->type == OBJECT_TYPE_MARKER)
        {
            IMFMediaEventQueue_QueueEventParamVar(renderer->stream_event_queue, MEStreamSinkMarker,
                &GUID_NULL, S_OK, &obj->u.marker.context);
        }
        else if (obj->type == OBJECT_TYPE_SAMPLE)
        {
            if (SUCCEEDED(IMFSample_ConvertToContiguousBuffer(obj->u.sample.sample, &buffer)))
            {
                if (SUCCEEDED(IMFMediaBuffer_Lock(buffer, &src, NULL, &src_len)))
                {
                    if ((src_frames = src_len / renderer->frame_size))
                    {
                        if (SUCCEEDED(IAudioClient_GetBufferSize(renderer->audio_client, &max_frames)))
                        {
                            if (SUCCEEDED(IAudioClient_GetCurrentPadding(renderer->audio_client, &pad_frames)))
                            {
                                max_frames -= pad_frames;
                                src_frames -= obj->u.sample.frame_offset;
                                dst_frames = min(src_frames, max_frames);

                                if (SUCCEEDED(hr = IAudioRenderClient_GetBuffer(renderer->audio_render_client, dst_frames, &dst)))
                                {
                                    memcpy(dst, src + obj->u.sample.frame_offset * renderer->frame_size,
                                            dst_frames * renderer->frame_size);

                                    IAudioRenderClient_ReleaseBuffer(renderer->audio_render_client, dst_frames, 0);

                                    obj->u.sample.frame_offset += dst_frames;
                                    renderer->queued_frames -= dst_frames;
                                }

                                keep_sample = FAILED(hr) || src_frames > max_frames;
                            }
                        }
                    }
                    IMFMediaBuffer_Unlock(buffer);
                }
                IMFMediaBuffer_Release(buffer);
            }
        }

        if (keep_sample)
            break;

        list_remove(&obj->entry);
        release_pending_object(obj);
    }

    if (list_empty(&renderer->queue) && !(renderer->flags & SAR_SAMPLE_REQUESTED))
    {
        IMFMediaEventQueue_QueueEventParamVar(renderer->stream_event_queue, MEStreamSinkRequestSample, &GUID_NULL, S_OK, NULL);
        renderer->flags |= SAR_SAMPLE_REQUESTED;
    }

    if (FAILED(hr = MFPutWaitingWorkItem(renderer->buffer_ready_event, 0, result, &renderer->buffer_ready_key)))
        WARN("Failed to submit wait item, hr %#lx.\n", hr);
}

static HRESULT WINAPI audio_renderer_render_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct audio_renderer *renderer = impl_from_render_callback_IMFAsyncCallback(iface);

    EnterCriticalSection(&renderer->cs);
    if (!(renderer->flags & SAR_SHUT_DOWN))
        audio_renderer_render(renderer, result);
    LeaveCriticalSection(&renderer->cs);

    return S_OK;
}

static const IMFAsyncCallbackVtbl audio_renderer_render_callback_vtbl =
{
    audio_renderer_render_callback_QueryInterface,
    audio_renderer_render_callback_AddRef,
    audio_renderer_render_callback_Release,
    audio_renderer_render_callback_GetParameters,
    audio_renderer_render_callback_Invoke,
};

static HRESULT sar_create_object(IMFAttributes *attributes, void *user_context, IUnknown **obj)
{
    struct audio_renderer *renderer;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", attributes, user_context, obj);

    if (!(renderer = calloc(1, sizeof(*renderer))))
        return E_OUTOFMEMORY;

    renderer->IMFMediaSink_iface.lpVtbl = &audio_renderer_sink_vtbl;
    renderer->IMFMediaSinkPreroll_iface.lpVtbl = &audio_renderer_preroll_vtbl;
    renderer->IMFStreamSink_iface.lpVtbl = &audio_renderer_stream_vtbl;
    renderer->IMFMediaTypeHandler_iface.lpVtbl = &audio_renderer_stream_type_handler_vtbl;
    renderer->IMFClockStateSink_iface.lpVtbl = &audio_renderer_clock_sink_vtbl;
    renderer->IMFMediaEventGenerator_iface.lpVtbl = &audio_renderer_events_vtbl;
    renderer->IMFGetService_iface.lpVtbl = &audio_renderer_get_service_vtbl;
    renderer->IMFSimpleAudioVolume_iface.lpVtbl = &audio_renderer_simple_volume_vtbl;
    renderer->IMFAudioStreamVolume_iface.lpVtbl = &audio_renderer_stream_volume_vtbl;
    renderer->IMFAudioPolicy_iface.lpVtbl = &audio_renderer_policy_vtbl;
    renderer->render_callback.lpVtbl = &audio_renderer_render_callback_vtbl;
    renderer->refcount = 1;
    InitializeCriticalSection(&renderer->cs);
    renderer->buffer_ready_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    list_init(&renderer->queue);

    if (FAILED(hr = MFCreateEventQueue(&renderer->event_queue)))
        goto failed;

    if (FAILED(hr = MFCreateEventQueue(&renderer->stream_event_queue)))
        goto failed;

    if (FAILED(hr = sar_create_mmdevice(attributes, renderer)))
        goto failed;

    if (FAILED(hr = audio_renderer_collect_supported_types(renderer)))
        goto failed;

    *obj = (IUnknown *)&renderer->IMFMediaSink_iface;

    return S_OK;

failed:

    IMFMediaSink_Release(&renderer->IMFMediaSink_iface);

    return hr;
}

BOOL mf_is_sar_sink(IMFMediaSink *sink)
{
    return sink->lpVtbl == &audio_renderer_sink_vtbl;
}

static void sar_shutdown_object(void *user_context, IUnknown *obj)
{
    IMFMediaSink *sink;

    if (SUCCEEDED(IUnknown_QueryInterface(obj, &IID_IMFMediaSink, (void **)&sink)))
    {
        IMFMediaSink_Shutdown(sink);
        IMFMediaSink_Release(sink);
    }
}

static const struct activate_funcs sar_activate_funcs =
{
    .create_object = sar_create_object,
    .shutdown_object = sar_shutdown_object,
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
