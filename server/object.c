/*
 * Server-side objects
 *
 * Copyright (C) 1998 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "thread.h"
#include "unicode.h"
#include "list.h"


struct object_name
{
    struct list         entry;    /* entry in the hash list */
    struct object      *obj;
    size_t              len;
    WCHAR               name[1];
};

struct namespace
{
    unsigned int        hash_size;       /* size of hash table */
    int                 case_sensitive;  /* are names case sensitive? */
    struct list         names[1];        /* array of hash entry lists */
};


#ifdef DEBUG_OBJECTS
static struct list object_list = LIST_INIT(object_list);

void dump_objects(void)
{
    struct list *p;

    LIST_FOR_EACH( p, &object_list )
    {
        struct object *ptr = LIST_ENTRY( p, struct object, obj_list );
        fprintf( stderr, "%p:%d: ", ptr, ptr->refcount );
        ptr->ops->dump( ptr, 1 );
    }
}
#endif

/*****************************************************************/

/* malloc replacement */
void *mem_alloc( size_t size )
{
    void *ptr = malloc( size );
    if (ptr) memset( ptr, 0x55, size );
    else set_error( STATUS_NO_MEMORY );
    return ptr;
}

/* duplicate a block of memory */
void *memdup( const void *data, size_t len )
{
    void *ptr = malloc( len );
    if (ptr) memcpy( ptr, data, len );
    else set_error( STATUS_NO_MEMORY );
    return ptr;
}


/*****************************************************************/

static int get_name_hash( const struct namespace *namespace, const WCHAR *name, size_t len )
{
    WCHAR hash = 0;
    len /= sizeof(WCHAR);
    if (namespace->case_sensitive) while (len--) hash ^= *name++;
    else while (len--) hash ^= tolowerW(*name++);
    return hash % namespace->hash_size;
}

/* allocate a name for an object */
static struct object_name *alloc_name( const WCHAR *name, size_t len )
{
    struct object_name *ptr;

    if ((ptr = mem_alloc( sizeof(*ptr) + len - sizeof(ptr->name) )))
    {
        ptr->len = len;
        memcpy( ptr->name, name, len );
    }
    return ptr;
}

/* free the name of an object */
static void free_name( struct object *obj )
{
    struct object_name *ptr = obj->name;
    list_remove( &ptr->entry );
    free( ptr );
}

/* set the name of an existing object */
static void set_object_name( struct namespace *namespace,
                             struct object *obj, struct object_name *ptr )
{
    int hash = get_name_hash( namespace, ptr->name, ptr->len );

    list_add_head( &namespace->names[hash], &ptr->entry );
    ptr->obj = obj;
    obj->name = ptr;
}

/* allocate and initialize an object */
/* if the function fails the fd is closed */
void *alloc_object( const struct object_ops *ops, int fd )
{
    struct object *obj = mem_alloc( ops->size );
    if (obj)
    {
        obj->refcount = 1;
        obj->fd       = fd;
        obj->select   = -1;
        obj->ops      = ops;
        obj->head     = NULL;
        obj->tail     = NULL;
        obj->name     = NULL;
        if ((fd != -1) && (add_select_user( obj ) == -1))
        {
            close( fd );
            free( obj );
            return NULL;
        }
#ifdef DEBUG_OBJECTS
        list_add_head( &object_list, &obj->obj_list );
#endif
        return obj;
    }
    if (fd != -1) close( fd );
    return NULL;
}

void *create_named_object( struct namespace *namespace, const struct object_ops *ops,
                           const WCHAR *name, size_t len )
{
    struct object *obj;
    struct object_name *name_ptr;

    if (!name || !len) return alloc_object( ops, -1 );

    if ((obj = find_object( namespace, name, len )))
    {
        if (obj->ops == ops)
        {
            set_error( STATUS_OBJECT_NAME_COLLISION );
            return obj;
        }
        set_error( STATUS_OBJECT_TYPE_MISMATCH );
        return NULL;
    }
    if (!(name_ptr = alloc_name( name, len ))) return NULL;
    if ((obj = alloc_object( ops, -1 )))
    {
        set_object_name( namespace, obj, name_ptr );
        clear_error();
    }
    else free( name_ptr );
    return obj;
}

