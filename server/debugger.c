/*
 * Server-side debugger functions
 *
 * Copyright (C) 1999 Alexandre Julliard
 */

#include <assert.h>
#include "winbase.h"
#include "winerror.h"

#include "server.h"
#include "handle.h"
#include "process.h"
#include "thread.h"


struct debug_event
{
    struct debug_event    *next;    /* event queue */
    struct debug_event    *prev;
    struct thread         *thread;  /* thread which sent this event */
    int                    sent;    /* already sent to the debugger? */
    int                    code;    /* event code */
    union debug_event_data data;    /* event data */
};

struct debug_ctx
{
    struct thread       *owner;       /* thread owning this debug context */   
    int                  waiting;     /* is thread waiting for an event? */
    struct timeout_user *timeout;     /* timeout user for wait timeout */
    struct debug_event  *event_head;  /* head of pending events queue */
    struct debug_event  *event_tail;  /* tail of pending events queue */
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
                  THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE )) == -1)
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
                                              PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE )) == -1)
        {
            if (handle != -1) close_handle( debugger->process, handle );
            return 0;
        }
        if ((event->data.create_process.thread = alloc_handle( debugger->process, thread,
                  THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE )) == -1)
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

/* free a debug event structure */
static void free_event( struct debug_event *event )
{
    switch(event->code)
    {
    case CREATE_THREAD_DEBUG_EVENT:
        close_handle( event->thread->process, event->data.create_thread.handle );
        break;
    case CREATE_PROCESS_DEBUG_EVENT:
        if (event->data.create_process.file != -1)
            close_handle( event->thread->process, event->data.create_process.file );
        close_handle( event->thread->process, event->data.create_process.thread );
        close_handle( event->thread->process, event->data.create_process.process );
        break;
    case LOAD_DLL_DEBUG_EVENT:
        if (event->data.load_dll.handle != -1)
            close_handle( event->thread->process, event->data.load_dll.handle );
        break;
    }
    event->thread->debug_event = NULL;
    release_object( event->thread );
    free( event );
}

/* unlink the first event from the queue */
static void unlink_event( struct debug_ctx *debug_ctx, struct debug_event *event )
{
    if (event->prev) event->prev->next = event->next;
    else debug_ctx->event_head = event->next;
    if (event->next) event->next->prev = event->prev;
    else debug_ctx->event_tail = event->prev;
    event->next = event->prev = NULL;
}

/* link an event at the end of the queue */
static void link_event( struct debug_ctx *debug_ctx, struct debug_event *event )
{
    event->next = NULL;
    event->prev = debug_ctx->event_tail;
    if (event->prev) event->prev->next = event;
    else debug_ctx->event_head = event;
    debug_ctx->event_tail = event;
}

/* send the first queue event as a reply */
static void send_event_reply( struct debug_ctx *debug_ctx )
{
    struct wait_debug_event_reply reply;
    struct debug_event *event = debug_ctx->event_head;
    struct thread *thread = event->thread;

    assert( event );
    assert( debug_ctx->waiting );

    unlink_event( debug_ctx, event );
    event->sent = 1;
    reply.code = event->code;
    reply.pid  = thread->process;
    reply.tid  = thread;
    debug_ctx->waiting = 0;
    if (debug_ctx->timeout)
    {
        remove_timeout_user( debug_ctx->timeout );
        debug_ctx->timeout = NULL;
    }
    debug_ctx->owner->error = 0;
    send_reply( debug_ctx->owner, -1, 2, &reply, sizeof(reply),
                &event->data, event_sizes[event->code] );
}

/* timeout callback while waiting for a debug event */
static void wait_event_timeout( void *ctx )
{
    struct debug_ctx *debug_ctx = (struct debug_ctx *)ctx;
    struct wait_debug_event_reply reply;

    assert( debug_ctx->waiting );

    reply.code = 0;
    reply.pid  = 0;
    reply.tid  = 0;
    debug_ctx->waiting = 0;
    debug_ctx->timeout = NULL;
    debug_ctx->owner->error = WAIT_TIMEOUT;
    send_reply( debug_ctx->owner, -1, 1, &reply, sizeof(reply) );    
}

/* wait for a debug event (or send a reply at once if one is pending) */
static int wait_for_debug_event( int timeout )
{
    struct debug_ctx *debug_ctx = current->debug_ctx;
    struct timeval when;

    if (!debug_ctx)  /* current thread is not a debugger */
    {
        SET_ERROR( ERROR_ACCESS_DENIED ); /* FIXME */
        return 0;
    }
    assert( !debug_ctx->waiting );
    if (debug_ctx->event_head)  /* already have a pending event */
    {
        debug_ctx->waiting = 1;
        send_event_reply( debug_ctx );
        return 1;
    }
    if (!timeout)  /* no event and we don't want to wait */
    {
        SET_ERROR( WAIT_TIMEOUT );
        return 0;
    }
    if (timeout != -1)  /* start the timeout */
    {
        make_timeout( &when, timeout );
        if (!(debug_ctx->timeout = add_timeout_user( &when, wait_event_timeout, debug_ctx )))
            return 0;
    }
    debug_ctx->waiting = 1;
    return 1;
}

