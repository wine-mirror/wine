/*
 * Copyright 2018 Józef Kucia for CodeWeavers
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

#define VK_NO_PROTOTYPES
#define VKD3D_NO_VULKAN_H
#define VKD3D_NO_WIN32_TYPES
#define WINE_VK_HOST

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"

#include "d3d12.h"

#include <vkd3d.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

WINE_DEFAULT_DEBUG_CHANNEL(d3d12);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

HRESULT WINAPI D3D12GetDebugInterface(REFIID iid, void **debug)
{
    FIXME("iid %s, debug %p stub!\n", debugstr_guid(iid), debug);

    return E_NOTIMPL;
}

static HRESULT d3d12_signal_event(HANDLE event)
{
    return SetEvent(event) ? S_OK : E_FAIL;
}

struct d3d12_thread_data
{
    PFN_vkd3d_thread main_pfn;
    void *data;
};

static DWORD WINAPI d3d12_thread_main(void *data)
{
    struct d3d12_thread_data *thread_data = data;

    thread_data->main_pfn(thread_data->data);
    heap_free(thread_data);
    return 0;
}

static void *d3d12_create_thread(PFN_vkd3d_thread main_pfn, void *data)
{
    struct d3d12_thread_data *thread_data;
    HANDLE thread;

    if (!(thread_data = heap_alloc(sizeof(*thread_data))))
    {
        ERR("Failed to allocate thread data.\n");
        return NULL;
    }

    thread_data->main_pfn = main_pfn;
    thread_data->data = data;

    if (!(thread = CreateThread(NULL, 0, d3d12_thread_main, thread_data, 0, NULL)))
        heap_free(thread_data);

    return thread;
}

static HRESULT d3d12_join_thread(void *handle)
{
    HANDLE thread = handle;
    DWORD ret;

    if ((ret = WaitForSingleObject(thread, INFINITE)) != WAIT_OBJECT_0)
        ERR("Failed to wait for thread, ret %#x.\n", ret);
    CloseHandle(thread);
    return ret == WAIT_OBJECT_0 ? S_OK : E_FAIL;
}

static const struct vulkan_funcs *get_vk_funcs(void)
{
    const struct vulkan_funcs *vk_funcs;
    HDC hdc;

    hdc = GetDC(0);
    vk_funcs = __wine_get_vulkan_driver(hdc, WINE_VULKAN_DRIVER_VERSION);
    ReleaseDC(0, hdc);
    return vk_funcs;
}

HRESULT WINAPI D3D12CreateDevice(IUnknown *adapter, D3D_FEATURE_LEVEL minimum_feature_level,
        REFIID iid, void **device)
{
    struct vkd3d_instance_create_info instance_create_info;
    struct vkd3d_device_create_info device_create_info;
    const struct vulkan_funcs *vk_funcs;

    static const char * const instance_extensions[] =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };
    static const char * const device_extensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    TRACE("adapter %p, minimum_feature_level %#x, iid %s, device %p.\n",
            adapter, minimum_feature_level, debugstr_guid(iid), device);

    if (!(vk_funcs = get_vk_funcs()))
    {
        ERR_(winediag)("Failed to load Wine Vulkan driver.\n");
        return E_FAIL;
    }

    FIXME("Ignoring adapter %p.\n", adapter);

    memset(&instance_create_info, 0, sizeof(instance_create_info));
    instance_create_info.type = VKD3D_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pfn_signal_event = d3d12_signal_event;
    instance_create_info.pfn_create_thread = d3d12_create_thread;
    instance_create_info.pfn_join_thread = d3d12_join_thread;
    instance_create_info.wchar_size = sizeof(WCHAR);
    instance_create_info.pfn_vkGetInstanceProcAddr
            = (PFN_vkGetInstanceProcAddr)vk_funcs->p_vkGetInstanceProcAddr;
    instance_create_info.instance_extensions = instance_extensions;
    instance_create_info.instance_extension_count = ARRAY_SIZE(instance_extensions);

    memset(&device_create_info, 0, sizeof(device_create_info));
    device_create_info.type = VKD3D_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.minimum_feature_level = minimum_feature_level;
    device_create_info.instance_create_info = &instance_create_info;
    device_create_info.device_extensions = device_extensions;
    device_create_info.device_extension_count = ARRAY_SIZE(device_extensions);

    return vkd3d_create_device(&device_create_info, iid, device);
}

HRESULT WINAPI D3D12CreateRootSignatureDeserializer(const void *data, SIZE_T data_size,
        REFIID iid, void **deserializer)
{
    TRACE("data %p, data_size %lu, iid %s, deserializer %p.\n",
            data, data_size, debugstr_guid(iid), deserializer);

    return vkd3d_create_root_signature_deserializer(data, data_size, iid, deserializer);
}

HRESULT WINAPI D3D12SerializeRootSignature(const D3D12_ROOT_SIGNATURE_DESC *root_signature_desc,
        D3D_ROOT_SIGNATURE_VERSION version, ID3DBlob **blob, ID3DBlob **error_blob)
{
    TRACE("root_signature_desc %p, version %#x, blob %p, error_blob %p.\n",
            root_signature_desc, version, blob, error_blob);

    return vkd3d_serialize_root_signature(root_signature_desc, version, blob, error_blob);
}
