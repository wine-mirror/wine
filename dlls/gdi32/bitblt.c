/*
 * GDI bit-blit operations
 *
 * Copyright 1993, 1994  Alexandre Julliard
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

#include "config.h"

#include <stdarg.h>

#include <math.h>
#ifdef HAVE_FLOAT_H
#include <float.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitblt);

static inline BOOL rop_uses_src( DWORD rop )
{
    return ((rop >> 2) & 0x330000) != (rop & 0x330000);
}

static inline void swap_ints( int *i, int *j )
{
    int tmp = *i;
    *i = *j;
    *j = tmp;
}

static void get_vis_rectangles( DC *dc_dst, struct bitblt_coords *dst,
                                DC *dc_src, struct bitblt_coords *src )
{
    RECT rect, clip;

    /* get the destination visible rectangle */

    rect.left   = dst->log_x;
    rect.top    = dst->log_y;
    rect.right  = dst->log_x + dst->log_width;
    rect.bottom = dst->log_y + dst->log_height;
    LPtoDP( dc_dst->hSelf, (POINT *)&rect, 2 );
    dst->x      = rect.left;
    dst->y      = rect.top;
    dst->width  = rect.right - rect.left;
    dst->height = rect.bottom - rect.top;
    if (dst->layout & LAYOUT_RTL && dst->layout & LAYOUT_BITMAPORIENTATIONPRESERVED)
    {
        swap_ints( &rect.left, &rect.right );
        dst->x = rect.left;
        dst->width = rect.right - rect.left;
    }
    if (rect.left > rect.right) { swap_ints( &rect.left, &rect.right ); rect.left++; rect.right++; }
    if (rect.top > rect.bottom) { swap_ints( &rect.top, &rect.bottom ); rect.top++; rect.bottom++; }

    if (get_clip_box( dc_dst, &clip ))
        intersect_rect( &dst->visrect, &rect, &clip );
    else
        dst->visrect = rect;

    /* get the source visible rectangle */

    if (!src) return;

    rect.left   = src->log_x;
    rect.top    = src->log_y;
    rect.right  = src->log_x + src->log_width;
    rect.bottom = src->log_y + src->log_height;
    LPtoDP( dc_src->hSelf, (POINT *)&rect, 2 );
    src->x      = rect.left;
    src->y      = rect.top;
    src->width  = rect.right - rect.left;
    src->height = rect.bottom - rect.top;
    if (src->layout & LAYOUT_RTL && src->layout & LAYOUT_BITMAPORIENTATIONPRESERVED)
    {
        swap_ints( &rect.left, &rect.right );
        src->x = rect.left;
        src->width = rect.right - rect.left;
    }
    if (rect.left > rect.right) { swap_ints( &rect.left, &rect.right ); rect.left++; rect.right++; }
    if (rect.top > rect.bottom) { swap_ints( &rect.top, &rect.bottom ); rect.top++; rect.bottom++; }

    /* source is not clipped */
    if (dc_src->header.type == OBJ_MEMDC)
        intersect_rect( &src->visrect, &rect, &dc_src->vis_rect );
    else
        src->visrect = rect;  /* FIXME: clip to device size */

    /* intersect the rectangles */

    if ((src->width == dst->width) && (src->height == dst->height)) /* no stretching */
    {
        offset_rect( &src->visrect, dst->x - src->x, dst->y - src->y );
        intersect_rect( &rect, &src->visrect, &dst->visrect );
        src->visrect = dst->visrect = rect;
        offset_rect( &src->visrect, src->x - dst->x, src->y - dst->y );
    }
    else  /* stretching */
    {
        /* map source rectangle into destination coordinates */
        rect.left   = dst->x + (src->visrect.left - src->x)*dst->width/src->width;
        rect.top    = dst->y + (src->visrect.top - src->y)*dst->height/src->height;
        rect.right  = dst->x + (src->visrect.right - src->x)*dst->width/src->width;
        rect.bottom = dst->y + (src->visrect.bottom - src->y)*dst->height/src->height;
        if (rect.left > rect.right) swap_ints( &rect.left, &rect.right );
        if (rect.top > rect.bottom) swap_ints( &rect.top, &rect.bottom );

        /* avoid rounding errors */
        rect.left--;
        rect.top--;
        rect.right++;
        rect.bottom++;
        if (!intersect_rect( &dst->visrect, &rect, &dst->visrect )) return;

        /* map destination rectangle back to source coordinates */
        rect = dst->visrect;
        rect.left   = src->x + (dst->visrect.left - dst->x)*src->width/dst->width;
        rect.top    = src->y + (dst->visrect.top - dst->y)*src->height/dst->height;
        rect.right  = src->x + (dst->visrect.right - dst->x)*src->width/dst->width;
        rect.bottom = src->y + (dst->visrect.bottom - dst->y)*src->height/dst->height;
        if (rect.left > rect.right) swap_ints( &rect.left, &rect.right );
        if (rect.top > rect.bottom) swap_ints( &rect.top, &rect.bottom );

        /* avoid rounding errors */
        rect.left--;
        rect.top--;
        rect.right++;
        rect.bottom++;
        intersect_rect( &src->visrect, &rect, &src->visrect );
    }
}

