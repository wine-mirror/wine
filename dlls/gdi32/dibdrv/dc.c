/*
 * DIB driver initialization and DC functions.
 *
 * Copyright 2011 Huw Davies
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

#include <assert.h>

#include "gdi_private.h"
#include "dibdrv.h"

#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dib);

static const DWORD bit_fields_888[3] = {0xff0000, 0x00ff00, 0x0000ff};
static const DWORD bit_fields_555[3] = {0x7c00, 0x03e0, 0x001f};

static void calc_shift_and_len(DWORD mask, int *shift, int *len)
{
    int s, l;

    if(!mask)
    {
        *shift = *len = 0;
        return;
    }

    s = 0;
    while ((mask & 1) == 0)
    {
        mask >>= 1;
        s++;
    }
    l = 0;
    while ((mask & 1) == 1)
    {
        mask >>= 1;
        l++;
    }
    *shift = s;
    *len = l;
}

static void init_bit_fields(dib_info *dib, const DWORD *bit_fields)
{
    dib->red_mask    = bit_fields[0];
    dib->green_mask  = bit_fields[1];
    dib->blue_mask   = bit_fields[2];
    calc_shift_and_len(dib->red_mask,   &dib->red_shift,   &dib->red_len);
    calc_shift_and_len(dib->green_mask, &dib->green_shift, &dib->green_len);
    calc_shift_and_len(dib->blue_mask,  &dib->blue_shift,  &dib->blue_len);
}

static void init_dib_info(dib_info *dib, const BITMAPINFOHEADER *bi, const DWORD *bit_fields,
                          const RGBQUAD *color_table, void *bits, enum dib_info_flags flags)
{
    dib->bit_count    = bi->biBitCount;
    dib->width        = bi->biWidth;
    dib->height       = bi->biHeight;
    dib->compression  = bi->biCompression;
    dib->stride       = get_dib_stride( dib->width, dib->bit_count );
    dib->bits.ptr     = bits;
    dib->bits.is_copy = FALSE;
    dib->bits.free    = NULL;
    dib->bits.param   = NULL;

    if(dib->height < 0) /* top-down */
    {
        dib->height = -dib->height;
    }
    else /* bottom-up */
    {
        /* bits always points to the top-left corner and the stride is -ve */
        dib->bits.ptr = (BYTE*)dib->bits.ptr + (dib->height - 1) * dib->stride;
        dib->stride  = -dib->stride;
    }

    dib->funcs = &funcs_null;

    switch(dib->bit_count)
    {
    case 32:
        if(bi->biCompression == BI_RGB)
            bit_fields = bit_fields_888;

        init_bit_fields(dib, bit_fields);

        if(dib->red_mask == 0xff0000 && dib->green_mask == 0x00ff00 && dib->blue_mask == 0x0000ff)
            dib->funcs = &funcs_8888;
        else
            dib->funcs = &funcs_32;
        break;

    case 24:
        dib->funcs = &funcs_24;
        break;

    case 16:
        if(bi->biCompression == BI_RGB)
            bit_fields = bit_fields_555;

        init_bit_fields(dib, bit_fields);

        if(dib->red_mask == 0x7c00 && dib->green_mask == 0x03e0 && dib->blue_mask == 0x001f)
            dib->funcs = &funcs_555;
        else
            dib->funcs = &funcs_16;
        break;

    case 8:
        dib->funcs = &funcs_8;
        break;

    case 4:
        dib->funcs = &funcs_4;
        break;

    case 1:
        dib->funcs = &funcs_1;
        break;
    }

    if (color_table && bi->biClrUsed)
    {
        dib->color_table = color_table;
        dib->color_table_size = bi->biClrUsed;
    }
    else if (flags & default_color_table)
    {
        dib->color_table = get_default_color_table( dib->bit_count );
        dib->color_table_size = dib->color_table ? (1 << dib->bit_count) : 0;
    }
    else
    {
        dib->color_table = NULL;
        dib->color_table_size = 0;
    }
}

void init_dib_info_from_bitmapinfo(dib_info *dib, const BITMAPINFO *info, void *bits, enum dib_info_flags flags)
{
    init_dib_info( dib, &info->bmiHeader, (const DWORD *)info->bmiColors, info->bmiColors, bits, flags );
}

