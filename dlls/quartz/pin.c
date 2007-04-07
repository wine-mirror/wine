/*
 * Generic Implementation of IPin Interface
 *
 * Copyright 2003 Robert Shearman
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

#include "quartz_private.h"
#include "pin.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "uuids.h"
#include "vfwmsgs.h"
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const IPinVtbl InputPin_Vtbl;
static const IPinVtbl OutputPin_Vtbl;
static const IMemInputPinVtbl MemInputPin_Vtbl;
static const IPinVtbl PullPin_Vtbl;

#define ALIGNDOWN(value,boundary) ((value)/(boundary)*(boundary))
#define ALIGNUP(value,boundary) (ALIGNDOWN((value)+(boundary)-1, (boundary)))

static inline InputPin *impl_from_IMemInputPin( IMemInputPin *iface )
{
    return (InputPin *)((char*)iface - FIELD_OFFSET(InputPin, lpVtblMemInput));
}


static void Copy_PinInfo(PIN_INFO * pDest, const PIN_INFO * pSrc)
{
    /* Tempting to just do a memcpy, but the name field is
       128 characters long! We will probably never exceed 10
       most of the time, so we are better off copying 
       each field manually */
    strcpyW(pDest->achName, pSrc->achName);
    pDest->dir = pSrc->dir;
    pDest->pFilter = pSrc->pFilter;
}

/* Function called as a helper to IPin_Connect */
/* specific AM_MEDIA_TYPE - it cannot be NULL */
/* NOTE: not part of standard interface */
static HRESULT OutputPin_ConnectSpecific(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    OutputPin *This = (OutputPin *)iface;
    HRESULT hr;
    IMemAllocator * pMemAlloc = NULL;
    ALLOCATOR_PROPERTIES actual; /* FIXME: should we put the actual props back in to This? */

    TRACE("(%p, %p)\n", pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    /* FIXME: call queryacceptproc */

    This->pin.pConnectedTo = pReceivePin;
    IPin_AddRef(pReceivePin);
    CopyMediaType(&This->pin.mtCurrent, pmt);

    hr = IPin_ReceiveConnection(pReceivePin, iface, pmt);

    /* get the IMemInputPin interface we will use to deliver samples to the
     * connected pin */
    if (SUCCEEDED(hr))
    {
        This->pMemInputPin = NULL;
        hr = IPin_QueryInterface(pReceivePin, &IID_IMemInputPin, (LPVOID)&This->pMemInputPin);

        if (SUCCEEDED(hr))
        {
            hr = IMemInputPin_GetAllocator(This->pMemInputPin, &pMemAlloc);

            if (hr == VFW_E_NO_ALLOCATOR)
            {
                /* Input pin provides no allocator, use standard memory allocator */
                hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER, &IID_IMemAllocator, (LPVOID*)&pMemAlloc);

                if (SUCCEEDED(hr))
                {
                    hr = IMemInputPin_NotifyAllocator(This->pMemInputPin, pMemAlloc, FALSE);
                }
            }

            if (SUCCEEDED(hr))
                hr = IMemAllocator_SetProperties(pMemAlloc, &This->allocProps, &actual);

            if (pMemAlloc)
                IMemAllocator_Release(pMemAlloc);
        }

        /* break connection if we couldn't get the allocator */
        if (FAILED(hr))
        {
            if (This->pMemInputPin)
                IMemInputPin_Release(This->pMemInputPin);
            This->pMemInputPin = NULL;

            IPin_Disconnect(pReceivePin);
        }
    }

    if (FAILED(hr))
    {
        IPin_Release(This->pin.pConnectedTo);
        This->pin.pConnectedTo = NULL;
        FreeMediaType(&This->pin.mtCurrent);
    }

    TRACE(" -- %x\n", hr);
    return hr;
}

HRESULT InputPin_Construct(const PIN_INFO * pPinInfo, SAMPLEPROC pSampleProc, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, IPin ** ppPin)
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
        pPinImpl->pin.lpVtbl = &InputPin_Vtbl;
        pPinImpl->lpVtblMemInput = &MemInputPin_Vtbl;
        
        *ppPin = (IPin *)(&pPinImpl->pin.lpVtbl);
        return S_OK;
    }
    return E_FAIL;
}

/* Note that we don't init the vtables here (like C++ constructor) */
HRESULT InputPin_Init(const PIN_INFO * pPinInfo, SAMPLEPROC pSampleProc, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, InputPin * pPinImpl)
{
    TRACE("\n");

    /* Common attributes */
    pPinImpl->pin.refCount = 1;
    pPinImpl->pin.pConnectedTo = NULL;
    pPinImpl->pin.fnQueryAccept = pQueryAccept;
    pPinImpl->pin.pUserData = pUserData;
    pPinImpl->pin.pCritSec = pCritSec;
    Copy_PinInfo(&pPinImpl->pin.pinInfo, pPinInfo);
    ZeroMemory(&pPinImpl->pin.mtCurrent, sizeof(AM_MEDIA_TYPE));

    /* Input pin attributes */
    pPinImpl->fnSampleProc = pSampleProc;
    pPinImpl->pAllocator = NULL;
    pPinImpl->tStart = 0;
    pPinImpl->tStop = 0;
    pPinImpl->dRate = 0;

    return S_OK;
}

