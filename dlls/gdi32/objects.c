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

#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "gdi_private.h"
#include "ntuser.h"
#include "winreg.h"
#include "winnls.h"
#include "initguid.h"
#include "devguid.h"
#include "setupapi.h"

#include "wine/rbtree.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);


DEFINE_DEVPROPKEY(DEVPROPKEY_GPU_LUID, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 2);

#define FIRST_GDI_HANDLE 32

HMODULE gdi32_module;

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

static HGDIOBJ entry_to_handle( GDI_HANDLE_ENTRY *entry )
{
    unsigned int idx = entry - get_gdi_shared()->Handles;
    return ULongToHandle( idx | (entry->Unique << NTGDI_HANDLE_TYPE_SHIFT) );
}

static DWORD get_object_type( HGDIOBJ obj )
{
    GDI_HANDLE_ENTRY *entry = handle_entry( obj );
    return entry ? entry->ExtType << NTGDI_HANDLE_TYPE_SHIFT : 0;
}

void set_gdi_client_ptr( HGDIOBJ obj, void *ptr )
{
    GDI_HANDLE_ENTRY *entry = handle_entry( obj );
    if (entry) entry->UserPointer = (UINT_PTR)ptr;
}

void *get_gdi_client_ptr( HGDIOBJ obj, DWORD type )
{
    GDI_HANDLE_ENTRY *entry = handle_entry( obj );
    if (!entry || (type && entry->ExtType << NTGDI_HANDLE_TYPE_SHIFT != type))
        return NULL;
    return (void *)(UINT_PTR)entry->UserPointer;
}

HGDIOBJ get_full_gdi_handle( HGDIOBJ obj )
{
    GDI_HANDLE_ENTRY *entry = handle_entry( obj );
    return entry ? entry_to_handle( entry ) : 0;
}

/***********************************************************************
 *           DllMain
 *
 * GDI initialization.
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    if (reason != DLL_PROCESS_ATTACH) return TRUE;

    DisableThreadLibraryCalls( inst );
    gdi32_module = inst;
    return TRUE;
}

/***********************************************************************
 *           GetObjectType    (GDI32.@)
 */
