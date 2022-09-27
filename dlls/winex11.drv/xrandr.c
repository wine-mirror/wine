/*
 * Wine X11drv Xrandr interface
 *
 * Copyright 2003 Alexander James Pasadyn
 * Copyright 2012 Henri Verbeet for CodeWeavers
 * Copyright 2019 Zhiyi Zhang for CodeWeavers
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

#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xrandr);
#ifdef HAVE_XRRGETPROVIDERRESOURCES
WINE_DECLARE_DEBUG_CHANNEL(winediag);
#endif

#ifdef SONAME_LIBXRANDR

#include <assert.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <dlfcn.h>
#include "x11drv.h"

#define VK_NO_PROTOTYPES
#define WINE_VK_HOST

#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"

static void *xrandr_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f;
MAKE_FUNCPTR(XRRConfigCurrentConfiguration)
MAKE_FUNCPTR(XRRConfigCurrentRate)
MAKE_FUNCPTR(XRRFreeScreenConfigInfo)
MAKE_FUNCPTR(XRRGetScreenInfo)
MAKE_FUNCPTR(XRRQueryExtension)
MAKE_FUNCPTR(XRRQueryVersion)
MAKE_FUNCPTR(XRRRates)
MAKE_FUNCPTR(XRRSetScreenConfig)
MAKE_FUNCPTR(XRRSetScreenConfigAndRate)
MAKE_FUNCPTR(XRRSizes)

#ifdef HAVE_XRRGETPROVIDERRESOURCES
MAKE_FUNCPTR(XRRFreeCrtcInfo)
MAKE_FUNCPTR(XRRFreeOutputInfo)
MAKE_FUNCPTR(XRRFreeScreenResources)
MAKE_FUNCPTR(XRRGetCrtcInfo)
MAKE_FUNCPTR(XRRGetOutputInfo)
MAKE_FUNCPTR(XRRGetOutputProperty)
MAKE_FUNCPTR(XRRGetScreenResources)
MAKE_FUNCPTR(XRRGetScreenResourcesCurrent)
MAKE_FUNCPTR(XRRGetScreenSizeRange)
MAKE_FUNCPTR(XRRSetCrtcConfig)
MAKE_FUNCPTR(XRRSetScreenSize)
MAKE_FUNCPTR(XRRSelectInput)
MAKE_FUNCPTR(XRRGetOutputPrimary)
MAKE_FUNCPTR(XRRGetProviderResources)
MAKE_FUNCPTR(XRRFreeProviderResources)
MAKE_FUNCPTR(XRRGetProviderInfo)
MAKE_FUNCPTR(XRRFreeProviderInfo)
#endif

#undef MAKE_FUNCPTR

static int load_xrandr(void)
{
    int r = 0;

    if (dlopen(SONAME_LIBXRENDER, RTLD_NOW|RTLD_GLOBAL) &&
        (xrandr_handle = dlopen(SONAME_LIBXRANDR, RTLD_NOW)))
    {

#define LOAD_FUNCPTR(f) \
        if((p##f = dlsym(xrandr_handle, #f)) == NULL) goto sym_not_found

        LOAD_FUNCPTR(XRRConfigCurrentConfiguration);
        LOAD_FUNCPTR(XRRConfigCurrentRate);
        LOAD_FUNCPTR(XRRFreeScreenConfigInfo);
        LOAD_FUNCPTR(XRRGetScreenInfo);
        LOAD_FUNCPTR(XRRQueryExtension);
        LOAD_FUNCPTR(XRRQueryVersion);
        LOAD_FUNCPTR(XRRRates);
        LOAD_FUNCPTR(XRRSetScreenConfig);
        LOAD_FUNCPTR(XRRSetScreenConfigAndRate);
        LOAD_FUNCPTR(XRRSizes);
        r = 1;

#ifdef HAVE_XRRGETPROVIDERRESOURCES
        LOAD_FUNCPTR(XRRFreeCrtcInfo);
        LOAD_FUNCPTR(XRRFreeOutputInfo);
        LOAD_FUNCPTR(XRRFreeScreenResources);
        LOAD_FUNCPTR(XRRGetCrtcInfo);
        LOAD_FUNCPTR(XRRGetOutputInfo);
        LOAD_FUNCPTR(XRRGetOutputProperty);
        LOAD_FUNCPTR(XRRGetScreenResources);
        LOAD_FUNCPTR(XRRGetScreenResourcesCurrent);
        LOAD_FUNCPTR(XRRGetScreenSizeRange);
        LOAD_FUNCPTR(XRRSetCrtcConfig);
        LOAD_FUNCPTR(XRRSetScreenSize);
        LOAD_FUNCPTR(XRRSelectInput);
        LOAD_FUNCPTR(XRRGetOutputPrimary);
        LOAD_FUNCPTR(XRRGetProviderResources);
        LOAD_FUNCPTR(XRRFreeProviderResources);
        LOAD_FUNCPTR(XRRGetProviderInfo);
        LOAD_FUNCPTR(XRRFreeProviderInfo);
        r = 4;
#endif

#undef LOAD_FUNCPTR

sym_not_found:
        if (!r)  TRACE("Unable to load function ptrs from XRandR library\n");
    }
    return r;
}

static int XRandRErrorHandler(Display *dpy, XErrorEvent *event, void *arg)
{
    return 1;
}

/* XRandR 1.0 display settings handler */
static BOOL xrandr10_get_id( const WCHAR *device_name, ULONG_PTR *id )
{
    WCHAR primary_adapter[CCHDEVICENAME];

    if (!get_primary_adapter( primary_adapter ))
        return FALSE;

    /* RandR 1.0 only supports changing the primary adapter settings.
     * For non-primary adapters, an id is still provided but getting
     * and changing non-primary adapters' settings will be ignored. */
    *id = !wcsicmp( device_name, primary_adapter ) ? 1 : 0;
    return TRUE;
}

static void add_xrandr10_mode( DEVMODEW *mode, DWORD depth, DWORD width, DWORD height,
                               DWORD frequency, SizeID size_id )
{
    mode->dmSize = sizeof(*mode);
    mode->dmDriverExtra = sizeof(SizeID);
    mode->dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH |
                     DM_PELSHEIGHT | DM_DISPLAYFLAGS;
    if (frequency)
    {
        mode->dmFields |= DM_DISPLAYFREQUENCY;
        mode->dmDisplayFrequency = frequency;
    }
    mode->u1.s2.dmDisplayOrientation = DMDO_DEFAULT;
    mode->dmBitsPerPel = depth;
    mode->dmPelsWidth = width;
    mode->dmPelsHeight = height;
    mode->u2.dmDisplayFlags = 0;
    memcpy( (BYTE *)mode + sizeof(*mode), &size_id, sizeof(size_id) );
}

