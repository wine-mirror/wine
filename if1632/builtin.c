/*
 * Built-in modules
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "windows.h"
#include "gdi.h"
#include "global.h"
#include "heap.h"
#include "module.h"
#include "miscemu.h"
#include "neexe.h"
#include "stackframe.h"
#include "user.h"
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
    const char         *name;       /* DLL name */
    int                 base;       /* Ordinal base */
    int                 nb_funcs;   /* Number of functions */
    int                 nb_names;   /* Number of function names */
    const void        **functions;  /* Pointer to function table */
    const char * const *names;      /* Pointer to names table */
    const WORD         *ordinals;   /* Pointer to ordinals table */
    const BYTE         *args;       /* Pointer to argument lengths */
} WIN32_DESCRIPTOR;

typedef union
{
    const char *name;               /* DLL name */
    WIN16_DESCRIPTOR win16;         /* Descriptor for Win16 DLL */
    WIN32_DESCRIPTOR win32;         /* Descriptor for Win32 DLL */
} DLL_DESCRIPTOR;

typedef struct
{
    BYTE  call;                    /* 0xe8 call callfrom32 (relative) */
    DWORD callfrom32 WINE_PACKED;  /* RELAY_CallFrom32 relative addr */
    BYTE  ret;                     /* 0xc2 ret $n  or  0xc3 ret */
    WORD  args;                    /* nb of args to remove from the stack */
} DEBUG_ENTRY_POINT;

typedef struct
{
    const DLL_DESCRIPTOR *descr;     /* DLL descriptor */
    DEBUG_ENTRY_POINT    *dbg_funcs; /* Relay debugging functions table */
    int                   flags;     /* flags (see below) */
} BUILTIN_DLL;

/* DLL flags */
#define DLL_FLAG_NOT_USED    0x01  /* Use original Windows DLL if possible */
#define DLL_FLAG_ALWAYS_USED 0x02  /* Always use built-in DLL */
#define DLL_FLAG_WIN32       0x04  /* DLL is a Win32 DLL */

/* 16-bit DLLs */

extern const DLL_DESCRIPTOR KERNEL_Descriptor;
extern const DLL_DESCRIPTOR USER_Descriptor;
extern const DLL_DESCRIPTOR GDI_Descriptor;
extern const DLL_DESCRIPTOR WIN87EM_Descriptor;
extern const DLL_DESCRIPTOR MMSYSTEM_Descriptor;
extern const DLL_DESCRIPTOR SHELL_Descriptor;
extern const DLL_DESCRIPTOR SOUND_Descriptor;
extern const DLL_DESCRIPTOR KEYBOARD_Descriptor;
extern const DLL_DESCRIPTOR WINSOCK_Descriptor;
extern const DLL_DESCRIPTOR STRESS_Descriptor;
extern const DLL_DESCRIPTOR SYSTEM_Descriptor;
extern const DLL_DESCRIPTOR TOOLHELP_Descriptor;
extern const DLL_DESCRIPTOR MOUSE_Descriptor;
extern const DLL_DESCRIPTOR COMMDLG_Descriptor;
extern const DLL_DESCRIPTOR OLE2_Descriptor;
extern const DLL_DESCRIPTOR OLE2CONV_Descriptor;
extern const DLL_DESCRIPTOR OLE2DISP_Descriptor;
extern const DLL_DESCRIPTOR OLE2NLS_Descriptor;
extern const DLL_DESCRIPTOR OLE2PROX_Descriptor;
extern const DLL_DESCRIPTOR OLECLI_Descriptor;
extern const DLL_DESCRIPTOR OLESVR_Descriptor;
extern const DLL_DESCRIPTOR COMPOBJ_Descriptor;
extern const DLL_DESCRIPTOR STORAGE_Descriptor;
extern const DLL_DESCRIPTOR WPROCS_Descriptor;
extern const DLL_DESCRIPTOR DDEML_Descriptor;
extern const DLL_DESCRIPTOR LZEXPAND_Descriptor;
extern const DLL_DESCRIPTOR VER_Descriptor;
extern const DLL_DESCRIPTOR W32SYS_Descriptor;
extern const DLL_DESCRIPTOR WING_Descriptor;

/* 32-bit DLLs */

