/*
 * Server-side thread management
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "file.h"
#include "handle.h"
#include "process.h"
#include "thread.h"
#include "request.h"
#include "user.h"
#include "security.h"


/* thread queues */

struct thread_wait
{
    struct thread_wait     *next;       /* next wait structure for this thread */
    struct thread          *thread;     /* owner thread */
    int                     count;      /* count of objects */
    int                     flags;
    void                   *cookie;     /* magic cookie to return to client */
    struct timeval          timeout;
    struct timeout_user    *user;
    struct wait_queue_entry queues[1];
};

/* asynchronous procedure calls */

struct thread_apc
{
    struct object       obj;      /* object header */
    struct list         entry;    /* queue linked list */
    struct thread      *caller;   /* thread that queued this apc */
    struct object      *owner;    /* object that queued this apc */
    int                 executed; /* has it been executed by the client? */
    apc_call_t          call;     /* call arguments */
    apc_result_t        result;   /* call results once executed */
};

static void dump_thread_apc( struct object *obj, int verbose );
static int thread_apc_signaled( struct object *obj, struct thread *thread );
static void thread_apc_destroy( struct object *obj );
static void clear_apc_queue( struct list *queue );

static const struct object_ops thread_apc_ops =
{
    sizeof(struct thread_apc),  /* size */
    dump_thread_apc,            /* dump */
    add_queue,                  /* add_queue */
    remove_queue,               /* remove_queue */
    thread_apc_signaled,        /* signaled */
    no_satisfied,               /* satisfied */
    no_signal,                  /* signal */
    no_get_fd,                  /* get_fd */
    no_map_access,              /* map_access */
    no_lookup_name,             /* lookup_name */
    no_open_file,               /* open_file */
    no_close_handle,            /* close_handle */
    thread_apc_destroy          /* destroy */
};


/* thread operations */

static void dump_thread( struct object *obj, int verbose );
static int thread_signaled( struct object *obj, struct thread *thread );
static unsigned int thread_map_access( struct object *obj, unsigned int access );
static void thread_poll_event( struct fd *fd, int event );
static void destroy_thread( struct object *obj );

static const struct object_ops thread_ops =
{
    sizeof(struct thread),      /* size */
    dump_thread,                /* dump */
    add_queue,                  /* add_queue */
    remove_queue,               /* remove_queue */
    thread_signaled,            /* signaled */
    no_satisfied,               /* satisfied */
    no_signal,                  /* signal */
    no_get_fd,                  /* get_fd */
    thread_map_access,          /* map_access */
    no_lookup_name,             /* lookup_name */
    no_open_file,               /* open_file */
    no_close_handle,            /* close_handle */
    destroy_thread              /* destroy */
};

static const struct fd_ops thread_fd_ops =
{
    NULL,                       /* get_poll_events */
    thread_poll_event,          /* poll_event */
    NULL,                       /* flush */
    NULL,                       /* get_fd_type */
    NULL,                       /* queue_async */
    NULL,                       /* reselect_async */
    NULL                        /* cancel_async */
};

static struct list thread_list = LIST_INIT(thread_list);

/* initialize the structure for a newly allocated thread */
static inline void init_thread_structure( struct thread *thread )
{
    int i;

    thread->unix_pid        = -1;  /* not known yet */
    thread->unix_tid        = -1;  /* not known yet */
    thread->context         = NULL;
    thread->suspend_context = NULL;
    thread->teb             = NULL;
    thread->debug_ctx       = NULL;
    thread->debug_event     = NULL;
    thread->debug_break     = 0;
    thread->queue           = NULL;
    thread->wait            = NULL;
    thread->error           = 0;
    thread->req_data        = NULL;
    thread->req_toread      = 0;
    thread->reply_data      = NULL;
    thread->reply_towrite   = 0;
    thread->request_fd      = NULL;
    thread->reply_fd        = NULL;
    thread->wait_fd         = NULL;
    thread->state           = RUNNING;
    thread->exit_code       = 0;
    thread->priority        = 0;
    thread->affinity        = 1;
    thread->suspend         = 0;
    thread->desktop_users   = 0;
    thread->token           = NULL;

    thread->creation_time = current_time;
    thread->exit_time.tv_sec = thread->exit_time.tv_usec = 0;

    list_init( &thread->mutex_list );
    list_init( &thread->system_apc );
    list_init( &thread->user_apc );

    for (i = 0; i < MAX_INFLIGHT_FDS; i++)
        thread->inflight[i].server = thread->inflight[i].client = -1;
}

/* check if address looks valid for a client-side data structure (TEB etc.) */
static inline int is_valid_address( void *addr )
{
    return addr && !((unsigned long)addr % sizeof(int));
}

/* create a new thread */
struct thread *create_thread( int fd, struct process *process )
{
    struct thread *thread;

    if (!(thread = alloc_object( &thread_ops ))) return NULL;

    init_thread_structure( thread );

    thread->process = (struct process *)grab_object( process );
    thread->desktop = process->desktop;
    if (!current) current = thread;

    list_add_head( &thread_list, &thread->entry );

    if (!(thread->id = alloc_ptid( thread )))
    {
        release_object( thread );
        return NULL;
    }
    if (!(thread->request_fd = create_anonymous_fd( &thread_fd_ops, fd, &thread->obj, 0 )))
    {
        release_object( thread );
        return NULL;
    }

