/*
 * Server-side process management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "winerror.h"
#include "winbase.h"
#include "winnt.h"

#include "server.h"
#include "handle.h"
#include "process.h"
#include "thread.h"

/* process structure */

static struct process initial_process;
static struct process *first_process = &initial_process;
static int running_processes;

/* process operations */

static void process_dump( struct object *obj, int verbose );
static int process_signaled( struct object *obj, struct thread *thread );
static void process_destroy( struct object *obj );

static const struct object_ops process_ops =
{
    process_dump,
    add_queue,
    remove_queue,
    process_signaled,
    no_satisfied,
    no_read_fd,
    no_write_fd,
    no_flush,
    no_get_file_info,
    process_destroy
};


/* initialization of a process structure */
static void init_process( struct process *process )
{
    init_object( &process->obj, &process_ops, NULL );
    process->next            = NULL;
    process->prev            = NULL;
    process->thread_list     = NULL;
    process->debug_next      = NULL;
    process->debug_prev      = NULL;
    process->debugger        = NULL;
    process->exit_code       = 0x103;  /* STILL_ACTIVE */
    process->running_threads = 0;
    process->priority        = NORMAL_PRIORITY_CLASS;
    process->affinity        = 1;
    process->suspend         = 0;
    process->console_in      = NULL;
    process->console_out     = NULL;
    process->info            = NULL;
    gettimeofday( &process->start_time, NULL );
    /* alloc a handle for the process itself */
    alloc_handle( process, process, PROCESS_ALL_ACCESS, 0 );
}

/* create the initial process */
struct process *create_initial_process(void)
{
    struct new_process_request *info;

    copy_handle_table( &initial_process, NULL );
    init_process( &initial_process );

    if (!alloc_console( &initial_process )) return NULL;
    if (!(info = mem_alloc( sizeof(*info) ))) return NULL;
    info->start_flags = STARTF_USESTDHANDLES;
    info->hstdin  = alloc_handle( &initial_process, initial_process.console_in,
                                  GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, 1 );
    info->hstdout = alloc_handle( &initial_process, initial_process.console_out,
                                  GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, 1 );
    info->hstderr = alloc_handle( &initial_process, initial_process.console_out,
                                  GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, 1 );
    info->env_ptr = NULL;
    initial_process.info = info;
    grab_object( &initial_process ); /* so that we never free it */
    return &initial_process;

}

/* create a new process */
static struct process *create_process( struct new_process_request *req )
{
    struct process *process = NULL;
    struct process *parent = current->process;

    if (!(process = mem_alloc( sizeof(*process) ))) return NULL;
    if (!copy_handle_table( process, req->inherit_all ? parent : NULL ))
    {
        free( process );
        return NULL;
    }
    init_process( process );

    if (!(process->info = mem_alloc( sizeof(*process->info) ))) goto error;
    memcpy( process->info, req, sizeof(*req) );

    /* set the process console */
    if (req->create_flags & CREATE_NEW_CONSOLE)
    {
        if (!alloc_console( process )) goto error;
    }
    else if (!(req->create_flags & DETACHED_PROCESS))
    {
        if (parent->console_in) process->console_in = grab_object( parent->console_in );
        if (parent->console_out) process->console_out = grab_object( parent->console_out );
    }

    if (!req->inherit_all && !(req->start_flags & STARTF_USESTDHANDLES))
    {
        process->info->hstdin  = duplicate_handle( parent, req->hstdin, process,
                                                   0, TRUE, DUPLICATE_SAME_ACCESS );
        process->info->hstdout = duplicate_handle( parent, req->hstdout, process,
                                                   0, TRUE, DUPLICATE_SAME_ACCESS );
        process->info->hstderr = duplicate_handle( parent, req->hstderr, process,
                                                   0, TRUE, DUPLICATE_SAME_ACCESS );
    }

    /* attach to the debugger if requested */
    if (req->create_flags & DEBUG_PROCESS)
        debugger_attach( process, current );
    else if (parent->debugger && !(req->create_flags & DEBUG_ONLY_THIS_PROCESS))
        debugger_attach( process, parent->debugger );

    process->next = first_process;
    first_process->prev = process;
    first_process = process;
    return process;

