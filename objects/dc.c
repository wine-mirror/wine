/*
 * GDI Device Context functions
 *
 * Copyright 1993 Alexandre Julliard
 *
 */

#include <stdlib.h>
#include <string.h>
#include "gdi.h"
#include "bitmap.h"
#include "metafile.h"
#include "stddebug.h"
#include "color.h"
#include "debug.h"
#include "font.h"

static DeviceCaps * displayDevCaps = NULL;

extern void CLIPPING_UpdateGCRegion( DC * dc );     /* objects/clipping.c */

  /* Default DC values */
static const WIN_DC_INFO DC_defaultValues =
{
    0,                      /* flags */
    NULL,                   /* devCaps */
    0,                      /* hMetaFile */
    0,                      /* hClipRgn */
    0,                      /* hVisRgn */
    0,                      /* hGCClipRgn */
    STOCK_BLACK_PEN,        /* hPen */
    STOCK_WHITE_BRUSH,      /* hBrush */
    STOCK_SYSTEM_FONT,      /* hFont */
    0,                      /* hBitmap */
    0,                      /* hFirstBitmap */
    0,                      /* hDevice */
    STOCK_DEFAULT_PALETTE,  /* hPalette */
    R2_COPYPEN,             /* ROPmode */
    ALTERNATE,              /* polyFillMode */
    BLACKONWHITE,           /* stretchBltMode */
    ABSOLUTE,               /* relAbsMode */
    OPAQUE,                 /* backgroundMode */
    RGB( 255, 255, 255 ),   /* backgroundColor */
    RGB( 0, 0, 0 ),         /* textColor */
    0,                      /* backgroundPixel */
    0,                      /* textPixel */
    0,                      /* brushOrgX */
    0,                      /* brushOrgY */
    TA_LEFT | TA_TOP | TA_NOUPDATECP,  /* textAlign */
    0,                      /* charExtra */
    0,                      /* breakTotalExtra */
    0,                      /* breakCount */
    0,                      /* breakExtra */
    0,                      /* breakRem */
    1,                      /* bitsPerPixel */
    MM_TEXT,                /* MapMode */
    0,                      /* DCOrgX */
    0,                      /* DCOrgY */
    0,                      /* CursPosX */
    0,                      /* CursPosY */
    0,                      /* WndOrgX */
    0,                      /* WndOrgY */
    1,                      /* WndExtX */
    1,                      /* WndExtY */
    0,                      /* VportOrgX */
    0,                      /* VportOrgY */
    1,                      /* VportExtX */
    1                       /* VportExtY */
};

  /* ROP code to GC function conversion */
const int DC_XROPfunction[16] =
{
    GXclear,        /* R2_BLACK */
    GXnor,          /* R2_NOTMERGEPEN */
    GXandInverted,  /* R2_MASKNOTPEN */
    GXcopyInverted, /* R2_NOTCOPYPEN */
    GXandReverse,   /* R2_MASKPENNOT */
    GXinvert,       /* R2_NOT */
    GXxor,          /* R2_XORPEN */
    GXnand,         /* R2_NOTMASKPEN */
    GXand,          /* R2_MASKPEN */
    GXequiv,        /* R2_NOTXORPEN */
    GXnoop,         /* R2_NOP */
    GXorInverted,   /* R2_MERGENOTPEN */
    GXcopy,         /* R2_COPYPEN */
    GXorReverse,    /* R2_MERGEPENNOT */
    GXor,           /* R2_MERGEPEN */
    GXset           /* R2_WHITE */
};


/***********************************************************************
 *           DC_FillDevCaps
 *
 * Fill the device caps structure.
 */
