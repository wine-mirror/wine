/*
 * Copyright 2023 Dmitry Timoshkov
 *
 * This file contains only stubs to get the printui.dll up and running
 * activeds.dll is much much more than this
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
#include "iads.h"
#include "adserr.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(activeds);

typedef struct
{
    IADsDNWithBinary IADsDNWithBinary_iface;
    LONG ref;
    BSTR dn;
    VARIANT bin;
} ADsDNWithBinary;

static inline ADsDNWithBinary *impl_from_IADsDNWithBinary(IADsDNWithBinary *iface)
{
    return CONTAINING_RECORD(iface, ADsDNWithBinary, IADsDNWithBinary_iface);
}

static HRESULT WINAPI dnb_QueryInterface(IADsDNWithBinary *iface, REFIID riid, void **obj)
{
    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IADsDNWithBinary))
    {
        IADsDNWithBinary_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI dnb_AddRef(IADsDNWithBinary *iface)
{
    ADsDNWithBinary *path = impl_from_IADsDNWithBinary(iface);
    return InterlockedIncrement(&path->ref);
}

static ULONG WINAPI dnb_Release(IADsDNWithBinary *iface)
{
    ADsDNWithBinary *dnb = impl_from_IADsDNWithBinary(iface);
    LONG ref = InterlockedDecrement(&dnb->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        SysFreeString(dnb->dn);
        VariantClear(&dnb->bin);
        free(dnb);
    }

    return ref;
}

static HRESULT WINAPI dnb_GetTypeInfoCount(IADsDNWithBinary *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI dnb_GetTypeInfo(IADsDNWithBinary *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%#lx,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI dnb_GetIDsOfNames(IADsDNWithBinary *iface, REFIID riid, LPOLESTR *names,
                                         UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%lu,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI dnb_Invoke(IADsDNWithBinary *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                  DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%ld,%s,%04lx,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI dnb_get_BinaryValue(IADsDNWithBinary *iface, VARIANT *value)
{
    ADsDNWithBinary *dnb = impl_from_IADsDNWithBinary(iface);

    TRACE("%p,%p\n", iface, value);

    VariantInit(value);
    return VariantCopy(value, &dnb->bin);
}

static HRESULT WINAPI dnb_put_BinaryValue(IADsDNWithBinary *iface, VARIANT value)
{
    ADsDNWithBinary *dnb = impl_from_IADsDNWithBinary(iface);

    TRACE("%p,%s\n", iface, wine_dbgstr_variant(&value));

    VariantClear(&dnb->bin);
    return VariantCopy(&dnb->bin, &value);
}

static HRESULT WINAPI dnb_get_DNString(IADsDNWithBinary *iface, BSTR *value)
{
    ADsDNWithBinary *dnb = impl_from_IADsDNWithBinary(iface);

    TRACE("%p,%p\n", iface, value);

    *value = SysAllocString(dnb->dn);
    return *value ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI dnb_put_DNString(IADsDNWithBinary *iface, BSTR value)
{
    ADsDNWithBinary *dnb = impl_from_IADsDNWithBinary(iface);

    TRACE("%p,%s\n", iface, debugstr_w(value));

    SysFreeString(dnb->dn);
    dnb->dn = SysAllocString(value);
    return dnb->dn ? S_OK : E_OUTOFMEMORY;
}

static const IADsDNWithBinaryVtbl IADsDNWithBinary_vtbl =
{
    dnb_QueryInterface,
    dnb_AddRef,
    dnb_Release,
    dnb_GetTypeInfoCount,
    dnb_GetTypeInfo,
    dnb_GetIDsOfNames,
    dnb_Invoke,
    dnb_get_BinaryValue,
    dnb_put_BinaryValue,
    dnb_get_DNString,
    dnb_put_DNString
};

HRESULT ADsDNWithBinary_create(REFIID riid, void **obj)
{
    ADsDNWithBinary *dnb;
    HRESULT hr;

    dnb = malloc(sizeof(*dnb));
    if (!dnb) return E_OUTOFMEMORY;

    dnb->IADsDNWithBinary_iface.lpVtbl = &IADsDNWithBinary_vtbl;
    dnb->ref = 1;
    dnb->dn = NULL;
    VariantInit(&dnb->bin);

    hr = IADsDNWithBinary_QueryInterface(&dnb->IADsDNWithBinary_iface, riid, obj);
    IADsDNWithBinary_Release(&dnb->IADsDNWithBinary_iface);

    return hr;
}
