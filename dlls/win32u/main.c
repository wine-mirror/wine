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
#include "rtlsupportapi.h"
#include "wine/unixlib.h"
#include "wine/asm.h"
#include "win32syscalls.h"

void *__wine_syscall_dispatcher = NULL;

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

#define SYSCALL_API __attribute__((naked, hybrid_patchable))
#define SYSCALL_FUNC(name) __ASM_SYSCALL_FUNC( __id_##name, name )

INT SYSCALL_API NtGdiAbortDoc( HDC hdc )
{
    SYSCALL_FUNC( NtGdiAbortDoc );
}

BOOL SYSCALL_API NtGdiAbortPath( HDC hdc )
{
    SYSCALL_FUNC( NtGdiAbortPath );
}

HANDLE SYSCALL_API NtGdiAddFontMemResourceEx( void *ptr, DWORD size, void *dv, ULONG dv_size,
                                              DWORD *count )
{
    SYSCALL_FUNC( NtGdiAddFontMemResourceEx );
}

INT SYSCALL_API NtGdiAddFontResourceW( const WCHAR *str, ULONG size, ULONG files, DWORD flags,
                                       DWORD tid, void *dv )
{
    SYSCALL_FUNC( NtGdiAddFontResourceW );
}

BOOL SYSCALL_API NtGdiAlphaBlend( HDC hdcDst, int xDst, int yDst, int widthDst, int heightDst,
                                  HDC hdcSrc, int xSrc, int ySrc, int widthSrc, int heightSrc,
                                  DWORD blend_func, HANDLE xform )
{
    SYSCALL_FUNC( NtGdiAlphaBlend );
}

BOOL SYSCALL_API NtGdiAngleArc( HDC hdc, INT x, INT y, DWORD dwRadius, DWORD start_angle, DWORD sweep_angle )
{
    SYSCALL_FUNC( NtGdiAngleArc );
}

BOOL SYSCALL_API NtGdiArcInternal( UINT type, HDC hdc, INT left, INT top, INT right,
                                   INT bottom, INT xstart, INT ystart, INT xend, INT yend )
{
    SYSCALL_FUNC( NtGdiArcInternal );
}

BOOL SYSCALL_API NtGdiBeginPath( HDC hdc )
{
    SYSCALL_FUNC( NtGdiBeginPath );
}

BOOL SYSCALL_API NtGdiBitBlt( HDC hdc_dst, INT x_dst, INT y_dst, INT width, INT height,
                              HDC hdc_src, INT x_src, INT y_src, DWORD rop, DWORD bk_color, FLONG fl )
{
    SYSCALL_FUNC( NtGdiBitBlt );
}

BOOL SYSCALL_API NtGdiCloseFigure( HDC hdc )
{
    SYSCALL_FUNC( NtGdiCloseFigure );
}

INT SYSCALL_API NtGdiCombineRgn( HRGN hDest, HRGN hSrc1, HRGN hSrc2, INT mode )
{
    SYSCALL_FUNC( NtGdiCombineRgn );
}

BOOL SYSCALL_API NtGdiComputeXformCoefficients( HDC hdc )
{
    SYSCALL_FUNC( NtGdiComputeXformCoefficients );
}

HBITMAP SYSCALL_API NtGdiCreateBitmap( INT width, INT height, UINT planes,
                                       UINT bpp, const void *bits )
{
    SYSCALL_FUNC( NtGdiCreateBitmap );
}

HANDLE SYSCALL_API NtGdiCreateClientObj( ULONG type )
{
    SYSCALL_FUNC( NtGdiCreateClientObj );
}

HBITMAP SYSCALL_API NtGdiCreateCompatibleBitmap( HDC hdc, INT width, INT height )
{
    SYSCALL_FUNC( NtGdiCreateCompatibleBitmap );
}

HDC SYSCALL_API NtGdiCreateCompatibleDC( HDC hdc )
{
    SYSCALL_FUNC( NtGdiCreateCompatibleDC );
}

HBRUSH SYSCALL_API NtGdiCreateDIBBrush( const void *data, UINT coloruse, UINT size,
                                        BOOL is_8x8, BOOL pen, const void *client )
{
    SYSCALL_FUNC( NtGdiCreateDIBBrush );
}

HBITMAP SYSCALL_API NtGdiCreateDIBSection( HDC hdc, HANDLE section, DWORD offset, const BITMAPINFO *bmi,
                                           UINT usage, UINT header_size, ULONG flags,
                                           ULONG_PTR color_space, void **bits )
{
    SYSCALL_FUNC( NtGdiCreateDIBSection );
}

HBITMAP SYSCALL_API NtGdiCreateDIBitmapInternal( HDC hdc, INT width, INT height, DWORD init,
                                                 const void *bits, const BITMAPINFO *data,
                                                 UINT coloruse, UINT max_info, UINT max_bits,
                                                 ULONG flags, HANDLE xform )
{
    SYSCALL_FUNC( NtGdiCreateDIBitmapInternal );
}

HRGN SYSCALL_API NtGdiCreateEllipticRgn( INT left, INT top, INT right, INT bottom )
{
    SYSCALL_FUNC( NtGdiCreateEllipticRgn );
}

HPALETTE SYSCALL_API NtGdiCreateHalftonePalette( HDC hdc )
{
    SYSCALL_FUNC( NtGdiCreateHalftonePalette );
}

HBRUSH SYSCALL_API NtGdiCreateHatchBrushInternal( INT style, COLORREF color, BOOL pen )
{
    SYSCALL_FUNC( NtGdiCreateHatchBrushInternal );
}

HDC SYSCALL_API NtGdiCreateMetafileDC( HDC hdc )
{
    SYSCALL_FUNC( NtGdiCreateMetafileDC );
}

HPALETTE SYSCALL_API NtGdiCreatePaletteInternal( const LOGPALETTE *palette, UINT count )
{
    SYSCALL_FUNC( NtGdiCreatePaletteInternal );
}

HBRUSH SYSCALL_API NtGdiCreatePatternBrushInternal( HBITMAP bitmap, BOOL pen, BOOL is_8x8 )
{
    SYSCALL_FUNC( NtGdiCreatePatternBrushInternal );
}

HPEN SYSCALL_API NtGdiCreatePen( INT style, INT width, COLORREF color, HBRUSH brush )
{
    SYSCALL_FUNC( NtGdiCreatePen );
}

HRGN SYSCALL_API NtGdiCreateRectRgn( INT left, INT top, INT right, INT bottom )
{
    SYSCALL_FUNC( NtGdiCreateRectRgn );
}

HRGN SYSCALL_API NtGdiCreateRoundRectRgn( INT left, INT top, INT right, INT bottom,
                                          INT ellipse_width, INT ellipse_height )
{
    SYSCALL_FUNC( NtGdiCreateRoundRectRgn );
}

HBRUSH SYSCALL_API NtGdiCreateSolidBrush( COLORREF color, HBRUSH brush )
{
    SYSCALL_FUNC( NtGdiCreateSolidBrush );
}

NTSTATUS SYSCALL_API NtGdiDdDDICheckVidPnExclusiveOwnership( const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *desc )
{
    SYSCALL_FUNC( NtGdiDdDDICheckVidPnExclusiveOwnership );
}

NTSTATUS SYSCALL_API NtGdiDdDDICloseAdapter( const D3DKMT_CLOSEADAPTER *desc )
{
    SYSCALL_FUNC( NtGdiDdDDICloseAdapter );
}

NTSTATUS SYSCALL_API NtGdiDdDDICreateAllocation( D3DKMT_CREATEALLOCATION *desc )
{
    SYSCALL_FUNC( NtGdiDdDDICreateAllocation );
}

NTSTATUS SYSCALL_API NtGdiDdDDICreateAllocation2( D3DKMT_CREATEALLOCATION *desc )
{
    SYSCALL_FUNC( NtGdiDdDDICreateAllocation2 );
}

NTSTATUS SYSCALL_API NtGdiDdDDICreateDCFromMemory( D3DKMT_CREATEDCFROMMEMORY *desc )
{
    SYSCALL_FUNC( NtGdiDdDDICreateDCFromMemory );
}

NTSTATUS SYSCALL_API NtGdiDdDDIDestroyAllocation( const D3DKMT_DESTROYALLOCATION *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIDestroyAllocation );
}

NTSTATUS SYSCALL_API NtGdiDdDDIDestroyAllocation2( const D3DKMT_DESTROYALLOCATION2 *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIDestroyAllocation2 );
}

NTSTATUS SYSCALL_API NtGdiDdDDICreateDevice( D3DKMT_CREATEDEVICE *desc )
{
    SYSCALL_FUNC( NtGdiDdDDICreateDevice );
}

NTSTATUS SYSCALL_API NtGdiDdDDICreateKeyedMutex( D3DKMT_CREATEKEYEDMUTEX *desc )
{
    SYSCALL_FUNC( NtGdiDdDDICreateKeyedMutex );
}

NTSTATUS SYSCALL_API NtGdiDdDDICreateKeyedMutex2( D3DKMT_CREATEKEYEDMUTEX2 *desc )
{
    SYSCALL_FUNC( NtGdiDdDDICreateKeyedMutex2 );
}

NTSTATUS SYSCALL_API NtGdiDdDDICreateSynchronizationObject( D3DKMT_CREATESYNCHRONIZATIONOBJECT *desc )
{
    SYSCALL_FUNC( NtGdiDdDDICreateSynchronizationObject );
}

NTSTATUS SYSCALL_API NtGdiDdDDICreateSynchronizationObject2( D3DKMT_CREATESYNCHRONIZATIONOBJECT2 *desc )
{
    SYSCALL_FUNC( NtGdiDdDDICreateSynchronizationObject2 );
}

NTSTATUS SYSCALL_API NtGdiDdDDIDestroyDCFromMemory( const D3DKMT_DESTROYDCFROMMEMORY *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIDestroyDCFromMemory );
}

NTSTATUS SYSCALL_API NtGdiDdDDIDestroyDevice( const D3DKMT_DESTROYDEVICE *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIDestroyDevice );
}

NTSTATUS SYSCALL_API NtGdiDdDDIDestroyKeyedMutex( const D3DKMT_DESTROYKEYEDMUTEX *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIDestroyKeyedMutex );
}

NTSTATUS SYSCALL_API NtGdiDdDDIDestroySynchronizationObject( const D3DKMT_DESTROYSYNCHRONIZATIONOBJECT *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIDestroySynchronizationObject );
}

NTSTATUS SYSCALL_API NtGdiDdDDIEnumAdapters( D3DKMT_ENUMADAPTERS *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIEnumAdapters );
}

NTSTATUS SYSCALL_API NtGdiDdDDIEnumAdapters2( D3DKMT_ENUMADAPTERS2 *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIEnumAdapters2 );
}

NTSTATUS SYSCALL_API NtGdiDdDDIEscape( const D3DKMT_ESCAPE *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIEscape );
}

NTSTATUS SYSCALL_API NtGdiDdDDIOpenAdapterFromDeviceName( D3DKMT_OPENADAPTERFROMDEVICENAME *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIOpenAdapterFromDeviceName );
}

NTSTATUS SYSCALL_API NtGdiDdDDIOpenAdapterFromHdc( D3DKMT_OPENADAPTERFROMHDC *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIOpenAdapterFromHdc );
}

NTSTATUS SYSCALL_API NtGdiDdDDIOpenAdapterFromLuid( D3DKMT_OPENADAPTERFROMLUID *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIOpenAdapterFromLuid );
}

NTSTATUS SYSCALL_API NtGdiDdDDIOpenKeyedMutex( D3DKMT_OPENKEYEDMUTEX *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIOpenKeyedMutex );
}

NTSTATUS SYSCALL_API NtGdiDdDDIOpenKeyedMutex2( D3DKMT_OPENKEYEDMUTEX2 *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIOpenKeyedMutex2 );
}

