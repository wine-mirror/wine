/*
 * X11 graphics driver initialisation functions
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <string.h>
#include "x11drv.h"
#include "bitmap.h"
#include "gdi.h"


static BOOL32 X11DRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device,
                               LPCSTR output, const DEVMODE* initData );
static BOOL32 X11DRV_DeleteDC( DC *dc );

static const DC_FUNCTIONS X11DRV_Funcs =
{
    NULL,                            /* pArc */
    X11DRV_BitBlt,                   /* pBitBlt */
    NULL,                            /* pChord */
    X11DRV_CreateDC,                 /* pCreateDC */
    X11DRV_DeleteDC,                 /* pDeleteDC */
    NULL,                            /* pDeleteObject */
    NULL,                            /* pEllipse */
    NULL,                            /* pEscape */
    NULL,                            /* pExcludeClipRect */
    NULL,                            /* pExcludeVisRect */
    NULL,                            /* pExtFloodFill */
    NULL,                            /* pExtTextOut */
    NULL,                            /* pFillRgn */
    NULL,                            /* pFloodFill */
    NULL,                            /* pFrameRgn */
    X11DRV_GetTextExtentPoint,       /* pGetTextExtentPoint */
    NULL,                            /* pGetTextMetrics */
    NULL,                            /* pIntersectClipRect */
    NULL,                            /* pIntersectVisRect */
    NULL,                            /* pInvertRgn */
    NULL,                            /* pLineTo */
    NULL,                            /* pMoveToEx */
    NULL,                            /* pOffsetClipRgn */
    NULL,                            /* pOffsetViewportOrgEx */
    NULL,                            /* pOffsetWindowOrgEx */
    NULL,                            /* pPaintRgn */
    X11DRV_PatBlt,                   /* pPatBlt */
    NULL,                            /* pPie */
    NULL,                            /* pPolyPolygon */
    NULL,                            /* pPolygon */
    NULL,                            /* pPolyline */
    NULL,                            /* pRealizePalette */
    NULL,                            /* pRectangle */
    NULL,                            /* pRestoreDC */
    NULL,                            /* pRoundRect */
    NULL,                            /* pSaveDC */
    NULL,                            /* pScaleViewportExtEx */
    NULL,                            /* pScaleWindowExtEx */
    NULL,                            /* pSelectClipRgn */
    NULL,                            /* pSelectObject */
    NULL,                            /* pSelectPalette */
    NULL,                            /* pSetBkColor */
    NULL,                            /* pSetBkMode */
    X11DRV_SetDeviceClipping,        /* pSetDeviceClipping */
    NULL,                            /* pSetDIBitsToDevice */
    NULL,                            /* pSetMapMode */
    NULL,                            /* pSetMapperFlags */
    NULL,                            /* pSetPixel */
    NULL,                            /* pSetPolyFillMode */
    NULL,                            /* pSetROP2 */
    NULL,                            /* pSetRelAbs */
    NULL,                            /* pSetStretchBltMode */
    NULL,                            /* pSetTextAlign */
    NULL,                            /* pSetTextCharacterExtra */
    NULL,                            /* pSetTextColor */
    NULL,                            /* pSetTextJustification */
    NULL,                            /* pSetViewportExtEx */
    NULL,                            /* pSetViewportOrgEx */
    NULL,                            /* pSetWindowExtEx */
    NULL,                            /* pSetWindowOrgEx */
    X11DRV_StretchBlt,               /* pStretchBlt */
    NULL,                            /* pStretchDIBits */
    NULL                             /* pTextOut */
};

static DeviceCaps X11DRV_DevCaps;

/**********************************************************************
 *	     X11DRV_Init
 */
BOOL32 X11DRV_Init(void)
{
    return DRIVER_RegisterDriver( "DISPLAY", &X11DRV_Funcs );
}


/**********************************************************************
 *	     X11DRV_CreateDC
 */
static BOOL32 X11DRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device,
                               LPCSTR output, const DEVMODE* initData )
{
    X11DRV_PDEVICE *physDev;

    if (!X11DRV_DevCaps.version) DC_FillDevCaps( &X11DRV_DevCaps );

    physDev = &dc->u.x;  /* for now */

    memset( physDev, 0, sizeof(*physDev) );
    dc->physDev        = physDev;
    dc->w.devCaps      = &X11DRV_DevCaps;
    if (dc->w.flags & DC_MEMORY)
    {
        BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( dc->w.hBitmap,
                                                      BITMAP_MAGIC );
        physDev->drawable  = bmp->pixmap;
        physDev->gc        = XCreateGC( display, physDev->drawable, 0, NULL );
        dc->w.bitsPerPixel = bmp->bitmap.bmBitsPixel;
        dc->w.hVisRgn      = CreateRectRgn32( 0, 0, bmp->bitmap.bmWidth,
                                              bmp->bitmap.bmHeight );
    }
    else
    {
        physDev->drawable  = rootWindow;
        physDev->gc        = XCreateGC( display, physDev->drawable, 0, NULL );
        dc->w.bitsPerPixel = screenDepth;
        dc->w.hVisRgn      = CreateRectRgn32( 0, 0, screenWidth, screenHeight);
    }

    if (!dc->w.hVisRgn)
    {
        XFreeGC( display, physDev->gc );
        return FALSE;
    }

    XSetGraphicsExposures( display, physDev->gc, False );
    XSetSubwindowMode( display, physDev->gc, IncludeInferiors );

    return TRUE;
}


/**********************************************************************
 *	     X11DRV_DeleteDC
 */
static BOOL32 X11DRV_DeleteDC( DC *dc )
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    XFreeGC( display, physDev->gc );
    return TRUE;
}
