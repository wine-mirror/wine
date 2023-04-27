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
#include <limits.h>

#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"

void set_gdi_client_ptr( HGDIOBJ handle, void *ptr ) DECLSPEC_HIDDEN;
void *get_gdi_client_ptr( HGDIOBJ handle, DWORD type ) DECLSPEC_HIDDEN;
DC_ATTR *get_dc_attr( HDC hdc ) DECLSPEC_HIDDEN;
HGDIOBJ get_full_gdi_handle( HGDIOBJ handle ) DECLSPEC_HIDDEN;
void GDI_hdc_using_object( HGDIOBJ obj, HDC hdc,
                           void (*delete)( HDC hdc, HGDIOBJ handle )) DECLSPEC_HIDDEN;
void GDI_hdc_not_using_object( HGDIOBJ obj, HDC hdc ) DECLSPEC_HIDDEN;

static inline DWORD gdi_handle_type( HGDIOBJ obj )
{
    unsigned int handle = HandleToULong( obj );
    return handle & NTGDI_HANDLE_TYPE_MASK;
}

/* metafile defines */

#define META_EOF 0x0000

#define METAFILE_MEMORY 1
#define METAFILE_DISK   2

#define MFVERSION 0x300

/* Undocumented value for DIB's iUsage: Indicates a mono DIB w/o pal entries */
#define DIB_PAL_INDICES 2

/* Format of comment record added by GetWinMetaFileBits */
#include <pshpack2.h>
typedef struct
{
    DWORD comment_id;   /* WMFC */
    DWORD comment_type; /* Always 0x00000001 */
    DWORD version;      /* Always 0x00010000 */
    WORD checksum;
    DWORD flags;        /* Always 0 */
    DWORD num_chunks;
    DWORD chunk_size;
    DWORD remaining_size;
    DWORD emf_size;
    BYTE emf_data[1];
} emf_in_wmf_comment;
#include <poppack.h>

static inline BOOL is_meta_dc( HDC hdc )
{
    return gdi_handle_type( hdc ) == NTGDI_OBJ_METADC;
}

