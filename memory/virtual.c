/*
 * Win32 virtual memory functions
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
#include "wine/port.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winnls.h"
#include "winbase.h"
#include "winternl.h"
#include "winerror.h"
#include "wine/exception.h"
#include "excpt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(virtual);

static unsigned int page_size;

/* filter for page-fault exceptions */
static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}


/***********************************************************************
 *             VirtualAlloc   (KERNEL32.@)
 * Reserves or commits a region of pages in virtual address space
 *
 * RETURNS
 *	Base address of allocated region of pages
 *	NULL: Failure
 */
LPVOID WINAPI VirtualAlloc(
              LPVOID addr,  /* [in] Address of region to reserve or commit */
              SIZE_T size,  /* [in] Size of region */
              DWORD type,   /* [in] Type of allocation */
              DWORD protect)/* [in] Type of access protection */
{
    return VirtualAllocEx( GetCurrentProcess(), addr, size, type, protect );
}


/***********************************************************************
 *             VirtualAllocEx   (KERNEL32.@)
 *
 * Seems to be just as VirtualAlloc, but with process handle.
 */
LPVOID WINAPI VirtualAllocEx(
              HANDLE hProcess, /* [in] Handle of process to do mem operation */
              LPVOID addr,     /* [in] Address of region to reserve or commit */
              SIZE_T size,     /* [in] Size of region */
              DWORD type,      /* [in] Type of allocation */
              DWORD protect )  /* [in] Type of access protection */
{
    LPVOID ret;
    NTSTATUS status;

    if ((status = NtAllocateVirtualMemory( hProcess, &ret, addr, &size, type, protect )))
    {
        SetLastError( RtlNtStatusToDosError(status) );
        ret = NULL;
    }
    return ret;
}


/***********************************************************************
 *             VirtualFree   (KERNEL32.@)
 * Release or decommits a region of pages in virtual address space.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI VirtualFree(
              LPVOID addr, /* [in] Address of region of committed pages */
              SIZE_T size, /* [in] Size of region */
              DWORD type   /* [in] Type of operation */
) {
    return VirtualFreeEx( GetCurrentProcess(), addr, size, type );
}


