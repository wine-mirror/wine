/*
 * Creation of Wine fake dlls for apps that access the dll file directly.
 *
 * Copyright 2006 Alexandre Julliard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <fcntl.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/unicode.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

static const char fakedll_signature[] = "Wine placeholder DLL";

static const unsigned int file_alignment = 512;
static const unsigned int section_alignment = 4096;

struct dll_info
{
    HANDLE            handle;
    IMAGE_NT_HEADERS *nt;
    DWORD             file_pos;
    DWORD             mem_pos;
};

#define ALIGN(size,align) (((size) + (align) - 1) & ~((align) - 1))

/* contents of the dll sections */

static const BYTE dll_code_section[] = { 0x31, 0xc0,          /* xor %eax,%eax */
                                         0xc2, 0x0c, 0x00 };  /* ret $12 */

static const BYTE exe_code_section[] = { 0xb8, 0x01, 0x00, 0x00, 0x00,  /* movl $1,%eax */
                                         0xc2, 0x04, 0x00 };            /* ret $4 */

static const IMAGE_BASE_RELOCATION reloc_section;  /* empty relocs */


/* wrapper for WriteFile */
static inline BOOL xwrite( struct dll_info *info, const void *data, DWORD size, DWORD offset )
{
    DWORD res;

    return (SetFilePointer( info->handle, offset, NULL, FILE_BEGIN ) != INVALID_SET_FILE_POINTER &&
            WriteFile( info->handle, data, size, &res, NULL ) &&
            res == size);
}

/* add a new section to the dll NT header */
static void add_section( struct dll_info *info, const char *name, DWORD size, DWORD flags )
{
    IMAGE_SECTION_HEADER *sec = (IMAGE_SECTION_HEADER *)(info->nt + 1);

    sec += info->nt->FileHeader.NumberOfSections;
    memcpy( sec->Name, name, min( strlen(name), sizeof(sec->Name)) );
    sec->Misc.VirtualSize = ALIGN( size, section_alignment );
    sec->VirtualAddress   = info->mem_pos;
    sec->SizeOfRawData    = size;
    sec->PointerToRawData = info->file_pos;
    sec->Characteristics  = flags;
    info->file_pos += ALIGN( size, file_alignment );
    info->mem_pos  += ALIGN( size, section_alignment );
    info->nt->FileHeader.NumberOfSections++;
}

/* add a data directory to the dll NT header */
static inline void add_directory( struct dll_info *info, unsigned int idx, DWORD rva, DWORD size )
{
    info->nt->OptionalHeader.DataDirectory[idx].VirtualAddress = rva;
    info->nt->OptionalHeader.DataDirectory[idx].Size = size;
}

/* find the limits of the resource data */
static void get_resource_data( const IMAGE_RESOURCE_DIRECTORY *dir, const BYTE *root,
                               DWORD *rva_start, DWORD *rva_end )
{
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    int i;

    entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    for (i = 0; i < dir->NumberOfNamedEntries + dir->NumberOfIdEntries; i++, entry++)
    {
        const void *ptr = root + entry->u2.s3.OffsetToDirectory;
        if (entry->u2.s3.DataIsDirectory) get_resource_data( ptr, root, rva_start, rva_end );
        else
        {
            const IMAGE_RESOURCE_DATA_ENTRY *data = ptr;
            *rva_start = min( *rva_start, data->OffsetToData );
            *rva_end = max( *rva_end, data->OffsetToData + data->Size );
        }
    }
}

