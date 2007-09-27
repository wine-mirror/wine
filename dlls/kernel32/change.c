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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "kernel_private.h"
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

/*
 * NtNotifyChangeDirectoryFile may write back to the IO_STATUS_BLOCK
 * asynchronously.  We don't care about the contents, but it can't
 * be placed on the stack since it will go out of scope when we return.
 */
static IO_STATUS_BLOCK FindFirstChange_iosb;

/****************************************************************************
 *		FindFirstChangeNotificationW (KERNEL32.@)
 */
HANDLE WINAPI FindFirstChangeNotificationW( LPCWSTR lpPathName, BOOL bWatchSubtree,
                                            DWORD dwNotifyFilter)
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    HANDLE handle = INVALID_HANDLE_VALUE;

    TRACE( "%s %d %x\n", debugstr_w(lpPathName), bWatchSubtree, dwNotifyFilter );

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

    status = NtOpenFile( &handle, FILE_LIST_DIRECTORY | SYNCHRONIZE,
                         &attr, &FindFirstChange_iosb,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
    RtlFreeUnicodeString( &nt_name );

    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return INVALID_HANDLE_VALUE;
    }

    status = NtNotifyChangeDirectoryFile( handle, NULL, NULL, NULL,
                                          &FindFirstChange_iosb,
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
    NTSTATUS status;

    TRACE("%p\n",handle);

    status = NtNotifyChangeDirectoryFile( handle, NULL, NULL, NULL,
                                          &FindFirstChange_iosb,
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

/****************************************************************************
 *		ReadDirectoryChangesW (KERNEL32.@)
 *
 * NOTES
 *
 *  The filter is remember from the first run and ignored on successive runs.
 *
 *  If there's no output buffer on the first run, it's ignored successive runs
 *   and STATUS_NOTIFY_ENUM_DIRECTORY is returned with an empty buffer.
 *
 *  If a NULL overlapped->hEvent is passed, the directory handle is used
 *   for signalling.
 */
BOOL WINAPI ReadDirectoryChangesW( HANDLE handle, LPVOID buffer, DWORD len, BOOL subtree,
                                   DWORD filter, LPDWORD returned, LPOVERLAPPED overlapped,
                                   LPOVERLAPPED_COMPLETION_ROUTINE completion )
{
    OVERLAPPED ov, *pov;
    IO_STATUS_BLOCK *ios;
    NTSTATUS status;
    BOOL ret = TRUE;
    LPVOID cvalue = NULL;

    TRACE("%p %p %08x %d %08x %p %p %p\n", handle, buffer, len, subtree, filter,
           returned, overlapped, completion );

    if (!overlapped)
    {
        memset( &ov, 0, sizeof ov );
        ov.hEvent = CreateEventW( NULL, 0, 0, NULL );
        pov = &ov;
    }
    else
    {
        pov = overlapped;
        if (!completion && ((ULONG_PTR)overlapped->hEvent & 1) == 0) cvalue = overlapped;
    }

    ios = (PIO_STATUS_BLOCK) pov;
    ios->u.Status = STATUS_PENDING;

    status = NtNotifyChangeDirectoryFile( handle, pov->hEvent, NULL, cvalue,
                                          ios, buffer, len, filter, subtree );
    if (status == STATUS_PENDING)
    {
        if (overlapped)
            return TRUE;

        WaitForSingleObjectEx( ov.hEvent, INFINITE, TRUE );
        CloseHandle( ov.hEvent );
        if (returned)
            *returned = ios->Information;
        status = ios->u.Status;
    }

    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        ret = FALSE;
    }

    return ret;
}
