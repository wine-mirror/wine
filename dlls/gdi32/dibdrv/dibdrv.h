/*
 * DIB driver include file.
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

enum dib_info_flags
{
    private_color_table = 1
};

typedef struct
{
    int bit_count, width, height;
    int stride; /* stride in bytes.  Will be -ve for bottom-up dibs (see bits). */
    struct gdi_image_bits bits; /* bits.ptr points to the top-left corner of the dib. */

    DWORD red_mask, green_mask, blue_mask;
    int red_shift, green_shift, blue_shift;
    int red_len, green_len, blue_len;

    RGBQUAD *color_table;
    DWORD color_table_size;

    enum dib_info_flags flags;

    const struct primitive_funcs *funcs;
} dib_info;

typedef struct
{
    DWORD count;
    DWORD dashes[16]; /* 16 is the maximum number for a PS_USERSTYLE pen */
    DWORD total_len;  /* total length of the dashes, should be multiplied by 2 if there are an odd number of dash lengths */
} dash_pattern;

typedef struct
{
    int left_in_dash;
    int cur_dash;
    BOOL mark;
} dash_pos;

typedef struct
{
    DWORD and;
    DWORD xor;
} rop_mask;

typedef struct
{
    void *and;
    void *xor;
} rop_mask_bits;

typedef struct dibdrv_physdev
{
    struct gdi_physdev dev;
    dib_info dib;

    HRGN clip;
    DWORD defer;

    /* pen */
    COLORREF pen_colorref;
    DWORD pen_color, pen_and, pen_xor;
    dash_pattern pen_pattern;
    dash_pos dash_pos;
    BOOL   (* pen_lines)(struct dibdrv_physdev *pdev, int num, POINT *pts);

    /* brush */
    UINT brush_style;
    UINT brush_hatch;
    INT brush_rop;   /* PatBlt, for example, can override the DC's rop2 */
    COLORREF brush_colorref;
    DWORD brush_color, brush_and, brush_xor;
    dib_info brush_dib;
    void *brush_and_bits, *brush_xor_bits;
    BOOL   (* brush_rects)(struct dibdrv_physdev *pdev, dib_info *dib, int num, const RECT *rects, HRGN clip);

    /* background */
    DWORD bkgnd_color, bkgnd_and, bkgnd_xor;
} dibdrv_physdev;

#define DEFER_FORMAT     1
#define DEFER_PEN        2
#define DEFER_BRUSH      4

extern BOOL     dibdrv_AlphaBlend( PHYSDEV dst_dev, struct bitblt_coords *dst,
                                   PHYSDEV src_dev, struct bitblt_coords *src, BLENDFUNCTION blend ) DECLSPEC_HIDDEN;
extern DWORD    dibdrv_BlendImage( PHYSDEV dev, BITMAPINFO *info, const struct gdi_image_bits *bits,
                                   struct bitblt_coords *src, struct bitblt_coords *dst, BLENDFUNCTION func ) DECLSPEC_HIDDEN;
extern DWORD    dibdrv_GetImage( PHYSDEV dev, HBITMAP hbitmap, BITMAPINFO *info,
                                 struct gdi_image_bits *bits, struct bitblt_coords *src ) DECLSPEC_HIDDEN;
