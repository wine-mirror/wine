/*
 * Copyright 2012 Austin English
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

WINE_DEFAULT_DEBUG_CHANNEL(wmvcore);

static struct wm_stream *get_stream_by_output_number(struct wm_reader *reader, DWORD output)
{
    if (output < reader->stream_count)
        return &reader->streams[output];
    WARN("Invalid output number %lu.\n", output);
    return NULL;
}

struct output_props
{
    IWMOutputMediaProps IWMOutputMediaProps_iface;
    LONG refcount;

    AM_MEDIA_TYPE mt;
};

static inline struct output_props *impl_from_IWMOutputMediaProps(IWMOutputMediaProps *iface)
{
    return CONTAINING_RECORD(iface, struct output_props, IWMOutputMediaProps_iface);
}

static HRESULT WINAPI output_props_QueryInterface(IWMOutputMediaProps *iface, REFIID iid, void **out)
{
    struct output_props *props = impl_from_IWMOutputMediaProps(iface);

    TRACE("props %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IWMOutputMediaProps))
        *out = &props->IWMOutputMediaProps_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI output_props_AddRef(IWMOutputMediaProps *iface)
{
    struct output_props *props = impl_from_IWMOutputMediaProps(iface);
    ULONG refcount = InterlockedIncrement(&props->refcount);

    TRACE("%p increasing refcount to %lu.\n", props, refcount);

    return refcount;
}

static ULONG WINAPI output_props_Release(IWMOutputMediaProps *iface)
{
    struct output_props *props = impl_from_IWMOutputMediaProps(iface);
    ULONG refcount = InterlockedDecrement(&props->refcount);

    TRACE("%p decreasing refcount to %lu.\n", props, refcount);

    if (!refcount)
        free(props);

    return refcount;
}

static HRESULT WINAPI output_props_GetType(IWMOutputMediaProps *iface, GUID *major_type)
{
    const struct output_props *props = impl_from_IWMOutputMediaProps(iface);

    TRACE("iface %p, major_type %p.\n", iface, major_type);

    *major_type = props->mt.majortype;
    return S_OK;
}

static HRESULT WINAPI output_props_GetMediaType(IWMOutputMediaProps *iface, WM_MEDIA_TYPE *mt, DWORD *size)
{
    const struct output_props *props = impl_from_IWMOutputMediaProps(iface);
    const DWORD req_size = *size;

    TRACE("iface %p, mt %p, size %p.\n", iface, mt, size);

    *size = sizeof(*mt) + props->mt.cbFormat;
    if (!mt)
        return S_OK;
    if (req_size < *size)
        return ASF_E_BUFFERTOOSMALL;

    strmbase_dump_media_type(&props->mt);

    memcpy(mt, &props->mt, sizeof(*mt));
    memcpy(mt + 1, props->mt.pbFormat, props->mt.cbFormat);
    mt->pbFormat = (BYTE *)(mt + 1);
    return S_OK;
}

static HRESULT WINAPI output_props_SetMediaType(IWMOutputMediaProps *iface, WM_MEDIA_TYPE *mt)
{
    const struct output_props *props = impl_from_IWMOutputMediaProps(iface);

    TRACE("iface %p, mt %p.\n", iface, mt);

    if (!mt)
        return E_POINTER;

    if (!IsEqualGUID(&props->mt.majortype, &mt->majortype))
        return E_FAIL;

    FreeMediaType((AM_MEDIA_TYPE *)&props->mt);
    return CopyMediaType((AM_MEDIA_TYPE *)&props->mt, (AM_MEDIA_TYPE *)mt);
}

static HRESULT WINAPI output_props_GetStreamGroupName(IWMOutputMediaProps *iface, WCHAR *name, WORD *len)
{
    FIXME("iface %p, name %p, len %p, stub!\n", iface, name, len);
    return E_NOTIMPL;
}

static HRESULT WINAPI output_props_GetConnectionName(IWMOutputMediaProps *iface, WCHAR *name, WORD *len)
{
    FIXME("iface %p, name %p, len %p, stub!\n", iface, name, len);
    return E_NOTIMPL;
}

static const struct IWMOutputMediaPropsVtbl output_props_vtbl =
{
    output_props_QueryInterface,
    output_props_AddRef,
    output_props_Release,
    output_props_GetType,
    output_props_GetMediaType,
    output_props_SetMediaType,
    output_props_GetStreamGroupName,
    output_props_GetConnectionName,
};

static struct output_props *unsafe_impl_from_IWMOutputMediaProps(IWMOutputMediaProps *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &output_props_vtbl);
    return impl_from_IWMOutputMediaProps(iface);
}

static IWMOutputMediaProps *output_props_create(const struct wg_format *format)
{
    struct output_props *object;

    if (!(object = calloc(1, sizeof(*object))))
        return NULL;
    object->IWMOutputMediaProps_iface.lpVtbl = &output_props_vtbl;
    object->refcount = 1;

    if (!amt_from_wg_format(&object->mt, format, true))
    {
        free(object);
        return NULL;
    }

    TRACE("Created output properties %p.\n", object);
    return &object->IWMOutputMediaProps_iface;
}

struct buffer
{
    INSSBuffer INSSBuffer_iface;
    LONG refcount;

    DWORD size, capacity;
    BYTE data[1];
};

static struct buffer *impl_from_INSSBuffer(INSSBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct buffer, INSSBuffer_iface);
}

static HRESULT WINAPI buffer_QueryInterface(INSSBuffer *iface, REFIID iid, void **out)
{
    struct buffer *buffer = impl_from_INSSBuffer(iface);

    TRACE("buffer %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_INSSBuffer))
        *out = &buffer->INSSBuffer_iface;
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
    struct buffer *buffer = impl_from_INSSBuffer(iface);
    ULONG refcount = InterlockedIncrement(&buffer->refcount);

    TRACE("%p increasing refcount to %lu.\n", buffer, refcount);

    return refcount;
}

static ULONG WINAPI buffer_Release(INSSBuffer *iface)
{
    struct buffer *buffer = impl_from_INSSBuffer(iface);
    ULONG refcount = InterlockedDecrement(&buffer->refcount);

    TRACE("%p decreasing refcount to %lu.\n", buffer, refcount);

    if (!refcount)
        free(buffer);

    return refcount;
}

static HRESULT WINAPI buffer_GetLength(INSSBuffer *iface, DWORD *size)
{
    FIXME("iface %p, size %p, stub!\n", iface, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI buffer_SetLength(INSSBuffer *iface, DWORD size)
{
    struct buffer *buffer = impl_from_INSSBuffer(iface);

    TRACE("iface %p, size %lu.\n", buffer, size);

    if (size > buffer->capacity)
        return E_INVALIDARG;

    buffer->size = size;
    return S_OK;
}

static HRESULT WINAPI buffer_GetMaxLength(INSSBuffer *iface, DWORD *size)
{
    struct buffer *buffer = impl_from_INSSBuffer(iface);

    TRACE("buffer %p, size %p.\n", buffer, size);

    *size = buffer->capacity;
    return S_OK;
}

static HRESULT WINAPI buffer_GetBuffer(INSSBuffer *iface, BYTE **data)
{
    struct buffer *buffer = impl_from_INSSBuffer(iface);

    TRACE("buffer %p, data %p.\n", buffer, data);

    *data = buffer->data;
    return S_OK;
}

static HRESULT WINAPI buffer_GetBufferAndLength(INSSBuffer *iface, BYTE **data, DWORD *size)
{
    struct buffer *buffer = impl_from_INSSBuffer(iface);

    TRACE("buffer %p, data %p, size %p.\n", buffer, data, size);

    *size = buffer->size;
    *data = buffer->data;
    return S_OK;
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

struct stream_config
{
    IWMStreamConfig IWMStreamConfig_iface;
    IWMMediaProps IWMMediaProps_iface;
    LONG refcount;

    const struct wm_stream *stream;
};

static struct stream_config *impl_from_IWMStreamConfig(IWMStreamConfig *iface)
{
    return CONTAINING_RECORD(iface, struct stream_config, IWMStreamConfig_iface);
}

static HRESULT WINAPI stream_config_QueryInterface(IWMStreamConfig *iface, REFIID iid, void **out)
{
    struct stream_config *config = impl_from_IWMStreamConfig(iface);

    TRACE("config %p, iid %s, out %p.\n", config, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IWMStreamConfig))
        *out = &config->IWMStreamConfig_iface;
    else if (IsEqualGUID(iid, &IID_IWMMediaProps))
        *out = &config->IWMMediaProps_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI stream_config_AddRef(IWMStreamConfig *iface)
{
    struct stream_config *config = impl_from_IWMStreamConfig(iface);
    ULONG refcount = InterlockedIncrement(&config->refcount);

    TRACE("%p increasing refcount to %lu.\n", config, refcount);

    return refcount;
}

static ULONG WINAPI stream_config_Release(IWMStreamConfig *iface)
{
    struct stream_config *config = impl_from_IWMStreamConfig(iface);
    ULONG refcount = InterlockedDecrement(&config->refcount);

    TRACE("%p decreasing refcount to %lu.\n", config, refcount);

    if (!refcount)
    {
        IWMProfile3_Release(&config->stream->reader->IWMProfile3_iface);
        free(config);
    }

    return refcount;
}

static HRESULT WINAPI stream_config_GetStreamType(IWMStreamConfig *iface, GUID *type)
{
    struct stream_config *config = impl_from_IWMStreamConfig(iface);
    struct wm_reader *reader = config->stream->reader;
    AM_MEDIA_TYPE mt;

    TRACE("config %p, type %p.\n", config, type);

    EnterCriticalSection(&reader->cs);

    if (!amt_from_wg_format(&mt, &config->stream->format, true))
    {
        LeaveCriticalSection(&reader->cs);
        return E_OUTOFMEMORY;
    }

    *type = mt.majortype;
    FreeMediaType(&mt);

    LeaveCriticalSection(&reader->cs);

    return S_OK;
}

static HRESULT WINAPI stream_config_GetStreamNumber(IWMStreamConfig *iface, WORD *number)
{
    struct stream_config *config = impl_from_IWMStreamConfig(iface);

    TRACE("config %p, number %p.\n", config, number);

    *number = config->stream->index + 1;
    return S_OK;
}

static HRESULT WINAPI stream_config_SetStreamNumber(IWMStreamConfig *iface, WORD number)
{
    FIXME("iface %p, number %u, stub!\n", iface, number);
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_config_GetStreamName(IWMStreamConfig *iface, WCHAR *name, WORD *len)
{
    FIXME("iface %p, name %p, len %p, stub!\n", iface, name, len);
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_config_SetStreamName(IWMStreamConfig *iface, const WCHAR *name)
{
    FIXME("iface %p, name %s, stub!\n", iface, debugstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_config_GetConnectionName(IWMStreamConfig *iface, WCHAR *name, WORD *len)
{
    FIXME("iface %p, name %p, len %p, stub!\n", iface, name, len);
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_config_SetConnectionName(IWMStreamConfig *iface, const WCHAR *name)
{
    FIXME("iface %p, name %s, stub!\n", iface, debugstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_config_GetBitrate(IWMStreamConfig *iface, DWORD *bitrate)
{
    FIXME("iface %p, bitrate %p, stub!\n", iface, bitrate);
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_config_SetBitrate(IWMStreamConfig *iface, DWORD bitrate)
{
    FIXME("iface %p, bitrate %lu, stub!\n", iface, bitrate);
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_config_GetBufferWindow(IWMStreamConfig *iface, DWORD *window)
{
    FIXME("iface %p, window %p, stub!\n", iface, window);
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_config_SetBufferWindow(IWMStreamConfig *iface, DWORD window)
{
    FIXME("iface %p, window %lu, stub!\n", iface, window);
    return E_NOTIMPL;
}

static const IWMStreamConfigVtbl stream_config_vtbl =
{
    stream_config_QueryInterface,
    stream_config_AddRef,
    stream_config_Release,
    stream_config_GetStreamType,
    stream_config_GetStreamNumber,
    stream_config_SetStreamNumber,
    stream_config_GetStreamName,
    stream_config_SetStreamName,
    stream_config_GetConnectionName,
    stream_config_SetConnectionName,
    stream_config_GetBitrate,
    stream_config_SetBitrate,
    stream_config_GetBufferWindow,
    stream_config_SetBufferWindow,
};

static struct stream_config *impl_from_IWMMediaProps(IWMMediaProps *iface)
{
    return CONTAINING_RECORD(iface, struct stream_config, IWMMediaProps_iface);
}

static HRESULT WINAPI stream_props_QueryInterface(IWMMediaProps *iface, REFIID iid, void **out)
{
    struct stream_config *config = impl_from_IWMMediaProps(iface);
    return IWMStreamConfig_QueryInterface(&config->IWMStreamConfig_iface, iid, out);
}

static ULONG WINAPI stream_props_AddRef(IWMMediaProps *iface)
{
    struct stream_config *config = impl_from_IWMMediaProps(iface);
    return IWMStreamConfig_AddRef(&config->IWMStreamConfig_iface);
}

static ULONG WINAPI stream_props_Release(IWMMediaProps *iface)
{
    struct stream_config *config = impl_from_IWMMediaProps(iface);
    return IWMStreamConfig_Release(&config->IWMStreamConfig_iface);
}

static HRESULT WINAPI stream_props_GetType(IWMMediaProps *iface, GUID *major_type)
{
    FIXME("iface %p, major_type %p, stub!\n", iface, major_type);
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_props_GetMediaType(IWMMediaProps *iface, WM_MEDIA_TYPE *mt, DWORD *size)
{
    struct stream_config *config = impl_from_IWMMediaProps(iface);
    const DWORD req_size = *size;
    AM_MEDIA_TYPE stream_mt;

    TRACE("iface %p, mt %p, size %p.\n", iface, mt, size);

    if (!amt_from_wg_format(&stream_mt, &config->stream->format, true))
        return E_OUTOFMEMORY;

    *size = sizeof(stream_mt) + stream_mt.cbFormat;
    if (!mt)
        return S_OK;
    if (req_size < *size)
        return ASF_E_BUFFERTOOSMALL;

    strmbase_dump_media_type(&stream_mt);

    memcpy(mt, &stream_mt, sizeof(*mt));
    memcpy(mt + 1, stream_mt.pbFormat, stream_mt.cbFormat);
    mt->pbFormat = (BYTE *)(mt + 1);
    return S_OK;
}

static HRESULT WINAPI stream_props_SetMediaType(IWMMediaProps *iface, WM_MEDIA_TYPE *mt)
{
    FIXME("iface %p, mt %p, stub!\n", iface, mt);
    return E_NOTIMPL;
}

static const IWMMediaPropsVtbl stream_props_vtbl =
{
    stream_props_QueryInterface,
    stream_props_AddRef,
    stream_props_Release,
    stream_props_GetType,
    stream_props_GetMediaType,
    stream_props_SetMediaType,
};

static DWORD CALLBACK read_thread(void *arg)
{
    struct wm_reader *reader = arg;
    IStream *stream = reader->source_stream;
    HANDLE file = reader->file;
    size_t buffer_size = 4096;
    uint64_t file_size;
    void *data;

    if (!(data = malloc(buffer_size)))
        return 0;

    if (file)
    {
        LARGE_INTEGER size;

        GetFileSizeEx(file, &size);
        file_size = size.QuadPart;
    }
    else
    {
        STATSTG stat;

        IStream_Stat(stream, &stat, STATFLAG_NONAME);
        file_size = stat.cbSize.QuadPart;
    }

    TRACE("Starting read thread for reader %p.\n", reader);

    while (!reader->read_thread_shutdown)
    {
        LARGE_INTEGER large_offset;
        uint64_t offset;
        ULONG ret_size;
        uint32_t size;
        HRESULT hr;

        if (!wg_parser_get_next_read_offset(reader->wg_parser, &offset, &size))
            continue;

        if (offset >= file_size)
            size = 0;
        else if (offset + size >= file_size)
            size = file_size - offset;

        if (!size)
        {
            wg_parser_push_data(reader->wg_parser, data, 0);
            continue;
        }

        if (!array_reserve(&data, &buffer_size, size, 1))
        {
            free(data);
            return 0;
        }

        ret_size = 0;

        large_offset.QuadPart = offset;
        if (file)
        {
            if (!SetFilePointerEx(file, large_offset, NULL, FILE_BEGIN)
                    || !ReadFile(file, data, size, &ret_size, NULL))
            {
                ERR("Failed to read %u bytes at offset %I64u, error %lu.\n", size, offset, GetLastError());
                wg_parser_push_data(reader->wg_parser, NULL, 0);
                continue;
            }
        }
        else
        {
            if (SUCCEEDED(hr = IStream_Seek(stream, large_offset, STREAM_SEEK_SET, NULL)))
                hr = IStream_Read(stream, data, size, &ret_size);
            if (FAILED(hr))
            {
                ERR("Failed to read %u bytes at offset %I64u, hr %#lx.\n", size, offset, hr);
                wg_parser_push_data(reader->wg_parser, NULL, 0);
                continue;
            }
        }

        if (ret_size != size)
            ERR("Unexpected short read: requested %u bytes, got %lu.\n", size, ret_size);
        wg_parser_push_data(reader->wg_parser, data, ret_size);
    }

    free(data);
    TRACE("Reader is shutting down; exiting.\n");
    return 0;
}

static struct wm_reader *impl_from_IWMProfile3(IWMProfile3 *iface)
{
    return CONTAINING_RECORD(iface, struct wm_reader, IWMProfile3_iface);
}

static HRESULT WINAPI profile_QueryInterface(IWMProfile3 *iface, REFIID iid, void **out)
{
    struct wm_reader *reader = impl_from_IWMProfile3(iface);
    return IUnknown_QueryInterface(reader->outer, iid, out);
}

static ULONG WINAPI profile_AddRef(IWMProfile3 *iface)
{
    struct wm_reader *reader = impl_from_IWMProfile3(iface);
    return IUnknown_AddRef(reader->outer);
}

static ULONG WINAPI profile_Release(IWMProfile3 *iface)
{
    struct wm_reader *reader = impl_from_IWMProfile3(iface);
    return IUnknown_Release(reader->outer);
}

static HRESULT WINAPI profile_GetVersion(IWMProfile3 *iface, WMT_VERSION *version)
{
    FIXME("iface %p, version %p, stub!\n", iface, version);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_GetName(IWMProfile3 *iface, WCHAR *name, DWORD *length)
{
    FIXME("iface %p, name %p, length %p, stub!\n", iface, name, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_SetName(IWMProfile3 *iface, const WCHAR *name)
{
    FIXME("iface %p, name %s, stub!\n", iface, debugstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_GetDescription(IWMProfile3 *iface, WCHAR *description, DWORD *length)
{
    FIXME("iface %p, description %p, length %p, stub!\n", iface, description, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_SetDescription(IWMProfile3 *iface, const WCHAR *description)
{
    FIXME("iface %p, description %s, stub!\n", iface, debugstr_w(description));
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_GetStreamCount(IWMProfile3 *iface, DWORD *count)
{
    struct wm_reader *reader = impl_from_IWMProfile3(iface);

    TRACE("reader %p, count %p.\n", reader, count);

    if (!count)
        return E_INVALIDARG;

    EnterCriticalSection(&reader->cs);
    *count = reader->stream_count;
    LeaveCriticalSection(&reader->cs);
    return S_OK;
}

static HRESULT WINAPI profile_GetStream(IWMProfile3 *iface, DWORD index, IWMStreamConfig **config)
{
    struct wm_reader *reader = impl_from_IWMProfile3(iface);
    struct stream_config *object;

    TRACE("reader %p, index %lu, config %p.\n", reader, index, config);

    EnterCriticalSection(&reader->cs);

    if (index >= reader->stream_count)
    {
        LeaveCriticalSection(&reader->cs);
        WARN("Index %lu exceeds stream count %u; returning E_INVALIDARG.\n", index, reader->stream_count);
        return E_INVALIDARG;
    }

    if (!(object = calloc(1, sizeof(*object))))
    {
        LeaveCriticalSection(&reader->cs);
        return E_OUTOFMEMORY;
    }

    object->IWMStreamConfig_iface.lpVtbl = &stream_config_vtbl;
    object->IWMMediaProps_iface.lpVtbl = &stream_props_vtbl;
    object->refcount = 1;
    object->stream = &reader->streams[index];
    IWMProfile3_AddRef(&reader->IWMProfile3_iface);

    LeaveCriticalSection(&reader->cs);

    TRACE("Created stream config %p.\n", object);
    *config = &object->IWMStreamConfig_iface;
    return S_OK;
}

static HRESULT WINAPI profile_GetStreamByNumber(IWMProfile3 *iface, WORD stream_number, IWMStreamConfig **config)
{
    FIXME("iface %p, stream_number %u, config %p, stub!\n", iface, stream_number, config);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_RemoveStream(IWMProfile3 *iface, IWMStreamConfig *config)
{
    FIXME("iface %p, config %p, stub!\n", iface, config);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_RemoveStreamByNumber(IWMProfile3 *iface, WORD stream_number)
{
    FIXME("iface %p, stream_number %u, stub!\n", iface, stream_number);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_AddStream(IWMProfile3 *iface, IWMStreamConfig *config)
{
    FIXME("iface %p, config %p, stub!\n", iface, config);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_ReconfigStream(IWMProfile3 *iface, IWMStreamConfig *config)
{
    FIXME("iface %p, config %p, stub!\n", iface, config);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_CreateNewStream(IWMProfile3 *iface, REFGUID type, IWMStreamConfig **config)
{
    FIXME("iface %p, type %s, config %p, stub!\n", iface, debugstr_guid(type), config);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_GetMutualExclusionCount(IWMProfile3 *iface, DWORD *count)
{
    FIXME("iface %p, count %p, stub!\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_GetMutualExclusion(IWMProfile3 *iface, DWORD index, IWMMutualExclusion **excl)
{
    FIXME("iface %p, index %lu, excl %p, stub!\n", iface, index, excl);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_RemoveMutualExclusion(IWMProfile3 *iface, IWMMutualExclusion *excl)
{
    FIXME("iface %p, excl %p, stub!\n", iface, excl);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_AddMutualExclusion(IWMProfile3 *iface, IWMMutualExclusion *excl)
{
    FIXME("iface %p, excl %p, stub!\n", iface, excl);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_CreateNewMutualExclusion(IWMProfile3 *iface, IWMMutualExclusion **excl)
{
    FIXME("iface %p, excl %p, stub!\n", iface, excl);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_GetProfileID(IWMProfile3 *iface, GUID *id)
{
    FIXME("iface %p, id %p, stub!\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_GetStorageFormat(IWMProfile3 *iface, WMT_STORAGE_FORMAT *format)
{
    FIXME("iface %p, format %p, stub!\n", iface, format);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_SetStorageFormat(IWMProfile3 *iface, WMT_STORAGE_FORMAT format)
{
    FIXME("iface %p, format %#x, stub!\n", iface, format);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_GetBandwidthSharingCount(IWMProfile3 *iface, DWORD *count)
{
    FIXME("iface %p, count %p, stub!\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_GetBandwidthSharing(IWMProfile3 *iface, DWORD index, IWMBandwidthSharing **sharing)
{
    FIXME("iface %p, index %lu, sharing %p, stub!\n", iface, index, sharing);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_RemoveBandwidthSharing( IWMProfile3 *iface, IWMBandwidthSharing *sharing)
{
    FIXME("iface %p, sharing %p, stub!\n", iface, sharing);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_AddBandwidthSharing(IWMProfile3 *iface, IWMBandwidthSharing *sharing)
{
    FIXME("iface %p, sharing %p, stub!\n", iface, sharing);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_CreateNewBandwidthSharing( IWMProfile3 *iface, IWMBandwidthSharing **sharing)
{
    FIXME("iface %p, sharing %p, stub!\n", iface, sharing);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_GetStreamPrioritization(IWMProfile3 *iface, IWMStreamPrioritization **stream)
{
    FIXME("iface %p, stream %p, stub!\n", iface, stream);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_SetStreamPrioritization(IWMProfile3 *iface, IWMStreamPrioritization *stream)
{
    FIXME("iface %p, stream %p, stub!\n", iface, stream);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_RemoveStreamPrioritization(IWMProfile3 *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_CreateNewStreamPrioritization(IWMProfile3 *iface, IWMStreamPrioritization **stream)
{
    FIXME("iface %p, stream %p, stub!\n", iface, stream);
    return E_NOTIMPL;
}

static HRESULT WINAPI profile_GetExpectedPacketCount(IWMProfile3 *iface, QWORD duration, QWORD *count)
{
    FIXME("iface %p, duration %s, count %p, stub!\n", iface, debugstr_time(duration), count);
    return E_NOTIMPL;
}

static const IWMProfile3Vtbl profile_vtbl =
{
    profile_QueryInterface,
    profile_AddRef,
    profile_Release,
    profile_GetVersion,
    profile_GetName,
    profile_SetName,
    profile_GetDescription,
    profile_SetDescription,
    profile_GetStreamCount,
    profile_GetStream,
    profile_GetStreamByNumber,
    profile_RemoveStream,
    profile_RemoveStreamByNumber,
    profile_AddStream,
    profile_ReconfigStream,
    profile_CreateNewStream,
    profile_GetMutualExclusionCount,
    profile_GetMutualExclusion,
    profile_RemoveMutualExclusion,
    profile_AddMutualExclusion,
    profile_CreateNewMutualExclusion,
    profile_GetProfileID,
    profile_GetStorageFormat,
    profile_SetStorageFormat,
    profile_GetBandwidthSharingCount,
    profile_GetBandwidthSharing,
    profile_RemoveBandwidthSharing,
    profile_AddBandwidthSharing,
    profile_CreateNewBandwidthSharing,
    profile_GetStreamPrioritization,
    profile_SetStreamPrioritization,
    profile_RemoveStreamPrioritization,
    profile_CreateNewStreamPrioritization,
    profile_GetExpectedPacketCount,
};

static struct wm_reader *impl_from_IWMHeaderInfo3(IWMHeaderInfo3 *iface)
{
    return CONTAINING_RECORD(iface, struct wm_reader, IWMHeaderInfo3_iface);
}

static HRESULT WINAPI header_info_QueryInterface(IWMHeaderInfo3 *iface, REFIID iid, void **out)
{
    struct wm_reader *reader = impl_from_IWMHeaderInfo3(iface);
    return IUnknown_QueryInterface(reader->outer, iid, out);
}

static ULONG WINAPI header_info_AddRef(IWMHeaderInfo3 *iface)
{
    struct wm_reader *reader = impl_from_IWMHeaderInfo3(iface);
    return IUnknown_AddRef(reader->outer);
}

static ULONG WINAPI header_info_Release(IWMHeaderInfo3 *iface)
{
    struct wm_reader *reader = impl_from_IWMHeaderInfo3(iface);
    return IUnknown_Release(reader->outer);
}

static HRESULT WINAPI header_info_GetAttributeCount(IWMHeaderInfo3 *iface, WORD stream_number, WORD *count)
{
    FIXME("iface %p, stream_number %u, count %p, stub!\n", iface, stream_number, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_GetAttributeByIndex(IWMHeaderInfo3 *iface, WORD index, WORD *stream_number,
        WCHAR *name, WORD *name_len, WMT_ATTR_DATATYPE *type, BYTE *value, WORD *size)
{
    FIXME("iface %p, index %u, stream_number %p, name %p, name_len %p, type %p, value %p, size %p, stub!\n",
            iface, index, stream_number, name, name_len, type, value, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_GetAttributeByName(IWMHeaderInfo3 *iface, WORD *stream_number,
        const WCHAR *name, WMT_ATTR_DATATYPE *type, BYTE *value, WORD *size)
{
    struct wm_reader *reader = impl_from_IWMHeaderInfo3(iface);
    const WORD req_size = *size;

    TRACE("reader %p, stream_number %p, name %s, type %p, value %p, size %u.\n",
            reader, stream_number, debugstr_w(name), type, value, *size);

    if (!stream_number)
        return E_INVALIDARG;

    if (!wcscmp(name, L"Duration"))
    {
        QWORD duration;

        if (*stream_number)
        {
            WARN("Requesting duration for stream %u, returning ASF_E_NOTFOUND.\n", *stream_number);
            return ASF_E_NOTFOUND;
        }

        *size = sizeof(QWORD);
        if (!value)
        {
            *type = WMT_TYPE_QWORD;
            return S_OK;
        }
        if (req_size < *size)
            return ASF_E_BUFFERTOOSMALL;

        *type = WMT_TYPE_QWORD;
        EnterCriticalSection(&reader->cs);
        duration = wg_parser_stream_get_duration(wg_parser_get_stream(reader->wg_parser, 0));
        LeaveCriticalSection(&reader->cs);
        TRACE("Returning duration %s.\n", debugstr_time(duration));
        memcpy(value, &duration, sizeof(QWORD));
        return S_OK;
    }
    else if (!wcscmp(name, L"Seekable"))
    {
        if (*stream_number)
        {
            WARN("Requesting duration for stream %u, returning ASF_E_NOTFOUND.\n", *stream_number);
            return ASF_E_NOTFOUND;
        }

        *size = sizeof(BOOL);
        if (!value)
        {
            *type = WMT_TYPE_BOOL;
            return S_OK;
        }
        if (req_size < *size)
            return ASF_E_BUFFERTOOSMALL;

        *type = WMT_TYPE_BOOL;
        *(BOOL *)value = TRUE;
        return S_OK;
    }
    else
    {
        FIXME("Unknown attribute %s.\n", debugstr_w(name));
        return ASF_E_NOTFOUND;
    }
}

static HRESULT WINAPI header_info_SetAttribute(IWMHeaderInfo3 *iface, WORD stream_number,
        const WCHAR *name, WMT_ATTR_DATATYPE type, const BYTE *value, WORD size)
{
    FIXME("iface %p, stream_number %u, name %s, type %#x, value %p, size %u, stub!\n",
            iface, stream_number, debugstr_w(name), type, value, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_GetMarkerCount(IWMHeaderInfo3 *iface, WORD *count)
{
    FIXME("iface %p, count %p, stub!\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_GetMarker(IWMHeaderInfo3 *iface,
        WORD index, WCHAR *name, WORD *len, QWORD *time)
{
    FIXME("iface %p, index %u, name %p, len %p, time %p, stub!\n", iface, index, name, len, time);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_AddMarker(IWMHeaderInfo3 *iface, const WCHAR *name, QWORD time)
{
    FIXME("iface %p, name %s, time %s, stub!\n", iface, debugstr_w(name), debugstr_time(time));
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_RemoveMarker(IWMHeaderInfo3 *iface, WORD index)
{
    FIXME("iface %p, index %u, stub!\n", iface, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_GetScriptCount(IWMHeaderInfo3 *iface, WORD *count)
{
    FIXME("iface %p, count %p, stub!\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_GetScript(IWMHeaderInfo3 *iface, WORD index, WCHAR *type,
        WORD *type_len, WCHAR *command, WORD *command_len, QWORD *time)
{
    FIXME("iface %p, index %u, type %p, type_len %p, command %p, command_len %p, time %p, stub!\n",
            iface, index, type, type_len, command, command_len, time);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_AddScript(IWMHeaderInfo3 *iface,
        const WCHAR *type, const WCHAR *command, QWORD time)
{
    FIXME("iface %p, type %s, command %s, time %s, stub!\n",
            iface, debugstr_w(type), debugstr_w(command), debugstr_time(time));
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_RemoveScript(IWMHeaderInfo3 *iface, WORD index)
{
    FIXME("iface %p, index %u, stub!\n", iface, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_GetCodecInfoCount(IWMHeaderInfo3 *iface, DWORD *count)
{
    FIXME("iface %p, count %p, stub!\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_GetCodecInfo(IWMHeaderInfo3 *iface, DWORD index, WORD *name_len,
        WCHAR *name, WORD *desc_len, WCHAR *desc, WMT_CODEC_INFO_TYPE *type, WORD *size, BYTE *info)
{
    FIXME("iface %p, index %lu, name_len %p, name %p, desc_len %p, desc %p, type %p, size %p, info %p, stub!\n",
            iface, index, name_len, name, desc_len, desc, type, size, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_GetAttributeCountEx(IWMHeaderInfo3 *iface, WORD stream_number, WORD *count)
{
    FIXME("iface %p, stream_number %u, count %p, stub!\n", iface, stream_number, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_GetAttributeIndices(IWMHeaderInfo3 *iface, WORD stream_number,
        const WCHAR *name, WORD *lang_index, WORD *indices, WORD *count)
{
    FIXME("iface %p, stream_number %u, name %s, lang_index %p, indices %p, count %p, stub!\n",
            iface, stream_number, debugstr_w(name), lang_index, indices, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_GetAttributeByIndexEx(IWMHeaderInfo3 *iface,
        WORD stream_number, WORD index, WCHAR *name, WORD *name_len,
        WMT_ATTR_DATATYPE *type, WORD *lang_index, BYTE *value, DWORD *size)
{
    FIXME("iface %p, stream_number %u, index %u, name %p, name_len %p,"
            " type %p, lang_index %p, value %p, size %p, stub!\n",
            iface, stream_number, index, name, name_len, type, lang_index, value, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_ModifyAttribute(IWMHeaderInfo3 *iface, WORD stream_number,
        WORD index, WMT_ATTR_DATATYPE type, WORD lang_index, const BYTE *value, DWORD size)
{
    FIXME("iface %p, stream_number %u, index %u, type %#x, lang_index %u, value %p, size %lu, stub!\n",
            iface, stream_number, index, type, lang_index, value, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_AddAttribute(IWMHeaderInfo3 *iface,
        WORD stream_number, const WCHAR *name, WORD *index,
        WMT_ATTR_DATATYPE type, WORD lang_index, const BYTE *value, DWORD size)
{
    FIXME("iface %p, stream_number %u, name %s, index %p, type %#x, lang_index %u, value %p, size %lu, stub!\n",
            iface, stream_number, debugstr_w(name), index, type, lang_index, value, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_DeleteAttribute(IWMHeaderInfo3 *iface, WORD stream_number, WORD index)
{
    FIXME("iface %p, stream_number %u, index %u, stub!\n", iface, stream_number, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI header_info_AddCodecInfo(IWMHeaderInfo3 *iface, const WCHAR *name,
        const WCHAR *desc, WMT_CODEC_INFO_TYPE type, WORD size, BYTE *info)
{
    FIXME("iface %p, name %s, desc %s, type %#x, size %u, info %p, stub!\n",
            info, debugstr_w(name), debugstr_w(desc), type, size, info);
    return E_NOTIMPL;
}

static const IWMHeaderInfo3Vtbl header_info_vtbl =
{
    header_info_QueryInterface,
    header_info_AddRef,
    header_info_Release,
    header_info_GetAttributeCount,
    header_info_GetAttributeByIndex,
    header_info_GetAttributeByName,
    header_info_SetAttribute,
    header_info_GetMarkerCount,
    header_info_GetMarker,
    header_info_AddMarker,
    header_info_RemoveMarker,
    header_info_GetScriptCount,
    header_info_GetScript,
    header_info_AddScript,
    header_info_RemoveScript,
    header_info_GetCodecInfoCount,
    header_info_GetCodecInfo,
    header_info_GetAttributeCountEx,
    header_info_GetAttributeIndices,
    header_info_GetAttributeByIndexEx,
    header_info_ModifyAttribute,
    header_info_AddAttribute,
    header_info_DeleteAttribute,
    header_info_AddCodecInfo,
};

static struct wm_reader *impl_from_IWMLanguageList(IWMLanguageList *iface)
{
    return CONTAINING_RECORD(iface, struct wm_reader, IWMLanguageList_iface);
}

static HRESULT WINAPI language_list_QueryInterface(IWMLanguageList *iface, REFIID iid, void **out)
{
    struct wm_reader *reader = impl_from_IWMLanguageList(iface);
    return IUnknown_QueryInterface(reader->outer, iid, out);
}

static ULONG WINAPI language_list_AddRef(IWMLanguageList *iface)
{
    struct wm_reader *reader = impl_from_IWMLanguageList(iface);
    return IUnknown_AddRef(reader->outer);
}

static ULONG WINAPI language_list_Release(IWMLanguageList *iface)
{
    struct wm_reader *reader = impl_from_IWMLanguageList(iface);
    return IUnknown_Release(reader->outer);
}

static HRESULT WINAPI language_list_GetLanguageCount(IWMLanguageList *iface, WORD *count)
{
    FIXME("iface %p, count %p, stub!\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI language_list_GetLanguageDetails(IWMLanguageList *iface,
        WORD index, WCHAR *lang, WORD *len)
{
    FIXME("iface %p, index %u, lang %p, len %p, stub!\n", iface, index, lang, len);
    return E_NOTIMPL;
}

static HRESULT WINAPI language_list_AddLanguageByRFC1766String(IWMLanguageList *iface,
        const WCHAR *lang, WORD *index)
{
    FIXME("iface %p, lang %s, index %p, stub!\n", iface, debugstr_w(lang), index);
    return E_NOTIMPL;
}

static const IWMLanguageListVtbl language_list_vtbl =
{
    language_list_QueryInterface,
    language_list_AddRef,
    language_list_Release,
    language_list_GetLanguageCount,
    language_list_GetLanguageDetails,
    language_list_AddLanguageByRFC1766String,
};

static struct wm_reader *impl_from_IWMPacketSize2(IWMPacketSize2 *iface)
{
    return CONTAINING_RECORD(iface, struct wm_reader, IWMPacketSize2_iface);
}

static HRESULT WINAPI packet_size_QueryInterface(IWMPacketSize2 *iface, REFIID iid, void **out)
{
    struct wm_reader *reader = impl_from_IWMPacketSize2(iface);
    return IUnknown_QueryInterface(reader->outer, iid, out);
}

static ULONG WINAPI packet_size_AddRef(IWMPacketSize2 *iface)
{
    struct wm_reader *reader = impl_from_IWMPacketSize2(iface);
    return IUnknown_AddRef(reader->outer);
}

static ULONG WINAPI packet_size_Release(IWMPacketSize2 *iface)
{
    struct wm_reader *reader = impl_from_IWMPacketSize2(iface);
    return IUnknown_Release(reader->outer);
}

static HRESULT WINAPI packet_size_GetMaxPacketSize(IWMPacketSize2 *iface, DWORD *size)
{
    FIXME("iface %p, size %p, stub!\n", iface, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI packet_size_SetMaxPacketSize(IWMPacketSize2 *iface, DWORD size)
{
    FIXME("iface %p, size %lu, stub!\n", iface, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI packet_size_GetMinPacketSize(IWMPacketSize2 *iface, DWORD *size)
{
    FIXME("iface %p, size %p, stub!\n", iface, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI packet_size_SetMinPacketSize(IWMPacketSize2 *iface, DWORD size)
{
    FIXME("iface %p, size %lu, stub!\n", iface, size);
    return E_NOTIMPL;
}

static const IWMPacketSize2Vtbl packet_size_vtbl =
{
    packet_size_QueryInterface,
    packet_size_AddRef,
    packet_size_Release,
    packet_size_GetMaxPacketSize,
    packet_size_SetMaxPacketSize,
    packet_size_GetMinPacketSize,
    packet_size_SetMinPacketSize,
};

static struct wm_reader *impl_from_IWMReaderPlaylistBurn(IWMReaderPlaylistBurn *iface)
{
    return CONTAINING_RECORD(iface, struct wm_reader, IWMReaderPlaylistBurn_iface);
}

static HRESULT WINAPI playlist_QueryInterface(IWMReaderPlaylistBurn *iface, REFIID iid, void **out)
{
    struct wm_reader *reader = impl_from_IWMReaderPlaylistBurn(iface);
    return IUnknown_QueryInterface(reader->outer, iid, out);
}

static ULONG WINAPI playlist_AddRef(IWMReaderPlaylistBurn *iface)
{
    struct wm_reader *reader = impl_from_IWMReaderPlaylistBurn(iface);
    return IUnknown_AddRef(reader->outer);
}

static ULONG WINAPI playlist_Release(IWMReaderPlaylistBurn *iface)
{
    struct wm_reader *reader = impl_from_IWMReaderPlaylistBurn(iface);
    return IUnknown_Release(reader->outer);
}

static HRESULT WINAPI playlist_InitPlaylistBurn(IWMReaderPlaylistBurn *iface, DWORD count,
        const WCHAR **filenames, IWMStatusCallback *callback, void *context)
{
    FIXME("iface %p, count %lu, filenames %p, callback %p, context %p, stub!\n",
            iface, count, filenames, callback, context);
    return E_NOTIMPL;
}

static HRESULT WINAPI playlist_GetInitResults(IWMReaderPlaylistBurn *iface, DWORD count, HRESULT *hrs)
{
    FIXME("iface %p, count %lu, hrs %p, stub!\n", iface, count, hrs);
    return E_NOTIMPL;
}

static HRESULT WINAPI playlist_Cancel(IWMReaderPlaylistBurn *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI playlist_EndPlaylistBurn(IWMReaderPlaylistBurn *iface, HRESULT hr)
{
    FIXME("iface %p, hr %#lx, stub!\n", iface, hr);
    return E_NOTIMPL;
}

static const IWMReaderPlaylistBurnVtbl playlist_vtbl =
{
    playlist_QueryInterface,
    playlist_AddRef,
    playlist_Release,
    playlist_InitPlaylistBurn,
    playlist_GetInitResults,
    playlist_Cancel,
    playlist_EndPlaylistBurn,
};

static struct wm_reader *impl_from_IWMReaderTimecode(IWMReaderTimecode *iface)
{
    return CONTAINING_RECORD(iface, struct wm_reader, IWMReaderTimecode_iface);
}

static HRESULT WINAPI timecode_QueryInterface(IWMReaderTimecode *iface, REFIID iid, void **out)
{
    struct wm_reader *reader = impl_from_IWMReaderTimecode(iface);
    return IUnknown_QueryInterface(reader->outer, iid, out);
}

static ULONG WINAPI timecode_AddRef(IWMReaderTimecode *iface)
{
    struct wm_reader *reader = impl_from_IWMReaderTimecode(iface);
    return IUnknown_AddRef(reader->outer);
}

static ULONG WINAPI timecode_Release(IWMReaderTimecode *iface)
{
    struct wm_reader *reader = impl_from_IWMReaderTimecode(iface);
    return IUnknown_Release(reader->outer);
}

static HRESULT WINAPI timecode_GetTimecodeRangeCount(IWMReaderTimecode *iface,
        WORD stream_number, WORD *count)
{
    FIXME("iface %p, stream_number %u, count %p, stub!\n", iface, stream_number, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI timecode_GetTimecodeRangeBounds(IWMReaderTimecode *iface,
        WORD stream_number, WORD index, DWORD *start, DWORD *end)
{
    FIXME("iface %p, stream_number %u, index %u, start %p, end %p, stub!\n",
            iface, stream_number, index, start, end);
    return E_NOTIMPL;
}

static const IWMReaderTimecodeVtbl timecode_vtbl =
{
    timecode_QueryInterface,
    timecode_AddRef,
    timecode_Release,
    timecode_GetTimecodeRangeCount,
    timecode_GetTimecodeRangeBounds,
};

static HRESULT init_stream(struct wm_reader *reader, QWORD file_size)
{
    struct wg_parser *wg_parser;
    HRESULT hr;
    WORD i;

    if (!(wg_parser = wg_parser_create(WG_PARSER_DECODEBIN, false)))
        return E_OUTOFMEMORY;

    reader->wg_parser = wg_parser;
    reader->read_thread_shutdown = false;
    if (!(reader->read_thread = CreateThread(NULL, 0, read_thread, reader, 0, NULL)))
    {
        hr = E_OUTOFMEMORY;
        goto out_destroy_parser;
    }

    if (FAILED(hr = wg_parser_connect(reader->wg_parser, file_size)))
    {
        ERR("Failed to connect parser, hr %#lx.\n", hr);
        goto out_shutdown_thread;
    }

    reader->stream_count = wg_parser_get_stream_count(reader->wg_parser);

    if (!(reader->streams = calloc(reader->stream_count, sizeof(*reader->streams))))
    {
        hr = E_OUTOFMEMORY;
        goto out_disconnect_parser;
    }

    for (i = 0; i < reader->stream_count; ++i)
    {
        struct wm_stream *stream = &reader->streams[i];

        stream->wg_stream = wg_parser_get_stream(reader->wg_parser, i);
        stream->reader = reader;
        stream->index = i;
        stream->selection = WMT_ON;
        wg_parser_stream_get_preferred_format(stream->wg_stream, &stream->format);
        if (stream->format.major_type == WG_MAJOR_TYPE_AUDIO)
        {
            /* R.U.S.E enumerates available audio types, picks the first one it
             * likes, and then sets the wrong stream to that type. libav might
             * give us WG_AUDIO_FORMAT_F32LE by default, which will result in
             * the game incorrectly interpreting float data as integer.
             * Therefore just match native and always set our default format to
             * S16LE. */
            stream->format.u.audio.format = WG_AUDIO_FORMAT_S16LE;
        }
        else if (stream->format.major_type == WG_MAJOR_TYPE_VIDEO)
        {
            /* Call of Juarez: Bound in Blood breaks if I420 is enumerated.
             * Some native decoders output I420, but the msmpeg4v3 decoder
             * never does.
             *
             * Shadowgrounds provides wmv3 video and assumes that the initial
             * video type will be BGR. */
            stream->format.u.video.format = WG_VIDEO_FORMAT_BGR;
        }
        wg_parser_stream_enable(stream->wg_stream, &stream->format);
    }

    /* We probably discarded events because streams weren't enabled yet.
     * Now that they're all enabled seek back to the start again. */
    wg_parser_stream_seek(reader->streams[0].wg_stream, 1.0, 0, 0,
            AM_SEEKING_AbsolutePositioning, AM_SEEKING_NoPositioning);

    return S_OK;