HRESULT OutputPin_Init(const PIN_INFO * pPinInfo, ALLOCATOR_PROPERTIES * props, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, OutputPin * pPinImpl)
{
    TRACE("\n");

    /* Common attributes */
    pPinImpl->pin.lpVtbl = &OutputPin_Vtbl;
    pPinImpl->pin.refCount = 1;
    pPinImpl->pin.pConnectedTo = NULL;
    pPinImpl->pin.fnQueryAccept = pQueryAccept;
    pPinImpl->pin.pUserData = pUserData;
    pPinImpl->pin.pCritSec = pCritSec;
    Copy_PinInfo(&pPinImpl->pin.pinInfo, pPinInfo);
    ZeroMemory(&pPinImpl->pin.mtCurrent, sizeof(AM_MEDIA_TYPE));

    /* Output pin attributes */
    pPinImpl->pMemInputPin = NULL;
    pPinImpl->pConnectSpecific = OutputPin_ConnectSpecific;
    if (props)
    {
        memcpy(&pPinImpl->allocProps, props, sizeof(pPinImpl->allocProps));
        if (pPinImpl->allocProps.cbAlign == 0)
            pPinImpl->allocProps.cbAlign = 1;
    }
    else
        ZeroMemory(&pPinImpl->allocProps, sizeof(pPinImpl->allocProps));

    return S_OK;
}

HRESULT OutputPin_Construct(const PIN_INFO * pPinInfo, ALLOCATOR_PROPERTIES *props, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, IPin ** ppPin)
{
    OutputPin * pPinImpl;

    *ppPin = NULL;

    if (pPinInfo->dir != PINDIR_OUTPUT)
    {
        ERR("Pin direction(%x) != PINDIR_OUTPUT\n", pPinInfo->dir);
        return E_INVALIDARG;
    }

    pPinImpl = CoTaskMemAlloc(sizeof(*pPinImpl));

    if (!pPinImpl)
        return E_OUTOFMEMORY;

    if (SUCCEEDED(OutputPin_Init(pPinInfo, props, pUserData, pQueryAccept, pCritSec, pPinImpl)))
    {
        pPinImpl->pin.lpVtbl = &OutputPin_Vtbl;
        
        *ppPin = (IPin *)(&pPinImpl->pin.lpVtbl);
        return S_OK;
    }
    return E_FAIL;
}

/*** Common pin functions ***/

ULONG WINAPI IPinImpl_AddRef(IPin * iface)
{
    IPinImpl *This = (IPinImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->refCount);
    
    TRACE("(%p)->() AddRef from %d\n", iface, refCount - 1);
    
    return refCount;
}

HRESULT WINAPI IPinImpl_Disconnect(IPin * iface)
{
    HRESULT hr;
    IPinImpl *This = (IPinImpl *)iface;

    TRACE("()\n");

    EnterCriticalSection(This->pCritSec);
    {
        if (This->pConnectedTo)
        {
            IPin_Release(This->pConnectedTo);
            This->pConnectedTo = NULL;
            hr = S_OK;
        }
        else
            hr = S_FALSE;
    }
    LeaveCriticalSection(This->pCritSec);
    
    return hr;
}

HRESULT WINAPI IPinImpl_ConnectedTo(IPin * iface, IPin ** ppPin)
{
    HRESULT hr;
    IPinImpl *This = (IPinImpl *)iface;

/*  TRACE("(%p)\n", ppPin);*/

    EnterCriticalSection(This->pCritSec);
    {
        if (This->pConnectedTo)
        {
            *ppPin = This->pConnectedTo;
            IPin_AddRef(*ppPin);
            hr = S_OK;
        }
        else
            hr = VFW_E_NOT_CONNECTED;
    }
    LeaveCriticalSection(This->pCritSec);

    return hr;
}

HRESULT WINAPI IPinImpl_ConnectionMediaType(IPin * iface, AM_MEDIA_TYPE * pmt)
{
    HRESULT hr;
    IPinImpl *This = (IPinImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pmt);

    EnterCriticalSection(This->pCritSec);
    {
        if (This->pConnectedTo)
        {
            CopyMediaType(pmt, &This->mtCurrent);
            hr = S_OK;
        }
        else
        {
            ZeroMemory(pmt, sizeof(*pmt));
            hr = VFW_E_NOT_CONNECTED;
        }
    }
    LeaveCriticalSection(This->pCritSec);

    return hr;
}

HRESULT WINAPI IPinImpl_QueryPinInfo(IPin * iface, PIN_INFO * pInfo)
{
    IPinImpl *This = (IPinImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pInfo);

    Copy_PinInfo(pInfo, &This->pinInfo);
    IBaseFilter_AddRef(pInfo->pFilter);

    return S_OK;
}

HRESULT WINAPI IPinImpl_QueryDirection(IPin * iface, PIN_DIRECTION * pPinDir)
{
    IPinImpl *This = (IPinImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pPinDir);

    *pPinDir = This->pinInfo.dir;

    return S_OK;
}

