/*
 * Server-side thread management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdarg.h>


#include "winbase.h"
#include "winerror.h"

#include "handle.h"
#include "server.h"
#include "process.h"
#include "thread.h"


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
static void destroy_thread( struct object *obj );

static const struct object_ops thread_ops =
{
    dump_thread,
    add_queue,
    remove_queue,
    thread_signaled,
    no_satisfied,
    no_read_fd,
    no_write_fd,
    no_flush,
    no_get_file_info,
    destroy_thread
};

static struct thread initial_thread;
static struct thread *first_thread = &initial_thread;

/* initialization of a thread structure */
static void init_thread( struct thread *thread, int fd )
{
    init_object( &thread->obj, &thread_ops, NULL );
    thread->client      = NULL;
    thread->unix_pid    = 0;  /* not known yet */
    thread->teb         = NULL;
    thread->mutex       = NULL;
    thread->debug_ctx   = NULL;
    thread->debug_first = NULL;
    thread->wait        = NULL;
    thread->apc         = NULL;
    thread->apc_count   = 0;
    thread->error       = 0;
    thread->state       = STARTING;
    thread->exit_code   = 0x103;  /* STILL_ACTIVE */
    thread->next        = NULL;
    thread->prev        = NULL;
    thread->priority    = THREAD_PRIORITY_NORMAL;
    thread->affinity    = 1;
    thread->suspend     = 0;
}

/* create the initial thread and start the main server loop */
void create_initial_thread( int fd )
{
    current = &initial_thread;
    init_thread( &initial_thread, fd );
    initial_thread.process = create_initial_process(); 
    add_process_thread( initial_thread.process, &initial_thread );
    initial_thread.client = add_client( fd, &initial_thread );
    grab_object( &initial_thread ); /* so that we never free it */
    select_loop();
}

/* create a new thread */
static struct thread *create_thread( int fd, void *pid, int suspend, int inherit, int *handle )
{
    struct thread *thread;
    struct process *process;

    if (!(thread = mem_alloc( sizeof(*thread) ))) return NULL;

    if (!(process = get_process_from_id( pid )))
    {
        free( thread );
        return NULL;
    }
    init_thread( thread, fd );
    thread->process = process;

    if (suspend) thread->suspend++;

    thread->next = first_thread;
    first_thread->prev = thread;
    first_thread = thread;
    add_process_thread( process, thread );

    if ((*handle = alloc_handle( current->process, thread,
                                 THREAD_ALL_ACCESS, inherit )) == -1) goto error;
    if (!(thread->client = add_client( fd, thread )))
    {
        SET_ERROR( ERROR_TOO_MANY_OPEN_FILES );
        goto error;
    }
    return thread;

 error:
    if (current) close_handle( current->process, *handle );
    remove_process_thread( process, thread );
    release_object( thread );
    return NULL;
}

/* destroy a thread when its refcount is 0 */
static void destroy_thread( struct object *obj )
{
    struct thread *thread = (struct thread *)obj;
    assert( obj->ops == &thread_ops );

    release_object( thread->process );
    if (thread->next) thread->next->prev = thread->prev;
    if (thread->prev) thread->prev->next = thread->next;
    else first_thread = thread->next;
    if (thread->apc) free( thread->apc );
    if (debug_level) memset( thread, 0xaa, sizeof(thread) );  /* catch errors */
    free( thread );
}

