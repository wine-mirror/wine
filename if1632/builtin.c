/*
 * Built-in modules
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <assert.h>
#include <ctype.h>
#include <string.h>
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

DEFAULT_DEBUG_CHANNEL(module)

typedef struct
{
    LPVOID  res_start;          /* address of resource data */
    DWORD   nr_res;
    DWORD   res_size;           /* size of resource data */
} BUILTIN16_RESOURCE;

typedef struct
{
    const WIN16_DESCRIPTOR *descr;     /* DLL descriptor */
    int                     flags;     /* flags (see below) */
    const BUILTIN16_RESOURCE *res;     /* resource descriptor */
} BUILTIN16_DLL;

/* DLL flags */
#define DLL_FLAG_NOT_USED    0x01  /* Use original Windows DLL if possible */
#define DLL_FLAG_ALWAYS_USED 0x02  /* Always use built-in DLL */

/* 16-bit DLLs */

extern const WIN16_DESCRIPTOR AVIFILE_Descriptor;
extern const WIN16_DESCRIPTOR COMM_Descriptor;
extern const WIN16_DESCRIPTOR COMMDLG_Descriptor;
extern const WIN16_DESCRIPTOR COMPOBJ_Descriptor;
extern const WIN16_DESCRIPTOR DDEML_Descriptor;
extern const WIN16_DESCRIPTOR DISPDIB_Descriptor;
extern const WIN16_DESCRIPTOR DISPLAY_Descriptor;
extern const WIN16_DESCRIPTOR GDI_Descriptor;
extern const WIN16_DESCRIPTOR KERNEL_Descriptor;
extern const WIN16_DESCRIPTOR KEYBOARD_Descriptor;
extern const WIN16_DESCRIPTOR LZEXPAND_Descriptor;
extern const WIN16_DESCRIPTOR MMSYSTEM_Descriptor;
extern const WIN16_DESCRIPTOR MOUSE_Descriptor;
extern const WIN16_DESCRIPTOR MSACM_Descriptor;
extern const WIN16_DESCRIPTOR MSVIDEO_Descriptor;
extern const WIN16_DESCRIPTOR OLE2CONV_Descriptor;
extern const WIN16_DESCRIPTOR OLE2DISP_Descriptor;
extern const WIN16_DESCRIPTOR OLE2NLS_Descriptor;
extern const WIN16_DESCRIPTOR OLE2PROX_Descriptor;
extern const WIN16_DESCRIPTOR OLE2THK_Descriptor;
extern const WIN16_DESCRIPTOR OLE2_Descriptor;
extern const WIN16_DESCRIPTOR OLECLI_Descriptor;
extern const WIN16_DESCRIPTOR OLESVR_Descriptor;
extern const WIN16_DESCRIPTOR RASAPI16_Descriptor;
extern const WIN16_DESCRIPTOR SHELL_Descriptor;
extern const WIN16_DESCRIPTOR SOUND_Descriptor;
extern const WIN16_DESCRIPTOR STORAGE_Descriptor;
extern const WIN16_DESCRIPTOR STRESS_Descriptor;
extern const WIN16_DESCRIPTOR SYSTEM_Descriptor;
extern const WIN16_DESCRIPTOR TOOLHELP_Descriptor;
extern const WIN16_DESCRIPTOR TYPELIB_Descriptor;
extern const WIN16_DESCRIPTOR USER_Descriptor;
extern const WIN16_DESCRIPTOR VER_Descriptor;
extern const WIN16_DESCRIPTOR W32SYS_Descriptor;
extern const WIN16_DESCRIPTOR WIN32S16_Descriptor;
extern const WIN16_DESCRIPTOR WIN87EM_Descriptor;
extern const WIN16_DESCRIPTOR WINASPI_Descriptor;
extern const WIN16_DESCRIPTOR WINDEBUG_Descriptor;
extern const WIN16_DESCRIPTOR WINEPS_Descriptor;
extern const WIN16_DESCRIPTOR WING_Descriptor;
extern const WIN16_DESCRIPTOR WINSOCK_Descriptor;
extern const WIN16_DESCRIPTOR WPROCS_Descriptor;

