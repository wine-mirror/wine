/*
 * Win32 threads
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "thread.h"
#include "process.h"
#include "winerror.h"
#include "heap.h"
#include "selectors.h"
#include "miscemu.h"
#include "winnt.h"
#include "debug.h"
#include "stddebug.h"

#ifndef __i386__
THDB *pCurrentThread;
#endif

static BOOL32 THREAD_Signaled( K32OBJ *obj, DWORD thread_id );
static BOOL32 THREAD_Satisfied( K32OBJ *obj, DWORD thread_id );
static void THREAD_AddWait( K32OBJ *obj, DWORD thread_id );
static void THREAD_RemoveWait( K32OBJ *obj, DWORD thread_id );
static void THREAD_Destroy( K32OBJ *obj );

const K32OBJ_OPS THREAD_Ops =
{
    THREAD_Signaled,    /* signaled */
    THREAD_Satisfied,   /* satisfied */
    THREAD_AddWait,     /* add_wait */
    THREAD_RemoveWait,  /* remove_wait */
    THREAD_Destroy      /* destroy */
};


/***********************************************************************
 *           THREAD_GetPtr
 *
 * Return a pointer to a thread object. The object count must be decremented
 * when no longer used.
 */
static THDB *THREAD_GetPtr( HANDLE32 handle )
{
    THDB *thread;

    if (handle == 0xfffffffe)  /* Self-thread handle */
    {
        thread = THREAD_Current();
        K32OBJ_IncCount( &thread->header );
    }
    else thread = (THDB *)PROCESS_GetObjPtr( handle, K32OBJ_THREAD );
    return thread;
}


/***********************************************************************
 *           THREAD_Current
 *
 * Return the current thread THDB pointer.
 */
