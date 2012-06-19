/*
 * Copyright 2009 Hans Leidekker for CodeWeavers
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
#include "wine/unicode.h"
#include "wbemprox_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

typedef struct
{
    IWbemLocator IWbemLocator_iface;
    LONG refs;
} wbem_locator;

static inline wbem_locator *impl_from_IWbemLocator( IWbemLocator *iface )
{
    return CONTAINING_RECORD(iface, wbem_locator, IWbemLocator_iface);
}

static ULONG WINAPI wbem_locator_AddRef(
    IWbemLocator *iface )
{
    wbem_locator *wl = impl_from_IWbemLocator( iface );
    return InterlockedIncrement( &wl->refs );
}

static ULONG WINAPI wbem_locator_Release(
    IWbemLocator *iface )
{
    wbem_locator *wl = impl_from_IWbemLocator( iface );
    LONG refs = InterlockedDecrement( &wl->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", wl);
        heap_free( wl );
    }
    return refs;
}

static HRESULT WINAPI wbem_locator_QueryInterface(
    IWbemLocator *iface,
    REFIID riid,
    void **ppvObject )
{
    wbem_locator *This = impl_from_IWbemLocator( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IWbemLocator ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IWbemLocator_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI wbem_locator_ConnectServer(
    IWbemLocator *iface,
    const BSTR NetworkResource,
    const BSTR User,
    const BSTR Password,
    const BSTR Locale,
    LONG SecurityFlags,
    const BSTR Authority,
    IWbemContext *pCtx,
    IWbemServices **ppNamespace)
{
    HRESULT hr;

    TRACE("%p, %s, %s, %s, %s, 0x%08x, %s, %p, %p)\n", iface, debugstr_w(NetworkResource), debugstr_w(User),
          debugstr_w(Password), debugstr_w(Locale), SecurityFlags, debugstr_w(Authority), pCtx, ppNamespace);

    if (((NetworkResource[0] == '\\' && NetworkResource[1] == '\\') ||
         (NetworkResource[0] == '/' && NetworkResource[1] == '/')) && NetworkResource[2] != '.')
    {
        FIXME("remote computer not supported\n");
        return WBEM_E_TRANSPORT_FAILURE;
    }
    if (User || Password || Authority)
        FIXME("authentication not supported\n");
    if (Locale)
        FIXME("specific locale not supported\n");
    if (SecurityFlags)
        FIXME("unsupported flags\n");

    hr = WbemServices_create( NULL, (void **)ppNamespace );
    if (SUCCEEDED( hr ))
        return WBEM_NO_ERROR;

    return WBEM_E_FAILED;
}

static const IWbemLocatorVtbl wbem_locator_vtbl =
{
    wbem_locator_QueryInterface,
    wbem_locator_AddRef,
    wbem_locator_Release,
    wbem_locator_ConnectServer
};

HRESULT WbemLocator_create( IUnknown *pUnkOuter, LPVOID *ppObj )
{
    wbem_locator *wl;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    wl = heap_alloc( sizeof(*wl) );
    if (!wl) return E_OUTOFMEMORY;

    wl->IWbemLocator_iface.lpVtbl = &wbem_locator_vtbl;
    wl->refs = 1;

    *ppObj = &wl->IWbemLocator_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
