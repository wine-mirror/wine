/*
 * Win32 builtin functions
 *
 * Copyright 1997 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>
#include "builtin32.h"
#include "module.h"
#include "heap.h"
#include "task.h"
#include "process.h"
#include "debug.h"

typedef struct
{
    BYTE  call;                    /* 0xe8 call callfrom32 (relative) */
    DWORD callfrom32 WINE_PACKED;  /* RELAY_CallFrom32 relative addr */
    BYTE  ret;                     /* 0xc2 ret $n  or  0xc3 ret */
    WORD  args;                    /* nb of args to remove from the stack */
} DEBUG_ENTRY_POINT;

typedef struct
{
    const BUILTIN32_DESCRIPTOR *descr;     /* DLL descriptor */
    BOOL32                      used;      /* Used by default */
} BUILTIN32_DLL;


extern const BUILTIN32_DESCRIPTOR ADVAPI32_Descriptor;
extern const BUILTIN32_DESCRIPTOR COMCTL32_Descriptor;
extern const BUILTIN32_DESCRIPTOR COMDLG32_Descriptor;
extern const BUILTIN32_DESCRIPTOR CRTDLL_Descriptor;
extern const BUILTIN32_DESCRIPTOR DCIMAN32_Descriptor;
extern const BUILTIN32_DESCRIPTOR DDRAW_Descriptor;
extern const BUILTIN32_DESCRIPTOR DINPUT_Descriptor;
extern const BUILTIN32_DESCRIPTOR DPLAY_Descriptor;
extern const BUILTIN32_DESCRIPTOR DPLAYX_Descriptor;
extern const BUILTIN32_DESCRIPTOR DSOUND_Descriptor;
extern const BUILTIN32_DESCRIPTOR GDI32_Descriptor;
extern const BUILTIN32_DESCRIPTOR KERNEL32_Descriptor;
extern const BUILTIN32_DESCRIPTOR LZ32_Descriptor;
extern const BUILTIN32_DESCRIPTOR MPR_Descriptor;
extern const BUILTIN32_DESCRIPTOR MSVFW32_Descriptor;
extern const BUILTIN32_DESCRIPTOR NTDLL_Descriptor;
extern const BUILTIN32_DESCRIPTOR OLE32_Descriptor;
extern const BUILTIN32_DESCRIPTOR OLEAUT32_Descriptor;
extern const BUILTIN32_DESCRIPTOR OLECLI32_Descriptor;
extern const BUILTIN32_DESCRIPTOR OLESVR32_Descriptor;
extern const BUILTIN32_DESCRIPTOR RASAPI32_Descriptor;
extern const BUILTIN32_DESCRIPTOR SHELL32_Descriptor;
extern const BUILTIN32_DESCRIPTOR TAPI32_Descriptor;
extern const BUILTIN32_DESCRIPTOR USER32_Descriptor;
extern const BUILTIN32_DESCRIPTOR VERSION_Descriptor;
extern const BUILTIN32_DESCRIPTOR W32SKRNL_Descriptor;
extern const BUILTIN32_DESCRIPTOR WINMM_Descriptor;
extern const BUILTIN32_DESCRIPTOR WINSPOOL_Descriptor;
extern const BUILTIN32_DESCRIPTOR WNASPI32_Descriptor;
extern const BUILTIN32_DESCRIPTOR WOW32_Descriptor;
extern const BUILTIN32_DESCRIPTOR WSOCK32_Descriptor;

static BUILTIN32_DLL BuiltinDLLs[] =
{
    { &ADVAPI32_Descriptor, TRUE  },
    { &COMCTL32_Descriptor, FALSE },
    { &COMDLG32_Descriptor, TRUE  },
    { &CRTDLL_Descriptor,   TRUE  },
    { &DCIMAN32_Descriptor, FALSE },
    { &DDRAW_Descriptor,    TRUE  },
    { &DINPUT_Descriptor,   TRUE  },
    { &DPLAY_Descriptor,    TRUE  },
    { &DPLAYX_Descriptor,   TRUE  },
    { &DSOUND_Descriptor,   TRUE  },
    { &GDI32_Descriptor,    TRUE  },
    { &KERNEL32_Descriptor, TRUE  },
    { &LZ32_Descriptor,     TRUE  },
    { &MPR_Descriptor,      TRUE  },
    { &MSVFW32_Descriptor,  FALSE },
    { &NTDLL_Descriptor,    TRUE  },
    { &OLE32_Descriptor,    FALSE },
    { &OLEAUT32_Descriptor, FALSE },
    { &OLECLI32_Descriptor, FALSE },
    { &OLESVR32_Descriptor, FALSE },
    { &RASAPI32_Descriptor, FALSE },
    { &SHELL32_Descriptor,  TRUE  },
    { &TAPI32_Descriptor,   FALSE },
    { &USER32_Descriptor,   TRUE  },
    { &VERSION_Descriptor,  TRUE  },
    { &W32SKRNL_Descriptor, TRUE  },
    { &WINMM_Descriptor,    TRUE  },
    { &WINSPOOL_Descriptor, TRUE  },
    { &WNASPI32_Descriptor, TRUE  },
    { &WOW32_Descriptor,    TRUE  },
    { &WSOCK32_Descriptor,  TRUE  },
    /* Last entry */
    { NULL, FALSE }
};


