/*
 * X-specific shortcuts to speed up WM code.
 * No coordinate transformations except origin translation.
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <assert.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include "graphics.h"
#include "color.h"
#include "bitmap.h"
#include "gdi.h"
#include "dc.h"

#define MAX_DRAWLINES 8


/**********************************************************************
 *          GRAPH_DrawLines
 *
 * Draw multiple unconnected lines (limited by MAX_DRAWLINES).
 */
BOOL32 GRAPH_DrawLines( HDC32 hdc, LPPOINT32 pXY, INT32 N, HPEN32 hPen )
{
    BOOL32	bRet = FALSE;
    DC*         dc;

    assert( N <= MAX_DRAWLINES );
    if( (dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC )) )
    {
	HPEN32  hPrevPen  = 0;

	if( hPen ) hPrevPen = SelectObject32( hdc, hPen );
	if( DC_SetupGCForPen( dc ) )
	{
	    XSegment	l[MAX_DRAWLINES];
	    INT32 	i, j;

	    for( i = 0; i < N; i++ )
	    {
		 j = 2 * i;
		 l[i].x1 = pXY[j].x + dc->w.DCOrgX; 
		 l[i].x2 = pXY[j + 1].x + dc->w.DCOrgX;
		 l[i].y1 = pXY[j].y + dc->w.DCOrgY;
		 l[i].y2 = pXY[j + 1].y + dc->w.DCOrgY;
	    }
	    XDrawSegments( display, dc->u.x.drawable, dc->u.x.gc, l, N );
	    bRet = TRUE;
	}
	if( hPrevPen ) SelectObject32( hdc, hPrevPen );
    }
    return bRet;
}

/**********************************************************************
 * 
 *          GRAPH_DrawBitmap
 *
 * Short-cut function to blit a bitmap into a device.
 * Faster than CreateCompatibleDC() + SelectBitmap() + BitBlt() + DeleteDC().
 */
BOOL32 GRAPH_DrawBitmap( HDC32 hdc, HBITMAP32 hbitmap, 
			 INT32 xdest, INT32 ydest, INT32 xsrc, INT32 ysrc, 
                         INT32 width, INT32 height, BOOL32 bMono )
{
    BITMAPOBJ *bmp;
    DC *dc;

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return FALSE;
    if (!(bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC )))
        return FALSE;

    xdest += dc->w.DCOrgX; ydest += dc->w.DCOrgY;

    XSetFunction( display, dc->u.x.gc, GXcopy );
    if (bmp->bitmap.bmBitsPixel == 1)
    {
        XSetForeground( display, dc->u.x.gc, dc->w.backgroundPixel );
        XSetBackground( display, dc->u.x.gc, dc->w.textPixel );
        XCopyPlane( display, bmp->pixmap, dc->u.x.drawable, dc->u.x.gc,
                    xsrc, ysrc, width, height, xdest, ydest, 1 );
    }
    else if (bmp->bitmap.bmBitsPixel == dc->w.bitsPerPixel)
    {
	if( bMono )
	{
	    INT32	plane;

	    if( COLOR_GetMonoPlane(&plane) )
	    {
		XSetForeground( display, dc->u.x.gc, dc->w.backgroundPixel );
		XSetBackground( display, dc->u.x.gc, dc->w.textPixel );
	    }
	    else
	    {
		XSetForeground( display, dc->u.x.gc, dc->w.textPixel );
		XSetBackground( display, dc->u.x.gc, dc->w.backgroundPixel );
	    }
	    XCopyPlane( display, bmp->pixmap, dc->u.x.drawable, dc->u.x.gc,
			xsrc, ysrc, width, height, xdest, ydest, plane );
	}
	else 
	    XCopyArea( display, bmp->pixmap, dc->u.x.drawable, 
		       dc->u.x.gc, xsrc, ysrc, width, height, xdest, ydest );
    }
    else 
    {
      GDI_HEAP_UNLOCK( hbitmap );
      return FALSE;
    }

    GDI_HEAP_UNLOCK( hbitmap );
    return TRUE;
}


/**********************************************************************
 *          GRAPH_DrawReliefRect
 *
 * Used in the standard control code for button edge drawing.
 */
