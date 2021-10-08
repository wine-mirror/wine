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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
#include "win32u_private.h"
#include "wine/unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

static const struct unix_funcs *unix_funcs;

INT WINAPI NtGdiAbortDoc( HDC hdc )
{
    return unix_funcs->pNtGdiAbortDoc( hdc );
}

BOOL WINAPI NtGdiAbortPath( HDC hdc )
{
    return unix_funcs->pNtGdiAbortPath( hdc );
}

HANDLE WINAPI NtGdiAddFontMemResourceEx( void *ptr, DWORD size, void *dv, ULONG dv_size,
                                         DWORD *count )
{
    return unix_funcs->pNtGdiAddFontMemResourceEx( ptr, size, dv, dv_size, count );
}

INT WINAPI NtGdiAddFontResourceW( const WCHAR *str, ULONG size, ULONG files, DWORD flags,
                                  DWORD tid, void *dv )
{
    return unix_funcs->pNtGdiAddFontResourceW( str, size, files, flags, tid, dv );
}

BOOL WINAPI NtGdiAlphaBlend( HDC hdc_dst, int x_dst, int y_dst, int width_dst, int height_dst,
                             HDC hdc_src, int x_src, int y_src, int width_src, int height_src,
                             BLENDFUNCTION blend_function, HANDLE xform )
{
    return unix_funcs->pNtGdiAlphaBlend( hdc_dst, x_dst, y_dst, width_dst, height_dst, hdc_src,
                                         x_src, y_src, width_src, height_src, blend_function, xform );
}

BOOL WINAPI NtGdiAngleArc( HDC hdc, INT x, INT y, DWORD radius, FLOAT start_angle, FLOAT sweep_angle )
{
    return unix_funcs->pNtGdiAngleArc( hdc, x, y, radius, start_angle, sweep_angle );
}

BOOL WINAPI NtGdiArcInternal( UINT type, HDC hdc, INT left, INT top, INT right, INT bottom,
                              INT xstart, INT ystart, INT xend, INT yend )
{
    return unix_funcs->pNtGdiArcInternal( type, hdc, left, top, right, bottom, xstart, ystart, xend, yend );
}

BOOL WINAPI NtGdiBeginPath( HDC hdc )
{
    return unix_funcs->pNtGdiBeginPath( hdc );
}

BOOL WINAPI NtGdiBitBlt( HDC hdc_dst, INT x_dst, INT y_dst, INT width, INT height, HDC hdc_src,
                         INT x_src, INT y_src, DWORD rop, DWORD bk_color, FLONG fl )
{
    return unix_funcs->pNtGdiBitBlt( hdc_dst, x_dst, y_dst, width, height, hdc_src, x_src, y_src,
                                     rop, bk_color, fl );
}

BOOL WINAPI NtGdiCloseFigure( HDC hdc )
{
    return unix_funcs->pNtGdiCloseFigure( hdc );
}

INT WINAPI NtGdiCombineRgn( HRGN dest, HRGN src1, HRGN src2, INT mode )
{
    return unix_funcs->pNtGdiCombineRgn( dest, src1, src2, mode );
}

BOOL WINAPI NtGdiComputeXformCoefficients( HDC hdc )
{
    return unix_funcs->pNtGdiComputeXformCoefficients( hdc );
}

HBITMAP WINAPI NtGdiCreateBitmap( INT width, INT height, UINT planes, UINT bpp, const void *bits )
{
    return unix_funcs->pNtGdiCreateBitmap( width, height, planes, bpp, bits );
}

HANDLE WINAPI NtGdiCreateClientObj( ULONG type )
{
    return unix_funcs->pNtGdiCreateClientObj( type );
}

HBITMAP WINAPI NtGdiCreateCompatibleBitmap( HDC hdc, INT width, INT height )
{
    return unix_funcs->pNtGdiCreateCompatibleBitmap( hdc, width, height );
}

HDC WINAPI NtGdiCreateCompatibleDC( HDC hdc )
{
    return unix_funcs->pNtGdiCreateCompatibleDC( hdc );
}

HBITMAP WINAPI NtGdiCreateDIBSection( HDC hdc, HANDLE section, DWORD offset, const BITMAPINFO *bmi,
                                      UINT usage, UINT header_size, ULONG flags,
                                      ULONG_PTR color_space, void **bits )
{
    return unix_funcs->pNtGdiCreateDIBSection( hdc, section, offset, bmi, usage, header_size, flags,
                                               color_space, bits );
}

HBITMAP WINAPI NtGdiCreateDIBitmapInternal( HDC hdc, INT width, INT height, DWORD init,
                                            const void *bits, const BITMAPINFO *data,
                                            UINT coloruse, UINT max_info, UINT max_bits,
                                            ULONG flags, HANDLE xform )
{
    return unix_funcs->pNtGdiCreateDIBitmapInternal( hdc, width, height, init, bits, data,
                                                     coloruse, max_info, max_bits, flags, xform );
}

HRGN WINAPI NtGdiCreateEllipticRgn( INT left, INT top, INT right, INT bottom )
{
    return unix_funcs->pNtGdiCreateEllipticRgn( left, top, right, bottom );
}

HPALETTE WINAPI NtGdiCreateHalftonePalette( HDC hdc )
{
    return unix_funcs->pNtGdiCreateHalftonePalette( hdc );
}

HDC WINAPI NtGdiCreateMetafileDC( HDC hdc )
{
    return unix_funcs->pNtGdiCreateMetafileDC( hdc );
}

HPALETTE WINAPI NtGdiCreatePaletteInternal( const LOGPALETTE *palette, UINT count )
{
    return unix_funcs->pNtGdiCreatePaletteInternal( palette, count );
}

HPEN WINAPI NtGdiCreatePen( INT style, INT width, COLORREF color, HBRUSH brush )
{
    return unix_funcs->pNtGdiCreatePen( style, width, color, brush );
}