static BOOL xrandr10_get_modes( ULONG_PTR id, DWORD flags, DEVMODEW **new_modes, UINT *new_mode_count )
{
    INT size_idx, depth_idx, rate_idx, mode_idx = 0;
    INT size_count, rate_count, mode_count = 0;
    DEVMODEW *modes, *mode;
    XRRScreenSize *sizes;
    short *rates;

    sizes = pXRRSizes( gdi_display, DefaultScreen( gdi_display ), &size_count );
    if (size_count <= 0)
        return FALSE;

    for (size_idx = 0; size_idx < size_count; ++size_idx)
    {
        rates = pXRRRates( gdi_display, DefaultScreen( gdi_display ), size_idx, &rate_count );
        if (rate_count)
            mode_count += rate_count;
        else
            ++mode_count;
    }

    /* Allocate space for reported modes in three depths, and put an SizeID at the end of DEVMODEW as
     * driver private data */
    modes = calloc( mode_count * DEPTH_COUNT, sizeof(*modes) + sizeof(SizeID) );
    if (!modes)
    {
        RtlSetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    for (size_idx = 0; size_idx < size_count; ++size_idx)
    {
        for (depth_idx = 0; depth_idx < DEPTH_COUNT; ++depth_idx)
        {
            rates = pXRRRates( gdi_display, DefaultScreen( gdi_display ), size_idx, &rate_count );
            if (!rate_count)
            {
                mode = (DEVMODEW *)((BYTE *)modes + (sizeof(*mode) + sizeof(SizeID)) * mode_idx++);
                add_xrandr10_mode( mode, depths[depth_idx], sizes[size_idx].width,
                                   sizes[size_idx].height, 0, size_idx );
                continue;
            }

            for (rate_idx = 0; rate_idx < rate_count; ++rate_idx)
            {
                mode = (DEVMODEW *)((BYTE *)modes + (sizeof(*mode) + sizeof(SizeID)) * mode_idx++);
                add_xrandr10_mode( mode, depths[depth_idx], sizes[size_idx].width,
                                   sizes[size_idx].height, rates[rate_idx], size_idx );
            }
        }
    }

    *new_modes = modes;
    *new_mode_count = mode_idx;
    return TRUE;
}

static void xrandr10_free_modes( DEVMODEW *modes )
{
    free( modes );
}

static BOOL xrandr10_get_current_mode( ULONG_PTR id, DEVMODEW *mode )
{
    XRRScreenConfiguration *screen_config;
    XRRScreenSize *sizes;
    Rotation rotation;
    SizeID size_id;
    INT size_count;
    short rate;

    mode->dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                     DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY | DM_POSITION;
    mode->u1.s2.dmDisplayOrientation = DMDO_DEFAULT;
    mode->u2.dmDisplayFlags = 0;
    mode->u1.s2.dmPosition.x = 0;
    mode->u1.s2.dmPosition.y = 0;

    if (id != 1)
    {
        FIXME("Non-primary adapters are unsupported.\n");
        mode->dmBitsPerPel = 0;
        mode->dmPelsWidth = 0;
        mode->dmPelsHeight = 0;
        mode->dmDisplayFrequency = 0;
        return TRUE;
    }

    sizes = pXRRSizes( gdi_display, DefaultScreen( gdi_display ), &size_count );
    if (size_count <= 0)
        return FALSE;

    screen_config = pXRRGetScreenInfo( gdi_display, DefaultRootWindow( gdi_display ) );
    size_id = pXRRConfigCurrentConfiguration( screen_config, &rotation );
    rate = pXRRConfigCurrentRate( screen_config );
    pXRRFreeScreenConfigInfo( screen_config );

    mode->dmBitsPerPel = screen_bpp;
    mode->dmPelsWidth = sizes[size_id].width;
    mode->dmPelsHeight = sizes[size_id].height;
    mode->dmDisplayFrequency = rate;
    return TRUE;
}

static LONG xrandr10_set_current_mode( ULONG_PTR id, const DEVMODEW *mode )
{
    XRRScreenConfiguration *screen_config;
    Rotation rotation;
    SizeID size_id;
    Window root;
    Status stat;

    if (id != 1)
    {
        FIXME("Non-primary adapters are unsupported.\n");
        return DISP_CHANGE_SUCCESSFUL;
    }

    if (is_detached_mode(mode))
    {
        FIXME("Detaching adapters is unsupported.\n");
        return DISP_CHANGE_SUCCESSFUL;
    }

    if (mode->dmFields & DM_BITSPERPEL && mode->dmBitsPerPel != screen_bpp)
        WARN("Cannot change screen bit depth from %dbits to %dbits!\n", screen_bpp, mode->dmBitsPerPel);

    root = DefaultRootWindow( gdi_display );
    screen_config = pXRRGetScreenInfo( gdi_display, root );
    pXRRConfigCurrentConfiguration( screen_config, &rotation );

    assert( mode->dmDriverExtra == sizeof(SizeID) );
    memcpy( &size_id, (BYTE *)mode + sizeof(*mode), sizeof(size_id) );

    if (mode->dmFields & DM_DISPLAYFREQUENCY && mode->dmDisplayFrequency)
        stat = pXRRSetScreenConfigAndRate( gdi_display, screen_config, root, size_id, rotation,
                                           mode->dmDisplayFrequency, CurrentTime );
    else
        stat = pXRRSetScreenConfig( gdi_display, screen_config, root, size_id, rotation, CurrentTime );
    pXRRFreeScreenConfigInfo( screen_config );

    if (stat != RRSetConfigSuccess)
        return DISP_CHANGE_FAILED;

    XFlush( gdi_display );
    return DISP_CHANGE_SUCCESSFUL;
}

#ifdef HAVE_XRRGETPROVIDERRESOURCES

static struct current_mode
{
    ULONG_PTR id;
    BOOL loaded;
    DEVMODEW mode;
} *current_modes;
static int current_mode_count;

static pthread_mutex_t xrandr_mutex = PTHREAD_MUTEX_INITIALIZER;

static void xrandr14_invalidate_current_mode_cache(void)
{
    pthread_mutex_lock( &xrandr_mutex );
    free( current_modes);
    current_modes = NULL;
    current_mode_count = 0;
    pthread_mutex_unlock( &xrandr_mutex );
}

static XRRScreenResources *xrandr_get_screen_resources(void)
{
    XRRScreenResources *resources = pXRRGetScreenResourcesCurrent( gdi_display, root_window );
    if (resources && !resources->ncrtc)
    {
        pXRRFreeScreenResources( resources );
        resources = pXRRGetScreenResources( gdi_display, root_window );
    }

    if (!resources)
        ERR("Failed to get screen resources.\n");
    return resources;
}

/* Some (304.64, possibly earlier) versions of the NVIDIA driver only
 * report a DFP's native mode through RandR 1.2 / 1.3. Standard DMT modes
 * are only listed through RandR 1.0 / 1.1. This is completely useless,
 * but NVIDIA considers this a feature, so it's unlikely to change. The
 * best we can do is to fall back to RandR 1.0 and encourage users to
 * consider more cooperative driver vendors when we detect such a
 * configuration. */
static BOOL is_broken_driver(void)
{
    XRRScreenResources *screen_resources;
    XRROutputInfo *output_info;
    XRRModeInfo *first_mode;
    INT major, event, error;
    INT output_idx, i, j;
    BOOL only_one_mode;

    screen_resources = xrandr_get_screen_resources();
    if (!screen_resources)
        return TRUE;

    /* Check if any output only has one native mode */
    for (output_idx = 0; output_idx < screen_resources->noutput; ++output_idx)
    {
        output_info = pXRRGetOutputInfo( gdi_display, screen_resources,
                                         screen_resources->outputs[output_idx] );
        if (!output_info)
            continue;

        if (output_info->connection != RR_Connected)
        {
            pXRRFreeOutputInfo( output_info );
            continue;
        }

        first_mode = NULL;
        only_one_mode = TRUE;
        for (i = 0; i < output_info->nmode; ++i)
        {
            for (j = 0; j < screen_resources->nmode; ++j)
            {
                if (output_info->modes[i] != screen_resources->modes[j].id)
                    continue;

                if (!first_mode)
                {
                    first_mode = &screen_resources->modes[j];
                    break;
                }

                if (first_mode->width != screen_resources->modes[j].width ||
                    first_mode->height != screen_resources->modes[j].height)
                    only_one_mode = FALSE;

                break;
            }

            if (!only_one_mode)
                break;
        }
        pXRRFreeOutputInfo( output_info );

        if (!only_one_mode)
            continue;

        /* Check if it is NVIDIA proprietary driver */
        if (XQueryExtension( gdi_display, "NV-CONTROL", &major, &event, &error ))
        {
            ERR_(winediag)("Broken NVIDIA RandR detected, falling back to RandR 1.0. "
                           "Please consider using the Nouveau driver instead.\n");
            pXRRFreeScreenResources( screen_resources );
            return TRUE;
        }
    }
    pXRRFreeScreenResources( screen_resources );
    return FALSE;
}

static void get_screen_size( XRRScreenResources *resources, unsigned int *width, unsigned int *height )
{
    int min_width = 0, min_height = 0, max_width, max_height;
    XRRCrtcInfo *crtc_info;
    int i;

    pXRRGetScreenSizeRange( gdi_display, root_window, &min_width, &min_height, &max_width, &max_height );
    *width = min_width;
    *height = min_height;

    for (i = 0; i < resources->ncrtc; ++i)
    {
        if (!(crtc_info = pXRRGetCrtcInfo( gdi_display, resources, resources->crtcs[i] )))
            continue;

        if (crtc_info->mode != None)
        {
            *width = max(*width, crtc_info->x + crtc_info->width);
            *height = max(*height, crtc_info->y + crtc_info->height);
        }

        pXRRFreeCrtcInfo( crtc_info );
    }
}

static unsigned int get_edid( RROutput output, unsigned char **prop )
{
    int result, actual_format;
    unsigned long bytes_after, len;
    Atom actual_type;

    result = pXRRGetOutputProperty( gdi_display, output, x11drv_atom(EDID), 0, 128, FALSE, FALSE,
                                    AnyPropertyType, &actual_type, &actual_format, &len,
                                    &bytes_after, prop );

    if (result != Success)
    {
        WARN("Could not retrieve EDID property for output %#lx.\n", output);
        *prop = NULL;
        return 0;
    }
    return len;
}

static void set_screen_size( int width, int height )
{
    int screen = default_visual.screen;
    int mm_width, mm_height;

    mm_width = width * DisplayWidthMM( gdi_display, screen ) / DisplayWidth( gdi_display, screen );
    mm_height = height * DisplayHeightMM( gdi_display, screen ) / DisplayHeight( gdi_display, screen );
    pXRRSetScreenSize( gdi_display, root_window, width, height, mm_width, mm_height );
}

static unsigned int get_frequency( const XRRModeInfo *mode )
{
    unsigned int dots = mode->hTotal * mode->vTotal;

    if (!dots)
        return 0;

    if (mode->modeFlags & RR_DoubleScan)
        dots *= 2;
    if (mode->modeFlags & RR_Interlace)
        dots /= 2;

    return (mode->dotClock + dots / 2) / dots;
}

static DWORD get_orientation( Rotation rotation )
{
    if (rotation & RR_Rotate_270) return DMDO_270;
    if (rotation & RR_Rotate_180) return DMDO_180;
    if (rotation & RR_Rotate_90) return DMDO_90;
    return DMDO_DEFAULT;
}

static DWORD get_orientation_count( Rotation rotations )
{
    DWORD count = 0;

    if (rotations & RR_Rotate_0) ++count;
    if (rotations & RR_Rotate_90) ++count;
    if (rotations & RR_Rotate_180) ++count;
    if (rotations & RR_Rotate_270) ++count;
    return count;
}

static Rotation get_rotation( DWORD orientation )
{
    return (Rotation)(1 << orientation);
}

static RRCrtc get_output_free_crtc( XRRScreenResources *resources, XRROutputInfo *output_info )
{
    XRRCrtcInfo *crtc_info;
    INT crtc_idx;
    RRCrtc crtc;

    for (crtc_idx = 0; crtc_idx < output_info->ncrtc; ++crtc_idx)
    {
        crtc_info = pXRRGetCrtcInfo( gdi_display, resources, output_info->crtcs[crtc_idx] );
        if (!crtc_info)
            continue;

        if (!crtc_info->noutput)
        {
            crtc = output_info->crtcs[crtc_idx];
            pXRRFreeCrtcInfo( crtc_info );
            return crtc;
        }

        pXRRFreeCrtcInfo( crtc_info );
    }

    return 0;
}

static RECT get_primary_rect( XRRScreenResources *resources )
{
    XRROutputInfo *output_info = NULL;
    XRRCrtcInfo *crtc_info = NULL;
    RROutput primary_output;
    RECT primary_rect = {0};
    RECT first_rect = {0};
    INT i;

    primary_output = pXRRGetOutputPrimary( gdi_display, root_window );
    if (!primary_output)
        goto fallback;

    output_info = pXRRGetOutputInfo( gdi_display, resources, primary_output );
    if (!output_info || output_info->connection != RR_Connected || !output_info->crtc)
        goto fallback;

    crtc_info = pXRRGetCrtcInfo( gdi_display, resources, output_info->crtc );
    if (!crtc_info || !crtc_info->mode)
        goto fallback;

    SetRect( &primary_rect, crtc_info->x, crtc_info->y, crtc_info->x + crtc_info->width, crtc_info->y + crtc_info->height );
    pXRRFreeCrtcInfo( crtc_info );
    pXRRFreeOutputInfo( output_info );
    return primary_rect;

/* Fallback when XRandR primary output is a disconnected output.
 * Try to find a crtc with (x, y) being (0, 0). If it's found then get the primary rect from that crtc,
 * otherwise use the first active crtc to get the primary rect */
fallback:
    if (crtc_info)
        pXRRFreeCrtcInfo( crtc_info );
    if (output_info)
        pXRRFreeOutputInfo( output_info );

    WARN("Primary is set to a disconnected XRandR output.\n");
    for (i = 0; i < resources->ncrtc; ++i)
    {
        crtc_info = pXRRGetCrtcInfo( gdi_display, resources, resources->crtcs[i] );
        if (!crtc_info)
            continue;

        if (!crtc_info->mode)
        {
            pXRRFreeCrtcInfo( crtc_info );
            continue;
        }

        if (!crtc_info->x && !crtc_info->y)
        {
            SetRect( &primary_rect, 0, 0, crtc_info->width, crtc_info->height );
            pXRRFreeCrtcInfo( crtc_info );
            break;
        }

        if (IsRectEmpty( &first_rect ))
            SetRect( &first_rect, crtc_info->x, crtc_info->y,
                     crtc_info->x + crtc_info->width, crtc_info->y + crtc_info->height );

        pXRRFreeCrtcInfo( crtc_info );
    }

    return IsRectEmpty( &primary_rect ) ? first_rect : primary_rect;
}

static BOOL is_crtc_primary( RECT primary, const XRRCrtcInfo *crtc )
{
    return crtc &&
           crtc->mode &&
           crtc->x == primary.left &&
           crtc->y == primary.top &&
           crtc->x + crtc->width == primary.right &&
           crtc->y + crtc->height == primary.bottom;
}

VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDisplayKHR)

