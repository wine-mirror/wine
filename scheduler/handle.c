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
#include "server.h"
#include "thread.h"
#include "debug.h"

#define HTABLE_SIZE  0x30  /* Handle table initial size */
#define HTABLE_INC   0x10  /* Handle table increment */

/* Reserved access rights */
#define RESERVED_ALL           (0x0007 << RESERVED_SHIFT)
#define RESERVED_SHIFT         25
#define RESERVED_INHERIT       (HANDLE_FLAG_INHERIT<<RESERVED_SHIFT)
#define RESERVED_CLOSE_PROTECT (HANDLE_FLAG_PROTECT_FROM_CLOSE<<RESERVED_SHIFT)


/***********************************************************************
 *           HANDLE_GrowTable
 */
static BOOL32 HANDLE_GrowTable( PDB32 *process, INT32 incr )
{
    HANDLE_TABLE *table;

    SYSTEM_LOCK();
    table = process->handle_table;
    table = HeapReAlloc( process->system_heap,
                         HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE, table,
                         sizeof(HANDLE_TABLE) +
                         (table->count + incr - 1) * sizeof(HANDLE_ENTRY) );
    if (table)
    {
        table->count += incr;
        process->handle_table = table;
    }
    SYSTEM_UNLOCK();
    return (table != NULL);
}


/***********************************************************************
 *           HANDLE_CreateTable
 *
 * Create a process handle table, optionally inheriting the parent's handles.
 */
BOOL32 HANDLE_CreateTable( PDB32 *pdb, BOOL32 inherit )
{
    DWORD size;

    /* Process must not already have a handle table */
    assert( !pdb->handle_table );

    /* If this is the first process, simply allocate a table */
    if (!pdb->parent) inherit = FALSE;

    SYSTEM_LOCK();
    size = inherit ? pdb->parent->handle_table->count : HTABLE_SIZE;
    if ((pdb->handle_table = HeapAlloc( pdb->system_heap,
                                        HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE,
                                        sizeof(HANDLE_TABLE) +
                                        (size-1) * sizeof(HANDLE_ENTRY) )))
    {
        pdb->handle_table->count = size;
        if (inherit)
        {
            HANDLE_ENTRY *src = pdb->parent->handle_table->entries;
            HANDLE_ENTRY *dst = pdb->handle_table->entries;
            HANDLE32 h;

            for (h = 0; h < size; h++, src++, dst++)
            {
                /* Check if handle is valid and inheritable */
                if (src->ptr && (src->access & RESERVED_INHERIT))
                {
                    dst->access = src->access;
                    dst->ptr    = src->ptr;
                    dst->server = src->server;
                    K32OBJ_IncCount( dst->ptr );
                }
            }
        }
        /* Handle 1 is the process itself (unless the parent decided otherwise) */
        if (!pdb->handle_table->entries[1].ptr)
        {
            pdb->handle_table->entries[1].ptr    = &pdb->header;
            pdb->handle_table->entries[1].access = PROCESS_ALL_ACCESS;
            pdb->handle_table->entries[1].server = -1;  /* FIXME */
            K32OBJ_IncCount( &pdb->header );
        }
    }
    SYSTEM_UNLOCK();
    return (pdb->handle_table != NULL);
}


/***********************************************************************
 *           HANDLE_Alloc
 *
 * Allocate a handle for a kernel object and increment its refcount.
 */
HANDLE32 HANDLE_Alloc( PDB32 *pdb, K32OBJ *ptr, DWORD access,
                       BOOL32 inherit, int server_handle )
{
    HANDLE32 h;
    HANDLE_ENTRY *entry;

    assert( ptr );

    /* Set the inherit reserved flag */
    access &= ~RESERVED_ALL;
    if (inherit) access |= RESERVED_INHERIT;

    SYSTEM_LOCK();
    K32OBJ_IncCount( ptr );
    /* Don't try to allocate handle 0 */
    entry = pdb->handle_table->entries + 1;
    for (h = 1; h < pdb->handle_table->count; h++, entry++)
        if (!entry->ptr) break;
    if ((h < pdb->handle_table->count) || HANDLE_GrowTable( pdb, HTABLE_INC ))
    {
        entry = &pdb->handle_table->entries[h];
        entry->access = access;
        entry->ptr    = ptr;
        entry->server = server_handle;
        SYSTEM_UNLOCK();
        return h;
    }
    K32OBJ_DecCount( ptr );
    SYSTEM_UNLOCK();
    if (server_handle != -1) CLIENT_CloseHandle( server_handle );
    SetLastError( ERROR_OUTOFMEMORY );
    return INVALID_HANDLE_VALUE32;
}


/***********************************************************************
 *           HANDLE_GetObjPtr
 *
 * Retrieve a pointer to a kernel object and increments its reference count.
 * The refcount must be decremented when the pointer is no longer used.
 */
