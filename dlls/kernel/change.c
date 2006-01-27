/*
 * Win32 file change notification functions
 *
 * Copyright 1998 Ulrich Weigand
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "kernel_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

/****************************************************************************
 *		FindFirstChangeNotificationA (KERNEL32.@)
 */
HANDLE WINAPI FindFirstChangeNotificationA( LPCSTR lpPathName, BOOL bWatchSubtree,
                                            DWORD dwNotifyFilter )
{
    WCHAR *pathW;

    if (!(pathW = FILE_name_AtoW( lpPathName, FALSE ))) return INVALID_HANDLE_VALUE;
    return FindFirstChangeNotificationW( pathW, bWatchSubtree, dwNotifyFilter );
}

/****************************************************************************
 *		FindFirstChangeNotificationW (KERNEL32.@)
 */
HANDLE WINAPI FindFirstChangeNotificationW( LPCWSTR lpPathName, BOOL bWatchSubtree,
                                            DWORD dwNotifyFilter)
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle = INVALID_HANDLE_VALUE;

    TRACE( "%s %d %lx\n", debugstr_w(lpPathName), bWatchSubtree, dwNotifyFilter );

    if (!RtlDosPathNameToNtPathName_U( lpPathName, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return handle;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( &handle, SYNCHRONIZE, &attr, &io,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
    RtlFreeUnicodeString( &nt_name );

    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return INVALID_HANDLE_VALUE;
    }

    status = NtNotifyChangeDirectoryFile( handle, NULL, NULL, NULL, &io,
                                          NULL, 0, dwNotifyFilter, bWatchSubtree );
    if (status != STATUS_PENDING)
    {
        NtClose( handle );
        SetLastError( RtlNtStatusToDosError(status) );
        return INVALID_HANDLE_VALUE;
    }
    return handle;
}

/****************************************************************************
 *		FindNextChangeNotification (KERNEL32.@)
 */
BOOL WINAPI FindNextChangeNotification( HANDLE handle )
{
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    TRACE("%p\n",handle);

    status = NtNotifyChangeDirectoryFile( handle, NULL, NULL, NULL, &io,
                                          NULL, 0, FILE_NOTIFY_CHANGE_SIZE, 0 );
    if (status != STATUS_PENDING)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}

/****************************************************************************
 *		FindCloseChangeNotification (KERNEL32.@)
 */
BOOL WINAPI FindCloseChangeNotification( HANDLE handle )
{
    return CloseHandle( handle );
}

BOOL WINAPI ReadDirectoryChangesW( HANDLE handle, LPVOID buffer, DWORD len, BOOL subtree,
                                   DWORD filter, LPDWORD returned, LPOVERLAPPED overlapped,
                                   LPOVERLAPPED_COMPLETION_ROUTINE completion )
{
    FIXME( "%p %p 0x%08lx %d 0x%08lx %p %p %p\n", handle, buffer, len, subtree, filter,
           returned, overlapped, completion );

    SetLastError( ERROR_INVALID_FUNCTION );
    return FALSE;
}