THDB *THREAD_Current(void)
{
    TEB *teb = NtCurrentTeb();
    if (!teb) return NULL;
    return (THDB *)((char *)teb - (int)&((THDB *)0)->teb);
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
THDB *THREAD_Create( PDB32 *pdb, DWORD stack_size,
                     LPTHREAD_START_ROUTINE start_addr, LPVOID param )
{
    DWORD old_prot;
    WORD cs, ds;

    THDB *thdb = HeapAlloc( SystemHeap, HEAP_ZERO_MEMORY, sizeof(THDB) );
    if (!thdb) return NULL;
    thdb->header.type     = K32OBJ_THREAD;
    thdb->header.refcount = 1;
    thdb->process         = pdb;
    thdb->teb.except      = (void *)-1;
    thdb->teb.htask16     = 0; /* FIXME */
    thdb->teb.stack_sel   = 0; /* FIXME */
    thdb->teb.self        = &thdb->teb;
    thdb->teb.tls_ptr     = thdb->tls_array;
    thdb->wait_list       = &thdb->wait_struct;
    thdb->exit_code       = 0x103; /* STILL_ACTIVE */
    thdb->entry_point     = start_addr;
    thdb->entry_arg       = param;

    /* Allocate the stack */

    if (!stack_size) stack_size = 1024 * 1024;  /* default size = 1Mb */
    thdb->stack_base = VirtualAlloc( NULL, stack_size, MEM_COMMIT,
                                     PAGE_EXECUTE_READWRITE );
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

    /* Allocate the event */

    if (!(thdb->event = EVENT_Create( TRUE, FALSE ))) goto error;

    /* Initialize the thread context */

    GET_CS(cs);
    GET_DS(ds);
    thdb->pcontext        = &thdb->context;
    thdb->context.SegCs   = cs;
    thdb->context.SegDs   = ds;
    thdb->context.SegEs   = ds;
    thdb->context.SegGs   = ds;
    thdb->context.SegSs   = ds;
    thdb->context.SegFs   = thdb->teb_sel;
    thdb->context.Eip     = (DWORD)start_addr;
    thdb->context.Esp     = (DWORD)thdb->teb.stack_top;
    PE_InitTls( thdb );
    return thdb;

error:
    if (thdb->event) K32OBJ_DecCount( thdb->event );
    if (thdb->teb_sel) SELECTOR_FreeBlock( thdb->teb_sel, 1 );
    if (thdb->stack_base) VirtualFree( thdb->stack_base, 0, MEM_RELEASE );
    HeapFree( SystemHeap, 0, thdb );
    return NULL;
}


/***********************************************************************
 *           THREAD_Signaled
 */
static BOOL32 THREAD_Signaled( K32OBJ *obj, DWORD thread_id )
{
    THDB *thdb = (THDB *)obj;
    assert( obj->type == K32OBJ_THREAD );
    return K32OBJ_OPS( thdb->event )->signaled( thdb->event, thread_id );
}


/***********************************************************************
 *           THREAD_Satisfied
 *
 * Wait on this object has been satisfied.
 */
static BOOL32 THREAD_Satisfied( K32OBJ *obj, DWORD thread_id )
{
    THDB *thdb = (THDB *)obj;
    assert( obj->type == K32OBJ_THREAD );
    return K32OBJ_OPS( thdb->event )->satisfied( thdb->event, thread_id );
}


/***********************************************************************
 *           THREAD_AddWait
 *
 * Add thread to object wait queue.
 */
static void THREAD_AddWait( K32OBJ *obj, DWORD thread_id )
{
    THDB *thdb = (THDB *)obj;
    assert( obj->type == K32OBJ_THREAD );
    return K32OBJ_OPS( thdb->event )->add_wait( thdb->event, thread_id );
}


/***********************************************************************
 *           THREAD_RemoveWait
 *
 * Remove thread from object wait queue.
 */
static void THREAD_RemoveWait( K32OBJ *obj, DWORD thread_id )
{
    THDB *thdb = (THDB *)obj;
    assert( obj->type == K32OBJ_THREAD );
    return K32OBJ_OPS( thdb->event )->remove_wait( thdb->event, thread_id );
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
    K32OBJ_DecCount( thdb->event );
    SELECTOR_FreeBlock( thdb->teb_sel, 1 );
    HeapFree( SystemHeap, 0, thdb );

}


/***********************************************************************
 *           CreateThread   (KERNEL32.63)
 */
HANDLE32 WINAPI CreateThread( LPSECURITY_ATTRIBUTES attribs, DWORD stack,
                              LPTHREAD_START_ROUTINE start, LPVOID param,
                              DWORD flags, LPDWORD id )
{
    HANDLE32 handle;
    THDB *thread = THREAD_Create( PROCESS_Current(), stack, start, param );
    if (!thread) return INVALID_HANDLE_VALUE32;
    handle = PROCESS_AllocHandle( &thread->header, 0 );
    if (handle == INVALID_HANDLE_VALUE32) goto error;
    if (SYSDEPS_SpawnThread( thread ) == -1) goto error;
    *id = THDB_TO_THREAD_ID( thread );
    return handle;

error:
    K32OBJ_DecCount( &thread->header );
    return INVALID_HANDLE_VALUE32;
}


/***********************************************************************
 *           ExitThread   (KERNEL32.215)
 */
void WINAPI ExitThread( DWORD code )
{
    THDB *thdb = THREAD_Current();
    LONG count;

    SYSTEM_LOCK();
    thdb->exit_code = code;
    EVENT_Set( thdb->event );

    /* Abandon all owned mutexes */
    while (thdb->mutex_list) MUTEX_Abandon( thdb->mutex_list );

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
 *           GetCurrentThread   (KERNEL32.200)
 */
HANDLE32 WINAPI GetCurrentThread(void)
{
    return 0xFFFFFFFE;
}


/***********************************************************************
 *           GetCurrentThreadId   (KERNEL32.201)
 */
DWORD WINAPI GetCurrentThreadId(void)
{
    return THDB_TO_THREAD_ID( THREAD_Current() );
}


/**********************************************************************
 *           GetLastError   (KERNEL.148) (KERNEL32.227)
 */
DWORD WINAPI GetLastError(void)
{
    THDB *thread = THREAD_Current();
    return thread->last_error;
}


/**********************************************************************
 *           SetLastError   (KERNEL.147) (KERNEL32.497)
 */
void WINAPI SetLastError( DWORD error )
{
    THDB *thread = THREAD_Current();
    /* This one must work before we have a thread (FIXME) */
    if (thread) thread->last_error = error;
}


/**********************************************************************
 *           SetLastErrorEx   (USER32.484)
 */
void WINAPI SetLastErrorEx( DWORD error, DWORD type )
{
    /* FIXME: what about 'type'? */
    SetLastError( error );
}


/**********************************************************************
 *           TlsAlloc   (KERNEL32.530)
 */
DWORD WINAPI TlsAlloc(void)
{
    DWORD i, mask, ret = 0;
    THDB *thread = THREAD_Current();
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
 *           TlsFree   (KERNEL32.531)
 */
BOOL32 WINAPI TlsFree( DWORD index )
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
 *           TlsGetValue   (KERNEL32.532)
 */
LPVOID WINAPI TlsGetValue( DWORD index )
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
 *           TlsSetValue   (KERNEL32.533)
 */
BOOL32 WINAPI TlsSetValue( DWORD index, LPVOID value )
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
 *           GetThreadContext   (KERNEL32.294)
 */
BOOL32 WINAPI GetThreadContext( HANDLE32 handle, CONTEXT *context )
{
    THDB *thread = THREAD_GetPtr( handle );
    if (!thread) return FALSE;
    *context = thread->context;
    K32OBJ_DecCount( &thread->header );
    return TRUE;
}


/**********************************************************************
 *           GetThreadPriority   (KERNEL32.296)
 */
INT32 WINAPI GetThreadPriority(HANDLE32 hthread)
{
    THDB *thread;
    INT32 ret;
    
    if (!(thread = THREAD_GetPtr( hthread ))) return 0;
    ret = thread->delta_priority;
    K32OBJ_DecCount( &thread->header );
    return ret;
}


/**********************************************************************
 *           SetThreadPriority   (KERNEL32.514)
 */
BOOL32 WINAPI SetThreadPriority(HANDLE32 hthread,INT32 priority)
{
    THDB *thread;
    
    if (!(thread = THREAD_GetPtr( hthread ))) return FALSE;
    thread->delta_priority = priority;
    K32OBJ_DecCount( &thread->header );
    return TRUE;
}

/**********************************************************************
 *           TerminateThread   (KERNEL32)
 */
BOOL32 WINAPI TerminateThread(HANDLE32 handle,DWORD exitcode)
{
    fprintf(stdnimp,"TerminateThread(0x%08x,%ld), STUB!\n",handle,exitcode);
    return TRUE;
}

/**********************************************************************
 *           GetExitCodeThread   (KERNEL32)
 */
BOOL32 WINAPI GetExitCodeThread(HANDLE32 hthread,LPDWORD exitcode)
{
    THDB *thread;
    
    if (!(thread = THREAD_GetPtr( hthread ))) return FALSE;
    if (exitcode) *exitcode = thread->exit_code;
    K32OBJ_DecCount( &thread->header );
    return TRUE;
}

/**********************************************************************
 *           ResumeThread   (KERNEL32)
 */
BOOL32 WINAPI ResumeThread( HANDLE32 handle )
{
    fprintf(stdnimp,"ResumeThread(0x%08x), STUB!\n",handle);
    return TRUE;
}

/**********************************************************************
 *           SuspendThread   (KERNEL32)
 */
BOOL32 WINAPI SuspendThread( HANDLE32 handle )
{
    fprintf(stdnimp,"SuspendThread(0x%08x), STUB!\n",handle);
    return TRUE;
}
