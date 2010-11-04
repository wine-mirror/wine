/*
 * Null Renderer (Promiscuous, not rendering anything at all!)
 *
 * Copyright 2004 Christian Costa
 * Copyright 2008 Maarten Lankhorst
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

#include "config.h"

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "quartz_private.h"
#include "control_private.h"
#include "pin.h"

#include "uuids.h"
#include "vfwmsgs.h"
#include "amvideo.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "evcode.h"
#include "strmif.h"
#include "ddraw.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const WCHAR wcsInputPinName[] = {'i','n','p','u','t',' ','p','i','n',0};
static const WCHAR wcsAltInputPinName[] = {'I','n',0};

static const IBaseFilterVtbl NullRenderer_Vtbl;
static const IUnknownVtbl IInner_VTable;
static const IPinVtbl NullRenderer_InputPin_Vtbl;
static const IAMFilterMiscFlagsVtbl IAMFilterMiscFlags_Vtbl;

typedef struct NullRendererImpl
{
    BaseFilter filter;
    const IUnknownVtbl * IInner_vtbl;
    const IAMFilterMiscFlagsVtbl *IAMFilterMiscFlags_vtbl;
    IUnknown *seekthru_unk;

    BaseInputPin *pInputPin;
    IUnknown * pUnkOuter;
    BOOL bUnkOuterValid;
    BOOL bAggregatable;
} NullRendererImpl;

static HRESULT WINAPI NullRenderer_Receive(BaseInputPin *pin, IMediaSample * pSample)
{
    NullRendererImpl *This = ((NullRendererImpl*)pin->pin.pinInfo.pFilter);
    HRESULT hr = S_OK;
    REFERENCE_TIME start, stop;

    TRACE("%p %p\n", pin, pSample);

    if (SUCCEEDED(IMediaSample_GetTime(pSample, &start, &stop)))
        MediaSeekingPassThru_RegisterMediaTime(This->seekthru_unk, start);
    EnterCriticalSection(&This->filter.csFilter);
    if (This->pInputPin->flushing || This->pInputPin->end_of_stream)
        hr = S_FALSE;
    LeaveCriticalSection(&This->filter.csFilter);

    return hr;
}

static HRESULT WINAPI NullRenderer_CheckMediaType(BasePin *iface, const AM_MEDIA_TYPE * pmt)
{
    TRACE("Not a stub!\n");
    return S_OK;
}

static IPin* WINAPI NullRenderer_GetPin(BaseFilter *iface, int pos)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    if (pos >= 1 || pos < 0)
        return NULL;

    IPin_AddRef((IPin *)This->pInputPin);
    return (IPin *)This->pInputPin;
}

static LONG WINAPI NullRenderer_GetPinCount(BaseFilter *iface)
{
    return 1;
}

static const BaseFilterFuncTable BaseFuncTable = {
    NullRenderer_GetPin,
    NullRenderer_GetPinCount
};

static const  BasePinFuncTable input_BaseFuncTable = {
    NullRenderer_CheckMediaType,
    NULL,
    BasePinImpl_GetMediaTypeVersion,
    BasePinImpl_GetMediaType
};

static const BaseInputPinFuncTable input_BaseInputFuncTable = {
   NullRenderer_Receive
};


HRESULT NullRenderer_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    PIN_INFO piInput;
    NullRendererImpl * pNullRenderer;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    pNullRenderer = CoTaskMemAlloc(sizeof(NullRendererImpl));
    pNullRenderer->pUnkOuter = pUnkOuter;
    pNullRenderer->bUnkOuterValid = FALSE;
    pNullRenderer->bAggregatable = FALSE;
    pNullRenderer->IInner_vtbl = &IInner_VTable;
    pNullRenderer->IAMFilterMiscFlags_vtbl = &IAMFilterMiscFlags_Vtbl;

    BaseFilter_Init(&pNullRenderer->filter, &NullRenderer_Vtbl, &CLSID_NullRenderer, (DWORD_PTR)(__FILE__ ": NullRendererImpl.csFilter"), &BaseFuncTable);

    /* construct input pin */
    piInput.dir = PINDIR_INPUT;
    piInput.pFilter = (IBaseFilter *)pNullRenderer;
    lstrcpynW(piInput.achName, wcsInputPinName, sizeof(piInput.achName) / sizeof(piInput.achName[0]));

    hr = BaseInputPin_Construct(&NullRenderer_InputPin_Vtbl, &piInput, &input_BaseFuncTable, &input_BaseInputFuncTable, &pNullRenderer->filter.csFilter, NULL, (IPin **)&pNullRenderer->pInputPin);

    if (SUCCEEDED(hr))
    {
        ISeekingPassThru *passthru;
        hr = CoCreateInstance(&CLSID_SeekingPassThru, pUnkOuter ? pUnkOuter : (IUnknown*)&pNullRenderer->IInner_vtbl, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&pNullRenderer->seekthru_unk);
        if (FAILED(hr)) {
            IUnknown_Release((IUnknown*)pNullRenderer);
            return hr;
        }
        IUnknown_QueryInterface(pNullRenderer->seekthru_unk, &IID_ISeekingPassThru, (void**)&passthru);
        ISeekingPassThru_Init(passthru, TRUE, (IPin*)pNullRenderer->pInputPin);
        ISeekingPassThru_Release(passthru);
        *ppv = pNullRenderer;
    }
    else
    {
        BaseFilterImpl_Release((IBaseFilter*)pNullRenderer);
        CoTaskMemFree(pNullRenderer);
    }

    return hr;
}

