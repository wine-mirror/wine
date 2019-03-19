/*
 *
 * Copyright 2014 Austin English
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
#include "config.h"

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "rpcproxy.h"

#undef INITGUID
#include <guiddef.h>
#include "mfapi.h"
#include "mferror.h"
#include "mfidl.h"
#include "mfreadwrite.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static HINSTANCE mfinstance;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            mfinstance = instance;
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( mfinstance );
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( mfinstance );
}

typedef struct source_reader
{
    IMFSourceReader IMFSourceReader_iface;
    IMFAsyncCallback source_events_callback;
    LONG refcount;
    IMFMediaSource *source;
    IMFPresentationDescriptor *descriptor;
    DWORD first_audio_stream_index;
    DWORD first_video_stream_index;
} srcreader;

struct sink_writer
{
    IMFSinkWriter IMFSinkWriter_iface;
    LONG refcount;
};

static inline srcreader *impl_from_IMFSourceReader(IMFSourceReader *iface)
{
    return CONTAINING_RECORD(iface, srcreader, IMFSourceReader_iface);
}

static struct source_reader *impl_from_source_callback_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct source_reader, source_events_callback);
}

static inline struct sink_writer *impl_from_IMFSinkWriter(IMFSinkWriter *iface)
{
    return CONTAINING_RECORD(iface, struct sink_writer, IMFSinkWriter_iface);
}

static HRESULT WINAPI source_reader_source_events_callback_QueryInterface(IMFAsyncCallback *iface,
        REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI source_reader_source_events_callback_AddRef(IMFAsyncCallback *iface)
{
    struct source_reader *reader = impl_from_source_callback_IMFAsyncCallback(iface);
    return IMFSourceReader_AddRef(&reader->IMFSourceReader_iface);
}

static ULONG WINAPI source_reader_source_events_callback_Release(IMFAsyncCallback *iface)
{
    struct source_reader *reader = impl_from_source_callback_IMFAsyncCallback(iface);
    return IMFSourceReader_Release(&reader->IMFSourceReader_iface);
}

static HRESULT WINAPI source_reader_source_events_callback_GetParameters(IMFAsyncCallback *iface,
        DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI source_reader_source_events_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct source_reader *reader = impl_from_source_callback_IMFAsyncCallback(iface);
    MediaEventType event_type;
    IMFMediaSource *source;
    IMFMediaEvent *event;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, result);

    source = (IMFMediaSource *)IMFAsyncResult_GetStateNoAddRef(result);

    if (FAILED(hr = IMFMediaSource_EndGetEvent(source, result, &event)))
        return hr;

    IMFMediaEvent_GetType(event, &event_type);

    TRACE("Got event %u.\n", event_type);

    IMFMediaEvent_Release(event);

    IMFMediaSource_BeginGetEvent(source, &reader->source_events_callback,
        (IUnknown *)source);

    return S_OK;
}

static const IMFAsyncCallbackVtbl source_events_callback_vtbl =
{
    source_reader_source_events_callback_QueryInterface,
    source_reader_source_events_callback_AddRef,
    source_reader_source_events_callback_Release,
    source_reader_source_events_callback_GetParameters,
    source_reader_source_events_callback_Invoke,
};

static HRESULT WINAPI src_reader_QueryInterface(IMFSourceReader *iface, REFIID riid, void **out)
{
    srcreader *This = impl_from_IMFSourceReader(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), out);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IMFSourceReader))
    {
        *out = &This->IMFSourceReader_iface;
    }
    else
    {
        FIXME("(%s, %p)\n", debugstr_guid(riid), out);
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI src_reader_AddRef(IMFSourceReader *iface)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    ULONG ref = InterlockedIncrement(&This->refcount);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI src_reader_Release(IMFSourceReader *iface)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);
    ULONG refcount = InterlockedDecrement(&reader->refcount);

    TRACE("%p, refcount %d.\n", iface, refcount);

    if (!refcount)
    {
        IMFMediaSource_Shutdown(reader->source);
        if (reader->descriptor)
            IMFPresentationDescriptor_Release(reader->descriptor);
        IMFMediaSource_Release(reader->source);
        heap_free(reader);
    }

    return refcount;
}

static HRESULT WINAPI src_reader_GetStreamSelection(IMFSourceReader *iface, DWORD index, BOOL *selected)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);
    IMFStreamDescriptor *sd;

    TRACE("%p, %#x, %p.\n", iface, index, selected);

    switch (index)
    {
        case MF_SOURCE_READER_FIRST_VIDEO_STREAM:
            index = reader->first_video_stream_index;
            break;
        case MF_SOURCE_READER_FIRST_AUDIO_STREAM:
            index = reader->first_audio_stream_index;
            break;
        default:
            ;
    }

    if (FAILED(IMFPresentationDescriptor_GetStreamDescriptorByIndex(reader->descriptor, index, selected, &sd)))
        return MF_E_INVALIDSTREAMNUMBER;
    IMFStreamDescriptor_Release(sd);

    return S_OK;
}

static HRESULT WINAPI src_reader_SetStreamSelection(IMFSourceReader *iface, DWORD index, BOOL selected)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);
    unsigned int count;
    HRESULT hr;

    TRACE("%p, %#x, %d.\n", iface, index, selected);

    switch (index)
    {
        case MF_SOURCE_READER_FIRST_VIDEO_STREAM:
            index = reader->first_video_stream_index;
            break;
        case MF_SOURCE_READER_FIRST_AUDIO_STREAM:
            index = reader->first_audio_stream_index;
            break;
        case MF_SOURCE_READER_ALL_STREAMS:
            if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorCount(reader->descriptor, &count)))
                return hr;

            for (index = 0; index < count; ++index)
            {
                if (selected)
                    IMFPresentationDescriptor_SelectStream(reader->descriptor, index);
                else
                    IMFPresentationDescriptor_DeselectStream(reader->descriptor, index);
            }

            return S_OK;
        default:
            ;
    }

    if (selected)
        hr = IMFPresentationDescriptor_SelectStream(reader->descriptor, index);
    else
        hr = IMFPresentationDescriptor_DeselectStream(reader->descriptor, index);

    if (FAILED(hr))
        return MF_E_INVALIDSTREAMNUMBER;

    return S_OK;
}

static HRESULT WINAPI src_reader_GetNativeMediaType(IMFSourceReader *iface, DWORD index, DWORD type_index,
            IMFMediaType **type)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);
    IMFMediaTypeHandler *handler;
    IMFStreamDescriptor *sd;
    BOOL selected;
    HRESULT hr;

    TRACE("%p, %#x, %#x, %p.\n", iface, index, type_index, type);

    switch (index)
    {
        case MF_SOURCE_READER_FIRST_VIDEO_STREAM:
            index = reader->first_video_stream_index;
            break;
        case MF_SOURCE_READER_FIRST_AUDIO_STREAM:
            index = reader->first_audio_stream_index;
            break;
        default:
            ;
    }

    if (FAILED(IMFPresentationDescriptor_GetStreamDescriptorByIndex(reader->descriptor, index, &selected, &sd)))
        return MF_E_INVALIDSTREAMNUMBER;

    hr = IMFStreamDescriptor_GetMediaTypeHandler(sd, &handler);
    IMFStreamDescriptor_Release(sd);
    if (FAILED(hr))
        return hr;

    if (type_index == MF_SOURCE_READER_CURRENT_TYPE_INDEX)
        hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, type);
    else
        hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, type_index, type);
    IMFMediaTypeHandler_Release(handler);

    return hr;
}

static HRESULT WINAPI src_reader_GetCurrentMediaType(IMFSourceReader *iface, DWORD index, IMFMediaType **type)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, %p\n", This, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_SetCurrentMediaType(IMFSourceReader *iface, DWORD index, DWORD *reserved,
        IMFMediaType *type)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, %p, %p\n", This, index, reserved, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_SetCurrentPosition(IMFSourceReader *iface, REFGUID format, REFPROPVARIANT position)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, %s, %p\n", This, debugstr_guid(format), position);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_ReadSample(IMFSourceReader *iface, DWORD index,
        DWORD flags, DWORD *actualindex, DWORD *sampleflags, LONGLONG *timestamp,
        IMFSample **sample)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, 0x%08x, %p, %p, %p, %p\n", This, index, flags, actualindex,
          sampleflags, timestamp, sample);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_Flush(IMFSourceReader *iface, DWORD index)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x\n", This, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_GetServiceForStream(IMFSourceReader *iface, DWORD index, REFGUID service,
        REFIID riid, void **object)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);
    IUnknown *obj = NULL;
    HRESULT hr;

    TRACE("%p, %#x, %s, %s, %p\n", iface, index, debugstr_guid(service), debugstr_guid(riid), object);

    switch (index)
    {
        case MF_SOURCE_READER_MEDIASOURCE:
            obj = (IUnknown *)reader->source;
            break;
        default:
            FIXME("Unsupported index %#x.\n", index);
            return E_NOTIMPL;
    }

    if (IsEqualGUID(service, &GUID_NULL))
    {
        hr = IUnknown_QueryInterface(obj, riid, object);
    }
    else
    {
        IMFGetService *gs;

        hr = IUnknown_QueryInterface(obj, &IID_IMFGetService, (void **)&gs);
        if (SUCCEEDED(hr))
        {
            hr = IMFGetService_GetService(gs, service, riid, object);
            IMFGetService_Release(gs);
        }
    }

    return hr;
}

static HRESULT WINAPI src_reader_GetPresentationAttribute(IMFSourceReader *iface, DWORD index,
        REFGUID guid, PROPVARIANT *value)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);
    HRESULT hr;

    TRACE("%p, %#x, %s, %p.\n", iface, index, debugstr_guid(guid), value);

    switch (index)
    {
        case MF_SOURCE_READER_MEDIASOURCE:
            if (IsEqualGUID(guid, &MF_SOURCE_READER_MEDIASOURCE_CHARACTERISTICS))
            {
                DWORD flags;

                if (FAILED(hr = IMFMediaSource_GetCharacteristics(reader->source, &flags)))
                    return hr;

                value->vt = VT_UI4;
                value->u.ulVal = flags;
                return S_OK;
            }
            else
            {
                return IMFPresentationDescriptor_GetItem(reader->descriptor, guid, value);
            }
            break;
        default:
            FIXME("Unsupported index %#x.\n", index);
            return E_NOTIMPL;
    }

    return E_NOTIMPL;
}

struct IMFSourceReaderVtbl srcreader_vtbl =
{
    src_reader_QueryInterface,
    src_reader_AddRef,
    src_reader_Release,
    src_reader_GetStreamSelection,
    src_reader_SetStreamSelection,
    src_reader_GetNativeMediaType,
    src_reader_GetCurrentMediaType,
    src_reader_SetCurrentMediaType,
    src_reader_SetCurrentPosition,
    src_reader_ReadSample,
    src_reader_Flush,
    src_reader_GetServiceForStream,
    src_reader_GetPresentationAttribute
};

static DWORD reader_get_first_stream_index(IMFPresentationDescriptor *descriptor, const GUID *major)
{
    unsigned int count, i;
    BOOL selected;
    HRESULT hr;
    GUID guid;

    if (FAILED(IMFPresentationDescriptor_GetStreamDescriptorCount(descriptor, &count)))
        return MF_SOURCE_READER_INVALID_STREAM_INDEX;

    for (i = 0; i < count; ++i)
    {
        IMFMediaTypeHandler *handler;
        IMFStreamDescriptor *sd;

        if (SUCCEEDED(IMFPresentationDescriptor_GetStreamDescriptorByIndex(descriptor, i, &selected, &sd)))
        {
            hr = IMFStreamDescriptor_GetMediaTypeHandler(sd, &handler);
            IMFStreamDescriptor_Release(sd);
            if (SUCCEEDED(hr))
            {
                hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
                IMFMediaTypeHandler_Release(handler);
                if (SUCCEEDED(hr) && IsEqualGUID(&guid, major))
                {
                    return i;
                }
            }
        }
    }

    return MF_SOURCE_READER_INVALID_STREAM_INDEX;
}

static HRESULT create_source_reader_from_source(IMFMediaSource *source, IMFAttributes *attributes,
        REFIID riid, void **out)
{
    srcreader *object;
    HRESULT hr;

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFSourceReader_iface.lpVtbl = &srcreader_vtbl;
    object->source_events_callback.lpVtbl = &source_events_callback_vtbl;
    object->refcount = 1;
    object->source = source;
    IMFMediaSource_AddRef(object->source);

    if (FAILED(hr = IMFMediaSource_CreatePresentationDescriptor(object->source, &object->descriptor)))
        goto failed;

    /* At least one major type has to be set. */
    object->first_audio_stream_index = reader_get_first_stream_index(object->descriptor, &MFMediaType_Audio);
    object->first_video_stream_index = reader_get_first_stream_index(object->descriptor, &MFMediaType_Video);

    if (object->first_audio_stream_index == MF_SOURCE_READER_INVALID_STREAM_INDEX &&
            object->first_video_stream_index == MF_SOURCE_READER_INVALID_STREAM_INDEX)
    {
        hr = MF_E_ATTRIBUTENOTFOUND;
    }

    if (FAILED(hr = IMFMediaSource_BeginGetEvent(object->source, &object->source_events_callback,
            (IUnknown *)object->source)))
    {
        goto failed;
    }

    hr = IMFSourceReader_QueryInterface(&object->IMFSourceReader_iface, riid, out);

