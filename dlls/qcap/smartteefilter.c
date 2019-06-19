/*
 * Implementation of the SmartTee filter
 *
 * Copyright 2015 Damjan Jovanovic
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
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"

#include "qcap_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

typedef struct {
    BaseFilter filter;
    BaseInputPin sink;
    BaseOutputPin capture, preview;
} SmartTeeFilter;

static inline SmartTeeFilter *impl_from_BaseFilter(BaseFilter *filter)
{
    return CONTAINING_RECORD(filter, SmartTeeFilter, filter);
}

static inline SmartTeeFilter *impl_from_IBaseFilter(IBaseFilter *iface)
{
    BaseFilter *filter = CONTAINING_RECORD(iface, BaseFilter, IBaseFilter_iface);
    return impl_from_BaseFilter(filter);
}

static inline SmartTeeFilter *impl_from_BasePin(BasePin *pin)
{
    return impl_from_IBaseFilter(pin->pinInfo.pFilter);
}

static inline SmartTeeFilter *impl_from_IPin(IPin *iface)
{
    BasePin *bp = CONTAINING_RECORD(iface, BasePin, IPin_iface);
    return impl_from_IBaseFilter(bp->pinInfo.pFilter);
}

static HRESULT WINAPI SmartTeeFilter_Stop(IBaseFilter *iface)
{
    SmartTeeFilter *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)\n", This);
    EnterCriticalSection(&This->filter.csFilter);
    This->filter.state = State_Stopped;
    LeaveCriticalSection(&This->filter.csFilter);
    return S_OK;
}

static HRESULT WINAPI SmartTeeFilter_Pause(IBaseFilter *iface)
{
    SmartTeeFilter *This = impl_from_IBaseFilter(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI SmartTeeFilter_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    SmartTeeFilter *This = impl_from_IBaseFilter(iface);
    HRESULT hr = S_OK;
    TRACE("(%p, %s)\n", This, wine_dbgstr_longlong(tStart));
    EnterCriticalSection(&This->filter.csFilter);
    if(This->filter.state != State_Running) {
        /* We share an allocator among all pins, an allocator can only get committed
         * once, state transitions occur in upstream order, and only output pins
         * commit allocators, so let the filter attached to the input pin worry about it. */
        if (This->sink.pin.pConnectedTo)
            This->filter.state = State_Running;
        else
            hr = VFW_E_NOT_CONNECTED;
    }
    LeaveCriticalSection(&This->filter.csFilter);
    return hr;
}

