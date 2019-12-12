/*
 * NT process handling
 *
 * Copyright 1996-1998 Marcus Meissner
 * Copyright 2018 Alexandre Julliard
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

#include "config.h"
#include "wine/port.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <pthread.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/debug.h"
#include "windef.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/exception.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/unicode.h"

#ifdef HAVE_MACH_MACH_H
#include <mach/mach.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(process);

static ULONG execute_flags = MEM_EXECUTE_OPTION_DISABLE | (sizeof(void *) > sizeof(int) ?
                                                           MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION |
                                                           MEM_EXECUTE_OPTION_PERMANENT : 0);

static const BOOL is_win64 = (sizeof(void *) > sizeof(int));

static const char * const cpu_names[] = { "x86", "x86_64", "PowerPC", "ARM", "ARM64" };

static inline BOOL is_64bit_arch( client_cpu_t cpu )
{
    return (cpu == CPU_x86_64 || cpu == CPU_ARM64);
}

/*
 *	Process object
 */

/******************************************************************************
 *  NtTerminateProcess			[NTDLL.@]
 *
 *  Native applications must kill themselves when done
 */
NTSTATUS WINAPI NtTerminateProcess( HANDLE handle, LONG exit_code )
{
    NTSTATUS ret;
    BOOL self;
    SERVER_START_REQ( terminate_process )
    {
        req->handle    = wine_server_obj_handle( handle );
        req->exit_code = exit_code;
        ret = wine_server_call( req );
        self = !ret && reply->self;
    }
    SERVER_END_REQ;
    if (self && handle) _exit( get_unix_exit_code( exit_code ));
    return ret;
}

/******************************************************************************
 *  RtlGetCurrentPeb  [NTDLL.@]
 *
 */
PEB * WINAPI RtlGetCurrentPeb(void)
{
    return NtCurrentTeb()->Peb;
}

/***********************************************************************
 *           __wine_make_process_system   (NTDLL.@)
 *
 * Mark the current process as a system process.
 * Returns the event that is signaled when all non-system processes have exited.
 */
HANDLE CDECL __wine_make_process_system(void)
{
    HANDLE ret = 0;
    SERVER_START_REQ( make_process_system )
    {
        if (!wine_server_call( req )) ret = wine_server_ptr_handle( reply->event );
    }
    SERVER_END_REQ;
    return ret;
}

static UINT process_error_mode;

#define UNIMPLEMENTED_INFO_CLASS(c) \
    case c: \
        FIXME("(process=%p) Unimplemented information class: " #c "\n", ProcessHandle); \
        ret = STATUS_INVALID_INFO_CLASS; \
        break

ULONG_PTR get_system_affinity_mask(void)
{
    ULONG num_cpus = NtCurrentTeb()->Peb->NumberOfProcessors;
    if (num_cpus >= sizeof(ULONG_PTR) * 8) return ~(ULONG_PTR)0;
    return ((ULONG_PTR)1 << num_cpus) - 1;
}

#if defined(HAVE_MACH_MACH_H)

static void fill_VM_COUNTERS(VM_COUNTERS* pvmi)
{
#if defined(MACH_TASK_BASIC_INFO)
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if(task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) == KERN_SUCCESS)
    {
        pvmi->VirtualSize = info.resident_size + info.virtual_size;
        pvmi->PagefileUsage = info.virtual_size;
        pvmi->WorkingSetSize = info.resident_size;
        pvmi->PeakWorkingSetSize = info.resident_size_max;
    }
#endif
}

#elif defined(linux)

static void fill_VM_COUNTERS(VM_COUNTERS* pvmi)
{
    FILE *f;
    char line[256];
    unsigned long value;

    f = fopen("/proc/self/status", "r");
    if (!f) return;

    while (fgets(line, sizeof(line), f))
    {
        if (sscanf(line, "VmPeak: %lu", &value))
            pvmi->PeakVirtualSize = (ULONG64)value * 1024;
        else if (sscanf(line, "VmSize: %lu", &value))
            pvmi->VirtualSize = (ULONG64)value * 1024;
        else if (sscanf(line, "VmHWM: %lu", &value))
            pvmi->PeakWorkingSetSize = (ULONG64)value * 1024;
        else if (sscanf(line, "VmRSS: %lu", &value))
            pvmi->WorkingSetSize = (ULONG64)value * 1024;
        else if (sscanf(line, "RssAnon: %lu", &value))
            pvmi->PagefileUsage += (ULONG64)value * 1024;
        else if (sscanf(line, "VmSwap: %lu", &value))
            pvmi->PagefileUsage += (ULONG64)value * 1024;
    }
    pvmi->PeakPagefileUsage = pvmi->PagefileUsage;

    fclose(f);
}

#else

static void fill_VM_COUNTERS(VM_COUNTERS* pvmi)
{
    /* FIXME : real data */
}

#endif