HRESULT WINAPI IPinImpl_QueryId(IPin * iface, LPWSTR * Id)
{
    IPinImpl *This = (IPinImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, Id);

    *Id = CoTaskMemAlloc((strlenW(This->pinInfo.achName) + 1) * sizeof(WCHAR));
    if (!*Id)
        return E_OUTOFMEMORY;

    strcpyW(*Id, This->pinInfo.achName);

    return S_OK;
}

HRESULT WINAPI IPinImpl_QueryAccept(IPin * iface, const AM_MEDIA_TYPE * pmt)
{
    IPinImpl *This = (IPinImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pmt);

    return (This->fnQueryAccept(This->pUserData, pmt) == S_OK ? S_OK : S_FALSE);
}

HRESULT WINAPI IPinImpl_EnumMediaTypes(IPin * iface, IEnumMediaTypes ** ppEnum)
{
    IPinImpl *This = (IPinImpl *)iface;
    ENUMMEDIADETAILS emd;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    /* override this method to allow enumeration of your types */
    emd.cMediaTypes = 0;
    emd.pMediaTypes = NULL;

    return IEnumMediaTypesImpl_Construct(&emd, ppEnum);
}

HRESULT WINAPI IPinImpl_QueryInternalConnections(IPin * iface, IPin ** apPin, ULONG * cPin)
{
    IPinImpl *This = (IPinImpl *)iface;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, apPin, cPin);

    return E_NOTIMPL; /* to tell caller that all input pins connected to all output pins */
}

/*** IPin implementation for an input pin ***/

HRESULT WINAPI InputPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    InputPin *This = (InputPin *)iface;

    TRACE("(%p)->(%s, %p)\n", iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)iface;
    else if (IsEqualIID(riid, &IID_IPin))
        *ppv = (LPVOID)iface;
    else if (IsEqualIID(riid, &IID_IMemInputPin))
        *ppv = (LPVOID)&This->lpVtblMemInput;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

ULONG WINAPI InputPin_Release(IPin * iface)
{
    InputPin *This = (InputPin *)iface;
    ULONG refCount = InterlockedDecrement(&This->pin.refCount);
    
    TRACE("(%p)->() Release from %d\n", iface, refCount + 1);
    
    if (!refCount)
    {
        FreeMediaType(&This->pin.mtCurrent);
        if (This->pAllocator)
            IMemAllocator_Release(This->pAllocator);
        CoTaskMemFree(This);
        return 0;
    }
    else
        return refCount;
}

HRESULT WINAPI InputPin_Connect(IPin * iface, IPin * pConnector, const AM_MEDIA_TYPE * pmt)
{
    ERR("Outgoing connection on an input pin! (%p, %p)\n", pConnector, pmt);

    return E_UNEXPECTED;
}


HRESULT WINAPI InputPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    InputPin *This = (InputPin *)iface;
    PIN_DIRECTION pindirReceive;
    HRESULT hr = S_OK;

    TRACE("(%p, %p)\n", pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (This->pin.pConnectedTo)
            hr = VFW_E_ALREADY_CONNECTED;

        if (SUCCEEDED(hr) && This->pin.fnQueryAccept(This->pin.pUserData, pmt) != S_OK)
            hr = VFW_E_TYPE_NOT_ACCEPTED; /* FIXME: shouldn't we just map common errors onto
                                           * VFW_E_TYPE_NOT_ACCEPTED and pass the value on otherwise? */

        if (SUCCEEDED(hr))
        {
            IPin_QueryDirection(pReceivePin, &pindirReceive);

            if (pindirReceive != PINDIR_OUTPUT)
            {
                ERR("Can't connect from non-output pin\n");
                hr = VFW_E_INVALID_DIRECTION;
            }
        }

        if (SUCCEEDED(hr))
        {
            CopyMediaType(&This->pin.mtCurrent, pmt);
            This->pin.pConnectedTo = pReceivePin;
            IPin_AddRef(pReceivePin);
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}

HRESULT WINAPI InputPin_EndOfStream(IPin * iface)
{
    TRACE("()\n");

    return S_OK;
}

HRESULT WINAPI InputPin_BeginFlush(IPin * iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

HRESULT WINAPI InputPin_EndFlush(IPin * iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

HRESULT WINAPI InputPin_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    InputPin *This = (InputPin *)iface;

    TRACE("(%x%08x, %x%08x, %e)\n", (ULONG)(tStart >> 32), (ULONG)tStart, (ULONG)(tStop >> 32), (ULONG)tStop, dRate);

    This->tStart = tStart;
    This->tStop = tStop;
    This->dRate = dRate;

    return S_OK;
}

static const IPinVtbl InputPin_Vtbl = 
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
    InputPin_EndOfStream,
    InputPin_BeginFlush,
    InputPin_EndFlush,
    InputPin_NewSegment
};

/*** IMemInputPin implementation ***/

HRESULT WINAPI MemInputPin_QueryInterface(IMemInputPin * iface, REFIID riid, LPVOID * ppv)
{
    InputPin *This = impl_from_IMemInputPin(iface);

    return IPin_QueryInterface((IPin *)&This->pin, riid, ppv);
}

ULONG WINAPI MemInputPin_AddRef(IMemInputPin * iface)
{
    InputPin *This = impl_from_IMemInputPin(iface);

    return IPin_AddRef((IPin *)&This->pin);
}

ULONG WINAPI MemInputPin_Release(IMemInputPin * iface)
{
    InputPin *This = impl_from_IMemInputPin(iface);

    return IPin_Release((IPin *)&This->pin);
}

HRESULT WINAPI MemInputPin_GetAllocator(IMemInputPin * iface, IMemAllocator ** ppAllocator)
{
    InputPin *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppAllocator);

    *ppAllocator = This->pAllocator;
    if (*ppAllocator)
        IMemAllocator_AddRef(*ppAllocator);
    
    return *ppAllocator ? S_OK : VFW_E_NO_ALLOCATOR;
}

