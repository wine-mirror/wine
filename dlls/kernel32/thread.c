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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/debug.h"

#include "kernel_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(thread);


/***********************************************************************
 *           CreateThread   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateThread( SECURITY_ATTRIBUTES *sa, SIZE_T stack, LPTHREAD_START_ROUTINE start,
                                              LPVOID param, DWORD flags, LPDWORD id )
{
     return CreateRemoteThread( GetCurrentProcess(),
                                sa, stack, start, param, flags, id );
}


/***************************************************************************
 *                  CreateRemoteThread   (KERNEL32.@)
 *
 * Creates a thread that runs in the address space of another process
 *
 * PARAMS
 *
 * RETURNS
 *   Success: Handle to the new thread.
 *   Failure: NULL. Use GetLastError() to find the error cause.
 *
 * BUGS
 *   Improper memory allocation: there's no ability to free new_thread_info
 *   in other process.
 *   Bad start address for RtlCreateUserThread because the library
 *   may be loaded at different address in other process.
 */
HANDLE WINAPI CreateRemoteThread( HANDLE hProcess, SECURITY_ATTRIBUTES *sa, SIZE_T stack,
                                  LPTHREAD_START_ROUTINE start, LPVOID param,
                                  DWORD flags, LPDWORD id )
{
    HANDLE handle;
    CLIENT_ID client_id;
    NTSTATUS status;
    SIZE_T stack_reserve = 0, stack_commit = 0;

    if (flags & STACK_SIZE_PARAM_IS_A_RESERVATION) stack_reserve = stack;
    else stack_commit = stack;

    status = RtlCreateUserThread( hProcess, NULL, TRUE,
                                  NULL, stack_reserve, stack_commit,
                                  (PRTL_THREAD_START_ROUTINE)start, param, &handle, &client_id );
    if (status == STATUS_SUCCESS)
    {
        if (id) *id = HandleToULong(client_id.UniqueThread);
        if (sa && (sa->nLength >= sizeof(*sa)) && sa->bInheritHandle)
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
 * OpenThread  [KERNEL32.@]   Retrieves a handle to a thread from its thread id
 */
HANDLE WINAPI OpenThread( DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwThreadId )
{
    NTSTATUS status;
    HANDLE handle;
    OBJECT_ATTRIBUTES attr;
    CLIENT_ID cid;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = bInheritHandle ? OBJ_INHERIT : 0;
    attr.ObjectName = NULL;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    cid.UniqueProcess = 0; /* FIXME */
    cid.UniqueThread = ULongToHandle(dwThreadId);
    status = NtOpenThread( &handle, dwDesiredAccess, &attr, &cid );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        handle = 0;
    }
    return handle;
}


/***********************************************************************
 * ExitThread [KERNEL32.@]  Ends a thread
 *
 * RETURNS
 *    None
 */
void WINAPI ExitThread( DWORD code ) /* [in] Exit code for this thread */
{
    RtlExitUserThread( code );
}


/**********************************************************************
 * TerminateThread [KERNEL32.@]  Terminates a thread
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI TerminateThread( HANDLE handle,    /* [in] Handle to thread */
                             DWORD exit_code)  /* [in] Exit code for thread */
{
    NTSTATUS status = NtTerminateThread( handle, exit_code );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *           FreeLibraryAndExitThread (KERNEL32.@)
 */
void WINAPI FreeLibraryAndExitThread(HINSTANCE hLibModule, DWORD dwExitCode)
{
    FreeLibrary(hLibModule);
    ExitThread(dwExitCode);
}


/**********************************************************************
 *		GetExitCodeThread (KERNEL32.@)
 *
 * Gets termination status of thread.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetExitCodeThread(
    HANDLE hthread, /* [in]  Handle to thread */
    LPDWORD exitcode) /* [out] Address to receive termination status */
{
    THREAD_BASIC_INFORMATION info;
    NTSTATUS status = NtQueryInformationThread( hthread, ThreadBasicInformation,
                                                &info, sizeof(info), NULL );

    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    if (exitcode) *exitcode = info.ExitStatus;
    return TRUE;
}