/* dump a thread on stdout for debugging purposes */
static void dump_thread( struct object *obj, int verbose )
{
    struct thread *thread = (struct thread *)obj;
    assert( obj->ops == &thread_ops );

    fprintf( stderr, "Thread pid=%d teb=%p client=%p\n",
             thread->unix_pid, thread->teb, thread->client );
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

/* set all information about a thread */
static void set_thread_info( struct thread *thread,
                             struct set_thread_info_request *req )
{
    if (req->mask & SET_THREAD_INFO_PRIORITY)
        thread->priority = req->priority;
    if (req->mask & SET_THREAD_INFO_AFFINITY)
    {
        if (req->affinity != 1) SET_ERROR( ERROR_INVALID_PARAMETER );
        else thread->affinity = req->affinity;
    }
}

/* suspend a thread */
static int suspend_thread( struct thread *thread )
{
    int old_count = thread->suspend;
    if (thread->suspend < MAXIMUM_SUSPEND_COUNT)
    {
        if (!(thread->process->suspend + thread->suspend++))
        {
            if (thread->unix_pid) kill( thread->unix_pid, SIGSTOP );
        }
    }
    return old_count;
}

/* resume a thread */
static int resume_thread( struct thread *thread )
{
    int old_count = thread->suspend;
    if (thread->suspend > 0)
    {
        if (!(--thread->suspend + thread->process->suspend))
        {
            if (thread->unix_pid) kill( thread->unix_pid, SIGCONT );
        }
    }
    return old_count;
}

/* suspend all threads but the current */
void suspend_all_threads( void )
{
    struct thread *thread;
    for ( thread = first_thread; thread; thread = thread->next )
        if ( thread != current )
            suspend_thread( thread );
}

/* resume all threads but the current */
void resume_all_threads( void )
{
    struct thread *thread;
    for ( thread = first_thread; thread; thread = thread->next )
        if ( thread != current )
            resume_thread( thread );
}

/* send a reply to a thread */
int send_reply( struct thread *thread, int pass_fd, int n,
                ... /* arg_1, len_1, ..., arg_n, len_n */ )
{
    struct iovec vec[16];
    va_list args;
    int i;

    assert( n < 16 );
    va_start( args, n );
    for (i = 0; i < n; i++)
    {
        vec[i].iov_base = va_arg( args, void * );
        vec[i].iov_len  = va_arg( args, int );
    }
    va_end( args );
    return send_reply_v( thread->client, thread->error, pass_fd, vec, n );
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
        SET_ERROR( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!(wait = mem_alloc( sizeof(*wait) + (count-1) * sizeof(*entry) ))) return 0;
    thread->wait  = wait;
    wait->count   = count;
    wait->flags   = flags;
    wait->user    = NULL;
    if (flags & SELECT_TIMEOUT) make_timeout( &wait->timeout, timeout );

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
                *signaled += STATUS_ABANDONED_WAIT_0;
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
        if ((now.tv_sec > wait->timeout.tv_sec) ||
            ((now.tv_sec == wait->timeout.tv_sec) &&
             (now.tv_usec >= wait->timeout.tv_usec)))
        {
            *signaled = STATUS_TIMEOUT;
            return 1;
        }
    }
    return 0;
}

/* send the select reply to wake up the client */
static void send_select_reply( struct thread *thread, int signaled )
{
    struct select_reply reply;
    reply.signaled = signaled;
    if ((signaled == STATUS_USER_APC) && thread->apc)
    {
        struct thread_apc *apc = thread->apc;
        int len = thread->apc_count * sizeof(*apc);
        thread->apc = NULL;
        thread->apc_count = 0;
        send_reply( thread, -1, 2, &reply, sizeof(reply),
                    apc, len );
        free( apc );
    }
    else send_reply( thread, -1, 1, &reply, sizeof(reply) );
}

/* attempt to wake up a thread */
/* return 1 if OK, 0 if the wait condition is still not satisfied */
static int wake_thread( struct thread *thread )
{
    int signaled;

    if (!check_wait( thread, &signaled )) return 0;
    end_wait( thread );
    send_select_reply( thread, signaled );
    return 1;
}

/* sleep on a list of objects */
static void sleep_on( struct thread *thread, int count, int *handles, int flags, int timeout )
{
    assert( !thread->wait );
    if (!wait_on( thread, count, handles, flags, timeout ))
    {
        /* return an error */
        send_select_reply( thread, -1 );
        return;
    }
    if (wake_thread( thread )) return;
    /* now we need to wait */
    if (flags & SELECT_TIMEOUT)
    {
        if (!(thread->wait->user = add_timeout_user( &thread->wait->timeout,
                                                     call_timeout_handler, thread )))
        {
            send_select_reply( thread, -1 );
        }
    }
}