HRESULT WINAPI MemInputPin_NotifyAllocator(IMemInputPin * iface, IMemAllocator * pAllocator, BOOL bReadOnly)
{
    InputPin *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->(%p, %d)\n", This, iface, pAllocator, bReadOnly);

    if (This->pAllocator)
        IMemAllocator_Release(This->pAllocator);
    This->pAllocator = pAllocator;
    if (This->pAllocator)
        IMemAllocator_AddRef(This->pAllocator);

    return S_OK;
}

HRESULT WINAPI MemInputPin_GetAllocatorRequirements(IMemInputPin * iface, ALLOCATOR_PROPERTIES * pProps)
{
    InputPin *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pProps);

    /* override this method if you have any specific requirements */

    return E_NOTIMPL;
}

HRESULT WINAPI MemInputPin_Receive(IMemInputPin * iface, IMediaSample * pSample)
{
    InputPin *This = impl_from_IMemInputPin(iface);

    /* this trace commented out for performance reasons */
    /*TRACE("(%p/%p)->(%p)\n", This, iface, pSample);*/

    return This->fnSampleProc(This->pin.pUserData, pSample);
}

HRESULT WINAPI MemInputPin_ReceiveMultiple(IMemInputPin * iface, IMediaSample ** pSamples, long nSamples, long *nSamplesProcessed)
{
    HRESULT hr = S_OK;
    InputPin *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->(%p, %ld, %p)\n", This, iface, pSamples, nSamples, nSamplesProcessed);

    for (*nSamplesProcessed = 0; *nSamplesProcessed < nSamples; (*nSamplesProcessed)++)
    {
        hr = IMemInputPin_Receive(iface, pSamples[*nSamplesProcessed]);
        if (hr != S_OK)
            break;
    }

    return hr;
}

HRESULT WINAPI MemInputPin_ReceiveCanBlock(IMemInputPin * iface)
{
    InputPin *This = impl_from_IMemInputPin(iface);

    FIXME("(%p/%p)->()\n", This, iface);

    /* FIXME: we should check whether any output pins will block */

    return S_OK;
}

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

HRESULT WINAPI OutputPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    OutputPin *This = (OutputPin *)iface;

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)iface;
    else if (IsEqualIID(riid, &IID_IPin))
        *ppv = (LPVOID)iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

ULONG WINAPI OutputPin_Release(IPin * iface)
{
    OutputPin *This = (OutputPin *)iface;
    ULONG refCount = InterlockedDecrement(&This->pin.refCount);
    
    TRACE("(%p)->() Release from %d\n", iface, refCount + 1);
    
    if (!refCount)
    {
        FreeMediaType(&This->pin.mtCurrent);
        CoTaskMemFree(This);
        return 0;
    }
    return refCount;
}

HRESULT WINAPI OutputPin_Connect(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    HRESULT hr;
    OutputPin *This = (OutputPin *)iface;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    /* If we try to connect to ourself, we will definitely deadlock.
     * There are other cases where we could deadlock too, but this
     * catches the obvious case */
    assert(pReceivePin != iface);

    EnterCriticalSection(This->pin.pCritSec);
    {
        /* if we have been a specific type to connect with, then we can either connect
         * with that or fail. We cannot choose different AM_MEDIA_TYPE */
        if (pmt && !IsEqualGUID(&pmt->majortype, &GUID_NULL) && !IsEqualGUID(&pmt->subtype, &GUID_NULL))
            hr = This->pConnectSpecific(iface, pReceivePin, pmt);
        else
        {
            /* negotiate media type */

            IEnumMediaTypes * pEnumCandidates;
            AM_MEDIA_TYPE * pmtCandidate; /* Candidate media type */

            if (SUCCEEDED(hr = IPin_EnumMediaTypes(iface, &pEnumCandidates)))
            {
                hr = VFW_E_NO_ACCEPTABLE_TYPES; /* Assume the worst, but set to S_OK if connected successfully */

                /* try this filter's media types first */
                while (S_OK == IEnumMediaTypes_Next(pEnumCandidates, 1, &pmtCandidate, NULL))
                {
                    if (( !pmt || CompareMediaTypes(pmt, pmtCandidate, TRUE) ) && 
                        (This->pConnectSpecific(iface, pReceivePin, pmtCandidate) == S_OK))
                    {
                        hr = S_OK;
                        CoTaskMemFree(pmtCandidate);
                        break;
                    }
                    CoTaskMemFree(pmtCandidate);
                }
                IEnumMediaTypes_Release(pEnumCandidates);
            }

            /* then try receiver filter's media types */
            if (hr != S_OK && SUCCEEDED(hr = IPin_EnumMediaTypes(pReceivePin, &pEnumCandidates))) /* if we haven't already connected successfully */
            {
                hr = VFW_E_NO_ACCEPTABLE_TYPES; /* Assume the worst, but set to S_OK if connected successfully */

                while (S_OK == IEnumMediaTypes_Next(pEnumCandidates, 1, &pmtCandidate, NULL))
                {
                    if (( !pmt || CompareMediaTypes(pmt, pmtCandidate, TRUE) ) && 
                        (This->pConnectSpecific(iface, pReceivePin, pmtCandidate) == S_OK))
                    {
                        hr = S_OK;
                        CoTaskMemFree(pmtCandidate);
                        break;
                    }
                    CoTaskMemFree(pmtCandidate);
                } /* while */
                IEnumMediaTypes_Release(pEnumCandidates);
            } /* if not found */
        } /* if negotiate media type */
    } /* if succeeded */
    LeaveCriticalSection(This->pin.pCritSec);

    TRACE(" -- %x\n", hr);
    return hr;
}

