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

void set_gdi_client_ptr( HGDIOBJ handle, void *ptr );
void *get_gdi_client_ptr( HGDIOBJ handle, DWORD type );
DC_ATTR *get_dc_attr( HDC hdc );
HGDIOBJ get_full_gdi_handle( HGDIOBJ handle );
void GDI_hdc_using_object( HGDIOBJ obj, HDC hdc, void (*delete)( HDC hdc, HGDIOBJ handle ));
void GDI_hdc_not_using_object( HGDIOBJ obj, HDC hdc );

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

BOOL METADC_Arc( HDC hdc, INT left, INT top, INT right, INT bottom,
                 INT xstart, INT ystart, INT xend, INT yend );
BOOL METADC_BitBlt( HDC hdc_dst, INT x_dst, INT y_dst, INT width, INT height,
                    HDC hdc_src, INT x_src, INT y_src, DWORD rop );
BOOL METADC_Chord( HDC hdc, INT left, INT top, INT right, INT bottom, INT xstart,
                   INT ystart, INT xend, INT yend );
BOOL METADC_DeleteDC( HDC hdc );
BOOL METADC_Ellipse( HDC hdc, INT left, INT top, INT right, INT bottom );
BOOL METADC_ExcludeClipRect( HDC hdc, INT left, INT top, INT right, INT bottom );
BOOL METADC_ExtEscape( HDC hdc, INT escape, INT input_size, const void *input,
                       INT output_size, void *output );
BOOL METADC_ExtFloodFill( HDC hdc, INT x, INT y, COLORREF color, UINT fill_type );
BOOL METADC_ExtSelectClipRgn( HDC hdc, HRGN hrgn, INT mode );
BOOL METADC_ExtTextOut( HDC hdc, INT x, INT y, UINT flags, const RECT *rect,
                        const WCHAR *str, UINT count, const INT *dx );
BOOL METADC_FillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush );
BOOL METADC_FrameRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush, INT x, INT y );
INT  METADC_GetDeviceCaps( HDC hdc, INT cap );
BOOL METADC_IntersectClipRect( HDC hdc, INT left, INT top, INT right, INT bottom );
BOOL METADC_InvertRgn( HDC hdc, HRGN hrgn );
BOOL METADC_LineTo( HDC hdc, INT x, INT y );
BOOL METADC_MoveTo( HDC hdc, INT x, INT y );
BOOL METADC_OffsetClipRgn( HDC hdc, INT x, INT y );
BOOL METADC_OffsetViewportOrgEx( HDC hdc, INT x, INT y );
BOOL METADC_OffsetWindowOrgEx( HDC hdc, INT x, INT y );
BOOL METADC_PaintRgn( HDC hdc, HRGN hrgn );
BOOL METADC_PatBlt( HDC hdc, INT left, INT top, INT width, INT height, DWORD rop );
BOOL METADC_Pie( HDC hdc, INT left, INT top, INT right, INT bottom,
                 INT xstart, INT ystart, INT xend, INT yend );
BOOL METADC_PolyPolygon( HDC hdc, const POINT *points, const INT *counts, UINT polygons );
BOOL METADC_Polygon( HDC hdc, const POINT *points, INT count );
BOOL METADC_Polyline( HDC hdc, const POINT *points,INT count);
BOOL METADC_RealizePalette( HDC hdc );
BOOL METADC_Rectangle( HDC hdc, INT left, INT top, INT right, INT bottom);
BOOL METADC_RestoreDC( HDC hdc, INT level );
BOOL METADC_RoundRect( HDC hdc, INT left, INT top, INT right, INT bottom,
                       INT ell_width, INT ell_height );
BOOL METADC_SaveDC( HDC hdc );
BOOL METADC_ScaleViewportExtEx( HDC hdc, INT x_num, INT x_denom, INT y_num, INT y_denom );
BOOL METADC_ScaleWindowExtEx( HDC hdc, INT x_num, INT x_denom, INT y_num, INT y_denom );
HGDIOBJ METADC_SelectObject( HDC hdc, HGDIOBJ obj );
BOOL METADC_SelectPalette( HDC hdc, HPALETTE palette );
BOOL METADC_SetBkColor( HDC hdc, COLORREF color );
BOOL METADC_SetBkMode( HDC hdc, INT mode );
INT  METADC_SetDIBitsToDevice( HDC hdc, INT x_dest, INT y_dest, DWORD width, DWORD height,
                               INT x_src, INT y_src, UINT startscan, UINT lines,
                               const void *bits, const BITMAPINFO *info, UINT coloruse );
