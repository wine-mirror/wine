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
    struct client_surface client;
    macdrv_metal_device device;
    macdrv_metal_view view;
};

static struct wine_vk_surface *impl_from_client_surface(struct client_surface *client)
{
    return CONTAINING_RECORD(client, struct wine_vk_surface, client);
}

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

static const struct client_surface_funcs macdrv_vulkan_surface_funcs;
static const struct vulkan_driver_funcs macdrv_vulkan_driver_funcs;

static VkResult macdrv_vulkan_surface_create(HWND hwnd, const struct vulkan_instance *instance, VkSurfaceKHR *handle,
                                             struct client_surface **client)
{
    VkResult res;
    struct wine_vk_surface *surface;
    struct macdrv_win_data *data;

    TRACE("%p %p %p %p\n", hwnd, instance, handle, client);

    if (!(data = get_win_data(hwnd)))
    {
        FIXME("DC for window %p of other process: not implemented\n", hwnd);
        return VK_ERROR_INCOMPATIBLE_DRIVER;
    }

    if (!(surface = client_surface_create(sizeof(*surface), &macdrv_vulkan_surface_funcs, hwnd)))
    {
        release_win_data(data);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    surface->device = macdrv_create_metal_device();
    if (!surface->device)
    {
        ERR("Failed to allocate Metal device for hwnd=%p\n", hwnd);
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        goto err;
    }

    surface->view = macdrv_view_create_metal_view(data->client_cocoa_view, surface->device);
    if (!surface->view)
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
        create_info_host.pLayer = macdrv_view_get_metal_layer(surface->view);

        res = pvkCreateMetalSurfaceEXT(instance->host.instance, &create_info_host, NULL /* allocator */, handle);
    }
    else
    {
        VkMacOSSurfaceCreateInfoMVK create_info_host;
        create_info_host.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
        create_info_host.pNext = NULL;
        create_info_host.flags = 0; /* reserved */
        create_info_host.pView = macdrv_view_get_metal_layer(surface->view);

        res = pvkCreateMacOSSurfaceMVK(instance->host.instance, &create_info_host, NULL /* allocator */, handle);
    }
    if (res != VK_SUCCESS)
    {
        ERR("Failed to create MoltenVK surface, res=%d\n", res);
        goto err;
    }

    release_win_data(data);

    *client = &surface->client;

    TRACE("Created surface=0x%s, client=%p\n", wine_dbgstr_longlong(*handle), *client);
    return VK_SUCCESS;

err:
    client_surface_release(&surface->client);
    release_win_data(data);
    return res;
}

static void macdrv_vulkan_surface_destroy(struct client_surface *client)
{
    struct wine_vk_surface *surface = impl_from_client_surface(client);

    TRACE("%s\n", debugstr_client_surface(client));

    if (surface->view)
        macdrv_view_release_metal_view(surface->view);
    if (surface->device)
        macdrv_release_metal_device(surface->device);
}

static void macdrv_vulkan_surface_detach(struct client_surface *client)
{
}

static void macdrv_vulkan_surface_update(struct client_surface *client)
{
}

static void macdrv_vulkan_surface_presented(struct client_surface *client)
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

static const struct client_surface_funcs macdrv_vulkan_surface_funcs =
{
    .destroy = macdrv_vulkan_surface_destroy,
};

static const struct vulkan_driver_funcs macdrv_vulkan_driver_funcs =
{
    .p_vulkan_surface_create = macdrv_vulkan_surface_create,
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
