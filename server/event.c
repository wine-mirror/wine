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
#include "server/thread.h"

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
    event_destroy
};


struct object *create_event( const char *name, int manual_reset, int initial_state )
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

int open_event( unsigned int access, int inherit, const char *name )
{
    return open_object( name, &event_ops, access, inherit );
}

int pulse_event( int handle )
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

int set_event( int handle )
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

int reset_event( int handle )
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
    printf( "Event manual=%d signaled=%d\n", event->manual_reset, event->signaled );
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
