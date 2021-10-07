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

static const WCHAR timer_name[] = {'T','i','m','e','r'};

struct type_descr timer_type =
{
    { timer_name, sizeof(timer_name) },   /* name */
    TIMER_ALL_ACCESS,                     /* valid_access */
    {                                     /* mapping */
        STANDARD_RIGHTS_READ | TIMER_QUERY_STATE,
        STANDARD_RIGHTS_WRITE | TIMER_MODIFY_STATE,
        STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
        TIMER_ALL_ACCESS
    },
};

struct timer
{
    struct object        obj;       /* object header */
    int                  manual;    /* manual reset */
    int                  signaled;  /* current signaled state */
    unsigned int         period;    /* timer period in ms */
    abstime_t            when;      /* next expiration */
    struct timeout_user *timeout;   /* timeout user */
    struct thread       *thread;    /* thread that set the APC function */
    client_ptr_t         callback;  /* callback APC function */
    client_ptr_t         arg;       /* callback argument */
};

static void timer_dump( struct object *obj, int verbose );
static int timer_signaled( struct object *obj, struct wait_queue_entry *entry );
static void timer_satisfied( struct object *obj, struct wait_queue_entry *entry );
static void timer_destroy( struct object *obj );

static const struct object_ops timer_ops =
{
    sizeof(struct timer),      /* size */
    &timer_type,               /* type */
    timer_dump,                /* dump */
    add_queue,                 /* add_queue */
    remove_queue,              /* remove_queue */
    timer_signaled,            /* signaled */
    timer_satisfied,           /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    default_map_access,        /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    default_get_full_name,     /* get_full_name */
    no_lookup_name,            /* lookup_name */
    directory_link_name,       /* link_name */
    default_unlink_name,       /* unlink_name */
    no_open_file,              /* open_file */
    no_kernel_obj_list,        /* get_kernel_obj_list */
    no_close_handle,           /* close_handle */
    timer_destroy              /* destroy */
};


/* create a timer object */
static struct timer *create_timer( struct object *root, const struct unicode_str *name,
                                   unsigned int attr, int manual, const struct security_descriptor *sd )
{
    struct timer *timer;

    if ((timer = create_named_object( root, &timer_ops, name, attr, sd )))
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

        assert (timer->callback);
        memset( &data, 0, sizeof(data) );
        data.type         = APC_USER;
        data.user.func    = timer->callback;
        data.user.args[0] = timer->arg;
        data.user.args[1] = (unsigned int)timer->when;
        data.user.args[2] = timer->when >> 32;

        if (!thread_queue_apc( NULL, timer->thread, &timer->obj, &data ))
        {
            release_object( timer->thread );
            timer->thread = NULL;
        }
    }

    if (timer->period)  /* schedule the next expiration */
    {
        if (timer->when > 0) timer->when = -monotonic_time;
        timer->when -= (abstime_t)timer->period * 10000;
        timer->timeout = add_timeout_user( abstime_to_timeout(timer->when), timer_callback, timer );
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
        thread_cancel_apc( timer->thread, &timer->obj, APC_USER );
        release_object( timer->thread );
        timer->thread = NULL;
    }
    return signaled;
}

/* set the timer expiration and period */
static int set_timer( struct timer *timer, timeout_t expire, unsigned int period,
                      client_ptr_t callback, client_ptr_t arg )
{
    int signaled = cancel_timer( timer );
    if (timer->manual)
    {
        period = 0;  /* period doesn't make any sense for a manual timer */
        timer->signaled = 0;
    }
    timer->when     = (expire <= 0) ? expire - monotonic_time : max( expire, current_time );
    timer->period   = period;
    timer->callback = callback;
    timer->arg      = arg;
    if (callback) timer->thread = (struct thread *)grab_object( current );
    if (expire != TIMEOUT_INFINITE)
        timer->timeout = add_timeout_user( expire, timer_callback, timer );
    return signaled;
}

static void timer_dump( struct object *obj, int verbose )
{
    struct timer *timer = (struct timer *)obj;
    timeout_t timeout = abstime_to_timeout( timer->when );
    assert( obj->ops == &timer_ops );
    fprintf( stderr, "Timer manual=%d when=%s period=%u\n",
             timer->manual, get_timeout_str(timeout), timer->period );
}

static int timer_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct timer *timer = (struct timer *)obj;
    assert( obj->ops == &timer_ops );
    return timer->signaled;
}

static void timer_satisfied( struct object *obj, struct wait_queue_entry *entry )
{
    struct timer *timer = (struct timer *)obj;
    assert( obj->ops == &timer_ops );
    if (!timer->manual) timer->signaled = 0;
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
    struct object *root;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, &root );

    if (!objattr) return;

    if ((timer = create_timer( root, &name, objattr->attributes, req->manual, sd )))
    {
        reply->handle = alloc_handle( current->process, timer, req->access, objattr->attributes );
        release_object( timer );
    }

    if (root) release_object( root );
}

/* open a handle to a timer */
DECL_HANDLER(open_timer)
{
    struct unicode_str name = get_req_unicode_str();

    reply->handle = open_object( current->process, req->rootdir, req->access,
                                 &timer_ops, &name, req->attributes );
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
