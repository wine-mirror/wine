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

#include <dlfcn.h>

#include "ntstatus.h"
#include "macdrv.h"
#include "wine/debug.h"

#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

static const struct vulkan_driver_funcs macdrv_vulkan_driver_funcs;

static VkResult macdrv_vulkan_surface_create(struct client_surface *client, const struct vulkan_instance *instance, VkSurfaceKHR *handle)
{
    VkResult res;
    struct macdrv_client_surface *surface = impl_from_client_surface(client);

    TRACE("%s %p %p\n", debugstr_client_surface(client), instance, handle);

    if (!macdrv_client_surface_acquire_metal_swapchain(surface)) return VK_ERROR_INCOMPATIBLE_DRIVER;

    if (instance->p_vkCreateMetalSurfaceEXT)
    {
        VkMetalSurfaceCreateInfoEXT create_info_host;
        create_info_host.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        create_info_host.pNext = NULL;
        create_info_host.flags = 0; /* reserved */
        create_info_host.pLayer = macdrv_swapchain_get_layer(surface->metal_swapchain);

        if ((res = instance->p_vkCreateMetalSurfaceEXT(instance->host.instance, &create_info_host, NULL /* allocator */, handle))) return res;
    }
    else
    {
        VkMacOSSurfaceCreateInfoMVK create_info_host;
        create_info_host.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
        create_info_host.pNext = NULL;
        create_info_host.flags = 0; /* reserved */
        create_info_host.pView = macdrv_swapchain_get_layer(surface->metal_swapchain);

        if ((res = instance->p_vkCreateMacOSSurfaceMVK(instance->host.instance, &create_info_host, NULL /* allocator */, handle))) return res;
    }

    TRACE("Created surface=0x%s\n", wine_dbgstr_longlong(*handle));
    return VK_SUCCESS;
}

static VkBool32 macdrv_get_physical_device_presentation_support(struct vulkan_physical_device *physical_device,
        uint32_t index)
{
    TRACE("%p %u\n", physical_device, index);

    return VK_TRUE;
}

static BOOL use_VK_EXT_metal_surface;

static void macdrv_map_instance_extensions(struct vulkan_instance_extensions *extensions)
{
    if (use_VK_EXT_metal_surface)
    {
        if (extensions->has_VK_KHR_win32_surface) extensions->has_VK_EXT_metal_surface = 1;
        if (extensions->has_VK_EXT_metal_surface) extensions->has_VK_KHR_win32_surface = 1;
    }
    else
    {
        if (extensions->has_VK_KHR_win32_surface) extensions->has_VK_MVK_macos_surface = 1;
        if (extensions->has_VK_MVK_macos_surface) extensions->has_VK_KHR_win32_surface = 1;
    }
}

static void macdrv_map_device_extensions(struct vulkan_device_extensions *extensions)
{
}

static const struct vulkan_driver_funcs macdrv_vulkan_driver_funcs =
{
    .p_vulkan_surface_create = macdrv_vulkan_surface_create,
    .p_get_physical_device_presentation_support = macdrv_get_physical_device_presentation_support,
    .p_map_instance_extensions = macdrv_map_instance_extensions,
    .p_map_device_extensions = macdrv_map_device_extensions,
};

UINT macdrv_VulkanInit(UINT version, void *vulkan_handle, const struct vulkan_driver_funcs **driver_funcs)
{
    if (version != WINE_VULKAN_DRIVER_VERSION)
    {
        ERR("version mismatch, win32u wants %u but driver has %u\n", version, WINE_VULKAN_DRIVER_VERSION);
        return STATUS_INVALID_PARAMETER;
    }

    use_VK_EXT_metal_surface = !!dlsym(vulkan_handle, "vkCreateMetalSurfaceEXT");

    *driver_funcs = &macdrv_vulkan_driver_funcs;
    return STATUS_SUCCESS;
}
