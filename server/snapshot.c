/*
 * Server-side snapshots
 *
 * Copyright (C) 1999 Alexandre Julliard
 *
 * FIXME: heap snapshots not implemented
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "winnt.h"
#include "tlhelp32.h"

#include "handle.h"
#include "process.h"
#include "thread.h"
#include "request.h"


struct snapshot
{
    struct object             obj;           /* object header */
    struct process           *process;       /* process of this snapshot (for modules and heaps) */
    struct process_snapshot  *processes;     /* processes snapshot */
    int                       process_count; /* count of processes */
    int                       process_pos;   /* current position in proc snapshot */
    struct thread_snapshot   *threads;       /* threads snapshot */
    int                       thread_count;  /* count of threads */
    int                       thread_pos;    /* current position in thread snapshot */
    struct module_snapshot   *modules;       /* modules snapshot */
    int                       module_count;  /* count of modules */
    int                       module_pos;    /* current position in module snapshot */
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
static struct snapshot *create_snapshot( void *pid, int flags )
{
    struct process *process = NULL;
    struct snapshot *snapshot;

    /* need a process for modules and heaps */
    if (flags & (TH32CS_SNAPMODULE|TH32CS_SNAPHEAPLIST))
    {
        if (!pid) process = (struct process *)grab_object( current->process );
        else if (!(process = get_process_from_id( pid ))) return NULL;
    }

    if (!(snapshot = alloc_object( &snapshot_ops, -1 )))
    {
        if (process) release_object( process );
        return NULL;
    }

    snapshot->process = process;

    snapshot->process_pos = 0;
    snapshot->process_count = 0;
    if (flags & TH32CS_SNAPPROCESS)
        snapshot->processes = process_snap( &snapshot->process_count );

    snapshot->thread_pos = 0;
    snapshot->thread_count = 0;
    if (flags & TH32CS_SNAPTHREAD)
        snapshot->threads = thread_snap( &snapshot->thread_count );

    snapshot->module_pos = 0;
    snapshot->module_count = 0;
    if (flags & TH32CS_SNAPMODULE)
        snapshot->modules = module_snap( process, &snapshot->module_count );

    return snapshot;
}

/* get the next process in the snapshot */
static int snapshot_next_process( struct snapshot *snapshot, struct next_process_request *req )
{
    struct process_snapshot *ptr;

    if (!snapshot->process_count)
    {
        set_error( STATUS_INVALID_PARAMETER );  /* FIXME */
        return 0;
    }
    if (req->reset) snapshot->process_pos = 0;
    else if (snapshot->process_pos >= snapshot->process_count)
    {
        set_error( STATUS_NO_MORE_FILES );
        return 0;
    }
    ptr = &snapshot->processes[snapshot->process_pos++];
    req->count    = ptr->count;
    req->pid      = get_process_id( ptr->process );
    req->threads  = ptr->threads;
    req->priority = ptr->priority;
    return 1;
}

/* get the next thread in the snapshot */
static int snapshot_next_thread( struct snapshot *snapshot, struct next_thread_request *req )
{
    struct thread_snapshot *ptr;

    if (!snapshot->thread_count)
    {
        set_error( STATUS_INVALID_PARAMETER );  /* FIXME */
        return 0;
    }
    if (req->reset) snapshot->thread_pos = 0;
    else if (snapshot->thread_pos >= snapshot->thread_count)
    {
        set_error( STATUS_NO_MORE_FILES );
        return 0;
    }
    ptr = &snapshot->threads[snapshot->thread_pos++];
    req->count     = ptr->count;
    req->pid       = get_process_id( ptr->thread->process );
    req->tid       = get_thread_id( ptr->thread );
    req->base_pri  = ptr->priority;
    req->delta_pri = 0;  /* FIXME */
    return 1;
}

/* get the next module in the snapshot */
static int snapshot_next_module( struct snapshot *snapshot, struct next_module_request *req )
{
    struct module_snapshot *ptr;

    if (!snapshot->module_count)
    {
        set_error( STATUS_INVALID_PARAMETER );  /* FIXME */
        return 0;
    }
    if (req->reset) snapshot->module_pos = 0;
    else if (snapshot->module_pos >= snapshot->module_count)
    {
        set_error( STATUS_NO_MORE_FILES );
        return 0;
    }
    ptr = &snapshot->modules[snapshot->module_pos++];
    req->pid  = get_process_id( snapshot->process );
    req->base = ptr->base;
    return 1;
}

static void snapshot_dump( struct object *obj, int verbose )
{
    struct snapshot *snapshot = (struct snapshot *)obj;
    assert( obj->ops == &snapshot_ops );
    fprintf( stderr, "Snapshot: %d procs %d threads %d modules\n",
             snapshot->process_count, snapshot->thread_count, snapshot->module_count );
}

static void snapshot_destroy( struct object *obj )
{
    int i;
    struct snapshot *snapshot = (struct snapshot *)obj;
    assert( obj->ops == &snapshot_ops );
    if (snapshot->process_count)
    {
        for (i = 0; i < snapshot->process_count; i++)
            release_object( snapshot->processes[i].process );
        free( snapshot->processes );
    }
    if (snapshot->thread_count)
    {
        for (i = 0; i < snapshot->thread_count; i++)
            release_object( snapshot->threads[i].thread );
        free( snapshot->threads );
    }
    if (snapshot->module_count) free( snapshot->modules );
    if (snapshot->process) release_object( snapshot->process );
}

/* create a snapshot */
DECL_HANDLER(create_snapshot)
{
    struct snapshot *snapshot;

    req->handle = -1;
    if ((snapshot = create_snapshot( req->pid, req->flags )))
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

/* get the next thread from a snapshot */
DECL_HANDLER(next_thread)
{
    struct snapshot *snapshot;

    if ((snapshot = (struct snapshot *)get_handle_obj( current->process, req->handle,
                                                       0, &snapshot_ops )))
    {
        snapshot_next_thread( snapshot, req );
        release_object( snapshot );
    }
}

/* get the next module from a snapshot */
DECL_HANDLER(next_module)
{
    struct snapshot *snapshot;

    if ((snapshot = (struct snapshot *)get_handle_obj( current->process, req->handle,
                                                       0, &snapshot_ops )))
    {
        snapshot_next_module( snapshot, req );
        release_object( snapshot );
    }
}
