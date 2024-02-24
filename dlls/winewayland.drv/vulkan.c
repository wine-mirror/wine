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

static VkResult (*pvkCreateSwapchainKHR)(VkDevice, const VkSwapchainCreateInfoKHR *, const VkAllocationCallbacks *, VkSwapchainKHR *);
static VkResult (*pvkCreateWaylandSurfaceKHR)(VkInstance, const VkWaylandSurfaceCreateInfoKHR *, const VkAllocationCallbacks *, VkSurfaceKHR *);
static void (*pvkDestroySurfaceKHR)(VkInstance, VkSurfaceKHR, const VkAllocationCallbacks *);
static void (*pvkDestroySwapchainKHR)(VkDevice, VkSwapchainKHR, const VkAllocationCallbacks *);
static VkBool32 (*pvkGetPhysicalDeviceWaylandPresentationSupportKHR)(VkPhysicalDevice, uint32_t, struct wl_display *);
static VkResult (*pvkQueuePresentKHR)(VkQueue, const VkPresentInfoKHR *);

static const struct vulkan_funcs vulkan_funcs;

static pthread_mutex_t wine_vk_swapchain_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct list wine_vk_swapchain_list = LIST_INIT(wine_vk_swapchain_list);

struct wine_vk_surface
{
    struct wayland_client_surface *client;
    VkSurfaceKHR host_surface;
};

struct wine_vk_swapchain
{
    struct list entry;
    VkSwapchainKHR host_swapchain;
    HWND hwnd;
    VkExtent2D extent;
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

static BOOL wine_vk_surface_is_valid(struct wine_vk_surface *wine_vk_surface)
{
    HWND hwnd = wine_vk_surface_get_hwnd(wine_vk_surface);
    struct wayland_surface *wayland_surface;

    if ((wayland_surface = wayland_surface_lock_hwnd(hwnd)))
    {
        pthread_mutex_unlock(&wayland_surface->mutex);
        return TRUE;
    }

    return FALSE;
}

static struct wine_vk_swapchain *wine_vk_swapchain_from_handle(VkSwapchainKHR handle)
{
    struct wine_vk_swapchain *wine_vk_swapchain;

    pthread_mutex_lock(&wine_vk_swapchain_mutex);
    LIST_FOR_EACH_ENTRY(wine_vk_swapchain, &wine_vk_swapchain_list,
                        struct wine_vk_swapchain, entry)
    {
        if (wine_vk_swapchain->host_swapchain == handle)
        {
            pthread_mutex_unlock(&wine_vk_swapchain_mutex);
            return wine_vk_swapchain;
        }
    }
    pthread_mutex_unlock(&wine_vk_swapchain_mutex);

    return NULL;
}

static void vk_result_update_out_of_date(VkResult *res)
{
    /* If the current result is less severe than out_of_date, which for
     * now applies to all non-failure results, update it.
     * TODO: Handle VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT when
     * it is supported by winevulkan, since it's also considered
     * less severe than out_of_date. */
    if (*res >= 0) *res = VK_ERROR_OUT_OF_DATE_KHR;
}

static VkResult check_queue_present(const VkPresentInfoKHR *present_info,
                                    VkResult present_res)
{
    VkResult res = present_res;
    uint32_t i;

    for (i = 0; i < present_info->swapchainCount; ++i)
    {
        struct wine_vk_swapchain *wine_vk_swapchain =
            wine_vk_swapchain_from_handle(present_info->pSwapchains[i]);
        HWND hwnd = wine_vk_swapchain->hwnd;
        struct wayland_surface *wayland_surface;

        if ((wayland_surface = wayland_surface_lock_hwnd(hwnd)))
        {
            int client_width = wayland_surface->window.client_rect.right -
                               wayland_surface->window.client_rect.left;
            int client_height = wayland_surface->window.client_rect.bottom -
                                wayland_surface->window.client_rect.top;

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

            if (client_width == wine_vk_swapchain->extent.width &&
                client_height == wine_vk_swapchain->extent.height)
            {
                /* The window is still available and matches the swapchain size,
                 * so there is no new error to report. */
                continue;
            }
        }

        /* We use the out_of_date error even if the window is no longer
         * available, to match win32 behavior (e.g., nvidia). The application
         * will get surface_lost when it tries to recreate the swapchain. */
        if (present_info->pResults)
           vk_result_update_out_of_date(&present_info->pResults[i]);
        vk_result_update_out_of_date(&res);
    }

    return res;
}

static VkResult wayland_vkCreateSwapchainKHR(VkDevice device,
                                             const VkSwapchainCreateInfoKHR *create_info,
                                             const VkAllocationCallbacks *allocator,
                                             VkSwapchainKHR *swapchain)
{
    VkResult res;
    VkSwapchainCreateInfoKHR create_info_host;
    struct wine_vk_surface *wine_vk_surface;
    struct wine_vk_swapchain *wine_vk_swapchain;

    TRACE("%p %p %p %p\n", device, create_info, allocator, swapchain);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    wine_vk_surface = wine_vk_surface_from_handle(create_info->surface);
    if (!wine_vk_surface_is_valid(wine_vk_surface))
        return VK_ERROR_SURFACE_LOST_KHR;

    wine_vk_swapchain = calloc(1, sizeof(*wine_vk_swapchain));
    if (!wine_vk_swapchain)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    wine_vk_swapchain->hwnd = wine_vk_surface_get_hwnd(wine_vk_surface);
    wine_vk_swapchain->extent = create_info->imageExtent;

    create_info_host = *create_info;
    create_info_host.surface = wine_vk_surface->host_surface;

    res = pvkCreateSwapchainKHR(device, &create_info_host, NULL /* allocator */,
                                swapchain);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to create vulkan wayland swapchain, res=%d\n", res);
        free(wine_vk_swapchain);
        return res;
    }

