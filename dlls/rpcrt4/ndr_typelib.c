/*
 * Type library proxy/stub implementation
 *
 * Copyright 2018 Zebediah Figura
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
#include "oaidl.h"
#include "rpcproxy.h"
#include "wine/debug.h"
#include "wine/heap.h"

#include "cpsf.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

struct typelib_proxy
{
    StdProxyImpl proxy;
};

static ULONG WINAPI typelib_proxy_Release(IRpcProxyBuffer *iface)
{
    struct typelib_proxy *proxy = CONTAINING_RECORD(iface, struct typelib_proxy, proxy.IRpcProxyBuffer_iface);
    ULONG refcount = InterlockedDecrement(&proxy->proxy.RefCount);

    TRACE("(%p) decreasing refs to %d\n", proxy, refcount);

    if (!refcount)
    {
        heap_free(proxy);
    }
    return refcount;
}

static const IRpcProxyBufferVtbl typelib_proxy_vtbl =
{
    StdProxy_QueryInterface,
    StdProxy_AddRef,
    typelib_proxy_Release,
    StdProxy_Connect,
    StdProxy_Disconnect,
};

static HRESULT typelib_proxy_init(struct typelib_proxy *proxy, IUnknown *outer,
        IRpcProxyBuffer **proxy_buffer, void **out)
{
    if (!outer) outer = (IUnknown *)&proxy->proxy;

    proxy->proxy.IRpcProxyBuffer_iface.lpVtbl = &typelib_proxy_vtbl;
    proxy->proxy.RefCount = 1;
    proxy->proxy.pUnkOuter = outer;

    *proxy_buffer = &proxy->proxy.IRpcProxyBuffer_iface;

    return E_NOTIMPL;
}

HRESULT WINAPI CreateProxyFromTypeInfo(ITypeInfo *typeinfo, IUnknown *outer,
        REFIID iid, IRpcProxyBuffer **proxy_buffer, void **out)
{
    struct typelib_proxy *proxy;
    HRESULT hr;

    TRACE("typeinfo %p, outer %p, iid %s, proxy %p, out %p.\n",
            typeinfo, outer, debugstr_guid(iid), proxy_buffer, out);

    if (!(proxy = heap_alloc_zero(sizeof(*proxy))))
    {
        ERR("Failed to allocate proxy object.\n");
        return E_OUTOFMEMORY;
    }

    hr = typelib_proxy_init(proxy, outer, proxy_buffer, out);
    if (FAILED(hr))
        heap_free(proxy);

    return hr;
}