/* dump the name of an object to stderr */
void dump_object_name( struct object *obj )
{
    if (!obj->name) fprintf( stderr, "name=\"\"" );
    else
    {
        fprintf( stderr, "name=L\"" );
        dump_strW( obj->name->name, obj->name->len/sizeof(WCHAR), stderr, "\"\"" );
        fputc( '\"', stderr );
    }
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
        obj->ops->destroy( obj );
        if (obj->name) free_name( obj );
        if (obj->select != -1) remove_select_user( obj );
        if (obj->fd != -1) close( obj->fd );
#ifdef DEBUG_OBJECTS
        list_remove( &obj->obj_list );
        memset( obj, 0xaa, obj->ops->size );
#endif
        free( obj );
    }
}

/* find an object by its name; the refcount is incremented */
struct object *find_object( const struct namespace *namespace, const WCHAR *name, size_t len )
{
    const struct list *list, *p;

    if (!name || !len) return NULL;

    list = &namespace->names[ get_name_hash( namespace, name, len ) ];
    if (namespace->case_sensitive)
    {
        LIST_FOR_EACH( p, list )
        {
            struct object_name *ptr = LIST_ENTRY( p, struct object_name, entry );
            if (ptr->len != len) continue;
            if (!memcmp( ptr->name, name, len )) return grab_object( ptr->obj );
        }
    }
    else
    {
        LIST_FOR_EACH( p, list )
        {
            struct object_name *ptr = LIST_ENTRY( p, struct object_name, entry );
            if (ptr->len != len) continue;
            if (!strncmpiW( ptr->name, name, len/sizeof(WCHAR) )) return grab_object( ptr->obj );
        }
    }
    return NULL;
}

/* allocate a namespace */
struct namespace *create_namespace( unsigned int hash_size, int case_sensitive )
{
    struct namespace *namespace;
    unsigned int i;

    namespace = mem_alloc( sizeof(*namespace) + (hash_size - 1) * sizeof(namespace->names[0]) );
    if (namespace)
    {
        namespace->hash_size      = hash_size;
        namespace->case_sensitive = case_sensitive;
        for (i = 0; i < hash_size; i++) list_init( &namespace->names[i] );
    }
    return namespace;
}

/* functions for unimplemented/default object operations */

int no_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return 0;
}

int no_satisfied( struct object *obj, struct thread *thread )
{
    return 0;  /* not abandoned */
}

int no_get_fd( struct object *obj )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return -1;
}

int no_flush( struct object *obj )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return 0;
}

int no_get_file_info( struct object *obj, struct get_file_info_reply *info, int *flags )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    *flags = 0;
    return FD_TYPE_INVALID;
}

void no_destroy( struct object *obj )
{
}

/* default add_queue() routine for objects that poll() on an fd */
int default_poll_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    if (!obj->head)  /* first on the queue */
        set_select_events( obj, obj->ops->get_poll_events( obj ) );
    add_queue( obj, entry );
    return 1;
}

/* default remove_queue() routine for objects that poll() on an fd */
void default_poll_remove_queue( struct object *obj, struct wait_queue_entry *entry )
{
    grab_object(obj);
    remove_queue( obj, entry );
    if (!obj->head)  /* last on the queue is gone */
        set_select_events( obj, 0 );
    release_object( obj );
}

/* default signaled() routine for objects that poll() on an fd */
int default_poll_signaled( struct object *obj, struct thread *thread )
{
    int events = obj->ops->get_poll_events( obj );

    if (check_select_events( obj->fd, events ))
    {
        /* stop waiting on select() if we are signaled */
        set_select_events( obj, 0 );
        return 1;
    }
    /* restart waiting on select() if we are no longer signaled */
    if (obj->head) set_select_events( obj, events );
    return 0;
}

/* default handler for poll() events */
void default_poll_event( struct object *obj, int event )
{
    /* an error occurred, stop polling this fd to avoid busy-looping */
    if (event & (POLLERR | POLLHUP)) set_select_events( obj, -1 );
    wake_up( obj, 0 );
}