NTSTATUS SYSCALL_API NtGdiDdDDIOpenKeyedMutexFromNtHandle( D3DKMT_OPENKEYEDMUTEXFROMNTHANDLE *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIOpenKeyedMutexFromNtHandle );
}

NTSTATUS SYSCALL_API NtGdiDdDDIOpenResource( D3DKMT_OPENRESOURCE *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIOpenResource );
}

NTSTATUS SYSCALL_API NtGdiDdDDIOpenResource2( D3DKMT_OPENRESOURCE *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIOpenResource2 );
}

NTSTATUS SYSCALL_API NtGdiDdDDIOpenResourceFromNtHandle( D3DKMT_OPENRESOURCEFROMNTHANDLE *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIOpenResourceFromNtHandle );
}

NTSTATUS SYSCALL_API NtGdiDdDDIOpenSyncObjectFromNtHandle( D3DKMT_OPENSYNCOBJECTFROMNTHANDLE *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIOpenSyncObjectFromNtHandle );
}

NTSTATUS SYSCALL_API NtGdiDdDDIOpenSyncObjectFromNtHandle2( D3DKMT_OPENSYNCOBJECTFROMNTHANDLE2 *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIOpenSyncObjectFromNtHandle2 );
}

NTSTATUS SYSCALL_API NtGdiDdDDIOpenSyncObjectNtHandleFromName( D3DKMT_OPENSYNCOBJECTNTHANDLEFROMNAME *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIOpenSyncObjectNtHandleFromName );
}

NTSTATUS SYSCALL_API NtGdiDdDDIOpenSynchronizationObject( D3DKMT_OPENSYNCHRONIZATIONOBJECT *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIOpenSynchronizationObject );
}

NTSTATUS SYSCALL_API NtGdiDdDDIQueryAdapterInfo( D3DKMT_QUERYADAPTERINFO *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIQueryAdapterInfo );
}

NTSTATUS SYSCALL_API NtGdiDdDDIQueryResourceInfo( D3DKMT_QUERYRESOURCEINFO *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIQueryResourceInfo );
}

NTSTATUS SYSCALL_API NtGdiDdDDIQueryResourceInfoFromNtHandle( D3DKMT_QUERYRESOURCEINFOFROMNTHANDLE *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIQueryResourceInfoFromNtHandle );
}

NTSTATUS SYSCALL_API NtGdiDdDDIQueryStatistics( D3DKMT_QUERYSTATISTICS *stats )
{
    SYSCALL_FUNC( NtGdiDdDDIQueryStatistics );
}

NTSTATUS SYSCALL_API NtGdiDdDDIQueryVideoMemoryInfo( D3DKMT_QUERYVIDEOMEMORYINFO *desc )
{
    SYSCALL_FUNC( NtGdiDdDDIQueryVideoMemoryInfo );
}

NTSTATUS SYSCALL_API NtGdiDdDDISetQueuedLimit( D3DKMT_SETQUEUEDLIMIT *desc )
{
    SYSCALL_FUNC( NtGdiDdDDISetQueuedLimit );
}

NTSTATUS SYSCALL_API NtGdiDdDDISetVidPnSourceOwner( const D3DKMT_SETVIDPNSOURCEOWNER *desc )
{
    SYSCALL_FUNC( NtGdiDdDDISetVidPnSourceOwner );
}

NTSTATUS SYSCALL_API NtGdiDdDDIShareObjects( UINT count, const D3DKMT_HANDLE *handles, OBJECT_ATTRIBUTES *attr, UINT access, HANDLE *handle )
{
    SYSCALL_FUNC( NtGdiDdDDIShareObjects );
}

BOOL SYSCALL_API NtGdiDeleteClientObj( HGDIOBJ handle )
{
    SYSCALL_FUNC( NtGdiDeleteClientObj );
}

BOOL SYSCALL_API NtGdiDeleteObjectApp( HGDIOBJ obj )
{
    SYSCALL_FUNC( NtGdiDeleteObjectApp );
}

INT SYSCALL_API NtGdiDescribePixelFormat( HDC hdc, INT format, UINT size, PIXELFORMATDESCRIPTOR *descr )
{
    SYSCALL_FUNC( NtGdiDescribePixelFormat );
}

LONG SYSCALL_API NtGdiDoPalette( HGDIOBJ handle, WORD start, WORD count, void *entries,
                                 DWORD func, BOOL inbound )
{
    SYSCALL_FUNC( NtGdiDoPalette );
}

BOOL SYSCALL_API NtGdiDrawStream( HDC hdc, ULONG in, void *pvin )
{
    SYSCALL_FUNC( NtGdiDrawStream );
}

BOOL SYSCALL_API NtGdiEllipse( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    SYSCALL_FUNC( NtGdiEllipse );
}

INT SYSCALL_API NtGdiEndDoc( HDC hdc )
{
    SYSCALL_FUNC( NtGdiEndDoc );
}

INT SYSCALL_API NtGdiEndPage( HDC hdc )
{
    SYSCALL_FUNC( NtGdiEndPage );
}

BOOL SYSCALL_API NtGdiEndPath( HDC hdc )
{
    SYSCALL_FUNC( NtGdiEndPath );
}

BOOL SYSCALL_API NtGdiEnumFonts( HDC hdc, ULONG type, ULONG win32_compat, ULONG face_name_len,
                                 const WCHAR *face_name, ULONG charset, ULONG *count, void *buf )
{
    SYSCALL_FUNC( NtGdiEnumFonts );
}

BOOL SYSCALL_API NtGdiEqualRgn( HRGN hrgn1, HRGN hrgn2 )
{
    SYSCALL_FUNC( NtGdiEqualRgn );
}

INT SYSCALL_API NtGdiExcludeClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    SYSCALL_FUNC( NtGdiExcludeClipRect );
}

HPEN SYSCALL_API NtGdiExtCreatePen( DWORD style, DWORD width, ULONG brush_style, ULONG color,
                                    ULONG_PTR client_hatch, ULONG_PTR hatch, DWORD style_count,
                                    const DWORD *style_bits, ULONG dib_size, BOOL old_style,
                                    HBRUSH brush )
{
    SYSCALL_FUNC( NtGdiExtCreatePen );
}

HRGN SYSCALL_API NtGdiExtCreateRegion( const XFORM *xform, DWORD count, const RGNDATA *rgndata )
{
    SYSCALL_FUNC( NtGdiExtCreateRegion );
}

INT SYSCALL_API NtGdiExtEscape( HDC hdc, WCHAR *driver, int driver_id, INT escape, INT input_size,
                                const char *input, INT output_size, char *output )
{
    SYSCALL_FUNC( NtGdiExtEscape );
}

BOOL SYSCALL_API NtGdiExtFloodFill( HDC hdc, INT x, INT y, COLORREF color, UINT fill_type )
{
    SYSCALL_FUNC( NtGdiExtFloodFill );
}

INT SYSCALL_API NtGdiExtGetObjectW( HGDIOBJ handle, INT count, void *buffer )
{
    SYSCALL_FUNC( NtGdiExtGetObjectW );
}

INT SYSCALL_API NtGdiExtSelectClipRgn( HDC hdc, HRGN rgn, INT mode )
{
    SYSCALL_FUNC( NtGdiExtSelectClipRgn );
}

BOOL SYSCALL_API NtGdiExtTextOutW( HDC hdc, INT x, INT y, UINT flags, const RECT *lprect,
                                   const WCHAR *str, UINT count, const INT *lpDx, DWORD cp )
{
    SYSCALL_FUNC( NtGdiExtTextOutW );
}

BOOL SYSCALL_API NtGdiFillPath( HDC hdc )
{
    SYSCALL_FUNC( NtGdiFillPath );
}

BOOL SYSCALL_API NtGdiFillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush )
{
    SYSCALL_FUNC( NtGdiFillRgn );
}

BOOL SYSCALL_API NtGdiFlattenPath( HDC hdc )
{
    SYSCALL_FUNC( NtGdiFlattenPath );
}

BOOL SYSCALL_API NtGdiFlush(void)
{
    SYSCALL_FUNC( NtGdiFlush );
}

BOOL SYSCALL_API NtGdiFontIsLinked( HDC hdc )
{
    SYSCALL_FUNC( NtGdiFontIsLinked );
}

BOOL SYSCALL_API NtGdiFrameRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush, INT width, INT height )
{
    SYSCALL_FUNC( NtGdiFrameRgn );
}

BOOL SYSCALL_API NtGdiGetAndSetDCDword( HDC hdc, UINT method, DWORD value, DWORD *prev_value )
{
    SYSCALL_FUNC( NtGdiGetAndSetDCDword );
}

INT SYSCALL_API NtGdiGetAppClipBox( HDC hdc, RECT *rect )
{
    SYSCALL_FUNC( NtGdiGetAppClipBox );
}

LONG SYSCALL_API NtGdiGetBitmapBits( HBITMAP hbitmap, LONG count, LPVOID bits )
{
    SYSCALL_FUNC( NtGdiGetBitmapBits );
}

BOOL SYSCALL_API NtGdiGetBitmapDimension( HBITMAP hbitmap, LPSIZE size )
{
    SYSCALL_FUNC( NtGdiGetBitmapDimension );
}

UINT SYSCALL_API NtGdiGetBoundsRect( HDC hdc, RECT *rect, UINT flags )
{
    SYSCALL_FUNC( NtGdiGetBoundsRect );
}

BOOL SYSCALL_API NtGdiGetCharABCWidthsW( HDC hdc, UINT first, UINT last, WCHAR *chars,
                                         ULONG flags, void *buffer )
{
    SYSCALL_FUNC( NtGdiGetCharABCWidthsW );
}

BOOL SYSCALL_API NtGdiGetCharWidthInfo( HDC hdc, struct char_width_info *info )
{
    SYSCALL_FUNC( NtGdiGetCharWidthInfo );
}

BOOL SYSCALL_API NtGdiGetCharWidthW( HDC hdc, UINT first, UINT last, WCHAR *chars,
                                     ULONG flags, void *buf )
{
    SYSCALL_FUNC( NtGdiGetCharWidthW );
}

BOOL SYSCALL_API NtGdiGetColorAdjustment( HDC hdc, COLORADJUSTMENT *ca )
{
    SYSCALL_FUNC( NtGdiGetColorAdjustment );
}

BOOL SYSCALL_API NtGdiGetDCDword( HDC hdc, UINT method, DWORD *result )
{
    SYSCALL_FUNC( NtGdiGetDCDword );
}

HANDLE SYSCALL_API NtGdiGetDCObject( HDC hdc, UINT type )
{
    SYSCALL_FUNC( NtGdiGetDCObject );
}

BOOL SYSCALL_API NtGdiGetDCPoint( HDC hdc, UINT method, POINT *result )
{
    SYSCALL_FUNC( NtGdiGetDCPoint );
}

INT SYSCALL_API NtGdiGetDIBitsInternal( HDC hdc, HBITMAP hbitmap, UINT startscan, UINT lines,
                                        void *bits, BITMAPINFO *info, UINT coloruse,
                                        UINT max_bits, UINT max_info )
{
    SYSCALL_FUNC( NtGdiGetDIBitsInternal );
}

INT SYSCALL_API NtGdiGetDeviceCaps( HDC hdc, INT cap )
{
    SYSCALL_FUNC( NtGdiGetDeviceCaps );
}

BOOL SYSCALL_API NtGdiGetDeviceGammaRamp( HDC hdc, void *ptr )
{
    SYSCALL_FUNC( NtGdiGetDeviceGammaRamp );
}

DWORD SYSCALL_API NtGdiGetFontData( HDC hdc, DWORD table, DWORD offset, void *buffer, DWORD length )
{
    SYSCALL_FUNC( NtGdiGetFontData );
}

BOOL SYSCALL_API NtGdiGetFontFileData( DWORD instance_id, DWORD file_index, UINT64 *offset,
                                       void *buff, SIZE_T buff_size )
{
    SYSCALL_FUNC( NtGdiGetFontFileData );
}