static HRESULT WINAPI NullRendererInner_QueryInterface(IUnknown * iface, REFIID riid, LPVOID * ppv)
{
    ICOM_THIS_MULTI(NullRendererImpl, IInner_vtbl, iface);
    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->IInner_vtbl;
    else if (IsEqualIID(riid, &IID_IPersist))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IMediaFilter))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IBaseFilter))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
        return IUnknown_QueryInterface(This->seekthru_unk, riid, ppv);
    else if (IsEqualIID(riid, &IID_IAMFilterMiscFlags))
        *ppv = &This->IAMFilterMiscFlags_vtbl;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    if (!IsEqualIID(riid, &IID_IPin) && !IsEqualIID(riid, &IID_IVideoWindow))
        FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI NullRendererInner_AddRef(IUnknown * iface)
{
    ICOM_THIS_MULTI(NullRendererImpl, IInner_vtbl, iface);
    ULONG refCount = InterlockedIncrement(&This->filter.refCount);

    TRACE("(%p/%p)->() AddRef from %d\n", This, iface, refCount - 1);

    return refCount;
}

static ULONG WINAPI NullRendererInner_Release(IUnknown * iface)
{
    ICOM_THIS_MULTI(NullRendererImpl, IInner_vtbl, iface);
    ULONG refCount = InterlockedDecrement(&This->filter.refCount);

    TRACE("(%p/%p)->() Release from %d\n", This, iface, refCount + 1);

    if (!refCount)
    {
        IPin *pConnectedTo;

        if (SUCCEEDED(IPin_ConnectedTo((IPin *)This->pInputPin, &pConnectedTo)))
        {
            IPin_Disconnect(pConnectedTo);
            IPin_Release(pConnectedTo);
        }
        IPin_Disconnect((IPin *)This->pInputPin);
        IPin_Release((IPin *)This->pInputPin);

        This->filter.lpVtbl = NULL;
        if (This->seekthru_unk)
            IUnknown_Release(This->seekthru_unk);

        This->filter.csFilter.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->filter.csFilter);

        TRACE("Destroying Null Renderer\n");
        CoTaskMemFree(This);
        return 0;
    }
    else
        return refCount;
}

static const IUnknownVtbl IInner_VTable =
{
    NullRendererInner_QueryInterface,
    NullRendererInner_AddRef,
    NullRendererInner_Release
};