BOOL METADC_SetLayout( HDC hdc, DWORD layout );
BOOL METADC_SetTextCharacterExtra( HDC hdc, INT extra );
BOOL METADC_SetMapMode( HDC hdc, INT mode );
BOOL METADC_SetMapperFlags( HDC hdc, DWORD flags );
BOOL METADC_SetPixel( HDC hdc, INT x, INT y, COLORREF color );
BOOL METADC_SetPolyFillMode( HDC hdc, INT mode );
BOOL METADC_SetRelAbs( HDC hdc, INT mode );
BOOL METADC_SetROP2( HDC hdc, INT rop );
BOOL METADC_SetStretchBltMode( HDC hdc, INT mode );
BOOL METADC_SetTextAlign( HDC hdc, UINT align );
BOOL METADC_SetTextColor( HDC hdc, COLORREF color );
BOOL METADC_SetTextJustification( HDC hdc, INT extra, INT breaks );
BOOL METADC_SetViewportExtEx( HDC hdc, INT x, INT y );
BOOL METADC_SetViewportOrgEx( HDC hdc, INT x, INT y );
BOOL METADC_SetWindowExtEx( HDC hdc, INT x, INT y );
BOOL METADC_SetWindowOrgEx( HDC, INT x, INT y );
BOOL METADC_StretchBlt( HDC hdc_dst, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                        HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                        DWORD rop );
INT  METADC_StretchDIBits( HDC hdc, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                           INT x_src, INT y_src, INT width_src, INT height_src,
                           const void *bits, const BITMAPINFO *info, UINT coloruse,
                           DWORD rop );

HMETAFILE MF_Create_HMETAFILE( METAHEADER *mh );

/* enhanced metafiles */

#define WMFC_MAGIC 0x43464d57

typedef struct
{
    EMR   emr;
    INT   nBreakExtra;
    INT   nBreakCount;
} EMRSETTEXTJUSTIFICATION, *PEMRSETTEXTJUSTIFICATION;

/* Public structure EMRSETMITERLIMIT is using a float field,
   that does not match serialized output or documentation,
   which are both using unsigned integer. */
struct emr_set_miter_limit
{
    EMR   emr;
    DWORD eMiterLimit;
};

BOOL EMFDC_AbortPath( DC_ATTR *dc_attr );
BOOL EMFDC_AlphaBlend( DC_ATTR *dc_attr, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                       HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                       BLENDFUNCTION blend_function );
BOOL EMFDC_AngleArc( DC_ATTR *dc_attr, INT x, INT y, DWORD radius, FLOAT start, FLOAT sweep );
BOOL EMFDC_ArcChordPie( DC_ATTR *dc_attr, INT left, INT top, INT right,
                        INT bottom, INT xstart, INT ystart, INT xend,
                        INT yend, DWORD type );
BOOL EMFDC_BeginPath( DC_ATTR *dc_attr );
BOOL EMFDC_BitBlt( DC_ATTR *dc_attr, INT x_dst, INT y_dst, INT width, INT height,
                   HDC hdc_src, INT x_src, INT y_src, DWORD rop );
BOOL EMFDC_CloseFigure( DC_ATTR *dc_attr );
void EMFDC_DeleteDC( DC_ATTR *dc_attr );
BOOL EMFDC_Ellipse( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom );
BOOL EMFDC_EndPath( DC_ATTR *dc_attr );
BOOL EMFDC_ExcludeClipRect( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom );
INT  EMFDC_ExtEscape( DC_ATTR *dc_attr, INT escape, INT input_size, const char *input,
                      INT output_size, char *output );
BOOL EMFDC_ExtFloodFill( DC_ATTR *dc_attr, INT x, INT y, COLORREF color, UINT fill_type );
BOOL EMFDC_ExtSelectClipRgn( DC_ATTR *dc_attr, HRGN hrgn, INT mode );
BOOL EMFDC_ExtTextOut( DC_ATTR *dc_attr, INT x, INT y, UINT flags, const RECT *rect,
                       const WCHAR *str, UINT count, const INT *dx );
