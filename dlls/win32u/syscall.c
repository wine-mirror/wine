/*
 * Unix interface for Win32 syscalls
 *
 * Copyright (C) 2021 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "ntgdi_private.h"
#include "ntuser_private.h"
#include "ntuser.h"
#include "wine/unixlib.h"
#include "win32syscalls.h"

ULONG_PTR zero_bits = 0;

static ULONG_PTR syscalls[] =
{
#define SYSCALL_ENTRY(id,name,args) (ULONG_PTR)name,
    ALL_SYSCALLS
#undef SYSCALL_ENTRY
};

static BYTE arguments[ARRAY_SIZE(syscalls)] =
{
#define SYSCALL_ENTRY(id,name,args) args,
    ALL_SYSCALLS
#undef SYSCALL_ENTRY
};

static NTSTATUS init( void *args )
{
#ifdef _WIN64
    if (NtCurrentTeb()->WowTebOffset)
    {
        SYSTEM_BASIC_INFORMATION info;

        NtQuerySystemInformation(SystemEmulationBasicInformation, &info, sizeof(info), NULL);
        zero_bits = (ULONG_PTR)info.HighestUserAddress | 0x7fffffff;
    }
#endif
    KeAddSystemServiceTable( syscalls, NULL, ARRAY_SIZE(syscalls), arguments, 1 );
    return STATUS_SUCCESS;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    init,
};
