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
#include <utime.h>

#include "winerror.h"
#include "winbase.h"
#include "server/thread.h"

struct file
{
    struct object  obj;             /* object header */
    struct file   *next;            /* next file in hashing list */
    char          *name;            /* file name */
    int            fd;              /* Unix file descriptor */
    unsigned int   access;          /* file access (GENERIC_READ/WRITE) */
    unsigned int   flags;           /* flags (FILE_FLAG_*) */
    unsigned int   sharing;         /* file sharing mode */
};

#define NAME_HASH_SIZE 37

static struct file *file_hash[NAME_HASH_SIZE];

static void file_dump( struct object *obj, int verbose );
static int file_add_queue( struct object *obj, struct wait_queue_entry *entry );
static void file_remove_queue( struct object *obj, struct wait_queue_entry *entry );
static int file_signaled( struct object *obj, struct thread *thread );
static int file_get_read_fd( struct object *obj );
static int file_get_write_fd( struct object *obj );
static int file_flush( struct object *obj );
static int file_get_info( struct object *obj, struct get_file_info_reply *reply );
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
    file_get_info,
    file_destroy
};

static const struct select_ops select_ops =
{
    default_select_event,
    NULL   /* we never set a timeout on a file */
};


static int get_name_hash( const char *name )
{
    int hash = 0;
    while (*name) hash ^= *name++;
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
    if ((access & GENERIC_READ) && !(existing_sharing & FILE_SHARE_READ)) return 0;
    if ((access & GENERIC_WRITE) && !(existing_sharing & FILE_SHARE_WRITE)) return 0;
    if ((existing_access & GENERIC_READ) && !(sharing & FILE_SHARE_READ)) return 0;
    if ((existing_access & GENERIC_WRITE) && !(sharing & FILE_SHARE_WRITE)) return 0;
    return 1;
}

struct object *create_file( int fd, const char *name, unsigned int access,
                            unsigned int sharing, int create, unsigned int attrs )
{
    struct file *file;
    int hash = 0;

    if (fd == -1)
    {
        int flags;
        struct stat st;

        if (!name)
        {
            SET_ERROR( ERROR_INVALID_PARAMETER );
            return NULL;
        }

        /* check sharing mode */
        hash = get_name_hash( name );
        if (!check_sharing( name, hash, access, sharing ))
        {
            SET_ERROR( ERROR_SHARING_VIOLATION );
            return NULL;
        }

        switch(create)
        {
        case CREATE_NEW:        flags = O_CREAT | O_EXCL; break;
        case CREATE_ALWAYS:     flags = O_CREAT | O_TRUNC; break;
        case OPEN_ALWAYS:       flags = O_CREAT; break;
        case TRUNCATE_EXISTING: flags = O_TRUNC; break;
        case OPEN_EXISTING:     flags = 0; break;
        default:                SET_ERROR( ERROR_INVALID_PARAMETER ); return NULL;
        }
        switch(access & (GENERIC_READ | GENERIC_WRITE))
        {
        case 0: break;
        case GENERIC_READ:  flags |= O_RDONLY; break;
        case GENERIC_WRITE: flags |= O_WRONLY; break;
        case GENERIC_READ|GENERIC_WRITE: flags |= O_RDWR; break;
        }

        if ((fd = open( name, flags | O_NONBLOCK,
                        (attrs & FILE_ATTRIBUTE_READONLY) ? 0444 : 0666 )) == -1)
        {
            file_set_error();
            return NULL;
        }
        /* Refuse to open a directory */
        if (fstat( fd, &st ) == -1)
        {
            file_set_error();
            close( fd );
            return NULL;
        }
        if (S_ISDIR(st.st_mode))
        {
            SET_ERROR( ERROR_ACCESS_DENIED );
            close( fd );
            return NULL;
        }            
    }
    else
    {
        if ((fd = dup(fd)) == -1)
        {
            file_set_error();
            return NULL;
        }
    }

    if (!(file = mem_alloc( sizeof(*file) )))
    {
        close( fd );
        return NULL;
    }
    if (name)
    {
        if (!(file->name = mem_alloc( strlen(name) + 1 )))
        {
            close( fd );
            free( file );
            return NULL;
        }
        strcpy( file->name, name );
        file->next = file_hash[hash];
        file_hash[hash] = file;
    }
    else
    {
        file->name = NULL;
        file->next = NULL;
    }
    init_object( &file->obj, &file_ops, NULL );
    file->fd      = fd;
    file->access  = access;
    file->flags   = attrs;
    file->sharing = sharing;
    CLEAR_ERROR();
    return &file->obj;
}

static void file_dump( struct object *obj, int verbose )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
    printf( "File fd=%d flags=%08x name='%s'\n",
            file->fd, file->flags, file->name );
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
    if (file->access & GENERIC_READ) FD_SET( file->fd, &read_fds );
    if (file->access & GENERIC_WRITE) FD_SET( file->fd, &write_fds );
    return select( file->fd + 1, &read_fds, &write_fds, NULL, &tv ) > 0;
}

static int file_get_read_fd( struct object *obj )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
    return dup( file->fd );
}

static int file_get_write_fd( struct object *obj )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
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

static int file_get_info( struct object *obj, struct get_file_info_reply *reply )
{
    struct stat st;
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );

    if (fstat( file->fd, &st ) == -1)
    {
        file_set_error();
        return 0;
    }
    if (S_ISCHR(st.st_mode) || S_ISFIFO(st.st_mode) ||
        S_ISSOCK(st.st_mode) || isatty(file->fd)) reply->type = FILE_TYPE_CHAR;
    else reply->type = FILE_TYPE_DISK;
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
        free( file->name );
    }
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

int set_file_time( int handle, time_t access_time, time_t write_time )
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

int file_lock( struct file *file, int offset_high, int offset_low,
               int count_high, int count_low )
{
    /* FIXME: implement this */
    return 1;
}

int file_unlock( struct file *file, int offset_high, int offset_low,
                 int count_high, int count_low )
{
    /* FIXME: implement this */
    return 1;
}