HRGN WINAPI NtGdiCreateRectRgn( INT left, INT top, INT right, INT bottom )
{
    return unix_funcs->pNtGdiCreateRectRgn( left, top, right, bottom );
}

HRGN WINAPI NtGdiCreateRoundRectRgn( INT left, INT top, INT right, INT bottom,
                                     INT ellipse_width, INT ellipse_height )
{
    return unix_funcs->pNtGdiCreateRoundRectRgn( left, top, right, bottom, ellipse_width, ellipse_height );
}

BOOL WINAPI NtGdiDeleteClientObj( HGDIOBJ obj )
{
    return unix_funcs->pNtGdiDeleteClientObj( obj );
}

BOOL WINAPI NtGdiDeleteObjectApp( HGDIOBJ obj )
{
    return unix_funcs->pNtGdiDeleteObjectApp( obj );
}

INT WINAPI NtGdiDescribePixelFormat( HDC hdc, INT format, UINT size, PIXELFORMATDESCRIPTOR *descr )
{
    return unix_funcs->pNtGdiDescribePixelFormat( hdc, format, size, descr );
}

LONG WINAPI NtGdiDoPalette( HGDIOBJ handle, WORD start, WORD count, void *entries,
                            DWORD func, BOOL inbound )
{
    return unix_funcs->pNtGdiDoPalette( handle, start, count, entries, func, inbound );
}

BOOL WINAPI NtGdiDrawStream( HDC hdc, ULONG in, void *pvin )
{
    return unix_funcs->pNtGdiDrawStream( hdc, in, pvin );
}

BOOL WINAPI NtGdiEllipse( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    return unix_funcs->pNtGdiEllipse( hdc, left, top, right, bottom );
}

INT WINAPI NtGdiEndDoc( HDC hdc )
{
    return unix_funcs->pNtGdiEndDoc( hdc );
}

BOOL WINAPI NtGdiEndPath( HDC hdc )
{
    return unix_funcs->pNtGdiEndPath( hdc );
}

INT WINAPI NtGdiEndPage( HDC hdc )
{
    return unix_funcs->pNtGdiEndPage( hdc );
}

BOOL WINAPI NtGdiEnumFonts( HDC hdc, ULONG type, ULONG win32_compat, ULONG face_name_len,
                            const WCHAR *face_name, ULONG charset, ULONG *count, void *buf )
{
    return unix_funcs->pNtGdiEnumFonts( hdc, type, win32_compat, face_name_len, face_name,
                                        charset, count, buf );
}

BOOL WINAPI NtGdiEqualRgn( HRGN hrgn1, HRGN hrgn2 )
{
    return unix_funcs->pNtGdiEqualRgn( hrgn1, hrgn2 );
}

INT WINAPI NtGdiExcludeClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    return unix_funcs->pNtGdiExcludeClipRect( hdc, left, top, right, bottom );
}

HPEN WINAPI NtGdiExtCreatePen( DWORD style, DWORD width, ULONG brush_style, ULONG color,
                               ULONG_PTR client_hatch, ULONG_PTR hatch, DWORD style_count,
                               const DWORD *style_bits, ULONG dib_size, BOOL old_style,
                               HBRUSH brush )
{
    return unix_funcs->pNtGdiExtCreatePen( style, width, brush_style, color, client_hatch, hatch, style_count,
                                           style_bits, dib_size, old_style, brush );
}

INT WINAPI NtGdiExtEscape( HDC hdc, WCHAR *driver, INT driver_id, INT escape, INT input_size,
                           const char *input, INT output_size, char *output )
{
    return unix_funcs->pNtGdiExtEscape( hdc, driver, driver_id, escape, input_size, input,
                                        output_size, output );
}

BOOL WINAPI NtGdiExtFloodFill( HDC hdc, INT x, INT y, COLORREF color, UINT type )
{
    return unix_funcs->pNtGdiExtFloodFill( hdc, x, y, color, type );
}

BOOL WINAPI NtGdiExtTextOutW( HDC hdc, INT x, INT y, UINT flags, const RECT *rect,
                              const WCHAR *str, UINT count, const INT *dx, DWORD cp )
{
    return unix_funcs->pNtGdiExtTextOutW( hdc, x, y, flags, rect, str, count, dx, cp );
}

HRGN WINAPI NtGdiExtCreateRegion( const XFORM *xform, DWORD count, const RGNDATA *data )
{
    return unix_funcs->pNtGdiExtCreateRegion( xform, count, data );
}

INT WINAPI NtGdiExtGetObjectW( HGDIOBJ handle, INT count, void *buffer )
{
    return unix_funcs->pNtGdiExtGetObjectW( handle, count, buffer );
}

INT WINAPI NtGdiExtSelectClipRgn( HDC hdc, HRGN region, INT mode )
{
    return unix_funcs->pNtGdiExtSelectClipRgn( hdc, region, mode );
}

BOOL WINAPI NtGdiFillPath( HDC hdc )
{
    return unix_funcs->pNtGdiFillPath( hdc );
}

BOOL WINAPI NtGdiFillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush )
{
    return unix_funcs->pNtGdiFillRgn( hdc, hrgn, hbrush );
}

BOOL WINAPI NtGdiFlattenPath( HDC hdc )
{
    return unix_funcs->pNtGdiFlattenPath( hdc );
}

BOOL WINAPI NtGdiFontIsLinked( HDC hdc )
{
    return unix_funcs->pNtGdiFontIsLinked( hdc );
}

BOOL WINAPI NtGdiFrameRgn( HDC hdc, HRGN hrgn, HBRUSH brush, INT width, INT height )
{
    return unix_funcs->pNtGdiFrameRgn( hdc, hrgn, brush, width, height );
}

BOOL WINAPI NtGdiGetAndSetDCDword( HDC hdc, UINT method, DWORD value, DWORD *result )
{
    return unix_funcs->pNtGdiGetAndSetDCDword( hdc, method, value, result );
}

