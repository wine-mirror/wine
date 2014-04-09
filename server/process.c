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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
#include "user.h"
#include "security.h"

/* process structure */

static struct list process_list = LIST_INIT(process_list);
static int running_processes, user_processes;
static struct event *shutdown_event;           /* signaled when shutdown starts */
static struct timeout_user *shutdown_timeout;  /* timeout for server shutdown */
static int shutdown_stage;  /* current stage in the shutdown process */

/* process operations */

static void process_dump( struct object *obj, int verbose );
static int process_signaled( struct object *obj, struct wait_queue_entry *entry );
static unsigned int process_map_access( struct object *obj, unsigned int access );
static void process_poll_event( struct fd *fd, int event );
static void process_destroy( struct object *obj );

static const struct object_ops process_ops =
{
    sizeof(struct process),      /* size */
    process_dump,                /* dump */
    no_get_type,                 /* get_type */
    add_queue,                   /* add_queue */
    remove_queue,                /* remove_queue */
    process_signaled,            /* signaled */
    no_satisfied,                /* satisfied */
    no_signal,                   /* signal */
    no_get_fd,                   /* get_fd */
    process_map_access,          /* map_access */
    default_get_sd,              /* get_sd */
    default_set_sd,              /* set_sd */
    no_lookup_name,              /* lookup_name */
    no_open_file,                /* open_file */
    no_close_handle,             /* close_handle */
    process_destroy              /* destroy */
};

static const struct fd_ops process_fd_ops =
{
    NULL,                        /* get_poll_events */
    process_poll_event,          /* poll_event */
    NULL,                        /* flush */
    NULL,                        /* get_fd_type */
    NULL,                        /* ioctl */
    NULL,                        /* queue_async */
    NULL,                        /* reselect_async */
    NULL                         /* cancel async */
};

/* process startup info */

struct startup_info
{
    struct object       obj;          /* object header */
    struct file        *exe_file;     /* file handle for main exe */
    struct process     *process;      /* created process */
    data_size_t         info_size;    /* size of startup info */
    data_size_t         data_size;    /* size of whole startup data */
    startup_info_t     *data;         /* data for startup info */
};

static void startup_info_dump( struct object *obj, int verbose );
static int startup_info_signaled( struct object *obj, struct wait_queue_entry *entry );
static void startup_info_destroy( struct object *obj );

static const struct object_ops startup_info_ops =
{
    sizeof(struct startup_info),   /* size */
    startup_info_dump,             /* dump */
    no_get_type,                   /* get_type */
    add_queue,                     /* add_queue */
    remove_queue,                  /* remove_queue */
    startup_info_signaled,         /* signaled */
    no_satisfied,                  /* satisfied */
    no_signal,                     /* signal */
    no_get_fd,                     /* get_fd */
    no_map_access,                 /* map_access */
    default_get_sd,                /* get_sd */
    default_set_sd,                /* set_sd */
    no_lookup_name,                /* lookup_name */
    no_open_file,                  /* open_file */
    no_close_handle,               /* close_handle */
    startup_info_destroy           /* destroy */
};


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

static void kill_all_processes(void);

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

/* callback for server shutdown */
static void server_shutdown_timeout( void *arg )
{
    shutdown_timeout = NULL;
    if (!running_processes)
    {
        close_master_socket( 0 );
        return;
    }
    switch(++shutdown_stage)
    {
    case 1:  /* signal system processes to exit */
        if (debug_level) fprintf( stderr, "wineserver: shutting down\n" );
        if (shutdown_event) set_event( shutdown_event );
        shutdown_timeout = add_timeout_user( 2 * -TICKS_PER_SEC, server_shutdown_timeout, NULL );
        close_master_socket( 4 * -TICKS_PER_SEC );
        break;
    case 2:  /* now forcibly kill all processes (but still wait for SIGKILL timeouts) */
        kill_all_processes();
        break;
    }
}

