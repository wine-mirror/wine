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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"

#include "qcap_main.h"
#include "pin.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "uuids.h"
#include "vfwmsgs.h"
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

#define ALIGNDOWN(value,boundary) ((value) & ~(boundary-1))
#define ALIGNUP(value,boundary) (ALIGNDOWN(value - 1, boundary) + boundary)

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

    if (!This->fnQueryAccept) return S_OK;

    return (This->fnQueryAccept(This->pUserData, pmt) == S_OK ? S_OK : S_FALSE);
}

HRESULT WINAPI IPinImpl_QueryInternalConnections(IPin * iface, IPin ** apPin, ULONG * cPin)
{
    IPinImpl *This = (IPinImpl *)iface;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, apPin, cPin);

    return E_NOTIMPL; /* to tell caller that all input pins connected to all output pins */
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
        hr = IPin_QueryInterface(pReceivePin, &IID_IMemInputPin, (LPVOID)&This->pMemInputPin);

        if (SUCCEEDED(hr))
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

    TRACE(" -- %x\n", hr);
    return hr;
}

HRESULT OutputPin_Init(const PIN_INFO * pPinInfo, const ALLOCATOR_PROPERTIES * props,
                       LPVOID pUserData, QUERYACCEPTPROC pQueryAccept,
                       LPCRITICAL_SECTION pCritSec, OutputPin * pPinImpl)
{
    TRACE("\n");

    /* Common attributes */
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
                        TRACE("o_o\n");
                        DeleteMediaType(pmtCandidate);
                        break;
                    }
                    DeleteMediaType(pmtCandidate);
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
                        DeleteMediaType(pmtCandidate);
                        break;
                    }
                    DeleteMediaType(pmtCandidate);
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

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo || !This->pMemInputPin)
            hr = VFW_E_NOT_CONNECTED;
        else
        {
            /* we don't have the lock held when using This->pMemInputPin,
             * so we need to AddRef it to stop it being deleted while we are
             * using it. */
            pMemConnected = This->pMemInputPin;
            IMemInputPin_AddRef(pMemConnected);
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);

    if (SUCCEEDED(hr))
    {
        /* NOTE: if we are in a critical section when Receive is called
         * then it causes some problems (most notably with the native Video
         * Renderer) if we are re-entered for whatever reason */
        hr = IMemInputPin_Receive(pMemConnected, pSample);
        IMemInputPin_Release(pMemConnected);
    }

    return hr;
}
