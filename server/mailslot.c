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

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
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
    int                 write_fd;
    unsigned int        max_msgsize;
    timeout_t           read_timeout;
    struct list         writers;
};

/* mailslot functions */
static void mailslot_dump( struct object*, int );
static struct fd *mailslot_get_fd( struct object * );
static unsigned int mailslot_map_access( struct object *obj, unsigned int access );
static int mailslot_link_name( struct object *obj, struct object_name *name, struct object *parent );
static struct object *mailslot_open_file( struct object *obj, unsigned int access,
                                          unsigned int sharing, unsigned int options );
static void mailslot_destroy( struct object * );

static const struct object_ops mailslot_ops =
{
    sizeof(struct mailslot),   /* size */
    &file_type,                /* type */
    mailslot_dump,             /* dump */
    add_queue,                 /* add_queue */
    remove_queue,              /* remove_queue */
    default_fd_signaled,       /* signaled */
    no_satisfied,              /* satisfied */
    no_signal,                 /* signal */
    mailslot_get_fd,           /* get_fd */
    mailslot_map_access,       /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    default_get_full_name,     /* get_full_name */
    no_lookup_name,            /* lookup_name */
    mailslot_link_name,        /* link_name */
    default_unlink_name,       /* unlink_name */
    mailslot_open_file,        /* open_file */
    no_kernel_obj_list,        /* get_kernel_obj_list */
    no_close_handle,           /* close_handle */
    mailslot_destroy           /* destroy */
};

static enum server_fd_type mailslot_get_fd_type( struct fd *fd );
static void mailslot_queue_async( struct fd *fd, struct async *async, int type, int count );

static const struct fd_ops mailslot_fd_ops =
{
    default_fd_get_poll_events, /* get_poll_events */
    default_poll_event,         /* poll_event */
    mailslot_get_fd_type,       /* get_fd_type */
    no_fd_read,                 /* read */
    no_fd_write,                /* write */
    no_fd_flush,                /* flush */
    default_fd_get_file_info,   /* get_file_info */
    no_fd_get_volume_info,      /* get_volume_info */
    default_fd_ioctl,           /* ioctl */
    default_fd_cancel_async,    /* cancel_async */
    mailslot_queue_async,       /* queue_async */
    default_fd_reselect_async   /* reselect_async */
};


struct mail_writer
{
    struct object         obj;
    struct fd            *fd;
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
    &file_type,                 /* type */
    mail_writer_dump,           /* dump */
    no_add_queue,               /* add_queue */
    NULL,                       /* remove_queue */
    NULL,                       /* signaled */
    NULL,                       /* satisfied */
    no_signal,                  /* signal */
    mail_writer_get_fd,         /* get_fd */
    mail_writer_map_access,     /* map_access */
    default_get_sd,             /* get_sd */
    default_set_sd,             /* set_sd */
    no_get_full_name,           /* get_full_name */
    no_lookup_name,             /* lookup_name */
    no_link_name,               /* link_name */
    NULL,                       /* unlink_name */
    no_open_file,               /* open_file */
    no_kernel_obj_list,         /* get_kernel_obj_list */
    no_close_handle,            /* close_handle */
    mail_writer_destroy         /* destroy */
};

static enum server_fd_type mail_writer_get_fd_type( struct fd *fd );

static const struct fd_ops mail_writer_fd_ops =
{
    default_fd_get_poll_events,  /* get_poll_events */
    default_poll_event,          /* poll_event */
    mail_writer_get_fd_type,     /* get_fd_type */
    no_fd_read,                  /* read */
    no_fd_write,                 /* write */
    no_fd_flush,                 /* flush */
    default_fd_get_file_info,    /* get_file_info */
    no_fd_get_volume_info,       /* get_volume_info */
    default_fd_ioctl,            /* ioctl */
    default_fd_cancel_async,     /* cancel_async */
    default_fd_queue_async,      /* queue_async */
    default_fd_reselect_async    /* reselect_async */
};


