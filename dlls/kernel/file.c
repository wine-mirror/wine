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
#include "wine/winbase16.h"
#include "kernel_private.h"

#include "wine/exception.h"
#include "excpt.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "async.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

HANDLE dos_handles[DOS_TABLE_SIZE];

/* info structure for FindFirstFile handle */
typedef struct
{
    HANDLE           handle;      /* handle to directory */
    CRITICAL_SECTION cs;          /* crit section protecting this structure */
    UNICODE_STRING   mask;        /* file mask */
    BOOL             is_root;     /* is directory the root of the drive? */
    UINT             data_pos;    /* current position in dir data */
    UINT             data_len;    /* length of dir data */
    BYTE             data[8192];  /* directory data */
} FIND_FIRST_INFO;


static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

/**************************************************************************
 *                      Operations on file handles                        *
 **************************************************************************/

/***********************************************************************
 *           FILE_InitProcessDosHandles
 *
 * Allocates the default DOS handles for a process. Called either by
 * Win32HandleToDosFileHandle below or by the DOSVM stuff.
 */
static void FILE_InitProcessDosHandles( void )
{
    static BOOL init_done /* = FALSE */;
    HANDLE cp = GetCurrentProcess();

    if (init_done) return;
    init_done = TRUE;
    DuplicateHandle(cp, GetStdHandle(STD_INPUT_HANDLE), cp, &dos_handles[0],
                    0, TRUE, DUPLICATE_SAME_ACCESS);
    DuplicateHandle(cp, GetStdHandle(STD_OUTPUT_HANDLE), cp, &dos_handles[1],
                    0, TRUE, DUPLICATE_SAME_ACCESS);
    DuplicateHandle(cp, GetStdHandle(STD_ERROR_HANDLE), cp, &dos_handles[2],
                    0, TRUE, DUPLICATE_SAME_ACCESS);
    DuplicateHandle(cp, GetStdHandle(STD_ERROR_HANDLE), cp, &dos_handles[3],
                    0, TRUE, DUPLICATE_SAME_ACCESS);
    DuplicateHandle(cp, GetStdHandle(STD_ERROR_HANDLE), cp, &dos_handles[4],
                    0, TRUE, DUPLICATE_SAME_ACCESS);
}


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
    if (!ReadFile( (HANDLE)handle, buffer, count, &result, NULL ))
        return HFILE_ERROR;
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

    lpFileSize->u.LowPart = info.nFileSizeLow;
    lpFileSize->u.HighPart = info.nFileSizeHigh;

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

    count.u.LowPart = count_low;
    count.u.HighPart = count_high;
    offset.u.LowPart = offset_low;
    offset.u.HighPart = offset_high;

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

    count.u.LowPart = count_low;
    count.u.HighPart = count_high;
    offset.u.LowPart = overlapped->Offset;
    offset.u.HighPart = overlapped->OffsetHigh;

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

    count.u.LowPart = count_low;
    count.u.HighPart = count_high;
    offset.u.LowPart = offset_low;
    offset.u.HighPart = offset_high;

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


/***********************************************************************
 *           Win32HandleToDosFileHandle   (KERNEL32.21)
 *
 * Allocate a DOS handle for a Win32 handle. The Win32 handle is no
 * longer valid after this function (even on failure).
 *
 * Note: this is not exactly right, since on Win95 the Win32 handles
 *       are on top of DOS handles and we do it the other way
 *       around. Should be good enough though.
 */
HFILE WINAPI Win32HandleToDosFileHandle( HANDLE handle )
{
    int i;

    if (!handle || (handle == INVALID_HANDLE_VALUE))
        return HFILE_ERROR;

    FILE_InitProcessDosHandles();
    for (i = 0; i < DOS_TABLE_SIZE; i++)
        if (!dos_handles[i])
        {
            dos_handles[i] = handle;
            TRACE("Got %d for h32 %p\n", i, handle );
            return (HFILE)i;
        }
    CloseHandle( handle );
    SetLastError( ERROR_TOO_MANY_OPEN_FILES );
    return HFILE_ERROR;
}


