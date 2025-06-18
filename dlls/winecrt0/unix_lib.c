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

static inline void *get_dispatcher( const char *name )
{
    UNICODE_STRING ntdll_name = RTL_CONSTANT_STRING( L"ntdll.dll" );
    HMODULE module;
    void **dispatcher;

    LdrGetDllHandle( NULL, 0, &ntdll_name, &module );
    dispatcher = RtlFindExportedRoutineByName( module, "__wine_unix_call_dispatcher" );
    return dispatcher ? *dispatcher : (void *)unix_call_fallback;
}

static NTSTATUS WINAPI unix_call_init( unixlib_handle_t handle, unsigned int code, void *args )
{
    InterlockedExchangePointer( (void **)&__wine_unix_call_dispatcher,
                                get_dispatcher( "__wine_unix_call_dispatcher" ));
    return __wine_unix_call_dispatcher( handle, code, args );
}

unixlib_handle_t __wine_unixlib_handle = 0;
NTSTATUS (WINAPI *__wine_unix_call_dispatcher)( unixlib_handle_t, unsigned int, void * ) = unix_call_init;

#ifdef __arm64ec__

static NTSTATUS WINAPI unix_call_init_arm64ec( unixlib_handle_t handle, unsigned int code, void *args );

static __attribute__((used)) NTSTATUS (WINAPI *__wine_unix_call_dispatcher_arm64ec)( unixlib_handle_t, unsigned int, void * ) = unix_call_init_arm64ec;

static NTSTATUS WINAPI unix_call_init_arm64ec( unixlib_handle_t handle, unsigned int code, void *args )
{
    InterlockedExchangePointer( (void **)&__wine_unix_call_dispatcher_arm64ec,
                                get_dispatcher( "__wine_unix_call_dispatcher_arm64ec" ));
    return __wine_unix_call_arm64ec( handle, code, args );
}

NTSTATUS __attribute__((naked)) __wine_unix_call_arm64ec( unixlib_handle_t handle, unsigned int code, void *args )
{
    asm( ".seh_proc \"#__wine_unix_call_arm64ec\"\n\t"
         ".seh_endprologue\n\t"
         "adrp x16, __wine_unix_call_dispatcher_arm64ec\n\t"
         "ldr x16, [x16, #:lo12:__wine_unix_call_dispatcher_arm64ec]\n\t"
         "br x16\n\t"
         ".seh_endproc" );
}

#endif

NTSTATUS WINAPI __wine_init_unix_call(void)
{
    return NtQueryVirtualMemory( GetCurrentProcess(), image_base(), MemoryWineUnixFuncs,
                                 &__wine_unixlib_handle, sizeof(__wine_unixlib_handle), NULL );
}
