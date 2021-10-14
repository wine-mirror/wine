/*
 * WoW64 User functions
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
#include "ntuser.h"
#include "wow64win_private.h"


NTSTATUS WINAPI wow64_NtUserCreateWindowStation( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    ULONG arg3 = get_ulong( &args );
    ULONG arg4 = get_ulong( &args );
    ULONG arg5 = get_ulong( &args );
    ULONG arg6 = get_ulong( &args );
    ULONG arg7 = get_ulong( &args );

    struct object_attr64 attr;

    return HandleToUlong( NtUserCreateWindowStation( objattr_32to64( &attr, attr32 ), access,
                                                     arg3, arg4, arg5, arg6, arg7 ));
}

NTSTATUS WINAPI wow64_NtUserOpenWindowStation( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );

    struct object_attr64 attr;

    return HandleToUlong( NtUserOpenWindowStation( objattr_32to64( &attr, attr32 ), access ));
}

NTSTATUS WINAPI wow64_NtUserCloseWindowStation( UINT *args )
{
    HWINSTA handle = get_handle( &args );

    return NtUserCloseWindowStation( handle );
}

NTSTATUS WINAPI wow64_NtUserGetProcessWindowStation( UINT *args )
{
    return HandleToUlong( NtUserGetProcessWindowStation() );
}

NTSTATUS WINAPI wow64_NtUserSetProcessWindowStation( UINT *args )
{
    HWINSTA handle = get_handle( &args );

    return NtUserSetProcessWindowStation( handle );
}

NTSTATUS WINAPI wow64_NtUserCreateDesktopEx( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    UNICODE_STRING32 *device32 = get_ptr( &args );
    DEVMODEW *devmode = get_ptr( &args );
    DWORD flags = get_ulong( &args );
    ACCESS_MASK access = get_ulong( &args );
    ULONG heap_size = get_ulong( &args );

    struct object_attr64 attr;
    UNICODE_STRING device;
    HANDLE ret;

    ret = NtUserCreateDesktopEx( objattr_32to64( &attr, attr32 ),
                                 unicode_str_32to64( &device, device32 ),
                                 devmode, flags, access, heap_size );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserOpenDesktop( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    DWORD flags = get_ulong( &args );
    ACCESS_MASK access = get_ulong( &args );

    struct object_attr64 attr;
    HANDLE ret;

    ret = NtUserOpenDesktop( objattr_32to64( &attr, attr32 ), flags, access );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserCloseDesktop( UINT *args )
{
    HDESK handle = get_handle( &args );

    return NtUserCloseDesktop( handle );
}

NTSTATUS WINAPI wow64_NtUserGetThreadDesktop( UINT *args )
{
    DWORD thread = get_ulong( &args );

    return HandleToUlong( NtUserGetThreadDesktop( thread ));
}

NTSTATUS WINAPI wow64_NtUserSetThreadDesktop( UINT *args )
{
    HDESK handle = get_handle( &args );

    return NtUserSetThreadDesktop( handle );
}

NTSTATUS WINAPI wow64_NtUserOpenInputDesktop( UINT *args )
{
    DWORD flags = get_ulong( &args );
    BOOL inherit = get_ulong( &args );
    ACCESS_MASK access = get_ulong( &args );

    return HandleToUlong( NtUserOpenInputDesktop( flags, inherit, access ));
}

NTSTATUS WINAPI wow64_NtUserGetObjectInformation( UINT *args )
{
    HANDLE handle = get_handle( &args );
    INT index = get_ulong( &args );
    void *info = get_ptr( &args );
    DWORD len = get_ulong( &args );
    DWORD *needed = get_ptr( &args );

    return NtUserGetObjectInformation( handle, index, info, len, needed );
}

NTSTATUS WINAPI wow64_NtUserSetObjectInformation( UINT *args )
{
    HANDLE handle = get_handle( &args );
    INT index = get_ulong( &args );
    void *info = get_ptr( &args );
    DWORD len = get_ulong( &args );

    return NtUserSetObjectInformation( handle, index, info, len );
}
