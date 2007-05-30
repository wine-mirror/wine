/*
 * GDI region objects. Shamelessly ripped out from the X11 distribution
 * Thanks for the nice licence.
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 * Modifications and additions: Copyright 1998 Huw Davies
 *					  1999 Alex Korobka
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(region);

typedef struct {
    INT size;
    INT numRects;
    RECT *rects;
    RECT extents;
} WINEREGION;

  /* GDI logical region object */
typedef struct
{
    GDIOBJHDR   header;
    WINEREGION  *rgn;
} RGNOBJ;


static HGDIOBJ REGION_SelectObject( HGDIOBJ handle, void *obj, HDC hdc );
static BOOL REGION_DeleteObject( HGDIOBJ handle, void *obj );

static const struct gdi_obj_funcs region_funcs =
{
    REGION_SelectObject,  /* pSelectObject */
    NULL,                 /* pGetObject16 */
    NULL,                 /* pGetObjectA */
    NULL,                 /* pGetObjectW */
    NULL,                 /* pUnrealizeObject */
    REGION_DeleteObject   /* pDeleteObject */
};

/*  1 if two RECTs overlap.
 *  0 if two RECTs do not overlap.
 */
#define EXTENTCHECK(r1, r2) \
	((r1)->right > (r2)->left && \
	 (r1)->left < (r2)->right && \
	 (r1)->bottom > (r2)->top && \
	 (r1)->top < (r2)->bottom)

/*
 *   Check to see if there is enough memory in the present region.
 */

static inline int xmemcheck(WINEREGION *reg, LPRECT *rect, LPRECT *firstrect ) {
    if (reg->numRects >= (reg->size - 1)) {
	*firstrect = HeapReAlloc( GetProcessHeap(), 0, *firstrect, (2 * (sizeof(RECT)) * (reg->size)));
	if (*firstrect == 0)
	    return 0;
	reg->size *= 2;
	*rect = (*firstrect)+reg->numRects;
    }
    return 1;
}

#define MEMCHECK(reg, rect, firstrect) xmemcheck(reg,&(rect),&(firstrect))

#define EMPTY_REGION(pReg) { \
    (pReg)->numRects = 0; \
    (pReg)->extents.left = (pReg)->extents.top = 0; \
    (pReg)->extents.right = (pReg)->extents.bottom = 0; \
 }

#define REGION_NOT_EMPTY(pReg) pReg->numRects

#define INRECT(r, x, y) \
      ( ( ((r).right >  x)) && \
        ( ((r).left <= x)) && \
        ( ((r).bottom >  y)) && \
        ( ((r).top <= y)) )


/*
 * number of points to buffer before sending them off
 * to scanlines() :  Must be an even number
 */
#define NUMPTSTOBUFFER 200

/*
 * used to allocate buffers for points and link
 * the buffers together
 */

typedef struct _POINTBLOCK {
    POINT pts[NUMPTSTOBUFFER];
    struct _POINTBLOCK *next;
} POINTBLOCK;



/*
 *     This file contains a few macros to help track
 *     the edge of a filled object.  The object is assumed
 *     to be filled in scanline order, and thus the
 *     algorithm used is an extension of Bresenham's line
 *     drawing algorithm which assumes that y is always the
 *     major axis.
 *     Since these pieces of code are the same for any filled shape,
 *     it is more convenient to gather the library in one
 *     place, but since these pieces of code are also in
 *     the inner loops of output primitives, procedure call
 *     overhead is out of the question.
 *     See the author for a derivation if needed.
 */


/*
 *  In scan converting polygons, we want to choose those pixels
 *  which are inside the polygon.  Thus, we add .5 to the starting
 *  x coordinate for both left and right edges.  Now we choose the
 *  first pixel which is inside the pgon for the left edge and the
 *  first pixel which is outside the pgon for the right edge.
 *  Draw the left pixel, but not the right.
 *
 *  How to add .5 to the starting x coordinate:
 *      If the edge is moving to the right, then subtract dy from the
 *  error term from the general form of the algorithm.
 *      If the edge is moving to the left, then add dy to the error term.
 *
 *  The reason for the difference between edges moving to the left
 *  and edges moving to the right is simple:  If an edge is moving
 *  to the right, then we want the algorithm to flip immediately.
 *  If it is moving to the left, then we don't want it to flip until
 *  we traverse an entire pixel.
 */
#define BRESINITPGON(dy, x1, x2, xStart, d, m, m1, incr1, incr2) { \
    int dx;      /* local storage */ \
\
    /* \
     *  if the edge is horizontal, then it is ignored \
     *  and assumed not to be processed.  Otherwise, do this stuff. \
     */ \
    if ((dy) != 0) { \
        xStart = (x1); \
        dx = (x2) - xStart; \
        if (dx < 0) { \
            m = dx / (dy); \
            m1 = m - 1; \
            incr1 = -2 * dx + 2 * (dy) * m1; \
            incr2 = -2 * dx + 2 * (dy) * m; \
            d = 2 * m * (dy) - 2 * dx - 2 * (dy); \
        } else { \
            m = dx / (dy); \
            m1 = m + 1; \
            incr1 = 2 * dx - 2 * (dy) * m1; \
            incr2 = 2 * dx - 2 * (dy) * m; \
            d = -2 * m * (dy) + 2 * dx; \
        } \
    } \
}

