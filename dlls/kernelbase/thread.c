/*
 * Win32 threads
 *
 * Copyright 1996 Alexandre Julliard
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

WINE_DEFAULT_DEBUG_CHANNEL(thread);


/***********************************************************************
 * Threads
 ***********************************************************************/


static DWORD rtlmode_to_win32mode( DWORD rtlmode )
{
    DWORD win32mode = 0;

    if (rtlmode & 0x10) win32mode |= SEM_FAILCRITICALERRORS;
    if (rtlmode & 0x20) win32mode |= SEM_NOGPFAULTERRORBOX;
    if (rtlmode & 0x40) win32mode |= SEM_NOOPENFILEERRORBOX;
    return win32mode;
}


/***************************************************************************
 *           CreateRemoteThread   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateRemoteThread( HANDLE process, SECURITY_ATTRIBUTES *sa, SIZE_T stack,
                                                    LPTHREAD_START_ROUTINE start, LPVOID param,
                                                    DWORD flags, DWORD *id )
{
    return CreateRemoteThreadEx( process, sa, stack, start, param, flags, NULL, id );
}


/***************************************************************************
 *           CreateRemoteThreadEx   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateRemoteThreadEx( HANDLE process, SECURITY_ATTRIBUTES *sa,
                                                      SIZE_T stack, LPTHREAD_START_ROUTINE start,
                                                      LPVOID param, DWORD flags,
                                                      LPPROC_THREAD_ATTRIBUTE_LIST attributes, DWORD *id )
{
    HANDLE handle;
    CLIENT_ID client_id;
    NTSTATUS status;
    SIZE_T stack_reserve = 0, stack_commit = 0;

    if (attributes) FIXME("thread attributes ignored\n");

    if (flags & STACK_SIZE_PARAM_IS_A_RESERVATION) stack_reserve = stack;
    else stack_commit = stack;

    status = RtlCreateUserThread( process, sa ? sa->lpSecurityDescriptor : NULL, TRUE,
                                  NULL, stack_reserve, stack_commit,
                                  (PRTL_THREAD_START_ROUTINE)start, param, &handle, &client_id );
    if (status == STATUS_SUCCESS)
    {
        if (id) *id = HandleToULong( client_id.UniqueThread );
        if (sa && sa->nLength >= sizeof(*sa) && sa->bInheritHandle)
            SetHandleInformation( handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT );
        if (!(flags & CREATE_SUSPENDED))
        {
            ULONG ret;
            if (NtResumeThread( handle, &ret ))
            {
                NtClose( handle );
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                handle = 0;
            }
        }
    }
    else
    {
        SetLastError( RtlNtStatusToDosError(status) );
        handle = 0;
    }
    return handle;
}


/***********************************************************************
 *           CreateThread   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateThread( SECURITY_ATTRIBUTES *sa, SIZE_T stack,
                                              LPTHREAD_START_ROUTINE start, LPVOID param,
                                              DWORD flags, LPDWORD id )
{
     return CreateRemoteThread( GetCurrentProcess(), sa, stack, start, param, flags, id );
}


/***********************************************************************
 *           FreeLibraryAndExitThread   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH FreeLibraryAndExitThread( HINSTANCE module, DWORD exit_code )
{
    FreeLibrary( module );
    RtlExitUserThread( exit_code );
}


/***********************************************************************
 *	     GetCurrentThreadStackLimits   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH GetCurrentThreadStackLimits( ULONG_PTR *low, ULONG_PTR *high )
{
    *low = (ULONG_PTR)NtCurrentTeb()->DeallocationStack;
    *high = (ULONG_PTR)NtCurrentTeb()->Tib.StackBase;
}


/***********************************************************************
 *           GetCurrentThread   (kernelbase.@)
 */
HANDLE WINAPI kernelbase_GetCurrentThread(void)
{
    return (HANDLE)~(ULONG_PTR)1;
}


/***********************************************************************
 *           GetCurrentThreadId   (kernelbase.@)
 */
DWORD WINAPI kernelbase_GetCurrentThreadId(void)
{
    return HandleToULong( NtCurrentTeb()->ClientId.UniqueThread );
}


