/*
 * Server-side snapshots
 *
 * Copyright (C) 1999 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * FIXME: heap snapshots not implemented
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

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
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    no_map_access,                /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    no_lookup_name,               /* lookup_name */
    no_open_file,                 /* open_file */
    no_close_handle,              /* close_handle */
    snapshot_destroy              /* destroy */
};


/* create a new snapshot */
static struct snapshot *create_snapshot( process_id_t pid, int flags )
{
    struct process *process = NULL;
    struct snapshot *snapshot;

    /* need a process for modules and heaps */
    if (flags & (SNAP_MODULE|SNAP_HEAPLIST))
    {
        if (!pid) process = (struct process *)grab_object( current->process );
        else if (!(process = get_process_from_id( pid ))) return NULL;
    }

    if (!(snapshot = alloc_object( &snapshot_ops )))
    {
        if (process) release_object( process );
        return NULL;
    }

    snapshot->process = process;

    snapshot->process_pos = 0;
    snapshot->process_count = 0;
    if (flags & SNAP_PROCESS)
        snapshot->processes = process_snap( &snapshot->process_count );

    snapshot->thread_pos = 0;
    snapshot->thread_count = 0;
    if (flags & SNAP_THREAD)
        snapshot->threads = thread_snap( &snapshot->thread_count );

    snapshot->module_pos = 0;
    snapshot->module_count = 0;
    if (flags & SNAP_MODULE)
        snapshot->modules = module_snap( process, &snapshot->module_count );

    return snapshot;
}

/* get the next process in the snapshot */
static int snapshot_next_process( struct snapshot *snapshot, struct next_process_reply *reply )
{
    struct process_snapshot *ptr;
    struct process_dll *exe_module;

    if (!snapshot->process_count)
    {
        set_error( STATUS_INVALID_PARAMETER );  /* FIXME */
        return 0;
    }
    if (snapshot->process_pos >= snapshot->process_count)
    {
        set_error( STATUS_NO_MORE_FILES );
        return 0;
    }
    ptr = &snapshot->processes[snapshot->process_pos++];
    reply->count    = ptr->count;
    reply->pid      = get_process_id( ptr->process );
    reply->ppid     = ptr->process->parent ? get_process_id( ptr->process->parent ) : 0;
    reply->heap     = NULL;  /* FIXME */
    reply->module   = NULL;  /* FIXME */
    reply->threads  = ptr->threads;
    reply->priority = ptr->priority;
    reply->handles  = ptr->handles;
    if ((exe_module = get_process_exe_module( ptr->process )) && exe_module->filename)
    {
        data_size_t len = min( exe_module->namelen, get_reply_max_size() );
        set_reply_data( exe_module->filename, len );
    }
    return 1;
}

/* get the next thread in the snapshot */
static int snapshot_next_thread( struct snapshot *snapshot, struct next_thread_reply *reply )
{
    struct thread_snapshot *ptr;

    if (!snapshot->thread_count)
    {
        set_error( STATUS_INVALID_PARAMETER );  /* FIXME */
        return 0;
    }
    if (snapshot->thread_pos >= snapshot->thread_count)
    {
        set_error( STATUS_NO_MORE_FILES );
        return 0;
    }
    ptr = &snapshot->threads[snapshot->thread_pos++];
    reply->count     = ptr->count;
    reply->pid       = get_process_id( ptr->thread->process );
    reply->tid       = get_thread_id( ptr->thread );
    reply->base_pri  = ptr->priority;
    reply->delta_pri = 0;  /* FIXME */
    return 1;
}

/* get the next module in the snapshot */
static int snapshot_next_module( struct snapshot *snapshot, struct next_module_reply *reply )
{
    struct module_snapshot *ptr;

    if (!snapshot->module_count)
    {
        set_error( STATUS_INVALID_PARAMETER );  /* FIXME */
        return 0;
    }
    if (snapshot->module_pos >= snapshot->module_count)
    {
        set_error( STATUS_NO_MORE_FILES );
        return 0;
    }
    ptr = &snapshot->modules[snapshot->module_pos++];
    reply->pid  = get_process_id( snapshot->process );
    reply->base = ptr->base;
    reply->size = ptr->size;
    if (ptr->filename)
    {
        data_size_t len = min( ptr->namelen, get_reply_max_size() );
        set_reply_data( ptr->filename, len );
    }
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
    if (snapshot->module_count)
    {
        for (i = 0; i < snapshot->module_count; i++)
            free( snapshot->modules[i].filename );
        free( snapshot->modules );
    }
    if (snapshot->process) release_object( snapshot->process );
}

/* create a snapshot */
DECL_HANDLER(create_snapshot)
{
    struct snapshot *snapshot;

    reply->handle = 0;
    if ((snapshot = create_snapshot( req->pid, req->flags )))
    {
        reply->handle = alloc_handle( current->process, snapshot, 0, req->attributes );
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
        if (req->reset) snapshot->process_pos = 0;
        snapshot_next_process( snapshot, reply );
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
        if (req->reset) snapshot->thread_pos = 0;
        snapshot_next_thread( snapshot, reply );
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
        if (req->reset) snapshot->module_pos = 0;
        snapshot_next_module( snapshot, reply );
        release_object( snapshot );
    }
}
