/*
 * Enhanced MetaFile driver
 *
 * Copyright 1999 Huw D M Davies
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "ntgdi_private.h"


typedef struct
{
    struct gdi_physdev dev;
    INT dev_caps[COLORMGMTCAPS + 1];
} EMFDRV_PDEVICE;

static inline EMFDRV_PDEVICE *get_emf_physdev( PHYSDEV dev )
{
    return CONTAINING_RECORD( dev, EMFDRV_PDEVICE, dev );
}

static void emfdrv_update_bounds( DC *dc, RECTL *rect )
{
    RECTL *bounds = &dc->attr->emf_bounds;
    RECTL vport_rect = *rect;

    lp_to_dp( dc, (POINT *)&vport_rect, 2 );

    /* The coordinate systems may be mirrored
       (LPtoDP handles points, not rectangles) */
    if (vport_rect.left > vport_rect.right)
    {
        LONG temp = vport_rect.right;
        vport_rect.right = vport_rect.left;
        vport_rect.left = temp;
    }
    if (vport_rect.top > vport_rect.bottom)
    {
        LONG temp = vport_rect.bottom;
        vport_rect.bottom = vport_rect.top;
        vport_rect.top = temp;
    }

    if (bounds->left > bounds->right)
    {
        /* first bounding rectangle */
        *bounds = vport_rect;
    }
    else
    {
        bounds->left   = min( bounds->left,   vport_rect.left );
        bounds->top    = min( bounds->top,    vport_rect.top );
        bounds->right  = max( bounds->right,  vport_rect.right );
        bounds->bottom = max( bounds->bottom, vport_rect.bottom );
    }
}

static BOOL EMFDRV_LineTo( PHYSDEV dev, INT x, INT y )
{
    DC *dc = get_physdev_dc( dev );
    RECTL bounds;
    POINT pt;

    pt = dc->attr->cur_pos;

    bounds.left   = min( x, pt.x );
    bounds.top    = min( y, pt.y );
    bounds.right  = max( x, pt.x );
    bounds.bottom = max( y, pt.y );
    emfdrv_update_bounds( dc, &bounds );
    return TRUE;
}

static BOOL EMFDRV_RoundRect( PHYSDEV dev, INT left, INT top, INT right,
                              INT bottom, INT ell_width, INT ell_height )
{
    DC *dc = get_physdev_dc( dev );
    RECTL bounds;

    if (left == right || top == bottom) return FALSE;

    bounds.left   = min( left, right );
    bounds.top    = min( top, bottom );
    bounds.right  = max( left, right );
    bounds.bottom = max( top, bottom );
    if (dc->attr->graphics_mode == GM_COMPATIBLE)
    {
        bounds.right--;
        bounds.bottom--;
    }

    emfdrv_update_bounds( dc, &bounds );
    return TRUE;
}

static BOOL EMFDRV_ArcChordPie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                                INT xstart, INT ystart, INT xend, INT yend, DWORD type )
{
    DC *dc = get_physdev_dc( dev );
    INT temp, x_centre, y_centre, i;
    double angle_start, angle_end;
    double xinter_start, yinter_start, xinter_end, yinter_end;
    EMRARC emr;
    RECTL bounds;

    if (left == right || top == bottom) return FALSE;

    if (left > right) { temp = left; left = right; right = temp; }
    if (top > bottom) { temp = top; top = bottom; bottom = temp; }

    if (dc->attr->graphics_mode == GM_COMPATIBLE)
    {
        right--;
        bottom--;
    }

    emr.emr.iType     = type;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = left;
    emr.rclBox.top    = top;
    emr.rclBox.right  = right;
    emr.rclBox.bottom = bottom;
    emr.ptlStart.x    = xstart;
    emr.ptlStart.y    = ystart;
    emr.ptlEnd.x      = xend;
    emr.ptlEnd.y      = yend;

    /* Now calculate the BBox */
    x_centre = (left + right + 1) / 2;
    y_centre = (top + bottom + 1) / 2;

    xstart -= x_centre;
    ystart -= y_centre;
    xend   -= x_centre;
    yend   -= y_centre;

    /* invert y co-ords to get angle anti-clockwise from x-axis */
    angle_start = atan2( -(double)ystart, (double)xstart );
    angle_end   = atan2( -(double)yend, (double)xend );

    /* These are the intercepts of the start/end lines with the arc */
    xinter_start = (right - left + 1)/2 * cos(angle_start) + x_centre;
    yinter_start = -(bottom - top + 1)/2 * sin(angle_start) + y_centre;
    xinter_end   = (right - left + 1)/2 * cos(angle_end) + x_centre;
    yinter_end   = -(bottom - top + 1)/2 * sin(angle_end) + y_centre;

    if (angle_start < 0) angle_start += 2 * M_PI;
    if (angle_end < 0) angle_end += 2 * M_PI;
    if (angle_end < angle_start) angle_end += 2 * M_PI;

    bounds.left   = min( xinter_start, xinter_end );
    bounds.top    = min( yinter_start, yinter_end );
    bounds.right  = max( xinter_start, xinter_end );
    bounds.bottom = max( yinter_start, yinter_end );

    for (i = 0; i <= 8; i++)
    {
        if(i * M_PI / 2 < angle_start) /* loop until we're past start */
	    continue;
	if(i * M_PI / 2 > angle_end)   /* if we're past end we're finished */
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
    if (type == EMR_PIE)
    {
        if (bounds.left > x_centre) bounds.left = x_centre;
	else if (bounds.right < x_centre) bounds.right = x_centre;
	if (bounds.top > y_centre) bounds.top = y_centre;
	else if (bounds.bottom < y_centre) bounds.bottom = y_centre;
    }
    else if (type == EMR_ARCTO)
    {
        POINT pt;
        pt = dc->attr->cur_pos;
        bounds.left   = min( bounds.left, pt.x );
        bounds.top    = min( bounds.top, pt.y );
        bounds.right  = max( bounds.right, pt.x );
        bounds.bottom = max( bounds.bottom, pt.y );
    }
    emfdrv_update_bounds( dc, &bounds );
    return TRUE;
}

static BOOL EMFDRV_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                        INT xstart, INT ystart, INT xend, INT yend )
{
    return EMFDRV_ArcChordPie( dev, left, top, right, bottom, xstart, ystart,
                               xend, yend, EMR_ARC );
}

