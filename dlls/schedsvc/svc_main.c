/*
 * Task Scheduler Service
 *
 * Copyright 2014 Dmitry Timoshkov
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
#include "rpc.h"
#include "schrpc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(schedsvc);

static SERVICE_STATUS_HANDLE schedsvc_handle;
static HANDLE done_event;

static void schedsvc_update_status(DWORD state)
{
    SERVICE_STATUS status;

    status.dwServiceType = SERVICE_WIN32;
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    status.dwWin32ExitCode = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint = 0;
    status.dwWaitHint = 0;
    status.dwControlsAccepted = 0;
    status.dwCurrentState = state;

    SetServiceStatus(schedsvc_handle, &status);
}

static void WINAPI schedsvc_handler(DWORD control)
{
    WINE_TRACE("%#x\n", control);

    switch (control)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        schedsvc_update_status(SERVICE_STOP_PENDING);
        SetEvent(done_event);
        break;

    default:
        schedsvc_update_status(SERVICE_RUNNING);
        break;
    }
}

static RPC_BINDING_VECTOR *sched_bindings;

static RPC_STATUS RPC_init(void)
{
    WCHAR transport[] = SCHEDSVC_TRANSPORT;
    RPC_STATUS status;

    WINE_TRACE("using %s\n", wine_dbgstr_w(transport));

    status = RpcServerUseProtseqEpW(transport, 0, NULL, NULL);
    if (status != RPC_S_OK)
    {
        WINE_ERR("RpcServerUseProtseqEp error %#x\n", status);
        return status;
    }

    status = RpcServerRegisterIf(ITaskSchedulerService_v1_0_s_ifspec, 0, 0);
    if (status != RPC_S_OK)
    {
        WINE_ERR("RpcServerRegisterIf error %#x\n", status);
        return status;
    }

    status = RpcServerInqBindings(&sched_bindings);
    if (status != RPC_S_OK)
    {
        WINE_ERR("RpcServerInqBindings error %#x\n", status);
        return status;
    }

    status = RpcEpRegisterW(ITaskSchedulerService_v1_0_s_ifspec, sched_bindings, NULL, NULL);
    if (status != RPC_S_OK)
    {
        WINE_ERR("RpcEpRegister error %#x\n", status);
        return status;
    }

    status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);
    if (status != RPC_S_OK)
    {
        WINE_ERR("RpcServerListen error %#x\n", status);
        return status;
    }
    return RPC_S_OK;
}

static void RPC_finish(void)
{
    RpcMgmtStopServerListening(NULL);
    RpcEpUnregister(ITaskSchedulerService_v1_0_s_ifspec, sched_bindings, NULL);
    RpcBindingVectorFree(&sched_bindings);
    RpcServerUnregisterIf(NULL, NULL, FALSE);
}

void WINAPI ServiceMain(DWORD argc, LPWSTR *argv)
{
    static const WCHAR scheduleW[] = {'S','c','h','e','d','u','l','e',0};

    WINE_TRACE("starting Task Scheduler Service\n");

    if (RPC_init() != RPC_S_OK) return;

    schedsvc_handle = RegisterServiceCtrlHandlerW(scheduleW, schedsvc_handler);
    if (!schedsvc_handle)
    {
        WINE_ERR("RegisterServiceCtrlHandler error %d\n", GetLastError());
        return;
    }

    done_event = CreateEventW(NULL, TRUE, FALSE, NULL);

    schedsvc_update_status(SERVICE_RUNNING);

    WaitForSingleObject(done_event, INFINITE);

    RPC_finish();
    schedsvc_update_status(SERVICE_STOPPED);

    WINE_TRACE("exiting Task Scheduler Service\n");
}

void  __RPC_FAR * __RPC_USER MIDL_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}
