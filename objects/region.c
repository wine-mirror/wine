/*
 * GDI region objects. Shamelessly ripped out from the X11 distribution
 * Thanks for the nice licence.
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 * Modifications and additions: Copyright 1998 Huw Davies
 *					  1999 Alex Korobka
 *
 */

/************************************************************************

Copyright (c) 1987, 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/
/*
 * The functions in this file implement the Region abstraction, similar to one
 * used in the X11 sample server. A Region is simply an area, as the name
 * implies, and is implemented as a "y-x-banded" array of rectangles. To
 * explain: Each Region is made up of a certain number of rectangles sorted
 * by y coordinate first, and then by x coordinate.
 *
 * Furthermore, the rectangles are banded such that every rectangle with a
 * given upper-left y coordinate (y1) will have the same lower-right y
 * coordinate (y2) and vice versa. If a rectangle has scanlines in a band, it
 * will span the entire vertical distance of the band. This means that some
 * areas that could be merged into a taller rectangle will be represented as
 * several shorter rectangles to account for shorter rectangles to its left
 * or right but within its "vertical scope".
 *
 * An added constraint on the rectangles is that they must cover as much
 * horizontal area as possible. E.g. no two rectangles in a band are allowed
 * to touch.
 *
 * Whenever possible, bands will be merged together to cover a greater vertical
 * distance (and thus reduce the number of rectangles). Two bands can be merged
 * only if the bottom of one touches the top of the other and they have
 * rectangles in the same places (of the same width, of course). This maintains
 * the y-x-banding that's so nice to have...
 */

#include <stdlib.h>
#include <string.h>
#include "region.h"
#include "winuser.h"
#include "debugtools.h"
#include "heap.h"
#include "dc.h"

DEFAULT_DEBUG_CHANNEL(region) 

typedef void (*voidProcp)();

/* Note the parameter order is different from the X11 equivalents */

static void REGION_CopyRegion(WINEREGION *d, WINEREGION *s);
static void REGION_IntersectRegion(WINEREGION *d, WINEREGION *s1, WINEREGION *s2);
static void REGION_UnionRegion(WINEREGION *d, WINEREGION *s1, WINEREGION *s2);
static void REGION_SubtractRegion(WINEREGION *d, WINEREGION *s1, WINEREGION *s2);
static void REGION_XorRegion(WINEREGION *d, WINEREGION *s1, WINEREGION *s2);
static void REGION_UnionRectWithRegion(const RECT *rect, WINEREGION *rgn);

#define RGN_DEFAULT_RECTS	2

/***********************************************************************
 *            REGION_DumpRegion
 *            Outputs the contents of a WINEREGION
 */
static void REGION_DumpRegion(WINEREGION *pReg)
{
    RECT *pRect, *pRectEnd = pReg->rects + pReg->numRects;

    TRACE("Region %p: %d,%d - %d,%d %d rects\n", pReg,
	    pReg->extents.left, pReg->extents.top,
	    pReg->extents.right, pReg->extents.bottom, pReg->numRects);
    for(pRect = pReg->rects; pRect < pRectEnd; pRect++)
        TRACE("\t%d,%d - %d,%d\n", pRect->left, pRect->top,
		       pRect->right, pRect->bottom);
    return;
}


/***********************************************************************
 *            REGION_AllocWineRegion
 *            Create a new empty WINEREGION.
 */
static WINEREGION *REGION_AllocWineRegion( INT n )
{
    WINEREGION *pReg;

    if ((pReg = HeapAlloc(SystemHeap, 0, sizeof( WINEREGION ))))
    {
        if ((pReg->rects = HeapAlloc(SystemHeap, 0, n * sizeof( RECT ))))
        {
            pReg->size = n;
            EMPTY_REGION(pReg);
            return pReg;
        }
        HeapFree(SystemHeap, 0, pReg);
    }
    return NULL;
}


/***********************************************************************
 *          REGION_CreateRegion
 *          Create a new empty region.
 */
static HRGN REGION_CreateRegion( INT n )
{
    HRGN hrgn;
    RGNOBJ *obj;

    if(!(hrgn = GDI_AllocObject( sizeof(RGNOBJ), REGION_MAGIC )))
        return 0;
    obj = (RGNOBJ *) GDI_HEAP_LOCK( hrgn );
    if(!(obj->rgn = REGION_AllocWineRegion(n))) {
        GDI_FreeObject( hrgn );
        return 0;
    }
    GDI_HEAP_UNLOCK( hrgn );
    return hrgn;
}


/***********************************************************************
 *           REGION_DestroyWineRegion
 */
static void REGION_DestroyWineRegion( WINEREGION* pReg )
{
    HeapFree( SystemHeap, 0, pReg->rects );
    HeapFree( SystemHeap, 0, pReg );
    return;
}

/***********************************************************************
 *           REGION_DeleteObject
 */
BOOL REGION_DeleteObject( HRGN hrgn, RGNOBJ * obj )
{
    TRACE(" %04x\n", hrgn );

    REGION_DestroyWineRegion( obj->rgn );
    return GDI_FreeObject( hrgn );
}

/***********************************************************************
 *           OffsetRgn16    (GDI.101)
 */
INT16 WINAPI OffsetRgn16( HRGN16 hrgn, INT16 x, INT16 y )
{
    return OffsetRgn( hrgn, x, y );
}

/***********************************************************************
 *           OffsetRgn32   (GDI32.256)
 */
INT WINAPI OffsetRgn( HRGN hrgn, INT x, INT y )
{
    RGNOBJ * obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC );

    if (obj && (x || y))
    {
	INT ret;
	int nbox = obj->rgn->numRects;
	RECT *pbox = obj->rgn->rects;
	
	TRACE(" %04x %d,%d\n", hrgn, x, y );
	if(nbox) {
	    while(nbox--) {
	        pbox->left += x;
		pbox->right += x;
		pbox->top += y;
		pbox->bottom += y;
		pbox++;
	    }
	    obj->rgn->extents.left += x;
	    obj->rgn->extents.right += x;
	    obj->rgn->extents.top += y;
	    obj->rgn->extents.bottom += y;
	}
	ret = obj->rgn->type;
	GDI_HEAP_UNLOCK( hrgn );
	return ret;
    }
    return ERROR;
}


/***********************************************************************
 *           GetRgnBox16    (GDI.134)
 */
INT16 WINAPI GetRgnBox16( HRGN16 hrgn, LPRECT16 rect )
{
    RECT r;
    INT16 ret = (INT16)GetRgnBox( hrgn, &r );
    CONV_RECT32TO16( &r, rect );
    return ret;
}

/***********************************************************************
 *           GetRgnBox32    (GDI32.219)
 */
INT WINAPI GetRgnBox( HRGN hrgn, LPRECT rect )
{
    RGNOBJ * obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC );
    if (obj)
    {
	INT ret;
	TRACE(" %04x\n", hrgn );
	rect->left = obj->rgn->extents.left;
	rect->top = obj->rgn->extents.top;
	rect->right = obj->rgn->extents.right;
	rect->bottom = obj->rgn->extents.bottom;
	ret = obj->rgn->type;
	GDI_HEAP_UNLOCK(hrgn);
	return ret;
    }
    return ERROR;
}


/***********************************************************************
 *           CreateRectRgn16    (GDI.64)
 *
 * NOTE: Doesn't call CreateRectRgn32 because of differences in SetRectRgn16/32
 */
HRGN16 WINAPI CreateRectRgn16(INT16 left, INT16 top, INT16 right, INT16 bottom)
{
    HRGN16 hrgn;

    if (!(hrgn = (HRGN16)REGION_CreateRegion(RGN_DEFAULT_RECTS)))
	return 0;
    TRACE("\n");
    SetRectRgn16(hrgn, left, top, right, bottom);
    return hrgn;
}


/***********************************************************************
 *           CreateRectRgn32   (GDI32.59)
 */
HRGN WINAPI CreateRectRgn(INT left, INT top, INT right, INT bottom)
{
    HRGN hrgn;

    /* Allocate 2 rects by default to reduce the number of reallocs */

    if (!(hrgn = REGION_CreateRegion(RGN_DEFAULT_RECTS)))
	return 0;
    TRACE("\n");
    SetRectRgn(hrgn, left, top, right, bottom);
    return hrgn;
}

/***********************************************************************
 *           CreateRectRgnIndirect16    (GDI.65)
 */
HRGN16 WINAPI CreateRectRgnIndirect16( const RECT16* rect )
{
    return CreateRectRgn16( rect->left, rect->top, rect->right, rect->bottom );
}


/***********************************************************************
 *           CreateRectRgnIndirect32    (GDI32.60)
 */
HRGN WINAPI CreateRectRgnIndirect( const RECT* rect )
{
    return CreateRectRgn( rect->left, rect->top, rect->right, rect->bottom );
}


/***********************************************************************
 *           SetRectRgn16    (GDI.172)
 *
 * NOTE: Win 3.1 sets region to empty if left > right
 */
VOID WINAPI SetRectRgn16( HRGN16 hrgn, INT16 left, INT16 top,
			  INT16 right, INT16 bottom )
{
    if(left < right)
        SetRectRgn( hrgn, left, top, right, bottom );
    else
        SetRectRgn( hrgn, 0, 0, 0, 0 );
}


/***********************************************************************
 *           SetRectRgn32    (GDI32.332)
 *
 * Allows either or both left and top to be greater than right or bottom.
 */
BOOL WINAPI SetRectRgn( HRGN hrgn, INT left, INT top,
			  INT right, INT bottom )
{
    RGNOBJ * obj;

    TRACE(" %04x %d,%d-%d,%d\n", 
	  hrgn, left, top, right, bottom );
    
    if (!(obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC ))) return FALSE;

    if (left > right) { INT tmp = left; left = right; right = tmp; }
    if (top > bottom) { INT tmp = top; top = bottom; bottom = tmp; }

    if((left != right) && (top != bottom))
    {
        obj->rgn->rects->left = obj->rgn->extents.left = left;
        obj->rgn->rects->top = obj->rgn->extents.top = top;
        obj->rgn->rects->right = obj->rgn->extents.right = right;
        obj->rgn->rects->bottom = obj->rgn->extents.bottom = bottom;
	obj->rgn->numRects = 1;
	obj->rgn->type = SIMPLEREGION;
    }
    else
	EMPTY_REGION(obj->rgn);

    GDI_HEAP_UNLOCK( hrgn );
    return TRUE;
}


/***********************************************************************
 *           CreateRoundRectRgn16    (GDI.444)
 *
 * If either ellipse dimension is zero we call CreateRectRgn16 for its
 * `special' behaviour. -ve ellipse dimensions can result in GPFs under win3.1
 * we just let CreateRoundRectRgn32 convert them to +ve values. 
 */

HRGN16 WINAPI CreateRoundRectRgn16( INT16 left, INT16 top,
				    INT16 right, INT16 bottom,
				    INT16 ellipse_width, INT16 ellipse_height )
{
    if( ellipse_width == 0 || ellipse_height == 0 )
        return CreateRectRgn16( left, top, right, bottom );
    else
        return (HRGN16)CreateRoundRectRgn( left, top, right, bottom,
					 ellipse_width, ellipse_height );
}

