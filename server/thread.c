/*
 * Server-side thread management
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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>
#ifdef HAVE_SCHED_H
/* FreeBSD needs this for cpu_set_t instead of its cpuset_t */
#define _WITH_CPU_SET_T
#include <sched.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#ifdef __APPLE__
#include <mach/mach_init.h>
#include <mach/mach_time.h>
#include <mach/mach_port.h>
#include <mach/thread_act.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "file.h"
#include "handle.h"
#include "process.h"
#include "thread.h"
#include "request.h"
#include "user.h"
#include "security.h"


/* thread queues */

struct thread_wait
{
    struct thread_wait     *next;       /* next wait structure for this thread */
    struct thread          *thread;     /* owner thread */
    int                     count;      /* count of objects */
    int                     flags;
    int                     abandoned;
    enum select_opcode      select;
    client_ptr_t            key;        /* wait key for keyed events */
    client_ptr_t            cookie;     /* magic cookie to return to client */
    abstime_t               when;
    struct timeout_user    *user;
    int                     status;     /* status to return (unless STATUS_PENDING) */
    struct wait_queue_entry queues[1];
};

/* asynchronous procedure calls */

struct thread_apc
{
    struct object       obj;      /* object header */
    struct list         entry;    /* queue linked list */
    struct thread      *caller;   /* thread that queued this apc */
    struct object      *owner;    /* object that queued this apc */
    int                 executed; /* has it been executed by the client? */
    union apc_call      call;     /* call arguments */
    union apc_result    result;   /* call results once executed */
};

static void dump_thread_apc( struct object *obj, int verbose );
static int thread_apc_signaled( struct object *obj, struct wait_queue_entry *entry );
static void thread_apc_destroy( struct object *obj );
static void clear_apc_queue( struct list *queue );

static const struct object_ops thread_apc_ops =
{
    sizeof(struct thread_apc),  /* size */
    &no_type,                   /* type */
    dump_thread_apc,            /* dump */
    add_queue,                  /* add_queue */
    remove_queue,               /* remove_queue */
    thread_apc_signaled,        /* signaled */
    no_satisfied,               /* satisfied */
    no_signal,                  /* signal */
    no_get_fd,                  /* get_fd */
    default_map_access,         /* map_access */
    default_get_sd,             /* get_sd */
    default_set_sd,             /* set_sd */
    no_get_full_name,           /* get_full_name */
    no_lookup_name,             /* lookup_name */
    no_link_name,               /* link_name */
    NULL,                       /* unlink_name */
    no_open_file,               /* open_file */
    no_kernel_obj_list,         /* get_kernel_obj_list */
    no_close_handle,            /* close_handle */
    thread_apc_destroy          /* destroy */
};


/* thread CPU context */

struct context
{
    struct object   obj;        /* object header */
    unsigned int    status;     /* status of the context */
    struct context_data regs[2];/* context data */
};
#define CTX_NATIVE  0  /* context for native machine */
#define CTX_WOW     1  /* context if thread is inside WoW */

/* flags for registers that always need to be set from the server side */
static const unsigned int system_flags = SERVER_CTX_DEBUG_REGISTERS;

static void dump_context( struct object *obj, int verbose );
static int context_signaled( struct object *obj, struct wait_queue_entry *entry );

static const struct object_ops context_ops =
{
    sizeof(struct context),     /* size */
    &no_type,                   /* type */
    dump_context,               /* dump */
    add_queue,                  /* add_queue */
    remove_queue,               /* remove_queue */
    context_signaled,           /* signaled */
    no_satisfied,               /* satisfied */
    no_signal,                  /* signal */
    no_get_fd,                  /* get_fd */
    default_map_access,         /* map_access */
    default_get_sd,             /* get_sd */
    default_set_sd,             /* set_sd */
    no_get_full_name,           /* get_full_name */
    no_lookup_name,             /* lookup_name */
    no_link_name,               /* link_name */
    NULL,                       /* unlink_name */
    no_open_file,               /* open_file */
    no_kernel_obj_list,         /* get_kernel_obj_list */
    no_close_handle,            /* close_handle */
    no_destroy                  /* destroy */
};


/* thread operations */

static const WCHAR thread_name[] = {'T','h','r','e','a','d'};

struct type_descr thread_type =
{
    { thread_name, sizeof(thread_name) },   /* name */
    THREAD_ALL_ACCESS,                      /* valid_access */
    {                                       /* mapping */
        STANDARD_RIGHTS_READ | THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT,
        STANDARD_RIGHTS_WRITE | THREAD_SET_LIMITED_INFORMATION | THREAD_SET_INFORMATION
        | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME | THREAD_TERMINATE | 0x04,
        STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | THREAD_RESUME | THREAD_QUERY_LIMITED_INFORMATION,
        THREAD_ALL_ACCESS
    },
};

static void dump_thread( struct object *obj, int verbose );
static int thread_signaled( struct object *obj, struct wait_queue_entry *entry );
static unsigned int thread_map_access( struct object *obj, unsigned int access );
static void thread_poll_event( struct fd *fd, int event );
static struct list *thread_get_kernel_obj_list( struct object *obj );
static void destroy_thread( struct object *obj );

static const struct object_ops thread_ops =
{
    sizeof(struct thread),      /* size */
    &thread_type,               /* type */
    dump_thread,                /* dump */
    add_queue,                  /* add_queue */
    remove_queue,               /* remove_queue */
    thread_signaled,            /* signaled */
    no_satisfied,               /* satisfied */
    no_signal,                  /* signal */
    no_get_fd,                  /* get_fd */
    thread_map_access,          /* map_access */
    default_get_sd,             /* get_sd */
    default_set_sd,             /* set_sd */
    no_get_full_name,           /* get_full_name */
    no_lookup_name,             /* lookup_name */
    no_link_name,               /* link_name */
    NULL,                       /* unlink_name */
    no_open_file,               /* open_file */
    thread_get_kernel_obj_list, /* get_kernel_obj_list */
    no_close_handle,            /* close_handle */
    destroy_thread              /* destroy */
};

static const struct fd_ops thread_fd_ops =
{
    NULL,                       /* get_poll_events */
    thread_poll_event,          /* poll_event */
    NULL,                       /* flush */
    NULL,                       /* get_fd_type */
    NULL,                       /* ioctl */
    NULL,                       /* queue_async */
    NULL                        /* reselect_async */
};

static struct list thread_list = LIST_INIT(thread_list);

#if defined(__linux__) && defined(RLIMIT_NICE)
static int nice_limit;

void init_threading(void)
{
    struct rlimit rlimit;

    /* if wineserver has cap_sys_nice we are unlimited, but leave -20 to the user */
    if (!setpriority( PRIO_PROCESS, getpid(), -20 )) nice_limit = -19;
    setpriority( PRIO_PROCESS, getpid(), 0 );

    if (!nice_limit && !getrlimit( RLIMIT_NICE, &rlimit ))
    {
        rlimit.rlim_cur = rlimit.rlim_max;
        setrlimit( RLIMIT_NICE, &rlimit );
        if (rlimit.rlim_max <= 40) nice_limit = 20 - rlimit.rlim_max;
        else if (rlimit.rlim_max == -1) nice_limit = -20;
        if (nice_limit >= 0 && debug_level) fprintf(stderr, "wine: RLIMIT_NICE is <= 20, unable to use setpriority safely\n");
    }
    if (nice_limit < 0 && debug_level) fprintf(stderr, "wine: Using setpriority to control niceness in the [%d,%d] range\n", nice_limit, -nice_limit );
}

static void apply_thread_priority( struct thread *thread, int base_priority )
{
    int min = -nice_limit, max = nice_limit, range = max - min, niceness;

    /* FIXME: handle realtime priorities using SCHED_RR if possible */
    if (base_priority > THREAD_BASE_PRIORITY_LOWRT) base_priority = THREAD_BASE_PRIORITY_LOWRT;
    /* map an NT application band [1,15] base priority to [-nice_limit, nice_limit] */
    niceness = (min + (base_priority - 1) * range / 14);
    setpriority( PRIO_PROCESS, thread->unix_tid, niceness );
}

#elif defined(__APPLE__)
static unsigned int mach_ticks_per_second;

void init_threading(void)
{
    struct mach_timebase_info tb_info;
    if (mach_timebase_info( &tb_info ) == KERN_SUCCESS)
    {
        mach_ticks_per_second = (tb_info.denom * 1000000000U) / tb_info.numer;
    }
    else
    {
        const unsigned int best_guess = 24000000U;
        mach_ticks_per_second = best_guess;
    }
}

static int get_mach_importance( int base_priority )
{
    int min = -31, max = 32, range = max - min;
    return min + (base_priority - 1) * range / 14;
}

