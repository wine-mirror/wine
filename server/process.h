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

struct process;
struct handle_table;

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
extern struct handle_table *get_process_handles( struct process *process );
extern void add_process_thread( struct process *process,
                                struct thread *thread );
extern void remove_process_thread( struct process *process,
                                   struct thread *thread );
extern int alloc_console( struct process *process );
extern int free_console( struct process *process );
extern struct object *get_console( struct process *process, int output );
extern struct process_snapshot *process_snap( int *count );

#endif  /* __WINE_SERVER_PROCESS_H */
