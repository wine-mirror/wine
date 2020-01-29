/*
 * Transform Filter (Base for decoders, etc...)
 *
 * Copyright 2005 Christian Costa
 * Copyright 2010 Aric Stewart, CodeWeavers
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

#include "strmbase_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

static const WCHAR wcsInputPinName[] = {'I','n',0};
static const WCHAR wcsOutputPinName[] = {'O','u','t',0};

static inline TransformFilter *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, TransformFilter, filter);
}

static inline TransformFilter *impl_from_sink_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, TransformFilter, sink.pin.IPin_iface);
}

static HRESULT sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *pmt)
{
    return S_OK;
}

static HRESULT WINAPI TransformFilter_Input_Receive(struct strmbase_sink *This, IMediaSample *pInSample)
{
    TransformFilter *pTransform = impl_from_sink_IPin(&This->pin.IPin_iface);
    HRESULT hr;

    TRACE("%p\n", This);

    /* We do not expect pin connection state to change while the filter is
     * running. This guarantee is necessary, since otherwise we would have to
     * take the filter lock, and we can't take the filter lock from a streaming
     * thread. */
    if (!pTransform->source.pMemInputPin)
    {
        WARN("Source is not connected, returning VFW_E_NOT_CONNECTED.\n");
        return VFW_E_NOT_CONNECTED;
    }

    EnterCriticalSection(&pTransform->csReceive);
    if (pTransform->filter.state == State_Stopped)
    {
        LeaveCriticalSection(&pTransform->csReceive);
        return VFW_E_WRONG_STATE;
    }

    if (This->flushing)
    {
        LeaveCriticalSection(&pTransform->csReceive);
        return S_FALSE;
    }

    if (pTransform->pFuncsTable->pfnReceive)
        hr = pTransform->pFuncsTable->pfnReceive(pTransform, pInSample);
    else
        hr = S_FALSE;

    LeaveCriticalSection(&pTransform->csReceive);
    return hr;
}

static inline TransformFilter *impl_from_source_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, TransformFilter, source.pin.IPin_iface);
}

static HRESULT source_query_accept(struct strmbase_pin *This, const AM_MEDIA_TYPE *pmt)
{
    TransformFilter *pTransformFilter = impl_from_source_IPin(&This->IPin_iface);
    AM_MEDIA_TYPE* outpmt = &pTransformFilter->pmt;

    if (IsEqualIID(&pmt->majortype, &outpmt->majortype)
        && (IsEqualIID(&pmt->subtype, &outpmt->subtype) || IsEqualIID(&outpmt->subtype, &GUID_NULL)))
        return S_OK;
    return S_FALSE;
}

static HRESULT WINAPI TransformFilter_Output_DecideBufferSize(struct strmbase_source *This,
        IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    TransformFilter *pTransformFilter = impl_from_source_IPin(&This->pin.IPin_iface);
    return pTransformFilter->pFuncsTable->pfnDecideBufferSize(pTransformFilter, pAlloc, ppropInputRequest);
}

static HRESULT source_get_media_type(struct strmbase_pin *This, unsigned int iPosition, AM_MEDIA_TYPE *pmt)
{
    TransformFilter *pTransform = impl_from_source_IPin(&This->IPin_iface);

    if (iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;
    CopyMediaType(pmt, &pTransform->pmt);
    return S_OK;
}

static struct strmbase_pin *transform_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    TransformFilter *filter = impl_from_strmbase_filter(iface);

    if (index == 0)
        return &filter->sink.pin;
    else if (index == 1)
        return &filter->source.pin;
    return NULL;
}

static void transform_destroy(struct strmbase_filter *iface)
{
    TransformFilter *filter = impl_from_strmbase_filter(iface);

    if (filter->sink.pin.peer)
        IPin_Disconnect(filter->sink.pin.peer);
    IPin_Disconnect(&filter->sink.pin.IPin_iface);

    if (filter->source.pin.peer)
        IPin_Disconnect(filter->source.pin.peer);
    IPin_Disconnect(&filter->source.pin.IPin_iface);

    strmbase_sink_cleanup(&filter->sink);
    strmbase_source_cleanup(&filter->source);

    filter->csReceive.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&filter->csReceive);
    FreeMediaType(&filter->pmt);
    IUnknown_Release(filter->seekthru_unk);
    strmbase_filter_cleanup(&filter->filter);
    CoTaskMemFree(filter);
}

static HRESULT transform_init_stream(struct strmbase_filter *iface)
{
    TransformFilter *filter = impl_from_strmbase_filter(iface);
    HRESULT hr = S_OK;

    EnterCriticalSection(&filter->csReceive);

    if (filter->pFuncsTable->pfnStartStreaming)
        hr = filter->pFuncsTable->pfnStartStreaming(filter);
    if (SUCCEEDED(hr))
        hr = BaseOutputPinImpl_Active(&filter->source);

    LeaveCriticalSection(&filter->csReceive);

    return hr;
}

