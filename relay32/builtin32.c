/*
 * Win32 builtin functions
 *
 * Copyright 1997 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "builtin32.h"
#include "peexe.h"
#include "neexe.h"
#include "heap.h"
#include "main.h"
#include "snoop.h"
#include "winerror.h"
#include "server.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(module);
DECLARE_DEBUG_CHANNEL(relay);

typedef struct
{
    BYTE  call;                    /* 0xe8 call callfrom32 (relative) */
    DWORD callfrom32 WINE_PACKED;  /* RELAY_CallFrom32 relative addr */
    BYTE  ret;                     /* 0xc2 ret $n  or  0xc3 ret */
    WORD  args;                    /* nb of args to remove from the stack */
} DEBUG_ENTRY_POINT;

typedef struct
{
	const BYTE			*restab;
	const DWORD			nresources;
	const DWORD			restabsize;
	const IMAGE_RESOURCE_DATA_ENTRY	*entries;
} BUILTIN32_RESOURCE;

#define MAX_DLLS 60

static const BUILTIN32_DESCRIPTOR *builtin_dlls[MAX_DLLS];
static HMODULE dll_modules[MAX_DLLS];
static int nb_dlls;

extern void RELAY_CallFrom32();
extern void RELAY_CallFrom32Regs();


/***********************************************************************
 *           BUILTIN32_WarnSecondInstance
 *
 * Emit a warning when we are creating a second instance for a DLL
 * that is known to not support this.
 */
static void BUILTIN32_WarnSecondInstance( const char *name )
{
    static const char * const warning_list[] =
    { "comctl32", "comdlg32", "crtdll", "imagehlp", "msacm32", "shell32", NULL };

    const char * const *ptr = warning_list;

    while (*ptr)
    {
        if (!strcasecmp( *ptr, name ))
        {
            ERR( "Attempt to instantiate built-in dll '%s' twice "
                 "in the same address space. Expect trouble!\n", name );
            return;
        }
        ptr++;
    }
}

/***********************************************************************
 *           BUILTIN32_DoLoadImage
 *
 * Load a built-in Win32 module. Helper function for BUILTIN32_LoadImage.
 */
