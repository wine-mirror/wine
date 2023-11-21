/*
 * Win32u kernel interface
 *
 * Copyright 2021 Alexandre Julliard
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
#include "ntuser.h"
#include "wine/unixlib.h"
#include "wine/asm.h"
#include "win32syscalls.h"

void *__wine_syscall_dispatcher = NULL;

static unixlib_handle_t win32u_handle;

/*******************************************************************
 *         syscalls
 */
#ifdef __arm64ec__
enum syscall_ids
{
#define SYSCALL_ENTRY(id,name,args) __id_##name = id + 0x1000,
ALL_SYSCALLS64
#undef SYSCALL_ENTRY
};

#define SYSCALL_API __attribute__((naked))

INT SYSCALL_API NtGdiAbortDoc( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiAbortDoc );
}

BOOL SYSCALL_API NtGdiAbortPath( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiAbortPath );
}

HANDLE SYSCALL_API NtGdiAddFontMemResourceEx( void *ptr, DWORD size, void *dv, ULONG dv_size,
                                              DWORD *count )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiAddFontMemResourceEx );
}

INT SYSCALL_API NtGdiAddFontResourceW( const WCHAR *str, ULONG size, ULONG files, DWORD flags,
                                       DWORD tid, void *dv )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiAddFontResourceW );
}

BOOL SYSCALL_API NtGdiAlphaBlend( HDC hdcDst, int xDst, int yDst, int widthDst, int heightDst,
                                  HDC hdcSrc, int xSrc, int ySrc, int widthSrc, int heightSrc,
                                  DWORD blend_func, HANDLE xform )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiAlphaBlend );
}

BOOL SYSCALL_API NtGdiAngleArc( HDC hdc, INT x, INT y, DWORD dwRadius, DWORD start_angle, DWORD sweep_angle )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiAngleArc );
}

BOOL SYSCALL_API NtGdiArcInternal( UINT type, HDC hdc, INT left, INT top, INT right,
                                   INT bottom, INT xstart, INT ystart, INT xend, INT yend )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiArcInternal );
}

BOOL SYSCALL_API NtGdiBeginPath( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiBeginPath );
}

BOOL SYSCALL_API NtGdiBitBlt( HDC hdc_dst, INT x_dst, INT y_dst, INT width, INT height,
                              HDC hdc_src, INT x_src, INT y_src, DWORD rop, DWORD bk_color, FLONG fl )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiBitBlt );
}

BOOL SYSCALL_API NtGdiCloseFigure( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCloseFigure );
}

INT SYSCALL_API NtGdiCombineRgn( HRGN hDest, HRGN hSrc1, HRGN hSrc2, INT mode )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCombineRgn );
}

BOOL SYSCALL_API NtGdiComputeXformCoefficients( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiComputeXformCoefficients );
}

HBITMAP SYSCALL_API NtGdiCreateBitmap( INT width, INT height, UINT planes,
                                       UINT bpp, const void *bits )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreateBitmap );
}

HANDLE SYSCALL_API NtGdiCreateClientObj( ULONG type )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreateClientObj );
}

HBITMAP SYSCALL_API NtGdiCreateCompatibleBitmap( HDC hdc, INT width, INT height )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreateCompatibleBitmap );
}

HDC SYSCALL_API NtGdiCreateCompatibleDC( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreateCompatibleDC );
}

HBRUSH SYSCALL_API NtGdiCreateDIBBrush( const void *data, UINT coloruse, UINT size,
                                        BOOL is_8x8, BOOL pen, const void *client )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreateDIBBrush );
}

HBITMAP SYSCALL_API NtGdiCreateDIBSection( HDC hdc, HANDLE section, DWORD offset, const BITMAPINFO *bmi,
                                           UINT usage, UINT header_size, ULONG flags,
                                           ULONG_PTR color_space, void **bits )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreateDIBSection );
}

HBITMAP SYSCALL_API NtGdiCreateDIBitmapInternal( HDC hdc, INT width, INT height, DWORD init,
                                                 const void *bits, const BITMAPINFO *data,
                                                 UINT coloruse, UINT max_info, UINT max_bits,
                                                 ULONG flags, HANDLE xform )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreateDIBitmapInternal );
}

HRGN SYSCALL_API NtGdiCreateEllipticRgn( INT left, INT top, INT right, INT bottom )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreateEllipticRgn );
}

HPALETTE SYSCALL_API NtGdiCreateHalftonePalette( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreateHalftonePalette );
}

HBRUSH SYSCALL_API NtGdiCreateHatchBrushInternal( INT style, COLORREF color, BOOL pen )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreateHatchBrushInternal );
}

HDC SYSCALL_API NtGdiCreateMetafileDC( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreateMetafileDC );
}

HPALETTE SYSCALL_API NtGdiCreatePaletteInternal( const LOGPALETTE *palette, UINT count )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreatePaletteInternal );
}

HBRUSH SYSCALL_API NtGdiCreatePatternBrushInternal( HBITMAP bitmap, BOOL pen, BOOL is_8x8 )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreatePatternBrushInternal );
}

HPEN SYSCALL_API NtGdiCreatePen( INT style, INT width, COLORREF color, HBRUSH brush )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreatePen );
}

HRGN SYSCALL_API NtGdiCreateRectRgn( INT left, INT top, INT right, INT bottom )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreateRectRgn );
}

HRGN SYSCALL_API NtGdiCreateRoundRectRgn( INT left, INT top, INT right, INT bottom,
                                          INT ellipse_width, INT ellipse_height )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreateRoundRectRgn );
}

HBRUSH SYSCALL_API NtGdiCreateSolidBrush( COLORREF color, HBRUSH brush )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiCreateSolidBrush );
}

NTSTATUS SYSCALL_API NtGdiDdDDICheckVidPnExclusiveOwnership( const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *desc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDdDDICheckVidPnExclusiveOwnership );
}

NTSTATUS SYSCALL_API NtGdiDdDDICloseAdapter( const D3DKMT_CLOSEADAPTER *desc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDdDDICloseAdapter );
}

NTSTATUS SYSCALL_API NtGdiDdDDICreateDCFromMemory( D3DKMT_CREATEDCFROMMEMORY *desc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDdDDICreateDCFromMemory );
}

NTSTATUS SYSCALL_API NtGdiDdDDICreateDevice( D3DKMT_CREATEDEVICE *desc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDdDDICreateDevice );
}

NTSTATUS SYSCALL_API NtGdiDdDDIDestroyDCFromMemory( const D3DKMT_DESTROYDCFROMMEMORY *desc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDdDDIDestroyDCFromMemory );
}

NTSTATUS SYSCALL_API NtGdiDdDDIDestroyDevice( const D3DKMT_DESTROYDEVICE *desc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDdDDIDestroyDevice );
}

NTSTATUS SYSCALL_API NtGdiDdDDIEscape( const D3DKMT_ESCAPE *desc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDdDDIEscape );
}

NTSTATUS SYSCALL_API NtGdiDdDDIOpenAdapterFromDeviceName( D3DKMT_OPENADAPTERFROMDEVICENAME *desc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDdDDIOpenAdapterFromDeviceName );
}

NTSTATUS SYSCALL_API NtGdiDdDDIOpenAdapterFromHdc( D3DKMT_OPENADAPTERFROMHDC *desc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDdDDIOpenAdapterFromHdc );
}

NTSTATUS SYSCALL_API NtGdiDdDDIOpenAdapterFromLuid( D3DKMT_OPENADAPTERFROMLUID *desc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDdDDIOpenAdapterFromLuid );
}

NTSTATUS SYSCALL_API NtGdiDdDDIQueryAdapterInfo( D3DKMT_QUERYADAPTERINFO *desc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDdDDIQueryAdapterInfo );
}

NTSTATUS SYSCALL_API NtGdiDdDDIQueryStatistics( D3DKMT_QUERYSTATISTICS *stats )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDdDDIQueryStatistics );
}

NTSTATUS SYSCALL_API NtGdiDdDDIQueryVideoMemoryInfo( D3DKMT_QUERYVIDEOMEMORYINFO *desc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDdDDIQueryVideoMemoryInfo );
}

NTSTATUS SYSCALL_API NtGdiDdDDISetQueuedLimit( D3DKMT_SETQUEUEDLIMIT *desc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDdDDISetQueuedLimit );
}

NTSTATUS SYSCALL_API NtGdiDdDDISetVidPnSourceOwner( const D3DKMT_SETVIDPNSOURCEOWNER *desc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDdDDISetVidPnSourceOwner );
}

BOOL SYSCALL_API NtGdiDeleteClientObj( HGDIOBJ handle )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDeleteClientObj );
}

BOOL SYSCALL_API NtGdiDeleteObjectApp( HGDIOBJ obj )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDeleteObjectApp );
}

INT SYSCALL_API NtGdiDescribePixelFormat( HDC hdc, INT format, UINT size, PIXELFORMATDESCRIPTOR *descr )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDescribePixelFormat );
}

LONG SYSCALL_API NtGdiDoPalette( HGDIOBJ handle, WORD start, WORD count, void *entries,
                                 DWORD func, BOOL inbound )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDoPalette );
}

