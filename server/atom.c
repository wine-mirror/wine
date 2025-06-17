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

#define MIN_STR_ATOM  0xc000
#define MAX_ATOMS     0x4000

struct atom_entry
{
    struct atom_entry *next;   /* hash table list */
    struct atom_entry *prev;   /* hash table list */
    int                count;  /* reference count */
    atom_t             atom;   /* atom handle */
    unsigned short     hash;   /* string hash */
    unsigned short     len;    /* string len */
    WCHAR              str[1]; /* atom string */
};

struct atom_table
{
    struct object       obj;                 /* object header */
    int                 count;               /* number of used atoms */
    struct atom_entry  *atoms[MAX_ATOMS];    /* atom entries */
    struct atom_entry  *entries[HASH_SIZE];  /* hash table entries */
};

C_ASSERT( sizeof(struct atom_table) <= 256 * 1024 );

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
    NULL,                         /* satisfied */
    no_signal,                    /* signal */
    no_get_fd,                    /* get_fd */
    default_get_sync,             /* get_sync */
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
static struct atom_table *user_table;

static void add_atom_asciiz( struct atom_table *table, const char *name )
{
    WCHAR buffer[MAX_ATOM_LEN + 1];
    struct unicode_str str = { buffer, strlen( name ) * sizeof(WCHAR) };
    for (int i = 0; i < str.len / sizeof(WCHAR); i++) buffer[i] = name[i];
    add_atom( table, &str );
}

struct object *create_atom_table(void)
{
    struct atom_table *table;

    if (!(table = alloc_object( &atom_table_ops ))) return NULL;
    memset( table->atoms, 0, sizeof(*table->atoms) * ARRAY_SIZE(table->atoms) );
    memset( table->entries, 0, sizeof(*table->entries) * ARRAY_SIZE(table->entries) );
    table->count = 1; /* atom 0xc000 is reserved */

    return &table->obj;
}

void set_global_atom_table( struct object *obj )
{
    assert( obj->ops == &atom_table_ops );
    global_table = (struct atom_table *)obj;
    make_object_permanent( obj );
    grab_object( obj );

    add_atom_asciiz( global_table, "StdExit" );
    add_atom_asciiz( global_table, "StdNewDocument" );
    add_atom_asciiz( global_table, "StdOpenDocument" );
    add_atom_asciiz( global_table, "StdEditDocument" );
    add_atom_asciiz( global_table, "StdNewfromTemplate" );
    add_atom_asciiz( global_table, "StdCloseDocument" );
    add_atom_asciiz( global_table, "StdShowItem" );
    add_atom_asciiz( global_table, "StdDoVerbItem" );
    add_atom_asciiz( global_table, "System" );
    add_atom_asciiz( global_table, "OLEsystem" );
    add_atom_asciiz( global_table, "StdDocumentName" );
    add_atom_asciiz( global_table, "Protocols" );
    add_atom_asciiz( global_table, "Topics" );
    add_atom_asciiz( global_table, "Formats" );
    add_atom_asciiz( global_table, "Status" );
    add_atom_asciiz( global_table, "EditEnvItems" );
    add_atom_asciiz( global_table, "True" );
    add_atom_asciiz( global_table, "False" );
    add_atom_asciiz( global_table, "Change" );
    add_atom_asciiz( global_table, "Save" );
    add_atom_asciiz( global_table, "Close" );
    add_atom_asciiz( global_table, "MSDraw" );
    add_atom_asciiz( global_table, "CC32SubclassInfo" );
}

struct atom_table *get_global_atom_table(void)
{
    return global_table;
}