    set_fd_events( thread->request_fd, POLLIN );  /* start listening to events */
    add_process_thread( thread->process, thread );
    return thread;
}

/* handle a client event */
static void thread_poll_event( struct fd *fd, int event )
{
    struct thread *thread = get_fd_user( fd );
    assert( thread->obj.ops == &thread_ops );

    if (event & (POLLERR | POLLHUP)) kill_thread( thread, 0 );
    else if (event & POLLIN) read_request( thread );
    else if (event & POLLOUT) write_reply( thread );
}

/* cleanup everything that is no longer needed by a dead thread */
/* used by destroy_thread and kill_thread */
static void cleanup_thread( struct thread *thread )
{
    int i;

    clear_apc_queue( &thread->system_apc );
    clear_apc_queue( &thread->user_apc );
    free( thread->req_data );
    free( thread->reply_data );
    if (thread->request_fd) release_object( thread->request_fd );
    if (thread->reply_fd) release_object( thread->reply_fd );
    if (thread->wait_fd) release_object( thread->wait_fd );
    free( thread->suspend_context );
    free_msg_queue( thread );
    cleanup_clipboard_thread(thread);
    destroy_thread_windows( thread );
    close_thread_desktop( thread );
    for (i = 0; i < MAX_INFLIGHT_FDS; i++)
    {
        if (thread->inflight[i].client != -1)
        {
            close( thread->inflight[i].server );
            thread->inflight[i].client = thread->inflight[i].server = -1;
        }
    }
    thread->req_data = NULL;
    thread->reply_data = NULL;
    thread->request_fd = NULL;
    thread->reply_fd = NULL;
    thread->wait_fd = NULL;
    thread->context = NULL;
    thread->suspend_context = NULL;
    thread->desktop = 0;
}

/* destroy a thread when its refcount is 0 */
static void destroy_thread( struct object *obj )
{
    struct thread *thread = (struct thread *)obj;
    assert( obj->ops == &thread_ops );

    assert( !thread->debug_ctx );  /* cannot still be debugging something */
    list_remove( &thread->entry );
    cleanup_thread( thread );
    release_object( thread->process );
    if (thread->id) free_ptid( thread->id );
    if (thread->token) release_object( thread->token );
}

/* dump a thread on stdout for debugging purposes */
static void dump_thread( struct object *obj, int verbose )
{
    struct thread *thread = (struct thread *)obj;
    assert( obj->ops == &thread_ops );

    fprintf( stderr, "Thread id=%04x unix pid=%d unix tid=%d teb=%p state=%d\n",
             thread->id, thread->unix_pid, thread->unix_tid, thread->teb, thread->state );
}

static int thread_signaled( struct object *obj, struct thread *thread )
{
    struct thread *mythread = (struct thread *)obj;
    return (mythread->state == TERMINATED);
}

static unsigned int thread_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= STANDARD_RIGHTS_READ | SYNCHRONIZE;
    if (access & GENERIC_WRITE)   access |= STANDARD_RIGHTS_WRITE | SYNCHRONIZE;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE;
    if (access & GENERIC_ALL)     access |= THREAD_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static void dump_thread_apc( struct object *obj, int verbose )
{
    struct thread_apc *apc = (struct thread_apc *)obj;
    assert( obj->ops == &thread_apc_ops );

    fprintf( stderr, "APC owner=%p type=%u\n", apc->owner, apc->call.type );
}

static int thread_apc_signaled( struct object *obj, struct thread *thread )
{
    struct thread_apc *apc = (struct thread_apc *)obj;
    return apc->executed;
}

static void thread_apc_destroy( struct object *obj )
{
    struct thread_apc *apc = (struct thread_apc *)obj;
    if (apc->caller) release_object( apc->caller );
    if (apc->owner) release_object( apc->owner );
}

/* queue an async procedure call */
static struct thread_apc *create_apc( struct object *owner, const apc_call_t *call_data )
{
    struct thread_apc *apc;

    if ((apc = alloc_object( &thread_apc_ops )))
    {
        apc->call        = *call_data;
        apc->caller      = NULL;
        apc->owner       = owner;
        apc->executed    = 0;
        apc->result.type = APC_NONE;
        if (owner) grab_object( owner );
    }
    return apc;
}

/* get a thread pointer from a thread id (and increment the refcount) */
struct thread *get_thread_from_id( thread_id_t id )
{
    struct object *obj = get_ptid_entry( id );

    if (obj && obj->ops == &thread_ops) return (struct thread *)grab_object( obj );
    set_error( STATUS_INVALID_CID );
    return NULL;
}

/* get a thread from a handle (and increment the refcount) */
struct thread *get_thread_from_handle( obj_handle_t handle, unsigned int access )
{
    return (struct thread *)get_handle_obj( current->process, handle,
                                            access, &thread_ops );
}

/* find a thread from a Unix tid */
struct thread *get_thread_from_tid( int tid )
{
    struct thread *thread;

    LIST_FOR_EACH_ENTRY( thread, &thread_list, struct thread, entry )
    {
        if (thread->unix_tid == tid) return thread;
    }
    return NULL;
}

/* find a thread from a Unix pid */
struct thread *get_thread_from_pid( int pid )
{
    struct thread *thread;

