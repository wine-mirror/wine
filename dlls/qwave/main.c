/*
 * Copyright 2018 Louis Lenders
 * Copyright 2019 Vijay Kiran Kamuju
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
#include "qos2.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qwave);

BOOL WINAPI QOSCreateHandle(PQOS_VERSION version, PHANDLE handle)
{
    FIXME("%p %p stub!\n", version, handle);
    if (!version || !((version->MajorVersion == 1) && (version->MinorVersion == 0)) || !handle)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
        SetLastError(ERROR_SERVICE_ALREADY_RUNNING);
    return FALSE;
}

BOOL WINAPI QOSAddSocketToFlow(HANDLE handle, SOCKET socket, PSOCKADDR addr,
                               QOS_TRAFFIC_TYPE traffictype, DWORD flags, PQOS_FLOWID flowid)
{
    FIXME("%p, %Ix, %p, %d, 0x%08lx, %p stub!\n", handle, socket, addr, traffictype, flags, flowid);
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

BOOL WINAPI QOSCloseHandle(HANDLE handle)
{
    FIXME("%p stub!\n", handle);
    return FALSE;
}
