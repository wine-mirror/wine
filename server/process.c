/*
 * Server-side process management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include <unistd.h>

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
    sizeof(struct process),      /* size */
    process_dump,                /* dump */
    add_queue,                   /* add_queue */
    remove_queue,                /* remove_queue */
    process_signaled,            /* signaled */
    no_satisfied,                /* satisfied */
    NULL,                        /* get_poll_events */
    NULL,                        /* poll_event */
    no_read_fd,                  /* get_read_fd */
    no_write_fd,                 /* get_write_fd */
    no_flush,                    /* flush */
    no_get_file_info,            /* get_file_info */
    process_destroy              /* destroy */
};

/* process startup info */

struct startup_info
{
    struct object       obj;          /* object header */
    int                 inherit_all;  /* inherit all handles from parent */
    int                 create_flags; /* creation flags */
    int                 start_flags;  /* flags from startup info */
    int                 hstdin;       /* handle for stdin */
    int                 hstdout;      /* handle for stdout */
    int                 hstderr;      /* handle for stderr */
    int                 cmd_show;     /* main window show mode */
    struct file        *exe_file;     /* file handle for main exe */
    char               *filename;     /* file name for main exe */
    struct process     *process;      /* created process */
    struct thread      *thread;       /* created thread */
};

static void startup_info_dump( struct object *obj, int verbose );
static int startup_info_signaled( struct object *obj, struct thread *thread );
static void startup_info_destroy( struct object *obj );

static const struct object_ops startup_info_ops =
{
    sizeof(struct startup_info),   /* size */
    startup_info_dump,             /* dump */
    add_queue,                     /* add_queue */
    remove_queue,                  /* remove_queue */
    startup_info_signaled,         /* signaled */
    no_satisfied,                  /* satisfied */
    NULL,                          /* get_poll_events */
    NULL,                          /* poll_event */
    no_read_fd,                    /* get_read_fd */
    no_write_fd,                   /* get_write_fd */
    no_flush,                      /* flush */
    no_get_file_info,              /* get_file_info */
    startup_info_destroy           /* destroy */
};


/* set the console and stdio handles for a newly created process */
static int set_process_console( struct process *process, struct process *parent,
                                struct startup_info *info, struct init_process_request *req )
{
    if (process->create_flags & CREATE_NEW_CONSOLE)
    {
        if (!alloc_console( process )) return 0;
    }
    else if (parent && !(process->create_flags & DETACHED_PROCESS))
    {
        if (parent->console_in) process->console_in = grab_object( parent->console_in );
        if (parent->console_out) process->console_out = grab_object( parent->console_out );
    }
    if (parent)
    {
        if (!info->inherit_all && !(info->start_flags & STARTF_USESTDHANDLES))
        {
            /* duplicate the handle from the parent into this process */
            req->hstdin  = duplicate_handle( parent, info->hstdin, process,
                                             0, TRUE, DUPLICATE_SAME_ACCESS );
            req->hstdout = duplicate_handle( parent, info->hstdout, process,
                                             0, TRUE, DUPLICATE_SAME_ACCESS );
            req->hstderr = duplicate_handle( parent, info->hstderr, process,
                                             0, TRUE, DUPLICATE_SAME_ACCESS );
        }
        else
        {
            req->hstdin  = info->hstdin;
            req->hstdout = info->hstdout;
            req->hstderr = info->hstderr;
        }
    }
    else
    {
        /* no parent, use handles to the console for stdio */
        req->hstdin  = alloc_handle( process, process->console_in,
                                     GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, 1 );
        req->hstdout = alloc_handle( process, process->console_out,
                                     GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, 1 );
        req->hstderr = alloc_handle( process, process->console_out,
                                     GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, 1 );
    }
    /* some handles above may have been invalid; this is not an error */
    if (get_error() == STATUS_INVALID_HANDLE) clear_error();
    return 1;
}

/* create a new process and its main thread */
struct thread *create_process( int fd )
{
    struct process *process;
    struct thread *thread = NULL;

    if (!(process = alloc_object( &process_ops, -1 )))
    {
        close( fd );
        return NULL;
    }
    process->next            = NULL;
    process->prev            = NULL;
    process->thread_list     = NULL;
    process->debugger        = NULL;
    process->handles         = NULL;
    process->exit_code       = STILL_ACTIVE;
    process->running_threads = 0;
    process->priority        = NORMAL_PRIORITY_CLASS;
    process->affinity        = 1;
    process->suspend         = 0;
    process->create_flags    = 0;
    process->console_in      = NULL;
    process->console_out     = NULL;
    process->init_event      = NULL;
    process->idle_event      = NULL;
    process->queue           = NULL;
    process->atom_table      = NULL;
    process->ldt_copy        = NULL;
    process->ldt_flags       = NULL;
    process->exe.next        = NULL;
    process->exe.prev        = NULL;
    process->exe.file        = NULL;
    process->exe.dbg_offset  = 0;
    process->exe.dbg_size    = 0;
    gettimeofday( &process->start_time, NULL );
    if ((process->next = first_process) != NULL) process->next->prev = process;
    first_process = process;

