/*
 * Enhanced MetaFile driver graphics functions
 *
 * Copyright 1999 Huw D M Davies
 */

#include <stdlib.h>
#include "gdi.h"
#include "dc.h"
#include "enhmetafiledrv.h"
#include "heap.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(enhmetafile)

/**********************************************************************
 *	     EMFDRV_MoveToEx
 */
BOOL
EMFDRV_MoveToEx(DC *dc,INT x,INT y,LPPOINT pt)
{
    EMRMOVETOEX emr;

    emr.emr.iType = EMR_MOVETOEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptl.x = x;
    emr.ptl.y = y;

    if(!EMFDRV_WriteRecord( dc, &emr.emr ))
    	return FALSE;

    if (pt) {
	pt->x = dc->w.CursPosX;
	pt->y = dc->w.CursPosY;
    }
    dc->w.CursPosX = x;
    dc->w.CursPosY = y;
    return TRUE;
}

/***********************************************************************
 *           EMFDRV_LineTo
 */
BOOL
EMFDRV_LineTo( DC *dc, INT x, INT y )
{
    EMRLINETO emr;
    RECTL bounds;

    emr.emr.iType = EMR_LINETO;
    emr.emr.nSize = sizeof(emr);
    emr.ptl.x = x;
    emr.ptl.y = y;

    if(!EMFDRV_WriteRecord( dc, &emr.emr ))
    	return FALSE;

    bounds.left   = MIN(x, dc->w.CursPosX);
    bounds.top    = MIN(y, dc->w.CursPosY);
    bounds.right  = MAX(x, dc->w.CursPosX);
    bounds.bottom = MAX(y, dc->w.CursPosY);

    EMFDRV_UpdateBBox( dc, &bounds );

    dc->w.CursPosX = x;
    dc->w.CursPosY = y;
    return TRUE;
}


/***********************************************************************
 *           EMFDRV_ArcChordPie
 */
static BOOL
EMFDRV_ArcChordPie( DC *dc, INT left, INT top, INT right, INT bottom,
		    INT xstart, INT ystart, INT xend, INT yend, DWORD iType )
{
    INT temp, xCentre, yCentre, i;
    double angleStart, angleEnd;
    double xinterStart, yinterStart, xinterEnd, yinterEnd;
    EMRARC emr;
    RECTL bounds;

    if(left == right || top == bottom) return FALSE;

    if(left > right) {temp = left; left = right; right = temp;}
    if(top > bottom) {temp = top; top = bottom; bottom = temp;}

    right--;
    bottom--;

    emr.emr.iType     = iType;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = left;
    emr.rclBox.top    = top;
    emr.rclBox.right  = right;
    emr.rclBox.bottom = bottom;
    emr.ptlStart.x    = xstart;
    emr.ptlStart.y    = ystart;
    emr.ptlEnd.x      = xend;
    emr.ptlEnd.x      = yend;


    /* Now calculate the BBox */
    xCentre = (left + right + 1) / 2;
    yCentre = (top + bottom + 1) / 2;

    xstart -= xCentre;
    ystart -= yCentre;
    xend   -= xCentre;
    yend   -= yCentre;

    /* invert y co-ords to get angle anti-clockwise from x-axis */
    angleStart = atan2( -(double)ystart, (double)xstart);
    angleEnd   = atan2( -(double)yend, (double)xend);

    /* These are the intercepts of the start/end lines with the arc */

    xinterStart = (right - left + 1)/2 * cos(angleStart) + xCentre;
    yinterStart = -(bottom - top + 1)/2 * sin(angleStart) + yCentre;
    xinterEnd   = (right - left + 1)/2 * cos(angleEnd) + xCentre;
    yinterEnd   = -(bottom - top + 1)/2 * sin(angleEnd) + yCentre;

    if(angleStart < 0) angleStart += 2 * M_PI;
    if(angleEnd < 0) angleEnd += 2 * M_PI;
    if(angleEnd < angleStart) angleEnd += 2 * M_PI;

    bounds.left   = MIN(xinterStart, xinterEnd);
    bounds.top    = MIN(yinterStart, yinterEnd);
    bounds.right  = MAX(xinterStart, xinterEnd);
    bounds.bottom = MAX(yinterStart, yinterEnd);
    
    for(i = 0; i <= 8; i++) {
        if(i * M_PI / 2 < angleStart) /* loop until we're past start */
	    continue;
	if(i * M_PI / 2 > angleEnd)   /* if we're past end we're finished */
	    break;

	/* the arc touches the rectangle at the start of quadrant i, so adjust
	   BBox to reflect this. */

	switch(i % 4) { 
	case 0:
	    bounds.right = right;
	    break;
	case 1:
	    bounds.top = top;
	    break;
	case 2:
	    bounds.left = left;
	    break;
	case 3:
	    bounds.bottom = bottom;
	    break;
	}
    }

    /* If we're drawing a pie then make sure we include the centre */
    if(iType == EMR_PIE) {
        if(bounds.left > xCentre) bounds.left = xCentre;
	else if(bounds.right < xCentre) bounds.right = xCentre;
	if(bounds.top > yCentre) bounds.top = yCentre;
	else if(bounds.bottom < yCentre) bounds.right = yCentre;
    }
    if(!EMFDRV_WriteRecord( dc, &emr.emr ))
        return FALSE;
    EMFDRV_UpdateBBox( dc, &bounds );
    return TRUE;
}