/***********************************************************************
 *           CreateRoundRectRgn32    (GDI32.61)
 */
HRGN WINAPI CreateRoundRectRgn( INT left, INT top,
				    INT right, INT bottom,
				    INT ellipse_width, INT ellipse_height )
{
    RGNOBJ * obj;
    HRGN hrgn;
    int asq, bsq, d, xd, yd;
    RECT rect;
    
      /* Check if we can do a normal rectangle instead */

    if ((ellipse_width == 0) || (ellipse_height == 0))
	return CreateRectRgn( left, top, right, bottom );

      /* Make the dimensions sensible */

    if (left > right) { INT tmp = left; left = right; right = tmp; }
    if (top > bottom) { INT tmp = top; top = bottom; bottom = tmp; }

    ellipse_width = abs(ellipse_width);
    ellipse_height = abs(ellipse_height);

      /* Create region */

    d = (ellipse_height < 128) ? ((3 * ellipse_height) >> 2) : 64;
    if (!(hrgn = REGION_CreateRegion(d))) return 0;
    obj = (RGNOBJ *) GDI_HEAP_LOCK( hrgn );
    TRACE("(%d,%d-%d,%d %dx%d): ret=%04x\n",
	  left, top, right, bottom, ellipse_width, ellipse_height, hrgn );

      /* Check parameters */

    if (ellipse_width > right-left) ellipse_width = right-left;
    if (ellipse_height > bottom-top) ellipse_height = bottom-top;

      /* Ellipse algorithm, based on an article by K. Porter */
      /* in DDJ Graphics Programming Column, 8/89 */

    asq = ellipse_width * ellipse_width / 4;        /* a^2 */
    bsq = ellipse_height * ellipse_height / 4;      /* b^2 */
    d = bsq - asq * ellipse_height / 2 + asq / 4;   /* b^2 - a^2b + a^2/4 */
    xd = 0;
    yd = asq * ellipse_height;                      /* 2a^2b */

    rect.left   = left + ellipse_width / 2;
    rect.right  = right - ellipse_width / 2;

      /* Loop to draw first half of quadrant */

    while (xd < yd)
    {
	if (d > 0)  /* if nearest pixel is toward the center */
	{
	      /* move toward center */
	    rect.top = top++;
	    rect.bottom = rect.top + 1;
	    REGION_UnionRectWithRegion( &rect, obj->rgn );
	    rect.top = --bottom;
	    rect.bottom = rect.top + 1;
	    REGION_UnionRectWithRegion( &rect, obj->rgn );
	    yd -= 2*asq;
	    d  -= yd;
	}
	rect.left--;        /* next horiz point */
	rect.right++;
	xd += 2*bsq;
	d  += bsq + xd;
    }

      /* Loop to draw second half of quadrant */

    d += (3 * (asq-bsq) / 2 - (xd+yd)) / 2;
    while (yd >= 0)
    {
	  /* next vertical point */
	rect.top = top++;
	rect.bottom = rect.top + 1;
	REGION_UnionRectWithRegion( &rect, obj->rgn );
	rect.top = --bottom;
	rect.bottom = rect.top + 1;
	REGION_UnionRectWithRegion( &rect, obj->rgn );
	if (d < 0)   /* if nearest pixel is outside ellipse */
	{
	    rect.left--;     /* move away from center */
	    rect.right++;
	    xd += 2*bsq;
	    d  += xd;
	}
	yd -= 2*asq;
	d  += asq - yd;
    }

      /* Add the inside rectangle */

    if (top <= bottom)
    {
	rect.top = top;
	rect.bottom = bottom;
	REGION_UnionRectWithRegion( &rect, obj->rgn );
    }
    obj->rgn->type = SIMPLEREGION; /* FIXME? */
    GDI_HEAP_UNLOCK( hrgn );
    return hrgn;
}


/***********************************************************************
 *           CreateEllipticRgn16    (GDI.54)
 */
HRGN16 WINAPI CreateEllipticRgn16( INT16 left, INT16 top,
				   INT16 right, INT16 bottom )
{
    return (HRGN16)CreateRoundRectRgn( left, top, right, bottom,
					 right-left, bottom-top );
}


/***********************************************************************
 *           CreateEllipticRgn32    (GDI32.39)
 */
HRGN WINAPI CreateEllipticRgn( INT left, INT top,
				   INT right, INT bottom )
{
    return CreateRoundRectRgn( left, top, right, bottom,
				 right-left, bottom-top );
}


/***********************************************************************
 *           CreateEllipticRgnIndirect16    (GDI.55)
 */
HRGN16 WINAPI CreateEllipticRgnIndirect16( const RECT16 *rect )
{
    return CreateRoundRectRgn( rect->left, rect->top, rect->right,
				 rect->bottom, rect->right - rect->left,
				 rect->bottom - rect->top );
}


/***********************************************************************
 *           CreateEllipticRgnIndirect32    (GDI32.40)
 */
HRGN WINAPI CreateEllipticRgnIndirect( const RECT *rect )
{
    return CreateRoundRectRgn( rect->left, rect->top, rect->right,
				 rect->bottom, rect->right - rect->left,
				 rect->bottom - rect->top );
}

/***********************************************************************
 *           GetRegionData32   (GDI32.217)
 * 
 */
DWORD WINAPI GetRegionData(HRGN hrgn, DWORD count, LPRGNDATA rgndata)
{
    DWORD size;
    RGNOBJ *obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC );
    
    TRACE(" %04x count = %ld, rgndata = %p\n",
		   hrgn, count, rgndata);

    if(!obj) return 0;

    size = obj->rgn->numRects * sizeof(RECT);
    if(count < (size + sizeof(RGNDATAHEADER)) || rgndata == NULL)
    {
        GDI_HEAP_UNLOCK( hrgn );
        return size + sizeof(RGNDATAHEADER);
    }

    rgndata->rdh.dwSize = sizeof(RGNDATAHEADER);
    rgndata->rdh.iType = RDH_RECTANGLES;
    rgndata->rdh.nCount = obj->rgn->numRects;
    rgndata->rdh.nRgnSize = size;
    rgndata->rdh.rcBound.left = obj->rgn->extents.left;
    rgndata->rdh.rcBound.top = obj->rgn->extents.top;
    rgndata->rdh.rcBound.right = obj->rgn->extents.right;
    rgndata->rdh.rcBound.bottom = obj->rgn->extents.bottom;

    memcpy( rgndata->Buffer, obj->rgn->rects, size );

    GDI_HEAP_UNLOCK( hrgn );
    return 1;
}

/***********************************************************************
 *           GetRegionData16   (GDI.607)
 * FIXME: is LPRGNDATA the same in Win16 and Win32 ?
 */
DWORD WINAPI GetRegionData16(HRGN16 hrgn, DWORD count, LPRGNDATA rgndata)
{
    return GetRegionData((HRGN)hrgn, count, rgndata);
}

/***********************************************************************
 *           ExtCreateRegion   (GDI32.94)
 * 
 */
HRGN WINAPI ExtCreateRegion( const XFORM* lpXform, DWORD dwCount, const RGNDATA* rgndata)
{
    HRGN hrgn;

    TRACE(" %p %ld %p = ", lpXform, dwCount, rgndata );

    if( lpXform )
        WARN("(Xform not implemented - ignored) ");
    
    if( rgndata->rdh.iType != RDH_RECTANGLES )
    {
	/* FIXME: We can use CreatePolyPolygonRgn() here
	 *        for trapezoidal data */

        WARN("(Unsupported region data) ");
	goto fail;
    }

    if( (hrgn = REGION_CreateRegion( rgndata->rdh.nCount )) )
    {
	RECT *pCurRect, *pEndRect;
	RGNOBJ *obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC );

	pEndRect = (RECT *)rgndata->Buffer + rgndata->rdh.nCount;
	for(pCurRect = (RECT *)rgndata->Buffer; pCurRect < pEndRect; pCurRect++)
	    REGION_UnionRectWithRegion( pCurRect, obj->rgn );
	GDI_HEAP_UNLOCK( hrgn );

	TRACE("%04x\n", hrgn );
	return hrgn;
    }
fail:
    WARN("Failed\n");
    return 0;
}

/***********************************************************************
 *           PtInRegion16    (GDI.161)
 */
BOOL16 WINAPI PtInRegion16( HRGN16 hrgn, INT16 x, INT16 y )
{
    return PtInRegion( hrgn, x, y );
}


/***********************************************************************
 *           PtInRegion32    (GDI32.278)
 */
BOOL WINAPI PtInRegion( HRGN hrgn, INT x, INT y )
{
    RGNOBJ * obj;
    
    if ((obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC )))
    {
	BOOL ret = FALSE;
	int i;

	if (obj->rgn->numRects > 0 && INRECT(obj->rgn->extents, x, y))
	    for (i = 0; i < obj->rgn->numRects; i++)
		if (INRECT (obj->rgn->rects[i], x, y))
		    ret = TRUE;
	GDI_HEAP_UNLOCK( hrgn );
	return ret;
    }
    return FALSE;
}


/***********************************************************************
 *           RectInRegion16    (GDI.181)
 */
BOOL16 WINAPI RectInRegion16( HRGN16 hrgn, const RECT16 *rect )
{
    RECT r32;

    CONV_RECT16TO32(rect, &r32);
    return (BOOL16)RectInRegion(hrgn, &r32);
}


/***********************************************************************
 *           RectInRegion32    (GDI32.281)
 *
 * Returns TRUE if rect is at least partly inside hrgn
 */
BOOL WINAPI RectInRegion( HRGN hrgn, const RECT *rect )
{
    RGNOBJ * obj;
    
    if ((obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC )))
    {
	RECT *pCurRect, *pRectEnd;
	BOOL ret = FALSE;
    
    /* this is (just) a useful optimization */
	if ((obj->rgn->numRects > 0) && EXTENTCHECK(&obj->rgn->extents,
						      rect))
	{
	    for (pCurRect = obj->rgn->rects, pRectEnd = pCurRect +
	     obj->rgn->numRects; pCurRect < pRectEnd; pCurRect++)
	    {
	        if (pCurRect->bottom <= rect->top)
		    continue;             /* not far enough down yet */

		if (pCurRect->top >= rect->bottom) {
		    ret = FALSE;          /* too far down */
		    break;
		}

		if (pCurRect->right <= rect->left)
		    continue;              /* not far enough over yet */

		if (pCurRect->left >= rect->right) {
		    continue;
		}

		ret = TRUE;
		break;
	    }
	}
	GDI_HEAP_UNLOCK(hrgn);
	return ret;
    }
    return FALSE;
}

/***********************************************************************
 *           EqualRgn16    (GDI.72)
 */
BOOL16 WINAPI EqualRgn16( HRGN16 rgn1, HRGN16 rgn2 )
{
    return EqualRgn( rgn1, rgn2 );
}


/***********************************************************************
 *           EqualRgn32    (GDI32.90)
 */
