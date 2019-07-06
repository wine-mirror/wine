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

typedef struct
{
    int bit_count, width, height;
    int compression;
    RECT rect;  /* visible rectangle relative to bitmap origin */
    int stride; /* stride in bytes.  Will be -ve for bottom-up dibs (see bits). */
    struct gdi_image_bits bits; /* bits.ptr points to the top-left corner of the dib. */

    DWORD red_mask, green_mask, blue_mask;
    int red_shift, green_shift, blue_shift;
    int red_len, green_len, blue_len;

    const RGBQUAD *color_table;
    DWORD color_table_size;

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

struct dibdrv_physdev;
struct cached_font;

typedef struct dib_brush
{
    UINT     style;
    UINT     hatch;
    INT      rop;   /* rop2 last used to create the brush bits */
    COLORREF colorref;
    dib_info dib;
    rop_mask_bits masks;
    struct brush_pattern pattern;
    BOOL (*rects)(struct dibdrv_physdev *pdev, struct dib_brush *brush, dib_info *dib,
                  int num, const RECT *rects, const POINT *brush_org, INT rop);
} dib_brush;

struct intensity_range
{
    BYTE r_min, r_max;
    BYTE g_min, g_max;
    BYTE b_min, b_max;
};

struct font_intensities
{
    struct intensity_range ranges[17];
    struct font_gamma_ramp *gamma_ramp;
};

typedef struct dibdrv_physdev
{
    struct gdi_physdev dev;
    dib_info dib;
    dib_brush brush;

    HRGN clip;
    RECT *bounds;
    struct cached_font *font;

    /* pen */
    DWORD pen_style, pen_endcap, pen_join;
    BOOL pen_uses_region, pen_is_ext;
    int pen_width;
    dib_brush pen_brush;
    dash_pattern pen_pattern;
    dash_pos dash_pos;
    rop_mask dash_masks[2];
    BOOL   (* pen_lines)(struct dibdrv_physdev *pdev, int num, POINT *pts, BOOL close, HRGN region);
} dibdrv_physdev;

extern BOOL     CDECL dibdrv_AlphaBlend( PHYSDEV dst_dev, struct bitblt_coords *dst,
                                         PHYSDEV src_dev, struct bitblt_coords *src, BLENDFUNCTION blend ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                                  INT start_x, INT start_y, INT end_x, INT end_y ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_ArcTo( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                                    INT start_x, INT start_y, INT end_x, INT end_y ) DECLSPEC_HIDDEN;
extern DWORD    CDECL dibdrv_BlendImage( PHYSDEV dev, BITMAPINFO *info, const struct gdi_image_bits *bits,
                                         struct bitblt_coords *src, struct bitblt_coords *dst, BLENDFUNCTION func ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                                    INT start_x, INT start_y, INT end_x, INT end_y ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT type ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags,
                                         const RECT *rect, LPCWSTR str, UINT count, const INT *dx ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_FillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern DWORD    CDECL dibdrv_GetImage( PHYSDEV dev, BITMAPINFO *info, struct gdi_image_bits *bits,
                                       struct bitblt_coords *src ) DECLSPEC_HIDDEN;
extern COLORREF CDECL dibdrv_GetNearestColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF CDECL dibdrv_GetPixel( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_GradientFill( PHYSDEV dev, TRIVERTEX *vert_array, ULONG nvert,
                                           void *grad_array, ULONG ngrad, ULONG mode ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_LineTo( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_PatBlt( PHYSDEV dev, struct bitblt_coords *dst, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_PaintRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                                  INT start_x, INT start_y, INT end_x, INT end_y ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_PolyPolygon( PHYSDEV dev, const POINT *pt, const INT *counts, DWORD polygons ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_PolyPolyline( PHYSDEV dev, const POINT* pt, const DWORD* counts,
                                           DWORD polylines ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_Polygon( PHYSDEV dev, const POINT *pt, INT count ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_Polyline( PHYSDEV dev, const POINT* pt, INT count ) DECLSPEC_HIDDEN;
extern DWORD    CDECL dibdrv_PutImage( PHYSDEV dev, HRGN clip, BITMAPINFO *info,
                                       const struct gdi_image_bits *bits, struct bitblt_coords *src,
                                       struct bitblt_coords *dst, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_Rectangle( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_RoundRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                                        INT ellipse_width, INT ellipse_height ) DECLSPEC_HIDDEN;
extern HBRUSH   CDECL dibdrv_SelectBrush( PHYSDEV dev, HBRUSH hbrush, const struct brush_pattern *pattern ) DECLSPEC_HIDDEN;
extern HFONT    CDECL dibdrv_SelectFont( PHYSDEV dev, HFONT font, UINT *aa_flags ) DECLSPEC_HIDDEN;
extern HPEN     CDECL dibdrv_SelectPen( PHYSDEV dev, HPEN hpen, const struct brush_pattern *pattern ) DECLSPEC_HIDDEN;
extern COLORREF CDECL dibdrv_SetDCBrushColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF CDECL dibdrv_SetDCPenColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF CDECL dibdrv_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_StretchBlt( PHYSDEV dst_dev, struct bitblt_coords *dst,
                                         PHYSDEV src_dev, struct bitblt_coords *src, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_StrokeAndFillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     CDECL dibdrv_StrokePath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern struct opengl_funcs * CDECL dibdrv_wine_get_wgl_driver( PHYSDEV dev, UINT version ) DECLSPEC_HIDDEN;

static inline dibdrv_physdev *get_dibdrv_pdev( PHYSDEV dev )
{
    return (dibdrv_physdev *)dev;
}

struct line_params
{
    int err_start, err_add_1, err_add_2, bias;
    unsigned int length;
    int x_inc, y_inc;
    BOOL x_major;
};

struct stretch_params
{
    int err_start, err_add_1, err_add_2;
    unsigned int length;
    int dst_inc, src_inc;
};

typedef struct primitive_funcs
{
    void            (* solid_rects)(const dib_info *dib, int num, const RECT *rc, DWORD and, DWORD xor);
    void             (* solid_line)(const dib_info *dib, const POINT *start, const struct line_params *params,
                                    DWORD and, DWORD xor);
    void          (* pattern_rects)(const dib_info *dib, int num, const RECT *rc, const POINT *origin,
                                    const dib_info *brush, const rop_mask_bits *bits);
    void              (* copy_rect)(const dib_info *dst, const RECT *rc, const dib_info *src,
                                    const POINT *origin, int rop2, int overlap);
    void             (* blend_rect)(const dib_info *dst, const RECT *rc, const dib_info *src,
                                    const POINT *origin, BLENDFUNCTION blend);
    BOOL          (* gradient_rect)(const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode);
    void              (* mask_rect)(const dib_info *dst, const RECT *rc, const dib_info *src,
                                    const POINT *origin, int rop2);
    void             (* draw_glyph)(const dib_info *dst, const RECT *rc, const dib_info *glyph,
                                    const POINT *origin, DWORD text_pixel, const struct intensity_range *ranges);
    void    (* draw_subpixel_glyph)(const dib_info *dst, const RECT *rc, const dib_info *glyph,
                                    const POINT *origin, DWORD text_pixel, const struct font_gamma_ramp *gamma_ramp);
    DWORD             (* get_pixel)(const dib_info *dib, int x, int y);
    DWORD     (* colorref_to_pixel)(const dib_info *dib, COLORREF color);
    COLORREF  (* pixel_to_colorref)(const dib_info *dib, DWORD pixel);
    void             (* convert_to)(dib_info *dst, const dib_info *src, const RECT *src_rect, BOOL dither);
    void       (* create_rop_masks)(const dib_info *dib, const BYTE *hatch_ptr,
                                    const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits);
    void    (* create_dither_masks)(const dib_info *dib, int rop2, COLORREF color, rop_mask_bits *bits);
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

struct clipped_rects
{
    RECT *rects;
    int   count;
    RECT  buffer[32];
};

extern void get_rop_codes(INT rop, struct rop_codes *codes) DECLSPEC_HIDDEN;
extern void reset_dash_origin(dibdrv_physdev *pdev) DECLSPEC_HIDDEN;
extern void init_dib_info_from_bitmapinfo(dib_info *dib, const BITMAPINFO *info, void *bits) DECLSPEC_HIDDEN;
extern BOOL init_dib_info_from_bitmapobj(dib_info *dib, BITMAPOBJ *bmp) DECLSPEC_HIDDEN;
extern void free_dib_info(dib_info *dib) DECLSPEC_HIDDEN;
extern void free_pattern_brush(dib_brush *brush) DECLSPEC_HIDDEN;
extern void copy_dib_color_info(dib_info *dst, const dib_info *src) DECLSPEC_HIDDEN;
extern BOOL convert_dib(dib_info *dst, const dib_info *src) DECLSPEC_HIDDEN;
extern DWORD get_pixel_color( DC *dc, const dib_info *dib, COLORREF color, BOOL mono_fixup ) DECLSPEC_HIDDEN;
extern int get_dib_rect( const dib_info *dib, RECT *rc ) DECLSPEC_HIDDEN;
extern int clip_rect_to_dib( const dib_info *dib, RECT *rc ) DECLSPEC_HIDDEN;
extern int get_clipped_rects( const dib_info *dib, const RECT *rc, HRGN clip, struct clipped_rects *clip_rects ) DECLSPEC_HIDDEN;
extern void add_clipped_bounds( dibdrv_physdev *dev, const RECT *rect, HRGN clip ) DECLSPEC_HIDDEN;
extern int clip_line(const POINT *start, const POINT *end, const RECT *clip,
                     const bres_params *params, POINT *pt1, POINT *pt2) DECLSPEC_HIDDEN;
extern void release_cached_font( struct cached_font *font ) DECLSPEC_HIDDEN;
extern BOOL fill_with_pixel( DC *dc, dib_info *dib, DWORD pixel, int num, const RECT *rects, INT rop ) DECLSPEC_HIDDEN;

static inline void init_clipped_rects( struct clipped_rects *clip_rects )
{
    clip_rects->count = 0;
    clip_rects->rects = clip_rects->buffer;
}

static inline void free_clipped_rects( struct clipped_rects *clip_rects )
{
    if (clip_rects->rects != clip_rects->buffer) HeapFree( GetProcessHeap(), 0, clip_rects->rects );
}

/* compute the x coordinate corresponding to y on the specified edge */
static inline int edge_coord( int y, int x1, int y1, int x2, int y2 )
{
    if (x2 > x1)  /* always follow the edge from right to left to get correct rounding */
        return x2 + (y - y2) * (x2 - x1) / (y2 - y1);
    else
        return x1 + (y - y1) * (x2 - x1) / (y2 - y1);
}

static inline const RGBQUAD *get_dib_color_table( const dib_info *dib )
{
    return dib->color_table ? dib->color_table : get_default_color_table( dib->bit_count );
}
