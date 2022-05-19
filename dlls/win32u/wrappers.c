/*
 * Unix call wrappers
 *
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "win32u_private.h"

static const struct unix_funcs *unix_funcs;

INT WINAPI NtGdiAbortDoc( HDC hdc )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiAbortDoc( hdc );
}

BOOL WINAPI NtGdiAbortPath( HDC hdc )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiAbortPath( hdc );
}

BOOL WINAPI NtGdiAlphaBlend( HDC hdc_dst, int x_dst, int y_dst, int width_dst, int height_dst,
                             HDC hdc_src, int x_src, int y_src, int width_src, int height_src,
                             BLENDFUNCTION blend_function, HANDLE xform )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiAlphaBlend( hdc_dst, x_dst, y_dst, width_dst, height_dst, hdc_src,
                                         x_src, y_src, width_src, height_src, blend_function, xform );
}

BOOL WINAPI NtGdiAngleArc( HDC hdc, INT x, INT y, DWORD radius, FLOAT start_angle, FLOAT sweep_angle )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiAngleArc( hdc, x, y, radius, start_angle, sweep_angle );
}

BOOL WINAPI NtGdiArcInternal( UINT type, HDC hdc, INT left, INT top, INT right, INT bottom,
                              INT xstart, INT ystart, INT xend, INT yend )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiArcInternal( type, hdc, left, top, right, bottom, xstart, ystart, xend, yend );
}

BOOL WINAPI NtGdiBeginPath( HDC hdc )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiBeginPath( hdc );
}

BOOL WINAPI NtGdiBitBlt( HDC hdc_dst, INT x_dst, INT y_dst, INT width, INT height, HDC hdc_src,
                         INT x_src, INT y_src, DWORD rop, DWORD bk_color, FLONG fl )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiBitBlt( hdc_dst, x_dst, y_dst, width, height, hdc_src, x_src, y_src,
                                     rop, bk_color, fl );
}

BOOL WINAPI NtGdiCloseFigure( HDC hdc )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiCloseFigure( hdc );
}

BOOL WINAPI NtGdiComputeXformCoefficients( HDC hdc )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiComputeXformCoefficients( hdc );
}

HBITMAP WINAPI NtGdiCreateCompatibleBitmap( HDC hdc, INT width, INT height )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiCreateCompatibleBitmap( hdc, width, height );
}

HDC WINAPI NtGdiCreateCompatibleDC( HDC hdc )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiCreateCompatibleDC( hdc );
}

HBITMAP WINAPI NtGdiCreateDIBitmapInternal( HDC hdc, INT width, INT height, DWORD init,
                                            const void *bits, const BITMAPINFO *data,
                                            UINT coloruse, UINT max_info, UINT max_bits,
                                            ULONG flags, HANDLE xform )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiCreateDIBitmapInternal( hdc, width, height, init, bits, data,
                                                     coloruse, max_info, max_bits, flags, xform );
}

HDC WINAPI NtGdiCreateMetafileDC( HDC hdc )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiCreateMetafileDC( hdc );
}

BOOL WINAPI NtGdiDeleteObjectApp( HGDIOBJ obj )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiDeleteObjectApp( obj );
}

LONG WINAPI NtGdiDoPalette( HGDIOBJ handle, WORD start, WORD count, void *entries,
                            DWORD func, BOOL inbound )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiDoPalette( handle, start, count, entries, func, inbound );
}

BOOL WINAPI NtGdiEllipse( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiEllipse( hdc, left, top, right, bottom );
}

INT WINAPI NtGdiEndDoc( HDC hdc )
{
    if (!unix_funcs) return SP_ERROR;
    return unix_funcs->pNtGdiEndDoc( hdc );
}

BOOL WINAPI NtGdiEndPath( HDC hdc )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiEndPath( hdc );
}

INT WINAPI NtGdiEndPage( HDC hdc )
{
    if (!unix_funcs) return SP_ERROR;
    return unix_funcs->pNtGdiEndPage( hdc );
}

BOOL WINAPI NtUserEndPaint( HWND hwnd, const PAINTSTRUCT *ps )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserEndPaint( hwnd, ps );
}

BOOL WINAPI NtGdiEnumFonts( HDC hdc, ULONG type, ULONG win32_compat, ULONG face_name_len,
                            const WCHAR *face_name, ULONG charset, ULONG *count, void *buf )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiEnumFonts( hdc, type, win32_compat, face_name_len, face_name,
                                        charset, count, buf );
}

INT WINAPI NtGdiExcludeClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    if (!unix_funcs) return ERROR;
    return unix_funcs->pNtGdiExcludeClipRect( hdc, left, top, right, bottom );
}

INT WINAPI NtGdiExtEscape( HDC hdc, WCHAR *driver, INT driver_id, INT escape, INT input_size,
                           const char *input, INT output_size, char *output )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiExtEscape( hdc, driver, driver_id, escape, input_size, input,
                                        output_size, output );
}

BOOL WINAPI NtGdiExtFloodFill( HDC hdc, INT x, INT y, COLORREF color, UINT type )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiExtFloodFill( hdc, x, y, color, type );
}

BOOL WINAPI NtGdiExtTextOutW( HDC hdc, INT x, INT y, UINT flags, const RECT *rect,
                              const WCHAR *str, UINT count, const INT *dx, DWORD cp )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiExtTextOutW( hdc, x, y, flags, rect, str, count, dx, cp );
}

INT WINAPI NtGdiExtSelectClipRgn( HDC hdc, HRGN region, INT mode )
{
    if (!unix_funcs) return ERROR;
    return unix_funcs->pNtGdiExtSelectClipRgn( hdc, region, mode );
}

BOOL WINAPI NtGdiFillPath( HDC hdc )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiFillPath( hdc );
}

BOOL WINAPI NtGdiFillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiFillRgn( hdc, hrgn, hbrush );
}

BOOL WINAPI NtGdiFontIsLinked( HDC hdc )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiFontIsLinked( hdc );
}

BOOL WINAPI NtGdiFrameRgn( HDC hdc, HRGN hrgn, HBRUSH brush, INT width, INT height )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiFrameRgn( hdc, hrgn, brush, width, height );
}

BOOL WINAPI NtGdiGetAndSetDCDword( HDC hdc, UINT method, DWORD value, DWORD *result )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiGetAndSetDCDword( hdc, method, value, result );
}

INT WINAPI NtGdiGetAppClipBox( HDC hdc, RECT *rect )
{
    if (!unix_funcs) return ERROR;
    return unix_funcs->pNtGdiGetAppClipBox( hdc, rect );
}

UINT WINAPI NtGdiGetBoundsRect( HDC hdc, RECT *rect, UINT flags )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiGetBoundsRect( hdc, rect, flags );
}

BOOL WINAPI NtGdiGetCharABCWidthsW( HDC hdc, UINT first, UINT last, WCHAR *chars,
                                    ULONG flags, void *buffer )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiGetCharABCWidthsW( hdc, first, last, chars, flags, buffer );
}

BOOL WINAPI NtGdiGetCharWidthW( HDC hdc, UINT first_char, UINT last_char, WCHAR *chars,
                                ULONG flags, void *buffer )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiGetCharWidthW( hdc, first_char, last_char, chars, flags, buffer );
}

BOOL WINAPI NtGdiGetCharWidthInfo( HDC hdc, struct char_width_info *info )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiGetCharWidthInfo( hdc, info );
}

INT WINAPI NtGdiGetDIBitsInternal( HDC hdc, HBITMAP hbitmap, UINT startscan, UINT lines,
                                   void *bits, BITMAPINFO *info, UINT coloruse,
                                   UINT max_bits, UINT max_info )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiGetDIBitsInternal( hdc, hbitmap, startscan, lines, bits, info, coloruse,
                                                max_bits, max_info );
}

INT WINAPI NtGdiGetDeviceCaps( HDC hdc, INT cap )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiGetDeviceCaps( hdc, cap );
}

BOOL WINAPI NtGdiGetDeviceGammaRamp( HDC hdc, void *ptr )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiGetDeviceGammaRamp( hdc, ptr );
}

DWORD WINAPI NtGdiGetFontData( HDC hdc, DWORD table, DWORD offset, void *buffer, DWORD length )
{
    if (!unix_funcs) return GDI_ERROR;
    return unix_funcs->pNtGdiGetFontData( hdc, table, offset, buffer, length );
}

DWORD WINAPI NtGdiGetFontUnicodeRanges( HDC hdc, GLYPHSET *lpgs )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiGetFontUnicodeRanges( hdc, lpgs );
}

DWORD WINAPI NtGdiGetGlyphIndicesW( HDC hdc, const WCHAR *str, INT count,
                                    WORD *indices, DWORD flags )
{
    if (!unix_funcs) return GDI_ERROR;
    return unix_funcs->pNtGdiGetGlyphIndicesW( hdc, str, count, indices, flags );
}

DWORD WINAPI NtGdiGetGlyphOutline( HDC hdc, UINT ch, UINT format, GLYPHMETRICS *metrics,
                                   DWORD size, void *buffer, const MAT2 *mat2,
                                   BOOL ignore_rotation )
{
    if (!unix_funcs) return GDI_ERROR;
    return unix_funcs->pNtGdiGetGlyphOutline( hdc, ch, format, metrics, size, buffer, mat2, ignore_rotation );
}

DWORD WINAPI NtGdiGetKerningPairs( HDC hdc, DWORD count, KERNINGPAIR *kern_pair )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiGetKerningPairs( hdc, count, kern_pair );
}

COLORREF WINAPI NtGdiGetNearestColor( HDC hdc, COLORREF color )
{
    if (!unix_funcs) return CLR_INVALID;
    return unix_funcs->pNtGdiGetNearestColor( hdc, color );
}

UINT WINAPI NtGdiGetOutlineTextMetricsInternalW( HDC hdc, UINT cbData,
                                                 OUTLINETEXTMETRICW *otm, ULONG opts )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiGetOutlineTextMetricsInternalW( hdc, cbData, otm, opts );
}

COLORREF WINAPI NtGdiGetPixel( HDC hdc, INT x, INT y )
{
    if (!unix_funcs) return CLR_INVALID;
    return unix_funcs->pNtGdiGetPixel( hdc, x, y );
}

INT WINAPI NtGdiGetRandomRgn( HDC hdc, HRGN region, INT code )
{
    if (!unix_funcs) return -1;
    return unix_funcs->pNtGdiGetRandomRgn( hdc, region, code );
}

BOOL WINAPI NtGdiGetRasterizerCaps( RASTERIZER_STATUS *status, UINT size )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiGetRasterizerCaps( status, size );
}

BOOL WINAPI NtGdiGetRealizationInfo( HDC hdc, struct font_realization_info *info )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiGetRealizationInfo( hdc, info );
}

UINT WINAPI NtGdiGetTextCharsetInfo( HDC hdc, FONTSIGNATURE *fs, DWORD flags )
{
    if (!unix_funcs) return DEFAULT_CHARSET;
    return unix_funcs->pNtGdiGetTextCharsetInfo( hdc, fs, flags );
}

BOOL WINAPI NtGdiGetTextExtentExW( HDC hdc, const WCHAR *str, INT count, INT max_ext,
                                   INT *nfit, INT *dxs, SIZE *size, UINT flags )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiGetTextExtentExW( hdc, str, count, max_ext, nfit, dxs, size, flags );
}

INT WINAPI NtGdiGetTextFaceW( HDC hdc, INT count, WCHAR *name, BOOL alias_name )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiGetTextFaceW( hdc, count, name, alias_name );
}

BOOL WINAPI NtGdiGetTextMetricsW( HDC hdc, TEXTMETRICW *metrics, ULONG flags )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiGetTextMetricsW( hdc, metrics, flags );
}

BOOL WINAPI NtGdiGradientFill( HDC hdc, TRIVERTEX *vert_array, ULONG nvert,
                               void *grad_array, ULONG ngrad, ULONG mode )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiGradientFill( hdc, vert_array, nvert, grad_array, ngrad, mode );
}

INT WINAPI NtGdiIntersectClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    if (!unix_funcs) return ERROR;
    return unix_funcs->pNtGdiIntersectClipRect( hdc, left, top, right, bottom );
}

BOOL WINAPI NtGdiInvertRgn( HDC hdc, HRGN hrgn )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiInvertRgn( hdc, hrgn );
}

BOOL WINAPI NtGdiLineTo( HDC hdc, INT x, INT y )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiLineTo( hdc, x, y );
}

BOOL WINAPI NtGdiMaskBlt( HDC hdc, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                          HDC hdc_src, INT x_src, INT y_src, HBITMAP mask,
                          INT x_mask, INT y_mask, DWORD rop, DWORD bk_color )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiMaskBlt( hdc, x_dst, y_dst, width_dst, height_dst, hdc_src,
                                      x_src, y_src, mask, x_mask, y_mask, rop, bk_color );
}

BOOL WINAPI NtGdiModifyWorldTransform( HDC hdc, const XFORM *xform, DWORD mode )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiModifyWorldTransform( hdc, xform, mode );
}

BOOL WINAPI NtGdiMoveTo( HDC hdc, INT x, INT y, POINT *pt )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiMoveTo( hdc, x, y, pt );
}

INT WINAPI NtGdiOffsetClipRgn( HDC hdc, INT x, INT y )
{
    if (!unix_funcs) return ERROR;
    return unix_funcs->pNtGdiOffsetClipRgn( hdc, x, y );
}

HDC WINAPI NtGdiOpenDCW( UNICODE_STRING *device, const DEVMODEW *devmode,
                         UNICODE_STRING *output, ULONG type, BOOL is_display,
                         HANDLE hspool, DRIVER_INFO_2W *driver_info, void *pdev )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiOpenDCW( device, devmode, output, type, is_display,
                                      hspool, driver_info, pdev );
}

BOOL WINAPI NtGdiPatBlt( HDC hdc, INT left, INT top, INT width, INT height, DWORD rop )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiPatBlt( hdc, left, top, width, height, rop );
}

BOOL WINAPI NtGdiPlgBlt( HDC hdc, const POINT *point, HDC hdc_src, INT x_src, INT y_src,
                         INT width, INT height, HBITMAP mask, INT x_mask, INT y_mask,
                         DWORD bk_color )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiPlgBlt( hdc, point, hdc_src, x_src, y_src, width, height, mask,
                                     x_mask, y_mask, bk_color );
}

BOOL WINAPI NtGdiPolyDraw( HDC hdc, const POINT *points, const BYTE *types, DWORD count )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiPolyDraw( hdc, points, types, count );
}

ULONG WINAPI NtGdiPolyPolyDraw( HDC hdc, const POINT *points, const ULONG *counts,
                                DWORD count, UINT function )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiPolyPolyDraw( hdc, points, counts, count, function );
}

BOOL WINAPI NtGdiPtVisible( HDC hdc, INT x, INT y )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiPtVisible( hdc, x, y );
}

BOOL WINAPI NtGdiRectVisible( HDC hdc, const RECT *rect )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiRectVisible( hdc, rect );
}

BOOL WINAPI NtGdiRectangle( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiRectangle( hdc, left, top, right, bottom );
}

BOOL WINAPI NtGdiResetDC( HDC hdc, const DEVMODEW *devmode, BOOL *banding,
                          DRIVER_INFO_2W *driver_info, void *dev )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiResetDC( hdc, devmode, banding, driver_info, dev );
}

BOOL WINAPI NtGdiResizePalette( HPALETTE palette, UINT count )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiResizePalette( palette, count );
}

BOOL WINAPI NtGdiRestoreDC( HDC hdc, INT level )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiRestoreDC( hdc, level );
}

BOOL WINAPI NtGdiRoundRect( HDC hdc, INT left, INT top, INT right,
                            INT bottom, INT ell_width, INT ell_height )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiRoundRect( hdc, left, top, right, bottom, ell_width, ell_height );
}

BOOL WINAPI NtGdiScaleViewportExtEx( HDC hdc, INT x_num, INT x_denom,
                                     INT y_num, INT y_denom, SIZE *size )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiScaleViewportExtEx( hdc, x_num, x_denom, y_num, y_denom, size );
}

BOOL WINAPI NtGdiScaleWindowExtEx( HDC hdc, INT x_num, INT x_denom,
                                   INT y_num, INT y_denom, SIZE *size )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiScaleWindowExtEx( hdc, x_num, x_denom, y_num, y_denom, size );
}

HGDIOBJ WINAPI NtGdiSelectBitmap( HDC hdc, HGDIOBJ handle )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiSelectBitmap( hdc, handle );
}

HGDIOBJ WINAPI NtGdiSelectBrush( HDC hdc, HGDIOBJ handle )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiSelectBrush( hdc, handle );
}

BOOL WINAPI NtGdiSelectClipPath( HDC hdc, INT mode )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiSelectClipPath( hdc, mode );
}

HGDIOBJ WINAPI NtGdiSelectFont( HDC hdc, HGDIOBJ handle )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiSelectFont( hdc, handle );
}

HGDIOBJ WINAPI NtGdiSelectPen( HDC hdc, HGDIOBJ handle )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiSelectPen( hdc, handle );
}

UINT WINAPI NtGdiSetBoundsRect( HDC hdc, const RECT *rect, UINT flags )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiSetBoundsRect( hdc, rect, flags );
}

INT WINAPI NtGdiSetDIBitsToDeviceInternal( HDC hdc, INT x_dst, INT y_dst, DWORD cx,
                                           DWORD cy, INT x_src, INT y_src, UINT startscan,
                                           UINT lines, const void *bits, const BITMAPINFO *bmi,
                                           UINT coloruse, UINT max_bits, UINT max_info,
                                           BOOL xform_coords, HANDLE xform )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiSetDIBitsToDeviceInternal( hdc, x_dst, y_dst, cx, cy, x_src, y_src,
                                                        startscan, lines, bits, bmi, coloruse,
                                                        max_bits, max_info, xform_coords, xform );
}

BOOL WINAPI NtGdiSetDeviceGammaRamp( HDC hdc, void *ptr )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiSetDeviceGammaRamp( hdc, ptr );
}

DWORD WINAPI NtGdiSetLayout( HDC hdc, LONG wox, DWORD layout )
{
    if (!unix_funcs) return GDI_ERROR;
    return unix_funcs->pNtGdiSetLayout( hdc, wox, layout );
}

COLORREF WINAPI NtGdiSetPixel( HDC hdc, INT x, INT y, COLORREF color )
{
    if (!unix_funcs) return CLR_INVALID;
    return unix_funcs->pNtGdiSetPixel( hdc, x, y, color );
}

UINT WINAPI NtGdiSetSystemPaletteUse( HDC hdc, UINT use )
{
    if (!unix_funcs) return SYSPAL_ERROR;
    return unix_funcs->pNtGdiSetSystemPaletteUse( hdc, use );
}

INT WINAPI NtGdiStartDoc( HDC hdc, const DOCINFOW *doc, BOOL *banding, INT job )
{
    if (!unix_funcs) return SP_ERROR;
    return unix_funcs->pNtGdiStartDoc( hdc, doc, banding, job );
}

INT WINAPI NtGdiStartPage( HDC hdc )
{
    if (!unix_funcs) return SP_ERROR;
    return unix_funcs->pNtGdiStartPage( hdc );
}

BOOL WINAPI NtGdiStretchBlt( HDC hdc, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                             HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                             DWORD rop, COLORREF bk_color )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiStretchBlt( hdc, x_dst, y_dst, width_dst, height_dst, hdc_src,
                                         x_src, y_src, width_src, height_src, rop, bk_color );
}

INT WINAPI NtGdiStretchDIBitsInternal( HDC hdc, INT x_dst, INT y_dst, INT width_dst,
                                       INT height_dst, INT x_src, INT y_src, INT width_src,
                                       INT height_src, const void *bits, const BITMAPINFO *bmi,
                                       UINT coloruse, DWORD rop, UINT max_info, UINT max_bits,
                                       HANDLE xform )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtGdiStretchDIBitsInternal( hdc, x_dst, y_dst, width_dst, height_dst,
                                                    x_src, y_src, width_src, height_src, bits, bmi,
                                                    coloruse, rop, max_info, max_bits, xform );
}

BOOL WINAPI NtGdiStrokeAndFillPath( HDC hdc )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiStrokeAndFillPath( hdc );
}

BOOL WINAPI NtGdiStrokePath( HDC hdc )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiStrokePath( hdc );
}

BOOL WINAPI NtGdiTransparentBlt( HDC hdc, int x_dst, int y_dst, int width_dst, int height_dst,
                                 HDC hdc_src, int x_src, int y_src, int width_src, int height_src,
                                 UINT color )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiTransparentBlt( hdc, x_dst, y_dst, width_dst, height_dst, hdc_src,
                                             x_src, y_src, width_src, height_src, color );
}

BOOL WINAPI NtGdiUnrealizeObject( HGDIOBJ obj )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiUnrealizeObject( obj );
}

BOOL WINAPI NtGdiUpdateColors( HDC hdc )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiUpdateColors( hdc );
}

BOOL WINAPI NtGdiWidenPath( HDC hdc )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtGdiWidenPath( hdc );
}

NTSTATUS WINAPI NtGdiDdDDICheckVidPnExclusiveOwnership( const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *desc )
{
    if (!unix_funcs) return STATUS_NOT_SUPPORTED;
    return unix_funcs->pNtGdiDdDDICheckVidPnExclusiveOwnership( desc );
}

NTSTATUS WINAPI NtGdiDdDDICloseAdapter( const D3DKMT_CLOSEADAPTER *desc )
{
    if (!unix_funcs) return STATUS_NOT_SUPPORTED;
    return unix_funcs->pNtGdiDdDDICloseAdapter( desc );
}

NTSTATUS WINAPI NtGdiDdDDICreateDCFromMemory( D3DKMT_CREATEDCFROMMEMORY *desc )
{
    if (!unix_funcs) return STATUS_NOT_SUPPORTED;
    return unix_funcs->pNtGdiDdDDICreateDCFromMemory( desc );
}

NTSTATUS WINAPI NtGdiDdDDIDestroyDCFromMemory( const D3DKMT_DESTROYDCFROMMEMORY *desc )
{
    if (!unix_funcs) return STATUS_NOT_SUPPORTED;
    return unix_funcs->pNtGdiDdDDIDestroyDCFromMemory( desc );
}

NTSTATUS WINAPI NtGdiDdDDIDestroyDevice( const D3DKMT_DESTROYDEVICE *desc )
{
    if (!unix_funcs) return STATUS_NOT_SUPPORTED;
    return unix_funcs->pNtGdiDdDDIDestroyDevice( desc );
}

NTSTATUS WINAPI NtGdiDdDDIEscape( const D3DKMT_ESCAPE *desc )
{
    if (!unix_funcs) return STATUS_NOT_SUPPORTED;
    return unix_funcs->pNtGdiDdDDIEscape( desc );
}

NTSTATUS WINAPI NtGdiDdDDIOpenAdapterFromDeviceName( D3DKMT_OPENADAPTERFROMDEVICENAME *desc )
{
    if (!unix_funcs) return STATUS_NOT_SUPPORTED;
    return unix_funcs->pNtGdiDdDDIOpenAdapterFromDeviceName( desc );
}

NTSTATUS WINAPI NtGdiDdDDIOpenAdapterFromLuid( D3DKMT_OPENADAPTERFROMLUID *desc )
{
    if (!unix_funcs) return STATUS_NOT_SUPPORTED;
    return unix_funcs->pNtGdiDdDDIOpenAdapterFromLuid( desc );
}

NTSTATUS WINAPI NtGdiDdDDIQueryVideoMemoryInfo( D3DKMT_QUERYVIDEOMEMORYINFO *desc )
{
    if (!unix_funcs) return STATUS_NOT_SUPPORTED;
    return unix_funcs->pNtGdiDdDDIQueryVideoMemoryInfo( desc );
}

NTSTATUS WINAPI NtGdiDdDDISetVidPnSourceOwner( const D3DKMT_SETVIDPNSOURCEOWNER *desc )
{
    if (!unix_funcs) return STATUS_NOT_SUPPORTED;
    return unix_funcs->pNtGdiDdDDISetVidPnSourceOwner( desc );
}

HKL WINAPI NtUserActivateKeyboardLayout( HKL layout, UINT flags )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserActivateKeyboardLayout( layout, flags );
}

HDC WINAPI NtUserBeginPaint( HWND hwnd, PAINTSTRUCT *ps )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserBeginPaint( hwnd, ps );
}

LRESULT WINAPI NtUserCallNextHookEx( HHOOK hhook, INT code, WPARAM wparam, LPARAM lparam )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserCallNextHookEx( hhook, code, wparam, lparam );
}

ULONG_PTR WINAPI NtUserCallNoParam( ULONG code )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserCallNoParam( code );
}

ULONG_PTR WINAPI NtUserCallOneParam( ULONG_PTR arg, ULONG code )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserCallOneParam( arg, code );
}

ULONG_PTR WINAPI NtUserCallTwoParam( ULONG_PTR arg1, ULONG_PTR arg2, ULONG code )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserCallTwoParam( arg1, arg2, code );
}

ULONG_PTR WINAPI NtUserCallHwnd( HWND hwnd, DWORD code )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserCallHwnd( hwnd, code );
}

ULONG_PTR WINAPI NtUserCallHwndParam( HWND hwnd, DWORD_PTR param, DWORD code )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserCallHwndParam( hwnd, param, code );
}

BOOL WINAPI NtUserCloseClipboard(void)
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserCloseClipboard();
}

BOOL WINAPI NtUserChangeClipboardChain( HWND hwnd, HWND next )
{
    if (!unix_funcs) return DISP_CHANGE_FAILED;
    return unix_funcs->pNtUserChangeClipboardChain( hwnd, next );
}

LONG WINAPI NtUserChangeDisplaySettings( UNICODE_STRING *devname, DEVMODEW *devmode, HWND hwnd,
                                         DWORD flags, void *lparam )
{
    if (!unix_funcs) return DISP_CHANGE_FAILED;
    return unix_funcs->pNtUserChangeDisplaySettings( devname, devmode, hwnd, flags, lparam );
}

BOOL WINAPI NtUserClipCursor( const RECT *rect )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserClipCursor( rect );
}

INT WINAPI NtUserCountClipboardFormats(void)
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserCountClipboardFormats();
}

BOOL WINAPI NtUserCreateCaret( HWND hwnd, HBITMAP bitmap, int width, int height )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserCreateCaret( hwnd, bitmap, width, height );
}

HWND WINAPI NtUserCreateWindowEx( DWORD ex_style, UNICODE_STRING *class_name,
                                  UNICODE_STRING *version, UNICODE_STRING *window_name,
                                  DWORD style, INT x, INT y, INT width, INT height,
                                  HWND parent, HMENU menu, HINSTANCE instance, void *params,
                                  DWORD flags, CBT_CREATEWNDW *cbtc, DWORD unk, BOOL ansi )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserCreateWindowEx( ex_style, class_name, version, window_name,
                                              style, x, y, width, height, parent, menu,
                                              instance, params, flags, cbtc, unk, ansi );
}

HDWP WINAPI NtUserDeferWindowPosAndBand( HDWP hdwp, HWND hwnd, HWND after,
                                         INT x, INT y, INT cx, INT cy,
                                         UINT flags, UINT unk1, UINT unk2 )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserDeferWindowPosAndBand( hdwp, hwnd, after, x, y, cx, cy,
                                                     flags, unk1, unk2 );
}

BOOL WINAPI NtUserDestroyCursor( HCURSOR cursor, ULONG arg )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserDestroyCursor( cursor, arg );
}

BOOL WINAPI NtUserDestroyMenu( HMENU handle )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserDestroyMenu( handle );
}

BOOL WINAPI NtUserDestroyWindow( HWND hwnd )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserDestroyWindow( hwnd );
}

LRESULT WINAPI NtUserDispatchMessage( const MSG *msg )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserDispatchMessage( msg );
}

BOOL WINAPI NtUserDrawIconEx( HDC hdc, INT x0, INT y0, HICON icon, INT width,
                              INT height, UINT istep, HBRUSH hbr, UINT flags )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserDrawIconEx( hdc, x0, y0, icon, width, height, istep, hbr, flags );
}

BOOL WINAPI NtUserEnableMenuItem( HMENU handle, UINT id, UINT flags )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserEnableMenuItem( handle, id, flags );
}

BOOL WINAPI NtUserEndDeferWindowPosEx( HDWP hdwp, BOOL async )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserEndDeferWindowPosEx( hdwp, async );
}

BOOL WINAPI NtUserEmptyClipboard(void)
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserEmptyClipboard();
}

NTSTATUS WINAPI NtUserEnumDisplayDevices( UNICODE_STRING *device, DWORD index,
                                          DISPLAY_DEVICEW *info, DWORD flags )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserEnumDisplayDevices( device, index, info, flags );
}

BOOL WINAPI NtUserEnumDisplayMonitors( HDC hdc, RECT *rect, MONITORENUMPROC proc, LPARAM lp )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserEnumDisplayMonitors( hdc, rect, proc, lp );
}

BOOL WINAPI NtUserEnumDisplaySettings( UNICODE_STRING *device, DWORD mode,
                                       DEVMODEW *dev_mode, DWORD flags )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserEnumDisplaySettings( device, mode, dev_mode, flags );
}

INT WINAPI NtUserExcludeUpdateRgn( HDC hdc, HWND hwnd )
{
    if (!unix_funcs) return ERROR;
    return unix_funcs->pNtUserExcludeUpdateRgn( hdc, hwnd );
}

BOOL WINAPI NtUserFlashWindowEx( FLASHWINFO *info )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserFlashWindowEx( info );
}

SHORT WINAPI NtUserGetAsyncKeyState( INT key )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserGetAsyncKeyState( key );
}

ATOM WINAPI NtUserGetClassInfoEx( HINSTANCE instance, UNICODE_STRING *name, WNDCLASSEXW *wc,
                                  struct client_menu_name *menu_name, BOOL ansi )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserGetClassInfoEx( instance, name, wc, menu_name, ansi );
}

HANDLE WINAPI NtUserGetClipboardData( UINT format, struct get_clipboard_params *params )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserGetClipboardData( format, params );
}

BOOL WINAPI NtUserGetCursorInfo( CURSORINFO *info )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserGetCursorInfo( info );
}

HDC WINAPI NtUserGetDCEx( HWND hwnd, HRGN clip_rgn, DWORD flags )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserGetDCEx( hwnd, clip_rgn, flags );
}

LONG WINAPI NtUserGetDisplayConfigBufferSizes( UINT32 flags, UINT32 *num_path_info,
                                               UINT32 *num_mode_info )
{
    if (!unix_funcs) return ERROR_NOT_SUPPORTED;
    return unix_funcs->pNtUserGetDisplayConfigBufferSizes( flags, num_path_info, num_mode_info );
}

BOOL WINAPI NtUserGetIconInfo( HICON icon, ICONINFO *info, UNICODE_STRING *module,
                               UNICODE_STRING *res_name, DWORD *bpp, LONG unk )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserGetIconInfo( icon, info, module, res_name, bpp, unk );
}

UINT WINAPI NtUserGetKeyboardLayoutList( INT size, HKL *layouts )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserGetKeyboardLayoutList( size, layouts );
}

INT WINAPI NtUserGetKeyNameText( LONG lparam, WCHAR *buffer, INT size )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserGetKeyNameText( lparam, buffer, size );
}

BOOL WINAPI NtUserGetMessage( MSG *msg, HWND hwnd, UINT first, UINT last )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserGetMessage( msg, hwnd, first, last );
}

HMENU WINAPI NtUserGetSystemMenu( HWND hwnd, BOOL revert )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserGetSystemMenu( hwnd, revert );
}

BOOL WINAPI NtUserGetUpdateRect( HWND hwnd, RECT *rect, BOOL erase )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserGetUpdateRect( hwnd, rect, erase );
}

INT WINAPI NtUserGetUpdateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserGetUpdateRgn( hwnd, hrgn, erase );
}

BOOL WINAPI NtUserHideCaret( HWND hwnd )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserHideCaret( hwnd );
}

BOOL WINAPI NtUserMoveWindow( HWND hwnd, INT x, INT y, INT cx, INT cy, BOOL repaint )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserMoveWindow( hwnd, x, y, cx, cy, repaint );
}

INT WINAPI NtUserGetPriorityClipboardFormat( UINT *list, INT count )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserGetPriorityClipboardFormat( list, count );
}

DWORD WINAPI NtUserGetQueueStatus( UINT flags )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserGetQueueStatus( flags );
}

BOOL WINAPI NtUserGetUpdatedClipboardFormats( UINT *formats, UINT size, UINT *out_size )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserGetUpdatedClipboardFormats( formats, size, out_size );
}

BOOL WINAPI NtUserGetWindowPlacement( HWND hwnd, WINDOWPLACEMENT *placement )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserGetWindowPlacement( hwnd, placement );
}

BOOL WINAPI NtUserIsClipboardFormatAvailable( UINT format )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserIsClipboardFormatAvailable( format );
}

UINT WINAPI NtUserMapVirtualKeyEx( UINT code, UINT type, HKL layout )
{
    if (!unix_funcs) return -1;
    return unix_funcs->pNtUserMapVirtualKeyEx( code, type, layout );
}

LRESULT WINAPI NtUserMessageCall( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                  void *result_info, DWORD type, BOOL ansi )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserMessageCall( hwnd, msg, wparam, lparam, result_info, type, ansi );
}

DWORD WINAPI NtUserMsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles,
                                                DWORD timeout, DWORD mask, DWORD flags )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserMsgWaitForMultipleObjectsEx( count, handles, timeout, mask, flags );
}

BOOL WINAPI NtUserOpenClipboard( HWND hwnd, ULONG unk )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserOpenClipboard( hwnd, unk );
}

BOOL WINAPI NtUserPeekMessage( MSG *msg_out, HWND hwnd, UINT first, UINT last, UINT flags )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserPeekMessage( msg_out, hwnd, first, last, flags );
}

BOOL WINAPI NtUserPostMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserPostMessage( hwnd, msg, wparam, lparam );
}

BOOL WINAPI NtUserPostThreadMessage( DWORD thread, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserPostThreadMessage( thread, msg, wparam, lparam );
}

BOOL WINAPI NtUserRedrawWindow( HWND hwnd, const RECT *rect, HRGN hrgn, UINT flags )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserRedrawWindow( hwnd, rect, hrgn, flags );
}

ATOM WINAPI NtUserRegisterClassExWOW( const WNDCLASSEXW *wc, UNICODE_STRING *name, UNICODE_STRING *version,
                                      struct client_menu_name *client_menu_name, DWORD fnid, DWORD flags,
                                      DWORD *wow )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserRegisterClassExWOW( wc, name, version, client_menu_name, fnid, flags, wow );
}

BOOL WINAPI NtUserRegisterHotKey( HWND hwnd, INT id, UINT modifiers, UINT vk )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserRegisterHotKey( hwnd, id, modifiers, vk );
}

INT WINAPI NtUserReleaseDC( HWND hwnd, HDC hdc )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserReleaseDC( hwnd, hdc );
}

BOOL WINAPI NtUserScrollDC( HDC hdc, INT dx, INT dy, const RECT *scroll, const RECT *clip,
                            HRGN ret_update_rgn, RECT *update_rect )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserScrollDC( hdc, dx, dy, scroll, clip, ret_update_rgn, update_rect );
}

HPALETTE WINAPI NtUserSelectPalette( HDC hdc, HPALETTE hpal, WORD bkg )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSelectPalette( hdc, hpal, bkg );
}

UINT WINAPI NtUserSendInput( UINT count, INPUT *inputs, int size )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSendInput( count, inputs, size );
}

HWND WINAPI NtUserSetActiveWindow( HWND hwnd )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSetActiveWindow( hwnd );
}

HWND WINAPI NtUserSetCapture( HWND hwnd )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSetCapture( hwnd );
}

NTSTATUS WINAPI NtUserSetClipboardData( UINT format, HANDLE handle, struct set_clipboard_params *params )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSetClipboardData( format, handle, params );
}

HCURSOR WINAPI NtUserSetCursor( HCURSOR cursor )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSetCursor( cursor );
}

DWORD WINAPI NtUserSetClassLong( HWND hwnd, INT offset, LONG newval, BOOL ansi )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSetClassLong( hwnd, offset, newval, ansi );
}

ULONG_PTR WINAPI NtUserSetClassLongPtr( HWND hwnd, INT offset, LONG_PTR newval, BOOL ansi )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSetClassLongPtr( hwnd, offset, newval, ansi );
}

WORD WINAPI NtUserSetClassWord( HWND hwnd, INT offset, WORD newval )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSetClassWord( hwnd, offset, newval );
}

HWND WINAPI NtUserSetClipboardViewer( HWND hwnd )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSetClipboardViewer( hwnd );
}

BOOL WINAPI NtUserSetCursorIconData( HCURSOR cursor, UNICODE_STRING *module, UNICODE_STRING *res_name,
                                     struct cursoricon_desc *desc )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserSetCursorIconData( cursor, module, res_name, desc );
}

BOOL WINAPI NtUserSetCursorPos( INT x, INT y )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserSetCursorPos( x, y );
}

HWND WINAPI NtUserSetFocus( HWND hwnd )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserSetFocus( hwnd );
}

void WINAPI NtUserSetInternalWindowPos( HWND hwnd, UINT cmd, RECT *rect, POINT *pt )
{
    if (!unix_funcs) return;
    return unix_funcs->pNtUserSetInternalWindowPos( hwnd, cmd, rect, pt );
}

BOOL WINAPI NtUserSetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserSetLayeredWindowAttributes( hwnd, key, alpha, flags );
}

BOOL WINAPI NtUserSetMenu( HWND hwnd, HMENU menu )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserSetMenu( hwnd, menu );
}

HWND WINAPI NtUserSetParent( HWND hwnd, HWND parent )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSetParent( hwnd, parent );
}

BOOL WINAPI NtUserSetSysColors( INT count, const INT *colors, const COLORREF *values )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserSetSysColors( count, colors, values );
}

BOOL WINAPI NtUserSetSystemMenu( HWND hwnd, HMENU menu )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserSetSystemMenu( hwnd, menu );
}

LONG WINAPI NtUserSetWindowLong( HWND hwnd, INT offset, LONG newval, BOOL ansi )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSetWindowLong( hwnd, offset, newval, ansi );
}

LONG_PTR WINAPI NtUserSetWindowLongPtr( HWND hwnd, INT offset, LONG_PTR newval, BOOL ansi )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSetWindowLongPtr( hwnd, offset, newval, ansi );
}

BOOL WINAPI NtUserSetWindowPlacement( HWND hwnd, const WINDOWPLACEMENT *wpl )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSetWindowPlacement( hwnd, wpl );
}

BOOL WINAPI NtUserSetWindowPos( HWND hwnd, HWND after, INT x, INT y, INT cx, INT cy, UINT flags )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSetWindowPos( hwnd, after, x, y, cx, cy, flags );
}

int WINAPI NtUserSetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSetWindowRgn( hwnd, hrgn, redraw );
}

WORD WINAPI NtUserSetWindowWord( HWND hwnd, INT offset, WORD newval )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserSetWindowWord( hwnd, offset, newval );
}

BOOL WINAPI NtUserShowCaret( HWND hwnd )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserShowCaret( hwnd );
}

INT WINAPI NtUserShowCursor( BOOL show )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserShowCursor( show );
}

BOOL WINAPI NtUserShowWindowAsync( HWND hwnd, INT cmd )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserShowWindowAsync( hwnd, cmd );
}

BOOL WINAPI NtUserShowWindow( HWND hwnd, INT cmd )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserShowWindow( hwnd, cmd );
}

BOOL WINAPI NtUserSystemParametersInfo( UINT action, UINT val, PVOID ptr, UINT winini )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserSystemParametersInfo( action, val, ptr, winini );
}

BOOL WINAPI NtUserSystemParametersInfoForDpi( UINT action, UINT val, PVOID ptr, UINT winini, UINT dpi )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserSystemParametersInfoForDpi( action, val, ptr, winini, dpi );
}

INT WINAPI NtUserToUnicodeEx( UINT virt, UINT scan, const BYTE *state,
                              WCHAR *str, int size, UINT flags, HKL layout )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserToUnicodeEx( virt, scan, state, str, size, flags, layout );
}

BOOL WINAPI NtUserTrackMouseEvent( TRACKMOUSEEVENT *info )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserTrackMouseEvent( info );
}

INT WINAPI NtUserTranslateAccelerator( HWND hwnd, HACCEL accel, MSG *msg )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserTranslateAccelerator( hwnd, accel, msg );
}

BOOL WINAPI NtUserTranslateMessage( const MSG *msg, UINT flags )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserTranslateMessage( msg, flags );
}

BOOL WINAPI NtUserUnregisterClass( UNICODE_STRING *name, HINSTANCE instance,
                                   struct client_menu_name *client_menu_name )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserUnregisterClass( name, instance, client_menu_name );
}

BOOL WINAPI NtUserUnregisterHotKey( HWND hwnd, INT id )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserUnregisterHotKey( hwnd, id );
}

BOOL WINAPI NtUserUpdateLayeredWindow( HWND hwnd, HDC hdc_dst, const POINT *pts_dst, const SIZE *size,
                                       HDC hdc_src, const POINT *pts_src, COLORREF key,
                                       const BLENDFUNCTION *blend, DWORD flags, const RECT *dirty )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->pNtUserUpdateLayeredWindow( hwnd, hdc_dst, pts_dst, size, hdc_src, pts_src,
                                                   key, blend, flags, dirty );
}

WORD WINAPI NtUserVkKeyScanEx( WCHAR chr, HKL layout )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserVkKeyScanEx( chr, layout );
}

DWORD WINAPI NtUserWaitForInputIdle( HANDLE process, DWORD timeout, BOOL wow )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserWaitForInputIdle( process, timeout, wow );
}

HWND WINAPI NtUserWindowFromPoint( LONG x, LONG y )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pNtUserWindowFromPoint( x, y );
}

INT WINAPI SetDIBits( HDC hdc, HBITMAP hbitmap, UINT startscan,
                      UINT lines, const void *bits, const BITMAPINFO *info,
                      UINT coloruse )
{
    if (!unix_funcs) return 0;
    return unix_funcs->pSetDIBits( hdc, hbitmap, startscan, lines, bits, info, coloruse );
}

BOOL CDECL __wine_get_icm_profile( HDC hdc, BOOL allow_default, DWORD *size, WCHAR *filename )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->get_icm_profile( hdc, allow_default, size, filename );
}

BOOL CDECL __wine_get_brush_bitmap_info( HBRUSH handle, BITMAPINFO *info, void *bits, UINT *usage )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->get_brush_bitmap_info( handle, info, bits, usage );
}

BOOL CDECL __wine_get_file_outline_text_metric( const WCHAR *path, OUTLINETEXTMETRICW *otm )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->get_file_outline_text_metric( path, otm );
}

const struct vulkan_funcs * CDECL __wine_get_vulkan_driver(UINT version)
{
    if (!unix_funcs) return NULL;
    return unix_funcs->get_vulkan_driver( version );
}

struct opengl_funcs * CDECL __wine_get_wgl_driver( HDC hdc, UINT version )
{
    if (!unix_funcs) return NULL;
    return unix_funcs->get_wgl_driver( hdc, version );
}

BOOL CDECL __wine_send_input( HWND hwnd, const INPUT *input, const RAWINPUT *rawinput )
{
    if (!unix_funcs) return FALSE;
    return unix_funcs->wine_send_input( hwnd, input, rawinput );
}

/***********************************************************************
 *           __wine_set_user_driver    (win32u.@)
 */
void CDECL __wine_set_user_driver( const struct user_driver_funcs *funcs, UINT version )
{
    if (!unix_funcs) return;
    return unix_funcs->set_user_driver( funcs, version );
}

extern void wrappers_init( unixlib_handle_t handle )
{
    const void *args;
    if (!__wine_unix_call( handle, 1, &args )) unix_funcs = args;
}
