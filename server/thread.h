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

enum run_state
{
    RUNNING,    /* running normally */
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
    struct thread_wait *wait;      /* current wait condition if sleeping */
    struct thread_apc  *apc;       /* list of async procedure calls */
    int                 apc_count; /* number of outstanding APCs */
    int                 error;     /* current error code */
    int                 pass_fd;   /* fd to pass to the client */
    enum run_state      state;     /* running state */
    int                 attached;  /* is thread attached with ptrace? */
    int                 exit_code; /* thread exit code */
    int                 unix_pid;  /* Unix pid of client */
    CONTEXT            *context;   /* current context if in an exception handler */
    void               *teb;       /* TEB address (in client address space) */
    void               *entry;     /* thread entry point (in client address space) */
    int                 priority;  /* priority level */
    int                 affinity;  /* affinity mask */
    int                 suspend;   /* suspend count */
    void               *buffer;    /* buffer for communication with the client */
    enum request        last_req;  /* last request received (for debugging) */
};

/* callback function for building the thread reply when sleep_on is finished */
typedef void (*sleep_reply)( struct thread *thread, struct object *obj, int signaled );

extern struct thread *current;

/* thread functions */

extern struct thread *create_thread( int fd, struct process *process, int suspend );
extern struct thread *get_thread_from_id( void *id );
extern struct thread *get_thread_from_handle( int handle, unsigned int access );
extern struct thread *get_thread_from_pid( int pid );
extern int suspend_thread( struct thread *thread, int check_limit );
extern int resume_thread( struct thread *thread );
extern void suspend_all_threads( void );
extern void resume_all_threads( void );
extern int add_queue( struct object *obj, struct wait_queue_entry *entry );
extern void remove_queue( struct object *obj, struct wait_queue_entry *entry );
extern void kill_thread( struct thread *thread, int violent_death );
extern void wake_up( struct object *obj, int max );
extern int sleep_on( int count, struct object *objects[], int flags,
                     int timeout, sleep_reply func );

/* ptrace functions */

extern void sigchld_handler();
extern void wait4_thread( struct thread *thread, int signal );
extern void stop_thread( struct thread *thread );
extern void continue_thread( struct thread *thread );
extern void detach_thread( struct thread *thread, int sig );
extern int suspend_for_ptrace( struct thread *thread );
extern int read_thread_int( struct thread *thread, const int *addr, int *data );
extern int write_thread_int( struct thread *thread, int *addr, int data, unsigned int mask );


static inline int get_error(void)       { return current->error; }
static inline void set_error( int err ) { current->error = err; }
static inline void clear_error(void)    { set_error(0); }

static inline void *get_thread_id( struct thread *thread ) { return thread; }

#endif  /* __WINE_SERVER_THREAD_H */
