/*
 * GDI region objects. Shamelessly ripped out from the X11 distribution
 * Thanks for the nice license.
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

#if 0
#pragma makedep unix
#endif

#include <assert.h>
#include "ntgdi_private.h"
#include "ntuser_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(region);


static BOOL REGION_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs region_funcs =
{
    NULL,                 /* pGetObjectW */
    NULL,                 /* pUnrealizeObject */
    REGION_DeleteObject   /* pDeleteObject */
};

/* Check if two RECTs overlap. */
static inline BOOL overlapping( const RECT *r1, const RECT *r2 )
{
    return (r1->right > r2->left && r1->left < r2->right &&
            r1->bottom > r2->top && r1->top < r2->bottom);
}

static BOOL grow_region( WINEREGION *rgn, int size )
{
    RECT *new_rects;

    if (size <= rgn->size) return TRUE;

    if (rgn->rects == rgn->rects_buf)
    {
        new_rects = malloc( size * sizeof(RECT) );
        if (!new_rects) return FALSE;
        memcpy( new_rects, rgn->rects, rgn->numRects * sizeof(RECT) );
    }
    else
    {
        new_rects = realloc( rgn->rects, size * sizeof(RECT) );
        if (!new_rects) return FALSE;
    }
    rgn->rects = new_rects;
    rgn->size = size;
    return TRUE;
}

static BOOL add_rect( WINEREGION *reg, INT left, INT top, INT right, INT bottom )
{
    RECT *rect;
    if (reg->numRects >= reg->size && !grow_region( reg, 2 * reg->size ))
        return FALSE;

    rect = reg->rects + reg->numRects++;
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
    return TRUE;
}

static inline void empty_region( WINEREGION *reg )
{
    reg->numRects = 0;
    reg->extents.left = reg->extents.top = reg->extents.right = reg->extents.bottom = 0;
}

static inline BOOL is_in_rect( const RECT *rect, int x, int y )
{
    return (rect->right > x && rect->left <= x && rect->bottom > y && rect->top <= y);
}


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
 *     This structure contains all of the information needed
 *     to run the bresenham algorithm.
 *     The variables may be hardcoded into the declarations
 *     instead of using this structure to make use of
 *     register declarations.
 */
struct bres_info
{
    INT minor_axis;	/* minor axis        */
    INT d;		/* decision variable */
    INT m, m1;       	/* slope and slope+1 */
    INT incr1, incr2;	/* error increments */
};


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
static inline void bres_init_polygon( int dy, int x1, int x2, struct bres_info *bres )
{
    int dx;

    /*
     *  if the edge is horizontal, then it is ignored
     *  and assumed not to be processed.  Otherwise, do this stuff.
     */
    if (!dy) return;

    bres->minor_axis = x1;
    dx = x2 - x1;
    if (dx < 0)
    {
        bres->m = dx / dy;
        bres->m1 = bres->m - 1;
        bres->incr1 = -2 * dx + 2 * dy * bres->m1;
        bres->incr2 = -2 * dx + 2 * dy * bres->m;
        bres->d = 2 * bres->m * dy - 2 * dx - 2 * dy;
    }
    else
    {
        bres->m = dx / (dy);
        bres->m1 = bres->m + 1;
        bres->incr1 = 2 * dx - 2 * dy * bres->m1;
        bres->incr2 = 2 * dx - 2 * dy * bres->m;
        bres->d = -2 * bres->m * dy + 2 * dx;
    }
}

static inline void bres_incr_polygon( struct bres_info *bres )
{
    if (bres->m1 > 0) {
        if (bres->d > 0) {
            bres->minor_axis += bres->m1;
            bres->d += bres->incr1;
        }
        else {
            bres->minor_axis += bres->m;
            bres->d += bres->incr2;
        }
    } else {
        if (bres->d >= 0) {
            bres->minor_axis += bres->m1;
            bres->d += bres->incr1;
        }
        else {
            bres->minor_axis += bres->m;
            bres->d += bres->incr2;
        }
    }
}


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

typedef struct edge_table_entry {
    struct list entry;
    struct list winding_entry;
    INT ymax;                     /* ycoord at which we exit this edge. */
    struct bres_info bres;        /* Bresenham info to run the edge     */
    int ClockWise;                /* flag for winding number rule       */
} EdgeTableEntry;