/***********************************************************************
 *             VirtualFreeEx   (KERNEL32.@)
 * Release or decommits a region of pages in virtual address space.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI VirtualFreeEx( HANDLE process, LPVOID addr, SIZE_T size, DWORD type )
{
    NTSTATUS status = NtFreeVirtualMemory( process, &addr, &size, type );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *             VirtualLock   (KERNEL32.@)
 * Locks the specified region of virtual address space
 *
 * NOTE
 *	Always returns TRUE
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI VirtualLock( LPVOID addr, /* [in] Address of first byte of range to lock */
                         SIZE_T size ) /* [in] Number of bytes in range to lock */
{
    NTSTATUS status = NtLockVirtualMemory( GetCurrentProcess(), &addr, &size, 1 );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *             VirtualUnlock   (KERNEL32.@)
 * Unlocks a range of pages in the virtual address space
 *
 * NOTE
 *	Always returns TRUE
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI VirtualUnlock( LPVOID addr, /* [in] Address of first byte of range */
                           SIZE_T size ) /* [in] Number of bytes in range */
{
    NTSTATUS status = NtUnlockVirtualMemory( GetCurrentProcess(), &addr, &size, 1 );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *             VirtualProtect   (KERNEL32.@)
 * Changes the access protection on a region of committed pages
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI VirtualProtect(
              LPVOID addr,     /* [in] Address of region of committed pages */
              SIZE_T size,     /* [in] Size of region */
              DWORD new_prot,  /* [in] Desired access protection */
              LPDWORD old_prot /* [out] Address of variable to get old protection */
) {
    return VirtualProtectEx( GetCurrentProcess(), addr, size, new_prot, old_prot );
}


/***********************************************************************
 *             VirtualProtectEx   (KERNEL32.@)
 * Changes the access protection on a region of committed pages in the
 * virtual address space of a specified process
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI VirtualProtectEx(
              HANDLE process,  /* [in]  Handle of process */
              LPVOID addr,     /* [in]  Address of region of committed pages */
              SIZE_T size,     /* [in]  Size of region */
              DWORD new_prot,  /* [in]  Desired access protection */
              LPDWORD old_prot /* [out] Address of variable to get old protection */ )
{
    NTSTATUS status = NtProtectVirtualMemory( process, &addr, &size, new_prot, old_prot );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *             VirtualQuery   (KERNEL32.@)
 * Provides info about a range of pages in virtual address space
 *
 * RETURNS
 *	Number of bytes returned in information buffer
 *	or 0 if addr is >= 0xc0000000 (kernel space).
 */
SIZE_T WINAPI VirtualQuery(
             LPCVOID addr,                    /* [in]  Address of region */
             PMEMORY_BASIC_INFORMATION info,  /* [out] Address of info buffer */
             SIZE_T len                       /* [in]  Size of buffer */
) {
    return VirtualQueryEx( GetCurrentProcess(), addr, info, len );
}


/***********************************************************************
 *             VirtualQueryEx   (KERNEL32.@)
 * Provides info about a range of pages in virtual address space of a
 * specified process
 *
 * RETURNS
 *	Number of bytes returned in information buffer
 */
SIZE_T WINAPI VirtualQueryEx(
             HANDLE process,                  /* [in] Handle of process */
             LPCVOID addr,                    /* [in] Address of region */
             PMEMORY_BASIC_INFORMATION info,  /* [out] Address of info buffer */
             SIZE_T len                       /* [in] Size of buffer */ )
{
    DWORD ret;
    NTSTATUS status;

    if ((status = NtQueryVirtualMemory( process, addr, MemoryBasicInformation, info, len, &ret )))
    {
        SetLastError( RtlNtStatusToDosError(status) );
        ret = 0;
    }
    return ret;
}


/***********************************************************************
 *             CreateFileMappingA   (KERNEL32.@)
 * Creates a named or unnamed file-mapping object for the specified file
 *
 * RETURNS
 *	Handle: Success
 *	0: Mapping object does not exist
 *	NULL: Failure
 */
HANDLE WINAPI CreateFileMappingA(
                HANDLE hFile,   /* [in] Handle of file to map */
                SECURITY_ATTRIBUTES *sa, /* [in] Optional security attributes*/
                DWORD protect,   /* [in] Protection for mapping object */
                DWORD size_high, /* [in] High-order 32 bits of object size */
                DWORD size_low,  /* [in] Low-order 32 bits of object size */
                LPCSTR name      /* [in] Name of file-mapping object */ )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateFileMappingW( hFile, sa, protect, size_high, size_low, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateFileMappingW( hFile, sa, protect, size_high, size_low, buffer );
}


/***********************************************************************
 *             CreateFileMappingW   (KERNEL32.@)
 * See CreateFileMappingA
 */
HANDLE WINAPI CreateFileMappingW( HANDLE hFile, LPSECURITY_ATTRIBUTES sa,
                                  DWORD protect, DWORD size_high,
                                  DWORD size_low, LPCWSTR name )
{
    static const int sec_flags = SEC_FILE | SEC_IMAGE | SEC_RESERVE | SEC_COMMIT | SEC_NOCACHE;

    HANDLE ret;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    NTSTATUS status;
    DWORD access, sec_type;
    LARGE_INTEGER size;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = (sa && sa->bInheritHandle) ? OBJ_INHERIT : 0;
    attr.SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;

    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
    }

    sec_type = protect & sec_flags;
    protect &= ~sec_flags;
    if (!sec_type) sec_type = SEC_COMMIT;

    switch(protect)
    {
    case 0:
    case PAGE_READONLY:
    case PAGE_WRITECOPY:
        access = STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ;
        break;
    case PAGE_READWRITE:
        access = STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE;
        break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (hFile == INVALID_HANDLE_VALUE)
    {
        hFile = 0;
        if (!size_low && !size_high)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
    }

    size.s.LowPart  = size_low;
    size.s.HighPart = size_high;

    status = NtCreateSection( &ret, access, &attr, &size, protect, sec_type, hFile );
    SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/***********************************************************************
 *             OpenFileMappingA   (KERNEL32.@)
 * Opens a named file-mapping object.
 *
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 */
HANDLE WINAPI OpenFileMappingA(
                DWORD access,   /* [in] Access mode */
                BOOL inherit, /* [in] Inherit flag */
                LPCSTR name )   /* [in] Name of file-mapping object */
{
    WCHAR buffer[MAX_PATH];

    if (!name) return OpenFileMappingW( access, inherit, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return OpenFileMappingW( access, inherit, buffer );
}


/***********************************************************************
 *             OpenFileMappingW   (KERNEL32.@)
 * See OpenFileMappingA
 */
HANDLE WINAPI OpenFileMappingW( DWORD access, BOOL inherit, LPCWSTR name)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE ret;
    NTSTATUS status;

    if (!name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = inherit ? OBJ_INHERIT : 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, name );

    if (access == FILE_MAP_COPY) access = FILE_MAP_READ;

    if ((status = NtOpenSection( &ret, access, &attr )))
    {
        SetLastError( RtlNtStatusToDosError(status) );
        ret = 0;
    }
    return ret;
}


/***********************************************************************
 *             MapViewOfFile   (KERNEL32.@)
 * Maps a view of a file into the address space
 *
 * RETURNS
 *	Starting address of mapped view
 *	NULL: Failure
 */
LPVOID WINAPI MapViewOfFile(
              HANDLE mapping,  /* [in] File-mapping object to map */
              DWORD access,      /* [in] Access mode */
              DWORD offset_high, /* [in] High-order 32 bits of file offset */
              DWORD offset_low,  /* [in] Low-order 32 bits of file offset */
              SIZE_T count       /* [in] Number of bytes to map */
) {
    return MapViewOfFileEx( mapping, access, offset_high,
                            offset_low, count, NULL );
}


/***********************************************************************
 *             MapViewOfFileEx   (KERNEL32.@)
 * Maps a view of a file into the address space
 *
 * RETURNS
 *	Starting address of mapped view
 *	NULL: Failure
 */
LPVOID WINAPI MapViewOfFileEx(
              HANDLE handle,   /* [in] File-mapping object to map */
              DWORD access,      /* [in] Access mode */
              DWORD offset_high, /* [in] High-order 32 bits of file offset */
              DWORD offset_low,  /* [in] Low-order 32 bits of file offset */
              SIZE_T count,      /* [in] Number of bytes to map */
              LPVOID addr        /* [in] Suggested starting address for mapped view */
) {
    NTSTATUS status;
    LARGE_INTEGER offset;
    ULONG protect;

    offset.s.LowPart  = offset_low;
    offset.s.HighPart = offset_high;

    if (access & FILE_MAP_WRITE) protect = PAGE_READWRITE;
    else if (access & FILE_MAP_READ) protect = PAGE_READONLY;
    else if (access & FILE_MAP_COPY) protect = PAGE_WRITECOPY;
    else protect = PAGE_NOACCESS;

    if ((status = NtMapViewOfSection( handle, GetCurrentProcess(), &addr, 0, 0, &offset,
                                      &count, ViewShare, 0, protect )))
    {
        SetLastError( RtlNtStatusToDosError(status) );
        addr = NULL;
    }
    return addr;
}


/***********************************************************************
 *             UnmapViewOfFile   (KERNEL32.@)
 * Unmaps a mapped view of a file.
 *
 * NOTES
 *	Should addr be an LPCVOID?
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI UnmapViewOfFile( LPVOID addr ) /* [in] Address where mapped view begins */
{
    NTSTATUS status = NtUnmapViewOfSection( GetCurrentProcess(), addr );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *             FlushViewOfFile   (KERNEL32.@)
 * Writes to the disk a byte range within a mapped view of a file
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI FlushViewOfFile( LPCVOID base,  /* [in] Start address of byte range to flush */
                             SIZE_T size )   /* [in] Number of bytes in range */
{
    NTSTATUS status = NtFlushVirtualMemory( GetCurrentProcess(), &base, &size, 0 );
    if (status)
    {
        if (status == STATUS_NOT_MAPPED_DATA) status = STATUS_SUCCESS;
        else SetLastError( RtlNtStatusToDosError(status) );
    }
    return !status;
}


/***********************************************************************
 *             IsBadReadPtr   (KERNEL32.@)
 *
 * RETURNS
 *	FALSE: Process has read access to entire block
 *      TRUE: Otherwise
 */
BOOL WINAPI IsBadReadPtr(
              LPCVOID ptr, /* [in] Address of memory block */
              UINT size )  /* [in] Size of block */
{
    if (!size) return FALSE;  /* handle 0 size case w/o reference */
    if (!page_size) page_size = getpagesize();
    __TRY
    {
        volatile const char *p = ptr;
        char dummy;
        UINT count = size;

        while (count > page_size)
        {
            dummy = *p;
            p += page_size;
            count -= page_size;
        }
        dummy = p[0];
        dummy = p[count - 1];
    }
    __EXCEPT(page_fault) { return TRUE; }
    __ENDTRY
    return FALSE;
}


/***********************************************************************
 *             IsBadWritePtr   (KERNEL32.@)
 *
 * RETURNS
 *	FALSE: Process has write access to entire block
 *      TRUE: Otherwise
 */
BOOL WINAPI IsBadWritePtr(
              LPVOID ptr, /* [in] Address of memory block */
              UINT size ) /* [in] Size of block in bytes */
{
    if (!size) return FALSE;  /* handle 0 size case w/o reference */
    if (!page_size) page_size = getpagesize();
    __TRY
    {
        volatile char *p = ptr;
        UINT count = size;

        while (count > page_size)
        {
            *p |= 0;
            p += page_size;
            count -= page_size;
        }
        p[0] |= 0;
        p[count - 1] |= 0;
    }
    __EXCEPT(page_fault) { return TRUE; }
    __ENDTRY
    return FALSE;
}


/***********************************************************************
 *             IsBadHugeReadPtr   (KERNEL32.@)
 * RETURNS
 *	FALSE: Process has read access to entire block
 *      TRUE: Otherwise
 */
BOOL WINAPI IsBadHugeReadPtr(
              LPCVOID ptr, /* [in] Address of memory block */
              UINT size  /* [in] Size of block */
) {
    return IsBadReadPtr( ptr, size );
}


/***********************************************************************
 *             IsBadHugeWritePtr   (KERNEL32.@)
 * RETURNS
 *	FALSE: Process has write access to entire block
 *      TRUE: Otherwise
 */
BOOL WINAPI IsBadHugeWritePtr(
              LPVOID ptr, /* [in] Address of memory block */
              UINT size /* [in] Size of block */
) {
    return IsBadWritePtr( ptr, size );
}


/***********************************************************************
 *             IsBadCodePtr   (KERNEL32.@)
 *
 * RETURNS
 *	FALSE: Process has read access to specified memory
 *	TRUE: Otherwise
 */
BOOL WINAPI IsBadCodePtr( FARPROC ptr ) /* [in] Address of function */
{
    return IsBadReadPtr( ptr, 1 );
}


/***********************************************************************
 *             IsBadStringPtrA   (KERNEL32.@)
 *
 * RETURNS
 *	FALSE: Read access to all bytes in string
 *	TRUE: Else
 */
BOOL WINAPI IsBadStringPtrA(
              LPCSTR str, /* [in] Address of string */
              UINT max )  /* [in] Maximum size of string */
{
    __TRY
    {
        volatile const char *p = str;
        while (p != str + max) if (!*p++) break;
    }
    __EXCEPT(page_fault) { return TRUE; }
    __ENDTRY
    return FALSE;
}


/***********************************************************************
 *             IsBadStringPtrW   (KERNEL32.@)
 * See IsBadStringPtrA
 */
BOOL WINAPI IsBadStringPtrW( LPCWSTR str, UINT max )
{
    __TRY
    {
        volatile const WCHAR *p = str;
        while (p != str + max) if (!*p++) break;
    }
    __EXCEPT(page_fault) { return TRUE; }
    __ENDTRY
    return FALSE;
}
