/*
 * Win32 threads
 *
 * Copyright 1996 Alexandre Julliard
 */

#include "config.h"

#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <unistd.h>
#include "wine/winbase16.h"
#include "thread.h"
#include "process.h"
#include "task.h"
#include "module.h"
#include "user.h"
#include "winerror.h"
#include "heap.h"
#include "selectors.h"
#include "winnt.h"
#include "server.h"
#include "services.h"
#include "stackframe.h"
#include "builtin16.h"
#include "debugtools.h"
#include "queue.h"
#include "hook.h"

DEFAULT_DEBUG_CHANNEL(thread)

/* TEB of the initial thread */
static TEB initial_teb;

/* Global thread list (FIXME: not thread-safe) */
TEB *THREAD_First = &initial_teb;

/***********************************************************************
 *           THREAD_IsWin16
 */
BOOL THREAD_IsWin16( TEB *teb )
{
    return !teb || !(teb->flags & TEBF_WIN32);
}

/***********************************************************************
 *           THREAD_IdToTEB
 *
 * Convert a thread id to a TEB, making sure it is valid.
 */
TEB *THREAD_IdToTEB( DWORD id )
{
    TEB *teb = THREAD_First;

    if (!id) return NtCurrentTeb();
    while (teb)
    {
        if ((DWORD)teb->tid == id) return teb;
        teb = teb->next;
    }
    /* Allow task handles to be used; convert to main thread */
    if ( IsTask16( id ) )
    {
        TDB *pTask = (TDB *)GlobalLock16( id );
        if (pTask) return pTask->teb;
    }
    SetLastError( ERROR_INVALID_PARAMETER );
    return NULL;
}


/***********************************************************************
 *           THREAD_InitTEB
 *
 * Initialization of a newly created TEB.
 */
