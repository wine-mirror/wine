/*
 * Module/Library loadorder
 *
 * Copyright 1999 Bertho Stultiens
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winerror.h"
#include "winternl.h"
#include "file.h"
#include "module.h"

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
    { "system",       { LOADORDER_BI, 0, 0 } },
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

static struct loadorder_list cmdline_list;


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
 *	get_tok	(internal, static)
 *
 * strtok wrapper for non-destructive buffer writing.
 * NOTE: strtok is not reentrant and therefore this code is neither.
 */
static char *get_tok(const char *str, const char *delim)
{
	static char *buf = NULL;
	char *cptr;

	if(!str && !buf)
		return NULL;

	if(str && buf)
	{
		HeapFree(GetProcessHeap(), 0, buf);
		buf = NULL;
	}

	if(str && !buf)
	{
		buf = HeapAlloc(GetProcessHeap(), 0, strlen(str)+1);
		strcpy( buf, str );
		cptr = strtok(buf, delim);
	}
	else
	{
		cptr = strtok(NULL, delim);
	}

	if(!cptr)
	{
		HeapFree(GetProcessHeap(), 0, buf);
		buf = NULL;
	}
	return cptr;
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
 *	ParseLoadOrder	(internal, static)
 *
 * Parses the loadorder options from the configuration and puts it into
 * a structure.
 */
static BOOL ParseLoadOrder(char *order, enum loadorder_type lo[])
{
    static int warn;
	char *cptr;
	int n = 0;

	cptr = get_tok(order, ", \t");
	while(cptr)
	{
            enum loadorder_type type = LOADORDER_INVALID;

		if(n >= LOADORDER_NTYPES-1)
		{
			ERR("More than existing %d module-types specified, rest ignored\n", LOADORDER_NTYPES-1);
			break;
		}

		switch(*cptr)
		{
		case 'N':	/* Native */
		case 'n': type = LOADORDER_DLL; break;

		case 'S':	/* So */
		case 's':
                    if (!warn++) MESSAGE("Load order 'so' no longer supported, ignored\n");
                    break;

		case 'B':	/* Builtin */
		case 'b': type = LOADORDER_BI; break;

		default:
			ERR("Invalid load order module-type '%s', ignored\n", cptr);
		}

                if(type != LOADORDER_INVALID) lo[n++] = type;
		cptr = get_tok(NULL, ", \t");
	}
        lo[n] = LOADORDER_INVALID;
	return TRUE;
}


/***************************************************************************
 *	AddLoadOrder	(internal, static)
 *
 * Adds an entry in the list of command-line overrides.
 */
static BOOL AddLoadOrder(module_loadorder_t *plo)
{
	int i;

	/* TRACE(module, "'%s' -> %08lx\n", plo->modulename, *(DWORD *)(plo->loadorder)); */

	for(i = 0; i < cmdline_list.count; i++)
	{
            if(!cmp_sort_func(plo, &cmdline_list.order[i] ))
            {
                /* replace existing option */
                memcpy( cmdline_list.order[i].loadorder, plo->loadorder, sizeof(plo->loadorder));
                return TRUE;
            }
	}

	if (i >= cmdline_list.alloc)
	{
		/* No space in current array, make it larger */
		cmdline_list.alloc += LOADORDER_ALLOC_CLUSTER;
		cmdline_list.order = HeapReAlloc(GetProcessHeap(), 0, cmdline_list.order,
                                          cmdline_list.alloc * sizeof(module_loadorder_t));
		if(!cmdline_list.order)
		{
			MESSAGE("Virtual memory exhausted\n");
			exit(1);
		}
	}
	memcpy(cmdline_list.order[i].loadorder, plo->loadorder, sizeof(plo->loadorder));
	cmdline_list.order[i].modulename = HeapAlloc(GetProcessHeap(), 0, strlen(plo->modulename)+1);
	strcpy( (char *)cmdline_list.order[i].modulename, plo->modulename );
	cmdline_list.count++;
	return TRUE;
}


/***************************************************************************
 *	AddLoadOrderSet	(internal, static)
 *
 * Adds a set of entries in the list of command-line overrides from the key parameter.
 */
static BOOL AddLoadOrderSet(char *key, char *order)
{
	module_loadorder_t ldo;
	char *cptr;

	/* Parse the loadorder before the rest because strtok is not reentrant */
	if(!ParseLoadOrder(order, ldo.loadorder))
		return FALSE;

	cptr = get_tok(key, ", \t");
	while(cptr)
	{
		char *ext = strrchr(cptr, '.');
		if(ext && !FILE_strcasecmp( ext, ".dll" )) *ext = 0;
		ldo.modulename = cptr;
		if(!AddLoadOrder(&ldo)) return FALSE;
		cptr = get_tok(NULL, ", \t");
	}
	return TRUE;
}


/***************************************************************************
 *	MODULE_AddLoadOrderOption
 *
 * The commandline option is in the form:
 * name[,name,...]=native[,b,...]
 */
void MODULE_AddLoadOrderOption( const char *option )
{
    char *value, *key = HeapAlloc(GetProcessHeap(), 0, strlen(option)+1);

    strcpy( key, option );
    if (!(value = strchr(key, '='))) goto error;
    *value++ = '\0';

    TRACE("Commandline override '%s' = '%s'\n", key, value);

    if (!AddLoadOrderSet(key, value)) goto error;
    HeapFree(GetProcessHeap(), 0, key);

    /* sort the array for quick lookup */
    qsort(cmdline_list.order, cmdline_list.count, sizeof(cmdline_list.order[0]), cmp_sort_func);
    return;

 error:
    MESSAGE( "Syntax: -dll name[,name[,...]]={native|so|builtin}[,{n|s|b}[,...]]\n"
             "    - 'name' is the name of any dll without extension\n"
             "    - the order of loading (native, so and builtin) can be abbreviated\n"
             "      with the first letter\n"
             "    - the option can be specified multiple times\n"
             "    Example:\n"
             "    -dll comdlg32,commdlg=n -dll shell,shell32=b\n" );
    ExitProcess(1);
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
static HKEY open_app_key( const char *module )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HKEY hkey, appkey;
    char buffer[MAX_PATH+16], *appname;
    static const WCHAR AppDefaultsW[] = {'M','a','c','h','i','n','e','\\',
                                         'S','o','f','t','w','a','r','e','\\',
                                         'W','i','n','e','\\',
                                         'W','i','n','e','\\',
                                         'C','o','n','f','i','g','\\',
                                         'A','p','p','D','e','f','a','u','l','t','s',0};

    if (!GetModuleFileNameA( 0, buffer, MAX_PATH ))
    {
        WARN( "could not get module file name loading %s\n", module );
        return 0;
    }
    appname = (char *)get_basename( buffer );

    TRACE( "searching '%s' in AppDefaults\\%s\\DllOverrides\n", module, appname );

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, AppDefaultsW );

    if (NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr )) return 0;
    attr.RootDirectory = hkey;

    /* open AppDefaults\\appname\\DllOverrides key */
    strcat( appname, "\\DllOverrides" );
    RtlCreateUnicodeStringFromAsciiz( &nameW, appname );
    if (NtOpenKey( &appkey, KEY_ALL_ACCESS, &attr )) appkey = 0;
    RtlFreeUnicodeString( &nameW );
    NtClose( hkey );
    return appkey;
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
            if (!(tmp = HeapAlloc( GetProcessHeap(), 0, strlen(libname)+1 ))) return FALSE;
            strcpy( tmp, libname );
            for (p = tmp; *p; p++) if (*p == '/') *p = '\\';
        }
        else tmp = (char *)libname;

        if (!FILE_strncasecmp( filename, tmp, len ) && tmp[len] == '\\')
        {
            strcpy( filename, tmp );
            ret = TRUE;
        }
        if (tmp != libname) HeapFree( GetProcessHeap(), 0, tmp );
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
 */
