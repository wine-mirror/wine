/*
 * Server-side thread management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include "config.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
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

struct wait_queue_entry
{
    struct wait_queue_entry *next;
    struct wait_queue_entry *prev;
    struct object           *obj;
    struct thread           *thread;
};

struct thread_wait
{
    int                     count;      /* count of objects */
    int                     flags;
    struct timeval          timeout;
    struct timeout_user    *user;
    sleep_reply             reply;      /* function to build the reply */
    struct wait_queue_entry queues[1];
};

/* asynchronous procedure calls */

struct thread_apc
{
    void                   *func;    /* function to call in client */
    void                   *param;   /* function param */
};
#define MAX_THREAD_APC  16  /* Max outstanding APCs for a thread */


/* thread operations */

static void dump_thread( struct object *obj, int verbose );
static int thread_signaled( struct object *obj, struct thread *thread );
extern void thread_poll_event( struct object *obj, int event );
static void destroy_thread( struct object *obj );

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
    no_read_fd,                 /* get_read_fd */
    no_write_fd,                /* get_write_fd */
    no_flush,                   /* flush */
    no_get_file_info,           /* get_file_info */
    destroy_thread              /* destroy */
};

static struct thread *first_thread;
static struct thread *booting_thread;

/* allocate the buffer for the communication with the client */
static int alloc_client_buffer( struct thread *thread )
{
    struct get_thread_buffer_request *req;
    int fd;

    if ((fd = create_anonymous_file()) == -1) return -1;
    if (ftruncate( fd, MAX_REQUEST_LENGTH ) == -1) goto error;
    if ((thread->buffer = mmap( 0, MAX_REQUEST_LENGTH, PROT_READ | PROT_WRITE,
                                MAP_SHARED, fd, 0 )) == (void*)-1) goto error;
    /* build the first request into the buffer and send it */
    req = thread->buffer;
    req->pid  = get_process_id( thread->process );
    req->tid  = get_thread_id( thread );
    req->boot = (thread == booting_thread);
    set_reply_fd( thread, fd );
    send_reply( thread );
    return 1;

 error:
    file_set_error();
    if (fd != -1) close( fd );
    return 0;
}

/* create a new thread */
struct thread *create_thread( int fd, struct process *process, int suspend )
{
    struct thread *thread;

    int flags = fcntl( fd, F_GETFL, 0 );
    fcntl( fd, F_SETFL, flags | O_NONBLOCK );

    if (!(thread = alloc_object( &thread_ops, fd ))) return NULL;

    thread->unix_pid    = 0;  /* not known yet */
    thread->teb         = NULL;
    thread->mutex       = NULL;
    thread->debug_ctx   = NULL;
    thread->wait        = NULL;
    thread->apc         = NULL;
    thread->apc_count   = 0;
    thread->error       = 0;
    thread->pass_fd     = -1;
    thread->state       = RUNNING;
    thread->attached    = 0;
    thread->exit_code   = STILL_ACTIVE;
    thread->next        = NULL;
    thread->prev        = NULL;
    thread->priority    = THREAD_PRIORITY_NORMAL;
    thread->affinity    = 1;
    thread->suspend     = (suspend != 0);
    thread->buffer      = (void *)-1;
    thread->last_req    = REQ_GET_THREAD_BUFFER;
    thread->process     = (struct process *)grab_object( process );

    if (!current) current = thread;

    if (!booting_thread)  /* first thread ever */
    {
        booting_thread = thread;
        lock_master_socket(1);
    }

    if ((thread->next = first_thread) != NULL) thread->next->prev = thread;
    first_thread = thread;
    add_process_thread( process, thread );

    set_select_events( &thread->obj, POLLIN );  /* start listening to events */
    if (!alloc_client_buffer( thread )) goto error;
    return thread;

 error:
    remove_process_thread( process, thread );
    release_object( thread );
    return NULL;
}

/* handle a client event */
void thread_poll_event( struct object *obj, int event )
{
    struct thread *thread = (struct thread *)obj;
    assert( obj->ops == &thread_ops );

    if (event & (POLLERR | POLLHUP)) kill_thread( thread, BROKEN_PIPE );
    else
    {
        if (event & POLLOUT) write_request( thread );
        if (event & POLLIN) read_request( thread );
    }
}

