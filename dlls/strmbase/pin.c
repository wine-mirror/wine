/*
 * Generic Implementation of IPin Interface
 *
 * Copyright 2003 Robert Shearman
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

#define COBJMACROS

#include "dshow.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/strmbase.h"
#include "uuids.h"
#include "vfwmsgs.h"
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

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

/*** Common Base Pin function */
HRESULT WINAPI BasePinImpl_GetMediaType(IPin *iface, int iPosition, AM_MEDIA_TYPE *pmt)
{
    if (iPosition < 0)
        return E_INVALIDARG;
    return VFW_S_NO_MORE_ITEMS;
}

LONG WINAPI BasePinImpl_GetMediaTypeVersion(IPin *iface)
{
    return 1;
}

ULONG WINAPI BasePinImpl_AddRef(IPin * iface)
{
    BasePin *This = (BasePin *)iface;
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p)->() AddRef from %d\n", iface, refCount - 1);

    return refCount;
}

HRESULT WINAPI BasePinImpl_Disconnect(IPin * iface)
{
    HRESULT hr;
    BasePin *This = (BasePin *)iface;

    TRACE("()\n");

    EnterCriticalSection(This->pCritSec);
    {
        if (This->pConnectedTo)
        {
            IPin_Release(This->pConnectedTo);
            This->pConnectedTo = NULL;
            FreeMediaType(&This->mtCurrent);
            ZeroMemory(&This->mtCurrent, sizeof(This->mtCurrent));
            hr = S_OK;
        }
        else
            hr = S_FALSE;
    }
    LeaveCriticalSection(This->pCritSec);

    return hr;
}

HRESULT WINAPI BasePinImpl_ConnectedTo(IPin * iface, IPin ** ppPin)
{
    HRESULT hr;
    BasePin *This = (BasePin *)iface;

    TRACE("(%p)\n", ppPin);

    EnterCriticalSection(This->pCritSec);
    {
        if (This->pConnectedTo)
        {
            *ppPin = This->pConnectedTo;
            IPin_AddRef(*ppPin);
            hr = S_OK;
        }
        else
        {
            hr = VFW_E_NOT_CONNECTED;
            *ppPin = NULL;
        }
    }
    LeaveCriticalSection(This->pCritSec);

    return hr;
}

HRESULT WINAPI BasePinImpl_ConnectionMediaType(IPin * iface, AM_MEDIA_TYPE * pmt)
{
    HRESULT hr;
    BasePin *This = (BasePin *)iface;

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

HRESULT WINAPI BasePinImpl_QueryPinInfo(IPin * iface, PIN_INFO * pInfo)
{
    BasePin *This = (BasePin *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pInfo);

    Copy_PinInfo(pInfo, &This->pinInfo);
    IBaseFilter_AddRef(pInfo->pFilter);

    return S_OK;
}

HRESULT WINAPI BasePinImpl_QueryDirection(IPin * iface, PIN_DIRECTION * pPinDir)
{
    BasePin *This = (BasePin *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pPinDir);

    *pPinDir = This->pinInfo.dir;

    return S_OK;
}

HRESULT WINAPI BasePinImpl_QueryId(IPin * iface, LPWSTR * Id)
{
    BasePin *This = (BasePin *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, Id);

    *Id = CoTaskMemAlloc((strlenW(This->pinInfo.achName) + 1) * sizeof(WCHAR));
    if (!*Id)
        return E_OUTOFMEMORY;

    strcpyW(*Id, This->pinInfo.achName);

    return S_OK;
}

HRESULT WINAPI BasePinImpl_QueryAccept(IPin * iface, const AM_MEDIA_TYPE * pmt)
{
    TRACE("(%p)->(%p)\n", iface, pmt);

    return S_OK;
}

HRESULT WINAPI BasePinImpl_EnumMediaTypes(IPin * iface, IEnumMediaTypes ** ppEnum)
{
    BasePin *This = (BasePin *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    /* override this method to allow enumeration of your types */

    return EnumMediaTypes_Construct(iface, BasePinImpl_GetMediaType, BasePinImpl_GetMediaTypeVersion , ppEnum);
}

HRESULT WINAPI BasePinImpl_QueryInternalConnections(IPin * iface, IPin ** apPin, ULONG * cPin)
{
    BasePin *This = (BasePin *)iface;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, apPin, cPin);

    return E_NOTIMPL; /* to tell caller that all input pins connected to all output pins */
}
