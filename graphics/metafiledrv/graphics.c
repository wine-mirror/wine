/*
 * Metafile driver graphics functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <stdlib.h>
#include "graphics.h"
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
BOOL32
MFDRV_MoveToEx(DC *dc,INT32 x,INT32 y,LPPOINT32 pt)
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
BOOL32
MFDRV_LineTo( DC *dc, INT32 x, INT32 y )
{
     return MF_MetaParam2(dc, META_LINETO, x, y);
}


/***********************************************************************
 *           MFDRV_Arc
 */
BOOL32 
MFDRV_Arc( DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom,
           INT32 xstart, INT32 ystart, INT32 xend, INT32 yend )
{
     return MF_MetaParam8(dc, META_ARC, left, top, right, bottom,
			  xstart, ystart, xend, yend);
}


/***********************************************************************
 *           MFDRV_Pie
 */
BOOL32
MFDRV_Pie( DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom,
           INT32 xstart, INT32 ystart, INT32 xend, INT32 yend )
{
    return MF_MetaParam8(dc, META_PIE, left, top, right, bottom,
			 xstart, ystart, xend, yend);
}


/***********************************************************************
 *           MFDRV_Chord
 */
BOOL32
MFDRV_Chord( DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom,
             INT32 xstart, INT32 ystart, INT32 xend, INT32 yend )
{
    return MF_MetaParam8(dc, META_CHORD, left, top, right, bottom,
			 xstart, ystart, xend, yend);
}

/***********************************************************************
 *           MFDRV_Ellipse
 */
BOOL32
MFDRV_Ellipse( DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom )
{
    return MF_MetaParam4(dc, META_ELLIPSE, left, top, right, bottom);
}

/***********************************************************************
 *           MFDRV_Rectangle
 */
BOOL32
MFDRV_Rectangle(DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom)
{
    return MF_MetaParam4(dc, META_RECTANGLE, left, top, right, bottom);
}

/***********************************************************************
 *           MFDRV_RoundRect
 */
BOOL32 
MFDRV_RoundRect( DC *dc, INT32 left, INT32 top, INT32 right,
                 INT32 bottom, INT32 ell_width, INT32 ell_height )
{
    return MF_MetaParam6(dc, META_ROUNDRECT, left, top, right, bottom,
			 ell_width, ell_height);
}

/***********************************************************************
 *           MFDRV_SetPixel
 */
COLORREF
MFDRV_SetPixel( DC *dc, INT32 x, INT32 y, COLORREF color )
{
    return MF_MetaParam4(dc, META_SETPIXEL, x, y,HIWORD(color),LOWORD(color)); 
}


/**********************************************************************
 *          MFDRV_Polyline
 */
BOOL32
MFDRV_Polyline( DC *dc, const POINT32* pt, INT32 count )
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
BOOL32
MFDRV_Polygon( DC *dc, const POINT32* pt, INT32 count )
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
BOOL32 
MFDRV_PolyPolygon( DC *dc, const POINT32* pt, const INT32* counts, UINT32 polygons)
{
    int		i,j;
    LPPOINT16	pt16;
    const POINT32* curpt=pt;
    BOOL32	ret;

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
BOOL32 
MFDRV_ExtFloodFill( DC *dc, INT32 x, INT32 y, COLORREF color, UINT32 fillType )
{
    return MF_MetaParam4(dc,META_FLOODFILL,x,y,HIWORD(color),LOWORD(color)); 
}


/**********************************************************************
 *          MFDRV_PaintRgn
 */
BOOL32
MFDRV_PaintRgn( DC *dc, HRGN32 hrgn )
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