struct mailslot_device
{
    struct object       obj;         /* object header */
    struct namespace   *mailslots;   /* mailslot namespace */
};

struct mailslot_device_file
{
    struct object           obj;    /* object header */
    struct fd              *fd;     /* pseudo-fd for ioctls */
    struct mailslot_device *device; /* mailslot device */
};

static void mailslot_device_dump( struct object *obj, int verbose );
static struct object *mailslot_device_lookup_name( struct object *obj, struct unicode_str *name,
                                                   unsigned int attr, struct object *root );
static struct object *mailslot_device_open_file( struct object *obj, unsigned int access,
                                                 unsigned int sharing, unsigned int options );
static void mailslot_device_destroy( struct object *obj );

static const struct object_ops mailslot_device_ops =
{
    sizeof(struct mailslot_device), /* size */
    &device_type,                   /* type */
    mailslot_device_dump,           /* dump */
    no_add_queue,                   /* add_queue */
    NULL,                           /* remove_queue */
    NULL,                           /* signaled */
    no_satisfied,                   /* satisfied */
    no_signal,                      /* signal */
    no_get_fd,                      /* get_fd */
    default_map_access,             /* map_access */
    default_get_sd,                 /* get_sd */
    default_set_sd,                 /* set_sd */
    default_get_full_name,          /* get_full_name */
    mailslot_device_lookup_name,    /* lookup_name */
    directory_link_name,            /* link_name */
    default_unlink_name,            /* unlink_name */
    mailslot_device_open_file,      /* open_file */
    no_kernel_obj_list,             /* get_kernel_obj_list */
    no_close_handle,                /* close_handle */
    mailslot_device_destroy         /* destroy */
};

static void mailslot_device_file_dump( struct object *obj, int verbose );
static struct fd *mailslot_device_file_get_fd( struct object *obj );
static WCHAR *mailslot_device_file_get_full_name( struct object *obj, data_size_t *len );
static void mailslot_device_file_destroy( struct object *obj );
static enum server_fd_type mailslot_device_file_get_fd_type( struct fd *fd );

static const struct object_ops mailslot_device_file_ops =
{
    sizeof(struct mailslot_device_file),    /* size */
    &file_type,                             /* type */
    mailslot_device_file_dump,              /* dump */
    add_queue,                              /* add_queue */
    remove_queue,                           /* remove_queue */
    default_fd_signaled,                    /* signaled */
    no_satisfied,                           /* satisfied */
    no_signal,                              /* signal */
    mailslot_device_file_get_fd,            /* get_fd */
    default_map_access,                     /* map_access */
    default_get_sd,                         /* get_sd */
    default_set_sd,                         /* set_sd */
    mailslot_device_file_get_full_name,     /* get_full_name */
    no_lookup_name,                         /* lookup_name */
    no_link_name,                           /* link_name */
    NULL,                                   /* unlink_name */
    no_open_file,                           /* open_file */
    no_kernel_obj_list,                     /* get_kernel_obj_list */
    no_close_handle,                        /* close_handle */
    mailslot_device_file_destroy            /* destroy */
};

static const struct fd_ops mailslot_device_fd_ops =
{
    default_fd_get_poll_events,         /* get_poll_events */
    default_poll_event,                 /* poll_event */
    mailslot_device_file_get_fd_type,   /* get_fd_type */
    no_fd_read,                         /* read */
    no_fd_write,                        /* write */
    no_fd_flush,                        /* flush */
    default_fd_get_file_info,           /* get_file_info */
    no_fd_get_volume_info,              /* get_volume_info */
    default_fd_ioctl,                   /* ioctl */
    default_fd_cancel_async,            /* cancel_async */
    default_fd_queue_async,             /* queue_async */
    default_fd_reselect_async           /* reselect_async */
};

static void mailslot_destroy( struct object *obj)
{
    struct mailslot *mailslot = (struct mailslot *) obj;

    assert( mailslot->fd );

    if (mailslot->write_fd != -1)
    {
        shutdown( mailslot->write_fd, SHUT_RDWR );
        close( mailslot->write_fd );
    }
    release_object( mailslot->fd );
}

