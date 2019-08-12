/*
 * File handling functions
 *
 * Copyright 1993 John Burton
 * Copyright 1996, 2004 Alexandre Julliard
 * Copyright 2008 Jeff Zaroyko
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
#include <stdio.h>

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
#include "fileapi.h"
#include "ddk/ntddk.h"

#include "kernelbase.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

static BOOL oem_file_apis;


static void WINAPI read_write_apc( void *apc_user, PIO_STATUS_BLOCK io, ULONG reserved )
{
    LPOVERLAPPED_COMPLETION_ROUTINE func = apc_user;
    func( RtlNtStatusToDosError( io->u.Status ), io->Information, (LPOVERLAPPED)io );
}


/***********************************************************************
 * Operations on file names
 ***********************************************************************/


/***********************************************************************
 *           file_name_AtoW
 *
 * Convert a file name to Unicode, taking into account the OEM/Ansi API mode.
 *
 * If alloc is FALSE uses the TEB static buffer, so it can only be used when
 * there is no possibility for the function to do that twice, taking into
 * account any called function.
 */
WCHAR *file_name_AtoW( LPCSTR name, BOOL alloc )
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
 *           file_name_WtoA
 *
 * Convert a file name back to OEM/Ansi. Returns number of bytes copied.
 */
DWORD file_name_WtoA( LPCWSTR src, INT srclen, LPSTR dest, INT destlen )
{
    DWORD ret;

    if (srclen < 0) srclen = lstrlenW( src ) + 1;
    if (oem_file_apis)
        RtlUnicodeToOemN( dest, destlen, &ret, src, srclen * sizeof(WCHAR) );
    else
        RtlUnicodeToMultiByteN( dest, destlen, &ret, src, srclen * sizeof(WCHAR) );
    return ret;
}


/******************************************************************************
 *	AreFileApisANSI   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH AreFileApisANSI(void)
{
    return !oem_file_apis;
}


/**************************************************************************
 *	SetFileApisToANSI   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH SetFileApisToANSI(void)
{
    oem_file_apis = FALSE;
}


/**************************************************************************
 *	SetFileApisToOEM   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH SetFileApisToOEM(void)
{
    oem_file_apis = TRUE;
}


/***********************************************************************
 * Operations on file handles
 ***********************************************************************/


/***********************************************************************
 *	CancelIo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CancelIo( HANDLE handle )
{
    IO_STATUS_BLOCK io;

    NtCancelIoFile( handle, &io );
    return set_ntstatus( io.u.Status );
}


/***********************************************************************
 *	CancelIoEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CancelIoEx( HANDLE handle, LPOVERLAPPED overlapped )
{
    IO_STATUS_BLOCK io;

    NtCancelIoFileEx( handle, (PIO_STATUS_BLOCK)overlapped, &io );
    return set_ntstatus( io.u.Status );
}


/***********************************************************************
 *	CancelSynchronousIo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CancelSynchronousIo( HANDLE thread )
{
    FIXME( "(%p): stub\n", thread );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *	FlushFileBuffers   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FlushFileBuffers( HANDLE file )
{
    IO_STATUS_BLOCK iosb;

    /* this will fail (as expected) for an output handle */
    if (is_console_handle( file )) return FlushConsoleInputBuffer( file );

    return set_ntstatus( NtFlushBuffersFile( file, &iosb ));
}


