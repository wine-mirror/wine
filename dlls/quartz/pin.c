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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "quartz_private.h"
#include "pin.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "uuids.h"
#include "vfwmsgs.h"
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const struct IPinVtbl InputPin_Vtbl;
static const struct IPinVtbl OutputPin_Vtbl;
static const struct IMemInputPinVtbl MemInputPin_Vtbl;
static const struct IPinVtbl PullPin_Vtbl;

#define ALIGNDOWN(value,boundary) ((value) & ~(boundary-1))
#define ALIGNUP(value,boundary) (ALIGNDOWN(value - 1, boundary) + boundary)

#define _IMemInputPin_Offset ((int)(&(((InputPin*)0)->lpVtblMemInput)))
#define ICOM_THIS_From_IMemInputPin(impl, iface) impl* This = (impl*)(((char*)iface)-_IMemInputPin_Offset);

static void Copy_PinInfo(PIN_INFO * pDest, const PIN_INFO * pSrc)
{
    /* Tempting to just do a memcpy, but the name field is
       128 characters long! We will probably never exceed 10
       most of the time, so we are better off copying 
       each field manually */
    strcpyW(pDest->achName, pSrc->achName);
    pDest->dir = pSrc->dir;
    pDest->pFilter = pSrc->pFilter;
    IBaseFilter_AddRef(pDest->pFilter);
}

/* Internal function called as a helper to IPin_Connect */
/* specific AM_MEDIA_TYPE - it cannot be NULL */
/* NOTE: not part of standard interface */
static HRESULT OutputPin_ConnectSpecific(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    ICOM_THIS(OutputPin, iface);
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
        hr = IPin_QueryInterface(pReceivePin, &IID_IMemInputPin, (LPVOID)&This->pMemInputPin);

        if (SUCCEEDED(hr))
            hr = IMemInputPin_GetAllocator(This->pMemInputPin, &pMemAlloc);

        if (SUCCEEDED(hr))
            hr = IMemAllocator_SetProperties(pMemAlloc, &This->allocProps, &actual);

        if (pMemAlloc)
            IMemAllocator_Release(pMemAlloc);

        /* break connection if we couldn't get the allocator */
        if (FAILED(hr))
            IPin_Disconnect(pReceivePin);
    }

    if (FAILED(hr))
    {
        IPin_Release(This->pin.pConnectedTo);
        This->pin.pConnectedTo = NULL;
        DeleteMediaType(&This->pin.mtCurrent);
    }

    TRACE(" -- %lx\n", hr);
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
    /* Common attributes */
    pPinImpl->pin.refCount = 1;
    pPinImpl->pin.pConnectedTo = NULL;
    pPinImpl->pin.fnQueryAccept = pQueryAccept;
    pPinImpl->pin.pUserData = pUserData;
    pPinImpl->pin.pCritSec = pCritSec;
    Copy_PinInfo(&pPinImpl->pin.pinInfo, pPinInfo);

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
    /* Common attributes */
    pPinImpl->pin.lpVtbl = &OutputPin_Vtbl;
    pPinImpl->pin.refCount = 1;
    pPinImpl->pin.pConnectedTo = NULL;
    pPinImpl->pin.fnQueryAccept = pQueryAccept;
    pPinImpl->pin.pUserData = pUserData;
    pPinImpl->pin.pCritSec = pCritSec;
    Copy_PinInfo(&pPinImpl->pin.pinInfo, pPinInfo);

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
    ICOM_THIS(IPinImpl, iface);
    
    TRACE("()\n");
    
    return InterlockedIncrement(&This->refCount);
}

HRESULT WINAPI IPinImpl_Disconnect(IPin * iface)
{
    HRESULT hr;
    ICOM_THIS(IPinImpl, iface);

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
    ICOM_THIS(IPinImpl, iface);

/*  TRACE("(%p)\n", ppPin);*/

    *ppPin = This->pConnectedTo;

    if (*ppPin)
    {
        IPin_AddRef(*ppPin);
        return S_OK;
    }
    else
        return VFW_E_NOT_CONNECTED;
}

HRESULT WINAPI IPinImpl_ConnectionMediaType(IPin * iface, AM_MEDIA_TYPE * pmt)
{
    HRESULT hr;
    ICOM_THIS(IPinImpl, iface);

    TRACE("(%p)\n", pmt);

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
    ICOM_THIS(IPinImpl, iface);

    TRACE("(%p)\n", pInfo);

    Copy_PinInfo(pInfo, &This->pinInfo);

    return S_OK;
}

