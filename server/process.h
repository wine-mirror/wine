/*
 * Wine server processes
 *
 * Copyright (C) 1999 Alexandre Julliard
 */

#ifndef __WINE_SERVER_PROCESS_H
#define __WINE_SERVER_PROCESS_H

#ifndef __WINE_SERVER__
#error This file can only be used in the Wine server
#endif

#include "object.h"

/* process structures */

struct handle_entry;

struct process
{
    struct object        obj;             /* object header */
    struct process      *next;            /* system-wide process list */
    struct process      *prev;
    struct thread       *thread_list;     /* head of the thread list */
    struct process      *debug_next;      /* per-debugger process list */
    struct process      *debug_prev;
    struct thread       *debugger;        /* thread debugging this process */
    struct handle_entry *entries;         /* handle entries */
    int                  handle_last;     /* last valid handle */
    int                  handle_count;    /* allocated table entries */
    int                  exit_code;       /* process exit code */
    int                  running_threads; /* number of threads running in this process */
    struct timeval       start_time;      /* absolute time at process start */
    struct timeval       end_time;        /* absolute time at process end */
    int                  priority;        /* priority class */
    int                  affinity;        /* process affinity mask */
    int                  suspend;         /* global process suspend count */
    struct object       *console_in;      /* console input */
    struct object       *console_out;     /* console output */
    struct new_process_request *info;     /* startup info (freed after startup) */
};

struct process_snapshot
{
    struct process *process;  /* process ptr */
    struct process *parent;   /* process parent */
    int             threads;  /* number of threads */
    int             priority; /* priority class */
};

/* process functions */

extern struct process *create_initial_process(void);
extern struct process *get_process_from_id( void *id );
extern struct process *get_process_from_handle( int handle, unsigned int access );
extern int process_set_debugger( struct process *process, struct thread *thread );
extern void add_process_thread( struct process *process,
                                struct thread *thread );
extern void remove_process_thread( struct process *process,
                                   struct thread *thread );
extern void suspend_process( struct process *process );
extern void resume_process( struct process *process );
extern void kill_process( struct process *process, int exit_code );
extern int alloc_console( struct process *process );
extern int free_console( struct process *process );
extern struct object *get_console( struct process *process, int output );
extern struct process_snapshot *process_snap( int *count );

#endif  /* __WINE_SERVER_PROCESS_H */
