/*
 * GDI Device Context functions
 *
 * Copyright 1993 Alexandre Julliard
 *
 */

#include <stdlib.h>
#include <string.h>
#include "dc.h"
#include "gdi.h"
#include "heap.h"
#include "metafile.h"
#include "color.h"
#include "debug.h"
#include "font.h"
#include "winerror.h"
#include "x11font.h"

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
 *           DC_Init_DC_INFO
 *
 * Fill the WIN_DC_INFO structure.
 */
static void DC_Init_DC_INFO( WIN_DC_INFO *win_dc_info )
{
    win_dc_info->flags               = 0;
    win_dc_info->devCaps             = NULL;
    win_dc_info->hClipRgn            = 0;
    win_dc_info->hVisRgn             = 0;
    win_dc_info->hGCClipRgn          = 0;
    win_dc_info->hPen                = STOCK_BLACK_PEN;
    win_dc_info->hBrush              = STOCK_WHITE_BRUSH;
    win_dc_info->hFont               = STOCK_SYSTEM_FONT;
    win_dc_info->hBitmap             = 0;
    win_dc_info->hFirstBitmap        = 0;
    win_dc_info->hDevice             = 0;
    win_dc_info->hPalette            = STOCK_DEFAULT_PALETTE;
    win_dc_info->ROPmode             = R2_COPYPEN;
    win_dc_info->polyFillMode        = ALTERNATE;
    win_dc_info->stretchBltMode      = BLACKONWHITE;
    win_dc_info->relAbsMode          = ABSOLUTE;
    win_dc_info->backgroundMode      = OPAQUE;
    win_dc_info->backgroundColor     = RGB( 255, 255, 255 );
    win_dc_info->textColor           = RGB( 0, 0, 0 );
    win_dc_info->backgroundPixel     = 0;
    win_dc_info->textPixel           = 0;
    win_dc_info->brushOrgX           = 0;
    win_dc_info->brushOrgY           = 0;
    win_dc_info->textAlign           = TA_LEFT | TA_TOP | TA_NOUPDATECP;
    win_dc_info->charExtra           = 0;
    win_dc_info->breakTotalExtra     = 0;
    win_dc_info->breakCount          = 0;
    win_dc_info->breakExtra          = 0;
    win_dc_info->breakRem            = 0;
    win_dc_info->bitsPerPixel        = 1;
    win_dc_info->MapMode             = MM_TEXT;
    win_dc_info->GraphicsMode        = GM_COMPATIBLE;
    win_dc_info->DCOrgX              = 0;
    win_dc_info->DCOrgY              = 0;
    win_dc_info->lpfnPrint           = NULL;
    win_dc_info->CursPosX            = 0;
    win_dc_info->CursPosY            = 0;
    win_dc_info->ArcDirection        = AD_COUNTERCLOCKWISE;
    win_dc_info->xformWorld2Wnd.eM11 = 1.0f;
    win_dc_info->xformWorld2Wnd.eM12 = 0.0f;
    win_dc_info->xformWorld2Wnd.eM21 = 0.0f;
    win_dc_info->xformWorld2Wnd.eM22 = 1.0f;
    win_dc_info->xformWorld2Wnd.eDx  = 0.0f;
    win_dc_info->xformWorld2Wnd.eDy  = 0.0f;
    win_dc_info->xformWorld2Vport    = win_dc_info->xformWorld2Wnd;
    win_dc_info->xformVport2World    = win_dc_info->xformWorld2Wnd;
    win_dc_info->vport2WorldValid    = TRUE;

    PATH_InitGdiPath(&win_dc_info->path);
}


/***********************************************************************
 *           DC_AllocDC
 */
DC *DC_AllocDC( const DC_FUNCTIONS *funcs )
{
    HDC16 hdc;
    DC *dc;

    if (!(hdc = GDI_AllocObject( sizeof(DC), DC_MAGIC ))) return NULL;
    dc = (DC *) GDI_HEAP_LOCK( hdc );

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

    DC_Init_DC_INFO( &dc->w );

    return dc;
}



/***********************************************************************
 *           DC_GetDCPtr
 */
