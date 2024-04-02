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

#if 0
#pragma makedep unix
#endif

#define _GNU_SOURCE

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#include <poll.h>
#include <sys/types.h>
#include <unistd.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winternl.h"
#include "wine/server.h"
#include "wine/debug.h"

#include "unix_private.h"
#include "esync.h"
#include "msync.h"

WINE_DEFAULT_DEBUG_CHANNEL(esync);

int do_esync(void)
{
    static int do_esync_cached = -1;

    if (do_esync_cached == -1)
        do_esync_cached = getenv("WINEESYNC") && atoi(getenv("WINEESYNC")) && !do_msync();;

    return do_esync_cached;
}

struct esync
{
    enum esync_type type;
#ifdef HAVE_SYS_EVENTFD_H
    int fd;
#else
    int readfd, writefd;
#endif
    int refcount;
    void *shm;
};

/* Emulate read() on an eventfd: clear the object if it's not a semaphore, read
 * 1 from it if it is. Return a nonzero value if the object was signaled. */
static ssize_t efd_read(struct esync *esync)
{
#ifdef HAVE_SYS_EVENTFD_H
    uint64_t value;

    return read( esync->fd, &value, sizeof(value) ) > 0;
#else
    static char buffer[4096];

    if (esync->type == ESYNC_SEMAPHORE)
        return read( esync->readfd, buffer, 1 ) > 0;
    else
    {
        int ret = 0, size;
        do
        {
            size = read( esync->readfd, buffer, sizeof(buffer) );
            if (size > 0) ret = 1;
        } while (size == sizeof(buffer));
        return ret;
    }
#endif
}

/* Emulate write() on an eventfd. Return -1 on failure. */
static ssize_t efd_write(struct esync *esync, uint64_t value)
{
#ifdef HAVE_SYS_EVENTFD_H
    return write( esync->fd, &value, sizeof(value) );
#else
    static const char buffer[4096];

    do
    {
        ssize_t ret = write( esync->writefd, buffer, min(value, sizeof(buffer)) );
        if (ret == -1) return -1;
        value -= ret;
    } while (value);

    return 0;
#endif
}

static void efd_close(struct esync *esync)
{
#ifdef HAVE_SYS_EVENTFD_H
    close( esync->fd );
#else
    close( esync->readfd );
    close( esync->writefd );
#endif
}

static void grab_object( struct esync *obj )
{
    InterlockedIncrement( &obj->refcount );
}

static void release_object( struct esync *obj )
{
    if (!InterlockedDecrement( &obj->refcount ))
    {
        efd_close( obj );
        free( obj );
    }
}

