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

struct sync_reader
{
    struct wm_reader reader;

    IWMSyncReader2 IWMSyncReader2_iface;
};

static struct sync_reader *impl_from_IWMSyncReader2(IWMSyncReader2 *iface)
{
    return CONTAINING_RECORD(iface, struct sync_reader, IWMSyncReader2_iface);
}

static HRESULT WINAPI WMSyncReader_QueryInterface(IWMSyncReader2 *iface, REFIID iid, void **out)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    return IWMProfile3_QueryInterface(&reader->reader.IWMProfile3_iface, iid, out);
}

static ULONG WINAPI WMSyncReader_AddRef(IWMSyncReader2 *iface)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    return IWMProfile3_AddRef(&reader->reader.IWMProfile3_iface);
}

static ULONG WINAPI WMSyncReader_Release(IWMSyncReader2 *iface)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    return IWMProfile3_Release(&reader->reader.IWMProfile3_iface);
}

static HRESULT WINAPI WMSyncReader_Close(IWMSyncReader2 *iface)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p.\n", reader);

    return wm_reader_close(&reader->reader);
}

static HRESULT WINAPI WMSyncReader_GetMaxOutputSampleSize(IWMSyncReader2 *iface, DWORD output, DWORD *max)
{
    struct sync_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%lu %p): stub!\n", This, output, max);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetMaxStreamSampleSize(IWMSyncReader2 *iface, WORD stream, DWORD *max)
{
    struct sync_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%d %p): stub!\n", This, stream, max);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetNextSample(IWMSyncReader2 *iface,
        WORD stream_number, INSSBuffer **sample, QWORD *pts, QWORD *duration,
        DWORD *flags, DWORD *output_number, WORD *ret_stream_number)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);
    HRESULT hr = NS_E_NO_MORE_SAMPLES;

    TRACE("reader %p, stream_number %u, sample %p, pts %p, duration %p,"
            " flags %p, output_number %p, ret_stream_number %p.\n",
            reader, stream_number, sample, pts, duration, flags, output_number, ret_stream_number);

    if (!stream_number && !output_number && !ret_stream_number)
        return E_INVALIDARG;

    EnterCriticalSection(&reader->reader.cs);

    hr = wm_reader_get_stream_sample(&reader->reader, stream_number, sample, pts, duration, flags, &stream_number);
    if (output_number && hr == S_OK)
        *output_number = stream_number - 1;
    if (ret_stream_number && (hr == S_OK || stream_number))
        *ret_stream_number = stream_number;

    LeaveCriticalSection(&reader->reader.cs);
    return hr;
}

static HRESULT WINAPI WMSyncReader_GetOutputCount(IWMSyncReader2 *iface, DWORD *count)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, count %p.\n", reader, count);

    EnterCriticalSection(&reader->reader.cs);
    *count = reader->reader.stream_count;
    LeaveCriticalSection(&reader->reader.cs);
    return S_OK;
}

static HRESULT WINAPI WMSyncReader_GetOutputFormat(IWMSyncReader2 *iface,
        DWORD output, DWORD index, IWMOutputMediaProps **props)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, output %lu, index %lu, props %p.\n", reader, output, index, props);

    return wm_reader_get_output_format(&reader->reader, output, index, props);
}

static HRESULT WINAPI WMSyncReader_GetOutputFormatCount(IWMSyncReader2 *iface, DWORD output, DWORD *count)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, output %lu, count %p.\n", reader, output, count);

    return wm_reader_get_output_format_count(&reader->reader, output, count);
}

static HRESULT WINAPI WMSyncReader_GetOutputNumberForStream(IWMSyncReader2 *iface,
        WORD stream_number, DWORD *output)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, stream_number %u, output %p.\n", reader, stream_number, output);

    *output = stream_number - 1;
    return S_OK;
}

static HRESULT WINAPI WMSyncReader_GetOutputProps(IWMSyncReader2 *iface,
        DWORD output, IWMOutputMediaProps **props)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, output %lu, props %p.\n", reader, output, props);

    return wm_reader_get_output_props(&reader->reader, output, props);
}