#define BRESINCRPGON(d, minval, m, m1, incr1, incr2) { \
    if (m1 > 0) { \
        if (d > 0) { \
            minval += m1; \
            d += incr1; \
        } \
        else { \
            minval += m; \
            d += incr2; \
        } \
    } else {\
        if (d >= 0) { \
            minval += m1; \
            d += incr1; \
        } \
        else { \
            minval += m; \
            d += incr2; \
        } \
    } \
}

/*
 *     This structure contains all of the information needed
 *     to run the bresenham algorithm.
 *     The variables may be hardcoded into the declarations
 *     instead of using this structure to make use of
 *     register declarations.
 */
typedef struct {
    INT minor_axis;	/* minor axis        */
    INT d;		/* decision variable */
    INT m, m1;       	/* slope and slope+1 */
    INT incr1, incr2;	/* error increments */
} BRESINFO;


#define BRESINITPGONSTRUCT(dmaj, min1, min2, bres) \
	BRESINITPGON(dmaj, min1, min2, bres.minor_axis, bres.d, \
                     bres.m, bres.m1, bres.incr1, bres.incr2)

#define BRESINCRPGONSTRUCT(bres) \
        BRESINCRPGON(bres.d, bres.minor_axis, bres.m, bres.m1, bres.incr1, bres.incr2)



/*
 *     These are the data structures needed to scan
 *     convert regions.  Two different scan conversion
 *     methods are available -- the even-odd method, and
 *     the winding number method.
 *     The even-odd rule states that a point is inside
 *     the polygon if a ray drawn from that point in any
 *     direction will pass through an odd number of
 *     path segments.
 *     By the winding number rule, a point is decided
 *     to be inside the polygon if a ray drawn from that
 *     point in any direction passes through a different
 *     number of clockwise and counter-clockwise path
 *     segments.
 *
 *     These data structures are adapted somewhat from
 *     the algorithm in (Foley/Van Dam) for scan converting
 *     polygons.
 *     The basic algorithm is to start at the top (smallest y)
 *     of the polygon, stepping down to the bottom of
 *     the polygon by incrementing the y coordinate.  We
 *     keep a list of edges which the current scanline crosses,
 *     sorted by x.  This list is called the Active Edge Table (AET)
 *     As we change the y-coordinate, we update each entry in
 *     in the active edge table to reflect the edges new xcoord.
 *     This list must be sorted at each scanline in case
 *     two edges intersect.
 *     We also keep a data structure known as the Edge Table (ET),
 *     which keeps track of all the edges which the current
 *     scanline has not yet reached.  The ET is basically a
 *     list of ScanLineList structures containing a list of
 *     edges which are entered at a given scanline.  There is one
 *     ScanLineList per scanline at which an edge is entered.
 *     When we enter a new edge, we move it from the ET to the AET.
 *
 *     From the AET, we can implement the even-odd rule as in
 *     (Foley/Van Dam).
 *     The winding number rule is a little trickier.  We also
 *     keep the EdgeTableEntries in the AET linked by the
 *     nextWETE (winding EdgeTableEntry) link.  This allows
 *     the edges to be linked just as before for updating
 *     purposes, but only uses the edges linked by the nextWETE
 *     link as edges representing spans of the polygon to
 *     drawn (as with the even-odd rule).
 */

/*
 * for the winding number rule
 */
#define CLOCKWISE          1
#define COUNTERCLOCKWISE  -1

typedef struct _EdgeTableEntry {
     INT ymax;           /* ycoord at which we exit this edge. */
     BRESINFO bres;        /* Bresenham info to run the edge     */
     struct _EdgeTableEntry *next;       /* next in the list     */
     struct _EdgeTableEntry *back;       /* for insertion sort   */
     struct _EdgeTableEntry *nextWETE;   /* for winding num rule */
     int ClockWise;        /* flag for winding number rule       */
} EdgeTableEntry;


typedef struct _ScanLineList{
     INT scanline;            /* the scanline represented */
     EdgeTableEntry *edgelist;  /* header node              */
     struct _ScanLineList *next;  /* next in the list       */
} ScanLineList;


typedef struct {
     INT ymax;               /* ymax for the polygon     */
     INT ymin;               /* ymin for the polygon     */
     ScanLineList scanlines;   /* header node              */
} EdgeTable;


/*
 * Here is a struct to help with storage allocation
 * so we can allocate a big chunk at a time, and then take
 * pieces from this heap when we need to.
 */
#define SLLSPERBLOCK 25

typedef struct _ScanLineListBlock {
     ScanLineList SLLs[SLLSPERBLOCK];
     struct _ScanLineListBlock *next;
} ScanLineListBlock;


/*
 *
 *     a few macros for the inner loops of the fill code where
 *     performance considerations don't allow a procedure call.
 *
 *     Evaluate the given edge at the given scanline.
 *     If the edge has expired, then we leave it and fix up
 *     the active edge table; otherwise, we increment the
 *     x value to be ready for the next scanline.
 *     The winding number rule is in effect, so we must notify
 *     the caller when the edge has been removed so he
 *     can reorder the Winding Active Edge Table.
 */
