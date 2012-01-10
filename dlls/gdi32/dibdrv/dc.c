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
    if (!is_bitmapobj_dib( bmp ))
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
    else init_dib_info( dib, &bmp->dib.dsBmih, bmp->dib.dsBitfields,
                        bmp->color_table, bmp->dib.dsBm.bmBits, flags );
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

int get_clipped_rects( const dib_info *dib, const RECT *rc, HRGN clip, struct clipped_rects *clip_rects )
{
    const WINEREGION *region;
    RECT rect, *out = clip_rects->buffer;
    int i;

    init_clipped_rects( clip_rects );

    rect.left   = 0;
    rect.top    = 0;
    rect.right  = dib->width;
    rect.bottom = dib->height;
    if (rc && !intersect_rect( &rect, &rect, rc )) return 0;

    if (!clip)
    {
        *out = rect;
        clip_rects->count = 1;
        return 1;
    }

    if (!(region = get_wine_region( clip ))) return 0;

    for (i = 0; i < region->numRects; i++)
    {
        if (region->rects[i].top >= rect.bottom) break;
        if (!intersect_rect( out, &rect, &region->rects[i] )) continue;
        out++;
        if (out == &clip_rects->buffer[sizeof(clip_rects->buffer) / sizeof(RECT)])
        {
            clip_rects->rects = HeapAlloc( GetProcessHeap(), 0, region->numRects * sizeof(RECT) );
            if (!clip_rects->rects) return 0;
            memcpy( clip_rects->rects, clip_rects->buffer, (out - clip_rects->buffer) * sizeof(RECT) );
            out = clip_rects->rects + (out - clip_rects->buffer);
        }
    }
    release_wine_region( clip );
    clip_rects->count = out - clip_rects->rects;
    return clip_rects->count;
}

/**********************************************************************
 *	     dibdrv_CreateDC
 */
static BOOL dibdrv_CreateDC( PHYSDEV *dev, LPCWSTR driver, LPCWSTR device,
                             LPCWSTR output, const DEVMODEW *data )
{
    dibdrv_physdev *pdev = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pdev) );

    if (!pdev) return FALSE;
    clear_dib_info(&pdev->dib);
    clear_dib_info(&pdev->brush.dib);
    clear_dib_info(&pdev->pen_brush.dib);
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
    free_pattern_brush( &pdev->brush );
    free_pattern_brush( &pdev->pen_brush );
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
    GDI_ReleaseObj( bitmap );

    return bitmap;
}

/***********************************************************************
 *           dibdrv_SetDeviceClipping
 */
static void dibdrv_SetDeviceClipping( PHYSDEV dev, HRGN rgn )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    TRACE("(%p, %p)\n", dev, rgn);

    pdev->clip = rgn;
}

/***********************************************************************
 *           dibdrv_ChoosePixelFormat
 */
static INT dibdrv_ChoosePixelFormat( PHYSDEV dev, const PIXELFORMATDESCRIPTOR *pfd )
{
    FIXME( "Not supported on DIB section\n" );
    return 0;
}

/***********************************************************************
 *           dibdrv_DescribePixelFormat
 */
static INT dibdrv_DescribePixelFormat( PHYSDEV dev, INT fmt, UINT size, PIXELFORMATDESCRIPTOR *pfd )
{
    FIXME( "Not supported on DIB section\n" );
    return 0;
}

/***********************************************************************
 *           dibdrv_ExtEscape
 */
static INT dibdrv_ExtEscape( PHYSDEV dev, INT escape, INT in_size, const void *in_data,
                             INT out_size, void *out_data )
{
    return 0;
}

/***********************************************************************
 *           dibdrv_GetDeviceGammaRamp
 */
static BOOL dibdrv_GetDeviceGammaRamp( PHYSDEV dev, void *ramp )
{
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}

/***********************************************************************
 *           dibdrv_GetPixelFormat
 */
static INT dibdrv_GetPixelFormat( PHYSDEV dev )
{
    FIXME( "Not supported on DIB section\n" );
    return 0;
}

/***********************************************************************
 *           dibdrv_SetDeviceGammaRamp
 */
static BOOL dibdrv_SetDeviceGammaRamp( PHYSDEV dev, void *ramp )
{
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}

/***********************************************************************
 *           dibdrv_SetPixelFormat
 */
static BOOL dibdrv_SetPixelFormat( PHYSDEV dev, INT fmt, const PIXELFORMATDESCRIPTOR *pfd )
{
    FIXME( "Not supported on DIB section\n" );
    return FALSE;
}

/***********************************************************************
 *           dibdrv_SwapBuffers
 */
