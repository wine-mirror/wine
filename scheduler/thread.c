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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "wine/winbase16.h"
#include "thread.h"
#include "module.h"
#include "winerror.h"
#include "selectors.h"
#include "winnt.h"
#include "wine/server.h"
#include "stackframe.h"
#include "wine/debug.h"
#include "winnls.h"

WINE_DEFAULT_DEBUG_CHANNEL(thread);
WINE_DECLARE_DEBUG_CHANNEL(relay);

/* TEB of the initial thread */
static TEB initial_teb;

extern struct _PDB current_process;

/***********************************************************************
 *           THREAD_InitTEB
 *
 * Initialization of a newly created TEB.
 */
static BOOL THREAD_InitTEB( TEB *teb )
{
    teb->except    = (void *)~0UL;
    teb->self      = teb;
    teb->tibflags  = TEBF_WIN32;
    teb->exit_code = STILL_ACTIVE;
    teb->request_fd = -1;
    teb->reply_fd   = -1;
    teb->wait_fd[0] = -1;
    teb->wait_fd[1] = -1;
    teb->stack_top  = (void *)~0UL;
    teb->StaticUnicodeString.MaximumLength = sizeof(teb->StaticUnicodeBuffer);
    teb->StaticUnicodeString.Buffer = (PWSTR)teb->StaticUnicodeBuffer;
    teb->teb_sel = wine_ldt_alloc_fs();
    return (teb->teb_sel != 0);
}


/***********************************************************************
 *           THREAD_FreeTEB
 *
 * Free data structures associated with a thread.
 * Must be called from the context of another thread.
 */
static void THREAD_FreeTEB( TEB *teb )
{
    TRACE("(%p) called\n", teb );
    /* Free the associated memory */
    wine_ldt_free_fs( teb->teb_sel );
    VirtualFree( teb->stack_base, 0, MEM_RELEASE );
}


/***********************************************************************
 *           THREAD_InitStack
 *
 * Allocate the stack of a thread.
 */
TEB *THREAD_InitStack( TEB *teb, DWORD stack_size )
{
    DWORD old_prot, total_size;
    DWORD page_size = getpagesize();
    void *base;

    /* Allocate the stack */

    if (stack_size >= 16*1024*1024)
    	WARN("Thread stack size is %ld MB.\n",stack_size/1024/1024);

    /* if size is smaller than default, get stack size from parent */
    if (stack_size < 1024 * 1024)
    {
        if (teb)
            stack_size = 1024 * 1024;  /* no parent */
        else
            stack_size = ((char *)NtCurrentTeb()->stack_top - (char *)NtCurrentTeb()->stack_base
                          - SIGNAL_STACK_SIZE - 3 * page_size);
    }

    /* FIXME: some Wine functions use a lot of stack, so we add 64Kb here */
    stack_size += 64 * 1024;

    /* Memory layout in allocated block:
     *
     *   size                 contents
     * 1 page              NOACCESS guard page
     * SIGNAL_STACK_SIZE   signal stack
     * 1 page              NOACCESS guard page
     * 1 page              PAGE_GUARD guard page
     * stack_size          normal stack
     * 1 page              TEB (except for initial thread)
     */

    stack_size = (stack_size + (page_size - 1)) & ~(page_size - 1);
    total_size = stack_size + SIGNAL_STACK_SIZE + 3 * page_size;
    if (!teb) total_size += page_size;

    if (!(base = VirtualAlloc( NULL, total_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE )))
        return NULL;

    if (!teb)
    {
        teb = (TEB *)((char *)base + total_size - page_size);
        if (!THREAD_InitTEB( teb ))
        {
            VirtualFree( base, 0, MEM_RELEASE );
            return NULL;
        }
    }

    teb->stack_low    = base;
    teb->stack_base   = base;
    teb->signal_stack = (char *)base + page_size;
    teb->stack_top    = (char *)base + 3 * page_size + SIGNAL_STACK_SIZE + stack_size;

    /* Setup guard pages */

    VirtualProtect( base, 1, PAGE_NOACCESS, &old_prot );
    VirtualProtect( (char *)teb->signal_stack + SIGNAL_STACK_SIZE, 1, PAGE_NOACCESS, &old_prot );
    VirtualProtect( (char *)teb->signal_stack + SIGNAL_STACK_SIZE + page_size, 1,
                    PAGE_EXECUTE_READWRITE | PAGE_GUARD, &old_prot );
    return teb;
}


