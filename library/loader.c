/*
 * Win32 builtin dlls support
 *
 * Copyright 2000 Alexandre Julliard
 */

#include "config.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef HAVE_DL_API
#include <dlfcn.h>
#endif

#include "windef.h"
#include "wine/library.h"

#define MAX_DLLS 100

static struct
{
    const IMAGE_NT_HEADERS *nt;           /* NT header */
    const char             *filename;     /* DLL file name */
} builtin_dlls[MAX_DLLS];

static int nb_dlls;

static const IMAGE_NT_HEADERS *main_exe;

static load_dll_callback_t load_dll_callback;

static char **dll_paths;
static int nb_dll_paths;
static int dll_path_maxlen;
static int init_done;


/* build the dll load path from the WINEDLLPATH variable */
static void build_dll_path(void)
{
    int count = 0;
    char *p, *path = getenv( "WINEDLLPATH" );

    init_done = 1;
    if (!path) return;
    path = strdup(path);
    p = path;
    while (*p)
    {
        while (*p == ':') p++;
        if (!*p) break;
        count++;
        while (*p && *p != ':') p++;
    }

    dll_paths = malloc( count * sizeof(*dll_paths) );
    p = path;
    nb_dll_paths = 0;
    while (*p)
    {
        while (*p == ':') *p++ = 0;
        if (!*p) break;
        dll_paths[nb_dll_paths] = p;
        while (*p && *p != ':') p++;
        if (p - dll_paths[nb_dll_paths] > dll_path_maxlen)
            dll_path_maxlen = p - dll_paths[nb_dll_paths];
        nb_dll_paths++;
    }
}


/* open a library for a given dll, searching in the dll path
 * 'name' must be the Windows dll name (e.g. "kernel32.dll") */
static void *dlopen_dll( const char *name )
{
#ifdef HAVE_DL_API
    int i, namelen = strlen(name);
    char *buffer, *p;
    void *ret = NULL;

    if (!init_done) build_dll_path();

    /* check for .dll or .exe extension to remove */
    if ((p = strrchr( name, '.' )))
    {
        if (!strcasecmp( p, ".dll" ) || !strcasecmp( p, ".exe" )) namelen -= 4;
    }

    buffer = malloc( dll_path_maxlen + namelen + 8 );

    /* store the name at the end of the buffer, prefixed by /lib and followed by .so */
    p = buffer + dll_path_maxlen;
    memcpy( p, "/lib", 4 );
    for (i = 0, p += 4; i < namelen; i++, p++) *p = tolower(name[i]);
    memcpy( p, ".so", 4 );

    for (i = 0; i < nb_dll_paths; i++)
    {
        int len = strlen(dll_paths[i]);
        char *p = buffer + dll_path_maxlen - len;
        memcpy( p, dll_paths[i], len );
        if ((ret = dlopen( p, RTLD_NOW ))) break;
    }

    /* now try the default dlopen search path */
    if (!ret) ret = dlopen( buffer + dll_path_maxlen + 1, RTLD_NOW );
    free( buffer );
    return ret;
#else
    return NULL;
#endif
}


/***********************************************************************
 *           __wine_dll_register
 *
 * Register a built-in DLL descriptor.
 */
void __wine_dll_register( const IMAGE_NT_HEADERS *header, const char *filename )
{
    if (load_dll_callback) load_dll_callback( header, filename );
    else
    {
        if (!(header->FileHeader.Characteristics & IMAGE_FILE_DLL))
            main_exe = header;
        else
        {
            assert( nb_dlls < MAX_DLLS );
            builtin_dlls[nb_dlls].nt = header;
            builtin_dlls[nb_dlls].filename = filename;
            nb_dlls++;
        }
    }
}


/***********************************************************************
 *           wine_dll_set_callback
 *
 * Set the callback function for dll loading, and call it
 * for all dlls that were implicitly loaded already.
 */
void wine_dll_set_callback( load_dll_callback_t load )
{
    int i;
    load_dll_callback = load;
    for (i = 0; i < nb_dlls; i++)
    {
        const IMAGE_NT_HEADERS *nt = builtin_dlls[i].nt;
        if (!nt) continue;
        builtin_dlls[i].nt = NULL;
        load_dll_callback( nt, builtin_dlls[i].filename );
    }
    nb_dlls = 0;
    if (main_exe) load_dll_callback( main_exe, "" );
}


/***********************************************************************
 *           wine_dll_load
 *
 * Load a builtin dll.
 */
void *wine_dll_load( const char *filename )
{
    int i;

    /* callback must have been set already */
    assert( load_dll_callback );

    /* check if we have it in the list */
    /* this can happen when initializing pre-loaded dlls in wine_dll_set_callback */
    for (i = 0; i < nb_dlls; i++)
    {
        if (!builtin_dlls[i].nt) continue;
        if (!strcasecmp( builtin_dlls[i].filename, filename ))
        {
            const IMAGE_NT_HEADERS *nt = builtin_dlls[i].nt;
            builtin_dlls[i].nt = NULL;
            load_dll_callback( nt, builtin_dlls[i].filename );
            return (void *)1;
        }
    }
    return dlopen_dll( filename );
}


/***********************************************************************
 *           wine_dll_unload
 *
 * Unload a builtin dll.
 */
void wine_dll_unload( void *handle )
{
#ifdef HAVE_DL_API
    if (handle != (void *)1) dlclose( handle );
#endif
}