BOOL SYSCALL_API NtGdiDrawStream( HDC hdc, ULONG in, void *pvin )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiDrawStream );
}

BOOL SYSCALL_API NtGdiEllipse( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiEllipse );
}

INT SYSCALL_API NtGdiEndDoc( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiEndDoc );
}

INT SYSCALL_API NtGdiEndPage( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiEndPage );
}

BOOL SYSCALL_API NtGdiEndPath( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiEndPath );
}

BOOL SYSCALL_API NtGdiEnumFonts( HDC hdc, ULONG type, ULONG win32_compat, ULONG face_name_len,
                                 const WCHAR *face_name, ULONG charset, ULONG *count, void *buf )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiEnumFonts );
}

BOOL SYSCALL_API NtGdiEqualRgn( HRGN hrgn1, HRGN hrgn2 )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiEqualRgn );
}

INT SYSCALL_API NtGdiExcludeClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiExcludeClipRect );
}

HPEN SYSCALL_API NtGdiExtCreatePen( DWORD style, DWORD width, ULONG brush_style, ULONG color,
                                    ULONG_PTR client_hatch, ULONG_PTR hatch, DWORD style_count,
                                    const DWORD *style_bits, ULONG dib_size, BOOL old_style,
                                    HBRUSH brush )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiExtCreatePen );
}

HRGN SYSCALL_API NtGdiExtCreateRegion( const XFORM *xform, DWORD count, const RGNDATA *rgndata )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiExtCreateRegion );
}

INT SYSCALL_API NtGdiExtEscape( HDC hdc, WCHAR *driver, int driver_id, INT escape, INT input_size,
                                const char *input, INT output_size, char *output )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiExtEscape );
}

BOOL SYSCALL_API NtGdiExtFloodFill( HDC hdc, INT x, INT y, COLORREF color, UINT fill_type )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiExtFloodFill );
}

INT SYSCALL_API NtGdiExtGetObjectW( HGDIOBJ handle, INT count, void *buffer )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiExtGetObjectW );
}

INT SYSCALL_API NtGdiExtSelectClipRgn( HDC hdc, HRGN rgn, INT mode )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiExtSelectClipRgn );
}

BOOL SYSCALL_API NtGdiExtTextOutW( HDC hdc, INT x, INT y, UINT flags, const RECT *lprect,
                                   const WCHAR *str, UINT count, const INT *lpDx, DWORD cp )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiExtTextOutW );
}

BOOL SYSCALL_API NtGdiFillPath( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiFillPath );
}

BOOL SYSCALL_API NtGdiFillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiFillRgn );
}

BOOL SYSCALL_API NtGdiFlattenPath( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiFlattenPath );
}

BOOL SYSCALL_API NtGdiFlush(void)
{
    __ASM_SYSCALL_FUNC( __id_NtGdiFlush );
}

BOOL SYSCALL_API NtGdiFontIsLinked( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiFontIsLinked );
}

BOOL SYSCALL_API NtGdiFrameRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush, INT width, INT height )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiFrameRgn );
}

BOOL SYSCALL_API NtGdiGetAndSetDCDword( HDC hdc, UINT method, DWORD value, DWORD *prev_value )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetAndSetDCDword );
}

INT SYSCALL_API NtGdiGetAppClipBox( HDC hdc, RECT *rect )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetAppClipBox );
}

LONG SYSCALL_API NtGdiGetBitmapBits( HBITMAP hbitmap, LONG count, LPVOID bits )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetBitmapBits );
}

BOOL SYSCALL_API NtGdiGetBitmapDimension( HBITMAP hbitmap, LPSIZE size )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetBitmapDimension );
}

UINT SYSCALL_API NtGdiGetBoundsRect( HDC hdc, RECT *rect, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetBoundsRect );
}

BOOL SYSCALL_API NtGdiGetCharABCWidthsW( HDC hdc, UINT first, UINT last, WCHAR *chars,
                                         ULONG flags, void *buffer )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetCharABCWidthsW );
}

BOOL SYSCALL_API NtGdiGetCharWidthInfo( HDC hdc, struct char_width_info *info )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetCharWidthInfo );
}

BOOL SYSCALL_API NtGdiGetCharWidthW( HDC hdc, UINT first, UINT last, WCHAR *chars,
                                     ULONG flags, void *buf )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetCharWidthW );
}

BOOL SYSCALL_API NtGdiGetColorAdjustment( HDC hdc, COLORADJUSTMENT *ca )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetColorAdjustment );
}

BOOL SYSCALL_API NtGdiGetDCDword( HDC hdc, UINT method, DWORD *result )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetDCDword );
}

HANDLE SYSCALL_API NtGdiGetDCObject( HDC hdc, UINT type )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetDCObject );
}

BOOL SYSCALL_API NtGdiGetDCPoint( HDC hdc, UINT method, POINT *result )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetDCPoint );
}

INT SYSCALL_API NtGdiGetDIBitsInternal( HDC hdc, HBITMAP hbitmap, UINT startscan, UINT lines,
                                        void *bits, BITMAPINFO *info, UINT coloruse,
                                        UINT max_bits, UINT max_info )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetDIBitsInternal );
}

INT SYSCALL_API NtGdiGetDeviceCaps( HDC hdc, INT cap )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetDeviceCaps );
}

BOOL SYSCALL_API NtGdiGetDeviceGammaRamp( HDC hdc, void *ptr )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetDeviceGammaRamp );
}

DWORD SYSCALL_API NtGdiGetFontData( HDC hdc, DWORD table, DWORD offset, void *buffer, DWORD length )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetFontData );
}

BOOL SYSCALL_API NtGdiGetFontFileData( DWORD instance_id, DWORD file_index, UINT64 *offset,
                                       void *buff, SIZE_T buff_size )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetFontFileData );
}

BOOL SYSCALL_API NtGdiGetFontFileInfo( DWORD instance_id, DWORD file_index, struct font_fileinfo *info,
                                       SIZE_T size, SIZE_T *needed )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetFontFileInfo );
}

DWORD SYSCALL_API NtGdiGetFontUnicodeRanges( HDC hdc, GLYPHSET *lpgs )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetFontUnicodeRanges );
}

DWORD SYSCALL_API NtGdiGetGlyphIndicesW( HDC hdc, const WCHAR *str, INT count,
                                         WORD *indices, DWORD flags )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetGlyphIndicesW );
}

DWORD SYSCALL_API NtGdiGetGlyphOutline( HDC hdc, UINT ch, UINT format, GLYPHMETRICS *metrics,
                                        DWORD size, void *buffer, const MAT2 *mat2,
                                        BOOL ignore_rotation )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetGlyphOutline );
}

DWORD SYSCALL_API NtGdiGetKerningPairs( HDC hdc, DWORD count, KERNINGPAIR *kern_pair )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetKerningPairs );
}

COLORREF SYSCALL_API NtGdiGetNearestColor( HDC hdc, COLORREF color )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetNearestColor );
}

UINT SYSCALL_API NtGdiGetNearestPaletteIndex( HPALETTE hpalette, COLORREF color )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetNearestPaletteIndex );
}

UINT SYSCALL_API NtGdiGetOutlineTextMetricsInternalW( HDC hdc, UINT cbData,
                                                      OUTLINETEXTMETRICW *lpOTM, ULONG opts )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetOutlineTextMetricsInternalW );
}

INT SYSCALL_API NtGdiGetPath( HDC hdc, POINT *points, BYTE *types, INT size )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetPath );
}

COLORREF SYSCALL_API NtGdiGetPixel( HDC hdc, INT x, INT y )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetPixel );
}

INT SYSCALL_API NtGdiGetRandomRgn( HDC hDC, HRGN hRgn, INT iCode )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetRandomRgn );
}

BOOL SYSCALL_API NtGdiGetRasterizerCaps( RASTERIZER_STATUS *status, UINT size )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetRasterizerCaps );
}

BOOL SYSCALL_API NtGdiGetRealizationInfo( HDC hdc, struct font_realization_info *info )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetRealizationInfo );
}

DWORD SYSCALL_API NtGdiGetRegionData( HRGN hrgn, DWORD count, RGNDATA *rgndata )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetRegionData );
}

INT SYSCALL_API NtGdiGetRgnBox( HRGN hrgn, RECT *rect )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetRgnBox );
}

DWORD SYSCALL_API NtGdiGetSpoolMessage( void *ptr1, DWORD data2, void *ptr3, DWORD data4 )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetSpoolMessage );
}

UINT SYSCALL_API NtGdiGetSystemPaletteUse( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetSystemPaletteUse );
}

UINT SYSCALL_API NtGdiGetTextCharsetInfo( HDC hdc, FONTSIGNATURE *fs, DWORD flags )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetTextCharsetInfo );
}

BOOL SYSCALL_API NtGdiGetTextExtentExW( HDC hdc, const WCHAR *str, INT count, INT max_ext,
                                        INT *nfit, INT *dxs, SIZE *size, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetTextExtentExW );
}

