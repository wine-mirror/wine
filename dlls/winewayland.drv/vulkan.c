/* WAYLANDDRV Vulkan implementation
 *
 * Copyright 2017 Roderick Colenbrander
 * Copyright 2021 Alexandros Frantzis
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <dlfcn.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "waylanddrv.h"
#include "wine/debug.h"

#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

static const struct vulkan_driver_funcs wayland_vulkan_driver_funcs;

static VkResult wayland_vulkan_surface_create(HWND hwnd, const struct vulkan_instance *instance, VkSurfaceKHR *handle,
                                              struct client_surface **client)
{
    VkResult res;
    VkWaylandSurfaceCreateInfoKHR create_info_host;
    struct wayland_client_surface *surface;

    TRACE("%p %p %p %p\n", hwnd, instance, handle, client);

    if (!(surface = wayland_client_surface_create(hwnd))) return VK_ERROR_OUT_OF_HOST_MEMORY;
    create_info_host.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    create_info_host.pNext = NULL;
    create_info_host.flags = 0; /* reserved */
    create_info_host.display = process_wayland.wl_display;
    create_info_host.surface = surface->wl_surface;

    res = instance->p_vkCreateWaylandSurfaceKHR(instance->host.instance, &create_info_host, NULL /* allocator */, handle);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to create vulkan wayland surface, res=%d\n", res);
        client_surface_release(&surface->client);
        return res;
    }

    set_client_surface(hwnd, surface);
    *client = &surface->client;

    TRACE("Created surface=0x%s, client=%p\n", wine_dbgstr_longlong(*handle), *client);
    return VK_SUCCESS;
}

static VkBool32 wayland_get_physical_device_presentation_support(struct vulkan_physical_device *physical_device,
                                                                 uint32_t index)
{
    struct vulkan_instance *instance = physical_device->instance;

    TRACE("%p %u\n", physical_device, index);

    return instance->p_vkGetPhysicalDeviceWaylandPresentationSupportKHR(physical_device->host.physical_device, index,
                                                                        process_wayland.wl_display);
}

static const char *wayland_get_host_surface_extension(void)
{
    return "VK_KHR_wayland_surface";
}

static const struct vulkan_driver_funcs wayland_vulkan_driver_funcs =
{
    .p_vulkan_surface_create = wayland_vulkan_surface_create,
    .p_get_physical_device_presentation_support = wayland_get_physical_device_presentation_support,
    .p_get_host_surface_extension = wayland_get_host_surface_extension,
};

/**********************************************************************
 *           WAYLAND_VulkanInit
 */
UINT WAYLAND_VulkanInit(UINT version, void *vulkan_handle, const struct vulkan_driver_funcs **driver_funcs)
{
    if (version != WINE_VULKAN_DRIVER_VERSION)
    {
        ERR("version mismatch, win32u wants %u but driver has %u\n", version, WINE_VULKAN_DRIVER_VERSION);
        return STATUS_INVALID_PARAMETER;
    }

    *driver_funcs = &wayland_vulkan_driver_funcs;
    return STATUS_SUCCESS;
}
