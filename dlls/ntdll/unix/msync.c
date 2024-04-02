/*
 * mach semaphore-based synchronization objects
 *
 * Copyright (C) 2018 Zebediah Figura
 * Copyright (C) 2023 Marc-Aurel Zent
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

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
# include <sys/syscall.h>
#endif
#ifdef __APPLE__
# include <mach/mach_init.h>
# include <mach/mach_port.h>
# include <mach/message.h>
# include <mach/port.h>
# include <mach/task.h>
# include <mach/semaphore.h>
# include <mach/mach_error.h>
# include <servers/bootstrap.h>
# include <os/lock.h>
#endif
#include <dlfcn.h>
#include <sched.h>
#include <unistd.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"
#include "wine/server.h"

#include "unix_private.h"
#include "msync.h"

WINE_DEFAULT_DEBUG_CHANNEL(msync);

static LONGLONG update_timeout( ULONGLONG end )
{
    LARGE_INTEGER now;
    LONGLONG timeleft;

    NtQuerySystemTime( &now );
    timeleft = end - now.QuadPart;
    if (timeleft < 0) timeleft = 0;
    return timeleft;
}

static inline mach_timespec_t convert_to_mach_time( LONGLONG win32_time )
{
    mach_timespec_t ret;

    ret.tv_sec = win32_time / (ULONGLONG)TICKSPERSEC;
    ret.tv_nsec = (win32_time % TICKSPERSEC) * 100;
    return ret;
}

#define UL_COMPARE_AND_WAIT_SHARED  0x3
#define ULF_WAKE_ALL                0x00000100
#define ULF_NO_ERRNO                0x01000000
extern int __ulock_wake( uint32_t operation, void *addr, uint64_t wake_value );

typedef int (*__ulock_wait2_ptr_t)( uint32_t operation, void *addr, uint64_t value,
                                    uint64_t timeout_ns, uint64_t value2 );
static __ulock_wait2_ptr_t __ulock_wait2;

/*
 * Faster to directly do the syscall and inline everything, taken and slightly adapted
 * from xnu/libsyscall/mach/mach_msg.c
 */

#define LIBMACH_OPTIONS64 (MACH_SEND_INTERRUPT|MACH_RCV_INTERRUPT)
#define MACH64_SEND_MQ_CALL 0x0000000400000000ull

typedef mach_msg_return_t (*mach_msg2_trap_ptr_t)( void *data, uint64_t options,
    uint64_t msgh_bits_and_send_size, uint64_t msgh_remote_and_local_port,
    uint64_t msgh_voucher_and_id, uint64_t desc_count_and_rcv_name,
    uint64_t rcv_size_and_priority, uint64_t timeout );

static mach_msg2_trap_ptr_t mach_msg2_trap;

static inline mach_msg_return_t mach_msg2_internal( void *data, uint64_t option64, uint64_t msgh_bits_and_send_size,
    uint64_t msgh_remote_and_local_port, uint64_t msgh_voucher_and_id, uint64_t desc_count_and_rcv_name,
    uint64_t rcv_size_and_priority, uint64_t timeout)
{
    mach_msg_return_t mr;

    mr = mach_msg2_trap( data, option64 & ~LIBMACH_OPTIONS64, msgh_bits_and_send_size,
             msgh_remote_and_local_port, msgh_voucher_and_id, desc_count_and_rcv_name,
             rcv_size_and_priority, timeout );

    if (mr == MACH_MSG_SUCCESS)
        return MACH_MSG_SUCCESS;

    while (mr == MACH_SEND_INTERRUPTED)
        mr = mach_msg2_trap( data, option64 & ~LIBMACH_OPTIONS64, msgh_bits_and_send_size,
                 msgh_remote_and_local_port, msgh_voucher_and_id, desc_count_and_rcv_name,
                 rcv_size_and_priority, timeout );

    while (mr == MACH_RCV_INTERRUPTED)
        mr = mach_msg2_trap( data, option64 & ~LIBMACH_OPTIONS64, msgh_bits_and_send_size & 0xffffffffull,
                 msgh_remote_and_local_port, msgh_voucher_and_id, desc_count_and_rcv_name,
                 rcv_size_and_priority, timeout);

    return mr;
}

/* For older versions of macOS we need to provide fallback in case there is no mach_msg2... */
extern mach_msg_return_t mach_msg_trap( mach_msg_header_t *msg, mach_msg_option_t option,
        mach_msg_size_t send_size, mach_msg_size_t rcv_size, mach_port_name_t rcv_name, mach_msg_timeout_t timeout,
        mach_port_name_t notify );

static inline mach_msg_return_t mach_msg2( mach_msg_header_t *data, uint64_t option64,
    mach_msg_size_t send_size, mach_msg_size_t rcv_size, mach_port_t rcv_name, uint64_t timeout,
    uint32_t priority)
{
    mach_msg_base_t *base;
    mach_msg_size_t descriptors;

    if (!mach_msg2_trap)
        return mach_msg_trap( data, (mach_msg_option_t)option64, send_size,
                              rcv_size, rcv_name, timeout, priority );

    base = (mach_msg_base_t *)data;

    if ((option64 & MACH_SEND_MSG) &&
        (base->header.msgh_bits & MACH_MSGH_BITS_COMPLEX))
        descriptors = base->body.msgh_descriptor_count;
    else
        descriptors = 0;

#define MACH_MSG2_SHIFT_ARGS(lo, hi) ((uint64_t)hi << 32 | (uint32_t)lo)
    return mach_msg2_internal(data, option64 | MACH64_SEND_MQ_CALL,
               MACH_MSG2_SHIFT_ARGS(data->msgh_bits, send_size),
               MACH_MSG2_SHIFT_ARGS(data->msgh_remote_port, data->msgh_local_port),
               MACH_MSG2_SHIFT_ARGS(data->msgh_voucher_port, data->msgh_id),
               MACH_MSG2_SHIFT_ARGS(descriptors, rcv_name),
               MACH_MSG2_SHIFT_ARGS(rcv_size, priority), timeout);
#undef MACH_MSG2_SHIFT_ARGS
}

/* this is a lot, but running out cripples performance */
#define MAX_POOL_SEMAPHORES 1024
#define POOL_SHRINK_THRESHOLD 30
#define POOL_SHRINK_COUNT 10

