/*
 * Server-side file management
 *
 * Copyright (C) 1998 Alexandre Julliard
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

#include "winerror.h"
#include "winbase.h"

#include "handle.h"
#include "thread.h"
#include "request.h"

struct file
{
    struct object       obj;        /* object header */
    struct file        *next;       /* next file in hashing list */
    char               *name;       /* file name */
    unsigned int        access;     /* file access (GENERIC_READ/WRITE) */
    unsigned int        flags;      /* flags (FILE_FLAG_*) */
    unsigned int        sharing;    /* file sharing mode */
};

#define NAME_HASH_SIZE 37

static struct file *file_hash[NAME_HASH_SIZE];

static void file_dump( struct object *obj, int verbose );
static int file_get_poll_events( struct object *obj );
static int file_get_read_fd( struct object *obj );
static int file_get_write_fd( struct object *obj );
static int file_flush( struct object *obj );
static int file_get_info( struct object *obj, struct get_file_info_request *req );
static void file_destroy( struct object *obj );

static const struct object_ops file_ops =
{
    sizeof(struct file),          /* size */
    file_dump,                    /* dump */
    default_poll_add_queue,       /* add_queue */
    default_poll_remove_queue,    /* remove_queue */
    default_poll_signaled,        /* signaled */
    no_satisfied,                 /* satisfied */
    file_get_poll_events,         /* get_poll_events */
    default_poll_event,           /* poll_event */
    file_get_read_fd,             /* get_read_fd */
    file_get_write_fd,            /* get_write_fd */
    file_flush,                   /* flush */
    file_get_info,                /* get_file_info */
    file_destroy                  /* destroy */
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
                                        unsigned int attrs )
{
    struct file *file;
    if ((file = alloc_object( &file_ops, fd )))
    {
        file->name    = NULL;
        file->next    = NULL;
        file->access  = access;
        file->flags   = attrs;
        file->sharing = sharing;
    }
    return file;
}


static struct file *create_file( const char *nameptr, size_t len, unsigned int access,
                                 unsigned int sharing, int create, unsigned int attrs )
{
    struct file *file;
    int hash, flags;
    struct stat st;
    char *name;
    int fd = -1;

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

    /* FIXME: should set error to STATUS_OBJECT_NAME_COLLISION if file existed before */
    if ((fd = open( name, flags | O_NONBLOCK,
                    (attrs & FILE_ATTRIBUTE_READONLY) ? 0444 : 0666 )) == -1)
        goto file_error;
    /* refuse to open a directory */
    if (fstat( fd, &st ) == -1) goto file_error;
    if (S_ISDIR(st.st_mode))
    {
        set_error( STATUS_ACCESS_DENIED );
        goto error;
    }            

    if (!(file = create_file_for_fd( fd, access, sharing, attrs )))
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

/* Create an anonymous Unix file */
int create_anonymous_file(void)
{
    char *name;
    int fd;

    do
    {
        if (!(name = tmpnam(NULL)))
        {
            set_error( STATUS_TOO_MANY_OPENED_FILES );
            return -1;
        }
        fd = open( name, O_CREAT | O_EXCL | O_RDWR, 0600 );
    } while ((fd == -1) && (errno == EEXIST));
    if (fd == -1)
    {
        file_set_error();
        return -1;
    }
    unlink( name );
    return fd;
}

/* Create a temp file for anonymous mappings */
struct file *create_temp_file( int access )
{
    int fd;

    if ((fd = create_anonymous_file()) == -1) return NULL;
    return create_file_for_fd( fd, access, 0, 0 );
}

static void file_dump( struct object *obj, int verbose )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
    fprintf( stderr, "File fd=%d flags=%08x name='%s'\n", file->obj.fd, file->flags, file->name );
}

static int file_get_poll_events( struct object *obj )
{
    struct file *file = (struct file *)obj;
    int events = 0;
    assert( obj->ops == &file_ops );
    if (file->access & GENERIC_READ) events |= POLLIN;
    if (file->access & GENERIC_WRITE) events |= POLLOUT;
    return events;
}

static int file_get_read_fd( struct object *obj )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
    return dup( file->obj.fd );
}

static int file_get_write_fd( struct object *obj )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
    return dup( file->obj.fd );
}

static int file_flush( struct object *obj )
{
    int ret;
    struct file *file = (struct file *)grab_object(obj);
    assert( obj->ops == &file_ops );

    ret = (fsync( file->obj.fd ) != -1);
    if (!ret) file_set_error();
    release_object( file );
    return ret;
}

static int file_get_info( struct object *obj, struct get_file_info_request *req )
{
    struct stat st;
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );

    if (fstat( file->obj.fd, &st ) == -1)
    {
        file_set_error();
        return 0;
    }
    if (S_ISCHR(st.st_mode) || S_ISFIFO(st.st_mode) ||
        S_ISSOCK(st.st_mode) || isatty(file->obj.fd)) req->type = FILE_TYPE_CHAR;
    else req->type = FILE_TYPE_DISK;
    if (S_ISDIR(st.st_mode)) req->attr = FILE_ATTRIBUTE_DIRECTORY;
    else req->attr = FILE_ATTRIBUTE_ARCHIVE;
    if (!(st.st_mode & S_IWUSR)) req->attr |= FILE_ATTRIBUTE_READONLY;
    req->access_time = st.st_atime;
    req->write_time  = st.st_mtime;
    req->size_high   = 0;
    req->size_low    = S_ISDIR(st.st_mode) ? 0 : st.st_size;
    req->links       = st.st_nlink;
    req->index_high  = st.st_dev;
    req->index_low   = st.st_ino;
    req->serial      = 0; /* FIXME */
    return 1;
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

