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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* To do:
 * - behavior with deleted keys
 * - values larger than request buffer
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
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "object.h"
#include "handle.h"
#include "request.h"
#include "unicode.h"

#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/library.h"

struct notify
{
    struct event     *event;    /* event to set when changing this key */
    int               subtree;  /* true if subtree notification */
    unsigned int      filter;   /* which events to notify on */
    obj_handle_t      hkey;     /* hkey associated with this notification */
    struct notify    *next;     /* list of notifications */
    struct notify    *prev;     /* list of notifications */
};

/* a registry key */
struct key
{
    struct object     obj;         /* object header */
    WCHAR            *name;        /* key name */
    WCHAR            *class;       /* key class */
    struct key       *parent;      /* parent key */
    int               last_subkey; /* last in use subkey */
    int               nb_subkeys;  /* count of allocated subkeys */
    struct key      **subkeys;     /* subkeys array */
    int               last_value;  /* last in use value */
    int               nb_values;   /* count of allocated values in array */
    struct key_value *values;      /* values array */
    short             flags;       /* flags */
    short             level;       /* saving level */
    time_t            modif;       /* last modification time */
    struct notify    *first_notify; /* list of notifications */
    struct notify    *last_notify; /* list of notifications */
};

/* key flags */
#define KEY_VOLATILE 0x0001  /* key is volatile (not saved to disk) */
#define KEY_DELETED  0x0002  /* key has been deleted */
#define KEY_DIRTY    0x0004  /* key has been modified */
#define KEY_ROOT     0x0008  /* key is a root key */

/* a key value */
struct key_value
{
    WCHAR            *name;    /* value name */
    int               type;    /* value type */
    size_t            len;     /* value data length in bytes */
    void             *data;    /* pointer to value data */
};

#define MIN_SUBKEYS  8   /* min. number of allocated subkeys per key */
#define MIN_VALUES   8   /* min. number of allocated values per key */


/* the special root keys */
#define HKEY_SPECIAL_ROOT_FIRST   ((unsigned int)HKEY_CLASSES_ROOT)
#define HKEY_SPECIAL_ROOT_LAST    ((unsigned int)HKEY_DYN_DATA)
#define NB_SPECIAL_ROOT_KEYS      (HKEY_SPECIAL_ROOT_LAST - HKEY_SPECIAL_ROOT_FIRST + 1)
#define IS_SPECIAL_ROOT_HKEY(h)   (((unsigned int)(h) >= HKEY_SPECIAL_ROOT_FIRST) && \
                                   ((unsigned int)(h) <= HKEY_SPECIAL_ROOT_LAST))

static struct key *special_root_keys[NB_SPECIAL_ROOT_KEYS];

/* the real root key */
static struct key *root_key;

/* the special root key names */
static const char * const special_root_names[NB_SPECIAL_ROOT_KEYS] =
{
    "Machine\\Software\\Classes",                                    /* HKEY_CLASSES_ROOT */
    "User\\",    /* we append the user name dynamically */           /* HKEY_CURRENT_USER */
    "Machine",                                                       /* HKEY_LOCAL_MACHINE */
    "User",                                                          /* HKEY_USERS */
    "PerfData",                                                      /* HKEY_PERFORMANCE_DATA */
    "Machine\\System\\CurrentControlSet\\HardwareProfiles\\Current", /* HKEY_CURRENT_CONFIG */
    "DynData"                                                        /* HKEY_DYN_DATA */
};


/* keys saving level */
/* current_level is the level that is put into all newly created or modified keys */
/* saving_level is the minimum level that a key needs in order to get saved */
static int current_level;
static int saving_level;

static struct timeval next_save_time;           /* absolute time of next periodic save */
static int save_period;                         /* delay between periodic saves (ms) */
static struct timeout_user *save_timeout_user;  /* saving timer */

/* information about where to save a registry branch */
struct save_branch_info
{
    struct key  *key;
    char        *path;
};

#define MAX_SAVE_BRANCH_INFO 8
static int save_branch_count;
static struct save_branch_info save_branch_info[MAX_SAVE_BRANCH_INFO];