/***********************************************************************
 *           DosFileHandleToWin32Handle   (KERNEL32.20)
 *
 * Return the Win32 handle for a DOS handle.
 *
 * Note: this is not exactly right, since on Win95 the Win32 handles
 *       are on top of DOS handles and we do it the other way
 *       around. Should be good enough though.
 */
HANDLE WINAPI DosFileHandleToWin32Handle( HFILE handle )
{
    HFILE16 hfile = (HFILE16)handle;
    if (hfile < 5) FILE_InitProcessDosHandles();
    if ((hfile >= DOS_TABLE_SIZE) || !dos_handles[hfile])
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return INVALID_HANDLE_VALUE;
    }
    return dos_handles[hfile];
}


/***********************************************************************
 *           DisposeLZ32Handle   (KERNEL32.22)
 *
 * Note: this is not entirely correct, we should only close the
 *       32-bit handle and not the 16-bit one, but we cannot do
 *       this because of the way our DOS handles are implemented.
 *       It shouldn't break anything though.
 */
void WINAPI DisposeLZ32Handle( HANDLE handle )
{
    int i;

    if (!handle || (handle == INVALID_HANDLE_VALUE)) return;

    for (i = 5; i < DOS_TABLE_SIZE; i++)
        if (dos_handles[i] == handle)
        {
            dos_handles[i] = 0;
            CloseHandle( handle );
            break;
        }
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


/*************************************************************************
 *           FindFirstFileExW  (KERNEL32.@)
 */
HANDLE WINAPI FindFirstFileExW( LPCWSTR filename, FINDEX_INFO_LEVELS level,
                                LPVOID data, FINDEX_SEARCH_OPS search_op,
                                LPVOID filter, DWORD flags)
{
    WCHAR buffer[MAX_PATH];
    WCHAR *mask, *tmp = buffer;
    FIND_FIRST_INFO *info = NULL;
    DWORD size;

    if ((search_op != FindExSearchNameMatch) || (flags != 0))
    {
        FIXME("options not implemented 0x%08x 0x%08lx\n", search_op, flags );
        return INVALID_HANDLE_VALUE;
    }
    if (level != FindExInfoStandard)
    {
        FIXME("info level %d not implemented\n", level );
        return INVALID_HANDLE_VALUE;
    }
    size = RtlGetFullPathName_U( filename, sizeof(buffer), buffer, &mask );
    if (!size)
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }
    if (size > sizeof(buffer))
    {
        tmp = HeapAlloc( GetProcessHeap(), 0, size );
        if (!tmp)
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return INVALID_HANDLE_VALUE;
        }
        size = RtlGetFullPathName_U( filename, size, tmp, &mask );
    }

    if (!mask || !*mask)
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        goto error;
    }

    if (!(info = HeapAlloc( GetProcessHeap(), 0, sizeof(*info))))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto error;
    }

    if (!RtlCreateUnicodeString( &info->mask, mask ))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto error;
    }
    *mask = 0;

    /* check if path is the root of the drive */
    info->is_root = FALSE;
    if (tmp[0] && tmp[1] == ':')
    {
        WCHAR *p = tmp + 2;
        while (*p == '\\') p++;
        info->is_root = (*p == 0);
    }

    info->handle = CreateFileW( tmp, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                                OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    if (info->handle == INVALID_HANDLE_VALUE)
    {
        RtlFreeUnicodeString( &info->mask );
        goto error;
    }

    RtlInitializeCriticalSection( &info->cs );
    info->data_pos = 0;
    info->data_len = 0;
    if (tmp != buffer) HeapFree( GetProcessHeap(), 0, tmp );

    if (!FindNextFileW( (HANDLE)info, data ))
    {
        TRACE( "%s not found\n", debugstr_w(filename) );
        FindClose( (HANDLE)info );
        SetLastError( ERROR_FILE_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }
    return (HANDLE)info;

error:
    if (tmp != buffer) HeapFree( GetProcessHeap(), 0, tmp );
    if (info) HeapFree( GetProcessHeap(), 0, info );
    return INVALID_HANDLE_VALUE;
}


/*************************************************************************
 *           FindNextFileW   (KERNEL32.@)
 */
BOOL WINAPI FindNextFileW( HANDLE handle, WIN32_FIND_DATAW *data )
{
    FIND_FIRST_INFO *info;
    FILE_BOTH_DIR_INFORMATION *dir_info;
    BOOL ret = FALSE;

    if (handle == INVALID_HANDLE_VALUE)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return ret;
    }
    info = (FIND_FIRST_INFO *)handle;

    RtlEnterCriticalSection( &info->cs );

    for (;;)
    {
        if (info->data_pos >= info->data_len)  /* need to read some more data */
        {
            IO_STATUS_BLOCK io;

            NtQueryDirectoryFile( info->handle, 0, NULL, NULL, &io, info->data, sizeof(info->data),
                                  FileBothDirectoryInformation, FALSE, &info->mask, FALSE );
            if (io.u.Status)
            {
                SetLastError( RtlNtStatusToDosError( io.u.Status ) );
                break;
            }
            info->data_len = io.Information;
            info->data_pos = 0;
        }

        dir_info = (FILE_BOTH_DIR_INFORMATION *)(info->data + info->data_pos);

        if (dir_info->NextEntryOffset) info->data_pos += dir_info->NextEntryOffset;
        else info->data_pos = info->data_len;

        /* don't return '.' and '..' in the root of the drive */
        if (info->is_root)
        {
            if (dir_info->FileNameLength == sizeof(WCHAR) && dir_info->FileName[0] == '.') continue;
            if (dir_info->FileNameLength == 2 * sizeof(WCHAR) &&
                dir_info->FileName[0] == '.' && dir_info->FileName[1] == '.') continue;
        }

        data->dwFileAttributes = dir_info->FileAttributes;
        data->ftCreationTime   = *(FILETIME *)&dir_info->CreationTime;
        data->ftLastAccessTime = *(FILETIME *)&dir_info->LastAccessTime;
        data->ftLastWriteTime  = *(FILETIME *)&dir_info->LastWriteTime;
        data->nFileSizeHigh    = dir_info->EndOfFile.QuadPart >> 32;
        data->nFileSizeLow     = (DWORD)dir_info->EndOfFile.QuadPart;
        data->dwReserved0      = 0;
        data->dwReserved1      = 0;

        memcpy( data->cFileName, dir_info->FileName, dir_info->FileNameLength );
        data->cFileName[dir_info->FileNameLength/sizeof(WCHAR)] = 0;
        memcpy( data->cAlternateFileName, dir_info->ShortName, dir_info->ShortNameLength );
        data->cAlternateFileName[dir_info->ShortNameLength/sizeof(WCHAR)] = 0;

        TRACE("returning %s (%s)\n",
              debugstr_w(data->cFileName), debugstr_w(data->cAlternateFileName) );

        ret = TRUE;
        break;
    }

    RtlLeaveCriticalSection( &info->cs );
    return ret;
}