typedef struct _ScanLineList{
    struct list edgelist;
    INT scanline;            /* the scanline represented */
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


/* Note the parameter order is different from the X11 equivalents */

static BOOL REGION_CopyRegion(WINEREGION *d, WINEREGION *s);
static BOOL REGION_OffsetRegion(WINEREGION *d, WINEREGION *s, INT x, INT y);
static BOOL REGION_IntersectRegion(WINEREGION *d, WINEREGION *s1, WINEREGION *s2);
static BOOL REGION_UnionRegion(WINEREGION *d, WINEREGION *s1, WINEREGION *s2);
static BOOL REGION_SubtractRegion(WINEREGION *d, WINEREGION *s1, WINEREGION *s2);
static BOOL REGION_XorRegion(WINEREGION *d, WINEREGION *s1, WINEREGION *s2);
static BOOL REGION_UnionRectWithRegion(const RECT *rect, WINEREGION *rgn);

/***********************************************************************
 *            get_region_type
 */
static inline INT get_region_type( const WINEREGION *obj )
{
    switch(obj->numRects)
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

    TRACE("Region %p: %s %d rects\n", pReg, wine_dbgstr_rect(&pReg->extents), pReg->numRects);
    for(pRect = pReg->rects; pRect < pRectEnd; pRect++)
        TRACE("\t%s\n", wine_dbgstr_rect(pRect));
    return;
}


/***********************************************************************
 *            init_region
 *
 * Initialize a new empty region.
 */
static BOOL init_region( WINEREGION *pReg, INT n )
{
    n = max( n, RGN_DEFAULT_RECTS );

    if (n > RGN_DEFAULT_RECTS)
    {
        if (n > INT_MAX / sizeof(RECT)) return FALSE;
        if (!(pReg->rects = malloc( n * sizeof( RECT ) )))
            return FALSE;
    }
    else
        pReg->rects = pReg->rects_buf;

    pReg->size = n;
    empty_region(pReg);
    return TRUE;
}

/***********************************************************************
 *           destroy_region
 */
static void destroy_region( WINEREGION *pReg )
{
    if (pReg->rects != pReg->rects_buf)
        free( pReg->rects );
}

/***********************************************************************
 *           free_region
 */
static void free_region( WINEREGION *rgn )
{
    destroy_region( rgn );
    free( rgn );
}

/***********************************************************************
 *           alloc_region
 */
static WINEREGION *alloc_region( INT n )
{
    WINEREGION *rgn = malloc( sizeof(*rgn) );

    if (rgn && !init_region( rgn, n ))
    {
        free( rgn );
        rgn = NULL;
    }
    return rgn;
}

/************************************************************
 *              move_rects
 *
 * Move rectangles from src to dst leaving src with no rectangles.
 */
static inline void move_rects( WINEREGION *dst, WINEREGION *src )
{
    destroy_region( dst );
    if (src->rects == src->rects_buf)
    {
        dst->rects = dst->rects_buf;
        memcpy( dst->rects, src->rects, src->numRects * sizeof(RECT) );
    }
    else
        dst->rects = src->rects;
    dst->size = src->size;
    dst->numRects = src->numRects;
    init_region( src, 0 );
}

/***********************************************************************
 *           REGION_DeleteObject
 */
static BOOL REGION_DeleteObject( HGDIOBJ handle )
{
    WINEREGION *rgn = free_gdi_handle( handle );

    if (!rgn) return FALSE;
    free_region( rgn );
    return TRUE;
}


/***********************************************************************
 *           REGION_OffsetRegion
 *           Offset a WINEREGION by x,y
 */
static BOOL REGION_OffsetRegion( WINEREGION *rgn, WINEREGION *srcrgn, INT x, INT y )
{
    if( rgn != srcrgn)
    {
        if (!REGION_CopyRegion( rgn, srcrgn)) return FALSE;
    }
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
    return TRUE;
}

/***********************************************************************
 *           NtGdiOffsetRgn   (win32u.@)
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
INT WINAPI NtGdiOffsetRgn( HRGN hrgn, INT x, INT y )
{
    WINEREGION *obj = GDI_GetObjPtr( hrgn, NTGDI_OBJ_REGION );
    INT ret;

    TRACE("%p %d,%d\n", hrgn, x, y);

    if (!obj)
        return ERROR;

    REGION_OffsetRegion( obj, obj, x, y);

    ret = get_region_type( obj );
    GDI_ReleaseObj( hrgn );
    return ret;
}


/***********************************************************************
 *           NtGdiGetRgnBox    (win32u.@)
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
INT WINAPI NtGdiGetRgnBox( HRGN hrgn, RECT *rect )
{
    WINEREGION *obj = GDI_GetObjPtr( hrgn, NTGDI_OBJ_REGION );
    if (obj)
    {
	INT ret;
	rect->left = obj->extents.left;
	rect->top = obj->extents.top;
	rect->right = obj->extents.right;
	rect->bottom = obj->extents.bottom;
        TRACE("%p %s\n", hrgn, wine_dbgstr_rect(rect));
	ret = get_region_type( obj );
	GDI_ReleaseObj(hrgn);
	return ret;
    }
    return ERROR;
}


/***********************************************************************
 *           NtGdiCreateRectRgn   (win32u.@)
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
HRGN WINAPI NtGdiCreateRectRgn( INT left, INT top, INT right, INT bottom )
{
    HRGN hrgn;
    WINEREGION *obj;

    if (!(obj = alloc_region( RGN_DEFAULT_RECTS ))) return 0;

    if (!(hrgn = alloc_gdi_handle( &obj->obj, NTGDI_OBJ_REGION, &region_funcs )))
    {
        free_region( obj );
        return 0;
    }
    TRACE( "%d,%d-%d,%d returning %p\n", left, top, right, bottom, hrgn );
    NtGdiSetRectRgn( hrgn, left, top, right, bottom );
    return hrgn;
}


/***********************************************************************
 *           NtGdiSetRectRgn    (win32u.@)
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
BOOL WINAPI NtGdiSetRectRgn( HRGN hrgn, INT left, INT top, INT right, INT bottom )
{
    WINEREGION *obj;

    TRACE("%p %d,%d-%d,%d\n", hrgn, left, top, right, bottom );

    if (!(obj = GDI_GetObjPtr( hrgn, NTGDI_OBJ_REGION ))) return FALSE;

    if (left > right) { INT tmp = left; left = right; right = tmp; }
    if (top > bottom) { INT tmp = top; top = bottom; bottom = tmp; }

    if((left != right) && (top != bottom))
    {
        obj->rects->left = obj->extents.left = left;
        obj->rects->top = obj->extents.top = top;
        obj->rects->right = obj->extents.right = right;
        obj->rects->bottom = obj->extents.bottom = bottom;
        obj->numRects = 1;
    }
    else
	empty_region(obj);

    GDI_ReleaseObj( hrgn );
    return TRUE;
}


/***********************************************************************
 *           NtGdiCreateRoundRectRgn    (win32u.@)
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
HRGN WINAPI NtGdiCreateRoundRectRgn( INT left, INT top, INT right, INT bottom,
                                     INT ellipse_width, INT ellipse_height )
{
    WINEREGION *obj;
    HRGN hrgn = 0;
    int a, b, i, x, y;
    INT64 asq, bsq, dx, dy, err;
    RECT *rects;

      /* Make the dimensions sensible */

    if (left > right) { INT tmp = left; left = right; right = tmp; }
    if (top > bottom) { INT tmp = top; top = bottom; bottom = tmp; }
    /* the region is for the rectangle interior, but only at right and bottom for some reason */
    right--;
    bottom--;

    ellipse_width = min( right - left, abs( ellipse_width ));
    ellipse_height = min( bottom - top, abs( ellipse_height ));

      /* Check if we can do a normal rectangle instead */

    if ((ellipse_width < 2) || (ellipse_height < 2))
        return NtGdiCreateRectRgn( left, top, right, bottom );

    if (!(obj = alloc_region( ellipse_height ))) return 0;
    obj->numRects = ellipse_height;
    obj->extents.left   = left;
    obj->extents.top    = top;
    obj->extents.right  = right;
    obj->extents.bottom = bottom;
    rects = obj->rects;

    /* based on an algorithm by Alois Zingl */

    a = ellipse_width - 1;
    b = ellipse_height - 1;
    asq = (INT64)8 * a * a;
    bsq = (INT64)8 * b * b;
    dx  = (INT64)4 * b * b * (1 - a);
    dy  = (INT64)4 * a * a * (1 + (b % 2));
    err = dx + dy + a * a * (b % 2);

    x = 0;
    y = ellipse_height / 2;

    rects[y].left = left;
    rects[y].right = right;

    while (x <= ellipse_width / 2)
    {
        INT64 e2 = 2 * err;
        if (e2 >= dx)
        {
            x++;
            err += dx += bsq;
        }
        if (e2 <= dy)
        {
            y++;
            err += dy += asq;
            rects[y].left = left + x;
            rects[y].right = right - x;
        }
    }
    for (i = 0; i < ellipse_height / 2; i++)
    {
        rects[i].left = rects[b - i].left;
        rects[i].right = rects[b - i].right;
        rects[i].top = top + i;
        rects[i].bottom = rects[i].top + 1;
    }
    for (; i < ellipse_height; i++)
    {
        rects[i].top = bottom - ellipse_height + i;
        rects[i].bottom = rects[i].top + 1;
    }
    rects[ellipse_height / 2].top = top + ellipse_height / 2;  /* extend to top of rectangle */

    hrgn = alloc_gdi_handle( &obj->obj, NTGDI_OBJ_REGION, &region_funcs );

    TRACE("(%d,%d-%d,%d %dx%d): ret=%p\n",
	  left, top, right, bottom, ellipse_width, ellipse_height, hrgn );
    if (!hrgn) free_region( obj );
    return hrgn;
}


