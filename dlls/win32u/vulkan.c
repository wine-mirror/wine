/*
 * Vulkan display driver loading
 *
 * Copyright (c) 2017 Roderick Colenbrander
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
#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "win32u_private.h"
#include "ntuser_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

PFN_vkGetDeviceProcAddr p_vkGetDeviceProcAddr = NULL;
PFN_vkGetInstanceProcAddr p_vkGetInstanceProcAddr = NULL;

static void *vulkan_handle;
static struct vulkan_funcs vulkan_funcs;

#ifdef SONAME_LIBVULKAN

WINE_DECLARE_DEBUG_CHANNEL(fps);

static const struct vulkan_driver_funcs *driver_funcs;

struct surface
{
    struct vulkan_surface obj;
    struct client_surface *client;
    HWND hwnd;
};

static struct surface *surface_from_handle( VkSurfaceKHR handle )
{
    struct vulkan_surface *obj = vulkan_surface_from_handle( handle );
    return CONTAINING_RECORD( obj, struct surface, obj );
}

struct swapchain
{
    struct vulkan_swapchain obj;
    struct surface *surface;
    VkExtent2D extents;
};

static struct swapchain *swapchain_from_handle( VkSwapchainKHR handle )
{
    struct vulkan_swapchain *obj = vulkan_swapchain_from_handle( handle );
    return CONTAINING_RECORD( obj, struct swapchain, obj );
}

static VkResult win32u_vkCreateWin32SurfaceKHR( VkInstance client_instance, const VkWin32SurfaceCreateInfoKHR *create_info,
                                                const VkAllocationCallbacks *allocator, VkSurfaceKHR *ret )
{
    struct vulkan_instance *instance = vulkan_instance_from_handle( client_instance );
    VkSurfaceKHR host_surface;
    struct surface *surface;
    HWND dummy = NULL;
    VkResult res;

    TRACE( "client_instance %p, create_info %p, allocator %p, ret %p\n", client_instance, create_info, allocator, ret );
    if (allocator) FIXME( "Support for allocation callbacks not implemented yet\n" );

    if (!(surface = calloc( 1, sizeof(*surface) ))) return VK_ERROR_OUT_OF_HOST_MEMORY;

    /* Windows allows surfaces to be created with no HWND, they return VK_ERROR_SURFACE_LOST_KHR later */
    if (!(surface->hwnd = create_info->hwnd))
    {
        static const WCHAR staticW[] = {'s','t','a','t','i','c',0};
        UNICODE_STRING static_us = RTL_CONSTANT_STRING( staticW );
        dummy = NtUserCreateWindowEx( 0, &static_us, &static_us, &static_us, WS_POPUP, 0, 0, 0, 0,
                                      NULL, NULL, NULL, NULL, 0, NULL, 0, FALSE );
        WARN( "Created dummy window %p for null surface window\n", dummy );
        surface->hwnd = dummy;
    }

    if ((res = driver_funcs->p_vulkan_surface_create( surface->hwnd, instance, &host_surface, &surface->client )))
    {
        if (dummy) NtUserDestroyWindow( dummy );
        free( surface );
        return res;
    }

    vulkan_object_init( &surface->obj.obj, host_surface );
    surface->obj.instance = instance;
    instance->p_insert_object( instance, &surface->obj.obj );

    if (dummy) NtUserDestroyWindow( dummy );

    *ret = surface->obj.client.surface;
    return VK_SUCCESS;
}

static void win32u_vkDestroySurfaceKHR( VkInstance client_instance, VkSurfaceKHR client_surface,
                                        const VkAllocationCallbacks *allocator )
{
    struct vulkan_instance *instance = vulkan_instance_from_handle( client_instance );
    struct surface *surface = surface_from_handle( client_surface );

    if (!surface) return;

    TRACE( "instance %p, handle 0x%s, allocator %p\n", instance, wine_dbgstr_longlong( client_surface ), allocator );
    if (allocator) FIXME( "Support for allocation callbacks not implemented yet\n" );

    instance->p_vkDestroySurfaceKHR( instance->host.instance, surface->obj.host.surface, NULL /* allocator */ );
    client_surface_release( surface->client );

    instance->p_remove_object( instance, &surface->obj.obj );

    free( surface );
}