static BOOL THREAD_InitTEB( TEB *teb, DWORD stack_size, BOOL alloc_stack16,
                            LPSECURITY_ATTRIBUTES sa )
{
    DWORD old_prot;

    /* Allocate the stack */

    /* FIXME:
     * If stacksize smaller than 1 MB, allocate 1MB 
     * (one program wanted only 10 kB, which is recommendable, but some WINE
     *  functions, noteably in the files subdir, push HUGE structures and
     *  arrays on the stack. They probably shouldn't.)
     * If stacksize larger than 16 MB, warn the user. (We could shrink the stack
     * but this could give more or less unexplainable crashes.)
     */
    if (stack_size<1024*1024)
    	stack_size = 1024 * 1024;
    if (stack_size >= 16*1024*1024)
    	WARN("Thread stack size is %ld MB.\n",stack_size/1024/1024);
    teb->stack_base = VirtualAlloc(NULL, stack_size + SIGNAL_STACK_SIZE +
                                   (alloc_stack16 ? 0x10000 : 0),
                                   MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    if (!teb->stack_base) goto error;
    /* Set a guard page at the bottom of the stack */
    VirtualProtect( teb->stack_base, 1, PAGE_EXECUTE_READWRITE | PAGE_GUARD, &old_prot );
    teb->stack_top    = (char *)teb->stack_base + stack_size;
    teb->stack_low    = teb->stack_base;
    teb->signal_stack = teb->stack_top;  /* start of signal stack */

    /* Allocate the 16-bit stack selector */

    if (alloc_stack16)
    {
        teb->stack_sel = SELECTOR_AllocBlock( teb->stack_top, 0x10000, SEGMENT_DATA,
                                              FALSE, FALSE );
        if (!teb->stack_sel) goto error;
        teb->cur_stack = PTR_SEG_OFF_TO_SEGPTR( teb->stack_sel, 
                                                0x10000 - sizeof(STACK16FRAME) );
        teb->signal_stack = (char *)teb->signal_stack + 0x10000;
    }

    /* Create the thread event */

    if (!(teb->event = CreateEventA( NULL, FALSE, FALSE, NULL ))) goto error;
    teb->event = ConvertToGlobalHandle( teb->event );
    return TRUE;

error:
    if (teb->event) CloseHandle( teb->event );
    if (teb->stack_sel) SELECTOR_FreeBlock( teb->stack_sel, 1 );
    if (teb->stack_base) VirtualFree( teb->stack_base, 0, MEM_RELEASE );
    return FALSE;
}


/***********************************************************************
 *           THREAD_FreeTEB
 *
 * Free data structures associated with a thread.
 * Must be called from the context of another thread.
 */
void CALLBACK THREAD_FreeTEB( ULONG_PTR arg )
{
    TEB *teb = (TEB *)arg;
    TEB **pptr = &THREAD_First;

    TRACE("(%p) called\n", teb );
    SERVICE_Delete( teb->cleanup );

    PROCESS_CallUserSignalProc( USIG_THREAD_EXIT, (DWORD)teb->tid, 0 );
    
    CloseHandle( teb->event );
    while (*pptr && (*pptr != teb)) pptr = &(*pptr)->next;
    if (*pptr) *pptr = teb->next;

    /* Free the associated memory */

    if (teb->stack_sel) SELECTOR_FreeBlock( teb->stack_sel, 1 );
    SELECTOR_FreeBlock( teb->teb_sel, 1 );
    close( teb->socket );
    if (teb->buffer) munmap( teb->buffer, teb->buffer_size );
    VirtualFree( teb->stack_base, 0, MEM_RELEASE );
    HeapFree( SystemHeap, 0, teb );
}


/***********************************************************************
 *           THREAD_CreateInitialThread
 *
 * Create the initial thread.
 */
TEB *THREAD_CreateInitialThread( PDB *pdb, int server_fd )
{
    initial_teb.except      = (void *)-1;
    initial_teb.self        = &initial_teb;
    initial_teb.flags       = /* TEBF_WIN32 */ 0;
    initial_teb.tls_ptr     = initial_teb.tls_array;
    initial_teb.process     = pdb;
    initial_teb.exit_code   = 0x103; /* STILL_ACTIVE */
    initial_teb.socket      = server_fd;

    /* Allocate the TEB selector (%fs register) */

    if (!(initial_teb.teb_sel = SELECTOR_AllocBlock( &initial_teb, 0x1000,
                                                     SEGMENT_DATA, TRUE, FALSE )))
    {
        MESSAGE("Could not allocate fs register for initial thread\n" );
        return NULL;
    }
    SYSDEPS_SetCurThread( &initial_teb );

    /* Now proceed with normal initialization */

    if (CLIENT_InitThread()) return NULL;
    if (!THREAD_InitTEB( &initial_teb, 0, TRUE, NULL )) return NULL;
    return &initial_teb;
}


/***********************************************************************
 *           THREAD_Create
 */
TEB *THREAD_Create( PDB *pdb, DWORD flags, DWORD stack_size, BOOL alloc_stack16,
                     LPSECURITY_ATTRIBUTES sa, int *server_handle )
{
    struct new_thread_request *req = get_req_buffer();
    int fd[2];
    HANDLE cleanup_object;

    TEB *teb = HeapAlloc( SystemHeap, HEAP_ZERO_MEMORY, sizeof(TEB) );
    if (!teb) return NULL;
    teb->except      = (void *)-1;
    teb->htask16     = pdb->task;
    teb->self        = teb;
    teb->flags       = (pdb->flags & PDB32_WIN16_PROC)? 0 : TEBF_WIN32;
    teb->tls_ptr     = teb->tls_array;
    teb->process     = pdb;
    teb->exit_code   = 0x103; /* STILL_ACTIVE */
    teb->socket      = -1;

    /* Allocate the TEB selector (%fs register) */

    *server_handle = -1;
    teb->teb_sel = SELECTOR_AllocBlock( teb, 0x1000, SEGMENT_DATA, TRUE, FALSE );
    if (!teb->teb_sel) goto error;

    /* Create the socket pair for server communication */

    if (socketpair( AF_UNIX, SOCK_STREAM, 0, fd ) == -1)
    {
        SetLastError( ERROR_TOO_MANY_OPEN_FILES );  /* FIXME */
        goto error;
    }
    teb->socket = fd[0];
    fcntl( fd[0], F_SETFD, 1 ); /* set close on exec flag */

    /* Create the thread on the server side */

    req->pid     = teb->process->server_pid;
    req->suspend = ((flags & CREATE_SUSPENDED) != 0);
    req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    if (server_call_fd( REQ_NEW_THREAD, fd[1], NULL )) goto error;
    teb->tid = req->tid;
    *server_handle = req->handle;

    /* Do the rest of the initialization */

    if (!THREAD_InitTEB( teb, stack_size, alloc_stack16, sa )) goto error;
    teb->next = THREAD_First;
    THREAD_First = teb;

    /* Install cleanup handler */
    if ( !DuplicateHandle( GetCurrentProcess(), *server_handle, 
			   GetCurrentProcess(), &cleanup_object, 
			   0, FALSE, DUPLICATE_SAME_ACCESS ) ) goto error;
    teb->cleanup = SERVICE_AddObject( cleanup_object, THREAD_FreeTEB, (ULONG_PTR)teb );

    return teb;

error:
    if (*server_handle != -1) CloseHandle( *server_handle );
    if (teb->teb_sel) SELECTOR_FreeBlock( teb->teb_sel, 1 );
    if (teb->socket != -1) close( teb->socket );
    HeapFree( SystemHeap, 0, teb );
    return NULL;
}


/***********************************************************************
 *           THREAD_Start
 *
 * Start execution of a newly created thread. Does not return.
 */
static void THREAD_Start(void)
{
    LPTHREAD_START_ROUTINE func = (LPTHREAD_START_ROUTINE)NtCurrentTeb()->entry_point;
    PROCESS_CallUserSignalProc( USIG_THREAD_INIT, (DWORD)NtCurrentTeb()->tid, 0 );
    PE_InitTls();
    MODULE_DllThreadAttach( NULL );

    if (NtCurrentTeb()->process->flags & PDB32_DEBUGGED) DEBUG_SendCreateThreadEvent( func );

    ExitThread( func( NtCurrentTeb()->entry_arg ) );
}


/***********************************************************************
 *           CreateThread   (KERNEL32.63)
 */
HANDLE WINAPI CreateThread( SECURITY_ATTRIBUTES *sa, DWORD stack,
                            LPTHREAD_START_ROUTINE start, LPVOID param,
                            DWORD flags, LPDWORD id )
{
    int handle = -1;
    TEB *teb = THREAD_Create( PROCESS_Current(), flags, stack, TRUE, sa, &handle );
    if (!teb) return 0;
    teb->flags      |= TEBF_WIN32;
    teb->entry_point = start;
    teb->entry_arg   = param;
    teb->startup     = THREAD_Start;
    if (SYSDEPS_SpawnThread( teb ) == -1)
    {
        CloseHandle( handle );
        return 0;
    }
    if (id) *id = (DWORD)teb->tid;
    return handle;
}

/***********************************************************************
 *           CreateThread16   (KERNEL.441)
 */
static DWORD CALLBACK THREAD_StartThread16( LPVOID threadArgs )
{
    FARPROC16 start = ((FARPROC16 *)threadArgs)[0];
    DWORD     param = ((DWORD *)threadArgs)[1];
    HeapFree( GetProcessHeap(), 0, threadArgs );

    ((LPDWORD)CURRENT_STACK16)[-1] = param;
    return CallTo16Long( start, sizeof(DWORD) );
}
HANDLE WINAPI CreateThread16( SECURITY_ATTRIBUTES *sa, DWORD stack,
                              FARPROC16 start, SEGPTR param,
                              DWORD flags, LPDWORD id )
{
    DWORD *threadArgs = HeapAlloc( GetProcessHeap(), 0, 2*sizeof(DWORD) );
    if (!threadArgs) return INVALID_HANDLE_VALUE;
    threadArgs[0] = (DWORD)start;
    threadArgs[1] = (DWORD)param;

    return CreateThread( sa, stack, THREAD_StartThread16, threadArgs, flags, id );
}


/***********************************************************************
 * ExitThread [KERNEL32.215]  Ends a thread
 *
 * RETURNS
 *    None
 */
void WINAPI ExitThread( DWORD code ) /* [in] Exit code for this thread */
{
    MODULE_DllThreadDetach( NULL );
    TerminateThread( GetCurrentThread(), code );
}


/***********************************************************************
 * GetCurrentThread [KERNEL32.200]  Gets pseudohandle for current thread
 *
 * RETURNS
 *    Pseudohandle for the current thread
 */
HANDLE WINAPI GetCurrentThread(void)
{
    return CURRENT_THREAD_PSEUDOHANDLE;
}


/**********************************************************************
 * GetLastError [KERNEL.148] [KERNEL32.227]  Returns last-error code.
 *
 * RETURNS
 *     Calling thread's last error code value.
 */
DWORD WINAPI GetLastError(void)
{
    return NtCurrentTeb()->last_error;
}


/**********************************************************************
 * SetLastErrorEx [USER32.485]  Sets the last-error code.
 *
 * RETURNS
 *    None.
 */
void WINAPI SetLastErrorEx(
    DWORD error, /* [in] Per-thread error code */
    DWORD type)  /* [in] Error type */
{
    TRACE("(0x%08lx, 0x%08lx)\n", error,type);
    switch(type) {
        case 0:
            break;
        case SLE_ERROR:
        case SLE_MINORERROR:
        case SLE_WARNING:
            /* Fall through for now */
        default:
            FIXME("(error=%08lx, type=%08lx): Unhandled type\n", error,type);
            break;
    }
    SetLastError( error );
}


/**********************************************************************
 * TlsAlloc [KERNEL32.530]  Allocates a TLS index.
 *
 * Allocates a thread local storage index
 *
 * RETURNS
 *    Success: TLS Index
 *    Failure: 0xFFFFFFFF
 */
DWORD WINAPI TlsAlloc( void )
{
    PDB *process = PROCESS_Current();
    DWORD i, mask, ret = 0;
    DWORD *bits = process->tls_bits;
    EnterCriticalSection( &process->crit_section );
    if (*bits == 0xffffffff)
    {
        bits++;
        ret = 32;
        if (*bits == 0xffffffff)
        {
            LeaveCriticalSection( &process->crit_section );
            SetLastError( ERROR_NO_MORE_ITEMS );
            return 0xffffffff;
        }
    }
    for (i = 0, mask = 1; i < 32; i++, mask <<= 1) if (!(*bits & mask)) break;
    *bits |= mask;
    LeaveCriticalSection( &process->crit_section );
    return ret + i;
}


/**********************************************************************
 * TlsFree [KERNEL32.531]  Releases a TLS index.
 *
 * Releases a thread local storage index, making it available for reuse
 * 
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI TlsFree(
    DWORD index) /* [in] TLS Index to free */
{
    PDB *process = PROCESS_Current();
    DWORD mask;
    DWORD *bits = process->tls_bits;
    if (index >= 64)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    EnterCriticalSection( &process->crit_section );
    if (index >= 32) bits++;
    mask = (1 << (index & 31));
    if (!(*bits & mask))  /* already free? */
    {
        LeaveCriticalSection( &process->crit_section );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    *bits &= ~mask;
    NtCurrentTeb()->tls_array[index] = 0;
    /* FIXME: should zero all other thread values */
    LeaveCriticalSection( &process->crit_section );
    return TRUE;
}


/**********************************************************************
 * TlsGetValue [KERNEL32.532]  Gets value in a thread's TLS slot
 *
 * RETURNS
 *    Success: Value stored in calling thread's TLS slot for index
 *    Failure: 0 and GetLastError returns NO_ERROR
 */
LPVOID WINAPI TlsGetValue(
    DWORD index) /* [in] TLS index to retrieve value for */
{
    if (index >= 64)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    SetLastError( ERROR_SUCCESS );
    return NtCurrentTeb()->tls_array[index];
}


/**********************************************************************
 * TlsSetValue [KERNEL32.533]  Stores a value in the thread's TLS slot.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI TlsSetValue(
    DWORD index,  /* [in] TLS index to set value for */
    LPVOID value) /* [in] Value to be stored */
{
    if (index >= 64)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    NtCurrentTeb()->tls_array[index] = value;
    return TRUE;
}


/***********************************************************************
 * SetThreadContext [KERNEL32.670]  Sets context of thread.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetThreadContext(
    HANDLE handle,  /* [in]  Handle to thread with context */
    const CONTEXT *context) /* [out] Address of context structure */
{
    FIXME("not implemented\n" );
    return TRUE;
}