extern BOOL     dibdrv_LineTo( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL     dibdrv_PatBlt( PHYSDEV dev, struct bitblt_coords *dst, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL     dibdrv_PaintRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL     dibdrv_PolyPolyline( PHYSDEV dev, const POINT* pt, const DWORD* counts,
                                     DWORD polylines ) DECLSPEC_HIDDEN;
extern BOOL     dibdrv_Polyline( PHYSDEV dev, const POINT* pt, INT count ) DECLSPEC_HIDDEN;
extern DWORD    dibdrv_PutImage( PHYSDEV dev, HBITMAP hbitmap, HRGN clip, BITMAPINFO *info,
                                 const struct gdi_image_bits *bits, struct bitblt_coords *src,
                                 struct bitblt_coords *dst, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL     dibdrv_Rectangle( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern HBRUSH   dibdrv_SelectBrush( PHYSDEV dev, HBRUSH hbrush ) DECLSPEC_HIDDEN;
extern HPEN     dibdrv_SelectPen( PHYSDEV dev, HPEN hpen ) DECLSPEC_HIDDEN;
extern COLORREF dibdrv_SetDCBrushColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF dibdrv_SetDCPenColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL     dibdrv_StretchBlt( PHYSDEV dst_dev, struct bitblt_coords *dst,
                                   PHYSDEV src_dev, struct bitblt_coords *src, DWORD rop ) DECLSPEC_HIDDEN;

static inline dibdrv_physdev *get_dibdrv_pdev( PHYSDEV dev )
{
    return (dibdrv_physdev *)dev;
}

struct stretch_params
{
    int err_start, err_add_1, err_add_2;
    unsigned int length;
    int dst_inc, src_inc;
};

typedef struct primitive_funcs
{
    void            (* solid_rects)(const dib_info *dib, int num, const RECT *rc, DWORD and, DWORD xor);
    void          (* pattern_rects)(const dib_info *dib, int num, const RECT *rc, const POINT *orign,
                                    const dib_info *brush, void *and_bits, void *xor_bits);
    void              (* copy_rect)(const dib_info *dst, const RECT *rc, const dib_info *src,
                                    const POINT *origin, int rop2, int overlap);
    void             (* blend_rect)(const dib_info *dst, const RECT *rc, const dib_info *src,
                                    const POINT *origin, BLENDFUNCTION blend);
    DWORD     (* colorref_to_pixel)(const dib_info *dib, COLORREF color);
    void             (* convert_to)(dib_info *dst, const dib_info *src, const RECT *src_rect);
    BOOL       (* create_rop_masks)(const dib_info *dib, const dib_info *hatch,
                                    const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits);
    void            (* stretch_row)(const dib_info *dst_dib, const POINT *dst_start,
                                    const dib_info *src_dib, const POINT *src_start,
                                    const struct stretch_params *params, int mode, BOOL keep_dst);
    void             (* shrink_row)(const dib_info *dst_dib, const POINT *dst_start,
                                    const dib_info *src_dib, const POINT *src_start,
                                    const struct stretch_params *params, int mode, BOOL keep_dst);
} primitive_funcs;

extern const primitive_funcs funcs_8888 DECLSPEC_HIDDEN;
extern const primitive_funcs funcs_32   DECLSPEC_HIDDEN;
extern const primitive_funcs funcs_24   DECLSPEC_HIDDEN;
extern const primitive_funcs funcs_555  DECLSPEC_HIDDEN;
extern const primitive_funcs funcs_16   DECLSPEC_HIDDEN;
extern const primitive_funcs funcs_8    DECLSPEC_HIDDEN;
extern const primitive_funcs funcs_4    DECLSPEC_HIDDEN;
extern const primitive_funcs funcs_1    DECLSPEC_HIDDEN;
extern const primitive_funcs funcs_null DECLSPEC_HIDDEN;

struct rop_codes
{
    DWORD a1, a2, x1, x2;
};

#define OVERLAP_LEFT  0x01  /* dest starts left of source */
#define OVERLAP_RIGHT 0x02  /* dest starts right of source */
#define OVERLAP_ABOVE 0x04  /* dest starts above source */
#define OVERLAP_BELOW 0x08  /* dest starts below source */

typedef struct
{
    unsigned int dx, dy;
    int bias;
    DWORD octant;
} bres_params;

extern void get_rop_codes(INT rop, struct rop_codes *codes) DECLSPEC_HIDDEN;
extern void calc_and_xor_masks(INT rop, DWORD color, DWORD *and, DWORD *xor) DECLSPEC_HIDDEN;
extern void update_brush_rop( dibdrv_physdev *pdev, INT rop ) DECLSPEC_HIDDEN;
extern void reset_dash_origin(dibdrv_physdev *pdev) DECLSPEC_HIDDEN;
extern BOOL init_dib_info(dib_info *dib, const BITMAPINFOHEADER *bi, const DWORD *bit_fields,
                          RGBQUAD *color_table, int color_table_size, void *bits,
                          enum dib_info_flags flags) DECLSPEC_HIDDEN;
extern BOOL init_dib_info_from_packed(dib_info *dib, const BITMAPINFOHEADER *bi, WORD usage, HPALETTE pal) DECLSPEC_HIDDEN;
extern BOOL init_dib_info_from_bitmapinfo(dib_info *dib, const BITMAPINFO *info, void *bits,
                                          enum dib_info_flags flags) DECLSPEC_HIDDEN;
extern BOOL init_dib_info_from_bitmapobj(dib_info *dib, BITMAPOBJ *bmp, enum dib_info_flags flags) DECLSPEC_HIDDEN;
extern void free_dib_info(dib_info *dib) DECLSPEC_HIDDEN;
extern void free_pattern_brush(dibdrv_physdev *pdev) DECLSPEC_HIDDEN;
extern void copy_dib_color_info(dib_info *dst, const dib_info *src) DECLSPEC_HIDDEN;
extern BOOL convert_dib(dib_info *dst, const dib_info *src) DECLSPEC_HIDDEN;
extern DWORD get_pixel_color(dibdrv_physdev *pdev, COLORREF color, BOOL mono_fixup) DECLSPEC_HIDDEN;
extern BOOL brush_rects( dibdrv_physdev *pdev, int num, const RECT *rects ) DECLSPEC_HIDDEN;
extern HRGN add_extra_clipping_region( dibdrv_physdev *pdev, HRGN rgn ) DECLSPEC_HIDDEN;
extern void restore_clipping_region( dibdrv_physdev *pdev, HRGN rgn ) DECLSPEC_HIDDEN;
extern int clip_line(const POINT *start, const POINT *end, const RECT *clip,
                     const bres_params *params, POINT *pt1, POINT *pt2) DECLSPEC_HIDDEN;

static inline BOOL defer_pen(dibdrv_physdev *pdev)
{
    return pdev->defer & (DEFER_FORMAT | DEFER_PEN);
}

static inline BOOL defer_brush(dibdrv_physdev *pdev)
{
    return pdev->defer & (DEFER_FORMAT | DEFER_BRUSH);
}
