/*
 *	PostScript driver graphics functions
 *
 *	Copyright 1998  Huw D M Davies
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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "psdrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

/***********************************************************************
 *           PSDRV_XWStoDS
 *
 * Performs a world-to-viewport transformation on the specified width.
 */
INT PSDRV_XWStoDS( PHYSDEV dev, INT width )
{
    POINT pt[2];

    pt[0].x = 0;
    pt[0].y = 0;
    pt[1].x = width;
    pt[1].y = 0;
    LPtoDP( dev->hdc, pt, 2 );
    return pt[1].x - pt[0].x;
}

/***********************************************************************
 *           PSDRV_DrawLine
 */
static void PSDRV_DrawLine( PHYSDEV dev )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );

    if(physDev->pathdepth)
        return;

    if (physDev->pen.style == PS_NULL)
	PSDRV_WriteNewPath(dev);
    else
	PSDRV_WriteStroke(dev);
}

/***********************************************************************
 *           PSDRV_LineTo
 */
BOOL CDECL PSDRV_LineTo(PHYSDEV dev, INT x, INT y)
{
    POINT pt[2];

    TRACE("%d %d\n", x, y);

    GetCurrentPositionEx( dev->hdc, pt );
    pt[1].x = x;
    pt[1].y = y;
    LPtoDP( dev->hdc, pt, 2 );

    PSDRV_SetPen(dev);

    PSDRV_SetClip(dev);
    PSDRV_WriteMoveTo(dev, pt[0].x, pt[0].y );
    PSDRV_WriteLineTo(dev, pt[1].x, pt[1].y );
    PSDRV_DrawLine(dev);
    PSDRV_ResetClip(dev);

    return TRUE;
}


/***********************************************************************
 *           PSDRV_Rectangle
 */
BOOL CDECL PSDRV_Rectangle( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );
    RECT rect;

    TRACE("%d %d - %d %d\n", left, top, right, bottom);

    SetRect(&rect, left, top, right, bottom);
    LPtoDP( dev->hdc, (POINT *)&rect, 2 );

    /* Windows does something truly hacky here.  If we're in passthrough mode
       and our rop is R2_NOP, then we output the string below.  This is used in
       Office 2k when inserting eps files */
    if (physDev->job.passthrough_state == passthrough_active && GetROP2(dev->hdc) == R2_NOP)
    {
        char buf[256];

        sprintf(buf, "N %ld %ld %ld %ld B\n", rect.right - rect.left, rect.bottom - rect.top, rect.left, rect.top);
        write_spool(dev, buf, strlen(buf));
        physDev->job.passthrough_state = passthrough_had_rect;
        return TRUE;
    }

    PSDRV_SetPen(dev);

    PSDRV_SetClip(dev);
    PSDRV_WriteRectangle(dev, rect.left, rect.top, rect.right - rect.left,
                         rect.bottom - rect.top );
    PSDRV_Brush(dev,0);
    PSDRV_DrawLine(dev);
    PSDRV_ResetClip(dev);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_RoundRect
 */
BOOL CDECL PSDRV_RoundRect( PHYSDEV dev, INT left, INT top, INT right,
                            INT bottom, INT ell_width, INT ell_height )
{
    RECT rect[2];

    SetRect(&rect[0], left, top, right, bottom);
    SetRect(&rect[1], 0, 0, ell_width, ell_height);
    LPtoDP( dev->hdc, (POINT *)rect, 4 );

    left   = rect[0].left;
    top    = rect[0].top;
    right  = rect[0].right;
    bottom = rect[0].bottom;
    if (left > right) { INT tmp = left; left = right; right = tmp; }
    if (top > bottom) { INT tmp = top; top = bottom; bottom = tmp; }

    ell_width  = rect[1].right - rect[1].left;
    ell_height = rect[1].bottom - rect[1].top;
    if (ell_width > right - left) ell_width = right - left;
    if (ell_height > bottom - top) ell_height = bottom - top;

    PSDRV_WriteSpool(dev, "%RoundRect\n",11);
    PSDRV_SetPen(dev);

    PSDRV_SetClip(dev);
    PSDRV_WriteMoveTo( dev, left, top + ell_height/2 );
    PSDRV_WriteArc( dev, left + ell_width/2, top + ell_height/2, ell_width,
		    ell_height, 90.0, 180.0);
    PSDRV_WriteLineTo( dev, right - ell_width/2, top );
    PSDRV_WriteArc( dev, right - ell_width/2, top + ell_height/2, ell_width,
		    ell_height, 0.0, 90.0);
    PSDRV_WriteLineTo( dev, right, bottom - ell_height/2 );
    PSDRV_WriteArc( dev, right - ell_width/2, bottom - ell_height/2, ell_width,
		    ell_height, -90.0, 0.0);
    PSDRV_WriteLineTo( dev, right - ell_width/2, bottom);
    PSDRV_WriteArc( dev, left + ell_width/2, bottom - ell_height/2, ell_width,
		    ell_height, 180.0, -90.0);
    PSDRV_WriteClosePath( dev );

    PSDRV_Brush(dev,0);
    PSDRV_DrawLine(dev);
    PSDRV_ResetClip(dev);
    return TRUE;
}