HRESULT WINAPI IPinImpl_QueryDirection(IPin * iface, PIN_DIRECTION * pPinDir)
{
    ICOM_THIS(IPinImpl, iface);

    TRACE("(%p)\n", pPinDir);

    *pPinDir = This->pinInfo.dir;

    return S_OK;
}

HRESULT WINAPI IPinImpl_QueryId(IPin * iface, LPWSTR * Id)
{
    ICOM_THIS(IPinImpl, iface);

    TRACE("(%p)\n", Id);

    *Id = CoTaskMemAlloc((strlenW(This->pinInfo.achName) + 1) * sizeof(WCHAR));
    if (!Id)
        return E_OUTOFMEMORY;

    strcpyW(*Id, This->pinInfo.achName);

    return S_OK;
}

HRESULT WINAPI IPinImpl_QueryAccept(IPin * iface, const AM_MEDIA_TYPE * pmt)
{
    ICOM_THIS(IPinImpl, iface);

    TRACE("(%p)\n", pmt);

    return (This->fnQueryAccept(This->pUserData, pmt) == S_OK ? S_OK : S_FALSE);
}

HRESULT WINAPI IPinImpl_EnumMediaTypes(IPin * iface, IEnumMediaTypes ** ppEnum)
{
    ENUMMEDIADETAILS emd;

    TRACE("(%p)\n", ppEnum);

    /* override this method to allow enumeration of your types */
    emd.cMediaTypes = 0;
    emd.pMediaTypes = NULL;

    return IEnumMediaTypesImpl_Construct(&emd, ppEnum);
}

HRESULT WINAPI IPinImpl_QueryInternalConnections(IPin * iface, IPin ** apPin, ULONG * cPin)
{
    TRACE("(%p, %p)\n", apPin, cPin);

    return E_NOTIMPL; /* to tell caller that all input pins connected to all output pins */
}

/*** IPin implementation for an input pin ***/

HRESULT WINAPI InputPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    ICOM_THIS(InputPin, iface);

    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

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
    ICOM_THIS(InputPin, iface);
    
    TRACE("()\n");
    
    if (!InterlockedDecrement(&This->pin.refCount))
    {
        DeleteMediaType(&This->pin.mtCurrent);
        if (This->pAllocator)
            IMemAllocator_Release(This->pAllocator);
        CoTaskMemFree(This);
        return 0;
    }
    else
        return This->pin.refCount;
}

HRESULT WINAPI InputPin_Connect(IPin * iface, IPin * pConnector, const AM_MEDIA_TYPE * pmt)
{
    ERR("Outgoing connection on an input pin! (%p, %p)\n", pConnector, pmt);

    return E_UNEXPECTED;
}


HRESULT WINAPI InputPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    ICOM_THIS(InputPin, iface);
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
    ICOM_THIS(InputPin, iface);

    TRACE("(%lx%08lx, %lx%08lx, %e)\n", (ULONG)(tStart >> 32), (ULONG)tStart, (ULONG)(tStop >> 32), (ULONG)tStop, dRate);

    This->tStart = tStart;
    This->tStop = tStop;
    This->dRate = dRate;

    return S_OK;
}

static const IPinVtbl InputPin_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
    ICOM_THIS_From_IMemInputPin(InputPin, iface);

    return IPin_QueryInterface((IPin *)&This->pin, riid, ppv);
}

ULONG WINAPI MemInputPin_AddRef(IMemInputPin * iface)
{
    ICOM_THIS_From_IMemInputPin(InputPin, iface);

    return IPin_AddRef((IPin *)&This->pin);
}

ULONG WINAPI MemInputPin_Release(IMemInputPin * iface)
{
    ICOM_THIS_From_IMemInputPin(InputPin, iface);

    return IPin_Release((IPin *)&This->pin);
}

HRESULT WINAPI MemInputPin_GetAllocator(IMemInputPin * iface, IMemAllocator ** ppAllocator)
{
    ICOM_THIS_From_IMemInputPin(InputPin, iface);

    TRACE("MemInputPin_GetAllocator()\n");

    *ppAllocator = This->pAllocator;
    if (*ppAllocator)
        IMemAllocator_AddRef(*ppAllocator);
    
    return *ppAllocator ? S_OK : VFW_E_NO_ALLOCATOR;
}

