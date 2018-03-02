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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "winerror.h"
#include "psapi.h"
#include "wine/exception.h"
#include "wine/debug.h"

#include "kernel_private.h"

WINE_DECLARE_DEBUG_CHANNEL(seh);
WINE_DECLARE_DEBUG_CHANNEL(file);


/***********************************************************************
 *             VirtualAlloc   (KERNEL32.@)
 *
 * Reserves or commits a region of pages in virtual address space.
 *
 * PARAMS
 *  addr    [I] Address of region to reserve or commit.
 *  size    [I] Size of region.
 *  type    [I] Type of allocation.
 *  protect [I] Type of access protection.
 *
 * RETURNS
 *	Success: Base address of allocated region of pages.
 *	Failure: NULL.
 */
LPVOID WINAPI DECLSPEC_HOTPATCH VirtualAlloc( void *addr, SIZE_T size, DWORD type, DWORD protect )
{
    return VirtualAllocEx( GetCurrentProcess(), addr, size, type, protect );
}


/***********************************************************************
 *             VirtualAllocEx   (KERNEL32.@)
 *
 * Seems to be just as VirtualAlloc, but with process handle.
 *
 * PARAMS
 *  hProcess [I] Handle to process to do mem operation.
 *  addr     [I] Address of region to reserve or commit.
 *  size     [I] Size of region.
 *  type     [I] Type of allocation.
 *  protect  [I] Type of access protection.
 *
 *
 * RETURNS
 *	Success: Base address of allocated region of pages.
 *	Failure: NULL.
 */
LPVOID WINAPI VirtualAllocEx( HANDLE hProcess, LPVOID addr, SIZE_T size,
    DWORD type, DWORD protect )
{
    LPVOID ret = addr;
    NTSTATUS status;

    if ((status = NtAllocateVirtualMemory( hProcess, &ret, 0, &size, type, protect )))
    {
        SetLastError( RtlNtStatusToDosError(status) );
        ret = NULL;
    }
    return ret;
}


/***********************************************************************
 *             VirtualFree   (KERNEL32.@)
 *
 * Releases or decommits a region of pages in virtual address space.
 *
 * PARAMS
 *  addr [I] Address of region of committed pages.
 *  size [I] Size of region.
 *  type [I] Type of operation.
 *
 * RETURNS
 *	Success: TRUE.
 *	Failure: FALSE.
 */
BOOL WINAPI VirtualFree( LPVOID addr, SIZE_T size, DWORD type )
{
    return VirtualFreeEx( GetCurrentProcess(), addr, size, type );
}


/***********************************************************************
 *             VirtualFreeEx   (KERNEL32.@)
 *
 * Releases or decommits a region of pages in virtual address space.
 *
 * PARAMS
 *  process [I] Handle to process.
 *  addr    [I] Address of region to free.
 *  size    [I] Size of region.
 *  type    [I] Type of operation.
 *
 * RETURNS
 *	Success: TRUE.
 *	Failure: FALSE.
 */
