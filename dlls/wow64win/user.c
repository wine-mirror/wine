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

NTSTATUS WINAPI wow64_NtUserCloseDesktop( UINT *args )
{
    HDESK handle = get_handle( &args );

    return NtUserCloseDesktop( handle );
}
