/*
 * Server-side handle management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "winerror.h"
#include "winbase.h"

#include "handle.h"
#include "process.h"
#include "thread.h"

struct handle_entry
{
    struct object *ptr;
    unsigned int   access;
};

struct handle_table
{
    struct object        obj;         /* object header */
    struct process      *process;     /* process owning this table */
    int                  count;       /* number of allocated entries */
    int                  last;        /* last used entry */
    int                  free;        /* first entry that may be free */
    struct handle_entry *entries;     /* handle entries */
};

static struct handle_table *global_table;

/* reserved handle access rights */
#define RESERVED_SHIFT         25
#define RESERVED_INHERIT       (HANDLE_FLAG_INHERIT << RESERVED_SHIFT)
#define RESERVED_CLOSE_PROTECT (HANDLE_FLAG_PROTECT_FROM_CLOSE << RESERVED_SHIFT)
#define RESERVED_ALL           (RESERVED_INHERIT | RESERVED_CLOSE_PROTECT)

/* global handle macros */
#define HANDLE_OBFUSCATOR         0x544a4def
#define HANDLE_IS_GLOBAL(h)       (((h) ^ HANDLE_OBFUSCATOR) < 0x10000)
#define HANDLE_LOCAL_TO_GLOBAL(h) ((h) ^ HANDLE_OBFUSCATOR)
#define HANDLE_GLOBAL_TO_LOCAL(h) ((h) ^ HANDLE_OBFUSCATOR)

#define MIN_HANDLE_ENTRIES  32


static void handle_table_dump( struct object *obj, int verbose );
static void handle_table_destroy( struct object *obj );

static const struct object_ops handle_table_ops =
{
    handle_table_dump,
    no_add_queue,
    NULL,  /* should never get called */
    NULL,  /* should never get called */
    NULL,  /* should never get called */
    no_read_fd,
    no_write_fd,
    no_flush,
    no_get_file_info,
    handle_table_destroy
};

/* dump a handle table */
static void handle_table_dump( struct object *obj, int verbose )
{
    int i;
    struct handle_table *table = (struct handle_table *)obj;
    struct handle_entry *entry = table->entries;

    assert( obj->ops == &handle_table_ops );

    fprintf( stderr, "Handle table last=%d count=%d process=%p\n",
             table->last, table->count, table->process );
    if (!verbose) return;
    entry = table->entries;
    for (i = 0; i <= table->last; i++, entry++)
    {
        if (!entry->ptr) continue;
        fprintf( stderr, "%9d: %p %08x ", i + 1, entry->ptr, entry->access );
        entry->ptr->ops->dump( entry->ptr, 0 );
    }
}

/* destroy a handle table */
static void handle_table_destroy( struct object *obj )
{
    int i;
    struct handle_table *table = (struct handle_table *)obj;
    struct handle_entry *entry = table->entries;

    assert( obj->ops == &handle_table_ops );

    for (i = 0; i <= table->last; i++, entry++)
    {
        struct object *obj = entry->ptr;
        entry->ptr = NULL;
        if (obj) release_object( obj );
    }
    free( table->entries );
    free( table );
}

/* allocate a new handle table */
struct object *alloc_handle_table( struct process *process, int count )
{
    struct handle_table *table;

    if (count < MIN_HANDLE_ENTRIES) count = MIN_HANDLE_ENTRIES;
    if (!(table = alloc_object( sizeof(*table), &handle_table_ops, NULL )))
        return NULL;
    table->process = process;
    table->count   = count;
    table->last    = -1;
    table->free    = 0;
    if ((table->entries = mem_alloc( count * sizeof(*table->entries) ))) return &table->obj;
    release_object( table );
    return NULL;
}

/* grow a handle table */
static int grow_handle_table( struct handle_table *table )
{
    struct handle_entry *new_entries;
    int count = table->count;

    if (count >= INT_MAX / 2) return 0;
    count *= 2;
    if (!(new_entries = realloc( table->entries, count * sizeof(struct handle_entry) )))
    {
        SET_ERROR( ERROR_OUTOFMEMORY );
        return 0;
    }
    table->entries = new_entries;
    table->count   = count;
    return 1;
}

/* find the first free entry in the handle table */
static struct handle_entry *get_free_entry( struct handle_table *table, int *phandle )
{
    struct handle_entry *entry = table->entries + table->free;
    int handle;