/***********************************************************************
 *           NtGdiCreateEllipticRgn    (win32u.@)
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
 *   ellipse at each corner is equal to the width the rectangle and
 *   the same for the height.
 */
HRGN WINAPI NtGdiCreateEllipticRgn( INT left, INT top, INT right, INT bottom )
{
    return NtGdiCreateRoundRectRgn( left, top, right, bottom,
                                    right - left, bottom - top );
}


/***********************************************************************
 *           NtGdiGetRegionData   (win32u.@)
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
DWORD WINAPI NtGdiGetRegionData( HRGN hrgn, DWORD count, RGNDATA *rgndata )
{
    DWORD size;
    WINEREGION *obj = GDI_GetObjPtr( hrgn, NTGDI_OBJ_REGION );

    TRACE(" %p count = %d, rgndata = %p\n", hrgn, count, rgndata);

    if(!obj) return 0;

    size = obj->numRects * sizeof(RECT);
    if (!rgndata || count < FIELD_OFFSET(RGNDATA, Buffer[size]))
    {
        GDI_ReleaseObj( hrgn );
	if (rgndata) /* buffer is too small, signal it by return 0 */
	    return 0;
        /* user requested buffer size with NULL rgndata */
        return FIELD_OFFSET(RGNDATA, Buffer[size]);
    }

    rgndata->rdh.dwSize = sizeof(RGNDATAHEADER);
    rgndata->rdh.iType = RDH_RECTANGLES;
    rgndata->rdh.nCount = obj->numRects;
    rgndata->rdh.nRgnSize = size;
    rgndata->rdh.rcBound.left = obj->extents.left;
    rgndata->rdh.rcBound.top = obj->extents.top;
    rgndata->rdh.rcBound.right = obj->extents.right;
    rgndata->rdh.rcBound.bottom = obj->extents.bottom;

    memcpy( rgndata->Buffer, obj->rects, size );

    GDI_ReleaseObj( hrgn );
    return FIELD_OFFSET(RGNDATA, Buffer[size]);
}


static void translate( POINT *pt, UINT count, const XFORM *xform )
{
    while (count--)
    {
        double x = pt->x;
        double y = pt->y;
        pt->x = GDI_ROUND( x * xform->eM11 + y * xform->eM21 + xform->eDx );
        pt->y = GDI_ROUND( x * xform->eM12 + y * xform->eM22 + xform->eDy );
        pt++;
    }
}


/***********************************************************************
 *           NtGdiExtCreateRegion   (win32u.@)
 *
 * Creates a region as specified by the transformation data and region data.
 *
 * PARAMS
 *   lpXform [I] World-space to logical-space transformation data.
 *   dwCount [I] Size of the data pointed to by rgndata, in bytes.
 *   rgndata [I] Data that specifies the region.
 *
 * RETURNS
 *   Success: Handle to region.
 *   Failure: NULL.
 *
 * NOTES
 *   See GetRegionData().
 */
HRGN WINAPI NtGdiExtCreateRegion( const XFORM *xform, DWORD count, const RGNDATA *rgndata )
{
    HRGN hrgn = 0;
    WINEREGION *obj;
    const RECT *pCurRect, *pEndRect;

    if (!rgndata || rgndata->rdh.dwSize < sizeof(RGNDATAHEADER))
        return 0;

    /* XP doesn't care about the type */
    if( rgndata->rdh.iType != RDH_RECTANGLES )
        WARN("(Unsupported region data type: %u)\n", rgndata->rdh.iType);

    if (xform)
    {
        const RECT *pCurRect, *pEndRect;

        hrgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );

        pEndRect = (const RECT *)rgndata->Buffer + rgndata->rdh.nCount;
        for (pCurRect = (const RECT *)rgndata->Buffer; pCurRect < pEndRect; pCurRect++)
        {
            static const INT count = 4;
            HRGN poly_hrgn;
            POINT pt[4];

            pt[0].x = pCurRect->left;
            pt[0].y = pCurRect->top;
            pt[1].x = pCurRect->right;
            pt[1].y = pCurRect->top;
            pt[2].x = pCurRect->right;
            pt[2].y = pCurRect->bottom;
            pt[3].x = pCurRect->left;
            pt[3].y = pCurRect->bottom;

            translate( pt, 4, xform );
            poly_hrgn = create_polypolygon_region( pt, &count, 1, WINDING, NULL );
            NtGdiCombineRgn( hrgn, hrgn, poly_hrgn, RGN_OR );
            NtGdiDeleteObjectApp( poly_hrgn );
        }
        return hrgn;
    }

    if (!(obj = alloc_region( rgndata->rdh.nCount ))) return 0;

    pEndRect = (const RECT *)rgndata->Buffer + rgndata->rdh.nCount;
    for(pCurRect = (const RECT *)rgndata->Buffer; pCurRect < pEndRect; pCurRect++)
    {
        if (pCurRect->left < pCurRect->right && pCurRect->top < pCurRect->bottom)
        {
            if (!REGION_UnionRectWithRegion( pCurRect, obj )) goto done;
        }
    }
    hrgn = alloc_gdi_handle( &obj->obj, NTGDI_OBJ_REGION, &region_funcs );

