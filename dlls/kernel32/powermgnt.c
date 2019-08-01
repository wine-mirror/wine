/*
 * Copyright 1995 Thomas Sandford (tdgsandf@prds-grn.demon.co.uk)
 * Copyright 2003 Dimitrie O. Paun
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(powermgnt);

/******************************************************************************
 *           GetDevicePowerState   (KERNEL32.@)
 */
BOOL WINAPI GetDevicePowerState(HANDLE hDevice, BOOL* pfOn)
{
    WARN("(hDevice %p pfOn %p): stub\n", hDevice, pfOn);
    return TRUE; /* no information */
}

/***********************************************************************
 *           GetSystemPowerStatus      (KERNEL32.@)
 */
BOOL WINAPI GetSystemPowerStatus(LPSYSTEM_POWER_STATUS ps)
{
    WARN("(%p): stub, harmless.\n", ps);

    if (ps)
    {
        ps->ACLineStatus        = 255;
        ps->BatteryFlag         = 255;
        ps->BatteryLifePercent  = 255;
        ps->Reserved1           = 0;
        ps->BatteryLifeTime     = ~0u;
        ps->BatteryFullLifeTime = ~0u;
        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *           IsSystemResumeAutomatic   (KERNEL32.@)
 */
BOOL WINAPI IsSystemResumeAutomatic(void)
{
    WARN("(): stub, harmless.\n");
    return FALSE;
}

/***********************************************************************
 *           RequestWakeupLatency      (KERNEL32.@)
 */
BOOL WINAPI RequestWakeupLatency(LATENCY_TIME latency)
{
    WARN("(): stub, harmless.\n");
    return TRUE;
}

/***********************************************************************
 *           RequestDeviceWakeup      (KERNEL32.@)
 */
BOOL WINAPI RequestDeviceWakeup(HANDLE device)
{
    FIXME("(%p): stub\n", device);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           SetSystemPowerState      (KERNEL32.@)
 */
BOOL WINAPI SetSystemPowerState(BOOL suspend_or_hibernate,
                                  BOOL force_flag)
{
    WARN("(): stub, harmless.\n");
    return TRUE;
}

/***********************************************************************
 * SetThreadExecutionState (KERNEL32.@)
 *
 * Informs the system that activity is taking place for
 * power management purposes.
 */
EXECUTION_STATE WINAPI SetThreadExecutionState(EXECUTION_STATE flags)
{
    EXECUTION_STATE old;

    NtSetThreadExecutionState(flags, &old);

    return old;
}

/***********************************************************************
 *           PowerCreateRequest      (KERNEL32.@)
 */
HANDLE WINAPI PowerCreateRequest(REASON_CONTEXT *context)
{
    COUNTED_REASON_CONTEXT nt_context;
    HANDLE handle;
    NTSTATUS status;
    WCHAR module_name[MAX_PATH];

    TRACE( "(%p)\n", context );

    nt_context.Version = context->Version;
    nt_context.Flags = context->Flags;
    if (context->Flags & POWER_REQUEST_CONTEXT_SIMPLE_STRING)
        RtlInitUnicodeString( &nt_context.u.SimpleString, context->Reason.SimpleReasonString );
    else if (context->Flags & POWER_REQUEST_CONTEXT_DETAILED_STRING)
    {
        int i;

        GetModuleFileNameW( context->Reason.Detailed.LocalizedReasonModule, module_name, ARRAY_SIZE(module_name) );
        RtlInitUnicodeString( &nt_context.u.s.ResourceFileName, module_name );
        nt_context.u.s.ResourceReasonId = context->Reason.Detailed.LocalizedReasonId;
        nt_context.u.s.StringCount = context->Reason.Detailed.ReasonStringCount;
        nt_context.u.s.ReasonStrings = heap_alloc( nt_context.u.s.StringCount * sizeof(UNICODE_STRING) );
        for (i = 0; i < nt_context.u.s.StringCount; i++)
            RtlInitUnicodeString( &nt_context.u.s.ReasonStrings[i], context->Reason.Detailed.ReasonStrings[i] );
    }

    status = NtCreatePowerRequest( &handle, &nt_context );
    if (nt_context.Flags & POWER_REQUEST_CONTEXT_DETAILED_STRING)
        heap_free( nt_context.u.s.ReasonStrings );
    if (status)
        SetLastError( RtlNtStatusToDosError(status) );
    return status == STATUS_SUCCESS ? handle : INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *           PowerSetRequest      (KERNEL32.@)
 */
BOOL WINAPI PowerSetRequest(HANDLE request, POWER_REQUEST_TYPE type)
{
    NTSTATUS status = NtSetPowerRequest( request, type );
    if (status)
        SetLastError( RtlNtStatusToDosError(status) );
    return status == STATUS_SUCCESS;
}

/***********************************************************************
 *           PowerClearRequest      (KERNEL32.@)
 */
BOOL WINAPI PowerClearRequest(HANDLE request, POWER_REQUEST_TYPE type)
{
    NTSTATUS status = NtClearPowerRequest( request, type );
    if (status)
        SetLastError( RtlNtStatusToDosError(status) );
    return status == STATUS_SUCCESS;
}