DC *DC_GetDCPtr( HDC32 hdc )
{
    GDIOBJHDR *ptr = (GDIOBJHDR *)GDI_HEAP_LOCK( hdc );
    if (!ptr) return NULL;
    if ((ptr->wMagic == DC_MAGIC) || (ptr->wMagic == METAFILE_DC_MAGIC))
        return (DC *)ptr;
    GDI_HEAP_UNLOCK( hdc );
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
    SetTextColor32( dc->hSelf, dc->w.textColor );
    SetBkColor32( dc->hSelf, dc->w.backgroundColor );
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
    /*
    ** Let's replace GXinvert by GXxor with (black xor white)
    ** This solves the selection color and leak problems in excel
    ** FIXME : Let's do that only if we work with X-pixels, not with Win-pixels
    */
    if (val.function == GXinvert)
	{
	val.foreground = BlackPixelOfScreen(screen) ^ WhitePixelOfScreen(screen);
	val.function = GXxor;
	}
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
            EnterCriticalSection( &X11DRV_CritSection );
            pixmap = XCreatePixmap( display, rootWindow, 8, 8, screenDepth );
            image = XGetImage( display, dc->u.x.brush.pixmap, 0, 0, 8, 8,
                               AllPlanes, ZPixmap );
            for (y = 0; y < 8; y++)
                for (x = 0; x < 8; x++)
                    XPutPixel( image, x, y,
                               COLOR_PixelToPalette[XGetPixel( image, x, y)] );
            XPutImage( display, pixmap, gc, image, 0, 0, 0, 0, 8, 8 );
            XDestroyImage( image );
            LeaveCriticalSection( &X11DRV_CritSection );
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
    TSXChangeGC( display, gc, 
	       GCFunction | GCForeground | GCBackground | GCFillStyle |
	       GCFillRule | GCTileStipXOrigin | GCTileStipYOrigin | mask,
	       &val );
    if (pixmap) TSXFreePixmap( display, pixmap );
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

    switch (dc->w.ROPmode)
    {
    case R2_BLACK :
	val.foreground = BlackPixelOfScreen( screen );
	val.function = GXcopy;
	break;
    case R2_WHITE :
	val.foreground = WhitePixelOfScreen( screen );
	val.function = GXcopy;
	break;
    case R2_XORPEN :
	val.foreground = dc->u.x.pen.pixel;
	/* It is very unlikely someone wants to XOR with 0 */
	/* This fixes the rubber-drawings in paintbrush */
	if (val.foreground == 0)
	    val.foreground = BlackPixelOfScreen( screen )
			    ^ WhitePixelOfScreen( screen );
	val.function = GXxor;
	break;
    default :
	val.foreground = dc->u.x.pen.pixel;
	val.function   = DC_XROPfunction[dc->w.ROPmode-1];
    }
    val.background = dc->w.backgroundPixel;
    val.fill_style = FillSolid;
    if ((dc->u.x.pen.style!=PS_SOLID) && (dc->u.x.pen.style!=PS_INSIDEFRAME))
    {
	TSXSetDashes( display, dc->u.x.gc, 0,
		    dc->u.x.pen.dashes, dc->u.x.pen.dash_len );
	val.line_style = (dc->w.backgroundMode == OPAQUE) ?
	                      LineDoubleDash : LineOnOffDash;
    }
    else val.line_style = LineSolid;
    val.line_width = dc->u.x.pen.width;
    if (val.line_width <= 1) {
	val.cap_style = CapNotLast;
    } else {
	switch (dc->u.x.pen.endcap)
	{
	case PS_ENDCAP_SQUARE:
	    val.cap_style = CapProjecting;
	    break;
	case PS_ENDCAP_FLAT:
	    val.cap_style = CapButt;
	    break;
	case PS_ENDCAP_ROUND:
	default:
	    val.cap_style = CapRound;
	}
    }
    switch (dc->u.x.pen.linejoin)
    {
    case PS_JOIN_BEVEL:
	val.join_style = JoinBevel;
    case PS_JOIN_MITER:
	val.join_style = JoinMiter;
    case PS_JOIN_ROUND:
    default:
	val.join_style = JoinRound;
    }
    TSXChangeGC( display, dc->u.x.gc, 
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
    XFontStruct* xfs = XFONT_GetFontStruct( dc->u.x.font );

    if( xfs )
    {
	XGCValues val;

	if (dc->w.flags & DC_DIRTY) CLIPPING_UpdateGCRegion(dc);

	val.function   = GXcopy;  /* Text is always GXcopy */
	val.foreground = dc->w.textPixel;
	val.background = dc->w.backgroundPixel;
	val.fill_style = FillSolid;
	val.font       = xfs->fid;

	TSXChangeGC( display, dc->u.x.gc,
		   GCFunction | GCForeground | GCBackground | GCFillStyle |
		   GCFont, &val );
	return TRUE;
    } 
    WARN(dc, "Physical font failure\n" );
    return FALSE;
}


/***********************************************************************
 *           DC_InvertXform
 *
 * Computes the inverse of the transformation xformSrc and stores it to
 * xformDest. Returns TRUE if successful or FALSE if the xformSrc matrix
 * is singular.
 */
static BOOL32 DC_InvertXform( const XFORM *xformSrc, XFORM *xformDest )
{
    FLOAT determinant;
    
    determinant = xformSrc->eM11*xformSrc->eM22 -
        xformSrc->eM12*xformSrc->eM21;
    if (determinant > -1e-12 && determinant < 1e-12)
        return FALSE;

    xformDest->eM11 =  xformSrc->eM22 / determinant;
    xformDest->eM12 = -xformSrc->eM12 / determinant;
    xformDest->eM21 = -xformSrc->eM21 / determinant;
    xformDest->eM22 =  xformSrc->eM11 / determinant;
    xformDest->eDx  = -xformSrc->eDx * xformDest->eM11 -
                       xformSrc->eDy * xformDest->eM21;
    xformDest->eDy  = -xformSrc->eDx * xformDest->eM12 -
                       xformSrc->eDy * xformDest->eM22;

    return TRUE;
}


/***********************************************************************
 *           DC_UpdateXforms
 *
 * Updates the xformWorld2Vport, xformVport2World and vport2WorldValid
 * fields of the specified DC by creating a transformation that
 * represents the current mapping mode and combining it with the DC's
 * world transform. This function should be called whenever the
 * parameters associated with the mapping mode (window and viewport
 * extents and origins) or the world transform change.
 */
void DC_UpdateXforms( DC *dc )
{
    XFORM xformWnd2Vport;
    FLOAT scaleX, scaleY;
    
    /* Construct a transformation to do the window-to-viewport conversion */
    scaleX = (FLOAT)dc->vportExtX / (FLOAT)dc->wndExtX;
    scaleY = (FLOAT)dc->vportExtY / (FLOAT)dc->wndExtY;
    xformWnd2Vport.eM11 = scaleX;
    xformWnd2Vport.eM12 = 0.0;
    xformWnd2Vport.eM21 = 0.0;
    xformWnd2Vport.eM22 = scaleY;
    xformWnd2Vport.eDx  = (FLOAT)dc->vportOrgX -
        scaleX * (FLOAT)dc->wndOrgX;
    xformWnd2Vport.eDy  = (FLOAT)dc->vportOrgY -
        scaleY * (FLOAT)dc->wndOrgY;

    /* Combine with the world transformation */
    CombineTransform( &dc->w.xformWorld2Vport, &dc->w.xformWorld2Wnd,
        &xformWnd2Vport );

    /* Create inverse of world-to-viewport transformation */
    dc->w.vport2WorldValid = DC_InvertXform( &dc->w.xformWorld2Vport,
        &dc->w.xformVport2World );
}


/***********************************************************************
 *           GetDCState    (GDI.179)
 */
HDC16 WINAPI GetDCState( HDC16 hdc )
{
    DC * newdc, * dc;
    HGDIOBJ16 handle;
    
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    if (!(handle = GDI_AllocObject( sizeof(DC), DC_MAGIC )))
    {
      GDI_HEAP_UNLOCK( hdc );
      return 0;
    }
    newdc = (DC *) GDI_HEAP_LOCK( handle );

    TRACE(dc, "(%04x): returning %04x\n", hdc, handle );

    memset( &newdc->u.x, 0, sizeof(newdc->u.x) );
    newdc->w.flags            = dc->w.flags | DC_SAVED;
    newdc->w.devCaps          = dc->w.devCaps;
    newdc->w.hPen             = dc->w.hPen;       
    newdc->w.hBrush           = dc->w.hBrush;     
    newdc->w.hFont            = dc->w.hFont;      
    newdc->w.hBitmap          = dc->w.hBitmap;    
    newdc->w.hFirstBitmap     = dc->w.hFirstBitmap;
    newdc->w.hDevice          = dc->w.hDevice;
    newdc->w.hPalette         = dc->w.hPalette;   
    newdc->w.bitsPerPixel     = dc->w.bitsPerPixel;
    newdc->w.ROPmode          = dc->w.ROPmode;
    newdc->w.polyFillMode     = dc->w.polyFillMode;
    newdc->w.stretchBltMode   = dc->w.stretchBltMode;
    newdc->w.relAbsMode       = dc->w.relAbsMode;
    newdc->w.backgroundMode   = dc->w.backgroundMode;
    newdc->w.backgroundColor  = dc->w.backgroundColor;
    newdc->w.textColor        = dc->w.textColor;
    newdc->w.backgroundPixel  = dc->w.backgroundPixel;
    newdc->w.textPixel        = dc->w.textPixel;
    newdc->w.brushOrgX        = dc->w.brushOrgX;
    newdc->w.brushOrgY        = dc->w.brushOrgY;
    newdc->w.textAlign        = dc->w.textAlign;
    newdc->w.charExtra        = dc->w.charExtra;
    newdc->w.breakTotalExtra  = dc->w.breakTotalExtra;
    newdc->w.breakCount       = dc->w.breakCount;
    newdc->w.breakExtra       = dc->w.breakExtra;
    newdc->w.breakRem         = dc->w.breakRem;
    newdc->w.MapMode          = dc->w.MapMode;
    newdc->w.GraphicsMode     = dc->w.GraphicsMode;
    newdc->w.DCOrgX           = dc->w.DCOrgX;
    newdc->w.DCOrgY           = dc->w.DCOrgY;
    newdc->w.CursPosX         = dc->w.CursPosX;
    newdc->w.CursPosY         = dc->w.CursPosY;
    newdc->w.ArcDirection     = dc->w.ArcDirection;
    newdc->w.xformWorld2Wnd   = dc->w.xformWorld2Wnd;
    newdc->w.xformWorld2Vport = dc->w.xformWorld2Vport;
    newdc->w.xformVport2World = dc->w.xformVport2World;
    newdc->w.vport2WorldValid = dc->w.vport2WorldValid;
    newdc->wndOrgX            = dc->wndOrgX;
    newdc->wndOrgY            = dc->wndOrgY;
    newdc->wndExtX            = dc->wndExtX;
    newdc->wndExtY            = dc->wndExtY;
    newdc->vportOrgX          = dc->vportOrgX;
    newdc->vportOrgY          = dc->vportOrgY;
    newdc->vportExtX          = dc->vportExtX;
    newdc->vportExtY          = dc->vportExtY;

    newdc->hSelf = (HDC32)handle;
    newdc->saveLevel = 0;

    PATH_InitGdiPath( &newdc->w.path );
    
    /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */

    newdc->w.hGCClipRgn = newdc->w.hVisRgn = 0;
    if (dc->w.hClipRgn)
    {
	newdc->w.hClipRgn = CreateRectRgn32( 0, 0, 0, 0 );
	CombineRgn32( newdc->w.hClipRgn, dc->w.hClipRgn, 0, RGN_COPY );
    }
    else
	newdc->w.hClipRgn = 0;
    GDI_HEAP_UNLOCK( handle );
    GDI_HEAP_UNLOCK( hdc );
    return handle;
}


/***********************************************************************
 *           SetDCState    (GDI.180)
 */
void WINAPI SetDCState( HDC16 hdc, HDC16 hdcs )
{
    DC *dc, *dcs;
    
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return;
    if (!(dcs = (DC *) GDI_GetObjPtr( hdcs, DC_MAGIC )))
    {
      GDI_HEAP_UNLOCK( hdc );
      return;
    }
    if (!dcs->w.flags & DC_SAVED)
    {
      GDI_HEAP_UNLOCK( hdc );
      GDI_HEAP_UNLOCK( hdcs );
      return;
    }
    TRACE(dc, "%04x %04x\n", hdc, hdcs );

    dc->w.flags            = dcs->w.flags & ~DC_SAVED;
    dc->w.devCaps          = dcs->w.devCaps;
    dc->w.hFirstBitmap     = dcs->w.hFirstBitmap;
    dc->w.hDevice          = dcs->w.hDevice;
    dc->w.ROPmode          = dcs->w.ROPmode;
    dc->w.polyFillMode     = dcs->w.polyFillMode;
    dc->w.stretchBltMode   = dcs->w.stretchBltMode;
    dc->w.relAbsMode       = dcs->w.relAbsMode;
    dc->w.backgroundMode   = dcs->w.backgroundMode;
    dc->w.backgroundColor  = dcs->w.backgroundColor;
    dc->w.textColor        = dcs->w.textColor;
    dc->w.backgroundPixel  = dcs->w.backgroundPixel;
    dc->w.textPixel        = dcs->w.textPixel;
    dc->w.brushOrgX        = dcs->w.brushOrgX;
    dc->w.brushOrgY        = dcs->w.brushOrgY;
    dc->w.textAlign        = dcs->w.textAlign;
    dc->w.charExtra        = dcs->w.charExtra;
    dc->w.breakTotalExtra  = dcs->w.breakTotalExtra;
    dc->w.breakCount       = dcs->w.breakCount;
    dc->w.breakExtra       = dcs->w.breakExtra;
    dc->w.breakRem         = dcs->w.breakRem;
    dc->w.MapMode          = dcs->w.MapMode;
    dc->w.GraphicsMode     = dcs->w.GraphicsMode;
    dc->w.DCOrgX           = dcs->w.DCOrgX;
    dc->w.DCOrgY           = dcs->w.DCOrgY;
    dc->w.CursPosX         = dcs->w.CursPosX;
    dc->w.CursPosY         = dcs->w.CursPosY;
    dc->w.ArcDirection     = dcs->w.ArcDirection;
    dc->w.xformWorld2Wnd   = dcs->w.xformWorld2Wnd;
    dc->w.xformWorld2Vport = dcs->w.xformWorld2Vport;
    dc->w.xformVport2World = dcs->w.xformVport2World;
    dc->w.vport2WorldValid = dcs->w.vport2WorldValid;

    dc->wndOrgX            = dcs->wndOrgX;
    dc->wndOrgY            = dcs->wndOrgY;
    dc->wndExtX            = dcs->wndExtX;
    dc->wndExtY            = dcs->wndExtY;
    dc->vportOrgX          = dcs->vportOrgX;
    dc->vportOrgY          = dcs->vportOrgY;
    dc->vportExtX          = dcs->vportExtX;
    dc->vportExtY          = dcs->vportExtY;

    if (!(dc->w.flags & DC_MEMORY)) dc->w.bitsPerPixel = dcs->w.bitsPerPixel;
    SelectClipRgn32( hdc, dcs->w.hClipRgn );

    SelectObject32( hdc, dcs->w.hBitmap );
    SelectObject32( hdc, dcs->w.hBrush );
    SelectObject32( hdc, dcs->w.hFont );
    SelectObject32( hdc, dcs->w.hPen );
    GDISelectPalette( hdc, dcs->w.hPalette, FALSE );
    GDI_HEAP_UNLOCK( hdc );
    GDI_HEAP_UNLOCK( hdcs );
}


/***********************************************************************
 *           SaveDC16    (GDI.30)
 */
INT16 WINAPI SaveDC16( HDC16 hdc )
{
    return (INT16)SaveDC32( hdc );
}


/***********************************************************************
 *           SaveDC32    (GDI32.292)
 */
INT32 WINAPI SaveDC32( HDC32 hdc )
{
    HDC32 hdcs;
    DC * dc, * dcs;
    INT32 ret;

    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
	MF_MetaParam0(dc, META_SAVEDC);
	GDI_HEAP_UNLOCK( hdc );
	return 1;  /* ?? */
    }
    if (!(hdcs = GetDCState( hdc )))
    {
      GDI_HEAP_UNLOCK( hdc );
      return 0;
    }
    dcs = (DC *) GDI_HEAP_LOCK( hdcs );

    /* Copy path. The reason why path saving / restoring is in SaveDC/
     * RestoreDC and not in GetDCState/SetDCState is that the ...DCState
     * functions are only in Win16 (which doesn't have paths) and that
     * SetDCState doesn't allow us to signal an error (which can happen
     * when copying paths).
     */
    if (!PATH_AssignGdiPath( &dcs->w.path, &dc->w.path ))
    {
        GDI_HEAP_UNLOCK( hdc );
	GDI_HEAP_UNLOCK( hdcs );
	DeleteDC32( hdcs );
	return 0;
    }
    
    dcs->header.hNext = dc->header.hNext;
    dc->header.hNext = hdcs;
    TRACE(dc, "(%04x): returning %d\n", hdc, dc->saveLevel+1 );
    ret = ++dc->saveLevel;
    GDI_HEAP_UNLOCK( hdcs );
    GDI_HEAP_UNLOCK( hdc );
    return ret;
}