static HRESULT transform_cleanup_stream(struct strmbase_filter *iface)
{
    TransformFilter *filter = impl_from_strmbase_filter(iface);
    HRESULT hr = S_OK;

    EnterCriticalSection(&filter->csReceive);

    if (filter->pFuncsTable->pfnStopStreaming)
        hr = filter->pFuncsTable->pfnStopStreaming(filter);
    if (SUCCEEDED(hr))
        hr = BaseOutputPinImpl_Inactive(&filter->source);

    LeaveCriticalSection(&filter->csReceive);

    return hr;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = transform_get_pin,
    .filter_destroy = transform_destroy,
    .filter_init_stream = transform_init_stream,
    .filter_cleanup_stream = transform_cleanup_stream,
};

static HRESULT sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    TransformFilter *filter = impl_from_sink_IPin(&iface->IPin_iface);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT sink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    TransformFilter *filter = impl_from_sink_IPin(&iface->pin.IPin_iface);

    if (filter->pFuncsTable->transform_connect_sink)
        return filter->pFuncsTable->transform_connect_sink(filter, mt);
    return S_OK;
}

static void sink_disconnect(struct strmbase_sink *iface)
{
    TransformFilter *filter = impl_from_sink_IPin(&iface->pin.IPin_iface);

    if (filter->pFuncsTable->pfnBreakConnect)
        filter->pFuncsTable->pfnBreakConnect(filter, PINDIR_INPUT);
}

static HRESULT sink_eos(struct strmbase_sink *iface)
{
    TransformFilter *filter = impl_from_sink_IPin(&iface->pin.IPin_iface);

    if (filter->source.pin.peer)
        return IPin_EndOfStream(filter->source.pin.peer);
    return VFW_E_NOT_CONNECTED;
}

static HRESULT sink_begin_flush(struct strmbase_sink *iface)
{
    TransformFilter *filter = impl_from_sink_IPin(&iface->pin.IPin_iface);
    if (filter->source.pin.peer)
        return IPin_BeginFlush(filter->source.pin.peer);
    return S_OK;
}

static HRESULT sink_end_flush(struct strmbase_sink *iface)
{
    TransformFilter *filter = impl_from_sink_IPin(&iface->pin.IPin_iface);
    HRESULT hr = S_OK;

    if (filter->pFuncsTable->pfnEndFlush)
        hr = filter->pFuncsTable->pfnEndFlush(filter);
    if (SUCCEEDED(hr) && filter->source.pin.peer)
        hr = IPin_EndFlush(filter->source.pin.peer);
    return hr;
}

static HRESULT sink_new_segment(struct strmbase_sink *iface,
        REFERENCE_TIME start, REFERENCE_TIME stop, double rate)
{
    TransformFilter *filter = impl_from_sink_IPin(&iface->pin.IPin_iface);
    if (filter->source.pin.peer)
        return IPin_NewSegment(filter->source.pin.peer, start, stop, rate);
    return S_OK;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_accept = sink_query_accept,
    .base.pin_get_media_type = strmbase_pin_get_media_type,
    .base.pin_query_interface = sink_query_interface,
    .pfnReceive = TransformFilter_Input_Receive,
    .sink_connect = sink_connect,
    .sink_disconnect = sink_disconnect,
    .sink_eos = sink_eos,
    .sink_begin_flush = sink_begin_flush,
    .sink_end_flush = sink_end_flush,
    .sink_new_segment = sink_new_segment,
};

static HRESULT source_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    TransformFilter *filter = impl_from_source_IPin(&iface->IPin_iface);

    if (IsEqualGUID(iid, &IID_IQualityControl))
        *out = &filter->source_IQualityControl_iface;
    else if (IsEqualGUID(iid, &IID_IMediaSeeking))
        return IUnknown_QueryInterface(filter->seekthru_unk, iid, out);
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_accept = source_query_accept,
    .base.pin_get_media_type = source_get_media_type,
    .base.pin_query_interface = source_query_interface,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideBufferSize = TransformFilter_Output_DecideBufferSize,
    .pfnDecideAllocator = BaseOutputPinImpl_DecideAllocator,
};

static TransformFilter *impl_from_source_IQualityControl(IQualityControl *iface)
{
    return CONTAINING_RECORD(iface, TransformFilter, source_IQualityControl_iface);
}

static HRESULT WINAPI transform_source_qc_QueryInterface(IQualityControl *iface,
        REFIID iid, void **out)
{
    TransformFilter *filter = impl_from_source_IQualityControl(iface);
    return IPin_QueryInterface(&filter->source.pin.IPin_iface, iid, out);
}

