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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "module.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);

#define LOADORDER_ALLOC_CLUSTER	32	/* Allocate with 32 entries at a time */

typedef struct module_loadorder
{
    const WCHAR        *modulename;
    enum loadorder_type loadorder[LOADORDER_NTYPES];
} module_loadorder_t;

struct loadorder_list
{
    int                 count;
    int                 alloc;
    module_loadorder_t *order;
};

/* dll to load as builtins if not explicitly specified otherwise */
/* the list must remain sorted by dll name */
static const WCHAR default_builtins[][10] =
{
    { 'g','d','i','3','2',0 },
    { 'i','c','m','p',0 },
    { 'k','e','r','n','e','l','3','2',0 },
    { 'n','t','d','l','l',0 },
    { 'o','d','b','c','3','2',0 },
    { 't','t','y','d','r','v',0 },
    { 'u','s','e','r','3','2',0 },
    { 'w','3','2','s','k','r','n','l',0 },
    { 'w','i','n','e','d','o','s',0 },
    { 'w','i','n','e','p','s',0 },
    { 'w','i','n','m','m',0 },
    { 'w','n','a','s','p','i','3','2',0 },
    { 'w','o','w','3','2',0 },
    { 'w','s','2','_','3','2',0 },
    { 'w','s','o','c','k','3','2',0 },
    { 'x','1','1','d','r','v',0 }
};

/* default if nothing else specified */
static const enum loadorder_type default_loadorder[LOADORDER_NTYPES] =
{
    LOADORDER_BI, LOADORDER_DLL, 0
};

static const WCHAR separatorsW[] = {',',' ','\t',0};

static int init_done;
static struct loadorder_list env_list;


/***************************************************************************
 *	cmp_sort_func	(internal, static)
 *
 * Sorting and comparing function used in sort and search of loadorder
 * entries.
 */
static int cmp_sort_func(const void *s1, const void *s2)
{
    return strcmpiW(((const module_loadorder_t *)s1)->modulename, ((const module_loadorder_t *)s2)->modulename);
}


/***************************************************************************
 *	strcmp_func
 */
static int strcmp_func(const void *s1, const void *s2)
{
    return strcmpiW( (const WCHAR *)s1, (const WCHAR *)s2 );
}


/***************************************************************************
 *	get_basename
 *
 * Return the base name of a file name (i.e. remove the path components).
 */
static const WCHAR *get_basename( const WCHAR *name )
{
    const WCHAR *ptr;

    if (name[0] && name[1] == ':') name += 2;  /* strip drive specification */
    if ((ptr = strrchrW( name, '\\' ))) name = ptr + 1;
    if ((ptr = strrchrW( name, '/' ))) name = ptr + 1;
    return name;
}

/***************************************************************************
 *	remove_dll_ext
 *
 * Remove extension if it is ".dll".
 */
static inline void remove_dll_ext( WCHAR *ext )
{
    if (ext[0] == '.' &&
        toupperW(ext[1]) == 'D' &&
        toupperW(ext[2]) == 'L' &&
        toupperW(ext[3]) == 'L' &&
        !ext[4]) ext[0] = 0;
}


/***************************************************************************
 *	debugstr_loadorder
 *
 * Return a loadorder in printable form.
 */
static const char *debugstr_loadorder( enum loadorder_type lo[] )
{
    int i;
    char buffer[LOADORDER_NTYPES*3+1];

    buffer[0] = 0;
    for(i = 0; i < LOADORDER_NTYPES; i++)
    {
        if (lo[i] == LOADORDER_INVALID) break;
        switch(lo[i])
        {
        case LOADORDER_DLL: strcat( buffer, "n," ); break;
        case LOADORDER_BI:  strcat( buffer, "b," ); break;
        default:            strcat( buffer, "?," ); break;
        }
    }
    if (buffer[0]) buffer[strlen(buffer)-1] = 0;
    return debugstr_a(buffer);
}


/***************************************************************************
 *	append_load_order
 *
 * Append a load order to the list if necessary.
 */
static void append_load_order(enum loadorder_type lo[], enum loadorder_type append)
{
    int i;

    for (i = 0; i < LOADORDER_NTYPES; i++)
    {
        if (lo[i] == LOADORDER_INVALID)  /* append it here */
        {
            lo[i++] = append;
            lo[i] = LOADORDER_INVALID;
            return;
        }
        if (lo[i] == append) return;  /* already in the list */
    }
    assert(0);  /* cannot get here */
}


