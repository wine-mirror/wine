/*
 * Waitable timers management
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
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "file.h"
#include "handle.h"
#include "request.h"

struct timer
{
    struct object        obj;       /* object header */
    int                  manual;    /* manual reset */
    int                  signaled;  /* current signaled state */
    unsigned int         period;    /* timer period in ms */
    timeout_t            when;      /* next expiration */
    struct timeout_user *timeout;   /* timeout user */
    struct thread       *thread;    /* thread that set the APC function */
    void                *callback;  /* callback APC function */
    void                *arg;       /* callback argument */
};

static void timer_dump( struct object *obj, int verbose );
static int timer_signaled( struct object *obj, struct thread *thread );
static int timer_satisfied( struct object *obj, struct thread *thread );
static unsigned int timer_map_access( struct object *obj, unsigned int access );
static void timer_destroy( struct object *obj );

static const struct object_ops timer_ops =
{
    sizeof(struct timer),      /* size */
    timer_dump,                /* dump */
    add_queue,                 /* add_queue */
    remove_queue,              /* remove_queue */
    timer_signaled,            /* signaled */
    timer_satisfied,           /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    timer_map_access,          /* map_access */
    no_lookup_name,            /* lookup_name */
    no_open_file,              /* open_file */
    no_close_handle,           /* close_handle */
    timer_destroy              /* destroy */
};


/* create a timer object */
static struct timer *create_timer( struct directory *root, const struct unicode_str *name,
                                   unsigned int attr, int manual )
{
    struct timer *timer;

    if ((timer = create_named_object_dir( root, name, attr, &timer_ops )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            /* initialize it if it didn't already exist */
            timer->manual   = manual;
            timer->signaled = 0;
            timer->when     = 0;
            timer->period   = 0;
            timer->timeout  = NULL;
            timer->thread   = NULL;
        }
    }
    return timer;
}

/* callback on timer expiration */
static void timer_callback( void *private )
{
    struct timer *timer = (struct timer *)private;

    /* queue an APC */
    if (timer->thread)
    {
        apc_call_t data;

        memset( &data, 0, sizeof(data) );
        if (timer->callback)
        {
            data.type       = APC_TIMER;
            data.timer.func = timer->callback;
            data.timer.time = timer->when;
            data.timer.arg  = timer->arg;
        }
        else data.type = APC_NONE;  /* wake up only */

        if (!thread_queue_apc( timer->thread, &timer->obj, &data ))
        {
            release_object( timer->thread );
            timer->thread = NULL;
        }
    }

    if (timer->period)  /* schedule the next expiration */
    {
        timer->when += (timeout_t)timer->period * 10000;
        timer->timeout = add_timeout_user( timer->when, timer_callback, timer );
    }
    else timer->timeout = NULL;

    /* wake up waiters */
    timer->signaled = 1;
    wake_up( &timer->obj, 0 );
}

/* cancel a running timer */
static int cancel_timer( struct timer *timer )
{
    int signaled = timer->signaled;

    if (timer->timeout)
    {
        remove_timeout_user( timer->timeout );
        timer->timeout = NULL;
    }
    if (timer->thread)
    {
        thread_cancel_apc( timer->thread, &timer->obj, APC_TIMER );
        release_object( timer->thread );
        timer->thread = NULL;
    }
    return signaled;
}

/* set the timer expiration and period */
static int set_timer( struct timer *timer, timeout_t expire, unsigned int period,
                      void *callback, void *arg )
{
    int signaled = cancel_timer( timer );
    if (timer->manual)
    {
        period = 0;  /* period doesn't make any sense for a manual timer */
        timer->signaled = 0;
    }
    if (expire > 0 && expire < current_time) expire = current_time;
    timer->when     = expire;
    timer->period   = period;
    timer->callback = callback;
    timer->arg      = arg;
    if (callback) timer->thread = (struct thread *)grab_object( current );
    timer->timeout = add_timeout_user( timer->when, timer_callback, timer );
    return signaled;
}

static void timer_dump( struct object *obj, int verbose )
{
    struct timer *timer = (struct timer *)obj;
    assert( obj->ops == &timer_ops );
    fprintf( stderr, "Timer manual=%d when=%s period=%u ",
             timer->manual, get_timeout_str(timer->when), timer->period );
    dump_object_name( &timer->obj );
    fputc( '\n', stderr );
}

static int timer_signaled( struct object *obj, struct thread *thread )
{
    struct timer *timer = (struct timer *)obj;
    assert( obj->ops == &timer_ops );
    return timer->signaled;
}

static int timer_satisfied( struct object *obj, struct thread *thread )
{
    struct timer *timer = (struct timer *)obj;
    assert( obj->ops == &timer_ops );
    if (!timer->manual) timer->signaled = 0;
    return 0;
}

static unsigned int timer_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= STANDARD_RIGHTS_READ | SYNCHRONIZE | TIMER_QUERY_STATE;
    if (access & GENERIC_WRITE)   access |= STANDARD_RIGHTS_WRITE | TIMER_MODIFY_STATE;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE;
    if (access & GENERIC_ALL)     access |= TIMER_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static void timer_destroy( struct object *obj )
{
    struct timer *timer = (struct timer *)obj;
    assert( obj->ops == &timer_ops );

    if (timer->timeout) remove_timeout_user( timer->timeout );
    if (timer->thread) release_object( timer->thread );
}

/* create a timer */
DECL_HANDLER(create_timer)
{
    struct timer *timer;
    struct unicode_str name;
    struct directory *root = NULL;

    reply->handle = 0;
    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ((timer = create_timer( root, &name, req->attributes, req->manual )))
    {
        reply->handle = alloc_handle( current->process, timer, req->access, req->attributes );
        release_object( timer );
    }

    if (root) release_object( root );
}

/* open a handle to a timer */
DECL_HANDLER(open_timer)
{
    struct unicode_str name;
    struct directory *root = NULL;
    struct timer *timer;

    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ((timer = open_object_dir( root, &name, req->attributes, &timer_ops )))
    {
        reply->handle = alloc_handle( current->process, &timer->obj, req->access, req->attributes );
        release_object( timer );
    }

    if (root) release_object( root );
}

/* set a waitable timer */
DECL_HANDLER(set_timer)
{
    struct timer *timer;

    if ((timer = (struct timer *)get_handle_obj( current->process, req->handle,
                                                 TIMER_MODIFY_STATE, &timer_ops )))
    {
        reply->signaled = set_timer( timer, req->expire, req->period, req->callback, req->arg );
        release_object( timer );
    }
}

/* cancel a waitable timer */
DECL_HANDLER(cancel_timer)
{
    struct timer *timer;

    if ((timer = (struct timer *)get_handle_obj( current->process, req->handle,
                                                 TIMER_MODIFY_STATE, &timer_ops )))
    {
        reply->signaled = cancel_timer( timer );
        release_object( timer );
    }
}

/* Get information on a waitable timer */
DECL_HANDLER(get_timer_info)
{
    struct timer *timer;

    if ((timer = (struct timer *)get_handle_obj( current->process, req->handle,
                                                 TIMER_QUERY_STATE, &timer_ops )))
    {
        reply->when      = timer->when;
        reply->signaled  = timer->signaled;
        release_object( timer );
    }
}
