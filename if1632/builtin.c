/*
 * Built-in modules
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "windows.h"
#include "builtin32.h"
#include "gdi.h"
#include "global.h"
#include "heap.h"
#include "module.h"
#include "miscemu.h"
#include "neexe.h"
#include "stackframe.h"
#include "user.h"
#include "process.h"
#include "stddebug.h"
#include "debug.h"

/* Built-in modules descriptors */
/* Don't change these structures! (see tools/build.c) */

typedef struct
{
    const char *name;              /* DLL name */
    void       *module_start;      /* 32-bit address of the module data */
    int         module_size;       /* Size of the module data */
    const BYTE *code_start;        /* 32-bit address of DLL code */
    const BYTE *data_start;        /* 32-bit address of DLL data */
} WIN16_DESCRIPTOR;

typedef struct
{
    const WIN16_DESCRIPTOR *descr;     /* DLL descriptor */
    int                     flags;     /* flags (see below) */
} BUILTIN16_DLL;

/* DLL flags */
#define DLL_FLAG_NOT_USED    0x01  /* Use original Windows DLL if possible */
#define DLL_FLAG_ALWAYS_USED 0x02  /* Always use built-in DLL */

/* 16-bit DLLs */

extern const WIN16_DESCRIPTOR COMMDLG_Descriptor;
extern const WIN16_DESCRIPTOR COMPOBJ_Descriptor;
extern const WIN16_DESCRIPTOR DDEML_Descriptor;
extern const WIN16_DESCRIPTOR GDI_Descriptor;
extern const WIN16_DESCRIPTOR KERNEL_Descriptor;
extern const WIN16_DESCRIPTOR KEYBOARD_Descriptor;
extern const WIN16_DESCRIPTOR LZEXPAND_Descriptor;
extern const WIN16_DESCRIPTOR MMSYSTEM_Descriptor;
extern const WIN16_DESCRIPTOR MOUSE_Descriptor;
extern const WIN16_DESCRIPTOR OLE2CONV_Descriptor;
extern const WIN16_DESCRIPTOR OLE2DISP_Descriptor;
extern const WIN16_DESCRIPTOR OLE2NLS_Descriptor;
extern const WIN16_DESCRIPTOR OLE2PROX_Descriptor;
extern const WIN16_DESCRIPTOR OLE2THK_Descriptor;
extern const WIN16_DESCRIPTOR OLE2_Descriptor;
extern const WIN16_DESCRIPTOR OLECLI_Descriptor;
extern const WIN16_DESCRIPTOR OLESVR_Descriptor;
extern const WIN16_DESCRIPTOR SHELL_Descriptor;
extern const WIN16_DESCRIPTOR SOUND_Descriptor;
extern const WIN16_DESCRIPTOR STORAGE_Descriptor;
extern const WIN16_DESCRIPTOR STRESS_Descriptor;
extern const WIN16_DESCRIPTOR SYSTEM_Descriptor;
extern const WIN16_DESCRIPTOR TOOLHELP_Descriptor;
extern const WIN16_DESCRIPTOR USER_Descriptor;
extern const WIN16_DESCRIPTOR VER_Descriptor;
extern const WIN16_DESCRIPTOR W32SYS_Descriptor;
extern const WIN16_DESCRIPTOR WIN32S16_Descriptor;
extern const WIN16_DESCRIPTOR WIN87EM_Descriptor;
extern const WIN16_DESCRIPTOR WINASPI_Descriptor;
extern const WIN16_DESCRIPTOR WINDEBUG_Descriptor;
extern const WIN16_DESCRIPTOR WING_Descriptor;
extern const WIN16_DESCRIPTOR WINSOCK_Descriptor;
extern const WIN16_DESCRIPTOR WPROCS_Descriptor;

/* Table of all built-in DLLs */