INT SYSCALL_API NtGdiGetTextFaceW( HDC hdc, INT count, WCHAR *name, BOOL alias_name )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetTextFaceW );
}

BOOL SYSCALL_API NtGdiGetTextMetricsW( HDC hdc, TEXTMETRICW *metrics, ULONG flags )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetTextMetricsW );
}

BOOL SYSCALL_API NtGdiGetTransform( HDC hdc, DWORD which, XFORM *xform )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGetTransform );
}

BOOL SYSCALL_API NtGdiGradientFill( HDC hdc, TRIVERTEX *vert_array, ULONG nvert,
                                    void *grad_array, ULONG ngrad, ULONG mode )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiGradientFill );
}

HFONT SYSCALL_API NtGdiHfontCreate( const void *logfont, ULONG size, ULONG type,
                                    ULONG flags, void *data )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiHfontCreate );
}

BOOL SYSCALL_API NtGdiIcmBrushInfo( HDC hdc, HBRUSH handle, BITMAPINFO *info, void *bits,
                                    ULONG *bits_size, UINT *usage, BOOL *unk, UINT mode )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiIcmBrushInfo );
}

DWORD SYSCALL_API NtGdiInitSpool(void)
{
    __ASM_SYSCALL_FUNC( __id_NtGdiInitSpool );
}

INT SYSCALL_API NtGdiIntersectClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiIntersectClipRect );
}

BOOL SYSCALL_API NtGdiInvertRgn( HDC hdc, HRGN hrgn )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiInvertRgn );
}

BOOL SYSCALL_API NtGdiLineTo( HDC hdc, INT x, INT y )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiLineTo );
}

BOOL SYSCALL_API NtGdiMaskBlt( HDC hdcDest, INT nXDest, INT nYDest, INT nWidth, INT nHeight,
                               HDC hdcSrc, INT nXSrc, INT nYSrc, HBITMAP hbmMask,
                               INT xMask, INT yMask, DWORD dwRop, DWORD bk_color )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiMaskBlt );
}

BOOL SYSCALL_API NtGdiModifyWorldTransform( HDC hdc, const XFORM *xform, DWORD mode )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiModifyWorldTransform );
}

BOOL SYSCALL_API NtGdiMoveTo( HDC hdc, INT x, INT y, POINT *pt )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiMoveTo );
}

INT SYSCALL_API NtGdiOffsetClipRgn( HDC hdc, INT x, INT y )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiOffsetClipRgn );
}

INT SYSCALL_API NtGdiOffsetRgn( HRGN hrgn, INT x, INT y )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiOffsetRgn );
}

HDC SYSCALL_API NtGdiOpenDCW( UNICODE_STRING *device, const DEVMODEW *devmode, UNICODE_STRING *output,
                              ULONG type, BOOL is_display, HANDLE hspool, DRIVER_INFO_2W *driver_info,
                              void *pdev )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiOpenDCW );
}

BOOL SYSCALL_API NtGdiPatBlt( HDC hdc, INT left, INT top, INT width, INT height, DWORD rop )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiPatBlt );
}

HRGN SYSCALL_API NtGdiPathToRegion( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiPathToRegion );
}

BOOL SYSCALL_API NtGdiPlgBlt( HDC hdcDest, const POINT *lpPoint, HDC hdcSrc, INT nXSrc, INT nYSrc,
                              INT nWidth, INT nHeight, HBITMAP hbmMask, INT xMask, INT yMask,
                              DWORD bk_color )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiPlgBlt );
}

BOOL SYSCALL_API NtGdiPolyDraw( HDC hdc, const POINT *points, const BYTE *types, DWORD count )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiPolyDraw );
}

ULONG SYSCALL_API NtGdiPolyPolyDraw( HDC hdc, const POINT *points, const ULONG *counts,
                                     DWORD count, UINT function )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiPolyPolyDraw );
}

BOOL SYSCALL_API NtGdiPtInRegion( HRGN hrgn, INT x, INT y )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiPtInRegion );
}

BOOL SYSCALL_API NtGdiPtVisible( HDC hdc, INT x, INT y )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiPtVisible );
}

BOOL SYSCALL_API NtGdiRectInRegion( HRGN hrgn, const RECT *rect )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiRectInRegion );
}

BOOL SYSCALL_API NtGdiRectVisible( HDC hdc, const RECT *rect )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiRectVisible );
}

BOOL SYSCALL_API NtGdiRectangle( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiRectangle );
}

BOOL SYSCALL_API NtGdiRemoveFontMemResourceEx( HANDLE handle )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiRemoveFontMemResourceEx );
}

BOOL SYSCALL_API NtGdiRemoveFontResourceW( const WCHAR *str, ULONG size, ULONG files, DWORD flags,
                                           DWORD tid, void *dv )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiRemoveFontResourceW );
}

BOOL SYSCALL_API NtGdiResetDC( HDC hdc, const DEVMODEW *devmode, BOOL *banding,
                               DRIVER_INFO_2W *driver_info, void *dev )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiResetDC );
}

BOOL SYSCALL_API NtGdiResizePalette( HPALETTE hPal, UINT count )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiResizePalette );
}

BOOL SYSCALL_API NtGdiRestoreDC( HDC hdc, INT level )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiRestoreDC );
}

BOOL SYSCALL_API NtGdiRoundRect( HDC hdc, INT left, INT top, INT right,
                                 INT bottom, INT ell_width, INT ell_height )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiRoundRect );
}

INT SYSCALL_API NtGdiSaveDC( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSaveDC );
}

BOOL SYSCALL_API NtGdiScaleViewportExtEx( HDC hdc, INT x_num, INT x_denom,
                                          INT y_num, INT y_denom, SIZE *size )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiScaleViewportExtEx );
}

BOOL SYSCALL_API NtGdiScaleWindowExtEx( HDC hdc, INT x_num, INT x_denom,
                                        INT y_num, INT y_denom, SIZE *size )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiScaleWindowExtEx );
}

HGDIOBJ SYSCALL_API NtGdiSelectBitmap( HDC hdc, HGDIOBJ handle )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSelectBitmap );
}

HGDIOBJ SYSCALL_API NtGdiSelectBrush( HDC hdc, HGDIOBJ handle )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSelectBrush );
}

BOOL SYSCALL_API NtGdiSelectClipPath( HDC hdc, INT mode )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSelectClipPath );
}

HGDIOBJ SYSCALL_API NtGdiSelectFont( HDC hdc, HGDIOBJ handle )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSelectFont );
}

HGDIOBJ SYSCALL_API NtGdiSelectPen( HDC hdc, HGDIOBJ handle )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSelectPen );
}

LONG SYSCALL_API NtGdiSetBitmapBits( HBITMAP hbitmap, LONG count, LPCVOID bits )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSetBitmapBits );
}

BOOL SYSCALL_API NtGdiSetBitmapDimension( HBITMAP hbitmap, INT x, INT y, LPSIZE prevSize )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSetBitmapDimension );
}

UINT SYSCALL_API NtGdiSetBoundsRect( HDC hdc, const RECT *rect, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSetBoundsRect );
}

BOOL SYSCALL_API NtGdiSetBrushOrg( HDC hdc, INT x, INT y, POINT *oldorg )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSetBrushOrg );
}

BOOL SYSCALL_API NtGdiSetColorAdjustment( HDC hdc, const COLORADJUSTMENT *ca )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSetColorAdjustment );
}

INT SYSCALL_API NtGdiSetDIBitsToDeviceInternal( HDC hdc, INT xDest, INT yDest, DWORD cx,
                                                DWORD cy, INT xSrc, INT ySrc, UINT startscan,
                                                UINT lines, const void *bits, const BITMAPINFO *bmi,
                                                UINT coloruse, UINT max_bits, UINT max_info,
                                                BOOL xform_coords, HANDLE xform )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSetDIBitsToDeviceInternal );
}

BOOL SYSCALL_API NtGdiSetDeviceGammaRamp( HDC hdc, void *ptr )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSetDeviceGammaRamp );
}

DWORD SYSCALL_API NtGdiSetLayout( HDC hdc, LONG wox, DWORD layout )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSetLayout );
}

BOOL SYSCALL_API NtGdiSetMagicColors( HDC hdc, DWORD magic, ULONG index )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSetMagicColors );
}

INT SYSCALL_API NtGdiSetMetaRgn( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSetMetaRgn );
}

COLORREF SYSCALL_API NtGdiSetPixel( HDC hdc, INT x, INT y, COLORREF color )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSetPixel );
}

BOOL SYSCALL_API NtGdiSetPixelFormat( HDC hdc, INT format )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSetPixelFormat );
}

BOOL SYSCALL_API NtGdiSetRectRgn( HRGN hrgn, INT left, INT top, INT right, INT bottom )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSetRectRgn );
}

UINT SYSCALL_API NtGdiSetSystemPaletteUse( HDC hdc, UINT use )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSetSystemPaletteUse );
}

BOOL SYSCALL_API NtGdiSetTextJustification( HDC hdc, INT extra, INT breaks )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSetTextJustification );
}