    LIST_FOR_EACH_ENTRY( thread, &thread_list, struct thread, entry )
    {
        if (thread->unix_pid == pid) return thread;
    }
    return NULL;
}

/* set all information about a thread */
static void set_thread_info( struct thread *thread,
                             const struct set_thread_info_request *req )
{
    if (req->mask & SET_THREAD_INFO_PRIORITY)
        thread->priority = req->priority;
    if (req->mask & SET_THREAD_INFO_AFFINITY)
    {
        if (req->affinity != 1) set_error( STATUS_INVALID_PARAMETER );
        else thread->affinity = req->affinity;
    }
    if (req->mask & SET_THREAD_INFO_TOKEN)
        security_set_thread_token( thread, req->token );
}

/* stop a thread (at the Unix level) */
void stop_thread( struct thread *thread )
{
    if (thread->context) return;  /* already inside a debug event, no need for a signal */
    /* can't stop a thread while initialisation is in progress */
    if (is_process_init_done(thread->process)) send_thread_signal( thread, SIGUSR1 );
}

/* suspend a thread */
static int suspend_thread( struct thread *thread )
{
    int old_count = thread->suspend;
    if (thread->suspend < MAXIMUM_SUSPEND_COUNT)
    {
        if (!(thread->process->suspend + thread->suspend++)) stop_thread( thread );
    }
    else set_error( STATUS_SUSPEND_COUNT_EXCEEDED );
    return old_count;
}

/* resume a thread */
static int resume_thread( struct thread *thread )
{
    int old_count = thread->suspend;
    if (thread->suspend > 0)
    {
        if (!(--thread->suspend + thread->process->suspend)) wake_thread( thread );
    }
    return old_count;
}

/* add a thread to an object wait queue; return 1 if OK, 0 on error */
int add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    grab_object( obj );
    entry->obj = obj;
    list_add_tail( &obj->wait_queue, &entry->entry );
    return 1;
}

/* remove a thread from an object wait queue */
void remove_queue( struct object *obj, struct wait_queue_entry *entry )
{
    list_remove( &entry->entry );
    release_object( obj );
}

/* finish waiting */
static void end_wait( struct thread *thread )
{
    struct thread_wait *wait = thread->wait;
    struct wait_queue_entry *entry;
    int i;

    assert( wait );
    thread->wait = wait->next;
    for (i = 0, entry = wait->queues; i < wait->count; i++, entry++)
        entry->obj->ops->remove_queue( entry->obj, entry );
    if (wait->user) remove_timeout_user( wait->user );
    free( wait );
}

/* build the thread wait structure */
static int wait_on( int count, struct object *objects[], int flags, const abs_time_t *timeout )
{
    struct thread_wait *wait;
    struct wait_queue_entry *entry;
    int i;

    if (!(wait = mem_alloc( sizeof(*wait) + (count-1) * sizeof(*entry) ))) return 0;
    wait->next    = current->wait;
    wait->thread  = current;
    wait->count   = count;
    wait->flags   = flags;
    wait->user    = NULL;
    current->wait = wait;
    if (flags & SELECT_TIMEOUT)
    {
        wait->timeout.tv_sec  = timeout->sec;
        wait->timeout.tv_usec = timeout->usec;
    }

    for (i = 0, entry = wait->queues; i < count; i++, entry++)
    {
        struct object *obj = objects[i];
        entry->thread = current;
        if (!obj->ops->add_queue( obj, entry ))
        {
            wait->count = i;
            end_wait( current );
            return 0;
        }
    }
    return 1;
}

/* check if the thread waiting condition is satisfied */
static int check_wait( struct thread *thread )
{
    int i, signaled;
    struct thread_wait *wait = thread->wait;
    struct wait_queue_entry *entry = wait->queues;

    /* Suspended threads may not acquire locks, but they can run system APCs */
    if (thread->process->suspend + thread->suspend > 0)
    {
        if ((wait->flags & SELECT_INTERRUPTIBLE) && !list_empty( &thread->system_apc ))
            return STATUS_USER_APC;
        return -1;
    }

    assert( wait );
    if (wait->flags & SELECT_ALL)
    {
        int not_ok = 0;
        /* Note: we must check them all anyway, as some objects may
         * want to do something when signaled, even if others are not */
        for (i = 0, entry = wait->queues; i < wait->count; i++, entry++)
            not_ok |= !entry->obj->ops->signaled( entry->obj, thread );
        if (not_ok) goto other_checks;
        /* Wait satisfied: tell it to all objects */
        signaled = 0;
        for (i = 0, entry = wait->queues; i < wait->count; i++, entry++)
            if (entry->obj->ops->satisfied( entry->obj, thread ))
                signaled = STATUS_ABANDONED_WAIT_0;
        return signaled;
    }
    else
    {
        for (i = 0, entry = wait->queues; i < wait->count; i++, entry++)
        {
            if (!entry->obj->ops->signaled( entry->obj, thread )) continue;
            /* Wait satisfied: tell it to the object */
            signaled = i;
            if (entry->obj->ops->satisfied( entry->obj, thread ))
                signaled = i + STATUS_ABANDONED_WAIT_0;
            return signaled;
        }
    }

 other_checks:
    if ((wait->flags & SELECT_INTERRUPTIBLE) && !list_empty(&thread->system_apc)) return STATUS_USER_APC;
    if ((wait->flags & SELECT_ALERTABLE) && !list_empty(&thread->user_apc)) return STATUS_USER_APC;
    if (wait->flags & SELECT_TIMEOUT)
    {
        if (!time_before( &current_time, &wait->timeout )) return STATUS_TIMEOUT;
    }
    return -1;
}