#define EVALUATEEDGEWINDING(pAET, pPrevAET, y, fixWAET) { \
   if (pAET->ymax == y) {          /* leaving this edge */ \
      pPrevAET->next = pAET->next; \
      pAET = pPrevAET->next; \
      fixWAET = 1; \
      if (pAET) \
         pAET->back = pPrevAET; \
   } \
   else { \
      BRESINCRPGONSTRUCT(pAET->bres); \
      pPrevAET = pAET; \
      pAET = pAET->next; \
   } \
}


/*
 *     Evaluate the given edge at the given scanline.
 *     If the edge has expired, then we leave it and fix up
 *     the active edge table; otherwise, we increment the
 *     x value to be ready for the next scanline.
 *     The even-odd rule is in effect.
 */
#define EVALUATEEDGEEVENODD(pAET, pPrevAET, y) { \
   if (pAET->ymax == y) {          /* leaving this edge */ \
      pPrevAET->next = pAET->next; \
      pAET = pPrevAET->next; \
      if (pAET) \
         pAET->back = pPrevAET; \
   } \
   else { \
      BRESINCRPGONSTRUCT(pAET->bres); \
      pPrevAET = pAET; \
      pAET = pAET->next; \
   } \
}

/* Note the parameter order is different from the X11 equivalents */

static void REGION_CopyRegion(WINEREGION *d, WINEREGION *s);
static void REGION_OffsetRegion(WINEREGION *d, WINEREGION *s, INT x, INT y);
static void REGION_IntersectRegion(WINEREGION *d, WINEREGION *s1, WINEREGION *s2);
static void REGION_UnionRegion(WINEREGION *d, WINEREGION *s1, WINEREGION *s2);
static void REGION_SubtractRegion(WINEREGION *d, WINEREGION *s1, WINEREGION *s2);
static void REGION_XorRegion(WINEREGION *d, WINEREGION *s1, WINEREGION *s2);
static void REGION_UnionRectWithRegion(const RECT *rect, WINEREGION *rgn);

#define RGN_DEFAULT_RECTS	2


/***********************************************************************
 *            get_region_type
 */
static inline INT get_region_type( const RGNOBJ *obj )
{
    switch(obj->rgn->numRects)
    {
    case 0:  return NULLREGION;
    case 1:  return SIMPLEREGION;
    default: return COMPLEXREGION;
    }
}


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

    if ((pReg = HeapAlloc(GetProcessHeap(), 0, sizeof( WINEREGION ))))
    {
        if ((pReg->rects = HeapAlloc(GetProcessHeap(), 0, n * sizeof( RECT ))))
        {
            pReg->size = n;
            EMPTY_REGION(pReg);
            return pReg;
        }
        HeapFree(GetProcessHeap(), 0, pReg);
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

    if(!(obj = GDI_AllocObject( sizeof(RGNOBJ), REGION_MAGIC, (HGDIOBJ *)&hrgn,
				&region_funcs ))) return 0;
    if(!(obj->rgn = REGION_AllocWineRegion(n))) {
        GDI_FreeObject( hrgn, obj );
        return 0;
    }
    GDI_ReleaseObj( hrgn );
    return hrgn;
}

/***********************************************************************
 *           REGION_DestroyWineRegion
 */
static void REGION_DestroyWineRegion( WINEREGION* pReg )
{
    HeapFree( GetProcessHeap(), 0, pReg->rects );
    HeapFree( GetProcessHeap(), 0, pReg );
}

/***********************************************************************
 *           REGION_DeleteObject
 */
static BOOL REGION_DeleteObject( HGDIOBJ handle, void *obj )
{
    RGNOBJ *rgn = obj;

    TRACE(" %p\n", handle );

    REGION_DestroyWineRegion( rgn->rgn );
    return GDI_FreeObject( handle, obj );
}

/***********************************************************************
 *           REGION_SelectObject
 */
static HGDIOBJ REGION_SelectObject( HGDIOBJ handle, void *obj, HDC hdc )
{
    return ULongToHandle(SelectClipRgn( hdc, handle ));
}


/***********************************************************************
 *           REGION_OffsetRegion
 *           Offset a WINEREGION by x,y
 */
static void REGION_OffsetRegion( WINEREGION *rgn, WINEREGION *srcrgn,
                                INT x, INT y )
{
    if( rgn != srcrgn)
        REGION_CopyRegion( rgn, srcrgn);
    if(x || y) {
	int nbox = rgn->numRects;
	RECT *pbox = rgn->rects;

	if(nbox) {
	    while(nbox--) {
	        pbox->left += x;
		pbox->right += x;
		pbox->top += y;
		pbox->bottom += y;
		pbox++;
	    }
	    rgn->extents.left += x;
	    rgn->extents.right += x;
	    rgn->extents.top += y;
	    rgn->extents.bottom += y;
	}
    }
}

