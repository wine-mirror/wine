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
#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif
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
static const unsigned int max_dll_name_len = 64;

static void *file_buffer;
static SIZE_T file_buffer_size;
static unsigned int handled_count;
static unsigned int handled_total;
static char **handled_dlls;

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

/* convert a dll name W->A without depending on the current codepage */
static char *dll_name_WtoA( char *nameA, const WCHAR *nameW, unsigned int len )
{
    unsigned int i;

    for (i = 0; i < len; i++)
    {
        char c = nameW[i];
        if (nameW[i] > 127) return NULL;
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        nameA[i] = c;
    }
    nameA[i] = 0;
    return nameA;
}

/* convert a dll name A->W without depending on the current codepage */
static WCHAR *dll_name_AtoW( WCHAR *nameW, const char *nameA, unsigned int len )
{
    unsigned int i;

    for (i = 0; i < len; i++) nameW[i] = nameA[i];
    nameW[i] = 0;
    return nameW;
}

/* add a dll to the list of dll that have been taken care of */
static BOOL add_handled_dll( const WCHAR *name )
{
    unsigned int len = strlenW( name );
    int i, min, max, pos, res;
    char *nameA;

    if (!(nameA = HeapAlloc( GetProcessHeap(), 0, len + 1 ))) return FALSE;
    if (!dll_name_WtoA( nameA, name, len )) goto failed;

    min = 0;
    max = handled_count - 1;
    while (min <= max)
    {
        pos = (min + max) / 2;
        res = strcmp( handled_dlls[pos], nameA );
        if (!res) goto failed;  /* already in the list */
        if (res < 0) min = pos + 1;
        else max = pos - 1;
    }

    if (handled_count >= handled_total)
    {
        char **new_dlls;
        unsigned int new_count = max( 64, handled_total * 2 );

        if (handled_dlls) new_dlls = HeapReAlloc( GetProcessHeap(), 0, handled_dlls,
                                                  new_count * sizeof(*handled_dlls) );
        else new_dlls = HeapAlloc( GetProcessHeap(), 0, new_count * sizeof(*handled_dlls) );
        if (!new_dlls) goto failed;
        handled_dlls = new_dlls;
        handled_total = new_count;
    }

    for (i = handled_count; i > min; i--) handled_dlls[i] = handled_dlls[i - 1];
    handled_dlls[i] = nameA;
    handled_count++;
    return TRUE;

failed:
    HeapFree( GetProcessHeap(), 0, nameA );
    return FALSE;
}

/* read in the contents of a file into the global file buffer */
/* return 1 on success, 0 on nonexistent file, -1 on other error */
static int read_file( const char *name, void **data, SIZE_T *size )
{
    struct stat st;
    int fd, ret = -1;
    size_t header_size;
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    const size_t min_size = sizeof(*dos) + sizeof(fakedll_signature) +
        FIELD_OFFSET( IMAGE_NT_HEADERS, OptionalHeader.MajorLinkerVersion );

    if ((fd = open( name, O_RDONLY | O_BINARY )) == -1) return 0;
    if (fstat( fd, &st ) == -1) goto done;
    *size = st.st_size;
    if (!file_buffer || st.st_size > file_buffer_size)
    {
        VirtualFree( file_buffer, 0, MEM_RELEASE );
        file_buffer = NULL;
        file_buffer_size = st.st_size;
        if (NtAllocateVirtualMemory( GetCurrentProcess(), &file_buffer, 0, &file_buffer_size,
                                     MEM_COMMIT, PAGE_READWRITE )) goto done;
    }

    /* check for valid fake dll file */

    if (st.st_size < min_size) goto done;
    header_size = min( st.st_size, 4096 );
    if (pread( fd, file_buffer, header_size, 0 ) != header_size) goto done;
    dos = file_buffer;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) goto done;
    if (dos->e_lfanew < sizeof(fakedll_signature)) goto done;
    if (memcmp( dos + 1, fakedll_signature, sizeof(fakedll_signature) )) goto done;
    if (dos->e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS,OptionalHeader.MajorLinkerVersion) > header_size)
        goto done;
    nt = (IMAGE_NT_HEADERS *)((char *)file_buffer + dos->e_lfanew);
    if (nt->Signature == IMAGE_NT_SIGNATURE && nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
    {
        /* wrong 32/64 type, pretend it doesn't exist */
        ret = 0;
        goto done;
    }
    if (st.st_size == header_size ||
        pread( fd, (char *)file_buffer + header_size,
               st.st_size - header_size, header_size ) == st.st_size - header_size)
    {
        *data = file_buffer;
        ret = 1;
    }
done:
    close( fd );
    return ret;
}

