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

static CRITICAL_SECTION device_data_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &device_data_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": device_data_section") }
};
static CRITICAL_SECTION device_data_section = { &critsect_debug, -1, 0, 0, 0, 0 };


static const struct gdi_dc_funcs macdrv_funcs;

/***********************************************************************
 *              compute_desktop_rect
 */
static void compute_desktop_rect(void)
{
    CGDirectDisplayID displayIDs[32];
    uint32_t count, i;

    desktop_rect = CGRectNull;
    if (CGGetActiveDisplayList(ARRAY_SIZE(displayIDs), displayIDs, &count) != kCGErrorSuccess ||
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

    EnterCriticalSection(&device_data_section);

    if (!device_data_valid)
    {
        check_retina_status();
        compute_desktop_rect();
    }
    ret = desktop_rect;

    LeaveCriticalSection(&device_data_section);

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
    EnterCriticalSection(&device_data_section);
    device_data_valid = FALSE;
    LeaveCriticalSection(&device_data_section);
}


static MACDRV_PDEVICE *create_mac_physdev(void)
{
    MACDRV_PDEVICE *physDev;

    EnterCriticalSection(&device_data_section);
    if (!device_data_valid) device_init();
    LeaveCriticalSection(&device_data_section);

    if (!(physDev = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*physDev)))) return NULL;

    return physDev;
}


/**********************************************************************
 *              CreateDC (MACDRV.@)
 */
static BOOL CDECL macdrv_CreateDC(PHYSDEV *pdev, LPCWSTR driver, LPCWSTR device,
                                  LPCWSTR output, const DEVMODEW* initData)
{
    MACDRV_PDEVICE *physDev = create_mac_physdev();

    TRACE("pdev %p hdc %p driver %s device %s output %s initData %p\n", pdev,
          (*pdev)->hdc, debugstr_w(driver),debugstr_w(device), debugstr_w(output),
          initData);

    if (!physDev) return FALSE;

    push_dc_driver(pdev, &physDev->dev, &macdrv_funcs);
    return TRUE;
}


/**********************************************************************
 *              CreateCompatibleDC (MACDRV.@)
 */
static BOOL CDECL macdrv_CreateCompatibleDC(PHYSDEV orig, PHYSDEV *pdev)
{
    MACDRV_PDEVICE *physDev = create_mac_physdev();

    TRACE("orig %p orig->hdc %p pdev %p pdev->hdc %p\n", orig, (orig ? orig->hdc : NULL), pdev,
          ((pdev && *pdev) ? (*pdev)->hdc : NULL));

    if (!physDev) return FALSE;

    push_dc_driver(pdev, &physDev->dev, &macdrv_funcs);
    return TRUE;
}


/**********************************************************************
 *              DeleteDC (MACDRV.@)
 */
static BOOL CDECL macdrv_DeleteDC(PHYSDEV dev)
{
    MACDRV_PDEVICE *physDev = get_macdrv_dev(dev);

    TRACE("hdc %p\n", dev->hdc);

    HeapFree(GetProcessHeap(), 0, physDev);
    return TRUE;
}


/***********************************************************************
 *              GetDeviceCaps (MACDRV.@)
 */
static INT CDECL macdrv_GetDeviceCaps(PHYSDEV dev, INT cap)
{
    INT ret;

    EnterCriticalSection(&device_data_section);

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
        LeaveCriticalSection(&device_data_section);
        dev = GET_NEXT_PHYSDEV( dev, pGetDeviceCaps );
        ret = dev->funcs->pGetDeviceCaps( dev, cap );
        if ((cap == HORZRES || cap == VERTRES) && retina_on)
            ret *= 2;
        return ret;
    }

    TRACE("cap %d -> %d\n", cap, ret);

    LeaveCriticalSection(&device_data_section);
    return ret;
}


