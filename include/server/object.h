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
#include "server.h"
#include "server/request.h"

/* kernel objects */

struct object;
struct object_name;
struct thread;
struct wait_queue_entry;

struct object_ops
{
    /* dump the object (for debugging) */
    void (*dump)(struct object *,int);
    /* add a thread to the object wait queue */
    void (*add_queue)(struct object *,struct wait_queue_entry *);
    /* remove a thread from the object wait queue */
    void (*remove_queue)(struct object *,struct wait_queue_entry *);
    /* is object signaled? */
    int  (*signaled)(struct object *,struct thread *);
    /* wait satisfied; return 1 if abandoned */
    int  (*satisfied)(struct object *,struct thread *);
    /* return a Unix fd that can be used to read from the object */
    int  (*get_read_fd)(struct object *);
    /* return a Unix fd that can be used to write to the object */
    int  (*get_write_fd)(struct object *);
    /* flush the object buffers */
    int  (*flush)(struct object *); 
    /* destroy on refcount == 0 */
    void (*destroy)(struct object *);
};

struct object
{
    unsigned int              refcount;
    const struct object_ops  *ops;
    struct wait_queue_entry  *head;
    struct wait_queue_entry  *tail;
    struct object_name       *name;
};

extern void *mem_alloc( size_t size );  /* malloc wrapper */
extern struct object *create_named_object( const char *name, const struct object_ops *ops,
                                           size_t size );
extern int init_object( struct object *obj, const struct object_ops *ops,
                        const char *name );
/* grab/release_object can take any pointer, but you better make sure */
/* that the thing pointed to starts with a struct object... */
extern struct object *grab_object( void *obj );
extern void release_object( void *obj );
extern struct object *find_object( const char *name );
extern int no_satisfied( struct object *obj, struct thread *thread );
extern int no_read_fd( struct object *obj );
extern int no_write_fd( struct object *obj );
extern int no_flush( struct object *obj );
extern void default_select_event( int fd, int event, void *private );

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

/* select functions */

#define READ_EVENT    1
#define WRITE_EVENT   2

struct select_ops
{
    void (*event)( int fd, int event, void *private );
    void (*timeout)( int fd, void *private );
};

extern int add_select_user( int fd, int events, const struct select_ops *ops, void *private );
extern void remove_select_user( int fd );
extern void set_select_timeout( int fd, struct timeval *when );
extern void set_select_events( int fd, int events );
extern void *get_select_private_data( const struct select_ops *ops, int fd );
extern void select_loop(void);

/* socket functions */

extern void server_init( int fd );
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
extern int open_object( const char *name, const struct object_ops *ops,
                        unsigned int access, int inherit );

/* event functions */

extern struct object *create_event( const char *name, int manual_reset, int initial_state );
extern int open_event( unsigned int access, int inherit, const char *name );
extern int pulse_event( int handle );
extern int set_event( int handle );
extern int reset_event( int handle );


/* mutex functions */

extern struct object *create_mutex( const char *name, int owned );
extern int open_mutex( unsigned int access, int inherit, const char *name );
extern int release_mutex( int handle );
extern void abandon_mutexes( struct thread *thread );


/* semaphore functions */

extern struct object *create_semaphore( const char *name, unsigned int initial, unsigned int max );
extern int open_semaphore( unsigned int access, int inherit, const char *name );
extern int release_semaphore( int handle, unsigned int count, unsigned int *prev_count );


/* file functions */

extern struct object *create_file( int fd );
extern int file_get_unix_handle( int handle, unsigned int access );
extern int set_file_pointer( int handle, int *low, int *high, int whence );
extern int truncate_file( int handle );
extern int get_file_info( int handle, struct get_file_info_reply *reply );
extern void file_set_error(void);


/* pipe functions */

extern int create_pipe( struct object *obj[2] );


/* console functions */

extern int create_console( int fd, struct object *obj[2] );
extern int set_console_fd( int handle, int fd );


/* change notification functions */

extern struct object *create_change_notification( int subtree, int filter );


extern int debug_level;

#endif  /* __WINE_SERVER_OBJECT_H */
