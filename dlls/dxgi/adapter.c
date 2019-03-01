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

static inline struct dxgi_adapter *impl_from_IWineDXGIAdapter(IWineDXGIAdapter *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_adapter, IWineDXGIAdapter_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_QueryInterface(IWineDXGIAdapter *iface, REFIID iid, void **out)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IWineDXGIAdapter)
            || IsEqualGUID(iid, &IID_IDXGIAdapter4)
            || IsEqualGUID(iid, &IID_IDXGIAdapter3)
            || IsEqualGUID(iid, &IID_IDXGIAdapter2)
            || IsEqualGUID(iid, &IID_IDXGIAdapter1)
            || IsEqualGUID(iid, &IID_IDXGIAdapter)
            || IsEqualGUID(iid, &IID_IDXGIObject)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IWineDXGISwapChainHelper))
    {
        IUnknown_AddRef(iface);
        *out = &adapter->IWineDXGISwapChainHelper_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_adapter_AddRef(IWineDXGIAdapter *iface)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
    ULONG refcount = InterlockedIncrement(&adapter->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_adapter_Release(IWineDXGIAdapter *iface)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
    ULONG refcount = InterlockedDecrement(&adapter->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        wined3d_private_store_cleanup(&adapter->private_store);
        IWineDXGIFactory_Release(&adapter->factory->IWineDXGIFactory_iface);
        heap_free(adapter);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_SetPrivateData(IWineDXGIAdapter *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&adapter->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_SetPrivateDataInterface(IWineDXGIAdapter *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&adapter->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetPrivateData(IWineDXGIAdapter *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&adapter->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetParent(IWineDXGIAdapter *iface, REFIID iid, void **parent)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);

    TRACE("iface %p, iid %s, parent %p.\n", iface, debugstr_guid(iid), parent);

    return IWineDXGIFactory_QueryInterface(&adapter->factory->IWineDXGIFactory_iface, iid, parent);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_EnumOutputs(IWineDXGIAdapter *iface,
        UINT output_idx, IDXGIOutput **output)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
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

    *output = (IDXGIOutput *)&output_object->IDXGIOutput4_iface;

    TRACE("Returning output %p.\n", *output);

    return S_OK;
}

static HRESULT dxgi_adapter_get_desc(struct dxgi_adapter *adapter, DXGI_ADAPTER_DESC3 *desc)
{
    struct wined3d_adapter_identifier adapter_id;
    char description[128];
    HRESULT hr;

    adapter_id.driver_size = 0;
    adapter_id.description = description;
    adapter_id.description_size = sizeof(description);
    adapter_id.device_name_size = 0;

    if (FAILED(hr = wined3d_get_adapter_identifier(adapter->factory->wined3d, adapter->ordinal, 0, &adapter_id)))
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
    desc->SharedSystemMemory = adapter_id.shared_system_memory;
    desc->AdapterLuid = adapter_id.adapter_luid;
    desc->Flags = 0;
    desc->GraphicsPreemptionGranularity = 0; /* FIXME */
    desc->ComputePreemptionGranularity = 0; /* FIXME */

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetDesc(IWineDXGIAdapter *iface, DXGI_ADAPTER_DESC *desc)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
    DXGI_ADAPTER_DESC3 desc3;
    HRESULT hr;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
        return E_INVALIDARG;

    if (SUCCEEDED(hr = dxgi_adapter_get_desc(adapter, &desc3)))
        memcpy(desc, &desc3, sizeof(*desc));

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_CheckInterfaceSupport(IWineDXGIAdapter *iface,
        REFGUID guid, LARGE_INTEGER *umd_version)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
    struct wined3d_adapter_identifier adapter_id;
    struct wined3d_caps caps;
    struct wined3d *wined3d;
    HRESULT hr;

    TRACE("iface %p, guid %s, umd_version %p.\n", iface, debugstr_guid(guid), umd_version);

    /* This method works only for D3D10 interfaces. */
    if (!(IsEqualGUID(guid, &IID_IDXGIDevice)
            || IsEqualGUID(guid, &IID_ID3D10Device)
            || IsEqualGUID(guid, &IID_ID3D10Device1)))
    {
        WARN("Returning DXGI_ERROR_UNSUPPORTED for %s.\n", debugstr_guid(guid));
        return DXGI_ERROR_UNSUPPORTED;
    }

    adapter_id.driver_size = 0;
    adapter_id.description_size = 0;
    adapter_id.device_name_size = 0;

    wined3d_mutex_lock();
    wined3d = adapter->factory->wined3d;
    hr = wined3d_get_device_caps(wined3d, adapter->ordinal, WINED3D_DEVICE_TYPE_HAL, &caps);
    if (SUCCEEDED(hr))
        hr = wined3d_get_adapter_identifier(wined3d, adapter->ordinal, 0, &adapter_id);
    wined3d_mutex_unlock();

    if (FAILED(hr))
        return hr;
    if (caps.max_feature_level < WINED3D_FEATURE_LEVEL_10)
        return DXGI_ERROR_UNSUPPORTED;

    if (umd_version)
        *umd_version = adapter_id.driver_version;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetDesc1(IWineDXGIAdapter *iface, DXGI_ADAPTER_DESC1 *desc)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
    DXGI_ADAPTER_DESC3 desc3;
    HRESULT hr;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
        return E_INVALIDARG;

    if (SUCCEEDED(hr = dxgi_adapter_get_desc(adapter, &desc3)))
        memcpy(desc, &desc3, sizeof(*desc));

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetDesc2(IWineDXGIAdapter *iface, DXGI_ADAPTER_DESC2 *desc)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
    DXGI_ADAPTER_DESC3 desc3;
    HRESULT hr;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
        return E_INVALIDARG;

    if (SUCCEEDED(hr = dxgi_adapter_get_desc(adapter, &desc3)))
        memcpy(desc, &desc3, sizeof(*desc));

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_RegisterHardwareContentProtectionTeardownStatusEvent(
        IWineDXGIAdapter *iface, HANDLE event, DWORD *cookie)
{
    FIXME("iface %p, event %p, cookie %p stub!\n", iface, event, cookie);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE dxgi_adapter_UnregisterHardwareContentProtectionTeardownStatus(
        IWineDXGIAdapter *iface, DWORD cookie)
{
    FIXME("iface %p, cookie %#x stub!\n", iface, cookie);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_QueryVideoMemoryInfo(IWineDXGIAdapter *iface,
        UINT node_index, DXGI_MEMORY_SEGMENT_GROUP segment_group, DXGI_QUERY_VIDEO_MEMORY_INFO *memory_info)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
    struct wined3d_adapter_identifier adapter_id;
    HRESULT hr;

    FIXME("iface %p, node_index %u, segment_group %#x, memory_info %p partial stub!\n",
            iface, node_index, segment_group, memory_info);

    if (node_index)
        FIXME("Ignoring node index %u.\n", node_index);

    adapter_id.driver_size = 0;
    adapter_id.description_size = 0;
    adapter_id.device_name_size = 0;

    if (FAILED(hr = wined3d_get_adapter_identifier(adapter->factory->wined3d, adapter->ordinal, 0, &adapter_id)))
        return hr;

    switch (segment_group)
    {
        case DXGI_MEMORY_SEGMENT_GROUP_LOCAL:
            memory_info->Budget = adapter_id.video_memory;
            memory_info->CurrentUsage = 0;
            memory_info->AvailableForReservation = adapter_id.video_memory / 2;
            memory_info->CurrentReservation = 0;
            break;
        case DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL:
            memset(memory_info, 0, sizeof(*memory_info));
            break;
        default:
            WARN("Invalid memory segment group %#x.\n", segment_group);
            return E_INVALIDARG;
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_SetVideoMemoryReservation(IWineDXGIAdapter *iface,
        UINT node_index, DXGI_MEMORY_SEGMENT_GROUP segment_group, UINT64 reservation)
{
    FIXME("iface %p, node_index %u, segment_group %#x, reservation %s stub!\n",
            iface, node_index, segment_group, wine_dbgstr_longlong(reservation));

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_RegisterVideoMemoryBudgetChangeNotificationEvent(
        IWineDXGIAdapter *iface, HANDLE event, DWORD *cookie)
{
    FIXME("iface %p, event %p, cookie %p stub!\n", iface, event, cookie);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE dxgi_adapter_UnregisterVideoMemoryBudgetChangeNotification(
        IWineDXGIAdapter *iface, DWORD cookie)
{
    FIXME("iface %p, cookie %#x stub!\n", iface, cookie);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetDesc3(IWineDXGIAdapter *iface, DXGI_ADAPTER_DESC3 *desc)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
        return E_INVALIDARG;

    return dxgi_adapter_get_desc(adapter, desc);
}

static const struct IWineDXGIAdapterVtbl dxgi_adapter_vtbl =
{
    dxgi_adapter_QueryInterface,
    dxgi_adapter_AddRef,
    dxgi_adapter_Release,
    dxgi_adapter_SetPrivateData,
    dxgi_adapter_SetPrivateDataInterface,
    dxgi_adapter_GetPrivateData,
    dxgi_adapter_GetParent,
    /* IDXGIAdapter methods */
    dxgi_adapter_EnumOutputs,
    dxgi_adapter_GetDesc,
    dxgi_adapter_CheckInterfaceSupport,
    /* IDXGIAdapter1 methods */
    dxgi_adapter_GetDesc1,
    /* IDXGIAdapter2 methods */
    dxgi_adapter_GetDesc2,
    /* IDXGIAdapter3 methods */
    dxgi_adapter_RegisterHardwareContentProtectionTeardownStatusEvent,
    dxgi_adapter_UnregisterHardwareContentProtectionTeardownStatus,
    dxgi_adapter_QueryVideoMemoryInfo,
    dxgi_adapter_SetVideoMemoryReservation,
    dxgi_adapter_RegisterVideoMemoryBudgetChangeNotificationEvent,
    dxgi_adapter_UnregisterVideoMemoryBudgetChangeNotification,
    /* IDXGIAdapter4 methods */
    dxgi_adapter_GetDesc3,
};

static inline struct dxgi_adapter *impl_from_IWineDXGISwapChainHelper(IWineDXGISwapChainHelper *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_adapter, IWineDXGISwapChainHelper_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_helper_QueryInterface(IWineDXGISwapChainHelper *iface,
        REFIID iid, void **out)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGISwapChainHelper(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    return dxgi_adapter_QueryInterface(&adapter->IWineDXGIAdapter_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE dxgi_swapchain_helper_AddRef(IWineDXGISwapChainHelper *iface)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGISwapChainHelper(iface);

    TRACE("iface %p.\n", iface);

    return dxgi_adapter_AddRef(&adapter->IWineDXGIAdapter_iface);
}

static ULONG STDMETHODCALLTYPE dxgi_swapchain_helper_Release(IWineDXGISwapChainHelper *iface)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGISwapChainHelper(iface);

    TRACE("iface %p.\n", iface);

    return dxgi_adapter_Release(&adapter->IWineDXGIAdapter_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_helper_get_monitor(IWineDXGISwapChainHelper *iface,
        HWND window, HMONITOR *monitor)
{
    RECT window_rect = {0, 0, 0, 0};
    POINT window_middle;

    TRACE("iface %p, window %p, monitor %p.\n", iface, window, monitor);

    if (!IsWindow(window) || !monitor)
        return DXGI_ERROR_INVALID_CALL;

    GetWindowRect(window, &window_rect);

    window_middle.x = (window_rect.left + window_rect.right) / 2; 
    window_middle.y = (window_rect.top + window_rect.bottom) / 2;

    *monitor = MonitorFromPoint(window_middle, MONITOR_DEFAULTTOPRIMARY);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_helper_get_window_info(IWineDXGISwapChainHelper *iface,
        HWND window, RECT *rect, RECT *client_rect, LONG *style, LONG *exstyle)
{
    TRACE("iface %p, window %p, rect %p, style %p, exstyle %p.\n", iface, window, rect, style, exstyle);

    if (!IsWindow(window))
        return DXGI_ERROR_INVALID_CALL;

    if (rect)
        GetWindowRect(window, rect);

    if (client_rect)
        GetClientRect(window, client_rect);

    if (style)
        *style = GetWindowLongW(window, GWL_STYLE);

    if (exstyle)
        *exstyle = GetWindowLongW(window, GWL_EXSTYLE);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_helper_set_window_pos(IWineDXGISwapChainHelper *iface,
        HWND window, HWND hwnd_insert_after, RECT position, UINT flags)
{
    TRACE("iface %p, window %p, hwnd_insert_after %p, position (%d,%d)-(%d,%d), flags %08x.\n", 
            iface, window, hwnd_insert_after, position.left, position.top, position.right, position.bottom, flags);

    if (!IsWindow(window))
        return DXGI_ERROR_INVALID_CALL;

    SetWindowPos(window, hwnd_insert_after, position.left, position.top, position.right - position.left, 
            position.bottom - position.top, flags);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_helper_resize_window(IWineDXGISwapChainHelper *iface,
        HWND window, UINT width, UINT height)
{
    RECT old_rect = {0, 0, 0, 0};
    RECT new_rect = {0, 0, 0, 0};

    TRACE("iface %p, window %p, width %u, height %u.\n", iface, window, width, height);

    if (!IsWindow(window))
        return DXGI_ERROR_INVALID_CALL;

    GetWindowRect(window, &old_rect);
    SetRect(&new_rect, 0, 0, width, height);
    AdjustWindowRectEx( &new_rect, GetWindowLongW(window, GWL_STYLE), FALSE, GetWindowLongW(window, GWL_EXSTYLE) );
    SetRect(&new_rect, 0, 0, new_rect.right - new_rect.left, new_rect.bottom - new_rect.top);
    OffsetRect(&new_rect, old_rect.left, old_rect.top);
    MoveWindow(window, new_rect.left, new_rect.top, new_rect.right - new_rect.left, new_rect.bottom - new_rect.top, TRUE);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_helper_set_window_styles(IWineDXGISwapChainHelper *iface,
        HWND window, const LONG *style, const LONG *exstyle)
{
    TRACE("iface %p, window %p, style %p, exstyle %p.\n", iface, window, style, exstyle);

    if (!IsWindow(window))
        return DXGI_ERROR_INVALID_CALL;

    if (style)
        SetWindowLongW(window, GWL_STYLE, *style);

    if (exstyle)
        SetWindowLongW(window, GWL_EXSTYLE, *exstyle);

    return S_OK;
}

static unsigned int GetMonitorFormatBpp(DXGI_FORMAT format)
{
    switch (format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            return 32;

        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return 64;

        default:
            WARN("Unknown format: %s\n", debug_dxgi_format(format));
            return 32;
    }
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_helper_get_display_mode(IWineDXGISwapChainHelper *iface,
        HMONITOR monitor, DWORD mode_num, DXGI_MODE_DESC *mode)
{
    MONITORINFOEXW mon_info;
    DEVMODEW dev_mode = {};
    DXGI_RATIONAL rate;

    TRACE("iface %p, monitor %p, mode_num %u, mode %p.\n", iface, monitor, mode_num, mode);

    mon_info.cbSize = sizeof(mon_info);
    if (!GetMonitorInfoW(monitor, (MONITORINFO*)&mon_info))
    {
        ERR("Failed to query monitor info\n");
        return E_FAIL;
    }

    dev_mode.dmSize = sizeof(dev_mode);
    if (!EnumDisplaySettingsW(mon_info.szDevice, mode_num, &dev_mode))
        return DXGI_ERROR_NOT_FOUND;

    mode->Width = dev_mode.dmPelsWidth;
    mode->Height = dev_mode.dmPelsHeight;
    rate.Numerator = dev_mode.dmDisplayFrequency;
    rate.Denominator = 1;
    mode->RefreshRate = rate;
    mode->Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; /* FIXME */
    mode->ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    mode->Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_helper_set_display_mode(IWineDXGISwapChainHelper *iface,
        HMONITOR monitor, const DXGI_MODE_DESC *mode)
{
    MONITORINFOEXW mon_info;
    DEVMODEW dev_mode = {};
    LONG status;

    TRACE("iface %p, monitor %p, mode %p.\n", iface, monitor, mode);
    
    mon_info.cbSize = sizeof(mon_info);
    if (!GetMonitorInfoW(monitor, (MONITORINFO*)&mon_info))
    {
        ERR("Failed to query monitor info\n");
        return E_FAIL;
    }

    dev_mode.dmSize = sizeof(dev_mode);
    dev_mode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
    dev_mode.dmPelsWidth = mode->Width;
    dev_mode.dmPelsHeight = mode->Height;
    dev_mode.dmBitsPerPel = GetMonitorFormatBpp(mode->Format);

    if (mode->RefreshRate.Numerator)
    {
        dev_mode.dmFields |= DM_DISPLAYFREQUENCY;
        dev_mode.dmDisplayFrequency = mode->RefreshRate.Numerator / mode->RefreshRate.Denominator;
    }

    TRACE("Setting Display Mode: %u x %u @ %u\n", dev_mode.dmPelsWidth, dev_mode.dmPelsHeight, dev_mode.dmDisplayFrequency);

    status = ChangeDisplaySettingsExW(mon_info.szDevice, &dev_mode, NULL, CDS_FULLSCREEN, NULL);

    return status == DISP_CHANGE_SUCCESSFUL ? S_OK : DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;
}

static const struct IWineDXGISwapChainHelperVtbl dxgi_swapchain_helper_vtbl =
{
    dxgi_swapchain_helper_QueryInterface,
    dxgi_swapchain_helper_AddRef,
    dxgi_swapchain_helper_Release,
    dxgi_swapchain_helper_get_monitor,
    dxgi_swapchain_helper_get_window_info,
    dxgi_swapchain_helper_set_window_pos,
    dxgi_swapchain_helper_resize_window,
    dxgi_swapchain_helper_set_window_styles,
    dxgi_swapchain_helper_get_display_mode,
    dxgi_swapchain_helper_set_display_mode
};

struct dxgi_adapter *unsafe_impl_from_IDXGIAdapter(IDXGIAdapter *iface)
{
    IWineDXGIAdapter *wine_adapter;
    struct dxgi_adapter *adapter;
    HRESULT hr;

    if (!iface)
        return NULL;
    if (FAILED(hr = IDXGIAdapter_QueryInterface(iface, &IID_IWineDXGIAdapter, (void **)&wine_adapter)))
    {
        ERR("Failed to get IWineDXGIAdapter interface, hr %#x.\n", hr);
        return NULL;
    }
    assert(wine_adapter->lpVtbl == &dxgi_adapter_vtbl);
    adapter = CONTAINING_RECORD(wine_adapter, struct dxgi_adapter, IWineDXGIAdapter_iface);
    IWineDXGIAdapter_Release(wine_adapter);
    return adapter;
}

static void dxgi_adapter_init(struct dxgi_adapter *adapter, struct dxgi_factory *factory, UINT ordinal)
{
    adapter->IWineDXGIAdapter_iface.lpVtbl = &dxgi_adapter_vtbl;
    adapter->IWineDXGISwapChainHelper_iface.lpVtbl = &dxgi_swapchain_helper_vtbl;
    adapter->refcount = 1;
    wined3d_private_store_init(&adapter->private_store);
    adapter->ordinal = ordinal;
    adapter->factory = factory;
    IWineDXGIFactory_AddRef(&adapter->factory->IWineDXGIFactory_iface);
}

HRESULT dxgi_adapter_create(struct dxgi_factory *factory, UINT ordinal, struct dxgi_adapter **adapter)
{
    if (!(*adapter = heap_alloc(sizeof(**adapter))))
        return E_OUTOFMEMORY;

    dxgi_adapter_init(*adapter, factory, ordinal);
    return S_OK;
}