INT WINAPI NtGdiGetAppClipBox( HDC hdc, RECT *rect )
{
    return unix_funcs->pNtGdiGetAppClipBox( hdc, rect );
}

LONG WINAPI NtGdiGetBitmapBits( HBITMAP bitmap, LONG count, void *bits )
{
    return unix_funcs->pNtGdiGetBitmapBits( bitmap, count, bits );
}

BOOL WINAPI NtGdiGetBitmapDimension( HBITMAP bitmap, SIZE *size )
{
    return unix_funcs->pNtGdiGetBitmapDimension( bitmap, size );
}

UINT WINAPI NtGdiGetBoundsRect( HDC hdc, RECT *rect, UINT flags )
{
    return unix_funcs->pNtGdiGetBoundsRect( hdc, rect, flags );
}

BOOL WINAPI NtGdiGetCharABCWidthsW( HDC hdc, UINT first, UINT last, WCHAR *chars,
                                    ULONG flags, void *buffer )
{
    return unix_funcs->pNtGdiGetCharABCWidthsW( hdc, first, last, chars, flags, buffer );
}

BOOL WINAPI NtGdiGetCharWidthW( HDC hdc, UINT first_char, UINT last_char, WCHAR *chars,
                                ULONG flags, void *buffer )
{
    return unix_funcs->pNtGdiGetCharWidthW( hdc, first_char, last_char, chars, flags, buffer );
}

BOOL WINAPI NtGdiGetCharWidthInfo( HDC hdc, struct char_width_info *info )
{
    return unix_funcs->pNtGdiGetCharWidthInfo( hdc, info );
}

BOOL WINAPI NtGdiGetColorAdjustment( HDC hdc, COLORADJUSTMENT *ca )
{
    return unix_funcs->pNtGdiGetColorAdjustment( hdc, ca );
}

HANDLE WINAPI NtGdiGetDCObject( HDC hdc, UINT type )
{
    return unix_funcs->pNtGdiGetDCObject( hdc, type );
}

INT WINAPI NtGdiGetDIBitsInternal( HDC hdc, HBITMAP hbitmap, UINT startscan, UINT lines,
                                   void *bits, BITMAPINFO *info, UINT coloruse,
                                   UINT max_bits, UINT max_info )
{
    return unix_funcs->pNtGdiGetDIBitsInternal( hdc, hbitmap, startscan, lines, bits, info, coloruse,
                                                max_bits, max_info );
}

INT WINAPI NtGdiGetDeviceCaps( HDC hdc, INT cap )
{
    return unix_funcs->pNtGdiGetDeviceCaps( hdc, cap );
}

BOOL WINAPI NtGdiGetDeviceGammaRamp( HDC hdc, void *ptr )
{
    return unix_funcs->pNtGdiGetDeviceGammaRamp( hdc, ptr );
}

DWORD WINAPI NtGdiGetFontData( HDC hdc, DWORD table, DWORD offset, void *buffer, DWORD length )
{
    return unix_funcs->pNtGdiGetFontData( hdc, table, offset, buffer, length );
}

BOOL WINAPI NtGdiGetFontFileData( DWORD instance_id, DWORD file_index, UINT64 *offset,
                                  void *buff, DWORD buff_size )
{
    return unix_funcs->pNtGdiGetFontFileData( instance_id, file_index, offset, buff, buff_size );
}

BOOL WINAPI NtGdiGetFontFileInfo( DWORD instance_id, DWORD file_index, struct font_fileinfo *info,
                                  SIZE_T size, SIZE_T *needed )
{
    return unix_funcs->pNtGdiGetFontFileInfo( instance_id, file_index, info, size, needed );
}

DWORD WINAPI NtGdiGetFontUnicodeRanges( HDC hdc, GLYPHSET *lpgs )
{
    return unix_funcs->pNtGdiGetFontUnicodeRanges( hdc, lpgs );
}

DWORD WINAPI NtGdiGetGlyphIndicesW( HDC hdc, const WCHAR *str, INT count,
                                    WORD *indices, DWORD flags )
{
    return unix_funcs->pNtGdiGetGlyphIndicesW( hdc, str, count, indices, flags );
}

DWORD WINAPI NtGdiGetGlyphOutline( HDC hdc, UINT ch, UINT format, GLYPHMETRICS *metrics,
                                   DWORD size, void *buffer, const MAT2 *mat2,
                                   BOOL ignore_rotation )
{
    return unix_funcs->pNtGdiGetGlyphOutline( hdc, ch, format, metrics, size, buffer, mat2, ignore_rotation );
}

DWORD WINAPI NtGdiGetKerningPairs( HDC hdc, DWORD count, KERNINGPAIR *kern_pair )
{
    return unix_funcs->pNtGdiGetKerningPairs( hdc, count, kern_pair );
}

COLORREF WINAPI NtGdiGetNearestColor( HDC hdc, COLORREF color )
{
    return unix_funcs->pNtGdiGetNearestColor( hdc, color );
}

UINT WINAPI NtGdiGetNearestPaletteIndex( HPALETTE hpalette, COLORREF color )
{
    return unix_funcs->pNtGdiGetNearestPaletteIndex( hpalette, color );
}

UINT WINAPI NtGdiGetOutlineTextMetricsInternalW( HDC hdc, UINT cbData,
                                                 OUTLINETEXTMETRICW *otm, ULONG opts )
{
    return unix_funcs->pNtGdiGetOutlineTextMetricsInternalW( hdc, cbData, otm, opts );
}

INT WINAPI NtGdiGetPath( HDC hdc, POINT *points, BYTE *types, INT size )
{
    return unix_funcs->pNtGdiGetPath( hdc, points, types, size );
}

COLORREF WINAPI NtGdiGetPixel( HDC hdc, INT x, INT y )
{
    return unix_funcs->pNtGdiGetPixel( hdc, x, y );
}

INT WINAPI NtGdiGetRandomRgn( HDC hdc, HRGN region, INT code )
{
    return unix_funcs->pNtGdiGetRandomRgn( hdc, region, code );
}

