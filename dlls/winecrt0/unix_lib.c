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

#ifdef __WINE_PE_BUILD

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/unixlib.h"

static NTSTATUS (WINAPI *p__wine_unix_call)( unixlib_handle_t, unsigned int, void * );

static void load_func( void **func, const char *name, void *def )
{
    if (!*func)
    {
        HMODULE module = GetModuleHandleA( "ntdll.dll" );
        void *proc = GetProcAddress( module, name );
        InterlockedExchangePointer( func, proc ? proc : def );
    }
}
#define LOAD_FUNC(name) load_func( (void **)&p ## name, #name, fallback ## name )

static NTSTATUS __cdecl fallback__wine_unix_call( unixlib_handle_t handle, unsigned int code, void *args )
{
    return STATUS_DLL_NOT_FOUND;
}

NTSTATUS WINAPI __wine_unix_call( unixlib_handle_t handle, unsigned int code, void *args )
{
    LOAD_FUNC( __wine_unix_call );
    return p__wine_unix_call( handle, code, args );
}

#endif  /* __WINE_PE_BUILD */
