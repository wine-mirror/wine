/*
 * Server-side file management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "winerror.h"
#include "winnt.h"
#include "server/thread.h"

struct file
{
    struct object  obj;             /* object header */
    int            fd;              /* Unix file descriptor */
    int            event;           /* possible events on this file */
};

static void file_dump( struct object *obj, int verbose );
static void file_add_queue( struct object *obj, struct wait_queue_entry *entry );
static void file_remove_queue( struct object *obj, struct wait_queue_entry *entry );
static int file_signaled( struct object *obj, struct thread *thread );
static int file_satisfied( struct object *obj, struct thread *thread );
static void file_destroy( struct object *obj );

static const struct object_ops file_ops =
{
    file_dump,
    file_add_queue,
    file_remove_queue,
    file_signaled,
    file_satisfied,
    file_destroy
};

static void file_event( int fd, int event, void *private );
static void file_timeout( int fd, void *private );

static const struct select_ops select_ops =
{
    file_event,
    file_timeout
};

struct object *create_file( int fd )
{
    struct file *file;
    int flags;

    if ((flags = fcntl( fd, F_GETFL )) == -1)
    {
        perror( "fcntl" );
        return NULL;
    }
    if (!(file = mem_alloc( sizeof(*file) ))) return NULL;
    init_object( &file->obj, &file_ops, NULL );
    file->fd = fd;
    switch(flags & 3)
    {
    case O_RDONLY:
        file->event = READ_EVENT;
        break;
    case O_WRONLY:
        file->event = WRITE_EVENT;
        break;
    case O_RDWR:
        file->event = READ_EVENT | WRITE_EVENT;
        break;
    }
    CLEAR_ERROR();
    return &file->obj;
}

static void file_dump( struct object *obj, int verbose )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
    printf( "File fd=%d\n", file->fd );
}

static void file_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
    if (!obj->head)  /* first on the queue */
        add_select_user( file->fd, READ_EVENT | WRITE_EVENT, &select_ops, file );
    add_queue( obj, entry );
}

static void file_remove_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct file *file = (struct file *)grab_object(obj);
    assert( obj->ops == &file_ops );

    remove_queue( obj, entry );
    if (!obj->head)  /* last on the queue is gone */
        remove_select_user( file->fd );
    release_object( obj );
}

static int file_signaled( struct object *obj, struct thread *thread )
{
    fd_set read_fds, write_fds;
    struct timeval tv = { 0, 0 };

    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );

    FD_ZERO( &read_fds );
    FD_ZERO( &write_fds );
    if (file->event & READ_EVENT) FD_SET( file->fd, &read_fds );
    if (file->event & WRITE_EVENT) FD_SET( file->fd, &write_fds );
    return select( file->fd + 1, &read_fds, &write_fds, NULL, &tv ) > 0;
}

static int file_satisfied( struct object *obj, struct thread *thread )
{
    /* Nothing to do */
    return 0;  /* Not abandoned */
}

static void file_destroy( struct object *obj )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
    close( file->fd );
    free( file );
}

static void file_event( int fd, int event, void *private )
{
    struct file *file = (struct file *)private;
    assert( file );

    wake_up( &file->obj, 0 );
}

static void file_timeout( int fd, void *private )
{
    /* we never set a timeout on a file */
    assert( 0 );
}

int file_get_unix_handle( int handle, unsigned int access )
{
    struct file *file;
    int unix_handle;

    if (!(file = (struct file *)get_handle_obj( current->process, handle,
                                                access, &file_ops )))
        return -1;
    unix_handle = dup( file->fd );
    release_object( file );
    return unix_handle;
}

int get_file_info( int handle, struct get_file_info_reply *reply )
{
    struct file *file;
    struct stat st;

    if (!(file = (struct file *)get_handle_obj( current->process, handle,
                                                0, &file_ops )))
        return 0;
    if (fstat( file->fd, &st ) == -1)
    {
        /* file_set_error(); */
        release_object( file );
        return 0;
    }
    if (S_ISDIR(st.st_mode)) reply->attr = FILE_ATTRIBUTE_DIRECTORY;
    else reply->attr = FILE_ATTRIBUTE_ARCHIVE;
    if (!(st.st_mode & S_IWUSR)) reply->attr |= FILE_ATTRIBUTE_READONLY;
    reply->access_time = st.st_atime;
    reply->write_time  = st.st_mtime;
    reply->size_high   = 0;
    reply->size_low    = S_ISDIR(st.st_mode) ? 0 : st.st_size;
    reply->links       = st.st_nlink;
    reply->index_high  = st.st_dev;
    reply->index_low   = st.st_ino;
    reply->serial      = 0; /* FIXME */
    
    release_object( file );
    return 1;
}
