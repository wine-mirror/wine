/*
 * Server-side registry management
 *
 * Copyright (C) 1999 Alexandre Julliard
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

/* To do:
 * - symbolic links
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "object.h"
#include "file.h"
#include "handle.h"
#include "request.h"
#include "unicode.h"
#include "security.h"

#include "winternl.h"
#include "wine/library.h"

struct notify
{
    struct list       entry;    /* entry in list of notifications */
    struct event     *event;    /* event to set when changing this key */
    int               subtree;  /* true if subtree notification */
    unsigned int      filter;   /* which events to notify on */
    obj_handle_t      hkey;     /* hkey associated with this notification */
    struct process   *process;  /* process in which the hkey is valid */
};

/* a registry key */
struct key
{
    struct object     obj;         /* object header */
    WCHAR            *name;        /* key name */
    WCHAR            *class;       /* key class */
    unsigned short    namelen;     /* length of key name */
    unsigned short    classlen;    /* length of class name */
    struct key       *parent;      /* parent key */
    int               last_subkey; /* last in use subkey */
    int               nb_subkeys;  /* count of allocated subkeys */
    struct key      **subkeys;     /* subkeys array */
    int               last_value;  /* last in use value */
    int               nb_values;   /* count of allocated values in array */
    struct key_value *values;      /* values array */
    unsigned int      flags;       /* flags */
    time_t            modif;       /* last modification time */
    struct list       notify_list; /* list of notifications */
};

/* key flags */
#define KEY_VOLATILE 0x0001  /* key is volatile (not saved to disk) */
#define KEY_DELETED  0x0002  /* key has been deleted */
#define KEY_DIRTY    0x0004  /* key has been modified */

/* a key value */
struct key_value
{
    WCHAR            *name;    /* value name */
    unsigned short    namelen; /* length of value name */
    unsigned short    type;    /* value type */
    data_size_t       len;     /* value data length in bytes */
    void             *data;    /* pointer to value data */
};

#define MIN_SUBKEYS  8   /* min. number of allocated subkeys per key */
#define MIN_VALUES   8   /* min. number of allocated values per key */

#define MAX_NAME_LEN  MAX_PATH  /* max. length of a key name */
#define MAX_VALUE_LEN MAX_PATH  /* max. length of a value name */

/* the root of the registry tree */
static struct key *root_key;

static const timeout_t save_period = 30 * -TICKS_PER_SEC;  /* delay between periodic saves */
static struct timeout_user *save_timeout_user;  /* saving timer */

static void set_periodic_save_timer(void);

/* information about where to save a registry branch */
struct save_branch_info
{
    struct key  *key;
    char        *path;
};

#define MAX_SAVE_BRANCH_INFO 3
static int save_branch_count;
static struct save_branch_info save_branch_info[MAX_SAVE_BRANCH_INFO];


/* information about a file being loaded */
struct file_load_info
{
    const char *filename; /* input file name */
    FILE       *file;     /* input file */
    char       *buffer;   /* line buffer */
    int         len;      /* buffer length */
    int         line;     /* current input line */
    WCHAR      *tmp;      /* temp buffer to use while parsing input */
    size_t      tmplen;   /* length of temp buffer */
};


static void key_dump( struct object *obj, int verbose );
static unsigned int key_map_access( struct object *obj, unsigned int access );
static int key_close_handle( struct object *obj, struct process *process, obj_handle_t handle );
static void key_destroy( struct object *obj );

static const struct object_ops key_ops =
{
    sizeof(struct key),      /* size */
    key_dump,                /* dump */
    no_add_queue,            /* add_queue */
    NULL,                    /* remove_queue */
    NULL,                    /* signaled */
    NULL,                    /* satisfied */
    no_signal,               /* signal */
    no_get_fd,               /* get_fd */
    key_map_access,          /* map_access */
    no_lookup_name,          /* lookup_name */
    no_open_file,            /* open_file */
    key_close_handle,        /* close_handle */
    key_destroy              /* destroy */
};


/*
 * The registry text file format v2 used by this code is similar to the one
 * used by REGEDIT import/export functionality, with the following differences:
 * - strings and key names can contain \x escapes for Unicode
 * - key names use escapes too in order to support Unicode
 * - the modification time optionally follows the key name
 * - REG_EXPAND_SZ and REG_MULTI_SZ are saved as strings instead of hex
 */

static inline char to_hex( char ch )
{
    if (isdigit(ch)) return ch - '0';
    return tolower(ch) - 'a' + 10;
}

/* dump the full path of a key */
static void dump_path( const struct key *key, const struct key *base, FILE *f )
{
    if (key->parent && key->parent != base)
    {
        dump_path( key->parent, base, f );
        fprintf( f, "\\\\" );
    }
    dump_strW( key->name, key->namelen / sizeof(WCHAR), f, "[]" );
}

/* dump a value to a text file */
static void dump_value( const struct key_value *value, FILE *f )
{
    unsigned int i;
    int count;

    if (value->namelen)
    {
        fputc( '\"', f );
        count = 1 + dump_strW( value->name, value->namelen / sizeof(WCHAR), f, "\"\"" );
        count += fprintf( f, "\"=" );
    }
    else count = fprintf( f, "@=" );

    switch(value->type)
    {
    case REG_SZ:
    case REG_EXPAND_SZ:
    case REG_MULTI_SZ:
        if (value->type != REG_SZ) fprintf( f, "str(%d):", value->type );
        fputc( '\"', f );
        if (value->data) dump_strW( (WCHAR *)value->data, value->len / sizeof(WCHAR), f, "\"\"" );
        fputc( '\"', f );
        break;
    case REG_DWORD:
        if (value->len == sizeof(DWORD))
        {
            DWORD dw;
            memcpy( &dw, value->data, sizeof(DWORD) );
            fprintf( f, "dword:%08x", dw );
            break;
        }
        /* else fall through */
    default:
        if (value->type == REG_BINARY) count += fprintf( f, "hex:" );
        else count += fprintf( f, "hex(%x):", value->type );
        for (i = 0; i < value->len; i++)
        {
            count += fprintf( f, "%02x", *((unsigned char *)value->data + i) );
            if (i < value->len-1)
            {
                fputc( ',', f );
                if (++count > 76)
                {
                    fprintf( f, "\\\n  " );
                    count = 2;
                }
            }
        }
        break;
    }
    fputc( '\n', f );
}

/* save a registry and all its subkeys to a text file */
static void save_subkeys( const struct key *key, const struct key *base, FILE *f )
{
    int i;

    if (key->flags & KEY_VOLATILE) return;
    /* save key if it has either some values or no subkeys */
    /* keys with no values but subkeys are saved implicitly by saving the subkeys */
    if ((key->last_value >= 0) || (key->last_subkey == -1))
    {
        fprintf( f, "\n[" );
        if (key != base) dump_path( key, base, f );
        fprintf( f, "] %ld\n", (long)key->modif );
        for (i = 0; i <= key->last_value; i++) dump_value( &key->values[i], f );
    }
    for (i = 0; i <= key->last_subkey; i++) save_subkeys( key->subkeys[i], base, f );
}