static const IBaseFilterVtbl SmartTeeFilterVtbl = {
    BaseFilterImpl_QueryInterface,
    BaseFilterImpl_AddRef,
    BaseFilterImpl_Release,
    BaseFilterImpl_GetClassID,
    SmartTeeFilter_Stop,
    SmartTeeFilter_Pause,
    SmartTeeFilter_Run,
    BaseFilterImpl_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    BaseFilterImpl_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

static IPin *smart_tee_get_pin(BaseFilter *iface, unsigned int index)
{
    SmartTeeFilter *filter = impl_from_BaseFilter(iface);

    if (index == 0)
        return &filter->sink.pin.IPin_iface;
    else if (index == 1)
        return &filter->capture.pin.IPin_iface;
    else if (index == 2)
        return &filter->preview.pin.IPin_iface;
    return NULL;
}

static void smart_tee_destroy(BaseFilter *iface)
{
    SmartTeeFilter *filter = impl_from_BaseFilter(iface);

    strmbase_sink_cleanup(&filter->sink);
    strmbase_source_cleanup(&filter->capture);
    strmbase_source_cleanup(&filter->preview);
    strmbase_filter_cleanup(&filter->filter);
    CoTaskMemFree(filter);
}

static const BaseFilterFuncTable SmartTeeFilterFuncs = {
    .filter_get_pin = smart_tee_get_pin,
    .filter_destroy = smart_tee_destroy,
};

static const IPinVtbl SmartTeeFilterInputVtbl = {
    BaseInputPinImpl_QueryInterface,
    BasePinImpl_AddRef,
    BasePinImpl_Release,
    BaseInputPinImpl_Connect,
    BaseInputPinImpl_ReceiveConnection,
    BasePinImpl_Disconnect,
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

static HRESULT WINAPI SmartTeeFilterInput_CheckMediaType(BasePin *base, const AM_MEDIA_TYPE *pmt)
{
    SmartTeeFilter *This = impl_from_BasePin(base);
    TRACE("(%p, AM_MEDIA_TYPE(%p))\n", This, pmt);
    dump_AM_MEDIA_TYPE(pmt);
    if (!pmt)
        return VFW_E_TYPE_NOT_ACCEPTED;
    /* We'll take any media type, but the output pins will later
     * struggle to connect downstream. */
    return S_OK;
}

static HRESULT WINAPI SmartTeeFilterInput_GetMediaType(BasePin *base, int iPosition, AM_MEDIA_TYPE *amt)
{
    SmartTeeFilter *This = impl_from_BasePin(base);
    HRESULT hr;
    TRACE("(%p)->(%d, %p)\n", This, iPosition, amt);
    if (iPosition)
        return S_FALSE;
    EnterCriticalSection(&This->filter.csFilter);
    if (This->sink.pin.pConnectedTo)
    {
        CopyMediaType(amt, &This->sink.pin.mtCurrent);
        hr = S_OK;
    }
    else
        hr = S_FALSE;
    LeaveCriticalSection(&This->filter.csFilter);
    return hr;
}

static HRESULT copy_sample(IMediaSample *inputSample, IMemAllocator *allocator, IMediaSample **pOutputSample)
{
    REFERENCE_TIME startTime, endTime;
    BOOL haveStartTime = TRUE, haveEndTime = TRUE;
    IMediaSample *outputSample = NULL;
    BYTE *ptrIn, *ptrOut;
    AM_MEDIA_TYPE *mediaType = NULL;
    HRESULT hr;

    hr = IMediaSample_GetTime(inputSample, &startTime, &endTime);
    if (hr == S_OK)
        ;
    else if (hr == VFW_S_NO_STOP_TIME)
        haveEndTime = FALSE;
    else if (hr == VFW_E_SAMPLE_TIME_NOT_SET)
        haveStartTime = haveEndTime = FALSE;
    else
        goto end;

    hr = IMemAllocator_GetBuffer(allocator, &outputSample,
            haveStartTime ? &startTime : NULL, haveEndTime ? &endTime : NULL, 0);
    if (FAILED(hr)) goto end;
    if (IMediaSample_GetSize(outputSample) < IMediaSample_GetActualDataLength(inputSample)) {
        ERR("insufficient space in sample\n");
        hr = VFW_E_BUFFER_OVERFLOW;
        goto end;
    }

    hr = IMediaSample_SetTime(outputSample, haveStartTime ? &startTime : NULL, haveEndTime ? &endTime : NULL);
    if (FAILED(hr)) goto end;

    hr = IMediaSample_GetPointer(inputSample, &ptrIn);
    if (FAILED(hr)) goto end;
    hr = IMediaSample_GetPointer(outputSample, &ptrOut);
    if (FAILED(hr)) goto end;
    memcpy(ptrOut, ptrIn, IMediaSample_GetActualDataLength(inputSample));
    IMediaSample_SetActualDataLength(outputSample, IMediaSample_GetActualDataLength(inputSample));

    hr = IMediaSample_SetDiscontinuity(outputSample, IMediaSample_IsDiscontinuity(inputSample) == S_OK);
    if (FAILED(hr)) goto end;

    haveStartTime = haveEndTime = TRUE;
    hr = IMediaSample_GetMediaTime(inputSample, &startTime, &endTime);
    if (hr == S_OK)
        ;
    else if (hr == VFW_S_NO_STOP_TIME)
        haveEndTime = FALSE;
    else if (hr == VFW_E_MEDIA_TIME_NOT_SET)
        haveStartTime = haveEndTime = FALSE;
    else
        goto end;
    hr = IMediaSample_SetMediaTime(outputSample, haveStartTime ? &startTime : NULL, haveEndTime ? &endTime : NULL);
    if (FAILED(hr)) goto end;

    hr = IMediaSample_GetMediaType(inputSample, &mediaType);
    if (FAILED(hr)) goto end;
    if (hr == S_OK) {
        hr = IMediaSample_SetMediaType(outputSample, mediaType);
        if (FAILED(hr)) goto end;
    }

    hr = IMediaSample_SetPreroll(outputSample, IMediaSample_IsPreroll(inputSample) == S_OK);
    if (FAILED(hr)) goto end;

    hr = IMediaSample_SetSyncPoint(outputSample, IMediaSample_IsSyncPoint(inputSample) == S_OK);
    if (FAILED(hr)) goto end;

end:
    if (mediaType)
        DeleteMediaType(mediaType);
    if (FAILED(hr) && outputSample) {
        IMediaSample_Release(outputSample);
        outputSample = NULL;
    }
    *pOutputSample = outputSample;
    return hr;
}

static HRESULT WINAPI SmartTeeFilterInput_Receive(BaseInputPin *base, IMediaSample *inputSample)
{
    SmartTeeFilter *This = impl_from_BasePin(&base->pin);
    IMediaSample *captureSample = NULL;
    IMediaSample *previewSample = NULL;
    HRESULT hrCapture = VFW_E_NOT_CONNECTED, hrPreview = VFW_E_NOT_CONNECTED;

    TRACE("(%p)->(%p)\n", This, inputSample);

    /* Modifying the image coming out of one pin doesn't modify the image
     * coming out of the other. MSDN claims the filter doesn't copy,
     * but unless it somehow uses copy-on-write, I just don't see how
     * that's possible. */

    /* FIXME: we should ideally do each of these in a separate thread */
    EnterCriticalSection(&This->filter.csFilter);
    if (This->capture.pin.pConnectedTo)
        hrCapture = copy_sample(inputSample, This->capture.pAllocator, &captureSample);
    LeaveCriticalSection(&This->filter.csFilter);
    if (SUCCEEDED(hrCapture))
        hrCapture = BaseOutputPinImpl_Deliver(&This->capture, captureSample);
    if (captureSample)
        IMediaSample_Release(captureSample);

    EnterCriticalSection(&This->filter.csFilter);
    if (This->preview.pin.pConnectedTo)
        hrPreview = copy_sample(inputSample, This->preview.pAllocator, &previewSample);
    LeaveCriticalSection(&This->filter.csFilter);
    /* No timestamps on preview stream: */
    if (SUCCEEDED(hrPreview))
        hrPreview = IMediaSample_SetTime(previewSample, NULL, NULL);
    if (SUCCEEDED(hrPreview))
        hrPreview = BaseOutputPinImpl_Deliver(&This->preview, previewSample);
    if (previewSample)
        IMediaSample_Release(previewSample);

    /* FIXME: how to merge the HRESULTs from the 2 pins? */
    if (SUCCEEDED(hrCapture))
        return hrCapture;
    else
        return hrPreview;
}

static const BaseInputPinFuncTable SmartTeeFilterInputFuncs = {
    {
        SmartTeeFilterInput_CheckMediaType,
        SmartTeeFilterInput_GetMediaType
    },
    SmartTeeFilterInput_Receive
};

static HRESULT WINAPI SmartTeeFilterCapture_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    SmartTeeFilter *This = impl_from_IPin(iface);
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, ppEnum);
    EnterCriticalSection(&This->filter.csFilter);
    if (This->sink.pin.pConnectedTo)
        hr = BasePinImpl_EnumMediaTypes(iface, ppEnum);
    else
        hr = VFW_E_NOT_CONNECTED;
    LeaveCriticalSection(&This->filter.csFilter);
    return hr;
}