out_disconnect_parser:
    wg_parser_disconnect(reader->wg_parser);

out_shutdown_thread:
    reader->read_thread_shutdown = true;
    WaitForSingleObject(reader->read_thread, INFINITE);
    CloseHandle(reader->read_thread);
    reader->read_thread = NULL;

out_destroy_parser:
    wg_parser_destroy(reader->wg_parser);
    reader->wg_parser = NULL;

    return hr;
}

static struct wm_stream *wm_reader_get_stream_by_stream_number(struct wm_reader *reader, WORD stream_number)
{
    if (stream_number && stream_number <= reader->stream_count)
        return &reader->streams[stream_number - 1];
    WARN("Invalid stream number %u.\n", stream_number);
    return NULL;
}

static const enum wg_video_format video_formats[] =
{
    /* Try to prefer YUV formats over RGB ones. Most decoders output in the
     * YUV color space, and it's generally much less expensive for
     * videoconvert to do YUV -> YUV transformations. */
    WG_VIDEO_FORMAT_NV12,
    WG_VIDEO_FORMAT_YV12,
    WG_VIDEO_FORMAT_YUY2,
    WG_VIDEO_FORMAT_UYVY,
    WG_VIDEO_FORMAT_YVYU,
    WG_VIDEO_FORMAT_BGRx,
    WG_VIDEO_FORMAT_BGR,
    WG_VIDEO_FORMAT_RGB16,
    WG_VIDEO_FORMAT_RGB15,
};