/******************************************************************************
*  NtQueryInformationProcess		[NTDLL.@]
*  ZwQueryInformationProcess		[NTDLL.@]
*
*/
NTSTATUS WINAPI NtQueryInformationProcess(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength)
{
    NTSTATUS ret = STATUS_SUCCESS;
    ULONG len = 0;

    TRACE("(%p,0x%08x,%p,0x%08x,%p)\n",
          ProcessHandle,ProcessInformationClass,
          ProcessInformation,ProcessInformationLength,
          ReturnLength);

    switch (ProcessInformationClass) 
    {
    UNIMPLEMENTED_INFO_CLASS(ProcessQuotaLimits);
    UNIMPLEMENTED_INFO_CLASS(ProcessBasePriority);
    UNIMPLEMENTED_INFO_CLASS(ProcessRaisePriority);
    UNIMPLEMENTED_INFO_CLASS(ProcessExceptionPort);
    UNIMPLEMENTED_INFO_CLASS(ProcessAccessToken);
    UNIMPLEMENTED_INFO_CLASS(ProcessLdtInformation);
    UNIMPLEMENTED_INFO_CLASS(ProcessLdtSize);
    UNIMPLEMENTED_INFO_CLASS(ProcessIoPortHandlers);
    UNIMPLEMENTED_INFO_CLASS(ProcessPooledUsageAndLimits);
    UNIMPLEMENTED_INFO_CLASS(ProcessWorkingSetWatch);
    UNIMPLEMENTED_INFO_CLASS(ProcessUserModeIOPL);
    UNIMPLEMENTED_INFO_CLASS(ProcessEnableAlignmentFaultFixup);
    UNIMPLEMENTED_INFO_CLASS(ProcessWx86Information);
    UNIMPLEMENTED_INFO_CLASS(ProcessPriorityBoost);
    UNIMPLEMENTED_INFO_CLASS(ProcessDeviceMap);
    UNIMPLEMENTED_INFO_CLASS(ProcessSessionInformation);
    UNIMPLEMENTED_INFO_CLASS(ProcessForegroundInformation);
    UNIMPLEMENTED_INFO_CLASS(ProcessLUIDDeviceMapsEnabled);
    UNIMPLEMENTED_INFO_CLASS(ProcessBreakOnTermination);
    UNIMPLEMENTED_INFO_CLASS(ProcessHandleTracing);

    case ProcessBasicInformation:
        {
            PROCESS_BASIC_INFORMATION pbi;
            const ULONG_PTR affinity_mask = get_system_affinity_mask();

            if (ProcessInformationLength >= sizeof(PROCESS_BASIC_INFORMATION))
            {
                if (!ProcessInformation)
                    ret = STATUS_ACCESS_VIOLATION;
                else if (!ProcessHandle)
                    ret = STATUS_INVALID_HANDLE;
                else
                {
                    SERVER_START_REQ(get_process_info)
                    {
                        req->handle = wine_server_obj_handle( ProcessHandle );
                        if ((ret = wine_server_call( req )) == STATUS_SUCCESS)
                        {
                            pbi.ExitStatus = reply->exit_code;
                            pbi.PebBaseAddress = wine_server_get_ptr( reply->peb );
                            pbi.AffinityMask = reply->affinity & affinity_mask;
                            pbi.BasePriority = reply->priority;
                            pbi.UniqueProcessId = reply->pid;
                            pbi.InheritedFromUniqueProcessId = reply->ppid;
                        }
                    }
                    SERVER_END_REQ;

                    memcpy(ProcessInformation, &pbi, sizeof(PROCESS_BASIC_INFORMATION));

                    len = sizeof(PROCESS_BASIC_INFORMATION);
                }

                if (ProcessInformationLength > sizeof(PROCESS_BASIC_INFORMATION))
                    ret = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                len = sizeof(PROCESS_BASIC_INFORMATION);
                ret = STATUS_INFO_LENGTH_MISMATCH;
            }
        }
        break;
    case ProcessIoCounters:
        {
            IO_COUNTERS pii;

            if (ProcessInformationLength >= sizeof(IO_COUNTERS))
            {
                if (!ProcessInformation)
                    ret = STATUS_ACCESS_VIOLATION;
                else if (!ProcessHandle)
                    ret = STATUS_INVALID_HANDLE;
                else
                {
                    /* FIXME : real data */
                    memset(&pii, 0 , sizeof(IO_COUNTERS));

                    memcpy(ProcessInformation, &pii, sizeof(IO_COUNTERS));

                    len = sizeof(IO_COUNTERS);
                }

                if (ProcessInformationLength > sizeof(IO_COUNTERS))
                    ret = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                len = sizeof(IO_COUNTERS);
                ret = STATUS_INFO_LENGTH_MISMATCH;
            }
        }
        break;
    case ProcessVmCounters:
        {
            VM_COUNTERS pvmi;

            /* older Windows versions don't have the PrivatePageCount field */
            if (ProcessInformationLength >= FIELD_OFFSET(VM_COUNTERS,PrivatePageCount))
            {
                if (!ProcessInformation)
                    ret = STATUS_ACCESS_VIOLATION;
                else
                {
                    memset(&pvmi, 0 , sizeof(VM_COUNTERS));
                    if (ProcessHandle == GetCurrentProcess())
                        fill_VM_COUNTERS(&pvmi);
                    else
                    {
                        SERVER_START_REQ(get_process_vm_counters)
                        {
                            req->handle = wine_server_obj_handle( ProcessHandle );
                            if (!(ret = wine_server_call( req )))
                            {
                                pvmi.PeakVirtualSize = reply->peak_virtual_size;
                                pvmi.VirtualSize = reply->virtual_size;
                                pvmi.PeakWorkingSetSize = reply->peak_working_set_size;
                                pvmi.WorkingSetSize = reply->working_set_size;
                                pvmi.PagefileUsage = reply->pagefile_usage;
                                pvmi.PeakPagefileUsage = reply->peak_pagefile_usage;
                            }
                        }
                        SERVER_END_REQ;
                        if (ret) break;
                    }

                    len = ProcessInformationLength;
                    if (len != FIELD_OFFSET(VM_COUNTERS,PrivatePageCount)) len = sizeof(VM_COUNTERS);

                    memcpy(ProcessInformation, &pvmi, min(ProcessInformationLength,sizeof(VM_COUNTERS)));
                }

                if (ProcessInformationLength != FIELD_OFFSET(VM_COUNTERS,PrivatePageCount) &&
                    ProcessInformationLength != sizeof(VM_COUNTERS))
                    ret = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                len = sizeof(pvmi);
                ret = STATUS_INFO_LENGTH_MISMATCH;
            }
        }
        break;
    case ProcessTimes:
        {
            KERNEL_USER_TIMES pti;

            if (ProcessInformationLength >= sizeof(KERNEL_USER_TIMES))
            {
                if (!ProcessInformation)
                    ret = STATUS_ACCESS_VIOLATION;
                else if (!ProcessHandle)
                    ret = STATUS_INVALID_HANDLE;
                else
                {
                    /* FIXME : User- and KernelTime have to be implemented */
                    memset(&pti, 0, sizeof(KERNEL_USER_TIMES));

                    SERVER_START_REQ(get_process_info)
                    {
                      req->handle = wine_server_obj_handle( ProcessHandle );
                      if ((ret = wine_server_call( req )) == STATUS_SUCCESS)
                      {
                          pti.CreateTime.QuadPart = reply->start_time;
                          pti.ExitTime.QuadPart = reply->end_time;
                      }
                    }
                    SERVER_END_REQ;

                    memcpy(ProcessInformation, &pti, sizeof(KERNEL_USER_TIMES));
                    len = sizeof(KERNEL_USER_TIMES);
                }

                if (ProcessInformationLength > sizeof(KERNEL_USER_TIMES))
                    ret = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                len = sizeof(KERNEL_USER_TIMES);
                ret = STATUS_INFO_LENGTH_MISMATCH;
            }
        }
        break;
    case ProcessDebugPort:
        len = sizeof(DWORD_PTR);
        if (ProcessInformationLength == len)
        {
            if (!ProcessInformation)
                ret = STATUS_ACCESS_VIOLATION;
            else if (!ProcessHandle)
                ret = STATUS_INVALID_HANDLE;
            else
            {
                SERVER_START_REQ(get_process_info)
                {
                    req->handle = wine_server_obj_handle( ProcessHandle );
                    if ((ret = wine_server_call( req )) == STATUS_SUCCESS)
                    {
                        *(DWORD_PTR *)ProcessInformation = reply->debugger_present ? ~(DWORD_PTR)0 : 0;
                    }
                }
                SERVER_END_REQ;
            }
        }
        else
            ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    case ProcessDebugFlags:
        len = sizeof(DWORD);
        if (ProcessInformationLength == len)
        {
            if (!ProcessInformation)
                ret = STATUS_ACCESS_VIOLATION;
            else if (!ProcessHandle)
                ret = STATUS_INVALID_HANDLE;
            else
            {
                SERVER_START_REQ(get_process_info)
                {
                    req->handle = wine_server_obj_handle( ProcessHandle );
                    if ((ret = wine_server_call( req )) == STATUS_SUCCESS)
                    {
                        *(DWORD *)ProcessInformation = reply->debug_children;
                    }
                }
                SERVER_END_REQ;
            }
        }
        else
            ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    case ProcessDefaultHardErrorMode:
        len = sizeof(process_error_mode);
        if (ProcessInformationLength == len)
            memcpy(ProcessInformation, &process_error_mode, len);
        else
            ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    case ProcessDebugObjectHandle:
        /* "These are not the debuggers you are looking for." *
         * set it to 0 aka "no debugger" to satisfy copy protections */
        len = sizeof(HANDLE);
        if (ProcessInformationLength == len)
        {
            if (!ProcessInformation)
                ret = STATUS_ACCESS_VIOLATION;
            else if (!ProcessHandle)
                ret = STATUS_INVALID_HANDLE;
            else
            {
                memset(ProcessInformation, 0, ProcessInformationLength);
                ret = STATUS_PORT_NOT_SET;
            }
        }
        else
            ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    case ProcessHandleCount:
        if (ProcessInformationLength >= 4)
        {
            if (!ProcessInformation)
                ret = STATUS_ACCESS_VIOLATION;
            else if (!ProcessHandle)
                ret = STATUS_INVALID_HANDLE;
            else
            {
                memset(ProcessInformation, 0, 4);
                len = 4;
            }

            if (ProcessInformationLength > 4)
                ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        else
        {
            len = 4;
            ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;

    case ProcessAffinityMask:
        len = sizeof(ULONG_PTR);
        if (ProcessInformationLength == len)
        {
            const ULONG_PTR system_mask = get_system_affinity_mask();

            SERVER_START_REQ(get_process_info)
            {
                req->handle = wine_server_obj_handle( ProcessHandle );
                if (!(ret = wine_server_call( req )))
                    *(ULONG_PTR *)ProcessInformation = reply->affinity & system_mask;
            }
            SERVER_END_REQ;
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;

    case ProcessWow64Information:
        len = sizeof(ULONG_PTR);
        if (ProcessInformationLength != len) ret = STATUS_INFO_LENGTH_MISMATCH;
        else if (!ProcessInformation) ret = STATUS_ACCESS_VIOLATION;
        else if(!ProcessHandle) ret = STATUS_INVALID_HANDLE;
        else
        {
            ULONG_PTR val = 0;

            if (ProcessHandle == GetCurrentProcess()) val = is_wow64;
            else if (server_cpus & ((1 << CPU_x86_64) | (1 << CPU_ARM64)))
            {
                SERVER_START_REQ( get_process_info )
                {
                    req->handle = wine_server_obj_handle( ProcessHandle );
                    if (!(ret = wine_server_call( req )))
                        val = (reply->cpu != CPU_x86_64 && reply->cpu != CPU_ARM64);
                }
                SERVER_END_REQ;
            }
            *(ULONG_PTR *)ProcessInformation = val;
        }
        break;
    case ProcessImageFileName:
        /* FIXME: Should return a device path */
    case ProcessImageFileNameWin32:
        SERVER_START_REQ(get_dll_info)
        {
            UNICODE_STRING *image_file_name_str = ProcessInformation;

            req->handle = wine_server_obj_handle( ProcessHandle );
            req->base_address = 0; /* main module */
            wine_server_set_reply( req, image_file_name_str ? image_file_name_str + 1 : NULL,
                                   ProcessInformationLength > sizeof(UNICODE_STRING) ? ProcessInformationLength - sizeof(UNICODE_STRING) : 0 );
            ret = wine_server_call( req );
            if (ret == STATUS_BUFFER_TOO_SMALL) ret = STATUS_INFO_LENGTH_MISMATCH;

            len = sizeof(UNICODE_STRING) + reply->filename_len;
            if (ret == STATUS_SUCCESS)
            {
                image_file_name_str->MaximumLength = image_file_name_str->Length = reply->filename_len;
                image_file_name_str->Buffer = (PWSTR)(image_file_name_str + 1);
            }
        }
        SERVER_END_REQ;
        break;
    case ProcessExecuteFlags:
        len = sizeof(ULONG);
        if (ProcessInformationLength == len)
            *(ULONG *)ProcessInformation = execute_flags;
        else
            ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    case ProcessPriorityClass:
        len = sizeof(PROCESS_PRIORITY_CLASS);
        if (ProcessInformationLength == len)
        {
            if (!ProcessInformation)
                ret = STATUS_ACCESS_VIOLATION;
            else if (!ProcessHandle)
                ret = STATUS_INVALID_HANDLE;
            else
            {
                PROCESS_PRIORITY_CLASS *priority = ProcessInformation;

                SERVER_START_REQ(get_process_info)
                {
                    req->handle = wine_server_obj_handle( ProcessHandle );
                    if ((ret = wine_server_call( req )) == STATUS_SUCCESS)
                    {
                        priority->PriorityClass = reply->priority;
                        /* FIXME: Not yet supported by the wineserver */
                        priority->Foreground = FALSE;
                    }
                }
                SERVER_END_REQ;
            }
        }
        else
            ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    case ProcessCookie:
        FIXME("ProcessCookie (%p,%p,0x%08x,%p) stub\n",
              ProcessHandle,ProcessInformation,
              ProcessInformationLength,ReturnLength);

        if(ProcessHandle == NtCurrentProcess())
        {
            len = sizeof(ULONG);
            if (ProcessInformationLength == len)
                *(ULONG *)ProcessInformation = 0;
            else
                ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        else
            ret = STATUS_INVALID_PARAMETER;
        break;
    default:
        FIXME("(%p,info_class=%d,%p,0x%08x,%p) Unknown information class\n",
              ProcessHandle,ProcessInformationClass,
              ProcessInformation,ProcessInformationLength,
              ReturnLength);
        ret = STATUS_INVALID_INFO_CLASS;
        break;
    }

    if (ReturnLength) *ReturnLength = len;
    
    return ret;
}

/******************************************************************************
 * NtSetInformationProcess [NTDLL.@]
 * ZwSetInformationProcess [NTDLL.@]
 */
NTSTATUS WINAPI NtSetInformationProcess(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	IN PVOID ProcessInformation,
	IN ULONG ProcessInformationLength)
{
    NTSTATUS ret = STATUS_SUCCESS;

    switch (ProcessInformationClass)
    {
    case ProcessDefaultHardErrorMode:
        if (ProcessInformationLength != sizeof(UINT)) return STATUS_INVALID_PARAMETER;
        process_error_mode = *(UINT *)ProcessInformation;
        break;
    case ProcessAffinityMask:
    {
        const ULONG_PTR system_mask = get_system_affinity_mask();

        if (ProcessInformationLength != sizeof(DWORD_PTR)) return STATUS_INVALID_PARAMETER;
        if (*(PDWORD_PTR)ProcessInformation & ~system_mask)
            return STATUS_INVALID_PARAMETER;
        if (!*(PDWORD_PTR)ProcessInformation)
            return STATUS_INVALID_PARAMETER;
        SERVER_START_REQ( set_process_info )
        {
            req->handle   = wine_server_obj_handle( ProcessHandle );
            req->affinity = *(PDWORD_PTR)ProcessInformation;
            req->mask     = SET_PROCESS_INFO_AFFINITY;
            ret = wine_server_call( req );
        }
        SERVER_END_REQ;
        break;
    }
    case ProcessPriorityClass:
        if (ProcessInformationLength != sizeof(PROCESS_PRIORITY_CLASS))
            return STATUS_INVALID_PARAMETER;
        else
        {
            PROCESS_PRIORITY_CLASS* ppc = ProcessInformation;

            SERVER_START_REQ( set_process_info )
            {
                req->handle   = wine_server_obj_handle( ProcessHandle );
                /* FIXME Foreground isn't used */
                req->priority = ppc->PriorityClass;
                req->mask     = SET_PROCESS_INFO_PRIORITY;
                ret = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        break;

    case ProcessExecuteFlags:
        if (is_win64 || ProcessInformationLength != sizeof(ULONG))
            return STATUS_INVALID_PARAMETER;
        else if (execute_flags & MEM_EXECUTE_OPTION_PERMANENT)
            return STATUS_ACCESS_DENIED;
        else
        {
            BOOL enable;
            switch (*(ULONG *)ProcessInformation & (MEM_EXECUTE_OPTION_ENABLE|MEM_EXECUTE_OPTION_DISABLE))
            {
            case MEM_EXECUTE_OPTION_ENABLE:
                enable = TRUE;
                break;
            case MEM_EXECUTE_OPTION_DISABLE:
                enable = FALSE;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
            }
            execute_flags = *(ULONG *)ProcessInformation;
            VIRTUAL_SetForceExec( enable );
        }
        break;

    default:
        FIXME("(%p,0x%08x,%p,0x%08x) stub\n",
              ProcessHandle,ProcessInformationClass,ProcessInformation,
              ProcessInformationLength);
        ret = STATUS_NOT_IMPLEMENTED;
        break;
    }
    return ret;
}

/******************************************************************************
 * NtFlushInstructionCache [NTDLL.@]
 * ZwFlushInstructionCache [NTDLL.@]
 */
NTSTATUS WINAPI NtFlushInstructionCache( HANDLE handle, const void *addr, SIZE_T size )
{
#if defined(__x86_64__) || defined(__i386__)
    /* no-op */
#elif defined(HAVE___CLEAR_CACHE)
    if (handle == GetCurrentProcess())
    {
        __clear_cache( (char *)addr, (char *)addr + size );
    }
    else
    {
        static int once;
        if (!once++) FIXME( "%p %p %ld other process not supported\n", handle, addr, size );
    }
#else
    static int once;
    if (!once++) FIXME( "%p %p %ld\n", handle, addr, size );
#endif
    return STATUS_SUCCESS;
}

/******************************************************************
 *		NtOpenProcess [NTDLL.@]
 *		ZwOpenProcess [NTDLL.@]
 */
NTSTATUS  WINAPI NtOpenProcess(PHANDLE handle, ACCESS_MASK access,
                               const OBJECT_ATTRIBUTES* attr, const CLIENT_ID* cid)
{
    NTSTATUS    status;

    SERVER_START_REQ( open_process )
    {
        req->pid        = HandleToULong(cid->UniqueProcess);
        req->access     = access;
        req->attributes = attr ? attr->Attributes : 0;
        status = wine_server_call( req );
        if (!status) *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return status;
}

/******************************************************************************
 * NtResumeProcess
 * ZwResumeProcess
 */
NTSTATUS WINAPI NtResumeProcess( HANDLE handle )
{
    NTSTATUS ret;

    SERVER_START_REQ( resume_process )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;

    return ret;
}

/******************************************************************************
 * NtSuspendProcess
 * ZwSuspendProcess
 */
NTSTATUS WINAPI NtSuspendProcess( HANDLE handle )
{
    NTSTATUS ret;

    SERVER_START_REQ( suspend_process )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;

    return ret;
}


/***********************************************************************
 *           build_argv
 *
 * Build an argv array from a command-line.
 * 'reserved' is the number of args to reserve before the first one.
 */
static char **build_argv( const UNICODE_STRING *cmdlineW, int reserved )
{
    int argc;
    char **argv;
    char *arg, *s, *d, *cmdline;
    int in_quotes, bcount, len;

    len = ntdll_wcstoumbs( 0, cmdlineW->Buffer, cmdlineW->Length / sizeof(WCHAR), NULL, 0, NULL, NULL );
    if (!(cmdline = RtlAllocateHeap( GetProcessHeap(), 0, len + 1 ))) return NULL;
    ntdll_wcstoumbs( 0, cmdlineW->Buffer, cmdlineW->Length / sizeof(WCHAR), cmdline, len, NULL, NULL );
    cmdline[len++] = 0;

    argc = reserved + 1;
    bcount = 0;
    in_quotes = 0;
    s = cmdline;
    while (1)
    {
        if (*s == '\0' || ((*s == ' ' || *s == '\t') && !in_quotes))
        {
            /* space */
            argc++;
            /* skip the remaining spaces */
            while (*s == ' ' || *s == '\t') s++;
            if (*s == '\0') break;
            bcount = 0;
            continue;
        }
        else if (*s == '\\') bcount++;  /* '\', count them */
        else if ((*s == '"') && ((bcount & 1) == 0))
        {
            if (in_quotes && s[1] == '"') s++;
            else
            {
                /* unescaped '"' */
                in_quotes = !in_quotes;
                bcount = 0;
            }
        }
        else bcount = 0; /* a regular character */
        s++;
    }
    if (!(argv = RtlAllocateHeap( GetProcessHeap(), 0, argc * sizeof(*argv) + len )))
    {
        RtlFreeHeap( GetProcessHeap(), 0, cmdline );
        return NULL;
    }

    arg = d = s = (char *)(argv + argc);
    memcpy( d, cmdline, len );
    bcount = 0;
    in_quotes = 0;
    argc = reserved;
    while (*s)
    {
        if ((*s == ' ' || *s == '\t') && !in_quotes)
        {
            /* Close the argument and copy it */
            *d = 0;
            argv[argc++] = arg;
            /* skip the remaining spaces */
            do
            {
                s++;
            } while (*s == ' ' || *s == '\t');

            /* Start with a new argument */
            arg = d = s;
            bcount = 0;
        }
        else if (*s == '\\')
        {
            *d++ = *s++;
            bcount++;
        }
        else if (*s == '"')
        {
            if ((bcount & 1) == 0)
            {
                /* Preceded by an even number of '\', this is half that
                 * number of '\', plus a '"' which we discard.
                 */
                d -= bcount/2;
                s++;
                if (in_quotes && *s == '"')
                {
                    *d++ = '"';
                    s++;
                }
                else in_quotes = !in_quotes;
            }
            else
            {
                /* Preceded by an odd number of '\', this is half that
                 * number of '\' followed by a '"'
                 */
                d = d - bcount / 2 - 1;
                *d++ = '"';
                s++;
            }
            bcount = 0;
        }
        else
        {
            /* a regular character */
            *d++ = *s++;
            bcount = 0;
        }
    }
    if (*arg)
    {
        *d = '\0';
        argv[argc++] = arg;
    }
    argv[argc] = NULL;

    RtlFreeHeap( GetProcessHeap(), 0, cmdline );
    return argv;
}


static inline const WCHAR *get_params_string( const RTL_USER_PROCESS_PARAMETERS *params,
                                              const UNICODE_STRING *str )
{
    if (params->Flags & PROCESS_PARAMS_FLAG_NORMALIZED) return str->Buffer;
    return (const WCHAR *)((const char *)params + (UINT_PTR)str->Buffer);
}

static inline DWORD append_string( void **ptr, const RTL_USER_PROCESS_PARAMETERS *params,
                                   const UNICODE_STRING *str )
{
    const WCHAR *buffer = get_params_string( params, str );
    memcpy( *ptr, buffer, str->Length );
    *ptr = (WCHAR *)*ptr + str->Length / sizeof(WCHAR);
    return str->Length;
}

/***********************************************************************
 *           create_startup_info
 */
static startup_info_t *create_startup_info( const RTL_USER_PROCESS_PARAMETERS *params, DWORD *info_size )
{
    startup_info_t *info;
    DWORD size;
    void *ptr;

    size = sizeof(*info);
    size += params->CurrentDirectory.DosPath.Length;
    size += params->DllPath.Length;
    size += params->ImagePathName.Length;
    size += params->CommandLine.Length;
    size += params->WindowTitle.Length;
    size += params->Desktop.Length;
    size += params->ShellInfo.Length;
    size += params->RuntimeInfo.Length;
    size = (size + 1) & ~1;
    *info_size = size;

    if (!(info = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, size ))) return NULL;

    info->debug_flags   = params->DebugFlags;
    info->console_flags = params->ConsoleFlags;
    info->console       = wine_server_obj_handle( params->ConsoleHandle );
    info->hstdin        = wine_server_obj_handle( params->hStdInput );
    info->hstdout       = wine_server_obj_handle( params->hStdOutput );
    info->hstderr       = wine_server_obj_handle( params->hStdError );
    info->x             = params->dwX;
    info->y             = params->dwY;
    info->xsize         = params->dwXSize;
    info->ysize         = params->dwYSize;
    info->xchars        = params->dwXCountChars;
    info->ychars        = params->dwYCountChars;
    info->attribute     = params->dwFillAttribute;
    info->flags         = params->dwFlags;
    info->show          = params->wShowWindow;

    ptr = info + 1;
    info->curdir_len = append_string( &ptr, params, &params->CurrentDirectory.DosPath );
    info->dllpath_len = append_string( &ptr, params, &params->DllPath );
    info->imagepath_len = append_string( &ptr, params, &params->ImagePathName );
    info->cmdline_len = append_string( &ptr, params, &params->CommandLine );
    info->title_len = append_string( &ptr, params, &params->WindowTitle );
    info->desktop_len = append_string( &ptr, params, &params->Desktop );
    info->shellinfo_len = append_string( &ptr, params, &params->ShellInfo );
    info->runtime_len = append_string( &ptr, params, &params->RuntimeInfo );
    return info;
}


/***********************************************************************
 *           get_alternate_loader
 *
 * Get the name of the alternate (32 or 64 bit) Wine loader.
 */
static const char *get_alternate_loader( char **ret_env )
{
    char *env;
    const char *loader = NULL;
    const char *loader_env = getenv( "WINELOADER" );

    *ret_env = NULL;

    if (wine_get_build_dir()) loader = is_win64 ? "loader/wine" : "loader/wine64";

    if (loader_env)
    {
        int len = strlen( loader_env );
        if (!is_win64)
        {
            if (!(env = RtlAllocateHeap( GetProcessHeap(), 0, sizeof("WINELOADER=") + len + 2 ))) return NULL;
            strcpy( env, "WINELOADER=" );
            strcat( env, loader_env );
            strcat( env, "64" );
        }
        else
        {
            if (!(env = RtlAllocateHeap( GetProcessHeap(), 0, sizeof("WINELOADER=") + len ))) return NULL;
            strcpy( env, "WINELOADER=" );
            strcat( env, loader_env );
            len += sizeof("WINELOADER=") - 1;
            if (!strcmp( env + len - 2, "64" )) env[len - 2] = 0;
        }
        if (!loader)
        {
            if ((loader = strrchr( env, '/' ))) loader++;
            else loader = env;
        }
        *ret_env = env;
    }
    if (!loader) loader = is_win64 ? "wine" : "wine64";
    return loader;
}


#ifdef __APPLE__
/***********************************************************************
 *           terminate_main_thread
 *
 * On some versions of Mac OS X, the execve system call fails with
 * ENOTSUP if the process has multiple threads.  Wine is always multi-
 * threaded on Mac OS X because it specifically reserves the main thread
 * for use by the system frameworks (see apple_main_thread() in
 * libs/wine/loader.c).  So, when we need to exec without first forking,
 * we need to terminate the main thread first.  We do this by installing
 * a custom run loop source onto the main run loop and signaling it.
 * The source's "perform" callback is pthread_exit and it will be
 * executed on the main thread, terminating it.
 *
 * Returns TRUE if there's still hope the main thread has terminated or
 * will soon.  Return FALSE if we've given up.
 */
static BOOL terminate_main_thread(void)
{
    static int delayms;

    if (!delayms)
    {
        CFRunLoopSourceContext source_context = { 0 };
        CFRunLoopSourceRef source;

        source_context.perform = pthread_exit;
        if (!(source = CFRunLoopSourceCreate( NULL, 0, &source_context )))
            return FALSE;

        CFRunLoopAddSource( CFRunLoopGetMain(), source, kCFRunLoopCommonModes );
        CFRunLoopSourceSignal( source );
        CFRunLoopWakeUp( CFRunLoopGetMain() );
        CFRelease( source );

        delayms = 20;
    }

    if (delayms > 1000)
        return FALSE;

    usleep(delayms * 1000);
    delayms *= 2;

    return TRUE;
}
#endif


/***********************************************************************
 *           set_stdio_fd
 */
static void set_stdio_fd( int stdin_fd, int stdout_fd )
{
    int fd = -1;

    if (stdin_fd == -1 || stdout_fd == -1)
    {
        fd = open( "/dev/null", O_RDWR );
        if (stdin_fd == -1) stdin_fd = fd;
        if (stdout_fd == -1) stdout_fd = fd;
    }

    dup2( stdin_fd, 0 );
    dup2( stdout_fd, 1 );
    if (fd != -1) close( fd );
}


/***********************************************************************
 *           spawn_loader
 */
static NTSTATUS spawn_loader( const RTL_USER_PROCESS_PARAMETERS *params, int socketfd,
                              const char *unixdir, char *winedebug, const pe_image_info_t *pe_info )
{
    pid_t pid;
    int stdin_fd = -1, stdout_fd = -1;
    char *wineloader = NULL;
    const char *loader = NULL;
    char **argv;
    NTSTATUS status = STATUS_SUCCESS;

    argv = build_argv( &params->CommandLine, 1 );

    if (!is_win64 ^ !is_64bit_arch( pe_info->cpu ))
        loader = get_alternate_loader( &wineloader );

    wine_server_handle_to_fd( params->hStdInput, FILE_READ_DATA, &stdin_fd, NULL );
    wine_server_handle_to_fd( params->hStdOutput, FILE_WRITE_DATA, &stdout_fd, NULL );

    if (!(pid = fork()))  /* child */
    {
        if (!(pid = fork()))  /* grandchild */
        {
            char preloader_reserve[64], socket_env[64];
            ULONGLONG res_start = pe_info->base;
            ULONGLONG res_end   = pe_info->base + pe_info->map_size;

            if (params->ConsoleFlags ||
                params->ConsoleHandle == (HANDLE)1 /* KERNEL32_CONSOLE_ALLOC */ ||
                (params->hStdInput == INVALID_HANDLE_VALUE && params->hStdOutput == INVALID_HANDLE_VALUE))
            {
                setsid();
                set_stdio_fd( -1, -1 );  /* close stdin and stdout */
            }
            else set_stdio_fd( stdin_fd, stdout_fd );

            if (stdin_fd != -1) close( stdin_fd );
            if (stdout_fd != -1) close( stdout_fd );

            /* Reset signals that we previously set to SIG_IGN */
            signal( SIGPIPE, SIG_DFL );

            sprintf( socket_env, "WINESERVERSOCKET=%u", socketfd );
            sprintf( preloader_reserve, "WINEPRELOADRESERVE=%x%08x-%x%08x",
                     (ULONG)(res_start >> 32), (ULONG)res_start, (ULONG)(res_end >> 32), (ULONG)res_end );

            putenv( preloader_reserve );
            putenv( socket_env );
            if (winedebug) putenv( winedebug );
            if (wineloader) putenv( wineloader );
            if (unixdir) chdir( unixdir );

            if (argv) wine_exec_wine_binary( loader, argv, getenv("WINELOADER") );
            _exit(1);
        }

        _exit(pid == -1);
    }

    if (pid != -1)
    {
        /* reap child */
        pid_t wret;
        do {
            wret = waitpid(pid, NULL, 0);
        } while (wret < 0 && errno == EINTR);
    }
    else status = FILE_GetNtStatus();

    if (stdin_fd != -1) close( stdin_fd );
    if (stdout_fd != -1) close( stdout_fd );
    RtlFreeHeap( GetProcessHeap(), 0, wineloader );
    RtlFreeHeap( GetProcessHeap(), 0, argv );
    return status;
}


/***********************************************************************
 *           exec_loader
 */
static NTSTATUS exec_loader( const UNICODE_STRING *cmdline, int socketfd, const pe_image_info_t *pe_info )
{
    char *wineloader = NULL;
    const char *loader = NULL;
    char **argv;
    char preloader_reserve[64], socket_env[64];
    ULONGLONG res_start = pe_info->base;
    ULONGLONG res_end   = pe_info->base + pe_info->map_size;

    if (!(argv = build_argv( cmdline, 1 ))) return STATUS_NO_MEMORY;

    if (!is_win64 ^ !is_64bit_arch( pe_info->cpu ))
        loader = get_alternate_loader( &wineloader );

    /* Reset signals that we previously set to SIG_IGN */
    signal( SIGPIPE, SIG_DFL );

    sprintf( socket_env, "WINESERVERSOCKET=%u", socketfd );
    sprintf( preloader_reserve, "WINEPRELOADRESERVE=%x%08x-%x%08x",
             (ULONG)(res_start >> 32), (ULONG)res_start, (ULONG)(res_end >> 32), (ULONG)res_end );

    putenv( preloader_reserve );
    putenv( socket_env );
    if (wineloader) putenv( wineloader );

    do
    {
        wine_exec_wine_binary( loader, argv, getenv("WINELOADER") );
    }
#ifdef __APPLE__
    while (errno == ENOTSUP && terminate_main_thread());
#else
    while (0);
#endif

    RtlFreeHeap( GetProcessHeap(), 0, wineloader );
    RtlFreeHeap( GetProcessHeap(), 0, argv );
    return STATUS_INVALID_IMAGE_FORMAT;
}


/***************************************************************************
 *	is_builtin_path
 */
static BOOL is_builtin_path( UNICODE_STRING *path, BOOL *is_64bit )
{
    static const WCHAR systemW[] = {'\\','?','?','\\','c',':','\\','w','i','n','d','o','w','s','\\',
                                    's','y','s','t','e','m','3','2','\\'};
    static const WCHAR wow64W[] = {'\\','?','?','\\','c',':','\\','w','i','n','d','o','w','s','\\',
                                   's','y','s','w','o','w','6','4'};

    *is_64bit = is_win64;
    if (path->Length > sizeof(systemW) && !strncmpiW( path->Buffer, systemW, ARRAY_SIZE(systemW) ))
    {
        if (is_wow64 && !ntdll_get_thread_data()->wow64_redir) *is_64bit = TRUE;
        return TRUE;
    }
    if ((is_win64 || is_wow64) && path->Length > sizeof(wow64W) &&
        !strncmpiW( path->Buffer, wow64W, ARRAY_SIZE(wow64W) ))
    {
        *is_64bit = FALSE;
        return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           get_so_file_info
 */
static BOOL get_so_file_info( HANDLE handle, pe_image_info_t *info )
{
    union
    {
        struct
        {
            unsigned char magic[4];
            unsigned char class;
            unsigned char data;
            unsigned char version;
            unsigned char ignored1[9];
            unsigned short type;
            unsigned short machine;
            unsigned char ignored2[8];
            unsigned int phoff;
            unsigned char ignored3[12];
            unsigned short phnum;
        } elf;
        struct
        {
            unsigned char magic[4];
            unsigned char class;
            unsigned char data;
            unsigned char ignored1[10];
            unsigned short type;
            unsigned short machine;
            unsigned char ignored2[12];
            unsigned __int64 phoff;
            unsigned char ignored3[16];
            unsigned short phnum;
        } elf64;
        struct
        {
            unsigned int magic;
            unsigned int cputype;
            unsigned int cpusubtype;
            unsigned int filetype;
        } macho;
        IMAGE_DOS_HEADER mz;
    } header;

    IO_STATUS_BLOCK io;
    LARGE_INTEGER offset;

    offset.QuadPart = 0;
    if (NtReadFile( handle, 0, NULL, NULL, &io, &header, sizeof(header), &offset, 0 )) return FALSE;
    if (io.Information != sizeof(header)) return FALSE;

    if (!memcmp( header.elf.magic, "\177ELF", 4 ))
    {
        unsigned int type;
        unsigned short phnum;

        if (header.elf.version != 1 /* EV_CURRENT */) return FALSE;
#ifdef WORDS_BIGENDIAN
        if (header.elf.data != 2 /* ELFDATA2MSB */) return FALSE;
#else
        if (header.elf.data != 1 /* ELFDATA2LSB */) return FALSE;
#endif
        switch (header.elf.machine)
        {
        case 3:   info->cpu = CPU_x86; break;
        case 20:  info->cpu = CPU_POWERPC; break;
        case 40:  info->cpu = CPU_ARM; break;
        case 62:  info->cpu = CPU_x86_64; break;
        case 183: info->cpu = CPU_ARM64; break;
        }
        if (header.elf.type != 3 /* ET_DYN */) return FALSE;
        if (header.elf.class == 2 /* ELFCLASS64 */)
        {
            offset.QuadPart = header.elf64.phoff;
            phnum = header.elf64.phnum;
        }
        else
        {
            offset.QuadPart = header.elf.phoff;
            phnum = header.elf.phnum;
        }
        while (phnum--)
        {
            if (NtReadFile( handle, 0, NULL, NULL, &io, &type, sizeof(type), &offset, 0 )) return FALSE;
            if (io.Information < sizeof(type)) return FALSE;
            if (type == 3 /* PT_INTERP */) return FALSE;
            offset.QuadPart += (header.elf.class == 2) ? 56 : 32;
        }
        return TRUE;
    }
    else if (header.macho.magic == 0xfeedface || header.macho.magic == 0xfeedfacf)
    {
        switch (header.macho.cputype)
        {
        case 0x00000007: info->cpu = CPU_x86; break;
        case 0x01000007: info->cpu = CPU_x86_64; break;
        case 0x0000000c: info->cpu = CPU_ARM; break;
        case 0x0100000c: info->cpu = CPU_ARM64; break;
        case 0x00000012: info->cpu = CPU_POWERPC; break;
        }
        if (header.macho.filetype == 8) return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           get_pe_file_info
 */
static NTSTATUS get_pe_file_info( UNICODE_STRING *path, ULONG attributes,
                                  HANDLE *handle, pe_image_info_t *info )
{
    NTSTATUS status;
    HANDLE mapping;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;

    memset( info, 0, sizeof(*info) );
    InitializeObjectAttributes( &attr, path, attributes, 0, 0 );
    if ((status = NtOpenFile( handle, GENERIC_READ, &attr, &io,
                              FILE_SHARE_READ | FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT )))
    {
        BOOL is_64bit;

        if (is_builtin_path( path, &is_64bit ))
        {
            TRACE( "assuming %u-bit builtin for %s\n", is_64bit ? 64 : 32, debugstr_us(path));
            /* assume current arch */
#if defined(__i386__) || defined(__x86_64__)
            info->cpu = is_64bit ? CPU_x86_64 : CPU_x86;
#elif defined(__powerpc__)
            info->cpu = CPU_POWERPC;
#elif defined(__arm__)
            info->cpu = CPU_ARM;
#elif defined(__aarch64__)
            info->cpu = CPU_ARM64;
#endif
            *handle = 0;
            return STATUS_SUCCESS;
        }
        return status;
    }

    if (!(status = NtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY |
                                    SECTION_MAP_READ | SECTION_MAP_EXECUTE,
                                    NULL, NULL, PAGE_EXECUTE_READ, SEC_IMAGE, *handle )))
    {
        SERVER_START_REQ( get_mapping_info )
        {
            req->handle = wine_server_obj_handle( mapping );
            req->access = SECTION_QUERY;
            wine_server_set_reply( req, info, sizeof(*info) );
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
        NtClose( mapping );
    }
    else if (status == STATUS_INVALID_IMAGE_NOT_MZ)
    {
        if (get_so_file_info( *handle, info )) return STATUS_SUCCESS;
    }
    return status;
}


/***********************************************************************
 *           get_env_size
 */
static ULONG get_env_size( const RTL_USER_PROCESS_PARAMETERS *params, char **winedebug )
{
    WCHAR *ptr = params->Environment;

    while (*ptr)
    {
        static const WCHAR WINEDEBUG[] = {'W','I','N','E','D','E','B','U','G','=',0};
        if (!*winedebug && !strncmpW( ptr, WINEDEBUG, ARRAY_SIZE( WINEDEBUG ) - 1 ))
        {
            DWORD len = ntdll_wcstoumbs( 0, ptr, strlenW(ptr) + 1, NULL, 0, NULL, NULL );
            if ((*winedebug = RtlAllocateHeap( GetProcessHeap(), 0, len )))
                ntdll_wcstoumbs( 0, ptr, strlenW(ptr) + 1, *winedebug, len, NULL, NULL );
        }
        ptr += strlenW(ptr) + 1;
    }
    ptr++;
    return (ptr - params->Environment) * sizeof(WCHAR);
}


/***********************************************************************
 *           get_unix_curdir
 */
static char *get_unix_curdir( const RTL_USER_PROCESS_PARAMETERS *params )
{
    UNICODE_STRING nt_name;
    ANSI_STRING unix_name;
    NTSTATUS status;

    if (!RtlDosPathNameToNtPathName_U( params->CurrentDirectory.DosPath.Buffer, &nt_name, NULL, NULL ))
        return NULL;
    status = wine_nt_to_unix_file_name( &nt_name, &unix_name, FILE_OPEN_IF, FALSE );
    RtlFreeUnicodeString( &nt_name );
    if (status && status != STATUS_NO_SUCH_FILE) return NULL;
    return unix_name.Buffer;
}


/***********************************************************************
 *           fork_and_exec
 *
 * Fork and exec a new Unix binary, checking for errors.
 */
static NTSTATUS fork_and_exec( UNICODE_STRING *path, const RTL_USER_PROCESS_PARAMETERS *params )
{
    pid_t pid;
    int fd[2], stdin_fd = -1, stdout_fd = -1;
    char **argv, **envp;
    char *unixdir;
    ANSI_STRING unix_name;
    NTSTATUS status;

    status = wine_nt_to_unix_file_name( path, &unix_name, FILE_OPEN, FALSE );
    if (status) return status;

#ifdef HAVE_PIPE2
    if (pipe2( fd, O_CLOEXEC ) == -1)
#endif
    {
        if (pipe(fd) == -1)
        {
            RtlFreeAnsiString( &unix_name );
            return STATUS_TOO_MANY_OPENED_FILES;
        }
        fcntl( fd[0], F_SETFD, FD_CLOEXEC );
        fcntl( fd[1], F_SETFD, FD_CLOEXEC );
    }

    wine_server_handle_to_fd( params->hStdInput, FILE_READ_DATA, &stdin_fd, NULL );
    wine_server_handle_to_fd( params->hStdOutput, FILE_WRITE_DATA, &stdout_fd, NULL );

    argv = build_argv( &params->CommandLine, 0 );
    envp = build_envp( params->Environment );
    unixdir = get_unix_curdir( params );

    if (!(pid = fork()))  /* child */
    {
        if (!(pid = fork()))  /* grandchild */
        {
            close( fd[0] );

            if (params->ConsoleFlags ||
                params->ConsoleHandle == (HANDLE)1 /* KERNEL32_CONSOLE_ALLOC */ ||
                (params->hStdInput == INVALID_HANDLE_VALUE && params->hStdOutput == INVALID_HANDLE_VALUE))
            {
                setsid();
                set_stdio_fd( -1, -1 );  /* close stdin and stdout */
            }
            else set_stdio_fd( stdin_fd, stdout_fd );

            if (stdin_fd != -1) close( stdin_fd );
            if (stdout_fd != -1) close( stdout_fd );

            /* Reset signals that we previously set to SIG_IGN */
            signal( SIGPIPE, SIG_DFL );

            if (unixdir) chdir( unixdir );

            if (argv && envp) execve( unix_name.Buffer, argv, envp );
        }

        if (pid <= 0)  /* grandchild if exec failed or child if fork failed */
        {
            status = FILE_GetNtStatus();
            write( fd[1], &status, sizeof(status) );
            _exit(1);
        }

        _exit(0); /* child if fork succeeded */
    }
    close( fd[1] );

    if (pid != -1)
    {
        /* reap child */
        pid_t wret;
        do {
            wret = waitpid(pid, NULL, 0);
        } while (wret < 0 && errno == EINTR);
        read( fd[0], &status, sizeof(status) );  /* if we read something, exec or second fork failed */
    }
    else status = FILE_GetNtStatus();

    close( fd[0] );
    if (stdin_fd != -1) close( stdin_fd );
    if (stdout_fd != -1) close( stdout_fd );
    RtlFreeHeap( GetProcessHeap(), 0, argv );
    RtlFreeHeap( GetProcessHeap(), 0, envp );
    RtlFreeAnsiString( &unix_name );
    return status;
}


/***********************************************************************
 *           restart_process
 */
NTSTATUS restart_process( RTL_USER_PROCESS_PARAMETERS *params, NTSTATUS status )
{
    static const WCHAR argsW[] = {'%','s','%','s',' ','-','-','a','p','p','-','n','a','m','e',' ','"','%','s','"',' ','%','s',0};
    static const WCHAR winevdm[] = {'w','i','n','e','v','d','m','.','e','x','e',0};
    static const WCHAR comW[] = {'.','c','o','m',0};
    static const WCHAR pifW[] = {'.','p','i','f',0};

    int socketfd[2];
    WCHAR *p, *cmdline;
    UNICODE_STRING strW;
    pe_image_info_t pe_info;
    HANDLE handle;

    /* check for .com or .pif extension */
    if (status == STATUS_INVALID_IMAGE_NOT_MZ &&
        (p = strrchrW( params->ImagePathName.Buffer, '.' )) &&
        (!strcmpiW( p, comW ) || !strcmpiW( p, pifW )))
        status = STATUS_INVALID_IMAGE_WIN_16;

    switch (status)
    {
    case STATUS_CONFLICTING_ADDRESSES:
    case STATUS_NO_MEMORY:
    case STATUS_INVALID_IMAGE_FORMAT:
    case STATUS_INVALID_IMAGE_NOT_MZ:
        if (getenv( "WINEPRELOADRESERVE" ))
            return status;
        if ((status = RtlDosPathNameToNtPathName_U_WithStatus( params->ImagePathName.Buffer, &strW,
                                                               NULL, NULL )))
            return status;
        if ((status = get_pe_file_info( &strW, OBJ_CASE_INSENSITIVE, &handle, &pe_info )))
            return status;
        strW = params->CommandLine;
        break;
    case STATUS_INVALID_IMAGE_WIN_16:
    case STATUS_INVALID_IMAGE_NE_FORMAT:
    case STATUS_INVALID_IMAGE_PROTECT:
        cmdline = RtlAllocateHeap( GetProcessHeap(), 0,
                                   (strlenW(system_dir) + strlenW(winevdm) + 16 +
                                    strlenW(params->ImagePathName.Buffer) +
                                    strlenW(params->CommandLine.Buffer)) * sizeof(WCHAR));
        if (!cmdline) return STATUS_NO_MEMORY;
        sprintfW( cmdline, argsW, (is_win64 || is_wow64) ? syswow64_dir : system_dir,
                  winevdm, params->ImagePathName.Buffer, params->CommandLine.Buffer );
        RtlInitUnicodeString( &strW, cmdline );
        memset( &pe_info, 0, sizeof(pe_info) );
        pe_info.cpu = CPU_x86;
        break;
    default:
        return status;
    }

    /* exec the new process */

    if (socketpair( PF_UNIX, SOCK_STREAM, 0, socketfd ) == -1) return STATUS_TOO_MANY_OPENED_FILES;
#ifdef SO_PASSCRED
    else
    {
        int enable = 1;
        setsockopt( socketfd[0], SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable) );
    }
#endif
    wine_server_send_fd( socketfd[1] );
    close( socketfd[1] );

    SERVER_START_REQ( exec_process )
    {
        req->socket_fd = socketfd[1];
        req->cpu       = pe_info.cpu;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    if (!status) status = exec_loader( &strW, socketfd[0], &pe_info );
    close( socketfd[0] );
    return status;
}


/**********************************************************************
 *           RtlCreateUserProcess  (NTDLL.@)
 */
NTSTATUS WINAPI RtlCreateUserProcess( UNICODE_STRING *path, ULONG attributes,
                                      RTL_USER_PROCESS_PARAMETERS *params,
                                      SECURITY_DESCRIPTOR *process_descr,
                                      SECURITY_DESCRIPTOR *thread_descr,
                                      HANDLE parent, BOOLEAN inherit, HANDLE debug, HANDLE exception,
                                      RTL_USER_PROCESS_INFORMATION *info )
{
    NTSTATUS status;
    BOOL success = FALSE;
    HANDLE file_handle, process_info = 0, process_handle = 0, thread_handle = 0;
    ULONG process_id, thread_id;
    struct object_attributes *objattr;
    data_size_t attr_len;
    char *unixdir = NULL, *winedebug = NULL;
    startup_info_t *startup_info = NULL;
    ULONG startup_info_size, env_size;
    int socketfd[2] = { -1, -1 };
    OBJECT_ATTRIBUTES attr;
    pe_image_info_t pe_info;

    RtlNormalizeProcessParams( params );

    TRACE( "%s image %s cmdline %s\n", debugstr_us( path ),
           debugstr_us( &params->ImagePathName ), debugstr_us( &params->CommandLine ));

    if ((status = get_pe_file_info( path, attributes, &file_handle, &pe_info )))
    {
        if (status == STATUS_INVALID_IMAGE_NOT_MZ && !fork_and_exec( path, params ))
        {
            memset( info, 0, sizeof(*info) );
            return STATUS_SUCCESS;
        }
        goto done;
    }
    if (!(startup_info = create_startup_info( params, &startup_info_size ))) goto done;
    env_size = get_env_size( params, &winedebug );
    unixdir = get_unix_curdir( params );

    InitializeObjectAttributes( &attr, NULL, 0, NULL, process_descr );
    if ((status = alloc_object_attributes( &attr, &objattr, &attr_len ))) goto done;

    /* create the socket for the new process */

    if (socketpair( PF_UNIX, SOCK_STREAM, 0, socketfd ) == -1)
    {
        status = STATUS_TOO_MANY_OPENED_FILES;
        RtlFreeHeap( GetProcessHeap(), 0, objattr );
        goto done;
    }
#ifdef SO_PASSCRED
    else
    {
        int enable = 1;
        setsockopt( socketfd[0], SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable) );
    }
#endif

    wine_server_send_fd( socketfd[1] );
    close( socketfd[1] );

    /* create the process on the server side */

    SERVER_START_REQ( new_process )
    {
        req->parent_process = wine_server_obj_handle(parent);
        req->inherit_all    = inherit;
        req->create_flags   = params->DebugFlags; /* hack: creation flags stored in DebugFlags for now */
        req->socket_fd      = socketfd[1];
        req->exe_file       = wine_server_obj_handle( file_handle );
        req->access         = PROCESS_ALL_ACCESS;
        req->cpu            = pe_info.cpu;
        req->info_size      = startup_info_size;
        wine_server_add_data( req, objattr, attr_len );
        wine_server_add_data( req, startup_info, startup_info_size );
        wine_server_add_data( req, params->Environment, env_size );
        if (!(status = wine_server_call( req )))
        {
            process_id = reply->pid;
            process_handle = wine_server_ptr_handle( reply->handle );
        }
        process_info = wine_server_ptr_handle( reply->info );
    }
    SERVER_END_REQ;
    RtlFreeHeap( GetProcessHeap(), 0, objattr );

    if (status)
    {
        switch (status)
        {
        case STATUS_INVALID_IMAGE_WIN_64:
            ERR( "64-bit application %s not supported in 32-bit prefix\n", debugstr_us(path) );
            break;
        case STATUS_INVALID_IMAGE_FORMAT:
            ERR( "%s not supported on this installation (%s binary)\n",
                 debugstr_us(path), cpu_names[pe_info.cpu] );
            break;
        }
        goto done;
    }

    InitializeObjectAttributes( &attr, NULL, 0, NULL, thread_descr );
    if ((status = alloc_object_attributes( &attr, &objattr, &attr_len ))) goto done;

    SERVER_START_REQ( new_thread )
    {
        req->process    = wine_server_obj_handle( process_handle );
        req->access     = THREAD_ALL_ACCESS;
        req->suspend    = 1;
        req->request_fd = -1;
        wine_server_add_data( req, objattr, attr_len );
        if (!(status = wine_server_call( req )))
        {
            thread_handle = wine_server_ptr_handle( reply->handle );
            thread_id = reply->tid;
        }
    }
    SERVER_END_REQ;
    RtlFreeHeap( GetProcessHeap(), 0, objattr );
    if (status) goto done;

    /* create the child process */

    if ((status = spawn_loader( params, socketfd[0], unixdir, winedebug, &pe_info ))) goto done;

    close( socketfd[0] );
    socketfd[0] = -1;

    /* wait for the new process info to be ready */

    NtWaitForSingleObject( process_info, FALSE, NULL );
    SERVER_START_REQ( get_new_process_info )
    {
        req->info = wine_server_obj_handle( process_info );
        wine_server_call( req );
        success = reply->success;
        status = reply->exit_code;
    }
    SERVER_END_REQ;

    if (success)
    {
        TRACE( "%s pid %04x tid %04x handles %p/%p\n", debugstr_us( path ),
               process_id, thread_id, process_handle, thread_handle );
        info->Process = process_handle;
        info->Thread = thread_handle;
        info->ClientId.UniqueProcess = ULongToHandle( process_id );
        info->ClientId.UniqueThread = ULongToHandle( thread_id );
        virtual_fill_image_information( &pe_info, &info->ImageInformation );
        process_handle = thread_handle = 0;
        status = STATUS_SUCCESS;
    }
    else if (!status) status = STATUS_INTERNAL_ERROR;

done:
    if (file_handle) NtClose( file_handle );
    if (process_info) NtClose( process_info );
    if (process_handle) NtClose( process_handle );
    if (thread_handle) NtClose( thread_handle );
    if (socketfd[0] != -1) close( socketfd[0] );
    RtlFreeHeap( GetProcessHeap(), 0, startup_info );
    RtlFreeHeap( GetProcessHeap(), 0, winedebug );
    RtlFreeHeap( GetProcessHeap(), 0, unixdir );
    return status;
}

/***********************************************************************
 *      DbgUiRemoteBreakin (NTDLL.@)
 */
void WINAPI DbgUiRemoteBreakin( void *arg )
{
    TRACE( "\n" );
    if (NtCurrentTeb()->Peb->BeingDebugged)
    {
        __TRY
        {
            DbgBreakPoint();
        }
        __EXCEPT_ALL
        {
            /* do nothing */
        }
        __ENDTRY
    }
    RtlExitUserThread( STATUS_SUCCESS );
}

/***********************************************************************
 *      DbgUiIssueRemoteBreakin (NTDLL.@)
 */
NTSTATUS WINAPI DbgUiIssueRemoteBreakin( HANDLE process )
{
    apc_call_t call;
    apc_result_t result;
    NTSTATUS status;

    TRACE( "(%p)\n", process );

    memset( &call, 0, sizeof(call) );

    call.type = APC_BREAK_PROCESS;
    status = server_queue_process_apc( process, &call, &result );
    if (status) return status;
    return result.break_process.status;
}
