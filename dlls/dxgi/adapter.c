/*
 * Copyright 2008 Henri Verbeet for CodeWeavers
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
 *
 */

#include "config.h"
#include "wine/port.h"

#include "dxgi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE dxgi_adapter_QueryInterface(IWineDXGIAdapter *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IDXGIAdapter)
            || IsEqualGUID(riid, &IID_IWineDXGIAdapter))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_adapter_AddRef(IWineDXGIAdapter *iface)
{
    struct dxgi_adapter *This = (struct dxgi_adapter *)iface;
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_adapter_Release(IWineDXGIAdapter *iface)
{
    struct dxgi_adapter *This = (struct dxgi_adapter *)iface;
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_adapter_SetPrivateData(IWineDXGIAdapter *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    FIXME("iface %p, guid %s, data_size %u, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_SetPrivateDataInterface(IWineDXGIAdapter *iface,
        REFGUID guid, const IUnknown *object)
{
    FIXME("iface %p, guid %s, object %p stub!\n", iface, debugstr_guid(guid), object);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetPrivateData(IWineDXGIAdapter *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    FIXME("iface %p, guid %s, data_size %p, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetParent(IWineDXGIAdapter *iface, REFIID riid, void **parent)
{
    struct dxgi_adapter *This = (struct dxgi_adapter *)iface;

    TRACE("iface %p, riid %s, parent %p\n", iface, debugstr_guid(riid), parent);

    return IDXGIFactory_QueryInterface(This->parent, riid, parent);
}

/* IDXGIAdapter methods */

static HRESULT STDMETHODCALLTYPE dxgi_adapter_EnumOutputs(IWineDXGIAdapter *iface,
        UINT output_idx, IDXGIOutput **output)
{
    FIXME("iface %p, output_idx %u, output %p stub!\n", iface, output_idx, output);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetDesc(IWineDXGIAdapter *iface, DXGI_ADAPTER_DESC *desc)
{
    FIXME("iface %p, desc %p stub!\n", iface, desc);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_CheckInterfaceSupport(IWineDXGIAdapter *iface,
        REFGUID guid, LARGE_INTEGER *umd_version)
{
    FIXME("iface %p, guid %s, umd_version %p stub!\n", iface, debugstr_guid(guid), umd_version);

    return E_NOTIMPL;
}

/* IWineDXGIAdapter methods */

static UINT STDMETHODCALLTYPE dxgi_adapter_get_ordinal(IWineDXGIAdapter *iface)
{
    struct dxgi_adapter *This = (struct dxgi_adapter *)iface;

    TRACE("iface %p, returning %u\n", iface, This->ordinal);

    return This->ordinal;
}

const struct IWineDXGIAdapterVtbl dxgi_adapter_vtbl =
{
    /* IUnknown methods */
    dxgi_adapter_QueryInterface,
    dxgi_adapter_AddRef,
    dxgi_adapter_Release,
    /* IDXGIObject methods */
    dxgi_adapter_SetPrivateData,
    dxgi_adapter_SetPrivateDataInterface,
    dxgi_adapter_GetPrivateData,
    dxgi_adapter_GetParent,
    /* IDXGIAdapter methods */
    dxgi_adapter_EnumOutputs,
    dxgi_adapter_GetDesc,
    dxgi_adapter_CheckInterfaceSupport,
    /* IWineDXGIAdapter methods */
    dxgi_adapter_get_ordinal,
};
