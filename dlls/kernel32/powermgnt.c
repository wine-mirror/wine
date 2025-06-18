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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "kernel_private.h"
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
    SYSTEM_BATTERY_STATE bs;
    NTSTATUS status;

    TRACE("(%p)\n", ps);

    ps->ACLineStatus        = AC_LINE_UNKNOWN;
    ps->BatteryFlag         = BATTERY_FLAG_UNKNOWN;
    ps->BatteryLifePercent  = BATTERY_PERCENTAGE_UNKNOWN;
    ps->SystemStatusFlag    = 0;
    ps->BatteryLifeTime     = BATTERY_LIFE_UNKNOWN;
    ps->BatteryFullLifeTime = BATTERY_LIFE_UNKNOWN;

    status = NtPowerInformation(SystemBatteryState, NULL, 0, &bs, sizeof(bs));
    if (status == STATUS_NOT_IMPLEMENTED) return TRUE;
    if (FAILED(status)) return FALSE;

    ps->ACLineStatus = bs.AcOnLine;

    if (bs.BatteryPresent)
    {
        ps->BatteryLifePercent = bs.MaxCapacity ? 100 * bs.RemainingCapacity / bs.MaxCapacity : 100;
        ps->BatteryLifeTime = bs.EstimatedTime;
        if (!bs.Charging && (LONG)bs.Rate < 0)
            ps->BatteryFullLifeTime = 3600 * bs.MaxCapacity / -(LONG)bs.Rate;

        ps->BatteryFlag = 0;
        if (bs.Charging)
            ps->BatteryFlag |= BATTERY_FLAG_CHARGING;
        if (ps->BatteryLifePercent > 66)
            ps->BatteryFlag |= BATTERY_FLAG_HIGH;
        if (ps->BatteryLifePercent < 33)
            ps->BatteryFlag |= BATTERY_FLAG_LOW;
        if (ps->BatteryLifePercent < 5)
            ps->BatteryFlag |= BATTERY_FLAG_CRITICAL;
    }
    else
    {
        ps->BatteryFlag = BATTERY_FLAG_NO_BATTERY;
    }

    return TRUE;
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
    FIXME("(%p): stub\n", context);

    return CreateEventW(NULL, TRUE, FALSE, NULL);
}

/***********************************************************************
 *           PowerSetRequest      (KERNEL32.@)
 */
BOOL WINAPI PowerSetRequest(HANDLE request, POWER_REQUEST_TYPE type)
{
    FIXME("(%p, %u): stub\n", request, type);

    return TRUE;
}

/***********************************************************************
 *           PowerClearRequest      (KERNEL32.@)
 */
BOOL WINAPI PowerClearRequest(HANDLE request, POWER_REQUEST_TYPE type)
{
    FIXME("(%p, %u): stub\n", request, type);

    return TRUE;
}
