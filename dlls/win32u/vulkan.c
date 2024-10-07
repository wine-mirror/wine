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

#define VK_NO_PROTOTYPES
#define WINE_VK_HOST
#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

void *(*p_vkGetDeviceProcAddr)(VkDevice, const char *) = NULL;
void *(*p_vkGetInstanceProcAddr)(VkInstance, const char *) = NULL;

static void *vulkan_handle;
static struct vulkan_funcs vulkan_funcs;

#ifdef SONAME_LIBVULKAN

static const struct vulkan_driver_funcs *driver_funcs;

static void (*p_vkDestroySurfaceKHR)(VkInstance, VkSurfaceKHR, const VkAllocationCallbacks *);
static VkResult (*p_vkQueuePresentKHR)(VkQueue, const VkPresentInfoKHR *);

struct surface
{
    struct list entry;
    VkSurfaceKHR host_surface;
    void *driver_private;
    HWND hwnd;
};

static inline struct surface *surface_from_handle( VkSurfaceKHR handle )
{
    return (struct surface *)(uintptr_t)handle;
}

static inline VkSurfaceKHR surface_to_handle( struct surface *surface )
{
    return (VkSurfaceKHR)(uintptr_t)surface;
}

static VkResult win32u_vkCreateWin32SurfaceKHR( VkInstance instance, const VkWin32SurfaceCreateInfoKHR *info,
                                                const VkAllocationCallbacks *allocator, VkSurfaceKHR *handle )
{
    struct surface *surface;
    VkResult res;
    WND *win;

    TRACE( "instance %p, info %p, allocator %p, handle %p\n", instance, info, allocator, handle );
    if (allocator) FIXME( "Support for allocation callbacks not implemented yet\n" );

    if (!(surface = calloc( 1, sizeof(*surface) ))) return VK_ERROR_OUT_OF_HOST_MEMORY;
    if ((res = driver_funcs->p_vulkan_surface_create( info->hwnd, instance, &surface->host_surface, &surface->driver_private )))
    {
        free( surface );
        return res;
    }

    if (!(win = get_win_ptr( info->hwnd )) || win == WND_DESKTOP || win == WND_OTHER_PROCESS)
        list_init( &surface->entry );
    else
    {
        list_add_tail( &win->vulkan_surfaces, &surface->entry );
        release_win_ptr( win );
    }

    surface->hwnd = info->hwnd;
    *handle = surface_to_handle( surface );
    return VK_SUCCESS;
}

static void win32u_vkDestroySurfaceKHR( VkInstance instance, VkSurfaceKHR handle, const VkAllocationCallbacks *allocator )
{
    struct surface *surface = surface_from_handle( handle );
    WND *win;

    TRACE( "instance %p, handle 0x%s, allocator %p\n", instance, wine_dbgstr_longlong(handle), allocator );
    if (allocator) FIXME( "Support for allocation callbacks not implemented yet\n" );

    if ((win = get_win_ptr( surface->hwnd )) && win != WND_DESKTOP && win != WND_OTHER_PROCESS)
    {
        list_remove( &surface->entry );
        release_win_ptr( win );
    }

    p_vkDestroySurfaceKHR( instance, surface->host_surface, NULL /* allocator */ );
    driver_funcs->p_vulkan_surface_destroy( surface->hwnd, surface->driver_private );
    free( surface );
}

static VkResult win32u_vkQueuePresentKHR( VkQueue queue, const VkPresentInfoKHR *present_info, VkSurfaceKHR *surfaces )
{
    VkResult res;
    UINT i;

    TRACE( "queue %p, present_info %p\n", queue, present_info );

    res = p_vkQueuePresentKHR( queue, present_info );

    for (i = 0; i < present_info->swapchainCount; i++)
    {
        VkResult swapchain_res = present_info->pResults ? present_info->pResults[i] : res;
        struct surface *surface = surface_from_handle( surfaces[i] );

        driver_funcs->p_vulkan_surface_presented( surface->hwnd, surface->driver_private, swapchain_res );
    }

    return res;
}

static void *win32u_vkGetDeviceProcAddr( VkDevice device, const char *name )
{
    TRACE( "device %p, name %s\n", device, debugstr_a(name) );

    if (!strcmp( name, "vkGetDeviceProcAddr" )) return vulkan_funcs.p_vkGetDeviceProcAddr;
    if (!strcmp( name, "vkQueuePresentKHR" )) return vulkan_funcs.p_vkQueuePresentKHR;

    return p_vkGetDeviceProcAddr( device, name );
}

static void *win32u_vkGetInstanceProcAddr( VkInstance instance, const char *name )
{
    TRACE( "instance %p, name %s\n", instance, debugstr_a(name) );

    if (!instance) return p_vkGetInstanceProcAddr( instance, name );

    if (!strcmp( name, "vkCreateWin32SurfaceKHR" )) return vulkan_funcs.p_vkCreateWin32SurfaceKHR;
    if (!strcmp( name, "vkDestroySurfaceKHR" )) return vulkan_funcs.p_vkDestroySurfaceKHR;
    if (!strcmp( name, "vkGetInstanceProcAddr" )) return vulkan_funcs.p_vkGetInstanceProcAddr;
    if (!strcmp( name, "vkGetPhysicalDeviceWin32PresentationSupportKHR" )) return vulkan_funcs.p_vkGetPhysicalDeviceWin32PresentationSupportKHR;

    /* vkGetInstanceProcAddr also loads any children of instance, so device functions as well. */
    if (!strcmp( name, "vkGetDeviceProcAddr" )) return vulkan_funcs.p_vkGetDeviceProcAddr;
    if (!strcmp( name, "vkQueuePresentKHR" )) return vulkan_funcs.p_vkQueuePresentKHR;

    return p_vkGetInstanceProcAddr( instance, name );
}

