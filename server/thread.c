/*
 * Server-side thread management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include "config.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include <sys/uio.h>
#include <unistd.h>
#include <stdarg.h>

#include "winbase.h"

#include "handle.h"
#include "process.h"
#include "thread.h"
#include "request.h"


/* thread queues */

struct thread_wait
{
    int                     count;      /* count of objects */
    int                     flags;
    struct timeval          timeout;
    struct timeout_user    *user;
    struct wait_queue_entry queues[1];
};

/* asynchronous procedure calls */

struct thread_apc
{
    struct thread_apc  *next;     /* queue linked list */
    struct thread_apc  *prev;
    struct object      *owner;    /* object that queued this apc */
    void               *func;     /* function to call in client */
    enum apc_type       type;     /* type of apc function */
    int                 nb_args;  /* number of arguments */
    void               *args[1];  /* function arguments */
};


/* thread operations */

static void dump_thread( struct object *obj, int verbose );
static int thread_signaled( struct object *obj, struct thread *thread );
extern void thread_poll_event( struct object *obj, int event );
static void destroy_thread( struct object *obj );
static struct thread_apc *thread_dequeue_apc( struct thread *thread, int system_only );

static const struct object_ops thread_ops =
{
    sizeof(struct thread),      /* size */
    dump_thread,                /* dump */
    add_queue,                  /* add_queue */
    remove_queue,               /* remove_queue */
    thread_signaled,            /* signaled */
    no_satisfied,               /* satisfied */
    NULL,                       /* get_poll_events */
    thread_poll_event,          /* poll_event */
    no_get_fd,                  /* get_fd */
    no_flush,                   /* flush */
    no_get_file_info,           /* get_file_info */
    destroy_thread              /* destroy */
};

static struct thread *first_thread;
static struct thread *booting_thread;

/* allocate the buffer for the communication with the client */
static int alloc_client_buffer( struct thread *thread )
{
    union generic_request *req;
    int fd = -1, fd_pipe[2], wait_pipe[2];

    wait_pipe[0] = wait_pipe[1] = -1;
    if (pipe( fd_pipe ) == -1)
    {
        file_set_error();
        return 0;
    }
    if (pipe( wait_pipe ) == -1) goto error;
    if ((fd = create_anonymous_file()) == -1) goto error;
    if (ftruncate( fd, MAX_REQUEST_LENGTH ) == -1) goto error;
    if ((thread->buffer = mmap( 0, MAX_REQUEST_LENGTH, PROT_READ | PROT_WRITE,
                                MAP_SHARED, fd, 0 )) == (void*)-1) goto error;
    if (!(thread->request_fd = create_request_socket( thread ))) goto error;
    thread->reply_fd = fd_pipe[1];
    thread->wait_fd  = wait_pipe[1];

    /* make the pipes non-blocking */
    fcntl( fd_pipe[1], F_SETFL, O_NONBLOCK );
    fcntl( wait_pipe[1], F_SETFL, O_NONBLOCK );

    /* build the first request into the buffer and send it */
    req = thread->buffer;
    req->get_thread_buffer.pid  = get_process_id( thread->process );
    req->get_thread_buffer.tid  = get_thread_id( thread );
    req->get_thread_buffer.boot = (thread == booting_thread);
    req->get_thread_buffer.version = SERVER_PROTOCOL_VERSION;

    /* add it here since send_client_fd may call kill_thread */
    add_process_thread( thread->process, thread );

    send_client_fd( thread, fd_pipe[0], 0 );
    send_client_fd( thread, wait_pipe[0], 0 );
    send_client_fd( thread, fd, 0 );
    send_reply( thread, req );
    close( fd_pipe[0] );
    close( wait_pipe[0] );
    close( fd );

    return 1;

 error:
    file_set_error();
    if (fd != -1) close( fd );
    close( fd_pipe[0] );
    close( fd_pipe[1] );
    if (wait_pipe[0] != -1) close( wait_pipe[0] );
    if (wait_pipe[1] != -1) close( wait_pipe[1] );
    return 0;
}

