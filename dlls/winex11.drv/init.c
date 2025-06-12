/*
 * X11 graphics driver initialisation functions
 *
 * Copyright 1996 Alexandre Julliard
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

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "x11drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

Display *gdi_display;  /* display to use for all GDI functions */

static int palette_size;

static Pixmap stock_bitmap_pixmap;  /* phys bitmap for the default stock bitmap */

static pthread_once_t init_once = PTHREAD_ONCE_INIT;

static const struct user_driver_funcs x11drv_funcs;
static const struct gdi_dc_funcs *xrender_funcs;


void init_recursive_mutex( pthread_mutex_t *mutex )
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init( &attr );
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
    pthread_mutex_init( mutex, &attr );
    pthread_mutexattr_destroy( &attr );
}


/**********************************************************************
 *	     device_init
 *
 * Perform initializations needed upon creation of the first device.
 */
static void device_init(void)
{
    /* Initialize XRender */
    xrender_funcs = X11DRV_XRender_Init();

    /* Init Xcursor */
    X11DRV_Xcursor_Init();

    palette_size = X11DRV_PALETTE_Init();

    stock_bitmap_pixmap = XCreatePixmap( gdi_display, root_window, 1, 1, 1 );
}


static X11DRV_PDEVICE *create_x11_physdev( Drawable drawable )
{
    X11DRV_PDEVICE *physDev;

    pthread_once( &init_once, device_init );

    if (!(physDev = calloc( 1, sizeof(*physDev) ))) return NULL;

    physDev->drawable = drawable;
    physDev->gc = XCreateGC( gdi_display, drawable, 0, NULL );
    XSetGraphicsExposures( gdi_display, physDev->gc, False );
    XSetSubwindowMode( gdi_display, physDev->gc, IncludeInferiors );
    XFlush( gdi_display );
    return physDev;
}

/**********************************************************************
 *	     X11DRV_CreateDC
 */
static BOOL X11DRV_CreateDC( PHYSDEV *pdev, LPCWSTR device, LPCWSTR output, const DEVMODEW* initData )
{
    X11DRV_PDEVICE *physDev = create_x11_physdev( root_window );

    if (!physDev) return FALSE;

    physDev->depth         = default_visual.depth;
    physDev->color_shifts  = &X11DRV_PALETTE_default_shifts;
    physDev->dc_rect       = NtUserGetVirtualScreenRect( MDT_DEFAULT );
    OffsetRect( &physDev->dc_rect, -physDev->dc_rect.left, -physDev->dc_rect.top );
    push_dc_driver( pdev, &physDev->dev, &x11drv_funcs.dc_funcs );
    if (xrender_funcs && !xrender_funcs->pCreateDC( pdev, device, output, initData )) return FALSE;
    return TRUE;
}


/**********************************************************************
 *	     X11DRV_CreateCompatibleDC
 */
static BOOL X11DRV_CreateCompatibleDC( PHYSDEV orig, PHYSDEV *pdev )
{
    X11DRV_PDEVICE *physDev = create_x11_physdev( stock_bitmap_pixmap );

    if (!physDev) return FALSE;

    physDev->depth  = 1;
    SetRect( &physDev->dc_rect, 0, 0, 1, 1 );
    push_dc_driver( pdev, &physDev->dev, &x11drv_funcs.dc_funcs );
    if (orig) return TRUE;  /* we already went through Xrender if we have an orig device */
    if (xrender_funcs && !xrender_funcs->pCreateCompatibleDC( NULL, pdev )) return FALSE;
    return TRUE;
}


/**********************************************************************
 *	     X11DRV_DeleteDC
 */
static BOOL X11DRV_DeleteDC( PHYSDEV dev )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );

    XFreeGC( gdi_display, physDev->gc );
    free( physDev );
    return TRUE;
}


