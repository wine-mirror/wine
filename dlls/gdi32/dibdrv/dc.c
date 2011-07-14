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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dib);

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

static BOOL init_dib_info(dib_info *dib, const BITMAPINFOHEADER *bi, const DWORD *bit_fields,
                          RGBQUAD *color_table, int color_table_size, void *bits, enum dib_info_flags flags)
{
    static const DWORD bit_fields_888[3] = {0xff0000, 0x00ff00, 0x0000ff};
    static const DWORD bit_fields_555[3] = {0x7c00, 0x03e0, 0x001f};

    dib->bit_count = bi->biBitCount;
    dib->width     = bi->biWidth;
    dib->height    = bi->biHeight;
    dib->stride    = ((dib->width * dib->bit_count + 31) >> 3) & ~3;
    dib->bits      = bits;
    dib->ptr_to_free = NULL;
    dib->flags     = flags;

    if(dib->height < 0) /* top-down */
    {
        dib->height = -dib->height;
    }
    else /* bottom-up */
    {
        /* bits always points to the top-left corner and the stride is -ve */
        dib->bits    = (BYTE*)dib->bits + (dib->height - 1) * dib->stride;
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

    default:
        TRACE("bpp %d not supported, will forward to graphics driver.\n", dib->bit_count);
        return FALSE;
    }

    if(color_table)
    {
        if (flags & private_color_table)
        {
            dib->color_table = HeapAlloc(GetProcessHeap(), 0, color_table_size * sizeof(dib->color_table[0]));
            if(!dib->color_table) return FALSE;
            memcpy(dib->color_table, color_table, color_table_size * sizeof(color_table[0]));
        }
        else
            dib->color_table = color_table;
        dib->color_table_size = color_table_size;
    }
    else
    {
        dib->color_table = NULL;
        dib->color_table_size = 0;
    }

    return TRUE;
}

BOOL init_dib_info_from_packed(dib_info *dib, const BITMAPINFOHEADER *bi, WORD usage, HPALETTE palette)
{
    DWORD *masks = NULL;
    RGBQUAD *color_table = NULL, pal_table[256];
    BYTE *ptr = (BYTE*)bi + bi->biSize;
    int num_colors = bi->biClrUsed;

    if(bi->biCompression == BI_BITFIELDS)
    {
        masks = (DWORD *)ptr;
        ptr += 3 * sizeof(DWORD);
    }

    if(!num_colors && bi->biBitCount <= 8) num_colors = 1 << bi->biBitCount;
    if(num_colors)
    {
        if(usage == DIB_PAL_COLORS)
        {
            PALETTEENTRY entries[256];
            const WORD *index = (const WORD*) ptr;
            UINT i, count = GetPaletteEntries( palette, 0, num_colors, entries );
            for (i = 0; i < num_colors; i++, index++)
            {
                PALETTEENTRY *entry = &entries[*index % count];
                pal_table[i].rgbRed      = entry->peRed;
                pal_table[i].rgbGreen    = entry->peGreen;
                pal_table[i].rgbBlue     = entry->peBlue;
                pal_table[i].rgbReserved = 0;
            }
            color_table = pal_table;
            ptr += num_colors * sizeof(WORD);
        }
        else
        {
            color_table = (RGBQUAD*)ptr;
            ptr += num_colors * sizeof(*color_table);
        }
    }

    return init_dib_info(dib, bi, masks, color_table, num_colors, ptr, private_color_table);
}

static void clear_dib_info(dib_info *dib)
{
    dib->color_table = NULL;
    dib->bits = NULL;
    dib->ptr_to_free = NULL;
}

/**********************************************************************
 *      free_dib_info
 *
 * Free the resources associated with a dib and optionally the bits
 */
void free_dib_info(dib_info *dib)
{
    if (dib->flags & private_color_table)
        HeapFree(GetProcessHeap(), 0, dib->color_table);
    dib->color_table = NULL;

    HeapFree(GetProcessHeap(), 0, dib->ptr_to_free);
    dib->ptr_to_free = NULL;
    dib->bits = NULL;
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
    dst->color_table      = NULL;
    dst->flags            = src->flags;
    if(dst->color_table_size)
    {
        int size = dst->color_table_size * sizeof(dst->color_table[0]);
        if (dst->flags & private_color_table)
        {
            dst->color_table = HeapAlloc(GetProcessHeap(), 0, size);
            memcpy(dst->color_table, src->color_table, size);
        }
        else
            dst->color_table = src->color_table;
    }
}

