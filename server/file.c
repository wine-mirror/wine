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
#include "winbase.h"

#include "file.h"
#include "handle.h"
#include "thread.h"
#include "request.h"
#include "async.h"

struct file
{
    struct object       obj;        /* object header */
    struct file        *next;       /* next file in hashing list */
    char               *name;       /* file name */
    unsigned int        access;     /* file access (GENERIC_READ/WRITE) */
    unsigned int        flags;      /* flags (FILE_FLAG_*) */
    unsigned int        sharing;    /* file sharing mode */
    int                 drive_type; /* type of drive the file is on */
    struct async_queue  read_q;
    struct async_queue  write_q;
};

#define NAME_HASH_SIZE 37

static struct file *file_hash[NAME_HASH_SIZE];

static void file_dump( struct object *obj, int verbose );
static void file_destroy( struct object *obj );

static int file_get_poll_events( struct fd *fd );
static void file_poll_event( struct fd *fd, int event );
static int file_flush( struct fd *fd );
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
    default_get_fd,               /* get_fd */
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

static int get_name_hash( const char *name )
{
    int hash = 0;
    while (*name) hash ^= (unsigned char)*name++;
    return hash % NAME_HASH_SIZE;
}

/* check if the desired access is possible without violating */
/* the sharing mode of other opens of the same file */
static int check_sharing( const char *name, int hash, unsigned int access,
                          unsigned int sharing )
{
    struct file *file;
    unsigned int existing_sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
    unsigned int existing_access = 0;

    for (file = file_hash[hash]; file; file = file->next)
    {
        if (strcmp( file->name, name )) continue;
        existing_sharing &= file->sharing;
        existing_access |= file->access;
    }
    if ((access & GENERIC_READ) && !(existing_sharing & FILE_SHARE_READ)) goto error;
    if ((access & GENERIC_WRITE) && !(existing_sharing & FILE_SHARE_WRITE)) goto error;
    if ((existing_access & GENERIC_READ) && !(sharing & FILE_SHARE_READ)) goto error;
    if ((existing_access & GENERIC_WRITE) && !(sharing & FILE_SHARE_WRITE)) goto error;
    return 1;
 error:
    set_error( STATUS_SHARING_VIOLATION );
    return 0;
}

/* create a file from a file descriptor */
/* if the function fails the fd is closed */
static struct file *create_file_for_fd( int fd, unsigned int access, unsigned int sharing,
                                        unsigned int attrs, int drive_type )
{
    struct file *file;

    if ((file = alloc_fd_object( &file_ops, &file_fd_ops, fd )))
    {
        file->name       = NULL;
        file->next       = NULL;
        file->access     = access;
        file->flags      = attrs;
        file->sharing    = sharing;
        file->drive_type = drive_type;
        if (file->flags & FILE_FLAG_OVERLAPPED)
        {
            init_async_queue (&file->read_q);
            init_async_queue (&file->write_q);
        }
    }
    return file;
}


static struct file *create_file( const char *nameptr, size_t len, unsigned int access,
                                 unsigned int sharing, int create, unsigned int attrs,
                                 int drive_type )
{
    struct file *file;
    int hash, flags;
    struct stat st;
    char *name;
    int fd = -1;
    mode_t mode;

    if (!(name = mem_alloc( len + 1 ))) return NULL;
    memcpy( name, nameptr, len );
    name[len] = 0;

    /* check sharing mode */
    hash = get_name_hash( name );
    if (!check_sharing( name, hash, access, sharing )) goto error;

    switch(create)
    {
    case CREATE_NEW:        flags = O_CREAT | O_EXCL; break;
    case CREATE_ALWAYS:     flags = O_CREAT | O_TRUNC; break;
    case OPEN_ALWAYS:       flags = O_CREAT; break;
    case TRUNCATE_EXISTING: flags = O_TRUNC; break;
    case OPEN_EXISTING:     flags = 0; break;
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

    /* FIXME: should set error to STATUS_OBJECT_NAME_COLLISION if file existed before */
    if ((fd = open( name, flags | O_NONBLOCK | O_LARGEFILE, mode )) == -1 )
        goto file_error;
    /* refuse to open a directory */
    if (fstat( fd, &st ) == -1) goto file_error;
    if (S_ISDIR(st.st_mode))
    {
        set_error( STATUS_ACCESS_DENIED );
        goto error;
    }

    if (!(file = create_file_for_fd( fd, access, sharing, attrs, drive_type )))
    {
        free( name );
        return NULL;
    }
    file->name = name;
    file->next = file_hash[hash];
    file_hash[hash] = file;
    return file;

 file_error:
    file_set_error();
 error:
    if (fd != -1) close( fd );
    free( name );
    return NULL;
}

/* check if two file objects point to the same file */
int is_same_file( struct file *file1, struct file *file2 )
{
    return !strcmp( file1->name, file2->name );
}

/* get the type of drive the file is on */
int get_file_drive_type( struct file *file )
{
    return file->drive_type;
}

/* Create an anonymous Unix file */
int create_anonymous_file(void)
{
    char tmpfn[21];
    int fd;

    sprintf(tmpfn,"/tmp/anonmap.XXXXXX");
    fd = mkstemp(tmpfn);
    if (fd == -1)
    {
        file_set_error();
        return -1;
    }
    unlink( tmpfn );
    return fd;
}

/* Create a temp file for anonymous mappings */
struct file *create_temp_file( int access )
{
    int fd;

    if ((fd = create_anonymous_file()) == -1) return NULL;
    return create_file_for_fd( fd, access, 0, 0, DRIVE_FIXED );
}

static void file_dump( struct object *obj, int verbose )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
    fprintf( stderr, "File fd=%p flags=%08x name='%s'\n", file->obj.fd_obj, file->flags, file->name );
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
    if ( file->flags & FILE_FLAG_OVERLAPPED )
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


static int file_flush( struct fd *fd )
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
        if (S_ISDIR(st.st_mode))
        {
            reply->size_high = 0;
            reply->size_low  = 0;
        }
        else
        {
            reply->size_high = st.st_size >> 32;
            reply->size_low  = st.st_size & 0xffffffff;
        }
        reply->links       = st.st_nlink;
        reply->index_high  = st.st_dev;
        reply->index_low   = st.st_ino;
        reply->serial      = 0; /* FIXME */
    }
    *flags = 0;
    if (file->flags & FILE_FLAG_OVERLAPPED) *flags |= FD_FLAG_OVERLAPPED;
    return FD_TYPE_DEFAULT;
}

