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
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_TIMES_H
# include <sys/times.h>
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
#include "wine/server.h"

#ifdef HAVE_MACH_MACH_H
#include <mach/mach.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(process);

static ULONG execute_flags = MEM_EXECUTE_OPTION_DISABLE | (sizeof(void *) > sizeof(int) ?
                                                           MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION |
                                                           MEM_EXECUTE_OPTION_PERMANENT : 0);

static const BOOL is_win64 = (sizeof(void *) > sizeof(int));


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
            KERNEL_USER_TIMES pti = {{{0}}};

            if (ProcessInformationLength >= sizeof(KERNEL_USER_TIMES))
            {
                if (!ProcessInformation)
                    ret = STATUS_ACCESS_VIOLATION;
                else if (!ProcessHandle)
                    ret = STATUS_INVALID_HANDLE;
                else
                {
                    long ticks = sysconf(_SC_CLK_TCK);
                    struct tms tms;

                    /* FIXME: user/kernel times only work for current process */
                    if (ticks && times( &tms ) != -1)
                    {
                        pti.UserTime.QuadPart = (ULONGLONG)tms.tms_utime * 10000000 / ticks;
                        pti.KernelTime.QuadPart = (ULONGLONG)tms.tms_stime * 10000000 / ticks;
                    }

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

    case ProcessImageInformation:
        len = sizeof(SECTION_IMAGE_INFORMATION);
        if (ProcessInformationLength == len)
        {
            if (ProcessInformation)
            {
                pe_image_info_t pe_info;

                SERVER_START_REQ( get_process_info )
                {
                    req->handle = wine_server_obj_handle( ProcessHandle );
                    wine_server_set_reply( req, &pe_info, sizeof(pe_info) );
                    if ((ret = wine_server_call( req )) == STATUS_SUCCESS)
                        virtual_fill_image_information( &pe_info, ProcessInformation );
                }
                SERVER_END_REQ;
            }
            else ret = STATUS_ACCESS_VIOLATION;
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
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
            unix_funcs->virtual_set_force_exec( enable );
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

/**********************************************************************
 * NtFlushProcessWriteBuffers [NTDLL.@]
 */
void WINAPI NtFlushProcessWriteBuffers(void)
{
    static int once = 0;
    if (!once++) FIXME( "stub\n" );
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
    if (path->Length > sizeof(systemW) && !wcsnicmp( path->Buffer, systemW, ARRAY_SIZE(systemW) ))
    {
        if (is_wow64 && !ntdll_get_thread_data()->wow64_redir) *is_64bit = TRUE;
        return TRUE;
    }
    if ((is_win64 || is_wow64) && path->Length > sizeof(wow64W) &&
        !wcsnicmp( path->Buffer, wow64W, ARRAY_SIZE(wow64W) ))
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
 *           restart_process
 */
NTSTATUS restart_process( RTL_USER_PROCESS_PARAMETERS *params, NTSTATUS status )
{
    static const WCHAR argsW[] = {'%','s','%','s',' ','-','-','a','p','p','-','n','a','m','e',' ','"','%','s','"',' ','%','s',0};
    static const WCHAR winevdm[] = {'w','i','n','e','v','d','m','.','e','x','e',0};
    static const WCHAR comW[] = {'.','c','o','m',0};
    static const WCHAR pifW[] = {'.','p','i','f',0};

    WCHAR *p, *cmdline;
    UNICODE_STRING strW;
    pe_image_info_t pe_info;
    HANDLE handle;

    /* check for .com or .pif extension */
    if (status == STATUS_INVALID_IMAGE_NOT_MZ &&
        (p = wcsrchr( params->ImagePathName.Buffer, '.' )) &&
        (!wcsicmp( p, comW ) || !wcsicmp( p, pifW )))
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
                                   (wcslen(system_dir) + wcslen(winevdm) + 16 +
                                    wcslen(params->ImagePathName.Buffer) +
                                    wcslen(params->CommandLine.Buffer)) * sizeof(WCHAR));
        if (!cmdline) return STATUS_NO_MEMORY;
        NTDLL_swprintf( cmdline, argsW, (is_win64 || is_wow64) ? syswow64_dir : system_dir,
                  winevdm, params->ImagePathName.Buffer, params->CommandLine.Buffer );
        RtlInitUnicodeString( &strW, cmdline );
        memset( &pe_info, 0, sizeof(pe_info) );
        pe_info.cpu = CPU_x86;
        break;
    default:
        return status;
    }

    return unix_funcs->exec_process( &strW, &pe_info );
}


/**********************************************************************
 *           NtCreateUserProcess  (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateUserProcess( HANDLE *process_handle_ptr, HANDLE *thread_handle_ptr,
                                     ACCESS_MASK process_access, ACCESS_MASK thread_access,
                                     OBJECT_ATTRIBUTES *process_attr, OBJECT_ATTRIBUTES *thread_attr,
                                     ULONG process_flags, ULONG thread_flags,
                                     RTL_USER_PROCESS_PARAMETERS *params, PS_CREATE_INFO *info,
                                     PS_ATTRIBUTE_LIST *attr )
{
    return unix_funcs->NtCreateUserProcess( process_handle_ptr, thread_handle_ptr,
                                            process_access, thread_access,
                                            process_attr, thread_attr,
                                            process_flags, thread_flags,
                                            params, info, attr );
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
    OBJECT_ATTRIBUTES process_attr, thread_attr;
    PS_CREATE_INFO create_info;
    ULONG_PTR buffer[offsetof( PS_ATTRIBUTE_LIST, Attributes[5] ) / sizeof(ULONG_PTR)];
    PS_ATTRIBUTE_LIST *attr = (PS_ATTRIBUTE_LIST *)buffer;
    UINT pos = 0;

    RtlNormalizeProcessParams( params );

    attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_IMAGE_NAME;
    attr->Attributes[pos].Size         = path->Length;
    attr->Attributes[pos].ValuePtr     = path->Buffer;
    attr->Attributes[pos].ReturnLength = NULL;
    pos++;
    attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_CLIENT_ID;
    attr->Attributes[pos].Size         = sizeof(info->ClientId);
    attr->Attributes[pos].ValuePtr     = &info->ClientId;
    attr->Attributes[pos].ReturnLength = NULL;
    pos++;
    attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_IMAGE_INFO;
    attr->Attributes[pos].Size         = sizeof(info->ImageInformation);
    attr->Attributes[pos].ValuePtr     = &info->ImageInformation;
    attr->Attributes[pos].ReturnLength = NULL;
    pos++;
    if (parent)
    {
        attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_PARENT_PROCESS;
        attr->Attributes[pos].Size         = sizeof(parent);
        attr->Attributes[pos].ValuePtr     = parent;
        attr->Attributes[pos].ReturnLength = NULL;
        pos++;
    }
    if (debug)
    {
        attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_DEBUG_PORT;
        attr->Attributes[pos].Size         = sizeof(debug);
        attr->Attributes[pos].ValuePtr     = debug;
        attr->Attributes[pos].ReturnLength = NULL;
        pos++;
    }
    attr->TotalLength = offsetof( PS_ATTRIBUTE_LIST, Attributes[pos] );

    InitializeObjectAttributes( &process_attr, NULL, 0, NULL, process_descr );
    InitializeObjectAttributes( &thread_attr, NULL, 0, NULL, thread_descr );

    return NtCreateUserProcess( &info->Process, &info->Thread, PROCESS_ALL_ACCESS, THREAD_ALL_ACCESS,
                                &process_attr, &thread_attr,
                                inherit ? PROCESS_CREATE_FLAGS_INHERIT_HANDLES : 0,
                                THREAD_CREATE_FLAGS_CREATE_SUSPENDED, params,
                                &create_info, attr );
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
    return unix_funcs->DbgUiIssueRemoteBreakin( process );
}