/**************************************************************
 *            convert_dib
 *
 * Converts src into the format specified in dst.
 *
 * FIXME: At the moment this always creates a top-down dib,
 * do we want to give the option of bottom-up?
 */
BOOL convert_dib(dib_info *dst, const dib_info *src)
{
    BOOL ret;
    RECT src_rect;

    dst->height = src->height;
    dst->width = src->width;
    dst->stride = ((dst->width * dst->bit_count + 31) >> 3) & ~3;
    dst->ptr_to_free = dst->bits = HeapAlloc(GetProcessHeap(), 0, dst->height * dst->stride);

    src_rect.left = src_rect.top = 0;
    src_rect.right = src->width;
    src_rect.bottom = src->height;

    ret = dst->funcs->convert_to(dst, src, &src_rect);

    if(!ret) free_dib_info(dst);
    return ret;
}

static void update_fg_colors( dibdrv_physdev *pdev )
{
    pdev->pen_color   = get_fg_color( pdev, pdev->pen_colorref );
    pdev->brush_color = get_fg_color( pdev, pdev->brush_colorref );
}

static void update_masks( dibdrv_physdev *pdev, INT rop )
{
    calc_and_xor_masks( rop, pdev->pen_color, &pdev->pen_and, &pdev->pen_xor );
    update_brush_rop( pdev, rop );
    if( GetBkMode( pdev->dev.hdc ) == OPAQUE )
        calc_and_xor_masks( rop, pdev->bkgnd_color, &pdev->bkgnd_and, &pdev->bkgnd_xor );
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
    free_dib_info(&pdev->dib);
    return 0;
}

/***********************************************************************
 *           dibdrv_SelectBitmap
 */
static HBITMAP dibdrv_SelectBitmap( PHYSDEV dev, HBITMAP bitmap )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSelectBitmap );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    BITMAPOBJ *bmp = GDI_GetObjPtr( bitmap, OBJ_BITMAP );
    TRACE("(%p, %p)\n", dev, bitmap);

    if (!bmp) return 0;
    assert(bmp->dib);

    pdev->clip = CreateRectRgn(0, 0, 0, 0);
    pdev->defer = 0;

    clear_dib_info(&pdev->dib);
    clear_dib_info(&pdev->brush_dib);
    pdev->brush_and_bits = pdev->brush_xor_bits = NULL;

    if(!init_dib_info(&pdev->dib, &bmp->dib->dsBmih, bmp->dib->dsBitfields,
                      bmp->color_table, bmp->nb_colors, bmp->dib->dsBm.bmBits, private_color_table))
        pdev->defer |= DEFER_FORMAT;

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

    pdev->bkgnd_color = pdev->dib.funcs->colorref_to_pixel( &pdev->dib, color );

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
static void dibdrv_SetDeviceClipping( PHYSDEV dev, HRGN vis_rgn, HRGN clip_rgn )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetDeviceClipping );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    TRACE("(%p, %p, %p)\n", dev, vis_rgn, clip_rgn);

    CombineRgn( pdev->clip, vis_rgn, clip_rgn, clip_rgn ? RGN_AND : RGN_COPY );
    return next->funcs->pSetDeviceClipping( next, vis_rgn, clip_rgn);
}

/***********************************************************************
 *           dibdrv_SetDIBColorTable
 */