/***********************************************************************
 *           THREAD_Init
 *
 * Setup the initial thread.
 *
 * NOTES: The first allocated TEB on NT is at 0x7ffde000.
 */
void THREAD_Init(void)
{
    static struct debug_info info;  /* debug info for initial thread */

    if (!initial_teb.self)  /* do it only once */
    {
        THREAD_InitTEB( &initial_teb );
        assert( initial_teb.teb_sel );
        info.str_pos = info.strings;
        info.out_pos = info.output;
        initial_teb.debug_info = &info;
        initial_teb.Peb = (PEB *)&current_process;  /* FIXME */
        SYSDEPS_SetCurThread( &initial_teb );
    }
}

DECL_GLOBAL_CONSTRUCTOR(thread_init) { THREAD_Init(); }


/***********************************************************************
 *           THREAD_Start
 *
 * Start execution of a newly created thread. Does not return.
 */
static void THREAD_Start( TEB *teb )
{
    LPTHREAD_START_ROUTINE func = (LPTHREAD_START_ROUTINE)teb->entry_point;
    struct debug_info info;

    info.str_pos = info.strings;
    info.out_pos = info.output;
    teb->debug_info = &info;

    SYSDEPS_SetCurThread( teb );
    SIGNAL_Init();
    CLIENT_InitThread();

    if (TRACE_ON(relay))
        DPRINTF("%04lx:Starting thread (entryproc=%p)\n", GetCurrentThreadId(), func );

    __TRY
    {
        MODULE_DllThreadAttach( NULL );
        ExitThread( func( NtCurrentTeb()->entry_arg ) );
    }
    __EXCEPT(UnhandledExceptionFilter)
    {
        TerminateThread( GetCurrentThread(), GetExceptionCode() );
    }
    __ENDTRY
}


/***********************************************************************
 *           CreateThread   (KERNEL32.@)
 */
