/*
 * Win32 processes
 *
 * Copyright 1996, 1998 Alexandre Julliard
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
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winternl.h"

#include "kernelbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(process);

static DWORD shutdown_flags = 0;
static DWORD shutdown_priority = 0x280;

/***********************************************************************
 * Processes
 ***********************************************************************/


/****************************************************************************
 *           FlushInstructionCache   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FlushInstructionCache( HANDLE process, LPCVOID addr, SIZE_T size )
{
    return set_ntstatus( NtFlushInstructionCache( process, addr, size ));
}


/***********************************************************************
 *           GetCurrentProcess   (kernelbase.@)
 */
HANDLE WINAPI kernelbase_GetCurrentProcess(void)
{
    return (HANDLE)~(ULONG_PTR)0;
}


/***********************************************************************
 *           GetCurrentProcessId   (kernelbase.@)
 */
DWORD WINAPI kernelbase_GetCurrentProcessId(void)
{
    return HandleToULong( NtCurrentTeb()->ClientId.UniqueProcess );
}


/***********************************************************************
 *           GetErrorMode   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetErrorMode(void)
{
    UINT mode;

    NtQueryInformationProcess( GetCurrentProcess(), ProcessDefaultHardErrorMode,
                               &mode, sizeof(mode), NULL );
    return mode;
}


/***********************************************************************
 *           GetExitCodeProcess   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetExitCodeProcess( HANDLE process, LPDWORD exit_code )
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION pbi;

    status = NtQueryInformationProcess( process, ProcessBasicInformation, &pbi, sizeof(pbi), NULL );
    if (status && exit_code) *exit_code = pbi.ExitStatus;
    return set_ntstatus( status );
}


/***********************************************************************
 *           GetPriorityClass   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetPriorityClass( HANDLE process )
{
    PROCESS_BASIC_INFORMATION pbi;

    if (!set_ntstatus( NtQueryInformationProcess( process, ProcessBasicInformation,
                                                  &pbi, sizeof(pbi), NULL )))
        return 0;

    switch (pbi.BasePriority)
    {
    case PROCESS_PRIOCLASS_IDLE: return IDLE_PRIORITY_CLASS;
    case PROCESS_PRIOCLASS_BELOW_NORMAL: return BELOW_NORMAL_PRIORITY_CLASS;
    case PROCESS_PRIOCLASS_NORMAL: return NORMAL_PRIORITY_CLASS;
    case PROCESS_PRIOCLASS_ABOVE_NORMAL: return ABOVE_NORMAL_PRIORITY_CLASS;
    case PROCESS_PRIOCLASS_HIGH: return HIGH_PRIORITY_CLASS;
    case PROCESS_PRIOCLASS_REALTIME: return REALTIME_PRIORITY_CLASS;
    default: return 0;
    }
}


/******************************************************************
 *           GetProcessHandleCount   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessHandleCount( HANDLE process, DWORD *count )
{
    return set_ntstatus( NtQueryInformationProcess( process, ProcessHandleCount,
                                                    count, sizeof(*count), NULL ));
}


/***********************************************************************
 *           GetProcessHeap   (kernelbase.@)
 */
HANDLE WINAPI kernelbase_GetProcessHeap(void)
{
    return NtCurrentTeb()->Peb->ProcessHeap;
}


/*********************************************************************
 *           GetProcessId   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetProcessId( HANDLE process )
{
    PROCESS_BASIC_INFORMATION pbi;

    if (!set_ntstatus( NtQueryInformationProcess( process, ProcessBasicInformation,
                                                  &pbi, sizeof(pbi), NULL )))
        return 0;
    return pbi.UniqueProcessId;
}


/**********************************************************************
 *           GetProcessMitigationPolicy   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessMitigationPolicy( HANDLE process, PROCESS_MITIGATION_POLICY policy,
                                                          void *buffer, SIZE_T length )
{
    FIXME( "(%p, %u, %p, %lu): stub\n", process, policy, buffer, length );
    return TRUE;
}


/***********************************************************************
 *           GetProcessPriorityBoost   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessPriorityBoost( HANDLE process, PBOOL disable )
{
    FIXME( "(%p,%p): semi-stub\n", process, disable );
    *disable = FALSE;  /* report that no boost is present */
    return TRUE;
}