/* forced shutdown, used for wineserver -k */
void shutdown_master_socket(void)
{
    kill_all_processes();
    shutdown_stage = 2;
    if (shutdown_timeout)
    {
        remove_timeout_user( shutdown_timeout );
        shutdown_timeout = NULL;
    }
    close_master_socket( 2 * -TICKS_PER_SEC );  /* for SIGKILL timeouts */
}

/* final cleanup once we are sure a process is really dead */
static void process_died( struct process *process )
{
    if (debug_level) fprintf( stderr, "%04x: *process killed*\n", process->id );
    if (!process->is_system)
    {
        if (!--user_processes && !shutdown_stage && master_socket_timeout != TIMEOUT_INFINITE)
            shutdown_timeout = add_timeout_user( master_socket_timeout, server_shutdown_timeout, NULL );
    }
    release_object( process );
    if (!--running_processes && shutdown_stage) close_master_socket( 0 );
}

/* callback for process sigkill timeout */
static void process_sigkill( void *private )
{
    struct process *process = private;

    process->sigkill_timeout = NULL;
    kill( process->unix_pid, SIGKILL );
    process_died( process );
}

/* start the sigkill timer for a process upon exit */
static void start_sigkill_timer( struct process *process )
{
    grab_object( process );
    if (process->unix_pid != -1 && process->msg_fd)
        process->sigkill_timeout = add_timeout_user( -TICKS_PER_SEC, process_sigkill, process );
    else
        process_died( process );
}

/* create a new process and its main thread */
/* if the function fails the fd is closed */
struct thread *create_process( int fd, struct thread *parent_thread, int inherit_all )
{
    struct process *process;
    struct thread *thread = NULL;
    int request_pipe[2];

    if (!(process = alloc_object( &process_ops )))
    {
        close( fd );
        goto error;
    }
    process->parent          = NULL;
    process->debugger        = NULL;
    process->handles         = NULL;
    process->msg_fd          = NULL;
    process->sigkill_timeout = NULL;
    process->unix_pid        = -1;
    process->exit_code       = STILL_ACTIVE;
    process->running_threads = 0;
    process->priority        = PROCESS_PRIOCLASS_NORMAL;
    process->suspend         = 0;
    process->is_system       = 0;
    process->debug_children  = 0;
    process->is_terminating  = 0;
    process->console         = NULL;
    process->startup_state   = STARTUP_IN_PROGRESS;
    process->startup_info    = NULL;
    process->idle_event      = NULL;
    process->peb             = 0;
    process->ldt_copy        = 0;
    process->winstation      = 0;
    process->desktop         = 0;
    process->token           = NULL;
    process->trace_data      = 0;
    process->rawinput_mouse  = NULL;
    process->rawinput_kbd    = NULL;
    list_init( &process->thread_list );
    list_init( &process->locks );
    list_init( &process->classes );
    list_init( &process->dlls );
    list_init( &process->rawinput_devices );

    process->end_time = 0;
    list_add_tail( &process_list, &process->entry );

    if (!(process->id = process->group_id = alloc_ptid( process )))
    {
        close( fd );
        goto error;
    }
    if (!(process->msg_fd = create_anonymous_fd( &process_fd_ops, fd, &process->obj, 0 ))) goto error;

    /* create the handle table */
    if (!parent_thread)
    {
        process->handles = alloc_handle_table( process, 0 );
        process->token = token_create_admin();
        process->affinity = ~0;
    }
    else
    {
        struct process *parent = parent_thread->process;
        process->parent = (struct process *)grab_object( parent );
        process->handles = inherit_all ? copy_handle_table( process, parent )
                                       : alloc_handle_table( process, 0 );
        /* Note: for security reasons, starting a new process does not attempt
         * to use the current impersonation token for the new process */
        process->token = token_duplicate( parent->token, TRUE, 0 );
        process->affinity = parent->affinity;
    }
    if (!process->handles || !process->token) goto error;