static HMODULE BUILTIN32_DoLoadImage( const BUILTIN32_DESCRIPTOR *descr )
{

    IMAGE_DATA_DIRECTORY *dir;
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sec;
    IMAGE_EXPORT_DIRECTORY *exp;
    IMAGE_IMPORT_DESCRIPTOR *imp;
    const BUILTIN32_RESOURCE *rsrc = descr->rsrc;
    LPVOID *funcs;
    LPSTR *names;
    LPSTR pfwd, rtab;
    DEBUG_ENTRY_POINT *debug;
    INT i, size, nb_sections;
    BYTE *addr;

    /* Allocate the module */

    nb_sections = 2;  /* exports + code */
    if (descr->nb_imports) nb_sections++;
    size = (sizeof(IMAGE_DOS_HEADER)
            + sizeof(IMAGE_NT_HEADERS)
            + nb_sections * sizeof(IMAGE_SECTION_HEADER)
            + (descr->nb_imports+1) * sizeof(IMAGE_IMPORT_DESCRIPTOR)
            + sizeof(IMAGE_EXPORT_DIRECTORY)
            + descr->nb_funcs * sizeof(LPVOID)
            + descr->nb_names * sizeof(LPSTR)
            + descr->fwd_size);
#ifdef __i386__
    if (WARN_ON(relay) || TRACE_ON(relay))
        size += descr->nb_funcs * sizeof(DEBUG_ENTRY_POINT);
#endif
    if (rsrc) size += rsrc->restabsize;
    addr  = VirtualAlloc( NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    if (!addr) return 0;
    dos   = (IMAGE_DOS_HEADER *)addr;
    nt    = (IMAGE_NT_HEADERS *)(dos + 1);
    sec   = (IMAGE_SECTION_HEADER *)(nt + 1);
    imp   = (IMAGE_IMPORT_DESCRIPTOR *)(sec + nb_sections);
    exp   = (IMAGE_EXPORT_DIRECTORY *)(imp + descr->nb_imports + 1);
    funcs = (LPVOID *)(exp + 1);
    names = (LPSTR *)(funcs + descr->nb_funcs);
    pfwd  = (LPSTR)(names + descr->nb_names);
    rtab  = pfwd + descr->fwd_size;
    debug = (DEBUG_ENTRY_POINT *)(rtab + (rsrc ? rsrc->restabsize : 0));

    /* Build the DOS and NT headers */

    dos->e_magic  = IMAGE_DOS_SIGNATURE;
    dos->e_lfanew = sizeof(*dos);

    nt->Signature                       = IMAGE_NT_SIGNATURE;
    nt->FileHeader.Machine              = IMAGE_FILE_MACHINE_I386;
    nt->FileHeader.NumberOfSections     = nb_sections;
    nt->FileHeader.SizeOfOptionalHeader = sizeof(nt->OptionalHeader);
    nt->FileHeader.Characteristics      = descr->characteristics;

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
    if (descr->dllentrypoint) 
        nt->OptionalHeader.AddressOfEntryPoint = (DWORD)descr->dllentrypoint - (DWORD)addr;
    
    /* Build the code section */

    strcpy( sec->Name, ".code" );
    sec->SizeOfRawData = 0;
#ifdef __i386__
    if (WARN_ON(relay) || TRACE_ON(relay))
        sec->SizeOfRawData += descr->nb_funcs * sizeof(DEBUG_ENTRY_POINT);
#endif
    sec->Misc.VirtualSize = sec->SizeOfRawData;
    sec->VirtualAddress   = (BYTE *)debug - addr;
    sec->PointerToRawData = (BYTE *)debug - addr;
    sec->Characteristics  = (IMAGE_SCN_CNT_INITIALIZED_DATA |
                             IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ);
    sec++;

    /* Build the import directory */

    if (descr->nb_imports)
    {
        dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_IMPORT_DIRECTORY];
        dir->VirtualAddress = (BYTE *)imp - addr;
        dir->Size = sizeof(*imp) * (descr->nb_imports + 1);

        /* Build the imports section */
        strcpy( sec->Name, ".idata" );
        sec->Misc.VirtualSize = dir->Size;
        sec->VirtualAddress   = (BYTE *)imp - addr;
        sec->SizeOfRawData    = dir->Size;
        sec->PointerToRawData = (BYTE *)imp - addr;
        sec->Characteristics  = (IMAGE_SCN_CNT_INITIALIZED_DATA |
                                 IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ |
                                 IMAGE_SCN_MEM_WRITE);
        sec++;

        /* Build the imports */
        for (i = 0; i < descr->nb_imports; i++)
        {
            imp[i].u.Characteristics = 0;
            imp[i].ForwarderChain = -1;
            imp[i].Name = (BYTE *)descr->imports[i] - addr;
            /* hack: make first thunk point to some zero value */
            imp[i].FirstThunk = (PIMAGE_THUNK_DATA)((BYTE *)&imp[i].u.Characteristics - addr);
        }
    }

    /* Build the export directory */

    dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_EXPORT_DIRECTORY];
    dir->VirtualAddress = (BYTE *)exp - addr;
    dir->Size = sizeof(*exp)
                + descr->nb_funcs * sizeof(LPVOID)
                + descr->nb_names * sizeof(LPSTR)
                + descr->fwd_size;

    /* Build the exports section */

    strcpy( sec->Name, ".edata" );
    sec->Misc.VirtualSize = dir->Size;
    sec->VirtualAddress   = (BYTE *)exp - addr;
    sec->SizeOfRawData    = dir->Size;
    sec->PointerToRawData = (BYTE *)exp - addr;
    sec->Characteristics  = (IMAGE_SCN_CNT_INITIALIZED_DATA |
                             IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ |
                             IMAGE_SCN_MEM_WRITE);
    sec++;

    /* Build the resource directory */

    if (rsrc)
    {
	IMAGE_RESOURCE_DATA_ENTRY *rdep;

	/*
	 * The resource directory has to be copied because it contains
	 * RVAs. These would be invalid if the dll is instantiated twice.
	 */
	memcpy(rtab, rsrc->restab, rsrc->restabsize);

	dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_RESOURCE_DIRECTORY];
	dir->VirtualAddress = (BYTE *)rtab - addr;
	dir->Size = rsrc->restabsize;
	rdep = (IMAGE_RESOURCE_DATA_ENTRY *)((DWORD)rtab + (DWORD)rsrc->entries - (DWORD)rsrc->restab);
	for(i = 0; i < rsrc->nresources; i++)
	{
		rdep[i].OffsetToData += (DWORD)rsrc->restab - (DWORD)addr;
	}
    }

    /* Build the exports section data */

    exp->Name                  = ((BYTE *)descr->name) - addr;  /*??*/
    exp->Base                  = descr->base;
    exp->NumberOfFunctions     = descr->nb_funcs;
    exp->NumberOfNames         = descr->nb_names;
    exp->AddressOfFunctions    = (LPDWORD *)((BYTE *)funcs - addr);
    exp->AddressOfNames        = (LPDWORD *)((BYTE *)names - addr);
    exp->AddressOfNameOrdinals = (LPWORD *)((BYTE *)descr->ordinals - addr);

    /* Build the funcs table */

    for (i = 0; i < descr->nb_funcs; i++, funcs++, debug++)
    {
        BYTE args = descr->args[i];
	int j;

        if (!descr->functions[i]) continue;

        if (args == 0xfd)  /* forward func */
        {
            strcpy( pfwd, (LPSTR)descr->functions[i] );
            *funcs = (LPVOID)((BYTE *)pfwd - addr);
            pfwd += strlen(pfwd) + 1;
        }
        else *funcs = (LPVOID)((BYTE *)descr->functions[i] - addr);

#ifdef __i386__
	if (!(WARN_ON(relay) || TRACE_ON(relay))) continue;
	for (j=0;j<descr->nb_names;j++)
	    if (descr->ordinals[j] == i)
		break;
	if (j<descr->nb_names) {
	    if (descr->names[j]) {
	    	char buffer[200];
		sprintf(buffer,"%s.%d: %s",descr->name,i,descr->names[j]);
		if (!RELAY_ShowDebugmsgRelay(buffer))
		    continue;
	    }
	}
        switch(args)
        {
        case 0xfd:  /* forward */
        case 0xff:  /* stub or extern */
            break;
        default:  /* normal function (stdcall or cdecl or register) */
	    if (TRACE_ON(relay)) {
		debug->call       = 0xe8; /* lcall relative */
                if (args & 0x40)  /* register func */
                    debug->callfrom32 = (DWORD)RELAY_CallFrom32Regs -
                        (DWORD)&debug->ret;
                else
                    debug->callfrom32 = (DWORD)RELAY_CallFrom32 -
                        (DWORD)&debug->ret;
	    } else {
		debug->call       = 0xe9; /* ljmp relative */
		debug->callfrom32 = (DWORD)descr->functions[i] -
				    (DWORD)&debug->ret;
	    }
	    debug->ret        = (args & 0x80) ? 0xc3 : 0xc2; /*ret/ret $n*/
	    debug->args       = (args & 0x3f) * sizeof(int);
            *funcs = (LPVOID)((BYTE *)debug - addr);
            break;
        }
#endif  /* __i386__ */
    }

    /* Build the names table */

    for (i = 0; i < exp->NumberOfNames; i++, names++)
        if (descr->names[i])
            *names = (LPSTR)((BYTE *)descr->names[i] - addr);

    return (HMODULE)addr;
}