/***********************************************************************
 * SetThreadContext [KERNEL32.@]  Sets context of thread.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetThreadContext( HANDLE handle,           /* [in]  Handle to thread with context */
                              const CONTEXT *context ) /* [in] Address of context structure */
{
    NTSTATUS status = NtSetContextThread( handle, context );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 * GetThreadContext [KERNEL32.@]  Retrieves context of thread.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetThreadContext( HANDLE handle,     /* [in]  Handle to thread with context */
                              CONTEXT *context ) /* [out] Address of context structure */
{
    NTSTATUS status = NtGetContextThread( handle, context );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/**********************************************************************
 * SuspendThread [KERNEL32.@]  Suspends a thread.
 *
 * RETURNS
 *    Success: Previous suspend count
 *    Failure: 0xFFFFFFFF
 */
DWORD WINAPI SuspendThread( HANDLE hthread ) /* [in] Handle to the thread */
{
    DWORD ret;
    NTSTATUS status = NtSuspendThread( hthread, &ret );

    if (status)
    {
        ret = ~0U;
        SetLastError( RtlNtStatusToDosError(status) );
    }
    return ret;
}


/**********************************************************************
 * ResumeThread [KERNEL32.@]  Resumes a thread.
 *
 * Decrements a thread's suspend count.  When count is zero, the
 * execution of the thread is resumed.
 *
 * RETURNS
 *    Success: Previous suspend count
 *    Failure: 0xFFFFFFFF
 *    Already running: 0
 */
DWORD WINAPI ResumeThread( HANDLE hthread ) /* [in] Identifies thread to restart */
{
    DWORD ret;
    NTSTATUS status = NtResumeThread( hthread, &ret );

    if (status)
    {
        ret = ~0U;
        SetLastError( RtlNtStatusToDosError(status) );
    }
    return ret;
}


/**********************************************************************
 * GetThreadPriority [KERNEL32.@]  Returns priority for thread.
 *
 * RETURNS
 *    Success: Thread's priority level.
 *    Failure: THREAD_PRIORITY_ERROR_RETURN
 */
INT WINAPI GetThreadPriority(
    HANDLE hthread) /* [in] Handle to thread */
{
    THREAD_BASIC_INFORMATION info;
    NTSTATUS status = NtQueryInformationThread( hthread, ThreadBasicInformation,
                                                &info, sizeof(info), NULL );

    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return THREAD_PRIORITY_ERROR_RETURN;
    }
    return info.Priority;
}


/**********************************************************************
 * SetThreadPriority [KERNEL32.@]  Sets priority for thread.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetThreadPriority(
    HANDLE hthread, /* [in] Handle to thread */
    INT priority)   /* [in] Thread priority level */
{
    DWORD       prio = priority;
    NTSTATUS    status;

    status = NtSetInformationThread(hthread, ThreadBasePriority,
                                    &prio, sizeof(prio));

    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 * GetThreadPriorityBoost [KERNEL32.@]  Returns priority boost for thread.
 *
 * Always reports that priority boost is disabled.
 *
 * RETURNS
 *    Success: TRUE.
 *    Failure: FALSE
 */
BOOL WINAPI GetThreadPriorityBoost(
    HANDLE hthread, /* [in] Handle to thread */
    PBOOL pstate)   /* [out] pointer to var that receives the boost state */
{
    if (pstate) *pstate = FALSE;
    return TRUE;
}


/**********************************************************************
 * SetThreadPriorityBoost [KERNEL32.@]  Sets priority boost for thread.
 *
 * Priority boost is not implemented, but we return TRUE
 * anyway because some games crash otherwise.
 */
BOOL WINAPI SetThreadPriorityBoost(
    HANDLE hthread, /* [in] Handle to thread */
    BOOL disable)   /* [in] TRUE to disable priority boost */
{
    return TRUE;
}


/**********************************************************************
 *           SetThreadStackGuarantee   (KERNEL32.@)
 */
BOOL WINAPI SetThreadStackGuarantee(PULONG stacksize)
{
    static int once;
    if (once++ == 0)
        FIXME("(%p): stub\n", stacksize);
    return TRUE;
}

/***********************************************************************
 *              GetThreadGroupAffinity (KERNEL32.@)
 */
BOOL WINAPI GetThreadGroupAffinity( HANDLE thread, GROUP_AFFINITY *affinity )
{
    NTSTATUS status;

    if (!affinity)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    status = NtQueryInformationThread( thread, ThreadGroupInformation,
                                       affinity, sizeof(*affinity), NULL );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *              SetThreadGroupAffinity (KERNEL32.@)
 */
BOOL WINAPI SetThreadGroupAffinity( HANDLE thread, const GROUP_AFFINITY *affinity_new,
                                    GROUP_AFFINITY *affinity_old )
{
    NTSTATUS status;

    if (affinity_old && !GetThreadGroupAffinity( thread, affinity_old ))
        return FALSE;

    status = NtSetInformationThread( thread, ThreadGroupInformation,
                                     affinity_new, sizeof(*affinity_new) );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    return TRUE;
}

/**********************************************************************
 *           SetThreadAffinityMask   (KERNEL32.@)
 */
DWORD_PTR WINAPI SetThreadAffinityMask( HANDLE hThread, DWORD_PTR dwThreadAffinityMask )
{
    NTSTATUS                    status;
    THREAD_BASIC_INFORMATION    tbi;

    status = NtQueryInformationThread( hThread, ThreadBasicInformation, 
                                       &tbi, sizeof(tbi), NULL );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    status = NtSetInformationThread( hThread, ThreadAffinityMask, 
                                     &dwThreadAffinityMask,
                                     sizeof(dwThreadAffinityMask));
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    return tbi.AffinityMask;
}


/**********************************************************************
 * SetThreadIdealProcessor [KERNEL32.@]  Sets preferred processor for thread.
 *
 * RETURNS
 *    Success: Value of last call to SetThreadIdealProcessor
 *    Failure: -1
 */
DWORD WINAPI SetThreadIdealProcessor(
    HANDLE hThread,          /* [in] Specifies the thread of interest */
    DWORD dwIdealProcessor)  /* [in] Specifies the new preferred processor */
{
    FIXME("(%p): stub\n",hThread);
    if (dwIdealProcessor > MAXIMUM_PROCESSORS)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return ~0u;
    }
    return 0;
}


/***********************************************************************
 *           GetThreadSelectorEntry   (KERNEL32.@)
 */
BOOL WINAPI GetThreadSelectorEntry( HANDLE hthread, DWORD sel, LPLDT_ENTRY ldtent )
{
    THREAD_DESCRIPTOR_INFORMATION tdi;
    NTSTATUS status;

    tdi.Selector = sel;
    status = NtQueryInformationThread( hthread, ThreadDescriptorTableEntry, &tdi, sizeof(tdi), NULL);
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    *ldtent = tdi.Entry;
    return TRUE;
}


/* callback for QueueUserAPC */
static void CALLBACK call_user_apc( ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3 )
{
    PAPCFUNC func = (PAPCFUNC)arg1;
    func( arg2 );
}

/***********************************************************************
 *              QueueUserAPC  (KERNEL32.@)
 */
DWORD WINAPI QueueUserAPC( PAPCFUNC func, HANDLE hthread, ULONG_PTR data )
{
    NTSTATUS status = NtQueueApcThread( hthread, call_user_apc, (ULONG_PTR)func, data, 0 );

    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/***********************************************************************
 *              QueueUserWorkItem  (KERNEL32.@)
 */
BOOL WINAPI QueueUserWorkItem( LPTHREAD_START_ROUTINE Function, PVOID Context, ULONG Flags )
{
    NTSTATUS status;

    TRACE("(%p,%p,0x%08x)\n", Function, Context, Flags);

    status = RtlQueueWorkItem( Function, Context, Flags );

    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/**********************************************************************
 * GetThreadTimes [KERNEL32.@]  Obtains timing information.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetThreadTimes(
    HANDLE thread,         /* [in]  Specifies the thread of interest */
    LPFILETIME creationtime, /* [out] When the thread was created */
    LPFILETIME exittime,     /* [out] When the thread was destroyed */
    LPFILETIME kerneltime,   /* [out] Time thread spent in kernel mode */
    LPFILETIME usertime)     /* [out] Time thread spent in user mode */
{
    KERNEL_USER_TIMES   kusrt;
    NTSTATUS            status;

    status = NtQueryInformationThread(thread, ThreadTimes, &kusrt,
                                      sizeof(kusrt), NULL);
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    if (creationtime)
    {
        creationtime->dwLowDateTime = kusrt.CreateTime.u.LowPart;
        creationtime->dwHighDateTime = kusrt.CreateTime.u.HighPart;
    }
    if (exittime)
    {
        exittime->dwLowDateTime = kusrt.ExitTime.u.LowPart;
        exittime->dwHighDateTime = kusrt.ExitTime.u.HighPart;
    }
    if (kerneltime)
    {
        kerneltime->dwLowDateTime = kusrt.KernelTime.u.LowPart;
        kerneltime->dwHighDateTime = kusrt.KernelTime.u.HighPart;
    }
    if (usertime)
    {
        usertime->dwLowDateTime = kusrt.UserTime.u.LowPart;
        usertime->dwHighDateTime = kusrt.UserTime.u.HighPart;
    }
    
    return TRUE;
}

/**********************************************************************
 * GetThreadId [KERNEL32.@]
 *
 * Retrieve the identifier of a thread.
 *
 * PARAMS
 *  Thread [I] The thread to retrieve the identifier of.
 *
 * RETURNS
 *    Success: Identifier of the target thread.
 *    Failure: 0
 */
DWORD WINAPI GetThreadId(HANDLE Thread)
{
    THREAD_BASIC_INFORMATION tbi;
    NTSTATUS status;

    TRACE("(%p)\n", Thread);

    status = NtQueryInformationThread(Thread, ThreadBasicInformation, &tbi,
                                      sizeof(tbi), NULL);
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }

    return HandleToULong(tbi.ClientId.UniqueThread);
}

/**********************************************************************
 * GetProcessIdOfThread [KERNEL32.@]
 *
 * Retrieve process identifier given thread belongs to.
 *
 * PARAMS
 *  Thread [I] The thread identifier.
 *
 * RETURNS
 *    Success: Process identifier
 *    Failure: 0
 */
DWORD WINAPI GetProcessIdOfThread(HANDLE Thread)
{
    THREAD_BASIC_INFORMATION tbi;
    NTSTATUS status;

    TRACE("(%p)\n", Thread);

    status = NtQueryInformationThread(Thread, ThreadBasicInformation, &tbi,
                                      sizeof(tbi), NULL);
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }

    return HandleToULong(tbi.ClientId.UniqueProcess);
}