BOOL EMFDC_FillPath( DC_ATTR *dc_attr );
BOOL EMFDC_FillRgn( DC_ATTR *dc_attr, HRGN hrgn, HBRUSH hbrush );
BOOL EMFDC_FlattenPath( DC_ATTR *dc_attr );
BOOL EMFDC_FrameRgn( DC_ATTR *dc_attr, HRGN hrgn, HBRUSH hbrush, INT width, INT height );
BOOL EMFDC_GradientFill( DC_ATTR *dc_attr, TRIVERTEX *vert_array, ULONG nvert,
                         void *grad_array, ULONG ngrad, ULONG mode );
BOOL EMFDC_IntersectClipRect( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom );
BOOL EMFDC_InvertRgn( DC_ATTR *dc_attr, HRGN hrgn );
BOOL EMFDC_LineTo( DC_ATTR *dc_attr, INT x, INT y );
BOOL EMFDC_MaskBlt( DC_ATTR *dc_attr, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                    HDC hdc_src, INT x_src, INT y_src, HBITMAP mask,
                    INT x_mask, INT y_mask, DWORD rop );
BOOL EMFDC_ModifyWorldTransform( DC_ATTR *dc_attr, const XFORM *xform, DWORD mode );
BOOL EMFDC_MoveTo( DC_ATTR *dc_attr, INT x, INT y );
BOOL EMFDC_OffsetClipRgn( DC_ATTR *dc_attr, INT x, INT y );
BOOL EMFDC_PaintRgn( DC_ATTR *dc_attr, HRGN hrgn );
BOOL EMFDC_PatBlt( DC_ATTR *dc_attr, INT left, INT top, INT width, INT height, DWORD rop );
BOOL EMFDC_PlgBlt( DC_ATTR *dc_attr, const POINT *point, HDC hdc_src, INT x_src, INT y_src,
                   INT width, INT height, HBITMAP mask, INT x_mask, INT y_mask );
BOOL EMFDC_PolyBezier( DC_ATTR *dc_attr, const POINT *points, DWORD count );
BOOL EMFDC_PolyBezierTo( DC_ATTR *dc_attr, const POINT *points, DWORD count );
BOOL EMFDC_PolyDraw( DC_ATTR *dc_attr, const POINT *points, const BYTE *types, DWORD count );
BOOL EMFDC_PolyPolyline( DC_ATTR *dc_attr, const POINT *points, const DWORD *counts, DWORD polys );
BOOL EMFDC_PolyPolygon( DC_ATTR *dc_attr, const POINT *points, const INT *counts, UINT polys );
BOOL EMFDC_Polygon( DC_ATTR *dc_attr, const POINT *points, INT count );
BOOL EMFDC_Polyline( DC_ATTR *dc_attr, const POINT *points, INT count);
BOOL EMFDC_PolylineTo( DC_ATTR *dc_attr, const POINT *points, INT count );
BOOL EMFDC_RealizePalette( DC_ATTR *dc_attr );
BOOL EMFDC_Rectangle( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom);
BOOL EMFDC_RestoreDC( DC_ATTR *dc_attr, INT level );
BOOL EMFDC_RoundRect( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom,
                      INT ell_width, INT ell_height );
BOOL EMFDC_SaveDC( DC_ATTR *dc_attr );
BOOL EMFDC_ScaleViewportExtEx( DC_ATTR *dc_attr, INT x_num, INT x_denom, INT y_num, INT y_denom );
BOOL EMFDC_ScaleWindowExtEx( DC_ATTR *dc_attr, INT x_num, INT x_denom, INT y_num,
                                    INT y_denom );