BOOL WINAPI NtGdiGetRasterizerCaps( RASTERIZER_STATUS *status, UINT size )
{
    return unix_funcs->pNtGdiGetRasterizerCaps( status, size );
}

BOOL WINAPI NtGdiGetRealizationInfo( HDC hdc, struct font_realization_info *info )
{
    return unix_funcs->pNtGdiGetRealizationInfo( hdc, info );
}

DWORD WINAPI NtGdiGetRegionData( HRGN hrgn, DWORD count, RGNDATA *data )
{
    return unix_funcs->pNtGdiGetRegionData( hrgn, count, data );
}

INT WINAPI NtGdiGetRgnBox( HRGN hrgn, RECT *rect )
{
    return unix_funcs->pNtGdiGetRgnBox( hrgn, rect );
}

DWORD WINAPI NtGdiGetSpoolMessage( void *ptr1, DWORD data2, void *ptr3, DWORD data4 )
{
    return unix_funcs->pNtGdiGetSpoolMessage( ptr1, data2, ptr3, data4 );
}

UINT WINAPI NtGdiGetSystemPaletteUse( HDC hdc )
{
    return unix_funcs->pNtGdiGetSystemPaletteUse( hdc );
}

UINT WINAPI NtGdiGetTextCharsetInfo( HDC hdc, FONTSIGNATURE *fs, DWORD flags )
{
    return unix_funcs->pNtGdiGetTextCharsetInfo( hdc, fs, flags );
}

BOOL WINAPI NtGdiGetTextExtentExW( HDC hdc, const WCHAR *str, INT count, INT max_ext,
                                   INT *nfit, INT *dxs, SIZE *size, UINT flags )
{
    return unix_funcs->pNtGdiGetTextExtentExW( hdc, str, count, max_ext, nfit, dxs, size, flags );
}

INT WINAPI NtGdiGetTextFaceW( HDC hdc, INT count, WCHAR *name, BOOL alias_name )
{
    return unix_funcs->pNtGdiGetTextFaceW( hdc, count, name, alias_name );
}

BOOL WINAPI NtGdiGetTextMetricsW( HDC hdc, TEXTMETRICW *metrics, ULONG flags )
{
    return unix_funcs->pNtGdiGetTextMetricsW( hdc, metrics, flags );
}

BOOL WINAPI NtGdiGetTransform( HDC hdc, DWORD which, XFORM *xform )
{
    return unix_funcs->pNtGdiGetTransform( hdc, which, xform );
}

BOOL WINAPI NtGdiGradientFill( HDC hdc, TRIVERTEX *vert_array, ULONG nvert,
                               void *grad_array, ULONG ngrad, ULONG mode )
{
    return unix_funcs->pNtGdiGradientFill( hdc, vert_array, nvert, grad_array, ngrad, mode );
}

HFONT WINAPI NtGdiHfontCreate( const ENUMLOGFONTEXDVW *enumex, ULONG unk2, ULONG unk3,
                               ULONG unk4, void *data )
{
    return unix_funcs->pNtGdiHfontCreate( enumex, unk2, unk3, unk4, data );
}

DWORD WINAPI NtGdiInitSpool(void)
{
    return unix_funcs->pNtGdiInitSpool();
}

INT WINAPI NtGdiIntersectClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    return unix_funcs->pNtGdiIntersectClipRect( hdc, left, top, right, bottom );
}

BOOL WINAPI NtGdiInvertRgn( HDC hdc, HRGN hrgn )
{
    return unix_funcs->pNtGdiInvertRgn( hdc, hrgn );
}

BOOL WINAPI NtGdiLineTo( HDC hdc, INT x, INT y )
{
    return unix_funcs->pNtGdiLineTo( hdc, x, y );
}

BOOL WINAPI NtGdiMaskBlt( HDC hdc, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                          HDC hdc_src, INT x_src, INT y_src, HBITMAP mask,
                          INT x_mask, INT y_mask, DWORD rop, DWORD bk_color )
{
    return unix_funcs->pNtGdiMaskBlt( hdc, x_dst, y_dst, width_dst, height_dst, hdc_src,
                                      x_src, y_src, mask, x_mask, y_mask, rop, bk_color );
}

BOOL WINAPI NtGdiModifyWorldTransform( HDC hdc, const XFORM *xform, DWORD mode )
{
    return unix_funcs->pNtGdiModifyWorldTransform( hdc, xform, mode );
}

BOOL WINAPI NtGdiMoveTo( HDC hdc, INT x, INT y, POINT *pt )
{
    return unix_funcs->pNtGdiMoveTo( hdc, x, y, pt );
}

INT WINAPI NtGdiOffsetClipRgn( HDC hdc, INT x, INT y )
{
    return unix_funcs->pNtGdiOffsetClipRgn( hdc, x, y );
}

INT WINAPI NtGdiOffsetRgn( HRGN hrgn, INT x, INT y )
{
    return unix_funcs->pNtGdiOffsetRgn( hrgn, x, y );
}

HDC WINAPI NtGdiOpenDCW( UNICODE_STRING *device, const DEVMODEW *devmode,
                         UNICODE_STRING *output, ULONG type, BOOL is_display,
                         HANDLE hspool, DRIVER_INFO_2W *driver_info, void *pdev )
{
    return unix_funcs->pNtGdiOpenDCW( device, devmode, output, type, is_display,
                                      hspool, driver_info, pdev );
}

BOOL WINAPI NtGdiPatBlt( HDC hdc, INT left, INT top, INT width, INT height, DWORD rop )
{
    return unix_funcs->pNtGdiPatBlt( hdc, left, top, width, height, rop );
}

HRGN WINAPI NtGdiPathToRegion( HDC hdc )
{
    return unix_funcs->pNtGdiPathToRegion( hdc );
}

