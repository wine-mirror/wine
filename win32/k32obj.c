/*
 * KERNEL32 objects
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <assert.h>
#include "handle32.h"
#include "heap.h"
#include "file.h"
#include "process.h"
#include "thread.h"

typedef void (*destroy_object)(K32OBJ *);

static const destroy_object K32OBJ_Destroy[K32OBJ_NBOBJECTS] =
{
    NULL,
    NULL,              /* K32OBJ_SEMAPHORE */
    NULL,              /* K32OBJ_EVENT */
    NULL,              /* K32OBJ_MUTEX */
    NULL,              /* K32OBJ_CRITICAL_SECTION */
    PROCESS_Destroy,   /* K32OBJ_PROCESS */
    THREAD_Destroy,    /* K32OBJ_THREAD */
    FILE_Destroy,      /* K32OBJ_FILE */
    NULL,              /* K32OBJ_CHANGE */
    NULL,              /* K32OBJ_CONSOLE */
    NULL,              /* K32OBJ_SCREEN_BUFFER */
    NULL,              /* K32OBJ_MEM_MAPPED_FILE */
    NULL,              /* K32OBJ_SERIAL */
    NULL,              /* K32OBJ_DEVICE_IOCTL */
    NULL,              /* K32OBJ_PIPE */
    NULL,              /* K32OBJ_MAILSLOT */
    NULL,              /* K32OBJ_TOOLHELP_SNAPSHOT */
    NULL               /* K32OBJ_SOCKET */
};


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
    /* FIXME: not atomic */
    assert( ptr->type && ((unsigned)ptr->type < K32OBJ_NBOBJECTS) );
    assert( ptr->refcount );
    if (--ptr->refcount) return;
    /* Free the object */
    if (K32OBJ_Destroy[ptr->type]) K32OBJ_Destroy[ptr->type]( ptr );
}

