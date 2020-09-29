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
#include <errno.h>
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
static struct object_type *process_get_type( struct object *obj );
static int process_signaled( struct object *obj, struct wait_queue_entry *entry );
static unsigned int process_map_access( struct object *obj, unsigned int access );
static struct security_descriptor *process_get_sd( struct object *obj );
static void process_poll_event( struct fd *fd, int event );
static struct list *process_get_kernel_obj_list( struct object *obj );
static void process_destroy( struct object *obj );
static void terminate_process( struct process *process, struct thread *skip, int exit_code );

static const struct object_ops process_ops =
{
    sizeof(struct process),      /* size */
    process_dump,                /* dump */
    process_get_type,            /* get_type */
    add_queue,                   /* add_queue */
    remove_queue,                /* remove_queue */
    process_signaled,            /* signaled */
    no_satisfied,                /* satisfied */
    no_signal,                   /* signal */
    no_get_fd,                   /* get_fd */
    process_map_access,          /* map_access */
    process_get_sd,              /* get_sd */
    default_set_sd,              /* set_sd */
    no_get_full_name,            /* get_full_name */
    no_lookup_name,              /* lookup_name */
    no_link_name,                /* link_name */
    NULL,                        /* unlink_name */
    no_open_file,                /* open_file */
    process_get_kernel_obj_list, /* get_kernel_obj_list */
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
    no_get_full_name,              /* get_full_name */
    no_lookup_name,                /* lookup_name */
    no_link_name,                  /* link_name */
    NULL,                          /* unlink_name */
    no_open_file,                  /* open_file */
    no_kernel_obj_list,            /* get_kernel_obj_list */
    no_close_handle,               /* close_handle */
    startup_info_destroy           /* destroy */
};

/* job object */

static void job_dump( struct object *obj, int verbose );
static struct object_type *job_get_type( struct object *obj );
static int job_signaled( struct object *obj, struct wait_queue_entry *entry );
static unsigned int job_map_access( struct object *obj, unsigned int access );
static int job_close_handle( struct object *obj, struct process *process, obj_handle_t handle );
static void job_destroy( struct object *obj );

struct job
{
    struct object obj;             /* object header */
    struct list process_list;      /* list of all processes */
    int num_processes;             /* count of running processes */
    int total_processes;           /* count of processes which have been assigned */
    unsigned int limit_flags;      /* limit flags */
    int terminating;               /* job is terminating */
    int signaled;                  /* job is signaled */
    struct completion *completion_port; /* associated completion port */
    apc_param_t completion_key;    /* key to send with completion messages */
};

static const struct object_ops job_ops =
{
    sizeof(struct job),            /* size */
    job_dump,                      /* dump */
    job_get_type,                  /* get_type */
    add_queue,                     /* add_queue */
    remove_queue,                  /* remove_queue */
    job_signaled,                  /* signaled */
    no_satisfied,                  /* satisfied */
    no_signal,                     /* signal */
    no_get_fd,                     /* get_fd */
    job_map_access,                /* map_access */
    default_get_sd,                /* get_sd */
    default_set_sd,                /* set_sd */
    default_get_full_name,         /* get_full_name */
    no_lookup_name,                /* lookup_name */
    directory_link_name,           /* link_name */
    default_unlink_name,           /* unlink_name */
    no_open_file,                  /* open_file */
    no_kernel_obj_list,            /* get_kernel_obj_list */
    job_close_handle,              /* close_handle */
    job_destroy                    /* destroy */
};

static struct job *create_job_object( struct object *root, const struct unicode_str *name,
                                      unsigned int attr, const struct security_descriptor *sd )
{
    struct job *job;

    if ((job = create_named_object( root, &job_ops, name, attr, sd )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            /* initialize it if it didn't already exist */
            list_init( &job->process_list );
            job->num_processes = 0;
            job->total_processes = 0;
            job->limit_flags = 0;
            job->terminating = 0;
            job->signaled = 0;
            job->completion_port = NULL;
            job->completion_key = 0;
        }
    }
    return job;
}

static struct job *get_job_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct job *)get_handle_obj( process, handle, access, &job_ops );
}

static struct object_type *job_get_type( struct object *obj )
{
    static const WCHAR name[] = {'J','o','b'};
    static const struct unicode_str str = { name, sizeof(name) };
    return get_object_type( &str );
};

