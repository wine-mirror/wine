/*
 * Server-side file management
 *
 * Copyright (C) 1998 Alexandre Julliard
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
    int                 drive_type; /* type of drive the file is on */
};

#define NAME_HASH_SIZE 37

static struct file *file_hash[NAME_HASH_SIZE];

static void file_dump( struct object *obj, int verbose );
static int file_get_poll_events( struct object *obj );
static int file_get_fd( struct object *obj );
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
    file_get_fd,                  /* get_fd */
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
                                        unsigned int attrs, int drive_type )
{
    struct file *file;
    if ((file = alloc_object( &file_ops, fd )))
    {
        file->name       = NULL;
        file->next       = NULL;
        file->access     = access;
        file->flags      = attrs;
        file->sharing    = sharing;
        file->drive_type = drive_type;
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
    return create_file_for_fd( fd, access, 0, 0, DRIVE_FIXED );
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

static int file_get_fd( struct object *obj )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
    return file->obj.fd;
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

    if (req)
    {
        if (fstat( file->obj.fd, &st ) == -1)
        {
            file_set_error();
            return FD_TYPE_INVALID;
        }
        if (S_ISCHR(st.st_mode) || S_ISFIFO(st.st_mode) ||
            S_ISSOCK(st.st_mode) || isatty(file->obj.fd)) req->type = FILE_TYPE_CHAR;
        else req->type = FILE_TYPE_DISK;
        if (S_ISDIR(st.st_mode)) req->attr = FILE_ATTRIBUTE_DIRECTORY;
        else req->attr = FILE_ATTRIBUTE_ARCHIVE;
        if (!(st.st_mode & S_IWUSR)) req->attr |= FILE_ATTRIBUTE_READONLY;
        req->access_time = st.st_atime;
        req->write_time  = st.st_mtime;
        if (S_ISDIR(st.st_mode))
        {
            req->size_high = 0;
            req->size_low  = 0;
        }
        else
        {
            req->size_high = st.st_size >> 32;
            req->size_low  = st.st_size & 0xffffffff;
        }
        req->links       = st.st_nlink;
        req->index_high  = st.st_dev;
        req->index_low   = st.st_ino;
        req->serial      = 0; /* FIXME */
    }
    return FD_TYPE_DEFAULT;
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

struct file *get_file_obj( struct process *process, handle_t handle, unsigned int access )
{
    return (struct file *)get_handle_obj( process, handle, access, &file_ops );
}

static int set_file_pointer( handle_t handle, unsigned int *low, int *high, int whence )
{
    struct file *file;
    off_t result,xto;

    xto = *low+((off_t)*high<<32);
    if (!(file = get_file_obj( current->process, handle, 0 )))
        return 0;
    if ((result = lseek(file->obj.fd,xto,whence))==-1)
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

static int truncate_file( handle_t handle )
{
    struct file *file;
    off_t result;

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
    off_t size = size_low + (((off_t)size_high)<<32);

    if (fstat( file->obj.fd, &st ) == -1)
    {
        file_set_error();
        return 0;
    }
    if (st.st_size >= size) return 1;  /* already large enough */
    if (ftruncate( file->obj.fd, size ) != -1) return 1;
    file_set_error();
    return 0;
}

static int set_file_time( handle_t handle, time_t access_time, time_t write_time )
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

    req->handle = 0;
    if ((file = create_file( get_req_data(req), get_req_data_size(req), req->access,
                             req->sharing, req->create, req->attrs, req->drive_type )))
    {
        req->handle = alloc_handle( current->process, file, req->access, req->inherit );
        release_object( file );
    }
}

/* allocate a file handle for a Unix fd */
DECL_HANDLER(alloc_file_handle)
{
    struct file *file;
    int fd;

    req->handle = 0;
    if ((fd = thread_get_inflight_fd( current, req->fd )) == -1)
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    if ((file = create_file_for_fd( fd, req->access, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    0, DRIVE_UNKNOWN )))
    {
        req->handle = alloc_handle( current->process, file, req->access, req->inherit );
        release_object( file );
    }
}

/* get a Unix fd to access a file */
DECL_HANDLER(get_handle_fd)
{
    struct object *obj;

    req->fd = -1;
    req->type = FD_TYPE_INVALID;
    if ((obj = get_handle_obj( current->process, req->handle, req->access, NULL )))
    {
        int fd = get_handle_fd( current->process, req->handle, req->access );
        if (fd != -1) req->fd = fd;
        else if (!get_error())
        {
            if ((fd = obj->ops->get_fd( obj )) != -1)
                send_client_fd( current->process, fd, req->handle );
        }
        req->type = obj->ops->get_file_info( obj, NULL );
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