static BOOL dibdrv_SwapBuffers( PHYSDEV dev )
{
    FIXME( "Not supported on DIB section\n" );
    return FALSE;
}

/***********************************************************************
 *           dibdrv_wglCopyContext
 */
static BOOL dibdrv_wglCopyContext( HGLRC src, HGLRC dst, UINT mask )
{
    FIXME( "Not supported on DIB section\n" );
    return FALSE;
}

/***********************************************************************
 *           dibdrv_wglCreateContext
 */
static HGLRC dibdrv_wglCreateContext( PHYSDEV dev )
{
    FIXME( "Not supported on DIB section\n" );
    return 0;
}

/***********************************************************************
 *           dibdrv_wglCreateContextAttribsARB
 */
static HGLRC dibdrv_wglCreateContextAttribsARB( PHYSDEV dev, HGLRC ctx, const int *attribs )
{
    FIXME( "Not supported on DIB section\n" );
    return 0;
}

/***********************************************************************
 *           dibdrv_wglDeleteContext
 */
static BOOL dibdrv_wglDeleteContext( HGLRC ctx )
{
    FIXME( "Not supported on DIB section\n" );
    return FALSE;
}

/***********************************************************************
 *           dibdrv_wglGetPbufferDCARB
 */
static HDC dibdrv_wglGetPbufferDCARB( PHYSDEV dev, void *buffer )
{
    FIXME( "Not supported on DIB section\n" );
    return 0;
}

/***********************************************************************
 *           dibdrv_wglGetProcAddress
 */
static PROC dibdrv_wglGetProcAddress( LPCSTR name )
{
    FIXME( "Not supported on DIB section\n" );
    return NULL;
}

/***********************************************************************
 *           dibdrv_wglMakeContextCurrentARB
 */
static BOOL dibdrv_wglMakeContextCurrentARB( PHYSDEV draw_dev, PHYSDEV read_dev, HGLRC ctx )
{
    FIXME( "Not supported on DIB section\n" );
    return FALSE;
}

/***********************************************************************
 *           dibdrv_wglMakeCurrent
 */
static BOOL dibdrv_wglMakeCurrent( PHYSDEV dev, HGLRC ctx )
{
    FIXME( "Not supported on DIB section\n" );
    return FALSE;
}

/***********************************************************************
 *           dibdrv_wglSetPixelFormatWINE
 */
static BOOL dibdrv_wglSetPixelFormatWINE( PHYSDEV dev, INT fmt, const PIXELFORMATDESCRIPTOR *pfd )
{
    FIXME( "Not supported on DIB section\n" );
    return FALSE;
}

/***********************************************************************
 *           dibdrv_wglShareLists
 */
static BOOL dibdrv_wglShareLists( HGLRC ctx1, HGLRC ctx2 )
{
    FIXME( "Not supported on DIB section\n" );
    return FALSE;
}

/***********************************************************************
 *           dibdrv_wglUseFontBitmapsA
 */
static BOOL dibdrv_wglUseFontBitmapsA( PHYSDEV dev, DWORD first, DWORD count, DWORD base )
{
    FIXME( "Not supported on DIB section\n" );
    return FALSE;
}

/***********************************************************************
 *           dibdrv_wglUseFontBitmapsW
 */
static BOOL dibdrv_wglUseFontBitmapsW( PHYSDEV dev, DWORD first, DWORD count, DWORD base )
{
    FIXME( "Not supported on DIB section\n" );
    return FALSE;
}

