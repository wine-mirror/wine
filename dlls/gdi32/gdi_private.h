/*
 * GDI definitions
 *
 * Copyright 1993 Alexandre Julliard
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#ifndef __WINE_GDI_PRIVATE_H
#define __WINE_GDI_PRIVATE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"

void set_gdi_client_ptr( HGDIOBJ handle, void *ptr ) DECLSPEC_HIDDEN;
void *get_gdi_client_ptr( HGDIOBJ handle, WORD type ) DECLSPEC_HIDDEN;

static inline WORD gdi_handle_type( HGDIOBJ obj )
{
    unsigned int handle = HandleToULong( obj );
    return (handle & NTGDI_HANDLE_TYPE_MASK) >> NTGDI_HANDLE_TYPE_SHIFT;
}

static inline BOOL is_meta_dc( HDC hdc )
{
    return gdi_handle_type( hdc ) == NTGDI_OBJ_METADC;
}

extern BOOL METADC_Arc( HDC hdc, INT left, INT top, INT right, INT bottom,
                        INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL METADC_Chord( HDC hdc, INT left, INT top, INT right, INT bottom, INT xstart,
                          INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL METADC_Ellipse( HDC hdc, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL METADC_ExtFloodFill( HDC hdc, INT x, INT y, COLORREF color,
                                 UINT fill_type ) DECLSPEC_HIDDEN;
extern BOOL METADC_ExtTextOut( HDC hdc, INT x, INT y, UINT flags, const RECT *rect,
                               const WCHAR *str, UINT count, const INT *dx ) DECLSPEC_HIDDEN;
extern BOOL METADC_FillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush ) DECLSPEC_HIDDEN;
extern BOOL METADC_FrameRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT  METADC_GetDeviceCaps( HDC hdc, INT cap );
extern BOOL METADC_InvertRgn( HDC hdc, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL METADC_LineTo( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_MoveTo( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_PaintRgn( HDC hdc, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL METADC_Pie( HDC hdc, INT left, INT top, INT right, INT bottom,
                        INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL METADC_PolyPolygon( HDC hdc, const POINT *points, const INT *counts,
                                UINT polygons ) DECLSPEC_HIDDEN;
extern BOOL METADC_Polygon( HDC hdc, const POINT *points, INT count ) DECLSPEC_HIDDEN;
extern BOOL METADC_Polyline( HDC hdc, const POINT *points,INT count) DECLSPEC_HIDDEN;
extern BOOL METADC_Rectangle( HDC hdc, INT left, INT top, INT right, INT bottom) DECLSPEC_HIDDEN;
extern BOOL METADC_RoundRect( HDC hdc, INT left, INT top, INT right, INT bottom,
                              INT ell_width, INT ell_height ) DECLSPEC_HIDDEN;
extern BOOL METADC_SaveDC( HDC hdc ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetBkMode( HDC hdc, INT mode ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetPixel( HDC hdc, INT x, INT y, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetPolyFillMode( HDC hdc, INT mode ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetRelAbs( HDC hdc, INT mode ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetROP2( HDC hdc, INT rop ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetStretchBltMode( HDC hdc, INT mode ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetTextAlign( HDC hdc, UINT align ) DECLSPEC_HIDDEN;

/* enhanced metafiles */
extern BOOL EMFDC_AbortPath( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_AngleArc( DC_ATTR *dc_attr, INT x, INT y, DWORD radius, FLOAT start,
                            FLOAT sweep ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ArcChordPie( DC_ATTR *dc_attr, INT left, INT top, INT right,
                               INT bottom, INT xstart, INT ystart, INT xend,
                               INT yend, DWORD type ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_BeginPath( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_CloseFigure( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_Ellipse( DC_ATTR *dc_attr, INT left, INT top, INT right,
                           INT bottom ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_EndPath( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ExtFloodFill( DC_ATTR *dc_attr, INT x, INT y, COLORREF color,
                                UINT fill_type ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ExtTextOut( DC_ATTR *dc_attr, INT x, INT y, UINT flags, const RECT *rect,
                              const WCHAR *str, UINT count, const INT *dx ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_FillRgn( DC_ATTR *dc_attr, HRGN hrgn, HBRUSH hbrush ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_FrameRgn( DC_ATTR *dc_attr, HRGN hrgn, HBRUSH hbrush, INT width,
                            INT height ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_GradientFill( DC_ATTR *dc_attr, TRIVERTEX *vert_array, ULONG nvert,
                                void *grad_array, ULONG ngrad, ULONG mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_InvertRgn( DC_ATTR *dc_attr, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_LineTo( DC_ATTR *dc_attr, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_MoveTo( DC_ATTR *dc_attr, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_PaintRgn( DC_ATTR *dc_attr, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_PolyBezier( DC_ATTR *dc_attr, const POINT *points, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_PolyBezierTo( DC_ATTR *dc_attr, const POINT *points, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_PolyDraw( DC_ATTR *dc_attr, const POINT *points, const BYTE *types,
                            DWORD count ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_PolyPolyline( DC_ATTR *dc_attr, const POINT *points, const DWORD *counts,
                                DWORD polys ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_PolyPolygon( DC_ATTR *dc_attr, const POINT *points, const INT *counts,
                               UINT polys ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_Polygon( DC_ATTR *dc_attr, const POINT *points, INT count ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_Polyline( DC_ATTR *dc_attr, const POINT *points, INT count) DECLSPEC_HIDDEN;
extern BOOL EMFDC_PolylineTo( DC_ATTR *dc_attr, const POINT *points, INT count ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_Rectangle( DC_ATTR *dc_attr, INT left, INT top, INT right,
                             INT bottom) DECLSPEC_HIDDEN;
extern BOOL EMFDC_RoundRect( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom,
                             INT ell_width, INT ell_height ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SaveDC( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetArcDirection( DC_ATTR *dc_attr, INT dir ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetBkMode( DC_ATTR *dc_attr, INT mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetPixel( DC_ATTR *dc_attr, INT x, INT y, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetPolyFillMode( DC_ATTR *dc_attr, INT mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetROP2( DC_ATTR *dc_attr, INT rop ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetStretchBltMode( DC_ATTR *dc_attr, INT mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetTextAlign( DC_ATTR *dc_attr, UINT align ) DECLSPEC_HIDDEN;

#endif /* __WINE_GDI_PRIVATE_H */
