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

static void serial_dump( struct object *obj, int verbose );
static void serial_destroy( struct object *obj );
static int serial_get_read_fd( struct object *obj );
static int serial_get_write_fd( struct object *obj );
static int serial_get_info( struct object *obj, struct get_file_info_request *req );
static int serial_get_poll_events( struct object *obj );

struct serial
{
    struct object       obj;
    char                name[16]; /* eg. /dev/ttyS1 */
    int                 access;

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
    serial_get_read_fd,           /* get_read_fd */
    serial_get_write_fd,          /* get_write_fd */
    no_flush,                     /* flush */
    serial_get_info,              /* get_file_info */
    serial_destroy                /* destroy */
};

/* SERIAL PORT functions */

static void serial_dump( struct object *obj, int verbose )
{
    struct serial *serial = (struct serial *)obj;
    assert( obj->ops == &serial_ops );

    fprintf( stderr, "Port fd=%d name='%s' mask=%x\n", 
             serial->obj.fd, serial->name,serial->eventmask);
}

/* same as file_destroy, but don't delete comm ports */
static void serial_destroy( struct object *obj )
{
    assert( obj->ops == &serial_ops );
}

struct serial *get_serial_obj( struct process *process, int handle, unsigned int access )
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

static int serial_get_read_fd( struct object *obj )
{
    struct serial *serial = (struct serial *)obj;
    assert( obj->ops == &serial_ops );
    return dup( serial->obj.fd );
}

static int serial_get_write_fd( struct object *obj )
{
    struct serial *serial = (struct serial *)obj;
    assert( obj->ops == &serial_ops );
    return dup( serial->obj.fd );
}

static int serial_get_info( struct object *obj, struct get_file_info_request *req )
{
    assert( obj->ops == &serial_ops );
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

/* create a serial */
DECL_HANDLER(create_serial)
{
    struct serial *serial;
    int fd,flags;
    struct termios tios;

    req->handle = -1;

    flags = 0;
    switch(req->access & (GENERIC_READ | GENERIC_WRITE))
    {
    case GENERIC_READ:  flags |= O_RDONLY; break;
    case GENERIC_WRITE: flags |= O_WRONLY; break;
    case GENERIC_READ|GENERIC_WRITE: flags |= O_RDWR; break;
    default: break;
    }

    fd = open( req->name, flags );
    if(fd < 0)
    {
        file_set_error();
        return;
    }

    /* check its really a serial port */
    if(0>tcgetattr(fd,&tios))
    {
        file_set_error();
        close(fd);
        return;
    }

    serial = alloc_object( &serial_ops, fd );
    if (serial)
    {
        strncpy(serial->name,req->name,sizeof serial->name);
        serial->name[sizeof(serial->name)-1] = 0;

        serial->access       = req->access;
        serial->readinterval = 0;
        serial->readmult     = 0;
        serial->readconst    = 0;
        serial->writemult    = 0;
        serial->writeconst   = 0;
        serial->eventmask    = 0;
        serial->commerror    = 0;

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