static ULONG WINAPI transform_source_qc_AddRef(IQualityControl *iface)
{
    TransformFilter *filter = impl_from_source_IQualityControl(iface);
    return IPin_AddRef(&filter->source.pin.IPin_iface);
}

static ULONG WINAPI transform_source_qc_Release(IQualityControl *iface)
{
    TransformFilter *filter = impl_from_source_IQualityControl(iface);
    return IPin_Release(&filter->source.pin.IPin_iface);
}

HRESULT WINAPI TransformFilterImpl_Notify(TransformFilter *filter, IBaseFilter *sender, Quality q)
{
    IQualityControl *peer;
    HRESULT hr = S_FALSE;

    if (filter->source_qc_sink)
        return IQualityControl_Notify(filter->source_qc_sink, &filter->filter.IBaseFilter_iface, q);

    if (filter->sink.pin.peer
            && SUCCEEDED(IPin_QueryInterface(filter->sink.pin.peer, &IID_IQualityControl, (void **)&peer)))
    {
        hr = IQualityControl_Notify(peer, &filter->filter.IBaseFilter_iface, q);
        IQualityControl_Release(peer);
    }
    return hr;
}

static HRESULT WINAPI transform_source_qc_Notify(IQualityControl *iface,
        IBaseFilter *sender, Quality q)
{
    TransformFilter *filter = impl_from_source_IQualityControl(iface);

    TRACE("filter %p, sender %p, type %#x, proportion %u, late %s, timestamp %s.\n",
            filter, sender, q.Type, q.Proportion, debugstr_time(q.Late), debugstr_time(q.TimeStamp));

    if (filter->pFuncsTable->pfnNotify)
        return filter->pFuncsTable->pfnNotify(filter, sender, q);
    return TransformFilterImpl_Notify(filter, sender, q);
}

static HRESULT WINAPI transform_source_qc_SetSink(IQualityControl *iface, IQualityControl *sink)
{
    TransformFilter *filter = impl_from_source_IQualityControl(iface);

    TRACE("filter %p, sink %p.\n", filter, sink);

    filter->source_qc_sink = sink;

    return S_OK;
}

static const IQualityControlVtbl source_qc_vtbl =
{
    transform_source_qc_QueryInterface,
    transform_source_qc_AddRef,
    transform_source_qc_Release,
    transform_source_qc_Notify,
    transform_source_qc_SetSink,
};

static HRESULT strmbase_transform_init(IUnknown *outer, const CLSID *clsid,
        const TransformFilterFuncTable *func_table, TransformFilter *filter)
{
    ISeekingPassThru *passthru;
    HRESULT hr;

    strmbase_filter_init(&filter->filter, outer, clsid, &filter_ops);

    InitializeCriticalSection(&filter->csReceive);
    filter->csReceive.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__": TransformFilter.csReceive");

    /* pTransformFilter is already allocated */
    filter->pFuncsTable = func_table;
    ZeroMemory(&filter->pmt, sizeof(filter->pmt));

    strmbase_sink_init(&filter->sink, &filter->filter, wcsInputPinName, &sink_ops, NULL);

    strmbase_source_init(&filter->source, &filter->filter, wcsOutputPinName, &source_ops);
    filter->source_IQualityControl_iface.lpVtbl = &source_qc_vtbl;

    filter->seekthru_unk = NULL;
    hr = CoCreateInstance(&CLSID_SeekingPassThru, (IUnknown *)&filter->filter.IBaseFilter_iface,
            CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&filter->seekthru_unk);
    if (SUCCEEDED(hr))
    {
        IUnknown_QueryInterface(filter->seekthru_unk, &IID_ISeekingPassThru, (void **)&passthru);
        ISeekingPassThru_Init(passthru, FALSE, &filter->sink.pin.IPin_iface);
        ISeekingPassThru_Release(passthru);
    }

    if (FAILED(hr))
    {
        strmbase_sink_cleanup(&filter->sink);
        strmbase_source_cleanup(&filter->source);
        strmbase_filter_cleanup(&filter->filter);
    }

    return hr;
}

HRESULT strmbase_transform_create(LONG filter_size, IUnknown *outer, const CLSID *pClsid,
        const TransformFilterFuncTable *pFuncsTable, IBaseFilter **ppTransformFilter)
{
    TransformFilter* pTf;

    *ppTransformFilter = NULL;

    assert(filter_size >= sizeof(TransformFilter));

    pTf = CoTaskMemAlloc(filter_size);

    if (!pTf)
        return E_OUTOFMEMORY;

    ZeroMemory(pTf, filter_size);

    if (SUCCEEDED(strmbase_transform_init(outer, pClsid, pFuncsTable, pTf)))
    {
        *ppTransformFilter = &pTf->filter.IBaseFilter_iface;
        return S_OK;
    }

    CoTaskMemFree(pTf);
    return E_FAIL;
}
