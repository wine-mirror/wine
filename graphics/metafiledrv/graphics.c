/*
 * Metafile driver graphics functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <stdlib.h>
#include "gdi.h"
#include "dc.h"
#include "metafile.h"
#include "region.h"
#include "xmalloc.h"
#include "metafiledrv.h"
#include "debug.h"

/**********************************************************************
 *	     MFDRV_MoveToEx
 */
BOOL
MFDRV_MoveToEx(DC *dc,INT x,INT y,LPPOINT pt)
{
    if (!MF_MetaParam2(dc,META_MOVETO,x,y))
    	return FALSE;

    if (pt)
    {
	pt->x = dc->w.CursPosX;
	pt->y = dc->w.CursPosY;
    }
    dc->w.CursPosX = x;
    dc->w.CursPosY = y;
    return TRUE;
}

/***********************************************************************
 *           MFDRV_LineTo
 */
BOOL
MFDRV_LineTo( DC *dc, INT x, INT y )
{
     return MF_MetaParam2(dc, META_LINETO, x, y);
}


/***********************************************************************
 *           MFDRV_Arc
 */
BOOL 
MFDRV_Arc( DC *dc, INT left, INT top, INT right, INT bottom,
           INT xstart, INT ystart, INT xend, INT yend )
{
     return MF_MetaParam8(dc, META_ARC, left, top, right, bottom,
			  xstart, ystart, xend, yend);
}


/***********************************************************************
 *           MFDRV_Pie
 */
BOOL
MFDRV_Pie( DC *dc, INT left, INT top, INT right, INT bottom,
           INT xstart, INT ystart, INT xend, INT yend )
{
    return MF_MetaParam8(dc, META_PIE, left, top, right, bottom,
			 xstart, ystart, xend, yend);
}


/***********************************************************************
 *           MFDRV_Chord
 */
BOOL
MFDRV_Chord( DC *dc, INT left, INT top, INT right, INT bottom,
             INT xstart, INT ystart, INT xend, INT yend )
{
    return MF_MetaParam8(dc, META_CHORD, left, top, right, bottom,
			 xstart, ystart, xend, yend);
}

/***********************************************************************
 *           MFDRV_Ellipse
 */
BOOL
MFDRV_Ellipse( DC *dc, INT left, INT top, INT right, INT bottom )
{
    return MF_MetaParam4(dc, META_ELLIPSE, left, top, right, bottom);
}

/***********************************************************************
 *           MFDRV_Rectangle
 */
BOOL
MFDRV_Rectangle(DC *dc, INT left, INT top, INT right, INT bottom)
{
    return MF_MetaParam4(dc, META_RECTANGLE, left, top, right, bottom);
}

/***********************************************************************
 *           MFDRV_RoundRect
 */
BOOL 
MFDRV_RoundRect( DC *dc, INT left, INT top, INT right,
                 INT bottom, INT ell_width, INT ell_height )
{
    return MF_MetaParam6(dc, META_ROUNDRECT, left, top, right, bottom,
			 ell_width, ell_height);
}

/***********************************************************************
 *           MFDRV_SetPixel
 */
COLORREF
MFDRV_SetPixel( DC *dc, INT x, INT y, COLORREF color )
{
    return MF_MetaParam4(dc, META_SETPIXEL, x, y,HIWORD(color),LOWORD(color)); 
}


/**********************************************************************
 *          MFDRV_Polyline
 */
BOOL
MFDRV_Polyline( DC *dc, const POINT* pt, INT count )
{
    register int i;
    LPPOINT16	pt16;
    BOOL16	ret;

    pt16 = (LPPOINT16)xmalloc(sizeof(POINT16)*count);
    for (i=count;i--;) CONV_POINT32TO16(&(pt[i]),&(pt16[i]));
    ret = MF_MetaPoly(dc, META_POLYLINE, pt16, count); 

    free(pt16);
    return ret;
}


/**********************************************************************
 *          MFDRV_Polygon
 */
BOOL
MFDRV_Polygon( DC *dc, const POINT* pt, INT count )
{
    register int i;
    LPPOINT16	pt16;
    BOOL16	ret;

    pt16 = (LPPOINT16)xmalloc(sizeof(POINT16)*count);
    for (i=count;i--;) CONV_POINT32TO16(&(pt[i]),&(pt16[i]));
    ret = MF_MetaPoly(dc, META_POLYGON, pt16, count); 

    free(pt16);
    return ret;
}


/**********************************************************************
 *          PolyPolygon
 */
BOOL 
MFDRV_PolyPolygon( DC *dc, const POINT* pt, const INT* counts, UINT polygons)
{
    int		i,j;
    LPPOINT16	pt16;
    const POINT* curpt=pt;
    BOOL	ret;

    for (i=0;i<polygons;i++) {
    	pt16=(LPPOINT16)xmalloc(sizeof(POINT16)*counts[i]);
	for (j=counts[i];j--;) CONV_POINT32TO16(&(curpt[j]),&(pt16[j]));
	ret = MF_MetaPoly(dc, META_POLYGON, pt16, counts[i]);
	free(pt16);
	if (!ret)
	    return FALSE;
	curpt+=counts[i];
    }
    return TRUE;
}


/**********************************************************************
 *          MFDRV_ExtFloodFill
 */
BOOL 
MFDRV_ExtFloodFill( DC *dc, INT x, INT y, COLORREF color, UINT fillType )
{
    return MF_MetaParam4(dc,META_FLOODFILL,x,y,HIWORD(color),LOWORD(color)); 
}


/**********************************************************************
 *          MFDRV_PaintRgn
 */
BOOL
MFDRV_PaintRgn( DC *dc, HRGN hrgn )
{
    INT16 index;
    index = MF_CreateRegion( dc, hrgn );
    if(index == -1)
        return FALSE;
    return MF_MetaParam1( dc, META_PAINTREGION, index );
}


/**********************************************************************
 *          MFDRV_SetBkColor
 */
COLORREF
MFDRV_SetBkColor( DC *dc, COLORREF color )
{
    return MF_MetaParam2(dc, META_SETBKCOLOR, HIWORD(color), LOWORD(color));
}


/**********************************************************************
 *          MFDRV_SetTextColor
 */
COLORREF
MFDRV_SetTextColor( DC *dc, COLORREF color )
{
    return MF_MetaParam2(dc, META_SETTEXTCOLOR, HIWORD(color), LOWORD(color));
}

