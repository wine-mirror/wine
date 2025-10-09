/*
 * Dlls load order support
 *
 * Copyright 1999 Bertho Stultiens
 * Copyright 2003 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "unix_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);

#define LOADORDER_ALLOC_CLUSTER	32	/* Allocate with 32 entries at a time */

struct module_loadorder
{
    const WCHAR        *modulename;
    enum loadorder      loadorder;
};

static struct
{
    int                 count;
    int                 alloc;
    struct module_loadorder *order;
} env_list;

static const WCHAR separatorsW[] = {',',' ','\t',0};

static HANDLE std_key;
static HANDLE app_key;
static BOOL init_done;
static BOOL main_exe_loaded;


/***************************************************************************
 *	cmp_sort_func	(internal, static)
 *
 * Sorting and comparing function used in sort and search of loadorder
 * entries.
 */
static int cmp_sort_func(const void *s1, const void *s2)
{
    return wcsicmp( ((const struct module_loadorder *)s1)->modulename,
                    ((const struct module_loadorder *)s2)->modulename );
}


/***************************************************************************
 *	get_basename
 *
 * Return the base name of a file name (i.e. remove the path components).
 */
static WCHAR *get_basename( WCHAR *name )
{
    WCHAR *ptr;

    if (name[0] && name[1] == ':') name += 2;  /* strip drive specification */
    if ((ptr = wcsrchr( name, '\\' ))) name = ptr + 1;
    if ((ptr = wcsrchr( name, '/' ))) name = ptr + 1;
    return name;
}

/***************************************************************************
 *	remove_dll_ext
 *
 * Remove extension if it is ".dll".
 */
static inline void remove_dll_ext( WCHAR *name )
{
    static const WCHAR dllW[] = {'.','d','l','l',0};
    WCHAR *p = wcsrchr( name, '.' );

    if (p && !wcsicmp( p, dllW )) *p = 0;
}


/***************************************************************************
 *	debugstr_loadorder
 *
 * Return a loadorder in printable form.
 */
static const char *debugstr_loadorder( enum loadorder lo )
{
    switch(lo)
    {
    case LO_DISABLED: return "";
    case LO_NATIVE: return "n";
    case LO_BUILTIN: return "b";
    case LO_NATIVE_BUILTIN: return "n,b";
    case LO_BUILTIN_NATIVE: return "b,n";
    case LO_DEFAULT: return "default";
    default: return "??";
    }
}


/***************************************************************************
 *	parse_load_order
 *
 * Parses the loadorder options from the configuration and puts it into
 * a structure.
 */
static enum loadorder parse_load_order( const WCHAR *order )
{
    enum loadorder ret = LO_DISABLED;

    while (*order)
    {
        order += wcsspn( order, separatorsW );
        switch(*order)
        {
        case 'N':  /* native */
        case 'n':
            if (ret == LO_DISABLED) ret = LO_NATIVE;
            else if (ret == LO_BUILTIN) return LO_BUILTIN_NATIVE;
            break;
        case 'B':  /* builtin */
        case 'b':
            if (ret == LO_DISABLED) ret = LO_BUILTIN;
            else if (ret == LO_NATIVE) return LO_NATIVE_BUILTIN;
            break;
        }
        order += wcscspn( order, separatorsW );
    }
    return ret;
}


/***************************************************************************
 *	add_load_order
 *
 * Adds an entry in the list of environment overrides.
 */
static void add_load_order( const struct module_loadorder *lo )
{
    int i;

    for(i = 0; i < env_list.count; i++)
    {
        if (!cmp_sort_func( lo, &env_list.order[i] ))
        {
            /* replace existing option */
            env_list.order[i].loadorder = lo->loadorder;
            return;
        }
    }

    if (i >= env_list.alloc)
    {
        /* No space in current array, make it larger */
        env_list.alloc += LOADORDER_ALLOC_CLUSTER;
        env_list.order = realloc( env_list.order, env_list.alloc * sizeof(*lo) );
    }
    env_list.order[i].loadorder  = lo->loadorder;
    env_list.order[i].modulename = lo->modulename;
    env_list.count++;
}