    for (handle = table->free; handle <= table->last; handle++, entry++)
        if (!entry->ptr) goto found;
    if (handle >= table->count)
    {
        if (!grow_handle_table( table )) return NULL;
        entry = table->entries + handle;  /* the entries may have moved */
    }
    table->last = handle;
 found:
    table->free = *phandle = handle + 1;  /* avoid handle 0 */
    return entry;
}

/* allocate a handle for an object, incrementing its refcount */
/* return the handle, or -1 on error */
int alloc_handle( struct process *process, void *obj, unsigned int access, int inherit )
{
    struct handle_table *table = (struct handle_table *)process->handles;
    struct handle_entry *entry;
    int handle;

    assert( table );
    assert( !(access & RESERVED_ALL) );
    if (inherit) access |= RESERVED_INHERIT;

    if (!(entry = get_free_entry( table, &handle ))) return -1;
    entry->ptr    = grab_object( obj );
    entry->access = access;
    return handle;
}

/* allocate a global handle for an object, incrementing its refcount */
/* return the handle, or -1 on error */
static int alloc_global_handle( void *obj, unsigned int access )
{
    struct handle_entry *entry;
    int handle;

    if (!global_table)
    {
        if (!(global_table = (struct handle_table *)alloc_handle_table( NULL, 0 ))) return -1;
    }
    if (!(entry = get_free_entry( global_table, &handle ))) return -1;
    entry->ptr    = grab_object( obj );
    entry->access = access;
    return HANDLE_LOCAL_TO_GLOBAL(handle);
}

/* return an handle entry, or NULL if the handle is invalid */
static struct handle_entry *get_handle( struct process *process, int handle )
{
    struct handle_table *table = (struct handle_table *)process->handles;
    struct handle_entry *entry;

    if (HANDLE_IS_GLOBAL(handle))
    {
        handle = HANDLE_GLOBAL_TO_LOCAL(handle);
        table = global_table;
    }
    if (!table) goto error;
    handle--;  /* handles start at 1 */
    if (handle < 0) goto error;
    if (handle > table->last) goto error;
    entry = table->entries + handle;
    if (!entry->ptr) goto error;
    return entry;

 error:
    SET_ERROR( ERROR_INVALID_HANDLE );
    return NULL;
}

/* attempt to shrink a table */
static void shrink_handle_table( struct handle_table *table )
{
    struct handle_entry *entry = table->entries + table->last;
    struct handle_entry *new_entries;
    int count = table->count;

    while (table->last >= 0)
    {
        if (entry->ptr) break;
        table->last--;
        entry--;
    }
    if (table->last >= count / 4) return;  /* no need to shrink */
    if (count < MIN_HANDLE_ENTRIES * 2) return;  /* too small to shrink */
    count /= 2;
    if (!(new_entries = realloc( table->entries, count * sizeof(*new_entries) ))) return;
    table->count   = count;
    table->entries = new_entries;
}

/* copy the handle table of the parent process */
/* return 1 if OK, 0 on error */
struct object *copy_handle_table( struct process *process, struct process *parent )
{
    struct handle_table *parent_table = (struct handle_table *)parent->handles;
    struct handle_table *table;
    int i;

    assert( parent_table );
    assert( parent_table->obj.ops == &handle_table_ops );

    if (!(table = (struct handle_table *)alloc_handle_table( process, parent_table->count )))
        return NULL;

    if ((table->last = parent_table->last) >= 0)
    {
        struct handle_entry *ptr = table->entries;
        memcpy( ptr, parent_table->entries, (table->last + 1) * sizeof(struct handle_entry) );
        for (i = 0; i <= table->last; i++, ptr++)
        {
            if (!ptr->ptr) continue;
            if (ptr->access & RESERVED_INHERIT) grab_object( ptr->ptr );
            else ptr->ptr = NULL; /* don't inherit this entry */
        }
    }
    /* attempt to shrink the table */
    shrink_handle_table( table );
    return &table->obj;
}

/* close a handle and decrement the refcount of the associated object */
/* return 1 if OK, 0 on error */
int close_handle( struct process *process, int handle )
{
    struct handle_table *table;
    struct handle_entry *entry;
    struct object *obj;

    if (!(entry = get_handle( process, handle ))) return 0;
    if (entry->access & RESERVED_CLOSE_PROTECT) return 0;  /* FIXME: error code */
    obj = entry->ptr;
    entry->ptr = NULL;
    table = HANDLE_IS_GLOBAL(handle) ? global_table : (struct handle_table *)process->handles;
    if (entry < table->entries + table->free) table->free = entry - table->entries;
    if (entry == table->entries + table->last) shrink_handle_table( table );
    release_object( obj );
    return 1;
}

