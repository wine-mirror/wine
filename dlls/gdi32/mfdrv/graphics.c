/*
 * Metafile driver graphics functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
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
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "mfdrv/metafiledrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(metafile);

/**********************************************************************
 *	     METADC_MoveTo
 */
BOOL METADC_MoveTo( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_MOVETO, x, y );
}

/***********************************************************************
 *           METADC_LineTo
 */
BOOL METADC_LineTo( HDC hdc, INT x, INT y )
{
     return metadc_param2( hdc, META_LINETO, x, y );
}


/***********************************************************************
 *           METADC_Arc
 */
BOOL METADC_Arc( HDC hdc, INT left, INT top, INT right, INT bottom,
                 INT xstart, INT ystart, INT xend, INT yend )
{
     return metadc_param8( hdc, META_ARC, left, top, right, bottom,
                           xstart, ystart, xend, yend );
}


/***********************************************************************
 *           MFDRV_ArcTo
 */
BOOL CDECL MFDRV_ArcTo( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                        INT xstart, INT ystart, INT xend, INT yend )
{
    return FALSE;
}


/***********************************************************************
 *           METADC_Pie
 */
BOOL METADC_Pie( HDC hdc, INT left, INT top, INT right, INT bottom,
                 INT xstart, INT ystart, INT xend, INT yend )
{
    return metadc_param8( hdc, META_PIE, left, top, right, bottom,
                          xstart, ystart, xend, yend );
}


/***********************************************************************
 *           METADC_Chord
 */
BOOL METADC_Chord( HDC hdc, INT left, INT top, INT right, INT bottom,
                   INT xstart, INT ystart, INT xend, INT yend )
{
    return metadc_param8( hdc, META_CHORD, left, top, right, bottom,
                          xstart, ystart, xend, yend );
}

/***********************************************************************
 *           METADC_Ellipse
 */
BOOL METADC_Ellipse( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    return metadc_param4( hdc, META_ELLIPSE, left, top, right, bottom );
}

/***********************************************************************
 *           METADC_Rectangle
 */
BOOL METADC_Rectangle( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    return metadc_param4( hdc, META_RECTANGLE, left, top, right, bottom );
}

/***********************************************************************
 *           MF_RoundRect
 */
BOOL METADC_RoundRect( HDC hdc, INT left, INT top, INT right,
                       INT bottom, INT ell_width, INT ell_height )
{
    return metadc_param6( hdc, META_ROUNDRECT, left, top, right, bottom,
                          ell_width, ell_height );
}

/***********************************************************************
 *           METADC_SetPixel
 */
BOOL METADC_SetPixel( HDC hdc, INT x, INT y, COLORREF color )
{
    return metadc_param4( hdc, META_SETPIXEL, x, y, HIWORD(color), LOWORD(color) );
}


/******************************************************************
 *         metadc_poly - implements Polygon and Polyline
 */
static BOOL metadc_poly( HDC hdc, short func, POINTS *pt, short count )
{
    BOOL ret;
    DWORD len;
    METARECORD *mr;

    len = sizeof(METARECORD) + (count * 4);
    if (!(mr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len )))
	return FALSE;

    mr->rdSize = len / 2;
    mr->rdFunction = func;
    *(mr->rdParm) = count;
    memcpy(mr->rdParm + 1, pt, count * 4);
    ret = metadc_record( hdc, mr, mr->rdSize * 2);
    HeapFree( GetProcessHeap(), 0, mr);
    return ret;
}


/**********************************************************************
 *          METADC_Polyline
 */
BOOL METADC_Polyline( HDC hdc, const POINT *pt, INT count )
{
    int i;
    POINTS *pts;
    BOOL ret;

    pts = HeapAlloc( GetProcessHeap(), 0, sizeof(POINTS)*count );
    if(!pts) return FALSE;
    for (i=count;i--;)
    {
        pts[i].x = pt[i].x;
        pts[i].y = pt[i].y;
    }
    ret = metadc_poly( hdc, META_POLYLINE, pts, count );

    HeapFree( GetProcessHeap(), 0, pts );
    return ret;
}


/**********************************************************************
 *          METADC_Polygon
 */
BOOL METADC_Polygon( HDC hdc, const POINT *pt, INT count )
{
    int i;
    POINTS *pts;
    BOOL ret;

    pts = HeapAlloc( GetProcessHeap(), 0, sizeof(POINTS)*count );
    if(!pts) return FALSE;
    for (i=count;i--;)
    {
        pts[i].x = pt[i].x;
        pts[i].y = pt[i].y;
    }
    ret = metadc_poly( hdc, META_POLYGON, pts, count );

    HeapFree( GetProcessHeap(), 0, pts );
    return ret;
}


/**********************************************************************
 *          METADC_PolyPolygon
 */
