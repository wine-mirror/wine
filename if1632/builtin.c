/*
 * Built-in modules
 *
 * Copyright 1996 Alexandre Julliard
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

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "winbase.h"
#include "wine/winbase16.h"
#include "builtin16.h"
#include "global.h"
#include "file.h"
#include "module.h"
#include "miscemu.h"
#include "stackframe.h"
#include "wine/library.h"
#include "wine/debug.h"
#include "toolhelp.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);

typedef struct
{
    void       *module_start;      /* 32-bit address of the module data */
    int         module_size;       /* Size of the module data */
    void       *code_start;        /* 32-bit address of DLL code */
    void       *data_start;        /* 32-bit address of DLL data */
    const char *owner;             /* 32-bit dll that contains this dll */
    const void *rsrc;              /* resources data */
} BUILTIN16_DESCRIPTOR;

/* Table of all built-in DLLs */

#define MAX_DLLS 50

static const BUILTIN16_DESCRIPTOR *builtin_dlls[MAX_DLLS];


/* patch all the flat cs references of the code segment if necessary */
inline static void patch_code_segment( void *code_segment )
{
#ifdef __i386__
    CALLFROM16 *call = code_segment;
    if (call->flatcs == wine_get_cs()) return;  /* nothing to patch */
    while (call->pushl == 0x68)
    {
        call->flatcs = wine_get_cs();
        call++;
    }
#endif
}


/***********************************************************************
 *           BUILTIN_DoLoadModule16
 *
 * Load a built-in Win16 module. Helper function for BUILTIN_LoadModule.
 */
static HMODULE16 BUILTIN_DoLoadModule16( const BUILTIN16_DESCRIPTOR *descr )
{
    NE_MODULE *pModule;
    int minsize;
    SEGTABLEENTRY *pSegTable;
    HMODULE16 hModule;

    hModule = GLOBAL_CreateBlock( GMEM_MOVEABLE, descr->module_start,
                                  descr->module_size, 0, WINE_LDT_FLAGS_DATA );
    if (!hModule) return 0;
    FarSetOwner16( hModule, hModule );

    pModule = (NE_MODULE *)GlobalLock16( hModule );
    pModule->self = hModule;
    /* NOTE: (Ab)use the hRsrcMap parameter for resource data pointer */
    pModule->hRsrcMap = (void *)descr->rsrc;

    /* Allocate the code segment */

    pSegTable = NE_SEG_TABLE( pModule );
    pSegTable->hSeg = GLOBAL_CreateBlock( GMEM_FIXED, descr->code_start,
                                          pSegTable->minsize, hModule,
                                          WINE_LDT_FLAGS_CODE|WINE_LDT_FLAGS_32BIT );
    if (!pSegTable->hSeg) return 0;
    patch_code_segment( descr->code_start );
    pSegTable++;

    /* Allocate the data segment */

    minsize = pSegTable->minsize ? pSegTable->minsize : 0x10000;
    minsize += pModule->heap_size;
    if (minsize > 0x10000) minsize = 0x10000;
    pSegTable->hSeg = GlobalAlloc16( GMEM_FIXED, minsize );
    if (!pSegTable->hSeg) return 0;
    FarSetOwner16( pSegTable->hSeg, hModule );
    if (pSegTable->minsize) memcpy( GlobalLock16( pSegTable->hSeg ),
                                    descr->data_start, pSegTable->minsize);
    if (pModule->heap_size)
        LocalInit16( GlobalHandleToSel16(pSegTable->hSeg),
		pSegTable->minsize, minsize );

    if (descr->rsrc) NE_InitResourceHandler(pModule);

    NE_RegisterModule( pModule );

    /* make sure the 32-bit library containing this one is loaded too */
    LoadLibraryA( descr->owner );

    return hModule;
}


/***********************************************************************
 *           find_dll_descr
 *
 * Find a descriptor in the list
 */