/***********************************************************************
 *           BUILTIN32_LoadLibraryExA
 *
 * Partly copied from the original PE_ version.
 *
 */
WINE_MODREF *BUILTIN32_LoadLibraryExA(LPCSTR path, DWORD flags)
{
    struct load_dll_request *req = get_req_buffer();
    HMODULE16      hModule16;
    NE_MODULE     *pModule;
    WINE_MODREF   *wm;
    char           dllname[MAX_PATH], *p;
    int i;

    /* Fix the name in case we have a full path and extension */
    if ((p = strrchr( path, '\\' ))) path = p + 1;
    lstrcpynA( dllname, path, sizeof(dllname) );

    p = strrchr( dllname, '.' );
    if (!p) strcat( dllname, ".dll" );

    /* Search built-in descriptor */
    for (i = 0; i < nb_dlls; i++)
        if (!lstrcmpiA( builtin_dlls[i]->filename, dllname )) break;

    if (i == nb_dlls)
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        return NULL;
    }

    /* Load built-in module */
    if (!dll_modules[i])
    {
        if (!(dll_modules[i] = BUILTIN32_DoLoadImage( builtin_dlls[i] ))) return NULL;
    }
    else BUILTIN32_WarnSecondInstance( builtin_dlls[i]->name );

    /* Create 16-bit dummy module */
    if ((hModule16 = MODULE_CreateDummyModule( dllname, 0 )) < 32)
    {
        SetLastError( (DWORD)hModule16 );
        return NULL;	/* FIXME: Should unload the builtin module */
    }

    pModule = (NE_MODULE *)GlobalLock16( hModule16 );
    pModule->flags = NE_FFLAGS_LIBMODULE | NE_FFLAGS_SINGLEDATA | NE_FFLAGS_WIN32 | NE_FFLAGS_BUILTIN;
    pModule->module32 = dll_modules[i];

    /* Create 32-bit MODREF */
    if ( !(wm = PE_CreateModule( pModule->module32, dllname, flags, TRUE )) )
    {
        ERR( "can't load %s\n", path );
        FreeLibrary16( hModule16 );	/* FIXME: Should unload the builtin module */
        SetLastError( ERROR_OUTOFMEMORY );
        return NULL;
    }

    if (wm->binfmt.pe.pe_export)
        SNOOP_RegisterDLL(wm->module,wm->modname,wm->binfmt.pe.pe_export->NumberOfFunctions);

    req->handle     = -1;
    req->base       = (void *)pModule->module32;
    req->dbg_offset = 0;
    req->dbg_size   = 0;
    req->name       = &wm->modname;
    server_call_noerr( REQ_LOAD_DLL );
    return wm;
}