/**********************************************************************
 *           GetExitCodeThread   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetExitCodeThread( HANDLE thread, LPDWORD exit_code )
{
    THREAD_BASIC_INFORMATION info;
    NTSTATUS status = NtQueryInformationThread( thread, ThreadBasicInformation,
                                                &info, sizeof(info), NULL );
    if (!status && exit_code) *exit_code = info.ExitStatus;
    return set_ntstatus( status );
}


/**********************************************************************
 *           GetProcessIdOfThread   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetProcessIdOfThread( HANDLE thread )
{
    THREAD_BASIC_INFORMATION tbi;

    if (!set_ntstatus( NtQueryInformationThread( thread, ThreadBasicInformation, &tbi, sizeof(tbi), NULL)))
        return 0;
    return HandleToULong( tbi.ClientId.UniqueProcess );
}


/***********************************************************************
 *           GetThreadContext   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetThreadContext( HANDLE thread, CONTEXT *context )
{
    return set_ntstatus( NtGetContextThread( thread, context ));
}


/***********************************************************************
 *           GetThreadErrorMode   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetThreadErrorMode(void)
{
    return rtlmode_to_win32mode( RtlGetThreadErrorMode() );
}


/***********************************************************************
 *           GetThreadGroupAffinity   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetThreadGroupAffinity( HANDLE thread, GROUP_AFFINITY *affinity )
{
    if (!affinity)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return set_ntstatus( NtQueryInformationThread( thread, ThreadGroupInformation,
                                                   affinity, sizeof(*affinity), NULL ));
}


/***********************************************************************
 *	     GetThreadIOPendingFlag   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetThreadIOPendingFlag( HANDLE thread, PBOOL pending )
{
    return set_ntstatus( NtQueryInformationThread( thread, ThreadIsIoPending,
                                                   pending, sizeof(*pending), NULL ));
}


/**********************************************************************
 *           GetThreadId   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetThreadId( HANDLE thread )
{
    THREAD_BASIC_INFORMATION tbi;

    if (!set_ntstatus( NtQueryInformationThread( thread, ThreadBasicInformation, &tbi, sizeof(tbi), NULL)))
        return 0;
    return HandleToULong( tbi.ClientId.UniqueThread );
}


/**********************************************************************
 *           GetThreadPriority   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetThreadPriority( HANDLE thread )
{
    THREAD_BASIC_INFORMATION info;

    if (!set_ntstatus( NtQueryInformationThread( thread, ThreadBasicInformation,
                                                 &info, sizeof(info), NULL )))
        return THREAD_PRIORITY_ERROR_RETURN;
    return info.Priority;
}


/**********************************************************************
 *           GetThreadPriorityBoost   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetThreadPriorityBoost( HANDLE thread, BOOL *state )
{
    if (state) *state = FALSE;
    return TRUE;
}


/**********************************************************************
 *           GetThreadTimes   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetThreadTimes( HANDLE thread, LPFILETIME creationtime, LPFILETIME exittime,
                                              LPFILETIME kerneltime, LPFILETIME usertime )
{
    KERNEL_USER_TIMES times;
    NTSTATUS status = NtQueryInformationThread( thread, ThreadTimes, &times, sizeof(times), NULL);
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    if (creationtime)
    {
        creationtime->dwLowDateTime = times.CreateTime.u.LowPart;
        creationtime->dwHighDateTime = times.CreateTime.u.HighPart;
    }
    if (exittime)
    {
        exittime->dwLowDateTime = times.ExitTime.u.LowPart;
        exittime->dwHighDateTime = times.ExitTime.u.HighPart;
    }
    if (kerneltime)
    {
        kerneltime->dwLowDateTime = times.KernelTime.u.LowPart;
        kerneltime->dwHighDateTime = times.KernelTime.u.HighPart;
    }
    if (usertime)
    {
        usertime->dwLowDateTime = times.UserTime.u.LowPart;
        usertime->dwHighDateTime = times.UserTime.u.HighPart;
    }
    return TRUE;
}


/***********************************************************************
 *	     GetThreadUILanguage   (kernelbase.@)
 */
LANGID WINAPI DECLSPEC_HOTPATCH GetThreadUILanguage(void)
{
    LANGID lang;

    FIXME(": stub, returning default language.\n");
    NtQueryDefaultUILanguage( &lang );
    return lang;
}


