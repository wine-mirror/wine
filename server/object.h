/*
 * Wine server objects
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#ifndef __WINE_OBJECT_H
#define __WINE_OBJECT_H

#include <sys/time.h>

struct object;
struct object_name;

struct object_ops
{
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
/* release object can take any pointer, but you better make sure that */
/* the thing pointed to starts with a struct object... */
extern void release_object( void *obj );

/* request handlers */

struct thread;
typedef void (*req_handler)( void *data, int len, int fd, struct thread *self);
extern const req_handler req_handlers[REQ_NB_REQUESTS];

/* socket functions */

extern int add_client( int client_fd, struct thread *self );
extern void remove_client( int client_fd );
extern int get_initial_client_fd(void);
extern void set_timeout( int client_fd, struct timeval *when );
extern int send_reply( int client_fd, int err_code, int pass_fd,
                       int n, ... /* arg_1, len_1, ..., arg_n, len_n */ );

/* process functions */

struct process;

extern struct process *create_process(void);
extern struct process *get_process_from_id( void *id );

#endif  /* __WINE_OBJECT_H */