static HRESULT WINAPI WMSyncReader_GetOutputSetting(IWMSyncReader2 *iface, DWORD output_num, const WCHAR *name,
        WMT_ATTR_DATATYPE *type, BYTE *value, WORD *length)
{
    struct sync_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%lu %s %p %p %p): stub!\n", This, output_num, debugstr_w(name), type, value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetReadStreamSamples(IWMSyncReader2 *iface, WORD stream_num, BOOL *compressed)
{
    struct sync_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%d %p): stub!\n", This, stream_num, compressed);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetStreamNumberForOutput(IWMSyncReader2 *iface,
        DWORD output, WORD *stream_number)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, output %lu, stream_number %p.\n", reader, output, stream_number);

    *stream_number = output + 1;
    return S_OK;
}

static HRESULT WINAPI WMSyncReader_GetStreamSelected(IWMSyncReader2 *iface,
        WORD stream_number, WMT_STREAM_SELECTION *selection)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, stream_number %u, selection %p.\n", reader, stream_number, selection);

    return wm_reader_get_stream_selection(&reader->reader, stream_number, selection);
}

static HRESULT WINAPI WMSyncReader_Open(IWMSyncReader2 *iface, const WCHAR *filename)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, filename %s.\n", reader, debugstr_w(filename));

    return wm_reader_open_file(&reader->reader, filename);
}

static HRESULT WINAPI WMSyncReader_OpenStream(IWMSyncReader2 *iface, IStream *stream)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, stream %p.\n", reader, stream);

    return wm_reader_open_stream(&reader->reader, stream);
}

static HRESULT WINAPI WMSyncReader_SetOutputProps(IWMSyncReader2 *iface, DWORD output, IWMOutputMediaProps *props)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, output %lu, props %p.\n", reader, output, props);

    return wm_reader_set_output_props(&reader->reader, output, props);
}

static HRESULT WINAPI WMSyncReader_SetOutputSetting(IWMSyncReader2 *iface, DWORD output,
        const WCHAR *name, WMT_ATTR_DATATYPE type, const BYTE *value, WORD size)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

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

static HRESULT WINAPI WMSyncReader_SetRange(IWMSyncReader2 *iface, QWORD start, LONGLONG duration)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, start %I64u, duration %I64d.\n", reader, start, duration);

    wm_reader_seek(&reader->reader, start, duration);
    return S_OK;
}

static HRESULT WINAPI WMSyncReader_SetRangeByFrame(IWMSyncReader2 *iface, WORD stream_num, QWORD frame_num,
        LONGLONG frames)
{
    struct sync_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%d %s %s): stub!\n", This, stream_num, wine_dbgstr_longlong(frame_num), wine_dbgstr_longlong(frames));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_SetReadStreamSamples(IWMSyncReader2 *iface, WORD stream_number, BOOL compressed)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, stream_index %u, compressed %d.\n", reader, stream_number, compressed);

    return wm_reader_set_read_compressed(&reader->reader, stream_number, compressed);
}

static HRESULT WINAPI WMSyncReader_SetStreamsSelected(IWMSyncReader2 *iface,
        WORD count, WORD *stream_numbers, WMT_STREAM_SELECTION *selections)
{
    struct sync_reader *reader = impl_from_IWMSyncReader2(iface);

    TRACE("reader %p, count %u, stream_numbers %p, selections %p.\n",
            reader, count, stream_numbers, selections);

    return wm_reader_set_streams_selected(&reader->reader, count, stream_numbers, selections);
}

