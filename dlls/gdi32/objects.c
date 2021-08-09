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

#include "wine/rbtree.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);


struct hdc_list
{
    HDC hdc;
    void (*delete)( HDC hdc, HGDIOBJ handle );
    struct hdc_list *next;
};

struct obj_map_entry
{
    struct wine_rb_entry entry;
    struct hdc_list *list;
    HGDIOBJ obj;
};

static CRITICAL_SECTION obj_map_cs;
static CRITICAL_SECTION_DEBUG obj_map_debug =
{
    0, 0, &obj_map_cs,
    { &obj_map_debug.ProcessLocksList, &obj_map_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": obj_map_cs") }
};
static CRITICAL_SECTION obj_map_cs = { &obj_map_debug, -1, 0, 0, 0, 0 };

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

static BOOL is_stock_object( HGDIOBJ obj )
{
    unsigned int handle = HandleToULong( obj );
    return !!(handle & NTGDI_HANDLE_STOCK_OBJECT);
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

static int obj_map_cmp( const void *key, const struct wine_rb_entry *entry )
{
    struct obj_map_entry *obj_entry = WINE_RB_ENTRY_VALUE( entry, struct obj_map_entry, entry );
    return HandleToLong( key ) - HandleToLong( obj_entry->obj );
};

struct wine_rb_tree obj_map = { obj_map_cmp };

/***********************************************************************
 *           DeleteObject    (GDI32.@)
 *
 * Delete a Gdi object.
 */
BOOL WINAPI DeleteObject( HGDIOBJ obj )
{
    struct hdc_list *hdc_list = NULL;
    struct wine_rb_entry *entry;

    EnterCriticalSection( &obj_map_cs );

    if ((entry = wine_rb_get( &obj_map, obj )))
    {
        struct obj_map_entry *obj_entry = WINE_RB_ENTRY_VALUE( entry, struct obj_map_entry, entry );
        wine_rb_remove( &obj_map, entry );
        hdc_list = obj_entry->list;
        HeapFree( GetProcessHeap(), 0, obj_entry );
    }

    LeaveCriticalSection( &obj_map_cs );

    while (hdc_list)
    {
        struct hdc_list *next = hdc_list->next;

        TRACE( "hdc %p has interest in %p\n", hdc_list->hdc, obj );

        hdc_list->delete( hdc_list->hdc, obj );
        HeapFree( GetProcessHeap(), 0, hdc_list );
        hdc_list = next;
    }

    return NtGdiDeleteObjectApp( obj );
}

/***********************************************************************
 *           GDI_hdc_using_object
 *
 * Call this if the dc requires DeleteObject notification
 */
void GDI_hdc_using_object( HGDIOBJ obj, HDC hdc, void (*delete)( HDC hdc, HGDIOBJ handle ))
{
    struct hdc_list *hdc_list;
    GDI_HANDLE_ENTRY *entry;

    TRACE( "obj %p hdc %p\n", obj, hdc );

    EnterCriticalSection( &obj_map_cs );
    if (!is_stock_object( obj ) && (entry = handle_entry( obj )))
    {
        struct obj_map_entry *map_entry;
        struct wine_rb_entry *entry;

        if (!(entry = wine_rb_get( &obj_map, obj )))
        {
            if (!(map_entry = HeapAlloc( GetProcessHeap(), 0, sizeof(*map_entry) )))
            {
                LeaveCriticalSection( &obj_map_cs );
                return;
            }
            map_entry->obj  = obj;
            map_entry->list = NULL;
            wine_rb_put( &obj_map, obj, &map_entry->entry );
        }
        else map_entry = WINE_RB_ENTRY_VALUE( entry, struct obj_map_entry, entry );

        for (hdc_list = map_entry->list; hdc_list; hdc_list = hdc_list->next)
            if (hdc_list->hdc == hdc) break;

        if (!hdc_list)
        {
            if (!(hdc_list = HeapAlloc( GetProcessHeap(), 0, sizeof(*hdc_list) )))
            {
                LeaveCriticalSection( &obj_map_cs );
                return;
            }
            hdc_list->hdc    = hdc;
            hdc_list->delete = delete;
            hdc_list->next   = map_entry->list;
            map_entry->list  = hdc_list;
        }
    }
    LeaveCriticalSection( &obj_map_cs );
}

/***********************************************************************
 *           GDI_hdc_not_using_object
 *
 */
void GDI_hdc_not_using_object( HGDIOBJ obj, HDC hdc )
{
    struct wine_rb_entry *entry;

    TRACE( "obj %p hdc %p\n", obj, hdc );

    EnterCriticalSection( &obj_map_cs );
    if ((entry = wine_rb_get( &obj_map, obj )))
    {
        struct obj_map_entry *map_entry = WINE_RB_ENTRY_VALUE( entry, struct obj_map_entry, entry );
        struct hdc_list **list_ptr, *hdc_list;

        for (list_ptr = &map_entry->list; *list_ptr; list_ptr = &(*list_ptr)->next)
        {
            if ((*list_ptr)->hdc != hdc) continue;

            hdc_list = *list_ptr;
            *list_ptr = hdc_list->next;
            HeapFree( GetProcessHeap(), 0, hdc_list );
            if (list_ptr == &map_entry->list && !*list_ptr)
            {
                wine_rb_remove( &obj_map, &map_entry->entry );
                HeapFree( GetProcessHeap(), 0, map_entry );
            }
            break;
        }
    }
    LeaveCriticalSection( &obj_map_cs );
}

/***********************************************************************
 *           SelectObject    (GDI32.@)
 *
 * Select a Gdi object into a device context.
 */
HGDIOBJ WINAPI SelectObject( HDC hdc, HGDIOBJ obj )
{
    DC_ATTR *dc_attr;
    HGDIOBJ ret;

    TRACE( "(%p,%p)\n", hdc, obj );

    if (is_meta_dc( hdc )) return METADC_SelectObject( hdc, obj );
    if (!(dc_attr = get_dc_attr( hdc ))) return 0;
    if (dc_attr->emf && !EMFDC_SelectObject( dc_attr, obj )) return 0;

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
 *           CreatePolyPolygonRgn    (GDI32.@)
 */
HRGN WINAPI CreatePolyPolygonRgn( const POINT *points, const INT *counts, INT count, INT mode )
{
    ULONG ret = NtGdiPolyPolyDraw( ULongToHandle(mode), points, (const UINT *)counts,
                                   count, NtGdiPolyPolygonRgn );
    return ULongToHandle( ret );
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

/***********************************************************************
 *           CreateColorSpaceA    (GDI32.@)
 */
HCOLORSPACE WINAPI CreateColorSpaceA( LOGCOLORSPACEA *cs )
{
    FIXME( "stub\n" );
    return 0;
}

/***********************************************************************
 *           CreateColorSpaceW    (GDI32.@)
 */
HCOLORSPACE WINAPI CreateColorSpaceW( LOGCOLORSPACEW *cs )
{
    FIXME( "stub\n" );
    return 0;
}

/***********************************************************************
 *           DeleteColorSpace     (GDI32.@)
 */
BOOL WINAPI DeleteColorSpace( HCOLORSPACE cs )
{
    FIXME( "stub\n" );
    return TRUE;
}

/***********************************************************************
 *           GetColorSpace    (GDI32.@)
 */
HCOLORSPACE WINAPI GetColorSpace( HDC hdc )
{
    FIXME( "stub\n" );
    return 0;
}

/***********************************************************************
 *           SetColorSpace     (GDI32.@)
 */
HCOLORSPACE WINAPI SetColorSpace( HDC hdc, HCOLORSPACE cs )
{
    FIXME( "stub\n" );
    return cs;
}