BOOL SYSCALL_API NtGdiGetFontFileInfo( DWORD instance_id, DWORD file_index, struct font_fileinfo *info,
                                       SIZE_T size, SIZE_T *needed )
{
    SYSCALL_FUNC( NtGdiGetFontFileInfo );
}

DWORD SYSCALL_API NtGdiGetFontUnicodeRanges( HDC hdc, GLYPHSET *lpgs )
{
    SYSCALL_FUNC( NtGdiGetFontUnicodeRanges );
}

DWORD SYSCALL_API NtGdiGetGlyphIndicesW( HDC hdc, const WCHAR *str, INT count,
                                         WORD *indices, DWORD flags )
{
    SYSCALL_FUNC( NtGdiGetGlyphIndicesW );
}

DWORD SYSCALL_API NtGdiGetGlyphOutline( HDC hdc, UINT ch, UINT format, GLYPHMETRICS *metrics,
                                        DWORD size, void *buffer, const MAT2 *mat2,
                                        BOOL ignore_rotation )
{
    SYSCALL_FUNC( NtGdiGetGlyphOutline );
}

DWORD SYSCALL_API NtGdiGetKerningPairs( HDC hdc, DWORD count, KERNINGPAIR *kern_pair )
{
    SYSCALL_FUNC( NtGdiGetKerningPairs );
}

COLORREF SYSCALL_API NtGdiGetNearestColor( HDC hdc, COLORREF color )
{
    SYSCALL_FUNC( NtGdiGetNearestColor );
}

UINT SYSCALL_API NtGdiGetNearestPaletteIndex( HPALETTE hpalette, COLORREF color )
{
    SYSCALL_FUNC( NtGdiGetNearestPaletteIndex );
}

UINT SYSCALL_API NtGdiGetOutlineTextMetricsInternalW( HDC hdc, UINT cbData,
                                                      OUTLINETEXTMETRICW *lpOTM, ULONG opts )
{
    SYSCALL_FUNC( NtGdiGetOutlineTextMetricsInternalW );
}

INT SYSCALL_API NtGdiGetPath( HDC hdc, POINT *points, BYTE *types, INT size )
{
    SYSCALL_FUNC( NtGdiGetPath );
}

COLORREF SYSCALL_API NtGdiGetPixel( HDC hdc, INT x, INT y )
{
    SYSCALL_FUNC( NtGdiGetPixel );
}

INT SYSCALL_API NtGdiGetRandomRgn( HDC hDC, HRGN hRgn, INT iCode )
{
    SYSCALL_FUNC( NtGdiGetRandomRgn );
}

BOOL SYSCALL_API NtGdiGetRasterizerCaps( RASTERIZER_STATUS *status, UINT size )
{
    SYSCALL_FUNC( NtGdiGetRasterizerCaps );
}

BOOL SYSCALL_API NtGdiGetRealizationInfo( HDC hdc, struct font_realization_info *info )
{
    SYSCALL_FUNC( NtGdiGetRealizationInfo );
}

DWORD SYSCALL_API NtGdiGetRegionData( HRGN hrgn, DWORD count, RGNDATA *rgndata )
{
    SYSCALL_FUNC( NtGdiGetRegionData );
}

INT SYSCALL_API NtGdiGetRgnBox( HRGN hrgn, RECT *rect )
{
    SYSCALL_FUNC( NtGdiGetRgnBox );
}

DWORD SYSCALL_API NtGdiGetSpoolMessage( void *ptr1, DWORD data2, void *ptr3, DWORD data4 )
{
    SYSCALL_FUNC( NtGdiGetSpoolMessage );
}

UINT SYSCALL_API NtGdiGetSystemPaletteUse( HDC hdc )
{
    SYSCALL_FUNC( NtGdiGetSystemPaletteUse );
}

UINT SYSCALL_API NtGdiGetTextCharsetInfo( HDC hdc, FONTSIGNATURE *fs, DWORD flags )
{
    SYSCALL_FUNC( NtGdiGetTextCharsetInfo );
}

BOOL SYSCALL_API NtGdiGetTextExtentExW( HDC hdc, const WCHAR *str, INT count, INT max_ext,
                                        INT *nfit, INT *dxs, SIZE *size, UINT flags )
{
    SYSCALL_FUNC( NtGdiGetTextExtentExW );
}

INT SYSCALL_API NtGdiGetTextFaceW( HDC hdc, INT count, WCHAR *name, BOOL alias_name )
{
    SYSCALL_FUNC( NtGdiGetTextFaceW );
}

BOOL SYSCALL_API NtGdiGetTextMetricsW( HDC hdc, TEXTMETRICW *metrics, ULONG flags )
{
    SYSCALL_FUNC( NtGdiGetTextMetricsW );
}

BOOL SYSCALL_API NtGdiGetTransform( HDC hdc, DWORD which, XFORM *xform )
{
    SYSCALL_FUNC( NtGdiGetTransform );
}

BOOL SYSCALL_API NtGdiGradientFill( HDC hdc, TRIVERTEX *vert_array, ULONG nvert,
                                    void *grad_array, ULONG ngrad, ULONG mode )
{
    SYSCALL_FUNC( NtGdiGradientFill );
}

HFONT SYSCALL_API NtGdiHfontCreate( const void *logfont, ULONG size, ULONG type,
                                    ULONG flags, void *data )
{
    SYSCALL_FUNC( NtGdiHfontCreate );
}

BOOL SYSCALL_API NtGdiIcmBrushInfo( HDC hdc, HBRUSH handle, BITMAPINFO *info, void *bits,
                                    ULONG *bits_size, UINT *usage, BOOL *unk, UINT mode )
{
    SYSCALL_FUNC( NtGdiIcmBrushInfo );
}

DWORD SYSCALL_API NtGdiInitSpool(void)
{
    SYSCALL_FUNC( NtGdiInitSpool );
}

INT SYSCALL_API NtGdiIntersectClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    SYSCALL_FUNC( NtGdiIntersectClipRect );
}

BOOL SYSCALL_API NtGdiInvertRgn( HDC hdc, HRGN hrgn )
{
    SYSCALL_FUNC( NtGdiInvertRgn );
}

BOOL SYSCALL_API NtGdiLineTo( HDC hdc, INT x, INT y )
{
    SYSCALL_FUNC( NtGdiLineTo );
}

ULONG SYSCALL_API NtGdiMakeFontDir( DWORD embed, BYTE *buffer, UINT size, const WCHAR *path, UINT len )
{
    SYSCALL_FUNC( NtGdiMakeFontDir );
}

BOOL SYSCALL_API NtGdiMaskBlt( HDC hdcDest, INT nXDest, INT nYDest, INT nWidth, INT nHeight,
                               HDC hdcSrc, INT nXSrc, INT nYSrc, HBITMAP hbmMask,
                               INT xMask, INT yMask, DWORD dwRop, DWORD bk_color )
{
    SYSCALL_FUNC( NtGdiMaskBlt );
}

BOOL SYSCALL_API NtGdiModifyWorldTransform( HDC hdc, const XFORM *xform, DWORD mode )
{
    SYSCALL_FUNC( NtGdiModifyWorldTransform );
}

BOOL SYSCALL_API NtGdiMoveTo( HDC hdc, INT x, INT y, POINT *pt )
{
    SYSCALL_FUNC( NtGdiMoveTo );
}

INT SYSCALL_API NtGdiOffsetClipRgn( HDC hdc, INT x, INT y )
{
    SYSCALL_FUNC( NtGdiOffsetClipRgn );
}

INT SYSCALL_API NtGdiOffsetRgn( HRGN hrgn, INT x, INT y )
{
    SYSCALL_FUNC( NtGdiOffsetRgn );
}

HDC SYSCALL_API NtGdiOpenDCW( UNICODE_STRING *device, const DEVMODEW *devmode, UNICODE_STRING *output,
                              ULONG type, BOOL is_display, HANDLE hspool, DRIVER_INFO_2W *driver_info,
                              void *pdev )
{
    SYSCALL_FUNC( NtGdiOpenDCW );
}

BOOL SYSCALL_API NtGdiPatBlt( HDC hdc, INT left, INT top, INT width, INT height, DWORD rop )
{
    SYSCALL_FUNC( NtGdiPatBlt );
}

HRGN SYSCALL_API NtGdiPathToRegion( HDC hdc )
{
    SYSCALL_FUNC( NtGdiPathToRegion );
}

BOOL SYSCALL_API NtGdiPlgBlt( HDC hdcDest, const POINT *lpPoint, HDC hdcSrc, INT nXSrc, INT nYSrc,
                              INT nWidth, INT nHeight, HBITMAP hbmMask, INT xMask, INT yMask,
                              DWORD bk_color )
{
    SYSCALL_FUNC( NtGdiPlgBlt );
}

BOOL SYSCALL_API NtGdiPolyDraw( HDC hdc, const POINT *points, const BYTE *types, DWORD count )
{
    SYSCALL_FUNC( NtGdiPolyDraw );
}

ULONG SYSCALL_API NtGdiPolyPolyDraw( HDC hdc, const POINT *points, const ULONG *counts,
                                     DWORD count, UINT function )
{
    SYSCALL_FUNC( NtGdiPolyPolyDraw );
}

BOOL SYSCALL_API NtGdiPtInRegion( HRGN hrgn, INT x, INT y )
{
    SYSCALL_FUNC( NtGdiPtInRegion );
}

BOOL SYSCALL_API NtGdiPtVisible( HDC hdc, INT x, INT y )
{
    SYSCALL_FUNC( NtGdiPtVisible );
}

BOOL SYSCALL_API NtGdiRectInRegion( HRGN hrgn, const RECT *rect )
{
    SYSCALL_FUNC( NtGdiRectInRegion );
}

BOOL SYSCALL_API NtGdiRectVisible( HDC hdc, const RECT *rect )
{
    SYSCALL_FUNC( NtGdiRectVisible );
}

BOOL SYSCALL_API NtGdiRectangle( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    SYSCALL_FUNC( NtGdiRectangle );
}

BOOL SYSCALL_API NtGdiRemoveFontMemResourceEx( HANDLE handle )
{
    SYSCALL_FUNC( NtGdiRemoveFontMemResourceEx );
}

BOOL SYSCALL_API NtGdiRemoveFontResourceW( const WCHAR *str, ULONG size, ULONG files, DWORD flags,
                                           DWORD tid, void *dv )
{
    SYSCALL_FUNC( NtGdiRemoveFontResourceW );
}

BOOL SYSCALL_API NtGdiResetDC( HDC hdc, const DEVMODEW *devmode, BOOL *banding,
                               DRIVER_INFO_2W *driver_info, void *dev )
{
    SYSCALL_FUNC( NtGdiResetDC );
}

BOOL SYSCALL_API NtGdiResizePalette( HPALETTE hPal, UINT count )
{
    SYSCALL_FUNC( NtGdiResizePalette );
}

BOOL SYSCALL_API NtGdiRestoreDC( HDC hdc, INT level )
{
    SYSCALL_FUNC( NtGdiRestoreDC );
}

BOOL SYSCALL_API NtGdiRoundRect( HDC hdc, INT left, INT top, INT right,
                                 INT bottom, INT ell_width, INT ell_height )
{
    SYSCALL_FUNC( NtGdiRoundRect );
}

INT SYSCALL_API NtGdiSaveDC( HDC hdc )
{
    SYSCALL_FUNC( NtGdiSaveDC );
}

BOOL SYSCALL_API NtGdiScaleViewportExtEx( HDC hdc, INT x_num, INT x_denom,
                                          INT y_num, INT y_denom, SIZE *size )
{
    SYSCALL_FUNC( NtGdiScaleViewportExtEx );
}