BOOL SYSCALL_API NtGdiSetVirtualResolution( HDC hdc, DWORD horz_res, DWORD vert_res,
                                            DWORD horz_size, DWORD vert_size )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSetVirtualResolution );
}

INT SYSCALL_API NtGdiStartDoc( HDC hdc, const DOCINFOW *doc, BOOL *banding, INT job )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiStartDoc );
}

INT SYSCALL_API NtGdiStartPage( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiStartPage );
}

BOOL SYSCALL_API NtGdiStretchBlt( HDC hdcDst, INT xDst, INT yDst, INT widthDst, INT heightDst,
                                  HDC hdcSrc, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                                  DWORD rop, COLORREF bk_color )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiStretchBlt );
}

INT SYSCALL_API NtGdiStretchDIBitsInternal( HDC hdc, INT xDst, INT yDst, INT widthDst, INT heightDst,
                                            INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                                            const void *bits, const BITMAPINFO *bmi, UINT coloruse,
                                            DWORD rop, UINT max_info, UINT max_bits, HANDLE xform )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiStretchDIBitsInternal );
}

BOOL SYSCALL_API NtGdiStrokeAndFillPath( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiStrokeAndFillPath );
}

BOOL SYSCALL_API NtGdiStrokePath( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiStrokePath );
}

BOOL SYSCALL_API NtGdiSwapBuffers( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiSwapBuffers );
}

BOOL SYSCALL_API NtGdiTransformPoints( HDC hdc, const POINT *points_in, POINT *points_out,
                                       INT count, UINT mode )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiTransformPoints );
}

BOOL SYSCALL_API NtGdiTransparentBlt( HDC hdcDest, int xDest, int yDest, int widthDest, int heightDest,
                                      HDC hdcSrc, int xSrc, int ySrc, int widthSrc, int heightSrc,
                                      UINT crTransparent )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiTransparentBlt );
}

BOOL SYSCALL_API NtGdiUnrealizeObject( HGDIOBJ obj )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiUnrealizeObject );
}

BOOL SYSCALL_API NtGdiUpdateColors( HDC hDC )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiUpdateColors );
}

BOOL SYSCALL_API NtGdiWidenPath( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtGdiWidenPath );
}

HKL SYSCALL_API NtUserActivateKeyboardLayout( HKL layout, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserActivateKeyboardLayout );
}

BOOL SYSCALL_API NtUserAddClipboardFormatListener( HWND hwnd )
{
    __ASM_SYSCALL_FUNC( __id_NtUserAddClipboardFormatListener );
}

UINT SYSCALL_API NtUserAssociateInputContext( HWND hwnd, HIMC ctx, ULONG flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserAssociateInputContext );
}

BOOL SYSCALL_API NtUserAttachThreadInput( DWORD from, DWORD to, BOOL attach )
{
    __ASM_SYSCALL_FUNC( __id_NtUserAttachThreadInput );
}

HDC SYSCALL_API NtUserBeginPaint( HWND hwnd, PAINTSTRUCT *ps )
{
    __ASM_SYSCALL_FUNC( __id_NtUserBeginPaint );
}

NTSTATUS SYSCALL_API NtUserBuildHimcList( UINT thread_id, UINT count, HIMC *buffer, UINT *size )
{
    __ASM_SYSCALL_FUNC( __id_NtUserBuildHimcList );
}

NTSTATUS SYSCALL_API NtUserBuildHwndList( HDESK desktop, ULONG unk2, ULONG unk3, ULONG unk4,
                                          ULONG thread_id, ULONG count, HWND *buffer, ULONG *size )
{
    __ASM_SYSCALL_FUNC( __id_NtUserBuildHwndList );
}

ULONG_PTR SYSCALL_API NtUserCallHwnd( HWND hwnd, DWORD code )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCallHwnd );
}

ULONG_PTR SYSCALL_API NtUserCallHwndParam( HWND hwnd, DWORD_PTR param, DWORD code )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCallHwndParam );
}

BOOL SYSCALL_API NtUserCallMsgFilter( MSG *msg, INT code )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCallMsgFilter );
}

LRESULT SYSCALL_API NtUserCallNextHookEx( HHOOK hhook, INT code, WPARAM wparam, LPARAM lparam )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCallNextHookEx );
}

ULONG_PTR SYSCALL_API NtUserCallNoParam( ULONG code )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCallNoParam );
}

ULONG_PTR SYSCALL_API NtUserCallOneParam( ULONG_PTR arg, ULONG code )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCallOneParam );
}

ULONG_PTR SYSCALL_API NtUserCallTwoParam( ULONG_PTR arg1, ULONG_PTR arg2, ULONG code )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCallTwoParam );
}

BOOL SYSCALL_API NtUserChangeClipboardChain( HWND hwnd, HWND next )
{
    __ASM_SYSCALL_FUNC( __id_NtUserChangeClipboardChain );
}

LONG SYSCALL_API NtUserChangeDisplaySettings( UNICODE_STRING *devname, DEVMODEW *devmode, HWND hwnd,
                                              DWORD flags, void *lparam )
{
    __ASM_SYSCALL_FUNC( __id_NtUserChangeDisplaySettings );
}

DWORD SYSCALL_API NtUserCheckMenuItem( HMENU handle, UINT id, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCheckMenuItem );
}

HWND SYSCALL_API NtUserChildWindowFromPointEx( HWND parent, LONG x, LONG y, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserChildWindowFromPointEx );
}

BOOL SYSCALL_API NtUserClipCursor( const RECT *rect )
{
    __ASM_SYSCALL_FUNC( __id_NtUserClipCursor );
}

BOOL SYSCALL_API NtUserCloseClipboard(void)
{
    __ASM_SYSCALL_FUNC( __id_NtUserCloseClipboard );
}

BOOL SYSCALL_API NtUserCloseDesktop( HDESK handle )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCloseDesktop );
}

BOOL SYSCALL_API NtUserCloseWindowStation( HWINSTA handle )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCloseWindowStation );
}

INT SYSCALL_API NtUserCopyAcceleratorTable( HACCEL src, ACCEL *dst, INT count )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCopyAcceleratorTable );
}

INT SYSCALL_API NtUserCountClipboardFormats(void)
{
    __ASM_SYSCALL_FUNC( __id_NtUserCountClipboardFormats );
}

HACCEL SYSCALL_API NtUserCreateAcceleratorTable( ACCEL *table, INT count )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCreateAcceleratorTable );
}

BOOL SYSCALL_API NtUserCreateCaret( HWND hwnd, HBITMAP bitmap, int width, int height )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCreateCaret );
}

HDESK SYSCALL_API NtUserCreateDesktopEx( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *device,
                                         DEVMODEW *devmode, DWORD flags, ACCESS_MASK access,
                                         ULONG heap_size )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCreateDesktopEx );
}

HIMC SYSCALL_API NtUserCreateInputContext( UINT_PTR client_ptr )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCreateInputContext );
}

HWND SYSCALL_API NtUserCreateWindowEx( DWORD ex_style, UNICODE_STRING *class_name,
                                       UNICODE_STRING *version, UNICODE_STRING *window_name,
                                       DWORD style, INT x, INT y, INT cx, INT cy,
                                       HWND parent, HMENU menu, HINSTANCE instance, void *params,
                                       DWORD flags, HINSTANCE client_instance, DWORD unk, BOOL ansi )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCreateWindowEx );
}

HWINSTA SYSCALL_API NtUserCreateWindowStation( OBJECT_ATTRIBUTES *attr, ACCESS_MASK access, ULONG arg3,
                                               ULONG arg4, ULONG arg5, ULONG arg6, ULONG arg7 )
{
    __ASM_SYSCALL_FUNC( __id_NtUserCreateWindowStation );
}

HDWP SYSCALL_API NtUserDeferWindowPosAndBand( HDWP hdwp, HWND hwnd, HWND after,
                                              INT x, INT y, INT cx, INT cy,
                                              UINT flags, UINT unk1, UINT unk2 )
{
    __ASM_SYSCALL_FUNC( __id_NtUserDeferWindowPosAndBand );
}

BOOL SYSCALL_API NtUserDeleteMenu( HMENU handle, UINT id, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserDeleteMenu );
}

BOOL SYSCALL_API NtUserDestroyAcceleratorTable( HACCEL handle )
{
    __ASM_SYSCALL_FUNC( __id_NtUserDestroyAcceleratorTable );
}

BOOL SYSCALL_API NtUserDestroyCursor( HCURSOR cursor, ULONG arg )
{
    __ASM_SYSCALL_FUNC( __id_NtUserDestroyCursor );
}

BOOL SYSCALL_API NtUserDestroyInputContext( HIMC handle )
{
    __ASM_SYSCALL_FUNC( __id_NtUserDestroyInputContext );
}

BOOL SYSCALL_API NtUserDestroyMenu( HMENU handle )
{
    __ASM_SYSCALL_FUNC( __id_NtUserDestroyMenu );
}

BOOL SYSCALL_API NtUserDestroyWindow( HWND hwnd )
{
    __ASM_SYSCALL_FUNC( __id_NtUserDestroyWindow );
}

