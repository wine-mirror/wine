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

extern BOOL     CDECL dibdrv_LineTo( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern HPEN     CDECL dibdrv_SelectPen( PHYSDEV dev, HPEN hpen ) DECLSPEC_HIDDEN;
extern COLORREF CDECL dibdrv_SetDCPenColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;

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
    void        (* solid_rects)(const dib_info *dib, int num, RECT *rc, DWORD and, DWORD xor);
    DWORD (* colorref_to_pixel)(const dib_info *dib, COLORREF color);
} primitive_funcs;

extern const primitive_funcs funcs_8888 DECLSPEC_HIDDEN;
extern const primitive_funcs funcs_32   DECLSPEC_HIDDEN;
extern const primitive_funcs funcs_null DECLSPEC_HIDDEN;

extern void calc_and_xor_masks(INT rop, DWORD color, DWORD *and, DWORD *xor) DECLSPEC_HIDDEN;