/***********************************************************************
 *	GetFileInformationByHandle   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileInformationByHandle( HANDLE file, BY_HANDLE_FILE_INFORMATION *info )
{
    FILE_ALL_INFORMATION all_info;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtQueryInformationFile( file, &io, &all_info, sizeof(all_info), FileAllInformation );
    if (status == STATUS_BUFFER_OVERFLOW) status = STATUS_SUCCESS;
    if (!set_ntstatus( status )) return FALSE;

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


/***********************************************************************
 *	GetFileInformationByHandleEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileInformationByHandleEx( HANDLE handle, FILE_INFO_BY_HANDLE_CLASS class,
                                                            LPVOID info, DWORD size )
{
    NTSTATUS status;
    IO_STATUS_BLOCK io;

    switch (class)
    {
    case FileStreamInfo:
    case FileCompressionInfo:
    case FileAttributeTagInfo:
    case FileRemoteProtocolInfo:
    case FileFullDirectoryInfo:
    case FileFullDirectoryRestartInfo:
    case FileStorageInfo:
    case FileAlignmentInfo:
    case FileIdExtdDirectoryInfo:
    case FileIdExtdDirectoryRestartInfo:
        FIXME( "%p, %u, %p, %u\n", handle, class, info, size );
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return FALSE;

    case FileBasicInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileBasicInformation );
        break;

    case FileStandardInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileStandardInformation );
        break;

    case FileNameInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileNameInformation );
        break;

    case FileIdInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileIdInformation );
        break;

    case FileIdBothDirectoryRestartInfo:
    case FileIdBothDirectoryInfo:
        status = NtQueryDirectoryFile( handle, NULL, NULL, NULL, &io, info, size,
                                       FileIdBothDirectoryInformation, FALSE, NULL,
                                       (class == FileIdBothDirectoryRestartInfo) );
        break;

    case FileRenameInfo:
    case FileDispositionInfo:
    case FileAllocationInfo:
    case FileIoPriorityHintInfo:
    case FileEndOfFileInfo:
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return set_ntstatus( status );
}


/***********************************************************************
 *	GetFileSize   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFileSize( HANDLE file, LPDWORD size_high )
{
    LARGE_INTEGER size;

    if (!GetFileSizeEx( file, &size )) return INVALID_FILE_SIZE;
    if (size_high) *size_high = size.u.HighPart;
    if (size.u.LowPart == INVALID_FILE_SIZE) SetLastError( 0 );
    return size.u.LowPart;
}


/***********************************************************************
 *	GetFileSizeEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileSizeEx( HANDLE file, PLARGE_INTEGER size )
{
    FILE_STANDARD_INFORMATION info;
    IO_STATUS_BLOCK io;

    if (is_console_handle( file ))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (!set_ntstatus( NtQueryInformationFile( file, &io, &info, sizeof(info), FileStandardInformation )))
        return FALSE;

    *size = info.EndOfFile;
    return TRUE;
}


/***********************************************************************
 *	GetFileTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileTime( HANDLE file, FILETIME *creation,
                                           FILETIME *access, FILETIME *write )
{
    FILE_BASIC_INFORMATION info;
    IO_STATUS_BLOCK io;

    if (!set_ntstatus( NtQueryInformationFile( file, &io, &info, sizeof(info), FileBasicInformation )))
        return FALSE;

    if (creation)
    {
        creation->dwHighDateTime = info.CreationTime.u.HighPart;
        creation->dwLowDateTime  = info.CreationTime.u.LowPart;
    }
    if (access)
    {
        access->dwHighDateTime = info.LastAccessTime.u.HighPart;
        access->dwLowDateTime  = info.LastAccessTime.u.LowPart;
    }
    if (write)
    {
        write->dwHighDateTime = info.LastWriteTime.u.HighPart;
        write->dwLowDateTime  = info.LastWriteTime.u.LowPart;
    }
    return TRUE;
}


/***********************************************************************
 *	GetFileType   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFileType( HANDLE file )
{
    FILE_FS_DEVICE_INFORMATION info;
    IO_STATUS_BLOCK io;

    if (file == (HANDLE)STD_INPUT_HANDLE ||
        file == (HANDLE)STD_OUTPUT_HANDLE ||
        file == (HANDLE)STD_ERROR_HANDLE)
        file = GetStdHandle( (DWORD_PTR)file );

    if (is_console_handle( file )) return FILE_TYPE_CHAR;

    if (!set_ntstatus( NtQueryVolumeInformationFile( file, &io, &info, sizeof(info),
                                                     FileFsDeviceInformation )))
        return FILE_TYPE_UNKNOWN;

    switch (info.DeviceType)
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
 *	GetOverlappedResult   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetOverlappedResult( HANDLE file, LPOVERLAPPED overlapped,
                                                   LPDWORD result, BOOL wait )
{
    NTSTATUS status;

    TRACE( "(%p %p %p %x)\n", file, overlapped, result, wait );

    status = overlapped->Internal;
    if (status == STATUS_PENDING)
    {
        if (!wait)
        {
            SetLastError( ERROR_IO_INCOMPLETE );
            return FALSE;
        }
        if (WaitForSingleObject( overlapped->hEvent ? overlapped->hEvent : file, INFINITE ) == WAIT_FAILED)
            return FALSE;

        status = overlapped->Internal;
        if (status == STATUS_PENDING) status = STATUS_SUCCESS;
    }

    *result = overlapped->InternalHigh;
    return set_ntstatus( status );
}


/**************************************************************************
 *	LockFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH LockFile( HANDLE file, DWORD offset_low, DWORD offset_high,
                                        DWORD count_low, DWORD count_high )
{
    LARGE_INTEGER count, offset;

    TRACE( "%p %x%08x %x%08x\n", file, offset_high, offset_low, count_high, count_low );

    count.u.LowPart = count_low;
    count.u.HighPart = count_high;
    offset.u.LowPart = offset_low;
    offset.u.HighPart = offset_high;
    return set_ntstatus( NtLockFile( file, 0, NULL, NULL, NULL, &offset, &count, NULL, TRUE, TRUE ));
}


/**************************************************************************
 *	LockFileEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH LockFileEx( HANDLE file, DWORD flags, DWORD reserved,
                                          DWORD count_low, DWORD count_high, LPOVERLAPPED overlapped )
{
    LARGE_INTEGER count, offset;
    LPVOID cvalue = NULL;

    if (reserved)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    TRACE( "%p %x%08x %x%08x flags %x\n",
           file, overlapped->u.s.OffsetHigh, overlapped->u.s.Offset, count_high, count_low, flags );

    count.u.LowPart = count_low;
    count.u.HighPart = count_high;
    offset.u.LowPart = overlapped->u.s.Offset;
    offset.u.HighPart = overlapped->u.s.OffsetHigh;

    if (((ULONG_PTR)overlapped->hEvent & 1) == 0) cvalue = overlapped;

    return set_ntstatus( NtLockFile( file, overlapped->hEvent, NULL, cvalue,
                                     NULL, &offset, &count, NULL,
                                     flags & LOCKFILE_FAIL_IMMEDIATELY,
                                     flags & LOCKFILE_EXCLUSIVE_LOCK ));
}


/***********************************************************************
 *	ReadFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadFile( HANDLE file, LPVOID buffer, DWORD count,
                                        LPDWORD result, LPOVERLAPPED overlapped )
{
    LARGE_INTEGER offset;
    PLARGE_INTEGER poffset = NULL;
    IO_STATUS_BLOCK iosb;
    PIO_STATUS_BLOCK io_status = &iosb;
    HANDLE event = 0;
    NTSTATUS status;
    LPVOID cvalue = NULL;

    TRACE( "%p %p %d %p %p\n", file, buffer, count, result, overlapped );

    if (result) *result = 0;

    if (is_console_handle( file ))
    {
        DWORD conread, mode;

        if (!ReadConsoleA( file, buffer, count, &conread, NULL) || !GetConsoleMode( file, &mode ))
            return FALSE;
        /* ctrl-Z (26) means end of file on window (if at beginning of buffer)
         * but Unix uses ctrl-D (4), and ctrl-Z is a bad idea on Unix :-/
         * So map both ctrl-D ctrl-Z to EOF.
         */
        if ((mode & ENABLE_PROCESSED_INPUT) && conread > 0 &&
            (((char *)buffer)[0] == 26 || ((char *)buffer)[0] == 4))
        {
            conread = 0;
        }
        if (result) *result = conread;
        return TRUE;
    }

    if (overlapped)
    {
        offset.u.LowPart = overlapped->u.s.Offset;
        offset.u.HighPart = overlapped->u.s.OffsetHigh;
        poffset = &offset;
        event = overlapped->hEvent;
        io_status = (PIO_STATUS_BLOCK)overlapped;
        if (((ULONG_PTR)event & 1) == 0) cvalue = overlapped;
    }
    else io_status->Information = 0;
    io_status->u.Status = STATUS_PENDING;

    status = NtReadFile( file, event, NULL, cvalue, io_status, buffer, count, poffset, NULL);

    if (status == STATUS_PENDING && !overlapped)
    {
        WaitForSingleObject( file, INFINITE );
        status = io_status->u.Status;
    }

    if (result) *result = overlapped && status ? 0 : io_status->Information;

    if (status == STATUS_END_OF_FILE)
    {
        if (overlapped != NULL)
        {
            SetLastError( RtlNtStatusToDosError(status) );
            return FALSE;
        }
    }
    else if (status && status != STATUS_TIMEOUT)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *	ReadFileEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadFileEx( HANDLE file, LPVOID buffer, DWORD count, LPOVERLAPPED overlapped,
                                          LPOVERLAPPED_COMPLETION_ROUTINE completion )
{
    PIO_STATUS_BLOCK io;
    LARGE_INTEGER offset;
    NTSTATUS status;

    TRACE( "(file=%p, buffer=%p, bytes=%u, ovl=%p, ovl_fn=%p)\n",
           file, buffer, count, overlapped, completion );

    if (!overlapped)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    offset.u.LowPart = overlapped->u.s.Offset;
    offset.u.HighPart = overlapped->u.s.OffsetHigh;
    io = (PIO_STATUS_BLOCK)overlapped;
    io->u.Status = STATUS_PENDING;
    io->Information = 0;

    status = NtReadFile( file, NULL, read_write_apc, completion, io, buffer, count, &offset, NULL);
    if (status == STATUS_PENDING) return TRUE;
    return set_ntstatus( status );
}


