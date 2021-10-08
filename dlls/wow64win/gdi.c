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

NTSTATUS WINAPI wow64_NtGdiFlush( UINT *args )
{
    return NtGdiFlush();
}