BOOL WINAPI EqualRgn( HRGN hrgn1, HRGN hrgn2 )
{
    RGNOBJ *obj1, *obj2;
    BOOL ret = FALSE;

    if ((obj1 = (RGNOBJ *) GDI_GetObjPtr( hrgn1, REGION_MAGIC ))) 
    {
	if ((obj2 = (RGNOBJ *) GDI_GetObjPtr( hrgn2, REGION_MAGIC ))) 
	{
	    int i;

	    ret = TRUE;
	    if ( obj1->rgn->numRects != obj2->rgn->numRects ) ret = FALSE;
	    else if ( obj1->rgn->numRects == 0 ) ret = TRUE;
	    else if ( !EqualRect(&obj1->rgn->extents, &obj2->rgn->extents) )
		ret = FALSE;
	    else for( i = 0; i < obj1->rgn->numRects; i++ ) {
		if (!EqualRect(obj1->rgn->rects + i, obj2->rgn->rects + i)) {
		    ret = FALSE;
		    break;
		}
	    }
	    GDI_HEAP_UNLOCK(hrgn2);
	}
	GDI_HEAP_UNLOCK(hrgn1);
    }
    return ret;
}
/***********************************************************************
 *           REGION_UnionRectWithRegion
 *           Adds a rectangle to a WINEREGION
 *           See below for REGION_UnionRectWithRgn
 */
static void REGION_UnionRectWithRegion(const RECT *rect, WINEREGION *rgn)
{
    WINEREGION region;

    region.rects = &region.extents;
    region.numRects = 1;
    region.size = 1;
    region.type = SIMPLEREGION;
    CopyRect(&(region.extents), rect);
    REGION_UnionRegion(rgn, rgn, &region);
    return;
}

/***********************************************************************
 *           REGION_UnionRectWithRgn
 *           Adds a rectangle to a HRGN32
 *           A helper used by scroll.c
 */
BOOL REGION_UnionRectWithRgn( HRGN hrgn, const RECT *lpRect )
{
    RGNOBJ *obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC );

    if(!obj) return FALSE;
    REGION_UnionRectWithRegion( lpRect, obj->rgn );
    GDI_HEAP_UNLOCK(hrgn);
    return TRUE;
}

/***********************************************************************
 *           REGION_CreateFrameRgn
 *
 * Create a region that is a frame around another region.
 * Expand all rectangles by +/- x and y, then subtract original region.
 */
BOOL REGION_FrameRgn( HRGN hDest, HRGN hSrc, INT x, INT y )
{
    BOOL bRet;
    RGNOBJ *srcObj = (RGNOBJ*) GDI_GetObjPtr( hSrc, REGION_MAGIC );

    if (srcObj->rgn->numRects != 0) 
    {
	RGNOBJ* destObj = (RGNOBJ*) GDI_GetObjPtr( hDest, REGION_MAGIC );
	RECT *pRect, *pEndRect;
	RECT tempRect;

	EMPTY_REGION( destObj->rgn );
	
	pEndRect = srcObj->rgn->rects + srcObj->rgn->numRects;
	for(pRect = srcObj->rgn->rects; pRect < pEndRect; pRect++)
	{
	    tempRect.left = pRect->left - x;        
	    tempRect.top = pRect->top - y;
	    tempRect.right = pRect->right + x;
	    tempRect.bottom = pRect->bottom + y;
	    REGION_UnionRectWithRegion( &tempRect, destObj->rgn );
	}
	REGION_SubtractRegion( destObj->rgn, destObj->rgn, srcObj->rgn );
	GDI_HEAP_UNLOCK ( hDest );
	bRet = TRUE;
    }
    else
	bRet = FALSE;
    GDI_HEAP_UNLOCK( hSrc );
    return bRet;
}

/***********************************************************************
 *           REGION_LPTODP
 *
 * Convert region to device co-ords for the supplied dc. 
 */
BOOL REGION_LPTODP( HDC hdc, HRGN hDest, HRGN hSrc )
{
    RECT *pCurRect, *pEndRect;
    RGNOBJ *srcObj, *destObj;
    DC * dc = DC_GetDCPtr( hdc );
    RECT tmpRect;

    TRACE(" hdc=%04x dest=%04x src=%04x\n",
	  hdc, hDest, hSrc) ;
    
    if (dc->w.MapMode == MM_TEXT) /* Requires only a translation */
    {
        if( CombineRgn( hDest, hSrc, 0, RGN_COPY ) == ERROR ) return FALSE;
	OffsetRgn( hDest, dc->vportOrgX - dc->wndOrgX, 
		     dc->vportOrgY - dc->wndOrgY );
	return TRUE;
    }

    if(!( srcObj = (RGNOBJ *) GDI_GetObjPtr( hSrc, REGION_MAGIC) ))
        return FALSE;
    if(!( destObj = (RGNOBJ *) GDI_GetObjPtr( hDest, REGION_MAGIC) ))
    {
        GDI_HEAP_UNLOCK( hSrc );
        return FALSE;
    }
    EMPTY_REGION( destObj->rgn );

    pEndRect = srcObj->rgn->rects + srcObj->rgn->numRects;
    for(pCurRect = srcObj->rgn->rects; pCurRect < pEndRect; pCurRect++)
    {
        tmpRect = *pCurRect;
	tmpRect.left = XLPTODP( dc, tmpRect.left );
	tmpRect.top = YLPTODP( dc, tmpRect.top );
	tmpRect.right = XLPTODP( dc, tmpRect.right );
	tmpRect.bottom = YLPTODP( dc, tmpRect.bottom );
	REGION_UnionRectWithRegion( &tmpRect, destObj->rgn );
    }
    
    GDI_HEAP_UNLOCK( hDest );
    GDI_HEAP_UNLOCK( hSrc );
    return TRUE;
}
    
/***********************************************************************
 *           CombineRgn16    (GDI.451)
 */
INT16 WINAPI CombineRgn16(HRGN16 hDest, HRGN16 hSrc1, HRGN16 hSrc2, INT16 mode)
{
    return (INT16)CombineRgn( hDest, hSrc1, hSrc2, mode );
}


/***********************************************************************
 *           CombineRgn32   (GDI32.19)
 *
 * Note: The behavior is correct even if src and dest regions are the same.
 */
INT WINAPI CombineRgn(HRGN hDest, HRGN hSrc1, HRGN hSrc2, INT mode)
{
    RGNOBJ *destObj = (RGNOBJ *) GDI_GetObjPtr( hDest, REGION_MAGIC);
    INT result = ERROR;

    TRACE(" %04x,%04x -> %04x mode=%x\n", 
	  hSrc1, hSrc2, hDest, mode );
    if (destObj)
    {
	RGNOBJ *src1Obj = (RGNOBJ *) GDI_GetObjPtr( hSrc1, REGION_MAGIC);

	if (src1Obj)
	{
	    TRACE("dump:\n");
	    if(TRACE_ON(region)) 
	      REGION_DumpRegion(src1Obj->rgn);
	    if (mode == RGN_COPY)
	    {
		REGION_CopyRegion( destObj->rgn, src1Obj->rgn );
		result = destObj->rgn->type;
	    }
	    else
	    {
		RGNOBJ *src2Obj = (RGNOBJ *) GDI_GetObjPtr( hSrc2, REGION_MAGIC);

		if (src2Obj)
		{
		    TRACE("dump:\n");
		    if(TRACE_ON(region)) 
		        REGION_DumpRegion(src2Obj->rgn);
		    switch (mode)
		    {
		    case RGN_AND:
			REGION_IntersectRegion( destObj->rgn, src1Obj->rgn, src2Obj->rgn);
			break;
		    case RGN_OR:
			REGION_UnionRegion( destObj->rgn, src1Obj->rgn, src2Obj->rgn );
			break;
		    case RGN_XOR:
			REGION_XorRegion( destObj->rgn, src1Obj->rgn, src2Obj->rgn );
			break;
		    case RGN_DIFF:
			REGION_SubtractRegion( destObj->rgn, src1Obj->rgn, src2Obj->rgn );
			break;
		    }
		    result = destObj->rgn->type;
		    GDI_HEAP_UNLOCK( hSrc2 );
		}
	    }
	    GDI_HEAP_UNLOCK( hSrc1 );
	}
	TRACE("dump:\n");
	if(TRACE_ON(region)) 
	  REGION_DumpRegion(destObj->rgn);

	GDI_HEAP_UNLOCK( hDest );
    } else {
       ERR("Invalid rgn=%04x\n", hDest);
    }
    return result;
}

/***********************************************************************
 *           REGION_SetExtents
 *           Re-calculate the extents of a region
 */
static void REGION_SetExtents (WINEREGION *pReg)
{
    RECT *pRect, *pRectEnd, *pExtents;

    if (pReg->numRects == 0)
    {
	pReg->extents.left = 0;
	pReg->extents.top = 0;
	pReg->extents.right = 0;
	pReg->extents.bottom = 0;
	return;
    }

    pExtents = &pReg->extents;
    pRect = pReg->rects;
    pRectEnd = &pRect[pReg->numRects - 1];

    /*
     * Since pRect is the first rectangle in the region, it must have the
     * smallest top and since pRectEnd is the last rectangle in the region,
     * it must have the largest bottom, because of banding. Initialize left and
     * right from pRect and pRectEnd, resp., as good things to initialize them
     * to...
     */
    pExtents->left = pRect->left;
    pExtents->top = pRect->top;
    pExtents->right = pRectEnd->right;
    pExtents->bottom = pRectEnd->bottom;

    while (pRect <= pRectEnd)
    {
	if (pRect->left < pExtents->left)
	    pExtents->left = pRect->left;
	if (pRect->right > pExtents->right)
	    pExtents->right = pRect->right;
	pRect++;
    }
}

/***********************************************************************
 *           REGION_CopyRegion
 */
static void REGION_CopyRegion(WINEREGION *dst, WINEREGION *src)
{
    if (dst != src) /*  don't want to copy to itself */
    {  
	if (dst->size < src->numRects)
	{
	    if (! (dst->rects = HeapReAlloc( SystemHeap, 0, dst->rects,
				src->numRects * sizeof(RECT) )))
		return;
	    dst->size = src->numRects;
	}
	dst->numRects = src->numRects;
	dst->extents.left = src->extents.left;
	dst->extents.top = src->extents.top;
	dst->extents.right = src->extents.right;
	dst->extents.bottom = src->extents.bottom;
	dst->type = src->type;

	memcpy((char *) dst->rects, (char *) src->rects,
	       (int) (src->numRects * sizeof(RECT)));
    }
    return;
}

/***********************************************************************
 *           REGION_Coalesce
 *
 *      Attempt to merge the rects in the current band with those in the
 *      previous one. Used only by REGION_RegionOp.
 *
 * Results:
 *      The new index for the previous band.
 *
 * Side Effects:
 *      If coalescing takes place:
 *          - rectangles in the previous band will have their bottom fields
 *            altered.
 *          - pReg->numRects will be decreased.
 *
 */