void MODULE_GetLoadOrder( enum loadorder_type loadorder[], const char *path, BOOL win32 )
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

    TRACE("looking for %s\n", path);

    loadorder[0] = LOADORDER_INVALID;  /* in case something bad happens below */

    /* Strip path information for 16 bit modules or if the module
     * resides in the system directory */
    if (!win32)
    {
        path = get_basename( path );
        if (BUILTIN_IsPresent(path))
        {
            TRACE( "forcing loadorder to builtin for %s\n", debugstr_a(path) );
            /* force builtin loadorder since the dll is already in memory */
            loadorder[0] = LOADORDER_BI;
            loadorder[1] = LOADORDER_INVALID;
            return;
        }
    }
    else
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
    if (!(module = HeapAlloc( GetProcessHeap(), 0, len + 2 ))) return;
    strcpy( module+1, path );  /* reserve module[0] for the wildcard char */

    if (len >= 4)
    {
        char *ext = module + 1 + len - 4;
        if (!FILE_strcasecmp( ext, ".dll" )) *ext = 0;
    }

    /* check command-line first */
    if (get_list_load_order( module+1, &cmdline_list, loadorder ))
    {
        TRACE( "got cmdline %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_a(path) );
        goto done;
    }

    /* then explicit module name in AppDefaults */
    app_key = open_app_key( module+1 );
    if (app_key && get_registry_value( app_key, module+1, loadorder ))
    {
        TRACE( "got app defaults %s for %s\n",
               debugstr_loadorder(loadorder), debugstr_a(path) );
        goto done;
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
    HeapFree( GetProcessHeap(), 0, module );
}
