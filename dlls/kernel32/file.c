/*
 * File handling functions
 *
 * Copyright 1993 John Burton
 * Copyright 1996, 2004 Alexandre Julliard
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
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winerror.h"
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "wincon.h"
#include "wine/winbase16.h"
#include "kernel_private.h"

#include "wine/exception.h"
#include "excpt.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "thread.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

HANDLE dos_handles[DOS_TABLE_SIZE];

/* info structure for FindFirstFile handle */
typedef struct
{
    DWORD             magic;       /* magic number */
    HANDLE            handle;      /* handle to directory */
    CRITICAL_SECTION  cs;          /* crit section protecting this structure */
    FINDEX_SEARCH_OPS search_op;   /* Flags passed to FindFirst.  */
    UNICODE_STRING    mask;        /* file mask */
    UNICODE_STRING    path;        /* NT path used to open the directory */
    BOOL              is_root;     /* is directory the root of the drive? */
    UINT              data_pos;    /* current position in dir data */
    UINT              data_len;    /* length of dir data */
    BYTE              data[8192];  /* directory data */
} FIND_FIRST_INFO;

#define FIND_FIRST_MAGIC  0xc0ffee11

static BOOL oem_file_apis;


/***********************************************************************
 *              create_file_OF
 *
 * Wrapper for CreateFile that takes OF_* mode flags.
 */
static HANDLE create_file_OF( LPCSTR path, INT mode )
{
    DWORD access, sharing, creation;

    if (mode & OF_CREATE)
    {
        creation = CREATE_ALWAYS;
        access = GENERIC_READ | GENERIC_WRITE;
    }
    else
    {
        creation = OPEN_EXISTING;
        switch(mode & 0x03)
        {
        case OF_READ:      access = GENERIC_READ; break;
        case OF_WRITE:     access = GENERIC_WRITE; break;
        case OF_READWRITE: access = GENERIC_READ | GENERIC_WRITE; break;
        default:           access = 0; break;
        }
    }

    switch(mode & 0x70)
    {
    case OF_SHARE_EXCLUSIVE:  sharing = 0; break;
    case OF_SHARE_DENY_WRITE: sharing = FILE_SHARE_READ; break;
    case OF_SHARE_DENY_READ:  sharing = FILE_SHARE_WRITE; break;
    case OF_SHARE_DENY_NONE:
    case OF_SHARE_COMPAT:
    default:                  sharing = FILE_SHARE_READ | FILE_SHARE_WRITE; break;
    }
    return CreateFileA( path, access, sharing, NULL, creation, FILE_ATTRIBUTE_NORMAL, 0 );
}


/***********************************************************************
 *              check_dir_symlink
 *
 * Check if a dir symlink should be returned by FindNextFile.
 */
static BOOL check_dir_symlink( FIND_FIRST_INFO *info, const FILE_BOTH_DIR_INFORMATION *file_info )
{
    UNICODE_STRING str;
    ANSI_STRING unix_name;
    struct stat st, parent_st;
    BOOL ret = TRUE;
    DWORD len;

    str.MaximumLength = info->path.Length + sizeof(WCHAR) + file_info->FileNameLength;
    if (!(str.Buffer = HeapAlloc( GetProcessHeap(), 0, str.MaximumLength ))) return TRUE;
    memcpy( str.Buffer, info->path.Buffer, info->path.Length );
    len = info->path.Length / sizeof(WCHAR);
    if (!len || str.Buffer[len-1] != '\\') str.Buffer[len++] = '\\';
    memcpy( str.Buffer + len, file_info->FileName, file_info->FileNameLength );
    str.Length = len * sizeof(WCHAR) + file_info->FileNameLength;

    unix_name.Buffer = NULL;
    if (!wine_nt_to_unix_file_name( &str, &unix_name, OPEN_EXISTING, FALSE ) &&
        !stat( unix_name.Buffer, &st ))
    {
        char *p = unix_name.Buffer + unix_name.Length - 1;

        /* skip trailing slashes */
        while (p > unix_name.Buffer && *p == '/') p--;

        while (ret && p > unix_name.Buffer)
        {
            while (p > unix_name.Buffer && *p != '/') p--;
            while (p > unix_name.Buffer && *p == '/') p--;
            p[1] = 0;
            if (!stat( unix_name.Buffer, &parent_st ) &&
                parent_st.st_dev == st.st_dev &&
                parent_st.st_ino == st.st_ino)
            {
                WARN( "suppressing dir symlink %s pointing to parent %s\n",
                      debugstr_wn( str.Buffer, str.Length/sizeof(WCHAR) ),
                      debugstr_a( unix_name.Buffer ));
                ret = FALSE;
            }
        }
    }
    RtlFreeAnsiString( &unix_name );
    RtlFreeUnicodeString( &str );
    return ret;
}


/***********************************************************************
 *           FILE_SetDosError
 *
 * Set the DOS error code from errno.
 */
void FILE_SetDosError(void)
{
    int save_errno = errno; /* errno gets overwritten by printf */

    TRACE("errno = %d %s\n", errno, strerror(errno));
    switch (save_errno)
    {
    case EAGAIN:
        SetLastError( ERROR_SHARING_VIOLATION );
        break;
    case EBADF:
        SetLastError( ERROR_INVALID_HANDLE );
        break;
    case ENOSPC:
        SetLastError( ERROR_HANDLE_DISK_FULL );
        break;
    case EACCES:
    case EPERM:
    case EROFS:
        SetLastError( ERROR_ACCESS_DENIED );
        break;
    case EBUSY:
        SetLastError( ERROR_LOCK_VIOLATION );
        break;
    case ENOENT:
        SetLastError( ERROR_FILE_NOT_FOUND );
        break;
    case EISDIR:
        SetLastError( ERROR_CANNOT_MAKE );
        break;
    case ENFILE:
    case EMFILE:
        SetLastError( ERROR_TOO_MANY_OPEN_FILES );
        break;
    case EEXIST:
        SetLastError( ERROR_FILE_EXISTS );
        break;
    case EINVAL:
    case ESPIPE:
        SetLastError( ERROR_SEEK );
        break;
    case ENOTEMPTY:
        SetLastError( ERROR_DIR_NOT_EMPTY );
        break;
    case ENOEXEC:
        SetLastError( ERROR_BAD_FORMAT );
        break;
    case ENOTDIR:
        SetLastError( ERROR_PATH_NOT_FOUND );
        break;
    case EXDEV:
        SetLastError( ERROR_NOT_SAME_DEVICE );
        break;
    default:
        WARN("unknown file error: %s\n", strerror(save_errno) );
        SetLastError( ERROR_GEN_FAILURE );
        break;
    }
    errno = save_errno;
}


