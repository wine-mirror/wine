/*
 * Wine server objects
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#ifndef __WINE_SERVER_OBJECT_H
#define __WINE_SERVER_OBJECT_H

#ifndef __WINE_SERVER__
#error This file can only be used in the Wine server
#endif

#include <sys/time.h>
#include "server/request.h"

/* kernel objects */

struct object;
struct object_name;

struct object_ops
{
    void (*dump)(struct object *,int);   /* dump the object (for debugging) */
    void (*destroy)(struct object *);    /* destroy on refcount == 0 */
};

struct object
{
    unsigned int              refcount;
    const struct object_ops  *ops;
    struct object_name       *name;
};

extern void init_object( struct object *obj, const struct object_ops *ops,
                         const char *name );
/* grab/release_object can take any pointer, but you better make sure */
/* that the thing pointed to starts with a struct object... */
extern struct object *grab_object( void *obj );
extern void release_object( void *obj );

/* request handlers */

struct iovec;
struct thread;

extern void call_req_handler( struct thread *thread, enum request req,
                              void *data, int len, int fd );
extern void call_timeout_handler( struct thread *thread );
extern void call_kill_handler( struct thread *thread, int exit_code );

extern void trace_request( enum request req, void *data, int len, int fd );
extern void trace_timeout(void);
extern void trace_kill( int exit_code );
extern void trace_reply( struct thread *thread, int type, int pass_fd,
                         struct iovec *vec, int veclen );

/* socket functions */

extern int add_client( int client_fd, struct thread *self );
extern void remove_client( int client_fd, int exit_code );
extern int get_initial_client_fd(void);
extern void set_timeout( int client_fd, struct timeval *when );
extern int send_reply_v( int client_fd, int type, int pass_fd,
                         struct iovec *vec, int veclen );

/* process functions */

struct process;

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
                             int dst_handle, unsigned int access, int inherit, int options );

extern int debug_level;

#endif  /* __WINE_SERVER_OBJECT_H */