void DC_FillDevCaps( DeviceCaps * caps )
{
    caps->version       = 0x300; 
    caps->technology    = DT_RASDISPLAY;
    caps->horzSize      = WidthMMOfScreen(screen) * screenWidth / WidthOfScreen(screen);
    caps->vertSize      = HeightMMOfScreen(screen) * screenHeight / HeightOfScreen(screen);
    caps->horzRes       = screenWidth;
    caps->vertRes       = screenHeight;
    caps->bitsPixel     = screenDepth;
    caps->planes        = 1;
    caps->numBrushes    = 0;
    caps->numPens       = 0;
    caps->numMarkers    = 0;
    caps->numFonts      = 0;
    caps->numColors     = 1 << caps->bitsPixel;
    caps->pdeviceSize   = 0;
    caps->curveCaps     = CC_CIRCLES | CC_PIE | CC_CHORD | CC_ELLIPSES |
	                  CC_WIDE | CC_STYLED | CC_WIDESTYLED | 
			  CC_INTERIORS | CC_ROUNDRECT;
    caps->lineCaps      = LC_POLYLINE | LC_MARKER | LC_POLYMARKER | LC_WIDE |
	                  LC_STYLED | LC_WIDESTYLED | LC_INTERIORS;
    caps->polygonalCaps = PC_POLYGON | PC_RECTANGLE | PC_WINDPOLYGON |
	                  PC_SCANLINE | PC_WIDE | PC_STYLED | 
			  PC_WIDESTYLED | PC_INTERIORS;
    caps->textCaps      = TC_OP_CHARACTER | TC_OP_STROKE | TC_CP_STROKE |
	                  TC_IA_ABLE | TC_UA_ABLE | TC_SO_ABLE | TC_RA_ABLE;
    caps->clipCaps      = CP_REGION;
    caps->rasterCaps    = RC_BITBLT | RC_BANDING | RC_SCALING | RC_BITMAP64 |
	                  RC_DI_BITMAP | RC_PALETTE | RC_DIBTODEV | RC_BIGFONT|
			  RC_STRETCHBLT | RC_STRETCHDIB | RC_DEVBITS;
    caps->aspectX       = 36;  /* ?? */
    caps->aspectY       = 36;  /* ?? */
    caps->aspectXY      = 51;
    caps->logPixelsX    = (int)(caps->horzRes * 25.4 / caps->horzSize);
    caps->logPixelsY    = (int)(caps->vertRes * 25.4 / caps->vertSize);
    caps->sizePalette   = DefaultVisual(display,DefaultScreen(display))->map_entries;
    caps->numReserved   = 0;
    caps->colorRes      = 0;
}


/***********************************************************************
 *           DC_InitDC
 *
 * Setup device-specific DC values for a newly created DC.
 */
void DC_InitDC( HDC hdc )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    RealizeDefaultPalette( hdc );
    SetTextColor( hdc, dc->w.textColor );
    SetBkColor( hdc, dc->w.backgroundColor );
    SelectObject( hdc, dc->w.hPen );
    SelectObject( hdc, dc->w.hBrush );
    SelectObject( hdc, dc->w.hFont );
    XSetGraphicsExposures( display, dc->u.x.gc, False );
    XSetSubwindowMode( display, dc->u.x.gc, IncludeInferiors );
    CLIPPING_UpdateGCRegion( dc );
}


/***********************************************************************
 *           DC_SetupDCForPatBlt
 *
 * Setup the GC for a PatBlt operation using current brush.
 * If fMapColors is TRUE, X pixels are mapped to Windows colors.
 * Return FALSE if brush is BS_NULL, TRUE otherwise.
 */