BOOL init_dib_info_from_bitmapobj(dib_info *dib, BITMAPOBJ *bmp, enum dib_info_flags flags)
{
    if (!bmp->dib)
    {
        BITMAPINFO info;

        get_ddb_bitmapinfo( bmp, &info );
        if (!bmp->bitmap.bmBits)
        {
            int width_bytes = get_dib_stride( bmp->bitmap.bmWidth, bmp->bitmap.bmBitsPixel );
            bmp->bitmap.bmBits = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                            bmp->bitmap.bmHeight * width_bytes );
            if (!bmp->bitmap.bmBits) return FALSE;
        }
        init_dib_info_from_bitmapinfo( dib, &info, bmp->bitmap.bmBits, flags );
    }
    else init_dib_info( dib, &bmp->dib->dsBmih, bmp->dib->dsBitfields,
                        bmp->color_table, bmp->dib->dsBm.bmBits, flags );
    return TRUE;
}

static void clear_dib_info(dib_info *dib)
{
    dib->color_table = NULL;
    dib->bits.ptr    = NULL;
    dib->bits.free   = NULL;
    dib->bits.param  = NULL;
}

/**********************************************************************
 *      free_dib_info
 *
 * Free the resources associated with a dib and optionally the bits
 */
void free_dib_info(dib_info *dib)
{
    if (dib->bits.free) dib->bits.free( &dib->bits );
    clear_dib_info( dib );
}

void copy_dib_color_info(dib_info *dst, const dib_info *src)
{
    dst->bit_count        = src->bit_count;
    dst->red_mask         = src->red_mask;
    dst->green_mask       = src->green_mask;
    dst->blue_mask        = src->blue_mask;
    dst->red_len          = src->red_len;
    dst->green_len        = src->green_len;
    dst->blue_len         = src->blue_len;
    dst->red_shift        = src->red_shift;
    dst->green_shift      = src->green_shift;
    dst->blue_shift       = src->blue_shift;
    dst->funcs            = src->funcs;
    dst->color_table_size = src->color_table_size;
    dst->color_table      = src->color_table;
}

DWORD convert_bitmapinfo( const BITMAPINFO *src_info, void *src_bits, struct bitblt_coords *src,
                          const BITMAPINFO *dst_info, void *dst_bits, BOOL add_alpha )
{
    dib_info src_dib, dst_dib;
    DWORD ret;

    init_dib_info_from_bitmapinfo( &src_dib, src_info, src_bits, default_color_table );
    init_dib_info_from_bitmapinfo( &dst_dib, dst_info, dst_bits, default_color_table );

    __TRY
    {
        dst_dib.funcs->convert_to( &dst_dib, &src_dib, &src->visrect );
        ret = TRUE;
    }
    __EXCEPT_PAGE_FAULT
    {
        WARN( "invalid bits pointer %p\n", src_bits );
        ret = FALSE;
    }
    __ENDTRY

    /* We shared the color tables, so there's no need to free the dib_infos here */
    if(!ret) return ERROR_BAD_FORMAT;

    /* update coordinates, the destination rectangle is always stored at 0,0 */
    src->x -= src->visrect.left;
    src->y -= src->visrect.top;
    offset_rect( &src->visrect, -src->visrect.left, -src->visrect.top );

    if (add_alpha && dst_dib.funcs == &funcs_8888 && src_dib.funcs != &funcs_8888)
    {
        DWORD *pixel = dst_dib.bits.ptr;
        int x, y;

        for (y = src->visrect.top; y < src->visrect.bottom; y++, pixel += dst_dib.stride / 4)
            for (x = src->visrect.left; x < src->visrect.right; x++)
                pixel[x] |= 0xff000000;
    }

    return ERROR_SUCCESS;
}

static void update_fg_colors( dibdrv_physdev *pdev )
{
    pdev->pen_color   = get_pixel_color( pdev, pdev->pen_colorref,   TRUE );
    pdev->brush_color = get_pixel_color( pdev, pdev->brush_colorref, TRUE );
    pdev->text_color  = get_pixel_color( pdev, GetTextColor( pdev->dev.hdc ), TRUE );
}

static void update_masks( dibdrv_physdev *pdev, INT rop )
{
    calc_and_xor_masks( rop, pdev->pen_color, &pdev->pen_and, &pdev->pen_xor );
    update_brush_rop( pdev, rop );
    if( GetBkMode( pdev->dev.hdc ) == OPAQUE )
        calc_and_xor_masks( rop, pdev->bkgnd_color, &pdev->bkgnd_and, &pdev->bkgnd_xor );
}

 /***********************************************************************
 *           add_extra_clipping_region
 *
 * Temporarily add a region to the current clipping region.
 * The returned region must be restored with restore_clipping_region.
 */