static void free_heap_bits( struct gdi_image_bits *bits )
{
    HeapFree( GetProcessHeap(), 0, bits->ptr );
}

/* nulldrv fallback implementation using StretchDIBits */
BOOL nulldrv_StretchBlt( PHYSDEV dst_dev, struct bitblt_coords *dst,
                         PHYSDEV src_dev, struct bitblt_coords *src, DWORD rop )
{
    DC *dc_src, *dc_dst = get_nulldrv_dc( dst_dev );
    char src_buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    char dst_buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *src_info = (BITMAPINFO *)src_buffer;
    BITMAPINFO *dst_info = (BITMAPINFO *)dst_buffer;
    DWORD err;
    struct gdi_image_bits src_bits, dst_bits;
    BITMAP bm;
    HBITMAP hbm;
    LPVOID bits;
    INT lines;

    if (dst->visrect.left >= dst->visrect.right || dst->visrect.top >= dst->visrect.bottom) return TRUE;

    /* make sure we have a real implementation for StretchDIBits */
    if (GET_DC_PHYSDEV( dc_dst, pStretchDIBits ) == dst_dev) goto try_get_image;

    if (GetObjectType( src_dev->hdc ) != OBJ_MEMDC) return FALSE;
    if (!GetObjectW( GetCurrentObject( src_dev->hdc, OBJ_BITMAP ), sizeof(bm), &bm )) return FALSE;

    src_info->bmiHeader.biSize = sizeof(src_info->bmiHeader);
    src_info->bmiHeader.biWidth = bm.bmWidth;
    src_info->bmiHeader.biHeight = bm.bmHeight;
    src_info->bmiHeader.biPlanes = 1;
    src_info->bmiHeader.biBitCount = 32;
    src_info->bmiHeader.biCompression = BI_RGB;
    src_info->bmiHeader.biSizeImage = 0;
    src_info->bmiHeader.biXPelsPerMeter = 0;
    src_info->bmiHeader.biYPelsPerMeter = 0;
    src_info->bmiHeader.biClrUsed = 0;
    src_info->bmiHeader.biClrImportant = 0;

    if (!(bits = HeapAlloc(GetProcessHeap(), 0, bm.bmHeight * bm.bmWidth * 4)))
        return FALSE;

    /* Select out the src bitmap before calling GetDIBits */
    hbm = SelectObject( src_dev->hdc, GetStockObject(DEFAULT_BITMAP) );
    lines = GetDIBits( src_dev->hdc, hbm, 0, bm.bmHeight, bits, src_info, DIB_RGB_COLORS );
    SelectObject( src_dev->hdc, hbm );

    if (lines) lines = StretchDIBits( dst_dev->hdc, dst->log_x, dst->log_y, dst->log_width, dst->log_height,
                                      src->x, bm.bmHeight - src->height - src->y, src->width, src->height,
                                      bits, src_info, DIB_RGB_COLORS, rop );
    HeapFree( GetProcessHeap(), 0, bits );
    return (lines == src->height);

try_get_image:

    if (!(dc_src = get_dc_ptr( src_dev->hdc ))) return FALSE;
    src_dev = GET_DC_PHYSDEV( dc_src, pGetImage );
    err = src_dev->funcs->pGetImage( src_dev, 0, src_info, &src_bits, &src->visrect );
    release_dc_ptr( dc_src );
    if (err) return FALSE;

    dst_dev = GET_DC_PHYSDEV( dc_dst, pPutImage );

    if ((src->width != dst->width) || (src->height != dst->height))
        FIXME( "should stretch %dx%d -> %dx%d\n",
               src->width, src->height, dst->width, dst->height );

    memcpy( dst_info, src_info, FIELD_OFFSET( BITMAPINFO, bmiColors[256] ));
    err = dst_dev->funcs->pPutImage( dst_dev, 0, dst_info, &src_bits, &dst->visrect, rop );
    if (err == ERROR_BAD_FORMAT)
    {
        RECT src_rect = src->visrect;

        offset_rect( &src_rect, src_bits.offset - src->visrect.left, -src->visrect.top );
        dst_info->bmiHeader.biWidth = src_rect.right - src_rect.left;
        dst_bits.ptr = HeapAlloc( GetProcessHeap(), 0, get_dib_image_size( dst_info ));
        if (dst_bits.ptr)
        {
            dst_bits.is_copy = TRUE;
            dst_bits.offset = 0;
            dst_bits.free = free_heap_bits;
            if (!(err = convert_bitmapinfo( src_info, src_bits.ptr, &src_rect, dst_info, dst_bits.ptr )))
                err = dst_dev->funcs->pPutImage( dst_dev, 0, dst_info, &dst_bits, &dst->visrect, rop );
            if (dst_bits.free) dst_bits.free( &dst_bits );
        }
        else err = ERROR_OUTOFMEMORY;
    }
    if (src_bits.free) src_bits.free( &src_bits );
    return !err;
}

