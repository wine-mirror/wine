/*
 * File handling functions
 *
 * Copyright 1993 John Burton
 * Copyright 1996 Alexandre Julliard
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
 *
 * TODO:
 *    Fix the CopyFileEx methods to implement the "extended" functionality.
 *    Right now, they simply call the CopyFile method.
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winerror.h"
#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "wincon.h"
#include "kernel_private.h"

#include "wine/unicode.h"
#include "wine/debug.h"
#include "async.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

/**************************************************************************
 *                      Operations on file handles                        *
 **************************************************************************/

/***********************************************************************
 *              GetOverlappedResult     (KERNEL32.@)
 *
 * Check the result of an Asynchronous data transfer from a file.
 *
 * Parameters
 *   HANDLE hFile                 [in] handle of file to check on
 *   LPOVERLAPPED lpOverlapped    [in/out] pointer to overlapped
 *   LPDWORD lpTransferred        [in/out] number of bytes transferred
 *   BOOL bWait                   [in] wait for the transfer to complete ?
 *
 * RETURNS
 *   TRUE on success
 *   FALSE on failure
 *
 *  If successful (and relevant) lpTransferred will hold the number of
 *   bytes transferred during the async operation.
 *
 * BUGS
 *
 * Currently only works for WaitCommEvent, ReadFile, WriteFile
 *   with communications ports.
 *
 */
BOOL WINAPI GetOverlappedResult(HANDLE hFile, LPOVERLAPPED lpOverlapped,
                                LPDWORD lpTransferred, BOOL bWait)
{
    DWORD r;

    TRACE("(%p %p %p %x)\n", hFile, lpOverlapped, lpTransferred, bWait);

    if (lpOverlapped==NULL)
    {
        ERR("lpOverlapped was null\n");
        return FALSE;
    }
    if (!lpOverlapped->hEvent)
    {
        ERR("lpOverlapped->hEvent was null\n");
        return FALSE;
    }

    if ( bWait )
    {
        do {
            TRACE("waiting on %p\n",lpOverlapped);
            r = WaitForSingleObjectEx(lpOverlapped->hEvent, INFINITE, TRUE);
            TRACE("wait on %p returned %ld\n",lpOverlapped,r);
        } while (r==STATUS_USER_APC);
    }
    else if ( lpOverlapped->Internal == STATUS_PENDING )
    {
        /* Wait in order to give APCs a chance to run. */
        /* This is cheating, so we must set the event again in case of success -
           it may be a non-manual reset event. */
        do {
            TRACE("waiting on %p\n",lpOverlapped);
            r = WaitForSingleObjectEx(lpOverlapped->hEvent, 0, TRUE);
            TRACE("wait on %p returned %ld\n",lpOverlapped,r);
        } while (r==STATUS_USER_APC);
        if ( r == WAIT_OBJECT_0 )
            NtSetEvent ( lpOverlapped->hEvent, NULL );
    }

    if(lpTransferred)
        *lpTransferred = lpOverlapped->InternalHigh;

    switch ( lpOverlapped->Internal )
    {
    case STATUS_SUCCESS:
        return TRUE;
    case STATUS_PENDING:
        SetLastError ( ERROR_IO_INCOMPLETE );
        if ( bWait ) ERR ("PENDING status after waiting!\n");
        return FALSE;
    default:
        SetLastError ( RtlNtStatusToDosError ( lpOverlapped->Internal ) );
        return FALSE;
    }
}

/***********************************************************************
 *             CancelIo                   (KERNEL32.@)
 */
BOOL WINAPI CancelIo(HANDLE handle)
{
    async_private *ovp,*t;

    TRACE("handle = %p\n",handle);

    for (ovp = NtCurrentTeb()->pending_list; ovp; ovp = t)
    {
        t = ovp->next;
        if ( ovp->handle == handle )
             cancel_async ( ovp );
    }
    SleepEx(1,TRUE);
    return TRUE;
}

/***********************************************************************
 *           _hread   (KERNEL32.@)
 */
LONG WINAPI _hread( HFILE hFile, LPVOID buffer, LONG count)
{
    return _lread( hFile, buffer, count );
}


/***********************************************************************
 *           _hwrite   (KERNEL32.@)
 *
 *	experimentation yields that _lwrite:
 *		o truncates the file at the current position with
 *		  a 0 len write
 *		o returns 0 on a 0 length write
 *		o works with console handles
 *
 */
