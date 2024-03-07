/*
 * WM ASF reader
 *
 * Copyright 2020 Jactry Zeng for CodeWeavers
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

#include "qasf_private.h"

#include "mediaobj.h"
#include "propsys.h"
#include "initguid.h"
#include "wmsdkidl.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct buffer
{
    INSSBuffer INSSBuffer_iface;
    LONG refcount;
    IMediaSample *sample;
};

static struct buffer *impl_from_INSSBuffer(INSSBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct buffer, INSSBuffer_iface);
}

static HRESULT WINAPI buffer_QueryInterface(INSSBuffer *iface, REFIID iid, void **out)
{
    struct buffer *impl = impl_from_INSSBuffer(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_INSSBuffer))
        *out = &impl->INSSBuffer_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI buffer_AddRef(INSSBuffer *iface)
{
    struct buffer *impl = impl_from_INSSBuffer(iface);
    ULONG ref = InterlockedIncrement(&impl->refcount);
    TRACE("iface %p increasing refcount to %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI buffer_Release(INSSBuffer *iface)
{
    struct buffer *impl = impl_from_INSSBuffer(iface);
    ULONG ref = InterlockedDecrement(&impl->refcount);

    TRACE("iface %p decreasing refcount to %lu.\n", iface, ref);

    if (!ref)
    {
        IMediaSample_Release(impl->sample);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI buffer_GetLength(INSSBuffer *iface, DWORD *size)
{
    struct buffer *impl = impl_from_INSSBuffer(iface);
    TRACE("iface %p, size %p.\n", iface, size);
    *size = IMediaSample_GetActualDataLength(impl->sample);
    return S_OK;
}

static HRESULT WINAPI buffer_SetLength(INSSBuffer *iface, DWORD size)
{
    struct buffer *impl = impl_from_INSSBuffer(iface);
    TRACE("iface %p, size %lu.\n", iface, size);
    return IMediaSample_SetActualDataLength(impl->sample, size);
}

static HRESULT WINAPI buffer_GetMaxLength(INSSBuffer *iface, DWORD *size)
{
    struct buffer *impl = impl_from_INSSBuffer(iface);
    TRACE("iface %p, size %p.\n", iface, size);
    *size = IMediaSample_GetSize(impl->sample);
    return S_OK;
}

static HRESULT WINAPI buffer_GetBuffer(INSSBuffer *iface, BYTE **data)
{
    struct buffer *impl = impl_from_INSSBuffer(iface);
    TRACE("iface %p, data %p.\n", iface, data);
    return IMediaSample_GetPointer(impl->sample, data);
}

static HRESULT WINAPI buffer_GetBufferAndLength(INSSBuffer *iface, BYTE **data, DWORD *size)
{
    struct buffer *impl = impl_from_INSSBuffer(iface);
    TRACE("iface %p, data %p, size %p.\n", iface, data, size);
    *size = IMediaSample_GetSize(impl->sample);
    return IMediaSample_GetPointer(impl->sample, data);
}

static const INSSBufferVtbl buffer_vtbl =
{
    buffer_QueryInterface,
    buffer_AddRef,
    buffer_Release,
    buffer_GetLength,
    buffer_SetLength,
    buffer_GetMaxLength,
    buffer_GetBuffer,
    buffer_GetBufferAndLength,
};

static HRESULT buffer_create(IMediaSample *sample, INSSBuffer **out)
{
    struct buffer *buffer;

    if (!(buffer = calloc(1, sizeof(struct buffer))))
        return E_OUTOFMEMORY;

    buffer->INSSBuffer_iface.lpVtbl = &buffer_vtbl;
    buffer->refcount = 1;
    buffer->sample = sample;

    *out = &buffer->INSSBuffer_iface;
    TRACE("Created buffer %p for sample %p\n", *out, sample);

    return S_OK;
}

static struct buffer *unsafe_impl_from_INSSBuffer(INSSBuffer *iface)
{
    if (iface->lpVtbl != &buffer_vtbl) return NULL;
    return impl_from_INSSBuffer(iface);
}

struct asf_stream
{
    struct strmbase_source source;
    struct SourceSeeking seek;
    DWORD index;
};

struct asf_reader
{
    struct strmbase_filter filter;
    IFileSourceFilter IFileSourceFilter_iface;

    WCHAR *file_name;

    HRESULT result;
    WMT_STATUS status;
    CRITICAL_SECTION status_cs;
    CONDITION_VARIABLE status_cv;

    IWMReaderCallback *callback;
    IWMReader *reader;

    UINT stream_count;
    struct asf_stream streams[16];
};

static inline struct asf_stream *impl_from_strmbase_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, struct asf_stream, source.pin);
}

static inline struct asf_reader *asf_reader_from_asf_stream(struct asf_stream *stream)
{
    return CONTAINING_RECORD(stream, struct asf_reader, streams[stream->index]);
}

static HRESULT asf_stream_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *media_type)
{
    struct asf_stream *stream = impl_from_strmbase_pin(iface);
    struct asf_reader *filter = asf_reader_from_asf_stream(stream);
    IWMOutputMediaProps *props;
    WM_MEDIA_TYPE *mt;
    DWORD size, i = 0;
    HRESULT hr;

    TRACE("iface %p, media_type %p.\n", iface, media_type);

    if (FAILED(hr = IWMReader_GetOutputFormat(filter->reader, stream->index, i, &props)))
        return hr;
    if (FAILED(hr = IWMOutputMediaProps_GetMediaType(props, NULL, &size)))
    {
        IWMOutputMediaProps_Release(props);
        return hr;
    }
    if (!(mt = malloc(size)))
    {
        IWMOutputMediaProps_Release(props);
        return E_OUTOFMEMORY;
    }

    do
    {
        if (SUCCEEDED(hr = IWMOutputMediaProps_GetMediaType(props, mt, &size))
                && IsEqualGUID(&mt->majortype, &media_type->majortype)
                && IsEqualGUID(&mt->subtype, &media_type->subtype))
        {
            IWMOutputMediaProps_Release(props);
            break;
        }

        IWMOutputMediaProps_Release(props);
    } while (SUCCEEDED(hr = IWMReader_GetOutputFormat(filter->reader, stream->index, ++i, &props)));

    free(mt);
    return hr;
}

static HRESULT asf_stream_get_media_type(struct strmbase_pin *iface, unsigned int index, AM_MEDIA_TYPE *media_type)
{
    struct asf_stream *stream = impl_from_strmbase_pin(iface);
    struct asf_reader *filter = asf_reader_from_asf_stream(stream);
    IWMOutputMediaProps *props;
    WM_MEDIA_TYPE *mt;
    DWORD size;
    HRESULT hr;

    TRACE("iface %p, index %u, media_type %p.\n", iface, index, media_type);

    if (FAILED(IWMReader_GetOutputFormat(filter->reader, stream->index, index, &props)))
        return VFW_S_NO_MORE_ITEMS;
    if (FAILED(hr = IWMOutputMediaProps_GetMediaType(props, NULL, &size)))
    {
        IWMOutputMediaProps_Release(props);
        return hr;
    }
    if (!(mt = malloc(size)))
    {
        IWMOutputMediaProps_Release(props);
        return E_OUTOFMEMORY;
    }

    hr = IWMOutputMediaProps_GetMediaType(props, mt, &size);
    if (SUCCEEDED(hr))
        hr = CopyMediaType(media_type, (AM_MEDIA_TYPE *)mt);

    free(mt);
    IWMOutputMediaProps_Release(props);
    return hr;
}

static HRESULT asf_stream_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct asf_stream *stream = impl_from_strmbase_pin(iface);

    if (IsEqualGUID(iid, &IID_IMediaSeeking))
        *out = &stream->seek.IMediaSeeking_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static inline struct asf_stream *impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, struct asf_stream, seek.IMediaSeeking_iface);
}

static HRESULT WINAPI media_seeking_ChangeCurrent(IMediaSeeking *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT WINAPI media_seeking_ChangeStop(IMediaSeeking *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT WINAPI media_seeking_ChangeRate(IMediaSeeking *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT WINAPI media_seeking_QueryInterface(IMediaSeeking *iface, REFIID riid, void **ppv)
{
    struct asf_stream *impl = impl_from_IMediaSeeking(iface);
    return IUnknown_QueryInterface(&impl->source.pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI media_seeking_AddRef(IMediaSeeking *iface)
{
    struct asf_stream *impl = impl_from_IMediaSeeking(iface);
    return IUnknown_AddRef(&impl->source.pin.IPin_iface);
}

static ULONG WINAPI media_seeking_Release(IMediaSeeking *iface)
{
    struct asf_stream *impl = impl_from_IMediaSeeking(iface);
    return IUnknown_Release(&impl->source.pin.IPin_iface);
}

static HRESULT WINAPI media_seeking_SetPositions(IMediaSeeking *iface,
        LONGLONG *current, DWORD current_flags, LONGLONG *stop, DWORD stop_flags)
{
    FIXME("iface %p, current %s, current_flags %#lx, stop %s, stop_flags %#lx stub!\n",
            iface, current ? debugstr_time(*current) : "<null>", current_flags,
            stop ? debugstr_time(*stop) : "<null>", stop_flags);
    return SourceSeekingImpl_SetPositions(iface, current, current_flags, stop, stop_flags);
}

static const IMediaSeekingVtbl media_seeking_vtbl =
{
    media_seeking_QueryInterface,
    media_seeking_AddRef,
    media_seeking_Release,
    SourceSeekingImpl_GetCapabilities,
    SourceSeekingImpl_CheckCapabilities,
    SourceSeekingImpl_IsFormatSupported,
    SourceSeekingImpl_QueryPreferredFormat,
    SourceSeekingImpl_GetTimeFormat,
    SourceSeekingImpl_IsUsingTimeFormat,
    SourceSeekingImpl_SetTimeFormat,
    SourceSeekingImpl_GetDuration,
    SourceSeekingImpl_GetStopPosition,
    SourceSeekingImpl_GetCurrentPosition,
    SourceSeekingImpl_ConvertTimeFormat,
    media_seeking_SetPositions,
    SourceSeekingImpl_GetPositions,
    SourceSeekingImpl_GetAvailable,
    SourceSeekingImpl_SetRate,
    SourceSeekingImpl_GetRate,
    SourceSeekingImpl_GetPreroll,
};

static inline struct asf_reader *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct asf_reader, filter);
}

static struct strmbase_pin *asf_reader_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct asf_reader *filter = impl_from_strmbase_filter(iface);
    struct strmbase_pin *pin = NULL;

    TRACE("iface %p, index %u.\n", iface, index);

    EnterCriticalSection(&filter->filter.filter_cs);
    if (index < filter->stream_count)
        pin = &filter->streams[index].source.pin;
    LeaveCriticalSection(&filter->filter.filter_cs);

    return pin;
}

static void asf_reader_destroy(struct strmbase_filter *iface)
{
    struct asf_reader *filter = impl_from_strmbase_filter(iface);
    struct strmbase_source *source;

    while (filter->stream_count--)
    {
        source = &filter->streams[filter->stream_count].source;
        if (source->pin.peer) IPin_Disconnect(source->pin.peer);
        IPin_Disconnect(&source->pin.IPin_iface);
        strmbase_source_cleanup(source);
    }

    free(filter->file_name);
    IWMReaderCallback_Release(filter->callback);
    IWMReader_Release(filter->reader);

    strmbase_filter_cleanup(&filter->filter);

    filter->status_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&filter->status_cs);

    free(filter);
}

static HRESULT asf_reader_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct asf_reader *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IFileSourceFilter))
    {
        *out = &filter->IFileSourceFilter_iface;
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static HRESULT asf_reader_init_stream(struct strmbase_filter *iface)
{
    struct asf_reader *filter = impl_from_strmbase_filter(iface);
    WMT_STREAM_SELECTION selections[ARRAY_SIZE(filter->streams)];
    WORD stream_numbers[ARRAY_SIZE(filter->streams)];
    IWMReaderAdvanced *reader_advanced;
    HRESULT hr = S_OK;
    int i;

    TRACE("iface %p\n", iface);

    if (FAILED(hr = IWMReader_QueryInterface(filter->reader, &IID_IWMReaderAdvanced, (void **)&reader_advanced)))
        return hr;

    for (i = 0; i < filter->stream_count; ++i)
    {
        struct asf_stream *stream = filter->streams + i;
        IWMOutputMediaProps *props;

        stream_numbers[i] = i + 1;
        selections[i] = WMT_OFF;

        if (!stream->source.pin.peer)
            continue;

        if (FAILED(hr = IMemAllocator_Commit(stream->source.pAllocator)))
        {
            WARN("Failed to commit stream %u allocator, hr %#lx\n", i, hr);
            break;
        }

        if (FAILED(hr = IWMReaderAdvanced_SetAllocateForOutput(reader_advanced, i, TRUE)))
        {
            WARN("Failed to enable allocation for stream %u, hr %#lx\n", i, hr);
            break;
        }

        if (FAILED(hr = IWMReader_GetOutputFormat(filter->reader, stream->index, 0, &props)))
        {
            WARN("Failed to get stream %u output format, hr %#lx\n", i, hr);
            break;
        }

        hr = IWMOutputMediaProps_SetMediaType(props, (WM_MEDIA_TYPE *)&stream->source.pin.mt);
        if (SUCCEEDED(hr))
            hr = IWMReader_SetOutputProps(filter->reader, stream->index, props);
        IWMOutputMediaProps_Release(props);
        if (FAILED(hr))
        {
            WARN("Failed to set stream %u output format, hr %#lx\n", i, hr);
            break;
        }

        if (FAILED(hr = IPin_NewSegment(stream->source.pin.peer, 0, 0, 1)))
        {
            WARN("Failed to start stream %u new segment, hr %#lx\n", i, hr);
            break;
        }

        selections[i] = WMT_ON;
    }

    if (SUCCEEDED(hr) && FAILED(hr = IWMReaderAdvanced_SetStreamsSelected(reader_advanced,
            filter->stream_count, stream_numbers, selections)))
        WARN("Failed to set reader %p stream selection, hr %#lx\n", filter->reader, hr);

    IWMReaderAdvanced_Release(reader_advanced);

    if (FAILED(hr))
        return hr;

    EnterCriticalSection(&filter->status_cs);
    if (SUCCEEDED(hr = IWMReader_Start(filter->reader, 0, 0, 1, NULL)))
    {
        filter->status = -1;
        while (filter->status != WMT_STARTED)
            SleepConditionVariableCS(&filter->status_cv, &filter->status_cs, INFINITE);
        hr = filter->result;
    }
    LeaveCriticalSection(&filter->status_cs);

    if (FAILED(hr))
        WARN("Failed to start WMReader %p, hr %#lx\n", filter->reader, hr);

    return hr;
}

static HRESULT asf_reader_cleanup_stream(struct strmbase_filter *iface)
{
    struct asf_reader *filter = impl_from_strmbase_filter(iface);
    HRESULT hr = S_OK;
    int i;

    TRACE("iface %p\n", iface);

    EnterCriticalSection(&filter->status_cs);
    if (SUCCEEDED(hr = IWMReader_Stop(filter->reader)))
    {
        filter->status = -1;
        while (filter->status != WMT_STOPPED)
            SleepConditionVariableCS(&filter->status_cv, &filter->status_cs, INFINITE);
        hr = filter->result;
    }
    LeaveCriticalSection(&filter->status_cs);

    if (FAILED(hr))
        WARN("Failed to stop WMReader %p, hr %#lx\n", filter->reader, hr);

    for (i = 0; i < filter->stream_count; ++i)
    {
        struct asf_stream *stream = filter->streams + i;

        if (!stream->source.pin.peer)
            continue;

        if (FAILED(hr = IMemAllocator_Decommit(stream->source.pAllocator)))
        {
            WARN("Failed to decommit stream %u allocator, hr %#lx\n", i, hr);
            break;
        }
    }

    return hr;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = asf_reader_get_pin,
    .filter_destroy = asf_reader_destroy,
    .filter_query_interface = asf_reader_query_interface,
    .filter_init_stream = asf_reader_init_stream,
    .filter_cleanup_stream = asf_reader_cleanup_stream,
};

static HRESULT WINAPI asf_reader_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *allocator, ALLOCATOR_PROPERTIES *req_props)
{
    struct asf_stream *stream = impl_from_strmbase_pin(&iface->pin);
    unsigned int buffer_size = 16384;
    ALLOCATOR_PROPERTIES ret_props;

    TRACE("iface %p, allocator %p, req_props %p.\n", iface, allocator, req_props);

    if (IsEqualGUID(&stream->source.pin.mt.formattype, &FORMAT_VideoInfo))
    {
        VIDEOINFOHEADER *format = (VIDEOINFOHEADER *)stream->source.pin.mt.pbFormat;
        buffer_size = format->bmiHeader.biSizeImage;
    }
    else if (IsEqualGUID(&stream->source.pin.mt.formattype, &FORMAT_WaveFormatEx)
            && (IsEqualGUID(&stream->source.pin.mt.subtype, &MEDIASUBTYPE_PCM)
            || IsEqualGUID(&stream->source.pin.mt.subtype, &MEDIASUBTYPE_IEEE_FLOAT)))
    {
        WAVEFORMATEX *format = (WAVEFORMATEX *)stream->source.pin.mt.pbFormat;
        buffer_size = format->nAvgBytesPerSec;
    }

    req_props->cBuffers = max(req_props->cBuffers, 1);
    req_props->cbBuffer = max(req_props->cbBuffer, buffer_size);
    req_props->cbAlign = max(req_props->cbAlign, 1);
    return IMemAllocator_SetProperties(allocator, req_props, &ret_props);
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_accept = asf_stream_query_accept,
    .base.pin_get_media_type = asf_stream_get_media_type,
    .base.pin_query_interface = asf_stream_query_interface,
    .pfnDecideAllocator = BaseOutputPinImpl_DecideAllocator,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideBufferSize = asf_reader_DecideBufferSize,
};

static inline struct asf_reader *impl_from_IFileSourceFilter(IFileSourceFilter *iface)
{
    return CONTAINING_RECORD(iface, struct asf_reader, IFileSourceFilter_iface);
}

static HRESULT WINAPI file_source_QueryInterface(IFileSourceFilter *iface, REFIID iid, void **out)
{
    struct asf_reader *filter = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_QueryInterface(&filter->filter.IBaseFilter_iface, iid, out);
}

static ULONG WINAPI file_source_AddRef(IFileSourceFilter *iface)
{
    struct asf_reader *filter = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_AddRef(&filter->filter.IBaseFilter_iface);
}

static ULONG WINAPI file_source_Release(IFileSourceFilter *iface)
{
    struct asf_reader *filter = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_Release(&filter->filter.IBaseFilter_iface);
}

static HRESULT WINAPI file_source_Load(IFileSourceFilter *iface, LPCOLESTR file_name, const AM_MEDIA_TYPE *media_type)
{
    struct asf_reader *filter = impl_from_IFileSourceFilter(iface);
    HRESULT hr;

    TRACE("filter %p, file_name %s, media_type %p.\n", filter, debugstr_w(file_name), media_type);
    strmbase_dump_media_type(media_type);

    if (!file_name)
        return E_POINTER;

    EnterCriticalSection(&filter->filter.filter_cs);

    if (filter->file_name || !(filter->file_name = wcsdup(file_name)))
    {
        LeaveCriticalSection(&filter->filter.filter_cs);
        return E_FAIL;
    }

    EnterCriticalSection(&filter->status_cs);
    if (SUCCEEDED(hr = IWMReader_Open(filter->reader, filter->file_name, filter->callback, NULL)))
    {
        filter->status = -1;
        while (filter->status != WMT_OPENED)
            SleepConditionVariableCS(&filter->status_cv, &filter->status_cs, INFINITE);
        hr = filter->result;
    }
    LeaveCriticalSection(&filter->status_cs);

    if (FAILED(hr))
        WARN("Failed to open WM reader, hr %#lx.\n", hr);

    LeaveCriticalSection(&filter->filter.filter_cs);

    return S_OK;
}

static HRESULT WINAPI file_source_GetCurFile(IFileSourceFilter *iface, LPOLESTR *file_name, AM_MEDIA_TYPE *media_type)
{
    struct asf_reader *filter = impl_from_IFileSourceFilter(iface);

    TRACE("filter %p, file_name %p, media_type %p.\n", filter, file_name, media_type);

    if (!file_name)
        return E_POINTER;
    *file_name = NULL;

    if (media_type)
    {
        media_type->majortype = GUID_NULL;
        media_type->subtype = GUID_NULL;
        media_type->lSampleSize = 0;
        media_type->pUnk = NULL;
        media_type->cbFormat = 0;
    }

    if (filter->file_name)
    {
        *file_name = CoTaskMemAlloc((wcslen(filter->file_name) + 1) * sizeof(WCHAR));
        wcscpy(*file_name, filter->file_name);
    }

    return S_OK;
}

static const IFileSourceFilterVtbl file_source_vtbl =
{
    file_source_QueryInterface,
    file_source_AddRef,
    file_source_Release,
    file_source_Load,
    file_source_GetCurFile,
};

struct asf_callback
{
    IWMReaderCallback IWMReaderCallback_iface;
    IWMReaderCallbackAdvanced IWMReaderCallbackAdvanced_iface;
    LONG ref;

    struct asf_reader *filter;
};

static inline struct asf_callback *impl_from_IWMReaderCallback(IWMReaderCallback *iface)
{
    return CONTAINING_RECORD(iface, struct asf_callback, IWMReaderCallback_iface);
}

static HRESULT WINAPI reader_callback_QueryInterface(IWMReaderCallback *iface, const IID *iid, void **out)
{
    struct asf_callback *callback = impl_from_IWMReaderCallback(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IWMStatusCallback)
            || IsEqualGUID(iid, &IID_IWMReaderCallback))
        *out = &callback->IWMReaderCallback_iface;
    else if (IsEqualGUID(iid, &IID_IWMReaderCallbackAdvanced))
        *out = &callback->IWMReaderCallbackAdvanced_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI reader_callback_AddRef(IWMReaderCallback *iface)
{
    struct asf_callback *callback = impl_from_IWMReaderCallback(iface);
    ULONG ref = InterlockedIncrement(&callback->ref);

    TRACE("%p increasing ref to %lu.\n", callback, ref);

    return ref;
}

static ULONG WINAPI reader_callback_Release(IWMReaderCallback *iface)
{
    struct asf_callback *callback = impl_from_IWMReaderCallback(iface);
    ULONG ref = InterlockedDecrement(&callback->ref);

    TRACE("%p decreasing ref to %lu.\n", callback, ref);

    if (!ref)
        free(callback);

    return ref;
}

static HRESULT WINAPI reader_callback_OnStatus(IWMReaderCallback *iface, WMT_STATUS status, HRESULT result,
        WMT_ATTR_DATATYPE type, BYTE *value, void *context)
{
    struct asf_reader *filter = impl_from_IWMReaderCallback(iface)->filter;
    AM_MEDIA_TYPE stream_media_type = {{0}};
    IWMHeaderInfo *header_info;
    DWORD i, stream_count;
    WCHAR name[MAX_PATH];
    QWORD duration;
    HRESULT hr;

    TRACE("iface %p, status %d, result %#lx, type %d, value %p, context %p.\n",
            iface, status, result, type, value, context);

    switch (status)
    {
        case WMT_OPENED:
            if (FAILED(hr = IWMReader_GetOutputCount(filter->reader, &stream_count)))
            {
                ERR("Failed to get WMReader output count, hr %#lx.\n", hr);
                stream_count = 0;
            }
            if (stream_count > ARRAY_SIZE(filter->streams))
            {
                FIXME("Found %lu streams, not supported!\n", stream_count);
                stream_count = ARRAY_SIZE(filter->streams);
            }

            if (FAILED(hr = IWMReader_QueryInterface(filter->reader, &IID_IWMHeaderInfo,
                    (void **)&header_info)))
                duration = 0;
            else
            {
                WMT_ATTR_DATATYPE type = WMT_TYPE_QWORD;
                WORD index = 0, size = sizeof(duration);

                if (FAILED(IWMHeaderInfo_GetAttributeByName(header_info, &index, L"Duration",
                        &type, (BYTE *)&duration, &size )))
                    duration = 0;
                IWMHeaderInfo_Release(header_info);
            }

            for (i = 0; i < stream_count; ++i)
            {
                struct asf_stream *stream = filter->streams + i;

                if (FAILED(hr = asf_stream_get_media_type(&stream->source.pin, 0, &stream_media_type)))
                    WARN("Failed to get stream media type, hr %#lx.\n", hr);
                if (IsEqualGUID(&stream_media_type.majortype, &MEDIATYPE_Video))
                    swprintf(name, ARRAY_SIZE(name), L"Raw Video %u", stream->index);
                else
                    swprintf(name, ARRAY_SIZE(name), L"Raw Audio %u", stream->index);
                FreeMediaType(&stream_media_type);

                strmbase_source_init(&stream->source, &filter->filter, name, &source_ops);
                strmbase_seeking_init(&stream->seek, &media_seeking_vtbl, media_seeking_ChangeStop,
                        media_seeking_ChangeCurrent, media_seeking_ChangeRate);
                stream->seek.llCurrent = 0;
                stream->seek.llDuration = duration;
                stream->seek.llStop = duration;
            }
            filter->stream_count = stream_count;
            BaseFilterImpl_IncrementPinVersion(&filter->filter);

            EnterCriticalSection(&filter->status_cs);
            filter->result = result;
            filter->status = WMT_OPENED;
            LeaveCriticalSection(&filter->status_cs);
            WakeConditionVariable(&filter->status_cv);
            break;

        case WMT_END_OF_STREAMING:
            for (i = 0; i < filter->stream_count; ++i)
            {
                struct asf_stream *stream = filter->streams + i;

                if (!stream->source.pin.peer)
                    continue;

                IPin_EndOfStream(stream->source.pin.peer);
            }
            break;

        case WMT_STARTED:
            EnterCriticalSection(&filter->status_cs);
            filter->result = result;
            filter->status = WMT_STARTED;
            LeaveCriticalSection(&filter->status_cs);
            WakeConditionVariable(&filter->status_cv);
            break;

        case WMT_STOPPED:
            EnterCriticalSection(&filter->status_cs);
            filter->result = result;
            filter->status = WMT_STOPPED;
            LeaveCriticalSection(&filter->status_cs);
            WakeConditionVariable(&filter->status_cv);
            break;

        default:
            WARN("Ignoring status %#x.\n", status);
            break;
    }

    return S_OK;
}

static HRESULT WINAPI reader_callback_OnSample(IWMReaderCallback *iface, DWORD output, QWORD time,
        QWORD duration, DWORD flags, INSSBuffer *sample, void *context)
{
    struct asf_reader *filter = impl_from_IWMReaderCallback(iface)->filter;
    REFERENCE_TIME start_time = time, end_time = time + duration;
    struct asf_stream *stream = filter->streams + output;
    struct buffer *buffer;
    HRESULT hr = S_OK;

    TRACE("iface %p, output %lu, time %I64u, duration %I64u, flags %#lx, sample %p, context %p.\n",
            iface, output, time, duration, flags, sample, context);

    if (!stream->source.pin.peer)
    {
        WARN("Output %lu pin is not connected, discarding %p.\n", output, sample);
        return S_OK;
    }

    if (!(buffer = unsafe_impl_from_INSSBuffer(sample)))
        WARN("Unexpected buffer iface %p, discarding.\n", sample);
    else
    {
        IMediaSample_SetTime(buffer->sample, &start_time, &end_time);
        IMediaSample_SetDiscontinuity(buffer->sample, !!(flags & WM_SF_DISCONTINUITY));
        IMediaSample_SetSyncPoint(buffer->sample, !!(flags & WM_SF_CLEANPOINT));

        hr = IMemInputPin_Receive(stream->source.pMemInputPin, buffer->sample);

        TRACE("Receive returned hr %#lx.\n", hr);
    }

    return hr;
}

static const IWMReaderCallbackVtbl reader_callback_vtbl =
{
    reader_callback_QueryInterface,
    reader_callback_AddRef,
    reader_callback_Release,
    reader_callback_OnStatus,
    reader_callback_OnSample,
};

static inline struct asf_callback *impl_from_IWMReaderCallbackAdvanced(IWMReaderCallbackAdvanced *iface)
{
    return CONTAINING_RECORD(iface, struct asf_callback, IWMReaderCallbackAdvanced_iface);
}

static HRESULT WINAPI reader_callback_advanced_QueryInterface(IWMReaderCallbackAdvanced *iface, REFIID riid, LPVOID * ppv)
{
    struct asf_callback *impl = impl_from_IWMReaderCallbackAdvanced(iface);
    return IUnknown_QueryInterface(&impl->IWMReaderCallback_iface, riid, ppv);
}

static ULONG WINAPI reader_callback_advanced_AddRef(IWMReaderCallbackAdvanced *iface)
{
    struct asf_callback *impl = impl_from_IWMReaderCallbackAdvanced(iface);
    return IUnknown_AddRef(&impl->IWMReaderCallback_iface);
}

static ULONG WINAPI reader_callback_advanced_Release(IWMReaderCallbackAdvanced *iface)
{
    struct asf_callback *impl = impl_from_IWMReaderCallbackAdvanced(iface);
    return IUnknown_Release(&impl->IWMReaderCallback_iface);
}

static HRESULT WINAPI reader_callback_advanced_OnStreamSample(IWMReaderCallbackAdvanced *iface,
        WORD stream, QWORD time, QWORD duration, DWORD flags, INSSBuffer *sample, void *context)
{
    FIXME("iface %p, stream %u, time %I64u, duration %I64u, flags %#lx, sample %p, context %p stub!\n",
            iface, stream, time, duration, flags, sample, context);
    return E_NOTIMPL;
}

static HRESULT WINAPI reader_callback_advanced_OnTime(IWMReaderCallbackAdvanced *iface,
        QWORD time, void *context)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI reader_callback_advanced_OnStreamSelection(IWMReaderCallbackAdvanced *iface,
        WORD count, WORD *stream_numbers, WMT_STREAM_SELECTION *selections, void *context)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI reader_callback_advanced_OnOutputPropsChanged(IWMReaderCallbackAdvanced *iface,
        DWORD output, WM_MEDIA_TYPE *mt, void *context)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI reader_callback_advanced_AllocateForStream(IWMReaderCallbackAdvanced *iface,
        WORD stream, DWORD size, INSSBuffer **out, void *context)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI reader_callback_advanced_AllocateForOutput(IWMReaderCallbackAdvanced *iface,
        DWORD output, DWORD size, INSSBuffer **out, void *context)
{
    struct asf_reader *filter = impl_from_IWMReaderCallbackAdvanced(iface)->filter;
    struct asf_stream *stream = filter->streams + output;
    IMediaSample *sample;
    HRESULT hr;

    TRACE("iface %p, output %lu, size %lu, out %p, context %p.\n", iface, output, size, out, context);

    *out = NULL;

    if (!stream->source.pin.peer)
        return VFW_E_NOT_CONNECTED;

    if (FAILED(hr = IMemAllocator_GetBuffer(stream->source.pAllocator, &sample, NULL, NULL, 0)))
    {
        WARN("Failed to get a sample, hr %#lx.\n", hr);
        return hr;
    }

    if (size > IMediaSample_GetSize(sample))
    {
        WARN("Allocated media sample is too small, size %lu.\n", size);
        IMediaSample_Release(sample);
        return VFW_E_BUFFER_OVERFLOW;
    }

    return buffer_create(sample, out);
}

static const IWMReaderCallbackAdvancedVtbl reader_callback_advanced_vtbl =
{
    reader_callback_advanced_QueryInterface,
    reader_callback_advanced_AddRef,
    reader_callback_advanced_Release,
    reader_callback_advanced_OnStreamSample,
    reader_callback_advanced_OnTime,
    reader_callback_advanced_OnStreamSelection,
    reader_callback_advanced_OnOutputPropsChanged,
    reader_callback_advanced_AllocateForStream,
    reader_callback_advanced_AllocateForOutput,
};

static HRESULT asf_callback_create(struct asf_reader *filter, IWMReaderCallback **out)
{
    struct asf_callback *callback;

    if (!(callback = calloc(1, sizeof(*callback))))
        return E_OUTOFMEMORY;

    callback->IWMReaderCallback_iface.lpVtbl = &reader_callback_vtbl;
    callback->IWMReaderCallbackAdvanced_iface.lpVtbl = &reader_callback_advanced_vtbl;
    callback->filter = filter;
    callback->ref = 1;

    *out = &callback->IWMReaderCallback_iface;
    return S_OK;
}

HRESULT asf_reader_create(IUnknown *outer, IUnknown **out)
{
    struct asf_reader *object;
    HRESULT hr;
    int i;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = WMCreateReader(NULL, 0, &object->reader)))
    {
        free(object);
        return hr;
    }
    if (FAILED(hr = asf_callback_create(object, &object->callback)))
    {
        IWMReader_Release(object->reader);
        free(object);
        return hr;
    }

    for (i = 0; i < ARRAY_SIZE(object->streams); ++i) object->streams[i].index = i;
    strmbase_filter_init(&object->filter, outer, &CLSID_WMAsfReader, &filter_ops);
    object->IFileSourceFilter_iface.lpVtbl = &file_source_vtbl;

    InitializeCriticalSectionEx(&object->status_cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    object->status_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": status_cs");

    TRACE("Created WM ASF reader %p.\n", object);
    *out = &object->filter.IUnknown_inner;

    return S_OK;
}