/***********************************************************************
 *           PatBlt    (GDI32.@)
 */
BOOL WINAPI PatBlt( HDC hdc, INT left, INT top, INT width, INT height, DWORD rop)
{
    DC * dc;
    BOOL ret = FALSE;

    if (rop_uses_src( rop )) return FALSE;
    if ((dc = get_dc_ptr( hdc )))
    {
        struct bitblt_coords dst;
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pPatBlt );

        update_dc( dc );

        dst.log_x      = left;
        dst.log_y      = top;
        dst.log_width  = width;
        dst.log_height = height;
        dst.layout     = dc->layout;
        if (rop & NOMIRRORBITMAP)
        {
            dst.layout |= LAYOUT_BITMAPORIENTATIONPRESERVED;
            rop &= ~NOMIRRORBITMAP;
        }
        get_vis_rectangles( dc, &dst, NULL, NULL );

        TRACE("dst %p log=%d,%d %dx%d phys=%d,%d %dx%d vis=%s  rop=%06x\n",
              hdc, dst.log_x, dst.log_y, dst.log_width, dst.log_height,
              dst.x, dst.y, dst.width, dst.height, wine_dbgstr_rect(&dst.visrect), rop );

        ret = physdev->funcs->pPatBlt( physdev, &dst, rop );

        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           BitBlt    (GDI32.@)
 */
BOOL WINAPI BitBlt( HDC hdcDst, INT xDst, INT yDst, INT width,
                    INT height, HDC hdcSrc, INT xSrc, INT ySrc, DWORD rop )
{
    if (!rop_uses_src( rop )) return PatBlt( hdcDst, xDst, yDst, width, height, rop );
    else return StretchBlt( hdcDst, xDst, yDst, width, height,
                            hdcSrc, xSrc, ySrc, width, height, rop );
}


/***********************************************************************
 *           StretchBlt    (GDI32.@)
 */
BOOL WINAPI StretchBlt( HDC hdcDst, INT xDst, INT yDst, INT widthDst, INT heightDst,
                        HDC hdcSrc, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc, DWORD rop )
{
    BOOL ret = FALSE;
    DC *dcDst, *dcSrc;

    if (!rop_uses_src( rop )) return PatBlt( hdcDst, xDst, yDst, widthDst, heightDst, rop );

    if (!(dcDst = get_dc_ptr( hdcDst ))) return FALSE;

    if ((dcSrc = get_dc_ptr( hdcSrc )))
    {
        struct bitblt_coords src, dst;
        PHYSDEV src_dev = GET_DC_PHYSDEV( dcSrc, pStretchBlt );
        PHYSDEV dst_dev = GET_DC_PHYSDEV( dcDst, pStretchBlt );

        update_dc( dcSrc );
        update_dc( dcDst );

        src.log_x      = xSrc;
        src.log_y      = ySrc;
        src.log_width  = widthSrc;
        src.log_height = heightSrc;
        src.layout     = dcSrc->layout;
        dst.log_x      = xDst;
        dst.log_y      = yDst;
        dst.log_width  = widthDst;
        dst.log_height = heightDst;
        dst.layout     = dcDst->layout;
        if (rop & NOMIRRORBITMAP)
        {
            src.layout |= LAYOUT_BITMAPORIENTATIONPRESERVED;
            dst.layout |= LAYOUT_BITMAPORIENTATIONPRESERVED;
            rop &= ~NOMIRRORBITMAP;
        }
        get_vis_rectangles( dcDst, &dst, dcSrc, &src );

        TRACE("src %p log=%d,%d %dx%d phys=%d,%d %dx%d vis=%s  dst %p log=%d,%d %dx%d phys=%d,%d %dx%d vis=%s  rop=%06x\n",
              hdcSrc, src.log_x, src.log_y, src.log_width, src.log_height,
              src.x, src.y, src.width, src.height, wine_dbgstr_rect(&src.visrect),
              hdcDst, dst.log_x, dst.log_y, dst.log_width, dst.log_height,
              dst.x, dst.y, dst.width, dst.height, wine_dbgstr_rect(&dst.visrect), rop );

        ret = dst_dev->funcs->pStretchBlt( dst_dev, &dst, src_dev, &src, rop );
        release_dc_ptr( dcSrc );
    }
    release_dc_ptr( dcDst );
    return ret;
}

#define FRGND_ROP3(ROP4)	((ROP4) & 0x00FFFFFF)
#define BKGND_ROP3(ROP4)	(ROP3Table[((ROP4)>>24) & 0xFF])

/***********************************************************************
 *           MaskBlt [GDI32.@]
 */
BOOL WINAPI MaskBlt(HDC hdcDest, INT nXDest, INT nYDest,
                        INT nWidth, INT nHeight, HDC hdcSrc,
			INT nXSrc, INT nYSrc, HBITMAP hbmMask,
			INT xMask, INT yMask, DWORD dwRop)
{
    HBITMAP hBitmap1, hOldBitmap1, hBitmap2, hOldBitmap2;
    HDC hDC1, hDC2;
    HBRUSH hbrMask, hbrDst, hbrTmp;

    static const DWORD ROP3Table[256] = 
    {
        0x00000042, 0x00010289,
        0x00020C89, 0x000300AA,
        0x00040C88, 0x000500A9,
        0x00060865, 0x000702C5,
        0x00080F08, 0x00090245,
        0x000A0329, 0x000B0B2A,
        0x000C0324, 0x000D0B25,
        0x000E08A5, 0x000F0001,
        0x00100C85, 0x001100A6,
        0x00120868, 0x001302C8,
        0x00140869, 0x001502C9,
        0x00165CCA, 0x00171D54,
        0x00180D59, 0x00191CC8,
        0x001A06C5, 0x001B0768,
        0x001C06CA, 0x001D0766,
        0x001E01A5, 0x001F0385,
        0x00200F09, 0x00210248,
        0x00220326, 0x00230B24,
        0x00240D55, 0x00251CC5,
        0x002606C8, 0x00271868,
        0x00280369, 0x002916CA,
        0x002A0CC9, 0x002B1D58,
        0x002C0784, 0x002D060A,
        0x002E064A, 0x002F0E2A,
        0x0030032A, 0x00310B28,
        0x00320688, 0x00330008,
        0x003406C4, 0x00351864,
        0x003601A8, 0x00370388,
        0x0038078A, 0x00390604,
        0x003A0644, 0x003B0E24,
        0x003C004A, 0x003D18A4,
        0x003E1B24, 0x003F00EA,
        0x00400F0A, 0x00410249,
        0x00420D5D, 0x00431CC4,
        0x00440328, 0x00450B29,
        0x004606C6, 0x0047076A,
        0x00480368, 0x004916C5,
        0x004A0789, 0x004B0605,
        0x004C0CC8, 0x004D1954,
        0x004E0645, 0x004F0E25,
        0x00500325, 0x00510B26,
        0x005206C9, 0x00530764,
        0x005408A9, 0x00550009,
        0x005601A9, 0x00570389,
        0x00580785, 0x00590609,
        0x005A0049, 0x005B18A9,
        0x005C0649, 0x005D0E29,
        0x005E1B29, 0x005F00E9,
        0x00600365, 0x006116C6,
        0x00620786, 0x00630608,
        0x00640788, 0x00650606,
        0x00660046, 0x006718A8,
        0x006858A6, 0x00690145,
        0x006A01E9, 0x006B178A,
        0x006C01E8, 0x006D1785,
        0x006E1E28, 0x006F0C65,
        0x00700CC5, 0x00711D5C,
        0x00720648, 0x00730E28,
        0x00740646, 0x00750E26,
        0x00761B28, 0x007700E6,
        0x007801E5, 0x00791786,
        0x007A1E29, 0x007B0C68,
        0x007C1E24, 0x007D0C69,
        0x007E0955, 0x007F03C9,
        0x008003E9, 0x00810975,
        0x00820C49, 0x00831E04,
        0x00840C48, 0x00851E05,
        0x008617A6, 0x008701C5,
        0x008800C6, 0x00891B08,
        0x008A0E06, 0x008B0666,
        0x008C0E08, 0x008D0668,
        0x008E1D7C, 0x008F0CE5,
        0x00900C45, 0x00911E08,
        0x009217A9, 0x009301C4,
        0x009417AA, 0x009501C9,
        0x00960169, 0x0097588A,
        0x00981888, 0x00990066,
        0x009A0709, 0x009B07A8,
        0x009C0704, 0x009D07A6,
        0x009E16E6, 0x009F0345,
        0x00A000C9, 0x00A11B05,
        0x00A20E09, 0x00A30669,
        0x00A41885, 0x00A50065,
        0x00A60706, 0x00A707A5,
        0x00A803A9, 0x00A90189,
        0x00AA0029, 0x00AB0889,
        0x00AC0744, 0x00AD06E9,
        0x00AE0B06, 0x00AF0229,
        0x00B00E05, 0x00B10665,
        0x00B21974, 0x00B30CE8,
        0x00B4070A, 0x00B507A9,
        0x00B616E9, 0x00B70348,
        0x00B8074A, 0x00B906E6,
        0x00BA0B09, 0x00BB0226,
        0x00BC1CE4, 0x00BD0D7D,
        0x00BE0269, 0x00BF08C9,
        0x00C000CA, 0x00C11B04,
        0x00C21884, 0x00C3006A,
        0x00C40E04, 0x00C50664,
        0x00C60708, 0x00C707AA,
        0x00C803A8, 0x00C90184,
        0x00CA0749, 0x00CB06E4,
        0x00CC0020, 0x00CD0888,
        0x00CE0B08, 0x00CF0224,
        0x00D00E0A, 0x00D1066A,
        0x00D20705, 0x00D307A4,
        0x00D41D78, 0x00D50CE9,
        0x00D616EA, 0x00D70349,
        0x00D80745, 0x00D906E8,
        0x00DA1CE9, 0x00DB0D75,
        0x00DC0B04, 0x00DD0228,
        0x00DE0268, 0x00DF08C8,
        0x00E003A5, 0x00E10185,
        0x00E20746, 0x00E306EA,
        0x00E40748, 0x00E506E5,
        0x00E61CE8, 0x00E70D79,
        0x00E81D74, 0x00E95CE6,
        0x00EA02E9, 0x00EB0849,
        0x00EC02E8, 0x00ED0848,
        0x00EE0086, 0x00EF0A08,
        0x00F00021, 0x00F10885,
        0x00F20B05, 0x00F3022A,
        0x00F40B0A, 0x00F50225,
        0x00F60265, 0x00F708C5,
        0x00F802E5, 0x00F90845,
        0x00FA0089, 0x00FB0A09,
        0x00FC008A, 0x00FD0A0A,
        0x00FE02A9, 0x00FF0062,
    };

    if (!hbmMask)
	return BitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, FRGND_ROP3(dwRop));

    hbrMask = CreatePatternBrush(hbmMask);
    hbrDst = SelectObject(hdcDest, GetStockObject(NULL_BRUSH));

    /* make bitmap */
    hDC1 = CreateCompatibleDC(hdcDest);
    hBitmap1 = CreateCompatibleBitmap(hdcDest, nWidth, nHeight);
    hOldBitmap1 = SelectObject(hDC1, hBitmap1);

    /* draw using bkgnd rop */
    BitBlt(hDC1, 0, 0, nWidth, nHeight, hdcDest, nXDest, nYDest, SRCCOPY);
    hbrTmp = SelectObject(hDC1, hbrDst);
    BitBlt(hDC1, 0, 0, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, BKGND_ROP3(dwRop));
    SelectObject(hDC1, hbrTmp);

    /* make bitmap */
    hDC2 = CreateCompatibleDC(hdcDest);
    hBitmap2 = CreateCompatibleBitmap(hdcDest, nWidth, nHeight);
    hOldBitmap2 = SelectObject(hDC2, hBitmap2);

    /* draw using foregnd rop */
    BitBlt(hDC2, 0, 0, nWidth, nHeight, hdcDest, nXDest, nYDest, SRCCOPY);
    hbrTmp = SelectObject(hDC2, hbrDst);
    BitBlt(hDC2, 0, 0, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, FRGND_ROP3(dwRop));

    /* combine both using the mask as a pattern brush */
    SelectObject(hDC2, hbrMask);
    BitBlt(hDC2, 0, 0, nWidth, nHeight, hDC1, 0, 0, 0xac0744 ); /* (D & P) | (S & ~P) */ 
    SelectObject(hDC2, hbrTmp);

    /* blit to dst */
    BitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hDC2, 0, 0, SRCCOPY);

    /* restore all objects */
    SelectObject(hdcDest, hbrDst);
    SelectObject(hDC1, hOldBitmap1);
    SelectObject(hDC2, hOldBitmap2);

    /* delete all temp objects */
    DeleteObject(hBitmap1);
    DeleteObject(hBitmap2);
    DeleteObject(hbrMask);

    DeleteDC(hDC1);
    DeleteDC(hDC2);

    return TRUE;
}