HRESULT WINAPI OutputPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    ERR("Incoming connection on an output pin! (%p, %p)\n", pReceivePin, pmt);

    return E_UNEXPECTED;
}

HRESULT WINAPI OutputPin_Disconnect(IPin * iface)
{
    HRESULT hr;
    OutputPin *This = (OutputPin *)iface;

    TRACE("()\n");

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (This->pMemInputPin)
        {
            IMemInputPin_Release(This->pMemInputPin);
            This->pMemInputPin = NULL;
        }
        if (This->pin.pConnectedTo)
        {
            IPin_Release(This->pin.pConnectedTo);
            This->pin.pConnectedTo = NULL;
            hr = S_OK;
        }
        else
            hr = S_FALSE;
    }
    LeaveCriticalSection(This->pin.pCritSec);
    
    return hr;
}

HRESULT WINAPI OutputPin_EndOfStream(IPin * iface)
{
    TRACE("()\n");

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

HRESULT WINAPI OutputPin_BeginFlush(IPin * iface)
{
    TRACE("(%p)->()\n", iface);

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

HRESULT WINAPI OutputPin_EndFlush(IPin * iface)
{
    TRACE("(%p)->()\n", iface);

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

HRESULT WINAPI OutputPin_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    TRACE("(%p)->(%x%08x, %x%08x, %e)\n", iface, (ULONG)(tStart >> 32), (ULONG)tStart, (ULONG)(tStop >> 32), (ULONG)tStop, dRate);

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

static const IPinVtbl OutputPin_Vtbl = 
{
    OutputPin_QueryInterface,
    IPinImpl_AddRef,
    OutputPin_Release,
    OutputPin_Connect,
    OutputPin_ReceiveConnection,
    OutputPin_Disconnect,
    IPinImpl_ConnectedTo,
    IPinImpl_ConnectionMediaType,
    IPinImpl_QueryPinInfo,
    IPinImpl_QueryDirection,
    IPinImpl_QueryId,
    IPinImpl_QueryAccept,
    IPinImpl_EnumMediaTypes,
    IPinImpl_QueryInternalConnections,
    OutputPin_EndOfStream,
    OutputPin_BeginFlush,
    OutputPin_EndFlush,
    OutputPin_NewSegment
};

HRESULT OutputPin_GetDeliveryBuffer(OutputPin * This, IMediaSample ** ppSample, REFERENCE_TIME * tStart, REFERENCE_TIME * tStop, DWORD dwFlags)
{
    HRESULT hr;

    TRACE("(%p, %p, %p, %x)\n", ppSample, tStart, tStop, dwFlags);

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo)
            hr = VFW_E_NOT_CONNECTED;
        else
        {
            IMemAllocator * pAlloc = NULL;
            
            hr = IMemInputPin_GetAllocator(This->pMemInputPin, &pAlloc);

            if (SUCCEEDED(hr))
                hr = IMemAllocator_GetBuffer(pAlloc, ppSample, tStart, tStop, dwFlags);

            if (SUCCEEDED(hr))
                hr = IMediaSample_SetTime(*ppSample, tStart, tStop);

            if (pAlloc)
                IMemAllocator_Release(pAlloc);
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);
    
    return hr;
}

HRESULT OutputPin_SendSample(OutputPin * This, IMediaSample * pSample)
{
    HRESULT hr = S_OK;
    IMemInputPin * pMemConnected = NULL;
    PIN_INFO pinInfo;

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo || !This->pMemInputPin)
            hr = VFW_E_NOT_CONNECTED;
        else
        {
            /* we don't have the lock held when using This->pMemInputPin,
             * so we need to AddRef it to stop it being deleted while we are
             * using it. Same with its filter. */
            pMemConnected = This->pMemInputPin;
            IMemInputPin_AddRef(pMemConnected);
            hr = IPin_QueryPinInfo(This->pin.pConnectedTo, &pinInfo);
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);

    if (SUCCEEDED(hr))
    {
        /* NOTE: if we are in a critical section when Receive is called
         * then it causes some problems (most notably with the native Video
         * Renderer) if we are re-entered for whatever reason */
        hr = IMemInputPin_Receive(pMemConnected, pSample);

        /* If the filter's destroyed, tell upstream to stop sending data */
        if(IBaseFilter_Release(pinInfo.pFilter) == 0 && SUCCEEDED(hr))
            hr = S_FALSE;
    }
    if (pMemConnected)
        IMemInputPin_Release(pMemConnected);

    return hr;
}

HRESULT OutputPin_DeliverNewSegment(OutputPin * This, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    HRESULT hr;

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo)
            hr = VFW_E_NOT_CONNECTED;
        else
            hr = IPin_NewSegment(This->pin.pConnectedTo, tStart, tStop, dRate);
    }
    LeaveCriticalSection(This->pin.pCritSec);
    
    return hr;
}

