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
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winternl.h"
#include "ntdll_misc.h"

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
static const WCHAR default_builtins[][16] =
{
    { 'g','d','i','3','2',0 },
    { 'i','c','m','p',0 },
    { 'k','e','r','n','e','l','3','2',0 },
    { 'm','m','s','y','s','t','e','m',0 },
    { 'n','t','d','l','l',0 },
    { 'o','d','b','c','3','2',0 },
    { 's','o','u','n','d',0 },
    { 't','t','y','d','r','v',0 },
    { 'u','s','e','r','3','2',0 },
    { 'w','3','2','s','k','r','n','l',0 },
    { 'w','3','2','s','y','s',0 },
    { 'w','i','n','3','2','s','1','6',0 },
    { 'w','i','n','a','s','p','i',0 },
    { 'w','i','n','e','d','o','s',0 },
    { 'w','i','n','e','p','s','1','6','.','d','r','v',0 },
    { 'w','i','n','e','p','s',0 },
    { 'w','i','n','m','m',0 },
    { 'w','i','n','s','o','c','k',0 },
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

/* default for modules with an explicit path */
static const enum loadorder_type default_path_loadorder[LOADORDER_NTYPES] =
{
    LOADORDER_DLL, LOADORDER_BI, 0
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
    return strcmpiW(((module_loadorder_t *)s1)->modulename, ((module_loadorder_t *)s2)->modulename);
}


/***************************************************************************
 *	strcmp_func
 */
static int strcmp_func(const void *s1, const void *s2)
{
    return strcmpiW( (WCHAR *)s1, (WCHAR *)s2 );
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
                 "  WINEDLLOVERRIDES=\"comdlg32,commdlg=n,b;shell,shell32=b\"\n" );
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
    return TRUE;
}


/***************************************************************************
 *	open_app_key
 *
 * Open the registry key to the app-specific DllOverrides list.
 */
static HKEY open_app_key( const WCHAR *app_name, const WCHAR *module )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HKEY hkey;
    WCHAR *str;
    static const WCHAR AppDefaultsW[] = {'M','a','c','h','i','n','e','\\',
                                         'S','o','f','t','w','a','r','e','\\',
                                         'W','i','n','e','\\',
                                         'W','i','n','e','\\',
                                         'C','o','n','f','i','g','\\',
                                         'A','p','p','D','e','f','a','u','l','t','s','\\',0};
    static const WCHAR DllOverridesW[] = {'\\','D','l','l','O','v','e','r','r','i','d','e','s',0};

    str = RtlAllocateHeap( GetProcessHeap(), 0,
                           sizeof(AppDefaultsW) + sizeof(DllOverridesW) +
                           strlenW(app_name) * sizeof(WCHAR) );
    if (!str) return 0;
    strcpyW( str, AppDefaultsW );
    strcatW( str, app_name );
    strcatW( str, DllOverridesW );

    TRACE( "searching %s in %s\n", debugstr_w(module), debugstr_w(str) );

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, str );

    if (NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr )) hkey = 0;
    RtlFreeHeap( GetProcessHeap(), 0, str );
    return hkey;
}


/***************************************************************************
 *	get_registry_value
 *
 * Load the registry loadorder value for a given module.
 */
static BOOL get_registry_value( HKEY hkey, const WCHAR *module, enum loadorder_type lo[] )
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
 *	MODULE_GetLoadOrderW	(internal)
 *
 * Locate the loadorder of a module.
 * Any path is stripped from the path-argument and so are the extension
 * '.dll' and '.exe'. A lookup in the table can yield an override for
 * the specific dll. Otherwise the default load order is returned.
 */