/***********************************************************************
 * GetCurrentThread [KERNEL32.@]  Gets pseudohandle for current thread
 *
 * RETURNS
 *    Pseudohandle for the current thread
 */
#undef GetCurrentThread
HANDLE WINAPI GetCurrentThread(void)
{
    return (HANDLE)~(ULONG_PTR)1;
}


#ifdef __i386__

/***********************************************************************
 *		SetLastError (KERNEL32.@)
 */
/* void WINAPI SetLastError( DWORD error ); */
__ASM_STDCALL_FUNC( SetLastError, 4,
                   "movl 4(%esp),%eax\n\t"
                   ".byte 0x64\n\t"
                   "movl %eax,0x34\n\t"
                   "ret $4" )

/***********************************************************************
 *		GetLastError (KERNEL32.@)
 */
/* DWORD WINAPI GetLastError(void); */
__ASM_STDCALL_FUNC( GetLastError, 0, ".byte 0x64\n\tmovl 0x34,%eax\n\tret" )

/***********************************************************************
 *		GetCurrentProcessId (KERNEL32.@)
 */
/* DWORD WINAPI GetCurrentProcessId(void) */
__ASM_STDCALL_FUNC( GetCurrentProcessId, 0, ".byte 0x64\n\tmovl 0x20,%eax\n\tret" )

