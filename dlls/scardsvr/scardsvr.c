/*
 * Smart card service
 *
 * Copyright 2014 Nikolay Sivov for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(winscard);

static SERVICE_STATUS_HANDLE service_handle;
static HANDLE stop_event;

static DWORD WINAPI service_handler(DWORD ctrl, DWORD event_type, void *event_data, void *context)
{
    SERVICE_STATUS status;

    status.dwServiceType = SERVICE_WIN32;
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    status.dwWin32ExitCode = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint = 0;
    status.dwWaitHint = 0;

    switch(ctrl)
    {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            TRACE("Shutting down.\n");
            status.dwCurrentState = SERVICE_STOP_PENDING;
            status.dwControlsAccepted = 0;
            SetServiceStatus(service_handle, &status);
            SetEvent(stop_event);
            return ERROR_SUCCESS;

        default:
            FIXME("Got unknown control %#lx.\n", ctrl);
            status.dwCurrentState = SERVICE_RUNNING;
            SetServiceStatus(service_handle, &status);
            return ERROR_SUCCESS;
    }
}

void WINAPI CalaisMain(DWORD argc, WCHAR **argv)
{
    SERVICE_STATUS status;

    TRACE("Starting service.\n");

    stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);

    if (!(service_handle = RegisterServiceCtrlHandlerExW(L"scardsvr", service_handler, NULL)))
        return;

    status.dwServiceType = SERVICE_WIN32;
    status.dwCurrentState = SERVICE_RUNNING;
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    status.dwWin32ExitCode = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint = 0;
    status.dwWaitHint = 10000;
    SetServiceStatus(service_handle, &status);

    WaitForSingleObject(stop_event, INFINITE);

    status.dwCurrentState = SERVICE_STOPPED;
    status.dwControlsAccepted = 0;
    SetServiceStatus(service_handle, &status);
    TRACE("Service stopped.\n");
}
