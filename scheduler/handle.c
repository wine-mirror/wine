/*
 * Win32 process handles
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include <stdio.h>
#include "winbase.h"
#include "winerror.h"
#include "heap.h"
#include "process.h"

#define HTABLE_SIZE  0x30  /* Handle table initial size */
#define HTABLE_INC   0x10  /* Handle table increment */

/* Reserved access rights */
#define RESERVED_ALL           (0x0007 << RESERVED_SHIFT)
#define RESERVED_SHIFT         25
#define RESERVED_INHERIT       (HANDLE_FLAG_INHERIT<<RESERVED_SHIFT)
#define RESERVED_CLOSE_PROTECT (HANDLE_FLAG_PROTECT_FROM_CLOSE<<RESERVED_SHIFT)


/***********************************************************************
 *           HANDLE_AllocTable
 */
HANDLE_TABLE *HANDLE_AllocTable( PDB32 *process )
{
    HANDLE_TABLE *table = HeapAlloc( process->system_heap, HEAP_ZERO_MEMORY,
                                     sizeof(HANDLE_TABLE) +
                                     (HTABLE_SIZE-1) * sizeof(HANDLE_ENTRY) );
    if (!table) return NULL;
    table->count = HTABLE_SIZE;
    return table;
}


/***********************************************************************
 *           HANDLE_GrowTable
 */
static BOOL32 HANDLE_GrowTable( PDB32 *process )
{
    HANDLE_TABLE *table;

    SYSTEM_LOCK();
    table = process->handle_table;
    table = HeapReAlloc( process->system_heap,
                         HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE, table,
                         sizeof(HANDLE_TABLE) +
                         (table->count+HTABLE_INC-1) * sizeof(HANDLE_ENTRY) );
    if (table)
    {
        table->count += HTABLE_INC;
        process->handle_table = table;
    }
    SYSTEM_UNLOCK();
    return (table != NULL);
}


/***********************************************************************
 *           HANDLE_Alloc
 *
 * Allocate a handle for a kernel object and increment its refcount.
 */
HANDLE32 HANDLE_Alloc( K32OBJ *ptr, DWORD access, BOOL32 inherit )
{
    HANDLE32 h;
    HANDLE_ENTRY *entry;
    PDB32 *pdb = PROCESS_Current();

    assert( ptr );

    /* Set the inherit reserved flag */
    access &= ~RESERVED_ALL;
    if (inherit) access |= RESERVED_INHERIT;

    SYSTEM_LOCK();
    K32OBJ_IncCount( ptr );
    entry = pdb->handle_table->entries;
    for (h = 0; h < pdb->handle_table->count; h++, entry++)
        if (!entry->ptr) break;
    if ((h < pdb->handle_table->count) || HANDLE_GrowTable( pdb ))
    {
        entry = &pdb->handle_table->entries[h];
        entry->access = access;
        entry->ptr    = ptr;
        SYSTEM_UNLOCK();
        return h + 1;  /* Avoid handle 0 */
    }
    K32OBJ_DecCount( ptr );
    SYSTEM_UNLOCK();
    SetLastError( ERROR_OUTOFMEMORY );
    return INVALID_HANDLE_VALUE32;
}


/***********************************************************************
 *           HANDLE_GetObjPtr
 *
 * Retrieve a pointer to a kernel object and increments its reference count.
 * The refcount must be decremented when the pointer is no longer used.
 */
K32OBJ *HANDLE_GetObjPtr( HANDLE32 handle, K32OBJ_TYPE type, DWORD access )
{
    K32OBJ *ptr = NULL;
    PDB32 *pdb = PROCESS_Current();

    SYSTEM_LOCK();
    if ((handle > 0) && (handle <= pdb->handle_table->count))
    {
        HANDLE_ENTRY *entry = &pdb->handle_table->entries[handle-1];
        if ((entry->access & access) != access)
            fprintf( stderr, "Warning: handle %08x bad access (acc=%08lx req=%08lx)\n",
                     handle, entry->access, access );
        ptr = entry->ptr;
        if (ptr && ((type == K32OBJ_UNKNOWN) || (ptr->type == type)))
            K32OBJ_IncCount( ptr );
        else
            ptr = NULL;
    }
    SYSTEM_UNLOCK();
    if (!ptr) SetLastError( ERROR_INVALID_HANDLE );
    return ptr;
}


