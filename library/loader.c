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
#include <unistd.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "winnt.h"
#include "wine/library.h"

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


/* open a library for a given dll, searching in the dll path
 * 'name' must be the Windows dll name (e.g. "kernel32.dll") */
static void *dlopen_dll( const char *name, char *error, int errorsize )
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
        if ((ret = wine_dlopen( p, RTLD_NOW, error, errorsize ))) break;
    }

    /* now try the default dlopen search path */
    if (!ret)
        ret = wine_dlopen( buffer + dll_path_maxlen + 1, RTLD_NOW, error, errorsize );
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

    if (nt_descr->OptionalHeader.ImageBase)
    {
        addr = wine_anon_mmap( (void *)nt_descr->OptionalHeader.ImageBase,
                               page_size, PROT_READ|PROT_WRITE, MAP_FIXED );
        if (addr != (BYTE *)nt_descr->OptionalHeader.ImageBase) return NULL;
    }
    else
    {
        /* this will leak memory; but it should never happen */
        addr = wine_anon_mmap( NULL, page_size, PROT_READ|PROT_WRITE, 0 );
        if (addr == (BYTE *)-1) return NULL;
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
    }
    return addr;
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
    return dlopen_dll( filename, error, errorsize );
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
 * Try to load the .so for the main exe, optionally searching for it in PATH.
 */
void *wine_dll_load_main_exe( const char *name, int search_path, char *error, int errorsize )
{
    void *ret = NULL;
    const char *path = NULL;
    if (search_path) path = getenv( "PATH" );

    if (!path)
    {
        /* no path, try only the specified name */
        ret = wine_dlopen( name, RTLD_NOW, error, errorsize );
    }
    else
    {
        char buffer[128], *tmp = buffer;
        size_t namelen = strlen(name);
        size_t pathlen = strlen(path);

        if (namelen + pathlen + 2 > sizeof(buffer)) tmp = malloc( namelen + pathlen + 2 );
        if (tmp)
        {
            char *basename = tmp + pathlen;
            *basename = '/';
            strcpy( basename + 1, name );
            for (;;)
            {
                int len;
                const char *p = strchr( path, ':' );
                if (!p) p = path + strlen(path);
                if ((len = p - path) > 0)
                {
                    memcpy( basename - len, path, len );
                    if ((ret = wine_dlopen( basename - len, RTLD_NOW, error, errorsize ))) break;
                }
                if (!*p) break;
                path = p + 1;
            }
            if (tmp != buffer) free( tmp );
        }
    }
    return ret;
}