static unsigned int job_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= STANDARD_RIGHTS_READ;
    if (access & GENERIC_WRITE)   access |= STANDARD_RIGHTS_WRITE;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE;
    if (access & GENERIC_ALL)     access |= JOB_OBJECT_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static void add_job_completion( struct job *job, apc_param_t msg, apc_param_t pid )
{
    if (job->completion_port)
        add_completion( job->completion_port, job->completion_key, pid, STATUS_SUCCESS, msg );
}

static void add_job_process( struct job *job, struct process *process )
{
    process->job = (struct job *)grab_object( job );
    list_add_tail( &job->process_list, &process->job_entry );
    job->num_processes++;
    job->total_processes++;

    add_job_completion( job, JOB_OBJECT_MSG_NEW_PROCESS, get_process_id(process) );
}

/* called when a process has terminated, allow one additional process */
static void release_job_process( struct process *process )
{
    struct job *job = process->job;

    if (!job) return;

    assert( job->num_processes );
    job->num_processes--;

    if (!job->terminating)
        add_job_completion( job, JOB_OBJECT_MSG_EXIT_PROCESS, get_process_id(process) );

    if (!job->num_processes)
        add_job_completion( job, JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO, 0 );
}

static void terminate_job( struct job *job, int exit_code )
{
    /* don't report completion events for terminated processes */
    job->terminating = 1;

    for (;;)  /* restart from the beginning of the list every time */
    {
        struct process *process;

        /* find the first process associated with this job and still running */
        LIST_FOR_EACH_ENTRY( process, &job->process_list, struct process, job_entry )
        {
            if (process->running_threads) break;
        }
        if (&process->job_entry == &job->process_list) break;  /* no process found */
        assert( process->job == job );
        terminate_process( process, NULL, exit_code );
    }

    job->terminating = 0;
    job->signaled = 1;
    wake_up( &job->obj, 0 );
}

static int job_close_handle( struct object *obj, struct process *process, obj_handle_t handle )
{
    struct job *job = (struct job *)obj;
    assert( obj->ops == &job_ops );

    if (obj->handle_count == 1)  /* last handle */
    {
        if (job->limit_flags & JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE)
            terminate_job( job, 0 );
    }
    return 1;
}

static void job_destroy( struct object *obj )
{
    struct job *job = (struct job *)obj;
    assert( obj->ops == &job_ops );

    assert( !job->num_processes );
    assert( list_empty(&job->process_list) );

    if (job->completion_port) release_object( job->completion_port );
}

static void job_dump( struct object *obj, int verbose )
{
    struct job *job = (struct job *)obj;
    assert( obj->ops == &job_ops );
    fprintf( stderr, "Job processes=%d\n", list_count(&job->process_list) );
}

static int job_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct job *job = (struct job *)obj;
    return job->signaled;
}

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
static unsigned int num_free_ptids;         /* number of free ptids */

static void kill_all_processes(void);

#define PTID_OFFSET 8  /* offset for first ptid value */

static unsigned int index_from_ptid(unsigned int id) { return id / 4; }
static unsigned int ptid_from_index(unsigned int index) { return index * 4; }

/* allocate a new process or thread id */
unsigned int alloc_ptid( void *ptr )
{
    struct ptid_entry *entry;
    unsigned int index;

    if (used_ptid_entries < alloc_ptid_entries)
    {
        index = used_ptid_entries + PTID_OFFSET;
        entry = &ptid_entries[used_ptid_entries++];
    }
    else if (next_free_ptid && num_free_ptids >= 256)
    {
        index = next_free_ptid;
        entry = &ptid_entries[index - PTID_OFFSET];
        if (!(next_free_ptid = entry->next)) last_free_ptid = 0;
        num_free_ptids--;
    }
    else  /* need to grow the array */
    {
        unsigned int count = alloc_ptid_entries + (alloc_ptid_entries / 2);
        if (!count) count = 512;
        if (!(entry = realloc( ptid_entries, count * sizeof(*entry) )))
        {
            set_error( STATUS_NO_MEMORY );
            return 0;
        }
        ptid_entries = entry;
        alloc_ptid_entries = count;
        index = used_ptid_entries + PTID_OFFSET;
        entry = &ptid_entries[used_ptid_entries++];
    }

    entry->ptr = ptr;
    return ptid_from_index( index );
}

