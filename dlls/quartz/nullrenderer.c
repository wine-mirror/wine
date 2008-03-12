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

static const IBaseFilterVtbl NullRenderer_Vtbl;
static const IUnknownVtbl IInner_VTable;
static const IPinVtbl NullRenderer_InputPin_Vtbl;

typedef struct NullRendererImpl
{
    const IBaseFilterVtbl * lpVtbl;
    const IUnknownVtbl * IInner_vtbl;

    LONG refCount;
    CRITICAL_SECTION csFilter;
    FILTER_STATE state;
    REFERENCE_TIME rtStreamStart;
    IReferenceClock * pClock;
    FILTER_INFO filterInfo;

    InputPin * pInputPin;
    IPin ** ppPins;
    IUnknown * pUnkOuter;
    BOOL bUnkOuterValid;
    BOOL bAggregatable;
} NullRendererImpl;

static const IMemInputPinVtbl MemInputPin_Vtbl =
{
    MemInputPin_QueryInterface,
    MemInputPin_AddRef,
    MemInputPin_Release,
    MemInputPin_GetAllocator,
    MemInputPin_NotifyAllocator,
    MemInputPin_GetAllocatorRequirements,
    MemInputPin_Receive,
    MemInputPin_ReceiveMultiple,
    MemInputPin_ReceiveCanBlock
};

static HRESULT NullRenderer_InputPin_Construct(const PIN_INFO * pPinInfo, SAMPLEPROC pSampleProc, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, IPin ** ppPin)
{
    InputPin * pPinImpl;

    *ppPin = NULL;

    if (pPinInfo->dir != PINDIR_INPUT)
    {
        ERR("Pin direction(%x) != PINDIR_INPUT\n", pPinInfo->dir);
        return E_INVALIDARG;
    }

    pPinImpl = CoTaskMemAlloc(sizeof(*pPinImpl));

    if (!pPinImpl)
        return E_OUTOFMEMORY;

    if (SUCCEEDED(InputPin_Init(pPinInfo, pSampleProc, pUserData, pQueryAccept, pCritSec, pPinImpl)))
    {
        pPinImpl->pin.lpVtbl = &NullRenderer_InputPin_Vtbl;
        pPinImpl->lpVtblMemInput = &MemInputPin_Vtbl;

        *ppPin = (IPin *)(&pPinImpl->pin.lpVtbl);
        return S_OK;
    }

    CoTaskMemFree(pPinImpl);
    return E_FAIL;
}



static HRESULT NullRenderer_Sample(LPVOID iface, IMediaSample * pSample)
{
    LPBYTE pbSrcStream = NULL;
    long cbSrcStream = 0;
    REFERENCE_TIME tStart, tStop;
    HRESULT hr;

    TRACE("%p %p\n", iface, pSample);

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%x)\n", hr);
        return hr;
    }

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);
    if (FAILED(hr))
        ERR("Cannot get sample time (%x)\n", hr);

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    TRACE("val %p %ld\n", pbSrcStream, cbSrcStream);

    return S_OK;
}

static HRESULT NullRenderer_QueryAccept(LPVOID iface, const AM_MEDIA_TYPE * pmt)
{
    TRACE("Not a stub!\n");
    return S_OK;
}

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

    pNullRenderer->lpVtbl = &NullRenderer_Vtbl;
    pNullRenderer->refCount = 1;
    InitializeCriticalSection(&pNullRenderer->csFilter);
    pNullRenderer->csFilter.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": NullRendererImpl.csFilter");
    pNullRenderer->state = State_Stopped;
    pNullRenderer->pClock = NULL;
    ZeroMemory(&pNullRenderer->filterInfo, sizeof(FILTER_INFO));

    pNullRenderer->ppPins = CoTaskMemAlloc(1 * sizeof(IPin *));

    /* construct input pin */
    piInput.dir = PINDIR_INPUT;
    piInput.pFilter = (IBaseFilter *)pNullRenderer;
    lstrcpynW(piInput.achName, wcsInputPinName, sizeof(piInput.achName) / sizeof(piInput.achName[0]));

    hr = NullRenderer_InputPin_Construct(&piInput, NullRenderer_Sample, (LPVOID)pNullRenderer, NullRenderer_QueryAccept, &pNullRenderer->csFilter, (IPin **)&pNullRenderer->pInputPin);

    if (SUCCEEDED(hr))
    {
        pNullRenderer->ppPins[0] = (IPin *)pNullRenderer->pInputPin;
        *ppv = (LPVOID)pNullRenderer;
    }
    else
    {
        CoTaskMemFree(pNullRenderer->ppPins);
        pNullRenderer->csFilter.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&pNullRenderer->csFilter);
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
        *ppv = (LPVOID)&(This->IInner_vtbl);
    else if (IsEqualIID(riid, &IID_IPersist))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IMediaFilter))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IBaseFilter))
        *ppv = (LPVOID)This;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI NullRendererInner_AddRef(IUnknown * iface)
{
    ICOM_THIS_MULTI(NullRendererImpl, IInner_vtbl, iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p/%p)->() AddRef from %d\n", This, iface, refCount - 1);

    return refCount;
}

static ULONG WINAPI NullRendererInner_Release(IUnknown * iface)
{
    ICOM_THIS_MULTI(NullRendererImpl, IInner_vtbl, iface);
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("(%p/%p)->() Release from %d\n", This, iface, refCount + 1);

    if (!refCount)
    {
        IPin *pConnectedTo;

        if (This->pClock)
            IReferenceClock_Release(This->pClock);

        if (SUCCEEDED(IPin_ConnectedTo(This->ppPins[0], &pConnectedTo)))
        {
            IPin_Disconnect(pConnectedTo);
            IPin_Release(pConnectedTo);
        }
        IPin_Disconnect(This->ppPins[0]);
        IPin_Release(This->ppPins[0]);

        CoTaskMemFree(This->ppPins);
        This->lpVtbl = NULL;

        This->csFilter.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->csFilter);

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

/** IPersist methods **/

static HRESULT WINAPI NullRenderer_GetClassID(IBaseFilter * iface, CLSID * pClsid)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pClsid);

    *pClsid = CLSID_NullRenderer;

    return S_OK;
}

