/*
 * Mac graphics driver initialisation functions
 *
 * Copyright 1996 Alexandre Julliard
 * Copyright 2011, 2012, 2013 Ken Thomases for CodeWeavers, Inc.
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

#include "macdrv.h"
#include "winreg.h"

WINE_DEFAULT_DEBUG_CHANNEL(macdrv);


typedef struct
{
    struct gdi_physdev  dev;
} MACDRV_PDEVICE;

static inline MACDRV_PDEVICE *get_macdrv_dev(PHYSDEV dev)
{
    return (MACDRV_PDEVICE*)dev;
}


/* a few dynamic device caps */
static CGRect desktop_rect;     /* virtual desktop rectangle */
static int horz_size;           /* horz. size of screen in millimeters */
static int vert_size;           /* vert. size of screen in millimeters */
static int bits_per_pixel;      /* pixel depth of screen */
static int device_data_valid;   /* do the above variables have up-to-date values? */

int retina_on = FALSE;

static pthread_mutex_t device_data_mutex = PTHREAD_MUTEX_INITIALIZER;

static const struct user_driver_funcs macdrv_funcs;

/***********************************************************************
 *              compute_desktop_rect
 */
static void compute_desktop_rect(void)
{
    CGDirectDisplayID displayIDs[32];
    uint32_t count, i;

    desktop_rect = CGRectNull;
    if (CGGetOnlineDisplayList(ARRAY_SIZE(displayIDs), displayIDs, &count) != kCGErrorSuccess ||
        !count)
    {
        displayIDs[0] = CGMainDisplayID();
        count = 1;
    }

    for (i = 0; i < count; i++)
        desktop_rect = CGRectUnion(desktop_rect, CGDisplayBounds(displayIDs[i]));
    desktop_rect = cgrect_win_from_mac(desktop_rect);
}


/***********************************************************************
 *              macdrv_get_desktop_rect
 *
 * Returns the rectangle encompassing all the screens.
 */
CGRect macdrv_get_desktop_rect(void)
{
    CGRect ret;

    pthread_mutex_lock(&device_data_mutex);

    if (!device_data_valid)
    {
        check_retina_status();
        compute_desktop_rect();
    }
    ret = desktop_rect;

    pthread_mutex_unlock(&device_data_mutex);

    TRACE("%s\n", wine_dbgstr_cgrect(ret));

    return ret;
}


/**********************************************************************
 *              device_init
 *
 * Perform initializations needed upon creation of the first device.
 */
static void device_init(void)
{
    CGDirectDisplayID mainDisplay = CGMainDisplayID();
    CGSize size_mm = CGDisplayScreenSize(mainDisplay);
    CGDisplayModeRef mode = CGDisplayCopyDisplayMode(mainDisplay);

    check_retina_status();

    /* Initialize device caps */
    horz_size = size_mm.width;
    vert_size = size_mm.height;

    bits_per_pixel = 32;
    if (mode)
    {
        CFStringRef pixelEncoding = CGDisplayModeCopyPixelEncoding(mode);

        if (pixelEncoding)
        {
            if (CFEqual(pixelEncoding, CFSTR(IO32BitDirectPixels)))
                bits_per_pixel = 32;
            else if (CFEqual(pixelEncoding, CFSTR(IO16BitDirectPixels)))
                bits_per_pixel = 16;
            else if (CFEqual(pixelEncoding, CFSTR(IO8BitIndexedPixels)))
                bits_per_pixel = 8;
            CFRelease(pixelEncoding);
        }

        CGDisplayModeRelease(mode);
    }

    compute_desktop_rect();

    device_data_valid = TRUE;
}


void macdrv_reset_device_metrics(void)
{
    pthread_mutex_lock(&device_data_mutex);
    device_data_valid = FALSE;
    pthread_mutex_unlock(&device_data_mutex);
}


static MACDRV_PDEVICE *create_mac_physdev(void)
{
    MACDRV_PDEVICE *physDev;

    pthread_mutex_lock(&device_data_mutex);
    if (!device_data_valid) device_init();
    pthread_mutex_unlock(&device_data_mutex);

    if (!(physDev = calloc(1, sizeof(*physDev)))) return NULL;

    return physDev;
}


/**********************************************************************
 *              CreateDC (MACDRV.@)
 */
static BOOL macdrv_CreateDC(PHYSDEV *pdev, LPCWSTR device, LPCWSTR output, const DEVMODEW* initData)
{
    MACDRV_PDEVICE *physDev = create_mac_physdev();

    TRACE("pdev %p hdc %p device %s output %s initData %p\n", pdev,
          (*pdev)->hdc, debugstr_w(device), debugstr_w(output), initData);

    if (!physDev) return FALSE;

    push_dc_driver(pdev, &physDev->dev, &macdrv_funcs.dc_funcs);
    return TRUE;
}


/**********************************************************************
 *              CreateCompatibleDC (MACDRV.@)
 */
static BOOL macdrv_CreateCompatibleDC(PHYSDEV orig, PHYSDEV *pdev)
{
    MACDRV_PDEVICE *physDev = create_mac_physdev();

    TRACE("orig %p orig->hdc %p pdev %p pdev->hdc %p\n", orig, (orig ? orig->hdc : NULL), pdev,
          ((pdev && *pdev) ? (*pdev)->hdc : NULL));

    if (!physDev) return FALSE;

    push_dc_driver(pdev, &physDev->dev, &macdrv_funcs.dc_funcs);
    return TRUE;
}