/***************************************************************************
 *	add_load_order_set
 *
 * Adds a set of entries in the list of command-line overrides from the key parameter.
 */
static void add_load_order_set( WCHAR *entry )
{
    struct module_loadorder ldo;
    WCHAR *end = wcschr( entry, '=' );

    if (!end) return;
    *end++ = 0;
    ldo.loadorder = parse_load_order( end );

    while (*entry)
    {
        entry += wcsspn( entry, separatorsW );
        end = entry + wcscspn( entry, separatorsW );
        if (*end) *end++ = 0;
        if (*entry)
        {
            remove_dll_ext( entry );
            ldo.modulename = entry;
            add_load_order( &ldo );
            entry = end;
        }
    }
}


/***************************************************************************
 *	init_load_order
 */
static void init_load_order(void)
{
    WCHAR *entry, *next, *order;
    const char *overrides = getenv( "WINEDLLOVERRIDES" );

    /* @@ Wine registry key: HKCU\Software\Wine\DllOverrides */
    open_hkcu_key( "Software\\Wine\\DllOverrides", &std_key );

    init_done = TRUE;

    if (!overrides) return;
    order = entry = malloc( (strlen(overrides) + 1) * sizeof(WCHAR) );
    ntdll_umbstowcs( overrides, strlen(overrides) + 1, order, strlen(overrides) + 1 );
    while (*entry)
    {
        while (*entry == ';') entry++;
        if (!*entry) break;
        next = wcschr( entry, ';' );
        if (next) *next++ = 0;
        else next = entry + wcslen(entry);
        add_load_order_set( entry );
        entry = next;
    }

    /* sort the array for quick lookup */
    if (env_list.count)
        qsort(env_list.order, env_list.count, sizeof(env_list.order[0]), cmp_sort_func);

    /* note: we don't free the string because the stored module names point inside it */
}


/***************************************************************************
 *	get_env_load_order
 *
 * Get the load order for a given module from the WINEDLLOVERRIDES environment variable.
 */
static inline enum loadorder get_env_load_order( const WCHAR *module )
{
    struct module_loadorder tmp, *res;

    tmp.modulename = module;
    /* some bsearch implementations (Solaris) are buggy when the number of items is 0 */
    if (env_list.count &&
        (res = bsearch(&tmp, env_list.order, env_list.count, sizeof(env_list.order[0]), cmp_sort_func)))
        return res->loadorder;
    return LO_INVALID;
}


/***************************************************************************
 *	open_app_key
 *
 * Get the registry key for the app-specific DllOverrides list.
 */
static HANDLE open_app_key( const WCHAR *app_name )
{
    static const WCHAR dlloverridesW[] = {'\\','D','l','l','O','v','e','r','r','i','d','e','s',0};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE root, app_key = 0;

    if (!open_hkcu_key( "Software\\Wine\\AppDefaults", &root ))
    {
        ULONG len = wcslen( app_name ) + ARRAY_SIZE(dlloverridesW);
        nameW.Length = (len - 1) * sizeof(WCHAR);
        nameW.Buffer = malloc( len * sizeof(WCHAR) );
        wcscpy( nameW.Buffer, app_name );
        wcscat( nameW.Buffer, dlloverridesW );
        InitializeObjectAttributes( &attr, &nameW, 0, root, NULL );

        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\DllOverrides */
        NtOpenKey( &app_key, KEY_ALL_ACCESS, &attr );
        NtClose( root );
        free( nameW.Buffer );
    }
    return app_key;
}


/***************************************************************************
 *	get_registry_value
 *
 * Load the registry loadorder value for a given module.
 */
static enum loadorder get_registry_value( HANDLE hkey, WCHAR *module )
{
    UNICODE_STRING valueW;
    char buffer[80];
    DWORD count;

    valueW.Length = wcslen( module ) * sizeof(WCHAR);
    valueW.Buffer = module;

    if (!NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation,
                                 buffer, sizeof(buffer), &count ))
    {
        WCHAR *str = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)buffer)->Data;
        return parse_load_order( str );
    }
    return LO_INVALID;
}


