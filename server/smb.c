/*
 * Server-side smb network file management
 *
 * Copyright (C) 1998 Alexandre Julliard
 * Copyright (C) 2000, 2001, 2002 Mike McCormack
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
 *
 * FIXME: if you can't find something to fix,
 *          you're not looking hard enough
 */

#include "config.h"

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

static void smb_dump( struct object *obj, int verbose );
static struct fd *smb_get_fd( struct object *obj );
static void smb_destroy(struct object *obj);

static int smb_get_info( struct fd *fd, struct get_file_info_reply *reply, int *flags );
static int smb_get_poll_events( struct fd *fd );

struct smb
{
    struct object       obj;
    struct fd          *fd;
    unsigned int        tree_id;
    unsigned int        user_id;
    unsigned int        dialect;
    unsigned int        file_id;
    unsigned int        offset;
};

static const struct object_ops smb_ops =
{
    sizeof(struct smb),        /* size */
    smb_dump,                  /* dump */
    default_fd_add_queue,      /* add_queue */
    default_fd_remove_queue,   /* remove_queue */
    default_fd_signaled,       /* signaled */
    no_satisfied,              /* satisfied */
    smb_get_fd,                /* get_fd */
    smb_destroy                /* destroy */
};

static const struct fd_ops smb_fd_ops =
{
    smb_get_poll_events,       /* get_poll_events */
    default_poll_event,        /* poll_event */
    no_flush,                  /* flush */
    smb_get_info,              /* get_file_info */
    no_queue_async             /* queue_async */
};

static struct fd *smb_get_fd( struct object *obj )
{
    struct smb *smb = (struct smb *)obj;
    return (struct fd *)grab_object( smb->fd );
}

static void smb_destroy( struct object *obj)
{
    struct smb *smb = (struct smb *)obj;
    assert( obj->ops == &smb_ops );
    if (smb->fd) release_object( smb->fd );
}

static void smb_dump( struct object *obj, int verbose )
{
    struct smb *smb = (struct smb *)obj;
    assert( obj->ops == &smb_ops );
    fprintf( stderr, "Smb file fd=%p\n", smb->fd );
}

static struct smb *get_smb_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct smb *)get_handle_obj( process, handle, access, &smb_ops );
}

static int smb_get_poll_events( struct fd *fd )
{
    struct smb *smb = get_fd_user( fd );
    int events = 0;
    assert( smb->obj.ops == &smb_ops );

    events |= POLLIN;

    /* fprintf(stderr,"poll events are %04x\n",events); */

    return events;
}

static int smb_get_info( struct fd *fd, struct get_file_info_reply *reply, int *flags )
{
/*    struct smb *smb = get_fd_user( fd ); */
/*    assert( smb->obj.ops == &smb_ops ); */

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

    return FD_TYPE_SMB;
}

/* create a smb */
DECL_HANDLER(create_smb)
{
    struct smb *smb;
    int fd;

    reply->handle = 0;

    fd = thread_get_inflight_fd( current, req->fd );
    if (fd == -1)
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }

    if (!(smb = alloc_object( &smb_ops )))
    {
        close( fd );
        return;
    }
    smb->tree_id = req->tree_id;
    smb->user_id = req->user_id;
    smb->dialect = req->dialect;
    smb->file_id = req->file_id;
    smb->offset = 0;
    if ((smb->fd = create_anonymous_fd( &smb_fd_ops, fd, &smb->obj )))
    {
        reply->handle = alloc_handle( current->process, smb, GENERIC_READ, 0);
    }
    release_object( smb );
}

DECL_HANDLER(get_smb_info)
{
    struct smb *smb;

    if ((smb = get_smb_obj( current->process, req->handle, 0 )))
    {
        if(req->flags & SMBINFO_SET_OFFSET)
            smb->offset = req->offset;

        reply->tree_id = smb->tree_id;
        reply->user_id = smb->user_id;
        reply->dialect = smb->dialect;
        reply->file_id = smb->file_id;
        reply->offset  = smb->offset;

        release_object( smb );
    }
}
