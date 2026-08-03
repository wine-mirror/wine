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

static inline struct opc_uri *impl_from_IOpcPartUri(IOpcPartUri *iface)
{
    return CONTAINING_RECORD(iface, struct opc_uri, IOpcPartUri_iface);
}

static HRESULT opc_source_uri_create(struct opc_uri *uri, IOpcUri **out);

static HRESULT WINAPI opc_uri_QueryInterface(IOpcPartUri *iface, REFIID iid, void **out)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if ((uri->is_part_uri && IsEqualIID(iid, &IID_IOpcPartUri)) ||
            IsEqualIID(iid, &IID_IOpcUri) ||
            IsEqualIID(iid, &IID_IUri) ||
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

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI opc_uri_Release(IOpcPartUri *iface)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);
    ULONG refcount = InterlockedDecrement(&uri->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (uri->rels_part_uri)
            IUri_Release(uri->rels_part_uri);
        if (uri->source_uri)
            IOpcPartUri_Release(&uri->source_uri->IOpcPartUri_iface);
        IUri_Release(uri->uri);
        free(uri);
    }

    return refcount;
}

static HRESULT WINAPI opc_uri_GetPropertyBSTR(IOpcPartUri *iface, Uri_PROPERTY property,
        BSTR *value, DWORD flags)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, property %d, value %p, flags %#lx.\n", iface, property, value, flags);

    return IUri_GetPropertyBSTR(uri->uri, property, value, flags);
}

static HRESULT WINAPI opc_uri_GetPropertyLength(IOpcPartUri *iface, Uri_PROPERTY property,
        DWORD *length, DWORD flags)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, property %d, length %p, flags %#lx.\n", iface, property, length, flags);

    return IUri_GetPropertyLength(uri->uri, property, length, flags);
}

static HRESULT WINAPI opc_uri_GetPropertyDWORD(IOpcPartUri *iface, Uri_PROPERTY property,
        DWORD *value, DWORD flags)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, property %d, value %p, flags %#lx.\n", iface, property, value, flags);

    return IUri_GetPropertyDWORD(uri->uri, property, value, flags);
}

static HRESULT WINAPI opc_uri_HasProperty(IOpcPartUri *iface, Uri_PROPERTY property,
        BOOL *has_property)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, property %d, has_property %p.\n", iface, property, has_property);

    return IUri_HasProperty(uri->uri, property, has_property);
}

