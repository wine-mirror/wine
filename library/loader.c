/*
 * Win32 builtin dlls support
 *
 * Copyright 2000 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "wine/library.h"

/* argc/argv for the Windows application */
int __wine_main_argc = 0;
char **__wine_main_argv = NULL;
WCHAR **__wine_main_wargv = NULL;

#define MAX_DLLS 100

static struct
{
    const IMAGE_NT_HEADERS *nt;           /* NT header */
    const char             *filename;     /* DLL file name */
} builtin_dlls[MAX_DLLS];

static int nb_dlls;

static const IMAGE_NT_HEADERS *main_exe;

static load_dll_callback_t load_dll_callback;

static const char **dll_paths;
static int nb_dll_paths;
static int dll_path_maxlen;
static int init_done;


/* build the dll load path from the WINEDLLPATH variable */
static void build_dll_path(void)
{
    static const char * const dlldir = DLLDIR;
    int len, count = 0;
    char *p, *path = getenv( "WINEDLLPATH" );

    init_done = 1;

    if (path)
    {
        /* count how many path elements we need */
        path = strdup(path);
        p = path;
        while (*p)
        {
            while (*p == ':') p++;
            if (!*p) break;
            count++;
            while (*p && *p != ':') p++;
        }
    }

    dll_paths = malloc( (count+1) * sizeof(*dll_paths) );

    if (count)
    {
        p = path;
        nb_dll_paths = 0;
        while (*p)
        {
            while (*p == ':') *p++ = 0;
            if (!*p) break;
            dll_paths[nb_dll_paths] = p;
            while (*p && *p != ':') p++;
            if (p - dll_paths[nb_dll_paths] > dll_path_maxlen)
                dll_path_maxlen = p - dll_paths[nb_dll_paths];
            nb_dll_paths++;
        }
    }

    /* append default dll dir (if not empty) to path */
    if ((len = strlen(dlldir)))
    {
        if (len > dll_path_maxlen) dll_path_maxlen = len;
        dll_paths[nb_dll_paths++] = dlldir;
    }
}

/* check if a given file can be opened */
inline static int file_exists( const char *name )
{
    int fd = open( name, O_RDONLY );
    if (fd != -1) close( fd );
    return (fd != -1);
}

/* open a library for a given dll, searching in the dll path
 * 'name' must be the Windows dll name (e.g. "kernel32.dll") */
static void *dlopen_dll( const char *name, char *error, int errorsize, int test_only )
{
    int i, namelen = strlen(name);
    char *buffer, *p;
    void *ret = NULL;

    if (!init_done) build_dll_path();

    buffer = malloc( dll_path_maxlen + namelen + 5 );

    /* store the name at the end of the buffer, followed by .so */
    p = buffer + dll_path_maxlen;
    *p++ = '/';
    memcpy( p, name, namelen );
    strcpy( p + namelen, ".so" );

    for (i = 0; i < nb_dll_paths; i++)
    {
        int len = strlen(dll_paths[i]);
        p = buffer + dll_path_maxlen - len;
        memcpy( p, dll_paths[i], len );
        if (test_only)  /* just test for file existence */
        {
            if ((ret = (void *)file_exists( p ))) break;
        }
        else
        {
            if ((ret = wine_dlopen( p, RTLD_NOW, error, errorsize ))) break;
            if (file_exists( p )) break; /* exists but cannot be loaded, return the error */
        }
    }
    free( buffer );
    return ret;
}


/* adjust an array of pointers to make them into RVAs */
static inline void fixup_rva_ptrs( void *array, void *base, int count )
{
    void **ptr = (void **)array;
    while (count--)
    {
        if (*ptr) *ptr = (void *)((char *)*ptr - (char *)base);
        ptr++;
    }
}


/* fixup RVAs in the import directory */
static void fixup_imports( IMAGE_IMPORT_DESCRIPTOR *dir, DWORD size, void *base )
{
    int count = size / sizeof(void *);
    void **ptr = (void **)dir;

    /* everything is either a pointer or a ordinal value below 0x10000 */
    while (count--)
    {
        if (*ptr >= (void *)0x10000) *ptr = (void *)((char *)*ptr - (char *)base);
        else if (*ptr) *ptr = (void *)(0x80000000 | (unsigned int)*ptr);
        ptr++;
    }
}


/* fixup RVAs in the resource directory */
static void fixup_resources( IMAGE_RESOURCE_DIRECTORY *dir, char *root, void *base )
{
    IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    int i;

    entry = (IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    for (i = 0; i < dir->NumberOfNamedEntries + dir->NumberOfIdEntries; i++, entry++)
    {
        void *ptr = root + entry->u2.s3.OffsetToDirectory;
        if (entry->u2.s3.DataIsDirectory) fixup_resources( ptr, root, base );
        else
        {
            IMAGE_RESOURCE_DATA_ENTRY *data = ptr;
            fixup_rva_ptrs( &data->OffsetToData, base, 1 );
        }
    }
}


/* map a builtin dll in memory and fixup RVAs */
static void *map_dll( const IMAGE_NT_HEADERS *nt_descr )
{
#ifdef HAVE_MMAP
    IMAGE_DATA_DIRECTORY *dir;
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sec;
    BYTE *addr, *code_start, *data_start;
    size_t page_size = getpagesize();
    int nb_sections = 2;  /* code + data */

    size_t size = (sizeof(IMAGE_DOS_HEADER)
                   + sizeof(IMAGE_NT_HEADERS)
                   + nb_sections * sizeof(IMAGE_SECTION_HEADER));

    assert( size <= page_size );

    /* module address must be aligned on 64K boundary */
    addr = (BYTE *)((nt_descr->OptionalHeader.ImageBase + 0xffff) & ~0xffff);
    if (wine_anon_mmap( addr, page_size, PROT_READ|PROT_WRITE, MAP_FIXED ) != addr) return NULL;

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
        fixup_imports( imports, dir->Size, addr );
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
    }
    return addr;
#else  /* HAVE_MMAP */
    return NULL;
#endif  /* HAVE_MMAP */
}


