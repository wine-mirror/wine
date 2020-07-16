/*
 * Win32 virtual memory functions
 *
 * Copyright 1997, 2002 Alexandre Julliard
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

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winternl.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(virtual);


/**********************************************************************
 *           RtlCreateUserStack (NTDLL.@)
 */
NTSTATUS WINAPI RtlCreateUserStack( SIZE_T commit, SIZE_T reserve, ULONG zero_bits,
                                    SIZE_T commit_align, SIZE_T reserve_align, INITIAL_TEB *stack )
{
    TRACE("commit %#lx, reserve %#lx, zero_bits %u, commit_align %#lx, reserve_align %#lx, stack %p\n",
            commit, reserve, zero_bits, commit_align, reserve_align, stack);

    if (!commit_align || !reserve_align)
        return STATUS_INVALID_PARAMETER;

    if (!commit || !reserve)
    {
        IMAGE_NT_HEADERS *nt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress );
        if (!reserve) reserve = nt->OptionalHeader.SizeOfStackReserve;
        if (!commit) commit = nt->OptionalHeader.SizeOfStackCommit;
    }

    reserve = (reserve + reserve_align - 1) & ~(reserve_align - 1);
    commit = (commit + commit_align - 1) & ~(commit_align - 1);

    return unix_funcs->virtual_alloc_thread_stack( stack, reserve, commit, NULL );
}


/**********************************************************************
 *           RtlFreeUserStack (NTDLL.@)
 */
void WINAPI RtlFreeUserStack( void *stack )
{
    SIZE_T size = 0;

    TRACE("stack %p\n", stack);

    NtFreeVirtualMemory( NtCurrentProcess(), &stack, &size, MEM_RELEASE );
}

/***********************************************************************
 *           __wine_locked_recvmsg
 */
ssize_t CDECL __wine_locked_recvmsg( int fd, struct msghdr *hdr, int flags )
{
    return unix_funcs->virtual_locked_recvmsg( fd, hdr, flags );
}
