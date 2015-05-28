/*
 *    SmartTeeFilter tests
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#define COBJMACROS
#include <dshow.h>
#include <guiddef.h>
#include <devguid.h>
#include <stdio.h>

#include "wine/strmbase.h"
#include "wine/test.h"

typedef struct {
    IBaseFilter IBaseFilter_iface;
    LONG ref;
    IPin IPin_iface;
    IMemInputPin IMemInputPin_iface;
    IBaseFilter *nullRenderer;
    IPin *nullRendererPin;
    IMemInputPin *nullRendererMemInputPin;
} SinkFilter;

typedef struct {
    IEnumPins IEnumPins_iface;
    LONG ref;
    ULONG index;
    SinkFilter *filter;
} SinkEnumPins;

static SinkEnumPins* create_SinkEnumPins(SinkFilter *filter);

static inline SinkFilter* impl_from_SinkFilter_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, SinkFilter, IBaseFilter_iface);
}

static inline SinkFilter* impl_from_SinkFilter_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, SinkFilter, IPin_iface);
}

static inline SinkFilter* impl_from_SinkFilter_IMemInputPin(IMemInputPin *iface)
{
    return CONTAINING_RECORD(iface, SinkFilter, IMemInputPin_iface);
}

static inline SinkEnumPins* impl_from_SinkFilter_IEnumPins(IEnumPins *iface)
{
    return CONTAINING_RECORD(iface, SinkEnumPins, IEnumPins_iface);
}

static HRESULT WINAPI SinkFilter_QueryInterface(IBaseFilter *iface, REFIID riid, void **ppv)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    if(IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = &This->IBaseFilter_iface;
    } else if(IsEqualIID(riid, &IID_IPersist)) {
        *ppv = &This->IBaseFilter_iface;
    } else if(IsEqualIID(riid, &IID_IMediaFilter)) {
        *ppv = &This->IBaseFilter_iface;
    } else if(IsEqualIID(riid, &IID_IBaseFilter)) {
        *ppv = &This->IBaseFilter_iface;
    } else {
        trace("no interface for %s\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI SinkFilter_AddRef(IBaseFilter *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI SinkFilter_Release(IBaseFilter *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    if(!ref) {
        IMemInputPin_Release(This->nullRendererMemInputPin);
        IPin_Release(This->nullRendererPin);
        IBaseFilter_Release(This->nullRenderer);
        CoTaskMemFree(This);
    }
    return ref;
}

static HRESULT WINAPI SinkFilter_GetClassID(IBaseFilter *iface, CLSID *pClassID)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_GetClassID(This->nullRenderer, pClassID);
}

static HRESULT WINAPI SinkFilter_Stop(IBaseFilter *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_Stop(This->nullRenderer);
}

static HRESULT WINAPI SinkFilter_Pause(IBaseFilter *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_Pause(This->nullRenderer);
}

static HRESULT WINAPI SinkFilter_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_Run(This->nullRenderer, tStart);
}

static HRESULT WINAPI SinkFilter_GetState(IBaseFilter *iface, DWORD dwMilliSecsTimeout, FILTER_STATE *state)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_GetState(This->nullRenderer, dwMilliSecsTimeout, state);
}

static HRESULT WINAPI SinkFilter_SetSyncSource(IBaseFilter *iface, IReferenceClock *pClock)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_SetSyncSource(This->nullRenderer, pClock);
}

static HRESULT WINAPI SinkFilter_GetSyncSource(IBaseFilter *iface, IReferenceClock **ppClock)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_GetSyncSource(This->nullRenderer, ppClock);
}

static HRESULT WINAPI SinkFilter_EnumPins(IBaseFilter *iface, IEnumPins **ppEnum)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    SinkEnumPins *sinkEnumPins = create_SinkEnumPins(This);
    if (sinkEnumPins) {
        *ppEnum = &sinkEnumPins->IEnumPins_iface;
        return S_OK;
    }
    else
        return E_OUTOFMEMORY;
}

static HRESULT WINAPI SinkFilter_FindPin(IBaseFilter *iface, LPCWSTR id, IPin **ppPin)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    HRESULT hr = IBaseFilter_FindPin(This->nullRenderer, id, ppPin);
    if (SUCCEEDED(hr)) {
        IPin_Release(*ppPin);
        *ppPin = &This->IPin_iface;
        IPin_AddRef(&This->IPin_iface);
    }
    return hr;
}

static HRESULT WINAPI SinkFilter_QueryFilterInfo(IBaseFilter *iface, FILTER_INFO *pInfo)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_QueryFilterInfo(This->nullRenderer, pInfo);
}

static HRESULT WINAPI SinkFilter_JoinFilterGraph(IBaseFilter *iface, IFilterGraph *pGraph, LPCWSTR pName)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_JoinFilterGraph(This->nullRenderer, pGraph, pName);
}

static HRESULT WINAPI SinkFilter_QueryVendorInfo(IBaseFilter *iface, LPWSTR *pVendorInfo)
{
    SinkFilter *This = impl_from_SinkFilter_IBaseFilter(iface);
    return IBaseFilter_QueryVendorInfo(This->nullRenderer, pVendorInfo);
}

static const IBaseFilterVtbl SinkFilterVtbl = {
    SinkFilter_QueryInterface,
    SinkFilter_AddRef,
    SinkFilter_Release,
    SinkFilter_GetClassID,
    SinkFilter_Stop,
    SinkFilter_Pause,
    SinkFilter_Run,
    SinkFilter_GetState,
    SinkFilter_SetSyncSource,
    SinkFilter_GetSyncSource,
    SinkFilter_EnumPins,
    SinkFilter_FindPin,
    SinkFilter_QueryFilterInfo,
    SinkFilter_JoinFilterGraph,
    SinkFilter_QueryVendorInfo
};

static HRESULT WINAPI SinkEnumPins_QueryInterface(IEnumPins *iface, REFIID riid, void **ppv)
{
    SinkEnumPins *This = impl_from_SinkFilter_IEnumPins(iface);
    if(IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = &This->IEnumPins_iface;
    } else if(IsEqualIID(riid, &IID_IEnumPins)) {
        *ppv = &This->IEnumPins_iface;
    } else {
        trace("no interface for %s\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI SinkEnumPins_AddRef(IEnumPins *iface)
{
    SinkEnumPins *This = impl_from_SinkFilter_IEnumPins(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI SinkEnumPins_Release(IEnumPins *iface)
{
    SinkEnumPins *This = impl_from_SinkFilter_IEnumPins(iface);
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        IBaseFilter_Release(&This->filter->IBaseFilter_iface);
        CoTaskMemFree(This);
    }
    return ref;
}

static HRESULT WINAPI SinkEnumPins_Next(IEnumPins *iface, ULONG cPins, IPin **ppPins, ULONG *pcFetched)
{
    SinkEnumPins *This = impl_from_SinkFilter_IEnumPins(iface);
    if (!ppPins)
        return E_POINTER;
    if (cPins > 1 && !pcFetched)
        return E_INVALIDARG;
    if (pcFetched)
        *pcFetched = 0;
    if (cPins == 0)
        return S_OK;
    if (This->index == 0) {
        ppPins[0] = &This->filter->IPin_iface;
        IPin_AddRef(&This->filter->IPin_iface);
        ++This->index;
        if (pcFetched)
            *pcFetched = 1;
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI SinkEnumPins_Skip(IEnumPins *iface, ULONG cPins)
{
    SinkEnumPins *This = impl_from_SinkFilter_IEnumPins(iface);
    if (This->index + cPins >= 1)
        return S_FALSE;
    This->index += cPins;
    return S_OK;
}

static HRESULT WINAPI SinkEnumPins_Reset(IEnumPins *iface)
{
    SinkEnumPins *This = impl_from_SinkFilter_IEnumPins(iface);
    This->index = 0;
    return S_OK;
}

static HRESULT WINAPI SinkEnumPins_Clone(IEnumPins *iface, IEnumPins **ppEnum)
{
    SinkEnumPins *This = impl_from_SinkFilter_IEnumPins(iface);
    SinkEnumPins *clone = create_SinkEnumPins(This->filter);
    if (clone == NULL)
        return E_OUTOFMEMORY;
    clone->index = This->index;
    return S_OK;
}

static const IEnumPinsVtbl SinkEnumPinsVtbl = {
    SinkEnumPins_QueryInterface,
    SinkEnumPins_AddRef,
    SinkEnumPins_Release,
    SinkEnumPins_Next,
    SinkEnumPins_Skip,
    SinkEnumPins_Reset,
    SinkEnumPins_Clone
};

static SinkEnumPins* create_SinkEnumPins(SinkFilter *filter)
{
    SinkEnumPins *This;
    This = CoTaskMemAlloc(sizeof(*This));
    if (This == NULL) {
        return NULL;
    }
    This->IEnumPins_iface.lpVtbl = &SinkEnumPinsVtbl;
    This->ref = 1;
    This->index = 0;
    This->filter = filter;
    IBaseFilter_AddRef(&filter->IBaseFilter_iface);
    return This;
}

static HRESULT WINAPI SinkPin_QueryInterface(IPin *iface, REFIID riid, void **ppv)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    if(IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = &This->IPin_iface;
    } else if(IsEqualIID(riid, &IID_IPin)) {
        *ppv = &This->IPin_iface;
    } else if(IsEqualIID(riid, &IID_IMemInputPin)) {
        *ppv = &This->IMemInputPin_iface;
    } else {
        trace("no interface for %s\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI SinkPin_AddRef(IPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IBaseFilter_AddRef(&This->IBaseFilter_iface);
}

static ULONG WINAPI SinkPin_Release(IPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IBaseFilter_Release(&This->IBaseFilter_iface);
}

static HRESULT WINAPI SinkPin_Connect(IPin *iface, IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_Connect(This->nullRendererPin, pReceivePin, pmt);
}

static HRESULT WINAPI SinkPin_ReceiveConnection(IPin *iface, IPin *connector, const AM_MEDIA_TYPE *pmt)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_ReceiveConnection(This->nullRendererPin, connector, pmt);
}

static HRESULT WINAPI SinkPin_Disconnect(IPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_Disconnect(This->nullRendererPin);
}

static HRESULT WINAPI SinkPin_ConnectedTo(IPin *iface, IPin **pPin)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_ConnectedTo(This->nullRendererPin, pPin);
}

static HRESULT WINAPI SinkPin_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *pmt)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_ConnectionMediaType(This->nullRendererPin, pmt);
}

static HRESULT WINAPI SinkPin_QueryPinInfo(IPin *iface, PIN_INFO *pInfo)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    HRESULT hr = IPin_QueryPinInfo(This->nullRendererPin, pInfo);
    if (SUCCEEDED(hr)) {
        IBaseFilter_Release(pInfo->pFilter);
        pInfo->pFilter = &This->IBaseFilter_iface;
        IBaseFilter_AddRef(&This->IBaseFilter_iface);
    }
    return hr;
}

static HRESULT WINAPI SinkPin_QueryDirection(IPin *iface, PIN_DIRECTION *pPinDir)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_QueryDirection(This->nullRendererPin, pPinDir);
}

static HRESULT WINAPI SinkPin_QueryId(IPin *iface, LPWSTR *id)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_QueryId(This->nullRendererPin, id);
}

static HRESULT WINAPI SinkPin_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *pmt)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_QueryAccept(This->nullRendererPin, pmt);
}

static HRESULT WINAPI SinkPin_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_EnumMediaTypes(This->nullRendererPin, ppEnum);
}

static HRESULT WINAPI SinkPin_QueryInternalConnections(IPin *iface, IPin **apPin, ULONG *nPin)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_QueryInternalConnections(This->nullRendererPin, apPin, nPin);
}

static HRESULT WINAPI SinkPin_EndOfStream(IPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_EndOfStream(This->nullRendererPin);
}

static HRESULT WINAPI SinkPin_BeginFlush(IPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_BeginFlush(This->nullRendererPin);
}

static HRESULT WINAPI SinkPin_EndFlush(IPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_EndFlush(This->nullRendererPin);
}

static HRESULT WINAPI SinkPin_NewSegment(IPin *iface, REFERENCE_TIME tStart,
        REFERENCE_TIME tStop, double dRate)
{
    SinkFilter *This = impl_from_SinkFilter_IPin(iface);
    return IPin_NewSegment(This->nullRendererPin, tStart, tStop, dRate);
}

static const IPinVtbl SinkPinVtbl = {
    SinkPin_QueryInterface,
    SinkPin_AddRef,
    SinkPin_Release,
    SinkPin_Connect,
    SinkPin_ReceiveConnection,
    SinkPin_Disconnect,
    SinkPin_ConnectedTo,
    SinkPin_ConnectionMediaType,
    SinkPin_QueryPinInfo,
    SinkPin_QueryDirection,
    SinkPin_QueryId,
    SinkPin_QueryAccept,
    SinkPin_EnumMediaTypes,
    SinkPin_QueryInternalConnections,
    SinkPin_EndOfStream,
    SinkPin_BeginFlush,
    SinkPin_EndFlush,
    SinkPin_NewSegment
};

static HRESULT WINAPI SinkMemInputPin_QueryInterface(IMemInputPin *iface, REFIID riid, void **ppv)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    return IPin_QueryInterface(&This->IPin_iface, riid, ppv);
}

static ULONG WINAPI SinkMemInputPin_AddRef(IMemInputPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    return IBaseFilter_AddRef(&This->IBaseFilter_iface);
}

static ULONG WINAPI SinkMemInputPin_Release(IMemInputPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    return IBaseFilter_Release(&This->IBaseFilter_iface);
}

static HRESULT WINAPI SinkMemInputPin_GetAllocator(IMemInputPin *iface, IMemAllocator **ppAllocator)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    return IMemInputPin_GetAllocator(This->nullRendererMemInputPin, ppAllocator);
}

static HRESULT WINAPI SinkMemInputPin_NotifyAllocator(IMemInputPin *iface, IMemAllocator *pAllocator,
        BOOL bReadOnly)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    return IMemInputPin_NotifyAllocator(This->nullRendererMemInputPin, pAllocator, bReadOnly);
}

static HRESULT WINAPI SinkMemInputPin_GetAllocatorRequirements(IMemInputPin *iface,
        ALLOCATOR_PROPERTIES *pProps)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    return IMemInputPin_GetAllocatorRequirements(This->nullRendererMemInputPin, pProps);
}

static HRESULT WINAPI SinkMemInputPin_Receive(IMemInputPin *iface, IMediaSample *pSample)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    return IMemInputPin_Receive(This->nullRendererMemInputPin, pSample);
}

static HRESULT WINAPI SinkMemInputPin_ReceiveMultiple(IMemInputPin *iface, IMediaSample **pSamples,
        LONG nSamples, LONG *nSamplesProcessed)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    return IMemInputPin_ReceiveMultiple(This->nullRendererMemInputPin, pSamples,
            nSamples, nSamplesProcessed);
}

static HRESULT WINAPI SinkMemInputPin_ReceiveCanBlock(IMemInputPin *iface)
{
    SinkFilter *This = impl_from_SinkFilter_IMemInputPin(iface);
    return IMemInputPin_ReceiveCanBlock(This->nullRendererMemInputPin);
}

static const IMemInputPinVtbl SinkMemInputPinVtbl = {
    SinkMemInputPin_QueryInterface,
    SinkMemInputPin_AddRef,
    SinkMemInputPin_Release,
    SinkMemInputPin_GetAllocator,
    SinkMemInputPin_NotifyAllocator,
    SinkMemInputPin_GetAllocatorRequirements,
    SinkMemInputPin_Receive,
    SinkMemInputPin_ReceiveMultiple,
    SinkMemInputPin_ReceiveCanBlock
};

static SinkFilter* create_SinkFilter(void)
{
    SinkFilter *This = NULL;
    HRESULT hr;
    This = CoTaskMemAlloc(sizeof(*This));
    if (This) {
        memset(This, 0, sizeof(*This));
        This->IBaseFilter_iface.lpVtbl = &SinkFilterVtbl;
        This->ref = 1;
        This->IPin_iface.lpVtbl = &SinkPinVtbl;
        This->IMemInputPin_iface.lpVtbl = &SinkMemInputPinVtbl;
        hr = CoCreateInstance(&CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
                &IID_IBaseFilter, (LPVOID*)&This->nullRenderer);
        if (SUCCEEDED(hr)) {
            IEnumPins *enumPins = NULL;
            hr = IBaseFilter_EnumPins(This->nullRenderer, &enumPins);
            if (SUCCEEDED(hr)) {
                hr = IEnumPins_Next(enumPins, 1, &This->nullRendererPin, NULL);
                IEnumPins_Release(enumPins);
                if (SUCCEEDED(hr)) {
                    hr = IPin_QueryInterface(This->nullRendererPin, &IID_IMemInputPin,
                            (LPVOID*)&This->nullRendererMemInputPin);
                    if (SUCCEEDED(hr))
                        return This;
                    IPin_Release(This->nullRendererPin);
                }
            }
            IBaseFilter_Release(This->nullRenderer);
        }
        CoTaskMemFree(This);
    }
    return NULL;
}

static BOOL has_interface(IUnknown *unknown, REFIID uuid)
{
     HRESULT hr;
     IUnknown *iface = NULL;

     hr = IUnknown_QueryInterface(unknown, uuid, (void**)&iface);
     if (SUCCEEDED(hr))
     {
         IUnknown_Release(iface);
         return TRUE;
     }
     else
         return FALSE;
}

static void test_smart_tee_filter_in_graph(IBaseFilter *smartTeeFilter, IPin *inputPin,
        IPin *capturePin, IPin *previewPin)
{
    HRESULT hr;
    IGraphBuilder *graphBuilder = NULL;
    SinkFilter *sinkFilter = NULL;

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder,
            (LPVOID*)&graphBuilder);
    ok(SUCCEEDED(hr), "couldn't create graph builder, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    hr = IGraphBuilder_AddFilter(graphBuilder, smartTeeFilter, NULL);
    ok(SUCCEEDED(hr), "couldn't add smart tee filter to graph, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    sinkFilter = create_SinkFilter();
    if (sinkFilter == NULL) {
        trace("couldn't create sink filter\n");
        goto end;
    }
    hr = IGraphBuilder_AddFilter(graphBuilder, &sinkFilter->IBaseFilter_iface, NULL);
    if (FAILED(hr)) {
        skip("couldn't add sink filter to graph, hr=0x%08x\n", hr);
        goto end;
    }

    hr = IGraphBuilder_Connect(graphBuilder, capturePin, &sinkFilter->IPin_iface);
    ok(hr == VFW_E_NOT_CONNECTED, "connecting Capture pin without first connecting input pin returned 0x%08x\n", hr);
    hr = IGraphBuilder_Connect(graphBuilder, previewPin, &sinkFilter->IPin_iface);
    ok(hr == VFW_E_NOT_CONNECTED, "connecting Preview pin without first connecting input pin returned 0x%08x\n", hr);

end:
    if (sinkFilter)
        IBaseFilter_Release(&sinkFilter->IBaseFilter_iface);
    if (graphBuilder)
        IGraphBuilder_Release(graphBuilder);
}

static void test_smart_tee_filter(void)
{
    HRESULT hr;
    IBaseFilter *smartTeeFilter = NULL;
    IEnumPins *enumPins = NULL;
    IPin *pin;
    IPin *inputPin = NULL;
    IPin *capturePin = NULL;
    IPin *previewPin = NULL;
    FILTER_INFO filterInfo;
    int pinNumber = 0;
    IMemInputPin *memInputPin = NULL;
    IEnumMediaTypes *enumMediaTypes = NULL;
    ULONG nPin = 0;

    hr = CoCreateInstance(&CLSID_SmartTee, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void**)&smartTeeFilter);
    todo_wine ok(SUCCEEDED(hr), "couldn't create smart tee filter, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    hr = IBaseFilter_QueryFilterInfo(smartTeeFilter, &filterInfo);
    ok(SUCCEEDED(hr), "QueryFilterInfo failed, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    ok(lstrlenW(filterInfo.achName) == 0,
            "filter's name is meant to be empty but it's %s\n", wine_dbgstr_w(filterInfo.achName));

    hr = IBaseFilter_EnumPins(smartTeeFilter, &enumPins);
    ok(SUCCEEDED(hr), "cannot enum filter pins, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;

    while (IEnumPins_Next(enumPins, 1, &pin, NULL) == S_OK)
    {
        LPWSTR id = NULL;
        PIN_INFO pinInfo;
        memset(&pinInfo, 0, sizeof(pinInfo));
        hr = IPin_QueryPinInfo(pin, &pinInfo);
        ok(SUCCEEDED(hr), "QueryPinInfo failed, hr=0x%08x\n", hr);
        if (FAILED(hr))
            goto endwhile;

        ok(pinInfo.pFilter == smartTeeFilter, "pin's filter isn't the filter owning the pin\n");
        if (pinNumber == 0)
        {
            static const WCHAR wszInput[] = {'I','n','p','u','t',0};
            ok(pinInfo.dir == PINDIR_INPUT, "pin 0 isn't an input pin\n");
            ok(!lstrcmpW(pinInfo.achName, wszInput), "pin 0 is called %s, not 'Input'\n", wine_dbgstr_w(pinInfo.achName));
            hr = IPin_QueryId(pin, &id);
            ok(SUCCEEDED(hr), "IPin_QueryId() failed with 0x%08x\n", hr);
            ok(!lstrcmpW(id, wszInput), "pin 0's id is %s, not 'Input'\n", wine_dbgstr_w(id));
            inputPin = pin;
            IPin_AddRef(inputPin);
        }
        else if (pinNumber == 1)
        {
            static const WCHAR wszCapture[] = {'C','a','p','t','u','r','e',0};
            ok(pinInfo.dir == PINDIR_OUTPUT, "pin 1 isn't an output pin\n");
            ok(!lstrcmpW(pinInfo.achName, wszCapture), "pin 1 is called %s, not 'Capture'\n", wine_dbgstr_w(pinInfo.achName));
            hr = IPin_QueryId(pin, &id);
            ok(SUCCEEDED(hr), "IPin_QueryId() failed with 0x%08x\n", hr);
            ok(!lstrcmpW(id, wszCapture), "pin 1's id is %s, not 'Capture'\n", wine_dbgstr_w(id));
            capturePin = pin;
            IPin_AddRef(capturePin);
        }
        else if (pinNumber == 2)
        {
            static const WCHAR wszPreview[] = {'P','r','e','v','i','e','w',0};
            ok(pinInfo.dir == PINDIR_OUTPUT, "pin 2 isn't an output pin\n");
            ok(!lstrcmpW(pinInfo.achName, wszPreview), "pin 2 is called %s, not 'Preview'\n", wine_dbgstr_w(pinInfo.achName));
            hr = IPin_QueryId(pin, &id);
            ok(SUCCEEDED(hr), "IPin_QueryId() failed with 0x%08x\n", hr);
            ok(!lstrcmpW(id, wszPreview), "pin 2's id is %s, not 'Preview'\n", wine_dbgstr_w(id));
            previewPin = pin;
            IPin_AddRef(previewPin);
        }
        else
            ok(0, "pin %d isn't supposed to exist\n", pinNumber);

    endwhile:
        if (pinInfo.pFilter)
            IBaseFilter_Release(pinInfo.pFilter);
        if (id)
            CoTaskMemFree(id);
        IPin_Release(pin);
        pinNumber++;
    }

    ok(inputPin && capturePin && previewPin, "couldn't find all pins\n");
    if (!(inputPin && capturePin && previewPin))
        goto end;

    ok(has_interface((IUnknown*)inputPin, &IID_IUnknown), "IUnknown should exist on the input pin\n");
    ok(has_interface((IUnknown*)inputPin, &IID_IMemInputPin), "IMemInputPin should exist the input pin\n");
    ok(!has_interface((IUnknown*)inputPin, &IID_IKsPropertySet), "IKsPropertySet shouldn't eixst on the input pin\n");
    ok(!has_interface((IUnknown*)inputPin, &IID_IAMStreamConfig), "IAMStreamConfig shouldn't exist on the input pin\n");
    ok(!has_interface((IUnknown*)inputPin, &IID_IAMStreamControl), "IAMStreamControl shouldn't exist on the input pin\n");
    ok(!has_interface((IUnknown*)inputPin, &IID_IPropertyBag), "IPropertyBag shouldn't exist on the input pin\n");
    ok(has_interface((IUnknown*)inputPin, &IID_IQualityControl), "IQualityControl should exist on the input pin\n");

    ok(has_interface((IUnknown*)capturePin, &IID_IUnknown), "IUnknown should exist on the capture pin\n");
    ok(!has_interface((IUnknown*)capturePin, &IID_IAsyncReader), "IAsyncReader shouldn't exist on the capture pin\n");
    ok(!has_interface((IUnknown*)capturePin, &IID_IKsPropertySet), "IKsPropertySet shouldn't exist on the capture pin\n");
    ok(!has_interface((IUnknown*)capturePin, &IID_IAMStreamConfig), "IAMStreamConfig shouldn't exist on the capture pin\n");
    ok(has_interface((IUnknown*)capturePin, &IID_IAMStreamControl), "IAMStreamControl should exist on the capture pin\n");
    ok(!has_interface((IUnknown*)capturePin, &IID_IPropertyBag), "IPropertyBag shouldn't exist on the capture pin\n");
    ok(has_interface((IUnknown*)capturePin, &IID_IQualityControl), "IQualityControl should exist on the capture pin\n");

    ok(has_interface((IUnknown*)previewPin, &IID_IUnknown), "IUnknown should exist on the preview pin\n");
    ok(!has_interface((IUnknown*)previewPin, &IID_IAsyncReader), "IAsyncReader shouldn't exist on the preview pin\n");
    ok(!has_interface((IUnknown*)previewPin, &IID_IKsPropertySet), "IKsPropertySet shouldn't exist on the preview pin\n");
    ok(!has_interface((IUnknown*)previewPin, &IID_IAMStreamConfig), "IAMStreamConfig shouldn't exist on the preview pin\n");
    ok(has_interface((IUnknown*)capturePin, &IID_IAMStreamControl), "IAMStreamControl should exist on the preview pin\n");
    ok(!has_interface((IUnknown*)previewPin, &IID_IPropertyBag), "IPropertyBag shouldn't exist on the preview pin\n");
    ok(has_interface((IUnknown*)previewPin, &IID_IQualityControl), "IQualityControl should exist on the preview pin\n");

    hr = IPin_QueryInterface(inputPin, &IID_IMemInputPin, (void**)&memInputPin);
    ok(SUCCEEDED(hr), "couldn't get mem input pin, hr=0x%08x\n", hr);
    if (FAILED(hr))
        goto end;
    hr = IMemInputPin_ReceiveCanBlock(memInputPin);
    ok(hr == S_OK, "unexpected IMemInputPin_ReceiveCanBlock() = 0x%08x\n", hr);

    hr = IPin_EnumMediaTypes(inputPin, &enumMediaTypes);
    ok(SUCCEEDED(hr), "IPin_EnumMediaTypes() failed, hr=0x%08x\n", hr);
    if (SUCCEEDED(hr)) {
        AM_MEDIA_TYPE *mediaType = NULL;
        hr = IEnumMediaTypes_Next(enumMediaTypes, 1, &mediaType, NULL);
        ok(hr == S_FALSE, "the media types are non-empty\n");
    }
    IEnumMediaTypes_Release(enumMediaTypes);
    enumMediaTypes = NULL;
    hr = IPin_EnumMediaTypes(capturePin, &enumMediaTypes);
    ok(hr == VFW_E_NOT_CONNECTED, "IPin_EnumMediaTypes() failed, hr=0x%08x\n", hr);
    hr = IPin_EnumMediaTypes(previewPin, &enumMediaTypes);
    ok(hr == VFW_E_NOT_CONNECTED, "IPin_EnumMediaTypes() failed, hr=0x%08x\n", hr);

    hr = IPin_QueryInternalConnections(inputPin, NULL, &nPin);
    ok(hr == E_NOTIMPL, "IPin_QueryInternalConnections() returned hr=0x%08x\n", hr);
    hr = IPin_QueryInternalConnections(capturePin, NULL, &nPin);
    ok(hr == E_NOTIMPL, "IPin_QueryInternalConnections() returned hr=0x%08x\n", hr);
    hr = IPin_QueryInternalConnections(previewPin, NULL, &nPin);
    ok(hr == E_NOTIMPL, "IPin_QueryInternalConnections() returned hr=0x%08x\n", hr);

    test_smart_tee_filter_in_graph(smartTeeFilter, inputPin, capturePin, previewPin);

end:
    if (inputPin)
        IPin_Release(inputPin);
    if (capturePin)
        IPin_Release(capturePin);
    if (previewPin)
        IPin_Release(previewPin);
    if (smartTeeFilter)
        IBaseFilter_Release(smartTeeFilter);
    if (enumPins)
        IEnumPins_Release(enumPins);
    if (memInputPin)
        IMemInputPin_Release(memInputPin);
    if (enumMediaTypes)
        IEnumMediaTypes_Release(enumMediaTypes);
}

START_TEST(smartteefilter)
{
    if (SUCCEEDED(CoInitialize(NULL)))
    {
        test_smart_tee_filter();

        CoUninitialize();
    }
    else
        skip("CoInitialize failed\n");
}