static void adjust_surface_capabilities( struct vulkan_instance *instance, struct surface *surface,
                                         VkSurfaceCapabilitiesKHR *capabilities )
{
    RECT client_rect;

    /* Many Windows games, for example Strange Brigade, No Man's Sky, Path of Exile
     * and World War Z, do not expect that maxImageCount can be set to 0.
     * A value of 0 means that there is no limit on the number of images.
     * Nvidia reports 8 on Windows, AMD 16.
     * https://vulkan.gpuinfo.org/displayreport.php?id=9122#surface
     * https://vulkan.gpuinfo.org/displayreport.php?id=9121#surface
     */
    if (!capabilities->maxImageCount) capabilities->maxImageCount = max( capabilities->minImageCount, 16 );

    /* Update the image extents to match what the Win32 WSI would provide. */
    /* FIXME: handle DPI scaling, somehow */
    NtUserGetClientRect( surface->hwnd, &client_rect, NtUserGetDpiForWindow( surface->hwnd ) );
    capabilities->minImageExtent.width = client_rect.right - client_rect.left;
    capabilities->minImageExtent.height = client_rect.bottom - client_rect.top;
    capabilities->maxImageExtent.width = client_rect.right - client_rect.left;
    capabilities->maxImageExtent.height = client_rect.bottom - client_rect.top;
    capabilities->currentExtent.width = client_rect.right - client_rect.left;
    capabilities->currentExtent.height = client_rect.bottom - client_rect.top;
}

static VkResult win32u_vkGetPhysicalDeviceSurfaceCapabilitiesKHR( VkPhysicalDevice client_physical_device, VkSurfaceKHR client_surface,
                                                                  VkSurfaceCapabilitiesKHR *capabilities )
{
    struct vulkan_physical_device *physical_device = vulkan_physical_device_from_handle( client_physical_device );
    struct surface *surface = surface_from_handle( client_surface );
    struct vulkan_instance *instance = physical_device->instance;
    VkResult res;

    if (!NtUserIsWindow( surface->hwnd )) return VK_ERROR_SURFACE_LOST_KHR;
    res = instance->p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR( physical_device->host.physical_device,
                                                       surface->obj.host.surface, capabilities );
    if (!res) adjust_surface_capabilities( instance, surface, capabilities );
    return res;
}

static VkResult win32u_vkGetPhysicalDeviceSurfaceCapabilities2KHR( VkPhysicalDevice client_physical_device, const VkPhysicalDeviceSurfaceInfo2KHR *surface_info,
                                                                   VkSurfaceCapabilities2KHR *capabilities )
{
    struct vulkan_physical_device *physical_device = vulkan_physical_device_from_handle( client_physical_device );
    struct surface *surface = surface_from_handle( surface_info->surface );
    VkPhysicalDeviceSurfaceInfo2KHR surface_info_host = *surface_info;
    struct vulkan_instance *instance = physical_device->instance;
    VkResult res;

    if (!instance->p_vkGetPhysicalDeviceSurfaceCapabilities2KHR)
    {
        /* Until the loader version exporting this function is common, emulate it using the older non-2 version. */
        if (surface_info->pNext || capabilities->pNext) FIXME( "Emulating vkGetPhysicalDeviceSurfaceCapabilities2KHR, ignoring pNext.\n" );
        return win32u_vkGetPhysicalDeviceSurfaceCapabilitiesKHR( client_physical_device, surface_info->surface,
                                                                 &capabilities->surfaceCapabilities );
    }

    surface_info_host.surface = surface->obj.host.surface;

    if (!NtUserIsWindow( surface->hwnd )) return VK_ERROR_SURFACE_LOST_KHR;
    res = instance->p_vkGetPhysicalDeviceSurfaceCapabilities2KHR( physical_device->host.physical_device,
                                                                     &surface_info_host, capabilities );
    if (!res) adjust_surface_capabilities( instance, surface, &capabilities->surfaceCapabilities );
    return res;
}