static void file_queue_async(struct fd *fd, void *ptr, unsigned int status, int type, int count)
{
    struct file *file = get_fd_user( fd );
    struct async *async;
    struct async_queue *q;

    assert( file->obj.ops == &file_ops );

    if ( !(file->flags & FILE_FLAG_OVERLAPPED) )
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

    set_select_events( &file->obj, file_get_poll_events( fd ));
}

static void file_destroy( struct object *obj )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );

    if (file->name)
    {
        /* remove it from the hashing list */
        struct file **pptr = &file_hash[get_name_hash( file->name )];
        while (*pptr && *pptr != file) pptr = &(*pptr)->next;
        assert( *pptr );
        *pptr = (*pptr)->next;
        if (file->flags & FILE_FLAG_DELETE_ON_CLOSE) unlink( file->name );
        free( file->name );
    }
    if (file->flags & FILE_FLAG_OVERLAPPED)
    {
        destroy_async_queue (&file->read_q);
        destroy_async_queue (&file->write_q);
    }
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
    case EISDIR:    set_error( 0xc0010000 | ERROR_CANNOT_MAKE /* FIXME */ ); break;
    case ENFILE:
    case EMFILE:    set_error( STATUS_NO_MORE_FILES ); break;
    case EEXIST:    set_error( STATUS_OBJECT_NAME_COLLISION ); break;
    case EINVAL:    set_error( STATUS_INVALID_PARAMETER ); break;
    case ESPIPE:    set_error( 0xc0010000 | ERROR_SEEK /* FIXME */ ); break;
    case ENOTEMPTY: set_error( STATUS_DIRECTORY_NOT_EMPTY ); break;
    case EIO:       set_error( STATUS_ACCESS_VIOLATION ); break;
    default:        perror("file_set_error"); set_error( ERROR_UNKNOWN /* FIXME */ ); break;
    }
}

struct file *get_file_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct file *)get_handle_obj( process, handle, access, &file_ops );
}

int get_file_unix_fd( struct file *file )
{
    return get_unix_fd( file->obj.fd_obj );
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
            set_error( 0xc0010000 | ERROR_NEGATIVE_SEEK /* FIXME */ );
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

static int set_file_time( obj_handle_t handle, time_t access_time, time_t write_time )
{
    struct file *file;
    struct utimbuf utimbuf;

    if (!(file = get_file_obj( current->process, handle, GENERIC_WRITE )))
        return 0;
    if (!file->name)
    {
        set_error( STATUS_INVALID_HANDLE );
        release_object( file );
        return 0;
    }
    if (!access_time || !write_time)
    {
        struct stat st;
        if (stat( file->name, &st ) == -1) goto error;
        if (!access_time) access_time = st.st_atime;
        if (!write_time) write_time = st.st_mtime;
    }
    utimbuf.actime  = access_time;
    utimbuf.modtime = write_time;
    if (utime( file->name, &utimbuf ) == -1) goto error;
    release_object( file );
    return 1;
 error:
    file_set_error();
    release_object( file );
    return 0;
}

static int file_lock( struct file *file, int offset_high, int offset_low,
                      int count_high, int count_low )
{
    /* FIXME: implement this */
    return 1;
}

static int file_unlock( struct file *file, int offset_high, int offset_low,
                        int count_high, int count_low )
{
    /* FIXME: implement this */
    return 1;
}

/* create a file */
DECL_HANDLER(create_file)
{
    struct file *file;

    reply->handle = 0;
    if ((file = create_file( get_req_data(), get_req_data_size(), req->access,
                             req->sharing, req->create, req->attrs, req->drive_type )))
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
    if ((file = create_file_for_fd( fd, req->access, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    0, DRIVE_UNKNOWN )))
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

/* set a file access and modification times */
DECL_HANDLER(set_file_time)
{
    set_file_time( req->handle, req->access_time, req->write_time );
}

/* lock a region of a file */
DECL_HANDLER(lock_file)
{
    struct file *file;

    if ((file = get_file_obj( current->process, req->handle, 0 )))
    {
        file_lock( file, req->offset_high, req->offset_low,
                   req->count_high, req->count_low );
        release_object( file );
    }
}

/* unlock a region of a file */
DECL_HANDLER(unlock_file)
{
    struct file *file;

    if ((file = get_file_obj( current->process, req->handle, 0 )))
    {
        file_unlock( file, req->offset_high, req->offset_low,
                     req->count_high, req->count_low );
        release_object( file );
    }
}
