/*
 * Server-side process management
 *
 * Copyright (C) 1998 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include <unistd.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"

#include "file.h"
#include "handle.h"
#include "process.h"
#include "thread.h"
#include "request.h"
#include "console.h"

/* process structure */

static struct process *first_process;
static int running_processes;

/* process operations */

static void process_dump( struct object *obj, int verbose );
static int process_signaled( struct object *obj, struct thread *thread );
static void process_poll_event( struct fd *fd, int event );
static void process_destroy( struct object *obj );

static const struct object_ops process_ops =
{
    sizeof(struct process),      /* size */
    process_dump,                /* dump */
    add_queue,                   /* add_queue */
    remove_queue,                /* remove_queue */
    process_signaled,            /* signaled */
    no_satisfied,                /* satisfied */
    no_get_fd,                   /* get_fd */
    process_destroy              /* destroy */
};

static const struct fd_ops process_fd_ops =
{
    NULL,                        /* get_poll_events */
    process_poll_event,          /* poll_event */
    no_flush,                    /* flush */
    no_get_file_info,            /* get_file_info */
    no_queue_async               /* queue_async */
};

/* process startup info */

struct startup_info
{
    struct object       obj;          /* object header */
    struct list         entry;        /* entry in list of startup infos */
    int                 inherit_all;  /* inherit all handles from parent */
    int                 create_flags; /* creation flags */
    int                 unix_pid;     /* Unix pid of new process */
    obj_handle_t        hstdin;       /* handle for stdin */
    obj_handle_t        hstdout;      /* handle for stdout */
    obj_handle_t        hstderr;      /* handle for stderr */
    struct file        *exe_file;     /* file handle for main exe */
    struct thread      *owner;        /* owner thread (the one that created the new process) */
    struct process     *process;      /* created process */
    struct thread      *thread;       /* created thread */
    size_t              data_size;    /* size of startup data */
    void               *data;         /* data for startup info */
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
    no_get_fd,                     /* get_fd */
    startup_info_destroy           /* destroy */
};


static struct list startup_info_list = LIST_INIT(startup_info_list);

struct ptid_entry
{
    void        *ptr;   /* entry ptr */
    unsigned int next;  /* next free entry */
};

static struct ptid_entry *ptid_entries;     /* array of ptid entries */
static unsigned int used_ptid_entries;      /* number of entries in use */
static unsigned int alloc_ptid_entries;     /* number of allocated entries */
static unsigned int next_free_ptid;         /* next free entry */
static unsigned int last_free_ptid;         /* last free entry */

#define PTID_OFFSET 8  /* offset for first ptid value */

/* allocate a new process or thread id */
unsigned int alloc_ptid( void *ptr )
{
    struct ptid_entry *entry;
    unsigned int id;

    if (used_ptid_entries < alloc_ptid_entries)
    {
        id = used_ptid_entries + PTID_OFFSET;
        entry = &ptid_entries[used_ptid_entries++];
    }
    else if (next_free_ptid)
    {
        id = next_free_ptid;
        entry = &ptid_entries[id - PTID_OFFSET];
        if (!(next_free_ptid = entry->next)) last_free_ptid = 0;
    }
    else  /* need to grow the array */
    {
        unsigned int count = alloc_ptid_entries + (alloc_ptid_entries / 2);
        if (!count) count = 64;
        if (!(entry = realloc( ptid_entries, count * sizeof(*entry) )))
        {
            set_error( STATUS_NO_MEMORY );
            return 0;
        }
        ptid_entries = entry;
        alloc_ptid_entries = count;
        id = used_ptid_entries + PTID_OFFSET;
        entry = &ptid_entries[used_ptid_entries++];
    }

    entry->ptr = ptr;
    return id;
}

/* free a process or thread id */
void free_ptid( unsigned int id )
{
    struct ptid_entry *entry = &ptid_entries[id - PTID_OFFSET];

    entry->ptr  = NULL;
    entry->next = 0;

    /* append to end of free list so that we don't reuse it too early */
    if (last_free_ptid) ptid_entries[last_free_ptid - PTID_OFFSET].next = id;
    else next_free_ptid = id;

    last_free_ptid = id;
}