HRGN add_extra_clipping_region( dibdrv_physdev *pdev, HRGN rgn )
{
    HRGN ret, clip;

    if (!(clip = CreateRectRgn( 0, 0, 0, 0 ))) return 0;
    CombineRgn( clip, pdev->clip, rgn, RGN_AND );
    ret = pdev->clip;
    pdev->clip = clip;
    return ret;
}

/***********************************************************************
 *           restore_clipping_region
 */
void restore_clipping_region( dibdrv_physdev *pdev, HRGN rgn )
{
    if (!rgn) return;
    DeleteObject( pdev->clip );
    pdev->clip = rgn;
}

/**********************************************************************
 *	     dibdrv_CreateDC
 */
static BOOL dibdrv_CreateDC( PHYSDEV *dev, LPCWSTR driver, LPCWSTR device,
                             LPCWSTR output, const DEVMODEW *data )
{
    dibdrv_physdev *pdev = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pdev) );

    if (!pdev) return FALSE;
    if (!(pdev->clip = CreateRectRgn(0, 0, 0, 0)))
    {
        HeapFree( GetProcessHeap(), 0, pdev );
        return FALSE;
    }
    clear_dib_info(&pdev->dib);
    clear_dib_info(&pdev->brush_dib);
    push_dc_driver( dev, &pdev->dev, &dib_driver );
    return TRUE;
}

/***********************************************************************
 *           dibdrv_DeleteDC
 */
static BOOL dibdrv_DeleteDC( PHYSDEV dev )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    TRACE("(%p)\n", dev);
    DeleteObject(pdev->clip);
    free_pattern_brush(pdev);
    HeapFree( GetProcessHeap(), 0, pdev );
    return TRUE;
}

/***********************************************************************
 *           dibdrv_CopyBitmap
 */
static BOOL dibdrv_CopyBitmap( HBITMAP src, HBITMAP dst )
{
    return nulldrv_CopyBitmap( src, dst );
}

/***********************************************************************
 *           dibdrv_DeleteBitmap
 */
static BOOL dibdrv_DeleteBitmap( HBITMAP bitmap )
{
    return TRUE;
}

/***********************************************************************
 *           dibdrv_SelectBitmap
 */
static HBITMAP dibdrv_SelectBitmap( PHYSDEV dev, HBITMAP bitmap )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSelectBitmap );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    BITMAPOBJ *bmp = GDI_GetObjPtr( bitmap, OBJ_BITMAP );
    dib_info dib;

    TRACE("(%p, %p)\n", dev, bitmap);

    if (!bmp) return 0;

    if(!init_dib_info_from_bitmapobj(&dib, bmp, default_color_table))
    {
        GDI_ReleaseObj( bitmap );
        return 0;
    }
    pdev->dib = dib;
    pdev->defer = 0;
    GDI_ReleaseObj( bitmap );

    return next->funcs->pSelectBitmap( next, bitmap );
}

/***********************************************************************
 *           dibdrv_SetBkColor
 */
static COLORREF dibdrv_SetBkColor( PHYSDEV dev, COLORREF color )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetBkColor );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);

    pdev->bkgnd_color = get_pixel_color( pdev, color, FALSE );

    if( GetBkMode(dev->hdc) == OPAQUE )
        calc_and_xor_masks( GetROP2(dev->hdc), pdev->bkgnd_color, &pdev->bkgnd_and, &pdev->bkgnd_xor );
    else
    {
        pdev->bkgnd_and = ~0u;
        pdev->bkgnd_xor = 0;
    }

    update_fg_colors( pdev ); /* Only needed in the 1 bpp case */

    return next->funcs->pSetBkColor( next, color );
}

/***********************************************************************
 *           dibdrv_SetBkMode
 */
static INT dibdrv_SetBkMode( PHYSDEV dev, INT mode )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetBkMode );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);

    if( mode == OPAQUE )
        calc_and_xor_masks( GetROP2(dev->hdc), pdev->bkgnd_color, &pdev->bkgnd_and, &pdev->bkgnd_xor );
    else
    {
        pdev->bkgnd_and = ~0u;
        pdev->bkgnd_xor = 0;
    }

    return next->funcs->pSetBkMode( next, mode );
}

/***********************************************************************
 *           dibdrv_SetDeviceClipping
 */
static void dibdrv_SetDeviceClipping( PHYSDEV dev, HRGN rgn )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetDeviceClipping );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    TRACE("(%p, %p)\n", dev, rgn);

    SetRectRgn( pdev->clip, 0, 0, pdev->dib.width, pdev->dib.height );
    if (rgn) CombineRgn( pdev->clip, pdev->clip, rgn, RGN_AND );
    return next->funcs->pSetDeviceClipping( next, rgn );
}