/******************************************************************************
 *           GdiTransparentBlt [GDI32.@]
 */
BOOL WINAPI GdiTransparentBlt( HDC hdcDest, int xDest, int yDest, int widthDest, int heightDest,
                            HDC hdcSrc, int xSrc, int ySrc, int widthSrc, int heightSrc,
                            UINT crTransparent )
{
    BOOL ret = FALSE;
    HDC hdcWork;
    HBITMAP bmpWork;
    HGDIOBJ oldWork;
    HDC hdcMask = NULL;
    HBITMAP bmpMask = NULL;
    HBITMAP oldMask = NULL;
    COLORREF oldBackground;
    COLORREF oldForeground;
    int oldStretchMode;

    if(widthDest < 0 || heightDest < 0 || widthSrc < 0 || heightSrc < 0) {
        TRACE("Cannot mirror\n");
        return FALSE;
    }

    oldBackground = SetBkColor(hdcDest, RGB(255,255,255));
    oldForeground = SetTextColor(hdcDest, RGB(0,0,0));

    /* Stretch bitmap */
    oldStretchMode = GetStretchBltMode(hdcSrc);
    if(oldStretchMode == BLACKONWHITE || oldStretchMode == WHITEONBLACK)
        SetStretchBltMode(hdcSrc, COLORONCOLOR);
    hdcWork = CreateCompatibleDC(hdcDest);
    bmpWork = CreateCompatibleBitmap(hdcDest, widthDest, heightDest);
    oldWork = SelectObject(hdcWork, bmpWork);
    if(!StretchBlt(hdcWork, 0, 0, widthDest, heightDest, hdcSrc, xSrc, ySrc, widthSrc, heightSrc, SRCCOPY)) {
        TRACE("Failed to stretch\n");
        goto error;
    }
    SetBkColor(hdcWork, crTransparent);

    /* Create mask */
    hdcMask = CreateCompatibleDC(hdcDest);
    bmpMask = CreateCompatibleBitmap(hdcMask, widthDest, heightDest);
    oldMask = SelectObject(hdcMask, bmpMask);
    if(!BitBlt(hdcMask, 0, 0, widthDest, heightDest, hdcWork, 0, 0, SRCCOPY)) {
        TRACE("Failed to create mask\n");
        goto error;
    }

    /* Replace transparent color with black */
    SetBkColor(hdcWork, RGB(0,0,0));
    SetTextColor(hdcWork, RGB(255,255,255));
    if(!BitBlt(hdcWork, 0, 0, widthDest, heightDest, hdcMask, 0, 0, SRCAND)) {
        TRACE("Failed to mask out background\n");
        goto error;
    }

    /* Replace non-transparent area on destination with black */
    if(!BitBlt(hdcDest, xDest, yDest, widthDest, heightDest, hdcMask, 0, 0, SRCAND)) {
        TRACE("Failed to clear destination area\n");
        goto error;
    }

    /* Draw the image */
    if(!BitBlt(hdcDest, xDest, yDest, widthDest, heightDest, hdcWork, 0, 0, SRCPAINT)) {
        TRACE("Failed to paint image\n");
        goto error;
    }

    ret = TRUE;
error:
    SetStretchBltMode(hdcSrc, oldStretchMode);
    SetBkColor(hdcDest, oldBackground);
    SetTextColor(hdcDest, oldForeground);
    if(hdcWork) {
        SelectObject(hdcWork, oldWork);
        DeleteDC(hdcWork);
    }
    if(bmpWork) DeleteObject(bmpWork);
    if(hdcMask) {
        SelectObject(hdcMask, oldMask);
        DeleteDC(hdcMask);
    }
    if(bmpMask) DeleteObject(bmpMask);
    return ret;
}

