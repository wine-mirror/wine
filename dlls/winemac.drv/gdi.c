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
static int log_pixels_x;        /* pixels per logical inch in x direction */
static int log_pixels_y;        /* pixels per logical inch in y direction */
static int horz_size;           /* horz. size of screen in millimeters */
static int vert_size;           /* vert. size of screen in millimeters */
static int horz_res;            /* width in pixels of screen */
static int vert_res;            /* height in pixels of screen */
static int desktop_horz_res;    /* width in pixels of virtual desktop */
static int desktop_vert_res;    /* height in pixels of virtual desktop */
static int bits_per_pixel;      /* pixel depth of screen */
static int device_data_valid;   /* do the above variables have up-to-date values? */

static CRITICAL_SECTION device_data_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &device_data_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": device_data_section") }
};
static CRITICAL_SECTION device_data_section = { &critsect_debug, -1, 0, 0, 0, 0 };


static const WCHAR dpi_key_name[] = {'S','o','f','t','w','a','r','e','\\','F','o','n','t','s','\0'};
static const WCHAR dpi_value_name[] = {'L','o','g','P','i','x','e','l','s','\0'};

static const struct gdi_dc_funcs macdrv_funcs;


/******************************************************************************
 *              get_dpi
 *
 * get the dpi from the registry
 */
static DWORD get_dpi(void)
{
    DWORD dpi = 0;
    HKEY hkey;

    if (RegOpenKeyW(HKEY_CURRENT_CONFIG, dpi_key_name, &hkey) == ERROR_SUCCESS)
    {
        DWORD type, size, new_dpi;

        size = sizeof(new_dpi);
        if (RegQueryValueExW(hkey, dpi_value_name, NULL, &type, (void *)&new_dpi, &size) == ERROR_SUCCESS)
        {
            if (type == REG_DWORD && new_dpi != 0)
                dpi = new_dpi;
        }
        RegCloseKey(hkey);
    }
    return dpi;
}


/***********************************************************************
 *              macdrv_get_desktop_rect
 *
 * Returns the rectangle encompassing all the screens.
 */
