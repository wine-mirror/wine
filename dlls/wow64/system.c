/*
 * WoW64 system functions
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
#include "wow64_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wow);


/**********************************************************************
 *           wow64_NtDisplayString
 */
NTSTATUS WINAPI wow64_NtDisplayString( UINT *args )
{
    const UNICODE_STRING32 *str32 = get_ptr( &args );

    UNICODE_STRING str;

    return NtDisplayString( unicode_str_32to64( &str, str32 ));
}


/**********************************************************************
 *           wow64_NtInitiatePowerAction
 */
NTSTATUS WINAPI wow64_NtInitiatePowerAction( UINT *args )
{
    POWER_ACTION action = get_ulong( &args );
    SYSTEM_POWER_STATE state = get_ulong( &args );
    ULONG flags = get_ulong( &args );
    BOOLEAN async = get_ulong( &args );

    return NtInitiatePowerAction( action, state, flags, async );
}


/**********************************************************************
 *           wow64_NtLoadDriver
 */
NTSTATUS WINAPI wow64_NtLoadDriver( UINT *args )
{
    UNICODE_STRING32 *str32 = get_ptr( &args );

    UNICODE_STRING str;

    return NtLoadDriver( unicode_str_32to64( &str, str32 ));
}


/**********************************************************************
 *           wow64_NtPowerInformation
 */
NTSTATUS WINAPI wow64_NtPowerInformation( UINT *args )
{
    POWER_INFORMATION_LEVEL level = get_ulong( &args );
    void *in_buf = get_ptr( &args );
    ULONG in_len = get_ulong( &args );
    void *out_buf = get_ptr( &args );
    ULONG out_len = get_ulong( &args );

    switch (level)
    {
    case SystemPowerCapabilities:   /* SYSTEM_POWER_CAPABILITIES */
    case SystemBatteryState:   /* SYSTEM_BATTERY_STATE */
    case SystemExecutionState:   /* ULONG */
    case ProcessorInformation:   /* PROCESSOR_POWER_INFORMATION */
        return NtPowerInformation( level, in_buf, in_len, out_buf, out_len );

    default:
        FIXME( "unsupported level %u\n", level );
        return STATUS_NOT_IMPLEMENTED;
    }
}


/**********************************************************************
 *           wow64_NtQueryLicenseValue
 */
NTSTATUS WINAPI wow64_NtQueryLicenseValue( UINT *args )
{
    UNICODE_STRING32 *str32 = get_ptr( &args );
    ULONG *type = get_ptr( &args );
    void *buffer = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    UNICODE_STRING str;

    return NtQueryLicenseValue( unicode_str_32to64( &str, str32 ), type, buffer, len, retlen );
}


/**********************************************************************
 *           wow64_NtQuerySystemEnvironmentValue
 */
NTSTATUS WINAPI wow64_NtQuerySystemEnvironmentValue( UINT *args )
{
    UNICODE_STRING32 *str32 = get_ptr( &args );
    WCHAR *buffer = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    UNICODE_STRING str;

    return NtQuerySystemEnvironmentValue( unicode_str_32to64( &str, str32 ), buffer, len, retlen );
}


/**********************************************************************
 *           wow64_NtQuerySystemEnvironmentValueEx
 */
NTSTATUS WINAPI wow64_NtQuerySystemEnvironmentValueEx( UINT *args )
{
    UNICODE_STRING32 *str32 = get_ptr( &args );
    GUID *vendor = get_ptr( &args );
    void *buffer = get_ptr( &args );
    ULONG *retlen = get_ptr( &args );
    ULONG *attrib = get_ptr( &args );

    UNICODE_STRING str;

    return NtQuerySystemEnvironmentValueEx( unicode_str_32to64( &str, str32 ),
                                            vendor, buffer, retlen, attrib );
}


/**********************************************************************
 *           wow64_NtQuerySystemTime
 */
NTSTATUS WINAPI wow64_NtQuerySystemTime( UINT *args )
{
    LARGE_INTEGER *time = get_ptr( &args );

    return NtQuerySystemTime( time );
}


/**********************************************************************
 *           wow64_NtSetIntervalProfile
 */
NTSTATUS WINAPI wow64_NtSetIntervalProfile( UINT *args )
{
    ULONG interval = get_ulong( &args );
    KPROFILE_SOURCE source = get_ulong( &args );

    return NtSetIntervalProfile( interval, source );
}


/**********************************************************************
 *           wow64_NtSetSystemInformation
 */
NTSTATUS WINAPI wow64_NtSetSystemInformation( UINT *args )
{
    SYSTEM_INFORMATION_CLASS class = get_ulong( &args );
    void *info = get_ptr( &args );
    ULONG len = get_ulong( &args );

    return NtSetSystemInformation( class, info, len );
}


/**********************************************************************
 *           wow64_NtSetSystemTime
 */
NTSTATUS WINAPI wow64_NtSetSystemTime( UINT *args )
{
    const LARGE_INTEGER *new = get_ptr( &args );
    LARGE_INTEGER *old = get_ptr( &args );

    return NtSetSystemTime( new, old );
}


/**********************************************************************
 *           wow64_NtShutdownSystem
 */
NTSTATUS WINAPI wow64_NtShutdownSystem( UINT *args )
{
    SHUTDOWN_ACTION action = get_ulong( &args );

    return NtShutdownSystem( action );
}


/**********************************************************************
 *           wow64_NtSystemDebugControl
 */
NTSTATUS WINAPI wow64_NtSystemDebugControl( UINT *args )
{
    SYSDBG_COMMAND command = get_ulong( &args );
    void *in_buf = get_ptr( &args );
    ULONG in_len = get_ulong( &args );
    void *out_buf = get_ptr( &args );
    ULONG out_len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    return NtSystemDebugControl( command, in_buf, in_len, out_buf, out_len, retlen );
}


/**********************************************************************
 *           wow64_NtUnloadDriver
 */
NTSTATUS WINAPI wow64_NtUnloadDriver( UINT *args )
{
    UNICODE_STRING32 *str32 = get_ptr( &args );

    UNICODE_STRING str;

    return NtUnloadDriver( unicode_str_32to64( &str, str32 ));
}