/***********************************************************************
 *		GetCurrentThreadId (KERNEL32.@)
 */
/* DWORD WINAPI GetCurrentThreadId(void) */
__ASM_STDCALL_FUNC( GetCurrentThreadId, 0, ".byte 0x64\n\tmovl 0x24,%eax\n\tret" )

/***********************************************************************
 *		GetProcessHeap (KERNEL32.@)
 */
/* HANDLE WINAPI GetProcessHeap(void) */
__ASM_STDCALL_FUNC( GetProcessHeap, 0, ".byte 0x64\n\tmovl 0x30,%eax\n\tmovl 0x18(%eax),%eax\n\tret");

#elif defined(__x86_64__) && !defined(__APPLE__)

/***********************************************************************
 *		SetLastError (KERNEL32.@)
 */
/* void WINAPI SetLastError( DWORD error ); */
__ASM_STDCALL_FUNC( SetLastError, 8, ".byte 0x65\n\tmovl %ecx,0x68\n\tret" );

/***********************************************************************
 *		GetLastError (KERNEL32.@)
 */
/* DWORD WINAPI GetLastError(void); */
__ASM_STDCALL_FUNC( GetLastError, 0, ".byte 0x65\n\tmovl 0x68,%eax\n\tret" );

/***********************************************************************
 *		GetCurrentProcessId (KERNEL32.@)
 */