static void apply_thread_priority( struct thread *thread, int base_priority )
{
    kern_return_t kr;
    mach_msg_type_name_t type;
    int throughput_qos, latency_qos;
    struct thread_extended_policy thread_extended_policy;
    struct thread_precedence_policy thread_precedence_policy;
    mach_port_t thread_port, process_port = thread->process->trace_data;

    if (!process_port) return;
    kr = mach_port_extract_right( process_port, thread->unix_tid,
                                  MACH_MSG_TYPE_COPY_SEND, &thread_port, &type );
    if (kr != KERN_SUCCESS) return;
    /* base priority 15 is for time-critical threads, so not compute-bound */
    thread_extended_policy.timeshare = base_priority > 14 ? 0 : 1;
    thread_precedence_policy.importance = get_mach_importance( base_priority );
    /* adapted from the QoS table at xnu/osfmk/kern/thread_policy.c */
    switch (thread->priority)
    {
    case THREAD_PRIORITY_IDLE: /* THREAD_QOS_MAINTENANCE */
    case THREAD_PRIORITY_LOWEST: /* THREAD_QOS_BACKGROUND */
        throughput_qos = THROUGHPUT_QOS_TIER_5;
        latency_qos = LATENCY_QOS_TIER_3;
        break;
    case THREAD_PRIORITY_BELOW_NORMAL: /* THREAD_QOS_UTILITY */
        throughput_qos = THROUGHPUT_QOS_TIER_2;
        latency_qos = LATENCY_QOS_TIER_3;
        break;
    case THREAD_PRIORITY_NORMAL: /* THREAD_QOS_LEGACY */
    case THREAD_PRIORITY_ABOVE_NORMAL: /* THREAD_QOS_USER_INITIATED */
        throughput_qos = THROUGHPUT_QOS_TIER_1;
        latency_qos = LATENCY_QOS_TIER_1;
        break;
    case THREAD_PRIORITY_HIGHEST: /* THREAD_QOS_USER_INTERACTIVE */
        throughput_qos = THROUGHPUT_QOS_TIER_0;
        latency_qos = LATENCY_QOS_TIER_0;
        break;
    case THREAD_PRIORITY_TIME_CRITICAL:
    default: /* THREAD_QOS_UNSPECIFIED */
        throughput_qos = THROUGHPUT_QOS_TIER_UNSPECIFIED;
        latency_qos = LATENCY_QOS_TIER_UNSPECIFIED;
        break;
    }
    /* QOS_UNSPECIFIED is assigned the highest tier available, so it does not provide a limit */
    if (base_priority > THREAD_BASE_PRIORITY_LOWRT)
    {
        throughput_qos = THROUGHPUT_QOS_TIER_UNSPECIFIED;
        latency_qos = LATENCY_QOS_TIER_UNSPECIFIED;
    }

    thread_policy_set( thread_port, THREAD_LATENCY_QOS_POLICY, (thread_policy_t)&latency_qos,
                       THREAD_LATENCY_QOS_POLICY_COUNT );
    thread_policy_set( thread_port, THREAD_THROUGHPUT_QOS_POLICY, (thread_policy_t)&throughput_qos,
                       THREAD_THROUGHPUT_QOS_POLICY_COUNT );
    thread_policy_set( thread_port, THREAD_EXTENDED_POLICY, (thread_policy_t)&thread_extended_policy,
                       THREAD_EXTENDED_POLICY_COUNT );
    thread_policy_set( thread_port, THREAD_PRECEDENCE_POLICY, (thread_policy_t)&thread_precedence_policy,
                       THREAD_PRECEDENCE_POLICY_COUNT );
    if (base_priority > THREAD_BASE_PRIORITY_LOWRT)
    {
        /* For realtime threads we are requesting from the scheduler to be moved
         * into the Mach realtime band (96-127) above the kernel.
         * The scheduler will bump us back into the application band though if we
         * lie too much about our computation constraints...
         * The maximum available amount of resources granted in that band is using
         * half of the available bus cycles, and computation (nominally 1/10 of
         * the time constraint) is a hint to the scheduler where to place our
         * realtime threads relative to each other.
         * If someone is violating the time contraint policy, they will be moved
         * back where they were (non-timeshare application band with very high
         * importance), which is on XNU equivalent to setting SCHED_RR with the
         * pthread API. */
        struct thread_time_constraint_policy thread_time_constraint_policy;
        int realtime_priority = base_priority - THREAD_BASE_PRIORITY_LOWRT;
        unsigned int max_constraint = mach_ticks_per_second / 2;
        unsigned int max_computation = max_constraint / 10;
        /* unfortunately we can't give a hint for the periodicity of calculations */
        thread_time_constraint_policy.period = 0;
        thread_time_constraint_policy.constraint = max_constraint;
        thread_time_constraint_policy.computation = realtime_priority * max_computation / 16;
        thread_time_constraint_policy.preemptible = thread->priority == THREAD_PRIORITY_TIME_CRITICAL ? 0 : 1;
        thread_policy_set( thread_port, THREAD_TIME_CONSTRAINT_POLICY,
                           (thread_policy_t)&thread_time_constraint_policy,
                           THREAD_TIME_CONSTRAINT_POLICY_COUNT );
    }
    mach_port_deallocate( mach_task_self(), thread_port );
}

#else

void init_threading(void)
{
}

static void apply_thread_priority( struct thread *thread, int base_priority )
{
}

#endif

/* initialize the structure for a newly allocated thread */
static inline void init_thread_structure( struct thread *thread )
{
    int i;

    thread->unix_pid        = -1;  /* not known yet */
    thread->unix_tid        = -1;  /* not known yet */
    thread->context         = NULL;
    thread->teb             = 0;
    thread->entry_point     = 0;
    thread->system_regs     = 0;
    thread->queue           = NULL;
    thread->wait            = NULL;
    thread->error           = 0;
    thread->req_data        = NULL;
    thread->req_toread      = 0;
    thread->reply_data      = NULL;
    thread->reply_towrite   = 0;
    thread->request_fd      = NULL;
    thread->reply_fd        = NULL;
    thread->wait_fd         = NULL;
    thread->state           = RUNNING;
    thread->exit_code       = 0;
    thread->priority        = 0;
    thread->suspend         = 0;
    thread->dbg_hidden      = 0;
    thread->desktop_users   = 0;
    thread->token           = NULL;
    thread->desc            = NULL;
    thread->desc_len        = 0;

    thread->creation_time = current_time;
    thread->exit_time     = 0;
    thread->completion_wait = NULL;

    list_init( &thread->mutex_list );
    list_init( &thread->system_apc );
    list_init( &thread->user_apc );
    list_init( &thread->kernel_object );

    for (i = 0; i < MAX_INFLIGHT_FDS; i++)
        thread->inflight[i].server = thread->inflight[i].client = -1;
}

/* check if address looks valid for a client-side data structure (TEB etc.) */
static inline int is_valid_address( client_ptr_t addr )
{
    return addr && !(addr % sizeof(int));
}


/* dump a context on stdout for debugging purposes */
static void dump_context( struct object *obj, int verbose )
{
    struct context *context = (struct context *)obj;
    assert( obj->ops == &context_ops );

    fprintf( stderr, "context flags=%x/%x\n",
             context->regs[CTX_NATIVE].flags, context->regs[CTX_WOW].flags );
}


static int context_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct context *context = (struct context *)obj;
    return context->status != STATUS_PENDING;
}


static struct context *create_thread_context( struct thread *thread )
{
    struct context *context;
    if (!(context = alloc_object( &context_ops ))) return NULL;
    context->status = STATUS_PENDING;
    memset( &context->regs, 0, sizeof(context->regs) );
    context->regs[CTX_NATIVE].machine = native_machine;
    return context;
}


/* create a new thread */
struct thread *create_thread( int fd, struct process *process, const struct security_descriptor *sd )
{
    struct desktop *desktop;
    struct thread *thread;
    int request_pipe[2];

    if (fd == -1)
    {
        if (pipe( request_pipe ) == -1)
        {
            file_set_error();
            return NULL;
        }
        if (send_client_fd( process, request_pipe[1], SERVER_PROTOCOL_VERSION ) == -1)
        {
            close( request_pipe[0] );
            close( request_pipe[1] );
            return NULL;
        }
        close( request_pipe[1] );
        fd = request_pipe[0];
    }

    if (process->is_terminating)
    {
        close( fd );
        set_error( STATUS_PROCESS_IS_TERMINATING );
        return NULL;
    }

    if (!(thread = alloc_object( &thread_ops )))
    {
        close( fd );
        return NULL;
    }

    init_thread_structure( thread );

    thread->process = (struct process *)grab_object( process );
    thread->desktop = 0;
    thread->affinity = process->affinity;
    if (!current) current = thread;

    list_add_tail( &thread_list, &thread->entry );

    if (sd && !set_sd_defaults_from_token( &thread->obj, sd,
                                           OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                                           DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
                                           process->token ))
    {
        close( fd );
        release_object( thread );
        return NULL;
    }
    if (!(thread->id = alloc_ptid( thread )))
    {
        close( fd );
        release_object( thread );
        return NULL;
    }
    if (!(thread->request_fd = create_anonymous_fd( &thread_fd_ops, fd, &thread->obj, 0 )))
    {
        release_object( thread );
        return NULL;
    }

    if (process->desktop)
    {
        if (!(desktop = get_desktop_obj( process, process->desktop, 0 ))) clear_error();  /* ignore errors */
        else
        {
            set_thread_default_desktop( thread, desktop, process->desktop );
            release_object( desktop );
        }
    }

