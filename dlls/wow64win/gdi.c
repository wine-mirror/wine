/*
 * WoW64 GDI functions
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
#include "wow64win_private.h"

NTSTATUS WINAPI wow64_NtGdiCreateClientObj( UINT *args )
{
    ULONG type = get_ulong( &args );

    return HandleToUlong( NtGdiCreateClientObj( type ));
}

NTSTATUS WINAPI wow64_NtGdiDeleteClientObj( UINT *args )
{
    HGDIOBJ obj = get_handle( &args );

    return NtGdiDeleteClientObj( obj );
}

NTSTATUS WINAPI wow64_NtGdiExtGetObjectW( UINT *args )
{
    HGDIOBJ handle = get_handle( &args );
    INT count = get_ulong( &args );
    void *buffer = get_ptr( &args );

    return NtGdiExtGetObjectW( handle, count, buffer );
}

NTSTATUS WINAPI wow64_NtGdiGetDCObject( UINT *args )
{
    HDC hdc = get_handle( &args );
    UINT type = get_ulong( &args );

    return HandleToUlong( NtGdiGetDCObject( hdc, type ));
}

NTSTATUS WINAPI wow64_NtGdiCreateBitmap( UINT *args )
{
    INT width = get_ulong( &args );
    INT height = get_ulong( &args );
    UINT planes = get_ulong( &args );
    UINT bpp = get_ulong( &args );
    const void *bits = get_ptr( &args );

    return HandleToUlong( NtGdiCreateBitmap( width, height, planes, bpp, bits ));
}

NTSTATUS WINAPI wow64_NtGdiGetBitmapBits( UINT *args )
{
    HBITMAP bitmap = get_handle( &args );
    LONG count = get_ulong( &args );
    void *bits = get_ptr( &args );

    return NtGdiGetBitmapBits( bitmap, count, bits );
}

NTSTATUS WINAPI wow64_NtGdiSetBitmapBits( UINT *args )
{
    HBITMAP hbitmap = get_handle( &args );
    LONG count = get_ulong( &args );
    const void *bits = get_ptr( &args );

    return NtGdiSetBitmapBits( hbitmap, count, bits );
}

NTSTATUS WINAPI wow64_NtGdiGetBitmapDimension( UINT *args )
{
    HBITMAP bitmap = get_handle( &args );
    SIZE *size = get_ptr( &args );

    return NtGdiGetBitmapDimension( bitmap, size );
}

NTSTATUS WINAPI wow64_NtGdiSetBitmapDimension( UINT *args )
{
    HBITMAP hbitmap = get_handle( &args );
    INT x = get_ulong( &args );
    INT y = get_ulong( &args );
    SIZE *prev_size = get_ptr( &args );

    return NtGdiSetBitmapDimension( hbitmap, x, y, prev_size );
}

NTSTATUS WINAPI wow64_NtGdiCreateDIBSection( UINT *args )
{
    HDC hdc = get_handle( &args );
    HANDLE section = get_handle( &args );
    DWORD offset = get_ulong( &args );
    const BITMAPINFO *bmi = get_ptr( &args );
    UINT usage = get_ulong( &args );
    UINT header_size = get_ulong( &args );
    ULONG flags = get_ulong( &args );
    ULONG_PTR color_space = get_ulong( &args );
    void *bits32 = get_ptr( &args );

    void *bits;
    HBITMAP ret;

    ret = NtGdiCreateDIBSection( hdc, section, offset, bmi, usage, header_size,
                                 flags, color_space, addr_32to64( &bits, bits32 ));
    if (ret) put_addr( bits32, bits );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtGdiCreateDIBBrush( UINT *args )
{
    const void *data = get_ptr( &args );
    UINT coloruse = get_ulong( &args );
    UINT size = get_ulong( &args );
    BOOL is_8x8 = get_ulong( &args );
    BOOL pen = get_ulong( &args );
    const void *client = get_ptr( &args );

    return HandleToUlong( NtGdiCreateDIBBrush( data, coloruse, size, is_8x8, pen, client ));
}

NTSTATUS WINAPI wow64_NtGdiCreateHatchBrushInternal( UINT *args )
{
    INT style = get_ulong( &args );
    COLORREF color = get_ulong( &args );
    BOOL pen = get_ulong( &args );

    return HandleToULong( NtGdiCreateHatchBrushInternal( style, color, pen ));
}

NTSTATUS WINAPI wow64_NtGdiCreatePatternBrushInternal( UINT *args )
{
    HBITMAP hbitmap = get_handle( &args );
    BOOL pen = get_ulong( &args );
    BOOL is_8x8 = get_ulong( &args );

    return HandleToUlong( NtGdiCreatePatternBrushInternal( hbitmap, pen, is_8x8 ));
}

NTSTATUS WINAPI wow64_NtGdiCreateSolidBrush( UINT *args )
{
    COLORREF color = get_ulong( &args );
    HBRUSH brush = get_handle( &args );

    return HandleToUlong( NtGdiCreateSolidBrush( color, brush ));
}

NTSTATUS WINAPI wow64_NtGdiCreateRectRgn( UINT *args )
{
    INT left = get_ulong( &args );
    INT top = get_ulong( &args );
    INT right = get_ulong( &args );
    INT bottom = get_ulong( &args );

    return HandleToUlong( NtGdiCreateRectRgn( left, top, right, bottom ));
}

NTSTATUS WINAPI wow64_NtGdiCreateRoundRectRgn( UINT *args )
{
    INT left = get_ulong( &args );
    INT top = get_ulong( &args );
    INT right = get_ulong( &args );
    INT bottom = get_ulong( &args );
    INT ellipse_width = get_ulong( &args );
    INT ellipse_height = get_ulong( &args );

    return HandleToUlong( NtGdiCreateRoundRectRgn( left, top, right, bottom,
                                                   ellipse_width, ellipse_height ));
}

NTSTATUS WINAPI wow64_NtGdiCreateEllipticRgn( UINT *args )
{
    INT left = get_ulong( &args );
    INT top = get_ulong( &args );
    INT right = get_ulong( &args );
    INT bottom = get_ulong( &args );

    return HandleToUlong( NtGdiCreateEllipticRgn( left, top, right, bottom ));
}

NTSTATUS WINAPI wow64_NtGdiExtCreateRegion( UINT *args )
{
    const XFORM *xform = get_ptr( &args );
    DWORD count = get_ulong( &args );
    const RGNDATA *data = get_ptr( &args );

    return HandleToUlong( NtGdiExtCreateRegion( xform, count, data ));
}

NTSTATUS WINAPI wow64_NtGdiGetRegionData( UINT *args )
{
    HRGN hrgn = get_ptr( &args );
    DWORD count = get_ulong( &args );
    RGNDATA *data = get_ptr( &args );

    return NtGdiGetRegionData( hrgn, count, data );
}

NTSTATUS WINAPI wow64_NtGdiEqualRgn( UINT *args )
{
    HRGN hrgn1 = get_handle( &args );
    HRGN hrgn2 = get_handle( &args );

    return NtGdiEqualRgn( hrgn1, hrgn2 );
}

INT WINAPI wow64_NtGdiGetRgnBox( UINT *args )
{
    HRGN hrgn = get_handle( &args );
    RECT *rect = get_ptr( &args );

    return NtGdiGetRgnBox( hrgn, rect );
}

BOOL WINAPI wow64_NtGdiSetRectRgn( UINT *args )
{
    HRGN hrgn = get_handle( &args );
    INT left = get_ulong( &args );
    INT top = get_ulong( &args );
    INT right = get_ulong( &args );
    INT bottom = get_ulong( &args );

    return NtGdiSetRectRgn( hrgn, left, top, right, bottom );
}

INT WINAPI wow64_NtGdiOffsetRgn( UINT *args )
{
    HRGN hrgn = get_handle( &args );
    INT x = get_ulong( &args );
    INT y = get_ulong( &args );

    return NtGdiOffsetRgn( hrgn, x, y );
}

NTSTATUS WINAPI wow64_NtGdiCombineRgn( UINT *args )
{
    HRGN dest = get_handle( &args );
    HRGN src1 = get_handle( &args );
    HRGN src2 = get_handle( &args );
    INT mode = get_ulong( &args );

    return NtGdiCombineRgn( dest, src1, src2, mode );
}

NTSTATUS WINAPI wow64_NtGdiPtInRegion( UINT *args )
{
    HRGN hrgn = get_handle( &args );
    INT x = get_ulong( &args );
    INT y = get_ulong( &args );

    return NtGdiPtInRegion( hrgn, x, y );
}

BOOL WINAPI wow64_NtGdiRectInRegion( UINT *args )
{
    HRGN hrgn = get_handle( &args );
    const RECT *rect = get_ptr( &args );

    return NtGdiRectInRegion( hrgn, rect );
}

NTSTATUS WINAPI wow64_NtGdiSetMetaRgn( UINT *args )
{
    HDC hdc = get_handle( &args );

    return NtGdiSetMetaRgn( hdc );
}

NTSTATUS WINAPI wow64_NtGdiSaveDC( UINT *args )
{
    HDC hdc = get_handle( &args );

    return NtGdiSaveDC( hdc );
}

BOOL WINAPI wow64_NtGdiSetBrushOrg( UINT *args )
{
    HDC hdc = get_handle( &args );
    INT x = get_ulong( &args );
    INT y = get_ulong( &args );
    POINT *prev_org = get_ptr( &args );

    return NtGdiSetBrushOrg( hdc, x, y, prev_org );
}

BOOL WINAPI wow64_NtGdiGetTransform( UINT *args )
{
    HDC hdc = get_handle( &args );
    DWORD which = get_ulong( &args );
    XFORM *xform = get_ptr( &args );

    return NtGdiGetTransform( hdc, which, xform );
}

NTSTATUS WINAPI wow64_NtGdiDescribePixelFormat( UINT *args )
{
    HDC hdc = get_handle( &args );
    INT format = get_ulong( &args );
    UINT size = get_ulong( &args );
    PIXELFORMATDESCRIPTOR *descr = get_ptr( &args );

    return NtGdiDescribePixelFormat( hdc, format, size, descr );
}

NTSTATUS WINAPI wow64_NtGdiSetPixelFormat( UINT *args )
{
    HDC hdc = get_handle( &args );
    INT format = get_ulong( &args );

    return NtGdiSetPixelFormat( hdc, format );
}

NTSTATUS WINAPI wow64_NtGdiSwapBuffers( UINT *args )
{
    HDC hdc = get_handle( &args );

    return NtGdiSwapBuffers( hdc );
}

NTSTATUS WINAPI wow64_NtGdiDrawStream( UINT *args )
{
    HDC hdc = get_handle( &args );
    ULONG in = get_ulong( &args );
    void *pvin = get_ptr( &args );

    return NtGdiDrawStream( hdc, in, pvin );
}

NTSTATUS WINAPI wow64_NtGdiSetTextJustification( UINT *args )
{
    HDC hdc = get_handle( &args );
    INT extra = get_ulong( &args );
    INT breaks = get_ulong( &args );

    return NtGdiSetTextJustification( hdc, extra, breaks );
}

NTSTATUS WINAPI wow64_NtGdiHfontCreate( UINT *args )
{
    const ENUMLOGFONTEXDVW *enumex = get_ptr( &args );
    ULONG unk2 = get_ulong( &args );
    ULONG unk3 = get_ulong( &args );
    ULONG unk4 = get_ulong( &args );
    void *data = get_ptr( &args );

    return HandleToUlong( NtGdiHfontCreate( enumex, unk2, unk3, unk4, data ));
}

NTSTATUS WINAPI wow64_NtGdiGetFontFileData( UINT *args )
{
    DWORD instance_id = get_ulong( &args );
    DWORD file_index = get_ulong( &args );
    UINT64 *offset = get_ptr( &args );
    void *buff = get_ptr( &args );
    DWORD buff_size = get_ulong( &args );

    return NtGdiGetFontFileData( instance_id, file_index, offset, buff, buff_size );
}

NTSTATUS WINAPI wow64_NtGdiGetFontFileInfo( UINT *args )
{
    DWORD instance_id = get_ulong( &args );
    DWORD file_index = get_ulong( &args );
    struct font_fileinfo *info = get_ptr( &args );
    SIZE_T size = get_ulong( &args );
    ULONG *needed32 = get_ptr( &args );

    SIZE_T needed;
    BOOL ret;

    ret = NtGdiGetFontFileInfo( instance_id, file_index, info, size, size_32to64( &needed, needed32 ));
    put_size( needed32, needed );
    return ret;
}

NTSTATUS WINAPI wow64_NtGdiAddFontMemResourceEx( UINT *args )
{
    void *ptr = get_ptr( &args );
    DWORD size = get_ulong( &args );
    void *dv = get_ptr( &args );
    ULONG dv_size = get_ulong( &args );
    DWORD *count = get_ptr( &args );

    return HandleToUlong( NtGdiAddFontMemResourceEx( ptr, size, dv, dv_size, count ));
}

NTSTATUS WINAPI wow64_NtGdiAddFontResourceW( UINT *args )
{
    const WCHAR *str = get_ptr( &args );
    ULONG size = get_ulong( &args );
    ULONG files = get_ulong( &args );
    DWORD flags = get_ulong( &args );
    DWORD tid = get_ulong( &args );
    void *dv = get_ptr( &args );

    return NtGdiAddFontResourceW( str, size, files, flags, tid, dv );
}

NTSTATUS WINAPI wow64_NtGdiRemoveFontMemResourceEx( UINT *args )
{
    HANDLE handle = get_handle( &args );

    return NtGdiRemoveFontMemResourceEx( handle );
}

NTSTATUS WINAPI wow64_NtGdiRemoveFontResourceW( UINT *args )
{
    const WCHAR *str = get_ptr( &args );
    ULONG size = get_ulong( &args );
    ULONG files = get_ulong( &args );
    DWORD flags = get_ulong( &args );
    DWORD tid = get_ulong( &args );
    void *dv = get_ptr( &args );

    return NtGdiRemoveFontResourceW( str, size, files, flags, tid, dv );
}

NTSTATUS WINAPI wow64_NtGdiGetColorAdjustment( UINT *args )
{
    HDC hdc = get_handle( &args );
    COLORADJUSTMENT *ca = get_ptr( &args );

    return NtGdiGetColorAdjustment( hdc, ca );
}

NTSTATUS WINAPI wow64_NtGdiSetColorAdjustment( UINT *args )
{
    HDC hdc = get_handle( &args );
    const COLORADJUSTMENT *ca = get_ptr( &args );

    return NtGdiSetColorAdjustment( hdc, ca );
}

NTSTATUS WINAPI wow64_NtGdiFlush( UINT *args )
{
    return NtGdiFlush();
}
