/*
 * Win32 threads
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <assert.h>
#include "thread.h"
#include "winerror.h"
#include "heap.h"
#include "selectors.h"
#include "winnt.h"

THDB *pCurrentThread = NULL;


/***********************************************************************
 *           THREAD_Create
 */
THDB *THREAD_Create( PDB32 *pdb, DWORD stack_size )
{
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
    thdb->process2        = pdb;
    thdb->exit_code       = 0x103; /* STILL_ACTIVE */

    /* Allocate the stack */

    if (!stack_size) stack_size = 1024 * 1024;  /* default size = 1Mb */
    thdb->stack_base = VirtualAlloc( NULL, stack_size, MEM_COMMIT,
                                     PAGE_EXECUTE_READWRITE );
    if (!thdb->stack_base) goto error;
    /* Un-commit the first page (FIXME: should use PAGE_GUARD instead) */
    VirtualFree( thdb->stack_base, 1, MEM_DECOMMIT );
    thdb->teb.stack_top   = (char *)thdb->stack_base + stack_size;
    thdb->teb.stack_low   = thdb->teb.stack_top;
    thdb->exit_stack      = thdb->teb.stack_top;

    /* Allocate the TEB selector (%fs register) */

    thdb->teb_sel = SELECTOR_AllocBlock( &thdb->teb, 0x1000, SEGMENT_DATA,
                                         TRUE, FALSE );
    if (!thdb->teb_sel) goto error;
    return thdb;

error:
    if (thdb->teb_sel) SELECTOR_FreeBlock( thdb->teb_sel, 1 );
    if (thdb->stack_base) VirtualFree( thdb->stack_base, 0, MEM_RELEASE );
    HeapFree( SystemHeap, 0, thdb );
    return NULL;
}


/***********************************************************************
 *           THREAD_Destroy
 */
void THREAD_Destroy( K32OBJ *ptr )
{
    THDB *thdb = (THDB *)ptr;
    assert( ptr->type == K32OBJ_THREAD );
    ptr->type = K32OBJ_UNKNOWN;
    SELECTOR_FreeBlock( thdb->teb_sel, 1 );
    HeapFree( SystemHeap, 0, thdb );
}


/***********************************************************************
 *           GetCurrentThread   (KERNEL32.200)
 */
HANDLE32 GetCurrentThread(void)
{
    return 0xFFFFFFFE;
}


/***********************************************************************
 *           GetCurrentThreadId   (KERNEL32.201)
 * Returns crypted (xor'ed) pointer to THDB in Win95.
 */
DWORD GetCurrentThreadId(void)
{
    /* FIXME: should probably use %fs register here */
    assert( pCurrentThread );
    return (DWORD)pCurrentThread;
}


/**********************************************************************
 *           GetLastError   (KERNEL.148) (KERNEL32.227)
 */
DWORD GetLastError(void)
{
    THDB *thread = (THDB *)GetCurrentThreadId();
    return thread->last_error;
}


/**********************************************************************
 *           SetLastError   (KERNEL.147) (KERNEL32.497)
 */
void SetLastError( DWORD error )
{
    THDB *thread;
    if (!pCurrentThread) return;  /* FIXME */
    thread = (THDB *)GetCurrentThreadId();
    thread->last_error = error;
}


/**********************************************************************
 *           SetLastErrorEx   (USER32.484)
 */
void SetLastErrorEx( DWORD error, DWORD type )
{
    /* FIXME: what about 'type'? */
    SetLastError( error );
}


/**********************************************************************
 *           TlsAlloc   (KERNEL32.530)
 */
DWORD TlsAlloc(void)
{
    DWORD i, mask, ret = 0;
    THDB *thread = (THDB *)GetCurrentThreadId();
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
BOOL32 TlsFree( DWORD index )
{
    DWORD mask;
    THDB *thread = (THDB *)GetCurrentThreadId();
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
LPVOID TlsGetValue( DWORD index )
{
    THDB *thread = (THDB *)GetCurrentThreadId();
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
BOOL32 TlsSetValue( DWORD index, LPVOID value )
{
    THDB *thread = (THDB *)GetCurrentThreadId();
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
BOOL32 GetThreadContext( HANDLE32 handle, CONTEXT *context )
{
    return FALSE;
}


/**********************************************************************
 *           NtCurrentTeb   (NTDLL.89)
 */
void NtCurrentTeb( CONTEXT *context )
{
    EAX_reg(context) = GetSelectorBase( FS_reg(context) );
}

/**********************************************************************
 *           GetThreadPriority   (KERNEL32.296)
 */
INT32
GetThreadPriority(HANDLE32 hthread) {
    THDB *thread;
    INT32 ret;
    
    ret = 0;
    thread = (THDB*)PROCESS_GetObjPtr(hthread,K32OBJ_THREAD);
    if (thread) {
    	ret = thread->delta_priority;
	K32OBJ_DecCount((K32OBJ*)thread);
    }
    return ret;
}

/**********************************************************************
 *           SetThreadPriority   (KERNEL32.514)
 */
BOOL32
SetThreadPriority(HANDLE32 hthread,INT32 priority) {
    THDB *thread;
    
    thread = (THDB*)PROCESS_GetObjPtr(hthread,K32OBJ_THREAD);
    if (thread) {
	thread->delta_priority = priority;
	K32OBJ_DecCount((K32OBJ*)thread);
    }
    return TRUE;
}
