/*
 * Server-side serial port communications management
 *
 * Copyright (C) 1998 Alexandre Julliard
 * Copyright (C) 2000,2001 Mike McCormack
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#ifdef HAVE_UTIME_H
#include <utime.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include "winerror.h"
#include "winbase.h"

#include "file.h"
#include "handle.h"
#include "thread.h"
#include "request.h"
#include "async.h"

static void serial_dump( struct object *obj, int verbose );
static struct fd *serial_get_fd( struct object *obj );
static void serial_destroy(struct object *obj);

static int serial_get_poll_events( struct fd *fd );
static void serial_poll_event( struct fd *fd, int event );
static int serial_get_info( struct fd *fd, struct get_file_info_reply *reply, int *flags );
static int serial_flush( struct fd *fd, struct event **event );
static void serial_queue_async(struct fd *fd, void *ptr, unsigned int status, int type, int count);

struct serial
{
    struct object       obj;
    struct fd          *fd;
    unsigned int        access;
    unsigned int        attrib;

    /* timeout values */
    unsigned int        readinterval;
    unsigned int        readconst;
    unsigned int        readmult;
    unsigned int        writeconst;
    unsigned int        writemult;

    unsigned int        eventmask;
    unsigned int        commerror;

    struct termios      original;

    struct async_queue  read_q;
    struct async_queue  write_q;
    struct async_queue  wait_q;

    /* FIXME: add dcb, comm status, handler module, sharing */
};

static const struct object_ops serial_ops =
{
    sizeof(struct serial),        /* size */
    serial_dump,                  /* dump */
    default_fd_add_queue,         /* add_queue */
    default_fd_remove_queue,      /* remove_queue */
    default_fd_signaled,          /* signaled */
    no_satisfied,                 /* satisfied */
    serial_get_fd,                /* get_fd */
    serial_destroy                /* destroy */
};

static const struct fd_ops serial_fd_ops =
{
    serial_get_poll_events,       /* get_poll_events */
    serial_poll_event,            /* poll_event */
    serial_flush,                 /* flush */
    serial_get_info,              /* get_file_info */
    serial_queue_async            /* queue_async */
};

static struct serial *create_serial( const char *nameptr, size_t len, unsigned int access, int attributes )
{
    struct serial *serial;
    struct termios tios;
    int fd, flags = 0;
    char *name;

    if (!(name = mem_alloc( len + 1 ))) return NULL;
    memcpy( name, nameptr, len );
    name[len] = 0;

    switch(access & (GENERIC_READ | GENERIC_WRITE))
    {
    case GENERIC_READ:  flags |= O_RDONLY; break;
    case GENERIC_WRITE: flags |= O_WRONLY; break;
    case GENERIC_READ|GENERIC_WRITE: flags |= O_RDWR; break;
    default: break;
    }

    flags |= O_NONBLOCK;

    fd = open( name, flags );
    free( name );
    if (fd < 0)
    {
        file_set_error();
        return NULL;
    }

    /* check its really a serial port */
    if (tcgetattr(fd,&tios))
    {
        file_set_error();
        close( fd );
        return NULL;
    }

    /* set the fd back to blocking if necessary */
    if( ! (attributes & FILE_FLAG_OVERLAPPED) )
       if(0>fcntl(fd, F_SETFL, 0))
           perror("fcntl");

    if (!(serial = alloc_object( &serial_ops )))
    {
        close( fd );
        return NULL;
    }
    serial->attrib       = attributes;
    serial->access       = access;
    serial->readinterval = 0;
    serial->readmult     = 0;
    serial->readconst    = 0;
    serial->writemult    = 0;
    serial->writeconst   = 0;
    serial->eventmask    = 0;
    serial->commerror    = 0;
    init_async_queue(&serial->read_q);
    init_async_queue(&serial->write_q);
    init_async_queue(&serial->wait_q);
    if (!(serial->fd = create_anonymous_fd( &serial_fd_ops, fd, &serial->obj )))
    {
        release_object( serial );
        return NULL;
    }
    return serial;
}

static struct fd *serial_get_fd( struct object *obj )
{
    struct serial *serial = (struct serial *)obj;
    return (struct fd *)grab_object( serial->fd );
}

static void serial_destroy( struct object *obj)
{
    struct serial *serial = (struct serial *)obj;

    destroy_async_queue(&serial->read_q);
    destroy_async_queue(&serial->write_q);
    destroy_async_queue(&serial->wait_q);
    if (serial->fd) release_object( serial->fd );
}

static void serial_dump( struct object *obj, int verbose )
{
    struct serial *serial = (struct serial *)obj;
    assert( obj->ops == &serial_ops );
    fprintf( stderr, "Port fd=%p mask=%x\n", serial->fd, serial->eventmask );
}

static struct serial *get_serial_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct serial *)get_handle_obj( process, handle, access, &serial_ops );
}

static int serial_get_poll_events( struct fd *fd )
{
    struct serial *serial = get_fd_user( fd );
    int events = 0;
    assert( serial->obj.ops == &serial_ops );

    if(IS_READY(serial->read_q))
        events |= POLLIN;
    if(IS_READY(serial->write_q))
        events |= POLLOUT;
    if(IS_READY(serial->wait_q))
        events |= POLLIN;

    /* fprintf(stderr,"poll events are %04x\n",events); */

    return events;
}

