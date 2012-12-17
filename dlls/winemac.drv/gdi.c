/*
 * Mac graphics driver initialisation functions
 *
 * Copyright 1996 Alexandre Julliard
 * Copyright 2011, 2012 Ken Thomases for CodeWeavers, Inc.
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "wine/gdi_driver.h"
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


static const struct gdi_dc_funcs macdrv_funcs;


static MACDRV_PDEVICE *create_mac_physdev(void)
{
    MACDRV_PDEVICE *physDev;

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
    NULL,                                   /* pGetDeviceCaps */
    NULL,                                   /* pGetDeviceGammaRamp */
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
    NULL,                                   /* pSetDeviceGammaRamp */
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
    NULL,                                   /* wine_get_wgl_driver */
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
