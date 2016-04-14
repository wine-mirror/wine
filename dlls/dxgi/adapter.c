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

static inline struct dxgi_adapter *impl_from_IDXGIAdapter1(IDXGIAdapter1 *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_adapter, IDXGIAdapter1_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_QueryInterface(IDXGIAdapter1 *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IDXGIAdapter1)
            || IsEqualGUID(iid, &IID_IDXGIAdapter)
            || IsEqualGUID(iid, &IID_IDXGIObject)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_adapter_AddRef(IDXGIAdapter1 *iface)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter1(iface);
    ULONG refcount = InterlockedIncrement(&adapter->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_adapter_Release(IDXGIAdapter1 *iface)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter1(iface);
    ULONG refcount = InterlockedDecrement(&adapter->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        wined3d_private_store_cleanup(&adapter->private_store);
        IDXGIFactory1_Release(&adapter->factory->IDXGIFactory1_iface);
        HeapFree(GetProcessHeap(), 0, adapter);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_SetPrivateData(IDXGIAdapter1 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter1(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&adapter->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_SetPrivateDataInterface(IDXGIAdapter1 *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter1(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&adapter->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetPrivateData(IDXGIAdapter1 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter1(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&adapter->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetParent(IDXGIAdapter1 *iface, REFIID iid, void **parent)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter1(iface);

    TRACE("iface %p, iid %s, parent %p.\n", iface, debugstr_guid(iid), parent);

    return IDXGIFactory1_QueryInterface(&adapter->factory->IDXGIFactory1_iface, iid, parent);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_EnumOutputs(IDXGIAdapter1 *iface,
        UINT output_idx, IDXGIOutput **output)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter1(iface);
    struct dxgi_output *output_object;
    HRESULT hr;

    TRACE("iface %p, output_idx %u, output %p.\n", iface, output_idx, output);

    if (output_idx > 0)
    {
        *output = NULL;
        return DXGI_ERROR_NOT_FOUND;
    }

    if (FAILED(hr = dxgi_output_create(adapter, &output_object)))
    {
        *output = NULL;
        return hr;
    }

    *output = &output_object->IDXGIOutput_iface;

    TRACE("Returning output %p.\n", *output);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetDesc1(IDXGIAdapter1 *iface, DXGI_ADAPTER_DESC1 *desc)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter1(iface);
    struct wined3d_adapter_identifier adapter_id;
    char description[128];
    HRESULT hr;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
        return E_INVALIDARG;

    adapter_id.driver_size = 0;
    adapter_id.description = description;
    adapter_id.description_size = sizeof(description);
    adapter_id.device_name_size = 0;

    wined3d_mutex_lock();
    hr = wined3d_get_adapter_identifier(adapter->factory->wined3d, adapter->ordinal, 0, &adapter_id);
    wined3d_mutex_unlock();

    if (FAILED(hr))
        return hr;

    if (!MultiByteToWideChar(CP_ACP, 0, description, -1, desc->Description, 128))
    {
        DWORD err = GetLastError();
        ERR("Failed to translate description %s (%#x).\n", debugstr_a(description), err);
        hr = E_FAIL;
    }

    desc->VendorId = adapter_id.vendor_id;
    desc->DeviceId = adapter_id.device_id;
    desc->SubSysId = adapter_id.subsystem_id;
    desc->Revision = adapter_id.revision;
    desc->DedicatedVideoMemory = adapter_id.video_memory;
    desc->DedicatedSystemMemory = 0; /* FIXME */
    desc->SharedSystemMemory = 0; /* FIXME */
    memcpy(&desc->AdapterLuid, &adapter_id.adapter_luid, sizeof(desc->AdapterLuid));
    desc->Flags = 0;

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetDesc(IDXGIAdapter1 *iface, DXGI_ADAPTER_DESC *desc)
{
    DXGI_ADAPTER_DESC1 desc1;
    HRESULT hr;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
        return E_INVALIDARG;

    if (FAILED(hr = dxgi_adapter_GetDesc1(iface, &desc1)))
        return hr;
    memcpy(desc, &desc1, sizeof(*desc));

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_CheckInterfaceSupport(IDXGIAdapter1 *iface,
        REFGUID guid, LARGE_INTEGER *umd_version)
{
    static const D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_10_0;
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter1(iface);
    struct wined3d_adapter_identifier adapter_id;
    HRESULT hr;

    TRACE("iface %p, guid %s, umd_version %p.\n", iface, debugstr_guid(guid), umd_version);

    /* This method works only for D3D10 interfaces. */
    if (!(IsEqualGUID(guid, &IID_ID3D10Device)
            || IsEqualGUID(guid, &IID_ID3D10Device1)))
    {
        WARN("Returning DXGI_ERROR_UNSUPPORTED for %s.\n", debugstr_guid(guid));
        return DXGI_ERROR_UNSUPPORTED;
    }

    if (!dxgi_check_feature_level_support(adapter->factory, adapter, &feature_level, 1))
        return DXGI_ERROR_UNSUPPORTED;

    if (umd_version)
    {
        adapter_id.driver_size = 0;
        adapter_id.description_size = 0;
        adapter_id.device_name_size = 0;

        wined3d_mutex_lock();
        hr = wined3d_get_adapter_identifier(adapter->factory->wined3d, adapter->ordinal, 0, &adapter_id);
        wined3d_mutex_unlock();
        if (FAILED(hr))
            return hr;

        *umd_version = adapter_id.driver_version;
    }

    return S_OK;
}

static const struct IDXGIAdapter1Vtbl dxgi_adapter_vtbl =
{
    dxgi_adapter_QueryInterface,
    dxgi_adapter_AddRef,
    dxgi_adapter_Release,
    dxgi_adapter_SetPrivateData,
    dxgi_adapter_SetPrivateDataInterface,
    dxgi_adapter_GetPrivateData,
    dxgi_adapter_GetParent,
    dxgi_adapter_EnumOutputs,
    dxgi_adapter_GetDesc,
    dxgi_adapter_CheckInterfaceSupport,
    dxgi_adapter_GetDesc1,
};

struct dxgi_adapter *unsafe_impl_from_IDXGIAdapter1(IDXGIAdapter1 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &dxgi_adapter_vtbl);
    return CONTAINING_RECORD(iface, struct dxgi_adapter, IDXGIAdapter1_iface);
}

static void dxgi_adapter_init(struct dxgi_adapter *adapter, struct dxgi_factory *factory, UINT ordinal)
{
    adapter->IDXGIAdapter1_iface.lpVtbl = &dxgi_adapter_vtbl;
    adapter->refcount = 1;
    wined3d_private_store_init(&adapter->private_store);
    adapter->ordinal = ordinal;
    adapter->factory = factory;
    IDXGIFactory1_AddRef(&adapter->factory->IDXGIFactory1_iface);
}

HRESULT dxgi_adapter_create(struct dxgi_factory *factory, UINT ordinal, struct dxgi_adapter **adapter)
{
    if (!(*adapter = HeapAlloc(GetProcessHeap(), 0, sizeof(**adapter))))
        return E_OUTOFMEMORY;

    dxgi_adapter_init(*adapter, factory, ordinal);
    return S_OK;
}
