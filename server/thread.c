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
#include <sys/uio.h>
#include <unistd.h>

#include "winnt.h"
#include "winerror.h"
#include "server.h"
#include "server/thread.h"


/* thread operations */

static void dump_thread( struct object *obj, int verbose );
static void destroy_thread( struct object *obj );

static const struct object_ops thread_ops =
{
    dump_thread,
    destroy_thread
};

static struct thread *first_thread;


/* create a new thread */
struct thread *create_thread( int fd, void *pid, int *thread_handle,
                              int *process_handle )
{
    struct thread *thread;
    struct process *process;

    if (!(thread = malloc( sizeof(*thread) ))) return NULL;

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
    thread->error     = 0;
    thread->exit_code = 0x103;  /* STILL_ACTIVE */
    thread->next      = first_thread;
    thread->prev      = NULL;

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
    free( thread );
}

/* dump a thread on stdout for debugging purposes */
static void dump_thread( struct object *obj, int verbose )
{
    struct thread *thread = (struct thread *)obj;
    assert( obj->ops == &thread_ops );

    printf( "Thread pid=%d fd=%d name='%s'\n",
            thread->unix_pid, thread->client_fd, thread->name );
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

/* kill a thread on the spot */
void kill_thread( struct thread *thread, int exit_code )
{
    if (thread->unix_pid) kill( thread->unix_pid, SIGTERM );
    remove_client( thread->client_fd, exit_code ); /* this will call thread_killed */
}

/* a thread has been killed */
void thread_killed( struct thread *thread, int exit_code )
{
    thread->exit_code = exit_code;
    remove_process_thread( thread->process, thread );
    release_object( thread );
}