BOOL SYSCALL_API NtUserDisableThreadIme( DWORD thread_id )
{
    __ASM_SYSCALL_FUNC( __id_NtUserDisableThreadIme );
}

LRESULT SYSCALL_API NtUserDispatchMessage( const MSG *msg )
{
    __ASM_SYSCALL_FUNC( __id_NtUserDispatchMessage );
}

NTSTATUS SYSCALL_API NtUserDisplayConfigGetDeviceInfo( DISPLAYCONFIG_DEVICE_INFO_HEADER *packet )
{
    __ASM_SYSCALL_FUNC( __id_NtUserDisplayConfigGetDeviceInfo );
}

BOOL SYSCALL_API NtUserDragDetect( HWND hwnd, int x, int y )
{
    __ASM_SYSCALL_FUNC( __id_NtUserDragDetect );
}

DWORD SYSCALL_API NtUserDragObject( HWND parent, HWND hwnd, UINT fmt, ULONG_PTR data, HCURSOR cursor )
{
    __ASM_SYSCALL_FUNC( __id_NtUserDragObject );
}

BOOL SYSCALL_API NtUserDrawCaptionTemp( HWND hwnd, HDC hdc, const RECT *rect, HFONT font,
                                        HICON icon, const WCHAR *str, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserDrawCaptionTemp );
}

BOOL SYSCALL_API NtUserDrawIconEx( HDC hdc, INT x0, INT y0, HICON icon, INT width,
                                   INT height, UINT step, HBRUSH brush, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserDrawIconEx );
}

DWORD SYSCALL_API NtUserDrawMenuBarTemp( HWND hwnd, HDC hdc, RECT *rect, HMENU handle, HFONT font )
{
    __ASM_SYSCALL_FUNC( __id_NtUserDrawMenuBarTemp );
}

BOOL SYSCALL_API NtUserEmptyClipboard(void)
{
    __ASM_SYSCALL_FUNC( __id_NtUserEmptyClipboard );
}

BOOL SYSCALL_API NtUserEnableMenuItem( HMENU handle, UINT id, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserEnableMenuItem );
}

BOOL SYSCALL_API NtUserEnableMouseInPointer( BOOL enable )
{
    __ASM_SYSCALL_FUNC( __id_NtUserEnableMouseInPointer );
}

BOOL SYSCALL_API NtUserEnableScrollBar( HWND hwnd, UINT bar, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserEnableScrollBar );
}

BOOL SYSCALL_API NtUserEndDeferWindowPosEx( HDWP hdwp, BOOL async )
{
    __ASM_SYSCALL_FUNC( __id_NtUserEndDeferWindowPosEx );
}

BOOL SYSCALL_API NtUserEndMenu(void)
{
    __ASM_SYSCALL_FUNC( __id_NtUserEndMenu );
}

BOOL SYSCALL_API NtUserEndPaint( HWND hwnd, const PAINTSTRUCT *ps )
{
    __ASM_SYSCALL_FUNC( __id_NtUserEndPaint );
}

NTSTATUS SYSCALL_API NtUserEnumDisplayDevices( UNICODE_STRING *device, DWORD index,
                                               DISPLAY_DEVICEW *info, DWORD flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserEnumDisplayDevices );
}

BOOL SYSCALL_API NtUserEnumDisplayMonitors( HDC hdc, RECT *rect, MONITORENUMPROC proc, LPARAM lparam )
{
    __ASM_SYSCALL_FUNC( __id_NtUserEnumDisplayMonitors );
}

BOOL SYSCALL_API NtUserEnumDisplaySettings( UNICODE_STRING *device, DWORD index, DEVMODEW *devmode, DWORD flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserEnumDisplaySettings );
}

INT SYSCALL_API NtUserExcludeUpdateRgn( HDC hdc, HWND hwnd )
{
    __ASM_SYSCALL_FUNC( __id_NtUserExcludeUpdateRgn );
}

HICON SYSCALL_API NtUserFindExistingCursorIcon( UNICODE_STRING *module, UNICODE_STRING *res_name, void *desc )
{
    __ASM_SYSCALL_FUNC( __id_NtUserFindExistingCursorIcon );
}

HWND SYSCALL_API NtUserFindWindowEx( HWND parent, HWND child, UNICODE_STRING *class, UNICODE_STRING *title,
                                     ULONG unk )
{
    __ASM_SYSCALL_FUNC( __id_NtUserFindWindowEx );
}

BOOL SYSCALL_API NtUserFlashWindowEx( FLASHWINFO *info )
{
    __ASM_SYSCALL_FUNC( __id_NtUserFlashWindowEx );
}

HWND SYSCALL_API NtUserGetAncestor( HWND hwnd, UINT type )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetAncestor );
}

SHORT SYSCALL_API NtUserGetAsyncKeyState( INT key )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetAsyncKeyState );
}

ULONG SYSCALL_API NtUserGetAtomName( ATOM atom, UNICODE_STRING *name )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetAtomName );
}

UINT SYSCALL_API NtUserGetCaretBlinkTime(void)
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetCaretBlinkTime );
}

BOOL SYSCALL_API NtUserGetCaretPos( POINT *pt )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetCaretPos );
}

ATOM SYSCALL_API NtUserGetClassInfoEx( HINSTANCE instance, UNICODE_STRING *name, WNDCLASSEXW *wc,
                                       struct client_menu_name *menu_name, BOOL ansi )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetClassInfoEx );
}

INT SYSCALL_API NtUserGetClassName( HWND hwnd, BOOL real, UNICODE_STRING *name )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetClassName );
}

HANDLE SYSCALL_API NtUserGetClipboardData( UINT format, struct get_clipboard_params *params )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetClipboardData );
}

INT SYSCALL_API NtUserGetClipboardFormatName( UINT format, WCHAR *buffer, INT maxlen )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetClipboardFormatName );
}

HWND SYSCALL_API NtUserGetClipboardOwner(void)
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetClipboardOwner );
}

DWORD SYSCALL_API NtUserGetClipboardSequenceNumber(void)
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetClipboardSequenceNumber );
}

HWND SYSCALL_API NtUserGetClipboardViewer(void)
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetClipboardViewer );
}

HCURSOR SYSCALL_API NtUserGetCursor(void)
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetCursor );
}

HCURSOR SYSCALL_API NtUserGetCursorFrameInfo( HCURSOR cursor, DWORD istep, DWORD *rate_jiffies,
                                              DWORD *num_steps )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetCursorFrameInfo );
}

BOOL SYSCALL_API NtUserGetCursorInfo( CURSORINFO *info )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetCursorInfo );
}

HDC SYSCALL_API NtUserGetDC( HWND hwnd )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetDC );
}

HDC SYSCALL_API NtUserGetDCEx( HWND hwnd, HRGN clip_rgn, DWORD flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetDCEx );
}

LONG SYSCALL_API NtUserGetDisplayConfigBufferSizes( UINT32 flags, UINT32 *num_path_info,
                                                    UINT32 *num_mode_info )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetDisplayConfigBufferSizes );
}

UINT SYSCALL_API NtUserGetDoubleClickTime(void)
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetDoubleClickTime );
}

BOOL SYSCALL_API NtUserGetDpiForMonitor( HMONITOR monitor, UINT type, UINT *x, UINT *y )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetDpiForMonitor );
}

HWND SYSCALL_API NtUserGetForegroundWindow(void)
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetForegroundWindow );
}

BOOL SYSCALL_API NtUserGetGUIThreadInfo( DWORD id, GUITHREADINFO *info )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetGUIThreadInfo );
}

BOOL SYSCALL_API NtUserGetIconInfo( HICON icon, ICONINFO *info, UNICODE_STRING *module,
                                    UNICODE_STRING *res_name, DWORD *bpp, LONG unk )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetIconInfo );
}

BOOL SYSCALL_API NtUserGetIconSize( HICON handle, UINT step, LONG *width, LONG *height )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetIconSize );
}

UINT SYSCALL_API NtUserGetInternalWindowPos( HWND hwnd, RECT *rect, POINT *pt )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetInternalWindowPos );
}

INT SYSCALL_API NtUserGetKeyNameText( LONG lparam, WCHAR *buffer, INT size )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetKeyNameText );
}

SHORT SYSCALL_API NtUserGetKeyState( INT vkey )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetKeyState );
}

HKL SYSCALL_API NtUserGetKeyboardLayout( DWORD thread_id )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetKeyboardLayout );
}

UINT SYSCALL_API NtUserGetKeyboardLayoutList( INT size, HKL *layouts )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetKeyboardLayoutList );
}

BOOL SYSCALL_API NtUserGetKeyboardLayoutName( WCHAR *name )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetKeyboardLayoutName );
}

BOOL SYSCALL_API NtUserGetKeyboardState( BYTE *state )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetKeyboardState );
}

BOOL SYSCALL_API NtUserGetLayeredWindowAttributes( HWND hwnd, COLORREF *key, BYTE *alpha, DWORD *flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetLayeredWindowAttributes );
}

BOOL SYSCALL_API NtUserGetMenuBarInfo( HWND hwnd, LONG id, LONG item, MENUBARINFO *info )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetMenuBarInfo );
}

