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

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct sample_grabber_stream
{
    IMFStreamSink IMFStreamSink_iface;
    IMFMediaTypeHandler IMFMediaTypeHandler_iface;
    LONG refcount;
    IMFMediaSink *sink;
    IMFMediaEventQueue *event_queue;
};

struct sample_grabber
{
    IMFMediaSink IMFMediaSink_iface;
    IMFClockStateSink IMFClockStateSink_iface;
    IMFMediaEventGenerator IMFMediaEventGenerator_iface;
    LONG refcount;
    IMFSampleGrabberSinkCallback *callback;
    IMFMediaType *media_type;
    BOOL is_shut_down;
    IMFStreamSink *stream;
    IMFMediaEventQueue *event_queue;
    CRITICAL_SECTION cs;
};

struct sample_grabber_activate_context
{
    IMFMediaType *media_type;
    IMFSampleGrabberSinkCallback *callback;
};

static void sample_grabber_free_private(void *user_context)
{
    struct sample_grabber_activate_context *context = user_context;
    IMFMediaType_Release(context->media_type);
    IMFSampleGrabberSinkCallback_Release(context->callback);
    heap_free(context);
}

static struct sample_grabber *impl_from_IMFMediaSink(IMFMediaSink *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber, IMFMediaSink_iface);
}

static struct sample_grabber *impl_from_IMFClockStateSink(IMFClockStateSink *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber, IMFClockStateSink_iface);
}

static struct sample_grabber *impl_from_IMFMediaEventGenerator(IMFMediaEventGenerator *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber, IMFMediaEventGenerator_iface);
}

static struct sample_grabber_stream *impl_from_IMFStreamSink(IMFStreamSink *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber_stream, IMFStreamSink_iface);
}

static struct sample_grabber_stream *impl_from_IMFMediaTypeHandler(IMFMediaTypeHandler *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber_stream, IMFMediaTypeHandler_iface);
}

static HRESULT WINAPI sample_grabber_stream_QueryInterface(IMFStreamSink *iface, REFIID riid, void **obj)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);

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

static ULONG WINAPI sample_grabber_stream_AddRef(IMFStreamSink *iface)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);
    ULONG refcount = InterlockedIncrement(&stream->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI sample_grabber_stream_Release(IMFStreamSink *iface)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        IMFMediaSink_Release(stream->sink);
        if (stream->event_queue)
        {
            IMFMediaEventQueue_Shutdown(stream->event_queue);
            IMFMediaEventQueue_Release(stream->event_queue);
        }
        heap_free(stream);
    }

    return refcount;
}

static HRESULT WINAPI sample_grabber_stream_GetEvent(IMFStreamSink *iface, DWORD flags, IMFMediaEvent **event)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %#x, %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(stream->event_queue, flags, event);
}

static HRESULT WINAPI sample_grabber_stream_BeginGetEvent(IMFStreamSink *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(stream->event_queue, callback, state);
}

static HRESULT WINAPI sample_grabber_stream_EndGetEvent(IMFStreamSink *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(stream->event_queue, result, event);
}

static HRESULT WINAPI sample_grabber_stream_QueueEvent(IMFStreamSink *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %u, %s, %#x, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(stream->event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI sample_grabber_stream_GetMediaSink(IMFStreamSink *iface, IMFMediaSink **sink)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, sink);

    *sink = stream->sink;
    IMFMediaSink_AddRef(*sink);

    return S_OK;
}

static HRESULT WINAPI sample_grabber_stream_GetIdentifier(IMFStreamSink *iface, DWORD *identifier)
{
    TRACE("%p, %p.\n", iface, identifier);

    *identifier = 0;

    return S_OK;
}

static HRESULT WINAPI sample_grabber_stream_GetMediaTypeHandler(IMFStreamSink *iface, IMFMediaTypeHandler **handler)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, handler);

    *handler = &stream->IMFMediaTypeHandler_iface;
    IMFMediaTypeHandler_AddRef(*handler);

    return S_OK;
}

