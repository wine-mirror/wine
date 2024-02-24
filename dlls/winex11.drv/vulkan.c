/* X11DRV Vulkan implementation
 *
 * Copyright 2017 Roderick Colenbrander
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
#include "windef.h"
#include "winbase.h"

#include "wine/debug.h"
#include "x11drv.h"

#define VK_NO_PROTOTYPES
#define WINE_VK_HOST

#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

#ifdef SONAME_LIBVULKAN

#define VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR 1000004000

typedef struct VkXlibSurfaceCreateInfoKHR
{
    VkStructureType sType;
    const void *pNext;
    VkXlibSurfaceCreateFlagsKHR flags;
    Display *dpy;
    Window window;
} VkXlibSurfaceCreateInfoKHR;

static VkResult (*pvkCreateXlibSurfaceKHR)(VkInstance, const VkXlibSurfaceCreateInfoKHR *, const VkAllocationCallbacks *, VkSurfaceKHR *);
static VkBool32 (*pvkGetPhysicalDeviceXlibPresentationSupportKHR)(VkPhysicalDevice, uint32_t, Display *, VisualID);

static const struct vulkan_driver_funcs x11drv_vulkan_driver_funcs;

static VkResult X11DRV_vulkan_surface_create( HWND hwnd, VkInstance instance, VkSurfaceKHR *surface, void **private )
{
    VkXlibSurfaceCreateInfoKHR info =
    {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .dpy = gdi_display,
    };

    TRACE( "%p %p %p %p\n", hwnd, instance, surface, private );

    /* TODO: support child window rendering. */
    if (NtUserGetAncestor( hwnd, GA_PARENT ) != NtUserGetDesktopWindow())
    {
        FIXME("Application requires child window rendering, which is not implemented yet!\n");
        return VK_ERROR_INCOMPATIBLE_DRIVER;
    }

    if (!(info.window = create_client_window( hwnd, &default_visual, default_colormap )))
    {
        ERR("Failed to allocate client window for hwnd=%p\n", hwnd);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    if (pvkCreateXlibSurfaceKHR( instance, &info, NULL /* allocator */, surface ))
    {
        ERR("Failed to create Xlib surface\n");
        destroy_client_window( hwnd, info.window );
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    *private = (void *)info.window;

    TRACE("Created surface 0x%s, private %p\n", wine_dbgstr_longlong(*surface), *private);
    return VK_SUCCESS;
}

static void X11DRV_vulkan_surface_destroy( HWND hwnd, void *private )
{
    Window client_window = (Window)private;

    TRACE( "%p %p\n", hwnd, private );

    destroy_client_window( hwnd, client_window );
}

static void X11DRV_vulkan_surface_detach( HWND hwnd, void *private )
{
    Window client_window = (Window)private;
    struct x11drv_win_data *data;

    TRACE( "%p %p\n", hwnd, private );

    if ((data = get_win_data( hwnd )))
    {
        detach_client_window( data, client_window );
        release_win_data( data );
    }
}

static void X11DRV_vulkan_surface_presented(HWND hwnd, VkResult result)
{
}

static VkBool32 X11DRV_vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice phys_dev,
        uint32_t index)
{
    TRACE("%p %u\n", phys_dev, index);

    return pvkGetPhysicalDeviceXlibPresentationSupportKHR(phys_dev, index, gdi_display,
            default_visual.visual->visualid);
}

static const char *X11DRV_get_host_surface_extension(void)
{
    return "VK_KHR_xlib_surface";
}

static const struct vulkan_driver_funcs x11drv_vulkan_driver_funcs =
{
    .p_vulkan_surface_create = X11DRV_vulkan_surface_create,
    .p_vulkan_surface_destroy = X11DRV_vulkan_surface_destroy,
    .p_vulkan_surface_detach = X11DRV_vulkan_surface_detach,
    .p_vulkan_surface_presented = X11DRV_vulkan_surface_presented,

    .p_vkGetPhysicalDeviceWin32PresentationSupportKHR = X11DRV_vkGetPhysicalDeviceWin32PresentationSupportKHR,
    .p_get_host_surface_extension = X11DRV_get_host_surface_extension,
};

UINT X11DRV_VulkanInit( UINT version, void *vulkan_handle, const struct vulkan_driver_funcs **driver_funcs )
{
    if (version != WINE_VULKAN_DRIVER_VERSION)
    {
        ERR( "version mismatch, win32u wants %u but driver has %u\n", version, WINE_VULKAN_DRIVER_VERSION );
        return STATUS_INVALID_PARAMETER;
    }

#define LOAD_FUNCPTR( f ) if (!(p##f = dlsym( vulkan_handle, #f ))) return STATUS_PROCEDURE_NOT_FOUND;
    LOAD_FUNCPTR( vkCreateXlibSurfaceKHR );
    LOAD_FUNCPTR( vkGetPhysicalDeviceXlibPresentationSupportKHR );
#undef LOAD_FUNCPTR

    *driver_funcs = &x11drv_vulkan_driver_funcs;
    return STATUS_SUCCESS;
}

#else /* No vulkan */

UINT X11DRV_VulkanInit( UINT version, void *vulkan_handle, const struct vulkan_driver_funcs **driver_funcs )
{
    ERR( "Wine was built without Vulkan support.\n" );
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* SONAME_LIBVULKAN */
