/*
 * Wine server threads
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#ifndef __WINE_SERVER_THREAD_H
#define __WINE_SERVER_THREAD_H

#ifndef __WINE_SERVER__
#error This file can only be used in the Wine server
#endif

#include "object.h"

/* thread structure */

struct process;
struct thread_wait;
struct thread_apc;
struct mutex;
struct debug_ctx;
struct debug_event;

enum run_state
{
    RUNNING,    /* running normally */
    SLEEPING,   /* sleeping waiting for a request to terminate */
    TERMINATED  /* terminated */
};


struct thread
{
    struct object       obj;       /* object header */
    struct thread      *next;      /* system-wide thread list */
    struct thread      *prev;
    struct thread      *proc_next; /* per-process thread list */
    struct thread      *proc_prev;
    struct process     *process;
    struct mutex       *mutex;       /* list of currently owned mutexes */
    struct debug_ctx   *debug_ctx;   /* debugger context if this thread is a debugger */
    struct process     *debug_first; /* head of debugged processes list */
    struct debug_event *debug_event; /* pending debug event for this thread */
    struct thread_wait *wait;      /* current wait condition if sleeping */
    struct thread_apc  *apc;       /* list of async procedure calls */
    int                 apc_count; /* number of outstanding APCs */
    int                 error;     /* current error code */
    enum run_state      state;     /* running state */
    int                 attached;  /* is thread attached with ptrace? */
    int                 exit_code; /* thread exit code */
    struct client      *client;    /* client for socket communications */
    int                 unix_pid;  /* Unix pid of client */
    void               *teb;       /* TEB address (in client address space) */
    int                 priority;  /* priority level */
    int                 affinity;  /* affinity mask */
    int                 suspend;   /* suspend count */
    void               *buffer;    /* buffer for communication with the client */
    enum request        last_req;  /* last request received (for debugging) */
};

extern struct thread *current;

/* thread functions */

extern void create_initial_thread( int fd );
extern struct thread *get_thread_from_id( void *id );
extern struct thread *get_thread_from_handle( int handle, unsigned int access );
extern void wait4_thread( struct thread *thread, int wait );
extern void stop_thread( struct thread *thread );
extern void continue_thread( struct thread *thread );
extern int suspend_thread( struct thread *thread, int check_limit );
extern int resume_thread( struct thread *thread );
extern void suspend_all_threads( void );
extern void resume_all_threads( void );
extern int add_queue( struct object *obj, struct wait_queue_entry *entry );
extern void remove_queue( struct object *obj, struct wait_queue_entry *entry );
extern void kill_thread( struct thread *thread, int exit_code );
extern void thread_killed( struct thread *thread, int exit_code );
extern void thread_timeout(void);
extern void wake_up( struct object *obj, int max );

static inline int get_error(void)       { return current->error; }
static inline void set_error( int err ) { current->error = err; }
static inline void clear_error(void)    { set_error(0); }

#endif  /* __WINE_SERVER_THREAD_H */