BOOL SYSCALL_API NtUserGetMenuItemRect( HWND hwnd, HMENU handle, UINT item, RECT *rect )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetMenuItemRect );
}

BOOL SYSCALL_API NtUserGetMessage( MSG *msg, HWND hwnd, UINT first, UINT last )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetMessage );
}

int SYSCALL_API NtUserGetMouseMovePointsEx( UINT size, MOUSEMOVEPOINT *ptin, MOUSEMOVEPOINT *ptout,
                                            int count, DWORD resolution )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetMouseMovePointsEx );
}

BOOL SYSCALL_API NtUserGetObjectInformation( HANDLE handle, INT index, void *info,
                                             DWORD len, DWORD *needed )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetObjectInformation );
}

HWND SYSCALL_API NtUserGetOpenClipboardWindow(void)
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetOpenClipboardWindow );
}

BOOL SYSCALL_API NtUserGetPointerInfoList( UINT32 id, POINTER_INPUT_TYPE type, UINT_PTR unk0, UINT_PTR unk1, SIZE_T size,
                                           UINT32 *entry_count, UINT32 *pointer_count, void *pointer_info )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetPointerInfoList );
}

INT SYSCALL_API NtUserGetPriorityClipboardFormat( UINT *list, INT count )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetPriorityClipboardFormat );
}

ULONG SYSCALL_API NtUserGetProcessDpiAwarenessContext( HANDLE process )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetProcessDpiAwarenessContext );
}

HWINSTA SYSCALL_API NtUserGetProcessWindowStation(void)
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetProcessWindowStation );
}

HANDLE SYSCALL_API NtUserGetProp( HWND hwnd, const WCHAR *str )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetProp );
}

DWORD SYSCALL_API NtUserGetQueueStatus( UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetQueueStatus );
}

UINT SYSCALL_API NtUserGetRawInputBuffer( RAWINPUT *data, UINT *data_size, UINT header_size )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetRawInputBuffer );
}

UINT SYSCALL_API NtUserGetRawInputData( HRAWINPUT rawinput, UINT command, void *data, UINT *data_size, UINT header_size )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetRawInputData );
}

UINT SYSCALL_API NtUserGetRawInputDeviceInfo( HANDLE handle, UINT command, void *data, UINT *data_size )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetRawInputDeviceInfo );
}

UINT SYSCALL_API NtUserGetRawInputDeviceList( RAWINPUTDEVICELIST *device_list, UINT *device_count, UINT size )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetRawInputDeviceList );
}

UINT SYSCALL_API NtUserGetRegisteredRawInputDevices( RAWINPUTDEVICE *devices, UINT *device_count, UINT device_size )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetRegisteredRawInputDevices );
}

BOOL SYSCALL_API NtUserGetScrollBarInfo( HWND hwnd, LONG id, SCROLLBARINFO *info )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetScrollBarInfo );
}

ULONG SYSCALL_API NtUserGetSystemDpiForProcess( HANDLE process )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetSystemDpiForProcess );
}

HMENU SYSCALL_API NtUserGetSystemMenu( HWND hwnd, BOOL revert )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetSystemMenu );
}

HDESK SYSCALL_API NtUserGetThreadDesktop( DWORD thread )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetThreadDesktop );
}

BOOL SYSCALL_API NtUserGetTitleBarInfo( HWND hwnd, TITLEBARINFO *info )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetTitleBarInfo );
}

BOOL SYSCALL_API NtUserGetUpdateRect( HWND hwnd, RECT *rect, BOOL erase )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetUpdateRect );
}

INT SYSCALL_API NtUserGetUpdateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetUpdateRgn );
}

BOOL SYSCALL_API NtUserGetUpdatedClipboardFormats( UINT *formats, UINT size, UINT *out_size )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetUpdatedClipboardFormats );
}

HDC SYSCALL_API NtUserGetWindowDC( HWND hwnd )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetWindowDC );
}

BOOL SYSCALL_API NtUserGetWindowPlacement( HWND hwnd, WINDOWPLACEMENT *placement )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetWindowPlacement );
}

int SYSCALL_API NtUserGetWindowRgnEx( HWND hwnd, HRGN hrgn, UINT unk )
{
    __ASM_SYSCALL_FUNC( __id_NtUserGetWindowRgnEx );
}

BOOL SYSCALL_API NtUserHideCaret( HWND hwnd )
{
    __ASM_SYSCALL_FUNC( __id_NtUserHideCaret );
}

BOOL SYSCALL_API NtUserHiliteMenuItem( HWND hwnd, HMENU handle, UINT item, UINT hilite )
{
    __ASM_SYSCALL_FUNC( __id_NtUserHiliteMenuItem );
}

NTSTATUS SYSCALL_API NtUserInitializeClientPfnArrays( const struct user_client_procs *client_procsA,
                                                      const struct user_client_procs *client_procsW,
                                                      const void *client_workers, HINSTANCE user_module )
{
    __ASM_SYSCALL_FUNC( __id_NtUserInitializeClientPfnArrays );
}

HICON SYSCALL_API NtUserInternalGetWindowIcon( HWND hwnd, UINT type )
{
    __ASM_SYSCALL_FUNC( __id_NtUserInternalGetWindowIcon );
}

INT SYSCALL_API NtUserInternalGetWindowText( HWND hwnd, WCHAR *text, INT count )
{
    __ASM_SYSCALL_FUNC( __id_NtUserInternalGetWindowText );
}

BOOL SYSCALL_API NtUserInvalidateRect( HWND hwnd, const RECT *rect, BOOL erase )
{
    __ASM_SYSCALL_FUNC( __id_NtUserInvalidateRect );
}

BOOL SYSCALL_API NtUserInvalidateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    __ASM_SYSCALL_FUNC( __id_NtUserInvalidateRgn );
}

BOOL SYSCALL_API NtUserIsClipboardFormatAvailable( UINT format )
{
    __ASM_SYSCALL_FUNC( __id_NtUserIsClipboardFormatAvailable );
}

BOOL SYSCALL_API NtUserIsMouseInPointerEnabled(void)
{
    __ASM_SYSCALL_FUNC( __id_NtUserIsMouseInPointerEnabled );
}

BOOL SYSCALL_API NtUserKillTimer( HWND hwnd, UINT_PTR id )
{
    __ASM_SYSCALL_FUNC( __id_NtUserKillTimer );
}

BOOL SYSCALL_API NtUserLockWindowUpdate( HWND hwnd )
{
    __ASM_SYSCALL_FUNC( __id_NtUserLockWindowUpdate );
}

BOOL SYSCALL_API NtUserLogicalToPerMonitorDPIPhysicalPoint( HWND hwnd, POINT *pt )
{
    __ASM_SYSCALL_FUNC( __id_NtUserLogicalToPerMonitorDPIPhysicalPoint );
}

UINT SYSCALL_API NtUserMapVirtualKeyEx( UINT code, UINT type, HKL layout )
{
    __ASM_SYSCALL_FUNC( __id_NtUserMapVirtualKeyEx );
}

INT SYSCALL_API NtUserMenuItemFromPoint( HWND hwnd, HMENU handle, int x, int y )
{
    __ASM_SYSCALL_FUNC( __id_NtUserMenuItemFromPoint );
}

LRESULT SYSCALL_API NtUserMessageCall( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                       void *result_info, DWORD type, BOOL ansi )
{
    __ASM_SYSCALL_FUNC( __id_NtUserMessageCall );
}

BOOL SYSCALL_API NtUserMoveWindow( HWND hwnd, INT x, INT y, INT cx, INT cy, BOOL repaint )
{
    __ASM_SYSCALL_FUNC( __id_NtUserMoveWindow );
}

DWORD SYSCALL_API NtUserMsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles,
                                                     DWORD timeout, DWORD mask, DWORD flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserMsgWaitForMultipleObjectsEx );
}

void SYSCALL_API NtUserNotifyIMEStatus( HWND hwnd, UINT status )
{
    __ASM_SYSCALL_FUNC( __id_NtUserNotifyIMEStatus );
}

void SYSCALL_API NtUserNotifyWinEvent( DWORD event, HWND hwnd, LONG object_id, LONG child_id )
{
    __ASM_SYSCALL_FUNC( __id_NtUserNotifyWinEvent );
}

BOOL SYSCALL_API NtUserOpenClipboard( HWND hwnd, ULONG unk )
{
    __ASM_SYSCALL_FUNC( __id_NtUserOpenClipboard );
}

HDESK SYSCALL_API NtUserOpenDesktop( OBJECT_ATTRIBUTES *attr, DWORD flags, ACCESS_MASK access )
{
    __ASM_SYSCALL_FUNC( __id_NtUserOpenDesktop );
}

HDESK SYSCALL_API NtUserOpenInputDesktop( DWORD flags, BOOL inherit, ACCESS_MASK access )
{
    __ASM_SYSCALL_FUNC( __id_NtUserOpenInputDesktop );
}