LONG WINAPI _hwrite( HFILE handle, LPCSTR buffer, LONG count )
{
    DWORD result;

    TRACE("%d %p %ld\n", handle, buffer, count );

    if (!count)
    {
        /* Expand or truncate at current position */
        if (!SetEndOfFile( (HANDLE)handle )) return HFILE_ERROR;
        return 0;
    }
    if (!WriteFile( (HANDLE)handle, buffer, count, &result, NULL ))
        return HFILE_ERROR;
    return result;
}


/***********************************************************************
 *           _lclose   (KERNEL32.@)
 */
HFILE WINAPI _lclose( HFILE hFile )
{
    TRACE("handle %d\n", hFile );
    return CloseHandle( (HANDLE)hFile ) ? 0 : HFILE_ERROR;
}


/***********************************************************************
 *           _lcreat   (KERNEL32.@)
 */
HFILE WINAPI _lcreat( LPCSTR path, INT attr )
{
    /* Mask off all flags not explicitly allowed by the doc */
    attr &= FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
    TRACE("%s %02x\n", path, attr );
    return (HFILE)CreateFileA( path, GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               CREATE_ALWAYS, attr, 0 );
}


/***********************************************************************
 *           _lopen   (KERNEL32.@)
 */
HFILE WINAPI _lopen( LPCSTR path, INT mode )
{
    DWORD access, sharing;

    TRACE("('%s',%04x)\n", path, mode );
    FILE_ConvertOFMode( mode, &access, &sharing );
    return (HFILE)CreateFileA( path, access, sharing, NULL, OPEN_EXISTING, 0, 0 );
}


/***********************************************************************
 *           _lread   (KERNEL32.@)
 */
UINT WINAPI _lread( HFILE handle, LPVOID buffer, UINT count )
{
    DWORD result;
    if (!ReadFile( (HANDLE)handle, buffer, count, &result, NULL )) return -1;
    return result;
}


/***********************************************************************
 *           _llseek   (KERNEL32.@)
 */
LONG WINAPI _llseek( HFILE hFile, LONG lOffset, INT nOrigin )
{
    return SetFilePointer( (HANDLE)hFile, lOffset, NULL, nOrigin );
}


/***********************************************************************
 *           _lwrite   (KERNEL32.@)
 */
UINT WINAPI _lwrite( HFILE hFile, LPCSTR buffer, UINT count )
{
    return (UINT)_hwrite( hFile, buffer, (LONG)count );
}


/***********************************************************************
 *           FlushFileBuffers   (KERNEL32.@)
 */
BOOL WINAPI FlushFileBuffers( HANDLE hFile )
{
    NTSTATUS            nts;
    IO_STATUS_BLOCK     ioblk;

    if (is_console_handle( hFile ))
    {
        /* this will fail (as expected) for an output handle */
        /* FIXME: wait until FlushFileBuffers is moved to dll/kernel */
        /* return FlushConsoleInputBuffer( hFile ); */
        return TRUE;
    }
    nts = NtFlushBuffersFile( hFile, &ioblk );
    if (nts != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError( nts ) );
        return FALSE;
    }

    return TRUE;
}


/***********************************************************************
 *           GetFileSize   (KERNEL32.@)
 */
DWORD WINAPI GetFileSize( HANDLE hFile, LPDWORD filesizehigh )
{
    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle( hFile, &info )) return -1;
    if (filesizehigh) *filesizehigh = info.nFileSizeHigh;
    return info.nFileSizeLow;
}


/***********************************************************************
 *           GetFileSizeEx   (KERNEL32.@)
 */
BOOL WINAPI GetFileSizeEx( HANDLE hFile, PLARGE_INTEGER lpFileSize )
{
    BY_HANDLE_FILE_INFORMATION info;

    if (!lpFileSize)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!GetFileInformationByHandle( hFile, &info ))
    {
        return FALSE;
    }

    lpFileSize->s.LowPart = info.nFileSizeLow;
    lpFileSize->s.HighPart = info.nFileSizeHigh;

    return TRUE;
}


/**************************************************************************
 *           LockFile   (KERNEL32.@)
 */
