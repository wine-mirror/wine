/*
 * Server-side window hooks support
 *
 * Copyright (C) 2002 Alexandre Julliard
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
#include <stdio.h>

#include "winbase.h"
#include "winuser.h"

#include "object.h"
#include "request.h"
#include "user.h"

struct hook_table;

struct hook
{
    struct list         chain;    /* hook chain entry */
    user_handle_t       handle;   /* user handle for this hook */
    struct thread      *thread;   /* thread owning the hook */
    int                 index;    /* hook table index */
    void               *proc;     /* hook function */
    int                 unicode;  /* is it a unicode hook? */
};

#define NB_HOOKS (WH_MAXHOOK-WH_MINHOOK+1)
#define HOOK_ENTRY(p)  LIST_ENTRY( (p), struct hook, chain )

struct hook_table
{
    struct object obj;              /* object header */
    struct list   hooks[NB_HOOKS];  /* array of hook chains */
    int           counts[NB_HOOKS]; /* use counts for each hook chain */
};

static void hook_table_dump( struct object *obj, int verbose );
static void hook_table_destroy( struct object *obj );

static const struct object_ops hook_table_ops =
{
    sizeof(struct hook_table),    /* size */
    hook_table_dump,              /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    NULL,                         /* get_poll_events */
    NULL,                         /* poll_event */
    no_get_fd,                    /* get_fd */
    no_flush,                     /* flush */
    no_get_file_info,             /* get_file_info */
    NULL,                         /* queue_async */
    hook_table_destroy            /* destroy */
};


/* create a new hook table */
static struct hook_table *alloc_hook_table(void)
{
    struct hook_table *table;
    int i;

    if ((table = alloc_object( &hook_table_ops, -1 )))
    {
        for (i = 0; i < NB_HOOKS; i++)
        {
            list_init( &table->hooks[i] );
            table->counts[i] = 0;
        }
    }
    return table;
}

/* create a new hook and add it to the specified table */
static struct hook *add_hook( struct thread *thread, int index )
{
    struct hook *hook;
    struct hook_table *table = thread->hooks;

    if (!table)
    {
        if (!(table = alloc_hook_table())) return NULL;
        thread->hooks = table;
    }
    if (!(hook = mem_alloc( sizeof(*hook) ))) return NULL;

    if (!(hook->handle = alloc_user_handle( hook, USER_HOOK )))
    {
        free( hook );
        return NULL;
    }
    hook->thread = thread ? (struct thread *)grab_object( thread ) : NULL;
    hook->index  = index;
    list_add_head( &table->hooks[index], &hook->chain );
    return hook;
}

/* free a hook, removing it from its chain */
static void free_hook( struct hook *hook )
{
    free_user_handle( hook->handle );
    if (hook->thread) release_object( hook->thread );
    list_remove( &hook->chain );
    free( hook );
}

/* find a hook from its index and proc */
static struct hook *find_hook( struct thread *thread, int index, void *proc )
{
    struct list *p;
    struct hook_table *table = thread->hooks;

    if (table)
    {
        LIST_FOR_EACH( p, &table->hooks[index] )
        {
            struct hook *hook = HOOK_ENTRY( p );
            if (hook->proc == proc) return hook;
        }
    }
    return NULL;
}

/* get the hook table that a given hook belongs to */
inline static struct hook_table *get_table( struct hook *hook )
{
    return hook->thread->hooks;
}

/* get the first hook in the chain */
inline static struct hook *get_first_hook( struct hook_table *table, int index )
{
    struct list *elem = list_head( &table->hooks[index] );
    return elem ? HOOK_ENTRY( elem ) : NULL;
}

/* find the next hook in the chain, skipping the deleted ones */
static struct hook *get_next_hook( struct hook *hook )
{
    struct hook_table *table = get_table( hook );
    struct hook *next;

    while ((next = HOOK_ENTRY( list_next( &table->hooks[hook->index], &hook->chain ) )))
    {
        if (next->proc) break;
    }
    return next;
}

static void hook_table_dump( struct object *obj, int verbose )
{
/*    struct hook_table *table = (struct hook_table *)obj; */
    fprintf( stderr, "Hook table\n" );
}

static void hook_table_destroy( struct object *obj )
{
    int i;
    struct hook *hook;
    struct hook_table *table = (struct hook_table *)obj;

    for (i = 0; i < NB_HOOKS; i++)
    {
        while ((hook = get_first_hook( table, i )) != NULL) free_hook( hook );
    }
}

/* remove a hook, freeing it if the chain is not in use */
static void remove_hook( struct hook *hook )
{
    struct hook_table *table = get_table( hook );
    if (table->counts[hook->index])
        hook->proc = NULL; /* chain is in use, just mark it and return */
    else
        free_hook( hook );
}

/* release a hook chain, removing deleted hooks if the use count drops to 0 */
static void release_hook_chain( struct hook_table *table, int index )
{
    if (!table->counts[index])  /* use count shouldn't already be 0 */
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    if (!--table->counts[index])
    {
        struct hook *hook = get_first_hook( table, index );
        while (hook)
        {
            struct hook *next = HOOK_ENTRY( list_next( &table->hooks[hook->index], &hook->chain ) );
            if (!hook->proc) free_hook( hook );
            hook = next;
        }
    }
}


/* set a window hook */
DECL_HANDLER(set_hook)
{
    struct thread *thread;
    struct hook *hook;

    if (!req->proc || req->id < WH_MINHOOK || req->id > WH_MAXHOOK)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    if (!req->tid) thread = (struct thread *)grab_object( current );
    else if (!(thread = get_thread_from_id( req->tid ))) return;

    if ((hook = add_hook( thread, req->id - WH_MINHOOK )))
    {
        hook->proc    = req->proc;
        hook->unicode = req->unicode;
        reply->handle = hook->handle;
    }
    release_object( thread );
}


/* remove a window hook */
DECL_HANDLER(remove_hook)
{
    struct hook *hook;

    if (req->handle) hook = get_user_object( req->handle, USER_HOOK );
    else
    {
        if (!req->proc || req->id < WH_MINHOOK || req->id > WH_MAXHOOK)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }
        if (!(hook = find_hook( current, req->id - WH_MINHOOK, req->proc )))
            set_error( STATUS_INVALID_PARAMETER );
    }
    if (hook) remove_hook( hook );
}


/* start calling a hook chain */
DECL_HANDLER(start_hook_chain)
{
    struct hook *hook;
    struct hook_table *table = current->hooks;

    if (req->id < WH_MINHOOK || req->id > WH_MAXHOOK)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    if (!table) return;  /* no hook set */
    if (!(hook = get_first_hook( table, req->id - WH_MINHOOK ))) return;  /* no hook set */
    reply->handle  = hook->handle;
    reply->proc    = hook->proc;
    reply->unicode = hook->unicode;
    table->counts[hook->index]++;
}


/* finished calling a hook chain */
DECL_HANDLER(finish_hook_chain)
{
    struct hook_table *table = current->hooks;
    int index = req->id - WH_MINHOOK;

    if (req->id < WH_MINHOOK || req->id > WH_MAXHOOK)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    if (table) release_hook_chain( table, index );
}


/* get the next hook to call */
DECL_HANDLER(get_next_hook)
{
    struct hook *hook, *next;

    if (!(hook = get_user_object( req->handle, USER_HOOK ))) return;
    if (hook->thread != current)
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    if ((next = get_next_hook( hook )))
    {
        reply->next = next->handle;
        reply->id   = next->index + WH_MINHOOK;
        reply->proc = next->proc;
        reply->prev_unicode = hook->unicode;
        reply->next_unicode = next->unicode;
    }
}