failed:
    IMFSourceReader_Release(&object->IMFSourceReader_iface);
    return hr;
}

static HRESULT create_source_reader_from_stream(IMFByteStream *stream, IMFAttributes *attributes,
        REFIID riid, void **out)
{
    IMFSourceResolver *resolver;
    MF_OBJECT_TYPE obj_type;
    IMFMediaSource *source;
    HRESULT hr;

    if (FAILED(hr = MFCreateSourceResolver(&resolver)))
        return hr;

    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
            &obj_type, (IUnknown **)&source);
    IMFSourceResolver_Release(resolver);
    if (FAILED(hr))
        return hr;

    hr = create_source_reader_from_source(source, attributes, riid, out);
    IMFMediaSource_Release(source);
    return hr;
}

static HRESULT create_source_reader_from_url(const WCHAR *url, IMFAttributes *attributes, REFIID riid, void **out)
{
    IMFSourceResolver *resolver;
    IUnknown *object = NULL;
    MF_OBJECT_TYPE obj_type;
    IMFMediaSource *source;
    HRESULT hr;

    if (FAILED(hr = MFCreateSourceResolver(&resolver)))
        return hr;

    hr = IMFSourceResolver_CreateObjectFromURL(resolver, url, MF_RESOLUTION_MEDIASOURCE, NULL, &obj_type,
            &object);
    if (SUCCEEDED(hr))
    {
        switch (obj_type)
        {
            case MF_OBJECT_BYTESTREAM:
                hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, (IMFByteStream *)object, NULL,
                        MF_RESOLUTION_MEDIASOURCE, NULL, &obj_type, (IUnknown **)&source);
                break;
            case MF_OBJECT_MEDIASOURCE:
                source = (IMFMediaSource *)object;
                IMFMediaSource_AddRef(source);
                break;
            default:
                WARN("Unknown object type %d.\n", obj_type);
                hr = E_UNEXPECTED;
        }
        IUnknown_Release(object);
    }

    IMFSourceResolver_Release(resolver);
    if (FAILED(hr))
        return hr;

    hr = create_source_reader_from_source(source, attributes, riid, out);
    IMFMediaSource_Release(source);
    return hr;
}