/*************************************************************************
 *           FindClose   (KERNEL32.@)
 */
BOOL WINAPI FindClose( HANDLE handle )
{
    FIND_FIRST_INFO *info = (FIND_FIRST_INFO *)handle;

    if (!handle || handle == INVALID_HANDLE_VALUE) goto error;

    __TRY
    {
        RtlEnterCriticalSection( &info->cs );
        if (info->handle) CloseHandle( info->handle );
        info->handle = 0;
        RtlFreeUnicodeString( &info->mask );
        info->mask.Buffer = NULL;
        info->data_pos = 0;
        info->data_len = 0;
    }
    __EXCEPT(page_fault)
    {
        WARN("Illegal handle %p\n", handle);
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    __ENDTRY

    RtlLeaveCriticalSection( &info->cs );
    RtlDeleteCriticalSection( &info->cs );
    HeapFree(GetProcessHeap(), 0, info);
    return TRUE;

 error:
    SetLastError( ERROR_INVALID_HANDLE );
    return FALSE;
}


/*************************************************************************
 *           FindFirstFileA   (KERNEL32.@)
 */
HANDLE WINAPI FindFirstFileA( LPCSTR lpFileName, WIN32_FIND_DATAA *lpFindData )
{
    return FindFirstFileExA(lpFileName, FindExInfoStandard, lpFindData,
                            FindExSearchNameMatch, NULL, 0);
}

/*************************************************************************
 *           FindFirstFileExA   (KERNEL32.@)
 */
HANDLE WINAPI FindFirstFileExA( LPCSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId,
                                LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp,
                                LPVOID lpSearchFilter, DWORD dwAdditionalFlags)
{
    HANDLE handle;
    WIN32_FIND_DATAA *dataA;
    WIN32_FIND_DATAW dataW;
    UNICODE_STRING pathW;

    if (!lpFileName)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
    }

    if (!RtlCreateUnicodeStringFromAsciiz(&pathW, lpFileName))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }

    handle = FindFirstFileExW(pathW.Buffer, fInfoLevelId, &dataW, fSearchOp, lpSearchFilter, dwAdditionalFlags);
    RtlFreeUnicodeString(&pathW);
    if (handle == INVALID_HANDLE_VALUE) return handle;

    dataA = (WIN32_FIND_DATAA *) lpFindFileData;
    dataA->dwFileAttributes = dataW.dwFileAttributes;
    dataA->ftCreationTime   = dataW.ftCreationTime;
    dataA->ftLastAccessTime = dataW.ftLastAccessTime;
    dataA->ftLastWriteTime  = dataW.ftLastWriteTime;
    dataA->nFileSizeHigh    = dataW.nFileSizeHigh;
    dataA->nFileSizeLow     = dataW.nFileSizeLow;
    WideCharToMultiByte( CP_ACP, 0, dataW.cFileName, -1,
                         dataA->cFileName, sizeof(dataA->cFileName), NULL, NULL );
    WideCharToMultiByte( CP_ACP, 0, dataW.cAlternateFileName, -1,
                         dataA->cAlternateFileName, sizeof(dataA->cAlternateFileName), NULL, NULL );
    return handle;
}