/* free a process or thread id */
void free_ptid( unsigned int id )
{
    unsigned int index = index_from_ptid( id );
    struct ptid_entry *entry = &ptid_entries[index - PTID_OFFSET];

    entry->ptr  = NULL;
    entry->next = 0;

    /* append to end of free list so that we don't reuse it too early */
    if (last_free_ptid) ptid_entries[last_free_ptid - PTID_OFFSET].next = index;
    else next_free_ptid = index;
    last_free_ptid = index;
    num_free_ptids++;
}

/* retrieve the pointer corresponding to a process or thread id */
void *get_ptid_entry( unsigned int id )
{
    unsigned int index = index_from_ptid( id );
    if (index < PTID_OFFSET) return NULL;
    if (index - PTID_OFFSET >= used_ptid_entries) return NULL;
    return ptid_entries[index - PTID_OFFSET].ptr;
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

/* create a new process */
/* if the function fails the fd is closed */
struct process *create_process( int fd, struct process *parent, int inherit_all, const startup_info_t *info,
                                const struct security_descriptor *sd, const obj_handle_t *handles,
                                unsigned int handle_count, struct token *token )
{
    struct process *process;

    if (!(process = alloc_object( &process_ops )))
    {
        close( fd );
        goto error;
    }
    process->parent_id       = 0;
    process->debugger        = NULL;
    process->debug_event     = NULL;
    process->handles         = NULL;
    process->msg_fd          = NULL;
    process->sigkill_timeout = NULL;
    process->unix_pid        = -1;
    process->exit_code       = STILL_ACTIVE;
    process->running_threads = 0;
    process->priority        = PROCESS_PRIOCLASS_NORMAL;
    process->suspend         = 0;
    process->is_system       = 0;
    process->debug_children  = 1;
    process->is_terminating  = 0;
    process->job             = NULL;
    process->console         = NULL;
    process->startup_state   = STARTUP_IN_PROGRESS;
    process->startup_info    = NULL;
    process->idle_event      = NULL;
    process->exe_file        = NULL;
    process->peb             = 0;
    process->ldt_copy        = 0;
    process->dir_cache       = NULL;
    process->winstation      = 0;
    process->desktop         = 0;
    process->token           = NULL;
    process->trace_data      = 0;
    process->rawinput_mouse  = NULL;
    process->rawinput_kbd    = NULL;
    list_init( &process->kernel_object );
    list_init( &process->thread_list );
    list_init( &process->locks );
    list_init( &process->asyncs );
    list_init( &process->classes );
    list_init( &process->views );
    list_init( &process->dlls );
    list_init( &process->rawinput_devices );

    process->end_time = 0;
    list_add_tail( &process_list, &process->entry );

    if (sd && !default_set_sd( &process->obj, sd, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                               DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION ))
    {
        close( fd );
        goto error;
    }
    if (!(process->id = process->group_id = alloc_ptid( process )))
    {
        close( fd );
        goto error;
    }
    if (!(process->msg_fd = create_anonymous_fd( &process_fd_ops, fd, &process->obj, 0 ))) goto error;

    /* create the handle table */
    if (!parent)
    {
        process->handles = alloc_handle_table( process, 0 );
        process->token = token_create_admin();
        process->affinity = ~0;
    }
    else
    {
        obj_handle_t std_handles[3];

        std_handles[0] = info->hstdin;
        std_handles[1] = info->hstdout;
        std_handles[2] = info->hstderr;

        process->parent_id = parent->id;
        process->handles = inherit_all ? copy_handle_table( process, parent, handles, handle_count, std_handles )
                                       : alloc_handle_table( process, 0 );
        /* Note: for security reasons, starting a new process does not attempt
         * to use the current impersonation token for the new process */
        process->token = token_duplicate( token ? token : parent->token, TRUE, 0, NULL, NULL, 0, NULL, 0 );
        process->affinity = parent->affinity;
    }
    if (!process->handles || !process->token) goto error;

    /* Assign a high security label to the token. The default would be medium
     * but Wine provides admin access to all applications right now so high
     * makes more sense for the time being. */
    if (!token_assign_label( process->token, security_high_label_sid ))
        goto error;

    set_fd_events( process->msg_fd, POLLIN );  /* start listening to events */
    return process;

 error:
    if (process) release_object( process );
    /* if we failed to start our first process, close everything down */
    if (!running_processes && master_socket_timeout != TIMEOUT_INFINITE) close_master_socket( 0 );
    return NULL;
}

/* initialize the current process and fill in the request */
data_size_t init_process( struct thread *thread )
{
    struct process *process = thread->process;
    struct startup_info *info = process->startup_info;

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
    assert( list_empty( &process->asyncs ));

    assert( !process->sigkill_timeout );  /* timeout should hold a reference to the process */

    close_process_handles( process );
    set_process_startup_state( process, STARTUP_ABORTED );

    if (process->job)
    {
        list_remove( &process->job_entry );
        release_object( process->job );
    }
    if (process->console) release_object( process->console );
    if (process->msg_fd) release_object( process->msg_fd );
    list_remove( &process->entry );
    if (process->idle_event) release_object( process->idle_event );
    if (process->exe_file) release_object( process->exe_file );
    if (process->id) free_ptid( process->id );
    if (process->token) release_object( process->token );
    free( process->dir_cache );
}

/* dump a process on stdout for debugging purposes */
static void process_dump( struct object *obj, int verbose )
{
    struct process *process = (struct process *)obj;
    assert( obj->ops == &process_ops );

    fprintf( stderr, "Process id=%04x handles=%p\n", process->id, process->handles );
}

static struct object_type *process_get_type( struct object *obj )
{
    static const WCHAR name[] = {'P','r','o','c','e','s','s'};
    static const struct unicode_str str = { name, sizeof(name) };
    return get_object_type( &str );
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

    if (access & PROCESS_QUERY_INFORMATION) access |= PROCESS_QUERY_LIMITED_INFORMATION;
    if (access & PROCESS_SET_INFORMATION) access |= PROCESS_SET_LIMITED_INFORMATION;

    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static struct list *process_get_kernel_obj_list( struct object *obj )
{
    struct process *process = (struct process *)obj;
    return &process->kernel_object;
}

static struct security_descriptor *process_get_sd( struct object *obj )
{
    static struct security_descriptor *process_default_sd;

    if (obj->sd) return obj->sd;

    if (!process_default_sd)
    {
        size_t users_sid_len = security_sid_len( security_domain_users_sid );
        size_t admins_sid_len = security_sid_len( security_builtin_admins_sid );
        size_t dacl_len = sizeof(ACL) + 2 * offsetof( ACCESS_ALLOWED_ACE, SidStart )
                          + users_sid_len + admins_sid_len;
        ACCESS_ALLOWED_ACE *aaa;
        ACL *dacl;

        process_default_sd = mem_alloc( sizeof(*process_default_sd) + admins_sid_len + users_sid_len
                                    + dacl_len );
        process_default_sd->control   = SE_DACL_PRESENT;
        process_default_sd->owner_len = admins_sid_len;
        process_default_sd->group_len = users_sid_len;
        process_default_sd->sacl_len  = 0;
        process_default_sd->dacl_len  = dacl_len;
        memcpy( process_default_sd + 1, security_builtin_admins_sid, admins_sid_len );
        memcpy( (char *)(process_default_sd + 1) + admins_sid_len, security_domain_users_sid, users_sid_len );

        dacl = (ACL *)((char *)(process_default_sd + 1) + admins_sid_len + users_sid_len);
        dacl->AclRevision = ACL_REVISION;
        dacl->Sbz1 = 0;
        dacl->AclSize = dacl_len;
        dacl->AceCount = 2;
        dacl->Sbz2 = 0;
        aaa = (ACCESS_ALLOWED_ACE *)(dacl + 1);
        aaa->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
        aaa->Header.AceFlags = INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE;
        aaa->Header.AceSize = offsetof( ACCESS_ALLOWED_ACE, SidStart ) + users_sid_len;
        aaa->Mask = GENERIC_READ;
        memcpy( &aaa->SidStart, security_domain_users_sid, users_sid_len );
        aaa = (ACCESS_ALLOWED_ACE *)((char *)aaa + aaa->Header.AceSize);
        aaa->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
        aaa->Header.AceFlags = 0;
        aaa->Header.AceSize = offsetof( ACCESS_ALLOWED_ACE, SidStart ) + admins_sid_len;
        aaa->Mask = PROCESS_ALL_ACCESS;
        memcpy( &aaa->SidStart, security_builtin_admins_sid, admins_sid_len );
    }
    return process_default_sd;
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
    if (info->process) release_object( info->process );
}

static void startup_info_dump( struct object *obj, int verbose )
{
    struct startup_info *info = (struct startup_info *)obj;
    assert( obj->ops == &startup_info_ops );

    fputs( "Startup info", stderr );
    if (info->data)
        fprintf( stderr, " in=%04x out=%04x err=%04x",
                 info->data->hstdin, info->data->hstdout, info->data->hstderr );
    fputc( '\n', stderr );
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
static struct process_dll *process_load_dll( struct process *process, mod_handle_t base,
                                             const WCHAR *filename, data_size_t name_len )
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
        dll->base = base;
        dll->filename = NULL;
        dll->namelen  = name_len;
        if (name_len && !(dll->filename = memdup( filename, name_len )))
        {
            free( dll );
            return NULL;
        }
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
    cancel_process_asyncs( process );
    if (process->idle_event) release_object( process->idle_event );
    if (process->exe_file) release_object( process->exe_file );
    process->idle_event = NULL;
    process->exe_file = NULL;

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
        free( dll->filename );
        list_remove( &dll->entry );
        free( dll );
    }
    destroy_process_classes( process );
    free_mapped_views( process );
    free_process_user_handles( process );
    remove_process_locks( process );
    set_process_startup_state( process, STARTUP_ABORTED );
    finish_process_tracing( process );
    release_job_process( process );
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

/* create a new process */
DECL_HANDLER(new_process)
{
    struct startup_info *info;
    const void *info_ptr;
    struct unicode_str name;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, NULL );
    struct process *process = NULL;
    struct token *token = NULL;
    struct process *parent;
    struct thread *parent_thread = current;
    int socket_fd = thread_get_inflight_fd( current, req->socket_fd );
    const obj_handle_t *handles = NULL;

    if (socket_fd == -1)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    if (!objattr)
    {
        set_error( STATUS_INVALID_PARAMETER );
        close( socket_fd );
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

    if (req->parent_process)
    {
        if (!(parent = get_process_from_handle( req->parent_process, PROCESS_CREATE_PROCESS)))
        {
            close( socket_fd );
            return;
        }
        parent_thread = NULL;
    }
    else parent = (struct process *)grab_object( current->process );

    if (parent->job && (req->create_flags & CREATE_BREAKAWAY_FROM_JOB) &&
        !(parent->job->limit_flags & (JOB_OBJECT_LIMIT_BREAKAWAY_OK | JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK)))
    {
        set_error( STATUS_ACCESS_DENIED );
        close( socket_fd );
        release_object( parent );
        return;
    }

    /* build the startup info for a new process */
    if (!(info = alloc_object( &startup_info_ops )))
    {
        close( socket_fd );
        release_object( parent );
        return;
    }
    info->process  = NULL;
    info->data     = NULL;

    info_ptr = get_req_data_after_objattr( objattr, &info->data_size );

    if ((req->handles_size & 3) || req->handles_size > info->data_size)
    {
        set_error( STATUS_INVALID_PARAMETER );
        close( socket_fd );
        goto done;
    }
    if (req->handles_size)
    {
        handles = info_ptr;
        info_ptr = (const char *)info_ptr + req->handles_size;
        info->data_size -= req->handles_size;
    }
    info->info_size = min( req->info_size, info->data_size );

    if (req->info_size < sizeof(*info->data))
    {
        /* make sure we have a full startup_info_t structure */
        data_size_t env_size = info->data_size - info->info_size;
        data_size_t info_size = min( req->info_size, FIELD_OFFSET( startup_info_t, curdir_len ));

        if (!(info->data = mem_alloc( sizeof(*info->data) + env_size )))
        {
            close( socket_fd );
            goto done;
        }
        memcpy( info->data, info_ptr, info_size );
        memset( (char *)info->data + info_size, 0, sizeof(*info->data) - info_size );
        memcpy( info->data + 1, (const char *)info_ptr + req->info_size, env_size );
        info->info_size = sizeof(startup_info_t);
        info->data_size = info->info_size + env_size;
    }
    else
    {
        data_size_t pos = sizeof(*info->data);

        if (!(info->data = memdup( info_ptr, info->data_size )))
        {
            close( socket_fd );
            goto done;
        }
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

    if (req->token && !(token = get_token_obj( current->process, req->token, TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY )))
    {
        close( socket_fd );
        goto done;
    }

    if (!(process = create_process( socket_fd, parent, req->inherit_all, info->data, sd,
                                    handles, req->handles_size / sizeof(*handles), token )))
        goto done;

    process->startup_info = (struct startup_info *)grab_object( info );

    if (req->exe_file &&
        !(process->exe_file = get_file_obj( current->process, req->exe_file, FILE_READ_DATA )))
        goto done;

    if (parent->job
       && !(req->create_flags & CREATE_BREAKAWAY_FROM_JOB)
       && !(parent->job->limit_flags & JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK))
    {
        add_job_process( parent->job, process );
    }

    /* connect to the window station */
    connect_process_winstation( process, parent_thread, parent );

    /* set the process console */
    if (!(req->create_flags & (DETACHED_PROCESS | CREATE_NEW_CONSOLE)))
    {
        /* FIXME: some better error checking should be done...
         * like if hConOut and hConIn are console handles, then they should be on the same
         * physical console
         */
        info->data->console = inherit_console( parent_thread, info->data->console,
                                               process, req->inherit_all ? info->data->hstdin : 0 );
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
    {
        set_process_debugger( process, current );
        process->debug_children = !(req->create_flags & DEBUG_ONLY_THIS_PROCESS);
    }
    else if (current->process->debugger && current->process->debug_children)
    {
        set_process_debugger( process, current->process->debugger );
        /* debug_children is set to 1 by default */
    }

    if (!(req->create_flags & CREATE_NEW_PROCESS_GROUP))
        process->group_id = parent->group_id;

    info->process = (struct process *)grab_object( process );
    reply->info = alloc_handle( current->process, info, SYNCHRONIZE, 0 );
    reply->pid = get_process_id( process );
    reply->handle = alloc_handle_no_access_check( current->process, process, req->access, objattr->attributes );

 done:
    if (process) release_object( process );
    if (token) release_object( token );
    release_object( parent );
    release_object( info );
}

/* execute a new process, replacing the existing one */
DECL_HANDLER(exec_process)
{
    struct process *process;
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
    if (!(process = create_process( socket_fd, NULL, 0, NULL, NULL, NULL, 0, NULL ))) return;
    create_thread( -1, process, NULL );
    release_object( process );
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
    current->entry_point = req->entry;
    if (process->exe_file) release_object( process->exe_file );
    process->exe_file = NULL;

    init_process_tracing( process );
    generate_startup_debug_events( process, req->entry );
    set_process_startup_state( process, STARTUP_DONE );

    if (req->gui) process->idle_event = create_event( NULL, NULL, 0, 1, 0, NULL );
    if (process->debugger) set_process_debug_flag( process, 1 );
    reply->suspend = (current->suspend || process->suspend);
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

    if ((process = get_process_from_handle( req->handle, PROCESS_QUERY_LIMITED_INFORMATION )))
    {
        reply->pid              = get_process_id( process );
        reply->ppid             = process->parent_id;
        reply->exit_code        = process->exit_code;
        reply->priority         = process->priority;
        reply->affinity         = process->affinity;
        reply->peb              = process->peb;
        reply->start_time       = process->start_time;
        reply->end_time         = process->end_time;
        reply->cpu              = process->cpu;
        reply->debugger_present = !!process->debugger;
        reply->debug_children   = process->debug_children;
        if (get_reply_max_size())
        {
            const pe_image_info_t *info;
            struct process_dll *exe = get_process_exe_module( process );
            if (exe && (info = get_mapping_image_info( process, exe->base )))
                set_reply_data( info, min( sizeof(*info), get_reply_max_size() ));
        }
        release_object( process );
    }
}

/* retrieve information about a process memory usage */
DECL_HANDLER(get_process_vm_counters)
{
    struct process *process = get_process_from_handle( req->handle, PROCESS_QUERY_LIMITED_INFORMATION );

    if (!process) return;
#ifdef linux
    if (process->unix_pid != -1)
    {
        FILE *f;
        char proc_path[32], line[256];
        unsigned long value;

        sprintf( proc_path, "/proc/%u/status", process->unix_pid );
        if ((f = fopen( proc_path, "r" )))
        {
            while (fgets( line, sizeof(line), f ))
            {
                if (sscanf( line, "VmPeak: %lu", &value ))
                    reply->peak_virtual_size = (mem_size_t)value * 1024;
                else if (sscanf( line, "VmSize: %lu", &value ))
                    reply->virtual_size = (mem_size_t)value * 1024;
                else if (sscanf( line, "VmHWM: %lu", &value ))
                    reply->peak_working_set_size = (mem_size_t)value * 1024;
                else if (sscanf( line, "VmRSS: %lu", &value ))
                    reply->working_set_size = (mem_size_t)value * 1024;
                else if (sscanf( line, "RssAnon: %lu", &value ))
                    reply->pagefile_usage += (mem_size_t)value * 1024;
                else if (sscanf( line, "VmSwap: %lu", &value ))
                    reply->pagefile_usage += (mem_size_t)value * 1024;
            }
            reply->peak_pagefile_usage = reply->pagefile_usage;
            fclose( f );
        }
        else set_error( STATUS_ACCESS_DENIED );
    }
    else set_error( STATUS_ACCESS_DENIED );
#endif
    release_object( process );
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

    if ((dll = process_load_dll( current->process, req->base, get_req_data(), get_req_data_size() )))
    {
        dll->dbg_offset = req->dbg_offset;
        dll->dbg_size   = req->dbg_size;
        dll->name       = req->name;
        /* only generate event if initialization is done */
        if (is_process_init_done( current->process ))
            generate_debug_event( current, LOAD_DLL_DEBUG_EVENT, dll );
    }
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

    if ((process = get_process_from_handle( req->handle, PROCESS_QUERY_LIMITED_INFORMATION )))
    {
        struct process_dll *dll;

        if (req->base_address)
            dll = find_process_dll( process, req->base_address );
        else /* NULL means main module */
            dll = list_head( &process->dlls ) ?
                LIST_ENTRY(list_head( &process->dlls ), struct process_dll, entry) : NULL;

        if (dll)
        {
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
        if (!(shutdown_event = create_event( NULL, NULL, OBJ_PERMANENT, 1, 0, NULL ))) return;
        release_object( shutdown_event );
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

/* create a new job object */
DECL_HANDLER(create_job)
{
    struct job *job;
    struct unicode_str name;
    struct object *root;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, &root );

    if (!objattr) return;

    if ((job = create_job_object( root, &name, objattr->attributes, sd )))
    {
        if (get_error() == STATUS_OBJECT_NAME_EXISTS)
            reply->handle = alloc_handle( current->process, job, req->access, objattr->attributes );
        else
            reply->handle = alloc_handle_no_access_check( current->process, job,
                                                          req->access, objattr->attributes );
        release_object( job );
    }
    if (root) release_object( root );
}

/* open a job object */
DECL_HANDLER(open_job)
{
    struct unicode_str name = get_req_unicode_str();

    reply->handle = open_object( current->process, req->rootdir, req->access,
                                 &job_ops, &name, req->attributes );
}

/* assign a job object to a process */
DECL_HANDLER(assign_job)
{
    struct process *process;
    struct job *job = get_job_obj( current->process, req->job, JOB_OBJECT_ASSIGN_PROCESS );

    if (!job) return;

    if ((process = get_process_from_handle( req->process, PROCESS_SET_QUOTA | PROCESS_TERMINATE )))
    {
        if (!process->running_threads)
            set_error( STATUS_PROCESS_IS_TERMINATING );
        else if (process->job)
            set_error( STATUS_ACCESS_DENIED );
        else
            add_job_process( job, process );
        release_object( process );
    }
    release_object( job );
}


/* check if a process is associated with a job */
DECL_HANDLER(process_in_job)
{
    struct process *process;
    struct job *job;

    if (!(process = get_process_from_handle( req->process, PROCESS_QUERY_INFORMATION )))
        return;

    if (!req->job)
    {
        set_error( process->job ? STATUS_PROCESS_IN_JOB : STATUS_PROCESS_NOT_IN_JOB );
    }
    else if ((job = get_job_obj( current->process, req->job, JOB_OBJECT_QUERY )))
    {
        set_error( process->job == job ? STATUS_PROCESS_IN_JOB : STATUS_PROCESS_NOT_IN_JOB );
        release_object( job );
    }
    release_object( process );
}

/* retrieve information about a job */
DECL_HANDLER(get_job_info)
{
    struct job *job = get_job_obj( current->process, req->handle, JOB_OBJECT_QUERY );

    if (!job) return;

    reply->total_processes = job->total_processes;
    reply->active_processes = job->num_processes;
    release_object( job );
}

/* terminate all processes associated with the job */
DECL_HANDLER(terminate_job)
{
    struct job *job = get_job_obj( current->process, req->handle, JOB_OBJECT_TERMINATE );

    if (!job) return;

    terminate_job( job, req->status );
    release_object( job );
}

/* update limits of the job object */
DECL_HANDLER(set_job_limits)
{
    struct job *job = get_job_obj( current->process, req->handle, JOB_OBJECT_SET_ATTRIBUTES );

    if (!job) return;

    job->limit_flags = req->limit_flags;
    release_object( job );
}

/* set the jobs completion port */
DECL_HANDLER(set_job_completion_port)
{
    struct job *job = get_job_obj( current->process, req->job, JOB_OBJECT_SET_ATTRIBUTES );

    if (!job) return;

    if (!job->completion_port)
    {
        job->completion_port = get_completion_obj( current->process, req->port, IO_COMPLETION_MODIFY_STATE );
        job->completion_key = req->key;
    }
    else
        set_error( STATUS_INVALID_PARAMETER );

    release_object( job );
}

/* Suspend a process */
DECL_HANDLER(suspend_process)
{
    struct process *process;

    if ((process = get_process_from_handle( req->handle, PROCESS_SUSPEND_RESUME )))
    {
        struct list *ptr, *next;

        LIST_FOR_EACH_SAFE( ptr, next, &process->thread_list )
        {
            struct thread *thread = LIST_ENTRY( ptr, struct thread, proc_entry );
            suspend_thread( thread );
        }

        release_object( process );
    }
}

/* Resume a process */
DECL_HANDLER(resume_process)
{
    struct process *process;

    if ((process = get_process_from_handle( req->handle, PROCESS_SUSPEND_RESUME )))
    {
        struct list *ptr, *next;

        LIST_FOR_EACH_SAFE( ptr, next, &process->thread_list )
        {
            struct thread *thread = LIST_ENTRY( ptr, struct thread, proc_entry );
            resume_thread( thread );
        }

        release_object( process );
    }
}

/* Get a list of processes and threads currently running */
DECL_HANDLER(list_processes)
{
    struct process *process;
    struct thread *thread;
    unsigned int pos = 0;
    char *buffer;

    reply->process_count = 0;
    reply->info_size = 0;

    LIST_FOR_EACH_ENTRY( process, &process_list, struct process, entry )
    {
        struct process_dll *exe = get_process_exe_module( process );
        reply->info_size = (reply->info_size + 7) & ~7;
        reply->info_size += sizeof(struct process_info);
        if (exe) reply->info_size += exe->namelen;
        reply->info_size = (reply->info_size + 7) & ~7;
        reply->info_size += process->running_threads * sizeof(struct thread_info);
        reply->process_count++;
    }

    if (reply->info_size > get_reply_max_size())
    {
        set_error( STATUS_INFO_LENGTH_MISMATCH );
        return;
    }

    if (!(buffer = set_reply_data_size( reply->info_size ))) return;

    memset( buffer, 0, reply->info_size );
    LIST_FOR_EACH_ENTRY( process, &process_list, struct process, entry )
    {
        struct process_info *process_info;
        struct process_dll *exe = get_process_exe_module( process );

        pos = (pos + 7) & ~7;
        process_info = (struct process_info *)(buffer + pos);
        process_info->start_time = process->start_time;
        process_info->name_len = exe ? exe->namelen : 0;
        process_info->thread_count = process->running_threads;
        process_info->priority = process->priority;
        process_info->pid = process->id;
        process_info->parent_pid = process->parent_id;
        process_info->handle_count = get_handle_table_count(process);
        process_info->unix_pid = process->unix_pid;
        pos += sizeof(*process_info);

        if (exe)
        {
            memcpy( buffer + pos, exe->filename, exe->namelen );
            pos += exe->namelen;
        }

        pos = (pos + 7) & ~7;
        LIST_FOR_EACH_ENTRY( thread, &process->thread_list, struct thread, proc_entry )
        {
            struct thread_info *thread_info = (struct thread_info *)(buffer + pos);

            thread_info->start_time = thread->creation_time;
            thread_info->tid = thread->id;
            thread_info->base_priority = thread->priority;
            thread_info->current_priority = thread->priority; /* FIXME */
            thread_info->unix_tid = thread->unix_tid;
            pos += sizeof(*thread_info);
        }
    }
}
