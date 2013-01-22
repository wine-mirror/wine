/*
 * Copyright 2013 Hans Leidekker for CodeWeavers
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

#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wbemcli.h"

#include "wine/debug.h"
#include "wbemprox_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

struct qualifier_set
{
    IWbemQualifierSet IWbemQualifierSet_iface;
    LONG refs;
};

static inline struct qualifier_set *impl_from_IWbemQualifierSet(
    IWbemQualifierSet *iface )
{
    return CONTAINING_RECORD(iface, struct qualifier_set, IWbemQualifierSet_iface);
}

static ULONG WINAPI qualifier_set_AddRef(
    IWbemQualifierSet *iface )
{
    struct qualifier_set *set = impl_from_IWbemQualifierSet( iface );
    return InterlockedIncrement( &set->refs );
}

static ULONG WINAPI qualifier_set_Release(
    IWbemQualifierSet *iface )
{
    struct qualifier_set *set = impl_from_IWbemQualifierSet( iface );
    LONG refs = InterlockedDecrement( &set->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", set);
        heap_free( set );
    }
    return refs;
}

static HRESULT WINAPI qualifier_set_QueryInterface(
    IWbemQualifierSet *iface,
    REFIID riid,
    void **ppvObject )
{
    struct qualifier_set *set = impl_from_IWbemQualifierSet( iface );

    TRACE("%p, %s, %p\n", set, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IWbemQualifierSet ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = set;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IWbemQualifierSet_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI qualifier_set_Get(
    IWbemQualifierSet *iface,
    LPCWSTR wszName,
    LONG lFlags,
    VARIANT *pVal,
    LONG *plFlavor )
{
    FIXME("%p, %s, %08x, %p, %p\n", iface, debugstr_w(wszName), lFlags, pVal, plFlavor);
    return E_NOTIMPL;
}

static HRESULT WINAPI qualifier_set_Put(
    IWbemQualifierSet *iface,
    LPCWSTR wszName,
    VARIANT *pVal,
    LONG lFlavor )
{
    FIXME("%p, %s, %p, %d\n", iface, debugstr_w(wszName), pVal, lFlavor);
    return E_NOTIMPL;
}

static HRESULT WINAPI qualifier_set_Delete(
    IWbemQualifierSet *iface,
    LPCWSTR wszName )
{
    FIXME("%p, %s\n", iface, debugstr_w(wszName));
    return E_NOTIMPL;
}

static HRESULT WINAPI qualifier_set_GetNames(
    IWbemQualifierSet *iface,
    LONG lFlags,
    SAFEARRAY **pNames )
{
    FIXME("%p, %08x, %p\n", iface, lFlags, pNames);
    return E_NOTIMPL;
}

static HRESULT WINAPI qualifier_set_BeginEnumeration(
    IWbemQualifierSet *iface,
    LONG lFlags )
{
    FIXME("%p, %08x\n", iface, lFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI qualifier_set_Next(
    IWbemQualifierSet *iface,
    LONG lFlags,
    BSTR *pstrName,
    VARIANT *pVal,
    LONG *plFlavor )
{
    FIXME("%p, %08x, %p, %p, %p\n", iface, lFlags, pstrName, pVal, plFlavor);
    return E_NOTIMPL;
}

static HRESULT WINAPI qualifier_set_EndEnumeration(
    IWbemQualifierSet *iface )
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static const IWbemQualifierSetVtbl qualifier_set_vtbl =
{
    qualifier_set_QueryInterface,
    qualifier_set_AddRef,
    qualifier_set_Release,
    qualifier_set_Get,
    qualifier_set_Put,
    qualifier_set_Delete,
    qualifier_set_GetNames,
    qualifier_set_BeginEnumeration,
    qualifier_set_Next,
    qualifier_set_EndEnumeration
};

HRESULT WbemQualifierSet_create(
    IUnknown *pUnkOuter, LPVOID *ppObj )
{
    struct qualifier_set *set;

    TRACE("%p, %p\n", pUnkOuter, ppObj);

    if (!(set = heap_alloc( sizeof(*set) ))) return E_OUTOFMEMORY;

    set->IWbemQualifierSet_iface.lpVtbl = &qualifier_set_vtbl;
    set->refs = 1;

    *ppObj = &set->IWbemQualifierSet_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