BOOL DC_SetupGCForPatBlt( DC * dc, GC gc, BOOL fMapColors )
{
    XGCValues val;
    unsigned long mask;
    Pixmap pixmap = 0;

    if (dc->u.x.brush.style == BS_NULL) return FALSE;
    if (dc->u.x.brush.pixel == -1)
    {
	/* Special case used for monochrome pattern brushes.
	 * We need to swap foreground and background because
	 * Windows does it the wrong way...
	 */
	val.foreground = dc->w.backgroundPixel;
	val.background = dc->w.textPixel;
    }
    else
    {
	val.foreground = dc->u.x.brush.pixel;
	val.background = dc->w.backgroundPixel;
    }
    if (fMapColors && COLOR_PixelToPalette)
    {
        val.foreground = COLOR_PixelToPalette[val.foreground];
        val.background = COLOR_PixelToPalette[val.background];
    }

    val.function = DC_XROPfunction[dc->w.ROPmode-1];
    val.fill_style = dc->u.x.brush.fillStyle;
    switch(val.fill_style)
    {
    case FillStippled:
    case FillOpaqueStippled:
	if (dc->w.backgroundMode==OPAQUE) val.fill_style = FillOpaqueStippled;
	val.stipple = dc->u.x.brush.pixmap;
	mask = GCStipple;
        break;

    case FillTiled:
        if (fMapColors && COLOR_PixelToPalette)
        {
            register int x, y;
            XImage *image;
            pixmap = XCreatePixmap( display, rootWindow, 8, 8, screenDepth );
            image = XGetImage( display, dc->u.x.brush.pixmap, 0, 0, 8, 8,
                               AllPlanes, ZPixmap );
            for (y = 0; y < 8; y++)
                for (x = 0; x < 8; x++)
                    XPutPixel( image, x, y,
                               COLOR_PixelToPalette[XGetPixel( image, x, y)] );
            XPutImage( display, pixmap, gc, image, 0, 0, 0, 0, 8, 8 );
            XDestroyImage( image );
            val.tile = pixmap;
        }
        else val.tile = dc->u.x.brush.pixmap;
	mask = GCTile;
        break;

    default:
        mask = 0;
        break;
    }
    val.ts_x_origin = dc->w.DCOrgX + dc->w.brushOrgX;
    val.ts_y_origin = dc->w.DCOrgY + dc->w.brushOrgY;
    val.fill_rule = (dc->w.polyFillMode==WINDING) ? WindingRule : EvenOddRule;
    XChangeGC( display, gc, 
	       GCFunction | GCForeground | GCBackground | GCFillStyle |
	       GCFillRule | GCTileStipXOrigin | GCTileStipYOrigin | mask,
	       &val );
    if (pixmap) XFreePixmap( display, pixmap );
    return TRUE;
}


/***********************************************************************
 *           DC_SetupDCForBrush
 *
 * Setup dc->u.x.gc for drawing operations using current brush.
 * Return FALSE if brush is BS_NULL, TRUE otherwise.
 */
BOOL DC_SetupGCForBrush( DC * dc )
{
    return DC_SetupGCForPatBlt( dc, dc->u.x.gc, FALSE );
}


/***********************************************************************
 *           DC_SetupDCForPen
 *
 * Setup dc->u.x.gc for drawing operations using current pen.
 * Return FALSE if pen is PS_NULL, TRUE otherwise.
 */
BOOL DC_SetupGCForPen( DC * dc )
{
    XGCValues val;

    if (dc->u.x.pen.style == PS_NULL) return FALSE;
    val.function   = DC_XROPfunction[dc->w.ROPmode-1];
    val.foreground = dc->u.x.pen.pixel;
    val.background = dc->w.backgroundPixel;
    val.fill_style = FillSolid;
    if ((dc->u.x.pen.style!=PS_SOLID) && (dc->u.x.pen.style!=PS_INSIDEFRAME))
    {
	XSetDashes( display, dc->u.x.gc, 0,
		    dc->u.x.pen.dashes, dc->u.x.pen.dash_len );
	val.line_style = (dc->w.backgroundMode == OPAQUE) ?
	                      LineDoubleDash : LineOnOffDash;
    }
    else val.line_style = LineSolid;
    val.line_width = dc->u.x.pen.width;
    val.cap_style  = CapRound;
    val.join_style = JoinBevel;
    XChangeGC( display, dc->u.x.gc, 
	       GCFunction | GCForeground | GCBackground | GCLineWidth |
	       GCLineStyle | GCCapStyle | GCJoinStyle | GCFillStyle, &val );
    return TRUE;
}


/***********************************************************************
 *           DC_SetupGCForText
 *
 * Setup dc->u.x.gc for text drawing operations.
 * Return FALSE if the font is null, TRUE otherwise.
 */
BOOL DC_SetupGCForText( DC * dc )
{
    XGCValues val;

    if (!dc->u.x.font.fstruct)
    {
        fprintf( stderr, "DC_SetupGCForText: fstruct is NULL. Please report this\n" );
        return FALSE;
    }
    val.function   = GXcopy;  /* Text is always GXcopy */
    val.foreground = dc->w.textPixel;
    val.background = dc->w.backgroundPixel;
    val.fill_style = FillSolid;
    val.font       = dc->u.x.font.fstruct->fid;
    XChangeGC( display, dc->u.x.gc, 
	       GCFunction | GCForeground | GCBackground | GCFillStyle |
	       GCFont, &val );
    return TRUE;
}


