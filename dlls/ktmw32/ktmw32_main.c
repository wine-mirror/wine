/*
 * Copyright 2010 Vincent Povirk for CodeWeavers
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
#include "winternl.h"

static inline BOOL set_ntstatus( NTSTATUS status )
{
    if (status) RtlSetLastWin32Error( RtlNtStatusToDosError( status ));
    return !status;
}

static inline LARGE_INTEGER *get_nt_timeout( LARGE_INTEGER *time, DWORD timeout )
{
    if (timeout == INFINITE) return NULL;
    time->QuadPart = (ULONGLONG)timeout * -10000;
    return time;
}

/***********************************************************************
 * CommitTransaction (ktmw32.@)
 */
BOOL WINAPI CommitTransaction(HANDLE transaction)
{
    return set_ntstatus( NtCommitTransaction( transaction, TRUE ));
}

/***********************************************************************
 * CreateTransaction (ktmw32.@)
 */
HANDLE WINAPI CreateTransaction( SECURITY_ATTRIBUTES *sa, GUID *guid, DWORD options, DWORD level, DWORD flags,
        DWORD timeout, WCHAR *desc )
{
    ULONG obj_flags = OBJ_CASE_INSENSITIVE;
    UNICODE_STRING desc_str;
    OBJECT_ATTRIBUTES attr;
    LARGE_INTEGER time;
    HANDLE handle;

    if (sa && sa->bInheritHandle) obj_flags |= OBJ_INHERIT;
    InitializeObjectAttributes( &attr, NULL, obj_flags, 0, sa ? sa->lpSecurityDescriptor : NULL );

    RtlInitUnicodeString( &desc_str, desc );

    if (!set_ntstatus( NtCreateTransaction( &handle, 0 /* FIXME */, &attr, guid, NULL, options, level, flags,
            get_nt_timeout( &time, timeout ), &desc_str )))
    {
        return INVALID_HANDLE_VALUE;
    }

    return handle;
}

/***********************************************************************
 * RollbackTransaction (ktmw32.@)
 */
BOOL WINAPI RollbackTransaction(HANDLE transaction)
{
    return set_ntstatus( NtRollbackTransaction( transaction, TRUE ));
}
