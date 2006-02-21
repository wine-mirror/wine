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
#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "file.h"
#include "handle.h"
#include "process.h"
#include "thread.h"
#include "request.h"
#include "console.h"
#include "user.h"
#include "security.h"

/* process structure */

static struct list process_list = LIST_INIT(process_list);
static int running_processes;

/* process operations */

static void process_dump( struct object *obj, int verbose );
static int process_signaled( struct object *obj, struct thread *thread );
static unsigned int process_map_access( struct object *obj, unsigned int access );
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
    no_signal,                   /* signal */
    no_get_fd,                   /* get_fd */
    process_map_access,          /* map_access */
    no_lookup_name,              /* lookup_name */
    no_close_handle,             /* close_handle */
    process_destroy              /* destroy */
};

static const struct fd_ops process_fd_ops =
{
    NULL,                        /* get_poll_events */
    process_poll_event,          /* poll_event */
    no_flush,                    /* flush */
    no_get_file_info,            /* get_file_info */
    no_queue_async,              /* queue_async */
    no_cancel_async              /* cancel async */
};

/* process startup info */

struct startup_info
{
    struct object       obj;          /* object header */
    struct list         entry;        /* entry in list of startup infos */
    int                 inherit_all;  /* inherit all handles from parent */
    unsigned int        create_flags; /* creation flags */
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
    no_signal,                     /* signal */
    no_get_fd,                     /* get_fd */
    no_map_access,                 /* map_access */
    no_lookup_name,                /* lookup_name */
    no_close_handle,               /* close_handle */
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

/* return the main thread of the process */
struct thread *get_process_first_thread( struct process *process )
{
    struct list *ptr = list_head( &process->thread_list );
    if (!ptr) return NULL;
    return LIST_ENTRY( ptr, struct thread, proc_entry );
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

/* create a new process and its main thread */
struct thread *create_process( int fd )
{
    struct process *process;
    struct thread *thread = NULL;
    int request_pipe[2];

    if (!(process = alloc_object( &process_ops ))) goto error;
    process->parent          = NULL;
    process->debugger        = NULL;
    process->handles         = NULL;
    process->msg_fd          = NULL;
    process->exit_code       = STILL_ACTIVE;
    process->running_threads = 0;
    process->priority        = PROCESS_PRIOCLASS_NORMAL;
    process->affinity        = 1;
    process->suspend         = 0;
    process->create_flags    = 0;
    process->console         = NULL;
    process->startup_state   = STARTUP_IN_PROGRESS;
    process->startup_info    = NULL;
    process->idle_event      = NULL;
    process->queue           = NULL;
    process->peb             = NULL;
    process->ldt_copy        = NULL;
    process->winstation      = 0;
    process->desktop         = 0;
    process->token           = token_create_admin();
    list_init( &process->thread_list );
    list_init( &process->locks );
    list_init( &process->classes );
    list_init( &process->dlls );

    gettimeofday( &process->start_time, NULL );
    list_add_head( &process_list, &process->entry );

    if (!(process->id = process->group_id = alloc_ptid( process ))) goto error;
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
size_t init_process( struct thread *thread )
{
    struct process *process = thread->process;
    struct thread *parent_thread = NULL;
    struct process *parent = NULL;
    struct startup_info *info;

    if (process->startup_info) return process->startup_info->data_size;  /* already initialized */

    if ((info = find_startup_info( thread->unix_pid )))
    {
        if (info->thread) return info->data_size;  /* already initialized */

        info->thread  = (struct thread *)grab_object( thread );
        info->process = (struct process *)grab_object( process );
        process->startup_info = (struct startup_info *)grab_object( info );

        parent_thread = info->owner;
        parent = parent_thread->process;
        process->parent = (struct process *)grab_object( parent );

        /* set the process flags */
        process->create_flags = info->create_flags;

        if (info->inherit_all) process->handles = copy_handle_table( process, parent );
    }

    /* create the handle table */
    if (!process->handles) process->handles = alloc_handle_table( process, 0 );
    if (!process->handles)
    {
        fatal_protocol_error( thread, "Failed to allocate handle table\n" );
        return 0;
    }

    /* connect to the window station and desktop */
    connect_process_winstation( process, NULL );
    connect_process_desktop( process, NULL );
    thread->desktop = process->desktop;

    if (!info) return 0;

    /* thread will be actually suspended in init_done */
    if (info->create_flags & CREATE_SUSPENDED) thread->suspend++;

    /* set the process console */
    if (!(info->create_flags & (DETACHED_PROCESS | CREATE_NEW_CONSOLE)))
    {
        /* FIXME: some better error checking should be done...
         * like if hConOut and hConIn are console handles, then they should be on the same
         * physical console
         */
        inherit_console( parent_thread, process, info->inherit_all ? info->hstdin : 0 );
    }

    /* attach to the debugger if requested */
    if (process->create_flags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS))
        set_process_debugger( process, parent_thread );
    else if (parent->debugger && !(parent->create_flags & DEBUG_ONLY_THIS_PROCESS))
        set_process_debugger( process, parent->debugger );

