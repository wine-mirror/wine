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

#ifdef SONAME_LIBVULKAN

typedef VkFlags VkMacOSSurfaceCreateFlagsMVK;
#define VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK 1000123000

typedef VkFlags VkMetalSurfaceCreateFlagsEXT;
#define VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT 1000217000

struct wine_vk_surface
{
    macdrv_metal_device device;
    macdrv_metal_view view;
};

typedef struct VkMacOSSurfaceCreateInfoMVK
{
    VkStructureType sType;
    const void *pNext;
    VkMacOSSurfaceCreateFlagsMVK flags;
    const void *pView; /* NSView */
} VkMacOSSurfaceCreateInfoMVK;

typedef struct VkMetalSurfaceCreateInfoEXT
{
    VkStructureType sType;
    const void *pNext;
    VkMetalSurfaceCreateFlagsEXT flags;
    const void *pLayer; /* CAMetalLayer */
} VkMetalSurfaceCreateInfoEXT;

static VkResult (*pvkCreateMacOSSurfaceMVK)(VkInstance, const VkMacOSSurfaceCreateInfoMVK*, const VkAllocationCallbacks *, VkSurfaceKHR *);
static VkResult (*pvkCreateMetalSurfaceEXT)(VkInstance, const VkMetalSurfaceCreateInfoEXT*, const VkAllocationCallbacks *, VkSurfaceKHR *);
static VkResult (*pvkGetPhysicalDeviceSurfaceCapabilities2KHR)(VkPhysicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *, VkSurfaceCapabilities2KHR *);

static const struct vulkan_driver_funcs macdrv_vulkan_driver_funcs;

static void wine_vk_surface_destroy(struct wine_vk_surface *surface)
{
    if (surface->view)
        macdrv_view_release_metal_view(surface->view);

    if (surface->device)
        macdrv_release_metal_device(surface->device);

    free(surface);
}

static VkResult macdrv_vulkan_surface_create(HWND hwnd, const struct vulkan_instance *instance, VkSurfaceKHR *surface, void **private)
{
    VkResult res;
    struct wine_vk_surface *mac_surface;
    struct macdrv_win_data *data;

    TRACE("%p %p %p %p\n", hwnd, instance, surface, private);

    if (!(data = get_win_data(hwnd)))
    {
        FIXME("DC for window %p of other process: not implemented\n", hwnd);
        return VK_ERROR_INCOMPATIBLE_DRIVER;
    }

    mac_surface = calloc(1, sizeof(*mac_surface));
    if (!mac_surface)
    {
        release_win_data(data);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    mac_surface->device = macdrv_create_metal_device();
    if (!mac_surface->device)
    {
        ERR("Failed to allocate Metal device for hwnd=%p\n", hwnd);
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        goto err;
    }

    mac_surface->view = macdrv_view_create_metal_view(data->client_cocoa_view, mac_surface->device);
    if (!mac_surface->view)
    {
        ERR("Failed to allocate Metal view for hwnd=%p\n", hwnd);

        /* VK_KHR_win32_surface only allows out of host and device memory as errors. */
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        goto err;
    }

    if (pvkCreateMetalSurfaceEXT)
    {
        VkMetalSurfaceCreateInfoEXT create_info_host;
        create_info_host.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        create_info_host.pNext = NULL;
        create_info_host.flags = 0; /* reserved */
        create_info_host.pLayer = macdrv_view_get_metal_layer(mac_surface->view);

        res = pvkCreateMetalSurfaceEXT(instance->host.instance, &create_info_host, NULL /* allocator */, surface);
    }
    else
    {
        VkMacOSSurfaceCreateInfoMVK create_info_host;
        create_info_host.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
        create_info_host.pNext = NULL;
        create_info_host.flags = 0; /* reserved */
        create_info_host.pView = macdrv_view_get_metal_layer(mac_surface->view);

        res = pvkCreateMacOSSurfaceMVK(instance->host.instance, &create_info_host, NULL /* allocator */, surface);
    }
    if (res != VK_SUCCESS)
    {
        ERR("Failed to create MoltenVK surface, res=%d\n", res);
        goto err;
    }

    release_win_data(data);

    *private = mac_surface;

    TRACE("Created surface=0x%s, private=%p\n", wine_dbgstr_longlong(*surface), *private);
    return VK_SUCCESS;

err:
    wine_vk_surface_destroy(mac_surface);
    release_win_data(data);
    return res;
}

static void macdrv_vulkan_surface_destroy(HWND hwnd, void *private)
{
    struct wine_vk_surface *mac_surface = private;

    TRACE("%p %p\n", hwnd, private);

    wine_vk_surface_destroy(mac_surface);
}

static void macdrv_vulkan_surface_detach(HWND hwnd, void *private)
{
}

static void macdrv_vulkan_surface_update(HWND hwnd, void *private)
{
}

static void macdrv_vulkan_surface_presented(HWND hwnd, void *private, VkResult result)
{
}

static VkBool32 macdrv_vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice phys_dev,
        uint32_t index)
{
    TRACE("%p %u\n", phys_dev, index);

    return VK_TRUE;
}

static const char *macdrv_get_host_surface_extension(void)
{
    return pvkCreateMetalSurfaceEXT ? "VK_EXT_metal_surface" : "VK_MVK_macos_surface";
}

static const struct vulkan_driver_funcs macdrv_vulkan_driver_funcs =
{
    .p_vulkan_surface_create = macdrv_vulkan_surface_create,
    .p_vulkan_surface_destroy = macdrv_vulkan_surface_destroy,
    .p_vulkan_surface_detach = macdrv_vulkan_surface_detach,
    .p_vulkan_surface_update = macdrv_vulkan_surface_update,
    .p_vulkan_surface_presented = macdrv_vulkan_surface_presented,

    .p_vkGetPhysicalDeviceWin32PresentationSupportKHR = macdrv_vkGetPhysicalDeviceWin32PresentationSupportKHR,
    .p_get_host_surface_extension = macdrv_get_host_surface_extension,
};

UINT macdrv_VulkanInit(UINT version, void *vulkan_handle, const struct vulkan_driver_funcs **driver_funcs)
{
    if (version != WINE_VULKAN_DRIVER_VERSION)
    {
        ERR("version mismatch, win32u wants %u but driver has %u\n", version, WINE_VULKAN_DRIVER_VERSION);
        return STATUS_INVALID_PARAMETER;
    }

#define LOAD_FUNCPTR(f) if ((p##f = dlsym(vulkan_handle, #f)) == NULL) return STATUS_PROCEDURE_NOT_FOUND;
    LOAD_FUNCPTR(vkCreateMacOSSurfaceMVK)
    LOAD_FUNCPTR(vkCreateMetalSurfaceEXT)
#undef LOAD_FUNCPTR

    *driver_funcs = &macdrv_vulkan_driver_funcs;
    return STATUS_SUCCESS;
}

#else /* No vulkan */

UINT macdrv_VulkanInit(UINT version, void *vulkan_handle, const struct vulkan_driver_funcs **driver_funcs)
{
    ERR("Wine was built without Vulkan support.\n");
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* SONAME_LIBVULKAN */
