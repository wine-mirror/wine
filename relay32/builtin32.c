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

typedef struct
{
    BYTE			*restab;
    DWORD			nresources;
    DWORD			restabsize;
    IMAGE_RESOURCE_DATA_ENTRY	*entries;
} BUILTIN32_RESOURCE;

#define MAX_DLLS 100

typedef struct
{
    const IMAGE_NT_HEADERS *nt;           /* NT header */
    const char             *filename;     /* DLL file name */
} BUILTIN32_DESCRIPTOR;

extern void RELAY_SetupDLL( const char *module );

static BUILTIN32_DESCRIPTOR builtin_dlls[MAX_DLLS];
static int nb_dlls;


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
    {
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
    INT i, size, nb_sections;
    BYTE *addr, *code_start, *data_start;
    int page_size = VIRTUAL_GetPageSize();

    /* Allocate the module */

    nb_sections = 2;  /* code + data */

    size = (sizeof(IMAGE_DOS_HEADER)
            + sizeof(IMAGE_NT_HEADERS)
            + nb_sections * sizeof(IMAGE_SECTION_HEADER));

    assert( size <= page_size );

    if (descr->nt->OptionalHeader.ImageBase)
    {
        void *base = (void *)descr->nt->OptionalHeader.ImageBase;
        if ((addr = VIRTUAL_mmap( -1, base, page_size, 0,
                                  PROT_READ|PROT_WRITE, MAP_FIXED )) != base)
        {
            ERR("failed to map over PE header for %s at %p\n", descr->filename, base );
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

    *nt = *descr->nt;

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
    if (dir->VirtualAddress)
    {
        BUILTIN32_RESOURCE *rsrc = (BUILTIN32_RESOURCE *)dir->VirtualAddress;
        IMAGE_RESOURCE_DATA_ENTRY *rdep = rsrc->entries;
        dir->VirtualAddress = (BYTE *)rsrc->restab - addr;
        dir->Size = rsrc->restabsize;
        for (i = 0; i < rsrc->nresources; i++) rdep[i].OffsetToData += dir->VirtualAddress;
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
    HMODULE module;
    WINE_MODREF   *wm;
    char dllname[20], *p;
    LPCSTR name;
    void *handle;
    int i;

    /* Fix the name in case we have a full path and extension */
    name = path;
    if ((p = strrchr( name, '\\' ))) name = p + 1;
    if ((p = strrchr( name, '/' ))) name = p + 1;

    if (strlen(name) >= sizeof(dllname)-4) goto error;

    strcpy( dllname, name );
    p = strrchr( dllname, '.' );
    if (!p) strcat( dllname, ".dll" );

    /* Search built-in descriptor */
    for (i = 0; i < nb_dlls; i++)
        if (!strcasecmp( builtin_dlls[i].filename, dllname )) goto found;

    if ((handle = BUILTIN32_dlopen( dllname )))
    {
        for (i = 0; i < nb_dlls; i++)
            if (!strcasecmp( builtin_dlls[i].filename, dllname )) goto found;
        ERR( "loaded .so but dll %s still not found\n", dllname );
        BUILTIN32_dlclose( handle );
    }

 error:
    SetLastError( ERROR_FILE_NOT_FOUND );
    return NULL;

 found:
    /* Load built-in module */
    if (!(module = BUILTIN32_DoLoadImage( &builtin_dlls[i] ))) return NULL;

    /* Create 32-bit MODREF */
    if ( !(wm = PE_CreateModule( module, path, flags, -1, TRUE )) )
    {
        ERR( "can't load %s\n", path );
        SetLastError( ERROR_OUTOFMEMORY );
        return NULL;
    }

    wm->refCount++;  /* we don't support freeing builtin dlls (FIXME)*/
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
        if ( !(builtin_dlls[i].nt->FileHeader.Characteristics & IMAGE_FILE_DLL) )
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
    return BUILTIN32_DoLoadImage( &builtin_dlls[exe] );
}


/***********************************************************************
 *           BUILTIN32_RegisterDLL
 *
 * Register a built-in DLL descriptor.
 */
void BUILTIN32_RegisterDLL( const IMAGE_NT_HEADERS *header, const char *filename )
{
    assert( nb_dlls < MAX_DLLS );
    builtin_dlls[nb_dlls].nt = header;
    builtin_dlls[nb_dlls].filename = filename;
    nb_dlls++;
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

    MESSAGE( "FATAL: No handler for Win32 routine %s.%s", dllname, funcname );
#ifdef __GNUC__
    MESSAGE( " (called from %p)", __builtin_return_address(1) );
#endif
    MESSAGE( "\n" );
    ExitProcess(1);
}
