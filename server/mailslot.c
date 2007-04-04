/*
 * Server-side mailslot management
 *
 * Copyright (C) 1998 Alexandre Julliard
 * Copyright (C) 2005 Mike McCormack
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
 *
 */

#include "config.h"
#include "wine/port.h"
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/unicode.h"

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#include "windef.h"
#include "winternl.h"

#include "file.h"
#include "handle.h"
#include "thread.h"
#include "request.h"

struct mailslot
{
    struct object       obj;
    struct fd          *fd;
    struct fd          *write_fd;
    unsigned int        max_msgsize;
    int                 read_timeout;
    struct list         writers;
};

/* mailslot functions */
static void mailslot_dump( struct object*, int );
static struct fd *mailslot_get_fd( struct object * );
static unsigned int mailslot_map_access( struct object *obj, unsigned int access );
static struct object *mailslot_open_file( struct object *obj, unsigned int access,
                                          unsigned int sharing, unsigned int options );
static void mailslot_destroy( struct object * );

static const struct object_ops mailslot_ops =
{
    sizeof(struct mailslot),   /* size */
    mailslot_dump,             /* dump */
    add_queue,                 /* add_queue */
    remove_queue,              /* remove_queue */
    default_fd_signaled,       /* signaled */
    no_satisfied,              /* satisfied */
    no_signal,                 /* signal */
    mailslot_get_fd,           /* get_fd */
    mailslot_map_access,       /* map_access */
    no_lookup_name,            /* lookup_name */
    mailslot_open_file,        /* open_file */
    fd_close_handle,           /* close_handle */
    mailslot_destroy           /* destroy */
};

static enum server_fd_type mailslot_get_info( struct fd *fd, int *flags );
static void mailslot_queue_async( struct fd *fd, const async_data_t *data, int type, int count );

static const struct fd_ops mailslot_fd_ops =
{
    default_fd_get_poll_events, /* get_poll_events */
    default_poll_event,         /* poll_event */
    no_flush,                   /* flush */
    mailslot_get_info,          /* get_file_info */
    mailslot_queue_async,       /* queue_async */
    default_fd_cancel_async     /* cancel_async */
};


struct mail_writer
{
    struct object         obj;
    struct mailslot      *mailslot;
    struct list           entry;
    unsigned int          access;
    unsigned int          sharing;
};

static void mail_writer_dump( struct object *obj, int verbose );
static struct fd *mail_writer_get_fd( struct object *obj );
static unsigned int mail_writer_map_access( struct object *obj, unsigned int access );
static void mail_writer_destroy( struct object *obj);

static const struct object_ops mail_writer_ops =
{
    sizeof(struct mail_writer), /* size */
    mail_writer_dump,           /* dump */
    no_add_queue,               /* add_queue */
    NULL,                       /* remove_queue */
    NULL,                       /* signaled */
    NULL,                       /* satisfied */
    no_signal,                  /* signal */
    mail_writer_get_fd,         /* get_fd */
    mail_writer_map_access,     /* map_access */
    no_lookup_name,             /* lookup_name */
    no_open_file,               /* open_file */
    fd_close_handle,            /* close_handle */
    mail_writer_destroy         /* destroy */
};

static enum server_fd_type mail_writer_get_info( struct fd *fd, int *flags );

static const struct fd_ops mail_writer_fd_ops =
{
    NULL,                        /* get_poll_events */
    NULL,                        /* poll_event */
    no_flush,                    /* flush */
    mail_writer_get_info,        /* get_file_info */
    no_queue_async,              /* queue_async */
    NULL                         /* cancel_async */
};


struct mailslot_device
{
    struct object       obj;         /* object header */
    struct fd          *fd;          /* pseudo-fd for ioctls */
    struct namespace   *mailslots;   /* mailslot namespace */
};

static void mailslot_device_dump( struct object *obj, int verbose );
static struct fd *mailslot_device_get_fd( struct object *obj );
static struct object *mailslot_device_lookup_name( struct object *obj, struct unicode_str *name,
                                                   unsigned int attr );
static struct object *mailslot_device_open_file( struct object *obj, unsigned int access,
                                                 unsigned int sharing, unsigned int options );
static void mailslot_device_destroy( struct object *obj );
static enum server_fd_type mailslot_device_get_file_info( struct fd *fd, int *flags );

static const struct object_ops mailslot_device_ops =
{
    sizeof(struct mailslot_device), /* size */
    mailslot_device_dump,           /* dump */
    no_add_queue,                   /* add_queue */
    NULL,                           /* remove_queue */
    NULL,                           /* signaled */
    no_satisfied,                   /* satisfied */
    no_signal,                      /* signal */
    mailslot_device_get_fd,         /* get_fd */
    no_map_access,                  /* map_access */
    mailslot_device_lookup_name,    /* lookup_name */
    mailslot_device_open_file,      /* open_file */
    fd_close_handle,                /* close_handle */
    mailslot_device_destroy         /* destroy */
};