static VkSurfaceKHR win32u_wine_get_host_surface( VkSurfaceKHR handle )
{
    struct surface *surface = surface_from_handle( handle );
    return surface->host_surface;
}

static void win32u_vulkan_surface_update( VkSurfaceKHR handle )
{
    struct surface *surface = surface_from_handle( handle );
    driver_funcs->p_vulkan_surface_update( surface->hwnd, surface->driver_private );
}

static struct vulkan_funcs vulkan_funcs =
{
    .p_vkCreateWin32SurfaceKHR = win32u_vkCreateWin32SurfaceKHR,
    .p_vkDestroySurfaceKHR = win32u_vkDestroySurfaceKHR,
    .p_vkQueuePresentKHR = win32u_vkQueuePresentKHR,
    .p_vkGetDeviceProcAddr = win32u_vkGetDeviceProcAddr,
    .p_vkGetInstanceProcAddr = win32u_vkGetInstanceProcAddr,
    .p_wine_get_host_surface = win32u_wine_get_host_surface,
    .p_vulkan_surface_update = win32u_vulkan_surface_update,
};

static VkResult nulldrv_vulkan_surface_create( HWND hwnd, VkInstance instance, VkSurfaceKHR *surface, void **private )
{
    FIXME( "stub!\n" );
    return VK_ERROR_INCOMPATIBLE_DRIVER;
}

static void nulldrv_vulkan_surface_destroy( HWND hwnd, void *private )
{
}

static void nulldrv_vulkan_surface_detach( HWND hwnd, void *private )
{
}

static void nulldrv_vulkan_surface_update( HWND hwnd, void *private )
{
}

static void nulldrv_vulkan_surface_presented( HWND hwnd, void *private, VkResult result )
{
}

static VkBool32 nulldrv_vkGetPhysicalDeviceWin32PresentationSupportKHR( VkPhysicalDevice device, uint32_t queue )
{
    return VK_TRUE;
}

static const char *nulldrv_get_host_surface_extension(void)
{
    return "VK_WINE_nulldrv_surface";
}

static const struct vulkan_driver_funcs nulldrv_funcs =
{
    .p_vulkan_surface_create = nulldrv_vulkan_surface_create,
    .p_vulkan_surface_destroy = nulldrv_vulkan_surface_destroy,
    .p_vulkan_surface_detach = nulldrv_vulkan_surface_detach,
    .p_vulkan_surface_update = nulldrv_vulkan_surface_update,
    .p_vulkan_surface_presented = nulldrv_vulkan_surface_presented,
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

    if (status == STATUS_NOT_IMPLEMENTED)
        driver_funcs = &nulldrv_funcs;
    else
    {
        vulkan_funcs.p_vkGetPhysicalDeviceWin32PresentationSupportKHR = driver_funcs->p_vkGetPhysicalDeviceWin32PresentationSupportKHR;
        vulkan_funcs.p_get_host_surface_extension = driver_funcs->p_get_host_surface_extension;
    }
}

static void vulkan_driver_load(void)
{
    static pthread_once_t init_once = PTHREAD_ONCE_INIT;
    pthread_once( &init_once, vulkan_driver_init );
}

static VkResult lazydrv_vulkan_surface_create( HWND hwnd, VkInstance instance, VkSurfaceKHR *surface, void **private )
{
    vulkan_driver_load();
    return driver_funcs->p_vulkan_surface_create( hwnd, instance, surface, private );
}

static void lazydrv_vulkan_surface_destroy( HWND hwnd, void *private )
{
    vulkan_driver_load();
    return driver_funcs->p_vulkan_surface_destroy( hwnd, private );
}

static void lazydrv_vulkan_surface_detach( HWND hwnd, void *private )
{
    vulkan_driver_load();
    return driver_funcs->p_vulkan_surface_detach( hwnd, private );
}

static void lazydrv_vulkan_surface_update( HWND hwnd, void *private )
{
    vulkan_driver_load();
    return driver_funcs->p_vulkan_surface_update( hwnd, private );
}

static void lazydrv_vulkan_surface_presented( HWND hwnd, void *private, VkResult result )
{
    vulkan_driver_load();
    driver_funcs->p_vulkan_surface_presented( hwnd, private, result );
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
    .p_vulkan_surface_destroy = lazydrv_vulkan_surface_destroy,
    .p_vulkan_surface_detach = lazydrv_vulkan_surface_detach,
    .p_vulkan_surface_update = lazydrv_vulkan_surface_update,
    .p_vulkan_surface_presented = lazydrv_vulkan_surface_presented,
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

    LOAD_FUNCPTR( vkDestroySurfaceKHR );
    LOAD_FUNCPTR( vkQueuePresentKHR );
    LOAD_FUNCPTR( vkGetDeviceProcAddr );
    LOAD_FUNCPTR( vkGetInstanceProcAddr );
#undef LOAD_FUNCPTR

    driver_funcs = &lazydrv_funcs;
    vulkan_funcs.p_vkGetPhysicalDeviceWin32PresentationSupportKHR = lazydrv_vkGetPhysicalDeviceWin32PresentationSupportKHR;
    vulkan_funcs.p_get_host_surface_extension = lazydrv_get_host_surface_extension;
}

void vulkan_detach_surfaces( struct list *surfaces )
{
    struct surface *surface, *next;

    LIST_FOR_EACH_ENTRY_SAFE( surface, next, surfaces, struct surface, entry )
    {
        driver_funcs->p_vulkan_surface_detach( surface->hwnd, surface->driver_private );
        list_remove( &surface->entry );
        list_init( &surface->entry );
    }
}

#else /* SONAME_LIBVULKAN */

void vulkan_detach_surfaces( struct list *surfaces )
{
}

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
