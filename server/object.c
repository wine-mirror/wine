/*
 * Server-side objects
 * These are the server equivalent of K32OBJ
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "winerror.h"
#include "server.h"
#include "server/thread.h"

int debug_level = 0;

struct object_name
{
    struct object_name *next;
    struct object      *obj;
    int                 len;
    char                name[1];
};

#define NAME_HASH_SIZE 37

static struct object_name *names[NAME_HASH_SIZE];

/*****************************************************************/

void *mem_alloc( size_t size )
{
    void *ptr = malloc( size );
    if (ptr) memset( ptr, 0x55, size );
    else if (current) SET_ERROR( ERROR_OUTOFMEMORY );
    return ptr;
}

/*****************************************************************/

static int get_name_hash( const char *name )
{
    int hash = 0;
    while (*name) hash ^= *name++;
    return hash % NAME_HASH_SIZE;
}

static struct object_name *add_name( struct object *obj, const char *name )
{
    struct object_name *ptr;
    int hash = get_name_hash( name );
    int len = strlen( name );

    if (!(ptr = (struct object_name *)mem_alloc( sizeof(*ptr) + len )))
        return NULL;
    ptr->next = names[hash];
    ptr->obj  = obj;
    ptr->len  = len;
    strcpy( ptr->name, name );
    names[hash] = ptr;
    return ptr;
}

static void free_name( struct object *obj )
{
    int hash = get_name_hash( obj->name->name );
    struct object_name **pptr = &names[hash];
    while (*pptr && *pptr != obj->name) pptr = &(*pptr)->next;
    assert( *pptr );
    *pptr = (*pptr)->next;
    free( obj->name );
}

/* initialize an already allocated object */
/* return 1 if OK, 0 on error */
int init_object( struct object *obj, const struct object_ops *ops,
                 const char *name )
{
    obj->refcount = 1;
    obj->ops      = ops;
    obj->head     = NULL;
    obj->tail     = NULL;
    if (!name) obj->name = NULL;
    else if (!(obj->name = add_name( obj, name ))) return 0;
    return 1;
}

struct object *create_named_object( const char *name, const struct object_ops *ops, size_t size )
{
    struct object *obj;
    if ((obj = find_object( name )))
    {
        if (obj->ops == ops)
        {
            SET_ERROR( ERROR_ALREADY_EXISTS );
            return obj;
        }
        SET_ERROR( ERROR_INVALID_HANDLE );
        return NULL;
    }
    if (!(obj = mem_alloc( size ))) return NULL;
    if (!init_object( obj, ops, name ))
    {
        free( obj );
        return NULL;
    }
    CLEAR_ERROR();
    return obj;
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
        obj->ops->destroy( obj );
    }
}

/* find an object by its name; the refcount is incremented */
struct object *find_object( const char *name )
{
    struct object_name *ptr;
    if (!name) return NULL;
    ptr = names[ get_name_hash( name ) ];
    while (ptr && strcmp( ptr->name, name )) ptr = ptr->next;
    if (!ptr) return NULL;
    return grab_object( ptr->obj );
}
