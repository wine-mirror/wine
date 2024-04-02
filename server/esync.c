/*
 * eventfd-based synchronization objects
 *
 * Copyright (C) 2018 Zebediah Figura
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"


#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#ifdef HAVE_SYS_EVENTFD_H
# include <sys/eventfd.h>
#endif
#include <sys/mman.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#include <unistd.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "handle.h"
#include "request.h"
#include "file.h"
#include "esync.h"
#include "msync.h"

int do_esync(void)
{
    static int do_esync_cached = -1;

    if (do_esync_cached == -1)
        do_esync_cached = getenv("WINEESYNC") && atoi(getenv("WINEESYNC")) && !do_msync();

    return do_esync_cached;
}

static char shm_name[29];
static int shm_fd;
static off_t shm_size;
static void **shm_addrs;
static int shm_addrs_size;  /* length of the allocated shm_addrs array */
static long pagesize;

static void shm_cleanup(void)
{
    close( shm_fd );
    if (shm_unlink( shm_name ) == -1)
        perror( "shm_unlink" );
}

void esync_init(void)
{
    struct stat st;

    if (fstat( config_dir_fd, &st ) == -1)
        fatal_error( "cannot stat config dir\n" );

    if (st.st_ino != (unsigned long)st.st_ino)
        sprintf( shm_name, "/wine-%lx%08lx-esync", (unsigned long)((unsigned long long)st.st_ino >> 32), (unsigned long)st.st_ino );
    else
        sprintf( shm_name, "/wine-%lx-esync", (unsigned long)st.st_ino );

    shm_unlink( shm_name );

    shm_fd = shm_open( shm_name, O_RDWR | O_CREAT | O_EXCL, 0644 );
    if (shm_fd == -1)
        perror( "shm_open" );

    pagesize = sysconf( _SC_PAGESIZE );

    shm_addrs = calloc( 128, sizeof(shm_addrs[0]) );
    shm_addrs_size = 128;

    shm_size = pagesize * 1024;
    if (ftruncate( shm_fd, shm_size ) == -1)
        perror( "ftruncate" );

    fprintf( stderr, "esync: up and running.\n" );

    atexit( shm_cleanup );
}

static struct list mutex_list = LIST_INIT(mutex_list);

struct esync_fd
{
#ifdef HAVE_SYS_EVENTFD_H
    int fd;
#else
    /* Write to 0, read from 1. */
    int fds[2];
#endif
};

struct esync
{
    struct object   obj;            /* object header */
    /* No, this isn't a pointer. We can have lots of these so let's try to save
     * memory a little. */
    struct esync_fd fd;             /* eventfd file descriptor */
    enum esync_type type;
    unsigned int    shm_idx;        /* index into the shared memory section */
    struct list     mutex_entry;    /* entry in the mutex list (if applicable) */
};

static void esync_dump( struct object *obj, int verbose );
static struct esync_fd *esync_get_esync_fd( struct object *obj, enum esync_type *type );
static unsigned int esync_map_access( struct object *obj, unsigned int access );
static void esync_destroy( struct object *obj );

const struct object_ops esync_ops =
{
    sizeof(struct esync),      /* size */
    &no_type,                  /* type */
    esync_dump,                /* dump */
    no_add_queue,              /* add_queue */
    NULL,                      /* remove_queue */
    NULL,                      /* signaled */
    esync_get_esync_fd,        /* get_esync_fd */
    NULL,                      /* get_msync_idx */
    NULL,                      /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    esync_map_access,          /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    default_get_full_name,     /* get_full_name */
    no_lookup_name,            /* lookup_name */
    directory_link_name,       /* link_name */
    default_unlink_name,       /* unlink_name */
    no_open_file,              /* open_file */
    no_kernel_obj_list,        /* get_kernel_obj_list */
    no_close_handle,           /* close_handle */
    esync_destroy              /* destroy */
};

