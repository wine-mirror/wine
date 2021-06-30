/*
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

#ifndef _NTGDI_
#define _NTGDI_

#include <wingdi.h>

typedef struct _GDI_HANDLE_ENTRY
{
    UINT64 Object;
    union
    {
        struct
        {
            USHORT ProcessId;
            USHORT Lock : 1;
            USHORT Count : 15;
        };
        ULONG Value;
    } Owner;
    USHORT Unique;
    UCHAR  Type;
    UCHAR  Flags;
    UINT64 UserPointer;
} GDI_HANDLE_ENTRY, *PGDI_HANDLE_ENTRY;

#define GDI_MAX_HANDLE_COUNT 0x10000

typedef struct _GDI_SHARED_MEMORY
{
    GDI_HANDLE_ENTRY Handles[GDI_MAX_HANDLE_COUNT];
} GDI_SHARED_MEMORY, *PGDI_SHARED_MEMORY;

INT      WINAPI NtGdiAbortDoc( HDC hdc );
BOOL     WINAPI NtGdiAbortPath( HDC hdc );
BOOL     WINAPI NtGdiAngleArc( HDC hdc, INT x, INT y, DWORD radius, FLOAT start_angle,
                               FLOAT sweep_angle );
BOOL     WINAPI NtGdiArcInternal( UINT type, HDC hdc, INT left, INT top, INT right, INT bottom,
                                  INT xstart, INT ystart, INT xend, INT yend );
BOOL     WINAPI NtGdiBeginPath( HDC hdc );
BOOL     WINAPI NtGdiCancelDC( HDC hdc );
BOOL     WINAPI NtGdiCloseFigure( HDC hdc );
INT      WINAPI NtGdiCombineRgn( HRGN dest, HRGN src1, HRGN src2, INT mode );
HBITMAP  WINAPI NtGdiCreateBitmap( INT width, INT height, UINT planes,
                                   UINT bpp, const void *bits );
HBRUSH   WINAPI NtGdiCreateHatchBrushInternal( INT style, COLORREF color, BOOL pen );
BOOL     WINAPI NtGdiEllipse( HDC hdc, INT left, INT top, INT right, INT bottom );
INT      WINAPI NtGdiEndDoc(HDC hdc);
BOOL     WINAPI NtGdiEndPath( HDC hdc );
HANDLE   WINAPI NtGdiCreateClientObj( ULONG type );
HFONT    WINAPI NtGdiHfontCreate( const ENUMLOGFONTEXDVW *enumex, ULONG unk2, ULONG unk3,
                                  ULONG unk4, void *data );
HBRUSH   WINAPI NtGdiCreateDIBBrush( const void* data, UINT coloruse );
HBRUSH   WINAPI NtGdiCreatePatternBrushInternal( HBITMAP hbitmap, BOOL pen );
HPEN     WINAPI NtGdiCreatePen( INT style, INT width, COLORREF color );
HRGN     WINAPI NtGdiCreateRectRgn( INT left, INT top, INT right, INT bottom );
HRGN     WINAPI NtGdiCreateRoundRectRgn( INT left, INT top, INT right, INT bottom,
                                         INT ellipse_width, INT ellipse_height );
HBRUSH   WINAPI NtGdiCreateSolidBrush( COLORREF color );
BOOL     WINAPI NtGdiDeleteObjectApp( HGDIOBJ obj );
LONG     WINAPI NtGdiDoPalette( HGDIOBJ handle, WORD start, WORD count, void *entries,
                                DWORD func, BOOL inbound );
INT      WINAPI NtGdiEndPage( HDC hdc );
HPEN     WINAPI NtGdiExtCreatePen( DWORD style, DWORD width, const LOGBRUSH *brush,
                                   DWORD style_count, const DWORD *style_bits );
HRGN     WINAPI NtGdiExtCreateRegion( const XFORM *xform, DWORD count, const RGNDATA *data );
INT      WINAPI NtGdiExtGetObjectW( HGDIOBJ handle, INT count, void *buffer );
INT      WINAPI NtGdiExtSelectClipRgn( HDC hdc, HRGN region, INT mode );
BOOL     WINAPI NtGdiFillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush );
INT      WINAPI NtGdiExtEscape( HDC hdc, INT escape, INT input_size, const char *input,
                                INT output_size, char *output );
BOOL     WINAPI NtGdiExtFloodFill( HDC hdc, INT x, INT y, COLORREF color, UINT type );
BOOL     WINAPI NtGdiFrameRgn( HDC hdc, HRGN hrgn, HBRUSH brush,
                               INT width, INT height );
BOOL     WINAPI NtGdiFillPath( HDC hdc );
BOOL     WINAPI NtGdiGetAndSetDCDword( HDC hdc, UINT method, DWORD value, DWORD *result );
INT      WINAPI NtGdiGetAppClipBox( HDC hdc, RECT *rect );
BOOL     WINAPI NtGdiGetBitmapDimension( HBITMAP bitmap, SIZE *size );
UINT     WINAPI NtGdiGetBoundsRect( HDC hdc, RECT *rect, UINT flags );
BOOL     WINAPI NtGdiGetCharABCWidthsW( HDC hdc, UINT first_char, UINT last_char, ABC *abc );
BOOL     WINAPI NtGdiGetCharWidthW( HDC hdc, UINT first_char, UINT last_char, INT *buffer );
BOOL     WINAPI NtGdiGetDCDword( HDC hdc, UINT method, DWORD *result );
BOOL     WINAPI NtGdiGetDCPoint( HDC hdc, UINT method, POINT *result );
INT      WINAPI NtGdiGetDeviceCaps( HDC hdc, INT cap );
BOOL     WINAPI NtGdiGetDeviceGammaRamp( HDC hdc, void *ptr );
BOOL     WINAPI NtGdiGetMiterLimit( HDC hdc, FLOAT *limit );
COLORREF WINAPI NtGdiGetNearestColor( HDC hdc, COLORREF color );
UINT     WINAPI NtGdiGetNearestPaletteIndex( HPALETTE hpalette, COLORREF color );
UINT     WINAPI NtGdiGetOutlineTextMetricsInternalW( HDC hdc, UINT cbData,
                                                     OUTLINETEXTMETRICW *otm, ULONG opts);
INT      WINAPI NtGdiGetPath( HDC hdc, POINT *points, BYTE *types, INT size );
INT      WINAPI NtGdiGetRandomRgn( HDC hdc, HRGN region, INT code );
DWORD    WINAPI NtGdiGetRegionData( HRGN hrgn, DWORD count, RGNDATA *data );
INT      WINAPI NtGdiGetRgnBox( HRGN hrgn, RECT *rect );
UINT     WINAPI NtGdiGetTextCharsetInfo( HDC hdc, FONTSIGNATURE *fs, DWORD flags );
INT      WINAPI NtGdiGetTextFaceW( HDC hdc, INT count, WCHAR *name );
BOOL     WINAPI NtGdiGetTextMetricsW( HDC hdc, TEXTMETRICW *metrics );
BOOL     WINAPI NtGdiGetTransform( HDC hdc, DWORD which, XFORM *xform );
BOOL     WINAPI NtGdiGradientFill( HDC hdc, TRIVERTEX *vert_array, ULONG nvert,
                                   void *grad_array, ULONG ngrad, ULONG mode );
BOOL     WINAPI NtGdiEqualRgn( HRGN hrgn1, HRGN hrgn2 );
INT      WINAPI NtGdiExcludeClipRect( HDC hdc, INT left, INT top, INT right, INT bottom );
BOOL     WINAPI NtGdiFlattenPath( HDC hdc );
BOOL     WINAPI NtGdiFontIsLinked( HDC hdc );
INT      WINAPI NtGdiIntersectClipRect( HDC hdc, INT left, INT top, INT right, INT bottom );
BOOL     WINAPI NtGdiInvertRgn( HDC hdc, HRGN hrgn );
BOOL     WINAPI NtGdiLineTo( HDC hdc, INT x, INT y );
BOOL     WINAPI NtGdiMoveTo( HDC hdc, INT x, INT y, POINT *pt );
INT      WINAPI NtGdiOffsetClipRgn( HDC hdc, INT x, INT y );
INT      WINAPI NtGdiOffsetRgn( HRGN hrgn, INT x, INT y );
HRGN     WINAPI NtGdiPathToRegion( HDC hdc );
BOOL     WINAPI NtGdiPolyDraw(HDC hdc, const POINT *points, const BYTE *types, DWORD count );
ULONG    WINAPI NtGdiPolyPolyDraw( HDC hdc, const POINT *points, const UINT *counts,
                                   DWORD count, UINT function );
BOOL     WINAPI NtGdiPtInRegion( HRGN hrgn, INT x, INT y );
BOOL     WINAPI NtGdiPtVisible( HDC hdc, INT x, INT y );
BOOL     WINAPI NtGdiRectInRegion( HRGN hrgn, const RECT *rect );
BOOL     WINAPI NtGdiRectVisible( HDC hdc, const RECT *rect );
BOOL     WINAPI NtGdiRectangle( HDC hdc, INT left, INT top, INT right, INT bottom );
HDC      WINAPI NtGdiResetDC( HDC hdc, const DEVMODEW *devmode );
BOOL     WINAPI NtGdiRestoreDC( HDC hdc, INT level );
BOOL     WINAPI NtGdiRoundRect( HDC hdc, INT left, INT top, INT right,
                                INT bottom, INT ell_width, INT ell_height );
INT      WINAPI NtGdiSaveDC( HDC hdc );
BOOL     WINAPI NtGdiScaleViewportExtEx( HDC hdc, INT x_num, INT x_denom,
                                        INT y_num, INT y_denom, SIZE *size );
BOOL     WINAPI NtGdiScaleWindowExtEx( HDC hdc, INT x_num, INT x_denom,
                                       INT y_num, INT y_denom, SIZE *size );
HGDIOBJ  WINAPI NtGdiSelectBitmap( HDC hdc, HGDIOBJ handle );
HGDIOBJ  WINAPI NtGdiSelectBrush( HDC hdc, HGDIOBJ handle );
BOOL     WINAPI NtGdiSelectClipPath( HDC hdc, INT mode );
HGDIOBJ  WINAPI NtGdiSelectFont( HDC hdc, HGDIOBJ handle );
HGDIOBJ  WINAPI NtGdiSelectPen( HDC hdc, HGDIOBJ handle );
LONG     WINAPI NtGdiSetBitmapBits( HBITMAP hbitmap, LONG count, const void *bits );
BOOL     WINAPI NtGdiSetBitmapDimension( HBITMAP hbitmap, INT x, INT y, SIZE *prev_size );
BOOL     WINAPI NtGdiSetBrushOrg( HDC hdc, INT x, INT y, POINT *prev_org );
UINT     WINAPI NtGdiSetBoundsRect( HDC hdc, const RECT *rect, UINT flags );
BOOL     WINAPI NtGdiSetDeviceGammaRamp( HDC hdc, void *ptr );
DWORD    WINAPI NtGdiSetLayout( HDC hdc, DWORD layout );
INT      WINAPI NtGdiSetMetaRgn( HDC hdc );
BOOL     WINAPI NtGdiSetMiterLimit( HDC hdc, FLOAT limit, FLOAT *prev_limit );
COLORREF WINAPI NtGdiSetPixel( HDC hdc, INT x, INT y, COLORREF color );
BOOL     WINAPI NtGdiSetPixelFormat( HDC hdc, INT format, const PIXELFORMATDESCRIPTOR *descr );
BOOL     WINAPI NtGdiSetRectRgn( HRGN hrgn, INT left, INT top, INT right, INT bottom );
BOOL     WINAPI NtGdiSetTextJustification( HDC hdc, INT extra, INT breaks );
BOOL     WINAPI NtGdiSetVirtualResolution( HDC hdc, DWORD horz_res, DWORD vert_res,
                                           DWORD horz_size, DWORD vert_size );
INT      WINAPI NtGdiStartDoc( HDC hdc, const DOCINFOW *doc );
INT      WINAPI NtGdiStartPage( HDC hdc );
BOOL     WINAPI NtGdiStrokePath( HDC hdc );
BOOL     WINAPI NtGdiStrokeAndFillPath( HDC hdc );
BOOL     WINAPI NtGdiTransformPoints( HDC hdc, POINT *points, INT count, UINT mode );
BOOL     WINAPI NtGdiUnrealizeObject( HGDIOBJ obj );
BOOL     WINAPI NtGdiWidenPath( HDC hdc );

#endif /* _NTGDI_ */
