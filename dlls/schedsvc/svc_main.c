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
#include "atsvc.h"
#include "schrpc.h"
#include "wine/debug.h"

#include "schedsvc_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(schedsvc);

static const WCHAR scheduleW[] = {'S','c','h','e','d','u','l','e',0};
static SERVICE_STATUS_HANDLE schedsvc_handle;
static HANDLE done_event, hjob_queue;

void add_process_to_queue(HANDLE process)
{
    if (!AssignProcessToJobObject(hjob_queue, process))
        ERR("AssignProcessToJobObject failed\n");
}

static DWORD WINAPI tasks_monitor_thread(void *arg)
{
    static const WCHAR tasksW[] = { '\\','T','a','s','k','s','\\',0 };
    WCHAR path[MAX_PATH];
    HANDLE htasks, hport, htimer;
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT job_info;
    OVERLAPPED ov;
    LARGE_INTEGER period;
    struct
    {
        FILE_NOTIFY_INFORMATION data;
        WCHAR name_buffer[MAX_PATH];
    } info;

    /* the buffer must be DWORD aligned */
    C_ASSERT(!(sizeof(info) & 3));

    TRACE("Starting...\n");

    load_at_tasks();
    check_missed_task_time();

    htimer = CreateWaitableTimerW(NULL, FALSE, NULL);
    if (htimer == NULL)
    {
        ERR("CreateWaitableTimer failed\n");
        return -1;
    }

    GetWindowsDirectoryW(path, MAX_PATH);
    lstrcatW(path, tasksW);

    /* Just in case it's an old Wine prefix with missing c:\windows\tasks */
    CreateDirectoryW(path, NULL);

    htasks = CreateFileW(path, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
    if (htasks == INVALID_HANDLE_VALUE)
    {
        ERR("Couldn't start monitoring %s for tasks, error %lu\n", debugstr_w(path), GetLastError());
        return -1;
    }

    hjob_queue = CreateJobObjectW(NULL, NULL);
    if (!hjob_queue)
    {
        ERR("CreateJobObject failed\n");
        return -1;
    }

    hport = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    if (!hport)
    {
        ERR("CreateIoCompletionPort failed\n");
        return -1;
    }

    job_info.CompletionKey = hjob_queue;
    job_info.CompletionPort = hport;
    if (!SetInformationJobObject(hjob_queue, JobObjectAssociateCompletionPortInformation, &job_info, sizeof(job_info)))
    {
        ERR("SetInformationJobObject failed\n");
        return -1;
    }

    memset(&ov, 0, sizeof(ov));
    ov.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    memset(&info, 0, sizeof(info));
    ReadDirectoryChangesW(htasks, &info, sizeof(info), FALSE,
                          FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE,
                          NULL, &ov, NULL);

    for (;;)
    {
        HANDLE events[4];
        DWORD ret;

        events[0] = done_event;
        events[1] = htimer;
        events[2] = hport;
        events[3] = ov.hEvent;

        ret = WaitForMultipleObjects(4, events, FALSE, INFINITE);
        /* Done event */
        if (ret == WAIT_OBJECT_0) break;

        /* Next runtime timer */
        if (ret == WAIT_OBJECT_0 + 1)
        {
            check_task_time();
            continue;
        }

        /* Job queue */
        if (ret == WAIT_OBJECT_0 + 2)
        {
            DWORD msg;
            ULONG_PTR dummy, pid;

            if (GetQueuedCompletionStatus(hport, &msg, &dummy, (OVERLAPPED **)&pid, 0))
            {
                if (msg == JOB_OBJECT_MSG_EXIT_PROCESS)
                {
                    TRACE("got message: process %#Ix has terminated\n", pid);
                    update_process_status(pid);
                }
                else
                    FIXME("got message %#lx from the job\n", msg);
            }
            continue;
        }

        if (info.data.NextEntryOffset)
            FIXME("got multiple entries\n");

        /* Directory change notification */
        info.data.FileName[info.data.FileNameLength/sizeof(WCHAR)] = 0;

        switch (info.data.Action)
        {
        case FILE_ACTION_ADDED:
            TRACE("FILE_ACTION_ADDED %s\n", debugstr_w(info.data.FileName));

            GetWindowsDirectoryW(path, MAX_PATH);
            lstrcatW(path, tasksW);
            lstrcatW(path, info.data.FileName);
            add_job(path);
            break;

        case FILE_ACTION_REMOVED:
            TRACE("FILE_ACTION_REMOVED %s\n", debugstr_w(info.data.FileName));
            GetWindowsDirectoryW(path, MAX_PATH);
            lstrcatW(path, tasksW);
            lstrcatW(path, info.data.FileName);
            remove_job(path);
            break;

        case FILE_ACTION_MODIFIED:
            TRACE("FILE_ACTION_MODIFIED %s\n", debugstr_w(info.data.FileName));

            GetWindowsDirectoryW(path, MAX_PATH);
            lstrcatW(path, tasksW);
            lstrcatW(path, info.data.FileName);
            remove_job(path);
            add_job(path);
            break;

        default:
            FIXME("%s: action %#lx not handled\n", debugstr_w(info.data.FileName), info.data.Action);
            break;
        }

        check_task_state();

        if (get_next_runtime(&period))
        {
            if (!SetWaitableTimer(htimer, &period, 0, NULL, NULL, FALSE))
                ERR("SetWaitableTimer failed\n");
        }

        memset(&info, 0, sizeof(info));
        if (!ReadDirectoryChangesW(htasks, &info, sizeof(info), FALSE,
                                   FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE,
                                   NULL, &ov, NULL)) break;
    }

    CancelWaitableTimer(htimer);
    CloseHandle(htimer);
    CloseHandle(ov.hEvent);
    CloseHandle(hport);
    CloseHandle(hjob_queue);
    CloseHandle(htasks);

    TRACE("Finished.\n");

    return 0;
}

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
        WARN("failed to open SCM (%lu)\n", GetLastError());
        return;
    }

    service = OpenServiceW(scm, scheduleW, SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG);
    if (service)
    {
        if (!QueryServiceConfigW(service, NULL, 0, &cfg_size) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            cfg = malloc(cfg_size);
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
                free(cfg);
            }
        }
        else
            WARN("failed to query service config (%lu)\n", GetLastError());

        CloseServiceHandle(service);
    }
    else
        WARN("failed to open service (%lu)\n", GetLastError());

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
    status.dwCurrentState = state;

    SetServiceStatus(schedsvc_handle, &status);
}