static void dump_operation( const struct key *key, const struct key_value *value, const char *op )
{
    fprintf( stderr, "%s key ", op );
    if (key) dump_path( key, NULL, stderr );
    else fprintf( stderr, "ERROR" );
    if (value)
    {
        fprintf( stderr, " value ");
        dump_value( value, stderr );
    }
    else fprintf( stderr, "\n" );
}

static void key_dump( struct object *obj, int verbose )
{
    struct key *key = (struct key *)obj;
    assert( obj->ops == &key_ops );
    fprintf( stderr, "Key flags=%x ", key->flags );
    dump_path( key, NULL, stderr );
    fprintf( stderr, "\n" );
}

/* notify waiter and maybe delete the notification */
static void do_notification( struct key *key, struct notify *notify, int del )
{
    if (notify->event)
    {
        set_event( notify->event );
        release_object( notify->event );
        notify->event = NULL;
    }
    if (del)
    {
        list_remove( &notify->entry );
        free( notify );
    }
}

static inline struct notify *find_notify( struct key *key, struct process *process, obj_handle_t hkey )
{
    struct notify *notify;

    LIST_FOR_EACH_ENTRY( notify, &key->notify_list, struct notify, entry )
    {
        if (notify->process == process && notify->hkey == hkey) return notify;
    }
    return NULL;
}

static unsigned int key_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= KEY_READ;
    if (access & GENERIC_WRITE)   access |= KEY_WRITE;
    if (access & GENERIC_EXECUTE) access |= KEY_EXECUTE;
    if (access & GENERIC_ALL)     access |= KEY_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

/* close the notification associated with a handle */
static int key_close_handle( struct object *obj, struct process *process, obj_handle_t handle )
{
    struct key * key = (struct key *) obj;
    struct notify *notify = find_notify( key, process, handle );
    if (notify) do_notification( key, notify, 1 );
    return 1;  /* ok to close */
}

static void key_destroy( struct object *obj )
{
    int i;
    struct list *ptr;
    struct key *key = (struct key *)obj;
    assert( obj->ops == &key_ops );

    free( key->name );
    free( key->class );
    for (i = 0; i <= key->last_value; i++)
    {
        if (key->values[i].name) free( key->values[i].name );
        if (key->values[i].data) free( key->values[i].data );
    }
    free( key->values );
    for (i = 0; i <= key->last_subkey; i++)
    {
        key->subkeys[i]->parent = NULL;
        release_object( key->subkeys[i] );
    }
    free( key->subkeys );
    /* unconditionally notify everything waiting on this key */
    while ((ptr = list_head( &key->notify_list )))
    {
        struct notify *notify = LIST_ENTRY( ptr, struct notify, entry );
        do_notification( key, notify, 1 );
    }
}

/* get the request vararg as registry path */
static inline void get_req_path( struct unicode_str *str, int skip_root )
{
    static const WCHAR root_name[] = { '\\','R','e','g','i','s','t','r','y','\\' };

    str->str = get_req_data();
    str->len = (get_req_data_size() / sizeof(WCHAR)) * sizeof(WCHAR);

    if (skip_root && str->len >= sizeof(root_name) &&
        !memicmpW( str->str, root_name, sizeof(root_name)/sizeof(WCHAR) ))
    {
        str->str += sizeof(root_name)/sizeof(WCHAR);
        str->len -= sizeof(root_name);
    }
}

/* return the next token in a given path */
/* token->str must point inside the path, or be NULL for the first call */
static struct unicode_str *get_path_token( const struct unicode_str *path, struct unicode_str *token )
{
    data_size_t i = 0, len = path->len / sizeof(WCHAR);

    if (!token->str)  /* first time */
    {
        /* path cannot start with a backslash */
        if (len && path->str[0] == '\\')
        {
            set_error( STATUS_OBJECT_PATH_INVALID );
            return NULL;
        }
    }
    else
    {
        i = token->str - path->str;
        i += token->len / sizeof(WCHAR);
        while (i < len && path->str[i] == '\\') i++;
    }
    token->str = path->str + i;
    while (i < len && path->str[i] != '\\') i++;
    token->len = (path->str + i - token->str) * sizeof(WCHAR);
    return token;
}

/* allocate a key object */
static struct key *alloc_key( const struct unicode_str *name, time_t modif )
{
    struct key *key;
    if ((key = alloc_object( &key_ops )))
    {
        key->name        = NULL;
        key->class       = NULL;
        key->namelen     = name->len;
        key->classlen    = 0;
        key->flags       = 0;
        key->last_subkey = -1;
        key->nb_subkeys  = 0;
        key->subkeys     = NULL;
        key->nb_values   = 0;
        key->last_value  = -1;
        key->values      = NULL;
        key->modif       = modif;
        key->parent      = NULL;
        list_init( &key->notify_list );
        if (name->len && !(key->name = memdup( name->str, name->len )))
        {
            release_object( key );
            key = NULL;
        }
    }
    return key;
}

/* mark a key and all its parents as dirty (modified) */
static void make_dirty( struct key *key )
{
    while (key)
    {
        if (key->flags & (KEY_DIRTY|KEY_VOLATILE)) return;  /* nothing to do */
        key->flags |= KEY_DIRTY;
        key = key->parent;
    }
}

/* mark a key and all its subkeys as clean (not modified) */
static void make_clean( struct key *key )
{
    int i;

    if (key->flags & KEY_VOLATILE) return;
    if (!(key->flags & KEY_DIRTY)) return;
    key->flags &= ~KEY_DIRTY;
    for (i = 0; i <= key->last_subkey; i++) make_clean( key->subkeys[i] );
}

/* go through all the notifications and send them if necessary */
static void check_notify( struct key *key, unsigned int change, int not_subtree )
{
    struct list *ptr, *next;

    LIST_FOR_EACH_SAFE( ptr, next, &key->notify_list )
    {
        struct notify *n = LIST_ENTRY( ptr, struct notify, entry );
        if ( ( not_subtree || n->subtree ) && ( change & n->filter ) )
            do_notification( key, n, 0 );
    }
}

/* update key modification time */
static void touch_key( struct key *key, unsigned int change )
{
    struct key *k;

    key->modif = time(NULL);
    make_dirty( key );

    /* do notifications */
    check_notify( key, change, 1 );
    for ( k = key->parent; k; k = k->parent )
        check_notify( k, change & ~REG_NOTIFY_CHANGE_LAST_SET, 0 );
}

/* try to grow the array of subkeys; return 1 if OK, 0 on error */
static int grow_subkeys( struct key *key )
{
    struct key **new_subkeys;
    int nb_subkeys;

    if (key->nb_subkeys)
    {
        nb_subkeys = key->nb_subkeys + (key->nb_subkeys / 2);  /* grow by 50% */
        if (!(new_subkeys = realloc( key->subkeys, nb_subkeys * sizeof(*new_subkeys) )))
        {
            set_error( STATUS_NO_MEMORY );
            return 0;
        }
    }
    else
    {
        nb_subkeys = MIN_VALUES;
        if (!(new_subkeys = mem_alloc( nb_subkeys * sizeof(*new_subkeys) ))) return 0;
    }
    key->subkeys    = new_subkeys;
    key->nb_subkeys = nb_subkeys;
    return 1;
}

