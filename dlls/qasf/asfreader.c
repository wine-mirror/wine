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

struct asf_stream
{
    struct strmbase_source source;
    DWORD index;
};

struct asf_reader
{
    struct strmbase_filter filter;
    IFileSourceFilter IFileSourceFilter_iface;

    AM_MEDIA_TYPE media_type;
    WCHAR *file_name;

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
    FreeMediaType(&filter->media_type);
    IWMReaderCallback_Release(filter->callback);
    IWMReader_Release(filter->reader);

    strmbase_filter_cleanup(&filter->filter);
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

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = asf_reader_get_pin,
    .filter_destroy = asf_reader_destroy,
    .filter_query_interface = asf_reader_query_interface,
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

    if (filter->file_name)
        return E_FAIL;

    if (!(filter->file_name = wcsdup(file_name)))
        return E_OUTOFMEMORY;

    if (media_type)
        CopyMediaType(&filter->media_type, media_type);

    if (FAILED(hr = IWMReader_Open(filter->reader, filter->file_name, filter->callback, NULL)))
        WARN("Failed to open WM reader, hr %#lx.\n", hr);

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
        media_type->majortype = filter->media_type.majortype;
        media_type->subtype = filter->media_type.subtype;
        media_type->lSampleSize = filter->media_type.lSampleSize;
        media_type->pUnk = filter->media_type.pUnk;
        media_type->cbFormat = filter->media_type.cbFormat;
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

static HRESULT WINAPI reader_callback_OnStatus(IWMReaderCallback *iface, WMT_STATUS status, HRESULT hr,
        WMT_ATTR_DATATYPE type, BYTE *value, void *context)
{
    struct asf_reader *filter = impl_from_IWMReaderCallback(iface)->filter;
    AM_MEDIA_TYPE stream_media_type = {0};
    DWORD i, stream_count;
    WCHAR name[MAX_PATH];

    TRACE("iface %p, status %d, hr %#lx, type %d, value %p, context %p.\n",
            iface, status, hr, type, value, context);

    switch (status)
    {
        case WMT_OPENED:
            if (FAILED(hr))
            {
                ERR("Failed to open WMReader, hr %#lx.\n", hr);
                break;
            }
            if (FAILED(hr = IWMReader_GetOutputCount(filter->reader, &stream_count)))
            {
                ERR("Failed to get WMReader output count, hr %#lx.\n", hr);
                break;
            }
            if (stream_count > ARRAY_SIZE(filter->streams))
            {
                FIXME("Found %lu streams, not supported!\n", stream_count);
                stream_count = ARRAY_SIZE(filter->streams);
            }

            EnterCriticalSection(&filter->filter.filter_cs);
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
            }
            filter->stream_count = stream_count;
            LeaveCriticalSection(&filter->filter.filter_cs);

            BaseFilterImpl_IncrementPinVersion(&filter->filter);
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
    FIXME("iface %p, output %lu, time %I64u, duration %I64u, flags %#lx, sample %p, context %p stub!\n",
            iface, output, time, duration, flags, sample, context);
    return E_NOTIMPL;
}

static const IWMReaderCallbackVtbl reader_callback_vtbl =
{
    reader_callback_QueryInterface,
    reader_callback_AddRef,
    reader_callback_Release,
    reader_callback_OnStatus,
    reader_callback_OnSample,
};

static HRESULT asf_callback_create(struct asf_reader *filter, IWMReaderCallback **out)
{
    struct asf_callback *callback;

    if (!(callback = calloc(1, sizeof(*callback))))
        return E_OUTOFMEMORY;

    callback->IWMReaderCallback_iface.lpVtbl = &reader_callback_vtbl;
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

    TRACE("Created WM ASF reader %p.\n", object);
    *out = &object->filter.IUnknown_inner;

    return S_OK;
}
