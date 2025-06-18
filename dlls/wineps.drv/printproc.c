/*
 * Print processor implementation.
 *
 * Copyright 2022 Piotr Caban for CodeWeavers
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

#include <math.h>
#include <stdlib.h>

#include <windows.h>
#include <ntgdi.h>
#include <winppi.h>
#include <winspool.h>
#include <ddk/winsplp.h>
#include <usp10.h>

#include "psdrv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

#define EMFSPOOL_VERSION 0x10000
#define PP_MAGIC 0x952173fe

struct pp_data
{
    DWORD magic;
    HANDLE hport;
    WCHAR *port;
    WCHAR *doc_name;
    WCHAR *out_file;

    print_ctx *ctx;

    struct ps_brush_pattern *patterns;
    BOOL path;
    INT break_extra;
    INT break_rem;

    INT saved_dc_size;
    INT saved_dc_top;
    struct
    {
        INT break_extra;
        INT break_rem;
    } *saved_dc;
};

typedef enum
{
    EMRI_METAFILE = 1,
    EMRI_ENGINE_FONT,
    EMRI_DEVMODE,
    EMRI_TYPE1_FONT,
    EMRI_PRESTARTPAGE,
    EMRI_DESIGNVECTOR,
    EMRI_SUBSET_FONT,
    EMRI_DELTA_FONT,
    EMRI_FORM_METAFILE,
    EMRI_BW_METAFILE,
    EMRI_BW_FORM_METAFILE,
    EMRI_METAFILE_DATA,
    EMRI_METAFILE_EXT,
    EMRI_BW_METAFILE_EXT,
    EMRI_ENGINE_FONT_EXT,
    EMRI_TYPE1_FONT_EXT,
    EMRI_DESIGNVECTOR_EXT,
    EMRI_SUBSET_FONT_EXT,
    EMRI_DELTA_FONT_EXT,
    EMRI_PS_JOB_DATA,
    EMRI_EMBED_FONT_EXT,
} record_type;

typedef struct
{
    unsigned int dwVersion;
    unsigned int cjSize;
    unsigned int dpszDocName;
    unsigned int dpszOutput;
} emfspool_header;

typedef struct
{
    unsigned int ulID;
    unsigned int cjSize;
} record_hdr;

typedef struct
{
    EMR emr;
    INT break_extra;
    INT break_count;
} EMRSETTEXTJUSTIFICATION;

BOOL WINAPI SeekPrinter(HANDLE, LARGE_INTEGER, LARGE_INTEGER*, DWORD, BOOL);

static const WCHAR emf_1003[] = L"NT EMF 1.003";

#define EMRICASE(x) case x: return #x
static const char * debugstr_rec_type(int id)
{
    switch (id)
    {
    EMRICASE(EMRI_METAFILE);
    EMRICASE(EMRI_ENGINE_FONT);
    EMRICASE(EMRI_DEVMODE);
    EMRICASE(EMRI_TYPE1_FONT);
    EMRICASE(EMRI_PRESTARTPAGE);
    EMRICASE(EMRI_DESIGNVECTOR);
    EMRICASE(EMRI_SUBSET_FONT);
    EMRICASE(EMRI_DELTA_FONT);
    EMRICASE(EMRI_FORM_METAFILE);
    EMRICASE(EMRI_BW_METAFILE);
    EMRICASE(EMRI_BW_FORM_METAFILE);
    EMRICASE(EMRI_METAFILE_DATA);
    EMRICASE(EMRI_METAFILE_EXT);
    EMRICASE(EMRI_BW_METAFILE_EXT);
    EMRICASE(EMRI_ENGINE_FONT_EXT);
    EMRICASE(EMRI_TYPE1_FONT_EXT);
    EMRICASE(EMRI_DESIGNVECTOR_EXT);
    EMRICASE(EMRI_SUBSET_FONT_EXT);
    EMRICASE(EMRI_DELTA_FONT_EXT);
    EMRICASE(EMRI_PS_JOB_DATA);
    EMRICASE(EMRI_EMBED_FONT_EXT);
    default:
        FIXME("unknown record type: %d\n", id);
        return NULL;
    }
}
#undef EMRICASE

static struct pp_data* get_handle_data(HANDLE pp)
{
    struct pp_data *ret = (struct pp_data *)pp;

    if (!ret || ret->magic != PP_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }
    return ret;
}

static inline INT GDI_ROUND(double val)
{
    return (int)floor(val + 0.5);
}

static void translate(RECT *rect, const XFORM *xform)
{
    double x, y;

    x = rect->left;
    y = rect->top;
    rect->left = GDI_ROUND(x * xform->eM11 + y * xform->eM21 + xform->eDx);
    rect->top = GDI_ROUND(x * xform->eM12 + y * xform->eM22 + xform->eDy);

    x = rect->right;
    y = rect->bottom;
    rect->right = GDI_ROUND(x * xform->eM11 + y * xform->eM21 + xform->eDx);
    rect->bottom = GDI_ROUND(x * xform->eM12 + y * xform->eM22 + xform->eDy);
}

static inline void get_bounding_rect(RECT *rect, int x, int y, int width, int height)
{
    rect->left = x;
    rect->right = x + width;
    rect->top = y;
    rect->bottom = y + height;
    if (rect->left > rect->right)
    {
        int tmp = rect->left;
        rect->left = rect->right + 1;
        rect->right = tmp + 1;
    }
    if (rect->top > rect->bottom)
    {
        int tmp = rect->top;
        rect->top = rect->bottom + 1;
        rect->bottom = tmp + 1;
    }
}

static inline void order_rect(RECT *rect)
{
    if (rect->left > rect->right)
    {
        int tmp = rect->left;
        rect->left = rect->right;
        rect->right = tmp;
    }
    if (rect->top > rect->bottom)
    {
        int tmp = rect->top;
        rect->top = rect->bottom;
        rect->bottom = tmp;
    }
}

static BOOL intersect_vis_rectangles(struct ps_bitblt_coords *dst, struct ps_bitblt_coords *src)
{
    RECT rect;

    /* intersect the rectangles */

    if ((src->width == dst->width) && (src->height == dst->height)) /* no stretching */
    {
        OffsetRect(&src->visrect, dst->x - src->x, dst->y - src->y);
        if (!IntersectRect(&rect, &src->visrect, &dst->visrect)) return FALSE;
        src->visrect = dst->visrect = rect;
        OffsetRect(&src->visrect, src->x - dst->x, src->y - dst->y);
    }
    else  /* stretching */
    {
        /* map source rectangle into destination coordinates */
        rect = src->visrect;
        OffsetRect(&rect,
                    -src->x - (src->width < 0 ? 1 : 0),
                    -src->y - (src->height < 0 ? 1 : 0));
        rect.left   = rect.left * dst->width / src->width;
        rect.top    = rect.top * dst->height / src->height;
        rect.right  = rect.right * dst->width / src->width;
        rect.bottom = rect.bottom * dst->height / src->height;
        order_rect(&rect);

        /* when the source rectangle needs to flip and it doesn't fit in the source device
           area, the destination area isn't flipped. So, adjust destination coordinates */
        if (src->width < 0 && dst->width > 0 &&
            (src->x + src->width + 1 < src->visrect.left || src->x > src->visrect.right))
            dst->x += (dst->width - rect.right) - rect.left;
        else if (src->width > 0 && dst->width < 0 &&
                 (src->x < src->visrect.left || src->x + src->width > src->visrect.right))
            dst->x -= rect.right - (dst->width - rect.left);

        if (src->height < 0 && dst->height > 0 &&
            (src->y + src->height + 1 < src->visrect.top || src->y > src->visrect.bottom))
            dst->y += (dst->height - rect.bottom) - rect.top;
        else if (src->height > 0 && dst->height < 0 &&
                 (src->y < src->visrect.top || src->y + src->height > src->visrect.bottom))
            dst->y -= rect.bottom - (dst->height - rect.top);

        OffsetRect(&rect, dst->x, dst->y);

        /* avoid rounding errors */
        rect.left--;
        rect.top--;
        rect.right++;
        rect.bottom++;
        if (!IntersectRect(&dst->visrect, &rect, &dst->visrect)) return FALSE;

        /* map destination rectangle back to source coordinates */
        rect = dst->visrect;
        OffsetRect(&rect,
                    -dst->x - (dst->width < 0 ? 1 : 0),
                    -dst->y - (dst->height < 0 ? 1 : 0));
        rect.left   = src->x + rect.left * src->width / dst->width;
        rect.top    = src->y + rect.top * src->height / dst->height;
        rect.right  = src->x + rect.right * src->width / dst->width;
        rect.bottom = src->y + rect.bottom * src->height / dst->height;
        order_rect(&rect);

        /* avoid rounding errors */
        rect.left--;
        rect.top--;
        rect.right++;
        rect.bottom++;
        if (!IntersectRect(&src->visrect, &rect, &src->visrect)) return FALSE;
    }
    return TRUE;
}

static void clip_visrect(HDC hdc, RECT *dst, const RECT *src)
{
    HRGN hrgn;

    hrgn = CreateRectRgn(0, 0, 0, 0);
    if (GetRandomRgn(hdc, hrgn, 3) == 1)
    {
        GetRgnBox(hrgn, dst);
        IntersectRect(dst, dst, src);
    }
    else
    {
        *dst = *src;
    }
    DeleteObject(hrgn);
}

static BOOL get_vis_rectangles(HDC hdc, struct ps_bitblt_coords *dst,
        const XFORM *xform, DWORD width, DWORD height, struct ps_bitblt_coords *src)
{
    RECT rect;

    rect.left = dst->log_x;
    rect.top = dst->log_y;
    rect.right = dst->log_x + dst->log_width;
    rect.bottom = dst->log_y + dst->log_height;
    LPtoDP(hdc, (POINT *)&rect, 2);
    dst->x = rect.left;
    dst->y = rect.top;
    dst->width = rect.right - rect.left;
    dst->height = rect.bottom - rect.top;
    if (dst->layout & LAYOUT_RTL && dst->layout & LAYOUT_BITMAPORIENTATIONPRESERVED)
    {
        dst->x += dst->width;
        dst->width = -dst->width;
    }
    get_bounding_rect(&rect, dst->x, dst->y, dst->width, dst->height);
    clip_visrect(hdc, &dst->visrect, &rect);

    if (IsRectEmpty(&dst->visrect)) return FALSE;
    if (!src) return TRUE;

    rect.left   = src->log_x;
    rect.top    = src->log_y;
    rect.right  = src->log_x + src->log_width;
    rect.bottom = src->log_y + src->log_height;
    translate(&rect, xform);
    src->x      = rect.left;
    src->y      = rect.top;
    src->width  = rect.right - rect.left;
    src->height = rect.bottom - rect.top;
    get_bounding_rect(&rect, src->x, src->y, src->width, src->height);
    if (rect.left < 0) rect.left = 0;
    if (rect.top < 0) rect.top = 0;
    if (rect.right > width) rect.right = width;
    if (rect.bottom > height) rect.bottom = height;
    src->visrect = rect;

    return intersect_vis_rectangles(dst, src);
}

static int stretch_blt(print_ctx *ctx, const EMRSTRETCHBLT *blt,
        const BITMAPINFO *bi, const BYTE *src_bits)
{
    char dst_buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *dst_info = (BITMAPINFO *)dst_buffer;
    struct ps_bitblt_coords src, dst;
    struct ps_image_bits bits;
    DWORD err;

    dst.log_x = blt->xDest;
    dst.log_y = blt->yDest;
    dst.log_width = blt->cxDest;
    dst.log_height = blt->cyDest;
    dst.layout = GetLayout(ctx->hdc);
    if (blt->dwRop & NOMIRRORBITMAP)
        dst.layout |= LAYOUT_BITMAPORIENTATIONPRESERVED;

    if (!blt->cbBmiSrc)
    {
        if (!get_vis_rectangles(ctx->hdc, &dst, NULL, 0, 0, NULL))
            return TRUE;
        return PSDRV_PatBlt(ctx, &dst, blt->dwRop);
    }

    src.log_x = blt->xSrc;
    src.log_y = blt->ySrc;
    src.log_width = blt->cxSrc;
    src.log_height = blt->cySrc;
    src.layout = 0;

    if (!get_vis_rectangles(ctx->hdc, &dst, &blt->xformSrc,
                bi->bmiHeader.biWidth, abs(bi->bmiHeader.biHeight), &src))
        return TRUE;

    memcpy(dst_info, bi, blt->cbBmiSrc);
    memset(&bits, 0, sizeof(bits));
    bits.ptr = (BYTE *)src_bits;
    err = PSDRV_PutImage(ctx, 0, dst_info, &bits, &src, &dst, blt->dwRop);
    if (err == ERROR_BAD_FORMAT)
    {
        HDC hdc = CreateCompatibleDC(NULL);
        HBITMAP bitmap;

        bits.is_copy = TRUE;
        bitmap = CreateDIBSection(hdc, dst_info, DIB_RGB_COLORS, &bits.ptr, NULL, 0);
        SetDIBits(hdc, bitmap, 0, abs(bi->bmiHeader.biHeight), src_bits, bi, blt->iUsageSrc);

        err = PSDRV_PutImage(ctx, 0, dst_info, &bits, &src, &dst, blt->dwRop);
        DeleteObject(bitmap);
        DeleteObject(hdc);
    }

    if (err != ERROR_SUCCESS)
        FIXME("PutImage returned %ld\n", err);
    return !err;
}

#define FRGND_ROP3(ROP4)        ((ROP4) & 0x00FFFFFF)
#define BKGND_ROP3(ROP4)        (ROP3Table[((ROP4)>>24) & 0xFF])