static const IPinVtbl SmartTeeFilterCaptureVtbl = {
    BaseOutputPinImpl_QueryInterface,
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
    SmartTeeFilterCapture_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    BaseOutputPinImpl_EndOfStream,
    BaseOutputPinImpl_BeginFlush,
    BaseOutputPinImpl_EndFlush,
    BasePinImpl_NewSegment
};

static HRESULT WINAPI SmartTeeFilterCapture_CheckMediaType(BasePin *base, const AM_MEDIA_TYPE *amt)
{
    FIXME("(%p) stub\n", base);
    return S_OK;
}

static HRESULT WINAPI SmartTeeFilterCapture_GetMediaType(BasePin *base, int iPosition, AM_MEDIA_TYPE *amt)
{
    SmartTeeFilter *This = impl_from_BasePin(base);
    TRACE("(%p, %d, %p)\n", This, iPosition, amt);
    if (iPosition == 0) {
        CopyMediaType(amt, &This->sink.pin.mtCurrent);
        return S_OK;
    } else
        return S_FALSE;
}

static HRESULT WINAPI SmartTeeFilterCapture_DecideAllocator(BaseOutputPin *base, IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    SmartTeeFilter *This = impl_from_BasePin(&base->pin);
    TRACE("(%p, %p, %p)\n", This, pPin, pAlloc);
    *pAlloc = This->sink.pAllocator;
    IMemAllocator_AddRef(This->sink.pAllocator);
    return IMemInputPin_NotifyAllocator(pPin, This->sink.pAllocator, TRUE);
}

static const BaseOutputPinFuncTable SmartTeeFilterCaptureFuncs = {
    {
        SmartTeeFilterCapture_CheckMediaType,
        SmartTeeFilterCapture_GetMediaType
    },
    BaseOutputPinImpl_AttemptConnection,
    NULL,
    SmartTeeFilterCapture_DecideAllocator,
};

