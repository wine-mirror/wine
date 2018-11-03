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
#define USE_STUBLESS_PROXY
#include "rpcproxy.h"
#include "wine/debug.h"
#include "wine/heap.h"

#include "cpsf.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static HRESULT build_format_strings(ITypeInfo *typeinfo, WORD funcs,
        const unsigned char **type_ret, const unsigned char **proc_ret,
        unsigned short **offset_ret)
{
    return E_NOTIMPL;
}

/* Common helper for Create{Proxy,Stub}FromTypeInfo(). */
static HRESULT get_iface_info(ITypeInfo *typeinfo, WORD *funcs, WORD *parentfuncs)
{
    TYPEATTR *typeattr;
    ITypeLib *typelib;
    TLIBATTR *libattr;
    SYSKIND syskind;
    HRESULT hr;

    hr = ITypeInfo_GetContainingTypeLib(typeinfo, &typelib, NULL);
    if (FAILED(hr))
        return hr;

    hr = ITypeLib_GetLibAttr(typelib, &libattr);
    if (FAILED(hr))
    {
        ITypeLib_Release(typelib);
        return hr;
    }
    syskind = libattr->syskind;
    ITypeLib_ReleaseTLibAttr(typelib, libattr);
    ITypeLib_Release(typelib);

    hr = ITypeInfo_GetTypeAttr(typeinfo, &typeattr);
    if (FAILED(hr))
        return hr;
    *funcs = typeattr->cFuncs;
    *parentfuncs = typeattr->cbSizeVft / (syskind == SYS_WIN64 ? 8 : 4) - *funcs;
    ITypeInfo_ReleaseTypeAttr(typeinfo, typeattr);

    return S_OK;
}

static void init_stub_desc(MIDL_STUB_DESC *desc)
{
    desc->pfnAllocate = NdrOleAllocate;
    desc->pfnFree = NdrOleFree;
    desc->Version = 0x50002;
    /* type format string is initialized with proc format string and offset table */
}

struct typelib_proxy
{
    StdProxyImpl proxy;
    IID iid;
    MIDL_STUB_DESC stub_desc;
    MIDL_STUBLESS_PROXY_INFO proxy_info;
    CInterfaceProxyVtbl *proxy_vtbl;
    unsigned short *offset_table;
};