/* retrieve the pointer corresponding to a process or thread id */
void *get_ptid_entry( unsigned int id )
{
    if (id < PTID_OFFSET) return NULL;
    if (id - PTID_OFFSET >= used_ptid_entries) return NULL;
    return ptid_entries[id - PTID_OFFSET].ptr;
}

/* set the state of the process startup info */
static void set_process_startup_state( struct process *process, enum startup_state state )
{
    if (process->startup_state == STARTUP_IN_PROGRESS) process->startup_state = state;
    if (process->startup_info)
    {
        wake_up( &process->startup_info->obj, 0 );
        release_object( process->startup_info );
        process->startup_info = NULL;
    }
}

/* set the console and stdio handles for a newly created process */
static int set_process_console( struct process *process, struct thread *parent_thread,
                                struct startup_info *info, struct init_process_reply *reply )
{
    if (process->create_flags & CREATE_NEW_CONSOLE)
    {
        /* let the process init do the allocation */
        return 1;
    }
    else if (info && !(process->create_flags & DETACHED_PROCESS))
    {
        /* FIXME: some better error checking should be done...
         * like if hConOut and hConIn are console handles, then they should be on the same
         * physical console
         */
        inherit_console( parent_thread, process, info->inherit_all ? info->hstdin : 0 );
    }
    if (info)
    {
        if (!info->inherit_all)
        {
            reply->hstdin  = duplicate_handle( parent_thread->process, info->hstdin, process,
                                               0, TRUE, DUPLICATE_SAME_ACCESS );
            reply->hstdout = duplicate_handle( parent_thread->process, info->hstdout, process,
                                               0, TRUE, DUPLICATE_SAME_ACCESS );
            reply->hstderr = duplicate_handle( parent_thread->process, info->hstderr, process,
                                               0, TRUE, DUPLICATE_SAME_ACCESS );
        }
        else
        {
            reply->hstdin  = info->hstdin;
            reply->hstdout = info->hstdout;
            reply->hstderr = info->hstderr;
        }
    }
    else reply->hstdin = reply->hstdout = reply->hstderr = 0;
    /* some handles above may have been invalid; this is not an error */
    if (get_error() == STATUS_INVALID_HANDLE ||
        get_error() == STATUS_OBJECT_TYPE_MISMATCH) clear_error();
    return 1;
}

/* create a new process and its main thread */
struct thread *create_process( int fd )
{
    struct process *process;
    struct thread *thread = NULL;
    int request_pipe[2];

    if (!(process = alloc_object( &process_ops ))) goto error;
    process->next            = NULL;
    process->prev            = NULL;
    process->parent          = NULL;
    process->thread_list     = NULL;
    process->debugger        = NULL;
    process->handles         = NULL;
    process->msg_fd          = NULL;
    process->exit_code       = STILL_ACTIVE;
    process->running_threads = 0;
    process->priority        = NORMAL_PRIORITY_CLASS;
    process->affinity        = 1;
    process->suspend         = 0;
    process->create_flags    = 0;
    process->console         = NULL;
    process->startup_state   = STARTUP_IN_PROGRESS;
    process->startup_info    = NULL;
    process->idle_event      = NULL;
    process->queue           = NULL;
    process->atom_table      = NULL;
    process->ldt_copy        = NULL;
    process->exe.next        = NULL;
    process->exe.prev        = NULL;
    process->exe.file        = NULL;
    process->exe.dbg_offset  = 0;
    process->exe.dbg_size    = 0;
    process->exe.namelen     = 0;
    process->exe.filename    = NULL;
    process->group_id        = 0;
    process->token           = create_token();
    list_init( &process->locks );

    gettimeofday( &process->start_time, NULL );
    if ((process->next = first_process) != NULL) process->next->prev = process;
    first_process = process;

    if (!(process->id = alloc_ptid( process ))) goto error;
    if (!(process->msg_fd = create_anonymous_fd( &process_fd_ops, fd, &process->obj ))) goto error;