static const char *get_major_type_string(enum wg_major_type type)
{
    switch (type)
    {
        case WG_MAJOR_TYPE_AUDIO:
            return "audio";
        case WG_MAJOR_TYPE_VIDEO:
            return "video";
        case WG_MAJOR_TYPE_UNKNOWN:
            return "unknown";
        case WG_MAJOR_TYPE_MPEG1_AUDIO:
            return "mpeg1-audio";
        case WG_MAJOR_TYPE_WMA:
            return "wma";
        case WG_MAJOR_TYPE_H264:
            return "h264";
    }
    assert(0);
    return NULL;
}

/* Find the earliest buffer by PTS.
 *
 * Native seems to behave similarly to this with the async reader, although our
 * unit tests show that it's not entirely consistent—some frames are received
 * slightly out of order. It's possible that one stream is being manually offset
 * to account for decoding latency.
 *
 * The behaviour with the synchronous reader, when stream 0 is requested, seems
 * consistent with this hypothesis, but with a much larger offset—the video
 * stream seems to be "behind" by about 150 ms.
 *
 * The main reason for doing this is that the video and audio stream probably
 * don't have quite the same "frame rate", and we don't want to force one stream
 * to decode faster just to keep up with the other. Delivering samples in PTS
 * order should avoid that problem. */