BOOL SYSCALL_API NtGdiScaleWindowExtEx( HDC hdc, INT x_num, INT x_denom,
                                        INT y_num, INT y_denom, SIZE *size )
{
    SYSCALL_FUNC( NtGdiScaleWindowExtEx );
}

HGDIOBJ SYSCALL_API NtGdiSelectBitmap( HDC hdc, HGDIOBJ handle )
{
    SYSCALL_FUNC( NtGdiSelectBitmap );
}

HGDIOBJ SYSCALL_API NtGdiSelectBrush( HDC hdc, HGDIOBJ handle )
{
    SYSCALL_FUNC( NtGdiSelectBrush );
}

BOOL SYSCALL_API NtGdiSelectClipPath( HDC hdc, INT mode )
{
    SYSCALL_FUNC( NtGdiSelectClipPath );
}

HGDIOBJ SYSCALL_API NtGdiSelectFont( HDC hdc, HGDIOBJ handle )
{
    SYSCALL_FUNC( NtGdiSelectFont );
}

HGDIOBJ SYSCALL_API NtGdiSelectPen( HDC hdc, HGDIOBJ handle )
{
    SYSCALL_FUNC( NtGdiSelectPen );
}

LONG SYSCALL_API NtGdiSetBitmapBits( HBITMAP hbitmap, LONG count, LPCVOID bits )
{
    SYSCALL_FUNC( NtGdiSetBitmapBits );
}

BOOL SYSCALL_API NtGdiSetBitmapDimension( HBITMAP hbitmap, INT x, INT y, LPSIZE prevSize )
{
    SYSCALL_FUNC( NtGdiSetBitmapDimension );
}

UINT SYSCALL_API NtGdiSetBoundsRect( HDC hdc, const RECT *rect, UINT flags )
{
    SYSCALL_FUNC( NtGdiSetBoundsRect );
}

BOOL SYSCALL_API NtGdiSetBrushOrg( HDC hdc, INT x, INT y, POINT *oldorg )
{
    SYSCALL_FUNC( NtGdiSetBrushOrg );
}

BOOL SYSCALL_API NtGdiSetColorAdjustment( HDC hdc, const COLORADJUSTMENT *ca )
{
    SYSCALL_FUNC( NtGdiSetColorAdjustment );
}

INT SYSCALL_API NtGdiSetDIBitsToDeviceInternal( HDC hdc, INT xDest, INT yDest, DWORD cx,
                                                DWORD cy, INT xSrc, INT ySrc, UINT startscan,
                                                UINT lines, const void *bits, const BITMAPINFO *bmi,
                                                UINT coloruse, UINT max_bits, UINT max_info,
                                                BOOL xform_coords, HANDLE xform )
{
    SYSCALL_FUNC( NtGdiSetDIBitsToDeviceInternal );
}

BOOL SYSCALL_API NtGdiSetDeviceGammaRamp( HDC hdc, void *ptr )
{
    SYSCALL_FUNC( NtGdiSetDeviceGammaRamp );
}

DWORD SYSCALL_API NtGdiSetLayout( HDC hdc, LONG wox, DWORD layout )
{
    SYSCALL_FUNC( NtGdiSetLayout );
}

BOOL SYSCALL_API NtGdiSetMagicColors( HDC hdc, DWORD magic, ULONG index )
{
    SYSCALL_FUNC( NtGdiSetMagicColors );
}

INT SYSCALL_API NtGdiSetMetaRgn( HDC hdc )
{
    SYSCALL_FUNC( NtGdiSetMetaRgn );
}

COLORREF SYSCALL_API NtGdiSetPixel( HDC hdc, INT x, INT y, COLORREF color )
{
    SYSCALL_FUNC( NtGdiSetPixel );
}

BOOL SYSCALL_API NtGdiSetPixelFormat( HDC hdc, INT format )
{
    SYSCALL_FUNC( NtGdiSetPixelFormat );
}

BOOL SYSCALL_API NtGdiSetRectRgn( HRGN hrgn, INT left, INT top, INT right, INT bottom )
{
    SYSCALL_FUNC( NtGdiSetRectRgn );
}

UINT SYSCALL_API NtGdiSetSystemPaletteUse( HDC hdc, UINT use )
{
    SYSCALL_FUNC( NtGdiSetSystemPaletteUse );
}

BOOL SYSCALL_API NtGdiSetTextJustification( HDC hdc, INT extra, INT breaks )
{
    SYSCALL_FUNC( NtGdiSetTextJustification );
}

BOOL SYSCALL_API NtGdiSetVirtualResolution( HDC hdc, DWORD horz_res, DWORD vert_res,
                                            DWORD horz_size, DWORD vert_size )
{
    SYSCALL_FUNC( NtGdiSetVirtualResolution );
}

INT SYSCALL_API NtGdiStartDoc( HDC hdc, const DOCINFOW *doc, BOOL *banding, INT job )
{
    SYSCALL_FUNC( NtGdiStartDoc );
}

INT SYSCALL_API NtGdiStartPage( HDC hdc )
{
    SYSCALL_FUNC( NtGdiStartPage );
}

BOOL SYSCALL_API NtGdiStretchBlt( HDC hdcDst, INT xDst, INT yDst, INT widthDst, INT heightDst,
                                  HDC hdcSrc, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                                  DWORD rop, COLORREF bk_color )
{
    SYSCALL_FUNC( NtGdiStretchBlt );
}

INT SYSCALL_API NtGdiStretchDIBitsInternal( HDC hdc, INT xDst, INT yDst, INT widthDst, INT heightDst,
                                            INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                                            const void *bits, const BITMAPINFO *bmi, UINT coloruse,
                                            DWORD rop, UINT max_info, UINT max_bits, HANDLE xform )
{
    SYSCALL_FUNC( NtGdiStretchDIBitsInternal );
}

BOOL SYSCALL_API NtGdiStrokeAndFillPath( HDC hdc )
{
    SYSCALL_FUNC( NtGdiStrokeAndFillPath );
}

BOOL SYSCALL_API NtGdiStrokePath( HDC hdc )
{
    SYSCALL_FUNC( NtGdiStrokePath );
}

BOOL SYSCALL_API NtGdiSwapBuffers( HDC hdc )
{
    SYSCALL_FUNC( NtGdiSwapBuffers );
}

BOOL SYSCALL_API NtGdiTransformPoints( HDC hdc, const POINT *points_in, POINT *points_out,
                                       INT count, UINT mode )
{
    SYSCALL_FUNC( NtGdiTransformPoints );
}

BOOL SYSCALL_API NtGdiTransparentBlt( HDC hdcDest, int xDest, int yDest, int widthDest, int heightDest,
                                      HDC hdcSrc, int xSrc, int ySrc, int widthSrc, int heightSrc,
                                      UINT crTransparent )
{
    SYSCALL_FUNC( NtGdiTransparentBlt );
}

BOOL SYSCALL_API NtGdiUnrealizeObject( HGDIOBJ obj )
{
    SYSCALL_FUNC( NtGdiUnrealizeObject );
}

BOOL SYSCALL_API NtGdiUpdateColors( HDC hDC )
{
    SYSCALL_FUNC( NtGdiUpdateColors );
}

BOOL SYSCALL_API NtGdiWidenPath( HDC hdc )
{
    SYSCALL_FUNC( NtGdiWidenPath );
}

HKL SYSCALL_API NtUserActivateKeyboardLayout( HKL layout, UINT flags )
{
    SYSCALL_FUNC( NtUserActivateKeyboardLayout );
}

BOOL SYSCALL_API NtUserAddClipboardFormatListener( HWND hwnd )
{
    SYSCALL_FUNC( NtUserAddClipboardFormatListener );
}

UINT SYSCALL_API NtUserArrangeIconicWindows( HWND parent )
{
    SYSCALL_FUNC( NtUserArrangeIconicWindows );
}

UINT SYSCALL_API NtUserAssociateInputContext( HWND hwnd, HIMC ctx, ULONG flags )
{
    SYSCALL_FUNC( NtUserAssociateInputContext );
}

BOOL SYSCALL_API NtUserAttachThreadInput( DWORD from, DWORD to, BOOL attach )
{
    SYSCALL_FUNC( NtUserAttachThreadInput );
}

HDWP SYSCALL_API NtUserBeginDeferWindowPos( INT count )
{
    SYSCALL_FUNC( NtUserBeginDeferWindowPos );
}

HDC SYSCALL_API NtUserBeginPaint( HWND hwnd, PAINTSTRUCT *ps )
{
    SYSCALL_FUNC( NtUserBeginPaint );
}

NTSTATUS SYSCALL_API NtUserBuildHimcList( UINT thread_id, UINT count, HIMC *buffer, UINT *size )
{
    SYSCALL_FUNC( NtUserBuildHimcList );
}

NTSTATUS SYSCALL_API NtUserBuildHwndList( HDESK desktop, HWND hwnd, BOOL children, BOOL non_immersive,
                                          ULONG thread_id, ULONG count, HWND *buffer, ULONG *size )
{
    SYSCALL_FUNC( NtUserBuildHwndList );
}

NTSTATUS SYSCALL_API NtUserBuildNameList( HWINSTA winsta, ULONG size, struct ntuser_name_list *buffer, ULONG *ret_size )
{
    SYSCALL_FUNC( NtUserBuildNameList );
}

NTSTATUS SYSCALL_API NtUserBuildPropList( HWND hwnd, ULONG count, struct ntuser_property_list *buffer, ULONG *ret_count )
{
    SYSCALL_FUNC( NtUserBuildPropList );
}

ULONG_PTR SYSCALL_API NtUserCallHwnd( HWND hwnd, DWORD code )
{
    SYSCALL_FUNC( NtUserCallHwnd );
}

ULONG_PTR SYSCALL_API NtUserCallHwndParam( HWND hwnd, DWORD_PTR param, DWORD code )
{
    SYSCALL_FUNC( NtUserCallHwndParam );
}

BOOL SYSCALL_API NtUserCallMsgFilter( MSG *msg, INT code )
{
    SYSCALL_FUNC( NtUserCallMsgFilter );
}

LRESULT SYSCALL_API NtUserCallNextHookEx( HHOOK hhook, INT code, WPARAM wparam, LPARAM lparam )
{
    SYSCALL_FUNC( NtUserCallNextHookEx );
}

ULONG_PTR SYSCALL_API NtUserCallNoParam( ULONG code )
{
    SYSCALL_FUNC( NtUserCallNoParam );
}

ULONG_PTR SYSCALL_API NtUserCallOneParam( ULONG_PTR arg, ULONG code )
{
    SYSCALL_FUNC( NtUserCallOneParam );
}

ULONG_PTR SYSCALL_API NtUserCallTwoParam( ULONG_PTR arg1, ULONG_PTR arg2, ULONG code )
{
    SYSCALL_FUNC( NtUserCallTwoParam );
}

BOOL SYSCALL_API NtUserChangeClipboardChain( HWND hwnd, HWND next )
{
    SYSCALL_FUNC( NtUserChangeClipboardChain );
}

LONG SYSCALL_API NtUserChangeDisplaySettings( UNICODE_STRING *devname, DEVMODEW *devmode, HWND hwnd,
                                              DWORD flags, void *lparam )
{
    SYSCALL_FUNC( NtUserChangeDisplaySettings );
}

DWORD SYSCALL_API NtUserCheckMenuItem( HMENU handle, UINT id, UINT flags )
{
    SYSCALL_FUNC( NtUserCheckMenuItem );
}

HWND SYSCALL_API NtUserChildWindowFromPointEx( HWND parent, LONG x, LONG y, UINT flags )
{
    SYSCALL_FUNC( NtUserChildWindowFromPointEx );
}

BOOL SYSCALL_API NtUserClipCursor( const RECT *rect )
{
    SYSCALL_FUNC( NtUserClipCursor );
}

BOOL SYSCALL_API NtUserCloseClipboard(void)
{
    SYSCALL_FUNC( NtUserCloseClipboard );
}

