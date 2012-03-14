/*
 * IAssemblyName implementation
 *
 * Copyright 2012 Hans Leidekker for CodeWeavers
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
#include "ole2.h"
#include "winsxs.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sxs);

struct name
{
    IAssemblyName IAssemblyName_iface;
    LONG   refs;
};

static inline struct name *impl_from_IAssemblyName( IAssemblyName *iface )
{
    return CONTAINING_RECORD( iface, struct name, IAssemblyName_iface );
}

static HRESULT WINAPI name_QueryInterface(
    IAssemblyName *iface,
    REFIID riid,
    void **obj )
{
    struct name *name = impl_from_IAssemblyName( iface );

    TRACE("%p, %s, %p\n", name, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID( riid, &IID_IUnknown ) ||
        IsEqualIID( riid, &IID_IAssemblyName ))
    {
        IUnknown_AddRef( iface );
        *obj = name;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI name_AddRef(
    IAssemblyName *iface )
{
    struct name *name = impl_from_IAssemblyName( iface );
    return InterlockedIncrement( &name->refs );
}

static ULONG WINAPI name_Release( IAssemblyName *iface )
{
    struct name *name = impl_from_IAssemblyName( iface );
    ULONG refs = InterlockedDecrement( &name->refs );

    if (!refs)
    {
        TRACE("destroying %p\n", name);
        HeapFree( GetProcessHeap(), 0, name );
    }
    return refs;
}

static HRESULT WINAPI name_SetProperty(
    IAssemblyName *iface,
    DWORD id,
    LPVOID property,
    DWORD size )
{
    FIXME("%p, %d, %p, %d\n", iface, id, property, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI name_GetProperty(
    IAssemblyName *iface,
    DWORD id,
    LPVOID buffer,
    LPDWORD buflen )
{
    FIXME("%p, %d, %p, %p\n", iface, id, buffer, buflen);
    return E_NOTIMPL;
}

static HRESULT WINAPI name_Finalize(
    IAssemblyName *iface )
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI name_GetDisplayName(
    IAssemblyName *iface,
    LPOLESTR buffer,
    LPDWORD buflen,
    DWORD flags )
{
    FIXME("%p, %p, %p, 0x%08x\n", iface, buffer, buflen, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI name_Reserved(
    IAssemblyName *iface,
    REFIID riid,
    IUnknown *pUnkReserved1,
    IUnknown *pUnkReserved2,
    LPCOLESTR szReserved,
    LONGLONG llReserved,
    LPVOID pvReserved,
    DWORD cbReserved,
    LPVOID *ppReserved )
{
    FIXME("%p, %s, %p, %p, %s, %x%08x, %p, %d, %p\n", iface,
          debugstr_guid(riid), pUnkReserved1, pUnkReserved2,
          debugstr_w(szReserved), (DWORD)(llReserved >> 32), (DWORD)llReserved,
          pvReserved, cbReserved, ppReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI name_GetName(
    IAssemblyName *iface,
    LPDWORD buflen,
    WCHAR *buffer )
{
    FIXME("%p, %p, %p\n", iface, buflen, buffer);
    return E_NOTIMPL;
}

static HRESULT WINAPI name_GetVersion(
    IAssemblyName *iface,
    LPDWORD hi,
    LPDWORD low )
{
    FIXME("%p, %p, %p\n", iface, hi, low);
    return E_NOTIMPL;
}

static HRESULT WINAPI name_IsEqual(
    IAssemblyName *name1,
    IAssemblyName *name2,
    DWORD flags )
{
    FIXME("%p, %p, 0x%08x\n", name1, name2, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI name_Clone(
    IAssemblyName *iface,
    IAssemblyName **name )
{
    FIXME("%p, %p\n", iface, name);
    return E_NOTIMPL;
}

static const IAssemblyNameVtbl name_vtbl =
{
    name_QueryInterface,
    name_AddRef,
    name_Release,
    name_SetProperty,
    name_GetProperty,
    name_Finalize,
    name_GetDisplayName,
    name_Reserved,
    name_GetName,
    name_GetVersion,
    name_IsEqual,
    name_Clone
};

/******************************************************************
 *  CreateAssemblyNameObject   (SXS.@)
 */
HRESULT WINAPI CreateAssemblyNameObject(
    LPASSEMBLYNAME *obj,
    LPCWSTR assembly,
    DWORD flags,
    LPVOID reserved )
{
    struct name *name;

    TRACE("%p, %s, 0x%08x, %p\n", obj, debugstr_w(assembly), flags, reserved);

    if (!obj) return E_INVALIDARG;

    *obj = NULL;
    if (!assembly || !assembly[0] || flags != CANOF_PARSE_DISPLAY_NAME)
        return E_INVALIDARG;

    if (!(name = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*name) )))
        return E_OUTOFMEMORY;

    name->IAssemblyName_iface.lpVtbl = &name_vtbl;
    name->refs = 1;

    *obj = &name->IAssemblyName_iface;
    return S_OK;
}