BOOL WINAPI VirtualFreeEx( HANDLE process, LPVOID addr, SIZE_T size, DWORD type )
{
    NTSTATUS status = NtFreeVirtualMemory( process, &addr, &size, type );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *             VirtualLock   (KERNEL32.@)
 *
 * Locks the specified region of virtual address space.
 *
 * PARAMS
 *  addr [I] Address of first byte of range to lock.
 *  size [I] Number of bytes in range to lock.
 *
 * RETURNS
 *	Success: TRUE.
 *	Failure: FALSE.
 *
 * NOTES
 *	Always returns TRUE.
 *
 */
BOOL WINAPI VirtualLock( LPVOID addr, SIZE_T size )
{
    NTSTATUS status = NtLockVirtualMemory( GetCurrentProcess(), &addr, &size, 1 );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *             VirtualUnlock   (KERNEL32.@)
 *
 * Unlocks a range of pages in the virtual address space.
 *
 * PARAMS
 *  addr [I] Address of first byte of range.
 *  size [I] Number of bytes in range.
 *
 * RETURNS
 *	Success: TRUE.
 *	Failure: FALSE.
 *
 * NOTES
 *	Always returns TRUE.
 *
 */
BOOL WINAPI VirtualUnlock( LPVOID addr, SIZE_T size )
{
    NTSTATUS status = NtUnlockVirtualMemory( GetCurrentProcess(), &addr, &size, 1 );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *             VirtualProtect   (KERNEL32.@)
 *
 * Changes the access protection on a region of committed pages.
 *
 * PARAMS
 *  addr     [I] Address of region of committed pages.
 *  size     [I] Size of region.
 *  new_prot [I] Desired access protection.
 *  old_prot [O] Address of variable to get old protection.
 *
 * RETURNS
 *	Success: TRUE.
 *	Failure: FALSE.
 */
BOOL WINAPI VirtualProtect( LPVOID addr, SIZE_T size, DWORD new_prot, LPDWORD old_prot)
{
    return VirtualProtectEx( GetCurrentProcess(), addr, size, new_prot, old_prot );
}


/***********************************************************************
 *             VirtualProtectEx   (KERNEL32.@)
 *
 * Changes the access protection on a region of committed pages in the
 * virtual address space of a specified process.
 *
 * PARAMS
 *  process  [I] Handle of process.
 *  addr     [I] Address of region of committed pages.
 *  size     [I] Size of region.
 *  new_prot [I] Desired access protection.
 *  old_prot [O] Address of variable to get old protection.
 *
 * RETURNS
 *	Success: TRUE.
 *	Failure: FALSE.
 */
BOOL WINAPI VirtualProtectEx( HANDLE process, LPVOID addr, SIZE_T size,
    DWORD new_prot, LPDWORD old_prot )
{
    NTSTATUS status;
    DWORD prot;

    /* Win9x allows passing NULL as old_prot while this fails on NT */
    if (!old_prot && (GetVersion() & 0x80000000)) old_prot = &prot;

    status = NtProtectVirtualMemory( process, &addr, &size, new_prot, old_prot );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *             VirtualQuery   (KERNEL32.@)
 *
 * Provides info about a range of pages in virtual address space.
 *
 * PARAMS
 *  addr [I] Address of region.
 *  info [O] Address of info buffer.
 *  len  [I] Size of buffer.
 *
 * RETURNS
 *	Number of bytes returned in information buffer or 0 if
 *	addr >= 0xc0000000 (kernel space).
 */
SIZE_T WINAPI VirtualQuery( LPCVOID addr, PMEMORY_BASIC_INFORMATION info,
    SIZE_T len )
{
    return VirtualQueryEx( GetCurrentProcess(), addr, info, len );
}


/***********************************************************************
 *             VirtualQueryEx   (KERNEL32.@)
 *
 * Provides info about a range of pages in virtual address space of a
 * specified process.
 *
 * PARAMS
 *  process [I] Handle to process.
 *  addr    [I] Address of region.
 *  info    [O] Address of info buffer.
 *  len     [I] Size of buffer.
 *
 * RETURNS
 *	Number of bytes returned in information buffer.
 */
SIZE_T WINAPI VirtualQueryEx( HANDLE process, LPCVOID addr,
    PMEMORY_BASIC_INFORMATION info, SIZE_T len )
{
    SIZE_T ret;
    NTSTATUS status;

    if ((status = NtQueryVirtualMemory( process, addr, MemoryBasicInformation, info, len, &ret )))
    {
        SetLastError( RtlNtStatusToDosError(status) );
        ret = 0;
    }
    return ret;
}


/***********************************************************************
 *             MapViewOfFile   (KERNEL32.@)
 *
 * Maps a view of a file into the address space.
 *
 * PARAMS
 *  mapping     [I] File-mapping object to map.
 *  access      [I] Access mode.
 *  offset_high [I] High-order 32 bits of file offset.
 *  offset_low  [I] Low-order 32 bits of file offset.
 *  count       [I] Number of bytes to map.
 *
 * RETURNS
 *	Success: Starting address of mapped view.
 *	Failure: NULL.
 */
LPVOID WINAPI DECLSPEC_HOTPATCH MapViewOfFile( HANDLE mapping, DWORD access,
    DWORD offset_high, DWORD offset_low, SIZE_T count )
{
    return MapViewOfFileEx( mapping, access, offset_high,
                            offset_low, count, NULL );
}


/***********************************************************************
 *             MapViewOfFileEx   (KERNEL32.@)
 *
 * Maps a view of a file into the address space.
 *
 * PARAMS
 *  handle      [I] File-mapping object to map.
 *  access      [I] Access mode.
 *  offset_high [I] High-order 32 bits of file offset.
 *  offset_low  [I] Low-order 32 bits of file offset.
 *  count       [I] Number of bytes to map.
 *  addr        [I] Suggested starting address for mapped view.
 *
 * RETURNS
 *	Success: Starting address of mapped view.
 *	Failure: NULL.
 */
LPVOID WINAPI MapViewOfFileEx( HANDLE handle, DWORD access,
    DWORD offset_high, DWORD offset_low, SIZE_T count, LPVOID addr )
{
    NTSTATUS status;
    LARGE_INTEGER offset;
    ULONG protect;
    BOOL exec;

    offset.u.LowPart  = offset_low;
    offset.u.HighPart = offset_high;

    exec = access & FILE_MAP_EXECUTE;
    access &= ~FILE_MAP_EXECUTE;

    if (access == FILE_MAP_COPY)
        protect = exec ? PAGE_EXECUTE_WRITECOPY : PAGE_WRITECOPY;
    else if (access & FILE_MAP_WRITE)
        protect = exec ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
    else if (access & FILE_MAP_READ)
        protect = exec ? PAGE_EXECUTE_READ : PAGE_READONLY;
    else protect = PAGE_NOACCESS;

    if ((status = NtMapViewOfSection( handle, GetCurrentProcess(), &addr, 0, 0, &offset,
                                      &count, ViewShare, 0, protect )) < 0)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        addr = NULL;
    }
    return addr;
}


/***********************************************************************
 *             UnmapViewOfFile   (KERNEL32.@)
 *
 * Unmaps a mapped view of a file.
 *
 * PARAMS
 *  addr [I] Address where mapped view begins.
 *
 * RETURNS
 *	Success: TRUE.
 *	Failure: FALSE.
 *
 */
BOOL WINAPI UnmapViewOfFile( LPCVOID addr )
{
    NTSTATUS status;

    if (GetVersion() & 0x80000000)
    {
        MEMORY_BASIC_INFORMATION info;
        if (!VirtualQuery( addr, &info, sizeof(info) ) || info.AllocationBase != addr)
        {
            SetLastError( ERROR_INVALID_ADDRESS );
            return FALSE;
        }
    }

    status = NtUnmapViewOfSection( GetCurrentProcess(), (void *)addr );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *             FlushViewOfFile   (KERNEL32.@)
 *
 * Writes to the disk a byte range within a mapped view of a file.
 *
 * PARAMS
 *  base [I] Start address of byte range to flush.
 *  size [I] Number of bytes in range.
 *
 * RETURNS
 *	Success: TRUE.
 *	Failure: FALSE.
 */
BOOL WINAPI FlushViewOfFile( LPCVOID base, SIZE_T size )
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
 *             GetWriteWatch   (KERNEL32.@)
 */
UINT WINAPI GetWriteWatch( DWORD flags, LPVOID base, SIZE_T size, LPVOID *addresses,
                           ULONG_PTR *count, ULONG *granularity )
{
    NTSTATUS status;

    status = NtGetWriteWatch( GetCurrentProcess(), flags, base, size, addresses, count, granularity );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return status ? ~0u : 0;
}


/***********************************************************************
 *             ResetWriteWatch   (KERNEL32.@)
 */
UINT WINAPI ResetWriteWatch( LPVOID base, SIZE_T size )
{
    NTSTATUS status;

    status = NtResetWriteWatch( GetCurrentProcess(), base, size );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return status ? ~0u : 0;
}


/***********************************************************************
 *             IsBadReadPtr   (KERNEL32.@)
 *
 * Check for read access on a memory block.
 *
 * ptr  [I] Address of memory block.
 * size [I] Size of block.
 *
 * RETURNS
 *  Success: TRUE.
 *	Failure: FALSE. Process has read access to entire block.
 */
BOOL WINAPI IsBadReadPtr( LPCVOID ptr, UINT_PTR size )
{
    if (!size) return FALSE;  /* handle 0 size case w/o reference */
    if (!ptr) return TRUE;
    __TRY
    {
        volatile const char *p = ptr;
        char dummy __attribute__((unused));
        UINT_PTR count = size;

        while (count > system_info.PageSize)
        {
            dummy = *p;
            p += system_info.PageSize;
            count -= system_info.PageSize;
        }
        dummy = p[0];
        dummy = p[count - 1];
    }
    __EXCEPT_PAGE_FAULT
    {
        TRACE_(seh)("%p caused page fault during read\n", ptr);
        return TRUE;
    }
    __ENDTRY
    return FALSE;
}


/***********************************************************************
 *             IsBadWritePtr   (KERNEL32.@)
 *
 * Check for write access on a memory block.
 *
 * PARAMS
 *  ptr  [I] Address of memory block.
 *  size [I] Size of block in bytes.
 *
 * RETURNS
 *  Success: TRUE.
 *	Failure: FALSE. Process has write access to entire block.
 */
BOOL WINAPI IsBadWritePtr( LPVOID ptr, UINT_PTR size )
{
    if (!size) return FALSE;  /* handle 0 size case w/o reference */
    if (!ptr) return TRUE;
    __TRY
    {
        volatile char *p = ptr;
        UINT_PTR count = size;

        while (count > system_info.PageSize)
        {
            *p |= 0;
            p += system_info.PageSize;
            count -= system_info.PageSize;
        }
        p[0] |= 0;
        p[count - 1] |= 0;
    }
    __EXCEPT_PAGE_FAULT
    {
        TRACE_(seh)("%p caused page fault during write\n", ptr);
        return TRUE;
    }
    __ENDTRY
    return FALSE;
}


/***********************************************************************
 *             IsBadHugeReadPtr   (KERNEL32.@)
 *
 * Check for read access on a memory block.
 *
 * PARAMS
 *  ptr  [I] Address of memory block.
 *  size [I] Size of block.
 *
 * RETURNS
 *  Success: TRUE.
 *	Failure: FALSE. Process has read access to entire block.
 */
BOOL WINAPI IsBadHugeReadPtr( LPCVOID ptr, UINT_PTR size )
{
    return IsBadReadPtr( ptr, size );
}


/***********************************************************************
 *             IsBadHugeWritePtr   (KERNEL32.@)
 *
 * Check for write access on a memory block.
 *
 * PARAMS
 *  ptr  [I] Address of memory block.
 *  size [I] Size of block.
 *
 * RETURNS
 *  Success: TRUE.
 *	Failure: FALSE. Process has write access to entire block.
 */
BOOL WINAPI IsBadHugeWritePtr( LPVOID ptr, UINT_PTR size )
{
    return IsBadWritePtr( ptr, size );
}


/***********************************************************************
 *             IsBadCodePtr   (KERNEL32.@)
 *
 * Check for read access on a memory address.
 *
 * PARAMS
 *  ptr [I] Address of function.
 *
 * RETURNS
 *	Success: TRUE.
 *	Failure: FALSE. Process has read access to specified memory.
 */
BOOL WINAPI IsBadCodePtr( FARPROC ptr )
{
    return IsBadReadPtr( ptr, 1 );
}


/***********************************************************************
 *             IsBadStringPtrA   (KERNEL32.@)
 *
 * Check for read access on a range of memory pointed to by a string pointer.
 *
 * PARAMS
 *  str [I] Address of string.
 *  max [I] Maximum size of string.
 *
 * RETURNS
 *	Success: TRUE.
 *	Failure: FALSE. Read access to all bytes in string.
 */
BOOL WINAPI IsBadStringPtrA( LPCSTR str, UINT_PTR max )
{
    if (!str) return TRUE;
    
    __TRY
    {
        volatile const char *p = str;
        while (p != str + max) if (!*p++) break;
    }
    __EXCEPT_PAGE_FAULT
    {
        TRACE_(seh)("%p caused page fault during read\n", str);
        return TRUE;
    }
    __ENDTRY
    return FALSE;
}


/***********************************************************************
 *             IsBadStringPtrW   (KERNEL32.@)
 *
 * See IsBadStringPtrA.
 */
BOOL WINAPI IsBadStringPtrW( LPCWSTR str, UINT_PTR max )
{
    if (!str) return TRUE;
    
    __TRY
    {
        volatile const WCHAR *p = str;
        while (p != str + max) if (!*p++) break;
    }
    __EXCEPT_PAGE_FAULT
    {
        TRACE_(seh)("%p caused page fault during read\n", str);
        return TRUE;
    }
    __ENDTRY
    return FALSE;
}

/***********************************************************************
 *           K32GetMappedFileNameA (KERNEL32.@)
 */
DWORD WINAPI K32GetMappedFileNameA(HANDLE process, LPVOID lpv, LPSTR file_name, DWORD size)
{
    FIXME_(file)("(%p, %p, %p, %d): stub\n", process, lpv, file_name, size);

    if (file_name && size)
        file_name[0] = '\0';

    return 0;
}

/***********************************************************************
 *           K32GetMappedFileNameW (KERNEL32.@)
 */
DWORD WINAPI K32GetMappedFileNameW(HANDLE process, LPVOID lpv, LPWSTR file_name, DWORD size)
{
    FIXME_(file)("(%p, %p, %p, %d): stub\n", process, lpv, file_name, size);

    if (file_name && size)
        file_name[0] = '\0';

    return 0;
}

/***********************************************************************
 *           K32EnumPageFilesA (KERNEL32.@)
 */
BOOL WINAPI K32EnumPageFilesA( PENUM_PAGE_FILE_CALLBACKA callback, LPVOID context )
{
    FIXME_(file)("(%p, %p) stub\n", callback, context );
    return FALSE;
}

/***********************************************************************
 *           K32EnumPageFilesW (KERNEL32.@)
 */
BOOL WINAPI K32EnumPageFilesW( PENUM_PAGE_FILE_CALLBACKW callback, LPVOID context )
{
    FIXME_(file)("(%p, %p) stub\n", callback, context );
    return FALSE;
}

/***********************************************************************
 *           K32GetWsChanges (KERNEL32.@)
 */
BOOL WINAPI K32GetWsChanges(HANDLE process, PPSAPI_WS_WATCH_INFORMATION watchinfo, DWORD size)
{
    NTSTATUS status;

    TRACE_(seh)("(%p, %p, %d)\n", process, watchinfo, size);

    status = NtQueryInformationProcess( process, ProcessWorkingSetWatch, watchinfo, size, NULL );

    if (status)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           K32InitializeProcessForWsWatch (KERNEL32.@)
 */
BOOL WINAPI K32InitializeProcessForWsWatch(HANDLE process)
{
    FIXME_(seh)("(process=%p): stub\n", process);

    return TRUE;
}