    /* create the main thread */
    if (pipe( request_pipe ) == -1)
    {
        file_set_error();
        goto error;
    }
    if (send_client_fd( process, request_pipe[1], SERVER_PROTOCOL_VERSION ) == -1)
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
    if (!running_processes) close_master_socket( 0 );
    return NULL;
}

/* initialize the current process and fill in the request */
data_size_t init_process( struct thread *thread )
{
    struct process *process = thread->process;
    struct startup_info *info = process->startup_info;

    init_process_tracing( process );
    if (!info) return 0;
    return info->data_size;
}

/* destroy a process when its refcount is 0 */
static void process_destroy( struct object *obj )
{
    struct process *process = (struct process *)obj;
    assert( obj->ops == &process_ops );

    /* we can't have a thread remaining */
    assert( list_empty( &process->thread_list ));

    assert( !process->sigkill_timeout );  /* timeout should hold a reference to the process */

    close_process_handles( process );
    set_process_startup_state( process, STARTUP_ABORTED );
    if (process->console) release_object( process->console );
    if (process->parent) release_object( process->parent );
    if (process->msg_fd) release_object( process->msg_fd );
    list_remove( &process->entry );
    if (process->idle_event) release_object( process->idle_event );
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

static int process_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct process *process = (struct process *)obj;
    return !process->running_threads;
}

static unsigned int process_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= STANDARD_RIGHTS_READ | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;
    if (access & GENERIC_WRITE)   access |= STANDARD_RIGHTS_WRITE | PROCESS_SET_QUOTA | PROCESS_SET_INFORMATION | PROCESS_SUSPEND_RESUME |
                                            PROCESS_VM_WRITE | PROCESS_DUP_HANDLE | PROCESS_CREATE_PROCESS | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE;
    if (access & GENERIC_ALL)     access |= PROCESS_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static void process_poll_event( struct fd *fd, int event )
{
    struct process *process = get_fd_user( fd );
    assert( process->obj.ops == &process_ops );

    if (event & (POLLERR | POLLHUP)) kill_process( process, 0 );
    else if (event & POLLIN) receive_fd( process );
}

static void startup_info_destroy( struct object *obj )
{
    struct startup_info *info = (struct startup_info *)obj;
    assert( obj->ops == &startup_info_ops );
    free( info->data );
    if (info->exe_file) release_object( info->exe_file );
    if (info->process) release_object( info->process );
}

static void startup_info_dump( struct object *obj, int verbose )
{
    struct startup_info *info = (struct startup_info *)obj;
    assert( obj->ops == &startup_info_ops );

    fprintf( stderr, "Startup info in=%04x out=%04x err=%04x\n",
             info->data->hstdin, info->data->hstdout, info->data->hstderr );
}

static int startup_info_signaled( struct object *obj, struct wait_queue_entry *entry )
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
static inline struct process_dll *find_process_dll( struct process *process, mod_handle_t base )
{
    struct process_dll *dll;

    LIST_FOR_EACH_ENTRY( dll, &process->dlls, struct process_dll, entry )
    {
        if (dll->base == base) return dll;
    }
    return NULL;
}

/* add a dll to a process list */
static struct process_dll *process_load_dll( struct process *process, struct mapping *mapping,
                                             mod_handle_t base, const WCHAR *filename,
                                             data_size_t name_len )
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
        dll->mapping = NULL;
        dll->base = base;
        dll->filename = NULL;
        dll->namelen  = name_len;
        if (name_len && !(dll->filename = memdup( filename, name_len )))
        {
            free( dll );
            return NULL;
        }
        if (mapping) dll->mapping = grab_mapping_unless_removable( mapping );
        list_add_tail( &process->dlls, &dll->entry );
    }
    return dll;
}

