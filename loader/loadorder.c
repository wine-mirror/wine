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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winreg.h"
#include "winerror.h"
#include "file.h"
#include "module.h"
#include "wine/debug.h"

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
    { "display",      { LOADORDER_BI,  0,             0, 0 } },
    { "gdi.exe",      { LOADORDER_BI,  0,             0, 0 } },
    { "gdi32",        { LOADORDER_BI,  0,             0, 0 } },
    { "glide2x",      { LOADORDER_SO,  LOADORDER_DLL, 0, 0 } },
    { "glide3x",      { LOADORDER_SO,  LOADORDER_DLL, 0, 0 } },
    { "icmp",         { LOADORDER_BI,  0,             0, 0 } },
    { "kernel",       { LOADORDER_BI,  0,             0, 0 } },
    { "kernel32",     { LOADORDER_BI,  0,             0, 0 } },
    { "keyboard",     { LOADORDER_BI,  0,             0, 0 } },
    { "krnl386.exe",  { LOADORDER_BI,  0,             0, 0 } },
    { "mmsystem",     { LOADORDER_BI,  0,             0, 0 } },
    { "mouse",        { LOADORDER_BI,  0,             0, 0 } },
    { "ntdll",        { LOADORDER_BI,  0,             0, 0 } },
    { "odbc32",       { LOADORDER_BI,  0,             0, 0 } },
    { "system",       { LOADORDER_BI,  0,             0, 0 } },
    { "toolhelp",     { LOADORDER_BI,  0,             0, 0 } },
    { "ttydrv",       { LOADORDER_BI,  0,             0, 0 } },
    { "user.exe",     { LOADORDER_BI,  0,             0, 0 } },
    { "user32",       { LOADORDER_BI,  0,             0, 0 } },
    { "w32skrnl",     { LOADORDER_BI,  0,             0, 0 } },
    { "winaspi",      { LOADORDER_BI,  0,             0, 0 } },
    { "windebug",     { LOADORDER_DLL, LOADORDER_BI,  0, 0 } },
    { "winedos",      { LOADORDER_BI,  0,             0, 0 } },
    { "wineps",       { LOADORDER_BI,  0,             0, 0 } },
    { "wing",         { LOADORDER_BI,  0,             0, 0 } },
    { "winmm",        { LOADORDER_BI,  0,             0, 0 } },
    { "winsock",      { LOADORDER_BI,  0,             0, 0 } },
    { "wnaspi32",     { LOADORDER_BI,  0,             0, 0 } },
    { "wow32",        { LOADORDER_BI,  0,             0, 0 } },
    { "wprocs",       { LOADORDER_BI,  0,             0, 0 } },
    { "ws2_32",       { LOADORDER_BI,  0,             0, 0 } },
    { "wsock32",      { LOADORDER_BI,  0,             0, 0 } },
    { "x11drv",       { LOADORDER_BI,  0,             0, 0 } }
};