HRESULT OutputPin_CommitAllocator(OutputPin * This)
{
    HRESULT hr;

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo || !This->pMemInputPin)
            hr = VFW_E_NOT_CONNECTED;
        else
        {
            IMemAllocator * pAlloc = NULL;

            hr = IMemInputPin_GetAllocator(This->pMemInputPin, &pAlloc);

            if (SUCCEEDED(hr))
                hr = IMemAllocator_Commit(pAlloc);

            if (pAlloc)
                IMemAllocator_Release(pAlloc);
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);
    
    return hr;
}

HRESULT OutputPin_DeliverDisconnect(OutputPin * This)
{
    HRESULT hr;

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo || !This->pMemInputPin)
            hr = VFW_E_NOT_CONNECTED;
        else
        {
            IMemAllocator * pAlloc = NULL;

            hr = IMemInputPin_GetAllocator(This->pMemInputPin, &pAlloc);

            if (SUCCEEDED(hr))
                hr = IMemAllocator_Decommit(pAlloc);

            if (pAlloc)
                IMemAllocator_Release(pAlloc);

            if (SUCCEEDED(hr))
                hr = IPin_Disconnect(This->pin.pConnectedTo);
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}


HRESULT PullPin_Construct(const PIN_INFO * pPinInfo, SAMPLEPROC pSampleProc, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, IPin ** ppPin)
{
    PullPin * pPinImpl;

    *ppPin = NULL;

    if (pPinInfo->dir != PINDIR_INPUT)
    {
        ERR("Pin direction(%x) != PINDIR_INPUT\n", pPinInfo->dir);
        return E_INVALIDARG;
    }

    pPinImpl = CoTaskMemAlloc(sizeof(*pPinImpl));

    if (!pPinImpl)
        return E_OUTOFMEMORY;

    if (SUCCEEDED(PullPin_Init(pPinInfo, pSampleProc, pUserData, pQueryAccept, pCritSec, pPinImpl)))
    {
        pPinImpl->pin.lpVtbl = &PullPin_Vtbl;
        
        *ppPin = (IPin *)(&pPinImpl->pin.lpVtbl);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT PullPin_Init(const PIN_INFO * pPinInfo, SAMPLEPROC pSampleProc, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, PullPin * pPinImpl)
{
    /* Common attributes */
    pPinImpl->pin.refCount = 1;
    pPinImpl->pin.pConnectedTo = NULL;
    pPinImpl->pin.fnQueryAccept = pQueryAccept;
    pPinImpl->pin.pUserData = pUserData;
    pPinImpl->pin.pCritSec = pCritSec;
    Copy_PinInfo(&pPinImpl->pin.pinInfo, pPinInfo);
    ZeroMemory(&pPinImpl->pin.mtCurrent, sizeof(AM_MEDIA_TYPE));

    /* Input pin attributes */
    pPinImpl->fnSampleProc = pSampleProc;
    pPinImpl->fnPreConnect = NULL;
    pPinImpl->pAlloc = NULL;
    pPinImpl->pReader = NULL;
    pPinImpl->hThread = NULL;
    pPinImpl->hEventStateChanged = CreateEventW(NULL, FALSE, TRUE, NULL);

    pPinImpl->rtStart = 0;
    pPinImpl->rtCurrent = 0;
    pPinImpl->rtStop = ((LONGLONG)0x7fffffff << 32) | 0xffffffff;

    return S_OK;
}

HRESULT WINAPI PullPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    PIN_DIRECTION pindirReceive;
    HRESULT hr = S_OK;
    PullPin *This = (PullPin *)iface;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (This->pin.pConnectedTo)
            hr = VFW_E_ALREADY_CONNECTED;

        if (SUCCEEDED(hr) && (This->pin.fnQueryAccept(This->pin.pUserData, pmt) != S_OK))
            hr = VFW_E_TYPE_NOT_ACCEPTED; /* FIXME: shouldn't we just map common errors onto 
                                           * VFW_E_TYPE_NOT_ACCEPTED and pass the value on otherwise? */

        if (SUCCEEDED(hr))
        {
            IPin_QueryDirection(pReceivePin, &pindirReceive);

            if (pindirReceive != PINDIR_OUTPUT)
            {
                ERR("Can't connect from non-output pin\n");
                hr = VFW_E_INVALID_DIRECTION;
            }
        }

        This->pReader = NULL;
        This->pAlloc = NULL;
        if (SUCCEEDED(hr))
        {
            hr = IPin_QueryInterface(pReceivePin, &IID_IAsyncReader, (LPVOID *)&This->pReader);
        }

        if (SUCCEEDED(hr))
        {
            ALLOCATOR_PROPERTIES props;
            props.cBuffers = 3;
            props.cbBuffer = 64 * 1024; /* 64k bytes */
            props.cbAlign = 1;
            props.cbPrefix = 0;
            hr = IAsyncReader_RequestAllocator(This->pReader, NULL, &props, &This->pAlloc);
        }

        if (SUCCEEDED(hr) && This->fnPreConnect)
        {
            hr = This->fnPreConnect(iface, pReceivePin);
        }

        if (SUCCEEDED(hr))
        {
            CopyMediaType(&This->pin.mtCurrent, pmt);
            This->pin.pConnectedTo = pReceivePin;
            IPin_AddRef(pReceivePin);
        }
        else
        {
             if (This->pReader)
                 IAsyncReader_Release(This->pReader);
             This->pReader = NULL;
             if (This->pAlloc)
                 IMemAllocator_Release(This->pAlloc);
             This->pAlloc = NULL;
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);
    return hr;
}

HRESULT WINAPI PullPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    PullPin *This = (PullPin *)iface;

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)iface;
    else if (IsEqualIID(riid, &IID_IPin))
        *ppv = (LPVOID)iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