/***********************************************************************
 * GetThreadContext [KERNEL32.294]  Retrieves context of thread.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetThreadContext(
    HANDLE handle,  /* [in]  Handle to thread with context */
    CONTEXT *context) /* [out] Address of context structure */
{
#ifdef __i386__
    WORD cs, ds;

    FIXME("returning dummy info\n" );

    /* make up some plausible values for segment registers */
    GET_CS(cs);
    GET_DS(ds);
    context->SegCs   = cs;
    context->SegDs   = ds;
    context->SegEs   = ds;
    context->SegGs   = ds;
    context->SegSs   = ds;
    context->SegFs   = ds;
#endif
    return TRUE;
}


/**********************************************************************
 * GetThreadPriority [KERNEL32.296]  Returns priority for thread.
 *
 * RETURNS
 *    Success: Thread's priority level.
 *    Failure: THREAD_PRIORITY_ERROR_RETURN
 */
INT WINAPI GetThreadPriority(
    HANDLE hthread) /* [in] Handle to thread */
{
    INT ret = THREAD_PRIORITY_ERROR_RETURN;
    struct get_thread_info_request *req = get_req_buffer();
    req->handle = hthread;
    if (!server_call( REQ_GET_THREAD_INFO )) ret = req->priority;
    return ret;
}