/***********************************************************************
 *           __wine_dll_register
 *
 * Register a built-in DLL descriptor.
 */
void __wine_dll_register( const IMAGE_NT_HEADERS *header, const char *filename )
{
    if (load_dll_callback) load_dll_callback( map_dll(header), filename );
    else
    {
        if (!(header->FileHeader.Characteristics & IMAGE_FILE_DLL))
            main_exe = header;
        else
        {
            assert( nb_dlls < MAX_DLLS );
            builtin_dlls[nb_dlls].nt = header;
            builtin_dlls[nb_dlls].filename = filename;
            nb_dlls++;
        }
    }
}


/***********************************************************************
 *           wine_dll_set_callback
 *
 * Set the callback function for dll loading, and call it
 * for all dlls that were implicitly loaded already.
 */
void wine_dll_set_callback( load_dll_callback_t load )
{
    int i;
    load_dll_callback = load;
    for (i = 0; i < nb_dlls; i++)
    {
        const IMAGE_NT_HEADERS *nt = builtin_dlls[i].nt;
        if (!nt) continue;
        builtin_dlls[i].nt = NULL;
        load_dll_callback( map_dll(nt), builtin_dlls[i].filename );
    }
    nb_dlls = 0;
    if (main_exe) load_dll_callback( map_dll(main_exe), "" );
}


/***********************************************************************
 *           wine_dll_load
 *
 * Load a builtin dll.
 */
void *wine_dll_load( const char *filename, char *error, int errorsize )
{
    int i;

    /* callback must have been set already */
    assert( load_dll_callback );

    /* check if we have it in the list */
    /* this can happen when initializing pre-loaded dlls in wine_dll_set_callback */
    for (i = 0; i < nb_dlls; i++)
    {
        if (!builtin_dlls[i].nt) continue;
        if (!strcmp( builtin_dlls[i].filename, filename ))
        {
            const IMAGE_NT_HEADERS *nt = builtin_dlls[i].nt;
            builtin_dlls[i].nt = NULL;
            load_dll_callback( map_dll(nt), builtin_dlls[i].filename );
            return (void *)1;
        }
    }
    return dlopen_dll( filename, error, errorsize, 0 );
}


/***********************************************************************
 *           wine_dll_unload
 *
 * Unload a builtin dll.
 */
void wine_dll_unload( void *handle )
{
    if (handle != (void *)1)
	wine_dlclose( handle, NULL, 0 );
}


/***********************************************************************
 *           wine_dll_load_main_exe
 *
 * Try to load the .so for the main exe.
 */
void *wine_dll_load_main_exe( const char *name, char *error, int errorsize, int test_only )
{
    return dlopen_dll( name, error, errorsize, test_only );
}


/*
 * These functions provide wrappers around dlopen() and associated
 * functions.  They work around a bug in glibc 2.1.x where calling
 * a dl*() function after a previous dl*() function has failed
 * without a dlerror() call between the two will cause a crash.
 * They all take a pointer to a buffer that
 * will receive the error description (from dlerror()).  This
 * parameter may be NULL if the error description is not required.
 */

/***********************************************************************
 *		wine_dlopen
 */
void *wine_dlopen( const char *filename, int flag, char *error, int errorsize )
{
#ifdef HAVE_DLOPEN
    void *ret;
    const char *s;
    dlerror(); dlerror();
    ret = dlopen( filename, flag );
    s = dlerror();
    if (error)
    {
        strncpy( error, s ? s : "", errorsize );
        error[errorsize - 1] = '\0';
    }
    dlerror();
    return ret;
#else
    if (error)
    {
        strncpy( error, "dlopen interface not detected by configure", errorsize );
        error[errorsize - 1] = '\0';
    }
    return NULL;
#endif
}

/***********************************************************************
 *		wine_dlsym
 */
void *wine_dlsym( void *handle, const char *symbol, char *error, int errorsize )
{
#ifdef HAVE_DLOPEN
    void *ret;
    const char *s;
    dlerror(); dlerror();
    ret = dlsym( handle, symbol );
    s = dlerror();
    if (error)
    {
        strncpy( error, s ? s : "", errorsize );
        error[errorsize - 1] = '\0';
    }
    dlerror();
    return ret;
#else
    if (error)
    {
        strncpy( error, "dlopen interface not detected by configure", errorsize );
        error[errorsize - 1] = '\0';
    }
    return NULL;
#endif
}

/***********************************************************************
 *		wine_dlclose
 */
int wine_dlclose( void *handle, char *error, int errorsize )
{
#ifdef HAVE_DLOPEN
    int ret;
    const char *s;
    dlerror(); dlerror();
    ret = dlclose( handle );
    s = dlerror();
    if (error)
    {
        strncpy( error, s ? s : "", errorsize );
        error[errorsize - 1] = '\0';
    }
    dlerror();
    return ret;
#else
    if (error)
    {
        strncpy( error, "dlopen interface not detected by configure", errorsize );
        error[errorsize - 1] = '\0';
    }
    return 1;
#endif
}