void add_device_bounds( X11DRV_PDEVICE *dev, const RECT *rect )
{
    RECT rc;

    if (!dev->bounds) return;
    if (dev->region && NtGdiGetRgnBox( dev->region, &rc ))
    {
        if (intersect_rect( &rc, &rc, rect )) add_bounds_rect( dev->bounds, &rc );
    }
    else add_bounds_rect( dev->bounds, rect );
}

/***********************************************************************
 *           X11DRV_SetBoundsRect
 */
static UINT X11DRV_SetBoundsRect( PHYSDEV dev, RECT *rect, UINT flags )
{
    X11DRV_PDEVICE *pdev = get_x11drv_dev( dev );

    if (flags & DCB_DISABLE) pdev->bounds = NULL;
    else if (flags & DCB_ENABLE) pdev->bounds = rect;
    return DCB_RESET;  /* we don't have device-specific bounds */
}


/***********************************************************************
 *           GetDeviceCaps    (X11DRV.@)
 */
static INT X11DRV_GetDeviceCaps( PHYSDEV dev, INT cap )
{
    switch(cap)
    {
    case SIZEPALETTE:
        return palette_size;
    default:
        dev = GET_NEXT_PHYSDEV( dev, pGetDeviceCaps );
        return dev->funcs->pGetDeviceCaps( dev, cap );
    }
}


/***********************************************************************
 *           SelectFont
 */
static HFONT X11DRV_SelectFont( PHYSDEV dev, HFONT hfont, UINT *aa_flags )
{
    if (default_visual.depth <= 8) *aa_flags = GGO_BITMAP;  /* no anti-aliasing on <= 8bpp */
    dev = GET_NEXT_PHYSDEV( dev, pSelectFont );
    return dev->funcs->pSelectFont( dev, hfont, aa_flags );
}

static BOOL needs_client_window_clipping( HWND hwnd )
{
    RECT rect, client;
    UINT ret = 0;
    HRGN region;
    HDC hdc;

    NtUserGetClientRect( hwnd, &client, NtUserGetDpiForWindow( hwnd ) );
    OffsetRect( &client, -client.left, -client.top );

    if (!(hdc = NtUserGetDCEx( hwnd, 0, DCX_CACHE | DCX_USESTYLE ))) return FALSE;
    if ((region = NtGdiCreateRectRgn( 0, 0, 0, 0 )))
    {
        ret = NtGdiGetRandomRgn( hdc, region, SYSRGN );
        if (ret > 0 && (ret = NtGdiGetRgnBox( region, &rect )) < NULLREGION) ret = 0;
        if (ret == SIMPLEREGION && EqualRect( &rect, &client )) ret = 0;
        NtGdiDeleteObjectApp( region );
    }
    NtUserReleaseDC( hwnd, hdc );

    return ret > 0;
}

BOOL needs_offscreen_rendering( HWND hwnd )
{
    if (NtUserGetDpiForWindow( hwnd ) != NtUserGetWinMonitorDpi( hwnd, MDT_RAW_DPI )) return TRUE; /* needs DPI scaling */
    if (NtUserGetAncestor( hwnd, GA_PARENT ) != NtUserGetDesktopWindow()) return TRUE; /* child window, needs compositing */
    if (NtUserGetWindowRelative( hwnd, GW_CHILD )) return needs_client_window_clipping( hwnd ); /* window has children, needs compositing */
    return FALSE;
}