static void esync_dump( struct object *obj, int verbose )
{
    struct esync *esync = (struct esync *)obj;
    assert( obj->ops == &esync_ops );
#ifdef HAVE_SYS_EVENTFD_H
    fprintf( stderr, "esync type=%d fd=%d\n", esync->type, esync->fd.fd );
#else
    fprintf( stderr, "esync type=%d fd=%d,%d\n", esync->type, esync->fd.fds[0], esync->fd.fds[1] );
#endif
}

static struct esync_fd *esync_get_esync_fd( struct object *obj, enum esync_type *type )
{
    struct esync *esync = (struct esync *)obj;
    *type = esync->type;
    return &esync->fd;
}

static unsigned int esync_map_access( struct object *obj, unsigned int access )
{
    /* Sync objects have the same flags. */
    if (access & GENERIC_READ)    access |= STANDARD_RIGHTS_READ | EVENT_QUERY_STATE;
    if (access & GENERIC_WRITE)   access |= STANDARD_RIGHTS_WRITE | EVENT_MODIFY_STATE;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE;
    if (access & GENERIC_ALL)     access |= STANDARD_RIGHTS_ALL | EVENT_QUERY_STATE | EVENT_MODIFY_STATE;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static void esync_destroy( struct object *obj )
{
    struct esync *esync = (struct esync *)obj;
    if (esync->type == ESYNC_MUTEX)
        list_remove( &esync->mutex_entry );
#ifdef HAVE_SYS_EVENTFD_H
    close( esync->fd.fd );
#else
    close( esync->fd.fds[0] );
    close( esync->fd.fds[1] );
#endif
}

static int type_matches( enum esync_type type1, enum esync_type type2 )
{
    return (type1 == type2) ||
           ((type1 == ESYNC_AUTO_EVENT || type1 == ESYNC_MANUAL_EVENT) &&
            (type2 == ESYNC_AUTO_EVENT || type2 == ESYNC_MANUAL_EVENT));
}

static void *get_shm( unsigned int idx )
{
    int entry  = (idx * 8) / pagesize;
    int offset = (idx * 8) % pagesize;

    if (entry >= shm_addrs_size)
    {
        int new_size = max(shm_addrs_size * 2, entry + 1);

        if (!(shm_addrs = realloc( shm_addrs, new_size * sizeof(shm_addrs[0]) )))
            fprintf( stderr, "esync: couldn't expand shm_addrs array to size %d\n", entry + 1 );

        memset( shm_addrs + shm_addrs_size, 0, (new_size - shm_addrs_size) * sizeof(shm_addrs[0]) );

        shm_addrs_size = new_size;
    }

    if (!shm_addrs[entry])
    {
        void *addr = mmap( NULL, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, entry * pagesize );
        if (addr == (void *)-1)
        {
            fprintf( stderr, "esync: failed to map page %d (offset %#lx): ", entry, entry * pagesize );
            perror( "mmap" );
        }

        if (debug_level)
            fprintf( stderr, "esync: Mapping page %d at %p.\n", entry, addr );

        if (__sync_val_compare_and_swap( &shm_addrs[entry], 0, addr ))
            munmap( addr, pagesize ); /* someone beat us to it */
    }

    return (void *)((unsigned long)shm_addrs[entry] + offset);
}

struct semaphore
{
    int max;
    int count;
};
C_ASSERT(sizeof(struct semaphore) == 8);

struct mutex
{
    DWORD tid;
    int count;    /* recursion count */
};
C_ASSERT(sizeof(struct mutex) == 8);

struct event
{
    int signaled;
    int locked;
};
C_ASSERT(sizeof(struct event) == 8);

static int esync_init_fd( struct esync_fd *fd, int initval, int semaphore )
{
#ifdef HAVE_SYS_EVENTFD_H
    int flags = EFD_CLOEXEC | EFD_NONBLOCK;
    if (semaphore) flags |= EFD_SEMAPHORE;
    fd->fd = eventfd( initval, flags );
    if (fd->fd == -1)
    {
        perror( "eventfd" );
        return -1;
    }
#else
    static const unsigned char value;
    int fdflags;

    if (pipe(fd->fds) == -1)
    {
        perror( "pipe" );
        return -1;
    }
    fcntl( fd->fds[0], F_SETFD, FD_CLOEXEC );
    if ((fdflags = fcntl( fd->fds[0], F_GETFL, 0 )) == -1)
        fdflags = 0;
    fcntl( fd->fds[0], F_SETFL, fdflags | O_NONBLOCK );

    fcntl( fd->fds[1], F_SETFD, FD_CLOEXEC );
    if ((fdflags = fcntl( fd->fds[1], F_GETFL, 0 )) == -1)
        fdflags = 0;
    fcntl( fd->fds[1], F_SETFL, fdflags | O_NONBLOCK );

    while (initval--)
        write( fd->fds[1], &value, sizeof(value) );
#endif
    return 0;
}

struct esync *create_esync( struct object *root, const struct unicode_str *name,
                            unsigned int attr, int initval, int max, enum esync_type type,
                            const struct security_descriptor *sd )
{
    struct esync *esync;

    if ((esync = create_named_object( root, &esync_ops, name, attr, sd )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            esync->type = type;

            /* initialize it if it didn't already exist */
            if (esync_init_fd( &esync->fd, initval, type == ESYNC_SEMAPHORE ) == -1)
            {
                perror( "eventfd" );
                file_set_error();
                release_object( esync );
                return NULL;
            }

            /* Use the fd as index, since that'll be unique across all
             * processes, but should hopefully end up also allowing reuse. */
#ifdef HAVE_SYS_EVENTFD_H
            esync->shm_idx = esync->fd.fd + 1; /* we keep index 0 reserved */
#else
            esync->shm_idx = esync->fd.fds[0] + 1; /* we keep index 0 reserved */
#endif

            while (esync->shm_idx * 8 >= shm_size)
            {
                /* Better expand the shm section. */
                shm_size += pagesize;
                if (ftruncate( shm_fd, shm_size ) == -1)
                {
                    fprintf( stderr, "esync: couldn't expand %s to size %ld: ",
                             shm_name, (long)shm_size );
                    perror( "ftruncate" );
                }
            }

            /* Initialize the shared memory portion. We want to do this on the
             * server side to avoid a potential though unlikely race whereby
             * the same object is opened and used between the time it's created
             * and the time its shared memory portion is initialized. */
            switch (type)
            {
            case ESYNC_SEMAPHORE:
            {
                struct semaphore *semaphore = get_shm( esync->shm_idx );
                semaphore->max = max;
                semaphore->count = initval;
                break;
            }
            case ESYNC_AUTO_EVENT:
            case ESYNC_MANUAL_EVENT:
            {
                struct event *event = get_shm( esync->shm_idx );
                event->signaled = initval ? 1 : 0;
                event->locked = 0;
                break;
            }
            case ESYNC_MUTEX:
            {
                struct mutex *mutex = get_shm( esync->shm_idx );
                mutex->tid = initval ? 0 : current->id;
                mutex->count = initval ? 0 : 1;
                list_add_tail( &mutex_list, &esync->mutex_entry );
                break;
            }
            default:
                assert( 0 );
            }
        }
        else
        {
            /* validate the type */
            if (!type_matches( type, esync->type ))
            {
                release_object( &esync->obj );
                set_error( STATUS_OBJECT_TYPE_MISMATCH );
                return NULL;
            }
        }
    }
    return esync;
}

/* Create an eventfd or eventfd-like primitive for an existing handle.
 * Caller must close with esync_close_fd(). */
struct esync_fd *esync_create_fd( int initval, int semaphore )
{
    struct esync_fd *fd = mem_alloc( sizeof(*fd) );
    esync_init_fd( fd, initval, semaphore );
    return fd;
}

void esync_close_fd( struct esync_fd *fd )
{
#ifdef HAVE_SYS_EVENTFD_H
    close( fd->fd );
#else
    close( fd->fds[0] );
    close( fd->fds[1] );
#endif
    free( fd );
}

/* Wake up a specific fd. */
void esync_wake_fd( struct esync_fd *fd )
{
#ifdef HAVE_SYS_EVENTFD_H
    static const uint64_t value = 1;

    if (write( fd->fd, &value, sizeof(value) ) == -1 && errno != EAGAIN)
        perror( "esync: write" );
#else
    static const char value;

    if (write( fd->fds[1], &value, sizeof(value) ) == -1 && errno != EAGAIN)
        perror( "esync: write" );
#endif
}

/* Wake up a server-side esync object. */
void esync_wake_up( struct object *obj )
{
    enum esync_type dummy;

    if (obj->ops->get_esync_fd)
    {
        esync_wake_fd( obj->ops->get_esync_fd( obj, &dummy ) );
    }
}

void esync_clear( struct esync_fd *fd )
{
#ifdef HAVE_SYS_EVENTFD_H
    uint64_t value;

    /* we don't care about the return value */
    read( fd->fd, &value, sizeof(value) );
#else
    static char buffer[4096];
    while (read( fd->fds[0], buffer, sizeof(buffer) ) == sizeof(buffer));
#endif
}

static inline void small_pause(void)
{
#ifdef __i386__
    __asm__ __volatile__( "rep;nop" : : : "memory" );
#else
    __asm__ __volatile__( "" : : : "memory" );
#endif
}

/* Server-side event support. */
void esync_set_event( struct esync *esync )
{
    struct event *event = get_shm( esync->shm_idx );

    assert( esync->obj.ops == &esync_ops );
    assert( event != NULL );

    if (debug_level)
    {
        fprintf( stderr, "esync_set_event()" );
        esync_dump( &esync->obj, 0 );
        fprintf( stderr, "\n" );
    }

    if (esync->type == ESYNC_MANUAL_EVENT)
    {
        /* Acquire the spinlock. */
        while (__sync_val_compare_and_swap( &event->locked, 0, 1 ))
            small_pause();
    }

    if (!InterlockedExchange( &event->signaled, 1 ))
        esync_wake_fd( &esync->fd );

    if (esync->type == ESYNC_MANUAL_EVENT)
    {
        /* Release the spinlock. */
        event->locked = 0;
    }
}

void esync_reset_event( struct esync *esync )
{
    struct event *event = get_shm( esync->shm_idx );

    assert( esync->obj.ops == &esync_ops );
    assert( event != NULL );

    if (debug_level)
    {
        fprintf( stderr, "esync_reset_event()" );
        esync_dump( &esync->obj, 0 );
        fprintf( stderr, "\n" );
    }

    if (esync->type == ESYNC_MANUAL_EVENT)
    {
        /* Acquire the spinlock. */
        while (__sync_val_compare_and_swap( &event->locked, 0, 1 ))
            small_pause();
    }

    /* Only bother signaling the fd if we weren't already signaled. */
    if (InterlockedExchange( &event->signaled, 0 ))
        esync_clear( &esync->fd );

    if (esync->type == ESYNC_MANUAL_EVENT)
    {
        /* Release the spinlock. */
        event->locked = 0;
    }
}

void esync_abandon_mutexes( struct thread *thread )
{
    struct esync *esync;

    LIST_FOR_EACH_ENTRY( esync, &mutex_list, struct esync, mutex_entry )
    {
        struct mutex *mutex = get_shm( esync->shm_idx );

        if (mutex->tid == thread->id)
        {
            if (debug_level)
            {
                fprintf( stderr, "esync_abandon_mutexes()\n" );
                esync_dump( &esync->obj, 0 );
                fprintf( stderr, "\n" );
            }
            mutex->tid = ~0;
            mutex->count = 0;
            esync_wake_fd( &esync->fd );
        }
    }
}

DECL_HANDLER(create_esync)
{
    struct esync *esync;
    struct unicode_str name;
    struct object *root;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, &root );

    if (!do_esync())
    {
        set_error( STATUS_NOT_IMPLEMENTED );
        return;
    }

    if (!req->type)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if (!objattr) return;

    if ((esync = create_esync( root, &name, objattr->attributes, req->initval, req->max, req->type, sd )))
    {
        if (get_error() == STATUS_OBJECT_NAME_EXISTS)
            reply->handle = alloc_handle( current->process, esync, req->access, objattr->attributes );
        else
            reply->handle = alloc_handle_no_access_check( current->process, esync,
                                                          req->access, objattr->attributes );

        reply->type = esync->type;
        reply->shm_idx = esync->shm_idx;
#ifdef HAVE_SYS_EVENTFD_H
        send_client_fd( current->process, esync->fd.fd, reply->handle );
#else
        send_client_fd( current->process, esync->fd.fds[0], reply->handle );
#endif
        release_object( esync );
    }

    if (root) release_object( root );
}