/* allocate a subkey for a given key, and return its index */
static struct key *alloc_subkey( struct key *parent, const struct unicode_str *name,
                                 int index, time_t modif )
{
    struct key *key;
    int i;

    if (name->len > MAX_NAME_LEN * sizeof(WCHAR))
    {
        set_error( STATUS_NAME_TOO_LONG );
        return NULL;
    }
    if (parent->last_subkey + 1 == parent->nb_subkeys)
    {
        /* need to grow the array */
        if (!grow_subkeys( parent )) return NULL;
    }
    if ((key = alloc_key( name, modif )) != NULL)
    {
        key->parent = parent;
        for (i = ++parent->last_subkey; i > index; i--)
            parent->subkeys[i] = parent->subkeys[i-1];
        parent->subkeys[index] = key;
    }
    return key;
}

/* free a subkey of a given key */
static void free_subkey( struct key *parent, int index )
{
    struct key *key;
    int i, nb_subkeys;

    assert( index >= 0 );
    assert( index <= parent->last_subkey );

    key = parent->subkeys[index];
    for (i = index; i < parent->last_subkey; i++) parent->subkeys[i] = parent->subkeys[i + 1];
    parent->last_subkey--;
    key->flags |= KEY_DELETED;
    key->parent = NULL;
    release_object( key );

    /* try to shrink the array */
    nb_subkeys = parent->nb_subkeys;
    if (nb_subkeys > MIN_SUBKEYS && parent->last_subkey < nb_subkeys / 2)
    {
        struct key **new_subkeys;
        nb_subkeys -= nb_subkeys / 3;  /* shrink by 33% */
        if (nb_subkeys < MIN_SUBKEYS) nb_subkeys = MIN_SUBKEYS;
        if (!(new_subkeys = realloc( parent->subkeys, nb_subkeys * sizeof(*new_subkeys) ))) return;
        parent->subkeys = new_subkeys;
        parent->nb_subkeys = nb_subkeys;
    }
}

/* find the named child of a given key and return its index */
static struct key *find_subkey( const struct key *key, const struct unicode_str *name, int *index )
{
    int i, min, max, res;
    data_size_t len;

    min = 0;
    max = key->last_subkey;
    while (min <= max)
    {
        i = (min + max) / 2;
        len = min( key->subkeys[i]->namelen, name->len );
        res = memicmpW( key->subkeys[i]->name, name->str, len / sizeof(WCHAR) );
        if (!res) res = key->subkeys[i]->namelen - name->len;
        if (!res)
        {
            *index = i;
            return key->subkeys[i];
        }
        if (res > 0) max = i - 1;
        else min = i + 1;
    }
    *index = min;  /* this is where we should insert it */
    return NULL;
}

/* open a subkey */
static struct key *open_key( struct key *key, const struct unicode_str *name )
{
    int index;
    struct unicode_str token;

    token.str = NULL;
    if (!get_path_token( name, &token )) return NULL;
    while (token.len)
    {
        if (!(key = find_subkey( key, &token, &index )))
        {
            set_error( STATUS_OBJECT_NAME_NOT_FOUND );
            break;
        }
        get_path_token( name, &token );
    }

    if (debug_level > 1) dump_operation( key, NULL, "Open" );
    if (key) grab_object( key );
    return key;
}

/* create a subkey */
static struct key *create_key( struct key *key, const struct unicode_str *name,
                               const struct unicode_str *class, int flags, time_t modif, int *created )
{
    struct key *base;
    int index;
    struct unicode_str token;

    if (key->flags & KEY_DELETED) /* we cannot create a subkey under a deleted key */
    {
        set_error( STATUS_KEY_DELETED );
        return NULL;
    }
    if (!(flags & KEY_VOLATILE) && (key->flags & KEY_VOLATILE))
    {
        set_error( STATUS_CHILD_MUST_BE_VOLATILE );
        return NULL;
    }
    if (!modif) modif = time(NULL);

    token.str = NULL;
    if (!get_path_token( name, &token )) return NULL;
    *created = 0;
    while (token.len)
    {
        struct key *subkey;
        if (!(subkey = find_subkey( key, &token, &index ))) break;
        key = subkey;
        get_path_token( name, &token );
    }

    /* create the remaining part */

    if (!token.len) goto done;
    *created = 1;
    if (flags & KEY_DIRTY) make_dirty( key );
    if (!(key = alloc_subkey( key, &token, index, modif ))) return NULL;
    base = key;
    for (;;)
    {
        key->flags |= flags;
        get_path_token( name, &token );
        if (!token.len) break;
        /* we know the index is always 0 in a new key */
        if (!(key = alloc_subkey( key, &token, 0, modif )))
        {
            free_subkey( base, index );
            return NULL;
        }
    }

 done:
    if (debug_level > 1) dump_operation( key, NULL, "Create" );
    if (class && class->len)
    {
        key->classlen = class->len;
        free(key->class);
        if (!(key->class = memdup( class->str, key->classlen ))) key->classlen = 0;
    }
    grab_object( key );
    return key;
}

/* query information about a key or a subkey */
static void enum_key( const struct key *key, int index, int info_class,
                      struct enum_key_reply *reply )
{
    int i;
    data_size_t len, namelen, classlen;
    data_size_t max_subkey = 0, max_class = 0;
    data_size_t max_value = 0, max_data = 0;
    char *data;

    if (index != -1)  /* -1 means use the specified key directly */
    {
        if ((index < 0) || (index > key->last_subkey))
        {
            set_error( STATUS_NO_MORE_ENTRIES );
            return;
        }
        key = key->subkeys[index];
    }

    namelen = key->namelen;
    classlen = key->classlen;