    set_fd_events( thread->request_fd, POLLIN );  /* start listening to events */
    add_process_thread( thread->process, thread );
    return thread;
}

/* handle a client event */
static void thread_poll_event( struct fd *fd, int event )
{
    struct thread *thread = get_fd_user( fd );
    assert( thread->obj.ops == &thread_ops );

    grab_object( thread );
    if (event & (POLLERR | POLLHUP)) kill_thread( thread, 0 );
    else if (event & POLLIN) read_request( thread );
    else if (event & POLLOUT) write_reply( thread );
    release_object( thread );
}

static struct list *thread_get_kernel_obj_list( struct object *obj )
{
    struct thread *thread = (struct thread *)obj;
    return &thread->kernel_object;
}

/* cleanup everything that is no longer needed by a dead thread */
/* used by destroy_thread and kill_thread */
static void cleanup_thread( struct thread *thread )
{
    int i;

    cleanup_thread_completion( thread );
    if (thread->context)
    {
        thread->context->status = STATUS_ACCESS_DENIED;
        wake_up( &thread->context->obj, 0 );
        release_object( thread->context );
        thread->context = NULL;
    }
    clear_apc_queue( &thread->system_apc );
    clear_apc_queue( &thread->user_apc );
    free( thread->req_data );
    free( thread->reply_data );
    if (thread->request_fd) release_object( thread->request_fd );
    if (thread->reply_fd) release_object( thread->reply_fd );
    if (thread->wait_fd) release_object( thread->wait_fd );
    cleanup_clipboard_thread(thread);
    destroy_thread_windows( thread );
    free_msg_queue( thread );
    release_thread_desktop( thread, 1 );
    for (i = 0; i < MAX_INFLIGHT_FDS; i++)
    {
        if (thread->inflight[i].client != -1)
        {
            close( thread->inflight[i].server );
            thread->inflight[i].client = thread->inflight[i].server = -1;
        }
    }
    free( thread->desc );
    thread->req_data = NULL;
    thread->reply_data = NULL;
    thread->request_fd = NULL;
    thread->reply_fd = NULL;
    thread->wait_fd = NULL;
    thread->desktop = 0;
    thread->desc = NULL;
    thread->desc_len = 0;
}

/* destroy a thread when its refcount is 0 */
static void destroy_thread( struct object *obj )
{
    struct thread *thread = (struct thread *)obj;
    assert( obj->ops == &thread_ops );

    list_remove( &thread->entry );
    cleanup_thread( thread );
    release_object( thread->process );
    if (thread->id) free_ptid( thread->id );
    if (thread->token) release_object( thread->token );
}

/* dump a thread on stdout for debugging purposes */
static void dump_thread( struct object *obj, int verbose )
{
    struct thread *thread = (struct thread *)obj;
    assert( obj->ops == &thread_ops );

    fprintf( stderr, "Thread id=%04x unix pid=%d unix tid=%d state=%d\n",
             thread->id, thread->unix_pid, thread->unix_tid, thread->state );
}

static int thread_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct thread *mythread = (struct thread *)obj;
    return (mythread->state == TERMINATED);
}

static unsigned int thread_map_access( struct object *obj, unsigned int access )
{
    access = default_map_access( obj, access );
    if (access & THREAD_QUERY_INFORMATION) access |= THREAD_QUERY_LIMITED_INFORMATION;
    if (access & THREAD_SET_INFORMATION) access |= THREAD_SET_LIMITED_INFORMATION;
    return access;
}

static void dump_thread_apc( struct object *obj, int verbose )
{
    struct thread_apc *apc = (struct thread_apc *)obj;
    assert( obj->ops == &thread_apc_ops );

    fprintf( stderr, "APC owner=%p type=%u\n", apc->owner, apc->call.type );
}

static int thread_apc_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct thread_apc *apc = (struct thread_apc *)obj;
    return apc->executed;
}

static void thread_apc_destroy( struct object *obj )
{
    struct thread_apc *apc = (struct thread_apc *)obj;

    if (apc->caller) release_object( apc->caller );
    if (apc->owner)
    {
        if (apc->result.type == APC_ASYNC_IO)
            async_set_result( apc->owner, apc->result.async_io.status, apc->result.async_io.total );
        else if (apc->call.type == APC_ASYNC_IO)
            async_set_result( apc->owner, apc->call.async_io.status, 0 );
        release_object( apc->owner );
    }
}

/* queue an async procedure call */
static struct thread_apc *create_apc( struct object *owner, const union apc_call *call_data )
{
    struct thread_apc *apc;

    if ((apc = alloc_object( &thread_apc_ops )))
    {
        if (call_data) apc->call = *call_data;
        else apc->call.type = APC_NONE;
        apc->caller      = NULL;
        apc->owner       = owner;
        apc->executed    = 0;
        apc->result.type = APC_NONE;
        if (owner) grab_object( owner );
    }
    return apc;
}

/* get a thread pointer from a thread id (and increment the refcount) */
struct thread *get_thread_from_id( thread_id_t id )
{
    struct object *obj = get_ptid_entry( id );

    if (obj && obj->ops == &thread_ops) return (struct thread *)grab_object( obj );
    set_error( STATUS_INVALID_CID );
    return NULL;
}

/* get a thread from a handle (and increment the refcount) */
struct thread *get_thread_from_handle( obj_handle_t handle, unsigned int access )
{
    return (struct thread *)get_handle_obj( current->process, handle,
                                            access, &thread_ops );
}

/* find a thread from a Unix tid */
struct thread *get_thread_from_tid( int tid )
{
    struct thread *thread;

    LIST_FOR_EACH_ENTRY( thread, &thread_list, struct thread, entry )
    {
        if (thread->unix_tid == tid) return thread;
    }
    return NULL;
}

/* find a thread from a Unix pid */
struct thread *get_thread_from_pid( int pid )
{
    struct thread *thread;

    LIST_FOR_EACH_ENTRY( thread, &thread_list, struct thread, entry )
    {
        if (thread->unix_pid == pid) return thread;
    }
    return NULL;
}

int set_thread_affinity( struct thread *thread, affinity_t affinity )
{
    int ret = 0;
#ifdef HAVE_SCHED_SETAFFINITY
    if (thread->unix_tid != -1)
    {
        cpu_set_t set;
        int i;
        affinity_t mask;

        CPU_ZERO( &set );
        for (i = 0, mask = 1; mask; i++, mask <<= 1)
            if (affinity & mask) CPU_SET( i, &set );

        ret = sched_setaffinity( thread->unix_tid, sizeof(set), &set );
    }
#endif
    if (!ret) thread->affinity = affinity;
    return ret;
}

affinity_t get_thread_affinity( struct thread *thread )
{
    affinity_t mask = 0;
#ifdef HAVE_SCHED_SETAFFINITY
    if (thread->unix_tid != -1)
    {
        cpu_set_t set;
        unsigned int i;

        if (!sched_getaffinity( thread->unix_tid, sizeof(set), &set ))
            for (i = 0; i < 8 * sizeof(mask); i++)
                if (CPU_ISSET( i, &set )) mask |= (affinity_t)1 << i;
    }
#endif
    if (!mask) mask = ~(affinity_t)0;
    return mask;
}

static int get_base_priority( int priority_class, int priority )
{
    /* offsets taken from https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities */
    static const int class_offsets[] = { 4, 8, 13, 24, 6, 10 };

    if (priority == THREAD_PRIORITY_IDLE) return (priority_class == PROCESS_PRIOCLASS_REALTIME ? 16 : 1);
    if (priority == THREAD_PRIORITY_TIME_CRITICAL) return (priority_class == PROCESS_PRIOCLASS_REALTIME ? 31 : 15);
    if (priority_class >= ARRAY_SIZE(class_offsets)) return 8;
    return class_offsets[priority_class - 1] + priority;
}

#define THREAD_PRIORITY_REALTIME_HIGHEST 6
#define THREAD_PRIORITY_REALTIME_LOWEST -7

unsigned int set_thread_priority( struct thread *thread, int priority )
{
    int priority_class = thread->process->priority;
    int max = THREAD_PRIORITY_HIGHEST;
    int min = THREAD_PRIORITY_LOWEST;

    if (priority_class == PROCESS_PRIOCLASS_REALTIME)
    {
        max = THREAD_PRIORITY_REALTIME_HIGHEST;
        min = THREAD_PRIORITY_REALTIME_LOWEST;
    }
    if ((priority < min || priority > max) &&
        priority != THREAD_PRIORITY_IDLE &&
        priority != THREAD_PRIORITY_TIME_CRITICAL)
        return STATUS_INVALID_PARAMETER;

    thread->priority = priority;

    /* if thread is gone or hasn't started yet, this will be called again from init_thread with a unix_tid */
    if (thread->state == RUNNING && thread->unix_tid != -1)
        apply_thread_priority( thread, get_base_priority( priority_class, priority ));

    return STATUS_SUCCESS;
}