HANDLE WINAPI CreateThread( SECURITY_ATTRIBUTES *sa, SIZE_T stack,
                            LPTHREAD_START_ROUTINE start, LPVOID param,
                            DWORD flags, LPDWORD id )
{
    HANDLE handle = 0;
    TEB *teb;
    DWORD tid = 0;
    int request_pipe[2];

    if (pipe( request_pipe ) == -1)
    {
        SetLastError( ERROR_TOO_MANY_OPEN_FILES );
        return 0;
    }
    fcntl( request_pipe[1], F_SETFD, 1 ); /* set close on exec flag */
    wine_server_send_fd( request_pipe[0] );

    SERVER_START_REQ( new_thread )
    {
        req->suspend    = ((flags & CREATE_SUSPENDED) != 0);
        req->inherit    = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        req->request_fd = request_pipe[0];
        if (!wine_server_call_err( req ))
        {
            handle = reply->handle;
            tid = reply->tid;
        }
        close( request_pipe[0] );
    }
    SERVER_END_REQ;

    if (!handle || !(teb = THREAD_InitStack( NULL, stack )))
    {
        close( request_pipe[1] );
        return 0;
    }

    teb->Peb         = NtCurrentTeb()->Peb;
    teb->tid         = tid;
    teb->request_fd  = request_pipe[1];
    teb->entry_point = start;
    teb->entry_arg   = param;
    teb->htask16     = GetCurrentTask();

    if (id) *id = tid;
    if (SYSDEPS_SpawnThread( THREAD_Start, teb ) == -1)
    {
        CloseHandle( handle );
        close( request_pipe[1] );
        THREAD_FreeTEB( teb );
        return 0;
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
    BOOL last;
    SERVER_START_REQ( terminate_thread )
    {
        /* send the exit code to the server */
        req->handle    = GetCurrentThread();
        req->exit_code = code;
        wine_server_call( req );
        last = reply->last;
    }
    SERVER_END_REQ;

    if (last)
    {
        LdrShutdownProcess();
        exit( code );
    }
    else
    {
        LdrShutdownThread();
        SYSDEPS_ExitThread( code );
    }
}

/***********************************************************************
 * OpenThread Retrieves a handle to a thread from its thread id
 *
 * RETURNS
 *    None
 */
HANDLE WINAPI OpenThread( DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwThreadId )
{
    HANDLE ret = 0;
    SERVER_START_REQ( open_thread )
    {
        req->tid     = dwThreadId;
        req->access  = dwDesiredAccess;
        req->inherit = bInheritHandle;
        if (!wine_server_call_err( req )) ret = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
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
 * ProcessIdToSessionId   (KERNEL32.@)
 * This function is available on Terminal Server 4SP4 and Windows 2000
 */
BOOL WINAPI ProcessIdToSessionId( DWORD procid, DWORD *sessionid_ptr )
{
	/* According to MSDN, if the calling process is not in a terminal
	 * services environment, then the sessionid returned is zero.
	 */
	*sessionid_ptr = 0;
	return TRUE;
}


#ifdef __i386__

/***********************************************************************
 *		SetLastError (KERNEL.147)
 *		SetLastError (KERNEL32.@)
 */
/* void WINAPI SetLastError( DWORD error ); */
__ASM_GLOBAL_FUNC( SetLastError,
                   "movl 4(%esp),%eax\n\t"
                   ".byte 0x64\n\t"
                   "movl %eax,0x60\n\t"
                   "ret $4" );

/***********************************************************************
 *		GetLastError (KERNEL.148)
 *		GetLastError (KERNEL32.@)
 */
/* DWORD WINAPI GetLastError(void); */
__ASM_GLOBAL_FUNC( GetLastError, ".byte 0x64\n\tmovl 0x60,%eax\n\tret" );

/***********************************************************************
 *		GetCurrentProcessId (KERNEL.471)
 *		GetCurrentProcessId (KERNEL32.@)
 */
/* DWORD WINAPI GetCurrentProcessId(void) */
__ASM_GLOBAL_FUNC( GetCurrentProcessId, ".byte 0x64\n\tmovl 0x20,%eax\n\tret" );

/***********************************************************************
 *		GetCurrentThreadId (KERNEL.462)
 *		GetCurrentThreadId (KERNEL32.@)
 */
/* DWORD WINAPI GetCurrentThreadId(void) */
__ASM_GLOBAL_FUNC( GetCurrentThreadId, ".byte 0x64\n\tmovl 0x24,%eax\n\tret" );

#else  /* __i386__ */

/**********************************************************************
 *		SetLastError (KERNEL.147)
 *		SetLastError (KERNEL32.@)
 *
 * Sets the last-error code.
 */
void WINAPI SetLastError( DWORD error ) /* [in] Per-thread error code */
{
    NtCurrentTeb()->last_error = error;
}

/**********************************************************************
 *		GetLastError (KERNEL.148)
 *              GetLastError (KERNEL32.@)
 *
 * Returns last-error code.
 */
DWORD WINAPI GetLastError(void)
{
    return NtCurrentTeb()->last_error;
}

/***********************************************************************
 *		GetCurrentProcessId (KERNEL.471)
 *		GetCurrentProcessId (KERNEL32.@)
 *
 * Returns process identifier.
 */
DWORD WINAPI GetCurrentProcessId(void)
{
    return (DWORD)NtCurrentTeb()->pid;
}

/***********************************************************************
 *		GetCurrentThreadId (KERNEL.462)
 *		GetCurrentThreadId (KERNEL32.@)
 *
 * Returns thread identifier.
 */
DWORD WINAPI GetCurrentThreadId(void)
{
    return NtCurrentTeb()->tid;
}

#endif  /* __i386__ */