/***********************************************************************
 *           BUILTIN32_DoLoadModule
 *
 * Load a built-in Win32 module. Helper function for BUILTIN32_LoadModule.
 */
static HMODULE32 BUILTIN32_DoLoadModule( BUILTIN32_DLL *dll, PDB32 *pdb )
{
    extern void RELAY_CallFrom32();

    HMODULE16 hModule;
    NE_MODULE *pModule;
    OFSTRUCT ofs;
    IMAGE_DATA_DIRECTORY *dir;
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sec;
    IMAGE_EXPORT_DIRECTORY *exp;
    LPVOID *funcs;
    LPSTR *names;
    DEBUG_ENTRY_POINT *debug;
    WINE_MODREF *wm;
    PE_MODREF *pem;
    INT32 i, size;
    BYTE *addr;

    /* Allocate the module */

    size = (sizeof(IMAGE_DOS_HEADER)
            + sizeof(IMAGE_NT_HEADERS)
            + 2 * sizeof(IMAGE_SECTION_HEADER)
            + sizeof(IMAGE_EXPORT_DIRECTORY)
            + dll->descr->nb_funcs * sizeof(LPVOID)
            + dll->descr->nb_names * sizeof(LPSTR));
#ifdef __i386__
    if (TRACE_ON(relay))
        size += dll->descr->nb_funcs * sizeof(DEBUG_ENTRY_POINT);
#endif
    addr  = VirtualAlloc( NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    if (!addr) return 0;
    dos   = (IMAGE_DOS_HEADER *)addr;
    nt    = (IMAGE_NT_HEADERS *)(dos + 1);
    sec   = (IMAGE_SECTION_HEADER *)(nt + 1);
    exp   = (IMAGE_EXPORT_DIRECTORY *)(sec + 2);
    funcs = (LPVOID *)(exp + 1);
    names = (LPSTR *)(funcs + dll->descr->nb_funcs);
    debug = (DEBUG_ENTRY_POINT *)(names + dll->descr->nb_names);

    /* Build the DOS and NT headers */

    dos->e_magic  = IMAGE_DOS_SIGNATURE;
    dos->e_lfanew = sizeof(*dos);

    nt->Signature                       = IMAGE_NT_SIGNATURE;
    nt->FileHeader.Machine              = IMAGE_FILE_MACHINE_I386;
    nt->FileHeader.NumberOfSections     = 2;  /* exports + code */
    nt->FileHeader.SizeOfOptionalHeader = sizeof(nt->OptionalHeader);
    nt->FileHeader.Characteristics      = IMAGE_FILE_DLL;

    nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR_MAGIC;
    nt->OptionalHeader.SizeOfCode                  = 0x1000;
    nt->OptionalHeader.SizeOfInitializedData       = 0;
    nt->OptionalHeader.SizeOfUninitializedData     = 0;
    nt->OptionalHeader.ImageBase                   = (DWORD)addr;
    nt->OptionalHeader.SectionAlignment            = 0x1000;
    nt->OptionalHeader.FileAlignment               = 0x1000;
    nt->OptionalHeader.MajorOperatingSystemVersion = 1;
    nt->OptionalHeader.MinorOperatingSystemVersion = 0;
    nt->OptionalHeader.MajorSubsystemVersion       = 4;
    nt->OptionalHeader.MinorSubsystemVersion       = 0;
    nt->OptionalHeader.SizeOfImage                 = size;
    nt->OptionalHeader.SizeOfHeaders               = (BYTE *)exp - addr;
    nt->OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    if (dll->descr->dllentrypoint) 
        nt->OptionalHeader.AddressOfEntryPoint = (DWORD)dll->descr->dllentrypoint - (DWORD)addr;
    
    /* Build the export directory */

    dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_EXPORT_DIRECTORY];
    dir->VirtualAddress = (BYTE *)exp - addr;
    dir->Size = sizeof(*exp)
                + dll->descr->nb_funcs * sizeof(LPVOID)
                + dll->descr->nb_names * sizeof(LPSTR);

    /* Build the exports section */

    strcpy( sec->Name, ".edata" );
    sec->Misc.VirtualSize = dir->Size;
    sec->VirtualAddress   = (BYTE *)exp - addr;
    sec->SizeOfRawData    = dir->Size;
    sec->PointerToRawData = (BYTE *)exp - addr;
    sec->Characteristics  = (IMAGE_SCN_CNT_INITIALIZED_DATA |
                             IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ |
                             IMAGE_SCN_MEM_WRITE);

    /* Build the code section */

    sec++;
    strcpy( sec->Name, ".code" );
    sec->SizeOfRawData = 0;
#ifdef __i386__
    if (TRACE_ON(relay))
        sec->SizeOfRawData += dll->descr->nb_funcs * sizeof(DEBUG_ENTRY_POINT);
#endif
    sec->Misc.VirtualSize = sec->SizeOfRawData;
    sec->VirtualAddress   = (BYTE *)debug - addr;
    sec->PointerToRawData = (BYTE *)debug - addr;
    sec->Characteristics  = (IMAGE_SCN_CNT_INITIALIZED_DATA |
                             IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ);

    /* Build the exports section data */

    exp->Name                  = ((BYTE *)dll->descr->name) - addr;  /*??*/
    exp->Base                  = dll->descr->base;
    exp->NumberOfFunctions     = dll->descr->nb_funcs;
    exp->NumberOfNames         = dll->descr->nb_names;
    exp->AddressOfFunctions    = (LPDWORD *)((BYTE *)funcs - addr);
    exp->AddressOfNames        = (LPDWORD *)((BYTE *)names - addr);
    exp->AddressOfNameOrdinals = (LPWORD *)((BYTE *)dll->descr->ordinals - addr);

    /* Build the funcs table */

    for (i = 0; i < dll->descr->nb_funcs; i++, funcs++, debug++)
    {
        BYTE args = dll->descr->args[i];
        if (!dll->descr->functions[i]) continue;
        *funcs = (LPVOID)((BYTE *)dll->descr->functions[i] - addr);
#ifdef __i386__
        if (!TRACE_ON(relay)) continue;
        switch(args)
        {
        case 0xfe:  /* register func */
            debug->call       = 0xe8;
            debug->callfrom32 = (DWORD)dll->descr->functions[i] -
                                (DWORD)&debug->ret;
            debug->ret        = 0x90;  /* nop */
            debug->args       = 0;
            *funcs = (LPVOID)((BYTE *)debug - addr);
            break;
        case 0xff:  /* stub or extern */
            break;
        default:  /* normal function (stdcall or cdecl) */
            debug->call       = 0xe8;
            debug->callfrom32 = (DWORD)RELAY_CallFrom32 -
                                (DWORD)&debug->ret;
            debug->ret        = (args & 0x80) ? 0xc3 : 0xc2; /*ret/ret $n*/
            debug->args       = (args & 0x7f) * sizeof(int);
            *funcs = (LPVOID)((BYTE *)debug - addr);
            break;
        }
#endif  /* __i386__ */
    }

    /* Build the names table */

    for (i = 0; i < exp->NumberOfNames; i++, names++)
        if (dll->descr->names[i])
            *names = (LPSTR)((BYTE *)dll->descr->names[i] - addr);

    /* Create a modref */
    wm = (WINE_MODREF *)HeapAlloc( pdb->heap, HEAP_ZERO_MEMORY, sizeof(*wm) );
    wm->type = MODULE32_PE;
    pem = &(wm->binfmt.pe);
    wm->module		= (HMODULE32)addr;
    wm->next		= pdb->modref_list;
    pdb->modref_list	= wm;
    wm->modname		= HEAP_strdupA(pdb->heap,0,dll->descr->name);
    /* FIXME: hmm ... probably add windows directory? don't know ... -MM */
    wm->shortname	= HEAP_strdupA(pdb->heap,0,wm->modname);
    wm->longname	= HEAP_strdupA(pdb->heap,0,wm->modname);

    pem->pe_export	= exp;
    pem->flags		= PE_MODREF_INTERNAL;

    /* Create a Win16 dummy module */

    sprintf( ofs.szPathName, "%s.DLL", dll->descr->name );
    hModule = MODULE_CreateDummyModule( &ofs );
    pModule = (NE_MODULE *)GlobalLock16( hModule );
    pModule->flags = NE_FFLAGS_SINGLEDATA | NE_FFLAGS_BUILTIN |
                     NE_FFLAGS_LIBMODULE | NE_FFLAGS_WIN32;
    pModule->module32 = (HMODULE32)addr;
    return pModule->module32;
}