static HRESULT WINAPI NullRenderer_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    if (This->pUnkOuter)
    {
        if (This->bAggregatable)
            return IUnknown_QueryInterface(This->pUnkOuter, riid, ppv);

        if (IsEqualIID(riid, &IID_IUnknown))
        {
            HRESULT hr;

            IUnknown_AddRef((IUnknown *)&(This->IInner_vtbl));
            hr = IUnknown_QueryInterface((IUnknown *)&(This->IInner_vtbl), riid, ppv);
            IUnknown_Release((IUnknown *)&(This->IInner_vtbl));
            This->bAggregatable = TRUE;
            return hr;
        }

        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return IUnknown_QueryInterface((IUnknown *)&(This->IInner_vtbl), riid, ppv);
}

static ULONG WINAPI NullRenderer_AddRef(IBaseFilter * iface)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    if (This->pUnkOuter && This->bUnkOuterValid)
        return IUnknown_AddRef(This->pUnkOuter);
    return IUnknown_AddRef((IUnknown *)&(This->IInner_vtbl));
}

static ULONG WINAPI NullRenderer_Release(IBaseFilter * iface)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    if (This->pUnkOuter && This->bUnkOuterValid)
        return IUnknown_Release(This->pUnkOuter);
    return IUnknown_Release((IUnknown *)&(This->IInner_vtbl));
}

/** IMediaFilter methods **/

static HRESULT WINAPI NullRenderer_Stop(IBaseFilter * iface)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->filter.csFilter);
    {
        This->filter.state = State_Stopped;
        MediaSeekingPassThru_ResetMediaTime(This->seekthru_unk);
    }
    LeaveCriticalSection(&This->filter.csFilter);

    return S_OK;
}

static HRESULT WINAPI NullRenderer_Pause(IBaseFilter * iface)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->filter.csFilter);
    {
        if (This->filter.state == State_Stopped)
            This->pInputPin->end_of_stream = 0;
        This->filter.state = State_Paused;
    }
    LeaveCriticalSection(&This->filter.csFilter);

    return S_OK;
}

static HRESULT WINAPI NullRenderer_Run(IBaseFilter * iface, REFERENCE_TIME tStart)
{
    HRESULT hr = S_OK;
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%s)\n", This, iface, wine_dbgstr_longlong(tStart));

    EnterCriticalSection(&This->filter.csFilter);
    if (This->filter.state == State_Running)
        goto out;
    if (This->pInputPin->pin.pConnectedTo)
    {
        This->filter.rtStreamStart = tStart;
        This->pInputPin->end_of_stream = 0;
    }
    else if (This->filter.filterInfo.pGraph)
    {
        IMediaEventSink *pEventSink;
        hr = IFilterGraph_QueryInterface(This->filter.filterInfo.pGraph, &IID_IMediaEventSink, (LPVOID*)&pEventSink);
        if (SUCCEEDED(hr))
        {
            hr = IMediaEventSink_Notify(pEventSink, EC_COMPLETE, S_OK, (LONG_PTR)This);
            IMediaEventSink_Release(pEventSink);
        }
        hr = S_OK;
    }
    if (SUCCEEDED(hr))
        This->filter.state = State_Running;
out:
    LeaveCriticalSection(&This->filter.csFilter);

    return hr;
}

/** IBaseFilter implementation **/

static HRESULT WINAPI NullRenderer_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%p,%p)\n", This, iface, debugstr_w(Id), ppPin);

    if (!Id || !ppPin)
        return E_POINTER;

    if (!lstrcmpiW(Id,wcsInputPinName) || !lstrcmpiW(Id,wcsAltInputPinName))
    {
        *ppPin = (IPin *)This->pInputPin;
        IPin_AddRef(*ppPin);
        return S_OK;
    }
    *ppPin = NULL;
    return VFW_E_NOT_FOUND;
}