static VkResult win32u_vkGetPhysicalDevicePresentRectanglesKHR( VkPhysicalDevice client_physical_device, VkSurfaceKHR client_surface,
                                                                uint32_t *rect_count, VkRect2D *rects )
{
    struct vulkan_physical_device *physical_device = vulkan_physical_device_from_handle( client_physical_device );
    struct surface *surface = surface_from_handle( client_surface );
    struct vulkan_instance *instance = physical_device->instance;

    if (!NtUserIsWindow( surface->hwnd ))
    {
        if (rects && !*rect_count) return VK_INCOMPLETE;
        if (rects) memset( rects, 0, sizeof(VkRect2D) );
        *rect_count = 1;
        return VK_SUCCESS;
    }

    return instance->p_vkGetPhysicalDevicePresentRectanglesKHR( physical_device->host.physical_device,
                                                                   surface->obj.host.surface, rect_count, rects );
}

static VkResult win32u_vkGetPhysicalDeviceSurfaceFormatsKHR( VkPhysicalDevice client_physical_device, VkSurfaceKHR client_surface,
                                                             uint32_t *format_count, VkSurfaceFormatKHR *formats )
{
    struct vulkan_physical_device *physical_device = vulkan_physical_device_from_handle( client_physical_device );
    struct surface *surface = surface_from_handle( client_surface );
    struct vulkan_instance *instance = physical_device->instance;

    return instance->p_vkGetPhysicalDeviceSurfaceFormatsKHR( physical_device->host.physical_device,
                                                                surface->obj.host.surface, format_count, formats );
}

static VkResult win32u_vkGetPhysicalDeviceSurfaceFormats2KHR( VkPhysicalDevice client_physical_device, const VkPhysicalDeviceSurfaceInfo2KHR *surface_info,
                                                              uint32_t *format_count, VkSurfaceFormat2KHR *formats )
{
    struct vulkan_physical_device *physical_device = vulkan_physical_device_from_handle( client_physical_device );
    struct surface *surface = surface_from_handle( surface_info->surface );
    VkPhysicalDeviceSurfaceInfo2KHR surface_info_host = *surface_info;
    struct vulkan_instance *instance = physical_device->instance;
    VkResult res;

    if (!instance->p_vkGetPhysicalDeviceSurfaceFormats2KHR)
    {
        VkSurfaceFormatKHR *surface_formats;
        UINT i;

        /* Until the loader version exporting this function is common, emulate it using the older non-2 version. */
        if (surface_info->pNext) FIXME( "Emulating vkGetPhysicalDeviceSurfaceFormats2KHR, ignoring pNext.\n" );
        if (!formats) return win32u_vkGetPhysicalDeviceSurfaceFormatsKHR( client_physical_device, surface_info->surface, format_count, NULL );

        surface_formats = calloc( *format_count, sizeof(*surface_formats) );
        if (!surface_formats) return VK_ERROR_OUT_OF_HOST_MEMORY;

        res = win32u_vkGetPhysicalDeviceSurfaceFormatsKHR( client_physical_device, surface_info->surface, format_count, surface_formats );
        if (!res || res == VK_INCOMPLETE) for (i = 0; i < *format_count; i++) formats[i].surfaceFormat = surface_formats[i];

        free( surface_formats );
        return res;
    }

    surface_info_host.surface = surface->obj.host.surface;

    return instance->p_vkGetPhysicalDeviceSurfaceFormats2KHR( physical_device->host.physical_device,
                                                                 &surface_info_host, format_count, formats );
}

static VkBool32 win32u_vkGetPhysicalDeviceWin32PresentationSupportKHR( VkPhysicalDevice client_physical_device, uint32_t queue )
{
    struct vulkan_physical_device *physical_device = vulkan_physical_device_from_handle( client_physical_device );
    return driver_funcs->p_vkGetPhysicalDeviceWin32PresentationSupportKHR( physical_device->host.physical_device, queue );
}

static BOOL extents_equals( const VkExtent2D *extents, const RECT *rect )
{
    return extents->width == rect->right - rect->left && extents->height == rect->bottom - rect->top;
}

