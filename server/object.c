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
#include <unistd.h>

#include "thread.h"
#include "unicode.h"


struct object_name
{
    struct object_name *next;
    struct object_name *prev;
    struct object      *obj;
    size_t              len;
    WCHAR               name[1];
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
    else if (current) set_error( STATUS_NO_MEMORY );
    return ptr;
}

/* duplicate a block of memory */
void *memdup( const void *data, size_t len )
{
    void *ptr = malloc( len );
    if (ptr) memcpy( ptr, data, len );
    else if (current) set_error( STATUS_NO_MEMORY );
    return ptr;
}


/*****************************************************************/

static int get_name_hash( const WCHAR *name, size_t len )
{
    WCHAR hash = 0;
    while (len--) hash ^= *name++;
    return hash % NAME_HASH_SIZE;
}

/* allocate a name for an object */
static struct object_name *alloc_name( const WCHAR *name, size_t len )
{
    struct object_name *ptr;

    if ((ptr = mem_alloc( sizeof(*ptr) + len * sizeof(ptr->name[0]) )))
    {
        ptr->len = len;
        memcpy( ptr->name, name, len * sizeof(ptr->name[0]) );
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
        obj->prev = NULL;
        if ((obj->next = first) != NULL) obj->next->prev = obj;
        first = obj;
#endif
        return obj;
    }
    if (fd != -1) close( fd );
    return NULL;
}

void *create_named_object( const struct object_ops *ops, const WCHAR *name, size_t len )
{
    struct object *obj;
    struct object_name *name_ptr;

    if (!name || !len) return alloc_object( ops, -1 );
    if (!(name_ptr = alloc_name( name, len ))) return NULL;

    if ((obj = find_object( name_ptr->name, name_ptr->len )))
    {
        free( name_ptr );  /* we no longer need it */
        if (obj->ops == ops)
        {
            set_error( STATUS_OBJECT_NAME_COLLISION );
            return obj;
        }
        set_error( STATUS_OBJECT_TYPE_MISMATCH );
        return NULL;
    }
    if ((obj = alloc_object( ops, -1 )))
    {
        set_object_name( obj, name_ptr );
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
        dump_strW( obj->name->name, strlenW(obj->name->name), stderr, "\"\"" );
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
        if (obj->next) obj->next->prev = obj->prev;
        if (obj->prev) obj->prev->next = obj->next;
        else first = obj->next;
        memset( obj, 0xaa, obj->ops->size );
#endif
        free( obj );
    }
}

/* find an object by its name; the refcount is incremented */
struct object *find_object( const WCHAR *name, size_t len )
{
    struct object_name *ptr;
    if (!name || !len) return NULL;
    for (ptr = names[ get_name_hash( name, len ) ]; ptr; ptr = ptr->next)
    {
        if (ptr->len != len) continue;
        if (!memcmp( ptr->name, name, len*sizeof(WCHAR) )) return grab_object( ptr->obj );
    }
    return NULL;
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

int no_read_fd( struct object *obj )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return -1;
}

int no_write_fd( struct object *obj )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return -1;
}

int no_flush( struct object *obj )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return 0;
}

int no_get_file_info( struct object *obj, struct get_file_info_request *info )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return 0;
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