/***********************************************************************
 *           RestoreDC16    (GDI.39)
 */
BOOL16 WINAPI RestoreDC16( HDC16 hdc, INT16 level )
{
    return RestoreDC32( hdc, level );
}


/***********************************************************************
 *           RestoreDC32    (GDI32.290)
 */
BOOL32 WINAPI RestoreDC32( HDC32 hdc, INT32 level )
{
    DC * dc, * dcs;
    BOOL32 success;

    TRACE(dc, "%04x %d\n", hdc, level );
    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	if (level != -1) 
	{
	  GDI_HEAP_UNLOCK( hdc );
	  return FALSE;
	}
	MF_MetaParam1(dc, META_RESTOREDC, level);
	GDI_HEAP_UNLOCK( hdc );
	return TRUE;
    }
    if (level == -1) level = dc->saveLevel;
    if ((level < 1) || (level > dc->saveLevel))
    {
      GDI_HEAP_UNLOCK( hdc );
      return FALSE;
    }
    
    success=TRUE;
    while (dc->saveLevel >= level)
    {
	HDC16 hdcs = dc->header.hNext;
	if (!(dcs = (DC *) GDI_GetObjPtr( hdcs, DC_MAGIC )))
	{
	  GDI_HEAP_UNLOCK( hdc );
	  return FALSE;
	}	
	dc->header.hNext = dcs->header.hNext;
	if (--dc->saveLevel < level)
	{
	    SetDCState( hdc, hdcs );
            if (!PATH_AssignGdiPath( &dc->w.path, &dcs->w.path ))
		/* FIXME: This might not be quite right, since we're
		 * returning FALSE but still destroying the saved DC state */
	        success=FALSE;
	}
	DeleteDC32( hdcs );
    }
    GDI_HEAP_UNLOCK( hdc );
    return success;
}