/* information about a file being loaded */
struct file_load_info
{
    FILE *file;    /* input file */
    char *buffer;  /* line buffer */
    int   len;     /* buffer length */
    int   line;    /* current input line */
    char *tmp;     /* temp buffer to use while parsing input */
    int   tmplen;  /* length of temp buffer */
};


static void key_dump( struct object *obj, int verbose );
static void key_destroy( struct object *obj );

static const struct object_ops key_ops =
{
    sizeof(struct key),      /* size */
    key_dump,                /* dump */
    no_add_queue,            /* add_queue */
    NULL,                    /* remove_queue */
    NULL,                    /* signaled */
    NULL,                    /* satisfied */
    NULL,                    /* get_poll_events */
    NULL,                    /* poll_event */
    no_get_fd,               /* get_fd */
    no_flush,                /* flush */
    no_get_file_info,        /* get_file_info */
    NULL,                    /* queue_async */
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
    dump_strW( key->name, strlenW(key->name), f, "[]" );
}

/* dump a value to a text file */
static void dump_value( const struct key_value *value, FILE *f )
{
    int i, count;

    if (value->name[0])
    {
        fputc( '\"', f );
        count = 1 + dump_strW( value->name, strlenW(value->name), f, "\"\"" );
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
            fprintf( f, "dword:%08lx", dw );
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
    /* save key if it has the proper level, and has either some values or no subkeys */
    /* keys with no values but subkeys are saved implicitly by saving the subkeys */
    if ((key->level >= saving_level) && ((key->last_value >= 0) || (key->last_subkey == -1)))
    {
        fprintf( f, "\n[" );
        if (key != base) dump_path( key, base, f );
        fprintf( f, "] %ld\n", key->modif );
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
    if( notify->event )
    {
        set_event( notify->event );
        release_object( notify->event );
        notify->event = NULL;
    }

    if ( !del )
        return;
    if( notify->next )
        notify->next->prev = notify->prev;
    else
        key->last_notify = notify->prev;
    if( notify->prev )
        notify->prev->next = notify->next;
    else
        key->first_notify = notify->next;
    free( notify );
}

static struct notify *find_notify( struct key *key, obj_handle_t hkey)
{
    struct notify *n;

    for( n=key->first_notify; n; n = n->next)
        if( n->hkey == hkey )
            break;
    return n;
}

/* close the notification associated with a handle */
void registry_close_handle( struct object *obj, obj_handle_t hkey )
{
    struct key * key = (struct key *) obj;
    struct notify *notify;

    if( obj->ops != &key_ops )
        return;
    notify = find_notify( key, hkey );
    if( !notify )
        return;
    do_notification( key, notify, 1 );
}

static void key_destroy( struct object *obj )
{
    int i;
    struct key *key = (struct key *)obj;
    assert( obj->ops == &key_ops );

    if (key->name) free( key->name );
    if (key->class) free( key->class );
    for (i = 0; i <= key->last_value; i++)
    {
        free( key->values[i].name );
        if (key->values[i].data) free( key->values[i].data );
    }
    for (i = 0; i <= key->last_subkey; i++)
    {
        key->subkeys[i]->parent = NULL;
        release_object( key->subkeys[i] );
    }
    /* unconditionally notify everything waiting on this key */
    while ( key->first_notify )
        do_notification( key, key->first_notify, 1 );
}

/* duplicate a key path */
/* returns a pointer to a static buffer, so only useable once per request */
static WCHAR *copy_path( const WCHAR *path, size_t len, int skip_root )
{
    static WCHAR buffer[MAX_PATH+1];
    static const WCHAR root_name[] = { '\\','R','e','g','i','s','t','r','y','\\',0 };

    if (len > sizeof(buffer)-sizeof(buffer[0]))
    {
        set_error( STATUS_BUFFER_OVERFLOW );
        return NULL;
    }
    memcpy( buffer, path, len );
    buffer[len / sizeof(WCHAR)] = 0;
    if (skip_root && !strncmpiW( buffer, root_name, 10 )) return buffer + 10;
    return buffer;
}

/* copy a path from the request buffer */
static WCHAR *copy_req_path( size_t len, int skip_root )
{
    const WCHAR *name_ptr = get_req_data();
    if (len > get_req_data_size())
    {
        fatal_protocol_error( current, "copy_req_path: invalid length %d/%d\n",
                              len, get_req_data_size() );
        return NULL;
    }
    return copy_path( name_ptr, len, skip_root );
}

/* return the next token in a given path */
/* returns a pointer to a static buffer, so only useable once per request */
static WCHAR *get_path_token( WCHAR *initpath )
{
    static WCHAR *path;
    WCHAR *ret;

    if (initpath)
    {
        /* path cannot start with a backslash */
        if (*initpath == '\\')
        {
            set_error( STATUS_OBJECT_PATH_INVALID );
            return NULL;
        }
        path = initpath;
    }
    else while (*path == '\\') path++;

    ret = path;
    while (*path && *path != '\\') path++;
    if (*path) *path++ = 0;
    return ret;
}

/* duplicate a Unicode string from the request buffer */
static WCHAR *req_strdupW( const void *req, const WCHAR *str, size_t len )
{
    WCHAR *name;
    if ((name = mem_alloc( len + sizeof(WCHAR) )) != NULL)
    {
        memcpy( name, str, len );
        name[len / sizeof(WCHAR)] = 0;
    }
    return name;
}

/* allocate a key object */
static struct key *alloc_key( const WCHAR *name, time_t modif )
{
    struct key *key;
    if ((key = (struct key *)alloc_object( &key_ops, -1 )))
    {
        key->class       = NULL;
        key->flags       = 0;
        key->last_subkey = -1;
        key->nb_subkeys  = 0;
        key->subkeys     = NULL;
        key->nb_values   = 0;
        key->last_value  = -1;
        key->values      = NULL;
        key->level       = current_level;
        key->modif       = modif;
        key->parent      = NULL;
        key->first_notify = NULL;
        key->last_notify  = NULL;
        if (!(key->name = strdupW( name )))
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
void check_notify( struct key *key, unsigned int change, int not_subtree )
{
    struct notify *n = key->first_notify;
    while (n)
    {
        struct notify *next = n->next;
        if ( ( not_subtree || n->subtree ) && ( change & n->filter ) )
            do_notification( key, n, 0 );
        n = next;
    }
}

/* update key modification time */
static void touch_key( struct key *key, unsigned int change )
{
    struct key *k;

    key->modif = time(NULL);
    key->level = max( key->level, current_level );
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
static struct key *alloc_subkey( struct key *parent, const WCHAR *name, int index, time_t modif )
{
    struct key *key;
    int i;

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
static struct key *find_subkey( const struct key *key, const WCHAR *name, int *index )
{
    int i, min, max, res;

    min = 0;
    max = key->last_subkey;
    while (min <= max)
    {
        i = (min + max) / 2;
        if (!(res = strcmpiW( key->subkeys[i]->name, name )))
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
/* warning: the key name must be writeable (use copy_path) */
static struct key *open_key( struct key *key, WCHAR *name )
{
    int index;
    WCHAR *path;

    if (!(path = get_path_token( name ))) return NULL;
    while (*path)
    {
        if (!(key = find_subkey( key, path, &index )))
        {
            set_error( STATUS_OBJECT_NAME_NOT_FOUND );
            break;
        }
        path = get_path_token( NULL );
    }

    if (debug_level > 1) dump_operation( key, NULL, "Open" );
    if (key) grab_object( key );
    return key;
}

/* create a subkey */
/* warning: the key name must be writeable (use copy_path) */
static struct key *create_key( struct key *key, WCHAR *name, WCHAR *class,
                               int flags, time_t modif, int *created )
{
    struct key *base;
    int base_idx, index;
    WCHAR *path;

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

    if (!(path = get_path_token( name ))) return NULL;
    *created = 0;
    while (*path)
    {
        struct key *subkey;
        if (!(subkey = find_subkey( key, path, &index ))) break;
        key = subkey;
        path = get_path_token( NULL );
    }

    /* create the remaining part */

    if (!*path) goto done;
    *created = 1;
    touch_key( key, REG_NOTIFY_CHANGE_NAME ); /* FIXME: is this right? */
    if (flags & KEY_DIRTY) make_dirty( key );
    base = key;
    base_idx = index;
    key = alloc_subkey( key, path, index, modif );
    while (key)
    {
        key->flags |= flags;
        path = get_path_token( NULL );
        if (!*path) goto done;
        /* we know the index is always 0 in a new key */
        key = alloc_subkey( key, path, 0, modif );
    }
    if (base_idx != -1) free_subkey( base, base_idx );
    return NULL;

 done:
    if (debug_level > 1) dump_operation( key, NULL, "Create" );
    if (class) key->class = strdupW(class);
    grab_object( key );
    return key;
}

/* query information about a key or a subkey */
static void enum_key( const struct key *key, int index, int info_class,
                      struct enum_key_reply *reply )
{
    int i;
    size_t len, namelen, classlen;
    int max_subkey = 0, max_class = 0;
    int max_value = 0, max_data = 0;
    WCHAR *data;

    if (index != -1)  /* -1 means use the specified key directly */
    {
        if ((index < 0) || (index > key->last_subkey))
        {
            set_error( STATUS_NO_MORE_ENTRIES );
            return;
        }
        key = key->subkeys[index];
    }

    namelen = strlenW(key->name) * sizeof(WCHAR);
    classlen = key->class ? strlenW(key->class) * sizeof(WCHAR) : 0;

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
            len = strlenW( subkey->name );
            if (len > max_subkey) max_subkey = len;
            if (!subkey->class) continue;
            len = strlenW( subkey->class );
            if (len > max_class) max_class = len;
        }
        for (i = 0; i <= key->last_value; i++)
        {
            len = strlenW( key->values[i].name );
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
            memcpy( (char *)data + namelen, key->class, len - namelen );
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
static void delete_key( struct key *key )
{
    int index;
    struct key *parent;

    /* must find parent and index */
    if (key->flags & KEY_ROOT)
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    if (!(parent = key->parent) || (key->flags & KEY_DELETED))
    {
        set_error( STATUS_KEY_DELETED );
        return;
    }
    for (index = 0; index <= parent->last_subkey; index++)
        if (parent->subkeys[index] == key) break;
    assert( index <= parent->last_subkey );

    /* we can only delete a key that has no subkeys (FIXME) */
    if ((key->flags & KEY_ROOT) || (key->last_subkey >= 0))
    {
        set_error( STATUS_ACCESS_DENIED );
        return;
    }
    if (debug_level > 1) dump_operation( key, NULL, "Delete" );
    free_subkey( parent, index );
    touch_key( parent, REG_NOTIFY_CHANGE_NAME );
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
static struct key_value *find_value( const struct key *key, const WCHAR *name, int *index )
{
    int i, min, max, res;

    min = 0;
    max = key->last_value;
    while (min <= max)
    {
        i = (min + max) / 2;
        if (!(res = strcmpiW( key->values[i].name, name )))
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
static struct key_value *insert_value( struct key *key, const WCHAR *name, int index )
{
    struct key_value *value;
    WCHAR *new_name;
    int i;

    if (key->last_value + 1 == key->nb_values)
    {
        if (!grow_values( key )) return NULL;
    }
    if (!(new_name = strdupW(name))) return NULL;
    for (i = ++key->last_value; i > index; i--) key->values[i] = key->values[i - 1];
    value = &key->values[index];
    value->name = new_name;
    value->len  = 0;
    value->data = NULL;
    return value;
}

/* set a key value */
static void set_value( struct key *key, WCHAR *name, int type, const void *data, size_t len )
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
            if (ptr) free( ptr );
            return;
        }
    }
    else if (value->data) free( value->data ); /* already existing, free previous data */

    value->type  = type;
    value->len   = len;
    value->data  = ptr;
    touch_key( key, REG_NOTIFY_CHANGE_LAST_SET );
    if (debug_level > 1) dump_operation( key, value, "Set" );
}

/* get a key value */
static void get_value( struct key *key, const WCHAR *name, int *type, int *len )
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
        size_t namelen, maxlen;

        value = &key->values[i];
        reply->type = value->type;
        namelen = strlenW( value->name ) * sizeof(WCHAR);

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
static void delete_value( struct key *key, const WCHAR *name )
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
    if (value->data) free( value->data );
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

static struct key *create_root_key( obj_handle_t hkey )
{
    WCHAR keyname[80];
    int i, dummy;
    struct key *key;
    const char *p;

    p = special_root_names[(unsigned int)hkey - HKEY_SPECIAL_ROOT_FIRST];
    i = 0;
    while (*p) keyname[i++] = *p++;

    if (hkey == (obj_handle_t)HKEY_CURRENT_USER)  /* this one is special */
    {
        /* get the current user name */
        p = wine_get_user_name();
        while (*p && i < sizeof(keyname)/sizeof(WCHAR)-1) keyname[i++] = *p++;
    }
    keyname[i++] = 0;

    if ((key = create_key( root_key, keyname, NULL, 0, time(NULL), &dummy )))
    {
        special_root_keys[(unsigned int)hkey - HKEY_SPECIAL_ROOT_FIRST] = key;
        key->flags |= KEY_ROOT;
    }
    return key;
}

/* get the registry key corresponding to an hkey handle */
static struct key *get_hkey_obj( obj_handle_t hkey, unsigned int access )
{
    struct key *key;

    if (!hkey) return (struct key *)grab_object( root_key );
    if (IS_SPECIAL_ROOT_HKEY(hkey))
    {
        if (!(key = special_root_keys[(unsigned int)hkey - HKEY_SPECIAL_ROOT_FIRST]))
            key = create_root_key( hkey );
        else
            grab_object( key );
    }
    else
        key = (struct key *)get_handle_obj( current->process, hkey, access, &key_ops );
    return key;
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
static int get_file_tmp_space( struct file_load_info *info, int size )
{
    char *tmp;
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
    fprintf( stderr, "Line %d: %s '%s'\n", info->line, err, info->buffer );
}

/* parse an escaped string back into Unicode */
/* return the number of chars read from the input, or -1 on output overflow */
static int parse_strW( WCHAR *dest, int *len, const char *src, char endchar )
{
    int count = sizeof(WCHAR);  /* for terminating null */
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
                             int prefix_len, struct file_load_info *info )
{
    WCHAR *p, *name;
    int res, len, modif;

    len = strlen(buffer) * sizeof(WCHAR);
    if (!get_file_tmp_space( info, len )) return NULL;

    if ((res = parse_strW( (WCHAR *)info->tmp, &len, buffer, ']' )) == -1)
    {
        file_read_error( "Malformed key", info );
        return NULL;
    }
    if (sscanf( buffer + res, " %d", &modif ) != 1) modif = time(NULL);

    p = (WCHAR *)info->tmp;
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
    if (!(name = copy_path( p, len - ((char *)p - info->tmp), 0 )))
    {
        file_read_error( "Key is too long", info );
        return NULL;
    }
    return create_key( base, name, NULL, flags, modif, &res );
}

/* parse a comma-separated list of hex digits */
static int parse_hex( unsigned char *dest, int *len, const char *buffer )
{
    const char *p = buffer;
    int count = 0;
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
static struct key_value *parse_value_name( struct key *key, const char *buffer, int *len,
                                           struct file_load_info *info )
{
    struct key_value *value;
    int index, maxlen;

    maxlen = strlen(buffer) * sizeof(WCHAR);
    if (!get_file_tmp_space( info, maxlen )) return NULL;
    if (buffer[0] == '@')
    {
        info->tmp[0] = info->tmp[1] = 0;
        *len = 1;
    }
    else
    {
        if ((*len = parse_strW( (WCHAR *)info->tmp, &maxlen, buffer + 1, '\"' )) == -1) goto error;
        (*len)++;  /* for initial quote */
    }
    while (isspace(buffer[*len])) (*len)++;
    if (buffer[*len] != '=') goto error;
    (*len)++;
    while (isspace(buffer[*len])) (*len)++;
    if (!(value = find_value( key, (WCHAR *)info->tmp, &index )))
        value = insert_value( key, (WCHAR *)info->tmp, index );
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
    int maxlen, len, res;
    int type, parse_type;
    struct key_value *value;

    if (!(value = parse_value_name( key, buffer, &len, info ))) return 0;
    if (!(res = get_data_type( buffer + len, &type, &parse_type ))) goto error;
    buffer += len + res;

    switch(parse_type)
    {
    case REG_SZ:
        len = strlen(buffer) * sizeof(WCHAR);
        if (!get_file_tmp_space( info, len )) return 0;
        if ((res = parse_strW( (WCHAR *)info->tmp, &len, buffer, '\"' )) == -1) goto error;
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
            if ((res = parse_hex( info->tmp + len, &maxlen, buffer )) == -1) goto error;
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

    if (value->data) free( value->data );
    value->data = newptr;
    value->len  = len;
    value->type = type;
    /* update the key level but not the modification time */
    key->level = max( key->level, current_level );
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
    int len = strlen(name) * sizeof(WCHAR);
    if (!get_file_tmp_space( info, len )) return 0;

    if ((res = parse_strW( (WCHAR *)info->tmp, &len, name, ']' )) == -1)
    {
        file_read_error( "Malformed key", info );
        return 0;
    }
    for (p = (WCHAR *)info->tmp; *p; p++) if (*p == '\\') break;
    *p = 0;
    for (res = 1; key != root_key; res++)
    {
        if (!strcmpiW( (WCHAR *)info->tmp, key->name )) break;
        key = key->parent;
    }
    if (key == root_key) res = 0;  /* no matching name */
    return res;
}

/* load all the keys from the input file */
static void load_keys( struct key *key, FILE *f )
{
    struct key *subkey = NULL;
    struct file_load_info info;
    char *p;
    int flags = (key->flags & KEY_VOLATILE) ? KEY_VOLATILE : KEY_DIRTY;
    int prefix_len = -1;  /* number of key name prefixes to skip */

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
            if (!(subkey = load_key( key, p + 1, flags, prefix_len, &info )))
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
    struct object *obj;
    int fd;

    if (!(obj = get_handle_obj( current->process, handle, GENERIC_READ, NULL ))) return;
    fd = dup(obj->ops->get_fd( obj ));
    release_object( obj );
    if (fd != -1)
    {
        FILE *f = fdopen( fd, "r" );
        if (f)
        {
            load_keys( key, f );
            fclose( f );
        }
        else file_set_error();
    }
}

/* registry initialisation */
void init_registry(void)
{
    static const WCHAR root_name[] = { 0 };
    static const WCHAR config_name[] =
    { 'M','a','c','h','i','n','e','\\','S','o','f','t','w','a','r','e','\\',
      'W','i','n','e','\\','W','i','n','e','\\','C','o','n','f','i','g',0 };

    char *filename;
    const char *config;
    FILE *f;

    /* create the root key */
    root_key = alloc_key( root_name, time(NULL) );
    assert( root_key );
    root_key->flags |= KEY_ROOT;

    /* load the config file */
    config = wine_get_config_dir();
    if (!(filename = malloc( strlen(config) + 8 ))) fatal_error( "out of memory\n" );
    strcpy( filename, config );
    strcat( filename, "/config" );
    if ((f = fopen( filename, "r" )))
    {
        struct key *key;
        int dummy;

        /* create the config key */
        if (!(key = create_key( root_key, copy_path( config_name, sizeof(config_name), 0 ),
                                NULL, 0, time(NULL), &dummy )))
            fatal_error( "could not create config key\n" );
        key->flags |= KEY_VOLATILE;

        load_keys( key, f );
        fclose( f );
        if (get_error() == STATUS_NOT_REGISTRY_FILE)
            fatal_error( "%s is not a valid registry file\n", filename );
        if (get_error())
            fatal_error( "loading %s failed with error %x\n", filename, get_error() );

        release_object( key );
    }
    free( filename );
}

/* update the level of the parents of a key (only needed for the old format) */
static int update_level( struct key *key )
{
    int i;
    int max = key->level;
    for (i = 0; i <= key->last_subkey; i++)
    {
        int sub = update_level( key->subkeys[i] );
        if (sub > max) max = sub;
    }
    key->level = max;
    return max;
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
    struct object *obj;
    int fd;

    if (key->flags & KEY_DELETED)
    {
        set_error( STATUS_KEY_DELETED );
        return;
    }
    if (!(obj = get_handle_obj( current->process, handle, GENERIC_WRITE, NULL ))) return;
    fd = dup(obj->ops->get_fd( obj ));
    release_object( obj );
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

/* register a key branch for being saved on exit */
static void register_branch_for_saving( struct key *key, const char *path, size_t len )
{
    if (save_branch_count >= MAX_SAVE_BRANCH_INFO)
    {
        set_error( STATUS_NO_MORE_ENTRIES );
        return;
    }
    if (!len || !(save_branch_info[save_branch_count].path = memdup( path, len ))) return;
    save_branch_info[save_branch_count].path[len - 1] = 0;
    save_branch_info[save_branch_count].key = (struct key *)grab_object( key );
    save_branch_count++;
}

/* save a registry branch to a file */
static int save_branch( struct key *key, const char *path )
{
    char *p, *real, *tmp = NULL;
    int fd, count = 0, ret = 0;
    FILE *f;

    if (!(key->flags & KEY_DIRTY))
    {
        if (debug_level > 1) dump_operation( key, NULL, "Not saving clean" );
        return 1;
    }

    /* get the real path */

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
        struct stat st;
        /* if file is not a regular file or has multiple links,
           write directly into it; otherwise use a temp file */
        if (!fstat( fd, &st ) && (!S_ISREG(st.st_mode) || st.st_nlink > 1))
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
        free( tmp );
    }

done:
    if (real) free( real );
    if (ret) make_clean( key );
    return ret;
}

/* periodic saving of the registry */
static void periodic_save( void *arg )
{
    int i;
    for (i = 0; i < save_branch_count; i++)
        save_branch( save_branch_info[i].key, save_branch_info[i].path );
    add_timeout( &next_save_time, save_period );
    save_timeout_user = add_timeout_user( &next_save_time, periodic_save, 0 );
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

/* close the top-level keys; used on server exit */
void close_registry(void)
{
    int i;

    for (i = 0; i < save_branch_count; i++) release_object( save_branch_info[i].key );
    release_object( root_key );
}


/* create a registry key */
DECL_HANDLER(create_key)
{
    struct key *key = NULL, *parent;
    unsigned int access = req->access;
    WCHAR *name, *class;

    if (access & MAXIMUM_ALLOWED) access = KEY_ALL_ACCESS;  /* FIXME: needs general solution */
    reply->hkey = 0;
    if (!(name = copy_req_path( req->namelen, !req->parent ))) return;
    if ((parent = get_hkey_obj( req->parent, 0 /*FIXME*/ )))
    {
        int flags = (req->options & REG_OPTION_VOLATILE) ? KEY_VOLATILE : KEY_DIRTY;

        if (req->namelen == get_req_data_size())  /* no class specified */
        {
            key = create_key( parent, name, NULL, flags, req->modif, &reply->created );
        }
        else
        {
            const WCHAR *class_ptr = (WCHAR *)((char *)get_req_data() + req->namelen);

            if ((class = req_strdupW( req, class_ptr, get_req_data_size() - req->namelen )))
            {
                key = create_key( parent, name, class, flags, req->modif, &reply->created );
                free( class );
            }
        }
        if (key)
        {
            reply->hkey = alloc_handle( current->process, key, access, 0 );
            release_object( key );
        }
        release_object( parent );
    }
}

/* open a registry key */
DECL_HANDLER(open_key)
{
    struct key *key, *parent;
    unsigned int access = req->access;

    if (access & MAXIMUM_ALLOWED) access = KEY_ALL_ACCESS;  /* FIXME: needs general solution */
    reply->hkey = 0;
    if ((parent = get_hkey_obj( req->parent, 0 /*FIXME*/ )))
    {
        WCHAR *name = copy_path( get_req_data(), get_req_data_size(), !req->parent );
        if (name && (key = open_key( parent, name )))
        {
            reply->hkey = alloc_handle( current->process, key, access, 0 );
            release_object( key );
        }
        release_object( parent );
    }
}

/* delete a registry key */
DECL_HANDLER(delete_key)
{
    struct key *key;

    if ((key = get_hkey_obj( req->hkey, 0 /*FIXME*/ )))
    {
        delete_key( key );
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
    WCHAR *name;

    if (!(name = copy_req_path( req->namelen, 0 ))) return;
    if ((key = get_hkey_obj( req->hkey, KEY_SET_VALUE )))
    {
        size_t datalen = get_req_data_size() - req->namelen;
        const char *data = (char *)get_req_data() + req->namelen;

        set_value( key, name, req->type, data, datalen );
        release_object( key );
    }
}

/* retrieve the value of a registry key */
DECL_HANDLER(get_key_value)
{
    struct key *key;
    WCHAR *name;

    reply->total = 0;
    if (!(name = copy_path( get_req_data(), get_req_data_size(), 0 ))) return;
    if ((key = get_hkey_obj( req->hkey, KEY_QUERY_VALUE )))
    {
        get_value( key, name, &reply->type, &reply->total );
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
    WCHAR *name;
    struct key *key;

    if ((key = get_hkey_obj( req->hkey, KEY_SET_VALUE )))
    {
        if ((name = req_strdupW( req, get_req_data(), get_req_data_size() )))
        {
            delete_value( key, name );
            free( name );
        }
        release_object( key );
    }
}

/* load a registry branch from a file */
DECL_HANDLER(load_registry)
{
    struct key *key;

    if ((key = get_hkey_obj( req->hkey, KEY_SET_VALUE | KEY_CREATE_SUB_KEY )))
    {
        /* FIXME: use subkey name */
        load_registry( key, req->file );
        release_object( key );
    }
}

/* save a registry branch to a file */
DECL_HANDLER(save_registry)
{
    struct key *key;

    if ((key = get_hkey_obj( req->hkey, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS )))
    {
        save_registry( key, req->file );
        release_object( key );
    }
}

/* set the current and saving level for the registry */
DECL_HANDLER(set_registry_levels)
{
    current_level  = req->current;
    saving_level   = req->saving;

    /* set periodic save timer */

    if (save_timeout_user)
    {
        remove_timeout_user( save_timeout_user );
        save_timeout_user = NULL;
    }
    if ((save_period = req->period))
    {
        if (save_period < 10000) save_period = 10000;  /* limit rate */
        gettimeofday( &next_save_time, 0 );
        add_timeout( &next_save_time, save_period );
        save_timeout_user = add_timeout_user( &next_save_time, periodic_save, 0 );
    }
}

/* save a registry branch at server exit */
DECL_HANDLER(save_registry_atexit)
{
    struct key *key;

    if ((key = get_hkey_obj( req->hkey, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS )))
    {
        register_branch_for_saving( key, get_req_data(), get_req_data_size() );
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
    if( key )
    {
        event = get_event_obj( current->process, req->event, SYNCHRONIZE );
        if( event )
        {
            notify = find_notify( key, req->hkey );
            if( notify )
            {
                release_object( notify->event );
                grab_object( event );
                notify->event = event;
            }
            else
            {
                notify = (struct notify *) malloc (sizeof *notify);
                if( notify )
                {
                    grab_object( event );
                    notify->event   = event;
                    notify->subtree = req->subtree;
                    notify->filter  = req->filter;
                    notify->hkey    = req->hkey;
    
                    /* add to linked list */
                    notify->prev = NULL;
                    notify->next = key->first_notify;
                    if ( notify->next )
                        notify->next->prev = notify;
                    else
                        key->last_notify = notify;
                    key->first_notify = notify;
                }
                else
                    set_error( STATUS_NO_MEMORY );
            }
            release_object( event );
        }
        release_object( key );
    }
}