static BOOL get_gpu_properties_from_vulkan( struct gdi_gpu *gpu, const XRRProviderInfo *provider_info,
                                            struct gdi_gpu *prev_gpus, int prev_gpu_count )
{
    static const char *extensions[] =
    {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        "VK_EXT_acquire_xlib_display",
        "VK_EXT_direct_mode_display",
        "VK_KHR_display",
        VK_KHR_SURFACE_EXTENSION_NAME,
    };
    const struct vulkan_funcs *vulkan_funcs = get_vulkan_driver( WINE_VULKAN_DRIVER_VERSION );
    VkResult (*pvkGetRandROutputDisplayEXT)( VkPhysicalDevice, Display *, RROutput, VkDisplayKHR * );
    PFN_vkGetPhysicalDeviceProperties2KHR pvkGetPhysicalDeviceProperties2KHR;
    PFN_vkEnumeratePhysicalDevices pvkEnumeratePhysicalDevices;
    uint32_t device_count, device_idx, output_idx, i;
    VkPhysicalDevice *vk_physical_devices = NULL;
    VkPhysicalDeviceProperties2 properties2;
    VkInstanceCreateInfo create_info;
    VkPhysicalDeviceIDProperties id;
    VkInstance vk_instance = NULL;
    VkDisplayKHR vk_display;
    DWORD len;
    BOOL ret = FALSE;
    VkResult vr;