HRESULT WINAPI MemInputPin_NotifyAllocator(IMemInputPin * iface, IMemAllocator * pAllocator, BOOL bReadOnly)
{
    ICOM_THIS_From_IMemInputPin(InputPin, iface);

    TRACE("()\n");

    if (This->pAllocator)
        IMemAllocator_Release(This->pAllocator);
    This->pAllocator = pAllocator;
    if (This->pAllocator)
        IMemAllocator_AddRef(This->pAllocator);

    return S_OK;
}

HRESULT WINAPI MemInputPin_GetAllocatorRequirements(IMemInputPin * iface, ALLOCATOR_PROPERTIES * pProps)
{
    TRACE("(%p)\n", pProps);

    /* override this method if you have any specific requirements */

    return E_NOTIMPL;
}

HRESULT WINAPI MemInputPin_Receive(IMemInputPin * iface, IMediaSample * pSample)
{
    ICOM_THIS_From_IMemInputPin(InputPin, iface);

    /* this trace commented out for performance reasons */
/*  TRACE("(%p)\n", pSample);*/

    return This->fnSampleProc(This->pin.pUserData, pSample);
}

HRESULT WINAPI MemInputPin_ReceiveMultiple(IMemInputPin * iface, IMediaSample ** pSamples, long nSamples, long *nSamplesProcessed)
{
    HRESULT hr = S_OK;
    TRACE("(%p, %ld, %p)\n", pSamples, nSamples, nSamplesProcessed);

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
    FIXME("()\n");

    /* FIXME: we should check whether any output pins will block */

    return S_OK;
}

static const IMemInputPinVtbl MemInputPin_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

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
    ICOM_THIS(OutputPin, iface);
    
    TRACE("()\n");
    
    if (!InterlockedDecrement(&This->pin.refCount))
    {
        DeleteMediaType(&This->pin.mtCurrent);
        CoTaskMemFree(This);
        return 0;
    }
    return This->pin.refCount;
}

HRESULT WINAPI OutputPin_Connect(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    HRESULT hr;
    ICOM_THIS(OutputPin, iface);

    TRACE("(%p, %p)\n", pReceivePin, pmt);
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

    FIXME(" -- %lx\n", hr);
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
    ICOM_THIS(OutputPin, iface);

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
    TRACE("()\n");

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

HRESULT WINAPI OutputPin_EndFlush(IPin * iface)
{
    TRACE("()\n");

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

HRESULT WINAPI OutputPin_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    TRACE("(%lx%08lx, %lx%08lx, %e)\n", (ULONG)(tStart >> 32), (ULONG)tStart, (ULONG)(tStop >> 32), (ULONG)tStop, dRate);

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

static const IPinVtbl OutputPin_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

HRESULT OutputPin_GetDeliveryBuffer(OutputPin * This, IMediaSample ** ppSample, const REFERENCE_TIME * tStart, const REFERENCE_TIME * tStop, DWORD dwFlags)
{
    HRESULT hr;

    TRACE("(%p, %p, %p, %lx)\n", ppSample, tStart, tStop, dwFlags);

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo)
            hr = VFW_E_NOT_CONNECTED;
        else
        {
            IMemInputPin * pMemInputPin = NULL;
            IMemAllocator * pAlloc = NULL;
            
            /* FIXME: should we cache this? */
            hr = IPin_QueryInterface(This->pin.pConnectedTo, &IID_IMemInputPin, (LPVOID *)&pMemInputPin);

            if (SUCCEEDED(hr))
                hr = IMemInputPin_GetAllocator(pMemInputPin, &pAlloc);

            if (SUCCEEDED(hr))
                hr = IMemAllocator_GetBuffer(pAlloc, ppSample, (REFERENCE_TIME *)tStart, (REFERENCE_TIME *)tStop, dwFlags);

            if (SUCCEEDED(hr))
                hr = IMediaSample_SetTime(*ppSample, (REFERENCE_TIME *)tStart, (REFERENCE_TIME *)tStop);

            if (pAlloc)
                IMemAllocator_Release(pAlloc);
            if (pMemInputPin)
                IMemInputPin_Release(pMemInputPin);
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);
    
    return hr;
}

HRESULT OutputPin_SendSample(OutputPin * This, IMediaSample * pSample)
{
    HRESULT hr;
    IMemInputPin * pMemConnected = NULL;

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo)
            hr = VFW_E_NOT_CONNECTED;
        else
        {
            /* FIXME: should we cache this? - if we do, then we need to keep the local version
             * and AddRef here instead */
            hr = IPin_QueryInterface(This->pin.pConnectedTo, &IID_IMemInputPin, (LPVOID *)&pMemConnected);
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);

    if (SUCCEEDED(hr) && pMemConnected)
    {
        /* NOTE: if we are in a critical section when Receive is called
         * then it causes some problems (most notably with the native Video
         * Renderer) if we are re-entered for whatever reason */
        hr = IMemInputPin_Receive(pMemConnected, pSample);
        IMemInputPin_Release(pMemConnected);
    }
    
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

    TRACE("()\n");

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo)
            hr = VFW_E_NOT_CONNECTED;
        else
        {
            IMemInputPin * pMemInputPin = NULL;
            IMemAllocator * pAlloc = NULL;
            /* FIXME: should we cache this? */
            hr = IPin_QueryInterface(This->pin.pConnectedTo, &IID_IMemInputPin, (LPVOID *)&pMemInputPin);

            if (SUCCEEDED(hr))
                hr = IMemInputPin_GetAllocator(pMemInputPin, &pAlloc);

            if (SUCCEEDED(hr))
                hr = IMemAllocator_Commit(pAlloc);

            if (pAlloc)
                IMemAllocator_Release(pAlloc);

            if (pMemInputPin)
                IMemInputPin_Release(pMemInputPin);
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);
    
    return hr;
}