static INT REGION_Coalesce (
	     WINEREGION *pReg, /* Region to coalesce */
	     INT prevStart,  /* Index of start of previous band */
	     INT curStart    /* Index of start of current band */
) {
    RECT *pPrevRect;          /* Current rect in previous band */
    RECT *pCurRect;           /* Current rect in current band */
    RECT *pRegEnd;            /* End of region */
    INT curNumRects;          /* Number of rectangles in current band */
    INT prevNumRects;         /* Number of rectangles in previous band */
    INT bandtop;               /* top coordinate for current band */

    pRegEnd = &pReg->rects[pReg->numRects];

    pPrevRect = &pReg->rects[prevStart];
    prevNumRects = curStart - prevStart;

    /*
     * Figure out how many rectangles are in the current band. Have to do
     * this because multiple bands could have been added in REGION_RegionOp
     * at the end when one region has been exhausted.
     */
    pCurRect = &pReg->rects[curStart];
    bandtop = pCurRect->top;
    for (curNumRects = 0;
	 (pCurRect != pRegEnd) && (pCurRect->top == bandtop);
	 curNumRects++)
    {
	pCurRect++;
    }
    
    if (pCurRect != pRegEnd)
    {
	/*
	 * If more than one band was added, we have to find the start
	 * of the last band added so the next coalescing job can start
	 * at the right place... (given when multiple bands are added,
	 * this may be pointless -- see above).
	 */
	pRegEnd--;
	while (pRegEnd[-1].top == pRegEnd->top)
	{
	    pRegEnd--;
	}
	curStart = pRegEnd - pReg->rects;
	pRegEnd = pReg->rects + pReg->numRects;
    }
	
    if ((curNumRects == prevNumRects) && (curNumRects != 0)) {
	pCurRect -= curNumRects;
	/*
	 * The bands may only be coalesced if the bottom of the previous
	 * matches the top scanline of the current.
	 */
	if (pPrevRect->bottom == pCurRect->top)
	{
	    /*
	     * Make sure the bands have rects in the same places. This
	     * assumes that rects have been added in such a way that they
	     * cover the most area possible. I.e. two rects in a band must
	     * have some horizontal space between them.
	     */
	    do
	    {
		if ((pPrevRect->left != pCurRect->left) ||
		    (pPrevRect->right != pCurRect->right))
		{
		    /*
		     * The bands don't line up so they can't be coalesced.
		     */
		    return (curStart);
		}
		pPrevRect++;
		pCurRect++;
		prevNumRects -= 1;
	    } while (prevNumRects != 0);

	    pReg->numRects -= curNumRects;
	    pCurRect -= curNumRects;
	    pPrevRect -= curNumRects;

	    /*
	     * The bands may be merged, so set the bottom of each rect
	     * in the previous band to that of the corresponding rect in
	     * the current band.
	     */
	    do
	    {
		pPrevRect->bottom = pCurRect->bottom;
		pPrevRect++;
		pCurRect++;
		curNumRects -= 1;
	    } while (curNumRects != 0);

	    /*
	     * If only one band was added to the region, we have to backup
	     * curStart to the start of the previous band.
	     *
	     * If more than one band was added to the region, copy the
	     * other bands down. The assumption here is that the other bands
	     * came from the same region as the current one and no further
	     * coalescing can be done on them since it's all been done
	     * already... curStart is already in the right place.
	     */
	    if (pCurRect == pRegEnd)
	    {
		curStart = prevStart;
	    }
	    else
	    {
		do
		{
		    *pPrevRect++ = *pCurRect++;
		} while (pCurRect != pRegEnd);
	    }
	    
	}
    }
    return (curStart);
}

/***********************************************************************
 *           REGION_RegionOp
 *
 *      Apply an operation to two regions. Called by REGION_Union,
 *      REGION_Inverse, REGION_Subtract, REGION_Intersect...
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The new region is overwritten.
 *
 * Notes:
 *      The idea behind this function is to view the two regions as sets.
 *      Together they cover a rectangle of area that this function divides
 *      into horizontal bands where points are covered only by one region
 *      or by both. For the first case, the nonOverlapFunc is called with
 *      each the band and the band's upper and lower extents. For the
 *      second, the overlapFunc is called to process the entire band. It
 *      is responsible for clipping the rectangles in the band, though
 *      this function provides the boundaries.
 *      At the end of each band, the new region is coalesced, if possible,
 *      to reduce the number of rectangles in the region.
 *
 */
static void REGION_RegionOp(
	    WINEREGION *newReg, /* Place to store result */
	    WINEREGION *reg1,   /* First region in operation */
            WINEREGION *reg2,   /* 2nd region in operation */
	    void (*overlapFunc)(),     /* Function to call for over-lapping bands */
	    void (*nonOverlap1Func)(), /* Function to call for non-overlapping bands in region 1 */
	    void (*nonOverlap2Func)()  /* Function to call for non-overlapping bands in region 2 */
) {
    RECT *r1;                         /* Pointer into first region */
    RECT *r2;                         /* Pointer into 2d region */
    RECT *r1End;                      /* End of 1st region */
    RECT *r2End;                      /* End of 2d region */
    INT ybot;                         /* Bottom of intersection */
    INT ytop;                         /* Top of intersection */
    RECT *oldRects;                   /* Old rects for newReg */
    INT prevBand;                     /* Index of start of
						 * previous band in newReg */
    INT curBand;                      /* Index of start of current
						 * band in newReg */
    RECT *r1BandEnd;                  /* End of current band in r1 */
    RECT *r2BandEnd;                  /* End of current band in r2 */
    INT top;                          /* Top of non-overlapping band */
    INT bot;                          /* Bottom of non-overlapping band */
    
    /*
     * Initialization:
     *  set r1, r2, r1End and r2End appropriately, preserve the important
     * parts of the destination region until the end in case it's one of
     * the two source regions, then mark the "new" region empty, allocating
     * another array of rectangles for it to use.
     */
    r1 = reg1->rects;
    r2 = reg2->rects;
    r1End = r1 + reg1->numRects;
    r2End = r2 + reg2->numRects;
    

    /*
     * newReg may be one of the src regions so we can't empty it. We keep a 
     * note of its rects pointer (so that we can free them later), preserve its
     * extents and simply set numRects to zero. 
     */

    oldRects = newReg->rects;
    newReg->numRects = 0;

    /*
     * Allocate a reasonable number of rectangles for the new region. The idea
     * is to allocate enough so the individual functions don't need to
     * reallocate and copy the array, which is time consuming, yet we don't
     * have to worry about using too much memory. I hope to be able to
     * nuke the Xrealloc() at the end of this function eventually.
     */
    newReg->size = MAX(reg1->numRects,reg2->numRects) * 2;

    if (! (newReg->rects = HeapAlloc( SystemHeap, 0, 
			          sizeof(RECT) * newReg->size )))
    {
	newReg->size = 0;
	return;
    }
    
    /*
     * Initialize ybot and ytop.
     * In the upcoming loop, ybot and ytop serve different functions depending
     * on whether the band being handled is an overlapping or non-overlapping
     * band.
     *  In the case of a non-overlapping band (only one of the regions
     * has points in the band), ybot is the bottom of the most recent
     * intersection and thus clips the top of the rectangles in that band.
     * ytop is the top of the next intersection between the two regions and
     * serves to clip the bottom of the rectangles in the current band.
     *  For an overlapping band (where the two regions intersect), ytop clips
     * the top of the rectangles of both regions and ybot clips the bottoms.
     */
    if (reg1->extents.top < reg2->extents.top)
	ybot = reg1->extents.top;
    else
	ybot = reg2->extents.top;
    
    /*
     * prevBand serves to mark the start of the previous band so rectangles
     * can be coalesced into larger rectangles. qv. miCoalesce, above.
     * In the beginning, there is no previous band, so prevBand == curBand
     * (curBand is set later on, of course, but the first band will always
     * start at index 0). prevBand and curBand must be indices because of
     * the possible expansion, and resultant moving, of the new region's
     * array of rectangles.
     */
    prevBand = 0;
    
    do
    {
	curBand = newReg->numRects;

	/*
	 * This algorithm proceeds one source-band (as opposed to a
	 * destination band, which is determined by where the two regions
	 * intersect) at a time. r1BandEnd and r2BandEnd serve to mark the
	 * rectangle after the last one in the current band for their
	 * respective regions.
	 */
	r1BandEnd = r1;
	while ((r1BandEnd != r1End) && (r1BandEnd->top == r1->top))
	{
	    r1BandEnd++;
	}
	
	r2BandEnd = r2;
	while ((r2BandEnd != r2End) && (r2BandEnd->top == r2->top))
	{
	    r2BandEnd++;
	}
	
	/*
	 * First handle the band that doesn't intersect, if any.
	 *
	 * Note that attention is restricted to one band in the
	 * non-intersecting region at once, so if a region has n
	 * bands between the current position and the next place it overlaps
	 * the other, this entire loop will be passed through n times.
	 */
	if (r1->top < r2->top)
	{
	    top = MAX(r1->top,ybot);
	    bot = MIN(r1->bottom,r2->top);

	    if ((top != bot) && (nonOverlap1Func != (void (*)())NULL))
	    {
		(* nonOverlap1Func) (newReg, r1, r1BandEnd, top, bot);
	    }

	    ytop = r2->top;
	}
	else if (r2->top < r1->top)
	{
	    top = MAX(r2->top,ybot);
	    bot = MIN(r2->bottom,r1->top);

	    if ((top != bot) && (nonOverlap2Func != (void (*)())NULL))
	    {
		(* nonOverlap2Func) (newReg, r2, r2BandEnd, top, bot);
	    }

	    ytop = r1->top;
	}
	else
	{
	    ytop = r1->top;
	}

	/*
	 * If any rectangles got added to the region, try and coalesce them
	 * with rectangles from the previous band. Note we could just do
	 * this test in miCoalesce, but some machines incur a not
	 * inconsiderable cost for function calls, so...
	 */
	if (newReg->numRects != curBand)
	{
	    prevBand = REGION_Coalesce (newReg, prevBand, curBand);
	}

	/*
	 * Now see if we've hit an intersecting band. The two bands only
	 * intersect if ybot > ytop
	 */
	ybot = MIN(r1->bottom, r2->bottom);
	curBand = newReg->numRects;
	if (ybot > ytop)
	{
	    (* overlapFunc) (newReg, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot);

	}
	
	if (newReg->numRects != curBand)
	{
	    prevBand = REGION_Coalesce (newReg, prevBand, curBand);
	}

	/*
	 * If we've finished with a band (bottom == ybot) we skip forward
	 * in the region to the next band.
	 */
	if (r1->bottom == ybot)
	{
	    r1 = r1BandEnd;
	}
	if (r2->bottom == ybot)
	{
	    r2 = r2BandEnd;
	}
    } while ((r1 != r1End) && (r2 != r2End));

    /*
     * Deal with whichever region still has rectangles left.
     */
    curBand = newReg->numRects;
    if (r1 != r1End)
    {
	if (nonOverlap1Func != (void (*)())NULL)
	{
	    do
	    {
		r1BandEnd = r1;
		while ((r1BandEnd < r1End) && (r1BandEnd->top == r1->top))
		{
		    r1BandEnd++;
		}
		(* nonOverlap1Func) (newReg, r1, r1BandEnd,
				     MAX(r1->top,ybot), r1->bottom);
		r1 = r1BandEnd;
	    } while (r1 != r1End);
	}
    }
    else if ((r2 != r2End) && (nonOverlap2Func != (void (*)())NULL))
    {
	do
	{
	    r2BandEnd = r2;
	    while ((r2BandEnd < r2End) && (r2BandEnd->top == r2->top))
	    {
		 r2BandEnd++;
	    }
	    (* nonOverlap2Func) (newReg, r2, r2BandEnd,
				MAX(r2->top,ybot), r2->bottom);
	    r2 = r2BandEnd;
	} while (r2 != r2End);
    }

    if (newReg->numRects != curBand)
    {
	(void) REGION_Coalesce (newReg, prevBand, curBand);
    }

    /*
     * A bit of cleanup. To keep regions from growing without bound,
     * we shrink the array of rectangles to match the new number of
     * rectangles in the region. This never goes to 0, however...
     *
     * Only do this stuff if the number of rectangles allocated is more than
     * twice the number of rectangles in the region (a simple optimization...).
     */
    if ((newReg->numRects < (newReg->size >> 1)) && (newReg->numRects > 2))
    {
	if (REGION_NOT_EMPTY(newReg))
	{
	    RECT *prev_rects = newReg->rects;
	    newReg->size = newReg->numRects;
	    newReg->rects = HeapReAlloc( SystemHeap, 0, newReg->rects,
				   sizeof(RECT) * newReg->size );
	    if (! newReg->rects)
		newReg->rects = prev_rects;
	}
	else
	{
	    /*
	     * No point in doing the extra work involved in an Xrealloc if
	     * the region is empty
	     */
	    newReg->size = 1;
	    HeapFree( SystemHeap, 0, newReg->rects );
	    newReg->rects = HeapAlloc( SystemHeap, 0, sizeof(RECT) );
	}
    }
    HeapFree( SystemHeap, 0, oldRects );
    return;
}