static HRESULT WINAPI opc_uri_GetAbsoluteUri(IOpcPartUri *iface, BSTR *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetAbsoluteUri(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetAuthority(IOpcPartUri *iface, BSTR *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetAuthority(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetDisplayUri(IOpcPartUri *iface, BSTR *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetDisplayUri(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetDomain(IOpcPartUri *iface, BSTR *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetDomain(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetExtension(IOpcPartUri *iface, BSTR *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetExtension(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetFragment(IOpcPartUri *iface, BSTR *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetFragment(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetHost(IOpcPartUri *iface, BSTR *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetHost(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetPassword(IOpcPartUri *iface, BSTR *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetPassword(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetPath(IOpcPartUri *iface, BSTR *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetPath(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetPathAndQuery(IOpcPartUri *iface, BSTR *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetPathAndQuery(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetQuery(IOpcPartUri *iface, BSTR *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetQuery(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetRawUri(IOpcPartUri *iface, BSTR *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetRawUri(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetSchemeName(IOpcPartUri *iface, BSTR *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetSchemeName(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetUserInfo(IOpcPartUri *iface, BSTR *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetUserInfo(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetUserName(IOpcPartUri *iface, BSTR *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetUserName(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetHostType(IOpcPartUri *iface, DWORD *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetHostType(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetPort(IOpcPartUri *iface, DWORD *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetPort(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetScheme(IOpcPartUri *iface, DWORD *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetScheme(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetZone(IOpcPartUri *iface, DWORD *value)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IUri_GetZone(uri->uri, value);
}

static HRESULT WINAPI opc_uri_GetProperties(IOpcPartUri *iface, DWORD *flags)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, flags %p.\n", iface, flags);

    return IUri_GetProperties(uri->uri, flags);
}

static HRESULT WINAPI opc_uri_IsEqual(IOpcPartUri *iface, IUri *comparand, BOOL *is_equal)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, comparand %p, is_equal %p.\n", iface, comparand, is_equal);

    if (!is_equal)
        return E_POINTER;

    if (!comparand)
    {
        if (uri->is_part_uri)
        {
            *is_equal = FALSE;
            return S_OK;
        }

        return E_POINTER;
    }

    return IUri_IsEqual(comparand, uri->uri, is_equal);
}

static HRESULT WINAPI opc_uri_GetRelationshipsPartUri(IOpcPartUri *iface, IOpcPartUri **part_uri)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, part_uri %p.\n", iface, part_uri);

    if (!part_uri)
        return E_POINTER;

    if (!uri->rels_part_uri)
    {
        *part_uri = NULL;
        return OPC_E_NONCONFORMING_URI;
    }

    return opc_part_uri_create(uri->rels_part_uri, uri, part_uri);
}

static HRESULT WINAPI opc_uri_GetRelativeUri(IOpcPartUri *iface, IOpcPartUri *part_uri,
        IUri **relative_uri)
{
    FIXME("iface %p, part_uri %p, relative_uri %p stub!\n", iface, part_uri, relative_uri);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_CombinePartUri(IOpcPartUri *iface, IUri *relative_uri, IOpcPartUri **combined)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);
    IUri *combined_uri;
    HRESULT hr;

    TRACE("iface %p, relative_uri %p, combined %p.\n", iface, relative_uri, combined);

    if (!combined)
        return E_POINTER;

    *combined = NULL;

    if (!relative_uri)
        return E_POINTER;

    if (FAILED(hr = CoInternetCombineIUri(uri->uri, relative_uri, 0, &combined_uri, 0)))
        return hr;

    hr = opc_part_uri_create(combined_uri, NULL, combined);
    IUri_Release(combined_uri);
    return hr;
}

static HRESULT WINAPI opc_uri_ComparePartUri(IOpcPartUri *iface, IOpcPartUri *part_uri,
        INT32 *result)
{
    FIXME("iface %p, part_uri %p, result %p stub!\n", iface, part_uri, result);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_uri_GetSourceUri(IOpcPartUri *iface, IOpcUri **source_uri)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, source_uri %p.\n", iface, source_uri);

    return opc_source_uri_create(uri, source_uri);
}

static HRESULT WINAPI opc_uri_IsRelationshipsPartUri(IOpcPartUri *iface, BOOL *result)
{
    struct opc_uri *uri = impl_from_IOpcPartUri(iface);

    TRACE("iface %p, result %p.\n", iface, result);

    if (!result)
        return E_POINTER;

    *result = !uri->rels_part_uri;

    return S_OK;
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

static IUri *opc_part_uri_get_rels_uri(IUri *uri)
{
    static const WCHAR relsdirW[] = L"/_rels";
    static const WCHAR relsextW[] = L".rels";
    WCHAR *start = NULL, *end, *ret;
    IUri *rels_uri;
    HRESULT hr;
    DWORD len;
    BSTR path;

    if (FAILED(IUri_GetPath(uri, &path)))
        return NULL;

    if (FAILED(IUri_GetPropertyLength(uri, Uri_PROPERTY_PATH, &len, 0)))
    {
        SysFreeString(path);
        return NULL;
    }

    end = wcsrchr(path, '/');
    if (end && end >= path + ARRAY_SIZE(relsdirW) - 1)
        start = end - ARRAY_SIZE(relsdirW) + 1;
    if (!start)
        start = end;

    /* Test if it's already relationships uri. */
    if (len > ARRAY_SIZE(relsextW))
    {
        if (!wcscmp(path + len - ARRAY_SIZE(relsextW) + 1, relsextW))
        {
            if (start && !memcmp(start, relsdirW, ARRAY_SIZE(relsdirW) - sizeof(WCHAR)))
            {
                SysFreeString(path);
                return NULL;
            }
        }
    }

    ret = malloc((len + ARRAY_SIZE(relsextW) + ARRAY_SIZE(relsdirW)) * sizeof(WCHAR));
    if (!ret)
    {
        SysFreeString(path);
        return NULL;
    }
    ret[0] = 0;

    if (start != path)
    {
        memcpy(ret, path, (start - path) * sizeof(WCHAR));
        ret[start - path] = 0;
    }

    lstrcatW(ret, relsdirW);
    lstrcatW(ret, end);
    lstrcatW(ret, relsextW);

    if (FAILED(hr = CreateUri(ret, Uri_CREATE_ALLOW_RELATIVE, 0, &rels_uri)))
        WARN("Failed to create rels uri, hr %#lx.\n", hr);
    free(ret);
    SysFreeString(path);

    return rels_uri;
}

static HRESULT opc_part_uri_init(struct opc_uri *object, struct opc_uri *source_uri, BOOL is_part_uri, IUri *uri)
{
    object->IOpcPartUri_iface.lpVtbl = &opc_part_uri_vtbl;
    object->refcount = 1;
    object->is_part_uri = is_part_uri;
    object->uri = uri;
    IUri_AddRef(object->uri);
    object->rels_part_uri = opc_part_uri_get_rels_uri(object->uri);
    object->source_uri = source_uri;
    if (object->source_uri)
        IOpcPartUri_AddRef(&object->source_uri->IOpcPartUri_iface);

    return S_OK;
}

static HRESULT opc_source_uri_create(struct opc_uri *uri, IOpcUri **out)
{
    struct opc_uri *obj;
    HRESULT hr;

    if (!out)
        return E_POINTER;

    *out = NULL;

    if (!uri->source_uri)
        return OPC_E_RELATIONSHIP_URI_REQUIRED;

    if (!(obj = calloc(1, sizeof(*obj))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = opc_part_uri_init(obj, NULL, uri->source_uri->is_part_uri, uri->source_uri->uri)))
    {
        WARN("Failed to init part uri, hr %#lx.\n", hr);
        free(obj);
        return hr;
    }

    *out = (IOpcUri *)&obj->IOpcPartUri_iface;

    TRACE("Created source uri %p.\n", *out);

    return S_OK;
}

HRESULT opc_part_uri_create(IUri *uri, struct opc_uri *source_uri, IOpcPartUri **out)
{
    struct opc_uri *obj;
    HRESULT hr;

    if (!(obj = calloc(1, sizeof(*obj))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = opc_part_uri_init(obj, source_uri, TRUE, uri)))
    {
        WARN("Failed to init part uri, hr %#lx.\n", hr);
        free(obj);
        return hr;
    }

    *out = &obj->IOpcPartUri_iface;
    TRACE("Created part uri %p.\n", *out);
    return S_OK;
}

HRESULT opc_root_uri_create(IOpcUri **out)
{
    struct opc_uri *obj;
    HRESULT hr;
    IUri *uri;

    *out = NULL;

    if (!(obj = calloc(1, sizeof(*obj))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = CreateUri(L"/", Uri_CREATE_ALLOW_RELATIVE, 0, &uri)))
    {
        WARN("Failed to create rels uri, hr %#lx.\n", hr);
        free(obj);
        return hr;
    }

    hr = opc_part_uri_init(obj, NULL, FALSE, uri);
    IUri_Release(uri);
    if (FAILED(hr))
    {
        WARN("Failed to init uri, hr %#lx.\n", hr);
        free(uri);
        return hr;
    }

    *out = (IOpcUri *)&obj->IOpcPartUri_iface;
    TRACE("Created part uri %p.\n", *out);
    return S_OK;
}