    switch(info_class)
    {
    case KeyBasicInformation:
        classlen = 0; /* only return the name */
        /* fall through */
    case KeyNodeInformation:
        reply->max_subkey = 0;
        reply->max_class  = 0;
        reply->max_value  = 0;
        reply->max_data   = 0;
        break;
    case KeyFullInformation:
        for (i = 0; i <= key->last_subkey; i++)
        {
            struct key *subkey = key->subkeys[i];
            len = subkey->namelen / sizeof(WCHAR);
            if (len > max_subkey) max_subkey = len;
            len = subkey->classlen / sizeof(WCHAR);
            if (len > max_class) max_class = len;
        }
        for (i = 0; i <= key->last_value; i++)
        {
            len = key->values[i].namelen / sizeof(WCHAR);
            if (len > max_value) max_value = len;
            len = key->values[i].len;
            if (len > max_data) max_data = len;
        }
        reply->max_subkey = max_subkey;
        reply->max_class  = max_class;
        reply->max_value  = max_value;
        reply->max_data   = max_data;
        namelen = 0;  /* only return the class */
        break;
    default:
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    reply->subkeys = key->last_subkey + 1;
    reply->values  = key->last_value + 1;
    reply->modif   = key->modif;
    reply->total   = namelen + classlen;

    len = min( reply->total, get_reply_max_size() );
    if (len && (data = set_reply_data_size( len )))
    {
        if (len > namelen)
        {
            reply->namelen = namelen;
            memcpy( data, key->name, namelen );
            memcpy( data + namelen, key->class, len - namelen );
        }
        else
        {
            reply->namelen = len;
            memcpy( data, key->name, len );
        }
    }
    if (debug_level > 1) dump_operation( key, NULL, "Enum" );
}

/* delete a key and its values */
static int delete_key( struct key *key, int recurse )
{
    int index;
    struct key *parent;

    /* must find parent and index */
    if (key == root_key)
    {
        set_error( STATUS_ACCESS_DENIED );
        return -1;
    }
    if (!(parent = key->parent) || (key->flags & KEY_DELETED))
    {
        set_error( STATUS_KEY_DELETED );
        return -1;
    }

    while (recurse && (key->last_subkey>=0))
        if (0 > delete_key(key->subkeys[key->last_subkey], 1))
            return -1;

    for (index = 0; index <= parent->last_subkey; index++)
        if (parent->subkeys[index] == key) break;
    assert( index <= parent->last_subkey );

    /* we can only delete a key that has no subkeys */
    if (key->last_subkey >= 0)
    {
        set_error( STATUS_ACCESS_DENIED );
        return -1;
    }

    if (debug_level > 1) dump_operation( key, NULL, "Delete" );
    free_subkey( parent, index );
    touch_key( parent, REG_NOTIFY_CHANGE_NAME );
    return 0;
}

/* try to grow the array of values; return 1 if OK, 0 on error */
static int grow_values( struct key *key )
{
    struct key_value *new_val;
    int nb_values;

    if (key->nb_values)
    {
        nb_values = key->nb_values + (key->nb_values / 2);  /* grow by 50% */
        if (!(new_val = realloc( key->values, nb_values * sizeof(*new_val) )))
        {
            set_error( STATUS_NO_MEMORY );
            return 0;
        }
    }
    else
    {
        nb_values = MIN_VALUES;
        if (!(new_val = mem_alloc( nb_values * sizeof(*new_val) ))) return 0;
    }
    key->values = new_val;
    key->nb_values = nb_values;
    return 1;
}

/* find the named value of a given key and return its index in the array */
static struct key_value *find_value( const struct key *key, const struct unicode_str *name, int *index )
{
    int i, min, max, res;
    data_size_t len;

    min = 0;
    max = key->last_value;
    while (min <= max)
    {
        i = (min + max) / 2;
        len = min( key->values[i].namelen, name->len );
        res = memicmpW( key->values[i].name, name->str, len / sizeof(WCHAR) );
        if (!res) res = key->values[i].namelen - name->len;
        if (!res)
        {
            *index = i;
            return &key->values[i];
        }
        if (res > 0) max = i - 1;
        else min = i + 1;
    }
    *index = min;  /* this is where we should insert it */
    return NULL;
}

/* insert a new value; the index must have been returned by find_value */
static struct key_value *insert_value( struct key *key, const struct unicode_str *name, int index )
{
    struct key_value *value;
    WCHAR *new_name = NULL;
    int i;