K32OBJ *HANDLE_GetObjPtr( PDB32 *pdb, HANDLE32 handle,
                          K32OBJ_TYPE type, DWORD access,
                          int *server_handle )
{
    K32OBJ *ptr = NULL;

    SYSTEM_LOCK();
    if (HANDLE_IS_GLOBAL( handle ))
    {
        handle = HANDLE_GLOBAL_TO_LOCAL( handle );
        pdb = PROCESS_Initial();
    }
    if ((handle > 0) && (handle < pdb->handle_table->count))
    {
        HANDLE_ENTRY *entry = &pdb->handle_table->entries[handle];
        if ((entry->access & access) != access)
            WARN(win32, "Handle %08x bad access (acc=%08lx req=%08lx)\n",
                     handle, entry->access, access );
        ptr = entry->ptr;
        if (server_handle) *server_handle = entry->server;
    }
    else if (handle == CURRENT_THREAD_PSEUDOHANDLE)
    {
       ptr = (K32OBJ *)THREAD_Current();
       if (server_handle) *server_handle = CURRENT_THREAD_PSEUDOHANDLE;
    }
    else if (handle == CURRENT_PROCESS_PSEUDOHANDLE)
    {
       ptr = (K32OBJ *)PROCESS_Current();
       if (server_handle) *server_handle = CURRENT_PROCESS_PSEUDOHANDLE;
    }

    if (ptr && ((type == K32OBJ_UNKNOWN) || (ptr->type == type)))
        K32OBJ_IncCount( ptr );
    else
        ptr = NULL;

    SYSTEM_UNLOCK();
    if (!ptr) SetLastError( ERROR_INVALID_HANDLE );
    return ptr;
}


/***********************************************************************
 *           HANDLE_GetServerHandle
 *
 * Retrieve the server handle associated to an object.
 */
int HANDLE_GetServerHandle( PDB32 *pdb, HANDLE32 handle,
                            K32OBJ_TYPE type, DWORD access )
{
    int server_handle;
    K32OBJ *obj;

    SYSTEM_LOCK();
    if ((obj = HANDLE_GetObjPtr( pdb, handle, type, access, &server_handle )))
        K32OBJ_DecCount( obj );
    else
        server_handle = -1;
    SYSTEM_UNLOCK();
    return server_handle;
}


/***********************************************************************
 *           HANDLE_SetObjPtr
 *
 * Change the object pointer of a handle, and increment the refcount.
 * Use with caution!
 */
