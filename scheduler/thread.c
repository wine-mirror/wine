/*
 * Win32 threads
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include "thread.h"
#include "process.h"
#include "task.h"
#include "module.h"
#include "user.h"
#include "winerror.h"
#include "heap.h"
#include "selectors.h"
#include "miscemu.h"
#include "winnt.h"
#include "server.h"
#include "stackframe.h"
#include "debug.h"

#ifndef __i386__
THDB *pCurrentThread;
#endif

static void THREAD_Destroy( K32OBJ *obj );

const K32OBJ_OPS THREAD_Ops =
{
    THREAD_Destroy      /* destroy */
};

/* Is threading code initialized? */
BOOL32 THREAD_InitDone = FALSE;


/***********************************************************************
 *           THREAD_Current
 *
 * Return the current thread THDB pointer.
 */
THDB *THREAD_Current(void)
{
    if (!THREAD_InitDone) return NULL;
    return (THDB *)((char *)NtCurrentTeb() - (int)&((THDB *)0)->teb);
}

/***********************************************************************
 *           THREAD_IsWin16
 */
BOOL32 THREAD_IsWin16( THDB *thdb )
{
    if (!thdb || !thdb->process)
        return TRUE;
    else
    {
        TDB* pTask = (TDB*)GlobalLock16( thdb->process->task );
        return !pTask || pTask->thdb == thdb;
    }
}

/***********************************************************************
 *           THREAD_IdToTHDB
 *
 * Convert a thread id to a THDB, making sure it is valid.
 */