static int serial_get_info( struct fd *fd, struct get_file_info_reply *reply, int *flags )
{
    struct serial *serial = get_fd_user( fd );
    assert( serial->obj.ops == &serial_ops );

    if (reply)
    {
        reply->type        = FILE_TYPE_CHAR;
        reply->attr        = 0;
        reply->access_time = 0;
        reply->write_time  = 0;
        reply->size_high   = 0;
        reply->size_low    = 0;
        reply->links       = 0;
        reply->index_high  = 0;
        reply->index_low   = 0;
        reply->serial      = 0;
    }

    *flags = 0;
    if(serial->attrib & FILE_FLAG_OVERLAPPED)
        *flags |= FD_FLAG_OVERLAPPED;
    else if(!((serial->readinterval == MAXDWORD) &&
              (serial->readmult == 0) && (serial->readconst == 0)) )
        *flags |= FD_FLAG_TIMEOUT;

    return FD_TYPE_DEFAULT;
}

static void serial_poll_event(struct fd *fd, int event)
{
    struct serial *serial = get_fd_user( fd );

    /* fprintf(stderr,"Poll event %02x\n",event); */

    if(IS_READY(serial->read_q) && (POLLIN & event) )
        async_notify(serial->read_q.head,STATUS_ALERTED);

    if(IS_READY(serial->write_q) && (POLLOUT & event) )
        async_notify(serial->write_q.head,STATUS_ALERTED);

    if(IS_READY(serial->wait_q) && (POLLIN & event) )
        async_notify(serial->wait_q.head,STATUS_ALERTED);

    set_fd_events( fd, serial_get_poll_events(fd) );
}

static void serial_queue_async(struct fd *fd, void *ptr, unsigned int status, int type, int count)
{
    struct serial *serial = get_fd_user( fd );
    struct async_queue *q;
    struct async *async;
    int timeout;

    assert(serial->obj.ops == &serial_ops);

    switch(type)
    {
    case ASYNC_TYPE_READ:
        q = &serial->read_q;
        timeout = serial->readconst + serial->readmult*count;
        break;
    case ASYNC_TYPE_WAIT:
        q = &serial->wait_q;
        timeout = 0;
        break;
    case ASYNC_TYPE_WRITE:
        q = &serial->write_q;
        timeout = serial->writeconst + serial->writemult*count;
        break;
    default:
        set_error(STATUS_INVALID_PARAMETER);
        return;
    }

    async = find_async ( q, current, ptr );

    if ( status == STATUS_PENDING )
    {
        int events;

        if ( !async )
            async = create_async ( &serial->obj, current, ptr );
        if ( !async )
            return;

        async->status = STATUS_PENDING;
        if(!async->q)
        {
            async_add_timeout(async,timeout);
            async_insert(q, async);
        }

        /* Check if the new pending request can be served immediately */
        events = check_fd_events( fd, serial_get_poll_events( fd ) );
        if (events)
        {
            /* serial_poll_event() calls set_select_events() */
            serial_poll_event( fd, events );
            return;
        }
    }
    else if ( async ) destroy_async ( async );
    else set_error ( STATUS_INVALID_PARAMETER );

    set_fd_events ( fd, serial_get_poll_events( fd ));
}

static int serial_flush( struct fd *fd, struct event **event )
{
    /* MSDN says: If hFile is a handle to a communications device,
     * the function only flushes the transmit buffer.
     */
    int ret = (tcflush( get_unix_fd(fd), TCOFLUSH ) != -1);
    if (!ret) file_set_error();
    return ret;
}

/* create a serial */
DECL_HANDLER(create_serial)
{
    struct serial *serial;

    reply->handle = 0;
    if ((serial = create_serial( get_req_data(), get_req_data_size(), req->access, req->attributes )))
    {
        reply->handle = alloc_handle( current->process, serial, req->access, req->inherit );
        release_object( serial );
    }
}

DECL_HANDLER(get_serial_info)
{
    struct serial *serial;

    if ((serial = get_serial_obj( current->process, req->handle, 0 )))
    {
        /* timeouts */
        reply->readinterval = serial->readinterval;
        reply->readconst    = serial->readconst;
        reply->readmult     = serial->readmult;
        reply->writeconst   = serial->writeconst;
        reply->writemult    = serial->writemult;

        /* event mask */
        reply->eventmask    = serial->eventmask;

        /* comm port error status */
        reply->commerror    = serial->commerror;

        release_object( serial );
    }
}

DECL_HANDLER(set_serial_info)
{
    struct serial *serial;

    if ((serial = get_serial_obj( current->process, req->handle, 0 )))
    {
        /* timeouts */
        if(req->flags & SERIALINFO_SET_TIMEOUTS)
        {
            serial->readinterval = req->readinterval;
            serial->readconst    = req->readconst;
            serial->readmult     = req->readmult;
            serial->writeconst   = req->writeconst;
            serial->writemult    = req->writemult;
        }

        /* event mask */
        if(req->flags & SERIALINFO_SET_MASK)
        {
            serial->eventmask = req->eventmask;
            if(!serial->eventmask)
            {
                while(serial->wait_q.head)
                {
                    async_notify(serial->wait_q.head, STATUS_SUCCESS);
                    destroy_async(serial->wait_q.head);
                }
            }
        }

        /* comm port error status */
        if(req->flags & SERIALINFO_SET_ERROR)
        {
            serial->commerror = req->commerror;
        }

        release_object( serial );
    }
}