    /* create the main thread */
    if (pipe( request_pipe ) == -1)
    {
        file_set_error();
        goto error;
    }
    if (send_client_fd( process, request_pipe[1], 0 ) == -1)
    {
        close( request_pipe[0] );
        close( request_pipe[1] );
        goto error;
    }
    close( request_pipe[1] );
    if (!(thread = create_thread( request_pipe[0], process ))) goto error;

    set_fd_events( process->msg_fd, POLLIN );  /* start listening to events */
    release_object( process );
    return thread;

 error:
    if (process) release_object( process );
    /* if we failed to start our first process, close everything down */
    if (!running_processes) close_master_socket();
    return NULL;
}

/* find the startup info for a given Unix process */
inline static struct startup_info *find_startup_info( int unix_pid )
{
    struct list *ptr;

    LIST_FOR_EACH( ptr, &startup_info_list )
    {
        struct startup_info *info = LIST_ENTRY( ptr, struct startup_info, entry );
        if (info->unix_pid == unix_pid) return info;
    }
    return NULL;
}

/* initialize the current process and fill in the request */
static struct startup_info *init_process( struct init_process_reply *reply )
{
    struct process *process = current->process;
    struct thread *parent_thread = NULL;
    struct process *parent = NULL;
    struct startup_info *info = find_startup_info( current->unix_pid );

    if (info)
    {
        if (info->thread)
        {
            fatal_protocol_error( current, "init_process: called twice?\n" );
            return NULL;
        }
        parent_thread = info->owner;
        parent = parent_thread->process;
        process->parent = (struct process *)grab_object( parent );
    }

    /* set the process flags */
    process->create_flags = info ? info->create_flags : 0;

    /* create the handle table */
    if (info && info->inherit_all)
        process->handles = copy_handle_table( process, parent );
    else
        process->handles = alloc_handle_table( process, 0 );
    if (!process->handles) return NULL;

    /* retrieve the main exe file */
    reply->exe_file = 0;
    if (info && info->exe_file)
    {
        process->exe.file = (struct file *)grab_object( info->exe_file );
        if (!(reply->exe_file = alloc_handle( process, process->exe.file, GENERIC_READ, 0 )))
            return NULL;
    }

    /* set the process console */
    if (!set_process_console( process, parent_thread, info, reply )) return NULL;

    process->group_id = get_process_id( process );
    if (parent)
    {
        /* attach to the debugger if requested */
        if (process->create_flags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS))
            set_process_debugger( process, parent_thread );
        else if (parent->debugger && !(parent->create_flags & DEBUG_ONLY_THIS_PROCESS))
            set_process_debugger( process, parent->debugger );
        if (!(process->create_flags & CREATE_NEW_PROCESS_GROUP))
            process->group_id = parent->group_id;
    }

    /* thread will be actually suspended in init_done */
    if (process->create_flags & CREATE_SUSPENDED) current->suspend++;

    if (info)
    {
        reply->info_size = info->data_size;
        info->process = (struct process *)grab_object( process );
        info->thread  = (struct thread *)grab_object( current );
    }
    reply->create_flags = process->create_flags;
    reply->server_start = server_start_ticks;
    return info ? (struct startup_info *)grab_object( info ) : NULL;
}

/* destroy a process when its refcount is 0 */
static void process_destroy( struct object *obj )
{
    struct process *process = (struct process *)obj;
    assert( obj->ops == &process_ops );

    /* we can't have a thread remaining */
    assert( !process->thread_list );

    set_process_startup_state( process, STARTUP_ABORTED );
    if (process->console) release_object( process->console );
    if (process->parent) release_object( process->parent );
    if (process->msg_fd) release_object( process->msg_fd );
    if (process->next) process->next->prev = process->prev;
    if (process->prev) process->prev->next = process->next;
    else first_process = process->next;
    if (process->idle_event) release_object( process->idle_event );
    if (process->queue) release_object( process->queue );
    if (process->atom_table) release_object( process->atom_table );
    if (process->exe.file) release_object( process->exe.file );
    if (process->exe.filename) free( process->exe.filename );
    if (process->id) free_ptid( process->id );
    if (process->token) release_object( process->token );
}