    if (!(process->create_flags & CREATE_NEW_PROCESS_GROUP))
        process->group_id = parent->group_id;

    return info->data_size;
}

/* destroy a process when its refcount is 0 */
static void process_destroy( struct object *obj )
{
    struct process *process = (struct process *)obj;
    assert( obj->ops == &process_ops );

    /* we can't have a thread remaining */
    assert( list_empty( &process->thread_list ));

    set_process_startup_state( process, STARTUP_ABORTED );
    if (process->console) release_object( process->console );
    if (process->parent) release_object( process->parent );
    if (process->msg_fd) release_object( process->msg_fd );
    list_remove( &process->entry );
    if (process->idle_event) release_object( process->idle_event );
    if (process->queue) release_object( process->queue );
    if (process->id) free_ptid( process->id );
    if (process->token) release_object( process->token );
}

/* dump a process on stdout for debugging purposes */
static void process_dump( struct object *obj, int verbose )
{
    struct process *process = (struct process *)obj;
    assert( obj->ops == &process_ops );

    fprintf( stderr, "Process id=%04x handles=%p\n", process->id, process->handles );
}

static int process_signaled( struct object *obj, struct thread *thread )
{
    struct process *process = (struct process *)obj;
    return !process->running_threads;
}

static unsigned int process_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= STANDARD_RIGHTS_READ | SYNCHRONIZE;
    if (access & GENERIC_WRITE)   access |= STANDARD_RIGHTS_WRITE | SYNCHRONIZE;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE;
    if (access & GENERIC_ALL)     access |= PROCESS_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
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
    return info->process && info->process->startup_state != STARTUP_IN_PROGRESS;
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

/* find a dll from its base address */
static inline struct process_dll *find_process_dll( struct process *process, void *base )
{
    struct process_dll *dll;

    LIST_FOR_EACH_ENTRY( dll, &process->dlls, struct process_dll, entry )
    {
        if (dll->base == base) return dll;
    }
    return NULL;
}

/* add a dll to a process list */
static struct process_dll *process_load_dll( struct process *process, struct file *file,
                                             void *base, const WCHAR *filename, size_t name_len )
{
    struct process_dll *dll;

    /* make sure we don't already have one with the same base address */
    if (find_process_dll( process, base ))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }

