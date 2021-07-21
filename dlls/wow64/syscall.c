/*
 * WoW64 syscall wrapping
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
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wow);

void *dummy = RtlUnwind;

BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, void *reserved )
{
    if (reason == DLL_PROCESS_ATTACH) LdrDisableThreadCalloutsForDll( inst );
    return TRUE;
}

void __cdecl __wine_spec_unimplemented_stub( const char *module, const char *function )
{
    EXCEPTION_RECORD record;

    record.ExceptionCode    = EXCEPTION_WINE_STUB;
    record.ExceptionFlags   = EH_NONCONTINUABLE;
    record.ExceptionRecord  = NULL;
    record.ExceptionAddress = __wine_spec_unimplemented_stub;
    record.NumberParameters = 2;
    record.ExceptionInformation[0] = (ULONG_PTR)module;
    record.ExceptionInformation[1] = (ULONG_PTR)function;
    for (;;) RtlRaiseException( &record );
}


/**********************************************************************
 *           Wow64SystemServiceEx  (NTDLL.@)
 */
NTSTATUS WINAPI Wow64SystemServiceEx( UINT num, UINT *args )
{
    FIXME( "stub\n" );
    return STATUS_INVALID_SYSTEM_SERVICE;
}


/**********************************************************************
 *           Wow64LdrpInitialize  (NTDLL.@)
 */
void WINAPI Wow64LdrpInitialize( CONTEXT *context )
{
    FIXME( "stub\n" );
}