BOOL WINAPI NtGdiPlgBlt( HDC hdc, const POINT *point, HDC hdc_src, INT x_src, INT y_src,
                         INT width, INT height, HBITMAP mask, INT x_mask, INT y_mask,
                         DWORD bk_color )
{
    return unix_funcs->pNtGdiPlgBlt( hdc, point, hdc_src, x_src, y_src, width, height, mask,
                                     x_mask, y_mask, bk_color );
}

BOOL WINAPI NtGdiPolyDraw( HDC hdc, const POINT *points, const BYTE *types, DWORD count )
{
    return unix_funcs->pNtGdiPolyDraw( hdc, points, types, count );
}

ULONG WINAPI NtGdiPolyPolyDraw( HDC hdc, const POINT *points, const UINT *counts,
                                DWORD count, UINT function )
{
    return unix_funcs->pNtGdiPolyPolyDraw( hdc, points, counts, count, function );
}

BOOL WINAPI NtGdiPtInRegion( HRGN hrgn, INT x, INT y )
{
    return unix_funcs->pNtGdiPtInRegion( hrgn, x, y );
}

BOOL WINAPI NtGdiPtVisible( HDC hdc, INT x, INT y )
{
    return unix_funcs->pNtGdiPtVisible( hdc, x, y );
}

BOOL WINAPI NtGdiRectInRegion( HRGN hrgn, const RECT *rect )
{
    return unix_funcs->pNtGdiRectInRegion( hrgn, rect );
}

BOOL WINAPI NtGdiRectVisible( HDC hdc, const RECT *rect )
{
    return unix_funcs->pNtGdiRectVisible( hdc, rect );
}

BOOL WINAPI NtGdiRectangle( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    return unix_funcs->pNtGdiRectangle( hdc, left, top, right, bottom );
}

BOOL WINAPI NtGdiRemoveFontMemResourceEx( HANDLE handle )
{
    return unix_funcs->pNtGdiRemoveFontMemResourceEx( handle );
}

BOOL WINAPI NtGdiRemoveFontResourceW( const WCHAR *str, ULONG size, ULONG files,
                                      DWORD flags, DWORD tid, void *dv )
{
    return unix_funcs->pNtGdiRemoveFontResourceW( str, size, files, flags, tid, dv );
}

BOOL WINAPI NtGdiResetDC( HDC hdc, const DEVMODEW *devmode, BOOL *banding,
                          DRIVER_INFO_2W *driver_info, void *dev )
{
    return unix_funcs->pNtGdiResetDC( hdc, devmode, banding, driver_info, dev );
}

BOOL WINAPI NtGdiResizePalette( HPALETTE palette, UINT count )
{
    return unix_funcs->pNtGdiResizePalette( palette, count );
}

BOOL WINAPI NtGdiRestoreDC( HDC hdc, INT level )
{
    return unix_funcs->pNtGdiRestoreDC( hdc, level );
}

BOOL WINAPI NtGdiRoundRect( HDC hdc, INT left, INT top, INT right,
                            INT bottom, INT ell_width, INT ell_height )
{
    return unix_funcs->pNtGdiRoundRect( hdc, left, top, right, bottom, ell_width, ell_height );
}

INT WINAPI NtGdiSaveDC( HDC hdc )
{
    return unix_funcs->pNtGdiSaveDC( hdc );
}

BOOL WINAPI NtGdiScaleViewportExtEx( HDC hdc, INT x_num, INT x_denom,
                                     INT y_num, INT y_denom, SIZE *size )
{
    return unix_funcs->pNtGdiScaleViewportExtEx( hdc, x_num, x_denom, y_num, y_denom, size );
}

BOOL WINAPI NtGdiScaleWindowExtEx( HDC hdc, INT x_num, INT x_denom,
                                   INT y_num, INT y_denom, SIZE *size )
{
    return unix_funcs->pNtGdiScaleWindowExtEx( hdc, x_num, x_denom, y_num, y_denom, size );
}

HGDIOBJ WINAPI NtGdiSelectBitmap( HDC hdc, HGDIOBJ handle )
{
    return unix_funcs->pNtGdiSelectBitmap( hdc, handle );
}

HGDIOBJ WINAPI NtGdiSelectBrush( HDC hdc, HGDIOBJ handle )
{
    return unix_funcs->pNtGdiSelectBrush( hdc, handle );
}

BOOL WINAPI NtGdiSelectClipPath( HDC hdc, INT mode )
{
    return unix_funcs->pNtGdiSelectClipPath( hdc, mode );
}

HGDIOBJ WINAPI NtGdiSelectFont( HDC hdc, HGDIOBJ handle )
{
    return unix_funcs->pNtGdiSelectFont( hdc, handle );
}

HGDIOBJ WINAPI NtGdiSelectPen( HDC hdc, HGDIOBJ handle )
{
    return unix_funcs->pNtGdiSelectPen( hdc, handle );
}

LONG WINAPI NtGdiSetBitmapBits( HBITMAP hbitmap, LONG count, const void *bits )
{
    return unix_funcs->pNtGdiSetBitmapBits( hbitmap, count, bits );
}

BOOL WINAPI NtGdiSetBitmapDimension( HBITMAP hbitmap, INT x, INT y, SIZE *prev_size )
{
    return unix_funcs->pNtGdiSetBitmapDimension( hbitmap, x, y, prev_size );
}

BOOL WINAPI NtGdiSetBrushOrg( HDC hdc, INT x, INT y, POINT *prev_org )
{
    return unix_funcs->pNtGdiSetBrushOrg( hdc, x, y, prev_org );
}

UINT WINAPI NtGdiSetBoundsRect( HDC hdc, const RECT *rect, UINT flags )
{
    return unix_funcs->pNtGdiSetBoundsRect( hdc, rect, flags );
}

BOOL WINAPI NtGdiSetColorAdjustment( HDC hdc, const COLORADJUSTMENT *ca )
{
    return unix_funcs->pNtGdiSetColorAdjustment( hdc, ca );
}

