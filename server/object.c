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

#include "server.h"

#include "server/object.h"

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

    if (!(ptr = (struct object_name *)malloc( sizeof(*ptr) + len )))
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
void init_object( struct object *obj, const struct object_ops *ops,
                  const char *name )
{
    obj->refcount = 1;
    obj->ops      = ops;
    if (!name) obj->name = NULL;
    else obj->name = add_name( obj, name );
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
        if (obj->name) free_name( obj );
        obj->ops->destroy( obj );
    }
}

/* find an object by its name; the refcount is incremented */
struct object *find_object( const char *name )
{
    int hash = get_name_hash( name );
    struct object_name *ptr = names[hash];
    while (ptr && strcmp( ptr->name, name )) ptr = ptr->next;
    if (!ptr) return NULL;
    grab_object( ptr->obj );
    return ptr->obj;
}