HRESULT OutputPin_DeliverDisconnect(OutputPin * This)
{
    HRESULT hr;

    TRACE("()\n");

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo)
            hr = VFW_E_NOT_CONNECTED;
        else
        {
            IMemInputPin * pMemInputPin = NULL;
            IMemAllocator * pAlloc = NULL;
            /* FIXME: should we cache this? */
            hr = IPin_QueryInterface(This->pin.pConnectedTo, &IID_IMemInputPin, (LPVOID *)&pMemInputPin);

            if (SUCCEEDED(hr))
                hr = IMemInputPin_GetAllocator(pMemInputPin, &pAlloc);

            if (SUCCEEDED(hr))
                hr = IMemAllocator_Decommit(pAlloc);

            if (pAlloc)
                IMemAllocator_Release(pAlloc);

            if (pMemInputPin)
                IMemInputPin_Release(pMemInputPin);

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

    /* Input pin attributes */
    pPinImpl->fnSampleProc = pSampleProc;
    pPinImpl->fnPreConnect = NULL;
    pPinImpl->pAlloc = NULL;
    pPinImpl->pReader = NULL;
    pPinImpl->hThread = NULL;
    pPinImpl->hEventStateChanged = CreateEventW(NULL, FALSE, TRUE, NULL);

    pPinImpl->rtStart = 0;
    pPinImpl->rtStop = ((LONGLONG)0x7fffffff << 32) | 0xffffffff;

    return S_OK;
}

HRESULT WINAPI PullPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    PIN_DIRECTION pindirReceive;
    HRESULT hr = S_OK;
    ICOM_THIS(PullPin, iface);

    TRACE("(%p, %p)\n", pReceivePin, pmt);
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
    }
    LeaveCriticalSection(This->pin.pCritSec);
    return hr;
}

HRESULT WINAPI PullPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

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
    ICOM_THIS(PullPin, iface);

    TRACE("()\n");

    if (!InterlockedDecrement(&This->pin.refCount))
    {
        if (This->hThread)
            PullPin_StopProcessing(This);
        IMemAllocator_Release(This->pAlloc);
        IAsyncReader_Release(This->pReader);
        CloseHandle(This->hEventStateChanged);
        CoTaskMemFree(This);
        return 0;
    }
    return This->pin.refCount;
}

static DWORD WINAPI PullPin_Thread_Main(LPVOID pv)
{
    for (;;)
        SleepEx(INFINITE, TRUE);
}