static const struct fd_ops mailslot_device_fd_ops =
{
    default_fd_get_poll_events,     /* get_poll_events */
    default_poll_event,             /* poll_event */
    no_flush,                       /* flush */
    mailslot_device_get_file_info,  /* get_file_info */
    default_fd_queue_async,         /* queue_async */
    default_fd_cancel_async         /* cancel_async */
};

static void mailslot_destroy( struct object *obj)
{
    struct mailslot *mailslot = (struct mailslot *) obj;

    assert( mailslot->fd );
    assert( mailslot->write_fd );

    release_object( mailslot->fd );
    release_object( mailslot->write_fd );
}

static void mailslot_dump( struct object *obj, int verbose )
{
    struct mailslot *mailslot = (struct mailslot *) obj;

    assert( obj->ops == &mailslot_ops );
    fprintf( stderr, "Mailslot max_msgsize=%d read_timeout=%d\n",
             mailslot->max_msgsize, mailslot->read_timeout );
}

static enum server_fd_type mailslot_get_info( struct fd *fd, int *flags )
{
    struct mailslot *mailslot = get_fd_user( fd );
    assert( mailslot->obj.ops == &mailslot_ops );
    *flags = FD_FLAG_TIMEOUT | FD_FLAG_AVAILABLE;
    return FD_TYPE_MAILSLOT;
}

static struct fd *mailslot_get_fd( struct object *obj )
{
    struct mailslot *mailslot = (struct mailslot *) obj;

    return (struct fd *)grab_object( mailslot->fd );
}

