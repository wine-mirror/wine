/*
 * Server-side snapshots
 *
 * Copyright (C) 1999 Alexandre Julliard
 *
 * FIXME: only process snapshots implemented for now
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "winerror.h"
#include "winnt.h"
#include "tlhelp32.h"

#include "handle.h"
#include "process.h"
#include "thread.h"
#include "request.h"


struct snapshot
{
    struct object             obj;           /* object header */
    struct process_snapshot  *process;       /* processes snapshot */
    int                       process_count; /* count of processes */
    int                       process_pos;   /* current position in proc snapshot */
};

static void snapshot_dump( struct object *obj, int verbose );
static void snapshot_destroy( struct object *obj );

static const struct object_ops snapshot_ops =
{
    sizeof(struct snapshot),      /* size */
    snapshot_dump,                /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    NULL,                         /* get_poll_events */
    NULL,                         /* poll_event */
    no_read_fd,                   /* get_read_fd */
    no_write_fd,                  /* get_write_fd */
    no_flush,                     /* flush */
    no_get_file_info,             /* get_file_info */
    snapshot_destroy              /* destroy */
};


/* create a new snapshot */
static struct snapshot *create_snapshot( int flags )
{
    struct snapshot *snapshot;

    if ((snapshot = alloc_object( &snapshot_ops, -1 )))
    {
        if (flags & TH32CS_SNAPPROCESS)
            snapshot->process = process_snap( &snapshot->process_count );
        else
            snapshot->process_count = 0;
        snapshot->process_pos = 0;
    }
    return snapshot;
}

/* get the next process in the snapshot */
static int snapshot_next_process( struct snapshot *snapshot, struct next_process_request *req )
{
    struct process_snapshot *ptr;

    if (!snapshot->process_count)
    {
        set_error( ERROR_INVALID_PARAMETER );  /* FIXME */
        return 0;
    }
    if (req->reset) snapshot->process_pos = 0;
    else if (snapshot->process_pos >= snapshot->process_count)
    {
        set_error( ERROR_NO_MORE_FILES );
        return 0;
    }
    ptr = &snapshot->process[snapshot->process_pos++];
    req->pid      = ptr->process;
    req->threads  = ptr->threads;
    req->priority = ptr->priority;
    return 1;
}

static void snapshot_dump( struct object *obj, int verbose )
{
    struct snapshot *snapshot = (struct snapshot *)obj;
    assert( obj->ops == &snapshot_ops );
    fprintf( stderr, "Snapshot: %d processes\n",
             snapshot->process_count );
}

static void snapshot_destroy( struct object *obj )
{
    int i;
    struct snapshot *snapshot = (struct snapshot *)obj;
    assert( obj->ops == &snapshot_ops );
    if (snapshot->process_count)
    {
        for (i = 0; i < snapshot->process_count; i++)
            release_object( snapshot->process[i].process );
        free( snapshot->process );
    }
}

/* create a snapshot */
DECL_HANDLER(create_snapshot)
{
    struct snapshot *snapshot;

    req->handle = -1;
    if ((snapshot = create_snapshot( req->flags )))
    {
        req->handle = alloc_handle( current->process, snapshot, 0, req->inherit );
        release_object( snapshot );
    }
}

/* get the next process from a snapshot */
DECL_HANDLER(next_process)
{
    struct snapshot *snapshot;

    if ((snapshot = (struct snapshot *)get_handle_obj( current->process, req->handle,
                                                       0, &snapshot_ops )))
    {
        snapshot_next_process( snapshot, req );
        release_object( snapshot );
    }
}
