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

static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;

static const struct gdi_dc_funcs x11drv_funcs;
static const struct gdi_dc_funcs *xrender_funcs;

/**********************************************************************
 *	     device_init
 *
 * Perform initializations needed upon creation of the first device.
 */
static BOOL WINAPI device_init( INIT_ONCE *once, void *param, void **context )
{
    /* Initialize XRender */
    xrender_funcs = X11DRV_XRender_Init();

    /* Init Xcursor */
    X11DRV_Xcursor_Init();

    palette_size = X11DRV_PALETTE_Init();

    stock_bitmap_pixmap = XCreatePixmap( gdi_display, root_window, 1, 1, 1 );

    return TRUE;
}


static X11DRV_PDEVICE *create_x11_physdev( Drawable drawable )
{
    X11DRV_PDEVICE *physDev;

    InitOnceExecuteOnce( &init_once, device_init, NULL, NULL );

    if (!(physDev = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*physDev) ))) return NULL;

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
static BOOL CDECL X11DRV_CreateDC( PHYSDEV *pdev, LPCWSTR driver, LPCWSTR device,
                                   LPCWSTR output, const DEVMODEW* initData )
{
    X11DRV_PDEVICE *physDev = create_x11_physdev( root_window );

    if (!physDev) return FALSE;

    physDev->depth         = default_visual.depth;
    physDev->color_shifts  = &X11DRV_PALETTE_default_shifts;
    physDev->dc_rect       = get_virtual_screen_rect();
    OffsetRect( &physDev->dc_rect, -physDev->dc_rect.left, -physDev->dc_rect.top );
    push_dc_driver( pdev, &physDev->dev, &x11drv_funcs );
    if (xrender_funcs && !xrender_funcs->pCreateDC( pdev, driver, device, output, initData )) return FALSE;
    return TRUE;
}


/**********************************************************************
 *	     X11DRV_CreateCompatibleDC
 */
static BOOL CDECL X11DRV_CreateCompatibleDC( PHYSDEV orig, PHYSDEV *pdev )
{
    X11DRV_PDEVICE *physDev = create_x11_physdev( stock_bitmap_pixmap );

    if (!physDev) return FALSE;

    physDev->depth  = 1;
    SetRect( &physDev->dc_rect, 0, 0, 1, 1 );
    push_dc_driver( pdev, &physDev->dev, &x11drv_funcs );
    if (orig) return TRUE;  /* we already went through Xrender if we have an orig device */
    if (xrender_funcs && !xrender_funcs->pCreateCompatibleDC( NULL, pdev )) return FALSE;
    return TRUE;
}


/**********************************************************************
 *	     X11DRV_DeleteDC
 */
static BOOL CDECL X11DRV_DeleteDC( PHYSDEV dev )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );

    XFreeGC( gdi_display, physDev->gc );
    HeapFree( GetProcessHeap(), 0, physDev );
    return TRUE;
}


void add_device_bounds( X11DRV_PDEVICE *dev, const RECT *rect )
{
    RECT rc;

    if (!dev->bounds) return;
    if (dev->region && GetRgnBox( dev->region, &rc ))
    {
        if (IntersectRect( &rc, &rc, rect )) add_bounds_rect( dev->bounds, &rc );
    }
    else add_bounds_rect( dev->bounds, rect );
}

/***********************************************************************
 *           X11DRV_SetBoundsRect
 */
static UINT CDECL X11DRV_SetBoundsRect( PHYSDEV dev, RECT *rect, UINT flags )
{
    X11DRV_PDEVICE *pdev = get_x11drv_dev( dev );

    if (flags & DCB_DISABLE) pdev->bounds = NULL;
    else if (flags & DCB_ENABLE) pdev->bounds = rect;
    return DCB_RESET;  /* we don't have device-specific bounds */
}


/***********************************************************************
 *           GetDeviceCaps    (X11DRV.@)
 */