/***********************************************************************
 *           dibdrv_SetDIBColorTable
 */
static UINT dibdrv_SetDIBColorTable( PHYSDEV dev, UINT pos, UINT count, const RGBQUAD *colors )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetDIBColorTable );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    TRACE("(%p, %d, %d, %p)\n", dev, pos, count, colors);

    if (pdev->dib.color_table)
    {
        pdev->bkgnd_color = get_pixel_color( pdev, GetBkColor( dev->hdc ), FALSE );
        update_fg_colors( pdev );

        update_masks( pdev, GetROP2( dev->hdc ) );
    }
    return next->funcs->pSetDIBColorTable( next, pos, count, colors );
}

/***********************************************************************
 *           dibdrv_SetROP2
 */
static INT dibdrv_SetROP2( PHYSDEV dev, INT rop )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetROP2 );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);

    update_masks( pdev, rop );

    return next->funcs->pSetROP2( next, rop );
}

/***********************************************************************
 *           dibdrv_SetTextColor
 */
static COLORREF dibdrv_SetTextColor( PHYSDEV dev, COLORREF color )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetTextColor );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);

    pdev->text_color = get_pixel_color( pdev, color, TRUE );
    update_aa_ranges( pdev );

    return next->funcs->pSetTextColor( next, color );
}

