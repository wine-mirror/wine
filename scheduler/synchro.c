/*
 * Win32 process and thread synchronisation
 *
 * Copyright 1997 Alexandre Julliard
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

#include "winbase.h"
#include "winternl.h"
#include "../kernel/kernel_private.h" /* FIXME: to be changed when moving file to dlls/kernel */


/***********************************************************************
 *              Sleep  (KERNEL32.@)
 */
VOID WINAPI Sleep( DWORD timeout )
{
    WaitForMultipleObjectsEx( 0, NULL, FALSE, timeout, FALSE );
}

/******************************************************************************
 *              SleepEx   (KERNEL32.@)
 */
DWORD WINAPI SleepEx( DWORD timeout, BOOL alertable )
{
    DWORD ret = WaitForMultipleObjectsEx( 0, NULL, FALSE, timeout, alertable );
    if (ret != WAIT_IO_COMPLETION) ret = 0;
    return ret;
}


/***********************************************************************
 *           WaitForSingleObject   (KERNEL32.@)
 */
DWORD WINAPI WaitForSingleObject( HANDLE handle, DWORD timeout )
{
    return WaitForMultipleObjectsEx( 1, &handle, FALSE, timeout, FALSE );
}


/***********************************************************************
 *           WaitForSingleObjectEx   (KERNEL32.@)
 */
DWORD WINAPI WaitForSingleObjectEx( HANDLE handle, DWORD timeout,
                                    BOOL alertable )
{
    return WaitForMultipleObjectsEx( 1, &handle, FALSE, timeout, alertable );
}


/***********************************************************************
 *           WaitForMultipleObjects   (KERNEL32.@)
 */
DWORD WINAPI WaitForMultipleObjects( DWORD count, const HANDLE *handles,
                                     BOOL wait_all, DWORD timeout )
{
    return WaitForMultipleObjectsEx( count, handles, wait_all, timeout, FALSE );
}


/***********************************************************************
 *           WaitForMultipleObjectsEx   (KERNEL32.@)
 */
DWORD WINAPI WaitForMultipleObjectsEx( DWORD count, const HANDLE *handles,
                                       BOOL wait_all, DWORD timeout,
                                       BOOL alertable )
{
    NTSTATUS status;
    HANDLE hloc[MAXIMUM_WAIT_OBJECTS];
    int i;

    if (count >= MAXIMUM_WAIT_OBJECTS)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return WAIT_FAILED;
    }
    for (i = 0; i < count; i++)
    {
        if ((handles[i] == (HANDLE)STD_INPUT_HANDLE) ||
            (handles[i] == (HANDLE)STD_OUTPUT_HANDLE) ||
            (handles[i] == (HANDLE)STD_ERROR_HANDLE))
            hloc[i] = GetStdHandle( (DWORD)handles[i] );
        else
            hloc[i] = handles[i];

        /* yes, even screen buffer console handles are waitable, and are
         * handled as a handle to the console itself !!
         */
        if (is_console_handle(hloc[i]))
        {
            if (!VerifyConsoleIoHandle(hloc[i]))
            {
                return FALSE;
            }
            hloc[i] = GetConsoleInputWaitHandle();
        }
    }

    if (timeout == INFINITE)
    {
        status = NtWaitForMultipleObjects( count, hloc, wait_all, alertable, NULL );
    }
    else
    {
        LARGE_INTEGER time;

        time.QuadPart = timeout * (ULONGLONG)10000;
        time.QuadPart = -time.QuadPart;
        status = NtWaitForMultipleObjects( count, hloc, wait_all, alertable, &time );
    }

    if (HIWORD(status))  /* is it an error code? */
    {
        SetLastError( RtlNtStatusToDosError(status) );
        status = WAIT_FAILED;
    }
    return status;
}


/***********************************************************************
 *           WaitForSingleObject   (KERNEL.460)
 */
DWORD WINAPI WaitForSingleObject16( HANDLE handle, DWORD timeout )
{
    DWORD retval, mutex_count;

    ReleaseThunkLock( &mutex_count );
    retval = WaitForSingleObject( handle, timeout );
    RestoreThunkLock( mutex_count );
    return retval;
}

/***********************************************************************
 *           WaitForMultipleObjects   (KERNEL.461)
 */
DWORD WINAPI WaitForMultipleObjects16( DWORD count, const HANDLE *handles,
                                       BOOL wait_all, DWORD timeout )
{
    DWORD retval, mutex_count;

    ReleaseThunkLock( &mutex_count );
    retval = WaitForMultipleObjectsEx( count, handles, wait_all, timeout, FALSE );
    RestoreThunkLock( mutex_count );
    return retval;
}

/***********************************************************************
 *           WaitForMultipleObjectsEx   (KERNEL.495)
 */
DWORD WINAPI WaitForMultipleObjectsEx16( DWORD count, const HANDLE *handles,
                                         BOOL wait_all, DWORD timeout, BOOL alertable )
{
    DWORD retval, mutex_count;

    ReleaseThunkLock( &mutex_count );
    retval = WaitForMultipleObjectsEx( count, handles, wait_all, timeout, alertable );
    RestoreThunkLock( mutex_count );
    return retval;
}