struct semaphore_memory_pool
{
    semaphore_t semaphores[MAX_POOL_SEMAPHORES];
    semaphore_t *free_semaphores[MAX_POOL_SEMAPHORES];
    unsigned int count;
    unsigned int total;
    os_unfair_lock lock;
};

static struct semaphore_memory_pool *pool;

static void semaphore_pool_init(void)
{
    unsigned int i;

    pool = malloc( sizeof(struct semaphore_memory_pool) );

    pool->lock = OS_UNFAIR_LOCK_INIT;

    for (i = 0; i < MAX_POOL_SEMAPHORES; i++)
    {
        pool->free_semaphores[i] = &pool->semaphores[i];
    }

    pool->count = 0;
    pool->total = 0;
}

static inline semaphore_t *semaphore_pool_alloc(void)
{
    semaphore_t *new_semaphore;
    kern_return_t kr;

    os_unfair_lock_lock(&pool->lock);

    if (pool->count == 0)
    {
        if (pool->total < MAX_POOL_SEMAPHORES)
        {
            TRACE("Dynamically growing semaphore pool\n");
            kr = semaphore_create(mach_task_self(), &pool->semaphores[pool->total], SYNC_POLICY_FIFO, 0);
            if (kr != KERN_SUCCESS)
                ERR("Cannot create dynamic semaphore: %#x %s\n", kr, mach_error_string(kr));

            new_semaphore = &pool->semaphores[pool->total];
            pool->total++;

            os_unfair_lock_unlock(&pool->lock);

            return new_semaphore;
        }
        else
        {
            os_unfair_lock_unlock(&pool->lock);

            WARN("Semaphore pool exhausted, consider increasing MAX_POOL_SEMAPHORES\n");
            new_semaphore = malloc(sizeof(semaphore_t));
            kr = semaphore_create(mach_task_self(), new_semaphore, SYNC_POLICY_FIFO, 0);
            if (kr != KERN_SUCCESS)
                ERR("Cannot create dynamic semaphore: %#x %s\n", kr, mach_error_string(kr));

            return new_semaphore;
        }
    }

    new_semaphore = pool->free_semaphores[pool->count - 1];
    pool->count--;

    os_unfair_lock_unlock(&pool->lock);

    return new_semaphore;
}

static inline void semaphore_pool_free(semaphore_t *sem)
{
    int i;

    os_unfair_lock_lock(&pool->lock);

    if (sem < pool->semaphores || sem >= pool->semaphores + MAX_POOL_SEMAPHORES)
    {
        os_unfair_lock_unlock(&pool->lock);

        semaphore_destroy(mach_task_self(), *sem);
        free(sem);

        return;
    }

    if (pool->count >= POOL_SHRINK_THRESHOLD)
    {
        TRACE("Dynamically shrinking semaphore pool\n");
        for (i = 0; i < POOL_SHRINK_COUNT; i++)
        {
            semaphore_destroy(mach_task_self(), *sem);
            pool->total--;
        }
    }
    else
    {
        pool->free_semaphores[pool->count] = sem;
        pool->count++;
    }

    os_unfair_lock_unlock(&pool->lock);
}

struct msync
{
    void *shm;              /* pointer to shm section */
    enum msync_type type;
    unsigned int shm_idx;
};

typedef struct
{
    mach_msg_header_t header;
    mach_msg_body_t body;
    mach_msg_port_descriptor_t descriptor;
} mach_register_message_prolog_t;

typedef struct
{
    mach_register_message_prolog_t prolog;
    unsigned int shm_idx[MAXIMUM_WAIT_OBJECTS + 1];
} mach_register_message_t;

typedef struct
{
    mach_msg_header_t header;
    unsigned int shm_idx[MAXIMUM_WAIT_OBJECTS + 1];
} mach_unregister_message_t;

static mach_port_t server_port;

static const mach_msg_bits_t msgh_bits_complex_send = MACH_MSGH_BITS_SET(
                MACH_MSG_TYPE_COPY_SEND, 0, 0, MACH_MSGH_BITS_COMPLEX);

static const mach_msg_bits_t msgh_bits_send = MACH_MSGH_BITS_REMOTE(MACH_MSG_TYPE_COPY_SEND);

static inline void server_register_wait( semaphore_t sem, unsigned int msgh_id,
                                         struct msync **wait_objs, const int count )
{
    int i, is_mutex;
    mach_msg_return_t mr;
    __thread static mach_register_message_t message;

    message.prolog.header.msgh_remote_port = server_port;
    message.prolog.header.msgh_bits = msgh_bits_complex_send;
    message.prolog.header.msgh_id = msgh_id;

    message.prolog.body.msgh_descriptor_count = 1;

    message.prolog.descriptor.name = sem;
    message.prolog.descriptor.disposition = MACH_MSG_TYPE_COPY_SEND;
    message.prolog.descriptor.type = MACH_MSG_PORT_DESCRIPTOR;

    for (i = 0; i < count; i++)
    {
        is_mutex = wait_objs[i]->type == MSYNC_MUTEX ? 1 : 0;
        message.shm_idx[i] = wait_objs[i]->shm_idx | (is_mutex << 28);
        __atomic_add_fetch( (int *)(wait_objs[i]->shm) + 3, 1, __ATOMIC_SEQ_CST);
    }

    message.prolog.header.msgh_size = sizeof(mach_register_message_prolog_t) +
                                      count * sizeof(unsigned int);

    mr = mach_msg2( (mach_msg_header_t *)&message, MACH_SEND_MSG, message.prolog.header.msgh_size,
                     0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, 0 );

    if (mr != MACH_MSG_SUCCESS)
        ERR("Failed to send server register wait: %#x\n", mr);
}

static inline void server_remove_wait( semaphore_t sem, unsigned int msgh_id,
                                       struct msync **wait_objs, const int count )
{
    int i;
    mach_msg_return_t mr;
    __thread static mach_unregister_message_t message;

    message.header.msgh_remote_port = server_port;
    message.header.msgh_bits = msgh_bits_send;
    message.header.msgh_id = msgh_id;