static HRESULT WINAPI sample_grabber_stream_ProcessSample(IMFStreamSink *iface, IMFSample *sample)
{
    FIXME("%p, %p.\n", iface, sample);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_grabber_stream_PlaceMarker(IMFStreamSink *iface, MFSTREAMSINK_MARKER_TYPE marker_type,
        const PROPVARIANT *marker_value, const PROPVARIANT *context_value)
{
    FIXME("%p, %d, %p, %p.\n", iface, marker_type, marker_value, context_value);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_grabber_stream_Flush(IMFStreamSink *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static const IMFStreamSinkVtbl sample_grabber_stream_vtbl =
{
    sample_grabber_stream_QueryInterface,
    sample_grabber_stream_AddRef,
    sample_grabber_stream_Release,
    sample_grabber_stream_GetEvent,
    sample_grabber_stream_BeginGetEvent,
    sample_grabber_stream_EndGetEvent,
    sample_grabber_stream_QueueEvent,
    sample_grabber_stream_GetMediaSink,
    sample_grabber_stream_GetIdentifier,
    sample_grabber_stream_GetMediaTypeHandler,
    sample_grabber_stream_ProcessSample,
    sample_grabber_stream_PlaceMarker,
    sample_grabber_stream_Flush,
};

static HRESULT WINAPI sample_grabber_stream_type_handler_QueryInterface(IMFMediaTypeHandler *iface, REFIID riid,
        void **obj)
{
    struct sample_grabber_stream *stream = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_QueryInterface(&stream->IMFStreamSink_iface, riid, obj);
}

static ULONG WINAPI sample_grabber_stream_type_handler_AddRef(IMFMediaTypeHandler *iface)
{
    struct sample_grabber_stream *stream = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_AddRef(&stream->IMFStreamSink_iface);
}

static ULONG WINAPI sample_grabber_stream_type_handler_Release(IMFMediaTypeHandler *iface)
{
    struct sample_grabber_stream *stream = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_Release(&stream->IMFStreamSink_iface);
}

static HRESULT WINAPI sample_grabber_stream_type_handler_IsMediaTypeSupported(IMFMediaTypeHandler *iface,
        IMFMediaType *in_type, IMFMediaType **out_type)
{
    FIXME("%p, %p, %p.\n", iface, in_type, out_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_grabber_stream_type_handler_GetMediaTypeCount(IMFMediaTypeHandler *iface, DWORD *count)
{
    FIXME("%p, %p.\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_grabber_stream_type_handler_GetMediaTypeByIndex(IMFMediaTypeHandler *iface, DWORD index,
        IMFMediaType **type)
{
    FIXME("%p, %u, %p.\n", iface, index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_grabber_stream_type_handler_SetCurrentMediaType(IMFMediaTypeHandler *iface,
        IMFMediaType *type)
{
    FIXME("%p, %p.\n", iface, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_grabber_stream_type_handler_GetCurrentMediaType(IMFMediaTypeHandler *iface,
        IMFMediaType **type)
{
    FIXME("%p, %p.\n", iface, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_grabber_stream_type_handler_GetMajorType(IMFMediaTypeHandler *iface, GUID *type)
{
    FIXME("%p, %p.\n", iface, type);

    return E_NOTIMPL;
}

static const IMFMediaTypeHandlerVtbl sample_grabber_stream_type_handler_vtbl =
{
    sample_grabber_stream_type_handler_QueryInterface,
    sample_grabber_stream_type_handler_AddRef,
    sample_grabber_stream_type_handler_Release,
    sample_grabber_stream_type_handler_IsMediaTypeSupported,
    sample_grabber_stream_type_handler_GetMediaTypeCount,
    sample_grabber_stream_type_handler_GetMediaTypeByIndex,
    sample_grabber_stream_type_handler_SetCurrentMediaType,
    sample_grabber_stream_type_handler_GetCurrentMediaType,
    sample_grabber_stream_type_handler_GetMajorType,
};

static HRESULT WINAPI sample_grabber_sink_QueryInterface(IMFMediaSink *iface, REFIID riid, void **obj)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFMediaSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &grabber->IMFMediaSink_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFClockStateSink))
    {
        *obj = &grabber->IMFClockStateSink_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaEventGenerator))
    {
        *obj = &grabber->IMFMediaEventGenerator_iface;
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

static ULONG WINAPI sample_grabber_sink_AddRef(IMFMediaSink *iface)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);
    ULONG refcount = InterlockedIncrement(&grabber->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI sample_grabber_sink_Release(IMFMediaSink *iface)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);
    ULONG refcount = InterlockedDecrement(&grabber->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        IMFSampleGrabberSinkCallback_Release(grabber->callback);
        IMFMediaType_Release(grabber->media_type);
        if (grabber->event_queue)
        {
            IMFMediaEventQueue_Shutdown(grabber->event_queue);
            IMFMediaEventQueue_Release(grabber->event_queue);
        }
        DeleteCriticalSection(&grabber->cs);
        heap_free(grabber);
    }

    return refcount;
}

static HRESULT WINAPI sample_grabber_sink_GetCharacteristics(IMFMediaSink *iface, DWORD *flags)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);

    TRACE("%p, %p.\n", iface, flags);

    if (grabber->is_shut_down)
        return MF_E_SHUTDOWN;

    *flags = MEDIASINK_FIXED_STREAMS;

    return S_OK;
}

static HRESULT WINAPI sample_grabber_sink_AddStreamSink(IMFMediaSink *iface, DWORD stream_sink_id,
    IMFMediaType *media_type, IMFStreamSink **stream_sink)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);

    TRACE("%p, %#x, %p, %p.\n", iface, stream_sink_id, media_type, stream_sink);

    return grabber->is_shut_down ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI sample_grabber_sink_RemoveStreamSink(IMFMediaSink *iface, DWORD stream_sink_id)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);

    TRACE("%p, %#x.\n", iface, stream_sink_id);

    return grabber->is_shut_down ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI sample_grabber_sink_GetStreamSinkCount(IMFMediaSink *iface, DWORD *count)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);

    TRACE("%p, %p.\n", iface, count);

    if (grabber->is_shut_down)
        return MF_E_SHUTDOWN;

    *count = 1;

    return S_OK;
}

static HRESULT WINAPI sample_grabber_sink_GetStreamSinkByIndex(IMFMediaSink *iface, DWORD index,
        IMFStreamSink **stream)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p.\n", iface, index, stream);

    if (grabber->is_shut_down)
        return MF_E_SHUTDOWN;

    EnterCriticalSection(&grabber->cs);

    if (grabber->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else if (index > 0)
        hr = MF_E_INVALIDINDEX;
    else
    {
       *stream = grabber->stream;
       IMFStreamSink_AddRef(*stream);
    }

    LeaveCriticalSection(&grabber->cs);

    return hr;
}

static HRESULT WINAPI sample_grabber_sink_GetStreamSinkById(IMFMediaSink *iface, DWORD stream_sink_id,
        IMFStreamSink **stream)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %#x, %p.\n", iface, stream_sink_id, stream);

    EnterCriticalSection(&grabber->cs);

    if (grabber->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else if (stream_sink_id > 0)
        hr = MF_E_INVALIDSTREAMNUMBER;
    else
    {
        *stream = grabber->stream;
        IMFStreamSink_AddRef(*stream);
    }

    LeaveCriticalSection(&grabber->cs);

    return hr;
}

static HRESULT WINAPI sample_grabber_sink_SetPresentationClock(IMFMediaSink *iface, IMFPresentationClock *clock)
{
    FIXME("%p, %p.\n", iface, clock);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_grabber_sink_GetPresentationClock(IMFMediaSink *iface, IMFPresentationClock **clock)
{
    FIXME("%p, %p.\n", iface, clock);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_grabber_sink_Shutdown(IMFMediaSink *iface)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);

    TRACE("%p.\n", iface);

    if (grabber->is_shut_down)
        return MF_E_SHUTDOWN;

    EnterCriticalSection(&grabber->cs);
    grabber->is_shut_down = TRUE;
    IMFStreamSink_Release(grabber->stream);
    grabber->stream = NULL;
    EnterCriticalSection(&grabber->cs);

    return E_NOTIMPL;
}

static const IMFMediaSinkVtbl sample_grabber_sink_vtbl =
{
    sample_grabber_sink_QueryInterface,
    sample_grabber_sink_AddRef,
    sample_grabber_sink_Release,
    sample_grabber_sink_GetCharacteristics,
    sample_grabber_sink_AddStreamSink,
    sample_grabber_sink_RemoveStreamSink,
    sample_grabber_sink_GetStreamSinkCount,
    sample_grabber_sink_GetStreamSinkByIndex,
    sample_grabber_sink_GetStreamSinkById,
    sample_grabber_sink_SetPresentationClock,
    sample_grabber_sink_GetPresentationClock,
    sample_grabber_sink_Shutdown,
};

static HRESULT WINAPI sample_grabber_clock_sink_QueryInterface(IMFClockStateSink *iface, REFIID riid, void **obj)
{
    struct sample_grabber *grabber = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_QueryInterface(&grabber->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI sample_grabber_clock_sink_AddRef(IMFClockStateSink *iface)
{
    struct sample_grabber *grabber = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_AddRef(&grabber->IMFMediaSink_iface);
}

static ULONG WINAPI sample_grabber_clock_sink_Release(IMFClockStateSink *iface)
{
    struct sample_grabber *grabber = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_Release(&grabber->IMFMediaSink_iface);
}

static HRESULT WINAPI sample_grabber_clock_sink_OnClockStart(IMFClockStateSink *iface, MFTIME systime, LONGLONG offset)
{
    FIXME("%p, %s, %s.\n", iface, wine_dbgstr_longlong(systime), wine_dbgstr_longlong(offset));

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_grabber_clock_sink_OnClockStop(IMFClockStateSink *iface, MFTIME systime)
{
    FIXME("%p, %s.\n", iface, wine_dbgstr_longlong(systime));

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_grabber_clock_sink_OnClockPause(IMFClockStateSink *iface, MFTIME systime)
{
    FIXME("%p, %s.\n", iface, wine_dbgstr_longlong(systime));

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_grabber_clock_sink_OnClockRestart(IMFClockStateSink *iface, MFTIME systime)
{
    FIXME("%p, %s.\n", iface, wine_dbgstr_longlong(systime));

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_grabber_clock_sink_OnClockSetRate(IMFClockStateSink *iface, MFTIME systime, float rate)
{
    FIXME("%p, %s, %f.\n", iface, wine_dbgstr_longlong(systime), rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_grabber_events_QueryInterface(IMFMediaEventGenerator *iface, REFIID riid, void **obj)
{
    struct sample_grabber *grabber = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_QueryInterface(&grabber->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI sample_grabber_events_AddRef(IMFMediaEventGenerator *iface)
{
    struct sample_grabber *grabber = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_AddRef(&grabber->IMFMediaSink_iface);
}

static ULONG WINAPI sample_grabber_events_Release(IMFMediaEventGenerator *iface)
{
    struct sample_grabber *grabber = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_Release(&grabber->IMFMediaSink_iface);
}

static HRESULT WINAPI sample_grabber_events_GetEvent(IMFMediaEventGenerator *iface, DWORD flags, IMFMediaEvent **event)
{
    struct sample_grabber *grabber = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %#x, %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(grabber->event_queue, flags, event);
}

static HRESULT WINAPI sample_grabber_events_BeginGetEvent(IMFMediaEventGenerator *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct sample_grabber *grabber = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(grabber->event_queue, callback, state);
}

static HRESULT WINAPI sample_grabber_events_EndGetEvent(IMFMediaEventGenerator *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct sample_grabber *grabber = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(grabber->event_queue, result, event);
}

static HRESULT WINAPI sample_grabber_events_QueueEvent(IMFMediaEventGenerator *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct sample_grabber *grabber = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %u, %s, %#x, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(grabber->event_queue, event_type, ext_type, hr, value);
}

static const IMFMediaEventGeneratorVtbl sample_grabber_sink_events_vtbl =
{
    sample_grabber_events_QueryInterface,
    sample_grabber_events_AddRef,
    sample_grabber_events_Release,
    sample_grabber_events_GetEvent,
    sample_grabber_events_BeginGetEvent,
    sample_grabber_events_EndGetEvent,
    sample_grabber_events_QueueEvent,
};

static const IMFClockStateSinkVtbl sample_grabber_clock_sink_vtbl =
{
    sample_grabber_clock_sink_QueryInterface,
    sample_grabber_clock_sink_AddRef,
    sample_grabber_clock_sink_Release,
    sample_grabber_clock_sink_OnClockStart,
    sample_grabber_clock_sink_OnClockStop,
    sample_grabber_clock_sink_OnClockPause,
    sample_grabber_clock_sink_OnClockRestart,
    sample_grabber_clock_sink_OnClockSetRate,
};

static HRESULT sample_grabber_create_stream(IMFMediaSink *sink, IMFStreamSink **stream)
{
    struct sample_grabber_stream *object;
    HRESULT hr;

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFStreamSink_iface.lpVtbl = &sample_grabber_stream_vtbl;
    object->IMFMediaTypeHandler_iface.lpVtbl = &sample_grabber_stream_type_handler_vtbl;
    object->refcount = 1;
    object->sink = sink;
    IMFMediaSink_AddRef(object->sink);

    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
        goto failed;

    *stream = &object->IMFStreamSink_iface;

    return S_OK;

failed:
    IMFStreamSink_Release(&object->IMFStreamSink_iface);

    return hr;
}

static HRESULT sample_grabber_create_object(IMFAttributes *attributes, void *user_context, IUnknown **obj)
{
    struct sample_grabber_activate_context *context = user_context;
    struct sample_grabber *object;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", attributes, user_context, obj);

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFMediaSink_iface.lpVtbl = &sample_grabber_sink_vtbl;
    object->IMFClockStateSink_iface.lpVtbl = &sample_grabber_clock_sink_vtbl;
    object->IMFMediaEventGenerator_iface.lpVtbl = &sample_grabber_sink_events_vtbl;
    object->refcount = 1;
    object->callback = context->callback;
    IMFSampleGrabberSinkCallback_AddRef(object->callback);
    object->media_type = context->media_type;
    IMFMediaType_AddRef(object->media_type);
    InitializeCriticalSection(&object->cs);

    if (FAILED(hr = sample_grabber_create_stream(&object->IMFMediaSink_iface, &object->stream)))
        goto failed;

    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
        goto failed;

    *obj = (IUnknown *)&object->IMFMediaSink_iface;

    TRACE("Created %p.\n", *obj);

    return S_OK;

failed:

    IMFMediaSink_Release(&object->IMFMediaSink_iface);

    return hr;
}

static const struct activate_funcs sample_grabber_activate_funcs =
{
    sample_grabber_create_object,
    sample_grabber_free_private,
};

/***********************************************************************
 *      MFCreateSampleGrabberSinkActivate (mf.@)
 */
HRESULT WINAPI MFCreateSampleGrabberSinkActivate(IMFMediaType *media_type, IMFSampleGrabberSinkCallback *callback,
        IMFActivate **activate)
{
    struct sample_grabber_activate_context *context;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", media_type, callback, activate);

    if (!media_type || !callback || !activate)
        return E_POINTER;

    context = heap_alloc_zero(sizeof(*context));
    if (!context)
        return E_OUTOFMEMORY;

    context->media_type = media_type;
    IMFMediaType_AddRef(context->media_type);
    context->callback = callback;
    IMFSampleGrabberSinkCallback_AddRef(context->callback);

    if (FAILED(hr = create_activation_object(context, &sample_grabber_activate_funcs, activate)))
        sample_grabber_free_private(context);

    return hr;
}