/***********************************************************************
 *           BUILTIN32_LoadExeModule
 */
HMODULE16 BUILTIN32_LoadExeModule( void )
{
    HMODULE16 hModule16;
    NE_MODULE *pModule;
    int i, exe = -1;

    /* Search built-in EXE descriptor */
    for ( i = 0; i < nb_dlls; i++ )
        if ( !(builtin_dlls[i]->characteristics & IMAGE_FILE_DLL) ) 
        {
            if ( exe != -1 )
            {
                MESSAGE( "More than one built-in EXE module loaded!\n" );
                break;
            }

            exe = i;
        }

    if ( exe == -1 ) 
    {
        MESSAGE( "No built-in EXE module loaded!  Did you create a .spec file?\n" );
        return 0;
    }

    /* Load built-in module */
    if ( !dll_modules[exe] )
        if ( !(dll_modules[exe] = BUILTIN32_DoLoadImage( builtin_dlls[exe] )) )
            return 0;

    /* Create 16-bit dummy module */
    hModule16 = MODULE_CreateDummyModule( builtin_dlls[exe]->filename, 0 );
    if ( hModule16 < 32 ) return 0;
    pModule = (NE_MODULE *)GlobalLock16( hModule16 );
    pModule->flags = NE_FFLAGS_SINGLEDATA | NE_FFLAGS_WIN32 | NE_FFLAGS_BUILTIN;
    pModule->module32 = dll_modules[exe];

    return hModule16;
}


