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
    int  (*get_poll_events)(struct fd *);
    /* a poll() event occured */
    void (*poll_event)(struct fd *,int event);
    /* flush the object buffers */
    int  (*flush)(struct fd *);
    /* get file information */
    int  (*get_file_info)(struct fd *,struct get_file_info_reply *, int *flags);
    /* queue an async operation - see register_async handler in async.c*/
    void (*queue_async)(struct fd *, void* ptr, unsigned int status, int type, int count);
};

/* file descriptor functions */

extern void *alloc_fd_object( const struct object_ops *ops,
                              const struct fd_ops *fd_user_ops, int unix_fd );
extern void *get_fd_user( struct fd *fd );
extern int get_unix_fd( struct fd *fd );
extern void set_unix_fd( struct object *obj, int unix_fd );
extern void fd_poll_event( struct object *obj, int event );
extern int check_fd_events( struct fd *fd, int events );

extern int default_fd_add_queue( struct object *obj, struct wait_queue_entry *entry );
extern void default_fd_remove_queue( struct object *obj, struct wait_queue_entry *entry );
extern int default_fd_signaled( struct object *obj, struct thread *thread );
extern void default_poll_event( struct fd *fd, int event );
extern int no_flush( struct fd *fd );
extern int no_get_file_info( struct fd *fd, struct get_file_info_reply *info, int *flags );
extern void no_queue_async( struct fd *fd, void* ptr, unsigned int status, int type, int count );

inline static struct fd *get_obj_fd( struct object *obj ) { return obj->ops->get_fd( obj ); }

/* file functions */

extern struct file *get_file_obj( struct process *process, obj_handle_t handle,
                                  unsigned int access );
extern int get_file_unix_fd( struct file *file );
extern int is_same_file( struct file *file1, struct file *file2 );
extern int get_file_drive_type( struct file *file );
extern int grow_file( struct file *file, int size_high, int size_low );
extern int create_anonymous_file(void);
extern struct file *create_temp_file( int access );
extern void file_set_error(void);

#endif  /* __WINE_SERVER_FILE_H */