BOOL METADC_PolyPolygon( HDC hdc, const POINT *pt, const INT *counts, UINT polygons )
{
    BOOL ret;
    DWORD len;
    METARECORD *mr;
    unsigned int i,j;
    POINTS *pts;
    INT16 totalpoint16 = 0;
    INT16 * pointcounts;

    for (i=0;i<polygons;i++) {
         totalpoint16 += counts[i];
    }

    /* allocate space for all points */
    pts=HeapAlloc( GetProcessHeap(), 0, sizeof(POINTS) * totalpoint16 );
    pointcounts = HeapAlloc( GetProcessHeap(), 0, sizeof(INT16) * totalpoint16 );

    /* copy point counts */
    for (i=0;i<polygons;i++) {
          pointcounts[i] = counts[i];
    }

    /* convert all points */
    for (j = totalpoint16; j--;){
        pts[j].x = pt[j].x;
        pts[j].y = pt[j].y;
    }

    len = sizeof(METARECORD) + sizeof(WORD) + polygons*sizeof(INT16) + totalpoint16*sizeof(*pts);

    if (!(mr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len ))) {
         HeapFree( GetProcessHeap(), 0, pts );
         HeapFree( GetProcessHeap(), 0, pointcounts );
         return FALSE;
    }

    mr->rdSize = len /2;
    mr->rdFunction = META_POLYPOLYGON;
    *(mr->rdParm) = polygons;
    memcpy(mr->rdParm + 1, pointcounts, polygons*sizeof(INT16));
    memcpy(mr->rdParm + 1+polygons, pts , totalpoint16*sizeof(*pts));
    ret = metadc_record( hdc, mr, mr->rdSize * 2);

    HeapFree( GetProcessHeap(), 0, pts );
    HeapFree( GetProcessHeap(), 0, pointcounts );
    HeapFree( GetProcessHeap(), 0, mr);
    return ret;
}


/**********************************************************************
 *          METADC_ExtFloodFill
 */
BOOL METADC_ExtFloodFill( HDC hdc, INT x, INT y, COLORREF color, UINT fill_type )
{
    return metadc_param5( hdc, META_EXTFLOODFILL, x, y, HIWORD(color), LOWORD(color), fill_type );
}


/******************************************************************
 *         MFDRV_CreateRegion
 *
 * For explanation of the format of the record see MF_Play_MetaCreateRegion in
 * objects/metafile.c
 */
static INT16 MFDRV_CreateRegion(PHYSDEV dev, HRGN hrgn)
{
    DWORD len;
    METARECORD *mr;
    RGNDATA *rgndata;
    RECT *pCurRect, *pEndRect;
    WORD Bands = 0, MaxBands = 0;
    WORD *Param, *StartBand;
    BOOL ret;

    if (!(len = NtGdiGetRegionData( hrgn, 0, NULL ))) return -1;
    if( !(rgndata = HeapAlloc( GetProcessHeap(), 0, len )) ) {
        WARN("Can't alloc rgndata buffer\n");
	return -1;
    }
    NtGdiGetRegionData( hrgn, len, rgndata );

    /* Overestimate of length:
     * Assume every rect is a separate band -> 6 WORDs per rect
     */
    len = sizeof(METARECORD) + 20 + (rgndata->rdh.nCount * 12);
    if( !(mr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len )) ) {
        WARN("Can't alloc METARECORD buffer\n");
	HeapFree( GetProcessHeap(), 0, rgndata );
	return -1;
    }

    Param = mr->rdParm + 11;
    StartBand = NULL;

    pEndRect = (RECT *)rgndata->Buffer + rgndata->rdh.nCount;
    for(pCurRect = (RECT *)rgndata->Buffer; pCurRect < pEndRect; pCurRect++)
    {
        if( StartBand && pCurRect->top == *(StartBand + 1) )
        {
	    *Param++ = pCurRect->left;
	    *Param++ = pCurRect->right;
	}
	else
	{
	    if(StartBand)
	    {
	        *StartBand = Param - StartBand - 3;
		*Param++ = *StartBand;
		if(*StartBand > MaxBands)
		    MaxBands = *StartBand;
		Bands++;
	    }
	    StartBand = Param++;
	    *Param++ = pCurRect->top;
	    *Param++ = pCurRect->bottom;
	    *Param++ = pCurRect->left;
	    *Param++ = pCurRect->right;
	}
    }

    if (StartBand)
    {
        *StartBand = Param - StartBand - 3;
        *Param++ = *StartBand;
        if(*StartBand > MaxBands)
            MaxBands = *StartBand;
        Bands++;
    }

    mr->rdParm[0] = 0;
    mr->rdParm[1] = 6;
    mr->rdParm[2] = 0x2f6;
    mr->rdParm[3] = 0;
    mr->rdParm[4] = (Param - &mr->rdFunction) * sizeof(WORD);
    mr->rdParm[5] = Bands;
    mr->rdParm[6] = MaxBands;
    mr->rdParm[7] = rgndata->rdh.rcBound.left;
    mr->rdParm[8] = rgndata->rdh.rcBound.top;
    mr->rdParm[9] = rgndata->rdh.rcBound.right;
    mr->rdParm[10] = rgndata->rdh.rcBound.bottom;
    mr->rdFunction = META_CREATEREGION;
    mr->rdSize = Param - (WORD *)mr;
    ret = MFDRV_WriteRecord( dev, mr, mr->rdSize * 2 );
    HeapFree( GetProcessHeap(), 0, mr );
    HeapFree( GetProcessHeap(), 0, rgndata );
    if(!ret)
    {
        WARN("MFDRV_WriteRecord failed\n");
	return -1;
    }
    return MFDRV_AddHandle( dev, hrgn );
}