static int mask_blt(print_ctx *ctx, const EMRMASKBLT *p, const BITMAPINFO *src_bi,
        const BYTE *src_bits, const BITMAPINFO *mask_bi, const BYTE *mask_bits)
{
    HBITMAP bmp1, old_bmp1, bmp2, old_bmp2, bmp_src, old_bmp_src;
    char bmp2_buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *bmp2_info = (BITMAPINFO *)bmp2_buffer;
    HBRUSH brush_mask, brush_dest, old_brush;
    HDC hdc_src, hdc1, hdc2;
    BYTE *bits;
    EMRSTRETCHBLT blt;

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

    if (!p->cbBmiMask)
    {
        blt.rclBounds = p->rclBounds;
        blt.xDest = p->xDest;
        blt.yDest = p->yDest;
        blt.cxDest = p->cxDest;
        blt.cyDest = p->cyDest;
        blt.dwRop = FRGND_ROP3(p->dwRop);
        blt.xSrc = p->xSrc;
        blt.ySrc = p->ySrc;
        blt.xformSrc = p->xformSrc;
        blt.crBkColorSrc = p->crBkColorSrc;
        blt.iUsageSrc = p->iUsageSrc;
        blt.offBmiSrc = 0;
        blt.cbBmiSrc = p->cbBmiSrc;
        blt.offBitsSrc = 0;
        blt.cbBitsSrc = p->cbBitsSrc;
        blt.cxSrc = p->cxDest;
        blt.cySrc = p->cyDest;

        return stretch_blt(ctx, &blt, src_bi, src_bits);
    }

    hdc_src = CreateCompatibleDC(NULL);
    SetGraphicsMode(hdc_src, GM_ADVANCED);
    SetWorldTransform(hdc_src, &p->xformSrc);
    brush_dest = CreateSolidBrush(p->crBkColorSrc);
    old_brush = SelectObject(hdc_src, brush_dest);
    PatBlt(hdc_src, p->rclBounds.left, p->rclBounds.top,
            p->rclBounds.right - p->rclBounds.left,
            p->rclBounds.bottom - p->rclBounds.top, PATCOPY);
    SelectObject(hdc_src, old_brush);
    DeleteObject(brush_dest);

    bmp_src = CreateDIBSection(hdc_src, src_bi, p->iUsageSrc, (void **)&bits, NULL, 0);
    memcpy(bits, src_bits, p->cbBitsSrc);
    old_bmp_src = SelectObject(hdc_src, bmp_src);

    bmp1 = CreateBitmap(mask_bi->bmiHeader.biWidth, mask_bi->bmiHeader.biHeight, 1, 1, NULL);
    SetDIBits(ctx->hdc, bmp1, 0, mask_bi->bmiHeader.biHeight, mask_bits, mask_bi, p->iUsageMask);
    brush_mask = CreatePatternBrush(bmp1);
    DeleteObject(bmp1);
    brush_dest = SelectObject(ctx->hdc, GetStockObject(NULL_BRUSH));

    /* make bitmap */
    hdc1 = CreateCompatibleDC(NULL);
    bmp1 = CreateBitmap(p->cxDest, p->cyDest, 1, 32, NULL);
    old_bmp1 = SelectObject(hdc1, bmp1);

    /* draw using bkgnd rop */
    old_brush = SelectObject(hdc1, brush_dest);
    BitBlt(hdc1, 0, 0, p->cxDest, p->cyDest, hdc_src, p->xSrc, p->ySrc, BKGND_ROP3(p->dwRop));
    SelectObject(hdc1, old_brush);

    /* make bitmap */
    hdc2 = CreateCompatibleDC(NULL);
    bmp2_info->bmiHeader.biSize = sizeof(bmp2_info->bmiHeader);
    bmp2_info->bmiHeader.biWidth = p->cxDest;
    bmp2_info->bmiHeader.biHeight = p->cyDest;
    bmp2_info->bmiHeader.biPlanes = 1;
    bmp2_info->bmiHeader.biBitCount = 32;
    bmp2_info->bmiHeader.biCompression = BI_RGB;
    bmp2_info->bmiHeader.biSizeImage = p->cxDest * p->cyDest * bmp2_info->bmiHeader.biBitCount / 8;
    bmp2 = CreateDIBSection(hdc2, bmp2_info, DIB_RGB_COLORS, (void **)&bits, NULL, 0);
    old_bmp2 = SelectObject(hdc2, bmp2);

    /* draw using foregnd rop */
    old_brush = SelectObject(hdc2, brush_dest);
    BitBlt(hdc2, 0, 0, p->cxDest, p->cyDest, hdc_src, p->xSrc, p->ySrc, FRGND_ROP3(p->dwRop));

    /* combine both using the mask as a pattern brush */
    SelectObject(hdc2, brush_mask);
    SetBrushOrgEx(hdc2, -p->xMask, -p->yMask, NULL);
    /* (D & P) | (S & ~P) */
    BitBlt(hdc2, 0, 0, p->cxDest, p->cyDest, hdc1, 0, 0, 0xac0744);
    SelectObject(hdc2, old_brush);

    /* blit to dst */
    blt.rclBounds = p->rclBounds;
    blt.xDest = p->xDest;
    blt.yDest = p->yDest;
    blt.cxDest = p->cxDest;
    blt.cyDest = p->cyDest;
    blt.dwRop = SRCCOPY;
    blt.xSrc = 0;
    blt.ySrc = 0;
    GetTransform(hdc2, 0x204, &blt.xformSrc);
    blt.crBkColorSrc = p->crBkColorSrc;
    blt.iUsageSrc = DIB_RGB_COLORS;
    blt.offBmiSrc = 0;
    blt.cbBmiSrc = bmp2_info->bmiHeader.biSize;
    blt.offBitsSrc = 0;
    blt.cbBitsSrc = bmp2_info->bmiHeader.biSizeImage;
    blt.cxSrc = p->cxDest;
    blt.cySrc = p->cyDest;

    stretch_blt(ctx, &blt, bmp2_info, bits);

    /* restore all objects */
    SelectObject(ctx->hdc, brush_dest);
    SelectObject(hdc1, old_bmp1);
    SelectObject(hdc2, old_bmp2);
    SelectObject(hdc_src, old_bmp_src);

    /* delete all temp objects */
    DeleteObject(bmp1);
    DeleteObject(bmp2);
    DeleteObject(bmp_src);
    DeleteObject(brush_mask);

    DeleteObject(hdc1);
    DeleteObject(hdc2);
    DeleteObject(hdc_src);

    return TRUE;
}

static void combine_transform(XFORM *result, const XFORM *xform1, const XFORM *xform2)
{
    XFORM r;

    /* Create the result in a temporary XFORM, since result may be
     * equal to xform1 or xform2 */
    r.eM11 = xform1->eM11 * xform2->eM11 + xform1->eM12 * xform2->eM21;
    r.eM12 = xform1->eM11 * xform2->eM12 + xform1->eM12 * xform2->eM22;
    r.eM21 = xform1->eM21 * xform2->eM11 + xform1->eM22 * xform2->eM21;
    r.eM22 = xform1->eM21 * xform2->eM12 + xform1->eM22 * xform2->eM22;
    r.eDx  = xform1->eDx  * xform2->eM11 + xform1->eDy  * xform2->eM21 + xform2->eDx;
    r.eDy  = xform1->eDx  * xform2->eM12 + xform1->eDy  * xform2->eM22 + xform2->eDy;

    *result = r;
}

static int plg_blt(print_ctx *ctx, const EMRPLGBLT *p)
{
    const BITMAPINFO *src_bi, *mask_bi;
    const BYTE *src_bits, *mask_bits;
    XFORM xf, xform_dest;
    EMRMASKBLT maskblt;
    /* rect coords */
    POINT rect[3];
    /* parallelogram coords */
    POINT plg[3];
    double det;

    memcpy(plg, p->aptlDest, sizeof(plg));
    rect[0].x = p->xSrc;
    rect[0].y = p->ySrc;
    rect[1].x = p->xSrc + p->cxSrc;
    rect[1].y = p->ySrc;
    rect[2].x = p->xSrc;
    rect[2].y = p->ySrc + p->cySrc;
    /* calc XFORM matrix to transform hdcDest -> hdcSrc (parallelogram to rectangle) */
    /* determinant */
    det = rect[1].x*(rect[2].y - rect[0].y) - rect[2].x*(rect[1].y - rect[0].y) - rect[0].x*(rect[2].y - rect[1].y);

    if (fabs(det) < 1e-5)
        return TRUE;

    TRACE("%ld,%ld,%ldx%ld -> %ld,%ld,%ld,%ld,%ld,%ld\n", p->xSrc, p->ySrc, p->cxSrc, p->cySrc,
            plg[0].x, plg[0].y, plg[1].x, plg[1].y, plg[2].x, plg[2].y);

    /* X components */
    xf.eM11 = (plg[1].x*(rect[2].y - rect[0].y) - plg[2].x*(rect[1].y - rect[0].y) - plg[0].x*(rect[2].y - rect[1].y)) / det;
    xf.eM21 = (rect[1].x*(plg[2].x - plg[0].x) - rect[2].x*(plg[1].x - plg[0].x) - rect[0].x*(plg[2].x - plg[1].x)) / det;
    xf.eDx  = (rect[0].x*(rect[1].y*plg[2].x - rect[2].y*plg[1].x) -
            rect[1].x*(rect[0].y*plg[2].x - rect[2].y*plg[0].x) +
            rect[2].x*(rect[0].y*plg[1].x - rect[1].y*plg[0].x)
            ) / det;

    /* Y components */
    xf.eM12 = (plg[1].y*(rect[2].y - rect[0].y) - plg[2].y*(rect[1].y - rect[0].y) - plg[0].y*(rect[2].y - rect[1].y)) / det;
    xf.eM22 = (rect[1].x*(plg[2].y - plg[0].y) - rect[2].x*(plg[1].y - plg[0].y) - rect[0].x*(plg[2].y - plg[1].y)) / det;
    xf.eDy  = (rect[0].x*(rect[1].y*plg[2].y - rect[2].y*plg[1].y) -
            rect[1].x*(rect[0].y*plg[2].y - rect[2].y*plg[0].y) +
            rect[2].x*(rect[0].y*plg[1].y - rect[1].y*plg[0].y)
            ) / det;

    combine_transform(&xf, &xf, &p->xformSrc);

    GetTransform(ctx->hdc, 0x203, &xform_dest);
    SetWorldTransform(ctx->hdc, &xf);
    /* now destination and source DCs use same coords */
    maskblt.rclBounds = p->rclBounds;
    maskblt.xDest = p->xSrc;
    maskblt.yDest = p->ySrc;
    maskblt.cxDest = p->cxSrc;
    maskblt.cyDest = p->cySrc;
    maskblt.dwRop = SRCCOPY;
    maskblt.xSrc = p->xSrc;
    maskblt.ySrc = p->ySrc;
    maskblt.xformSrc = p->xformSrc;
    maskblt.crBkColorSrc = p->crBkColorSrc;
    maskblt.iUsageSrc = p->iUsageSrc;
    maskblt.offBmiSrc = 0;
    maskblt.cbBmiSrc = p->cbBmiSrc;
    maskblt.offBitsSrc = 0;
    maskblt.cbBitsSrc = p->cbBitsSrc;
    maskblt.xMask = p->xMask;
    maskblt.yMask = p->yMask;
    maskblt.iUsageMask = p->iUsageMask;
    maskblt.offBmiMask = 0;
    maskblt.cbBmiMask = p->cbBmiMask;
    maskblt.offBitsMask = 0;
    maskblt.cbBitsMask = p->cbBitsMask;
    src_bi = (const BITMAPINFO *)((BYTE *)p + p->offBmiSrc);
    src_bits = (BYTE *)p + p->offBitsSrc;
    mask_bi = (const BITMAPINFO *)((BYTE *)p + p->offBmiMask);
    mask_bits = (BYTE *)p + p->offBitsMask;
    mask_blt(ctx, &maskblt, src_bi, src_bits, mask_bi, mask_bits);
    SetWorldTransform(ctx->hdc, &xform_dest);
    return TRUE;
}

static inline int get_dib_stride( int width, int bpp )
{
    return ((width * bpp + 31) >> 3) & ~3;
}

static inline int get_dib_image_size( const BITMAPINFO *info )
{
    return get_dib_stride( info->bmiHeader.biWidth, info->bmiHeader.biBitCount )
        * abs( info->bmiHeader.biHeight );
}

static int set_di_bits_to_device(print_ctx *ctx, const EMRSETDIBITSTODEVICE *p)
{
    const BITMAPINFO *info = (const BITMAPINFO *)((BYTE *)p + p->offBmiSrc);
    char bi_buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *bi = (BITMAPINFO *)bi_buffer;
    HBITMAP bitmap, old_bitmap;
    int width, height, ret;
    BYTE *bits;

    width = min(p->cxSrc, info->bmiHeader.biWidth);
    height = min(p->cySrc, abs(info->bmiHeader.biHeight));

    memset(bi_buffer, 0, sizeof(bi_buffer));
    bi->bmiHeader.biSize = sizeof(bi->bmiHeader);
    bi->bmiHeader.biWidth = width;
    bi->bmiHeader.biHeight = height;
    bi->bmiHeader.biPlanes = 1;
    if (p->iUsageSrc == DIB_PAL_COLORS && (info->bmiHeader.biBitCount == 1 ||
                info->bmiHeader.biBitCount == 4 || info->bmiHeader.biBitCount == 8))
    {
        PALETTEENTRY pal[256];
        HPALETTE hpal;
        UINT i, size;

        bi->bmiHeader.biBitCount = info->bmiHeader.biBitCount;
        bi->bmiHeader.biClrUsed = 1 << info->bmiHeader.biBitCount;
        bi->bmiHeader.biClrImportant = bi->bmiHeader.biClrUsed;

        hpal = GetCurrentObject(ctx->hdc, OBJ_PAL);
        size = GetPaletteEntries(hpal, 0, bi->bmiHeader.biClrUsed, pal);
        for (i = 0; i < size; i++)
        {
            bi->bmiColors[i].rgbBlue = pal[i].peBlue;
            bi->bmiColors[i].rgbGreen = pal[i].peGreen;
            bi->bmiColors[i].rgbRed = pal[i].peRed;
        }
    }
    else
    {
        bi->bmiHeader.biBitCount = 24;
    }
    bi->bmiHeader.biCompression = BI_RGB;
    bitmap = CreateDIBSection(ctx->hdc, bi, DIB_RGB_COLORS, (void **)&bits, NULL, 0);
    if (!bitmap)
        return 1;
    old_bitmap = SelectObject(ctx->hdc, bitmap);

    ret = SetDIBitsToDevice(ctx->hdc, 0, 0, width, height, p->xSrc, p->ySrc,
            p->iStartScan, p->cScans, (const BYTE*)p + p->offBitsSrc, info, p->iUsageSrc);
    SelectObject(ctx->hdc, old_bitmap);
    if (ret)
    {
        EMRSTRETCHBLT blt;

        memset(&blt, 0, sizeof(blt));
        blt.rclBounds = p->rclBounds;
        blt.xDest = p->xDest;
        blt.yDest = p->yDest + p->cySrc - height;
        blt.cxDest = width;
        blt.cyDest = ret;
        blt.dwRop = SRCCOPY;
        blt.xformSrc.eM11 = 1;
        blt.xformSrc.eM22 = 1;
        blt.iUsageSrc = DIB_RGB_COLORS;
        blt.cbBmiSrc = sizeof(bi_buffer);
        blt.cbBitsSrc = get_dib_image_size(bi);
        blt.cxSrc = blt.cxDest;
        blt.cySrc = blt.cyDest;
        stretch_blt(ctx, &blt, bi, bits);
    }

    DeleteObject(bitmap);
    return 1;
}

