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
#include "file.h"
#include "module.h"
#include "ntdll_misc.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);

#define LOADORDER_ALLOC_CLUSTER	32	/* Allocate with 32 entries at a time */

typedef struct module_loadorder
{
    const char         *modulename;
    enum loadorder_type loadorder[LOADORDER_NTYPES];
} module_loadorder_t;

struct loadorder_list
{
    int                 count;
    int                 alloc;
    module_loadorder_t *order;
};

/* default load-order if nothing specified */
/* the list must remain sorted by dll name */
static module_loadorder_t default_order_list[] =
{
    { "display",      { LOADORDER_BI, 0, 0 } },
    { "gdi.exe",      { LOADORDER_BI, 0, 0 } },
    { "gdi32",        { LOADORDER_BI, 0, 0 } },
    { "icmp",         { LOADORDER_BI, 0, 0 } },
    { "kernel",       { LOADORDER_BI, 0, 0 } },
    { "kernel32",     { LOADORDER_BI, 0, 0 } },
    { "keyboard",     { LOADORDER_BI, 0, 0 } },
    { "krnl386.exe",  { LOADORDER_BI, 0, 0 } },
    { "mmsystem",     { LOADORDER_BI, 0, 0 } },
    { "mouse",        { LOADORDER_BI, 0, 0 } },
    { "ntdll",        { LOADORDER_BI, 0, 0 } },
    { "odbc32",       { LOADORDER_BI, 0, 0 } },
    { "system.drv",   { LOADORDER_BI, 0, 0 } },
    { "toolhelp",     { LOADORDER_BI, 0, 0 } },
    { "ttydrv",       { LOADORDER_BI, 0, 0 } },
    { "user.exe",     { LOADORDER_BI, 0, 0 } },
    { "user32",       { LOADORDER_BI, 0, 0 } },
    { "w32skrnl",     { LOADORDER_BI, 0, 0 } },
    { "winaspi",      { LOADORDER_BI, 0, 0 } },
    { "winedos",      { LOADORDER_BI, 0, 0 } },
    { "wineps",       { LOADORDER_BI, 0, 0 } },
    { "wing",         { LOADORDER_BI, 0, 0 } },
    { "winmm",        { LOADORDER_BI, 0, 0 } },
    { "winsock",      { LOADORDER_BI, 0, 0 } },
    { "wnaspi32",     { LOADORDER_BI, 0, 0 } },
    { "wow32",        { LOADORDER_BI, 0, 0 } },
    { "wprocs",       { LOADORDER_BI, 0, 0 } },
    { "ws2_32",       { LOADORDER_BI, 0, 0 } },
    { "wsock32",      { LOADORDER_BI, 0, 0 } },
    { "x11drv",       { LOADORDER_BI, 0, 0 } }
};

static const struct loadorder_list default_list =
{
    sizeof(default_order_list)/sizeof(default_order_list[0]),
    sizeof(default_order_list)/sizeof(default_order_list[0]),
    default_order_list
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
    return FILE_strcasecmp(((module_loadorder_t *)s1)->modulename,
                           ((module_loadorder_t *)s2)->modulename);
}


/***************************************************************************
 *	get_basename
 *
 * Return the base name of a file name (i.e. remove the path components).
 */