HWINSTA SYSCALL_API NtUserOpenWindowStation( OBJECT_ATTRIBUTES *attr, ACCESS_MASK access )
{
    __ASM_SYSCALL_FUNC( __id_NtUserOpenWindowStation );
}

BOOL SYSCALL_API NtUserPeekMessage( MSG *msg_out, HWND hwnd, UINT first, UINT last, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserPeekMessage );
}

BOOL SYSCALL_API NtUserPerMonitorDPIPhysicalToLogicalPoint( HWND hwnd, POINT *pt )
{
    __ASM_SYSCALL_FUNC( __id_NtUserPerMonitorDPIPhysicalToLogicalPoint );
}

BOOL SYSCALL_API NtUserPostMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    __ASM_SYSCALL_FUNC( __id_NtUserPostMessage );
}

BOOL SYSCALL_API NtUserPostThreadMessage( DWORD thread, UINT msg, WPARAM wparam, LPARAM lparam )
{
    __ASM_SYSCALL_FUNC( __id_NtUserPostThreadMessage );
}

BOOL SYSCALL_API NtUserPrintWindow( HWND hwnd, HDC hdc, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserPrintWindow );
}

LONG SYSCALL_API NtUserQueryDisplayConfig( UINT32 flags, UINT32 *paths_count, DISPLAYCONFIG_PATH_INFO *paths,
                                           UINT32 *modes_count, DISPLAYCONFIG_MODE_INFO *modes,
                                           DISPLAYCONFIG_TOPOLOGY_ID *topology_id )
{
    __ASM_SYSCALL_FUNC( __id_NtUserQueryDisplayConfig );
}

UINT_PTR SYSCALL_API NtUserQueryInputContext( HIMC handle, UINT attr )
{
    __ASM_SYSCALL_FUNC( __id_NtUserQueryInputContext );
}

HWND SYSCALL_API NtUserRealChildWindowFromPoint( HWND parent, LONG x, LONG y )
{
    __ASM_SYSCALL_FUNC( __id_NtUserRealChildWindowFromPoint );
}

BOOL SYSCALL_API NtUserRedrawWindow( HWND hwnd, const RECT *rect, HRGN hrgn, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserRedrawWindow );
}

ATOM SYSCALL_API NtUserRegisterClassExWOW( const WNDCLASSEXW *wc, UNICODE_STRING *name, UNICODE_STRING *version,
                                           struct client_menu_name *client_menu_name, DWORD fnid,
                                           DWORD flags, DWORD *wow )
{
    __ASM_SYSCALL_FUNC( __id_NtUserRegisterClassExWOW );
}

BOOL SYSCALL_API NtUserRegisterHotKey( HWND hwnd, INT id, UINT modifiers, UINT vk )
{
    __ASM_SYSCALL_FUNC( __id_NtUserRegisterHotKey );
}

BOOL SYSCALL_API NtUserRegisterRawInputDevices( const RAWINPUTDEVICE *devices, UINT device_count, UINT device_size )
{
    __ASM_SYSCALL_FUNC( __id_NtUserRegisterRawInputDevices );
}

INT SYSCALL_API NtUserReleaseDC( HWND hwnd, HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtUserReleaseDC );
}

BOOL SYSCALL_API NtUserRemoveClipboardFormatListener( HWND hwnd )
{
    __ASM_SYSCALL_FUNC( __id_NtUserRemoveClipboardFormatListener );
}

BOOL SYSCALL_API NtUserRemoveMenu( HMENU handle, UINT id, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserRemoveMenu );
}

HANDLE SYSCALL_API NtUserRemoveProp( HWND hwnd, const WCHAR *str )
{
    __ASM_SYSCALL_FUNC( __id_NtUserRemoveProp );
}

BOOL SYSCALL_API NtUserScrollDC( HDC hdc, INT dx, INT dy, const RECT *scroll, const RECT *clip,
                                 HRGN ret_update_rgn, RECT *update_rect )
{
    __ASM_SYSCALL_FUNC( __id_NtUserScrollDC );
}

INT SYSCALL_API NtUserScrollWindowEx( HWND hwnd, INT dx, INT dy, const RECT *rect,
                                      const RECT *clip_rect, HRGN update_rgn,
                                      RECT *update_rect, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserScrollWindowEx );
}

HPALETTE SYSCALL_API NtUserSelectPalette( HDC hdc, HPALETTE hpal, WORD bkg )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSelectPalette );
}

UINT SYSCALL_API NtUserSendInput( UINT count, INPUT *inputs, int size )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSendInput );
}

HWND SYSCALL_API NtUserSetActiveWindow( HWND hwnd )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetActiveWindow );
}

HWND SYSCALL_API NtUserSetCapture( HWND hwnd )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetCapture );
}

DWORD SYSCALL_API NtUserSetClassLong( HWND hwnd, INT offset, LONG newval, BOOL ansi )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetClassLong );
}

ULONG_PTR SYSCALL_API NtUserSetClassLongPtr( HWND hwnd, INT offset, LONG_PTR newval, BOOL ansi )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetClassLongPtr );
}

WORD SYSCALL_API NtUserSetClassWord( HWND hwnd, INT offset, WORD newval )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetClassWord );
}

NTSTATUS SYSCALL_API NtUserSetClipboardData( UINT format, HANDLE data, struct set_clipboard_params *params )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetClipboardData );
}

HWND SYSCALL_API NtUserSetClipboardViewer( HWND hwnd )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetClipboardViewer );
}

HCURSOR SYSCALL_API NtUserSetCursor( HCURSOR cursor )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetCursor );
}

BOOL SYSCALL_API NtUserSetCursorIconData( HCURSOR cursor, UNICODE_STRING *module, UNICODE_STRING *res_name,
                                          struct cursoricon_desc *desc )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetCursorIconData );
}

BOOL SYSCALL_API NtUserSetCursorPos( INT x, INT y )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetCursorPos );
}

HWND SYSCALL_API NtUserSetFocus( HWND hwnd )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetFocus );
}

void SYSCALL_API NtUserSetInternalWindowPos( HWND hwnd, UINT cmd, RECT *rect, POINT *pt )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetInternalWindowPos );
}

BOOL SYSCALL_API NtUserSetKeyboardState( BYTE *state )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetKeyboardState );
}

BOOL SYSCALL_API NtUserSetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetLayeredWindowAttributes );
}

BOOL SYSCALL_API NtUserSetMenu( HWND hwnd, HMENU menu )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetMenu );
}

BOOL SYSCALL_API NtUserSetMenuContextHelpId( HMENU handle, DWORD id )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetMenuContextHelpId );
}

BOOL SYSCALL_API NtUserSetMenuDefaultItem( HMENU handle, UINT item, UINT bypos )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetMenuDefaultItem );
}

BOOL SYSCALL_API NtUserSetObjectInformation( HANDLE handle, INT index, void *info, DWORD len )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetObjectInformation );
}

HWND SYSCALL_API NtUserSetParent( HWND hwnd, HWND parent )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetParent );
}

BOOL SYSCALL_API NtUserSetProcessDpiAwarenessContext( ULONG awareness, ULONG unknown )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetProcessDpiAwarenessContext );
}

BOOL SYSCALL_API NtUserSetProcessWindowStation( HWINSTA handle )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetProcessWindowStation );
}

BOOL SYSCALL_API NtUserSetProp( HWND hwnd, const WCHAR *str, HANDLE handle )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetProp );
}

INT SYSCALL_API NtUserSetScrollInfo( HWND hwnd, int bar, const SCROLLINFO *info, BOOL redraw )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetScrollInfo );
}

BOOL SYSCALL_API NtUserSetShellWindowEx( HWND shell, HWND list_view )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetShellWindowEx );
}

BOOL SYSCALL_API NtUserSetSysColors( INT count, const INT *colors, const COLORREF *values )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetSysColors );
}

BOOL SYSCALL_API NtUserSetSystemMenu( HWND hwnd, HMENU menu )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetSystemMenu );
}

UINT_PTR SYSCALL_API NtUserSetSystemTimer( HWND hwnd, UINT_PTR id, UINT timeout )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetSystemTimer );
}

BOOL SYSCALL_API NtUserSetThreadDesktop( HDESK handle )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetThreadDesktop );
}

UINT_PTR SYSCALL_API NtUserSetTimer( HWND hwnd, UINT_PTR id, UINT timeout, TIMERPROC proc, ULONG tolerance )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetTimer );
}

HWINEVENTHOOK SYSCALL_API NtUserSetWinEventHook( DWORD event_min, DWORD event_max, HMODULE inst,
                                                 UNICODE_STRING *module, WINEVENTPROC proc,
                                                 DWORD pid, DWORD tid, DWORD flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetWinEventHook );
}

LONG SYSCALL_API NtUserSetWindowLong( HWND hwnd, INT offset, LONG newval, BOOL ansi )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetWindowLong );
}

LONG_PTR SYSCALL_API NtUserSetWindowLongPtr( HWND hwnd, INT offset, LONG_PTR newval, BOOL ansi )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetWindowLongPtr );
}

BOOL SYSCALL_API NtUserSetWindowPlacement( HWND hwnd, const WINDOWPLACEMENT *wpl )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetWindowPlacement );
}