    for (i = 0; i < count; i++)
    {
        int old_val, new_val;
        do
        {
            old_val = __atomic_load_n( (int *)(wait_objs[i]->shm) + 3, __ATOMIC_SEQ_CST );
            if (old_val <= 0) break;
            new_val = old_val - 1;
        } while (!__atomic_compare_exchange_n( (int *)(wait_objs[i]->shm) + 3, &old_val,
                                               new_val, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST ));

        message.shm_idx[i] = wait_objs[i]->shm_idx;
    }

    message.header.msgh_size = sizeof(mach_msg_header_t) +
                               count * sizeof(unsigned int);

    mr = mach_msg2( (mach_msg_header_t *)&message, MACH_SEND_MSG, message.header.msgh_size,
                     0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, 0 );

    if (mr != MACH_MSG_SUCCESS)
        ERR("Failed to send server remove wait: %#x\n", mr);
}

static NTSTATUS destroyed_wait( ULONGLONG *end )
{
    if (end)
    {
        usleep( update_timeout( *end ) / 10 );
        return STATUS_TIMEOUT;
    }
    pause();
    return STATUS_PENDING;
}

static inline int is_destroyed( struct msync **objs, int count)
{
    int i;

    for (i = 0; i < count; i++)
        if (__atomic_load_n( (int *)objs[i]->shm + 2, __ATOMIC_RELAXED ))
            return 0;

    return 1;
}

static inline NTSTATUS msync_wait_single( struct msync *wait_obj,
                                          ULONGLONG *end )
{
    int ret, val = 0;
    void *addr = wait_obj->shm;
    ULONGLONG ns_timeleft = 0;

    do 
    {
        if (wait_obj->type == MSYNC_MUTEX)
        {
            val = __atomic_load_n( (int *)addr, __ATOMIC_ACQUIRE );
            if (!val || val == ~0)
                val = GetCurrentThreadId();
        }

        if (__atomic_load_n( (int *)addr, __ATOMIC_ACQUIRE ) != val)
            return STATUS_PENDING;

        if (end)
        {
            ns_timeleft = update_timeout( *end ) * 100;
            if (!ns_timeleft) return STATUS_TIMEOUT;
        }
        ret = __ulock_wait2( UL_COMPARE_AND_WAIT_SHARED | ULF_NO_ERRNO, addr, val, ns_timeleft, 0 );
    } while (ret == -EINTR);

    if (ret == -ETIMEDOUT)
        return STATUS_TIMEOUT;

    if (is_destroyed( &wait_obj, 1 ))
        return destroyed_wait( end );

    return STATUS_SUCCESS;
}

static inline int resize_wait_objs( struct msync **wait_objs, struct msync **objs, int count )
{
    int read_index, write_index = 0;

    for (read_index = 0; read_index < count; read_index++)
    {
        if (wait_objs[read_index] &&
            __atomic_load_n( (int *)wait_objs[read_index]->shm + 2, __ATOMIC_RELAXED ))
        {
            objs[write_index] = wait_objs[read_index];
            write_index++;
        }
    }

    return write_index;
}

static inline int check_shm_contention( struct msync **wait_objs,
                                        int count, int tid )
{
    int i, val;

    for (i = 0; i < count; i++)
    {
        val = __atomic_load_n((int *)wait_objs[i]->shm, __ATOMIC_SEQ_CST);
        if (wait_objs[i]->type == MSYNC_MUTEX)
        {
            if (val == 0 || val == ~0 || val == tid) return 1;
        }
        else
        {
            if (val != 0)  return 1;
        }
    }
    return 0;
}

static NTSTATUS msync_wait_multiple( struct msync **wait_objs,
                                     int count, ULONGLONG *end )
{
    int tid;
    semaphore_t *sem;
    kern_return_t kr;
    unsigned int msgh_id;
    __thread static struct msync *objs[MAXIMUM_WAIT_OBJECTS + 1];

    count = resize_wait_objs( wait_objs, objs, count );

    if (count == 1 && __ulock_wait2) return msync_wait_single( objs[0], end );
    if (!count) return destroyed_wait( end );

    tid = GetCurrentThreadId();

    if (check_shm_contention( objs, count, tid ))
        return STATUS_PENDING;

    sem = semaphore_pool_alloc();
    msgh_id = (tid << 8) | count;
    server_register_wait( *sem, msgh_id, objs, count );

    do
    {
        if (end)
            kr = semaphore_timedwait( *sem,
                        convert_to_mach_time( update_timeout( *end ) ) );
        else
            kr = semaphore_wait( *sem );
    } while (kr == KERN_ABORTED);

    semaphore_pool_free( sem );

    if (is_destroyed( objs, count ))
        return destroyed_wait( end );

    server_remove_wait( *sem, msgh_id, objs, count );

    switch (kr) {
        case KERN_SUCCESS:
            return STATUS_SUCCESS;
        case KERN_OPERATION_TIMED_OUT:
            if (check_shm_contention( objs, count, tid ))
                return STATUS_PENDING;
            return STATUS_TIMEOUT;
        case KERN_TERMINATED:
            return destroyed_wait( end );
        default:
            ERR("Unexpected kernel return code: %#x %s\n", kr, mach_error_string( kr ));
            return STATUS_PENDING;
    }
}

int do_msync(void)
{
#ifdef __APPLE__
    static int do_msync_cached = -1;

    if (do_msync_cached == -1)
        do_msync_cached = getenv("WINEMSYNC") && atoi(getenv("WINEMSYNC"));

    return do_msync_cached;
#else
    static int once;
    if (!once++)
        FIXME("mach semaphores not supported on this platform.\n");
    return 0;
#endif
}

struct semaphore
{
    int count;
    int max;
};
C_ASSERT(sizeof(struct semaphore) == 8);

struct event
{
    int signaled;
    int unused;
};
C_ASSERT(sizeof(struct event) == 8);

struct mutex
{
    int tid;
    int count;  /* recursion count */
};
C_ASSERT(sizeof(struct mutex) == 8);

static char shm_name[29];
static int shm_fd;
static void **shm_addrs;
static int shm_addrs_size;  /* length of the allocated shm_addrs array */
static long pagesize;

static os_unfair_lock shm_addrs_lock = OS_UNFAIR_LOCK_INIT;