    /* create the main thread */
    if (!(thread = create_thread( fd, process ))) goto error;

    /* create the init done event */
    if (!(process->init_event = create_event( NULL, 0, 1, 0 ))) goto error;

    add_process_thread( process, thread );
    release_object( process );
    return thread;

 error:
    if (thread) release_object( thread );
    release_object( process );
    return NULL;
}

/* initialize the current process and fill in the request */
static void init_process( int ppid, struct init_process_request *req )
{
    struct process *process = current->process;
    struct thread *parent_thread = get_thread_from_pid( ppid );
    struct process *parent = NULL;
    struct startup_info *info = NULL;

    if (parent_thread)
    {
        parent = parent_thread->process;
        info = parent_thread->info;
        if (!info)
        {
            fatal_protocol_error( current, "init_process: parent but no info\n" );
            return;
        }
        if (info->thread)
        {
            fatal_protocol_error( current, "init_process: called twice?\n" );
            return;
        }
    }

    /* set the process flags */
    process->create_flags = info ? info->create_flags : CREATE_NEW_CONSOLE;

    /* create the handle table */
    if (parent && info->inherit_all == 2)  /* HACK! */
        process->handles = grab_object( parent->handles );
    else if (parent && info->inherit_all)
        process->handles = copy_handle_table( process, parent );
    else
        process->handles = alloc_handle_table( process, 0 );
    if (!process->handles) goto error;

    /* retrieve the main exe file */
    req->exe_file = -1;
    if (parent && info->exe_file)
    {
        process->exe.file = (struct file *)grab_object( info->exe_file );
        if ((req->exe_file = alloc_handle( process, process->exe.file, GENERIC_READ, 0 )) == -1)
            goto error;
    }

    /* set the process console */
    if (!set_process_console( process, parent, info, req )) goto error;

    if (parent)
    {
        /* attach to the debugger if requested */
        if (process->create_flags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS))
            set_process_debugger( process, parent_thread );
        else if (parent && parent->debugger && !(parent->create_flags & DEBUG_ONLY_THIS_PROCESS))
            set_process_debugger( process, parent->debugger );
    }

    /* thread will be actually suspended in init_done */
    if (process->create_flags & CREATE_SUSPENDED) current->suspend++;

    if (info)
    {
        req->start_flags = info->start_flags;
        req->cmd_show    = info->cmd_show;
        strcpy( req->filename, info->filename );
        info->process = (struct process *)grab_object( process );
        info->thread  = (struct thread *)grab_object( current );
        wake_up( &info->obj, 0 );
    }
    else
    {
        req->start_flags  = STARTF_USESTDHANDLES;
        req->cmd_show     = 0;
        req->filename[0]  = 0;
    }
 error:
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
    if (process->init_event) release_object( process->init_event );
    if (process->idle_event) release_object( process->idle_event );
    if (process->queue) release_object( process->queue );
    if (process->atom_table) release_object( process->atom_table );
    if (process->exe.file) release_object( process->exe.file );
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


static void startup_info_destroy( struct object *obj )
{
    struct startup_info *info = (struct startup_info *)obj;
    assert( obj->ops == &startup_info_ops );
    if (info->filename) free( info->filename );
    if (info->exe_file) release_object( info->exe_file );
    if (info->process) release_object( info->process );
    if (info->thread) release_object( info->thread );
}

static void startup_info_dump( struct object *obj, int verbose )
{
    struct startup_info *info = (struct startup_info *)obj;
    assert( obj->ops == &startup_info_ops );

    fprintf( stderr, "Startup info flags=%x in=%d out=%d err=%d name='%s'\n",
             info->start_flags, info->hstdin, info->hstdout, info->hstderr, info->filename );
}

static int startup_info_signaled( struct object *obj, struct thread *thread )
{
    struct startup_info *info = (struct startup_info *)obj;
    return (info->thread != NULL);
}


/* build a reply for the wait_process request */
static void build_wait_process_reply( struct thread *thread, struct object *obj, int signaled )
{
    struct wait_process_request *req = get_req_ptr( thread );
    if (obj)
    {
        struct startup_info *info = (struct startup_info *)obj;
        assert( obj->ops == &startup_info_ops );

        req->pid = get_process_id( info->process );
        req->tid = get_thread_id( info->thread );
        req->phandle = alloc_handle( thread->process, info->process,
                                     PROCESS_ALL_ACCESS, req->pinherit );
        req->thandle = alloc_handle( thread->process, info->thread,
                                     THREAD_ALL_ACCESS, req->tinherit );
        if (info->process->init_event)
            req->event = alloc_handle( thread->process, info->process->init_event,
                                       EVENT_ALL_ACCESS, 0 );
        else
            req->event = -1;

        /* FIXME: set_error */
    }
    release_object( thread->info );
    thread->info = NULL;
}

