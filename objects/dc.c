/*
 * GDI Device Context functions
 *
 * Copyright 1993 Alexandre Julliard
 *
 */

#define NO_TRANSITION_TYPES  /* This file is Win32-clean */
#include <stdlib.h>
#include <string.h>
#include "gdi.h"
#include "bitmap.h"
#include "heap.h"
#include "metafile.h"
#include "stddebug.h"
#include "color.h"
#include "debug.h"
#include "font.h"

extern void CLIPPING_UpdateGCRegion( DC * dc );     /* objects/clipping.c */

  /* Default DC values */
static const WIN_DC_INFO DC_defaultValues =
{
    0,                      /* flags */
    NULL,                   /* devCaps */
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
    0                       /* CursPosY */
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
    caps->numBrushes    = 16+6;  /* 16 solid + 6 hatched brushes */
    caps->numPens       = 16;    /* 16 solid pens */
    caps->numMarkers    = 0;
    caps->numFonts      = 0;
    caps->numColors     = 100;
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
                          RC_DI_BITMAP | RC_DIBTODEV | RC_BIGFONT|
                          RC_STRETCHBLT | RC_STRETCHDIB | RC_DEVBITS;

    if( !(COLOR_GetSystemPaletteFlags() & COLOR_VIRTUAL) )
        caps->rasterCaps |= RC_PALETTE;

    caps->aspectX       = 36;  /* ?? */
    caps->aspectY       = 36;  /* ?? */
    caps->aspectXY      = 51;
    caps->logPixelsX    = (int)(caps->horzRes * 25.4 / caps->horzSize);
    caps->logPixelsY    = (int)(caps->vertRes * 25.4 / caps->vertSize);
    caps->sizePalette   = (caps->rasterCaps & RC_PALETTE)
                          ? DefaultVisual(display,DefaultScreen(display))->map_entries
                          : 0;
    caps->numReserved   = 0;
    caps->colorRes      = 0;
}


/***********************************************************************
 *           DC_AllocDC
 */
DC *DC_AllocDC( const DC_FUNCTIONS *funcs )
{
    HDC16 hdc;
    DC *dc;

    if (!(hdc = GDI_AllocObject( sizeof(DC), DC_MAGIC ))) return NULL;
    dc = (DC *) GDI_HEAP_LIN_ADDR( hdc );

    dc->hSelf      = hdc;
    dc->funcs      = funcs;
    dc->physDev    = NULL;
    dc->saveLevel  = 0;
    dc->dwHookData = 0L;
    dc->hookProc   = NULL;
    dc->wndOrgX    = 0;
    dc->wndOrgY    = 0;
    dc->wndExtX    = 1;
    dc->wndExtY    = 1;
    dc->vportOrgX  = 0;
    dc->vportOrgY  = 0;
    dc->vportExtX  = 1;
    dc->vportExtY  = 1;

    memcpy( &dc->w, &DC_defaultValues, sizeof(DC_defaultValues) );
    return dc;
}


/***********************************************************************
 *           DC_GetDCPtr
 */
DC *DC_GetDCPtr( HDC32 hdc )
{
    GDIOBJHDR *ptr = (GDIOBJHDR *)GDI_HEAP_LIN_ADDR( hdc );
    if (!ptr) return NULL;
    if ((ptr->wMagic == DC_MAGIC) || (ptr->wMagic == METAFILE_DC_MAGIC))
        return (DC *)ptr;
    return NULL;
}


/***********************************************************************
 *           DC_InitDC
 *
 * Setup device-specific DC values for a newly created DC.
 */
void DC_InitDC( DC* dc )
{
    RealizeDefaultPalette( dc->hSelf );
    SetTextColor( dc->hSelf, dc->w.textColor );
    SetBkColor( dc->hSelf, dc->w.backgroundColor );
    SelectObject32( dc->hSelf, dc->w.hPen );
    SelectObject32( dc->hSelf, dc->w.hBrush );
    SelectObject32( dc->hSelf, dc->w.hFont );
    CLIPPING_UpdateGCRegion( dc );
}


