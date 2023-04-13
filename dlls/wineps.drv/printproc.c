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
#include <winspool.h>
#include <ddk/winsplp.h>

#include "psdrv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

#define EMFSPOOL_VERSION 0x10000
#define PP_MAGIC 0x952173fe

struct pp_data
{
    DWORD magic;
    HANDLE hport;
    WCHAR *doc_name;
    WCHAR *out_file;
    PSDRV_PDEVICE *pdev;
    struct brush_pattern *patterns;
    BOOL path;
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

static BOOL intersect_vis_rectangles(struct bitblt_coords *dst, struct bitblt_coords *src)
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

static void get_vis_rectangles(HDC hdc, struct bitblt_coords *dst,
        const XFORM *xform, DWORD width, DWORD height, struct bitblt_coords *src)
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

    if (!src) return;

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
    src->visrect = rect;

    intersect_vis_rectangles(dst, src);
}

static int stretch_blt(PHYSDEV dev, const EMRSTRETCHBLT *blt,
        const BITMAPINFO *bi, const BYTE *src_bits)
{
    char dst_buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *dst_info = (BITMAPINFO *)dst_buffer;
    struct bitblt_coords src, dst;
    struct gdi_image_bits bits;
    DWORD err;

    dst.log_x = blt->xDest;
    dst.log_y = blt->yDest;
    dst.log_width = blt->cxDest;
    dst.log_height = blt->cyDest;
    dst.layout = GetLayout(dev->hdc);
    if (blt->dwRop & NOMIRRORBITMAP)
        dst.layout |= LAYOUT_BITMAPORIENTATIONPRESERVED;

    if (!blt->cbBmiSrc)
    {
        get_vis_rectangles(dev->hdc, &dst, NULL, 0, 0, NULL);
        return PSDRV_PatBlt(dev, &dst, blt->dwRop);
    }

    src.log_x = blt->xSrc;
    src.log_y = blt->ySrc;
    src.log_width = blt->cxSrc;
    src.log_height = blt->cySrc;
    src.layout = 0;

    get_vis_rectangles(dev->hdc, &dst, &blt->xformSrc,
            bi->bmiHeader.biWidth, bi->bmiHeader.biHeight, &src);

    memcpy(dst_info, bi, blt->cbBmiSrc);
    memset(&bits, 0, sizeof(bits));
    bits.ptr = (BYTE *)src_bits;
    err = PSDRV_PutImage(dev, 0, dst_info, &bits, &src, &dst, blt->dwRop);
    if (err == ERROR_BAD_FORMAT)
    {
        HDC hdc = CreateCompatibleDC(NULL);
        HBITMAP bitmap;

        bits.is_copy = TRUE;
        bitmap = CreateDIBSection(hdc, dst_info, DIB_RGB_COLORS, &bits.ptr, NULL, 0);
        SetDIBits(hdc, bitmap, 0, bi->bmiHeader.biHeight, src_bits, bi, blt->iUsageSrc);

        err = PSDRV_PutImage(dev, 0, dst_info, &bits, &src, &dst, blt->dwRop);
        DeleteObject(bitmap);
        DeleteObject(hdc);
    }

    if (err != ERROR_SUCCESS)
        FIXME("PutImage returned %ld\n", err);
    return !err;
}

#define FRGND_ROP3(ROP4)        ((ROP4) & 0x00FFFFFF)
#define BKGND_ROP3(ROP4)        (ROP3Table[((ROP4)>>24) & 0xFF])