ULONG WINAPI PullPin_Release(IPin * iface)
{
    PullPin *This = (PullPin *)iface;
    ULONG refCount = InterlockedDecrement(&This->pin.refCount);

    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (!refCount)
    {
        if(This->pAlloc)
            IMemAllocator_Release(This->pAlloc);
        if(This->pReader)
            IAsyncReader_Release(This->pReader);
        CloseHandle(This->hEventStateChanged);
        CoTaskMemFree(This);
        return 0;
    }
    return refCount;
}

static DWORD WINAPI PullPin_Thread_Main(LPVOID pv)
{
    for (;;)
        SleepEx(INFINITE, TRUE);
}

static void CALLBACK PullPin_Thread_Process(ULONG_PTR iface)
{
    PullPin *This = (PullPin *)iface;
    HRESULT hr;

    ALLOCATOR_PROPERTIES allocProps;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    
    SetEvent(This->hEventStateChanged);

    hr = IMemAllocator_GetProperties(This->pAlloc, &allocProps);

    if (This->rtCurrent < This->rtStart)
        This->rtCurrent = MEDIATIME_FROM_BYTES(ALIGNDOWN(BYTES_FROM_MEDIATIME(This->rtStart), allocProps.cbAlign));

    TRACE("Start\n");

    while (This->rtCurrent < This->rtStop && hr == S_OK)
    {
        /* FIXME: to improve performance by quite a bit this should be changed
         * so that one sample is processed while one sample is fetched. However,
         * it is harder to debug so for the moment it will stay as it is */
        IMediaSample * pSample = NULL;
        REFERENCE_TIME rtSampleStart;
        REFERENCE_TIME rtSampleStop;
        DWORD_PTR dwUser;

        TRACE("Process sample\n");

        hr = IMemAllocator_GetBuffer(This->pAlloc, &pSample, NULL, NULL, 0);

        if (SUCCEEDED(hr))
        {
            rtSampleStart = This->rtCurrent;
            rtSampleStop = rtSampleStart + MEDIATIME_FROM_BYTES(IMediaSample_GetSize(pSample));
            if (rtSampleStop > This->rtStop)
                rtSampleStop = MEDIATIME_FROM_BYTES(ALIGNUP(BYTES_FROM_MEDIATIME(This->rtStop), allocProps.cbAlign));
            hr = IMediaSample_SetTime(pSample, &rtSampleStart, &rtSampleStop);
            This->rtCurrent = rtSampleStop;
        }

        if (SUCCEEDED(hr))
            hr = IAsyncReader_Request(This->pReader, pSample, (ULONG_PTR)0);

        if (SUCCEEDED(hr))
            hr = IAsyncReader_WaitForNext(This->pReader, 10000, &pSample, &dwUser);

        if (SUCCEEDED(hr))
        {
            rtSampleStop = rtSampleStart + MEDIATIME_FROM_BYTES(IMediaSample_GetActualDataLength(pSample));
            if (rtSampleStop > This->rtStop)
                rtSampleStop = MEDIATIME_FROM_BYTES(ALIGNUP(BYTES_FROM_MEDIATIME(This->rtStop), allocProps.cbAlign));
            hr = IMediaSample_SetTime(pSample, &rtSampleStart, &rtSampleStop);
        }

        if (SUCCEEDED(hr))
            hr = This->fnSampleProc(This->pin.pUserData, pSample);
        else
            ERR("Processing error: %x\n", hr);

        if (pSample)
            IMediaSample_Release(pSample);
    }

    CoUninitialize();

    TRACE("End\n");
}

