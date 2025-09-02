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
#include "xcomposite.h"

#define WINE_VULKAN_NO_X11_TYPES
#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

static const struct vulkan_driver_funcs x11drv_vulkan_driver_funcs;

static VkResult X11DRV_vulkan_surface_create( HWND hwnd, const struct vulkan_instance *instance, VkSurfaceKHR *handle,
                                              struct client_surface **client )
{
    VkXlibSurfaceCreateInfoKHR info =
    {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .dpy = gdi_display,
    };

    TRACE( "%p %p %p %p\n", hwnd, instance, handle, client );

    if (!(info.window = x11drv_client_surface_create( hwnd, &default_visual, default_colormap, client ))) return VK_ERROR_OUT_OF_HOST_MEMORY;
    if (instance->p_vkCreateXlibSurfaceKHR( instance->host.instance, &info, NULL /* allocator */, handle ))
    {
        ERR("Failed to create Xlib surface\n");
        client_surface_release( *client );
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    TRACE( "Created surface 0x%s, client %s\n", wine_dbgstr_longlong( *handle ), debugstr_client_surface( *client ) );
    return VK_SUCCESS;
}

static VkBool32 X11DRV_get_physical_device_presentation_support( struct vulkan_physical_device *physical_device, uint32_t index )
{
    struct vulkan_instance *instance = physical_device->instance;
    TRACE( "%p %u\n", physical_device, index );
    return instance->p_vkGetPhysicalDeviceXlibPresentationSupportKHR( physical_device->host.physical_device, index, gdi_display,
                                                                      default_visual.visual->visualid );
}

static const char *X11DRV_get_host_surface_extension(void)
{
    return "VK_KHR_xlib_surface";
}

static const struct vulkan_driver_funcs x11drv_vulkan_driver_funcs =
{
    .p_vulkan_surface_create = X11DRV_vulkan_surface_create,
    .p_get_physical_device_presentation_support = X11DRV_get_physical_device_presentation_support,
    .p_get_host_surface_extension = X11DRV_get_host_surface_extension,
};

UINT X11DRV_VulkanInit( UINT version, void *vulkan_handle, const struct vulkan_driver_funcs **driver_funcs )
{
    if (version != WINE_VULKAN_DRIVER_VERSION)
    {
        ERR( "version mismatch, win32u wants %u but driver has %u\n", version, WINE_VULKAN_DRIVER_VERSION );
        return STATUS_INVALID_PARAMETER;
    }

    *driver_funcs = &x11drv_vulkan_driver_funcs;
    return STATUS_SUCCESS;
}