/* set all information about a thread */
static void set_thread_info( struct thread *thread,
                             const struct set_thread_info_request *req )
{
    if (req->mask & SET_THREAD_INFO_PRIORITY)
    {
        unsigned int status = set_thread_priority( thread, req->priority );
        if (status) set_error( status );
    }
    if (req->mask & SET_THREAD_INFO_AFFINITY)
    {
        if ((req->affinity & thread->process->affinity) != req->affinity)
            set_error( STATUS_INVALID_PARAMETER );
        else if (thread->state == TERMINATED)
            set_error( STATUS_THREAD_IS_TERMINATING );
        else if (set_thread_affinity( thread, req->affinity ))
            file_set_error();
    }
    if (req->mask & SET_THREAD_INFO_TOKEN)
        security_set_thread_token( thread, req->token );
    if (req->mask & SET_THREAD_INFO_ENTRYPOINT)
        thread->entry_point = req->entry_point;
    if (req->mask & SET_THREAD_INFO_DBG_HIDDEN)
        thread->dbg_hidden = 1;
    if (req->mask & SET_THREAD_INFO_DESCRIPTION)
    {
        WCHAR *desc;
        data_size_t desc_len = get_req_data_size();

        if (desc_len)
        {
            if ((desc = mem_alloc( desc_len )))
            {
                memcpy( desc, get_req_data(), desc_len );
                free( thread->desc );
                thread->desc = desc;
                thread->desc_len = desc_len;
            }
        }
        else
        {
            free( thread->desc );
            thread->desc = NULL;
            thread->desc_len = 0;
        }
    }
}

/* stop a thread (at the Unix level) */
void stop_thread( struct thread *thread )
{
    if (thread->context) return;  /* already suspended, no need for a signal */
    if (!(thread->context = create_thread_context( thread ))) return;
    /* can't stop a thread while initialisation is in progress */
    if (is_process_init_done(thread->process)) send_thread_signal( thread, SIGUSR1 );
}

/* suspend a thread */
int suspend_thread( struct thread *thread )
{
    int old_count = thread->suspend;
    if (thread->suspend < MAXIMUM_SUSPEND_COUNT)
    {
        if (!(thread->process->suspend + thread->suspend++)) stop_thread( thread );
    }
    else set_error( STATUS_SUSPEND_COUNT_EXCEEDED );
    return old_count;
}

/* resume a thread */
int resume_thread( struct thread *thread )
{
    int old_count = thread->suspend;
    if (thread->suspend > 0)
    {
        if (!(--thread->suspend)) resume_delayed_debug_events( thread );
        if (!(thread->suspend + thread->process->suspend)) wake_thread( thread );
    }
    return old_count;
}

/* add a thread to an object wait queue; return 1 if OK, 0 on error */
int add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    grab_object( obj );
    entry->obj = obj;
    list_add_tail( &obj->wait_queue, &entry->entry );
    return 1;
}

/* remove a thread from an object wait queue */
void remove_queue( struct object *obj, struct wait_queue_entry *entry )
{
    list_remove( &entry->entry );
    release_object( obj );
}

struct thread *get_wait_queue_thread( struct wait_queue_entry *entry )
{
    return entry->wait->thread;
}

enum select_opcode get_wait_queue_select_op( struct wait_queue_entry *entry )
{
    return entry->wait->select;
}

client_ptr_t get_wait_queue_key( struct wait_queue_entry *entry )
{
    return entry->wait->key;
}

void make_wait_abandoned( struct wait_queue_entry *entry )
{
    entry->wait->abandoned = 1;
}

void set_wait_status( struct wait_queue_entry *entry, int status )
{
    entry->wait->status = status;
}

/* finish waiting */
static unsigned int end_wait( struct thread *thread, unsigned int status )
{
    struct thread_wait *wait = thread->wait;
    struct wait_queue_entry *entry;
    int i;

    assert( wait );
    thread->wait = wait->next;

    if (status < wait->count)  /* wait satisfied, tell it to the objects */
    {
        wait->status = status;
        if (wait->select == SELECT_WAIT_ALL)
        {
            for (i = 0, entry = wait->queues; i < wait->count; i++, entry++)
                entry->obj->ops->satisfied( entry->obj, entry );
        }
        else
        {
            entry = wait->queues + status;
            entry->obj->ops->satisfied( entry->obj, entry );
        }
        status = wait->status;
        if (wait->abandoned) status += STATUS_ABANDONED_WAIT_0;
    }
    for (i = 0, entry = wait->queues; i < wait->count; i++, entry++)
        entry->obj->ops->remove_queue( entry->obj, entry );
    if (wait->user) remove_timeout_user( wait->user );
    free( wait );
    return status;
}

/* build the thread wait structure */
static int wait_on( const union select_op *select_op, unsigned int count, struct object *objects[],
                    int flags, abstime_t when )
{
    struct thread_wait *wait;
    struct wait_queue_entry *entry;
    unsigned int i, idle = 0;

    if (!(wait = mem_alloc( FIELD_OFFSET(struct thread_wait, queues[count]) ))) return 0;
    wait->next    = current->wait;
    wait->thread  = current;
    wait->count   = count;
    wait->flags   = flags;
    wait->select  = select_op->op;
    wait->cookie  = 0;
    wait->user    = NULL;
    wait->when = when;
    wait->abandoned = 0;
    current->wait = wait;

    for (i = 0, entry = wait->queues; i < count; i++, entry++)
    {
        struct object *obj = objects[i];
        entry->wait = wait;
        if (!obj->ops->add_queue( obj, entry ))
        {
            wait->count = i;
            end_wait( current, get_error() );
            return 0;
        }

        if (obj == (struct object *)current->queue) idle = 1;
    }

    if (idle) check_thread_queue_idle( current );
    return current->wait ? 1 : 0;
}

static int wait_on_handles( const union select_op *select_op, unsigned int count, const obj_handle_t *handles,
                            int flags, abstime_t when )
{
    struct object *objects[MAXIMUM_WAIT_OBJECTS];
    unsigned int i;
    int ret = 0;

    assert( count <= MAXIMUM_WAIT_OBJECTS );

    for (i = 0; i < count; i++)
        if (!(objects[i] = get_handle_obj( current->process, handles[i], SYNCHRONIZE, NULL )))
            break;

    if (i == count) ret = wait_on( select_op, count, objects, flags, when );

    while (i > 0) release_object( objects[--i] );
    return ret;
}

/* check if the thread waiting condition is satisfied */
static int check_wait( struct thread *thread )
{
    int i;
    struct thread_wait *wait = thread->wait;
    struct wait_queue_entry *entry;

    assert( wait );

    if ((wait->flags & SELECT_INTERRUPTIBLE) && !list_empty( &thread->system_apc ))
        return STATUS_KERNEL_APC;

    /* Suspended threads may not acquire locks, but they can run system APCs */
    if (thread->process->suspend + thread->suspend > 0) return -1;

    if (wait->select == SELECT_WAIT_ALL)
    {
        int not_ok = 0;
        /* Note: we must check them all anyway, as some objects may
         * want to do something when signaled, even if others are not */
        for (i = 0, entry = wait->queues; i < wait->count; i++, entry++)
            not_ok |= !entry->obj->ops->signaled( entry->obj, entry );
        if (!not_ok) return STATUS_WAIT_0;
    }
    else
    {
        for (i = 0, entry = wait->queues; i < wait->count; i++, entry++)
            if (entry->obj->ops->signaled( entry->obj, entry )) return i;
    }

    if ((wait->flags & SELECT_ALERTABLE) && !list_empty(&thread->user_apc)) return STATUS_USER_APC;
    if (wait->when >= 0 && wait->when <= current_time) return STATUS_TIMEOUT;
    if (wait->when < 0 && -wait->when <= monotonic_time) return STATUS_TIMEOUT;
    return -1;
}

/* send the wakeup signal to a thread */
static int send_thread_wakeup( struct thread *thread, client_ptr_t cookie, int signaled )
{
    struct wake_up_reply reply;
    int ret;

    /* check if we're waking current suspend wait */
    if (thread->context && thread->suspend_cookie == cookie
        && signaled != STATUS_KERNEL_APC && signaled != STATUS_USER_APC)
    {
        if (!thread->context->regs[CTX_NATIVE].flags && !thread->context->regs[CTX_WOW].flags)
        {
            release_object( thread->context );
            thread->context = NULL;
        }
        else signaled = STATUS_KERNEL_APC; /* signal a fake APC so that client calls select to get a new context */
    }

    memset( &reply, 0, sizeof(reply) );
    reply.cookie   = cookie;
    reply.signaled = signaled;
    if ((ret = write( get_unix_fd( thread->wait_fd ), &reply, sizeof(reply) )) == sizeof(reply))
        return 0;
    if (ret >= 0)
        fatal_protocol_error( thread, "partial wakeup write %d\n", ret );
    else if (errno == EPIPE)
        kill_thread( thread, 0 );  /* normal death */
    else
        fatal_protocol_error( thread, "write: %s\n", strerror( errno ));
    return -1;
}

/* attempt to wake up a thread */
/* return >0 if OK, 0 if the wait condition is still not satisfied and -1 on error */
int wake_thread( struct thread *thread )
{
    int signaled, count;
    client_ptr_t cookie;

    for (count = 0; thread->wait; count++)
    {
        if ((signaled = check_wait( thread )) == -1) break;

        cookie = thread->wait->cookie;
        signaled = end_wait( thread, signaled );
        if (debug_level) fprintf( stderr, "%04x: *wakeup* signaled=%d\n", thread->id, signaled );
        if (cookie && send_thread_wakeup( thread, cookie, signaled ) == -1) /* error */
        {
            if (!count) count = -1;
            break;
        }
    }
    return count;
}