static ULONG WINAPI typelib_proxy_Release(IRpcProxyBuffer *iface)
{
    struct typelib_proxy *proxy = CONTAINING_RECORD(iface, struct typelib_proxy, proxy.IRpcProxyBuffer_iface);
    ULONG refcount = InterlockedDecrement(&proxy->proxy.RefCount);

    TRACE("(%p) decreasing refs to %d\n", proxy, refcount);

    if (!refcount)
    {
        if (proxy->proxy.pChannel)
            IRpcProxyBuffer_Disconnect(&proxy->proxy.IRpcProxyBuffer_iface);
        heap_free((void *)proxy->stub_desc.pFormatTypes);
        heap_free((void *)proxy->proxy_info.ProcFormatString);
        heap_free(proxy->offset_table);
        heap_free(proxy->proxy_vtbl);
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
        ULONG count, IRpcProxyBuffer **proxy_buffer, void **out)
{
    if (!fill_stubless_table((IUnknownVtbl *)proxy->proxy_vtbl->Vtbl, count))
        return E_OUTOFMEMORY;

    if (!outer) outer = (IUnknown *)&proxy->proxy;

    proxy->proxy.IRpcProxyBuffer_iface.lpVtbl = &typelib_proxy_vtbl;
    proxy->proxy.PVtbl = proxy->proxy_vtbl->Vtbl;
    proxy->proxy.RefCount = 1;
    proxy->proxy.piid = proxy->proxy_vtbl->header.piid;
    proxy->proxy.pUnkOuter = outer;

    *proxy_buffer = &proxy->proxy.IRpcProxyBuffer_iface;
    *out = &proxy->proxy.PVtbl;
    IUnknown_AddRef((IUnknown *)*out);

    return S_OK;
}

HRESULT WINAPI CreateProxyFromTypeInfo(ITypeInfo *typeinfo, IUnknown *outer,
        REFIID iid, IRpcProxyBuffer **proxy_buffer, void **out)
{
    struct typelib_proxy *proxy;
    WORD funcs, parentfuncs, i;
    HRESULT hr;

    TRACE("typeinfo %p, outer %p, iid %s, proxy_buffer %p, out %p.\n",
            typeinfo, outer, debugstr_guid(iid), proxy_buffer, out);

    hr = get_iface_info(typeinfo, &funcs, &parentfuncs);
    if (FAILED(hr))
        return hr;

    if (!(proxy = heap_alloc_zero(sizeof(*proxy))))
    {
        ERR("Failed to allocate proxy object.\n");
        return E_OUTOFMEMORY;
    }

    init_stub_desc(&proxy->stub_desc);
    proxy->proxy_info.pStubDesc = &proxy->stub_desc;

    proxy->proxy_vtbl = heap_alloc_zero(sizeof(proxy->proxy_vtbl->header) + (funcs + parentfuncs) * sizeof(void *));
    if (!proxy->proxy_vtbl)
    {
        ERR("Failed to allocate proxy vtbl.\n");
        heap_free(proxy);
        return E_OUTOFMEMORY;
    }
    proxy->proxy_vtbl->header.pStublessProxyInfo = &proxy->proxy_info;
    proxy->iid = *iid;
    proxy->proxy_vtbl->header.piid = &proxy->iid;
    for (i = 0; i < funcs; i++)
        proxy->proxy_vtbl->Vtbl[3 + i] = (void *)-1;

    hr = build_format_strings(typeinfo, funcs, &proxy->stub_desc.pFormatTypes,
            &proxy->proxy_info.ProcFormatString, &proxy->offset_table);
    if (FAILED(hr))
    {
        heap_free(proxy->proxy_vtbl);
        heap_free(proxy);
        return hr;
    }
    proxy->proxy_info.FormatStringOffset = &proxy->offset_table[-3];

    hr = typelib_proxy_init(proxy, outer, funcs + parentfuncs, proxy_buffer, out);
    if (FAILED(hr))
    {
        heap_free((void *)proxy->stub_desc.pFormatTypes);
        heap_free((void *)proxy->proxy_info.ProcFormatString);
        heap_free((void *)proxy->offset_table);
        heap_free(proxy->proxy_vtbl);
        heap_free(proxy);
    }

    return hr;
}

struct typelib_stub
{
    CStdStubBuffer stub;
    IID iid;
    MIDL_STUB_DESC stub_desc;
    MIDL_SERVER_INFO server_info;
    CInterfaceStubVtbl stub_vtbl;
    unsigned short *offset_table;
};

static ULONG WINAPI typelib_stub_Release(IRpcStubBuffer *iface)
{
    struct typelib_stub *stub = CONTAINING_RECORD(iface, struct typelib_stub, stub);
    ULONG refcount = InterlockedDecrement(&stub->stub.RefCount);

    TRACE("(%p) decreasing refs to %d\n", stub, refcount);

    if (!refcount)
    {
        /* test_Release shows that native doesn't call Disconnect here.
           We'll leave it in for the time being. */
        IRpcStubBuffer_Disconnect(iface);

        heap_free((void *)stub->stub_desc.pFormatTypes);
        heap_free((void *)stub->server_info.ProcString);
        heap_free(stub->offset_table);
        heap_free(stub);
    }

    return refcount;
}

static HRESULT typelib_stub_init(struct typelib_stub *stub, IUnknown *server,
        IRpcStubBuffer **stub_buffer)
{
    HRESULT hr;

    hr = IUnknown_QueryInterface(server, stub->stub_vtbl.header.piid,
            (void **)&stub->stub.pvServerObject);
    if (FAILED(hr))
    {
        WARN("Failed to get interface %s, hr %#x.\n",
                debugstr_guid(stub->stub_vtbl.header.piid), hr);
        stub->stub.pvServerObject = server;
        IUnknown_AddRef(server);
    }

    stub->stub.lpVtbl = &stub->stub_vtbl.Vtbl;
    stub->stub.RefCount = 1;

    *stub_buffer = (IRpcStubBuffer *)&stub->stub;
    return S_OK;
}

HRESULT WINAPI CreateStubFromTypeInfo(ITypeInfo *typeinfo, REFIID iid,
        IUnknown *server, IRpcStubBuffer **stub_buffer)
{
    struct typelib_stub *stub;
    WORD funcs, parentfuncs;
    HRESULT hr;

    TRACE("typeinfo %p, iid %s, server %p, stub_buffer %p.\n",
            typeinfo, debugstr_guid(iid), server, stub_buffer);

    hr = get_iface_info(typeinfo, &funcs, &parentfuncs);
    if (FAILED(hr))
        return hr;

    if (!(stub = heap_alloc_zero(sizeof(*stub))))
    {
        ERR("Failed to allocate stub object.\n");
        return E_OUTOFMEMORY;
    }

    init_stub_desc(&stub->stub_desc);
    stub->server_info.pStubDesc = &stub->stub_desc;

    hr = build_format_strings(typeinfo, funcs, &stub->stub_desc.pFormatTypes,
            &stub->server_info.ProcString, &stub->offset_table);
    if (FAILED(hr))
    {
        heap_free(stub);
        return hr;
    }
    stub->server_info.FmtStringOffset = &stub->offset_table[-3];

    stub->iid = *iid;
    stub->stub_vtbl.header.piid = &stub->iid;
    stub->stub_vtbl.header.pServerInfo = &stub->server_info;
    stub->stub_vtbl.header.DispatchTableCount = funcs + parentfuncs;
    stub->stub_vtbl.Vtbl = CStdStubBuffer_Vtbl;
    stub->stub_vtbl.Vtbl.Release = typelib_stub_Release;

    hr = typelib_stub_init(stub, server, stub_buffer);
    if (FAILED(hr))
    {
        heap_free((void *)stub->stub_desc.pFormatTypes);
        heap_free((void *)stub->server_info.ProcString);
        heap_free(stub->offset_table);
        heap_free(stub);
    }

    return hr;
}
