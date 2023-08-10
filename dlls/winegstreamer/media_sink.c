/* Gstreamer Media Sink
 *
 * Copyright 2023 Ziqing Hui for CodeWeavers
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

#include "gst_private.h"
#include "mferror.h"
#include "mfapi.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct stream_sink
{
    IMFStreamSink IMFStreamSink_iface;
    LONG refcount;
    DWORD id;

    IMFMediaType *type;
    IMFFinalizableMediaSink *media_sink;
    IMFMediaEventQueue *event_queue;

    struct list entry;
};

struct media_sink
{
    IMFFinalizableMediaSink IMFFinalizableMediaSink_iface;
    LONG refcount;
    CRITICAL_SECTION cs;
    bool shutdown;

    IMFByteStream *bytestream;

    struct list stream_sinks;
};

static struct stream_sink *impl_from_IMFStreamSink(IMFStreamSink *iface)
{
    return CONTAINING_RECORD(iface, struct stream_sink, IMFStreamSink_iface);
}

static struct media_sink *impl_from_IMFFinalizableMediaSink(IMFFinalizableMediaSink *iface)
{
    return CONTAINING_RECORD(iface, struct media_sink, IMFFinalizableMediaSink_iface);
}

static HRESULT WINAPI stream_sink_QueryInterface(IMFStreamSink *iface, REFIID riid, void **obj)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);

    TRACE("iface %p, riid %s, obj %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFStreamSink) ||
            IsEqualGUID(riid, &IID_IMFMediaEventGenerator) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &stream_sink->IMFStreamSink_iface;
    }
    else
    {
        WARN("Unsupported interface %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);

    return S_OK;
}

static ULONG WINAPI stream_sink_AddRef(IMFStreamSink *iface)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);
    ULONG refcount = InterlockedIncrement(&stream_sink->refcount);
    TRACE("iface %p, refcount %lu.\n", iface, refcount);
    return refcount;
}

static ULONG WINAPI stream_sink_Release(IMFStreamSink *iface)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);
    ULONG refcount = InterlockedDecrement(&stream_sink->refcount);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IMFMediaEventQueue_Release(stream_sink->event_queue);
        IMFFinalizableMediaSink_Release(stream_sink->media_sink);
        if (stream_sink->type)
            IMFMediaType_Release(stream_sink->type);
        free(stream_sink);
    }

    return refcount;
}

static HRESULT WINAPI stream_sink_GetEvent(IMFStreamSink *iface, DWORD flags, IMFMediaEvent **event)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);

    TRACE("iface %p, flags %#lx, event %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(stream_sink->event_queue, flags, event);
}

static HRESULT WINAPI stream_sink_BeginGetEvent(IMFStreamSink *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);

    TRACE("iface %p, callback %p, state %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(stream_sink->event_queue, callback, state);
}

static HRESULT WINAPI stream_sink_EndGetEvent(IMFStreamSink *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);

    TRACE("iface %p, result %p, event %p.\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(stream_sink->event_queue, result, event);
}

static HRESULT WINAPI stream_sink_QueueEvent(IMFStreamSink *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);

    TRACE("iface %p, event_type %lu, ext_type %s, hr %#lx, value %p.\n",
            iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(stream_sink->event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI stream_sink_GetMediaSink(IMFStreamSink *iface, IMFMediaSink **ret)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);

    TRACE("iface %p, ret %p.\n", iface, ret);

    IMFMediaSink_AddRef((*ret = (IMFMediaSink *)stream_sink->media_sink));

    return S_OK;
}

static HRESULT WINAPI stream_sink_GetIdentifier(IMFStreamSink *iface, DWORD *identifier)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);

    TRACE("iface %p, identifier %p.\n", iface, identifier);

    *identifier = stream_sink->id;

    return S_OK;
}

static HRESULT WINAPI stream_sink_GetMediaTypeHandler(IMFStreamSink *iface, IMFMediaTypeHandler **handler)
{
    FIXME("iface %p, handler %p stub!\n", iface, handler);

    return E_NOTIMPL;
}

static HRESULT WINAPI stream_sink_ProcessSample(IMFStreamSink *iface, IMFSample *sample)
{
    FIXME("iface %p, sample %p stub!\n", iface, sample);

    return E_NOTIMPL;
}

static HRESULT WINAPI stream_sink_PlaceMarker(IMFStreamSink *iface, MFSTREAMSINK_MARKER_TYPE marker_type,
        const PROPVARIANT *marker_value, const PROPVARIANT *context_value)
{
    FIXME("iface %p, marker_type %d, marker_value %p, context_value %p stub!\n",
            iface, marker_type, marker_value, context_value);

    return E_NOTIMPL;
}

static HRESULT WINAPI stream_sink_Flush(IMFStreamSink *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static const IMFStreamSinkVtbl stream_sink_vtbl =
{
    stream_sink_QueryInterface,
    stream_sink_AddRef,
    stream_sink_Release,
    stream_sink_GetEvent,
    stream_sink_BeginGetEvent,
    stream_sink_EndGetEvent,
    stream_sink_QueueEvent,
    stream_sink_GetMediaSink,
    stream_sink_GetIdentifier,
    stream_sink_GetMediaTypeHandler,
    stream_sink_ProcessSample,
    stream_sink_PlaceMarker,
    stream_sink_Flush,
};

static HRESULT stream_sink_create(DWORD stream_sink_id, IMFMediaType *media_type, struct media_sink *media_sink,
        struct stream_sink **out)
{
    struct stream_sink *stream_sink;
    HRESULT hr;

    TRACE("stream_sink_id %#lx, media_type %p, media_sink %p, out %p.\n",
            stream_sink_id, media_type, media_sink, out);

    if (!(stream_sink = calloc(1, sizeof(*stream_sink))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = MFCreateEventQueue(&stream_sink->event_queue)))
    {
        free(stream_sink);
        return hr;
    }

    stream_sink->IMFStreamSink_iface.lpVtbl = &stream_sink_vtbl;
    stream_sink->refcount = 1;
    stream_sink->id = stream_sink_id;
    if (media_type)
        IMFMediaType_AddRef((stream_sink->type = media_type));
    IMFFinalizableMediaSink_AddRef((stream_sink->media_sink = &media_sink->IMFFinalizableMediaSink_iface));

    TRACE("Created stream sink %p.\n", stream_sink);
    *out = stream_sink;

    return S_OK;
}

static struct stream_sink *media_sink_get_stream_sink_by_id(struct media_sink *media_sink, DWORD id)
{
    struct stream_sink *stream_sink;

    LIST_FOR_EACH_ENTRY(stream_sink, &media_sink->stream_sinks, struct stream_sink, entry)
    {
        if (stream_sink->id == id)
            return stream_sink;
    }

    return NULL;
}

static HRESULT WINAPI media_sink_QueryInterface(IMFFinalizableMediaSink *iface, REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFFinalizableMediaSink) ||
            IsEqualIID(riid, &IID_IMFMediaSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else
    {
        WARN("Unsupported interface %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);

    return S_OK;
}

static ULONG WINAPI media_sink_AddRef(IMFFinalizableMediaSink *iface)
{
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(iface);
    ULONG refcount = InterlockedIncrement(&media_sink->refcount);
    TRACE("iface %p, refcount %lu.\n", iface, refcount);
    return refcount;
}

static ULONG WINAPI media_sink_Release(IMFFinalizableMediaSink *iface)
{
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(iface);
    ULONG refcount = InterlockedDecrement(&media_sink->refcount);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IMFFinalizableMediaSink_Shutdown(iface);
        IMFByteStream_Release(media_sink->bytestream);
        media_sink->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&media_sink->cs);
        free(media_sink);
    }

    return refcount;
}

static HRESULT WINAPI media_sink_GetCharacteristics(IMFFinalizableMediaSink *iface, DWORD *flags)
{
    FIXME("iface %p, flags %p stub!\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_AddStreamSink(IMFFinalizableMediaSink *iface, DWORD stream_sink_id,
    IMFMediaType *media_type, IMFStreamSink **stream_sink)
{
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(iface);
    struct stream_sink *object;
    HRESULT hr;

    TRACE("iface %p, stream_sink_id %#lx, media_type %p, stream_sink %p.\n",
            iface, stream_sink_id, media_type, stream_sink);

    EnterCriticalSection(&media_sink->cs);

    if (media_sink_get_stream_sink_by_id(media_sink, stream_sink_id))
    {
        LeaveCriticalSection(&media_sink->cs);
        return MF_E_STREAMSINK_EXISTS;
    }

    if (FAILED(hr = stream_sink_create(stream_sink_id, media_type, media_sink, &object)))
    {
        WARN("Failed to create stream sink, hr %#lx.\n", hr);
        LeaveCriticalSection(&media_sink->cs);
        return hr;
    }

    list_add_tail(&media_sink->stream_sinks, &object->entry);

    LeaveCriticalSection(&media_sink->cs);

    if (stream_sink)
        IMFStreamSink_AddRef((*stream_sink = &object->IMFStreamSink_iface));

    return S_OK;
}

static HRESULT WINAPI media_sink_RemoveStreamSink(IMFFinalizableMediaSink *iface, DWORD stream_sink_id)
{
    FIXME("iface %p, stream_sink_id %#lx stub!\n", iface, stream_sink_id);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_GetStreamSinkCount(IMFFinalizableMediaSink *iface, DWORD *count)
{
    FIXME("iface %p, count %p stub!\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_GetStreamSinkByIndex(IMFFinalizableMediaSink *iface, DWORD index,
        IMFStreamSink **stream)
{
    FIXME("iface %p, index %lu, stream %p stub!\n", iface, index, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_GetStreamSinkById(IMFFinalizableMediaSink *iface, DWORD stream_sink_id,
        IMFStreamSink **stream)
{
    FIXME("iface %p, stream_sink_id %#lx, stream %p stub!\n", iface, stream_sink_id, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_SetPresentationClock(IMFFinalizableMediaSink *iface, IMFPresentationClock *clock)
{
    FIXME("iface %p, clock %p stub!\n", iface, clock);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_GetPresentationClock(IMFFinalizableMediaSink *iface, IMFPresentationClock **clock)
{
    FIXME("iface %p, clock %p stub!\n", iface, clock);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_Shutdown(IMFFinalizableMediaSink *iface)
{
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(iface);
    struct stream_sink *stream_sink, *next;

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&media_sink->cs);

    if (media_sink->shutdown)
    {
        LeaveCriticalSection(&media_sink->cs);
        return MF_E_SHUTDOWN;
    }

    LIST_FOR_EACH_ENTRY_SAFE(stream_sink, next, &media_sink->stream_sinks, struct stream_sink, entry)
    {
        list_remove(&stream_sink->entry);
        IMFMediaEventQueue_Shutdown(stream_sink->event_queue);
        IMFStreamSink_Release(&stream_sink->IMFStreamSink_iface);
    }

    IMFByteStream_Close(media_sink->bytestream);

    media_sink->shutdown = TRUE;

    LeaveCriticalSection(&media_sink->cs);

    return S_OK;
}

static HRESULT WINAPI media_sink_BeginFinalize(IMFFinalizableMediaSink *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    FIXME("iface %p, callback %p, state %p stub!\n", iface, callback, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_EndFinalize(IMFFinalizableMediaSink *iface, IMFAsyncResult *result)
{
    FIXME("iface %p, result %p stub!\n", iface, result);

    return E_NOTIMPL;
}

static const IMFFinalizableMediaSinkVtbl media_sink_vtbl =
{
    media_sink_QueryInterface,
    media_sink_AddRef,
    media_sink_Release,
    media_sink_GetCharacteristics,
    media_sink_AddStreamSink,
    media_sink_RemoveStreamSink,
    media_sink_GetStreamSinkCount,
    media_sink_GetStreamSinkByIndex,
    media_sink_GetStreamSinkById,
    media_sink_SetPresentationClock,
    media_sink_GetPresentationClock,
    media_sink_Shutdown,
    media_sink_BeginFinalize,
    media_sink_EndFinalize,
};

static HRESULT media_sink_create(IMFByteStream *bytestream, struct media_sink **out)
{
    struct media_sink *media_sink;

    TRACE("bytestream %p, out %p.\n", bytestream, out);

    if (!bytestream)
        return E_POINTER;

    if (!(media_sink = calloc(1, sizeof(*media_sink))))
        return E_OUTOFMEMORY;

    media_sink->IMFFinalizableMediaSink_iface.lpVtbl = &media_sink_vtbl;
    media_sink->refcount = 1;
    InitializeCriticalSection(&media_sink->cs);
    media_sink->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": cs");
    IMFByteStream_AddRef((media_sink->bytestream = bytestream));
    list_init(&media_sink->stream_sinks);

    *out = media_sink;
    TRACE("Created media sink %p.\n", media_sink);

    return S_OK;
}

static HRESULT WINAPI sink_class_factory_QueryInterface(IMFSinkClassFactory *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IMFSinkClassFactory)
            || IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFSinkClassFactory_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI sink_class_factory_AddRef(IMFSinkClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI sink_class_factory_Release(IMFSinkClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI sink_class_factory_CreateMediaSink(IMFSinkClassFactory *iface, IMFByteStream *bytestream,
        IMFMediaType *video_type, IMFMediaType *audio_type, IMFMediaSink **out)
{
    IMFFinalizableMediaSink *media_sink_iface;
    struct media_sink *media_sink;
    HRESULT hr;

    TRACE("iface %p, bytestream %p, video_type %p, audio_type %p, out %p.\n",
            iface, bytestream, video_type, audio_type, out);

    if (FAILED(hr = media_sink_create(bytestream, &media_sink)))
        return hr;
    media_sink_iface = &media_sink->IMFFinalizableMediaSink_iface;

    if (video_type)
    {
        if (FAILED(hr = IMFFinalizableMediaSink_AddStreamSink(media_sink_iface, 1, video_type, NULL)))
        {
            IMFFinalizableMediaSink_Shutdown(media_sink_iface);
            IMFFinalizableMediaSink_Release(media_sink_iface);
            return hr;
        }
    }
    if (audio_type)
    {
        if (FAILED(hr = IMFFinalizableMediaSink_AddStreamSink(media_sink_iface, 2, audio_type, NULL)))
        {
            IMFFinalizableMediaSink_Shutdown(media_sink_iface);
            IMFFinalizableMediaSink_Release(media_sink_iface);
            return hr;
        }
    }

    *out = (IMFMediaSink *)media_sink_iface;
    return S_OK;
}

static const IMFSinkClassFactoryVtbl sink_class_factory_vtbl =
{
    sink_class_factory_QueryInterface,
    sink_class_factory_AddRef,
    sink_class_factory_Release,
    sink_class_factory_CreateMediaSink,
};

static IMFSinkClassFactory sink_class_factory = { &sink_class_factory_vtbl };

HRESULT sink_class_factory_create(IUnknown *outer, IUnknown **out)
{
    *out = (IUnknown *)&sink_class_factory;
    return S_OK;
}