const struct gdi_dc_funcs dib_driver =
{
    NULL,                               /* pAbortDoc */
    NULL,                               /* pAbortPath */
    dibdrv_AlphaBlend,                  /* pAlphaBlend */
    NULL,                               /* pAngleArc */
    dibdrv_Arc,                         /* pArc */
    dibdrv_ArcTo,                       /* pArcTo */
    NULL,                               /* pBeginPath */
    dibdrv_BlendImage,                  /* pBlendImage */
    dibdrv_ChoosePixelFormat,           /* pChoosePixelFormat */
    dibdrv_Chord,                       /* pChord */
    NULL,                               /* pCloseFigure */
    dibdrv_CopyBitmap,                  /* pCopyBitmap */
    NULL,                               /* pCreateBitmap */
    NULL,                               /* pCreateCompatibleDC */
    dibdrv_CreateDC,                    /* pCreateDC */
    NULL,                               /* pCreateDIBSection */
    dibdrv_DeleteBitmap,                /* pDeleteBitmap */
    dibdrv_DeleteDC,                    /* pDeleteDC */
    NULL,                               /* pDeleteObject */
    dibdrv_DescribePixelFormat,         /* pDescribePixelFormat */
    NULL,                               /* pDeviceCapabilities */
    dibdrv_Ellipse,                     /* pEllipse */
    NULL,                               /* pEndDoc */
    NULL,                               /* pEndPage */
    NULL,                               /* pEndPath */
    NULL,                               /* pEnumFonts */
    NULL,                               /* pEnumICMProfiles */
    NULL,                               /* pExcludeClipRect */
    NULL,                               /* pExtDeviceMode */
    dibdrv_ExtEscape,                   /* pExtEscape */
    dibdrv_ExtFloodFill,                /* pExtFloodFill */
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
    dibdrv_GetDeviceGammaRamp,          /* pGetDeviceGammaRamp */
    NULL,                               /* pGetFontData */
    NULL,                               /* pGetFontUnicodeRanges */
    NULL,                               /* pGetGlyphIndices */
    NULL,                               /* pGetGlyphOutline */
    NULL,                               /* pGetICMProfile */
    dibdrv_GetImage,                    /* pGetImage */
    NULL,                               /* pGetKerningPairs */
    dibdrv_GetNearestColor,             /* pGetNearestColor */
    NULL,                               /* pGetOutlineTextMetrics */
    dibdrv_GetPixel,                    /* pGetPixel */
    dibdrv_GetPixelFormat,              /* pGetPixelFormat */
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
    dibdrv_Pie,                         /* pPie */
    NULL,                               /* pPolyBezier */
    NULL,                               /* pPolyBezierTo */
    NULL,                               /* pPolyDraw */
    dibdrv_PolyPolygon,                 /* pPolyPolygon */
    dibdrv_PolyPolyline,                /* pPolyPolyline */
    dibdrv_Polygon,                     /* pPolygon */
    dibdrv_Polyline,                    /* pPolyline */
    NULL,                               /* pPolylineTo */
    dibdrv_PutImage,                    /* pPutImage */
    NULL,                               /* pRealizeDefaultPalette */
    NULL,                               /* pRealizePalette */
    dibdrv_Rectangle,                   /* pRectangle */
    NULL,                               /* pResetDC */
    NULL,                               /* pRestoreDC */
    dibdrv_RoundRect,                   /* pRoundRect */
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
    NULL,                               /* pSetBkColor */
    NULL,                               /* pSetBkMode */
    dibdrv_SetDCBrushColor,             /* pSetDCBrushColor */
    dibdrv_SetDCPenColor,               /* pSetDCPenColor */
    NULL,                               /* pSetDIBitsToDevice */
    dibdrv_SetDeviceClipping,           /* pSetDeviceClipping */
    dibdrv_SetDeviceGammaRamp,          /* pSetDeviceGammaRamp */
    NULL,                               /* pSetLayout */
    NULL,                               /* pSetMapMode */
    NULL,                               /* pSetMapperFlags */
    dibdrv_SetPixel,                    /* pSetPixel */
    dibdrv_SetPixelFormat,              /* pSetPixelFormat */
    NULL,                               /* pSetPolyFillMode */
    NULL,                               /* pSetROP2 */
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
    dibdrv_StretchBlt,                  /* pStretchBlt */
    NULL,                               /* pStretchDIBits */
    NULL,                               /* pStrokeAndFillPath */
    NULL,                               /* pStrokePath */
    dibdrv_SwapBuffers,                 /* pSwapBuffers */
    NULL,                               /* pUnrealizePalette */
    NULL,                               /* pWidenPath */
    dibdrv_wglCopyContext,              /* pwglCopyContext */
    dibdrv_wglCreateContext,            /* pwglCreateContext */
    dibdrv_wglCreateContextAttribsARB,  /* pwglCreateContextAttribsARB */
    dibdrv_wglDeleteContext,            /* pwglDeleteContext */
    dibdrv_wglGetPbufferDCARB,          /* pwglGetPbufferDCARB */
    dibdrv_wglGetProcAddress,           /* pwglGetProcAddress */
    dibdrv_wglMakeContextCurrentARB,    /* pwglMakeContextCurrentARB */
    dibdrv_wglMakeCurrent,              /* pwglMakeCurrent */
    dibdrv_wglSetPixelFormatWINE,       /* pwglSetPixelFormatWINE */
    dibdrv_wglShareLists,               /* pwglShareLists */
    dibdrv_wglUseFontBitmapsA,          /* pwglUseFontBitmapsA */
    dibdrv_wglUseFontBitmapsW,          /* pwglUseFontBitmapsW */
};