void set_user_atom_table( struct object *obj )
{
    assert( obj->ops == &atom_table_ops );
    user_table = (struct atom_table *)obj;
    make_object_permanent( obj );
    grab_object( obj );

    add_atom_asciiz( user_table, "USER32" );
    add_atom_asciiz( user_table, "ObjectLink" );
    add_atom_asciiz( user_table, "OwnerLink" );
    add_atom_asciiz( user_table, "Native" );
    add_atom_asciiz( user_table, "Binary" );
    add_atom_asciiz( user_table, "FileName" );
    add_atom_asciiz( user_table, "FileNameW" );
    add_atom_asciiz( user_table, "NetworkName" );
    add_atom_asciiz( user_table, "DataObject" );
    add_atom_asciiz( user_table, "Embedded Object" );
    add_atom_asciiz( user_table, "Embed Source" );
    add_atom_asciiz( user_table, "Custom Link Source" );
    add_atom_asciiz( user_table, "Link Source" );
    add_atom_asciiz( user_table, "Object Descriptor" );
    add_atom_asciiz( user_table, "Link Source Descriptor" );
    add_atom_asciiz( user_table, "OleDraw" );
    add_atom_asciiz( user_table, "PBrush" );
    add_atom_asciiz( user_table, "MSDraw" );
    add_atom_asciiz( user_table, "Ole Private Data" );
    add_atom_asciiz( user_table, "Screen Picture" );
    add_atom_asciiz( user_table, "OleClipboardPersistOnFlush" );
    add_atom_asciiz( user_table, "MoreOlePrivateData" );
    add_atom_asciiz( user_table, "Button" );
    add_atom_asciiz( user_table, "Edit" );
    add_atom_asciiz( user_table, "Static" );
    add_atom_asciiz( user_table, "ListBox" );
    add_atom_asciiz( user_table, "ScrollBar" );
    add_atom_asciiz( user_table, "ComboBox" );
}

struct atom_table *get_user_atom_table(void)
{
    return user_table;
}

/* retrieve an entry pointer from its atom */
static struct atom_entry *get_atom_entry( struct atom_table *table, atom_t atom )
{
    struct atom_entry *entry = NULL;
    if (table && (atom >= MIN_STR_ATOM) && (atom < MIN_STR_ATOM + table->count))
        entry = table->atoms[atom - MIN_STR_ATOM];
    if (!entry) set_error( STATUS_INVALID_HANDLE );
    return entry;
}

/* add an atom entry in the table and return its handle */
static atom_t add_atom_entry( struct atom_table *table, struct atom_entry *entry )
{
    int i;
    for (i = 1 /* atom 0xc000 is reserved */; i < table->count; i++) if (!table->atoms[i]) break;
    if (i == ARRAY_SIZE(table->atoms)) return 0;
    if (i == table->count) table->count++;
    table->atoms[i] = entry;
    entry->atom = i + MIN_STR_ATOM;
    return entry->atom;
}

/* dump an atom table */
static void atom_table_dump( struct object *obj, int verbose )
{
    int i;
    struct atom_table *table = (struct atom_table *)obj;
    assert( obj->ops == &atom_table_ops );

    fprintf( stderr, "Atom table size=%d\n", table->count );
    if (!verbose) return;
    for (i = 0; i < table->count; i++)
    {
        struct atom_entry *entry = table->atoms[i];
        if (!entry) continue;
        fprintf( stderr, "  %04x: ref=%d hash=%d \"",
                 entry->atom, entry->count, entry->hash );
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
    for (i = 0; i < table->count; i++) free( table->atoms[i] );
}

static atom_t get_int_atom_value( const struct unicode_str *name )
{
    const WCHAR *ptr = name->str;
    const WCHAR *end = ptr + name->len / sizeof(WCHAR);
    unsigned int ret = 0;

    if (*ptr++ != '#') return 0;
    while (ptr < end)
    {
        if (*ptr < '0' || *ptr > '9') return 0;
        ret = ret * 10 + *ptr++ - '0';
        if (ret >= MAXINTATOM) return 0;
    }
    return ret;
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
atom_t add_atom( struct atom_table *table, const struct unicode_str *str )
{
    struct atom_entry *entry;
    unsigned short hash;
    atom_t atom = 0;

    if (!str->len)
    {
        set_error( STATUS_OBJECT_NAME_INVALID );
        return 0;
    }
    if (str->len > MAX_ATOM_LEN * sizeof(WCHAR))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return 0;
    }
    if ((atom = get_int_atom_value( str ))) return atom;

    hash = hash_strW( str->str, str->len, ARRAY_SIZE(table->entries) );
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
            entry->hash   = hash;
            entry->len    = str->len;
            memcpy( entry->str, str->str, str->len );
        }
        else
        {
            set_error( STATUS_NO_MEMORY );
            free( entry );
        }
    }

    return atom;
}