/* DWORD WINAPI GetCurrentProcessId(void) */
__ASM_STDCALL_FUNC( GetCurrentProcessId, 0, ".byte 0x65\n\tmovl 0x40,%eax\n\tret" );

/***********************************************************************
 *		GetCurrentThreadId (KERNEL32.@)
 */
/* DWORD WINAPI GetCurrentThreadId(void) */
__ASM_STDCALL_FUNC( GetCurrentThreadId, 0, ".byte 0x65\n\tmovl 0x48,%eax\n\tret" );

/***********************************************************************
 *		GetProcessHeap (KERNEL32.@)
 */
/* HANDLE WINAPI GetProcessHeap(void) */
__ASM_STDCALL_FUNC( GetProcessHeap, 0, ".byte 0x65\n\tmovq 0x60,%rax\n\tmovq 0x30(%rax),%rax\n\tret");

#else  /* __x86_64__ */

/**********************************************************************
 *		SetLastError (KERNEL32.@)
 *
 * Sets the last-error code.
 *
 * RETURNS
 * Nothing.
 */
void WINAPI SetLastError( DWORD error ) /* [in] Per-thread error code */
{
    NtCurrentTeb()->LastErrorValue = error;
}

/**********************************************************************
 *              GetLastError (KERNEL32.@)
 *
 * Get the last-error code.
 *
 * RETURNS
 *  last-error code.
 */
DWORD WINAPI GetLastError(void)
{
    return NtCurrentTeb()->LastErrorValue;
}

/***********************************************************************
 *		GetCurrentProcessId (KERNEL32.@)
 *
 * Get the current process identifier.
 *
 * RETURNS
 *  current process identifier
 */
DWORD WINAPI GetCurrentProcessId(void)
{
    return HandleToULong(NtCurrentTeb()->ClientId.UniqueProcess);
}

/***********************************************************************
 *		GetCurrentThreadId (KERNEL32.@)
 *
 * Get the current thread identifier.
 *
 * RETURNS
 *  current thread identifier
 */
DWORD WINAPI GetCurrentThreadId(void)
{
    return HandleToULong(NtCurrentTeb()->ClientId.UniqueThread);
}

/***********************************************************************
 *           GetProcessHeap    (KERNEL32.@)
 */
