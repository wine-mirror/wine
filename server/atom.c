/*
 * Server-side atom management
 *
 * Copyright (C) 1999, 2000 Alexandre Julliard
 * Copyright (C) 2000 Turchanov Sergei
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "unicode.h"
#include "request.h"
#include "object.h"
#include "process.h"
#include "handle.h"
#include "user.h"
#include "winuser.h"
#include "winternl.h"

#define HASH_SIZE     37
#define MIN_HASH_SIZE 4
#define MAX_HASH_SIZE 0x200

#define MAX_ATOM_LEN  (255 * sizeof(WCHAR))
#define MIN_STR_ATOM  0xc000
#define MAX_ATOMS     0x4000

struct atom_entry
{
    struct atom_entry *next;   /* hash table list */
    struct atom_entry *prev;   /* hash table list */
    int                count;  /* reference count */
    short              pinned; /* whether the atom is pinned or not */
    atom_t             atom;   /* atom handle */
    unsigned short     hash;   /* string hash */
    unsigned short     len;    /* string len */
    WCHAR              str[1]; /* atom string */
};

struct atom_table
{
    struct object       obj;                 /* object header */
    int                 count;               /* count of atom handles */
    int                 last;                /* last handle in-use */
    struct atom_entry **handles;             /* atom handles */
    int                 entries_count;       /* number of hash entries */
    struct atom_entry **entries;             /* hash table entries */
};

static void atom_table_dump( struct object *obj, int verbose );
static void atom_table_destroy( struct object *obj );

static const struct object_ops atom_table_ops =
{
    sizeof(struct atom_table),    /* size */
    &no_type,                     /* type */
    atom_table_dump,              /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* get_esync_fd */
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    default_map_access,           /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    no_get_full_name,             /* get_full_name */
    no_lookup_name,               /* lookup_name */
    no_link_name,                 /* link_name */
    NULL,                         /* unlink_name */
    no_open_file,                 /* open_file */
    no_kernel_obj_list,           /* get_kernel_obj_list */
    no_close_handle,              /* close_handle */
    atom_table_destroy            /* destroy */
};

static struct atom_table *global_table;

/* create an atom table */
static struct atom_table *create_table(int entries_count)
{
    struct atom_table *table;

    if ((table = alloc_object( &atom_table_ops )))
    {
        if ((entries_count < MIN_HASH_SIZE) ||
            (entries_count > MAX_HASH_SIZE)) entries_count = HASH_SIZE;
        table->handles = NULL;
        table->entries_count = entries_count;
        if (!(table->entries = malloc( sizeof(*table->entries) * table->entries_count )))
        {
            set_error( STATUS_NO_MEMORY );
            goto fail;
        }
        memset( table->entries, 0, sizeof(*table->entries) * table->entries_count );
        table->count = 64;
        table->last  = -1;
        if ((table->handles = mem_alloc( sizeof(*table->handles) * table->count )))
            return table;
fail:
        release_object( table );
        table = NULL;
    }
    return table;
}

/* retrieve an entry pointer from its atom */
static struct atom_entry *get_atom_entry( struct atom_table *table, atom_t atom )
{
    struct atom_entry *entry = NULL;
    if (table && (atom >= MIN_STR_ATOM) && (atom <= MIN_STR_ATOM + table->last))
        entry = table->handles[atom - MIN_STR_ATOM];
    if (!entry) set_error( STATUS_INVALID_HANDLE );
    return entry;
}

/* add an atom entry in the table and return its handle */
static atom_t add_atom_entry( struct atom_table *table, struct atom_entry *entry )
{
    int i;
    for (i = 0; i <= table->last; i++)
        if (!table->handles[i]) goto found;
    if (i == table->count)
    {
        struct atom_entry **new_table = NULL;
        int new_size = table->count + table->count / 2;
        if (new_size > MAX_ATOMS) new_size = MAX_ATOMS;
        if (new_size > table->count)
            new_table = realloc( table->handles, sizeof(*table->handles) * new_size );
        if (!new_table)
        {
            set_error( STATUS_NO_MEMORY );
            return 0;
        }
        table->count = new_size;
        table->handles = new_table;
    }
    table->last = i;
 found:
    table->handles[i] = entry;
    entry->atom = i + MIN_STR_ATOM;
    return entry->atom;
}

/* dump an atom table */
static void atom_table_dump( struct object *obj, int verbose )
{
    int i;
    struct atom_table *table = (struct atom_table *)obj;
    assert( obj->ops == &atom_table_ops );

    fprintf( stderr, "Atom table size=%d entries=%d\n",
             table->last + 1, table->entries_count );
    if (!verbose) return;
    for (i = 0; i <= table->last; i++)
    {
        struct atom_entry *entry = table->handles[i];
        if (!entry) continue;
        fprintf( stderr, "  %04x: ref=%d pinned=%c hash=%d \"",
                 entry->atom, entry->count, entry->pinned ? 'Y' : 'N', entry->hash );
        dump_strW( entry->str, entry->len, stderr, "\"\"");
        fprintf( stderr, "\"\n" );
    }
}

/* destroy the atom table */
static void atom_table_destroy( struct object *obj )
{
    int i;
    struct atom_table *table = (struct atom_table *)obj;
    assert( obj->ops == &atom_table_ops );
    if (table->handles)
    {
        for (i = 0; i <= table->last; i++) free( table->handles[i] );
        free( table->handles );
    }
    free( table->entries );
}

/* find an atom entry in its hash list */
static struct atom_entry *find_atom_entry( struct atom_table *table, const struct unicode_str *str,
                                           unsigned short hash )
{
    struct atom_entry *entry = table->entries[hash];
    while (entry)
    {
        if (entry->len == str->len && !memicmp_strW( entry->str, str->str, str->len )) break;
        entry = entry->next;
    }
    return entry;
}

