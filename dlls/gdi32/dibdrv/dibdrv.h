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

extern BOOL     dibdrv_LineTo( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL     dibdrv_PatBlt( PHYSDEV dev, struct bitblt_coords *dst, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL     dibdrv_PaintRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL     dibdrv_PolyPolyline( PHYSDEV dev, const POINT* pt, const DWORD* counts,
                                     DWORD polylines ) DECLSPEC_HIDDEN;
extern BOOL     dibdrv_Polyline( PHYSDEV dev, const POINT* pt, INT count ) DECLSPEC_HIDDEN;
extern BOOL     dibdrv_Rectangle( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern HBRUSH   dibdrv_SelectBrush( PHYSDEV dev, HBRUSH hbrush ) DECLSPEC_HIDDEN;
extern HPEN     dibdrv_SelectPen( PHYSDEV dev, HPEN hpen ) DECLSPEC_HIDDEN;
extern COLORREF dibdrv_SetDCBrushColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF dibdrv_SetDCPenColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;

static inline dibdrv_physdev *get_dibdrv_pdev( PHYSDEV dev )
{
    return (dibdrv_physdev *)dev;
}

static inline DC *get_dibdrv_dc( PHYSDEV dev )
{
    return CONTAINING_RECORD( dev, DC, dibdrv );
}

typedef struct primitive_funcs
{
    void        (* solid_rects)(const dib_info *dib, int num, const RECT *rc, DWORD and, DWORD xor);
    void      (* pattern_rects)(const dib_info *dib, int num, const RECT *rc, const POINT *orign, const dib_info *brush, void *and_bits, void *xor_bits);
    void          (* copy_rect)(const dib_info *dst, const RECT *rc, const dib_info *src, const POINT *origin, int rop2);
    DWORD (* colorref_to_pixel)(const dib_info *dib, COLORREF color);
    BOOL         (* convert_to)(dib_info *dst, const dib_info *src, const RECT *src_rect);
    BOOL   (* create_rop_masks)(const dib_info *dib, const dib_info *hatch, const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits);
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

extern void get_rop_codes(INT rop, struct rop_codes *codes);
extern void calc_and_xor_masks(INT rop, DWORD color, DWORD *and, DWORD *xor) DECLSPEC_HIDDEN;
extern void update_brush_rop( dibdrv_physdev *pdev, INT rop ) DECLSPEC_HIDDEN;
extern void reset_dash_origin(dibdrv_physdev *pdev) DECLSPEC_HIDDEN;
extern BOOL init_dib_info_from_packed(dib_info *dib, const BITMAPINFOHEADER *bi, WORD usage, HPALETTE pal) DECLSPEC_HIDDEN;
extern void free_dib_info(dib_info *dib) DECLSPEC_HIDDEN;
extern void free_pattern_brush(dibdrv_physdev *pdev) DECLSPEC_HIDDEN;
extern void copy_dib_color_info(dib_info *dst, const dib_info *src) DECLSPEC_HIDDEN;
extern BOOL convert_dib(dib_info *dst, const dib_info *src) DECLSPEC_HIDDEN;
extern DWORD get_fg_color(dibdrv_physdev *pdev, COLORREF color) DECLSPEC_HIDDEN;

static inline BOOL defer_pen(dibdrv_physdev *pdev)
{
    return pdev->defer & (DEFER_FORMAT | DEFER_PEN);
}

static inline BOOL defer_brush(dibdrv_physdev *pdev)
{
    return pdev->defer & (DEFER_FORMAT | DEFER_BRUSH);
}
