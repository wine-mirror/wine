/*
 * Win32 processes
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <assert.h>
#include "process.h"
#include "heap.h"
#include "task.h"
#include "winerror.h"

PDB32 *pCurrentProcess = NULL;

#define HTABLE_SIZE  0x30  /* Handle table initial size */
#define HTABLE_INC   0x10  /* Handle table increment */

#define BOOT_HTABLE_SIZE  5

static HANDLE_ENTRY boot_handles[BOOT_HTABLE_SIZE];

/***********************************************************************
 *           PROCESS_AllocHandleTable
 */
static HANDLE_TABLE *PROCESS_AllocHandleTable( PDB32 *process )
{
    HANDLE_TABLE *table = HeapAlloc( process->system_heap, HEAP_ZERO_MEMORY,
                                     sizeof(HANDLE_TABLE) +
                                     (HTABLE_SIZE-1) * sizeof(HANDLE_ENTRY) );
    if (!table) return NULL;
    table->count = HTABLE_SIZE;
    return table;
}


/***********************************************************************
 *           PROCESS_GrowHandleTable
 */
static BOOL32 PROCESS_GrowHandleTable( PDB32 *process )
{
    HANDLE_TABLE *table = process->handle_table;
    table = HeapReAlloc( process->system_heap, HEAP_ZERO_MEMORY, table,
                         sizeof(HANDLE_TABLE) +
                         (table->count+HTABLE_INC-1) * sizeof(HANDLE_ENTRY) );
    if (!table) return FALSE;
    table->count += HTABLE_INC;
    process->handle_table = table;
    return TRUE;
}


/***********************************************************************
 *           PROCESS_AllocBootHandle
 *
 * Allocate a handle from the boot table.
 */
static HANDLE32 PROCESS_AllocBootHandle( K32OBJ *ptr, DWORD flags )
{
    HANDLE32 h;
    for (h = 0; h < BOOT_HTABLE_SIZE; h++)
        if (!boot_handles[h].ptr) break;
    assert( h < BOOT_HTABLE_SIZE );
    K32OBJ_IncCount( ptr );
    boot_handles[h].flags = flags;
    boot_handles[h].ptr   = ptr;
    return h + 1;  /* Avoid handle 0 */
}


/***********************************************************************
 *           PROCESS_CloseBootHandle
 *
 * Close a handle from the boot table.
 */
static BOOL32 PROCESS_CloseBootHandle( HANDLE32 handle )
{
    HANDLE_ENTRY *entry = &boot_handles[handle - 1];
    assert( (handle > 0) && (handle <= BOOT_HTABLE_SIZE) );
    assert( entry->ptr );
    K32OBJ_DecCount( entry->ptr );
    entry->flags = 0;
    entry->ptr   = NULL;
    return TRUE;
}


/***********************************************************************
 *           PROCESS_GetBootObjPtr
 *
 * Get a handle ptr from the boot table.
 */
static K32OBJ *PROCESS_GetBootObjPtr( HANDLE32 handle, K32OBJ_TYPE type )
{
    K32OBJ *ptr;

    assert( (handle > 0) && (handle <= BOOT_HTABLE_SIZE) );
    ptr = boot_handles[handle - 1].ptr;
    assert (ptr && (ptr->type == type));
    K32OBJ_IncCount( ptr );
    return ptr;
}


/***********************************************************************
 *           PROCESS_SetBootObjPtr
 *
 * Set a handle ptr from the boot table.
 */
static BOOL32 PROCESS_SetBootObjPtr( HANDLE32 handle, K32OBJ *ptr, DWORD flags)
{
    K32OBJ *old_ptr;

    assert( (handle > 0) && (handle <= BOOT_HTABLE_SIZE) );
    K32OBJ_IncCount( ptr );
    if ((old_ptr = boot_handles[handle - 1].ptr)) K32OBJ_DecCount( old_ptr );
    boot_handles[handle - 1].flags = flags;
    boot_handles[handle - 1].ptr   = ptr;
    return TRUE;
}


