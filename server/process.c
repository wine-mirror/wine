/*
 * Server-side process management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <errno.h>
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

#include "handle.h"
#include "process.h"
#include "thread.h"
#include "request.h"

/* process structure */

static struct process *first_process;
static int running_processes;

/* process operations */

static void process_dump( struct object *obj, int verbose );
static int process_signaled( struct object *obj, struct thread *thread );
static void process_destroy( struct object *obj );

static const struct object_ops process_ops =
{
    sizeof(struct process),
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


/* create a new process */
static struct process *create_process( struct process *parent, struct new_process_request *req,
                                       const char *cmd_line, size_t len )
{
    struct process *process;

    if (!(process = alloc_object( &process_ops ))) return NULL;
    process->next            = NULL;
    process->prev            = NULL;
    process->thread_list     = NULL;
    process->debugger        = NULL;
    process->handles         = NULL;
    process->exit_code       = 0x103;  /* STILL_ACTIVE */
    process->running_threads = 0;
    process->priority        = NORMAL_PRIORITY_CLASS;
    process->affinity        = 1;
    process->suspend         = 0;
    process->create_flags    = 0;
    process->console_in      = NULL;
    process->console_out     = NULL;
    process->init_event      = NULL;
    process->info            = NULL;
    gettimeofday( &process->start_time, NULL );

    /* copy the request structure */
    if (!(process->info = mem_alloc( sizeof(*process->info) + len ))) goto error;
    memcpy( process->info, req, sizeof(*req) );
    memcpy( process->info->cmdline, cmd_line, len );
    process->info->cmdline[len] = 0;
    req = process->info;  /* use the copy now */
    process->create_flags = req->create_flags;

    if (req->inherit_all)
        process->handles = copy_handle_table( process, parent );
    else
        process->handles = alloc_handle_table( process, 0 );
    if (!process->handles) goto error;

    /* alloc a handle for the process itself */
    alloc_handle( process, process, PROCESS_ALL_ACCESS, 0 );

    /* get the init done event */
    if (req->event != -1)
    {
        if (!(process->init_event = get_event_obj( parent, req->event, EVENT_MODIFY_STATE )))
            goto error;
    }

    /* set the process console */
    if (process->create_flags & CREATE_NEW_CONSOLE)
    {
        if (!alloc_console( process )) goto error;
    }
    else if (!(process->create_flags & DETACHED_PROCESS))
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
    if (process->create_flags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS))
        debugger_attach( process, current );
    else if (parent && parent->debugger && !(parent->create_flags & DEBUG_ONLY_THIS_PROCESS))
        debugger_attach( process, parent->debugger );

    if ((process->next = first_process) != NULL) process->next->prev = process;
    first_process = process;
    return process;

 error:
    free_console( process );
    if (process->handles) release_object( process->handles );
    release_object( process );
    return NULL;
}

/* create the initial process */
struct process *create_initial_process(void)
{
    struct process *process;
    struct new_process_request req;

    req.inherit      = 0;
    req.inherit_all  = 0;
    req.create_flags = CREATE_NEW_CONSOLE;
    req.start_flags  = STARTF_USESTDHANDLES;
    req.hstdin       = -1;
    req.hstdout      = -1;
    req.hstderr      = -1;
    req.event        = -1;
    req.cmd_show     = 0;
    req.env_ptr      = NULL;
    if ((process = create_process( NULL, &req, "", 1 )))
    {
        process->info->hstdin  = alloc_handle( process, process->console_in,
                                               GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, 1 );
        process->info->hstdout = alloc_handle( process, process->console_out,
                                               GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, 1 );
        process->info->hstderr = alloc_handle( process, process->console_out,
                                               GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, 1 );
    }
    return process;
}

/* destroy a process when its refcount is 0 */
static void process_destroy( struct object *obj )
{
    struct process *process = (struct process *)obj;
    assert( obj->ops == &process_ops );

    /* we can't have a thread remaining */
    assert( !process->thread_list );
    if (process->next) process->next->prev = process->prev;
    if (process->prev) process->prev->next = process->next;
    else first_process = process->next;
    if (process->info) free( process->info );
    if (process->init_event) release_object( process->init_event );
}