done:
    if (!hrgn) free_region( obj );

    TRACE("%p %d %p returning %p\n", xform, count, rgndata, hrgn );
    return hrgn;
}


/***********************************************************************
 *           NtGdiPtInRegion    (win32u.@)
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
BOOL WINAPI NtGdiPtInRegion( HRGN hrgn, INT x, INT y )
{
    WINEREGION *obj;
    BOOL ret = FALSE;

    if ((obj = GDI_GetObjPtr( hrgn, NTGDI_OBJ_REGION )))
    {
        if (obj->numRects > 0 && is_in_rect( &obj->extents, x, y ))
            region_find_pt( obj, x, y, &ret );
	GDI_ReleaseObj( hrgn );
    }
    return ret;
}


/***********************************************************************
 *           NtGdiRectInRegion    (win32u.@)
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
BOOL WINAPI NtGdiRectInRegion( HRGN hrgn, const RECT *rect )
{
    WINEREGION *obj;
    BOOL ret = FALSE;
    RECT rc;
    int i;

    /* swap the coordinates to make right >= left and bottom >= top */
    /* (region building rectangles are normalized the same way) */
    rc = *rect;
    order_rect( &rc );

    if ((obj = GDI_GetObjPtr( hrgn, NTGDI_OBJ_REGION )))
    {
	if ((obj->numRects > 0) && overlapping(&obj->extents, &rc))
	{
	    for (i = region_find_pt( obj, rc.left, rc.top, &ret ); !ret && i < obj->numRects; i++ )
	    {
	        if (obj->rects[i].bottom <= rc.top)
		    continue;             /* not far enough down yet */

		if (obj->rects[i].top >= rc.bottom)
		    break;                /* too far down */

		if (obj->rects[i].right <= rc.left)
		    continue;              /* not far enough over yet */

		if (obj->rects[i].left >= rc.right)
		    continue;

		ret = TRUE;
	    }
	}
	GDI_ReleaseObj(hrgn);
    }
    return ret;
}

/***********************************************************************
 *           NtGdiEqualRgn    (win32u.@)
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
BOOL WINAPI NtGdiEqualRgn( HRGN hrgn1, HRGN hrgn2 )
{
    WINEREGION *obj1, *obj2;
    BOOL ret = FALSE;

    if ((obj1 = GDI_GetObjPtr( hrgn1, NTGDI_OBJ_REGION )))
    {
        if ((obj2 = GDI_GetObjPtr( hrgn2, NTGDI_OBJ_REGION )))
	{
	    int i;

	    if ( obj1->numRects != obj2->numRects ) goto done;
            if ( obj1->numRects == 0 )
            {
                ret = TRUE;
                goto done;

            }
            if (obj1->extents.left   != obj2->extents.left) goto done;
            if (obj1->extents.right  != obj2->extents.right) goto done;
            if (obj1->extents.top    != obj2->extents.top) goto done;
            if (obj1->extents.bottom != obj2->extents.bottom) goto done;
            for( i = 0; i < obj1->numRects; i++ )
            {
                if (obj1->rects[i].left   != obj2->rects[i].left) goto done;
                if (obj1->rects[i].right  != obj2->rects[i].right) goto done;
                if (obj1->rects[i].top    != obj2->rects[i].top) goto done;
                if (obj1->rects[i].bottom != obj2->rects[i].bottom) goto done;
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
static BOOL REGION_UnionRectWithRegion(const RECT *rect, WINEREGION *rgn)
{
    WINEREGION region;

    init_region( &region, 1 );
    region.numRects = 1;
    region.extents = *region.rects = *rect;
    return REGION_UnionRegion(rgn, rgn, &region);
}


BOOL add_rect_to_region( HRGN rgn, const RECT *rect )
{
    WINEREGION *obj = GDI_GetObjPtr( rgn, NTGDI_OBJ_REGION );
    BOOL ret;

    if (!obj) return FALSE;
    ret = REGION_UnionRectWithRegion( rect, obj );
    GDI_ReleaseObj( rgn );
    return ret;
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
    WINEREGION tmprgn;
    BOOL bRet = FALSE;
    WINEREGION* destObj = NULL;
    WINEREGION *srcObj = GDI_GetObjPtr( hSrc, NTGDI_OBJ_REGION );

    tmprgn.rects = NULL;
    if (!srcObj) return FALSE;
    if (srcObj->numRects != 0)
    {
        if (!(destObj = GDI_GetObjPtr( hDest, NTGDI_OBJ_REGION ))) goto done;
        if (!init_region( &tmprgn, srcObj->numRects )) goto done;

        if (!REGION_OffsetRegion( destObj, srcObj, -x, 0)) goto done;
        if (!REGION_OffsetRegion( &tmprgn, srcObj, x, 0)) goto done;
        if (!REGION_IntersectRegion( destObj, destObj, &tmprgn )) goto done;
        if (!REGION_OffsetRegion( &tmprgn, srcObj, 0, -y)) goto done;
        if (!REGION_IntersectRegion( destObj, destObj, &tmprgn )) goto done;
        if (!REGION_OffsetRegion( &tmprgn, srcObj, 0, y)) goto done;
        if (!REGION_IntersectRegion( destObj, destObj, &tmprgn )) goto done;
        if (!REGION_SubtractRegion( destObj, srcObj, destObj )) goto done;
	bRet = TRUE;
    }
done:
    destroy_region( &tmprgn );
    if (destObj) GDI_ReleaseObj ( hDest );
    GDI_ReleaseObj( hSrc );
    return bRet;
}


/***********************************************************************
 *           NtGdiCombineRgn   (win32u.@)
 *
 * Combines two regions with the specified operation and stores the result
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
INT WINAPI NtGdiCombineRgn( HRGN hDest, HRGN hSrc1, HRGN hSrc2, INT mode )
{
    WINEREGION *destObj = GDI_GetObjPtr( hDest, NTGDI_OBJ_REGION );
    INT result = ERROR;

    TRACE(" %p,%p -> %p mode=%x\n", hSrc1, hSrc2, hDest, mode );
    if (destObj)
    {
        WINEREGION *src1Obj = GDI_GetObjPtr( hSrc1, NTGDI_OBJ_REGION );

	if (src1Obj)
	{
	    TRACE("dump src1Obj:\n");
	    if(TRACE_ON(region))
	      REGION_DumpRegion(src1Obj);
	    if (mode == RGN_COPY)
	    {
		if (REGION_CopyRegion( destObj, src1Obj ))
                    result = get_region_type( destObj );
	    }
	    else
	    {
                WINEREGION *src2Obj = GDI_GetObjPtr( hSrc2, NTGDI_OBJ_REGION );

		if (src2Obj)
		{
		    TRACE("dump src2Obj:\n");
		    if(TRACE_ON(region))
		        REGION_DumpRegion(src2Obj);
		    switch (mode)
		    {
		    case RGN_AND:
			if (REGION_IntersectRegion( destObj, src1Obj, src2Obj ))
                            result = get_region_type( destObj );
			break;
		    case RGN_OR:
			if (REGION_UnionRegion( destObj, src1Obj, src2Obj ))
                            result = get_region_type( destObj );
			break;
		    case RGN_XOR:
			if (REGION_XorRegion( destObj, src1Obj, src2Obj ))
                            result = get_region_type( destObj );
			break;
		    case RGN_DIFF:
			if (REGION_SubtractRegion( destObj, src1Obj, src2Obj ))
                            result = get_region_type( destObj );
			break;
		    }
		    GDI_ReleaseObj( hSrc2 );
		}
	    }
	    GDI_ReleaseObj( hSrc1 );
	}
	TRACE("dump destObj:\n");
	if(TRACE_ON(region))
	  REGION_DumpRegion(destObj);

	GDI_ReleaseObj( hDest );
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
static BOOL REGION_CopyRegion(WINEREGION *dst, WINEREGION *src)
{
    if (dst != src) /*  don't want to copy to itself */
    {
	if (dst->size < src->numRects && !grow_region( dst, src->numRects ))
            return FALSE;

	dst->numRects = src->numRects;
	dst->extents.left = src->extents.left;
	dst->extents.top = src->extents.top;
	dst->extents.right = src->extents.right;
	dst->extents.bottom = src->extents.bottom;
        memcpy(dst->rects, src->rects, src->numRects * sizeof(RECT));
    }
    return TRUE;
}