/***********************************************************************
 *           HANDLE_SetObjPtr
 *
 * Change the object pointer of a handle, and increment the refcount.
 * Use with caution!
 */
BOOL32 HANDLE_SetObjPtr( HANDLE32 handle, K32OBJ *ptr, DWORD access )
{
    BOOL32 ret = FALSE;
    PDB32 *pdb = PROCESS_Current();

    SYSTEM_LOCK();
    if ((handle > 0) && (handle <= pdb->handle_table->count))
    {
        HANDLE_ENTRY *entry = &pdb->handle_table->entries[handle-1];
        K32OBJ *old_ptr = entry->ptr;
        K32OBJ_IncCount( ptr );
        entry->access = access;
        entry->ptr    = ptr;
        if (old_ptr) K32OBJ_DecCount( old_ptr );
        ret = TRUE;
    }
    SYSTEM_UNLOCK();
    if (!ret) SetLastError( ERROR_INVALID_HANDLE );
    return ret;
}


/*********************************************************************
 *           CloseHandle   (KERNEL32.23)
 */
BOOL32 WINAPI CloseHandle( HANDLE32 handle )
{
    BOOL32 ret = FALSE;
    PDB32 *pdb = PROCESS_Current();
    K32OBJ *ptr;

    SYSTEM_LOCK();
    if ((handle > 0) && (handle <= pdb->handle_table->count))
    {
        HANDLE_ENTRY *entry = &pdb->handle_table->entries[handle-1];
        if ((ptr = entry->ptr))
        {
            if (!(entry->access & RESERVED_CLOSE_PROTECT))
            {
                entry->access = 0;
                entry->ptr    = NULL;
                K32OBJ_DecCount( ptr );
                ret = TRUE;
            }
            /* FIXME: else SetLastError */
        }
    }
    SYSTEM_UNLOCK();
    if (!ret) SetLastError( ERROR_INVALID_HANDLE );
    return ret;
}


/*********************************************************************
 *           GetHandleInformation   (KERNEL32.336)
 */
BOOL32 WINAPI GetHandleInformation( HANDLE32 handle, LPDWORD flags )
{
    BOOL32 ret = FALSE;
    PDB32 *pdb = PROCESS_Current();

    SYSTEM_LOCK();
    if ((handle > 0) && (handle <= pdb->handle_table->count))
    {
        HANDLE_ENTRY *entry = &pdb->handle_table->entries[handle-1];
        if (entry->ptr)
        {
            if (flags)
                *flags = (entry->access & RESERVED_ALL) >> RESERVED_SHIFT;
            ret = TRUE;
        }
    }
    SYSTEM_UNLOCK();
    if (!ret) SetLastError( ERROR_INVALID_HANDLE );
    return ret;
}


/*********************************************************************
 *           SetHandleInformation   (KERNEL32.653)
 */
BOOL32 WINAPI SetHandleInformation( HANDLE32 handle, DWORD mask, DWORD flags )
{
    BOOL32 ret = FALSE;
    PDB32 *pdb = PROCESS_Current();

    mask  = (mask << RESERVED_SHIFT) & RESERVED_ALL;
    flags = (flags << RESERVED_SHIFT) & RESERVED_ALL;
    SYSTEM_LOCK();
    if ((handle > 0) && (handle <= pdb->handle_table->count))
    {
        HANDLE_ENTRY *entry = &pdb->handle_table->entries[handle-1];
        if (entry->ptr)
        {
            entry->access = (entry->access & ~mask) | flags;
            ret = TRUE;
        }
    }
    SYSTEM_UNLOCK();
    if (!ret) SetLastError( ERROR_INVALID_HANDLE );
    return ret;
}
