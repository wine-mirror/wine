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
                               LPCSTR output, const DEVMODE16* initData );
static BOOL32 X11DRV_DeleteDC( DC *dc );

static const DC_FUNCTIONS X11DRV_Funcs =
{
    X11DRV_Arc,                      /* pArc */
    X11DRV_BitBlt,                   /* pBitBlt */
    X11DRV_Chord,                    /* pChord */
    X11DRV_CreateDC,                 /* pCreateDC */
    X11DRV_DeleteDC,                 /* pDeleteDC */
    NULL,                            /* pDeleteObject */
    X11DRV_Ellipse,                  /* pEllipse */
    NULL,                            /* pEscape */
    NULL,                            /* pExcludeClipRect */
    NULL,                            /* pExcludeVisRect */
    X11DRV_ExtFloodFill,             /* pExtFloodFill */
    X11DRV_ExtTextOut,               /* pExtTextOut */
    X11DRV_GetPixel,                 /* pGetPixel */
    X11DRV_GetTextExtentPoint,       /* pGetTextExtentPoint */
    X11DRV_GetTextMetrics,           /* pGetTextMetrics */
    NULL,                            /* pIntersectClipRect */
    NULL,                            /* pIntersectVisRect */
    X11DRV_LineTo,                   /* pLineTo */
    X11DRV_MoveToEx,                 /* pMoveToEx */
    NULL,                            /* pOffsetClipRgn */
    NULL,                            /* pOffsetViewportOrg (optional) */
    NULL,                            /* pOffsetWindowOrg (optional) */
    X11DRV_PaintRgn,                 /* pPaintRgn */
    X11DRV_PatBlt,                   /* pPatBlt */
    X11DRV_Pie,                      /* pPie */
    X11DRV_PolyPolygon,              /* pPolyPolygon */
    X11DRV_Polygon,                  /* pPolygon */
    X11DRV_Polyline,                 /* pPolyline */
    NULL,                            /* pRealizePalette */
    X11DRV_Rectangle,                /* pRectangle */
    NULL,                            /* pRestoreDC */
    X11DRV_RoundRect,                /* pRoundRect */
    NULL,                            /* pSaveDC */
    NULL,                            /* pScaleViewportExt (optional) */
    NULL,                            /* pScaleWindowExt (optional) */
    NULL,                            /* pSelectClipRgn */
    X11DRV_SelectObject,             /* pSelectObject */
    NULL,                            /* pSelectPalette */
    NULL,                            /* pSetBkColor */
    NULL,                            /* pSetBkMode */
    X11DRV_SetDeviceClipping,        /* pSetDeviceClipping */
    NULL,                            /* pSetDIBitsToDevice */
    NULL,                            /* pSetMapMode (optional) */
    NULL,                            /* pSetMapperFlags */
    X11DRV_SetPixel,                 /* pSetPixel */
    NULL,                            /* pSetPolyFillMode */
    NULL,                            /* pSetROP2 */
    NULL,                            /* pSetRelAbs */
    NULL,                            /* pSetStretchBltMode */
    NULL,                            /* pSetTextAlign */
    NULL,                            /* pSetTextCharacterExtra */
    NULL,                            /* pSetTextColor */
    NULL,                            /* pSetTextJustification */
    NULL,                            /* pSetViewportExt (optional) */
    NULL,                            /* pSetViewportOrg (optional) */
    NULL,                            /* pSetWindowExt (optional) */
    NULL,                            /* pSetWindowOrg (optional) */
    X11DRV_StretchBlt,               /* pStretchBlt */
    NULL                             /* pStretchDIBits */
};

static DeviceCaps X11DRV_DevCaps;

/**********************************************************************
 *	     X11DRV_Init
 */
BOOL32 X11DRV_Init(void)
{
    /* Create default bitmap */

    if (!X11DRV_BITMAP_Init()) return FALSE;

    /* Initialize brush dithering */

    if (!X11DRV_BRUSH_Init()) return FALSE;

    /* Initialize fonts */

    if (!X11DRV_FONT_Init()) return FALSE;

    return DRIVER_RegisterDriver( "DISPLAY", &X11DRV_Funcs );
}


/**********************************************************************
 *	     X11DRV_CreateDC
 */
static BOOL32 X11DRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device,
                               LPCSTR output, const DEVMODE16* initData )
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