/* remove a dll from a process list */
static void process_unload_dll( struct process *process, mod_handle_t base )
{
    struct process_dll *dll = find_process_dll( process, base );

    if (dll && (&dll->entry != list_head( &process->dlls )))  /* main exe can't be unloaded */
    {
        if (dll->mapping) release_object( dll->mapping );
        free( dll->filename );
        list_remove( &dll->entry );
        free( dll );
        generate_debug_event( current, UNLOAD_DLL_DEBUG_EVENT, &base );
    }
    else set_error( STATUS_INVALID_PARAMETER );
}

/* terminate a process with the given exit code */
static void terminate_process( struct process *process, struct thread *skip, int exit_code )
{
    struct thread *thread;

    grab_object( process );  /* make sure it doesn't get freed when threads die */
    process->is_terminating = 1;

restart:
    LIST_FOR_EACH_ENTRY( thread, &process->thread_list, struct thread, proc_entry )
    {
        if (exit_code) thread->exit_code = exit_code;
        if (thread == skip) continue;
        if (thread->state == TERMINATED) continue;
        kill_thread( thread, 1 );
        goto restart;
    }
    release_object( process );
}

/* kill all processes */
static void kill_all_processes(void)
{
    for (;;)
    {
        struct process *process;

        LIST_FOR_EACH_ENTRY( process, &process_list, struct process, entry )
        {
            if (process->running_threads) break;
        }
        if (&process->entry == &process_list) break;  /* no process found */
        terminate_process( process, NULL, 1 );
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
            if (process->console && console_get_renderer( process->console ) == renderer) break;
        }
        if (&process->entry == &process_list) break;  /* no process found */
        terminate_process( process, NULL, exit_code );
    }
}

/* a process has been killed (i.e. its last thread died) */
static void process_killed( struct process *process )
{
    struct list *ptr;

    assert( list_empty( &process->thread_list ));
    process->end_time = current_time;
    if (!process->is_system) close_process_desktop( process );
    process->winstation = 0;
    process->desktop = 0;
    close_process_handles( process );
    if (process->idle_event)
    {
        release_object( process->idle_event );
        process->idle_event = NULL;
    }

    /* close the console attached to this process, if any */
    free_console( process );

    while ((ptr = list_head( &process->rawinput_devices )))
    {
        struct rawinput_device_entry *entry = LIST_ENTRY( ptr, struct rawinput_device_entry, entry );
        list_remove( &entry->entry );
        free( entry );
    }
    while ((ptr = list_head( &process->dlls )))
    {
        struct process_dll *dll = LIST_ENTRY( ptr, struct process_dll, entry );
        if (dll->mapping) release_object( dll->mapping );
        free( dll->filename );
        list_remove( &dll->entry );
        free( dll );
    }
    destroy_process_classes( process );
    free_process_user_handles( process );
    remove_process_locks( process );
    set_process_startup_state( process, STARTUP_ABORTED );
    finish_process_tracing( process );
    start_sigkill_timer( process );
    wake_up( &process->obj, 0 );
}

