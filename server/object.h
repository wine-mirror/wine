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
extern void default_select_event( int event, void *private );

/* request handlers */

struct iovec;
struct thread;

extern void fatal_protocol_error( const char *err, ... );
extern void call_req_handler( struct thread *thread, enum request req,
                              void *data, int len, int fd );
extern void call_timeout_handler( void *thread );
extern void call_kill_handler( struct thread *thread, int exit_code );

extern void trace_request( enum request req, void *data, int len, int fd );
extern void trace_timeout(void);
extern void trace_kill( int exit_code );
extern void trace_reply( struct thread *thread, int type, int pass_fd,
                         struct iovec *vec, int veclen );
/* check that the string is NULL-terminated and that the len is correct */
#define CHECK_STRING(func,str,len) \
  do { if (((str)[(len)-1] || strlen(str) != (len)-1)) \
         fatal_protocol_error( "%s: invalid string '%.*s'\n", (func), (len), (str) ); \
     } while(0)

/* select functions */

#define READ_EVENT    1
#define WRITE_EVENT   2

struct select_user
{
    int    fd;                              /* user fd */
    void (*func)(int event, void *private); /* callback function */
    void  *private;                         /* callback private data */
};

extern void register_select_user( struct select_user *user );
extern void unregister_select_user( struct select_user *user );
extern void set_select_events( struct select_user *user, int events );
extern int check_select_events( struct select_user *user, int events );
extern void select_loop(void);

/* timeout functions */

struct timeout_user;

typedef void (*timeout_callback)( void *private );

extern struct timeout_user *add_timeout_user( struct timeval *when,
                                              timeout_callback func, void *private );
extern void remove_timeout_user( struct timeout_user *user );
extern void make_timeout( struct timeval *when, int timeout );

/* socket functions */

struct client;

extern struct client *add_client( int client_fd, struct thread *self );
extern void remove_client( struct client *client, int exit_code );
extern int send_reply_v( struct client *client, int type, int pass_fd,
                         struct iovec *vec, int veclen );

/* mutex functions */

extern void abandon_mutexes( struct thread *thread );

/* file functions */

extern struct file *get_file_obj( struct process *process, int handle,
                                  unsigned int access );
extern int file_get_mmap_fd( struct file *file );
extern int grow_file( struct file *file, int size_high, int size_low );
extern struct file *create_temp_file( int access );
extern void file_set_error(void);

/* console functions */

extern int create_console( int fd, struct object *obj[2] );

extern int debug_level;

#endif  /* __WINE_SERVER_OBJECT_H */
