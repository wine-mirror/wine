/*
 * Server-side objects
 * These are the server equivalent of K32OBJ
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "winerror.h"
#include "thread.h"

int debug_level = 0;

struct object_name
{
    struct object_name *next;
    struct object_name *prev;
    struct object      *obj;
    size_t              len;
    char                name[1];
};

#define NAME_HASH_SIZE 37

static struct object_name *names[NAME_HASH_SIZE];

#ifdef DEBUG_OBJECTS
static struct object *first;

void dump_objects(void)
{
    struct object *ptr = first;
    while (ptr)
    {
        fprintf( stderr, "%p:%d: ", ptr, ptr->refcount );
        ptr->ops->dump( ptr, 1 );
        ptr = ptr->next;
    }
}
#endif

/*****************************************************************/

/* malloc replacement */
void *mem_alloc( size_t size )
{
    void *ptr = malloc( size );
    if (ptr) memset( ptr, 0x55, size );
    else if (current) set_error( ERROR_OUTOFMEMORY );
    return ptr;
}

/*****************************************************************/

static int get_name_hash( const char *name, size_t len )
{
    char hash = 0;
    while (len--) hash ^= *name++;
    return hash % NAME_HASH_SIZE;
}

/* allocate a name for an object */
static struct object_name *alloc_name( const char *name, size_t len )
{
    struct object_name *ptr;

    if ((ptr = mem_alloc( sizeof(*ptr) + len )))
    {
        ptr->len = len;
        memcpy( ptr->name, name, len );
        ptr->name[len] = 0;
    }
    return ptr;
}

/* free the name of an object */
static void free_name( struct object *obj )
{
    struct object_name *ptr = obj->name;
    if (ptr->next) ptr->next->prev = ptr->prev;
    if (ptr->prev) ptr->prev->next = ptr->next;
    else
    {
        int hash;
        for (hash = 0; hash < NAME_HASH_SIZE; hash++)
            if (names[hash] == ptr)
            {
                names[hash] = ptr->next;
                break;
            }
    }
    free( ptr );
}

/* set the name of an existing object */
static void set_object_name( struct object *obj, struct object_name *ptr )
{
    int hash = get_name_hash( ptr->name, ptr->len );

    if ((ptr->next = names[hash]) != NULL) ptr->next->prev = ptr;
    ptr->obj = obj;
    ptr->prev = NULL;
    names[hash] = ptr;
    assert( !obj->name );
    obj->name = ptr;
}

/* allocate and initialize an object */
void *alloc_object( const struct object_ops *ops )
{
    struct object *obj = mem_alloc( ops->size );
    if (obj)
    {
        obj->refcount = 1;
        obj->ops      = ops;
        obj->head     = NULL;
        obj->tail     = NULL;
        obj->name     = NULL;
#ifdef DEBUG_OBJECTS
        obj->prev = NULL;
        if ((obj->next = first) != NULL) obj->next->prev = obj;
        first = obj;
#endif
    }
    return obj;
}

void *create_named_object( const struct object_ops *ops, const char *name, size_t len )
{
    struct object *obj;
    struct object_name *name_ptr;

    if (!name || !len) return alloc_object( ops );
    if (!(name_ptr = alloc_name( name, len ))) return NULL;

    if ((obj = find_object( name_ptr->name, name_ptr->len )))
    {
        free( name_ptr );  /* we no longer need it */
        if (obj->ops == ops)
        {
            set_error( ERROR_ALREADY_EXISTS );
            return obj;
        }
        set_error( ERROR_INVALID_HANDLE );
        return NULL;
    }
    if ((obj = alloc_object( ops )))
    {
        set_object_name( obj, name_ptr );
        clear_error();
    }
    else free( name_ptr );
    return obj;
}

/* return a pointer to the object name, or to an empty string */
const char *get_object_name( struct object *obj )
{
    if (!obj->name) return "";
    return obj->name->name;
}

/* grab an object (i.e. increment its refcount) and return the object */
struct object *grab_object( void *ptr )
{
    struct object *obj = (struct object *)ptr;
    assert( obj->refcount < INT_MAX );
    obj->refcount++;
    return obj;
}

/* release an object (i.e. decrement its refcount) */
void release_object( void *ptr )
{
    struct object *obj = (struct object *)ptr;
    assert( obj->refcount );
    if (!--obj->refcount)
    {
        /* if the refcount is 0, nobody can be in the wait queue */
        assert( !obj->head );
        assert( !obj->tail );
        if (obj->name) free_name( obj );
#ifdef DEBUG_OBJECTS
        if (obj->next) obj->next->prev = obj->prev;
        if (obj->prev) obj->prev->next = obj->next;
        else first = obj->next;
#endif
        obj->ops->destroy( obj );
        memset( obj, 0xaa, obj->ops->size );
        free( obj );
    }
}

/* find an object by its name; the refcount is incremented */
struct object *find_object( const char *name, size_t len )
{
    struct object_name *ptr;
    if (!name || !len) return NULL;
    for (ptr = names[ get_name_hash( name, len ) ]; ptr; ptr = ptr->next)
    {
        if (ptr->len != len) continue;
        if (!memcmp( ptr->name, name, len )) return grab_object( ptr->obj );
    }
    return NULL;
}

/* functions for unimplemented object operations */

int no_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    set_error( ERROR_INVALID_HANDLE );
    return 0;
}

int no_satisfied( struct object *obj, struct thread *thread )
{
    return 0;  /* not abandoned */
}

int no_read_fd( struct object *obj )
{
    set_error( ERROR_INVALID_HANDLE );
    return -1;
}

int no_write_fd( struct object *obj )
{
    set_error( ERROR_INVALID_HANDLE );
    return -1;
}

int no_flush( struct object *obj )
{
    set_error( ERROR_INVALID_HANDLE );
    return 0;
}

int no_get_file_info( struct object *obj, struct get_file_info_reply *info )
{
    set_error( ERROR_INVALID_HANDLE );
    return 0;
}

void no_destroy( struct object *obj )
{
}

void default_select_event( int event, void *private )
{
    struct object *obj = (struct object *)private;
    assert( obj );
    wake_up( obj, 0 );
}