    if ((dll = mem_alloc( sizeof(*dll) )))
    {
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
        list_add_tail( &process->dlls, &dll->entry );
    }
    return dll;
}

/* remove a dll from a process list */
static void process_unload_dll( struct process *process, void *base )
{
    struct process_dll *dll = find_process_dll( process, base );

    if (dll && (&dll->entry != list_head( &process->dlls )))  /* main exe can't be unloaded */
    {
        if (dll->file) release_object( dll->file );
        if (dll->filename) free( dll->filename );
        list_remove( &dll->entry );
        free( dll );
        generate_debug_event( current, UNLOAD_DLL_DEBUG_EVENT, base );
    }
    else set_error( STATUS_INVALID_PARAMETER );
}

/* kill all processes */
void kill_all_processes( struct process *skip, int exit_code )
{
    for (;;)
    {
        struct process *process;

        LIST_FOR_EACH_ENTRY( process, &process_list, struct process, entry )
        {
            if (process == skip) continue;
            if (process->running_threads) break;
        }
        if (&process->entry == &process_list) break;  /* no process found */
        kill_process( process, NULL, exit_code );
    }
}

/* kill all processes being attached to a console renderer */
void kill_console_processes( struct thread *renderer, int exit_code )
{
    for (;;)  /* restart from the beginning of the list every time */
    {
        struct process *process;

        /* find the first process being attached to 'renderer' and still running */
        LIST_FOR_EACH_ENTRY( process, &process_list, struct process, entry )
        {
            if (process == renderer->process) continue;
            if (!process->running_threads) continue;
            if (process->console && process->console->renderer == renderer) break;
        }
        if (&process->entry == &process_list) break;  /* no process found */
        kill_process( process, NULL, exit_code );
    }
}

/* a process has been killed (i.e. its last thread died) */
static void process_killed( struct process *process )
{
    struct handle_table *handles;
    struct list *ptr;

    assert( list_empty( &process->thread_list ));
    gettimeofday( &process->end_time, NULL );
    handles = process->handles;
    process->handles = NULL;
    if (handles) release_object( handles );

    /* close the console attached to this process, if any */
    free_console( process );

    while ((ptr = list_head( &process->dlls )))
    {
        struct process_dll *dll = LIST_ENTRY( ptr, struct process_dll, entry );
        if (dll->file) release_object( dll->file );
        if (dll->filename) free( dll->filename );
        list_remove( &dll->entry );
        free( dll );
    }
    destroy_process_classes( process );
    remove_process_locks( process );
    set_process_startup_state( process, STARTUP_ABORTED );
    wake_up( &process->obj, 0 );
    if (!--running_processes) close_master_socket();
}

/* add a thread to a process running threads list */
void add_process_thread( struct process *process, struct thread *thread )
{
    list_add_head( &process->thread_list, &thread->proc_entry );
    if (!process->running_threads++) running_processes++;
    grab_object( thread );
}

/* remove a thread from a process running threads list */
void remove_process_thread( struct process *process, struct thread *thread )
{
    assert( process->running_threads > 0 );
    assert( !list_empty( &process->thread_list ));

    list_remove( &thread->proc_entry );

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
        struct list *ptr, *next;

        LIST_FOR_EACH_SAFE( ptr, next, &process->thread_list )
        {
            struct thread *thread = LIST_ENTRY( ptr, struct thread, proc_entry );
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
        struct list *ptr, *next;

        LIST_FOR_EACH_SAFE( ptr, next, &process->thread_list )
        {
            struct thread *thread = LIST_ENTRY( ptr, struct thread, proc_entry );
            if (!thread->suspend) wake_thread( thread );
        }
    }
}

/* kill a process on the spot */
void kill_process( struct process *process, struct thread *skip, int exit_code )
{
    struct list *ptr, *next;

    LIST_FOR_EACH_SAFE( ptr, next, &process->thread_list )
    {
        struct thread *thread = LIST_ENTRY( ptr, struct thread, proc_entry );

        thread->exit_code = exit_code;
        if (thread != skip) kill_thread( thread, 1 );
    }
}

/* kill all processes being debugged by a given thread */
void kill_debugged_processes( struct thread *debugger, int exit_code )
{
    for (;;)  /* restart from the beginning of the list every time */
    {
        struct process *process;

        /* find the first process being debugged by 'debugger' and still running */
        LIST_FOR_EACH_ENTRY( process, &process_list, struct process, entry )
        {
            if (!process->running_threads) continue;
            if (process->debugger == debugger) break;
        }
        if (&process->entry == &process_list) break;  /* no process found */
        process->debugger = NULL;
        kill_process( process, NULL, exit_code );
    }
}


/* detach a debugger from all its debuggees */
void detach_debugged_processes( struct thread *debugger )
{
    struct process *process;

    LIST_FOR_EACH_ENTRY( process, &process_list, struct process, entry )
    {
        if (process->debugger == debugger && process->running_threads)
        {
            debugger_detach( process, debugger );
        }
    }
}


void enum_processes( int (*cb)(struct process*, void*), void *user )
{
    struct list *ptr, *next;

    LIST_FOR_EACH_SAFE( ptr, next, &process_list )
    {
        struct process *process = LIST_ENTRY( ptr, struct process, entry );
        if ((cb)(process, user)) break;
    }
}


/* read data from a process memory space */
/* len is the total size (in ints) */
static int read_process_memory( struct process *process, const int *addr, size_t len, int *dest )
{
    struct thread *thread = get_process_first_thread( process );

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
static int write_process_memory( struct process *process, int *addr, size_t len,
                                 unsigned int first_mask, unsigned int last_mask, const int *src )
{
    struct thread *thread = get_process_first_thread( process );
    int ret = 0;

    assert( !((unsigned int)addr % sizeof(int) ));  /* address must be aligned */

    if (!thread)  /* process is dead */
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
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
        ret = 1;

    done:
        resume_after_ptrace( thread );
    }
    return ret;
}

/* set the debugged flag in the process PEB */
int set_process_debug_flag( struct process *process, int flag )
{
    int mask = 0, data = 0;

    /* BeingDebugged flag is the byte at offset 2 in the PEB */
    memset( (char *)&mask + 2, 0xff, 1 );
    memset( (char *)&data + 2, flag, 1 );
    return write_process_memory( process, process->peb, 1, mask, mask, &data );
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
    LIST_FOR_EACH_ENTRY( process, &process_list, struct process, entry )
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

    LIST_FOR_EACH_ENTRY( dll, &process->dlls, struct process_dll, entry ) total++;
    if (!(snapshot = mem_alloc( sizeof(*snapshot) * total ))) return NULL;

    ptr = snapshot;
    LIST_FOR_EACH_ENTRY( dll, &process->dlls, struct process_dll, entry )
    {
        ptr->base     = dll->base;
        ptr->size     = dll->size;
        ptr->namelen  = dll->namelen;
        ptr->filename = memdup( dll->filename, dll->namelen );
        ptr++;
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
        !(info->exe_file = get_file_obj( current->process, req->exe_file, FILE_READ_DATA )))
        goto done;

    if (!(info->data = memdup( get_req_data(), info->data_size ))) goto done;
    reply->info = alloc_handle( current->process, info, SYNCHRONIZE, 0 );

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
                                       req->process_access, req->process_attr );
        reply->thandle = alloc_handle( current->process, info->thread,
                                       req->thread_access, req->thread_attr );
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
    struct process *process = current->process;
    struct startup_info *info = process->startup_info;
    size_t size;