HANDLE WINAPI GetProcessHeap(void)
{
    return NtCurrentTeb()->Peb->ProcessHeap;
}

#endif  /* __i386__ */

/*************************************************************************
 *              rtlmode_to_win32mode
 */
static DWORD rtlmode_to_win32mode( DWORD rtlmode )
{
    DWORD win32mode = 0;

    if (rtlmode & 0x10)
        win32mode |= SEM_FAILCRITICALERRORS;
    if (rtlmode & 0x20)
        win32mode |= SEM_NOGPFAULTERRORBOX;
    if (rtlmode & 0x40)
        win32mode |= SEM_NOOPENFILEERRORBOX;

    return win32mode;
}

/***********************************************************************
 *              SetThreadErrorMode (KERNEL32.@)
 *
 * Set the thread local error mode.
 *
 * PARAMS
 *  mode    [I] The new error mode, a bitwise or of SEM_FAILCRITICALERRORS,
 *              SEM_NOGPFAULTERRORBOX and SEM_NOOPENFILEERRORBOX.
 *  oldmode [O] Destination of the old error mode (may be NULL)
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE, check GetLastError
 */
BOOL WINAPI SetThreadErrorMode( DWORD mode, LPDWORD oldmode )
{
    NTSTATUS status;
    DWORD tmp = 0;

    if (mode & ~(SEM_FAILCRITICALERRORS |
                 SEM_NOGPFAULTERRORBOX |
                 SEM_NOOPENFILEERRORBOX))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (mode & SEM_FAILCRITICALERRORS)
        tmp |= 0x10;
    if (mode & SEM_NOGPFAULTERRORBOX)
        tmp |= 0x20;
    if (mode & SEM_NOOPENFILEERRORBOX)
        tmp |= 0x40;

    status = RtlSetThreadErrorMode( tmp, oldmode );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    if (oldmode)
        *oldmode = rtlmode_to_win32mode(*oldmode);

    return TRUE;
}

/***********************************************************************
 *              GetThreadErrorMode (KERNEL32.@)
 *
 * Get the thread local error mode.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The current thread local error mode.
 */
DWORD WINAPI GetThreadErrorMode( void )
{
    return rtlmode_to_win32mode( RtlGetThreadErrorMode() );
}

/***********************************************************************
 *              GetThreadUILanguage (KERNEL32.@)
 *
 * Get the current thread's language identifier.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The current thread's language identifier.
 */
LANGID WINAPI GetThreadUILanguage( void )
{
    LANGID lang;
    NtQueryDefaultUILanguage( &lang );
    FIXME(": stub, returning default language.\n");
    return lang;
}

/***********************************************************************
 *              GetThreadIOPendingFlag (KERNEL32.@)
 */
BOOL WINAPI GetThreadIOPendingFlag( HANDLE thread, PBOOL io_pending )
{
    FIXME("%p, %p\n", thread, io_pending);
    *io_pending = FALSE;
    return TRUE;
}

/***********************************************************************
 *              SetThreadPreferredUILanguages (KERNEL32.@)
 */
BOOL WINAPI SetThreadPreferredUILanguages( DWORD flags, PCZZWSTR buffer, PULONG count )
{
    FIXME( "%u, %p, %p\n", flags, buffer, count );
    return TRUE;
}

/***********************************************************************
 *              GetThreadPreferredUILanguages (KERNEL32.@)
 */
BOOL WINAPI GetThreadPreferredUILanguages( DWORD flags, PULONG count, PCZZWSTR buffer, PULONG buffersize )
{
    FIXME( "%u, %p, %p %p\n", flags, count, buffer, buffersize );
    *count = 0;
    *buffersize = 0;
    return TRUE;
}

/***********************************************************************
 *              CallbackMayRunLong (KERNEL32.@)
 */
