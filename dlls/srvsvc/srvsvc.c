/*
 * LanmanServer Service
 *
 * Copyright 2022 Dmitry Timoshkov
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
#include "winsvc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(lanman);

static SERVICE_STATUS_HANDLE svc_handle;
static HANDLE done_event;

static void svc_update_status(DWORD state)
{
    SERVICE_STATUS status;

    status.dwServiceType = SERVICE_WIN32;
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    status.dwWin32ExitCode = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint = 0;
    status.dwWaitHint = 0;
    status.dwCurrentState = state;

    SetServiceStatus(svc_handle, &status);
}

static void WINAPI svc_handler(DWORD control)
{
    TRACE("%#lx\n", control);

    switch (control)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        svc_update_status(SERVICE_STOP_PENDING);
        SetEvent(done_event);
        break;

    default:
        svc_update_status(SERVICE_RUNNING);
        break;
    }
}

void WINAPI ServiceMain(DWORD argc, LPWSTR *argv)
{
    TRACE("Starting LanmanServer\n");

    svc_handle = RegisterServiceCtrlHandlerW(L"LanmanServer", svc_handler);
    if (!svc_handle)
    {
        ERR("RegisterServiceCtrlHandler error %ld\n", GetLastError());
        return;
    }

    svc_update_status(SERVICE_START_PENDING);
    done_event = CreateEventW(NULL, TRUE, FALSE, NULL);

    svc_update_status(SERVICE_RUNNING);
    WaitForSingleObject(done_event, INFINITE);
    CloseHandle(done_event);

    svc_update_status(SERVICE_STOPPED);

    TRACE("Exiting LanmanServer\n");
}
