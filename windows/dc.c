/*
 * GDI Device Context functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdlib.h>
#include <X11/Intrinsic.h>

#include "gdi.h"

extern HBITMAP BITMAP_hbitmapMemDC;

static DeviceCaps * displayDevCaps = NULL;

extern const WIN_DC_INFO DCVAL_defaultValues;


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
    caps->horzSize      = WidthMMOfScreen( XT_screen );
    caps->vertSize      = HeightMMOfScreen( XT_screen );
    caps->horzRes       = WidthOfScreen( XT_screen );
    caps->vertRes       = HeightOfScreen( XT_screen );
    caps->bitsPixel     = DefaultDepthOfScreen( XT_screen );
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
    caps->sizePalette   = DefaultVisual( XT_display, DefaultScreen(XT_display) )->map_entries;
    caps->numReserved   = 0;
    caps->colorRes      = 0;
}


/***********************************************************************
 *           DC_SetDeviceInfo
 *
 * Set device-specific info from device-independent info.
 */
void DC_SetDeviceInfo( HDC hdc, DC * dc )
{
    SetTextColor( hdc, dc->w.textColor );
    SetBkColor( hdc, dc->w.backgroundColor );
    SetROP2( hdc, dc->w.ROPmode );
    SelectObject( hdc, dc->w.hPen );
    SelectObject( hdc, dc->w.hBrush );
    SelectObject( hdc, dc->w.hFont );
    
    if (dc->w.hGCClipRgn)
    {
	RGNOBJ *obj = (RGNOBJ *) GDI_GetObjPtr(dc->w.hGCClipRgn, REGION_MAGIC);
	XSetClipMask( XT_display, dc->u.x.gc, obj->region.pixmap );
	XSetClipOrigin( XT_display, dc->u.x.gc,
		        obj->region.box.left, obj->region.box.top );
    }
    else
    {
	XSetClipMask( XT_display, dc->u.x.gc, None );
	XSetClipOrigin( XT_display, dc->u.x.gc, 0, 0 );
    }
}


/***********************************************************************
 *           DC_SetupDCForBrush
 *
 * Setup dc->u.x.gc for drawing operations using current brush.
 * Return 0 if brush is BS_NULL, 1 otherwise.
 */
int DC_SetupGCForBrush( DC * dc )
{
    if (dc->u.x.brush.style == BS_NULL) return 0;
    if (dc->u.x.brush.pixel == -1)
    {
	/* Special case used for monochrome pattern brushes.
	 * We need to swap foreground and background because
	 * Windows does it the wrong way...
	 */
	XSetForeground( XT_display, dc->u.x.gc, dc->w.backgroundPixel );
	XSetBackground( XT_display, dc->u.x.gc, dc->w.textPixel );
    }
    else
    {
	XSetForeground( XT_display, dc->u.x.gc, dc->u.x.brush.pixel );
	XSetBackground( XT_display, dc->u.x.gc, dc->w.backgroundPixel );
    }

    if (dc->u.x.brush.fillStyle != FillStippled)
	XSetFillStyle( XT_display, dc->u.x.gc, dc->u.x.brush.fillStyle );
    else
	XSetFillStyle( XT_display, dc->u.x.gc,
		       (dc->w.backgroundMode == OPAQUE) ? 
		          FillOpaqueStippled : FillStippled );
    XSetTSOrigin( XT_display, dc->u.x.gc, dc->w.brushOrgX, dc->w.brushOrgY );
    XSetFunction( XT_display, dc->u.x.gc, DC_XROPfunction[dc->w.ROPmode-1] );
    return 1;
}


/***********************************************************************
 *           DC_SetupDCForPen
 *
 * Setup dc->u.x.gc for drawing operations using current pen.
 * Return 0 if pen is PS_NULL, 1 otherwise.
 */
int DC_SetupGCForPen( DC * dc )
{
    if (dc->u.x.pen.style == PS_NULL) return 0;
    XSetForeground( XT_display, dc->u.x.gc, dc->u.x.pen.pixel );
    XSetBackground( XT_display, dc->u.x.gc, dc->w.backgroundPixel );
    XSetFillStyle( XT_display, dc->u.x.gc, FillSolid );
    if ((dc->u.x.pen.style == PS_SOLID) ||
	(dc->u.x.pen.style == PS_INSIDEFRAME))
    {
	XSetLineAttributes( XT_display, dc->u.x.gc, dc->u.x.pen.width, 
			    LineSolid, CapRound, JoinBevel );    
    }
    else
    {
    	XSetLineAttributes( XT_display, dc->u.x.gc, dc->u.x.pen.width, 
	     (dc->w.backgroundMode == OPAQUE) ? LineDoubleDash : LineOnOffDash,
	     CapRound, JoinBevel );
    }
    XSetFunction( XT_display, dc->u.x.gc, DC_XROPfunction[dc->w.ROPmode-1] );
    return 1;    
}