    if (name->len > MAX_VALUE_LEN * sizeof(WCHAR))
    {
        set_error( STATUS_NAME_TOO_LONG );
        return NULL;
    }
    if (key->last_value + 1 == key->nb_values)
    {
        if (!grow_values( key )) return NULL;
    }
    if (name->len && !(new_name = memdup( name->str, name->len ))) return NULL;
    for (i = ++key->last_value; i > index; i--) key->values[i] = key->values[i - 1];
    value = &key->values[index];
    value->name    = new_name;
    value->namelen = name->len;
    value->len     = 0;
    value->data    = NULL;
    return value;
}

/* set a key value */
static void set_value( struct key *key, const struct unicode_str *name,
                       int type, const void *data, data_size_t len )
{
    struct key_value *value;
    void *ptr = NULL;
    int index;

    if ((value = find_value( key, name, &index )))
    {
        /* check if the new value is identical to the existing one */
        if (value->type == type && value->len == len &&
            value->data && !memcmp( value->data, data, len ))
        {
            if (debug_level > 1) dump_operation( key, value, "Skip setting" );
            return;
        }
    }

    if (len && !(ptr = memdup( data, len ))) return;

    if (!value)
    {
        if (!(value = insert_value( key, name, index )))
        {
            free( ptr );
            return;
        }
    }
    else free( value->data ); /* already existing, free previous data */

    value->type  = type;
    value->len   = len;
    value->data  = ptr;
    touch_key( key, REG_NOTIFY_CHANGE_LAST_SET );
    if (debug_level > 1) dump_operation( key, value, "Set" );
}

/* get a key value */
static void get_value( struct key *key, const struct unicode_str *name, int *type, data_size_t *len )
{
    struct key_value *value;
    int index;

    if ((value = find_value( key, name, &index )))
    {
        *type = value->type;
        *len  = value->len;
        if (value->data) set_reply_data( value->data, min( value->len, get_reply_max_size() ));
        if (debug_level > 1) dump_operation( key, value, "Get" );
    }
    else
    {
        *type = -1;
        set_error( STATUS_OBJECT_NAME_NOT_FOUND );
    }
}

/* enumerate a key value */
static void enum_value( struct key *key, int i, int info_class, struct enum_key_value_reply *reply )
{
    struct key_value *value;

    if (i < 0 || i > key->last_value) set_error( STATUS_NO_MORE_ENTRIES );
    else
    {
        void *data;
        data_size_t namelen, maxlen;

        value = &key->values[i];
        reply->type = value->type;
        namelen = value->namelen;

        switch(info_class)
        {
        case KeyValueBasicInformation:
            reply->total = namelen;
            break;
        case KeyValueFullInformation:
            reply->total = namelen + value->len;
            break;
        case KeyValuePartialInformation:
            reply->total = value->len;
            namelen = 0;
            break;
        default:
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }

        maxlen = min( reply->total, get_reply_max_size() );
        if (maxlen && ((data = set_reply_data_size( maxlen ))))
        {
            if (maxlen > namelen)
            {
                reply->namelen = namelen;
                memcpy( data, value->name, namelen );
                memcpy( (char *)data + namelen, value->data, maxlen - namelen );
            }
            else
            {
                reply->namelen = maxlen;
                memcpy( data, value->name, maxlen );
            }
        }
        if (debug_level > 1) dump_operation( key, value, "Enum" );
    }
}

/* delete a value */
static void delete_value( struct key *key, const struct unicode_str *name )
{
    struct key_value *value;
    int i, index, nb_values;

    if (!(value = find_value( key, name, &index )))
    {
        set_error( STATUS_OBJECT_NAME_NOT_FOUND );
        return;
    }
    if (debug_level > 1) dump_operation( key, value, "Delete" );
    free( value->name );
    free( value->data );
    for (i = index; i < key->last_value; i++) key->values[i] = key->values[i + 1];
    key->last_value--;
    touch_key( key, REG_NOTIFY_CHANGE_LAST_SET );

    /* try to shrink the array */
    nb_values = key->nb_values;
    if (nb_values > MIN_VALUES && key->last_value < nb_values / 2)
    {
        struct key_value *new_val;
        nb_values -= nb_values / 3;  /* shrink by 33% */
        if (nb_values < MIN_VALUES) nb_values = MIN_VALUES;
        if (!(new_val = realloc( key->values, nb_values * sizeof(*new_val) ))) return;
        key->values = new_val;
        key->nb_values = nb_values;
    }
}

/* get the registry key corresponding to an hkey handle */
static inline struct key *get_hkey_obj( obj_handle_t hkey, unsigned int access )
{
    return (struct key *)get_handle_obj( current->process, hkey, access, &key_ops );
}

/* get the registry key corresponding to a parent key handle */
static inline struct key *get_parent_hkey_obj( obj_handle_t hkey )
{
    if (!hkey) return (struct key *)grab_object( root_key );
    return (struct key *)get_handle_obj( current->process, hkey, 0, &key_ops );
}

/* read a line from the input file */
static int read_next_line( struct file_load_info *info )
{
    char *newbuf;
    int newlen, pos = 0;

    info->line++;
    for (;;)
    {
        if (!fgets( info->buffer + pos, info->len - pos, info->file ))
            return (pos != 0);  /* EOF */
        pos = strlen(info->buffer);
        if (info->buffer[pos-1] == '\n')
        {
            /* got a full line */
            info->buffer[--pos] = 0;
            if (pos > 0 && info->buffer[pos-1] == '\r') info->buffer[pos-1] = 0;
            return 1;
        }
        if (pos < info->len - 1) return 1;  /* EOF but something was read */

        /* need to enlarge the buffer */
        newlen = info->len + info->len / 2;
        if (!(newbuf = realloc( info->buffer, newlen )))
        {
            set_error( STATUS_NO_MEMORY );
            return -1;
        }
        info->buffer = newbuf;
        info->len = newlen;
    }
}

/* make sure the temp buffer holds enough space */
static int get_file_tmp_space( struct file_load_info *info, size_t size )
{
    WCHAR *tmp;
    if (info->tmplen >= size) return 1;
    if (!(tmp = realloc( info->tmp, size )))
    {
        set_error( STATUS_NO_MEMORY );
        return 0;
    }
    info->tmp = tmp;
    info->tmplen = size;
    return 1;
}

/* report an error while loading an input file */
static void file_read_error( const char *err, struct file_load_info *info )
{
    if (info->filename)
        fprintf( stderr, "%s:%d: %s '%s'\n", info->filename, info->line, err, info->buffer );
    else
        fprintf( stderr, "<fd>:%d: %s '%s'\n", info->line, err, info->buffer );
}

/* parse an escaped string back into Unicode */
/* return the number of chars read from the input, or -1 on output overflow */
static int parse_strW( WCHAR *dest, data_size_t *len, const char *src, char endchar )
{
    data_size_t count = sizeof(WCHAR);  /* for terminating null */
    const char *p = src;
    while (*p && *p != endchar)
    {
        if (*p != '\\') *dest = (WCHAR)*p++;
        else
        {
            p++;
            switch(*p)
            {
            case 'a': *dest = '\a'; p++; break;
            case 'b': *dest = '\b'; p++; break;
            case 'e': *dest = '\e'; p++; break;
            case 'f': *dest = '\f'; p++; break;
            case 'n': *dest = '\n'; p++; break;
            case 'r': *dest = '\r'; p++; break;
            case 't': *dest = '\t'; p++; break;
            case 'v': *dest = '\v'; p++; break;
            case 'x':  /* hex escape */
                p++;
                if (!isxdigit(*p)) *dest = 'x';
                else
                {
                    *dest = to_hex(*p++);
                    if (isxdigit(*p)) *dest = (*dest * 16) + to_hex(*p++);
                    if (isxdigit(*p)) *dest = (*dest * 16) + to_hex(*p++);
                    if (isxdigit(*p)) *dest = (*dest * 16) + to_hex(*p++);
                }
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':  /* octal escape */
                *dest = *p++ - '0';
                if (*p >= '0' && *p <= '7') *dest = (*dest * 8) + (*p++ - '0');
                if (*p >= '0' && *p <= '7') *dest = (*dest * 8) + (*p++ - '0');
                break;
            default:
                *dest = (WCHAR)*p++;
                break;
            }
        }
        if ((count += sizeof(WCHAR)) > *len) return -1;  /* dest buffer overflow */
        dest++;
    }
    *dest = 0;
    if (!*p) return -1;  /* delimiter not found */
    *len = count;
    return p + 1 - src;
}

/* convert a data type tag to a value type */
static int get_data_type( const char *buffer, int *type, int *parse_type )
{
    struct data_type { const char *tag; int len; int type; int parse_type; };

    static const struct data_type data_types[] =
    {                   /* actual type */  /* type to assume for parsing */
        { "\"",        1,   REG_SZ,              REG_SZ },
        { "str:\"",    5,   REG_SZ,              REG_SZ },
        { "str(2):\"", 8,   REG_EXPAND_SZ,       REG_SZ },
        { "str(7):\"", 8,   REG_MULTI_SZ,        REG_SZ },
        { "hex:",      4,   REG_BINARY,          REG_BINARY },
        { "dword:",    6,   REG_DWORD,           REG_DWORD },
        { "hex(",      4,   -1,                  REG_BINARY },
        { NULL,        0,    0,                  0 }
    };

    const struct data_type *ptr;
    char *end;

    for (ptr = data_types; ptr->tag; ptr++)
    {
        if (memcmp( ptr->tag, buffer, ptr->len )) continue;
        *parse_type = ptr->parse_type;
        if ((*type = ptr->type) != -1) return ptr->len;
        /* "hex(xx):" is special */
        *type = (int)strtoul( buffer + 4, &end, 16 );
        if ((end <= buffer) || memcmp( end, "):", 2 )) return 0;
        return end + 2 - buffer;
    }
    return 0;
}

/* load and create a key from the input file */
static struct key *load_key( struct key *base, const char *buffer, int flags,
                             int prefix_len, struct file_load_info *info,
                             int default_modif )
{
    WCHAR *p;
    struct unicode_str name;
    int res, modif;
    data_size_t len = strlen(buffer) * sizeof(WCHAR);

    if (!get_file_tmp_space( info, len )) return NULL;

    if ((res = parse_strW( info->tmp, &len, buffer, ']' )) == -1)
    {
        file_read_error( "Malformed key", info );
        return NULL;
    }
    if (sscanf( buffer + res, " %d", &modif ) != 1) modif = default_modif;

    p = info->tmp;
    while (prefix_len && *p) { if (*p++ == '\\') prefix_len--; }

    if (!*p)
    {
        if (prefix_len > 1)
        {
            file_read_error( "Malformed key", info );
            return NULL;
        }
        /* empty key name, return base key */
        return (struct key *)grab_object( base );
    }
    name.str = p;
    name.len = len - (p - info->tmp + 1) * sizeof(WCHAR);
    return create_key( base, &name, NULL, flags, modif, &res );
}

/* parse a comma-separated list of hex digits */
static int parse_hex( unsigned char *dest, data_size_t *len, const char *buffer )
{
    const char *p = buffer;
    data_size_t count = 0;
    while (isxdigit(*p))
    {
        int val;
        char buf[3];
        memcpy( buf, p, 2 );
        buf[2] = 0;
        sscanf( buf, "%x", &val );
        if (count++ >= *len) return -1;  /* dest buffer overflow */
        *dest++ = (unsigned char )val;
        p += 2;
        if (*p == ',') p++;
    }
    *len = count;
    return p - buffer;
}

/* parse a value name and create the corresponding value */
static struct key_value *parse_value_name( struct key *key, const char *buffer, data_size_t *len,
                                           struct file_load_info *info )
{
    struct key_value *value;
    struct unicode_str name;
    int index;
    data_size_t maxlen = strlen(buffer) * sizeof(WCHAR);

