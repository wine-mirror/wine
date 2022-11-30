/*
 * Support for the Unix part of builtin dlls
 *
 * Copyright 2019 Alexandre Julliard
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
#include "winternl.h"
#include "wine/unixlib.h"

static inline void *image_base(void)
{
#ifdef __WINE_PE_BUILD
    extern IMAGE_DOS_HEADER __ImageBase;
    return (void *)&__ImageBase;
#else
    extern IMAGE_NT_HEADERS __wine_spec_nt_header;
    return (void *)((__wine_spec_nt_header.OptionalHeader.ImageBase + 0xffff) & ~0xffff);
#endif
}

static NTSTATUS WINAPI unix_call_fallback( unixlib_handle_t handle, unsigned int code, void *args )
{
    return STATUS_DLL_NOT_FOUND;
}

unixlib_handle_t __wine_unixlib_handle = 0;
NTSTATUS (WINAPI *__wine_unix_call_dispatcher)( unixlib_handle_t, unsigned int, void * ) = unix_call_fallback;

NTSTATUS WINAPI __wine_init_unix_call(void)
{
    NTSTATUS status;
    HMODULE module = GetModuleHandleW( L"ntdll.dll" );
    void **p__wine_unix_call_dispatcher = (void **)GetProcAddress( module, "__wine_unix_call_dispatcher" );

    if (!p__wine_unix_call_dispatcher) return STATUS_DLL_NOT_FOUND;
    status = NtQueryVirtualMemory( GetCurrentProcess(), image_base(), MemoryWineUnixFuncs,
                                   &__wine_unixlib_handle, sizeof(__wine_unixlib_handle), NULL );
    if (!status) __wine_unix_call_dispatcher = *p__wine_unix_call_dispatcher;
    return status;
}