static int stretch_di_bits(print_ctx *ctx, const EMRSTRETCHDIBITS *p)
{
    char bi_buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    const BYTE *bits = (BYTE *)p + p->offBitsSrc;
    BITMAPINFO *bi = (BITMAPINFO *)bi_buffer;
    EMRSTRETCHBLT blt;

    memcpy(bi, (BYTE *)p + p->offBmiSrc, p->cbBmiSrc);
    memset(bi_buffer + p->cbBmiSrc, 0, sizeof(bi_buffer) - p->cbBmiSrc);

    if (p->iUsageSrc == DIB_PAL_COLORS && (bi->bmiHeader.biBitCount == 1 ||
                bi->bmiHeader.biBitCount == 4 || bi->bmiHeader.biBitCount == 8))
    {
        PALETTEENTRY pal[256];
        HPALETTE hpal;
        UINT i, size;

        hpal = GetCurrentObject(ctx->hdc, OBJ_PAL);
        size = GetPaletteEntries(hpal, 0, 1 << bi->bmiHeader.biBitCount, pal);
        for (i = 0; i < size; i++)
        {
            bi->bmiColors[i].rgbBlue = pal[i].peBlue;
            bi->bmiColors[i].rgbGreen = pal[i].peGreen;
            bi->bmiColors[i].rgbRed = pal[i].peRed;
        }
    }

    memset(&blt, 0, sizeof(blt));
    blt.rclBounds = p->rclBounds;
    blt.xDest = p->xDest;
    blt.yDest = p->yDest;
    blt.cxDest = p->cxDest;
    blt.cyDest = p->cyDest;
    blt.dwRop = p->dwRop;
    blt.xSrc = p->xSrc;
    blt.ySrc = abs(bi->bmiHeader.biHeight) - p->ySrc - p->cySrc;
    blt.xformSrc.eM11 = 1;
    blt.xformSrc.eM22 = 1;
    blt.iUsageSrc = p->iUsageSrc;
    blt.cbBmiSrc = sizeof(bi_buffer);
    blt.cbBitsSrc = p->cbBitsSrc;
    blt.cxSrc = p->cxSrc;
    blt.cySrc = p->cySrc;
    return stretch_blt(ctx, &blt, bi, bits);
}

static int poly_draw(print_ctx *ctx, const POINT *points, const BYTE *types, DWORD count)
{
    POINT first, cur, pts[4];
    DWORD i, num_pts;

    /* check for valid point types */
    for (i = 0; i < count; i++)
    {
        switch (types[i])
        {
        case PT_MOVETO:
        case PT_LINETO | PT_CLOSEFIGURE:
        case PT_LINETO:
            break;
        case PT_BEZIERTO:
            if (i + 2 >= count) return FALSE;
            if (types[i + 1] != PT_BEZIERTO) return FALSE;
            if ((types[i + 2] & ~PT_CLOSEFIGURE) != PT_BEZIERTO) return FALSE;
            i += 2;
            break;
        default:
            return FALSE;
        }
    }

    GetCurrentPositionEx(ctx->hdc, &cur);
    first = cur;

    for (i = 0; i < count; i++)
    {
        switch (types[i])
        {
        case PT_MOVETO:
            first = points[i];
            break;
        case PT_LINETO:
        case (PT_LINETO | PT_CLOSEFIGURE):
            pts[0] = cur;
            pts[1] = points[i];
            num_pts = 2;
            if (!PSDRV_PolyPolyline(ctx, pts, &num_pts, 1))
                return FALSE;
            break;
        case PT_BEZIERTO:
            pts[0] = cur;
            pts[1] = points[i];
            pts[2] = points[i + 1];
            pts[3] = points[i + 2];
            if (!PSDRV_PolyBezier(ctx, pts, 4))
                return FALSE;
            i += 2;
            break;
        }

        cur = points[i];
        if (types[i] & PT_CLOSEFIGURE)
        {
            pts[0] = cur;
            pts[1] = first;
            num_pts = 2;
            if (!PSDRV_PolyPolyline(ctx, pts, &num_pts, 1))
                return FALSE;
        }
    }

    return TRUE;
}

static inline void reset_bounds(RECT *bounds)
{
    bounds->left = bounds->top = INT_MAX;
    bounds->right = bounds->bottom = INT_MIN;
}

static BOOL gradient_fill(print_ctx *ctx, const TRIVERTEX *vert_array, DWORD nvert,
        const void *grad_array, DWORD ngrad, ULONG mode)
{
    char buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    struct ps_bitblt_coords src, dst;
    struct ps_image_bits bits;
    HBITMAP bmp, old_bmp;
    BOOL ret = FALSE;
    TRIVERTEX *pts;
    unsigned int i;
    HDC hdc_src;
    HRGN rgn;

    if (!(pts = malloc(nvert * sizeof(*pts)))) return FALSE;
    memcpy(pts, vert_array, sizeof(*pts) * nvert);
    for (i = 0; i < nvert; i++)
        LPtoDP(ctx->hdc, (POINT *)&pts[i], 1);

    /* compute bounding rect of all the rectangles/triangles */
    reset_bounds(&dst.visrect);
    for (i = 0; i < ngrad * (mode == GRADIENT_FILL_TRIANGLE ? 3 : 2); i++)
    {
        ULONG v = ((ULONG *)grad_array)[i];
        dst.visrect.left   = min(dst.visrect.left,   pts[v].x);
        dst.visrect.top    = min(dst.visrect.top,    pts[v].y);
        dst.visrect.right  = max(dst.visrect.right,  pts[v].x);
        dst.visrect.bottom = max(dst.visrect.bottom, pts[v].y);
    }

    dst.x = dst.visrect.left;
    dst.y = dst.visrect.top;
    dst.width = dst.visrect.right - dst.visrect.left;
    dst.height = dst.visrect.bottom - dst.visrect.top;
    clip_visrect(ctx->hdc, &dst.visrect, &dst.visrect);

    info->bmiHeader.biSize          = sizeof(info->bmiHeader);
    info->bmiHeader.biPlanes        = 1;
    info->bmiHeader.biBitCount      = 24;
    info->bmiHeader.biCompression   = BI_RGB;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed       = 0;
    info->bmiHeader.biClrImportant  = 0;
    info->bmiHeader.biWidth         = dst.visrect.right - dst.visrect.left;
    info->bmiHeader.biHeight        = dst.visrect.bottom - dst.visrect.top;
    info->bmiHeader.biSizeImage     = 0;
    memset(&bits, 0, sizeof(bits));
    hdc_src = CreateCompatibleDC(NULL);
    if (!hdc_src)
    {
        free(pts);
        return FALSE;
    }
    bmp = CreateDIBSection(hdc_src, info, DIB_RGB_COLORS, &bits.ptr, NULL, 0);
    if (!bmp)
    {
        DeleteObject(hdc_src);
        free(pts);
        return FALSE;
    }
    old_bmp = SelectObject(hdc_src, bmp);

    /* make src and points relative to the bitmap */
    src = dst;
    src.x -= dst.visrect.left;
    src.y -= dst.visrect.top;
    OffsetRect(&src.visrect, -dst.visrect.left, -dst.visrect.top);
    for (i = 0; i < nvert; i++)
    {
        pts[i].x -= dst.visrect.left;
        pts[i].y -= dst.visrect.top;
    }
    ret = GdiGradientFill(hdc_src, pts, nvert, (void *)grad_array, ngrad, mode);
    SelectObject(hdc_src, old_bmp);
    DeleteObject(hdc_src);

    rgn = CreateRectRgn(0, 0, 0, 0);
    if (mode == GRADIENT_FILL_TRIANGLE)
    {
        const GRADIENT_TRIANGLE *gt = grad_array;
        POINT triangle[3];
        HRGN tmp;

        for (i = 0; i < ngrad; i++)
        {
            triangle[0].x = pts[gt[i].Vertex1].x;
            triangle[0].y = pts[gt[i].Vertex1].y;
            triangle[1].x = pts[gt[i].Vertex2].x;
            triangle[1].y = pts[gt[i].Vertex2].y;
            triangle[2].x = pts[gt[i].Vertex3].x;
            triangle[2].y = pts[gt[i].Vertex3].y;
            tmp = CreatePolygonRgn(triangle, 3, ALTERNATE);
            CombineRgn(rgn, rgn, tmp, RGN_OR);
            DeleteObject(tmp);
        }
    }
    else
    {
        const GRADIENT_RECT *gr = grad_array;
        HRGN tmp = CreateRectRgn(0, 0, 0, 0);

        for (i = 0; i < ngrad; i++)
        {
            SetRectRgn(tmp, pts[gr[i].UpperLeft].x, pts[gr[i].UpperLeft].y,
                    pts[gr[i].LowerRight].x, pts[gr[i].LowerRight].y);
            CombineRgn(rgn, rgn, tmp, RGN_OR);
        }
        DeleteObject(tmp);
    }
    free(pts);

    OffsetRgn(rgn, dst.visrect.left, dst.visrect.top);
    if (ret)
        ret = (PSDRV_PutImage(ctx, rgn, info, &bits, &src, &dst, SRCCOPY) == ERROR_SUCCESS);
    DeleteObject(rgn);
    DeleteObject(bmp);
    return ret;
}

static HGDIOBJ get_object_handle(struct pp_data *data, HANDLETABLE *handletable,
        DWORD i, struct ps_brush_pattern **pattern)
{
    if (i & 0x80000000)
    {
        *pattern = NULL;
        return GetStockObject(i & 0x7fffffff);
    }
    *pattern = data->patterns + i;
    return handletable->objectHandle[i];
}

static BOOL select_hbrush(struct pp_data *data, HANDLETABLE *htable, int handle_count, HBRUSH brush)
{
    struct ps_brush_pattern *pattern = NULL;
    int i;

    for (i = 0; i < handle_count; i++)
    {
        if (htable->objectHandle[i] == brush)
        {
            pattern = data->patterns + i;
            break;
        }
    }

    return PSDRV_SelectBrush(data->ctx, brush, pattern) != NULL;
}

/* Performs a device to world transformation on the specified size (which
 * is in integer format).
 */
static inline INT INTERNAL_YWSTODS(HDC hdc, INT height)
{
    POINT pt[2];
    pt[0].x = pt[0].y = 0;
    pt[1].x = 0;
    pt[1].y = height;
    LPtoDP(hdc, pt, 2);
    return pt[1].y - pt[0].y;
}

extern const unsigned short bidi_direction_table[];

/*------------------------------------------------------------------------
    Bidirectional Character Types

    as defined by the Unicode Bidirectional Algorithm Table 3-7.

    Note:

      The list of bidirectional character types here is not grouped the
      same way as the table 3-7, since the numeric values for the types
      are chosen to keep the state and action tables compact.
------------------------------------------------------------------------*/
enum directions
{
    /* input types */
             /* ON MUST be zero, code relies on ON = N = 0 */
    ON = 0,  /* Other Neutral */
    L,       /* Left Letter */
    R,       /* Right Letter */
    AN,      /* Arabic Number */
    EN,      /* European Number */
    AL,      /* Arabic Letter (Right-to-left) */
    NSM,     /* Non-spacing Mark */
    CS,      /* Common Separator */
    ES,      /* European Separator */
    ET,      /* European Terminator (post/prefix e.g. $ and %) */

    /* resolved types */
    BN,      /* Boundary neutral (type of RLE etc after explicit levels) */

    /* input types, */
    S,       /* Segment Separator (TAB)        // used only in L1 */
    WS,      /* White space                    // used only in L1 */
    B,       /* Paragraph Separator (aka as PS) */

    /* types for explicit controls */
    RLO,     /* these are used only in X1-X9 */
    RLE,
    LRO,
    LRE,
    PDF,

    LRI, /* Isolate formatting characters new with 6.3 */
    RLI,
    FSI,
    PDI,

    /* resolved types, also resolved directions */
    NI = ON,  /* alias, where ON, WS and S are treated the same */
};

static inline unsigned short get_table_entry_32(const unsigned short *table, UINT ch)
{
    return table[table[table[table[ch >> 12] + ((ch >> 8) & 0x0f)] + ((ch >> 4) & 0x0f)] + (ch & 0xf)];
}

/* Convert the libwine information to the direction enum */
static void classify(LPCWSTR lpString, WORD *chartype, DWORD uCount)
{
    unsigned i;

    for (i = 0; i < uCount; ++i)
        chartype[i] = get_table_entry_32(bidi_direction_table, lpString[i]);
}

/* Set a run of cval values at locations all prior to, but not including */
/* iStart, to the new value nval. */
static void SetDeferredRun(BYTE *pval, int cval, int iStart, int nval)
{
    int i = iStart - 1;
    for (; i >= iStart - cval; i--)
    {
        pval[i] = nval;
    }
}

/* THE PARAGRAPH LEVEL */

/*------------------------------------------------------------------------
    Function: resolveParagraphs

    Resolves the input strings into blocks over which the algorithm
    is then applied.

    Implements Rule P1 of the Unicode Bidi Algorithm

    Input: Text string
           Character count

    Output: revised character count

    Note:    This is a very simplistic function. In effect it restricts
            the action of the algorithm to the first paragraph in the input
            where a paragraph ends at the end of the first block separator
            or at the end of the input text.

------------------------------------------------------------------------*/

static int resolveParagraphs(WORD *types, int cch)
{
    /* skip characters not of type B */
    int ich = 0;
    for(; ich < cch && types[ich] != B; ich++);
    /* stop after first B, make it a BN for use in the next steps */
    if (ich < cch && types[ich] == B)
        types[ich++] = BN;
    return ich;
}

