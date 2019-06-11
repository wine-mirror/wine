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

static const IPinVtbl TransformFilter_InputPin_Vtbl;
static const IPinVtbl TransformFilter_OutputPin_Vtbl;
static const IQualityControlVtbl TransformFilter_QualityControl_Vtbl;

static inline BaseInputPin *impl_BaseInputPin_from_BasePin( BasePin *iface )
{
    return CONTAINING_RECORD(iface, BaseInputPin, pin);
}

static inline BaseInputPin *impl_BaseInputPin_from_IPin( IPin *iface )
{
    return CONTAINING_RECORD(iface, BaseInputPin, pin.IPin_iface);
}

static inline BaseOutputPin *impl_BaseOutputPin_from_IPin( IPin *iface )
{
    return CONTAINING_RECORD(iface, BaseOutputPin, pin.IPin_iface);
}

static inline TransformFilter *impl_from_IBaseFilter( IBaseFilter *iface )
{
    return CONTAINING_RECORD(iface, TransformFilter, filter.IBaseFilter_iface);
}

static inline TransformFilter *impl_from_BaseFilter( BaseFilter *iface )
{
    return CONTAINING_RECORD(iface, TransformFilter, filter);
}

static HRESULT WINAPI TransformFilter_Input_CheckMediaType(BasePin *iface, const AM_MEDIA_TYPE * pmt)
{
    BaseInputPin* This = impl_BaseInputPin_from_BasePin(iface);
    TransformFilter * pTransform;

    TRACE("%p\n", iface);
    pTransform = impl_from_IBaseFilter(This->pin.pinInfo.pFilter);

    if (pTransform->pFuncsTable->pfnCheckInputType)
        return pTransform->pFuncsTable->pfnCheckInputType(pTransform, pmt);
    /* Assume OK if there's no query method (the connection will fail if
       needed) */
    return S_OK;
}

static HRESULT WINAPI TransformFilter_Input_Receive(BaseInputPin *This, IMediaSample *pInSample)
{
    HRESULT hr;
    TransformFilter * pTransform;
    TRACE("%p\n", This);
    pTransform = impl_from_IBaseFilter(This->pin.pinInfo.pFilter);

    EnterCriticalSection(&pTransform->csReceive);
    if (pTransform->filter.state == State_Stopped)
    {
        LeaveCriticalSection(&pTransform->csReceive);
        return VFW_E_WRONG_STATE;
    }

    if (This->end_of_stream || This->flushing)
    {
        LeaveCriticalSection(&pTransform->csReceive);
        return S_FALSE;
    }

    LeaveCriticalSection(&pTransform->csReceive);
    if (pTransform->pFuncsTable->pfnReceive)
        hr = pTransform->pFuncsTable->pfnReceive(pTransform, pInSample);
    else
        hr = S_FALSE;

    return hr;
}

static HRESULT WINAPI TransformFilter_Output_CheckMediaType(BasePin *This, const AM_MEDIA_TYPE *pmt)
{
    TransformFilter *pTransformFilter = impl_from_IBaseFilter(This->pinInfo.pFilter);
    AM_MEDIA_TYPE* outpmt = &pTransformFilter->pmt;

    if (IsEqualIID(&pmt->majortype, &outpmt->majortype)
        && (IsEqualIID(&pmt->subtype, &outpmt->subtype) || IsEqualIID(&outpmt->subtype, &GUID_NULL)))
        return S_OK;
    return S_FALSE;
}

static HRESULT WINAPI TransformFilter_Output_DecideBufferSize(BaseOutputPin *This, IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    TransformFilter *pTransformFilter = impl_from_IBaseFilter(This->pin.pinInfo.pFilter);
    return pTransformFilter->pFuncsTable->pfnDecideBufferSize(pTransformFilter, pAlloc, ppropInputRequest);
}

