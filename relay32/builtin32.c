/*
 * Win32 builtin functions
 *
 * Copyright 1997 Alexandre Julliard
 */

#include "config.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#ifdef HAVE_DL_API
#include <dlfcn.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "windef.h"
#include "wine/winbase16.h"
#include "wine/library.h"
#include "global.h"
#include "neexe.h"
#include "module.h"
#include "heap.h"
#include "main.h"
#include "winerror.h"
#include "server.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(module);
DECLARE_DEBUG_CHANNEL(relay);

extern void RELAY_SetupDLL( const char *module );

static HMODULE main_module;

/***********************************************************************
 *           BUILTIN32_dlopen
 */
void *BUILTIN32_dlopen( const char *name )
{
#ifdef HAVE_DL_API
    void *handle;

    if (!(handle = wine_dll_load( name )))
    {
	LPSTR pErr, p;
	pErr = dlerror();
	p = strchr(pErr, ':');
	if ((p) && 
	   (!strncmp(p, ": undefined symbol", 18))) /* undef symbol -> ERR() */
	    ERR("failed to load %s: %s\n", name, pErr);
	else /* WARN() for libraries that are supposed to be native */
            WARN("failed to load %s: %s\n", name, pErr );
    }
    return handle;
#else
    return NULL;
#endif
}

/***********************************************************************
 *           BUILTIN32_dlclose
 */
int BUILTIN32_dlclose( void *handle )
{
#ifdef HAVE_DL_API
    /* FIXME: should unregister descriptors first */
    /* return dlclose( handle ); */
#endif
    return 0;
}


/***********************************************************************
 *           load_library
 *
 * Load a library in memory; callback function for wine_dll_register
 */
static void load_library( void *base, const char *filename )
{
    HMODULE module = (HMODULE)base;
    WINE_MODREF *wm;

    if (!base)
    {
        ERR("could not map image for %s\n", filename ? filename : "main exe" );
        return;
    }

    if (!(PE_HEADER(module)->FileHeader.Characteristics & IMAGE_FILE_DLL))
    {
        /* if we already have an executable, ignore this one */
        if (!main_module) main_module = module;
        return; /* don't create the modref here, will be done later on */
    }

    if (GetModuleHandleA( filename ))
        MESSAGE( "Warning: loading builtin %s, but native version already present. Expect trouble.\n", filename );

    /* Create 32-bit MODREF */
    if (!(wm = PE_CreateModule( module, filename, 0, -1, TRUE )))
    {
        ERR( "can't load %s\n", filename );
        SetLastError( ERROR_OUTOFMEMORY );
        return;
    }
    TRACE( "loaded %s %p %x\n", filename, wm, module );
    wm->refCount++;  /* we don't support freeing builtin dlls (FIXME)*/

    /* setup relay debugging entry points */
    if (TRACE_ON(relay)) RELAY_SetupDLL( (void *)module );
}


/***********************************************************************
 *           BUILTIN32_LoadLibraryExA
 *
 * Partly copied from the original PE_ version.
 *
 */
WINE_MODREF *BUILTIN32_LoadLibraryExA(LPCSTR path, DWORD flags)
{
    WINE_MODREF   *wm;
    char dllname[20], *p;
    LPCSTR name;
    void *handle;

    /* Fix the name in case we have a full path and extension */
    name = path;
    if ((p = strrchr( name, '\\' ))) name = p + 1;
    if ((p = strrchr( name, '/' ))) name = p + 1;

    if (strlen(name) >= sizeof(dllname)-4) goto error;

    strcpy( dllname, name );
    p = strrchr( dllname, '.' );
    if (!p) strcat( dllname, ".dll" );

    if (!(handle = BUILTIN32_dlopen( dllname ))) goto error;

    if (!(wm = MODULE_FindModule( path ))) wm = MODULE_FindModule( dllname );
    if (!wm)
    {
        ERR( "loaded .so but dll %s still not found\n", dllname );
        /* wine_dll_unload( handle );*/
        return NULL;
    }
    wm->dlhandle = handle;
    return wm;

 error:
    SetLastError( ERROR_FILE_NOT_FOUND );
    return NULL;
}

/***********************************************************************
 *           BUILTIN32_Init
 *
 * Initialize loading callbacks and return HMODULE of main exe.
 * 'main' is the main exe in case if was already loaded from a PE file.
 */
HMODULE BUILTIN32_LoadExeModule( HMODULE main )
{
    main_module = main;
    wine_dll_set_callback( load_library );
    if (!main_module)
        MESSAGE( "No built-in EXE module loaded!  Did you create a .spec file?\n" );
    return main_module;
}


/***********************************************************************
 *           BUILTIN32_RegisterDLL
 *
 * Register a built-in DLL descriptor.
 */
void BUILTIN32_RegisterDLL( const IMAGE_NT_HEADERS *header, const char *filename )
{
    extern void __wine_dll_register( const IMAGE_NT_HEADERS *header, const char *filename );
    __wine_dll_register( header, filename );
}
