/*
 * Server-side file management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
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
static int file_add_queue( struct object *obj, struct wait_queue_entry *entry );
static void file_remove_queue( struct object *obj, struct wait_queue_entry *entry );
static int file_signaled( struct object *obj, struct thread *thread );
static int file_get_read_fd( struct object *obj );
static int file_get_write_fd( struct object *obj );
static int file_flush( struct object *obj );
static void file_destroy( struct object *obj );

static const struct object_ops file_ops =
{
    file_dump,
    file_add_queue,
    file_remove_queue,
    file_signaled,
    no_satisfied,
    file_get_read_fd,
    file_get_write_fd,
    file_flush,
    file_destroy
};

static const struct select_ops select_ops =
{
    default_select_event,
    NULL   /* we never set a timeout on a file */
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

static int file_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
    if (!obj->head)  /* first on the queue */
    {
        if (!add_select_user( file->fd, READ_EVENT | WRITE_EVENT, &select_ops, file ))
        {
            SET_ERROR( ERROR_OUTOFMEMORY );
            return 0;
        }
    }
    add_queue( obj, entry );
    return 1;
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

static int file_get_read_fd( struct object *obj )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );

    if (!(file->event & READ_EVENT))  /* FIXME: should not be necessary */
    {
        SET_ERROR( ERROR_ACCESS_DENIED );
        return -1;
    }
    return dup( file->fd );
}

static int file_get_write_fd( struct object *obj )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );

    if (!(file->event & WRITE_EVENT))  /* FIXME: should not be necessary */
    {
        SET_ERROR( ERROR_ACCESS_DENIED );
        return -1;
    }
    return dup( file->fd );
}

static int file_flush( struct object *obj )
{
    int ret;
    struct file *file = (struct file *)grab_object(obj);
    assert( obj->ops == &file_ops );

    ret = (fsync( file->fd ) != -1);
    if (!ret) file_set_error();
    release_object( file );
    return ret;
}

static void file_destroy( struct object *obj )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
    close( file->fd );
    free( file );
}

/* set the last error depending on errno */
void file_set_error(void)
{
    switch (errno)
    {
    case EAGAIN:    SET_ERROR( ERROR_SHARING_VIOLATION ); break;
    case EBADF:     SET_ERROR( ERROR_INVALID_HANDLE ); break;
    case ENOSPC:    SET_ERROR( ERROR_HANDLE_DISK_FULL ); break;
    case EACCES:
    case EPERM:     SET_ERROR( ERROR_ACCESS_DENIED ); break;
    case EROFS:     SET_ERROR( ERROR_WRITE_PROTECT ); break;
    case EBUSY:     SET_ERROR( ERROR_LOCK_VIOLATION ); break;
    case ENOENT:    SET_ERROR( ERROR_FILE_NOT_FOUND ); break;
    case EISDIR:    SET_ERROR( ERROR_CANNOT_MAKE ); break;
    case ENFILE:
    case EMFILE:    SET_ERROR( ERROR_NO_MORE_FILES ); break;
    case EEXIST:    SET_ERROR( ERROR_FILE_EXISTS ); break;
    case EINVAL:    SET_ERROR( ERROR_INVALID_PARAMETER ); break;
    case ESPIPE:    SET_ERROR( ERROR_SEEK ); break;
    case ENOTEMPTY: SET_ERROR( ERROR_DIR_NOT_EMPTY ); break;
    default:        perror("file_set_error"); SET_ERROR( ERROR_UNKNOWN ); break;
    }
}

struct file *get_file_obj( struct process *process, int handle,
                           unsigned int access )
{
    return (struct file *)get_handle_obj( current->process, handle,
                                          access, &file_ops );
}

int file_get_mmap_fd( struct file *file )
{
    return dup( file->fd );
}

int set_file_pointer( int handle, int *low, int *high, int whence )
{
    struct file *file;
    int result;

    if (*high)
    {
        fprintf( stderr, "set_file_pointer: offset > 4Gb not supported yet\n" );
        SET_ERROR( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (!(file = get_file_obj( current->process, handle, 0 )))
        return 0;
    if ((result = lseek( file->fd, *low, whence )) == -1)
    {
        /* Check for seek before start of file */
        if ((errno == EINVAL) && (whence != SEEK_SET) && (*low < 0))
            SET_ERROR( ERROR_NEGATIVE_SEEK );
        else
            file_set_error();
        release_object( file );
        return 0;
    }
    *low = result;
    release_object( file );
    return 1;
}

int truncate_file( int handle )
{
    struct file *file;
    int result;

    if (!(file = get_file_obj( current->process, handle, GENERIC_WRITE )))
        return 0;
    if (((result = lseek( file->fd, 0, SEEK_CUR )) == -1) ||
        (ftruncate( file->fd, result ) == -1))
    {
        file_set_error();
        release_object( file );
        return 0;
    }
    release_object( file );
    return 1;
    
}

int get_file_info( int handle, struct get_file_info_reply *reply )
{
    struct file *file;
    struct stat st;

    if (!(file = get_file_obj( current->process, handle, 0 )))
        return 0;
    if (fstat( file->fd, &st ) == -1)
    {
        file_set_error();
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