/***********************************************************************
 *           FILE_name_AtoW
 *
 * Convert a file name to Unicode, taking into account the OEM/Ansi API mode.
 *
 * If alloc is FALSE uses the TEB static buffer, so it can only be used when
 * there is no possibility for the function to do that twice, taking into
 * account any called function.
 */
WCHAR *FILE_name_AtoW( LPCSTR name, BOOL alloc )
{
    ANSI_STRING str;
    UNICODE_STRING strW, *pstrW;
    NTSTATUS status;

    RtlInitAnsiString( &str, name );
    pstrW = alloc ? &strW : &NtCurrentTeb()->StaticUnicodeString;
    if (oem_file_apis)
        status = RtlOemStringToUnicodeString( pstrW, &str, alloc );
    else
        status = RtlAnsiStringToUnicodeString( pstrW, &str, alloc );
    if (status == STATUS_SUCCESS) return pstrW->Buffer;

    if (status == STATUS_BUFFER_OVERFLOW)
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return NULL;
}


/***********************************************************************
 *           FILE_name_WtoA
 *
 * Convert a file name back to OEM/Ansi. Returns number of bytes copied.
 */
DWORD FILE_name_WtoA( LPCWSTR src, INT srclen, LPSTR dest, INT destlen )
{
    DWORD ret;

    if (srclen < 0) srclen = strlenW( src ) + 1;
    if (oem_file_apis)
        RtlUnicodeToOemN( dest, destlen, &ret, src, srclen * sizeof(WCHAR) );
    else
        RtlUnicodeToMultiByteN( dest, destlen, &ret, src, srclen * sizeof(WCHAR) );
    return ret;
}


/**************************************************************************
 *              SetFileApisToOEM   (KERNEL32.@)
 */
VOID WINAPI SetFileApisToOEM(void)
{
    oem_file_apis = TRUE;
}


/**************************************************************************
 *              SetFileApisToANSI   (KERNEL32.@)
 */
VOID WINAPI SetFileApisToANSI(void)
{
    oem_file_apis = FALSE;
}


/******************************************************************************
 *              AreFileApisANSI   (KERNEL32.@)
 *
 *  Determines if file functions are using ANSI
 *
 * RETURNS
 *    TRUE:  Set of file functions is using ANSI code page
 *    FALSE: Set of file functions is using OEM code page
 */
