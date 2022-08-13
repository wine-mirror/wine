/*
 * Copyright 2022 Nikolay Sivov for CodeWeavers
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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static inline const char *debugstr_time(LONGLONG time)
{
    ULONGLONG abstime = time >= 0 ? time : -time;
    unsigned int i = 0, j = 0;
    char buffer[23], rev[23];

    while (abstime || i <= 8)
    {
        buffer[i++] = '0' + (abstime % 10);
        abstime /= 10;
        if (i == 7) buffer[i++] = '.';
    }
    if (time < 0) buffer[i++] = '-';

    while (i--) rev[j++] = buffer[i];
    while (rev[j-1] == '0' && rev[j-2] != '.') --j;
    rev[j] = 0;

    return wine_dbg_sprintf("%s", rev);
}

enum wave_sink_flags
{
    SINK_SHUT_DOWN = 0x1,
};

struct wave_sink
{
    IMFFinalizableMediaSink IMFFinalizableMediaSink_iface;
    IMFMediaEventGenerator IMFMediaEventGenerator_iface;
    IMFClockStateSink IMFClockStateSink_iface;
    IMFStreamSink IMFStreamSink_iface;
    LONG refcount;

    IMFMediaEventQueue *event_queue;
    IMFMediaEventQueue *stream_event_queue;
    IMFPresentationClock *clock;

    IMFByteStream *bytestream;

    unsigned int flags;
    CRITICAL_SECTION cs;
};

static struct wave_sink *impl_from_IMFFinalizableMediaSink(IMFFinalizableMediaSink *iface)
{
    return CONTAINING_RECORD(iface, struct wave_sink, IMFFinalizableMediaSink_iface);
}

static struct wave_sink *impl_from_IMFMediaEventGenerator(IMFMediaEventGenerator *iface)
{
    return CONTAINING_RECORD(iface, struct wave_sink, IMFMediaEventGenerator_iface);
}

static struct wave_sink *impl_from_IMFStreamSink(IMFStreamSink *iface)
{
    return CONTAINING_RECORD(iface, struct wave_sink, IMFStreamSink_iface);
}

static struct wave_sink *impl_from_IMFClockStateSink(IMFClockStateSink *iface)
{
    return CONTAINING_RECORD(iface, struct wave_sink, IMFClockStateSink_iface);
}

static HRESULT WINAPI wave_sink_QueryInterface(IMFFinalizableMediaSink *iface, REFIID riid, void **obj)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFFinalizableMediaSink) ||
            IsEqualIID(riid, &IID_IMFMediaSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaEventGenerator))
    {
        *obj = &sink->IMFMediaEventGenerator_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFClockStateSink))
    {
        *obj = &sink->IMFClockStateSink_iface;
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

static ULONG WINAPI wave_sink_AddRef(IMFFinalizableMediaSink *iface)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);
    ULONG refcount = InterlockedIncrement(&sink->refcount);
    TRACE("%p, refcount %lu.\n", iface, refcount);
    return refcount;
}

static ULONG WINAPI wave_sink_Release(IMFFinalizableMediaSink *iface)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);
    ULONG refcount = InterlockedDecrement(&sink->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (sink->event_queue)
            IMFMediaEventQueue_Release(sink->event_queue);
        if (sink->stream_event_queue)
            IMFMediaEventQueue_Release(sink->stream_event_queue);
        IMFByteStream_Release(sink->bytestream);
        DeleteCriticalSection(&sink->cs);
        free(sink);
    }

    return refcount;
}

static HRESULT WINAPI wave_sink_GetCharacteristics(IMFFinalizableMediaSink *iface, DWORD *flags)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);

    TRACE("%p, %p.\n", iface, flags);

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_SHUTDOWN;

    *flags = MEDIASINK_FIXED_STREAMS | MEDIASINK_RATELESS;

    return S_OK;
}

static HRESULT WINAPI wave_sink_AddStreamSink(IMFFinalizableMediaSink *iface, DWORD stream_sink_id,
    IMFMediaType *media_type, IMFStreamSink **stream_sink)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);

    TRACE("%p, %#lx, %p, %p.\n", iface, stream_sink_id, media_type, stream_sink);

    return sink->flags & SINK_SHUT_DOWN ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI wave_sink_RemoveStreamSink(IMFFinalizableMediaSink *iface, DWORD stream_sink_id)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);

    TRACE("%p, %#lx.\n", iface, stream_sink_id);

    return sink->flags & SINK_SHUT_DOWN ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI wave_sink_GetStreamSinkCount(IMFFinalizableMediaSink *iface, DWORD *count)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);

    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_POINTER;

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_SHUTDOWN;

    *count = 1;

    return S_OK;
}

static HRESULT WINAPI wave_sink_GetStreamSinkByIndex(IMFFinalizableMediaSink *iface, DWORD index,
        IMFStreamSink **stream)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %lu, %p.\n", iface, index, stream);

    EnterCriticalSection(&sink->cs);

    if (sink->flags & SINK_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (index)
        hr = MF_E_INVALIDINDEX;
    else
    {
       *stream = &sink->IMFStreamSink_iface;
       IMFStreamSink_AddRef(*stream);
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI wave_sink_GetStreamSinkById(IMFFinalizableMediaSink *iface, DWORD stream_sink_id,
        IMFStreamSink **stream)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %#lx, %p.\n", iface, stream_sink_id, stream);

    EnterCriticalSection(&sink->cs);

    if (sink->flags & SINK_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (stream_sink_id != 1)
        hr = MF_E_INVALIDSTREAMNUMBER;
    else
    {
        *stream = &sink->IMFStreamSink_iface;
        IMFStreamSink_AddRef(*stream);
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static void wave_sink_set_presentation_clock(struct wave_sink *sink, IMFPresentationClock *clock)
{
    if (sink->clock)
    {
        IMFPresentationClock_RemoveClockStateSink(sink->clock, &sink->IMFClockStateSink_iface);
        IMFPresentationClock_Release(sink->clock);
    }
    sink->clock = clock;
    if (sink->clock)
    {
        IMFPresentationClock_AddRef(sink->clock);
        IMFPresentationClock_AddClockStateSink(sink->clock, &sink->IMFClockStateSink_iface);
    }
}

static HRESULT WINAPI wave_sink_SetPresentationClock(IMFFinalizableMediaSink *iface, IMFPresentationClock *clock)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, clock);

    EnterCriticalSection(&sink->cs);

    if (sink->flags & SINK_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        wave_sink_set_presentation_clock(sink, clock);

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI wave_sink_GetPresentationClock(IMFFinalizableMediaSink *iface, IMFPresentationClock **clock)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, clock);

    if (!clock)
        return E_POINTER;

    EnterCriticalSection(&sink->cs);

    if (sink->flags & SINK_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (sink->clock)
    {
        *clock = sink->clock;
        IMFPresentationClock_AddRef(*clock);
    }
    else
        hr = MF_E_NO_CLOCK;

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI wave_sink_Shutdown(IMFFinalizableMediaSink *iface)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&sink->cs);

    if (sink->flags & SINK_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        sink->flags |= SINK_SHUT_DOWN;
        IMFMediaEventQueue_Shutdown(sink->event_queue);
        IMFMediaEventQueue_Shutdown(sink->stream_event_queue);
        wave_sink_set_presentation_clock(sink, NULL);
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI wave_sink_BeginFinalize(IMFFinalizableMediaSink *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    FIXME("%p, %p, %p.\n", iface, callback, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI wave_sink_EndFinalize(IMFFinalizableMediaSink *iface, IMFAsyncResult *result)
{
    FIXME("%p, %p.\n", iface, result);

    return E_NOTIMPL;
}

static const IMFFinalizableMediaSinkVtbl wave_sink_vtbl =
{
    wave_sink_QueryInterface,
    wave_sink_AddRef,
    wave_sink_Release,
    wave_sink_GetCharacteristics,
    wave_sink_AddStreamSink,
    wave_sink_RemoveStreamSink,
    wave_sink_GetStreamSinkCount,
    wave_sink_GetStreamSinkByIndex,
    wave_sink_GetStreamSinkById,
    wave_sink_SetPresentationClock,
    wave_sink_GetPresentationClock,
    wave_sink_Shutdown,
    wave_sink_BeginFinalize,
    wave_sink_EndFinalize,
};

static HRESULT WINAPI wave_sink_events_QueryInterface(IMFMediaEventGenerator *iface, REFIID riid, void **obj)
{
    struct wave_sink *sink = impl_from_IMFMediaEventGenerator(iface);
    return IMFFinalizableMediaSink_QueryInterface(&sink->IMFFinalizableMediaSink_iface, riid, obj);
}

static ULONG WINAPI wave_sink_events_AddRef(IMFMediaEventGenerator *iface)
{
    struct wave_sink *sink = impl_from_IMFMediaEventGenerator(iface);
    return IMFFinalizableMediaSink_AddRef(&sink->IMFFinalizableMediaSink_iface);
}

static ULONG WINAPI wave_sink_events_Release(IMFMediaEventGenerator *iface)
{
    struct wave_sink *sink = impl_from_IMFMediaEventGenerator(iface);
    return IMFFinalizableMediaSink_Release(&sink->IMFFinalizableMediaSink_iface);
}

static HRESULT WINAPI wave_sink_events_GetEvent(IMFMediaEventGenerator *iface, DWORD flags, IMFMediaEvent **event)
{
    struct wave_sink *sink = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %#lx, %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(sink->event_queue, flags, event);
}

static HRESULT WINAPI wave_sink_events_BeginGetEvent(IMFMediaEventGenerator *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct wave_sink *sink = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(sink->event_queue, callback, state);
}

static HRESULT WINAPI wave_sink_events_EndGetEvent(IMFMediaEventGenerator *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct wave_sink *sink = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(sink->event_queue, result, event);
}

static HRESULT WINAPI wave_sink_events_QueueEvent(IMFMediaEventGenerator *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct wave_sink *sink = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %lu, %s, %#lx, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(sink->event_queue, event_type, ext_type, hr, value);
}

static const IMFMediaEventGeneratorVtbl wave_sink_events_vtbl =
{
    wave_sink_events_QueryInterface,
    wave_sink_events_AddRef,
    wave_sink_events_Release,
    wave_sink_events_GetEvent,
    wave_sink_events_BeginGetEvent,
    wave_sink_events_EndGetEvent,
    wave_sink_events_QueueEvent,
};

static HRESULT WINAPI wave_stream_sink_QueryInterface(IMFStreamSink *iface, REFIID riid, void **obj)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFStreamSink) ||
            IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &sink->IMFStreamSink_iface;
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

static ULONG WINAPI wave_stream_sink_AddRef(IMFStreamSink *iface)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);
    return IMFFinalizableMediaSink_AddRef(&sink->IMFFinalizableMediaSink_iface);
}

static ULONG WINAPI wave_stream_sink_Release(IMFStreamSink *iface)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);
    return IMFFinalizableMediaSink_Release(&sink->IMFFinalizableMediaSink_iface);
}

static HRESULT WINAPI wave_stream_sink_GetEvent(IMFStreamSink *iface, DWORD flags, IMFMediaEvent **event)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %#lx, %p.\n", iface, flags, event);

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_GetEvent(sink->stream_event_queue, flags, event);
}

static HRESULT WINAPI wave_stream_sink_BeginGetEvent(IMFStreamSink *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_BeginGetEvent(sink->stream_event_queue, callback, state);
}

static HRESULT WINAPI wave_stream_sink_EndGetEvent(IMFStreamSink *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_EndGetEvent(sink->stream_event_queue, result, event);
}

static HRESULT WINAPI wave_stream_sink_QueueEvent(IMFStreamSink *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %lu, %s, %#lx, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_QueueEventParamVar(sink->stream_event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI wave_stream_sink_GetMediaSink(IMFStreamSink *iface, IMFMediaSink **ret)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, ret);

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    *ret = (IMFMediaSink *)&sink->IMFFinalizableMediaSink_iface;
    IMFMediaSink_AddRef(*ret);

    return S_OK;
}

static HRESULT WINAPI wave_stream_sink_GetIdentifier(IMFStreamSink *iface, DWORD *identifier)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, identifier);

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    *identifier = 1;

    return S_OK;
}

static HRESULT WINAPI wave_stream_sink_GetMediaTypeHandler(IMFStreamSink *iface, IMFMediaTypeHandler **handler)
{
    FIXME("%p, %p.\n", iface, handler);

    return E_NOTIMPL;
}

static HRESULT WINAPI wave_stream_sink_ProcessSample(IMFStreamSink *iface, IMFSample *sample)
{
    FIXME("%p, %p.\n", iface, sample);

    return E_NOTIMPL;
}

static HRESULT WINAPI wave_stream_sink_PlaceMarker(IMFStreamSink *iface, MFSTREAMSINK_MARKER_TYPE marker_type,
        const PROPVARIANT *marker_value, const PROPVARIANT *context_value)
{
    FIXME("%p, %d, %p, %p.\n", iface, marker_type, marker_value, context_value);

    return E_NOTIMPL;
}

static HRESULT WINAPI wave_stream_sink_Flush(IMFStreamSink *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static const IMFStreamSinkVtbl wave_stream_sink_vtbl =
{
    wave_stream_sink_QueryInterface,
    wave_stream_sink_AddRef,
    wave_stream_sink_Release,
    wave_stream_sink_GetEvent,
    wave_stream_sink_BeginGetEvent,
    wave_stream_sink_EndGetEvent,
    wave_stream_sink_QueueEvent,
    wave_stream_sink_GetMediaSink,
    wave_stream_sink_GetIdentifier,
    wave_stream_sink_GetMediaTypeHandler,
    wave_stream_sink_ProcessSample,
    wave_stream_sink_PlaceMarker,
    wave_stream_sink_Flush,
};

static HRESULT WINAPI wave_sink_clock_sink_QueryInterface(IMFClockStateSink *iface, REFIID riid, void **obj)
{
    struct wave_sink *sink = impl_from_IMFClockStateSink(iface);
    return IMFFinalizableMediaSink_QueryInterface(&sink->IMFFinalizableMediaSink_iface, riid, obj);
}

static ULONG WINAPI wave_sink_clock_sink_AddRef(IMFClockStateSink *iface)
{
    struct wave_sink *sink = impl_from_IMFClockStateSink(iface);
    return IMFFinalizableMediaSink_AddRef(&sink->IMFFinalizableMediaSink_iface);
}

static ULONG WINAPI wave_sink_clock_sink_Release(IMFClockStateSink *iface)
{
    struct wave_sink *sink = impl_from_IMFClockStateSink(iface);
    return IMFFinalizableMediaSink_Release(&sink->IMFFinalizableMediaSink_iface);
}

static HRESULT WINAPI wave_sink_clock_sink_OnClockStart(IMFClockStateSink *iface, MFTIME systime, LONGLONG offset)
{
    struct wave_sink *sink = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_time(systime), debugstr_time(offset));

    return IMFMediaEventQueue_QueueEventParamVar(sink->stream_event_queue, MEStreamSinkStarted, &GUID_NULL, S_OK, NULL);
}

static HRESULT WINAPI wave_sink_clock_sink_OnClockStop(IMFClockStateSink *iface, MFTIME systime)
{
    struct wave_sink *sink = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    return IMFMediaEventQueue_QueueEventParamVar(sink->stream_event_queue, MEStreamSinkStopped, &GUID_NULL, S_OK, NULL);
}

static HRESULT WINAPI wave_sink_clock_sink_OnClockPause(IMFClockStateSink *iface, MFTIME systime)
{
    struct wave_sink *sink = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    return IMFMediaEventQueue_QueueEventParamVar(sink->stream_event_queue, MEStreamSinkPaused, &GUID_NULL, S_OK, NULL);
}

static HRESULT WINAPI wave_sink_clock_sink_OnClockRestart(IMFClockStateSink *iface, MFTIME systime)
{
    struct wave_sink *sink = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    return IMFMediaEventQueue_QueueEventParamVar(sink->stream_event_queue, MEStreamSinkStarted, &GUID_NULL, S_OK, NULL);
}

static HRESULT WINAPI wave_sink_clock_sink_OnClockSetRate(IMFClockStateSink *iface, MFTIME systime, float rate)
{
    FIXME("%p, %s, %f.\n", iface, debugstr_time(systime), rate);

    return E_NOTIMPL;
}

static const IMFClockStateSinkVtbl wave_sink_clock_sink_vtbl =
{
    wave_sink_clock_sink_QueryInterface,
    wave_sink_clock_sink_AddRef,
    wave_sink_clock_sink_Release,
    wave_sink_clock_sink_OnClockStart,
    wave_sink_clock_sink_OnClockStop,
    wave_sink_clock_sink_OnClockPause,
    wave_sink_clock_sink_OnClockRestart,
    wave_sink_clock_sink_OnClockSetRate,
};

/***********************************************************************
 *      MFCreateWAVEMediaSink (mfsrcsnk.@)
 */