/***********************************************************************
 *           CreateDC16    (GDI.53)
 */
HDC16 WINAPI CreateDC16( LPCSTR driver, LPCSTR device, LPCSTR output,
                         const DEVMODE16 *initData )
{
    DC * dc;
    const DC_FUNCTIONS *funcs;

    if (!(funcs = DRIVER_FindDriver( driver ))) return 0;
    if (!(dc = DC_AllocDC( funcs ))) return 0;
    dc->w.flags = 0;

    TRACE(dc, "(%s %s %s): returning %04x\n",
               driver, device, output, dc->hSelf );

    if (dc->funcs->pCreateDC &&
        !dc->funcs->pCreateDC( dc, driver, device, output, initData ))
    {
        WARN(dc, "creation aborted by device\n" );
        GDI_HEAP_FREE( dc->hSelf );
        return 0;
    }

    DC_InitDC( dc );
    GDI_HEAP_UNLOCK( dc->hSelf );
    return dc->hSelf;
}


/***********************************************************************
 *           CreateDC32A    (GDI32.)
 */
HDC32 WINAPI CreateDC32A( LPCSTR driver, LPCSTR device, LPCSTR output,
                          const DEVMODE32A *initData )
{
    return CreateDC16( driver, device, output, (const DEVMODE16 *)initData );
}


