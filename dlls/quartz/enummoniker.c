/*
 * IEnumMoniker implementation
 *
 * Copyright 2003 Robert Shearman
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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

#define COM_NO_WINDOWS_H
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "strmif.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct EnumMonikerImpl
{
    ICOM_VFIELD(IEnumMoniker);
    ULONG ref;
    IMoniker ** ppMoniker;
    ULONG nMonikerCount;
    ULONG index;
} EnumMonikerImpl;

static struct ICOM_VTABLE(IEnumMoniker) EnumMonikerImpl_Vtbl;

static ULONG WINAPI EnumMonikerImpl_AddRef(LPENUMMONIKER iface);

HRESULT EnumMonikerImpl_Create(IMoniker ** ppMoniker, ULONG nMonikerCount, IEnumMoniker ** ppEnum)
{
    /* NOTE: assumes that array of IMonikers has already been AddRef'd
     * I.e. this function does not AddRef the array of incoming
     * IMonikers */
    EnumMonikerImpl * pemi = CoTaskMemAlloc(sizeof(EnumMonikerImpl));

    TRACE("(%p, %ld, %p)\n", ppMoniker, nMonikerCount, ppEnum);

    *ppEnum = NULL;

    if (!pemi)
        return E_OUTOFMEMORY;

    pemi->lpVtbl = &EnumMonikerImpl_Vtbl;
    pemi->ref = 1;
    pemi->ppMoniker = CoTaskMemAlloc(nMonikerCount * sizeof(IMoniker*));
    memcpy(pemi->ppMoniker, ppMoniker, nMonikerCount*sizeof(IMoniker*));
    pemi->nMonikerCount = nMonikerCount;
    pemi->index = 0;

    *ppEnum = (IEnumMoniker *)pemi;

    return S_OK;
}

/**********************************************************************
 * IEnumMoniker_QueryInterface (also IUnknown)
 */
static HRESULT WINAPI EnumMonikerImpl_QueryInterface(
    LPENUMMONIKER iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    ICOM_THIS(EnumMonikerImpl, iface);
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IEnumMoniker))
    {
        *ppvObj = (LPVOID)iface;
        EnumMonikerImpl_AddRef(iface);
        return S_OK;
    }

    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

/**********************************************************************
 * IEnumMoniker_AddRef (also IUnknown)
 */
static ULONG WINAPI EnumMonikerImpl_AddRef(LPENUMMONIKER iface)
{
    ICOM_THIS(EnumMonikerImpl, iface);

    TRACE("\n");

    if (This == NULL) return E_POINTER;

    return InterlockedIncrement(&This->ref);
}

/**********************************************************************
 * IEnumMoniker_Release (also IUnknown)
 */
static ULONG WINAPI EnumMonikerImpl_Release(LPENUMMONIKER iface)
{
    ICOM_THIS(EnumMonikerImpl, iface);

    TRACE("\n");

    if (!InterlockedDecrement(&This->ref))
    {
        CoTaskMemFree(This->ppMoniker);
        This->ppMoniker = NULL;
        CoTaskMemFree(This);
        return 0;
    }
    return This->ref;
}

static HRESULT WINAPI EnumMonikerImpl_Next(LPENUMMONIKER iface, ULONG celt, IMoniker ** rgelt, ULONG * pceltFetched)
{
    ULONG fetched;
    ICOM_THIS(EnumMonikerImpl, iface);

    TRACE("(%ld, %p, %p)\n", celt, rgelt, pceltFetched);

    for (fetched = 0; (This->index + fetched < This->nMonikerCount) && (fetched < celt); fetched++)
    {
        rgelt[fetched] = This->ppMoniker[This->index + fetched];
        IMoniker_AddRef(rgelt[fetched]);
    }

    This->index += fetched;

    if (pceltFetched)
        *pceltFetched = fetched;

    if (fetched != celt)
        return S_FALSE;
    else
        return S_OK;
}

static HRESULT WINAPI EnumMonikerImpl_Skip(LPENUMMONIKER iface, ULONG celt)
{
    ICOM_THIS(EnumMonikerImpl, iface);

    TRACE("(%ld)\n", celt);

    This->index += celt;

    return S_OK;
}

static HRESULT WINAPI EnumMonikerImpl_Reset(LPENUMMONIKER iface)
{
    ICOM_THIS(EnumMonikerImpl, iface);

    TRACE("()\n");

    This->index = 0;

    return S_OK;
}

static HRESULT WINAPI EnumMonikerImpl_Clone(LPENUMMONIKER iface, IEnumMoniker ** ppenum)
{
    FIXME("(%p): stub\n", ppenum);

    return E_NOTIMPL;
}

/**********************************************************************
 * IEnumMoniker_Vtbl
 */
static ICOM_VTABLE(IEnumMoniker) EnumMonikerImpl_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    EnumMonikerImpl_QueryInterface,
    EnumMonikerImpl_AddRef,
    EnumMonikerImpl_Release,
    EnumMonikerImpl_Next,
    EnumMonikerImpl_Skip,
    EnumMonikerImpl_Reset,
    EnumMonikerImpl_Clone
};
