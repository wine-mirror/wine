/*
 * Kernelbase internal definitions
 *
 * Copyright 2019 Alexandre Julliard
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

#ifndef __WINE_KERNELBASE_H
#define __WINE_KERNELBASE_H

#include "windef.h"
#include "winbase.h"

extern WCHAR *file_name_AtoW( LPCSTR name, BOOL alloc ) DECLSPEC_HIDDEN;
extern DWORD file_name_WtoA( LPCWSTR src, INT srclen, LPSTR dest, INT destlen ) DECLSPEC_HIDDEN;

extern const WCHAR windows_dir[] DECLSPEC_HIDDEN;
extern const WCHAR system_dir[] DECLSPEC_HIDDEN;

static inline BOOL is_console_handle(HANDLE h)
{
    return h != INVALID_HANDLE_VALUE && ((UINT_PTR)h & 3) == 3;
}

static inline BOOL set_ntstatus( NTSTATUS status )
{
    if (status) SetLastError( RtlNtStatusToDosError( status ));
    return !status;
}

#endif /* __WINE_KERNELBASE_H */
