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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winver.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/unicode.h"
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
    memcpy( (char *)sec->Name, name, min( strlen(name), sizeof(sec->Name)) );
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

/* add version resources to the dll by copying them from the source module */
static BOOL add_version_resource( HMODULE module, struct dll_info *dll_info )
{
    int i;
    BOOL ret = FALSE;
    DWORD size, pos;
    BYTE *buffer;
    LDR_RESOURCE_INFO info;
    IMAGE_RESOURCE_DIRECTORY *dir;
    IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    IMAGE_RESOURCE_DATA_ENTRY *data;
    const IMAGE_RESOURCE_DIRECTORY *basedir, *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *src_entry;
    const IMAGE_RESOURCE_DATA_ENTRY *src_data;

    if (!module) return TRUE;
    if (LdrFindResourceDirectory_U( module, NULL, 0, &basedir ) != STATUS_SUCCESS) return TRUE;
    info.Type = VS_FILE_INFO;
    info.Name = VS_VERSION_INFO;
    if (LdrFindResourceDirectory_U( module, &info, 2, &resdir ) != STATUS_SUCCESS) return TRUE;

    size = 3 * sizeof(IMAGE_RESOURCE_DIRECTORY);
    size += (resdir->NumberOfIdEntries + 2) * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY);
    size += resdir->NumberOfIdEntries * sizeof(IMAGE_RESOURCE_DATA_ENTRY);

    buffer = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size );

    /* types directory */
    dir = (IMAGE_RESOURCE_DIRECTORY *)buffer;
    dir->NumberOfIdEntries = 1;
    entry = (IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    entry->u1.s2.Id = info.Type;
    entry->u2.s3.DataIsDirectory = 1;
    entry->u2.s3.OffsetToDirectory = (BYTE *)(entry + 1) - buffer;

    /* names directory */
    dir = (IMAGE_RESOURCE_DIRECTORY *)(entry + 1);
    dir->NumberOfIdEntries = 1;
    entry = (IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    entry->u1.s2.Id = info.Name;
    entry->u2.s3.DataIsDirectory = 1;
    entry->u2.s3.OffsetToDirectory = (BYTE *)(entry + 1) - buffer;

    /* languages directory */
    dir = (IMAGE_RESOURCE_DIRECTORY *)(entry + 1);
    *dir = *resdir;
    entry = (IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    src_entry = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    memcpy( entry, src_entry, resdir->NumberOfIdEntries * sizeof(*entry) );

    /* data entries */
    data = (IMAGE_RESOURCE_DATA_ENTRY *)(entry + resdir->NumberOfIdEntries);
    pos = size;
    for (i = 0; i < resdir->NumberOfIdEntries; i++)
    {
        void *ptr;
        ULONG data_size;
        src_data = (const IMAGE_RESOURCE_DATA_ENTRY *)((const char *)basedir + src_entry[i].u2.OffsetToData);
        if (LdrAccessResource( module, src_data, &ptr, &data_size )) goto done;

        entry[i].u2.OffsetToData = (BYTE *)&data[i] - buffer;
        data[i] = *src_data;
        data[i].OffsetToData = dll_info->mem_pos + pos;
        if (!xwrite( dll_info, ptr, data_size, dll_info->file_pos + pos )) goto done;
        pos += (data_size + 3) & ~3;
    }
    if (!xwrite( dll_info, buffer, size, dll_info->file_pos )) goto done;

    add_directory( dll_info, IMAGE_DIRECTORY_ENTRY_RESOURCE, dll_info->mem_pos, pos );
    add_section( dll_info, ".rsrc", pos, IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ );
    ret = TRUE;
done:
    HeapFree( GetProcessHeap(), 0, buffer );
    return ret;
}

void dummy(void)
{
    __asm__ __volatile__("movl $1,%eax; ret $4");
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
    src_nt = RtlImageNtHeader( module );
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

/***********************************************************************
 *            create_fake_dll
 */
BOOL create_fake_dll( const WCHAR *name, const WCHAR *source )
{
    HANDLE h;
    HMODULE module;
    BOOL ret;

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
        h = CreateFileW( name, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL );
        if (h == INVALID_HANDLE_VALUE)
        {
            WARN( "failed to create %s\n", debugstr_w(name) );
            return FALSE;
        }
    }

    module = LoadLibraryW( source );

    ret = build_fake_dll( h, module );

    CloseHandle( h );
    if (module) FreeLibrary( module );
    if (!ret) DeleteFileW( name );
    return ret;
}
