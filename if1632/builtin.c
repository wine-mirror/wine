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
    const WIN16_DESCRIPTOR *descr;     /* DLL descriptor */
    int                     flags;     /* flags (see below) */
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

/* Table of all built-in DLLs */

static BUILTIN16_DLL BuiltinDLLs[] =
{
    { &KERNEL_Descriptor,   0 },
    { &USER_Descriptor,     0 },
    { &GDI_Descriptor,      0 },
    { &SYSTEM_Descriptor,   DLL_FLAG_ALWAYS_USED },
    { &DISPLAY_Descriptor,  DLL_FLAG_ALWAYS_USED },
    { &WPROCS_Descriptor,   DLL_FLAG_ALWAYS_USED },
    { &WINDEBUG_Descriptor, DLL_FLAG_NOT_USED },
    { &AVIFILE_Descriptor,  DLL_FLAG_NOT_USED },
    { &COMMDLG_Descriptor,  DLL_FLAG_NOT_USED },
    { &COMPOBJ_Descriptor,  DLL_FLAG_NOT_USED },
    { &DDEML_Descriptor,    DLL_FLAG_NOT_USED },
    { &DISPDIB_Descriptor,  0 },
    { &KEYBOARD_Descriptor, 0 },
    { &COMM_Descriptor,     0 },
    { &LZEXPAND_Descriptor, 0 },
    { &MMSYSTEM_Descriptor, 0 },
    { &MOUSE_Descriptor,    0 },
    { &MSACM_Descriptor,    0 },
    { &MSVIDEO_Descriptor,  0 },
    { &OLE2CONV_Descriptor, DLL_FLAG_NOT_USED },
    { &OLE2DISP_Descriptor, DLL_FLAG_NOT_USED },
    { &OLE2NLS_Descriptor,  DLL_FLAG_NOT_USED },
    { &OLE2PROX_Descriptor, DLL_FLAG_NOT_USED },
    { &OLE2THK_Descriptor,  DLL_FLAG_NOT_USED },
    { &OLE2_Descriptor,     DLL_FLAG_NOT_USED },
    { &OLECLI_Descriptor,   DLL_FLAG_NOT_USED },
    { &OLESVR_Descriptor,   DLL_FLAG_NOT_USED },
    { &RASAPI16_Descriptor, 0 },
    { &SHELL_Descriptor,    0 },
    { &SOUND_Descriptor,    0 },
    { &STORAGE_Descriptor,  DLL_FLAG_NOT_USED },
    { &STRESS_Descriptor,   0 },
    { &TOOLHELP_Descriptor, 0 },
    { &TYPELIB_Descriptor,  DLL_FLAG_NOT_USED },
    { &VER_Descriptor,      0 },
    { &W32SYS_Descriptor,   DLL_FLAG_NOT_USED },
    { &WIN32S16_Descriptor, DLL_FLAG_NOT_USED },
    { &WIN87EM_Descriptor,  DLL_FLAG_NOT_USED },
    { &WINASPI_Descriptor,  0 },
    { &WINEPS_Descriptor,   DLL_FLAG_ALWAYS_USED },
    { &WING_Descriptor,     0 },
    { &WINSOCK_Descriptor,  0 },
    /* Last entry */
    { NULL, 0 }
};

  /* Ordinal number for interrupt 0 handler in WPROCS.DLL */
#define FIRST_INTERRUPT_ORDINAL 100


/***********************************************************************
 *           BUILTIN_DoLoadModule16
 *
 * Load a built-in Win16 module. Helper function for BUILTIN_LoadModule
 * and BUILTIN_Init.
 */
static HMODULE16 BUILTIN_DoLoadModule16( const WIN16_DESCRIPTOR *descr )
{
    NE_MODULE *pModule;
    int minsize;
    SEGTABLEENTRY *pSegTable;

    HMODULE16 hModule = GLOBAL_CreateBlock( GMEM_MOVEABLE, descr->module_start,
                                            descr->module_size, 0,
                                            FALSE, FALSE, FALSE, NULL );
    if (!hModule) return 0;
    FarSetOwner16( hModule, hModule );

    TRACE("Built-in %s: hmodule=%04x\n",
                    descr->name, hModule );
    pModule = (NE_MODULE *)GlobalLock16( hModule );
    pModule->self = hModule;

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
            if (!BUILTIN_DoLoadModule16( dll->descr )) return FALSE;
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
    if ((p = strrchr( dllname, '.' ))) *p = '\0';

    for (table = BuiltinDLLs; table->descr; table++)
        if (!lstrcmpiA( table->descr->name, dllname )) break;
    if (!table->descr) return (HMODULE16)2;
    if ((table->flags & DLL_FLAG_NOT_USED) && !force) return (HMODULE16)2;

    return BUILTIN_DoLoadModule16( table->descr );
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