static const char *get_basename( const char *name )
{
    const char *ptr;

    if (name[0] && name[1] == ':') name += 2;  /* strip drive specification */
    if ((ptr = strrchr( name, '\\' ))) name = ptr + 1;
    if ((ptr = strrchr( name, '/' ))) name = ptr + 1;
    return name;
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
static void parse_load_order( const char *order, enum loadorder_type lo[] )
{
    lo[0] = LOADORDER_INVALID;
    while (*order)
    {
        order += strspn( order, ", \t" );
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
        order += strcspn( order, ", \t" );
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
    env_list.order[i].modulename = RtlAllocateHeap(GetProcessHeap(), 0, strlen(plo->modulename)+1);
    strcpy( (char *)env_list.order[i].modulename, plo->modulename );
    env_list.count++;
}


/***************************************************************************
 *	add_load_order_set
 *
 * Adds a set of entries in the list of command-line overrides from the key parameter.
 */
static void add_load_order_set( char *entry )
{
    module_loadorder_t ldo;
    char *end = strchr( entry, '=' );

    if (!end) return;
    *end++ = 0;
    parse_load_order( end, ldo.loadorder );

    while (*entry)
    {
        entry += strspn( entry, ", \t" );
        end = entry + strcspn( entry, ", \t" );
        if (*end) *end++ = 0;
        if (*entry)
	{
            char *ext = strrchr(entry, '.');
            if(ext && !FILE_strcasecmp( ext, ".dll" )) *ext = 0;
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
    char *str, *entry, *next;

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

    str = RtlAllocateHeap( GetProcessHeap(), 0, strlen(order)+1 );
    strcpy( str, order );
    entry = str;
    while (*entry)
    {
        while (*entry && *entry == ';') entry++;
        if (!*entry) break;
        next = strchr( entry, ';' );
        if (next) *next++ = 0;
        else next = entry + strlen(entry);
        add_load_order_set( entry );
        entry = next;
    }
    RtlFreeHeap( GetProcessHeap(), 0, str );

    /* sort the array for quick lookup */
    if (env_list.count)
        qsort(env_list.order, env_list.count, sizeof(env_list.order[0]), cmp_sort_func);
}


/***************************************************************************
 *	get_list_load_order
 *
 * Get the load order for a given module from the command-line or
 * default lists.
 */
static BOOL get_list_load_order( const char *module, const struct loadorder_list *list,
                                 enum loadorder_type lo[] )
{
    module_loadorder_t tmp, *res = NULL;

    tmp.modulename = module;
    /* some bsearch implementations (Solaris) are buggy when the number of items is 0 */
    if (list->count && (res = bsearch(&tmp, list->order, list->count, sizeof(list->order[0]), cmp_sort_func)))
        memcpy( lo, res->loadorder, sizeof(res->loadorder) );
    return (res != NULL);
}


/***************************************************************************
 *	open_app_key
 *
 * Open the registry key to the app-specific DllOverrides list.
 */
static HKEY open_app_key( const WCHAR *app_name, const char *module )
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

    TRACE( "searching '%s' in %s\n", module, debugstr_w(str) );

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
static BOOL get_registry_value( HKEY hkey, const char *module, enum loadorder_type lo[] )
{
    UNICODE_STRING valueW;
    char buffer[80];
    DWORD count;
    BOOL ret;

    RtlCreateUnicodeStringFromAsciiz( &valueW, module );

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
    RtlFreeUnicodeString( &valueW );
    return ret;
}


/***************************************************************************
 *	MODULE_GetBuiltinPath
 *
 * Get the path of a builtin module when the native file does not exist.
 */
BOOL MODULE_GetBuiltinPath( const char *libname, const char *ext, char *filename, UINT size )
{
    char *p;
    BOOL ret = FALSE;
    UINT len = GetSystemDirectoryA( filename, size );

    if (FILE_contains_path( libname ))
    {
        char *tmp;

        /* if the library name contains a path and can not be found,
         * return an error.
         * exception: if the path is the system directory, proceed,
         * so that modules which are not PE modules can be loaded.
         * If the library name does not contain a path and can not
         * be found, assume the system directory is meant */

        if (strlen(libname) >= size) return FALSE;  /* too long */
        if (strchr( libname, '/' ))  /* need to convert slashes */
        {
            if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, strlen(libname)+1 ))) return FALSE;
            strcpy( tmp, libname );
            for (p = tmp; *p; p++) if (*p == '/') *p = '\\';
        }
        else tmp = (char *)libname;

        if (!FILE_strncasecmp( filename, tmp, len ) && tmp[len] == '\\')
        {
            strcpy( filename, tmp );
            ret = TRUE;
        }
        if (tmp != libname) RtlFreeHeap( GetProcessHeap(), 0, tmp );
        if (!ret) return FALSE;
    }
    else
    {
        if (strlen(libname) >= size - len - 1) return FALSE;
        filename[len] = '\\';
        strcpy( filename+len+1, libname );
    }

    /* if the filename doesn't have an extension, append the default */
    if (!(p = strrchr( filename, '.')) || strchr( p, '/' ) || strchr( p, '\\'))
    {
        if (strlen(filename) + strlen(ext) >= size) return FALSE;
        strcat( filename, ext );
    }
    return TRUE;
}


/***************************************************************************
 *	MODULE_GetLoadOrder	(internal)
 *
 * Locate the loadorder of a module.
 * Any path is stripped from the path-argument and so are the extension
 * '.dll' and '.exe'. A lookup in the table can yield an override for
 * the specific dll. Otherwise the default load order is returned.
 *
 * FIXME: 'path' should be Unicode too.
 */
void MODULE_GetLoadOrder( enum loadorder_type loadorder[], const WCHAR *app_name,
                          const char *path, BOOL win32 )
{
    static const WCHAR DllOverridesW[] = {'M','a','c','h','i','n','e','\\',
                                          'S','o','f','t','w','a','r','e','\\',
                                          'W','i','n','e','\\',
                                          'W','i','n','e','\\',
                                          'C','o','n','f','i','g','\\',
                                          'D','l','l','O','v','e','r','r','i','d','e','s',0};

    static HKEY std_key = (HKEY)-1;  /* key to standard section, cached */
    HKEY app_key = 0;
    char *module, *basename;
    int len;

    if (!init_done) init_load_order();

    TRACE("looking for %s\n", path);

    loadorder[0] = LOADORDER_INVALID;  /* in case something bad happens below */

    /* Strip path information if the module resides in the system directory
     * (path is already stripped by caller for 16-bit modules)
     */
    if (win32)
    {
        char sysdir[MAX_PATH+1];
        if (!GetSystemDirectoryA( sysdir, MAX_PATH )) return;
        if (!FILE_strncasecmp( sysdir, path, strlen (sysdir) ))
        {
            path += strlen(sysdir);
            while (*path == '\\' || *path == '/') path++;
        }
    }

    if (!(len = strlen(path))) return;
    if (!(module = RtlAllocateHeap( GetProcessHeap(), 0, len + 2 ))) return;
    strcpy( module+1, path );  /* reserve module[0] for the wildcard char */

    if (len >= 4)
    {
        char *ext = module + 1 + len - 4;
        if (!FILE_strcasecmp( ext, ".dll" )) *ext = 0;
    }

    /* check environment variable first */
    if (get_list_load_order( module+1, &env_list, loadorder ))
    {
        TRACE( "got environment %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_a(path) );
        goto done;
    }

    /* then explicit module name in AppDefaults */
    if (app_name)
    {
        app_key = open_app_key( app_name, module+1 );
        if (app_key && get_registry_value( app_key, module+1, loadorder ))
        {
            TRACE( "got app defaults %s for %s\n",
                   debugstr_loadorder(loadorder), debugstr_a(path) );
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
               debugstr_loadorder(loadorder), debugstr_a(path) );
        goto done;
    }

    /* then module basename preceded by '*' in AppDefaults */
    basename = (char *)get_basename( module+1 );
    basename[-1] = '*';
    if (app_key && get_registry_value( app_key, basename-1, loadorder ))
    {
        TRACE( "got app defaults basename %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_a(path) );
        goto done;
    }

    /* then module name preceded by '*' in standard section */
    if (std_key && get_registry_value( std_key, basename-1, loadorder ))
    {
        TRACE( "got standard base name %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_a(path) );
        goto done;
    }

    /* then base name matching compiled-in defaults */
    if (get_list_load_order( basename, &default_list, loadorder ))
    {
        TRACE( "got compiled-in default %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_a(path) );
        goto done;
    }

    if (basename == module+1)
    {
        /* then wildcard entry in AppDefaults (only if no explicit path) */
        if (app_key && get_registry_value( app_key, "*", loadorder ))
        {
            TRACE( "got app defaults wildcard %s for %s\n",
                   debugstr_loadorder(loadorder), debugstr_a(path) );
            goto done;
        }

        /* then wildcard entry in standard section (only if no explicit path) */
        if (std_key && get_registry_value( std_key, "*", loadorder ))
        {
            TRACE( "got standard wildcard %s for %s\n",
                   debugstr_loadorder(loadorder), debugstr_a(path) );
            goto done;
        }

        /* and last the hard-coded default */
        memcpy( loadorder, default_loadorder, sizeof(default_loadorder) );
        TRACE( "got hardcoded default %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_a(path) );
    }
    else  /* module contains an explicit path */
    {
        memcpy( loadorder, default_path_loadorder, sizeof(default_path_loadorder) );
        TRACE( "got hardcoded path default %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_a(path) );
    }

 done:
    if (app_key) NtClose( app_key );
    RtlFreeHeap( GetProcessHeap(), 0, module );
}