static HRESULT WINAPI WMSyncReader2_SetRangeByTimecode(IWMSyncReader2 *iface, WORD stream_num,
        WMT_TIMECODE_EXTENSION_DATA *start, WMT_TIMECODE_EXTENSION_DATA *end)
{
    struct sync_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%u %p %p): stub!\n", This, stream_num, start, end);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader2_SetRangeByFrameEx(IWMSyncReader2 *iface, WORD stream_num, QWORD frame_num,
        LONGLONG frames_to_read, QWORD *starttime)
{
    struct sync_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%u %s %s %p): stub!\n", This, stream_num, wine_dbgstr_longlong(frame_num),
          wine_dbgstr_longlong(frames_to_read), starttime);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader2_SetAllocateForOutput(IWMSyncReader2 *iface, DWORD output_num, IWMReaderAllocatorEx *allocator)
{
    struct sync_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%lu %p): stub!\n", This, output_num, allocator);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader2_GetAllocateForOutput(IWMSyncReader2 *iface, DWORD output_num, IWMReaderAllocatorEx **allocator)
{
    struct sync_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%lu %p): stub!\n", This, output_num, allocator);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader2_SetAllocateForStream(IWMSyncReader2 *iface, DWORD stream_num, IWMReaderAllocatorEx *allocator)
{
    struct sync_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%lu %p): stub!\n", This, stream_num, allocator);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader2_GetAllocateForStream(IWMSyncReader2 *iface, DWORD stream_num, IWMReaderAllocatorEx **allocator)
{
    struct sync_reader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%lu %p): stub!\n", This, stream_num, allocator);
    return E_NOTIMPL;
}

static const IWMSyncReader2Vtbl WMSyncReader2Vtbl = {
    WMSyncReader_QueryInterface,
    WMSyncReader_AddRef,
    WMSyncReader_Release,
    WMSyncReader_Open,
    WMSyncReader_Close,
    WMSyncReader_SetRange,
    WMSyncReader_SetRangeByFrame,
    WMSyncReader_GetNextSample,
    WMSyncReader_SetStreamsSelected,
    WMSyncReader_GetStreamSelected,
    WMSyncReader_SetReadStreamSamples,
    WMSyncReader_GetReadStreamSamples,
    WMSyncReader_GetOutputSetting,
    WMSyncReader_SetOutputSetting,
    WMSyncReader_GetOutputCount,
    WMSyncReader_GetOutputProps,
    WMSyncReader_SetOutputProps,
    WMSyncReader_GetOutputFormatCount,
    WMSyncReader_GetOutputFormat,
    WMSyncReader_GetOutputNumberForStream,
    WMSyncReader_GetStreamNumberForOutput,
    WMSyncReader_GetMaxOutputSampleSize,
    WMSyncReader_GetMaxStreamSampleSize,
    WMSyncReader_OpenStream,
    WMSyncReader2_SetRangeByTimecode,
    WMSyncReader2_SetRangeByFrameEx,
    WMSyncReader2_SetAllocateForOutput,
    WMSyncReader2_GetAllocateForOutput,
    WMSyncReader2_SetAllocateForStream,
    WMSyncReader2_GetAllocateForStream
};

static struct sync_reader *impl_from_wm_reader(struct wm_reader *iface)
{
    return CONTAINING_RECORD(iface, struct sync_reader, reader);
}

static void *sync_reader_query_interface(struct wm_reader *iface, REFIID iid)
{
    struct sync_reader *reader = impl_from_wm_reader(iface);

    TRACE("reader %p, iid %s.\n", reader, debugstr_guid(iid));

    if (IsEqualIID(iid, &IID_IWMSyncReader)
            || IsEqualIID(iid, &IID_IWMSyncReader2))
        return &reader->IWMSyncReader2_iface;

    return NULL;
}

static void sync_reader_destroy(struct wm_reader *iface)
{
    struct sync_reader *reader = impl_from_wm_reader(iface);

    TRACE("reader %p.\n", reader);

    wm_reader_close(&reader->reader);
    wm_reader_cleanup(&reader->reader);
    free(reader);
}

static const struct wm_reader_ops sync_reader_ops =
{
    .query_interface = sync_reader_query_interface,
    .destroy = sync_reader_destroy,
};

HRESULT WINAPI winegstreamer_create_wm_sync_reader(IWMSyncReader **reader)
{
    struct sync_reader *object;

    TRACE("reader %p.\n", reader);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    wm_reader_init(&object->reader, &sync_reader_ops);

    object->IWMSyncReader2_iface.lpVtbl = &WMSyncReader2Vtbl;

    TRACE("Created sync reader %p.\n", object);
    *reader = (IWMSyncReader *)&object->IWMSyncReader2_iface;
    return S_OK;
}