static BOOL EMFDRV_ArcTo( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                          INT xstart, INT ystart, INT xend, INT yend )
{
    return EMFDRV_ArcChordPie( dev, left, top, right, bottom, xstart, ystart,
                               xend, yend, EMR_ARCTO );
}

static BOOL EMFDRV_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                        INT xstart, INT ystart, INT xend, INT yend )
{
    return EMFDRV_ArcChordPie( dev, left, top, right, bottom, xstart, ystart,
                               xend, yend, EMR_PIE );
}

static BOOL EMFDRV_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                          INT xstart, INT ystart, INT xend, INT yend )
{
    return EMFDRV_ArcChordPie( dev, left, top, right, bottom, xstart, ystart,
                               xend, yend, EMR_CHORD );
}

static BOOL EMFDRV_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    DC *dc = get_physdev_dc( dev );
    RECTL bounds;

    if (left == right || top == bottom) return FALSE;

    bounds.left   = min( left, right );
    bounds.top    = min( top, bottom );
    bounds.right  = max( left, right );
    bounds.bottom = max( top, bottom );
    if (dc->attr->graphics_mode == GM_COMPATIBLE)
    {
        bounds.right--;
        bounds.bottom--;
    }

    emfdrv_update_bounds( dc, &bounds );
    return TRUE;
}

static BOOL EMFDRV_Rectangle( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    DC *dc = get_physdev_dc( dev );
    RECTL bounds;

    if (left == right || top == bottom) return FALSE;

    bounds.left   = min( left, right );
    bounds.top    = min( top, bottom );
    bounds.right  = max( left, right );
    bounds.bottom = max( top, bottom );
    if (dc->attr->graphics_mode == GM_COMPATIBLE)
    {
        bounds.right--;
        bounds.bottom--;
    }

    emfdrv_update_bounds( dc, &bounds );
    return TRUE;
}

static COLORREF EMFDRV_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color )
{
    DC *dc = get_physdev_dc( dev );
    RECTL bounds;

    bounds.left = bounds.right = x;
    bounds.top = bounds.bottom = y;
    emfdrv_update_bounds( dc, &bounds );
    return CLR_INVALID;
}

static BOOL EMFDRV_PolylineTo( PHYSDEV dev, const POINT *pt, INT count )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

static BOOL EMFDRV_PolyBezier( PHYSDEV dev, const POINT *pts, DWORD count )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

static BOOL EMFDRV_PolyBezierTo( PHYSDEV dev, const POINT *pts, DWORD count )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

static BOOL EMFDRV_PolyPolyline( PHYSDEV dev, const POINT *pt, const DWORD *counts, DWORD polys )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

static BOOL EMFDRV_PolyPolygon( PHYSDEV dev, const POINT *pt, const INT *counts, UINT polys )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

static BOOL EMFDRV_PolyDraw( PHYSDEV dev, const POINT *pts, const BYTE *types, DWORD count )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

static BOOL EMFDRV_FillRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

static BOOL EMFDRV_FrameRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush, INT width, INT height )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

static BOOL EMFDRV_InvertRgn( PHYSDEV dev, HRGN hrgn )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