/* add an atom to the table */
static atom_t add_atom( struct atom_table *table, const struct unicode_str *str )
{
    struct atom_entry *entry;
    unsigned short hash = hash_strW( str->str, str->len, table->entries_count );
    atom_t atom = 0;

    if (!str->len)
    {
        set_error( STATUS_OBJECT_NAME_INVALID );
        return 0;
    }
    if (str->len > MAX_ATOM_LEN)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return 0;
    }
    if ((entry = find_atom_entry( table, str, hash )))  /* exists already */
    {
        entry->count++;
        return entry->atom;
    }

    if ((entry = mem_alloc( FIELD_OFFSET( struct atom_entry, str[str->len / sizeof(WCHAR)] ) )))
    {
        if ((atom = add_atom_entry( table, entry )))
        {
            entry->prev  = NULL;
            if ((entry->next = table->entries[hash])) entry->next->prev = entry;
            table->entries[hash] = entry;
            entry->count  = 1;
            entry->pinned = 0;
            entry->hash   = hash;
            entry->len    = str->len;
            memcpy( entry->str, str->str, str->len );
        }
        else free( entry );
    }
    else set_error( STATUS_NO_MEMORY );
    return atom;
}

/* delete an atom from the table */
static void delete_atom( struct atom_table *table, atom_t atom, int if_pinned )
{
    struct atom_entry *entry = get_atom_entry( table, atom );
    if (!entry) return;
    if (entry->pinned && !if_pinned) set_error( STATUS_WAS_LOCKED );
    else if (!--entry->count)
    {
        if (entry->next) entry->next->prev = entry->prev;
        if (entry->prev) entry->prev->next = entry->next;
        else table->entries[entry->hash] = entry->next;
        table->handles[atom - MIN_STR_ATOM] = NULL;
        free( entry );
    }
}

/* find an atom in the table */
static atom_t find_atom( struct atom_table *table, const struct unicode_str *str )
{
    struct atom_entry *entry;

    if (!str->len)
    {
        set_error( STATUS_OBJECT_NAME_INVALID );
        return 0;
    }
    if (str->len > MAX_ATOM_LEN)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return 0;
    }
    if (table && (entry = find_atom_entry( table, str,
                                           hash_strW( str->str, str->len, table->entries_count ))))
        return entry->atom;
    set_error( STATUS_OBJECT_NAME_NOT_FOUND );
    return 0;
}

static struct atom_table *get_global_table( struct winstation *winstation, int create )
{
    struct atom_table *table = winstation ? winstation->atom_table : global_table;
    if (!table)
    {
        if (create)
        {
            table = create_table( HASH_SIZE );
            if (winstation) winstation->atom_table = table;
            else
            {
                global_table = table;
                make_object_permanent( &global_table->obj );
            }
        }
        else set_error( STATUS_OBJECT_NAME_NOT_FOUND );
    }
    return table;
}

/* add an atom in the global table; used for window properties */
atom_t add_global_atom( struct winstation *winstation, const struct unicode_str *str )
{
    struct atom_table *table = get_global_table( winstation, 1 );
    if (!table) return 0;
    return add_atom( table, str );
}

/* find an atom in the global table; used for window properties */
atom_t find_global_atom( struct winstation *winstation, const struct unicode_str *str )
{
    struct atom_table *table = get_global_table( winstation, 0 );
    struct atom_entry *entry;

    if (!str->len || str->len > MAX_ATOM_LEN || !table) return 0;
    if ((entry = find_atom_entry( table, str, hash_strW( str->str, str->len, table->entries_count ))))
        return entry->atom;
    return 0;
}

/* increment the ref count of a global atom; used for window properties */
int grab_global_atom( struct winstation *winstation, atom_t atom )
{
    if (atom >= MIN_STR_ATOM)
    {
        struct atom_table *table = get_global_table( winstation, 0 );
        if (table)
        {
            struct atom_entry *entry = get_atom_entry( table, atom );
            if (entry) entry->count++;
            return (entry != NULL);
        }
        else return 0;
    }
    else return 1;
}

/* decrement the ref count of a global atom; used for window properties */
void release_global_atom( struct winstation *winstation, atom_t atom )
{
    if (atom >= MIN_STR_ATOM)
    {
        struct atom_table *table = get_global_table( winstation, 0 );
        if (table) delete_atom( table, atom, 1 );
    }
}

/* add a global atom */
DECL_HANDLER(add_atom)
{
    struct unicode_str name = get_req_unicode_str();
    struct atom_table *table = get_global_table( NULL, 1 );

    if (table) reply->atom = add_atom( table, &name );
}

/* delete a global atom */
DECL_HANDLER(delete_atom)
{
    struct atom_table *table = get_global_table( NULL, 0 );

    if (table) delete_atom( table, req->atom, 0 );
}

/* find a global atom */
DECL_HANDLER(find_atom)
{
    struct unicode_str name = get_req_unicode_str();
    struct atom_table *table = get_global_table( NULL, 0 );

    if (table) reply->atom = find_atom( table, &name );
}

/* get global atom name */
DECL_HANDLER(get_atom_information)
{
    struct atom_table *table = get_global_table( NULL, 0 );

    if (table)
    {
        struct atom_entry *entry;

        if ((entry = get_atom_entry( table, req->atom )))
        {
            set_reply_data( entry->str, min( entry->len, get_reply_max_size() ));
            reply->count = entry->count;
            reply->pinned = entry->pinned;
            reply->total = entry->len;
        }
        else reply->count = -1;
    }
}
