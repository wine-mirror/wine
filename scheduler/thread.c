/*
 * Win32 threads
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
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
#include "debugtools.h"
#include "queue.h"
#include "hook.h"

DEFAULT_DEBUG_CHANNEL(thread)

/* THDB of the initial thread */
static THDB initial_thdb;

/* Global thread list (FIXME: not thread-safe) */
THDB *THREAD_First = &initial_thdb;

/***********************************************************************
 *           THREAD_IsWin16
 */
BOOL THREAD_IsWin16( THDB *thdb )
{
    return !thdb || !(thdb->teb.flags & TEBF_WIN32);
}

/***********************************************************************
 *           THREAD_IdToTHDB
 *
 * Convert a thread id to a THDB, making sure it is valid.
 */
THDB *THREAD_IdToTHDB( DWORD id )
{
    THDB *thdb = THREAD_First;

    if (!id) return THREAD_Current();
    while (thdb)
    {
        if ((DWORD)thdb->teb.tid == id) return thdb;
        thdb = thdb->next;
    }
    /* Allow task handles to be used; convert to main thread */
    if ( IsTask16( id ) )
    {
        TDB *pTask = (TDB *)GlobalLock16( id );
        if (pTask) return pTask->thdb;
    }
    SetLastError( ERROR_INVALID_PARAMETER );
    return NULL;
}


/***********************************************************************
 *           THREAD_InitTHDB
 *
 * Initialization of a newly created THDB.
 */