static UINT dibdrv_SetDIBColorTable( PHYSDEV dev, UINT pos, UINT count, const RGBQUAD *colors )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetDIBColorTable );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    TRACE("(%p, %d, %d, %p)\n", dev, pos, count, colors);

    if( pdev->dib.color_table && pos < pdev->dib.color_table_size )
    {
        if( pos + count > pdev->dib.color_table_size ) count = pdev->dib.color_table_size - pos;
        memcpy( pdev->dib.color_table + pos, colors, count * sizeof(RGBQUAD) );

        pdev->bkgnd_color = pdev->dib.funcs->colorref_to_pixel( &pdev->dib, GetBkColor( dev->hdc ) );
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

const DC_FUNCTIONS dib_driver =
{
    NULL,                               /* pAbortDoc */
    NULL,                               /* pAbortPath */
    NULL,                               /* pAlphaBlend */
    NULL,                               /* pAngleArc */
    NULL,                               /* pArc */
    NULL,                               /* pArcTo */
    NULL,                               /* pBeginPath */
    NULL,                               /* pChoosePixelFormat */
    NULL,                               /* pChord */
    NULL,                               /* pCloseFigure */
    NULL,                               /* pCreateBitmap */
    NULL,                               /* pCreateDC */
    NULL,                               /* pCreateDIBSection */
    NULL,                               /* pDeleteBitmap */
    dibdrv_DeleteDC,                    /* pDeleteDC */
    NULL,                               /* pDeleteObject */
    NULL,                               /* pDescribePixelFormat */
    NULL,                               /* pDeviceCapabilities */
    NULL,                               /* pEllipse */
    NULL,                               /* pEndDoc */
    NULL,                               /* pEndPage */
    NULL,                               /* pEndPath */
    NULL,                               /* pEnumDeviceFonts */
    NULL,                               /* pEnumICMProfiles */
    NULL,                               /* pExcludeClipRect */
    NULL,                               /* pExtDeviceMode */
    NULL,                               /* pExtEscape */
    NULL,                               /* pExtFloodFill */
    NULL,                               /* pExtSelectClipRgn */
    NULL,                               /* pExtTextOut */
    NULL,                               /* pFillPath */
    NULL,                               /* pFillRgn */
    NULL,                               /* pFlattenPath */
    NULL,                               /* pFrameRgn */
    NULL,                               /* pGdiComment */
    NULL,                               /* pGetBitmapBits */
    NULL,                               /* pGetCharWidth */
    NULL,                               /* pGetDIBits */
    NULL,                               /* pGetDeviceCaps */
    NULL,                               /* pGetDeviceGammaRamp */
    NULL,                               /* pGetICMProfile */
    NULL,                               /* pGetImage */
    NULL,                               /* pGetNearestColor */
    NULL,                               /* pGetPixel */
    NULL,                               /* pGetPixelFormat */
    NULL,                               /* pGetSystemPaletteEntries */
    NULL,                               /* pGetTextExtentExPoint */
    NULL,                               /* pGetTextMetrics */
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
    NULL,                               /* pPolyPolyline */
    NULL,                               /* pPolygon */
    NULL,                               /* pPolyline */
    NULL,                               /* pPolylineTo */
    NULL,                               /* pPutImage */
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
    NULL,                               /* pSetBitmapBits */
    dibdrv_SetBkColor,                  /* pSetBkColor */
    dibdrv_SetBkMode,                   /* pSetBkMode */
    dibdrv_SetDCBrushColor,             /* pSetDCBrushColor */
    dibdrv_SetDCPenColor,               /* pSetDCPenColor */
    dibdrv_SetDIBColorTable,            /* pSetDIBColorTable */
    NULL,                               /* pSetDIBits */
    NULL,                               /* pSetDIBitsToDevice */
    dibdrv_SetDeviceClipping,           /* pSetDeviceClipping */
    NULL,                               /* pSetDeviceGammaRamp */
    NULL,                               /* pSetLayout */
    NULL,                               /* pSetMapMode */
    NULL,                               /* pSetMapperFlags */
    NULL,                               /* pSetPixel */
    NULL,                               /* pSetPixelFormat */
    NULL,                               /* pSetPolyFillMode */
    dibdrv_SetROP2,                     /* pSetROP2 */
    NULL,                               /* pSetRelAbs */
    NULL,                               /* pSetStretchBltMode */
    NULL,                               /* pSetTextAlign */
    NULL,                               /* pSetTextCharacterExtra */
    NULL,                               /* pSetTextColor */
    NULL,                               /* pSetTextJustification */
    NULL,                               /* pSetViewportExt */
    NULL,                               /* pSetViewportOrg */
    NULL,                               /* pSetWindowExt */
    NULL,                               /* pSetWindowOrg */
    NULL,                               /* pSetWorldTransform */
    NULL,                               /* pStartDoc */
    NULL,                               /* pStartPage */
    NULL,                               /* pStretchBlt */
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