/***********************************************************************
 *           PSDRV_DrawArc
 *
 * Does the work of Arc, Chord and Pie. lines is 0, 1 or 2 respectively.
 */
static BOOL PSDRV_DrawArc( PHYSDEV dev, INT left, INT top,
                           INT right, INT bottom, INT xstart, INT ystart,
                           INT xend, INT yend, int lines )
{
    INT x, y, h, w;
    double start_angle, end_angle, ratio;
    RECT rect;
    POINT start, end;

    SetRect(&rect, left, top, right, bottom);
    LPtoDP( dev->hdc, (POINT *)&rect, 2 );
    start.x = xstart;
    start.y = ystart;
    end.x = xend;
    end.y = yend;
    LPtoDP( dev->hdc, &start, 1 );
    LPtoDP( dev->hdc, &end, 1 );

    x = (rect.left + rect.right) / 2;
    y = (rect.top + rect.bottom) / 2;
    w = rect.right - rect.left;
    h = rect.bottom - rect.top;

    if(w < 0) w = -w;
    if(h < 0) h = -h;
    ratio = ((double)w)/h;

    /* angle is the angle after the rectangle is transformed to a square and is
       measured anticlockwise from the +ve x-axis */

    start_angle = atan2((double)(y - start.y) * ratio, (double)(start.x - x));
    end_angle = atan2((double)(y - end.y) * ratio, (double)(end.x - x));

    start_angle *= 180.0 / M_PI;
    end_angle *= 180.0 / M_PI;

    PSDRV_WriteSpool(dev,"%DrawArc\n", 9);
    PSDRV_SetPen(dev);

    PSDRV_SetClip(dev);
    if(lines == 2) /* pie */
        PSDRV_WriteMoveTo(dev, x, y);
    else
        PSDRV_WriteNewPath( dev );

    if(GetArcDirection(dev->hdc) == AD_COUNTERCLOCKWISE)
        PSDRV_WriteArc(dev, x, y, w, h, start_angle, end_angle);
    else
        PSDRV_WriteArc(dev, x, y, w, h, end_angle, start_angle);
    if(lines == 1 || lines == 2) { /* chord or pie */
        PSDRV_WriteClosePath(dev);
	PSDRV_Brush(dev,0);
    }
    PSDRV_DrawLine(dev);
    PSDRV_ResetClip(dev);

    return TRUE;
}


/***********************************************************************
 *           PSDRV_Arc
 */
BOOL CDECL PSDRV_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                      INT xstart, INT ystart, INT xend, INT yend )
{
    return PSDRV_DrawArc( dev, left, top, right, bottom, xstart, ystart, xend, yend, 0 );
}

/***********************************************************************
 *           PSDRV_Chord
 */
BOOL CDECL PSDRV_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                        INT xstart, INT ystart, INT xend, INT yend )
{
    return PSDRV_DrawArc( dev, left, top, right, bottom, xstart, ystart, xend, yend, 1 );
}


/***********************************************************************
 *           PSDRV_Pie
 */
BOOL CDECL PSDRV_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                      INT xstart, INT ystart, INT xend, INT yend )
{
    return PSDRV_DrawArc( dev, left, top, right, bottom, xstart, ystart, xend, yend, 2 );
}


/***********************************************************************
 *           PSDRV_Ellipse
 */
