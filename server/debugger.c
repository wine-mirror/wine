/*
 * Server-side debugger functions
 *
 * Copyright (C) 1999 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "winbase.h"
#include "winerror.h"

#include "handle.h"
#include "process.h"
#include "thread.h"
#include "request.h"

enum debug_event_state { EVENT_QUEUED, EVENT_SENT, EVENT_CONTINUED };

/* debug event */
struct debug_event
{
    struct object          obj;       /* object header */
    struct debug_event    *next;      /* event queue */
    struct debug_event    *prev;
    struct thread         *sender;    /* thread which sent this event */
    struct thread         *debugger;  /* debugger thread receiving the event */
    enum debug_event_state state;     /* event state */
    int                    status;    /* continuation status */
    int                    code;      /* event code */
    union debug_event_data data;      /* event data */
};

/* debug context */
struct debug_ctx
{
    struct object        obj;         /* object header */
    struct debug_event  *event_head;  /* head of pending events queue */
    struct debug_event  *event_tail;  /* tail of pending events queue */
    struct debug_event  *to_send;     /* next event on the queue to send to debugger */
};


static void debug_event_dump( struct object *obj, int verbose );
static int debug_event_signaled( struct object *obj, struct thread *thread );
static void debug_event_destroy( struct object *obj );

static const struct object_ops debug_event_ops =
{
    sizeof(struct debug_event),    /* size */
    debug_event_dump,              /* dump */
    add_queue,                     /* add_queue */
    remove_queue,                  /* remove_queue */
    debug_event_signaled,          /* signaled */
    no_satisfied,                  /* satisfied */
    NULL,                          /* get_poll_events */
    NULL,                          /* poll_event */
    no_read_fd,                    /* get_read_fd */
    no_write_fd,                   /* get_write_fd */
    no_flush,                      /* flush */
    no_get_file_info,              /* get_file_info */
    debug_event_destroy            /* destroy */
};

static void debug_ctx_dump( struct object *obj, int verbose );
static int debug_ctx_signaled( struct object *obj, struct thread *thread );
static void debug_ctx_destroy( struct object *obj );

static const struct object_ops debug_ctx_ops =
{
    sizeof(struct debug_ctx),      /* size */
    debug_ctx_dump,                /* dump */
    add_queue,                     /* add_queue */
    remove_queue,                  /* remove_queue */
    debug_ctx_signaled,            /* signaled */
    no_satisfied,                  /* satisfied */
    NULL,                          /* get_poll_events */
    NULL,                          /* poll_event */
    no_read_fd,                    /* get_read_fd */
    no_write_fd,                   /* get_write_fd */
    no_flush,                      /* flush */
    no_get_file_info,              /* get_file_info */
    debug_ctx_destroy              /* destroy */
};

/* size of the event data */
static const int event_sizes[] =
{
    0,
    sizeof(struct debug_event_exception),       /* EXCEPTION_DEBUG_EVENT */
    sizeof(struct debug_event_create_thread),   /* CREATE_THREAD_DEBUG_EVENT */
    sizeof(struct debug_event_create_process),  /* CREATE_PROCESS_DEBUG_EVENT */
    sizeof(struct debug_event_exit),            /* EXIT_THREAD_DEBUG_EVENT */
    sizeof(struct debug_event_exit),            /* EXIT_PROCESS_DEBUG_EVENT */
    sizeof(struct debug_event_load_dll),        /* LOAD_DLL_DEBUG_EVENT */
    sizeof(struct debug_event_unload_dll),      /* UNLOAD_DLL_DEBUG_EVENT */
    sizeof(struct debug_event_output_string),   /* OUTPUT_DEBUG_STRING_EVENT */
    sizeof(struct debug_event_rip_info)         /* RIP_EVENT */
};


/* initialise the fields that do not need to be filled by the client */
static int fill_debug_event( struct thread *debugger, struct thread *thread,
                             struct debug_event *event )
{
    int handle;

