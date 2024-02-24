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

#define VK_NO_PROTOTYPES
#define WINE_VK_HOST

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
static void (*pvkDestroySurfaceKHR)(VkInstance, VkSurfaceKHR, const VkAllocationCallbacks *);
static VkBool32 (*pvkGetPhysicalDeviceWaylandPresentationSupportKHR)(VkPhysicalDevice, uint32_t, struct wl_display *);

static const struct vulkan_funcs vulkan_funcs;

struct wine_vk_surface
{
    struct wayland_client_surface *client;
    VkSurfaceKHR host_surface;
};

static struct wine_vk_surface *wine_vk_surface_from_handle(VkSurfaceKHR handle)
{
    return (struct wine_vk_surface *)(uintptr_t)handle;
}

static HWND wine_vk_surface_get_hwnd(struct wine_vk_surface *wine_vk_surface)
{
    return wl_surface_get_user_data(wine_vk_surface->client->wl_surface);
}

static void wine_vk_surface_destroy(struct wine_vk_surface *wine_vk_surface)
{
    if (wine_vk_surface->client)
    {
        HWND hwnd = wine_vk_surface_get_hwnd(wine_vk_surface);
        struct wayland_surface *wayland_surface = wayland_surface_lock_hwnd(hwnd);

        if (wayland_client_surface_release(wine_vk_surface->client) &&
            wayland_surface)
        {
            wayland_surface->client = NULL;
        }

        if (wayland_surface) pthread_mutex_unlock(&wayland_surface->mutex);
    }

    free(wine_vk_surface);
}

static VkResult wayland_vkCreateWin32SurfaceKHR(VkInstance instance,
                                                const VkWin32SurfaceCreateInfoKHR *create_info,
                                                const VkAllocationCallbacks *allocator,
                                                VkSurfaceKHR *vk_surface)
{
    VkResult res;
    VkWaylandSurfaceCreateInfoKHR create_info_host;
    struct wine_vk_surface *wine_vk_surface;
    struct wayland_surface *wayland_surface;

    TRACE("%p %p %p %p\n", instance, create_info, allocator, vk_surface);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    wine_vk_surface = calloc(1, sizeof(*wine_vk_surface));
    if (!wine_vk_surface)
    {
        ERR("Failed to allocate memory for wayland vulkan surface\n");
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        goto err;
    }

    wayland_surface = wayland_surface_lock_hwnd(create_info->hwnd);
    if (!wayland_surface)
    {
        ERR("Failed to find wayland surface for hwnd=%p\n", create_info->hwnd);
        /* VK_KHR_win32_surface only allows out of host and device memory as errors. */
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        goto err;
    }

    wine_vk_surface->client = wayland_surface_get_client(wayland_surface);
    pthread_mutex_unlock(&wayland_surface->mutex);

    if (!wine_vk_surface->client)
    {
        ERR("Failed to create client surface for hwnd=%p\n", create_info->hwnd);
        /* VK_KHR_win32_surface only allows out of host and device memory as errors. */
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        goto err;
    }

    create_info_host.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    create_info_host.pNext = NULL;
    create_info_host.flags = 0; /* reserved */
    create_info_host.display = process_wayland.wl_display;
    create_info_host.surface = wine_vk_surface->client->wl_surface;

    res = pvkCreateWaylandSurfaceKHR(instance, &create_info_host,
                                     NULL /* allocator */,
                                     &wine_vk_surface->host_surface);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to create vulkan wayland surface, res=%d\n", res);
        goto err;
    }

    *vk_surface = (uintptr_t)wine_vk_surface;

    TRACE("Created surface=0x%s\n", wine_dbgstr_longlong(*vk_surface));
    return VK_SUCCESS;

err:
    if (wine_vk_surface) wine_vk_surface_destroy(wine_vk_surface);
    return res;
}

static void wayland_vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface,
                                        const VkAllocationCallbacks *allocator)
{
    struct wine_vk_surface *wine_vk_surface = wine_vk_surface_from_handle(surface);

    TRACE("%p 0x%s %p\n", instance, wine_dbgstr_longlong(surface), allocator);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    pvkDestroySurfaceKHR(instance, wine_vk_surface->host_surface, NULL /* allocator */);
    wine_vk_surface_destroy(wine_vk_surface);
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

static VkSurfaceKHR wayland_wine_get_host_surface(VkSurfaceKHR surface)
{
    return wine_vk_surface_from_handle(surface)->host_surface;
}

static void wayland_vulkan_surface_presented(HWND hwnd, VkResult result)
{
    struct wayland_surface *wayland_surface;

    if ((wayland_surface = wayland_surface_lock_hwnd(hwnd)))
    {
        wayland_surface_ensure_contents(wayland_surface);

        /* Handle any processed configure request, to ensure the related
         * surface state is applied by the compositor. */
        if (wayland_surface->processing.serial &&
            wayland_surface->processing.processed &&
            wayland_surface_reconfigure(wayland_surface))
        {
            wl_surface_commit(wayland_surface->wl_surface);
        }

        pthread_mutex_unlock(&wayland_surface->mutex);
    }
}

static const struct vulkan_funcs vulkan_funcs =
{
    .p_vkCreateWin32SurfaceKHR = wayland_vkCreateWin32SurfaceKHR,
    .p_vkDestroySurfaceKHR = wayland_vkDestroySurfaceKHR,
    .p_vkGetPhysicalDeviceWin32PresentationSupportKHR = wayland_vkGetPhysicalDeviceWin32PresentationSupportKHR,
    .p_get_host_surface_extension = wayland_get_host_surface_extension,
    .p_wine_get_host_surface = wayland_wine_get_host_surface,
    .p_vulkan_surface_presented = wayland_vulkan_surface_presented,
};

/**********************************************************************
 *           WAYLAND_VulkanInit
 */
UINT WAYLAND_VulkanInit(UINT version, void *vulkan_handle, struct vulkan_funcs *driver_funcs)
{
    if (version != WINE_VULKAN_DRIVER_VERSION)
    {
        ERR("version mismatch, win32u wants %u but driver has %u\n", version, WINE_VULKAN_DRIVER_VERSION);
        return STATUS_INVALID_PARAMETER;
    }

#define LOAD_FUNCPTR(f) if (!(p##f = dlsym(vulkan_handle, #f))) return STATUS_PROCEDURE_NOT_FOUND;
    LOAD_FUNCPTR(vkCreateWaylandSurfaceKHR);
    LOAD_FUNCPTR(vkDestroySurfaceKHR);
    LOAD_FUNCPTR(vkGetPhysicalDeviceWaylandPresentationSupportKHR);
#undef LOAD_FUNCPTR

    *driver_funcs = vulkan_funcs;
    return STATUS_SUCCESS;
}

#else /* No vulkan */

UINT WAYLAND_VulkanInit(UINT version, void *vulkan_handle, struct vulkan_funcs *driver_funcs)
{
    ERR( "Wine was built without Vulkan support.\n" );
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* SONAME_LIBVULKAN */