INT WINAPI NtGdiSetDIBitsToDeviceInternal( HDC hdc, INT x_dst, INT y_dst, DWORD cx,
                                           DWORD cy, INT x_src, INT y_src, UINT startscan,
                                           UINT lines, const void *bits, const BITMAPINFO *bmi,
                                           UINT coloruse, UINT max_bits, UINT max_info,
                                           BOOL xform_coords, HANDLE xform )
{
    return unix_funcs->pNtGdiSetDIBitsToDeviceInternal( hdc, x_dst, y_dst, cx, cy, x_src, y_src,
                                                        startscan, lines, bits, bmi, coloruse,
                                                        max_bits, max_info, xform_coords, xform );
}

BOOL WINAPI NtGdiSetDeviceGammaRamp( HDC hdc, void *ptr )
{
    return unix_funcs->pNtGdiSetDeviceGammaRamp( hdc, ptr );
}

DWORD WINAPI NtGdiSetLayout( HDC hdc, LONG wox, DWORD layout )
{
    return unix_funcs->pNtGdiSetLayout( hdc, wox, layout );
}

BOOL WINAPI NtGdiSetMagicColors( HDC hdc, DWORD magic, ULONG index )
{
    return unix_funcs->pNtGdiSetMagicColors( hdc, magic, index );
}

INT WINAPI NtGdiSetMetaRgn( HDC hdc )
{
    return unix_funcs->pNtGdiSetMetaRgn( hdc );
}

COLORREF WINAPI NtGdiSetPixel( HDC hdc, INT x, INT y, COLORREF color )
{
    return unix_funcs->pNtGdiSetPixel( hdc, x, y, color );
}

BOOL WINAPI NtGdiSetPixelFormat( HDC hdc, INT format )
{
    return unix_funcs->pNtGdiSetPixelFormat( hdc, format );
}

BOOL WINAPI NtGdiSetRectRgn( HRGN hrgn, INT left, INT top, INT right, INT bottom )
{
    return unix_funcs->pNtGdiSetRectRgn( hrgn, left, top, right, bottom );
}

UINT WINAPI NtGdiSetSystemPaletteUse( HDC hdc, UINT use )
{
    return unix_funcs->pNtGdiSetSystemPaletteUse( hdc, use );
}

BOOL WINAPI NtGdiSetTextJustification( HDC hdc, INT extra, INT breaks )
{
    return unix_funcs->pNtGdiSetTextJustification( hdc, extra, breaks );
}

BOOL WINAPI NtGdiSetVirtualResolution( HDC hdc, DWORD horz_res, DWORD vert_res,
                                       DWORD horz_size, DWORD vert_size )
{
    return unix_funcs->pNtGdiSetVirtualResolution( hdc, horz_res, vert_res, horz_size, vert_size );
}

INT WINAPI NtGdiStartDoc( HDC hdc, const DOCINFOW *doc, BOOL *banding, INT job )
{
    return unix_funcs->pNtGdiStartDoc( hdc, doc, banding, job );
}

INT WINAPI NtGdiStartPage( HDC hdc )
{
    return unix_funcs->pNtGdiStartPage( hdc );
}

BOOL WINAPI NtGdiStretchBlt( HDC hdc, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                             HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                             DWORD rop, COLORREF bk_color )
{
    return unix_funcs->pNtGdiStretchBlt( hdc, x_dst, y_dst, width_dst, height_dst, hdc_src,
                                         x_src, y_src, width_src, height_src, rop, bk_color );
}

INT WINAPI NtGdiStretchDIBitsInternal( HDC hdc, INT x_dst, INT y_dst, INT width_dst,
                                       INT height_dst, INT x_src, INT y_src, INT width_src,
                                       INT height_src, const void *bits, const BITMAPINFO *bmi,
                                       UINT coloruse, DWORD rop, UINT max_info, UINT max_bits,
                                       HANDLE xform )
{
    return unix_funcs->pNtGdiStretchDIBitsInternal( hdc, x_dst, y_dst, width_dst, height_dst,
                                                    x_src, y_src, width_src, height_src, bits, bmi,
                                                    coloruse, rop, max_info, max_bits, xform );
}

BOOL WINAPI NtGdiStrokeAndFillPath( HDC hdc )
{
    return unix_funcs->pNtGdiStrokeAndFillPath( hdc );
}

BOOL WINAPI NtGdiStrokePath( HDC hdc )
{
    return unix_funcs->pNtGdiStrokePath( hdc );
}

BOOL WINAPI NtGdiSwapBuffers( HDC hdc )
{
    return unix_funcs->pNtGdiSwapBuffers( hdc );
}

BOOL WINAPI NtGdiTransparentBlt( HDC hdc, int x_dst, int y_dst, int width_dst, int height_dst,
                                 HDC hdc_src, int x_src, int y_src, int width_src, int height_src,
                                 UINT color )
{
    return unix_funcs->pNtGdiTransparentBlt( hdc, x_dst, y_dst, width_dst, height_dst, hdc_src,
                                             x_src, y_src, width_src, height_src, color );
}

BOOL WINAPI NtGdiTransformPoints( HDC hdc, const POINT *points_in, POINT *points_out,
                                  INT count, UINT mode )
{
    return unix_funcs->pNtGdiTransformPoints( hdc, points_in, points_out, count, mode );
}

BOOL WINAPI NtGdiUnrealizeObject( HGDIOBJ obj )
{
    return unix_funcs->pNtGdiUnrealizeObject( obj );
}

BOOL WINAPI NtGdiUpdateColors( HDC hdc )
{
    return unix_funcs->pNtGdiUpdateColors( hdc );
}

BOOL WINAPI NtGdiWidenPath( HDC hdc )
{
    return unix_funcs->pNtGdiWidenPath( hdc );
}

NTSTATUS WINAPI NtGdiDdDDICheckVidPnExclusiveOwnership( const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *desc )
{
    return unix_funcs->pNtGdiDdDDICheckVidPnExclusiveOwnership( desc );
}