/* close all the global handles */
void close_global_handles(void)
{
    if (global_table)
    {
        release_object( global_table );
        global_table = NULL;
    }
}

/* retrieve the object corresponding to a handle, incrementing its refcount */
struct object *get_handle_obj( struct process *process, int handle,
                               unsigned int access, const struct object_ops *ops )
{
    struct handle_entry *entry;
    struct object *obj;

    switch( handle )
    {
    case 0xfffffffe:  /* current thread pseudo-handle */
        obj = &current->obj;
        break;
    case 0x7fffffff:  /* current process pseudo-handle */
        obj = (struct object *)current->process;
        break;
    default:
        if (!(entry = get_handle( process, handle ))) return NULL;
        if ((entry->access & access) != access)
        {
            SET_ERROR( ERROR_ACCESS_DENIED );
            return NULL;
        }
        obj = entry->ptr;
        break;
    }
    if (ops && (obj->ops != ops))
    {
        SET_ERROR( ERROR_INVALID_HANDLE );  /* not the right type */
        return NULL;
    }
    return grab_object( obj );
}

/* get/set the handle reserved flags */
/* return the new flags (or -1 on error) */
static int set_handle_info( struct process *process, int handle, int mask, int flags )
{
    struct handle_entry *entry;

    if (!(entry = get_handle( process, handle ))) return -1;
    mask  = (mask << RESERVED_SHIFT) & RESERVED_ALL;
    flags = (flags << RESERVED_SHIFT) & mask;
    entry->access = (entry->access & ~mask) | flags;
    return (entry->access & RESERVED_ALL) >> RESERVED_SHIFT;
}

/* duplicate a handle */
int duplicate_handle( struct process *src, int src_handle, struct process *dst,
                      unsigned int access, int inherit, int options )
{
    int res;
    struct object *obj = get_handle_obj( src, src_handle, 0, NULL );

    if (!obj) return -1;
    if (options & DUP_HANDLE_SAME_ACCESS)
    {
        struct handle_entry *entry = get_handle( src, src_handle );
        if (entry)
            access = entry->access;
        else  /* pseudo-handle, give it full access */
        {
            access = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
            CLEAR_ERROR();
        }
    }
    access &= ~RESERVED_ALL;
    if (options & DUP_HANDLE_MAKE_GLOBAL)
        res = alloc_global_handle( obj, access );
    else
        res = alloc_handle( dst, obj, access, inherit );
    release_object( obj );
    return res;
}

/* open a new handle to an existing object */
int open_object( const char *name, const struct object_ops *ops,
                 unsigned int access, int inherit )
{
    struct object *obj = find_object( name );
    if (!obj) 
    {
        SET_ERROR( ERROR_FILE_NOT_FOUND );
        return -1;
    }
    if (ops && obj->ops != ops)
    {
        release_object( obj );
        SET_ERROR( ERROR_INVALID_HANDLE );  /* FIXME: not the right type */ 
        return -1;
    }
    return alloc_handle( current->process, obj, access, inherit );
}

/* close a handle */
DECL_HANDLER(close_handle)
{
    close_handle( current->process, req->handle );
    send_reply( current, -1, 0 );
}

/* get information about a handle */
DECL_HANDLER(get_handle_info)
{
    struct get_handle_info_reply reply;
    reply.flags = set_handle_info( current->process, req->handle, 0, 0 );
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* set a handle information */
DECL_HANDLER(set_handle_info)
{
    set_handle_info( current->process, req->handle, req->mask, req->flags );
    send_reply( current, -1, 0 );
}

/* duplicate a handle */
DECL_HANDLER(dup_handle)
{
    struct dup_handle_reply reply = { -1 };
    struct process *src, *dst;

    if ((src = get_process_from_handle( req->src_process, PROCESS_DUP_HANDLE )))
    {
        if (req->options & DUP_HANDLE_MAKE_GLOBAL)
        {
            reply.handle = duplicate_handle( src, req->src_handle, NULL,
                                             req->access, req->inherit, req->options );
        }
        else if ((dst = get_process_from_handle( req->dst_process, PROCESS_DUP_HANDLE )))
        {
            reply.handle = duplicate_handle( src, req->src_handle, dst,
                                             req->access, req->inherit, req->options );
            release_object( dst );
        }
        /* close the handle no matter what happened */
        if (req->options & DUP_HANDLE_CLOSE_SOURCE)
            close_handle( src, req->src_handle );
        release_object( src );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}