BOOL WINAPI AreFileApisANSI(void)
{
    return !oem_file_apis;
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


/******************************************************************
 *		FILE_ReadWriteApc (internal)
 */
static void WINAPI FILE_ReadWriteApc(void* apc_user, PIO_STATUS_BLOCK io_status, ULONG reserved)
{
    LPOVERLAPPED_COMPLETION_ROUTINE  cr = (LPOVERLAPPED_COMPLETION_ROUTINE)apc_user;

    cr(RtlNtStatusToDosError(io_status->u.Status), io_status->Information, (LPOVERLAPPED)io_status);
}


/***********************************************************************
 *              ReadFileEx                (KERNEL32.@)
 */
BOOL WINAPI ReadFileEx(HANDLE hFile, LPVOID buffer, DWORD bytesToRead,
                       LPOVERLAPPED overlapped,
                       LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    LARGE_INTEGER       offset;
    NTSTATUS            status;
    PIO_STATUS_BLOCK    io_status;

    TRACE("(hFile=%p, buffer=%p, bytes=%u, ovl=%p, ovl_fn=%p)\n", hFile, buffer, bytesToRead, overlapped, lpCompletionRoutine);

    if (!overlapped)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    offset.u.LowPart = overlapped->u.s.Offset;
    offset.u.HighPart = overlapped->u.s.OffsetHigh;
    io_status = (PIO_STATUS_BLOCK)overlapped;
    io_status->u.Status = STATUS_PENDING;

    status = NtReadFile(hFile, NULL, FILE_ReadWriteApc, lpCompletionRoutine,
                        io_status, buffer, bytesToRead, &offset, NULL);

    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *              ReadFile                (KERNEL32.@)
 */
BOOL WINAPI ReadFile( HANDLE hFile, LPVOID buffer, DWORD bytesToRead,
                      LPDWORD bytesRead, LPOVERLAPPED overlapped )
{
    LARGE_INTEGER       offset;
    PLARGE_INTEGER      poffset = NULL;
    IO_STATUS_BLOCK     iosb;
    PIO_STATUS_BLOCK    io_status = &iosb;
    HANDLE              hEvent = 0;
    NTSTATUS            status;

    TRACE("%p %p %d %p %p\n", hFile, buffer, bytesToRead,
          bytesRead, overlapped );

    if (bytesRead) *bytesRead = 0;  /* Do this before anything else */
    if (!bytesToRead) return TRUE;

    if (is_console_handle(hFile))
        return ReadConsoleA(hFile, buffer, bytesToRead, bytesRead, NULL);

    if (overlapped != NULL)
    {
        offset.u.LowPart = overlapped->u.s.Offset;
        offset.u.HighPart = overlapped->u.s.OffsetHigh;
        poffset = &offset;
        hEvent = overlapped->hEvent;
        io_status = (PIO_STATUS_BLOCK)overlapped;
    }
    io_status->u.Status = STATUS_PENDING;
    io_status->Information = 0;

    status = NtReadFile(hFile, hEvent, NULL, NULL, io_status, buffer, bytesToRead, poffset, NULL);

    if (status != STATUS_PENDING && bytesRead)
        *bytesRead = io_status->Information;

    if (status && status != STATUS_END_OF_FILE && status != STATUS_TIMEOUT)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *              WriteFileEx                (KERNEL32.@)
 */
BOOL WINAPI WriteFileEx(HANDLE hFile, LPCVOID buffer, DWORD bytesToWrite,
                        LPOVERLAPPED overlapped,
                        LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    LARGE_INTEGER       offset;
    NTSTATUS            status;
    PIO_STATUS_BLOCK    io_status;

    TRACE("%p %p %d %p %p\n", hFile, buffer, bytesToWrite, overlapped, lpCompletionRoutine);

    if (overlapped == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    offset.u.LowPart = overlapped->u.s.Offset;
    offset.u.HighPart = overlapped->u.s.OffsetHigh;

    io_status = (PIO_STATUS_BLOCK)overlapped;
    io_status->u.Status = STATUS_PENDING;

    status = NtWriteFile(hFile, NULL, FILE_ReadWriteApc, lpCompletionRoutine,
                         io_status, buffer, bytesToWrite, &offset, NULL);

    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *             WriteFile               (KERNEL32.@)
 */
BOOL WINAPI WriteFile( HANDLE hFile, LPCVOID buffer, DWORD bytesToWrite,
                       LPDWORD bytesWritten, LPOVERLAPPED overlapped )
{
    HANDLE hEvent = NULL;
    LARGE_INTEGER offset;
    PLARGE_INTEGER poffset = NULL;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    PIO_STATUS_BLOCK piosb = &iosb;

    TRACE("%p %p %d %p %p\n", hFile, buffer, bytesToWrite, bytesWritten, overlapped );

    if (is_console_handle(hFile))
        return WriteConsoleA(hFile, buffer, bytesToWrite, bytesWritten, NULL);

    if (overlapped)
    {
        offset.u.LowPart = overlapped->u.s.Offset;
        offset.u.HighPart = overlapped->u.s.OffsetHigh;
        poffset = &offset;
        hEvent = overlapped->hEvent;
        piosb = (PIO_STATUS_BLOCK)overlapped;
    }
    piosb->u.Status = STATUS_PENDING;
    piosb->Information = 0;

    status = NtWriteFile(hFile, hEvent, NULL, NULL, piosb,
                         buffer, bytesToWrite, poffset, NULL);

    /* FIXME: NtWriteFile does not always cause page faults, generate them now */
    if (status == STATUS_INVALID_USER_BUFFER && !IsBadReadPtr( buffer, bytesToWrite ))
    {
        status = NtWriteFile(hFile, hEvent, NULL, NULL, piosb,
                             buffer, bytesToWrite, poffset, NULL);
        if (status != STATUS_INVALID_USER_BUFFER)
            FIXME("Could not access memory (%p,%d) at first, now OK. Protected by DIBSection code?\n",
                  buffer, bytesToWrite);
    }

    if (status != STATUS_PENDING && bytesWritten)
        *bytesWritten = piosb->Information;

    if (status && status != STATUS_TIMEOUT)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
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
    DWORD r = WAIT_OBJECT_0;

    TRACE( "(%p %p %p %x)\n", hFile, lpOverlapped, lpTransferred, bWait );

    if ( lpOverlapped == NULL )
    {
        ERR("lpOverlapped was null\n");
        return FALSE;
    }
    if ( bWait )
    {
        if ( lpOverlapped->hEvent )
        {
            do
            {
                TRACE( "waiting on %p\n", lpOverlapped );
                r = WaitForSingleObjectEx( lpOverlapped->hEvent, INFINITE, TRUE );
                TRACE( "wait on %p returned %d\n", lpOverlapped, r );
            } while ( r == WAIT_IO_COMPLETION );
        }
        else
        {
            /* busy loop */
            while ( ((volatile OVERLAPPED*)lpOverlapped)->Internal == STATUS_PENDING )
                Sleep( 10 );
        }
    }
    else if ( lpOverlapped->Internal == STATUS_PENDING )
    {
        /* Wait in order to give APCs a chance to run. */
        /* This is cheating, so we must set the event again in case of success -
           it may be a non-manual reset event. */
        do
        {
            TRACE( "waiting on %p\n", lpOverlapped );
            r = WaitForSingleObjectEx( lpOverlapped->hEvent, 0, TRUE );
            TRACE( "wait on %p returned %d\n", lpOverlapped, r );
        } while ( r == WAIT_IO_COMPLETION );
        if ( r == WAIT_OBJECT_0 && lpOverlapped->hEvent )
            NtSetEvent( lpOverlapped->hEvent, NULL );
    }
    if ( r == WAIT_FAILED )
    {
        WARN("wait operation failed\n");
        return FALSE;
    }
    if (lpTransferred) *lpTransferred = lpOverlapped->InternalHigh;

    switch ( lpOverlapped->Internal )
    {
    case STATUS_SUCCESS:
        return TRUE;
    case STATUS_PENDING:
        SetLastError( ERROR_IO_INCOMPLETE );
        if ( bWait ) ERR("PENDING status after waiting!\n");
        return FALSE;
    default:
        SetLastError( RtlNtStatusToDosError( lpOverlapped->Internal ) );
        return FALSE;
    }
}

/***********************************************************************
 *             CancelIo                   (KERNEL32.@)
 *
 * Cancels pending I/O operations initiated by the current thread on a file.
 *
 * PARAMS
 *  handle [I] File handle.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE, check GetLastError().
 */
BOOL WINAPI CancelIo(HANDLE handle)
{
    IO_STATUS_BLOCK    io_status;

    NtCancelIoFile(handle, &io_status);
    if (io_status.u.Status)
    {
        SetLastError( RtlNtStatusToDosError( io_status.u.Status ) );
        return FALSE;
    }
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

    TRACE("%d %p %d\n", handle, buffer, count );

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
    TRACE("(%s,%04x)\n", debugstr_a(path), mode );
    return (HFILE)create_file_OF( path, mode & ~OF_CREATE );
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
        return FlushConsoleInputBuffer( hFile );
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
 *           GetFileType   (KERNEL32.@)
 */
DWORD WINAPI GetFileType( HANDLE hFile )
{
    FILE_FS_DEVICE_INFORMATION info;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    if (is_console_handle( hFile )) return FILE_TYPE_CHAR;

    status = NtQueryVolumeInformationFile( hFile, &io, &info, sizeof(info), FileFsDeviceInformation );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FILE_TYPE_UNKNOWN;
    }

    switch(info.DeviceType)
    {
    case FILE_DEVICE_NULL:
    case FILE_DEVICE_SERIAL_PORT:
    case FILE_DEVICE_PARALLEL_PORT:
    case FILE_DEVICE_TAPE:
    case FILE_DEVICE_UNKNOWN:
        return FILE_TYPE_CHAR;
    case FILE_DEVICE_NAMED_PIPE:
        return FILE_TYPE_PIPE;
    default:
        return FILE_TYPE_DISK;
    }
}


/***********************************************************************
 *             GetFileInformationByHandle   (KERNEL32.@)
 */
BOOL WINAPI GetFileInformationByHandle( HANDLE hFile, BY_HANDLE_FILE_INFORMATION *info )
{
    FILE_ALL_INFORMATION all_info;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtQueryInformationFile( hFile, &io, &all_info, sizeof(all_info), FileAllInformation );
    if (status == STATUS_SUCCESS)
    {
        info->dwFileAttributes                = all_info.BasicInformation.FileAttributes;
        info->ftCreationTime.dwHighDateTime   = all_info.BasicInformation.CreationTime.u.HighPart;
        info->ftCreationTime.dwLowDateTime    = all_info.BasicInformation.CreationTime.u.LowPart;
        info->ftLastAccessTime.dwHighDateTime = all_info.BasicInformation.LastAccessTime.u.HighPart;
        info->ftLastAccessTime.dwLowDateTime  = all_info.BasicInformation.LastAccessTime.u.LowPart;
        info->ftLastWriteTime.dwHighDateTime  = all_info.BasicInformation.LastWriteTime.u.HighPart;
        info->ftLastWriteTime.dwLowDateTime   = all_info.BasicInformation.LastWriteTime.u.LowPart;
        info->dwVolumeSerialNumber            = 0;  /* FIXME */
        info->nFileSizeHigh                   = all_info.StandardInformation.EndOfFile.u.HighPart;
        info->nFileSizeLow                    = all_info.StandardInformation.EndOfFile.u.LowPart;
        info->nNumberOfLinks                  = all_info.StandardInformation.NumberOfLinks;
        info->nFileIndexHigh                  = all_info.InternalInformation.IndexNumber.u.HighPart;
        info->nFileIndexLow                   = all_info.InternalInformation.IndexNumber.u.LowPart;
        return TRUE;
    }
    SetLastError( RtlNtStatusToDosError(status) );
    return FALSE;
}


/***********************************************************************
 *           GetFileSize   (KERNEL32.@)
 *
 * Retrieve the size of a file.
 *
 * PARAMS
 *  hFile        [I] File to retrieve size of.
 *  filesizehigh [O] On return, the high bits of the file size.
 *
 * RETURNS
 *  Success: The low bits of the file size.
 *  Failure: INVALID_FILE_SIZE. As this is could also be a success value,
 *           check GetLastError() for values other than ERROR_SUCCESS.
 */
DWORD WINAPI GetFileSize( HANDLE hFile, LPDWORD filesizehigh )
{
    LARGE_INTEGER size;
    if (!GetFileSizeEx( hFile, &size )) return INVALID_FILE_SIZE;
    if (filesizehigh) *filesizehigh = size.u.HighPart;
    if (size.u.LowPart == INVALID_FILE_SIZE) SetLastError(0);
    return size.u.LowPart;
}


/***********************************************************************
 *           GetFileSizeEx   (KERNEL32.@)
 *
 * Retrieve the size of a file.
 *
 * PARAMS
 *  hFile        [I] File to retrieve size of.
 *  lpFileSIze   [O] On return, the size of the file.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE, check GetLastError().
 */
BOOL WINAPI GetFileSizeEx( HANDLE hFile, PLARGE_INTEGER lpFileSize )
{
    FILE_END_OF_FILE_INFORMATION info;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtQueryInformationFile( hFile, &io, &info, sizeof(info), FileEndOfFileInformation );
    if (status == STATUS_SUCCESS)
    {
        *lpFileSize = info.EndOfFile;
        return TRUE;
    }
    SetLastError( RtlNtStatusToDosError(status) );
    return FALSE;
}


/**************************************************************************
 *           SetEndOfFile   (KERNEL32.@)
 *
 * Sets the current position as the end of the file.
 *
 * PARAMS
 *  hFile [I] File handle.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE, check GetLastError().
 */
BOOL WINAPI SetEndOfFile( HANDLE hFile )
{
    FILE_POSITION_INFORMATION pos;
    FILE_END_OF_FILE_INFORMATION eof;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtQueryInformationFile( hFile, &io, &pos, sizeof(pos), FilePositionInformation );
    if (status == STATUS_SUCCESS)
    {
        eof.EndOfFile = pos.CurrentByteOffset;
        status = NtSetInformationFile( hFile, &io, &eof, sizeof(eof), FileEndOfFileInformation );
    }
    if (status == STATUS_SUCCESS) return TRUE;
    SetLastError( RtlNtStatusToDosError(status) );
    return FALSE;
}


/***********************************************************************
 *           SetFilePointer   (KERNEL32.@)
 */
DWORD WINAPI SetFilePointer( HANDLE hFile, LONG distance, LONG *highword, DWORD method )
{
    LARGE_INTEGER dist, newpos;

    if (highword)
    {
        dist.u.LowPart  = distance;
        dist.u.HighPart = *highword;
    }
    else dist.QuadPart = distance;

    if (!SetFilePointerEx( hFile, dist, &newpos, method )) return INVALID_SET_FILE_POINTER;

    if (highword) *highword = newpos.u.HighPart;
    if (newpos.u.LowPart == INVALID_SET_FILE_POINTER) SetLastError( 0 );
    return newpos.u.LowPart;
}


/***********************************************************************
 *           SetFilePointerEx   (KERNEL32.@)
 */
BOOL WINAPI SetFilePointerEx( HANDLE hFile, LARGE_INTEGER distance,
                              LARGE_INTEGER *newpos, DWORD method )
{
    LONGLONG pos;
    IO_STATUS_BLOCK io;
    FILE_POSITION_INFORMATION info;

    switch(method)
    {
    case FILE_BEGIN:
        pos = distance.QuadPart;
        break;
    case FILE_CURRENT:
        if (NtQueryInformationFile( hFile, &io, &info, sizeof(info), FilePositionInformation ))
            goto error;
        pos = info.CurrentByteOffset.QuadPart + distance.QuadPart;
        break;
    case FILE_END:
        {
            FILE_END_OF_FILE_INFORMATION eof;
            if (NtQueryInformationFile( hFile, &io, &eof, sizeof(eof), FileEndOfFileInformation ))
                goto error;
            pos = eof.EndOfFile.QuadPart + distance.QuadPart;
        }
        break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (pos < 0)
    {
        SetLastError( ERROR_NEGATIVE_SEEK );
        return FALSE;
    }

    info.CurrentByteOffset.QuadPart = pos;
    if (NtSetInformationFile( hFile, &io, &info, sizeof(info), FilePositionInformation ))
        goto error;
    if (newpos) newpos->QuadPart = pos;
    return TRUE;

error:
    SetLastError( RtlNtStatusToDosError(io.u.Status) );
    return FALSE;
}

/***********************************************************************
 *           GetFileTime   (KERNEL32.@)
 */
BOOL WINAPI GetFileTime( HANDLE hFile, FILETIME *lpCreationTime,
                         FILETIME *lpLastAccessTime, FILETIME *lpLastWriteTime )
{
    FILE_BASIC_INFORMATION info;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtQueryInformationFile( hFile, &io, &info, sizeof(info), FileBasicInformation );
    if (status == STATUS_SUCCESS)
    {
        if (lpCreationTime)
        {
            lpCreationTime->dwHighDateTime = info.CreationTime.u.HighPart;
            lpCreationTime->dwLowDateTime  = info.CreationTime.u.LowPart;
        }
        if (lpLastAccessTime)
        {
            lpLastAccessTime->dwHighDateTime = info.LastAccessTime.u.HighPart;
            lpLastAccessTime->dwLowDateTime  = info.LastAccessTime.u.LowPart;
        }
        if (lpLastWriteTime)
        {
            lpLastWriteTime->dwHighDateTime = info.LastWriteTime.u.HighPart;
            lpLastWriteTime->dwLowDateTime  = info.LastWriteTime.u.LowPart;
        }
        return TRUE;
    }
    SetLastError( RtlNtStatusToDosError(status) );
    return FALSE;
}


/***********************************************************************
 *              SetFileTime   (KERNEL32.@)
 */
BOOL WINAPI SetFileTime( HANDLE hFile, const FILETIME *ctime,
                         const FILETIME *atime, const FILETIME *mtime )
{
    FILE_BASIC_INFORMATION info;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    memset( &info, 0, sizeof(info) );
    if (ctime)
    {
        info.CreationTime.u.HighPart = ctime->dwHighDateTime;
        info.CreationTime.u.LowPart  = ctime->dwLowDateTime;
    }
    if (atime)
    {
        info.LastAccessTime.u.HighPart = atime->dwHighDateTime;
        info.LastAccessTime.u.LowPart  = atime->dwLowDateTime;
    }
    if (mtime)
    {
        info.LastWriteTime.u.HighPart = mtime->dwHighDateTime;
        info.LastWriteTime.u.LowPart  = mtime->dwLowDateTime;
    }

    status = NtSetInformationFile( hFile, &io, &info, sizeof(info), FileBasicInformation );
    if (status == STATUS_SUCCESS) return TRUE;
    SetLastError( RtlNtStatusToDosError(status) );
    return FALSE;
}


/**************************************************************************
 *           LockFile   (KERNEL32.@)
 */
BOOL WINAPI LockFile( HANDLE hFile, DWORD offset_low, DWORD offset_high,
                      DWORD count_low, DWORD count_high )
{
    NTSTATUS            status;
    LARGE_INTEGER       count, offset;

    TRACE( "%p %x%08x %x%08x\n",
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

    TRACE( "%p %x%08x %x%08x flags %x\n",
           hFile, overlapped->u.s.OffsetHigh, overlapped->u.s.Offset, 
           count_high, count_low, flags );

    count.u.LowPart = count_low;
    count.u.HighPart = count_high;
    offset.u.LowPart = overlapped->u.s.Offset;
    offset.u.HighPart = overlapped->u.s.OffsetHigh;

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

    return UnlockFile( hFile, overlapped->u.s.Offset, overlapped->u.s.OffsetHigh, count_low, count_high );
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


/*************************************************************************
 *           SetHandleCount   (KERNEL32.@)
 */
UINT WINAPI SetHandleCount( UINT count )
{
    return min( 256, count );
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


/*************************************************************************
 * CreateFileW [KERNEL32.@]  Creates or opens a file or other object
 *
 * Creates or opens an object, and returns a handle that can be used to
 * access that object.
 *
 * PARAMS
 *
 * filename     [in] pointer to filename to be accessed
 * access       [in] access mode requested
 * sharing      [in] share mode
 * sa           [in] pointer to security attributes
 * creation     [in] how to create the file
 * attributes   [in] attributes for newly created file
 * template     [in] handle to file with extended attributes to copy
 *
 * RETURNS
 *   Success: Open handle to specified file
 *   Failure: INVALID_HANDLE_VALUE
 */
HANDLE WINAPI CreateFileW( LPCWSTR filename, DWORD access, DWORD sharing,
                              LPSECURITY_ATTRIBUTES sa, DWORD creation,
                              DWORD attributes, HANDLE template )
{
    NTSTATUS status;
    UINT options;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    IO_STATUS_BLOCK io;
    HANDLE ret;
    DWORD dosdev;
    static const WCHAR bkslashes_with_dotW[] = {'\\','\\','.','\\',0};
    static const WCHAR coninW[] = {'C','O','N','I','N','$',0};
    static const WCHAR conoutW[] = {'C','O','N','O','U','T','$',0};
    SECURITY_QUALITY_OF_SERVICE qos;

    static const UINT nt_disposition[5] =
    {
        FILE_CREATE,        /* CREATE_NEW */
        FILE_OVERWRITE_IF,  /* CREATE_ALWAYS */
        FILE_OPEN,          /* OPEN_EXISTING */
        FILE_OPEN_IF,       /* OPEN_ALWAYS */
        FILE_OVERWRITE      /* TRUNCATE_EXISTING */
    };


    /* sanity checks */

    if (!filename || !filename[0])
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }

    TRACE("%s %s%s%s%s%s%s creation %d attributes 0x%x\n", debugstr_w(filename),
          (access & GENERIC_READ)?"GENERIC_READ ":"",
          (access & GENERIC_WRITE)?"GENERIC_WRITE ":"",
          (!access)?"QUERY_ACCESS ":"",
          (sharing & FILE_SHARE_READ)?"FILE_SHARE_READ ":"",
          (sharing & FILE_SHARE_WRITE)?"FILE_SHARE_WRITE ":"",
          (sharing & FILE_SHARE_DELETE)?"FILE_SHARE_DELETE ":"",
          creation, attributes);

    /* Open a console for CONIN$ or CONOUT$ */

    if (!strcmpiW(filename, coninW) || !strcmpiW(filename, conoutW))
    {
        ret = OpenConsoleW(filename, access, (sa && sa->bInheritHandle), creation);
        goto done;
    }

    if (!strncmpW(filename, bkslashes_with_dotW, 4))
    {
        static const WCHAR pipeW[] = {'P','I','P','E','\\',0};
        static const WCHAR mailslotW[] = {'M','A','I','L','S','L','O','T','\\',0};

        if ((isalphaW(filename[4]) && filename[5] == ':' && filename[6] == '\0') ||
            !strncmpiW( filename + 4, pipeW, 5 ) ||
            !strncmpiW( filename + 4, mailslotW, 9 ))
        {
            dosdev = 0;
        }
        else if ((dosdev = RtlIsDosDeviceName_U( filename + 4 )))
        {
            dosdev += MAKELONG( 0, 4*sizeof(WCHAR) );  /* adjust position to start of filename */
        }
        else if (!(GetVersion() & 0x80000000))
        {
            dosdev = 0;
        }
        else if (filename[4])
        {
            ret = VXD_Open( filename+4, access, sa );
            goto done;
        }
        else
        {
            SetLastError( ERROR_INVALID_NAME );
            return INVALID_HANDLE_VALUE;
        }
    }
    else dosdev = RtlIsDosDeviceName_U( filename );

    if (dosdev)
    {
        static const WCHAR conW[] = {'C','O','N'};

        if (LOWORD(dosdev) == sizeof(conW) &&
            !memicmpW( filename + HIWORD(dosdev)/sizeof(WCHAR), conW, sizeof(conW)/sizeof(WCHAR)))
        {
            switch (access & (GENERIC_READ|GENERIC_WRITE))
            {
            case GENERIC_READ:
                ret = OpenConsoleW(coninW, access, (sa && sa->bInheritHandle), creation);
                goto done;
            case GENERIC_WRITE:
                ret = OpenConsoleW(conoutW, access, (sa && sa->bInheritHandle), creation);
                goto done;
            default:
                SetLastError( ERROR_FILE_NOT_FOUND );
                return INVALID_HANDLE_VALUE;
            }
        }
    }

    if (creation < CREATE_NEW || creation > TRUNCATE_EXISTING)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }

    if (!RtlDosPathNameToNtPathName_U( filename, &nameW, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }

    /* now call NtCreateFile */

    options = 0;
    if (attributes & FILE_FLAG_BACKUP_SEMANTICS)
        options |= FILE_OPEN_FOR_BACKUP_INTENT;
    else
        options |= FILE_NON_DIRECTORY_FILE;
    if (attributes & FILE_FLAG_DELETE_ON_CLOSE)
    {
        options |= FILE_DELETE_ON_CLOSE;
        access |= DELETE;
    }
    if (!(attributes & FILE_FLAG_OVERLAPPED))
        options |= FILE_SYNCHRONOUS_IO_ALERT;
    if (attributes & FILE_FLAG_RANDOM_ACCESS)
        options |= FILE_RANDOM_ACCESS;
    attributes &= FILE_ATTRIBUTE_VALID_FLAGS;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nameW;
    attr.SecurityDescriptor = sa ? sa->lpSecurityDescriptor : NULL;
    if (attributes & SECURITY_SQOS_PRESENT)
    {
        qos.Length = sizeof(qos);
        qos.ImpersonationLevel = (attributes >> 16) & 0x3;
        qos.ContextTrackingMode = attributes & SECURITY_CONTEXT_TRACKING ? SECURITY_DYNAMIC_TRACKING : SECURITY_STATIC_TRACKING;
        qos.EffectiveOnly = attributes & SECURITY_EFFECTIVE_ONLY ? TRUE : FALSE;
        attr.SecurityQualityOfService = &qos;
    }
    else
        attr.SecurityQualityOfService = NULL;

    if (sa && sa->bInheritHandle) attr.Attributes |= OBJ_INHERIT;

    status = NtCreateFile( &ret, access, &attr, &io, NULL, attributes,
                           sharing, nt_disposition[creation - CREATE_NEW],
                           options, NULL, 0 );
    if (status)
    {
        WARN("Unable to create file %s (status %x)\n", debugstr_w(filename), status);
        ret = INVALID_HANDLE_VALUE;

        /* In the case file creation was rejected due to CREATE_NEW flag
         * was specified and file with that name already exists, correct
         * last error is ERROR_FILE_EXISTS and not ERROR_ALREADY_EXISTS.
         * Note: RtlNtStatusToDosError is not the subject to blame here.
         */
        if (status == STATUS_OBJECT_NAME_COLLISION)
            SetLastError( ERROR_FILE_EXISTS );
        else
            SetLastError( RtlNtStatusToDosError(status) );
    }
    else SetLastError(0);
    RtlFreeUnicodeString( &nameW );

 done:
    if (!ret) ret = INVALID_HANDLE_VALUE;
    TRACE("returning %p\n", ret);
    return ret;
}



/*************************************************************************
 *              CreateFileA              (KERNEL32.@)
 *
 * See CreateFileW.
 */
HANDLE WINAPI CreateFileA( LPCSTR filename, DWORD access, DWORD sharing,
                           LPSECURITY_ATTRIBUTES sa, DWORD creation,
                           DWORD attributes, HANDLE template)
{
    WCHAR *nameW;

    if (!(nameW = FILE_name_AtoW( filename, FALSE ))) return INVALID_HANDLE_VALUE;
    return CreateFileW( nameW, access, sharing, sa, creation, attributes, template );
}


/***********************************************************************
 *           DeleteFileW   (KERNEL32.@)
 *
 * Delete a file.
 *
 * PARAMS
 *  path [I] Path to the file to delete.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE, check GetLastError().
 */
BOOL WINAPI DeleteFileW( LPCWSTR path )
{
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    TRACE("%s\n", debugstr_w(path) );

    if (!RtlDosPathNameToNtPathName_U( path, &nameW, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nameW;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtDeleteFile(&attr);
    RtlFreeUnicodeString( &nameW );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           DeleteFileA   (KERNEL32.@)
 *
 * See DeleteFileW.
 */
BOOL WINAPI DeleteFileA( LPCSTR path )
{
    WCHAR *pathW;

    if (!(pathW = FILE_name_AtoW( path, FALSE ))) return FALSE;
    return DeleteFileW( pathW );
}


/**************************************************************************
 *           ReplaceFileW   (KERNEL32.@)
 *           ReplaceFile    (KERNEL32.@)
 */
BOOL WINAPI ReplaceFileW(LPCWSTR lpReplacedFileName,LPCWSTR lpReplacementFileName,
                         LPCWSTR lpBackupFileName, DWORD dwReplaceFlags,
                         LPVOID lpExclude, LPVOID lpReserved)
{
    FIXME("(%s,%s,%s,%08x,%p,%p) stub\n",debugstr_w(lpReplacedFileName),debugstr_w(lpReplacementFileName),
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
    FIXME("(%s,%s,%s,%08x,%p,%p) stub\n",lpReplacedFileName,lpReplacementFileName,
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
    static const WCHAR wildcardsW[] = { '*','?',0 };
    WCHAR *mask, *p;
    FIND_FIRST_INFO *info = NULL;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    DWORD device = 0;

    TRACE("%s %d %p %d %p %x\n", debugstr_w(filename), level, data, search_op, filter, flags);

    if ((search_op != FindExSearchNameMatch && search_op != FindExSearchLimitToDirectories)
	|| flags != 0)
    {
        FIXME("options not implemented 0x%08x 0x%08x\n", search_op, flags );
        return INVALID_HANDLE_VALUE;
    }
    if (level != FindExInfoStandard)
    {
        FIXME("info level %d not implemented\n", level );
        return INVALID_HANDLE_VALUE;
    }

    if (!RtlDosPathNameToNtPathName_U( filename, &nt_name, &mask, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }

    if (!(info = HeapAlloc( GetProcessHeap(), 0, sizeof(*info))))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto error;
    }

    if (!mask && (device = RtlIsDosDeviceName_U( filename )))
    {
        static const WCHAR dotW[] = {'.',0};
        WCHAR *dir = NULL;

        /* we still need to check that the directory can be opened */

        if (HIWORD(device))
        {
            if (!(dir = HeapAlloc( GetProcessHeap(), 0, HIWORD(device) + sizeof(WCHAR) )))
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                goto error;
            }
            memcpy( dir, filename, HIWORD(device) );
            dir[HIWORD(device)/sizeof(WCHAR)] = 0;
        }
        RtlFreeUnicodeString( &nt_name );
        if (!RtlDosPathNameToNtPathName_U( dir ? dir : dotW, &nt_name, &mask, NULL ))
        {
            HeapFree( GetProcessHeap(), 0, dir );
            SetLastError( ERROR_PATH_NOT_FOUND );
            goto error;
        }
        HeapFree( GetProcessHeap(), 0, dir );
        RtlInitUnicodeString( &info->mask, NULL );
    }
    else if (!mask || !*mask)
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        goto error;
    }
    else
    {
        if (!RtlCreateUnicodeString( &info->mask, mask ))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            goto error;
        }

        /* truncate dir name before mask */
        *mask = 0;
        nt_name.Length = (mask - nt_name.Buffer) * sizeof(WCHAR);
    }

    /* check if path is the root of the drive */
    info->is_root = FALSE;
    p = nt_name.Buffer + 4;  /* skip \??\ prefix */
    if (p[0] && p[1] == ':')
    {
        p += 2;
        while (*p == '\\') p++;
        info->is_root = (*p == 0);
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( &info->handle, GENERIC_READ, &attr, &io,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );

    if (status != STATUS_SUCCESS)
    {
        RtlFreeUnicodeString( &info->mask );
        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
            SetLastError( ERROR_PATH_NOT_FOUND );
        else
            SetLastError( RtlNtStatusToDosError(status) );
        goto error;
    }

    RtlInitializeCriticalSection( &info->cs );
    info->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": FIND_FIRST_INFO.cs");
    info->path     = nt_name;
    info->magic    = FIND_FIRST_MAGIC;
    info->data_pos = 0;
    info->data_len = 0;
    info->search_op = search_op;

    if (device)
    {
        WIN32_FIND_DATAW *wfd = data;

        memset( wfd, 0, sizeof(*wfd) );
        memcpy( wfd->cFileName, filename + HIWORD(device)/sizeof(WCHAR), LOWORD(device) );
        wfd->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
        CloseHandle( info->handle );
        info->handle = 0;
    }
    else
    {
        IO_STATUS_BLOCK io;

        NtQueryDirectoryFile( info->handle, 0, NULL, NULL, &io, info->data, sizeof(info->data),
                              FileBothDirectoryInformation, FALSE, &info->mask, TRUE );
        if (io.u.Status)
        {
            FindClose( info );
            SetLastError( RtlNtStatusToDosError( io.u.Status ) );
            return INVALID_HANDLE_VALUE;
        }
        info->data_len = io.Information;
        if (!FindNextFileW( info, data ))
        {
            TRACE( "%s not found\n", debugstr_w(filename) );
            FindClose( info );
            SetLastError( ERROR_FILE_NOT_FOUND );
            return INVALID_HANDLE_VALUE;
        }
        if (!strpbrkW( info->mask.Buffer, wildcardsW ))
        {
            /* we can't find two files with the same name */
            CloseHandle( info->handle );
            info->handle = 0;
        }
    }
    return info;

error:
    HeapFree( GetProcessHeap(), 0, info );
    RtlFreeUnicodeString( &nt_name );
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

    TRACE("%p %p\n", handle, data);

    if (!handle || handle == INVALID_HANDLE_VALUE)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return ret;
    }
    info = (FIND_FIRST_INFO *)handle;
    if (info->magic != FIND_FIRST_MAGIC)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return ret;
    }

    RtlEnterCriticalSection( &info->cs );

    if (!info->handle) SetLastError( ERROR_NO_MORE_FILES );
    else for (;;)
    {
        if (info->data_pos >= info->data_len)  /* need to read some more data */
        {
            IO_STATUS_BLOCK io;

            NtQueryDirectoryFile( info->handle, 0, NULL, NULL, &io, info->data, sizeof(info->data),
                                  FileBothDirectoryInformation, FALSE, &info->mask, FALSE );
            if (io.u.Status)
            {
                SetLastError( RtlNtStatusToDosError( io.u.Status ) );
                if (io.u.Status == STATUS_NO_MORE_FILES)
                {
                    CloseHandle( info->handle );
                    info->handle = 0;
                }
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

        /* check for dir symlink */
        if ((dir_info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            (dir_info->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
        {
            if (!check_dir_symlink( info, dir_info )) continue;
        }
	if (info->search_op == FindExSearchLimitToDirectories &&
	    (dir_info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
	    continue;

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

    if (!handle || handle == INVALID_HANDLE_VALUE)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    __TRY
    {
        if (info->magic == FIND_FIRST_MAGIC)
        {
            RtlEnterCriticalSection( &info->cs );
            if (info->magic == FIND_FIRST_MAGIC)  /* in case someone else freed it in the meantime */
            {
                info->magic = 0;
                if (info->handle) CloseHandle( info->handle );
                info->handle = 0;
                RtlFreeUnicodeString( &info->mask );
                info->mask.Buffer = NULL;
                RtlFreeUnicodeString( &info->path );
                info->data_pos = 0;
                info->data_len = 0;
                RtlLeaveCriticalSection( &info->cs );
                info->cs.DebugInfo->Spare[0] = 0;
                RtlDeleteCriticalSection( &info->cs );
                HeapFree( GetProcessHeap(), 0, info );
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        WARN("Illegal handle %p\n", handle);
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    __ENDTRY

    return TRUE;
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
    WCHAR *nameW;

    if (!(nameW = FILE_name_AtoW( lpFileName, FALSE ))) return INVALID_HANDLE_VALUE;

    handle = FindFirstFileExW(nameW, fInfoLevelId, &dataW, fSearchOp, lpSearchFilter, dwAdditionalFlags);
    if (handle == INVALID_HANDLE_VALUE) return handle;

    dataA = (WIN32_FIND_DATAA *) lpFindFileData;
    dataA->dwFileAttributes = dataW.dwFileAttributes;
    dataA->ftCreationTime   = dataW.ftCreationTime;
    dataA->ftLastAccessTime = dataW.ftLastAccessTime;
    dataA->ftLastWriteTime  = dataW.ftLastWriteTime;
    dataA->nFileSizeHigh    = dataW.nFileSizeHigh;
    dataA->nFileSizeLow     = dataW.nFileSizeLow;
    FILE_name_WtoA( dataW.cFileName, -1, dataA->cFileName, sizeof(dataA->cFileName) );
    FILE_name_WtoA( dataW.cAlternateFileName, -1, dataA->cAlternateFileName,
                    sizeof(dataA->cAlternateFileName) );
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
    FILE_name_WtoA( dataW.cFileName, -1, data->cFileName, sizeof(data->cFileName) );
    FILE_name_WtoA( dataW.cAlternateFileName, -1, data->cAlternateFileName,
                    sizeof(data->cAlternateFileName) );
    return TRUE;
}


/**************************************************************************
 *           GetFileAttributesW   (KERNEL32.@)
 */
DWORD WINAPI GetFileAttributesW( LPCWSTR name )
{
    FILE_BASIC_INFORMATION info;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    TRACE("%s\n", debugstr_w(name));

    if (!RtlDosPathNameToNtPathName_U( name, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_FILE_ATTRIBUTES;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtQueryAttributesFile( &attr, &info );
    RtlFreeUnicodeString( &nt_name );

    if (status == STATUS_SUCCESS) return info.FileAttributes;

    /* NtQueryAttributesFile fails on devices, but GetFileAttributesW succeeds */
    if (RtlIsDosDeviceName_U( name )) return FILE_ATTRIBUTE_ARCHIVE;

    SetLastError( RtlNtStatusToDosError(status) );
    return INVALID_FILE_ATTRIBUTES;
}


/**************************************************************************
 *           GetFileAttributesA   (KERNEL32.@)
 */
DWORD WINAPI GetFileAttributesA( LPCSTR name )
{
    WCHAR *nameW;

    if (!(nameW = FILE_name_AtoW( name, FALSE ))) return INVALID_FILE_ATTRIBUTES;
    return GetFileAttributesW( nameW );
}


/**************************************************************************
 *              SetFileAttributesW	(KERNEL32.@)
 */
BOOL WINAPI SetFileAttributesW( LPCWSTR name, DWORD attributes )
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;

    TRACE("%s %x\n", debugstr_w(name), attributes);

    if (!RtlDosPathNameToNtPathName_U( name, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( &handle, 0, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT );
    RtlFreeUnicodeString( &nt_name );

    if (status == STATUS_SUCCESS)
    {
        FILE_BASIC_INFORMATION info;

        memset( &info, 0, sizeof(info) );
        info.FileAttributes = attributes | FILE_ATTRIBUTE_NORMAL;  /* make sure it's not zero */
        status = NtSetInformationFile( handle, &io, &info, sizeof(info), FileBasicInformation );
        NtClose( handle );
    }

    if (status == STATUS_SUCCESS) return TRUE;
    SetLastError( RtlNtStatusToDosError(status) );
    return FALSE;
}


/**************************************************************************
 *              SetFileAttributesA	(KERNEL32.@)
 */
BOOL WINAPI SetFileAttributesA( LPCSTR name, DWORD attributes )
{
    WCHAR *nameW;

    if (!(nameW = FILE_name_AtoW( name, FALSE ))) return FALSE;
    return SetFileAttributesW( nameW, attributes );
}


/**************************************************************************
 *           GetFileAttributesExW   (KERNEL32.@)
 */
BOOL WINAPI GetFileAttributesExW( LPCWSTR name, GET_FILEEX_INFO_LEVELS level, LPVOID ptr )
{
    FILE_NETWORK_OPEN_INFORMATION info;
    WIN32_FILE_ATTRIBUTE_DATA *data = ptr;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    
    TRACE("%s %d %p\n", debugstr_w(name), level, ptr);

    if (level != GetFileExInfoStandard)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!RtlDosPathNameToNtPathName_U( name, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtQueryFullAttributesFile( &attr, &info );
    RtlFreeUnicodeString( &nt_name );

    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    data->dwFileAttributes = info.FileAttributes;
    data->ftCreationTime.dwLowDateTime    = info.CreationTime.u.LowPart;
    data->ftCreationTime.dwHighDateTime   = info.CreationTime.u.HighPart;
    data->ftLastAccessTime.dwLowDateTime  = info.LastAccessTime.u.LowPart;
    data->ftLastAccessTime.dwHighDateTime = info.LastAccessTime.u.HighPart;
    data->ftLastWriteTime.dwLowDateTime   = info.LastWriteTime.u.LowPart;
    data->ftLastWriteTime.dwHighDateTime  = info.LastWriteTime.u.HighPart;
    data->nFileSizeLow                    = info.EndOfFile.u.LowPart;
    data->nFileSizeHigh                   = info.EndOfFile.u.HighPart;
    return TRUE;
}


/**************************************************************************
 *           GetFileAttributesExA   (KERNEL32.@)
 */
BOOL WINAPI GetFileAttributesExA( LPCSTR name, GET_FILEEX_INFO_LEVELS level, LPVOID ptr )
{
    WCHAR *nameW;

    if (!(nameW = FILE_name_AtoW( name, FALSE ))) return FALSE;
    return GetFileAttributesExW( nameW, level, ptr );
}


/******************************************************************************
 *           GetCompressedFileSizeW   (KERNEL32.@)
 *
 * Get the actual number of bytes used on disk.
 *
 * RETURNS
 *    Success: Low-order doubleword of number of bytes
 *    Failure: INVALID_FILE_SIZE
 */
DWORD WINAPI GetCompressedFileSizeW(
    LPCWSTR name,       /* [in]  Pointer to name of file */
    LPDWORD size_high ) /* [out] Receives high-order doubleword of size */
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;
    DWORD ret = INVALID_FILE_SIZE;

    TRACE("%s %p\n", debugstr_w(name), size_high);

    if (!RtlDosPathNameToNtPathName_U( name, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_FILE_SIZE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( &handle, 0, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT );
    RtlFreeUnicodeString( &nt_name );

    if (status == STATUS_SUCCESS)
    {
        /* we don't support compressed files, simply return the file size */
        ret = GetFileSize( handle, size_high );
        NtClose( handle );
    }
    else SetLastError( RtlNtStatusToDosError(status) );

    return ret;
}


/******************************************************************************
 *           GetCompressedFileSizeA   (KERNEL32.@)
 *
 * See GetCompressedFileSizeW.
 */
DWORD WINAPI GetCompressedFileSizeA( LPCSTR name, LPDWORD size_high )
{
    WCHAR *nameW;

    if (!(nameW = FILE_name_AtoW( name, FALSE ))) return INVALID_FILE_SIZE;
    return GetCompressedFileSizeW( nameW, size_high );
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
        if ((handle = create_file_OF( name, mode )) == INVALID_HANDLE_VALUE)
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
