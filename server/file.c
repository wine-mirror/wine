/*
 * Server-side file management
 *
 * Copyright (C) 1998 Alexandre Julliard
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
#include <stdarg.h>
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
#ifdef HAVE_UTIME_H
#include <utime.h>
#endif

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"

#include "file.h"
#include "handle.h"
#include "thread.h"
#include "request.h"
#include "async.h"

struct file
{
    struct object       obj;        /* object header */
    struct fd          *fd;         /* file descriptor for this file */
    unsigned int        access;     /* file access (GENERIC_READ/WRITE) */
    unsigned int        options;    /* file options (FILE_DELETE_ON_CLOSE, FILE_SYNCHRONOUS...) */
    int                 removable;  /* is file on removable media? */
    struct async_queue  read_q;
    struct async_queue  write_q;
};

static void file_dump( struct object *obj, int verbose );
static struct fd *file_get_fd( struct object *obj );
static void file_destroy( struct object *obj );

static int file_get_poll_events( struct fd *fd );
static void file_poll_event( struct fd *fd, int event );
static int file_flush( struct fd *fd, struct event **event );
static int file_get_info( struct fd *fd, struct get_file_info_reply *reply, int *flags );
static void file_queue_async( struct fd *fd, void *ptr, unsigned int status, int type, int count );

static const struct object_ops file_ops =
{
    sizeof(struct file),          /* size */
    file_dump,                    /* dump */
    default_fd_add_queue,         /* add_queue */
    default_fd_remove_queue,      /* remove_queue */
    default_fd_signaled,          /* signaled */
    no_satisfied,                 /* satisfied */
    file_get_fd,                  /* get_fd */
    file_destroy                  /* destroy */
};

static const struct fd_ops file_fd_ops =
{
    file_get_poll_events,         /* get_poll_events */
    file_poll_event,              /* poll_event */
    file_flush,                   /* flush */
    file_get_info,                /* get_file_info */
    file_queue_async              /* queue_async */
};

static inline int is_overlapped( const struct file *file )
{
    return !(file->options & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT));
}

/* create a file from a file descriptor */
/* if the function fails the fd is closed */
static struct file *create_file_for_fd( int fd, unsigned int access, unsigned int sharing )
{
    struct file *file;

    if ((file = alloc_object( &file_ops )))
    {
        file->access     = access;
        file->options    = FILE_SYNCHRONOUS_IO_NONALERT;
        file->removable  = 0;
        if (!(file->fd = create_anonymous_fd( &file_fd_ops, fd, &file->obj )))
        {
            release_object( file );
            return NULL;
        }
    }
    return file;
}


static struct object *create_file( const char *nameptr, size_t len, unsigned int access,
                                   unsigned int sharing, int create, unsigned int options,
                                   unsigned int attrs, int removable )
{
    struct file *file;
    int flags;
    char *name;
    mode_t mode;

    if (!(name = mem_alloc( len + 1 ))) return NULL;
    memcpy( name, nameptr, len );
    name[len] = 0;

    switch(create)
    {
    case FILE_CREATE:       flags = O_CREAT | O_EXCL; break;
    case FILE_OVERWRITE_IF: /* FIXME: the difference is whether we trash existing attr or not */
    case FILE_SUPERSEDE:    flags = O_CREAT | O_TRUNC; break;
    case FILE_OPEN:         flags = 0; break;
    case FILE_OPEN_IF:      flags = O_CREAT; break;
    case FILE_OVERWRITE:    flags = O_TRUNC; break;
    default:                set_error( STATUS_INVALID_PARAMETER ); goto error;
    }
    switch(access & (GENERIC_READ | GENERIC_WRITE))
    {
    case 0: break;
    case GENERIC_READ:  flags |= O_RDONLY; break;
    case GENERIC_WRITE: flags |= O_WRONLY; break;
    case GENERIC_READ|GENERIC_WRITE: flags |= O_RDWR; break;
    }
    mode = (attrs & FILE_ATTRIBUTE_READONLY) ? 0444 : 0666;

    if (len >= 4 &&
        (!strcasecmp( name + len - 4, ".exe" ) || !strcasecmp( name + len - 4, ".com" )))
        mode |= 0111;

    if (!(file = alloc_object( &file_ops ))) goto error;

    file->access     = access;
    file->options    = options;
    file->removable  = removable;
    if (is_overlapped( file ))
    {
        init_async_queue (&file->read_q);
        init_async_queue (&file->write_q);
    }

    /* FIXME: should set error to STATUS_OBJECT_NAME_COLLISION if file existed before */
    if (!(file->fd = alloc_fd( &file_fd_ops, &file->obj )) ||
        !(file->fd = open_fd( file->fd, name, flags | O_NONBLOCK | O_LARGEFILE,
                              &mode, access, sharing,
                              (options & FILE_DELETE_ON_CLOSE) ? name : "" )))
    {
        free( name );
        release_object( file );
        return NULL;
    }
    free( name );

    /* refuse to open a directory */
    if (S_ISDIR(mode) && !(options & FILE_OPEN_FOR_BACKUP_INTENT))
    {
        set_error( STATUS_ACCESS_DENIED );
        release_object( file );
        return NULL;
    }
    /* check for serial port */
    if (S_ISCHR(mode) && is_serial_fd( file->fd ))
    {
        struct object *obj = create_serial( file->fd, file->options );
        release_object( file );
        return obj;
    }

    return &file->obj;

 error:
    free( name );
    return NULL;
}