BOOL32 HANDLE_SetObjPtr( PDB32 *pdb, HANDLE32 handle, K32OBJ *ptr,
                         DWORD access )
{
    BOOL32 ret = FALSE;

    SYSTEM_LOCK();
    if ((handle > 0) && (handle < pdb->handle_table->count))
    {
        HANDLE_ENTRY *entry = &pdb->handle_table->entries[handle];
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
 *           HANDLE_GetAccess
 */
static BOOL32 HANDLE_GetAccess( PDB32 *pdb, HANDLE32 handle, LPDWORD access )
{
    BOOL32 ret = FALSE;

    SYSTEM_LOCK();
    if ((handle > 0) && (handle < pdb->handle_table->count))
    {
        HANDLE_ENTRY *entry = &pdb->handle_table->entries[handle];
        if (entry->ptr)
        {
            *access = entry->access & ~RESERVED_ALL;
            ret = TRUE;
        }
    }
    SYSTEM_UNLOCK();
    if (!ret) SetLastError( ERROR_INVALID_HANDLE );
    return ret;
}


/*********************************************************************
 *           HANDLE_Close
 */
static BOOL32 HANDLE_Close( PDB32 *pdb, HANDLE32 handle )
{
    BOOL32 ret = FALSE;
    K32OBJ *ptr;

    SYSTEM_LOCK();
    if ((handle > 0) && (handle < pdb->handle_table->count))
    {
        HANDLE_ENTRY *entry = &pdb->handle_table->entries[handle];
        if ((ptr = entry->ptr))
        {
            if (!(entry->access & RESERVED_CLOSE_PROTECT))
            {
                entry->access = 0;
                entry->ptr    = NULL;
                if (entry->server != -1)
                    CLIENT_CloseHandle( entry->server );
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
 *           HANDLE_CloseAll
 *
 * Close all handles pointing to a given object (or all handles of the
 * process if the object is NULL)
 */
void HANDLE_CloseAll( PDB32 *pdb, K32OBJ *obj )
{
    HANDLE_ENTRY *entry;
    K32OBJ *ptr;
    HANDLE32 handle;

    SYSTEM_LOCK();
    entry = pdb->handle_table->entries;
    for (handle = 0; handle < pdb->handle_table->count; handle++, entry++)
    {
        if (!(ptr = entry->ptr)) continue;  /* empty slot */
        if (obj && (ptr != obj)) continue;  /* not the right object */
        entry->access = 0;
        entry->ptr    = NULL;
        if (entry->server != -1) CLIENT_CloseHandle( entry->server );
        K32OBJ_DecCount( ptr );
    }
    SYSTEM_UNLOCK();
}


/*********************************************************************
 *           CloseHandle   (KERNEL32.23)
 */
BOOL32 WINAPI CloseHandle( HANDLE32 handle )
{
    return HANDLE_Close( PROCESS_Current(), handle );
}


/*********************************************************************
 *           GetHandleInformation   (KERNEL32.336)
 */
BOOL32 WINAPI GetHandleInformation( HANDLE32 handle, LPDWORD flags )
{
    BOOL32 ret = FALSE;
    PDB32 *pdb = PROCESS_Current();

    SYSTEM_LOCK();
    if ((handle > 0) && (handle < pdb->handle_table->count))
    {
        HANDLE_ENTRY *entry = &pdb->handle_table->entries[handle];
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
    if ((handle > 0) && (handle < pdb->handle_table->count))
    {
        HANDLE_ENTRY *entry = &pdb->handle_table->entries[handle];
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


/*********************************************************************
 *           DuplicateHandle   (KERNEL32.192)
 */
BOOL32 WINAPI DuplicateHandle( HANDLE32 source_process, HANDLE32 source,
                               HANDLE32 dest_process, HANDLE32 *dest,
                               DWORD access, BOOL32 inherit, DWORD options )
{
    PDB32 *src_pdb = NULL, *dst_pdb = NULL;
    K32OBJ *obj = NULL;
    BOOL32 ret = FALSE;
    HANDLE32 handle;
    int src_process, src_handle, dst_process, dst_handle;

    SYSTEM_LOCK();

    if (!(src_pdb = PROCESS_GetPtr( source_process, PROCESS_DUP_HANDLE, &src_process )))
        goto done;
    if (!(obj = HANDLE_GetObjPtr( src_pdb, source, K32OBJ_UNKNOWN, 0, &src_handle )))
        goto done;

    /* Now that we are sure the source is valid, handle the options */

    if (options & DUPLICATE_SAME_ACCESS)
        HANDLE_GetAccess( src_pdb, source, &access );
    if (options & DUPLICATE_CLOSE_SOURCE)
        HANDLE_Close( src_pdb, source );

    /* And duplicate the handle in the dest process */

    if (!(dst_pdb = PROCESS_GetPtr( dest_process, PROCESS_DUP_HANDLE, &dst_process )))
        goto done;

    if ((src_process != -1) && (src_handle != -1) && (dst_process != -1))
        dst_handle = CLIENT_DuplicateHandle( src_process, src_handle, dst_process, -1,
                                             access, inherit, options );
    else
        dst_handle = -1;

    if ((handle = HANDLE_Alloc( dst_pdb, obj, access, inherit,
                                dst_handle )) != INVALID_HANDLE_VALUE32)
    {
        if (dest) *dest = handle;
        ret = TRUE;
    }

done:
    if (dst_pdb) K32OBJ_DecCount( &dst_pdb->header );
    if (obj) K32OBJ_DecCount( obj );
    if (src_pdb) K32OBJ_DecCount( &src_pdb->header );
    SYSTEM_UNLOCK();
    return ret;
}


/***********************************************************************
 *           ConvertToGlobalHandle    		(KERNEL32)
 */
HANDLE32 WINAPI ConvertToGlobalHandle(HANDLE32 hSrc)
{
    int src_handle, dst_handle;
    HANDLE32 handle;
    K32OBJ *obj = NULL;
    DWORD access;

    if (HANDLE_IS_GLOBAL(hSrc))
        return hSrc;

    if (!(obj = HANDLE_GetObjPtr( PROCESS_Current(), hSrc, K32OBJ_UNKNOWN, 0, &src_handle )))
        return 0;

    HANDLE_GetAccess( PROCESS_Current(), hSrc, &access );

    if (src_handle != -1)
        dst_handle = CLIENT_DuplicateHandle( GetCurrentProcess(), src_handle, -1, -1, 0, FALSE,
                                             DUP_HANDLE_MAKE_GLOBAL | DUP_HANDLE_SAME_ACCESS );
    else
        dst_handle = -1;

    if ((handle = HANDLE_Alloc( PROCESS_Initial(), obj, access, FALSE,
                                dst_handle )) != INVALID_HANDLE_VALUE32)
        handle = HANDLE_LOCAL_TO_GLOBAL(handle);
    else
        handle = 0;

    CloseHandle( hSrc );
    return handle;
}