/* Careful how this is used—we can't reliably completely clear an object with it. */
static int get_read_fd( struct esync *obj )
{
    if (!obj) return -1;

#ifdef HAVE_SYS_EVENTFD_H
    return obj->fd;
#else
    return obj->readfd;
#endif
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

static char shm_name[29];
static int shm_fd;
static void **shm_addrs;
static int shm_addrs_size;  /* length of the allocated shm_addrs array */
static long pagesize;

static pthread_mutex_t shm_addrs_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *get_shm( unsigned int idx )
{
    int entry  = (idx * 8) / pagesize;
    int offset = (idx * 8) % pagesize;
    void *ret;

    pthread_mutex_lock( &shm_addrs_mutex );

    if (entry >= shm_addrs_size)
    {
        int new_size = max(shm_addrs_size * 2, entry + 1);

        if (!(shm_addrs = realloc( shm_addrs, new_size * sizeof(shm_addrs[0]) )))
            ERR("Failed to grow shm_addrs array to size %d.\n", shm_addrs_size);
        memset( shm_addrs + shm_addrs_size, 0, (new_size - shm_addrs_size) * sizeof(shm_addrs[0]) );
        shm_addrs_size = new_size;
    }

    if (!shm_addrs[entry])
    {
        void *addr = mmap( NULL, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, entry * pagesize );
        if (addr == (void *)-1)
            ERR("Failed to map page %d (offset %#lx).\n", entry, entry * pagesize);

        TRACE("Mapping page %d at %p.\n", entry, addr);

        if (InterlockedCompareExchangePointer( &shm_addrs[entry], addr, 0 ))
            munmap( addr, pagesize ); /* someone beat us to it */
    }

    ret = (void *)((unsigned long)shm_addrs[entry] + offset);

    pthread_mutex_unlock( &shm_addrs_mutex );

    return ret;
}

/* We'd like lookup to be fast. To that end, we use a static list indexed by handle.
 * This is copied and adapted from the fd cache code. */

#define ESYNC_LIST_BLOCK_SIZE  (65536 / sizeof(struct esync *))
#define ESYNC_LIST_ENTRIES     256

static struct esync * *esync_list[ESYNC_LIST_ENTRIES];
static struct esync * esync_list_initial_block[ESYNC_LIST_BLOCK_SIZE];

static inline UINT_PTR handle_to_index( HANDLE handle, UINT_PTR *entry )
{
    UINT_PTR idx = (((UINT_PTR)handle) >> 2) - 1;
    *entry = idx / ESYNC_LIST_BLOCK_SIZE;
    return idx % ESYNC_LIST_BLOCK_SIZE;
}

static struct esync *add_to_list( HANDLE handle, enum esync_type type, int fd, void *shm )
{
    UINT_PTR entry, idx = handle_to_index( handle, &entry );

    /* We got the read fd, but not the write fd. We only need the latter if
     * we're not dealing with a server-bound object. */

    int writefd = -1;
    sigset_t sigset;
    NTSTATUS ret;
    obj_handle_t fd_handle;
    struct esync *obj;

    if (!(obj = malloc( sizeof(*obj) )))
    {
        ERR("Failed to allocate memory for esync object.\n");
        return NULL;
    }

#ifndef HAVE_SYS_EVENTFD_H
    if (type != ESYNC_MANUAL_SERVER && type != ESYNC_AUTO_SERVER && type != ESYNC_QUEUE)
    {
        server_enter_uninterrupted_section( &fd_cache_mutex, &sigset );

        SERVER_START_REQ( get_esync_write_fd )
        {
            req->handle = wine_server_obj_handle( handle );
            if (!(ret = wine_server_call( req )))
            {
                writefd = receive_fd( &fd_handle );
                assert( wine_server_ptr_handle(fd_handle) == handle );
            }
        }
        SERVER_END_REQ;

        server_leave_uninterrupted_section( &fd_cache_mutex, &sigset );
    }
#endif

    if (entry >= ESYNC_LIST_ENTRIES)
    {
        FIXME( "too many allocated handles, not caching %p\n", handle );
        return FALSE;
    }

    if (!esync_list[entry])  /* do we need to allocate a new block of entries? */
    {
        if (!entry) esync_list[0] = esync_list_initial_block;
        else
        {
            void *ptr = anon_mmap_alloc( ESYNC_LIST_BLOCK_SIZE * sizeof(struct esync),
                                         PROT_READ | PROT_WRITE );
            if (ptr == MAP_FAILED) return FALSE;
            esync_list[entry] = ptr;
        }
    }

    obj->type = type;
#ifdef HAVE_SYS_EVENTFD_H
    obj->fd = fd;
#else
    obj->readfd = fd;
    obj->writefd = writefd;
#endif
    obj->refcount = 1;
    obj->shm = shm;

    assert(!InterlockedExchangePointer((void **)&esync_list[entry][idx], obj));
    return obj;
}

static struct esync *get_cached_object( HANDLE handle )
{
    UINT_PTR entry, idx = handle_to_index( handle, &entry );

    if (entry >= ESYNC_LIST_ENTRIES || !esync_list[entry]) return NULL;

    return esync_list[entry][idx];
}

/* Gets an object. This is either a proper esync object (i.e. an event,
 * semaphore, etc. created using create_esync) or a generic synchronizable
 * server-side object which the server will signal (e.g. a process, thread,
 * message queue, etc.) */
static NTSTATUS get_object( HANDLE handle, struct esync **obj )
{
    NTSTATUS ret = STATUS_SUCCESS;
    enum esync_type type = 0;
    unsigned int shm_idx = 0;
    obj_handle_t fd_handle;
    sigset_t sigset;
    int fd = -1;

    if ((*obj = get_cached_object( handle ))) return STATUS_SUCCESS;

    if ((INT_PTR)handle < 0)
    {
        /* We can deal with pseudo-handles, but it's just easier this way */
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!handle)
    {
        /* Shadow of the Tomb Raider really likes passing in NULL handles to
         * various functions. Concerning, but let's avoid a server call. */
        return STATUS_INVALID_HANDLE;
    }

    /* We need to try grabbing it from the server. */
    server_enter_uninterrupted_section( &fd_cache_mutex, &sigset );
    if (!(*obj = get_cached_object( handle )))
    {
        SERVER_START_REQ( get_esync_read_fd )
        {
            req->handle = wine_server_obj_handle( handle );
            if (!(ret = wine_server_call( req )))
            {
                type = reply->type;
                shm_idx = reply->shm_idx;
                fd = receive_fd( &fd_handle );
                assert( wine_server_ptr_handle(fd_handle) == handle );
            }
        }
        SERVER_END_REQ;
    }
    server_leave_uninterrupted_section( &fd_cache_mutex, &sigset );

    if (*obj)
    {
        /* We managed to grab it while in the CS; return it. */
        return STATUS_SUCCESS;
    }

    if (ret)
    {
        WARN("Failed to retrieve fd for handle %p, status %#x.\n", handle, ret);
        *obj = NULL;
        return ret;
    }

    TRACE("Got fd %d for handle %p.\n", fd, handle);

    *obj = add_to_list( handle, type, fd, shm_idx ? get_shm( shm_idx ) : 0 );
    return ret;
}

NTSTATUS esync_close( HANDLE handle )
{
    UINT_PTR entry, idx = handle_to_index( handle, &entry );
    struct esync *obj;

    TRACE("%p.\n", handle);

    if (entry < ESYNC_LIST_ENTRIES && esync_list[entry])
    {
        if ((obj = InterlockedExchangePointer((void **)&esync_list[entry][idx], 0)))
        {
            release_object( obj );
            return STATUS_SUCCESS;
        }
    }

    return STATUS_INVALID_HANDLE;
}

static NTSTATUS create_esync( enum esync_type type, HANDLE *handle, ACCESS_MASK access,
                              const OBJECT_ATTRIBUTES *attr, int initval, int max )
{
    NTSTATUS ret;
    data_size_t len;
    struct object_attributes *objattr;
    obj_handle_t fd_handle;
    unsigned int shm_idx;
    sigset_t sigset;
    int fd;

    if ((ret = alloc_object_attributes( attr, &objattr, &len ))) return ret;

    /* We have to synchronize on the fd cache CS so that our calls to
     * receive_fd don't race with theirs. */
    server_enter_uninterrupted_section( &fd_cache_mutex, &sigset );
    SERVER_START_REQ( create_esync )
    {
        req->access  = access;
        req->initval = initval;
        req->type    = type;
        req->max     = max;
        wine_server_add_data( req, objattr, len );
        ret = wine_server_call( req );
        if (!ret || ret == STATUS_OBJECT_NAME_EXISTS)
        {
            *handle = wine_server_ptr_handle( reply->handle );
            type = reply->type;
            shm_idx = reply->shm_idx;
            fd = receive_fd( &fd_handle );
            assert( wine_server_ptr_handle(fd_handle) == *handle );
        }
    }
    SERVER_END_REQ;
    server_leave_uninterrupted_section( &fd_cache_mutex, &sigset );

    if (!ret || ret == STATUS_OBJECT_NAME_EXISTS)
    {
        add_to_list( *handle, type, fd, shm_idx ? get_shm( shm_idx ) : 0 );
        TRACE("-> handle %p, fd %d.\n", *handle, fd);
    }

    free( objattr );
    return ret;
}

static NTSTATUS open_esync( enum esync_type type, HANDLE *handle,
    ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS ret;
    obj_handle_t fd_handle;
    unsigned int shm_idx;
    sigset_t sigset;
    int fd;

    server_enter_uninterrupted_section( &fd_cache_mutex, &sigset );
    SERVER_START_REQ( open_esync )
    {
        req->access     = access;
        req->attributes = attr->Attributes;
        req->rootdir    = wine_server_obj_handle( attr->RootDirectory );
        req->type       = type;
        if (attr->ObjectName)
            wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        if (!(ret = wine_server_call( req )))
        {
            *handle = wine_server_ptr_handle( reply->handle );
            type = reply->type;
            shm_idx = reply->shm_idx;
            fd = receive_fd( &fd_handle );
            assert( wine_server_ptr_handle(fd_handle) == *handle );
        }
    }
    SERVER_END_REQ;
    server_leave_uninterrupted_section( &fd_cache_mutex, &sigset );

    if (!ret)
    {
        add_to_list( *handle, type, fd, shm_idx ? get_shm( shm_idx ) : 0 );

        TRACE("-> handle %p, fd %d.\n", *handle, fd);
    }
    return ret;
}

extern NTSTATUS esync_create_semaphore(HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr, LONG initial, LONG max)
{
    TRACE("name %s, initial %d, max %d.\n",
        attr ? debugstr_us(attr->ObjectName) : "<no name>", initial, max);

    return create_esync( ESYNC_SEMAPHORE, handle, access, attr, initial, max );
}

NTSTATUS esync_open_semaphore( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr )
{
    TRACE("name %s.\n", debugstr_us(attr->ObjectName));

    return open_esync( ESYNC_SEMAPHORE, handle, access, attr );
}

NTSTATUS esync_release_semaphore( HANDLE handle, ULONG count, ULONG *prev )
{
    struct esync *obj;
    struct semaphore *semaphore;
    uint64_t count64 = count;
    ULONG current;
    NTSTATUS ret;

    TRACE("%p, %d, %p.\n", handle, count, prev);

    if ((ret = get_object( handle, &obj))) return ret;
    semaphore = obj->shm;

    do
    {
        current = semaphore->count;

        if (count + current > semaphore->max)
            return STATUS_SEMAPHORE_LIMIT_EXCEEDED;
    } while (InterlockedCompareExchange( &semaphore->count, count + current, current ) != current);

    if (prev) *prev = current;

    /* We don't have to worry about a race between increasing the count and
     * write(). The fact that we were able to increase the count means that we
     * have permission to actually write that many releases to the semaphore. */

    if (efd_write( obj, count64 ) == -1)
    {
        FIXME("write() failed: %s\n", strerror(errno));
        return errno_to_status( errno );
    }

    return STATUS_SUCCESS;
}

NTSTATUS esync_query_semaphore( HANDLE handle, void *info, ULONG *ret_len )
{
    struct esync *obj;
    struct semaphore *semaphore;
    SEMAPHORE_BASIC_INFORMATION *out = info;
    NTSTATUS ret;

    TRACE("handle %p, info %p, ret_len %p.\n", handle, info, ret_len);

    if ((ret = get_object( handle, &obj ))) return ret;
    semaphore = obj->shm;

    out->CurrentCount = semaphore->count;
    out->MaximumCount = semaphore->max;
    if (ret_len) *ret_len = sizeof(*out);

    return STATUS_SUCCESS;
}

NTSTATUS esync_create_event( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr, EVENT_TYPE event_type, BOOLEAN initial )
{
    enum esync_type type = (event_type == SynchronizationEvent ? ESYNC_AUTO_EVENT : ESYNC_MANUAL_EVENT);

    TRACE("name %s, %s-reset, initial %d.\n",
        attr ? debugstr_us(attr->ObjectName) : "<no name>",
        event_type == NotificationEvent ? "manual" : "auto", initial);

    return create_esync( type, handle, access, attr, initial, 0 );
}

NTSTATUS esync_open_event( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr )
{
    TRACE("name %s.\n", debugstr_us(attr->ObjectName));

    return open_esync( ESYNC_AUTO_EVENT, handle, access, attr ); /* doesn't matter which */
}

static inline void small_pause(void)
{
#ifdef __i386__
    __asm__ __volatile__( "rep;nop" : : : "memory" );
#else
    __asm__ __volatile__( "" : : : "memory" );
#endif
}

/* Manual-reset events are actually racier than other objects in terms of shm
 * state. With other objects, races don't matter, because we only treat the shm
 * state as a hint that lets us skip poll()—we still have to read(). But with
 * manual-reset events we don't, which means that the shm state can be out of
 * sync with the actual state.
 *
 * In general we shouldn't have to worry about races between modifying the
 * event and waiting on it. If the state changes while we're waiting, it's
 * equally plausible that we caught it before or after the state changed.
 * However, we can have races between SetEvent() and ResetEvent(), so that the
 * event has inconsistent internal state.
 *
 * To solve this we have to use the other field to lock the event. Currently
 * this is implemented as a spinlock, but I'm not sure if a futex might be
 * better. I'm also not sure if it's possible to obviate locking by arranging
 * writes and reads in a certain way.
 *
 * Note that we don't have to worry about locking in esync_wait_objects().
 * There's only two general patterns:
 *
 * WaitFor()    SetEvent()
 * -------------------------
 * read()
 * signaled = 0
 *              signaled = 1
 *              write()
 * -------------------------
 * read()
 *              signaled = 1
 * signaled = 0
 *              <no write(), because it was already signaled>
 * -------------------------
 *
 * That is, if SetEvent() tries to signal the event before WaitFor() resets its
 * signaled state, it won't bother trying to write(), and then the signaled
 * state will be reset, so the result is a consistent non-signaled event.
 * There's several variations to this pattern but all of them are protected in
 * the same way. Note however this is why we have to use interlocked_xchg()
 * event inside of the lock.
 */

/* Removing this spinlock is harder than it looks. esync_wait_objects() can
 * deal with inconsistent state well enough, and a race between SetEvent() and
 * ResetEvent() gives us license to yield either result as long as we act
 * consistently, but that's not enough. Notably, esync_wait_objects() should
 * probably act like a fence, so that the second half of esync_set_event() does
 * not seep past a subsequent reset. That's one problem, but no guarantee there
 * aren't others. */

NTSTATUS esync_set_event( HANDLE handle )
{
    struct esync *obj;
    struct event *event;
    NTSTATUS ret;

    TRACE("%p.\n", handle);

    if ((ret = get_object( handle, &obj ))) return ret;
    event = obj->shm;

    if (obj->type != ESYNC_MANUAL_EVENT && obj->type != ESYNC_AUTO_EVENT)
        return STATUS_OBJECT_TYPE_MISMATCH;

    if (obj->type == ESYNC_MANUAL_EVENT)
    {
        /* Acquire the spinlock. */
        while (InterlockedCompareExchange( &event->locked, 1, 0 ))
            small_pause();
    }

    /* For manual-reset events, as long as we're in a lock, we can take the
     * optimization of only calling write() if the event wasn't already
     * signaled.
     *
     * For auto-reset events, esync_wait_objects() must grab the kernel object.
     * Thus if we got into a race so that the shm state is signaled but the
     * eventfd is unsignaled (i.e. reset shm, set shm, set fd, reset fd), we
     * *must* signal the fd now, or any waiting threads will never wake up. */

    if (!InterlockedExchange( &event->signaled, 1 ) || obj->type == ESYNC_AUTO_EVENT)
    {
        /* We don't need to do anything if we got EAGAIN. */
        if (efd_write( obj, 1 ) == -1 && errno != EAGAIN)
            ERR("write: %s\n", strerror(errno));
    }

    if (obj->type == ESYNC_MANUAL_EVENT)
    {
        /* Release the spinlock. */
        event->locked = 0;
    }

    return STATUS_SUCCESS;
}

NTSTATUS esync_reset_event( HANDLE handle )
{
    struct esync *obj;
    struct event *event;
    NTSTATUS ret;

    TRACE("%p.\n", handle);

    if ((ret = get_object( handle, &obj ))) return ret;
    event = obj->shm;

    if (obj->type == ESYNC_MANUAL_EVENT)
    {
        /* Acquire the spinlock. */
        while (InterlockedCompareExchange( &event->locked, 1, 0 ))
            small_pause();
    }

    /* For manual-reset events, as long as we're in a lock, we can take the
     * optimization of only calling read() if the event was already signaled.
     *
     * For auto-reset events, we have no guarantee that the previous "signaled"
     * state is actually correct. We need to leave both states unsignaled after
     * leaving this function, so we always have to read(). */
    if (InterlockedExchange( &event->signaled, 0 ) || obj->type == ESYNC_AUTO_EVENT)
    {
        efd_read( obj );
    }

    if (obj->type == ESYNC_MANUAL_EVENT)
    {
        /* Release the spinlock. */
        event->locked = 0;
    }

    return STATUS_SUCCESS;
}

NTSTATUS esync_pulse_event( HANDLE handle )
{
    struct esync *obj;
    NTSTATUS ret;

    TRACE("%p.\n", handle);

    if ((ret = get_object( handle, &obj ))) return ret;

    /* This isn't really correct; an application could miss the write.
     * Unfortunately we can't really do much better. Fortunately this is rarely
     * used (and publicly deprecated). */
    if (efd_write( obj, 1 ) == -1 && errno != EAGAIN)
        return errno_to_status( errno );

    /* Try to give other threads a chance to wake up. Hopefully erring on this
     * side is the better thing to do... */
    NtYieldExecution();

    efd_read( obj );

    return STATUS_SUCCESS;
}

NTSTATUS esync_query_event( HANDLE handle, void *info, ULONG *ret_len )
{
    struct esync *obj;
    EVENT_BASIC_INFORMATION *out = info;
    struct pollfd fd;
    NTSTATUS ret;

    TRACE("handle %p, info %p, ret_len %p.\n", handle, info, ret_len);

    if ((ret = get_object( handle, &obj ))) return ret;

    fd.fd = get_read_fd( obj );
    fd.events = POLLIN;
    out->EventState = poll( &fd, 1, 0 );
    out->EventType = (obj->type == ESYNC_AUTO_EVENT ? SynchronizationEvent : NotificationEvent);
    if (ret_len) *ret_len = sizeof(*out);

    return STATUS_SUCCESS;
}

NTSTATUS esync_create_mutex( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr, BOOLEAN initial )
{
    TRACE("name %s, initial %d.\n",
        attr ? debugstr_us(attr->ObjectName) : "<no name>", initial);

    return create_esync( ESYNC_MUTEX, handle, access, attr, initial ? 0 : 1, 0 );
}

NTSTATUS esync_open_mutex( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr )
{
    TRACE("name %s.\n", debugstr_us(attr->ObjectName));

    return open_esync( ESYNC_MUTEX, handle, access, attr );
}

NTSTATUS esync_release_mutex( HANDLE *handle, LONG *prev )
{
    struct esync *obj;
    struct mutex *mutex;
    NTSTATUS ret;

    TRACE("%p, %p.\n", handle, prev);

    if ((ret = get_object( handle, &obj ))) return ret;
    mutex = obj->shm;

    /* This is thread-safe, because the only thread that can change the tid to
     * or from our tid is ours. */
    if (mutex->tid != GetCurrentThreadId()) return STATUS_MUTANT_NOT_OWNED;

    if (prev) *prev = mutex->count;

    mutex->count--;

    if (!mutex->count)
    {
        /* This is also thread-safe, as long as signaling the file is the last
         * thing we do. Other threads don't care about the tid if it isn't
         * theirs. */
        mutex->tid = 0;

        if (efd_write( obj, 1 ) == -1 && errno != EAGAIN)
            return errno_to_status( errno );
    }

    return STATUS_SUCCESS;
}

NTSTATUS esync_query_mutex( HANDLE handle, void *info, ULONG *ret_len )
{
    struct esync *obj;
    struct mutex *mutex;
    MUTANT_BASIC_INFORMATION *out = info;
    NTSTATUS ret;

    TRACE("handle %p, info %p, ret_len %p.\n", handle, info, ret_len);

    if ((ret = get_object( handle, &obj ))) return ret;
    mutex = obj->shm;

    out->CurrentCount = 1 - mutex->count;
    out->OwnedByCaller = (mutex->tid == GetCurrentThreadId());
    out->AbandonedState = (mutex->tid == ~0);
    if (ret_len) *ret_len = sizeof(*out);

    return STATUS_SUCCESS;
}

#define TICKSPERSEC        10000000
#define TICKSPERMSEC       10000

static LONGLONG update_timeout( ULONGLONG end )
{
    LARGE_INTEGER now;
    LONGLONG timeleft;

    NtQuerySystemTime( &now );
    timeleft = end - now.QuadPart;
    if (timeleft < 0) timeleft = 0;
    return timeleft;
}

static int do_poll( struct pollfd *fds, nfds_t nfds, ULONGLONG *end )
{
    int ret;

    do
    {
        if (end)
        {
            LONGLONG timeleft = update_timeout( *end );

#ifdef HAVE_PPOLL
            /* We use ppoll() if available since the time granularity is better. */
            struct timespec tmo_p;
            tmo_p.tv_sec = timeleft / (ULONGLONG)TICKSPERSEC;
            tmo_p.tv_nsec = (timeleft % TICKSPERSEC) * 100;
            ret = ppoll( fds, nfds, &tmo_p, NULL );
#else
            ret = poll( fds, nfds, timeleft / TICKSPERMSEC );
#endif
        }
        else
            ret = poll( fds, nfds, -1 );

    /* If we receive EINTR we were probably suspended (SIGUSR1), possibly for a
     * system APC. The right thing to do is just try again. */
    } while (ret < 0 && errno == EINTR);

    return ret;
}

/* Return TRUE if abandoned. */
static BOOL update_grabbed_object( struct esync *obj )
{
    BOOL ret = FALSE;

    if (obj->type == ESYNC_MUTEX)
    {
        struct mutex *mutex = obj->shm;
        /* We don't have to worry about a race between this and read(); the
         * fact that we grabbed it means the count is now zero, so nobody else
         * can (and the only thread that can release it is us). */
        if (mutex->tid == ~0)
            ret = TRUE;
        mutex->tid = GetCurrentThreadId();
        mutex->count++;
    }
    else if (obj->type == ESYNC_SEMAPHORE)
    {
        struct semaphore *semaphore = obj->shm;
        /* We don't have to worry about a race between this and read(); the
         * fact that we were able to grab it at all means the count is nonzero,
         * and if someone else grabbed it then the count must have been >= 2,
         * etc. */
        InterlockedExchangeAdd( &semaphore->count, -1 );
    }
    else if (obj->type == ESYNC_AUTO_EVENT)
    {
        struct event *event = obj->shm;
        /* We don't have to worry about a race between this and read(), since
         * this is just a hint, and the real state is in the kernel object.
         * This might already be 0, but that's okay! */
        event->signaled = 0;
    }

    return ret;
}

/* A value of STATUS_NOT_IMPLEMENTED returned from this function means that we
 * need to delegate to server_select(). */
static NTSTATUS __esync_wait_objects( DWORD count, const HANDLE *handles, BOOLEAN wait_any,
                             BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    static const LARGE_INTEGER zero;

    struct esync *objs[MAXIMUM_WAIT_OBJECTS];
    struct pollfd fds[MAXIMUM_WAIT_OBJECTS + 1];
    int has_esync = 0, has_server = 0;
    BOOL msgwait = FALSE;
    LONGLONG timeleft;
    LARGE_INTEGER now;
    NTSTATUS status;
    DWORD pollcount;
    ULONGLONG end;
    int i, j, ret;

    /* Grab the APC fd if we don't already have it. */
    if (alertable && ntdll_get_thread_data()->esync_apc_fd == -1)
    {
        obj_handle_t fd_handle;
        sigset_t sigset;
        int fd = -1;

        server_enter_uninterrupted_section( &fd_cache_mutex, &sigset );
        SERVER_START_REQ( get_esync_apc_fd )
        {
            if (!(ret = wine_server_call( req )))
            {
                fd = receive_fd( &fd_handle );
                assert( fd_handle == GetCurrentThreadId() );
            }
        }
        SERVER_END_REQ;
        server_leave_uninterrupted_section( &fd_cache_mutex, &sigset );

        ntdll_get_thread_data()->esync_apc_fd = fd;
    }

    NtQuerySystemTime( &now );
    if (timeout)
    {
        if (timeout->QuadPart == TIMEOUT_INFINITE)
            timeout = NULL;
        else if (timeout->QuadPart >= 0)
            end = timeout->QuadPart;
        else
            end = now.QuadPart - timeout->QuadPart;
    }

    for (i = 0; i < count; i++)
    {
        ret = get_object( handles[i], &objs[i] );
        if (ret == STATUS_SUCCESS)
            has_esync = 1;
        else if (ret == STATUS_NOT_IMPLEMENTED)
            has_server = 1;
        else
            return ret;
    }

    if (count && objs[count - 1] && objs[count - 1]->type == ESYNC_QUEUE)
        msgwait = TRUE;

    if (has_esync && has_server)
        FIXME("Can't wait on esync and server objects at the same time!\n");
    else if (has_server)
        return STATUS_NOT_IMPLEMENTED;

    if (TRACE_ON(esync))
    {
        TRACE("Waiting for %s of %d handles:", wait_any ? "any" : "all", count);
        for (i = 0; i < count; i++)
            TRACE(" %p", handles[i]);

        if (msgwait)
            TRACE(" or driver events");
        if (alertable)
            TRACE(", alertable");

        if (!timeout)
            TRACE(", timeout = INFINITE.\n");
        else
        {
            timeleft = update_timeout( end );
            TRACE(", timeout = %ld.%07ld sec.\n",
                (long) timeleft / TICKSPERSEC, (long) timeleft % TICKSPERSEC);
        }
    }

    for (i = 0; i < count; i++)
    {
        if (objs[i])
            grab_object( objs[i] );
    }

    if (wait_any || count <= 1)
    {
        /* Try to check objects now, so we can obviate poll() at least. */
        for (i = 0; i < count; i++)
        {
            struct esync *obj = objs[i];

            if (obj)
            {
                switch (obj->type)
                {
                case ESYNC_MUTEX:
                {
                    struct mutex *mutex = obj->shm;

                    if (mutex->tid == GetCurrentThreadId())
                    {
                        TRACE("Woken up by handle %p [%d].\n", handles[i], i);
                        mutex->count++;
                        status = i;
                        goto done;
                    }
                    else if (!mutex->count)
                    {
                        if (efd_read( obj ))
                        {
                            if (mutex->tid == ~0)
                            {
                                TRACE("Woken up by abandoned mutex %p [%d].\n", handles[i], i);
                                i += STATUS_ABANDONED_WAIT_0;
                            }
                            else
                                TRACE("Woken up by handle %p [%d].\n", handles[i], i);
                            mutex->tid = GetCurrentThreadId();
                            mutex->count++;
                            status = i;
                            goto done;
                        }
                    }
                    break;
                }
                case ESYNC_SEMAPHORE:
                {
                    struct semaphore *semaphore = obj->shm;

                    if (semaphore->count)
                    {
                        if (efd_read( obj ))
                        {
                            TRACE("Woken up by handle %p [%d].\n", handles[i], i);
                            InterlockedDecrement( &semaphore->count );
                            status = i;
                            goto done;
                        }
                    }
                    break;
                }
                case ESYNC_AUTO_EVENT:
                {
                    struct event *event = obj->shm;

                    if (event->signaled)
                    {
                        if (efd_read( obj ))
                        {
                            TRACE("Woken up by handle %p [%d].\n", handles[i], i);
                            event->signaled = 0;
                            status = i;
                            goto done;
                        }
                    }
                    break;
                }
                case ESYNC_MANUAL_EVENT:
                {
                    struct event *event = obj->shm;

                    if (event->signaled)
                    {
                        TRACE("Woken up by handle %p [%d].\n", handles[i], i);
                        status = i;
                        goto done;
                    }
                    break;
                }
                case ESYNC_AUTO_SERVER:
                case ESYNC_MANUAL_SERVER:
                case ESYNC_QUEUE:
                    /* We can't wait on any of these. Fortunately I don't think
                     * they'll ever be uncontended anyway (at least, they won't be
                     * performance-critical). */
                    break;
                }
            }

            fds[i].fd = get_read_fd( obj );
            fds[i].events = POLLIN;
        }
        if (alertable)
        {
            fds[i].fd = ntdll_get_thread_data()->esync_apc_fd;
            fds[i].events = POLLIN;
            i++;
        }
        pollcount = i;

        while (1)
        {
            ret = do_poll( fds, pollcount, timeout ? &end : NULL );
            if (ret > 0)
            {
                /* We must check this first! The server may set an event that
                 * we're waiting on, but we need to return STATUS_USER_APC. */
                if (alertable)
                {
                    if (fds[pollcount - 1].revents & POLLIN)
                        goto userapc;
                }

                /* Find out which object triggered the wait. */
                for (i = 0; i < count; i++)
                {
                    struct esync *obj = objs[i];

                    if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
                    {
                        ERR("Polling on fd %d returned %#x.\n", fds[i].fd, fds[i].revents);
                        status = STATUS_INVALID_HANDLE;
                        goto done;
                    }

                    if (obj)
                    {
                        if (obj->type == ESYNC_MANUAL_EVENT
                                || obj->type == ESYNC_MANUAL_SERVER
                                || obj->type == ESYNC_QUEUE)
                        {
                            /* Don't grab the object, just check if it's signaled. */
                            if (fds[i].revents & POLLIN)
                            {
                                TRACE("Woken up by handle %p [%d].\n", handles[i], i);
                                status = i;
                                goto done;
                            }
                        }
                        else
                        {
                            if (efd_read( obj ))
                            {
                                /* We found our object. */
                                TRACE("Woken up by handle %p [%d].\n", handles[i], i);
                                status = i;
                                if (update_grabbed_object( obj ))
                                    status += STATUS_ABANDONED_WAIT_0;
                                goto done;
                            }
                        }
                    }
                }

                /* If we got here, someone else stole (or reset, etc.) whatever
                 * we were waiting for. So keep waiting. */
                NtQuerySystemTime( &now );
            }
            else
                goto err;
        }
    }
    else
    {
        /* Wait-all is a little trickier to implement correctly. Fortunately,
         * it's not as common.
         *
         * The idea is basically just to wait in sequence on every object in the
         * set. Then when we're done, try to grab them all in a tight loop. If
         * that fails, release any resources we've grabbed (and yes, we can
         * reliably do this—it's just mutexes and semaphores that we have to
         * put back, and in both cases we just put back 1), and if any of that
         * fails we start over.
         *
         * What makes this inherently bad is that we might temporarily grab a
         * resource incorrectly. Hopefully it'll be quick (and hey, it won't
         * block on wineserver) so nobody will notice. Besides, consider: if
         * object A becomes signaled but someone grabs it before we can grab it
         * and everything else, then they could just as well have grabbed it
         * before it became signaled. Similarly if object A was signaled and we
         * were blocking on object B, then B becomes available and someone grabs
         * A before we can, then they might have grabbed A before B became
         * signaled. In either case anyone who tries to wait on A or B will be
         * waiting for an instant while we put things back. */

        while (1)
        {
tryagain:
            /* First step: try to poll on each object in sequence. */
            fds[0].events = POLLIN;
            pollcount = 1;
            if (alertable)
            {
                /* We also need to wait on APCs. */
                fds[1].fd = ntdll_get_thread_data()->esync_apc_fd;
                fds[1].events = POLLIN;
                pollcount++;
            }
            for (i = 0; i < count; i++)
            {
                struct esync *obj = objs[i];

                fds[0].fd = get_read_fd( obj );

                if (obj && obj->type == ESYNC_MUTEX)
                {
                    /* It might be ours. */
                    struct mutex *mutex = obj->shm;

                    if (mutex->tid == GetCurrentThreadId())
                        continue;
                }

                ret = do_poll( fds, pollcount, timeout ? &end : NULL );
                if (ret <= 0)
                    goto err;
                else if (alertable && (fds[1].revents & POLLIN))
                    goto userapc;

                if (fds[0].revents & (POLLHUP | POLLERR | POLLNVAL))
                {
                    ERR("Polling on fd %d returned %#x.\n", fds[0].fd, fds[0].revents);
                    status = STATUS_INVALID_HANDLE;
                    goto done;
                }
            }

            /* If we got here and we haven't timed out, that means all of the
             * handles were signaled. Check to make sure they still are. */
            for (i = 0; i < count; i++)
            {
                fds[i].fd = get_read_fd( objs[i] );
                fds[i].events = POLLIN;
            }
            /* There's no reason to check for APCs here. */
            pollcount = i;

            /* Poll everything to see if they're still signaled. */
            ret = poll( fds, pollcount, 0 );
            if (ret == pollcount)
            {
                BOOL abandoned = FALSE;

                /* Quick, grab everything. */
                for (i = 0; i < count; i++)
                {
                    struct esync *obj = objs[i];

                    switch (obj->type)
                    {
                    case ESYNC_MUTEX:
                    {
                        struct mutex *mutex = obj->shm;
                        if (mutex->tid == GetCurrentThreadId())
                            break;
                        /* otherwise fall through */
                    }
                    case ESYNC_SEMAPHORE:
                    case ESYNC_AUTO_EVENT:
                        if (!efd_read( obj ))
                        {
                            /* We were too slow. Put everything back. */
                            for (j = i; j >= 0; j--)
                            {
                                if (efd_write( obj, 1 ) == -1 && errno != EAGAIN)
                                {
                                    status = errno_to_status( errno );
                                    goto done;
                                }
                            }

                            goto tryagain;  /* break out of two loops and a switch */
                        }
                        break;
                    default:
                        /* If a manual-reset event changed between there and
                         * here, it's shouldn't be a problem. */
                        break;
                    }
                }

                /* If we got here, we successfully waited on every object. */
                /* Make sure to let ourselves know that we grabbed the mutexes
                 * and semaphores. */
                for (i = 0; i < count; i++)
                    abandoned |= update_grabbed_object( objs[i] );

                if (abandoned)
                {
                    TRACE("Wait successful, but some object(s) were abandoned.\n");
                    status = STATUS_ABANDONED;
                    goto done;
                }
                TRACE("Wait successful.\n");
                status = STATUS_SUCCESS;
                goto done;
            }

            /* If we got here, ppoll() returned less than all of our objects.
             * So loop back to the beginning and try again. */
        } /* while(1) */
    } /* else (wait-all) */

err:
    /* We should only get here if poll() failed. */

    if (ret == 0)
    {
        TRACE("Wait timed out.\n");
        status = STATUS_TIMEOUT;
        goto done;
    }
    else
    {
        ERR("ppoll failed: %s\n", strerror(errno));
        status = errno_to_status( errno );
        goto done;
    }

userapc:
    TRACE("Woken up by user APC.\n");

    /* We have to make a server call anyway to get the APC to execute, so just
     * delegate down to server_select(). */
    status = server_wait( NULL, 0, SELECT_INTERRUPTIBLE | SELECT_ALERTABLE, &zero );

    /* This can happen if we received a system APC, and the APC fd was woken up
     * before we got SIGUSR1. poll() doesn't return EINTR in that case. The
     * right thing to do seems to be to return STATUS_USER_APC anyway. */
    if (status == STATUS_TIMEOUT) status = STATUS_USER_APC;

done:
    for (i = 0; i < count; i++)
    {
        if (objs[i])
            release_object( objs[i] );
    }
    return status;
}

/* We need to let the server know when we are doing a message wait, and when we
 * are done with one, so that all of the code surrounding hung queues works.
 * We also need this for WaitForInputIdle(). */
static void server_set_msgwait( int in_msgwait )
{
    SERVER_START_REQ( esync_msgwait )
    {
        req->in_msgwait = in_msgwait;
        wine_server_call( req );
    }
    SERVER_END_REQ;
}

/* This is a very thin wrapper around the proper implementation above. The
 * purpose is to make sure the server knows when we are doing a message wait.
 * This is separated into a wrapper function since there are at least a dozen
 * exit paths from esync_wait_objects(). */
NTSTATUS esync_wait_objects( DWORD count, const HANDLE *handles, BOOLEAN wait_any,
                             BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    BOOL msgwait = FALSE;
    struct esync *obj;
    NTSTATUS ret;

    if (count && !get_object( handles[count - 1], &obj ) && obj->type == ESYNC_QUEUE)
    {
        msgwait = TRUE;
        server_set_msgwait( 1 );
    }

    ret = __esync_wait_objects( count, handles, wait_any, alertable, timeout );

    if (msgwait)
        server_set_msgwait( 0 );

    return ret;
}

NTSTATUS esync_signal_and_wait( HANDLE signal, HANDLE wait, BOOLEAN alertable,
    const LARGE_INTEGER *timeout )
{
    struct esync *obj;
    NTSTATUS ret;

    if ((ret = get_object( signal, &obj ))) return ret;

    switch (obj->type)
    {
    case ESYNC_SEMAPHORE:
        ret = esync_release_semaphore( signal, 1, NULL );
        break;
    case ESYNC_AUTO_EVENT:
    case ESYNC_MANUAL_EVENT:
        ret = esync_set_event( signal );
        break;
    case ESYNC_MUTEX:
        ret = esync_release_mutex( signal, NULL );
        break;
    default:
        return STATUS_OBJECT_TYPE_MISMATCH;
    }
    if (ret) return ret;

    return esync_wait_objects( 1, &wait, TRUE, alertable, timeout );
}

void esync_init(void)
{
    struct stat st;

    if (!do_esync())
    {
        /* make sure the server isn't running with WINEESYNC */
        HANDLE handle;
        NTSTATUS ret;

        ret = create_esync( 0, &handle, 0, NULL, 0, 0 );
        if (ret != STATUS_NOT_IMPLEMENTED)
        {
            ERR("Server is running with WINEESYNC but this process is not, please enable WINEESYNC or restart wineserver.\n");
            exit(1);
        }

        return;
    }

    if (stat( config_dir, &st ) == -1)
        ERR("Cannot stat %s\n", config_dir);

    if (st.st_ino != (unsigned long)st.st_ino)
        sprintf( shm_name, "/wine-%lx%08lx-esync", (unsigned long)((unsigned long long)st.st_ino >> 32), (unsigned long)st.st_ino );
    else
        sprintf( shm_name, "/wine-%lx-esync", (unsigned long)st.st_ino );

    if ((shm_fd = shm_open( shm_name, O_RDWR, 0644 )) == -1)
    {
        /* probably the server isn't running with WINEESYNC, tell the user and bail */
        if (errno == ENOENT)
            ERR("Failed to open esync shared memory file; make sure no stale wineserver instances are running without WINEESYNC.\n");
        else
            ERR("Failed to initialize shared memory: %s\n", strerror( errno ));
        exit(1);
    }

    pagesize = sysconf( _SC_PAGESIZE );

    shm_addrs = calloc( 128, sizeof(shm_addrs[0]) );
    shm_addrs_size = 128;
}