/***********************************************************************
 *           DC_SetupDCForText
 *
 * Setup dc->u.x.gc for text drawing operations.
 * Return 0 if the font is null, 1 otherwise.
 */
int DC_SetupGCForText( DC * dc )
{
    XSetForeground( XT_display, dc->u.x.gc, dc->w.textPixel );
    XSetBackground( XT_display, dc->u.x.gc, dc->w.backgroundPixel );
    XSetFillStyle( XT_display, dc->u.x.gc, FillSolid );
    XSetFunction( XT_display, dc->u.x.gc, DC_XROPfunction[dc->w.ROPmode-1] );
    if (!dc->u.x.font.fstruct) return 0;
    XSetFont( XT_display, dc->u.x.gc, dc->u.x.font.fstruct->fid );
    return 1;
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

#ifdef DEBUG_DC
    printf( "GetDCState(%d): returning %d\n", hdc, handle );
#endif

    memcpy( &newdc->w, &dc->w, sizeof(dc->w) );
    newdc->saveLevel = 0;
    newdc->w.flags |= DC_SAVED;

    if (dc->w.hClipRgn)
    {
	newdc->w.hClipRgn = CreateRectRgn( 0, 0, 0, 0 );
	CombineRgn( newdc->w.hClipRgn, dc->w.hClipRgn, 0, RGN_COPY );
    }
    if (dc->w.hVisRgn)
    {
	newdc->w.hVisRgn = CreateRectRgn( 0, 0, 0, 0 );
	CombineRgn( newdc->w.hVisRgn, dc->w.hVisRgn, 0, RGN_COPY );
	
    }
    newdc->w.hGCClipRgn = 0;
    return handle;
}


/***********************************************************************
 *           SetDCState    (GDI.180)
 */
void SetDCState( HDC hdc, HDC hdcs )
{
    DC * dc, * dcs;
    
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return;
    if (!(dcs = (DC *) GDI_GetObjPtr( hdcs, DC_MAGIC ))) return;
    if (!dcs->w.flags & DC_SAVED) return;
#ifdef DEBUG_DC
    printf( "SetDCState: %d %d\n", hdc, hdcs );
#endif
    if (dc->w.hClipRgn)	DeleteObject( dc->w.hClipRgn );
    if (dc->w.hVisRgn) DeleteObject( dc->w.hVisRgn );
    if (dc->w.hGCClipRgn) DeleteObject( dc->w.hGCClipRgn );
    memcpy( &dc->w, &dcs->w, sizeof(dc->w) );
    dc->w.hClipRgn = dc->w.hVisRgn = dc->w.hGCClipRgn = 0;
    dc->w.flags &= ~DC_SAVED;
    DC_SetDeviceInfo( hdc, dc );
    SelectClipRgn( hdc, dcs->w.hClipRgn );
    SelectVisRgn( hdc, dcs->w.hGCClipRgn );
}


/***********************************************************************
 *           SaveDC    (GDI.30)
 */
int SaveDC( HDC hdc )
{
    HDC hdcs;
    DC * dc, * dcs;

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    if (!(hdcs = GetDCState( hdc ))) return 0;
    dcs = (DC *) GDI_HEAP_ADDR( hdcs );
    dcs->header.hNext = dc->header.hNext;
    dc->header.hNext = hdcs;
#ifdef DEBUG_DC
    printf( "SaveDC(%d): returning %d\n", hdc, dc->saveLevel+1 );
#endif    
    return ++dc->saveLevel;
}


/***********************************************************************
 *           RestoreDC    (GDI.39)
 */