CGRect macdrv_get_desktop_rect(void)
{
    CGRect ret;
    CGDirectDisplayID displayIDs[32];
    uint32_t count, i;

    EnterCriticalSection(&device_data_section);

    if (!device_data_valid)
    {
        desktop_rect = CGRectNull;
        if (CGGetActiveDisplayList(sizeof(displayIDs)/sizeof(displayIDs[0]),
                                   displayIDs, &count) != kCGErrorSuccess ||
            !count)
        {
            displayIDs[0] = CGMainDisplayID();
            count = 1;
        }

        for (i = 0; i < count; i++)
            desktop_rect = CGRectUnion(desktop_rect, CGDisplayBounds(displayIDs[i]));
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

    /* Initialize device caps */
    log_pixels_x = log_pixels_y = get_dpi();
    if (!log_pixels_x)
    {
        size_t width = CGDisplayPixelsWide(mainDisplay);
        size_t height = CGDisplayPixelsHigh(mainDisplay);
        log_pixels_x = MulDiv(width, 254, size_mm.width * 10);
        log_pixels_y = MulDiv(height, 254, size_mm.height * 10);
    }

    horz_size = size_mm.width;
    vert_size = size_mm.height;

    bits_per_pixel = 32;
    if (mode)
    {
        CFStringRef pixelEncoding = CGDisplayModeCopyPixelEncoding(mode);

        horz_res = CGDisplayModeGetWidth(mode);
        vert_res = CGDisplayModeGetHeight(mode);

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
    else
    {
        horz_res = CGDisplayPixelsWide(mainDisplay);
        vert_res = CGDisplayPixelsHigh(mainDisplay);
    }

    macdrv_get_desktop_rect();
    desktop_horz_res = desktop_rect.size.width;
    desktop_vert_res = desktop_rect.size.height;

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
static BOOL macdrv_CreateDC(PHYSDEV *pdev, LPCWSTR driver, LPCWSTR device,
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
static BOOL macdrv_CreateCompatibleDC(PHYSDEV orig, PHYSDEV *pdev)
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
static BOOL macdrv_DeleteDC(PHYSDEV dev)
{
    MACDRV_PDEVICE *physDev = get_macdrv_dev(dev);

    TRACE("hdc %p\n", dev->hdc);

    HeapFree(GetProcessHeap(), 0, physDev);
    return TRUE;
}


/***********************************************************************
 *              GetDeviceCaps (MACDRV.@)
 */
static INT macdrv_GetDeviceCaps(PHYSDEV dev, INT cap)
{
    INT ret;

    EnterCriticalSection(&device_data_section);

    if (!device_data_valid) device_init();

    switch(cap)
    {
    case DRIVERVERSION:
        ret = 0x300;
        break;
    case TECHNOLOGY:
        ret = DT_RASDISPLAY;
        break;
    case HORZSIZE:
        ret = horz_size;
        break;
    case VERTSIZE:
        ret = vert_size;
        break;
    case HORZRES:
        ret = horz_res;
        break;
    case VERTRES:
        ret = vert_res;
        break;
    case DESKTOPHORZRES:
        ret = desktop_horz_res;
        break;
    case DESKTOPVERTRES:
        ret = desktop_vert_res;
        break;
    case BITSPIXEL:
        ret = bits_per_pixel;
        break;
    case PLANES:
        ret = 1;
        break;
    case NUMBRUSHES:
        ret = -1;
        break;
    case NUMPENS:
        ret = -1;
        break;
    case NUMMARKERS:
        ret = 0;
        break;
    case NUMFONTS:
        ret = 0;
        break;
    case NUMCOLORS:
        /* MSDN: Number of entries in the device's color table, if the device has
         * a color depth of no more than 8 bits per pixel.For devices with greater
         * color depths, -1 is returned. */
        ret = (bits_per_pixel > 8) ? -1 : (1 << bits_per_pixel);
        break;
    case PDEVICESIZE:
        ret = sizeof(MACDRV_PDEVICE);
        break;
    case CURVECAPS:
        ret = (CC_CIRCLES | CC_PIE | CC_CHORD | CC_ELLIPSES | CC_WIDE |
               CC_STYLED | CC_WIDESTYLED | CC_INTERIORS | CC_ROUNDRECT);
        break;
    case LINECAPS:
        ret = (LC_POLYLINE | LC_MARKER | LC_POLYMARKER | LC_WIDE |
               LC_STYLED | LC_WIDESTYLED | LC_INTERIORS);
        break;
    case POLYGONALCAPS:
        ret = (PC_POLYGON | PC_RECTANGLE | PC_WINDPOLYGON | PC_SCANLINE |
               PC_WIDE | PC_STYLED | PC_WIDESTYLED | PC_INTERIORS);
        break;
    case TEXTCAPS:
        ret = (TC_OP_CHARACTER | TC_OP_STROKE | TC_CP_STROKE |
               TC_CR_ANY | TC_SF_X_YINDEP | TC_SA_DOUBLE | TC_SA_INTEGER |
               TC_SA_CONTIN | TC_UA_ABLE | TC_SO_ABLE | TC_RA_ABLE | TC_VA_ABLE);
        break;
    case CLIPCAPS:
        ret = CP_REGION;
        break;
    case COLORRES:
        /* The observed correspondence between BITSPIXEL and COLORRES is:
         * BITSPIXEL: 8  -> COLORRES: 18
         * BITSPIXEL: 16 -> COLORRES: 16
         * BITSPIXEL: 24 -> COLORRES: 24
         * (note that bits_per_pixel is never 24)
         * BITSPIXEL: 32 -> COLORRES: 24 */
        ret = (bits_per_pixel <= 8) ? 18 : (bits_per_pixel == 32) ? 24 : bits_per_pixel;
        break;
    case RASTERCAPS:
        ret = (RC_BITBLT | RC_BANDING | RC_SCALING | RC_BITMAP64 | RC_DI_BITMAP |
               RC_DIBTODEV | RC_BIGFONT | RC_STRETCHBLT | RC_STRETCHDIB | RC_DEVBITS |
               (bits_per_pixel <= 8 ? RC_PALETTE : 0));
        break;
    case SHADEBLENDCAPS:
        ret = (SB_GRAD_RECT | SB_GRAD_TRI | SB_CONST_ALPHA | SB_PIXEL_ALPHA);
        break;
    case ASPECTX:
    case ASPECTY:
        ret = 36;
        break;
    case ASPECTXY:
        ret = 51;
        break;
    case LOGPIXELSX:
        ret = log_pixels_x;
        break;
    case LOGPIXELSY:
        ret = log_pixels_y;
        break;
    case CAPS1:
        FIXME("(%p): CAPS1 is unimplemented, will return 0\n", dev->hdc);
        /* please see wingdi.h for the possible bit-flag values that need
           to be returned. */
        ret = 0;
        break;
    case SIZEPALETTE:
        ret = bits_per_pixel <= 8 ? 1 << bits_per_pixel : 0;
        break;
    case NUMRESERVED:
    case PHYSICALWIDTH:
    case PHYSICALHEIGHT:
    case PHYSICALOFFSETX:
    case PHYSICALOFFSETY:
    case SCALINGFACTORX:
    case SCALINGFACTORY:
    case VREFRESH:
    case BLTALIGNMENT:
        ret = 0;
        break;
    default:
        FIXME("(%p): unsupported capability %d, will return 0\n", dev->hdc, cap);
        ret = 0;
        goto done;
    }

    TRACE("cap %d -> %d\n", cap, ret);

done:
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
    NULL,                                   /* pGdiRealizationInfo */
    NULL,                                   /* pGetBoundsRect */
    NULL,                                   /* pGetCharABCWidths */
    NULL,                                   /* pGetCharABCWidthsI */
    NULL,                                   /* pGetCharWidth */
    macdrv_GetDeviceCaps,                   /* pGetDeviceCaps */
    macdrv_GetDeviceGammaRamp,              /* pGetDeviceGammaRamp */
    NULL,                                   /* pGetFontData */
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