/***********************************************************************
 *           GetDCState    (GDI.179)
 */
HDC GetDCState( HDC hdc )
{
    DC * newdc, * dc;
    HANDLE handle;
    
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    if (!(handle = GDI_AllocObject( sizeof(DC), DC_MAGIC ))) return 0;
    newdc = (DC *) GDI_HEAP_ADDR( handle );

    dprintf_dc(stddeb, "GetDCState(%d): returning %d\n", hdc, handle );

    memcpy( &newdc->w, &dc->w, sizeof(dc->w) );
    memcpy( &newdc->u.x.pen, &dc->u.x.pen, sizeof(dc->u.x.pen) );
    newdc->saveLevel = 0;
    newdc->w.flags |= DC_SAVED;

    newdc->w.hGCClipRgn = 0;
    newdc->w.hVisRgn = CreateRectRgn( 0, 0, 0, 0 );
    CombineRgn( newdc->w.hVisRgn, dc->w.hVisRgn, 0, RGN_COPY );	
    if (dc->w.hClipRgn)
    {
	newdc->w.hClipRgn = CreateRectRgn( 0, 0, 0, 0 );
	CombineRgn( newdc->w.hClipRgn, dc->w.hClipRgn, 0, RGN_COPY );
    }
    COLOR_SetMapping( newdc, dc->u.x.pal.hMapping,
                      dc->u.x.pal.hRevMapping, dc->u.x.pal.mappingSize );
    return handle;
}


/***********************************************************************
 *           SetDCState    (GDI.180)
 */
void SetDCState( HDC hdc, HDC hdcs )
{
    DC * dc, * dcs;
    HRGN hVisRgn, hClipRgn, hGCClipRgn;

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return;
    if (!(dcs = (DC *) GDI_GetObjPtr( hdcs, DC_MAGIC ))) return;
    if (!dcs->w.flags & DC_SAVED) return;
    dprintf_dc(stddeb, "SetDCState: %d %d\n", hdc, hdcs );

      /* Save the regions before overwriting everything */
    hVisRgn    = dc->w.hVisRgn;
    hClipRgn   = dc->w.hClipRgn;
    hGCClipRgn = dc->w.hGCClipRgn;
    memcpy( &dc->w, &dcs->w, sizeof(dc->w) );
    memcpy( &dc->u.x.pen, &dcs->u.x.pen, sizeof(dc->u.x.pen) );
    dc->w.flags &= ~DC_SAVED;

      /* Restore the regions */
    dc->w.hVisRgn    = hVisRgn;
    dc->w.hClipRgn   = hClipRgn;
    dc->w.hGCClipRgn = hGCClipRgn;
    CombineRgn( dc->w.hVisRgn, dcs->w.hVisRgn, 0, RGN_COPY );
    SelectClipRgn( hdc, dcs->w.hClipRgn );

    SelectObject( hdc, dcs->w.hBrush );
    SelectObject( hdc, dcs->w.hFont );
    COLOR_SetMapping( dc, dcs->u.x.pal.hMapping,
                      dcs->u.x.pal.hRevMapping, dcs->u.x.pal.mappingSize );
}


/***********************************************************************
 *           SaveDC    (GDI.30)
 */
int SaveDC( HDC hdc )
{
    HDC hdcs;
    DC * dc, * dcs;

    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
	MF_MetaParam0(dc, META_SAVEDC);
	return 1;  /* ?? */
    }
    if (!(hdcs = GetDCState( hdc ))) return 0;
    dcs = (DC *) GDI_HEAP_ADDR( hdcs );
    dcs->header.hNext = dc->header.hNext;
    dc->header.hNext = hdcs;
    dprintf_dc(stddeb, "SaveDC(%d): returning %d\n", hdc, dc->saveLevel+1 );
    return ++dc->saveLevel;
}