BOOL SYSCALL_API NtUserCloseDesktop( HDESK handle )
{
    SYSCALL_FUNC( NtUserCloseDesktop );
}

BOOL SYSCALL_API NtUserCloseWindowStation( HWINSTA handle )
{
    SYSCALL_FUNC( NtUserCloseWindowStation );
}

INT SYSCALL_API NtUserCopyAcceleratorTable( HACCEL src, ACCEL *dst, INT count )
{
    SYSCALL_FUNC( NtUserCopyAcceleratorTable );
}

INT SYSCALL_API NtUserCountClipboardFormats(void)
{
    SYSCALL_FUNC( NtUserCountClipboardFormats );
}

HACCEL SYSCALL_API NtUserCreateAcceleratorTable( ACCEL *table, INT count )
{
    SYSCALL_FUNC( NtUserCreateAcceleratorTable );
}

BOOL SYSCALL_API NtUserCreateCaret( HWND hwnd, HBITMAP bitmap, int width, int height )
{
    SYSCALL_FUNC( NtUserCreateCaret );
}

HDESK SYSCALL_API NtUserCreateDesktopEx( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *device,
                                         DEVMODEW *devmode, DWORD flags, ACCESS_MASK access,
                                         ULONG heap_size )
{
    SYSCALL_FUNC( NtUserCreateDesktopEx );
}

HIMC SYSCALL_API NtUserCreateInputContext( UINT_PTR client_ptr )
{
    SYSCALL_FUNC( NtUserCreateInputContext );
}

HMENU SYSCALL_API NtUserCreateMenu(void)
{
    SYSCALL_FUNC( NtUserCreateMenu );
}

HMENU SYSCALL_API NtUserCreatePopupMenu(void)
{
    SYSCALL_FUNC( NtUserCreatePopupMenu );
}

HWND SYSCALL_API NtUserCreateWindowEx( DWORD ex_style, UNICODE_STRING *class_name,
                                       UNICODE_STRING *version, UNICODE_STRING *window_name,
                                       DWORD style, INT x, INT y, INT cx, INT cy,
                                       HWND parent, HMENU menu, HINSTANCE instance, void *params,
                                       DWORD flags, HINSTANCE client_instance, DWORD unk, BOOL ansi )
{
    SYSCALL_FUNC( NtUserCreateWindowEx );
}

HWINSTA SYSCALL_API NtUserCreateWindowStation( OBJECT_ATTRIBUTES *attr, ACCESS_MASK access, ULONG arg3,
                                               ULONG arg4, ULONG arg5, ULONG arg6, ULONG arg7 )
{
    SYSCALL_FUNC( NtUserCreateWindowStation );
}

HDWP SYSCALL_API NtUserDeferWindowPosAndBand( HDWP hdwp, HWND hwnd, HWND after,
                                              INT x, INT y, INT cx, INT cy,
                                              UINT flags, UINT unk1, UINT unk2 )
{
    SYSCALL_FUNC( NtUserDeferWindowPosAndBand );
}

BOOL SYSCALL_API NtUserDeleteMenu( HMENU handle, UINT id, UINT flags )
{
    SYSCALL_FUNC( NtUserDeleteMenu );
}

BOOL SYSCALL_API NtUserDestroyAcceleratorTable( HACCEL handle )
{
    SYSCALL_FUNC( NtUserDestroyAcceleratorTable );
}

BOOL SYSCALL_API NtUserDestroyCaret(void)
{
    SYSCALL_FUNC( NtUserDestroyCaret );
}

BOOL SYSCALL_API NtUserDestroyCursor( HCURSOR cursor, ULONG arg )
{
    SYSCALL_FUNC( NtUserDestroyCursor );
}

BOOL SYSCALL_API NtUserDestroyInputContext( HIMC handle )
{
    SYSCALL_FUNC( NtUserDestroyInputContext );
}

BOOL SYSCALL_API NtUserDestroyMenu( HMENU handle )
{
    SYSCALL_FUNC( NtUserDestroyMenu );
}

BOOL SYSCALL_API NtUserDestroyWindow( HWND hwnd )
{
    SYSCALL_FUNC( NtUserDestroyWindow );
}

BOOL SYSCALL_API NtUserDisableThreadIme( DWORD thread_id )
{
    SYSCALL_FUNC( NtUserDisableThreadIme );
}

LRESULT SYSCALL_API NtUserDispatchMessage( const MSG *msg )
{
    SYSCALL_FUNC( NtUserDispatchMessage );
}

NTSTATUS SYSCALL_API NtUserDisplayConfigGetDeviceInfo( DISPLAYCONFIG_DEVICE_INFO_HEADER *packet )
{
    SYSCALL_FUNC( NtUserDisplayConfigGetDeviceInfo );
}

BOOL SYSCALL_API NtUserDragDetect( HWND hwnd, int x, int y )
{
    SYSCALL_FUNC( NtUserDragDetect );
}

DWORD SYSCALL_API NtUserDragObject( HWND parent, HWND hwnd, UINT fmt, ULONG_PTR data, HCURSOR cursor )
{
    SYSCALL_FUNC( NtUserDragObject );
}

BOOL SYSCALL_API NtUserDrawCaptionTemp( HWND hwnd, HDC hdc, const RECT *rect, HFONT font,
                                        HICON icon, const WCHAR *str, UINT flags )
{
    SYSCALL_FUNC( NtUserDrawCaptionTemp );
}

BOOL SYSCALL_API NtUserDrawIconEx( HDC hdc, INT x0, INT y0, HICON icon, INT width,
                                   INT height, UINT step, HBRUSH brush, UINT flags )
{
    SYSCALL_FUNC( NtUserDrawIconEx );
}

BOOL SYSCALL_API NtUserDrawMenuBar( HWND hwnd )
{
    SYSCALL_FUNC( NtUserDrawMenuBar );
}

DWORD SYSCALL_API NtUserDrawMenuBarTemp( HWND hwnd, HDC hdc, RECT *rect, HMENU handle, HFONT font )
{
    SYSCALL_FUNC( NtUserDrawMenuBarTemp );
}

BOOL SYSCALL_API NtUserEmptyClipboard(void)
{
    SYSCALL_FUNC( NtUserEmptyClipboard );
}

BOOL SYSCALL_API NtUserEnableMenuItem( HMENU handle, UINT id, UINT flags )
{
    SYSCALL_FUNC( NtUserEnableMenuItem );
}

BOOL SYSCALL_API NtUserEnableMouseInPointer( BOOL enable )
{
    SYSCALL_FUNC( NtUserEnableMouseInPointer );
}

BOOL SYSCALL_API NtUserEnableMouseInPointerForThread(void)
{
    SYSCALL_FUNC( NtUserEnableMouseInPointerForThread );
}

BOOL SYSCALL_API NtUserEnableScrollBar( HWND hwnd, UINT bar, UINT flags )
{
    SYSCALL_FUNC( NtUserEnableScrollBar );
}

BOOL SYSCALL_API NtUserEnableWindow( HWND hwnd, BOOL enable )
{
    SYSCALL_FUNC( NtUserEnableWindow );
}

BOOL SYSCALL_API NtUserEndDeferWindowPosEx( HDWP hdwp, BOOL async )
{
    SYSCALL_FUNC( NtUserEndDeferWindowPosEx );
}

BOOL SYSCALL_API NtUserEndMenu(void)
{
    SYSCALL_FUNC( NtUserEndMenu );
}

BOOL SYSCALL_API NtUserEndPaint( HWND hwnd, const PAINTSTRUCT *ps )
{
    SYSCALL_FUNC( NtUserEndPaint );
}

UINT SYSCALL_API NtUserEnumClipboardFormats( UINT format )
{
    SYSCALL_FUNC( NtUserEnumClipboardFormats );
}

NTSTATUS SYSCALL_API NtUserEnumDisplayDevices( UNICODE_STRING *device, DWORD index,
                                               DISPLAY_DEVICEW *info, DWORD flags )
{
    SYSCALL_FUNC( NtUserEnumDisplayDevices );
}

BOOL SYSCALL_API NtUserEnumDisplayMonitors( HDC hdc, RECT *rect, MONITORENUMPROC proc, LPARAM lparam )
{
    SYSCALL_FUNC( NtUserEnumDisplayMonitors );
}

BOOL SYSCALL_API NtUserEnumDisplaySettings( UNICODE_STRING *device, DWORD index, DEVMODEW *devmode, DWORD flags )
{
    SYSCALL_FUNC( NtUserEnumDisplaySettings );
}

INT SYSCALL_API NtUserExcludeUpdateRgn( HDC hdc, HWND hwnd )
{
    SYSCALL_FUNC( NtUserExcludeUpdateRgn );
}

HICON SYSCALL_API NtUserFindExistingCursorIcon( UNICODE_STRING *module, UNICODE_STRING *res_name, void *desc )
{
    SYSCALL_FUNC( NtUserFindExistingCursorIcon );
}

HWND SYSCALL_API NtUserFindWindowEx( HWND parent, HWND child, UNICODE_STRING *class, UNICODE_STRING *title,
                                     ULONG unk )
{
    SYSCALL_FUNC( NtUserFindWindowEx );
}

BOOL SYSCALL_API NtUserFlashWindowEx( FLASHWINFO *info )
{
    SYSCALL_FUNC( NtUserFlashWindowEx );
}

HWND SYSCALL_API NtUserGetAncestor( HWND hwnd, UINT type )
{
    SYSCALL_FUNC( NtUserGetAncestor );
}

SHORT SYSCALL_API NtUserGetAsyncKeyState( INT key )
{
    SYSCALL_FUNC( NtUserGetAsyncKeyState );
}

ULONG SYSCALL_API NtUserGetAtomName( ATOM atom, UNICODE_STRING *name )
{
    SYSCALL_FUNC( NtUserGetAtomName );
}

UINT SYSCALL_API NtUserGetCaretBlinkTime(void)
{
    SYSCALL_FUNC( NtUserGetCaretBlinkTime );
}

BOOL SYSCALL_API NtUserGetCaretPos( POINT *pt )
{
    SYSCALL_FUNC( NtUserGetCaretPos );
}

ATOM SYSCALL_API NtUserGetClassInfoEx( HINSTANCE instance, UNICODE_STRING *name, WNDCLASSEXW *wc,
                                       struct client_menu_name *menu_name, BOOL ansi )
{
    SYSCALL_FUNC( NtUserGetClassInfoEx );
}

INT SYSCALL_API NtUserGetClassName( HWND hwnd, BOOL real, UNICODE_STRING *name )
{
    SYSCALL_FUNC( NtUserGetClassName );
}

BOOL SYSCALL_API NtUserGetClipCursor( RECT *rect )
{
    SYSCALL_FUNC( NtUserGetClipCursor );
}

HANDLE SYSCALL_API NtUserGetClipboardData( UINT format, struct get_clipboard_params *params )
{
    SYSCALL_FUNC( NtUserGetClipboardData );
}

INT SYSCALL_API NtUserGetClipboardFormatName( UINT format, WCHAR *buffer, INT maxlen )
{
    SYSCALL_FUNC( NtUserGetClipboardFormatName );
}

HWND SYSCALL_API NtUserGetClipboardOwner(void)
{
    SYSCALL_FUNC( NtUserGetClipboardOwner );
}

DWORD SYSCALL_API NtUserGetClipboardSequenceNumber(void)
{
    SYSCALL_FUNC( NtUserGetClipboardSequenceNumber );
}

HWND SYSCALL_API NtUserGetClipboardViewer(void)
{
    SYSCALL_FUNC( NtUserGetClipboardViewer );
}

BOOL SYSCALL_API NtUserGetCurrentInputMessageSource( INPUT_MESSAGE_SOURCE *source )
{
    SYSCALL_FUNC( NtUserGetCurrentInputMessageSource );
}