/* dump a process on stdout for debugging purposes */
static void process_dump( struct object *obj, int verbose )
{
    struct process *process = (struct process *)obj;
    assert( obj->ops == &process_ops );

    fprintf( stderr, "Process id=%04x next=%p prev=%p handles=%p\n",
             process->id, process->next, process->prev, process->handles );
}

static int process_signaled( struct object *obj, struct thread *thread )
{
    struct process *process = (struct process *)obj;
    return !process->running_threads;
}

static void process_poll_event( struct fd *fd, int event )
{
    struct process *process = get_fd_user( fd );
    assert( process->obj.ops == &process_ops );

    if (event & (POLLERR | POLLHUP)) set_fd_events( fd, -1 );
    else if (event & POLLIN) receive_fd( process );
}

static void startup_info_destroy( struct object *obj )
{
    struct startup_info *info = (struct startup_info *)obj;
    assert( obj->ops == &startup_info_ops );
    list_remove( &info->entry );
    if (info->data) free( info->data );
    if (info->exe_file) release_object( info->exe_file );
    if (info->process) release_object( info->process );
    if (info->thread) release_object( info->thread );
    if (info->owner) release_object( info->owner );
}

static void startup_info_dump( struct object *obj, int verbose )
{
    struct startup_info *info = (struct startup_info *)obj;
    assert( obj->ops == &startup_info_ops );

    fprintf( stderr, "Startup info flags=%x in=%p out=%p err=%p\n",
             info->create_flags, info->hstdin, info->hstdout, info->hstderr );
}

static int startup_info_signaled( struct object *obj, struct thread *thread )
{
    struct startup_info *info = (struct startup_info *)obj;
    return info->process && is_process_init_done(info->process);
}

/* get a process from an id (and increment the refcount) */
struct process *get_process_from_id( process_id_t id )
{
    struct object *obj = get_ptid_entry( id );

    if (obj && obj->ops == &process_ops) return (struct process *)grab_object( obj );
    set_error( STATUS_INVALID_PARAMETER );
    return NULL;
}

/* get a process from a handle (and increment the refcount) */
struct process *get_process_from_handle( obj_handle_t handle, unsigned int access )
{
    return (struct process *)get_handle_obj( current->process, handle,
                                             access, &process_ops );
}

/* add a dll to a process list */
static struct process_dll *process_load_dll( struct process *process, struct file *file,
                                             void *base, const WCHAR *filename, size_t name_len )
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
        dll->filename = NULL;
        dll->namelen  = name_len;
        if (name_len && !(dll->filename = memdup( filename, name_len )))
        {
            free( dll );
            return NULL;
        }
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
            if (dll->filename) free( dll->filename );
            free( dll );
            generate_debug_event( current, UNLOAD_DLL_DEBUG_EVENT, base );
            return;
        }
    }
    set_error( STATUS_INVALID_PARAMETER );
}

/* kill all processes */
void kill_all_processes( struct process *skip, int exit_code )
{
    for (;;)
    {
        struct process *process = first_process;

        while (process && (!process->running_threads || process == skip))
            process = process->next;
        if (!process) break;
        kill_process( process, NULL, exit_code );
    }
}

/* kill all processes being attached to a console renderer */
void kill_console_processes( struct thread *renderer, int exit_code )
{
    for (;;)  /* restart from the beginning of the list every time */
    {
        struct process *process = first_process;

        /* find the first process being attached to 'renderer' and still running */
        while (process &&
               (process == renderer->process || !process->console ||
                process->console->renderer != renderer || !process->running_threads))
        {
            process = process->next;
        }
        if (!process) break;
        kill_process( process, NULL, exit_code );
    }
}