 error:
    release_object( process );
    return NULL;
}

/* destroy a process when its refcount is 0 */
static void process_destroy( struct object *obj )
{
    struct process *process = (struct process *)obj;
    assert( obj->ops == &process_ops );
    assert( process != &initial_process );

    /* we can't have a thread remaining */
    assert( !process->thread_list );
    if (process->next) process->next->prev = process->prev;
    if (process->prev) process->prev->next = process->next;
    else first_process = process->next;
    free_console( process );
    free_handles( process );
    if (process->info) free( process->info );
    if (debug_level) memset( process, 0xbb, sizeof(process) );  /* catch errors */
    free( process );
}

/* dump a process on stdout for debugging purposes */
static void process_dump( struct object *obj, int verbose )
{
    struct process *process = (struct process *)obj;
    assert( obj->ops == &process_ops );

    printf( "Process next=%p prev=%p\n", process->next, process->prev );
}

static int process_signaled( struct object *obj, struct thread *thread )
{
    struct process *process = (struct process *)obj;
    return !process->running_threads;
}


/* get a process from an id (and increment the refcount) */
struct process *get_process_from_id( void *id )
{
    struct process *p = first_process;
    while (p && (p != id)) p = p->next;
    if (p) grab_object( p );
    else SET_ERROR( ERROR_INVALID_PARAMETER );
    return p;
}

/* get a process from a handle (and increment the refcount) */
struct process *get_process_from_handle( int handle, unsigned int access )
{
    return (struct process *)get_handle_obj( current->process, handle,
                                             access, &process_ops );
}

/* retrieve the initialization info for a new process */
static int get_process_init_info( struct process *process, struct init_process_reply *reply )
{
    struct new_process_request *info;
    if (!(info = process->info)) return 0;
    process->info = NULL;
    reply->start_flags = info->start_flags;
    reply->hstdin      = info->hstdin;
    reply->hstdout     = info->hstdout;
    reply->hstderr     = info->hstderr;
    reply->env_ptr     = info->env_ptr;
    free( info );
    return 1;
}

/* a process has been killed (i.e. its last thread died) */
static void process_killed( struct process *process, int exit_code )
{
    assert( !process->thread_list );
    process->exit_code = exit_code;
    gettimeofday( &process->end_time, NULL );
    wake_up( &process->obj, 0 );
    free_handles( process );
}

/* add a thread to a process running threads list */
void add_process_thread( struct process *process, struct thread *thread )
{
    thread->proc_next = process->thread_list;
    thread->proc_prev = NULL;
    if (thread->proc_next) thread->proc_next->proc_prev = thread;
    process->thread_list = thread;
    if (!process->running_threads++) running_processes++;
    grab_object( thread );
}

/* remove a thread from a process running threads list */
void remove_process_thread( struct process *process, struct thread *thread )
{
    assert( process->running_threads > 0 );
    assert( process->thread_list );

    if (thread->proc_next) thread->proc_next->proc_prev = thread->proc_prev;
    if (thread->proc_prev) thread->proc_prev->proc_next = thread->proc_next;
    else process->thread_list = thread->proc_next;

    if (!--process->running_threads)
    {
        /* we have removed the last running thread, exit the process */
        running_processes--;
        process_killed( process, thread->exit_code );
    }
    release_object( thread );
}

/* suspend all the threads of a process */
void suspend_process( struct process *process )
{
    if (!process->suspend++)
    {
        struct thread *thread = process->thread_list;
        for (; thread; thread = thread->proc_next)
        {
            if (!thread->suspend && thread->unix_pid)
                kill( thread->unix_pid, SIGSTOP );
        }
    }
}

/* resume all the threads of a process */
void resume_process( struct process *process )
{
    assert (process->suspend > 0);
    if (!--process->suspend)
    {
        struct thread *thread = process->thread_list;
        for (; thread; thread = thread->proc_next)
        {
            if (!thread->suspend && thread->unix_pid)
                kill( thread->unix_pid, SIGCONT );
        }
    }
}

/* kill a process on the spot */
void kill_process( struct process *process, int exit_code )
{
    while (process->thread_list)
        kill_thread( process->thread_list, exit_code );
}