void MODULE_GetLoadOrderW( enum loadorder_type loadorder[], const WCHAR *app_name,
                          const WCHAR *path, BOOL win32 )
{
    static const WCHAR DllOverridesW[] = {'M','a','c','h','i','n','e','\\',
                                          'S','o','f','t','w','a','r','e','\\',
                                          'W','i','n','e','\\',
                                          'W','i','n','e','\\',
                                          'C','o','n','f','i','g','\\',
                                          'D','l','l','O','v','e','r','r','i','d','e','s',0};

    static HKEY std_key = (HKEY)-1;  /* key to standard section, cached */
    HKEY app_key = 0;
    WCHAR *module, *basename;
    int len;

    if (!init_done) init_load_order();

    TRACE("looking for %s\n", debugstr_w(path));

    loadorder[0] = LOADORDER_INVALID;  /* in case something bad happens below */

    /* Strip path information if the module resides in the system directory
     * (path is already stripped by caller for 16-bit modules)
     */
    if (win32)
    {
        WCHAR sysdir[MAX_PATH+1];
        UNICODE_STRING path_str, sysdir_str;
        if (!GetSystemDirectoryW( sysdir, MAX_PATH )) return;

        RtlInitUnicodeString( &path_str, path );
        RtlInitUnicodeString( &sysdir_str, sysdir );
        if (RtlPrefixUnicodeString( &sysdir_str, &path_str, TRUE ))
        {
            path += sysdir_str.Length / sizeof(WCHAR);
            while (*path == '\\' || *path == '/') path++;
        }
    }

    if (!(len = strlenW(path))) return;
    if (!(module = RtlAllocateHeap( GetProcessHeap(), 0, (len + 2) * sizeof(WCHAR) ))) return;
    strcpyW( module+1, path );  /* reserve module[0] for the wildcard char */

    if (len >= 4) remove_dll_ext( module + 1 + len - 4 );

    /* check environment variable first */
    if (get_env_load_order( module+1, loadorder ))
    {
        TRACE( "got environment %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_w(path) );
        goto done;
    }

    /* then explicit module name in AppDefaults */
    if (app_name)
    {
        app_key = open_app_key( app_name, module+1 );
        if (app_key && get_registry_value( app_key, module+1, loadorder ))
        {
            TRACE( "got app defaults %s for %s\n",
                   debugstr_loadorder(loadorder), debugstr_w(path) );
            goto done;
        }
    }

    /* then explicit module name in standard section */
    if (std_key == (HKEY)-1)
    {
        OBJECT_ATTRIBUTES attr;
        UNICODE_STRING nameW;

        attr.Length = sizeof(attr);
        attr.RootDirectory = 0;
        attr.ObjectName = &nameW;
        attr.Attributes = 0;
        attr.SecurityDescriptor = NULL;
        attr.SecurityQualityOfService = NULL;
        RtlInitUnicodeString( &nameW, DllOverridesW );

        if (NtOpenKey( &std_key, KEY_ALL_ACCESS, &attr )) std_key = 0;
    }

    if (std_key && get_registry_value( std_key, module+1, loadorder ))
    {
        TRACE( "got standard entry %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_w(path) );
        goto done;
    }

    /* then module basename preceded by '*' in AppDefaults */
    basename = (WCHAR *)get_basename( module+1 );
    basename[-1] = '*';
    if (app_key && get_registry_value( app_key, basename-1, loadorder ))
    {
        TRACE( "got app defaults basename %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_w(path) );
        goto done;
    }

    /* then module name preceded by '*' in standard section */
    if (std_key && get_registry_value( std_key, basename-1, loadorder ))
    {
        TRACE( "got standard base name %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_w(path) );
        goto done;
    }

    /* then base name matching compiled-in defaults */
    if (get_default_load_order( basename, loadorder ))
    {
        TRACE( "got compiled-in default %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_w(path) );
        goto done;
    }

    if (basename == module+1)
    {
        static const WCHAR wildcardW[] = {'*',0};

        /* then wildcard entry in AppDefaults (only if no explicit path) */
        if (app_key && get_registry_value( app_key, wildcardW, loadorder ))
        {
            TRACE( "got app defaults wildcard %s for %s\n",
                   debugstr_loadorder(loadorder), debugstr_w(path) );
            goto done;
        }

        /* then wildcard entry in standard section (only if no explicit path) */
        if (std_key && get_registry_value( std_key, wildcardW, loadorder ))
        {
            TRACE( "got standard wildcard %s for %s\n",
                   debugstr_loadorder(loadorder), debugstr_w(path) );
            goto done;
        }

        /* and last the hard-coded default */
        memcpy( loadorder, default_loadorder, sizeof(default_loadorder) );
        TRACE( "got hardcoded default %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_w(path) );
    }
    else  /* module contains an explicit path */
    {
        memcpy( loadorder, default_path_loadorder, sizeof(default_path_loadorder) );
        TRACE( "got hardcoded path default %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_w(path) );
    }

 done:
    if (app_key) NtClose( app_key );
    RtlFreeHeap( GetProcessHeap(), 0, module );
}


/***************************************************************************
 *	MODULE_GetLoadOrderA	(internal)
 *
 * Locate the loadorder of a module.
 * Any path is stripped from the path-argument and so are the extension
 * '.dll' and '.exe'. A lookup in the table can yield an override for
 * the specific dll. Otherwise the default load order is returned.
 *
 * FIXME: should be removed, everything should be Unicode.
 */
void MODULE_GetLoadOrderA( enum loadorder_type loadorder[], const WCHAR *app_name,
                           const char *path, BOOL win32 )
{
    UNICODE_STRING pathW;

    RtlCreateUnicodeStringFromAsciiz( &pathW, path );
    MODULE_GetLoadOrderW( loadorder, app_name, pathW.Buffer, win32 );
    RtlFreeUnicodeString( &pathW );
}
