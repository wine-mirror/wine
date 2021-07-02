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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
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
    return entry ? entry->Type : 0;
}

/***********************************************************************
 *           GetObjectType    (GDI32.@)
 */
DWORD WINAPI GetObjectType( HGDIOBJ handle )
{
    DWORD result = get_object_type( handle );
    TRACE("%p -> %u\n", handle, result );
    if (!result) SetLastError( ERROR_INVALID_HANDLE );
    return result;
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
    case OBJ_PEN:
    case OBJ_EXTPEN:
        ret = NtGdiSelectPen( hdc, obj );
        break;
    case OBJ_BRUSH:
        ret = NtGdiSelectBrush( hdc, obj );
        break;
    case OBJ_FONT:
        ret = NtGdiSelectFont( hdc, obj );
        break;
    case OBJ_BITMAP:
        ret = NtGdiSelectBitmap( hdc, obj );
        break;
    case OBJ_REGION:
        ret = ULongToHandle(SelectClipRgn( hdc, obj ));
        break;
    default:
        return 0;
    }

    if (!ret) SetLastError( ERROR_INVALID_HANDLE );
    return ret;
}