/**********************************************************************
 * SetThreadPriority [KERNEL32.514]  Sets priority for thread.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetThreadPriority(
    HANDLE hthread, /* [in] Handle to thread */
    INT priority)   /* [in] Thread priority level */
{
    struct set_thread_info_request *req = get_req_buffer();
    req->handle   = hthread;
    req->priority = priority;
    req->mask     = SET_THREAD_INFO_PRIORITY;
    return !server_call( REQ_SET_THREAD_INFO );
}


/**********************************************************************
 *           SetThreadAffinityMask   (KERNEL32.669)
 */
DWORD WINAPI SetThreadAffinityMask( HANDLE hThread, DWORD dwThreadAffinityMask )
{
    struct set_thread_info_request *req = get_req_buffer();
    req->handle   = hThread;
    req->affinity = dwThreadAffinityMask;
    req->mask     = SET_THREAD_INFO_AFFINITY;
    if (server_call( REQ_SET_THREAD_INFO )) return 0;
    return 1;  /* FIXME: should return previous value */
}


/**********************************************************************
 * TerminateThread [KERNEL32.685]  Terminates a thread
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI TerminateThread(
    HANDLE handle, /* [in] Handle to thread */
    DWORD exitcode)  /* [in] Exit code for thread */
{
    struct terminate_thread_request *req = get_req_buffer();
    req->handle    = handle;
    req->exit_code = exitcode;
    return !server_call( REQ_TERMINATE_THREAD );
}


