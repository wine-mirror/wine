/*
 * Server-side file definitions
 *
 * Copyright (C) 2003 Alexandre Julliard
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

#ifndef __WINE_SERVER_FILE_H
#define __WINE_SERVER_FILE_H

#include "object.h"

struct fd;

/* operations valid on file descriptor objects */
struct fd_ops
{
    /* get the events we want to poll() for on this object */
    int  (*get_poll_events)(struct object *);
    /* a poll() event occured */
    void (*poll_event)(struct object *,int event);
    /* flush the object buffers */
    int  (*flush)(struct object *);
    /* get file information */
    int  (*get_file_info)(struct object *,struct get_file_info_reply *, int *flags);
    /* queue an async operation - see register_async handler in async.c*/
    void (*queue_async)(struct object *, void* ptr, unsigned int status, int type, int count);
};

extern void *alloc_fd_object( const struct object_ops *ops,
                              const struct fd_ops *fd_user_ops, int unix_fd );
extern int get_unix_fd( struct object *obj );
extern void set_unix_fd( struct object *obj, int unix_fd );
extern void close_fd( struct fd *fd );
extern void fd_poll_event( struct object *obj, int event );

extern int default_fd_add_queue( struct object *obj, struct wait_queue_entry *entry );
extern void default_fd_remove_queue( struct object *obj, struct wait_queue_entry *entry );
extern int default_fd_signaled( struct object *obj, struct thread *thread );
extern int no_flush( struct object *obj );
extern void no_queue_async(struct object *obj, void* ptr, unsigned int status, int type, int count);

#endif  /* __WINE_SERVER_FILE_H */