/******************************************************************************
 *           GdiAlphaBlend [GDI32.@]
 */
BOOL WINAPI GdiAlphaBlend(HDC hdcDst, int xDst, int yDst, int widthDst, int heightDst,
                          HDC hdcSrc, int xSrc, int ySrc, int widthSrc, int heightSrc,
                          BLENDFUNCTION blendFunction)
{
    BOOL ret = FALSE;
    DC *dcDst, *dcSrc;

    dcSrc = get_dc_ptr( hdcSrc );
    if (!dcSrc) return FALSE;

    if ((dcDst = get_dc_ptr( hdcDst )))
    {
        struct bitblt_coords src, dst;
        PHYSDEV src_dev = GET_DC_PHYSDEV( dcSrc, pAlphaBlend );
        PHYSDEV dst_dev = GET_DC_PHYSDEV( dcDst, pAlphaBlend );

        update_dc( dcSrc );
        update_dc( dcDst );

        src.log_x      = xSrc;
        src.log_y      = ySrc;
        src.log_width  = widthSrc;
        src.log_height = heightSrc;
        src.layout     = GetLayout( src_dev->hdc );
        dst.log_x      = xDst;
        dst.log_y      = yDst;
        dst.log_width  = widthDst;
        dst.log_height = heightDst;
        dst.layout     = GetLayout( dst_dev->hdc );
        get_vis_rectangles( dcDst, &dst, dcSrc, &src );

        TRACE("src %p log=%d,%d %dx%d phys=%d,%d %dx%d vis=%s  dst %p log=%d,%d %dx%d phys=%d,%d %dx%d vis=%s  blend=%02x/%02x/%02x/%02x\n",
              hdcSrc, src.log_x, src.log_y, src.log_width, src.log_height,
              src.x, src.y, src.width, src.height, wine_dbgstr_rect(&src.visrect),
              hdcDst, dst.log_x, dst.log_y, dst.log_width, dst.log_height,
              dst.x, dst.y, dst.width, dst.height, wine_dbgstr_rect(&dst.visrect),
              blendFunction.BlendOp, blendFunction.BlendFlags,
              blendFunction.SourceConstantAlpha, blendFunction.AlphaFormat );

        ret = dst_dev->funcs->pAlphaBlend( dst_dev, &dst, src_dev, &src, blendFunction );
        release_dc_ptr( dcDst );
    }
    release_dc_ptr( dcSrc );
    return ret;
}