/* REORDER */
/*------------------------------------------------------------------------
    Function: resolveLines

    Breaks a paragraph into lines

    Input:  Array of line break flags
            Character count
    In/Out: Array of characters

    Returns the count of characters on the first line

    Note: This function only breaks lines at hard line breaks. Other
    line breaks can be passed in. If pbrk[n] is TRUE, then a break
    occurs after the character in pszInput[n]. Breaks before the first
    character are not allowed.
------------------------------------------------------------------------*/
static int resolveLines(LPCWSTR pszInput, const BOOL * pbrk, int cch)
{
    /* skip characters not of type LS */
    int ich = 0;
    for(; ich < cch; ich++)
    {
        if (pszInput[ich] == (WCHAR)'\n' || (pbrk && pbrk[ich]))
        {
            ich++;
            break;
        }
    }

    return ich;
}

/*------------------------------------------------------------------------
    Function: resolveWhiteSpace

    Resolves levels for WS and S
    Implements rule L1 of the Unicode bidi Algorithm.

    Input:  Base embedding level
            Character count
            Array of direction classes (for one line of text)

    In/Out: Array of embedding levels (for one line of text)

    Note: this should be applied a line at a time. The default driver
          code supplied in this file assumes a single line of text; for
          a real implementation, cch and the initial pointer values
          would have to be adjusted.
------------------------------------------------------------------------*/
static void resolveWhitespace(int baselevel, const WORD *pcls, BYTE *plevel, int cch)
{
    int cchrun = 0;
    BYTE oldlevel = baselevel;

    int ich = 0;
    for (; ich < cch; ich++)
    {
        switch(pcls[ich])
        {
        default:
            cchrun = 0; /* any other character breaks the run */
            break;
        case WS:
            cchrun++;
            break;

        case RLE:
        case LRE:
        case LRO:
        case RLO:
        case PDF:
        case LRI:
        case RLI:
        case FSI:
        case PDI:
        case BN:
            plevel[ich] = oldlevel;
            cchrun++;
            break;

        case S:
        case B:
            /* reset levels for WS before eot */
            SetDeferredRun(plevel, cchrun, ich, baselevel);
            cchrun = 0;
            plevel[ich] = baselevel;
            break;
        }
        oldlevel = plevel[ich];
    }
    /* reset level before eot */
    SetDeferredRun(plevel, cchrun, ich, baselevel);
}

/*------------------------------------------------------------------------
    Function: BidiLines

    Implements the Line-by-Line phases of the Unicode Bidi Algorithm

      Input:     Count of characters
                 Array of character directions

    Inp/Out: Input text
             Array of levels

------------------------------------------------------------------------*/
static void BidiLines(int baselevel, LPWSTR pszOutLine, LPCWSTR pszLine, const WORD * pclsLine,
                      BYTE * plevelLine, int cchPara, const BOOL * pbrk)
{
    int cchLine = 0;
    int done = 0;
    int *run;

    run = HeapAlloc(GetProcessHeap(), 0, cchPara * sizeof(int));
    if (!run)
    {
        WARN("Out of memory\n");
        return;
    }

    do
    {
        /* break lines at LS */
        cchLine = resolveLines(pszLine, pbrk, cchPara);

        /* resolve whitespace */
        resolveWhitespace(baselevel, pclsLine, plevelLine, cchLine);

        if (pszOutLine)
        {
            int i;
            /* reorder each line in place */
            ScriptLayout(cchLine, plevelLine, NULL, run);
            for (i = 0; i < cchLine; i++)
                pszOutLine[done+run[i]] = pszLine[i];
        }

        pszLine += cchLine;
        plevelLine += cchLine;
        pbrk += pbrk ? cchLine : 0;
        pclsLine += cchLine;
        cchPara -= cchLine;
        done += cchLine;

    } while (cchPara);

    HeapFree(GetProcessHeap(), 0, run);
}

#define WINE_GCPW_FORCE_LTR 0
#define WINE_GCPW_FORCE_RTL 1
#define WINE_GCPW_DIR_MASK 3

static BOOL BIDI_Reorder(HDC hDC,               /* [in] Display DC */
                         LPCWSTR lpString,      /* [in] The string for which information is to be returned */
                         INT uCount,            /* [in] Number of WCHARs in string. */
                         DWORD dwFlags,         /* [in] GetCharacterPlacement compatible flags */
                         DWORD dwWineGCP_Flags, /* [in] Wine internal flags - Force paragraph direction */
                         LPWSTR lpOutString,    /* [out] Reordered string */
                         INT uCountOut,         /* [in] Size of output buffer */
                         UINT *lpOrder,         /* [out] Logical -> Visual order map */
                         WORD **lpGlyphs,       /* [out] reordered, mirrored, shaped glyphs to display */
                         INT *cGlyphs)         /* [out] number of glyphs generated */
{
    WORD *chartype = NULL;
    BYTE *levels = NULL;
    INT i, done;
    unsigned glyph_i;
    BOOL is_complex, ret = FALSE;

    int maxItems;
    int nItems;
    SCRIPT_CONTROL Control;
    SCRIPT_STATE State;
    SCRIPT_ITEM *pItems = NULL;
    HRESULT res;
    SCRIPT_CACHE psc = NULL;
    WORD *run_glyphs = NULL;
    WORD *pwLogClust = NULL;
    SCRIPT_VISATTR *psva = NULL;
    DWORD cMaxGlyphs = 0;
    BOOL  doGlyphs = TRUE;

    TRACE("%s, %d, 0x%08lx lpOutString=%p, lpOrder=%p\n",
          debugstr_wn(lpString, uCount), uCount, dwFlags,
          lpOutString, lpOrder);

    memset(&Control, 0, sizeof(Control));
    memset(&State, 0, sizeof(State));
    if (lpGlyphs)
        *lpGlyphs = NULL;

    if (!(dwFlags & GCP_REORDER))
    {
        FIXME("Asked to reorder without reorder flag set\n");
        return FALSE;
    }

    if (lpOutString && uCountOut < uCount)
    {
        FIXME("lpOutString too small\n");
        return FALSE;
    }

    chartype = HeapAlloc(GetProcessHeap(), 0, uCount * sizeof(WORD));
    if (!chartype)
    {
        WARN("Out of memory\n");
        return FALSE;
    }

    if (lpOutString)
        memcpy(lpOutString, lpString, uCount * sizeof(WCHAR));

    is_complex = FALSE;
    for (i = 0; i < uCount && !is_complex; i++)
    {
        if ((lpString[i] >= 0x900 && lpString[i] <= 0xfff) ||
            (lpString[i] >= 0x1cd0 && lpString[i] <= 0x1cff) ||
            (lpString[i] >= 0xa840 && lpString[i] <= 0xa8ff))
            is_complex = TRUE;
    }

    /* Verify reordering will be required */
    if (WINE_GCPW_FORCE_RTL == (dwWineGCP_Flags & WINE_GCPW_DIR_MASK))
        State.uBidiLevel = 1;
    else if (!is_complex)
    {
        done = 1;
        classify(lpString, chartype, uCount);
        for (i = 0; i < uCount; i++)
            switch (chartype[i])
            {
                case R:
                case AL:
                case RLE:
                case RLO:
                    done = 0;
                    break;
            }
        if (done)
        {
            HeapFree(GetProcessHeap(), 0, chartype);
            if (lpOrder)
            {
                for (i = 0; i < uCount; i++)
                    lpOrder[i] = i;
            }
            return TRUE;
        }
    }

    levels = HeapAlloc(GetProcessHeap(), 0, uCount * sizeof(BYTE));
    if (!levels)
    {
        WARN("Out of memory\n");
        goto cleanup;
    }

    maxItems = 5;
    pItems = HeapAlloc(GetProcessHeap(),0, maxItems * sizeof(SCRIPT_ITEM));
    if (!pItems)
    {
        WARN("Out of memory\n");
        goto cleanup;
    }

    if (lpGlyphs)
    {
        cMaxGlyphs = 1.5 * uCount + 16;
        run_glyphs = HeapAlloc(GetProcessHeap(),0,sizeof(WORD) * cMaxGlyphs);
        if (!run_glyphs)
        {
            WARN("Out of memory\n");
            goto cleanup;
        }
        pwLogClust = HeapAlloc(GetProcessHeap(),0,sizeof(WORD) * uCount);
        if (!pwLogClust)
        {
            WARN("Out of memory\n");
            goto cleanup;
        }
        psva = HeapAlloc(GetProcessHeap(),0,sizeof(SCRIPT_VISATTR) * cMaxGlyphs);
        if (!psva)
        {
            WARN("Out of memory\n");
            goto cleanup;
        }
    }

    done = 0;
    glyph_i = 0;
    while (done < uCount)
    {
        INT j;
        classify(lpString + done, chartype, uCount - done);
        /* limit text to first block */
        i = resolveParagraphs(chartype, uCount - done);
        for (j = 0; j < i; ++j)
            switch(chartype[j])
            {
                case B:
                case S:
                case WS:
                case ON: chartype[j] = NI;
                default: continue;
            }

        res = ScriptItemize(lpString + done, i, maxItems, &Control, &State, pItems, &nItems);
        while (res == E_OUTOFMEMORY)
        {
            SCRIPT_ITEM *new_pItems = HeapReAlloc(GetProcessHeap(), 0, pItems, sizeof(*pItems) * maxItems * 2);
            if (!new_pItems)
            {
                WARN("Out of memory\n");
                goto cleanup;
            }
            pItems = new_pItems;
            maxItems *= 2;
            res = ScriptItemize(lpString + done, i, maxItems, &Control, &State, pItems, &nItems);
        }

        if (lpOutString || lpOrder)
            for (j = 0; j < nItems; j++)
            {
                int k;
                for (k = pItems[j].iCharPos; k < pItems[j+1].iCharPos; k++)
                    levels[k] = pItems[j].a.s.uBidiLevel;
            }

        if (lpOutString)
        {
            /* assign directional types again, but for WS, S this time */
            classify(lpString + done, chartype, i);

            BidiLines(State.uBidiLevel, lpOutString + done, lpString + done,
                        chartype, levels, i, 0);
        }

        if (lpOrder)
        {
            int k, lastgood;
            for (j = lastgood = 0; j < i; ++j)
                if (levels[j] != levels[lastgood])
                {
                    --j;
                    if (levels[lastgood] & 1)
                        for (k = j; k >= lastgood; --k)
                            lpOrder[done + k] = done + j - k;
                    else
                        for (k = lastgood; k <= j; ++k)
                            lpOrder[done + k] = done + k;
                    lastgood = ++j;
                }
            if (levels[lastgood] & 1)
                for (k = j - 1; k >= lastgood; --k)
                    lpOrder[done + k] = done + j - 1 - k;
            else
                for (k = lastgood; k < j; ++k)
                    lpOrder[done + k] = done + k;
        }

        if (lpGlyphs && doGlyphs)
        {
            BYTE *runOrder;
            int *visOrder;
            SCRIPT_ITEM *curItem;

            runOrder = HeapAlloc(GetProcessHeap(), 0, maxItems * sizeof(*runOrder));
            visOrder = HeapAlloc(GetProcessHeap(), 0, maxItems * sizeof(*visOrder));
            if (!runOrder || !visOrder)
            {
                WARN("Out of memory\n");
                HeapFree(GetProcessHeap(), 0, runOrder);
                HeapFree(GetProcessHeap(), 0, visOrder);
                goto cleanup;
            }

            for (j = 0; j < nItems; j++)
                runOrder[j] = pItems[j].a.s.uBidiLevel;

            ScriptLayout(nItems, runOrder, visOrder, NULL);

            for (j = 0; j < nItems; j++)
            {
                int k;
                int cChars,cOutGlyphs;
                curItem = &pItems[visOrder[j]];

                cChars = pItems[visOrder[j]+1].iCharPos - curItem->iCharPos;

                res = ScriptShape(hDC, &psc, lpString + done + curItem->iCharPos, cChars, cMaxGlyphs, &curItem->a, run_glyphs, pwLogClust, psva, &cOutGlyphs);
                while (res == E_OUTOFMEMORY)
                {
                    WORD *new_run_glyphs = HeapReAlloc(GetProcessHeap(), 0, run_glyphs, sizeof(*run_glyphs) * cMaxGlyphs * 2);
                    SCRIPT_VISATTR *new_psva = HeapReAlloc(GetProcessHeap(), 0, psva, sizeof(*psva) * cMaxGlyphs * 2);
                    if (!new_run_glyphs || !new_psva)
                    {
                        WARN("Out of memory\n");
                        HeapFree(GetProcessHeap(), 0, runOrder);
                        HeapFree(GetProcessHeap(), 0, visOrder);
                        HeapFree(GetProcessHeap(), 0, *lpGlyphs);
                        *lpGlyphs = NULL;
                        if (new_run_glyphs)
                            run_glyphs = new_run_glyphs;
                        if (new_psva)
                            psva = new_psva;
                        goto cleanup;
                    }
                    run_glyphs = new_run_glyphs;
                    psva = new_psva;
                    cMaxGlyphs *= 2;
                    res = ScriptShape(hDC, &psc, lpString + done + curItem->iCharPos, cChars, cMaxGlyphs, &curItem->a, run_glyphs, pwLogClust, psva, &cOutGlyphs);
                }
                if (res)
                {
                    if (res == USP_E_SCRIPT_NOT_IN_FONT)
                        TRACE("Unable to shape with currently selected font\n");
                    else
                        FIXME("Unable to shape string (%lx)\n",res);
                    j = nItems;
                    doGlyphs = FALSE;
                    HeapFree(GetProcessHeap(), 0, *lpGlyphs);
                    *lpGlyphs = NULL;
                }
                else
                {
                    WORD *new_glyphs;
                    if (*lpGlyphs)
                        new_glyphs = HeapReAlloc(GetProcessHeap(), 0, *lpGlyphs, sizeof(**lpGlyphs) * (glyph_i + cOutGlyphs));
                   else
                        new_glyphs = HeapAlloc(GetProcessHeap(), 0, sizeof(**lpGlyphs) * (glyph_i + cOutGlyphs));
                    if (!new_glyphs)
                    {
                        WARN("Out of memory\n");
                        HeapFree(GetProcessHeap(), 0, runOrder);
                        HeapFree(GetProcessHeap(), 0, visOrder);
                        HeapFree(GetProcessHeap(), 0, *lpGlyphs);
                        *lpGlyphs = NULL;
                        goto cleanup;
                    }
                    *lpGlyphs = new_glyphs;
                    for (k = 0; k < cOutGlyphs; k++)
                        (*lpGlyphs)[glyph_i+k] = run_glyphs[k];
                    glyph_i += cOutGlyphs;
                }
            }
            HeapFree(GetProcessHeap(), 0, runOrder);
            HeapFree(GetProcessHeap(), 0, visOrder);
        }

        done += i;
    }
    if (cGlyphs)
        *cGlyphs = glyph_i;

    ret = TRUE;
cleanup:
    HeapFree(GetProcessHeap(), 0, chartype);
    HeapFree(GetProcessHeap(), 0, levels);
    HeapFree(GetProcessHeap(), 0, pItems);
    HeapFree(GetProcessHeap(), 0, run_glyphs);
    HeapFree(GetProcessHeap(), 0, pwLogClust);
    HeapFree(GetProcessHeap(), 0, psva);
    ScriptFreeCache(&psc);
    return ret;
}

