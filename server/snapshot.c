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
    snapshot_dump,
    no_add_queue,
    NULL,  /* should never get called */
    NULL,  /* should never get called */
    NULL,  /* should never get called */
    no_read_fd,
    no_write_fd,
    no_flush,
    no_get_file_info,
    snapshot_destroy
};


/* create a new snapshot */
static struct object *create_snapshot( int flags )
{
    struct snapshot *snapshot;
    if (!(snapshot = mem_alloc( sizeof(*snapshot) ))) return NULL;
    init_object( &snapshot->obj, &snapshot_ops, NULL );
    if (flags & TH32CS_SNAPPROCESS)
        snapshot->process = process_snap( &snapshot->process_count );
    else
        snapshot->process_count = 0;

    snapshot->process_pos = 0;
    return &snapshot->obj;
}

/* get the next process in the snapshot */
static int snapshot_next_process( int handle, int reset, struct next_process_reply *reply )
{
    struct snapshot *snapshot;
    struct process_snapshot *ptr;
    if (!(snapshot = (struct snapshot *)get_handle_obj( current->process, handle,
                                                        0, &snapshot_ops )))
        return 0;
    if (!snapshot->process_count)
    {
        SET_ERROR( ERROR_INVALID_PARAMETER );  /* FIXME */
        release_object( snapshot );
        return 0;
    }
    if (reset) snapshot->process_pos = 0;
    else if (snapshot->process_pos >= snapshot->process_count)
    {
        SET_ERROR( ERROR_NO_MORE_FILES );
        release_object( snapshot );
        return 0;
    }
    ptr = &snapshot->process[snapshot->process_pos++];
    reply->pid      = ptr->process;
    reply->threads  = ptr->threads;
    reply->priority = ptr->priority;
    release_object( snapshot );
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
    free( snapshot );
}

/* create a snapshot */
DECL_HANDLER(create_snapshot)
{
    struct object *obj;
    struct create_snapshot_reply reply = { -1 };

    if ((obj = create_snapshot( req->flags )))
    {
        reply.handle = alloc_handle( current->process, obj, 0, req->inherit );
        release_object( obj );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* get the next process from a snapshot */
DECL_HANDLER(next_process)
{
    struct next_process_reply reply;
    snapshot_next_process( req->handle, req->reset, &reply );
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}