BOOL WINAPI LockFile( HANDLE hFile, DWORD offset_low, DWORD offset_high,
                      DWORD count_low, DWORD count_high )
{
    NTSTATUS            status;
    LARGE_INTEGER       count, offset;

    TRACE( "%p %lx%08lx %lx%08lx\n", 
           hFile, offset_high, offset_low, count_high, count_low );

    count.s.LowPart = count_low;
    count.s.HighPart = count_high;
    offset.s.LowPart = offset_low;
    offset.s.HighPart = offset_high;

    status = NtLockFile( hFile, 0, NULL, NULL, 
                         NULL, &offset, &count, NULL, TRUE, TRUE );
    
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/**************************************************************************
 * LockFileEx [KERNEL32.@]
 *
 * Locks a byte range within an open file for shared or exclusive access.
 *
 * RETURNS
 *   success: TRUE
 *   failure: FALSE
 *
 * NOTES
 * Per Microsoft docs, the third parameter (reserved) must be set to 0.
 */
BOOL WINAPI LockFileEx( HANDLE hFile, DWORD flags, DWORD reserved,
                        DWORD count_low, DWORD count_high, LPOVERLAPPED overlapped )
{
    NTSTATUS status;
    LARGE_INTEGER count, offset;

    if (reserved)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    TRACE( "%p %lx%08lx %lx%08lx flags %lx\n",
           hFile, overlapped->OffsetHigh, overlapped->Offset, 
           count_high, count_low, flags );

    count.s.LowPart = count_low;
    count.s.HighPart = count_high;
    offset.s.LowPart = overlapped->Offset;
    offset.s.HighPart = overlapped->OffsetHigh;

    status = NtLockFile( hFile, overlapped->hEvent, NULL, NULL, 
                         NULL, &offset, &count, NULL, 
                         flags & LOCKFILE_FAIL_IMMEDIATELY,
                         flags & LOCKFILE_EXCLUSIVE_LOCK );
    
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/**************************************************************************
 *           UnlockFile   (KERNEL32.@)
 */
BOOL WINAPI UnlockFile( HANDLE hFile, DWORD offset_low, DWORD offset_high,
                        DWORD count_low, DWORD count_high )
{
    NTSTATUS    status;
    LARGE_INTEGER count, offset;

    count.s.LowPart = count_low;
    count.s.HighPart = count_high;
    offset.s.LowPart = offset_low;
    offset.s.HighPart = offset_high;

    status = NtUnlockFile( hFile, NULL, &offset, &count, NULL);
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/**************************************************************************
 *           UnlockFileEx   (KERNEL32.@)
 */
BOOL WINAPI UnlockFileEx( HANDLE hFile, DWORD reserved, DWORD count_low, DWORD count_high,
                          LPOVERLAPPED overlapped )
{
    if (reserved)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (overlapped->hEvent) FIXME("Unimplemented overlapped operation\n");

    return UnlockFile( hFile, overlapped->Offset, overlapped->OffsetHigh, count_low, count_high );
}

/**************************************************************************
 *                      Operations on file names                          *
 **************************************************************************/

/**************************************************************************
 *           ReplaceFileW   (KERNEL32.@)
 *           ReplaceFile    (KERNEL32.@)
 */
BOOL WINAPI ReplaceFileW(LPCWSTR lpReplacedFileName,LPCWSTR lpReplacementFileName,
                         LPCWSTR lpBackupFileName, DWORD dwReplaceFlags,
                         LPVOID lpExclude, LPVOID lpReserved)
{
    FIXME("(%s,%s,%s,%08lx,%p,%p) stub\n",debugstr_w(lpReplacedFileName),debugstr_w(lpReplacementFileName),
                                          debugstr_w(lpBackupFileName),dwReplaceFlags,lpExclude,lpReserved);
    SetLastError(ERROR_UNABLE_TO_MOVE_REPLACEMENT);
    return FALSE;
}


/**************************************************************************
 *           ReplaceFileA (KERNEL32.@)
 */
BOOL WINAPI ReplaceFileA(LPCSTR lpReplacedFileName,LPCSTR lpReplacementFileName,
                         LPCSTR lpBackupFileName, DWORD dwReplaceFlags,
                         LPVOID lpExclude, LPVOID lpReserved)
{
    FIXME("(%s,%s,%s,%08lx,%p,%p) stub\n",lpReplacedFileName,lpReplacementFileName,
                                          lpBackupFileName,dwReplaceFlags,lpExclude,lpReserved);
    SetLastError(ERROR_UNABLE_TO_MOVE_REPLACEMENT);
    return FALSE;
}