static void *get_shm( unsigned int idx )
{
    int entry  = (idx * 16) / pagesize;
    int offset = (idx * 16) % pagesize;
    void *ret;

    os_unfair_lock_lock( &shm_addrs_lock );

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

        if (__sync_val_compare_and_swap( &shm_addrs[entry], 0, addr ))
            munmap( addr, pagesize ); /* someone beat us to it */
    }

    ret = (void *)((unsigned long)shm_addrs[entry] + offset);

    os_unfair_lock_unlock( &shm_addrs_lock );

    return ret;
}

/* We'd like lookup to be fast. To that end, we use a static list indexed by handle.
 * This is copied and adapted from the fd cache code. */

#define MSYNC_LIST_BLOCK_SIZE  (65536 / sizeof(struct msync))
#define MSYNC_LIST_ENTRIES     256

static struct msync *msync_list[MSYNC_LIST_ENTRIES];
static struct msync msync_list_initial_block[MSYNC_LIST_BLOCK_SIZE];

static inline UINT_PTR handle_to_index( HANDLE handle, UINT_PTR *entry )
{
    UINT_PTR idx = (((UINT_PTR)handle) >> 2) - 1;
    *entry = idx / MSYNC_LIST_BLOCK_SIZE;
    return idx % MSYNC_LIST_BLOCK_SIZE;
}

static struct msync *add_to_list( HANDLE handle, enum msync_type type, unsigned int shm_idx )
{
    UINT_PTR entry, idx = handle_to_index( handle, &entry );
    void *shm = get_shm( shm_idx );

    if (entry >= MSYNC_LIST_ENTRIES)
    {
        FIXME( "too many allocated handles, not caching %p\n", handle );
        return FALSE;
    }

    if (!msync_list[entry])  /* do we need to allocate a new block of entries? */
    {
        if (!entry) msync_list[0] = msync_list_initial_block;
        else
        {
            void *ptr = anon_mmap_alloc( MSYNC_LIST_BLOCK_SIZE * sizeof(struct msync),
                                         PROT_READ | PROT_WRITE );
            if (ptr == MAP_FAILED) return FALSE;
            msync_list[entry] = ptr;
        }
    }

    if (!__sync_val_compare_and_swap((int *)&msync_list[entry][idx].type, 0, type ))
    {
        msync_list[entry][idx].shm = shm;
        msync_list[entry][idx].shm_idx = shm_idx;
    }

    return &msync_list[entry][idx];
}

static struct msync *get_cached_object( HANDLE handle )
{
    UINT_PTR entry, idx = handle_to_index( handle, &entry );

    if (entry >= MSYNC_LIST_ENTRIES || !msync_list[entry]) return NULL;
    if (!msync_list[entry][idx].type) return NULL;

    return &msync_list[entry][idx];
}

/* Gets an object. This is either a proper msync object (i.e. an event,
 * semaphore, etc. created using create_msync) or a generic synchronizable
 * server-side object which the server will signal (e.g. a process, thread,
 * message queue, etc.) */
static NTSTATUS get_object( HANDLE handle, struct msync **obj )
{
    NTSTATUS ret = STATUS_SUCCESS;
    unsigned int shm_idx = 0;
    enum msync_type type;

    if ((*obj = get_cached_object( handle ))) return STATUS_SUCCESS;

    if ((INT_PTR)handle < 0)
    {
        /* We can deal with pseudo-handles, but it's just easier this way */
        return STATUS_NOT_IMPLEMENTED;
    }

    /* We need to try grabbing it from the server. */
    SERVER_START_REQ( get_msync_idx )
    {
        req->handle = wine_server_obj_handle( handle );
        if (!(ret = wine_server_call( req )))
        {
            shm_idx = reply->shm_idx;
            type    = reply->type;
        }
    }
    SERVER_END_REQ;

    if (ret)
    {
        WARN("Failed to retrieve shm index for handle %p, status %#x.\n", handle, ret);
        *obj = NULL;
        return ret;
    }

    TRACE("Got shm index %d for handle %p.\n", shm_idx, handle);
    *obj = add_to_list( handle, type, shm_idx );
    return ret;
}

NTSTATUS msync_close( HANDLE handle )
{
    UINT_PTR entry, idx = handle_to_index( handle, &entry );

    TRACE("%p.\n", handle);

    if (entry < MSYNC_LIST_ENTRIES && msync_list[entry])
    {
        if (__atomic_exchange_n( &msync_list[entry][idx].type, 0, __ATOMIC_SEQ_CST ))
            return STATUS_SUCCESS;
    }

    return STATUS_INVALID_HANDLE;
}

static NTSTATUS create_msync( enum msync_type type, HANDLE *handle,
    ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, int low, int high )
{
    NTSTATUS ret;
    data_size_t len;
    struct object_attributes *objattr;
    unsigned int shm_idx;

    if ((ret = alloc_object_attributes( attr, &objattr, &len ))) return ret;

    SERVER_START_REQ( create_msync )
    {
        req->access = access;
        req->low    = low;
        req->high   = high;
        req->type   = type;
        wine_server_add_data( req, objattr, len );
        ret = wine_server_call( req );
        if (!ret || ret == STATUS_OBJECT_NAME_EXISTS)
        {
            *handle = wine_server_ptr_handle( reply->handle );
            shm_idx = reply->shm_idx;
            type    = reply->type;
        }
    }
    SERVER_END_REQ;

    if (!ret || ret == STATUS_OBJECT_NAME_EXISTS)
    {
        add_to_list( *handle, type, shm_idx );
        TRACE("-> handle %p, shm index %d.\n", *handle, shm_idx);
    }

    free( objattr );
    return ret;
}

static NTSTATUS open_msync( enum msync_type type, HANDLE *handle,
    ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS ret;
    unsigned int shm_idx;

    SERVER_START_REQ( open_msync )
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
        }
    }
    SERVER_END_REQ;

    if (!ret)
    {
        add_to_list( *handle, type, shm_idx );
        TRACE("-> handle %p, shm index %u.\n", *handle, shm_idx);
    }
    return ret;
}