void set_dc_drawable( HDC hdc, Drawable drawable, const RECT *rect, int mode )
{
    struct x11drv_escape_set_drawable escape =
    {
        .code = X11DRV_SET_DRAWABLE,
        .drawable = drawable,
        .dc_rect = *rect,
        .mode = mode,
    };
    NtGdiExtEscape( hdc, NULL, 0, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
}

Drawable get_dc_drawable( HDC hdc, RECT *rect )
{
    struct x11drv_escape_get_drawable escape = {.code = X11DRV_GET_DRAWABLE};
    NtGdiExtEscape( hdc, NULL, 0, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, sizeof(escape), (LPSTR)&escape );
    *rect = escape.dc_rect;
    return escape.drawable;
}

HRGN get_dc_monitor_region( HWND hwnd, HDC hdc )
{
    HRGN region;

    if (!(region = NtGdiCreateRectRgn( 0, 0, 0, 0 ))) return 0;
    if (NtGdiGetRandomRgn( hdc, region, SYSRGN | NTGDI_RGN_MONITOR_DPI ) > 0) return region;
    NtGdiDeleteObjectApp( region );
    return 0;
}

/**********************************************************************
 *           ExtEscape  (X11DRV.@)
 */
static INT X11DRV_ExtEscape( PHYSDEV dev, INT escape, INT in_count, LPCVOID in_data,
                             INT out_count, LPVOID out_data )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );

    switch(escape)
    {
    case QUERYESCSUPPORT:
        if (in_data && in_count >= sizeof(DWORD))
        {
            switch (*(const INT *)in_data)
            {
            case X11DRV_ESCAPE:
                return TRUE;
            }
        }
        break;

    case X11DRV_ESCAPE:
        if (in_data && in_count >= sizeof(enum x11drv_escape_codes))
        {
            switch(*(const enum x11drv_escape_codes *)in_data)
            {
            case X11DRV_SET_DRAWABLE:
                if (in_count >= sizeof(struct x11drv_escape_set_drawable))
                {
                    const struct x11drv_escape_set_drawable *data = in_data;
                    physDev->dc_rect = data->dc_rect;
                    physDev->drawable = data->drawable;
                    XFreeGC( gdi_display, physDev->gc );
                    physDev->gc = XCreateGC( gdi_display, physDev->drawable, 0, NULL );
                    XSetGraphicsExposures( gdi_display, physDev->gc, False );
                    XSetSubwindowMode( gdi_display, physDev->gc, data->mode );
                    TRACE( "SET_DRAWABLE hdc %p drawable %lx dc_rect %s\n",
                           dev->hdc, physDev->drawable, wine_dbgstr_rect(&physDev->dc_rect) );
                    return TRUE;
                }
                break;
            case X11DRV_GET_DRAWABLE:
                if (out_count >= sizeof(struct x11drv_escape_get_drawable))
                {
                    struct x11drv_escape_get_drawable *data = out_data;
                    data->drawable = physDev->drawable;
                    data->dc_rect = physDev->dc_rect;
                    return TRUE;
                }
                break;
            case X11DRV_START_EXPOSURES:
                XSetGraphicsExposures( gdi_display, physDev->gc, True );
                physDev->exposures = 0;
                return TRUE;
            case X11DRV_END_EXPOSURES:
                if (out_count >= sizeof(HRGN))
                {
                    HRGN hrgn = 0, tmp = 0;

                    XSetGraphicsExposures( gdi_display, physDev->gc, False );
                    if (physDev->exposures)
                    {
                        for (;;)
                        {
                            XEvent event;

                            XWindowEvent( gdi_display, physDev->drawable, ~0, &event );
                            if (event.type == NoExpose) break;
                            if (event.type == GraphicsExpose)
                            {
                                DWORD layout;
                                RECT rect;

                                rect.left   = event.xgraphicsexpose.x - physDev->dc_rect.left;
                                rect.top    = event.xgraphicsexpose.y - physDev->dc_rect.top;
                                rect.right  = rect.left + event.xgraphicsexpose.width;
                                rect.bottom = rect.top + event.xgraphicsexpose.height;
                                if (NtGdiGetDCDword( dev->hdc, NtGdiGetLayout, &layout ) &&
                                    (layout & LAYOUT_RTL))
                                    mirror_rect( &physDev->dc_rect, &rect );

                                TRACE( "got %s count %d\n", wine_dbgstr_rect(&rect),
                                       event.xgraphicsexpose.count );

                                if (!tmp) tmp = NtGdiCreateRectRgn( rect.left, rect.top,
                                                                    rect.right, rect.bottom );
                                else NtGdiSetRectRgn( tmp, rect.left, rect.top, rect.right, rect.bottom );
                                if (hrgn) NtGdiCombineRgn( hrgn, hrgn, tmp, RGN_OR );
                                else
                                {
                                    hrgn = tmp;
                                    tmp = 0;
                                }
                                if (!event.xgraphicsexpose.count) break;
                            }
                            else
                            {
                                ERR( "got unexpected event %d\n", event.type );
                                break;
                            }
                        }
                        if (tmp) NtGdiDeleteObjectApp( tmp );
                    }
                    *(HRGN *)out_data = hrgn;
                    return TRUE;
                }
                break;
            default:
                break;
            }
        }
        break;
    }
    return 0;
}