static const struct loadorder_list default_list =
{
    sizeof(default_order_list)/sizeof(default_order_list[0]),
    sizeof(default_order_list)/sizeof(default_order_list[0]),
    default_order_list
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

		case 'E':	/* Elfdll */
		case 'e':
                    if (!warn++) MESSAGE("Load order 'elfdll' no longer supported, ignored\n");
                    break;
		case 'S':	/* So */
		case 's': type = LOADORDER_SO; break;

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
 *	set_registry_keys
 *
 * Set individual registry keys for a multiple dll specification
 * Helper for MODULE_InitLoadOrder().
 */
inline static void set_registry_keys( HKEY hkey, char *module, const char *buffer )
{
    static int warn;
    char *p = get_tok( module, ", \t" );

    TRACE( "converting \"%s\" = \"%s\"\n", module, buffer );

    if (!warn)
        MESSAGE( "Warning: setting multiple modules in a single DllOverrides entry is no longer\n"
                 "recommended. It is suggested that you rewrite the configuration file entry:\n\n"
                 "\"%s\" = \"%s\"\n\n"
                 "into something like:\n\n", module, buffer );
    while (p)
    {
        if (!warn) MESSAGE( "\"%s\" = \"%s\"\n", p, buffer );
        /* only set it if not existing already */
        if (RegQueryValueExA( hkey, p, 0, NULL, NULL, NULL ) == ERROR_FILE_NOT_FOUND)
            RegSetValueExA( hkey, p, 0, REG_SZ, buffer, strlen(buffer)+1 );
        p = get_tok( NULL, ", \t" );
    }
    if (!warn) MESSAGE( "\n" );
    warn = 1;
}


/***************************************************************************
 *	MODULE_InitLoadOrder
 *
 * Convert entries containing multiple dll names (old syntax) to the
 * new one dll module per entry syntax
 */
void MODULE_InitLoadOrder(void)
{
    char module[80];
    char buffer[1024];
    char *p;
    HKEY hkey;
    DWORD index = 0;

    if (RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\DllOverrides", &hkey ))
        return;

    for (;;)
    {
        DWORD type, count = sizeof(buffer), name_len = sizeof(module);

        if (RegEnumValueA( hkey, index, module, &name_len, NULL, &type, buffer, &count )) break;
        p = module;
        while (isspace(*p)) p++;
        p += strcspn( p, ", \t" );
        while (isspace(*p)) p++;
        if (*p)
        {
            RegDeleteValueA( hkey, module );
            set_registry_keys( hkey, module, buffer );
        }
        else index++;
    }
    RegCloseKey( hkey );
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
 *	get_app_load_order
 *
 * Get the load order for a given module from the app-specific DllOverrides list.
 * Also look for default '*' key if no module key found.
 */
static BOOL get_app_load_order( const char *module, enum loadorder_type lo[], BOOL *got_default )
{
    HKEY hkey, appkey;
    DWORD count, type, res;
    char buffer[MAX_PATH+16], *appname, *p;

    if (!GetModuleFileName16( GetCurrentTask(), buffer, MAX_PATH ) &&
        !GetModuleFileNameA( 0, buffer, MAX_PATH ))
    {
        WARN( "could not get module file name loading %s\n", module );
        return FALSE;
    }
    appname = buffer;
    if ((p = strrchr( appname, '/' ))) appname = p + 1;
    if ((p = strrchr( appname, '\\' ))) appname = p + 1;

    TRACE( "searching '%s' in AppDefaults\\%s\\DllOverrides\n", module, appname );

    if (RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\AppDefaults", &hkey ))
        return FALSE;

    /* open AppDefaults\\appname\\DllOverrides key */
    strcat( appname, "\\DllOverrides" );
    res = RegOpenKeyA( hkey, appname, &appkey );
    RegCloseKey( hkey );
    if (res) return FALSE;

    count = sizeof(buffer);
    if ((res = RegQueryValueExA( appkey, module, NULL, &type, buffer, &count )))
    {
        if (!(res = RegQueryValueExA( appkey, "*", NULL, &type, buffer, &count )))
            *got_default = TRUE;
    }
    else TRACE( "got app loadorder '%s' for '%s'\n", buffer, module );
    RegCloseKey( appkey );
    if (res) return FALSE;
    return ParseLoadOrder( buffer, lo );
}


/***************************************************************************
 *	get_standard_load_order
 *
 * Get the load order for a given module from the main DllOverrides list
 * Also look for default '*' key if no module key found.
 */
static BOOL get_standard_load_order( const char *module, enum loadorder_type lo[],
                                     BOOL *got_default )
{
    HKEY hkey;
    DWORD count, type, res;
    char buffer[80];

    if (RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\DllOverrides", &hkey ))
        return FALSE;

    count = sizeof(buffer);
    if ((res = RegQueryValueExA( hkey, module, NULL, &type, buffer, &count )))
    {
        if (!(res = RegQueryValueExA( hkey, "*", NULL, &type, buffer, &count )))
            *got_default = TRUE;
    }
    else TRACE( "got standard loadorder '%s' for '%s'\n", buffer, module );
    RegCloseKey( hkey );
    if (res) return FALSE;
    return ParseLoadOrder( buffer, lo );
}


/***************************************************************************
 *	get_default_load_order
 *
 * Get the default load order if nothing specified for a given dll.
 */
static void get_default_load_order( enum loadorder_type lo[] )
{
    DWORD res;
    static enum loadorder_type default_loadorder[LOADORDER_NTYPES];
    static int loaded;

    if (!loaded)
    {
        char buffer[80];
        HKEY hkey;

        if (!(res = RegOpenKeyA( HKEY_LOCAL_MACHINE,
                                 "Software\\Wine\\Wine\\Config\\DllDefaults", &hkey )))
        {
            DWORD type, count = sizeof(buffer);

            res = RegQueryValueExA( hkey, "DefaultLoadOrder", NULL, &type, buffer, &count );
            RegCloseKey( hkey );
        }
        if (res) strcpy( buffer, "n,b,s" );
        ParseLoadOrder( buffer, default_loadorder );
        loaded = 1;
        TRACE( "got default loadorder '%s'\n", buffer );
    }
    memcpy( lo, default_loadorder, sizeof(default_loadorder) );
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
	char fname[256];
	char sysdir[MAX_PATH+1];
	char *cptr;
	char *name;
	int len;
        BOOL got_app_default = FALSE, got_std_default = FALSE;
        enum loadorder_type lo_default[LOADORDER_NTYPES];

	TRACE("looking for %s\n", path);

        if ( ! GetSystemDirectoryA ( sysdir, MAX_PATH ) ) goto done;

	/* Strip path information for 16 bit modules or if the module
	   resides in the system directory */
	if ( !win32 || !FILE_strncasecmp ( sysdir, path, strlen (sysdir) ) )
	{

	    cptr = strrchr(path, '\\');
	    if(!cptr)
	        name = strrchr(path, '/');
	    else
	        name = strrchr(cptr, '/');

	    if(!name)
	        name = cptr ? cptr+1 : (char *)path;
	    else
	        name++;

	    if((cptr = strchr(name, ':')) != NULL)	/* Also strip drive if in format 'C:MODULE.DLL' */
	        name = cptr+1;
	}
	else
	  name = (char *)path;

	len = strlen(name);
	if(len >= sizeof(fname) || len <= 0)
	{
            WARN("Path '%s' -> '%s' reduces to zilch or just too large...\n", path, name);
            goto done;
	}

	strcpy(fname, name);
	if(len >= 4 && !FILE_strcasecmp(fname+len-4, ".dll")) fname[len-4] = '\0';

        /* check command-line first */
        if (get_list_load_order( fname, &cmdline_list, loadorder )) return;

        /* then app-specific config */
        if (get_app_load_order( fname, loadorder, &got_app_default ))
        {
            if (!got_app_default) return;
            /* save the default value for later on */
            memcpy( lo_default, loadorder, sizeof(lo_default) );
        }

        /* then standard config */
        if (get_standard_load_order( fname, loadorder, &got_std_default ))
        {
            if (!got_std_default) return;
            /* save the default value for later on */
            if (!got_app_default) memcpy( lo_default, loadorder, sizeof(lo_default) );
        }

        /* then compiled-in defaults */
        if (get_list_load_order( fname, &default_list, loadorder )) return;

 done:
        /* last, return the default */
        if (got_app_default || got_std_default)
            memcpy( loadorder, lo_default, sizeof(lo_default) );
        else
            get_default_load_order( loadorder );
}