/* add a thread to a process running threads list */
void add_process_thread( struct process *process, struct thread *thread )
{
    list_add_tail( &process->thread_list, &thread->proc_entry );
    if (!process->running_threads++)
    {
        running_processes++;
        if (!process->is_system)
        {
            if (!user_processes++ && shutdown_timeout)
            {
                remove_timeout_user( shutdown_timeout );
                shutdown_timeout = NULL;
            }
        }
    }
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
void kill_process( struct process *process, int violent_death )
{
    if (!violent_death && process->msg_fd)  /* normal termination on pipe close */
    {
        release_object( process->msg_fd );
        process->msg_fd = NULL;
    }

    if (process->sigkill_timeout)  /* already waiting for it to die */
    {
        remove_timeout_user( process->sigkill_timeout );
        process->sigkill_timeout = NULL;
        process_died( process );
        return;
    }

    if (violent_death) terminate_process( process, NULL, 1 );
    else
    {
        struct list *ptr;

        grab_object( process );  /* make sure it doesn't get freed when threads die */
        while ((ptr = list_head( &process->thread_list )))
        {
            struct thread *thread = LIST_ENTRY( ptr, struct thread, proc_entry );
            kill_thread( thread, 0 );
        }
        release_object( process );
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
        terminate_process( process, NULL, exit_code );
    }
}


/* trigger a breakpoint event in a given process */
void break_process( struct process *process )
{
    struct thread *thread;

    suspend_process( process );

    LIST_FOR_EACH_ENTRY( thread, &process->thread_list, struct thread, proc_entry )
    {
        if (thread->context)  /* inside an exception event already */
        {
            break_thread( thread );
            goto done;
        }
    }
    if ((thread = get_process_first_thread( process ))) thread->debug_break = 1;
    else set_error( STATUS_ACCESS_DENIED );
done:
    resume_process( process );
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

/* set the debugged flag in the process PEB */
int set_process_debug_flag( struct process *process, int flag )
{
    char data = (flag != 0);

    /* BeingDebugged flag is the byte at offset 2 in the PEB */
    return write_process_memory( process, process->peb + 2, 1, &data );
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
        ptr->unix_pid = process->unix_pid;
        grab_object( process );
        ptr++;
    }

    if (!(*count = ptr - snapshot))
    {
        free( snapshot );
        snapshot = NULL;
    }
    return snapshot;
}

/* create a new process */
DECL_HANDLER(new_process)
{
    struct startup_info *info;
    struct thread *thread;
    struct process *process;
    struct process *parent = current->process;
    int socket_fd = thread_get_inflight_fd( current, req->socket_fd );

    if (socket_fd == -1)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    if (fcntl( socket_fd, F_SETFL, O_NONBLOCK ) == -1)
    {
        set_error( STATUS_INVALID_HANDLE );
        close( socket_fd );
        return;
    }
    if (shutdown_stage)
    {
        set_error( STATUS_SHUTDOWN_IN_PROGRESS );
        close( socket_fd );
        return;
    }
    if (!is_cpu_supported( req->cpu ))
    {
        close( socket_fd );
        return;
    }

    if (!req->info_size)  /* create an orphaned process */
    {
        create_process( socket_fd, NULL, 0 );
        return;
    }

    /* build the startup info for a new process */
    if (!(info = alloc_object( &startup_info_ops ))) return;
    info->exe_file = NULL;
    info->process  = NULL;
    info->data     = NULL;

    if (req->exe_file &&
        !(info->exe_file = get_file_obj( current->process, req->exe_file, FILE_READ_DATA )))
        goto done;

    info->data_size = get_req_data_size();
    info->info_size = min( req->info_size, info->data_size );

    if (req->info_size < sizeof(*info->data))
    {
        /* make sure we have a full startup_info_t structure */
        data_size_t env_size = info->data_size - info->info_size;
        data_size_t info_size = min( req->info_size, FIELD_OFFSET( startup_info_t, curdir_len ));

        if (!(info->data = mem_alloc( sizeof(*info->data) + env_size ))) goto done;
        memcpy( info->data, get_req_data(), info_size );
        memset( (char *)info->data + info_size, 0, sizeof(*info->data) - info_size );
        memcpy( info->data + 1, (const char *)get_req_data() + req->info_size, env_size );
        info->info_size = sizeof(startup_info_t);
        info->data_size = info->info_size + env_size;
    }
    else
    {
        data_size_t pos = sizeof(*info->data);

        if (!(info->data = memdup( get_req_data(), info->data_size ))) goto done;
#define FIXUP_LEN(len) do { (len) = min( (len), info->info_size - pos ); pos += (len); } while(0)
        FIXUP_LEN( info->data->curdir_len );
        FIXUP_LEN( info->data->dllpath_len );
        FIXUP_LEN( info->data->imagepath_len );
        FIXUP_LEN( info->data->cmdline_len );
        FIXUP_LEN( info->data->title_len );
        FIXUP_LEN( info->data->desktop_len );
        FIXUP_LEN( info->data->shellinfo_len );
        FIXUP_LEN( info->data->runtime_len );
#undef FIXUP_LEN
    }

    if (!(thread = create_process( socket_fd, current, req->inherit_all ))) goto done;
    process = thread->process;
    process->debug_children = (req->create_flags & DEBUG_PROCESS)
        && !(req->create_flags & DEBUG_ONLY_THIS_PROCESS);
    process->startup_info = (struct startup_info *)grab_object( info );

    /* connect to the window station */
    connect_process_winstation( process, current );

    /* thread will be actually suspended in init_done */
    if (req->create_flags & CREATE_SUSPENDED) thread->suspend++;

    /* set the process console */
    if (!(req->create_flags & (DETACHED_PROCESS | CREATE_NEW_CONSOLE)))
    {
        /* FIXME: some better error checking should be done...
         * like if hConOut and hConIn are console handles, then they should be on the same
         * physical console
         */
        inherit_console( current, process, req->inherit_all ? info->data->hstdin : 0 );
    }

    if (!req->inherit_all && !(req->create_flags & CREATE_NEW_CONSOLE))
    {
        info->data->hstdin  = duplicate_handle( parent, info->data->hstdin, process,
                                                0, OBJ_INHERIT, DUPLICATE_SAME_ACCESS );
        info->data->hstdout = duplicate_handle( parent, info->data->hstdout, process,
                                                0, OBJ_INHERIT, DUPLICATE_SAME_ACCESS );
        info->data->hstderr = duplicate_handle( parent, info->data->hstderr, process,
                                                0, OBJ_INHERIT, DUPLICATE_SAME_ACCESS );
        /* some handles above may have been invalid; this is not an error */
        if (get_error() == STATUS_INVALID_HANDLE ||
            get_error() == STATUS_OBJECT_TYPE_MISMATCH) clear_error();
    }

    /* attach to the debugger if requested */
    if (req->create_flags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS))
        set_process_debugger( process, current );
    else if (parent->debugger && parent->debug_children)
        set_process_debugger( process, parent->debugger );

    if (!(req->create_flags & CREATE_NEW_PROCESS_GROUP))
        process->group_id = parent->group_id;

    info->process = (struct process *)grab_object( process );
    reply->info = alloc_handle( current->process, info, SYNCHRONIZE, 0 );
    reply->pid = get_process_id( process );
    reply->tid = get_thread_id( thread );
    reply->phandle = alloc_handle( parent, process, req->process_access, req->process_attr );
    reply->thandle = alloc_handle( parent, thread, req->thread_access, req->thread_attr );

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
        reply->success = is_process_init_done( info->process );
        reply->exit_code = info->process->exit_code;
        release_object( info );
    }
}