extern const DLL_DESCRIPTOR ADVAPI32_Descriptor;
extern const DLL_DESCRIPTOR COMCTL32_Descriptor;
extern const DLL_DESCRIPTOR COMDLG32_Descriptor;
extern const DLL_DESCRIPTOR CRTDLL_Descriptor;
extern const DLL_DESCRIPTOR OLE32_Descriptor;
extern const DLL_DESCRIPTOR GDI32_Descriptor;
extern const DLL_DESCRIPTOR KERNEL32_Descriptor;
extern const DLL_DESCRIPTOR LZ32_Descriptor;
extern const DLL_DESCRIPTOR MPR_Descriptor;
extern const DLL_DESCRIPTOR NTDLL_Descriptor;
extern const DLL_DESCRIPTOR SHELL32_Descriptor;
extern const DLL_DESCRIPTOR USER32_Descriptor;
extern const DLL_DESCRIPTOR VERSION_Descriptor;
extern const DLL_DESCRIPTOR WINMM_Descriptor;
extern const DLL_DESCRIPTOR WINSPOOL_Descriptor;
extern const DLL_DESCRIPTOR WSOCK32_Descriptor;

/* Table of all built-in DLLs */

static BUILTIN_DLL BuiltinDLLs[] =
{
    /* Win16 DLLs */
    { &KERNEL_Descriptor,   NULL, DLL_FLAG_ALWAYS_USED },
    { &USER_Descriptor,     NULL, DLL_FLAG_ALWAYS_USED },
    { &GDI_Descriptor,      NULL, DLL_FLAG_ALWAYS_USED },
    { &SYSTEM_Descriptor,   NULL, DLL_FLAG_ALWAYS_USED },
    { &WIN87EM_Descriptor,  NULL, DLL_FLAG_NOT_USED },
    { &SHELL_Descriptor,    NULL, 0 },
    { &SOUND_Descriptor,    NULL, 0 },
    { &KEYBOARD_Descriptor, NULL, 0 },
    { &WINSOCK_Descriptor,  NULL, 0 },
    { &STRESS_Descriptor,   NULL, 0 },
    { &MMSYSTEM_Descriptor, NULL, 0 },
    { &TOOLHELP_Descriptor, NULL, 0 },
    { &MOUSE_Descriptor,    NULL, 0 },
    { &COMMDLG_Descriptor,  NULL, DLL_FLAG_NOT_USED },
    { &OLE2_Descriptor,     NULL, DLL_FLAG_NOT_USED },
    { &OLE2CONV_Descriptor, NULL, DLL_FLAG_NOT_USED },
    { &OLE2DISP_Descriptor, NULL, DLL_FLAG_NOT_USED },
    { &OLE2NLS_Descriptor,  NULL, DLL_FLAG_NOT_USED },
    { &OLE2PROX_Descriptor, NULL, DLL_FLAG_NOT_USED },
    { &OLECLI_Descriptor,   NULL, DLL_FLAG_NOT_USED },
    { &OLESVR_Descriptor,   NULL, DLL_FLAG_NOT_USED },
    { &COMPOBJ_Descriptor,  NULL, DLL_FLAG_NOT_USED },
    { &STORAGE_Descriptor,  NULL, DLL_FLAG_NOT_USED },
    { &WPROCS_Descriptor,   NULL, DLL_FLAG_ALWAYS_USED },
    { &DDEML_Descriptor,    NULL, DLL_FLAG_NOT_USED },
    { &LZEXPAND_Descriptor, NULL, 0 },
    { &VER_Descriptor,      NULL, 0 },
    { &W32SYS_Descriptor,   NULL, 0 },
    { &WING_Descriptor,     NULL, 0 },
    /* Win32 DLLs */
    { &ADVAPI32_Descriptor, NULL, DLL_FLAG_WIN32 },
    { &COMCTL32_Descriptor, NULL, DLL_FLAG_WIN32 | DLL_FLAG_NOT_USED },
    { &COMDLG32_Descriptor, NULL, DLL_FLAG_WIN32 },
    { &CRTDLL_Descriptor,   NULL, DLL_FLAG_WIN32 },
    { &OLE32_Descriptor,    NULL, DLL_FLAG_WIN32 | DLL_FLAG_NOT_USED },
    { &GDI32_Descriptor,    NULL, DLL_FLAG_WIN32 },
    { &KERNEL32_Descriptor, NULL, DLL_FLAG_WIN32 | DLL_FLAG_ALWAYS_USED },
    { &LZ32_Descriptor,     NULL, DLL_FLAG_WIN32 },
    { &MPR_Descriptor,      NULL, DLL_FLAG_WIN32 | DLL_FLAG_NOT_USED },
    { &NTDLL_Descriptor,    NULL, DLL_FLAG_WIN32 },
    { &SHELL32_Descriptor,  NULL, DLL_FLAG_WIN32 },
    { &USER32_Descriptor,   NULL, DLL_FLAG_WIN32 },
    { &VERSION_Descriptor,  NULL, DLL_FLAG_WIN32 },
    { &WINMM_Descriptor,    NULL, DLL_FLAG_WIN32 },
    { &WINSPOOL_Descriptor, NULL, DLL_FLAG_WIN32 },
    { &WSOCK32_Descriptor,  NULL, DLL_FLAG_WIN32 },
    /* Last entry */
    { NULL, NULL, 0 }
};

  /* Ordinal number for interrupt 0 handler in WPROCS.DLL */