/***********************************************************************
 *          Region Intersection
 ***********************************************************************/


/***********************************************************************
 *	     REGION_IntersectO
 *
 * Handle an overlapping band for REGION_Intersect.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Rectangles may be added to the region.
 *
 */
static void REGION_IntersectO(WINEREGION *pReg,  RECT *r1, RECT *r1End,
		RECT *r2, RECT *r2End, INT top, INT bottom)

{
    INT       left, right;
    RECT      *pNextRect;

    pNextRect = &pReg->rects[pReg->numRects];

    while ((r1 != r1End) && (r2 != r2End))
    {
	left = MAX(r1->left, r2->left);
	right =	MIN(r1->right, r2->right);

	/*
	 * If there's any overlap between the two rectangles, add that
	 * overlap to the new region.
	 * There's no need to check for subsumption because the only way
	 * such a need could arise is if some region has two rectangles
	 * right next to each other. Since that should never happen...
	 */
	if (left < right)
	{
	    MEMCHECK(pReg, pNextRect, pReg->rects);
	    pNextRect->left = left;
	    pNextRect->top = top;
	    pNextRect->right = right;
	    pNextRect->bottom = bottom;
	    pReg->numRects += 1;
	    pNextRect++;
	}

	/*
	 * Need to advance the pointers. Shift the one that extends
	 * to the right the least, since the other still has a chance to
	 * overlap with that region's next rectangle, if you see what I mean.
	 */
	if (r1->right < r2->right)
	{
	    r1++;
	}
	else if (r2->right < r1->right)
	{
	    r2++;
	}
	else
	{
	    r1++;
	    r2++;
	}
    }
    return;
}

/***********************************************************************
 *	     REGION_IntersectRegion
 */
