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
#include "sxs_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(sxs);

struct name
{
    IAssemblyName IAssemblyName_iface;
    LONG   refs;
    WCHAR *name;
    WCHAR *arch;
    WCHAR *token;
    WCHAR *type;
    WCHAR *version;
    WCHAR *language;
};

static inline struct name *impl_from_IAssemblyName( IAssemblyName *iface )
{
    return CONTAINING_RECORD( iface, struct name, IAssemblyName_iface );
}

static WCHAR *escape( const WCHAR *str )
{
    static const WCHAR valid_chars[] = L"-._ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    WCHAR *dst, *ret;

    if (!str) return NULL;
    if (!(ret = malloc( (wcslen(str) * 8 + 1) * sizeof(WCHAR) ))) return NULL;
    for (dst = ret; *str; str++)
    {
        if (wcschr( valid_chars, *str )) *dst++ = *str;
        else switch (*str)
        {
        case '<':  wcscpy( dst, L"&lt;" ); dst += 4; break;
        case '>':  wcscpy( dst, L"&gt;" ); dst += 4; break;
        case '&':  wcscpy( dst, L"&amp;" ); dst += 5; break;
        case '"':  wcscpy( dst, L"&quot;" ); dst += 6; break;
        case '\'': wcscpy( dst, L"&apos;" ); dst += 6; break;
        default:   dst += swprintf( dst, 9, L"&#x%x;", *str ); break;
        }
    }
    *dst = 0;
    return ret;
}

static WCHAR *unescape( const WCHAR *str, size_t len )
{
    WCHAR *ret, *dst, *end;
    size_t j, i = 0;

    if (!(dst = ret = malloc( (len + 1) * sizeof(WCHAR) ))) return NULL;

    while (i < len)
    {
        if (str[i] != '&')
        {
            *dst++ = str[i++];
            continue;
        }
        if (!wcsncmp( str + i, L"&lt;", 4 )) { *dst++ = '<'; i += 4; }
        else if (!wcsncmp( str + i, L"&gt;", 4 )) { *dst++ = '>'; i += 4; }
        else if (!wcsncmp( str + i, L"&amp;", 5 )) { *dst++ = '&'; i += 5; }
        else if (!wcsncmp( str + i, L"&quot;", 6 )) { *dst++ = '"'; i += 6; }
        else if (!wcsncmp( str + i, L"&apos;", 6 )) { *dst++ = '\''; i += 6; }
        else if (i + 1 < len && str[i + 1] == '#')
        {
            int base = 10;

            i += 2;
            if (str[i] == 'x' || str[i] == 'X')
            {
                i++;
                base = 16;
            }
            *dst++ = wcstoul( str + i, &end, base );
            if (*end != ';') goto error;
            i = end + 1 - str;
        }
        else
        {
            for (j = i + 1; j < len; j++) if (str[j] == ';') break;
            if (j - i > 2) goto error;
            i = j + 1;
        }
    }
    *dst = 0;
    return ret;

error:
    free( ret );
    return NULL;
}

static HRESULT WINAPI name_QueryInterface(
    IAssemblyName *iface,
    REFIID riid,
    void **ret_iface )
{
    TRACE("%p, %s, %p\n", iface, debugstr_guid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID( riid, &IID_IUnknown ) ||
        IsEqualIID( riid, &IID_IAssemblyName ))
    {
        IAssemblyName_AddRef( iface );
        *ret_iface = iface;
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
        free( name->name );
        free( name->arch );
        free( name->token );
        free( name->type );
        free( name->version );
        free( name->language );
        free( name );
    }
    return refs;
}