static const struct user_driver_funcs x11drv_funcs =
{
    .dc_funcs.pArc = X11DRV_Arc,
    .dc_funcs.pChord = X11DRV_Chord,
    .dc_funcs.pCreateCompatibleDC = X11DRV_CreateCompatibleDC,
    .dc_funcs.pCreateDC = X11DRV_CreateDC,
    .dc_funcs.pDeleteDC = X11DRV_DeleteDC,
    .dc_funcs.pEllipse = X11DRV_Ellipse,
    .dc_funcs.pExtEscape = X11DRV_ExtEscape,
    .dc_funcs.pExtFloodFill = X11DRV_ExtFloodFill,
    .dc_funcs.pFillPath = X11DRV_FillPath,
    .dc_funcs.pGetDeviceCaps = X11DRV_GetDeviceCaps,
    .dc_funcs.pGetDeviceGammaRamp = X11DRV_GetDeviceGammaRamp,
    .dc_funcs.pGetICMProfile = X11DRV_GetICMProfile,
    .dc_funcs.pGetImage = X11DRV_GetImage,
    .dc_funcs.pGetNearestColor = X11DRV_GetNearestColor,
    .dc_funcs.pGetSystemPaletteEntries = X11DRV_GetSystemPaletteEntries,
    .dc_funcs.pGradientFill = X11DRV_GradientFill,
    .dc_funcs.pLineTo = X11DRV_LineTo,
    .dc_funcs.pPaintRgn = X11DRV_PaintRgn,
    .dc_funcs.pPatBlt = X11DRV_PatBlt,
    .dc_funcs.pPie = X11DRV_Pie,
    .dc_funcs.pPolyPolygon = X11DRV_PolyPolygon,
    .dc_funcs.pPolyPolyline = X11DRV_PolyPolyline,
    .dc_funcs.pPutImage = X11DRV_PutImage,
    .dc_funcs.pRealizeDefaultPalette = X11DRV_RealizeDefaultPalette,
    .dc_funcs.pRealizePalette = X11DRV_RealizePalette,
    .dc_funcs.pRectangle = X11DRV_Rectangle,
    .dc_funcs.pRoundRect = X11DRV_RoundRect,
    .dc_funcs.pSelectBrush = X11DRV_SelectBrush,
    .dc_funcs.pSelectFont = X11DRV_SelectFont,
    .dc_funcs.pSelectPen = X11DRV_SelectPen,
    .dc_funcs.pSetBoundsRect = X11DRV_SetBoundsRect,
    .dc_funcs.pSetDCBrushColor = X11DRV_SetDCBrushColor,
    .dc_funcs.pSetDCPenColor = X11DRV_SetDCPenColor,
    .dc_funcs.pSetDeviceClipping = X11DRV_SetDeviceClipping,
    .dc_funcs.pSetDeviceGammaRamp = X11DRV_SetDeviceGammaRamp,
    .dc_funcs.pSetPixel = X11DRV_SetPixel,
    .dc_funcs.pStretchBlt = X11DRV_StretchBlt,
    .dc_funcs.pStrokeAndFillPath = X11DRV_StrokeAndFillPath,
    .dc_funcs.pStrokePath = X11DRV_StrokePath,
    .dc_funcs.pUnrealizePalette = X11DRV_UnrealizePalette,
    .dc_funcs.priority = GDI_PRIORITY_GRAPHICS_DRV,

    .pActivateKeyboardLayout = X11DRV_ActivateKeyboardLayout,
    .pBeep = X11DRV_Beep,
    .pGetKeyNameText = X11DRV_GetKeyNameText,
    .pMapVirtualKeyEx = X11DRV_MapVirtualKeyEx,
    .pToUnicodeEx = X11DRV_ToUnicodeEx,
    .pVkKeyScanEx = X11DRV_VkKeyScanEx,
    .pNotifyIMEStatus = X11DRV_NotifyIMEStatus,
    .pSetIMECompositionRect = X11DRV_SetIMECompositionRect,
    .pDestroyCursorIcon = X11DRV_DestroyCursorIcon,
    .pSetCursor = X11DRV_SetCursor,
    .pGetCursorPos = X11DRV_GetCursorPos,
    .pSetCursorPos = X11DRV_SetCursorPos,
    .pClipCursor = X11DRV_ClipCursor,
    .pSystrayDockInit = X11DRV_SystrayDockInit,
    .pSystrayDockInsert = X11DRV_SystrayDockInsert,
    .pSystrayDockClear = X11DRV_SystrayDockClear,
    .pSystrayDockRemove = X11DRV_SystrayDockRemove,
    .pChangeDisplaySettings = X11DRV_ChangeDisplaySettings,
    .pUpdateDisplayDevices = X11DRV_UpdateDisplayDevices,
    .pCreateDesktop = X11DRV_CreateDesktop,
    .pCreateWindow = X11DRV_CreateWindow,
    .pDesktopWindowProc = X11DRV_DesktopWindowProc,
    .pDestroyWindow = X11DRV_DestroyWindow,
    .pFlashWindowEx = X11DRV_FlashWindowEx,
    .pGetDC = X11DRV_GetDC,
    .pProcessEvents = X11DRV_ProcessEvents,
    .pReleaseDC = X11DRV_ReleaseDC,
    .pScrollDC = X11DRV_ScrollDC,
    .pSetCapture = X11DRV_SetCapture,
    .pSetDesktopWindow = X11DRV_SetDesktopWindow,
    .pActivateWindow = X11DRV_ActivateWindow,
    .pSetLayeredWindowAttributes = X11DRV_SetLayeredWindowAttributes,
    .pSetParent = X11DRV_SetParent,
    .pSetWindowIcon = X11DRV_SetWindowIcon,
    .pSetWindowRgn = X11DRV_SetWindowRgn,
    .pSetWindowStyle = X11DRV_SetWindowStyle,
    .pSetWindowText = X11DRV_SetWindowText,
    .pShowWindow = X11DRV_ShowWindow,
    .pSysCommand = X11DRV_SysCommand,
    .pClipboardWindowProc = X11DRV_ClipboardWindowProc,
    .pUpdateClipboard = X11DRV_UpdateClipboard,
    .pUpdateLayeredWindow = X11DRV_UpdateLayeredWindow,
    .pWindowMessage = X11DRV_WindowMessage,
    .pWindowPosChanging = X11DRV_WindowPosChanging,
    .pGetWindowStyleMasks = X11DRV_GetWindowStyleMasks,
    .pGetWindowStateUpdates = X11DRV_GetWindowStateUpdates,
    .pCreateWindowSurface = X11DRV_CreateWindowSurface,
    .pMoveWindowBits = X11DRV_MoveWindowBits,
    .pWindowPosChanged = X11DRV_WindowPosChanged,
    .pSystemParametersInfo = X11DRV_SystemParametersInfo,
    .pVulkanInit = X11DRV_VulkanInit,
    .pOpenGLInit = X11DRV_OpenGLInit,
    .pThreadDetach = X11DRV_ThreadDetach,
};


void init_user_driver(void)
{
    __wine_set_user_driver( &x11drv_funcs, WINE_GDI_DRIVER_VERSION );
}