/* create a new thread */
struct thread *create_thread( int fd, struct process *process )
{
    struct thread *thread;

    int flags = fcntl( fd, F_GETFL, 0 );
    fcntl( fd, F_SETFL, flags | O_NONBLOCK );

    if (!(thread = alloc_object( &thread_ops, fd ))) return NULL;

    thread->unix_pid    = 0;  /* not known yet */
    thread->context     = NULL;
    thread->teb         = NULL;
    thread->mutex       = NULL;
    thread->debug_ctx   = NULL;
    thread->debug_event = NULL;
    thread->queue       = NULL;
    thread->info        = NULL;
    thread->wait        = NULL;
    thread->system_apc.head = NULL;
    thread->system_apc.tail = NULL;
    thread->user_apc.head   = NULL;
    thread->user_apc.tail   = NULL;
    thread->error       = 0;
    thread->pass_fd     = -1;
    thread->request_fd  = NULL;
    thread->reply_fd    = -1;
    thread->wait_fd     = -1;
    thread->state       = RUNNING;
    thread->attached    = 0;
    thread->exit_code   = 0;
    thread->next        = NULL;
    thread->prev        = NULL;
    thread->priority    = THREAD_PRIORITY_NORMAL;
    thread->affinity    = 1;
    thread->suspend     = 0;
    thread->buffer      = (void *)-1;
    thread->last_req    = REQ_get_thread_buffer;
    thread->process     = (struct process *)grab_object( process );

    if (!current) current = thread;

    if (!booting_thread)  /* first thread ever */
    {
        booting_thread = thread;
        lock_master_socket(1);
    }

    if ((thread->next = first_thread) != NULL) thread->next->prev = thread;
    first_thread = thread;

    set_select_events( &thread->obj, POLLIN );  /* start listening to events */
    if (!alloc_client_buffer( thread )) goto error;
    return thread;

 error:
    release_object( thread );
    return NULL;
}

/* handle a client event */
void thread_poll_event( struct object *obj, int event )
{
    struct thread *thread = (struct thread *)obj;
    assert( obj->ops == &thread_ops );

    if (event & (POLLERR | POLLHUP)) kill_thread( thread, 0 );
    else if (event & POLLIN) read_request( thread );
}

/* destroy a thread when its refcount is 0 */
static void destroy_thread( struct object *obj )
{
    struct thread_apc *apc;
    struct thread *thread = (struct thread *)obj;
    assert( obj->ops == &thread_ops );

    assert( !thread->debug_ctx );  /* cannot still be debugging something */
    release_object( thread->process );
    if (thread->next) thread->next->prev = thread->prev;
    if (thread->prev) thread->prev->next = thread->next;
    else first_thread = thread->next;
    while ((apc = thread_dequeue_apc( thread, 0 ))) free( apc );
    if (thread->info) release_object( thread->info );
    if (thread->queue) release_object( thread->queue );
    if (thread->buffer != (void *)-1) munmap( thread->buffer, MAX_REQUEST_LENGTH );
    if (thread->reply_fd != -1) close( thread->reply_fd );
    if (thread->wait_fd != -1) close( thread->wait_fd );
    if (thread->pass_fd != -1) close( thread->pass_fd );
    if (thread->request_fd) release_object( thread->request_fd );
}

/* dump a thread on stdout for debugging purposes */
static void dump_thread( struct object *obj, int verbose )
{
    struct thread *thread = (struct thread *)obj;
    assert( obj->ops == &thread_ops );

    fprintf( stderr, "Thread pid=%d teb=%p state=%d\n",
             thread->unix_pid, thread->teb, thread->state );
}

static int thread_signaled( struct object *obj, struct thread *thread )
{
    struct thread *mythread = (struct thread *)obj;
    return (mythread->state == TERMINATED);
}