/***************************************************************************
 *	parse_load_order
 *
 * Parses the loadorder options from the configuration and puts it into
 * a structure.
 */
static void parse_load_order( const WCHAR *order, enum loadorder_type lo[] )
{
    lo[0] = LOADORDER_INVALID;
    while (*order)
    {
        order += strspnW( order, separatorsW );
        switch(*order)
        {
        case 'N':	/* Native */
        case 'n':
            append_load_order( lo, LOADORDER_DLL );
            break;
        case 'B':	/* Builtin */
        case 'b':
            append_load_order( lo, LOADORDER_BI );
            break;
        }
        order += strcspnW( order, separatorsW );
    }
}


/***************************************************************************
 *	add_load_order
 *
 * Adds an entry in the list of environment overrides.
 */
static void add_load_order( const module_loadorder_t *plo )
{
    int i;

    for(i = 0; i < env_list.count; i++)
    {
        if(!cmp_sort_func(plo, &env_list.order[i] ))
        {
            /* replace existing option */
            memcpy( env_list.order[i].loadorder, plo->loadorder, sizeof(plo->loadorder));
            return;
        }
    }

    if (i >= env_list.alloc)
    {
        /* No space in current array, make it larger */
        env_list.alloc += LOADORDER_ALLOC_CLUSTER;
        if (env_list.order)
            env_list.order = RtlReAllocateHeap(GetProcessHeap(), 0, env_list.order,
                                               env_list.alloc * sizeof(module_loadorder_t));
        else
            env_list.order = RtlAllocateHeap(GetProcessHeap(), 0,
                                             env_list.alloc * sizeof(module_loadorder_t));
        if(!env_list.order)
        {
            MESSAGE("Virtual memory exhausted\n");
            exit(1);
        }
    }
    memcpy(env_list.order[i].loadorder, plo->loadorder, sizeof(plo->loadorder));
    env_list.order[i].modulename = plo->modulename;
    env_list.count++;
}


/***************************************************************************
 *	add_load_order_set
 *
 * Adds a set of entries in the list of command-line overrides from the key parameter.
 */