HCURSOR SYSCALL_API NtUserGetCursor(void)
{
    SYSCALL_FUNC( NtUserGetCursor );
}

HCURSOR SYSCALL_API NtUserGetCursorFrameInfo( HCURSOR cursor, DWORD istep, DWORD *rate_jiffies,
                                              DWORD *num_steps )
{
    SYSCALL_FUNC( NtUserGetCursorFrameInfo );
}

BOOL SYSCALL_API NtUserGetCursorInfo( CURSORINFO *info )
{
    SYSCALL_FUNC( NtUserGetCursorInfo );
}

HDC SYSCALL_API NtUserGetDC( HWND hwnd )
{
    SYSCALL_FUNC( NtUserGetDC );
}

HDC SYSCALL_API NtUserGetDCEx( HWND hwnd, HRGN clip_rgn, DWORD flags )
{
    SYSCALL_FUNC( NtUserGetDCEx );
}

LONG SYSCALL_API NtUserGetDisplayConfigBufferSizes( UINT32 flags, UINT32 *num_path_info,
                                                    UINT32 *num_mode_info )
{
    SYSCALL_FUNC( NtUserGetDisplayConfigBufferSizes );
}

UINT SYSCALL_API NtUserGetDoubleClickTime(void)
{
    SYSCALL_FUNC( NtUserGetDoubleClickTime );
}

BOOL SYSCALL_API NtUserGetDpiForMonitor( HMONITOR monitor, UINT type, UINT *x, UINT *y )
{
    SYSCALL_FUNC( NtUserGetDpiForMonitor );
}

HWND SYSCALL_API NtUserGetForegroundWindow(void)
{
    SYSCALL_FUNC( NtUserGetForegroundWindow );
}

BOOL SYSCALL_API NtUserGetGUIThreadInfo( DWORD id, GUITHREADINFO *info )
{
    SYSCALL_FUNC( NtUserGetGUIThreadInfo );
}

BOOL SYSCALL_API NtUserGetIconInfo( HICON icon, ICONINFO *info, UNICODE_STRING *module,
                                    UNICODE_STRING *res_name, DWORD *bpp, LONG unk )
{
    SYSCALL_FUNC( NtUserGetIconInfo );
}

BOOL SYSCALL_API NtUserGetIconSize( HICON handle, UINT step, LONG *width, LONG *height )
{
    SYSCALL_FUNC( NtUserGetIconSize );
}

UINT SYSCALL_API NtUserGetInternalWindowPos( HWND hwnd, RECT *rect, POINT *pt )
{
    SYSCALL_FUNC( NtUserGetInternalWindowPos );
}

INT SYSCALL_API NtUserGetKeyNameText( LONG lparam, WCHAR *buffer, INT size )
{
    SYSCALL_FUNC( NtUserGetKeyNameText );
}

SHORT SYSCALL_API NtUserGetKeyState( INT vkey )
{
    SYSCALL_FUNC( NtUserGetKeyState );
}

HKL SYSCALL_API NtUserGetKeyboardLayout( DWORD thread_id )
{
    SYSCALL_FUNC( NtUserGetKeyboardLayout );
}

UINT SYSCALL_API NtUserGetKeyboardLayoutList( INT size, HKL *layouts )
{
    SYSCALL_FUNC( NtUserGetKeyboardLayoutList );
}

BOOL SYSCALL_API NtUserGetKeyboardLayoutName( WCHAR *name )
{
    SYSCALL_FUNC( NtUserGetKeyboardLayoutName );
}

BOOL SYSCALL_API NtUserGetKeyboardState( BYTE *state )
{
    SYSCALL_FUNC( NtUserGetKeyboardState );
}

BOOL SYSCALL_API NtUserGetLayeredWindowAttributes( HWND hwnd, COLORREF *key, BYTE *alpha, DWORD *flags )
{
    SYSCALL_FUNC( NtUserGetLayeredWindowAttributes );
}

BOOL SYSCALL_API NtUserGetMenuBarInfo( HWND hwnd, LONG id, LONG item, MENUBARINFO *info )
{
    SYSCALL_FUNC( NtUserGetMenuBarInfo );
}

BOOL SYSCALL_API NtUserGetMenuItemRect( HWND hwnd, HMENU handle, UINT item, RECT *rect )
{
    SYSCALL_FUNC( NtUserGetMenuItemRect );
}

BOOL SYSCALL_API NtUserGetMessage( MSG *msg, HWND hwnd, UINT first, UINT last )
{
    SYSCALL_FUNC( NtUserGetMessage );
}

int SYSCALL_API NtUserGetMouseMovePointsEx( UINT size, MOUSEMOVEPOINT *ptin, MOUSEMOVEPOINT *ptout,
                                            int count, DWORD resolution )
{
    SYSCALL_FUNC( NtUserGetMouseMovePointsEx );
}

BOOL SYSCALL_API NtUserGetObjectInformation( HANDLE handle, INT index, void *info,
                                             DWORD len, DWORD *needed )
{
    SYSCALL_FUNC( NtUserGetObjectInformation );
}

HWND SYSCALL_API NtUserGetOpenClipboardWindow(void)
{
    SYSCALL_FUNC( NtUserGetOpenClipboardWindow );
}

BOOL SYSCALL_API NtUserGetPointerInfoList( UINT32 id, POINTER_INPUT_TYPE type, UINT_PTR unk0, UINT_PTR unk1, SIZE_T size,
                                           UINT32 *entry_count, UINT32 *pointer_count, void *pointer_info )
{
    SYSCALL_FUNC( NtUserGetPointerInfoList );
}

INT SYSCALL_API NtUserGetPriorityClipboardFormat( UINT *list, INT count )
{
    SYSCALL_FUNC( NtUserGetPriorityClipboardFormat );
}

BOOL SYSCALL_API NtUserGetProcessDefaultLayout( ULONG *layout )
{
    SYSCALL_FUNC( NtUserGetProcessDefaultLayout );
}

ULONG SYSCALL_API NtUserGetProcessDpiAwarenessContext( HANDLE process )
{
    SYSCALL_FUNC( NtUserGetProcessDpiAwarenessContext );
}

HWINSTA SYSCALL_API NtUserGetProcessWindowStation(void)
{
    SYSCALL_FUNC( NtUserGetProcessWindowStation );
}

HANDLE SYSCALL_API NtUserGetProp( HWND hwnd, const WCHAR *str )
{
    SYSCALL_FUNC( NtUserGetProp );
}

DWORD SYSCALL_API NtUserGetQueueStatus( UINT flags )
{
    SYSCALL_FUNC( NtUserGetQueueStatus );
}

UINT SYSCALL_API NtUserGetRawInputBuffer( RAWINPUT *data, UINT *data_size, UINT header_size )
{
    SYSCALL_FUNC( NtUserGetRawInputBuffer );
}

UINT SYSCALL_API NtUserGetRawInputData( HRAWINPUT rawinput, UINT command, void *data, UINT *data_size, UINT header_size )
{
    SYSCALL_FUNC( NtUserGetRawInputData );
}

UINT SYSCALL_API NtUserGetRawInputDeviceInfo( HANDLE handle, UINT command, void *data, UINT *data_size )
{
    SYSCALL_FUNC( NtUserGetRawInputDeviceInfo );
}

UINT SYSCALL_API NtUserGetRawInputDeviceList( RAWINPUTDEVICELIST *device_list, UINT *device_count, UINT size )
{
    SYSCALL_FUNC( NtUserGetRawInputDeviceList );
}

UINT SYSCALL_API NtUserGetRegisteredRawInputDevices( RAWINPUTDEVICE *devices, UINT *device_count, UINT device_size )
{
    SYSCALL_FUNC( NtUserGetRegisteredRawInputDevices );
}

BOOL SYSCALL_API NtUserGetScrollBarInfo( HWND hwnd, LONG id, SCROLLBARINFO *info )
{
    SYSCALL_FUNC( NtUserGetScrollBarInfo );
}

ULONG SYSCALL_API NtUserGetSystemDpiForProcess( HANDLE process )
{
    SYSCALL_FUNC( NtUserGetSystemDpiForProcess );
}

HMENU SYSCALL_API NtUserGetSystemMenu( HWND hwnd, BOOL revert )
{
    SYSCALL_FUNC( NtUserGetSystemMenu );
}

HDESK SYSCALL_API NtUserGetThreadDesktop( DWORD thread )
{
    SYSCALL_FUNC( NtUserGetThreadDesktop );
}

ULONG_PTR SYSCALL_API NtUserGetThreadState( USERTHREADSTATECLASS cls )
{
    SYSCALL_FUNC( NtUserGetThreadState );
}

BOOL SYSCALL_API NtUserGetTitleBarInfo( HWND hwnd, TITLEBARINFO *info )
{
    SYSCALL_FUNC( NtUserGetTitleBarInfo );
}

BOOL SYSCALL_API NtUserGetUpdateRect( HWND hwnd, RECT *rect, BOOL erase )
{
    SYSCALL_FUNC( NtUserGetUpdateRect );
}

INT SYSCALL_API NtUserGetUpdateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    SYSCALL_FUNC( NtUserGetUpdateRgn );
}

BOOL SYSCALL_API NtUserGetUpdatedClipboardFormats( UINT *formats, UINT size, UINT *out_size )
{
    SYSCALL_FUNC( NtUserGetUpdatedClipboardFormats );
}

DWORD SYSCALL_API NtUserGetWindowContextHelpId( HWND hwnd )
{
    SYSCALL_FUNC( NtUserGetWindowContextHelpId );
}

HDC SYSCALL_API NtUserGetWindowDC( HWND hwnd )
{
    SYSCALL_FUNC( NtUserGetWindowDC );
}

BOOL SYSCALL_API NtUserGetWindowPlacement( HWND hwnd, WINDOWPLACEMENT *placement )
{
    SYSCALL_FUNC( NtUserGetWindowPlacement );
}

int SYSCALL_API NtUserGetWindowRgnEx( HWND hwnd, HRGN hrgn, UINT unk )
{
    SYSCALL_FUNC( NtUserGetWindowRgnEx );
}

BOOL SYSCALL_API NtUserHideCaret( HWND hwnd )
{
    SYSCALL_FUNC( NtUserHideCaret );
}

BOOL SYSCALL_API NtUserHiliteMenuItem( HWND hwnd, HMENU handle, UINT item, UINT hilite )
{
    SYSCALL_FUNC( NtUserHiliteMenuItem );
}

NTSTATUS SYSCALL_API NtUserInitializeClientPfnArrays( const ntuser_client_func_ptr *client_procsA,
                                                      const ntuser_client_func_ptr *client_procsW,
                                                      const ntuser_client_func_ptr *client_workers, HINSTANCE user_module )
{
    SYSCALL_FUNC( NtUserInitializeClientPfnArrays );
}

HICON SYSCALL_API NtUserInternalGetWindowIcon( HWND hwnd, UINT type )
{
    SYSCALL_FUNC( NtUserInternalGetWindowIcon );
}

INT SYSCALL_API NtUserInternalGetWindowText( HWND hwnd, WCHAR *text, INT count )
{
    SYSCALL_FUNC( NtUserInternalGetWindowText );
}

BOOL SYSCALL_API NtUserInvalidateRect( HWND hwnd, const RECT *rect, BOOL erase )
{
    SYSCALL_FUNC( NtUserInvalidateRect );
}

BOOL SYSCALL_API NtUserInvalidateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    SYSCALL_FUNC( NtUserInvalidateRgn );
}

BOOL SYSCALL_API NtUserIsChildWindowDpiMessageEnabled( HWND hwnd )
{
    SYSCALL_FUNC( NtUserIsChildWindowDpiMessageEnabled );
}

BOOL SYSCALL_API NtUserIsClipboardFormatAvailable( UINT format )
{
    SYSCALL_FUNC( NtUserIsClipboardFormatAvailable );
}

