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
#include "wine/library.h"
#include "elfdll.h"
#include "global.h"
#include "neexe.h"
#include "heap.h"
#include "main.h"
#include "winerror.h"
#include "server.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(module);
DECLARE_DEBUG_CHANNEL(relay);

extern void RELAY_SetupDLL( const char *module );

static HMODULE main_module;

/***********************************************************************
 *           BUILTIN32_dlopen
 */
void *BUILTIN32_dlopen( const char *name )
{
#ifdef HAVE_DL_API
    void *handle;

    if (!(handle = wine_dll_load( name )))
    {
        char buffer[128];
	LPSTR pErr, p;
	pErr = dlerror();
	p = strchr(pErr, ':');
	if ((p) && 
	   (!strncmp(p, ": undefined symbol", 18))) /* undef symbol -> ERR() */
	    ERR("failed to load %s: %s\n", buffer, pErr);
	else /* WARN() for libraries that are supposed to be native */
            WARN("failed to load %s: %s\n", buffer, pErr );
    }
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
 *           fixup_resources
 */
static void fixup_resources( IMAGE_RESOURCE_DIRECTORY *dir, char *root, void *base )
{
    IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    int i;

    entry = (IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    for (i = 0; i < dir->NumberOfNamedEntries + dir->NumberOfIdEntries; i++, entry++)
    {
        void *ptr = root + entry->u2.s.OffsetToDirectory;
        if (entry->u2.s.DataIsDirectory) fixup_resources( ptr, root, base );
        else
        {
            IMAGE_RESOURCE_DATA_ENTRY *data = ptr;
            fixup_rva_ptrs( &data->OffsetToData, base, 1 );
        }
    }
}


/***********************************************************************
 *           load_image
 *
 * Load a built-in Win32 module. Helper function for load_library.
 */
static HMODULE load_image( const IMAGE_NT_HEADERS *nt_descr, const char *filename )
{
    IMAGE_DATA_DIRECTORY *dir;
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sec;
    INT size, nb_sections;
    BYTE *addr, *code_start, *data_start;
    int page_size = VIRTUAL_GetPageSize();

    /* Allocate the module */

    nb_sections = 2;  /* code + data */

    size = (sizeof(IMAGE_DOS_HEADER)
            + sizeof(IMAGE_NT_HEADERS)
            + nb_sections * sizeof(IMAGE_SECTION_HEADER));

    assert( size <= page_size );

    if (nt_descr->OptionalHeader.ImageBase)
    {
        void *base = (void *)nt_descr->OptionalHeader.ImageBase;
        if ((addr = wine_anon_mmap( base, page_size, PROT_READ|PROT_WRITE, MAP_FIXED )) != base)
        {
            ERR("failed to map over PE header for %s at %p\n", filename, base );
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
    code_start = addr + page_size;

    /* HACK! */
    data_start = code_start + page_size;

    /* Build the DOS and NT headers */

    dos->e_magic  = IMAGE_DOS_SIGNATURE;
    dos->e_lfanew = sizeof(*dos);

    *nt = *nt_descr;

    nt->FileHeader.NumberOfSections                = nb_sections;
    nt->OptionalHeader.SizeOfCode                  = data_start - code_start;
    nt->OptionalHeader.SizeOfInitializedData       = 0;
    nt->OptionalHeader.SizeOfUninitializedData     = 0;
    nt->OptionalHeader.ImageBase                   = (DWORD)addr;

    fixup_rva_ptrs( &nt->OptionalHeader.AddressOfEntryPoint, addr, 1 );

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

    dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_IMPORT_DIRECTORY];
    if (dir->Size)
    {
        IMAGE_IMPORT_DESCRIPTOR *imports = (void *)dir->VirtualAddress;
        fixup_rva_ptrs( &dir->VirtualAddress, addr, 1 );
        /* we can fixup everything at once since we only have pointers and 0 values */
        fixup_rva_ptrs( imports, addr, dir->Size / sizeof(void*) );
    }

    /* Build the resource directory */

    dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_RESOURCE_DIRECTORY];
    if (dir->Size)
    {
        void *ptr = (void *)dir->VirtualAddress;
        fixup_rva_ptrs( &dir->VirtualAddress, addr, 1 );
        fixup_resources( ptr, ptr, addr );
    }

    /* Build the export directory */

    dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_EXPORT_DIRECTORY];
    if (dir->Size)
    {
        IMAGE_EXPORT_DIRECTORY *exports = (void *)dir->VirtualAddress;
        fixup_rva_ptrs( &dir->VirtualAddress, addr, 1 );
        fixup_rva_ptrs( (void *)exports->AddressOfFunctions, addr, exports->NumberOfFunctions );
        fixup_rva_ptrs( (void *)exports->AddressOfNames, addr, exports->NumberOfNames );
        fixup_rva_ptrs( &exports->Name, addr, 1 );
        fixup_rva_ptrs( &exports->AddressOfFunctions, addr, 1 );
        fixup_rva_ptrs( &exports->AddressOfNames, addr, 1 );
        fixup_rva_ptrs( &exports->AddressOfNameOrdinals, addr, 1 );

        /* Setup relay debugging entry points */
        if (TRACE_ON(relay)) RELAY_SetupDLL( addr );
    }

    return (HMODULE)addr;
}


/***********************************************************************
 *           load_library
 *
 * Load a library in memory; callback function for wine_dll_register
 */
static void load_library( const IMAGE_NT_HEADERS *nt, const char *filename )
{
    HMODULE module;
    WINE_MODREF *wm;

    if (!(nt->FileHeader.Characteristics & IMAGE_FILE_DLL))
    {
        /* if we already have an executable, ignore this one */
        if (!main_module) main_module = load_image( nt, "main exe" );
        return; /* don't create the modref here, will be done later on */
    }

    if (GetModuleHandleA( filename ))
        MESSAGE( "Warning: loading builtin %s, but native version already present. Expect trouble.\n", filename );

    /* Load built-in module */
    if (!(module = load_image( nt, filename ))) return;

    /* Create 32-bit MODREF */
    if (!(wm = PE_CreateModule( module, filename, 0, -1, TRUE )))
    {
        ERR( "can't load %s\n", filename );
        SetLastError( ERROR_OUTOFMEMORY );
        return;
    }
    TRACE( "loaded %s %p %x %p\n", filename, wm, module, nt );
    wm->refCount++;  /* we don't support freeing builtin dlls (FIXME)*/
}


/***********************************************************************
 *           BUILTIN32_LoadLibraryExA
 *
 * Partly copied from the original PE_ version.
 *
 */
WINE_MODREF *BUILTIN32_LoadLibraryExA(LPCSTR path, DWORD flags)
{
    WINE_MODREF   *wm;
    char dllname[20], *p;
    LPCSTR name;
    void *handle;

    /* Fix the name in case we have a full path and extension */
    name = path;
    if ((p = strrchr( name, '\\' ))) name = p + 1;
    if ((p = strrchr( name, '/' ))) name = p + 1;

    if (strlen(name) >= sizeof(dllname)-4) goto error;

    strcpy( dllname, name );
    p = strrchr( dllname, '.' );
    if (!p) strcat( dllname, ".dll" );

    if (!(handle = BUILTIN32_dlopen( dllname ))) goto error;

    if (!(wm = MODULE_FindModule( path ))) wm = MODULE_FindModule( dllname );
    if (!wm)
    {
        ERR( "loaded .so but dll %s still not found\n", dllname );
        /* wine_dll_unload( handle );*/
        return NULL;
    }
    wm->dlhandle = handle;
    return wm;

 error:
    SetLastError( ERROR_FILE_NOT_FOUND );
    return NULL;
}

/***********************************************************************
 *           BUILTIN32_Init
 *
 * Initialize loading callbacks and return HMODULE of main exe.
 * 'main' is the main exe in case if was already loaded from a PE file.
 */
HMODULE BUILTIN32_LoadExeModule( HMODULE main )
{
    main_module = main;
    wine_dll_set_callback( load_library );
    if (!main_module)
        MESSAGE( "No built-in EXE module loaded!  Did you create a .spec file?\n" );
    return main_module;
}


/***********************************************************************
 *           BUILTIN32_RegisterDLL
 *
 * Register a built-in DLL descriptor.
 */
void BUILTIN32_RegisterDLL( const IMAGE_NT_HEADERS *header, const char *filename )
{
    extern void __wine_dll_register( const IMAGE_NT_HEADERS *header, const char *filename );
    __wine_dll_register( header, filename );
}