/***********************************************************************
 *	     OpenThread   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenThread( DWORD access, BOOL inherit, DWORD id )
{
    HANDLE handle;
    OBJECT_ATTRIBUTES attr;
    CLIENT_ID cid;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = inherit ? OBJ_INHERIT : 0;
    attr.ObjectName = NULL;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    cid.UniqueProcess = 0;
    cid.UniqueThread = ULongToHandle( id );

    if (!set_ntstatus( NtOpenThread( &handle, access, &attr, &cid ))) handle = 0;
    return handle;
}


/* callback for QueueUserAPC */
static void CALLBACK call_user_apc( ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3 )
{
    PAPCFUNC func = (PAPCFUNC)arg1;
    func( arg2 );
}

/***********************************************************************
 *	     QueueUserAPC   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH QueueUserAPC( PAPCFUNC func, HANDLE thread, ULONG_PTR data )
{
    return set_ntstatus( NtQueueApcThread( thread, call_user_apc, (ULONG_PTR)func, data, 0 ));
}


/**********************************************************************
 *           ResumeThread   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH ResumeThread( HANDLE thread )
{
    DWORD ret;

    if (!set_ntstatus( NtResumeThread( thread, &ret ))) ret = ~0U;
    return ret;
}


/***********************************************************************
 *           SetThreadContext   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadContext( HANDLE thread, const CONTEXT *context )
{
    return set_ntstatus( NtSetContextThread( thread, context ));
}


/***********************************************************************
 *           SetThreadErrorMode   (kernelbase.@)
 */
BOOL WINAPI SetThreadErrorMode( DWORD mode, DWORD *old )
{
    NTSTATUS status;
    DWORD new = 0;

    if (mode & ~(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (mode & SEM_FAILCRITICALERRORS) new |= 0x10;
    if (mode & SEM_NOGPFAULTERRORBOX) new |= 0x20;
    if (mode & SEM_NOOPENFILEERRORBOX) new |= 0x40;

    status = RtlSetThreadErrorMode( new, old );
    if (!status && old) *old = rtlmode_to_win32mode( *old );
    return set_ntstatus( status );
}


/***********************************************************************
 *           SetThreadGroupAffinity   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadGroupAffinity( HANDLE thread, const GROUP_AFFINITY *new,
                                                      GROUP_AFFINITY *old )
{
    if (old && !GetThreadGroupAffinity( thread, old )) return FALSE;
    return set_ntstatus( NtSetInformationThread( thread, ThreadGroupInformation, new, sizeof(*new) ));
}


/**********************************************************************
 *           SetThreadIdealProcessor   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH SetThreadIdealProcessor( HANDLE thread, DWORD proc )
{
    FIXME( "(%p %u): stub\n", thread, proc );
    if (proc > MAXIMUM_PROCESSORS)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return ~0u;
    }
    return 0;
}


/***********************************************************************
 *           SetThreadIdealProcessorEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadIdealProcessorEx( HANDLE thread, PROCESSOR_NUMBER *ideal,
                                                         PROCESSOR_NUMBER *previous )
{
    FIXME( "(%p %p %p): stub\n", thread, ideal, previous );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/**********************************************************************
 *           SetThreadPriority   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadPriority( HANDLE thread, INT priority )
{
    DWORD prio = priority;
    return set_ntstatus( NtSetInformationThread( thread, ThreadBasePriority, &prio, sizeof(prio) ));
}


/**********************************************************************
 *           SetThreadPriorityBoost   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadPriorityBoost( HANDLE thread, BOOL disable )
{
    return TRUE;
}


/**********************************************************************
 *           SetThreadStackGuarantee   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadStackGuarantee( ULONG *size )
{
    static int once;
    if (once++ == 0) FIXME("(%p): stub\n", size);
    return TRUE;
}


/**********************************************************************
 *           SuspendThread   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH SuspendThread( HANDLE thread )
{
    DWORD ret;

    if (!set_ntstatus( NtSuspendThread( thread, &ret ))) ret = ~0U;
    return ret;
}


/***********************************************************************
 *           SwitchToThread   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SwitchToThread(void)
{
    return (NtYieldExecution() != STATUS_NO_YIELD_PERFORMED);
}


/**********************************************************************
 *           TerminateThread   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH TerminateThread( HANDLE handle, DWORD exit_code )
{
    return set_ntstatus( NtTerminateThread( handle, exit_code ));
}