/* fixup RVAs of resource data */
static void fixup_resources( IMAGE_RESOURCE_DIRECTORY *dir, BYTE *root, int delta )
{
    IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    int i;

    entry = (IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    for (i = 0; i < dir->NumberOfNamedEntries + dir->NumberOfIdEntries; i++, entry++)
    {
        void *ptr = root + entry->u2.s3.OffsetToDirectory;
        if (entry->u2.s3.DataIsDirectory) fixup_resources( ptr, root, delta );
        else
        {
            IMAGE_RESOURCE_DATA_ENTRY *data = ptr;
            if (data->OffsetToData) data->OffsetToData += delta;
        }
    }
}

/* add version resources to the dll by copying them from the source module */
static BOOL add_version_resource( HMODULE module, struct dll_info *dll_info )
{
    BOOL ret = FALSE;
    DWORD rva_start = ~0U, rva_end = 0, dir_size, total_size;
    const IMAGE_RESOURCE_DIRECTORY *basedir;
    const BYTE *data_start, *root;
    BYTE *buffer;

    if (!module) return TRUE;
    if (LdrFindResourceDirectory_U( module, NULL, 0, &basedir ) != STATUS_SUCCESS) return TRUE;
    root = (const BYTE *)basedir;
    get_resource_data( basedir, root, &rva_start, &rva_end );
    data_start = (const BYTE *)module + rva_start;
    if (data_start <= root) return FALSE;
    dir_size = data_start - root;
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, dir_size ))) return FALSE;
    memcpy( buffer, root, dir_size );
    fixup_resources( (IMAGE_RESOURCE_DIRECTORY *)buffer, buffer, dll_info->mem_pos + dir_size - rva_start );
    if (!xwrite( dll_info, buffer, dir_size, dll_info->file_pos )) goto done;
    if (!xwrite( dll_info, data_start, rva_end - rva_start, dll_info->file_pos + dir_size )) goto done;
    total_size = dir_size + rva_end - rva_start;
    add_directory( dll_info, IMAGE_DIRECTORY_ENTRY_RESOURCE, dll_info->mem_pos, total_size );
    add_section( dll_info, ".rsrc", total_size, IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ );
    ret = TRUE;
done:
    HeapFree( GetProcessHeap(), 0, buffer );
    return ret;
}

/* build a complete fake dll, optionally using module as a source */
static BOOL build_fake_dll( HANDLE file, HMODULE module )
{
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    const IMAGE_NT_HEADERS *src_nt = NULL;
    struct dll_info info;
    BYTE *buffer;
    BOOL ret = FALSE;
    DWORD lfanew = (sizeof(*dos) + sizeof(fakedll_signature) + 15) & ~15;
    DWORD size, header_size = lfanew + sizeof(*nt);

    info.handle = file;
    buffer = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, header_size + 8 * sizeof(IMAGE_SECTION_HEADER) );

    dos = (IMAGE_DOS_HEADER *)buffer;
    dos->e_magic    = IMAGE_DOS_SIGNATURE;
    dos->e_cblp     = sizeof(*dos);
    dos->e_cp       = 1;
    dos->e_cparhdr  = lfanew / 16;
    dos->e_minalloc = 0;
    dos->e_maxalloc = 0xffff;
    dos->e_ss       = 0x0000;
    dos->e_sp       = 0x00b8;
    dos->e_lfarlc   = lfanew;
    dos->e_lfanew   = lfanew;
    memcpy( dos + 1, fakedll_signature, sizeof(fakedll_signature) );

    nt = info.nt = (IMAGE_NT_HEADERS *)(buffer + lfanew);
    if (module) src_nt = RtlImageNtHeader( module );
    /* some fields are copied from the source dll */
#define SET(field,def) nt->field = src_nt ? src_nt->field : def
    SET( FileHeader.Machine, IMAGE_FILE_MACHINE_I386 );
    SET( FileHeader.TimeDateStamp, 0 );
    SET( FileHeader.Characteristics, IMAGE_FILE_DLL );
    SET( OptionalHeader.MajorLinkerVersion, 1 );
    SET( OptionalHeader.MinorLinkerVersion, 0 );
    SET( OptionalHeader.MajorOperatingSystemVersion, 1 );
    SET( OptionalHeader.MinorOperatingSystemVersion, 0 );
    SET( OptionalHeader.MajorImageVersion, 1 );
    SET( OptionalHeader.MinorImageVersion, 0 );
    SET( OptionalHeader.MajorSubsystemVersion, 4 );
    SET( OptionalHeader.MinorSubsystemVersion, 0 );
    SET( OptionalHeader.Win32VersionValue, 0 );
    SET( OptionalHeader.Subsystem, IMAGE_SUBSYSTEM_WINDOWS_GUI );
    SET( OptionalHeader.DllCharacteristics, 0 );
    SET( OptionalHeader.SizeOfStackReserve, 0 );
    SET( OptionalHeader.SizeOfStackCommit, 0 );
    SET( OptionalHeader.SizeOfHeapReserve, 0 );
    SET( OptionalHeader.SizeOfHeapCommit, 0 );