static int mask_blt(PHYSDEV dev, const EMRMASKBLT *p, const BITMAPINFO *src_bi,
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

        return stretch_blt(dev, &blt, src_bi, src_bits);
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
    SetDIBits(dev->hdc, bmp1, 0, mask_bi->bmiHeader.biHeight, mask_bits, mask_bi, p->iUsageMask);
    brush_mask = CreatePatternBrush(bmp1);
    DeleteObject(bmp1);
    brush_dest = SelectObject(dev->hdc, GetStockObject(NULL_BRUSH));

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

    stretch_blt(dev, &blt, bmp2_info, bits);

    /* restore all objects */
    SelectObject(dev->hdc, brush_dest);
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

static int plg_blt(PHYSDEV dev, const EMRPLGBLT *p)
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

    GetTransform(dev->hdc, 0x203, &xform_dest);
    SetWorldTransform(dev->hdc, &xf);
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
    mask_blt(dev, &maskblt, src_bi, src_bits, mask_bi, mask_bits);
    SetWorldTransform(dev->hdc, &xform_dest);
    return TRUE;
}

static int poly_draw(PHYSDEV dev, const POINT *points, const BYTE *types, DWORD count)
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

    GetCurrentPositionEx(dev->hdc, &cur);
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
            if (!PSDRV_PolyPolyline(dev, pts, &num_pts, 1))
                return FALSE;
            break;
        case PT_BEZIERTO:
            pts[0] = cur;
            pts[1] = points[i];
            pts[2] = points[i + 1];
            pts[3] = points[i + 2];
            if (!PSDRV_PolyBezier(dev, pts, 4))
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
            if (!PSDRV_PolyPolyline(dev, pts, &num_pts, 1))
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

static BOOL gradient_fill(PHYSDEV dev, const TRIVERTEX *vert_array, DWORD nvert,
        const void *grad_array, DWORD ngrad, ULONG mode)
{
    char buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    struct bitblt_coords src, dst;
    struct gdi_image_bits bits;
    HBITMAP bmp, old_bmp;
    BOOL ret = FALSE;
    TRIVERTEX *pts;
    unsigned int i;
    HDC hdc_src;
    HRGN rgn;

    if (!(pts = malloc(nvert * sizeof(*pts)))) return FALSE;
    memcpy(pts, vert_array, sizeof(*pts) * nvert);
    for (i = 0; i < nvert; i++)
        LPtoDP(dev->hdc, (POINT *)&pts[i], 1);

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
    clip_visrect(dev->hdc, &dst.visrect, &dst.visrect);

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
        ret = (PSDRV_PutImage(dev, rgn, info, &bits, &src, &dst, SRCCOPY) == ERROR_SUCCESS);
    DeleteObject(rgn);
    DeleteObject(bmp);
    return ret;
}

static HGDIOBJ get_object_handle(struct pp_data *data, HANDLETABLE *handletable,
        DWORD i, struct brush_pattern **pattern)
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
    struct brush_pattern *pattern = NULL;
    int i;

    for (i = 0; i < handle_count; i++)
    {
        if (htable->objectHandle[i] == brush)
        {
            pattern = data->patterns + i;
            break;
        }
    }

    return PSDRV_SelectBrush(&data->pdev->dev, brush, pattern) != NULL;
}