static BUILTIN16_DLL BuiltinDLLs[] =
{
    { &KERNEL_Descriptor,   DLL_FLAG_ALWAYS_USED },
    { &USER_Descriptor,     DLL_FLAG_ALWAYS_USED },
    { &GDI_Descriptor,      DLL_FLAG_ALWAYS_USED },
    { &SYSTEM_Descriptor,   DLL_FLAG_ALWAYS_USED },
    { &WPROCS_Descriptor,   DLL_FLAG_ALWAYS_USED },
    { &WINDEBUG_Descriptor, DLL_FLAG_ALWAYS_USED },
    { &COMMDLG_Descriptor,  DLL_FLAG_NOT_USED },
    { &COMPOBJ_Descriptor,  DLL_FLAG_NOT_USED },
    { &DDEML_Descriptor,    DLL_FLAG_NOT_USED },
    { &KEYBOARD_Descriptor, 0 },
    { &LZEXPAND_Descriptor, 0 },
    { &MMSYSTEM_Descriptor, 0 },
    { &MOUSE_Descriptor,    0 },
    { &OLE2CONV_Descriptor, DLL_FLAG_NOT_USED },
    { &OLE2DISP_Descriptor, DLL_FLAG_NOT_USED },
    { &OLE2NLS_Descriptor,  DLL_FLAG_NOT_USED },
    { &OLE2PROX_Descriptor, DLL_FLAG_NOT_USED },
    { &OLE2THK_Descriptor,  DLL_FLAG_NOT_USED },
    { &OLE2_Descriptor,     DLL_FLAG_NOT_USED },
    { &OLECLI_Descriptor,   DLL_FLAG_NOT_USED },
    { &OLESVR_Descriptor,   DLL_FLAG_NOT_USED },
    { &SHELL_Descriptor,    0 },
    { &SOUND_Descriptor,    0 },
    { &STORAGE_Descriptor,  DLL_FLAG_NOT_USED },
    { &STRESS_Descriptor,   0 },
    { &TOOLHELP_Descriptor, 0 },
    { &VER_Descriptor,      0 },
    { &W32SYS_Descriptor,   0 },
    { &WIN32S16_Descriptor, 0 },
    { &WIN87EM_Descriptor,  DLL_FLAG_NOT_USED },
    { &WINASPI_Descriptor,  0 },
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
    FarSetOwner( hModule, hModule );

    dprintf_module( stddeb, "Built-in %s: hmodule=%04x\n",
                    descr->name, hModule );
    pModule = (NE_MODULE *)GlobalLock16( hModule );
    pModule->self = hModule;

    /* Allocate the code segment */

    pSegTable = NE_SEG_TABLE( pModule );
    pSegTable->selector = GLOBAL_CreateBlock( GMEM_FIXED, descr->code_start,
                                              pSegTable->minsize, hModule,
                                              TRUE, TRUE, FALSE, NULL );
    if (!pSegTable->selector) return 0;
    pSegTable++;

    /* Allocate the data segment */

    minsize = pSegTable->minsize ? pSegTable->minsize : 0x10000;
    minsize += pModule->heap_size;
    if (minsize > 0x10000) minsize = 0x10000;
    pSegTable->selector = GLOBAL_Alloc( GMEM_FIXED, minsize,
                                        hModule, FALSE, FALSE, FALSE );
    if (!pSegTable->selector) return 0;
    if (pSegTable->minsize) memcpy( GlobalLock16( pSegTable->selector ),
                                    descr->data_start, pSegTable->minsize);
    if (pModule->heap_size)
        LocalInit( pSegTable->selector, pSegTable->minsize, minsize );

    MODULE_RegisterModule( pModule );
    return hModule;
}


/***********************************************************************
 *           BUILTIN_Init
 *
 * Load all built-in modules marked as 'always used'.
 */
BOOL32 BUILTIN_Init(void)
{
    BUILTIN16_DLL *dll;
    NE_MODULE *pModule;
    WORD vector;
    HMODULE16 hModule;

    for (dll = BuiltinDLLs; dll->descr; dll++)
    {
        if (dll->flags & DLL_FLAG_ALWAYS_USED)
            if (!BUILTIN_DoLoadModule16( dll->descr )) return FALSE;
    }

    /* Set the USER and GDI heap selectors */

    pModule      = MODULE_GetPtr( GetModuleHandle16( "USER" ));
    USER_HeapSel = (NE_SEG_TABLE( pModule ) + pModule->dgroup - 1)->selector;
    pModule      = MODULE_GetPtr( GetModuleHandle16( "GDI" ));
    GDI_HeapSel  = (NE_SEG_TABLE( pModule ) + pModule->dgroup - 1)->selector;

    /* Initialize KERNEL.178 (__WINFLAGS) with the correct flags value */

    hModule = GetModuleHandle16( "KERNEL" );
    MODULE_SetEntryPoint( hModule, 178, GetWinFlags() );

    /* Initialize the real-mode selector entry points */

    DOSMEM_InitExports( hModule );

    /* Set interrupt vectors from entry points in WPROCS.DLL */

    hModule = GetModuleHandle16( "WPROCS" );
    for (vector = 0; vector < 256; vector++)
    {
        FARPROC16 proc = MODULE_GetEntryPoint( hModule,
                                               FIRST_INTERRUPT_ORDINAL+vector);
        assert(proc);
        INT_SetHandler( vector, proc );
    }

    return TRUE;
}


/***********************************************************************
 *           BUILTIN_LoadModule
 *
 * Load a built-in module. If the 'force' parameter is FALSE, we only
 * load the module if it has not been disabled via the -dll option.
 */
HMODULE32 BUILTIN_LoadModule( LPCSTR name, BOOL32 force )
{
    BUILTIN16_DLL *table;
    char dllname[16], *p;

    /* Fix the name in case we have a full path and extension */

    if ((p = strrchr( name, '\\' ))) name = p + 1;
    lstrcpyn32A( dllname, name, sizeof(dllname) );
    if ((p = strrchr( dllname, '.' ))) *p = '\0';

    for (table = BuiltinDLLs; table->descr; table++)
        if (!lstrcmpi32A( table->descr->name, dllname )) break;
    if (!table->descr) return BUILTIN32_LoadModule( name, force,
                                                    PROCESS_Current() );
    if ((table->flags & DLL_FLAG_NOT_USED) && !force) return 0;

    return BUILTIN_DoLoadModule16( table->descr );
}