/***********************************************************************
 *           OffsetRgn   (GDI32.@)
 *
 * Moves a region by the specified X- and Y-axis offsets.
 *
 * PARAMS
 *   hrgn [I] Region to offset.
 *   x    [I] Offset right if positive or left if negative.
 *   y    [I] Offset down if positive or up if negative.
 *
 * RETURNS
 *   Success:
 *     NULLREGION - The new region is empty.
 *     SIMPLEREGION - The new region can be represented by one rectangle.
 *     COMPLEXREGION - The new region can only be represented by more than
 *                     one rectangle.
 *   Failure: ERROR
 */
INT WINAPI OffsetRgn( HRGN hrgn, INT x, INT y )
{
    RGNOBJ * obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC );
    INT ret;

    TRACE("%p %d,%d\n", hrgn, x, y);

    if (!obj)
        return ERROR;

    REGION_OffsetRegion( obj->rgn, obj->rgn, x, y);

    ret = get_region_type( obj );
    GDI_ReleaseObj( hrgn );
    return ret;
}


/***********************************************************************
 *           GetRgnBox    (GDI32.@)
 *
 * Retrieves the bounding rectangle of the region. The bounding rectangle
 * is the smallest rectangle that contains the entire region.
 *
 * PARAMS
 *   hrgn [I] Region to retrieve bounding rectangle from.
 *   rect [O] Rectangle that will receive the coordinates of the bounding
 *            rectangle.
 *
 * RETURNS
 *     NULLREGION - The new region is empty.
 *     SIMPLEREGION - The new region can be represented by one rectangle.
 *     COMPLEXREGION - The new region can only be represented by more than
 *                     one rectangle.
 */
INT WINAPI GetRgnBox( HRGN hrgn, LPRECT rect )
{
    RGNOBJ * obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC );
    if (obj)
    {
	INT ret;
	rect->left = obj->rgn->extents.left;
	rect->top = obj->rgn->extents.top;
	rect->right = obj->rgn->extents.right;
	rect->bottom = obj->rgn->extents.bottom;
	TRACE("%p (%d,%d-%d,%d)\n", hrgn,
               rect->left, rect->top, rect->right, rect->bottom);
	ret = get_region_type( obj );
	GDI_ReleaseObj(hrgn);
	return ret;
    }
    return ERROR;
}


/***********************************************************************
 *           CreateRectRgn   (GDI32.@)
 *
 * Creates a simple rectangular region.
 *
 * PARAMS
 *   left   [I] Left coordinate of rectangle.
 *   top    [I] Top coordinate of rectangle.
 *   right  [I] Right coordinate of rectangle.
 *   bottom [I] Bottom coordinate of rectangle.
 *
 * RETURNS
 *   Success: Handle to region.
 *   Failure: NULL.
 */
HRGN WINAPI CreateRectRgn(INT left, INT top, INT right, INT bottom)
{
    HRGN hrgn;

    /* Allocate 2 rects by default to reduce the number of reallocs */

    if (!(hrgn = REGION_CreateRegion(RGN_DEFAULT_RECTS)))
	return 0;
    TRACE("%d,%d-%d,%d\n", left, top, right, bottom);
    SetRectRgn(hrgn, left, top, right, bottom);
    return hrgn;
}


/***********************************************************************
 *           CreateRectRgnIndirect    (GDI32.@)
 *
 * Creates a simple rectangular region.
 *
 * PARAMS
 *   rect [I] Coordinates of rectangular region.
 *
 * RETURNS
 *   Success: Handle to region.
 *   Failure: NULL.
 */
HRGN WINAPI CreateRectRgnIndirect( const RECT* rect )
{
    return CreateRectRgn( rect->left, rect->top, rect->right, rect->bottom );
}


/***********************************************************************
 *           SetRectRgn    (GDI32.@)
 *
 * Sets a region to a simple rectangular region.
 *
 * PARAMS
 *   hrgn   [I] Region to convert.
 *   left   [I] Left coordinate of rectangle.
 *   top    [I] Top coordinate of rectangle.
 *   right  [I] Right coordinate of rectangle.
 *   bottom [I] Bottom coordinate of rectangle.
 *
 * RETURNS
 *   Success: Non-zero.
 *   Failure: Zero.
 *
 * NOTES
 *   Allows either or both left and top to be greater than right or bottom.
 */
BOOL WINAPI SetRectRgn( HRGN hrgn, INT left, INT top,
			  INT right, INT bottom )
{
    RGNOBJ * obj;

    TRACE("%p %d,%d-%d,%d\n", hrgn, left, top, right, bottom );

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
    }
    else
	EMPTY_REGION(obj->rgn);

    GDI_ReleaseObj( hrgn );
    return TRUE;
}


/***********************************************************************
 *           CreateRoundRectRgn    (GDI32.@)
 *
 * Creates a rectangular region with rounded corners.
 *
 * PARAMS
 *   left           [I] Left coordinate of rectangle.
 *   top            [I] Top coordinate of rectangle.
 *   right          [I] Right coordinate of rectangle.
 *   bottom         [I] Bottom coordinate of rectangle.
 *   ellipse_width  [I] Width of the ellipse at each corner.
 *   ellipse_height [I] Height of the ellipse at each corner.
 *
 * RETURNS
 *   Success: Handle to region.
 *   Failure: NULL.
 *
 * NOTES
 *   If ellipse_width or ellipse_height is less than 2 logical units then
 *   it is treated as though CreateRectRgn() was called instead.
 */