static const struct gdi_dc_funcs macdrv_funcs =
{
    NULL,                                   /* pAbortDoc */
    NULL,                                   /* pAbortPath */
    NULL,                                   /* pAlphaBlend */
    NULL,                                   /* pAngleArc */
    NULL,                                   /* pArc */
    NULL,                                   /* pArcTo */
    NULL,                                   /* pBeginPath */
    NULL,                                   /* pBlendImage */
    NULL,                                   /* pChord */
    NULL,                                   /* pCloseFigure */
    macdrv_CreateCompatibleDC,              /* pCreateCompatibleDC */
    macdrv_CreateDC,                        /* pCreateDC */
    macdrv_DeleteDC,                        /* pDeleteDC */
    NULL,                                   /* pDeleteObject */
    NULL,                                   /* pDeviceCapabilities */
    NULL,                                   /* pEllipse */
    NULL,                                   /* pEndDoc */
    NULL,                                   /* pEndPage */
    NULL,                                   /* pEndPath */
    NULL,                                   /* pEnumFonts */
    NULL,                                   /* pEnumICMProfiles */
    NULL,                                   /* pExcludeClipRect */
    NULL,                                   /* pExtDeviceMode */
    NULL,                                   /* pExtEscape */
    NULL,                                   /* pExtFloodFill */
    NULL,                                   /* pExtSelectClipRgn */
    NULL,                                   /* pExtTextOut */
    NULL,                                   /* pFillPath */
    NULL,                                   /* pFillRgn */
    NULL,                                   /* pFlattenPath */
    NULL,                                   /* pFontIsLinked */
    NULL,                                   /* pFrameRgn */
    NULL,                                   /* pGdiComment */
    NULL,                                   /* pGetBoundsRect */
    NULL,                                   /* pGetCharABCWidths */
    NULL,                                   /* pGetCharABCWidthsI */
    NULL,                                   /* pGetCharWidth */
    NULL,                                   /* pGetCharWidthInfo */
    macdrv_GetDeviceCaps,                   /* pGetDeviceCaps */
    macdrv_GetDeviceGammaRamp,              /* pGetDeviceGammaRamp */
    NULL,                                   /* pGetFontData */
    NULL,                                   /* pGetFontRealizationInfo */
    NULL,                                   /* pGetFontUnicodeRanges */
    NULL,                                   /* pGetGlyphIndices */
    NULL,                                   /* pGetGlyphOutline */
    NULL,                                   /* pGetICMProfile */
    NULL,                                   /* pGetImage */
    NULL,                                   /* pGetKerningPairs */
    NULL,                                   /* pGetNearestColor */
    NULL,                                   /* pGetOutlineTextMetrics */
    NULL,                                   /* pGetPixel */
    NULL,                                   /* pGetSystemPaletteEntries */
    NULL,                                   /* pGetTextCharsetInfo */
    NULL,                                   /* pGetTextExtentExPoint */
    NULL,                                   /* pGetTextExtentExPointI */
    NULL,                                   /* pGetTextFace */
    NULL,                                   /* pGetTextMetrics */
    NULL,                                   /* pGradientFill */
    NULL,                                   /* pIntersectClipRect */
    NULL,                                   /* pInvertRgn */
    NULL,                                   /* pLineTo */
    NULL,                                   /* pModifyWorldTransform */
    NULL,                                   /* pMoveTo */
    NULL,                                   /* pOffsetClipRgn */
    NULL,                                   /* pOffsetViewportOrg */
    NULL,                                   /* pOffsetWindowOrg */
    NULL,                                   /* pPaintRgn */
    NULL,                                   /* pPatBlt */
    NULL,                                   /* pPie */
    NULL,                                   /* pPolyBezier */
    NULL,                                   /* pPolyBezierTo */
    NULL,                                   /* pPolyDraw */
    NULL,                                   /* pPolyPolygon */
    NULL,                                   /* pPolyPolyline */
    NULL,                                   /* pPolygon */
    NULL,                                   /* pPolyline */
    NULL,                                   /* pPolylineTo */
    NULL,                                   /* pPutImage */
    NULL,                                   /* pRealizeDefaultPalette */
    NULL,                                   /* pRealizePalette */
    NULL,                                   /* pRectangle */
    NULL,                                   /* pResetDC */
    NULL,                                   /* pRestoreDC */
    NULL,                                   /* pRoundRect */
    NULL,                                   /* pSaveDC */
    NULL,                                   /* pScaleViewportExt */
    NULL,                                   /* pScaleWindowExt */
    NULL,                                   /* pSelectBitmap */
    NULL,                                   /* pSelectBrush */
    NULL,                                   /* pSelectClipPath */
    NULL,                                   /* pSelectFont */
    NULL,                                   /* pSelectPalette */
    NULL,                                   /* pSelectPen */
    NULL,                                   /* pSetArcDirection */
    NULL,                                   /* pSetBkColor */
    NULL,                                   /* pSetBkMode */
    NULL,                                   /* pSetBoundsRect */
    NULL,                                   /* pSetDCBrushColor */
    NULL,                                   /* pSetDCPenColor */
    NULL,                                   /* pSetDIBitsToDevice */
    NULL,                                   /* pSetDeviceClipping */
    macdrv_SetDeviceGammaRamp,              /* pSetDeviceGammaRamp */
    NULL,                                   /* pSetLayout */
    NULL,                                   /* pSetMapMode */
    NULL,                                   /* pSetMapperFlags */
    NULL,                                   /* pSetPixel */
    NULL,                                   /* pSetPolyFillMode */
    NULL,                                   /* pSetROP2 */
    NULL,                                   /* pSetRelAbs */
    NULL,                                   /* pSetStretchBltMode */
    NULL,                                   /* pSetTextAlign */
    NULL,                                   /* pSetTextCharacterExtra */
    NULL,                                   /* pSetTextColor */
    NULL,                                   /* pSetTextJustification */
    NULL,                                   /* pSetViewportExt */
    NULL,                                   /* pSetViewportOrg */
    NULL,                                   /* pSetWindowExt */
    NULL,                                   /* pSetWindowOrg */
    NULL,                                   /* pSetWorldTransform */
    NULL,                                   /* pStartDoc */
    NULL,                                   /* pStartPage */
    NULL,                                   /* pStretchBlt */
    NULL,                                   /* pStretchDIBits */
    NULL,                                   /* pStrokeAndFillPath */
    NULL,                                   /* pStrokePath */
    NULL,                                   /* pUnrealizePalette */
    NULL,                                   /* pWidenPath */
    macdrv_wine_get_wgl_driver,             /* wine_get_wgl_driver */
    macdrv_wine_get_vulkan_driver,          /* wine_get_vulkan_driver */
    GDI_PRIORITY_GRAPHICS_DRV               /* priority */
};


/******************************************************************************
 *              macdrv_get_gdi_driver
 */
const struct gdi_dc_funcs * CDECL macdrv_get_gdi_driver(unsigned int version)
{
    if (version != WINE_GDI_DRIVER_VERSION)
    {
        ERR("version mismatch, gdi32 wants %u but winemac has %u\n", version, WINE_GDI_DRIVER_VERSION);
        return NULL;
    }
    return &macdrv_funcs;
}