/* send the wakeup signal to a thread */
static int send_thread_wakeup( struct thread *thread, void *cookie, int signaled )
{
    struct wake_up_reply reply;
    int ret;

    reply.cookie   = cookie;
    reply.signaled = signaled;
    if ((ret = write( get_unix_fd( thread->wait_fd ), &reply, sizeof(reply) )) == sizeof(reply))
        return 0;
    if (ret >= 0)
        fatal_protocol_error( thread, "partial wakeup write %d\n", ret );
    else if (errno == EPIPE)
        kill_thread( thread, 0 );  /* normal death */
    else
        fatal_protocol_perror( thread, "write" );
    return -1;
}

/* attempt to wake up a thread */
/* return >0 if OK, 0 if the wait condition is still not satisfied */
int wake_thread( struct thread *thread )
{
    int signaled, count;
    void *cookie;

    for (count = 0; thread->wait; count++)
    {
        if ((signaled = check_wait( thread )) == -1) break;

        cookie = thread->wait->cookie;
        if (debug_level) fprintf( stderr, "%04x: *wakeup* signaled=%d cookie=%p\n",
                                  thread->id, signaled, cookie );
        end_wait( thread );
        if (send_thread_wakeup( thread, cookie, signaled ) == -1) /* error */
	    break;
    }
    return count;
}

/* thread wait timeout */
static void thread_timeout( void *ptr )
{
    struct thread_wait *wait = ptr;
    struct thread *thread = wait->thread;
    void *cookie = wait->cookie;

    wait->user = NULL;
    if (thread->wait != wait) return; /* not the top-level wait, ignore it */
    if (thread->suspend + thread->process->suspend > 0) return;  /* suspended, ignore it */

    if (debug_level) fprintf( stderr, "%04x: *wakeup* signaled=%d cookie=%p\n",
                              thread->id, (int)STATUS_TIMEOUT, cookie );
    end_wait( thread );
    if (send_thread_wakeup( thread, cookie, STATUS_TIMEOUT ) == -1) return;
    /* check if other objects have become signaled in the meantime */
    wake_thread( thread );
}

/* try signaling an event flag, a semaphore or a mutex */
static int signal_object( obj_handle_t handle )
{
    struct object *obj;
    int ret = 0;

    obj = get_handle_obj( current->process, handle, 0, NULL );
    if (obj)
    {
        ret = obj->ops->signal( obj, get_handle_access( current->process, handle ));
        release_object( obj );
    }
    return ret;
}

/* select on a list of handles */
static void select_on( int count, void *cookie, const obj_handle_t *handles,
                       int flags, const abs_time_t *timeout, obj_handle_t signal_obj )
{
    int ret, i;
    struct object *objects[MAXIMUM_WAIT_OBJECTS];

    if ((count < 0) || (count > MAXIMUM_WAIT_OBJECTS))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    for (i = 0; i < count; i++)
    {
        if (!(objects[i] = get_handle_obj( current->process, handles[i], SYNCHRONIZE, NULL )))
            break;
    }

    if (i < count) goto done;
    if (!wait_on( count, objects, flags, timeout )) goto done;

    /* signal the object */
    if (signal_obj)
    {
        if (!signal_object( signal_obj ))
        {
            end_wait( current );
            goto done;
        }
        /* check if we woke ourselves up */
        if (!current->wait) goto done;
    }

    if ((ret = check_wait( current )) != -1)
    {
        /* condition is already satisfied */
        end_wait( current );
        set_error( ret );
        goto done;
    }

    /* now we need to wait */
    if (flags & SELECT_TIMEOUT)
    {
        if (!(current->wait->user = add_timeout_user( &current->wait->timeout,
                                                      thread_timeout, current->wait )))
        {
            end_wait( current );
            goto done;
        }
    }
    current->wait->cookie = cookie;
    set_error( STATUS_PENDING );

done:
    while (--i >= 0) release_object( objects[i] );
}

/* attempt to wake threads sleeping on the object wait queue */
void wake_up( struct object *obj, int max )
{
    struct list *ptr, *next;

    LIST_FOR_EACH_SAFE( ptr, next, &obj->wait_queue )
    {
        struct wait_queue_entry *entry = LIST_ENTRY( ptr, struct wait_queue_entry, entry );
        if (wake_thread( entry->thread ))
        {
            if (max && !--max) break;
        }
    }
}

/* return the apc queue to use for a given apc type */
static inline struct list *get_apc_queue( struct thread *thread, enum apc_type type )
{
    switch(type)
    {
    case APC_NONE:
    case APC_USER:
    case APC_TIMER:
        return &thread->user_apc;
    default:
        return &thread->system_apc;
    }
}