BOOL RestoreDC( HDC hdc, short level )
{
    DC * dc, * dcs;

#ifdef DEBUG_DC
    printf( "RestoreDC: %d %d\n", hdc, level );
#endif    
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return FALSE;
    if (level == -1) level = dc->saveLevel;
    if ((level < 1) || (level > dc->saveLevel))	return FALSE;
    
    while (dc->saveLevel >= level)
    {
	HDC hdcs = dc->header.hNext;
	if (!(dcs = (DC *) GDI_GetObjPtr( hdcs, DC_MAGIC ))) return FALSE;
	dc->header.hNext = dcs->header.hNext;
	if (--dc->saveLevel < level) SetDCState( hdc, hdcs );
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

#ifdef DEBUG_DC
    printf( "CreateDC(%s %s %s): returning %d\n", driver, device, output, handle );
#endif

    if (!displayDevCaps)
    {
	displayDevCaps = (DeviceCaps *) malloc( sizeof(DeviceCaps) );
	DC_FillDevCaps( displayDevCaps );
    }

    dc->saveLevel = 0;
    memcpy( &dc->w, &DCVAL_defaultValues, sizeof(DCVAL_defaultValues) );
    memset( &dc->u.x, 0, sizeof(dc->u.x) );

    dc->u.x.drawable   = DefaultRootWindow( XT_display );
    dc->u.x.gc         = XCreateGC( XT_display, dc->u.x.drawable, 0, NULL );
    dc->w.flags        = 0;
    dc->w.devCaps      = displayDevCaps;
    dc->w.planes       = displayDevCaps->planes;
    dc->w.bitsPerPixel = displayDevCaps->bitsPixel;

    XSetSubwindowMode( XT_display, dc->u.x.gc, IncludeInferiors );    
    DC_SetDeviceInfo( handle, dc );

    return handle;
}


/***********************************************************************
 *           CreateCompatibleDC    (GDI.52)
 */
HDC CreateCompatibleDC( HDC hdc )
{
    DC * dc;
    HANDLE handle;
    
    handle = GDI_AllocObject( sizeof(DC), DC_MAGIC );
    if (!handle) return 0;
    dc = (DC *) GDI_HEAP_ADDR( handle );

#ifdef DEBUG_DC
    printf( "CreateCompatibleDC(%d): returning %d\n", hdc, handle );
#endif

    dc->saveLevel = 0;
    memcpy( &dc->w, &DCVAL_defaultValues, sizeof(DCVAL_defaultValues) );
    memset( &dc->u.x, 0, sizeof(dc->u.x) );

    dc->u.x.drawable   = XCreatePixmap( XT_display,
				        DefaultRootWindow( XT_display ),
				        1, 1, 1 );
    dc->u.x.gc         = XCreateGC( XT_display, dc->u.x.drawable, 0, NULL );
    dc->w.flags        = DC_MEMORY;
    dc->w.planes       = 1;
    dc->w.bitsPerPixel = 1;
    dc->w.devCaps      = displayDevCaps;

    SelectObject( handle, BITMAP_hbitmapMemDC );
    DC_SetDeviceInfo( handle, dc );
    
    return handle;
}


/***********************************************************************
 *           DeleteDC    (GDI.68)
 */
BOOL DeleteDC( HDC hdc )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

#ifdef DEBUG_DC
    printf( "DeleteDC: %d\n", hdc );
#endif

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
	
	XFreeGC( XT_display, dc->u.x.gc );
	if (dc->w.flags & DC_MEMORY) BITMAP_UnselectBitmap( dc );
    }
    
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
    
#ifdef DEBUG_DC
    printf( "GetDeviceCaps(%d,%d): returning %d\n",
	    hdc, cap, *(WORD *)(((char *)dc->w.devCaps) + cap) );
#endif
    return *(WORD *)(((char *)dc->w.devCaps) + cap);
}


/***********************************************************************
 *           SetBkColor    (GDI.1)
 */
COLORREF SetBkColor( HDC hdc, COLORREF color )
{
    COLORREF oldColor;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0x80000000;

    oldColor = dc->w.backgroundColor;
    dc->w.backgroundColor = color;
    dc->w.backgroundPixel = GetNearestPaletteIndex( dc->w.hPalette, color );
    XSetBackground( XT_display, dc->u.x.gc, dc->w.backgroundPixel );
    return oldColor;
}


/***********************************************************************
 *           SetTextColor    (GDI.9)
 */
COLORREF SetTextColor( HDC hdc, COLORREF color )
{
    COLORREF oldColor;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0x80000000;

    oldColor = dc->w.textColor;
    dc->w.textColor = color;
    dc->w.textPixel = GetNearestPaletteIndex( dc->w.hPalette, color );
    XSetForeground( XT_display, dc->u.x.gc, dc->w.textPixel );
    return oldColor;
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