    if (!info) return;

    if (info->exe_file &&
        !(reply->exe_file = alloc_handle( process, info->exe_file, GENERIC_READ, 0 ))) return;

    if (!info->inherit_all && !(info->create_flags & CREATE_NEW_CONSOLE))
    {
        struct process *parent_process = info->owner->process;
        reply->hstdin  = duplicate_handle( parent_process, info->hstdin, process,
                                           0, OBJ_INHERIT, DUPLICATE_SAME_ACCESS );
        reply->hstdout = duplicate_handle( parent_process, info->hstdout, process,
                                           0, OBJ_INHERIT, DUPLICATE_SAME_ACCESS );
        reply->hstderr = duplicate_handle( parent_process, info->hstderr, process,
                                           0, OBJ_INHERIT, DUPLICATE_SAME_ACCESS );
        /* some handles above may have been invalid; this is not an error */
        if (get_error() == STATUS_INVALID_HANDLE ||
            get_error() == STATUS_OBJECT_TYPE_MISMATCH) clear_error();
    }
    else
    {
        reply->hstdin  = info->hstdin;
        reply->hstdout = info->hstdout;
        reply->hstderr = info->hstderr;
    }

    /* we return the data directly without making a copy so this can only be called once */
    size = info->data_size;
    if (size > get_reply_max_size()) size = get_reply_max_size();
    set_reply_data_ptr( info->data, size );
    info->data = NULL;
    info->data_size = 0;
}

/* signal the end of the process initialization */
DECL_HANDLER(init_process_done)
{
    struct process_dll *dll;
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

    /* check if main exe has been registered as a dll already */
    if (!(dll = find_process_dll( process, req->module )))
    {
        if (!(dll = process_load_dll( process, NULL, req->module,
                                      get_req_data(), get_req_data_size() ))) return;
        dll->size       = req->module_size;
        dll->dbg_offset = 0;
        dll->dbg_size   = 0;
        dll->name       = req->name;
    }

    /* main exe is the first in the dll list */
    list_remove( &dll->entry );
    list_add_head( &process->dlls, &dll->entry );

    generate_startup_debug_events( process, req->entry );
    set_process_startup_state( process, STARTUP_DONE );

    if (req->gui) process->idle_event = create_event( NULL, NULL, 0, 1, 0 );
    if (current->suspend + process->suspend > 0) stop_thread( current );
    if (process->debugger) set_process_debug_flag( process, 1 );
}

/* open a handle to a process */
DECL_HANDLER(open_process)
{
    struct process *process = get_process_from_id( req->pid );
    reply->handle = 0;
    if (process)
    {
        reply->handle = alloc_handle( current->process, process, req->access, req->attributes );
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
        reply->pid              = get_process_id( process );
        reply->ppid             = process->parent ? get_process_id( process->parent ) : 0;
        reply->exit_code        = process->exit_code;
        reply->priority         = process->priority;
        reply->affinity         = process->affinity;
        reply->peb              = process->peb;
        release_object( process );
    }
}

/* set information about a process */
DECL_HANDLER(set_process_info)
{
    struct process *process;

    if ((process = get_process_from_handle( req->handle, PROCESS_SET_INFORMATION )))
    {
        if (req->mask & SET_PROCESS_INFO_PRIORITY) process->priority = req->priority;
        if (req->mask & SET_PROCESS_INFO_AFFINITY)
        {
            if (req->affinity != 1) set_error( STATUS_INVALID_PARAMETER );
            else process->affinity = req->affinity;
        }
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

    if (req->handle && !(file = get_file_obj( current->process, req->handle, FILE_READ_DATA )))
        return;

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
        struct process_dll *dll = find_process_dll( process, req->base_address );

        if (dll)
        {
            reply->size = dll->size;
            reply->entry_point = NULL; /* FIXME */
            if (dll->filename)
            {
                size_t len = min( dll->namelen, get_reply_max_size() );
                set_reply_data( dll->filename, len );
            }
        }
        else
            set_error( STATUS_DLL_NOT_FOUND );

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
        if (process->idle_event && process != current->process)
            reply->event = alloc_handle( current->process, process->idle_event,
                                         EVENT_ALL_ACCESS, 0 );
        release_object( process );
    }
}