/***********************************************************************
 *           RestoreDC    (GDI.39)
 */
BOOL RestoreDC( HDC hdc, short level )
{
    DC * dc, * dcs;

    dprintf_dc(stddeb, "RestoreDC: %d %d\n", hdc, level );
    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	if (level != -1) return FALSE;
	MF_MetaParam1(dc, META_RESTOREDC, level);
	return TRUE;
    }
    if (level == -1) level = dc->saveLevel;
    if ((level < 1) || (level > (short)dc->saveLevel)) return FALSE;
    
    while ((short)dc->saveLevel >= level)
    {
	HDC hdcs = dc->header.hNext;
	if (!(dcs = (DC *) GDI_GetObjPtr( hdcs, DC_MAGIC ))) return FALSE;
	dc->header.hNext = dcs->header.hNext;
	if ((short)--dc->saveLevel < level) SetDCState( hdc, hdcs );
	DeleteDC( hdcs );
    }
    return TRUE;
}


/***********************************************************************
 *           CreateDC    (GDI.53)
 */
HDC CreateDC( LPSTR driver, LPSTR device, LPSTR output, LPSTR initData )
{
    DC * dc;
    HANDLE handle;
    
    handle = GDI_AllocObject( sizeof(DC), DC_MAGIC );
    if (!handle) return 0;
    dc = (DC *) GDI_HEAP_ADDR( handle );

    dprintf_dc(stddeb, "CreateDC(%s %s %s): returning %d\n", 
	    driver, device, output, handle );

    if (!displayDevCaps)
    {
	displayDevCaps = (DeviceCaps *) malloc( sizeof(DeviceCaps) );
	DC_FillDevCaps( displayDevCaps );
    }

    dc->saveLevel = 0;
    memcpy( &dc->w, &DC_defaultValues, sizeof(DC_defaultValues) );
    memset( &dc->u.x, 0, sizeof(dc->u.x) );

    dc->u.x.drawable   = rootWindow;
    dc->u.x.gc         = XCreateGC( display, dc->u.x.drawable, 0, NULL );
    dc->w.flags        = 0;
    dc->w.devCaps      = displayDevCaps;
    dc->w.bitsPerPixel = displayDevCaps->bitsPixel;
    dc->w.hVisRgn      = CreateRectRgn( 0, 0, displayDevCaps->horzRes,
                                        displayDevCaps->vertRes );
    if (!dc->w.hVisRgn)
    {
        GDI_HEAP_FREE( handle );
        return 0;
    }

    DC_InitDC( handle );

    return handle;
}


/***********************************************************************
 *           CreateIC    (GDI.153)
 */
HDC CreateIC( LPSTR driver, LPSTR device, LPSTR output, LPSTR initData )
{
      /* Nothing special yet for ICs */
    return CreateDC( driver, device, output, initData );
}


/***********************************************************************
 *           CreateCompatibleDC    (GDI.52)
 */
HDC CreateCompatibleDC( HDC hdc )
{
    DC * dc;
    HANDLE handle;
    HBITMAP hbitmap;
    BITMAPOBJ *bmp;

    handle = GDI_AllocObject( sizeof(DC), DC_MAGIC );
    if (!handle) return 0;
    dc = (DC *) GDI_HEAP_ADDR( handle );

    dprintf_dc(stddeb, "CreateCompatibleDC(%d): returning %d\n", hdc, handle );

      /* Create default bitmap */
    if (!(hbitmap = CreateBitmap( 1, 1, 1, 1, NULL )))
    {
	GDI_HEAP_FREE( handle );
	return 0;
    }
    bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    
    dc->saveLevel = 0;
    memcpy( &dc->w, &DC_defaultValues, sizeof(DC_defaultValues) );
    memset( &dc->u.x, 0, sizeof(dc->u.x) );

    dc->u.x.drawable   = bmp->pixmap;
    dc->u.x.gc         = XCreateGC( display, dc->u.x.drawable, 0, NULL );
    dc->w.flags        = DC_MEMORY;
    dc->w.bitsPerPixel = 1;
    dc->w.devCaps      = displayDevCaps;
    dc->w.hBitmap      = hbitmap;
    dc->w.hFirstBitmap = hbitmap;
    dc->w.hVisRgn      = CreateRectRgn( 0, 0, 1, 1 );

    if (!dc->w.hVisRgn)
    {
        DeleteObject( hbitmap );
        GDI_HEAP_FREE( handle );
        return 0;
    }

    DC_InitDC( handle );

    return handle;
}


