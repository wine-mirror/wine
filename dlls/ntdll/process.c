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


/******************************************************************************
 *  RtlGetCurrentPeb  [NTDLL.@]
 *
 */
PEB * WINAPI RtlGetCurrentPeb(void)
{
    return NtCurrentTeb()->Peb;
}


/******************************************************************************
 *              RtlIsCurrentProcess  (NTDLL.@)
 */
BOOLEAN WINAPI RtlIsCurrentProcess( HANDLE handle )
{
    return handle == NtCurrentProcess() || !NtCompareObjects( handle, NtCurrentProcess() );
}


/******************************************************************
 *		RtlWow64EnableFsRedirection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlWow64EnableFsRedirection( BOOLEAN enable )
{
    if (!NtCurrentTeb64()) return STATUS_NOT_IMPLEMENTED;
    NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR] = !enable;
    return STATUS_SUCCESS;
}


/******************************************************************
 *		RtlWow64EnableFsRedirectionEx   (NTDLL.@)
 */
NTSTATUS WINAPI RtlWow64EnableFsRedirectionEx( ULONG disable, ULONG *old_value )
{
    if (!NtCurrentTeb64()) return STATUS_NOT_IMPLEMENTED;

    __TRY
    {
        *old_value = NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR];
    }
    __EXCEPT_PAGE_FAULT
    {
        return STATUS_ACCESS_VIOLATION;
    }
    __ENDTRY

    NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR] = disable;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           RtlWow64GetCurrentMachine  (NTDLL.@)
 */
USHORT WINAPI RtlWow64GetCurrentMachine(void)
{
    USHORT machine = current_machine;
#ifdef _WIN64
    if (NtCurrentTeb()->WowTebOffset) RtlWow64GetCurrentCpuArea( &machine, NULL, NULL );
#endif
    return machine;
}


/**********************************************************************
 *           RtlWow64GetProcessMachines  (NTDLL.@)
 */
NTSTATUS WINAPI RtlWow64GetProcessMachines( HANDLE process, USHORT *current_ret, USHORT *native_ret )
{
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION machines[8];
    USHORT current = 0, native = 0;
    NTSTATUS status;
    ULONG i;

    status = NtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, sizeof(process),
                                         machines, sizeof(machines), NULL );
    if (status) return status;
    for (i = 0; machines[i].Machine; i++)
    {
        if (machines[i].Native) native = machines[i].Machine;
        else if (machines[i].Process) current = machines[i].Machine;
    }
    if (current_ret) *current_ret = current;
    if (native_ret) *native_ret = native;
    return status;
}


/**********************************************************************
 *           RtlWow64GetSharedInfoProcess  (NTDLL.@)
 */
NTSTATUS WINAPI RtlWow64GetSharedInfoProcess( HANDLE process, BOOLEAN *is_wow64, WOW64INFO *info )
{
    PEB32 *peb32;
    NTSTATUS status = NtQueryInformationProcess( process, ProcessWow64Information,
                                                 &peb32, sizeof(peb32), NULL );
    if (status) return status;
    if (peb32) status = NtReadVirtualMemory( process, peb32 + 1, info, sizeof(*info), NULL );
    *is_wow64 = !!peb32;
    return status;
}


/**********************************************************************
 *           RtlWow64IsWowGuestMachineSupported  (NTDLL.@)
 */
NTSTATUS WINAPI RtlWow64IsWowGuestMachineSupported( USHORT machine, BOOLEAN *supported )
{
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION machines[8];
    HANDLE process = 0;
    NTSTATUS status;
    ULONG i;

    status = NtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, sizeof(process),
                                         machines, sizeof(machines), NULL );
    if (status) return status;
    *supported = FALSE;
    for (i = 0; machines[i].Machine; i++)
    {
        if (machines[i].Native) continue;
        if (machine == machines[i].Machine) *supported = TRUE;
    }
    return status;
}


#ifdef _WIN64

/**********************************************************************
 *           RtlWow64GetCpuAreaInfo  (NTDLL.@)
 */
