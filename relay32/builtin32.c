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
#include "wingdi.h"
#include "winuser.h"
#include "builtin32.h"
#include "elfdll.h"
#include "file.h"
#include "global.h"
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
    BYTE			*restab;
    DWORD			nresources;
    DWORD			restabsize;
    IMAGE_RESOURCE_DATA_ENTRY	*entries;
} BUILTIN32_RESOURCE;

#define MAX_DLLS 60

static const BUILTIN32_DESCRIPTOR *builtin_dlls[MAX_DLLS];
static HMODULE dll_modules[MAX_DLLS];
static int nb_dlls;


/***********************************************************************
 *           BUILTIN32_WarnSecondInstance
 *
 * Emit a warning when we are creating a second instance for a DLL
 * that is known to not support this.
 */
static void BUILTIN32_WarnSecondInstance( const char *name )
{
    static const char * const warning_list[] =
    { "comctl32.dll", "comdlg32.dll", "crtdll.dll",
      "imagehlp.dll", "msacm32.dll", "shell32.dll", NULL };

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
 *           BUILTIN32_dlopen
 */
void *BUILTIN32_dlopen( const char *name )
{
#ifdef HAVE_DL_API
    void *handle;
    char buffer[128], *p;
    if ((p = strrchr( name, '/' ))) name = p + 1;
    if ((p = strrchr( name, '\\' ))) name = p + 1;
    sprintf( buffer, "lib%s", name );
    for (p = buffer; *p; p++) *p = tolower(*p);
    if ((p = strrchr( buffer, '.' )) && (!strcmp( p, ".dll" ) || !strcmp( p, ".exe" ))) *p = 0;
    strcat( buffer, ".so" );

    if (!(handle = ELFDLL_dlopen( buffer, RTLD_NOW )))
        ERR( "failed to load %s: %s\n", buffer, dlerror() );
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
 *           fixup_rva_ptrs
 *
 * Adjust an array of pointers to make them into RVAs.
 */
static inline void fixup_rva_ptrs( void *array, void *base, int count )
{
    void **ptr = (void **)array;
    while (count--)
    {
        if (*ptr) *ptr = (void *)((char *)*ptr - (char *)base);
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
    IMAGE_IMPORT_DESCRIPTOR *imp;
    IMAGE_EXPORT_DIRECTORY *exports = descr->exports;
    INT i, size, nb_sections;
    BYTE *addr, *code_start, *data_start;
    int page_size = VIRTUAL_GetPageSize();

    /* Allocate the module */

    nb_sections = 2;  /* code + data */

    size = (sizeof(IMAGE_DOS_HEADER)
            + sizeof(IMAGE_NT_HEADERS)
            + nb_sections * sizeof(IMAGE_SECTION_HEADER)
            + (descr->nb_imports+1) * sizeof(IMAGE_IMPORT_DESCRIPTOR));

    assert( size <= page_size );

    if (descr->pe_header)
    {
        if ((addr = FILE_dommap( -1, descr->pe_header, 0, page_size, 0, 0,
                                 PROT_READ|PROT_WRITE, MAP_FIXED )) != descr->pe_header)
        {
            ERR("failed to map over PE header for %s at %p\n", descr->filename, descr->pe_header );
            return 0;
        }
    }
    else
    {
        if (!(addr = VirtualAlloc( NULL, page_size, MEM_COMMIT, PAGE_READWRITE ))) return 0;
    }

    dos    = (IMAGE_DOS_HEADER *)addr;
    nt     = (IMAGE_NT_HEADERS *)(dos + 1);
    sec    = (IMAGE_SECTION_HEADER *)(nt + 1);
    imp    = (IMAGE_IMPORT_DESCRIPTOR *)(sec + nb_sections);
    code_start = addr + page_size;

    /* HACK! */
    data_start = code_start + page_size;

    /* Build the DOS and NT headers */

    dos->e_magic  = IMAGE_DOS_SIGNATURE;
    dos->e_lfanew = sizeof(*dos);

    nt->Signature                       = IMAGE_NT_SIGNATURE;
    nt->FileHeader.Machine              = IMAGE_FILE_MACHINE_I386;
    nt->FileHeader.NumberOfSections     = nb_sections;
    nt->FileHeader.SizeOfOptionalHeader = sizeof(nt->OptionalHeader);
    nt->FileHeader.Characteristics      = descr->characteristics;

    nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR_MAGIC;
    nt->OptionalHeader.SizeOfCode                  = data_start - code_start;
    nt->OptionalHeader.SizeOfInitializedData       = 0;
    nt->OptionalHeader.SizeOfUninitializedData     = 0;
    nt->OptionalHeader.ImageBase                   = (DWORD)addr;
    nt->OptionalHeader.SectionAlignment            = page_size;
    nt->OptionalHeader.FileAlignment               = page_size;
    nt->OptionalHeader.MajorOperatingSystemVersion = 1;
    nt->OptionalHeader.MinorOperatingSystemVersion = 0;
    nt->OptionalHeader.MajorSubsystemVersion       = 4;
    nt->OptionalHeader.MinorSubsystemVersion       = 0;
    nt->OptionalHeader.SizeOfImage                 = page_size;
    nt->OptionalHeader.SizeOfHeaders               = page_size;
    nt->OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    if (descr->dllentrypoint) 
        nt->OptionalHeader.AddressOfEntryPoint = (DWORD)descr->dllentrypoint - (DWORD)addr;
    
    /* Build the code section */

    strcpy( sec->Name, ".text" );
    sec->SizeOfRawData = data_start - code_start;
    sec->Misc.VirtualSize = sec->SizeOfRawData;
    sec->VirtualAddress   = code_start - addr;
    sec->PointerToRawData = code_start - addr;
    sec->Characteristics  = (IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ);
    sec++;

    /* Build the data section */

    strcpy( sec->Name, ".data" );
    sec->SizeOfRawData = 0;
    sec->Misc.VirtualSize = sec->SizeOfRawData;
    sec->VirtualAddress   = data_start - addr;
    sec->PointerToRawData = data_start - addr;
    sec->Characteristics  = (IMAGE_SCN_CNT_INITIALIZED_DATA |
                             IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ);
    sec++;

    /* Build the import directory */

    if (descr->nb_imports)
    {
        dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_IMPORT_DIRECTORY];
        dir->VirtualAddress = (BYTE *)imp - addr;
        dir->Size = sizeof(*imp) * (descr->nb_imports + 1);

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

    /* Build the resource directory */

    if (descr->rsrc)
    {
        BUILTIN32_RESOURCE *rsrc = descr->rsrc;
	IMAGE_RESOURCE_DATA_ENTRY *rdep;
	dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_RESOURCE_DIRECTORY];
	dir->VirtualAddress = (BYTE *)rsrc->restab - addr;
	dir->Size = rsrc->restabsize;
	rdep = rsrc->entries;
	for (i = 0; i < rsrc->nresources; i++) rdep[i].OffsetToData += dir->VirtualAddress;
    }

    /* Build the export directory */

    if (exports)
    {
        dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_EXPORT_DIRECTORY];
        dir->VirtualAddress = (BYTE *)exports - addr;
        dir->Size = descr->exports_size;

        fixup_rva_ptrs( (void *)exports->AddressOfFunctions, addr, exports->NumberOfFunctions );
        fixup_rva_ptrs( (void *)exports->AddressOfNames, addr, exports->NumberOfNames );
        fixup_rva_ptrs( &exports->Name, addr, 1 );
        fixup_rva_ptrs( &exports->AddressOfFunctions, addr, 1 );
        fixup_rva_ptrs( &exports->AddressOfNames, addr, 1 );
        fixup_rva_ptrs( &exports->AddressOfNameOrdinals, addr, 1 );

        /* Setup relay debugging entry points */
        if (WARN_ON(relay) || TRACE_ON(relay)) RELAY_SetupDLL( addr );
    }

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
    void *handle;
    int i;

    /* Fix the name in case we have a full path and extension */
    if ((p = strrchr( path, '\\' ))) path = p + 1;
    lstrcpynA( dllname, path, sizeof(dllname) );

    p = strrchr( dllname, '.' );
    if (!p) strcat( dllname, ".dll" );

    /* Search built-in descriptor */
    for (i = 0; i < nb_dlls; i++)
        if (!strcasecmp( builtin_dlls[i]->filename, dllname )) goto found;

    if ((handle = BUILTIN32_dlopen( dllname )))
    {
        for (i = 0; i < nb_dlls; i++)
            if (!strcasecmp( builtin_dlls[i]->filename, dllname )) goto found;
        ERR( "loaded .so but dll %s still not found\n", dllname );
        BUILTIN32_dlclose( handle );
    }

    SetLastError( ERROR_FILE_NOT_FOUND );
    return NULL;

 found:
    /* Load built-in module */
    if (!dll_modules[i])
    {
        if (!(dll_modules[i] = BUILTIN32_DoLoadImage( builtin_dlls[i] ))) return NULL;
    }
    else BUILTIN32_WarnSecondInstance( builtin_dlls[i]->filename );

    /* Create 16-bit dummy module */
    if ((hModule16 = MODULE_CreateDummyModule( dllname, dll_modules[i] )) < 32)
    {
        SetLastError( (DWORD)hModule16 );
        return NULL;	/* FIXME: Should unload the builtin module */
    }
    pModule = (NE_MODULE *)GlobalLock16( hModule16 );
    pModule->flags = NE_FFLAGS_LIBMODULE | NE_FFLAGS_SINGLEDATA | NE_FFLAGS_WIN32 | NE_FFLAGS_BUILTIN;

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
HMODULE BUILTIN32_LoadExeModule(void)
{
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
    return dll_modules[exe];
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
void BUILTIN32_Unimplemented( const char *dllname, const char *funcname )
{
    __RESTORE_ES;  /* Just in case */

    MESSAGE( "No handler for Win32 routine %s.%s", dllname, funcname );
#ifdef __GNUC__
    MESSAGE( " (called from %p)", __builtin_return_address(1) );
#endif
    MESSAGE( "\n" );
    ExitProcess(1);
}