void msync_init(void)
{
    struct stat st;
    mach_port_t bootstrap_port;
    void *dlhandle = dlopen( NULL, RTLD_NOW );

    if (!do_msync())
    {
        /* make sure the server isn't running with WINEMSYNC */
        HANDLE handle;
        NTSTATUS ret;

        ret = create_msync( 0, &handle, 0, NULL, 0, 0 );
        if (ret != STATUS_NOT_IMPLEMENTED)
        {
            ERR("Server is running with WINEMSYNC but this process is not, please enable WINEMSYNC or restart wineserver.\n");
            exit(1);
        }

        dlclose( dlhandle );
        return;
    }

    if (stat( config_dir, &st ) == -1)
        ERR("Cannot stat %s\n", config_dir);

    if (st.st_ino != (unsigned long)st.st_ino)
        sprintf( shm_name, "/wine-%lx%08lx-msync", (unsigned long)((unsigned long long)st.st_ino >> 32), (unsigned long)st.st_ino );
    else
        sprintf( shm_name, "/wine-%lx-msync", (unsigned long)st.st_ino );

    if ((shm_fd = shm_open( shm_name, O_RDWR, 0644 )) == -1)
    {
        /* probably the server isn't running with WINEMSYNC, tell the user and bail */
        if (errno == ENOENT)
            ERR("Failed to open msync shared memory file; make sure no stale wineserver instances are running without WINEMSYNC.\n");
        else
            ERR("Failed to initialize shared memory: %s\n", strerror( errno ));
        exit(1);
    }

    pagesize = sysconf( _SC_PAGESIZE );

    shm_addrs = calloc( 128, sizeof(shm_addrs[0]) );
    shm_addrs_size = 128;

    semaphore_pool_init();

    __ulock_wait2 = (__ulock_wait2_ptr_t)dlsym( dlhandle, "__ulock_wait2" );
    if (!__ulock_wait2)
        WARN("__ulock_wait2 not available, performance will be lower\n");

    /* Bootstrap mach wineserver communication */

    mach_msg2_trap = (mach_msg2_trap_ptr_t)dlsym( dlhandle, "mach_msg2_trap" );
    if (!mach_msg2_trap)
        WARN("Using mach_msg_overwrite instead of mach_msg2\n");
    dlclose( dlhandle );

    if (task_get_special_port(mach_task_self(), TASK_BOOTSTRAP_PORT, &bootstrap_port) != KERN_SUCCESS)
    {
        ERR("Failed task_get_special_port\n");
        exit(1);
    }

    if (bootstrap_look_up(bootstrap_port, shm_name + 1, &server_port) != KERN_SUCCESS)
    {
        ERR("Failed bootstrap_look_up for %s\n", shm_name + 1);
        exit(1);
    }
}

NTSTATUS msync_create_semaphore( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr, LONG initial, LONG max )
{
    TRACE("name %s, initial %d, max %d.\n",
        attr ? debugstr_us(attr->ObjectName) : "<no name>", initial, max);

    return create_msync( MSYNC_SEMAPHORE, handle, access, attr, initial, max );
}

NTSTATUS msync_open_semaphore( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr )
{
    TRACE("name %s.\n", debugstr_us(attr->ObjectName));

    return open_msync( MSYNC_SEMAPHORE, handle, access, attr );
}

static inline void signal_all( struct msync *obj )
{
    __thread static mach_msg_header_t send_header;

    __ulock_wake( UL_COMPARE_AND_WAIT_SHARED | ULF_WAKE_ALL, obj->shm, 0 );

    if (!__atomic_load_n( (int *)obj->shm + 3, __ATOMIC_ACQUIRE ))
        return;

    send_header.msgh_bits = msgh_bits_send;
    send_header.msgh_id = obj->shm_idx;
    send_header.msgh_size = sizeof(send_header);
    send_header.msgh_remote_port = server_port;

    mach_msg2( &send_header, MACH_SEND_MSG, send_header.msgh_size, 0,
               MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, 0 );
}

NTSTATUS msync_release_semaphore( HANDLE handle, ULONG count, ULONG *prev )
{
    struct msync *obj;
    struct semaphore *semaphore;
    ULONG current;
    NTSTATUS ret;

    TRACE("%p, %d, %p.\n", handle, count, prev);

    if ((ret = get_object( handle, &obj ))) return ret;
    semaphore = obj->shm;

    do
    {
        current = semaphore->count;
        if (count + current > semaphore->max)
            return STATUS_SEMAPHORE_LIMIT_EXCEEDED;
    } while (__sync_val_compare_and_swap( &semaphore->count, current, count + current ) != current);

    if (prev) *prev = current;

    signal_all( obj );

    return STATUS_SUCCESS;
}