/**********************************************************************
 * GetExitCodeThread [KERNEL32.???]  Gets termination status of thread.
 * 
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetExitCodeThread(
    HANDLE hthread, /* [in]  Handle to thread */
    LPDWORD exitcode) /* [out] Address to receive termination status */
{
    BOOL ret = FALSE;
    struct get_thread_info_request *req = get_req_buffer();
    req->handle = hthread;
    if (!server_call( REQ_GET_THREAD_INFO ))
    {
        if (exitcode) *exitcode = req->exit_code;
        ret = TRUE;
    }
    return ret;
}


/**********************************************************************
 * ResumeThread [KERNEL32.587]  Resumes a thread.
 *
 * Decrements a thread's suspend count.  When count is zero, the
 * execution of the thread is resumed.
 *
 * RETURNS
 *    Success: Previous suspend count
 *    Failure: 0xFFFFFFFF
 *    Already running: 0
 */
DWORD WINAPI ResumeThread(
    HANDLE hthread) /* [in] Identifies thread to restart */
{
    DWORD ret = 0xffffffff;
    struct resume_thread_request *req = get_req_buffer();
    req->handle = hthread;
    if (!server_call( REQ_RESUME_THREAD )) ret = req->count;
    return ret;
}


/**********************************************************************
 * SuspendThread [KERNEL32.681]  Suspends a thread.
 *
 * RETURNS
 *    Success: Previous suspend count
 *    Failure: 0xFFFFFFFF
 */
DWORD WINAPI SuspendThread(
    HANDLE hthread) /* [in] Handle to the thread */
{
    DWORD ret = 0xffffffff;
    struct suspend_thread_request *req = get_req_buffer();
    req->handle = hthread;
    if (!server_call( REQ_SUSPEND_THREAD )) ret = req->count;
    return ret;
}


/***********************************************************************
 *              QueueUserAPC  (KERNEL32.566)
 */
DWORD WINAPI QueueUserAPC( PAPCFUNC func, HANDLE hthread, ULONG_PTR data )
{
    struct queue_apc_request *req = get_req_buffer();
    req->handle = hthread;
    req->func   = func;
    req->param  = (void *)data;
    return !server_call( REQ_QUEUE_APC );
}