/***********************************************************************
 *	ReadFileScatter   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadFileScatter( HANDLE file, FILE_SEGMENT_ELEMENT *segments, DWORD count,
                                               LPDWORD reserved, LPOVERLAPPED overlapped )
{
    PIO_STATUS_BLOCK io;
    LARGE_INTEGER offset;
    void *cvalue = NULL;

    TRACE( "(%p %p %u %p)\n", file, segments, count, overlapped );

    offset.u.LowPart = overlapped->u.s.Offset;
    offset.u.HighPart = overlapped->u.s.OffsetHigh;
    if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
    io = (PIO_STATUS_BLOCK)overlapped;
    io->u.Status = STATUS_PENDING;
    io->Information = 0;

    return set_ntstatus( NtReadFileScatter( file, overlapped->hEvent, NULL, cvalue, io,
                                            segments, count, &offset, NULL ));
}


/**************************************************************************
 *	SetEndOfFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetEndOfFile( HANDLE file )
{
    FILE_POSITION_INFORMATION pos;
    FILE_END_OF_FILE_INFORMATION eof;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    if (!(status = NtQueryInformationFile( file, &io, &pos, sizeof(pos), FilePositionInformation )))
    {
        eof.EndOfFile = pos.CurrentByteOffset;
        status = NtSetInformationFile( file, &io, &eof, sizeof(eof), FileEndOfFileInformation );
    }
    return set_ntstatus( status );
}


/***********************************************************************
 *	SetFileInformationByHandle   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFileInformationByHandle( HANDLE file, FILE_INFO_BY_HANDLE_CLASS class,
                                                          void *info, DWORD size )
{
    NTSTATUS status;
    IO_STATUS_BLOCK io;

    TRACE( "%p %u %p %u\n", file, class, info, size );

    switch (class)
    {
    case FileNameInfo:
    case FileRenameInfo:
    case FileAllocationInfo:
    case FileEndOfFileInfo:
    case FileStreamInfo:
    case FileIdBothDirectoryInfo:
    case FileIdBothDirectoryRestartInfo:
    case FileFullDirectoryInfo:
    case FileFullDirectoryRestartInfo:
    case FileStorageInfo:
    case FileAlignmentInfo:
    case FileIdInfo:
    case FileIdExtdDirectoryInfo:
    case FileIdExtdDirectoryRestartInfo:
        FIXME( "%p, %u, %p, %u\n", file, class, info, size );
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return FALSE;

    case FileBasicInfo:
        status = NtSetInformationFile( file, &io, info, size, FileBasicInformation );
        break;
    case FileDispositionInfo:
        status = NtSetInformationFile( file, &io, info, size, FileDispositionInformation );
        break;
    case FileIoPriorityHintInfo:
        status = NtSetInformationFile( file, &io, info, size, FileIoPriorityHintInformation );
        break;
    case FileStandardInfo:
    case FileCompressionInfo:
    case FileAttributeTagInfo:
    case FileRemoteProtocolInfo:
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return set_ntstatus( status );
}


/***********************************************************************
 *	SetFilePointer   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH SetFilePointer( HANDLE file, LONG distance, LONG *highword, DWORD method )
{
    LARGE_INTEGER dist, newpos;

    if (highword)
    {
        dist.u.LowPart  = distance;
        dist.u.HighPart = *highword;
    }
    else dist.QuadPart = distance;

    if (!SetFilePointerEx( file, dist, &newpos, method )) return INVALID_SET_FILE_POINTER;

    if (highword) *highword = newpos.u.HighPart;
    if (newpos.u.LowPart == INVALID_SET_FILE_POINTER) SetLastError( 0 );
    return newpos.u.LowPart;
}


/***********************************************************************
 *	SetFilePointerEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFilePointerEx( HANDLE file, LARGE_INTEGER distance,
                                                LARGE_INTEGER *newpos, DWORD method )
{
    LONGLONG pos;
    IO_STATUS_BLOCK io;
    FILE_POSITION_INFORMATION info;
    FILE_END_OF_FILE_INFORMATION eof;

    switch(method)
    {
    case FILE_BEGIN:
        pos = distance.QuadPart;
        break;
    case FILE_CURRENT:
        if (NtQueryInformationFile( file, &io, &info, sizeof(info), FilePositionInformation ))
            goto error;
        pos = info.CurrentByteOffset.QuadPart + distance.QuadPart;
        break;
    case FILE_END:
        if (NtQueryInformationFile( file, &io, &eof, sizeof(eof), FileEndOfFileInformation ))
            goto error;
        pos = eof.EndOfFile.QuadPart + distance.QuadPart;
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
    if (!NtSetInformationFile( file, &io, &info, sizeof(info), FilePositionInformation ))
    {
        if (newpos) newpos->QuadPart = pos;
        return TRUE;
    }

error:
    return set_ntstatus( io.u.Status );
}


/***********************************************************************
 *	SetFileTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFileTime( HANDLE file, const FILETIME *ctime,
                                           const FILETIME *atime, const FILETIME *mtime )
{
    FILE_BASIC_INFORMATION info;
    IO_STATUS_BLOCK io;

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

    return set_ntstatus( NtSetInformationFile( file, &io, &info, sizeof(info), FileBasicInformation ));
}


/***********************************************************************
 *	SetFileValidData   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFileValidData( HANDLE file, LONGLONG length )
{
    FILE_VALID_DATA_LENGTH_INFORMATION info;
    IO_STATUS_BLOCK io;

    info.ValidDataLength.QuadPart = length;
    return set_ntstatus( NtSetInformationFile( file, &io, &info, sizeof(info),
                                               FileValidDataLengthInformation ));
}


/**************************************************************************
 *	UnlockFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH UnlockFile( HANDLE file, DWORD offset_low, DWORD offset_high,
                                          DWORD count_low, DWORD count_high )
{
    LARGE_INTEGER count, offset;

    count.u.LowPart = count_low;
    count.u.HighPart = count_high;
    offset.u.LowPart = offset_low;
    offset.u.HighPart = offset_high;
    return set_ntstatus( NtUnlockFile( file, NULL, &offset, &count, NULL ));
}


/**************************************************************************
 *	UnlockFileEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH UnlockFileEx( HANDLE file, DWORD reserved,
                                            DWORD count_low, DWORD count_high, LPOVERLAPPED overlapped )
{
    if (reserved)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (overlapped->hEvent) FIXME("Unimplemented overlapped operation\n");

    return UnlockFile( file, overlapped->u.s.Offset, overlapped->u.s.OffsetHigh, count_low, count_high );
}


/***********************************************************************
 *	WriteFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteFile( HANDLE file, LPCVOID buffer, DWORD count,
                                         LPDWORD result, LPOVERLAPPED overlapped )
{
    HANDLE event = NULL;
    LARGE_INTEGER offset;
    PLARGE_INTEGER poffset = NULL;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    PIO_STATUS_BLOCK piosb = &iosb;
    LPVOID cvalue = NULL;

    TRACE( "%p %p %d %p %p\n", file, buffer, count, result, overlapped );

    if (is_console_handle( file )) return WriteConsoleA( file, buffer, count, result, NULL);

    if (overlapped)
    {
        offset.u.LowPart = overlapped->u.s.Offset;
        offset.u.HighPart = overlapped->u.s.OffsetHigh;
        poffset = &offset;
        event = overlapped->hEvent;
        piosb = (PIO_STATUS_BLOCK)overlapped;
        if (((ULONG_PTR)event & 1) == 0) cvalue = overlapped;
    }
    else piosb->Information = 0;
    piosb->u.Status = STATUS_PENDING;

    status = NtWriteFile( file, event, NULL, cvalue, piosb, buffer, count, poffset, NULL );

    if (status == STATUS_PENDING && !overlapped)
    {
        WaitForSingleObject( file, INFINITE );
        status = piosb->u.Status;
    }

    if (result) *result = overlapped && status ? 0 : piosb->Information;

    if (status && status != STATUS_TIMEOUT)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *	WriteFileEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteFileEx( HANDLE file, LPCVOID buffer,
                                           DWORD count, LPOVERLAPPED overlapped,
                                           LPOVERLAPPED_COMPLETION_ROUTINE completion )
{
    LARGE_INTEGER offset;
    NTSTATUS status;
    PIO_STATUS_BLOCK io;

    TRACE( "%p %p %d %p %p\n", file, buffer, count, overlapped, completion );

    if (!overlapped)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    offset.u.LowPart = overlapped->u.s.Offset;
    offset.u.HighPart = overlapped->u.s.OffsetHigh;

    io = (PIO_STATUS_BLOCK)overlapped;
    io->u.Status = STATUS_PENDING;
    io->Information = 0;

    status = NtWriteFile( file, NULL, read_write_apc, completion, io, buffer, count, &offset, NULL );
    if (status == STATUS_PENDING) return TRUE;
    return set_ntstatus( status );
}


/***********************************************************************
 *	WriteFileGather   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteFileGather( HANDLE file, FILE_SEGMENT_ELEMENT *segments, DWORD count,
                                               LPDWORD reserved, LPOVERLAPPED overlapped )
{
    PIO_STATUS_BLOCK io;
    LARGE_INTEGER offset;
    void *cvalue = NULL;

    TRACE( "%p %p %u %p\n", file, segments, count, overlapped );

    offset.u.LowPart = overlapped->u.s.Offset;
    offset.u.HighPart = overlapped->u.s.OffsetHigh;
    if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
    io = (PIO_STATUS_BLOCK)overlapped;
    io->u.Status = STATUS_PENDING;
    io->Information = 0;

    return set_ntstatus( NtWriteFileGather( file, overlapped->hEvent, NULL, cvalue,
                                            io, segments, count, &offset, NULL ));
}
