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

#include "winnt.h"
#include "winerror.h"
#include "server.h"
#include "server/thread.h"


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

static struct thread *first_thread;


/* create a new thread */
struct thread *create_thread( int fd, void *pid, int *thread_handle,
                              int *process_handle )
{
    struct thread *thread;
    struct process *process;

    if (!(thread = mem_alloc( sizeof(*thread) ))) return NULL;

    if (pid) process = get_process_from_id( pid );
    else process = create_process();
    if (!process)
    {
        free( thread );
        return NULL;
    }

    init_object( &thread->obj, &thread_ops, NULL );
    thread->client_fd = fd;
    thread->process   = process;
    thread->unix_pid  = 0;  /* not known yet */
    thread->name      = NULL;
    thread->mutex     = NULL;
    thread->wait      = NULL;
    thread->apc       = NULL;
    thread->apc_count = 0;
    thread->error     = 0;
    thread->state     = STARTING;
    thread->exit_code = 0x103;  /* STILL_ACTIVE */
    thread->next      = first_thread;
    thread->prev      = NULL;
    thread->priority  = THREAD_PRIORITY_NORMAL;
    thread->affinity  = 1;
    thread->suspend   = 0;

    if (first_thread) first_thread->prev = thread;
    first_thread = thread;
    add_process_thread( process, thread );

    *thread_handle = *process_handle = -1;
    if (current)
    {
        if ((*thread_handle = alloc_handle( current->process, thread,
                                            THREAD_ALL_ACCESS, 0 )) == -1)
            goto error;
    }
    if (current && !pid)
    {
        if ((*process_handle = alloc_handle( current->process, process,
                                             PROCESS_ALL_ACCESS, 0 )) == -1)
            goto error;
    }

    if (add_client( fd, thread ) == -1) goto error;

    return thread;

 error:
    if (current)
    {
        close_handle( current->process, *thread_handle );
        close_handle( current->process, *process_handle );
    }
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
    if (thread->name) free( thread->name );
    if (thread->apc) free( thread->apc );
    if (debug_level) memset( thread, 0xaa, sizeof(thread) );  /* catch errors */
    free( thread );
}

/* dump a thread on stdout for debugging purposes */
static void dump_thread( struct object *obj, int verbose )
{
    struct thread *thread = (struct thread *)obj;
    assert( obj->ops == &thread_ops );

    fprintf( stderr, "Thread pid=%d fd=%d name='%s'\n",
             thread->unix_pid, thread->client_fd, thread->name );
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

/* get all information about a thread */
void get_thread_info( struct thread *thread,
                      struct get_thread_info_reply *reply )
{
    reply->pid       = thread;
    reply->exit_code = thread->exit_code;
    reply->priority  = thread->priority;
}


/* set all information about a thread */
void set_thread_info( struct thread *thread,
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
int suspend_thread( struct thread *thread )
{
    int old_count = thread->suspend;
    if (thread->suspend < MAXIMUM_SUSPEND_COUNT)
    {
        if (!thread->suspend++)
        {
            if (thread->unix_pid) kill( thread->unix_pid, SIGSTOP );
        }
    }
    return old_count;
}

/* resume a thread */
int resume_thread( struct thread *thread )
{
    int old_count = thread->suspend;
    if (thread->suspend > 0)
    {
        if (!--thread->suspend)
        {
            if (thread->unix_pid) kill( thread->unix_pid, SIGCONT );
        }
    }
    return old_count;
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
    return send_reply_v( thread->client_fd, thread->error, pass_fd, vec, n );
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
    if (wait->flags & SELECT_TIMEOUT) set_select_timeout( thread->client_fd, NULL );
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
    if (flags & SELECT_TIMEOUT)
    {
        gettimeofday( &wait->timeout, 0 );
        if (timeout)
        {
            wait->timeout.tv_usec += (timeout % 1000) * 1000;
            if (wait->timeout.tv_usec >= 1000000)
            {
                wait->timeout.tv_usec -= 1000000;
                wait->timeout.tv_sec++;
            }
            wait->timeout.tv_sec += timeout / 1000;
        }
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
        for (i = 0, entry = wait->queues; i < wait->count; i++, entry++)
            if (!entry->obj->ops->signaled( entry->obj, thread )) goto other_checks;
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
void sleep_on( struct thread *thread, int count, int *handles, int flags, int timeout )
{
    assert( !thread->wait );
    if (!wait_on( thread, count, handles, flags, timeout ))
    {
        /* return an error */
        send_select_reply( thread, -1 );
        return;
    }
    if (!wake_thread( thread ))
    {
        /* we need to wait */
        if (flags & SELECT_TIMEOUT)
            set_select_timeout( thread->client_fd, &thread->wait->timeout );
    }
}

/* timeout for the current thread */
void thread_timeout(void)
{
    assert( current->wait );
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
int thread_queue_apc( struct thread *thread, void *func, void *param )
{
    struct thread_apc *apc;
    if (!func)
    {
        SET_ERROR( ERROR_INVALID_PARAMETER );
        return 0;
    }
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
    wake_thread( thread );
    return 1;
}

/* kill a thread on the spot */
void kill_thread( struct thread *thread, int exit_code )
{
    if (thread->state == TERMINATED) return;  /* already killed */
    if (thread->unix_pid) kill( thread->unix_pid, SIGTERM );
    remove_client( thread->client_fd, exit_code ); /* this will call thread_killed */
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