/**********************************************************************
 * GetThreadTimes [KERNEL32.???]  Obtains timing information.
 *
 * NOTES
 *    What are the fields where these values are stored?
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
    FIXME("(0x%08x): stub\n",thread);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 * AttachThreadInput [KERNEL32.8]  Attaches input of 1 thread to other
 *
 * Attaches the input processing mechanism of one thread to that of
 * another thread.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 *
 * TODO:
 *    1. Reset the Key State (currenly per thread key state is not maintained)
 */
BOOL WINAPI AttachThreadInput( 
    DWORD idAttach,   /* [in] Thread to attach */
    DWORD idAttachTo, /* [in] Thread to attach to */
    BOOL fAttach)   /* [in] Attach or detach */
{
    MESSAGEQUEUE *pSrcMsgQ = 0, *pTgtMsgQ = 0;
    BOOL16 bRet = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    /* A thread cannot attach to itself */
    if ( idAttach == idAttachTo )
        goto CLEANUP;

    /* According to the docs this method should fail if a
     * "Journal record" hook is installed. (attaches all input queues together)
     */
    if ( HOOK_IsHooked( WH_JOURNALRECORD ) )
        goto CLEANUP;
        
    /* Retrieve message queues corresponding to the thread id's */
    pTgtMsgQ = (MESSAGEQUEUE *)QUEUE_Lock( GetThreadQueue16( idAttach ) );
    pSrcMsgQ = (MESSAGEQUEUE *)QUEUE_Lock( GetThreadQueue16( idAttachTo ) );

    /* Ensure we have message queues and that Src and Tgt threads
     * are not system threads.
     */
    if ( !pSrcMsgQ || !pTgtMsgQ || !pSrcMsgQ->pQData || !pTgtMsgQ->pQData )
        goto CLEANUP;

    if (fAttach)   /* Attach threads */
    {
        /* Only attach if currently detached  */
        if ( pTgtMsgQ->pQData != pSrcMsgQ->pQData )
        {
            /* First release the target threads perQData */
            PERQDATA_Release( pTgtMsgQ->pQData );
        
            /* Share a reference to the source threads perQDATA */
            PERQDATA_Addref( pSrcMsgQ->pQData );
            pTgtMsgQ->pQData = pSrcMsgQ->pQData;
        }
    }
    else    /* Detach threads */
    {
        /* Only detach if currently attached */
        if ( pTgtMsgQ->pQData == pSrcMsgQ->pQData )
        {
            /* First release the target threads perQData */
            PERQDATA_Release( pTgtMsgQ->pQData );
        
            /* Give the target thread its own private perQDATA once more */
            pTgtMsgQ->pQData = PERQDATA_CreateInstance();
        }
    }

    /* TODO: Reset the Key State */

    bRet = 1;      /* Success */
    
CLEANUP:

    /* Unlock the queues before returning */
    if ( pSrcMsgQ )
        QUEUE_Unlock( pSrcMsgQ );
    if ( pTgtMsgQ )
        QUEUE_Unlock( pTgtMsgQ );
    
    return bRet;
}

/**********************************************************************
 * VWin32_BoostThreadGroup [KERNEL.535]
 */
VOID WINAPI VWin32_BoostThreadGroup( DWORD threadId, INT boost )
{
    FIXME("(0x%08lx,%d): stub\n", threadId, boost);
}

/**********************************************************************
 * VWin32_BoostThreadStatic [KERNEL.536]
 */
VOID WINAPI VWin32_BoostThreadStatic( DWORD threadId, INT boost )
{
    FIXME("(0x%08lx,%d): stub\n", threadId, boost);
}

/**********************************************************************
 * SetThreadLocale [KERNEL32.671]  Sets the calling threads current locale.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 *
 * NOTES
 *  Implemented in NT only (3.1 and above according to MS
 */
BOOL WINAPI SetThreadLocale(
    LCID lcid)     /* [in] Locale identifier */
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 * SetLastError [KERNEL.147] [KERNEL32.497]  Sets the last-error code.
 *
 * RETURNS
 *    None.
 */
#undef SetLastError
void WINAPI SetLastError( DWORD error ) /* [in] Per-thread error code */
{
    NtCurrentTeb()->last_error = error;
}


/***********************************************************************
 * GetCurrentThreadId [KERNEL32.201]  Returns thread identifier.
 *
 * RETURNS
 *    Thread identifier of calling thread
 */
#undef GetCurrentThreadId
DWORD WINAPI GetCurrentThreadId(void)
{
    return (DWORD)NtCurrentTeb()->tid;
}