static WORD get_earliest_buffer(struct wm_reader *reader, struct wg_parser_buffer *ret_buffer)
{
    struct wg_parser_buffer buffer;
    QWORD earliest_pts = UI64_MAX;
    WORD stream_number = 0;
    WORD i;

    for (i = 0; i < reader->stream_count; ++i)
    {
        struct wm_stream *stream = &reader->streams[i];

        if (stream->selection == WMT_OFF)
            continue;

        if (!wg_parser_stream_get_buffer(stream->wg_stream, &buffer))
            continue;

        if (buffer.has_pts && buffer.pts < earliest_pts)
        {
            stream_number = i + 1;
            earliest_pts = buffer.pts;
            *ret_buffer = buffer;
        }
    }

    return stream_number;
}

HRESULT wm_reader_get_stream_sample(struct wm_reader *reader, IWMReaderCallbackAdvanced *callback_advanced, WORD stream_number,
        INSSBuffer **ret_sample, QWORD *pts, QWORD *duration, DWORD *flags, WORD *ret_stream_number)
{
    struct wg_parser_stream *wg_stream;
    struct wg_parser_buffer wg_buffer;
    struct wm_stream *stream;
    DWORD size, capacity;
    INSSBuffer *sample;
    HRESULT hr;
    BYTE *data;

    for (;;)
    {
        if (!stream_number)
        {
            if (!(stream_number = get_earliest_buffer(reader, &wg_buffer)))
            {
                /* All streams are disabled or EOS. */
                return NS_E_NO_MORE_SAMPLES;
            }

            stream = wm_reader_get_stream_by_stream_number(reader, stream_number);
            wg_stream = stream->wg_stream;
        }
        else
        {
            if (!(stream = wm_reader_get_stream_by_stream_number(reader, stream_number)))
            {
                WARN("Invalid stream number %u; returning E_INVALIDARG.\n", stream_number);
                return E_INVALIDARG;
            }
            wg_stream = stream->wg_stream;

            if (stream->selection == WMT_OFF)
            {
                WARN("Stream %u is deselected; returning NS_E_INVALID_REQUEST.\n", stream_number);
                return NS_E_INVALID_REQUEST;
            }

            if (stream->eos)
                return NS_E_NO_MORE_SAMPLES;

            if (!wg_parser_stream_get_buffer(wg_stream, &wg_buffer))
            {
                stream->eos = true;
                TRACE("End of stream.\n");
                return NS_E_NO_MORE_SAMPLES;
            }
        }

        TRACE("Got buffer for '%s' stream %p.\n", get_major_type_string(stream->format.major_type), stream);

        if (callback_advanced && stream->read_compressed && stream->allocate_stream)
        {
            if (FAILED(hr = IWMReaderCallbackAdvanced_AllocateForStream(callback_advanced,
                    stream->index + 1, wg_buffer.size, &sample, NULL)))
            {
                ERR("Failed to allocate stream sample of %u bytes, hr %#lx.\n", wg_buffer.size, hr);
                wg_parser_stream_release_buffer(wg_stream);
                return hr;
            }
        }
        else if (callback_advanced && !stream->read_compressed && stream->allocate_output)
        {
            if (FAILED(hr = IWMReaderCallbackAdvanced_AllocateForOutput(callback_advanced,
                    stream->index, wg_buffer.size, &sample, NULL)))
            {
                ERR("Failed to allocate output sample of %u bytes, hr %#lx.\n", wg_buffer.size, hr);
                wg_parser_stream_release_buffer(wg_stream);
                return hr;
            }
        }
        else
        {
            struct buffer *object;

            /* FIXME: Should these be pooled? */
            if (!(object = calloc(1, offsetof(struct buffer, data[wg_buffer.size]))))
            {
                wg_parser_stream_release_buffer(wg_stream);
                return E_OUTOFMEMORY;
            }

            object->INSSBuffer_iface.lpVtbl = &buffer_vtbl;
            object->refcount = 1;
            object->capacity = wg_buffer.size;

            TRACE("Created buffer %p.\n", object);
            sample = &object->INSSBuffer_iface;
        }

        if (FAILED(hr = INSSBuffer_GetBufferAndLength(sample, &data, &size)))
            ERR("Failed to get data pointer, hr %#lx.\n", hr);
        if (FAILED(hr = INSSBuffer_GetMaxLength(sample, &capacity)))
            ERR("Failed to get capacity, hr %#lx.\n", hr);
        if (wg_buffer.size > capacity)
            ERR("Returned capacity %lu is less than requested capacity %u.\n", capacity, wg_buffer.size);

        if (!wg_parser_stream_copy_buffer(wg_stream, data, 0, wg_buffer.size))
        {
            /* The GStreamer pin has been flushed. */
            INSSBuffer_Release(sample);
            continue;
        }

        if (FAILED(hr = INSSBuffer_SetLength(sample, wg_buffer.size)))
            ERR("Failed to set size %u, hr %#lx.\n", wg_buffer.size, hr);

        wg_parser_stream_release_buffer(wg_stream);

        if (!wg_buffer.has_pts)
            FIXME("Missing PTS.\n");
        if (!wg_buffer.has_duration)
            FIXME("Missing duration.\n");

        *pts = wg_buffer.pts;
        *duration = wg_buffer.duration;
        *flags = 0;
        if (wg_buffer.discontinuity)
            *flags |= WM_SF_DISCONTINUITY;
        if (!wg_buffer.delta)
            *flags |= WM_SF_CLEANPOINT;

        *ret_sample = sample;
        *ret_stream_number = stream_number;
        return S_OK;
    }
}

