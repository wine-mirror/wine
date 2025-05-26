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

#ifdef SONAME_LIBVULKAN

#define VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR 1000006000

typedef struct VkWaylandSurfaceCreateInfoKHR
{
    VkStructureType sType;
    const void *pNext;
    VkWaylandSurfaceCreateFlagsKHR flags;
    struct wl_display *display;
    struct wl_surface *surface;
} VkWaylandSurfaceCreateInfoKHR;

static VkResult (*pvkCreateWaylandSurfaceKHR)(VkInstance, const VkWaylandSurfaceCreateInfoKHR *, const VkAllocationCallbacks *, VkSurfaceKHR *);
static VkBool32 (*pvkGetPhysicalDeviceWaylandPresentationSupportKHR)(VkPhysicalDevice, uint32_t, struct wl_display *);

static const struct vulkan_driver_funcs wayland_vulkan_driver_funcs;

static void wine_vk_surface_destroy(struct wayland_client_surface *client)
{
    HWND hwnd = wl_surface_get_user_data(client->wl_surface);
    struct wayland_win_data *data = wayland_win_data_get(hwnd);

    if (wayland_client_surface_release(client) && data)
        data->client_surface = NULL;

    if (data) wayland_win_data_release(data);
}

static VkResult wayland_vulkan_surface_create(HWND hwnd, const struct vulkan_instance *instance, VkSurfaceKHR *surface, void **private)
{
    VkResult res;
    VkWaylandSurfaceCreateInfoKHR create_info_host;
    struct wayland_client_surface *client;

    TRACE("%p %p %p %p\n", hwnd, instance, surface, private);

    if (!(client = wayland_client_surface_create(hwnd)))
    {
        ERR("Failed to create client surface for hwnd=%p\n", hwnd);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    create_info_host.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    create_info_host.pNext = NULL;
    create_info_host.flags = 0; /* reserved */
    create_info_host.display = process_wayland.wl_display;
    create_info_host.surface = client->wl_surface;

    res = pvkCreateWaylandSurfaceKHR(instance->host.instance, &create_info_host,
                                     NULL /* allocator */,
                                     surface);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to create vulkan wayland surface, res=%d\n", res);
        wine_vk_surface_destroy(client);
        return res;
    }

    set_client_surface(hwnd, client);
    *private = client;

    TRACE("Created surface=0x%s, private=%p\n", wine_dbgstr_longlong(*surface), *private);
    return VK_SUCCESS;
}

static void wayland_vulkan_surface_destroy(HWND hwnd, void *private)
{
    struct wayland_client_surface *client = private;

    TRACE("%p %p\n", hwnd, private);

    wine_vk_surface_destroy(client);
}

static void wayland_vulkan_surface_detach(HWND hwnd, void *private)
{
}

static void wayland_vulkan_surface_update(HWND hwnd, void *private)
{
}

static void wayland_vulkan_surface_presented(HWND hwnd, void *private)
{
    struct wayland_client_surface *client = private;
    HWND toplevel = NtUserGetAncestor(hwnd, GA_ROOT);
    ensure_window_surface_contents(toplevel);
    set_client_surface(hwnd, client);
}

static VkBool32 wayland_vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice phys_dev,
                                                                       uint32_t index)
{
    TRACE("%p %u\n", phys_dev, index);

    return pvkGetPhysicalDeviceWaylandPresentationSupportKHR(phys_dev, index,
                                                             process_wayland.wl_display);
}

static const char *wayland_get_host_surface_extension(void)
{
    return "VK_KHR_wayland_surface";
}

static const struct vulkan_driver_funcs wayland_vulkan_driver_funcs =
{
    .p_vulkan_surface_create = wayland_vulkan_surface_create,
    .p_vulkan_surface_destroy = wayland_vulkan_surface_destroy,
    .p_vulkan_surface_detach = wayland_vulkan_surface_detach,
    .p_vulkan_surface_update = wayland_vulkan_surface_update,
    .p_vulkan_surface_presented = wayland_vulkan_surface_presented,

    .p_vkGetPhysicalDeviceWin32PresentationSupportKHR = wayland_vkGetPhysicalDeviceWin32PresentationSupportKHR,
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

#define LOAD_FUNCPTR(f) if (!(p##f = dlsym(vulkan_handle, #f))) return STATUS_PROCEDURE_NOT_FOUND;
    LOAD_FUNCPTR(vkCreateWaylandSurfaceKHR);
    LOAD_FUNCPTR(vkGetPhysicalDeviceWaylandPresentationSupportKHR);
#undef LOAD_FUNCPTR

    *driver_funcs = &wayland_vulkan_driver_funcs;
    return STATUS_SUCCESS;
}

#else /* No vulkan */

UINT WAYLAND_VulkanInit(UINT version, void *vulkan_handle, const struct vulkan_driver_funcs **driver_funcs)
{
    ERR( "Wine was built without Vulkan support.\n" );
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* SONAME_LIBVULKAN */