extern const BUILTIN16_RESOURCE display_ResourceDescriptor;
extern const BUILTIN16_RESOURCE mouse_ResourceDescriptor;

/* Table of all built-in DLLs */

static BUILTIN16_DLL BuiltinDLLs[] =
{
    { &KERNEL_Descriptor,   0, NULL },
    { &USER_Descriptor,     0, NULL },
    { &GDI_Descriptor,      0, NULL },
    { &SYSTEM_Descriptor,   DLL_FLAG_ALWAYS_USED, NULL },
    { &DISPLAY_Descriptor,  DLL_FLAG_ALWAYS_USED, &display_ResourceDescriptor },
    { &WPROCS_Descriptor,   DLL_FLAG_ALWAYS_USED, NULL },
    { &WINDEBUG_Descriptor, DLL_FLAG_NOT_USED, NULL },
    { &AVIFILE_Descriptor,  DLL_FLAG_NOT_USED, NULL },
    { &COMMDLG_Descriptor,  DLL_FLAG_NOT_USED, NULL },
    { &COMPOBJ_Descriptor,  DLL_FLAG_NOT_USED, NULL },
    { &DDEML_Descriptor,    DLL_FLAG_NOT_USED, NULL },
    { &DISPDIB_Descriptor,  0, NULL },
    { &KEYBOARD_Descriptor, 0, NULL },
    { &COMM_Descriptor,     0, NULL },
    { &LZEXPAND_Descriptor, 0, NULL },
    { &MMSYSTEM_Descriptor, 0, NULL },
    { &MOUSE_Descriptor,    0, &mouse_ResourceDescriptor },
    { &MSACM_Descriptor,    0, NULL },
    { &MSVIDEO_Descriptor,  0, NULL },
    { &OLE2CONV_Descriptor, DLL_FLAG_NOT_USED, NULL },
    { &OLE2DISP_Descriptor, DLL_FLAG_NOT_USED, NULL },
    { &OLE2NLS_Descriptor,  DLL_FLAG_NOT_USED, NULL },
    { &OLE2PROX_Descriptor, DLL_FLAG_NOT_USED, NULL },
    { &OLE2THK_Descriptor,  DLL_FLAG_NOT_USED, NULL },
    { &OLE2_Descriptor,     DLL_FLAG_NOT_USED, NULL },
    { &OLECLI_Descriptor,   DLL_FLAG_NOT_USED, NULL },
    { &OLESVR_Descriptor,   DLL_FLAG_NOT_USED, NULL },
    { &RASAPI16_Descriptor, 0, NULL },
    { &SHELL_Descriptor,    0, NULL },
    { &SOUND_Descriptor,    0, NULL },
    { &STORAGE_Descriptor,  DLL_FLAG_NOT_USED, NULL },
    { &STRESS_Descriptor,   0, NULL },
    { &TOOLHELP_Descriptor, 0, NULL },
    { &TYPELIB_Descriptor,  DLL_FLAG_NOT_USED, NULL },
    { &VER_Descriptor,      0, NULL },
    { &W32SYS_Descriptor,   DLL_FLAG_NOT_USED, NULL },
    { &WIN32S16_Descriptor, DLL_FLAG_NOT_USED, NULL },
    { &WIN87EM_Descriptor,  DLL_FLAG_NOT_USED, NULL },
    { &WINASPI_Descriptor,  0, NULL },
    { &WINEPS_Descriptor,   DLL_FLAG_ALWAYS_USED, NULL },
    { &WING_Descriptor,     0, NULL },
    { &WINSOCK_Descriptor,  0, NULL },
    /* Last entry */
    { NULL, 0, NULL }
};

  /* Ordinal number for interrupt 0 handler in WPROCS.DLL */
#define FIRST_INTERRUPT_ORDINAL 100


/***********************************************************************
 *           BUILTIN_DoLoadModule16
 *
 * Load a built-in Win16 module. Helper function for BUILTIN_LoadModule
 * and BUILTIN_Init.
 */