HRESULT wm_reader_set_allocate_for_output(struct wm_reader *reader, DWORD output, BOOL allocate)
{
    struct wm_stream *stream;

    EnterCriticalSection(&reader->cs);

    if (!(stream = get_stream_by_output_number(reader, output)))
    {
        LeaveCriticalSection(&reader->cs);
        return E_INVALIDARG;
    }

    stream->allocate_output = !!allocate;

    LeaveCriticalSection(&reader->cs);
    return S_OK;
}

HRESULT wm_reader_set_allocate_for_stream(struct wm_reader *reader, WORD stream_number, BOOL allocate)
{
    struct wm_stream *stream;

    EnterCriticalSection(&reader->cs);

    if (!(stream = wm_reader_get_stream_by_stream_number(reader, stream_number)))
    {
        LeaveCriticalSection(&reader->cs);
        return E_INVALIDARG;
    }

    stream->allocate_stream = !!allocate;

    LeaveCriticalSection(&reader->cs);
    return S_OK;
}

static struct wm_reader *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct wm_reader, IUnknown_inner);
}

static HRESULT WINAPI unknown_inner_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct wm_reader *reader = impl_from_IUnknown(iface);

    TRACE("reader %p, iid %s, out %p.\n", reader, debugstr_guid(iid), out);

    if (IsEqualIID(iid, &IID_IUnknown)
            || IsEqualIID(iid, &IID_IWMSyncReader)
            || IsEqualIID(iid, &IID_IWMSyncReader2))
        *out = &reader->IWMSyncReader2_iface;
    else if (IsEqualIID(iid, &IID_IWMHeaderInfo)
            || IsEqualIID(iid, &IID_IWMHeaderInfo2)
            || IsEqualIID(iid, &IID_IWMHeaderInfo3))
        *out = &reader->IWMHeaderInfo3_iface;
    else if (IsEqualIID(iid, &IID_IWMLanguageList))
        *out = &reader->IWMLanguageList_iface;
    else if (IsEqualIID(iid, &IID_IWMPacketSize)
            || IsEqualIID(iid, &IID_IWMPacketSize2))
        *out = &reader->IWMPacketSize2_iface;
    else if (IsEqualIID(iid, &IID_IWMProfile)
            || IsEqualIID(iid, &IID_IWMProfile2)
            || IsEqualIID(iid, &IID_IWMProfile3))
        *out = &reader->IWMProfile3_iface;
    else if (IsEqualIID(iid, &IID_IWMReaderPlaylistBurn))
        *out = &reader->IWMReaderPlaylistBurn_iface;
    else if (IsEqualIID(iid, &IID_IWMReaderTimecode))
        *out = &reader->IWMReaderTimecode_iface;
    else
    {
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI unknown_inner_AddRef(IUnknown *iface)
{
    struct wm_reader *reader = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&reader->refcount);
    TRACE("%p increasing refcount to %lu.\n", reader, refcount);
    return refcount;
}