/* check if two file objects point to the same file */
int is_same_file( struct file *file1, struct file *file2 )
{
    return is_same_file_fd( file1->fd, file2->fd );
}

/* check if the file is on removable media */
int is_file_removable( struct file *file )
{
    return file->removable;
}

/* create a temp file for anonymous mappings */
struct file *create_temp_file( int access )
{
    char tmpfn[16];
    int fd;

    sprintf( tmpfn, "anonmap.XXXXXX" );  /* create it in the server directory */
    fd = mkstemps( tmpfn, 0 );
    if (fd == -1)
    {
        file_set_error();
        return NULL;
    }
    unlink( tmpfn );
    return create_file_for_fd( fd, access, 0 );
}

static void file_dump( struct object *obj, int verbose )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
    fprintf( stderr, "File fd=%p options=%08x\n", file->fd, file->options );
}

static int file_get_poll_events( struct fd *fd )
{
    struct file *file = get_fd_user( fd );
    int events = 0;
    assert( file->obj.ops == &file_ops );
    if (file->access & GENERIC_READ) events |= POLLIN;
    if (file->access & GENERIC_WRITE) events |= POLLOUT;
    return events;
}

static void file_poll_event( struct fd *fd, int event )
{
    struct file *file = get_fd_user( fd );
    assert( file->obj.ops == &file_ops );
    if (is_overlapped( file ))
    {
        if( IS_READY(file->read_q) && (POLLIN & event) )
        {
            async_notify(file->read_q.head, STATUS_ALERTED);
            return;
        }
        if( IS_READY(file->write_q) && (POLLOUT & event) )
        {
            async_notify(file->write_q.head, STATUS_ALERTED);
            return;
        }
    }
    default_poll_event( fd, event );
}


static int file_flush( struct fd *fd, struct event **event )
{
    int ret = (fsync( get_unix_fd(fd) ) != -1);
    if (!ret) file_set_error();
    return ret;
}

static int file_get_info( struct fd *fd, struct get_file_info_reply *reply, int *flags )
{
    struct stat st;
    struct file *file = get_fd_user( fd );
    int unix_fd = get_unix_fd( fd );

    if (reply)
    {
        if (fstat( unix_fd, &st ) == -1)
        {
            file_set_error();
            return FD_TYPE_INVALID;
        }
        if (S_ISCHR(st.st_mode) || S_ISFIFO(st.st_mode) ||
            S_ISSOCK(st.st_mode) || isatty(unix_fd)) reply->type = FILE_TYPE_CHAR;
        else reply->type = FILE_TYPE_DISK;
        if (S_ISDIR(st.st_mode)) reply->attr = FILE_ATTRIBUTE_DIRECTORY;
        else reply->attr = FILE_ATTRIBUTE_ARCHIVE;
        if (!(st.st_mode & S_IWUSR)) reply->attr |= FILE_ATTRIBUTE_READONLY;
        reply->access_time = st.st_atime;
        reply->write_time  = st.st_mtime;
        reply->change_time = st.st_ctime;
        if (S_ISDIR(st.st_mode))
        {
            reply->size_high  = 0;
            reply->size_low   = 0;
            reply->alloc_high = 0;
            reply->alloc_low  = 0;
        }
        else
        {
            file_pos_t  alloc;
            reply->size_high  = st.st_size >> 32;
            reply->size_low   = st.st_size & 0xffffffff;
            alloc = (file_pos_t)st.st_blksize * st.st_blocks;
            reply->alloc_high = alloc >> 32;
            reply->alloc_low  = alloc & 0xffffffff;
        }
        reply->links       = st.st_nlink;
        reply->index_high  = st.st_dev;
        reply->index_low   = st.st_ino;
        reply->serial      = 0; /* FIXME */
    }
    *flags = 0;
    if (is_overlapped( file )) *flags |= FD_FLAG_OVERLAPPED;
    return FD_TYPE_DEFAULT;
}