static inline BOOL intersect_rect(RECT *dst, const RECT *src1, const RECT *src2)
{
    dst->left   = max(src1->left, src2->left);
    dst->top    = max(src1->top, src2->top);
    dst->right  = min(src1->right, src2->right);
    dst->bottom = min(src1->bottom, src2->bottom);
    return !IsRectEmpty(dst);
}

/***********************************************************************
 *           get_line_width
 *
 * Scale the underline / strikeout line width.
 */
static inline int get_line_width(HDC hdc, int metric_size)
{
    int width = abs(INTERNAL_YWSTODS(hdc, metric_size));
    if (width == 0) width = 1;
    if (metric_size < 0) width = -width;
    return width;
}

static BOOL ext_text_out(struct pp_data *data, HANDLETABLE *htable,
        int handle_count, INT x, INT y, UINT flags, const RECT *rect,
        const WCHAR *str, UINT count, const INT *dx)
{
    HDC hdc = data->ctx->hdc;
    BOOL ret = FALSE;
    UINT align;
    DWORD layout;
    POINT pt;
    TEXTMETRICW tm;
    LOGFONTW lf;
    double cosEsc, sinEsc;
    INT char_extra;
    SIZE sz;
    RECT rc;
    POINT *deltas = NULL, width = {0, 0};
    INT breakRem;
    WORD *glyphs = NULL;
    XFORM xform;

    if (!(flags & (ETO_GLYPH_INDEX | ETO_IGNORELANGUAGE)) && count > 0)
    {
        int glyphs_count = 0;
        UINT bidi_flags;

        bidi_flags = (GetTextAlign(hdc) & TA_RTLREADING) || (flags & ETO_RTLREADING)
            ? WINE_GCPW_FORCE_RTL : WINE_GCPW_FORCE_LTR;

        BIDI_Reorder(hdc, str, count, GCP_REORDER, bidi_flags,
                NULL, 0, NULL, &glyphs, &glyphs_count);

        flags |= ETO_IGNORELANGUAGE;
        if (glyphs)
        {
            flags |= ETO_GLYPH_INDEX;
            count = glyphs_count;
            str = glyphs;
        }
    }

    align = GetTextAlign(hdc);
    breakRem = data->break_rem;
    layout = GetLayout(hdc);

    if (flags & ETO_RTLREADING) align |= TA_RTLREADING;
    if (layout & LAYOUT_RTL)
    {
        if ((align & TA_CENTER) != TA_CENTER) align ^= TA_RIGHT;
        align ^= TA_RTLREADING;
    }

    TRACE("%d, %d, %08x, %s, %s, %d, %p)\n", x, y, flags,
            wine_dbgstr_rect(rect), debugstr_wn(str, count), count, dx);
    TRACE("align = %x bkmode = %x mapmode = %x\n", align, GetBkMode(hdc),
            GetMapMode(hdc));

    if(align & TA_UPDATECP)
    {
        GetCurrentPositionEx(hdc, &pt);
        x = pt.x;
        y = pt.y;
    }

    GetTextMetricsW(hdc, &tm);
    GetObjectW(GetCurrentObject(hdc, OBJ_FONT), sizeof(lf), &lf);

    if(!(tm.tmPitchAndFamily & TMPF_VECTOR)) /* Non-scalable fonts shouldn't be rotated */
        lf.lfEscapement = 0;

    GetWorldTransform(hdc, &xform);
    if (GetGraphicsMode(hdc) == GM_COMPATIBLE &&
            xform.eM11 * xform.eM22 < 0)
    {
        lf.lfEscapement = -lf.lfEscapement;
    }

    if(lf.lfEscapement != 0)
    {
        cosEsc = cos(lf.lfEscapement * M_PI / 1800);
        sinEsc = sin(lf.lfEscapement * M_PI / 1800);
    }
    else
    {
        cosEsc = 1;
        sinEsc = 0;
    }

    if (rect && (flags & (ETO_OPAQUE | ETO_CLIPPED)))
    {
        rc = *rect;
        LPtoDP(hdc, (POINT*)&rc, 2);
        order_rect(&rc);
        if (flags & ETO_OPAQUE)
            PSDRV_ExtTextOut(data->ctx, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    }
    else flags &= ~ETO_CLIPPED;

    if(count == 0)
    {
        ret = TRUE;
        goto done;
    }

    pt.x = x;
    pt.y = y;
    LPtoDP(hdc, &pt, 1);
    x = pt.x;
    y = pt.y;

    char_extra = GetTextCharacterExtra(hdc);
    if (char_extra && dx)
        char_extra = 0; /* Printer drivers don't add char_extra if dx is supplied */

    if(char_extra || data->break_extra || breakRem || dx || lf.lfEscapement != 0)
    {
        UINT i;
        POINT total = {0, 0}, desired[2];

        deltas = malloc(count * sizeof(*deltas));
        if (dx)
        {
            if (flags & ETO_PDY)
            {
                for (i = 0; i < count; i++)
                {
                    deltas[i].x = dx[i * 2] + char_extra;
                    deltas[i].y = -dx[i * 2 + 1];
                }
            }
            else
            {
                for (i = 0; i < count; i++)
                {
                    deltas[i].x = dx[i] + char_extra;
                    deltas[i].y = 0;
                }
            }
        }
        else
        {
            INT *dx = malloc(count * sizeof(*dx));

            NtGdiGetTextExtentExW(hdc, str, count, -1, NULL, dx, &sz, !!(flags & ETO_GLYPH_INDEX));

            deltas[0].x = dx[0];
            deltas[0].y = 0;
            for (i = 1; i < count; i++)
            {
                deltas[i].x = dx[i] - dx[i - 1];
                deltas[i].y = 0;
            }
            free(dx);
        }

        for(i = 0; i < count; i++)
        {
            total.x += deltas[i].x;
            total.y += deltas[i].y;

            desired[0].x = desired[0].y = 0;

            desired[1].x =  cosEsc * total.x + sinEsc * total.y;
            desired[1].y = -sinEsc * total.x + cosEsc * total.y;

            LPtoDP(hdc, desired, 2);
            desired[1].x -= desired[0].x;
            desired[1].y -= desired[0].y;

            if (GetGraphicsMode(hdc) == GM_COMPATIBLE)
            {
                if (xform.eM11 < 0)
                    desired[1].x = -desired[1].x;
                if (xform.eM22 < 0)
                    desired[1].y = -desired[1].y;
            }

            deltas[i].x = desired[1].x - width.x;
            deltas[i].y = desired[1].y - width.y;

            width = desired[1];
        }
        flags |= ETO_PDY;
    }
    else
    {
        POINT desired[2];

        NtGdiGetTextExtentExW(hdc, str, count, 0, NULL, NULL, &sz, !!(flags & ETO_GLYPH_INDEX));
        desired[0].x = desired[0].y = 0;
        desired[1].x = sz.cx;
        desired[1].y = 0;
        LPtoDP(hdc, desired, 2);
        desired[1].x -= desired[0].x;
        desired[1].y -= desired[0].y;

        if (GetGraphicsMode(hdc) == GM_COMPATIBLE)
        {
            if (xform.eM11 < 0)
                desired[1].x = -desired[1].x;
            if (xform.eM22 < 0)
                desired[1].y = -desired[1].y;
        }
        width = desired[1];
    }

    tm.tmAscent = abs(INTERNAL_YWSTODS(hdc, tm.tmAscent));
    tm.tmDescent = abs(INTERNAL_YWSTODS(hdc, tm.tmDescent));
    switch(align & (TA_LEFT | TA_RIGHT | TA_CENTER))
    {
    case TA_LEFT:
        if (align & TA_UPDATECP)
        {
            pt.x = x + width.x;
            pt.y = y + width.y;
            DPtoLP(hdc, &pt, 1);
            MoveToEx(hdc, pt.x, pt.y, NULL);
        }
        break;

    case TA_CENTER:
        x -= width.x / 2;
        y -= width.y / 2;
        break;

    case TA_RIGHT:
        x -= width.x;
        y -= width.y;
        if (align & TA_UPDATECP)
        {
            pt.x = x;
            pt.y = y;
            DPtoLP(hdc, &pt, 1);
            MoveToEx(hdc, pt.x, pt.y, NULL);
        }
        break;
    }

    switch(align & (TA_TOP | TA_BOTTOM | TA_BASELINE))
    {
    case TA_TOP:
        y += tm.tmAscent * cosEsc;
        x += tm.tmAscent * sinEsc;
        break;

    case TA_BOTTOM:
        y -= tm.tmDescent * cosEsc;
        x -= tm.tmDescent * sinEsc;
        break;

    case TA_BASELINE:
        break;
    }

    if (GetBkMode(hdc) != TRANSPARENT)
    {
        if(!((flags & ETO_CLIPPED) && (flags & ETO_OPAQUE)))
        {
            if(!(flags & ETO_OPAQUE) || !rect ||
               x < rc.left || x + width.x >= rc.right ||
               y - tm.tmAscent < rc.top || y + tm.tmDescent >= rc.bottom)
            {
                RECT text_box;
                text_box.left = x;
                text_box.right = x + width.x;
                text_box.top = y - tm.tmAscent;
                text_box.bottom = y + tm.tmDescent;

                if (flags & ETO_CLIPPED) intersect_rect(&text_box, &text_box, &rc);
                if (!IsRectEmpty(&text_box))
                    PSDRV_ExtTextOut(data->ctx, 0, 0, ETO_OPAQUE, &text_box, NULL, 0, NULL);
            }
        }
    }

    ret = PSDRV_ExtTextOut(data->ctx, x, y, (flags & ~ETO_OPAQUE), &rc,
            str, count, (INT*)deltas);

done:
    free(deltas);

    if (ret && (lf.lfUnderline || lf.lfStrikeOut))
    {
        int underlinePos, strikeoutPos;
        int underlineWidth, strikeoutWidth;
        UINT size = NtGdiGetOutlineTextMetricsInternalW(hdc, 0, NULL, 0);
        OUTLINETEXTMETRICW* otm = NULL;
        POINT pts[5];
        HBRUSH hbrush = CreateSolidBrush(GetTextColor(hdc));
        HPEN hpen = GetStockObject(NULL_PEN);

        PSDRV_SelectPen(data->ctx, hpen, NULL);
        hpen = SelectObject(hdc, hpen);

        PSDRV_SelectBrush(data->ctx, hbrush, NULL);
        hbrush = SelectObject(hdc, hbrush);

        if(!size)
        {
            underlinePos = 0;
            underlineWidth = tm.tmAscent / 20 + 1;
            strikeoutPos = tm.tmAscent / 2;
            strikeoutWidth = underlineWidth;
        }
        else
        {
            otm = malloc(size);
            NtGdiGetOutlineTextMetricsInternalW(hdc, size, otm, 0);
            underlinePos = abs(INTERNAL_YWSTODS(hdc, otm->otmsUnderscorePosition));
            if (otm->otmsUnderscorePosition < 0) underlinePos = -underlinePos;
            underlineWidth = get_line_width(hdc, otm->otmsUnderscoreSize);
            strikeoutPos = abs(INTERNAL_YWSTODS(hdc, otm->otmsStrikeoutPosition));
            if (otm->otmsStrikeoutPosition < 0) strikeoutPos = -strikeoutPos;
            strikeoutWidth = get_line_width(hdc, otm->otmsStrikeoutSize);
            free(otm);
        }


        if (lf.lfUnderline)
        {
            const INT cnt = 5;
            pts[0].x = x - (underlinePos + underlineWidth / 2) * sinEsc;
            pts[0].y = y - (underlinePos + underlineWidth / 2) * cosEsc;
            pts[1].x = x + width.x - (underlinePos + underlineWidth / 2) * sinEsc;
            pts[1].y = y + width.y - (underlinePos + underlineWidth / 2) * cosEsc;
            pts[2].x = pts[1].x + underlineWidth * sinEsc;
            pts[2].y = pts[1].y + underlineWidth * cosEsc;
            pts[3].x = pts[0].x + underlineWidth * sinEsc;
            pts[3].y = pts[0].y + underlineWidth * cosEsc;
            pts[4].x = pts[0].x;
            pts[4].y = pts[0].y;
            DPtoLP(hdc, pts, 5);
            PSDRV_PolyPolygon(data->ctx, pts, &cnt, 1);
        }

        if (lf.lfStrikeOut)
        {
            const INT cnt = 5;
            pts[0].x = x - (strikeoutPos + strikeoutWidth / 2) * sinEsc;
            pts[0].y = y - (strikeoutPos + strikeoutWidth / 2) * cosEsc;
            pts[1].x = x + width.x - (strikeoutPos + strikeoutWidth / 2) * sinEsc;
            pts[1].y = y + width.y - (strikeoutPos + strikeoutWidth / 2) * cosEsc;
            pts[2].x = pts[1].x + strikeoutWidth * sinEsc;
            pts[2].y = pts[1].y + strikeoutWidth * cosEsc;
            pts[3].x = pts[0].x + strikeoutWidth * sinEsc;
            pts[3].y = pts[0].y + strikeoutWidth * cosEsc;
            pts[4].x = pts[0].x;
            pts[4].y = pts[0].y;
            DPtoLP(hdc, pts, 5);
            PSDRV_PolyPolygon(data->ctx, pts, &cnt, 1);
        }

        PSDRV_SelectPen(data->ctx, hpen, NULL);
        SelectObject(hdc, hpen);
        select_hbrush(data, htable, handle_count, hbrush);
        SelectObject(hdc, hbrush);
        DeleteObject(hbrush);
    }

    HeapFree(GetProcessHeap(), 0, glyphs);
    return ret;
}

static BOOL fill_rgn(struct pp_data *data, HANDLETABLE *htable, int handle_count, DWORD brush, HRGN rgn)
{
    struct ps_brush_pattern *pattern;
    HBRUSH hbrush;
    int ret;

    hbrush = get_object_handle(data, htable, brush, &pattern);
    PSDRV_SelectBrush(data->ctx, hbrush, pattern);
    ret = PSDRV_PaintRgn(data->ctx, rgn);
    select_hbrush(data, htable, handle_count, GetCurrentObject(data->ctx->hdc, OBJ_BRUSH));
    PSDRV_SelectBrush(data->ctx, hbrush, pattern);
    return ret;
}

static BOOL is_path_record(int type)
{
    switch(type)
    {
    case EMR_POLYBEZIER:
    case EMR_POLYGON:
    case EMR_POLYLINE:
    case EMR_POLYBEZIERTO:
    case EMR_POLYLINETO:
    case EMR_POLYPOLYLINE:
    case EMR_POLYPOLYGON:
    case EMR_MOVETOEX:
    case EMR_ANGLEARC:
    case EMR_ELLIPSE:
    case EMR_RECTANGLE:
    case EMR_ROUNDRECT:
    case EMR_ARC:
    case EMR_CHORD:
    case EMR_PIE:
    case EMR_LINETO:
    case EMR_ARCTO:
    case EMR_POLYDRAW:
    case EMR_EXTTEXTOUTA:
    case EMR_EXTTEXTOUTW:
    case EMR_POLYBEZIER16:
    case EMR_POLYGON16:
    case EMR_POLYLINE16:
    case EMR_POLYBEZIERTO16:
    case EMR_POLYLINETO16:
    case EMR_POLYPOLYLINE16:
    case EMR_POLYPOLYGON16:
    case EMR_POLYDRAW16:
        return TRUE;
    default:
        return FALSE;
    }
}

static int WINAPI hmf_proc(HDC hdc, HANDLETABLE *htable,
        const ENHMETARECORD *rec, int handle_count, LPARAM arg)
{
    struct pp_data *data = (struct pp_data *)arg;

    if (data->path && is_path_record(rec->iType))
        return PlayEnhMetaFileRecord(data->ctx->hdc, htable, rec, handle_count);

    switch (rec->iType)
    {
    case EMR_HEADER:
    {
        const ENHMETAHEADER *header = (const ENHMETAHEADER *)rec;

        data->patterns = calloc(sizeof(*data->patterns), header->nHandles);
        return data->patterns && PSDRV_StartPage(data->ctx);
    }
    case EMR_POLYBEZIER:
    {
        const EMRPOLYBEZIER *p = (const EMRPOLYBEZIER *)rec;

        return PSDRV_PolyBezier(data->ctx, (const POINT *)p->aptl, p->cptl);
    }
    case EMR_POLYGON:
    {
        const EMRPOLYGON *p = (const EMRPOLYGON *)rec;

        return PSDRV_PolyPolygon(data->ctx, (const POINT *)p->aptl,
                (const INT *)&p->cptl, 1);
    }
    case EMR_POLYLINE:
    {
        const EMRPOLYLINE *p = (const EMRPOLYLINE *)rec;

        return PSDRV_PolyPolyline(data->ctx,
                (const POINT *)p->aptl, &p->cptl, 1);
    }
    case EMR_POLYBEZIERTO:
    {
        const EMRPOLYBEZIERTO *p = (const EMRPOLYBEZIERTO *)rec;

        return PSDRV_PolyBezierTo(data->ctx, (const POINT *)p->aptl, p->cptl) &&
            MoveToEx(data->ctx->hdc, p->aptl[p->cptl - 1].x, p->aptl[p->cptl - 1].y, NULL);
    }
    case EMR_POLYLINETO:
    {
        const EMRPOLYLINETO *p = (const EMRPOLYLINETO *)rec;
        POINT *pts;
        DWORD cnt;
        int ret;

        cnt = p->cptl + 1;
        pts = malloc(sizeof(*pts) * cnt);
        if (!pts) return 0;

        GetCurrentPositionEx(data->ctx->hdc, pts);
        memcpy(pts + 1, p->aptl, sizeof(*pts) * p->cptl);
        ret = PSDRV_PolyPolyline(data->ctx, pts, &cnt, 1) &&
            MoveToEx(data->ctx->hdc, pts[cnt - 1].x, pts[cnt - 1].y, NULL);
        free(pts);
        return ret;
    }
    case EMR_POLYPOLYLINE:
    {
        const EMRPOLYPOLYLINE *p = (const EMRPOLYPOLYLINE *)rec;

        return PSDRV_PolyPolyline(data->ctx,
                (const POINT *)(p->aPolyCounts + p->nPolys),
                p->aPolyCounts, p->nPolys);
    }
    case EMR_POLYPOLYGON:
    {
        const EMRPOLYPOLYGON *p = (const EMRPOLYPOLYGON *)rec;

        return PSDRV_PolyPolygon(data->ctx,
                (const POINT *)(p->aPolyCounts + p->nPolys),
                (const INT *)p->aPolyCounts, p->nPolys);
    }
    case EMR_EOF:
        return PSDRV_EndPage(data->ctx);
    case EMR_SETPIXELV:
    {
        const EMRSETPIXELV *p = (const EMRSETPIXELV *)rec;

        PSDRV_SetPixel(data->ctx, p->ptlPixel.x, p->ptlPixel.y, p->crColor);
        return 1;
    }
    case EMR_SETTEXTCOLOR:
    {
        const EMRSETTEXTCOLOR *p = (const EMRSETTEXTCOLOR *)rec;

        SetTextColor(data->ctx->hdc, p->crColor);
        PSDRV_SetTextColor(data->ctx, p->crColor);
        return 1;
    }
    case EMR_SETBKCOLOR:
    {
        const EMRSETBKCOLOR *p = (const EMRSETBKCOLOR *)rec;

        SetBkColor(data->ctx->hdc, p->crColor);
        PSDRV_SetBkColor(data->ctx, p->crColor);
        return 1;
    }
    case EMR_SAVEDC:
    {
        int ret = PlayEnhMetaFileRecord(data->ctx->hdc, htable, rec, handle_count);

        if (!data->saved_dc_size)
        {
            data->saved_dc = malloc(8 * sizeof(*data->saved_dc));
            if (data->saved_dc)
                data->saved_dc_size = 8;
        }
        else if (data->saved_dc_size == data->saved_dc_top)
        {
            void *alloc = realloc(data->saved_dc, data->saved_dc_size * 2);

            if (alloc)
            {
                data->saved_dc = alloc;
                data->saved_dc_size *= 2;
            }
        }
        if (data->saved_dc_size == data->saved_dc_top)
            return 0;

        data->saved_dc[data->saved_dc_top].break_extra = data->break_extra;
        data->saved_dc[data->saved_dc_top].break_rem = data->break_rem;
        data->saved_dc_top++;
        return ret;
    }
    case EMR_RESTOREDC:
    {
        const EMRRESTOREDC *p = (const EMRRESTOREDC *)rec;
        HDC hdc = data->ctx->hdc;
        int ret = PlayEnhMetaFileRecord(hdc, htable, rec, handle_count);
        UINT aa_flags;

        if (ret)
        {
            select_hbrush(data, htable, handle_count, GetCurrentObject(hdc, OBJ_BRUSH));
            PSDRV_SelectFont(data->ctx, GetCurrentObject(hdc, OBJ_FONT), &aa_flags);
            PSDRV_SelectPen(data->ctx, GetCurrentObject(hdc, OBJ_PEN), NULL);
            PSDRV_SetBkColor(data->ctx, GetBkColor(hdc));
            PSDRV_SetTextColor(data->ctx, GetTextColor(hdc));

            if (p->iRelative >= 0 || data->saved_dc_top + p->iRelative < 0)
                return 0;
            data->saved_dc_top += p->iRelative;
            data->break_extra = data->saved_dc[data->saved_dc_top].break_extra;
            data->break_rem = data->saved_dc[data->saved_dc_top].break_rem;
        }
        return ret;
    }
    case EMR_SELECTOBJECT:
    {
        const EMRSELECTOBJECT *so = (const EMRSELECTOBJECT *)rec;
        struct ps_brush_pattern *pattern;
        UINT aa_flags;
        HGDIOBJ obj;

        obj = get_object_handle(data, htable, so->ihObject, &pattern);
        SelectObject(data->ctx->hdc, obj);

        switch (GetObjectType(obj))
        {
        case OBJ_PEN: return PSDRV_SelectPen(data->ctx, obj, NULL) != NULL;
        case OBJ_BRUSH: return PSDRV_SelectBrush(data->ctx, obj, pattern) != NULL;
        case OBJ_FONT: return PSDRV_SelectFont(data->ctx, obj, &aa_flags) != NULL;
        case OBJ_EXTPEN: return PSDRV_SelectPen(data->ctx, obj, NULL) != NULL;
        default:
            FIXME("unhandled object type %ld\n", GetObjectType(obj));
            return 1;
        }
    }
    case EMR_DELETEOBJECT:
    {
        const EMRDELETEOBJECT *p = (const EMRDELETEOBJECT *)rec;

        memset(&data->patterns[p->ihObject], 0, sizeof(*data->patterns));
        return PlayEnhMetaFileRecord(data->ctx->hdc, htable, rec, handle_count);
    }
    case EMR_ANGLEARC:
    {
        const EMRANGLEARC *p = (const EMRANGLEARC *)rec;
        int arc_dir = SetArcDirection(data->ctx->hdc,
                p->eSweepAngle >= 0 ? AD_COUNTERCLOCKWISE : AD_CLOCKWISE);
        EMRARCTO arcto;
        int ret;

        arcto.emr.iType = EMR_ARCTO;
        arcto.rclBox.left = p->ptlCenter.x - p->nRadius;
        arcto.rclBox.top = p->ptlCenter.y - p->nRadius;
        arcto.rclBox.right = p->ptlCenter.x + p->nRadius;
        arcto.rclBox.bottom = p->ptlCenter.y + p->nRadius;
        arcto.ptlStart.x = GDI_ROUND(p->ptlCenter.x +
                cos(p->eStartAngle * M_PI / 180) * p->nRadius);
        arcto.ptlStart.y = GDI_ROUND(p->ptlCenter.y -
                sin(p->eStartAngle * M_PI / 180) * p->nRadius);
        arcto.ptlEnd.x = GDI_ROUND(p->ptlCenter.x +
                cos((p->eStartAngle + p->eSweepAngle) * M_PI / 180) * p->nRadius);
        arcto.ptlEnd.y = GDI_ROUND(p->ptlCenter.y -
                sin((p->eStartAngle + p->eSweepAngle) * M_PI / 180) * p->nRadius);

        ret = hmf_proc(hdc, htable, (ENHMETARECORD *)&arcto, handle_count, arg);
        SetArcDirection(data->ctx->hdc, arc_dir);
        return ret;
    }
    case EMR_ELLIPSE:
    {
        const EMRELLIPSE *p = (const EMRELLIPSE *)rec;
        const RECTL *r = &p->rclBox;

        return PSDRV_Ellipse(data->ctx, r->left, r->top, r->right, r->bottom);
    }
    case EMR_RECTANGLE:
    {
        const EMRRECTANGLE *rect = (const EMRRECTANGLE *)rec;

        return PSDRV_Rectangle(data->ctx, rect->rclBox.left,
                rect->rclBox.top, rect->rclBox.right, rect->rclBox.bottom);
    }
    case EMR_ROUNDRECT:
    {
        const EMRROUNDRECT *p = (const EMRROUNDRECT *)rec;

        return PSDRV_RoundRect(data->ctx, p->rclBox.left,
                p->rclBox.top, p->rclBox.right, p->rclBox.bottom,
                p->szlCorner.cx, p->szlCorner.cy);
    }
    case EMR_ARC:
    {
        const EMRARC *p = (const EMRARC *)rec;

        return PSDRV_Arc(data->ctx, p->rclBox.left, p->rclBox.top,
                p->rclBox.right, p->rclBox.bottom, p->ptlStart.x,
                p->ptlStart.y, p->ptlEnd.x, p->ptlEnd.y);
    }
    case EMR_CHORD:
    {
        const EMRCHORD *p = (const EMRCHORD *)rec;

        return PSDRV_Chord(data->ctx, p->rclBox.left, p->rclBox.top,
                p->rclBox.right, p->rclBox.bottom, p->ptlStart.x,
                p->ptlStart.y, p->ptlEnd.x, p->ptlEnd.y);
    }
    case EMR_PIE:
    {
        const EMRPIE *p = (const EMRPIE *)rec;

        return PSDRV_Pie(data->ctx, p->rclBox.left, p->rclBox.top,
                p->rclBox.right, p->rclBox.bottom, p->ptlStart.x,
                p->ptlStart.y, p->ptlEnd.x, p->ptlEnd.y);
    }
    case EMR_LINETO:
    {
        const EMRLINETO *line = (const EMRLINETO *)rec;

        return PSDRV_LineTo(data->ctx, line->ptl.x, line->ptl.y) &&
            MoveToEx(data->ctx->hdc, line->ptl.x, line->ptl.y, NULL);
    }
    case EMR_ARCTO:
    {
        const EMRARCTO *p = (const EMRARCTO *)rec;
        POINT pt;
        BOOL ret;

        ret = GetCurrentPositionEx(data->ctx->hdc, &pt);
        if (ret)
        {
            ret = ArcTo(data->ctx->hdc, p->rclBox.left, p->rclBox.top,
                    p->rclBox.right, p->rclBox.bottom, p->ptlStart.x,
                    p->ptlStart.y, p->ptlStart.x, p->ptlStart.y);
        }
        if (ret)
            ret = PSDRV_LineTo(data->ctx, pt.x, pt.y);
        if (ret)
        {
            ret = PSDRV_Arc(data->ctx, p->rclBox.left, p->rclBox.top,
                    p->rclBox.right, p->rclBox.bottom, p->ptlStart.x,
                    p->ptlStart.y, p->ptlEnd.x, p->ptlEnd.y);
        }
        if (ret)
        {
            ret = ArcTo(data->ctx->hdc, p->rclBox.left, p->rclBox.top,
                    p->rclBox.right, p->rclBox.bottom, p->ptlEnd.x,
                    p->ptlEnd.y, p->ptlEnd.x, p->ptlEnd.y);
        }
        return ret;
    }
    case EMR_POLYDRAW:
    {
        const EMRPOLYDRAW *p = (const EMRPOLYDRAW *)rec;
        const POINT *pts = (const POINT *)p->aptl;

        return poly_draw(data->ctx, pts, (BYTE *)(p->aptl + p->cptl), p->cptl) &&
            MoveToEx(data->ctx->hdc, pts[p->cptl - 1].x, pts[p->cptl - 1].y, NULL);
    }
    case EMR_BEGINPATH:
    {
        data->path = TRUE;
        return PlayEnhMetaFileRecord(data->ctx->hdc, htable, rec, handle_count);
    }
    case EMR_ENDPATH:
    {
        data->path = FALSE;
        return PlayEnhMetaFileRecord(data->ctx->hdc, htable, rec, handle_count);
    }
    case EMR_FILLPATH:
        PSDRV_FillPath(data->ctx);
        return 1;
    case EMR_STROKEANDFILLPATH:
        PSDRV_StrokeAndFillPath(data->ctx);
        return 1;
    case EMR_STROKEPATH:
        PSDRV_StrokePath(data->ctx);
        return 1;
    case EMR_ABORTPATH:
    {
        data->path = FALSE;
        return PlayEnhMetaFileRecord(data->ctx->hdc, htable, rec, handle_count);
    }
    case EMR_FILLRGN:
    {
        const EMRFILLRGN *p = (const EMRFILLRGN *)rec;
        HRGN rgn;
        int ret;

        rgn = ExtCreateRegion(NULL, p->cbRgnData, (const RGNDATA *)p->RgnData);
        ret = fill_rgn(data, htable, handle_count, p->ihBrush, rgn);
        DeleteObject(rgn);
        return ret;
    }
    case EMR_FRAMERGN:
    {
        const EMRFRAMERGN *p = (const EMRFRAMERGN *)rec;
        HRGN rgn, frame;
        int ret;

        rgn = ExtCreateRegion(NULL, p->cbRgnData, (const RGNDATA *)p->RgnData);
        frame = CreateRectRgn(0, 0, 0, 0);

        CombineRgn(frame, rgn, 0, RGN_COPY);
        OffsetRgn(frame, -p->szlStroke.cx, 0);
        OffsetRgn(rgn, p->szlStroke.cx, 0);
        CombineRgn(frame, frame, rgn, RGN_AND);
        OffsetRgn(rgn, -p->szlStroke.cx, -p->szlStroke.cy);
        CombineRgn(frame, frame, rgn, RGN_AND);
        OffsetRgn(rgn, 0, 2*p->szlStroke.cy);
        CombineRgn(frame, frame, rgn, RGN_AND);
        OffsetRgn(rgn, 0, -p->szlStroke.cy);
        CombineRgn(frame, rgn, frame, RGN_DIFF);

        ret = fill_rgn(data, htable, handle_count, p->ihBrush, frame);
        DeleteObject(rgn);
        DeleteObject(frame);
        return ret;
    }
    case EMR_INVERTRGN:
    {
        const EMRINVERTRGN *p = (const EMRINVERTRGN *)rec;
        int old_rop, ret;
        HRGN rgn;

        rgn = ExtCreateRegion(NULL, p->cbRgnData, (const RGNDATA *)p->RgnData);
        old_rop = SetROP2(data->ctx->hdc, R2_NOT);
        ret = fill_rgn(data, htable, handle_count, 0x80000000 | BLACK_BRUSH, rgn);
        SetROP2(data->ctx->hdc, old_rop);
        DeleteObject(rgn);
        return ret;
    }
    case EMR_PAINTRGN:
    {
        const EMRPAINTRGN *p = (const EMRPAINTRGN *)rec;
        HRGN rgn = ExtCreateRegion(NULL, p->cbRgnData, (const RGNDATA *)p->RgnData);
        int ret;

        ret = PSDRV_PaintRgn(data->ctx, rgn);
        DeleteObject(rgn);
        return ret;
    }
    case EMR_BITBLT:
    {
        const EMRBITBLT *p = (const EMRBITBLT *)rec;
        const BITMAPINFO *bi = (const BITMAPINFO *)((BYTE *)p + p->offBmiSrc);
        const BYTE *src_bits = (BYTE *)p + p->offBitsSrc;
        EMRSTRETCHBLT blt;


        blt.rclBounds = p->rclBounds;
        blt.xDest = p->xDest;
        blt.yDest = p->yDest;
        blt.cxDest = p->cxDest;
        blt.cyDest = p->cyDest;
        blt.dwRop = p->dwRop;
        blt.xSrc = p->xSrc;
        blt.ySrc = p->ySrc;
        blt.xformSrc = p->xformSrc;
        blt.crBkColorSrc = p->crBkColorSrc;
        blt.iUsageSrc = p->iUsageSrc;
        blt.offBmiSrc = p->offBmiSrc;
        blt.cbBmiSrc = p->cbBmiSrc;
        blt.offBitsSrc = p->offBitsSrc;
        blt.cbBitsSrc = p->cbBitsSrc;
        blt.cxSrc = p->cxDest;
        blt.cySrc = p->cyDest;

        return stretch_blt(data->ctx, &blt, bi, src_bits);
    }
    case EMR_STRETCHBLT:
    {
        const EMRSTRETCHBLT *p = (const EMRSTRETCHBLT *)rec;
        const BITMAPINFO *bi = (const BITMAPINFO *)((BYTE *)p + p->offBmiSrc);
        const BYTE *src_bits = (BYTE *)p + p->offBitsSrc;

        return stretch_blt(data->ctx, p, bi, src_bits);
    }
    case EMR_MASKBLT:
    {
        const EMRMASKBLT *p = (const EMRMASKBLT *)rec;
        const BITMAPINFO *mask_bi = (const BITMAPINFO *)((BYTE *)p + p->offBmiMask);
        const BITMAPINFO *src_bi = (const BITMAPINFO *)((BYTE *)p + p->offBmiSrc);
        const BYTE *mask_bits = (BYTE *)p + p->offBitsMask;
        const BYTE *src_bits = (BYTE *)p + p->offBitsSrc;

        return mask_blt(data->ctx, p, src_bi, src_bits, mask_bi, mask_bits);
    }
    case EMR_PLGBLT:
    {
        const EMRPLGBLT *p = (const EMRPLGBLT *)rec;

        return plg_blt(data->ctx, p);
    }
    case EMR_SETDIBITSTODEVICE:
        return set_di_bits_to_device(data->ctx, (const EMRSETDIBITSTODEVICE *)rec);
    case EMR_STRETCHDIBITS:
        return stretch_di_bits(data->ctx, (const EMRSTRETCHDIBITS *)rec);
    case EMR_EXTTEXTOUTW:
    {
        const EMREXTTEXTOUTW *p = (const EMREXTTEXTOUTW *)rec;
        HDC hdc = data->ctx->hdc;
        const INT *dx = NULL;
        int old_mode, ret;
        RECT rect;

        rect.left = p->emrtext.rcl.left;
        rect.top = p->emrtext.rcl.top;
        rect.right = p->emrtext.rcl.right;
        rect.bottom = p->emrtext.rcl.bottom;

        old_mode = SetGraphicsMode(hdc, p->iGraphicsMode);
        /* Reselect the font back into the dc so that the transformation
           gets updated. */
        SelectObject(hdc, GetCurrentObject(hdc, OBJ_FONT));

        if (p->emrtext.offDx)
            dx = (const INT *)((const BYTE *)rec + p->emrtext.offDx);

        ret = ext_text_out(data, htable, handle_count, p->emrtext.ptlReference.x,
                p->emrtext.ptlReference.y, p->emrtext.fOptions, &rect,
                (LPCWSTR)((const BYTE *)rec + p->emrtext.offString), p->emrtext.nChars, dx);

        SetGraphicsMode(hdc, old_mode);
        return ret;
    }
    case EMR_POLYBEZIER16:
    {
        const EMRPOLYBEZIER16 *p = (const EMRPOLYBEZIER16 *)rec;
        POINT *pts;
        int i;

        pts = malloc(sizeof(*pts) * p->cpts);
        if (!pts) return 0;
        for (i = 0; i < p->cpts; i++)
        {
            pts[i].x = p->apts[i].x;
            pts[i].y = p->apts[i].y;
        }
        i = PSDRV_PolyBezier(data->ctx, pts, p->cpts);
        free(pts);
        return i;
    }
    case EMR_POLYGON16:
    {
        const EMRPOLYGON16 *p = (const EMRPOLYGON16 *)rec;
        POINT *pts;
        int i;

        pts = malloc(sizeof(*pts) * p->cpts);
        if (!pts) return 0;
        for (i = 0; i < p->cpts; i++)
        {
            pts[i].x = p->apts[i].x;
            pts[i].y = p->apts[i].y;
        }
        i = PSDRV_PolyPolygon(data->ctx, pts, (const INT *)&p->cpts, 1);
        free(pts);
        return i;
    }
    case EMR_POLYLINE16:
    {
        const EMRPOLYLINE16 *p = (const EMRPOLYLINE16 *)rec;
        POINT *pts;
        int i;

        pts = malloc(sizeof(*pts) * p->cpts);
        if (!pts) return 0;
        for (i = 0; i < p->cpts; i++)
        {
            pts[i].x = p->apts[i].x;
            pts[i].y = p->apts[i].y;
        }
        i = PSDRV_PolyPolyline(data->ctx, pts, &p->cpts, 1);
        free(pts);
        return i;
    }
    case EMR_POLYBEZIERTO16:
    {
        const EMRPOLYBEZIERTO16 *p = (const EMRPOLYBEZIERTO16 *)rec;
        POINT *pts;
        int i;

        pts = malloc(sizeof(*pts) * p->cpts);
        if (!pts) return 0;
        for (i = 0; i < p->cpts; i++)
        {
            pts[i].x = p->apts[i].x;
            pts[i].y = p->apts[i].y;
        }
        i = PSDRV_PolyBezierTo(data->ctx, pts, p->cpts) &&
            MoveToEx(data->ctx->hdc, pts[p->cpts - 1].x, pts[p->cpts - 1].y, NULL);
        free(pts);
        return i;
    }
    case EMR_POLYLINETO16:
    {
        const EMRPOLYLINETO16 *p = (const EMRPOLYLINETO16 *)rec;
        POINT *pts;
        DWORD cnt;
        int i;

        cnt = p->cpts + 1;
        pts = malloc(sizeof(*pts) * cnt);
        if (!pts) return 0;
        GetCurrentPositionEx(data->ctx->hdc, pts);
        for (i = 0; i < p->cpts; i++)
        {
            pts[i + 1].x = p->apts[i].x;
            pts[i + 1].y = p->apts[i].y;
        }
        i = PSDRV_PolyPolyline(data->ctx, pts, &cnt, 1) &&
            MoveToEx(data->ctx->hdc, pts[cnt - 1].x, pts[cnt - 1].y, NULL);
        free(pts);
        return i;
    }
    case EMR_POLYPOLYLINE16:
    {
        const EMRPOLYPOLYLINE16 *p = (const EMRPOLYPOLYLINE16 *)rec;
        POINT *pts;
        int i;

        pts = malloc(sizeof(*pts) * p->cpts);
        if (!pts) return 0;
        for (i = 0; i < p->cpts; i++)
        {
            pts[i].x = ((const POINTS *)(p->aPolyCounts + p->nPolys))[i].x;
            pts[i].y = ((const POINTS *)(p->aPolyCounts + p->nPolys))[i].y;
        }
        i = PSDRV_PolyPolyline(data->ctx, pts, p->aPolyCounts, p->nPolys);
        free(pts);
        return i;
    }
    case EMR_POLYPOLYGON16:
    {
        const EMRPOLYPOLYGON16 *p = (const EMRPOLYPOLYGON16 *)rec;
        POINT *pts;
        int i;

        pts = malloc(sizeof(*pts) * p->cpts);
        if (!pts) return 0;
        for (i = 0; i < p->cpts; i++)
        {
            pts[i].x = ((const POINTS *)(p->aPolyCounts + p->nPolys))[i].x;
            pts[i].y = ((const POINTS *)(p->aPolyCounts + p->nPolys))[i].y;
        }
        i = PSDRV_PolyPolygon(data->ctx, pts, (const INT *)p->aPolyCounts, p->nPolys);
        free(pts);
        return i;
    }
    case EMR_POLYDRAW16:
    {
        const EMRPOLYDRAW16 *p = (const EMRPOLYDRAW16 *)rec;
        POINT *pts;
        int i;

        pts = malloc(sizeof(*pts) * p->cpts);
        if (!pts) return 0;
        for (i = 0; i < p->cpts; i++)
        {
            pts[i].x = ((const POINTS *)p->apts)[i].x;
            pts[i].y = ((const POINTS *)p->apts)[i].y;
        }
        i = poly_draw(data->ctx, pts, (BYTE *)(p->apts + p->cpts), p->cpts) &&
            MoveToEx(data->ctx->hdc, pts[p->cpts - 1].x, pts[p->cpts - 1].y, NULL);
        free(pts);
        return i;
    }
    case EMR_CREATEMONOBRUSH:
    {
        const EMRCREATEMONOBRUSH *p = (const EMRCREATEMONOBRUSH *)rec;

        if (!PlayEnhMetaFileRecord(data->ctx->hdc, htable, rec, handle_count))
            return 0;
        data->patterns[p->ihBrush].usage = p->iUsage;
        data->patterns[p->ihBrush].info = (BITMAPINFO *)((BYTE *)p + p->offBmi);
        data->patterns[p->ihBrush].bits.ptr = (BYTE *)p + p->offBits;
        return 1;
    }
    case EMR_CREATEDIBPATTERNBRUSHPT:
    {
        const EMRCREATEDIBPATTERNBRUSHPT *p = (const EMRCREATEDIBPATTERNBRUSHPT *)rec;

        if (!PlayEnhMetaFileRecord(data->ctx->hdc, htable, rec, handle_count))
            return 0;
        data->patterns[p->ihBrush].usage = p->iUsage;
        data->patterns[p->ihBrush].info = (BITMAPINFO *)((BYTE *)p + p->offBmi);
        data->patterns[p->ihBrush].bits.ptr = (BYTE *)p + p->offBits;
        return 1;
    }
    case EMR_EXTESCAPE:
    {
        const EMREXTESCAPE *p = (const EMREXTESCAPE *)rec;

        PSDRV_ExtEscape(data->ctx, p->iEscape, p->cbEscData, p->EscData, 0, NULL);
        return 1;
    }
    case EMR_GRADIENTFILL:
    {
        const EMRGRADIENTFILL *p = (const EMRGRADIENTFILL *)rec;

        return gradient_fill(data->ctx, p->Ver, p->nVer,
                p->Ver + p->nVer, p->nTri, p->ulMode);
    }
    case EMR_SETTEXTJUSTIFICATION:
    {
        const EMRSETTEXTJUSTIFICATION *p = (const EMRSETTEXTJUSTIFICATION *)rec;

        if (p->break_count)
        {
            data->break_extra = p->break_extra / p->break_count;
            data->break_rem = p->break_extra - data->break_extra * p->break_count;
        }
        else
        {
            data->break_extra = 0;
            data->break_rem = 0;
        }
        return PlayEnhMetaFileRecord(data->ctx->hdc, htable, rec, handle_count);
    }

    case EMR_EXTFLOODFILL:
    case EMR_ALPHABLEND:
        break;

    case EMR_SETWINDOWEXTEX:
    case EMR_SETWINDOWORGEX:
    case EMR_SETVIEWPORTEXTEX:
    case EMR_SETVIEWPORTORGEX:
    case EMR_SETBRUSHORGEX:
    case EMR_SETMAPPERFLAGS:
    case EMR_SETMAPMODE:
    case EMR_SETBKMODE:
    case EMR_SETPOLYFILLMODE:
    case EMR_SETROP2:
    case EMR_SETSTRETCHBLTMODE:
    case EMR_SETTEXTALIGN:
    case EMR_OFFSETCLIPRGN:
    case EMR_MOVETOEX:
    case EMR_EXCLUDECLIPRECT:
    case EMR_INTERSECTCLIPRECT:
    case EMR_SCALEVIEWPORTEXTEX:
    case EMR_SCALEWINDOWEXTEX:
    case EMR_SETWORLDTRANSFORM:
    case EMR_MODIFYWORLDTRANSFORM:
    case EMR_CREATEPEN:
    case EMR_CREATEBRUSHINDIRECT:
    case EMR_SELECTPALETTE:
    case EMR_CREATEPALETTE:
    case EMR_SETPALETTEENTRIES:
    case EMR_RESIZEPALETTE:
    case EMR_REALIZEPALETTE:
    case EMR_SETARCDIRECTION:
    case EMR_CLOSEFIGURE:
    case EMR_FLATTENPATH:
    case EMR_WIDENPATH:
    case EMR_SELECTCLIPPATH:
    case EMR_EXTSELECTCLIPRGN:
    case EMR_EXTCREATEFONTINDIRECTW:
    case EMR_EXTCREATEPEN:
    case EMR_SETLAYOUT:
        return PlayEnhMetaFileRecord(data->ctx->hdc, htable, rec, handle_count);
    default:
        FIXME("unsupported record: %ld\n", rec->iType);
    }

    return 1;
}

static BOOL print_metafile(struct pp_data *data, HANDLE hdata)
{
    XFORM xform = { .eM11 = 1, .eM22 = 1 };
    record_hdr header;
    HENHMETAFILE hmf;
    BYTE *buf;
    BOOL ret;
    DWORD r;

    if (!ReadPrinter(hdata, &header, sizeof(header), &r))
        return FALSE;
    if (r != sizeof(header))
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    buf = malloc(header.cjSize);
    if (!buf)
        return FALSE;

    if (!ReadPrinter(hdata, buf, header.cjSize, &r))
    {
        free(buf);
        return FALSE;
    }
    if (r != header.cjSize)
    {
        free(buf);
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    hmf = SetEnhMetaFileBits(header.cjSize, buf);
    free(buf);
    if (!hmf)
        return FALSE;

    AbortPath(data->ctx->hdc);
    MoveToEx(data->ctx->hdc, 0, 0, NULL);
    SetBkColor(data->ctx->hdc, RGB(255, 255, 255));
    SetBkMode(data->ctx->hdc, OPAQUE);
    SetMapMode(data->ctx->hdc, MM_TEXT);
    SetPolyFillMode(data->ctx->hdc, ALTERNATE);
    SetROP2(data->ctx->hdc, R2_COPYPEN);
    SetStretchBltMode(data->ctx->hdc, BLACKONWHITE);
    SetTextAlign(data->ctx->hdc, TA_LEFT | TA_TOP);
    SetTextColor(data->ctx->hdc, 0);
    SetTextJustification(data->ctx->hdc, 0, 0);
    SetWorldTransform(data->ctx->hdc, &xform);
    PSDRV_SetTextColor(data->ctx, 0);
    PSDRV_SetBkColor(data->ctx, RGB(255, 255, 255));

    ret = EnumEnhMetaFile(NULL, hmf, hmf_proc, (void *)data, NULL);
    DeleteEnhMetaFile(hmf);
    free(data->patterns);
    data->patterns = NULL;
    data->path = FALSE;
    data->break_extra = 0;
    data->break_rem = 0;
    data->saved_dc_top = 0;
    return ret;
}

BOOL WINAPI EnumPrintProcessorDatatypesW(WCHAR *server, WCHAR *name, DWORD level,
        BYTE *datatypes, DWORD size, DWORD *needed, DWORD *no)
{
    DATATYPES_INFO_1W *info = (DATATYPES_INFO_1W *)datatypes;

    TRACE("%s, %s, %ld, %p, %ld, %p, %p\n", debugstr_w(server), debugstr_w(name),
            level, datatypes, size, needed, no);

    if (!needed || !no)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *no = 0;
    *needed = sizeof(*info) + sizeof(emf_1003);

    if (level != 1 || (size && !datatypes))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (size < *needed)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    *no = 1;
    info->pName = (WCHAR*)(info + 1);
    memcpy(info + 1, emf_1003, sizeof(emf_1003));
    return TRUE;
}

HANDLE WINAPI OpenPrintProcessor(WCHAR *port, PRINTPROCESSOROPENDATA *open_data)
{
    struct pp_data *data;
    HANDLE hport;
    HDC hdc;

    TRACE("%s, %p\n", debugstr_w(port), open_data);

    if (!port || !open_data || !open_data->pDatatype)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    if (wcscmp(open_data->pDatatype, emf_1003))
    {
        SetLastError(ERROR_INVALID_DATATYPE);
        return NULL;
    }

    if (!OpenPrinterW(port, &hport, NULL))
        return NULL;

    data = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(*data));
    if (!data)
        return NULL;
    data->magic = PP_MAGIC;
    data->hport = hport;
    data->port = wcsdup(port);
    data->doc_name = wcsdup(open_data->pDocumentName);
    data->out_file = wcsdup(open_data->pOutputFile);

    hdc = CreateDCW(L"winspool", open_data->pPrinterName, NULL, open_data->pDevMode);
    if (!hdc)
    {
        LocalFree(data);
        return NULL;
    }
    SetGraphicsMode(hdc, GM_ADVANCED);
    data->ctx = create_print_ctx(hdc, open_data->pPrinterName, open_data->pDevMode);
    if (!data->ctx)
    {
        DeleteDC(hdc);
        LocalFree(data);
        return NULL;
    }
    return (HANDLE)data;
}

BOOL WINAPI PrintDocumentOnPrintProcessor(HANDLE pp, WCHAR *doc_name)
{
    struct pp_data *data = get_handle_data(pp);
    DEVMODEW *devmode = NULL;
    emfspool_header header;
    LARGE_INTEGER pos, cur;
    record_hdr record;
    HANDLE spool_data;
    DOC_INFO_1W info;
    BOOL ret;
    DWORD r;

    TRACE("%p, %s\n", pp, debugstr_w(doc_name));

    if (!data)
        return FALSE;

    spool_data = GdiGetSpoolFileHandle(data->port,
            &data->ctx->Devmode->dmPublic, doc_name);
    GdiGetDevmodeForPage(spool_data, 1, &devmode, NULL);
    if (devmode && devmode->dmFields & DM_COPIES)
    {
        data->ctx->Devmode->dmPublic.dmFields |= DM_COPIES;
        data->ctx->Devmode->dmPublic.dmCopies = devmode->dmCopies;
    }
    if (devmode && devmode->dmFields & DM_COLLATE)
    {
        data->ctx->Devmode->dmPublic.dmFields |= DM_COLLATE;
        data->ctx->Devmode->dmPublic.dmCollate = devmode->dmCollate;
    }
    GdiDeleteSpoolFileHandle(spool_data);

    if (!OpenPrinterW(doc_name, &spool_data, NULL))
        return FALSE;

    info.pDocName = data->doc_name;
    info.pOutputFile = data->out_file;
    info.pDatatype = (WCHAR *)L"RAW";
    data->ctx->job.id = StartDocPrinterW(data->hport, 1, (BYTE *)&info);
    if (!data->ctx->job.id)
    {
        ClosePrinter(spool_data);
        return FALSE;
    }

    if (!(ret = ReadPrinter(spool_data, &header, sizeof(header), &r)))
        goto cleanup;
    if (r != sizeof(header))
    {
        SetLastError(ERROR_INVALID_DATA);
        ret = FALSE;
        goto cleanup;
    }

    if (header.dwVersion != EMFSPOOL_VERSION)
    {
        FIXME("unrecognized spool file format\n");
        SetLastError(ERROR_INTERNAL_ERROR);
        goto cleanup;
    }
    pos.QuadPart = header.cjSize;
    if (!(ret = SeekPrinter(spool_data, pos, NULL, FILE_BEGIN, FALSE)))
        goto cleanup;

    data->ctx->job.hprinter = data->hport;
    if (!PSDRV_WriteHeader(data->ctx, data->doc_name))
    {
        WARN("Failed to write header\n");
        goto cleanup;
    }
    data->ctx->job.OutOfPage = TRUE;
    data->ctx->job.PageNo = 0;
    data->ctx->job.quiet = FALSE;
    data->ctx->job.passthrough_state = passthrough_none;
    data->ctx->job.doc_name = strdupW(data->doc_name);

    while (1)
    {
        if (!(ret = ReadPrinter(spool_data, &record, sizeof(record), &r)))
            goto cleanup;
        if (!r)
            break;
        if (r != sizeof(record))
        {
            SetLastError(ERROR_INVALID_DATA);
            ret = FALSE;
            goto cleanup;
        }

        switch (record.ulID)
        {
        case EMRI_DEVMODE:
        {
            DEVMODEW *devmode = NULL;

            if (record.cjSize)
            {
                devmode = malloc(record.cjSize);
                if (!devmode)
                    goto cleanup;
                ret = ReadPrinter(spool_data, devmode, record.cjSize, &r);
                if (ret && r != record.cjSize)
                {
                    SetLastError(ERROR_INVALID_DATA);
                    ret = FALSE;
                }
            }

            if (ret)
                ret = PSDRV_ResetDC(data->ctx, devmode);
            if (ret && devmode && (devmode->dmFields & DM_PAPERSIZE))
                ret = PSDRV_WritePageSize(data->ctx);
            free(devmode);
            if (!ret)
                goto cleanup;
            break;
        }
        case EMRI_METAFILE_DATA:
            pos.QuadPart = record.cjSize;
            ret = SeekPrinter(spool_data, pos, NULL, FILE_CURRENT, FALSE);
            if (!ret)
                goto cleanup;
            break;
        case EMRI_METAFILE_EXT:
        case EMRI_BW_METAFILE_EXT:
            pos.QuadPart = 0;
            ret = SeekPrinter(spool_data, pos, &cur, FILE_CURRENT, FALSE);
            if (ret)
            {
                cur.QuadPart += record.cjSize;
                ret = ReadPrinter(spool_data, &pos, sizeof(pos), &r);
                if (ret && r != sizeof(pos))
                {
                    SetLastError(ERROR_INVALID_DATA);
                    ret = FALSE;
                }
            }
            pos.QuadPart = -pos.QuadPart - 2 * sizeof(record);
            if (ret)
                ret = SeekPrinter(spool_data, pos, NULL, FILE_CURRENT, FALSE);
            if (ret)
                ret = print_metafile(data, spool_data);
            if (ret)
                ret = SeekPrinter(spool_data, cur, NULL, FILE_BEGIN, FALSE);
            if (!ret)
                goto cleanup;
            break;
        default:
            FIXME("%s not supported, skipping\n", debugstr_rec_type(record.ulID));
            pos.QuadPart = record.cjSize;
            ret = SeekPrinter(spool_data, pos, NULL, FILE_CURRENT, FALSE);
            if (!ret)
                goto cleanup;
            break;
        }
    }

cleanup:
    if (data->ctx->job.PageNo)
        PSDRV_WriteFooter(data->ctx);
    flush_spool(data->ctx);

    HeapFree(GetProcessHeap(), 0, data->ctx->job.doc_name);
    ClosePrinter(spool_data);
    return EndDocPrinter(data->hport) && ret;
}

BOOL WINAPI ControlPrintProcessor(HANDLE pp, DWORD cmd)
{
    FIXME("%p, %ld\n", pp, cmd);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI ClosePrintProcessor(HANDLE pp)
{
    struct pp_data *data = get_handle_data(pp);

    TRACE("%p\n", pp);

    if (!data)
        return FALSE;

    ClosePrinter(data->hport);
    free(data->port);
    free(data->doc_name);
    free(data->out_file);
    DeleteDC(data->ctx->hdc);
    HeapFree(GetProcessHeap(), 0, data->ctx->Devmode);
    HeapFree(GetProcessHeap(), 0, data->ctx);
    free(data->saved_dc);

    memset(data, 0, sizeof(*data));
    LocalFree(data);
    return TRUE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    AddPrintProcessorW(NULL, (WCHAR *)L"Windows 4.0", (WCHAR *)L"wineps.drv", (WCHAR *)L"wineps");
    AddPrintProcessorW(NULL, NULL, (WCHAR *)L"wineps.drv", (WCHAR *)L"wineps");
    return S_OK;
}
