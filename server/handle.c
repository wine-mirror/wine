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

static struct process *initial_process;

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

/* grow a handle table */
/* return 1 if OK, 0 on error */
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
    table->count   = count;
    table->entries = new_entries;
    return 1;
}

/* allocate a handle for an object, incrementing its refcount */
/* return the handle, or -1 on error */
int alloc_handle( struct process *process, void *obj, unsigned int access,
                  int inherit )
{
    struct handle_table *table = get_process_handles( process );
    struct handle_entry *entry;
    int handle;

    assert( !(access & RESERVED_ALL) );
    if (inherit) access |= RESERVED_INHERIT;

    /* find the first free entry */

    if (!(entry = table->entries)) return -1;
    for (handle = 0; handle <= table->last; handle++, entry++)
        if (!entry->ptr) goto found;

    if (handle >= table->count)
    {
        if (!grow_handle_table( table )) return -1;
        entry = table->entries + handle;  /* the table may have moved */
    }
    table->last = handle;

 found:
    entry->ptr    = grab_object( obj );
    entry->access = access;
    return handle + 1;  /* avoid handle 0 */
}

/* return an handle entry, or NULL if the handle is invalid */
static struct handle_entry *get_handle( struct process *process, int handle )
{
    struct handle_table *table;
    struct handle_entry *entry;

    if (HANDLE_IS_GLOBAL(handle))
    {
        handle = HANDLE_GLOBAL_TO_LOCAL(handle);
        process = initial_process;
    }
    table = get_process_handles( process );
    handle--;  /* handles start at 1 */
    if ((handle < 0) || (handle > table->last)) goto error;
    entry = table->entries + handle;
    if (!entry->ptr) goto error;
    return entry;

 error:
    SET_ERROR( ERROR_INVALID_HANDLE );
    return NULL;
}

/* attempt to shrink a table */
/* return 1 if OK, 0 on error */
static int shrink_handle_table( struct handle_table *table )
{
    struct handle_entry *new_entries;
    struct handle_entry *entry = table->entries + table->last;
    int count = table->count;

    while (table->last >= 0)
    {
        if (entry->ptr) break;
        table->last--;
        entry--;
    }
    if (table->last >= count / 4) return 1;  /* no need to shrink */
    if (count < MIN_HANDLE_ENTRIES * 2) return 1;  /* too small to shrink */
    count /= 2;
    if (!(new_entries = realloc( table->entries,
                                 count * sizeof(struct handle_entry) )))
        return 0;
    table->count   = count;
    table->entries = new_entries;
    return 1;
}

/* copy the handle table of the parent process */
/* return 1 if OK, 0 on error */
int copy_handle_table( struct process *process, struct process *parent )
{
    struct handle_table *parent_table;
    struct handle_table *table = get_process_handles( process );
    struct handle_entry *ptr;
    int i, count, last;

    if (!parent)  /* first process */
    {
        if (!initial_process) initial_process = process;
        parent_table = NULL;
        count = MIN_HANDLE_ENTRIES;
        last  = -1;
    }
    else
    {
        parent_table = get_process_handles( parent );
        assert( parent_table->entries );
        count = parent_table->count;
        last  = parent_table->last;
    }

    if (!(ptr = mem_alloc( count * sizeof(struct handle_entry)))) return 0;
    table->entries = ptr;
    table->count   = count;
    table->last    = last;

    if (last >= 0)
    {
        memcpy( ptr, parent_table->entries, (last + 1) * sizeof(struct handle_entry) );
        for (i = 0; i <= last; i++, ptr++)
        {
            if (!ptr->ptr) continue;
            if (ptr->access & RESERVED_INHERIT) grab_object( ptr->ptr );
            else ptr->ptr = NULL; /* don't inherit this entry */
        }
    }
    /* attempt to shrink the table */
    shrink_handle_table( table );
    return 1;
}

/* close a handle and decrement the refcount of the associated object */
/* return 1 if OK, 0 on error */
int close_handle( struct process *process, int handle )
{
    struct handle_table *table;
    struct handle_entry *entry;
    struct object *obj;

    if (HANDLE_IS_GLOBAL(handle))
    {
        handle = HANDLE_GLOBAL_TO_LOCAL(handle);
        process = initial_process;
    }
    table = get_process_handles( process );
    if (!(entry = get_handle( process, handle ))) return 0;
    if (entry->access & RESERVED_CLOSE_PROTECT) return 0;  /* FIXME: error code */
    obj = entry->ptr;
    entry->ptr = NULL;
    if (handle-1 == table->last) shrink_handle_table( table );
    release_object( obj );
    return 1;
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
    struct handle_entry *entry = get_handle( src, src_handle );
    if (!entry) return -1;

    if (options & DUP_HANDLE_SAME_ACCESS) access = entry->access;
    if (options & DUP_HANDLE_MAKE_GLOBAL) dst = initial_process;
    access &= ~RESERVED_ALL;
    res = alloc_handle( dst, entry->ptr, access, inherit );
    if (options & DUP_HANDLE_MAKE_GLOBAL) res = HANDLE_LOCAL_TO_GLOBAL(res);
    return res;
}

/* free the process handle entries */
void free_handles( struct process *process )
{
    struct handle_table *table = get_process_handles( process );
    struct handle_entry *entry;
    int handle;

    if (!(entry = table->entries)) return;
    for (handle = 0; handle <= table->last; handle++, entry++)
    {
        struct object *obj = entry->ptr;
        entry->ptr = NULL;
        if (obj) release_object( obj );
    }
    free( table->entries );
    table->count   = 0;
    table->last    = -1;
    table->entries = NULL;
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

/* dump a handle table on stdout */
void dump_handles( struct process *process )
{
    struct handle_table *table = get_process_handles( process );
    struct handle_entry *entry;
    int i;

    if (!table->entries) return;
    entry = table->entries;
    for (i = 0; i <= table->last; i++, entry++)
    {
        if (!entry->ptr) continue;
        printf( "%5d: %p %08x ", i + 1, entry->ptr, entry->access );
        entry->ptr->ops->dump( entry->ptr, 0 );
    }
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