/***********************************************************************
 *           BUILTIN32_LoadModule
 *
 * Load a built-in module. If the 'force' parameter is FALSE, we only
 * load the module if it has not been disabled via the -dll option.
 */
HMODULE32 BUILTIN32_LoadModule( LPCSTR name, BOOL32 force, PDB32 *process )
{
    BUILTIN32_DLL *table;
    char dllname[16], *p;

    /* Fix the name in case we have a full path and extension */

    if ((p = strrchr( name, '\\' ))) name = p + 1;
    lstrcpyn32A( dllname, name, sizeof(dllname) );
    if ((p = strrchr( dllname, '.' ))) *p = '\0';

    for (table = BuiltinDLLs; table->descr; table++)
        if (!lstrcmpi32A( table->descr->name, dllname )) break;
    if (!table->descr) return 0;
    if (!table->used)
    {
        if (!force) return 0;
        table->used = TRUE;  /* So next time we use it at once */
    }
    return BUILTIN32_DoLoadModule( table, process );
}


/***********************************************************************
 *           BUILTIN32_GetEntryPoint
 *
 * Return the name of the DLL entry point corresponding
 * to a relay entry point address. This is used only by relay debugging.
 *
 * This function _must_ return the real entry point to call
 * after the debug info is printed.
 */