/*************************************************************************
 *           FindFirstFileW   (KERNEL32.@)
 */
HANDLE WINAPI FindFirstFileW( LPCWSTR lpFileName, WIN32_FIND_DATAW *lpFindData )
{
    return FindFirstFileExW(lpFileName, FindExInfoStandard, lpFindData,
                            FindExSearchNameMatch, NULL, 0);
}


/*************************************************************************
 *           FindNextFileA   (KERNEL32.@)
 */
BOOL WINAPI FindNextFileA( HANDLE handle, WIN32_FIND_DATAA *data )
{
    WIN32_FIND_DATAW dataW;

    if (!FindNextFileW( handle, &dataW )) return FALSE;
    data->dwFileAttributes = dataW.dwFileAttributes;
    data->ftCreationTime   = dataW.ftCreationTime;
    data->ftLastAccessTime = dataW.ftLastAccessTime;
    data->ftLastWriteTime  = dataW.ftLastWriteTime;
    data->nFileSizeHigh    = dataW.nFileSizeHigh;
    data->nFileSizeLow     = dataW.nFileSizeLow;
    WideCharToMultiByte( CP_ACP, 0, dataW.cFileName, -1,
                         data->cFileName, sizeof(data->cFileName), NULL, NULL );
    WideCharToMultiByte( CP_ACP, 0, dataW.cAlternateFileName, -1,
                         data->cAlternateFileName,
                         sizeof(data->cAlternateFileName), NULL, NULL );
    return TRUE;
}


/***********************************************************************
 *           OpenFile   (KERNEL32.@)
 */