#define FIRST_INTERRUPT_ORDINAL 100

/***********************************************************************
 *           BUILTIN_BuildDebugEntryPoints
 *
 * Build the table of relay-debugging entry points for a Win32 DLL.
 */
static void BUILTIN_BuildDebugEntryPoints( BUILTIN_DLL *dll )
{
    int i;
    DEBUG_ENTRY_POINT *entry;
    extern void RELAY_CallFrom32();

    assert( !dll->dbg_funcs );
    assert( dll->flags & DLL_FLAG_WIN32 );
    dll->dbg_funcs = HeapAlloc( SystemHeap, 0,
                      dll->descr->win32.nb_funcs * sizeof(DEBUG_ENTRY_POINT) );
    entry = dll->dbg_funcs;
    for (i = 0; i < dll->descr->win32.nb_funcs; i++, entry++)
    {
        BYTE args = dll->descr->win32.args[i];
        entry->call = 0xe8;  /* call */
        switch(args)
        {
        case 0xfe:  /* register func */
            entry->callfrom32 = (DWORD)dll->descr->win32.functions[i] -
                                (DWORD)&entry->ret;
            entry->ret        = 0x90;  /* nop */
            entry->args       = 0;
            break;
        case 0xff:  /* stub */
            entry->args = 0xffff;
            break;
        default:  /* normal function (stdcall or cdecl) */
            entry->callfrom32 = (DWORD)RELAY_CallFrom32 - (DWORD)&entry->ret;
            entry->ret        = (args & 0x80) ? 0xc3 : 0xc2; /* ret / ret $n */
            entry->args       = (args & 0x7f) * sizeof(int);
            break;
        }
    }
}


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
 *           BUILTIN_DoLoadModule32
 *
 * Load a built-in Win32 module. Helper function for BUILTIN_LoadModule
 * and BUILTIN_Init.
 */
static HMODULE16 BUILTIN_DoLoadModule32( BUILTIN_DLL *table )
{
    HMODULE16 hModule;
    NE_MODULE *pModule;
    OFSTRUCT ofs;

    sprintf( ofs.szPathName, "%s.DLL", table->descr->name );
    hModule = MODULE_CreateDummyModule( &ofs );
    pModule = (NE_MODULE *)GlobalLock16( hModule );
    pModule->pe_module = (PE_MODULE *)table;
    pModule->flags = NE_FFLAGS_SINGLEDATA | NE_FFLAGS_BUILTIN |
        NE_FFLAGS_LIBMODULE | NE_FFLAGS_WIN32;
    if (debugging_relay) BUILTIN_BuildDebugEntryPoints( table );
    return hModule;
}


/***********************************************************************
 *           BUILTIN_Init
 *
 * Load all built-in modules marked as 'always used'.
 */