/***********************************************************************
 *           DC_SetupGCForPatBlt
 *
 * Setup the GC for a PatBlt operation using current brush.
 * If fMapColors is TRUE, X pixels are mapped to Windows colors.
 * Return FALSE if brush is BS_NULL, TRUE otherwise.
 */
BOOL32 DC_SetupGCForPatBlt( DC * dc, GC gc, BOOL32 fMapColors )
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

    if (dc->w.flags & DC_DIRTY) CLIPPING_UpdateGCRegion(dc);

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
 *           DC_SetupGCForBrush
 *
 * Setup dc->u.x.gc for drawing operations using current brush.
 * Return FALSE if brush is BS_NULL, TRUE otherwise.
 */
BOOL32 DC_SetupGCForBrush( DC * dc )
{
    return DC_SetupGCForPatBlt( dc, dc->u.x.gc, FALSE );
}


/***********************************************************************
 *           DC_SetupGCForPen
 *
 * Setup dc->u.x.gc for drawing operations using current pen.
 * Return FALSE if pen is PS_NULL, TRUE otherwise.
 */
BOOL32 DC_SetupGCForPen( DC * dc )
{
    XGCValues val;

    if (dc->u.x.pen.style == PS_NULL) return FALSE;

    if (dc->w.flags & DC_DIRTY) CLIPPING_UpdateGCRegion(dc); 

    if ((screenDepth <= 8) &&  /* FIXME: Should check for palette instead */
        ((dc->w.ROPmode == R2_BLACK) || (dc->w.ROPmode == R2_WHITE)))
    {
        val.function   = GXcopy;
        val.foreground = COLOR_ToPhysical( NULL, (dc->w.ROPmode == R2_BLACK) ?
                                               RGB(0,0,0) : RGB(255,255,255) );
    }
    else
    {
        val.function   = DC_XROPfunction[dc->w.ROPmode-1];
        val.foreground = dc->u.x.pen.pixel;
    }
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
    val.join_style = JoinMiter;
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
BOOL32 DC_SetupGCForText( DC * dc )
{
    XGCValues val;

    if (!dc->u.x.font.fstruct)
    {
        fprintf( stderr, "DC_SetupGCForText: fstruct is NULL. Please report this\n" );
        return FALSE;
    }
   
    if (dc->w.flags & DC_DIRTY) CLIPPING_UpdateGCRegion(dc);

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
HDC16 GetDCState( HDC16 hdc )
{
    DC * newdc, * dc;
    HGDIOBJ16 handle;
    
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    if (!(handle = GDI_AllocObject( sizeof(DC), DC_MAGIC ))) return 0;
    newdc = (DC *) GDI_HEAP_LIN_ADDR( handle );

    dprintf_dc(stddeb, "GetDCState(%04x): returning %04x\n", hdc, handle );

    memset( &newdc->u.x, 0, sizeof(newdc->u.x) );
    newdc->w.flags           = dc->w.flags | DC_SAVED;
    newdc->w.devCaps         = dc->w.devCaps;
    newdc->w.hPen            = dc->w.hPen;       
    newdc->w.hBrush          = dc->w.hBrush;     
    newdc->w.hFont           = dc->w.hFont;      
    newdc->w.hBitmap         = dc->w.hBitmap;    
    newdc->w.hFirstBitmap    = dc->w.hFirstBitmap;
    newdc->w.hDevice         = dc->w.hDevice;
    newdc->w.hPalette        = dc->w.hPalette;   
    newdc->w.bitsPerPixel    = dc->w.bitsPerPixel;
    newdc->w.ROPmode         = dc->w.ROPmode;
    newdc->w.polyFillMode    = dc->w.polyFillMode;
    newdc->w.stretchBltMode  = dc->w.stretchBltMode;
    newdc->w.relAbsMode      = dc->w.relAbsMode;
    newdc->w.backgroundMode  = dc->w.backgroundMode;
    newdc->w.backgroundColor = dc->w.backgroundColor;
    newdc->w.textColor       = dc->w.textColor;
    newdc->w.backgroundPixel = dc->w.backgroundPixel;
    newdc->w.textPixel       = dc->w.textPixel;
    newdc->w.brushOrgX       = dc->w.brushOrgX;
    newdc->w.brushOrgY       = dc->w.brushOrgY;
    newdc->w.textAlign       = dc->w.textAlign;
    newdc->w.charExtra       = dc->w.charExtra;
    newdc->w.breakTotalExtra = dc->w.breakTotalExtra;
    newdc->w.breakCount      = dc->w.breakCount;
    newdc->w.breakExtra      = dc->w.breakExtra;
    newdc->w.breakRem        = dc->w.breakRem;
    newdc->w.MapMode         = dc->w.MapMode;
    newdc->w.DCOrgX          = dc->w.DCOrgX;
    newdc->w.DCOrgY          = dc->w.DCOrgY;
    newdc->w.CursPosX        = dc->w.CursPosX;
    newdc->w.CursPosY        = dc->w.CursPosY;
    newdc->wndOrgX           = dc->wndOrgX;
    newdc->wndOrgY           = dc->wndOrgY;
    newdc->wndExtX           = dc->wndExtX;
    newdc->wndExtY           = dc->wndExtY;
    newdc->vportOrgX         = dc->vportOrgX;
    newdc->vportOrgY         = dc->vportOrgY;
    newdc->vportExtX         = dc->vportExtX;
    newdc->vportExtY         = dc->vportExtY;

    newdc->hSelf = (HDC32)handle;
    newdc->saveLevel = 0;

    newdc->w.hGCClipRgn = 0;
    newdc->w.hVisRgn = CreateRectRgn32( 0, 0, 0, 0 );
    CombineRgn32( newdc->w.hVisRgn, dc->w.hVisRgn, 0, RGN_COPY );	
    if (dc->w.hClipRgn)
    {
	newdc->w.hClipRgn = CreateRectRgn32( 0, 0, 0, 0 );
	CombineRgn32( newdc->w.hClipRgn, dc->w.hClipRgn, 0, RGN_COPY );
    }
    else
	newdc->w.hClipRgn = 0;
    return handle;
}


/***********************************************************************
 *           SetDCState    (GDI.180)
 */
void SetDCState( HDC16 hdc, HDC16 hdcs )
{
    DC *dc, *dcs;
    
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return;
    if (!(dcs = (DC *) GDI_GetObjPtr( hdcs, DC_MAGIC ))) return;
    if (!dcs->w.flags & DC_SAVED) return;
    dprintf_dc(stddeb, "SetDCState: %04x %04x\n", hdc, hdcs );

    dc->w.flags           = dcs->w.flags & ~DC_SAVED;
    dc->w.devCaps         = dcs->w.devCaps;
    dc->w.hFirstBitmap    = dcs->w.hFirstBitmap;
    dc->w.hDevice         = dcs->w.hDevice;
    dc->w.ROPmode         = dcs->w.ROPmode;
    dc->w.polyFillMode    = dcs->w.polyFillMode;
    dc->w.stretchBltMode  = dcs->w.stretchBltMode;
    dc->w.relAbsMode      = dcs->w.relAbsMode;
    dc->w.backgroundMode  = dcs->w.backgroundMode;
    dc->w.backgroundColor = dcs->w.backgroundColor;
    dc->w.textColor       = dcs->w.textColor;
    dc->w.backgroundPixel = dcs->w.backgroundPixel;
    dc->w.textPixel       = dcs->w.textPixel;
    dc->w.brushOrgX       = dcs->w.brushOrgX;
    dc->w.brushOrgY       = dcs->w.brushOrgY;
    dc->w.textAlign       = dcs->w.textAlign;
    dc->w.charExtra       = dcs->w.charExtra;
    dc->w.breakTotalExtra = dcs->w.breakTotalExtra;
    dc->w.breakCount      = dcs->w.breakCount;
    dc->w.breakExtra      = dcs->w.breakExtra;
    dc->w.breakRem        = dcs->w.breakRem;
    dc->w.MapMode         = dcs->w.MapMode;
    dc->w.DCOrgX          = dcs->w.DCOrgX;
    dc->w.DCOrgY          = dcs->w.DCOrgY;
    dc->w.CursPosX        = dcs->w.CursPosX;
    dc->w.CursPosY        = dcs->w.CursPosY;

    dc->wndOrgX           = dcs->wndOrgX;
    dc->wndOrgY           = dcs->wndOrgY;
    dc->wndExtX           = dcs->wndExtX;
    dc->wndExtY           = dcs->wndExtY;
    dc->vportOrgX         = dcs->vportOrgX;
    dc->vportOrgY         = dcs->vportOrgY;
    dc->vportExtX         = dcs->vportExtX;
    dc->vportExtY         = dcs->vportExtY;

    if (!(dc->w.flags & DC_MEMORY)) dc->w.bitsPerPixel = dcs->w.bitsPerPixel;
    CombineRgn32( dc->w.hVisRgn, dcs->w.hVisRgn, 0, RGN_COPY );
    SelectClipRgn32( hdc, dcs->w.hClipRgn );
    SelectObject32( hdc, dcs->w.hBitmap );
    SelectObject32( hdc, dcs->w.hBrush );
    SelectObject32( hdc, dcs->w.hFont );
    SelectObject32( hdc, dcs->w.hPen );
    GDISelectPalette( hdc, dcs->w.hPalette, FALSE );
}


/***********************************************************************
 *           SaveDC16    (GDI.30)
 */
INT16 SaveDC16( HDC16 hdc )
{
    return (INT16)SaveDC32( hdc );
}


/***********************************************************************
 *           SaveDC32    (GDI32.292)
 */
INT32 SaveDC32( HDC32 hdc )
{
    HDC32 hdcs;
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
    dcs = (DC *) GDI_HEAP_LIN_ADDR( hdcs );
    dcs->header.hNext = dc->header.hNext;
    dc->header.hNext = hdcs;
    dprintf_dc(stddeb, "SaveDC(%04x): returning %d\n", hdc, dc->saveLevel+1 );
    return ++dc->saveLevel;
}


/***********************************************************************
 *           RestoreDC16    (GDI.39)
 */
BOOL16 RestoreDC16( HDC16 hdc, INT16 level )
{
    return RestoreDC32( hdc, level );
}


/***********************************************************************
 *           RestoreDC32    (GDI32.290)
 */
BOOL32 RestoreDC32( HDC32 hdc, INT32 level )
{
    DC * dc, * dcs;

    dprintf_dc(stddeb, "RestoreDC: %04x %d\n", hdc, level );
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
    if ((level < 1) || (level > dc->saveLevel)) return FALSE;
    
    while (dc->saveLevel >= level)
    {
	HDC16 hdcs = dc->header.hNext;
	if (!(dcs = (DC *) GDI_GetObjPtr( hdcs, DC_MAGIC ))) return FALSE;
	dc->header.hNext = dcs->header.hNext;
	if (--dc->saveLevel < level) SetDCState( hdc, hdcs );
	DeleteDC32( hdcs );
    }
    return TRUE;
}


/***********************************************************************
 *           CreateDC16    (GDI.53)
 */
HDC16 CreateDC16( LPCSTR driver, LPCSTR device, LPCSTR output,
                  const DEVMODE16 *initData )
{
    DC * dc;
    const DC_FUNCTIONS *funcs;

    if (!(funcs = DRIVER_FindDriver( driver ))) return 0;
    if (!(dc = DC_AllocDC( funcs ))) return 0;
    dc->w.flags = 0;

    dprintf_dc(stddeb, "CreateDC(%s %s %s): returning %04x\n",
               driver, device, output, dc->hSelf );

    if (dc->funcs->pCreateDC &&
        !dc->funcs->pCreateDC( dc, driver, device, output, initData ))
    {
        dprintf_dc( stddeb, "CreateDC: creation aborted by device\n" );
        GDI_HEAP_FREE( dc->hSelf );
        return 0;
    }

    DC_InitDC( dc );
    return dc->hSelf;
}


/***********************************************************************
 *           CreateDC32A    (GDI32.)
 */
HDC32 CreateDC32A( LPCSTR driver, LPCSTR device, LPCSTR output,
                   const DEVMODE32A *initData )
{
    return CreateDC16( driver, device, output, (const DEVMODE16 *)initData );
}


/***********************************************************************
 *           CreateDC32W    (GDI32.)
 */
HDC32 CreateDC32W( LPCWSTR driver, LPCWSTR device, LPCWSTR output,
                   const DEVMODE32W *initData )
{ 
    LPSTR driverA = HEAP_strdupWtoA( GetProcessHeap(), 0, driver );
    LPSTR deviceA = HEAP_strdupWtoA( GetProcessHeap(), 0, device );
    LPSTR outputA = HEAP_strdupWtoA( GetProcessHeap(), 0, output );
    HDC32 res = CreateDC16( driverA, deviceA, outputA,
                            (const DEVMODE16 *)initData /*FIXME*/ );
    HeapFree( GetProcessHeap(), 0, driverA );
    HeapFree( GetProcessHeap(), 0, deviceA );
    HeapFree( GetProcessHeap(), 0, outputA );
    return res;
}


/***********************************************************************
 *           CreateIC16    (GDI.153)
 */
HDC16 CreateIC16( LPCSTR driver, LPCSTR device, LPCSTR output,
                  const DEVMODE16* initData )
{
      /* Nothing special yet for ICs */
    return CreateDC16( driver, device, output, initData );
}


/***********************************************************************
 *           CreateIC32A    (GDI32.49)
 */
HDC32 CreateIC32A( LPCSTR driver, LPCSTR device, LPCSTR output,
                   const DEVMODE32A* initData )
{
      /* Nothing special yet for ICs */
    return CreateDC32A( driver, device, output, initData );
}


/***********************************************************************
 *           CreateIC32W    (GDI32.50)
 */
HDC32 CreateIC32W( LPCWSTR driver, LPCWSTR device, LPCWSTR output,
                   const DEVMODE32W* initData )
{
      /* Nothing special yet for ICs */
    return CreateDC32W( driver, device, output, initData );
}


/***********************************************************************
 *           CreateCompatibleDC16    (GDI.52)
 */
HDC16 CreateCompatibleDC16( HDC16 hdc )
{
    return (HDC16)CreateCompatibleDC32( hdc );
}


/***********************************************************************
 *           CreateCompatibleDC32   (GDI32.31)
 */
HDC32 CreateCompatibleDC32( HDC32 hdc )
{
    DC *dc, *origDC;
    HBITMAP32 hbitmap;
    const DC_FUNCTIONS *funcs;

    if ((origDC = (DC *)GDI_GetObjPtr( hdc, DC_MAGIC ))) funcs = origDC->funcs;
    else funcs = DRIVER_FindDriver( "DISPLAY" );
    if (!funcs) return 0;

    if (!(dc = DC_AllocDC( funcs ))) return 0;

    dprintf_dc(stddeb, "CreateCompatibleDC(%04x): returning %04x\n",
               hdc, dc->hSelf );

      /* Create default bitmap */
    if (!(hbitmap = CreateBitmap( 1, 1, 1, 1, NULL )))
    {
	GDI_HEAP_FREE( dc->hSelf );
	return 0;
    }
    dc->w.flags        = DC_MEMORY;
    dc->w.bitsPerPixel = 1;
    dc->w.hBitmap      = hbitmap;
    dc->w.hFirstBitmap = hbitmap;

    if (dc->funcs->pCreateDC &&
        !dc->funcs->pCreateDC( dc, NULL, NULL, NULL, NULL ))
    {
        dprintf_dc(stddeb, "CreateCompatibleDC: creation aborted by device\n");
        DeleteObject32( hbitmap );
        GDI_HEAP_FREE( dc->hSelf );
        return 0;
    }

    DC_InitDC( dc );
    return dc->hSelf;
}


/***********************************************************************
 *           DeleteDC16    (GDI.68)
 */
BOOL16 DeleteDC16( HDC16 hdc )
{
    return DeleteDC32( hdc );
}


/***********************************************************************
 *           DeleteDC32    (GDI32.67)
 */
BOOL32 DeleteDC32( HDC32 hdc )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    dprintf_dc(stddeb, "DeleteDC: %04x\n", hdc );

    while (dc->saveLevel)
    {
	DC * dcs;
	HDC16 hdcs = dc->header.hNext;
	if (!(dcs = (DC *) GDI_GetObjPtr( hdcs, DC_MAGIC ))) break;
	dc->header.hNext = dcs->header.hNext;
	dc->saveLevel--;
	DeleteDC32( hdcs );
    }
    
    if (!(dc->w.flags & DC_SAVED))
    {
	SelectObject32( hdc, STOCK_BLACK_PEN );
	SelectObject32( hdc, STOCK_WHITE_BRUSH );
	SelectObject32( hdc, STOCK_SYSTEM_FONT );
        if (dc->w.flags & DC_MEMORY) DeleteObject32( dc->w.hFirstBitmap );
        if (dc->funcs->pDeleteDC) dc->funcs->pDeleteDC(dc);
    }

    if (dc->w.hClipRgn) DeleteObject32( dc->w.hClipRgn );
    if (dc->w.hVisRgn) DeleteObject32( dc->w.hVisRgn );
    if (dc->w.hGCClipRgn) DeleteObject32( dc->w.hGCClipRgn );
    
    return GDI_FreeObject( hdc );
}


/***********************************************************************
 *           ResetDC16    (GDI.376)
 */
HDC16 ResetDC16( HDC16 hdc, const DEVMODE16 *devmode )
{
    fprintf( stderr, "ResetDC16: empty stub!\n" );
    return hdc;
}


/***********************************************************************
 *           ResetDC32A    (GDI32.287)
 */
HDC32 ResetDC32A( HDC32 hdc, const DEVMODE32A *devmode )
{
    fprintf( stderr, "ResetDC32A: empty stub!\n" );
    return hdc;
}


/***********************************************************************
 *           ResetDC32W    (GDI32.288)
 */
HDC32 ResetDC32W( HDC32 hdc, const DEVMODE32W *devmode )
{
    fprintf( stderr, "ResetDC32A: empty stub!\n" );
    return hdc;
}


/***********************************************************************
 *           GetDeviceCaps16    (GDI.80)
 */
INT16 GetDeviceCaps16( HDC16 hdc, INT16 cap )
{
    return GetDeviceCaps32( hdc, cap );
}


/***********************************************************************
 *           GetDeviceCaps32    (GDI32.171)
 */
INT32 GetDeviceCaps32( HDC32 hdc, INT32 cap )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;

    if ((cap < 0) || (cap > sizeof(DeviceCaps)-sizeof(WORD))) return 0;
    
    dprintf_dc(stddeb, "GetDeviceCaps(%04x,%d): returning %d\n",
	    hdc, cap, *(WORD *)(((char *)dc->w.devCaps) + cap) );
    return *(WORD *)(((char *)dc->w.devCaps) + cap);
}