/* attempt to wake up a thread from a wait queue entry, assuming that it is signaled */
int wake_thread_queue_entry( struct wait_queue_entry *entry )
{
    struct thread_wait *wait = entry->wait;
    struct thread *thread = wait->thread;
    int signaled;
    client_ptr_t cookie;

    if (thread->wait != wait) return 0;  /* not the current wait */
    if (thread->process->suspend + thread->suspend > 0) return 0;  /* cannot acquire locks */

    assert( wait->select != SELECT_WAIT_ALL );

    cookie = wait->cookie;
    signaled = end_wait( thread, entry - wait->queues );
    if (debug_level) fprintf( stderr, "%04x: *wakeup* signaled=%d\n", thread->id, signaled );

    if (!cookie || send_thread_wakeup( thread, cookie, signaled ) != -1)
        wake_thread( thread );  /* check other waits too */

    return 1;
}

/* thread wait timeout */
static void thread_timeout( void *ptr )
{
    struct thread_wait *wait = ptr;
    struct thread *thread = wait->thread;
    client_ptr_t cookie = wait->cookie;

    wait->user = NULL;
    if (thread->wait != wait) return; /* not the top-level wait, ignore it */
    if (thread->suspend + thread->process->suspend > 0) return;  /* suspended, ignore it */

    if (debug_level) fprintf( stderr, "%04x: *wakeup* signaled=TIMEOUT\n", thread->id );
    end_wait( thread, STATUS_TIMEOUT );

    assert( cookie );
    if (send_thread_wakeup( thread, cookie, STATUS_TIMEOUT ) == -1) return;
    /* check if other objects have become signaled in the meantime */
    wake_thread( thread );
}

/* try signaling an event flag, a semaphore or a mutex */
static int signal_object( obj_handle_t handle )
{
    struct object *obj;
    int ret = 0;

    obj = get_handle_obj( current->process, handle, 0, NULL );
    if (obj)
    {
        ret = obj->ops->signal( obj, get_handle_access( current->process, handle ));
        release_object( obj );
    }
    return ret;
}

/* select on a list of handles */
static int select_on( const union select_op *select_op, data_size_t op_size, client_ptr_t cookie,
                      int flags, abstime_t when )
{
    int ret;
    unsigned int count;
    struct object *object;

    switch (select_op->op)
    {
    case SELECT_NONE:
        if (!wait_on( select_op, 0, NULL, flags, when )) return 1;
        break;

    case SELECT_WAIT:
    case SELECT_WAIT_ALL:
        count = (op_size - offsetof( union select_op, wait.handles )) / sizeof(select_op->wait.handles[0]);
        if (op_size < offsetof( union select_op, wait.handles ) || count > MAXIMUM_WAIT_OBJECTS)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return 1;
        }
        if (!wait_on_handles( select_op, count, select_op->wait.handles, flags, when ))
            return 1;
        break;

    case SELECT_SIGNAL_AND_WAIT:
        if (!wait_on_handles( select_op, 1, &select_op->signal_and_wait.wait, flags, when ))
            return 1;
        if (select_op->signal_and_wait.signal)
        {
            if (!signal_object( select_op->signal_and_wait.signal ))
            {
                end_wait( current, get_error() );
                return 1;
            }
            /* check if we woke ourselves up */
            if (!current->wait) return 1;
        }
        break;

    case SELECT_KEYED_EVENT_WAIT:
    case SELECT_KEYED_EVENT_RELEASE:
        object = (struct object *)get_keyed_event_obj( current->process, select_op->keyed_event.handle,
                         select_op->op == SELECT_KEYED_EVENT_WAIT ? KEYEDEVENT_WAIT : KEYEDEVENT_WAKE );
        if (!object) return 1;
        ret = wait_on( select_op, 1, &object, flags, when );
        release_object( object );
        if (!ret) return 1;
        current->wait->key = select_op->keyed_event.key;
        break;

    default:
        set_error( STATUS_INVALID_PARAMETER );
        return 1;
    }

    if ((ret = check_wait( current )) != -1)
    {
        /* condition is already satisfied */
        set_error( end_wait( current, ret ));
        return 1;
    }

    /* now we need to wait */
    if (current->wait->when != TIMEOUT_INFINITE)
    {
        if (!(current->wait->user = add_timeout_user( abstime_to_timeout(current->wait->when),
                                                      thread_timeout, current->wait )))
        {
            end_wait( current, get_error() );
            return 1;
        }
    }
    current->wait->cookie = cookie;
    set_error( STATUS_PENDING );
    return 0;
}

/* attempt to wake threads sleeping on the object wait queue */
void wake_up( struct object *obj, int max )
{
    struct list *ptr;
    int ret;

    LIST_FOR_EACH( ptr, &obj->wait_queue )
    {
        struct wait_queue_entry *entry = LIST_ENTRY( ptr, struct wait_queue_entry, entry );
        if (!(ret = wake_thread( get_wait_queue_thread( entry )))) continue;
        if (ret > 0 && max && !--max) break;
        /* restart at the head of the list since a wake up can change the object wait queue */
        ptr = &obj->wait_queue;
    }
}

/* return the apc queue to use for a given apc type */
static inline struct list *get_apc_queue( struct thread *thread, enum apc_type type )
{
    switch(type)
    {
    case APC_NONE:
        return NULL;
    case APC_USER:
        return &thread->user_apc;
    default:
        return &thread->system_apc;
    }
}

/* check if thread is currently waiting for a (system) apc */
static inline int is_in_apc_wait( struct thread *thread )
{
    return (thread->process->suspend || thread->suspend ||
            (thread->wait && (thread->wait->flags & SELECT_INTERRUPTIBLE)));
}

/* queue an existing APC to a given thread */
static int queue_apc( struct process *process, struct thread *thread, struct thread_apc *apc )
{
    struct list *queue;

    if (thread && thread->state == TERMINATED && process)
        thread = NULL;

    if (!thread)  /* find a suitable thread inside the process */
    {
        struct thread *candidate;

        /* first try to find a waiting thread */
        LIST_FOR_EACH_ENTRY( candidate, &process->thread_list, struct thread, proc_entry )
        {
            if (candidate->state == TERMINATED) continue;
            if (is_in_apc_wait( candidate ))
            {
                thread = candidate;
                break;
            }
        }
        if (!thread)
        {
            /* then use the first one that accepts a signal */
            LIST_FOR_EACH_ENTRY( candidate, &process->thread_list, struct thread, proc_entry )
            {
                if (send_thread_signal( candidate, SIGUSR1 ))
                {
                    thread = candidate;
                    break;
                }
            }
        }
        if (!thread) return 0;  /* nothing found */
        if (!(queue = get_apc_queue( thread, apc->call.type ))) return 1;
    }
    else
    {
        if (thread->state == TERMINATED) return 0;
        if (!(queue = get_apc_queue( thread, apc->call.type ))) return 1;
        /* send signal for system APCs if needed */
        if (queue == &thread->system_apc && list_empty( queue ) && !is_in_apc_wait( thread ))
        {
            if (!send_thread_signal( thread, SIGUSR1 )) return 0;
        }
        /* cancel a possible previous APC with the same owner */
        if (apc->owner) thread_cancel_apc( thread, apc->owner, apc->call.type );
    }

    grab_object( apc );
    list_add_tail( queue, &apc->entry );
    if (!list_prev( queue, &apc->entry ))  /* first one */
        wake_thread( thread );

    return 1;
}

/* queue an async procedure call */
int thread_queue_apc( struct process *process, struct thread *thread, struct object *owner, const union apc_call *call_data )
{
    struct thread_apc *apc;
    int ret = 0;

    if ((apc = create_apc( owner, call_data )))
    {
        ret = queue_apc( process, thread, apc );
        release_object( apc );
    }
    return ret;
}

/* cancel the async procedure call owned by a specific object */
void thread_cancel_apc( struct thread *thread, struct object *owner, enum apc_type type )
{
    struct thread_apc *apc;
    struct list *queue = get_apc_queue( thread, type );

    LIST_FOR_EACH_ENTRY( apc, queue, struct thread_apc, entry )
    {
        if (apc->owner != owner) continue;
        list_remove( &apc->entry );
        apc->executed = 1;
        wake_up( &apc->obj, 0 );
        release_object( apc );
        return;
    }
}

/* remove the head apc from the queue; the returned object must be released by the caller */
static struct thread_apc *thread_dequeue_apc( struct thread *thread, int system )
{
    struct thread_apc *apc = NULL;
    struct list *ptr = list_head( system ? &thread->system_apc : &thread->user_apc );

    if (ptr)
    {
        apc = LIST_ENTRY( ptr, struct thread_apc, entry );
        list_remove( ptr );
    }
    return apc;
}

