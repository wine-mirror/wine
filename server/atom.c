/*
 * Server-side atom management
 *
 * Copyright (C) 1999, 2000 Alexandre Julliard
 * Copyright (C) 2000 Turchanov Sergei
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "unicode.h"
#include "request.h"
#include "object.h"
#include "process.h"

#define HASH_SIZE     37
#define MIN_HASH_SIZE 4
#define MAX_HASH_SIZE 0x200

#define MAX_ATOM_LEN  255
#define MAX_ATOMS     0x4000

struct atom_entry
{
    struct atom_entry *next;   /* hash table list */
    struct atom_entry *prev;   /* hash table list */
    int                atom;   /* atom handle */
    int                count;  /* reference count */
    int                hash;   /* string hash */
    WCHAR              str[1]; /* atom string */
};

struct atom_table
{
    struct object       obj;                 /* object header */
    int                 count;               /* count of atom handles */
    int                 last;                /* last handle in-use */
    struct atom_entry **handles;             /* atom handles */
    int                 entries_count;       /* humber of hash entries */
    struct atom_entry **entries;             /* hash table entries */
};

static void atom_table_dump( struct object *obj, int verbose );
static void atom_table_destroy( struct object *obj );

static const struct object_ops atom_table_ops =
{
    sizeof(struct atom_table),    /* size */
    atom_table_dump,              /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satified */
    NULL,                         /* get_poll_events */
    NULL,                         /* poll_event */
    no_read_fd,                   /* get_read_fd */
    no_write_fd,                  /* get_write_fd */
    no_flush,                     /* flush */
    no_get_file_info,             /* get_file_info */
    atom_table_destroy            /* destroy */
};

static struct atom_table *global_table;


/* copy an atom name to a temporary area */
static const WCHAR *copy_name( const WCHAR *str )
{
    static WCHAR buffer[MAX_ATOM_LEN+1];
    WCHAR *p = buffer;

    while (p < buffer + sizeof(buffer) - 1) if (!(*p++ = *str++)) break;
    *p = 0;
    return buffer;
}

/* create an atom table */
static struct atom_table *create_table(int entries_count)
{
    struct atom_table *table;

    if ((table = alloc_object( &atom_table_ops, -1 )))
    {
        if ((entries_count < MIN_HASH_SIZE) ||
            (entries_count > MAX_HASH_SIZE)) entries_count = HASH_SIZE;
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
static struct atom_entry *get_atom_entry( struct atom_table *table, int atom )
{
    struct atom_entry *entry = NULL;
    if (table && (atom >= 0) && (atom <= table->last)) entry = table->handles[atom];
    if (!entry) set_error( STATUS_INVALID_HANDLE );
    return entry;
}

/* add an atom entry in the table and return its handle */
static int add_atom_entry( struct atom_table *table, struct atom_entry *entry )
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
            return -1;
        }
        table->count = new_size;
        table->handles = new_table;
    }
    table->last = i;
 found:
    table->handles[i] = entry;
    entry->atom = i;
    return i;
}

/* compute the hash code for a string */
static int atom_hash( struct atom_table *table, const WCHAR *str )
{
    int i;
    WCHAR hash = 0;
    for (i = 0; str[i]; i++) hash ^= toupperW(str[i]) + i;
    return hash % table->entries_count;
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
        fprintf( stderr, "  %5d: ref=%d hash=%d \"", i, entry->count, entry->hash );
        dump_strW( entry->str, strlenW(entry->str), stderr, "\"\"");
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
    if (table->entries) free( table->entries );
}

/* find an atom entry in its hash list */
static struct atom_entry *find_atom_entry( struct atom_table *table, const WCHAR *str, int hash )
{
    struct atom_entry *entry = table->entries[hash];
    while (entry)
    {
        if (!strcmpiW( entry->str, str )) break;
        entry = entry->next;
    }
    return entry;
}

/* close the atom table; used on server exit */
void close_atom_table(void)
{
    if (global_table) release_object( global_table );
}

/* add an atom to the table */
static int add_atom( struct atom_table *table, const WCHAR *str )
{
    struct atom_entry *entry;
    int hash = atom_hash( table, str );
    int atom = -1;

    if (!*str)
    {
        set_error( STATUS_OBJECT_NAME_INVALID );
        return -1;
    }
    if ((entry = find_atom_entry( table, str, hash )))  /* exists already */
    {
        entry->count++;
        return entry->atom;
    }

    if ((entry = mem_alloc( sizeof(*entry) + strlenW(str) * sizeof(WCHAR) )))
    {
        if ((atom = add_atom_entry( table, entry )) != -1)
        {
            entry->prev  = NULL;
            if ((entry->next = table->entries[hash])) entry->next->prev = entry;
            table->entries[hash] = entry;
            entry->count = 1;
            entry->hash  = hash;
            strcpyW( entry->str, str );
        }
        else free( entry );
    }
    else set_error( STATUS_NO_MEMORY );
    return atom;
}

/* delete an atom from the table */
static void delete_atom( struct atom_table *table, int atom )
{
    struct atom_entry *entry = get_atom_entry( table, atom );
    if (entry && !--entry->count)
    {
        if (entry->next) entry->next->prev = entry->prev;
        if (entry->prev) entry->prev->next = entry->next;
        else table->entries[entry->hash] = entry->next;
        table->handles[atom] = NULL;
        free( entry );
    }
}

/* find an atom in the table */
static int find_atom( struct atom_table *table, const WCHAR *str )
{
    struct atom_entry *entry;

    if (table && ((entry = find_atom_entry( table, str, atom_hash(table, str) )))) return entry->atom;
    if (!*str) set_error( STATUS_OBJECT_NAME_INVALID );
    else set_error( STATUS_OBJECT_NAME_NOT_FOUND );
    return -1;
}

/* get an atom name and refcount*/
static int get_atom_name( struct atom_table *table, int atom, WCHAR *str )
{
    int count = -1;
    struct atom_entry *entry = get_atom_entry( table, atom );
    if (entry)
    {
        strcpyW( str, entry->str );
        count = entry->count;
    }
    return count;
}

/* add a global atom */
DECL_HANDLER(add_atom)
{
    struct atom_table **table_ptr = req->local ? &current->process->atom_table : &global_table;

    if (!*table_ptr) *table_ptr = create_table(0);
    if (*table_ptr) req->atom = add_atom( *table_ptr, copy_name( req->name ) );
}

/* delete a global atom */
DECL_HANDLER(delete_atom)
{
    delete_atom( req->local ? current->process->atom_table : global_table,
                 req->atom );
}

/* find a global atom */
DECL_HANDLER(find_atom)
{
    req->atom = find_atom( req->local ? current->process->atom_table : global_table,
                           copy_name( req->name ) );
}

/* get global atom name */
DECL_HANDLER(get_atom_name)
{
    req->name[0] = 0;
    req->count = get_atom_name( req->local ? current->process->atom_table : global_table,
                                req->atom, req->name );
}

/* init the process atom table */
DECL_HANDLER(init_atom_table)
{
    if (!current->process->atom_table)
        current->process->atom_table = create_table( req->entries );
}
