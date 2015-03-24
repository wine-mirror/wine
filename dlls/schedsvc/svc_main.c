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

static const WCHAR scheduleW[] = {'S','c','h','e','d','u','l','e',0};
static SERVICE_STATUS_HANDLE schedsvc_handle;
static HANDLE done_event;

void schedsvc_auto_start(void)
{
    static DWORD start_type;
    SC_HANDLE scm, service;
    QUERY_SERVICE_CONFIGW *cfg;
    DWORD cfg_size;

    if (start_type == SERVICE_AUTO_START) return;

    TRACE("changing service start type to SERVICE_AUTO_START\n");

    scm = OpenSCManagerW(NULL, NULL, 0);
    if (!scm)
    {
        WARN("failed to open SCM (%u)\n", GetLastError());
        return;
    }

    service = OpenServiceW(scm, scheduleW, SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG);
    if (service)
    {
        if (!QueryServiceConfigW(service, NULL, 0, &cfg_size) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            cfg = HeapAlloc(GetProcessHeap(), 0, cfg_size);
            if (cfg)
            {
                if (QueryServiceConfigW(service, cfg, cfg_size, &cfg_size))
                {
                    start_type = cfg->dwStartType;
                    if (start_type != SERVICE_AUTO_START)
                    {
                        if (ChangeServiceConfigW(service, SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE,
                                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL))
                            start_type = SERVICE_AUTO_START;
                    }
                }
                HeapFree(GetProcessHeap(), 0, cfg);
            }
        }
        else
            WARN("failed to query service config (%u)\n", GetLastError());

        CloseServiceHandle(service);
    }
    else
        WARN("failed to open service (%u)\n", GetLastError());

    CloseServiceHandle(scm);
}

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
    TRACE("%#x\n", control);

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

    TRACE("using %s\n", debugstr_w(transport));

    status = RpcServerUseProtseqEpW(transport, 0, NULL, NULL);
    if (status != RPC_S_OK)
    {
        ERR("RpcServerUseProtseqEp error %#x\n", status);
        return status;
    }

    status = RpcServerRegisterIf(ITaskSchedulerService_v1_0_s_ifspec, 0, 0);
    if (status != RPC_S_OK)
    {
        ERR("RpcServerRegisterIf error %#x\n", status);
        return status;
    }

    status = RpcServerInqBindings(&sched_bindings);
    if (status != RPC_S_OK)
    {
        ERR("RpcServerInqBindings error %#x\n", status);
        return status;
    }

    status = RpcEpRegisterW(ITaskSchedulerService_v1_0_s_ifspec, sched_bindings, NULL, NULL);
    if (status != RPC_S_OK)
    {
        ERR("RpcEpRegister error %#x\n", status);
        return status;
    }

    status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);
    if (status != RPC_S_OK)
    {
        ERR("RpcServerListen error %#x\n", status);
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
    TRACE("starting Task Scheduler Service\n");

    if (RPC_init() != RPC_S_OK) return;

    schedsvc_handle = RegisterServiceCtrlHandlerW(scheduleW, schedsvc_handler);
    if (!schedsvc_handle)
    {
        ERR("RegisterServiceCtrlHandler error %d\n", GetLastError());
        return;
    }

    done_event = CreateEventW(NULL, TRUE, FALSE, NULL);

    schedsvc_update_status(SERVICE_RUNNING);

    WaitForSingleObject(done_event, INFINITE);

    RPC_finish();
    schedsvc_update_status(SERVICE_STOPPED);

    TRACE("exiting Task Scheduler Service\n");
}

void  __RPC_FAR * __RPC_USER MIDL_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}