BOOL EMFDC_SelectClipPath( DC_ATTR *dc_attr, INT mode );
BOOL EMFDC_SelectObject( DC_ATTR *dc_attr, HGDIOBJ obj );
BOOL EMFDC_SelectPalette( DC_ATTR *dc_attr, HPALETTE palette );
BOOL EMFDC_SetArcDirection( DC_ATTR *dc_attr, INT dir );
BOOL EMFDC_SetBkColor( DC_ATTR *dc_attr, COLORREF color );
BOOL EMFDC_SetBkMode( DC_ATTR *dc_attr, INT mode );
BOOL EMFDC_SetBrushOrgEx( DC_ATTR *dc_attr, INT x, INT y );
BOOL EMFDC_SetDCBrushColor( DC_ATTR *dc_attr, COLORREF color );
BOOL EMFDC_SetDCPenColor( DC_ATTR *dc_attr, COLORREF color );
INT  EMFDC_SetDIBitsToDevice( DC_ATTR *dc_attr, INT x_dest, INT y_dest, DWORD width, DWORD height,
                              INT x_src, INT y_src, UINT startscan, UINT lines, const void *bits,
                              const BITMAPINFO *info, UINT coloruse );
BOOL EMFDC_SetLayout( DC_ATTR *dc_attr, DWORD layout );
BOOL EMFDC_SetMapMode( DC_ATTR *dc_attr, INT mode );
BOOL EMFDC_SetMapperFlags( DC_ATTR *dc_attr, DWORD flags );
BOOL EMFDC_SetMetaRgn( DC_ATTR *dc_attr );
BOOL EMFDC_SetMiterLimit( DC_ATTR *dc_attr, FLOAT limit );
BOOL EMFDC_SetPixel( DC_ATTR *dc_attr, INT x, INT y, COLORREF color );
BOOL EMFDC_SetPolyFillMode( DC_ATTR *dc_attr, INT mode );
BOOL EMFDC_SetROP2( DC_ATTR *dc_attr, INT rop );
BOOL EMFDC_SetStretchBltMode( DC_ATTR *dc_attr, INT mode );
BOOL EMFDC_SetTextAlign( DC_ATTR *dc_attr, UINT align );
BOOL EMFDC_SetTextColor( DC_ATTR *dc_attr, COLORREF color );
BOOL EMFDC_SetTextJustification( DC_ATTR *dc_attr, INT extra, INT breaks );
BOOL EMFDC_SetViewportExtEx( DC_ATTR *dc_attr, INT x, INT y );
BOOL EMFDC_SetViewportOrgEx( DC_ATTR *dc_attr, INT x, INT y );
BOOL EMFDC_SetWindowExtEx( DC_ATTR *dc_attr, INT x, INT y );
BOOL EMFDC_SetWindowOrgEx( DC_ATTR *dc_attr, INT x, INT y );
BOOL EMFDC_SetWorldTransform( DC_ATTR *dc_attr, const XFORM *xform );
BOOL EMFDC_StretchBlt( DC_ATTR *dc_attr, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                       HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                       DWORD rop );
BOOL EMFDC_StretchDIBits( DC_ATTR *dc_attr, INT x_dst, INT y_dst, INT width_dst,
                          INT height_dst, INT x_src, INT y_src, INT width_src,
                          INT height_src, const void *bits, const BITMAPINFO *info,
                          UINT coloruse, DWORD rop );
BOOL EMFDC_StrokeAndFillPath( DC_ATTR *dc_attr );
BOOL EMFDC_StrokePath( DC_ATTR *dc_attr );
BOOL EMFDC_TransparentBlt( DC_ATTR *dc_attr, int x_dst, int y_dst, int width_dst,
                           int height_dst, HDC hdc_src, int x_src, int y_src, int width_src,
                           int height_src, UINT color );
BOOL EMFDC_WidenPath( DC_ATTR *dc_attr );

HENHMETAFILE EMF_Create_HENHMETAFILE( ENHMETAHEADER *emh, DWORD filesize, BOOL on_disk );

BOOL spool_start_doc( DC_ATTR *dc_attr, HANDLE hspool, const DOCINFOW *doc_info );
int spool_start_page( DC_ATTR *dc_attr, HANDLE hspool );
int spool_end_page( DC_ATTR *dc_attr, HANDLE hspool, const DEVMODEW *devmode,
                    BOOL write_devmode );
int spool_end_doc( DC_ATTR *dc_attr, HANDLE hspool );
int spool_abort_doc( DC_ATTR *dc_attr, HANDLE hspool );

void print_call_start_page( DC_ATTR *dc_attr );

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

extern HMODULE gdi32_module;

#endif /* __WINE_GDI_PRIVATE_H */
