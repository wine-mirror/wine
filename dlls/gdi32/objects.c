/*
 * GDI functions
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

#include "gdi_private.h"
#include "winnls.h"
#include "winternl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);


static GDI_SHARED_MEMORY *get_gdi_shared(void)
{
#ifndef _WIN64
    if (NtCurrentTeb()->GdiBatchCount)
    {
        TEB64 *teb64 = (TEB64 *)(UINT_PTR)NtCurrentTeb()->GdiBatchCount;
        PEB64 *peb64 = (PEB64 *)(UINT_PTR)teb64->Peb;
        return (GDI_SHARED_MEMORY *)(UINT_PTR)peb64->GdiSharedHandleTable;
    }
#endif
    return (GDI_SHARED_MEMORY *)NtCurrentTeb()->Peb->GdiSharedHandleTable;
}

static inline GDI_HANDLE_ENTRY *handle_entry( HGDIOBJ handle )
{
    GDI_SHARED_MEMORY *gdi_shared = get_gdi_shared();
    unsigned int idx = LOWORD(handle);

    if (idx < GDI_MAX_HANDLE_COUNT && gdi_shared->Handles[idx].Type)
    {
        if (!HIWORD( handle ) || HIWORD( handle ) == gdi_shared->Handles[idx].Unique)
            return &gdi_shared->Handles[idx];
    }
    if (handle) WARN( "invalid handle %p\n", handle );
    return NULL;
}

static WORD get_object_type( HGDIOBJ obj )
{
    GDI_HANDLE_ENTRY *entry = handle_entry( obj );
    return entry ? entry->ExtType : 0;
}

void set_gdi_client_ptr( HGDIOBJ obj, void *ptr )
{
    GDI_HANDLE_ENTRY *entry = handle_entry( obj );
    if (entry) entry->UserPointer = (UINT_PTR)ptr;
}

void *get_gdi_client_ptr( HGDIOBJ obj, WORD type )
{
    GDI_HANDLE_ENTRY *entry = handle_entry( obj );
    if (!entry || (type && entry->ExtType != type) || !entry->UserPointer)
        return NULL;
    return (void *)(UINT_PTR)entry->UserPointer;
}

/***********************************************************************
 *           GetObjectType    (GDI32.@)
 */
DWORD WINAPI GetObjectType( HGDIOBJ handle )
{
    DWORD type = get_object_type( handle );

    TRACE( "%p -> %u\n", handle, type );

    switch (type)
    {
    case NTGDI_OBJ_PEN:         return OBJ_PEN;
    case NTGDI_OBJ_BRUSH:       return OBJ_BRUSH;
    case NTGDI_OBJ_DC:          return OBJ_DC;
    case NTGDI_OBJ_METADC:      return OBJ_METADC;
    case NTGDI_OBJ_PAL:         return OBJ_PAL;
    case NTGDI_OBJ_FONT:        return OBJ_FONT;
    case NTGDI_OBJ_BITMAP:      return OBJ_BITMAP;
    case NTGDI_OBJ_REGION:      return OBJ_REGION;
    case NTGDI_OBJ_METAFILE:    return OBJ_METAFILE;
    case NTGDI_OBJ_MEMDC:       return OBJ_MEMDC;
    case NTGDI_OBJ_EXTPEN:      return OBJ_EXTPEN;
    case NTGDI_OBJ_ENHMETADC:   return OBJ_ENHMETADC;
    case NTGDI_OBJ_ENHMETAFILE: return OBJ_ENHMETAFILE;
    default:
        SetLastError( ERROR_INVALID_HANDLE );
        return 0;
    }
}

/***********************************************************************
 *           DeleteObject    (GDI32.@)
 *
 * Delete a Gdi object.
 */
BOOL WINAPI DeleteObject( HGDIOBJ obj )
{
    return NtGdiDeleteObjectApp( obj );
}

/***********************************************************************
 *           SelectObject    (GDI32.@)
 *
 * Select a Gdi object into a device context.
 */
HGDIOBJ WINAPI SelectObject( HDC hdc, HGDIOBJ obj )
{
    HGDIOBJ ret;

    TRACE( "(%p,%p)\n", hdc, obj );

    switch (get_object_type( obj ))
    {
    case NTGDI_OBJ_PEN:
    case NTGDI_OBJ_EXTPEN:
        ret = NtGdiSelectPen( hdc, obj );
        break;
    case NTGDI_OBJ_BRUSH:
        ret = NtGdiSelectBrush( hdc, obj );
        break;
    case NTGDI_OBJ_FONT:
        ret = NtGdiSelectFont( hdc, obj );
        break;
    case NTGDI_OBJ_BITMAP:
        ret = NtGdiSelectBitmap( hdc, obj );
        break;
    case NTGDI_OBJ_REGION:
        ret = ULongToHandle(SelectClipRgn( hdc, obj ));
        break;
    default:
        return 0;
    }

    if (!ret) SetLastError( ERROR_INVALID_HANDLE );
    return ret;
}