extern BOOL METADC_Arc( HDC hdc, INT left, INT top, INT right, INT bottom,
                        INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL METADC_BitBlt( HDC hdc_dst, INT x_dst, INT y_dst, INT width, INT height,
                           HDC hdc_src, INT x_src, INT y_src, DWORD rop );
extern BOOL METADC_Chord( HDC hdc, INT left, INT top, INT right, INT bottom, INT xstart,
                          INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL METADC_DeleteDC( HDC hdc );
extern BOOL METADC_Ellipse( HDC hdc, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL METADC_ExcludeClipRect( HDC hdc, INT left, INT top, INT right,
                                    INT bottom ) DECLSPEC_HIDDEN;
extern BOOL METADC_ExtEscape( HDC hdc, INT escape, INT input_size, const void *input,
                              INT output_size, void *output ) DECLSPEC_HIDDEN;
extern BOOL METADC_ExtFloodFill( HDC hdc, INT x, INT y, COLORREF color,
                                 UINT fill_type ) DECLSPEC_HIDDEN;
extern BOOL METADC_ExtSelectClipRgn( HDC hdc, HRGN hrgn, INT mode ) DECLSPEC_HIDDEN;
extern BOOL METADC_ExtTextOut( HDC hdc, INT x, INT y, UINT flags, const RECT *rect,
                               const WCHAR *str, UINT count, const INT *dx ) DECLSPEC_HIDDEN;
extern BOOL METADC_FillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush ) DECLSPEC_HIDDEN;
extern BOOL METADC_FrameRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT  METADC_GetDeviceCaps( HDC hdc, INT cap );
extern BOOL METADC_IntersectClipRect( HDC hdc, INT left, INT top, INT right,
                                      INT bottom ) DECLSPEC_HIDDEN;
extern BOOL METADC_InvertRgn( HDC hdc, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL METADC_LineTo( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_MoveTo( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_OffsetClipRgn( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_OffsetViewportOrgEx( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_OffsetWindowOrgEx( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_PaintRgn( HDC hdc, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL METADC_PatBlt( HDC hdc, INT left, INT top, INT width, INT height, DWORD rop );
extern BOOL METADC_Pie( HDC hdc, INT left, INT top, INT right, INT bottom,
                        INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL METADC_PolyPolygon( HDC hdc, const POINT *points, const INT *counts,
                                UINT polygons ) DECLSPEC_HIDDEN;
extern BOOL METADC_Polygon( HDC hdc, const POINT *points, INT count ) DECLSPEC_HIDDEN;
extern BOOL METADC_Polyline( HDC hdc, const POINT *points,INT count) DECLSPEC_HIDDEN;
extern BOOL METADC_RealizePalette( HDC hdc ) DECLSPEC_HIDDEN;
extern BOOL METADC_Rectangle( HDC hdc, INT left, INT top, INT right, INT bottom) DECLSPEC_HIDDEN;
extern BOOL METADC_RestoreDC( HDC hdc, INT level ) DECLSPEC_HIDDEN;
extern BOOL METADC_RoundRect( HDC hdc, INT left, INT top, INT right, INT bottom,
                              INT ell_width, INT ell_height ) DECLSPEC_HIDDEN;
extern BOOL METADC_SaveDC( HDC hdc ) DECLSPEC_HIDDEN;
extern BOOL METADC_ScaleViewportExtEx( HDC hdc, INT x_num, INT x_denom, INT y_num,
                                       INT y_denom ) DECLSPEC_HIDDEN;
extern BOOL METADC_ScaleWindowExtEx( HDC hdc, INT x_num, INT x_denom, INT y_num,
                                     INT y_denom ) DECLSPEC_HIDDEN;
extern HGDIOBJ METADC_SelectObject( HDC hdc, HGDIOBJ obj ) DECLSPEC_HIDDEN;
extern BOOL METADC_SelectPalette( HDC hdc, HPALETTE palette ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetBkColor( HDC hdc, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetBkMode( HDC hdc, INT mode ) DECLSPEC_HIDDEN;
extern INT  METADC_SetDIBitsToDevice( HDC hdc, INT x_dest, INT y_dest, DWORD width, DWORD height,
                                      INT x_src, INT y_src, UINT startscan, UINT lines,
                                      const void *bits, const BITMAPINFO *info,
                                      UINT coloruse ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetLayout( HDC hdc, DWORD layout ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetTextCharacterExtra( HDC hdc, INT extra ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetMapMode( HDC hdc, INT mode ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetMapperFlags( HDC hdc, DWORD flags ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetPixel( HDC hdc, INT x, INT y, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetPolyFillMode( HDC hdc, INT mode ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetRelAbs( HDC hdc, INT mode ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetROP2( HDC hdc, INT rop ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetStretchBltMode( HDC hdc, INT mode ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetTextAlign( HDC hdc, UINT align ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetTextColor( HDC hdc, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetTextJustification( HDC hdc, INT extra, INT breaks ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetViewportExtEx( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetViewportOrgEx( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetWindowExtEx( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetWindowOrgEx( HDC, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_StretchBlt( HDC hdc_dst, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                               HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                               DWORD rop );
extern INT  METADC_StretchDIBits( HDC hdc, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                                  INT x_src, INT y_src, INT width_src, INT height_src,
                                  const void *bits, const BITMAPINFO *info, UINT coloruse,
                                  DWORD rop ) DECLSPEC_HIDDEN;

extern HMETAFILE MF_Create_HMETAFILE(METAHEADER *mh) DECLSPEC_HIDDEN;

/* enhanced metafiles */

#define WMFC_MAGIC 0x43464d57

typedef struct
{
    EMR   emr;
    INT   nBreakExtra;
    INT   nBreakCount;
} EMRSETTEXTJUSTIFICATION, *PEMRSETTEXTJUSTIFICATION;

extern BOOL EMFDC_AbortPath( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_AlphaBlend( DC_ATTR *dc_attr, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                              HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                              BLENDFUNCTION blend_function );
extern BOOL EMFDC_AngleArc( DC_ATTR *dc_attr, INT x, INT y, DWORD radius, FLOAT start,
                            FLOAT sweep ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ArcChordPie( DC_ATTR *dc_attr, INT left, INT top, INT right,
                               INT bottom, INT xstart, INT ystart, INT xend,
                               INT yend, DWORD type ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_BeginPath( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_BitBlt( DC_ATTR *dc_attr, INT x_dst, INT y_dst, INT width, INT height,
                          HDC hdc_src, INT x_src, INT y_src, DWORD rop );
extern BOOL EMFDC_CloseFigure( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;
extern void EMFDC_DeleteDC( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_Ellipse( DC_ATTR *dc_attr, INT left, INT top, INT right,
                           INT bottom ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_EndPath( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ExcludeClipRect( DC_ATTR *dc_attr, INT left, INT top, INT right,
                                   INT bottom ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ExtFloodFill( DC_ATTR *dc_attr, INT x, INT y, COLORREF color,
                                UINT fill_type ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ExtSelectClipRgn( DC_ATTR *dc_attr, HRGN hrgn, INT mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ExtTextOut( DC_ATTR *dc_attr, INT x, INT y, UINT flags, const RECT *rect,
                              const WCHAR *str, UINT count, const INT *dx ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_FillPath( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_FillRgn( DC_ATTR *dc_attr, HRGN hrgn, HBRUSH hbrush ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_FlattenPath( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_FrameRgn( DC_ATTR *dc_attr, HRGN hrgn, HBRUSH hbrush, INT width,
                            INT height ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_GradientFill( DC_ATTR *dc_attr, TRIVERTEX *vert_array, ULONG nvert,
                                void *grad_array, ULONG ngrad, ULONG mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_IntersectClipRect( DC_ATTR *dc_attr, INT left, INT top, INT right,
                                     INT bottom ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_InvertRgn( DC_ATTR *dc_attr, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_LineTo( DC_ATTR *dc_attr, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_MaskBlt( DC_ATTR *dc_attr, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                           HDC hdc_src, INT x_src, INT y_src, HBITMAP mask,
                           INT x_mask, INT y_mask, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ModifyWorldTransform( DC_ATTR *dc_attr, const XFORM *xform,
                                        DWORD mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_MoveTo( DC_ATTR *dc_attr, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_OffsetClipRgn( DC_ATTR *dc_attr, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_PaintRgn( DC_ATTR *dc_attr, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_PatBlt( DC_ATTR *dc_attr, INT left, INT top, INT width, INT height, DWORD rop );
extern BOOL EMFDC_PlgBlt( DC_ATTR *dc_attr, const POINT *point, HDC hdc_src, INT x_src, INT y_src,
                          INT width, INT height, HBITMAP mask, INT x_mask, INT y_mask ) DECLSPEC_HIDDEN;
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
extern BOOL EMFDC_RealizePalette( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_Rectangle( DC_ATTR *dc_attr, INT left, INT top, INT right,
                             INT bottom) DECLSPEC_HIDDEN;
extern BOOL EMFDC_RestoreDC( DC_ATTR *dc_attr, INT level ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_RoundRect( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom,
                             INT ell_width, INT ell_height ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SaveDC( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ScaleViewportExtEx( DC_ATTR *dc_attr, INT x_num, INT x_denom, INT y_num,
                                      INT y_denom ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ScaleWindowExtEx( DC_ATTR *dc_attr, INT x_num, INT x_denom, INT y_num,
                                    INT y_denom ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SelectClipPath( DC_ATTR *dc_attr, INT mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SelectObject( DC_ATTR *dc_attr, HGDIOBJ obj ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SelectPalette( DC_ATTR *dc_attr, HPALETTE palette ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetArcDirection( DC_ATTR *dc_attr, INT dir ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetBkColor( DC_ATTR *dc_attr, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetBkMode( DC_ATTR *dc_attr, INT mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetDCBrushColor( DC_ATTR *dc_attr, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetDCPenColor( DC_ATTR *dc_attr, COLORREF color ) DECLSPEC_HIDDEN;
extern INT  EMFDC_SetDIBitsToDevice( DC_ATTR *dc_attr, INT x_dest, INT y_dest, DWORD width,
                                     DWORD height, INT x_src, INT y_src, UINT startscan,
                                     UINT lines, const void *bits, const BITMAPINFO *info,
                                     UINT coloruse ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetLayout( DC_ATTR *dc_attr, DWORD layout ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetMapMode( DC_ATTR *dc_attr, INT mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetMapperFlags( DC_ATTR *dc_attr, DWORD flags ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetPixel( DC_ATTR *dc_attr, INT x, INT y, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetPolyFillMode( DC_ATTR *dc_attr, INT mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetROP2( DC_ATTR *dc_attr, INT rop ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetStretchBltMode( DC_ATTR *dc_attr, INT mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetTextAlign( DC_ATTR *dc_attr, UINT align ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetTextColor( DC_ATTR *dc_attr, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetTextJustification( DC_ATTR *dc_attr, INT extra, INT breaks ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetViewportExtEx( DC_ATTR *dc_attr, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetViewportOrgEx( DC_ATTR *dc_attr, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetWindowExtEx( DC_ATTR *dc_attr, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetWindowOrgEx( DC_ATTR *dc_attr, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetWorldTransform( DC_ATTR *dc_attr, const XFORM *xform ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_StretchBlt( DC_ATTR *dc_attr, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                              HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                              DWORD rop );
extern BOOL EMFDC_StretchDIBits( DC_ATTR *dc_attr, INT x_dst, INT y_dst, INT width_dst,
                                 INT height_dst, INT x_src, INT y_src, INT width_src,
                                 INT height_src, const void *bits, const BITMAPINFO *info,
                                 UINT coloruse, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_StrokeAndFillPath( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_StrokePath( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_TransparentBlt( DC_ATTR *dc_attr, int x_dst, int y_dst, int width_dst,
                                  int height_dst, HDC hdc_src, int x_src, int y_src, int width_src,
                                  int height_src, UINT color ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_WidenPath( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;

extern HENHMETAFILE EMF_Create_HENHMETAFILE( ENHMETAHEADER *emh, DWORD filesize,
                                             BOOL on_disk ) DECLSPEC_HIDDEN;

extern BOOL spool_start_doc( DC_ATTR *dc_attr, HANDLE hspool,
                             const DOCINFOW *doc_info ) DECLSPEC_HIDDEN;
extern int spool_start_page( DC_ATTR *dc_attr, HANDLE hspool ) DECLSPEC_HIDDEN;
extern int spool_end_page( DC_ATTR *dc_attr, HANDLE hspool, const DEVMODEW *devmode,
                           BOOL write_devmode ) DECLSPEC_HIDDEN;
extern int spool_end_doc( DC_ATTR *dc_attr, HANDLE hspool ) DECLSPEC_HIDDEN;
extern int spool_abort_doc( DC_ATTR *dc_attr, HANDLE hspool ) DECLSPEC_HIDDEN;

extern void print_call_start_page( DC_ATTR *dc_attr ) DECLSPEC_HIDDEN;

static inline int get_dib_stride( int width, int bpp )
{
    return ((width * bpp + 31) >> 3) & ~3;
}

/* only for use on sanitized BITMAPINFO structures */
static inline int get_dib_info_size( const BITMAPINFO *info, UINT coloruse )
{
    if (info->bmiHeader.biCompression == BI_BITFIELDS)
        return sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD);
    if (coloruse == DIB_PAL_COLORS)
        return sizeof(BITMAPINFOHEADER) + info->bmiHeader.biClrUsed * sizeof(WORD);
    return FIELD_OFFSET( BITMAPINFO, bmiColors[info->bmiHeader.biClrUsed] );
}

extern HMODULE gdi32_module DECLSPEC_HIDDEN;

#endif /* __WINE_GDI_PRIVATE_H */