/* a process has been killed (i.e. its last thread died) */
static void process_killed( struct process *process )
{
    assert( !process->thread_list );
    gettimeofday( &process->end_time, NULL );
    if (process->handles) release_object( process->handles );
    process->handles = NULL;

    /* close the console attached to this process, if any */
    free_console( process );

    while (process->exe.next)
    {
        struct process_dll *dll = process->exe.next;
        process->exe.next = dll->next;
        if (dll->file) release_object( dll->file );
        if (dll->filename) free( dll->filename );
        free( dll );
    }
    set_process_startup_state( process, STARTUP_ABORTED );
    if (process->exe.file) release_object( process->exe.file );
    process->exe.file = NULL;
    wake_up( &process->obj, 0 );
    if (!--running_processes) close_master_socket();
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
        while (thread)
        {
            struct thread *next = thread->proc_next;
            if (!thread->suspend) stop_thread( thread );
            thread = next;
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
        while (thread)
        {
            struct thread *next = thread->proc_next;
            if (!thread->suspend) wake_thread( thread );
            thread = next;
        }
    }
}

/* kill a process on the spot */
void kill_process( struct process *process, struct thread *skip, int exit_code )
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


/* detach a debugger from all its debuggees */
void detach_debugged_processes( struct thread *debugger )
{
    struct process *process;
    for (process = first_process; process; process = process->next)
    {
        if (process->debugger == debugger && process->running_threads)
        {
            debugger_detach( process, debugger );
        }
    }
}


void enum_processes( int (*cb)(struct process*, void*), void *user )
{
    struct process *process;
    for (process = first_process; process; process = process->next)
    {
        if ((cb)(process, user)) break;
    }
}


/* get all information about a process */
static void get_process_info( struct process *process, struct get_process_info_reply *reply )
{
    reply->pid              = get_process_id( process );
    reply->debugged         = (process->debugger != 0);
    reply->exit_code        = process->exit_code;
    reply->priority         = process->priority;
    reply->process_affinity = process->affinity;
    reply->system_affinity  = 1;
}

/* set all information about a process */
static void set_process_info( struct process *process,
                              const struct set_process_info_request *req )
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
/* len is the total size (in ints) */
static int read_process_memory( struct process *process, const int *addr, size_t len, int *dest )
{
    struct thread *thread = process->thread_list;

    assert( !((unsigned int)addr % sizeof(int)) );  /* address must be aligned */

    if (!thread)  /* process is dead */
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    if (suspend_for_ptrace( thread ))
    {
        while (len > 0)
        {
            if (read_thread_int( thread, addr++, dest++ ) == -1) break;
            len--;
        }
        resume_after_ptrace( thread );
    }
    return !len;
}

/* make sure we can write to the whole address range */
/* len is the total size (in ints) */
static int check_process_write_access( struct thread *thread, int *addr, size_t len )
{
    int page = get_page_size() / sizeof(int);

    for (;;)
    {
        if (write_thread_int( thread, addr, 0, 0 ) == -1) return 0;
        if (len <= page) break;
        addr += page;
        len -= page;
    }
    return (write_thread_int( thread, addr + len - 1, 0, 0 ) != -1);
}

/* write data to a process memory space */
/* len is the total size (in ints), max is the size we can actually read from the input buffer */
/* we check the total size for write permissions */
static void write_process_memory( struct process *process, int *addr, size_t len,
                                  unsigned int first_mask, unsigned int last_mask, const int *src )
{
    struct thread *thread = process->thread_list;

    assert( !((unsigned int)addr % sizeof(int) ));  /* address must be aligned */

    if (!thread)  /* process is dead */
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    if (suspend_for_ptrace( thread ))
    {
        if (!check_process_write_access( thread, addr, len ))
        {
            set_error( STATUS_ACCESS_DENIED );
            goto done;
        }
        /* first word is special */
        if (len > 1)
        {
            if (write_thread_int( thread, addr++, *src++, first_mask ) == -1) goto done;
            len--;
        }
        else last_mask &= first_mask;

        while (len > 1)
        {
            if (write_thread_int( thread, addr++, *src++, ~0 ) == -1) goto done;
            len--;
        }

        /* last word is special too */
        if (write_thread_int( thread, addr, *src, last_mask ) == -1) goto done;

    done:
        resume_after_ptrace( thread );
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
        ptr->handles  = get_handle_table_count(process);
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
        ptr->base     = dll->base;
        ptr->size     = dll->size;
        ptr->namelen  = dll->namelen;
        ptr->filename = memdup( dll->filename, dll->namelen );
    }
    *count = total;
    return snapshot;
}