/* queue an existing APC to a given thread */
static int queue_apc( struct process *process, struct thread *thread, struct thread_apc *apc )
{
    struct list *queue;

    if (!thread)  /* find a suitable thread inside the process */
    {
        struct thread *candidate;

        /* first try to find a waiting thread */
        LIST_FOR_EACH_ENTRY( candidate, &process->thread_list, struct thread, proc_entry )
        {
            if (candidate->state == TERMINATED) continue;
            if (process->suspend || candidate->suspend ||
                (candidate->wait && (candidate->wait->flags & SELECT_INTERRUPTIBLE)))
            {
                thread = candidate;
                break;
            }
        }
        if (!thread)
        {
            /* then use the first one that accepts a signal */
            LIST_FOR_EACH_ENTRY( candidate, &process->thread_list, struct thread, proc_entry )
            {
                if (send_thread_signal( candidate, SIGUSR1 ))
                {
                    thread = candidate;
                    break;
                }
            }
        }
        if (!thread) return 0;  /* nothing found */
    }
    else
    {
        if (thread->state == TERMINATED) return 0;
        /* cancel a possible previous APC with the same owner */
        if (apc->owner) thread_cancel_apc( thread, apc->owner, apc->call.type );
    }

    queue = get_apc_queue( thread, apc->call.type );
    grab_object( apc );
    list_add_tail( queue, &apc->entry );
    if (!list_prev( queue, &apc->entry ))  /* first one */
        wake_thread( thread );

    return 1;
}

/* queue an async procedure call */
int thread_queue_apc( struct thread *thread, struct object *owner, const apc_call_t *call_data )
{
    struct thread_apc *apc;
    int ret = 0;

    if ((apc = create_apc( owner, call_data )))
    {
        ret = queue_apc( NULL, thread, apc );
        release_object( apc );
    }
    return ret;
}

/* cancel the async procedure call owned by a specific object */
void thread_cancel_apc( struct thread *thread, struct object *owner, enum apc_type type )
{
    struct thread_apc *apc;
    struct list *queue = get_apc_queue( thread, type );

    LIST_FOR_EACH_ENTRY( apc, queue, struct thread_apc, entry )
    {
        if (apc->owner != owner) continue;
        list_remove( &apc->entry );
        apc->executed = 1;
        wake_up( &apc->obj, 0 );
        release_object( apc );
        return;
    }
}

/* remove the head apc from the queue; the returned object must be released by the caller */
static struct thread_apc *thread_dequeue_apc( struct thread *thread, int system_only )
{
    struct thread_apc *apc = NULL;
    struct list *ptr = list_head( &thread->system_apc );

    if (!ptr && !system_only) ptr = list_head( &thread->user_apc );
    if (ptr)
    {
        apc = LIST_ENTRY( ptr, struct thread_apc, entry );
        list_remove( ptr );
    }
    return apc;
}

/* clear an APC queue, cancelling all the APCs on it */
static void clear_apc_queue( struct list *queue )
{
    struct list *ptr;

    while ((ptr = list_head( queue )))
    {
        struct thread_apc *apc = LIST_ENTRY( ptr, struct thread_apc, entry );
        list_remove( &apc->entry );
        apc->executed = 1;
        wake_up( &apc->obj, 0 );
        release_object( apc );
    }
}

/* add an fd to the inflight list */
/* return list index, or -1 on error */
int thread_add_inflight_fd( struct thread *thread, int client, int server )
{
    int i;

    if (server == -1) return -1;
    if (client == -1)
    {
        close( server );
        return -1;
    }

    /* first check if we already have an entry for this fd */
    for (i = 0; i < MAX_INFLIGHT_FDS; i++)
        if (thread->inflight[i].client == client)
        {
            close( thread->inflight[i].server );
            thread->inflight[i].server = server;
            return i;
        }

    /* now find a free spot to store it */
    for (i = 0; i < MAX_INFLIGHT_FDS; i++)
        if (thread->inflight[i].client == -1)
        {
            thread->inflight[i].client = client;
            thread->inflight[i].server = server;
            return i;
        }
    return -1;
}

/* get an inflight fd and purge it from the list */
/* the fd must be closed when no longer used */
int thread_get_inflight_fd( struct thread *thread, int client )
{
    int i, ret;

    if (client == -1) return -1;

    do
    {
        for (i = 0; i < MAX_INFLIGHT_FDS; i++)
        {
            if (thread->inflight[i].client == client)
            {
                ret = thread->inflight[i].server;
                thread->inflight[i].server = thread->inflight[i].client = -1;
                return ret;
            }
        }
    } while (!receive_fd( thread->process ));  /* in case it is still in the socket buffer */
    return -1;
}

/* kill a thread on the spot */
void kill_thread( struct thread *thread, int violent_death )
{
    if (thread->state == TERMINATED) return;  /* already killed */
    thread->state = TERMINATED;
    thread->exit_time = current_time;
    if (current == thread) current = NULL;
    if (debug_level)
        fprintf( stderr,"%04x: *killed* exit_code=%d\n",
                 thread->id, thread->exit_code );
    if (thread->wait)
    {
        while (thread->wait) end_wait( thread );
        send_thread_wakeup( thread, NULL, STATUS_PENDING );
        /* if it is waiting on the socket, we don't need to send a SIGTERM */
        violent_death = 0;
    }
    kill_console_processes( thread, 0 );
    debug_exit_thread( thread );
    abandon_mutexes( thread );
    wake_up( &thread->obj, 0 );
    if (violent_death) send_thread_signal( thread, SIGTERM );
    cleanup_thread( thread );
    remove_process_thread( thread->process, thread );
    release_object( thread );
}

