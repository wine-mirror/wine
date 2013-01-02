/*
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

#define COBJMACROS

#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "wbemcli.h"
#include "wmiutils.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wmiutils_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmiutils);

struct path
{
    IWbemPath IWbemPath_iface;
    LONG    refs;
    WCHAR  *text;
    int     len_text;
    WCHAR  *server;
    int     len_server;
    WCHAR **namespaces;
    int    *len_namespaces;
    int     num_namespaces;
    WCHAR  *class;
    int     len_class;
};

static void init_path( struct path *path )
{
    path->text           = NULL;
    path->len_text       = 0;
    path->server         = NULL;
    path->len_server     = 0;
    path->namespaces     = NULL;
    path->len_namespaces = NULL;
    path->num_namespaces = 0;
    path->class          = NULL;
    path->len_class      = 0;
}

static void clear_path( struct path *path )
{
    heap_free( path->text );
    heap_free( path->server );
    heap_free( path->namespaces );
    heap_free( path->len_namespaces );
    heap_free( path->class );
    init_path( path );
}

static inline struct path *impl_from_IWbemPath( IWbemPath *iface )
{
    return CONTAINING_RECORD(iface, struct path, IWbemPath_iface);
}

static ULONG WINAPI path_AddRef(
    IWbemPath *iface )
{
    struct path *path = impl_from_IWbemPath( iface );
    return InterlockedIncrement( &path->refs );
}

static ULONG WINAPI path_Release(
    IWbemPath *iface )
{
    struct path *path = impl_from_IWbemPath( iface );
    LONG refs = InterlockedDecrement( &path->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", path);
        clear_path( path );
        heap_free( path );
    }
    return refs;
}

static HRESULT WINAPI path_QueryInterface(
    IWbemPath *iface,
    REFIID riid,
    void **ppvObject )
{
    struct path *path = impl_from_IWbemPath( iface );

    TRACE("%p, %s, %p\n", path, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IWbemPath ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IWbemPath_AddRef( iface );
    return S_OK;
}

static HRESULT parse_text( struct path *path, ULONG mode, const WCHAR *text )
{
    HRESULT hr = E_OUTOFMEMORY;
    const WCHAR *p, *q;
    unsigned int i, len;

    p = q = text;
    if ((p[0] == '\\' && p[1] == '\\') || (p[0] == '/' && p[1] == '/'))
    {
        p += 2;
        q = p;
        while (*q && *q != '\\' && *q != '/') q++;
        len = q - p;
        if (!(path->server = heap_alloc( (len + 1) * sizeof(WCHAR) ))) goto done;
        memcpy( path->server, p, len * sizeof(WCHAR) );
        path->server[len] = 0;
        path->len_server = len;
    }
    p = q;
    while (*q && *q != ':')
    {
        if (*q == '\\' || *q == '/') path->num_namespaces++;
        q++;
    }
    if (path->num_namespaces)
    {
        if (!(path->namespaces = heap_alloc( path->num_namespaces * sizeof(WCHAR *) ))) goto done;
        if (!(path->len_namespaces = heap_alloc( path->num_namespaces * sizeof(int) ))) goto done;

        i = 0;
        q = p;
        while (*q && *q != ':')
        {
            if (*q == '\\' || *q == '/')
            {
                p = q + 1;
                while (*p && *p != '\\' && *p != '/' && *p != ':') p++;
                len = p - q - 1;
                if (!(path->namespaces[i] = heap_alloc( (len + 1) * sizeof(WCHAR) ))) goto done;
                memcpy( path->namespaces[i], q + 1, len * sizeof(WCHAR) );
                path->namespaces[i][len] = 0;
                path->len_namespaces[i] = len;
                i++;
            }
            q++;
        }
    }
    if (*q == ':') q++;
    p = q;
    while (*q && *q != '.') q++;
    len = q - p;
    if (!(path->class = heap_alloc( (len + 1) * sizeof(WCHAR) ))) goto done;
    memcpy( path->class, p, len * sizeof(WCHAR) );
    path->class[len] = 0;
    path->len_class = len;

    if (*q == '.') FIXME("handle key list\n");
    hr = S_OK;

done:
    if (hr != S_OK) clear_path( path );
    return hr;
}

static HRESULT WINAPI path_SetText(
    IWbemPath *iface,
    ULONG uMode,
    LPCWSTR pszPath)
{
    struct path *path = impl_from_IWbemPath( iface );
    HRESULT hr;
    int len;

    TRACE("%p, %u, %s\n", iface, uMode, debugstr_w(pszPath));

    if (!uMode || !pszPath) return WBEM_E_INVALID_PARAMETER;

    clear_path( path );
    if ((hr = parse_text( path, uMode, pszPath )) != S_OK) return hr;

    len = strlenW( pszPath );
    if (!(path->text = heap_alloc( (len + 1) * sizeof(WCHAR) )))
    {
        clear_path( path );
        return E_OUTOFMEMORY;
    }
    strcpyW( path->text, pszPath );
    path->len_text = len;
    return S_OK;
}

static HRESULT WINAPI path_GetText(
    IWbemPath *iface,
    LONG lFlags,
    ULONG *puBufferLength,
    LPWSTR pszText)
{
    struct path *path = impl_from_IWbemPath( iface );

    TRACE("%p, 0x%x, %p, %p\n", iface, lFlags, puBufferLength, pszText);

    if (!puBufferLength || !pszText) return WBEM_E_INVALID_PARAMETER;

    if (lFlags != WBEMPATH_GET_ORIGINAL)
    {
        FIXME("flags 0x%x not supported\n", lFlags);
        return WBEM_E_INVALID_PARAMETER;
    }
    if (*puBufferLength < path->len_text + 1)
    {
        *puBufferLength = path->len_text + 1;
        return S_OK;
    }
    if (pszText) strcpyW( pszText, path->text );
    *puBufferLength = path->len_text + 1;
    return S_OK;
}

static HRESULT WINAPI path_GetInfo(
    IWbemPath *iface,
    ULONG uRequestedInfo,
    ULONGLONG *puResponse)
{
    FIXME("%p, %d, %p\n", iface, uRequestedInfo, puResponse);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_SetServer(
    IWbemPath *iface,
    LPCWSTR Name)
{
    FIXME("%p, %s\n", iface, debugstr_w(Name));
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetServer(
    IWbemPath *iface,
    ULONG *puNameBufLength,
    LPWSTR pName)
{
    FIXME("%p, %p, %p\n", iface, puNameBufLength, pName);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetNamespaceCount(
    IWbemPath *iface,
    ULONG *puCount)
{
    struct path *path = impl_from_IWbemPath( iface );

    TRACE("%p, %p\n", iface, puCount);

    if (!puCount) return WBEM_E_INVALID_PARAMETER;
    *puCount = path->num_namespaces;
    return S_OK;
}

static HRESULT WINAPI path_SetNamespaceAt(
    IWbemPath *iface,
    ULONG uIndex,
    LPCWSTR pszName)
{
    FIXME("%p, %u, %s\n", iface, uIndex, debugstr_w(pszName));
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetNamespaceAt(
    IWbemPath *iface,
    ULONG uIndex,
    ULONG *puNameBufLength,
    LPWSTR pName)
{
    FIXME("%p, %u, %p, %p\n", iface, uIndex, puNameBufLength, pName);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_RemoveNamespaceAt(
    IWbemPath *iface,
    ULONG uIndex)
{
    FIXME("%p, %u\n", iface, uIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_RemoveAllNamespaces(
        IWbemPath *iface)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetScopeCount(
        IWbemPath *iface,
        ULONG *puCount)
{
    FIXME("%p, %p\n", iface, puCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_SetScope(
    IWbemPath *iface,
    ULONG uIndex,
    LPWSTR pszClass)
{
    FIXME("%p, %u, %s\n", iface, uIndex, debugstr_w(pszClass));
    return E_NOTIMPL;
}

static HRESULT WINAPI path_SetScopeFromText(
    IWbemPath *iface,
    ULONG uIndex,
    LPWSTR pszText)
{
    FIXME("%p, %u, %s\n", iface, uIndex, debugstr_w(pszText));
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetScope(
    IWbemPath *iface,
    ULONG uIndex,
    ULONG *puClassNameBufSize,
    LPWSTR pszClass,
    IWbemPathKeyList **pKeyList)
{
    FIXME("%p, %u, %p, %p, %p\n", iface, uIndex, puClassNameBufSize, pszClass, pKeyList);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetScopeAsText(
    IWbemPath *iface,
    ULONG uIndex,
    ULONG *puTextBufSize,
    LPWSTR pszText)
{
    FIXME("%p, %u, %p, %p\n", iface, uIndex, puTextBufSize, pszText);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_RemoveScope(
    IWbemPath *iface,
    ULONG uIndex)
{
    FIXME("%p, %u\n", iface, uIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_RemoveAllScopes(
    IWbemPath *iface)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_SetClassName(
    IWbemPath *iface,
    LPCWSTR Name)
{
    FIXME("%p, %s\n", iface, debugstr_w(Name));
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetClassName(
    IWbemPath *iface,
    ULONG *puBufferLength,
    LPWSTR pszName)
{
    FIXME("%p,%p, %p\n", iface, puBufferLength, pszName);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetKeyList(
    IWbemPath *iface,
    IWbemPathKeyList **pOut)
{
    FIXME("%p, %p\n", iface, pOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_CreateClassPart(
    IWbemPath *iface,
    LONG lFlags,
    LPCWSTR Name)
{
    FIXME("%p, 0x%x, %s\n", iface, lFlags, debugstr_w(Name));
    return E_NOTIMPL;
}

static HRESULT WINAPI path_DeleteClassPart(
    IWbemPath *iface,
    LONG lFlags)
{
    FIXME("%p, 0x%x\n", iface, lFlags);
    return E_NOTIMPL;
}

static BOOL WINAPI path_IsRelative(
    IWbemPath *iface,
    LPWSTR wszMachine,
    LPWSTR wszNamespace)
{
    FIXME("%p, %s, %s\n", iface, debugstr_w(wszMachine), debugstr_w(wszNamespace));
    return E_NOTIMPL;
}

static BOOL WINAPI path_IsRelativeOrChild(
    IWbemPath *iface,
    LPWSTR wszMachine,
    LPWSTR wszNamespace,
    LONG lFlags)
{
    FIXME("%p, %s, %s, 0x%x\n", iface, debugstr_w(wszMachine), debugstr_w(wszNamespace), lFlags);
    return E_NOTIMPL;
}

static BOOL WINAPI path_IsLocal(
    IWbemPath *iface,
    LPCWSTR wszMachine)
{
    FIXME("%p, %s\n", iface, debugstr_w(wszMachine));
    return E_NOTIMPL;
}

static BOOL WINAPI path_IsSameClassName(
    IWbemPath *iface,
    LPCWSTR wszClass)
{
    FIXME("%p, %s\n", iface, debugstr_w(wszClass));
    return E_NOTIMPL;
}

static const struct IWbemPathVtbl path_vtbl =
{
    path_QueryInterface,
    path_AddRef,
    path_Release,
    path_SetText,
    path_GetText,
    path_GetInfo,
    path_SetServer,
    path_GetServer,
    path_GetNamespaceCount,
    path_SetNamespaceAt,
    path_GetNamespaceAt,
    path_RemoveNamespaceAt,
    path_RemoveAllNamespaces,
    path_GetScopeCount,
    path_SetScope,
    path_SetScopeFromText,
    path_GetScope,
    path_GetScopeAsText,
    path_RemoveScope,
    path_RemoveAllScopes,
    path_SetClassName,
    path_GetClassName,
    path_GetKeyList,
    path_CreateClassPart,
    path_DeleteClassPart,
    path_IsRelative,
    path_IsRelativeOrChild,
    path_IsLocal,
    path_IsSameClassName
};

HRESULT WbemPath_create( IUnknown *pUnkOuter, LPVOID *ppObj )
{
    struct path *path;

    TRACE("%p, %p\n", pUnkOuter, ppObj);

    if (!(path = heap_alloc( sizeof(*path) ))) return E_OUTOFMEMORY;

    path->IWbemPath_iface.lpVtbl = &path_vtbl;
    path->refs = 1;
    init_path( path );

    *ppObj = &path->IWbemPath_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