BOOL SYSCALL_API NtUserIsMouseInPointerEnabled(void)
{
    SYSCALL_FUNC( NtUserIsMouseInPointerEnabled );
}

BOOL SYSCALL_API NtUserKillTimer( HWND hwnd, UINT_PTR id )
{
    SYSCALL_FUNC( NtUserKillTimer );
}

BOOL SYSCALL_API NtUserLockWindowUpdate( HWND hwnd )
{
    SYSCALL_FUNC( NtUserLockWindowUpdate );
}

BOOL SYSCALL_API NtUserLogicalToPerMonitorDPIPhysicalPoint( HWND hwnd, POINT *pt )
{
    SYSCALL_FUNC( NtUserLogicalToPerMonitorDPIPhysicalPoint );
}

UINT SYSCALL_API NtUserMapVirtualKeyEx( UINT code, UINT type, HKL layout )
{
    SYSCALL_FUNC( NtUserMapVirtualKeyEx );
}

INT SYSCALL_API NtUserMenuItemFromPoint( HWND hwnd, HMENU handle, int x, int y )
{
    SYSCALL_FUNC( NtUserMenuItemFromPoint );
}

BOOL SYSCALL_API NtUserMessageBeep( UINT type )
{
    SYSCALL_FUNC( NtUserMessageBeep );
}

LRESULT SYSCALL_API NtUserMessageCall( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                       void *result_info, DWORD type, BOOL ansi )
{
    SYSCALL_FUNC( NtUserMessageCall );
}

BOOL SYSCALL_API NtUserMoveWindow( HWND hwnd, INT x, INT y, INT cx, INT cy, BOOL repaint )
{
    SYSCALL_FUNC( NtUserMoveWindow );
}

DWORD SYSCALL_API NtUserMsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles,
                                                     DWORD timeout, DWORD mask, DWORD flags )
{
    SYSCALL_FUNC( NtUserMsgWaitForMultipleObjectsEx );
}

void SYSCALL_API NtUserNotifyIMEStatus( HWND hwnd, UINT status )
{
    SYSCALL_FUNC( NtUserNotifyIMEStatus );
}

void SYSCALL_API NtUserNotifyWinEvent( DWORD event, HWND hwnd, LONG object_id, LONG child_id )
{
    SYSCALL_FUNC( NtUserNotifyWinEvent );
}

BOOL SYSCALL_API NtUserOpenClipboard( HWND hwnd, ULONG unk )
{
    SYSCALL_FUNC( NtUserOpenClipboard );
}

HDESK SYSCALL_API NtUserOpenDesktop( OBJECT_ATTRIBUTES *attr, DWORD flags, ACCESS_MASK access )
{
    SYSCALL_FUNC( NtUserOpenDesktop );
}

HDESK SYSCALL_API NtUserOpenInputDesktop( DWORD flags, BOOL inherit, ACCESS_MASK access )
{
    SYSCALL_FUNC( NtUserOpenInputDesktop );
}

HWINSTA SYSCALL_API NtUserOpenWindowStation( OBJECT_ATTRIBUTES *attr, ACCESS_MASK access )
{
    SYSCALL_FUNC( NtUserOpenWindowStation );
}

BOOL SYSCALL_API NtUserPeekMessage( MSG *msg_out, HWND hwnd, UINT first, UINT last, UINT flags )
{
    SYSCALL_FUNC( NtUserPeekMessage );
}

BOOL SYSCALL_API NtUserPerMonitorDPIPhysicalToLogicalPoint( HWND hwnd, POINT *pt )
{
    SYSCALL_FUNC( NtUserPerMonitorDPIPhysicalToLogicalPoint );
}

BOOL SYSCALL_API NtUserPostMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    SYSCALL_FUNC( NtUserPostMessage );
}

BOOL SYSCALL_API NtUserPostQuitMessage( INT exit_code )
{
    SYSCALL_FUNC( NtUserPostQuitMessage );
}

BOOL SYSCALL_API NtUserPostThreadMessage( DWORD thread, UINT msg, WPARAM wparam, LPARAM lparam )
{
    SYSCALL_FUNC( NtUserPostThreadMessage );
}

BOOL SYSCALL_API NtUserPrintWindow( HWND hwnd, HDC hdc, UINT flags )
{
    SYSCALL_FUNC( NtUserPrintWindow );
}

LONG SYSCALL_API NtUserQueryDisplayConfig( UINT32 flags, UINT32 *paths_count, DISPLAYCONFIG_PATH_INFO *paths,
                                           UINT32 *modes_count, DISPLAYCONFIG_MODE_INFO *modes,
                                           DISPLAYCONFIG_TOPOLOGY_ID *topology_id )
{
    SYSCALL_FUNC( NtUserQueryDisplayConfig );
}

UINT_PTR SYSCALL_API NtUserQueryInputContext( HIMC handle, UINT attr )
{
    SYSCALL_FUNC( NtUserQueryInputContext );
}

HANDLE SYSCALL_API NtUserQueryWindow( HWND hwnd, WINDOWINFOCLASS cls )
{
    SYSCALL_FUNC( NtUserQueryWindow );
}

HWND SYSCALL_API NtUserRealChildWindowFromPoint( HWND parent, LONG x, LONG y )
{
    SYSCALL_FUNC( NtUserRealChildWindowFromPoint );
}

UINT SYSCALL_API NtUserRealizePalette( HDC hdc )
{
    SYSCALL_FUNC( NtUserRealizePalette );
}

BOOL SYSCALL_API NtUserRedrawWindow( HWND hwnd, const RECT *rect, HRGN hrgn, UINT flags )
{
    SYSCALL_FUNC( NtUserRedrawWindow );
}

ATOM SYSCALL_API NtUserRegisterClassExWOW( const WNDCLASSEXW *wc, UNICODE_STRING *name, UNICODE_STRING *version,
                                           struct client_menu_name *client_menu_name, DWORD fnid,
                                           DWORD flags, DWORD *wow )
{
    SYSCALL_FUNC( NtUserRegisterClassExWOW );
}

BOOL SYSCALL_API NtUserRegisterHotKey( HWND hwnd, INT id, UINT modifiers, UINT vk )
{
    SYSCALL_FUNC( NtUserRegisterHotKey );
}

BOOL SYSCALL_API NtUserRegisterRawInputDevices( const RAWINPUTDEVICE *devices, UINT device_count, UINT device_size )
{
    SYSCALL_FUNC( NtUserRegisterRawInputDevices );
}

BOOL SYSCALL_API NtUserRegisterTouchPadCapable( BOOL capable )
{
    SYSCALL_FUNC( NtUserRegisterTouchPadCapable );
}

BOOL SYSCALL_API NtUserReleaseCapture(void)
{
    SYSCALL_FUNC( NtUserReleaseCapture );
}

INT SYSCALL_API NtUserReleaseDC( HWND hwnd, HDC hdc )
{
    SYSCALL_FUNC( NtUserReleaseDC );
}

BOOL SYSCALL_API NtUserRemoveClipboardFormatListener( HWND hwnd )
{
    SYSCALL_FUNC( NtUserRemoveClipboardFormatListener );
}

BOOL SYSCALL_API NtUserRemoveMenu( HMENU handle, UINT id, UINT flags )
{
    SYSCALL_FUNC( NtUserRemoveMenu );
}

HANDLE SYSCALL_API NtUserRemoveProp( HWND hwnd, const WCHAR *str )
{
    SYSCALL_FUNC( NtUserRemoveProp );
}

BOOL SYSCALL_API NtUserReplyMessage( LRESULT result )
{
    SYSCALL_FUNC( NtUserReplyMessage );
}

BOOL SYSCALL_API NtUserScrollDC( HDC hdc, INT dx, INT dy, const RECT *scroll, const RECT *clip,
                                 HRGN ret_update_rgn, RECT *update_rect )
{
    SYSCALL_FUNC( NtUserScrollDC );
}

INT SYSCALL_API NtUserScrollWindowEx( HWND hwnd, INT dx, INT dy, const RECT *rect,
                                      const RECT *clip_rect, HRGN update_rgn,
                                      RECT *update_rect, UINT flags )
{
    SYSCALL_FUNC( NtUserScrollWindowEx );
}

HPALETTE SYSCALL_API NtUserSelectPalette( HDC hdc, HPALETTE hpal, WORD bkg )
{
    SYSCALL_FUNC( NtUserSelectPalette );
}

UINT SYSCALL_API NtUserSendInput( UINT count, INPUT *inputs, int size )
{
    SYSCALL_FUNC( NtUserSendInput );
}

HWND SYSCALL_API NtUserSetActiveWindow( HWND hwnd )
{
    SYSCALL_FUNC( NtUserSetActiveWindow );
}

BOOL SYSCALL_API NtUserSetAdditionalForegroundBoostProcesses( HWND hwnd, DWORD count, HANDLE *handles )
{
    SYSCALL_FUNC( NtUserSetAdditionalForegroundBoostProcesses );
}

HWND SYSCALL_API NtUserSetCapture( HWND hwnd )
{
    SYSCALL_FUNC( NtUserSetCapture );
}

BOOL SYSCALL_API NtUserSetCaretBlinkTime( unsigned int time )
{
    SYSCALL_FUNC( NtUserSetCaretBlinkTime );
}

BOOL SYSCALL_API NtUserSetCaretPos( INT x, INT y )
{
    SYSCALL_FUNC( NtUserSetCaretPos );
}

DWORD SYSCALL_API NtUserSetClassLong( HWND hwnd, INT offset, LONG newval, BOOL ansi )
{
    SYSCALL_FUNC( NtUserSetClassLong );
}

ULONG_PTR SYSCALL_API NtUserSetClassLongPtr( HWND hwnd, INT offset, LONG_PTR newval, BOOL ansi )
{
    SYSCALL_FUNC( NtUserSetClassLongPtr );
}

WORD SYSCALL_API NtUserSetClassWord( HWND hwnd, INT offset, WORD newval )
{
    SYSCALL_FUNC( NtUserSetClassWord );
}

NTSTATUS SYSCALL_API NtUserSetClipboardData( UINT format, HANDLE data, struct set_clipboard_params *params )
{
    SYSCALL_FUNC( NtUserSetClipboardData );
}

HWND SYSCALL_API NtUserSetClipboardViewer( HWND hwnd )
{
    SYSCALL_FUNC( NtUserSetClipboardViewer );
}

HCURSOR SYSCALL_API NtUserSetCursor( HCURSOR cursor )
{
    SYSCALL_FUNC( NtUserSetCursor );
}

BOOL SYSCALL_API NtUserSetCursorIconData( HCURSOR cursor, UNICODE_STRING *module, UNICODE_STRING *res_name,
                                          struct cursoricon_desc *desc )
{
    SYSCALL_FUNC( NtUserSetCursorIconData );
}

BOOL SYSCALL_API NtUserSetCursorPos( INT x, INT y )
{
    SYSCALL_FUNC( NtUserSetCursorPos );
}

HWND SYSCALL_API NtUserSetFocus( HWND hwnd )
{
    SYSCALL_FUNC( NtUserSetFocus );
}

BOOL SYSCALL_API NtUserSetForegroundWindow( HWND hwnd )
{
    SYSCALL_FUNC( NtUserSetForegroundWindow );
}

void SYSCALL_API NtUserSetInternalWindowPos( HWND hwnd, UINT cmd, RECT *rect, POINT *pt )
{
    SYSCALL_FUNC( NtUserSetInternalWindowPos );
}

BOOL SYSCALL_API NtUserSetKeyboardState( BYTE *state )
{
    SYSCALL_FUNC( NtUserSetKeyboardState );
}

BOOL SYSCALL_API NtUserSetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags )
{
    SYSCALL_FUNC( NtUserSetLayeredWindowAttributes );
}

BOOL SYSCALL_API NtUserSetMenu( HWND hwnd, HMENU menu )
{
    SYSCALL_FUNC( NtUserSetMenu );
}

