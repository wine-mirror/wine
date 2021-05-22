/*
 * Wine server threads
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

#ifndef __WINE_SERVER_THREAD_H
#define __WINE_SERVER_THREAD_H

#include "object.h"

/* thread structure */

struct process;
struct thread_wait;
struct thread_apc;
struct debug_obj;
struct debug_event;
struct msg_queue;

enum run_state
{
    RUNNING,    /* running normally */
    TERMINATED  /* terminated */
};

/* descriptor for fds currently in flight from client to server */
struct inflight_fd
{
    int client;  /* fd on the client side (or -1 if entry is free) */
    int server;  /* fd on the server side */
};
#define MAX_INFLIGHT_FDS 16  /* max number of fds in flight per thread */

struct thread
{
    struct object          obj;           /* object header */
    struct list            entry;         /* entry in system-wide thread list */
    struct list            proc_entry;    /* entry in per-process thread list */
    struct process        *process;
    thread_id_t            id;            /* thread id */
    struct list            mutex_list;    /* list of currently owned mutexes */
    unsigned int           system_regs;   /* which system regs have been set */
    struct msg_queue      *queue;         /* message queue */
    struct thread_wait    *wait;          /* current wait condition if sleeping */
    struct list            system_apc;    /* queue of system async procedure calls */
    struct list            user_apc;      /* queue of user async procedure calls */
    struct inflight_fd     inflight[MAX_INFLIGHT_FDS];  /* fds currently in flight */
    unsigned int           error;         /* current error code */
    union generic_request  req;           /* current request */
    void                  *req_data;      /* variable-size data for request */
    unsigned int           req_toread;    /* amount of data still to read in request */
    void                  *reply_data;    /* variable-size data for reply */
    unsigned int           reply_size;    /* size of reply data */
    unsigned int           reply_towrite; /* amount of data still to write in reply */
    struct fd             *request_fd;    /* fd for receiving client requests */
    struct fd             *reply_fd;      /* fd to send a reply to a client */
    struct fd             *wait_fd;       /* fd to use to wake a sleeping client */
    enum run_state         state;         /* running state */
    int                    exit_code;     /* thread exit code */
    int                    unix_pid;      /* Unix pid of client */
    int                    unix_tid;      /* Unix tid of client */
    struct context        *context;       /* current context */
    client_ptr_t           suspend_cookie;/* wait cookie of suspending select */
    client_ptr_t           teb;           /* TEB address (in client address space) */
    client_ptr_t           entry_point;   /* entry point (in client address space) */
    affinity_t             affinity;      /* affinity mask */
    int                    priority;      /* priority level */
    int                    suspend;       /* suspend count */
    int                    dbg_hidden;    /* hidden from debugger */
    obj_handle_t           desktop;       /* desktop handle */
    int                    desktop_users; /* number of objects using the thread desktop */
    timeout_t              creation_time; /* Thread creation time */
    timeout_t              exit_time;     /* Thread exit time */
    struct token          *token;         /* security token associated with this thread */
    struct list            kernel_object; /* list of kernel object pointers */
    data_size_t            desc_len;      /* thread description length in bytes */
    WCHAR                 *desc;          /* thread description string */
};

extern struct thread *current;

/* thread functions */

extern struct thread *create_thread( int fd, struct process *process,
                                     const struct security_descriptor *sd );
extern struct thread *get_thread_from_id( thread_id_t id );
extern struct thread *get_thread_from_handle( obj_handle_t handle, unsigned int access );
extern struct thread *get_thread_from_tid( int tid );
extern struct thread *get_thread_from_pid( int pid );
extern struct thread *get_wait_queue_thread( struct wait_queue_entry *entry );
extern enum select_op get_wait_queue_select_op( struct wait_queue_entry *entry );
extern client_ptr_t get_wait_queue_key( struct wait_queue_entry *entry );
extern void make_wait_abandoned( struct wait_queue_entry *entry );
extern void set_wait_status( struct wait_queue_entry *entry, int status );
extern void stop_thread( struct thread *thread );
extern int wake_thread( struct thread *thread );
extern int wake_thread_queue_entry( struct wait_queue_entry *entry );
extern int add_queue( struct object *obj, struct wait_queue_entry *entry );
extern void remove_queue( struct object *obj, struct wait_queue_entry *entry );
extern void kill_thread( struct thread *thread, int violent_death );
extern void wake_up( struct object *obj, int max );
extern int thread_queue_apc( struct process *process, struct thread *thread, struct object *owner, const apc_call_t *call_data );
extern void thread_cancel_apc( struct thread *thread, struct object *owner, enum apc_type type );
extern int thread_add_inflight_fd( struct thread *thread, int client, int server );
extern int thread_get_inflight_fd( struct thread *thread, int client );
extern struct token *thread_get_impersonation_token( struct thread *thread );
extern int set_thread_affinity( struct thread *thread, affinity_t affinity );
extern int suspend_thread( struct thread *thread );
extern int resume_thread( struct thread *thread );

/* ptrace functions */

extern void sigchld_callback(void);
extern void init_thread_context( struct thread *thread );
extern void get_thread_context( struct thread *thread, context_t *context, unsigned int flags );
extern void set_thread_context( struct thread *thread, const context_t *context, unsigned int flags );
extern int send_thread_signal( struct thread *thread, int sig );
extern void get_selector_entry( struct thread *thread, int entry, unsigned int *base,
                                unsigned int *limit, unsigned char *flags );

extern unsigned int global_error;  /* global error code for when no thread is current */

static inline unsigned int get_error(void)       { return current ? current->error : global_error; }
static inline void set_error( unsigned int err ) { global_error = err; if (current) current->error = err; }
static inline void clear_error(void)             { set_error(0); }
static inline void set_win32_error( unsigned int err ) { set_error( 0xc0010000 | err ); }

static inline thread_id_t get_thread_id( struct thread *thread ) { return thread->id; }

#endif  /* __WINE_SERVER_THREAD_H */
