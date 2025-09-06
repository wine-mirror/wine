/* Mac Driver Vulkan implementation
 *
 * Copyright 2017 Roderick Colenbrander
 * Copyright 2018 Andrew Eikum for CodeWeavers
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

/* NOTE: If making changes here, consider whether they should be reflected in
 * the other drivers. */

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <dlfcn.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "macdrv.h"
#include "wine/debug.h"

#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

static const struct vulkan_driver_funcs macdrv_vulkan_driver_funcs;

static VkResult macdrv_vulkan_surface_create(HWND hwnd, const struct vulkan_instance *instance, VkSurfaceKHR *handle,
                                             struct client_surface **client)
{
    VkResult res;
    struct macdrv_client_surface *surface;

    TRACE("%p %p %p %p\n", hwnd, instance, handle, client);

    if (!(surface = macdrv_client_surface_create(hwnd))) return VK_ERROR_OUT_OF_HOST_MEMORY;
    if (!(surface->metal_device = macdrv_create_metal_device())) goto err;
    if (!(surface->metal_view = macdrv_view_create_metal_view(surface->cocoa_view, surface->metal_device))) goto err;

    if (instance->p_vkCreateMetalSurfaceEXT)
    {
        VkMetalSurfaceCreateInfoEXT create_info_host;
        create_info_host.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        create_info_host.pNext = NULL;
        create_info_host.flags = 0; /* reserved */
        create_info_host.pLayer = macdrv_view_get_metal_layer(surface->metal_view);

        res = instance->p_vkCreateMetalSurfaceEXT(instance->host.instance, &create_info_host, NULL /* allocator */, handle);
    }
    else
    {
        VkMacOSSurfaceCreateInfoMVK create_info_host;
        create_info_host.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
        create_info_host.pNext = NULL;
        create_info_host.flags = 0; /* reserved */
        create_info_host.pView = macdrv_view_get_metal_layer(surface->metal_view);

        res = instance->p_vkCreateMacOSSurfaceMVK(instance->host.instance, &create_info_host, NULL /* allocator */, handle);
    }
    if (res != VK_SUCCESS)
    {
        ERR("Failed to create MoltenVK surface, res=%d\n", res);
        goto err;
    }

    *client = &surface->client;
    TRACE("Created surface=0x%s, client=%p\n", wine_dbgstr_longlong(*handle), *client);
    return VK_SUCCESS;

err:
    client_surface_release(&surface->client);
    return VK_ERROR_INCOMPATIBLE_DRIVER;
}

static VkBool32 macdrv_get_physical_device_presentation_support(struct vulkan_physical_device *physical_device,
        uint32_t index)
{
    TRACE("%p %u\n", physical_device, index);

    return VK_TRUE;
}

static const char *host_surface_extension = "VK_MVK_macos_surface";
static const char *macdrv_get_host_extension(const char *name)
{
    if (!strcmp( name, "VK_KHR_win32_surface" )) return host_surface_extension;
    return name;
}

static const struct vulkan_driver_funcs macdrv_vulkan_driver_funcs =
{
    .p_vulkan_surface_create = macdrv_vulkan_surface_create,
    .p_get_physical_device_presentation_support = macdrv_get_physical_device_presentation_support,
    .p_get_host_extension = macdrv_get_host_extension,
};

UINT macdrv_VulkanInit(UINT version, void *vulkan_handle, const struct vulkan_driver_funcs **driver_funcs)
{
    if (version != WINE_VULKAN_DRIVER_VERSION)
    {
        ERR("version mismatch, win32u wants %u but driver has %u\n", version, WINE_VULKAN_DRIVER_VERSION);
        return STATUS_INVALID_PARAMETER;
    }

    if (dlsym(vulkan_handle, "vkCreateMetalSurfaceEXT")) host_surface_extension = "VK_EXT_metal_surface";

    *driver_funcs = &macdrv_vulkan_driver_funcs;
    return STATUS_SUCCESS;
}
