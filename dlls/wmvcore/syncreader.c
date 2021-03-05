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
    IWMProfile3 IWMProfile3_iface;
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
    }else if(IsEqualGUID(riid, &IID_IWMProfile)) {
        TRACE("(%p)->(IID_IWMProfile %p)\n", This, ppv);
        *ppv = &This->IWMProfile3_iface;
    }else if(IsEqualGUID(riid, &IID_IWMProfile2)) {
        TRACE("(%p)->(IID_IWMProfile2 %p)\n", This, ppv);
        *ppv = &This->IWMProfile3_iface;
    }else if(IsEqualGUID(riid, &IID_IWMProfile3)) {
        TRACE("(%p)->(IID_IWMProfile3 %p)\n", This, ppv);
        *ppv = &This->IWMProfile3_iface;
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

static inline WMSyncReader *impl_from_IWMProfile3(IWMProfile3 *iface)
{
    return CONTAINING_RECORD(iface, WMSyncReader, IWMProfile3_iface);
}

static HRESULT WINAPI WMProfile_QueryInterface(IWMProfile3 *iface, REFIID riid, void **ppv)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    return IWMSyncReader2_QueryInterface(&This->IWMSyncReader2_iface, riid, ppv);
}

static ULONG WINAPI WMProfile_AddRef(IWMProfile3 *iface)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    return IWMSyncReader2_AddRef(&This->IWMSyncReader2_iface);
}

static ULONG WINAPI WMProfile_Release(IWMProfile3 *iface)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    return IWMSyncReader2_Release(&This->IWMSyncReader2_iface);
}

static HRESULT WINAPI WMProfile_GetVersion(IWMProfile3 *iface, WMT_VERSION *version)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, version);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile_GetName(IWMProfile3 *iface, WCHAR *name, DWORD *length)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p, %p\n", This, name, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile_SetName(IWMProfile3 *iface, const WCHAR *name)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %s\n", This, debugstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile_GetDescription(IWMProfile3 *iface, WCHAR *description, DWORD *length)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p, %p\n", This, description, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile_SetDescription(IWMProfile3 *iface, const WCHAR *description)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %s\n", This, debugstr_w(description));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile_GetStreamCount(IWMProfile3 *iface, DWORD *count)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, count);

    if (!count)
        return E_INVALIDARG;

    *count = 0;
    return S_OK;
}

static HRESULT WINAPI WMProfile_GetStream(IWMProfile3 *iface, DWORD index, IWMStreamConfig **config)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %d, %p\n", This, index, config);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile_GetStreamByNumber(IWMProfile3 *iface, WORD stream, IWMStreamConfig **config)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %d, %p\n", This, stream, config);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile_RemoveStream(IWMProfile3 *iface, IWMStreamConfig *config)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, config);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile_RemoveStreamByNumber(IWMProfile3 *iface, WORD stream)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %d\n", This, stream);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile_AddStream(IWMProfile3 *iface, IWMStreamConfig *config)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, config);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile_ReconfigStream(IWMProfile3 *iface, IWMStreamConfig *config)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, config);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile_CreateNewStream(IWMProfile3 *iface, REFGUID type, IWMStreamConfig **config)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %s, %p\n", This, debugstr_guid(type), config);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile_GetMutualExclusionCount(IWMProfile3 *iface, DWORD *count)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile_GetMutualExclusion(IWMProfile3 *iface, DWORD index, IWMMutualExclusion **mutual)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %d, %p\n", This, index, mutual);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile_RemoveMutualExclusion(IWMProfile3 *iface, IWMMutualExclusion *mutual)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, mutual);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile_AddMutualExclusion(IWMProfile3 *iface, IWMMutualExclusion *mutual)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, mutual);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile_CreateNewMutualExclusion(IWMProfile3 *iface, IWMMutualExclusion **mutual)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, mutual);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile2_GetProfileID(IWMProfile3 *iface, GUID *guid)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, guid);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile3_GetStorageFormat(IWMProfile3 *iface, WMT_STORAGE_FORMAT *storage)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, storage);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile3_SetStorageFormat(IWMProfile3 *iface, WMT_STORAGE_FORMAT storage)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %d\n", This, storage);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile3_GetBandwidthSharingCount(IWMProfile3 *iface, DWORD *count)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile3_GetBandwidthSharing(IWMProfile3 *iface, DWORD index, IWMBandwidthSharing **bandwidth)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %d, %p\n", This, index, bandwidth);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile3_RemoveBandwidthSharing( IWMProfile3 *iface, IWMBandwidthSharing *bandwidth)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, bandwidth);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile3_AddBandwidthSharing(IWMProfile3 *iface, IWMBandwidthSharing *bandwidth)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, bandwidth);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile3_CreateNewBandwidthSharing( IWMProfile3 *iface, IWMBandwidthSharing **bandwidth)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, bandwidth);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile3_GetStreamPrioritization(IWMProfile3 *iface, IWMStreamPrioritization **stream)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, stream);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile3_SetStreamPrioritization(IWMProfile3 *iface, IWMStreamPrioritization *stream)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, stream);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile3_RemoveStreamPrioritization(IWMProfile3 *iface)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile3_CreateNewStreamPrioritization(IWMProfile3 *iface, IWMStreamPrioritization **stream)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %p\n", This, stream);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfile3_GetExpectedPacketCount(IWMProfile3 *iface, QWORD duration, QWORD *packets)
{
    WMSyncReader *This = impl_from_IWMProfile3(iface);
    FIXME("%p, %s, %p\n", This, wine_dbgstr_longlong(duration), packets);
    return E_NOTIMPL;
}

