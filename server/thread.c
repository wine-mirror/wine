/*
 * Server-side thread management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "winerror.h"
#include "server.h"
#include "object.h"


/* request handling */

static void handle_timeout( void *data, int len, int fd,
                            struct thread *self );
static void handle_kill_thread( void *data, int len, int fd,
                                struct thread *self );
static void handle_new_thread( void *data, int len, int fd,
                               struct thread *self );
static void handle_init_thread( void *data, int len, int fd,
                                struct thread *self );

const req_handler req_handlers[REQ_NB_REQUESTS] =
{
    handle_timeout,        /* REQ_TIMEOUT */
    handle_kill_thread,    /* REQ_KILL_THREAD */
    handle_new_thread,     /* REQ_NEW_THREAD */
    handle_init_thread     /* REQ_INIT_THREAD */
};


/* thread structure; not much for now... */

struct thread
{
    struct object   obj;       /* object header */
    struct thread  *next;      /* system-wide thread list */
    struct thread  *prev;
    struct process *process;
    int             client_fd; /* client fd for socket communications */
    int             unix_pid;
    char           *name;
};

static struct thread *first_thread;

/* thread operations */

static void destroy_thread( struct object *obj );

static const struct object_ops thread_ops =
{
    destroy_thread
};


/* create a new thread */
static struct thread *create_thread( int fd, void *pid )
{
    struct thread *thread;
    struct process *process;

    if (pid)
    {
        if (!(process = get_process_from_id( pid ))) return NULL;
    }
    else
    {
        if (!(process = create_process())) return NULL;
    }
    if (!(thread = malloc( sizeof(*thread) )))
    {
        release_object( process );
        return NULL;
    }
    init_object( &thread->obj, &thread_ops, NULL );
    thread->client_fd = fd;
    thread->process   = process;
    thread->unix_pid  = 0;  /* not known yet */
    thread->name      = NULL;

    thread->next = first_thread;
    thread->prev = NULL;
    first_thread = thread;

    if (add_client( fd, thread ) == -1)
    {
        release_object( thread );
        return NULL;
    }
    return thread;
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

struct thread *get_thread_from_id( void *id )
{
    struct thread *t = first_thread;
    while (t && (t != id)) t = t->next;
    return t;
}

/* handle a client timeout (unused for now) */
static void handle_timeout( void *data, int len, int fd, struct thread *self )
{
/*    fprintf( stderr, "Server: got timeout for %s\n", self->name );*/
    send_reply( self->client_fd, 0, -1, 0 );
}

/* a thread has been killed */
static void handle_kill_thread( void *data, int len, int fd,
                                struct thread *self )
{
    if (!self) return;  /* initial client being killed */
/*    fprintf( stderr, "Server: thread '%s' killed\n",
               self->name ? self->name : "???" );
*/
    release_object( &self->obj );
}

/* create a new thread */
static void handle_new_thread( void *data, int len, int fd,
                               struct thread *self )
{
    struct new_thread_request *req = (struct new_thread_request *)data;
    struct new_thread_reply reply;
    struct thread *new_thread;
    int new_fd, err;

    if ((new_fd = dup(fd)) == -1)
    {
        new_thread = NULL;
        err = ERROR_TOO_MANY_OPEN_FILES;
        goto done;
    }
    if (len != sizeof(*req))
    {
        err = ERROR_INVALID_PARAMETER;
        goto done;
    }
    if (!(new_thread = create_thread( new_fd, req->pid )))
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        close( new_fd );
        goto done;
    }
    reply.tid = new_thread;
    reply.pid = new_thread->process;
    err = ERROR_SUCCESS;
 done:
    send_reply( self ? self->client_fd : get_initial_client_fd(),
                err, -1, 1, &reply, sizeof(reply) );
}

/* create a new thread */
static void handle_init_thread( void *data, int len, int fd,
                                struct thread *self )
{
    struct init_thread_request *req = (struct init_thread_request *)data;
    int err;

    if (len < sizeof(*req))
    {
        err = ERROR_INVALID_PARAMETER;
        goto done;
    }
    len -= sizeof(*req);
    self->unix_pid = req->pid;
    if (!(self->name = malloc( len + 1 )))
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }
    memcpy( self->name, (char *)data + sizeof(*req), len );
    self->name[len] = '\0';
    
/*    fprintf( stderr,
             "Server: init thread '%s' pid=%08x tid=%08x unix_pid=%d\n",
             self->name, (int)self->process, (int)self, self->unix_pid );
*/

    err = ERROR_SUCCESS;
 done:
    send_reply( self->client_fd, err, -1, 0 );
}