/**********************************************************************
 *          METADC_PaintRgn
 */
BOOL METADC_PaintRgn( HDC hdc, HRGN hrgn )
{
    METAFILEDRV_PDEVICE *mf;
    INT16 index;
    if (!(mf = get_metadc_ptr( hdc ))) return FALSE;
    index = MFDRV_CreateRegion( &mf->dev, hrgn );
    if(index == -1)
        return FALSE;
    return MFDRV_MetaParam1( &mf->dev, META_PAINTREGION, index );
}


/**********************************************************************
 *          METADC_InvertRgn
 */
BOOL METADC_InvertRgn( HDC hdc, HRGN hrgn )
{
    METAFILEDRV_PDEVICE *mf;
    INT16 index;
    if (!(mf = get_metadc_ptr( hdc ))) return FALSE;
    index = MFDRV_CreateRegion( &mf->dev, hrgn );
    if(index == -1)
        return FALSE;
    return MFDRV_MetaParam1( &mf->dev, META_INVERTREGION, index );
}


/**********************************************************************
 *          METADC_FillRgn
 */
BOOL METADC_FillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush )
{
    METAFILEDRV_PDEVICE *mf;
    INT16 iRgn, iBrush;

    if (!(mf = get_metadc_ptr( hdc ))) return FALSE;

    iRgn = MFDRV_CreateRegion( &mf->dev, hrgn );
    if(iRgn == -1)
        return FALSE;
    iBrush = MFDRV_CreateBrushIndirect( &mf->dev, hbrush );
    if(!iBrush)
        return FALSE;
    return MFDRV_MetaParam2( &mf->dev, META_FILLREGION, iRgn, iBrush );
}

/**********************************************************************
 *          MFDRV_FillRgn
 */
BOOL CDECL MFDRV_FillRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush )
{
    return TRUE;
}

/**********************************************************************
 *          METADC_FrameRgn
 */
BOOL METADC_FrameRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush, INT x, INT y )
{
    METAFILEDRV_PDEVICE *mf;
    INT16 iRgn, iBrush;

    if (!(mf = get_metadc_ptr( hdc ))) return FALSE;
    iRgn = MFDRV_CreateRegion( &mf->dev, hrgn );
    if(iRgn == -1)
        return FALSE;
    iBrush = MFDRV_CreateBrushIndirect( &mf->dev, hbrush );
    if(!iBrush)
        return FALSE;
    return MFDRV_MetaParam4( &mf->dev, META_FRAMEREGION, iRgn, iBrush, x, y );
}


/**********************************************************************
 *          METADC_ExtSelectClipRgn
 */
BOOL METADC_ExtSelectClipRgn( HDC hdc, HRGN hrgn, INT mode )
{
    METAFILEDRV_PDEVICE *metadc;
    INT16 iRgn;
    INT ret;

    if (!(metadc = get_metadc_ptr( hdc ))) return FALSE;
    if (mode != RGN_COPY) return ERROR;
    if (!hrgn) return NULLREGION;
    iRgn = MFDRV_CreateRegion( &metadc->dev, hrgn );
    if(iRgn == -1) return ERROR;
    ret = MFDRV_MetaParam1( &metadc->dev, META_SELECTOBJECT, iRgn ) ? NULLREGION : ERROR;
    MFDRV_MetaParam1( &metadc->dev, META_DELETEOBJECT, iRgn );
    MFDRV_RemoveHandle( &metadc->dev, iRgn );
    return ret;
}


/**********************************************************************
 *          MFDRV_PolyBezier
 * Since MetaFiles don't record Beziers and they don't even record
 * approximations to them using lines, we need this stub function.
 */
BOOL CDECL MFDRV_PolyBezier( PHYSDEV dev, const POINT *pts, DWORD count )
{
    return FALSE;
}

/**********************************************************************
 *          MFDRV_PolyBezierTo
 * Since MetaFiles don't record Beziers and they don't even record
 * approximations to them using lines, we need this stub function.
 */
BOOL CDECL MFDRV_PolyBezierTo( PHYSDEV dev, const POINT *pts, DWORD count )
{
    return FALSE;
}