static const BUILTIN16_DESCRIPTOR *find_dll_descr( const char *dllname )
{
    int i;
    for (i = 0; i < MAX_DLLS; i++)
    {
        const BUILTIN16_DESCRIPTOR *descr = builtin_dlls[i];
        if (descr)
        {
            NE_MODULE *pModule = (NE_MODULE *)descr->module_start;
            OFSTRUCT *pOfs = (OFSTRUCT *)((LPBYTE)pModule + pModule->fileinfo);
            BYTE *name_table = (BYTE *)pModule + pModule->name_table;

            /* check the dll file name */
            if (!FILE_strcasecmp( pOfs->szPathName, dllname )) return descr;
            /* check the dll module name (without extension) */
            if (!FILE_strncasecmp( dllname, name_table+1, *name_table ) &&
                !strcmp( dllname + *name_table, ".dll" ))
                return descr;
        }
    }
    return NULL;
}


/***********************************************************************
 *           BUILTIN_IsPresent
 *
 * Check if a builtin dll descriptor is present (because we loaded its 32-bit counterpart).
 */
BOOL BUILTIN_IsPresent( LPCSTR name )
{
    char dllname[20], *p;

    if (strlen(name) >= sizeof(dllname)-4) return FALSE;
    strcpy( dllname, name );
    p = strrchr( dllname, '.' );
    if (!p) strcat( dllname, ".dll" );
    for (p = dllname; *p; p++) *p = FILE_tolower(*p);

    return (find_dll_descr( dllname ) != NULL);
}


/***********************************************************************
 *           BUILTIN_LoadModule
 *
 * Load a built-in module.
 */
HMODULE16 BUILTIN_LoadModule( LPCSTR name )
{
    const BUILTIN16_DESCRIPTOR *descr;
    char error[256], dllname[20], *p;
    int file_exists;
    void *handle;

    /* Fix the name in case we have a full path and extension */

    if ((p = strrchr( name, '\\' ))) name = p + 1;
    if ((p = strrchr( name, '/' ))) name = p + 1;

    if (strlen(name) >= sizeof(dllname)-4) return (HMODULE16)2;

    strcpy( dllname, name );
    p = strrchr( dllname, '.' );
    if (!p) strcat( dllname, ".dll" );
    for (p = dllname; *p; p++) *p = FILE_tolower(*p);

    if ((descr = find_dll_descr( dllname )))
        return BUILTIN_DoLoadModule16( descr );

    if ((handle = wine_dll_load( dllname, error, sizeof(error), &file_exists )))
    {
        if ((descr = find_dll_descr( dllname )))
            return BUILTIN_DoLoadModule16( descr );

        ERR( "loaded .so but dll %s still not found\n", dllname );
    }
    else
    {
        if (!file_exists) WARN("cannot open .so lib for 16-bit builtin %s: %s\n", name, error);
        else ERR("failed to load .so lib for 16-bit builtin %s: %s\n", name, error );
    }
    return (HMODULE16)2;
}


/***********************************************************************
 *           __wine_register_dll_16 (KERNEL32.@)
 *
 * Register a built-in DLL descriptor.
 */
void __wine_register_dll_16( const BUILTIN16_DESCRIPTOR *descr )
{
    int i;

    for (i = 0; i < MAX_DLLS; i++)
    {
        if (builtin_dlls[i]) continue;
        builtin_dlls[i] = descr;
        break;
    }
    assert( i < MAX_DLLS );
}


/***********************************************************************
 *           __wine_unregister_dll_16 (KERNEL32.@)
 *
 * Unregister a built-in DLL descriptor.
 */
void __wine_unregister_dll_16( const BUILTIN16_DESCRIPTOR *descr )
{
    int i;

    for (i = 0; i < MAX_DLLS; i++)
    {
        if (builtin_dlls[i] != descr) continue;
        builtin_dlls[i] = NULL;
        break;
    }
}