static VkResult win32u_vkCreateSwapchainKHR( VkDevice client_device, const VkSwapchainCreateInfoKHR *create_info,
                                             const VkAllocationCallbacks *allocator, VkSwapchainKHR *ret )
{
    VkSwapchainPresentScalingCreateInfoEXT scaling = {.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_SCALING_CREATE_INFO_EXT};
    struct swapchain *swapchain, *old_swapchain = swapchain_from_handle( create_info->oldSwapchain );
    struct surface *surface = surface_from_handle( create_info->surface );
    struct vulkan_device *device = vulkan_device_from_handle( client_device );
    struct vulkan_physical_device *physical_device = device->physical_device;
    struct vulkan_instance *instance = physical_device->instance;
    VkSwapchainCreateInfoKHR create_info_host = *create_info;
    VkSurfaceCapabilitiesKHR capabilities;
    VkSwapchainKHR host_swapchain;
    RECT client_rect;
    VkResult res;

    if (!NtUserIsWindow( surface->hwnd ))
    {
        ERR( "surface %p, hwnd %p is invalid!\n", surface, surface->hwnd );
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (surface) create_info_host.surface = surface->obj.host.surface;
    if (old_swapchain) create_info_host.oldSwapchain = old_swapchain->obj.host.swapchain;

    /* Windows allows client rect to be empty, but host Vulkan often doesn't, adjust extents back to the host capabilities */
    res = instance->p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR( physical_device->host.physical_device, surface->obj.host.surface, &capabilities );
    if (res) return res;

    create_info_host.imageExtent.width = max( create_info_host.imageExtent.width, capabilities.minImageExtent.width );
    create_info_host.imageExtent.height = max( create_info_host.imageExtent.height, capabilities.minImageExtent.height );

    /* If the swapchain image size is not equal to the presentation size (e.g. because of DPI virtualization or
     * display mode change emulation), MoltenVK's vkQueuePresentKHR returns VK_SUBOPTIMAL_KHR.
     * Create the swapchain with VkSwapchainPresentScalingCreateInfoEXT to avoid this.
     */
    if (NtUserGetClientRect( surface->hwnd, &client_rect, NtUserGetWinMonitorDpi( surface->hwnd, MDT_RAW_DPI ) ) &&
        !extents_equals( &create_info_host.imageExtent, &client_rect ) &&
        physical_device->has_swapchain_maintenance1)
    {
        scaling.scalingBehavior = VK_PRESENT_SCALING_STRETCH_BIT_EXT;
        create_info_host.pNext = &scaling;
    }

    if (!(swapchain = calloc( 1, sizeof(*swapchain) ))) return VK_ERROR_OUT_OF_HOST_MEMORY;

    if ((res = device->p_vkCreateSwapchainKHR( device->host.device, &create_info_host, NULL, &host_swapchain )))
    {
        free( swapchain );
        return res;
    }

    vulkan_object_init( &swapchain->obj.obj, host_swapchain );
    swapchain->surface = surface;
    swapchain->extents = create_info->imageExtent;
    instance->p_insert_object( instance, &swapchain->obj.obj );

    *ret = swapchain->obj.client.swapchain;
    return VK_SUCCESS;
}

void win32u_vkDestroySwapchainKHR( VkDevice client_device, VkSwapchainKHR client_swapchain,
                                   const VkAllocationCallbacks *allocator )
{
    struct vulkan_device *device = vulkan_device_from_handle( client_device );
    struct vulkan_instance *instance = device->physical_device->instance;
    struct swapchain *swapchain = swapchain_from_handle( client_swapchain );

    if (allocator) FIXME( "Support for allocation callbacks not implemented yet\n" );
    if (!swapchain) return;

    device->p_vkDestroySwapchainKHR( device->host.device, swapchain->obj.host.swapchain, NULL );
    instance->p_remove_object( instance, &swapchain->obj.obj );

    free( swapchain );
}

static VkResult win32u_vkAcquireNextImage2KHR( VkDevice client_device, const VkAcquireNextImageInfoKHR *acquire_info,
                                               uint32_t *image_index )
{
    struct swapchain *swapchain = swapchain_from_handle( acquire_info->swapchain );
    struct vulkan_device *device = vulkan_device_from_handle( client_device );
    VkAcquireNextImageInfoKHR acquire_info_host = *acquire_info;
    struct surface *surface = swapchain->surface;
    RECT client_rect;
    VkResult res;

    acquire_info_host.swapchain = swapchain->obj.host.swapchain;

    res = device->p_vkAcquireNextImage2KHR( device->host.device, &acquire_info_host, image_index );

    if (!res && NtUserGetClientRect( surface->hwnd, &client_rect, NtUserGetDpiForWindow( surface->hwnd ) ) &&
        !extents_equals( &swapchain->extents, &client_rect ))
    {
        WARN( "Swapchain size %dx%d does not match client rect %s, returning VK_SUBOPTIMAL_KHR\n",
              swapchain->extents.width, swapchain->extents.height, wine_dbgstr_rect( &client_rect ) );
        return VK_SUBOPTIMAL_KHR;
    }

    return res;
}

static VkResult win32u_vkAcquireNextImageKHR( VkDevice client_device, VkSwapchainKHR client_swapchain, uint64_t timeout,
                                              VkSemaphore semaphore, VkFence fence, uint32_t *image_index )
{
    struct swapchain *swapchain = swapchain_from_handle( client_swapchain );
    struct vulkan_device *device = vulkan_device_from_handle( client_device );
    struct surface *surface = swapchain->surface;
    RECT client_rect;
    VkResult res;

    res = device->p_vkAcquireNextImageKHR( device->host.device, swapchain->obj.host.swapchain, timeout,
                                              semaphore, fence, image_index );

    if (!res && NtUserGetClientRect( surface->hwnd, &client_rect, NtUserGetDpiForWindow( surface->hwnd ) ) &&
        !extents_equals( &swapchain->extents, &client_rect ))
    {
        WARN( "Swapchain size %dx%d does not match client rect %s, returning VK_SUBOPTIMAL_KHR\n",
              swapchain->extents.width, swapchain->extents.height, wine_dbgstr_rect( &client_rect ) );
        return VK_SUBOPTIMAL_KHR;
    }

    return res;
}

static VkResult win32u_vkQueuePresentKHR( VkQueue client_queue, const VkPresentInfoKHR *present_info )
{
    struct vulkan_queue *queue = vulkan_queue_from_handle( client_queue );
    VkSwapchainKHR swapchains_buffer[16], *swapchains = swapchains_buffer;
    VkPresentInfoKHR present_info_host = *present_info;
    struct vulkan_device *device = queue->device;
    VkResult res;
    UINT i;

    TRACE( "queue %p, present_info %p\n", queue, present_info );

    if (present_info->swapchainCount > ARRAY_SIZE(swapchains_buffer) &&
        !(swapchains = malloc( present_info->swapchainCount * sizeof(*swapchains) )))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    for (i = 0; i < present_info->swapchainCount; i++)
    {
        struct swapchain *swapchain = swapchain_from_handle( present_info->pSwapchains[i] );
        swapchains[i] = swapchain->obj.host.swapchain;
    }

    present_info_host.pSwapchains = swapchains;

    res = device->p_vkQueuePresentKHR( queue->host.queue, &present_info_host );

    for (i = 0; i < present_info->swapchainCount; i++)
    {
        struct swapchain *swapchain = swapchain_from_handle( present_info->pSwapchains[i] );
        VkResult swapchain_res = present_info->pResults ? present_info->pResults[i] : res;
        struct surface *surface = swapchain->surface;
        RECT client_rect;

        client_surface_present( surface->client, NULL );

        if (swapchain_res < VK_SUCCESS) continue;
        if (!NtUserGetClientRect( surface->hwnd, &client_rect, NtUserGetDpiForWindow( surface->hwnd ) ))
        {
            WARN( "Swapchain window %p is invalid, returning VK_ERROR_OUT_OF_DATE_KHR\n", surface->hwnd );
            if (present_info->pResults) present_info->pResults[i] = VK_ERROR_OUT_OF_DATE_KHR;
            if (res >= VK_SUCCESS) res = VK_ERROR_OUT_OF_DATE_KHR;
        }
        else if (swapchain_res)
            WARN( "Present returned status %d for swapchain %p\n", swapchain_res, swapchain );
        else if (!extents_equals( &swapchain->extents, &client_rect ))
        {
            WARN( "Swapchain size %dx%d does not match client rect %s, returning VK_SUBOPTIMAL_KHR\n",
                  swapchain->extents.width, swapchain->extents.height, wine_dbgstr_rect( &client_rect ) );
            if (present_info->pResults) present_info->pResults[i] = VK_SUBOPTIMAL_KHR;
            if (!res) res = VK_SUBOPTIMAL_KHR;
        }
    }

    if (swapchains != swapchains_buffer) free( swapchains );

    if (TRACE_ON( fps ))
    {
        static unsigned long frames, frames_total;
        static long prev_time, start_time;
        DWORD time;

        time = NtGetTickCount();
        frames++;
        frames_total++;

        if (time - prev_time > 1500)
        {
            TRACE_(fps)( "%p @ approx %.2ffps, total %.2ffps\n", queue, 1000.0 * frames / (time - prev_time),
                         1000.0 * frames_total / (time - start_time) );
            prev_time = time;
            frames = 0;

            if (!start_time) start_time = time;
        }
    }

    return res;
}

static const char *win32u_get_host_surface_extension(void)
{
    return driver_funcs->p_get_host_surface_extension();
}

static struct vulkan_funcs vulkan_funcs =
{
    .p_vkCreateWin32SurfaceKHR = win32u_vkCreateWin32SurfaceKHR,
    .p_vkDestroySurfaceKHR = win32u_vkDestroySurfaceKHR,
    .p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR = win32u_vkGetPhysicalDeviceSurfaceCapabilitiesKHR,
    .p_vkGetPhysicalDeviceSurfaceCapabilities2KHR = win32u_vkGetPhysicalDeviceSurfaceCapabilities2KHR,
    .p_vkGetPhysicalDevicePresentRectanglesKHR = win32u_vkGetPhysicalDevicePresentRectanglesKHR,
    .p_vkGetPhysicalDeviceSurfaceFormatsKHR = win32u_vkGetPhysicalDeviceSurfaceFormatsKHR,
    .p_vkGetPhysicalDeviceSurfaceFormats2KHR = win32u_vkGetPhysicalDeviceSurfaceFormats2KHR,
    .p_vkGetPhysicalDeviceWin32PresentationSupportKHR = win32u_vkGetPhysicalDeviceWin32PresentationSupportKHR,
    .p_vkCreateSwapchainKHR = win32u_vkCreateSwapchainKHR,
    .p_vkDestroySwapchainKHR = win32u_vkDestroySwapchainKHR,
    .p_vkAcquireNextImage2KHR = win32u_vkAcquireNextImage2KHR,
    .p_vkAcquireNextImageKHR = win32u_vkAcquireNextImageKHR,
    .p_vkQueuePresentKHR = win32u_vkQueuePresentKHR,
    .p_get_host_surface_extension = win32u_get_host_surface_extension,
};

static const struct client_surface_funcs nulldrv_vulkan_surface_funcs;

static VkResult nulldrv_vulkan_surface_create( HWND hwnd, const struct vulkan_instance *instance, VkSurfaceKHR *surface,
                                               struct client_surface **client )
{
    VkHeadlessSurfaceCreateInfoEXT create_info = {.sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT};
    VkResult res;

    if (!(*client = client_surface_create(sizeof(**client), &nulldrv_vulkan_surface_funcs, hwnd))) return VK_ERROR_OUT_OF_HOST_MEMORY;
    if ((res = instance->p_vkCreateHeadlessSurfaceEXT( instance->host.instance, &create_info, NULL, surface )))
    {
        client_surface_release(*client);
        *client = NULL;
    }

    return res;
}

static void nulldrv_vulkan_surface_destroy( struct client_surface *client )
{
}

static void nulldrv_vulkan_surface_detach( struct client_surface *client )
{
}

static void nulldrv_vulkan_surface_update( struct client_surface *client )
{
}

static void nulldrv_vulkan_surface_present( struct client_surface *client, HDC hdc )
{
}

static VkBool32 nulldrv_vkGetPhysicalDeviceWin32PresentationSupportKHR( VkPhysicalDevice device, uint32_t queue )
{
    return VK_TRUE;
}

static const char *nulldrv_get_host_surface_extension(void)
{
    return "VK_EXT_headless_surface";
}

static const struct client_surface_funcs nulldrv_vulkan_surface_funcs =
{
    .destroy = nulldrv_vulkan_surface_destroy,
    .detach = nulldrv_vulkan_surface_detach,
    .update = nulldrv_vulkan_surface_update,
    .present = nulldrv_vulkan_surface_present,
};

static const struct vulkan_driver_funcs nulldrv_funcs =
{
    .p_vulkan_surface_create = nulldrv_vulkan_surface_create,
    .p_vkGetPhysicalDeviceWin32PresentationSupportKHR = nulldrv_vkGetPhysicalDeviceWin32PresentationSupportKHR,
    .p_get_host_surface_extension = nulldrv_get_host_surface_extension,
};

static void vulkan_driver_init(void)
{
    UINT status;

    if ((status = user_driver->pVulkanInit( WINE_VULKAN_DRIVER_VERSION, vulkan_handle, &driver_funcs )) &&
        status != STATUS_NOT_IMPLEMENTED)
    {
        ERR( "Failed to initialize the driver vulkan functions, status %#x\n", status );
        return;
    }

    if (status == STATUS_NOT_IMPLEMENTED) driver_funcs = &nulldrv_funcs;
    else vulkan_funcs.p_get_host_surface_extension = driver_funcs->p_get_host_surface_extension;
}

static void vulkan_driver_load(void)
{
    static pthread_once_t init_once = PTHREAD_ONCE_INIT;
    pthread_once( &init_once, vulkan_driver_init );
}

static VkResult lazydrv_vulkan_surface_create( HWND hwnd, const struct vulkan_instance *instance, VkSurfaceKHR *surface,
                                               struct client_surface **client )
{
    vulkan_driver_load();
    return driver_funcs->p_vulkan_surface_create( hwnd, instance, surface, client );
}

static VkBool32 lazydrv_vkGetPhysicalDeviceWin32PresentationSupportKHR( VkPhysicalDevice device, uint32_t queue )
{
    vulkan_driver_load();
    return driver_funcs->p_vkGetPhysicalDeviceWin32PresentationSupportKHR( device, queue );
}

static const char *lazydrv_get_host_surface_extension(void)
{
    vulkan_driver_load();
    return driver_funcs->p_get_host_surface_extension();
}

static const struct vulkan_driver_funcs lazydrv_funcs =
{
    .p_vulkan_surface_create = lazydrv_vulkan_surface_create,
    .p_vkGetPhysicalDeviceWin32PresentationSupportKHR = lazydrv_vkGetPhysicalDeviceWin32PresentationSupportKHR,
    .p_get_host_surface_extension = lazydrv_get_host_surface_extension,
};

static void vulkan_init_once(void)
{
    if (!(vulkan_handle = dlopen( SONAME_LIBVULKAN, RTLD_NOW )))
    {
        ERR( "Failed to load %s\n", SONAME_LIBVULKAN );
        return;
    }

#define LOAD_FUNCPTR( f )                                                                          \
    if (!(p_##f = dlsym( vulkan_handle, #f )))                                                     \
    {                                                                                              \
        ERR( "Failed to find " #f "\n" );                                                          \
        dlclose( vulkan_handle );                                                                  \
        vulkan_handle = NULL;                                                                      \
        return;                                                                                    \
    }

    LOAD_FUNCPTR( vkGetDeviceProcAddr );
    LOAD_FUNCPTR( vkGetInstanceProcAddr );
#undef LOAD_FUNCPTR

    driver_funcs = &lazydrv_funcs;
    vulkan_funcs.p_vkGetInstanceProcAddr = p_vkGetInstanceProcAddr;
    vulkan_funcs.p_vkGetDeviceProcAddr = p_vkGetDeviceProcAddr;
}

#else /* SONAME_LIBVULKAN */

static void vulkan_init_once(void)
{
    ERR( "Wine was built without Vulkan support.\n" );
}

#endif /* SONAME_LIBVULKAN */

BOOL vulkan_init(void)
{
    static pthread_once_t init_once = PTHREAD_ONCE_INIT;
    pthread_once( &init_once, vulkan_init_once );
    return !!vulkan_handle;
}

/***********************************************************************
 *      __wine_get_vulkan_driver  (win32u.so)
 */
const struct vulkan_funcs *__wine_get_vulkan_driver( UINT version )
{
    if (version != WINE_VULKAN_DRIVER_VERSION)
    {
        ERR( "version mismatch, vulkan wants %u but win32u has %u\n", version, WINE_VULKAN_DRIVER_VERSION );
        return NULL;
    }

    if (!vulkan_init()) return NULL;
    return &vulkan_funcs;
}