/***************************************************************************
 *	get_load_order_value
 *
 * Get the load order for the exact specified module string, looking in:
 * 1. The WINEDLLOVERRIDES environment variable
 * 2. The per-application DllOverrides key
 * 3. The standard DllOverrides key
 */
static enum loadorder get_load_order_value( HANDLE std_key, HANDLE app_key, WCHAR *module )
{
    enum loadorder ret;

    if ((ret = get_env_load_order( module )) != LO_INVALID)
    {
        TRACE( "got environment %s for %s\n", debugstr_loadorder(ret), debugstr_w(module) );
        return ret;
    }

    if (app_key && ((ret = get_registry_value( app_key, module )) != LO_INVALID))
    {
        TRACE( "got app defaults %s for %s\n", debugstr_loadorder(ret), debugstr_w(module) );
        return ret;
    }

    if (std_key && ((ret = get_registry_value( std_key, module )) != LO_INVALID))
    {
        TRACE( "got standard key %s for %s\n", debugstr_loadorder(ret), debugstr_w(module) );
        return ret;
    }

    return ret;
}


/***************************************************************************
 *	set_load_order_app_name
 */
void set_load_order_app_name( const WCHAR *app_name )
{
    const WCHAR *p;

    if ((p = wcsrchr( app_name, '\\' ))) app_name = p + 1;
    app_key = open_app_key( app_name );
    main_exe_loaded = TRUE;
}


/***************************************************************************
 *	get_load_order   (internal)
 *
 * Return the loadorder of a module.
 * The system directory and '.dll' extension is stripped from the path.
 */
enum loadorder get_load_order( const UNICODE_STRING *nt_name )
{
    static const WCHAR prefixW[] = {'\\','?','?','\\'};
    enum loadorder ret = LO_INVALID;
    const WCHAR *path = nt_name->Buffer;
    unsigned int i, len = nt_name->Length / sizeof(WCHAR);
    WCHAR *module, *basename;

    if (!init_done) init_load_order();

    if (len > 4 && !wcsncmp( path, prefixW, 4 ))
    {
        path += 4;
        len -= 4;
    }

    /* Strip path information if the module resides in the system directory
     */
    if (len > wcslen(system_dir) - 4 && !wcsnicmp( system_dir + 4, path, wcslen(system_dir) - 4 ))
    {
        unsigned int pos = wcslen( system_dir ) - 4;
        while (pos < len && (path[pos] == '\\' || path[pos] == '/')) pos++;
        for (i = pos; i < len; i++) if (path[i] == '\\' || path[i] == '/') break;
        if (i == len)
        {
            path += pos;
            len -= pos;
        }
    }

    if (!(module = malloc( (len + 2) * sizeof(WCHAR) ))) return ret;
    memcpy( module + 1, path, len * sizeof(WCHAR) );  /* reserve module[0] for the wildcard char */
    module[len + 1] = 0;
    remove_dll_ext( module + 1 );
    basename = get_basename( module + 1 );

    /* first explicit module name */
    if ((ret = get_load_order_value( std_key, app_key, module+1 )) != LO_INVALID)
        goto done;

    /* then module basename preceded by '*' */
    basename[-1] = '*';
    if ((ret = get_load_order_value( std_key, app_key, basename-1 )) != LO_INVALID)
        goto done;

    /* then module basename without '*' (only if explicit path) */
    if (basename != module+1 && ((ret = get_load_order_value( std_key, app_key, basename )) != LO_INVALID))
        goto done;

    /* if loading the main exe with an explicit path, try native first */
    if (!main_exe_loaded && basename != module+1)
    {
        ret = LO_NATIVE_BUILTIN;
        TRACE( "got main exe default %s for %s\n", debugstr_loadorder(ret), debugstr_us(nt_name) );
        goto done;
    }

    /* and last the hard-coded default */
    ret = LO_DEFAULT;
    TRACE( "got hardcoded %s for %s\n", debugstr_loadorder(ret), debugstr_us(nt_name) );

 done:
    free( module );
    return ret;
}