/***********************************************************************
 *           CreateDC32W    (GDI32.)
 */
HDC32 WINAPI CreateDC32W( LPCWSTR driver, LPCWSTR device, LPCWSTR output,
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
HDC16 WINAPI CreateIC16( LPCSTR driver, LPCSTR device, LPCSTR output,
                         const DEVMODE16* initData )
{
      /* Nothing special yet for ICs */
    return CreateDC16( driver, device, output, initData );
}


/***********************************************************************
 *           CreateIC32A    (GDI32.49)
 */
HDC32 WINAPI CreateIC32A( LPCSTR driver, LPCSTR device, LPCSTR output,
                          const DEVMODE32A* initData )
{
      /* Nothing special yet for ICs */
    return CreateDC32A( driver, device, output, initData );
}


/***********************************************************************
 *           CreateIC32W    (GDI32.50)
 */
HDC32 WINAPI CreateIC32W( LPCWSTR driver, LPCWSTR device, LPCWSTR output,
                          const DEVMODE32W* initData )
{
      /* Nothing special yet for ICs */
    return CreateDC32W( driver, device, output, initData );
}


/***********************************************************************
 *           CreateCompatibleDC16    (GDI.52)
 */
HDC16 WINAPI CreateCompatibleDC16( HDC16 hdc )
{
    return (HDC16)CreateCompatibleDC32( hdc );
}


/***********************************************************************
 *           CreateCompatibleDC32   (GDI32.31)
 */
HDC32 WINAPI CreateCompatibleDC32( HDC32 hdc )
{
    DC *dc, *origDC;
    HBITMAP32 hbitmap;
    const DC_FUNCTIONS *funcs;

    if ((origDC = (DC *)GDI_GetObjPtr( hdc, DC_MAGIC ))) funcs = origDC->funcs;
    else funcs = DRIVER_FindDriver( "DISPLAY" );
    if (!funcs) return 0;

    if (!(dc = DC_AllocDC( funcs ))) return 0;

    TRACE(dc, "(%04x): returning %04x\n",
               hdc, dc->hSelf );

      /* Create default bitmap */
    if (!(hbitmap = CreateBitmap32( 1, 1, 1, 1, NULL )))
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
        WARN(dc, "creation aborted by device\n");
        DeleteObject32( hbitmap );
        GDI_HEAP_FREE( dc->hSelf );
        return 0;
    }

    DC_InitDC( dc );
    GDI_HEAP_UNLOCK( dc->hSelf );
    return dc->hSelf;
}