    /* some events need special handling */
    switch(event->code)
    {
    case CREATE_THREAD_DEBUG_EVENT:
        if ((event->data.create_thread.handle = alloc_handle( debugger->process, thread,
               /* documented: THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME */
                                                              THREAD_ALL_ACCESS, FALSE )) == -1)
            return 0;
        break;
    case CREATE_PROCESS_DEBUG_EVENT:
        if ((handle = event->data.create_process.file) != -1)
        {
            if ((handle = duplicate_handle( thread->process, handle, debugger->process,
                                            GENERIC_READ, FALSE, 0 )) == -1)
                return 0;
            event->data.create_process.file = handle;
        }
        if ((event->data.create_process.process = alloc_handle( debugger->process, thread->process,
                                           /* documented: PROCESS_VM_READ | PROCESS_VM_WRITE */
                                                                PROCESS_ALL_ACCESS, FALSE )) == -1)
        {
            if (handle != -1) close_handle( debugger->process, handle );
            return 0;
        }
        if ((event->data.create_process.thread = alloc_handle( debugger->process, thread,
               /* documented: THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME */
                                                               THREAD_ALL_ACCESS, FALSE )) == -1)
        {
            if (handle != -1) close_handle( debugger->process, handle );
            close_handle( debugger->process, event->data.create_process.process );
            return 0;
        }
        break;
    case LOAD_DLL_DEBUG_EVENT:
        if ((handle = event->data.load_dll.handle) != -1)
        {
            if ((handle = duplicate_handle( thread->process, handle, debugger->process,
                                            GENERIC_READ, FALSE, 0 )) == -1)
                return 0;
            event->data.load_dll.handle = handle;
        }
        break;
    }
    return 1;
}

/* unlink the first event from the queue */
static void unlink_event( struct debug_ctx *debug_ctx, struct debug_event *event )
{
    if (event->prev) event->prev->next = event->next;
    else debug_ctx->event_head = event->next;
    if (event->next) event->next->prev = event->prev;
    else debug_ctx->event_tail = event->prev;
    if (debug_ctx->to_send == event) debug_ctx->to_send = event->next;
    event->next = event->prev = NULL;
    release_object( event );
}

/* link an event at the end of the queue */
static void link_event( struct debug_ctx *debug_ctx, struct debug_event *event )
{
    grab_object( event );
    event->next = NULL;
    event->prev = debug_ctx->event_tail;
    debug_ctx->event_tail = event;
    if (event->prev) event->prev->next = event;
    else debug_ctx->event_head = event;
    if (!debug_ctx->to_send)
    {
        debug_ctx->to_send = event;
        wake_up( &debug_ctx->obj, 0 );
    }
}

/* build a reply for the wait_debug_event request */
static void build_wait_debug_reply( struct thread *thread, struct object *obj, int signaled )
{
    struct wait_debug_event_request *req = get_req_ptr( thread );

    if (obj)
    {
        struct debug_ctx *debug_ctx = (struct debug_ctx *)obj; 
        struct debug_event *event = debug_ctx->to_send;

        /* the object that woke us has to be our debug context */
        assert( obj->ops == &debug_ctx_ops );
        assert( event );

        event->state = EVENT_SENT;
        debug_ctx->to_send = event->next;
        req->code = event->code;
        req->pid  = event->sender->process;
        req->tid  = event->sender;
        memcpy( req + 1, &event->data, event_sizes[event->code] );
    }
    else  /* timeout or error */
    {
        req->code = 0;
        req->pid  = 0;
        req->tid  = 0;
        thread->error = signaled;
    }
}

/* build a reply for the send_event request */
static void build_send_event_reply( struct thread *thread, struct object *obj, int signaled )
{
    struct send_debug_event_request *req = get_req_ptr( thread );
    struct debug_event *event = (struct debug_event *)obj;
    assert( obj->ops == &debug_event_ops );

    req->status = event->status;
    /* copy the context into the reply */
    if (event->code == EXCEPTION_DEBUG_EVENT)
        memcpy( req + 1, &event->data, event_sizes[event->code] );
}

static void debug_event_dump( struct object *obj, int verbose )
{
    struct debug_event *debug_event = (struct debug_event *)obj;
    assert( obj->ops == &debug_event_ops );
    fprintf( stderr, "Debug event sender=%p code=%d state=%d\n",
             debug_event->sender, debug_event->code, debug_event->state );
}