/* create a new process */
DECL_HANDLER(new_process)
{
    struct startup_info *info;

    /* build the startup info for a new process */
    if (!(info = alloc_object( &startup_info_ops ))) return;
    list_add_head( &startup_info_list, &info->entry );
    info->inherit_all  = req->inherit_all;
    info->create_flags = req->create_flags;
    info->unix_pid     = req->unix_pid;
    info->hstdin       = req->hstdin;
    info->hstdout      = req->hstdout;
    info->hstderr      = req->hstderr;
    info->exe_file     = NULL;
    info->owner        = (struct thread *)grab_object( current );
    info->process      = NULL;
    info->thread       = NULL;
    info->data_size    = get_req_data_size();
    info->data         = NULL;

    if (req->exe_file &&
        !(info->exe_file = get_file_obj( current->process, req->exe_file, GENERIC_READ )))
        goto done;

    if (!(info->data = memdup( get_req_data(), info->data_size ))) goto done;
    reply->info = alloc_handle( current->process, info, SYNCHRONIZE, FALSE );

 done:
    release_object( info );
}

/* Retrieve information about a newly started process */
DECL_HANDLER(get_new_process_info)
{
    struct startup_info *info;

    if ((info = (struct startup_info *)get_handle_obj( current->process, req->info,
                                                       0, &startup_info_ops )))
    {
        reply->pid = get_process_id( info->process );
        reply->tid = get_thread_id( info->thread );
        reply->phandle = alloc_handle( current->process, info->process,
                                       PROCESS_ALL_ACCESS, req->pinherit );
        reply->thandle = alloc_handle( current->process, info->thread,
                                       THREAD_ALL_ACCESS, req->tinherit );
        reply->success = is_process_init_done( info->process );
        release_object( info );
    }
    else
    {
        reply->pid     = 0;
        reply->tid     = 0;
        reply->phandle = 0;
        reply->thandle = 0;
        reply->success = 0;
    }
}

/* Retrieve the new process startup info */
DECL_HANDLER(get_startup_info)
{
    struct startup_info *info;

    if ((info = current->process->startup_info))
    {
        size_t size = info->data_size;
        if (size > get_reply_max_size()) size = get_reply_max_size();

        /* we return the data directly without making a copy so this can only be called once */
        set_reply_data_ptr( info->data, size );
        info->data = NULL;
        info->data_size = 0;
    }
}


/* initialize a new process */
DECL_HANDLER(init_process)
{
    if (current->unix_pid == -1)
    {
        fatal_protocol_error( current, "init_process: init_thread not called yet\n" );
        return;
    }
    if (current->process->startup_info)
    {
        fatal_protocol_error( current, "init_process: called twice\n" );
        return;
    }
    reply->info_size = 0;
    current->process->ldt_copy = req->ldt_copy;
    current->process->startup_info = init_process( reply );
}

/* signal the end of the process initialization */
DECL_HANDLER(init_process_done)
{
    struct file *file = NULL;
    struct process *process = current->process;

    if (is_process_init_done(process))
    {
        fatal_protocol_error( current, "init_process_done: called twice\n" );
        return;
    }
    if (!req->module)
    {
        fatal_protocol_error( current, "init_process_done: module base address cannot be 0\n" );
        return;
    }
    process->exe.base = req->module;
    process->exe.size = req->module_size;
    process->exe.name = req->name;

    if (req->exe_file) file = get_file_obj( process, req->exe_file, GENERIC_READ );
    if (process->exe.file) release_object( process->exe.file );
    process->exe.file = file;

    if ((process->exe.namelen = get_req_data_size()))
        process->exe.filename = memdup( get_req_data(), process->exe.namelen );

    generate_startup_debug_events( process, req->entry );
    set_process_startup_state( process, STARTUP_DONE );

    if (req->gui) process->idle_event = create_event( NULL, 0, 1, 0 );
    if (current->suspend + process->suspend > 0) stop_thread( current );
    reply->debugged = (process->debugger != 0);
}