/* timeout for the current thread */
void thread_timeout(void)
{
    assert( current->wait );
    current->wait->user = NULL;
    end_wait( current );
    send_select_reply( current, STATUS_TIMEOUT );
}

/* attempt to wake threads sleeping on the object wait queue */
void wake_up( struct object *obj, int max )
{
    struct wait_queue_entry *entry = obj->head;

    while (entry)
    {
        struct wait_queue_entry *next = entry->next;
        if (wake_thread( entry->thread ))
        {
            if (max && !--max) break;
        }
        entry = next;
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
    if (thread->wait) wake_thread( thread );
    return 1;
}

/* kill a thread on the spot */
void kill_thread( struct thread *thread, int exit_code )
{
    if (thread->state == TERMINATED) return;  /* already killed */
    if (thread->unix_pid) kill( thread->unix_pid, SIGTERM );
    remove_client( thread->client, exit_code ); /* this will call thread_killed */
}

/* a thread has been killed */
void thread_killed( struct thread *thread, int exit_code )
{
    thread->state = TERMINATED;
    thread->exit_code = exit_code;
    if (thread->wait) end_wait( thread );
    abandon_mutexes( thread );
    remove_process_thread( thread->process, thread );
    wake_up( &thread->obj, 0 );
    release_object( thread );
}

/* create a new thread */
DECL_HANDLER(new_thread)
{
    struct new_thread_reply reply;
    int new_fd;

    if ((new_fd = dup(fd)) != -1)
    {
        reply.tid = create_thread( new_fd, req->pid, req->suspend,
                                   req->inherit, &reply.handle );
        if (!reply.tid) close( new_fd );
    }
    else
        SET_ERROR( ERROR_TOO_MANY_OPEN_FILES );

    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* initialize a new thread */
DECL_HANDLER(init_thread)
{
    struct init_thread_reply reply;

    if (current->state != STARTING)
    {
        fatal_protocol_error( "init_thread: already running\n" );
        return;
    }
    current->state    = RUNNING;
    current->unix_pid = req->unix_pid;
    current->teb      = req->teb;
    if (current->suspend + current->process->suspend > 0)
        kill( current->unix_pid, SIGSTOP );
    reply.pid = current->process;
    reply.tid = current;
    send_reply( current, -1, 1, &reply, sizeof(reply) );
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
    if (current) send_reply( current, -1, 0 );
}

/* fetch information about a thread */
DECL_HANDLER(get_thread_info)
{
    struct thread *thread;
    struct get_thread_info_reply reply = { 0, 0, 0 };

    if ((thread = get_thread_from_handle( req->handle, THREAD_QUERY_INFORMATION )))
    {
        reply.tid       = thread;
        reply.exit_code = thread->exit_code;
        reply.priority  = thread->priority;
        release_object( thread );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
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
    send_reply( current, -1, 0 );
}

/* suspend a thread */
DECL_HANDLER(suspend_thread)
{
    struct thread *thread;
    struct suspend_thread_reply reply = { -1 };
    if ((thread = get_thread_from_handle( req->handle, THREAD_SUSPEND_RESUME )))
    {
        reply.count = suspend_thread( thread );
        release_object( thread );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
    
}

/* resume a thread */
DECL_HANDLER(resume_thread)
{
    struct thread *thread;
    struct resume_thread_reply reply = { -1 };
    if ((thread = get_thread_from_handle( req->handle, THREAD_SUSPEND_RESUME )))
    {
        reply.count = resume_thread( thread );
        release_object( thread );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
    
}

/* select on a handle list */
DECL_HANDLER(select)
{
    if (len != req->count * sizeof(int))
        fatal_protocol_error( "select: bad length %d for %d handles\n",
                              len, req->count );
    sleep_on( current, req->count, (int *)data, req->flags, req->timeout );
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
    send_reply( current, -1, 0 );
}
