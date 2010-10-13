/*
 * Implementation of MedaType utility functions
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
#include <stdarg.h>

#define COBJMACROS
#include "dshow.h"

#include "wine/strmbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

HRESULT WINAPI CopyMediaType(AM_MEDIA_TYPE * pDest, const AM_MEDIA_TYPE *pSrc)
{
    *pDest = *pSrc;
    if (!pSrc->pbFormat) return S_OK;
    if (!(pDest->pbFormat = CoTaskMemAlloc(pSrc->cbFormat)))
        return E_OUTOFMEMORY;
    memcpy(pDest->pbFormat, pSrc->pbFormat, pSrc->cbFormat);
    if (pDest->pUnk)
        IUnknown_AddRef(pDest->pUnk);
    return S_OK;
}

void WINAPI FreeMediaType(AM_MEDIA_TYPE * pMediaType)
{
    if (pMediaType->pbFormat)
    {
        CoTaskMemFree(pMediaType->pbFormat);
        pMediaType->pbFormat = NULL;
    }
    if (pMediaType->pUnk)
    {
        IUnknown_Release(pMediaType->pUnk);
        pMediaType->pUnk = NULL;
    }
}

AM_MEDIA_TYPE * WINAPI CreateMediaType(AM_MEDIA_TYPE const * pSrc)
{
    AM_MEDIA_TYPE * pDest;

    pDest = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (!pDest)
        return NULL;

    if (FAILED(CopyMediaType(pDest, pSrc)))
    {
        CoTaskMemFree(pDest);
        return NULL;
    }

    return pDest;
}

void WINAPI DeleteMediaType(AM_MEDIA_TYPE * pMediaType)
{
    FreeMediaType(pMediaType);
    CoTaskMemFree(pMediaType);
}

typedef struct tagENUMEDIADETAILS
{
    ULONG cMediaTypes;
    AM_MEDIA_TYPE * pMediaTypes;
} ENUMMEDIADETAILS;

typedef struct IEnumMediaTypesImpl
{
    const IEnumMediaTypesVtbl * lpVtbl;
    LONG refCount;
    BasePin *basePin;
    BasePin_GetMediaType enumMediaFunction;
    BasePin_GetMediaTypeVersion mediaVersionFunction;
    LONG currentVersion;
    ENUMMEDIADETAILS enumMediaDetails;
    ULONG uIndex;
} IEnumMediaTypesImpl;

static const struct IEnumMediaTypesVtbl IEnumMediaTypesImpl_Vtbl;

HRESULT WINAPI EnumMediaTypes_Construct(BasePin *basePin, BasePin_GetMediaType enumFunc, BasePin_GetMediaTypeVersion versionFunc, IEnumMediaTypes ** ppEnum)
{
    ULONG i;
    IEnumMediaTypesImpl * pEnumMediaTypes = CoTaskMemAlloc(sizeof(IEnumMediaTypesImpl));
    AM_MEDIA_TYPE amt;

    if (!pEnumMediaTypes)
    {
        *ppEnum = NULL;
        return E_OUTOFMEMORY;
    }
    pEnumMediaTypes->lpVtbl = &IEnumMediaTypesImpl_Vtbl;
    pEnumMediaTypes->refCount = 1;
    pEnumMediaTypes->uIndex = 0;
    pEnumMediaTypes->enumMediaFunction = enumFunc;
    pEnumMediaTypes->mediaVersionFunction = versionFunc;
    IPin_AddRef((IPin*)basePin);
    pEnumMediaTypes->basePin = basePin;

    i = 0;
    while (enumFunc(basePin,i,&amt) == S_OK) i++;

    pEnumMediaTypes->enumMediaDetails.cMediaTypes = i;
    pEnumMediaTypes->enumMediaDetails.pMediaTypes = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE) * i);
    for (i = 0; i < pEnumMediaTypes->enumMediaDetails.cMediaTypes; i++)
    {
        enumFunc(basePin,i,&amt);
        if (FAILED(CopyMediaType(&pEnumMediaTypes->enumMediaDetails.pMediaTypes[i], &amt)))
        {
           while (i--)
              CoTaskMemFree(pEnumMediaTypes->enumMediaDetails.pMediaTypes[i].pbFormat);
           CoTaskMemFree(pEnumMediaTypes->enumMediaDetails.pMediaTypes);
           return E_OUTOFMEMORY;
        }
    }
    *ppEnum = (IEnumMediaTypes *)(&pEnumMediaTypes->lpVtbl);
    pEnumMediaTypes->currentVersion = versionFunc(basePin);
    return S_OK;
}

static HRESULT WINAPI IEnumMediaTypesImpl_QueryInterface(IEnumMediaTypes * iface, REFIID riid, LPVOID * ppv)
{
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IEnumMediaTypes))
        *ppv = iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI IEnumMediaTypesImpl_AddRef(IEnumMediaTypes * iface)
{
    IEnumMediaTypesImpl *This = (IEnumMediaTypesImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p)->() AddRef from %d\n", iface, refCount - 1);

    return refCount;
}

static ULONG WINAPI IEnumMediaTypesImpl_Release(IEnumMediaTypes * iface)
{
    IEnumMediaTypesImpl *This = (IEnumMediaTypesImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("(%p)->() Release from %d\n", iface, refCount + 1);

    if (!refCount)
    {
        ULONG i;
        for (i = 0; i < This->enumMediaDetails.cMediaTypes; i++)
            if (This->enumMediaDetails.pMediaTypes[i].pbFormat)
                CoTaskMemFree(This->enumMediaDetails.pMediaTypes[i].pbFormat);
        CoTaskMemFree(This->enumMediaDetails.pMediaTypes);
        IPin_Release((IPin*)This->basePin);
        CoTaskMemFree(This);
    }
    return refCount;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Next(IEnumMediaTypes * iface, ULONG cMediaTypes, AM_MEDIA_TYPE ** ppMediaTypes, ULONG * pcFetched)
{
    ULONG cFetched;
    IEnumMediaTypesImpl *This = (IEnumMediaTypesImpl *)iface;

    cFetched = min(This->enumMediaDetails.cMediaTypes, This->uIndex + cMediaTypes) - This->uIndex;

    if (This->currentVersion != This->mediaVersionFunction(This->basePin))
        return VFW_E_ENUM_OUT_OF_SYNC;

    TRACE("(%u, %p, %p)\n", cMediaTypes, ppMediaTypes, pcFetched);
    TRACE("Next uIndex: %u, cFetched: %u\n", This->uIndex, cFetched);

    if (cFetched > 0)
    {
        ULONG i;
        for (i = 0; i < cFetched; i++)
            if (!(ppMediaTypes[i] = CreateMediaType(&This->enumMediaDetails.pMediaTypes[This->uIndex + i])))
            {
                while (i--)
                    DeleteMediaType(ppMediaTypes[i]);
                *pcFetched = 0;
                return E_OUTOFMEMORY;
            }
    }

    if ((cMediaTypes != 1) || pcFetched)
        *pcFetched = cFetched;

    This->uIndex += cFetched;

    if (cFetched != cMediaTypes)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Skip(IEnumMediaTypes * iface, ULONG cMediaTypes)
{
    IEnumMediaTypesImpl *This = (IEnumMediaTypesImpl *)iface;

    TRACE("(%u)\n", cMediaTypes);
    if (This->currentVersion != This->mediaVersionFunction(This->basePin))
        return VFW_E_ENUM_OUT_OF_SYNC;

    if (This->uIndex + cMediaTypes < This->enumMediaDetails.cMediaTypes)
    {
        This->uIndex += cMediaTypes;
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Reset(IEnumMediaTypes * iface)
{
    ULONG i;
    AM_MEDIA_TYPE amt;
    IEnumMediaTypesImpl *This = (IEnumMediaTypesImpl *)iface;

    TRACE("()\n");

    for (i = 0; i < This->enumMediaDetails.cMediaTypes; i++)
        if (This->enumMediaDetails.pMediaTypes[i].pbFormat)
            CoTaskMemFree(This->enumMediaDetails.pMediaTypes[i].pbFormat);
    CoTaskMemFree(This->enumMediaDetails.pMediaTypes);

    i = 0;
    while (This->enumMediaFunction(This->basePin, i,&amt) == S_OK) i++;

    This->enumMediaDetails.cMediaTypes = i;
    This->enumMediaDetails.pMediaTypes = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE) * i);
    for (i = 0; i < This->enumMediaDetails.cMediaTypes; i++)
    {
        This->enumMediaFunction(This->basePin, i,&amt);
        if (FAILED(CopyMediaType(&This->enumMediaDetails.pMediaTypes[i], &amt)))
        {
           while (i--)
              CoTaskMemFree(This->enumMediaDetails.pMediaTypes[i].pbFormat);
           CoTaskMemFree(This->enumMediaDetails.pMediaTypes);
           return E_OUTOFMEMORY;
        }
    }

    This->currentVersion = This->mediaVersionFunction(This->basePin);
    This->uIndex = 0;

    return S_OK;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Clone(IEnumMediaTypes * iface, IEnumMediaTypes ** ppEnum)
{
    HRESULT hr;
    IEnumMediaTypesImpl *This = (IEnumMediaTypesImpl *)iface;

    TRACE("(%p)\n", ppEnum);

    hr = EnumMediaTypes_Construct(This->basePin, This->enumMediaFunction, This->mediaVersionFunction, ppEnum);
    if (FAILED(hr))
        return hr;
    return IEnumMediaTypes_Skip(*ppEnum, This->uIndex);
}

static const IEnumMediaTypesVtbl IEnumMediaTypesImpl_Vtbl =
{
    IEnumMediaTypesImpl_QueryInterface,
    IEnumMediaTypesImpl_AddRef,
    IEnumMediaTypesImpl_Release,
    IEnumMediaTypesImpl_Next,
    IEnumMediaTypesImpl_Skip,
    IEnumMediaTypesImpl_Reset,
    IEnumMediaTypesImpl_Clone
};