/* get a thread pointer from a thread id (and increment the refcount) */
struct thread *get_thread_from_id( void *id )
{
    struct thread *t = first_thread;
    while (t && (t != id)) t = t->next;
    if (t) grab_object( t );
    return t;
}

/* get a thread from a handle (and increment the refcount) */
struct thread *get_thread_from_handle( handle_t handle, unsigned int access )
{
    return (struct thread *)get_handle_obj( current->process, handle,
                                            access, &thread_ops );
}

/* find a thread from a Unix pid */
struct thread *get_thread_from_pid( int pid )
{
    struct thread *t = first_thread;
    while (t && (t->unix_pid != pid)) t = t->next;
    return t;
}

/* set all information about a thread */
static void set_thread_info( struct thread *thread,
                             struct set_thread_info_request *req )
{
    if (req->mask & SET_THREAD_INFO_PRIORITY)
        thread->priority = req->priority;
    if (req->mask & SET_THREAD_INFO_AFFINITY)
    {
        if (req->affinity != 1) set_error( STATUS_INVALID_PARAMETER );
        else thread->affinity = req->affinity;
    }
}

/* suspend a thread */
int suspend_thread( struct thread *thread, int check_limit )
{
    int old_count = thread->suspend;
    if (thread->suspend < MAXIMUM_SUSPEND_COUNT || !check_limit)
    {
        if (!(thread->process->suspend + thread->suspend++)) stop_thread( thread );
    }
    else set_error( STATUS_SUSPEND_COUNT_EXCEEDED );
    return old_count;
}

/* resume a thread */
int resume_thread( struct thread *thread )
{
    int old_count = thread->suspend;
    if (thread->suspend > 0)
    {
        if (!(--thread->suspend + thread->process->suspend)) continue_thread( thread );
    }
    return old_count;
}

/* suspend all threads but the current */
void suspend_all_threads( void )
{
    struct thread *thread;
    for ( thread = first_thread; thread; thread = thread->next )
        if ( thread != current )
            suspend_thread( thread, 0 );
}

/* resume all threads but the current */
void resume_all_threads( void )
{
    struct thread *thread;
    for ( thread = first_thread; thread; thread = thread->next )
        if ( thread != current )
            resume_thread( thread );
}

/* add a thread to an object wait queue; return 1 if OK, 0 on error */
int add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    grab_object( obj );
    entry->obj    = obj;
    entry->prev   = obj->tail;
    entry->next   = NULL;
    if (obj->tail) obj->tail->next = entry;
    else obj->head = entry;
    obj->tail = entry;
    return 1;
}

/* remove a thread from an object wait queue */
void remove_queue( struct object *obj, struct wait_queue_entry *entry )
{
    if (entry->next) entry->next->prev = entry->prev;
    else obj->tail = entry->prev;
    if (entry->prev) entry->prev->next = entry->next;
    else obj->head = entry->next;
    release_object( obj );
}

/* finish waiting */
static void end_wait( struct thread *thread )
{
    struct thread_wait *wait = thread->wait;
    struct wait_queue_entry *entry;
    int i;

    assert( wait );
    for (i = 0, entry = wait->queues; i < wait->count; i++, entry++)
        entry->obj->ops->remove_queue( entry->obj, entry );
    if (wait->user) remove_timeout_user( wait->user );
    free( wait );
    thread->wait = NULL;
}