    if (!get_file_tmp_space( info, maxlen )) return NULL;
    name.str = info->tmp;
    if (buffer[0] == '@')
    {
        name.len = 0;
        *len = 1;
    }
    else
    {
        int r = parse_strW( info->tmp, &maxlen, buffer + 1, '\"' );
        if (r == -1) goto error;
        *len = r + 1; /* for initial quote */
        name.len = maxlen - sizeof(WCHAR);
    }
    while (isspace(buffer[*len])) (*len)++;
    if (buffer[*len] != '=') goto error;
    (*len)++;
    while (isspace(buffer[*len])) (*len)++;
    if (!(value = find_value( key, &name, &index ))) value = insert_value( key, &name, index );
    return value;

 error:
    file_read_error( "Malformed value name", info );
    return NULL;
}

/* load a value from the input file */
static int load_value( struct key *key, const char *buffer, struct file_load_info *info )
{
    DWORD dw;
    void *ptr, *newptr;
    int res, type, parse_type;
    data_size_t maxlen, len;
    struct key_value *value;

    if (!(value = parse_value_name( key, buffer, &len, info ))) return 0;
    if (!(res = get_data_type( buffer + len, &type, &parse_type ))) goto error;
    buffer += len + res;

    switch(parse_type)
    {
    case REG_SZ:
        len = strlen(buffer) * sizeof(WCHAR);
        if (!get_file_tmp_space( info, len )) return 0;
        if ((res = parse_strW( info->tmp, &len, buffer, '\"' )) == -1) goto error;
        ptr = info->tmp;
        break;
    case REG_DWORD:
        dw = strtoul( buffer, NULL, 16 );
        ptr = &dw;
        len = sizeof(dw);
        break;
    case REG_BINARY:  /* hex digits */
        len = 0;
        for (;;)
        {
            maxlen = 1 + strlen(buffer)/3;  /* 3 chars for one hex byte */
            if (!get_file_tmp_space( info, len + maxlen )) return 0;
            if ((res = parse_hex( (unsigned char *)info->tmp + len, &maxlen, buffer )) == -1) goto error;
            len += maxlen;
            buffer += res;
            while (isspace(*buffer)) buffer++;
            if (!*buffer) break;
            if (*buffer != '\\') goto error;
            if (read_next_line( info) != 1) goto error;
            buffer = info->buffer;
            while (isspace(*buffer)) buffer++;
        }
        ptr = info->tmp;
        break;
    default:
        assert(0);
        ptr = NULL;  /* keep compiler quiet */
        break;
    }

    if (!len) newptr = NULL;
    else if (!(newptr = memdup( ptr, len ))) return 0;

    free( value->data );
    value->data = newptr;
    value->len  = len;
    value->type = type;
    make_dirty( key );
    return 1;

 error:
    file_read_error( "Malformed value", info );
    return 0;
}

/* return the length (in path elements) of name that is part of the key name */
/* for instance if key is USER\foo\bar and name is foo\bar\baz, return 2 */
static int get_prefix_len( struct key *key, const char *name, struct file_load_info *info )
{
    WCHAR *p;
    int res;
    data_size_t len = strlen(name) * sizeof(WCHAR);

    if (!get_file_tmp_space( info, len )) return 0;

    if ((res = parse_strW( info->tmp, &len, name, ']' )) == -1)
    {
        file_read_error( "Malformed key", info );
        return 0;
    }
    for (p = info->tmp; *p; p++) if (*p == '\\') break;
    len = (p - info->tmp) * sizeof(WCHAR);
    for (res = 1; key != root_key; res++)
    {
        if (len == key->namelen && !memicmpW( info->tmp, key->name, len / sizeof(WCHAR) )) break;
        key = key->parent;
    }
    if (key == root_key) res = 0;  /* no matching name */
    return res;
}

/* load all the keys from the input file */
/* prefix_len is the number of key name prefixes to skip, or -1 for autodetection */
static void load_keys( struct key *key, const char *filename, FILE *f, int prefix_len )
{
    struct key *subkey = NULL;
    struct file_load_info info;
    char *p;
    int default_modif = time(NULL);
    int flags = (key->flags & KEY_VOLATILE) ? KEY_VOLATILE : KEY_DIRTY;

    info.filename = filename;
    info.file   = f;
    info.len    = 4;
    info.tmplen = 4;
    info.line   = 0;
    if (!(info.buffer = mem_alloc( info.len ))) return;
    if (!(info.tmp = mem_alloc( info.tmplen )))
    {
        free( info.buffer );
        return;
    }

    if ((read_next_line( &info ) != 1) ||
        strcmp( info.buffer, "WINE REGISTRY Version 2" ))
    {
        set_error( STATUS_NOT_REGISTRY_FILE );
        goto done;
    }

    while (read_next_line( &info ) == 1)
    {
        p = info.buffer;
        while (*p && isspace(*p)) p++;
        switch(*p)
        {
        case '[':   /* new key */
            if (subkey) release_object( subkey );
            if (prefix_len == -1) prefix_len = get_prefix_len( key, p + 1, &info );
            if (!(subkey = load_key( key, p + 1, flags, prefix_len, &info, default_modif )))
                file_read_error( "Error creating key", &info );
            break;
        case '@':   /* default value */
        case '\"':  /* value */
            if (subkey) load_value( subkey, p, &info );
            else file_read_error( "Value without key", &info );
            break;
        case '#':   /* comment */
        case ';':   /* comment */
        case 0:     /* empty line */
            break;
        default:
            file_read_error( "Unrecognized input", &info );
            break;
        }
    }

 done:
    if (subkey) release_object( subkey );
    free( info.buffer );
    free( info.tmp );
}

/* load a part of the registry from a file */
static void load_registry( struct key *key, obj_handle_t handle )
{
    struct file *file;
    int fd;

    if (!(file = get_file_obj( current->process, handle, FILE_READ_DATA ))) return;
    fd = dup( get_file_unix_fd( file ) );
    release_object( file );
    if (fd != -1)
    {
        FILE *f = fdopen( fd, "r" );
        if (f)
        {
            load_keys( key, NULL, f, -1 );
            fclose( f );
        }
        else file_set_error();
    }
}

/* load one of the initial registry files */
static void load_init_registry_from_file( const char *filename, struct key *key )
{
    FILE *f;

    if ((f = fopen( filename, "r" )))
    {
        load_keys( key, filename, f, 0 );
        fclose( f );
        if (get_error() == STATUS_NOT_REGISTRY_FILE)
        {
            fprintf( stderr, "%s is not a valid registry file\n", filename );
            return;
        }
    }

    assert( save_branch_count < MAX_SAVE_BRANCH_INFO );

    if ((save_branch_info[save_branch_count].path = strdup( filename )))
    {
        save_branch_info[save_branch_count++].key = (struct key *)grab_object( key );
        make_object_static( &key->obj );
    }
}

static WCHAR *format_user_registry_path( const SID *sid, struct unicode_str *path )
{
    static const WCHAR prefixW[] = {'U','s','e','r','\\','S',0};
    static const WCHAR formatW[] = {'-','%','u',0};
    WCHAR buffer[7 + 10 + 10 + 10 * SID_MAX_SUB_AUTHORITIES];
    WCHAR *p = buffer;
    unsigned int i;

    strcpyW( p, prefixW );
    p += strlenW( prefixW );
    p += sprintfW( p, formatW, sid->Revision );
    p += sprintfW( p, formatW, MAKELONG( MAKEWORD( sid->IdentifierAuthority.Value[5],
                                                   sid->IdentifierAuthority.Value[4] ),
                                         MAKEWORD( sid->IdentifierAuthority.Value[3],
                                                   sid->IdentifierAuthority.Value[2] )));
    for (i = 0; i < sid->SubAuthorityCount; i++)
        p += sprintfW( p, formatW, sid->SubAuthority[i] );

    path->len = (p - buffer) * sizeof(WCHAR);
    path->str = p = memdup( buffer, path->len );
    return p;
}

/* registry initialisation */
void init_registry(void)
{
    static const WCHAR HKLM[] = { 'M','a','c','h','i','n','e' };
    static const WCHAR HKU_default[] = { 'U','s','e','r','\\','.','D','e','f','a','u','l','t' };
    static const struct unicode_str root_name = { NULL, 0 };
    static const struct unicode_str HKLM_name = { HKLM, sizeof(HKLM) };
    static const struct unicode_str HKU_name = { HKU_default, sizeof(HKU_default) };

    WCHAR *current_user_path;
    struct unicode_str current_user_str;

    const char *config = wine_get_config_dir();
    char *p, *filename;
    struct key *key;
    int dummy;

    /* create the root key */
    root_key = alloc_key( &root_name, time(NULL) );
    assert( root_key );
    make_object_static( &root_key->obj );

    if (!(filename = malloc( strlen(config) + 16 ))) fatal_error( "out of memory\n" );
    strcpy( filename, config );
    p = filename + strlen(filename);

    /* load system.reg into Registry\Machine */

    if (!(key = create_key( root_key, &HKLM_name, NULL, 0, time(NULL), &dummy )))
        fatal_error( "could not create Machine registry key\n" );

    strcpy( p, "/system.reg" );
    load_init_registry_from_file( filename, key );
    release_object( key );

    /* load userdef.reg into Registry\User\.Default */

    if (!(key = create_key( root_key, &HKU_name, NULL, 0, time(NULL), &dummy )))
        fatal_error( "could not create User\\.Default registry key\n" );

    strcpy( p, "/userdef.reg" );
    load_init_registry_from_file( filename, key );
    release_object( key );

    /* load user.reg into HKEY_CURRENT_USER */

    /* FIXME: match default user in token.c. should get from process token instead */
    current_user_path = format_user_registry_path( security_interactive_sid, &current_user_str );
    if (!current_user_path ||
        !(key = create_key( root_key, &current_user_str, NULL, 0, time(NULL), &dummy )))
        fatal_error( "could not create HKEY_CURRENT_USER registry key\n" );
    free( current_user_path );
    strcpy( p, "/user.reg" );
    load_init_registry_from_file( filename, key );
    release_object( key );

    free( filename );

    /* start the periodic save timer */
    set_periodic_save_timer();
}

/* save a registry branch to a file */
static void save_all_subkeys( struct key *key, FILE *f )
{
    fprintf( f, "WINE REGISTRY Version 2\n" );
    fprintf( f, ";; All keys relative to " );
    dump_path( key, NULL, f );
    fprintf( f, "\n" );
    save_subkeys( key, key, f );
}

/* save a registry branch to a file handle */
static void save_registry( struct key *key, obj_handle_t handle )
{
    struct file *file;
    int fd;

    if (key->flags & KEY_DELETED)
    {
        set_error( STATUS_KEY_DELETED );
        return;
    }
    if (!(file = get_file_obj( current->process, handle, FILE_WRITE_DATA ))) return;
    fd = dup( get_file_unix_fd( file ) );
    release_object( file );
    if (fd != -1)
    {
        FILE *f = fdopen( fd, "w" );
        if (f)
        {
            save_all_subkeys( key, f );
            if (fclose( f )) file_set_error();
        }
        else
        {
            file_set_error();
            close( fd );
        }
    }
}

/* save a registry branch to a file */
static int save_branch( struct key *key, const char *path )
{
    struct stat st;
    char *p, *real, *tmp = NULL;
    int fd, count = 0, ret = 0, by_symlink;
    FILE *f;

    if (!(key->flags & KEY_DIRTY))
    {
        if (debug_level > 1) dump_operation( key, NULL, "Not saving clean" );
        return 1;
    }

    /* get the real path */

    by_symlink = (!lstat(path, &st) && S_ISLNK (st.st_mode));
    if (!(real = malloc( PATH_MAX ))) return 0;
    if (!realpath( path, real ))
    {
        free( real );
        real = NULL;
    }
    else path = real;

    /* test the file type */

    if ((fd = open( path, O_WRONLY )) != -1)
    {
        /* if file is not a regular file or has multiple links or is accessed
         * via symbolic links, write directly into it; otherwise use a temp file */
        if (by_symlink ||
            (!fstat( fd, &st ) && (!S_ISREG(st.st_mode) || st.st_nlink > 1)))
        {
            ftruncate( fd, 0 );
            goto save;
        }
        close( fd );
    }

    /* create a temp file in the same directory */

    if (!(tmp = malloc( strlen(path) + 20 ))) goto done;
    strcpy( tmp, path );
    if ((p = strrchr( tmp, '/' ))) p++;
    else p = tmp;
    for (;;)
    {
        sprintf( p, "reg%lx%04x.tmp", (long) getpid(), count++ );
        if ((fd = open( tmp, O_CREAT | O_EXCL | O_WRONLY, 0666 )) != -1) break;
        if (errno != EEXIST) goto done;
        close( fd );
    }

    /* now save to it */

 save:
    if (!(f = fdopen( fd, "w" )))
    {
        if (tmp) unlink( tmp );
        close( fd );
        goto done;
    }

    if (debug_level > 1)
    {
        fprintf( stderr, "%s: ", path );
        dump_operation( key, NULL, "saving" );
    }

    save_all_subkeys( key, f );
    ret = !fclose(f);

    if (tmp)
    {
        /* if successfully written, rename to final name */
        if (ret) ret = !rename( tmp, path );
        if (!ret) unlink( tmp );
    }

done:
    free( tmp );
    free( real );
    if (ret) make_clean( key );
    return ret;
}

/* periodic saving of the registry */
static void periodic_save( void *arg )
{
    int i;

    save_timeout_user = NULL;
    for (i = 0; i < save_branch_count; i++)
        save_branch( save_branch_info[i].key, save_branch_info[i].path );
    set_periodic_save_timer();
}

/* start the periodic save timer */
static void set_periodic_save_timer(void)
{
    if (save_timeout_user) remove_timeout_user( save_timeout_user );
    save_timeout_user = add_timeout_user( save_period, periodic_save, NULL );
}

/* save the modified registry branches to disk */
void flush_registry(void)
{
    int i;

    for (i = 0; i < save_branch_count; i++)
    {
        if (!save_branch( save_branch_info[i].key, save_branch_info[i].path ))
        {
            fprintf( stderr, "wineserver: could not save registry branch to %s",
                     save_branch_info[i].path );
            perror( " " );
        }
    }
}


/* create a registry key */
DECL_HANDLER(create_key)
{
    struct key *key = NULL, *parent;
    struct unicode_str name, class;
    unsigned int access = req->access;

    reply->hkey = 0;

    if (req->namelen > get_req_data_size())
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    class.str = (const WCHAR *)get_req_data() + req->namelen / sizeof(WCHAR);
    class.len = ((get_req_data_size() - req->namelen) / sizeof(WCHAR)) * sizeof(WCHAR);
    get_req_path( &name, !req->parent );
    if (name.str > class.str)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    name.len = (class.str - name.str) * sizeof(WCHAR);

    /* NOTE: no access rights are required from the parent handle to create a key */
    if ((parent = get_parent_hkey_obj( req->parent )))
    {
        int flags = (req->options & REG_OPTION_VOLATILE) ? KEY_VOLATILE : KEY_DIRTY;

        if ((key = create_key( parent, &name, &class, flags, req->modif, &reply->created )))
        {
            reply->hkey = alloc_handle( current->process, key, access, req->attributes );
            release_object( key );
        }
        release_object( parent );
    }
}

/* open a registry key */
DECL_HANDLER(open_key)
{
    struct key *key, *parent;
    struct unicode_str name;
    unsigned int access = req->access;

    reply->hkey = 0;
    /* NOTE: no access rights are required to open the parent key, only the child key */
    if ((parent = get_parent_hkey_obj( req->parent )))
    {
        get_req_path( &name, !req->parent );
        if ((key = open_key( parent, &name )))
        {
            reply->hkey = alloc_handle( current->process, key, access, req->attributes );
            release_object( key );
        }
        release_object( parent );
    }
}

/* delete a registry key */
DECL_HANDLER(delete_key)
{
    struct key *key;

    if ((key = get_hkey_obj( req->hkey, DELETE )))
    {
        delete_key( key, 0);
        release_object( key );
    }
}

/* flush a registry key */
DECL_HANDLER(flush_key)
{
    struct key *key = get_hkey_obj( req->hkey, 0 );
    if (key)
    {
        /* we don't need to do anything here with the current implementation */
        release_object( key );
    }
}

/* enumerate registry subkeys */
DECL_HANDLER(enum_key)
{
    struct key *key;

    if ((key = get_hkey_obj( req->hkey,
                             req->index == -1 ? KEY_QUERY_VALUE : KEY_ENUMERATE_SUB_KEYS )))
    {
        enum_key( key, req->index, req->info_class, reply );
        release_object( key );
    }
}

/* set a value of a registry key */
DECL_HANDLER(set_key_value)
{
    struct key *key;
    struct unicode_str name;

    if (req->namelen > get_req_data_size())
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }
    name.str = get_req_data();
    name.len = (req->namelen / sizeof(WCHAR)) * sizeof(WCHAR);

