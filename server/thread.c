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
#include <sys/uio.h>
#include <unistd.h>
#include <stdarg.h>


#include "winbase.h"
#include "winerror.h"

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

/* allocate the buffer for the communication with the client */
static int alloc_client_buffer( struct thread *thread )
{
    int fd;

    if ((fd = create_anonymous_file()) == -1) return -1;
    if (ftruncate( fd, MAX_REQUEST_LENGTH ) == -1) goto error;
    if ((thread->buffer = mmap( 0, MAX_REQUEST_LENGTH, PROT_READ | PROT_WRITE,
                                MAP_SHARED, fd, 0 )) == (void*)-1) goto error;
    return fd;

 error:
    file_set_error();
    if (fd != -1) close( fd );
    return -1;
}

/* create a new thread */
static struct thread *create_thread( int fd, struct process *process, int suspend )
{
    struct thread *thread;
    int buf_fd;

    int flags = fcntl( fd, F_GETFL, 0 );
    fcntl( fd, F_SETFL, flags | O_NONBLOCK );

    if (!(thread = alloc_object( &thread_ops, fd ))) return NULL;

    thread->unix_pid    = 0;  /* not known yet */
    thread->teb         = NULL;
    thread->mutex       = NULL;
    thread->debug_ctx   = NULL;
    thread->debug_event = NULL;
    thread->exit_event  = NULL;
    thread->wait        = NULL;
    thread->apc         = NULL;
    thread->apc_count   = 0;
    thread->error       = 0;
    thread->pass_fd     = -1;
    thread->state       = RUNNING;
    thread->attached    = 0;
    thread->exit_code   = 0x103;  /* STILL_ACTIVE */
    thread->next        = NULL;
    thread->prev        = NULL;
    thread->priority    = THREAD_PRIORITY_NORMAL;
    thread->affinity    = 1;
    thread->suspend     = (suspend != 0);
    thread->buffer      = (void *)-1;
    thread->last_req    = REQ_GET_THREAD_BUFFER;

    if (!first_thread)  /* creating the first thread */
    {
        current = thread;
        thread->process = process = create_initial_process(); 
        assert( process );
    }
    else thread->process = (struct process *)grab_object( process );

    if ((thread->next = first_thread) != NULL) thread->next->prev = thread;
    first_thread = thread;
    add_process_thread( process, thread );

    if ((buf_fd = alloc_client_buffer( thread )) == -1) goto error;

    set_select_events( &thread->obj, POLLIN );  /* start listening to events */
    set_reply_fd( thread, buf_fd );  /* send the fd to the client */
    send_reply( thread );
    return thread;

 error:
    remove_process_thread( process, thread );
    release_object( thread );
    return NULL;
}