static void WINAPI schedsvc_handler(DWORD control)
{
    TRACE("%#lx\n", control);

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
    static WCHAR ncacn_npW[] = { 'n','c','a','c','n','_','n','p',0 };
    static WCHAR endpoint_npW[] = { '\\','p','i','p','e','\\','a','t','s','v','c',0 };
    static WCHAR ncalrpcW[] = { 'n','c','a','l','r','p','c',0 };
    static WCHAR endpoint_lrpcW[] = { 'a','t','s','v','c',0 };
    RPC_STATUS status;

    status = RpcServerRegisterIf(ITaskSchedulerService_v1_0_s_ifspec, NULL, NULL);
    if (status != RPC_S_OK)
    {
        ERR("RpcServerRegisterIf error %#lx\n", status);
        return status;
    }

    status = RpcServerRegisterIf(atsvc_v1_0_s_ifspec, NULL, NULL);
    if (status != RPC_S_OK)
    {
        ERR("RpcServerRegisterIf error %#lx\n", status);
        RpcServerUnregisterIf(ITaskSchedulerService_v1_0_s_ifspec, NULL, FALSE);
        return status;
    }

    status = RpcServerUseProtseqEpW(ncacn_npW, RPC_C_PROTSEQ_MAX_REQS_DEFAULT, endpoint_npW, NULL);
    if (status != RPC_S_OK)
    {
        ERR("RpcServerUseProtseqEp error %#lx\n", status);
        return status;
    }

    status = RpcServerUseProtseqEpW(ncalrpcW, RPC_C_PROTSEQ_MAX_REQS_DEFAULT, endpoint_lrpcW, NULL);
    if (status != RPC_S_OK)
    {
        ERR("RpcServerUseProtseqEp error %#lx\n", status);
        return status;
    }

    status = RpcServerInqBindings(&sched_bindings);
    if (status != RPC_S_OK)
    {
        ERR("RpcServerInqBindings error %#lx\n", status);
        return status;
    }

    status = RpcEpRegisterW(ITaskSchedulerService_v1_0_s_ifspec, sched_bindings, NULL, NULL);
    if (status != RPC_S_OK)
    {
        ERR("RpcEpRegister error %#lx\n", status);
        return status;
    }

    status = RpcEpRegisterW(atsvc_v1_0_s_ifspec, sched_bindings, NULL, NULL);
    if (status != RPC_S_OK)
    {
        ERR("RpcEpRegister error %#lx\n", status);
        return status;
    }

    status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);
    if (status != RPC_S_OK)
    {
        ERR("RpcServerListen error %#lx\n", status);
        return status;
    }
    return RPC_S_OK;
}

static void RPC_finish(void)
{
    RpcMgmtStopServerListening(NULL);
    RpcEpUnregister(ITaskSchedulerService_v1_0_s_ifspec, sched_bindings, NULL);
    RpcEpUnregister(atsvc_v1_0_s_ifspec, sched_bindings, NULL);
    RpcBindingVectorFree(&sched_bindings);
    RpcServerUnregisterIf(ITaskSchedulerService_v1_0_s_ifspec, NULL, FALSE);
    RpcServerUnregisterIf(atsvc_v1_0_s_ifspec, NULL, FALSE);
}

void WINAPI ServiceMain(DWORD argc, LPWSTR *argv)
{
    HANDLE thread;
    DWORD tid;

    TRACE("starting Task Scheduler Service\n");

    schedsvc_handle = RegisterServiceCtrlHandlerW(scheduleW, schedsvc_handler);
    if (!schedsvc_handle)
    {
        ERR("RegisterServiceCtrlHandler error %ld\n", GetLastError());
        return;
    }

    schedsvc_update_status(SERVICE_START_PENDING);

    done_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    thread = CreateThread(NULL, 0, tasks_monitor_thread, NULL, 0, &tid);

    if (thread && RPC_init() == RPC_S_OK)
    {
        schedsvc_update_status(SERVICE_RUNNING);
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
        RPC_finish();
    }

    schedsvc_update_status(SERVICE_STOPPED);

    TRACE("exiting Task Scheduler Service\n");
}

void  __RPC_FAR * __RPC_USER MIDL_user_allocate(SIZE_T len)
{
    return malloc(len);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR * ptr)
{
    free(ptr);
}