static INT CDECL X11DRV_GetDeviceCaps( PHYSDEV dev, INT cap )
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
static HFONT CDECL X11DRV_SelectFont( PHYSDEV dev, HFONT hfont, UINT *aa_flags )
{
    if (default_visual.depth <= 8) *aa_flags = GGO_BITMAP;  /* no anti-aliasing on <= 8bpp */
    dev = GET_NEXT_PHYSDEV( dev, pSelectFont );
    return dev->funcs->pSelectFont( dev, hfont, aa_flags );
}

/**********************************************************************
 *           ExtEscape  (X11DRV.@)
 */
static INT CDECL X11DRV_ExtEscape( PHYSDEV dev, INT escape, INT in_count, LPCVOID in_data,
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
                    return TRUE;
                }
                break;
            case X11DRV_FLUSH_GL_DRAWABLE:
                if (in_count >= sizeof(struct x11drv_escape_flush_gl_drawable))
                {
                    const struct x11drv_escape_flush_gl_drawable *data = in_data;
                    RECT rect = physDev->dc_rect;

                    OffsetRect( &rect, -physDev->dc_rect.left, -physDev->dc_rect.top );
                    if (data->flush) XFlush( gdi_display );
                    XSetFunction( gdi_display, physDev->gc, GXcopy );
                    XCopyArea( gdi_display, data->gl_drawable, physDev->drawable, physDev->gc,
                               0, 0, rect.right, rect.bottom,
                               physDev->dc_rect.left, physDev->dc_rect.top );
                    add_device_bounds( physDev, &rect );
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
                                RECT rect;

                                rect.left   = event.xgraphicsexpose.x - physDev->dc_rect.left;
                                rect.top    = event.xgraphicsexpose.y - physDev->dc_rect.top;
                                rect.right  = rect.left + event.xgraphicsexpose.width;
                                rect.bottom = rect.top + event.xgraphicsexpose.height;
                                if (GetLayout( dev->hdc ) & LAYOUT_RTL)
                                    mirror_rect( &physDev->dc_rect, &rect );

                                TRACE( "got %s count %d\n", wine_dbgstr_rect(&rect),
                                       event.xgraphicsexpose.count );

                                if (!tmp) tmp = CreateRectRgnIndirect( &rect );
                                else SetRectRgn( tmp, rect.left, rect.top, rect.right, rect.bottom );
                                if (hrgn) CombineRgn( hrgn, hrgn, tmp, RGN_OR );
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
                        if (tmp) DeleteObject( tmp );
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

/**********************************************************************
 *           X11DRV_wine_get_wgl_driver
 */
static struct opengl_funcs * CDECL X11DRV_wine_get_wgl_driver( PHYSDEV dev, UINT version )
{
    struct opengl_funcs *ret;

    if (!(ret = get_glx_driver( version )))
    {
        dev = GET_NEXT_PHYSDEV( dev, wine_get_wgl_driver );
        ret = dev->funcs->wine_get_wgl_driver( dev, version );
    }
    return ret;
}

/**********************************************************************
 *           X11DRV_wine_get_vulkan_driver
 */
static const struct vulkan_funcs * CDECL X11DRV_wine_get_vulkan_driver( PHYSDEV dev, UINT version )
{
    const struct vulkan_funcs *ret;

    if (!(ret = get_vulkan_driver( version )))
    {
        dev = GET_NEXT_PHYSDEV( dev, wine_get_vulkan_driver );
        ret = dev->funcs->wine_get_vulkan_driver( dev, version );
    }
    return ret;
}


static const struct gdi_dc_funcs x11drv_funcs =
{
    NULL,                               /* pAbortDoc */
    NULL,                               /* pAbortPath */
    NULL,                               /* pAlphaBlend */
    NULL,                               /* pAngleArc */
    X11DRV_Arc,                         /* pArc */
    NULL,                               /* pArcTo */
    NULL,                               /* pBeginPath */
    NULL,                               /* pBlendImage */
    X11DRV_Chord,                       /* pChord */
    NULL,                               /* pCloseFigure */
    X11DRV_CreateCompatibleDC,          /* pCreateCompatibleDC */
    X11DRV_CreateDC,                    /* pCreateDC */
    X11DRV_DeleteDC,                    /* pDeleteDC */
    NULL,                               /* pDeleteObject */
    NULL,                               /* pDeviceCapabilities */
    X11DRV_Ellipse,                     /* pEllipse */
    NULL,                               /* pEndDoc */
    NULL,                               /* pEndPage */
    NULL,                               /* pEndPath */
    NULL,                               /* pEnumFonts */
    X11DRV_EnumICMProfiles,             /* pEnumICMProfiles */
    NULL,                               /* pExcludeClipRect */
    NULL,                               /* pExtDeviceMode */
    X11DRV_ExtEscape,                   /* pExtEscape */
    X11DRV_ExtFloodFill,                /* pExtFloodFill */
    NULL,                               /* pExtSelectClipRgn */
    NULL,                               /* pExtTextOut */
    X11DRV_FillPath,                    /* pFillPath */
    NULL,                               /* pFillRgn */
    NULL,                               /* pFlattenPath */
    NULL,                               /* pFontIsLinked */
    NULL,                               /* pFrameRgn */
    NULL,                               /* pGdiComment */
    NULL,                               /* pGetBoundsRect */
    NULL,                               /* pGetCharABCWidths */
    NULL,                               /* pGetCharABCWidthsI */
    NULL,                               /* pGetCharWidth */
    NULL,                               /* pGetCharWidthInfo */
    X11DRV_GetDeviceCaps,               /* pGetDeviceCaps */
    X11DRV_GetDeviceGammaRamp,          /* pGetDeviceGammaRamp */
    NULL,                               /* pGetFontData */
    NULL,                               /* pGetFontRealizationInfo */
    NULL,                               /* pGetFontUnicodeRanges */
    NULL,                               /* pGetGlyphIndices */
    NULL,                               /* pGetGlyphOutline */
    X11DRV_GetICMProfile,               /* pGetICMProfile */
    X11DRV_GetImage,                    /* pGetImage */
    NULL,                               /* pGetKerningPairs */
    X11DRV_GetNearestColor,             /* pGetNearestColor */
    NULL,                               /* pGetOutlineTextMetrics */
    NULL,                               /* pGetPixel */
    X11DRV_GetSystemPaletteEntries,     /* pGetSystemPaletteEntries */
    NULL,                               /* pGetTextCharsetInfo */
    NULL,                               /* pGetTextExtentExPoint */
    NULL,                               /* pGetTextExtentExPointI */
    NULL,                               /* pGetTextFace */
    NULL,                               /* pGetTextMetrics */
    X11DRV_GradientFill,                /* pGradientFill */
    NULL,                               /* pIntersectClipRect */
    NULL,                               /* pInvertRgn */
    X11DRV_LineTo,                      /* pLineTo */
    NULL,                               /* pModifyWorldTransform */
    NULL,                               /* pMoveTo */
    NULL,                               /* pOffsetClipRgn */
    NULL,                               /* pOffsetViewportOrg */
    NULL,                               /* pOffsetWindowOrg */
    X11DRV_PaintRgn,                    /* pPaintRgn */
    X11DRV_PatBlt,                      /* pPatBlt */
    X11DRV_Pie,                         /* pPie */
    NULL,                               /* pPolyBezier */
    NULL,                               /* pPolyBezierTo */
    NULL,                               /* pPolyDraw */
    X11DRV_PolyPolygon,                 /* pPolyPolygon */
    X11DRV_PolyPolyline,                /* pPolyPolyline */
    X11DRV_Polygon,                     /* pPolygon */
    NULL,                               /* pPolyline */
    NULL,                               /* pPolylineTo */
    X11DRV_PutImage,                    /* pPutImage */
    X11DRV_RealizeDefaultPalette,       /* pRealizeDefaultPalette */
    X11DRV_RealizePalette,              /* pRealizePalette */
    X11DRV_Rectangle,                   /* pRectangle */
    NULL,                               /* pResetDC */
    NULL,                               /* pRestoreDC */
    X11DRV_RoundRect,                   /* pRoundRect */
    NULL,                               /* pSaveDC */
    NULL,                               /* pScaleViewportExt */
    NULL,                               /* pScaleWindowExt */
    NULL,                               /* pSelectBitmap */
    X11DRV_SelectBrush,                 /* pSelectBrush */
    NULL,                               /* pSelectClipPath */
    X11DRV_SelectFont,                  /* pSelectFont */
    NULL,                               /* pSelectPalette */
    X11DRV_SelectPen,                   /* pSelectPen */
    NULL,                               /* pSetArcDirection */
    NULL,                               /* pSetBkColor */
    NULL,                               /* pSetBkMode */
    X11DRV_SetBoundsRect,               /* pSetBoundsRect */
    X11DRV_SetDCBrushColor,             /* pSetDCBrushColor */
    X11DRV_SetDCPenColor,               /* pSetDCPenColor */
    NULL,                               /* pSetDIBitsToDevice */
    X11DRV_SetDeviceClipping,           /* pSetDeviceClipping */
    X11DRV_SetDeviceGammaRamp,          /* pSetDeviceGammaRamp */
    NULL,                               /* pSetLayout */
    NULL,                               /* pSetMapMode */
    NULL,                               /* pSetMapperFlags */
    X11DRV_SetPixel,                    /* pSetPixel */
    NULL,                               /* pSetPolyFillMode */
    NULL,                               /* pSetROP2 */
    NULL,                               /* pSetRelAbs */
    NULL,                               /* pSetStretchBltMode */
    NULL,                               /* pSetTextAlign */
    NULL,                               /* pSetTextCharacterExtra */
    NULL,                               /* pSetTextColor */
    NULL,                               /* pSetTextJustification */
    NULL,                               /* pSetViewportExt */
    NULL,                               /* pSetViewportOrg */
    NULL,                               /* pSetWindowExt */
    NULL,                               /* pSetWindowOrg */
    NULL,                               /* pSetWorldTransform */
    NULL,                               /* pStartDoc */
    NULL,                               /* pStartPage */
    X11DRV_StretchBlt,                  /* pStretchBlt */
    NULL,                               /* pStretchDIBits */
    X11DRV_StrokeAndFillPath,           /* pStrokeAndFillPath */
    X11DRV_StrokePath,                  /* pStrokePath */
    X11DRV_UnrealizePalette,            /* pUnrealizePalette */
    NULL,                               /* pWidenPath */
    X11DRV_D3DKMTCheckVidPnExclusiveOwnership, /* pD3DKMTCheckVidPnExclusiveOwnership */
    X11DRV_D3DKMTSetVidPnSourceOwner,   /* pD3DKMTSetVidPnSourceOwner */
    X11DRV_wine_get_wgl_driver,         /* wine_get_wgl_driver */
    X11DRV_wine_get_vulkan_driver,      /* wine_get_vulkan_driver */
    GDI_PRIORITY_GRAPHICS_DRV           /* priority */
};


/******************************************************************************
 *      X11DRV_get_gdi_driver
 */
const struct gdi_dc_funcs * CDECL X11DRV_get_gdi_driver( unsigned int version )
{
    if (version != WINE_GDI_DRIVER_VERSION)
    {
        ERR( "version mismatch, gdi32 wants %u but winex11 has %u\n", version, WINE_GDI_DRIVER_VERSION );
        return NULL;
    }
    return &x11drv_funcs;
}