NTSTATUS WINAPI RtlWow64GetCpuAreaInfo( WOW64_CPURESERVED *cpu, ULONG reserved, WOW64_CPU_AREA_INFO *info )
{
    static const struct { ULONG machine, align, size, offset, flag; } data[] =
    {
#define ENTRY(machine,type,flag) { machine, TYPE_ALIGNMENT(type), sizeof(type), offsetof(type,ContextFlags), flag },
        ENTRY( IMAGE_FILE_MACHINE_I386, I386_CONTEXT, CONTEXT_i386 )
        ENTRY( IMAGE_FILE_MACHINE_AMD64, AMD64_CONTEXT, CONTEXT_AMD64 )
        ENTRY( IMAGE_FILE_MACHINE_ARMNT, ARM_CONTEXT, CONTEXT_ARM )
        ENTRY( IMAGE_FILE_MACHINE_ARM64, ARM64_NT_CONTEXT, CONTEXT_ARM64 )
#undef ENTRY
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(data); i++)
    {
#define ALIGN(ptr,align) ((void *)(((ULONG_PTR)(ptr) + (align) - 1) & ~((ULONG_PTR)(align) - 1)))
        if (data[i].machine != cpu->Machine) continue;
        info->Context = ALIGN( cpu + 1, data[i].align );
        info->ContextEx = ALIGN( (char *)info->Context + data[i].size, sizeof(void *) );
        info->ContextFlagsLocation = (char *)info->Context + data[i].offset;
        info->ContextFlag = data[i].flag;
        info->CpuReserved = cpu;
        info->Machine = data[i].machine;
        return STATUS_SUCCESS;
#undef ALIGN
    }
    return STATUS_INVALID_PARAMETER;
}


/**********************************************************************
 *           RtlWow64GetCurrentCpuArea  (NTDLL.@)
 */
NTSTATUS WINAPI RtlWow64GetCurrentCpuArea( USHORT *machine, void **context, void **context_ex )
{
    WOW64_CPU_AREA_INFO info;
    NTSTATUS status;

    if (!(status = RtlWow64GetCpuAreaInfo( NtCurrentTeb()->TlsSlots[WOW64_TLS_CPURESERVED], 0, &info )))
    {
        if (machine) *machine = info.Machine;
        if (context) *context = info.Context;
        if (context_ex) *context_ex = *(void **)info.ContextEx;
    }
    return status;
}


/******************************************************************************
 *              RtlWow64GetThreadContext  (NTDLL.@)
 */
NTSTATUS WINAPI RtlWow64GetThreadContext( HANDLE handle, WOW64_CONTEXT *context )
{
    return NtQueryInformationThread( handle, ThreadWow64Context, context, sizeof(*context), NULL );
}


/******************************************************************************
 *              RtlWow64SetThreadContext  (NTDLL.@)
 */
NTSTATUS WINAPI RtlWow64SetThreadContext( HANDLE handle, const WOW64_CONTEXT *context )
{
    return NtSetInformationThread( handle, ThreadWow64Context, context, sizeof(*context) );
}

/******************************************************************************
 *              RtlWow64GetThreadSelectorEntry  (NTDLL.@)
 */
