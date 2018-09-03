/*
 * Copyright 2018 Nikolay Sivov for CodeWeavers
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"

#include "wine/debug.h"

#include "opc_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msopc);

struct opc_uri
{
    IOpcPartUri IOpcPartUri_iface;
    LONG refcount;
};

static inline struct opc_uri *impl_from_IOpcPartUri(IOpcPartUri *iface)
{
    return CONTAINING_RECORD(iface, struct opc_uri, IOpcPartUri_iface);
}

static HRESULT WINAPI opc_uri_QueryInterface(IOpcPartUri *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualIID(iid, &IID_IOpcPartUri) ||
            IsEqualIID(iid, &IID_IUnknown))
    {
        *out = iface;
        IOpcPartUri_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI opc_uri_AddRef(IOpcPartUri *iface)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);
    ULONG refcount = InterlockedIncrement(&uri->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI opc_uri_Release(IOpcPartUri *iface)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);
    ULONG refcount = InterlockedDecrement(&uri->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        heap_free(uri);

    return refcount;
}

static HRESULT WINAPI opc_uri_GetPropertyBSTR(IOpcPartUri *iface, Uri_PROPERTY property,
        BSTR *value, DWORD flags)
{
    FIXME("iface %p, property %d, value %p, flags %#x stub!\n", iface, property, value, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetPropertyLength(IOpcPartUri *iface, Uri_PROPERTY property,
        DWORD *length, DWORD flags)
{
    FIXME("iface %p, property %d, length %p, flags %#x stub!\n", iface, property, length, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetPropertyDWORD(IOpcPartUri *iface, Uri_PROPERTY property,
        DWORD *value, DWORD flags)
{
    FIXME("iface %p, property %d, value %p, flags %#x stub!\n", iface, property, value, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_HasProperty(IOpcPartUri *iface, Uri_PROPERTY property,
        BOOL *has_property)
{
    FIXME("iface %p, property %d, has_property %p stub!\n", iface, property, has_property);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetAbsoluteUri(IOpcPartUri *iface, BSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetAuthority(IOpcPartUri *iface, BSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetDisplayUri(IOpcPartUri *iface, BSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetDomain(IOpcPartUri *iface, BSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetExtension(IOpcPartUri *iface, BSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetFragment(IOpcPartUri *iface, BSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetHost(IOpcPartUri *iface, BSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetPassword(IOpcPartUri *iface, BSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetPath(IOpcPartUri *iface, BSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetPathAndQuery(IOpcPartUri *iface, BSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetQuery(IOpcPartUri *iface, BSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetRawUri(IOpcPartUri *iface, BSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetSchemeName(IOpcPartUri *iface, BSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetUserInfo(IOpcPartUri *iface, BSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetUserName(IOpcPartUri *iface, BSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetHostType(IOpcPartUri *iface, DWORD *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetPort(IOpcPartUri *iface, DWORD *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetScheme(IOpcPartUri *iface, DWORD *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetZone(IOpcPartUri *iface, DWORD *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetProperties(IOpcPartUri *iface, DWORD *flags)
{
    FIXME("iface %p, flags %p stub!\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_IsEqual(IOpcPartUri *iface, IUri *uri, BOOL *is_equal)
{
    FIXME("iface %p, uri %p, is_equal %p stub!\n", iface, uri, is_equal);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetRelationshipsPartUri(IOpcPartUri *iface, IOpcPartUri **part_uri)
{
    FIXME("iface %p, part_uri %p stub!\n", iface, part_uri);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetRelativeUri(IOpcPartUri *iface, IOpcPartUri *part_uri,
        IUri **relative_uri)
{
    FIXME("iface %p, part_uri %p, relative_uri %p stub!\n", iface, part_uri, relative_uri);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_CombinePartUri(IOpcPartUri *iface, IUri *relative_uri, IOpcPartUri **combined)
{
    FIXME("iface %p, relative_uri %p, combined %p stub!\n", iface, relative_uri, combined);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_ComparePartUri(IOpcPartUri *iface, IOpcPartUri *part_uri,
        INT32 *result)
{
    FIXME("iface %p, part_uri %p, result %p stub!\n", iface, part_uri, result);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetSourceUri(IOpcPartUri *iface, IOpcUri **source_uri)
{
    FIXME("iface %p, source_uri %p stub!\n", iface, source_uri);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_IsRelationshipsPartUri(IOpcPartUri *iface, BOOL *result)
{
    FIXME("iface %p, result %p stub!\n", iface, result);

    return E_NOTIMPL;
}

static const IOpcPartUriVtbl opc_part_uri_vtbl =
{
    opc_uri_QueryInterface,
    opc_uri_AddRef,
    opc_uri_Release,
    opc_uri_GetPropertyBSTR,
    opc_uri_GetPropertyLength,
    opc_uri_GetPropertyDWORD,
    opc_uri_HasProperty,
    opc_uri_GetAbsoluteUri,
    opc_uri_GetAuthority,
    opc_uri_GetDisplayUri,
    opc_uri_GetDomain,
    opc_uri_GetExtension,
    opc_uri_GetFragment,
    opc_uri_GetHost,
    opc_uri_GetPassword,
    opc_uri_GetPath,
    opc_uri_GetPathAndQuery,
    opc_uri_GetQuery,
    opc_uri_GetRawUri,
    opc_uri_GetSchemeName,
    opc_uri_GetUserInfo,
    opc_uri_GetUserName,
    opc_uri_GetHostType,
    opc_uri_GetPort,
    opc_uri_GetScheme,
    opc_uri_GetZone,
    opc_uri_GetProperties,
    opc_uri_IsEqual,
    opc_uri_GetRelationshipsPartUri,
    opc_uri_GetRelativeUri,
    opc_uri_CombinePartUri,
    opc_uri_ComparePartUri,
    opc_uri_GetSourceUri,
    opc_uri_IsRelationshipsPartUri,
};

HRESULT opc_part_uri_create(const WCHAR *str, IOpcPartUri **out)
{
    struct opc_uri *uri;

    if (!(uri = heap_alloc_zero(sizeof(*uri))))
        return E_OUTOFMEMORY;

    uri->IOpcPartUri_iface.lpVtbl = &opc_part_uri_vtbl;
    uri->refcount = 1;

    *out = &uri->IOpcPartUri_iface;
    TRACE("Created part uri %p.\n", *out);
    return S_OK;
}