THDB *THREAD_IdToTHDB( DWORD id )
{
    THDB *thdb;

    if (!id) return THREAD_Current();
    thdb = THREAD_ID_TO_THDB( id );
    if (!K32OBJ_IsValid( &thdb->header, K32OBJ_THREAD ))
    {
        /* Allow task handles to be used; convert to main thread */
        if ( IsTask( id ) )
        {
            TDB *pTask = (TDB *)GlobalLock16( id );
            if (pTask) return pTask->thdb;
        }
        
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    return thdb;
}


/***********************************************************************
 *           THREAD_AddQueue
 *
 * Add a thread to a queue.
 */
void THREAD_AddQueue( THREAD_QUEUE *queue, THDB *thread )
{
    THREAD_ENTRY *entry = HeapAlloc( SystemHeap, HEAP_NO_SERIALIZE,
                                     sizeof(*entry) );
    assert(entry);
    SYSTEM_LOCK();
    entry->thread = thread;
    if (*queue)
    {
        entry->next = (*queue)->next;
        (*queue)->next = entry;
    }
    else entry->next = entry;
    *queue = entry;
    SYSTEM_UNLOCK();
}

/***********************************************************************
 *           THREAD_RemoveQueue
 *
 * Remove a thread from a queue.
 */
void THREAD_RemoveQueue( THREAD_QUEUE *queue, THDB *thread )
{
    THREAD_ENTRY *entry = *queue;
    SYSTEM_LOCK();
    if (entry->next == entry)  /* Only one element in the queue */
    {
        assert( entry->thread == thread );
        *queue = NULL;
    }
    else
    {
        THREAD_ENTRY *next;
        while (entry->next->thread != thread)
        {
            entry = entry->next;
            assert( entry != *queue );  /* Have we come all the way around? */
        }
        if ((next = entry->next) == *queue) *queue = entry;
        entry->next = entry->next->next;
        entry = next;  /* This is the one we want to free */
    }
    HeapFree( SystemHeap, 0, entry );
    SYSTEM_UNLOCK();
}


/***********************************************************************
 *           THREAD_Create
 */
THDB *THREAD_Create( PDB32 *pdb, DWORD stack_size, BOOL32 alloc_stack16,
                     int *server_thandle, int *server_phandle,
                     LPTHREAD_START_ROUTINE start_addr, LPVOID param )
{
    DWORD old_prot;

    THDB *thdb = HeapAlloc( SystemHeap, HEAP_ZERO_MEMORY, sizeof(THDB) );
    if (!thdb) return NULL;
    thdb->header.type     = K32OBJ_THREAD;
    thdb->header.refcount = 1;
    thdb->process         = pdb;
    thdb->teb.except      = (void *)-1;
    thdb->teb.htask16     = 0; /* FIXME */
    thdb->teb.self        = &thdb->teb;
    thdb->teb.flags       = (pdb->flags & PDB32_WIN16_PROC)? 0 : TEBF_WIN32;
    thdb->teb.tls_ptr     = thdb->tls_array;
    thdb->teb.process     = pdb;
    thdb->wait_list       = &thdb->wait_struct;
    thdb->exit_code       = 0x103; /* STILL_ACTIVE */
    thdb->entry_point     = start_addr;
    thdb->entry_arg       = param;
    thdb->socket          = -1;

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
    	WARN(thread,"Thread stack size is %ld MB.\n",stack_size/1024/1024);
    thdb->stack_base = VirtualAlloc(NULL,
                                    stack_size + (alloc_stack16 ? 0x10000 : 0),
                                    MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    if (!thdb->stack_base) goto error;
    /* Set a guard page at the bottom of the stack */
    VirtualProtect( thdb->stack_base, 1, PAGE_EXECUTE_READWRITE | PAGE_GUARD,
                    &old_prot );
    thdb->teb.stack_top   = (char *)thdb->stack_base + stack_size;
    thdb->teb.stack_low   = thdb->stack_base;
    thdb->exit_stack      = thdb->teb.stack_top;

    /* Allocate the TEB selector (%fs register) */

    thdb->teb_sel = SELECTOR_AllocBlock( &thdb->teb, 0x1000, SEGMENT_DATA,
                                         TRUE, FALSE );
    if (!thdb->teb_sel) goto error;

    /* Allocate the 16-bit stack selector */

    if (alloc_stack16)
    {
        thdb->teb.stack_sel = SELECTOR_AllocBlock( thdb->teb.stack_top,
                                                   0x10000, SEGMENT_DATA,
                                                   FALSE, FALSE );
        if (!thdb->teb.stack_sel) goto error;
        thdb->cur_stack = PTR_SEG_OFF_TO_SEGPTR( thdb->teb.stack_sel, 
                                                 0x10000 - sizeof(STACK16FRAME) );
    }

    /* Create the thread socket */

    if (CLIENT_NewThread( thdb, server_thandle, server_phandle )) goto error;

    /* Add thread to process's list of threads */

    THREAD_AddQueue( &pdb->thread_list, thdb );

    /* Create the thread event */

    if (!(thdb->event = CreateEvent32A( NULL, FALSE, FALSE, NULL ))) goto error;
    thdb->event = ConvertToGlobalHandle( thdb->event );

    PE_InitTls( thdb );
    return thdb;

error:
    if (thdb->socket != -1) close( thdb->socket );
    if (thdb->event) CloseHandle( thdb->event );
    if (thdb->teb.stack_sel) SELECTOR_FreeBlock( thdb->teb.stack_sel, 1 );
    if (thdb->teb_sel) SELECTOR_FreeBlock( thdb->teb_sel, 1 );
    if (thdb->stack_base) VirtualFree( thdb->stack_base, 0, MEM_RELEASE );
    HeapFree( SystemHeap, 0, thdb );
    return NULL;
}


/***********************************************************************
 *           THREAD_Destroy
 */
static void THREAD_Destroy( K32OBJ *ptr )
{
    THDB *thdb = (THDB *)ptr;

    assert( ptr->type == K32OBJ_THREAD );
    ptr->type = K32OBJ_UNKNOWN;

    /* Free the associated memory */

#ifdef __i386__
    {
        /* Check if we are deleting the current thread */
        WORD fs;
        GET_FS( fs );
        if (fs == thdb->teb_sel)
        {
            GET_DS( fs );
            SET_FS( fs );
        }
    }
#endif
    CloseHandle( thdb->event );
    close( thdb->socket );
    SELECTOR_FreeBlock( thdb->teb_sel, 1 );
    if (thdb->teb.stack_sel) SELECTOR_FreeBlock( thdb->teb.stack_sel, 1 );
    HeapFree( SystemHeap, 0, thdb );

}


/***********************************************************************
 *           THREAD_Start
 *
 * Start execution of a newly created thread. Does not return.
 */
void THREAD_Start( THDB *thdb )
{
    LPTHREAD_START_ROUTINE func = (LPTHREAD_START_ROUTINE)thdb->entry_point;
    assert( THREAD_Current() == thdb );
    CLIENT_InitThread();
    MODULE_InitializeDLLs( thdb->process, 0, DLL_THREAD_ATTACH, NULL );
    ExitThread( func( thdb->entry_arg ) );
}


/***********************************************************************
 *           CreateThread   (KERNEL32.63)
 */
HANDLE32 WINAPI CreateThread( SECURITY_ATTRIBUTES *sa, DWORD stack,
                              LPTHREAD_START_ROUTINE start, LPVOID param,
                              DWORD flags, LPDWORD id )
{
    int server_handle = -1;
    HANDLE32 handle = INVALID_HANDLE_VALUE32;
    BOOL32 inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);

    THDB *thread = THREAD_Create( PROCESS_Current(), stack,
                                  TRUE, &server_handle, NULL, start, param );
    if (!thread) return INVALID_HANDLE_VALUE32;
    handle = HANDLE_Alloc( PROCESS_Current(), &thread->header,
                           THREAD_ALL_ACCESS, inherit, server_handle );
    if (handle == INVALID_HANDLE_VALUE32) goto error;
    if (SYSDEPS_SpawnThread( thread ) == -1) goto error;
    if (id) *id = THDB_TO_THREAD_ID( thread );
    return handle;

error:
    if (handle != INVALID_HANDLE_VALUE32) CloseHandle( handle );
    K32OBJ_DecCount( &thread->header );
    return INVALID_HANDLE_VALUE32;
}


/***********************************************************************
 * ExitThread [KERNEL32.215]  Ends a thread
 *
 * RETURNS
 *    None
 */
void WINAPI ExitThread(
    DWORD code) /* [in] Exit code for this thread */
{
    THDB *thdb = THREAD_Current();
    LONG count;

    /* Remove thread from process's list */
    THREAD_RemoveQueue( &thdb->process->thread_list, thdb );

    MODULE_InitializeDLLs( thdb->process, 0, DLL_THREAD_DETACH, NULL );

    SYSTEM_LOCK();
    thdb->exit_code = code;

    /* FIXME: should free the stack somehow */
#if 0
    /* FIXME: We cannot do this; once the current thread is destroyed,
       synchronization primitives do not work properly. */
    K32OBJ_DecCount( &thdb->header );
#endif
    /* Completely unlock the system lock just in case */
    count = SYSTEM_LOCK_COUNT();
    while (count--) SYSTEM_UNLOCK();
    SYSDEPS_ExitThread();
}


/***********************************************************************
 * GetCurrentThread [KERNEL32.200]  Gets pseudohandle for current thread
 *
 * RETURNS
 *    Pseudohandle for the current thread
 */
HANDLE32 WINAPI GetCurrentThread(void)
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
    return THDB_TO_THREAD_ID( THREAD_Current() );
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
    TRACE(thread,"0x%lx\n",ret);
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
    /* This one must work before we have a thread (FIXME) */

    TRACE(thread,"%p error=0x%lx\n",thread,error);

    if (thread)
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
    TRACE(thread, "(0x%08lx, 0x%08lx)\n", error,type);
    switch(type) {
        case 0:
            break;
        case SLE_ERROR:
        case SLE_MINORERROR:
        case SLE_WARNING:
            /* Fall through for now */
        default:
            FIXME(thread, "(error=%08lx, type=%08lx): Unhandled type\n", error,type);
            break;
    }
    SetLastError( error );
}