/* Retrieve the new process startup info */
DECL_HANDLER(get_startup_info)
{
    struct process *process = current->process;
    struct startup_info *info = process->startup_info;
    data_size_t size;

    if (!info) return;

    if (info->exe_file &&
        !(reply->exe_file = alloc_handle( process, info->exe_file, GENERIC_READ, 0 ))) return;

    /* we return the data directly without making a copy so this can only be called once */
    reply->info_size = info->info_size;
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
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    if (!(dll = find_process_dll( process, req->module )))
    {
        set_error( STATUS_DLL_NOT_FOUND );
        return;
    }

    /* main exe is the first in the dll list */
    list_remove( &dll->entry );
    list_add_head( &process->dlls, &dll->entry );

    process->ldt_copy = req->ldt_copy;
    process->start_time = current_time;

    generate_startup_debug_events( process, req->entry );
    set_process_startup_state( process, STARTUP_DONE );

    if (req->gui) process->idle_event = create_event( NULL, NULL, 0, 1, 0, NULL );
    stop_thread_if_suspended( current );
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

    if (req->handle)
    {
        process = get_process_from_handle( req->handle, PROCESS_TERMINATE );
        if (!process) return;
    }
    else process = (struct process *)grab_object( current->process );

    reply->self = (current->process == process);
    terminate_process( process, current, req->exit_code );
    release_object( process );
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
        reply->start_time       = process->start_time;
        reply->end_time         = process->end_time;
        reply->cpu              = process->cpu;
        reply->debugger_present = !!process->debugger;
        release_object( process );
    }
}