/* get a process from an id (and increment the refcount) */
struct process *get_process_from_id( void *id )
{
    struct process *p = first_process;
    while (p && (p != id)) p = p->next;
    if (p) grab_object( p );
    else set_error( STATUS_INVALID_PARAMETER );
    return p;
}

/* get a process from a handle (and increment the refcount) */
struct process *get_process_from_handle( int handle, unsigned int access )
{
    return (struct process *)get_handle_obj( current->process, handle,
                                             access, &process_ops );
}

/* add a dll to a process list */
static struct process_dll *process_load_dll( struct process *process, struct file *file,
                                             void *base )
{
    struct process_dll *dll;

    /* make sure we don't already have one with the same base address */
    for (dll = process->exe.next; dll; dll = dll->next) if (dll->base == base)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }

    if ((dll = mem_alloc( sizeof(*dll) )))
    {
        dll->prev = &process->exe;
        dll->file = NULL;
        dll->base = base;
        if (file) dll->file = (struct file *)grab_object( file );
        if ((dll->next = process->exe.next)) dll->next->prev = dll;
        process->exe.next = dll;
    }
    return dll;
}

/* remove a dll from a process list */
static void process_unload_dll( struct process *process, void *base )
{
    struct process_dll *dll;

    for (dll = process->exe.next; dll; dll = dll->next)
    {
        if (dll->base == base)
        {
            if (dll->file) release_object( dll->file );
            if (dll->next) dll->next->prev = dll->prev;
            if (dll->prev) dll->prev->next = dll->next;
            free( dll );
            generate_debug_event( current, UNLOAD_DLL_DEBUG_EVENT, base );
            return;
        }
    }
    set_error( STATUS_INVALID_PARAMETER );
}

/* a process has been killed (i.e. its last thread died) */
static void process_killed( struct process *process )
{
    assert( !process->thread_list );
    gettimeofday( &process->end_time, NULL );
    if (process->handles) release_object( process->handles );
    process->handles = NULL;
    free_console( process );
    while (process->exe.next)
    {
        struct process_dll *dll = process->exe.next;
        process->exe.next = dll->next;
        if (dll->file) release_object( dll->file );
        free( dll );
    }
    if (process->exe.file) release_object( process->exe.file );
    process->exe.file = NULL;
    wake_up( &process->obj, 0 );
    if (!--running_processes)
    {
        /* last process died, close global handles */
        close_global_handles();
        /* this will cause the select loop to terminate */
        if (!persistent_server) close_master_socket();
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
        process->exit_code = thread->exit_code;
        generate_debug_event( thread, EXIT_PROCESS_DEBUG_EVENT, process );
        process_killed( process );
    }
    else generate_debug_event( thread, EXIT_THREAD_DEBUG_EVENT, thread );
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
static void kill_process( struct process *process, struct thread *skip, int exit_code )
{
    struct thread *thread = process->thread_list;
    while (thread)
    {
        struct thread *next = thread->proc_next;
        thread->exit_code = exit_code;
        if (thread != skip) kill_thread( thread, 1 );
        thread = next;
    }
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
        kill_process( process, NULL, exit_code );
    }
}

/* get all information about a process */
static void get_process_info( struct process *process, struct get_process_info_request *req )
{
    req->pid              = process;
    req->debugged         = (process->debugger != 0);
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
        if (req->affinity != 1) set_error( STATUS_INVALID_PARAMETER );
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
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    if (!thread)  /* process is dead */
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    if (suspend_for_ptrace( thread ))
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
    done:
        resume_thread( thread );
    }
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
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    if (!thread)  /* process is dead */
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    if (suspend_for_ptrace( thread ))
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
    done:
        resume_thread( thread );
    }
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
        ptr->count    = process->obj.refcount;
        ptr->priority = process->priority;
        grab_object( process );
        ptr++;
    }
    *count = running_processes;
    return snapshot;
}

/* take a snapshot of the modules of a process */
struct module_snapshot *module_snap( struct process *process, int *count )
{
    struct module_snapshot *snapshot, *ptr;
    struct process_dll *dll;
    int total = 0;

    for (dll = &process->exe; dll; dll = dll->next) total++;
    if (!(snapshot = mem_alloc( sizeof(*snapshot) * total ))) return NULL;

    for (ptr = snapshot, dll = &process->exe; dll; dll = dll->next, ptr++)
    {
        ptr->base = dll->base;
    }
    *count = total;
    return snapshot;
}