/* clear an APC queue, cancelling all the APCs on it */
static void clear_apc_queue( struct list *queue )
{
    struct list *ptr;

    while ((ptr = list_head( queue )))
    {
        struct thread_apc *apc = LIST_ENTRY( ptr, struct thread_apc, entry );
        list_remove( &apc->entry );
        apc->executed = 1;
        wake_up( &apc->obj, 0 );
        release_object( apc );
    }
}

/* add an fd to the inflight list */
/* return list index, or -1 on error */
int thread_add_inflight_fd( struct thread *thread, int client, int server )
{
    int i;

    if (server == -1) return -1;
    if (client == -1)
    {
        close( server );
        return -1;
    }

    /* first check if we already have an entry for this fd */
    for (i = 0; i < MAX_INFLIGHT_FDS; i++)
        if (thread->inflight[i].client == client)
        {
            close( thread->inflight[i].server );
            thread->inflight[i].server = server;
            return i;
        }

    /* now find a free spot to store it */
    for (i = 0; i < MAX_INFLIGHT_FDS; i++)
        if (thread->inflight[i].client == -1)
        {
            thread->inflight[i].client = client;
            thread->inflight[i].server = server;
            return i;
        }

    close( server );
    return -1;
}

/* get an inflight fd and purge it from the list */
/* the fd must be closed when no longer used */
int thread_get_inflight_fd( struct thread *thread, int client )
{
    int i, ret;

    if (client == -1) return -1;

    do
    {
        for (i = 0; i < MAX_INFLIGHT_FDS; i++)
        {
            if (thread->inflight[i].client == client)
            {
                ret = thread->inflight[i].server;
                thread->inflight[i].server = thread->inflight[i].client = -1;
                return ret;
            }
        }
    } while (!receive_fd( thread->process ));  /* in case it is still in the socket buffer */
    return -1;
}

/* kill a thread on the spot */
void kill_thread( struct thread *thread, int violent_death )
{
    if (thread->state == TERMINATED) return;  /* already killed */
    thread->state = TERMINATED;
    thread->exit_time = current_time;
    if (current == thread) current = NULL;
    if (debug_level)
        fprintf( stderr,"%04x: *killed* exit_code=%d\n",
                 thread->id, thread->exit_code );
    if (thread->wait)
    {
        while (thread->wait) end_wait( thread, STATUS_THREAD_IS_TERMINATING );
        send_thread_wakeup( thread, 0, thread->exit_code );
        /* if it is waiting on the socket, we don't need to send a SIGQUIT */
        violent_death = 0;
    }
    kill_console_processes( thread, 0 );
    abandon_mutexes( thread );
    wake_up( &thread->obj, 0 );
    if (violent_death) send_thread_signal( thread, SIGQUIT );
    cleanup_thread( thread );
    remove_process_thread( thread->process, thread );
    release_object( thread );
}

/* copy parts of a context structure */
static void copy_context( struct context_data *to, const struct context_data *from, unsigned int flags )
{
    assert( to->machine == from->machine );
    if (flags & SERVER_CTX_CONTROL) to->ctl = from->ctl;
    if (flags & SERVER_CTX_INTEGER) to->integer = from->integer;
    if (flags & SERVER_CTX_SEGMENTS) to->seg = from->seg;
    if (flags & SERVER_CTX_FLOATING_POINT) to->fp = from->fp;
    if (flags & SERVER_CTX_DEBUG_REGISTERS) to->debug = from->debug;
    if (flags & SERVER_CTX_EXTENDED_REGISTERS) to->ext = from->ext;
    if (flags & SERVER_CTX_YMM_REGISTERS) to->ymm = from->ymm;
    if (flags & SERVER_CTX_EXEC_SPACE) to->exec_space = from->exec_space;
}

/* gets the current impersonation token */
struct token *thread_get_impersonation_token( struct thread *thread )
{
    if (thread->token)
        return thread->token;
    else
        return thread->process->token;
}

/* create a new thread */
DECL_HANDLER(new_thread)
{
    struct thread *thread;
    struct process *process;
    struct unicode_str name;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, NULL );
    int request_fd = thread_get_inflight_fd( current, req->request_fd );

    if (!(process = get_process_from_handle( req->process, 0 )))
    {
        if (request_fd != -1) close( request_fd );
        return;
    }

    if (process != current->process)
    {
        if (request_fd != -1)  /* can't create a request fd in a different process */
        {
            close( request_fd );
            set_error( STATUS_INVALID_PARAMETER );
            goto done;
        }
        if (process->running_threads)  /* only the initial thread can be created in another process */
        {
            set_error( STATUS_ACCESS_DENIED );
            goto done;
        }
    }
    else if (request_fd == -1 || fcntl( request_fd, F_SETFL, O_NONBLOCK ) == -1)
    {
        if (request_fd != -1) close( request_fd );
        set_error( STATUS_INVALID_HANDLE );
        goto done;
    }
    else if (!(get_handle_access( current->process, req->process ) & PROCESS_CREATE_THREAD))
    {
        close( request_fd );
        set_error( STATUS_ACCESS_DENIED );
        goto done;
    }

    if ((thread = create_thread( request_fd, process, sd )))
    {
        thread->system_regs = current->system_regs;
        if (req->flags & THREAD_CREATE_FLAGS_CREATE_SUSPENDED) thread->suspend++;
        thread->dbg_hidden = !!(req->flags & THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER);
        reply->tid = get_thread_id( thread );
        if ((reply->handle = alloc_handle_no_access_check( current->process, thread,
                                                           req->access, objattr->attributes )))
        {
            /* thread object will be released when the thread gets killed */
            goto done;
        }
        kill_thread( thread, 1 );
    }
done:
    release_object( process );
}

static int init_thread( struct thread *thread, int reply_fd, int wait_fd )
{
    if ((reply_fd = thread_get_inflight_fd( thread, reply_fd )) == -1)
    {
        set_error( STATUS_TOO_MANY_OPENED_FILES );
        return 0;
    }
    if ((wait_fd = thread_get_inflight_fd( thread, wait_fd )) == -1)
    {
        set_error( STATUS_TOO_MANY_OPENED_FILES );
        goto error;
    }

    if (thread->reply_fd)  /* already initialised */
    {
        set_error( STATUS_INVALID_PARAMETER );
        goto error;
    }

    if (fcntl( reply_fd, F_SETFL, O_NONBLOCK ) == -1) goto error;

    thread->reply_fd = create_anonymous_fd( &thread_fd_ops, reply_fd, &thread->obj, 0 );
    thread->wait_fd  = create_anonymous_fd( &thread_fd_ops, wait_fd, &thread->obj, 0 );
    return thread->reply_fd && thread->wait_fd;

 error:
    if (reply_fd != -1) close( reply_fd );
    if (wait_fd != -1) close( wait_fd );
    return 0;
}

/* initialize the first thread of a new process */
DECL_HANDLER(init_first_thread)
{
    struct process *process = current->process;

    if (!init_thread( current, req->reply_fd, req->wait_fd )) return;

    current->unix_pid = process->unix_pid = req->unix_pid;
    current->unix_tid = req->unix_tid;

    if (!process->parent_id)
        process->affinity = current->affinity = get_thread_affinity( current );
    else
        set_thread_affinity( current, current->affinity );

    set_thread_priority( current, current->priority );

    debug_level = max( debug_level, req->debug_level );

    reply->pid          = get_process_id( process );
    reply->tid          = get_thread_id( current );
    reply->session_id   = process->session_id;
    reply->info_size    = get_process_startup_info_size( process );
    reply->server_start = server_start_time;
    set_reply_data( supported_machines,
                    min( supported_machines_count * sizeof(unsigned short), get_reply_max_size() ));
}

/* initialize a new thread */
DECL_HANDLER(init_thread)
{
    if (!init_thread( current, req->reply_fd, req->wait_fd )) return;

    if (!is_valid_address(req->teb))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    current->unix_pid = current->process->unix_pid;
    current->unix_tid = req->unix_tid;
    current->teb      = req->teb;
    current->entry_point = req->entry;

    init_thread_context( current );
    generate_debug_event( current, DbgCreateThreadStateChange, &req->entry );
    set_thread_priority( current, current->priority );
    set_thread_affinity( current, current->affinity );

    reply->suspend = (current->suspend || current->process->suspend || current->context != NULL);
}

/* terminate a thread */
DECL_HANDLER(terminate_thread)
{
    struct thread *thread;

    if ((thread = get_thread_from_handle( req->handle, THREAD_TERMINATE )))
    {
        thread->exit_code = req->exit_code;
        if (thread != current) kill_thread( thread, 1 );
        else reply->self = 1;
        cancel_terminating_thread_asyncs( thread );
        release_object( thread );
    }
}

/* open a handle to a thread */
DECL_HANDLER(open_thread)
{
    struct thread *thread = get_thread_from_id( req->tid );

    reply->handle = 0;
    if (thread)
    {
        reply->handle = alloc_handle( current->process, thread, req->access, req->attributes );
        release_object( thread );
    }
}