NTSTATUS WINAPI NtGdiDdDDICloseAdapter( const D3DKMT_CLOSEADAPTER *desc )
{
    return unix_funcs->pNtGdiDdDDICloseAdapter( desc );
}

NTSTATUS WINAPI NtGdiDdDDICreateDCFromMemory( D3DKMT_CREATEDCFROMMEMORY *desc )
{
    return unix_funcs->pNtGdiDdDDICreateDCFromMemory( desc );
}

NTSTATUS WINAPI NtGdiDdDDICreateDevice( D3DKMT_CREATEDEVICE *desc )
{
    return unix_funcs->pNtGdiDdDDICreateDevice( desc );
}

NTSTATUS WINAPI NtGdiDdDDIDestroyDCFromMemory( const D3DKMT_DESTROYDCFROMMEMORY *desc )
{
    return unix_funcs->pNtGdiDdDDIDestroyDCFromMemory( desc );
}

NTSTATUS WINAPI NtGdiDdDDIDestroyDevice( const D3DKMT_DESTROYDEVICE *desc )
{
    return unix_funcs->pNtGdiDdDDIDestroyDevice( desc );
}

NTSTATUS WINAPI NtGdiDdDDIEscape( const D3DKMT_ESCAPE *desc )
{
    return unix_funcs->pNtGdiDdDDIEscape( desc );
}

NTSTATUS WINAPI NtGdiDdDDIOpenAdapterFromDeviceName( D3DKMT_OPENADAPTERFROMDEVICENAME *desc )
{
    return unix_funcs->pNtGdiDdDDIOpenAdapterFromDeviceName( desc );
}

NTSTATUS WINAPI NtGdiDdDDIOpenAdapterFromHdc( D3DKMT_OPENADAPTERFROMHDC *desc )
{
    return unix_funcs->pNtGdiDdDDIOpenAdapterFromHdc( desc );
}

NTSTATUS WINAPI NtGdiDdDDIOpenAdapterFromLuid( D3DKMT_OPENADAPTERFROMLUID *desc )
{
    return unix_funcs->pNtGdiDdDDIOpenAdapterFromLuid( desc );
}

NTSTATUS WINAPI NtGdiDdDDIQueryStatistics( D3DKMT_QUERYSTATISTICS *stats )
{
    return unix_funcs->pNtGdiDdDDIQueryStatistics( stats );
}

NTSTATUS WINAPI NtGdiDdDDISetQueuedLimit( D3DKMT_SETQUEUEDLIMIT *desc )
{
    return unix_funcs->pNtGdiDdDDISetQueuedLimit( desc );
}

NTSTATUS WINAPI NtGdiDdDDISetVidPnSourceOwner( const D3DKMT_SETVIDPNSOURCEOWNER *desc )
{
    return unix_funcs->pNtGdiDdDDISetVidPnSourceOwner( desc );
}

UINT WINAPI GDIRealizePalette( HDC hdc )
{
    return unix_funcs->pGDIRealizePalette( hdc );
}

HPALETTE WINAPI GDISelectPalette( HDC hdc, HPALETTE hpal, WORD bkg )
{
    return unix_funcs->pGDISelectPalette( hdc, hpal, bkg );
}

DWORD_PTR WINAPI GetDCHook( HDC hdc, DCHOOKPROC *proc )
{
    return unix_funcs->pGetDCHook( hdc, proc );
}

BOOL WINAPI SetDCHook( HDC hdc, DCHOOKPROC proc, DWORD_PTR data )
{
    return unix_funcs->pSetDCHook( hdc, proc, data );
}

BOOL WINAPI MirrorRgn( HWND hwnd, HRGN hrgn )
{
    return unix_funcs->pMirrorRgn( hwnd, hrgn );
}

INT WINAPI SetDIBits( HDC hdc, HBITMAP hbitmap, UINT startscan,
                      UINT lines, const void *bits, const BITMAPINFO *info,
                      UINT coloruse )
{
    return unix_funcs->pSetDIBits( hdc, hbitmap, startscan, lines, bits, info, coloruse );
}

WORD WINAPI SetHookFlags( HDC hdc, WORD flags )
{
    return unix_funcs->pSetHookFlags( hdc, flags );
}

BOOL CDECL __wine_get_icm_profile( HDC hdc, BOOL allow_default, DWORD *size, WCHAR *filename )
{
    return unix_funcs->get_icm_profile( hdc, allow_default, size, filename );
}

void CDECL __wine_make_gdi_object_system( HGDIOBJ handle, BOOL set )
{
    return unix_funcs->make_gdi_object_system( handle, set );
}

void CDECL __wine_set_visible_region( HDC hdc, HRGN hrgn, const RECT *vis_rect, const RECT *device_rect,
                                      struct window_surface *surface )
{
    return unix_funcs->set_visible_region( hdc, hrgn, vis_rect, device_rect, surface );
}

BOOL CDECL __wine_get_brush_bitmap_info( HBRUSH handle, BITMAPINFO *info, void *bits, UINT *usage )
{
    return unix_funcs->get_brush_bitmap_info( handle, info, bits, usage );
}

BOOL CDECL __wine_get_file_outline_text_metric( const WCHAR *path, OUTLINETEXTMETRICW *otm )
{
    return unix_funcs->get_file_outline_text_metric( path, otm );
}

const struct vulkan_funcs * CDECL __wine_get_vulkan_driver(HDC hdc, UINT version)
{
    return unix_funcs->get_vulkan_driver( hdc, version );
}

struct opengl_funcs * CDECL __wine_get_wgl_driver( HDC hdc, UINT version )
{
    return unix_funcs->get_wgl_driver( hdc, version );
}

/***********************************************************************
 *           __wine_set_display_driver    (win32u.@)
 */