/*********************************************************************
 *      PlgBlt [GDI32.@]
 *
 */
BOOL WINAPI PlgBlt( HDC hdcDest, const POINT *lpPoint,
                        HDC hdcSrc, INT nXSrc, INT nYSrc, INT nWidth,
                        INT nHeight, HBITMAP hbmMask, INT xMask, INT yMask)
{
    int oldgMode;
    /* parallelogram coords */
    POINT plg[3];
    /* rect coords */
    POINT rect[3];
    XFORM xf;
    XFORM SrcXf;
    XFORM oldDestXf;
    double det;

    /* save actual mode, set GM_ADVANCED */
    oldgMode = SetGraphicsMode(hdcDest,GM_ADVANCED);
    if (oldgMode == 0)
        return FALSE;

    memcpy(plg,lpPoint,sizeof(POINT)*3);
    rect[0].x = nXSrc;
    rect[0].y = nYSrc;
    rect[1].x = nXSrc + nWidth;
    rect[1].y = nYSrc;
    rect[2].x = nXSrc;
    rect[2].y = nYSrc + nHeight;
    /* calc XFORM matrix to transform hdcDest -> hdcSrc (parallelogram to rectangle) */
    /* determinant */
    det = rect[1].x*(rect[2].y - rect[0].y) - rect[2].x*(rect[1].y - rect[0].y) - rect[0].x*(rect[2].y - rect[1].y);

    if (fabs(det) < 1e-5)
    {
        SetGraphicsMode(hdcDest,oldgMode);
        return FALSE;
    }

    TRACE("hdcSrc=%p %d,%d,%dx%d -> hdcDest=%p %d,%d,%d,%d,%d,%d\n",
        hdcSrc, nXSrc, nYSrc, nWidth, nHeight, hdcDest, plg[0].x, plg[0].y, plg[1].x, plg[1].y, plg[2].x, plg[2].y);

    /* X components */
    xf.eM11 = (plg[1].x*(rect[2].y - rect[0].y) - plg[2].x*(rect[1].y - rect[0].y) - plg[0].x*(rect[2].y - rect[1].y)) / det;
    xf.eM21 = (rect[1].x*(plg[2].x - plg[0].x) - rect[2].x*(plg[1].x - plg[0].x) - rect[0].x*(plg[2].x - plg[1].x)) / det;
    xf.eDx  = (rect[0].x*(rect[1].y*plg[2].x - rect[2].y*plg[1].x) -
               rect[1].x*(rect[0].y*plg[2].x - rect[2].y*plg[0].x) +
               rect[2].x*(rect[0].y*plg[1].x - rect[1].y*plg[0].x)
               ) / det;

    /* Y components */
    xf.eM12 = (plg[1].y*(rect[2].y - rect[0].y) - plg[2].y*(rect[1].y - rect[0].y) - plg[0].y*(rect[2].y - rect[1].y)) / det;
    xf.eM22 = (plg[1].x*(rect[2].y - rect[0].y) - plg[2].x*(rect[1].y - rect[0].y) - plg[0].x*(rect[2].y - rect[1].y)) / det;
    xf.eDy  = (rect[0].x*(rect[1].y*plg[2].y - rect[2].y*plg[1].y) -
               rect[1].x*(rect[0].y*plg[2].y - rect[2].y*plg[0].y) +
               rect[2].x*(rect[0].y*plg[1].y - rect[1].y*plg[0].y)
               ) / det;

    GetWorldTransform(hdcSrc,&SrcXf);
    CombineTransform(&xf,&xf,&SrcXf);

    /* save actual dest transform */
    GetWorldTransform(hdcDest,&oldDestXf);

    SetWorldTransform(hdcDest,&xf);
    /* now destination and source DCs use same coords */
    MaskBlt(hdcDest,nXSrc,nYSrc,nWidth,nHeight,
            hdcSrc, nXSrc,nYSrc,
            hbmMask,xMask,yMask,
            SRCCOPY);
    /* restore dest DC */
    SetWorldTransform(hdcDest,&oldDestXf);
    SetGraphicsMode(hdcDest,oldgMode);

    return TRUE;
}