/* build a complete fake dll from scratch */
static BOOL build_fake_dll( HANDLE file )
{
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
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
    /* some fields are copied from the source dll */
#ifdef _WIN64
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
#else
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
#endif
    nt->FileHeader.TimeDateStamp = 0;
    nt->FileHeader.Characteristics = IMAGE_FILE_DLL;
    nt->OptionalHeader.MajorLinkerVersion = 1;
    nt->OptionalHeader.MinorLinkerVersion = 0;
    nt->OptionalHeader.MajorOperatingSystemVersion = 1;
    nt->OptionalHeader.MinorOperatingSystemVersion = 0;
    nt->OptionalHeader.MajorImageVersion = 1;
    nt->OptionalHeader.MinorImageVersion = 0;
    nt->OptionalHeader.MajorSubsystemVersion = 4;
    nt->OptionalHeader.MinorSubsystemVersion = 0;
    nt->OptionalHeader.Win32VersionValue = 0;
    nt->OptionalHeader.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
    nt->OptionalHeader.DllCharacteristics = 0;
    nt->OptionalHeader.SizeOfStackReserve = 0;
    nt->OptionalHeader.SizeOfStackCommit = 0;
    nt->OptionalHeader.SizeOfHeapReserve = 0;
    nt->OptionalHeader.SizeOfHeapCommit = 0;
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
static void *load_fake_dll( const WCHAR *name, SIZE_T *size )
{
    const char *build_dir = wine_get_build_dir();
    const char *path;
    char *file, *ptr;
    void *data = NULL;
    unsigned int i, pos, len, namelen, maxlen = 0;
    WCHAR *p;
    int res = 0;

    if ((p = strrchrW( name, '\\' ))) name = p + 1;

    i = 0;
    if (build_dir) maxlen = strlen(build_dir) + sizeof("/programs/") + strlenW(name);
    while ((path = wine_dll_enum_load_path( i++ ))) maxlen = max( maxlen, strlen(path) );
    maxlen += sizeof("/fakedlls") + strlenW(name) + 2;

    if (!(file = HeapAlloc( GetProcessHeap(), 0, maxlen ))) return NULL;

    len = strlenW( name );
    pos = maxlen - len - sizeof(".fake");
    if (!dll_name_WtoA( file + pos, name, len )) goto done;
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
        if ((res = read_file( ptr, &data, size ))) goto done;

        /* now as a program */
        ptr = file + pos;
        namelen = len + 1;
        if (namelen > 4 && !memcmp( ptr + namelen - 4, ".exe", 4 )) namelen -= 4;
        ptr = prepend( ptr, ptr, namelen );
        ptr = prepend( ptr, "/programs", sizeof("/programs") - 1 );
        ptr = prepend( ptr, build_dir, strlen(build_dir) );
        if ((res = read_file( ptr, &data, size ))) goto done;
    }

    file[pos + len + 1] = 0;
    for (i = 0; (path = wine_dll_enum_load_path( i )); i++)
    {
        ptr = prepend( file + pos, "/fakedlls", sizeof("/fakedlls") - 1 );
        ptr = prepend( ptr, path, strlen(path) );
        if ((res = read_file( ptr, &data, size ))) break;
    }

done:
    HeapFree( GetProcessHeap(), 0, file );
    if (res == 1) return data;
    return NULL;
}

