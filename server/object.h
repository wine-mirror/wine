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

#define DEBUG_OBJECTS

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
    /* size of this object type */
    size_t size;
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
    int  (*get_file_info)(struct object *,struct get_file_info_request *);
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
#ifdef DEBUG_OBJECTS
    struct object            *prev;
    struct object            *next;
#endif
};

extern void *mem_alloc( size_t size );  /* malloc wrapper */
extern char *mem_strdup( const char *str );
extern void *alloc_object( const struct object_ops *ops );
extern const char *get_object_name( struct object *obj );
extern void *create_named_object( const struct object_ops *ops, const char *name, size_t len );
/* grab/release_object can take any pointer, but you better make sure */
/* that the thing pointed to starts with a struct object... */
extern struct object *grab_object( void *obj );
extern void release_object( void *obj );
extern struct object *find_object( const char *name, size_t len );
extern int no_add_queue( struct object *obj, struct wait_queue_entry *entry );
extern int no_satisfied( struct object *obj, struct thread *thread );
extern int no_read_fd( struct object *obj );
extern int no_write_fd( struct object *obj );
extern int no_flush( struct object *obj );
extern int no_get_file_info( struct object *obj, struct get_file_info_request *info );
extern void no_destroy( struct object *obj );
extern void default_select_event( int event, void *private );
#ifdef DEBUG_OBJECTS
extern void dump_objects(void);
#endif

/* select functions */

#define READ_EVENT    1
#define WRITE_EVENT   2
#define EXCEPT_EVENT  4

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
extern void client_pass_fd( struct client *client, int pass_fd );
extern void client_reply( struct client *client, unsigned int res );

/* event functions */

struct event;

extern struct event *get_event_obj( struct process *process, int handle, unsigned int access );
extern void pulse_event( struct event *event );
extern void set_event( struct event *event );
extern void reset_event( struct event *event );

/* mutex functions */

extern void abandon_mutexes( struct thread *thread );

/* file functions */

extern struct file *get_file_obj( struct process *process, int handle,
                                  unsigned int access );
extern int file_get_mmap_fd( struct file *file );
extern int grow_file( struct file *file, int size_high, int size_low );
extern int create_anonymous_file(void);
extern struct file *create_temp_file( int access );
extern void file_set_error(void);

/* console functions */

extern int alloc_console( struct process *process );
extern int free_console( struct process *process );

/* debugger functions */

extern int debugger_attach( struct process *process, struct thread *debugger );
extern void debug_exit_thread( struct thread *thread, int exit_code );

/* mapping functions */

extern int get_page_size(void);

extern int debug_level;

#endif  /* __WINE_SERVER_OBJECT_H */