HRESULT WINAPI MFCreateWAVEMediaSink(IMFByteStream *bytestream, IMFMediaType *media_type,
        IMFMediaSink **sink)
{
    struct wave_sink *object;
    DWORD flags = 0;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", bytestream, media_type, sink);

    if (!bytestream || !media_type || !sink)
        return E_POINTER;

    if (FAILED(hr = IMFByteStream_GetCapabilities(bytestream, &flags))) return hr;
    if (!(flags & MFBYTESTREAM_IS_WRITABLE)) return E_INVALIDARG;

    /* FIXME: do basic media type validation */

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFFinalizableMediaSink_iface.lpVtbl = &wave_sink_vtbl;
    object->IMFMediaEventGenerator_iface.lpVtbl = &wave_sink_events_vtbl;
    object->IMFStreamSink_iface.lpVtbl = &wave_stream_sink_vtbl;
    object->IMFClockStateSink_iface.lpVtbl = &wave_sink_clock_sink_vtbl;
    object->refcount = 1;
    IMFByteStream_AddRef((object->bytestream = bytestream));
    InitializeCriticalSection(&object->cs);

    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
        goto failed;

    if (FAILED(hr = MFCreateEventQueue(&object->stream_event_queue)))
        goto failed;

    *sink = (IMFMediaSink *)&object->IMFFinalizableMediaSink_iface;

    return S_OK;

failed:

    IMFFinalizableMediaSink_Release(&object->IMFFinalizableMediaSink_iface);

    return hr;
}