/***********************************************************************
 *           DeleteDC    (GDI.68)
 */
BOOL DeleteDC( HDC hdc )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    dprintf_dc(stddeb, "DeleteDC: %d\n", hdc );

    while (dc->saveLevel)
    {
	DC * dcs;
	HDC hdcs = dc->header.hNext;
	if (!(dcs = (DC *) GDI_GetObjPtr( hdcs, DC_MAGIC ))) break;
	dc->header.hNext = dcs->header.hNext;
	dc->saveLevel--;
	DeleteDC( hdcs );
    }
    
    if (!(dc->w.flags & DC_SAVED))
    {
	SelectObject( hdc, STOCK_BLACK_PEN );
	SelectObject( hdc, STOCK_WHITE_BRUSH );
	SelectObject( hdc, STOCK_SYSTEM_FONT );
	XFreeGC( display, dc->u.x.gc );
    }

    if (dc->w.flags & DC_MEMORY) DeleteObject( dc->w.hFirstBitmap );
    if (dc->w.hClipRgn) DeleteObject( dc->w.hClipRgn );
    if (dc->w.hVisRgn) DeleteObject( dc->w.hVisRgn );
    if (dc->w.hGCClipRgn) DeleteObject( dc->w.hGCClipRgn );
    
    return GDI_FreeObject( hdc );
}


/***********************************************************************
 *           GetDeviceCaps    (GDI.80)
 */
int GetDeviceCaps( HDC hdc, WORD cap )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;

    if (cap > sizeof(DeviceCaps)-sizeof(WORD)) return 0;
    
    dprintf_dc(stddeb, "GetDeviceCaps(%d,%d): returning %d\n",
	    hdc, cap, *(WORD *)(((char *)dc->w.devCaps) + cap) );
    return *(WORD *)(((char *)dc->w.devCaps) + cap);
}


/***********************************************************************
 *           SetBkColor    (GDI.1)
 */
COLORREF SetBkColor( HDC hdc, COLORREF color )
{
    COLORREF oldColor;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0x80000000;
	MF_MetaParam2(dc, META_SETBKCOLOR, HIWORD(color), LOWORD(color));
	return 0;  /* ?? */
    }

    oldColor = dc->w.backgroundColor;
    dc->w.backgroundColor = color;
    dc->w.backgroundPixel = COLOR_ToPhysical( dc, color );
    return oldColor;
}


/***********************************************************************
 *           SetTextColor    (GDI.9)
 */
COLORREF SetTextColor( HDC hdc, COLORREF color )
{
    COLORREF oldColor;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0x80000000;
	MF_MetaParam2(dc, META_SETTEXTCOLOR, HIWORD(color), LOWORD(color));
	return 0;  /* ?? */
    }

    oldColor = dc->w.textColor;
    dc->w.textColor = color;
    dc->w.textPixel = COLOR_ToPhysical( dc, color );
    return oldColor;
}


/***********************************************************************
 *           GetDCOrg    (GDI.79)
 */
DWORD GetDCOrg( HDC hdc )
{
    Window root;
    int x, y, w, h, border, depth;

    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    if (dc->w.flags & DC_MEMORY) return 0;
    XGetGeometry( display, dc->u.x.drawable, &root,
		  &x, &y, &w, &h, &border, &depth );
    return MAKELONG( dc->w.DCOrgX + (WORD)x, dc->w.DCOrgY + (WORD)y );
}


/***********************************************************************
 *           SetDCOrg    (GDI.117)
 */
DWORD SetDCOrg( HDC hdc, short x, short y )
{
    DWORD prevOrg;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    prevOrg = dc->w.DCOrgX | (dc->w.DCOrgY << 16);
    dc->w.DCOrgX = x;
    dc->w.DCOrgY = y;
    return prevOrg;
}
