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

#include "wmvcore.h"

#include "wmsdk.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmvcore);

typedef struct {
    IWMSyncReader2 IWMSyncReader2_iface;
    LONG ref;
} WMSyncReader;

static inline WMSyncReader *impl_from_IWMSyncReader2(IWMSyncReader2 *iface)
{
    return CONTAINING_RECORD(iface, WMSyncReader, IWMSyncReader2_iface);
}

static HRESULT WINAPI WMSyncReader_QueryInterface(IWMSyncReader2 *iface, REFIID riid, void **ppv)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IWMSyncReader2_iface;
    }else if(IsEqualGUID(riid, &IID_IWMSyncReader)) {
        TRACE("(%p)->(IID_IWMSyncReader %p)\n", This, ppv);
        *ppv = &This->IWMSyncReader2_iface;
    }else if(IsEqualGUID(riid, &IID_IWMSyncReader2)) {
        TRACE("(%p)->(IID_IWMSyncReader2 %p)\n", This, ppv);
        *ppv = &This->IWMSyncReader2_iface;
    }else {
        *ppv = NULL;
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WMSyncReader_AddRef(IWMSyncReader2 *iface)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI WMSyncReader_Release(IWMSyncReader2 *iface)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI WMSyncReader_Close(IWMSyncReader2 *iface)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p): stub!\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetMaxOutputSampleSize(IWMSyncReader2 *iface, DWORD output, DWORD *max)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%d %p): stub!\n", This, output, max);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetMaxStreamSampleSize(IWMSyncReader2 *iface, WORD stream, DWORD *max)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%d %p): stub!\n", This, stream, max);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetNextSample(IWMSyncReader2 *iface, WORD stream, INSSBuffer **sample,
        QWORD *sample_time, QWORD *sample_duration, DWORD *flags, DWORD *output_num, WORD *stream_num)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%d %p %p %p %p %p %p): stub!\n", This, stream, sample, sample_time,
          sample_duration, flags, output_num, stream_num);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetOutputCount(IWMSyncReader2 *iface, DWORD *outputs)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%p): stub!\n", This, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetOutputFormat(IWMSyncReader2 *iface, DWORD output_num, DWORD format_num,
        IWMOutputMediaProps **props)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%u %u %p): stub!\n", This, output_num, format_num, props);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetOutputFormatCount(IWMSyncReader2 *iface, DWORD output_num, DWORD *formats)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%u %p): stub!\n", This, output_num, formats);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetOutputNumberForStream(IWMSyncReader2 *iface, WORD stream_num, DWORD *output_num)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%u %p): stub!\n", This, stream_num, output_num);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetOutputProps(IWMSyncReader2 *iface, DWORD output_num, IWMOutputMediaProps **output)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%u %p): stub!\n", This, output_num, output);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetOutputSetting(IWMSyncReader2 *iface, DWORD output_num, const WCHAR *name,
        WMT_ATTR_DATATYPE *type, BYTE *value, WORD *length)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%u %s %p %p %p): stub!\n", This, output_num, debugstr_w(name), type, value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetReadStreamSamples(IWMSyncReader2 *iface, WORD stream_num, BOOL *compressed)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%d %p): stub!\n", This, stream_num, compressed);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_GetStreamNumberForOutput(IWMSyncReader2 *iface, DWORD output, WORD *stream_num)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%u %p): stub!\n", This, output, stream_num);
    return S_OK;
}

static HRESULT WINAPI WMSyncReader_GetStreamSelected(IWMSyncReader2 *iface, WORD stream_num, WMT_STREAM_SELECTION *selection)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%d %p): stub!\n", This, stream_num, selection);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_Open(IWMSyncReader2 *iface, const WCHAR *filename)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%s): stub!\n", This, debugstr_w(filename));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_OpenStream(IWMSyncReader2 *iface, IStream *stream)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%p): stub!\n", This, stream);
    return S_OK;
}

static HRESULT WINAPI WMSyncReader_SetOutputProps(IWMSyncReader2 *iface, DWORD output_num, IWMOutputMediaProps *output)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%u %p): stub!\n", This, output_num, output);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_SetOutputSetting(IWMSyncReader2 *iface, DWORD output_num, const WCHAR *name,
        WMT_ATTR_DATATYPE type, const BYTE *value, WORD length)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%d %s %d %p %d): stub!\n", This, output_num, debugstr_w(name), type, value, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_SetRange(IWMSyncReader2 *iface, QWORD start, LONGLONG duration)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%s %s): stub!\n", This, wine_dbgstr_longlong(start), wine_dbgstr_longlong(duration));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_SetRangeByFrame(IWMSyncReader2 *iface, WORD stream_num, QWORD frame_num,
        LONGLONG frames)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%d %s %s): stub!\n", This, stream_num, wine_dbgstr_longlong(frame_num), wine_dbgstr_longlong(frames));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_SetReadStreamSamples(IWMSyncReader2 *iface, WORD stream_num, BOOL compressed)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%u %x): stub!\n", This, stream_num, compressed);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader_SetStreamsSelected(IWMSyncReader2 *iface, WORD stream_count,
        WORD *stream_numbers, WMT_STREAM_SELECTION *selections)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%d %p %p): stub!\n", This, stream_count, stream_numbers, selections);
    return S_OK;
}

static HRESULT WINAPI WMSyncReader2_SetRangeByTimecode(IWMSyncReader2 *iface, WORD stream_num,
        WMT_TIMECODE_EXTENSION_DATA *start, WMT_TIMECODE_EXTENSION_DATA *end)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%u %p %p): stub!\n", This, stream_num, start, end);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader2_SetRangeByFrameEx(IWMSyncReader2 *iface, WORD stream_num, QWORD frame_num,
        LONGLONG frames_to_read, QWORD *starttime)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%u %s %s %p): stub!\n", This, stream_num, wine_dbgstr_longlong(frame_num),
          wine_dbgstr_longlong(frames_to_read), starttime);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader2_SetAllocateForOutput(IWMSyncReader2 *iface, DWORD output_num, IWMReaderAllocatorEx *allocator)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%d %p): stub!\n", This, output_num, allocator);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader2_GetAllocateForOutput(IWMSyncReader2 *iface, DWORD output_num, IWMReaderAllocatorEx **allocator)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%d %p): stub!\n", This, output_num, allocator);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader2_SetAllocateForStream(IWMSyncReader2 *iface, DWORD stream_num, IWMReaderAllocatorEx *allocator)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%d %p): stub!\n", This, stream_num, allocator);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMSyncReader2_GetAllocateForStream(IWMSyncReader2 *iface, DWORD stream_num, IWMReaderAllocatorEx **allocator)
{
    WMSyncReader *This = impl_from_IWMSyncReader2(iface);
    FIXME("(%p)->(%d %p): stub!\n", This, stream_num, allocator);
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

HRESULT WINAPI WMCreateSyncReader(IUnknown *pcert, DWORD rights, IWMSyncReader **syncreader)
{
    WMSyncReader *sync;

    TRACE("(%p, %x, %p)\n", pcert, rights, syncreader);

    sync = heap_alloc(sizeof(*sync));

    if (!sync)
        return E_OUTOFMEMORY;

    sync->IWMSyncReader2_iface.lpVtbl = &WMSyncReader2Vtbl;
    sync->ref = 1;

    *syncreader = (IWMSyncReader *)&sync->IWMSyncReader2_iface;

    return S_OK;
}

HRESULT WINAPI WMCreateSyncReaderPriv(IWMSyncReader **syncreader)
{
    return WMCreateSyncReader(NULL, 0, syncreader);
}
