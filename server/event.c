/*
 * Server-side event management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "winerror.h"
#include "winnt.h"

#include "handle.h"
#include "thread.h"

struct event
{
    struct object  obj;             /* object header */
    int            manual_reset;    /* is it a manual reset event? */
    int            signaled;        /* event has been signaled */
};

static void event_dump( struct object *obj, int verbose );
static int event_signaled( struct object *obj, struct thread *thread );
static int event_satisfied( struct object *obj, struct thread *thread );
static void event_destroy( struct object *obj );

static const struct object_ops event_ops =
{
    event_dump,
    add_queue,
    remove_queue,
    event_signaled,
    event_satisfied,
    no_read_fd,
    no_write_fd,
    no_flush,
    no_get_file_info,
    event_destroy
};


static struct object *create_event( const char *name, int manual_reset, int initial_state )
{
    struct event *event;

    if (!(event = (struct event *)create_named_object( name, &event_ops, sizeof(*event) )))
        return NULL;
    if (GET_ERROR() != ERROR_ALREADY_EXISTS)
    {
        /* initialize it if it didn't already exist */
        event->manual_reset = manual_reset;
        event->signaled     = initial_state;
    }
    return &event->obj;
}

static int pulse_event( int handle )
{
    struct event *event;

    if (!(event = (struct event *)get_handle_obj( current->process, handle,
                                                  EVENT_MODIFY_STATE, &event_ops )))
        return 0;
    event->signaled = 1;
    /* wake up all waiters if manual reset, a single one otherwise */
    wake_up( &event->obj, !event->manual_reset );
    event->signaled = 0;
    release_object( event );
    return 1;
}

static int set_event( int handle )
{
    struct event *event;

    if (!(event = (struct event *)get_handle_obj( current->process, handle,
                                                  EVENT_MODIFY_STATE, &event_ops )))
        return 0;
    event->signaled = 1;
    /* wake up all waiters if manual reset, a single one otherwise */
    wake_up( &event->obj, !event->manual_reset );
    release_object( event );
    return 1;
}

static int reset_event( int handle )
{
    struct event *event;

    if (!(event = (struct event *)get_handle_obj( current->process, handle,
                                                  EVENT_MODIFY_STATE, &event_ops )))
        return 0;
    event->signaled = 0;
    release_object( event );
    return 1;
}

static void event_dump( struct object *obj, int verbose )
{
    struct event *event = (struct event *)obj;
    assert( obj->ops == &event_ops );
    fprintf( stderr, "Event manual=%d signaled=%d name='%s'\n",
             event->manual_reset, event->signaled,
             get_object_name( &event->obj ) );
}

static int event_signaled( struct object *obj, struct thread *thread )
{
    struct event *event = (struct event *)obj;
    assert( obj->ops == &event_ops );
    return event->signaled;
}

static int event_satisfied( struct object *obj, struct thread *thread )
{
    struct event *event = (struct event *)obj;
    assert( obj->ops == &event_ops );
    /* Reset if it's an auto-reset event */
    if (!event->manual_reset) event->signaled = 0;
    return 0;  /* Not abandoned */
}

static void event_destroy( struct object *obj )
{
    struct event *event = (struct event *)obj;
    assert( obj->ops == &event_ops );
    free( event );
}

/* create an event */
DECL_HANDLER(create_event)
{
    struct create_event_reply reply = { -1 };
    struct object *obj;
    char *name = (char *)data;
    if (!len) name = NULL;
    else CHECK_STRING( "create_event", name, len );

    obj = create_event( name, req->manual_reset, req->initial_state );
    if (obj)
    {
        reply.handle = alloc_handle( current->process, obj, EVENT_ALL_ACCESS, req->inherit );
        release_object( obj );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* open a handle to an event */
DECL_HANDLER(open_event)
{
    struct open_event_reply reply;
    char *name = (char *)data;
    if (!len) name = NULL;
    else CHECK_STRING( "open_event", name, len );

    reply.handle = open_object( name, &event_ops, req->access, req->inherit );
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* do an event operation */
DECL_HANDLER(event_op)
{
    switch(req->op)
    {
    case PULSE_EVENT:
        pulse_event( req->handle );
        break;
    case SET_EVENT:
        set_event( req->handle );
        break;
    case RESET_EVENT:
        reset_event( req->handle );
        break;
    default:
        fatal_protocol_error( "event_op: invalid operation %d\n", req->op );
    }
    send_reply( current, -1, 0 );
}