static void add_load_order_set( WCHAR *entry )
{
    module_loadorder_t ldo;
    WCHAR *end = strchrW( entry, '=' );

    if (!end) return;
    *end++ = 0;
    parse_load_order( end, ldo.loadorder );

    while (*entry)
    {
        entry += strspnW( entry, separatorsW );
        end = entry + strcspnW( entry, separatorsW );
        if (*end) *end++ = 0;
        if (*entry)
        {
            WCHAR *ext = strrchrW(entry, '.');
            if (ext) remove_dll_ext( ext );
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
    const char *order = getenv( "WINEDLLOVERRIDES" );
    UNICODE_STRING strW;
    WCHAR *entry, *next;

    init_done = 1;
    if (!order) return;

    if (!strcmp( order, "help" ))
    {
        MESSAGE( "Syntax:\n"
                 "  WINEDLLOVERRIDES=\"entry;entry;entry...\"\n"
                 "    where each entry is of the form:\n"
                 "        module[,module...]={native|builtin}[,{b|n}]\n"
                 "\n"
                 "    Only the first letter of the override (native or builtin)\n"
                 "    is significant.\n\n"
                 "Example:\n"
                 "  WINEDLLOVERRIDES=\"comdlg32=n,b;shell32,shlwapi=b\"\n" );
        exit(0);
    }

    RtlCreateUnicodeStringFromAsciiz( &strW, order );
    entry = strW.Buffer;
    while (*entry)
    {
        while (*entry && *entry == ';') entry++;
        if (!*entry) break;
        next = strchrW( entry, ';' );
        if (next) *next++ = 0;
        else next = entry + strlenW(entry);
        add_load_order_set( entry );
        entry = next;
    }

    /* sort the array for quick lookup */
    if (env_list.count)
        qsort(env_list.order, env_list.count, sizeof(env_list.order[0]), cmp_sort_func);

    /* Note: we don't free the Unicode string because the
     * stored module names point inside it */
}


/***************************************************************************
 *	get_env_load_order
 *
 * Get the load order for a given module from the WINEDLLOVERRIDES environment variable.
 */
static inline BOOL get_env_load_order( const WCHAR *module, enum loadorder_type lo[] )
{
    module_loadorder_t tmp, *res = NULL;

    tmp.modulename = module;
    /* some bsearch implementations (Solaris) are buggy when the number of items is 0 */
    if (env_list.count &&
        (res = bsearch(&tmp, env_list.order, env_list.count, sizeof(env_list.order[0]), cmp_sort_func)))
        memcpy( lo, res->loadorder, sizeof(res->loadorder) );
    return (res != NULL);
}


/***************************************************************************
 *	get_default_load_order
 *
 * Get the load order for a given module from the default list.
 */
static inline BOOL get_default_load_order( const WCHAR *module, enum loadorder_type lo[] )
{
    const int count = sizeof(default_builtins) / sizeof(default_builtins[0]);
    if (!bsearch( module, default_builtins, count, sizeof(default_builtins[0]), strcmp_func ))
        return FALSE;
    lo[0] = LOADORDER_BI;
    lo[1] = LOADORDER_INVALID;
    TRACE( "got compiled-in default %s for %s\n", debugstr_loadorder(lo), debugstr_w(module) );
    return TRUE;
}


/***************************************************************************
 *	get_standard_key
 *
 * Return a handle to the standard DllOverrides registry section.
 */
static HANDLE get_standard_key(void)
{
    static const WCHAR DllOverridesW[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
                                          'D','l','l','O','v','e','r','r','i','d','e','s',0};
    static HANDLE std_key = (HANDLE)-1;

    if (std_key == (HANDLE)-1)
    {
        OBJECT_ATTRIBUTES attr;
        UNICODE_STRING nameW;
        HANDLE root;

        RtlOpenCurrentUser( KEY_ALL_ACCESS, &root );
        attr.Length = sizeof(attr);
        attr.RootDirectory = root;
        attr.ObjectName = &nameW;
        attr.Attributes = 0;
        attr.SecurityDescriptor = NULL;
        attr.SecurityQualityOfService = NULL;
        RtlInitUnicodeString( &nameW, DllOverridesW );

        /* @@ Wine registry key: HKCU\Software\Wine\DllOverrides */
        if (NtOpenKey( &std_key, KEY_ALL_ACCESS, &attr )) std_key = 0;
        NtClose( root );
    }
    return std_key;
}


/***************************************************************************
 *	get_app_key
 *
 * Get the registry key for the app-specific DllOverrides list.
 */
static HANDLE get_app_key( const WCHAR *app_name )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE root;
    WCHAR *str;
    static const WCHAR AppDefaultsW[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
                                         'A','p','p','D','e','f','a','u','l','t','s','\\',0};
    static const WCHAR DllOverridesW[] = {'\\','D','l','l','O','v','e','r','r','i','d','e','s',0};
    static HANDLE app_key = (HANDLE)-1;

    if (app_key != (HANDLE)-1) return app_key;

    str = RtlAllocateHeap( GetProcessHeap(), 0,
                           sizeof(AppDefaultsW) + sizeof(DllOverridesW) +
                           strlenW(app_name) * sizeof(WCHAR) );
    if (!str) return 0;
    strcpyW( str, AppDefaultsW );
    strcatW( str, app_name );
    strcatW( str, DllOverridesW );

    RtlOpenCurrentUser( KEY_ALL_ACCESS, &root );
    attr.Length = sizeof(attr);
    attr.RootDirectory = root;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, str );

    /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\DllOverrides */
    if (NtOpenKey( &app_key, KEY_ALL_ACCESS, &attr )) app_key = 0;
    NtClose( root );
    RtlFreeHeap( GetProcessHeap(), 0, str );
    return app_key;
}


/***************************************************************************
 *	get_registry_value
 *
 * Load the registry loadorder value for a given module.
 */