/***********************************************************************
 *           GetObjectW    (GDI32.@)
 */
INT WINAPI GetObjectW( HGDIOBJ handle, INT count, void *buffer )
{
    int result;

    TRACE( "%p %d %p\n", handle, count, buffer );

    result = NtGdiExtGetObjectW( handle, count, buffer );
    if (!result && count)
    {
        switch(get_object_type( handle ))
        {
        case 0:
        case NTGDI_OBJ_BITMAP:
        case NTGDI_OBJ_BRUSH:
        case NTGDI_OBJ_FONT:
        case NTGDI_OBJ_PAL:
        case NTGDI_OBJ_PEN:
        case NTGDI_OBJ_EXTPEN:
            break;
        default:
            SetLastError( ERROR_INVALID_HANDLE );
        }
    }
    return result;
}

/***********************************************************************
 *           GetObjectA    (GDI32.@)
 */
INT WINAPI GetObjectA( HGDIOBJ handle, INT count, void *buffer )
{
    TRACE("%p %d %p\n", handle, count, buffer );

    if (get_object_type( handle ) == NTGDI_OBJ_FONT)
    {
        LOGFONTA *lfA = buffer;
        LOGFONTW lf;

        if (!buffer) return sizeof(*lfA);
        if (!GetObjectW( handle, sizeof(lf), &lf )) return 0;
        if (count > sizeof(*lfA)) count = sizeof(*lfA);
        memcpy( lfA, &lf, min( count, FIELD_OFFSET(LOGFONTA, lfFaceName) ));
        if (count > FIELD_OFFSET(LOGFONTA, lfFaceName))
        {
            WideCharToMultiByte( CP_ACP, 0, lf.lfFaceName, -1, lfA->lfFaceName,
                                 count - FIELD_OFFSET(LOGFONTA, lfFaceName), NULL, NULL );
            if (count == sizeof(*lfA)) lfA->lfFaceName[LF_FACESIZE - 1] = 0;
        }
        return count;
    }

    return GetObjectW( handle, count, buffer );
}

/***********************************************************************
 *           CreatePenIndirect    (GDI32.@)
 */
HPEN WINAPI CreatePenIndirect( const LOGPEN *pen )
{
    return CreatePen( pen->lopnStyle, pen->lopnWidth.x, pen->lopnColor );
}

/***********************************************************************
 *           CreatePen    (GDI32.@)
 */
HPEN WINAPI CreatePen( INT style, INT width, COLORREF color )
{
    if (style < 0 || style > PS_INSIDEFRAME) style = PS_SOLID;
    return NtGdiCreatePen( style, width, color, NULL );
}

/***********************************************************************
 *           CreateBitmapIndirect (GDI32.@)
 */
HBITMAP WINAPI CreateBitmapIndirect( const BITMAP *bmp )
{
    if (!bmp || bmp->bmType)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    return CreateBitmap( bmp->bmWidth, bmp->bmHeight, bmp->bmPlanes,
                         bmp->bmBitsPixel, bmp->bmBits );
}

/******************************************************************************
 *           CreateBitmap (GDI32.@)
 *
 * Creates a bitmap with the specified info.
 */
HBITMAP WINAPI CreateBitmap( INT width, INT height, UINT planes,
                             UINT bpp, const void *bits )
{
    if (!width || !height)
        return GetStockObject( STOCK_LAST + 1 ); /* default 1x1 bitmap */

    return NtGdiCreateBitmap( width, height, planes, bpp, bits );
}

/******************************************************************************
 *           CreateDiscardableBitmap (GDI32.@)
 *
 * Creates a discardable bitmap.
 */
HBITMAP WINAPI CreateDiscardableBitmap( HDC hdc, INT width, INT height )
{
    return CreateCompatibleBitmap( hdc, width, height );
}

/***********************************************************************
 *           ExtCreateRegion   (GDI32.@)
 *
 * Creates a region as specified by the transformation data and region data.
 */
HRGN WINAPI ExtCreateRegion( const XFORM *xform, DWORD count, const RGNDATA *data )
{
    if (!data)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    return NtGdiExtCreateRegion( xform, count, data );
}

/***********************************************************************
 *           CreateRectRgnIndirect    (GDI32.@)
 *
 * Creates a simple rectangular region.
 */
HRGN WINAPI CreateRectRgnIndirect( const RECT* rect )
{
    return NtGdiCreateRectRgn( rect->left, rect->top, rect->right, rect->bottom );
}

/***********************************************************************
 *           CreateEllipticRgnIndirect    (GDI32.@)
 *
 * Creates an elliptical region.
 */
HRGN WINAPI CreateEllipticRgnIndirect( const RECT *rect )
{
    return NtGdiCreateEllipticRgn( rect->left, rect->top, rect->right, rect->bottom );
}

/***********************************************************************
 *           CreatePolygonRgn    (GDI32.@)
 */
HRGN WINAPI CreatePolygonRgn( const POINT *points, INT count, INT mode )
{
    return CreatePolyPolygonRgn( points, &count, 1, mode );
}