/* dump a process on stdout for debugging purposes */
static void process_dump( struct object *obj, int verbose )
{
    struct process *process = (struct process *)obj;
    assert( obj->ops == &process_ops );

    fprintf( stderr, "Process next=%p prev=%p console=%p/%p handles=%p\n",
             process->next, process->prev, process->console_in, process->console_out,
             process->handles );
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
    else set_error( ERROR_INVALID_PARAMETER );
    return p;
}

/* get a process from a handle (and increment the refcount) */
struct process *get_process_from_handle( int handle, unsigned int access )
{
    return (struct process *)get_handle_obj( current->process, handle,
                                             access, &process_ops );
}

/* a process has been killed (i.e. its last thread died) */
static void process_killed( struct process *process, int exit_code )
{
    assert( !process->thread_list );
    process->exit_code = exit_code;
    gettimeofday( &process->end_time, NULL );
    release_object( process->handles );
    process->handles = NULL;
    free_console( process );
    wake_up( &process->obj, 0 );
    if (!--running_processes)
    {
        /* last process died, close global handles */
        close_global_handles();
    }
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
            if (!thread->suspend) stop_thread( thread );
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
            if (!thread->suspend) continue_thread( thread );
        }
    }
}

/* kill a process on the spot */
void kill_process( struct process *process, int exit_code )
{
    while (process->thread_list)
        kill_thread( process->thread_list, exit_code );
}

/* kill all processes being debugged by a given thread */
void kill_debugged_processes( struct thread *debugger, int exit_code )
{
    for (;;)  /* restart from the beginning of the list every time */
    {
        struct process *process = first_process;
        /* find the first process being debugged by 'debugger' and still running */
        while (process && (process->debugger != debugger || !process->running_threads))
            process = process->next;
        if (!process) return;
        process->debugger = NULL;
        kill_process( process, exit_code );
    }
}

/* get all information about a process */
static void get_process_info( struct process *process, struct get_process_info_request *req )
{
    req->pid              = process;
    req->exit_code        = process->exit_code;
    req->priority         = process->priority;
    req->process_affinity = process->affinity;
    req->system_affinity  = 1;
}

/* set all information about a process */
static void set_process_info( struct process *process,
                              struct set_process_info_request *req )
{
    if (req->mask & SET_PROCESS_INFO_PRIORITY)
        process->priority = req->priority;
    if (req->mask & SET_PROCESS_INFO_AFFINITY)
    {
        if (req->affinity != 1) set_error( ERROR_INVALID_PARAMETER );
        else process->affinity = req->affinity;
    }
}

/* read data from a process memory space */
/* len is the total size (in ints), max is the size we can actually store in the output buffer */
/* we read the total size in all cases to check for permissions */
static void read_process_memory( struct process *process, const int *addr,
                                 size_t len, size_t max, int *dest )
{
    struct thread *thread = process->thread_list;

    if ((unsigned int)addr % sizeof(int))  /* address must be aligned */
    {
        set_error( ERROR_INVALID_PARAMETER );
        return;
    }
    suspend_thread( thread, 0 );
    if (thread->attached)
    {
        while (len > 0 && max)
        {
            if (read_thread_int( thread, addr++, dest++ ) == -1) goto done;
            max--;
            len--;
        }
        /* check the rest for read permission */
        if (len > 0)
        {
            int dummy, page = get_page_size() / sizeof(int);
            while (len >= page)
            {
                addr += page;
                len -= page;
                if (read_thread_int( thread, addr - 1, &dummy ) == -1) goto done;
            }
            if (len && (read_thread_int( thread, addr + len - 1, &dummy ) == -1)) goto done;
        }
    }
    else set_error( ERROR_ACCESS_DENIED );
 done:
    resume_thread( thread );
}

