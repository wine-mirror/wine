/*
 * GDI bit-blit operations
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

#include "gdi.h"

extern const int DC_XROPfunction[];

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))


/***********************************************************************
 *           PatBlt    (GDI.29)
 */
BOOL PatBlt( HDC hdc, short left, short top,
	     short width, short height, DWORD rop)
{
    int x1, x2, y1, y2;
    
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
#ifdef DEBUG_GDI
    printf( "PatBlt: %d %d,%d %dx%d %06x\n",
	    hdc, left, top, width, height, rop );
#endif

    rop >>= 16;
    if (!DC_SetupGCForBrush( dc )) rop &= 0x0f;
    else rop = (rop & 0x03) | ((rop >> 4) & 0x0c);
    XSetFunction( XT_display, dc->u.x.gc, DC_XROPfunction[rop] );

    x1 = XLPTODP( dc, left );
    x2 = XLPTODP( dc, left + width );
    y1 = YLPTODP( dc, top );
    y2 = YLPTODP( dc, top + height );
    XFillRectangle( XT_display, dc->u.x.drawable, dc->u.x.gc,
		   MIN(x1,x2), MIN(y1,y2), abs(x2-x1), abs(y2-y1) );
    return TRUE;
}


/***********************************************************************
 *           BitBlt    (GDI.34)
 */
BOOL BitBlt( HDC hdcDest, short xDest, short yDest, short width, short height,
	     HDC hdcSrc, short xSrc, short ySrc, DWORD rop )
{
    int xs1, xs2, ys1, ys2;
    int xd1, xd2, yd1, yd2;
    DC *dcDest, *dcSrc;

#ifdef DEBUG_GDI    
    printf( "BitBlt: %d %d,%d %dx%d %d %d,%d %08x\n",
	   hdcDest, xDest, yDest, width, height, hdcSrc, xSrc, ySrc, rop );
#endif

    if ((rop & 0xcc0000) == ((rop & 0x330000) << 2))
	return PatBlt( hdcDest, xDest, yDest, width, height, rop );

    rop >>= 16;
    if ((rop & 0x0f) != (rop >> 4))
    {
	printf( "BitBlt: Unimplemented ROP %02x\n", rop );
	return FALSE;
    }
    
    dcDest = (DC *) GDI_GetObjPtr( hdcDest, DC_MAGIC );
    if (!dcDest) return FALSE;
    dcSrc = (DC *) GDI_GetObjPtr( hdcSrc, DC_MAGIC );
    if (!dcSrc) return FALSE;

    xs1 = XLPTODP( dcSrc, xSrc );
    xs2 = XLPTODP( dcSrc, xSrc + width );
    ys1 = YLPTODP( dcSrc, ySrc );
    ys2 = YLPTODP( dcSrc, ySrc + height );
    xd1 = XLPTODP( dcDest, xDest );
    xd2 = XLPTODP( dcDest, xDest + width );
    yd1 = YLPTODP( dcDest, yDest );
    yd2 = YLPTODP( dcDest, yDest + height );

    if ((abs(xs2-xs1) != abs(xd2-xd1)) || (abs(ys2-ys1) != abs(yd2-yd1)))
	return FALSE;  /* Should call StretchBlt here */
    
    DC_SetupGCForText( dcDest );
    XSetFunction( XT_display, dcDest->u.x.gc, DC_XROPfunction[rop & 0x0f] );
    if (dcSrc->w.bitsPerPixel == dcDest->w.bitsPerPixel)
    {
	XCopyArea( XT_display, dcSrc->u.x.drawable,
		   dcDest->u.x.drawable, dcDest->u.x.gc,
		   MIN(xs1,xs2), MIN(ys1,ys2), abs(xs2-xs1), abs(ys2-ys1),
		   MIN(xd1,xd2), MIN(yd1,yd2) );
    }
    else
    {
	if (dcSrc->w.bitsPerPixel != 1) return FALSE;
	XCopyPlane( XT_display, dcSrc->u.x.drawable,
		    dcDest->u.x.drawable, dcDest->u.x.gc,
		    MIN(xs1,xs2), MIN(ys1,ys2), abs(xs2-xs1), abs(ys2-ys1),
		    MIN(xd1,xd2), MIN(yd1,yd2), 1 );
    }
    return TRUE;
}
