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

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

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
    static EXECUTION_STATE current =
        ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED | ES_USER_PRESENT;
    EXECUTION_STATE old = current;

    WARN("(0x%x): stub, harmless.\n", flags);

    if (!(current & ES_CONTINUOUS) || (flags & ES_CONTINUOUS))
        current = flags;
    return old;
}

/***********************************************************************
 *           PowerCreateRequest      (KERNEL32.@)
 */
HANDLE WINAPI PowerCreateRequest(REASON_CONTEXT *context)
{
    FIXME("(%p): stub\n", context);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *           PowerSetRequest      (KERNEL32.@)
 */
BOOL WINAPI PowerSetRequest(HANDLE request, POWER_REQUEST_TYPE type)
{
    FIXME("(%p, %u): stub\n", request, type);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           PowerClearRequest      (KERNEL32.@)
 */
BOOL WINAPI PowerClearRequest(HANDLE request, POWER_REQUEST_TYPE type)
{
    FIXME("(%p, %u): stub\n", request, type);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