static BOOL THREAD_InitTHDB( THDB *thdb, DWORD stack_size, BOOL alloc_stack16,
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
    thdb->stack_base = VirtualAlloc(NULL, stack_size + SIGNAL_STACK_SIZE +
                                    (alloc_stack16 ? 0x10000 : 0),
                                    MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    if (!thdb->stack_base) goto error;
    /* Set a guard page at the bottom of the stack */
    VirtualProtect( thdb->stack_base, 1, PAGE_EXECUTE_READWRITE | PAGE_GUARD,
                    &old_prot );
    thdb->teb.stack_top   = (char *)thdb->stack_base + stack_size;
    thdb->teb.stack_low   = thdb->stack_base;
    thdb->signal_stack    = thdb->teb.stack_top;  /* start of signal stack */

    /* Allocate the 16-bit stack selector */

    if (alloc_stack16)
    {
        thdb->teb.stack_sel = SELECTOR_AllocBlock( thdb->teb.stack_top,
                                                   0x10000, SEGMENT_DATA,
                                                   FALSE, FALSE );
        if (!thdb->teb.stack_sel) goto error;
        thdb->cur_stack = PTR_SEG_OFF_TO_SEGPTR( thdb->teb.stack_sel, 
                                                 0x10000 - sizeof(STACK16FRAME) );
        thdb->signal_stack = (char *)thdb->signal_stack + 0x10000;
    }

    /* Create the thread event */

    if (!(thdb->event = CreateEventA( NULL, FALSE, FALSE, NULL ))) goto error;
    thdb->event = ConvertToGlobalHandle( thdb->event );
    return TRUE;

error:
    if (thdb->event) CloseHandle( thdb->event );
    if (thdb->teb.stack_sel) SELECTOR_FreeBlock( thdb->teb.stack_sel, 1 );
    if (thdb->stack_base) VirtualFree( thdb->stack_base, 0, MEM_RELEASE );
    return FALSE;
}


/***********************************************************************
 *           THREAD_FreeTHDB
 *
 * Free data structures associated with a thread.
 * Must be called from the context of another thread.
 */
void CALLBACK THREAD_FreeTHDB( ULONG_PTR arg )
{
    THDB *thdb = (THDB *)arg;
    THDB **pptr = &THREAD_First;

    TRACE("(%p) called\n", thdb );
    SERVICE_Delete( thdb->cleanup );

    PROCESS_CallUserSignalProc( USIG_THREAD_EXIT, 0 );
    
    CloseHandle( thdb->event );
    while (*pptr && (*pptr != thdb)) pptr = &(*pptr)->next;
    if (*pptr) *pptr = thdb->next;

    /* Free the associated memory */

    if (thdb->teb.stack_sel) SELECTOR_FreeBlock( thdb->teb.stack_sel, 1 );
    SELECTOR_FreeBlock( thdb->teb_sel, 1 );
    close( thdb->socket );
    VirtualFree( thdb->stack_base, 0, MEM_RELEASE );
    HeapFree( SystemHeap, HEAP_NO_SERIALIZE, thdb );
}


/***********************************************************************
 *           THREAD_CreateInitialThread
 *
 * Create the initial thread.
 */
THDB *THREAD_CreateInitialThread( PDB *pdb, int server_fd )
{
    initial_thdb.process         = pdb;
    initial_thdb.teb.except      = (void *)-1;
    initial_thdb.teb.self        = &initial_thdb.teb;
    initial_thdb.teb.flags       = /* TEBF_WIN32 */ 0;
    initial_thdb.teb.tls_ptr     = initial_thdb.tls_array;
    initial_thdb.teb.process     = pdb;
    initial_thdb.exit_code       = 0x103; /* STILL_ACTIVE */
    initial_thdb.socket          = server_fd;

    /* Allocate the TEB selector (%fs register) */

    if (!(initial_thdb.teb_sel = SELECTOR_AllocBlock( &initial_thdb.teb, 0x1000,
                                                      SEGMENT_DATA, TRUE, FALSE )))
    {
        MESSAGE("Could not allocate fs register for initial thread\n" );
        return NULL;
    }
    SYSDEPS_SetCurThread( &initial_thdb );

    /* Now proceed with normal initialization */

    if (CLIENT_InitThread()) return NULL;
    if (!THREAD_InitTHDB( &initial_thdb, 0, TRUE, NULL )) return NULL;
    return &initial_thdb;
}


/***********************************************************************
 *           THREAD_Create
 */
THDB *THREAD_Create( PDB *pdb, DWORD flags, DWORD stack_size, BOOL alloc_stack16,
                     LPSECURITY_ATTRIBUTES sa, int *server_handle )
{
    struct new_thread_request request;
    struct new_thread_reply reply = { NULL, -1 };
    int fd[2];

    THDB *thdb = HeapAlloc( SystemHeap, HEAP_ZERO_MEMORY, sizeof(THDB) );
    if (!thdb) return NULL;
    thdb->process         = pdb;
    thdb->teb.except      = (void *)-1;
    thdb->teb.htask16     = pdb->task;
    thdb->teb.self        = &thdb->teb;
    thdb->teb.flags       = (pdb->flags & PDB32_WIN16_PROC)? 0 : TEBF_WIN32;
    thdb->teb.tls_ptr     = thdb->tls_array;
    thdb->teb.process     = pdb;
    thdb->exit_code       = 0x103; /* STILL_ACTIVE */
    thdb->flags           = flags;
    thdb->socket          = -1;

    /* Allocate the TEB selector (%fs register) */

    thdb->teb_sel = SELECTOR_AllocBlock( &thdb->teb, 0x1000, SEGMENT_DATA,
                                         TRUE, FALSE );
    if (!thdb->teb_sel) goto error;

    /* Create the socket pair for server communication */

    if (socketpair( AF_UNIX, SOCK_STREAM, 0, fd ) == -1)
    {
        SetLastError( ERROR_TOO_MANY_OPEN_FILES );  /* FIXME */
        goto error;
    }
    thdb->socket = fd[0];
    fcntl( fd[0], F_SETFD, 1 ); /* set close on exec flag */

    /* Create the thread on the server side */

    request.pid     = thdb->process->server_pid;
    request.suspend = ((thdb->flags & CREATE_SUSPENDED) != 0);
    request.inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    CLIENT_SendRequest( REQ_NEW_THREAD, fd[1], 1, &request, sizeof(request) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) goto error;
    thdb->teb.tid = reply.tid;
    *server_handle = reply.handle;

    /* Do the rest of the initialization */

    if (!THREAD_InitTHDB( thdb, stack_size, alloc_stack16, sa )) goto error;
    thdb->next = THREAD_First;
    THREAD_First = thdb;

    /* Install cleanup handler */

    thdb->cleanup = SERVICE_AddObject( *server_handle, 
                                       THREAD_FreeTHDB, (ULONG_PTR)thdb );
    return thdb;

error:
    if (reply.handle != -1) CloseHandle( reply.handle );
    if (thdb->teb_sel) SELECTOR_FreeBlock( thdb->teb_sel, 1 );
    HeapFree( SystemHeap, 0, thdb );
    if (thdb->socket != -1) close( thdb->socket );
    return NULL;
}


/***********************************************************************
 *           THREAD_Start
 *
 * Start execution of a newly created thread. Does not return.
 */
static void THREAD_Start(void)
{
    THDB *thdb = THREAD_Current();
    LPTHREAD_START_ROUTINE func = (LPTHREAD_START_ROUTINE)thdb->entry_point;
    PROCESS_CallUserSignalProc( USIG_THREAD_INIT, 0 );
    PE_InitTls();
    MODULE_DllThreadAttach( NULL );

    if (thdb->process->flags & PDB32_DEBUGGED) DEBUG_SendCreateThreadEvent( func );

    ExitThread( func( thdb->entry_arg ) );
}


/***********************************************************************
 *           CreateThread   (KERNEL32.63)
 */
HANDLE WINAPI CreateThread( SECURITY_ATTRIBUTES *sa, DWORD stack,
                            LPTHREAD_START_ROUTINE start, LPVOID param,
                            DWORD flags, LPDWORD id )
{
    int handle = -1;
    THDB *thread = THREAD_Create( PROCESS_Current(), flags, stack, TRUE, sa, &handle );
    if (!thread) return INVALID_HANDLE_VALUE;
    thread->teb.flags  |= TEBF_WIN32;
    thread->entry_point = start;
    thread->entry_arg   = param;
    thread->startup     = THREAD_Start;
    if (SYSDEPS_SpawnThread( thread ) == -1)
    {
        CloseHandle( handle );
        return INVALID_HANDLE_VALUE;
    }
    if (id) *id = (DWORD)thread->teb.tid;
    return handle;
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


/***********************************************************************
 * GetCurrentThreadId [KERNEL32.201]  Returns thread identifier.
 *
 * RETURNS
 *    Thread identifier of calling thread
 */
DWORD WINAPI GetCurrentThreadId(void)
{
    return (DWORD)CURRENT()->tid;
}


/**********************************************************************
 * GetLastError [KERNEL.148] [KERNEL32.227]  Returns last-error code.
 *
 * RETURNS
 *     Calling thread's last error code value.
 */
DWORD WINAPI GetLastError(void)
{
    THDB *thread = THREAD_Current();
    DWORD ret = thread->last_error;
    TRACE("0x%lx\n",ret);
    return ret;
}


/**********************************************************************
 * SetLastError [KERNEL.147] [KERNEL32.497]  Sets the last-error code.
 *
 * RETURNS
 *    None.
 */
void WINAPI SetLastError(
    DWORD error) /* [in] Per-thread error code */
{
    THDB *thread = THREAD_Current();
    TRACE("%p error=0x%lx\n",thread,error);
    thread->last_error = error;
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
    THDB *thread = THREAD_Current();
    DWORD i, mask, ret = 0;
    DWORD *bits = thread->process->tls_bits;
    EnterCriticalSection( &thread->process->crit_section );
    if (*bits == 0xffffffff)
    {
        bits++;
        ret = 32;
        if (*bits == 0xffffffff)
        {
            LeaveCriticalSection( &thread->process->crit_section );
            SetLastError( ERROR_NO_MORE_ITEMS );
            return 0xffffffff;
        }
    }
    for (i = 0, mask = 1; i < 32; i++, mask <<= 1) if (!(*bits & mask)) break;
    *bits |= mask;
    LeaveCriticalSection( &thread->process->crit_section );
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
    DWORD mask;
    THDB *thread = THREAD_Current();
    DWORD *bits = thread->process->tls_bits;
    if (index >= 64)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    EnterCriticalSection( &thread->process->crit_section );
    if (index >= 32) bits++;
    mask = (1 << (index & 31));
    if (!(*bits & mask))  /* already free? */
    {
        LeaveCriticalSection( &thread->process->crit_section );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    *bits &= ~mask;
    thread->tls_array[index] = 0;
    /* FIXME: should zero all other thread values */
    LeaveCriticalSection( &thread->process->crit_section );
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
    THDB *thread = THREAD_Current();
    if (index >= 64)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    SetLastError( ERROR_SUCCESS );
    return thread->tls_array[index];
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
    THDB *thread = THREAD_Current();
    if (index >= 64)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    thread->tls_array[index] = value;
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
    CONTEXT *context) /* [out] Address of context structure */
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
    struct get_thread_info_request req;
    struct get_thread_info_reply reply;
    req.handle = hthread;
    CLIENT_SendRequest( REQ_GET_THREAD_INFO, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL ))
        return THREAD_PRIORITY_ERROR_RETURN;
    return reply.priority;
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
    struct set_thread_info_request req;
    req.handle   = hthread;
    req.priority = priority;
    req.mask     = SET_THREAD_INFO_PRIORITY;
    CLIENT_SendRequest( REQ_SET_THREAD_INFO, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
}


/**********************************************************************
 *           SetThreadAffinityMask   (KERNEL32.669)
 */
DWORD WINAPI SetThreadAffinityMask( HANDLE hThread, DWORD dwThreadAffinityMask )
{
    struct set_thread_info_request req;
    req.handle   = hThread;
    req.affinity = dwThreadAffinityMask;
    req.mask     = SET_THREAD_INFO_AFFINITY;
    CLIENT_SendRequest( REQ_SET_THREAD_INFO, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitReply( NULL, NULL, 0 )) return 0;
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
    struct terminate_thread_request req;
    req.handle    = handle;
    req.exit_code = exitcode;
    CLIENT_SendRequest( REQ_TERMINATE_THREAD, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
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
    struct get_thread_info_request req;
    struct get_thread_info_reply reply;
    req.handle = hthread;
    CLIENT_SendRequest( REQ_GET_THREAD_INFO, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) return FALSE;
    if (exitcode) *exitcode = reply.exit_code;
    return TRUE;
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
    struct resume_thread_request req;
    struct resume_thread_reply reply;
    req.handle = hthread;
    CLIENT_SendRequest( REQ_RESUME_THREAD, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) return 0xffffffff;
    return reply.count;
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
    struct suspend_thread_request req;
    struct suspend_thread_reply reply;
    req.handle = hthread;
    CLIENT_SendRequest( REQ_SUSPEND_THREAD, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) return 0xffffffff;
    return reply.count;
}


/***********************************************************************
 *              QueueUserAPC  (KERNEL32.566)
 */
DWORD WINAPI QueueUserAPC( PAPCFUNC func, HANDLE hthread, ULONG_PTR data )
{
    struct queue_apc_request req;
    req.handle = hthread;
    req.func   = func;
    req.param  = (void *)data;
    CLIENT_SendRequest( REQ_QUEUE_APC, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
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