#undef SET
    /* other fields have fixed values */
    nt->Signature                              = IMAGE_NT_SIGNATURE;
    nt->FileHeader.NumberOfSections            = 0;
    nt->FileHeader.SizeOfOptionalHeader        = IMAGE_SIZEOF_NT_OPTIONAL_HEADER;
    nt->OptionalHeader.Magic                   = IMAGE_NT_OPTIONAL_HDR_MAGIC;
    nt->OptionalHeader.ImageBase               = 0x10000000;
    nt->OptionalHeader.SectionAlignment        = section_alignment;
    nt->OptionalHeader.FileAlignment           = file_alignment;
    nt->OptionalHeader.NumberOfRvaAndSizes     = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;

    header_size = (BYTE *)(nt + 1) - buffer;
    info.mem_pos  = ALIGN( header_size, section_alignment );
    info.file_pos = ALIGN( header_size, file_alignment );

    nt->OptionalHeader.AddressOfEntryPoint = info.mem_pos;
    nt->OptionalHeader.BaseOfCode          = info.mem_pos;

    if (nt->FileHeader.Characteristics & IMAGE_FILE_DLL)
    {
        size = sizeof(dll_code_section);
        if (!xwrite( &info, dll_code_section, size, info.file_pos )) goto done;
    }
    else
    {
        size = sizeof(exe_code_section);
        if (!xwrite( &info, exe_code_section, size, info.file_pos )) goto done;
    }
    nt->OptionalHeader.SizeOfCode = size;
    add_section( &info, ".text", size, IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ );

    if (!xwrite( &info, &reloc_section, sizeof(reloc_section), info.file_pos )) goto done;
    add_directory( &info, IMAGE_DIRECTORY_ENTRY_BASERELOC, info.mem_pos, sizeof(reloc_section) );
    add_section( &info, ".reloc", sizeof(reloc_section),
                 IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_MEM_READ );

    if (!add_version_resource( module, &info )) goto done;

    header_size += nt->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
    nt->OptionalHeader.SizeOfHeaders = ALIGN( header_size, file_alignment );
    nt->OptionalHeader.SizeOfImage   = ALIGN( info.mem_pos, section_alignment );
    ret = xwrite( &info, buffer, header_size, 0 );
done:
    HeapFree( GetProcessHeap(), 0, buffer );
    return ret;
}

/* check if an existing file is a fake dll so that we can overwrite it */
static BOOL is_fake_dll( HANDLE h )
{
    IMAGE_DOS_HEADER *dos;
    DWORD size;
    BYTE buffer[sizeof(*dos) + sizeof(fakedll_signature)];

    if (!ReadFile( h, buffer, sizeof(buffer), &size, NULL ) || size != sizeof(buffer))
        return FALSE;
    dos = (IMAGE_DOS_HEADER *)buffer;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return FALSE;
    if (dos->e_lfanew < size) return FALSE;
    return !memcmp( dos + 1, fakedll_signature, sizeof(fakedll_signature) );
}

/* create directories leading to a given file */
static void create_directories( const WCHAR *name )
{
    WCHAR *path, *p;

    /* create the directory/directories */
    path = HeapAlloc(GetProcessHeap(), 0, (strlenW(name) + 1)*sizeof(WCHAR));
    strcpyW(path, name);

    p = strchrW(path, '\\');
    while (p != NULL)
    {
        *p = 0;
        if (!CreateDirectoryW(path, NULL))
            TRACE("Couldn't create directory %s - error: %d\n", wine_dbgstr_w(path), GetLastError());
        *p = '\\';
        p = strchrW(p+1, '\\');
    }
    HeapFree(GetProcessHeap(), 0, path);
}

static inline char *prepend( char *buffer, const char *str, size_t len )
{
    return memcpy( buffer - len, str, len );
}