/* trigger a breakpoint event in a given thread */
void break_thread( struct thread *thread )
{
    struct debug_event_exception data;

    assert( thread->context );

    data.record.ExceptionCode    = STATUS_BREAKPOINT;
    data.record.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    data.record.ExceptionRecord  = NULL;
    data.record.ExceptionAddress = get_context_ip( thread->context );
    data.record.NumberParameters = 0;
    data.first = 1;
    generate_debug_event( thread, EXCEPTION_DEBUG_EVENT, &data );
    thread->debug_break = 0;
}

/* take a snapshot of currently running threads */
struct thread_snapshot *thread_snap( int *count )
{
    struct thread_snapshot *snapshot, *ptr;
    struct thread *thread;
    int total = 0;

    LIST_FOR_EACH_ENTRY( thread, &thread_list, struct thread, entry )
        if (thread->state != TERMINATED) total++;
    if (!total || !(snapshot = mem_alloc( sizeof(*snapshot) * total ))) return NULL;
    ptr = snapshot;
    LIST_FOR_EACH_ENTRY( thread, &thread_list, struct thread, entry )
    {
        if (thread->state == TERMINATED) continue;
        ptr->thread   = thread;
        ptr->count    = thread->obj.refcount;
        ptr->priority = thread->priority;
        grab_object( thread );
        ptr++;
    }
    *count = total;
    return snapshot;
}

/* gets the current impersonation token */
struct token *thread_get_impersonation_token( struct thread *thread )
{
    if (thread->token)
        return thread->token;
    else
        return thread->process->token;
}

/* create a new thread */
DECL_HANDLER(new_thread)
{
    struct thread *thread;
    int request_fd = thread_get_inflight_fd( current, req->request_fd );

    if (request_fd == -1 || fcntl( request_fd, F_SETFL, O_NONBLOCK ) == -1)
    {
        if (request_fd != -1) close( request_fd );
        set_error( STATUS_INVALID_HANDLE );
        return;
    }

    if ((thread = create_thread( request_fd, current->process )))
    {
        if (req->suspend) thread->suspend++;
        reply->tid = get_thread_id( thread );
        if ((reply->handle = alloc_handle( current->process, thread, req->access, req->attributes )))
        {
            /* thread object will be released when the thread gets killed */
            return;
        }
        kill_thread( thread, 1 );
    }
}

/* initialize a new thread */
DECL_HANDLER(init_thread)
{
    struct process *process = current->process;
    int reply_fd = thread_get_inflight_fd( current, req->reply_fd );
    int wait_fd = thread_get_inflight_fd( current, req->wait_fd );

    if (current->reply_fd)  /* already initialised */
    {
        set_error( STATUS_INVALID_PARAMETER );
        goto error;
    }

    if (reply_fd == -1 || fcntl( reply_fd, F_SETFL, O_NONBLOCK ) == -1) goto error;

    current->reply_fd = create_anonymous_fd( &thread_fd_ops, reply_fd, &current->obj, 0 );
    reply_fd = -1;
    if (!current->reply_fd) goto error;

    if (wait_fd == -1)
    {
        set_error( STATUS_TOO_MANY_OPENED_FILES );  /* most likely reason */
        return;
    }
    if (!(current->wait_fd  = create_anonymous_fd( &thread_fd_ops, wait_fd, &current->obj, 0 )))
        return;

    if (!is_valid_address(req->teb) || !is_valid_address(req->peb) || !is_valid_address(req->ldt_copy))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    current->unix_pid = req->unix_pid;
    current->unix_tid = req->unix_tid;
    current->teb      = req->teb;

    if (!process->peb)  /* first thread, initialize the process too */
    {
        process->unix_pid = current->unix_pid;
        process->peb      = req->peb;
        process->ldt_copy = req->ldt_copy;
        reply->info_size  = init_process( current );
    }
    else
    {
        if (process->unix_pid != current->unix_pid)
            process->unix_pid = -1;  /* can happen with linuxthreads */
        if (current->suspend + process->suspend > 0) stop_thread( current );
        generate_debug_event( current, CREATE_THREAD_DEBUG_EVENT, req->entry );
    }
    debug_level = max( debug_level, req->debug_level );

    reply->pid     = get_process_id( process );
    reply->tid     = get_thread_id( current );
    reply->version = SERVER_PROTOCOL_VERSION;
    reply->server_start.sec  = server_start_time.tv_sec;
    reply->server_start.usec = server_start_time.tv_usec;
    return;

 error:
    if (reply_fd != -1) close( reply_fd );
    if (wait_fd != -1) close( wait_fd );
}

/* terminate a thread */
DECL_HANDLER(terminate_thread)
{
    struct thread *thread;

    reply->self = 0;
    reply->last = 0;
    if ((thread = get_thread_from_handle( req->handle, THREAD_TERMINATE )))
    {
        thread->exit_code = req->exit_code;
        if (thread != current) kill_thread( thread, 1 );
        else
        {
            reply->self = 1;
            reply->last = (thread->process->running_threads == 1);
        }
        release_object( thread );
    }
}