/***********************************************************************
 *           SetBkColor    (GDI.1) (GDI32.305)
 */
COLORREF SetBkColor( HDC32 hdc, COLORREF color )
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
 *           SetTextColor    (GDI.9) (GDI32.338)
 */
COLORREF SetTextColor( HDC32 hdc, COLORREF color )
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
 *           SetTextAlign16    (GDI.346)
 */
UINT16 SetTextAlign16( HDC16 hdc, UINT16 textAlign )
{
    return SetTextAlign32( hdc, textAlign );
}


/***********************************************************************
 *           SetTextAlign32    (GDI32.336)
 */
UINT32 SetTextAlign32( HDC32 hdc, UINT32 textAlign )
{
    UINT32 prevAlign;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc)
    {
	if (!(dc = (DC *)GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC ))) return 0;
	MF_MetaParam1( dc, META_SETTEXTALIGN, textAlign );
	return 1;
    }
    prevAlign = dc->w.textAlign;
    dc->w.textAlign = textAlign;
    return prevAlign;
}


/***********************************************************************
 *           GetDCOrgEx  (GDI32.168)
 */
BOOL32 GetDCOrgEx( HDC32 hDC, LPPOINT32 lpp )
{
    DC * dc = (DC *) GDI_GetObjPtr( hDC, DC_MAGIC );
    if (!dc || !lpp) return FALSE;

    if (!(dc->w.flags & DC_MEMORY))
    {
       Window root;
       int w, h, border, depth;
       /* FIXME: this is not correct for managed windows */
       XGetGeometry( display, dc->u.x.drawable, &root,
                    &lpp->x, &lpp->y, &w, &h, &border, &depth );
    }
    else lpp->x = lpp->y = 0;
    lpp->x += dc->w.DCOrgX; lpp->y += dc->w.DCOrgY;
    return TRUE;
}


