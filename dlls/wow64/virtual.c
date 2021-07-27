/*
 * WoW64 virtual memory functions
 *
 * Copyright 2021 Alexandre Julliard
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winternl.h"
#include "winioctl.h"
#include "wow64_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wow);


/**********************************************************************
 *           wow64_NtAllocateVirtualMemory
 */
NTSTATUS WINAPI wow64_NtAllocateVirtualMemory( UINT *args )
{
    HANDLE process = get_handle( &args );
    ULONG *addr32 = get_ptr( &args );
    ULONG_PTR zero_bits = get_ulong( &args );
    ULONG *size32 = get_ptr( &args );
    ULONG type = get_ulong( &args );
    ULONG protect = get_ulong( &args );

    void *addr;
    SIZE_T size;
    NTSTATUS status;

    status = NtAllocateVirtualMemory( process, addr_32to64( &addr, addr32 ), get_zero_bits( zero_bits ),
                                      size_32to64( &size, size32 ), type, protect );
    if (!status)
    {
        put_addr( addr32, addr );
        put_size( size32, size );
    }
    return status;
}


/**********************************************************************
 *           wow64_NtAllocateVirtualMemoryEx
 */
NTSTATUS WINAPI wow64_NtAllocateVirtualMemoryEx( UINT *args )
{
    HANDLE process = get_handle( &args );
    ULONG *addr32 = get_ptr( &args );
    ULONG *size32 = get_ptr( &args );
    ULONG type = get_ulong( &args );
    ULONG protect = get_ulong( &args );
    MEM_EXTENDED_PARAMETER *params = get_ptr( &args );
    ULONG count = get_ulong( &args );

    void *addr;
    SIZE_T size;
    NTSTATUS status;

    if (count) FIXME( "%d extended parameters %p\n", count, params );
    status = NtAllocateVirtualMemoryEx( process, addr_32to64( &addr, addr32 ), size_32to64( &size, size32 ),
                                        type, protect, params, count );
    if (!status)
    {
        put_addr( addr32, addr );
        put_size( size32, size );
    }
    return status;
}


/**********************************************************************
 *           wow64_NtFlushVirtualMemory
 */
NTSTATUS WINAPI wow64_NtFlushVirtualMemory( UINT *args )
{
    HANDLE process = get_handle( &args );
    ULONG *addr32 = get_ptr( &args );
    ULONG *size32 = get_ptr( &args );
    ULONG unknown = get_ulong( &args );

    void *addr;
    SIZE_T size;
    NTSTATUS status;

    status = NtFlushVirtualMemory( process, (const void **)addr_32to64( &addr, addr32 ),
                                   size_32to64( &size, size32 ), unknown );
    if (!status)
    {
        put_addr( addr32, addr );
        put_size( size32, size );
    }
    return status;
}


/**********************************************************************
 *           wow64_NtFreeVirtualMemory
 */
NTSTATUS WINAPI wow64_NtFreeVirtualMemory( UINT *args )
{
    HANDLE process = get_handle( &args );
    ULONG *addr32 = get_ptr( &args );
    ULONG *size32 = get_ptr( &args );
    ULONG type = get_ulong( &args );

    void *addr;
    SIZE_T size;
    NTSTATUS status;

    status = NtFreeVirtualMemory( process, addr_32to64( &addr, addr32 ),
                                  size_32to64( &size, size32 ), type );
    if (!status)
    {
        put_addr( addr32, addr );
        put_size( size32, size );
    }
    return status;
}


/**********************************************************************
 *           wow64_NtLockVirtualMemory
 */
NTSTATUS WINAPI wow64_NtLockVirtualMemory( UINT *args )
{
    HANDLE process = get_handle( &args );
    ULONG *addr32 = get_ptr( &args );
    ULONG *size32 = get_ptr( &args );
    ULONG unknown = get_ulong( &args );

    void *addr;
    SIZE_T size;
    NTSTATUS status;

    status = NtLockVirtualMemory( process, addr_32to64( &addr, addr32 ),
                                  size_32to64( &size, size32 ), unknown );
    if (!status)
    {
        put_addr( addr32, addr );
        put_size( size32, size );
    }
    return status;
}


/**********************************************************************
 *           wow64_NtProtectVirtualMemory
 */
NTSTATUS WINAPI wow64_NtProtectVirtualMemory( UINT *args )
{
    HANDLE process = get_handle( &args );
    ULONG *addr32 = get_ptr( &args );
    ULONG *size32 = get_ptr( &args );
    ULONG new_prot = get_ulong( &args );
    ULONG *old_prot = get_ptr( &args );

    void *addr;
    SIZE_T size;
    NTSTATUS status;

    status = NtProtectVirtualMemory( process, addr_32to64( &addr, addr32 ),
                                     size_32to64( &size, size32 ), new_prot, old_prot );
    if (!status)
    {
        put_addr( addr32, addr );
        put_size( size32, size );
    }
    return status;
}


/**********************************************************************
 *           wow64_NtReadVirtualMemory
 */
NTSTATUS WINAPI wow64_NtReadVirtualMemory( UINT *args )
{
    HANDLE process = get_handle( &args );
    const void *addr = get_ptr( &args );
    void *buffer = get_ptr( &args );
    SIZE_T size = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    SIZE_T ret_size;
    NTSTATUS status;

    status = NtReadVirtualMemory( process, addr, buffer, size, &ret_size );
    put_size( retlen, ret_size );
    return status;
}


/**********************************************************************
 *           wow64_NtUnlockVirtualMemory
 */
NTSTATUS WINAPI wow64_NtUnlockVirtualMemory( UINT *args )
{
    HANDLE process = get_handle( &args );
    ULONG *addr32 = get_ptr( &args );
    ULONG *size32 = get_ptr( &args );
    ULONG unknown = get_ulong( &args );

    void *addr;
    SIZE_T size;
    NTSTATUS status;

    status = NtUnlockVirtualMemory( process, addr_32to64( &addr, addr32 ),
                                    size_32to64( &size, size32 ), unknown );
    if (!status)
    {
        put_addr( addr32, addr );
        put_size( size32, size );
    }
    return status;
}


/**********************************************************************
 *           wow64_NtWriteVirtualMemory
 */
NTSTATUS WINAPI wow64_NtWriteVirtualMemory( UINT *args )
{
    HANDLE process = get_handle( &args );
    void *addr = get_ptr( &args );
    const void *buffer = get_ptr( &args );
    SIZE_T size = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    SIZE_T ret_size;
    NTSTATUS status;

    status = NtWriteVirtualMemory( process, addr, buffer, size, &ret_size );
    put_size( retlen, ret_size );
    return status;
}
