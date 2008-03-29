/*
 * Implementation of IEnumPins Interface
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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

typedef struct IEnumPinsImpl
{
    const IEnumPinsVtbl * lpVtbl;
    LONG refCount;
    ENUMPINDETAILS enumPinDetails;
    ULONG uIndex;
} IEnumPinsImpl;

static const struct IEnumPinsVtbl IEnumPinsImpl_Vtbl;

HRESULT IEnumPinsImpl_Construct(const ENUMPINDETAILS * pDetails, IEnumPins ** ppEnum)
{
    IEnumPinsImpl * pEnumPins = CoTaskMemAlloc(sizeof(IEnumPinsImpl));
    if (!pEnumPins)
    {
        *ppEnum = NULL;
        return E_OUTOFMEMORY;
    }
    pEnumPins->lpVtbl = &IEnumPinsImpl_Vtbl;
    pEnumPins->refCount = 1;
    pEnumPins->uIndex = 0;
    pEnumPins->enumPinDetails = *pDetails;
    *ppEnum = (IEnumPins *)(&pEnumPins->lpVtbl);
    ObjectRefCount(TRUE);
    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_QueryInterface(IEnumPins * iface, REFIID riid, LPVOID * ppv)
{
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)iface;
    else if (IsEqualIID(riid, &IID_IEnumPins))
        *ppv = (LPVOID)iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI IEnumPinsImpl_AddRef(IEnumPins * iface)
{
    IEnumPinsImpl *This = (IEnumPinsImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("()\n");

    return refCount;
}

static ULONG WINAPI IEnumPinsImpl_Release(IEnumPins * iface)
{
    IEnumPinsImpl *This = (IEnumPinsImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("()\n");
    
    if (!refCount)
    {
        CoTaskMemFree(This);
        ObjectRefCount(FALSE);
    }
    return refCount;
}

static HRESULT WINAPI IEnumPinsImpl_Next(IEnumPins * iface, ULONG cPins, IPin ** ppPins, ULONG * pcFetched)
{
    ULONG cFetched; 
    IEnumPinsImpl *This = (IEnumPinsImpl *)iface;

    cFetched = min(This->enumPinDetails.cPins, This->uIndex + cPins) - This->uIndex;

    TRACE("(%u, %p, %p)\n", cPins, ppPins, pcFetched);

    if (cFetched > 0)
    {
        ULONG i;
        for (i = 0; i < cFetched; i++) {
            IPin_AddRef(This->enumPinDetails.ppPins[This->uIndex + i]);
            ppPins[i] = This->enumPinDetails.ppPins[This->uIndex + i];
        }
    }

    if ((cPins != 1) || pcFetched)
        *pcFetched = cFetched;

    This->uIndex += cFetched;

    if (cFetched != cPins)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_Skip(IEnumPins * iface, ULONG cPins)
{
    IEnumPinsImpl *This = (IEnumPinsImpl *)iface;

    TRACE("(%u)\n", cPins);

    if (This->uIndex + cPins < This->enumPinDetails.cPins)
    {
        This->uIndex += cPins;
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI IEnumPinsImpl_Reset(IEnumPins * iface)
{
    IEnumPinsImpl *This = (IEnumPinsImpl *)iface;

    TRACE("IEnumPinsImpl::Reset()\n");

    This->uIndex = 0;
    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_Clone(IEnumPins * iface, IEnumPins ** ppEnum)
{
    HRESULT hr;
    IEnumPinsImpl *This = (IEnumPinsImpl *)iface;

    TRACE("(%p)\n", ppEnum);

    hr = IEnumPinsImpl_Construct(&This->enumPinDetails, ppEnum);
    if (FAILED(hr))
        return hr;
    return IEnumPins_Skip(*ppEnum, This->uIndex);
}

static const IEnumPinsVtbl IEnumPinsImpl_Vtbl =
{
    IEnumPinsImpl_QueryInterface,
    IEnumPinsImpl_AddRef,
    IEnumPinsImpl_Release,
    IEnumPinsImpl_Next,
    IEnumPinsImpl_Skip,
    IEnumPinsImpl_Reset,
    IEnumPinsImpl_Clone
};