HRGN WINAPI CreateRoundRectRgn( INT left, INT top,
				    INT right, INT bottom,
				    INT ellipse_width, INT ellipse_height )
{
    RGNOBJ * obj;
    HRGN hrgn;
    int asq, bsq, d, xd, yd;
    RECT rect;

      /* Make the dimensions sensible */

    if (left > right) { INT tmp = left; left = right; right = tmp; }
    if (top > bottom) { INT tmp = top; top = bottom; bottom = tmp; }

    ellipse_width = abs(ellipse_width);
    ellipse_height = abs(ellipse_height);

      /* Check parameters */

    if (ellipse_width > right-left) ellipse_width = right-left;
    if (ellipse_height > bottom-top) ellipse_height = bottom-top;

      /* Check if we can do a normal rectangle instead */

    if ((ellipse_width < 2) || (ellipse_height < 2))
        return CreateRectRgn( left, top, right, bottom );

      /* Create region */

    d = (ellipse_height < 128) ? ((3 * ellipse_height) >> 2) : 64;
    if (!(hrgn = REGION_CreateRegion(d))) return 0;
    if (!(obj = GDI_GetObjPtr( hrgn, REGION_MAGIC ))) return 0;
    TRACE("(%d,%d-%d,%d %dx%d): ret=%p\n",
	  left, top, right, bottom, ellipse_width, ellipse_height, hrgn );

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
    GDI_ReleaseObj( hrgn );
    return hrgn;
}


/***********************************************************************
 *           CreateEllipticRgn    (GDI32.@)
 *
 * Creates an elliptical region.
 *
 * PARAMS
 *   left   [I] Left coordinate of bounding rectangle.
 *   top    [I] Top coordinate of bounding rectangle.
 *   right  [I] Right coordinate of bounding rectangle.
 *   bottom [I] Bottom coordinate of bounding rectangle.
 *
 * RETURNS
 *   Success: Handle to region.
 *   Failure: NULL.
 *
 * NOTES
 *   This is a special case of CreateRoundRectRgn() where the width of the
 *   ellipse at each corner is equal to the width the the rectangle and
 *   the same for the height.
 */
HRGN WINAPI CreateEllipticRgn( INT left, INT top,
				   INT right, INT bottom )
{
    return CreateRoundRectRgn( left, top, right, bottom,
				 right-left, bottom-top );
}


/***********************************************************************
 *           CreateEllipticRgnIndirect    (GDI32.@)
 *
 * Creates an elliptical region.
 *
 * PARAMS
 *   rect [I] Pointer to bounding rectangle of the ellipse.
 *
 * RETURNS
 *   Success: Handle to region.
 *   Failure: NULL.
 *
 * NOTES
 *   This is a special case of CreateRoundRectRgn() where the width of the
 *   ellipse at each corner is equal to the width the the rectangle and
 *   the same for the height.
 */
HRGN WINAPI CreateEllipticRgnIndirect( const RECT *rect )
{
    return CreateRoundRectRgn( rect->left, rect->top, rect->right,
				 rect->bottom, rect->right - rect->left,
				 rect->bottom - rect->top );
}

/***********************************************************************
 *           GetRegionData   (GDI32.@)
 *
 * Retrieves the data that specifies the region.
 *
 * PARAMS
 *   hrgn    [I] Region to retrieve the region data from.
 *   count   [I] The size of the buffer pointed to by rgndata in bytes.
 *   rgndata [I] The buffer to receive data about the region.
 *
 * RETURNS
 *   Success: If rgndata is NULL then the required number of bytes. Otherwise,
 *            the number of bytes copied to the output buffer.
 *   Failure: 0.
 *
 * NOTES
 *   The format of the Buffer member of RGNDATA is determined by the iType
 *   member of the region data header.
 *   Currently this is always RDH_RECTANGLES, which specifies that the format
 *   is the array of RECT's that specify the region. The length of the array
 *   is specified by the nCount member of the region data header.
 */
DWORD WINAPI GetRegionData(HRGN hrgn, DWORD count, LPRGNDATA rgndata)
{
    DWORD size;
    RGNOBJ *obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC );

    TRACE(" %p count = %d, rgndata = %p\n", hrgn, count, rgndata);

    if(!obj) return 0;

    size = obj->rgn->numRects * sizeof(RECT);
    if(count < (size + sizeof(RGNDATAHEADER)) || rgndata == NULL)
    {
        GDI_ReleaseObj( hrgn );
	if (rgndata) /* buffer is too small, signal it by return 0 */
	    return 0;
	else		/* user requested buffer size with rgndata NULL */
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

    GDI_ReleaseObj( hrgn );
    return size + sizeof(RGNDATAHEADER);
}


/***********************************************************************
 *           ExtCreateRegion   (GDI32.@)
 *
 * Creates a region as specified by the transformation data and region data.
 *
 * PARAMS
 *   lpXform [I] World-space to logical-space transformation data.
 *   dwCount [I] Size of the data pointed to by rgndata, in bytes.
 *   rgndata [I] Data that specifes the region.
 *
 * RETURNS
 *   Success: Handle to region.
 *   Failure: NULL.
 *
 * NOTES
 *   See GetRegionData().
 */