static BOOL get_registry_value( HANDLE hkey, const WCHAR *module, enum loadorder_type lo[] )
{
    UNICODE_STRING valueW;
    char buffer[80];
    DWORD count;
    BOOL ret;

    RtlInitUnicodeString( &valueW, module );

    if ((ret = !NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation,
                                 buffer, sizeof(buffer), &count )))
    {
        int i, n = 0;
        WCHAR *str = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)buffer)->Data;

        while (*str)
        {
            enum loadorder_type type = LOADORDER_INVALID;

            while (*str == ',' || isspaceW(*str)) str++;
            if (!*str) break;

            switch(tolowerW(*str))
            {
            case 'n': type = LOADORDER_DLL; break;
            case 'b': type = LOADORDER_BI; break;
            case 's': break;  /* no longer supported, ignore */
            case 0:   break;  /* end of string */
            default:
                ERR("Invalid load order module-type %s, ignored\n", debugstr_w(str));
                break;
            }
            if (type != LOADORDER_INVALID)
            {
                for (i = 0; i < n; i++) if (lo[i] == type) break;  /* already specified */
                if (i == n) lo[n++] = type;
            }
            while (*str && *str != ',' && !isspaceW(*str)) str++;
        }
        lo[n] = LOADORDER_INVALID;
    }
    return ret;
}


/***************************************************************************
 *	get_load_order_value
 *
 * Get the load order for the exact specified module string, looking in:
 * 1. The WINEDLLOVERRIDES environment variable
 * 2. The per-application DllOverrides key
 * 3. The standard DllOverrides key
 */
static BOOL get_load_order_value( HANDLE std_key, HANDLE app_key, const WCHAR *module,
                                  enum loadorder_type loadorder[] )
{
    if (get_env_load_order( module, loadorder ))
    {
        TRACE( "got environment %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_w(module) );
        return TRUE;
    }

    if (app_key && get_registry_value( app_key, module, loadorder ))
    {
        TRACE( "got app defaults %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_w(module) );
        return TRUE;
    }

    if (std_key && get_registry_value( std_key, module, loadorder ))
    {
        TRACE( "got standard key %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_w(module) );
        return TRUE;
    }
    return FALSE;
}


/***************************************************************************
 *	MODULE_GetLoadOrderW	(internal)
 *
 * Return the loadorder of a module.
 * The system directory and '.dll' extension is stripped from the path.
 */
void MODULE_GetLoadOrderW( enum loadorder_type loadorder[], const WCHAR *app_name,
                          const WCHAR *path )
{
    static const WCHAR wildcardW[] = {'*',0};

    HANDLE std_key, app_key = 0;
    WCHAR *module, *basename;
    UNICODE_STRING path_str;
    int len;

    if (!init_done) init_load_order();
    std_key = get_standard_key();
    if (app_name) app_key = get_app_key( app_name );

    TRACE("looking for %s\n", debugstr_w(path));

    loadorder[0] = LOADORDER_INVALID;  /* in case something bad happens below */

    /* Strip path information if the module resides in the system directory
     */
    RtlInitUnicodeString( &path_str, path );
    if (RtlPrefixUnicodeString( &system_dir, &path_str, TRUE ))
    {
        const WCHAR *p = path + system_dir.Length / sizeof(WCHAR);
        while (*p == '\\' || *p == '/') p++;
        if (!strchrW( p, '\\' ) && !strchrW( p, '/' )) path = p;
    }

    if (!(len = strlenW(path))) return;
    if (!(module = RtlAllocateHeap( GetProcessHeap(), 0, (len + 2) * sizeof(WCHAR) ))) return;
    strcpyW( module+1, path );  /* reserve module[0] for the wildcard char */
    basename = (WCHAR *)get_basename( module+1 );

    if (len >= 4) remove_dll_ext( module + 1 + len - 4 );

    /* first explicit module name */
    if (get_load_order_value( std_key, app_key, module+1, loadorder ))
        goto done;

    /* then module basename preceded by '*' */
    basename[-1] = '*';
    if (get_load_order_value( std_key, app_key, basename-1, loadorder ))
        goto done;

    /* then module basename without '*' (only if explicit path) */
    if (basename != module+1 && get_load_order_value( std_key, app_key, basename, loadorder ))
        goto done;

    /* then compiled-in defaults */
    if (get_default_load_order( basename, loadorder ))
        goto done;

    /* then wildcard entry (only if no explicit path) */
    if (basename == module+1 && get_load_order_value( std_key, app_key, wildcardW, loadorder ))
        goto done;

    /* and last the hard-coded default */
    memcpy( loadorder, default_loadorder, sizeof(default_loadorder) );
    TRACE( "got hardcoded default %s for %s\n",
           debugstr_loadorder(loadorder), debugstr_w(path) );

 done:
    RtlFreeHeap( GetProcessHeap(), 0, module );
}