/* build the thread wait structure */
static int wait_on( int count, struct object *objects[], int flags, int sec, int usec )
{
    struct thread_wait *wait;
    struct wait_queue_entry *entry;
    int i;

    if (!(wait = mem_alloc( sizeof(*wait) + (count-1) * sizeof(*entry) ))) return 0;
    current->wait = wait;
    wait->count   = count;
    wait->flags   = flags;
    wait->user    = NULL;
    if (flags & SELECT_TIMEOUT)
    {
        wait->timeout.tv_sec = sec;
        wait->timeout.tv_usec = usec;
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
    if ((wait->flags & SELECT_INTERRUPTIBLE) && thread->system_apc.head) return STATUS_USER_APC;
    if ((wait->flags & SELECT_ALERTABLE) && thread->user_apc.head) return STATUS_USER_APC;
    if (wait->flags & SELECT_TIMEOUT)
    {
        struct timeval now;
        gettimeofday( &now, NULL );
        if (!time_before( &now, &wait->timeout )) return STATUS_TIMEOUT;
    }
    return -1;
}

/* attempt to wake up a thread */
/* return 1 if OK, 0 if the wait condition is still not satisfied */
static int wake_thread( struct thread *thread )
{
    int signaled;
    if ((signaled = check_wait( thread )) == -1) return 0;

    if (debug_level) fprintf( stderr, "%08x: *wakeup* object=%d\n",
                              (unsigned int)thread, signaled );
    end_wait( thread );
    send_thread_wakeup( thread, signaled );
    return 1;
}

/* thread wait timeout */
static void thread_timeout( void *ptr )
{
    struct thread *thread = ptr;

    if (debug_level) fprintf( stderr, "%08x: *timeout*\n", (unsigned int)thread );

    assert( thread->wait );
    thread->wait->user = NULL;
    end_wait( thread );
    send_thread_wakeup( thread, STATUS_TIMEOUT );
}

/* select on a list of handles */
static void select_on( int count, handle_t *handles, int flags, int sec, int usec )
{
    int ret, i;
    struct object *objects[MAXIMUM_WAIT_OBJECTS];

    assert( !current->wait );

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
    if (!wait_on( count, objects, flags, sec, usec )) goto done;

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
                                                      thread_timeout, current )))
        {
            end_wait( current );
            goto done;
        }
    }
    set_error( STATUS_PENDING );

done:
    while (--i >= 0) release_object( objects[i] );
}

/* attempt to wake threads sleeping on the object wait queue */
void wake_up( struct object *obj, int max )
{
    struct wait_queue_entry *entry = obj->head;

    while (entry)
    {
        struct thread *thread = entry->thread;
        entry = entry->next;
        if (wake_thread( thread ))
        {
            if (max && !--max) break;
        }
    }
}

/* queue an async procedure call */
int thread_queue_apc( struct thread *thread, struct object *owner, void *func,
                      enum apc_type type, int system, int nb_args, ... )
{
    struct thread_apc *apc;
    struct apc_queue *queue = system ? &thread->system_apc : &thread->user_apc;

    /* cancel a possible previous APC with the same owner */
    if (owner) thread_cancel_apc( thread, owner, system );

    if (!(apc = mem_alloc( sizeof(*apc) + (nb_args-1)*sizeof(apc->args[0]) ))) return 0;
    apc->prev    = queue->tail;
    apc->next    = NULL;
    apc->owner   = owner;
    apc->func    = func;
    apc->type    = type;
    apc->nb_args = nb_args;
    if (nb_args)
    {
        int i;
        va_list args;
        va_start( args, nb_args );
        for (i = 0; i < nb_args; i++) apc->args[i] = va_arg( args, void * );
        va_end( args );
    }
    queue->tail = apc;
    if (!apc->prev)  /* first one */
    {
        queue->head = apc;
        if (thread->wait) wake_thread( thread );
    }
    return 1;
}

/* cancel the async procedure call owned by a specific object */
void thread_cancel_apc( struct thread *thread, struct object *owner, int system )
{
    struct thread_apc *apc;
    struct apc_queue *queue = system ? &thread->system_apc : &thread->user_apc;
    for (apc = queue->head; apc; apc = apc->next)
    {
        if (apc->owner != owner) continue;
        if (apc->next) apc->next->prev = apc->prev;
        else queue->tail = apc->prev;
        if (apc->prev) apc->prev->next = apc->next;
        else queue->head = apc->next;
        free( apc );
        return;
    }
}

/* remove the head apc from the queue; the returned pointer must be freed by the caller */
static struct thread_apc *thread_dequeue_apc( struct thread *thread, int system_only )
{
    struct thread_apc *apc;
    struct apc_queue *queue = &thread->system_apc;