static HRESULT WINAPI name_SetProperty(
    IAssemblyName *iface,
    DWORD id,
    LPVOID property,
    DWORD size )
{
    FIXME("%p, %ld, %p, %ld\n", iface, id, property, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI name_GetProperty(
    IAssemblyName *iface,
    DWORD id,
    LPVOID buffer,
    LPDWORD buflen )
{
    FIXME("%p, %ld, %p, %p\n", iface, id, buffer, buflen);
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
    struct name *name = impl_from_IAssemblyName( iface );
    unsigned int len;
    WCHAR *escname, *arch, *token, *type, *version, *language;
    HRESULT ret = S_OK;

    TRACE("%p, %p, %p, 0x%08lx\n", iface, buffer, buflen, flags);

    if (!buflen || flags) return E_INVALIDARG;

    escname  = escape( name->name );
    arch     = escape( name->arch );
    token    = escape( name->token );
    type     = escape( name->type );
    version  = escape( name->version );
    language = escape( name->language );

    len = lstrlenW( escname ) + 1;
    if (language) len += lstrlenW( L"language" ) + lstrlenW( language ) + 4;
    if (arch)     len += lstrlenW( L"processorArchitecture" ) + lstrlenW( arch ) + 4;
    if (token)    len += lstrlenW( L"publicKeyToken" ) + lstrlenW( token ) + 4;
    if (type)     len += lstrlenW( L"type" ) + lstrlenW( type ) + 4;
    if (version)  len += lstrlenW( L"version" ) + lstrlenW( version ) + 4;
    if (len > *buflen)
    {
        *buflen = len;
        ret = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
    }
    else
    {
        lstrcpyW( buffer, escname );
        len = lstrlenW( buffer );
        if (language) len += swprintf( buffer + len, *buflen - len, L",language=\"%s\"", language );
        if (arch)     len += swprintf( buffer + len, *buflen - len, L",processorArchitecture=\"%s\"", arch );
        if (token)    len += swprintf( buffer + len, *buflen - len, L",publicKeyToken=\"%s\"", token );
        if (type)     len += swprintf( buffer + len, *buflen - len, L",type=\"%s\"", type );
        if (version)  len += swprintf( buffer + len, *buflen - len, L",version=\"%s\"", version );
    }
    free( escname );
    free( arch );
    free( token );
    free( type );
    free( version );
    free( language );
    return ret;
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
    FIXME("%p, %s, %p, %p, %s, %s, %p, %ld, %p\n", iface,
          debugstr_guid(riid), pUnkReserved1, pUnkReserved2,
          debugstr_w(szReserved), wine_dbgstr_longlong(llReserved),
          pvReserved, cbReserved, ppReserved);
    return E_NOTIMPL;
}

const WCHAR *get_name_attribute( IAssemblyName *iface, enum name_attr_id id )
{
    struct name *name = impl_from_IAssemblyName( iface );

    switch (id)
    {
    case NAME_ATTR_ID_NAME:    return name->name;
    case NAME_ATTR_ID_ARCH:    return name->arch;
    case NAME_ATTR_ID_TOKEN:   return name->token;
    case NAME_ATTR_ID_TYPE:    return name->type;
    case NAME_ATTR_ID_VERSION: return name->version;
    case NAME_ATTR_ID_LANGUAGE: return name->language;
    default:
        ERR("unhandled name attribute %u\n", id);
        break;
    }
    return NULL;
}

static HRESULT WINAPI name_GetName(
    IAssemblyName *iface,
    LPDWORD buflen,
    WCHAR *buffer )
{
    const WCHAR *name;
    int len;

    TRACE("%p, %p, %p\n", iface, buflen, buffer);

    if (!buflen || !buffer) return E_INVALIDARG;

    name = get_name_attribute( iface, NAME_ATTR_ID_NAME );
    len = lstrlenW( name ) + 1;
    if (len > *buflen)
    {
        *buflen = len;
        return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
    }
    lstrcpyW( buffer, name );
    *buflen = len + 3;
    return S_OK;
}

static HRESULT parse_version( WCHAR *version, DWORD *high, DWORD *low )
{
    WORD ver[4];
    WCHAR *p, *q;
    unsigned int i;

    memset( ver, 0, sizeof(ver) );
    for (i = 0, p = version; i < 4; i++)
    {
        if (!*p) break;
        q = wcschr( p, '.' );
        if (q) *q = 0;
        ver[i] = wcstol( p, NULL, 10 );
        if (!q && i < 3) break;
        p = q + 1;
    }
    *high = (ver[0] << 16) + ver[1];
    *low = (ver[2] << 16) + ver[3];
    return S_OK;
}

static HRESULT WINAPI name_GetVersion(
    IAssemblyName *iface,
    LPDWORD high,
    LPDWORD low )
{
    struct name *name = impl_from_IAssemblyName( iface );
    WCHAR *version;
    HRESULT hr;

    TRACE("%p, %p, %p\n", iface, high, low);

    if (!name->version) return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
    if (!(version = wcsdup( name->version ))) return E_OUTOFMEMORY;
    hr = parse_version( version, high, low );
    free( version );
    return hr;
}

static HRESULT WINAPI name_IsEqual(
    IAssemblyName *name1,
    IAssemblyName *name2,
    DWORD flags )
{
    FIXME("%p, %p, 0x%08lx\n", name1, name2, flags);
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

static WCHAR *parse_value( const WCHAR *str, unsigned int *len )
{
    const WCHAR *p = str;

    if (*p++ != '\"') return NULL;
    while (*p && *p != '\"') p++;
    if (!*p) return NULL;

    *len = p - str;
    return unescape( str + 1, *len - 1 );
}

static HRESULT parse_displayname( struct name *name, const WCHAR *displayname )
{
    const WCHAR *p, *q;
    unsigned int len;

    p = q = displayname;
    while (*q && *q != ',') q++;
    len = q - p;
    if (!(name->name = unescape( p, len ))) return E_INVALIDARG;
    if (!*q) return S_OK;

    for (;;)
    {
        p = ++q;
        while (*q && *q != '=') q++;
        if (!*q) return E_INVALIDARG;
        len = q - p;
        if (len == ARRAY_SIZE(L"processorArchitecture") - 1 && !memcmp( p, L"processorArchitecture", len * sizeof(WCHAR) ))
        {
            p = ++q;
            if (!(name->arch = parse_value( p, &len ))) return E_INVALIDARG;
            q += len;
        }
        else if (len == ARRAY_SIZE(L"publicKeyToken") - 1 && !memcmp( p, L"publicKeyToken", len * sizeof(WCHAR) ))
        {
            p = ++q;
            if (!(name->token = parse_value( p, &len ))) return E_INVALIDARG;
            q += len;
        }
        else if (len == ARRAY_SIZE(L"type") - 1 && !memcmp( p, L"type", len * sizeof(WCHAR) ))
        {
            p = ++q;
            if (!(name->type = parse_value( p, &len ))) return E_INVALIDARG;
            q += len;
        }
        else if (len == ARRAY_SIZE(L"version") - 1 && !memcmp( p, L"version", len * sizeof(WCHAR) ))
        {
            p = ++q;
            if (!(name->version = parse_value( p, &len ))) return E_INVALIDARG;
            q += len;
        }
        else if (len == ARRAY_SIZE(L"language") - 1 && !memcmp( p, L"language", len * sizeof(WCHAR) ))
        {
            p = ++q;
            if (!(name->language = parse_value( p, &len ))) return E_INVALIDARG;
            q += len;
        }
        else return HRESULT_FROM_WIN32( ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME );
        while (*q && *q != ',') q++;
        if (!*q) break;
    }
    return S_OK;
}

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
    HRESULT hr;

    TRACE("%p, %s, 0x%08lx, %p\n", obj, debugstr_w(assembly), flags, reserved);

    if (!obj) return E_INVALIDARG;

    *obj = NULL;
    if (!assembly || !assembly[0] || flags != CANOF_PARSE_DISPLAY_NAME)
        return E_INVALIDARG;

    if (!(name = calloc(1, sizeof(*name) )))
        return E_OUTOFMEMORY;

    name->IAssemblyName_iface.lpVtbl = &name_vtbl;
    name->refs = 1;

    hr = parse_displayname( name, assembly );
    if (hr != S_OK)
    {
        free( name->name );
        free( name->arch );
        free( name->token );
        free( name->type );
        free( name->version );
        free( name->language );
        free( name );
        return hr;
    }
    *obj = &name->IAssemblyName_iface;
    return S_OK;
}
