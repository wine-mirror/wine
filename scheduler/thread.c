/*
 * Win32 threads
 *
 * Copyright 1996 Alexandre Julliard
 */

#include "config.h"

#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <unistd.h>
#include "wine/winbase16.h"
#include "thread.h"
#include "process.h"
#include "task.h"
#include "module.h"
#include "global.h"
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

DEFAULT_DEBUG_CHANNEL(thread);

/* TEB of the initial thread */
static TEB initial_teb;

/* The initial process PDB */
static PDB initial_pdb;

/***********************************************************************
 *           THREAD_IsWin16
 */
BOOL THREAD_IsWin16( TEB *teb )
{
    return !teb || !(teb->tibflags & TEBF_WIN32);
}

/***********************************************************************
 *           THREAD_IdToTEB
 *
 * Convert a thread id to a TEB, making sure it is valid.
 */
TEB *THREAD_IdToTEB( DWORD id )
{
    struct get_thread_info_request *req = get_req_buffer();

    if (!id || id == GetCurrentThreadId()) return NtCurrentTeb();
    req->handle = -1;
    req->tid_in = (void *)id;
    if (!server_call_noerr( REQ_GET_THREAD_INFO )) return req->teb;

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
static BOOL THREAD_InitTEB( TEB *teb )
{
    teb->except    = (void *)~0UL;
    teb->self      = teb;
    teb->tibflags  = TEBF_WIN32;
    teb->tls_ptr   = teb->tls_array;
    teb->exit_code = STILL_ACTIVE;
    teb->socket    = -1;
    teb->stack_top = (void *)~0UL;
    teb->StaticUnicodeString.MaximumLength = sizeof(teb->StaticUnicodeBuffer);
    teb->StaticUnicodeString.Buffer = (PWSTR)teb->StaticUnicodeBuffer;
    teb->teb_sel = SELECTOR_AllocBlock( teb, 0x1000, SEGMENT_DATA, TRUE, FALSE );
    return (teb->teb_sel != 0);
}


/***********************************************************************
 *           THREAD_FreeTEB
 *
 * Free data structures associated with a thread.
 * Must be called from the context of another thread.
 */
static void CALLBACK THREAD_FreeTEB( TEB *teb )
{
    TRACE("(%p) called\n", teb );
    if (teb->cleanup) SERVICE_Delete( teb->cleanup );

    /* Free the associated memory */

    if (teb->socket != -1) close( teb->socket );
    if (teb->stack_sel) SELECTOR_FreeBlock( teb->stack_sel, 1 );
    SELECTOR_FreeBlock( teb->teb_sel, 1 );
    if (teb->buffer) munmap( teb->buffer, teb->buffer_size );
    if (teb->debug_info) HeapFree( GetProcessHeap(), 0, teb->debug_info );
    VirtualFree( teb->stack_base, 0, MEM_RELEASE );
}


/***********************************************************************
 *           THREAD_InitStack
 *
 * Allocate the stack of a thread.
 */
TEB *THREAD_InitStack( TEB *teb, DWORD stack_size, BOOL alloc_stack16 )
{
    DWORD old_prot, total_size;
    DWORD page_size = VIRTUAL_GetPageSize();
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
     * 64Kb                16-bit stack (optional)
     * 1 page              TEB (except for initial thread)
     */

    stack_size = (stack_size + (page_size - 1)) & ~(page_size - 1);
    total_size = stack_size + SIGNAL_STACK_SIZE + 3 * page_size;
    if (alloc_stack16) total_size += 0x10000;
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

    /* Allocate the 16-bit stack selector */

    if (alloc_stack16)
    {
        teb->stack_sel = SELECTOR_AllocBlock( teb->stack_top, 0x10000, SEGMENT_DATA,
                                              FALSE, FALSE );
        if (!teb->stack_sel) goto error;
        teb->cur_stack = PTR_SEG_OFF_TO_SEGPTR( teb->stack_sel, 
                                                0x10000 - sizeof(STACK16FRAME) );
    }
    return teb;

error:
    THREAD_FreeTEB( teb );
    return NULL;
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
    if (!initial_teb.self)  /* do it only once */
    {
        THREAD_InitTEB( &initial_teb );
        assert( initial_teb.teb_sel );
        initial_teb.process = &initial_pdb;
        SYSDEPS_SetCurThread( &initial_teb );
    }
}

DECL_GLOBAL_CONSTRUCTOR(thread_init) { THREAD_Init(); }

/***********************************************************************
 *           THREAD_Create
 *
 */
TEB *THREAD_Create( int fd, DWORD stack_size, BOOL alloc_stack16 )
{
    TEB *teb;

    if ((teb = THREAD_InitStack( NULL, stack_size, alloc_stack16 )))
    {
        teb->tibflags = (PROCESS_Current()->flags & PDB32_WIN16_PROC) ? 0 : TEBF_WIN32;
        teb->process  = PROCESS_Current();
        teb->socket   = fd;
        fcntl( fd, F_SETFD, 1 ); /* set close on exec flag */
        TRACE("(%p) succeeded\n", teb);
    }
    return teb;
}


/***********************************************************************
 *           THREAD_Start
 *
 * Start execution of a newly created thread. Does not return.
 */
static void THREAD_Start(void)
{
    HANDLE cleanup_object;
    LPTHREAD_START_ROUTINE func = (LPTHREAD_START_ROUTINE)NtCurrentTeb()->entry_point;

    /* install cleanup handler */
    if (DuplicateHandle( GetCurrentProcess(), GetCurrentThread(),
                         GetCurrentProcess(), &cleanup_object, 
                         0, FALSE, DUPLICATE_SAME_ACCESS ))
        NtCurrentTeb()->cleanup = SERVICE_AddObject( cleanup_object, (PAPCFUNC)THREAD_FreeTEB,
                                                     (ULONG_PTR)NtCurrentTeb() );

    PROCESS_CallUserSignalProc( USIG_THREAD_INIT, 0 );
    PE_InitTls();
    MODULE_DllThreadAttach( NULL );
    ExitThread( func( NtCurrentTeb()->entry_arg ) );
}


/***********************************************************************
 *           CreateThread   (KERNEL32.63)
 */
HANDLE WINAPI CreateThread( SECURITY_ATTRIBUTES *sa, DWORD stack,
                            LPTHREAD_START_ROUTINE start, LPVOID param,
                            DWORD flags, LPDWORD id )
{
    struct new_thread_request *req = get_req_buffer();
    int socket, handle = -1;
    TEB *teb;
    void *tid;

    req->suspend = ((flags & CREATE_SUSPENDED) != 0);
    req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    if (server_call_fd( REQ_NEW_THREAD, -1, &socket )) return 0;
    handle = req->handle;
    tid = req->tid;

    if (!(teb = THREAD_Create( socket, stack, TRUE )))
    {
        close( socket );
        return 0;
    }
    teb->tibflags   |= TEBF_WIN32;
    teb->entry_point = start;
    teb->entry_arg   = param;
    teb->startup     = THREAD_Start;
    teb->htask16     = GetCurrentTask();
    if (id) *id = (DWORD)tid;
    if (SYSDEPS_SpawnThread( teb ) == -1)
    {
        CloseHandle( handle );
        return 0;
    }
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
    struct terminate_thread_request *req = get_req_buffer();

     /* send the exit code to the server */
    req->handle    = GetCurrentThread();
    req->exit_code = code;
    server_call( REQ_TERMINATE_THREAD );
    if (req->last)
    {
        MODULE_DllProcessDetach( TRUE, (LPVOID)1 );
        exit( code );
    }
    else
    {
        MODULE_DllThreadDetach( NULL );
        if (!(NtCurrentTeb()->tibflags & TEBF_WIN32)) TASK_KillTask( 0 );
        SYSDEPS_ExitThread( code );
    }
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
BOOL WINAPI SetThreadContext( HANDLE handle,           /* [in]  Handle to thread with context */
                              const CONTEXT *context ) /* [in] Address of context structure */
{
    struct set_thread_context_request *req = get_req_buffer();
    req->handle = handle;
    req->flags = context->ContextFlags;
    memcpy( &req->context, context, sizeof(*context) );
    return !server_call( REQ_SET_THREAD_CONTEXT );
}


/***********************************************************************
 * GetThreadContext [KERNEL32.294]  Retrieves context of thread.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetThreadContext( HANDLE handle,     /* [in]  Handle to thread with context */
                              CONTEXT *context ) /* [out] Address of context structure */
{
    struct get_thread_context_request *req = get_req_buffer();
    req->handle = handle;
    req->flags = context->ContextFlags;
    memcpy( &req->context, context, sizeof(*context) );
    if (server_call( REQ_GET_THREAD_CONTEXT )) return FALSE;
    memcpy( context, &req->context, sizeof(*context) );
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
    req->tid_in = 0;
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
    BOOL ret;
    struct terminate_thread_request *req = get_req_buffer();
    req->handle    = handle;
    req->exit_code = exitcode;
    if ((ret = !server_call( REQ_TERMINATE_THREAD )) && req->self)
    {
        PROCESS_CallUserSignalProc( USIG_THREAD_EXIT, 0 );
        if (req->last) exit( exitcode );
        else SYSDEPS_ExitThread( exitcode );
    }
    return ret;
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
    req->tid_in = 0;
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


/***********************************************************************
 * GetCurrentThread [KERNEL32.200]  Gets pseudohandle for current thread
 *
 * RETURNS
 *    Pseudohandle for the current thread
 */
#undef GetCurrentThread
HANDLE WINAPI GetCurrentThread(void)
{
    return 0xfffffffe;
}


#ifdef __i386__

/* void WINAPI SetLastError( DWORD error ); */
__ASM_GLOBAL_FUNC( SetLastError,
                   "movl 4(%esp),%eax\n\t"
                   ".byte 0x64\n\t"
                   "movl %eax,0x60\n\t"
                   "ret $4" );

/* DWORD WINAPI GetLastError(void); */
__ASM_GLOBAL_FUNC( GetLastError, ".byte 0x64\n\tmovl 0x60,%eax\n\tret" );

/* DWORD WINAPI GetCurrentProcessId(void) */
__ASM_GLOBAL_FUNC( GetCurrentProcessId, ".byte 0x64\n\tmovl 0x20,%eax\n\tret" );
                   
/* DWORD WINAPI GetCurrentThreadId(void) */
__ASM_GLOBAL_FUNC( GetCurrentThreadId, ".byte 0x64\n\tmovl 0x24,%eax\n\tret" );
                   
#else  /* __i386__ */

/**********************************************************************
 * SetLastError [KERNEL.147] [KERNEL32.497]  Sets the last-error code.
 */
void WINAPI SetLastError( DWORD error ) /* [in] Per-thread error code */
{
    NtCurrentTeb()->last_error = error;
}

/**********************************************************************
 * GetLastError [KERNEL.148] [KERNEL32.227]  Returns last-error code.
 */
DWORD WINAPI GetLastError(void)
{
    return NtCurrentTeb()->last_error;
}

/***********************************************************************
 * GetCurrentProcessId [KERNEL32.199]  Returns process identifier.
 */
DWORD WINAPI GetCurrentProcessId(void)
{
    return (DWORD)NtCurrentTeb()->pid;
}

/***********************************************************************
 * GetCurrentThreadId [KERNEL32.201]  Returns thread identifier.
 */
DWORD WINAPI GetCurrentThreadId(void)
{
    return (DWORD)NtCurrentTeb()->tid;
}

#endif  /* __i386__ */