static unsigned int mailslot_map_access( struct object *obj, unsigned int access )
{
    /* mailslots can only be read */
    if (access & GENERIC_READ)    access |= FILE_GENERIC_READ;
    if (access & GENERIC_ALL)     access |= FILE_GENERIC_READ;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static struct object *mailslot_open_file( struct object *obj, unsigned int access,
                                          unsigned int sharing, unsigned int options )
{
    struct mailslot *mailslot = (struct mailslot *)obj;
    struct mail_writer *writer;

    if (!(sharing & FILE_SHARE_READ))
    {
        set_error( STATUS_SHARING_VIOLATION );
        return NULL;
    }

    if (!list_empty( &mailslot->writers ))
    {
        /* Readers and writers cannot be mixed.
         * If there's more than one writer, all writers must open with FILE_SHARE_WRITE
         */
        writer = LIST_ENTRY( list_head(&mailslot->writers), struct mail_writer, entry );

        if (((access & (GENERIC_WRITE|FILE_WRITE_DATA)) || (writer->access & FILE_WRITE_DATA)) &&
           !((sharing & FILE_SHARE_WRITE) && (writer->sharing & FILE_SHARE_WRITE)))
        {
            set_error( STATUS_SHARING_VIOLATION );
            return NULL;
        }
    }

    if (!(writer = alloc_object( &mail_writer_ops ))) return NULL;
    grab_object( mailslot );
    writer->mailslot = mailslot;
    writer->access   = mail_writer_map_access( &writer->obj, access );
    writer->sharing  = sharing;

    list_add_head( &mailslot->writers, &writer->entry );
    return &writer->obj;
}

static void mailslot_queue_async( struct fd *fd, const async_data_t *data, int type, int count )
{
    struct mailslot *mailslot = get_fd_user( fd );
    struct async *async;

    assert(mailslot->obj.ops == &mailslot_ops);

    if ((async = fd_queue_async( fd, data, type, count )))
    {
        if (mailslot->read_timeout != -1)
        {
            struct timeval when = current_time;
            add_timeout( &when, max(1,mailslot->read_timeout) );
            async_set_timeout( async, &when, STATUS_IO_TIMEOUT );
        }
        release_object( async );
        set_error( STATUS_PENDING );
    }
}

static void mailslot_device_dump( struct object *obj, int verbose )
{
    assert( obj->ops == &mailslot_device_ops );
    fprintf( stderr, "Mail slot device\n" );
}

static struct fd *mailslot_device_get_fd( struct object *obj )
{
    struct mailslot_device *device = (struct mailslot_device *)obj;
    return (struct fd *)grab_object( device->fd );
}

static struct object *mailslot_device_lookup_name( struct object *obj, struct unicode_str *name,
                                                   unsigned int attr )
{
    struct mailslot_device *device = (struct mailslot_device*)obj;
    struct object *found;

    assert( obj->ops == &mailslot_device_ops );

    if ((found = find_object( device->mailslots, name, attr | OBJ_CASE_INSENSITIVE )))
        name->len = 0;

    return found;
}

static struct object *mailslot_device_open_file( struct object *obj, unsigned int access,
                                                 unsigned int sharing, unsigned int options )
{
    return grab_object( obj );
}

static void mailslot_device_destroy( struct object *obj )
{
    struct mailslot_device *device = (struct mailslot_device*)obj;
    assert( obj->ops == &mailslot_device_ops );
    if (device->fd) release_object( device->fd );
    free( device->mailslots );
}

static enum server_fd_type mailslot_device_get_file_info( struct fd *fd, int *flags )
{
    *flags = 0;
    return FD_TYPE_DEVICE;
}

void create_mailslot_device( struct directory *root, const struct unicode_str *name )
{
    struct mailslot_device *dev;

    if ((dev = create_named_object_dir( root, name, 0, &mailslot_device_ops )) &&
        get_error() != STATUS_OBJECT_NAME_EXISTS)
    {
        dev->mailslots = NULL;
        if (!(dev->fd = alloc_pseudo_fd( &mailslot_device_fd_ops, &dev->obj )) ||
            !(dev->mailslots = create_namespace( 7 )))
        {
            release_object( dev );
            dev = NULL;
        }
    }
    if (dev) make_object_static( &dev->obj );
}

static struct mailslot *create_mailslot( struct directory *root,
                                         const struct unicode_str *name, unsigned int attr,
                                         int max_msgsize, int read_timeout )
{
    struct object *obj;
    struct unicode_str new_name;
    struct mailslot_device *dev;
    struct mailslot *mailslot;
    int fds[2];

    if (!name || !name->len) return alloc_object( &mailslot_ops );

    if (!(obj = find_object_dir( root, name, attr, &new_name )))
    {
        set_error( STATUS_OBJECT_NAME_INVALID );
        return NULL;
    }

    if (!new_name.len)
    {
        if (attr & OBJ_OPENIF && obj->ops == &mailslot_ops)
            /* it already exists - there can only be one mailslot to read from */
            set_error( STATUS_OBJECT_NAME_EXISTS );
        else if (attr & OBJ_OPENIF)
            set_error( STATUS_OBJECT_TYPE_MISMATCH );
        else
            set_error( STATUS_OBJECT_NAME_COLLISION );
        release_object( obj );
        return NULL;
    }

    if (obj->ops != &mailslot_device_ops)
    {
        set_error( STATUS_OBJECT_NAME_INVALID );
        release_object( obj );
        return NULL;
    }

    dev = (struct mailslot_device *)obj;
    mailslot = create_object( dev->mailslots, &mailslot_ops, &new_name, NULL );
    release_object( dev );

    if (!mailslot) return NULL;

    mailslot->fd = NULL;
    mailslot->write_fd = NULL;
    mailslot->max_msgsize = max_msgsize;
    mailslot->read_timeout = read_timeout;
    list_init( &mailslot->writers );

    if (!socketpair( PF_UNIX, SOCK_DGRAM, 0, fds ))
    {
        fcntl( fds[0], F_SETFL, O_NONBLOCK );
        fcntl( fds[1], F_SETFL, O_NONBLOCK );
        mailslot->fd = create_anonymous_fd( &mailslot_fd_ops,
                                fds[1], &mailslot->obj );
        mailslot->write_fd = create_anonymous_fd( &mail_writer_fd_ops,
                                fds[0], &mailslot->obj );
        if (mailslot->fd && mailslot->write_fd) return mailslot;
    }
    else file_set_error();

    release_object( mailslot );
    return NULL;
}

static void mail_writer_dump( struct object *obj, int verbose )
{
    fprintf( stderr, "Mailslot writer\n" );
}

static void mail_writer_destroy( struct object *obj)
{
    struct mail_writer *writer = (struct mail_writer *) obj;

    list_remove( &writer->entry );
    release_object( writer->mailslot );
}

static enum server_fd_type mail_writer_get_info( struct fd *fd, int *flags )
{
    *flags = 0;
    return FD_TYPE_MAILSLOT;
}

static struct fd *mail_writer_get_fd( struct object *obj )
{
    struct mail_writer *writer = (struct mail_writer *) obj;

    return (struct fd *)grab_object( writer->mailslot->write_fd );
}

static unsigned int mail_writer_map_access( struct object *obj, unsigned int access )
{
    /* mailslot writers can only get write access */
    if (access & GENERIC_WRITE)   access |= FILE_GENERIC_WRITE;
    if (access & GENERIC_ALL)     access |= FILE_GENERIC_WRITE;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static struct mailslot *get_mailslot_obj( struct process *process, obj_handle_t handle,
                                          unsigned int access )
{
    return (struct mailslot *)get_handle_obj( process, handle, access, &mailslot_ops );
}


/* create a mailslot */
DECL_HANDLER(create_mailslot)
{
    struct mailslot *mailslot;
    struct unicode_str name;
    struct directory *root = NULL;

    reply->handle = 0;
    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ((mailslot = create_mailslot( root, &name, req->attributes, req->max_msgsize,
                                     req->read_timeout )))
    {
        reply->handle = alloc_handle( current->process, mailslot, req->access, req->attributes );
        release_object( mailslot );
    }

    if (root) release_object( root );
}


/* set mailslot information */
DECL_HANDLER(set_mailslot_info)
{
    struct mailslot *mailslot = get_mailslot_obj( current->process, req->handle, 0 );

    if (mailslot)
    {
        if (req->flags & MAILSLOT_SET_READ_TIMEOUT)
            mailslot->read_timeout = req->read_timeout;
        reply->max_msgsize = mailslot->max_msgsize;
        reply->read_timeout = mailslot->read_timeout;
        release_object( mailslot );
    }
}