/* create the initial thread and start the main server loop */
void create_initial_thread( int fd )
{
    create_thread( fd, NULL, 0 );
    select_loop();
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
        if (req->affinity != 1) set_error( ERROR_INVALID_PARAMETER );
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
    else set_error( ERROR_SIGNAL_REFUSED );
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
static int wait_on( struct thread *thread, int count,
                    int *handles, int flags, int timeout )
{
    struct thread_wait *wait;
    struct wait_queue_entry *entry;
    struct object *obj;
    int i;

    if ((count < 0) || (count > MAXIMUM_WAIT_OBJECTS))
    {
        set_error( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!(wait = mem_alloc( sizeof(*wait) + (count-1) * sizeof(*entry) ))) return 0;
    thread->wait  = wait;
    wait->count   = count;
    wait->flags   = flags;
    wait->user    = NULL;
    if (flags & SELECT_TIMEOUT)
    {
        gettimeofday( &wait->timeout, 0 );
        add_timeout( &wait->timeout, timeout );
    }

    for (i = 0, entry = wait->queues; i < count; i++, entry++)
    {
        if (!(obj = get_handle_obj( thread->process, handles[i],
                                    SYNCHRONIZE, NULL )))
        {
            wait->count = i - 1;
            end_wait( thread );
            return 0;
        }
        entry->thread = thread;
        if (!obj->ops->add_queue( obj, entry ))
        {
            wait->count = i - 1;
            end_wait( thread );
            return 0;
        }
        release_object( obj );
    }
    return 1;
}

/* check if the thread waiting condition is satisfied */
static int check_wait( struct thread *thread, int *signaled )
{
    int i;
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
        *signaled = 0;
        for (i = 0, entry = wait->queues; i < wait->count; i++, entry++)
            if (entry->obj->ops->satisfied( entry->obj, thread ))
                *signaled = STATUS_ABANDONED_WAIT_0;
        return 1;
    }
    else
    {
        for (i = 0, entry = wait->queues; i < wait->count; i++, entry++)
        {
            if (!entry->obj->ops->signaled( entry->obj, thread )) continue;
            /* Wait satisfied: tell it to the object */
            *signaled = i;
            if (entry->obj->ops->satisfied( entry->obj, thread ))
                *signaled = i + STATUS_ABANDONED_WAIT_0;
            return 1;
        }
    }

 other_checks:
    if ((wait->flags & SELECT_ALERTABLE) && thread->apc)
    {
        *signaled = STATUS_USER_APC;
        return 1;
    }
    if (wait->flags & SELECT_TIMEOUT)
    {
        struct timeval now;
        gettimeofday( &now, NULL );
        if (!time_before( &now, &wait->timeout ))
        {
            *signaled = STATUS_TIMEOUT;
            return 1;
        }
    }
    return 0;
}

/* attempt to wake up a thread */
/* return 1 if OK, 0 if the wait condition is still not satisfied */
static int wake_thread( struct thread *thread )
{
    struct select_request *req = get_req_ptr( thread );

    if (!check_wait( thread, &req->signaled )) return 0;
    end_wait( thread );
    return 1;
}

/* sleep on a list of objects */
static void sleep_on( struct thread *thread, int count, int *handles, int flags, int timeout )
{
    struct select_request *req;
    assert( !thread->wait );
    if (!wait_on( thread, count, handles, flags, timeout )) goto error;
    if (wake_thread( thread )) return;
    /* now we need to wait */
    if (flags & SELECT_TIMEOUT)
    {
        if (!(thread->wait->user = add_timeout_user( &thread->wait->timeout,
                                                     call_timeout_handler, thread )))
            goto error;
    }
    thread->state = SLEEPING;
    return;

 error:
    req = get_req_ptr( thread );
    req->signaled = -1;
}

/* timeout for the current thread */
void thread_timeout(void)
{
    struct select_request *req = get_req_ptr( current );

    assert( current->wait );
    current->wait->user = NULL;
    end_wait( current );
    req->signaled = STATUS_TIMEOUT;
    send_reply( current );
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

/* kill a thread on the spot */
void kill_thread( struct thread *thread, int exit_code )
{
    if (thread->state == TERMINATED) return;  /* already killed */
    thread->state = TERMINATED;
    thread->exit_code = exit_code;
    if (current == thread) current = NULL;
    if (debug_level) trace_kill( thread );
    if (thread->wait) end_wait( thread );
    debug_exit_thread( thread, exit_code );
    abandon_mutexes( thread );
    remove_process_thread( thread->process, thread );
    wake_up( &thread->obj, 0 );
    detach_thread( thread );
    remove_select_user( &thread->obj );
    release_object( thread );
}

/* create a new thread */
DECL_HANDLER(new_thread)
{
    struct thread *thread;
    struct process *process;

    if ((process = get_process_from_id( req->pid )))
    {
        if ((fd = dup(fd)) != -1)
        {
            if ((thread = create_thread( fd, process, req->suspend )))
            {
                req->tid = thread;
                if ((req->handle = alloc_handle( current->process, thread,
                                                 THREAD_ALL_ACCESS, req->inherit )) == -1)
                    release_object( thread );
                /* else will be released when the thread gets killed */
            }
            else close( fd );
        }
        else file_set_error();
        release_object( process );
    }
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
    req->pid = current->process;
    req->tid = current;
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
    sleep_on( current, req->count, req->handles, req->flags, req->timeout );
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