/* continue a debug event */
static int continue_debug_event( struct process *process, struct thread *thread, int status )
{
    struct debug_event *event = thread->debug_event;

    if (process->debugger != current || !event || !event->sent)
    {
        /* not debugging this process, or no event pending */
        SET_ERROR( ERROR_ACCESS_DENIED );  /* FIXME */
        return 0;
    }
    if (thread->state != TERMINATED)
    {
        /* only send a reply if the thread is still there */
        /* (we can get a continue on an exit thread/process event) */
        struct send_debug_event_reply reply;
        reply.status = status;
        send_reply( thread, -1, 1, &reply, sizeof(reply) );
    }
    free_event( event );
    resume_process( process );
    return 1;
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
    if (!(event = mem_alloc( sizeof(*event) - sizeof(event->data) + event_sizes[code] )))
        return NULL;
    event->sent   = 0;
    event->code   = code;
    event->thread = (struct thread *)grab_object( thread );
    memcpy( &event->data, data, event_sizes[code] );

    if (!fill_debug_event( debugger, thread, event ))
    {
        release_object( event->thread );
        free( event );
        return NULL;
    }

    if (thread->debug_event)
    {
        /* only exit events can replace others */
        assert( code == EXIT_THREAD_DEBUG_EVENT || code == EXIT_PROCESS_DEBUG_EVENT );
        if (!thread->debug_event->sent) unlink_event( debug_ctx, thread->debug_event );
        free_event( thread->debug_event );
    }

    link_event( debug_ctx, event );
    thread->debug_event = event;
    suspend_process( thread->process );
    if (debug_ctx->waiting) send_event_reply( debug_ctx );
    return event;
}

/* attach a process to a debugger thread */
int debugger_attach( struct process *process, struct thread *debugger )
{
    struct debug_ctx *debug_ctx;
    struct thread *thread;

    if (process->debugger)  /* already being debugged */
    {
        SET_ERROR( ERROR_ACCESS_DENIED );
        return 0;
    }
    /* make sure we don't create a debugging loop */
    for (thread = debugger; thread; thread = thread->process->debugger)
        if (thread->process == process)
        {
            SET_ERROR( ERROR_ACCESS_DENIED );
            return 0;
        }

    if (!debugger->debug_ctx)  /* need to allocate a context */
    {
        assert( !debugger->debug_first );
        if (!(debug_ctx = mem_alloc( sizeof(*debug_ctx) ))) return 0;
        debug_ctx->owner      = current;
        debug_ctx->waiting    = 0;
        debug_ctx->timeout    = NULL;
        debug_ctx->event_head = NULL;
        debug_ctx->event_tail = NULL;
        debugger->debug_ctx = debug_ctx;
    }
    process->debugger   = debugger;
    process->debug_prev = NULL;
    process->debug_next = debugger->debug_first;
    debugger->debug_first = process;
    return 1;
}

/* detach a process from its debugger thread */
static void debugger_detach( struct process *process )
{
    struct thread *debugger = process->debugger;

    assert( debugger );

    if (process->debug_next) process->debug_next->debug_prev = process->debug_prev;
    if (process->debug_prev) process->debug_prev->debug_next = process->debug_next;
    else debugger->debug_first = process;
    process->debugger = NULL;
}

/* a thread is exiting */
void debug_exit_thread( struct thread *thread, int exit_code )
{
    struct thread *debugger = current->process->debugger;
    struct debug_ctx *debug_ctx = thread->debug_ctx;

    if (debugger)  /* being debugged -> send an event to the debugger */
    {
        struct debug_event_exit event;
        event.exit_code = exit_code;
        if (!thread->proc_next && !thread->proc_prev)
        {
            assert( thread->process->thread_list == thread );
            /* this is the last thread, send an exit process event and cleanup */
            queue_debug_event( debugger, current, EXIT_PROCESS_DEBUG_EVENT, &event );
            debugger_detach( thread->process );
        }
        else queue_debug_event( debugger, current, EXIT_THREAD_DEBUG_EVENT, &event );
    }

    if (debug_ctx)  /* this thread is a debugger */
    {
        struct debug_event *event;

        /* kill all debugged processes */
        while (thread->debug_first) kill_process( thread->debug_first, exit_code );
        /* free all pending events */
        while ((event = debug_ctx->event_head) != NULL)
        {
            unlink_event( debug_ctx, event );
            free_event( event );
        }
        /* remove the timeout */
        if (debug_ctx->timeout) remove_timeout_user( debug_ctx->timeout );
        thread->debug_ctx = NULL;
        free( debug_ctx );
    }
}

/* Wait for a debug event */
DECL_HANDLER(wait_debug_event)
{
    struct wait_debug_event_reply reply;

    if (!wait_for_debug_event( req->timeout ))
    {
        reply.code = 0;
        reply.pid  = NULL;
        reply.tid  = NULL;
        send_reply( current, -1, 1, &reply, sizeof(reply) );
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
    send_reply( current, -1, 0 );
}

/* Start debugging an existing process */
DECL_HANDLER(debug_process)
{
    struct process *process = get_process_from_id( req->pid );
    if (process)
    {
        debugger_attach( process, current );
        /* FIXME: should notice the debugged process somehow */
        release_object( process );
    }
    send_reply( current, -1, 0 );
}

/* Send a debug event */
DECL_HANDLER(send_debug_event)
{
    struct thread *debugger = current->process->debugger;
    struct send_debug_event_reply reply;

    if ((req->code <= 0) || (req->code > RIP_EVENT))
        fatal_protocol_error( "send_debug_event: bad event code %d\n", req->code );
    if (len != event_sizes[req->code])
        fatal_protocol_error( "send_debug_event: bad event length %d/%d\n",
                              len, event_sizes[req->code] );
    assert( !current->debug_event );
    reply.status = 0;
    if (debugger)
    {
        if (queue_debug_event( debugger, current, req->code, data ))
            return;  /* don't reply now, wait for continue_debug_event */
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}