    if ((key = get_hkey_obj( req->hkey, KEY_SET_VALUE )))
    {
        data_size_t datalen = get_req_data_size() - req->namelen;
        const char *data = (const char *)get_req_data() + req->namelen;

        set_value( key, &name, req->type, data, datalen );
        release_object( key );
    }
}

/* retrieve the value of a registry key */
DECL_HANDLER(get_key_value)
{
    struct key *key;
    struct unicode_str name;

    reply->total = 0;
    if ((key = get_hkey_obj( req->hkey, KEY_QUERY_VALUE )))
    {
        get_req_unicode_str( &name );
        get_value( key, &name, &reply->type, &reply->total );
        release_object( key );
    }
}

/* enumerate the value of a registry key */
DECL_HANDLER(enum_key_value)
{
    struct key *key;

    if ((key = get_hkey_obj( req->hkey, KEY_QUERY_VALUE )))
    {
        enum_value( key, req->index, req->info_class, reply );
        release_object( key );
    }
}

/* delete a value of a registry key */
DECL_HANDLER(delete_key_value)
{
    struct key *key;
    struct unicode_str name;

    if ((key = get_hkey_obj( req->hkey, KEY_SET_VALUE )))
    {
        get_req_unicode_str( &name );
        delete_value( key, &name );
        release_object( key );
    }
}