/* fetch information about a thread */
DECL_HANDLER(get_thread_info)
{
    struct thread *thread;
    unsigned int access = req->access & (THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION);

    if (!access) access = THREAD_QUERY_LIMITED_INFORMATION;
    thread = get_thread_from_handle( req->handle, access );
    if (thread)
    {
        reply->pid            = get_process_id( thread->process );
        reply->tid            = get_thread_id( thread );
        reply->teb            = thread->teb;
        reply->entry_point    = thread->entry_point;
        reply->exit_code      = (thread->state == TERMINATED) ? thread->exit_code : STATUS_PENDING;
        reply->priority       = thread->priority;
        reply->affinity       = thread->affinity;
        reply->last           = thread->process->running_threads == 1;
        reply->suspend_count  = thread->suspend;
        reply->desc_len       = thread->desc_len;
        reply->flags          = 0;
        if (thread->dbg_hidden)
            reply->flags |= GET_THREAD_INFO_FLAG_DBG_HIDDEN;
        if (thread->state == TERMINATED)
            reply->flags |= GET_THREAD_INFO_FLAG_TERMINATED;

        if (thread->desc && get_reply_max_size())
        {
            if (thread->desc_len <= get_reply_max_size())
                set_reply_data( thread->desc, thread->desc_len );
            else
                set_error( STATUS_BUFFER_TOO_SMALL );
        }

        release_object( thread );
    }
}

/* fetch information about thread times */
DECL_HANDLER(get_thread_times)
{
    struct thread *thread;

    if ((thread = get_thread_from_handle( req->handle, THREAD_QUERY_LIMITED_INFORMATION )))
    {
        reply->creation_time  = thread->creation_time;
        reply->exit_time      = thread->exit_time;
        reply->unix_pid       = thread->unix_pid;
        reply->unix_tid       = thread->unix_tid;

        release_object( thread );
    }
}

/* set information about a thread */
DECL_HANDLER(set_thread_info)
{
    struct thread *thread;
    unsigned int access = (req->mask == SET_THREAD_INFO_DESCRIPTION) ? THREAD_SET_LIMITED_INFORMATION
                                                                     : THREAD_SET_INFORMATION;

    if ((thread = get_thread_from_handle( req->handle, access )))
    {
        set_thread_info( thread, req );
        release_object( thread );
    }
}

/* suspend a thread */
DECL_HANDLER(suspend_thread)
{
    struct thread *thread;

    if ((thread = get_thread_from_handle( req->handle, THREAD_SUSPEND_RESUME )))
    {
        if (thread->state == TERMINATED) set_error( STATUS_ACCESS_DENIED );
        else reply->count = suspend_thread( thread );
        release_object( thread );
    }
}

/* resume a thread */
DECL_HANDLER(resume_thread)
{
    struct thread *thread;

    if ((thread = get_thread_from_handle( req->handle, THREAD_SUSPEND_RESUME )))
    {
        reply->count = resume_thread( thread );
        release_object( thread );
    }
}

/* select on a handle list */
DECL_HANDLER(select)
{
    union select_op select_op;
    data_size_t op_size, ctx_size;
    struct context *ctx;
    struct thread_apc *apc;
    const union apc_result *result = get_req_data();
    unsigned int ctx_count;

    if (get_req_data_size() < sizeof(*result)) goto invalid_param;
    if (get_req_data_size() - sizeof(*result) < req->size) goto invalid_param;
    if (req->size & 3) goto invalid_param;
    ctx_size = get_req_data_size() - sizeof(*result) - req->size;
    ctx_count = ctx_size / sizeof(struct context_data);
    if (ctx_count * sizeof(struct context_data) != ctx_size) goto invalid_param;
    if (ctx_count > 1 + (current->process->machine != native_machine)) goto invalid_param;

    if (ctx_count)
    {
        const struct context_data *native_context = (const struct context_data *)((const char *)(result + 1) + req->size);
        const struct context_data *wow_context = (ctx_count > 1) ? native_context + 1 : NULL;

        if (native_context->machine == native_machine)
        {
            if (wow_context && wow_context->machine != current->process->machine) goto invalid_param;
        }
        else if (native_context->machine == current->process->machine)
        {
            if (wow_context) goto invalid_param;
            wow_context = native_context;
            native_context = NULL;
        }
        else goto invalid_param;

        if ((ctx = current->context))
        {
            if (ctx->status != STATUS_PENDING) goto invalid_param;
            /* if context was modified in pending state, discard irrelevant changes */
            if (wow_context) ctx->regs[CTX_NATIVE].flags &= ~ctx->regs[CTX_WOW].flags;
            else ctx->regs[CTX_WOW].flags = ctx->regs[CTX_WOW].machine = 0;
        }
        else if (!(current->context = create_thread_context( current ))) return;

        ctx = current->context;
        if (native_context)
        {
            copy_context( &ctx->regs[CTX_NATIVE], native_context,
                          native_context->flags & ~(ctx->regs[CTX_NATIVE].flags | system_flags) );
        }
        if (wow_context)
        {
            ctx->regs[CTX_WOW].machine = current->process->machine;
            copy_context( &ctx->regs[CTX_WOW], wow_context, wow_context->flags & ~ctx->regs[CTX_WOW].flags );
        }
        ctx->status = STATUS_SUCCESS;
        current->suspend_cookie = req->cookie;
        wake_up( &ctx->obj, 0 );
    }

    if (!req->cookie) goto invalid_param;

    op_size = min( req->size, sizeof(select_op) );
    memset( &select_op, 0, sizeof(select_op) );
    memcpy( &select_op, result + 1, op_size );

    /* first store results of previous apc */
    if (req->prev_apc)
    {
        if (!(apc = (struct thread_apc *)get_handle_obj( current->process, req->prev_apc,
                                                         0, &thread_apc_ops ))) return;
        apc->result = *result;
        apc->executed = 1;
        if (apc->result.type == APC_CREATE_THREAD)  /* transfer the handle to the caller process */
        {
            obj_handle_t handle = duplicate_handle( current->process, apc->result.create_thread.handle,
                                                    apc->caller->process, 0, 0, DUPLICATE_SAME_ACCESS );
            close_handle( current->process, apc->result.create_thread.handle );
            apc->result.create_thread.handle = handle;
            clear_error();  /* ignore errors from the above calls */
        }
        wake_up( &apc->obj, 0 );
        close_handle( current->process, req->prev_apc );
        release_object( apc );
    }

    reply->signaled = select_on( &select_op, op_size, req->cookie, req->flags, req->timeout );

    if (get_error() == STATUS_USER_APC && get_reply_max_size() >= sizeof(union apc_call))
    {
        apc = thread_dequeue_apc( current, 0 );
        set_reply_data( &apc->call, sizeof(apc->call) );
        release_object( apc );
    }
    else if (get_error() == STATUS_KERNEL_APC && get_reply_max_size() >= sizeof(union apc_call))
    {
        apc = thread_dequeue_apc( current, 1 );
        if ((reply->apc_handle = alloc_handle( current->process, apc, SYNCHRONIZE, 0 )))
        {
            set_reply_data( &apc->call, sizeof(apc->call) );
        }
        else
        {
            apc->executed = 1;
            wake_up( &apc->obj, 0 );
        }
        release_object( apc );
    }
    else if (reply->signaled && get_reply_max_size() >= sizeof(union apc_call) + sizeof(struct context_data) &&
             current->context && current->suspend_cookie == req->cookie)
    {
        ctx = current->context;
        if (ctx->regs[CTX_NATIVE].flags || ctx->regs[CTX_WOW].flags)
        {
            union apc_call *data;
            data_size_t size = sizeof(*data) + (ctx->regs[CTX_WOW].flags ? 2 : 1) * sizeof(struct context_data);
            unsigned int flags = system_flags & ctx->regs[CTX_NATIVE].flags;

            if (flags) set_thread_context( current, &ctx->regs[CTX_NATIVE], flags );
            size = min( size, get_reply_max_size() );
            if ((data = set_reply_data_size( size )))
            {
                memset( data, 0, sizeof(*data) );
                memcpy( data + 1, ctx->regs, size - sizeof(*data) );
            }
        }
        release_object( ctx );
        current->context = NULL;
    }
    return;

invalid_param:
    set_error( STATUS_INVALID_PARAMETER );
}