static void mailslot_dump( struct object *obj, int verbose )
{
    struct mailslot *mailslot = (struct mailslot *) obj;

    assert( obj->ops == &mailslot_ops );
    fprintf( stderr, "Mailslot max_msgsize=%d read_timeout=%s\n",
             mailslot->max_msgsize, get_timeout_str(mailslot->read_timeout) );
}

static enum server_fd_type mailslot_get_fd_type( struct fd *fd )
{
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
    return default_map_access( obj, access ) & FILE_GENERIC_READ;
}

static int mailslot_link_name( struct object *obj, struct object_name *name, struct object *parent )
{
    struct mailslot_device *dev = (struct mailslot_device *)parent;

    if (parent->ops != &mailslot_device_ops)
    {
        set_error( STATUS_OBJECT_NAME_INVALID );
        return 0;
    }
    namespace_add( dev->mailslots, name );
    name->parent = grab_object( parent );
    return 1;
}

static struct object *mailslot_open_file( struct object *obj, unsigned int access,
                                          unsigned int sharing, unsigned int options )
{
    struct mailslot *mailslot = (struct mailslot *)obj;
    struct mail_writer *writer;
    int unix_fd;

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

    if ((unix_fd = dup( mailslot->write_fd )) == -1)
    {
        file_set_error();
        return NULL;
    }
    if (!(writer = alloc_object( &mail_writer_ops )))
    {
        close( unix_fd );
        return NULL;
    }
    grab_object( mailslot );
    writer->mailslot = mailslot;
    writer->access   = mail_writer_map_access( &writer->obj, access );
    writer->sharing  = sharing;
    list_add_head( &mailslot->writers, &writer->entry );

    if (!(writer->fd = create_anonymous_fd( &mail_writer_fd_ops, unix_fd, &writer->obj, options )))
    {
        release_object( writer );
        return NULL;
    }
    allow_fd_caching( writer->fd );
    return &writer->obj;
}

static void mailslot_queue_async( struct fd *fd, struct async *async, int type, int count )
{
    struct mailslot *mailslot = get_fd_user( fd );

    assert(mailslot->obj.ops == &mailslot_ops);

    fd_queue_async( fd, async, type );
    async_set_timeout( async, mailslot->read_timeout ? mailslot->read_timeout : -1,
                       STATUS_IO_TIMEOUT );
    set_error( STATUS_PENDING );
}

static void mailslot_device_dump( struct object *obj, int verbose )
{
    fputs( "Mailslot device\n", stderr );
}

static struct object *mailslot_device_lookup_name( struct object *obj, struct unicode_str *name,
                                                   unsigned int attr, struct object *root )
{
    struct mailslot_device *device = (struct mailslot_device*)obj;
    struct object *found;

    assert( obj->ops == &mailslot_device_ops );

    if (!name) return NULL;  /* open the device itself */

    if ((found = find_object( device->mailslots, name, attr | OBJ_CASE_INSENSITIVE )))
        name->len = 0;

    return found;
}

static struct object *mailslot_device_open_file( struct object *obj, unsigned int access,
                                                 unsigned int sharing, unsigned int options )
{
    struct mailslot_device_file *file;

    if (!(file = alloc_object( &mailslot_device_file_ops ))) return NULL;
    file->device = (struct mailslot_device *)grab_object( obj );
    if (!(file->fd = alloc_pseudo_fd( &mailslot_device_fd_ops, obj, options )))
    {
        release_object( file );
        return NULL;
    }
    allow_fd_caching( file->fd );
    return &file->obj;
}

static void mailslot_device_destroy( struct object *obj )
{
    struct mailslot_device *device = (struct mailslot_device*)obj;
    assert( obj->ops == &mailslot_device_ops );
    free( device->mailslots );
}