/* open a handle to a process */
DECL_HANDLER(open_process)
{
    struct process *process = get_process_from_id( req->pid );
    reply->handle = 0;
    if (process)
    {
        reply->handle = alloc_handle( current->process, process, req->access, req->inherit );
        release_object( process );
    }
}

/* terminate a process */
DECL_HANDLER(terminate_process)
{
    struct process *process;

    if ((process = get_process_from_handle( req->handle, PROCESS_TERMINATE )))
    {
        reply->self = (current->process == process);
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
        get_process_info( process, reply );
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
    size_t len = get_reply_max_size();

    if (!(process = get_process_from_handle( req->handle, PROCESS_VM_READ ))) return;

    if (len)
    {
        unsigned int start_offset = (unsigned int)req->addr % sizeof(int);
        unsigned int nb_ints = (len + start_offset + sizeof(int) - 1) / sizeof(int);
        const int *start = (int *)((char *)req->addr - start_offset);
        int *buffer = mem_alloc( nb_ints * sizeof(int) );
        if (buffer)
        {
            if (read_process_memory( process, start, nb_ints, buffer ))
            {
                /* move start of requested data to start of buffer */
                if (start_offset) memmove( buffer, (char *)buffer + start_offset, len );
                set_reply_data_ptr( buffer, len );
            }
            else len = 0;
        }
    }
    release_object( process );
}

/* write data to a process address space */
DECL_HANDLER(write_process_memory)
{
    struct process *process;

    if ((process = get_process_from_handle( req->handle, PROCESS_VM_WRITE )))
    {
        size_t len = get_req_data_size();
        if ((len % sizeof(int)) || ((unsigned int)req->addr % sizeof(int)))
            set_error( STATUS_INVALID_PARAMETER );
        else
        {
            if (len) write_process_memory( process, req->addr, len / sizeof(int),
                                           req->first_mask, req->last_mask, get_req_data() );
        }
        release_object( process );
    }
}

/* notify the server that a dll has been loaded */
DECL_HANDLER(load_dll)
{
    struct process_dll *dll;
    struct file *file = NULL;

    if (req->handle &&
        !(file = get_file_obj( current->process, req->handle, GENERIC_READ ))) return;

    if ((dll = process_load_dll( current->process, file, req->base,
                                 get_req_data(), get_req_data_size() )))
    {
        dll->size       = req->size;
        dll->dbg_offset = req->dbg_offset;
        dll->dbg_size   = req->dbg_size;
        dll->name       = req->name;
        /* only generate event if initialization is done */
        if (is_process_init_done( current->process ))
            generate_debug_event( current, LOAD_DLL_DEBUG_EVENT, dll );
    }
    if (file) release_object( file );
}

/* notify the server that a dll is being unloaded */
DECL_HANDLER(unload_dll)
{
    process_unload_dll( current->process, req->base );
}

/* retrieve information about a module in a process */
DECL_HANDLER(get_dll_info)
{
    struct process *process;

    if ((process = get_process_from_handle( req->handle, PROCESS_QUERY_INFORMATION )))
    {
        struct process_dll *dll;

        for (dll = &process->exe; dll; dll = dll->next)
        {
            if (dll->base == req->base_address)
            {
                reply->size = dll->size;
                reply->entry_point = 0; /* FIXME */
                if (dll->filename)
                {
                    size_t len = min( dll->namelen, get_reply_max_size() );
                    set_reply_data( dll->filename, len );
                }
                break;
            }
        }
        if (!dll)
            set_error(STATUS_DLL_NOT_FOUND);
        release_object( process );
    }
}

/* wait for a process to start waiting on input */
/* FIXME: only returns event for now, wait is done in the client */
DECL_HANDLER(wait_input_idle)
{
    struct process *process;

    reply->event = 0;
    if ((process = get_process_from_handle( req->handle, PROCESS_QUERY_INFORMATION )))
    {
        if (process->idle_event && process != current->process && process->queue != current->queue)
            reply->event = alloc_handle( current->process, process->idle_event,
                                         EVENT_ALL_ACCESS, 0 );
        release_object( process );
    }
}