static const IBaseFilterVtbl NullRenderer_Vtbl =
{
    NullRenderer_QueryInterface,
    NullRenderer_AddRef,
    NullRenderer_Release,
    BaseFilterImpl_GetClassID,
    NullRenderer_Stop,
    NullRenderer_Pause,
    NullRenderer_Run,
    BaseFilterImpl_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    NullRenderer_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

static HRESULT WINAPI NullRenderer_InputPin_EndOfStream(IPin * iface)
{
    BaseInputPin* This = (BaseInputPin*)iface;
    IMediaEventSink* pEventSink;
    NullRendererImpl *pNull;
    IFilterGraph *graph;
    HRESULT hr = S_OK;

    TRACE("(%p/%p)->()\n", This, iface);

    BaseInputPinImpl_EndOfStream(iface);
    pNull = (NullRendererImpl*)This->pin.pinInfo.pFilter;
    graph = pNull->filter.filterInfo.pGraph;
    if (graph)
    {
        hr = IFilterGraph_QueryInterface(pNull->filter.filterInfo.pGraph, &IID_IMediaEventSink, (LPVOID*)&pEventSink);
        if (SUCCEEDED(hr))
        {
            hr = IMediaEventSink_Notify(pEventSink, EC_COMPLETE, S_OK, (LONG_PTR)pNull);
            IMediaEventSink_Release(pEventSink);
        }
    }
    MediaSeekingPassThru_EOS(pNull->seekthru_unk);

    return hr;
}

static HRESULT WINAPI NullRenderer_InputPin_EndFlush(IPin * iface)
{
    BaseInputPin* This = (BaseInputPin*)iface;
    NullRendererImpl *pNull;
    HRESULT hr = S_OK;

    TRACE("(%p/%p)->()\n", This, iface);

    hr = BaseInputPinImpl_EndOfStream(iface);
    pNull = (NullRendererImpl*)This->pin.pinInfo.pFilter;
    MediaSeekingPassThru_ResetMediaTime(pNull->seekthru_unk);
    return hr;
}

static const IPinVtbl NullRenderer_InputPin_Vtbl =
{
    BaseInputPinImpl_QueryInterface,
    BasePinImpl_AddRef,
    BaseInputPinImpl_Release,
    BaseInputPinImpl_Connect,
    BaseInputPinImpl_ReceiveConnection,
    BasePinImpl_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    BaseInputPinImpl_QueryAccept,
    BasePinImpl_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    NullRenderer_InputPin_EndOfStream,
    BaseInputPinImpl_BeginFlush,
    NullRenderer_InputPin_EndFlush,
    BaseInputPinImpl_NewSegment
};

static NullRendererImpl *from_IAMFilterMiscFlags(IAMFilterMiscFlags *iface) {
    return (NullRendererImpl*)((char*)iface - offsetof(NullRendererImpl, IAMFilterMiscFlags_vtbl));
}

static HRESULT WINAPI AMFilterMiscFlags_QueryInterface(IAMFilterMiscFlags *iface, const REFIID riid, void **ppv) {
    NullRendererImpl *This = from_IAMFilterMiscFlags(iface);
    return IUnknown_QueryInterface((IUnknown*)This, riid, ppv);
}

static ULONG WINAPI AMFilterMiscFlags_AddRef(IAMFilterMiscFlags *iface) {
    NullRendererImpl *This = from_IAMFilterMiscFlags(iface);
    return IUnknown_AddRef((IUnknown*)This);
}

static ULONG WINAPI AMFilterMiscFlags_Release(IAMFilterMiscFlags *iface) {
    NullRendererImpl *This = from_IAMFilterMiscFlags(iface);
    return IUnknown_Release((IUnknown*)This);
}

static ULONG WINAPI AMFilterMiscFlags_GetMiscFlags(IAMFilterMiscFlags *iface) {
    return AM_FILTER_MISC_FLAGS_IS_RENDERER;
}

static const IAMFilterMiscFlagsVtbl IAMFilterMiscFlags_Vtbl = {
    AMFilterMiscFlags_QueryInterface,
    AMFilterMiscFlags_AddRef,
    AMFilterMiscFlags_Release,
    AMFilterMiscFlags_GetMiscFlags
};
