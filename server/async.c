/*
 * Server-side support for async i/o operations
 *
 * Copyright (C) 1998 Alexandre Julliard
 * Copyright (C) 2000 Mike McCormack
 *
 * TODO:
 *  Fix up WaitCommEvent operations. Currently only EV_RXCHAR is supported.
 *    This may require modifications to the linux kernel to enable select
 *    to wait on Modem Status Register deltas. (delta DCD, CTS, DSR or RING)
 *
 */

#include "config.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "winerror.h"
#include "winbase.h"

#include "handle.h"
#include "thread.h"
#include "request.h"

struct async 
{
    struct object           obj;
    void                   *client_overlapped;
    int                     type;
    int                     result;
    int                     count;
    int                     eventmask;
    struct async           *next;
    struct timeval          tv;
    struct timeout_user    *timeout;
    struct wait_queue_entry wait;
    void                   *buffer;
    void                   *func;
    struct thread          *thread;
    struct object          *file;
};

static void async_dump( struct object *obj, int verbose );
static void async_destroy( struct object *obj );
static int async_get_poll_events( struct object *obj );
static int async_get_fd( struct object *obj );
static int async_get_info( struct object *obj, struct get_file_info_request *req );
static void async_poll_event( struct object *obj, int event );
static void overlapped_timeout (void *private);

static const struct object_ops async_ops =
{
    sizeof(struct async),         /* size */
    async_dump,                   /* dump */
    default_poll_add_queue,       /* add_queue */
    default_poll_remove_queue,    /* remove_queue */
    default_poll_signaled,        /* signaled */
    no_satisfied,                 /* satisfied */
    async_get_poll_events,        /* get_poll_events */
    async_poll_event,             /* poll_event */
    async_get_fd,                 /* get_fd */
    no_flush,                     /* flush */
    async_get_info,               /* get_file_info */
    async_destroy                 /* destroy */
};

static void async_dump( struct object *obj, int verbose )
{
    struct async *ov = (struct async *)obj;

    assert( obj->ops == &async_ops );

    fprintf( stderr, "async: overlapped %p %s\n", 
                 ov->client_overlapped, ov->timeout?"with timeout":"");
}

/* same as file_destroy, but don't delete comm ports */
static void async_destroy( struct object *obj )
{
    struct async *ov = (struct async *)obj;
    assert( obj->ops == &async_ops );

    if(ov->timeout)
    {
        remove_timeout_user(ov->timeout);
        ov->timeout = NULL;
    }
}

struct async *get_async_obj( struct process *process, handle_t handle, unsigned int access )
{
    return (struct async *)get_handle_obj( process, handle, access, &async_ops );
}

static int async_get_poll_events( struct object *obj )
{
    struct async *ov = (struct async *)obj;
    assert( obj->ops == &async_ops );

    /* FIXME: this should be a function pointer */
    return serial_async_get_poll_events(ov);
}

static int async_get_fd( struct object *obj )
{
    struct async *async = (struct async *)obj;
    assert( obj->ops == &async_ops );
    return async->obj.fd;
}

static int async_get_info( struct object *obj, struct get_file_info_request *req ) {
    assert( obj->ops == &async_ops );
    req->type        = FILE_TYPE_CHAR;
    req->attr        = 0;
    req->access_time = 0;
    req->write_time  = 0;
    req->size_high   = 0;
    req->size_low    = 0;
    req->links       = 0;
    req->index_high  = 0;
    req->index_low   = 0;
    req->serial      = 0;
    return 1;
}

/* data access functions */
int async_type(struct async *ov)
{
    return ov->type;
}

int async_count(struct async *ov)
{
    return ov->count;
}

int async_get_eventmask(struct async *ov)
{
    return ov->eventmask;
}

int async_set_eventmask(struct async *ov, int eventmask)
{
    return ov->eventmask = eventmask;
}

DECL_HANDLER(create_async)
{
    struct object *obj;
    struct async *ov = NULL;
    int fd;

    req->ov_handle = 0;
    if (!(obj = get_handle_obj( current->process, req->file_handle, 0, NULL)) )
        return;

    fd = dup(obj->fd);
    if(fd<0)
    {
        release_object(obj);
        set_error(STATUS_UNSUCCESSFUL);
        return;
    }

    if(0>fcntl(fd, F_SETFL, O_NONBLOCK))
    {
        release_object(obj);
        set_error(STATUS_UNSUCCESSFUL);
        return;
    }

    ov = alloc_object (&async_ops, fd);
    if(!ov)
    {
        release_object(obj);
        set_error(STATUS_UNSUCCESSFUL);
        return;
    }

    ov->client_overlapped = req->overlapped;
    ov->next    = NULL;
    ov->timeout = NULL;
    ov->type    = req->type;
    ov->thread  = current;
    ov->func    = req->func;
    ov->file    = obj;
    ov->buffer  = req->buffer;
    ov->count   = req->count;
    ov->tv.tv_sec = 0;
    ov->tv.tv_usec = 0;

    /* FIXME: this should be a function pointer */
    serial_async_setup(obj,ov);

    if( ov->tv.tv_sec || ov->tv.tv_usec )
    {
        ov->timeout = add_timeout_user(&ov->tv, overlapped_timeout, ov);
    }

    ov->obj.ops->add_queue(&ov->obj,&ov->wait);

    req->ov_handle = alloc_handle( current->process, ov, GENERIC_READ|GENERIC_WRITE, 0 );

    release_object(obj);
}

/* handler for async poll() events */
static void async_poll_event( struct object *obj, int event )
{
    struct async *ov = (struct async *) obj;

    /* queue an APC in the client thread to do our dirty work */
    ov->obj.ops->remove_queue(&ov->obj,&ov->wait);
    if(ov->timeout)
    {
        remove_timeout_user(ov->timeout);
        ov->timeout = NULL;
    }

    /* FIXME: this should be a function pointer */
    event = serial_async_poll_event(obj,event);

    thread_queue_apc(ov->thread, NULL, ov->func, APC_ASYNC, 1, 3,
                     ov->client_overlapped, ov->buffer, event);
}

/* handler for async i/o timeouts */
static void overlapped_timeout (void *private)
{
    struct async *ov = (struct async *) private;

    ov->obj.ops->remove_queue(&ov->obj,&ov->wait);
    ov->timeout = NULL;

    thread_queue_apc(ov->thread, NULL, ov->func, APC_ASYNC, 1, 3,
                     ov->client_overlapped,ov->buffer, 0);
}

void async_add_timeout(struct async *ov, int timeout)
{
    if(timeout)
    {
        gettimeofday(&ov->tv,0);
        add_timeout(&ov->tv,timeout);
    }
}

DECL_HANDLER(async_result)
{
    struct async *ov;

    if ((ov = get_async_obj( current->process, req->ov_handle, 0 )))
    {
        ov->result = req->result;
        if(ov->result == STATUS_PENDING)
        {
            ov->obj.ops->add_queue(&ov->obj,&ov->wait);
            if( (ov->tv.tv_sec || ov->tv.tv_usec) && !ov->timeout)
            {
                ov->timeout = add_timeout_user(&ov->tv, overlapped_timeout, ov);
            }
        }
        release_object( ov );
    }
}