static const IWMProfile3Vtbl WMProfile3Vtbl =
{
    WMProfile_QueryInterface,
    WMProfile_AddRef,
    WMProfile_Release,
    WMProfile_GetVersion,
    WMProfile_GetName,
    WMProfile_SetName,
    WMProfile_GetDescription,
    WMProfile_SetDescription,
    WMProfile_GetStreamCount,
    WMProfile_GetStream,
    WMProfile_GetStreamByNumber,
    WMProfile_RemoveStream,
    WMProfile_RemoveStreamByNumber,
    WMProfile_AddStream,
    WMProfile_ReconfigStream,
    WMProfile_CreateNewStream,
    WMProfile_GetMutualExclusionCount,
    WMProfile_GetMutualExclusion,
    WMProfile_RemoveMutualExclusion,
    WMProfile_AddMutualExclusion,
    WMProfile_CreateNewMutualExclusion,
    WMProfile2_GetProfileID,
    WMProfile3_GetStorageFormat,
    WMProfile3_SetStorageFormat,
    WMProfile3_GetBandwidthSharingCount,
    WMProfile3_GetBandwidthSharing,
    WMProfile3_RemoveBandwidthSharing,
    WMProfile3_AddBandwidthSharing,
    WMProfile3_CreateNewBandwidthSharing,
    WMProfile3_GetStreamPrioritization,
    WMProfile3_SetStreamPrioritization,
    WMProfile3_RemoveStreamPrioritization,
    WMProfile3_CreateNewStreamPrioritization,
    WMProfile3_GetExpectedPacketCount
};

HRESULT WINAPI WMCreateSyncReader(IUnknown *pcert, DWORD rights, IWMSyncReader **syncreader)
{
    WMSyncReader *sync;

    TRACE("(%p, %x, %p)\n", pcert, rights, syncreader);

    sync = heap_alloc(sizeof(*sync));

    if (!sync)
        return E_OUTOFMEMORY;

    sync->IWMProfile3_iface.lpVtbl = &WMProfile3Vtbl;
    sync->IWMSyncReader2_iface.lpVtbl = &WMSyncReader2Vtbl;
    sync->ref = 1;

    *syncreader = (IWMSyncReader *)&sync->IWMSyncReader2_iface;

    return S_OK;
}

HRESULT WINAPI WMCreateSyncReaderPriv(IWMSyncReader **syncreader)
{
    return WMCreateSyncReader(NULL, 0, syncreader);
}