static BOOL EMFDRV_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *lprect,
                               LPCWSTR str, UINT count, const INT *lpDx )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

static BOOL EMFDRV_GradientFill( PHYSDEV dev, TRIVERTEX *vert_array, ULONG nvert,
                                 void *grad_array, ULONG ngrad, ULONG mode )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

static BOOL EMFDRV_FillPath( PHYSDEV dev )
{
    /* FIXME: update bound rect */
    return TRUE;
}

static BOOL EMFDRV_StrokeAndFillPath( PHYSDEV dev )
{
    /* FIXME: update bound rect */
    return TRUE;
}

static BOOL EMFDRV_StrokePath( PHYSDEV dev )
{
    /* FIXME: update bound rect */
    return TRUE;
}

static BOOL EMFDRV_AlphaBlend( PHYSDEV dev_dst, struct bitblt_coords *dst,
                               PHYSDEV dev_src, struct bitblt_coords *src,
                               BLENDFUNCTION func )
{
    /* FIXME: update bound rect */
    return TRUE;
}

static BOOL EMFDRV_PatBlt( PHYSDEV dev, struct bitblt_coords *dst, DWORD rop )
{
    /* FIXME: update bound rect */
    return TRUE;
}

static HBITMAP EMFDRV_SelectBitmap( PHYSDEV dev, HBITMAP hbitmap )
{
    return 0;
}

static HFONT EMFDRV_SelectFont( PHYSDEV dev, HFONT font, UINT *aa_flags )
{
    *aa_flags = GGO_BITMAP;  /* no point in anti-aliasing on metafiles */

    dev = GET_NEXT_PHYSDEV( dev, pSelectFont );
    return dev->funcs->pSelectFont( dev, font, aa_flags );
}

static INT EMFDRV_GetDeviceCaps( PHYSDEV dev, INT cap )
{
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );

    if (cap >= 0 && cap < ARRAY_SIZE( physDev->dev_caps ))
        return physDev->dev_caps[cap];
    return 0;
}

static BOOL EMFDRV_DeleteDC( PHYSDEV dev )
{
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    free( physDev );
    return TRUE;
}