HFILE WINAPI OpenFile( LPCSTR name, OFSTRUCT *ofs, UINT mode )
{
    HANDLE handle;
    FILETIME filetime;
    WORD filedatetime[2];

    if (!ofs) return HFILE_ERROR;

    TRACE("%s %s %s %s%s%s%s%s%s%s%s%s\n",name,
          ((mode & 0x3 )==OF_READ)?"OF_READ":
          ((mode & 0x3 )==OF_WRITE)?"OF_WRITE":
          ((mode & 0x3 )==OF_READWRITE)?"OF_READWRITE":"unknown",
          ((mode & 0x70 )==OF_SHARE_COMPAT)?"OF_SHARE_COMPAT":
          ((mode & 0x70 )==OF_SHARE_DENY_NONE)?"OF_SHARE_DENY_NONE":
          ((mode & 0x70 )==OF_SHARE_DENY_READ)?"OF_SHARE_DENY_READ":
          ((mode & 0x70 )==OF_SHARE_DENY_WRITE)?"OF_SHARE_DENY_WRITE":
          ((mode & 0x70 )==OF_SHARE_EXCLUSIVE)?"OF_SHARE_EXCLUSIVE":"unknown",
          ((mode & OF_PARSE )==OF_PARSE)?"OF_PARSE ":"",
          ((mode & OF_DELETE )==OF_DELETE)?"OF_DELETE ":"",
          ((mode & OF_VERIFY )==OF_VERIFY)?"OF_VERIFY ":"",
          ((mode & OF_SEARCH )==OF_SEARCH)?"OF_SEARCH ":"",
          ((mode & OF_CANCEL )==OF_CANCEL)?"OF_CANCEL ":"",
          ((mode & OF_CREATE )==OF_CREATE)?"OF_CREATE ":"",
          ((mode & OF_PROMPT )==OF_PROMPT)?"OF_PROMPT ":"",
          ((mode & OF_EXIST )==OF_EXIST)?"OF_EXIST ":"",
          ((mode & OF_REOPEN )==OF_REOPEN)?"OF_REOPEN ":""
        );


    ofs->cBytes = sizeof(OFSTRUCT);
    ofs->nErrCode = 0;
    if (mode & OF_REOPEN) name = ofs->szPathName;

    if (!name) return HFILE_ERROR;

    TRACE("%s %04x\n", name, mode );

    /* the watcom 10.6 IDE relies on a valid path returned in ofs->szPathName
       Are there any cases where getting the path here is wrong?
       Uwe Bonnes 1997 Apr 2 */
    if (!GetFullPathNameA( name, sizeof(ofs->szPathName), ofs->szPathName, NULL )) goto error;

    /* OF_PARSE simply fills the structure */

    if (mode & OF_PARSE)
    {
        ofs->fFixedDisk = (GetDriveTypeA( ofs->szPathName ) != DRIVE_REMOVABLE);
        TRACE("(%s): OF_PARSE, res = '%s'\n", name, ofs->szPathName );
        return 0;
    }

    /* OF_CREATE is completely different from all other options, so
       handle it first */

    if (mode & OF_CREATE)
    {
        DWORD access, sharing;
        FILE_ConvertOFMode( mode, &access, &sharing );
        if ((handle = CreateFileA( name, GENERIC_READ | GENERIC_WRITE,
                                   sharing, NULL, CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL, 0 )) == INVALID_HANDLE_VALUE)
            goto error;
    }
    else
    {
        /* Now look for the file */

        if (!SearchPathA( NULL, name, NULL, sizeof(ofs->szPathName), ofs->szPathName, NULL ))
            goto error;

        TRACE("found %s\n", debugstr_a(ofs->szPathName) );

        if (mode & OF_DELETE)
        {
            if (!DeleteFileA( ofs->szPathName )) goto error;
            TRACE("(%s): OF_DELETE return = OK\n", name);
            return TRUE;
        }

        handle = (HANDLE)_lopen( ofs->szPathName, mode );
        if (handle == INVALID_HANDLE_VALUE) goto error;

        GetFileTime( handle, NULL, NULL, &filetime );
        FileTimeToDosDateTime( &filetime, &filedatetime[0], &filedatetime[1] );
        if ((mode & OF_VERIFY) && (mode & OF_REOPEN))
        {
            if (ofs->Reserved1 != filedatetime[0] || ofs->Reserved2 != filedatetime[1] )
            {
                CloseHandle( handle );
                WARN("(%s): OF_VERIFY failed\n", name );
                /* FIXME: what error here? */
                SetLastError( ERROR_FILE_NOT_FOUND );
                goto error;
            }
        }
        ofs->Reserved1 = filedatetime[0];
        ofs->Reserved2 = filedatetime[1];
    }
    TRACE("(%s): OK, return = %p\n", name, handle );
    if (mode & OF_EXIST)  /* Return TRUE instead of a handle */
    {
        CloseHandle( handle );
        return TRUE;
    }
    else return (HFILE)handle;

error:  /* We get here if there was an error opening the file */
    ofs->nErrCode = GetLastError();
    WARN("(%s): return = HFILE_ERROR error= %d\n", name,ofs->nErrCode );
    return HFILE_ERROR;
}