/* queue an APC for a thread or process */
DECL_HANDLER(queue_apc)
{
    struct thread *thread = NULL;
    struct process *process = NULL;
    struct thread_apc *apc;
    const union apc_call *call = get_req_data();

    if (get_req_data_size() < sizeof(*call)) call = NULL;

    if (!(apc = create_apc( NULL, call ))) return;

    switch (apc->call.type)
    {
    case APC_NONE:
    case APC_USER:
        thread = get_thread_from_handle( req->handle, THREAD_SET_CONTEXT );
        break;
    case APC_VIRTUAL_ALLOC:
    case APC_VIRTUAL_ALLOC_EX:
    case APC_VIRTUAL_FREE:
    case APC_VIRTUAL_PROTECT:
    case APC_VIRTUAL_FLUSH:
    case APC_VIRTUAL_LOCK:
    case APC_VIRTUAL_UNLOCK:
    case APC_UNMAP_VIEW:
        process = get_process_from_handle( req->handle, PROCESS_VM_OPERATION );
        break;
    case APC_VIRTUAL_QUERY:
        process = get_process_from_handle( req->handle, PROCESS_QUERY_LIMITED_INFORMATION );
        break;
    case APC_MAP_VIEW:
    case APC_MAP_VIEW_EX:
        process = get_process_from_handle( req->handle, PROCESS_VM_OPERATION );
        if (process && process != current->process)
        {
            /* duplicate the handle into the target process */
            obj_handle_t handle = duplicate_handle( current->process, apc->call.map_view.handle,
                                                    process, 0, 0, DUPLICATE_SAME_ACCESS );
            if (handle) apc->call.map_view.handle = handle;
            else
            {
                release_object( process );
                process = NULL;
            }
        }
        break;
    case APC_CREATE_THREAD:
        process = get_process_from_handle( req->handle, PROCESS_CREATE_THREAD );
        break;
    case APC_DUP_HANDLE:
        process = get_process_from_handle( req->handle, PROCESS_DUP_HANDLE );
        if (process && process != current->process)
        {
            /* duplicate the destination process handle into the target process */
            obj_handle_t handle = duplicate_handle( current->process, apc->call.dup_handle.dst_process,
                                                    process, 0, 0, DUPLICATE_SAME_ACCESS );
            if (handle) apc->call.dup_handle.dst_process = handle;
            else
            {
                release_object( process );
                process = NULL;
            }
        }
        break;
    default:
        set_error( STATUS_INVALID_PARAMETER );
        break;
    }

    if (thread)
    {
        if (!queue_apc( NULL, thread, apc )) set_error( STATUS_UNSUCCESSFUL );
        release_object( thread );
    }
    else if (process)
    {
        reply->self = (process == current->process);
        if (!reply->self)
        {
            obj_handle_t handle = alloc_handle( current->process, apc, SYNCHRONIZE, 0 );
            if (handle)
            {
                if (queue_apc( process, NULL, apc ))
                {
                    apc->caller = (struct thread *)grab_object( current );
                    reply->handle = handle;
                }
                else
                {
                    close_handle( current->process, handle );
                    set_error( STATUS_PROCESS_IS_TERMINATING );
                }
            }
        }
        release_object( process );
    }

    release_object( apc );
}

/* Get the result of an APC call */
DECL_HANDLER(get_apc_result)
{
    struct thread_apc *apc;

    if (!(apc = (struct thread_apc *)get_handle_obj( current->process, req->handle,
                                                     0, &thread_apc_ops ))) return;

    if (apc->executed) reply->result = apc->result;
    else set_error( STATUS_PENDING );

    /* close the handle directly to avoid an extra round-trip */
    close_handle( current->process, req->handle );
    release_object( apc );
}

/* retrieve the current context of a thread */
DECL_HANDLER(get_thread_context)
{
    struct context *thread_context = NULL;
    struct thread *thread;
    struct context_data *context;

    if (get_reply_max_size() < 2 * sizeof(struct context_data))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if (req->context)
    {
        if (!(thread_context = (struct context *)get_handle_obj( current->process, req->context,
                                                                 0, &context_ops )))
            return;
        close_handle( current->process, req->context ); /* avoid extra server call */
    }
    else
    {
        if (!(thread = get_thread_from_handle( req->handle, THREAD_GET_CONTEXT ))) return;
        if (req->machine != native_machine && req->machine != thread->process->machine)
            set_error( STATUS_INVALID_PARAMETER );
        else if (thread->state != RUNNING)
            set_error( STATUS_UNSUCCESSFUL );
        else
        {
            reply->self = (thread == current);
            if (thread != current) stop_thread( thread );
            if (thread->context)
            {
                /* make sure that system regs are valid in thread context */
                if (thread->unix_tid != -1 && (system_flags & ~thread->context->regs[CTX_NATIVE].flags))
                    get_thread_context( thread, &thread->context->regs[CTX_NATIVE], system_flags );
                if (!get_error()) thread_context = (struct context *)grab_object( thread->context );
            }
            else if (!get_error() && (context = set_reply_data_size( sizeof(struct context_data) )))
            {
                assert( reply->self );
                memset( context, 0, sizeof(struct context_data) );
                context->machine = native_machine;
                if (system_flags) get_thread_context( thread, context, system_flags );
            }
        }
        release_object( thread );
        if (!thread_context) return;
    }

    if (!thread_context->status)
    {
        unsigned int native_flags = req->flags, wow_flags = 0;

        if (req->machine == thread_context->regs[CTX_WOW].machine)
        {
            native_flags = req->native_flags;
            wow_flags = req->flags & ~native_flags;
        }
        if ((context = set_reply_data_size( (!!native_flags + !!wow_flags) * sizeof(struct context_data) )))
        {
            if (native_flags)
            {
                memset( context, 0, sizeof(*context) );
                context->machine = thread_context->regs[CTX_NATIVE].machine;
                copy_context( context, &thread_context->regs[CTX_NATIVE], native_flags );
                context->flags = native_flags;
                context++;
            }
            if (wow_flags)
            {
                memset( context, 0, sizeof(*context) );
                context->machine = thread_context->regs[CTX_WOW].machine;
                copy_context( context, &thread_context->regs[CTX_WOW], wow_flags );
                context->flags = wow_flags;
            }
        }
    }
    else
    {
        set_error( thread_context->status );
        if (thread_context->status == STATUS_PENDING)
            reply->handle = alloc_handle( current->process, thread_context, SYNCHRONIZE, 0 );
    }

    release_object( thread_context );
}

/* set the current context of a thread */
DECL_HANDLER(set_thread_context)
{
    struct thread *thread;
    const struct context_data *contexts = get_req_data();
    unsigned int ctx_count = get_req_data_size() / sizeof(struct context_data);

    if (!ctx_count || ctx_count > 2 || ctx_count * sizeof(struct context_data) != get_req_data_size())
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if (!(thread = get_thread_from_handle( req->handle, THREAD_SET_CONTEXT ))) return;
    reply->self = (thread == current);

    if (contexts[CTX_NATIVE].machine != native_machine ||
        (ctx_count == 2 && contexts[CTX_WOW].machine != thread->process->machine))
        set_error( STATUS_INVALID_PARAMETER );
    else if (thread->state != TERMINATED)
    {
        unsigned int flags = system_flags & contexts[CTX_NATIVE].flags;

        if (thread != current) stop_thread( thread );
        else if (flags) set_thread_context( thread, &contexts[CTX_NATIVE], flags );

        if (thread->context && !get_error())
        {
            /* If context is in a pending state, we don't know if we will use WoW or native
             * context, so store both and discard irrevelant one in select request. */
            const int is_pending = thread->context->status == STATUS_PENDING;
            unsigned int native_flags = contexts[CTX_NATIVE].flags & ~SERVER_CTX_EXEC_SPACE;

            if (ctx_count == 2 && (is_pending || thread->context->regs[CTX_WOW].machine))
            {
                struct context_data *ctx = &thread->context->regs[CTX_WOW];

                /* some regs are always set from the native context */
                flags = contexts[CTX_WOW].flags & ~(req->native_flags | SERVER_CTX_EXEC_SPACE);
                if (is_pending) ctx->machine = contexts[CTX_WOW].machine;
                else native_flags &= req->native_flags;

                copy_context( ctx, &contexts[CTX_WOW], flags );
                ctx->flags |= flags;
            }

            if (native_flags)
            {
                struct context_data *ctx = &thread->context->regs[CTX_NATIVE];
                copy_context( ctx, &contexts[CTX_NATIVE], native_flags );
                ctx->flags |= native_flags;
            }
        }
    }
    else set_error( STATUS_UNSUCCESSFUL );

    release_object( thread );
}

/* fetch a selector entry for a thread */
DECL_HANDLER(get_selector_entry)
{
    struct thread *thread;
    if ((thread = get_thread_from_handle( req->handle, THREAD_QUERY_INFORMATION )))
    {
        get_selector_entry( thread, req->entry, &reply->base, &reply->limit, &reply->flags );
        release_object( thread );
    }
}

/* Iterate thread list for process. Use global thread list to also
 * return terminated but not yet destroyed threads. */
DECL_HANDLER(get_next_thread)
{
    struct thread *thread;
    struct process *process;
    struct list *ptr;

    if (req->flags > 1)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if (!(process = get_process_from_handle( req->process, PROCESS_QUERY_INFORMATION )))
        return;

    if (!req->last)
    {
        ptr = req->flags ? list_tail( &thread_list ) : list_head( &thread_list );
    }
    else if ((thread = get_thread_from_handle( req->last, 0 )))
    {
        ptr = req->flags ? list_prev( &thread_list, &thread->entry )
                         : list_next( &thread_list, &thread->entry );
        release_object( thread );
    }
    else
    {
        release_object( process );
        return;
    }

    while (ptr)
    {
        thread = LIST_ENTRY( ptr, struct thread, entry );
        if (thread->process == process &&
            (reply->handle = alloc_handle( current->process, thread, req->access, req->attributes )))
        {
            release_object( process );
            return;
        }
        ptr = req->flags ? list_prev( &thread_list, &thread->entry )
                         : list_next( &thread_list, &thread->entry );
    }
    set_error( STATUS_NO_MORE_ENTRIES );
    release_object( process );
}