    if (!queue->head && !system_only) queue = &thread->user_apc;
    if ((apc = queue->head))
    {
        if (apc->next) apc->next->prev = NULL;
        else queue->tail = NULL;
        queue->head = apc->next;
    }
    return apc;
}

/* retrieve an LDT selector entry */
static void get_selector_entry( struct thread *thread, int entry,
                                unsigned int *base, unsigned int *limit,
                                unsigned char *flags )
{
    if (!thread->process->ldt_copy)
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    if (entry >= 8192)
    {
        set_error( STATUS_INVALID_PARAMETER );  /* FIXME */
        return;
    }
    if (suspend_for_ptrace( thread ))
    {
        unsigned char flags_buf[4];
        int *addr = (int *)thread->process->ldt_copy + entry;
        if (read_thread_int( thread, addr, base ) == -1) goto done;
        if (read_thread_int( thread, addr + 8192, limit ) == -1) goto done;
        addr = (int *)thread->process->ldt_copy + 2*8192 + (entry >> 2);
        if (read_thread_int( thread, addr, (int *)flags_buf ) == -1) goto done;
        *flags = flags_buf[entry & 3];
    done:
        resume_thread( thread );
    }
}

/* kill a thread on the spot */
void kill_thread( struct thread *thread, int violent_death )
{
    if (thread->state == TERMINATED) return;  /* already killed */
    thread->state = TERMINATED;
    if (current == thread) current = NULL;
    if (debug_level)
        fprintf( stderr,"%08x: *killed* exit_code=%d\n",
                 (unsigned int)thread, thread->exit_code );
    if (thread->wait)
    {
        end_wait( thread );
        /* if it is waiting on the socket, we don't need to send a SIGTERM */
        violent_death = 0;
    }
    debug_exit_thread( thread );
    abandon_mutexes( thread );
    remove_process_thread( thread->process, thread );
    wake_up( &thread->obj, 0 );
    detach_thread( thread, violent_death ? SIGTERM : 0 );
    remove_select_user( &thread->obj );
    release_object( thread->request_fd );
    close( thread->reply_fd );
    close( thread->wait_fd );
    munmap( thread->buffer, MAX_REQUEST_LENGTH );
    thread->request_fd = NULL;
    thread->reply_fd = -1;
    thread->wait_fd = -1;
    thread->buffer = (void *)-1;
    release_object( thread );
}

/* take a snapshot of currently running threads */
struct thread_snapshot *thread_snap( int *count )
{
    struct thread_snapshot *snapshot, *ptr;
    struct thread *thread;
    int total = 0;

