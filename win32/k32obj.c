/*
 * KERNEL32 objects
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <assert.h>
#include "winerror.h"
#include "handle32.h"
#include "heap.h"
#include "file.h"
#include "process.h"
#include "thread.h"

typedef void (*destroy_object)(K32OBJ *);

extern void VIRTUAL_DestroyMapping( K32OBJ *obj );

static const destroy_object K32OBJ_Destroy[K32OBJ_NBOBJECTS] =
{
    NULL,
    NULL,                     /* K32OBJ_SEMAPHORE */
    NULL,                     /* K32OBJ_EVENT */
    NULL,                     /* K32OBJ_MUTEX */
    NULL,                     /* K32OBJ_CRITICAL_SECTION */
    PROCESS_Destroy,          /* K32OBJ_PROCESS */
    THREAD_Destroy,           /* K32OBJ_THREAD */
    FILE_Destroy,             /* K32OBJ_FILE */
    NULL,                     /* K32OBJ_CHANGE */
    NULL,                     /* K32OBJ_CONSOLE */
    NULL,                     /* K32OBJ_SCREEN_BUFFER */
    VIRTUAL_DestroyMapping,   /* K32OBJ_MEM_MAPPED_FILE */
    NULL,                     /* K32OBJ_SERIAL */
    NULL,                     /* K32OBJ_DEVICE_IOCTL */
    NULL,                     /* K32OBJ_PIPE */
    NULL,                     /* K32OBJ_MAILSLOT */
    NULL,                     /* K32OBJ_TOOLHELP_SNAPSHOT */
    NULL                      /* K32OBJ_SOCKET */
};

typedef struct _NE
{
    struct _NE *next;
    K32OBJ     *obj;
    UINT32      len;
    char        name[1];
} NAME_ENTRY;

static NAME_ENTRY *K32OBJ_FirstEntry = NULL;


/***********************************************************************
 *           K32OBJ_IncCount
 */
void K32OBJ_IncCount( K32OBJ *ptr )
{
    /* FIXME: not atomic */
    assert( ptr->type && ((unsigned)ptr->type < K32OBJ_NBOBJECTS) );
    ptr->refcount++;
}


/***********************************************************************
 *           K32OBJ_DecCount
 */
void K32OBJ_DecCount( K32OBJ *ptr )
{
    NAME_ENTRY **pptr;

    /* FIXME: not atomic */
    assert( ptr->type && ((unsigned)ptr->type < K32OBJ_NBOBJECTS) );
    assert( ptr->refcount );
    if (--ptr->refcount) return;

    /* Check if the object has a name entry and free it */

    pptr = &K32OBJ_FirstEntry;
    while (*pptr && ((*pptr)->obj != ptr)) pptr = &(*pptr)->next;
    if (*pptr)
    {
        NAME_ENTRY *entry = *pptr;
        *pptr = entry->next;
        HeapFree( SystemHeap, 0, entry );
    }

    /* Free the object */

    if (K32OBJ_Destroy[ptr->type]) K32OBJ_Destroy[ptr->type]( ptr );
}


/***********************************************************************
 *           K32OBJ_AddName
 *
 * Add a name entry for an object. We don't check for duplicates here.
 * FIXME: should use some sort of hashing.
 */
BOOL32 K32OBJ_AddName( K32OBJ *obj, LPCSTR name )
{
    NAME_ENTRY *entry;
    UINT32 len = strlen( name );

    if (!(entry = HeapAlloc( SystemHeap, 0, sizeof(NAME_ENTRY) + len )))
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return FALSE;
    }
    entry->next = K32OBJ_FirstEntry;
    entry->obj = obj;
    lstrcpy32A( entry->name, name );
    K32OBJ_FirstEntry = entry;
    return TRUE;
}


/***********************************************************************
 *           K32OBJ_FindName
 *
 * Find the object referenced by a given name.
 * The reference count is not incremented.
 */
K32OBJ *K32OBJ_FindName( LPCSTR name )
{
    NAME_ENTRY *entry = K32OBJ_FirstEntry;
    UINT32 len;

    if (!name) return NULL;  /* Anonymous object */
    len = strlen( name );
    while (entry)
    {
        if ((len == entry->len) && !lstrcmp32A( name, entry->name))
            return entry->obj;
        entry = entry->next;
    }
    return NULL;
}


/***********************************************************************
 *           K32OBJ_FindNameType
 *
 * Find an object by name and check its type.
 * The reference count is not incremented.
 */
K32OBJ *K32OBJ_FindNameType( LPCSTR name, K32OBJ_TYPE type )
{
    K32OBJ *obj = K32OBJ_FindName( name );
    if (!obj) return NULL;
    if (obj->type == type) return obj;
    SetLastError( ERROR_DUP_NAME );
    return NULL;
}