static HRESULT WINAPI SmartTeeFilterPreview_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    SmartTeeFilter *This = impl_from_IPin(iface);
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, ppEnum);
    EnterCriticalSection(&This->filter.csFilter);
    if (This->sink.pin.pConnectedTo)
        hr = BasePinImpl_EnumMediaTypes(iface, ppEnum);
    else
        hr = VFW_E_NOT_CONNECTED;
    LeaveCriticalSection(&This->filter.csFilter);
    return hr;
}

static const IPinVtbl SmartTeeFilterPreviewVtbl = {
    BaseOutputPinImpl_QueryInterface,
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
    SmartTeeFilterPreview_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    BaseOutputPinImpl_EndOfStream,
    BaseOutputPinImpl_BeginFlush,
    BaseOutputPinImpl_EndFlush,
    BasePinImpl_NewSegment
};

static HRESULT WINAPI SmartTeeFilterPreview_CheckMediaType(BasePin *base, const AM_MEDIA_TYPE *amt)
{
    FIXME("(%p) stub\n", base);
    return S_OK;
}

static HRESULT WINAPI SmartTeeFilterPreview_GetMediaType(BasePin *base, int iPosition, AM_MEDIA_TYPE *amt)
{
    SmartTeeFilter *This = impl_from_BasePin(base);
    TRACE("(%p, %d, %p)\n", This, iPosition, amt);
    if (iPosition == 0) {
        CopyMediaType(amt, &This->sink.pin.mtCurrent);
        return S_OK;
    } else
        return S_FALSE;
}

static HRESULT WINAPI SmartTeeFilterPreview_DecideAllocator(BaseOutputPin *base, IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    SmartTeeFilter *This = impl_from_BasePin(&base->pin);
    TRACE("(%p, %p, %p)\n", This, pPin, pAlloc);
    *pAlloc = This->sink.pAllocator;
    IMemAllocator_AddRef(This->sink.pAllocator);
    return IMemInputPin_NotifyAllocator(pPin, This->sink.pAllocator, TRUE);
}

static const BaseOutputPinFuncTable SmartTeeFilterPreviewFuncs = {
    {
        SmartTeeFilterPreview_CheckMediaType,
        SmartTeeFilterPreview_GetMediaType
    },
    BaseOutputPinImpl_AttemptConnection,
    NULL,
    SmartTeeFilterPreview_DecideAllocator,
};
IUnknown* WINAPI QCAP_createSmartTeeFilter(IUnknown *outer, HRESULT *phr)
{
    PIN_INFO inputPinInfo  = {NULL, PINDIR_INPUT,  {'I','n','p','u','t',0}};
    PIN_INFO capturePinInfo = {NULL, PINDIR_OUTPUT, {'C','a','p','t','u','r','e',0}};
    PIN_INFO previewPinInfo = {NULL, PINDIR_OUTPUT, {'P','r','e','v','i','e','w',0}};
    HRESULT hr;
    SmartTeeFilter *This = NULL;

    This = CoTaskMemAlloc(sizeof(*This));
    if (This == NULL) {
        *phr = E_OUTOFMEMORY;
        return NULL;
    }
    memset(This, 0, sizeof(*This));

    strmbase_filter_init(&This->filter, &SmartTeeFilterVtbl, outer, &CLSID_SmartTee,
            (DWORD_PTR)(__FILE__ ": SmartTeeFilter.csFilter"), &SmartTeeFilterFuncs);

    inputPinInfo.pFilter = &This->filter.IBaseFilter_iface;
    strmbase_sink_init(&This->sink, &SmartTeeFilterInputVtbl, &inputPinInfo,
            &SmartTeeFilterInputFuncs, &This->filter.csFilter, NULL);
    hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void**)&This->sink.pAllocator);
    if (FAILED(hr))
    {
        *phr = hr;
        strmbase_filter_cleanup(&This->filter);
        return NULL;
    }

    capturePinInfo.pFilter = &This->filter.IBaseFilter_iface;
    strmbase_source_init(&This->capture, &SmartTeeFilterCaptureVtbl, &capturePinInfo,
            &SmartTeeFilterCaptureFuncs, &This->filter.csFilter);

    previewPinInfo.pFilter = &This->filter.IBaseFilter_iface;
    strmbase_source_init(&This->preview, &SmartTeeFilterPreviewVtbl, &previewPinInfo,
            &SmartTeeFilterPreviewFuncs, &This->filter.csFilter);

    *phr = S_OK;
    return &This->filter.IUnknown_inner;
}