BOOL SYSCALL_API NtUserSetMenuContextHelpId( HMENU handle, DWORD id )
{
    SYSCALL_FUNC( NtUserSetMenuContextHelpId );
}

BOOL SYSCALL_API NtUserSetMenuDefaultItem( HMENU handle, UINT item, UINT bypos )
{
    SYSCALL_FUNC( NtUserSetMenuDefaultItem );
}

BOOL SYSCALL_API NtUserSetObjectInformation( HANDLE handle, INT index, void *info, DWORD len )
{
    SYSCALL_FUNC( NtUserSetObjectInformation );
}

HWND SYSCALL_API NtUserSetParent( HWND hwnd, HWND parent )
{
    SYSCALL_FUNC( NtUserSetParent );
}

BOOL SYSCALL_API NtUserSetProcessDefaultLayout( ULONG layout )
{
    SYSCALL_FUNC( NtUserSetProcessDefaultLayout );
}

BOOL SYSCALL_API NtUserSetProcessDpiAwarenessContext( ULONG awareness, ULONG unknown )
{
    SYSCALL_FUNC( NtUserSetProcessDpiAwarenessContext );
}

BOOL SYSCALL_API NtUserSetProcessWindowStation( HWINSTA handle )
{
    SYSCALL_FUNC( NtUserSetProcessWindowStation );
}

HWND SYSCALL_API NtUserSetProgmanWindow( HWND hwnd )
{
    SYSCALL_FUNC( NtUserSetProgmanWindow );
}

BOOL SYSCALL_API NtUserSetProp( HWND hwnd, const WCHAR *str, HANDLE handle )
{
    SYSCALL_FUNC( NtUserSetProp );
}

INT SYSCALL_API NtUserSetScrollInfo( HWND hwnd, int bar, const SCROLLINFO *info, BOOL redraw )
{
    SYSCALL_FUNC( NtUserSetScrollInfo );
}

BOOL SYSCALL_API NtUserSetShellWindowEx( HWND shell, HWND list_view )
{
    SYSCALL_FUNC( NtUserSetShellWindowEx );
}

BOOL SYSCALL_API NtUserSetSysColors( INT count, const INT *colors, const COLORREF *values )
{
    SYSCALL_FUNC( NtUserSetSysColors );
}

BOOL SYSCALL_API NtUserSetSystemMenu( HWND hwnd, HMENU menu )
{
    SYSCALL_FUNC( NtUserSetSystemMenu );
}

UINT_PTR SYSCALL_API NtUserSetSystemTimer( HWND hwnd, UINT_PTR id, UINT timeout )
{
    SYSCALL_FUNC( NtUserSetSystemTimer );
}

HWND SYSCALL_API NtUserSetTaskmanWindow( HWND hwnd )
{
    SYSCALL_FUNC( NtUserSetTaskmanWindow );
}

BOOL SYSCALL_API NtUserSetThreadDesktop( HDESK handle )
{
    SYSCALL_FUNC( NtUserSetThreadDesktop );
}

UINT_PTR SYSCALL_API NtUserSetTimer( HWND hwnd, UINT_PTR id, UINT timeout, TIMERPROC proc, ULONG tolerance )
{
    SYSCALL_FUNC( NtUserSetTimer );
}

HWINEVENTHOOK SYSCALL_API NtUserSetWinEventHook( DWORD event_min, DWORD event_max, HMODULE inst,
                                                 UNICODE_STRING *module, WINEVENTPROC proc,
                                                 DWORD pid, DWORD tid, DWORD flags )
{
    SYSCALL_FUNC( NtUserSetWinEventHook );
}

BOOL SYSCALL_API NtUserSetWindowContextHelpId( HWND hwnd, DWORD id )
{
    SYSCALL_FUNC( NtUserSetWindowContextHelpId );
}

LONG SYSCALL_API NtUserSetWindowLong( HWND hwnd, INT offset, LONG newval, BOOL ansi )
{
    SYSCALL_FUNC( NtUserSetWindowLong );
}

LONG_PTR SYSCALL_API NtUserSetWindowLongPtr( HWND hwnd, INT offset, LONG_PTR newval, BOOL ansi )
{
    SYSCALL_FUNC( NtUserSetWindowLongPtr );
}

BOOL SYSCALL_API NtUserSetWindowPlacement( HWND hwnd, const WINDOWPLACEMENT *wpl )
{
    SYSCALL_FUNC( NtUserSetWindowPlacement );
}

BOOL SYSCALL_API NtUserSetWindowPos( HWND hwnd, HWND after, INT x, INT y, INT cx, INT cy, UINT flags )
{
    SYSCALL_FUNC( NtUserSetWindowPos );
}

int SYSCALL_API NtUserSetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
    SYSCALL_FUNC( NtUserSetWindowRgn );
}

WORD SYSCALL_API NtUserSetWindowWord( HWND hwnd, INT offset, WORD newval )
{
    SYSCALL_FUNC( NtUserSetWindowWord );
}

HHOOK SYSCALL_API NtUserSetWindowsHookEx( HINSTANCE inst, UNICODE_STRING *module, DWORD tid, INT id,
                                          HOOKPROC proc, BOOL ansi )
{
    SYSCALL_FUNC( NtUserSetWindowsHookEx );
}

BOOL SYSCALL_API NtUserShowCaret( HWND hwnd )
{
    SYSCALL_FUNC( NtUserShowCaret );
}

INT SYSCALL_API NtUserShowCursor( BOOL show )
{
    SYSCALL_FUNC( NtUserShowCursor );
}

BOOL SYSCALL_API NtUserShowOwnedPopups( HWND owner, BOOL show )
{
    SYSCALL_FUNC( NtUserShowOwnedPopups );
}

BOOL SYSCALL_API NtUserShowScrollBar( HWND hwnd, INT bar, BOOL show )
{
    SYSCALL_FUNC( NtUserShowScrollBar );
}

BOOL SYSCALL_API NtUserShowWindow( HWND hwnd, INT cmd )
{
    SYSCALL_FUNC( NtUserShowWindow );
}

BOOL SYSCALL_API NtUserShowWindowAsync( HWND hwnd, INT cmd )
{
    SYSCALL_FUNC( NtUserShowWindowAsync );
}

BOOL SYSCALL_API NtUserSwitchDesktop( HDESK handle )
{
    SYSCALL_FUNC( NtUserSwitchDesktop );
}

BOOL SYSCALL_API NtUserSystemParametersInfo( UINT action, UINT val, void *ptr, UINT winini )
{
    SYSCALL_FUNC( NtUserSystemParametersInfo );
}

BOOL SYSCALL_API NtUserSystemParametersInfoForDpi( UINT action, UINT val, PVOID ptr, UINT winini, UINT dpi )
{
    SYSCALL_FUNC( NtUserSystemParametersInfoForDpi );
}

BOOL SYSCALL_API NtUserThunkedMenuInfo( HMENU menu, const MENUINFO *info )
{
    SYSCALL_FUNC( NtUserThunkedMenuInfo );
}

UINT SYSCALL_API NtUserThunkedMenuItemInfo( HMENU handle, UINT pos, UINT flags, UINT method,
                                            MENUITEMINFOW *info, UNICODE_STRING *str )
{
    SYSCALL_FUNC( NtUserThunkedMenuItemInfo );
}

INT SYSCALL_API NtUserToUnicodeEx( UINT virt, UINT scan, const BYTE *state,
                                   WCHAR *str, int size, UINT flags, HKL layout )
{
    SYSCALL_FUNC( NtUserToUnicodeEx );
}

BOOL SYSCALL_API NtUserTrackMouseEvent( TRACKMOUSEEVENT *info )
{
    SYSCALL_FUNC( NtUserTrackMouseEvent );
}

BOOL SYSCALL_API NtUserTrackPopupMenuEx( HMENU handle, UINT flags, INT x, INT y, HWND hwnd,
                                         TPMPARAMS *params )
{
    SYSCALL_FUNC( NtUserTrackPopupMenuEx );
}

INT SYSCALL_API NtUserTranslateAccelerator( HWND hwnd, HACCEL accel, MSG *msg )
{
    SYSCALL_FUNC( NtUserTranslateAccelerator );
}

BOOL SYSCALL_API NtUserTranslateMessage( const MSG *msg, UINT flags )
{
    SYSCALL_FUNC( NtUserTranslateMessage );
}

BOOL SYSCALL_API NtUserUnhookWinEvent( HWINEVENTHOOK handle )
{
    SYSCALL_FUNC( NtUserUnhookWinEvent );
}

BOOL SYSCALL_API NtUserUnhookWindowsHook( INT id, HOOKPROC proc )
{
    SYSCALL_FUNC( NtUserUnhookWindowsHook );
}

BOOL SYSCALL_API NtUserUnhookWindowsHookEx( HHOOK handle )
{
    SYSCALL_FUNC( NtUserUnhookWindowsHookEx );
}

BOOL SYSCALL_API NtUserUnregisterClass( UNICODE_STRING *name, HINSTANCE instance,
                                        struct client_menu_name *client_menu_name )
{
    SYSCALL_FUNC( NtUserUnregisterClass );
}

BOOL SYSCALL_API NtUserUnregisterHotKey( HWND hwnd, INT id )
{
    SYSCALL_FUNC( NtUserUnregisterHotKey );
}

BOOL SYSCALL_API NtUserUpdateInputContext( HIMC handle, UINT attr, UINT_PTR value )
{
    SYSCALL_FUNC( NtUserUpdateInputContext );
}

BOOL SYSCALL_API NtUserUpdateLayeredWindow( HWND hwnd, HDC hdc_dst, const POINT *pts_dst, const SIZE *size,
                                            HDC hdc_src, const POINT *pts_src, COLORREF key,
                                            const BLENDFUNCTION *blend, DWORD flags, const RECT *dirty )
{
    SYSCALL_FUNC( NtUserUpdateLayeredWindow );
}

BOOL SYSCALL_API NtUserValidateRect( HWND hwnd, const RECT *rect )
{
    SYSCALL_FUNC( NtUserValidateRect );
}

BOOL SYSCALL_API NtUserValidateRgn( HWND hwnd, HRGN hrgn )
{
    SYSCALL_FUNC( NtUserValidateRgn );
}

WORD SYSCALL_API NtUserVkKeyScanEx( WCHAR chr, HKL layout )
{
    SYSCALL_FUNC( NtUserVkKeyScanEx );
}

DWORD SYSCALL_API NtUserWaitForInputIdle( HANDLE process, DWORD timeout, BOOL wow )
{
    SYSCALL_FUNC( NtUserWaitForInputIdle );
}

BOOL SYSCALL_API NtUserWaitMessage(void)
{
    SYSCALL_FUNC( NtUserWaitMessage );
}

HWND SYSCALL_API NtUserWindowFromDC( HDC hdc )
{
    SYSCALL_FUNC( NtUserWindowFromDC );
}

HWND SYSCALL_API NtUserWindowFromPoint( LONG x, LONG y )
{
    SYSCALL_FUNC( NtUserWindowFromPoint );
}

BOOL SYSCALL_API __wine_get_icm_profile( HDC hdc, BOOL allow_default, DWORD *size, WCHAR *filename )
{
    SYSCALL_FUNC( __wine_get_icm_profile );
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
    record.ExceptionFlags   = EXCEPTION_NONCONTINUABLE;
    record.ExceptionRecord  = NULL;
    record.ExceptionAddress = __wine_spec_unimplemented_stub;
    record.NumberParameters = 2;
    record.ExceptionInformation[0] = (ULONG_PTR)module;
    record.ExceptionInformation[1] = (ULONG_PTR)function;
    for (;;) RtlRaiseException( &record );
}

void *dummy = NtQueryVirtualMemory;  /* forced import to avoid link error with winecrt0 */

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
        if (!__wine_init_unix_call()) WINE_UNIX_CALL( 0, NULL );
        break;
    }
    return TRUE;
}
