/*
 * Server-side message queues
 *
 * Copyright (C) 2000 Alexandre Julliard
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "handle.h"
#include "thread.h"
#include "process.h"
#include "request.h"

struct msg_queue
{
    struct object  obj;             /* object header */
    struct thread *thread;          /* thread owning this queue */
    int            signaled;        /* queue has been signaled */
};

static void msg_queue_dump( struct object *obj, int verbose );
static int msg_queue_add_queue( struct object *obj, struct wait_queue_entry *entry );
static void msg_queue_remove_queue( struct object *obj, struct wait_queue_entry *entry );
static int msg_queue_signaled( struct object *obj, struct thread *thread );
static int msg_queue_satisfied( struct object *obj, struct thread *thread );

static const struct object_ops msg_queue_ops =
{
    sizeof(struct msg_queue),  /* size */
    msg_queue_dump,            /* dump */
    msg_queue_add_queue,       /* add_queue */
    msg_queue_remove_queue,    /* remove_queue */
    msg_queue_signaled,        /* signaled */
    msg_queue_satisfied,       /* satisfied */
    NULL,                      /* get_poll_events */
    NULL,                      /* poll_event */
    no_read_fd,                /* get_read_fd */
    no_write_fd,               /* get_write_fd */
    no_flush,                  /* flush */
    no_get_file_info,          /* get_file_info */
    no_destroy                 /* destroy */
};


static struct msg_queue *create_msg_queue( struct thread *thread )
{
    struct msg_queue *queue;

    if ((queue = alloc_object( &msg_queue_ops, -1 )))
    {
        queue->signaled = 0;
        queue->thread = thread;
        thread->queue = queue;
        if (!thread->process->queue)
            thread->process->queue = (struct msg_queue *)grab_object( queue );
    }
    return queue;
}

static int msg_queue_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    struct process *process = entry->thread->process;

    /* if waiting on the main process queue, set the idle event */
    if (entry->thread == queue->thread && process->queue == queue)
    {
        if (process->idle_event) set_event( process->idle_event );
    }
    add_queue( obj, entry );
    return 1;
}

static void msg_queue_remove_queue(struct object *obj, struct wait_queue_entry *entry )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    struct process *process = entry->thread->process;

    remove_queue( obj, entry );

    /* if waiting on the main process queue, reset the idle event */
    if (entry->thread == queue->thread && process->queue == queue)
    {
        if (process->idle_event) reset_event( process->idle_event );
    }
}

static void msg_queue_dump( struct object *obj, int verbose )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    fprintf( stderr, "Msg queue signaled=%d owner=%p\n", queue->signaled, queue->thread );
}

static int msg_queue_signaled( struct object *obj, struct thread *thread )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    return queue->signaled;
}

static int msg_queue_satisfied( struct object *obj, struct thread *thread )
{
    struct msg_queue *queue = (struct msg_queue *)obj;
    queue->signaled = 0;
    return 0;  /* Not abandoned */
}

/* get the message queue of the current thread */
DECL_HANDLER(get_msg_queue)
{
    struct msg_queue *queue = current->queue;

    req->handle = -1;
    if (!queue) queue = create_msg_queue( current );
    if (queue) req->handle = alloc_handle( current->process, queue, SYNCHRONIZE, 0 );
}

/* wake up a message queue */
DECL_HANDLER(wake_queue)
{
    struct msg_queue *queue = (struct msg_queue *)get_handle_obj( current->process, req->handle,
                                                                  0, &msg_queue_ops );
    if (queue)
    {
        queue->signaled = 1;
        wake_up( &queue->obj, 0 );
        release_object( queue );
    }
}