static const struct gdi_dc_funcs emfdrv_driver =
{
    NULL,                            /* pAbortDoc */
    NULL,                            /* pAbortPath */
    EMFDRV_AlphaBlend,               /* pAlphaBlend */
    NULL,                            /* pAngleArc */
    EMFDRV_Arc,                      /* pArc */
    EMFDRV_ArcTo,                    /* pArcTo */
    NULL,                            /* pBeginPath */
    NULL,                            /* pBlendImage */
    EMFDRV_Chord,                    /* pChord */
    NULL,                            /* pCloseFigure */
    NULL,                            /* pCreateCompatibleDC */
    NULL,                            /* pCreateDC */
    EMFDRV_DeleteDC,                 /* pDeleteDC */
    NULL,                            /* pDeleteObject */
    EMFDRV_Ellipse,                  /* pEllipse */
    NULL,                            /* pEndDoc */
    NULL,                            /* pEndPage */
    NULL,                            /* pEndPath */
    NULL,                            /* pEnumFonts */
    NULL,                            /* pExtEscape */
    NULL,                            /* pExtFloodFill */
    EMFDRV_ExtTextOut,               /* pExtTextOut */
    EMFDRV_FillPath,                 /* pFillPath */
    EMFDRV_FillRgn,                  /* pFillRgn */
    NULL,                            /* pFontIsLinked */
    EMFDRV_FrameRgn,                 /* pFrameRgn */
    NULL,                            /* pGetBoundsRect */
    NULL,                            /* pGetCharABCWidths */
    NULL,                            /* pGetCharABCWidthsI */
    NULL,                            /* pGetCharWidth */
    NULL,                            /* pGetCharWidthInfo */
    EMFDRV_GetDeviceCaps,            /* pGetDeviceCaps */
    NULL,                            /* pGetDeviceGammaRamp */
    NULL,                            /* pGetFontData */
    NULL,                            /* pGetFontRealizationInfo */
    NULL,                            /* pGetFontUnicodeRanges */
    NULL,                            /* pGetGlyphIndices */
    NULL,                            /* pGetGlyphOutline */
    NULL,                            /* pGetICMProfile */
    NULL,                            /* pGetImage */
    NULL,                            /* pGetKerningPairs */
    NULL,                            /* pGetNearestColor */
    NULL,                            /* pGetOutlineTextMetrics */
    NULL,                            /* pGetPixel */
    NULL,                            /* pGetSystemPaletteEntries */
    NULL,                            /* pGetTextCharsetInfo */
    NULL,                            /* pGetTextExtentExPoint */
    NULL,                            /* pGetTextExtentExPointI */
    NULL,                            /* pGetTextFace */
    NULL,                            /* pGetTextMetrics */
    EMFDRV_GradientFill,             /* pGradientFill */
    EMFDRV_InvertRgn,                /* pInvertRgn */
    EMFDRV_LineTo,                   /* pLineTo */
    NULL,                            /* pMoveTo */
    NULL,                            /* pPaintRgn */
    EMFDRV_PatBlt,                   /* pPatBlt */
    EMFDRV_Pie,                      /* pPie */
    EMFDRV_PolyBezier,               /* pPolyBezier */
    EMFDRV_PolyBezierTo,             /* pPolyBezierTo */
    EMFDRV_PolyDraw,                 /* pPolyDraw */
    EMFDRV_PolyPolygon,              /* pPolyPolygon */
    EMFDRV_PolyPolyline,             /* pPolyPolyline */
    EMFDRV_PolylineTo,               /* pPolylineTo */
    NULL,                            /* pPutImage */
    NULL,                            /* pRealizeDefaultPalette */
    NULL,                            /* pRealizePalette */
    EMFDRV_Rectangle,                /* pRectangle */
    NULL,                            /* pResetDC */
    EMFDRV_RoundRect,                /* pRoundRect */
    EMFDRV_SelectBitmap,             /* pSelectBitmap */
    NULL,                            /* pSelectBrush */
    EMFDRV_SelectFont,               /* pSelectFont */
    NULL,                            /* pSelectPen */
    NULL,                            /* pSetBkColor */
    NULL,                            /* pSetBoundsRect */
    NULL,                            /* pSetDCBrushColor*/
    NULL,                            /* pSetDCPenColor*/
    NULL,                            /* pSetDIBitsToDevice */
    NULL,                            /* pSetDeviceClipping */
    NULL,                            /* pSetDeviceGammaRamp */
    EMFDRV_SetPixel,                 /* pSetPixel */
    NULL,                            /* pSetTextColor */
    NULL,                            /* pStartDoc */
    NULL,                            /* pStartPage */
    NULL,                            /* pStretchBlt */
    NULL,                            /* pStretchDIBits */
    EMFDRV_StrokeAndFillPath,        /* pStrokeAndFillPath */
    EMFDRV_StrokePath,               /* pStrokePath */
    NULL,                            /* pUnrealizePalette */
    NULL,                            /* pD3DKMTCheckVidPnExclusiveOwnership */
    NULL,                            /* pD3DKMTCloseAdapter */
    NULL,                            /* pD3DKMTOpenAdapterFromLuid */
    NULL,                            /* pD3DKMTQueryVideoMemoryInfo */
    NULL,                            /* pD3DKMTSetVidPnSourceOwner */
    GDI_PRIORITY_GRAPHICS_DRV        /* priority */
};


static BOOL devcap_is_valid( int cap )
{
    if (cap >= 0 && cap <= ASPECTXY) return !(cap & 1);
    if (cap >= PHYSICALWIDTH && cap <= COLORMGMTCAPS) return TRUE;
    switch (cap)
    {
    case LOGPIXELSX:
    case LOGPIXELSY:
    case CAPS1:
    case SIZEPALETTE:
    case NUMRESERVED:
    case COLORRES:
        return TRUE;
    }
    return FALSE;
}

/**********************************************************************
 *           NtGdiCreateMetafileDC   (win32u.@)
 */
HDC WINAPI NtGdiCreateMetafileDC( HDC hdc )
{
    EMFDRV_PDEVICE *physDev;
    HDC ref_dc, ret;
    int cap;
    DC *dc;

    if (!(dc = alloc_dc_ptr( NTGDI_OBJ_ENHMETADC ))) return 0;

    physDev = malloc( sizeof(*physDev) );
    if (!physDev)
    {
        free_dc_ptr( dc );
        return 0;
    }

    push_dc_driver( &dc->physDev, &physDev->dev, &emfdrv_driver );

    if (hdc)  /* if no ref, use current display */
        ref_dc = hdc;
    else
        ref_dc = NtGdiOpenDCW( NULL, NULL, NULL, 0, TRUE, NULL, NULL, NULL );

    memset( physDev->dev_caps, 0, sizeof(physDev->dev_caps) );
    for (cap = 0; cap < ARRAY_SIZE( physDev->dev_caps ); cap++)
        if (devcap_is_valid( cap ))
            physDev->dev_caps[cap] = NtGdiGetDeviceCaps( ref_dc, cap );

    if (!hdc) NtGdiDeleteObjectApp( ref_dc );

    NtGdiSetVirtualResolution( dc->hSelf, 0, 0, 0, 0 );

    ret = dc->hSelf;
    release_dc_ptr( dc );
    return ret;
}