static void set_process_affinity( struct process *process, affinity_t affinity )
{
    struct thread *thread;

    if (!process->running_threads)
    {
        set_error( STATUS_PROCESS_IS_TERMINATING );
        return;
    }

    process->affinity = affinity;

    LIST_FOR_EACH_ENTRY( thread, &process->thread_list, struct thread, proc_entry )
    {
        set_thread_affinity( thread, affinity );
    }
}

/* set information about a process */
DECL_HANDLER(set_process_info)
{
    struct process *process;

    if ((process = get_process_from_handle( req->handle, PROCESS_SET_INFORMATION )))
    {
        if (req->mask & SET_PROCESS_INFO_PRIORITY) process->priority = req->priority;
        if (req->mask & SET_PROCESS_INFO_AFFINITY) set_process_affinity( process, req->affinity );
        release_object( process );
    }
}

/* read data from a process address space */
DECL_HANDLER(read_process_memory)
{
    struct process *process;
    data_size_t len = get_reply_max_size();

    if (!(process = get_process_from_handle( req->handle, PROCESS_VM_READ ))) return;

    if (len)
    {
        char *buffer = mem_alloc( len );
        if (buffer)
        {
            if (read_process_memory( process, req->addr, len, buffer ))
                set_reply_data_ptr( buffer, len );
            else
                free( buffer );
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
        data_size_t len = get_req_data_size();
        if (len) write_process_memory( process, req->addr, len, get_req_data() );
        else set_error( STATUS_INVALID_PARAMETER );
        release_object( process );
    }
}

/* notify the server that a dll has been loaded */
DECL_HANDLER(load_dll)
{
    struct process_dll *dll;
    struct mapping *mapping = NULL;

    if (req->mapping && !(mapping = get_mapping_obj( current->process, req->mapping, SECTION_QUERY )))
        return;

    if ((dll = process_load_dll( current->process, mapping, req->base,
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
    if (mapping) release_object( mapping );
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

        if (req->base_address)
            dll = find_process_dll( process, req->base_address );
        else /* NULL means main module */
            dll = list_head( &process->dlls ) ?
                LIST_ENTRY(list_head( &process->dlls ), struct process_dll, entry) : NULL;

        if (dll)
        {
            reply->size = dll->size;
            reply->entry_point = 0; /* FIXME */
            reply->filename_len = dll->namelen;
            if (dll->filename)
            {
                if (dll->namelen <= get_reply_max_size())
                    set_reply_data( dll->filename, dll->namelen );
                else
                    set_error( STATUS_BUFFER_TOO_SMALL );
            }
        }
        else
            set_error( STATUS_DLL_NOT_FOUND );

        release_object( process );
    }
}

/* retrieve the process idle event */
DECL_HANDLER(get_process_idle_event)
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

/* make the current process a system process */
DECL_HANDLER(make_process_system)
{
    struct process *process = current->process;

    if (!shutdown_event)
    {
        if (!(shutdown_event = create_event( NULL, NULL, 0, 1, 0, NULL ))) return;
        make_object_static( (struct object *)shutdown_event );
    }

    if (!(reply->event = alloc_handle( current->process, shutdown_event, SYNCHRONIZE, 0 )))
        return;

    if (!process->is_system)
    {
        process->is_system = 1;
        close_process_desktop( process );
        if (!--user_processes && !shutdown_stage && master_socket_timeout != TIMEOUT_INFINITE)
            shutdown_timeout = add_timeout_user( master_socket_timeout, server_shutdown_timeout, NULL );
    }
}