BOOL WINAPI CallbackMayRunLong( TP_CALLBACK_INSTANCE *instance )
{
    NTSTATUS status;

    TRACE( "%p\n", instance );

    status = TpCallbackMayRunLong( instance );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *              CreateThreadpool (KERNEL32.@)
 */
PTP_POOL WINAPI CreateThreadpool( PVOID reserved )
{
    TP_POOL *pool;
    NTSTATUS status;

    TRACE( "%p\n", reserved );

    status = TpAllocPool( &pool, reserved );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    return pool;
}

/***********************************************************************
 *              CreateThreadpoolCleanupGroup (KERNEL32.@)
 */
PTP_CLEANUP_GROUP WINAPI CreateThreadpoolCleanupGroup( void )
{
    TP_CLEANUP_GROUP *group;
    NTSTATUS status;

    TRACE( "\n" );

    status = TpAllocCleanupGroup( &group );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    return group;
}

/***********************************************************************
 *              CreateThreadpoolTimer (KERNEL32.@)
 */
PTP_TIMER WINAPI CreateThreadpoolTimer( PTP_TIMER_CALLBACK callback, PVOID userdata,
                                        TP_CALLBACK_ENVIRON *environment )
{
    TP_TIMER *timer;
    NTSTATUS status;

    TRACE( "%p, %p, %p\n", callback, userdata, environment );

    status = TpAllocTimer( &timer, callback, userdata, environment );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    return timer;
}

/***********************************************************************
 *              CreateThreadpoolWait (KERNEL32.@)
 */
PTP_WAIT WINAPI CreateThreadpoolWait( PTP_WAIT_CALLBACK callback, PVOID userdata,
                                      TP_CALLBACK_ENVIRON *environment )
{
    TP_WAIT *wait;
    NTSTATUS status;

    TRACE( "%p, %p, %p\n", callback, userdata, environment );

    status = TpAllocWait( &wait, callback, userdata, environment );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    return wait;
}

/***********************************************************************
 *              CreateThreadpoolWork (KERNEL32.@)
 */
PTP_WORK WINAPI CreateThreadpoolWork( PTP_WORK_CALLBACK callback, PVOID userdata,
                                      TP_CALLBACK_ENVIRON *environment )
{
    TP_WORK *work;
    NTSTATUS status;

    TRACE( "%p, %p, %p\n", callback, userdata, environment );

    status = TpAllocWork( &work, callback, userdata, environment );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    return work;
}

/***********************************************************************
 *              SetThreadpoolTimer (KERNEL32.@)
 */
VOID WINAPI SetThreadpoolTimer( TP_TIMER *timer, FILETIME *due_time,
                                DWORD period, DWORD window_length )
{
    LARGE_INTEGER timeout;

    TRACE( "%p, %p, %u, %u\n", timer, due_time, period, window_length );

    if (due_time)
    {
        timeout.u.LowPart = due_time->dwLowDateTime;
        timeout.u.HighPart = due_time->dwHighDateTime;
    }

    TpSetTimer( timer, due_time ? &timeout : NULL, period, window_length );
}

/***********************************************************************
 *              SetThreadpoolWait (KERNEL32.@)
 */
VOID WINAPI SetThreadpoolWait( TP_WAIT *wait, HANDLE handle, FILETIME *due_time )
{
    LARGE_INTEGER timeout;

    TRACE( "%p, %p, %p\n", wait, handle, due_time );

    if (!handle)
    {
        due_time = NULL;
    }
    else if (due_time)
    {
        timeout.u.LowPart = due_time->dwLowDateTime;
        timeout.u.HighPart = due_time->dwHighDateTime;
    }

    TpSetWait( wait, handle, due_time ? &timeout : NULL );
}

/***********************************************************************
 *              TrySubmitThreadpoolCallback (KERNEL32.@)
 */
BOOL WINAPI TrySubmitThreadpoolCallback( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                         TP_CALLBACK_ENVIRON *environment )
{
    NTSTATUS status;

    TRACE( "%p, %p, %p\n", callback, userdata, environment );

    status = TpSimpleTryPost( callback, userdata, environment );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    return TRUE;
}