/* try to load a pre-compiled fake dll */
static void *load_fake_dll( const WCHAR *name, unsigned int *size, void *buf, unsigned int buf_size )
{
    const char *build_dir = wine_get_build_dir();
    const char *path;
    struct stat st;
    char *file, *ptr, *buffer = NULL;
    unsigned int i, pos, len, namelen, maxlen = 0;
    WCHAR *p;
    int fd;

    if ((p = strrchrW( name, '\\' ))) name = p + 1;

    i = 0;
    if (build_dir) maxlen = strlen(build_dir) + sizeof("/programs/") + strlenW(name);
    while ((path = wine_dll_enum_load_path( i++ ))) maxlen = max( maxlen, strlen(path) );
    maxlen += sizeof("/fakedlls") + strlenW(name) + 2;

    if (!(file = HeapAlloc( GetProcessHeap(), 0, maxlen ))) return NULL;

    len = strlenW( name );
    pos = maxlen - len - sizeof(".fake");
    for (i = 0; i < len; i++)
    {
        /* we don't want to depend on the current codepage here */
        char c = name[i];
        if (name[i] > 127) goto done;
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        file[pos + i] = c;
    }
    file[--pos] = '/';

    if (build_dir)
    {
        strcpy( file + pos + len + 1, ".fake" );

        /* try as a dll */
        ptr = file + pos;
        namelen = len + 1;
        if (namelen > 4 && !memcmp( ptr + namelen - 4, ".dll", 4 )) namelen -= 4;
        ptr = prepend( ptr, ptr, namelen );
        ptr = prepend( ptr, "/dlls", sizeof("/dlls") - 1 );
        ptr = prepend( ptr, build_dir, strlen(build_dir) );
        if ((fd = open( ptr, O_RDONLY | O_BINARY )) != -1) goto found;

        /* now as a program */
        ptr = file + pos;
        namelen = len + 1;
        if (namelen > 4 && !memcmp( ptr + namelen - 4, ".exe", 4 )) namelen -= 4;
        ptr = prepend( ptr, ptr, namelen );
        ptr = prepend( ptr, "/programs", sizeof("/programs") - 1 );
        ptr = prepend( ptr, build_dir, strlen(build_dir) );
        if ((fd = open( ptr, O_RDONLY | O_BINARY )) != -1) goto found;
    }

    file[pos + len + 1] = 0;
    for (i = 0; (path = wine_dll_enum_load_path( i )); i++)
    {
        ptr = prepend( file + pos, "/fakedlls", sizeof("/fakedlls") - 1 );
        ptr = prepend( ptr, path, strlen(path) );
        if ((fd = open( ptr, O_RDONLY | O_BINARY )) != -1) goto found;
    }

found:
    if (!fstat( fd, &st ))
    {
        *size = st.st_size;
        if (st.st_size <= buf_size) buffer = buf;
        else buffer = HeapAlloc( GetProcessHeap(), 0, st.st_size );
        if (buffer)
        {
            if (read( fd, buffer, st.st_size ) != st.st_size)
            {
                if (buffer != buf) HeapFree( GetProcessHeap(), 0, buffer );
                buffer = NULL;
            }
            else TRACE( "found %s %u bytes\n", ptr, *size);
        }
    }
    close( fd );

done:
    HeapFree( GetProcessHeap(), 0, file );
    return buffer;
}

/***********************************************************************
 *            create_fake_dll
 */
BOOL create_fake_dll( const WCHAR *name, const WCHAR *source )
{
    HANDLE h;
    BOOL ret;
    unsigned int size = 0;
    char *temp_buffer[4096];
    void *buffer;

    /* check for empty name which means to only create the directory */
    if (name[strlenW(name) - 1] == '\\')
    {
        create_directories( name );
        return TRUE;
    }

    /* first check for an existing file */
    h = CreateFileW( name, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
    if (h != INVALID_HANDLE_VALUE)
    {
        if (!is_fake_dll( h ))
        {
            TRACE( "%s is not a fake dll, not overwriting it\n", debugstr_w(name) );
            CloseHandle( h );
            return TRUE;
        }
        /* truncate the file */
        SetFilePointer( h, 0, NULL, FILE_BEGIN );
        SetEndOfFile( h );
    }
    else
    {
        if (GetLastError() == ERROR_PATH_NOT_FOUND) create_directories( name );

        h = CreateFileW( name, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL );
        if (h == INVALID_HANDLE_VALUE)
        {
            ERR( "failed to create %s (error=%u)\n", debugstr_w(name), GetLastError() );
            return FALSE;
        }
    }

    if ((buffer = load_fake_dll( source, &size, temp_buffer, sizeof(temp_buffer) )))
    {
        DWORD written;

        ret = (WriteFile( h, buffer, size, &written, NULL ) && written == size);
        if (!ret) ERR( "failed to write to %s (error=%u)\n", debugstr_w(name), GetLastError() );
        if (buffer != temp_buffer) HeapFree( GetProcessHeap(), 0, buffer );
    }
    else
    {
        HMODULE module = LoadLibraryW( source );
        WARN( "fake dll %s not found for %s\n", debugstr_w(source), debugstr_w(name) );
        ret = build_fake_dll( h, module );
        if (module) FreeLibrary( module );
    }

    CloseHandle( h );
    if (!ret) DeleteFileW( name );
    return ret;
}