/** IMediaFilter methods **/

static HRESULT WINAPI NullRenderer_Stop(IBaseFilter * iface)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->csFilter);
    {
        This->state = State_Stopped;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI NullRenderer_Pause(IBaseFilter * iface)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->csFilter);
    {
        This->state = State_Paused;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI NullRenderer_Run(IBaseFilter * iface, REFERENCE_TIME tStart)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%s)\n", This, iface, wine_dbgstr_longlong(tStart));

    EnterCriticalSection(&This->csFilter);
    {
        This->rtStreamStart = tStart;
        This->state = State_Running;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI NullRenderer_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%d, %p)\n", This, iface, dwMilliSecsTimeout, pState);

    EnterCriticalSection(&This->csFilter);
    {
        *pState = This->state;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI NullRenderer_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pClock);

    EnterCriticalSection(&This->csFilter);
    {
        if (This->pClock)
            IReferenceClock_Release(This->pClock);
        This->pClock = pClock;
        if (This->pClock)
            IReferenceClock_AddRef(This->pClock);
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI NullRenderer_GetSyncSource(IBaseFilter * iface, IReferenceClock **ppClock)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppClock);

    EnterCriticalSection(&This->csFilter);
    {
        *ppClock = This->pClock;
        IReferenceClock_AddRef(This->pClock);
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

/** IBaseFilter implementation **/

static HRESULT WINAPI NullRenderer_EnumPins(IBaseFilter * iface, IEnumPins **ppEnum)
{
    ENUMPINDETAILS epd;
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    epd.cPins = 1; /* input pin */
    epd.ppPins = This->ppPins;
    return IEnumPinsImpl_Construct(&epd, ppEnum);
}

static HRESULT WINAPI NullRenderer_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%p,%p)\n", This, iface, debugstr_w(Id), ppPin);

    FIXME("NullRenderer::FindPin(...)\n");

    /* FIXME: critical section */

    return E_NOTIMPL;
}

static HRESULT WINAPI NullRenderer_QueryFilterInfo(IBaseFilter * iface, FILTER_INFO *pInfo)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pInfo);

    strcpyW(pInfo->achName, This->filterInfo.achName);
    pInfo->pGraph = This->filterInfo.pGraph;

    if (pInfo->pGraph)
        IFilterGraph_AddRef(pInfo->pGraph);

    return S_OK;
}

static HRESULT WINAPI NullRenderer_JoinFilterGraph(IBaseFilter * iface, IFilterGraph *pGraph, LPCWSTR pName)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%p, %s)\n", This, iface, pGraph, debugstr_w(pName));

    EnterCriticalSection(&This->csFilter);
    {
        if (pName)
            strcpyW(This->filterInfo.achName, pName);
        else
            *This->filterInfo.achName = '\0';
        This->filterInfo.pGraph = pGraph; /* NOTE: do NOT increase ref. count */
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI NullRenderer_QueryVendorInfo(IBaseFilter * iface, LPWSTR *pVendorInfo)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;
    TRACE("(%p/%p)->(%p)\n", This, iface, pVendorInfo);
    return E_NOTIMPL;
}

static const IBaseFilterVtbl NullRenderer_Vtbl =
{
    NullRenderer_QueryInterface,
    NullRenderer_AddRef,
    NullRenderer_Release,
    NullRenderer_GetClassID,
    NullRenderer_Stop,
    NullRenderer_Pause,
    NullRenderer_Run,
    NullRenderer_GetState,
    NullRenderer_SetSyncSource,
    NullRenderer_GetSyncSource,
    NullRenderer_EnumPins,
    NullRenderer_FindPin,
    NullRenderer_QueryFilterInfo,
    NullRenderer_JoinFilterGraph,
    NullRenderer_QueryVendorInfo
};

static HRESULT WINAPI NullRenderer_InputPin_EndOfStream(IPin * iface)
{
    InputPin* This = (InputPin*)iface;
    IMediaEventSink* pEventSink;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    hr = IFilterGraph_QueryInterface(((NullRendererImpl*)This->pin.pinInfo.pFilter)->filterInfo.pGraph, &IID_IMediaEventSink, (LPVOID*)&pEventSink);
    if (SUCCEEDED(hr))
    {
        hr = IMediaEventSink_Notify(pEventSink, EC_COMPLETE, S_OK, 0);
        IMediaEventSink_Release(pEventSink);
    }

    return hr;
}

static const IPinVtbl NullRenderer_InputPin_Vtbl =
{
    InputPin_QueryInterface,
    IPinImpl_AddRef,
    InputPin_Release,
    InputPin_Connect,
    InputPin_ReceiveConnection,
    IPinImpl_Disconnect,
    IPinImpl_ConnectedTo,
    IPinImpl_ConnectionMediaType,
    IPinImpl_QueryPinInfo,
    IPinImpl_QueryDirection,
    IPinImpl_QueryId,
    IPinImpl_QueryAccept,
    IPinImpl_EnumMediaTypes,
    IPinImpl_QueryInternalConnections,
    NullRenderer_InputPin_EndOfStream,
    InputPin_BeginFlush,
    InputPin_EndFlush,
    InputPin_NewSegment
};