/* create the fake dll destination file */
static HANDLE create_dest_file( const WCHAR *name )
{
    /* first check for an existing file */
    HANDLE h = CreateFileW( name, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
    if (h != INVALID_HANDLE_VALUE)
    {
        if (!is_fake_dll( h ))
        {
            TRACE( "%s is not a fake dll, not overwriting it\n", debugstr_w(name) );
            CloseHandle( h );
            return 0;
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
            ERR( "failed to create %s (error=%u)\n", debugstr_w(name), GetLastError() );
    }
    return h;
}

/* copy a fake dll file to the dest directory */
static void install_fake_dll( WCHAR *dest, char *file, const char *ext )
{
    int ret;
    SIZE_T size;
    void *data;
    DWORD written;
    WCHAR *destname = dest + strlenW(dest);
    char *name = strrchr( file, '/' ) + 1;
    char *end = name + strlen(name);

    if (ext) strcpy( end, ext );
    if (!(ret = read_file( file, &data, &size ))) return;

    if (end > name + 2 && !strncmp( end - 2, "16", 2 )) end -= 2;  /* remove "16" suffix */
    dll_name_AtoW( destname, name, end - name );
    if (!add_handled_dll( destname )) ret = -1;

    if (ret != -1)
    {
        HANDLE h = create_dest_file( dest );

        if (h && h != INVALID_HANDLE_VALUE)
        {
            TRACE( "%s -> %s\n", debugstr_a(file), debugstr_w(dest) );

            ret = (WriteFile( h, data, size, &written, NULL ) && written == size);
            if (!ret) ERR( "failed to write to %s (error=%u)\n", debugstr_w(dest), GetLastError() );
            CloseHandle( h );
            if (!ret) DeleteFileW( dest );
        }
    }
    *destname = 0;  /* restore it for next file */
}

/* find and install all fake dlls in a given lib directory */
static void install_lib_dir( WCHAR *dest, char *file, const char *default_ext )
{
    DIR *dir;
    struct dirent *de;
    char *name;

    if (!(dir = opendir( file ))) return;
    name = file + strlen(file);
    *name++ = '/';
    while ((de = readdir( dir )))
    {
        if (strlen( de->d_name ) > max_dll_name_len) continue;
        if (!strcmp( de->d_name, "." )) continue;
        if (!strcmp( de->d_name, ".." )) continue;
        strcpy( name, de->d_name );
        if (default_ext)  /* inside build dir */
        {
            strcat( name, "/" );
            strcat( name, de->d_name );
            if (!strchr( de->d_name, '.' )) strcat( name, default_ext );
            install_fake_dll( dest, file, ".fake" );
        }
        else install_fake_dll( dest, file, NULL );
    }
    closedir( dir );
}

/* create fake dlls in dirname for all the files we can find */
static BOOL create_wildcard_dlls( const WCHAR *dirname )
{
    const char *build_dir = wine_get_build_dir();
    const char *path;
    unsigned int i, maxlen = 0;
    char *file;
    WCHAR *dest;

    if (build_dir) maxlen = strlen(build_dir) + sizeof("/programs/");
    for (i = 0; (path = wine_dll_enum_load_path(i)); i++) maxlen = max( maxlen, strlen(path) );
    maxlen += 2 * max_dll_name_len + 2 + sizeof(".dll.fake");
    if (!(file = HeapAlloc( GetProcessHeap(), 0, maxlen ))) return FALSE;

    if (!(dest = HeapAlloc( GetProcessHeap(), 0, (strlenW(dirname) + max_dll_name_len) * sizeof(WCHAR) )))
    {
        HeapFree( GetProcessHeap(), 0, file );
        return FALSE;
    }
    strcpyW( dest, dirname );
    dest[strlenW(dest) - 1] = 0;  /* remove wildcard */

    if (build_dir)
    {
        strcpy( file, build_dir );
        strcat( file, "/dlls" );
        install_lib_dir( dest, file, ".dll" );
        strcpy( file, build_dir );
        strcat( file, "/programs" );
        install_lib_dir( dest, file, ".exe" );
    }
    for (i = 0; (path = wine_dll_enum_load_path( i )); i++)
    {
        strcpy( file, path );
        strcat( file, "/fakedlls" );
        install_lib_dir( dest, file, NULL );
    }
    HeapFree( GetProcessHeap(), 0, file );
    HeapFree( GetProcessHeap(), 0, dest );
    return TRUE;
}

/***********************************************************************
 *            create_fake_dll
 */
BOOL create_fake_dll( const WCHAR *name, const WCHAR *source )
{
    HANDLE h;
    BOOL ret;
    SIZE_T size;
    const WCHAR *filename;
    void *buffer;

    if (!(filename = strrchrW( name, '\\' ))) filename = name;
    else filename++;

    /* check for empty name which means to only create the directory */
    if (!filename[0])
    {
        create_directories( name );
        return TRUE;
    }
    if (filename[0] == '*' && !filename[1]) return create_wildcard_dlls( name );

    add_handled_dll( filename );

    if (!(h = create_dest_file( name ))) return TRUE;  /* not a fake dll */
    if (h == INVALID_HANDLE_VALUE) return FALSE;

    if (source[0] == '-' && !source[1])
    {
        /* '-' source means delete the file */
        TRACE( "deleting %s\n", debugstr_w(name) );
        ret = FALSE;
    }
    else if ((buffer = load_fake_dll( source, &size )))
    {
        DWORD written;

        ret = (WriteFile( h, buffer, size, &written, NULL ) && written == size);
        if (!ret) ERR( "failed to write to %s (error=%u)\n", debugstr_w(name), GetLastError() );
    }
    else
    {
        WARN( "fake dll %s not found for %s\n", debugstr_w(source), debugstr_w(name) );
        ret = build_fake_dll( h );
    }

    CloseHandle( h );
    if (!ret) DeleteFileW( name );
    return ret;
}


/***********************************************************************
 *            cleanup_fake_dlls
 */
void cleanup_fake_dlls(void)
{
    if (file_buffer) VirtualFree( file_buffer, 0, MEM_RELEASE );
    file_buffer = NULL;
    HeapFree( GetProcessHeap(), 0, handled_dlls );
    handled_dlls = NULL;
    handled_count = handled_total = 0;
}