NTSTATUS WINAPI RtlWow64GetThreadSelectorEntry( HANDLE handle, THREAD_DESCRIPTOR_INFORMATION *info,
                                                ULONG size, ULONG *retlen )
{
    DWORD sel;
    WOW64_CONTEXT context = { WOW64_CONTEXT_CONTROL | WOW64_CONTEXT_SEGMENTS };
    LDT_ENTRY entry = { 0 };

    if (size != sizeof(*info)) return STATUS_INFO_LENGTH_MISMATCH;
    if (RtlWow64GetThreadContext( handle, &context ))
    {
        /* hardcoded values */
#ifdef __arm64ec__
        context.SegCs = 0x33;
        context.SegSs = 0x2b;
        context.SegFs = 0x53;
#elif defined(__x86_64__)
        context.SegCs = 0x23;
        __asm__( "movw %%fs,%0" : "=m" (context.SegFs) );
        __asm__( "movw %%ss,%0" : "=m" (context.SegSs) );
#else
        context.SegCs = 0x1b;
        context.SegSs = 0x23;
        context.SegFs = 0x3b;
#endif
    }

    sel = info->Selector | 3;
    if (sel == 0x03) goto done; /* null selector */

    /* set common data */
    entry.HighWord.Bits.Dpl = 3;
    entry.HighWord.Bits.Pres = 1;
    entry.HighWord.Bits.Default_Big = 1;
    if (sel == context.SegCs)  /* code selector */
    {
        entry.LimitLow = 0xffff;
        entry.HighWord.Bits.LimitHi = 0xf;
        entry.HighWord.Bits.Type = 0x1b;  /* code */
        entry.HighWord.Bits.Granularity = 1;
    }
    else if (sel == context.SegSs)  /* data selector */
    {
        entry.LimitLow = 0xffff;
        entry.HighWord.Bits.LimitHi = 0xf;
        entry.HighWord.Bits.Type = 0x13;  /* data */
        entry.HighWord.Bits.Granularity = 1;
    }
    else if (sel == context.SegFs)  /* TEB selector */
    {
        THREAD_BASIC_INFORMATION tbi;

        entry.LimitLow = 0xfff;
        entry.HighWord.Bits.Type = 0x13;  /* data */
        if (!NtQueryInformationThread( handle, ThreadBasicInformation, &tbi, sizeof(tbi), NULL ))
        {
            ULONG addr = (ULONG_PTR)tbi.TebBaseAddress + 0x2000;  /* 32-bit teb offset */
            entry.BaseLow = addr;
            entry.HighWord.Bytes.BaseMid = addr >> 16;
            entry.HighWord.Bytes.BaseHi  = addr >> 24;
        }
    }
    else return STATUS_UNSUCCESSFUL;

done:
    info->Entry = entry;
    if (retlen) *retlen = sizeof(entry);
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           RtlOpenCrossProcessEmulatorWorkConnection  (NTDLL.@)
 */
void WINAPI RtlOpenCrossProcessEmulatorWorkConnection( HANDLE process, HANDLE *section, void **addr )
{
    WOW64INFO wow64info;
    BOOLEAN is_wow64;
    SIZE_T size = 0;

    *addr = NULL;
    *section = 0;

    if (RtlWow64GetSharedInfoProcess( process, &is_wow64, &wow64info )) return;
    if (!is_wow64) return;
    if (!wow64info.SectionHandle) return;

    if (NtDuplicateObject( process, (HANDLE)(ULONG_PTR)wow64info.SectionHandle,
                           GetCurrentProcess(), section, 0, 0, DUPLICATE_SAME_ACCESS ))
        return;

    if (!NtMapViewOfSection( *section, GetCurrentProcess(), addr, 0, 0, NULL,
                             &size, ViewShare, 0, PAGE_READWRITE )) return;

    NtClose( *section );
    *section = 0;
}


/**********************************************************************
 *           RtlWow64PopAllCrossProcessWorkFromWorkList  (NTDLL.@)
 */
CROSS_PROCESS_WORK_ENTRY * WINAPI RtlWow64PopAllCrossProcessWorkFromWorkList( CROSS_PROCESS_WORK_HDR *list, BOOLEAN *flush )
{
    CROSS_PROCESS_WORK_HDR prev, new;
    UINT pos, prev_pos = 0;

    do
    {
        prev.hdr = list->hdr;
        if (!prev.first) break;
        new.first = 0;
        new.counter = prev.counter + 1;
    } while (InterlockedCompareExchange64( &list->hdr, new.hdr, prev.hdr ) != prev.hdr);

    *flush = (prev.first & CROSS_PROCESS_LIST_FLUSH) != 0;
    if (!(pos = prev.first & ~CROSS_PROCESS_LIST_FLUSH)) return NULL;

    /* reverse the list */

    for (;;)
    {
        CROSS_PROCESS_WORK_ENTRY *entry = CROSS_PROCESS_LIST_ENTRY( list, pos );
        UINT next = entry->next;
        entry->next = prev_pos;
        if (!next) return entry;
        prev_pos = pos;
        pos = next;
    }
}


/**********************************************************************
 *           RtlWow64PopCrossProcessWorkFromFreeList  (NTDLL.@)
 */
CROSS_PROCESS_WORK_ENTRY * WINAPI RtlWow64PopCrossProcessWorkFromFreeList( CROSS_PROCESS_WORK_HDR *list )
{
    CROSS_PROCESS_WORK_ENTRY *ret;
    CROSS_PROCESS_WORK_HDR prev, new;

    do
    {
        prev.hdr = list->hdr;
        if (!prev.first) return NULL;
        ret = CROSS_PROCESS_LIST_ENTRY( list, prev.first );
        new.first = ret->next;
        new.counter = prev.counter + 1;
    } while (InterlockedCompareExchange64( &list->hdr, new.hdr, prev.hdr ) != prev.hdr);

    ret->next = 0;
    return ret;
}


/**********************************************************************
 *           RtlWow64PushCrossProcessWorkOntoFreeList  (NTDLL.@)
 */
BOOLEAN WINAPI RtlWow64PushCrossProcessWorkOntoFreeList( CROSS_PROCESS_WORK_HDR *list, CROSS_PROCESS_WORK_ENTRY *entry )
{
    CROSS_PROCESS_WORK_HDR prev, new;

    do
    {
        prev.hdr = list->hdr;
        entry->next = prev.first;
        new.first = (char *)entry - (char *)list;
        new.counter = prev.counter + 1;
    } while (InterlockedCompareExchange64( &list->hdr, new.hdr, prev.hdr ) != prev.hdr);

    return TRUE;
}


/**********************************************************************
 *           RtlWow64PushCrossProcessWorkOntoWorkList  (NTDLL.@)
 */
BOOLEAN WINAPI RtlWow64PushCrossProcessWorkOntoWorkList( CROSS_PROCESS_WORK_HDR *list, CROSS_PROCESS_WORK_ENTRY *entry, void **unknown )
{
    CROSS_PROCESS_WORK_HDR prev, new;

    *unknown = NULL;
    do
    {
        prev.hdr = list->hdr;
        entry->next = prev.first;
        new.first = ((char *)entry - (char *)list) | (prev.first & CROSS_PROCESS_LIST_FLUSH);
        new.counter = prev.counter + 1;
    } while (InterlockedCompareExchange64( &list->hdr, new.hdr, prev.hdr ) != prev.hdr);

    return TRUE;
}


/**********************************************************************
 *           RtlWow64RequestCrossProcessHeavyFlush  (NTDLL.@)
 */
BOOLEAN WINAPI RtlWow64RequestCrossProcessHeavyFlush( CROSS_PROCESS_WORK_HDR *list )
{
    CROSS_PROCESS_WORK_HDR prev, new;

    do
    {
        prev.hdr = list->hdr;
        new.first = prev.first | CROSS_PROCESS_LIST_FLUSH;
        new.counter = prev.counter + 1;
    } while (InterlockedCompareExchange64( &list->hdr, new.hdr, prev.hdr ) != prev.hdr);

    return TRUE;
}

#endif /* _WIN64 */

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
        else if (code == DBG_PRINTEXCEPTION_WIDE_C && info->ExceptionRecord.NumberParameters >= 2)
        {
            event->dwDebugEventCode = OUTPUT_DEBUG_STRING_EVENT;
            event->u.DebugString.lpDebugStringData  = (void *)info->ExceptionRecord.ExceptionInformation[1];
            event->u.DebugString.fUnicode           = TRUE;
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
 *      DbgUiIssueRemoteBreakin (NTDLL.@)
 */
NTSTATUS WINAPI DbgUiIssueRemoteBreakin( HANDLE process )
{
    HANDLE handle;
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr = { sizeof(attr) };

    status = NtCreateThreadEx( &handle, THREAD_ALL_ACCESS, &attr, process,
                               DbgUiRemoteBreakin, NULL, 0, 0, 0, 0, NULL );
#ifdef _WIN64
    /* FIXME: hack for debugging 32-bit wow64 process without a 64-bit ntdll */
    if (status == STATUS_INVALID_PARAMETER)
    {
        ULONG_PTR wow;
        if (!NtQueryInformationProcess( process, ProcessWow64Information, &wow, sizeof(wow), NULL ) && wow)
            status = NtCreateThreadEx( &handle, THREAD_ALL_ACCESS, &attr, process,
                                       (void *)0x7ffe1000, NULL, 0, 0, 0, 0, NULL );
    }
#endif
    if (!status) NtClose( handle );
    return status;
}
