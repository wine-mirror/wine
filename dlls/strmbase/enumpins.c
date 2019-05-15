/*
 * Implementation of IEnumPins Interface
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

#include "strmbase_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

typedef struct IEnumPinsImpl
{
    IEnumPins IEnumPins_iface;
    LONG refCount;
    unsigned int uIndex, count;
    BaseFilter *base;
    DWORD Version;
} IEnumPinsImpl;

static inline IEnumPinsImpl *impl_from_IEnumPins(IEnumPins *iface)
{
    return CONTAINING_RECORD(iface, IEnumPinsImpl, IEnumPins_iface);
}

static const struct IEnumPinsVtbl IEnumPinsImpl_Vtbl;

HRESULT enum_pins_create(BaseFilter *base, IEnumPins **out)
{
    IEnumPinsImpl *object;
    IPin *pin;

    if (!out)
        return E_POINTER;

    if (!(object = heap_alloc_zero(sizeof(*object))))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    object->IEnumPins_iface.lpVtbl = &IEnumPinsImpl_Vtbl;
    object->refCount = 1;
    object->base = base;
    IBaseFilter_AddRef(&base->IBaseFilter_iface);
    object->Version = base->pin_version;

    while ((pin = base->pFuncsTable->pfnGetPin(base, object->count)))
    {
        IPin_Release(pin);
        ++object->count;
    }

    TRACE("Created enumerator %p.\n", object);
    *out = &object->IEnumPins_iface;

    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_QueryInterface(IEnumPins * iface, REFIID riid, LPVOID * ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IEnumPins))
        *ppv = iface;

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
    IEnumPinsImpl *This = impl_from_IEnumPins(iface);
    ULONG ref = InterlockedIncrement(&This->refCount);

    TRACE("(%p)->(): new ref =  %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IEnumPinsImpl_Release(IEnumPins * iface)
{
    IEnumPinsImpl *This = impl_from_IEnumPins(iface);
    ULONG ref = InterlockedDecrement(&This->refCount);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref)
    {
        IBaseFilter_Release(&This->base->IBaseFilter_iface);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI IEnumPinsImpl_Next(IEnumPins * iface, ULONG cPins, IPin ** ppPins, ULONG * pcFetched)
{
    IEnumPinsImpl *This = impl_from_IEnumPins(iface);
    ULONG i = 0;

    TRACE("(%p)->(%u, %p, %p)\n", iface, cPins, ppPins, pcFetched);

    if (!ppPins)
        return E_POINTER;

    if (cPins > 1 && !pcFetched)
        return E_INVALIDARG;

    if (pcFetched)
        *pcFetched = 0;

    if (This->Version != This->base->pin_version)
        return VFW_E_ENUM_OUT_OF_SYNC;

    while (i < cPins)
    {
       IPin *pin;
       pin = This->base->pFuncsTable->pfnGetPin(This->base, This->uIndex + i);

       if (!pin)
         break;
       else
         ppPins[i] = pin;
       ++i;
    }

    if (pcFetched)
        *pcFetched = i;
    This->uIndex += i;

    if (i < cPins)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_Skip(IEnumPins *iface, ULONG count)
{
    IEnumPinsImpl *enum_pins = impl_from_IEnumPins(iface);

    TRACE("enum_pins %p, count %u.\n", enum_pins, count);

    if (enum_pins->Version != enum_pins->base->pin_version)
        return VFW_E_ENUM_OUT_OF_SYNC;

    if (enum_pins->uIndex + count > enum_pins->count)
        return S_FALSE;

    enum_pins->uIndex += count;
    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_Reset(IEnumPins *iface)
{
    IEnumPinsImpl *enum_pins = impl_from_IEnumPins(iface);
    IPin *pin;

    TRACE("iface %p.\n", iface);

    if (enum_pins->Version != enum_pins->base->pin_version)
    {
        enum_pins->count = 0;
        while ((pin = enum_pins->base->pFuncsTable->pfnGetPin(enum_pins->base, enum_pins->count)))
        {
            IPin_Release(pin);
            ++enum_pins->count;
        }
    }

    enum_pins->Version = enum_pins->base->pin_version;
    enum_pins->uIndex = 0;

    return S_OK;
}

static HRESULT WINAPI IEnumPinsImpl_Clone(IEnumPins *iface, IEnumPins **out)
{
    IEnumPinsImpl *enum_pins = impl_from_IEnumPins(iface);
    HRESULT hr;

    TRACE("iface %p, out %p.\n", iface, out);

    if (FAILED(hr = enum_pins_create(enum_pins->base, out)))
        return hr;
    return IEnumPins_Skip(*out, enum_pins->uIndex);
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