/**********************************************************************
 *           THREAD_TlsAlloc
 */
DWORD THREAD_TlsAlloc(THDB *thread)
{
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
 * TlsAlloc [KERNEL32.530]  Allocates a TLS index.
 *
 * Allocates a thread local storage index
 *
 * RETURNS
 *    Success: TLS Index
 *    Failure: 0xFFFFFFFF
 */
DWORD WINAPI TlsAlloc(void)
{
    return THREAD_TlsAlloc(THREAD_Current());
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
BOOL32 WINAPI TlsFree(
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
BOOL32 WINAPI TlsSetValue(
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
BOOL32 WINAPI SetThreadContext(
    HANDLE32 handle,  /* [in]  Handle to thread with context */
    CONTEXT *context) /* [out] Address of context structure */
{
    FIXME( thread, "not implemented\n" );
    return TRUE;
}

/***********************************************************************
 * GetThreadContext [KERNEL32.294]  Retrieves context of thread.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL32 WINAPI GetThreadContext(
    HANDLE32 handle,  /* [in]  Handle to thread with context */
    CONTEXT *context) /* [out] Address of context structure */
{
    WORD cs, ds;

    FIXME( thread, "returning dummy info\n" );

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
INT32 WINAPI GetThreadPriority(
    HANDLE32 hthread) /* [in] Handle to thread */
{
    struct get_thread_info_request req;
    struct get_thread_info_reply reply;
    req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hthread,
                                         K32OBJ_THREAD, THREAD_QUERY_INFORMATION );
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
BOOL32 WINAPI SetThreadPriority(
    HANDLE32 hthread, /* [in] Handle to thread */
    INT32 priority)   /* [in] Thread priority level */
{
    struct set_thread_info_request req;
    req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hthread,
                                         K32OBJ_THREAD, THREAD_SET_INFORMATION );
    if (req.handle == -1) return FALSE;
    req.priority = priority;
    req.mask     = SET_THREAD_INFO_PRIORITY;
    CLIENT_SendRequest( REQ_SET_THREAD_INFO, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
}


/**********************************************************************
 *           SetThreadAffinityMask   (KERNEL32.669)
 */
DWORD WINAPI SetThreadAffinityMask( HANDLE32 hThread, DWORD dwThreadAffinityMask )
{
    struct set_thread_info_request req;
    req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hThread,
                                         K32OBJ_THREAD, THREAD_SET_INFORMATION );
    if (req.handle == -1) return FALSE;
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
BOOL32 WINAPI TerminateThread(
    HANDLE32 handle, /* [in] Handle to thread */
    DWORD exitcode)  /* [in] Exit code for thread */
{
    struct terminate_thread_request req;

    req.handle = HANDLE_GetServerHandle( PROCESS_Current(), handle,
                                         K32OBJ_THREAD, THREAD_TERMINATE );
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
BOOL32 WINAPI GetExitCodeThread(
    HANDLE32 hthread, /* [in]  Handle to thread */
    LPDWORD exitcode) /* [out] Address to receive termination status */
{
    struct get_thread_info_request req;
    struct get_thread_info_reply reply;
    req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hthread,
                                         K32OBJ_THREAD, THREAD_QUERY_INFORMATION );
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
    HANDLE32 hthread) /* [in] Identifies thread to restart */
{
    struct resume_thread_request req;
    struct resume_thread_reply reply;
    req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hthread,
                                         K32OBJ_THREAD, THREAD_SUSPEND_RESUME );
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
    HANDLE32 hthread) /* [in] Handle to the thread */
{
    struct suspend_thread_request req;
    struct suspend_thread_reply reply;
    req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hthread,
                                         K32OBJ_THREAD, THREAD_SUSPEND_RESUME );
    CLIENT_SendRequest( REQ_SUSPEND_THREAD, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) return 0xffffffff;
    return reply.count;
}


/***********************************************************************
 *              QueueUserAPC  (KERNEL32.566)
 */
DWORD WINAPI QueueUserAPC( PAPCFUNC func, HANDLE32 hthread, ULONG_PTR data )
{
    struct queue_apc_request req;
    req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hthread,
                                         K32OBJ_THREAD, THREAD_SET_CONTEXT );
    req.func  = func;
    req.param = (void *)data;
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
BOOL32 WINAPI GetThreadTimes( 
    HANDLE32 thread,         /* [in]  Specifies the thread of interest */
    LPFILETIME creationtime, /* [out] When the thread was created */
    LPFILETIME exittime,     /* [out] When the thread was destroyed */
    LPFILETIME kerneltime,   /* [out] Time thread spent in kernel mode */
    LPFILETIME usertime)     /* [out] Time thread spent in user mode */
{
    FIXME(thread,"(0x%08x): stub\n",thread);
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
 */
BOOL32 WINAPI AttachThreadInput( 
    DWORD idAttach,   /* [in] Thread to attach */
    DWORD idAttachTo, /* [in] Thread to attach to */
    BOOL32 fAttach)   /* [in] Attach or detach */
{
    BOOL32 ret;

    FIXME(thread, "(0x%08lx,0x%08lx,%d): stub\n",idAttach,idAttachTo,fAttach);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    if (fAttach) {
        /* Attach threads */
        ret = FALSE;
    }
    else {
        /* Detach threads */
        ret = FALSE;
    };
    return ret;
}

/**********************************************************************
 * VWin32_BoostThreadGroup [KERNEL.535]
 */
VOID WINAPI VWin32_BoostThreadGroup( DWORD threadId, INT32 boost )
{
    FIXME(thread, "(0x%08lx,%d): stub\n", threadId, boost);
}

/**********************************************************************
 * VWin32_BoostThreadStatic [KERNEL.536]
 */
VOID WINAPI VWin32_BoostThreadStatic( DWORD threadId, INT32 boost )
{
    FIXME(thread, "(0x%08lx,%d): stub\n", threadId, boost);
}