/***********************************************************************
 *           PROCESS_AllocHandle
 *
 * Allocate a handle for a kernel object and increment its refcount.
 */
HANDLE32 PROCESS_AllocHandle( K32OBJ *ptr, DWORD flags )
{
    HANDLE32 h;
    HANDLE_ENTRY *entry;

    assert( ptr );
    if (!pCurrentProcess) return PROCESS_AllocBootHandle( ptr, flags );
    EnterCriticalSection( &pCurrentProcess->crit_section );
    K32OBJ_IncCount( ptr );
    entry = pCurrentProcess->handle_table->entries;
    for (h = 0; h < pCurrentProcess->handle_table->count; h++, entry++)
        if (!entry->ptr) break;
    if ((h < pCurrentProcess->handle_table->count) ||
        PROCESS_GrowHandleTable( pCurrentProcess ))
    {
        entry->flags = flags;
        entry->ptr   = ptr;
        LeaveCriticalSection( &pCurrentProcess->crit_section );
        return h + 1;  /* Avoid handle 0 */
    }
    LeaveCriticalSection( &pCurrentProcess->crit_section );
    SetLastError( ERROR_OUTOFMEMORY );
    K32OBJ_DecCount( ptr );
    return INVALID_HANDLE_VALUE32;
}


/***********************************************************************
 *           PROCESS_GetObjPtr
 *
 * Retrieve a pointer to a kernel object and increments its reference count.
 * The refcount must be decremented when the pointer is no longer used.
 */
K32OBJ *PROCESS_GetObjPtr( HANDLE32 handle, K32OBJ_TYPE type )
{
    K32OBJ *ptr = NULL;
    if (!pCurrentProcess) return PROCESS_GetBootObjPtr( handle, type );
    EnterCriticalSection( &pCurrentProcess->crit_section );
    if ((handle > 0) && (handle <= pCurrentProcess->handle_table->count))
    {
        ptr = pCurrentProcess->handle_table->entries[handle - 1].ptr;
        if (ptr && ((type == K32OBJ_UNKNOWN) || (ptr->type == type)))
            K32OBJ_IncCount( ptr );
        else ptr = NULL;
    }
    LeaveCriticalSection( &pCurrentProcess->crit_section );
    if (!ptr) SetLastError( ERROR_INVALID_HANDLE );
    return ptr;
}


/***********************************************************************
 *           PROCESS_SetObjPtr
 *
 * Change the object pointer of a handle, and increment the refcount.
 * Use with caution!
 */
BOOL32 PROCESS_SetObjPtr( HANDLE32 handle, K32OBJ *ptr, DWORD flags )
{
    BOOL32 ret = TRUE;
    K32OBJ *old_ptr = NULL;

    if (!pCurrentProcess) return PROCESS_SetBootObjPtr( handle, ptr, flags );
    EnterCriticalSection( &pCurrentProcess->crit_section );
    if ((handle > 0) && (handle <= pCurrentProcess->handle_table->count))
    {
        HANDLE_ENTRY*entry = &pCurrentProcess->handle_table->entries[handle-1];
        old_ptr = entry->ptr;
        K32OBJ_IncCount( ptr );
        entry->flags = flags;
        entry->ptr   = ptr;
    }
    else
    {
        SetLastError( ERROR_INVALID_HANDLE );
        ret = FALSE;
    }
    LeaveCriticalSection( &pCurrentProcess->crit_section );
    if (old_ptr) K32OBJ_DecCount( old_ptr );
    return ret;
}


/*********************************************************************
 *           CloseHandle   (KERNEL32.23)
 */