/* destroy a thread when its refcount is 0 */
static void destroy_thread( struct object *obj )
{
    struct thread *thread = (struct thread *)obj;
    assert( obj->ops == &thread_ops );

    assert( !thread->debug_ctx );  /* cannot still be debugging something */
    release_object( thread->process );
    if (thread->next) thread->next->prev = thread->prev;
    if (thread->prev) thread->prev->next = thread->next;
    else first_thread = thread->next;
    if (thread->apc) free( thread->apc );
    if (thread->buffer != (void *)-1) munmap( thread->buffer, MAX_REQUEST_LENGTH );
    if (thread->pass_fd != -1) close( thread->pass_fd );
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
struct thread *get_thread_from_handle( int handle, unsigned int access )
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
static int wait_on( int count, struct object *objects[], int flags,
                    int timeout, sleep_reply func )
{
    struct thread_wait *wait;
    struct wait_queue_entry *entry;
    int i;

    if (!(wait = mem_alloc( sizeof(*wait) + (count-1) * sizeof(*entry) ))) return 0;
    current->wait = wait;
    wait->count   = count;
    wait->flags   = flags;
    wait->user    = NULL;
    wait->reply   = func;
    if (flags & SELECT_TIMEOUT)
    {
        gettimeofday( &wait->timeout, 0 );
        add_timeout( &wait->timeout, timeout );
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
static int check_wait( struct thread *thread, struct object **object )
{
    int i, signaled;
    struct thread_wait *wait = thread->wait;
    struct wait_queue_entry *entry = wait->queues;

    assert( wait );
    *object = NULL;
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
            *object = entry->obj;
            if (entry->obj->ops->satisfied( entry->obj, thread ))
                signaled = i + STATUS_ABANDONED_WAIT_0;
            return signaled;
        }
    }

 other_checks:
    if ((wait->flags & SELECT_ALERTABLE) && thread->apc) return STATUS_USER_APC;
    if (wait->flags & SELECT_TIMEOUT)
    {
        struct timeval now;
        gettimeofday( &now, NULL );
        if (!time_before( &now, &wait->timeout )) return STATUS_TIMEOUT;
    }
    return -1;
}

/* build a reply to the select request */
static void build_select_reply( struct thread *thread, struct object *obj, int signaled )
{
    struct select_request *req = get_req_ptr( thread );
    req->signaled = signaled;
}

/* attempt to wake up a thread */
/* return 1 if OK, 0 if the wait condition is still not satisfied */
static int wake_thread( struct thread *thread )
{
    int signaled;
    struct object *object;
    if ((signaled = check_wait( thread, &object )) == -1) return 0;
    thread->error = 0;
    thread->wait->reply( thread, object, signaled );
    end_wait( thread );
    return 1;
}

/* thread wait timeout */
static void thread_timeout( void *ptr )
{
    struct thread *thread = ptr;
    if (debug_level) fprintf( stderr, "%08x: *timeout*\n", (unsigned int)thread );
    assert( thread->wait );
    thread->error = 0;
    thread->wait->user = NULL;
    thread->wait->reply( thread, NULL, STATUS_TIMEOUT );
    end_wait( thread );
    send_reply( thread );
}

/* sleep on a list of objects */
int sleep_on( int count, struct object *objects[], int flags, int timeout, sleep_reply func )
{
    assert( !current->wait );
    if (!wait_on( count, objects, flags, timeout, func )) return 0;
    if (wake_thread( current )) return 1;
    /* now we need to wait */
    if (flags & SELECT_TIMEOUT)
    {
        if (!(current->wait->user = add_timeout_user( &current->wait->timeout,
                                                      thread_timeout, current )))
        {
            end_wait( current );
            return 0;
        }
    }
    return 1;
}

/* select on a list of handles */
static int select_on( int count, int *handles, int flags, int timeout )
{
    int ret = 0;
    int i;
    struct object *objects[MAXIMUM_WAIT_OBJECTS];

    if ((count < 0) || (count > MAXIMUM_WAIT_OBJECTS))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return 0;
    }
    for (i = 0; i < count; i++)
    {
        if (!(objects[i] = get_handle_obj( current->process, handles[i], SYNCHRONIZE, NULL )))
            break;
    }
    if (i == count) ret = sleep_on( count, objects, flags, timeout, build_select_reply );
    while (--i >= 0) release_object( objects[i] );
    return ret;
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
            send_reply( thread );
            if (max && !--max) break;
        }
    }
}

