/*
 * NT basis DLL
 *
 * This file contains the Nt* API functions of NTDLL.DLL.
 * In the original ntdll.dll they all seem to just call int 0x2e (down to the NTOSKRNL)
 *
 * Copyright 1996-1998 Marcus Meissner
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wine/debug.h"

#include "windef.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

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
        req->handle    = handle;
        req->exit_code = exit_code;
        ret = wine_server_call( req );
        self = !ret && reply->self;
    }
    SERVER_END_REQ;
    if (self) exit( exit_code );
    return ret;
}

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

    TRACE("(%p,0x%08x,%p,0x%08lx,%p)\n",
          ProcessHandle,ProcessInformationClass,
          ProcessInformation,ProcessInformationLength,
          ReturnLength);

    switch (ProcessInformationClass) 
    {
    case ProcessBasicInformation:
        {
            PROCESS_BASIC_INFORMATION pbi;

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
                        req->handle = ProcessHandle;
                        if ((ret = wine_server_call( req )) == STATUS_SUCCESS)
                        {
                            pbi.ExitStatus = reply->exit_code;
                            pbi.PebBaseAddress = (DWORD)reply->peb;
                            pbi.AffinityMask = reply->process_affinity;
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
            else ret = STATUS_INFO_LENGTH_MISMATCH;
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
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;
    case ProcessVmCounters:
        {
            VM_COUNTERS pvmi;

            if (ProcessInformationLength >= sizeof(VM_COUNTERS))
            {
                if (!ProcessInformation)
                    ret = STATUS_ACCESS_VIOLATION;
                else if (!ProcessHandle)
                    ret = STATUS_INVALID_HANDLE;
                else
                {
                    /* FIXME : real data */
                    memset(&pvmi, 0 , sizeof(VM_COUNTERS));

                    memcpy(ProcessInformation, &pvmi, sizeof(VM_COUNTERS));

                    len = sizeof(VM_COUNTERS);
                }

                if (ProcessInformationLength > sizeof(VM_COUNTERS))
                    ret = STATUS_INFO_LENGTH_MISMATCH;
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
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
                    /* FIXME : real data */
                    memset(&pti, 0, sizeof(KERNEL_USER_TIMES));

                    memcpy(ProcessInformation, &pti, sizeof(KERNEL_USER_TIMES));

                    len = sizeof(KERNEL_USER_TIMES);
                }

                if (ProcessInformationLength > sizeof(KERNEL_USER_TIMES))
                    ret = STATUS_INFO_LENGTH_MISMATCH;
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;
    case ProcessDebugPort:
        /* "These are not the debuggers you are looking for." *
         * set it to 0 aka "no debugger" to satisfy copy protections */
        if (ProcessInformationLength == 4)
        {
            memset(ProcessInformation, 0, ProcessInformationLength);
            len = 4;
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
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
         else ret = STATUS_INFO_LENGTH_MISMATCH;
         break;
    case ProcessWow64Information:
        if (ProcessInformationLength == 4)
        {
            memset(ProcessInformation, 0, ProcessInformationLength);
            len = 4;
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    default:
        FIXME("(%p,0x%08x,%p,0x%08lx,%p),stub!\n",
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
    FIXME("(%p,0x%08x,%p,0x%08lx) stub\n",
          ProcessHandle,ProcessInformationClass,ProcessInformation,ProcessInformationLength);
    return 0;
}

/******************************************************************************
 * NtFlushInstructionCache [NTDLL.@]
 * ZwFlushInstructionCache [NTDLL.@]
 */
NTSTATUS WINAPI NtFlushInstructionCache(
        IN HANDLE ProcessHandle,
        IN LPCVOID BaseAddress,
        IN ULONG Size)
{
#ifdef __i386__
    TRACE("%p %p %ld - no-op on x86\n", ProcessHandle, BaseAddress, Size );
#else
    FIXME("%p %p %ld\n", ProcessHandle, BaseAddress, Size );
#endif
    return STATUS_SUCCESS;
}