NTSTATUS msync_query_semaphore( HANDLE handle, void *info, ULONG *ret_len )
{
    struct msync *obj;
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

NTSTATUS msync_create_event( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr, EVENT_TYPE event_type, BOOLEAN initial )
{
    enum msync_type type = (event_type == SynchronizationEvent ? MSYNC_AUTO_EVENT : MSYNC_MANUAL_EVENT);

    TRACE("name %s, %s-reset, initial %d.\n",
        attr ? debugstr_us(attr->ObjectName) : "<no name>",
        event_type == NotificationEvent ? "manual" : "auto", initial);

    return create_msync( type, handle, access, attr, initial, 0xdeadbeef );
}

NTSTATUS msync_open_event( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr )
{
    TRACE("name %s.\n", debugstr_us(attr->ObjectName));

    return open_msync( MSYNC_AUTO_EVENT, handle, access, attr );
}

NTSTATUS msync_set_event( HANDLE handle, LONG *prev )
{
    struct event *event;
    struct msync *obj;
    LONG current;
    NTSTATUS ret;

    TRACE("%p.\n", handle);

    if ((ret = get_object( handle, &obj ))) return ret;
    event = obj->shm;

    if (obj->type != MSYNC_MANUAL_EVENT && obj->type != MSYNC_AUTO_EVENT)
        return STATUS_OBJECT_TYPE_MISMATCH;

    if (!(current = __atomic_exchange_n( &event->signaled, 1, __ATOMIC_SEQ_CST )))
        signal_all( obj );

    if (prev) *prev = current;

    return STATUS_SUCCESS;
}

NTSTATUS msync_reset_event( HANDLE handle, LONG *prev )
{
    struct event *event;
    struct msync *obj;
    LONG current;
    NTSTATUS ret;

    TRACE("%p.\n", handle);

    if ((ret = get_object( handle, &obj ))) return ret;
    event = obj->shm;

    current = __atomic_exchange_n( &event->signaled, 0, __ATOMIC_SEQ_CST );

    if (prev) *prev = current;

    return STATUS_SUCCESS;
}

NTSTATUS msync_pulse_event( HANDLE handle, LONG *prev )
{
    struct event *event;
    struct msync *obj;
    LONG current;
    NTSTATUS ret;

    TRACE("%p.\n", handle);

    if ((ret = get_object( handle, &obj ))) return ret;
    event = obj->shm;

    /* This isn't really correct; an application could miss the write.
     * Unfortunately we can't really do much better. Fortunately this is rarely
     * used (and publicly deprecated). */
    if (!(current = __atomic_exchange_n( &event->signaled, 1, __ATOMIC_SEQ_CST )))
        signal_all( obj );

    /* Try to give other threads a chance to wake up. Hopefully erring on this
     * side is the better thing to do... */
    sched_yield();

    __atomic_store_n( &event->signaled, 0, __ATOMIC_SEQ_CST );

    if (prev) *prev = current;

    return STATUS_SUCCESS;
}

NTSTATUS msync_query_event( HANDLE handle, void *info, ULONG *ret_len )
{
    struct event *event;
    struct msync *obj;
    EVENT_BASIC_INFORMATION *out = info;
    NTSTATUS ret;

    TRACE("handle %p, info %p, ret_len %p.\n", handle, info, ret_len);

    if ((ret = get_object( handle, &obj ))) return ret;
    event = obj->shm;

    out->EventState = event->signaled;
    out->EventType = (obj->type == MSYNC_AUTO_EVENT ? SynchronizationEvent : NotificationEvent);
    if (ret_len) *ret_len = sizeof(*out);

    return STATUS_SUCCESS;
}

NTSTATUS msync_create_mutex( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr, BOOLEAN initial )
{
    TRACE("name %s, initial %d.\n",
        attr ? debugstr_us(attr->ObjectName) : "<no name>", initial);

    return create_msync( MSYNC_MUTEX, handle, access, attr,
        initial ? GetCurrentThreadId() : 0, initial ? 1 : 0 );
}

NTSTATUS msync_open_mutex( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr )
{
    TRACE("name %s.\n", debugstr_us(attr->ObjectName));

    return open_msync( MSYNC_MUTEX, handle, access, attr );
}

NTSTATUS msync_release_mutex( HANDLE handle, LONG *prev )
{
    struct mutex *mutex;
    struct msync *obj;
    NTSTATUS ret;

    TRACE("%p, %p.\n", handle, prev);

    if ((ret = get_object( handle, &obj ))) return ret;
    mutex = obj->shm;

    if (mutex->tid != GetCurrentThreadId()) return STATUS_MUTANT_NOT_OWNED;

    if (prev) *prev = mutex->count;

    if (!--mutex->count)
    {
        __atomic_store_n( &mutex->tid, 0, __ATOMIC_SEQ_CST );
        signal_all( obj );
    }

    return STATUS_SUCCESS;
}

NTSTATUS msync_query_mutex( HANDLE handle, void *info, ULONG *ret_len )
{
    struct msync *obj;
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

static NTSTATUS do_single_wait( struct msync *obj, ULONGLONG *end, BOOLEAN alertable )
{
    NTSTATUS status;
    struct msync *wait_objs[2];

    wait_objs[0] = obj;

    if (alertable)
    {
        struct msync apc_obj;
        int *apc_addr = ntdll_get_thread_data()->msync_apc_addr;

        apc_obj.type = MSYNC_AUTO_EVENT;
        apc_obj.shm = (void *)apc_addr;
        apc_obj.shm_idx = ntdll_get_thread_data()->msync_apc_idx;

        if (__atomic_load_n( apc_addr, __ATOMIC_SEQ_CST ))
            return STATUS_USER_APC;

        wait_objs[1] = &apc_obj;

        status = msync_wait_multiple( wait_objs, 2, end );

        if (__atomic_load_n( apc_addr, __ATOMIC_SEQ_CST ))
            return STATUS_USER_APC;
    }
    else
    {
        status = msync_wait_multiple( wait_objs, 1, end );
    }
    return status;
}

static NTSTATUS __msync_wait_objects( DWORD count, const HANDLE *handles,
    BOOLEAN wait_any, BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    static const LARGE_INTEGER zero = {0};

    __thread static struct msync *objs[MAXIMUM_WAIT_OBJECTS + 1];
    struct msync apc_obj;
    int has_msync = 0, has_server = 0;
    BOOL msgwait = FALSE;
    LONGLONG timeleft;
    LARGE_INTEGER now;
    DWORD waitcount;
    ULONGLONG end;
    int i, ret;

    /* Grab the APC idx if we don't already have it. */
    if (alertable && !ntdll_get_thread_data()->msync_apc_addr)
    {
        unsigned int idx = 0;
        SERVER_START_REQ( get_msync_apc_idx )
        {
            if (!(ret = wine_server_call( req )))
                idx = reply->shm_idx;
        }
        SERVER_END_REQ;

        if (idx)
        {
            struct event *apc_event = get_shm( idx );
            ntdll_get_thread_data()->msync_apc_addr = &apc_event->signaled;
            ntdll_get_thread_data()->msync_apc_idx = idx;
        }
    }

    NtQuerySystemTime( &now );
    if (timeout)
    {
        if (timeout->QuadPart == TIMEOUT_INFINITE)
            timeout = NULL;
        else if (timeout->QuadPart > 0)
            end = timeout->QuadPart;
        else
            end = now.QuadPart - timeout->QuadPart;
    }

    for (i = 0; i < count; i++)
    {
        ret = get_object( handles[i], &objs[i] );
        if (ret == STATUS_SUCCESS)
            has_msync = 1;
        else if (ret == STATUS_NOT_IMPLEMENTED)
            has_server = 1;
        else
            return ret;
    }

    if (count && objs[count - 1] && objs[count - 1]->type == MSYNC_QUEUE)
        msgwait = TRUE;

    if (has_msync && has_server)
        FIXME("Can't wait on msync and server objects at the same time!\n");
    else if (has_server)
        return STATUS_NOT_IMPLEMENTED;

    if (TRACE_ON(msync))
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
                (long) (timeleft / TICKSPERSEC), (long) (timeleft % TICKSPERSEC));
        }
    }

    if (wait_any || count <= 1)
    {
        while (1)
        {
            /* Try to grab anything. */

            if (alertable)
            {
                apc_obj.type = MSYNC_AUTO_EVENT;
                /* We must check this first! The server may set an event that
                 * we're waiting on, but we need to return STATUS_USER_APC. */
                if (__atomic_load_n( ntdll_get_thread_data()->msync_apc_addr, __ATOMIC_SEQ_CST ))
                    goto userapc;
            }

            for (i = 0; i < count; i++)
            {
                struct msync *obj = objs[i];

                if (obj)
                {
                    if (!obj->type) /* gcc complains if we put this in the switch */
                    {
                        /* Someone probably closed an object while waiting on it. */
                        WARN("Handle %p has type 0; was it closed?\n", handles[i]);
                        return STATUS_INVALID_HANDLE;
                    }

                    switch (obj->type)
                    {
                    case MSYNC_SEMAPHORE:
                    {
                        struct semaphore *semaphore = obj->shm;
                        int current;

                        current = __atomic_load_n(&semaphore->count, __ATOMIC_ACQUIRE);
                        if (current && __atomic_compare_exchange_n(&semaphore->count, &current, current - 1, 0, __ATOMIC_RELEASE, __ATOMIC_RELAXED))
                        {
                            TRACE("Woken up by handle %p [%d].\n", handles[i], i);
                            return i;
                        }
                        break;
                    }
                    case MSYNC_MUTEX:
                    {
                        struct mutex *mutex = obj->shm;
                        int tid;

                        if (mutex->tid == GetCurrentThreadId())
                        {
                            TRACE("Woken up by handle %p [%d].\n", handles[i], i);
                            mutex->count++;
                            return i;
                        }

                        tid = 0;
                        if (__atomic_compare_exchange_n(&mutex->tid, &tid, GetCurrentThreadId(), 0, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
                        {
                            TRACE("Woken up by handle %p [%d].\n", handles[i], i);
                            mutex->count = 1;
                            return i;
                        }
                        else if (tid == ~0 && __atomic_compare_exchange_n(&mutex->tid, &tid, GetCurrentThreadId(), 0, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
                        {
                            TRACE("Woken up by abandoned mutex %p [%d].\n", handles[i], i);
                            mutex->count = 1;
                            return STATUS_ABANDONED_WAIT_0 + i;
                        }

                        break;
                    }
                    case MSYNC_AUTO_EVENT:
                    case MSYNC_AUTO_SERVER:
                    {
                        struct event *event = obj->shm;
                        int signaled = 1;

                        if (__atomic_compare_exchange_n(&event->signaled, &signaled, 0, 0, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
                        {
                            TRACE("Woken up by handle %p [%d].\n", handles[i], i);
                            return i;
                        }

                        break;
                    }
                    case MSYNC_MANUAL_EVENT:
                    case MSYNC_MANUAL_SERVER:
                    case MSYNC_QUEUE:
                    {
                        struct event *event = obj->shm;

                        if (__atomic_load_n(&event->signaled, __ATOMIC_ACQUIRE))
                        {
                            TRACE("Woken up by handle %p [%d].\n", handles[i], i);
                            return i;
                        }
                        break;
                    }
                    default:
                        ERR("Invalid type %#x for handle %p.\n", obj->type, handles[i]);
                        assert(0);
                    }
                }
            }

            if (alertable)
            {
                /* We already checked if it was signaled; don't bother doing it again. */
                apc_obj.shm = (void *)ntdll_get_thread_data()->msync_apc_addr;
                apc_obj.shm_idx = ntdll_get_thread_data()->msync_apc_idx;
                objs[i] = &apc_obj;
                i++;
            }
            waitcount = i;

            /* Looks like everything is contended, so wait. */

            if (timeout && !timeout->QuadPart)
            {
                /* Unlike esync, we already know that we've timed out, so we
                 * can avoid a syscall. */
                TRACE("Wait timed out.\n");
                return STATUS_TIMEOUT;
            }

            ret = msync_wait_multiple( objs, waitcount, timeout ? &end : NULL );

            if (ret == STATUS_TIMEOUT)
            {
                TRACE("Wait timed out.\n");
                return STATUS_TIMEOUT;
            }
        } /* while (1) */
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

        NTSTATUS status = STATUS_SUCCESS;

        while (1)
        {
            BOOL abandoned;

tryagain:
            abandoned = FALSE;

            /* First step: try to wait on each object in sequence. */

            for (i = 0; i < count; i++)
            {
                struct msync *obj = objs[i];

                if (obj && obj->type == MSYNC_MUTEX)
                {
                    struct mutex *mutex = obj->shm;

                    if (mutex->tid == GetCurrentThreadId())
                        continue;

                    while (__atomic_load_n( &mutex->tid, __ATOMIC_SEQ_CST ))
                    {
                        status = do_single_wait( obj, timeout ? &end : NULL, alertable );
                        if (status != STATUS_PENDING)
                            break;
                    }
                }
                else if (obj)
                {
                    /* this works for semaphores too */
                    struct event *event = obj->shm;

                    while (!__atomic_load_n( &event->signaled, __ATOMIC_SEQ_CST ))
                    {
                        status = do_single_wait( obj, timeout ? &end : NULL, alertable );
                        if (status != STATUS_PENDING)
                            break;
                    }
                }

                if (status == STATUS_TIMEOUT)
                {
                    TRACE("Wait timed out.\n");
                    return STATUS_TIMEOUT;
                }
                else if (status == STATUS_USER_APC)
                    goto userapc;
            }

            /* If we got here and we haven't timed out, that means all of the
             * handles were signaled. Check to make sure they still are. */
            for (i = 0; i < count; i++)
            {
                struct msync *obj = objs[i];

                if (obj && obj->type == MSYNC_MUTEX)
                {
                    struct mutex *mutex = obj->shm;
                    int tid = __atomic_load_n( &mutex->tid, __ATOMIC_SEQ_CST );

                    if (tid && tid != ~0 && tid != GetCurrentThreadId())
                        goto tryagain;
                }
                else if (obj)
                {
                    struct event *event = obj->shm;

                    if (!__atomic_load_n( &event->signaled, __ATOMIC_SEQ_CST ))
                        goto tryagain;
                }
            }

            /* Yep, still signaled. Now quick, grab everything. */
            for (i = 0; i < count; i++)
            {
                struct msync *obj = objs[i];
                if (!obj) continue;
                switch (obj->type)
                {
                case MSYNC_MUTEX:
                {
                    struct mutex *mutex = obj->shm;
                    int tid = __atomic_load_n( &mutex->tid, __ATOMIC_SEQ_CST );
                    if (tid == GetCurrentThreadId())
                        break;
                    if (tid && tid != ~0)
                        goto tooslow;
                    if (__sync_val_compare_and_swap( &mutex->tid, tid, GetCurrentThreadId() ) != tid)
                        goto tooslow;
                    if (tid == ~0)
                        abandoned = TRUE;
                    break;
                }
                case MSYNC_SEMAPHORE:
                {
                    struct semaphore *semaphore = obj->shm;
                    int current;

                    if (!(current = __atomic_load_n( &semaphore->count, __ATOMIC_SEQ_CST ))
                            || __sync_val_compare_and_swap( &semaphore->count, current, current - 1 ) != current)
                        goto tooslow;
                    break;
                }
                case MSYNC_AUTO_EVENT:
                case MSYNC_AUTO_SERVER:
                {
                    struct event *event = obj->shm;
                    if (!__sync_val_compare_and_swap( &event->signaled, 1, 0 ))
                        goto tooslow;
                    break;
                }
                default:
                    /* If a manual-reset event changed between there and
                     * here, it's shouldn't be a problem. */
                    break;
                }
            }

            /* If we got here, we successfully waited on every object.
             * Make sure to let ourselves know that we grabbed the mutexes. */
            for (i = 0; i < count; i++)
            {
                if (objs[i] && objs[i]->type == MSYNC_MUTEX)
                {
                    struct mutex *mutex = objs[i]->shm;
                    mutex->count++;
                }
            }

            if (abandoned)
            {
                TRACE("Wait successful, but some object(s) were abandoned.\n");
                return STATUS_ABANDONED;
            }
            TRACE("Wait successful.\n");
            return STATUS_SUCCESS;

tooslow:
            for (--i; i >= 0; i--)
            {
                struct msync *obj = objs[i];
                if (!obj) continue;
                switch (obj->type)
                {
                case MSYNC_MUTEX:
                {
                    struct mutex *mutex = obj->shm;
                    /* HACK: This won't do the right thing with abandoned
                     * mutexes, but fixing it is probably more trouble than
                     * it's worth. */
                    __atomic_store_n( &mutex->tid, 0, __ATOMIC_SEQ_CST );
                    break;
                }
                case MSYNC_SEMAPHORE:
                {
                    struct semaphore *semaphore = obj->shm;
                    __sync_fetch_and_add( &semaphore->count, 1 );
                    break;
                }
                case MSYNC_AUTO_EVENT:
                case MSYNC_AUTO_SERVER:
                {
                    struct event *event = obj->shm;
                    __atomic_store_n( &event->signaled, 1, __ATOMIC_SEQ_CST );
                    break;
                }
                default:
                    /* doesn't need to be put back */
                    break;
                }
            }
        } /* while (1) */
    } /* else (wait-all) */

    assert(0);  /* shouldn't reach here... */

userapc:
    TRACE("Woken up by user APC.\n");

    /* We have to make a server call anyway to get the APC to execute, so just
     * delegate down to server_wait(). */
    ret = server_wait( NULL, 0, SELECT_INTERRUPTIBLE | SELECT_ALERTABLE, &zero );

    /* This can happen if we received a system APC, and the APC fd was woken up
     * before we got SIGUSR1. poll() doesn't return EINTR in that case. The
     * right thing to do seems to be to return STATUS_USER_APC anyway. */
    if (ret == STATUS_TIMEOUT) ret = STATUS_USER_APC;
    return ret;
}

/* Like esync, we need to let the server know when we are doing a message wait,
 * and when we are done with one, so that all of the code surrounding hung
 * queues works, and we also need this for WaitForInputIdle().
 *
 * Unlike esync, we can't wait on the queue fd itself locally. Instead we let
 * the server do that for us, the way it normally does. This could actually
 * work for esync too, and that might be better. */
static void server_set_msgwait( int in_msgwait )
{
    SERVER_START_REQ( msync_msgwait )
    {
        req->in_msgwait = in_msgwait;
        wine_server_call( req );
    }
    SERVER_END_REQ;
}

/* This is a very thin wrapper around the proper implementation above. The
 * purpose is to make sure the server knows when we are doing a message wait.
 * This is separated into a wrapper function since there are at least a dozen
 * exit paths from msync_wait_objects(). */
NTSTATUS msync_wait_objects( DWORD count, const HANDLE *handles, BOOLEAN wait_any,
                             BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    BOOL msgwait = FALSE;
    struct msync *obj;
    NTSTATUS ret;

    if (count && !get_object( handles[count - 1], &obj ) && obj->type == MSYNC_QUEUE)
    {
        msgwait = TRUE;
        server_set_msgwait( 1 );
    }

    ret = __msync_wait_objects( count, handles, wait_any, alertable, timeout );

    if (msgwait)
        server_set_msgwait( 0 );

    return ret;
}

NTSTATUS msync_signal_and_wait( HANDLE signal, HANDLE wait, BOOLEAN alertable,
    const LARGE_INTEGER *timeout )
{
    struct msync *obj;
    NTSTATUS ret;

    if ((ret = get_object( signal, &obj ))) return ret;

    switch (obj->type)
    {
    case MSYNC_SEMAPHORE:
        ret = msync_release_semaphore( signal, 1, NULL );
        break;
    case MSYNC_AUTO_EVENT:
    case MSYNC_MANUAL_EVENT:
        ret = msync_set_event( signal, NULL );
        break;
    case MSYNC_MUTEX:
        ret = msync_release_mutex( signal, NULL );
        break;
    default:
        return STATUS_OBJECT_TYPE_MISMATCH;
    }
    if (ret) return ret;

    return msync_wait_objects( 1, &wait, TRUE, alertable, timeout );
}