static void REGION_IntersectRegion(WINEREGION *newReg, WINEREGION *reg1,
				   WINEREGION *reg2)
{
   /* check for trivial reject */
    if ( (!(reg1->numRects)) || (!(reg2->numRects))  ||
	(!EXTENTCHECK(&reg1->extents, &reg2->extents)))
	newReg->numRects = 0;
    else
	REGION_RegionOp (newReg, reg1, reg2, 
	 (voidProcp) REGION_IntersectO, (voidProcp) NULL, (voidProcp) NULL);
    
    /*
     * Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the same. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    REGION_SetExtents(newReg);
    newReg->type = (newReg->numRects) ? COMPLEXREGION : NULLREGION ;
    return;
}

/***********************************************************************
 *	     Region Union
 ***********************************************************************/

/***********************************************************************
 *	     REGION_UnionNonO
 *
 *      Handle a non-overlapping band for the union operation. Just
 *      Adds the rectangles into the region. Doesn't have to check for
 *      subsumption or anything.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      pReg->numRects is incremented and the final rectangles overwritten
 *      with the rectangles we're passed.
 *
 */
static void REGION_UnionNonO (WINEREGION *pReg, RECT *r, RECT *rEnd,
			      INT top, INT bottom)
{
    RECT *pNextRect;

    pNextRect = &pReg->rects[pReg->numRects];

    while (r != rEnd)
    {
	MEMCHECK(pReg, pNextRect, pReg->rects);
	pNextRect->left = r->left;
	pNextRect->top = top;
	pNextRect->right = r->right;
	pNextRect->bottom = bottom;
	pReg->numRects += 1;
	pNextRect++;
	r++;
    }
    return;
}

/***********************************************************************
 *	     REGION_UnionO
 *
 *      Handle an overlapping band for the union operation. Picks the
 *      left-most rectangle each time and merges it into the region.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Rectangles are overwritten in pReg->rects and pReg->numRects will
 *      be changed.
 *
 */
static void REGION_UnionO (WINEREGION *pReg, RECT *r1, RECT *r1End,
			   RECT *r2, RECT *r2End, INT top, INT bottom)
{
    RECT *pNextRect;
    
    pNextRect = &pReg->rects[pReg->numRects];

#define MERGERECT(r) \
    if ((pReg->numRects != 0) &&  \
	(pNextRect[-1].top == top) &&  \
	(pNextRect[-1].bottom == bottom) &&  \
	(pNextRect[-1].right >= r->left))  \
    {  \
	if (pNextRect[-1].right < r->right)  \
	{  \
	    pNextRect[-1].right = r->right;  \
	}  \
    }  \
    else  \
    {  \
	MEMCHECK(pReg, pNextRect, pReg->rects);  \
	pNextRect->top = top;  \
	pNextRect->bottom = bottom;  \
	pNextRect->left = r->left;  \
	pNextRect->right = r->right;  \
	pReg->numRects += 1;  \
	pNextRect += 1;  \
    }  \
    r++;
    
    while ((r1 != r1End) && (r2 != r2End))
    {
	if (r1->left < r2->left)
	{
	    MERGERECT(r1);
	}
	else
	{
	    MERGERECT(r2);
	}
    }
    
    if (r1 != r1End)
    {
	do
	{
	    MERGERECT(r1);
	} while (r1 != r1End);
    }
    else while (r2 != r2End)
    {
	MERGERECT(r2);
    }
    return;
}

/***********************************************************************
 *	     REGION_UnionRegion
 */
static void REGION_UnionRegion(WINEREGION *newReg, WINEREGION *reg1,
			       WINEREGION *reg2)
{
    /*  checks all the simple cases */

    /*
     * Region 1 and 2 are the same or region 1 is empty
     */
    if ( (reg1 == reg2) || (!(reg1->numRects)) )
    {
	if (newReg != reg2)
	    REGION_CopyRegion(newReg, reg2);
	return;
    }

    /*
     * if nothing to union (region 2 empty)
     */
    if (!(reg2->numRects))
    {
	if (newReg != reg1)
	    REGION_CopyRegion(newReg, reg1);
	return;
    }

    /*
     * Region 1 completely subsumes region 2
     */
    if ((reg1->numRects == 1) && 
	(reg1->extents.left <= reg2->extents.left) &&
	(reg1->extents.top <= reg2->extents.top) &&
	(reg1->extents.right >= reg2->extents.right) &&
	(reg1->extents.bottom >= reg2->extents.bottom))
    {
	if (newReg != reg1)
	    REGION_CopyRegion(newReg, reg1);
	return;
    }

    /*
     * Region 2 completely subsumes region 1
     */
    if ((reg2->numRects == 1) && 
	(reg2->extents.left <= reg1->extents.left) &&
	(reg2->extents.top <= reg1->extents.top) &&
	(reg2->extents.right >= reg1->extents.right) &&
	(reg2->extents.bottom >= reg1->extents.bottom))
    {
	if (newReg != reg2)
	    REGION_CopyRegion(newReg, reg2);
	return;
    }

    REGION_RegionOp (newReg, reg1, reg2, (voidProcp) REGION_UnionO, 
		(voidProcp) REGION_UnionNonO, (voidProcp) REGION_UnionNonO);

    newReg->extents.left = MIN(reg1->extents.left, reg2->extents.left);
    newReg->extents.top = MIN(reg1->extents.top, reg2->extents.top);
    newReg->extents.right = MAX(reg1->extents.right, reg2->extents.right);
    newReg->extents.bottom = MAX(reg1->extents.bottom, reg2->extents.bottom);
    newReg->type = (newReg->numRects) ? COMPLEXREGION : NULLREGION ;
    return;
}

/***********************************************************************
 *	     Region Subtraction
 ***********************************************************************/

/***********************************************************************
 *	     REGION_SubtractNonO1
 *
 *      Deal with non-overlapping band for subtraction. Any parts from
 *      region 2 we discard. Anything from region 1 we add to the region.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      pReg may be affected.
 *
 */
static void REGION_SubtractNonO1 (WINEREGION *pReg, RECT *r, RECT *rEnd, 
		INT top, INT bottom)
{
    RECT *pNextRect;
	
    pNextRect = &pReg->rects[pReg->numRects];
	
    while (r != rEnd)
    {
	MEMCHECK(pReg, pNextRect, pReg->rects);
	pNextRect->left = r->left;
	pNextRect->top = top;
	pNextRect->right = r->right;
	pNextRect->bottom = bottom;
	pReg->numRects += 1;
	pNextRect++;
	r++;
    }
    return;
}


/***********************************************************************
 *	     REGION_SubtractO
 *
 *      Overlapping band subtraction. x1 is the left-most point not yet
 *      checked.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      pReg may have rectangles added to it.
 *
 */
static void REGION_SubtractO (WINEREGION *pReg, RECT *r1, RECT *r1End, 
		RECT *r2, RECT *r2End, INT top, INT bottom)
{
    RECT *pNextRect;
    INT left;
    
    left = r1->left;
    pNextRect = &pReg->rects[pReg->numRects];

    while ((r1 != r1End) && (r2 != r2End))
    {
	if (r2->right <= left)
	{
	    /*
	     * Subtrahend missed the boat: go to next subtrahend.
	     */
	    r2++;
	}
	else if (r2->left <= left)
	{
	    /*
	     * Subtrahend preceeds minuend: nuke left edge of minuend.
	     */
	    left = r2->right;
	    if (left >= r1->right)
	    {
		/*
		 * Minuend completely covered: advance to next minuend and
		 * reset left fence to edge of new minuend.
		 */
		r1++;
		if (r1 != r1End)
		    left = r1->left;
	    }
	    else
	    {
		/*
		 * Subtrahend now used up since it doesn't extend beyond
		 * minuend
		 */
		r2++;
	    }
	}
	else if (r2->left < r1->right)
	{
	    /*
	     * Left part of subtrahend covers part of minuend: add uncovered
	     * part of minuend to region and skip to next subtrahend.
	     */
	    MEMCHECK(pReg, pNextRect, pReg->rects);
	    pNextRect->left = left;
	    pNextRect->top = top;
	    pNextRect->right = r2->left;
	    pNextRect->bottom = bottom;
	    pReg->numRects += 1;
	    pNextRect++;
	    left = r2->right;
	    if (left >= r1->right)
	    {
		/*
		 * Minuend used up: advance to new...
		 */
		r1++;
		if (r1 != r1End)
		    left = r1->left;
	    }
	    else
	    {
		/*
		 * Subtrahend used up
		 */
		r2++;
	    }
	}
	else
	{
	    /*
	     * Minuend used up: add any remaining piece before advancing.
	     */
	    if (r1->right > left)
	    {
		MEMCHECK(pReg, pNextRect, pReg->rects);
		pNextRect->left = left;
		pNextRect->top = top;
		pNextRect->right = r1->right;
		pNextRect->bottom = bottom;
		pReg->numRects += 1;
		pNextRect++;
	    }
	    r1++;
	    left = r1->left;
	}
    }

    /*
     * Add remaining minuend rectangles to region.
     */
    while (r1 != r1End)
    {
	MEMCHECK(pReg, pNextRect, pReg->rects);
	pNextRect->left = left;
	pNextRect->top = top;
	pNextRect->right = r1->right;
	pNextRect->bottom = bottom;
	pReg->numRects += 1;
	pNextRect++;
	r1++;
	if (r1 != r1End)
	{
	    left = r1->left;
	}
    }
    return;
}
	
/***********************************************************************
 *	     REGION_SubtractRegion
 *
 *      Subtract regS from regM and leave the result in regD.
 *      S stands for subtrahend, M for minuend and D for difference.
 *
 * Results:
 *      TRUE.
 *
 * Side Effects:
 *      regD is overwritten.
 *
 */
static void REGION_SubtractRegion(WINEREGION *regD, WINEREGION *regM,
				                       WINEREGION *regS )
{
   /* check for trivial reject */
    if ( (!(regM->numRects)) || (!(regS->numRects))  ||
	(!EXTENTCHECK(&regM->extents, &regS->extents)) )
    {
	REGION_CopyRegion(regD, regM);
	return;
    }
 
    REGION_RegionOp (regD, regM, regS, (voidProcp) REGION_SubtractO, 
		(voidProcp) REGION_SubtractNonO1, (voidProcp) NULL);

    /*
     * Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the unaltered. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    REGION_SetExtents (regD);
    regD->type = (regD->numRects) ? COMPLEXREGION : NULLREGION ;
    return;
}

/***********************************************************************
 *	     REGION_XorRegion
 */
static void REGION_XorRegion(WINEREGION *dr, WINEREGION *sra,
							WINEREGION *srb)
{
    WINEREGION *tra, *trb;

    if ((! (tra = REGION_AllocWineRegion(sra->numRects + 1))) || 
	(! (trb = REGION_AllocWineRegion(srb->numRects + 1))))
	return;
    REGION_SubtractRegion(tra,sra,srb);
    REGION_SubtractRegion(trb,srb,sra);
    REGION_UnionRegion(dr,tra,trb);
    REGION_DestroyWineRegion(tra);
    REGION_DestroyWineRegion(trb);
    return;
}

/**************************************************************************
 *
 *    Poly Regions
 * 
 *************************************************************************/

#define LARGE_COORDINATE  0x7fffffff /* FIXME */
#define SMALL_COORDINATE  0x80000000

/***********************************************************************
 *     REGION_InsertEdgeInET
 *
 *     Insert the given edge into the edge table.
 *     First we must find the correct bucket in the
 *     Edge table, then find the right slot in the
 *     bucket.  Finally, we can insert it.
 *
 */
static void REGION_InsertEdgeInET(EdgeTable *ET, EdgeTableEntry *ETE,
                INT scanline, ScanLineListBlock **SLLBlock, INT *iSLLBlock)

{
    EdgeTableEntry *start, *prev;
    ScanLineList *pSLL, *pPrevSLL;
    ScanLineListBlock *tmpSLLBlock;

    /*
     * find the right bucket to put the edge into
     */
    pPrevSLL = &ET->scanlines;
    pSLL = pPrevSLL->next;
    while (pSLL && (pSLL->scanline < scanline)) 
    {
        pPrevSLL = pSLL;
        pSLL = pSLL->next;
    }

    /*
     * reassign pSLL (pointer to ScanLineList) if necessary
     */
    if ((!pSLL) || (pSLL->scanline > scanline)) 
    {
        if (*iSLLBlock > SLLSPERBLOCK-1) 
        {
            tmpSLLBlock = HeapAlloc( SystemHeap, 0, sizeof(ScanLineListBlock));
	    if(!tmpSLLBlock)
	    {
	        WARN("Can't alloc SLLB\n");
		return;
	    }
            (*SLLBlock)->next = tmpSLLBlock;
            tmpSLLBlock->next = (ScanLineListBlock *)NULL;
            *SLLBlock = tmpSLLBlock;
            *iSLLBlock = 0;
        }
        pSLL = &((*SLLBlock)->SLLs[(*iSLLBlock)++]);

        pSLL->next = pPrevSLL->next;
        pSLL->edgelist = (EdgeTableEntry *)NULL;
        pPrevSLL->next = pSLL;
    }
    pSLL->scanline = scanline;

    /*
     * now insert the edge in the right bucket
     */
    prev = (EdgeTableEntry *)NULL;
    start = pSLL->edgelist;
    while (start && (start->bres.minor_axis < ETE->bres.minor_axis)) 
    {
        prev = start;
        start = start->next;
    }
    ETE->next = start;

    if (prev)
        prev->next = ETE;
    else
        pSLL->edgelist = ETE;
}

/***********************************************************************
 *     REGION_CreateEdgeTable
 *
 *     This routine creates the edge table for
 *     scan converting polygons. 
 *     The Edge Table (ET) looks like:
 *
 *    EdgeTable
 *     --------
 *    |  ymax  |        ScanLineLists
 *    |scanline|-->------------>-------------->...
 *     --------   |scanline|   |scanline|
 *                |edgelist|   |edgelist|
 *                ---------    ---------
 *                    |             |
 *                    |             |
 *                    V             V
 *              list of ETEs   list of ETEs
 *
 *     where ETE is an EdgeTableEntry data structure,
 *     and there is one ScanLineList per scanline at
 *     which an edge is initially entered.
 *
 */
static void REGION_CreateETandAET(const INT *Count, INT nbpolygons, 
            const POINT *pts, EdgeTable *ET, EdgeTableEntry *AET,
            EdgeTableEntry *pETEs, ScanLineListBlock *pSLLBlock)
{
    const POINT *top, *bottom;
    const POINT *PrevPt, *CurrPt, *EndPt;
    INT poly, count;
    int iSLLBlock = 0;
    int dy;

    
    /*
     *  initialize the Active Edge Table
     */
    AET->next = (EdgeTableEntry *)NULL;
    AET->back = (EdgeTableEntry *)NULL;
    AET->nextWETE = (EdgeTableEntry *)NULL;
    AET->bres.minor_axis = SMALL_COORDINATE;

    /*
     *  initialize the Edge Table.
     */
    ET->scanlines.next = (ScanLineList *)NULL;
    ET->ymax = SMALL_COORDINATE;
    ET->ymin = LARGE_COORDINATE;
    pSLLBlock->next = (ScanLineListBlock *)NULL;

    EndPt = pts - 1;
    for(poly = 0; poly < nbpolygons; poly++)
    {
        count = Count[poly];
        EndPt += count;
        if(count < 2)
	    continue;
	
	PrevPt = EndPt;

    /*
     *  for each vertex in the array of points.
     *  In this loop we are dealing with two vertices at
     *  a time -- these make up one edge of the polygon.
     */
	while (count--) 
	{
	    CurrPt = pts++;

        /*
         *  find out which point is above and which is below.
         */
	    if (PrevPt->y > CurrPt->y) 
	    {
	        bottom = PrevPt, top = CurrPt;
		pETEs->ClockWise = 0;
	    }
	    else 
	    {
	        bottom = CurrPt, top = PrevPt;
		pETEs->ClockWise = 1;
	    }

        /*
         * don't add horizontal edges to the Edge table.
         */
	    if (bottom->y != top->y) 
	    {
	        pETEs->ymax = bottom->y-1;
				/* -1 so we don't get last scanline */

            /*
             *  initialize integer edge algorithm
             */
		dy = bottom->y - top->y;
		BRESINITPGONSTRUCT(dy, top->x, bottom->x, pETEs->bres);

		REGION_InsertEdgeInET(ET, pETEs, top->y, &pSLLBlock, 
								&iSLLBlock);

		if (PrevPt->y > ET->ymax)
		  ET->ymax = PrevPt->y;
		if (PrevPt->y < ET->ymin)
		  ET->ymin = PrevPt->y;
		pETEs++;
	    }

	    PrevPt = CurrPt;
	}
    }
}

/***********************************************************************
 *     REGION_loadAET
 *
 *     This routine moves EdgeTableEntries from the
 *     EdgeTable into the Active Edge Table,
 *     leaving them sorted by smaller x coordinate.
 *
 */
static void REGION_loadAET(EdgeTableEntry *AET, EdgeTableEntry *ETEs)
{
    EdgeTableEntry *pPrevAET;
    EdgeTableEntry *tmp;

    pPrevAET = AET;
    AET = AET->next;
    while (ETEs) 
    {
        while (AET && (AET->bres.minor_axis < ETEs->bres.minor_axis)) 
        {
            pPrevAET = AET;
            AET = AET->next;
        }
        tmp = ETEs->next;
        ETEs->next = AET;
        if (AET)
            AET->back = ETEs;
        ETEs->back = pPrevAET;
        pPrevAET->next = ETEs;
        pPrevAET = ETEs;

        ETEs = tmp;
    }
}

/***********************************************************************
 *     REGION_computeWAET
 *
 *     This routine links the AET by the
 *     nextWETE (winding EdgeTableEntry) link for
 *     use by the winding number rule.  The final 
 *     Active Edge Table (AET) might look something
 *     like:
 *
 *     AET
 *     ----------  ---------   ---------
 *     |ymax    |  |ymax    |  |ymax    | 
 *     | ...    |  |...     |  |...     |
 *     |next    |->|next    |->|next    |->...
 *     |nextWETE|  |nextWETE|  |nextWETE|
 *     ---------   ---------   ^--------
 *         |                   |       |
 *         V------------------->       V---> ...
 *
 */
static void REGION_computeWAET(EdgeTableEntry *AET)
{
    register EdgeTableEntry *pWETE;
    register int inside = 1;
    register int isInside = 0;

    AET->nextWETE = (EdgeTableEntry *)NULL;
    pWETE = AET;
    AET = AET->next;
    while (AET) 
    {
        if (AET->ClockWise)
            isInside++;
        else
            isInside--;

        if ((!inside && !isInside) ||
            ( inside &&  isInside)) 
        {
            pWETE->nextWETE = AET;
            pWETE = AET;
            inside = !inside;
        }
        AET = AET->next;
    }
    pWETE->nextWETE = (EdgeTableEntry *)NULL;
}

/***********************************************************************
 *     REGION_InsertionSort
 *
 *     Just a simple insertion sort using
 *     pointers and back pointers to sort the Active
 *     Edge Table.
 *
 */
static BOOL REGION_InsertionSort(EdgeTableEntry *AET)
{
    EdgeTableEntry *pETEchase;
    EdgeTableEntry *pETEinsert;
    EdgeTableEntry *pETEchaseBackTMP;
    BOOL changed = FALSE;

    AET = AET->next;
    while (AET) 
    {
        pETEinsert = AET;
        pETEchase = AET;
        while (pETEchase->back->bres.minor_axis > AET->bres.minor_axis)
            pETEchase = pETEchase->back;

        AET = AET->next;
        if (pETEchase != pETEinsert) 
        {
            pETEchaseBackTMP = pETEchase->back;
            pETEinsert->back->next = AET;
            if (AET)
                AET->back = pETEinsert->back;
            pETEinsert->next = pETEchase;
            pETEchase->back->next = pETEinsert;
            pETEchase->back = pETEinsert;
            pETEinsert->back = pETEchaseBackTMP;
            changed = TRUE;
        }
    }
    return changed;
}

/***********************************************************************
 *     REGION_FreeStorage
 *
 *     Clean up our act.
 */
static void REGION_FreeStorage(ScanLineListBlock *pSLLBlock)
{
    ScanLineListBlock   *tmpSLLBlock;

    while (pSLLBlock) 
    {
        tmpSLLBlock = pSLLBlock->next;
        HeapFree( SystemHeap, 0, pSLLBlock );
        pSLLBlock = tmpSLLBlock;
    }
}


/***********************************************************************
 *     REGION_PtsToRegion
 *
 *     Create an array of rectangles from a list of points.
 */
static int REGION_PtsToRegion(int numFullPtBlocks, int iCurPtBlock, 
		       POINTBLOCK *FirstPtBlock, WINEREGION *reg)
{
    RECT *rects;
    POINT *pts;
    POINTBLOCK *CurPtBlock;
    int i;
    RECT *extents;
    INT numRects;
 
    extents = &reg->extents;
 
    numRects = ((numFullPtBlocks * NUMPTSTOBUFFER) + iCurPtBlock) >> 1;
 
    if (!(reg->rects = HeapReAlloc( SystemHeap, 0, reg->rects, 
			   sizeof(RECT) * numRects )))
        return(0);
 
    reg->size = numRects;
    CurPtBlock = FirstPtBlock;
    rects = reg->rects - 1;
    numRects = 0;
    extents->left = LARGE_COORDINATE,  extents->right = SMALL_COORDINATE;
 
    for ( ; numFullPtBlocks >= 0; numFullPtBlocks--) {
	/* the loop uses 2 points per iteration */
	i = NUMPTSTOBUFFER >> 1;
	if (!numFullPtBlocks)
	    i = iCurPtBlock >> 1;
	for (pts = CurPtBlock->pts; i--; pts += 2) {
	    if (pts->x == pts[1].x)
		continue;
	    if (numRects && pts->x == rects->left && pts->y == rects->bottom &&
		pts[1].x == rects->right &&
		(numRects == 1 || rects[-1].top != rects->top) &&
		(i && pts[2].y > pts[1].y)) {
		rects->bottom = pts[1].y + 1;
		continue;
	    }
	    numRects++;
	    rects++;
	    rects->left = pts->x;  rects->top = pts->y;
	    rects->right = pts[1].x;  rects->bottom = pts[1].y + 1;
	    if (rects->left < extents->left)
		extents->left = rects->left;
	    if (rects->right > extents->right)
		extents->right = rects->right;
        }
	CurPtBlock = CurPtBlock->next;
    }

    if (numRects) {
	extents->top = reg->rects->top;
	extents->bottom = rects->bottom;
    } else {
	extents->left = 0;
	extents->top = 0;
	extents->right = 0;
	extents->bottom = 0;
    }
    reg->numRects = numRects;
 
    return(TRUE);
}

/***********************************************************************
 *           CreatePolyPolygonRgn32    (GDI32.57)
 */
HRGN WINAPI CreatePolyPolygonRgn(const POINT *Pts, const INT *Count, 
		      INT nbpolygons, INT mode)
{
    HRGN hrgn;
    RGNOBJ *obj;
    WINEREGION *region;
    register EdgeTableEntry *pAET;   /* Active Edge Table       */
    register INT y;                /* current scanline        */
    register int iPts = 0;           /* number of pts in buffer */
    register EdgeTableEntry *pWETE;  /* Winding Edge Table Entry*/
    register ScanLineList *pSLL;     /* current scanLineList    */
    register POINT *pts;           /* output buffer           */
    EdgeTableEntry *pPrevAET;        /* ptr to previous AET     */
    EdgeTable ET;                    /* header node for ET      */
    EdgeTableEntry AET;              /* header node for AET     */
    EdgeTableEntry *pETEs;           /* EdgeTableEntries pool   */
    ScanLineListBlock SLLBlock;      /* header for scanlinelist */
    int fixWAET = FALSE;
    POINTBLOCK FirstPtBlock, *curPtBlock; /* PtBlock buffers    */
    POINTBLOCK *tmpPtBlock;
    int numFullPtBlocks = 0;
    INT poly, total;

    if(!(hrgn = REGION_CreateRegion(nbpolygons)))
        return 0;
    obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC );
    region = obj->rgn;

    /* special case a rectangle */

    if (((nbpolygons == 1) && ((*Count == 4) ||
       ((*Count == 5) && (Pts[4].x == Pts[0].x) && (Pts[4].y == Pts[0].y)))) &&
	(((Pts[0].y == Pts[1].y) &&
	  (Pts[1].x == Pts[2].x) &&
	  (Pts[2].y == Pts[3].y) &&
	  (Pts[3].x == Pts[0].x)) ||
	 ((Pts[0].x == Pts[1].x) &&
	  (Pts[1].y == Pts[2].y) &&
	  (Pts[2].x == Pts[3].x) &&
	  (Pts[3].y == Pts[0].y))))
    {
        SetRectRgn( hrgn, MIN(Pts[0].x, Pts[2].x), MIN(Pts[0].y, Pts[2].y), 
		            MAX(Pts[0].x, Pts[2].x), MAX(Pts[0].y, Pts[2].y) );
	GDI_HEAP_UNLOCK( hrgn );
	return hrgn;
    }
    
    for(poly = total = 0; poly < nbpolygons; poly++)
        total += Count[poly];
    if (! (pETEs = HeapAlloc( SystemHeap, 0, sizeof(EdgeTableEntry) * total )))
    {
        REGION_DeleteObject( hrgn, obj );
	return 0;
    }
    pts = FirstPtBlock.pts;
    REGION_CreateETandAET(Count, nbpolygons, Pts, &ET, &AET, pETEs, &SLLBlock);
    pSLL = ET.scanlines.next;
    curPtBlock = &FirstPtBlock;
 
    if (mode != WINDING) {
        /*
         *  for each scanline
         */
        for (y = ET.ymin; y < ET.ymax; y++) {
            /*
             *  Add a new edge to the active edge table when we
             *  get to the next edge.
             */
            if (pSLL != NULL && y == pSLL->scanline) {
                REGION_loadAET(&AET, pSLL->edgelist);
                pSLL = pSLL->next;
            }
            pPrevAET = &AET;
            pAET = AET.next;
 
            /*
             *  for each active edge
             */
            while (pAET) {
                pts->x = pAET->bres.minor_axis,  pts->y = y;
                pts++, iPts++;
 
                /*
                 *  send out the buffer
                 */
                if (iPts == NUMPTSTOBUFFER) {
                    tmpPtBlock = HeapAlloc( SystemHeap, 0, sizeof(POINTBLOCK));
		    if(!tmpPtBlock) {
		        WARN("Can't alloc tPB\n");
			return 0;
		    }
                    curPtBlock->next = tmpPtBlock;
                    curPtBlock = tmpPtBlock;
                    pts = curPtBlock->pts;
                    numFullPtBlocks++;
                    iPts = 0;
                }
                EVALUATEEDGEEVENODD(pAET, pPrevAET, y);
            }
            REGION_InsertionSort(&AET);
        }
    }
    else {
        /*
         *  for each scanline
         */
        for (y = ET.ymin; y < ET.ymax; y++) {
            /*
             *  Add a new edge to the active edge table when we
             *  get to the next edge.
             */
            if (pSLL != NULL && y == pSLL->scanline) {
                REGION_loadAET(&AET, pSLL->edgelist);
                REGION_computeWAET(&AET);
                pSLL = pSLL->next;
            }
            pPrevAET = &AET;
            pAET = AET.next;
            pWETE = pAET;
 
            /*
             *  for each active edge
             */
            while (pAET) {
                /*
                 *  add to the buffer only those edges that
                 *  are in the Winding active edge table.
                 */
                if (pWETE == pAET) {
                    pts->x = pAET->bres.minor_axis,  pts->y = y;
                    pts++, iPts++;
 
                    /*
                     *  send out the buffer
                     */
                    if (iPts == NUMPTSTOBUFFER) {
                        tmpPtBlock = HeapAlloc( SystemHeap, 0,
					       sizeof(POINTBLOCK) );
			if(!tmpPtBlock) {
			    WARN("Can't alloc tPB\n");
			    return 0;
			}
                        curPtBlock->next = tmpPtBlock;
                        curPtBlock = tmpPtBlock;
                        pts = curPtBlock->pts;
                        numFullPtBlocks++;    iPts = 0;
                    }
                    pWETE = pWETE->nextWETE;
                }
                EVALUATEEDGEWINDING(pAET, pPrevAET, y, fixWAET);
            }
 
            /*
             *  recompute the winding active edge table if
             *  we just resorted or have exited an edge.
             */
            if (REGION_InsertionSort(&AET) || fixWAET) {
                REGION_computeWAET(&AET);
                fixWAET = FALSE;
            }
        }
    }
    REGION_FreeStorage(SLLBlock.next);	
    REGION_PtsToRegion(numFullPtBlocks, iPts, &FirstPtBlock, region);
    for (curPtBlock = FirstPtBlock.next; --numFullPtBlocks >= 0;) {
	tmpPtBlock = curPtBlock->next;
	HeapFree( SystemHeap, 0, curPtBlock );
	curPtBlock = tmpPtBlock;
    }
    HeapFree( SystemHeap, 0, pETEs );
    GDI_HEAP_UNLOCK( hrgn );
    return hrgn;
}


/***********************************************************************
 *           CreatePolygonRgn16    (GDI.63)
 */
HRGN16 WINAPI CreatePolygonRgn16( const POINT16 * points, INT16 count,
                                  INT16 mode )
{
    return CreatePolyPolygonRgn16( points, &count, 1, mode );
}

/***********************************************************************
 *           CreatePolyPolygonRgn16    (GDI.451)
 */
HRGN16 WINAPI CreatePolyPolygonRgn16( const POINT16 *points,
                const INT16 *count, INT16 nbpolygons, INT16 mode )
{
    HRGN hrgn;
    int i, npts = 0;
    INT *count32;
    POINT *points32;

    for (i = 0; i < nbpolygons; i++)
	npts += count[i];
    points32 = HeapAlloc( SystemHeap, 0, npts * sizeof(POINT) );
    for (i = 0; i < npts; i++)
    	CONV_POINT16TO32( &(points[i]), &(points32[i]) );

    count32 = HeapAlloc( SystemHeap, 0, nbpolygons * sizeof(INT) );
    for (i = 0; i < nbpolygons; i++)
    	count32[i] = count[i];
    hrgn = CreatePolyPolygonRgn( points32, count32, nbpolygons, mode );
    HeapFree( SystemHeap, 0, count32 );
    HeapFree( SystemHeap, 0, points32 );
    return hrgn;
}

/***********************************************************************
 *           CreatePolygonRgn32    (GDI32.58)
 */
HRGN WINAPI CreatePolygonRgn( const POINT *points, INT count,
                                  INT mode )
{
    return CreatePolyPolygonRgn( points, &count, 1, mode );
}


/***********************************************************************
 * GetRandomRgn [GDI32.215]
 *
 * NOTES
 *     This function is UNDOCUMENTED, it isn't even in the header file.
 */
INT WINAPI GetRandomRgn(HDC hDC, HRGN hRgn, DWORD dwCode)
{
    DC *dc;

    FIXME("(0x%lx 0x%lx 0x%lx): empty stub!\n",
	   hDC, hRgn, dwCode);

    switch (dwCode)
    {
	case 1:
	    return GetClipRgn (hDC, hRgn);

	case 4:
	    dc = DC_GetDCPtr (hDC);
	    CombineRgn (hRgn, dc->w.hVisRgn, 0, RGN_COPY);
	    return 1;

	default:
	    return -1;
    }

    return -1;
}

/***********************************************************************
 *           REGION_CropAndOffsetRegion
 */
static BOOL REGION_CropAndOffsetRegion(const POINT* off, const RECT *rect, WINEREGION *rgnSrc, WINEREGION* rgnDst)
{

    if( !rect )	/* just copy and offset */
    {
        RECT *xrect;
	if( rgnDst == rgnSrc )
	{
	    if( off->x || off->y )
		xrect = rgnDst->rects;
	    else 
		return TRUE;
	}
	else
	    xrect = HeapReAlloc( SystemHeap, 0, rgnDst->rects,
				rgnSrc->size * sizeof( RECT ));
	if( xrect )
	{
	    INT i;

	    if( rgnDst != rgnSrc )
		memcpy( rgnDst, rgnSrc, sizeof( WINEREGION ));

	    if( off->x || off->y )
	    {
		for( i = 0; i < rgnDst->numRects; i++ )
		{
		    xrect[i].left = rgnSrc->rects[i].left + off->x;
		    xrect[i].right = rgnSrc->rects[i].right + off->x;
		    xrect[i].top = rgnSrc->rects[i].top + off->y;
		    xrect[i].bottom = rgnSrc->rects[i].bottom + off->y;
		}
		OffsetRect( &rgnDst->extents, off->x, off->y );
	    }
	    else
		memcpy( xrect, rgnSrc->rects, rgnDst->numRects * sizeof(RECT));
	    rgnDst->rects = xrect;
	} else
	    return FALSE;
    }
    else if( IsRectEmpty(rect) || !EXTENTCHECK(rect, &rgnSrc->extents) )
    {
empty:
	if( !rgnDst->rects )
	{
	    rgnDst->rects = HeapAlloc(SystemHeap, 0, RGN_DEFAULT_RECTS * sizeof( RECT ));
	    if( rgnDst->rects )
		rgnDst->size = RGN_DEFAULT_RECTS;
	    else
		return FALSE;
	}

	TRACE("cropped to empty!\n");
	EMPTY_REGION(rgnDst);
    }
    else /* region box and clipping rect appear to intersect */
    {
	RECT *lpr;
	INT i, j, clipa, clipb;
	INT left = rgnSrc->extents.right + off->x;
	INT right = rgnSrc->extents.left + off->x;

	for( clipa = 0; rgnSrc->rects[clipa].bottom <= rect->top; clipa++ )
	     ; /* skip bands above the clipping rectangle */

	for( clipb = clipa; clipb < rgnSrc->numRects; clipb++ )
	     if( rgnSrc->rects[clipb].top >= rect->bottom )
		 break;    /* and below it */

	/* clipa - index of the first rect in the first intersecting band
	 * clipb - index of the last rect in the last intersecting band
	 */

	if((rgnDst != rgnSrc) && (rgnDst->size < (i = (clipb - clipa))))
	{
	    rgnDst->rects = HeapReAlloc( SystemHeap, 0, 
				rgnDst->rects, i * sizeof(RECT));
	    if( !rgnDst->rects ) return FALSE;
	    rgnDst->size = i;
	}

	if( TRACE_ON(region) )
	{
	    REGION_DumpRegion( rgnSrc );
	    TRACE("\tclipa = %i, clipb = %i\n", clipa, clipb );
	}

	for( i = clipa, j = 0; i < clipb ; i++ )
	{
	     /* i - src index, j - dst index, j is always <= i for obvious reasons */

	     lpr = rgnSrc->rects + i;
	     if( lpr->left < rect->right && lpr->right > rect->left )
	     {
		 rgnDst->rects[j].top = lpr->top + off->y;
		 rgnDst->rects[j].bottom = lpr->bottom + off->y;
		 rgnDst->rects[j].left = ((lpr->left > rect->left) ? lpr->left : rect->left) + off->x;
		 rgnDst->rects[j].right = ((lpr->right < rect->right) ? lpr->right : rect->right) + off->x;

		 if( rgnDst->rects[j].left < left ) left = rgnDst->rects[j].left;
		 if( rgnDst->rects[j].right > right ) right = rgnDst->rects[j].right;

		 j++;
	     }
	}

	if( j == 0 ) goto empty;

        rgnDst->extents.left = left;
        rgnDst->extents.right = right;

	left = rect->top + off->y;
	right = rect->bottom + off->y;

	rgnDst->numRects = j--;
	for( i = 0; i <= j; i++ )	/* fixup top band */
	     if( rgnDst->rects[i].top < left )
		 rgnDst->rects[i].top = left;
	     else
		 break;

	for( i = j; i >= 0; i-- )	/* fixup bottom band */
	     if( rgnDst->rects[i].bottom > right )
		 rgnDst->rects[i].bottom = right;
	     else
		 break;

	rgnDst->extents.top = rgnDst->rects[0].top;
	rgnDst->extents.bottom = rgnDst->rects[j].bottom;

	rgnDst->type = (j >= 1) ? COMPLEXREGION : SIMPLEREGION;

	if( TRACE_ON(region) )
	{
	    TRACE("result:\n");
	    REGION_DumpRegion( rgnDst );
	}
    }

    return TRUE;
}

/***********************************************************************
 *           REGION_CropRgn
 *
 *
 * hSrc: 	Region to crop and offset.
 * lpRect: 	Clipping rectangle. Can be NULL (no clipping).
 * lpPt:	Points to offset the cropped region. Can be NULL (no offset).
 *
 * hDst: Region to hold the result (a new region is created if it's 0).
 *       Allowed to be the same region as hSrc in which case everything
 *	 will be done in place, with no memory reallocations.
 *
 * Returns: hDst if success, 0 otherwise.
 */
HRGN REGION_CropRgn( HRGN hDst, HRGN hSrc, const RECT *lpRect, const POINT *lpPt )
{
/*  Optimization of the following generic code:

    HRGN h; 

    if( lpRect ) 
	h = CreateRectRgn( lpRect->left, lpRect->top, lpRect->right, lpRect->bottom );
    else
	h = CreateRectRgn( 0, 0, 0, 0 );
    if( hDst == 0 ) hDst = h;
    if( lpRect )
	CombineRgn( hDst, hSrc, h, RGN_AND );
    else
	CombineRgn( hDst, hSrc, 0, RGN_COPY );
    if( lpPt )
	OffsetRgn( hDst, lpPt->x, lpPt->y );
    if( hDst != h )
	DeleteObject( h );
    return hDst;

*/

    RGNOBJ *objSrc = (RGNOBJ *) GDI_GetObjPtr( hSrc, REGION_MAGIC );

    if(objSrc)
    {
 	RGNOBJ *objDst;
	WINEREGION *rgnDst;

	if( hDst )
	{
	    if (!(objDst = (RGNOBJ *) GDI_GetObjPtr( hDst, REGION_MAGIC )))
	    {
	       hDst = 0;
	       goto done;
	    }
	    rgnDst = objDst->rgn;
	}
	else
	{
	    if ((rgnDst = HeapAlloc(SystemHeap, 0, sizeof( WINEREGION ))))
	    {
	        rgnDst->size = rgnDst->numRects = 0;
	        rgnDst->rects = NULL;	/* back end will allocate exact number */
	    }
	}

	if( rgnDst )
	{
	    POINT pt = { 0, 0 };

	    if( !lpPt ) lpPt = &pt;

	    if( lpRect )
	        TRACE("src %p -> dst %p (%i,%i)-(%i,%i) by (%li,%li)\n", objSrc->rgn, rgnDst,
		      lpRect->left, lpRect->top, lpRect->right, lpRect->bottom, lpPt->x, lpPt->y );
	    else
		TRACE("src %p -> dst %p by (%li,%li)\n", objSrc->rgn, rgnDst, lpPt->x, lpPt->y ); 

	    if( REGION_CropAndOffsetRegion( lpPt, lpRect, objSrc->rgn, rgnDst ) == FALSE )
	    {
		if( hDst ) /* existing rgn */
		{
		    GDI_HEAP_UNLOCK(hDst);
		    hDst = 0;
		    goto done;
		}
		goto fail;
	    }
	    else if( hDst == 0 )
	    {
		if(!(hDst = GDI_AllocObject( sizeof(RGNOBJ), REGION_MAGIC )))
		{
fail:
		    if( rgnDst->rects )
			HeapFree( SystemHeap, 0, rgnDst->rects );
		    HeapFree( SystemHeap, 0, rgnDst );
		    goto done;
		}

		objDst = (RGNOBJ *) GDI_HEAP_LOCK( hDst );
		objDst->rgn = rgnDst;
	    }

	    GDI_HEAP_UNLOCK(hDst);
	}
	else hDst = 0;
done:
	GDI_HEAP_UNLOCK(hSrc);
	return hDst;
    }
    return 0;
}