static void file_queue_async(struct fd *fd, void *ptr, unsigned int status, int type, int count)
{
    struct file *file = get_fd_user( fd );
    struct async *async;
    struct async_queue *q;

    assert( file->obj.ops == &file_ops );

    if (!is_overlapped( file ))
    {
        set_error ( STATUS_INVALID_HANDLE );
        return;
    }

    switch(type)
    {
    case ASYNC_TYPE_READ:
        q = &file->read_q;
        break;
    case ASYNC_TYPE_WRITE:
        q = &file->write_q;
        break;
    default:
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    async = find_async ( q, current, ptr );

    if ( status == STATUS_PENDING )
    {
        int events;

        if ( !async )
            async = create_async ( &file->obj, current, ptr );
        if ( !async )
            return;

        async->status = STATUS_PENDING;
        if ( !async->q )
            async_insert( q, async );

        /* Check if the new pending request can be served immediately */
        events = check_fd_events( fd, file_get_poll_events( fd ) );
        if (events) file_poll_event ( fd, events );
    }
    else if ( async ) destroy_async ( async );
    else set_error ( STATUS_INVALID_PARAMETER );

    set_fd_events( fd, file_get_poll_events( fd ));
}

static struct fd *file_get_fd( struct object *obj )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
    return (struct fd *)grab_object( file->fd );
}

static void file_destroy( struct object *obj )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );

    if (is_overlapped( file ))
    {
        destroy_async_queue (&file->read_q);
        destroy_async_queue (&file->write_q);
    }
    if (file->fd) release_object( file->fd );
}

/* set the last error depending on errno */
void file_set_error(void)
{
    switch (errno)
    {
    case EAGAIN:    set_error( STATUS_SHARING_VIOLATION ); break;
    case EBADF:     set_error( STATUS_INVALID_HANDLE ); break;
    case ENOSPC:    set_error( STATUS_DISK_FULL ); break;
    case EACCES:
    case ESRCH:
    case EPERM:     set_error( STATUS_ACCESS_DENIED ); break;
    case EROFS:     set_error( STATUS_MEDIA_WRITE_PROTECTED ); break;
    case EBUSY:     set_error( STATUS_FILE_LOCK_CONFLICT ); break;
    case ENOENT:    set_error( STATUS_NO_SUCH_FILE ); break;
    case EISDIR:    set_win32_error( ERROR_CANNOT_MAKE ); break;
    case ENFILE:
    case EMFILE:    set_error( STATUS_NO_MORE_FILES ); break;
    case EEXIST:    set_error( STATUS_OBJECT_NAME_COLLISION ); break;
    case EINVAL:    set_error( STATUS_INVALID_PARAMETER ); break;
    case ESPIPE:    set_win32_error( ERROR_SEEK ); break;
    case ENOTEMPTY: set_error( STATUS_DIRECTORY_NOT_EMPTY ); break;
    case EIO:       set_error( STATUS_ACCESS_VIOLATION ); break;
    case ENOTDIR:   set_error( STATUS_NOT_A_DIRECTORY ); break;
#ifdef EOVERFLOW
    case EOVERFLOW: set_error( STATUS_INVALID_PARAMETER ); break;
#endif
    default:        perror("file_set_error"); set_win32_error( ERROR_UNKNOWN ); break;
    }
}

struct file *get_file_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct file *)get_handle_obj( process, handle, access, &file_ops );
}

int get_file_unix_fd( struct file *file )
{
    return get_unix_fd( file->fd );
}

static int set_file_pointer( obj_handle_t handle, unsigned int *low, int *high, int whence )
{
    struct file *file;
    off_t result,xto;

    xto = *low+((off_t)*high<<32);
    if (!(file = get_file_obj( current->process, handle, 0 )))
        return 0;
    if ((result = lseek( get_file_unix_fd(file), xto, whence))==-1)
    {
        /* Check for seek before start of file */

        /* also check EPERM due to SuSE7 2.2.16 lseek() EPERM kernel bug */
        if (((errno == EINVAL) || (errno == EPERM))
            && (whence != SEEK_SET) && (*high < 0))
            set_win32_error( ERROR_NEGATIVE_SEEK );
        else
            file_set_error();
        release_object( file );
        return 0;
    }
    *low  = result & 0xffffffff;
    *high = result >> 32;
    release_object( file );
    return 1;
}

