/*
 * Built-in modules
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "winbase.h"
#include "wine/winbase16.h"
#include "builtin16.h"
#include "global.h"
#include "heap.h"
#include "module.h"
#include "miscemu.h"
#include "stackframe.h"
#include "selectors.h"
#include "task.h"
#include "debugtools.h"
#include "toolhelp.h"

DEFAULT_DEBUG_CHANNEL(module);

/* Table of all built-in DLLs */

#define MAX_DLLS 50

static const BUILTIN16_DESCRIPTOR *builtin_dlls[MAX_DLLS];
static int nb_dlls;


/* patch all the flat cs references of the code segment if necessary */
inline static void patch_code_segment( void *code_segment )
{
#ifdef __i386__
    CALLFROM16 *call = code_segment;
    if (call->flatcs == __get_cs()) return;  /* nothing to patch */
    while (call->pushl == 0x68)
    {
        call->flatcs = __get_cs();
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

    TRACE( "Built-in %s: hmodule=%04x\n", descr->name, hModule );

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
    pSegTable->hSeg = GLOBAL_Alloc( GMEM_FIXED, minsize, hModule, WINE_LDT_FLAGS_DATA );
    if (!pSegTable->hSeg) return 0;
    if (pSegTable->minsize) memcpy( GlobalLock16( pSegTable->hSeg ),
                                    descr->data_start, pSegTable->minsize);
    if (pModule->heap_size)
        LocalInit16( GlobalHandleToSel16(pSegTable->hSeg),
		pSegTable->minsize, minsize );

    if (descr->rsrc) NE_InitResourceHandler(hModule);

    NE_RegisterModule( pModule );
    return hModule;
}


/***********************************************************************
 *           BUILTIN_LoadModule
 *
 * Load a built-in module.
 */
HMODULE16 BUILTIN_LoadModule( LPCSTR name )
{
    char dllname[20], *p;
    void *handle;
    int i;

    /* Fix the name in case we have a full path and extension */

    if ((p = strrchr( name, '\\' ))) name = p + 1;
    if ((p = strrchr( name, '/' ))) name = p + 1;

    if (strlen(name) >= sizeof(dllname)-4) return (HMODULE16)2;

    strcpy( dllname, name );
    p = strrchr( dllname, '.' );
    if (!p) strcat( dllname, ".dll" );

    for (i = 0; i < nb_dlls; i++)
    {
        const BUILTIN16_DESCRIPTOR *descr = builtin_dlls[i];
        NE_MODULE *pModule = (NE_MODULE *)descr->module_start;
        OFSTRUCT *pOfs = (OFSTRUCT *)((LPBYTE)pModule + pModule->fileinfo);
        if (!strcasecmp( pOfs->szPathName, dllname ))
            return BUILTIN_DoLoadModule16( descr );
    }

    if ((handle = BUILTIN32_dlopen( dllname )))
    {
        for (i = 0; i < nb_dlls; i++)
        {
            const BUILTIN16_DESCRIPTOR *descr = builtin_dlls[i];
            NE_MODULE *pModule = (NE_MODULE *)descr->module_start;
            OFSTRUCT *pOfs = (OFSTRUCT *)((LPBYTE)pModule + pModule->fileinfo);
            if (!strcasecmp( pOfs->szPathName, dllname ))
                return BUILTIN_DoLoadModule16( descr );
        }
        ERR( "loaded .so but dll %s still not found\n", dllname );
        BUILTIN32_dlclose( handle );
    }

    return (HMODULE16)2;
}


/***********************************************************************
 *           BUILTIN_GetEntryPoint16
 *
 * Return the ordinal, name, and type info corresponding to a CS:IP address.
 * This is used only by relay debugging.
 */
LPCSTR BUILTIN_GetEntryPoint16( STACK16FRAME *frame, LPSTR name, WORD *pOrd )
{
    WORD i, max_offset;
    register BYTE *p;
    NE_MODULE *pModule;
    ET_BUNDLE *bundle;
    ET_ENTRY *entry;

    if (!(pModule = NE_GetPtr( FarGetOwner16( GlobalHandle16( frame->module_cs ) ))))
        return NULL;

    max_offset = 0;
    *pOrd = 0;
    bundle = (ET_BUNDLE *)((BYTE *)pModule + pModule->entry_table);
    do 
    {
        entry = (ET_ENTRY *)((BYTE *)bundle+6);
	for (i = bundle->first + 1; i <= bundle->last; i++)
        {
	    if ((entry->offs < frame->entry_ip)
	    && (entry->segnum == 1) /* code segment ? */
	    && (entry->offs >= max_offset))
            {
		max_offset = entry->offs;
		*pOrd = i;
            }
	    entry++;
        }
    } while ( (bundle->next)
	   && (bundle = (ET_BUNDLE *)((BYTE *)pModule+bundle->next)));

    /* Search for the name in the resident names table */
    /* (built-in modules have no non-resident table)   */
    
    p = (BYTE *)pModule + pModule->name_table;
    while (*p)
    {
        p += *p + 1 + sizeof(WORD);
        if (*(WORD *)(p + *p + 1) == *pOrd) break;
    }

    sprintf( name, "%.*s.%d: %.*s",
             *((BYTE *)pModule + pModule->name_table),
             (char *)pModule + pModule->name_table + 1,
             *pOrd, *p, (char *)(p + 1) );

    /* Retrieve type info string */
    return *(LPCSTR *)((LPBYTE)PTR_SEG_OFF_TO_LIN( frame->module_cs, frame->callfrom_ip ) + 4);
}


/***********************************************************************
 *           __wine_register_dll_16
 *
 * Register a built-in DLL descriptor.
 */
void __wine_register_dll_16( const BUILTIN16_DESCRIPTOR *descr )
{
    assert( nb_dlls < MAX_DLLS );
    builtin_dlls[nb_dlls++] = descr;
}
