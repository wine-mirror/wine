/*
 * Implementation of IEnumMediaTypes Interface
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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

void CopyMediaType(AM_MEDIA_TYPE * pDest, const AM_MEDIA_TYPE *pSrc)
{
    memcpy(pDest, pSrc, sizeof(AM_MEDIA_TYPE));
    pDest->pbFormat = CoTaskMemAlloc(pSrc->cbFormat);
    memcpy(pDest->pbFormat, pSrc->pbFormat, pSrc->cbFormat);
}

BOOL CompareMediaTypes(const AM_MEDIA_TYPE * pmt1, const AM_MEDIA_TYPE * pmt2, BOOL bWildcards)
{
    TRACE("pmt1: ");
    dump_AM_MEDIA_TYPE(pmt1);
    TRACE("pmt2: ");
    dump_AM_MEDIA_TYPE(pmt2);
    return (((bWildcards && (IsEqualGUID(&pmt1->majortype, &GUID_NULL) || IsEqualGUID(&pmt2->majortype, &GUID_NULL))) || IsEqualGUID(&pmt1->majortype, &pmt2->majortype)) &&
            ((bWildcards && (IsEqualGUID(&pmt1->subtype, &GUID_NULL)   || IsEqualGUID(&pmt2->subtype, &GUID_NULL)))   || IsEqualGUID(&pmt1->subtype, &pmt2->subtype)));
}

void dump_AM_MEDIA_TYPE(const AM_MEDIA_TYPE * pmt)
{
    if (!pmt)
        return;
    TRACE("\t%s\n\t%s\n\t...\n\t%s\n", qzdebugstr_guid(&pmt->majortype), qzdebugstr_guid(&pmt->subtype), qzdebugstr_guid(&pmt->formattype));
}

typedef struct IEnumMediaTypesImpl
{
    const IEnumMediaTypesVtbl * lpVtbl;
    ULONG refCount;
    ENUMMEDIADETAILS enumMediaDetails;
    ULONG uIndex;
} IEnumMediaTypesImpl;

static const struct IEnumMediaTypesVtbl IEnumMediaTypesImpl_Vtbl;

HRESULT IEnumMediaTypesImpl_Construct(const ENUMMEDIADETAILS * pDetails, IEnumMediaTypes ** ppEnum)
{
    ULONG i;
    IEnumMediaTypesImpl * pEnumMediaTypes = CoTaskMemAlloc(sizeof(IEnumMediaTypesImpl));

    if (!pEnumMediaTypes)
    {
        *ppEnum = NULL;
        return E_OUTOFMEMORY;
    }
    pEnumMediaTypes->lpVtbl = &IEnumMediaTypesImpl_Vtbl;
    pEnumMediaTypes->refCount = 1;
    pEnumMediaTypes->uIndex = 0;
    pEnumMediaTypes->enumMediaDetails.cMediaTypes = pDetails->cMediaTypes;
    pEnumMediaTypes->enumMediaDetails.pMediaTypes = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE) * pDetails->cMediaTypes);
    for (i = 0; i < pDetails->cMediaTypes; i++)
        pEnumMediaTypes->enumMediaDetails.pMediaTypes[i] = pDetails->pMediaTypes[i];
    *ppEnum = (IEnumMediaTypes *)(&pEnumMediaTypes->lpVtbl);
    return S_OK;
}

static HRESULT WINAPI IEnumMediaTypesImpl_QueryInterface(IEnumMediaTypes * iface, REFIID riid, LPVOID * ppv)
{
    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)iface;
    else if (IsEqualIID(riid, &IID_IEnumMediaTypes))
        *ppv = (LPVOID)iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI IEnumMediaTypesImpl_AddRef(IEnumMediaTypes * iface)
{
    ICOM_THIS(IEnumMediaTypesImpl, iface);
    TRACE("()\n");
    return ++This->refCount;
}

static ULONG WINAPI IEnumMediaTypesImpl_Release(IEnumMediaTypes * iface)
{
    ICOM_THIS(IEnumMediaTypesImpl, iface);
    TRACE("()\n");
    if (!--This->refCount)
    {
        CoTaskMemFree(This->enumMediaDetails.pMediaTypes);
        CoTaskMemFree(This);
        return 0;
    }
    else
        return This->refCount;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Next(IEnumMediaTypes * iface, ULONG cMediaTypes, AM_MEDIA_TYPE ** ppMediaTypes, ULONG * pcFetched)
{
    ULONG cFetched; 
    ICOM_THIS(IEnumMediaTypesImpl, iface);

    cFetched = min(This->enumMediaDetails.cMediaTypes, This->uIndex + cMediaTypes) - This->uIndex;

    TRACE("(%lu, %p, %p)\n", cMediaTypes, ppMediaTypes, pcFetched);
    TRACE("Next uIndex: %lu, cFetched: %lu\n", This->uIndex, cFetched);

    if (cFetched > 0)
    {
        ULONG i;
        *ppMediaTypes = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE) * cFetched);
        for (i = 0; i < cFetched; i++)
            (*ppMediaTypes)[i] = This->enumMediaDetails.pMediaTypes[This->uIndex + i];
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
    ICOM_THIS(IEnumMediaTypesImpl, iface);

    TRACE("(%lu)\n", cMediaTypes);

    if (This->uIndex + cMediaTypes < This->enumMediaDetails.cMediaTypes)
    {
        This->uIndex += cMediaTypes;
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Reset(IEnumMediaTypes * iface)
{
    ICOM_THIS(IEnumMediaTypesImpl, iface);

    TRACE("()\n");

    This->uIndex = 0;
    return S_OK;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Clone(IEnumMediaTypes * iface, IEnumMediaTypes ** ppEnum)
{
    HRESULT hr;
    ICOM_THIS(IEnumMediaTypesImpl, iface);

    TRACE("(%p)\n", ppEnum);

    hr = IEnumMediaTypesImpl_Construct(&This->enumMediaDetails, ppEnum);
    if (FAILED(hr))
        return hr;
    return IEnumMediaTypes_Skip(*ppEnum, This->uIndex);
}

static const IEnumMediaTypesVtbl IEnumMediaTypesImpl_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IEnumMediaTypesImpl_QueryInterface,
    IEnumMediaTypesImpl_AddRef,
    IEnumMediaTypesImpl_Release,
    IEnumMediaTypesImpl_Next,
    IEnumMediaTypesImpl_Skip,
    IEnumMediaTypesImpl_Reset,
    IEnumMediaTypesImpl_Clone
};