    wine_vk_swapchain->host_swapchain = *swapchain;

    pthread_mutex_lock(&wine_vk_swapchain_mutex);
    list_add_head(&wine_vk_swapchain_list, &wine_vk_swapchain->entry);
    pthread_mutex_unlock(&wine_vk_swapchain_mutex);

    TRACE("Created swapchain=0x%s\n", wine_dbgstr_longlong(*swapchain));
    return res;
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

static void wayland_vkDestroySwapchainKHR(VkDevice device,
                                          VkSwapchainKHR swapchain,
                                          const VkAllocationCallbacks *allocator)
{
    struct wine_vk_swapchain *wine_vk_swapchain;

    TRACE("%p, 0x%s %p\n", device, wine_dbgstr_longlong(swapchain), allocator);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    pvkDestroySwapchainKHR(device, swapchain, NULL /* allocator */);

    if ((wine_vk_swapchain = wine_vk_swapchain_from_handle(swapchain)))
    {
        pthread_mutex_lock(&wine_vk_swapchain_mutex);
        list_remove(&wine_vk_swapchain->entry);
        pthread_mutex_unlock(&wine_vk_swapchain_mutex);
        free(wine_vk_swapchain);
    }
}

static VkBool32 wayland_vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice phys_dev,
                                                                       uint32_t index)
{
    TRACE("%p %u\n", phys_dev, index);

    return pvkGetPhysicalDeviceWaylandPresentationSupportKHR(phys_dev, index,
                                                             process_wayland.wl_display);
}

static VkResult wayland_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *present_info)
{
    VkResult res;

    TRACE("%p, %p\n", queue, present_info);

    res = pvkQueuePresentKHR(queue, present_info);

    return check_queue_present(present_info, res);
}

static const char *wayland_get_host_surface_extension(void)
{
    return "VK_KHR_wayland_surface";
}

static VkSurfaceKHR wayland_wine_get_host_surface(VkSurfaceKHR surface)
{
    return wine_vk_surface_from_handle(surface)->host_surface;
}

static const struct vulkan_funcs vulkan_funcs =
{
    .p_vkCreateSwapchainKHR = wayland_vkCreateSwapchainKHR,
    .p_vkCreateWin32SurfaceKHR = wayland_vkCreateWin32SurfaceKHR,
    .p_vkDestroySurfaceKHR = wayland_vkDestroySurfaceKHR,
    .p_vkDestroySwapchainKHR = wayland_vkDestroySwapchainKHR,
    .p_vkGetPhysicalDeviceWin32PresentationSupportKHR = wayland_vkGetPhysicalDeviceWin32PresentationSupportKHR,
    .p_vkQueuePresentKHR = wayland_vkQueuePresentKHR,
    .p_get_host_surface_extension = wayland_get_host_surface_extension,
    .p_wine_get_host_surface = wayland_wine_get_host_surface,
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
    LOAD_FUNCPTR(vkCreateSwapchainKHR);
    LOAD_FUNCPTR(vkCreateWaylandSurfaceKHR);
    LOAD_FUNCPTR(vkDestroySurfaceKHR);
    LOAD_FUNCPTR(vkDestroySwapchainKHR);
    LOAD_FUNCPTR(vkGetPhysicalDeviceWaylandPresentationSupportKHR);
    LOAD_FUNCPTR(vkQueuePresentKHR);
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