struct object *create_mailslot_device( struct object *root, const struct unicode_str *name,
                                       unsigned int attr, const struct security_descriptor *sd )
{
    struct mailslot_device *dev;

    if ((dev = create_named_object( root, &mailslot_device_ops, name, attr, sd )) &&
        get_error() != STATUS_OBJECT_NAME_EXISTS)
    {
        dev->mailslots = NULL;
        if (!(dev->mailslots = create_namespace( 7 )))
        {
            release_object( dev );
            dev = NULL;
        }
    }
    return &dev->obj;
}

static void mailslot_device_file_dump( struct object *obj, int verbose )
{
    struct mailslot_device_file *file = (struct mailslot_device_file *)obj;

    fprintf( stderr, "File on mailslot device %p\n", file->device );
}

static struct fd *mailslot_device_file_get_fd( struct object *obj )
{
    struct mailslot_device_file *file = (struct mailslot_device_file *)obj;
    return (struct fd *)grab_object( file->fd );
}

static WCHAR *mailslot_device_file_get_full_name( struct object *obj, data_size_t *len )
{
    struct mailslot_device_file *file = (struct mailslot_device_file *)obj;
    return file->device->obj.ops->get_full_name( &file->device->obj, len );
}

static void mailslot_device_file_destroy( struct object *obj )
{
    struct mailslot_device_file *file = (struct mailslot_device_file*)obj;
    assert( obj->ops == &mailslot_device_file_ops );
    if (file->fd) release_object( file->fd );
    release_object( file->device );
}

static enum server_fd_type mailslot_device_file_get_fd_type( struct fd *fd )
{
    return FD_TYPE_DEVICE;
}

static struct mailslot *create_mailslot( struct object *root,
                                         const struct unicode_str *name, unsigned int attr,
                                         int max_msgsize, timeout_t read_timeout,
                                         const struct security_descriptor *sd )
{
    struct mailslot *mailslot;
    int fds[2];

    if (!(mailslot = create_named_object( root, &mailslot_ops, name, attr, sd ))) return NULL;

    mailslot->fd = NULL;
    mailslot->write_fd = -1;
    mailslot->max_msgsize = max_msgsize;
    mailslot->read_timeout = read_timeout;
    list_init( &mailslot->writers );

    if (!socketpair( PF_UNIX, SOCK_DGRAM, 0, fds ))
    {
        fcntl( fds[0], F_SETFL, O_NONBLOCK );
        fcntl( fds[1], F_SETFL, O_NONBLOCK );
        shutdown( fds[0], SHUT_RD );
        mailslot->write_fd = fds[0];
        if ((mailslot->fd = create_anonymous_fd( &mailslot_fd_ops, fds[1], &mailslot->obj,
                                                 FILE_SYNCHRONOUS_IO_NONALERT )))
        {
            allow_fd_caching( mailslot->fd );
            return mailslot;
        }
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

    if (writer->fd) release_object( writer->fd );
    list_remove( &writer->entry );
    release_object( writer->mailslot );
}

static enum server_fd_type mail_writer_get_fd_type( struct fd *fd )
{
    return FD_TYPE_MAILSLOT;
}

static struct fd *mail_writer_get_fd( struct object *obj )
{
    struct mail_writer *writer = (struct mail_writer *) obj;
    return (struct fd *)grab_object( writer->fd );
}

static unsigned int mail_writer_map_access( struct object *obj, unsigned int access )
{
    /* mailslot writers can only get write access */
    return default_map_access( obj, access ) & FILE_GENERIC_WRITE;
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
    struct object *root;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, &root );

    if (!objattr) return;

    if (!name.len)  /* mailslots need a root directory even without a name */
    {
        if (!objattr->rootdir)
        {
            set_error( STATUS_OBJECT_PATH_SYNTAX_BAD );
            return;
        }
        if (!(root = get_directory_obj( current->process, objattr->rootdir ))) return;
    }

    if ((mailslot = create_mailslot( root, &name, objattr->attributes, req->max_msgsize,
                                     req->read_timeout, sd )))
    {
        reply->handle = alloc_handle( current->process, mailslot, req->access, objattr->attributes );
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
