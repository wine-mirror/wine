/*
 * Server-side serial port communications management
 *
 * Copyright (C) 1998 Alexandre Julliard
 * Copyright (C) 2000 Mike McCormack
 *
 * TODO:
 *  Add async read, write and WaitCommEvent handling.
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

static void serial_dump( struct object *obj, int verbose );
static int serial_get_fd( struct object *obj );
static int serial_get_info( struct object *obj, struct get_file_info_request *req );
static int serial_get_poll_events( struct object *obj );

struct serial
{
    struct object       obj;
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

    /* FIXME: add dcb, comm status, handler module, sharing */
};

static const struct object_ops serial_ops =
{
    sizeof(struct serial),        /* size */
    serial_dump,                  /* dump */
    default_poll_add_queue,       /* add_queue */
    default_poll_remove_queue,    /* remove_queue */
    default_poll_signaled,        /* signaled */
    no_satisfied,                 /* satisfied */
    serial_get_poll_events,       /* get_poll_events */
    default_poll_event,           /* poll_event */
    serial_get_fd,                /* get_fd */
    no_flush,                     /* flush */
    serial_get_info,              /* get_file_info */
    no_destroy                    /* destroy */
};

/* SERIAL PORT functions */

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

    if ((serial = alloc_object( &serial_ops, fd )))
    {
        serial->attrib       = attributes;
        serial->access       = access;
        serial->readinterval = 0;
        serial->readmult     = 0;
        serial->readconst    = 0;
        serial->writemult    = 0;
        serial->writeconst   = 0;
        serial->eventmask    = 0;
        serial->commerror    = 0;
    }
    return serial;
}

static void serial_dump( struct object *obj, int verbose )
{
    struct serial *serial = (struct serial *)obj;
    assert( obj->ops == &serial_ops );
    fprintf( stderr, "Port fd=%d mask=%x\n", serial->obj.fd, serial->eventmask );
}

struct serial *get_serial_obj( struct process *process, handle_t handle, unsigned int access )
{
    return (struct serial *)get_handle_obj( process, handle, access, &serial_ops );
}

static int serial_get_poll_events( struct object *obj )
{
    struct serial *serial = (struct serial *)obj;
    int events = 0;
    assert( obj->ops == &serial_ops );
    if (serial->access & GENERIC_READ) events |= POLLIN;
    if (serial->access & GENERIC_WRITE) events |= POLLOUT;
    return events;
}

static int serial_get_fd( struct object *obj )
{
    struct serial *serial = (struct serial *)obj;
    assert( obj->ops == &serial_ops );
    return serial->obj.fd;
}

static int serial_get_info( struct object *obj, struct get_file_info_request *req )
{
    struct serial *serial = (struct serial *) obj;
    assert( obj->ops == &serial_ops );

    if (req)
    {
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
    }

    if(serial->attrib & FILE_FLAG_OVERLAPPED)
        return FD_TYPE_OVERLAPPED;

    return FD_TYPE_TIMEOUT;
}

/* these function calculates the timeout for an async operation
   on a serial port */
int get_serial_async_timeout(struct object *obj, int type, int count)
{
    struct serial *serial = (struct serial *)obj;

    if(obj->ops != &serial_ops)
        return 0;

    switch(type)
    {
    case ASYNC_TYPE_READ:
        return serial->readconst + serial->readmult*count;
    case ASYNC_TYPE_WRITE:
        return serial->writeconst + serial->writemult*count;
    }
    return 0;
}


/* create a serial */
DECL_HANDLER(create_serial)
{
    struct serial *serial;

    req->handle = 0;
    if ((serial = create_serial( get_req_data(req), get_req_data_size(req), req->access, req->attributes )))
    {
        req->handle = alloc_handle( current->process, serial, req->access, req->inherit );
        release_object( serial );
    }
}

DECL_HANDLER(get_serial_info)
{
    struct serial *serial;

    if ((serial = get_serial_obj( current->process, req->handle, 0 )))
    {
        /* timeouts */
        req->readinterval = serial->readinterval;
        req->readconst    = serial->readconst;
        req->readmult     = serial->readmult;
        req->writeconst   = serial->writeconst;
        req->writemult    = serial->writemult;

        /* event mask */
        req->eventmask    = serial->eventmask;

        /* comm port error status */
        req->commerror    = serial->commerror;

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
        }

        /* comm port error status */
        if(req->flags & SERIALINFO_SET_ERROR)
        {
            serial->commerror = req->commerror;
        }

        release_object( serial );
    }
}