BOOL CDECL PSDRV_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom)
{
    INT x, y, w, h;
    RECT rect;

    TRACE("%d %d - %d %d\n", left, top, right, bottom);

    SetRect(&rect, left, top, right, bottom);
    LPtoDP( dev->hdc, (POINT *)&rect, 2 );

    x = (rect.left + rect.right) / 2;
    y = (rect.top + rect.bottom) / 2;
    w = rect.right - rect.left;
    h = rect.bottom - rect.top;

    PSDRV_WriteSpool(dev, "%Ellipse\n", 9);
    PSDRV_SetPen(dev);

    PSDRV_SetClip(dev);
    PSDRV_WriteNewPath(dev);
    PSDRV_WriteArc(dev, x, y, w, h, 0.0, 360.0);
    PSDRV_WriteClosePath(dev);
    PSDRV_Brush(dev,0);
    PSDRV_DrawLine(dev);
    PSDRV_ResetClip(dev);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_PolyPolyline
 */
BOOL CDECL PSDRV_PolyPolyline( PHYSDEV dev, const POINT* pts, const DWORD* counts, DWORD polylines )
{
    DWORD polyline, line, total;
    POINT *dev_pts, *pt;

    TRACE("\n");

    for (polyline = total = 0; polyline < polylines; polyline++) total += counts[polyline];
    if (!(dev_pts = HeapAlloc( GetProcessHeap(), 0, total * sizeof(*dev_pts) ))) return FALSE;
    memcpy( dev_pts, pts, total * sizeof(*dev_pts) );
    LPtoDP( dev->hdc, dev_pts, total );

    pt = dev_pts;

    PSDRV_WriteSpool(dev, "%PolyPolyline\n",14);
    PSDRV_SetPen(dev);
    PSDRV_SetClip(dev);

    for(polyline = 0; polyline < polylines; polyline++) {
        PSDRV_WriteMoveTo(dev, pt->x, pt->y);
	pt++;
        for(line = 1; line < counts[polyline]; line++, pt++)
            PSDRV_WriteLineTo(dev, pt->x, pt->y);
    }
    HeapFree( GetProcessHeap(), 0, dev_pts );

    PSDRV_DrawLine(dev);
    PSDRV_ResetClip(dev);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_PolyPolygon
 */
BOOL CDECL PSDRV_PolyPolygon( PHYSDEV dev, const POINT* pts, const INT* counts, UINT polygons )
{
    DWORD polygon, total;
    INT line;
    POINT *dev_pts, *pt;

    TRACE("\n");

    for (polygon = total = 0; polygon < polygons; polygon++) total += counts[polygon];
    if (!(dev_pts = HeapAlloc( GetProcessHeap(), 0, total * sizeof(*dev_pts) ))) return FALSE;
    memcpy( dev_pts, pts, total * sizeof(*dev_pts) );
    LPtoDP( dev->hdc, dev_pts, total );

    pt = dev_pts;

    PSDRV_WriteSpool(dev, "%PolyPolygon\n",13);
    PSDRV_SetPen(dev);
    PSDRV_SetClip(dev);

    for(polygon = 0; polygon < polygons; polygon++) {
        PSDRV_WriteMoveTo(dev, pt->x, pt->y);
	pt++;
        for(line = 1; line < counts[polygon]; line++, pt++)
            PSDRV_WriteLineTo(dev, pt->x, pt->y);
	PSDRV_WriteClosePath(dev);
    }
    HeapFree( GetProcessHeap(), 0, dev_pts );

    if(GetPolyFillMode( dev->hdc ) == ALTERNATE)
        PSDRV_Brush(dev, 1);
    else /* WINDING */
        PSDRV_Brush(dev, 0);

    PSDRV_DrawLine(dev);
    PSDRV_ResetClip(dev);
    return TRUE;
}

static BOOL poly_bezier( PHYSDEV dev, const POINT *pt0, const POINT *pts, DWORD count)
{
    POINT dev_pts[3];
    DWORD i;

    TRACE( "\n" );

    dev_pts[0] = *pt0;
    LPtoDP( dev->hdc, dev_pts, 1 );

    PSDRV_WriteSpool( dev, "%PolyBezier\n", 12 );
    PSDRV_SetPen( dev );
    PSDRV_SetClip( dev );
    PSDRV_WriteMoveTo( dev, dev_pts[0].x, dev_pts[0].y );
    for (i = 0; i < count; i += 3)
    {
        memcpy( dev_pts, pts, sizeof(dev_pts) );
        LPtoDP( dev->hdc, dev_pts, 3 );
        PSDRV_WriteCurveTo( dev, dev_pts );
    }
    PSDRV_DrawLine(dev);
    PSDRV_ResetClip(dev);
    return TRUE;
}

/***********************************************************************
 *           PSDRV_PolyBezier
 */
BOOL CDECL PSDRV_PolyBezier( PHYSDEV dev, const POINT *pts, DWORD count )
{
    return poly_bezier( dev, pts, pts + 1, count - 1 );
}


/***********************************************************************
 *           PSDRV_PolyBezierTo
 */
BOOL CDECL PSDRV_PolyBezierTo( PHYSDEV dev, const POINT *pts, DWORD count )
{
    POINT pt0;

    GetCurrentPositionEx( dev->hdc, &pt0 );
    return poly_bezier( dev, &pt0, pts, count );
}


/***********************************************************************
 *           PSDRV_SetPixel
 */
COLORREF CDECL PSDRV_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color )
{
    PSCOLOR pscolor;
    POINT pt;

    pt.x = x;
    pt.y = y;
    LPtoDP( dev->hdc, &pt, 1 );

    PSDRV_SetClip(dev);
    /* we bracket the setcolor in gsave/grestore so that we don't trash
       the current pen colour */
    PSDRV_WriteGSave(dev);
    PSDRV_WriteRectangle( dev, pt.x, pt.y, 1, 1 );
    PSDRV_CreateColor( dev, &pscolor, color );
    PSDRV_WriteSetColor( dev, &pscolor );
    PSDRV_WriteFill( dev );
    PSDRV_WriteGRestore(dev);
    PSDRV_ResetClip(dev);
    return color;
}

/***********************************************************************
 *           PSDRV_PaintRgn
 */
BOOL CDECL PSDRV_PaintRgn( PHYSDEV dev, HRGN hrgn )
{
    RGNDATA *rgndata = NULL;
    RECT *pRect;
    DWORD size, i;

    TRACE("hdc=%p\n", dev->hdc);

    size = GetRegionData(hrgn, 0, NULL);
    rgndata = HeapAlloc( GetProcessHeap(), 0, size );
    if(!rgndata) {
        ERR("Can't allocate buffer\n");
        return FALSE;
    }
    
    GetRegionData(hrgn, size, rgndata);
    if (rgndata->rdh.nCount == 0)
        goto end;

    LPtoDP(dev->hdc, (POINT*)rgndata->Buffer, rgndata->rdh.nCount * 2);

    PSDRV_SetClip(dev);
    for(i = 0, pRect = (RECT*)rgndata->Buffer; i < rgndata->rdh.nCount; i++, pRect++)
        PSDRV_WriteRectangle(dev, pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top);

    PSDRV_Brush(dev, 0);
    PSDRV_WriteNewPath(dev);
    PSDRV_ResetClip(dev);

 end:
    HeapFree(GetProcessHeap(), 0, rgndata);
    return TRUE;
}

static BOOL paint_path( PHYSDEV dev, BOOL stroke, BOOL fill )
{
    POINT *points;
    BYTE *types;
    int i, size = GetPath( dev->hdc, NULL, NULL, 0 );

    if (size == -1) return FALSE;
    if (!size)
    {
        AbortPath( dev->hdc );
        return TRUE;
    }
    points = HeapAlloc( GetProcessHeap(), 0, size * sizeof(*points) );
    types = HeapAlloc( GetProcessHeap(), 0, size * sizeof(*types) );
    if (!points || !types) goto done;
    if (GetPath( dev->hdc, points, types, size ) == -1) goto done;
    LPtoDP( dev->hdc, points, size );

    if (stroke) PSDRV_SetPen(dev);
    PSDRV_SetClip(dev);
    for (i = 0; i < size; i++)
    {
        switch (types[i])
        {
        case PT_MOVETO:
            PSDRV_WriteMoveTo( dev, points[i].x, points[i].y );
            break;
        case PT_LINETO:
        case PT_LINETO | PT_CLOSEFIGURE:
            PSDRV_WriteLineTo( dev, points[i].x, points[i].y );
            if (types[i] & PT_CLOSEFIGURE) PSDRV_WriteClosePath( dev );
            break;
        case PT_BEZIERTO:
        case PT_BEZIERTO | PT_CLOSEFIGURE:
            PSDRV_WriteCurveTo( dev, points + i );
            if (types[i] & PT_CLOSEFIGURE) PSDRV_WriteClosePath( dev );
            i += 2;
            break;
        }
    }
    if (fill) PSDRV_Brush( dev, GetPolyFillMode(dev->hdc) == ALTERNATE );
    if (stroke) PSDRV_DrawLine(dev);
    else PSDRV_WriteNewPath(dev);
    PSDRV_ResetClip(dev);
    AbortPath( dev->hdc );

done:
    HeapFree( GetProcessHeap(), 0, points );
    HeapFree( GetProcessHeap(), 0, types );
    return TRUE;
}

/***********************************************************************
 *           PSDRV_FillPath
 */
BOOL CDECL PSDRV_FillPath( PHYSDEV dev )
{
    return paint_path( dev, FALSE, TRUE );
}

/***********************************************************************
 *           PSDRV_StrokeAndFillPath
 */
BOOL CDECL PSDRV_StrokeAndFillPath( PHYSDEV dev )
{
    return paint_path( dev, TRUE, TRUE );
}

/***********************************************************************
 *           PSDRV_StrokePath
 */
BOOL CDECL PSDRV_StrokePath( PHYSDEV dev )
{
    return paint_path( dev, TRUE, FALSE );
}