/***********************************************************************
 *           REGION_MirrorRegion
 */
static BOOL REGION_MirrorRegion( WINEREGION *dst, WINEREGION *src, int width )
{
    int i, start, end;
    RECT extents;
    RECT *rects;
    WINEREGION tmp;

    if (dst != src)
    {
        if (!grow_region( dst, src->numRects )) return FALSE;
        rects = dst->rects;
        dst->numRects = src->numRects;
    }
    else
    {
        if (!init_region( &tmp, src->numRects )) return FALSE;
        rects = tmp.rects;
        tmp.numRects = src->numRects;
    }

    extents.left   = width - src->extents.right;
    extents.right  = width - src->extents.left;
    extents.top    = src->extents.top;
    extents.bottom = src->extents.bottom;

    for (start = 0; start < src->numRects; start = end)
    {
        /* find the end of the current band */
        for (end = start + 1; end < src->numRects; end++)
            if (src->rects[end].top != src->rects[end - 1].top) break;

        for (i = 0; i < end - start; i++)
        {
            rects[start + i].left   = width - src->rects[end - i - 1].right;
            rects[start + i].right  = width - src->rects[end - i - 1].left;
            rects[start + i].top    = src->rects[end - i - 1].top;
            rects[start + i].bottom = src->rects[end - i - 1].bottom;
        }
    }

    if (dst == src)
        move_rects( dst, &tmp );

    dst->extents = extents;
    return TRUE;
}

/***********************************************************************
 *           mirror_region
 */
INT mirror_region( HRGN dst, HRGN src, INT width )
{
    WINEREGION *src_rgn, *dst_rgn;
    INT ret = ERROR;

    if (!(src_rgn = GDI_GetObjPtr( src, NTGDI_OBJ_REGION ))) return ERROR;
    if ((dst_rgn = GDI_GetObjPtr( dst, NTGDI_OBJ_REGION )))
    {
        if (REGION_MirrorRegion( dst_rgn, src_rgn, width )) ret = get_region_type( dst_rgn );
        GDI_ReleaseObj( dst_rgn );
    }
    GDI_ReleaseObj( src_rgn );
    return ret;
}

/***********************************************************************
 *           mirror_window_region
 */