void CDECL __wine_set_display_driver( HMODULE module )
{
    void *wine_get_gdi_driver;
    ANSI_STRING name_str;

    if (!module) return;

    RtlInitAnsiString( &name_str, "wine_get_gdi_driver" );
    LdrGetProcedureAddress( module, &name_str, 0, &wine_get_gdi_driver );
    if (!wine_get_gdi_driver)
    {
        ERR( "Could not create graphics driver %p\n", module );
        return;
    }
    unix_funcs->set_display_driver( wine_get_gdi_driver );
}

static void *get_user_proc( const char *name, BOOL force_load )
{
    ANSI_STRING name_str;
    FARPROC proc = NULL;
    static HMODULE user32;

    if (!user32)
    {
        UNICODE_STRING str;
        NTSTATUS status;
        RtlInitUnicodeString( &str, L"user32.dll" );
        status = force_load
            ? LdrLoadDll( NULL, 0, &str, &user32 )
            : LdrGetDllHandle( NULL, 0, &str, &user32 );
        if (status < 0) return NULL;
    }
    RtlInitAnsiString( &name_str, name );
    LdrGetProcedureAddress( user32, &name_str, 0, (void**)&proc );
    return proc;
}

static HWND WINAPI call_GetDesktopWindow(void)
{
    static HWND (WINAPI *pGetDesktopWindow)(void);
    if (!pGetDesktopWindow)
        pGetDesktopWindow = get_user_proc( "GetDesktopWindow", TRUE );
    return pGetDesktopWindow ? pGetDesktopWindow() : NULL;
}

static UINT WINAPI call_GetDpiForSystem(void)
{
    static UINT (WINAPI *pGetDpiForSystem)(void);
    if (!pGetDpiForSystem)
        pGetDpiForSystem = get_user_proc( "GetDpiForSystem", FALSE );
    return pGetDpiForSystem ? pGetDpiForSystem() : 96;
}

static BOOL WINAPI call_GetMonitorInfoW( HMONITOR monitor, MONITORINFO *info )
{
    static BOOL (WINAPI *pGetMonitorInfoW)( HMONITOR, LPMONITORINFO );
    if (!pGetMonitorInfoW)
        pGetMonitorInfoW = get_user_proc( "GetMonitorInfoW", FALSE );
    return pGetMonitorInfoW && pGetMonitorInfoW( monitor, info );
}

static INT WINAPI call_GetSystemMetrics( INT metric )
{
    static INT (WINAPI *pGetSystemMetrics)(INT);
    if (!pGetSystemMetrics)
        pGetSystemMetrics = get_user_proc( "GetSystemMetrics", FALSE );
    return pGetSystemMetrics ? pGetSystemMetrics( metric ) : 0;
}

static BOOL WINAPI call_GetWindowRect( HWND hwnd, LPRECT rect )
{
    static BOOL (WINAPI *pGetWindowRect)( HWND hwnd, LPRECT rect );
    if (!pGetWindowRect)
        pGetWindowRect = get_user_proc( "GetWindowRect", FALSE );
    return pGetWindowRect && pGetWindowRect( hwnd, rect );
}

static BOOL WINAPI call_EnumDisplayMonitors( HDC hdc, RECT *rect, MONITORENUMPROC proc,
                                             LPARAM lparam )
{
    static BOOL (WINAPI *pEnumDisplayMonitors)( HDC, LPRECT, MONITORENUMPROC, LPARAM );
    if (!pEnumDisplayMonitors)
        pEnumDisplayMonitors = get_user_proc( "EnumDisplayMonitors", FALSE );
    return pEnumDisplayMonitors && pEnumDisplayMonitors( hdc, rect, proc, lparam );
}

static BOOL WINAPI call_EnumDisplaySettingsW( const WCHAR *device, DWORD mode, DEVMODEW *devmode )
{
    static BOOL (WINAPI *pEnumDisplaySettingsW)(LPCWSTR, DWORD, LPDEVMODEW );
    if (!pEnumDisplaySettingsW)
        pEnumDisplaySettingsW = get_user_proc( "EnumDisplaySettingsW", FALSE );
    return pEnumDisplaySettingsW && pEnumDisplaySettingsW( device, mode, devmode );
}

static BOOL WINAPI call_RedrawWindow( HWND hwnd, const RECT *rect, HRGN rgn, UINT flags )
{
    static BOOL (WINAPI *pRedrawWindow)( HWND, const RECT*, HRGN, UINT );
    if (!pRedrawWindow)
        pRedrawWindow = get_user_proc( "RedrawWindow", FALSE );
    return pRedrawWindow && pRedrawWindow( hwnd, rect, rgn, flags );
}

static DPI_AWARENESS_CONTEXT WINAPI call_SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT ctx )
{
    static DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)( DPI_AWARENESS_CONTEXT );
    if (!pSetThreadDpiAwarenessContext)
        pSetThreadDpiAwarenessContext = get_user_proc( "SetThreadDpiAwarenessContext", FALSE );
    return pSetThreadDpiAwarenessContext ? pSetThreadDpiAwarenessContext( ctx ) : 0;
}

static HWND WINAPI call_WindowFromDC( HDC hdc )
{
    static HWND (WINAPI *pWindowFromDC)( HDC );
    if (!pWindowFromDC)
        pWindowFromDC = get_user_proc( "WindowFromDC", FALSE );
    return pWindowFromDC ? pWindowFromDC( hdc ) : NULL;
}

static const struct user_callbacks user_callbacks =
{
    call_GetDesktopWindow,
    call_GetDpiForSystem,
    call_GetMonitorInfoW,
    call_GetSystemMetrics,
    call_GetWindowRect,
    call_EnumDisplayMonitors,
    call_EnumDisplaySettingsW,
    call_RedrawWindow,
    call_SetThreadDpiAwarenessContext,
    call_WindowFromDC,
};

extern void wrappers_init( unixlib_handle_t handle )
{
    const void *args = &user_callbacks;
    if (!__wine_unix_call( handle, 1, &args )) unix_funcs = args;
}
