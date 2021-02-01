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

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/debug.h"
#include "windef.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(process);


/******************************************************************************
 *  RtlGetCurrentPeb  [NTDLL.@]
 *
 */
PEB * WINAPI RtlGetCurrentPeb(void)
{
    return NtCurrentTeb()->Peb;
}


/**********************************************************************
 *           RtlCreateUserProcess  (NTDLL.@)
 */
NTSTATUS WINAPI RtlCreateUserProcess( UNICODE_STRING *path, ULONG attributes,
                                      RTL_USER_PROCESS_PARAMETERS *params,
                                      SECURITY_DESCRIPTOR *process_descr,
                                      SECURITY_DESCRIPTOR *thread_descr,
                                      HANDLE parent, BOOLEAN inherit, HANDLE debug, HANDLE token,
                                      RTL_USER_PROCESS_INFORMATION *info )
{
    OBJECT_ATTRIBUTES process_attr, thread_attr;
    PS_CREATE_INFO create_info;
    ULONG_PTR buffer[offsetof( PS_ATTRIBUTE_LIST, Attributes[6] ) / sizeof(ULONG_PTR)];
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
    if (token)
    {
        attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_TOKEN;
        attr->Attributes[pos].Size         = sizeof(token);
        attr->Attributes[pos].ValuePtr     = token;
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
 *      DbgUiGetThreadDebugObject (NTDLL.@)
 */
HANDLE WINAPI DbgUiGetThreadDebugObject(void)
{
    return NtCurrentTeb()->DbgSsReserved[1];
}

/***********************************************************************
 *      DbgUiSetThreadDebugObject (NTDLL.@)
 */
void WINAPI DbgUiSetThreadDebugObject( HANDLE handle )
{
    NtCurrentTeb()->DbgSsReserved[1] = handle;
}

/***********************************************************************
 *      DbgUiConnectToDbg (NTDLL.@)
 */
NTSTATUS WINAPI DbgUiConnectToDbg(void)
{
    HANDLE handle;
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr = { sizeof(attr) };

    if (DbgUiGetThreadDebugObject()) return STATUS_SUCCESS;  /* already connected */

    status = NtCreateDebugObject( &handle, DEBUG_ALL_ACCESS, &attr, DEBUG_KILL_ON_CLOSE );
    if (!status) DbgUiSetThreadDebugObject( handle );
    return status;
}

/***********************************************************************
 *      DbgUiDebugActiveProcess (NTDLL.@)
 */
NTSTATUS WINAPI DbgUiDebugActiveProcess( HANDLE process )
{
    NTSTATUS status;

    if ((status = NtDebugActiveProcess( process, DbgUiGetThreadDebugObject() ))) return status;
    if ((status = DbgUiIssueRemoteBreakin( process ))) DbgUiStopDebugging( process );
    return status;
}

/***********************************************************************
 *      DbgUiStopDebugging (NTDLL.@)
 */
NTSTATUS WINAPI DbgUiStopDebugging( HANDLE process )
{
    return NtRemoveProcessDebug( process, DbgUiGetThreadDebugObject() );
}

/***********************************************************************
 *      DbgUiContinue (NTDLL.@)
 */
NTSTATUS WINAPI DbgUiContinue( CLIENT_ID *client, NTSTATUS status )
{
    return NtDebugContinue( DbgUiGetThreadDebugObject(), client, status );
}

/***********************************************************************
 *      DbgUiWaitStateChange (NTDLL.@)
 */
NTSTATUS WINAPI DbgUiWaitStateChange( DBGUI_WAIT_STATE_CHANGE *state, LARGE_INTEGER *timeout )
{
    return NtWaitForDebugEvent( DbgUiGetThreadDebugObject(), TRUE, timeout, state );
}

/* helper for DbgUiConvertStateChangeStructure */
static inline void *get_thread_teb( HANDLE thread )
{
    THREAD_BASIC_INFORMATION info;

    if (NtQueryInformationThread( thread, ThreadBasicInformation, &info, sizeof(info), NULL )) return NULL;
    return info.TebBaseAddress;
}

/***********************************************************************
 *      DbgUiConvertStateChangeStructure (NTDLL.@)
 */
NTSTATUS WINAPI DbgUiConvertStateChangeStructure( DBGUI_WAIT_STATE_CHANGE *state, DEBUG_EVENT *event )
{
    event->dwProcessId = HandleToULong( state->AppClientId.UniqueProcess );
    event->dwThreadId  = HandleToULong( state->AppClientId.UniqueThread );
    switch (state->NewState)
    {
    case DbgCreateThreadStateChange:
    {
        DBGUI_CREATE_THREAD *info = &state->StateInfo.CreateThread;
        event->dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
        event->u.CreateThread.hThread           = info->HandleToThread;
        event->u.CreateThread.lpThreadLocalBase = get_thread_teb( info->HandleToThread );
        event->u.CreateThread.lpStartAddress    = info->NewThread.StartAddress;
        break;
    }
    case DbgCreateProcessStateChange:
    {
        DBGUI_CREATE_PROCESS *info = &state->StateInfo.CreateProcessInfo;
        event->dwDebugEventCode = CREATE_PROCESS_DEBUG_EVENT;
        event->u.CreateProcessInfo.hFile                 = info->NewProcess.FileHandle;
        event->u.CreateProcessInfo.hProcess              = info->HandleToProcess;
        event->u.CreateProcessInfo.hThread               = info->HandleToThread;
        event->u.CreateProcessInfo.lpBaseOfImage         = info->NewProcess.BaseOfImage;
        event->u.CreateProcessInfo.dwDebugInfoFileOffset = info->NewProcess.DebugInfoFileOffset;
        event->u.CreateProcessInfo.nDebugInfoSize        = info->NewProcess.DebugInfoSize;
        event->u.CreateProcessInfo.lpThreadLocalBase     = get_thread_teb( info->HandleToThread );
        event->u.CreateProcessInfo.lpStartAddress        = info->NewProcess.InitialThread.StartAddress;
        event->u.CreateProcessInfo.lpImageName           = NULL;
        event->u.CreateProcessInfo.fUnicode              = TRUE;
        break;
    }
    case DbgExitThreadStateChange:
    {
        DBGKM_EXIT_THREAD *info = &state->StateInfo.ExitThread;
        event->dwDebugEventCode = EXIT_THREAD_DEBUG_EVENT;
        event->u.ExitThread.dwExitCode = info->ExitStatus;
        break;
    }
    case DbgExitProcessStateChange:
    {
        DBGKM_EXIT_PROCESS *info = &state->StateInfo.ExitProcess;
        event->dwDebugEventCode = EXIT_PROCESS_DEBUG_EVENT;
        event->u.ExitProcess.dwExitCode = info->ExitStatus;
        break;
    }
    case DbgExceptionStateChange:
    case DbgBreakpointStateChange:
    case DbgSingleStepStateChange:
    {
        DBGKM_EXCEPTION *info = &state->StateInfo.Exception;
        DWORD code = info->ExceptionRecord.ExceptionCode;
        if (code == DBG_PRINTEXCEPTION_C && info->ExceptionRecord.NumberParameters >= 2)
        {
            event->dwDebugEventCode = OUTPUT_DEBUG_STRING_EVENT;
            event->u.DebugString.lpDebugStringData  = (void *)info->ExceptionRecord.ExceptionInformation[1];
            event->u.DebugString.fUnicode           = FALSE;
            event->u.DebugString.nDebugStringLength = info->ExceptionRecord.ExceptionInformation[0];
        }
        else if (code == DBG_RIPEXCEPTION && info->ExceptionRecord.NumberParameters >= 2)
        {
            event->dwDebugEventCode = RIP_EVENT;
            event->u.RipInfo.dwError = info->ExceptionRecord.ExceptionInformation[0];
            event->u.RipInfo.dwType  = info->ExceptionRecord.ExceptionInformation[1];
        }
        else
        {
            event->dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
            event->u.Exception.ExceptionRecord = info->ExceptionRecord;
            event->u.Exception.dwFirstChance   = info->FirstChance;
        }
        break;
    }
    case DbgLoadDllStateChange:
    {
        DBGKM_LOAD_DLL *info = &state->StateInfo.LoadDll;
        event->dwDebugEventCode = LOAD_DLL_DEBUG_EVENT;
        event->u.LoadDll.hFile                 = info->FileHandle;
        event->u.LoadDll.lpBaseOfDll           = info->BaseOfDll;
        event->u.LoadDll.dwDebugInfoFileOffset = info->DebugInfoFileOffset;
        event->u.LoadDll.nDebugInfoSize        = info->DebugInfoSize;
        event->u.LoadDll.lpImageName           = info->NamePointer;
        event->u.LoadDll.fUnicode              = TRUE;
        break;
    }
    case DbgUnloadDllStateChange:
    {
        DBGKM_UNLOAD_DLL *info = &state->StateInfo.UnloadDll;
        event->dwDebugEventCode = UNLOAD_DLL_DEBUG_EVENT;
        event->u.UnloadDll.lpBaseOfDll = info->BaseAddress;
        break;
    }
    default:
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
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
