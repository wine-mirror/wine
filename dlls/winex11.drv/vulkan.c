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
static const struct client_surface_funcs x11drv_vulkan_surface_funcs;

struct x11drv_vulkan_surface
{
    struct client_surface client;
    Window window;
    RECT rect;

    BOOL offscreen;
    HDC hdc_src;
    HDC hdc_dst;
};

static struct x11drv_vulkan_surface *impl_from_client_surface( struct client_surface *client )
{
    return CONTAINING_RECORD( client, struct x11drv_vulkan_surface, client );
}

static VkResult X11DRV_vulkan_surface_create( HWND hwnd, const struct vulkan_instance *instance, VkSurfaceKHR *handle,
                                              struct client_surface **client )
{
    VkXlibSurfaceCreateInfoKHR info =
    {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .dpy = gdi_display,
    };
    struct x11drv_vulkan_surface *surface;

    TRACE( "%p %p %p %p\n", hwnd, instance, handle, client );

    if (!(surface = client_surface_create( sizeof(*surface), &x11drv_vulkan_surface_funcs, hwnd )))
    {
        ERR("Failed to allocate vulkan surface for hwnd=%p\n", hwnd);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    if (!(surface->window = create_client_window( hwnd, &default_visual, default_colormap )))
    {
        ERR("Failed to allocate client window for hwnd=%p\n", hwnd);
        client_surface_release( &surface->client );
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    NtUserGetClientRect( hwnd, &surface->rect, NtUserGetDpiForWindow( hwnd ) );

    info.window = surface->window;
    if (pvkCreateXlibSurfaceKHR( instance->host.instance, &info, NULL /* allocator */, handle ))
    {
        ERR("Failed to create Xlib surface\n");
        client_surface_release( &surface->client );
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    *client = &surface->client;

    TRACE( "Created surface 0x%s, client %p\n", wine_dbgstr_longlong( *handle ), *client );
    return VK_SUCCESS;
}

static void x11drv_vulkan_surface_destroy( struct client_surface *client )
{
    struct x11drv_vulkan_surface *surface = impl_from_client_surface( client );
    HWND hwnd = client->hwnd;

    TRACE( "%s\n", debugstr_client_surface( client ) );

    if (surface->window) destroy_client_window( hwnd, surface->window );
    if (surface->hdc_dst) NtGdiDeleteObjectApp( surface->hdc_dst );
    if (surface->hdc_src) NtGdiDeleteObjectApp( surface->hdc_src );
}

static void X11DRV_vulkan_surface_detach( struct client_surface *client )
{
    struct x11drv_vulkan_surface *surface = impl_from_client_surface( client );
    Window client_window = surface->window;
    struct x11drv_win_data *data;
    HWND hwnd = client->hwnd;

    TRACE( "%s\n", debugstr_client_surface( client ) );

    if ((data = get_win_data( hwnd )))
    {
        detach_client_window( data, client_window );
        release_win_data( data );
    }
}

static void vulkan_surface_update_size( HWND hwnd, struct x11drv_vulkan_surface *surface )
{
    XWindowChanges changes;
    RECT rect;

    NtUserGetClientRect( hwnd, &rect, NtUserGetDpiForWindow( hwnd ) );
    if (EqualRect( &surface->rect, &rect )) return;

    changes.width  = min( max( 1, rect.right ), 65535 );
    changes.height = min( max( 1, rect.bottom ), 65535 );
    XConfigureWindow( gdi_display, surface->window, CWWidth | CWHeight, &changes );
    surface->rect = rect;
}

static void vulkan_surface_update_offscreen( HWND hwnd, struct x11drv_vulkan_surface *surface )
{
    BOOL offscreen = needs_offscreen_rendering( hwnd );
    struct x11drv_win_data *data;

    if (offscreen == surface->offscreen)
    {
        if (!offscreen && (data = get_win_data( hwnd )))
        {
            attach_client_window( data, surface->window );
            release_win_data( data );
        }
        return;
    }
    surface->offscreen = offscreen;

    if (!surface->offscreen)
    {
#ifdef SONAME_LIBXCOMPOSITE
        if (usexcomposite) pXCompositeUnredirectWindow( gdi_display, surface->window, CompositeRedirectManual );
#endif
        if (surface->hdc_dst)
        {
            NtGdiDeleteObjectApp( surface->hdc_dst );
            surface->hdc_dst = NULL;
        }
        if (surface->hdc_src)
        {
            NtGdiDeleteObjectApp( surface->hdc_src );
            surface->hdc_src = NULL;
        }
    }
    else
    {
        static const WCHAR displayW[] = {'D','I','S','P','L','A','Y'};
        UNICODE_STRING device_str = RTL_CONSTANT_STRING(displayW);
        surface->hdc_dst = NtGdiOpenDCW( &device_str, NULL, NULL, 0, TRUE, NULL, NULL, NULL );
        surface->hdc_src = NtGdiOpenDCW( &device_str, NULL, NULL, 0, TRUE, NULL, NULL, NULL );
        set_dc_drawable( surface->hdc_src, surface->window, &surface->rect, IncludeInferiors );
#ifdef SONAME_LIBXCOMPOSITE
        if (usexcomposite) pXCompositeRedirectWindow( gdi_display, surface->window, CompositeRedirectManual );
#endif
    }

    if ((data = get_win_data( hwnd )))
    {
        if (surface->offscreen) detach_client_window( data, surface->window );
        else attach_client_window( data, surface->window );
        release_win_data( data );
    }
}

static void X11DRV_vulkan_surface_update( struct client_surface *client )
{
    struct x11drv_vulkan_surface *surface = impl_from_client_surface( client );
    HWND hwnd = client->hwnd;

    TRACE( "%s\n", debugstr_client_surface( client ) );

    vulkan_surface_update_size( hwnd, surface );
    vulkan_surface_update_offscreen( hwnd, surface );
}

static void X11DRV_vulkan_surface_presented( struct client_surface *client )
{
    struct x11drv_vulkan_surface *surface = impl_from_client_surface( client );
    HWND hwnd = client->hwnd, toplevel = NtUserGetAncestor( hwnd, GA_ROOT );
    struct x11drv_win_data *data;
    RECT rect_dst, rect;
    Drawable window;
    HRGN region;
    HDC hdc;

    vulkan_surface_update_size( hwnd, surface );
    vulkan_surface_update_offscreen( hwnd, surface );

    if (!surface->offscreen) return;
    if (!(hdc = NtUserGetDCEx( hwnd, 0, DCX_CACHE | DCX_USESTYLE ))) return;
    window = X11DRV_get_whole_window( toplevel );
    region = get_dc_monitor_region( hwnd, hdc );

    NtUserGetClientRect( hwnd, &rect_dst, NtUserGetWinMonitorDpi( hwnd, MDT_RAW_DPI ) );
    NtUserMapWindowPoints( hwnd, toplevel, (POINT *)&rect_dst, 2, NtUserGetWinMonitorDpi( hwnd, MDT_RAW_DPI ) );

    if ((data = get_win_data( toplevel )))
    {
        OffsetRect( &rect_dst, data->rects.client.left - data->rects.visible.left,
                    data->rects.client.top - data->rects.visible.top );
        release_win_data( data );
    }

    if (get_dc_drawable( surface->hdc_dst, &rect ) != window || !EqualRect( &rect, &rect_dst ))
        set_dc_drawable( surface->hdc_dst, window, &rect_dst, IncludeInferiors );
    if (region) NtGdiExtSelectClipRgn( surface->hdc_dst, region, RGN_COPY );

    NtGdiStretchBlt( surface->hdc_dst, 0, 0, rect_dst.right - rect_dst.left, rect_dst.bottom - rect_dst.top,
                     surface->hdc_src, 0, 0, surface->rect.right, surface->rect.bottom, SRCCOPY, 0 );

    if (region) NtGdiDeleteObjectApp( region );
    if (hdc) NtUserReleaseDC( hwnd, hdc );
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

static const struct client_surface_funcs x11drv_vulkan_surface_funcs =
{
    .destroy = x11drv_vulkan_surface_destroy,
};

static const struct vulkan_driver_funcs x11drv_vulkan_driver_funcs =
{
    .p_vulkan_surface_create = X11DRV_vulkan_surface_create,
    .p_vulkan_surface_detach = X11DRV_vulkan_surface_detach,
    .p_vulkan_surface_update = X11DRV_vulkan_surface_update,
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