DWORD WINAPI GetObjectType( HGDIOBJ handle )
{
    DWORD type = get_object_type( handle );

    TRACE( "%p -> %lu\n", handle, type );

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
    UINT_PTR a = (UINT_PTR)key;
    UINT_PTR b = (UINT_PTR)obj_entry->obj;
    if (a > b) return 1;
    if (a < b) return -1;
    return 0;
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

    obj = get_full_gdi_handle( obj );
    switch (gdi_handle_type( obj ))
    {
    case NTGDI_OBJ_DC:
    case NTGDI_OBJ_MEMDC:
    case NTGDI_OBJ_ENHMETADC:
    case NTGDI_OBJ_METADC:
        return DeleteDC( obj );
    }

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

    obj = get_full_gdi_handle( obj );
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
 *           GetCurrentObject    (GDI32.@)
 */
HGDIOBJ WINAPI GetCurrentObject( HDC hdc, UINT type )
{
    unsigned int obj_type;

    switch (type)
    {
    case OBJ_EXTPEN: obj_type = NTGDI_OBJ_EXTPEN; break;
    case OBJ_PEN:    obj_type = NTGDI_OBJ_PEN; break;
    case OBJ_BRUSH:  obj_type = NTGDI_OBJ_BRUSH; break;
    case OBJ_PAL:    obj_type = NTGDI_OBJ_PAL; break;
    case OBJ_FONT:   obj_type = NTGDI_OBJ_FONT; break;
    case OBJ_BITMAP: obj_type = NTGDI_OBJ_SURF; break;
    case OBJ_REGION:
        /* tests show that OBJ_REGION is explicitly ignored */
        return 0;
    default:
        FIXME( "(%p,%d): unknown type.\n", hdc, type );
        return 0;
    }

    return NtGdiGetDCObject( hdc, obj_type );
}

/***********************************************************************
 *           GetStockObject    (GDI32.@)
 */
HGDIOBJ WINAPI DECLSPEC_HOTPATCH GetStockObject( INT obj )
{
    if (obj < 0 || obj > STOCK_LAST + 1 || obj == 9) return 0;

    /* Wine stores stock objects in predictable order, see init_stock_objects */
    switch (obj)
    {
    case OEM_FIXED_FONT:
        if (GetDpiForSystem() != 96) obj = 9;
        break;
    case SYSTEM_FONT:
        if (GetDpiForSystem() != 96) obj = STOCK_LAST + 2;
        break;
    case SYSTEM_FIXED_FONT:
        if (GetDpiForSystem() != 96) obj = STOCK_LAST + 3;
        break;
    case DEFAULT_GUI_FONT:
        if (GetDpiForSystem() != 96) obj = STOCK_LAST + 4;
        break;
    }

    return entry_to_handle( handle_entry( ULongToHandle( obj + FIRST_GDI_HANDLE )));
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
 *           ExtCreatePen    (GDI32.@)
 */
HPEN WINAPI ExtCreatePen( DWORD style, DWORD width, const LOGBRUSH *brush, DWORD style_count,
                          const DWORD *style_bits )
{
    ULONG brush_style = brush->lbStyle;
    ULONG_PTR hatch = brush->lbHatch;
    HPEN pen;

    if (brush_style == BS_DIBPATTERN)
    {
        if (!(hatch = (ULONG_PTR)GlobalLock( (HGLOBAL)hatch ))) return 0;
        brush_style = BS_DIBPATTERNPT;
    }

    pen = NtGdiExtCreatePen( style, width, brush_style, brush->lbColor, brush->lbHatch, hatch,
                             style_count, style_bits, /* FIXME */ 0, FALSE, NULL );

    if (brush->lbStyle == BS_DIBPATTERN) GlobalUnlock( (HGLOBAL)brush->lbHatch );
    return pen;
}

/***********************************************************************
 *           CreateBrushIndirect    (GDI32.@)
 */
HBRUSH WINAPI CreateBrushIndirect( const LOGBRUSH *brush )
{
    switch (brush->lbStyle)
    {
    case BS_NULL:
        return GetStockObject( NULL_BRUSH );
    case BS_SOLID:
        return CreateSolidBrush( brush->lbColor );
    case BS_HATCHED:
        return CreateHatchBrush( brush->lbHatch, brush->lbColor );
    case BS_PATTERN:
    case BS_PATTERN8X8:
        return CreatePatternBrush( (HBITMAP)brush->lbHatch );
    case BS_DIBPATTERN:
        return CreateDIBPatternBrush( (HGLOBAL)brush->lbHatch, brush->lbColor );
    case BS_DIBPATTERNPT:
        return CreateDIBPatternBrushPt( (void *)brush->lbHatch, brush->lbColor );
    default:
        WARN( "invalid brush style %u\n", brush->lbStyle );
        return 0;
    }
}

/***********************************************************************
 *           CreateSolidBrush    (GDI32.@)
 */
HBRUSH WINAPI CreateSolidBrush( COLORREF color )
{
    return NtGdiCreateSolidBrush( color, NULL );
}

/***********************************************************************
 *           CreateHatchBrush    (GDI32.@)
 */
HBRUSH WINAPI CreateHatchBrush( INT style, COLORREF color )
{
    return NtGdiCreateHatchBrushInternal( style, color, FALSE );
}

/***********************************************************************
 *           CreatePatternBrush    (GDI32.@)
 */
HBRUSH WINAPI CreatePatternBrush( HBITMAP bitmap )
{
    return NtGdiCreatePatternBrushInternal( bitmap, FALSE, FALSE );
}

/***********************************************************************
 *           CreateDIBPatternBrush    (GDI32.@)
 */
HBRUSH WINAPI CreateDIBPatternBrush( HGLOBAL hbitmap, UINT coloruse )
{
    HBRUSH brush;
    void *mem;

    TRACE( "%p\n", hbitmap );

    if (!(mem = GlobalLock( hbitmap ))) return 0;
    brush = NtGdiCreateDIBBrush( mem, coloruse, /* FIXME */ 0, FALSE, FALSE, hbitmap );
    GlobalUnlock( hbitmap );
    return brush;
}

/***********************************************************************
 *           CreateDIBPatternBrushPt    (GDI32.@)
 */
HBRUSH WINAPI CreateDIBPatternBrushPt( const void *data, UINT coloruse )
{
    return NtGdiCreateDIBBrush( data, coloruse, /* FIXME */ 0, FALSE, FALSE, data );
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
 *           CreateCompatibleBitmap    (GDI32.@)
 *
 * Creates a bitmap compatible with the DC.
 */
HBITMAP WINAPI CreateCompatibleBitmap( HDC hdc, INT width, INT height )
{
    if (!width || !height)
        return GetStockObject( STOCK_LAST + 1 ); /* default 1x1 bitmap */

    return NtGdiCreateCompatibleBitmap( hdc, width, height );
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
    ULONG ret = NtGdiPolyPolyDraw( ULongToHandle(mode), points, (const ULONG *)counts,
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
 *           MirrorRgn    (GDI32.@)
 */
BOOL WINAPI MirrorRgn( HWND hwnd, HRGN hrgn )
{
    return NtUserMirrorRgn( hwnd, hrgn );
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

/***********************************************************************
 *           CreatePalette     (GDI32.@)
 */
HPALETTE WINAPI CreatePalette( const LOGPALETTE *palette )
{
    if (!palette) return 0;
    return NtGdiCreatePaletteInternal( palette, palette->palNumEntries );
}

/***********************************************************************
 *           GetPaletteEntries    (GDI32.@)
 */
UINT WINAPI GetPaletteEntries( HPALETTE palette, UINT start, UINT count, PALETTEENTRY *entries )
{
    return NtGdiDoPalette( palette, start, count, entries, NtGdiGetPaletteEntries, TRUE );
}

/***********************************************************************
 *           SetPaletteEntries    (GDI32.@)
 */
UINT WINAPI SetPaletteEntries( HPALETTE palette, UINT start, UINT count,
                               const PALETTEENTRY *entries )
{
    palette = get_full_gdi_handle( palette );
    return NtGdiDoPalette( palette, start, count, (void *)entries, NtGdiSetPaletteEntries, FALSE );
}

/***********************************************************************
 *           AnimatePalette    (GDI32.@)
 */
BOOL WINAPI AnimatePalette( HPALETTE palette, UINT start, UINT count, const PALETTEENTRY *entries )
{
    palette = get_full_gdi_handle( palette );
    return NtGdiDoPalette( palette, start, count, (void *)entries, NtGdiAnimatePalette, FALSE );
}

/* first and last 10 entries are the default system palette entries */
static const PALETTEENTRY default_system_palette_low[] =
{
    { 0x00, 0x00, 0x00 }, { 0x80, 0x00, 0x00 }, { 0x00, 0x80, 0x00 }, { 0x80, 0x80, 0x00 },
    { 0x00, 0x00, 0x80 }, { 0x80, 0x00, 0x80 }, { 0x00, 0x80, 0x80 }, { 0xc0, 0xc0, 0xc0 },
    { 0xc0, 0xdc, 0xc0 }, { 0xa6, 0xca, 0xf0 }
};
static const PALETTEENTRY default_system_palette_high[] =
{
    { 0xff, 0xfb, 0xf0 }, { 0xa0, 0xa0, 0xa4 }, { 0x80, 0x80, 0x80 }, { 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0x00 }, { 0xff, 0xff, 0x00 }, { 0x00, 0x00, 0xff }, { 0xff, 0x00, 0xff },
    { 0x00, 0xff, 0xff }, { 0xff, 0xff, 0xff }
};

/***********************************************************************
 *           GetSystemPaletteEntries    (GDI32.@)
 *
 * Gets range of palette entries.
 */
UINT WINAPI GetSystemPaletteEntries( HDC hdc, UINT start, UINT count, PALETTEENTRY *entries )
{
    UINT i, ret;

    ret = NtGdiDoPalette( hdc, start, count, (void *)entries,
                          NtGdiGetSystemPaletteEntries, FALSE );
    if (ret) return ret;

    /* always fill output, even if hdc is an invalid handle */
    if (!entries || start >= 256) return 0;
    if (start + count > 256) count = 256 - start;

    for (i = 0; i < count; i++)
    {
        if (start + i < 10)
            entries[i] = default_system_palette_low[start + i];
        else if (start + i >= 246)
            entries[i] = default_system_palette_high[start + i - 246];
        else
            memset( &entries[i], 0, sizeof(entries[i]) );
    }

    return 0;
}

/***********************************************************************
 *           CreateDIBitmap    (GDI32.@)
 */
HBITMAP WINAPI CreateDIBitmap( HDC hdc, const BITMAPINFOHEADER *header, DWORD init,
                               const void *bits, const BITMAPINFO *data, UINT coloruse )
{
    int width, height;

    if (!header) return 0;

    if (header->biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)header;
        width  = core->bcWidth;
        height = core->bcHeight;
    }
    else if (header->biSize >= sizeof(BITMAPINFOHEADER))
    {
        if (header->biCompression == BI_JPEG || header->biCompression == BI_PNG) return 0;
        width  = header->biWidth;
        height = header->biHeight;
    }
    else return 0;

    if (!width || !height) return GetStockObject( STOCK_LAST + 1 ); /* default 1x1 bitmap */
    return NtGdiCreateDIBitmapInternal( hdc, width, height, init, bits, data, coloruse,
                                        0, 0, 0, 0 );
}

/***********************************************************************
 *           CreateDIBSection    (GDI32.@)
 */
HBITMAP WINAPI DECLSPEC_HOTPATCH CreateDIBSection( HDC hdc, const BITMAPINFO *bmi, UINT usage,
                                                   void **bits, HANDLE section, DWORD offset )
{
    return NtGdiCreateDIBSection( hdc, section, offset, bmi, usage, 0, 0, 0, bits );
}

/***********************************************************************
 *           GetDIBits    (win32u.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetDIBits( HDC hdc, HBITMAP hbitmap, UINT startscan, UINT lines,
                                        void *bits, BITMAPINFO *info, UINT coloruse )
{
    return NtGdiGetDIBitsInternal( hdc, hbitmap, startscan, lines, bits, info, coloruse, 0, 0 );
}

/***********************************************************************
 *           GetDIBColorTable    (GDI32.@)
 */
UINT WINAPI GetDIBColorTable( HDC hdc, UINT start, UINT count, RGBQUAD *colors )
{
    return NtGdiDoPalette( hdc, start, count, colors, NtGdiGetDIBColorTable, TRUE );
}

/***********************************************************************
 *           SetDIBColorTable    (GDI32.@)
 */
UINT WINAPI SetDIBColorTable( HDC hdc, UINT start, UINT count, const RGBQUAD *colors )
{
    return NtGdiDoPalette( hdc, start, count, (void *)colors, NtGdiSetDIBColorTable, FALSE );
}

/***********************************************************************
 *           D3DKMTOpenAdapterFromGdiDisplayName    (GDI32.@)
 */
NTSTATUS WINAPI D3DKMTOpenAdapterFromGdiDisplayName( D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME *desc )
{
    TRACE("(%p)\n", desc);

    if (!desc) return STATUS_UNSUCCESSFUL;
    return NtUserD3DKMTOpenAdapterFromGdiDisplayName( desc );
}

/***********************************************************************
 *           SetObjectOwner    (GDI32.@)
 */
void WINAPI SetObjectOwner( HGDIOBJ handle, HANDLE owner )
{
    /* Nothing to do */
}

/***********************************************************************
 *           GdiInitializeLanguagePack    (GDI32.@)
 */
DWORD WINAPI GdiInitializeLanguagePack( DWORD arg )
{
    FIXME( "stub\n" );
    return 0;
}

/***********************************************************************
 *           GdiGetBatchLimit    (GDI32.@)
 */
DWORD WINAPI GdiGetBatchLimit(void)
{
    return 1;  /* FIXME */
}

/***********************************************************************
 *           GdiSetBatchLimit    (GDI32.@)
 */
DWORD WINAPI GdiSetBatchLimit( DWORD limit )
{
    return 1; /* FIXME */
}

/* Solid colors to enumerate */
static const COLORREF solid_colors[] =
{
    RGB(0x00,0x00,0x00), RGB(0xff,0xff,0xff),
    RGB(0xff,0x00,0x00), RGB(0x00,0xff,0x00),
    RGB(0x00,0x00,0xff), RGB(0xff,0xff,0x00),
    RGB(0xff,0x00,0xff), RGB(0x00,0xff,0xff),
    RGB(0x80,0x00,0x00), RGB(0x00,0x80,0x00),
    RGB(0x80,0x80,0x00), RGB(0x00,0x00,0x80),
    RGB(0x80,0x00,0x80), RGB(0x00,0x80,0x80),
    RGB(0x80,0x80,0x80), RGB(0xc0,0xc0,0xc0)
};

/***********************************************************************
 *           EnumObjects    (GDI32.@)
 */
INT WINAPI EnumObjects( HDC hdc, INT type, GOBJENUMPROC enum_func, LPARAM param )
{
    UINT i;
    INT retval = 0;
    LOGPEN pen;
    LOGBRUSH brush;

    TRACE( "%p %d %p %08Ix\n", hdc, type, enum_func, param );

    switch(type)
    {
    case OBJ_PEN:
        /* Enumerate solid pens */
        for (i = 0; i < ARRAY_SIZE(solid_colors); i++)
        {
            pen.lopnStyle   = PS_SOLID;
            pen.lopnWidth.x = 1;
            pen.lopnWidth.y = 0;
            pen.lopnColor   = solid_colors[i];
            retval = enum_func( &pen, param );
            TRACE( "solid pen %08lx, ret=%d\n", solid_colors[i], retval );
            if (!retval) break;
        }
        break;

    case OBJ_BRUSH:
        /* Enumerate solid brushes */
        for (i = 0; i < ARRAY_SIZE(solid_colors); i++)
        {
            brush.lbStyle = BS_SOLID;
            brush.lbColor = solid_colors[i];
            brush.lbHatch = 0;
            retval = enum_func( &brush, param );
            TRACE( "solid brush %08lx, ret=%d\n", solid_colors[i], retval );
            if (!retval) break;
        }

        /* Now enumerate hatched brushes */
        for (i = HS_HORIZONTAL; retval && i <= HS_DIAGCROSS; i++)
        {
            brush.lbStyle = BS_HATCHED;
            brush.lbColor = RGB(0,0,0);
            brush.lbHatch = i;
            retval = enum_func( &brush, param );
            TRACE( "hatched brush %d, ret=%d\n", i, retval );
        }
        break;

    default:
        /* FIXME: implement Win32 types */
        WARN( "(%d): Invalid type\n", type );
        break;
    }

    return retval;
}

/***********************************************************************
 *           CombineTransform  (GDI32.@)
 *
 * Combines two transformation matrices.
 */
BOOL WINAPI CombineTransform( XFORM *result, const XFORM *xform1, const XFORM *xform2 )
{
    XFORM r;

    if (!result || !xform1 || !xform2) return FALSE;

    /* Create the result in a temporary XFORM, since result may be
     * equal to xform1 or xform2 */
    r.eM11 = xform1->eM11 * xform2->eM11 + xform1->eM12 * xform2->eM21;
    r.eM12 = xform1->eM11 * xform2->eM12 + xform1->eM12 * xform2->eM22;
    r.eM21 = xform1->eM21 * xform2->eM11 + xform1->eM22 * xform2->eM21;
    r.eM22 = xform1->eM21 * xform2->eM12 + xform1->eM22 * xform2->eM22;
    r.eDx  = xform1->eDx  * xform2->eM11 + xform1->eDy  * xform2->eM21 + xform2->eDx;
    r.eDy  = xform1->eDx  * xform2->eM12 + xform1->eDy  * xform2->eM22 + xform2->eDy;

    *result = r;
    return TRUE;
}

/***********************************************************************
 *           LineDDA   (GDI32.@)
 */
BOOL WINAPI LineDDA( INT x_start, INT y_start, INT x_end, INT y_end,
                     LINEDDAPROC callback, LPARAM lparam )
{
    INT xadd = 1, yadd = 1;
    INT err,erradd;
    INT cnt;
    INT dx = x_end - x_start;
    INT dy = y_end - y_start;

    TRACE( "(%d, %d), (%d, %d), %p, %Ix\n", x_start, y_start,
           x_end, y_end, callback, lparam );

    if (dx < 0)
    {
        dx = -dx;
        xadd = -1;
    }
    if (dy < 0)
    {
        dy = -dy;
        yadd = -1;
    }
    if (dx > dy)  /* line is "more horizontal" */
    {
        err = 2*dy - dx; erradd = 2*dy - 2*dx;
        for(cnt = 0;cnt < dx; cnt++)
        {
            callback( x_start, y_start, lparam );
            if (err > 0)
            {
                y_start += yadd;
                err += erradd;
            }
            else err += 2*dy;
            x_start += xadd;
        }
    }
    else   /* line is "more vertical" */
    {
        err = 2*dx - dy; erradd = 2*dx - 2*dy;
        for(cnt = 0; cnt < dy; cnt++)
        {
            callback( x_start, y_start, lparam );
            if (err > 0)
            {
                x_start += xadd;
                err += erradd;
            }
            else err += 2*dx;
            y_start += yadd;
        }
    }
    return TRUE;
}

/***********************************************************************
 *           GdiDllInitialize    (GDI32.@)
 *
 * Stub entry point, some games (CoD: Black Ops 3) call it directly.
 */
BOOL WINAPI GdiDllInitialize( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    FIXME( "stub\n" );
    return TRUE;
}
