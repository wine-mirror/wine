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

#include "server/object.h"

/* process structures */

struct process;

struct process_snapshot
{
    struct process *process;  /* process ptr */
    struct process *parent;   /* process parent */
    int             threads;  /* number of threads */
    int             priority; /* priority class */
};

/* process functions */

extern struct process *create_process(void);
extern struct process *get_process_from_id( void *id );
extern struct process *get_process_from_handle( int handle, unsigned int access );
extern void add_process_thread( struct process *process,
                                struct thread *thread );
extern void remove_process_thread( struct process *process,
                                   struct thread *thread );
extern void kill_process( struct process *process, int exit_code );
extern void get_process_info( struct process *process,
                              struct get_process_info_reply *reply );
extern void set_process_info( struct process *process,
                              struct set_process_info_request *req );
extern int alloc_console( struct process *process );
extern int free_console( struct process *process );
extern struct object *get_console( struct process *process, int output );
extern struct process_snapshot *process_snap( int *count );

/* handle functions */

/* alloc_handle takes a void *obj for convenience, but you better make sure */
/* that the thing pointed to starts with a struct object... */
extern int alloc_handle( struct process *process, void *obj,
                         unsigned int access, int inherit );
extern int close_handle( struct process *process, int handle );
extern int set_handle_info( struct process *process, int handle,
                            int mask, int flags );
extern struct object *get_handle_obj( struct process *process, int handle,
                                      unsigned int access, const struct object_ops *ops );
extern int duplicate_handle( struct process *src, int src_handle, struct process *dst,
                             unsigned int access, int inherit, int options );
extern int open_object( const char *name, const struct object_ops *ops,
                        unsigned int access, int inherit );

#endif  /* __WINE_SERVER_PROCESS_H */