/* queue an async procedure call */
static int thread_queue_apc( struct thread *thread, void *func, void *param )
{
    struct thread_apc *apc;
    if (!thread->apc)
    {
        if (!(thread->apc = mem_alloc( MAX_THREAD_APC * sizeof(*apc) )))
            return 0;
        thread->apc_count = 0;
    }
    else if (thread->apc_count >= MAX_THREAD_APC) return 0;
    thread->apc[thread->apc_count].func  = func;
    thread->apc[thread->apc_count].param = param;
    thread->apc_count++;
    if (thread->wait)
    {
        if (wake_thread( thread )) send_reply( thread );
    }
    return 1;
}

/* retrieve an LDT selector entry */
static void get_selector_entry( struct thread *thread, int entry,
                                unsigned int *base, unsigned int *limit,
                                unsigned char *flags )
{
    if (!thread->process->ldt_copy || !thread->process->ldt_flags)
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    if (entry >= 8192)
    {
        set_error( STATUS_INVALID_PARAMETER );  /* FIXME */
        return;
    }
    suspend_thread( thread, 0 );
    if (thread->attached)
    {
        unsigned char flags_buf[4];
        int *addr = (int *)thread->process->ldt_copy + 2 * entry;
        if (read_thread_int( thread, addr, base ) == -1) goto done;
        if (read_thread_int( thread, addr + 1, limit ) == -1) goto done;
        addr = (int *)thread->process->ldt_flags + (entry >> 2);
        if (read_thread_int( thread, addr, (int *)flags_buf ) == -1) goto done;
        *flags = flags_buf[entry & 3];
    }
    else set_error( STATUS_ACCESS_DENIED );
 done:
    resume_thread( thread );
}

/* kill a thread on the spot */
void kill_thread( struct thread *thread, int exit_code )
{
    if (thread->state == TERMINATED) return;  /* already killed */
    thread->state = TERMINATED;
    thread->exit_code = exit_code;
    if (current == thread) current = NULL;
    if (debug_level) trace_kill( thread );
    if (thread->wait) end_wait( thread );
    generate_debug_event( thread, (thread->process->running_threads == 1) ?
                          EXIT_PROCESS_DEBUG_EVENT : EXIT_THREAD_DEBUG_EVENT );
    debug_exit_thread( thread );
    abandon_mutexes( thread );
    remove_process_thread( thread->process, thread );
    wake_up( &thread->obj, 0 );
    detach_thread( thread );
    remove_select_user( &thread->obj );
    release_object( thread );
}

/* signal that we are finished booting on the client side */
DECL_HANDLER(boot_done)
{
    debug_level = req->debug_level;
    /* Make sure last_req is initialized */
    current->last_req = REQ_BOOT_DONE;
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
        if ((thread = create_thread( sock[0], current->process, req->suspend )))
        {
            req->tid = thread;
            if ((req->handle = alloc_handle( current->process, thread,
                                             THREAD_ALL_ACCESS, req->inherit )) != -1)
            {
                set_reply_fd( current, sock[1] );
                /* thread object will be released when the thread gets killed */
                return;
            }
            release_object( thread );
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
    current->entry    = req->entry;
    if (current->suspend + current->process->suspend > 0) stop_thread( current );
    if (current->process->running_threads > 1)
        generate_debug_event( current, CREATE_THREAD_DEBUG_EVENT );
}

/* terminate a thread */
DECL_HANDLER(terminate_thread)
{
    struct thread *thread;

    if ((thread = get_thread_from_handle( req->handle, THREAD_TERMINATE )))
    {
        kill_thread( thread, req->exit_code );
        release_object( thread );
    }
}

/* fetch information about a thread */
DECL_HANDLER(get_thread_info)
{
    struct thread *thread;

    if ((thread = get_thread_from_handle( req->handle, THREAD_QUERY_INFORMATION )))
    {
        req->tid       = thread;
        req->exit_code = thread->exit_code;
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
    if (!select_on( req->count, req->handles, req->flags, req->timeout ))
        req->signaled = -1;
}

/* queue an APC for a thread */
DECL_HANDLER(queue_apc)
{
    struct thread *thread;
    if ((thread = get_thread_from_handle( req->handle, THREAD_SET_CONTEXT )))
    {
        thread_queue_apc( thread, req->func, req->param );
        release_object( thread );
    }
}

/* get list of APC to call */
DECL_HANDLER(get_apcs)
{
    if ((req->count = current->apc_count))
    {
        memcpy( req->apcs, current->apc, current->apc_count * sizeof(*current->apc) );
        free( current->apc );
        current->apc = NULL;
        current->apc_count = 0;
    }
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