/* extend a file beyond the current end of file */
static int extend_file( struct file *file, off_t size )
{
    static const char zero;
    int unix_fd = get_file_unix_fd( file );

    /* extend the file one byte beyond the requested size and then truncate it */
    /* this should work around ftruncate implementations that can't extend files */
    if ((lseek( unix_fd, size, SEEK_SET ) != -1) &&
        (write( unix_fd, &zero, 1 ) != -1))
    {
        ftruncate( unix_fd, size );
        return 1;
    }
    file_set_error();
    return 0;
}

/* truncate file at current position */
static int truncate_file( struct file *file )
{
    int ret = 0;
    int unix_fd = get_file_unix_fd( file );
    off_t pos = lseek( unix_fd, 0, SEEK_CUR );
    off_t eof = lseek( unix_fd, 0, SEEK_END );

    if (eof < pos) ret = extend_file( file, pos );
    else
    {
        if (ftruncate( unix_fd, pos ) != -1) ret = 1;
        else file_set_error();
    }
    lseek( unix_fd, pos, SEEK_SET );  /* restore file pos */
    return ret;
}

/* try to grow the file to the specified size */
int grow_file( struct file *file, int size_high, int size_low )
{
    int ret = 0;
    struct stat st;
    int unix_fd = get_file_unix_fd( file );
    off_t old_pos, size = size_low + (((off_t)size_high)<<32);

    if (fstat( unix_fd, &st ) == -1)
    {
        file_set_error();
        return 0;
    }
    if (st.st_size >= size) return 1;  /* already large enough */
    old_pos = lseek( unix_fd, 0, SEEK_CUR );  /* save old pos */
    ret = extend_file( file, size );
    lseek( unix_fd, old_pos, SEEK_SET );  /* restore file pos */
    return ret;
}

/* create a file */
DECL_HANDLER(create_file)
{
    struct object *file;

    reply->handle = 0;
    if ((file = create_file( get_req_data(), get_req_data_size(), req->access,
                             req->sharing, req->create, req->options, req->attrs, req->removable )))
    {
        reply->handle = alloc_handle( current->process, file, req->access, req->inherit );
        release_object( file );
    }
}

/* allocate a file handle for a Unix fd */
DECL_HANDLER(alloc_file_handle)
{
    struct file *file;
    int fd;

    reply->handle = 0;
    if ((fd = thread_get_inflight_fd( current, req->fd )) == -1)
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    if ((file = create_file_for_fd( fd, req->access, FILE_SHARE_READ | FILE_SHARE_WRITE )))
    {
        reply->handle = alloc_handle( current->process, file, req->access, req->inherit );
        release_object( file );
    }
}

/* set a file current position */
DECL_HANDLER(set_file_pointer)
{
    int high = req->high;
    int low  = req->low;
    set_file_pointer( req->handle, &low, &high, req->whence );
    reply->new_low  = low;
    reply->new_high = high;
}

/* truncate (or extend) a file */
DECL_HANDLER(truncate_file)
{
    struct file *file;

    if ((file = get_file_obj( current->process, req->handle, GENERIC_WRITE )))
    {
        truncate_file( file );
        release_object( file );
    }
}

/* lock a region of a file */
DECL_HANDLER(lock_file)
{
    struct file *file;
    file_pos_t offset = ((file_pos_t)req->offset_high << 32) | req->offset_low;
    file_pos_t count = ((file_pos_t)req->count_high << 32) | req->count_low;

    if ((file = get_file_obj( current->process, req->handle, 0 )))
    {
        reply->handle = lock_fd( file->fd, offset, count, req->shared, req->wait );
        reply->overlapped = is_overlapped( file );
        release_object( file );
    }
}

/* unlock a region of a file */
DECL_HANDLER(unlock_file)
{
    struct file *file;
    file_pos_t offset = ((file_pos_t)req->offset_high << 32) | req->offset_low;
    file_pos_t count = ((file_pos_t)req->count_high << 32) | req->count_low;

    if ((file = get_file_obj( current->process, req->handle, 0 )))
    {
        unlock_fd( file->fd, offset, count );
        release_object( file );
    }
}