static HMODULE16 BUILTIN_DoLoadModule16( const BUILTIN16_DLL *dll )
{
    NE_MODULE *pModule;
    int minsize, res_off;
    SEGTABLEENTRY *pSegTable;
    HMODULE16 hModule;

    if ( !dll->res )
    {
        hModule = GLOBAL_CreateBlock( GMEM_MOVEABLE, dll->descr->module_start,
                                      dll->descr->module_size, 0,
                                            FALSE, FALSE, FALSE, NULL );
    if (!hModule) return 0;
    FarSetOwner16( hModule, hModule );

    pModule = (NE_MODULE *)GlobalLock16( hModule );
    }
    else
    {
        ET_BUNDLE *bundle;

        hModule = GLOBAL_Alloc( GMEM_MOVEABLE, 
                                dll->descr->module_size + dll->res->res_size, 
                                0, FALSE, FALSE, FALSE );
        if (!hModule) return 0;
        FarSetOwner16( hModule, hModule );

        pModule = (NE_MODULE *)GlobalLock16( hModule );
        res_off = ((NE_MODULE *)dll->descr->module_start)->res_table;

        memcpy( (LPBYTE)pModule, dll->descr->module_start, res_off );
        memcpy( (LPBYTE)pModule + res_off, dll->res->res_start, dll->res->res_size );
        memcpy( (LPBYTE)pModule + res_off + dll->res->res_size, 
                dll->descr->module_start + res_off, dll->descr->module_size - res_off );

        /* Have to fix up various pModule-based near pointers.  Ugh! */
        pModule->name_table   += dll->res->res_size;
        pModule->modref_table += dll->res->res_size;
        pModule->import_table += dll->res->res_size;
        pModule->entry_table  += dll->res->res_size;

        for ( bundle = (ET_BUNDLE *)((LPBYTE)pModule + pModule->entry_table);
              bundle->next;
              bundle = (ET_BUNDLE *)((LPBYTE)pModule + bundle->next) )
            bundle->next += dll->res->res_size;

        /* NOTE: (Ab)use the hRsrcMap parameter for resource data pointer */
        pModule->hRsrcMap = dll->res->res_start;
    }
    pModule->self = hModule;

    TRACE( "Built-in %s: hmodule=%04x\n", dll->descr->name, hModule );

    /* Allocate the code segment */

    pSegTable = NE_SEG_TABLE( pModule );
    pSegTable->hSeg = GLOBAL_CreateBlock( GMEM_FIXED, dll->descr->code_start,
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
                                    dll->descr->data_start, pSegTable->minsize);
    if (pModule->heap_size)
        LocalInit16( GlobalHandleToSel16(pSegTable->hSeg),
		pSegTable->minsize, minsize );

	if (dll->res)
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
    BUILTIN16_DLL *dll;
    WORD vector;
    HMODULE16 hModule;

    for (dll = BuiltinDLLs; dll->descr; dll++)
    {
        if (dll->flags & DLL_FLAG_ALWAYS_USED)
            if (!BUILTIN_DoLoadModule16( dll )) return FALSE;
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
 * Load a built-in module. If the 'force' parameter is FALSE, we only
 * load the module if it has not been disabled via the -dll option.
 */
HMODULE16 BUILTIN_LoadModule( LPCSTR name, BOOL force )
{
    BUILTIN16_DLL *table;
    char dllname[16], *p;

    /* Fix the name in case we have a full path and extension */

    if ((p = strrchr( name, '\\' ))) name = p + 1;
    lstrcpynA( dllname, name, sizeof(dllname) );
    p = strrchr( dllname, '.' );
	 
    if (!p) strcat( dllname, ".dll" );

    for (table = BuiltinDLLs; table->descr; table++)
    {
       NE_MODULE *pModule = (NE_MODULE *)table->descr->module_start;
       OFSTRUCT *pOfs = (OFSTRUCT *)((LPBYTE)pModule + pModule->fileinfo);
       if (!lstrcmpiA( pOfs->szPathName, dllname )) break;
    }

    if (!table->descr) return (HMODULE16)2;

    if ((table->flags & DLL_FLAG_NOT_USED) && !force) return (HMODULE16)2;

    return BUILTIN_DoLoadModule16( table );
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