/***********************************************************************
 *           EMFDRV_Arc
 */
BOOL
EMFDRV_Arc( DC *dc, INT left, INT top, INT right, INT bottom,
	    INT xstart, INT ystart, INT xend, INT yend )
{
    return EMFDRV_ArcChordPie( dc, left, top, right, bottom, xstart, ystart,
			       xend, yend, EMR_ARC );
}

/***********************************************************************
 *           EMFDRV_Pie
 */
BOOL
EMFDRV_Pie( DC *dc, INT left, INT top, INT right, INT bottom,
	    INT xstart, INT ystart, INT xend, INT yend )
{
    return EMFDRV_ArcChordPie( dc, left, top, right, bottom, xstart, ystart,
			       xend, yend, EMR_PIE );
}


/***********************************************************************
 *           EMFDRV_Chord
 */
BOOL
EMFDRV_Chord( DC *dc, INT left, INT top, INT right, INT bottom,
             INT xstart, INT ystart, INT xend, INT yend )
{
    return EMFDRV_ArcChordPie( dc, left, top, right, bottom, xstart, ystart,
			       xend, yend, EMR_CHORD );
}

/***********************************************************************
 *           EMFDRV_Ellipse
 */
BOOL
EMFDRV_Ellipse( DC *dc, INT left, INT top, INT right, INT bottom )
{
    EMRELLIPSE emr;
    INT temp;

    TRACE(enhmetafile, "%d,%d - %d,%d\n", left, top, right, bottom);

    if(left == right || top == bottom) return FALSE;

    if(left > right) {temp = left; left = right; right = temp;}
    if(top > bottom) {temp = top; top = bottom; bottom = temp;}

    right--;
    bottom--;

    emr.emr.iType     = EMR_ELLIPSE;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = left;
    emr.rclBox.top    = top;
    emr.rclBox.right  = right;
    emr.rclBox.bottom = bottom;

    EMFDRV_UpdateBBox( dc, &emr.rclBox );
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

/***********************************************************************
 *           EMFDRV_Rectangle
 */
BOOL
EMFDRV_Rectangle(DC *dc, INT left, INT top, INT right, INT bottom)
{
    EMRRECTANGLE emr;
    INT temp;

    TRACE(enhmetafile, "%d,%d - %d,%d\n", left, top, right, bottom);

    if(left == right || top == bottom) return FALSE;

    if(left > right) {temp = left; left = right; right = temp;}
    if(top > bottom) {temp = top; top = bottom; bottom = temp;}

    right--;
    bottom--;

    emr.emr.iType     = EMR_RECTANGLE;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = left;
    emr.rclBox.top    = top;
    emr.rclBox.right  = right;
    emr.rclBox.bottom = bottom;

    EMFDRV_UpdateBBox( dc, &emr.rclBox );
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

/***********************************************************************
 *           EMFDRV_RoundRect
 */
BOOL 
EMFDRV_RoundRect( DC *dc, INT left, INT top, INT right,
		  INT bottom, INT ell_width, INT ell_height )
{
    EMRROUNDRECT emr;
    INT temp;

    if(left == right || top == bottom) return FALSE;

    if(left > right) {temp = left; left = right; right = temp;}
    if(top > bottom) {temp = top; top = bottom; bottom = temp;}

    right--;
    bottom--;

    emr.emr.iType     = EMR_ROUNDRECT;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = left;
    emr.rclBox.top    = top;
    emr.rclBox.right  = right;
    emr.rclBox.bottom = bottom;
    emr.szlCorner.cx  = ell_width;
    emr.szlCorner.cy  = ell_height;

    EMFDRV_UpdateBBox( dc, &emr.rclBox );
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

/***********************************************************************
 *           EMFDRV_SetPixel
 */
COLORREF
EMFDRV_SetPixel( DC *dc, INT x, INT y, COLORREF color )
{
  return TRUE;
}


/**********************************************************************
 *          EMFDRV_Polylinegon
 *
 * Helper for EMFDRV_Poly{line|gon}
 */
static BOOL
EMFDRV_Polylinegon( DC *dc, const POINT* pt, INT count, DWORD iType )
{
    EMRPOLYLINE *emr;
    DWORD size;
    INT i;
    BOOL ret;

    size = sizeof(EMRPOLYLINE) + sizeof(POINTL) * (count - 1);

    emr = HeapAlloc( SystemHeap, 0, size );
    emr->emr.iType = iType;
    emr->emr.nSize = size;
    
    emr->rclBounds.left = emr->rclBounds.right = pt[0].x;
    emr->rclBounds.top = emr->rclBounds.bottom = pt[0].y;

    for(i = 1; i < count; i++) {
        if(pt[i].x < emr->rclBounds.left)
	    emr->rclBounds.left = pt[i].x;
	else if(pt[i].x > emr->rclBounds.right)
	    emr->rclBounds.right = pt[i].x;
	if(pt[i].y < emr->rclBounds.top)
	    emr->rclBounds.top = pt[i].y;
	else if(pt[i].y > emr->rclBounds.bottom)
	    emr->rclBounds.bottom = pt[i].y;
    }

    emr->cptl = count;
    memcpy(emr->aptl, pt, size);

    ret = EMFDRV_WriteRecord( dc, &emr->emr );
    if(ret)
        EMFDRV_UpdateBBox( dc, &emr->rclBounds );
    HeapFree( SystemHeap, 0, emr );
    return ret;
}


/**********************************************************************
 *          EMFDRV_Polyline
 */
BOOL
EMFDRV_Polyline( DC *dc, const POINT* pt, INT count )
{
    return EMFDRV_Polylinegon( dc, pt, count, EMR_POLYLINE );
}

/**********************************************************************
 *          EMFDRV_Polygon
 */
BOOL
EMFDRV_Polygon( DC *dc, const POINT* pt, INT count )
{
    if(count < 2) return FALSE;
    return EMFDRV_Polylinegon( dc, pt, count, EMR_POLYGON );
}


/**********************************************************************
 *          EMFDRV_PolyPolylinegon
 *
 * Helper for EMFDRV_PolyPoly{line|gon}
 */
static BOOL 
EMFDRV_PolyPolylinegon( DC *dc, const POINT* pt, const INT* counts, UINT polys,
			DWORD iType)
{
    EMRPOLYPOLYLINE *emr;
    DWORD cptl = 0, poly, size, point;
    RECTL bounds;
    const POINT *pts;
    BOOL ret;

    bounds.left = bounds.right = pt[0].x;
    bounds.top = bounds.bottom = pt[0].y;

    pts = pt;
    for(poly = 0; poly < polys; poly++) {
        cptl += counts[poly];
	for(point = 0; point < counts[poly]; point++) {
	    if(bounds.left > pts->x) bounds.left = pts->x;
	    else if(bounds.right < pts->x) bounds.right = pts->x;
	    if(bounds.top > pts->y) bounds.top = pts->y;
	    else if(bounds.bottom < pts->y) bounds.bottom = pts->y;
	    pts++;
	}
    }
    
    size = sizeof(EMRPOLYPOLYLINE) + (polys - 1) * sizeof(DWORD) + 
      (cptl - 1) * sizeof(POINTL);

    emr = HeapAlloc( SystemHeap, 0, size );

    emr->emr.iType = iType;
    emr->emr.nSize = size;
    emr->rclBounds = bounds;
    emr->nPolys = polys;
    emr->cptl = cptl;
    memcpy(emr->aPolyCounts, counts, polys * sizeof(DWORD));
    memcpy(emr->aPolyCounts + polys, pt, cptl * sizeof(POINT));
    ret = EMFDRV_WriteRecord( dc, &emr->emr );
    if(ret)
        EMFDRV_UpdateBBox( dc, &emr->rclBounds );
    HeapFree( SystemHeap, 0, emr );
    return ret;
}

/**********************************************************************
 *          EMFDRV_PolyPolyline
 */
BOOL 
EMFDRV_PolyPolyline(DC *dc, const POINT* pt, const DWORD* counts, DWORD polys)
{
    return EMFDRV_PolyPolylinegon( dc, pt, (INT *)counts, polys,
				   EMR_POLYPOLYLINE );
}

/**********************************************************************
 *          EMFDRV_PolyPolygon
 */
BOOL 
EMFDRV_PolyPolygon( DC *dc, const POINT* pt, const INT* counts, UINT polys )
{
    return EMFDRV_PolyPolylinegon( dc, pt, counts, polys, EMR_POLYPOLYGON );
}


/**********************************************************************
 *          EMFDRV_ExtFloodFill
 */
BOOL 
EMFDRV_ExtFloodFill( DC *dc, INT x, INT y, COLORREF color, UINT fillType )
{
    EMREXTFLOODFILL emr;

    emr.emr.iType = EMR_EXTFLOODFILL;
    emr.emr.nSize = sizeof(emr);
    emr.ptlStart.x = x;
    emr.ptlStart.y = y;
    emr.crColor = color;
    emr.iMode = fillType;

    return EMFDRV_WriteRecord( dc, &emr.emr );
}


/*********************************************************************
 *          EMFDRV_FillRgn
 */
BOOL EMFDRV_FillRgn( DC *dc, HRGN hrgn, HBRUSH hbrush )
{
    EMRFILLRGN *emr;
    DWORD size, rgnsize, index;
    BOOL ret;

    index = EMFDRV_CreateBrushIndirect( dc, hbrush );
    if(!index) return FALSE;

    rgnsize = GetRegionData( hrgn, 0, NULL );
    size = rgnsize + sizeof(EMRFILLRGN) - 1;
    emr = HeapAlloc( SystemHeap, 0, size );

    GetRegionData( hrgn, rgnsize, (RGNDATA *)&emr->RgnData );

    emr->emr.iType = EMR_FILLRGN;
    emr->emr.nSize = size;
    emr->rclBounds.left   = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.left;
    emr->rclBounds.top    = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.top;
    emr->rclBounds.right  = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.right - 1;
    emr->rclBounds.bottom = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.bottom - 1;
    emr->cbRgnData = rgnsize;
    emr->ihBrush = index;
    
    ret = EMFDRV_WriteRecord( dc, &emr->emr );
    if(ret)
        EMFDRV_UpdateBBox( dc, &emr->rclBounds );
    HeapFree( SystemHeap, 0, emr );
    return ret;
}
/*********************************************************************
 *          EMFDRV_FrameRgn
 */
BOOL EMFDRV_FrameRgn( DC *dc, HRGN hrgn, HBRUSH hbrush, INT width, INT height )
{
    EMRFRAMERGN *emr;
    DWORD size, rgnsize, index;
    BOOL ret;

    index = EMFDRV_CreateBrushIndirect( dc, hbrush );
    if(!index) return FALSE;

    rgnsize = GetRegionData( hrgn, 0, NULL );
    size = rgnsize + sizeof(EMRFRAMERGN) - 1;
    emr = HeapAlloc( SystemHeap, 0, size );

    GetRegionData( hrgn, rgnsize, (RGNDATA *)&emr->RgnData );

    emr->emr.iType = EMR_FRAMERGN;
    emr->emr.nSize = size;
    emr->rclBounds.left   = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.left;
    emr->rclBounds.top    = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.top;
    emr->rclBounds.right  = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.right - 1;
    emr->rclBounds.bottom = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.bottom - 1;
    emr->cbRgnData = rgnsize;
    emr->ihBrush = index;
    emr->szlStroke.cx = width;
    emr->szlStroke.cy = height;

    ret = EMFDRV_WriteRecord( dc, &emr->emr );
    if(ret)
        EMFDRV_UpdateBBox( dc, &emr->rclBounds );
    HeapFree( SystemHeap, 0, emr );
    return ret;
}

/*********************************************************************
 *          EMFDRV_PaintInvertRgn
 *
 * Helper for EMFDRV_{Paint|Invert}Rgn
 */
static BOOL EMFDRV_PaintInvertRgn( DC *dc, HRGN hrgn, DWORD iType )
{
    EMRINVERTRGN *emr;
    DWORD size, rgnsize;
    BOOL ret;


    rgnsize = GetRegionData( hrgn, 0, NULL );
    size = rgnsize + sizeof(EMRINVERTRGN) - 1;
    emr = HeapAlloc( SystemHeap, 0, size );

    GetRegionData( hrgn, rgnsize, (RGNDATA *)&emr->RgnData );

    emr->emr.iType = iType;
    emr->emr.nSize = size;
    emr->rclBounds.left   = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.left;
    emr->rclBounds.top    = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.top;
    emr->rclBounds.right  = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.right - 1;
    emr->rclBounds.bottom = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.bottom - 1;
    emr->cbRgnData = rgnsize;
    
    ret = EMFDRV_WriteRecord( dc, &emr->emr );
    if(ret)
        EMFDRV_UpdateBBox( dc, &emr->rclBounds );
    HeapFree( SystemHeap, 0, emr );
    return ret;
}

/**********************************************************************
 *          EMFDRV_PaintRgn
 */
BOOL
EMFDRV_PaintRgn( DC *dc, HRGN hrgn )
{
    return EMFDRV_PaintInvertRgn( dc, hrgn, EMR_PAINTRGN );
}

/**********************************************************************
 *          EMFDRV_InvertRgn
 */
BOOL
EMFDRV_InvertRgn( DC *dc, HRGN hrgn )
{
    return EMFDRV_PaintInvertRgn( dc, hrgn, EMR_INVERTRGN );
}

/**********************************************************************
 *          EMFDRV_SetBkColor
 */
COLORREF
EMFDRV_SetBkColor( DC *dc, COLORREF color )
{
    EMRSETBKCOLOR emr;

    emr.emr.iType = EMR_SETBKCOLOR;
    emr.emr.nSize = sizeof(emr);
    emr.crColor = color;

    return EMFDRV_WriteRecord( dc, &emr.emr );
}


/**********************************************************************
 *          EMFDRV_SetTextColor
 */
COLORREF
EMFDRV_SetTextColor( DC *dc, COLORREF color )
{
    EMRSETTEXTCOLOR emr;

    emr.emr.iType = EMR_SETTEXTCOLOR;
    emr.emr.nSize = sizeof(emr);
    emr.crColor = color;

    return EMFDRV_WriteRecord( dc, &emr.emr );
}