struct file *get_file_obj( struct process *process, int handle, unsigned int access )
{
    return (struct file *)get_handle_obj( process, handle, access, &file_ops );
}

int file_get_mmap_fd( struct file *file )
{
    return dup( file->obj.fd );
}

static int set_file_pointer( int handle, int *low, int *high, int whence )
{
    struct file *file;
    int result;

    if (*high)
    {
        fprintf( stderr, "set_file_pointer: offset > 4Gb not supported yet\n" );
        set_error( STATUS_INVALID_PARAMETER );
        return 0;
    }

    if (!(file = get_file_obj( current->process, handle, 0 )))
        return 0;
    if ((result = lseek( file->obj.fd, *low, whence )) == -1)
    {
        /* Check for seek before start of file */
        if ((errno == EINVAL) && (whence != SEEK_SET) && (*low < 0))
            set_error( 0xc0010000 | ERROR_NEGATIVE_SEEK /* FIXME */ );
        else
            file_set_error();
        release_object( file );
        return 0;
    }
    *low = result;
    release_object( file );
    return 1;
}

static int truncate_file( int handle )
{
    struct file *file;
    int result;

    if (!(file = get_file_obj( current->process, handle, GENERIC_WRITE )))
        return 0;
    if (((result = lseek( file->obj.fd, 0, SEEK_CUR )) == -1) ||
        (ftruncate( file->obj.fd, result ) == -1))
    {
        file_set_error();
        release_object( file );
        return 0;
    }
    release_object( file );
    return 1;
    
}

/* try to grow the file to the specified size */
int grow_file( struct file *file, int size_high, int size_low )
{
    struct stat st;

    if (size_high)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return 0;
    }
    if (fstat( file->obj.fd, &st ) == -1)
    {
        file_set_error();
        return 0;
    }
    if (st.st_size >= size_low) return 1;  /* already large enough */
    if (ftruncate( file->obj.fd, size_low ) != -1) return 1;
    file_set_error();
    return 0;
}

static int set_file_time( int handle, time_t access_time, time_t write_time )
{
    struct file *file;
    struct utimbuf utimbuf;

    if (!(file = get_file_obj( current->process, handle, GENERIC_WRITE )))
        return 0;
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
    size_t len = get_req_strlen( req, req->name );
    struct file *file;

    req->handle = -1;
    if ((file = create_file( req->name, len, req->access,
                             req->sharing, req->create, req->attrs )))
    {
        req->handle = alloc_handle( current->process, file, req->access, req->inherit );
        release_object( file );
    }
}

/* allocate a file handle for a Unix fd */
DECL_HANDLER(alloc_file_handle)
{
    struct file *file;

    req->handle = -1;
    if (current->pass_fd != -1)
    {
        if ((file = create_file_for_fd( current->pass_fd, req->access,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE, 0 )))
        {
            req->handle = alloc_handle( current->process, file, req->access, 0 );
            release_object( file );
        }
        current->pass_fd = -1;
    }
    else set_error( STATUS_INVALID_PARAMETER );
}

/* get a Unix fd to read from a file */
DECL_HANDLER(get_read_fd)
{
    struct object *obj;

    if ((obj = get_handle_obj( current->process, req->handle, GENERIC_READ, NULL )))
    {
        set_reply_fd( current, obj->ops->get_read_fd( obj ) );
        release_object( obj );
    }
}

/* get a Unix fd to write to a file */
DECL_HANDLER(get_write_fd)
{
    struct object *obj;

    if ((obj = get_handle_obj( current->process, req->handle, GENERIC_WRITE, NULL )))
    {
        set_reply_fd( current, obj->ops->get_write_fd( obj ) );
        release_object( obj );
    }
}

/* set a file current position */
DECL_HANDLER(set_file_pointer)
{
    int high = req->high;
    int low  = req->low;
    set_file_pointer( req->handle, &low, &high, req->whence );
    req->new_low  = low;
    req->new_high = high;
}

/* truncate (or extend) a file */
DECL_HANDLER(truncate_file)
{
    truncate_file( req->handle );
}

/* flush a file buffers */
DECL_HANDLER(flush_file)
{
    struct object *obj;

    if ((obj = get_handle_obj( current->process, req->handle, 0, NULL )))
    {
        obj->ops->flush( obj );
        release_object( obj );
    }
}

/* set a file access and modification times */
DECL_HANDLER(set_file_time)
{
    set_file_time( req->handle, req->access_time, req->write_time );
}

/* get a file information */
DECL_HANDLER(get_file_info)
{
    struct object *obj;

    if ((obj = get_handle_obj( current->process, req->handle, 0, NULL )))
    {
        obj->ops->get_file_info( obj, req );
        release_object( obj );
    }
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
