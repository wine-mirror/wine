/*
 * fltmgr.sys
 *
 * Copyright 2015 Austin English
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
#include "ddk/fltkernel.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(fltmgr);

NTSTATUS WINAPI DriverEntry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    TRACE( "(%p, %s)\n", driver, debugstr_w(path->Buffer) );

    return STATUS_SUCCESS;
}

void WINAPI FltInitializePushLock( EX_PUSH_LOCK *lock )
{
    FIXME( "(%p): stub\n", lock );
}

void WINAPI FltDeletePushLock( EX_PUSH_LOCK *lock )
{
    FIXME( "(%p): stub\n", lock );
}

void WINAPI FltAcquirePushLockExclusive( EX_PUSH_LOCK *lock )
{
    FIXME( "(%p): stub\n", lock );
}

void WINAPI FltReleasePushLock( EX_PUSH_LOCK *lock )
{
    FIXME( "(%p): stub\n", lock );
}

NTSTATUS WINAPI FltRegisterFilter( PDRIVER_OBJECT driver, const FLT_REGISTRATION *reg, PFLT_FILTER *filter )
{
    FIXME( "(%p,%p,%p): stub\n", driver, reg, filter );

    if(filter)
        *filter = UlongToHandle(0xdeadbeaf);

    return STATUS_SUCCESS;
}

NTSTATUS WINAPI FltStartFiltering( PFLT_FILTER filter )
{
    FIXME( "(%p): stub\n", filter );

    return STATUS_SUCCESS;
}

void WINAPI FltUnregisterFilter( PFLT_FILTER filter )
{
    FIXME( "(%p): stub\n", filter );
}

void* WINAPI FltGetRoutineAddress(LPCSTR name)
{
    HMODULE mod = GetModuleHandleW(L"fltmgr.sys");
    void *func;

    func = GetProcAddress(mod, name);
    if (func)
        TRACE( "%s -> %p\n", debugstr_a(name), func );
    else
        FIXME( "%s not found\n", debugstr_a(name) );

    return func;
}
