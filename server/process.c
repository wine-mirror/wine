/*
 * Server-side process management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <stdlib.h>

#include "server.h"
#include "object.h"


/* process structure; not much for now... */

struct process
{
    struct object   obj;       /* object header */
    struct process *next;      /* system-wide process list */
    struct process *prev;
};

static struct process *first_process;

/* process operations */

static void destroy_process( struct object *obj );

static const struct object_ops process_ops =
{
    destroy_process
};

/* create a new process */
struct process *create_process(void)
{
    struct process *process;

    if (!(process = malloc( sizeof(*process) ))) return NULL;
    init_object( &process->obj, &process_ops, NULL );
    process->next = first_process;
    process->prev = NULL;
    first_process = process;
    return process;
}

/* destroy a process when its refcount is 0 */
static void destroy_process( struct object *obj )
{
    struct process *process = (struct process *)obj;
    assert( obj->ops == &process_ops );

    if (process->next) process->next->prev = process->prev;
    if (process->prev) process->prev->next = process->next;
    else first_process = process->next;
    free( process );
}

/* get a process from an id (and increment the refcount) */
struct process *get_process_from_id( void *id )
{
    struct process *p = first_process;
    while (p && (p != id)) p = p->next;
    if (p) p->obj.refcount++;
    return p;
}