/* create a new process */
DECL_HANDLER(new_process)
{
    size_t len = get_req_strlen( req, req->filename );
    struct startup_info *info;
    int sock[2];

    if (current->info)
    {
        fatal_protocol_error( current, "new_process: another process is being created\n" );
        return;
    }

    /* build the startup info for a new process */
    if (!(info = alloc_object( &startup_info_ops, -1 ))) return;
    info->inherit_all  = req->inherit_all;
    info->create_flags = req->create_flags;
    info->start_flags  = req->start_flags;
    info->hstdin       = req->hstdin;
    info->hstdout      = req->hstdout;
    info->hstderr      = req->hstderr;
    info->cmd_show     = req->cmd_show;
    info->exe_file     = NULL;
    info->filename     = NULL;
    info->process      = NULL;
    info->thread       = NULL;

    if ((req->exe_file != -1) &&
        !(info->exe_file = get_file_obj( current->process, req->exe_file, GENERIC_READ )))
    {
        release_object( info );
        return;
    }

    if (!(info->filename = memdup( req->filename, len+1 )))
    {
        release_object( info );
        return;
    }
    info->filename[len] = 0;

    if (req->alloc_fd)
    {
        if (socketpair( AF_UNIX, SOCK_STREAM, 0, sock ) == -1)
        {
            file_set_error();
            release_object( info );
            return;
        }
        if (!create_process( sock[0] ))
        {
            release_object( info );
            close( sock[1] );
            return;
        }
        /* thread object will be released when the thread gets killed */
        set_reply_fd( current, sock[1] );
    }
    current->info = info;
}

/* Wait for the new process to start */
DECL_HANDLER(wait_process)
{
    if (!current->info)
    {
        fatal_protocol_error( current, "wait_process: no process is being created\n" );
        return;
    }
    req->pid     = 0;
    req->tid     = 0;
    req->phandle = -1;
    req->thandle = -1;
    req->event   = -1;
    if (req->cancel)
    {
        release_object( current->info );
        current->info = NULL;
    }
    else
    {
        struct object *obj = &current->info->obj;
        sleep_on( 1, &obj, SELECT_TIMEOUT, req->timeout, build_wait_process_reply );
    }
}

/* initialize a new process */
DECL_HANDLER(init_process)
{
    if (!current->unix_pid)
    {
        fatal_protocol_error( current, "init_process: init_thread not called yet\n" );
        return;
    }
    current->process->ldt_copy  = req->ldt_copy;
    current->process->ldt_flags = req->ldt_flags;
    init_process( req->ppid, req );
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
    process->exe.base = req->module;
    process->exe.name = req->name;
    generate_startup_debug_events( current->process, req->entry );
    set_event( process->init_event );
    release_object( process->init_event );
    process->init_event = NULL;
    if (req->gui) process->idle_event = create_event( NULL, 0, 1, 0 );
    if (current->suspend + current->process->suspend > 0) stop_thread( current );
    req->debugged = (current->process->debugger != 0);
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
        req->self = (current->process == process);
        kill_process( process, current, req->exit_code );
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
                             get_req_size( req, req->data, sizeof(int) ), req->data );
        release_object( process );
    }
}

/* write data to a process address space */
DECL_HANDLER(write_process_memory)
{
    struct process *process;

    if ((process = get_process_from_handle( req->handle, PROCESS_VM_WRITE )))
    {
        write_process_memory( process, req->addr, req->len,
                              get_req_size( req, req->data, sizeof(int) ),
                              req->first_mask, req->last_mask, req->data );
        release_object( process );
    }
}

/* notify the server that a dll has been loaded */
DECL_HANDLER(load_dll)
{
    struct process_dll *dll;
    struct file *file = NULL;

    if ((req->handle != -1) &&
        !(file = get_file_obj( current->process, req->handle, GENERIC_READ ))) return;
    
    if ((dll = process_load_dll( current->process, file, req->base )))
    {
        dll->dbg_offset = req->dbg_offset;
        dll->dbg_size   = req->dbg_size;
        dll->name       = req->name;
        /* only generate event if initialization is done */
        if (!current->process->init_event)
            generate_debug_event( current, LOAD_DLL_DEBUG_EVENT, dll );
    }
    if (file) release_object( file );
}

/* notify the server that a dll is being unloaded */
DECL_HANDLER(unload_dll)
{
    process_unload_dll( current->process, req->base );
}

/* wait for a process to start waiting on input */
/* FIXME: only returns event for now, wait is done in the client */
DECL_HANDLER(wait_input_idle)
{
    struct process *process;

    req->event = -1;
    if ((process = get_process_from_handle( req->handle, PROCESS_QUERY_INFORMATION )))
    {
        if (process->idle_event && process != current->process && process->queue != current->queue)
            req->event = alloc_handle( current->process, process->idle_event,
                                       EVENT_ALL_ACCESS, 0 );
        release_object( process );
    }
}