BOOL SYSCALL_API NtUserSetWindowPos( HWND hwnd, HWND after, INT x, INT y, INT cx, INT cy, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetWindowPos );
}

int SYSCALL_API NtUserSetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetWindowRgn );
}

WORD SYSCALL_API NtUserSetWindowWord( HWND hwnd, INT offset, WORD newval )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetWindowWord );
}

HHOOK SYSCALL_API NtUserSetWindowsHookEx( HINSTANCE inst, UNICODE_STRING *module, DWORD tid, INT id,
                                          HOOKPROC proc, BOOL ansi )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSetWindowsHookEx );
}

BOOL SYSCALL_API NtUserShowCaret( HWND hwnd )
{
    __ASM_SYSCALL_FUNC( __id_NtUserShowCaret );
}

INT SYSCALL_API NtUserShowCursor( BOOL show )
{
    __ASM_SYSCALL_FUNC( __id_NtUserShowCursor );
}

BOOL SYSCALL_API NtUserShowScrollBar( HWND hwnd, INT bar, BOOL show )
{
    __ASM_SYSCALL_FUNC( __id_NtUserShowScrollBar );
}

BOOL SYSCALL_API NtUserShowWindow( HWND hwnd, INT cmd )
{
    __ASM_SYSCALL_FUNC( __id_NtUserShowWindow );
}

BOOL SYSCALL_API NtUserShowWindowAsync( HWND hwnd, INT cmd )
{
    __ASM_SYSCALL_FUNC( __id_NtUserShowWindowAsync );
}

BOOL SYSCALL_API NtUserSystemParametersInfo( UINT action, UINT val, void *ptr, UINT winini )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSystemParametersInfo );
}

BOOL SYSCALL_API NtUserSystemParametersInfoForDpi( UINT action, UINT val, PVOID ptr, UINT winini, UINT dpi )
{
    __ASM_SYSCALL_FUNC( __id_NtUserSystemParametersInfoForDpi );
}

BOOL SYSCALL_API NtUserThunkedMenuInfo( HMENU menu, const MENUINFO *info )
{
    __ASM_SYSCALL_FUNC( __id_NtUserThunkedMenuInfo );
}

UINT SYSCALL_API NtUserThunkedMenuItemInfo( HMENU handle, UINT pos, UINT flags, UINT method,
                                            MENUITEMINFOW *info, UNICODE_STRING *str )
{
    __ASM_SYSCALL_FUNC( __id_NtUserThunkedMenuItemInfo );
}

INT SYSCALL_API NtUserToUnicodeEx( UINT virt, UINT scan, const BYTE *state,
                                   WCHAR *str, int size, UINT flags, HKL layout )
{
    __ASM_SYSCALL_FUNC( __id_NtUserToUnicodeEx );
}

BOOL SYSCALL_API NtUserTrackMouseEvent( TRACKMOUSEEVENT *info )
{
    __ASM_SYSCALL_FUNC( __id_NtUserTrackMouseEvent );
}

BOOL SYSCALL_API NtUserTrackPopupMenuEx( HMENU handle, UINT flags, INT x, INT y, HWND hwnd,
                                         TPMPARAMS *params )
{
    __ASM_SYSCALL_FUNC( __id_NtUserTrackPopupMenuEx );
}

INT SYSCALL_API NtUserTranslateAccelerator( HWND hwnd, HACCEL accel, MSG *msg )
{
    __ASM_SYSCALL_FUNC( __id_NtUserTranslateAccelerator );
}

BOOL SYSCALL_API NtUserTranslateMessage( const MSG *msg, UINT flags )
{
    __ASM_SYSCALL_FUNC( __id_NtUserTranslateMessage );
}

BOOL SYSCALL_API NtUserUnhookWinEvent( HWINEVENTHOOK handle )
{
    __ASM_SYSCALL_FUNC( __id_NtUserUnhookWinEvent );
}

BOOL SYSCALL_API NtUserUnhookWindowsHookEx( HHOOK handle )
{
    __ASM_SYSCALL_FUNC( __id_NtUserUnhookWindowsHookEx );
}

BOOL SYSCALL_API NtUserUnregisterClass( UNICODE_STRING *name, HINSTANCE instance,
                                        struct client_menu_name *client_menu_name )
{
    __ASM_SYSCALL_FUNC( __id_NtUserUnregisterClass );
}

BOOL SYSCALL_API NtUserUnregisterHotKey( HWND hwnd, INT id )
{
    __ASM_SYSCALL_FUNC( __id_NtUserUnregisterHotKey );
}

BOOL SYSCALL_API NtUserUpdateInputContext( HIMC handle, UINT attr, UINT_PTR value )
{
    __ASM_SYSCALL_FUNC( __id_NtUserUpdateInputContext );
}

BOOL SYSCALL_API NtUserUpdateLayeredWindow( HWND hwnd, HDC hdc_dst, const POINT *pts_dst, const SIZE *size,
                                            HDC hdc_src, const POINT *pts_src, COLORREF key,
                                            const BLENDFUNCTION *blend, DWORD flags, const RECT *dirty )
{
    __ASM_SYSCALL_FUNC( __id_NtUserUpdateLayeredWindow );
}

BOOL SYSCALL_API NtUserValidateRect( HWND hwnd, const RECT *rect )
{
    __ASM_SYSCALL_FUNC( __id_NtUserValidateRect );
}

WORD SYSCALL_API NtUserVkKeyScanEx( WCHAR chr, HKL layout )
{
    __ASM_SYSCALL_FUNC( __id_NtUserVkKeyScanEx );
}

DWORD SYSCALL_API NtUserWaitForInputIdle( HANDLE process, DWORD timeout, BOOL wow )
{
    __ASM_SYSCALL_FUNC( __id_NtUserWaitForInputIdle );
}

BOOL SYSCALL_API NtUserWaitMessage(void)
{
    __ASM_SYSCALL_FUNC( __id_NtUserWaitMessage );
}

HWND SYSCALL_API NtUserWindowFromDC( HDC hdc )
{
    __ASM_SYSCALL_FUNC( __id_NtUserWindowFromDC );
}

HWND SYSCALL_API NtUserWindowFromPoint( LONG x, LONG y )
{
    __ASM_SYSCALL_FUNC( __id_NtUserWindowFromPoint );
}

BOOL SYSCALL_API __wine_get_file_outline_text_metric( const WCHAR *path, TEXTMETRICW *otm,
                                                      UINT *em_square, WCHAR *face_name )
{
    __ASM_SYSCALL_FUNC( __id___wine_get_file_outline_text_metric );
}

BOOL SYSCALL_API __wine_get_icm_profile( HDC hdc, BOOL allow_default, DWORD *size, WCHAR *filename )
{
    __ASM_SYSCALL_FUNC( __id___wine_get_icm_profile );
}

BOOL SYSCALL_API __wine_send_input( HWND hwnd, const INPUT *input, const RAWINPUT *rawinput )
{
    __ASM_SYSCALL_FUNC( __id___wine_send_input );
}

#else /*  __arm64ec__ */

#ifdef _WIN64
#define SYSCALL_ENTRY(id,name,args) __ASM_SYSCALL_FUNC( id + 0x1000, name )
ALL_SYSCALLS64
#else
#define SYSCALL_ENTRY(id,name,args) __ASM_SYSCALL_FUNC( id + 0x1000, name, args )
DEFINE_SYSCALL_HELPER32()
ALL_SYSCALLS32
#endif
#undef SYSCALL_ENTRY

#endif /*  __arm64ec__ */


void __cdecl __wine_spec_unimplemented_stub( const char *module, const char *function )
{
    EXCEPTION_RECORD record;

    record.ExceptionCode    = EXCEPTION_WINE_STUB;
    record.ExceptionFlags   = EH_NONCONTINUABLE;
    record.ExceptionRecord  = NULL;
    record.ExceptionAddress = __wine_spec_unimplemented_stub;
    record.NumberParameters = 2;
    record.ExceptionInformation[0] = (ULONG_PTR)module;
    record.ExceptionInformation[1] = (ULONG_PTR)function;
    for (;;) RtlRaiseException( &record );
}

BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, void *reserved )
{
    HMODULE ntdll;
    void **dispatcher_ptr;
    const UNICODE_STRING ntdll_name = RTL_CONSTANT_STRING( L"ntdll.dll" );

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        LdrDisableThreadCalloutsForDll( inst );
        if (__wine_syscall_dispatcher) break;  /* already set through Wow64Transition */
        LdrGetDllHandle( NULL, 0, &ntdll_name, &ntdll );
        dispatcher_ptr = RtlFindExportedRoutineByName( ntdll, "__wine_syscall_dispatcher" );
        __wine_syscall_dispatcher = *dispatcher_ptr;
        if (!NtQueryVirtualMemory( GetCurrentProcess(), inst, MemoryWineUnixFuncs,
                                   &win32u_handle, sizeof(win32u_handle), NULL ))
            __wine_unix_call( win32u_handle, 0, NULL );
        break;
    }
    return TRUE;
}