/***********************************************************************
 *           GetDCOrg    (GDI.79)
 */
DWORD GetDCOrg( HDC16 hdc )
{
    POINT32	pt;
    if( GetDCOrgEx( hdc, &pt) )
  	return MAKELONG( (WORD)pt.x, (WORD)pt.y );    
    return 0;
}


/***********************************************************************
 *           SetDCOrg    (GDI.117)
 */
DWORD SetDCOrg( HDC16 hdc, INT16 x, INT16 y )
{
    DWORD prevOrg;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    prevOrg = dc->w.DCOrgX | (dc->w.DCOrgY << 16);
    dc->w.DCOrgX = x;
    dc->w.DCOrgY = y;
    return prevOrg;
}


/***********************************************************************
 *           SetDCHook   (GDI.190)
 */
BOOL16 SetDCHook( HDC16 hdc, FARPROC16 hookProc, DWORD dwHookData )
{
    DC *dc = (DC *)GDI_GetObjPtr( hdc, DC_MAGIC );

    dprintf_dc( stddeb, "SetDCHook: hookProc %08x, default is %08x\n",
                (UINT32)hookProc, (UINT32)DCHook );

    if (!dc) return FALSE;
    dc->hookProc = hookProc;
    dc->dwHookData = dwHookData;
    return TRUE;
}


/***********************************************************************
 *           GetDCHook   (GDI.191)
 */
DWORD GetDCHook( HDC16 hdc, FARPROC16 *phookProc )
{
    DC *dc = (DC *)GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    *phookProc = dc->hookProc;
    return dc->dwHookData;
}


/***********************************************************************
 *           SetHookFlags       (GDI.192)
 */
WORD SetHookFlags(HDC16 hDC, WORD flags)
{
  DC* dc = (DC*)GDI_GetObjPtr( hDC, DC_MAGIC );

  if( dc )
    {
        WORD wRet = dc->w.flags & DC_DIRTY;

        /* "Undocumented Windows" info is slightly
         *  confusing
         */

        dprintf_dc(stddeb,"SetHookFlags: hDC %04x, flags %04x\n",hDC,flags);

        if( flags & DCHF_INVALIDATEVISRGN )
            dc->w.flags |= DC_DIRTY;
        else if( flags & DCHF_VALIDATEVISRGN || !flags )
            dc->w.flags &= ~DC_DIRTY;
        return wRet;
    }
  return 0;
}

