/*
 * Copyright 2013 Jacek Caban for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "vfw.h"
#include "aviriff.h"

#include "qcap_main.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

typedef struct {
    BaseFilter filter;
    IPersistPropertyBag IPersistPropertyBag_iface;

    BaseInputPin sink;
    BaseOutputPin source;

    DWORD fcc_handler;
    HIC hic;

    VIDEOINFOHEADER *videoinfo;
    size_t videoinfo_size;
    DWORD driver_flags;
    DWORD max_frame_size;

    DWORD frame_cnt;
} AVICompressor;

static inline AVICompressor *impl_from_BaseFilter(BaseFilter *filter)
{
    return CONTAINING_RECORD(filter, AVICompressor, filter);
}

static inline AVICompressor *impl_from_IBaseFilter(IBaseFilter *iface)
{
    BaseFilter *filter = CONTAINING_RECORD(iface, BaseFilter, IBaseFilter_iface);
    return impl_from_BaseFilter(filter);
}

static inline AVICompressor *impl_from_BasePin(BasePin *pin)
{
    return impl_from_IBaseFilter(pin->pinInfo.pFilter);
}

static HRESULT ensure_driver(AVICompressor *This)
{
    if(This->hic)
        return S_OK;

    This->hic = ICOpen(FCC('v','i','d','c'), This->fcc_handler, ICMODE_COMPRESS);
    if(!This->hic) {
        FIXME("ICOpen failed\n");
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT fill_format_info(AVICompressor *This, VIDEOINFOHEADER *src_videoinfo)
{
    DWORD size;
    ICINFO icinfo;
    HRESULT hres;

    hres = ensure_driver(This);
    if(hres != S_OK)
        return hres;

    size = ICGetInfo(This->hic, &icinfo, sizeof(icinfo));
    if(size != sizeof(icinfo))
        return E_FAIL;

    size = ICCompressGetFormatSize(This->hic, &src_videoinfo->bmiHeader);
    if(!size) {
        FIXME("ICCompressGetFormatSize failed\n");
        return E_FAIL;
    }

    size += FIELD_OFFSET(VIDEOINFOHEADER, bmiHeader);
    This->videoinfo = heap_alloc(size);
    if(!This->videoinfo)
        return E_OUTOFMEMORY;

    This->videoinfo_size = size;
    This->driver_flags = icinfo.dwFlags;
    memset(This->videoinfo, 0, sizeof(*This->videoinfo));
    ICCompressGetFormat(This->hic, &src_videoinfo->bmiHeader, &This->videoinfo->bmiHeader);

    This->videoinfo->dwBitRate = 10000000/src_videoinfo->AvgTimePerFrame * This->videoinfo->bmiHeader.biSizeImage * 8;
    This->videoinfo->AvgTimePerFrame = src_videoinfo->AvgTimePerFrame;
    This->max_frame_size = This->videoinfo->bmiHeader.biSizeImage;
    return S_OK;
}

static HRESULT WINAPI AVICompressor_Stop(IBaseFilter *iface)
{
    AVICompressor *This = impl_from_IBaseFilter(iface);

    TRACE("(%p)\n", This);

    if(This->filter.state == State_Stopped)
        return S_OK;

    ICCompressEnd(This->hic);
    This->filter.state = State_Stopped;
    return S_OK;
}

static HRESULT WINAPI AVICompressor_Pause(IBaseFilter *iface)
{
    AVICompressor *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI AVICompressor_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    AVICompressor *This = impl_from_IBaseFilter(iface);
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, wine_dbgstr_longlong(tStart));

    if(This->filter.state == State_Running)
        return S_OK;

    hres = IMemAllocator_Commit(This->source.pAllocator);
    if(FAILED(hres)) {
        FIXME("Commit failed: %08x\n", hres);
        return hres;
    }

    This->frame_cnt = 0;

    This->filter.state = State_Running;
    return S_OK;
}

static const IBaseFilterVtbl AVICompressorVtbl = {
    BaseFilterImpl_QueryInterface,
    BaseFilterImpl_AddRef,
    BaseFilterImpl_Release,
    BaseFilterImpl_GetClassID,
    AVICompressor_Stop,
    AVICompressor_Pause,
    AVICompressor_Run,
    BaseFilterImpl_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    BaseFilterImpl_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo,
};

static IPin *avi_compressor_get_pin(BaseFilter *iface, unsigned int index)
{
    AVICompressor *filter = impl_from_BaseFilter(iface);

    if (index == 0)
        return &filter->sink.pin.IPin_iface;
    else if (index == 1)
        return &filter->source.pin.IPin_iface;
    return NULL;
}

static void avi_compressor_destroy(BaseFilter *iface)
{
    AVICompressor *filter = impl_from_BaseFilter(iface);

    if (filter->hic)
        ICClose(filter->hic);
    heap_free(filter->videoinfo);
    strmbase_sink_cleanup(&filter->sink);
    strmbase_source_cleanup(&filter->source);
    strmbase_filter_cleanup(&filter->filter);
    heap_free(filter);
}

static HRESULT avi_compressor_query_interface(BaseFilter *iface, REFIID iid, void **out)
{
    AVICompressor *filter = impl_from_BaseFilter(iface);

    if (IsEqualGUID(iid, &IID_IPersistPropertyBag))
        *out = &filter->IPersistPropertyBag_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static const BaseFilterFuncTable filter_func_table = {
    .filter_get_pin = avi_compressor_get_pin,
    .filter_destroy = avi_compressor_destroy,
    .filter_query_interface = avi_compressor_query_interface,
};

static AVICompressor *impl_from_IPersistPropertyBag(IPersistPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, AVICompressor, IPersistPropertyBag_iface);
}

static HRESULT WINAPI AVICompressorPropertyBag_QueryInterface(IPersistPropertyBag *iface, REFIID riid, void **ppv)
{
    AVICompressor *This = impl_from_IPersistPropertyBag(iface);
    return IBaseFilter_QueryInterface(&This->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI AVICompressorPropertyBag_AddRef(IPersistPropertyBag *iface)
{
    AVICompressor *This = impl_from_IPersistPropertyBag(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI AVICompressorPropertyBag_Release(IPersistPropertyBag *iface)
{
    AVICompressor *This = impl_from_IPersistPropertyBag(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI AVICompressorPropertyBag_GetClassID(IPersistPropertyBag *iface, CLSID *pClassID)
{
    AVICompressor *This = impl_from_IPersistPropertyBag(iface);
    return IBaseFilter_GetClassID(&This->filter.IBaseFilter_iface, pClassID);
}

static HRESULT WINAPI AVICompressorPropertyBag_InitNew(IPersistPropertyBag *iface)
{
    AVICompressor *This = impl_from_IPersistPropertyBag(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI AVICompressorPropertyBag_Load(IPersistPropertyBag *iface, IPropertyBag *pPropBag, IErrorLog *pErrorLog)
{
    AVICompressor *This = impl_from_IPersistPropertyBag(iface);
    BSTR str;
    VARIANT v;
    HRESULT hres;

    static const WCHAR fcc_handlerW[] = {'F','c','c','H','a','n','d','l','e','r',0};

    TRACE("(%p)->(%p %p)\n", This, pPropBag, pErrorLog);

    V_VT(&v) = VT_BSTR;
    hres = IPropertyBag_Read(pPropBag, fcc_handlerW, &v, NULL);
    if(FAILED(hres)) {
        WARN("Could not read FccHandler: %08x\n", hres);
        return hres;
    }

    if(V_VT(&v) != VT_BSTR) {
        FIXME("Got vt %d\n", V_VT(&v));
        VariantClear(&v);
        return E_FAIL;
    }

    str = V_BSTR(&v);
    TRACE("FccHandler = %s\n", debugstr_w(str));
    if(SysStringLen(str) != 4) {
        FIXME("Invalid FccHandler len\n");
        SysFreeString(str);
        return E_FAIL;
    }

    This->fcc_handler = FCC(str[0], str[1], str[2], str[3]);
    SysFreeString(str);
    return S_OK;
}

static HRESULT WINAPI AVICompressorPropertyBag_Save(IPersistPropertyBag *iface, IPropertyBag *pPropBag,
        BOOL fClearDirty, BOOL fSaveAllProperties)
{
    AVICompressor *This = impl_from_IPersistPropertyBag(iface);
    FIXME("(%p)->(%p %x %x)\n", This, pPropBag, fClearDirty, fSaveAllProperties);
    return E_NOTIMPL;
}

static const IPersistPropertyBagVtbl PersistPropertyBagVtbl = {
    AVICompressorPropertyBag_QueryInterface,
    AVICompressorPropertyBag_AddRef,
    AVICompressorPropertyBag_Release,
    AVICompressorPropertyBag_GetClassID,
    AVICompressorPropertyBag_InitNew,
    AVICompressorPropertyBag_Load,
    AVICompressorPropertyBag_Save
};

static inline AVICompressor *impl_from_IPin(IPin *iface)
{
    BasePin *bp = CONTAINING_RECORD(iface, BasePin, IPin_iface);
    return impl_from_IBaseFilter(bp->pinInfo.pFilter);
}

static HRESULT WINAPI AVICompressorIn_QueryInterface(IPin *iface, REFIID riid, void **ppv)
{
    return BaseInputPinImpl_QueryInterface(iface, riid, ppv);
}

static HRESULT WINAPI AVICompressorIn_ReceiveConnection(IPin *iface,
        IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
    AVICompressor *This = impl_from_IPin(iface);
    HRESULT hres;

    TRACE("(%p)->(%p AM_MEDIA_TYPE(%p))\n", This, pConnector, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    hres = BaseInputPinImpl_ReceiveConnection(iface, pConnector, pmt);
    if(FAILED(hres))
        return hres;

    hres = fill_format_info(This, (VIDEOINFOHEADER*)pmt->pbFormat);
    if(FAILED(hres))
        BasePinImpl_Disconnect(iface);
    return hres;
}

static HRESULT WINAPI AVICompressorIn_Disconnect(IPin *iface)
{
    AVICompressor *This = impl_from_IPin(iface);
    HRESULT hres;

    TRACE("(%p)\n", This);

    hres = BasePinImpl_Disconnect(iface);
    if(FAILED(hres))
        return hres;

    heap_free(This->videoinfo);
    This->videoinfo = NULL;
    return S_OK;
}

static const IPinVtbl AVICompressorInputPinVtbl = {
    AVICompressorIn_QueryInterface,
    BasePinImpl_AddRef,
    BasePinImpl_Release,
    BaseInputPinImpl_Connect,
    AVICompressorIn_ReceiveConnection,
    AVICompressorIn_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    BasePinImpl_QueryAccept,
    BasePinImpl_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    BaseInputPinImpl_EndOfStream,
    BaseInputPinImpl_BeginFlush,
    BaseInputPinImpl_EndFlush,
    BaseInputPinImpl_NewSegment
};

static HRESULT WINAPI AVICompressorIn_CheckMediaType(BasePin *base, const AM_MEDIA_TYPE *pmt)
{
    AVICompressor *This = impl_from_BasePin(base);
    VIDEOINFOHEADER *videoinfo;
    HRESULT hres;
    DWORD res;

    TRACE("(%p)->(AM_MEDIA_TYPE(%p))\n", base, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    if(!IsEqualIID(&pmt->majortype, &MEDIATYPE_Video))
        return S_FALSE;

    if(!IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo)) {
        FIXME("formattype %s unsupported\n", debugstr_guid(&pmt->formattype));
        return S_FALSE;
    }

    hres = ensure_driver(This);
    if(hres != S_OK)
        return hres;

    videoinfo = (VIDEOINFOHEADER*)pmt->pbFormat;
    res = ICCompressQuery(This->hic, &videoinfo->bmiHeader, NULL);
    return res == ICERR_OK ? S_OK : S_FALSE;
}

static HRESULT WINAPI AVICompressorIn_GetMediaType(BasePin *base, int iPosition, AM_MEDIA_TYPE *amt)
{
    TRACE("(%p)->(%d %p)\n", base, iPosition, amt);
    return S_FALSE;
}

static HRESULT WINAPI AVICompressorIn_Receive(BaseInputPin *base, IMediaSample *pSample)
{
    AVICompressor *This = impl_from_BasePin(&base->pin);
    VIDEOINFOHEADER *src_videoinfo;
    REFERENCE_TIME start, stop;
    IMediaSample *out_sample;
    AM_MEDIA_TYPE *mt;
    IMediaSample2 *sample2;
    DWORD comp_flags = 0;
    BOOL is_preroll;
    BOOL sync_point;
    BYTE *ptr, *buf;
    DWORD res;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", base, pSample);

    if(!This->hic) {
        FIXME("Driver not loaded\n");
        return E_UNEXPECTED;
    }

    hres = IMediaSample_QueryInterface(pSample, &IID_IMediaSample2, (void**)&sample2);
    if(SUCCEEDED(hres)) {
        FIXME("Use IMediaSample2\n");
        IMediaSample2_Release(sample2);
    }

    is_preroll = IMediaSample_IsPreroll(pSample) == S_OK;
    sync_point = IMediaSample_IsSyncPoint(pSample) == S_OK;

    hres = IMediaSample_GetTime(pSample, &start, &stop);
    if(FAILED(hres)) {
        WARN("GetTime failed: %08x\n", hres);
        return hres;
    }

    hres = IMediaSample_GetMediaType(pSample, &mt);
    if(FAILED(hres))
        return hres;

    hres = IMediaSample_GetPointer(pSample, &ptr);
    if(FAILED(hres)) {
        WARN("GetPointer failed: %08x\n", hres);
        return hres;
    }

    hres = BaseOutputPinImpl_GetDeliveryBuffer(&This->source, &out_sample, &start, &stop, 0);
    if(FAILED(hres))
        return hres;

    hres = IMediaSample_GetPointer(out_sample, &buf);
    if(FAILED(hres))
        return hres;

    if((This->driver_flags & VIDCF_TEMPORAL) && !(This->driver_flags & VIDCF_FASTTEMPORALC))
        FIXME("Unsupported temporal compression\n");

    src_videoinfo = (VIDEOINFOHEADER *)This->sink.pin.mtCurrent.pbFormat;
    This->videoinfo->bmiHeader.biSizeImage = This->max_frame_size;
    res = ICCompress(This->hic, sync_point ? ICCOMPRESS_KEYFRAME : 0, &This->videoinfo->bmiHeader, buf,
            &src_videoinfo->bmiHeader, ptr, 0, &comp_flags, This->frame_cnt, 0, 0, NULL, NULL);
    if(res != ICERR_OK) {
        WARN("ICCompress failed: %d\n", res);
        IMediaSample_Release(out_sample);
        return E_FAIL;
    }

    IMediaSample_SetActualDataLength(out_sample, This->videoinfo->bmiHeader.biSizeImage);
    IMediaSample_SetPreroll(out_sample, is_preroll);
    IMediaSample_SetSyncPoint(out_sample, (comp_flags&AVIIF_KEYFRAME) != 0);
    IMediaSample_SetDiscontinuity(out_sample, (IMediaSample_IsDiscontinuity(pSample) == S_OK));

    if (IMediaSample_GetMediaTime(pSample, &start, &stop) == S_OK)
        IMediaSample_SetMediaTime(out_sample, &start, &stop);
    else
        IMediaSample_SetMediaTime(out_sample, NULL, NULL);

    hres = BaseOutputPinImpl_Deliver(&This->source, out_sample);
    if(FAILED(hres))
        WARN("Deliver failed: %08x\n", hres);

    IMediaSample_Release(out_sample);
    This->frame_cnt++;
    return hres;
}

static const BaseInputPinFuncTable AVICompressorBaseInputPinVtbl = {
    {
        AVICompressorIn_CheckMediaType,
        AVICompressorIn_GetMediaType
    },
    AVICompressorIn_Receive
};

static HRESULT WINAPI AVICompressorOut_QueryInterface(IPin *iface, REFIID riid, void **ppv)
{
    return BaseInputPinImpl_QueryInterface(iface, riid, ppv);
}

static const IPinVtbl AVICompressorOutputPinVtbl = {
    AVICompressorOut_QueryInterface,
    BasePinImpl_AddRef,
    BasePinImpl_Release,
    BaseOutputPinImpl_Connect,
    BaseOutputPinImpl_ReceiveConnection,
    BaseOutputPinImpl_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    BasePinImpl_QueryAccept,
    BasePinImpl_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    BaseOutputPinImpl_EndOfStream,
    BaseOutputPinImpl_BeginFlush,
    BaseOutputPinImpl_EndFlush,
    BasePinImpl_NewSegment
};

static HRESULT WINAPI AVICompressorOut_GetMediaType(BasePin *base, int iPosition, AM_MEDIA_TYPE *amt)
{
    AVICompressor *This = impl_from_IBaseFilter(base->pinInfo.pFilter);

    TRACE("(%p)->(%d %p)\n", base, iPosition, amt);

    if(iPosition || !This->videoinfo)
        return S_FALSE;

    amt->majortype = MEDIATYPE_Video;
    amt->subtype = MEDIASUBTYPE_PCM;
    amt->bFixedSizeSamples = FALSE;
    amt->bTemporalCompression = (This->driver_flags & VIDCF_TEMPORAL) != 0;
    amt->lSampleSize = This->sink.pin.mtCurrent.lSampleSize;
    amt->formattype = FORMAT_VideoInfo;
    amt->pUnk = NULL;
    amt->cbFormat = This->videoinfo_size;
    amt->pbFormat = (BYTE*)This->videoinfo;
    return S_OK;
}

static HRESULT WINAPI AVICompressorOut_DecideBufferSize(BaseOutputPin *base, IMemAllocator *alloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    AVICompressor *This = impl_from_BasePin(&base->pin);
    ALLOCATOR_PROPERTIES actual;

    TRACE("(%p)\n", This);

    if (!ppropInputRequest->cBuffers)
        ppropInputRequest->cBuffers = 1;
    if (ppropInputRequest->cbBuffer < This->max_frame_size)
        ppropInputRequest->cbBuffer = This->max_frame_size;
    if (!ppropInputRequest->cbAlign)
        ppropInputRequest->cbAlign = 1;

    return IMemAllocator_SetProperties(alloc, ppropInputRequest, &actual);
}

static HRESULT WINAPI AVICompressorOut_DecideAllocator(BaseOutputPin *base,
        IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    TRACE("(%p)->(%p %p)\n", base, pPin, pAlloc);
    return BaseOutputPinImpl_DecideAllocator(base, pPin, pAlloc);
}

static const BaseOutputPinFuncTable AVICompressorBaseOutputPinVtbl = {
    {
        NULL,
        AVICompressorOut_GetMediaType
    },
    BaseOutputPinImpl_AttemptConnection,
    AVICompressorOut_DecideBufferSize,
    AVICompressorOut_DecideAllocator,
};

IUnknown* WINAPI QCAP_createAVICompressor(IUnknown *outer, HRESULT *phr)
{
    PIN_INFO in_pin_info  = {NULL, PINDIR_INPUT,  {'I','n',0}};
    PIN_INFO out_pin_info = {NULL, PINDIR_OUTPUT, {'O','u','t',0}};
    AVICompressor *compressor;

    compressor = heap_alloc_zero(sizeof(*compressor));
    if(!compressor) {
        *phr = E_NOINTERFACE;
        return NULL;
    }

    strmbase_filter_init(&compressor->filter, &AVICompressorVtbl, outer, &CLSID_AVICo,
            (DWORD_PTR)(__FILE__ ": AVICompressor.csFilter"), &filter_func_table);

    compressor->IPersistPropertyBag_iface.lpVtbl = &PersistPropertyBagVtbl;

    in_pin_info.pFilter = &compressor->filter.IBaseFilter_iface;
    strmbase_sink_init(&compressor->sink, &AVICompressorInputPinVtbl, &in_pin_info,
            &AVICompressorBaseInputPinVtbl, &compressor->filter.csFilter, NULL);

    out_pin_info.pFilter = &compressor->filter.IBaseFilter_iface;
    strmbase_source_init(&compressor->source, &AVICompressorOutputPinVtbl, &out_pin_info,
            &AVICompressorBaseOutputPinVtbl, &compressor->filter.csFilter);

    *phr = S_OK;
    return &compressor->filter.IUnknown_inner;
}