DECL_HANDLER(open_esync)
{
    struct unicode_str name = get_req_unicode_str();

    reply->handle = open_object( current->process, req->rootdir, req->access,
                                 &esync_ops, &name, req->attributes );

    /* send over the fd */
    if (reply->handle)
    {
        struct esync *esync;

        if (!(esync = (struct esync *)get_handle_obj( current->process, reply->handle,
                                                      0, &esync_ops )))
            return;

        if (!type_matches( req->type, esync->type ))
        {
            set_error( STATUS_OBJECT_TYPE_MISMATCH );
            release_object( esync );
            return;
        }

        reply->type = esync->type;
        reply->shm_idx = esync->shm_idx;

#ifdef HAVE_SYS_EVENTFD_H
        send_client_fd( current->process, esync->fd.fd, reply->handle );
#else
        send_client_fd( current->process, esync->fd.fds[0], reply->handle );
#endif
        release_object( esync );
    }
}

/* Retrieve a file descriptor for an esync object which will be signaled by the
 * server. The client should only read from (i.e. wait on) this object. */
DECL_HANDLER(get_esync_read_fd)
{
    struct object *obj;
    enum esync_type type;
    struct esync_fd *fd;

    if (!(obj = get_handle_obj( current->process, req->handle, 0, NULL )))
        return;

    if (obj->ops->get_esync_fd)
    {
        fd = obj->ops->get_esync_fd( obj, &type );
        reply->type = type;
        if (obj->ops == &esync_ops)
        {
            struct esync *esync = (struct esync *)obj;
            reply->shm_idx = esync->shm_idx;
        }
        else
            reply->shm_idx = 0;
#ifdef HAVE_SYS_EVENTFD_H
        send_client_fd( current->process, fd->fd, req->handle );
#else
        send_client_fd( current->process, fd->fds[0], req->handle );
#endif
    }
    else
    {
        if (debug_level)
        {
            fprintf( stderr, "%04x: esync: can't wait on object: ", current->id );
            obj->ops->dump( obj, 0 );
        }
        set_error( STATUS_NOT_IMPLEMENTED );
    }

    release_object( obj );
}

/* Retrieve a file descriptor for an esync object, which will be written to.
 * This should not be used for server-bound objects. */
DECL_HANDLER(get_esync_write_fd)
{
    struct object *obj;
    enum esync_type type;
    struct esync_fd *fd;

    if (!(obj = get_handle_obj( current->process, req->handle, 0, NULL )))
        return;

    if (obj->ops->get_esync_fd)
    {
        fd = obj->ops->get_esync_fd( obj, &type );
#ifdef HAVE_SYS_EVENTFD_H
        fprintf( stderr, "This path shouldn't be reached! ntdll is being stupid.\n" );
        send_client_fd( current->process, fd->fd, req->handle );
#else
        send_client_fd( current->process, fd->fds[1], req->handle );
#endif
    }
    else
        set_error( STATUS_NOT_IMPLEMENTED );

    release_object( obj );
}

/* Return the fd used for waiting on user APCs. */
DECL_HANDLER(get_esync_apc_fd)
{
#ifdef HAVE_SYS_EVENTFD_H
    send_client_fd( current->process, current->esync_apc_fd->fd, current->id );
#else
    send_client_fd( current->process, current->esync_apc_fd->fds[0], current->id );
#endif
}