/***********************************************************************
 *           GetProcessShutdownParameters   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessShutdownParameters( LPDWORD level, LPDWORD flags )
{
    *level = shutdown_priority;
    *flags = shutdown_flags;
    return TRUE;
}


/***********************************************************************
 *           GetProcessWorkingSetSizeEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessWorkingSetSizeEx( HANDLE process, SIZE_T *minset,
                                                          SIZE_T *maxset, DWORD *flags)
{
    FIXME( "(%p,%p,%p,%p): stub\n", process, minset, maxset, flags );
    /* 32 MB working set size */
    if (minset) *minset = 32*1024*1024;
    if (maxset) *maxset = 32*1024*1024;
    if (flags) *flags = QUOTA_LIMITS_HARDWS_MIN_DISABLE | QUOTA_LIMITS_HARDWS_MAX_DISABLE;
    return TRUE;
}


/**********************************************************************
 *           IsWow64Process   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsWow64Process( HANDLE process, PBOOL wow64 )
{
    ULONG_PTR pbi;
    NTSTATUS status;

    status = NtQueryInformationProcess( process, ProcessWow64Information, &pbi, sizeof(pbi), NULL );
    if (!status) *wow64 = !!pbi;
    return set_ntstatus( status );
}


/*********************************************************************
 *           OpenProcess   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenProcess( DWORD access, BOOL inherit, DWORD id )
{
    HANDLE handle;
    OBJECT_ATTRIBUTES attr;
    CLIENT_ID cid;

    if (GetVersion() & 0x80000000) access = PROCESS_ALL_ACCESS;

    attr.Length = sizeof(OBJECT_ATTRIBUTES);
    attr.RootDirectory = 0;
    attr.Attributes = inherit ? OBJ_INHERIT : 0;
    attr.ObjectName = NULL;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    cid.UniqueProcess = ULongToHandle(id);
    cid.UniqueThread  = 0;

    if (!set_ntstatus( NtOpenProcess( &handle, access, &attr, &cid ))) return NULL;
    return handle;
}


/***********************************************************************
 *           SetErrorMode   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH SetErrorMode( UINT mode )
{
    UINT old = GetErrorMode();

    NtSetInformationProcess( GetCurrentProcess(), ProcessDefaultHardErrorMode,
                             &mode, sizeof(mode) );
    return old;
}


/***********************************************************************
 *           SetPriorityClass   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetPriorityClass( HANDLE process, DWORD class )
{
    PROCESS_PRIORITY_CLASS ppc;

    ppc.Foreground = FALSE;
    switch (class)
    {
    case IDLE_PRIORITY_CLASS:         ppc.PriorityClass = PROCESS_PRIOCLASS_IDLE; break;
    case BELOW_NORMAL_PRIORITY_CLASS: ppc.PriorityClass = PROCESS_PRIOCLASS_BELOW_NORMAL; break;
    case NORMAL_PRIORITY_CLASS:       ppc.PriorityClass = PROCESS_PRIOCLASS_NORMAL; break;
    case ABOVE_NORMAL_PRIORITY_CLASS: ppc.PriorityClass = PROCESS_PRIOCLASS_ABOVE_NORMAL; break;
    case HIGH_PRIORITY_CLASS:         ppc.PriorityClass = PROCESS_PRIOCLASS_HIGH; break;
    case REALTIME_PRIORITY_CLASS:     ppc.PriorityClass = PROCESS_PRIOCLASS_REALTIME; break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return set_ntstatus( NtSetInformationProcess( process, ProcessPriorityClass, &ppc, sizeof(ppc) ));
}


/***********************************************************************
 *           SetProcessAffinityUpdateMode   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetProcessAffinityUpdateMode( HANDLE process, DWORD flags )
{
    FIXME( "(%p,0x%08x): stub\n", process, flags );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/**********************************************************************
 *           SetProcessMitigationPolicy   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetProcessMitigationPolicy( PROCESS_MITIGATION_POLICY policy,
                                                          void *buffer, SIZE_T length )
{
    FIXME( "(%d, %p, %lu): stub\n", policy, buffer, length );
    return TRUE;
}


/***********************************************************************
 *           SetProcessPriorityBoost   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetProcessPriorityBoost( HANDLE process, BOOL disable )
{
    FIXME( "(%p,%d): stub\n", process, disable );
    return TRUE;
}


/***********************************************************************
 *           SetProcessShutdownParameters   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetProcessShutdownParameters( DWORD level, DWORD flags )
{
    FIXME( "(%08x, %08x): partial stub.\n", level, flags );
    shutdown_flags = flags;
    shutdown_priority = level;
    return TRUE;
}


/***********************************************************************
 *           SetProcessWorkingSetSizeEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetProcessWorkingSetSizeEx( HANDLE process, SIZE_T minset,
                                                          SIZE_T maxset, DWORD flags )
{
    WARN( "(%p,%ld,%ld,%x): stub - harmless\n", process, minset, maxset, flags );
    return TRUE;
}


/******************************************************************************
 *           TerminateProcess   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH TerminateProcess( HANDLE handle, DWORD exit_code )
{
    if (!handle)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    return set_ntstatus( NtTerminateProcess( handle, exit_code ));
}


/***********************************************************************
 * Process/thread attribute lists
 ***********************************************************************/