/* delete an atom from the table */
static void delete_atom( struct atom_table *table, atom_t atom, int if_pinned )
{
    struct atom_entry *entry = get_atom_entry( table, atom );
    if (!entry) return;
    if (!--entry->count)
    {
        if (entry->next) entry->next->prev = entry->prev;
        if (entry->prev) entry->prev->next = entry->next;
        else table->entries[entry->hash] = entry->next;
        table->atoms[atom - MIN_STR_ATOM] = NULL;
        free( entry );
    }
}

/* find an atom in the table */
atom_t find_atom( struct atom_table *table, const struct unicode_str *str )
{
    struct atom_entry *entry;
    unsigned short hash;
    atom_t atom;

    if (!str->len)
    {
        set_error( STATUS_OBJECT_NAME_INVALID );
        return 0;
    }
    if (str->len > MAX_ATOM_LEN * sizeof(WCHAR))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return 0;
    }
    if ((atom = get_int_atom_value( str ))) return atom;

    hash = hash_strW( str->str, str->len, ARRAY_SIZE(table->entries) );
    if (!(entry = find_atom_entry( table, str, hash )))
    {
        set_error( STATUS_OBJECT_NAME_NOT_FOUND );
        return 0;
    }
    return entry->atom;
}

/* increment the ref count of a global atom; used for window properties */
atom_t grab_atom( struct atom_table *table, atom_t atom )
{
    if (atom >= MIN_STR_ATOM)
    {
        struct atom_entry *entry = get_atom_entry( table, atom );
        if (!entry) return 0;
        entry->count++;
    }
    return atom;
}

/* decrement the ref count of a global atom; used for window properties */
void release_atom( struct atom_table *table, atom_t atom )
{
    if (atom >= MIN_STR_ATOM) delete_atom( table, atom, 1 );
}

/* add a global atom */
DECL_HANDLER(add_atom)
{
    struct unicode_str name = get_req_unicode_str();
    reply->atom = add_atom( global_table, &name );
}

/* delete a global atom */
DECL_HANDLER(delete_atom)
{
    delete_atom( global_table, req->atom, 0 );
}

/* find a global atom */
DECL_HANDLER(find_atom)
{
    struct unicode_str name = get_req_unicode_str();
    reply->atom = find_atom( global_table, &name );
}

/* get global atom name */
DECL_HANDLER(get_atom_information)
{
    struct atom_entry *entry;

    if ((entry = get_atom_entry( global_table, req->atom )))
    {
        set_reply_data( entry->str, min( entry->len, get_reply_max_size() ));
        reply->count = entry->count;
        reply->pinned = 0;
        reply->total = entry->len;
    }
    else reply->count = -1;
}

/* add a user atom */
DECL_HANDLER(add_user_atom)
{
    struct unicode_str name = get_req_unicode_str();
    reply->atom = add_atom( user_table, &name );
}

/* get a user atom name */
DECL_HANDLER(get_user_atom_name)
{
    struct atom_entry *entry;

    if ((entry = get_atom_entry( user_table, req->atom )))
    {
        set_reply_data( (void *)entry->str, min( entry->len, get_reply_max_size() ));
        reply->total = entry->len;
    }
}