/* get all information about a process */
static void get_process_info( struct process *process,
                              struct get_process_info_reply *reply )
{
    reply->pid              = process;
    reply->exit_code        = process->exit_code;
    reply->priority         = process->priority;
    reply->process_affinity = process->affinity;
    reply->system_affinity  = 1;
}

/* set all information about a process */
static void set_process_info( struct process *process,
                              struct set_process_info_request *req )
{
    if (req->mask & SET_PROCESS_INFO_PRIORITY)
        process->priority = req->priority;
    if (req->mask & SET_PROCESS_INFO_AFFINITY)
    {
        if (req->affinity != 1) SET_ERROR( ERROR_INVALID_PARAMETER );
        else process->affinity = req->affinity;
    }
}

/* allocate a console for this process */
int alloc_console( struct process *process )
{
    struct object *obj[2];
    if (process->console_in || process->console_out)
    {
        SET_ERROR( ERROR_ACCESS_DENIED );
        return 0;
    }
    if (!create_console( -1, obj )) return 0;
    process->console_in  = obj[0];
    process->console_out = obj[1];
    return 1;
}

/* free the console for this process */
int free_console( struct process *process )
{
    if (process->console_in) release_object( process->console_in );
    if (process->console_out) release_object( process->console_out );
    process->console_in = process->console_out = NULL;
    return 1;
}

/* get the process console */
struct object *get_console( struct process *process, int output )
{
    struct object *obj;
    if (!(obj = output ? process->console_out : process->console_in))
        return NULL;
    return grab_object( obj );
}

/* take a snapshot of currently running processes */
struct process_snapshot *process_snap( int *count )
{
    struct process_snapshot *snapshot, *ptr;
    struct process *process;
    if (!running_processes) return NULL;
    if (!(snapshot = mem_alloc( sizeof(*snapshot) * running_processes )))
        return NULL;
    ptr = snapshot;
    for (process = first_process; process; process = process->next)
    {
        if (!process->running_threads) continue;
        ptr->process  = process;
        ptr->threads  = process->running_threads;
        ptr->priority = process->priority;
        grab_object( process );
        ptr++;
    }
    *count = running_processes;
    return snapshot;
}

/* create a new process */
DECL_HANDLER(new_process)
{
    struct new_process_reply reply;
    struct process *process;

    if ((process = create_process( req )))
    {
        reply.pid    = process;
        reply.handle = alloc_handle( current->process, process,
                                     PROCESS_ALL_ACCESS, req->inherit );
        release_object( process );
    }
    else
    {
        reply.handle = -1;
        reply.pid    = NULL;
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* initialize a new process */
DECL_HANDLER(init_process)
{
    struct init_process_reply reply;
    if (current->state != RUNNING)
    {
        fatal_protocol_error( "init_process: init_thread not called yet\n" );
        return;
    }
    if (!get_process_init_info( current->process, &reply ))
    {
        fatal_protocol_error( "init_process: called twice\n" );
        return;
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* open a handle to a process */
DECL_HANDLER(open_process)
{
    struct open_process_reply reply = { -1 };
    struct process *process = get_process_from_id( req->pid );
    if (process)
    {
        reply.handle = alloc_handle( current->process, process,
                                     req->access, req->inherit );
        release_object( process );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* terminate a process */
DECL_HANDLER(terminate_process)
{
    struct process *process;

    if ((process = get_process_from_handle( req->handle, PROCESS_TERMINATE )))
    {
        kill_process( process, req->exit_code );
        release_object( process );
    }
    if (current) send_reply( current, -1, 0 );
}

/* fetch information about a process */
DECL_HANDLER(get_process_info)
{
    struct process *process;
    struct get_process_info_reply reply = { 0, 0, 0 };

    if ((process = get_process_from_handle( req->handle, PROCESS_QUERY_INFORMATION )))
    {
        get_process_info( process, &reply );
        release_object( process );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* set information about a process */
DECL_HANDLER(set_process_info)
{
    struct process *process;

    if ((process = get_process_from_handle( req->handle, PROCESS_SET_INFORMATION )))
    {
        set_process_info( process, req );
        release_object( process );
    }
    send_reply( current, -1, 0 );
}