HRGN WINAPI ExtCreateRegion( const XFORM* lpXform, DWORD dwCount, const RGNDATA* rgndata)
{
    HRGN hrgn;

    TRACE(" %p %d %p\n", lpXform, dwCount, rgndata );

    if( lpXform )
        WARN("(Xform not implemented - ignored)\n");

    if( rgndata->rdh.iType != RDH_RECTANGLES )
    {
	/* FIXME: We can use CreatePolyPolygonRgn() here
	 *        for trapezoidal data */

        WARN("(Unsupported region data type: %u)\n", rgndata->rdh.iType);
	goto fail;
    }

    if( (hrgn = REGION_CreateRegion( rgndata->rdh.nCount )) )
    {
	RECT *pCurRect, *pEndRect;
	RGNOBJ *obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC );

	if (obj) {
            pEndRect = (RECT *)rgndata->Buffer + rgndata->rdh.nCount;
            for(pCurRect = (RECT *)rgndata->Buffer; pCurRect < pEndRect; pCurRect++)
            {
                if (pCurRect->left < pCurRect->right && pCurRect->top < pCurRect->bottom)
                    REGION_UnionRectWithRegion( pCurRect, obj->rgn );
            }
	    GDI_ReleaseObj( hrgn );

            TRACE("-- %p\n", hrgn );
            return hrgn;
        }
	else ERR("Could not get pointer to newborn Region!\n");
    }
fail:
    WARN("Failed\n");
    return 0;
}


/***********************************************************************
 *           PtInRegion    (GDI32.@)
 *
 * Tests whether the specified point is inside a region.
 *
 * PARAMS
 *   hrgn [I] Region to test.
 *   x    [I] X-coordinate of point to test.
 *   y    [I] Y-coordinate of point to test.
 *
 * RETURNS
 *   Non-zero if the point is inside the region or zero otherwise.
 */
BOOL WINAPI PtInRegion( HRGN hrgn, INT x, INT y )
{
    RGNOBJ * obj;
    BOOL ret = FALSE;

    if ((obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC )))
    {
	int i;

	if (obj->rgn->numRects > 0 && INRECT(obj->rgn->extents, x, y))
	    for (i = 0; i < obj->rgn->numRects; i++)
		if (INRECT (obj->rgn->rects[i], x, y))
                {
		    ret = TRUE;
                    break;
                }
	GDI_ReleaseObj( hrgn );
    }
    return ret;
}


/***********************************************************************
 *           RectInRegion    (GDI32.@)
 *
 * Tests if a rectangle is at least partly inside the specified region.
 *
 * PARAMS
 *   hrgn [I] Region to test.
 *   rect [I] Rectangle to test.
 *
 * RETURNS
 *   Non-zero if the rectangle is partially inside the region or
 *   zero otherwise.
 */
BOOL WINAPI RectInRegion( HRGN hrgn, const RECT *rect )
{
    RGNOBJ * obj;
    BOOL ret = FALSE;

    if ((obj = (RGNOBJ *) GDI_GetObjPtr( hrgn, REGION_MAGIC )))
    {
	RECT *pCurRect, *pRectEnd;

    /* this is (just) a useful optimization */
	if ((obj->rgn->numRects > 0) && EXTENTCHECK(&obj->rgn->extents,
						      rect))
	{
	    for (pCurRect = obj->rgn->rects, pRectEnd = pCurRect +
	     obj->rgn->numRects; pCurRect < pRectEnd; pCurRect++)
	    {
	        if (pCurRect->bottom <= rect->top)
		    continue;             /* not far enough down yet */

		if (pCurRect->top >= rect->bottom)
		    break;                /* too far down */

		if (pCurRect->right <= rect->left)
		    continue;              /* not far enough over yet */

		if (pCurRect->left >= rect->right) {
		    continue;
		}

		ret = TRUE;
		break;
	    }
	}
	GDI_ReleaseObj(hrgn);
    }
    return ret;
}

/***********************************************************************
 *           EqualRgn    (GDI32.@)
 *
 * Tests whether one region is identical to another.
 *
 * PARAMS
 *   hrgn1 [I] The first region to compare.
 *   hrgn2 [I] The second region to compare.
 *
 * RETURNS
 *   Non-zero if both regions are identical or zero otherwise.
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

	    if ( obj1->rgn->numRects != obj2->rgn->numRects ) goto done;
            if ( obj1->rgn->numRects == 0 )
            {
                ret = TRUE;
                goto done;

            }
            if (obj1->rgn->extents.left   != obj2->rgn->extents.left) goto done;
            if (obj1->rgn->extents.right  != obj2->rgn->extents.right) goto done;
            if (obj1->rgn->extents.top    != obj2->rgn->extents.top) goto done;
            if (obj1->rgn->extents.bottom != obj2->rgn->extents.bottom) goto done;
            for( i = 0; i < obj1->rgn->numRects; i++ )
            {
                if (obj1->rgn->rects[i].left   != obj2->rgn->rects[i].left) goto done;
                if (obj1->rgn->rects[i].right  != obj2->rgn->rects[i].right) goto done;
                if (obj1->rgn->rects[i].top    != obj2->rgn->rects[i].top) goto done;
                if (obj1->rgn->rects[i].bottom != obj2->rgn->rects[i].bottom) goto done;
	    }
            ret = TRUE;
        done:
	    GDI_ReleaseObj(hrgn2);
	}
	GDI_ReleaseObj(hrgn1);
    }
    return ret;
}

/***********************************************************************
 *           REGION_UnionRectWithRegion
 *           Adds a rectangle to a WINEREGION
 */