/***********************************************************************
 *           DeleteDC16    (GDI.68)
 */
BOOL16 WINAPI DeleteDC16( HDC16 hdc )
{
    return DeleteDC32( hdc );
}


/***********************************************************************
 *           DeleteDC32    (GDI32.67)
 */
BOOL32 WINAPI DeleteDC32( HDC32 hdc )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    TRACE(dc, "%04x\n", hdc );

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
    
    PATH_DestroyGdiPath(&dc->w.path);
    
    return GDI_FreeObject( hdc );
}


/***********************************************************************
 *           ResetDC16    (GDI.376)
 */
HDC16 WINAPI ResetDC16( HDC16 hdc, const DEVMODE16 *devmode )
{
    FIXME(dc, "stub\n" );
    return hdc;
}


/***********************************************************************
 *           ResetDC32A    (GDI32.287)
 */
HDC32 WINAPI ResetDC32A( HDC32 hdc, const DEVMODE32A *devmode )
{
    FIXME(dc, "stub\n" );
    return hdc;
}


/***********************************************************************
 *           ResetDC32W    (GDI32.288)
 */
HDC32 WINAPI ResetDC32W( HDC32 hdc, const DEVMODE32W *devmode )
{
    FIXME(dc, "stub\n" );
    return hdc;
}


/***********************************************************************
 *           GetDeviceCaps16    (GDI.80)
 */
INT16 WINAPI GetDeviceCaps16( HDC16 hdc, INT16 cap )
{
    return GetDeviceCaps32( hdc, cap );
}


/***********************************************************************
 *           GetDeviceCaps32    (GDI32.171)
 */
INT32 WINAPI GetDeviceCaps32( HDC32 hdc, INT32 cap )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    INT32 ret;

    if (!dc) return 0;

    if ((cap < 0) || (cap > sizeof(DeviceCaps)-sizeof(WORD)))
    {
      GDI_HEAP_UNLOCK( hdc );
      return 0;
    }
    
    TRACE(dc, "(%04x,%d): returning %d\n",
	    hdc, cap, *(WORD *)(((char *)dc->w.devCaps) + cap) );
    ret = *(WORD *)(((char *)dc->w.devCaps) + cap);
    GDI_HEAP_UNLOCK( hdc );
    return ret;
}


/***********************************************************************
 *           SetBkColor16    (GDI.1)
 */
COLORREF WINAPI SetBkColor16( HDC16 hdc, COLORREF color )
{
    return SetBkColor32( hdc, color );
}


/***********************************************************************
 *           SetBkColor32    (GDI32.305)
 */
COLORREF WINAPI SetBkColor32( HDC32 hdc, COLORREF color )
{
    COLORREF oldColor;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0x80000000;
	MF_MetaParam2(dc, META_SETBKCOLOR, HIWORD(color), LOWORD(color));
	GDI_HEAP_UNLOCK( hdc );
	return 0;  /* ?? */
    }

    oldColor = dc->w.backgroundColor;
    dc->w.backgroundColor = color;
    dc->w.backgroundPixel = COLOR_ToPhysical( dc, color );
    GDI_HEAP_UNLOCK( hdc );
    return oldColor;
}


/***********************************************************************
 *           SetTextColor16    (GDI.9)
 */
COLORREF WINAPI SetTextColor16( HDC16 hdc, COLORREF color )
{
    return SetTextColor32( hdc, color );
}


/***********************************************************************
 *           SetTextColor32    (GDI32.338)
 */
COLORREF WINAPI SetTextColor32( HDC32 hdc, COLORREF color )
{
    COLORREF oldColor;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0x80000000;
	MF_MetaParam2(dc, META_SETTEXTCOLOR, HIWORD(color), LOWORD(color));
	GDI_HEAP_UNLOCK( hdc );
	return 0;  /* ?? */
    }

    oldColor = dc->w.textColor;
    dc->w.textColor = color;
    dc->w.textPixel = COLOR_ToPhysical( dc, color );
    GDI_HEAP_UNLOCK( hdc );
    return oldColor;
}


/***********************************************************************
 *           SetTextAlign16    (GDI.346)
 */
UINT16 WINAPI SetTextAlign16( HDC16 hdc, UINT16 textAlign )
{
    return SetTextAlign32( hdc, textAlign );
}


/***********************************************************************
 *           SetTextAlign32    (GDI32.336)
 */