static HRESULT WINAPI TransformFilter_Output_GetMediaType(BasePin *This, int iPosition, AM_MEDIA_TYPE *pmt)
{
    TransformFilter *pTransform = impl_from_IBaseFilter(This->pinInfo.pFilter);

    if (iPosition < 0)
        return E_INVALIDARG;
    if (iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;
    CopyMediaType(pmt, &pTransform->pmt);
    return S_OK;
}

static IPin *transform_get_pin(BaseFilter *iface, unsigned int index)
{
    TransformFilter *filter = impl_from_BaseFilter(iface);

    if (index >= 2)
        return NULL;

    return filter->ppPins[index];
}

static void transform_destroy(BaseFilter *iface)
{
    TransformFilter *filter = impl_from_BaseFilter(iface);
    ULONG i;

    for (i = 0; i < 2; ++i)
    {
        IPin *peer;

        if (SUCCEEDED(IPin_ConnectedTo(filter->ppPins[i], &peer)))
        {
            IPin_Disconnect(peer);
            IPin_Release(peer);
        }
        IPin_Disconnect(filter->ppPins[i]);
        IPin_Release(filter->ppPins[i]);
    }

    CoTaskMemFree(filter->ppPins);

    filter->csReceive.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&filter->csReceive);
    FreeMediaType(&filter->pmt);
    QualityControlImpl_Destroy(filter->qcimpl);
    IUnknown_Release(filter->seekthru_unk);
    strmbase_filter_cleanup(&filter->filter);
    CoTaskMemFree(filter);
}

static const BaseFilterFuncTable tfBaseFuncTable = {
    .filter_get_pin = transform_get_pin,
    .filter_destroy = transform_destroy,
};

static const BaseInputPinFuncTable tf_input_BaseInputFuncTable = {
    {
        TransformFilter_Input_CheckMediaType,
        BasePinImpl_GetMediaType
    },
    TransformFilter_Input_Receive
};

static const BaseOutputPinFuncTable tf_output_BaseOutputFuncTable = {
    {
        TransformFilter_Output_CheckMediaType,
        TransformFilter_Output_GetMediaType
    },
    BaseOutputPinImpl_AttemptConnection,
    TransformFilter_Output_DecideBufferSize,
    BaseOutputPinImpl_DecideAllocator,
};

static HRESULT WINAPI TransformFilterImpl_Stop(IBaseFilter *iface)
{
    TransformFilter *This = impl_from_IBaseFilter(iface);
    HRESULT hr = S_OK;

    TRACE("(%p/%p)\n", This, iface);

    EnterCriticalSection(&This->csReceive);
    {
        This->filter.state = State_Stopped;
        if (This->pFuncsTable->pfnStopStreaming)
            hr = This->pFuncsTable->pfnStopStreaming(This);
        if (SUCCEEDED(hr))
            hr = BaseOutputPinImpl_Inactive(impl_BaseOutputPin_from_IPin(This->ppPins[1]));
    }
    LeaveCriticalSection(&This->csReceive);

    return hr;
}

static HRESULT WINAPI TransformFilterImpl_Pause(IBaseFilter *iface)
{
    TransformFilter *This = impl_from_IBaseFilter(iface);
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->csReceive);
    {
        if (This->filter.state == State_Stopped)
            hr = IBaseFilter_Run(iface, -1);
        else
            hr = S_OK;

        if (SUCCEEDED(hr))
            This->filter.state = State_Paused;
    }
    LeaveCriticalSection(&This->csReceive);

    return hr;
}