/* open a handle to a thread */
DECL_HANDLER(open_thread)
{
    struct thread *thread = get_thread_from_id( req->tid );

    reply->handle = 0;
    if (thread)
    {
        reply->handle = alloc_handle( current->process, thread, req->access, req->attributes );
        release_object( thread );
    }
}

/* fetch information about a thread */
DECL_HANDLER(get_thread_info)
{
    struct thread *thread;
    obj_handle_t handle = req->handle;

    if (!handle) thread = get_thread_from_id( req->tid_in );
    else thread = get_thread_from_handle( req->handle, THREAD_QUERY_INFORMATION );

    if (thread)
    {
        reply->pid            = get_process_id( thread->process );
        reply->tid            = get_thread_id( thread );
        reply->teb            = thread->teb;
        reply->exit_code      = (thread->state == TERMINATED) ? thread->exit_code : STATUS_PENDING;
        reply->priority       = thread->priority;
        reply->affinity       = thread->affinity;
        reply->creation_time.sec  = thread->creation_time.tv_sec;
        reply->creation_time.usec = thread->creation_time.tv_usec;
        reply->exit_time.sec  = thread->exit_time.tv_sec;
        reply->exit_time.usec = thread->exit_time.tv_usec;
        reply->last           = thread->process->running_threads == 1;

        release_object( thread );
    }
}

/* set information about a thread */
DECL_HANDLER(set_thread_info)
{
    struct thread *thread;

    if ((thread = get_thread_from_handle( req->handle, THREAD_SET_INFORMATION )))
    {
        set_thread_info( thread, req );
        release_object( thread );
    }
}

/* suspend a thread */
DECL_HANDLER(suspend_thread)
{
    struct thread *thread;

    if ((thread = get_thread_from_handle( req->handle, THREAD_SUSPEND_RESUME )))
    {
        if (thread->state == TERMINATED) set_error( STATUS_ACCESS_DENIED );
        else reply->count = suspend_thread( thread );
        release_object( thread );
    }
}

/* resume a thread */
DECL_HANDLER(resume_thread)
{
    struct thread *thread;

    if ((thread = get_thread_from_handle( req->handle, THREAD_SUSPEND_RESUME )))
    {
        if (thread->state == TERMINATED) set_error( STATUS_ACCESS_DENIED );
        else reply->count = resume_thread( thread );
        release_object( thread );
    }
}

/* select on a handle list */
DECL_HANDLER(select)
{
    int count = get_req_data_size() / sizeof(obj_handle_t);
    select_on( count, req->cookie, get_req_data(), req->flags, &req->timeout, req->signal );
}

/* queue an APC for a thread or process */
DECL_HANDLER(queue_apc)
{
    struct thread *thread = NULL;
    struct process *process = NULL;
    struct thread_apc *apc;

    if (!(apc = create_apc( NULL, &req->call ))) return;

    switch (apc->call.type)
    {
    case APC_NONE:
    case APC_USER:
        thread = get_thread_from_handle( req->thread, THREAD_SET_CONTEXT );
        break;
    case APC_VIRTUAL_ALLOC:
    case APC_VIRTUAL_FREE:
    case APC_VIRTUAL_PROTECT:
    case APC_VIRTUAL_FLUSH:
    case APC_VIRTUAL_LOCK:
    case APC_VIRTUAL_UNLOCK:
    case APC_UNMAP_VIEW:
        process = get_process_from_handle( req->process, PROCESS_VM_OPERATION );
        break;
    case APC_VIRTUAL_QUERY:
        process = get_process_from_handle( req->process, PROCESS_QUERY_INFORMATION );
        break;
    case APC_MAP_VIEW:
        process = get_process_from_handle( req->process, PROCESS_VM_OPERATION );
        if (process && process != current->process)
        {
            /* duplicate the handle into the target process */
            obj_handle_t handle = duplicate_handle( current->process, apc->call.map_view.handle,
                                                    process, 0, 0, DUP_HANDLE_SAME_ACCESS );
            if (handle) apc->call.map_view.handle = handle;
            else
            {
                release_object( process );
                process = NULL;
            }
        }
        break;
    case APC_CREATE_THREAD:
        process = get_process_from_handle( req->process, PROCESS_CREATE_THREAD );
        break;
    default:
        set_error( STATUS_INVALID_PARAMETER );
        break;
    }

    if (thread)
    {
        if (!queue_apc( NULL, thread, apc )) set_error( STATUS_THREAD_IS_TERMINATING );
        release_object( thread );
    }
    else if (process)
    {
        reply->self = (process == current->process);
        if (!reply->self)
        {
            obj_handle_t handle = alloc_handle( current->process, apc, SYNCHRONIZE, 0 );
            if (handle)
            {
                if (queue_apc( process, NULL, apc ))
                {
                    apc->caller = (struct thread *)grab_object( current );
                    reply->handle = handle;
                }
                else
                {
                    close_handle( current->process, handle );
                    set_error( STATUS_PROCESS_IS_TERMINATING );
                }
            }
        }
        release_object( process );
    }

    release_object( apc );
}

