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
struct process;
struct file;
struct wait_queue_entry;

/* operations valid on all objects */
struct object_ops
{
    /* dump the object (for debugging) */
    void (*dump)(struct object *,int);
    /* add a thread to the object wait queue */
    int  (*add_queue)(struct object *,struct wait_queue_entry *);
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
    /* get file information */
    int  (*get_file_info)(struct object *,struct get_file_info_reply *);
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
extern int init_object( struct object *obj, const struct object_ops *ops, const char *name );
extern const char *get_object_name( struct object *obj );
/* grab/release_object can take any pointer, but you better make sure */
/* that the thing pointed to starts with a struct object... */
extern struct object *grab_object( void *obj );
extern void release_object( void *obj );
extern struct object *find_object( const char *name );
extern int no_add_queue( struct object *obj, struct wait_queue_entry *entry );
extern int no_satisfied( struct object *obj, struct thread *thread );
extern int no_read_fd( struct object *obj );
extern int no_write_fd( struct object *obj );
extern int no_flush( struct object *obj );
extern int no_get_file_info( struct object *obj, struct get_file_info_reply *info );
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

extern struct object *create_file( int fd, const char *name, unsigned int access,
                                   unsigned int sharing, int create, unsigned int attrs );
extern struct file *get_file_obj( struct process *process, int handle,
                                  unsigned int access );
extern int file_get_mmap_fd( struct file *file );
extern int set_file_pointer( int handle, int *low, int *high, int whence );
extern int truncate_file( int handle );
extern int set_file_time( int handle, time_t access_time, time_t write_time );
extern int file_lock( struct file *file, int offset_high, int offset_low,
                      int count_high, int count_low );
extern int file_unlock( struct file *file, int offset_high, int offset_low,
                        int count_high, int count_low );
extern void file_set_error(void);


/* pipe functions */

extern int create_pipe( struct object *obj[2] );


/* console functions */

struct tagINPUT_RECORD;
extern int create_console( int fd, struct object *obj[2] );
extern int set_console_fd( int handle, int fd, int pid );
extern int get_console_mode( int handle, int *mode );
extern int set_console_mode( int handle, int mode );
extern int set_console_info( int handle, struct set_console_info_request *req,
                             const char *title );
extern int get_console_info( int handle, struct get_console_info_reply *reply,
                             const char **title );
extern int write_console_input( int handle, int count, struct tagINPUT_RECORD *records );
extern int read_console_input( int handle, int count, int flush );


/* change notification functions */

extern struct object *create_change_notification( int subtree, int filter );


/* file mapping functions */
extern struct object *create_mapping( int size_high, int size_low, int protect,
                                      int handle, const char *name );
extern int open_mapping( unsigned int access, int inherit, const char *name );
extern int get_mapping_info( int handle, struct get_mapping_info_reply *reply );


/* device functions */
extern struct object *create_device( int id );


/* snapshot functions */
extern struct object *create_snapshot( int flags );
extern int snapshot_next_process( int handle, int reset, struct next_process_reply *reply );

extern int debug_level;

#endif  /* __WINE_SERVER_OBJECT_H */