UINT32 WINAPI SetTextAlign32( HDC32 hdc, UINT32 textAlign )
{
    UINT32 prevAlign;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc)
    {
	if (!(dc = (DC *)GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC ))) return 0;
	MF_MetaParam1( dc, META_SETTEXTALIGN, textAlign );
	GDI_HEAP_UNLOCK( hdc );
	return 1;
    }
    prevAlign = dc->w.textAlign;
    dc->w.textAlign = textAlign;
    GDI_HEAP_UNLOCK( hdc );
    return prevAlign;
}


/***********************************************************************
 *           GetDCOrgEx  (GDI32.168)
 */
BOOL32 WINAPI GetDCOrgEx( HDC32 hDC, LPPOINT32 lpp )
{
    DC * dc;
    if (!lpp) return FALSE;
    if (!(dc = (DC *) GDI_GetObjPtr( hDC, DC_MAGIC ))) return FALSE;

    if (!(dc->w.flags & DC_MEMORY))
    {
       Window root;
       int w, h, border, depth;
       /* FIXME: this is not correct for managed windows */
       TSXGetGeometry( display, dc->u.x.drawable, &root,
                    &lpp->x, &lpp->y, &w, &h, &border, &depth );
    }
    else lpp->x = lpp->y = 0;
    lpp->x += dc->w.DCOrgX; lpp->y += dc->w.DCOrgY;
    GDI_HEAP_UNLOCK( hDC );
    return TRUE;
}


/***********************************************************************
 *           GetDCOrg    (GDI.79)
 */
DWORD WINAPI GetDCOrg( HDC16 hdc )
{
    POINT32	pt;
    if( GetDCOrgEx( hdc, &pt) )
  	return MAKELONG( (WORD)pt.x, (WORD)pt.y );    
    return 0;
}


/***********************************************************************
 *           SetDCOrg    (GDI.117)
 */
DWORD WINAPI SetDCOrg( HDC16 hdc, INT16 x, INT16 y )
{
    DWORD prevOrg;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    prevOrg = dc->w.DCOrgX | (dc->w.DCOrgY << 16);
    dc->w.DCOrgX = x;
    dc->w.DCOrgY = y;
    GDI_HEAP_UNLOCK( hdc );
    return prevOrg;
}


/***********************************************************************
 *           GetGraphicsMode    (GDI32.188)
 */
INT32 WINAPI GetGraphicsMode( HDC32 hdc )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    return dc->w.GraphicsMode;
}


/***********************************************************************
 *           SetGraphicsMode    (GDI32.317)
 */
INT32 WINAPI SetGraphicsMode( HDC32 hdc, INT32 mode )
{
    INT32 ret;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );

    /* One would think that setting the graphics mode to GM_COMPATIBLE
     * would also reset the world transformation matrix to the unity
     * matrix. However, in Windows, this is not the case. This doesn't
     * make a lot of sense to me, but that's the way it is.
     */
    
    if (!dc) return 0;
    if ((mode <= 0) || (mode > GM_LAST)) return 0;
    ret = dc->w.GraphicsMode;
    dc->w.GraphicsMode = mode;
    return ret;
}


/***********************************************************************
 *           GetArcDirection16    (GDI.524)
 */
INT16 WINAPI GetArcDirection16( HDC16 hdc )
{
    return GetArcDirection32( (HDC32)hdc );
}


/***********************************************************************
 *           GetArcDirection32    (GDI32.141)
 */
INT32 WINAPI GetArcDirection32( HDC32 hdc )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    
    if (!dc)
        return 0;

    return dc->w.ArcDirection;
}


/***********************************************************************
 *           SetArcDirection16    (GDI.525)
 */
INT16 WINAPI SetArcDirection16( HDC16 hdc, INT16 nDirection )
{
    return SetArcDirection32( (HDC32)hdc, (INT32)nDirection );
}


/***********************************************************************
 *           SetArcDirection32    (GDI32.302)
 */
INT32 WINAPI SetArcDirection32( HDC32 hdc, INT32 nDirection )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    INT32 nOldDirection;
    
    if (!dc)
        return 0;

    if (nDirection!=AD_COUNTERCLOCKWISE && nDirection!=AD_CLOCKWISE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }

    nOldDirection = dc->w.ArcDirection;
    dc->w.ArcDirection = nDirection;

    return nOldDirection;
}


/***********************************************************************
 *           GetWorldTransform    (GDI32.244)
 */
BOOL32 WINAPI GetWorldTransform( HDC32 hdc, LPXFORM xform )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    
    if (!dc)
        return FALSE;
    if (!xform)
	return FALSE;

    *xform = dc->w.xformWorld2Wnd;
    
    return TRUE;
}


/***********************************************************************
 *           SetWorldTransform    (GDI32.346)
 */
BOOL32 WINAPI SetWorldTransform( HDC32 hdc, const XFORM *xform )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    
    if (!dc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (!xform)
	return FALSE;
    
    /* Check that graphics mode is GM_ADVANCED */
    if (dc->w.GraphicsMode!=GM_ADVANCED)
       return FALSE;

    dc->w.xformWorld2Wnd = *xform;
    
    DC_UpdateXforms( dc );

    return TRUE;
}