ENTRYPOINT32 BUILTIN32_GetEntryPoint( char *buffer, void *relay,
                                      unsigned int *typemask )
{
    BUILTIN32_DLL *dll;
    HMODULE32 hModule;
    int ordinal = 0, i;

    /* First find the module */

    for (dll = BuiltinDLLs; dll->descr; dll++)
        if (dll->used 
            && ((hModule = GetModuleHandle32A(dll->descr->name)) != 0))
        {
            IMAGE_SECTION_HEADER *sec = PE_SECTIONS(hModule);
            DEBUG_ENTRY_POINT *debug = 
                 (DEBUG_ENTRY_POINT *)((DWORD)hModule + sec[1].VirtualAddress);
            DEBUG_ENTRY_POINT *func = (DEBUG_ENTRY_POINT *)relay;

            if (debug <= func && func < debug + dll->descr->nb_funcs)
            {
                ordinal = func - debug;
                break;
            }
        }
    
    if (!dll->descr)
    	return (ENTRYPOINT32)NULL;

    /* Now find the function */

    for (i = 0; i < dll->descr->nb_names; i++)
        if (dll->descr->ordinals[i] == ordinal) break;
    assert( i < dll->descr->nb_names );

    sprintf( buffer, "%s.%d: %s", dll->descr->name, ordinal + dll->descr->base,
             dll->descr->names[i] );
    *typemask = dll->descr->argtypes[ordinal];
    return dll->descr->functions[ordinal];
}


/***********************************************************************
 *           BUILTIN32_Unimplemented
 *
 * This function is called for unimplemented 32-bit entry points (declared
 * as 'stub' in the spec file).
 */
void BUILTIN32_Unimplemented( const BUILTIN32_DESCRIPTOR *descr, int ordinal )
{
    const char *func_name = "???";
    int i;

    __RESTORE_ES;  /* Just in case */

    for (i = 0; i < descr->nb_names; i++)
        if (descr->ordinals[i] + descr->base == ordinal) break;
    if (i < descr->nb_names) func_name = descr->names[i];

    MSG( "No handler for Win32 routine %s.%d: %s",
             descr->name, ordinal, func_name );
#ifdef __GNUC__
    MSG( " (called from %p)", __builtin_return_address(1) );
#endif
    MSG( "\n" );
    TASK_KillCurrentTask(1);
}


/***********************************************************************
 *           BUILTIN32_EnableDLL
 *
 * Enable or disable a built-in DLL.
 */
int BUILTIN32_EnableDLL( const char *name, int len, int enable )
{
    int i;
    BUILTIN32_DLL *dll;

    for (i = 0, dll = BuiltinDLLs; dll->descr; dll++)
    {
        if (!lstrncmpi32A( name, dll->descr->name, len ))
        {
            dll->used = enable;
            return TRUE;
        }
    }
    return FALSE;
}


/***********************************************************************
 *           BUILTIN32_PrintDLLs
 *
 * Print the list of built-in DLLs that can be disabled.
 */
void BUILTIN32_PrintDLLs(void)
{
    int i;
    BUILTIN32_DLL *dll;

    MSG("Available Win32 DLLs:\n");
    for (i = 0, dll = BuiltinDLLs; dll->descr; dll++)
        MSG("%-9s%c", dll->descr->name,
                 ((++i) % 8) ? ' ' : '\n' );
    MSG("\n");
}