static ULONG WINAPI unknown_inner_Release(IUnknown *iface)
{
    struct wm_reader *reader = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&reader->refcount);

    TRACE("%p decreasing refcount to %lu.\n", reader, refcount);

    if (!refcount)
    {
        IWMSyncReader2_Close(&reader->IWMSyncReader2_iface);

        reader->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&reader->cs);

        free(reader);
    }

    return refcount;
}

static const IUnknownVtbl unknown_inner_vtbl =
{
    unknown_inner_QueryInterface,
    unknown_inner_AddRef,
    unknown_inner_Release,
};

static struct wm_reader *impl_from_IWMSyncReader2(IWMSyncReader2 *iface)
{
    return CONTAINING_RECORD(iface, struct wm_reader, IWMSyncReader2_iface);
}

static HRESULT WINAPI reader_QueryInterface(IWMSyncReader2 *iface, REFIID iid, void **out)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);
    return IUnknown_QueryInterface(reader->outer, iid, out);
}

static ULONG WINAPI reader_AddRef(IWMSyncReader2 *iface)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);
    return IUnknown_AddRef(reader->outer);
}

static ULONG WINAPI reader_Release(IWMSyncReader2 *iface)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);
    return IUnknown_Release(reader->outer);
}

static HRESULT WINAPI reader_Close(IWMSyncReader2 *iface)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p.\n", reader);

    EnterCriticalSection(&reader->cs);

    if (!reader->wg_parser)
    {
        LeaveCriticalSection(&reader->cs);
        return NS_E_INVALID_REQUEST;
    }

    wg_parser_disconnect(reader->wg_parser);

    reader->read_thread_shutdown = true;
    WaitForSingleObject(reader->read_thread, INFINITE);
    CloseHandle(reader->read_thread);
    reader->read_thread = NULL;

    wg_parser_destroy(reader->wg_parser);
    reader->wg_parser = NULL;

    if (reader->source_stream)
        IStream_Release(reader->source_stream);
    reader->source_stream = NULL;
    if (reader->file)
        CloseHandle(reader->file);
    reader->file = NULL;

    LeaveCriticalSection(&reader->cs);
    return S_OK;
}

static HRESULT WINAPI reader_GetMaxOutputSampleSize(IWMSyncReader2 *iface, DWORD output, DWORD *max)
{
    struct wm_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%lu %p): stub!\n", This, output, max);
    return E_NOTIMPL;
}

static HRESULT WINAPI reader_GetMaxStreamSampleSize(IWMSyncReader2 *iface, WORD stream_number, DWORD *size)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);
    struct wm_stream *stream;

    TRACE("reader %p, stream_number %u, size %p.\n", reader, stream_number, size);

    EnterCriticalSection(&reader->cs);

    if (!(stream = wm_reader_get_stream_by_stream_number(reader, stream_number)))
    {
        LeaveCriticalSection(&reader->cs);
        return E_INVALIDARG;
    }

    *size = wg_format_get_max_size(&stream->format);

    LeaveCriticalSection(&reader->cs);
    return S_OK;
}

static HRESULT WINAPI reader_GetNextSample(IWMSyncReader2 *iface,
        WORD stream_number, INSSBuffer **sample, QWORD *pts, QWORD *duration,
        DWORD *flags, DWORD *output_number, WORD *ret_stream_number)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);
    HRESULT hr = NS_E_NO_MORE_SAMPLES;

    TRACE("reader %p, stream_number %u, sample %p, pts %p, duration %p,"
            " flags %p, output_number %p, ret_stream_number %p.\n",
            reader, stream_number, sample, pts, duration, flags, output_number, ret_stream_number);

    if (!stream_number && !output_number && !ret_stream_number)
        return E_INVALIDARG;

    EnterCriticalSection(&reader->cs);

    hr = wm_reader_get_stream_sample(reader, NULL, stream_number, sample, pts, duration, flags, &stream_number);
    if (output_number && hr == S_OK)
        *output_number = stream_number - 1;
    if (ret_stream_number && (hr == S_OK || stream_number))
        *ret_stream_number = stream_number;

    LeaveCriticalSection(&reader->cs);
    return hr;
}