/* get next APC to call */
DECL_HANDLER(get_apc)
{
    struct thread_apc *apc;
    int system_only = !req->alertable;

    if (req->prev)
    {
        if (!(apc = (struct thread_apc *)get_handle_obj( current->process, req->prev,
                                                         0, &thread_apc_ops ))) return;
        apc->result = req->result;
        apc->executed = 1;
        if (apc->result.type == APC_CREATE_THREAD)  /* transfer the handle to the caller process */
        {
            obj_handle_t handle = duplicate_handle( current->process, apc->result.create_thread.handle,
                                                    apc->caller->process, 0, 0, DUP_HANDLE_SAME_ACCESS );
            close_handle( current->process, apc->result.create_thread.handle );
            apc->result.create_thread.handle = handle;
            clear_error();  /* ignore errors from the above calls */
        }
        else if (apc->result.type == APC_ASYNC_IO)
        {
            if (apc->owner) async_set_result( apc->owner, apc->result.async_io.status );
        }
        wake_up( &apc->obj, 0 );
        close_handle( current->process, req->prev );
        release_object( apc );
    }

    if (current->suspend + current->process->suspend > 0) system_only = 1;

    for (;;)
    {
        if (!(apc = thread_dequeue_apc( current, system_only )))
        {
            /* no more APCs */
            set_error( STATUS_PENDING );
            return;
        }
        /* Optimization: ignore APC_NONE calls, they are only used to
         * wake up a thread, but since we got here the thread woke up already.
         */
        if (apc->call.type != APC_NONE) break;
        apc->executed = 1;
        wake_up( &apc->obj, 0 );
        release_object( apc );
    }

    if ((reply->handle = alloc_handle( current->process, apc, SYNCHRONIZE, 0 )))
        reply->call = apc->call;
    release_object( apc );
}

/* Get the result of an APC call */
DECL_HANDLER(get_apc_result)
{
    struct thread_apc *apc;

    if (!(apc = (struct thread_apc *)get_handle_obj( current->process, req->handle,
                                                     0, &thread_apc_ops ))) return;
    if (!apc->executed) set_error( STATUS_PENDING );
    else
    {
        reply->result = apc->result;
        /* close the handle directly to avoid an extra round-trip */
        close_handle( current->process, req->handle );
    }
    release_object( apc );
}

/* retrieve the current context of a thread */
DECL_HANDLER(get_thread_context)
{
    struct thread *thread;
    CONTEXT *context;

    if (get_reply_max_size() < sizeof(CONTEXT))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    if (!(thread = get_thread_from_handle( req->handle, THREAD_GET_CONTEXT ))) return;

    if (req->suspend)
    {
        if (thread != current || !thread->suspend_context)
        {
            /* not suspended, shouldn't happen */
            set_error( STATUS_INVALID_PARAMETER );
        }
        else
        {
            if (thread->context == thread->suspend_context) thread->context = NULL;
            set_reply_data_ptr( thread->suspend_context, sizeof(CONTEXT) );
            thread->suspend_context = NULL;
        }
    }
    else if (thread != current && !thread->context)
    {
        /* thread is not suspended, retry (if it's still running) */
        if (thread->state != RUNNING) set_error( STATUS_ACCESS_DENIED );
        else set_error( STATUS_PENDING );
    }
    else if ((context = set_reply_data_size( sizeof(CONTEXT) )))
    {
        unsigned int flags = get_context_system_regs( req->flags );

        memset( context, 0, sizeof(CONTEXT) );
        context->ContextFlags = get_context_cpu_flag();
        if (thread->context) copy_context( context, thread->context, req->flags & ~flags );
        if (flags) get_thread_context( thread, context, flags );
    }
    reply->self = (thread == current);
    release_object( thread );
}

/* set the current context of a thread */
DECL_HANDLER(set_thread_context)
{
    struct thread *thread;

    if (get_req_data_size() < sizeof(CONTEXT))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    if (!(thread = get_thread_from_handle( req->handle, THREAD_SET_CONTEXT ))) return;

    if (req->suspend)
    {
        if (thread != current || thread->context)
        {
            /* nested suspend or exception, shouldn't happen */
            set_error( STATUS_INVALID_PARAMETER );
        }
        else if ((thread->suspend_context = mem_alloc( sizeof(CONTEXT) )))
        {
            memcpy( thread->suspend_context, get_req_data(), sizeof(CONTEXT) );
            thread->context = thread->suspend_context;
            if (thread->debug_break) break_thread( thread );
        }
    }
    else if (thread != current && !thread->context)
    {
        /* thread is not suspended, retry (if it's still running) */
        if (thread->state != RUNNING) set_error( STATUS_ACCESS_DENIED );
        else set_error( STATUS_PENDING );
    }
    else
    {
        const CONTEXT *context = get_req_data();
        unsigned int flags = get_context_system_regs( req->flags );

        if (flags) set_thread_context( thread, context, flags );
        if (thread->context && !get_error())
            copy_context( thread->context, context, req->flags & ~flags );
    }
    reply->self = (thread == current);
    release_object( thread );
}

/* fetch a selector entry for a thread */
DECL_HANDLER(get_selector_entry)
{
    struct thread *thread;
    if ((thread = get_thread_from_handle( req->handle, THREAD_QUERY_INFORMATION )))
    {
        get_selector_entry( thread, req->entry, &reply->base, &reply->limit, &reply->flags );
        release_object( thread );
    }
}