/***********************************************************************
 *           BUILTIN_GetEntryPoint16
 *
 * Return the ordinal and name corresponding to a CS:IP address.
 * This is used only by relay debugging.
 */
LPCSTR BUILTIN_GetEntryPoint16( WORD cs, WORD ip, WORD *pOrd )
{
    static char buffer[80];
    WORD ordinal, i, max_offset;
    register BYTE *p;
    NE_MODULE *pModule;

    if (!(pModule = MODULE_GetPtr( FarGetOwner( GlobalHandle16(cs) ))))
        return NULL;

    /* Search for the ordinal */

    p = (BYTE *)pModule + pModule->entry_table;
    max_offset = 0;
    ordinal = 1;
    *pOrd = 0;
    while (*p)
    {
        switch(p[1])
        {
        case 0:    /* unused */
            ordinal += *p;
            p += 2;
            break;
        case 1:    /* code segment */
            i = *p;
            p += 2;
            while (i-- > 0)
            {
                p++;
                if ((*(WORD *)p <= ip) && (*(WORD *)p >= max_offset))
                {
                    max_offset = *(WORD *)p;
                    *pOrd = ordinal;
                }
                p += 2;
                ordinal++;
            }
            break;
        case 0xff: /* moveable (should not happen in built-in modules) */
            fprintf( stderr, "Built-in module has moveable entry\n" );
            ordinal += *p;
            p += 2 + *p * 6;
            break;
        default:   /* other segment */
            ordinal += *p;
            p += 2 + *p * 3;
            break;
        }
    }

    /* Search for the name in the resident names table */
    /* (built-in modules have no non-resident table)   */
    
    p = (BYTE *)pModule + pModule->name_table;
    while (*p)
    {
        p += *p + 1 + sizeof(WORD);
        if (*(WORD *)(p + *p + 1) == *pOrd) break;
    }

    sprintf( buffer, "%.*s.%d: %.*s",
             *((BYTE *)pModule + pModule->name_table),
             (char *)pModule + pModule->name_table + 1,
             *pOrd, *p, (char *)(p + 1) );
    return buffer;
}


/**********************************************************************
 *	    BUILTIN_DefaultIntHandler
 *
 * Default interrupt handler.
 */
void BUILTIN_DefaultIntHandler( CONTEXT *context )
{
    WORD ordinal;
    STACK16FRAME *frame = CURRENT_STACK16;
    BUILTIN_GetEntryPoint16( frame->entry_cs, frame->entry_ip, &ordinal );
    INT_BARF( context, ordinal - FIRST_INTERRUPT_ORDINAL );
}


/***********************************************************************
 *           BUILTIN_ParseDLLOptions
 *
 * Set runtime DLL usage flags
 */
BOOL32 BUILTIN_ParseDLLOptions( const char *str )
{
    BUILTIN16_DLL *dll;
    const char *p;

    while (*str)
    {
        while (*str && isspace(*str)) str++;
        if (!*str) return TRUE;
        if ((*str != '+') && (*str != '-')) return FALSE;
        str++;
        if (!(p = strchr( str, ',' ))) p = str + strlen(str);
        while ((p > str) && isspace(p[-1])) p--;
        if (p == str) return FALSE;
        for (dll = BuiltinDLLs; dll->descr; dll++)
        {
            if (!lstrncmpi32A( str, dll->descr->name, (int)(p - str) ))
            {
	        if (dll->descr->name[(int)(p-str)])  /* only partial match */
			continue;
                if (str[-1] == '-')
                {
                    if (dll->flags & DLL_FLAG_ALWAYS_USED) return FALSE;
                    dll->flags |= DLL_FLAG_NOT_USED;
                }
                else dll->flags &= ~DLL_FLAG_NOT_USED;
                break;
            }
        }
        if (!dll->descr)
            if (!BUILTIN32_EnableDLL( str, (int)(p - str), (str[-1] == '+') ))
                return FALSE;
        str = p;
        while (*str && (isspace(*str) || (*str == ','))) str++;
    }
    return TRUE;
}


/***********************************************************************
 *           BUILTIN_PrintDLLs
 *
 * Print the list of built-in DLLs that can be disabled.
 */
void BUILTIN_PrintDLLs(void)
{
    int i;
    BUILTIN16_DLL *dll;

    fprintf(stderr,"Example: -dll -ole2    Do not use emulated OLE2.DLL\n");
    fprintf(stderr,"Available Win16 DLLs:\n");
    for (i = 0, dll = BuiltinDLLs; dll->descr; dll++)
    {
        if (!(dll->flags & DLL_FLAG_ALWAYS_USED))
            fprintf( stderr, "%-9s%c", dll->descr->name,
                     ((++i) % 8) ? ' ' : '\n' );
    }
    fprintf(stderr,"\n");
    BUILTIN32_PrintDLLs();
}