/**********************************************************************
 *              DeleteDC (MACDRV.@)
 */
static BOOL macdrv_DeleteDC(PHYSDEV dev)
{
    MACDRV_PDEVICE *physDev = get_macdrv_dev(dev);

    TRACE("hdc %p\n", dev->hdc);

    free(physDev);
    return TRUE;
}


/***********************************************************************
 *              GetDeviceCaps (MACDRV.@)
 */
static INT macdrv_GetDeviceCaps(PHYSDEV dev, INT cap)
{
    INT ret;

    pthread_mutex_lock(&device_data_mutex);

    if (!device_data_valid) device_init();

    switch(cap)
    {
    case HORZSIZE:
        ret = horz_size;
        break;
    case VERTSIZE:
        ret = vert_size;
        break;
    case BITSPIXEL:
        ret = bits_per_pixel;
        break;
    case HORZRES:
    case VERTRES:
    default:
        pthread_mutex_unlock(&device_data_mutex);
        dev = GET_NEXT_PHYSDEV( dev, pGetDeviceCaps );
        ret = dev->funcs->pGetDeviceCaps( dev, cap );
        if ((cap == HORZRES || cap == VERTRES) && retina_on)
            ret *= 2;
        return ret;
    }

    TRACE("cap %d -> %d\n", cap, ret);

    pthread_mutex_unlock(&device_data_mutex);
    return ret;
}


static const struct user_driver_funcs macdrv_funcs =
{
    .dc_funcs.pCreateCompatibleDC = macdrv_CreateCompatibleDC,
    .dc_funcs.pCreateDC = macdrv_CreateDC,
    .dc_funcs.pDeleteDC = macdrv_DeleteDC,
    .dc_funcs.pGetDeviceCaps = macdrv_GetDeviceCaps,
    .dc_funcs.pGetDeviceGammaRamp = macdrv_GetDeviceGammaRamp,
    .dc_funcs.pSetDeviceGammaRamp = macdrv_SetDeviceGammaRamp,
    .dc_funcs.priority = GDI_PRIORITY_GRAPHICS_DRV,

    .pActivateKeyboardLayout = macdrv_ActivateKeyboardLayout,
    .pBeep = macdrv_Beep,
    .pChangeDisplaySettings = macdrv_ChangeDisplaySettings,
    .pClipCursor = macdrv_ClipCursor,
    .pNotifyIcon = macdrv_NotifyIcon,
    .pCleanupIcons = macdrv_CleanupIcons,
    .pClipboardWindowProc = macdrv_ClipboardWindowProc,
    .pDesktopWindowProc = macdrv_DesktopWindowProc,
    .pDestroyCursorIcon = macdrv_DestroyCursorIcon,
    .pDestroyWindow = macdrv_DestroyWindow,
    .pGetCurrentDisplaySettings = macdrv_GetCurrentDisplaySettings,
    .pGetDisplayDepth = macdrv_GetDisplayDepth,
    .pUpdateDisplayDevices = macdrv_UpdateDisplayDevices,
    .pGetCursorPos = macdrv_GetCursorPos,
    .pGetKeyboardLayoutList = macdrv_GetKeyboardLayoutList,
    .pGetKeyNameText = macdrv_GetKeyNameText,
    .pMapVirtualKeyEx = macdrv_MapVirtualKeyEx,
    .pProcessEvents = macdrv_ProcessEvents,
    .pRegisterHotKey = macdrv_RegisterHotKey,
    .pSetCapture = macdrv_SetCapture,
    .pSetCursor = macdrv_SetCursor,
    .pSetCursorPos = macdrv_SetCursorPos,
    .pSetDesktopWindow = macdrv_SetDesktopWindow,
    .pSetFocus = macdrv_SetFocus,
    .pSetLayeredWindowAttributes = macdrv_SetLayeredWindowAttributes,
    .pSetParent = macdrv_SetParent,
    .pSetWindowRgn = macdrv_SetWindowRgn,
    .pSetWindowStyle = macdrv_SetWindowStyle,
    .pSetWindowText = macdrv_SetWindowText,
    .pShowWindow = macdrv_ShowWindow,
    .pSysCommand =macdrv_SysCommand,
    .pSystemParametersInfo = macdrv_SystemParametersInfo,
    .pThreadDetach = macdrv_ThreadDetach,
    .pToUnicodeEx = macdrv_ToUnicodeEx,
    .pUnregisterHotKey = macdrv_UnregisterHotKey,
    .pUpdateClipboard = macdrv_UpdateClipboard,
    .pUpdateLayeredWindow = macdrv_UpdateLayeredWindow,
    .pVkKeyScanEx = macdrv_VkKeyScanEx,
    .pImeProcessKey = macdrv_ImeProcessKey,
    .pNotifyIMEStatus = macdrv_NotifyIMEStatus,
    .pWindowMessage = macdrv_WindowMessage,
    .pWindowPosChanged = macdrv_WindowPosChanged,
    .pWindowPosChanging = macdrv_WindowPosChanging,
    .pwine_get_vulkan_driver = macdrv_wine_get_vulkan_driver,
    .pwine_get_wgl_driver = macdrv_wine_get_wgl_driver,
};


void init_user_driver(void)
{
    __wine_set_user_driver( &macdrv_funcs, WINE_GDI_DRIVER_VERSION );
}