struct proc_thread_attr
{
    DWORD_PTR attr;
    SIZE_T size;
    void *value;
};

struct _PROC_THREAD_ATTRIBUTE_LIST
{
    DWORD mask;  /* bitmask of items in list */
    DWORD size;  /* max number of items in list */
    DWORD count; /* number of items in list */
    DWORD pad;
    DWORD_PTR unk;
    struct proc_thread_attr attrs[1];
};

/***********************************************************************
 *           InitializeProcThreadAttributeList   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH InitializeProcThreadAttributeList( struct _PROC_THREAD_ATTRIBUTE_LIST *list,
                                                                 DWORD count, DWORD flags, SIZE_T *size )
{
    SIZE_T needed;
    BOOL ret = FALSE;

    TRACE( "(%p %d %x %p)\n", list, count, flags, size );

    needed = FIELD_OFFSET( struct _PROC_THREAD_ATTRIBUTE_LIST, attrs[count] );
    if (list && *size >= needed)
    {
        list->mask = 0;
        list->size = count;
        list->count = 0;
        list->unk = 0;
        ret = TRUE;
    }
    else SetLastError( ERROR_INSUFFICIENT_BUFFER );

    *size = needed;
    return ret;
}


/***********************************************************************
 *           UpdateProcThreadAttribute   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH UpdateProcThreadAttribute( struct _PROC_THREAD_ATTRIBUTE_LIST *list,
                                                         DWORD flags, DWORD_PTR attr, void *value,
                                                         SIZE_T size, void *prev_ret, SIZE_T *size_ret )
{
    DWORD mask;
    struct proc_thread_attr *entry;

    TRACE( "(%p %x %08lx %p %ld %p %p)\n", list, flags, attr, value, size, prev_ret, size_ret );

    if (list->count >= list->size)
    {
        SetLastError( ERROR_GEN_FAILURE );
        return FALSE;
    }

    switch (attr)
    {
    case PROC_THREAD_ATTRIBUTE_PARENT_PROCESS:
        if (size != sizeof(HANDLE))
        {
            SetLastError( ERROR_BAD_LENGTH );
            return FALSE;
        }
        break;

    case PROC_THREAD_ATTRIBUTE_HANDLE_LIST:
        if ((size / sizeof(HANDLE)) * sizeof(HANDLE) != size)
        {
            SetLastError( ERROR_BAD_LENGTH );
            return FALSE;
        }
        break;

    case PROC_THREAD_ATTRIBUTE_IDEAL_PROCESSOR:
        if (size != sizeof(PROCESSOR_NUMBER))
        {
            SetLastError( ERROR_BAD_LENGTH );
            return FALSE;
        }
        break;

    case PROC_THREAD_ATTRIBUTE_CHILD_PROCESS_POLICY:
       if (size != sizeof(DWORD) && size != sizeof(DWORD64))
       {
           SetLastError( ERROR_BAD_LENGTH );
           return FALSE;
       }
       break;

    case PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY:
        if (size != sizeof(DWORD) && size != sizeof(DWORD64) && size != sizeof(DWORD64) * 2)
        {
            SetLastError( ERROR_BAD_LENGTH );
            return FALSE;
        }
        break;

    default:
        SetLastError( ERROR_NOT_SUPPORTED );
        FIXME( "Unhandled attribute %lu\n", attr & PROC_THREAD_ATTRIBUTE_NUMBER );
        return FALSE;
    }

    mask = 1 << (attr & PROC_THREAD_ATTRIBUTE_NUMBER);
    if (list->mask & mask)
    {
        SetLastError( ERROR_OBJECT_NAME_EXISTS );
        return FALSE;
    }
    list->mask |= mask;

    entry = list->attrs + list->count;
    entry->attr = attr;
    entry->size = size;
    entry->value = value;
    list->count++;
    return TRUE;
}


/***********************************************************************
 *           DeleteProcThreadAttributeList   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH DeleteProcThreadAttributeList( struct _PROC_THREAD_ATTRIBUTE_LIST *list )
{
    return;
}