static HRESULT WINAPI reader_GetOutputCount(IWMSyncReader2 *iface, DWORD *count)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, count %p.\n", reader, count);

    EnterCriticalSection(&reader->cs);
    *count = reader->stream_count;
    LeaveCriticalSection(&reader->cs);
    return S_OK;
}

static HRESULT WINAPI reader_GetOutputFormat(IWMSyncReader2 *iface,
        DWORD output, DWORD index, IWMOutputMediaProps **props)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);
    struct wm_stream *stream;
    struct wg_format format;

    TRACE("reader %p, output %lu, index %lu, props %p.\n", reader, output, index, props);

    EnterCriticalSection(&reader->cs);

    if (!(stream = get_stream_by_output_number(reader, output)))
    {
        LeaveCriticalSection(&reader->cs);
        return E_INVALIDARG;
    }

    wg_parser_stream_get_preferred_format(stream->wg_stream, &format);

    switch (format.major_type)
    {
        case WG_MAJOR_TYPE_VIDEO:
            if (index >= ARRAY_SIZE(video_formats))
            {
                LeaveCriticalSection(&reader->cs);
                return NS_E_INVALID_OUTPUT_FORMAT;
            }
            format.u.video.format = video_formats[index];
            break;

        case WG_MAJOR_TYPE_AUDIO:
            if (index)
            {
                LeaveCriticalSection(&reader->cs);
                return NS_E_INVALID_OUTPUT_FORMAT;
            }
            format.u.audio.format = WG_AUDIO_FORMAT_S16LE;
            break;

        case WG_MAJOR_TYPE_MPEG1_AUDIO:
        case WG_MAJOR_TYPE_WMA:
        case WG_MAJOR_TYPE_H264:
            FIXME("Format %u not implemented!\n", format.major_type);
            break;
        case WG_MAJOR_TYPE_UNKNOWN:
            break;
    }

    LeaveCriticalSection(&reader->cs);

    *props = output_props_create(&format);
    return *props ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI reader_GetOutputFormatCount(IWMSyncReader2 *iface, DWORD output, DWORD *count)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);
    struct wm_stream *stream;
    struct wg_format format;

    TRACE("reader %p, output %lu, count %p.\n", reader, output, count);

    EnterCriticalSection(&reader->cs);

    if (!(stream = get_stream_by_output_number(reader, output)))
    {
        LeaveCriticalSection(&reader->cs);
        return E_INVALIDARG;
    }

    wg_parser_stream_get_preferred_format(stream->wg_stream, &format);
    switch (format.major_type)
    {
        case WG_MAJOR_TYPE_VIDEO:
            *count = ARRAY_SIZE(video_formats);
            break;

        case WG_MAJOR_TYPE_MPEG1_AUDIO:
        case WG_MAJOR_TYPE_WMA:
        case WG_MAJOR_TYPE_H264:
            FIXME("Format %u not implemented!\n", format.major_type);
            /* fallthrough */
        case WG_MAJOR_TYPE_AUDIO:
        case WG_MAJOR_TYPE_UNKNOWN:
            *count = 1;
            break;
    }

    LeaveCriticalSection(&reader->cs);
    return S_OK;
}

static HRESULT WINAPI reader_GetOutputNumberForStream(IWMSyncReader2 *iface,
        WORD stream_number, DWORD *output)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, stream_number %u, output %p.\n", reader, stream_number, output);

    *output = stream_number - 1;
    return S_OK;
}

static HRESULT WINAPI reader_GetOutputProps(IWMSyncReader2 *iface,
        DWORD output, IWMOutputMediaProps **props)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);
    struct wm_stream *stream;

    TRACE("reader %p, output %lu, props %p.\n", reader, output, props);

    EnterCriticalSection(&reader->cs);

    if (!(stream = get_stream_by_output_number(reader, output)))
    {
        LeaveCriticalSection(&reader->cs);
        return E_INVALIDARG;
    }

    *props = output_props_create(&stream->format);
    LeaveCriticalSection(&reader->cs);
    return *props ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI reader_GetOutputSetting(IWMSyncReader2 *iface, DWORD output_num, const WCHAR *name,
        WMT_ATTR_DATATYPE *type, BYTE *value, WORD *length)
{
    struct wm_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%lu %s %p %p %p): stub!\n", This, output_num, debugstr_w(name), type, value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI reader_GetReadStreamSamples(IWMSyncReader2 *iface, WORD stream_number, BOOL *compressed)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);
    struct wm_stream *stream;

    TRACE("reader %p, stream_number %u, compressed %p.\n", reader, stream_number, compressed);

    EnterCriticalSection(&reader->cs);

    if (!(stream = wm_reader_get_stream_by_stream_number(reader, stream_number)))
    {
        LeaveCriticalSection(&reader->cs);
        return E_INVALIDARG;
    }

    *compressed = stream->read_compressed;

    LeaveCriticalSection(&reader->cs);
    return S_OK;
}

static HRESULT WINAPI reader_GetStreamNumberForOutput(IWMSyncReader2 *iface,
        DWORD output, WORD *stream_number)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, output %lu, stream_number %p.\n", reader, output, stream_number);

    *stream_number = output + 1;
    return S_OK;
}

static HRESULT WINAPI reader_GetStreamSelected(IWMSyncReader2 *iface,
        WORD stream_number, WMT_STREAM_SELECTION *selection)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);
    struct wm_stream *stream;

    TRACE("reader %p, stream_number %u, selection %p.\n", reader, stream_number, selection);

    EnterCriticalSection(&reader->cs);

    if (!(stream = wm_reader_get_stream_by_stream_number(reader, stream_number)))
    {
        LeaveCriticalSection(&reader->cs);
        return E_INVALIDARG;
    }

    *selection = stream->selection;

    LeaveCriticalSection(&reader->cs);
    return S_OK;
}

static HRESULT WINAPI reader_Open(IWMSyncReader2 *iface, const WCHAR *filename)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);
    LARGE_INTEGER size;
    HANDLE file;
    HRESULT hr;

    TRACE("reader %p, filename %s.\n", reader, debugstr_w(filename));

    if ((file = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
    {
        ERR("Failed to open %s, error %lu.\n", debugstr_w(filename), GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!GetFileSizeEx(file, &size))
    {
        ERR("Failed to get the size of %s, error %lu.\n", debugstr_w(filename), GetLastError());
        CloseHandle(file);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    EnterCriticalSection(&reader->cs);

    if (reader->wg_parser)
    {
        LeaveCriticalSection(&reader->cs);
        WARN("Stream is already open; returning E_UNEXPECTED.\n");
        CloseHandle(file);
        return E_UNEXPECTED;
    }

    reader->file = file;

    if (FAILED(hr = init_stream(reader, size.QuadPart)))
        reader->file = NULL;

    LeaveCriticalSection(&reader->cs);
    return hr;
}

static HRESULT WINAPI reader_OpenStream(IWMSyncReader2 *iface, IStream *stream)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);
    STATSTG stat;
    HRESULT hr;

    TRACE("reader %p, stream %p.\n", reader, stream);

    if (FAILED(hr = IStream_Stat(stream, &stat, STATFLAG_NONAME)))
    {
        ERR("Failed to stat stream, hr %#lx.\n", hr);
        return hr;
    }

    EnterCriticalSection(&reader->cs);

    if (reader->wg_parser)
    {
        LeaveCriticalSection(&reader->cs);
        WARN("Stream is already open; returning E_UNEXPECTED.\n");
        return E_UNEXPECTED;
    }

    IStream_AddRef(reader->source_stream = stream);
    if (FAILED(hr = init_stream(reader, stat.cbSize.QuadPart)))
    {
        IStream_Release(stream);
        reader->source_stream = NULL;
    }

    LeaveCriticalSection(&reader->cs);
    return hr;
}

static HRESULT WINAPI reader_SetOutputProps(IWMSyncReader2 *iface, DWORD output, IWMOutputMediaProps *props_iface)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);
    struct output_props *props = unsafe_impl_from_IWMOutputMediaProps(props_iface);
    struct wg_format format, pref_format;
    struct wm_stream *stream;
    HRESULT hr = S_OK;
    int i;

    TRACE("reader %p, output %lu, props_iface %p.\n", reader, output, props_iface);

    strmbase_dump_media_type(&props->mt);

    if (!amt_to_wg_format(&props->mt, &format))
    {
        ERR("Failed to convert media type to winegstreamer format.\n");
        return E_FAIL;
    }

    EnterCriticalSection(&reader->cs);

    if (!(stream = get_stream_by_output_number(reader, output)))
    {
        LeaveCriticalSection(&reader->cs);
        return E_INVALIDARG;
    }

    wg_parser_stream_get_preferred_format(stream->wg_stream, &pref_format);
    if (pref_format.major_type != format.major_type)
    {
        /* R.U.S.E sets the type of the wrong stream, apparently by accident. */
        hr = NS_E_INCOMPATIBLE_FORMAT;
    }
    else switch (pref_format.major_type)
    {
        case WG_MAJOR_TYPE_AUDIO:
            if (format.u.audio.format == WG_AUDIO_FORMAT_UNKNOWN)
                hr = NS_E_AUDIO_CODEC_NOT_INSTALLED;
            else if (format.u.audio.channels > pref_format.u.audio.channels)
                hr = NS_E_AUDIO_CODEC_NOT_INSTALLED;
            break;

        case WG_MAJOR_TYPE_VIDEO:
            for (i = 0; i < ARRAY_SIZE(video_formats); ++i)
                if (format.u.video.format == video_formats[i])
                    break;
            if (i == ARRAY_SIZE(video_formats))
                hr = NS_E_INVALID_OUTPUT_FORMAT;
            else if (pref_format.u.video.width != format.u.video.width)
                hr = NS_E_INVALID_OUTPUT_FORMAT;
            else if (pref_format.u.video.height != format.u.video.height)
                hr = NS_E_INVALID_OUTPUT_FORMAT;
            break;

        default:
            hr = NS_E_INCOMPATIBLE_FORMAT;
            break;
    }

    if (FAILED(hr))
    {
        WARN("Unsupported media type, returning %#lx.\n", hr);
        LeaveCriticalSection(&reader->cs);
        return hr;
    }

    stream->format = format;
    wg_parser_stream_enable(stream->wg_stream, &format);

    /* Re-decode any buffers that might have been generated with the old format.
     *
     * FIXME: Seeking in-place will cause some buffers to be dropped.
     * Unfortunately, we can't really store the last received PTS and seek there
     * either: since seeks are inexact and we aren't guaranteed to receive
     * samples in order, some buffers might be duplicated or dropped anyway.
     * In order to really seamlessly allow for format changes, we need
     * cooperation from each individual GStreamer stream, to be able to tell
     * upstream exactly which buffers they need resent...
     *
     * In all likelihood this function is being called not mid-stream but rather
     * while setting the stream up, before consuming any events. Accordingly
     * let's just seek back to the beginning. */
    wg_parser_stream_seek(reader->streams[0].wg_stream, 1.0, reader->start_time, 0,
            AM_SEEKING_AbsolutePositioning, AM_SEEKING_NoPositioning);

    LeaveCriticalSection(&reader->cs);
    return S_OK;
}

