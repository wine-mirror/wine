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
    HRESULT hr;
    ICOM_THIS(OutputPin, iface);

    TRACE("(%p, %p)\n", pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    hr = IPin_ReceiveConnection(pReceivePin, iface, pmt);

    if (SUCCEEDED(hr))
    {
        This->pin.pConnectedTo = pReceivePin;
        IPin_AddRef(pReceivePin);
        CopyMediaType(&This->pin.mtCurrent, pmt);
    }
    else
        This->pin.pConnectedTo = NULL;

    TRACE("-- %lx\n", hr);
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
    pPinImpl->pin.pQueryAccept = pQueryAccept;
    pPinImpl->pin.pUserData = pUserData;
    pPinImpl->pin.pCritSec = pCritSec;
    InitializeCriticalSection(pPinImpl->pin.pCritSec);
    Copy_PinInfo(&pPinImpl->pin.pinInfo, pPinInfo);

    /* Input pin attributes */
    pPinImpl->pSampleProc = pSampleProc;
    pPinImpl->pAllocator = NULL;
    pPinImpl->tStart = 0;
    pPinImpl->tStop = 0;
    pPinImpl->dRate = 0;

    return S_OK;
}

HRESULT OutputPin_Init(const PIN_INFO * pPinInfo, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, OutputPin * pPinImpl)
{
    /* Common attributes */
    pPinImpl->pin.lpVtbl = &OutputPin_Vtbl;
    pPinImpl->pin.refCount = 1;
    pPinImpl->pin.pConnectedTo = NULL;
    pPinImpl->pin.pQueryAccept = pQueryAccept;
    pPinImpl->pin.pUserData = pUserData;
    pPinImpl->pin.pCritSec = pCritSec;
    InitializeCriticalSection(pPinImpl->pin.pCritSec);
    Copy_PinInfo(&pPinImpl->pin.pinInfo, pPinInfo);

    /* Output pin attributes */
    pPinImpl->pMemInputPin = NULL;
    pPinImpl->pConnectSpecific = OutputPin_ConnectSpecific;

    return S_OK;
}

HRESULT OutputPin_Construct(const PIN_INFO * pPinInfo, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, IPin ** ppPin)
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

    if (SUCCEEDED(OutputPin_Init(pPinInfo, pUserData, pQueryAccept, pCritSec, pPinImpl)))
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
    HRESULT hr = S_OK;
    ICOM_THIS(IPinImpl, iface);

/*  TRACE("(%p)\n", ppPin);*/

    EnterCriticalSection(This->pCritSec);
    {
        if (!This->pConnectedTo)
        {
            *ppPin = NULL;
            hr = VFW_E_NOT_CONNECTED;
        }
        else
        {
            *ppPin = This->pConnectedTo;
            IPin_AddRef(*ppPin);
        }
    }
    LeaveCriticalSection(This->pCritSec);

    return hr;
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

    return (This->pQueryAccept(This->pUserData, pmt) == S_OK ? S_OK : S_FALSE);
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
    PIN_DIRECTION pindirReceive;
    ICOM_THIS(InputPin, iface);
    HRESULT hr = S_OK;

    TRACE("(%p, %p)\n", pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (This->pin.pConnectedTo)
            hr = VFW_E_ALREADY_CONNECTED;

        if (SUCCEEDED(hr) && This->pin.pQueryAccept(This->pin.pUserData, pmt) != S_OK)
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

    TRACE("MemInputPin_NotifyAllocator()\n");

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

    return This->pSampleProc(This->pin.pUserData, pSample);
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
        /* get the IMemInputPin interface we will use to deliver samples to the
         * connected pin */
        hr = IPin_QueryInterface(pReceivePin, &IID_IMemInputPin, (LPVOID *)&This->pMemInputPin);

        /* if we have been a specific type to connect with, then we can either connect
         * with that or fail. We cannot choose different AM_MEDIA_TYPE */
        if (SUCCEEDED(hr))
        {
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
    } /* if succeeded */
    LeaveCriticalSection(This->pin.pCritSec);

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