static int debug_event_signaled( struct object *obj, struct thread *thread )
{
    struct debug_event *debug_event = (struct debug_event *)obj;
    assert( obj->ops == &debug_event_ops );
    return debug_event->state == EVENT_CONTINUED;
}

static void debug_event_destroy( struct object *obj )
{
    struct debug_event *event = (struct debug_event *)obj;
    assert( obj->ops == &debug_event_ops );

    /* cannot still be in the queue */
    assert( !event->next );
    assert( !event->prev );

    /* If the event has been sent already, the handles are now under the */
    /* responsibility of the debugger process, so we don't touch them    */
    if (event->state == EVENT_QUEUED)
    {
        struct process *debugger = event->debugger->process;
        switch(event->code)
        {
        case CREATE_THREAD_DEBUG_EVENT:
            close_handle( debugger, event->data.create_thread.handle );
            break;
        case CREATE_PROCESS_DEBUG_EVENT:
            if (event->data.create_process.file != -1)
                close_handle( debugger, event->data.create_process.file );
            close_handle( debugger, event->data.create_process.thread );
            close_handle( debugger, event->data.create_process.process );
            break;
        case LOAD_DLL_DEBUG_EVENT:
            if (event->data.load_dll.handle != -1)
                close_handle( debugger, event->data.load_dll.handle );
            break;
        }
    }
    release_object( event->sender );
    release_object( event->debugger );
}

static void debug_ctx_dump( struct object *obj, int verbose )
{
    struct debug_ctx *debug_ctx = (struct debug_ctx *)obj;
    assert( obj->ops == &debug_ctx_ops );
    fprintf( stderr, "Debug context head=%p tail=%p to_send=%p\n",
             debug_ctx->event_head, debug_ctx->event_tail, debug_ctx->to_send );
}

static int debug_ctx_signaled( struct object *obj, struct thread *thread )
{
    struct debug_ctx *debug_ctx = (struct debug_ctx *)obj;
    assert( obj->ops == &debug_ctx_ops );
    return debug_ctx->to_send != NULL;
}

static void debug_ctx_destroy( struct object *obj )
{
    struct debug_event *event;
    struct debug_ctx *debug_ctx = (struct debug_ctx *)obj;
    assert( obj->ops == &debug_ctx_ops );

    /* free all pending events */
    while ((event = debug_ctx->event_head) != NULL) unlink_event( debug_ctx, event );
}

/* wait for a debug event (or send a reply at once if one is pending) */
static int wait_for_debug_event( int timeout )
{
    struct debug_ctx *debug_ctx = current->debug_ctx;
    struct object *obj = &debug_ctx->obj;
    int flags = 0;

    if (!debug_ctx)  /* current thread is not a debugger */
    {
        set_error( ERROR_INVALID_HANDLE );
        return 0;
    }
    if (timeout != -1) flags = SELECT_TIMEOUT;
    return sleep_on( 1, &obj, flags, timeout, build_wait_debug_reply );
}

/* continue a debug event */
static int continue_debug_event( struct process *process, struct thread *thread, int status )
{
    struct debug_event *event;
    struct debug_ctx *debug_ctx = current->debug_ctx;

    if (!debug_ctx || process->debugger != current || thread->process != process) goto error;

    /* find the event in the queue */
    for (event = debug_ctx->event_head; event; event = event->next)
    {
        if (event == debug_ctx->to_send) goto error;
        if (event->sender == thread) break;
        event = event->next;
    }
    if (!event) goto error;

    event->status = status;
    event->state  = EVENT_CONTINUED;
    wake_up( &event->obj, 0 );

    unlink_event( debug_ctx, event );
    resume_process( process );
    return 1;
 error:
    /* not debugging this process, or no such event */
    set_error( ERROR_ACCESS_DENIED );  /* FIXME */
    return 0;
}