/***********************************************************************
 *	BUILTIN32_UnloadLibrary
 *
 * Unload the built-in library and free the modref.
 */
void BUILTIN32_UnloadLibrary(WINE_MODREF *wm)
{
	/* FIXME: do something here */
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
    const BUILTIN32_DESCRIPTOR *descr = NULL;
    int ordinal = 0, i;

    /* First find the module */

    for (i = 0; i < nb_dlls; i++)
        if (dll_modules[i])
        {
            IMAGE_SECTION_HEADER *sec = PE_SECTIONS(dll_modules[i]);
            DEBUG_ENTRY_POINT *debug = 
                 (DEBUG_ENTRY_POINT *)((DWORD)dll_modules[i] + sec[0].VirtualAddress);
            DEBUG_ENTRY_POINT *func = (DEBUG_ENTRY_POINT *)relay;
            descr = builtin_dlls[i];
            if (debug <= func && func < debug + descr->nb_funcs)
            {
                ordinal = func - debug;
                break;
            }
        }
    
    if (!descr) return NULL;

    /* Now find the function */

    for (i = 0; i < descr->nb_names; i++)
        if (descr->ordinals[i] == ordinal) break;

    sprintf( buffer, "%s.%d: %s", descr->name, ordinal + descr->base,
             (i < descr->nb_names) ? descr->names[i] : "@" );
    *typemask = descr->argtypes[ordinal];
    return descr->functions[ordinal];
}

/***********************************************************************
 *           BUILTIN32_SwitchRelayDebug
 *
 * FIXME: enhance to do it module relative.
 */
void BUILTIN32_SwitchRelayDebug(BOOL onoff)
{
    const BUILTIN32_DESCRIPTOR *descr;
    IMAGE_SECTION_HEADER *sec;
    DEBUG_ENTRY_POINT *debug;
    int i, j;

#ifdef __i386__
    if (!(TRACE_ON(relay) || WARN_ON(relay)))
    	return;
    for (j = 0; j < nb_dlls; j++)
    {
        if (!dll_modules[j]) continue;
	sec = PE_SECTIONS(dll_modules[j]);
	debug = (DEBUG_ENTRY_POINT *)((DWORD)dll_modules[j] + sec[1].VirtualAddress);
        descr = builtin_dlls[j];
	for (i = 0; i < descr->nb_funcs; i++,debug++) {
	    if (!descr->functions[i]) continue;
	    if ((descr->args[i]==0xff) || (descr->args[i]==0xfe))
	    	continue;
	    if (onoff) {
                debug->call       = 0xe8; /* lcall relative */
                debug->callfrom32 = (DWORD)RELAY_CallFrom32 -
                                    (DWORD)&debug->ret;
	    } else {
                debug->call       = 0xe9; /* ljmp relative */
                debug->callfrom32 = (DWORD)descr->functions[i] -
                                    (DWORD)&debug->ret;
	    }
        }
    }
#endif /* __i386__ */
    return;
}

/***********************************************************************
 *           BUILTIN32_RegisterDLL
 *
 * Register a built-in DLL descriptor.
 */
void BUILTIN32_RegisterDLL( const BUILTIN32_DESCRIPTOR *descr )
{
    assert( nb_dlls < MAX_DLLS );
    builtin_dlls[nb_dlls++] = descr;
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

    MESSAGE( "No handler for Win32 routine %s.%d: %s",
             descr->name, ordinal, func_name );
#ifdef __GNUC__
    MESSAGE( " (called from %p)", __builtin_return_address(1) );
#endif
    MESSAGE( "\n" );
    ExitProcess(1);
}