static void REGION_UnionRectWithRegion(const RECT *rect, WINEREGION *rgn)
{
    WINEREGION region;

    region.rects = &region.extents;
    region.numRects = 1;
    region.size = 1;
    region.extents = *rect;
    REGION_UnionRegion(rgn, rgn, &region);
}


/***********************************************************************
 *           REGION_CreateFrameRgn
 *
 * Create a region that is a frame around another region.
 * Compute the intersection of the region moved in all 4 directions
 * ( +x, -x, +y, -y) and subtract from the original.
 * The result looks slightly better than in Windows :)
 */
BOOL REGION_FrameRgn( HRGN hDest, HRGN hSrc, INT x, INT y )
{
    BOOL bRet;
    RGNOBJ *srcObj = (RGNOBJ*) GDI_GetObjPtr( hSrc, REGION_MAGIC );

    if (!srcObj) return FALSE;
    if (srcObj->rgn->numRects != 0)
    {
        RGNOBJ* destObj = (RGNOBJ*) GDI_GetObjPtr( hDest, REGION_MAGIC );
        WINEREGION *tmprgn = REGION_AllocWineRegion( srcObj->rgn->numRects);

        REGION_OffsetRegion( destObj->rgn, srcObj->rgn, -x, 0);
        REGION_OffsetRegion( tmprgn, srcObj->rgn, x, 0);
        REGION_IntersectRegion( destObj->rgn, destObj->rgn, tmprgn);
        REGION_OffsetRegion( tmprgn, srcObj->rgn, 0, -y);
        REGION_IntersectRegion( destObj->rgn, destObj->rgn, tmprgn);
        REGION_OffsetRegion( tmprgn, srcObj->rgn, 0, y);
        REGION_IntersectRegion( destObj->rgn, destObj->rgn, tmprgn);
        REGION_SubtractRegion( destObj->rgn, srcObj->rgn, destObj->rgn);

        REGION_DestroyWineRegion(tmprgn);
	GDI_ReleaseObj ( hDest );
	bRet = TRUE;
    }
    else
	bRet = FALSE;
    GDI_ReleaseObj( hSrc );
    return bRet;
}


/***********************************************************************
 *           CombineRgn   (GDI32.@)
 *
 * Combines two regions with the specifed operation and stores the result
 * in the specified destination region.
 *
 * PARAMS
 *   hDest [I] The region that receives the combined result.
 *   hSrc1 [I] The first source region.
 *   hSrc2 [I] The second source region.
 *   mode  [I] The way in which the source regions will be combined. See notes.
 *
 * RETURNS
 *   Success:
 *     NULLREGION - The new region is empty.
 *     SIMPLEREGION - The new region can be represented by one rectangle.
 *     COMPLEXREGION - The new region can only be represented by more than
 *                     one rectangle.
 *   Failure: ERROR
 *
 * NOTES
 *   The two source regions can be the same region.
 *   The mode can be one of the following:
 *|  RGN_AND - Intersection of the regions
 *|  RGN_OR - Union of the regions
 *|  RGN_XOR - Unions of the regions minus any intersection.
 *|  RGN_DIFF - Difference (subtraction) of the regions.
 */