/* write data to a process memory space */
/* len is the total size (in ints), max is the size we can actually read from the input buffer */
/* we check the total size for write permissions */
static void write_process_memory( struct process *process, int *addr, size_t len,
                                  size_t max, unsigned int first_mask,
                                  unsigned int last_mask, const int *src )
{
    struct thread *thread = process->thread_list;

    if (!len || ((unsigned int)addr % sizeof(int)))  /* address must be aligned */
    {
        set_error( ERROR_INVALID_PARAMETER );
        return;
    }
    suspend_thread( thread, 0 );
    if (thread->attached)
    {
        /* first word is special */
        if (len > 1)
        {
            if (write_thread_int( thread, addr++, *src++, first_mask ) == -1) goto done;
            len--;
            max--;
        }
        else last_mask &= first_mask;

        while (len > 1 && max)
        {
            if (write_thread_int( thread, addr++, *src++, ~0 ) == -1) goto done;
            max--;
            len--;
        }

        if (max)
        {
            /* last word is special too */
            if (write_thread_int( thread, addr, *src, last_mask ) == -1) goto done;
        }
        else
        {
            /* check the rest for write permission */
            int page = get_page_size() / sizeof(int);
            while (len >= page)
            {
                addr += page;
                len -= page;
                if (write_thread_int( thread, addr - 1, 0, 0 ) == -1) goto done;
            }
            if (len && (write_thread_int( thread, addr + len - 1, 0, 0 ) == -1)) goto done;
        }
    }
    else set_error( ERROR_ACCESS_DENIED );
 done:
    resume_thread( thread );
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
    size_t len = get_req_strlen( req->cmdline );
    struct process *process;

    req->handle = -1;
    req->pid    = NULL;
    if ((process = create_process( current->process, req, req->cmdline, len )))
    {
        req->handle = alloc_handle( current->process, process, PROCESS_ALL_ACCESS, req->inherit );
        req->pid    = process;
        release_object( process );
    }
}

/* initialize a new process */
DECL_HANDLER(init_process)
{
    struct new_process_request *info;

    if (!current->unix_pid)
    {
        fatal_protocol_error( current, "init_process: init_thread not called yet\n" );
        return;
    }
    if (!(info = current->process->info))
    {
        fatal_protocol_error( current, "init_process: called twice\n" );
        return;
    }
    current->process->info = NULL;
    req->start_flags = info->start_flags;
    req->hstdin      = info->hstdin;
    req->hstdout     = info->hstdout;
    req->hstderr     = info->hstderr;
    req->cmd_show    = info->cmd_show;
    req->env_ptr     = info->env_ptr;
    strcpy( req->cmdline, info->cmdline );
    free( info );
}

/* signal the end of the process initialization */
DECL_HANDLER(init_process_done)
{
    struct process *process = current->process;
    if (!process->init_event)
    {
        fatal_protocol_error( current, "init_process_done: no event\n" );
        return;
    }
    set_event( process->init_event );
    release_object( process->init_event );
    process->init_event = NULL;
    if (current->suspend + current->process->suspend > 0) stop_thread( current );
}

/* open a handle to a process */
DECL_HANDLER(open_process)
{
    struct process *process = get_process_from_id( req->pid );
    req->handle = -1;
    if (process)
    {
        req->handle = alloc_handle( current->process, process, req->access, req->inherit );
        release_object( process );
    }
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
}

/* fetch information about a process */
DECL_HANDLER(get_process_info)
{
    struct process *process;

    if ((process = get_process_from_handle( req->handle, PROCESS_QUERY_INFORMATION )))
    {
        get_process_info( process, req );
        release_object( process );
    }
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
}

/* read data from a process address space */
DECL_HANDLER(read_process_memory)
{
    struct process *process;

    if ((process = get_process_from_handle( req->handle, PROCESS_VM_READ )))
    {
        read_process_memory( process, req->addr, req->len,
                             get_req_size( req->data, sizeof(int) ), req->data );
        release_object( process );
    }
}

/* write data to a process address space */
DECL_HANDLER(write_process_memory)
{
    struct process *process;

    if ((process = get_process_from_handle( req->handle, PROCESS_VM_WRITE )))
    {
        write_process_memory( process, req->addr, req->len, get_req_size( req->data, sizeof(int) ),
                              req->first_mask, req->last_mask, req->data );
        release_object( process );
    }
}