BOOL32 CloseHandle( HANDLE32 handle )
{
    BOOL32 ret = FALSE;
    K32OBJ *ptr = NULL;

    if (!pCurrentProcess) return PROCESS_CloseBootHandle( handle );
    EnterCriticalSection( &pCurrentProcess->crit_section );
    if ((handle > 0) && (handle <= pCurrentProcess->handle_table->count))
    {
        HANDLE_ENTRY*entry = &pCurrentProcess->handle_table->entries[handle-1];
        if ((ptr = entry->ptr))
        {
            entry->flags = 0;
            entry->ptr   = NULL;
            ret = TRUE;
        }
    }
    LeaveCriticalSection( &pCurrentProcess->crit_section );
    if (!ret) SetLastError( ERROR_INVALID_HANDLE );
    if (ptr) K32OBJ_DecCount( ptr );
    return ret;
}


/***********************************************************************
 *           PROCESS_Create
 */
PDB32 *PROCESS_Create(void)
{
    PDB32 *pdb = HeapAlloc( SystemHeap, HEAP_ZERO_MEMORY, sizeof(PDB32) );
    if (!pdb) return NULL;
    pdb->header.type     = K32OBJ_PROCESS;
    pdb->header.refcount = 1;
    pdb->exit_code       = 0x103; /* STILL_ACTIVE */
    pdb->threads         = 1;
    pdb->running_threads = 1;
    pdb->ring0_threads   = 1;
    pdb->system_heap     = SystemHeap;
    pdb->parent          = pCurrentProcess;
    pdb->group           = pdb;
    pdb->priority        = 8;  /* Normal */
    InitializeCriticalSection( &pdb->crit_section );
    if (!(pdb->heap = HeapCreate( HEAP_GROWABLE, 0x10000, 0 ))) goto error;
    if (!(pdb->env_DB = HeapAlloc(pdb->heap, HEAP_ZERO_MEMORY, sizeof(ENVDB))))
        goto error;
    if (!(pdb->handle_table = PROCESS_AllocHandleTable( pdb ))) goto error;
    pdb->heap_list       = pdb->heap;
    return pdb;

error:
    if (pdb->env_DB) HeapFree( pdb->heap, 0, pdb->env_DB );
    if (pdb->handle_table) HeapFree( pdb->system_heap, 0, pdb->handle_table );
    if (pdb->heap) HeapDestroy( pdb->heap );
    DeleteCriticalSection( &pdb->crit_section );
    HeapFree( SystemHeap, 0, pdb );
    return NULL;
}


/***********************************************************************
 *           PROCESS_Destroy
 */
void PROCESS_Destroy( K32OBJ *ptr )
{
    PDB32 *pdb = (PDB32 *)ptr;
    HANDLE32 handle;
    assert( ptr->type == K32OBJ_PROCESS );

    /* Close all handles */
    for (handle = 0; handle < pdb->handle_table->count; handle++)
        if (pdb->handle_table->entries[handle].ptr) CloseHandle( handle );

    /* Free everything */

    ptr->type = K32OBJ_UNKNOWN;
    HeapFree( pdb->heap, 0, pdb->env_DB );
    HeapFree( pdb->system_heap, 0, pdb->handle_table );
    HeapDestroy( pdb->heap );
    DeleteCriticalSection( &pdb->crit_section );
    HeapFree( SystemHeap, 0, pdb );
}


/***********************************************************************
 *           ExitProcess   (KERNEL32.100)
 */
void ExitProcess( DWORD status )
{
    TASK_KillCurrentTask( status );
}


/***********************************************************************
 *           GetCurrentProcess   (KERNEL32.198)
 */
HANDLE32 GetCurrentProcess(void)
{
    return 0x7fffffff;
}


/***********************************************************************
 *           GetCurrentProcessId   (KERNEL32.199)
 */
DWORD GetCurrentProcessId(void)
{
    return (DWORD)pCurrentProcess;
}


/***********************************************************************
 *           GetProcessHeap    (KERNEL32.259)
 */
HANDLE32 GetProcessHeap(void)
{
    if (!pCurrentProcess) return SystemHeap;  /* For the boot-up code */
    return pCurrentProcess->heap;
}
