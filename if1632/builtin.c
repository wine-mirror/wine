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
#include "wine/winestring.h"
#include "builtin16.h"
#include "builtin32.h"
#include "global.h"
#include "heap.h"
#include "module.h"
#include "miscemu.h"
#include "neexe.h"
#include "stackframe.h"
#include "user.h"
#include "process.h"
#include "task.h"
#include "debugtools.h"
#include "toolhelp.h"

DEFAULT_DEBUG_CHANNEL(module);

typedef struct
{
    LPVOID  res_start;          /* address of resource data */
    DWORD   nr_res;
    DWORD   res_size;           /* size of resource data */
} BUILTIN16_RESOURCE;


/* Table of all built-in DLLs */

#define MAX_DLLS 50

static const BUILTIN16_DESCRIPTOR *builtin_dlls[MAX_DLLS];
static int nb_dlls;

/* list of DLLs that should always be loaded at startup */
static const char * const always_load[] =
{
    "system", "display", "wprocs", "wineps", NULL
};

  /* Ordinal number for interrupt 0 handler in WPROCS.DLL */
#define FIRST_INTERRUPT_ORDINAL 100


/***********************************************************************
 *           BUILTIN_DoLoadModule16
 *
 * Load a built-in Win16 module. Helper function for BUILTIN_LoadModule
 * and BUILTIN_Init.
 */
static HMODULE16 BUILTIN_DoLoadModule16( const BUILTIN16_DESCRIPTOR *descr )
{
    NE_MODULE *pModule;
    int minsize, res_off;
    SEGTABLEENTRY *pSegTable;
    HMODULE16 hModule;
    const BUILTIN16_RESOURCE *rsrc = descr->rsrc;

    if (!rsrc)
    {
        hModule = GLOBAL_CreateBlock( GMEM_MOVEABLE, descr->module_start,
                                      descr->module_size, 0,
                                            FALSE, FALSE, FALSE, NULL );
        if (!hModule) return 0;
        FarSetOwner16( hModule, hModule );

        pModule = (NE_MODULE *)GlobalLock16( hModule );
    }
    else
    {
        ET_BUNDLE *bundle;

        hModule = GLOBAL_Alloc( GMEM_MOVEABLE, 
                                descr->module_size + rsrc->res_size, 
                                0, FALSE, FALSE, FALSE );
        if (!hModule) return 0;
        FarSetOwner16( hModule, hModule );

        pModule = (NE_MODULE *)GlobalLock16( hModule );
        res_off = ((NE_MODULE *)descr->module_start)->res_table;

        memcpy( (LPBYTE)pModule, descr->module_start, res_off );
        memcpy( (LPBYTE)pModule + res_off, rsrc->res_start, rsrc->res_size );
        memcpy( (LPBYTE)pModule + res_off + rsrc->res_size, 
                (LPBYTE)descr->module_start + res_off, descr->module_size - res_off );

        /* Have to fix up various pModule-based near pointers.  Ugh! */
        pModule->name_table   += rsrc->res_size;
        pModule->modref_table += rsrc->res_size;
        pModule->import_table += rsrc->res_size;
        pModule->entry_table  += rsrc->res_size;

        for ( bundle = (ET_BUNDLE *)((LPBYTE)pModule + pModule->entry_table);
              bundle->next;
              bundle = (ET_BUNDLE *)((LPBYTE)pModule + bundle->next) )
            bundle->next += rsrc->res_size;

        /* NOTE: (Ab)use the hRsrcMap parameter for resource data pointer */
        pModule->hRsrcMap = rsrc->res_start;
    }
    pModule->self = hModule;

    TRACE( "Built-in %s: hmodule=%04x\n", descr->name, hModule );

    /* Allocate the code segment */

    pSegTable = NE_SEG_TABLE( pModule );
    pSegTable->hSeg = GLOBAL_CreateBlock( GMEM_FIXED, descr->code_start,
                                              pSegTable->minsize, hModule,
                                              TRUE, TRUE, FALSE, NULL );
    if (!pSegTable->hSeg) return 0;
    pSegTable++;

    /* Allocate the data segment */

    minsize = pSegTable->minsize ? pSegTable->minsize : 0x10000;
    minsize += pModule->heap_size;
    if (minsize > 0x10000) minsize = 0x10000;
    pSegTable->hSeg = GLOBAL_Alloc( GMEM_FIXED, minsize,
                                        hModule, FALSE, FALSE, FALSE );
    if (!pSegTable->hSeg) return 0;
    if (pSegTable->minsize) memcpy( GlobalLock16( pSegTable->hSeg ),
                                    descr->data_start, pSegTable->minsize);
    if (pModule->heap_size)
        LocalInit16( GlobalHandleToSel16(pSegTable->hSeg),
		pSegTable->minsize, minsize );

	if (rsrc)
		NE_InitResourceHandler(hModule);

    NE_RegisterModule( pModule );
    return hModule;
}


/***********************************************************************
 *           BUILTIN_Init
 *
 * Load all built-in modules marked as 'always used'.
 */
BOOL BUILTIN_Init(void)
{
    WORD vector;
    HMODULE16 hModule;
    const char * const *ptr = always_load;

    while (*ptr)
    {
        if (!BUILTIN_LoadModule( *ptr )) return FALSE;
        ptr++;
    }

    /* Set interrupt vectors from entry points in WPROCS.DLL */

    hModule = GetModuleHandle16( "WPROCS" );
    for (vector = 0; vector < 256; vector++)
    {
        FARPROC16 proc = NE_GetEntryPoint( hModule,
                                           FIRST_INTERRUPT_ORDINAL + vector );
        assert(proc);
        INT_SetPMHandler( vector, proc );
    }

    return TRUE;
}


/***********************************************************************
 *           BUILTIN_LoadModule
 *
 * Load a built-in module.
 */
HMODULE16 BUILTIN_LoadModule( LPCSTR name )
{
    char dllname[16], *p;
    int i;

    /* Fix the name in case we have a full path and extension */

    if ((p = strrchr( name, '\\' ))) name = p + 1;
    lstrcpynA( dllname, name, sizeof(dllname) );
    p = strrchr( dllname, '.' );
	 
    if (!p) strcat( dllname, ".dll" );

    for (i = 0; i < nb_dlls; i++)
    {
        const BUILTIN16_DESCRIPTOR *descr = builtin_dlls[i];
        NE_MODULE *pModule = (NE_MODULE *)descr->module_start;
        OFSTRUCT *pOfs = (OFSTRUCT *)((LPBYTE)pModule + pModule->fileinfo);
        if (!lstrcmpiA( pOfs->szPathName, dllname ))
            return BUILTIN_DoLoadModule16( descr );
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
 *           BUILTIN_RegisterDLL
 *
 * Register a built-in DLL descriptor.
 */
void BUILTIN_RegisterDLL( const BUILTIN16_DESCRIPTOR *descr )
{
    assert( nb_dlls < MAX_DLLS );
    builtin_dlls[nb_dlls++] = descr;
}


/**********************************************************************
 *	    BUILTIN_DefaultIntHandler
 *
 * Default interrupt handler.
 */
void WINAPI BUILTIN_DefaultIntHandler( CONTEXT86 *context )
{
    WORD ordinal;
    char name[80];
    BUILTIN_GetEntryPoint16( CURRENT_STACK16, name, &ordinal );
    INT_BARF( context, ordinal - FIRST_INTERRUPT_ORDINAL );
}