    if (!vulkan_funcs)
        goto done;

    memset( &create_info, 0, sizeof(create_info) );
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.enabledExtensionCount = ARRAY_SIZE(extensions);
    create_info.ppEnabledExtensionNames = extensions;

    vr = vulkan_funcs->p_vkCreateInstance( &create_info, NULL, &vk_instance );
    if (vr != VK_SUCCESS)
    {
        WARN("Failed to create a Vulkan instance, vr %d.\n", vr);
        goto done;
    }

#define LOAD_VK_FUNC(f)                                                             \
    if (!(p##f = (void *)vulkan_funcs->p_vkGetInstanceProcAddr( vk_instance, #f ))) \
    {                                                                               \
        WARN("Failed to load " #f ".\n");                                           \
        goto done;                                                                  \
    }

    LOAD_VK_FUNC(vkEnumeratePhysicalDevices)
    LOAD_VK_FUNC(vkGetPhysicalDeviceProperties2KHR)
    LOAD_VK_FUNC(vkGetRandROutputDisplayEXT)
#undef LOAD_VK_FUNC

    vr = pvkEnumeratePhysicalDevices( vk_instance, &device_count, NULL );
    if (vr != VK_SUCCESS || !device_count)
    {
        WARN("No Vulkan device found, vr %d, device_count %d.\n", vr, device_count);
        goto done;
    }

    if (!(vk_physical_devices = calloc( device_count, sizeof(*vk_physical_devices) )))
        goto done;

    vr = pvkEnumeratePhysicalDevices( vk_instance, &device_count, vk_physical_devices );
    if (vr != VK_SUCCESS)
    {
        WARN("vkEnumeratePhysicalDevices failed, vr %d.\n", vr);
        goto done;
    }

    TRACE("provider name %s.\n", debugstr_a(provider_info->name));

    for (device_idx = 0; device_idx < device_count; ++device_idx)
    {
        for (output_idx = 0; output_idx < provider_info->noutputs; ++output_idx)
        {
            X11DRV_expect_error( gdi_display, XRandRErrorHandler, NULL );
            vr = pvkGetRandROutputDisplayEXT( vk_physical_devices[device_idx], gdi_display,
                                              provider_info->outputs[output_idx], &vk_display );
            XSync( gdi_display, FALSE );
            if (X11DRV_check_error() || vr != VK_SUCCESS || vk_display == VK_NULL_HANDLE)
                continue;

            memset( &id, 0, sizeof(id) );
            id.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
            properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            properties2.pNext = &id;

            pvkGetPhysicalDeviceProperties2KHR( vk_physical_devices[device_idx], &properties2 );
            for (i = 0; i < prev_gpu_count; ++i)
            {
                if (!memcmp( &prev_gpus[i].vulkan_uuid, &id.deviceUUID, sizeof(id.deviceUUID) ))
                {
                    WARN( "device UUID %#x:%#x already assigned to GPU %u.\n", *((uint32_t *)id.deviceUUID + 1),
                          *(uint32_t *)id.deviceUUID, i );
                    break;
                }
            }
            if (i < prev_gpu_count) continue;

            memcpy( &gpu->vulkan_uuid, id.deviceUUID, sizeof(id.deviceUUID) );

            /* Ignore Khronos vendor IDs */
            if (properties2.properties.vendorID < 0x10000)
            {
                gpu->vendor_id = properties2.properties.vendorID;
                gpu->device_id = properties2.properties.deviceID;
            }
            RtlUTF8ToUnicodeN( gpu->name, sizeof(gpu->name), &len, properties2.properties.deviceName,
                               strlen( properties2.properties.deviceName ) + 1 );
            ret = TRUE;
            goto done;
        }
    }

done:
    free( vk_physical_devices );
    if (vk_instance)
        vulkan_funcs->p_vkDestroyInstance( vk_instance, NULL );
    return ret;
}

/* Get a list of GPUs reported by XRandR 1.4. Set get_properties to FALSE if GPU properties are
 * not needed to avoid unnecessary querying */
static BOOL xrandr14_get_gpus2( struct gdi_gpu **new_gpus, int *count, BOOL get_properties )
{
    static const WCHAR wine_adapterW[] = {'W','i','n','e',' ','A','d','a','p','t','e','r',0};
    struct gdi_gpu *gpus = NULL;
    XRRScreenResources *screen_resources = NULL;
    XRRProviderResources *provider_resources = NULL;
    XRRProviderInfo *provider_info = NULL;
    XRRCrtcInfo *crtc_info = NULL;
    INT primary_provider = -1;
    RECT primary_rect;
    BOOL ret = FALSE;
    DWORD len;
    INT i, j;

    screen_resources = xrandr_get_screen_resources();
    if (!screen_resources)
        goto done;

    provider_resources = pXRRGetProviderResources( gdi_display, root_window );
    if (!provider_resources)
        goto done;

    gpus = calloc( provider_resources->nproviders ? provider_resources->nproviders : 1, sizeof(*gpus) );
    if (!gpus)
        goto done;

    /* Some XRandR implementations don't support providers.
     * In this case, report a fake one to try searching adapters in screen resources */
    if (!provider_resources->nproviders)
    {
        WARN("XRandR implementation doesn't report any providers, faking one.\n");
        lstrcpyW( gpus[0].name, wine_adapterW );
        *new_gpus = gpus;
        *count = 1;
        ret = TRUE;
        goto done;
    }

    primary_rect = get_primary_rect( screen_resources );
    for (i = 0; i < provider_resources->nproviders; ++i)
    {
        provider_info = pXRRGetProviderInfo( gdi_display, screen_resources, provider_resources->providers[i] );
        if (!provider_info)
            goto done;

        /* Find primary provider */
        for (j = 0; primary_provider == -1 && j < provider_info->ncrtcs; ++j)
        {
            crtc_info = pXRRGetCrtcInfo( gdi_display, screen_resources, provider_info->crtcs[j] );
            if (!crtc_info)
                continue;

            if (is_crtc_primary( primary_rect, crtc_info ))
            {
                primary_provider = i;
                pXRRFreeCrtcInfo( crtc_info );
                break;
            }

            pXRRFreeCrtcInfo( crtc_info );
        }

        gpus[i].id = provider_resources->providers[i];
        if (get_properties)
        {
            if (!get_gpu_properties_from_vulkan( &gpus[i], provider_info, gpus, i ))
                RtlUTF8ToUnicodeN( gpus[i].name, sizeof(gpus[i].name), &len, provider_info->name,
                                   strlen( provider_info->name ) + 1 );
            /* FIXME: Add an alternate method of getting PCI IDs, for systems that don't support Vulkan */
        }
        pXRRFreeProviderInfo( provider_info );
    }

    /* Make primary GPU the first */
    if (primary_provider > 0)
    {
        struct gdi_gpu tmp = gpus[0];
        gpus[0] = gpus[primary_provider];
        gpus[primary_provider] = tmp;
    }

    *new_gpus = gpus;
    *count = provider_resources->nproviders;
    ret = TRUE;
done:
    if (provider_resources)
        pXRRFreeProviderResources( provider_resources );
    if (screen_resources)
        pXRRFreeScreenResources( screen_resources );
    if (!ret)
    {
        free( gpus );
        ERR("Failed to get gpus\n");
    }
    return ret;
}

static BOOL xrandr14_get_gpus( struct gdi_gpu **new_gpus, int *count )
{
    return xrandr14_get_gpus2( new_gpus, count, TRUE );
}

static void xrandr14_free_gpus( struct gdi_gpu *gpus )
{
    free( gpus );
}

static BOOL xrandr14_get_adapters( ULONG_PTR gpu_id, struct gdi_adapter **new_adapters, int *count )
{
    struct gdi_adapter *adapters = NULL;
    XRRScreenResources *screen_resources = NULL;
    XRRProviderInfo *provider_info = NULL;
    XRRCrtcInfo *enum_crtc_info, *crtc_info = NULL;
    XRROutputInfo *enum_output_info, *output_info = NULL;
    RROutput *outputs;
    INT crtc_count, output_count;
    INT primary_adapter = 0;
    INT adapter_count = 0;
    BOOL mirrored, detached;
    RECT primary_rect;
    BOOL ret = FALSE;
    INT i, j;

    screen_resources = xrandr_get_screen_resources();
    if (!screen_resources)
        goto done;

    if (gpu_id)
    {
        provider_info = pXRRGetProviderInfo( gdi_display, screen_resources, gpu_id );
        if (!provider_info)
            goto done;

        crtc_count = provider_info->ncrtcs;
        output_count = provider_info->noutputs;
        outputs = provider_info->outputs;
    }
    /* Fake provider id, search adapters in screen resources */
    else
    {
        crtc_count = screen_resources->ncrtc;
        output_count = screen_resources->noutput;
        outputs = screen_resources->outputs;
    }

    /* Actual adapter count could be less */
    adapters = calloc( crtc_count, sizeof(*adapters) );
    if (!adapters)
        goto done;

    primary_rect = get_primary_rect( screen_resources );
    for (i = 0; i < output_count; ++i)
    {
        output_info = pXRRGetOutputInfo( gdi_display, screen_resources, outputs[i] );
        if (!output_info)
            goto done;

        /* Only connected output are considered as monitors */
        if (output_info->connection != RR_Connected)
        {
            pXRRFreeOutputInfo( output_info );
            output_info = NULL;
            continue;
        }

        /* Connected output doesn't mean the output is attached to a crtc */
        detached = FALSE;
        if (output_info->crtc)
        {
            crtc_info = pXRRGetCrtcInfo( gdi_display, screen_resources, output_info->crtc );
            if (!crtc_info)
                goto done;
        }

        if (!output_info->crtc || !crtc_info->mode)
            detached = TRUE;

        /* Ignore mirroring output replicas because mirrored monitors are under the same adapter */
        mirrored = FALSE;
        if (!detached)
        {
            for (j = 0; j < screen_resources->noutput; ++j)
            {
                enum_output_info = pXRRGetOutputInfo( gdi_display, screen_resources, screen_resources->outputs[j] );
                if (!enum_output_info)
                    continue;

                if (enum_output_info->connection != RR_Connected || !enum_output_info->crtc)
                {
                    pXRRFreeOutputInfo( enum_output_info );
                    continue;
                }

                enum_crtc_info = pXRRGetCrtcInfo( gdi_display, screen_resources, enum_output_info->crtc );
                pXRRFreeOutputInfo( enum_output_info );
                if (!enum_crtc_info)
                    continue;

                /* Some outputs may have the same coordinates, aka mirrored. Choose the output with
                 * the lowest value as primary and the rest will then be replicas in a mirroring set */
                if (crtc_info->x == enum_crtc_info->x &&
                    crtc_info->y == enum_crtc_info->y &&
                    crtc_info->width == enum_crtc_info->width &&
                    crtc_info->height == enum_crtc_info->height &&
                    outputs[i] > screen_resources->outputs[j])
                {
                    mirrored = TRUE;
                    pXRRFreeCrtcInfo( enum_crtc_info );
                    break;
                }

                pXRRFreeCrtcInfo( enum_crtc_info );
            }
        }

        if (!mirrored || detached)
        {
            /* Use RROutput as adapter id. The reason of not using RRCrtc is that we need to detect inactive but
             * attached monitors */
            adapters[adapter_count].id = outputs[i];
            if (!detached)
                adapters[adapter_count].state_flags |= DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
            if (is_crtc_primary( primary_rect, crtc_info ))
            {
                adapters[adapter_count].state_flags |= DISPLAY_DEVICE_PRIMARY_DEVICE;
                primary_adapter = adapter_count;
            }

            ++adapter_count;
        }

        pXRRFreeOutputInfo( output_info );
        output_info = NULL;
        if (crtc_info)
        {
            pXRRFreeCrtcInfo( crtc_info );
            crtc_info = NULL;
        }
    }

    /* Make primary adapter the first */
    if (primary_adapter)
    {
        struct gdi_adapter tmp = adapters[0];
        adapters[0] = adapters[primary_adapter];
        adapters[primary_adapter] = tmp;
    }

    *new_adapters = adapters;
    *count = adapter_count;
    ret = TRUE;
done:
    if (screen_resources)
        pXRRFreeScreenResources( screen_resources );
    if (provider_info)
        pXRRFreeProviderInfo( provider_info );
    if (output_info)
        pXRRFreeOutputInfo( output_info );
    if (crtc_info)
        pXRRFreeCrtcInfo( crtc_info );
    if (!ret)
    {
        free( adapters );
        ERR("Failed to get adapters\n");
    }
    return ret;
}

static void xrandr14_free_adapters( struct gdi_adapter *adapters )
{
    free( adapters );
}

static BOOL xrandr14_get_monitors( ULONG_PTR adapter_id, struct gdi_monitor **new_monitors, int *count )
{
    static const WCHAR generic_nonpnp_monitorW[] = {
        'G','e','n','e','r','i','c',' ',
        'N','o','n','-','P','n','P',' ','M','o','n','i','t','o','r',0};
    struct gdi_monitor *realloc_monitors, *monitors = NULL;
    XRRScreenResources *screen_resources = NULL;
    XRROutputInfo *output_info = NULL, *enum_output_info = NULL;
    XRRCrtcInfo *crtc_info = NULL, *enum_crtc_info;
    INT primary_index = 0, monitor_count = 0, capacity;
    RECT primary_rect;
    BOOL ret = FALSE;
    INT i;

    screen_resources = xrandr_get_screen_resources();
    if (!screen_resources)
        goto done;

    /* First start with a 2 monitors, should be enough for most cases */
    capacity = 2;
    monitors = calloc( capacity, sizeof(*monitors) );
    if (!monitors)
        goto done;

    output_info = pXRRGetOutputInfo( gdi_display, screen_resources, adapter_id );
    if (!output_info)
        goto done;

    if (output_info->crtc)
    {
        crtc_info = pXRRGetCrtcInfo( gdi_display, screen_resources, output_info->crtc );
        if (!crtc_info)
            goto done;
    }

    /* Inactive but attached monitor, no need to check for mirrored/replica monitors */
    if (!output_info->crtc || !crtc_info->mode)
    {
        lstrcpyW( monitors[monitor_count].name, generic_nonpnp_monitorW );
        monitors[monitor_count].state_flags = DISPLAY_DEVICE_ATTACHED;
        monitors[monitor_count].edid_len = get_edid( adapter_id, &monitors[monitor_count].edid );
        monitor_count = 1;
    }
    /* Active monitors, need to find other monitors with the same coordinates as mirrored */
    else
    {
        primary_rect = get_primary_rect( screen_resources );

        for (i = 0; i < screen_resources->noutput; ++i)
        {
            enum_output_info = pXRRGetOutputInfo( gdi_display, screen_resources, screen_resources->outputs[i] );
            if (!enum_output_info)
                goto done;

            /* Detached outputs don't count */
            if (enum_output_info->connection != RR_Connected)
            {
                pXRRFreeOutputInfo( enum_output_info );
                enum_output_info = NULL;
                continue;
            }

            /* Allocate more space if needed */
            if (monitor_count >= capacity)
            {
                capacity *= 2;
                realloc_monitors = realloc( monitors, capacity * sizeof(*monitors) );
                if (!realloc_monitors)
                    goto done;
                monitors = realloc_monitors;
            }

            if (enum_output_info->crtc)
            {
                enum_crtc_info = pXRRGetCrtcInfo( gdi_display, screen_resources, enum_output_info->crtc );
                if (!enum_crtc_info)
                    goto done;

                if (enum_crtc_info->x == crtc_info->x &&
                    enum_crtc_info->y == crtc_info->y &&
                    enum_crtc_info->width == crtc_info->width &&
                    enum_crtc_info->height == crtc_info->height)
                {
                    /* FIXME: Read output EDID property and parse the data to get the correct name */
                    lstrcpyW( monitors[monitor_count].name, generic_nonpnp_monitorW );

                    SetRect( &monitors[monitor_count].rc_monitor, crtc_info->x, crtc_info->y,
                             crtc_info->x + crtc_info->width, crtc_info->y + crtc_info->height );
                    monitors[monitor_count].rc_work = get_work_area( &monitors[monitor_count].rc_monitor );

                    monitors[monitor_count].state_flags = DISPLAY_DEVICE_ATTACHED;
                    if (!IsRectEmpty( &monitors[monitor_count].rc_monitor ))
                        monitors[monitor_count].state_flags |= DISPLAY_DEVICE_ACTIVE;

                    if (is_crtc_primary( primary_rect, crtc_info ))
                        primary_index = monitor_count;

                    monitors[monitor_count].edid_len = get_edid( screen_resources->outputs[i],
                                                                 &monitors[monitor_count].edid );
                    monitor_count++;
                }

                pXRRFreeCrtcInfo( enum_crtc_info );
            }

            pXRRFreeOutputInfo( enum_output_info );
            enum_output_info = NULL;
        }

        /* Make sure the first monitor is the primary */
        if (primary_index)
        {
            struct gdi_monitor tmp = monitors[0];
            monitors[0] = monitors[primary_index];
            monitors[primary_index] = tmp;
        }

        /* Make sure the primary monitor origin is at (0, 0) */
        for (i = 0; i < monitor_count; i++)
        {
            OffsetRect( &monitors[i].rc_monitor, -primary_rect.left, -primary_rect.top );
            OffsetRect( &monitors[i].rc_work, -primary_rect.left, -primary_rect.top );
        }
    }

    *new_monitors = monitors;
    *count = monitor_count;
    ret = TRUE;
done:
    if (screen_resources)
        pXRRFreeScreenResources( screen_resources );
    if (output_info)
        pXRRFreeOutputInfo( output_info);
    if (crtc_info)
        pXRRFreeCrtcInfo( crtc_info );
    if (enum_output_info)
        pXRRFreeOutputInfo( enum_output_info );
    if (!ret)
    {
        for (i = 0; i < monitor_count; i++)
        {
            if (monitors[i].edid)
                XFree( monitors[i].edid );
        }
        free( monitors );
        ERR("Failed to get monitors\n");
    }
    return ret;
}

static void xrandr14_free_monitors( struct gdi_monitor *monitors, int count )
{
    int i;

    for (i = 0; i < count; i++)
    {
        if (monitors[i].edid)
            XFree( monitors[i].edid );
    }
    free( monitors );
}

static BOOL xrandr14_device_change_handler( HWND hwnd, XEvent *event )
{
    xrandr14_invalidate_current_mode_cache();
    if (hwnd == NtUserGetDesktopWindow() && NtUserGetWindowThread( hwnd, NULL ) == GetCurrentThreadId())
    {
        X11DRV_DisplayDevices_Init( TRUE );
        init_registry_display_settings();
        X11DRV_resize_desktop();
    }
    return FALSE;
}

static void xrandr14_register_event_handlers(void)
{
    Display *display = thread_init_display();
    int event_base, error_base;

    if (!pXRRQueryExtension( display, &event_base, &error_base ))
        return;

    pXRRSelectInput( display, root_window,
                     RRCrtcChangeNotifyMask | RROutputChangeNotifyMask | RRProviderChangeNotifyMask );
    X11DRV_register_event_handler( event_base + RRNotify_CrtcChange, xrandr14_device_change_handler,
                                   "XRandR CrtcChange" );
    X11DRV_register_event_handler( event_base + RRNotify_OutputChange, xrandr14_device_change_handler,
                                   "XRandR OutputChange" );
    X11DRV_register_event_handler( event_base + RRNotify_ProviderChange, xrandr14_device_change_handler,
                                   "XRandR ProviderChange" );
}

/* XRandR 1.4 display settings handler */
static BOOL xrandr14_get_id( const WCHAR *device_name, ULONG_PTR *id )
{
    struct current_mode *tmp_modes, *new_current_modes = NULL;
    INT gpu_count, adapter_count, new_current_mode_count = 0;
    INT gpu_idx, adapter_idx, display_idx;
    struct gdi_adapter *adapters;
    struct gdi_gpu *gpus;
    WCHAR *end;

    /* Parse \\.\DISPLAY%d */
    display_idx = wcstol( device_name + 11, &end, 10 ) - 1;
    if (*end)
        return FALSE;

    /* Update cache */
    pthread_mutex_lock( &xrandr_mutex );
    if (!current_modes)
    {
        if (!xrandr14_get_gpus2( &gpus, &gpu_count, FALSE ))
        {
            pthread_mutex_unlock( &xrandr_mutex );
            return FALSE;
        }

        for (gpu_idx = 0; gpu_idx < gpu_count; ++gpu_idx)
        {
            if (!xrandr14_get_adapters( gpus[gpu_idx].id, &adapters, &adapter_count ))
                break;

            tmp_modes = realloc( new_current_modes, (new_current_mode_count + adapter_count) * sizeof(*tmp_modes) );
            if (!tmp_modes)
            {
                xrandr14_free_adapters( adapters );
                break;
            }
            new_current_modes = tmp_modes;

            for (adapter_idx = 0; adapter_idx < adapter_count; ++adapter_idx)
            {
                new_current_modes[new_current_mode_count + adapter_idx].id = adapters[adapter_idx].id;
                new_current_modes[new_current_mode_count + adapter_idx].loaded = FALSE;
            }
            new_current_mode_count += adapter_count;
            xrandr14_free_adapters( adapters );
        }
        xrandr14_free_gpus( gpus );

        if (new_current_modes)
        {
            free( current_modes );
            current_modes = new_current_modes;
            current_mode_count = new_current_mode_count;
        }
    }

    if (display_idx >= current_mode_count)
    {
        pthread_mutex_unlock( &xrandr_mutex );
        return FALSE;
    }

    *id = current_modes[display_idx].id;
    pthread_mutex_unlock( &xrandr_mutex );
    return TRUE;
}

static void add_xrandr14_mode( DEVMODEW *mode, XRRModeInfo *info, DWORD depth, DWORD frequency,
                               DWORD orientation )
{
    mode->dmSize = sizeof(*mode);
    mode->dmDriverExtra = sizeof(RRMode);
    mode->dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH |
                     DM_PELSHEIGHT | DM_DISPLAYFLAGS;
    if (frequency)
    {
        mode->dmFields |= DM_DISPLAYFREQUENCY;
        mode->dmDisplayFrequency = frequency;
    }
    if (orientation == DMDO_DEFAULT || orientation == DMDO_180)
    {
        mode->dmPelsWidth = info->width;
        mode->dmPelsHeight = info->height;
    }
    else
    {
        mode->dmPelsWidth = info->height;
        mode->dmPelsHeight = info->width;
    }
    mode->u1.s2.dmDisplayOrientation = orientation;
    mode->dmBitsPerPel = depth;
    mode->u2.dmDisplayFlags = 0;
    memcpy( (BYTE *)mode + sizeof(*mode), &info->id, sizeof(info->id) );
}

static BOOL xrandr14_get_modes( ULONG_PTR id, DWORD flags, DEVMODEW **new_modes, UINT *mode_count )
{
    DWORD frequency, orientation, orientation_count;
    XRRScreenResources *screen_resources;
    XRROutputInfo *output_info = NULL;
    RROutput output = (RROutput)id;
    XRRCrtcInfo *crtc_info = NULL;
    UINT depth_idx, mode_idx = 0;
    XRRModeInfo *mode_info;
    DEVMODEW *mode, *modes;
    Rotation rotations;
    BOOL ret = FALSE;
    RRCrtc crtc;
    INT i, j;

    screen_resources = xrandr_get_screen_resources();
    if (!screen_resources)
        goto done;

    output_info = pXRRGetOutputInfo( gdi_display, screen_resources, output );
    if (!output_info)
        goto done;

    if (output_info->connection != RR_Connected)
    {
        ret = TRUE;
        *new_modes = NULL;
        *mode_count = 0;
        goto done;
    }

    crtc = output_info->crtc;
    if (!crtc)
        crtc = get_output_free_crtc( screen_resources, output_info );
    if (crtc)
        crtc_info = pXRRGetCrtcInfo( gdi_display, screen_resources, crtc );

    /* If the output is connected to a CRTC, use rotations reported by the CRTC */
    if (crtc_info)
    {
        if (flags & EDS_ROTATEDMODE)
        {
            rotations = crtc_info->rotations;
        }
        else
        {
            /* According to the RandR spec, RRGetCrtcInfo should set the active rotation to Rotate_0
             * when a CRTC is disabled. However, some RandR implementations report 0 in this case */
            rotations = (crtc_info->rotation & 0xf) ? crtc_info->rotation : RR_Rotate_0;
        }
    }
    /* Not connected to CRTC, assume all rotations are supported */
    else
    {
        if (flags & EDS_ROTATEDMODE)
        {
            rotations = RR_Rotate_0 | RR_Rotate_90 | RR_Rotate_180 | RR_Rotate_270;
        }
        else
        {
            rotations = RR_Rotate_0;
        }
    }
    orientation_count = get_orientation_count( rotations );

    /* Allocate space for display modes in different color depths and orientations.
     * Store a RRMode at the end of each DEVMODEW as private driver data */
    modes = calloc( output_info->nmode * DEPTH_COUNT * orientation_count,
                    sizeof(*modes) + sizeof(RRMode) );
    if (!modes)
        goto done;

    for (i = 0; i < output_info->nmode; ++i)
    {
        for (j = 0; j < screen_resources->nmode; ++j)
        {
            if (output_info->modes[i] != screen_resources->modes[j].id)
                continue;

            mode_info = &screen_resources->modes[j];
            frequency = get_frequency( mode_info );

            for (depth_idx = 0; depth_idx < DEPTH_COUNT; ++depth_idx)
            {
                for (orientation = DMDO_DEFAULT; orientation <= DMDO_270; ++orientation)
                {
                    if (!((1 << orientation) & rotations))
                        continue;

                    mode = (DEVMODEW *)((BYTE *)modes + (sizeof(*modes) + sizeof(RRMode)) * mode_idx);
                    add_xrandr14_mode( mode, mode_info, depths[depth_idx], frequency, orientation );
                    ++mode_idx;
                }
            }

            break;
        }
    }

    ret = TRUE;
    *new_modes = modes;
    *mode_count = mode_idx;
done:
    if (crtc_info)
        pXRRFreeCrtcInfo( crtc_info );
    if (output_info)
        pXRRFreeOutputInfo( output_info );
    if (screen_resources)
        pXRRFreeScreenResources( screen_resources );
    return ret;
}

static void xrandr14_free_modes( DEVMODEW *modes )
{
    free( modes );
}

static BOOL xrandr14_get_current_mode( ULONG_PTR id, DEVMODEW *mode )
{
    struct current_mode *mode_ptr = NULL;
    XRRScreenResources *screen_resources;
    XRROutputInfo *output_info = NULL;
    RROutput output = (RROutput)id;
    XRRModeInfo *mode_info = NULL;
    XRRCrtcInfo *crtc_info = NULL;
    BOOL ret = FALSE;
    RECT primary;
    INT mode_idx;

    pthread_mutex_lock( &xrandr_mutex );
    for (mode_idx = 0; mode_idx < current_mode_count; ++mode_idx)
    {
        if (current_modes[mode_idx].id != id)
            continue;

        if (!current_modes[mode_idx].loaded)
        {
            mode_ptr = &current_modes[mode_idx];
            break;
        }

        memcpy( mode, &current_modes[mode_idx].mode, sizeof(*mode) );
        pthread_mutex_unlock( &xrandr_mutex );
        return TRUE;
    }

    screen_resources = xrandr_get_screen_resources();
    if (!screen_resources)
        goto done;

    output_info = pXRRGetOutputInfo( gdi_display, screen_resources, output );
    if (!output_info)
        goto done;

    if (output_info->crtc)
    {
        crtc_info = pXRRGetCrtcInfo( gdi_display, screen_resources, output_info->crtc );
        if (!crtc_info)
            goto done;
    }

    /* Detached */
    if (output_info->connection != RR_Connected || !output_info->crtc || !crtc_info->mode)
    {
        mode->dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                         DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY | DM_POSITION;
        mode->u1.s2.dmDisplayOrientation = DMDO_DEFAULT;
        mode->dmBitsPerPel = 0;
        mode->dmPelsWidth = 0;
        mode->dmPelsHeight = 0;
        mode->u2.dmDisplayFlags = 0;
        mode->dmDisplayFrequency = 0;
        mode->u1.s2.dmPosition.x = 0;
        mode->u1.s2.dmPosition.y = 0;
        ret = TRUE;
        goto done;
    }

    /* Attached */
    for (mode_idx = 0; mode_idx < screen_resources->nmode; ++mode_idx)
    {
        if (crtc_info->mode == screen_resources->modes[mode_idx].id)
        {
            mode_info = &screen_resources->modes[mode_idx];
            break;
        }
    }

    if (!mode_info)
        goto done;

    mode->dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                     DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY | DM_POSITION;
    mode->u1.s2.dmDisplayOrientation = get_orientation( crtc_info->rotation );
    mode->dmBitsPerPel = screen_bpp;
    mode->dmPelsWidth = crtc_info->width;
    mode->dmPelsHeight = crtc_info->height;
    mode->u2.dmDisplayFlags = 0;
    mode->dmDisplayFrequency = get_frequency( mode_info );
    /* Convert RandR coordinates to virtual screen coordinates */
    primary = get_primary_rect( screen_resources );
    mode->u1.s2.dmPosition.x = crtc_info->x - primary.left;
    mode->u1.s2.dmPosition.y = crtc_info->y - primary.top;
    ret = TRUE;

done:
    if (ret && mode_ptr)
    {
        memcpy( &mode_ptr->mode, mode, sizeof(*mode) );
        mode_ptr->mode.dmSize = sizeof(*mode);
        mode_ptr->mode.dmDriverExtra = 0;
        mode_ptr->loaded = TRUE;
    }
    pthread_mutex_unlock( &xrandr_mutex );
    if (crtc_info)
        pXRRFreeCrtcInfo( crtc_info );
    if (output_info)
        pXRRFreeOutputInfo( output_info );
    if (screen_resources)
        pXRRFreeScreenResources( screen_resources );
    return ret;
}

static LONG xrandr14_set_current_mode( ULONG_PTR id, const DEVMODEW *mode )
{
    unsigned int screen_width, screen_height;
    RROutput output = (RROutput)id, *outputs;
    XRRScreenResources *screen_resources;
    XRROutputInfo *output_info = NULL;
    XRRCrtcInfo *crtc_info = NULL;
    LONG ret = DISP_CHANGE_FAILED;
    Rotation rotation;
    INT output_count;
    RRCrtc crtc = 0;
    Status status;
    RRMode rrmode;

    if (mode->dmFields & DM_BITSPERPEL && mode->dmBitsPerPel != screen_bpp)
        WARN("Cannot change screen color depth from %ubits to %ubits!\n", screen_bpp, mode->dmBitsPerPel);

    screen_resources = xrandr_get_screen_resources();
    if (!screen_resources)
        return ret;

    XGrabServer( gdi_display );

    output_info = pXRRGetOutputInfo( gdi_display, screen_resources, output );
    if (!output_info || output_info->connection != RR_Connected)
        goto done;

    if (is_detached_mode(mode))
    {
        /* Already detached */
        if (!output_info->crtc)
        {
            ret = DISP_CHANGE_SUCCESSFUL;
            goto done;
        }

        /* Execute detach operation */
        status = pXRRSetCrtcConfig( gdi_display, screen_resources, output_info->crtc,
                                    CurrentTime, 0, 0, None, RR_Rotate_0, NULL, 0 );
        if (status == RRSetConfigSuccess)
        {
            get_screen_size( screen_resources, &screen_width, &screen_height );
            set_screen_size( screen_width, screen_height );
            ret = DISP_CHANGE_SUCCESSFUL;
        }
        goto done;
    }

    /* Attached */
    if (output_info->crtc)
    {
        crtc = output_info->crtc;
    }
    /* Detached, need to find a free CRTC */
    else
    {
        if (!(crtc = get_output_free_crtc( screen_resources, output_info )))
            goto done;
    }

    crtc_info = pXRRGetCrtcInfo( gdi_display, screen_resources, crtc );
    if (!crtc_info)
        goto done;

    assert( mode->dmDriverExtra == sizeof(RRMode) );
    memcpy( &rrmode, (BYTE *)mode + sizeof(*mode), sizeof(rrmode) );

    if (crtc_info->noutput)
    {
        outputs = crtc_info->outputs;
        output_count = crtc_info->noutput;
    }
    else
    {
        outputs = &output;
        output_count = 1;
    }
    rotation = get_rotation( mode->u1.s2.dmDisplayOrientation );

    /* According to the RandR spec, the entire CRTC must fit inside the screen.
     * Since we use the union of all enabled CRTCs to determine the necessary
     * screen size, this might involve shrinking the screen, so we must disable
     * the CRTC in question first. */
    status = pXRRSetCrtcConfig( gdi_display, screen_resources, crtc, CurrentTime, 0, 0, None,
                                RR_Rotate_0, NULL, 0 );
    if (status != RRSetConfigSuccess)
        goto done;

    get_screen_size( screen_resources, &screen_width, &screen_height );
    screen_width = max( screen_width, mode->u1.s2.dmPosition.x + mode->dmPelsWidth );
    screen_height = max( screen_height, mode->u1.s2.dmPosition.y + mode->dmPelsHeight );
    set_screen_size( screen_width, screen_height );

    status = pXRRSetCrtcConfig( gdi_display, screen_resources, crtc, CurrentTime,
                                mode->u1.s2.dmPosition.x, mode->u1.s2.dmPosition.y, rrmode,
                                rotation, outputs, output_count );
    if (status == RRSetConfigSuccess)
        ret = DISP_CHANGE_SUCCESSFUL;

done:
    XUngrabServer( gdi_display );
    XFlush( gdi_display );
    if (crtc_info)
        pXRRFreeCrtcInfo( crtc_info );
    if (output_info)
        pXRRFreeOutputInfo( output_info );
    pXRRFreeScreenResources( screen_resources );
    xrandr14_invalidate_current_mode_cache();
    return ret;
}

#endif

void X11DRV_XRandR_Init(void)
{
    struct x11drv_display_device_handler display_handler;
    struct x11drv_settings_handler settings_handler;
    int event_base, error_base, minor, ret;
    static int major;
    Bool ok;

    if (major) return; /* already initialized? */
    if (!usexrandr) return; /* disabled in config */
    if (is_virtual_desktop()) return;
    if (!(ret = load_xrandr())) return;  /* can't load the Xrandr library */

    /* see if Xrandr is available */
    if (!pXRRQueryExtension( gdi_display, &event_base, &error_base )) return;
    X11DRV_expect_error( gdi_display, XRandRErrorHandler, NULL );
    ok = pXRRQueryVersion( gdi_display, &major, &minor );
    if (X11DRV_check_error() || !ok) return;

    TRACE("Found XRandR %d.%d.\n", major, minor);

    settings_handler.name = "XRandR 1.0";
    settings_handler.priority = 200;
    settings_handler.get_id = xrandr10_get_id;
    settings_handler.get_modes = xrandr10_get_modes;
    settings_handler.free_modes = xrandr10_free_modes;
    settings_handler.get_current_mode = xrandr10_get_current_mode;
    settings_handler.set_current_mode = xrandr10_set_current_mode;
    X11DRV_Settings_SetHandler( &settings_handler );

#ifdef HAVE_XRRGETPROVIDERRESOURCES
    if (ret >= 4 && (major > 1 || (major == 1 && minor >= 4)))
    {
        XRRScreenResources *screen_resources;
        XRROutputInfo *output_info;
        BOOL found_output = FALSE;
        INT i;

        screen_resources = xrandr_get_screen_resources();
        if (!screen_resources)
            return;

        for (i = 0; i < screen_resources->noutput; ++i)
        {
            output_info = pXRRGetOutputInfo( gdi_display, screen_resources, screen_resources->outputs[i] );
            if (!output_info)
                continue;

            if (output_info->connection == RR_Connected)
            {
                pXRRFreeOutputInfo( output_info );
                found_output = TRUE;
                break;
            }

            pXRRFreeOutputInfo( output_info );
        }
        pXRRFreeScreenResources( screen_resources );

        if (!found_output)
        {
            WARN("No connected outputs found.\n");
            return;
        }

        display_handler.name = "XRandR 1.4";
        display_handler.priority = 200;
        display_handler.get_gpus = xrandr14_get_gpus;
        display_handler.get_adapters = xrandr14_get_adapters;
        display_handler.get_monitors = xrandr14_get_monitors;
        display_handler.free_gpus = xrandr14_free_gpus;
        display_handler.free_adapters = xrandr14_free_adapters;
        display_handler.free_monitors = xrandr14_free_monitors;
        display_handler.register_event_handlers = xrandr14_register_event_handlers;
        X11DRV_DisplayDevices_SetHandler( &display_handler );

        if (is_broken_driver())
            return;

        settings_handler.name = "XRandR 1.4";
        settings_handler.priority = 300;
        settings_handler.get_id = xrandr14_get_id;
        settings_handler.get_modes = xrandr14_get_modes;
        settings_handler.free_modes = xrandr14_free_modes;
        settings_handler.get_current_mode = xrandr14_get_current_mode;
        settings_handler.set_current_mode = xrandr14_set_current_mode;
        X11DRV_Settings_SetHandler( &settings_handler );
    }
#endif
}

#else /* SONAME_LIBXRANDR */

void X11DRV_XRandR_Init(void)
{
    TRACE("XRandR support not compiled in.\n");
}

#endif /* SONAME_LIBXRANDR */