static HRESULT WINAPI TransformFilterImpl_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    HRESULT hr = S_OK;
    TransformFilter *This = impl_from_IBaseFilter(iface);

    TRACE("(%p/%p)->(%s)\n", This, iface, wine_dbgstr_longlong(tStart));

    EnterCriticalSection(&This->csReceive);
    {
        if (This->filter.state == State_Stopped)
        {
            impl_BaseInputPin_from_IPin(This->ppPins[0])->end_of_stream = FALSE;
            if (This->pFuncsTable->pfnStartStreaming)
                hr = This->pFuncsTable->pfnStartStreaming(This);
            if (SUCCEEDED(hr))
                hr = BaseOutputPinImpl_Active(impl_BaseOutputPin_from_IPin(This->ppPins[1]));
        }

        if (SUCCEEDED(hr))
        {
            This->filter.rtStreamStart = tStart;
            This->filter.state = State_Running;
        }
    }
    LeaveCriticalSection(&This->csReceive);

    return hr;
}

static const IBaseFilterVtbl transform_vtbl =
{
    BaseFilterImpl_QueryInterface,
    BaseFilterImpl_AddRef,
    BaseFilterImpl_Release,
    BaseFilterImpl_GetClassID,
    TransformFilterImpl_Stop,
    TransformFilterImpl_Pause,
    TransformFilterImpl_Run,
    BaseFilterImpl_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    BaseFilterImpl_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

static HRESULT strmbase_transform_init(IUnknown *outer, const CLSID *clsid,
        const TransformFilterFuncTable *func_table, TransformFilter *filter)
{
    HRESULT hr;
    PIN_INFO piInput;
    PIN_INFO piOutput;

    strmbase_filter_init(&filter->filter, &transform_vtbl, outer, clsid,
            (DWORD_PTR)(__FILE__ ": TransformFilter.csFilter"), &tfBaseFuncTable);

    InitializeCriticalSection(&filter->csReceive);
    filter->csReceive.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__": TransformFilter.csReceive");

    /* pTransformFilter is already allocated */
    filter->pFuncsTable = func_table;
    ZeroMemory(&filter->pmt, sizeof(filter->pmt));

    filter->ppPins = CoTaskMemAlloc(2 * sizeof(IPin *));

    /* construct input pin */
    piInput.dir = PINDIR_INPUT;
    piInput.pFilter = &filter->filter.IBaseFilter_iface;
    lstrcpynW(piInput.achName, wcsInputPinName, ARRAY_SIZE(piInput.achName));
    piOutput.dir = PINDIR_OUTPUT;
    piOutput.pFilter = &filter->filter.IBaseFilter_iface;
    lstrcpynW(piOutput.achName, wcsOutputPinName, ARRAY_SIZE(piOutput.achName));

    hr = BaseInputPin_Construct(&TransformFilter_InputPin_Vtbl, sizeof(BaseInputPin), &piInput,
            &tf_input_BaseInputFuncTable, &filter->filter.csFilter, NULL, &filter->ppPins[0]);

    if (SUCCEEDED(hr))
    {
        hr = BaseOutputPin_Construct(&TransformFilter_OutputPin_Vtbl, sizeof(BaseOutputPin),
                &piOutput, &tf_output_BaseOutputFuncTable, &filter->filter.csFilter,
                &filter->ppPins[1]);

        if (FAILED(hr))
            ERR("Cannot create output pin (%x)\n", hr);
        else {
            QualityControlImpl_Create(filter->ppPins[0],
                    &filter->filter.IBaseFilter_iface, &filter->qcimpl);
            filter->qcimpl->IQualityControl_iface.lpVtbl = &TransformFilter_QualityControl_Vtbl;
        }
    }

    if (SUCCEEDED(hr))
    {
        ISeekingPassThru *passthru;
        filter->seekthru_unk = NULL;
        hr = CoCreateInstance(&CLSID_SeekingPassThru, (IUnknown *)&filter->filter.IBaseFilter_iface,
                CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&filter->seekthru_unk);
        if (SUCCEEDED(hr))
        {
            IUnknown_QueryInterface(filter->seekthru_unk, &IID_ISeekingPassThru, (void **)&passthru);
            ISeekingPassThru_Init(passthru, FALSE, filter->ppPins[0]);
            ISeekingPassThru_Release(passthru);
        }
    }

    if (FAILED(hr))
    {
        CoTaskMemFree(filter->ppPins);
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

HRESULT WINAPI TransformFilterImpl_Notify(TransformFilter *iface, IBaseFilter *sender, Quality qm)
{
    return QualityControlImpl_Notify((IQualityControl*)iface->qcimpl, sender, qm);
}

/** IBaseFilter implementation **/

static HRESULT WINAPI TransformFilter_InputPin_EndOfStream(IPin * iface)
{
    BaseInputPin* This = impl_BaseInputPin_from_IPin(iface);
    TransformFilter* pTransform;
    IPin* ppin;
    HRESULT hr;

    TRACE("(%p)->()\n", iface);

    /* Since we process samples synchronously, just forward notification downstream */
    pTransform = impl_from_IBaseFilter(This->pin.pinInfo.pFilter);
    if (!pTransform)
        hr = E_FAIL;
    else
        hr = IPin_ConnectedTo(pTransform->ppPins[1], &ppin);
    if (SUCCEEDED(hr))
    {
        hr = IPin_EndOfStream(ppin);
        IPin_Release(ppin);
    }

    if (FAILED(hr))
        ERR("%x\n", hr);
    return hr;
}

static HRESULT WINAPI TransformFilter_InputPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    BaseInputPin* This = impl_BaseInputPin_from_IPin(iface);
    TransformFilter* pTransform;
    HRESULT hr = S_OK;

    TRACE("(%p)->(%p, %p)\n", iface, pReceivePin, pmt);

    pTransform = impl_from_IBaseFilter(This->pin.pinInfo.pFilter);

    if (pTransform->pFuncsTable->pfnSetMediaType)
        hr = pTransform->pFuncsTable->pfnSetMediaType(pTransform, PINDIR_INPUT, pmt);

    if (SUCCEEDED(hr) && pTransform->pFuncsTable->pfnCompleteConnect)
        hr = pTransform->pFuncsTable->pfnCompleteConnect(pTransform, PINDIR_INPUT, pReceivePin);

    if (SUCCEEDED(hr))
    {
        hr = BaseInputPinImpl_ReceiveConnection(iface, pReceivePin, pmt);
        if (FAILED(hr) && pTransform->pFuncsTable->pfnBreakConnect)
            pTransform->pFuncsTable->pfnBreakConnect(pTransform, PINDIR_INPUT);
    }

    return hr;
}

static HRESULT WINAPI TransformFilter_InputPin_Disconnect(IPin * iface)
{
    BaseInputPin* This = impl_BaseInputPin_from_IPin(iface);
    TransformFilter* pTransform;

    TRACE("(%p)->()\n", iface);

    pTransform = impl_from_IBaseFilter(This->pin.pinInfo.pFilter);
    if (pTransform->pFuncsTable->pfnBreakConnect)
        pTransform->pFuncsTable->pfnBreakConnect(pTransform, PINDIR_INPUT);

    return BasePinImpl_Disconnect(iface);
}

static HRESULT WINAPI TransformFilter_InputPin_BeginFlush(IPin * iface)
{
    BaseInputPin* This = impl_BaseInputPin_from_IPin(iface);
    TransformFilter* pTransform;
    HRESULT hr = S_OK;

    TRACE("(%p)->()\n", iface);

    pTransform = impl_from_IBaseFilter(This->pin.pinInfo.pFilter);
    EnterCriticalSection(&pTransform->filter.csFilter);
    if (pTransform->pFuncsTable->pfnBeginFlush)
        hr = pTransform->pFuncsTable->pfnBeginFlush(pTransform);
    if (SUCCEEDED(hr))
        hr = BaseInputPinImpl_BeginFlush(iface);
    LeaveCriticalSection(&pTransform->filter.csFilter);
    return hr;
}

static HRESULT WINAPI TransformFilter_InputPin_EndFlush(IPin * iface)
{
    BaseInputPin* This = impl_BaseInputPin_from_IPin(iface);
    TransformFilter* pTransform;
    HRESULT hr = S_OK;

    TRACE("(%p)->()\n", iface);

    pTransform = impl_from_IBaseFilter(This->pin.pinInfo.pFilter);
    EnterCriticalSection(&pTransform->filter.csFilter);
    if (pTransform->pFuncsTable->pfnEndFlush)
        hr = pTransform->pFuncsTable->pfnEndFlush(pTransform);
    if (SUCCEEDED(hr))
        hr = BaseInputPinImpl_EndFlush(iface);
    LeaveCriticalSection(&pTransform->filter.csFilter);
    return hr;
}

static HRESULT WINAPI TransformFilter_InputPin_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    BaseInputPin* This = impl_BaseInputPin_from_IPin(iface);
    TransformFilter* pTransform;
    HRESULT hr = S_OK;

    TRACE("(%p)->(%s %s %e)\n", iface, wine_dbgstr_longlong(tStart), wine_dbgstr_longlong(tStop), dRate);

    pTransform = impl_from_IBaseFilter(This->pin.pinInfo.pFilter);
    EnterCriticalSection(&pTransform->filter.csFilter);
    if (pTransform->pFuncsTable->pfnNewSegment)
        hr = pTransform->pFuncsTable->pfnNewSegment(pTransform, tStart, tStop, dRate);
    if (SUCCEEDED(hr))
        hr = BaseInputPinImpl_NewSegment(iface, tStart, tStop, dRate);
    LeaveCriticalSection(&pTransform->filter.csFilter);
    return hr;
}