static HRESULT WINAPI sink_writer_QueryInterface(IMFSinkWriter *iface, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFSinkWriter) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFSinkWriter_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI sink_writer_AddRef(IMFSinkWriter *iface)
{
    struct sink_writer *writer = impl_from_IMFSinkWriter(iface);
    ULONG refcount = InterlockedIncrement(&writer->refcount);

    TRACE("%p, %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI sink_writer_Release(IMFSinkWriter *iface)
{
    struct sink_writer *writer = impl_from_IMFSinkWriter(iface);
    ULONG refcount = InterlockedDecrement(&writer->refcount);

    TRACE("%p, %u.\n", iface, refcount);

    if (!refcount)
    {
        heap_free(writer);
    }

    return refcount;
}

static HRESULT WINAPI sink_writer_AddStream(IMFSinkWriter *iface, IMFMediaType *type, DWORD *index)
{
    FIXME("%p, %p, %p.\n", iface, type, index);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_SetInputMediaType(IMFSinkWriter *iface, DWORD index, IMFMediaType *type,
        IMFAttributes *parameters)
{
    FIXME("%p, %u, %p, %p.\n", iface, index, type, parameters);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_BeginWriting(IMFSinkWriter *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_WriteSample(IMFSinkWriter *iface, DWORD index, IMFSample *sample)
{
    FIXME("%p, %u, %p.\n", iface, index, sample);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_SendStreamTick(IMFSinkWriter *iface, DWORD index, LONGLONG timestamp)
{
    FIXME("%p, %u, %s.\n", iface, index, wine_dbgstr_longlong(timestamp));

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_PlaceMarker(IMFSinkWriter *iface, DWORD index, void *context)
{
    FIXME("%p, %u, %p.\n", iface, index, context);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_NotifyEndOfSegment(IMFSinkWriter *iface, DWORD index)
{
    FIXME("%p, %u.\n", iface, index);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_Flush(IMFSinkWriter *iface, DWORD index)
{
    FIXME("%p, %u.\n", iface, index);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_Finalize(IMFSinkWriter *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_GetServiceForStream(IMFSinkWriter *iface, DWORD index, REFGUID service,
        REFIID riid, void **object)
{
    FIXME("%p, %u, %s, %s, %p.\n", iface, index, debugstr_guid(service), debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI sink_writer_GetStatistics(IMFSinkWriter *iface, DWORD index, MF_SINK_WRITER_STATISTICS *stats)
{
    FIXME("%p, %u, %p.\n", iface, index, stats);

    return E_NOTIMPL;
}

static const IMFSinkWriterVtbl sink_writer_vtbl =
{
    sink_writer_QueryInterface,
    sink_writer_AddRef,
    sink_writer_Release,
    sink_writer_AddStream,
    sink_writer_SetInputMediaType,
    sink_writer_BeginWriting,
    sink_writer_WriteSample,
    sink_writer_SendStreamTick,
    sink_writer_PlaceMarker,
    sink_writer_NotifyEndOfSegment,
    sink_writer_Flush,
    sink_writer_Finalize,
    sink_writer_GetServiceForStream,
    sink_writer_GetStatistics,
};

static HRESULT create_sink_writer_from_sink(IMFMediaSink *sink, IMFAttributes *attributes,
        REFIID riid, void **out)
{
    struct sink_writer *object;
    HRESULT hr;

    object = heap_alloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFSinkWriter_iface.lpVtbl = &sink_writer_vtbl;
    object->refcount = 1;

    hr = IMFSinkWriter_QueryInterface(&object->IMFSinkWriter_iface, riid, out);
    IMFSinkWriter_Release(&object->IMFSinkWriter_iface);
    return hr;
}

static HRESULT create_sink_writer_from_stream(IMFByteStream *stream, IMFAttributes *attributes,
        REFIID riid, void **out)
{
    struct sink_writer *object;
    HRESULT hr;

    object = heap_alloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFSinkWriter_iface.lpVtbl = &sink_writer_vtbl;
    object->refcount = 1;

    hr = IMFSinkWriter_QueryInterface(&object->IMFSinkWriter_iface, riid, out);
    IMFSinkWriter_Release(&object->IMFSinkWriter_iface);
    return hr;
}

/***********************************************************************
 *      MFCreateSinkWriterFromMediaSink (mfreadwrite.@)
 */
HRESULT WINAPI MFCreateSinkWriterFromMediaSink(IMFMediaSink *sink, IMFAttributes *attributes, IMFSinkWriter **writer)
{
    TRACE("%p, %p, %p.\n", sink, attributes, writer);

    return create_sink_writer_from_sink(sink, attributes, &IID_IMFSinkWriter, (void **)writer);
}

/***********************************************************************
 *      MFCreateSourceReaderFromByteStream (mfreadwrite.@)
 */
HRESULT WINAPI MFCreateSourceReaderFromByteStream(IMFByteStream *stream, IMFAttributes *attributes,
        IMFSourceReader **reader)
{
    TRACE("%p, %p, %p.\n", stream, attributes, reader);

    return create_source_reader_from_stream(stream, attributes, &IID_IMFSourceReader, (void **)reader);
}

/***********************************************************************
 *      MFCreateSourceReaderFromMediaSource (mfreadwrite.@)
 */
HRESULT WINAPI MFCreateSourceReaderFromMediaSource(IMFMediaSource *source, IMFAttributes *attributes,
                                                   IMFSourceReader **reader)
{
    TRACE("%p, %p, %p.\n", source, attributes, reader);

    return create_source_reader_from_source(source, attributes, &IID_IMFSourceReader, (void **)reader);
}

/***********************************************************************
 *      MFCreateSourceReaderFromURL (mfreadwrite.@)
 */
HRESULT WINAPI MFCreateSourceReaderFromURL(const WCHAR *url, IMFAttributes *attributes, IMFSourceReader **reader)
{
    TRACE("%s, %p, %p.\n", debugstr_w(url), attributes, reader);

    return create_source_reader_from_url(url, attributes, &IID_IMFSourceReader, (void **)reader);
}

static HRESULT WINAPI readwrite_factory_QueryInterface(IMFReadWriteClassFactory *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IMFReadWriteClassFactory) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFReadWriteClassFactory_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI readwrite_factory_AddRef(IMFReadWriteClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI readwrite_factory_Release(IMFReadWriteClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI readwrite_factory_CreateInstanceFromURL(IMFReadWriteClassFactory *iface, REFCLSID clsid,
        const WCHAR *url, IMFAttributes *attributes, REFIID riid, void **out)
{
    TRACE("%s, %s, %p, %s, %p.\n", debugstr_guid(clsid), debugstr_w(url), attributes, debugstr_guid(riid), out);

    if (IsEqualGUID(clsid, &CLSID_MFSourceReader))
    {
        return create_source_reader_from_url(url, attributes, &IID_IMFSourceReader, out);
    }

    FIXME("Unsupported %s.\n", debugstr_guid(clsid));

    return E_NOTIMPL;
}

static HRESULT WINAPI readwrite_factory_CreateInstanceFromObject(IMFReadWriteClassFactory *iface, REFCLSID clsid,
        IUnknown *unk, IMFAttributes *attributes, REFIID riid, void **out)
{
    HRESULT hr;

    TRACE("%s, %p, %p, %s, %p.\n", debugstr_guid(clsid), unk, attributes, debugstr_guid(riid), out);

    if (IsEqualGUID(clsid, &CLSID_MFSourceReader))
    {
        IMFMediaSource *source = NULL;
        IMFByteStream *stream = NULL;

        hr = IUnknown_QueryInterface(unk, &IID_IMFByteStream, (void **)&stream);
        if (FAILED(hr))
            hr = IUnknown_QueryInterface(unk, &IID_IMFMediaSource, (void **)&source);

        if (stream)
            hr = create_source_reader_from_stream(stream, attributes, riid, out);
        else if (source)
            hr = create_source_reader_from_source(source, attributes, riid, out);

        if (source)
            IMFMediaSource_Release(source);
        if (stream)
            IMFByteStream_Release(stream);

        return hr;
    }
    else if (IsEqualGUID(clsid, &CLSID_MFSinkWriter))
    {
        IMFByteStream *stream = NULL;
        IMFMediaSink *sink = NULL;

        hr = IUnknown_QueryInterface(unk, &IID_IMFByteStream, (void **)&stream);
        if (FAILED(hr))
            hr = IUnknown_QueryInterface(unk, &IID_IMFMediaSink, (void **)&sink);

        if (stream)
            hr = create_sink_writer_from_stream(stream, attributes, riid, out);
        else if (sink)
            hr = create_sink_writer_from_sink(sink, attributes, riid, out);

        if (sink)
            IMFMediaSink_Release(sink);
        if (stream)
            IMFByteStream_Release(stream);

        return hr;
    }
    else
    {
        WARN("Unsupported class %s.\n", debugstr_guid(clsid));
        *out = NULL;
        return E_FAIL;
    }
}

static const IMFReadWriteClassFactoryVtbl readwrite_factory_vtbl =
{
    readwrite_factory_QueryInterface,
    readwrite_factory_AddRef,
    readwrite_factory_Release,
    readwrite_factory_CreateInstanceFromURL,
    readwrite_factory_CreateInstanceFromObject,
};

static IMFReadWriteClassFactory readwrite_factory = { &readwrite_factory_vtbl };

static HRESULT WINAPI classfactory_QueryInterface(IClassFactory *iface, REFIID riid, void **out)
{
    TRACE("%s, %p.\n", debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IClassFactory) ||
            IsEqualGUID(riid, &IID_IUnknown))
    {
        IClassFactory_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("interface %s not implemented\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI classfactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI classfactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI classfactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", outer, debugstr_guid(riid), out);

    *out = NULL;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    return IMFReadWriteClassFactory_QueryInterface(&readwrite_factory, riid, out);
}

static HRESULT WINAPI classfactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("%d.\n", dolock);
    return S_OK;
}

static const struct IClassFactoryVtbl classfactoryvtbl =
{
    classfactory_QueryInterface,
    classfactory_AddRef,
    classfactory_Release,
    classfactory_CreateInstance,
    classfactory_LockServer,
};

static IClassFactory classfactory = { &classfactoryvtbl };

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    TRACE("%s, %s, %p.\n", debugstr_guid(clsid), debugstr_guid(riid), out);

    if (IsEqualGUID(clsid, &CLSID_MFReadWriteClassFactory))
        return IClassFactory_QueryInterface(&classfactory, riid, out);

    WARN("Unsupported class %s.\n", debugstr_guid(clsid));
    *out = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}