static BOOL fill_rgn(struct pp_data *data, HANDLETABLE *htable, int handle_count, DWORD brush, HRGN rgn)
{
    struct brush_pattern *pattern;
    HBRUSH hbrush;
    int ret;

    hbrush = get_object_handle(data, htable, brush, &pattern);
    PSDRV_SelectBrush(&data->pdev->dev, hbrush, pattern);
    ret = PSDRV_PaintRgn(&data->pdev->dev, rgn);
    select_hbrush(data, htable, handle_count, GetCurrentObject(data->pdev->dev.hdc, OBJ_BRUSH));
    PSDRV_SelectBrush(&data->pdev->dev, hbrush, pattern);
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
        const ENHMETARECORD *rec, int n, LPARAM arg)
{
    struct pp_data *data = (struct pp_data *)arg;

    if (data->path && is_path_record(rec->iType))
        return PlayEnhMetaFileRecord(data->pdev->dev.hdc, htable, rec, n);

    switch (rec->iType)
    {
    case EMR_HEADER:
    {
        const ENHMETAHEADER *header = (const ENHMETAHEADER *)rec;

        data->patterns = calloc(sizeof(*data->patterns), header->nHandles);
        return data->patterns && PSDRV_StartPage(&data->pdev->dev);
    }
    case EMR_POLYBEZIER:
    {
        const EMRPOLYBEZIER *p = (const EMRPOLYBEZIER *)rec;

        return PSDRV_PolyBezier(&data->pdev->dev, (const POINT *)p->aptl, p->cptl);
    }
    case EMR_POLYGON:
    {
        const EMRPOLYGON *p = (const EMRPOLYGON *)rec;

        return PSDRV_PolyPolygon(&data->pdev->dev, (const POINT *)p->aptl,
                (const INT *)&p->cptl, 1);
    }
    case EMR_POLYLINE:
    {
        const EMRPOLYLINE *p = (const EMRPOLYLINE *)rec;

        return PSDRV_PolyPolyline(&data->pdev->dev,
                (const POINT *)p->aptl, &p->cptl, 1);
    }
    case EMR_POLYBEZIERTO:
    {
        const EMRPOLYBEZIERTO *p = (const EMRPOLYBEZIERTO *)rec;

        return PSDRV_PolyBezierTo(&data->pdev->dev, (const POINT *)p->aptl, p->cptl) &&
            MoveToEx(data->pdev->dev.hdc, p->aptl[p->cptl - 1].x, p->aptl[p->cptl - 1].y, NULL);
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

        GetCurrentPositionEx(data->pdev->dev.hdc, pts);
        memcpy(pts + 1, p->aptl, sizeof(*pts) * p->cptl);
        ret = PSDRV_PolyPolyline(&data->pdev->dev, pts, &cnt, 1) &&
            MoveToEx(data->pdev->dev.hdc, pts[cnt - 1].x, pts[cnt - 1].y, NULL);
        free(pts);
        return ret;
    }
    case EMR_POLYPOLYLINE:
    {
        const EMRPOLYPOLYLINE *p = (const EMRPOLYPOLYLINE *)rec;

        return PSDRV_PolyPolyline(&data->pdev->dev,
                (const POINT *)(p->aPolyCounts + p->nPolys),
                p->aPolyCounts, p->nPolys);
    }
    case EMR_POLYPOLYGON:
    {
        const EMRPOLYPOLYGON *p = (const EMRPOLYPOLYGON *)rec;

        return PSDRV_PolyPolygon(&data->pdev->dev,
                (const POINT *)(p->aPolyCounts + p->nPolys),
                (const INT *)p->aPolyCounts, p->nPolys);
    }
    case EMR_EOF:
        return PSDRV_EndPage(&data->pdev->dev);
    case EMR_SETPIXELV:
    {
        const EMRSETPIXELV *p = (const EMRSETPIXELV *)rec;

        return PSDRV_SetPixel(&data->pdev->dev, p->ptlPixel.x,
                p->ptlPixel.y, p->crColor);
    }
    case EMR_SETTEXTCOLOR:
    {
        const EMRSETTEXTCOLOR *p = (const EMRSETTEXTCOLOR *)rec;

        SetTextColor(data->pdev->dev.hdc, p->crColor);
        PSDRV_SetTextColor(&data->pdev->dev, p->crColor);
        return 1;
    }
    case EMR_SETBKCOLOR:
    {
        const EMRSETBKCOLOR *p = (const EMRSETBKCOLOR *)rec;

        SetBkColor(data->pdev->dev.hdc, p->crColor);
        PSDRV_SetBkColor(&data->pdev->dev, p->crColor);
        return 1;
    }
    case EMR_RESTOREDC:
    {
        HDC hdc = data->pdev->dev.hdc;
        int ret = PlayEnhMetaFileRecord(hdc, htable, rec, n);

        select_hbrush(data, htable, n, GetCurrentObject(hdc, OBJ_BRUSH));
        /* TODO: reselect font */
        PSDRV_SelectPen(&data->pdev->dev, GetCurrentObject(hdc, OBJ_PEN), NULL);
        PSDRV_SetBkColor(&data->pdev->dev, GetBkColor(hdc));
        PSDRV_SetTextColor(&data->pdev->dev, GetTextColor(hdc));
        return ret;
    }
    case EMR_SELECTOBJECT:
    {
        const EMRSELECTOBJECT *so = (const EMRSELECTOBJECT *)rec;
        struct brush_pattern *pattern;
        HGDIOBJ obj;

        obj = get_object_handle(data, htable, so->ihObject, &pattern);
        SelectObject(data->pdev->dev.hdc, obj);

        switch (GetObjectType(obj))
        {
        case OBJ_PEN: return PSDRV_SelectPen(&data->pdev->dev, obj, NULL) != NULL;
        case OBJ_BRUSH: return PSDRV_SelectBrush(&data->pdev->dev, obj, pattern) != NULL;
        default:
            FIXME("unhandled object type %ld\n", GetObjectType(obj));
            return 1;
        }
    }
    case EMR_DELETEOBJECT:
    {
        const EMRDELETEOBJECT *p = (const EMRDELETEOBJECT *)rec;

        memset(&data->patterns[p->ihObject], 0, sizeof(*data->patterns));
        return PlayEnhMetaFileRecord(data->pdev->dev.hdc, htable, rec, n);
    }
    case EMR_ANGLEARC:
    {
        const EMRANGLEARC *p = (const EMRANGLEARC *)rec;
        int arc_dir = SetArcDirection(data->pdev->dev.hdc,
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

        ret = hmf_proc(hdc, htable, (ENHMETARECORD *)&arcto, n, arg);
        SetArcDirection(data->pdev->dev.hdc, arc_dir);
        return ret;
    }
    case EMR_ELLIPSE:
    {
        const EMRELLIPSE *p = (const EMRELLIPSE *)rec;
        const RECTL *r = &p->rclBox;

        return PSDRV_Ellipse(&data->pdev->dev, r->left, r->top, r->right, r->bottom);
    }
    case EMR_RECTANGLE:
    {
        const EMRRECTANGLE *rect = (const EMRRECTANGLE *)rec;

        return PSDRV_Rectangle(&data->pdev->dev, rect->rclBox.left,
                rect->rclBox.top, rect->rclBox.right, rect->rclBox.bottom);
    }
    case EMR_ROUNDRECT:
    {
        const EMRROUNDRECT *p = (const EMRROUNDRECT *)rec;

        return PSDRV_RoundRect(&data->pdev->dev, p->rclBox.left,
                p->rclBox.top, p->rclBox.right, p->rclBox.bottom,
                p->szlCorner.cx, p->szlCorner.cy);
    }
    case EMR_ARC:
    {
        const EMRARC *p = (const EMRARC *)rec;

        return PSDRV_Arc(&data->pdev->dev, p->rclBox.left, p->rclBox.top,
                p->rclBox.right, p->rclBox.bottom, p->ptlStart.x,
                p->ptlStart.y, p->ptlEnd.x, p->ptlEnd.y);
    }
    case EMR_CHORD:
    {
        const EMRCHORD *p = (const EMRCHORD *)rec;

        return PSDRV_Chord(&data->pdev->dev, p->rclBox.left, p->rclBox.top,
                p->rclBox.right, p->rclBox.bottom, p->ptlStart.x,
                p->ptlStart.y, p->ptlEnd.x, p->ptlEnd.y);
    }
    case EMR_PIE:
    {
        const EMRPIE *p = (const EMRPIE *)rec;

        return PSDRV_Pie(&data->pdev->dev, p->rclBox.left, p->rclBox.top,
                p->rclBox.right, p->rclBox.bottom, p->ptlStart.x,
                p->ptlStart.y, p->ptlEnd.x, p->ptlEnd.y);
    }
    case EMR_LINETO:
    {
        const EMRLINETO *line = (const EMRLINETO *)rec;

        return PSDRV_LineTo(&data->pdev->dev, line->ptl.x, line->ptl.y) &&
            MoveToEx(data->pdev->dev.hdc, line->ptl.x, line->ptl.y, NULL);
    }
    case EMR_ARCTO:
    {
        const EMRARCTO *p = (const EMRARCTO *)rec;
        POINT pt;
        BOOL ret;

        ret = GetCurrentPositionEx(data->pdev->dev.hdc, &pt);
        if (ret)
        {
            ret = ArcTo(data->pdev->dev.hdc, p->rclBox.left, p->rclBox.top,
                    p->rclBox.right, p->rclBox.bottom, p->ptlStart.x,
                    p->ptlStart.y, p->ptlStart.x, p->ptlStart.y);
        }
        if (ret)
            ret = PSDRV_LineTo(&data->pdev->dev, pt.x, pt.y);
        if (ret)
        {
            ret = PSDRV_Arc(&data->pdev->dev, p->rclBox.left, p->rclBox.top,
                    p->rclBox.right, p->rclBox.bottom, p->ptlStart.x,
                    p->ptlStart.y, p->ptlEnd.x, p->ptlEnd.y);
        }
        if (ret)
        {
            ret = ArcTo(data->pdev->dev.hdc, p->rclBox.left, p->rclBox.top,
                    p->rclBox.right, p->rclBox.bottom, p->ptlEnd.x,
                    p->ptlEnd.y, p->ptlEnd.x, p->ptlEnd.y);
        }
        return ret;
    }
    case EMR_POLYDRAW:
    {
        const EMRPOLYDRAW *p = (const EMRPOLYDRAW *)rec;
        const POINT *pts = (const POINT *)p->aptl;

        return poly_draw(&data->pdev->dev, pts, (BYTE *)(p->aptl + p->cptl), p->cptl) &&
            MoveToEx(data->pdev->dev.hdc, pts[p->cptl - 1].x, pts[p->cptl - 1].y, NULL);
    }
    case EMR_BEGINPATH:
    {
        data->path = TRUE;
        return PlayEnhMetaFileRecord(data->pdev->dev.hdc, htable, rec, n);
    }
    case EMR_ENDPATH:
    {
        data->path = FALSE;
        return PlayEnhMetaFileRecord(data->pdev->dev.hdc, htable, rec, n);
    }
    case EMR_FILLPATH:
        return PSDRV_FillPath(&data->pdev->dev);
    case EMR_STROKEANDFILLPATH:
        return PSDRV_StrokeAndFillPath(&data->pdev->dev);
    case EMR_STROKEPATH:
        return PSDRV_StrokePath(&data->pdev->dev);
    case EMR_ABORTPATH:
    {
        data->path = FALSE;
        return PlayEnhMetaFileRecord(data->pdev->dev.hdc, htable, rec, n);
    }
    case EMR_FILLRGN:
    {
        const EMRFILLRGN *p = (const EMRFILLRGN *)rec;
        HRGN rgn;
        int ret;

        rgn = ExtCreateRegion(NULL, p->cbRgnData, (const RGNDATA *)p->RgnData);
        ret = fill_rgn(data, htable, n, p->ihBrush, rgn);
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

        ret = fill_rgn(data, htable, n, p->ihBrush, frame);
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
        old_rop = SetROP2(data->pdev->dev.hdc, R2_NOT);
        ret = fill_rgn(data, htable, n, 0x80000000 | BLACK_BRUSH, rgn);
        SetROP2(data->pdev->dev.hdc, old_rop);
        DeleteObject(rgn);
        return ret;
    }
    case EMR_PAINTRGN:
    {
        const EMRPAINTRGN *p = (const EMRPAINTRGN *)rec;
        HRGN rgn = ExtCreateRegion(NULL, p->cbRgnData, (const RGNDATA *)p->RgnData);
        int ret;

        ret = PSDRV_PaintRgn(&data->pdev->dev, rgn);
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

        return stretch_blt(&data->pdev->dev, &blt, bi, src_bits);
    }
    case EMR_STRETCHBLT:
    {
        const EMRSTRETCHBLT *p = (const EMRSTRETCHBLT *)rec;
        const BITMAPINFO *bi = (const BITMAPINFO *)((BYTE *)p + p->offBmiSrc);
        const BYTE *src_bits = (BYTE *)p + p->offBitsSrc;

        return stretch_blt(&data->pdev->dev, p, bi, src_bits);
    }
    case EMR_MASKBLT:
    {
        const EMRMASKBLT *p = (const EMRMASKBLT *)rec;
        const BITMAPINFO *mask_bi = (const BITMAPINFO *)((BYTE *)p + p->offBmiMask);
        const BITMAPINFO *src_bi = (const BITMAPINFO *)((BYTE *)p + p->offBmiSrc);
        const BYTE *mask_bits = (BYTE *)p + p->offBitsMask;
        const BYTE *src_bits = (BYTE *)p + p->offBitsSrc;

        return mask_blt(&data->pdev->dev, p, src_bi, src_bits, mask_bi, mask_bits);
    }
    case EMR_PLGBLT:
    {
        const EMRPLGBLT *p = (const EMRPLGBLT *)rec;

        return plg_blt(&data->pdev->dev, p);
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
        i = PSDRV_PolyBezier(&data->pdev->dev, pts, p->cpts);
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
        i = PSDRV_PolyPolygon(&data->pdev->dev, pts, (const INT *)&p->cpts, 1);
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
        i = PSDRV_PolyPolyline(&data->pdev->dev, pts, &p->cpts, 1);
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
        i = PSDRV_PolyBezierTo(&data->pdev->dev, pts, p->cpts) &&
            MoveToEx(data->pdev->dev.hdc, pts[p->cpts - 1].x, pts[p->cpts - 1].y, NULL);
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
        GetCurrentPositionEx(data->pdev->dev.hdc, pts);
        for (i = 0; i < p->cpts; i++)
        {
            pts[i + 1].x = p->apts[i].x;
            pts[i + 1].y = p->apts[i].y;
        }
        i = PSDRV_PolyPolyline(&data->pdev->dev, pts, &cnt, 1) &&
            MoveToEx(data->pdev->dev.hdc, pts[cnt - 1].x, pts[cnt - 1].y, NULL);
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
        i = PSDRV_PolyPolyline(&data->pdev->dev, pts, p->aPolyCounts, p->nPolys);
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
        i = PSDRV_PolyPolygon(&data->pdev->dev, pts, (const INT *)p->aPolyCounts, p->nPolys);
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
            pts[i].x = p->apts[i].x;
            pts[i].y = p->apts[i].y;
        }
        i = poly_draw(&data->pdev->dev, pts, (BYTE *)(p->apts + p->cpts), p->cpts) &&
            MoveToEx(data->pdev->dev.hdc, pts[p->cpts - 1].x, pts[p->cpts - 1].y, NULL);
        free(pts);
        return i;
    }
    case EMR_CREATEMONOBRUSH:
    {
        const EMRCREATEMONOBRUSH *p = (const EMRCREATEMONOBRUSH *)rec;

        if (!PlayEnhMetaFileRecord(data->pdev->dev.hdc, htable, rec, n))
            return 0;
        data->patterns[p->ihBrush].usage = p->iUsage;
        data->patterns[p->ihBrush].info = (BITMAPINFO *)((BYTE *)p + p->offBmi);
        data->patterns[p->ihBrush].bits.ptr = (BYTE *)p + p->offBits;
        return 1;
    }
    case EMR_CREATEDIBPATTERNBRUSHPT:
    {
        const EMRCREATEDIBPATTERNBRUSHPT *p = (const EMRCREATEDIBPATTERNBRUSHPT *)rec;

        if (!PlayEnhMetaFileRecord(data->pdev->dev.hdc, htable, rec, n))
            return 0;
        data->patterns[p->ihBrush].usage = p->iUsage;
        data->patterns[p->ihBrush].info = (BITMAPINFO *)((BYTE *)p + p->offBmi);
        data->patterns[p->ihBrush].bits.ptr = (BYTE *)p + p->offBits;
        return 1;
    }
    case EMR_EXTESCAPE:
    {
        const struct EMREXTESCAPE
        {
            EMR emr;
            DWORD escape;
            DWORD size;
            BYTE data[1];
        } *p = (const struct EMREXTESCAPE *)rec;

        PSDRV_ExtEscape(&data->pdev->dev, p->escape, p->size, p->data, 0, NULL);
        return 1;
    }
    case EMR_GRADIENTFILL:
    {
        const EMRGRADIENTFILL *p = (const EMRGRADIENTFILL *)rec;

        return gradient_fill(&data->pdev->dev, p->Ver, p->nVer,
                p->Ver + p->nVer, p->nTri, p->ulMode);
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
    case EMR_SAVEDC:
    case EMR_SETWORLDTRANSFORM:
    case EMR_MODIFYWORLDTRANSFORM:
    case EMR_CREATEPEN:
    case EMR_CREATEBRUSHINDIRECT:
    case EMR_SETARCDIRECTION:
    case EMR_CLOSEFIGURE:
    case EMR_FLATTENPATH:
    case EMR_WIDENPATH:
    case EMR_SELECTCLIPPATH:
    case EMR_EXTSELECTCLIPRGN:
    case EMR_SETLAYOUT:
    case EMR_SETTEXTJUSTIFICATION:
        return PlayEnhMetaFileRecord(data->pdev->dev.hdc, htable, rec, n);
    default:
        FIXME("unsupported record: %ld\n", rec->iType);
    }

    return 1;
}

static BOOL print_metafile(struct pp_data *data, HANDLE hdata)
{
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

    ret = EnumEnhMetaFile(NULL, hmf, hmf_proc, (void *)data, NULL);
    DeleteEnhMetaFile(hmf);
    free(data->patterns);
    data->patterns = NULL;
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
    data->doc_name = wcsdup(open_data->pDocumentName);
    data->out_file = wcsdup(open_data->pOutputFile);

    hdc = CreateCompatibleDC(NULL);
    if (!hdc)
    {
        LocalFree(data);
        return NULL;
    }
    SetGraphicsMode(hdc, GM_ADVANCED);
    data->pdev = create_psdrv_physdev(hdc, open_data->pPrinterName,
            (const PSDRV_DEVMODE *)open_data->pDevMode);
    if (!data->pdev)
    {
        DeleteDC(hdc);
        LocalFree(data);
        return NULL;
    }
    data->pdev->dev.hdc = hdc;
    return (HANDLE)data;
}

BOOL WINAPI PrintDocumentOnPrintProcessor(HANDLE pp, WCHAR *doc_name)
{
    struct pp_data *data = get_handle_data(pp);
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

    if (!OpenPrinterW(doc_name, &spool_data, NULL))
        return FALSE;

    info.pDocName = data->doc_name;
    info.pOutputFile = data->out_file;
    info.pDatatype = (WCHAR *)L"RAW";
    data->pdev->job.id = StartDocPrinterW(data->hport, 1, (BYTE *)&info);
    if (!data->pdev->job.id)
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

    data->pdev->job.hprinter = data->hport;
    if (!PSDRV_WriteHeader(&data->pdev->dev, data->doc_name))
    {
        WARN("Failed to write header\n");
        goto cleanup;
    }
    data->pdev->job.banding = FALSE;
    data->pdev->job.OutOfPage = TRUE;
    data->pdev->job.PageNo = 0;
    data->pdev->job.quiet = FALSE;
    data->pdev->job.passthrough_state = passthrough_none;
    data->pdev->job.doc_name = strdupW(data->doc_name);

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
                if (r != sizeof(pos))
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
    if (data->pdev->job.PageNo)
        PSDRV_WriteFooter(&data->pdev->dev);

    HeapFree(GetProcessHeap(), 0, data->pdev->job.doc_name);
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
    free(data->doc_name);
    free(data->out_file);
    DeleteDC(data->pdev->dev.hdc);
    HeapFree(GetProcessHeap(), 0, data->pdev->Devmode);
    HeapFree(GetProcessHeap(), 0, data->pdev);

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