BOOL32 BUILTIN_Init(void)
{
    BUILTIN_DLL *dll;
    NE_MODULE *pModule;
    WORD vector;
    HMODULE16 hModule;

    for (dll = BuiltinDLLs; dll->descr; dll++)
    {
        if (!(dll->flags & DLL_FLAG_ALWAYS_USED)) continue;
        if (dll->flags & DLL_FLAG_WIN32)
        {
            if (!BUILTIN_DoLoadModule32( dll )) return FALSE;
        }
        else
        {
            if (!BUILTIN_DoLoadModule16( &dll->descr->win16 )) return FALSE;
        }
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
HMODULE16 BUILTIN_LoadModule( LPCSTR name, BOOL32 force )
{
    BUILTIN_DLL *table;
    char dllname[16], *p;

    /* Fix the name in case we have a full path and extension */

    if ((p = strrchr( name, '\\' ))) name = p + 1;
    lstrcpyn32A( dllname, name, sizeof(dllname) );
    if ((p = strrchr( dllname, '.' ))) *p = '\0';

    for (table = BuiltinDLLs; table->descr; table++)
        if (!lstrcmpi32A( table->descr->name, dllname )) break;
    if (!table->descr) return 0;
    if ((table->flags & DLL_FLAG_NOT_USED) && !force) return 0;

    if (table->flags & DLL_FLAG_WIN32)
        return BUILTIN_DoLoadModule32( table );
    else
        return BUILTIN_DoLoadModule16( &table->descr->win16 );
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


/***********************************************************************
 *           BUILTIN_GetEntryPoint32
 *
 * Return the name of the DLL entry point corresponding
 * to a relay entry point address. This is used only by relay debugging.
 *
 * This function _must_ return the real entry point to call
 * after the debug info is printed.
 */
FARPROC32 BUILTIN_GetEntryPoint32( char *buffer, void *relay )
{
    BUILTIN_DLL *dll;
    int ordinal, i;
    const WIN32_DESCRIPTOR *descr;

    /* First find the module */

    for (dll = BuiltinDLLs; dll->descr; dll++)
        if ((dll->flags & DLL_FLAG_WIN32) &&
            ((void *)dll->dbg_funcs <= relay) &&
            ((void *)(dll->dbg_funcs + dll->descr->win32.nb_funcs) > relay))
            break;
    assert(dll->descr);
    descr = &dll->descr->win32;

    /* Now find the function */

    ordinal = ((DWORD)relay-(DWORD)dll->dbg_funcs) / sizeof(DEBUG_ENTRY_POINT);
    ordinal += descr->base;
    for (i = 0; i < descr->nb_names; i++)
        if (descr->ordinals[i] == ordinal) break;
    assert( i < descr->nb_names );

    sprintf( buffer, "%s.%d: %s", descr->name, ordinal, descr->names[i] );
    return (FARPROC32)descr->functions[ordinal - descr->base];
}


/***********************************************************************
 *           BUILTIN_GetProcAddress32
 *
 * Implementation of GetProcAddress() for built-in Win32 modules.
 * FIXME: this should be unified with the real GetProcAddress32().
 */
FARPROC32 BUILTIN_GetProcAddress32( NE_MODULE *pModule, LPCSTR function )
{
    BUILTIN_DLL *dll = (BUILTIN_DLL *)pModule->pe_module;
    const WIN32_DESCRIPTOR *info = &dll->descr->win32;
    WORD ordinal = 0;

    if (!dll) return NULL;

    if (HIWORD(function))  /* Find function by name */
    {
        int i;

        dprintf_module( stddeb, "Looking for function %s in %s\n",
                        function, dll->descr->name );
        for (i = 0; i < info->nb_names; i++)
            if (!strcmp( function, info->names[i] ))
            {
                ordinal = info->ordinals[i];
                break;
            }
        if (i >= info->nb_names) return NULL;  /* not found */
    }
    else  /* Find function by ordinal */
    {
        ordinal = LOWORD(function);
        dprintf_module( stddeb, "Looking for ordinal %d in %s\n",
                        ordinal, dll->descr->name );
        if ((ordinal < info->base) || (ordinal >= info->base + info->nb_funcs))
            return NULL;  /* not found */
    }
    if (dll->dbg_funcs && (dll->dbg_funcs[ordinal-info->base].args != 0xffff))
        return (FARPROC32)&dll->dbg_funcs[ordinal - info->base];
    return (FARPROC32)info->functions[ordinal - info->base];
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
    BUILTIN_DLL *dll;
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
                if (str[-1] == '-')
                {
                    if (dll->flags & DLL_FLAG_ALWAYS_USED) return FALSE;
                    dll->flags |= DLL_FLAG_NOT_USED;
                }
                else dll->flags &= ~DLL_FLAG_NOT_USED;
                break;
            }
        }
        if (!dll->descr) return FALSE;
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
    BUILTIN_DLL *dll;

    for (i = 0, dll = BuiltinDLLs; dll->descr; dll++)
    {
        if (!(dll->flags & DLL_FLAG_ALWAYS_USED))
            fprintf( stderr, "%-9s%c", dll->descr->name,
                     ((++i) % 8) ? ' ' : '\n' );
    }
    fprintf(stderr,"\n");
    exit(1);
}