/* queue a debug event for a debugger */
static struct debug_event *queue_debug_event( struct thread *debugger, struct thread *thread,
                                              int code, void *data )
{
    struct debug_ctx *debug_ctx = debugger->debug_ctx;
    struct debug_event *event;

    assert( debug_ctx );
    /* cannot queue a debug event for myself */
    assert( debugger->process != thread->process );

    /* build the event */
    if (!(event = alloc_object( &debug_event_ops, -1 ))) return NULL;
    event->next     = NULL;
    event->prev     = NULL;
    event->state    = EVENT_QUEUED;
    event->code     = code;
    event->sender   = (struct thread *)grab_object( thread );
    event->debugger = (struct thread *)grab_object( debugger );
    memcpy( &event->data, data, event_sizes[code] );

    if (!fill_debug_event( debugger, thread, event ))
    {
        event->code = -1;  /* make sure we don't attempt to close handles */
        release_object( event );
        return NULL;
    }

    link_event( debug_ctx, event );
    suspend_process( thread->process );
    return event;
}

/* attach a process to a debugger thread */
int debugger_attach( struct process *process, struct thread *debugger )
{
    struct debug_ctx *debug_ctx;
    struct thread *thread;

    if (process->debugger)  /* already being debugged */
    {
        set_error( ERROR_ACCESS_DENIED );
        return 0;
    }
    /* make sure we don't create a debugging loop */
    for (thread = debugger; thread; thread = thread->process->debugger)
        if (thread->process == process)
        {
            set_error( ERROR_ACCESS_DENIED );
            return 0;
        }

    if (!debugger->debug_ctx)  /* need to allocate a context */
    {
        if (!(debug_ctx = alloc_object( &debug_ctx_ops, -1 ))) return 0;
        debug_ctx->event_head = NULL;
        debug_ctx->event_tail = NULL;
        debug_ctx->to_send    = NULL;
        debugger->debug_ctx = debug_ctx;
    }
    process->debugger = debugger;
    return 1;
}

/* a thread is exiting */
void debug_exit_thread( struct thread *thread, int exit_code )
{
    struct thread *debugger = thread->process->debugger;
    struct debug_ctx *debug_ctx = thread->debug_ctx;

    if (debugger)  /* being debugged -> send an event to the debugger */
    {
        struct debug_event *event;
        struct debug_event_exit exit;
        exit.exit_code = exit_code;
        if (thread->process->running_threads == 1)
            /* this is the last thread, send an exit process event */
            event = queue_debug_event( debugger, thread, EXIT_PROCESS_DEBUG_EVENT, &exit );
        else
            event = queue_debug_event( debugger, thread, EXIT_THREAD_DEBUG_EVENT, &exit );
        if (event) release_object( event );
    }

    if (debug_ctx)  /* this thread is a debugger */
    {
        /* kill all debugged processes */
        kill_debugged_processes( thread, exit_code );
        thread->debug_ctx = NULL;
        release_object( debug_ctx );
    }
}

/* Wait for a debug event */
DECL_HANDLER(wait_debug_event)
{
    if (!wait_for_debug_event( req->timeout ))
    {
        req->code = 0;
        req->pid  = NULL;
        req->tid  = NULL;
    }
}

/* Continue a debug event */
DECL_HANDLER(continue_debug_event)
{
    struct process *process = get_process_from_id( req->pid );
    if (process)
    {
        struct thread *thread = get_thread_from_id( req->tid );
        if (thread)
        {
            continue_debug_event( process, thread, req->status );
            release_object( thread );
        }
        release_object( process );
    }
}

/* Start debugging an existing process */
DECL_HANDLER(debug_process)
{
    struct process *process = get_process_from_id( req->pid );
    if (process)
    {
        debugger_attach( process, current );
        /* FIXME: should notify the debugged process somehow */
        release_object( process );
    }
}

/* Send a debug event */
DECL_HANDLER(send_debug_event)
{
    struct thread *debugger = current->process->debugger;
    struct debug_event *event;

    if ((req->code <= 0) || (req->code > RIP_EVENT))
    {
        fatal_protocol_error( current, "send_debug_event: bad code %d\n", req->code );
        return;
    }
    req->status = 0;
    if (debugger && ((event = queue_debug_event( debugger, current, req->code, req + 1 ))))
    {
        /* wait for continue_debug_event */
        struct object *obj = &event->obj;
        sleep_on( 1, &obj, 0, -1, build_send_event_reply );
        release_object( event );
    }
}