/* load a registry branch from a file */
DECL_HANDLER(load_registry)
{
    struct key *key, *parent;
    struct token *token = thread_get_impersonation_token( current );
    struct unicode_str name;

    const LUID_AND_ATTRIBUTES privs[] =
    {
        { SeBackupPrivilege,  0 },
        { SeRestorePrivilege, 0 },
    };

    if (!token || !token_check_privileges( token, TRUE, privs,
                                           sizeof(privs)/sizeof(privs[0]), NULL ))
    {
        set_error( STATUS_PRIVILEGE_NOT_HELD );
        return;
    }

    if ((parent = get_parent_hkey_obj( req->hkey )))
    {
        int dummy;
        get_req_path( &name, !req->hkey );
        if ((key = create_key( parent, &name, NULL, KEY_DIRTY, time(NULL), &dummy )))
        {
            load_registry( key, req->file );
            release_object( key );
        }
        release_object( parent );
    }
}

DECL_HANDLER(unload_registry)
{
    struct key *key;
    struct token *token = thread_get_impersonation_token( current );

    const LUID_AND_ATTRIBUTES privs[] =
    {
        { SeBackupPrivilege,  0 },
        { SeRestorePrivilege, 0 },
    };

    if (!token || !token_check_privileges( token, TRUE, privs,
                                           sizeof(privs)/sizeof(privs[0]), NULL ))
    {
        set_error( STATUS_PRIVILEGE_NOT_HELD );
        return;
    }

    if ((key = get_hkey_obj( req->hkey, 0 )))
    {
        delete_key( key, 1 );     /* FIXME */
        release_object( key );
    }
}

/* save a registry branch to a file */
DECL_HANDLER(save_registry)
{
    struct key *key;

    if (!thread_single_check_privilege( current, &SeBackupPrivilege ))
    {
        set_error( STATUS_PRIVILEGE_NOT_HELD );
        return;
    }

    if ((key = get_hkey_obj( req->hkey, 0 )))
    {
        save_registry( key, req->file );
        release_object( key );
    }
}

/* add a registry key change notification */
DECL_HANDLER(set_registry_notification)
{
    struct key *key;
    struct event *event;
    struct notify *notify;

    key = get_hkey_obj( req->hkey, KEY_NOTIFY );
    if (key)
    {
        event = get_event_obj( current->process, req->event, SYNCHRONIZE );
        if (event)
        {
            notify = find_notify( key, current->process, req->hkey );
            if (notify)
            {
                if (notify->event)
                    release_object( notify->event );
                grab_object( event );
                notify->event = event;
            }
            else
            {
                notify = mem_alloc( sizeof(*notify) );
                if (notify)
                {
                    grab_object( event );
                    notify->event   = event;
                    notify->subtree = req->subtree;
                    notify->filter  = req->filter;
                    notify->hkey    = req->hkey;
                    notify->process = current->process;
                    list_add_head( &key->notify_list, &notify->entry );
                }
            }
            release_object( event );
        }
        release_object( key );
    }
}