static void CALLBACK PullPin_Thread_Process(ULONG_PTR iface)
{
    ICOM_THIS(PullPin, iface);
    HRESULT hr;

    REFERENCE_TIME rtCurrent;
    ALLOCATOR_PROPERTIES allocProps;

    SetEvent(This->hEventStateChanged);

    hr = IMemAllocator_GetProperties(This->pAlloc, &allocProps);

    rtCurrent = MEDIATIME_FROM_BYTES(ALIGNDOWN(BYTES_FROM_MEDIATIME(This->rtStart), allocProps.cbAlign));

    while (rtCurrent < This->rtStop)
    {
        /* FIXME: to improve performance by quite a bit this should be changed
         * so that one sample is processed while one sample is fetched. However,
         * it is harder to debug so for the moment it will stay as it is */
        IMediaSample * pSample = NULL;
        REFERENCE_TIME rtSampleStop;
        DWORD dwUser;

        hr = IMemAllocator_GetBuffer(This->pAlloc, &pSample, NULL, NULL, 0);

        if (SUCCEEDED(hr))
        {
            rtSampleStop = rtCurrent + MEDIATIME_FROM_BYTES(IMediaSample_GetSize(pSample));
            if (rtSampleStop > This->rtStop)
                rtSampleStop = MEDIATIME_FROM_BYTES(ALIGNUP(BYTES_FROM_MEDIATIME(This->rtStop), allocProps.cbAlign));
            hr = IMediaSample_SetTime(pSample, &rtCurrent, &rtSampleStop);
            rtCurrent = rtSampleStop;
        }

        if (SUCCEEDED(hr))
            hr = IAsyncReader_Request(This->pReader, pSample, (ULONG_PTR)0);

        if (SUCCEEDED(hr))
            hr = IAsyncReader_WaitForNext(This->pReader, 10000, &pSample, &dwUser);

        if (SUCCEEDED(hr))
            hr = This->fnSampleProc(This->pin.pUserData, pSample);
        else
            ERR("Processing error: %lx\n", hr);
        
        if (pSample)
            IMemAllocator_ReleaseBuffer(This->pAlloc, pSample);
    }
}

static void CALLBACK PullPin_Thread_Stop(ULONG_PTR iface)
{
    ICOM_THIS(PullPin, iface);

    TRACE("()\n");

    EnterCriticalSection(This->pin.pCritSec);
    {
        HRESULT hr;

        CloseHandle(This->hThread);
        This->hThread = NULL;
        if (FAILED(hr = IMemAllocator_Decommit(This->pAlloc)))
            ERR("Allocator decommit failed with error %lx. Possible memory leak\n", hr);
    }
    LeaveCriticalSection(This->pin.pCritSec);

    SetEvent(This->hEventStateChanged);

    ExitThread(0);
}

HRESULT PullPin_InitProcessing(PullPin * This)
{
    HRESULT hr = S_OK;

    TRACE("()\n");

    assert(!This->hThread);

    /* if we are connected */
    if (This->pAlloc)
    {
        EnterCriticalSection(This->pin.pCritSec);
        {
            DWORD dwThreadId;
            assert(!This->hThread);
        
            This->hThread = CreateThread(NULL, 0, PullPin_Thread_Main, NULL, 0, &dwThreadId);
            if (!This->hThread)
                hr = HRESULT_FROM_WIN32(GetLastError());

            if (SUCCEEDED(hr))
                hr = IMemAllocator_Commit(This->pAlloc);
        }
        LeaveCriticalSection(This->pin.pCritSec);
    }

    TRACE(" -- %lx\n", hr);

    return hr;
}

HRESULT PullPin_StartProcessing(PullPin * This)
{
    /* if we are connected */
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
    FIXME("(%lx%08lx, %lx%08lx)\n", (LONG)(rtStart >> 32), (LONG)rtStart, (LONG)(rtStop >> 32), (LONG)rtStop);

    PullPin_BeginFlush((IPin *)This);
    /* FIXME: need critical section? */
    This->rtStart = rtStart;
    This->rtStop = rtStop;
    PullPin_EndFlush((IPin *)This);

    return S_OK;
}

HRESULT WINAPI PullPin_EndOfStream(IPin * iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

HRESULT WINAPI PullPin_BeginFlush(IPin * iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

HRESULT WINAPI PullPin_EndFlush(IPin * iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

HRESULT WINAPI PullPin_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

static const IPinVtbl PullPin_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