BOOL mirror_window_region( HWND hwnd, HRGN hrgn )
{
    RECT rect;

    if (!get_window_rect( hwnd, &rect, get_thread_dpi() )) return FALSE;
    return mirror_region( hrgn, hrgn, rect.right - rect.left ) != ERROR;
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

/**********************************************************************
 *          REGION_compact
 *
 * To keep regions from growing without bound, shrink the array of rectangles
 * to match the new number of rectangles in the region.
 *
 * Only do this if the number of rectangles allocated is more than
 * twice the number of rectangles in the region.
 */
static void REGION_compact( WINEREGION *reg )
{
    if ((reg->numRects < reg->size / 2) && (reg->numRects > RGN_DEFAULT_RECTS))
    {
        RECT *new_rects = realloc( reg->rects, reg->numRects * sizeof(RECT) );
        if (new_rects)
        {
            reg->rects = new_rects;
            reg->size = reg->numRects;
        }
    }
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
static BOOL REGION_RegionOp(
	    WINEREGION *destReg, /* Place to store result */
	    WINEREGION *reg1,   /* First region in operation */
            WINEREGION *reg2,   /* 2nd region in operation */
	    BOOL (*overlapFunc)(WINEREGION*, RECT*, RECT*, RECT*, RECT*, INT, INT),     /* Function to call for over-lapping bands */
	    BOOL (*nonOverlap1Func)(WINEREGION*, RECT*, RECT*, INT, INT), /* Function to call for non-overlapping bands in region 1 */
	    BOOL (*nonOverlap2Func)(WINEREGION*, RECT*, RECT*, INT, INT)  /* Function to call for non-overlapping bands in region 2 */
) {
    WINEREGION newReg;
    RECT *r1;                         /* Pointer into first region */
    RECT *r2;                         /* Pointer into 2d region */
    RECT *r1End;                      /* End of 1st region */
    RECT *r2End;                      /* End of 2d region */
    INT ybot;                         /* Bottom of intersection */
    INT ytop;                         /* Top of intersection */
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
     * Allocate a reasonable number of rectangles for the new region. The idea
     * is to allocate enough so the individual functions don't need to
     * reallocate and copy the array, which is time consuming, yet we don't
     * have to worry about using too much memory. I hope to be able to
     * nuke the Xrealloc() at the end of this function eventually.
     */
    if (!init_region( &newReg, max(reg1->numRects,reg2->numRects) * 2 )) return FALSE;

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
	curBand = newReg.numRects;

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

            if ((top != bot) && (nonOverlap1Func != NULL))
	    {
		if (!nonOverlap1Func(&newReg, r1, r1BandEnd, top, bot)) return FALSE;
	    }

	    ytop = r2->top;
	}
	else if (r2->top < r1->top)
	{
	    top = max(r2->top,ybot);
	    bot = min(r2->bottom,r1->top);

            if ((top != bot) && (nonOverlap2Func != NULL))
	    {
		if (!nonOverlap2Func(&newReg, r2, r2BandEnd, top, bot)) return FALSE;
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
	if (newReg.numRects != curBand)
	{
	    prevBand = REGION_Coalesce (&newReg, prevBand, curBand);
	}

	/*
	 * Now see if we've hit an intersecting band. The two bands only
	 * intersect if ybot > ytop
	 */
	ybot = min(r1->bottom, r2->bottom);
	curBand = newReg.numRects;
	if (ybot > ytop)
	{
	    if (!overlapFunc(&newReg, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot)) return FALSE;
	}

	if (newReg.numRects != curBand)
	{
	    prevBand = REGION_Coalesce (&newReg, prevBand, curBand);
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
    curBand = newReg.numRects;
    if (r1 != r1End)
    {
        if (nonOverlap1Func != NULL)
	{
	    do
	    {
		r1BandEnd = r1;
		while ((r1BandEnd < r1End) && (r1BandEnd->top == r1->top))
		{
		    r1BandEnd++;
		}
		if (!nonOverlap1Func(&newReg, r1, r1BandEnd, max(r1->top,ybot), r1->bottom))
                    return FALSE;
		r1 = r1BandEnd;
	    } while (r1 != r1End);
	}
    }
    else if ((r2 != r2End) && (nonOverlap2Func != NULL))
    {
	do
	{
	    r2BandEnd = r2;
	    while ((r2BandEnd < r2End) && (r2BandEnd->top == r2->top))
	    {
		 r2BandEnd++;
	    }
	    if (!nonOverlap2Func(&newReg, r2, r2BandEnd, max(r2->top,ybot), r2->bottom))
                return FALSE;
	    r2 = r2BandEnd;
	} while (r2 != r2End);
    }

    if (newReg.numRects != curBand)
    {
	REGION_Coalesce (&newReg, prevBand, curBand);
    }

    REGION_compact( &newReg );
    move_rects( destReg, &newReg );
    return TRUE;
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
static BOOL REGION_IntersectO(WINEREGION *pReg,  RECT *r1, RECT *r1End,
                              RECT *r2, RECT *r2End, INT top, INT bottom)

{
    INT       left, right;

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
            if (!add_rect( pReg, left, top, right, bottom )) return FALSE;
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
    return TRUE;
}

/***********************************************************************
 *	     REGION_IntersectRegion
 */
static BOOL REGION_IntersectRegion(WINEREGION *newReg, WINEREGION *reg1,
				   WINEREGION *reg2)
{
   /* check for trivial reject */
    if ( (!(reg1->numRects)) || (!(reg2->numRects))  ||
	(!overlapping(&reg1->extents, &reg2->extents)))
	newReg->numRects = 0;
    else
	if (!REGION_RegionOp (newReg, reg1, reg2, REGION_IntersectO, NULL, NULL)) return FALSE;

    /*
     * Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the same. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    REGION_SetExtents(newReg);
    return TRUE;
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
static BOOL REGION_UnionNonO(WINEREGION *pReg, RECT *r, RECT *rEnd, INT top, INT bottom)
{
    while (r != rEnd)
    {
        if (!add_rect( pReg, r->left, top, r->right, bottom )) return FALSE;
	r++;
    }
    return TRUE;
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
static BOOL REGION_UnionO (WINEREGION *pReg, RECT *r1, RECT *r1End,
			   RECT *r2, RECT *r2End, INT top, INT bottom)
{
#define MERGERECT(r) \
    if ((pReg->numRects != 0) &&  \
	(pReg->rects[pReg->numRects-1].top == top) &&  \
	(pReg->rects[pReg->numRects-1].bottom == bottom) &&  \
	(pReg->rects[pReg->numRects-1].right >= r->left))  \
    {  \
	if (pReg->rects[pReg->numRects-1].right < r->right)  \
	    pReg->rects[pReg->numRects-1].right = r->right;  \
    }  \
    else  \
    { \
        if (!add_rect( pReg, r->left, top, r->right, bottom )) return FALSE; \
    } \
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
    return TRUE;
#undef MERGERECT
}

/***********************************************************************
 *	     REGION_UnionRegion
 */
static BOOL REGION_UnionRegion(WINEREGION *newReg, WINEREGION *reg1, WINEREGION *reg2)
{
    BOOL ret = TRUE;

    /*  checks all the simple cases */

    /*
     * Region 1 and 2 are the same or region 1 is empty
     */
    if ( (reg1 == reg2) || (!(reg1->numRects)) )
    {
	if (newReg != reg2)
	    ret = REGION_CopyRegion(newReg, reg2);
	return ret;
    }

    /*
     * if nothing to union (region 2 empty)
     */
    if (!(reg2->numRects))
    {
	if (newReg != reg1)
	    ret = REGION_CopyRegion(newReg, reg1);
	return ret;
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
	    ret = REGION_CopyRegion(newReg, reg1);
	return ret;
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
	    ret = REGION_CopyRegion(newReg, reg2);
	return ret;
    }

    if ((ret = REGION_RegionOp (newReg, reg1, reg2, REGION_UnionO, REGION_UnionNonO, REGION_UnionNonO)))
    {
        newReg->extents.left = min(reg1->extents.left, reg2->extents.left);
        newReg->extents.top = min(reg1->extents.top, reg2->extents.top);
        newReg->extents.right = max(reg1->extents.right, reg2->extents.right);
        newReg->extents.bottom = max(reg1->extents.bottom, reg2->extents.bottom);
    }
    return ret;
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
static BOOL REGION_SubtractNonO1 (WINEREGION *pReg, RECT *r, RECT *rEnd, INT top, INT bottom)
{
    while (r != rEnd)
    {
        if (!add_rect( pReg, r->left, top, r->right, bottom )) return FALSE;
	r++;
    }
    return TRUE;
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
static BOOL REGION_SubtractO (WINEREGION *pReg, RECT *r1, RECT *r1End,
                              RECT *r2, RECT *r2End, INT top, INT bottom)
{
    INT left = r1->left;

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
            if (!add_rect( pReg, left, top, r2->left, bottom )) return FALSE;
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
                if (!add_rect( pReg, left, top, r1->right, bottom )) return FALSE;
	    }
	    r1++;
	    if (r1 != r1End)
	        left = r1->left;
	}
    }

    /*
     * Add remaining minuend rectangles to region.
     */
    while (r1 != r1End)
    {
        if (!add_rect( pReg, left, top, r1->right, bottom )) return FALSE;
	r1++;
	if (r1 != r1End)
	{
	    left = r1->left;
	}
    }
    return TRUE;
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
static BOOL REGION_SubtractRegion(WINEREGION *regD, WINEREGION *regM, WINEREGION *regS )
{
   /* check for trivial reject */
    if ( (!(regM->numRects)) || (!(regS->numRects))  ||
	(!overlapping(&regM->extents, &regS->extents)) )
	return REGION_CopyRegion(regD, regM);

    if (!REGION_RegionOp (regD, regM, regS, REGION_SubtractO, REGION_SubtractNonO1, NULL))
        return FALSE;

    /*
     * Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the unaltered. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    REGION_SetExtents (regD);
    return TRUE;
}

/***********************************************************************
 *	     REGION_XorRegion
 */
static BOOL REGION_XorRegion(WINEREGION *dr, WINEREGION *sra, WINEREGION *srb)
{
    WINEREGION tra, trb;
    BOOL ret;

    if (!init_region( &tra, sra->numRects + 1 )) return FALSE;
    if ((ret = init_region( &trb, srb->numRects + 1 )))
    {
        ret = REGION_SubtractRegion(&tra,sra,srb) &&
              REGION_SubtractRegion(&trb,srb,sra) &&
              REGION_UnionRegion(dr,&tra,&trb);
        destroy_region(&trb);
    }
    destroy_region(&tra);
    return ret;
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
    struct list *ptr;
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
            tmpSLLBlock = malloc( sizeof(ScanLineListBlock) );
	    if(!tmpSLLBlock)
	    {
	        WARN("Can't alloc SLLB\n");
		return;
	    }
            (*SLLBlock)->next = tmpSLLBlock;
            tmpSLLBlock->next = NULL;
            *SLLBlock = tmpSLLBlock;
            *iSLLBlock = 0;
        }
        pSLL = &((*SLLBlock)->SLLs[(*iSLLBlock)++]);

        pSLL->next = pPrevSLL->next;
        list_init( &pSLL->edgelist );
        pPrevSLL->next = pSLL;
    }
    pSLL->scanline = scanline;

    /*
     * now insert the edge in the right bucket
     */
    LIST_FOR_EACH( ptr, &pSLL->edgelist )
    {
        struct edge_table_entry *entry = LIST_ENTRY( ptr, struct edge_table_entry, entry );
        if (entry->bres.minor_axis >= ETE->bres.minor_axis) break;
    }
    list_add_before( ptr, &ETE->entry );
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
static unsigned int REGION_CreateEdgeTable(const INT *Count, INT nbpolygons,
                                           const POINT *pts, EdgeTable *ET,
                                           EdgeTableEntry *pETEs, ScanLineListBlock *pSLLBlock,
                                           const RECT *clip_rect)
{
    const POINT *top, *bottom;
    const POINT *PrevPt, *CurrPt, *EndPt;
    INT poly, count;
    int iSLLBlock = 0;
    unsigned int dy, total = 0;

    /*
     *  initialize the Edge Table.
     */
    ET->scanlines.next = NULL;
    ET->ymax = SMALL_COORDINATE;
    ET->ymin = LARGE_COORDINATE;
    pSLLBlock->next = NULL;

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
	for ( ; count; PrevPt = CurrPt, count--)
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
	    if (bottom->y == top->y) continue;
            if (clip_rect && (top->y >= clip_rect->bottom || bottom->y <= clip_rect->top)) continue;
            pETEs->ymax = bottom->y-1; /* -1 so we don't get last scanline */

            /*
             *  initialize integer edge algorithm
             */
            dy = bottom->y - top->y;
            bres_init_polygon(dy, top->x, bottom->x, &pETEs->bres);

            if (clip_rect) dy = min( bottom->y, clip_rect->bottom ) - max( top->y, clip_rect->top );
            if (total + dy < total) return 0;  /* overflow */
            total += dy;

            REGION_InsertEdgeInET(ET, pETEs, top->y, &pSLLBlock, &iSLLBlock);

            if (top->y    < ET->ymin) ET->ymin = top->y;
            if (bottom->y > ET->ymax) ET->ymax = bottom->y;
            pETEs++;
	}
    }
    return total;
}

/***********************************************************************
 *     REGION_loadAET
 *
 *     This routine moves EdgeTableEntries from the
 *     EdgeTable into the Active Edge Table,
 *     leaving them sorted by smaller x coordinate.
 *
 */
static void REGION_loadAET( struct list *AET, struct list *ETEs )
{
    struct edge_table_entry *ptr, *next, *entry;
    struct list *active;

    LIST_FOR_EACH_ENTRY_SAFE( ptr, next, ETEs, struct edge_table_entry, entry )
    {
        LIST_FOR_EACH( active, AET )
        {
            entry = LIST_ENTRY( active, struct edge_table_entry, entry );
            if (entry->bres.minor_axis >= ptr->bres.minor_axis) break;
        }
        list_remove( &ptr->entry );
        list_add_before( active, &ptr->entry );
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
static void REGION_computeWAET( struct list *AET, struct list *WETE )
{
    struct edge_table_entry *active;
    BOOL inside = TRUE;
    int isInside = 0;

    list_init( WETE );
    LIST_FOR_EACH_ENTRY( active, AET, struct edge_table_entry, entry )
    {
        if (active->ClockWise)
            isInside++;
        else
            isInside--;

        if ((!inside && !isInside) || (inside && isInside))
        {
            list_add_tail( WETE, &active->winding_entry );
            inside = !inside;
        }
    }
}

/***********************************************************************
 *     next_scanline
 *
 * Update the Active Edge Table for the next scan line and sort it again.
 */
static inline BOOL next_scanline( struct list *AET, int y )
{
    struct edge_table_entry *active, *next, *insert;
    BOOL changed = FALSE;

    LIST_FOR_EACH_ENTRY_SAFE( active, next, AET, struct edge_table_entry, entry )
    {
        if (active->ymax == y)  /* leaving this edge */
        {
            list_remove( &active->entry );
            changed = TRUE;
        }
        else bres_incr_polygon( &active->bres );
    }
    LIST_FOR_EACH_ENTRY_SAFE( active, next, AET, struct edge_table_entry, entry )
    {
        LIST_FOR_EACH_ENTRY( insert, AET, struct edge_table_entry, entry )
        {
            if (insert == active) break;
            if (insert->bres.minor_axis > active->bres.minor_axis) break;
        }
        if (insert == active) continue;
        list_remove( &active->entry );
        list_add_before( &insert->entry, &active->entry );
        changed = TRUE;
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
        free( pSLLBlock );
        pSLLBlock = tmpSLLBlock;
    }
}

static void scan_convert( WINEREGION *obj, EdgeTable *ET, INT mode, const RECT *clip_rect )
{
    struct list AET;
    ScanLineList *pSLL;
    struct edge_table_entry *active;
    INT i, y, first = 1, cur_band = 0, prev_band = 0;

    if (clip_rect) ET->ymax = min( ET->ymax, clip_rect->bottom );

    list_init( &AET );
    pSLL = ET->scanlines.next;

    if (mode != WINDING)
    {
        for (y = ET->ymin; y < ET->ymax; y++)
        {
            /* Add a new edge to the active edge table when we get to the next edge. */
            if (pSLL != NULL && y == pSLL->scanline)
            {
                REGION_loadAET(&AET, &pSLL->edgelist);
                pSLL = pSLL->next;
            }

            if (!clip_rect || y >= clip_rect->top)
            {
                LIST_FOR_EACH_ENTRY( active, &AET, struct edge_table_entry, entry )
                {
                    if (first)
                    {
                        obj->rects[obj->numRects].left = active->bres.minor_axis;
                        obj->rects[obj->numRects].top = y;
                        obj->rects[obj->numRects].bottom = y + 1;
                    }
                    else if (obj->rects[obj->numRects].left != active->bres.minor_axis)
                    {
                        /* create new rect only if we can't merge with the previous one */
                        if (!obj->numRects || obj->rects[obj->numRects-1].top != y ||
                            obj->rects[obj->numRects-1].right < obj->rects[obj->numRects].left)
                            obj->numRects++;
                        obj->rects[obj->numRects-1].right = active->bres.minor_axis;
                    }
                    first = !first;
                }
            }

            next_scanline( &AET, y );

            if (obj->numRects)
            {
                prev_band = REGION_Coalesce( obj, prev_band, cur_band );
                cur_band = obj->numRects;
            }
        }
    }
    else /* mode == WINDING */
    {
        struct list WETE, *pWETE;

        for (y = ET->ymin; y < ET->ymax; y++)
        {
            /* Add a new edge to the active edge table when we get to the next edge. */
            if (pSLL != NULL && y == pSLL->scanline)
            {
                REGION_loadAET(&AET, &pSLL->edgelist);
                REGION_computeWAET( &AET, &WETE );
                pSLL = pSLL->next;
            }
            pWETE = list_head( &WETE );

            if (!clip_rect || y >= clip_rect->top)
            {
                LIST_FOR_EACH_ENTRY( active, &AET, struct edge_table_entry, entry )
                {
                    /* Add to the buffer only those edges that are in the Winding active edge table. */
                    if (pWETE == &active->winding_entry)
                    {
                        if (first)
                        {
                            obj->rects[obj->numRects].left = active->bres.minor_axis;
                            obj->rects[obj->numRects].top = y;
                        }
                        else if (obj->rects[obj->numRects].left != active->bres.minor_axis)
                        {
                            obj->rects[obj->numRects].right = active->bres.minor_axis;
                            obj->rects[obj->numRects].bottom = y + 1;
                            obj->numRects++;
                        }
                        first = !first;
                        pWETE = list_next( &WETE, pWETE );
                    }
                }
            }

            /* Recompute the winding active edge table if we just resorted or have exited an edge. */
            if (next_scanline( &AET, y )) REGION_computeWAET( &AET, &WETE );

            if (obj->numRects)
            {
                prev_band = REGION_Coalesce( obj, prev_band, cur_band );
                cur_band = obj->numRects;
            }
        }
    }

    assert( obj->numRects <= obj->size );

    if (obj->numRects)
    {
        obj->extents.left = INT_MAX;
        obj->extents.right = INT_MIN;
        obj->extents.top = obj->rects[0].top;
        obj->extents.bottom = obj->rects[obj->numRects-1].bottom;
        for (i = 0; i < obj->numRects; i++)
        {
            obj->extents.left = min( obj->extents.left, obj->rects[i].left );
            obj->extents.right = max( obj->extents.right, obj->rects[i].right );
        }
    }
    REGION_compact( obj );
}

/***********************************************************************
 *           create_polypolygon_region
 *
 * Helper for CreatePolyPolygonRgn.
 */
HRGN create_polypolygon_region( const POINT *Pts, const INT *Count, INT nbpolygons, INT mode,
                                const RECT *clip_rect )
{
    HRGN hrgn = 0;
    WINEREGION *obj = NULL;
    EdgeTable ET;                    /* header node for ET      */
    EdgeTableEntry *pETEs;           /* EdgeTableEntries pool   */
    ScanLineListBlock SLLBlock;      /* header for scanlinelist */
    unsigned int nb_points;
    INT poly, total;

    TRACE("%p, count %d, polygons %d, mode %d\n", Pts, *Count, nbpolygons, mode);

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
        return NtGdiCreateRectRgn( min(Pts[0].x, Pts[2].x), min(Pts[0].y, Pts[2].y),
                                   max(Pts[0].x, Pts[2].x), max(Pts[0].y, Pts[2].y) );

    for(poly = total = 0; poly < nbpolygons; poly++)
        total += Count[poly];
    if (! (pETEs = malloc( sizeof(EdgeTableEntry) * total )))
	return 0;

    nb_points = REGION_CreateEdgeTable( Count, nbpolygons, Pts, &ET, pETEs, &SLLBlock, clip_rect );
    if ((obj = alloc_region( nb_points / 2 )))
    {
        if (nb_points) scan_convert( obj, &ET, mode, clip_rect );

        if (!(hrgn = alloc_gdi_handle( &obj->obj, NTGDI_OBJ_REGION, &region_funcs )))
            free_region( obj );
    }

    REGION_FreeStorage(SLLBlock.next);
    free( pETEs );
    return hrgn;
}