    for (thread = first_thread; thread; thread = thread->next)
        if (thread->state != TERMINATED) total++;
    if (!total || !(snapshot = mem_alloc( sizeof(*snapshot) * total ))) return NULL;
    ptr = snapshot;
    for (thread = first_thread; thread; thread = thread->next)
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

/* signal that we are finished booting on the client side */
DECL_HANDLER(boot_done)
{
    debug_level = max( debug_level, req->debug_level );
    /* Make sure last_req is initialized */
    current->last_req = REQ_boot_done;
    if (current == booting_thread)
    {
        booting_thread = (struct thread *)~0UL;  /* make sure it doesn't match other threads */
        lock_master_socket(0);  /* allow other clients now */
    }
}

/* create a new thread */
DECL_HANDLER(new_thread)
{
    struct thread *thread;
    int sock[2];

    if (socketpair( AF_UNIX, SOCK_STREAM, 0, sock ) != -1)
    {
        if ((thread = create_thread( sock[0], current->process )))
        {
            if (req->suspend) thread->suspend++;
            req->tid = thread;
            if ((req->handle = alloc_handle( current->process, thread,
                                             THREAD_ALL_ACCESS, req->inherit )))
            {
                send_client_fd( current, sock[1], req->handle );
                close( sock[1] );
                /* thread object will be released when the thread gets killed */
                return;
            }
            kill_thread( thread, 1 );
        }
        close( sock[1] );
    }
    else file_set_error();
}

/* retrieve the thread buffer file descriptor */
DECL_HANDLER(get_thread_buffer)
{
    fatal_protocol_error( current, "get_thread_buffer: should never get called directly\n" );
}

/* initialize a new thread */
DECL_HANDLER(init_thread)
{
    if (current->unix_pid)
    {
        fatal_protocol_error( current, "init_thread: already running\n" );
        return;
    }
    current->unix_pid = req->unix_pid;
    current->teb      = req->teb;
    if (current->suspend + current->process->suspend > 0) stop_thread( current );
    if (current->process->running_threads > 1)
        generate_debug_event( current, CREATE_THREAD_DEBUG_EVENT, req->entry );
}

/* terminate a thread */
DECL_HANDLER(terminate_thread)
{
    struct thread *thread;

    req->self = 0;
    req->last = 0;
    if ((thread = get_thread_from_handle( req->handle, THREAD_TERMINATE )))
    {
        thread->exit_code = req->exit_code;
        if (thread != current) kill_thread( thread, 1 );
        else
        {
            req->self = 1;
            req->last = (thread->process->running_threads == 1);
        }
        release_object( thread );
    }
}

/* fetch information about a thread */
DECL_HANDLER(get_thread_info)
{
    struct thread *thread;
    handle_t handle = req->handle;

    if (!handle) thread = get_thread_from_id( req->tid_in );
    else thread = get_thread_from_handle( req->handle, THREAD_QUERY_INFORMATION );

    if (thread)
    {
        req->tid       = get_thread_id( thread );
        req->teb       = thread->teb;
        req->exit_code = (thread->state == TERMINATED) ? thread->exit_code : STILL_ACTIVE;
        req->priority  = thread->priority;
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
        req->count = suspend_thread( thread, 1 );
        release_object( thread );
    }
}

/* resume a thread */
DECL_HANDLER(resume_thread)
{
    struct thread *thread;

    if ((thread = get_thread_from_handle( req->handle, THREAD_SUSPEND_RESUME )))
    {
        req->count = resume_thread( thread );
        release_object( thread );
    }
}

/* select on a handle list */
DECL_HANDLER(select)
{
    int count = get_req_data_size(req) / sizeof(int);
    select_on( count, get_req_data(req), req->flags, req->sec, req->usec );
}

/* queue an APC for a thread */
DECL_HANDLER(queue_apc)
{
    struct thread *thread;
    if ((thread = get_thread_from_handle( req->handle, THREAD_SET_CONTEXT )))
    {
        thread_queue_apc( thread, NULL, req->func, APC_USER, !req->user, 1, req->param );
        release_object( thread );
    }
}

/* get next APC to call */
DECL_HANDLER(get_apc)
{
    struct thread_apc *apc;
    size_t size;

    for (;;)
    {
        if (!(apc = thread_dequeue_apc( current, !req->alertable )))
        {
            /* no more APCs */
            req->func    = NULL;
            req->type    = APC_NONE;
            set_req_data_size( req, 0 );
            return;
        }
        /* Optimization: ignore APCs that have a NULL func; they are only used
         * to wake up a thread, but since we got here the thread woke up already.
         */
        if (apc->func) break;
        free( apc );
    }
    size = apc->nb_args * sizeof(apc->args[0]);
    if (size > get_req_data_size(req)) size = get_req_data_size(req);
    req->func = apc->func;
    req->type = apc->type;
    memcpy( get_req_data(req), apc->args, size );
    set_req_data_size( req, size );
    free( apc );
}

/* fetch a selector entry for a thread */
DECL_HANDLER(get_selector_entry)
{
    struct thread *thread;
    if ((thread = get_thread_from_handle( req->handle, THREAD_QUERY_INFORMATION )))
    {
        get_selector_entry( thread, req->entry, &req->base, &req->limit, &req->flags );
        release_object( thread );
    }
}