const struct gdi_dc_funcs dib_driver =
{
    NULL,                               /* pAbortDoc */
    NULL,                               /* pAbortPath */
    dibdrv_AlphaBlend,                  /* pAlphaBlend */
    NULL,                               /* pAngleArc */
    NULL,                               /* pArc */
    NULL,                               /* pArcTo */
    NULL,                               /* pBeginPath */
    dibdrv_BlendImage,                  /* pBlendImage */
    NULL,                               /* pChoosePixelFormat */
    NULL,                               /* pChord */
    NULL,                               /* pCloseFigure */
    dibdrv_CopyBitmap,                  /* pCopyBitmap */
    NULL,                               /* pCreateBitmap */
    NULL,                               /* pCreateCompatibleDC */
    dibdrv_CreateDC,                    /* pCreateDC */
    NULL,                               /* pCreateDIBSection */
    dibdrv_DeleteBitmap,                /* pDeleteBitmap */
    dibdrv_DeleteDC,                    /* pDeleteDC */
    NULL,                               /* pDeleteObject */
    NULL,                               /* pDescribePixelFormat */
    NULL,                               /* pDeviceCapabilities */
    NULL,                               /* pEllipse */
    NULL,                               /* pEndDoc */
    NULL,                               /* pEndPage */
    NULL,                               /* pEndPath */
    NULL,                               /* pEnumFonts */
    NULL,                               /* pEnumICMProfiles */
    NULL,                               /* pExcludeClipRect */
    NULL,                               /* pExtDeviceMode */
    NULL,                               /* pExtEscape */
    NULL,                               /* pExtFloodFill */
    NULL,                               /* pExtSelectClipRgn */
    dibdrv_ExtTextOut,                  /* pExtTextOut */
    NULL,                               /* pFillPath */
    NULL,                               /* pFillRgn */
    NULL,                               /* pFlattenPath */
    NULL,                               /* pFontIsLinked */
    NULL,                               /* pFrameRgn */
    NULL,                               /* pGdiComment */
    NULL,                               /* pGdiRealizationInfo */
    NULL,                               /* pGetCharABCWidths */
    NULL,                               /* pGetCharABCWidthsI */
    NULL,                               /* pGetCharWidth */
    NULL,                               /* pGetDeviceCaps */
    NULL,                               /* pGetDeviceGammaRamp */
    NULL,                               /* pGetFontData */
    NULL,                               /* pGetFontUnicodeRanges */
    NULL,                               /* pGetGlyphIndices */
    NULL,                               /* pGetGlyphOutline */
    NULL,                               /* pGetICMProfile */
    dibdrv_GetImage,                    /* pGetImage */
    NULL,                               /* pGetKerningPairs */
    NULL,                               /* pGetNearestColor */
    NULL,                               /* pGetOutlineTextMetrics */
    dibdrv_GetPixel,                    /* pGetPixel */
    NULL,                               /* pGetPixelFormat */
    NULL,                               /* pGetSystemPaletteEntries */
    NULL,                               /* pGetTextCharsetInfo */
    NULL,                               /* pGetTextExtentExPoint */
    NULL,                               /* pGetTextExtentExPointI */
    NULL,                               /* pGetTextFace */
    NULL,                               /* pGetTextMetrics */
    dibdrv_GradientFill,                /* pGradientFill */
    NULL,                               /* pIntersectClipRect */
    NULL,                               /* pInvertRgn */
    dibdrv_LineTo,                      /* pLineTo */
    NULL,                               /* pModifyWorldTransform */
    NULL,                               /* pMoveTo */
    NULL,                               /* pOffsetClipRgn */
    NULL,                               /* pOffsetViewportOrg */
    NULL,                               /* pOffsetWindowOrg */
    dibdrv_PaintRgn,                    /* pPaintRgn */
    dibdrv_PatBlt,                      /* pPatBlt */
    NULL,                               /* pPie */
    NULL,                               /* pPolyBezier */
    NULL,                               /* pPolyBezierTo */
    NULL,                               /* pPolyDraw */
    NULL,                               /* pPolyPolygon */
    dibdrv_PolyPolyline,                /* pPolyPolyline */
    NULL,                               /* pPolygon */
    dibdrv_Polyline,                    /* pPolyline */
    NULL,                               /* pPolylineTo */
    dibdrv_PutImage,                    /* pPutImage */
    NULL,                               /* pRealizeDefaultPalette */
    NULL,                               /* pRealizePalette */
    dibdrv_Rectangle,                   /* pRectangle */
    NULL,                               /* pResetDC */
    NULL,                               /* pRestoreDC */
    NULL,                               /* pRoundRect */
    NULL,                               /* pSaveDC */
    NULL,                               /* pScaleViewportExt */
    NULL,                               /* pScaleWindowExt */
    dibdrv_SelectBitmap,                /* pSelectBitmap */
    dibdrv_SelectBrush,                 /* pSelectBrush */
    NULL,                               /* pSelectClipPath */
    NULL,                               /* pSelectFont */
    NULL,                               /* pSelectPalette */
    dibdrv_SelectPen,                   /* pSelectPen */
    NULL,                               /* pSetArcDirection */
    dibdrv_SetBkColor,                  /* pSetBkColor */
    dibdrv_SetBkMode,                   /* pSetBkMode */
    dibdrv_SetDCBrushColor,             /* pSetDCBrushColor */
    dibdrv_SetDCPenColor,               /* pSetDCPenColor */
    dibdrv_SetDIBColorTable,            /* pSetDIBColorTable */
    NULL,                               /* pSetDIBitsToDevice */
    dibdrv_SetDeviceClipping,           /* pSetDeviceClipping */
    NULL,                               /* pSetDeviceGammaRamp */
    NULL,                               /* pSetLayout */
    NULL,                               /* pSetMapMode */
    NULL,                               /* pSetMapperFlags */
    dibdrv_SetPixel,                    /* pSetPixel */
    NULL,                               /* pSetPixelFormat */
    NULL,                               /* pSetPolyFillMode */
    dibdrv_SetROP2,                     /* pSetROP2 */
    NULL,                               /* pSetRelAbs */
    NULL,                               /* pSetStretchBltMode */
    NULL,                               /* pSetTextAlign */
    NULL,                               /* pSetTextCharacterExtra */
    dibdrv_SetTextColor,                /* pSetTextColor */
    NULL,                               /* pSetTextJustification */
    NULL,                               /* pSetViewportExt */
    NULL,                               /* pSetViewportOrg */
    NULL,                               /* pSetWindowExt */
    NULL,                               /* pSetWindowOrg */
    NULL,                               /* pSetWorldTransform */
    NULL,                               /* pStartDoc */
    NULL,                               /* pStartPage */
    dibdrv_StretchBlt,                  /* pStretchBlt */
    NULL,                               /* pStretchDIBits */
    NULL,                               /* pStrokeAndFillPath */
    NULL,                               /* pStrokePath */
    NULL,                               /* pSwapBuffers */
    NULL,                               /* pUnrealizePalette */
    NULL,                               /* pWidenPath */
    NULL,                               /* pwglCopyContext */
    NULL,                               /* pwglCreateContext */
    NULL,                               /* pwglCreateContextAttribsARB */
    NULL,                               /* pwglDeleteContext */
    NULL,                               /* pwglGetPbufferDCARB */
    NULL,                               /* pwglGetProcAddress */
    NULL,                               /* pwglMakeContextCurrentARB */
    NULL,                               /* pwglMakeCurrent */
    NULL,                               /* pwglSetPixelFormatWINE */
    NULL,                               /* pwglShareLists */
    NULL,                               /* pwglUseFontBitmapsA */
    NULL                                /* pwglUseFontBitmapsW */
};