void GRAPH_DrawReliefRect( HDC32 hdc, const RECT32 *rect, INT32 highlight_size,
                           INT32 shadow_size, BOOL32 pressed )
{
    if(pressed)
	GRAPH_DrawGenericReliefRect(hdc, rect, highlight_size, shadow_size,
				    GetSysColorBrush32(COLOR_BTNSHADOW),
				    GetSysColorBrush32(COLOR_BTNHIGHLIGHT));
    else
	GRAPH_DrawGenericReliefRect(hdc, rect, highlight_size, shadow_size,
				    GetSysColorBrush32(COLOR_BTNHIGHLIGHT),
				    GetSysColorBrush32(COLOR_BTNSHADOW));

    return;
}


/******************************************************************************
 *             GRAPH_DrawGenericReliefRect
 *
 *   Creates a rectangle with the specified highlight and shadow colors.
 *   Adapted from DrawReliefRect (which is somewhat misnamed).
 */
void  GRAPH_DrawGenericReliefRect(
    HDC32  hdc,
    const  RECT32 *rect,
    INT32  highlight_size,
    INT32  shadow_size,
    HBRUSH32  highlight,
    HBRUSH32  shadow )
{
    DC*		dc;
    HBRUSH32 	hPrevBrush;
    INT32 	w, h;
    RECT32	r = *rect;

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return;

    OffsetRect32( &r, dc->w.DCOrgX, dc->w.DCOrgY);
    h = rect->bottom - rect->top;  w = rect->right - rect->left;

    hPrevBrush = SelectObject32(hdc, highlight);

    if ( DC_SetupGCForBrush( dc ) )
    {
         INT32	i;

	 XSetFunction( display, dc->u.x.gc, GXcopy );
         for (i = 0; i < highlight_size; i++)
         {
	      XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
					r.left + i, r.top, 1, h - i );
	      XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
					r.left, r.top + i, w - i, 1 );
         }
    }

    SelectObject32( hdc, shadow );
    if ( DC_SetupGCForBrush( dc ) )
    {
	 INT32	i;

	 XSetFunction( display, dc->u.x.gc, GXcopy );
         for (i = 0; i < shadow_size; i++)
         {
	      XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
			      r.right - i - 1, r.top + i, 1, h - i );
	      XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
			      r.left + i, r.bottom - i - 1, w - i, 1 );
         }
    }

    SelectObject32( hdc, hPrevBrush );
}



/**********************************************************************
 *          GRAPH_DrawRectangle
 */
void GRAPH_DrawRectangle( HDC32 hdc, INT32 x, INT32 y, 
				     INT32 w, INT32 h, HPEN32 hPen )
{
    DC* dc;

    if( (dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC )) ) 
    {
        HPEN32	hPrevPen  = 0; 

	if( hPen ) hPrevPen = SelectObject32( hdc, hPen );
	if( DC_SetupGCForPen( dc ) )
	    XDrawRectangle( display, dc->u.x.drawable, dc->u.x.gc, 
			    x + dc->w.DCOrgX, y + dc->w.DCOrgY, w - 1, h - 1);
	if( hPrevPen ) SelectObject32( hdc, hPrevPen );
    }
}

/**********************************************************************
 *          GRAPH_SelectClipMask
 */
BOOL32 GRAPH_SelectClipMask( HDC32 hdc, HBITMAP32 hMonoBitmap, INT32 x, INT32 y)
{
    BITMAPOBJ *bmp = NULL;
    DC *dc;

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return FALSE;
    if ( hMonoBitmap ) 
    {
       if ( !(bmp = (BITMAPOBJ *) GDI_GetObjPtr( hMonoBitmap, BITMAP_MAGIC)) 
	   || bmp->bitmap.bmBitsPixel != 1 ) return FALSE;
	  
       XSetClipOrigin( display, dc->u.x.gc, dc->w.DCOrgX + x, dc->w.DCOrgY + y);
    }

    XSetClipMask( display, dc->u.x.gc, (bmp) ? bmp->pixmap : None );

    GDI_HEAP_UNLOCK( hMonoBitmap );
    return TRUE;
}