static void CALLBACK PullPin_Thread_Stop(ULONG_PTR iface)
{
    PullPin *This = (PullPin *)iface;

    TRACE("(%p/%p)->()\n", This, (LPVOID)iface);

    EnterCriticalSection(This->pin.pCritSec);
    {
        HRESULT hr;

        CloseHandle(This->hThread);
        This->hThread = NULL;
        if (FAILED(hr = IMemAllocator_Decommit(This->pAlloc)))
            ERR("Allocator decommit failed with error %x. Possible memory leak\n", hr);
    }
    LeaveCriticalSection(This->pin.pCritSec);

    SetEvent(This->hEventStateChanged);

    IBaseFilter_Release(This->pin.pinInfo.pFilter);

    ExitThread(0);
}

HRESULT PullPin_InitProcessing(PullPin * This)
{
    HRESULT hr = S_OK;

    TRACE("(%p)->()\n", This);

    assert(!This->hThread);

    /* if we are connected */
    if (This->pAlloc)
    {
        EnterCriticalSection(This->pin.pCritSec);
        {
            DWORD dwThreadId;
            assert(!This->hThread);
        
            /* AddRef the filter to make sure it and it's pins will be around
             * as long as the thread */
            IBaseFilter_AddRef(This->pin.pinInfo.pFilter);

            This->hThread = CreateThread(NULL, 0, PullPin_Thread_Main, NULL, 0, &dwThreadId);
            if (!This->hThread)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                IBaseFilter_Release(This->pin.pinInfo.pFilter);
            }

            if (SUCCEEDED(hr))
                hr = IMemAllocator_Commit(This->pAlloc);
        }
        LeaveCriticalSection(This->pin.pCritSec);
    }

    TRACE(" -- %x\n", hr);

    return hr;
}

HRESULT PullPin_StartProcessing(PullPin * This)
{
    /* if we are connected */
    TRACE("(%p)->()\n", This);
    if(This->pAlloc)
    {
        assert(This->hThread);
        
        ResetEvent(This->hEventStateChanged);

        if (!QueueUserAPC(PullPin_Thread_Process, This->hThread, (ULONG_PTR)This))
            return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

HRESULT PullPin_PauseProcessing(PullPin * This)
{
    /* make the processing function exit its loop */
    This->rtStop = 0;

    return S_OK;
}

HRESULT PullPin_StopProcessing(PullPin * This)
{
    /* if we are connected */
    if (This->pAlloc)
    {
        assert(This->hThread);

        ResetEvent(This->hEventStateChanged);

        PullPin_PauseProcessing(This);

        if (!QueueUserAPC(PullPin_Thread_Stop, This->hThread, (ULONG_PTR)This))
            return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

HRESULT PullPin_WaitForStateChange(PullPin * This, DWORD dwMilliseconds)
{
    if (WaitForSingleObject(This->hEventStateChanged, dwMilliseconds) == WAIT_TIMEOUT)
        return S_FALSE;
    return S_OK;
}

HRESULT PullPin_Seek(PullPin * This, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
    FIXME("(%p)->(%x%08x, %x%08x)\n", This, (LONG)(rtStart >> 32), (LONG)rtStart, (LONG)(rtStop >> 32), (LONG)rtStop);

    PullPin_BeginFlush((IPin *)This);
    /* FIXME: need critical section? */
    This->rtStart = rtStart;
    This->rtStop = rtStop;
    PullPin_EndFlush((IPin *)This);

    return S_OK;
}

HRESULT WINAPI PullPin_EndOfStream(IPin * iface)
{
    FIXME("(%p)->()\n", iface);
    return E_NOTIMPL;
}

HRESULT WINAPI PullPin_BeginFlush(IPin * iface)
{
    FIXME("(%p)->()\n", iface);
    return E_NOTIMPL;
}

HRESULT WINAPI PullPin_EndFlush(IPin * iface)
{
    FIXME("(%p)->()\n", iface);
    return E_NOTIMPL;
}

HRESULT WINAPI PullPin_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    FIXME("(%p)->(%s, %s, %g)\n", iface, wine_dbgstr_longlong(tStart), wine_dbgstr_longlong(tStop), dRate);
    return E_NOTIMPL;
}

static const IPinVtbl PullPin_Vtbl = 
{
    PullPin_QueryInterface,
    IPinImpl_AddRef,
    PullPin_Release,
    OutputPin_Connect,
    PullPin_ReceiveConnection,
    IPinImpl_Disconnect,
    IPinImpl_ConnectedTo,
    IPinImpl_ConnectionMediaType,
    IPinImpl_QueryPinInfo,
    IPinImpl_QueryDirection,
    IPinImpl_QueryId,
    IPinImpl_QueryAccept,
    IPinImpl_EnumMediaTypes,
    IPinImpl_QueryInternalConnections,
    PullPin_EndOfStream,
    PullPin_BeginFlush,
    PullPin_EndFlush,
    PullPin_NewSegment
};