INT WINAPI CombineRgn(HRGN hDest, HRGN hSrc1, HRGN hSrc2, INT mode)
{
    RGNOBJ *destObj = (RGNOBJ *) GDI_GetObjPtr( hDest, REGION_MAGIC);
    INT result = ERROR;

    TRACE(" %p,%p -> %p mode=%x\n", hSrc1, hSrc2, hDest, mode );
    if (destObj)
    {
	RGNOBJ *src1Obj = (RGNOBJ *) GDI_GetObjPtr( hSrc1, REGION_MAGIC);

	if (src1Obj)
	{
	    TRACE("dump src1Obj:\n");
	    if(TRACE_ON(region))
	      REGION_DumpRegion(src1Obj->rgn);
	    if (mode == RGN_COPY)
	    {
		REGION_CopyRegion( destObj->rgn, src1Obj->rgn );
		result = get_region_type( destObj );
	    }
	    else
	    {
		RGNOBJ *src2Obj = (RGNOBJ *) GDI_GetObjPtr( hSrc2, REGION_MAGIC);

		if (src2Obj)
		{
		    TRACE("dump src2Obj:\n");
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
		    result = get_region_type( destObj );
		    GDI_ReleaseObj( hSrc2 );
		}
	    }
	    GDI_ReleaseObj( hSrc1 );
	}
	TRACE("dump destObj:\n");
	if(TRACE_ON(region))
	  REGION_DumpRegion(destObj->rgn);

	GDI_ReleaseObj( hDest );
    } else {
       ERR("Invalid rgn=%p\n", hDest);
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
	    if (! (dst->rects = HeapReAlloc( GetProcessHeap(), 0, dst->rects,
				src->numRects * sizeof(RECT) )))
		return;
	    dst->size = src->numRects;
	}
	dst->numRects = src->numRects;
	dst->extents.left = src->extents.left;
	dst->extents.top = src->extents.top;
	dst->extents.right = src->extents.right;
	dst->extents.bottom = src->extents.bottom;
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
	    void (*overlapFunc)(WINEREGION*, RECT*, RECT*, RECT*, RECT*, INT, INT),     /* Function to call for over-lapping bands */
	    void (*nonOverlap1Func)(WINEREGION*, RECT*, RECT*, INT, INT), /* Function to call for non-overlapping bands in region 1 */
	    void (*nonOverlap2Func)(WINEREGION*, RECT*, RECT*, INT, INT)  /* Function to call for non-overlapping bands in region 2 */
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
    newReg->size = max(reg1->numRects,reg2->numRects) * 2;

    if (! (newReg->rects = HeapAlloc( GetProcessHeap(), 0,
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
	    top = max(r1->top,ybot);
	    bot = min(r1->bottom,r2->top);

	    if ((top != bot) && (nonOverlap1Func != (void (*)())NULL))
	    {
		(* nonOverlap1Func) (newReg, r1, r1BandEnd, top, bot);
	    }

	    ytop = r2->top;
	}
	else if (r2->top < r1->top)
	{
	    top = max(r2->top,ybot);
	    bot = min(r2->bottom,r1->top);

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
	ybot = min(r1->bottom, r2->bottom);
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
				     max(r1->top,ybot), r1->bottom);
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
				max(r2->top,ybot), r2->bottom);
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
	    newReg->rects = HeapReAlloc( GetProcessHeap(), 0, newReg->rects,
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
	    HeapFree( GetProcessHeap(), 0, newReg->rects );
	    newReg->rects = HeapAlloc( GetProcessHeap(), 0, sizeof(RECT) );
	}
    }
    HeapFree( GetProcessHeap(), 0, oldRects );
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
	left = max(r1->left, r2->left);
	right =	min(r1->right, r2->right);

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
	REGION_RegionOp (newReg, reg1, reg2, REGION_IntersectO, NULL, NULL);

    /*
     * Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the same. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    REGION_SetExtents(newReg);
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

    REGION_RegionOp (newReg, reg1, reg2, REGION_UnionO, REGION_UnionNonO, REGION_UnionNonO);

    newReg->extents.left = min(reg1->extents.left, reg2->extents.left);
    newReg->extents.top = min(reg1->extents.top, reg2->extents.top);
    newReg->extents.right = max(reg1->extents.right, reg2->extents.right);
    newReg->extents.bottom = max(reg1->extents.bottom, reg2->extents.bottom);
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
	     * Subtrahend precedes minuend: nuke left edge of minuend.
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

    REGION_RegionOp (regD, regM, regS, REGION_SubtractO, REGION_SubtractNonO1, NULL);

    /*
     * Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the unaltered. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    REGION_SetExtents (regD);
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
            tmpSLLBlock = HeapAlloc( GetProcessHeap(), 0, sizeof(ScanLineListBlock));
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
        HeapFree( GetProcessHeap(), 0, pSLLBlock );
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

    if (!(reg->rects = HeapReAlloc( GetProcessHeap(), 0, reg->rects,
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
 *           CreatePolyPolygonRgn    (GDI32.@)
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
        SetRectRgn( hrgn, min(Pts[0].x, Pts[2].x), min(Pts[0].y, Pts[2].y),
		            max(Pts[0].x, Pts[2].x), max(Pts[0].y, Pts[2].y) );
	GDI_ReleaseObj( hrgn );
	return hrgn;
    }

    for(poly = total = 0; poly < nbpolygons; poly++)
        total += Count[poly];
    if (! (pETEs = HeapAlloc( GetProcessHeap(), 0, sizeof(EdgeTableEntry) * total )))
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
                    tmpPtBlock = HeapAlloc( GetProcessHeap(), 0, sizeof(POINTBLOCK));
		    if(!tmpPtBlock) {
		        WARN("Can't alloc tPB\n");
                        HeapFree( GetProcessHeap(), 0, pETEs );
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
                        tmpPtBlock = HeapAlloc( GetProcessHeap(), 0,
					       sizeof(POINTBLOCK) );
			if(!tmpPtBlock) {
			    WARN("Can't alloc tPB\n");
			    REGION_DeleteObject( hrgn, obj );
                            HeapFree( GetProcessHeap(), 0, pETEs );
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
	HeapFree( GetProcessHeap(), 0, curPtBlock );
	curPtBlock = tmpPtBlock;
    }
    HeapFree( GetProcessHeap(), 0, pETEs );
    GDI_ReleaseObj( hrgn );
    return hrgn;
}


/***********************************************************************
 *           CreatePolygonRgn    (GDI32.@)
 */
HRGN WINAPI CreatePolygonRgn( const POINT *points, INT count,
                                  INT mode )
{
    return CreatePolyPolygonRgn( points, &count, 1, mode );
}