static const IPinVtbl TransformFilter_InputPin_Vtbl =
{
    BaseInputPinImpl_QueryInterface,
    BasePinImpl_AddRef,
    BaseInputPinImpl_Release,
    BaseInputPinImpl_Connect,
    TransformFilter_InputPin_ReceiveConnection,
    TransformFilter_InputPin_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    BasePinImpl_QueryAccept,
    BasePinImpl_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    TransformFilter_InputPin_EndOfStream,
    TransformFilter_InputPin_BeginFlush,
    TransformFilter_InputPin_EndFlush,
    TransformFilter_InputPin_NewSegment
};

static HRESULT WINAPI transform_source_QueryInterface(IPin *iface, REFIID iid, void **out)
{
    TransformFilter *filter = impl_from_IBaseFilter(impl_BaseOutputPin_from_IPin(iface)->pin.pinInfo.pFilter);
    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IPin))
        *out = iface;
    else if (IsEqualGUID(iid, &IID_IQualityControl))
        *out = &filter->qcimpl->IQualityControl_iface;
    else if (IsEqualGUID(iid, &IID_IMediaSeeking))
        return IUnknown_QueryInterface(filter->seekthru_unk, iid, out);
    else
    {
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static const IPinVtbl TransformFilter_OutputPin_Vtbl =
{
    transform_source_QueryInterface,
    BasePinImpl_AddRef,
    BaseOutputPinImpl_Release,
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

static HRESULT WINAPI TransformFilter_QualityControlImpl_Notify(IQualityControl *iface, IBaseFilter *sender, Quality qm) {
    QualityControlImpl *qc = (QualityControlImpl*)iface;
    TransformFilter *This = impl_from_IBaseFilter(qc->self);

    if (This->pFuncsTable->pfnNotify)
        return This->pFuncsTable->pfnNotify(This, sender, qm);
    else
        return TransformFilterImpl_Notify(This, sender, qm);
}

static const IQualityControlVtbl TransformFilter_QualityControl_Vtbl = {
    QualityControlImpl_QueryInterface,
    QualityControlImpl_AddRef,
    QualityControlImpl_Release,
    TransformFilter_QualityControlImpl_Notify,
    QualityControlImpl_SetSink
};