static HRESULT WINAPI reader_SetOutputSetting(IWMSyncReader2 *iface, DWORD output,
        const WCHAR *name, WMT_ATTR_DATATYPE type, const BYTE *value, WORD size)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, output %lu, name %s, type %#x, value %p, size %u.\n",
            reader, output, debugstr_w(name), type, value, size);

    if (!wcscmp(name, L"VideoSampleDurations"))
    {
        FIXME("Ignoring VideoSampleDurations setting.\n");
        return S_OK;
    }
    if (!wcscmp(name, L"EnableDiscreteOutput"))
    {
        FIXME("Ignoring EnableDiscreteOutput setting.\n");
        return S_OK;
    }
    if (!wcscmp(name, L"SpeakerConfig"))
    {
        FIXME("Ignoring SpeakerConfig setting.\n");
        return S_OK;
    }
    else
    {
        FIXME("Unknown setting %s; returning E_NOTIMPL.\n", debugstr_w(name));
        return E_NOTIMPL;
    }
}

static HRESULT WINAPI reader_SetRange(IWMSyncReader2 *iface, QWORD start, LONGLONG duration)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);
    WORD i;

    TRACE("reader %p, start %I64u, duration %I64d.\n", reader, start, duration);

    EnterCriticalSection(&reader->cs);

    reader->start_time = start;

    wg_parser_stream_seek(reader->streams[0].wg_stream, 1.0, start, start + duration,
            AM_SEEKING_AbsolutePositioning, duration ? AM_SEEKING_AbsolutePositioning : AM_SEEKING_NoPositioning);

    for (i = 0; i < reader->stream_count; ++i)
        reader->streams[i].eos = false;

    LeaveCriticalSection(&reader->cs);
    return S_OK;
}

static HRESULT WINAPI reader_SetRangeByFrame(IWMSyncReader2 *iface, WORD stream_num, QWORD frame_num,
        LONGLONG frames)
{
    struct wm_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%d %s %s): stub!\n", This, stream_num, wine_dbgstr_longlong(frame_num), wine_dbgstr_longlong(frames));
    return E_NOTIMPL;
}

static HRESULT WINAPI reader_SetReadStreamSamples(IWMSyncReader2 *iface, WORD stream_number, BOOL compressed)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);
    struct wm_stream *stream;

    TRACE("reader %p, stream_index %u, compressed %d.\n", reader, stream_number, compressed);

    EnterCriticalSection(&reader->cs);

    if (!(stream = wm_reader_get_stream_by_stream_number(reader, stream_number)))
    {
        LeaveCriticalSection(&reader->cs);
        return E_INVALIDARG;
    }

    stream->read_compressed = compressed;

    LeaveCriticalSection(&reader->cs);
    return S_OK;
}

static HRESULT WINAPI reader_SetStreamsSelected(IWMSyncReader2 *iface,
        WORD count, WORD *stream_numbers, WMT_STREAM_SELECTION *selections)
{
    struct wm_reader *reader = impl_from_IWMSyncReader2(iface);
    struct wm_stream *stream;
    WORD i;

    TRACE("reader %p, count %u, stream_numbers %p, selections %p.\n",
            reader, count, stream_numbers, selections);

    if (!count)
        return E_INVALIDARG;

    EnterCriticalSection(&reader->cs);

    for (i = 0; i < count; ++i)
    {
        if (!(stream = wm_reader_get_stream_by_stream_number(reader, stream_numbers[i])))
        {
            LeaveCriticalSection(&reader->cs);
            WARN("Invalid stream number %u; returning NS_E_INVALID_REQUEST.\n", stream_numbers[i]);
            return NS_E_INVALID_REQUEST;
        }
    }

    for (i = 0; i < count; ++i)
    {
        stream = wm_reader_get_stream_by_stream_number(reader, stream_numbers[i]);
        stream->selection = selections[i];
        if (selections[i] == WMT_OFF)
        {
            TRACE("Disabling stream %u.\n", stream_numbers[i]);
            wg_parser_stream_disable(stream->wg_stream);
        }
        else if (selections[i] == WMT_ON)
        {
            if (selections[i] != WMT_ON)
                FIXME("Ignoring selection %#x for stream %u; treating as enabled.\n",
                        selections[i], stream_numbers[i]);
            TRACE("Enabling stream %u.\n", stream_numbers[i]);
            wg_parser_stream_enable(stream->wg_stream, &stream->format);
        }
    }

    LeaveCriticalSection(&reader->cs);
    return S_OK;
}

static HRESULT WINAPI reader_SetRangeByTimecode(IWMSyncReader2 *iface, WORD stream_num,
        WMT_TIMECODE_EXTENSION_DATA *start, WMT_TIMECODE_EXTENSION_DATA *end)
{
    struct wm_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%u %p %p): stub!\n", This, stream_num, start, end);
    return E_NOTIMPL;
}

static HRESULT WINAPI reader_SetRangeByFrameEx(IWMSyncReader2 *iface, WORD stream_num, QWORD frame_num,
        LONGLONG frames_to_read, QWORD *starttime)
{
    struct wm_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%u %s %s %p): stub!\n", This, stream_num, wine_dbgstr_longlong(frame_num),
          wine_dbgstr_longlong(frames_to_read), starttime);
    return E_NOTIMPL;
}

static HRESULT WINAPI reader_SetAllocateForOutput(IWMSyncReader2 *iface, DWORD output_num, IWMReaderAllocatorEx *allocator)
{
    struct wm_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%lu %p): stub!\n", This, output_num, allocator);
    return E_NOTIMPL;
}

static HRESULT WINAPI reader_GetAllocateForOutput(IWMSyncReader2 *iface, DWORD output_num, IWMReaderAllocatorEx **allocator)
{
    struct wm_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%lu %p): stub!\n", This, output_num, allocator);
    return E_NOTIMPL;
}

static HRESULT WINAPI reader_SetAllocateForStream(IWMSyncReader2 *iface, DWORD stream_num, IWMReaderAllocatorEx *allocator)
{
    struct wm_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%lu %p): stub!\n", This, stream_num, allocator);
    return E_NOTIMPL;
}

static HRESULT WINAPI reader_GetAllocateForStream(IWMSyncReader2 *iface, DWORD stream_num, IWMReaderAllocatorEx **allocator)
{
    struct wm_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%lu %p): stub!\n", This, stream_num, allocator);
    return E_NOTIMPL;
}

static const IWMSyncReader2Vtbl reader_vtbl =
{
    reader_QueryInterface,
    reader_AddRef,
    reader_Release,
    reader_Open,
    reader_Close,
    reader_SetRange,
    reader_SetRangeByFrame,
    reader_GetNextSample,
    reader_SetStreamsSelected,
    reader_GetStreamSelected,
    reader_SetReadStreamSamples,
    reader_GetReadStreamSamples,
    reader_GetOutputSetting,
    reader_SetOutputSetting,
    reader_GetOutputCount,
    reader_GetOutputProps,
    reader_SetOutputProps,
    reader_GetOutputFormatCount,
    reader_GetOutputFormat,
    reader_GetOutputNumberForStream,
    reader_GetStreamNumberForOutput,
    reader_GetMaxOutputSampleSize,
    reader_GetMaxStreamSampleSize,
    reader_OpenStream,
    reader_SetRangeByTimecode,
    reader_SetRangeByFrameEx,
    reader_SetAllocateForOutput,
    reader_GetAllocateForOutput,
    reader_SetAllocateForStream,
    reader_GetAllocateForStream
};

struct wm_reader *wm_reader_from_sync_reader_inner(IUnknown *iface)
{
    return impl_from_IUnknown(iface);
}

HRESULT WINAPI winegstreamer_create_wm_sync_reader(IUnknown *outer, void **out)
{
    struct wm_reader *object;

    TRACE("out %p.\n", out);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IUnknown_inner.lpVtbl = &unknown_inner_vtbl;
    object->IWMSyncReader2_iface.lpVtbl = &reader_vtbl;
    object->IWMHeaderInfo3_iface.lpVtbl = &header_info_vtbl;
    object->IWMLanguageList_iface.lpVtbl = &language_list_vtbl;
    object->IWMPacketSize2_iface.lpVtbl = &packet_size_vtbl;
    object->IWMProfile3_iface.lpVtbl = &profile_vtbl;
    object->IWMReaderPlaylistBurn_iface.lpVtbl = &playlist_vtbl;
    object->IWMReaderTimecode_iface.lpVtbl = &timecode_vtbl;
    object->outer = outer ? outer : &object->IUnknown_inner;
    object->refcount = 1;

    InitializeCriticalSection(&object->cs);
    object->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": reader.cs");

    TRACE("Created reader %p.\n", object);
    *out = outer ? (void *)&object->IUnknown_inner : (void *)&object->IWMSyncReader2_iface;
    return S_OK;
}