/****************************************************************************
 * ModifyWorldTransform [GDI32.253]
 * Modifies the world transformation for a device context.
 *
 * PARAMS
 *    hdc   [I] Handle to device context
 *    xform [I] XFORM structure that will be used to modify the world
 *              transformation
 *    iMode [I] Specifies in what way to modify the world transformation
 *              Possible values:
 *              MWT_IDENTITY
 *                 Resets the world transformation to the identity matrix.
 *                 The parameter xform is ignored.
 *              MWT_LEFTMULTIPLY
 *                 Multiplies xform into the world transformation matrix from
 *                 the left.
 *              MWT_RIGHTMULTIPLY
 *                 Multiplies xform into the world transformation matrix from
 *                 the right.
 *
 * RETURNS STD
 */
BOOL32 WINAPI ModifyWorldTransform( HDC32 hdc, const XFORM *xform,
    DWORD iMode )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    
    /* Check for illegal parameters */
    if (!dc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (!xform)
	return FALSE;
    
    /* Check that graphics mode is GM_ADVANCED */
    if (dc->w.GraphicsMode!=GM_ADVANCED)
       return FALSE;
       
    switch (iMode)
    {
        case MWT_IDENTITY:
	    dc->w.xformWorld2Wnd.eM11 = 1.0f;
	    dc->w.xformWorld2Wnd.eM12 = 0.0f;
	    dc->w.xformWorld2Wnd.eM21 = 0.0f;
	    dc->w.xformWorld2Wnd.eM22 = 1.0f;
	    dc->w.xformWorld2Wnd.eDx  = 0.0f;
	    dc->w.xformWorld2Wnd.eDy  = 0.0f;
	    break;
        case MWT_LEFTMULTIPLY:
	    CombineTransform( &dc->w.xformWorld2Wnd, xform,
	        &dc->w.xformWorld2Wnd );
	    break;
	case MWT_RIGHTMULTIPLY:
	    CombineTransform( &dc->w.xformWorld2Wnd, &dc->w.xformWorld2Wnd,
	        xform );
	    break;
        default:
	    return FALSE;
    }

    DC_UpdateXforms( dc );

    return TRUE;
}


/****************************************************************************
 * CombineTransform [GDI32.20]
 * Combines two transformation matrices.
 *
 * PARAMS
 *    xformResult [O] Stores the result of combining the two matrices
 *    xform1      [I] Specifies the first matrix to apply
 *    xform2      [I] Specifies the second matrix to apply
 *
 * REMARKS
 *    The same matrix can be passed in for more than one of the parameters.
 *
 * RETURNS STD
 */
BOOL32 WINAPI CombineTransform( LPXFORM xformResult, const XFORM *xform1,
    const XFORM *xform2 )
{
    XFORM xformTemp;
    
    /* Check for illegal parameters */
    if (!xformResult || !xform1 || !xform2)
        return FALSE;

    /* Create the result in a temporary XFORM, since xformResult may be
     * equal to xform1 or xform2 */
    xformTemp.eM11 = xform1->eM11 * xform2->eM11 +
                     xform1->eM12 * xform2->eM21;
    xformTemp.eM12 = xform1->eM11 * xform2->eM12 +
                     xform1->eM12 * xform2->eM22;
    xformTemp.eM21 = xform1->eM21 * xform2->eM11 +
                     xform1->eM22 * xform2->eM21;
    xformTemp.eM22 = xform1->eM21 * xform2->eM12 +
                     xform1->eM22 * xform2->eM22;
    xformTemp.eDx  = xform1->eDx  * xform2->eM11 +
                     xform1->eDy  * xform2->eM21 +
                     xform2->eDx;
    xformTemp.eDy  = xform1->eDx  * xform2->eM12 +
                     xform1->eDy  * xform2->eM22 +
                     xform2->eDy;

    /* Copy the result to xformResult */
    *xformResult = xformTemp;

    return TRUE;
}


/***********************************************************************
 *           SetDCHook   (GDI.190)
 */
BOOL16 WINAPI SetDCHook( HDC16 hdc, FARPROC16 hookProc, DWORD dwHookData )
{
    DC *dc = (DC *)GDI_GetObjPtr( hdc, DC_MAGIC );

    TRACE(dc, "hookProc %08x, default is %08x\n",
                (UINT32)hookProc, (UINT32)DCHook );

    if (!dc) return FALSE;
    dc->hookProc = hookProc;
    dc->dwHookData = dwHookData;
    GDI_HEAP_UNLOCK( hdc );
    return TRUE;
}


/***********************************************************************
 *           GetDCHook   (GDI.191)
 */
DWORD WINAPI GetDCHook( HDC16 hdc, FARPROC16 *phookProc )
{
    DC *dc = (DC *)GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    *phookProc = dc->hookProc;
    GDI_HEAP_UNLOCK( hdc );
    return dc->dwHookData;
}


/***********************************************************************
 *           SetHookFlags       (GDI.192)
 */
WORD WINAPI SetHookFlags(HDC16 hDC, WORD flags)
{
    DC* dc = (DC*)GDI_GetObjPtr( hDC, DC_MAGIC );

    if( dc )
    {
        WORD wRet = dc->w.flags & DC_DIRTY;

        /* "Undocumented Windows" info is slightly confusing.
         */

        TRACE(dc,"hDC %04x, flags %04x\n",hDC,flags);

        if( flags & DCHF_INVALIDATEVISRGN )
            dc->w.flags |= DC_DIRTY;
        else if( flags & DCHF_VALIDATEVISRGN || !flags )
            dc->w.flags &= ~DC_DIRTY;
	GDI_HEAP_UNLOCK( hDC );
        return wRet;
    }
    return 0;
}

