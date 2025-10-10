/*
 * Creation of Wine fake dlls for apps that access the dll file directly.
 *
 * Copyright 2006, 2011 Alexandre Julliard
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

#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define COBJMACROS
#define ATL_INITGUID

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "ole2.h"
#include "atliface.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

#ifdef __i386__
static const WCHAR pe_dir[] = L"\\i386-windows";
static const char current_arch[] = "x86";
#elif defined __x86_64__
static const WCHAR pe_dir[] = L"\\x86_64-windows";
static const char current_arch[] = "amd64";
#elif defined __arm__
static const WCHAR pe_dir[] = L"\\arm-windows";
static const char current_arch[] = "arm";
#elif defined __aarch64__
static const WCHAR pe_dir[] = L"\\aarch64-windows";
static const char current_arch[] = "arm64";
#else
static const WCHAR pe_dir[] = L"";
static const char current_arch[] = "none";
#endif

static const char builtin_signature[] = "Wine builtin DLL";
static const char fakedll_signature[] = "Wine placeholder DLL";

static const unsigned int file_alignment = 512;
static const unsigned int section_alignment = 4096;
static const unsigned int max_dll_name_len = 64;

static void *file_buffer;
static SIZE_T file_buffer_size;
static unsigned int handled_count;
static unsigned int handled_total;
static WCHAR **handled_dlls;
static IRegistrar *registrar;

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

/* add a dll to the list of dll that have been taken care of */
static BOOL add_handled_dll( const WCHAR *name )
{
    int i, min, max, pos, res;

    min = 0;
    max = handled_count - 1;
    while (min <= max)
    {
        pos = (min + max) / 2;
        res = wcscmp( handled_dlls[pos], name );
        if (!res) return FALSE;  /* already in the list */
        if (res < 0) min = pos + 1;
        else max = pos - 1;
    }

    if (handled_count >= handled_total)
    {
        WCHAR **new_dlls;
        unsigned int new_count = max( 64, handled_total * 2 );

        new_dlls = realloc( handled_dlls, new_count * sizeof(*handled_dlls) );
        if (!new_dlls) return FALSE;
        handled_dlls = new_dlls;
        handled_total = new_count;
    }

    for (i = handled_count; i > min; i--) handled_dlls[i] = handled_dlls[i - 1];
    handled_dlls[i] = wcsdup( name );
    handled_count++;
    return TRUE;
}

static int is_valid_ptr( const void *data, SIZE_T size, const void *ptr, SIZE_T ptr_size )
{
    if (ptr < data) return 0;
    if ((char *)ptr - (char *)data >= size) return 0;
    return (size - ((char *)ptr - (char *)data) >= ptr_size);
}

/* extract the 16-bit NE dll from a PE builtin */
static void extract_16bit_image( IMAGE_NT_HEADERS *nt, void **data, SIZE_T *size )
{
    DWORD exp_size, *size_ptr;
    IMAGE_DOS_HEADER *dos;
    IMAGE_EXPORT_DIRECTORY *exports;
    IMAGE_SECTION_HEADER *section = NULL;
    WORD *ordinals;
    DWORD *names, *functions;
    int i;

    exports = RtlImageDirectoryEntryToData( *data, FALSE, IMAGE_DIRECTORY_ENTRY_EXPORT, &exp_size );
    if (!is_valid_ptr( *data, *size, exports, exp_size )) return;
    ordinals = RtlImageRvaToVa( nt, *data, exports->AddressOfNameOrdinals, &section );
    names = RtlImageRvaToVa( nt, *data, exports->AddressOfNames, &section );
    functions = RtlImageRvaToVa( nt, *data, exports->AddressOfFunctions, &section );
    if (!is_valid_ptr( *data, *size, ordinals, exports->NumberOfNames * sizeof(*ordinals) )) return;
    if (!is_valid_ptr( *data, *size, names, exports->NumberOfNames * sizeof(*names) )) return;

    for (i = 0; i < exports->NumberOfNames; i++)
    {
        char *ename = RtlImageRvaToVa( nt, *data, names[i], &section );
        if (strcmp( ename, "__wine_spec_dos_header" )) continue;
        if (ordinals[i] >= exports->NumberOfFunctions) return;
        if (!is_valid_ptr( *data, *size, functions, sizeof(*functions) )) return;
        if (!functions[ordinals[i]]) return;
        dos = RtlImageRvaToVa( nt, *data, functions[ordinals[i]], NULL );
        if (!is_valid_ptr( *data, *size, dos, sizeof(*dos) )) return;
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return;
        size_ptr = (DWORD *)dos->e_res2;
        *size = min( *size_ptr, *size - ((const char *)dos - (const char *)*data) );
        *size_ptr = 0;
        *data = dos;
        break;
    }
}

/* read in the contents of a file into the global file buffer */
/* return 1 on success, 0 on nonexistent file, -1 on other error */
static int read_file( const WCHAR *name, void **data, SIZE_T *size )
{
    struct stat st;
    int fd, ret = -1;
    size_t header_size;
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    const size_t min_size = sizeof(*dos) + 32 +
        FIELD_OFFSET( IMAGE_NT_HEADERS, OptionalHeader.MajorLinkerVersion );

    if ((fd = _wopen( name, O_RDONLY | O_BINARY )) == -1) return 0;
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
    if (read( fd, file_buffer, header_size ) != header_size) goto done;
    dos = file_buffer;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) goto done;
    if (dos->e_lfanew < sizeof(*dos) + 32) goto done;
    if (memcmp( dos + 1, builtin_signature, strlen(builtin_signature) + 1 ) &&
        memcmp( dos + 1, fakedll_signature, strlen(fakedll_signature) + 1 )) goto done;
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
        read( fd, (char *)file_buffer + header_size,
              st.st_size - header_size ) == st.st_size - header_size)
    {
        *data = file_buffer;
        if (lstrlenW(name) > 2 && !wcscmp( name + lstrlenW(name) - 2, L"16" ))
            extract_16bit_image( nt, data, size );
        ret = 1;
    }
done:
    close( fd );
    return ret;
}

/* build a complete fake dll from scratch */
static BOOL build_fake_dll( HANDLE file, const WCHAR *name )
{
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    struct dll_info info;
    const WCHAR *ext;
    BYTE *buffer;
    BOOL ret = FALSE;
    DWORD lfanew = (sizeof(*dos) + sizeof(fakedll_signature) + 15) & ~15;
    DWORD size, header_size = lfanew + sizeof(*nt);

    TRACE( "creating %s\n", debugstr_w(name) );
    info.handle = file;
    buffer = calloc( 1, header_size + 8 * sizeof(IMAGE_SECTION_HEADER) );

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
#if defined __x86_64__
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
#elif defined __aarch64__
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_ARM64;
#elif defined __arm__
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_ARMNT;
#else
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
#endif
    nt->FileHeader.TimeDateStamp = 0;
    nt->FileHeader.Characteristics = 0;
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

    ext = wcsrchr( name, '.' );
    if (!ext || wcsicmp( ext, L".exe" )) nt->FileHeader.Characteristics |= IMAGE_FILE_DLL;

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
    free( buffer );
    return ret;
}

/* check if an existing file is a fake dll so that we can overwrite it */
static BOOL is_fake_dll( HANDLE h )
{
    IMAGE_DOS_HEADER *dos;
    DWORD size;
    BYTE buffer[sizeof(*dos) + 32];

    if (!ReadFile( h, buffer, sizeof(buffer), &size, NULL ) || size != sizeof(buffer))
        return FALSE;
    dos = (IMAGE_DOS_HEADER *)buffer;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return FALSE;
    if (dos->e_lfanew < size) return FALSE;
    return (!memcmp( dos + 1, builtin_signature, sizeof(builtin_signature) ) ||
            !memcmp( dos + 1, fakedll_signature, sizeof(fakedll_signature) ));
}

/* create directories leading to a given file */
static void create_directories( const WCHAR *name )
{
    WCHAR *path, *p;

    /* create the directory/directories */
    path = malloc((wcslen(name) + 1) * sizeof(WCHAR));
    lstrcpyW(path, name);

    p = wcschr(path, '\\');
    while (p != NULL)
    {
        *p = 0;
        if (!CreateDirectoryW(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
            TRACE("Couldn't create directory %s - error: %ld\n", wine_dbgstr_w(path), GetLastError());
        *p = '\\';
        p = wcschr(p+1, '\\');
    }
    free(path);
}

static inline WCHAR *prepend( WCHAR *buffer, const WCHAR *str, size_t len )
{
    return memcpy( buffer - len, str, len * sizeof(WCHAR) );
}

static inline WCHAR *prepend_build_dir_path( WCHAR *ptr, const WCHAR *ext, const WCHAR *arch_dir,
                                             const WCHAR *top_dir, const WCHAR *build_dir )
{
    WCHAR *name = ptr;
    unsigned int namelen = wcslen(name), extlen = wcslen(ext);

    if (namelen > extlen && !wcscmp( name + namelen - extlen, ext )) namelen -= extlen;
    ptr = prepend( ptr, arch_dir, wcslen(arch_dir) );
    ptr = prepend( ptr, name, namelen );
    ptr = prepend( ptr, top_dir, wcslen(top_dir) );
    ptr = prepend( ptr, build_dir, wcslen(build_dir) );
    return ptr;
}


static const WCHAR *enum_load_path( unsigned int idx )
{
    WCHAR buffer[32];
    swprintf( buffer, ARRAY_SIZE(buffer), L"WINEDLLDIR%u", idx );
    return _wgetenv( buffer );
}

/* try to load a pre-compiled fake dll */
static void *load_fake_dll( const WCHAR *name, SIZE_T *size )
{
    const WCHAR *build_dir = _wgetenv( L"WINEBUILDDIR" );
    const WCHAR *path;
    WCHAR *file, *ptr;
    void *data = NULL;
    unsigned int i, pos, len, maxlen = 0;
    WCHAR *p;
    int res = 0;

    if ((p = wcsrchr( name, '\\' ))) name = p + 1;

    i = 0;
    len = lstrlenW( name );
    if (build_dir) maxlen = lstrlenW(build_dir) + ARRAY_SIZE(L"\\programs") + len + 1;
    while ((path = enum_load_path( i++ ))) maxlen = max( maxlen, lstrlenW(path) );
    maxlen += ARRAY_SIZE(pe_dir) + len + 1;

    if (!(file = malloc( maxlen * sizeof(WCHAR) ))) return NULL;

    pos = maxlen - len - 1;
    lstrcpyW( file + pos, name );
    file[--pos] = '\\';

    if (build_dir)
    {
        /* try as a dll */
        file[pos + len + 1] = 0;
        ptr = prepend_build_dir_path( file + pos, L".dll", pe_dir, L"\\dlls", build_dir );
        if ((res = read_file( ptr, &data, size ))) goto done;

        /* now as a program */
        file[pos + len + 1] = 0;
        ptr = prepend_build_dir_path( file + pos, L".exe", pe_dir, L"\\programs", build_dir );
        if ((res = read_file( ptr, &data, size ))) goto done;
    }

    file[pos + len + 1] = 0;
    for (i = 0; (path = enum_load_path( i )); i++)
    {
        ptr = prepend( file + pos, pe_dir, lstrlenW(pe_dir) );
        ptr = prepend( ptr, path, lstrlenW(path) );
        if ((res = read_file( ptr, &data, size ))) break;
        ptr = prepend( file + pos, path, lstrlenW(path) );
        if ((res = read_file( ptr, &data, size ))) break;
    }

done:
    free( file );
    if (res == 1) return data;
    return NULL;
}

/* create the fake dll destination file */
static HANDLE create_dest_file( const WCHAR *name, BOOL delete )
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
        if (delete)
        {
            CloseHandle( h );
            DeleteFileW( name );
            return INVALID_HANDLE_VALUE;
        }
        /* truncate the file */
        SetFilePointer( h, 0, NULL, FILE_BEGIN );
        SetEndOfFile( h );
    }
    else if (GetLastError() == ERROR_SHARING_VIOLATION &&
             (h = CreateFileW( name, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, 0, NULL )) != INVALID_HANDLE_VALUE)
    {
        if (!is_fake_dll( h ))
        {
            TRACE( "%s is not a fake dll, not overwriting it\n", debugstr_w(name) );
            CloseHandle( h );
            return 0;
        }
        CloseHandle( h );
        h = INVALID_HANDLE_VALUE;

        if (!delete)
        {
            WCHAR *p, *path = wcsdup( name );
            WCHAR tmpname[MAX_PATH];

            if ((p = wcsrchr( path, '\\' ))) *p = 0;
            if (GetTempFileNameW( path, L"dll", 0, tmpname ))
            {
                h = CreateFileW( tmpname, GENERIC_READ|GENERIC_WRITE, 0, NULL, TRUNCATE_EXISTING, 0, NULL );
                if (h != INVALID_HANDLE_VALUE)
                    MoveFileExW( tmpname, name, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING );
            }
            free( path );
        }
        else MoveFileExW( name, NULL, MOVEFILE_DELAY_UNTIL_REBOOT );
    }
    else if (!delete)
    {
        if (GetLastError() == ERROR_PATH_NOT_FOUND) create_directories( name );

        h = CreateFileW( name, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL );
        if (h == INVALID_HANDLE_VALUE)
            ERR( "failed to create %s (error=%lu)\n", debugstr_w(name), GetLastError() );
    }
    return h;
}

/* XML parsing code copied from ntdll */

typedef struct
{
    const char  *ptr;
    unsigned int len;
} xmlstr_t;

typedef struct
{
    const char *ptr;
    const char *end;
} xmlbuf_t;

static inline BOOL xmlstr_cmp(const xmlstr_t* xmlstr, const char *str)
{
    return !strncmp(xmlstr->ptr, str, xmlstr->len) && !str[xmlstr->len];
}

static inline BOOL isxmlspace( char ch )
{
    return (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t');
}

static BOOL next_xml_elem( xmlbuf_t *xmlbuf, xmlstr_t *elem )
{
    const char *ptr;

    for (;;)
    {
        ptr = memchr(xmlbuf->ptr, '<', xmlbuf->end - xmlbuf->ptr);
        if (!ptr)
        {
            xmlbuf->ptr = xmlbuf->end;
            return FALSE;
        }
        ptr++;
        if (ptr + 3 < xmlbuf->end && ptr[0] == '!' && ptr[1] == '-' && ptr[2] == '-') /* skip comment */
        {
            for (ptr += 3; ptr + 3 <= xmlbuf->end; ptr++)
                if (ptr[0] == '-' && ptr[1] == '-' && ptr[2] == '>') break;

            if (ptr + 3 > xmlbuf->end)
            {
                xmlbuf->ptr = xmlbuf->end;
                return FALSE;
            }
            xmlbuf->ptr = ptr + 3;
        }
        else break;
    }

    xmlbuf->ptr = ptr;
    while (ptr < xmlbuf->end && !isxmlspace(*ptr) && *ptr != '>' && (*ptr != '/' || ptr == xmlbuf->ptr))
        ptr++;

    elem->ptr = xmlbuf->ptr;
    elem->len = ptr - xmlbuf->ptr;
    xmlbuf->ptr = ptr;
    return xmlbuf->ptr != xmlbuf->end;
}

static BOOL next_xml_attr(xmlbuf_t* xmlbuf, xmlstr_t* name, xmlstr_t* value, BOOL* error)
{
    const char *ptr;

    *error = TRUE;

    while (xmlbuf->ptr < xmlbuf->end && isxmlspace(*xmlbuf->ptr))
        xmlbuf->ptr++;

    if (xmlbuf->ptr == xmlbuf->end) return FALSE;

    if (*xmlbuf->ptr == '/')
    {
        xmlbuf->ptr++;
        if (xmlbuf->ptr == xmlbuf->end || *xmlbuf->ptr != '>')
            return FALSE;

        xmlbuf->ptr++;
        *error = FALSE;
        return FALSE;
    }

    if (*xmlbuf->ptr == '>')
    {
        xmlbuf->ptr++;
        *error = FALSE;
        return FALSE;
    }

    ptr = xmlbuf->ptr;
    while (ptr < xmlbuf->end && *ptr != '=' && *ptr != '>' && !isxmlspace(*ptr)) ptr++;

    if (ptr == xmlbuf->end || *ptr != '=') return FALSE;

    name->ptr = xmlbuf->ptr;
    name->len = ptr-xmlbuf->ptr;
    xmlbuf->ptr = ptr;

    ptr++;
    if (ptr == xmlbuf->end || (*ptr != '"' && *ptr != '\'')) return FALSE;

    value->ptr = ++ptr;
    if (ptr == xmlbuf->end) return FALSE;

    ptr = memchr(ptr, ptr[-1], xmlbuf->end - ptr);
    if (!ptr)
    {
        xmlbuf->ptr = xmlbuf->end;
        return FALSE;
    }

    value->len = ptr - value->ptr;
    xmlbuf->ptr = ptr + 1;

    if (xmlbuf->ptr == xmlbuf->end) return FALSE;

    *error = FALSE;
    return TRUE;
}

static void append_manifest_filename( const xmlstr_t *arch, const xmlstr_t *name, const xmlstr_t *key,
                                      const xmlstr_t *version, const xmlstr_t *lang, WCHAR *buffer, DWORD size )
{
    DWORD pos = lstrlenW( buffer );

    pos += MultiByteToWideChar( CP_UTF8, 0, arch->ptr, arch->len, buffer + pos, size - pos );
    buffer[pos++] = '_';
    pos += MultiByteToWideChar( CP_UTF8, 0, name->ptr, name->len, buffer + pos, size - pos );
    buffer[pos++] = '_';
    pos += MultiByteToWideChar( CP_UTF8, 0, key->ptr, key->len, buffer + pos, size - pos );
    buffer[pos++] = '_';
    pos += MultiByteToWideChar( CP_UTF8, 0, version->ptr, version->len, buffer + pos, size - pos );
    buffer[pos++] = '_';
    pos += MultiByteToWideChar( CP_UTF8, 0, lang->ptr, lang->len, buffer + pos, size - pos );
    lstrcpyW( buffer + pos, L"_deadbeef" );
    wcslwr( buffer );
}

static WCHAR* create_winsxs_dll_path( const xmlstr_t *arch, const xmlstr_t *name,
                                      const xmlstr_t *key, const xmlstr_t *version,
                                      const xmlstr_t *lang )
{
    WCHAR *path;
    DWORD path_len;

    path_len = GetWindowsDirectoryW( NULL, 0 ) + ARRAY_SIZE( L"\\winsxs\\" )
        + arch->len + name->len + key->len + version->len + 19;

    path = malloc( path_len * sizeof(WCHAR) );
    GetWindowsDirectoryW( path, path_len );
    lstrcatW( path, L"\\winsxs\\" );
    append_manifest_filename( arch, name, key, version, lang, path, path_len );
    lstrcatW( path, L"\\" );
    return path;
}

static BOOL create_manifest( const xmlstr_t *arch, const xmlstr_t *name, const xmlstr_t *key,
                             const xmlstr_t *version, const xmlstr_t *lang, const void *data, DWORD len )
{
    WCHAR *path;
    DWORD written, path_len;
    HANDLE handle;
    BOOL ret = FALSE;

    path_len = GetWindowsDirectoryW( NULL, 0 ) + ARRAY_SIZE( L"\\winsxs\\manifests\\" )
        + arch->len + name->len + key->len + version->len + 18 + ARRAY_SIZE( L".manifest" );

    path = malloc( path_len * sizeof(WCHAR) );
    GetWindowsDirectoryW( path, path_len );
    lstrcatW( path, L"\\winsxs\\manifests\\" );
    append_manifest_filename( arch, name, key, version, lang, path, path_len );
    lstrcatW( path, L".manifest" );
    handle = CreateFileW( path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
    if (handle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_PATH_NOT_FOUND)
    {
        create_directories( path );
        handle = CreateFileW( path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
    }

    if (handle != INVALID_HANDLE_VALUE)
    {
        TRACE( "creating %s\n", debugstr_w(path) );
        ret = (WriteFile( handle, data, len, &written, NULL ) && written == len);
        if (!ret) ERR( "failed to write to %s (error=%lu)\n", debugstr_w(path), GetLastError() );
        CloseHandle( handle );
        if (!ret) DeleteFileW( path );
    }
    free( path );
    return ret;
}

struct delay_copy
{
    struct list entry;
    WCHAR *src;
    WCHAR *dest;
    WCHAR  data[];
};

struct dll_data
{
    struct list *delay_copy;
    const WCHAR *src;
};

static BOOL CALLBACK register_manifest( HMODULE module, const WCHAR *type, WCHAR *res_name, LONG_PTR arg )
{
    const struct dll_data *dll_data = (const struct dll_data*)arg;
    WCHAR *p, *dest = NULL;
    DWORD src_len = wcslen( dll_data->src ), dest_len = 0, file_count = 0;
    xmlbuf_t buffer;
    xmlstr_t elem, attr_name, attr_value;
    xmlstr_t name, version, arch, key, lang;
    BOOL error;
    const char *manifest;
    SIZE_T len;
    HRSRC rsrc;

    if (IS_INTRESOURCE( res_name ) || wcsncmp( res_name, L"WINE_MANIFEST", 13 )) return TRUE;

    rsrc = FindResourceW( module, res_name, type );
    manifest = LoadResource( module, rsrc );
    len = SizeofResource( module, rsrc );

    buffer.ptr = manifest;
    buffer.end = manifest + len;
    name.ptr = version.ptr = arch.ptr = key.ptr = lang.ptr = NULL;
    name.len = version.len = arch.len = key.len = lang.len = 0;

    while (next_xml_elem( &buffer, &elem ))
    {
        if (xmlstr_cmp( &elem, "file" ))
        {
            while (next_xml_attr( &buffer, &attr_name, &attr_value, &error ))
            {
                if (xmlstr_cmp(&attr_name, "name"))
                {
                    name = attr_value;
                    break;
                }
            }

            if (!error && dest && name.ptr)
            {
                int namelen = MultiByteToWideChar( CP_UTF8, 0, name.ptr, name.len, NULL, 0 );
                struct delay_copy *add = malloc( offsetof( struct delay_copy,
                                                           data[src_len + 1 + dest_len + 2 * namelen + 1] ));
                add->src = add->data;
                memcpy( add->src, dll_data->src, src_len * sizeof(WCHAR) );
                if (!file_count++)
                {
                    add->src[src_len] = 0;
                    if ((p = wcsrchr( dll_data->src, '\\' ))) src_len = p + 1 - dll_data->src;
                }
                else
                {
                    MultiByteToWideChar( CP_UTF8, 0, name.ptr, name.len, add->src + src_len, namelen );
                    add->src[src_len + namelen] = 0;
                }
                add->dest = add->src + wcslen(add->src) + 1;
                memcpy( add->dest, dest, dest_len * sizeof(WCHAR) );
                MultiByteToWideChar( CP_UTF8, 0, name.ptr, name.len, add->dest + dest_len, namelen );
                add->dest[dest_len + namelen] = 0;
                list_add_tail( dll_data->delay_copy, &add->entry );
            }
            continue;
        }

        if (!xmlstr_cmp( &elem, "assemblyIdentity" )) continue;
        free( dest );
        dest = NULL;
        while (next_xml_attr( &buffer, &attr_name, &attr_value, &error ))
        {
            if (xmlstr_cmp(&attr_name, "name")) name = attr_value;
            else if (xmlstr_cmp(&attr_name, "version")) version = attr_value;
            else if (xmlstr_cmp(&attr_name, "processorArchitecture")) arch = attr_value;
            else if (xmlstr_cmp(&attr_name, "publicKeyToken")) key = attr_value;
            else if (xmlstr_cmp(&attr_name, "language")) lang = attr_value;
        }
        if (!error && name.ptr && version.ptr && arch.ptr && key.ptr)
        {
            if (!lang.ptr)
            {
                lang.ptr = "none";
                lang.len = strlen( lang.ptr );
            }
            if (!arch.len)  /* fixup the architecture */
            {
                char *new_buffer = malloc( len + sizeof(current_arch) );
                memcpy( new_buffer, manifest, arch.ptr - manifest );
                strcpy( new_buffer + (arch.ptr - manifest), current_arch );
                memcpy( new_buffer + strlen(new_buffer), arch.ptr, len - (arch.ptr - manifest) );
                arch.ptr = current_arch;
                arch.len = strlen( current_arch );
                dest = create_winsxs_dll_path( &arch, &name, &key, &version, &lang );
                create_manifest( &arch, &name, &key, &version, &lang, new_buffer, len + arch.len );
                free( new_buffer );
            }
            else
            {
                dest = create_winsxs_dll_path( &arch, &name, &key, &version, &lang );
                create_manifest( &arch, &name, &key, &version, &lang, manifest, len );
            }
            dest_len = wcslen( dest );
        }
    }
    free( dest );

    return TRUE;
}

static BOOL CALLBACK register_resource( HMODULE module, LPCWSTR type, LPWSTR name, LONG_PTR arg )
{
    HRESULT *hr = (HRESULT *)arg;
    WCHAR *buffer;
    HRSRC rsrc = FindResourceW( module, name, type );
    char *str = LoadResource( module, rsrc );
    DWORD lenW, lenA = SizeofResource( module, rsrc );

    if (!str) return FALSE;
    lenW = MultiByteToWideChar( CP_UTF8, 0, str, lenA, NULL, 0 ) + 1;
    if (!(buffer = malloc( lenW * sizeof(WCHAR) ))) return FALSE;
    MultiByteToWideChar( CP_UTF8, 0, str, lenA, buffer, lenW );
    buffer[lenW - 1] = 0;
    *hr = IRegistrar_StringRegister( registrar, buffer );
    free( buffer );
    return TRUE;
}

static void register_fake_dll( const WCHAR *name, const void *data, size_t size, struct list *delay_copy )
{
    const IMAGE_RESOURCE_DIRECTORY *resdir;
    LDR_RESOURCE_INFO info;
    HRESULT hr = S_OK;
    HMODULE module = (HMODULE)((ULONG_PTR)data | 1);
    struct dll_data dll_data = { delay_copy, name };
    WCHAR buffer[MAX_PATH];

    EnumResourceNamesW( module, (WCHAR*)RT_MANIFEST, register_manifest, (LONG_PTR)&dll_data );

    info.Type = (ULONG_PTR)L"WINE_REGISTRY";
    if (LdrFindResourceDirectory_U( module, &info, 1, &resdir )) return;

    if (!registrar)
    {
        HRESULT (WINAPI *pAtlCreateRegistrar)(IRegistrar**);
        HMODULE atl = LoadLibraryW( L"atl100.dll" );

        if ((pAtlCreateRegistrar = (void *)GetProcAddress( atl, "AtlCreateRegistrar" )))
            hr = pAtlCreateRegistrar( &registrar );
        else
            hr = E_NOINTERFACE;

        if (!registrar)
        {
            ERR( "failed to create IRegistrar: %lx\n", hr );
            return;
        }
    }

    TRACE( "registering %s\n", debugstr_w(name) );
    IRegistrar_ClearReplacements( registrar );
    IRegistrar_AddReplacement( registrar, L"MODULE", name );
    GetEnvironmentVariableW( L"SystemRoot", buffer, ARRAY_SIZE(buffer) );
    IRegistrar_AddReplacement( registrar, L"SystemRoot", buffer );
    EnumResourceNamesW( module, L"WINE_REGISTRY", register_resource, (LONG_PTR)&hr );
    if (FAILED(hr)) ERR( "failed to register %s: %lx\n", debugstr_w(name), hr );
}

/* copy a fake dll file to the dest directory */
static int install_fake_dll( WCHAR *dest, WCHAR *file, BOOL delete, struct list *delay_copy )
{
    int ret;
    SIZE_T size;
    void *data;
    DWORD written;
    WCHAR *destname = dest + lstrlenW(dest);
    WCHAR *name = wcsrchr( file, '\\' ) + 1;
    WCHAR *end = name + lstrlenW(name);
    SIZE_T len = end - name;

    if (!(ret = read_file( file, &data, &size )))
    {
        *end = 0;
        return 0;
    }

    if (end > name + 2 && !wcsncmp( end - 2, L"16", 2 )) len -= 2;  /* remove "16" suffix */
    memcpy( destname, name, len * sizeof(WCHAR) );
    destname[len] = 0;
    if (!add_handled_dll( destname )) ret = -1;

    if (ret != -1)
    {
        HANDLE h = create_dest_file( dest, delete );

        if (h && h != INVALID_HANDLE_VALUE)
        {
            TRACE( "%s -> %s\n", debugstr_w(file), debugstr_w(dest) );

            ret = (WriteFile( h, data, size, &written, NULL ) && written == size);
            if (!ret) ERR( "failed to write to %s (error=%lu)\n", debugstr_w(dest), GetLastError() );
            CloseHandle( h );
            if (ret) register_fake_dll( dest, data, size, delay_copy );
            else DeleteFileW( dest );
        }
    }
    *destname = 0;  /* restore it for next file */
    *end = 0;
    return ret;
}

static void delay_copy_files( struct list *delay_copy )
{
    struct delay_copy *copy, *next;
    DWORD written;
    SIZE_T size;
    void *data;
    HANDLE h;
    int ret;

    LIST_FOR_EACH_ENTRY_SAFE( copy, next, delay_copy, struct delay_copy, entry )
    {
        list_remove( &copy->entry );
        ret = read_file( copy->src, &data, &size );
        if (ret != 1)
        {
            free( copy );
            continue;
        }

        h = create_dest_file( copy->dest, FALSE );
        if (h && h != INVALID_HANDLE_VALUE)
        {
            ret = (WriteFile( h, data, size, &written, NULL ) && written == size);
            if (!ret) ERR( "failed to write to %s (error=%lu)\n", debugstr_w(copy->dest), GetLastError() );
            CloseHandle( h );
            if (!ret) DeleteFileW( copy->dest );
        }
        free( copy );
    }
}

/* find and install all fake dlls in a given lib directory */
static void install_lib_dir( WCHAR *dest, WCHAR *file, const WCHAR *wildcard,
                             const WCHAR *default_ext, BOOL delete )
{
    WCHAR *name;
    intptr_t handle;
    struct _wfinddata_t data;
    struct list delay_copy = LIST_INIT( delay_copy );

    file[1] = '\\';  /* change \??\ to \\?\ */
    name = file + lstrlenW(file);
    *name++ = '\\';
    lstrcpyW( name, wildcard );

    if ((handle = _wfindfirst( file, &data )) == -1) return;
    do
    {
        if (lstrlenW( data.name ) > max_dll_name_len) continue;
        if (!wcscmp( data.name, L"." )) continue;
        if (!wcscmp( data.name, L".." )) continue;
        lstrcpyW( name, data.name );
        if (default_ext)  /* inside build dir */
        {
            lstrcatW( name, pe_dir );
            lstrcatW( name, L"\\" );
            lstrcatW( name, data.name );
            if (wcschr( data.name, '.' ) && install_fake_dll( dest, file, delete, &delay_copy ))
                continue;
            if (wcschr( wildcard, '.' ))  /* don't append default if wildcard has an explicit extension */
                continue;
            lstrcatW( name, default_ext );
        }
        install_fake_dll( dest, file, delete, &delay_copy );
    }
    while (!_wfindnext( handle, &data ));
    _findclose( handle );

    delay_copy_files( &delay_copy );
}

/* create fake dlls in dirname for all the files we can find */
static BOOL create_wildcard_dlls( const WCHAR *dirname, const WCHAR *wildcard, BOOL delete )
{
    const WCHAR *build_dir = _wgetenv( L"WINEBUILDDIR" );
    const WCHAR *path;
    unsigned int i, maxlen = 0;
    WCHAR *file, *dest, *p;

    if (build_dir) maxlen = lstrlenW(build_dir) + ARRAY_SIZE(L"\\programs") + 1;
    for (i = 0; (path = enum_load_path(i)); i++) maxlen = max( maxlen, lstrlenW(path) );
    maxlen += 2 * max_dll_name_len + 2 + ARRAY_SIZE(pe_dir) + 10; /* ".dll" */
    if (!(file = malloc( maxlen * sizeof(WCHAR) ))) return FALSE;

    if (!(dest = malloc( (wcslen(dirname) + max_dll_name_len) * sizeof(WCHAR) )))
    {
        free( file );
        return FALSE;
    }
    lstrcpyW( dest, dirname );
    if ((p = wcsrchr( dest, '\\' ))) p[1] = 0;  /* remove wildcard */

    if (build_dir)
    {
        lstrcpyW( file, build_dir );
        lstrcatW( file, L"\\dlls" );
        install_lib_dir( dest, file, wildcard, L".dll", delete );
        lstrcpyW( file, build_dir );
        lstrcatW( file, L"\\programs" );
        install_lib_dir( dest, file, wildcard, L".exe", delete );
    }
    for (i = 0; (path = enum_load_path( i )); i++)
    {
        swprintf( file, maxlen, L"%s%s", path, pe_dir );
        install_lib_dir( dest, file, wildcard, NULL, delete );
        lstrcpyW( file, path );
        install_lib_dir( dest, file, wildcard, NULL, delete );
    }
    free( file );
    free( dest );
    return TRUE;
}

/***********************************************************************
 *            create_fake_dll
 */
BOOL create_fake_dll( const WCHAR *name, const WCHAR *source )
{
    struct list delay_copy = LIST_INIT( delay_copy );
    HANDLE h;
    BOOL ret = FALSE;
    SIZE_T size;
    const WCHAR *filename;
    void *buffer;
    BOOL delete = !wcscmp( source, L"-" );  /* '-' source means delete the file */

    if (!(filename = wcsrchr( name, '\\' ))) filename = name;
    else filename++;

    /* check for empty name which means to only create the directory */
    if (!filename[0])
    {
        create_directories( name );
        return TRUE;
    }
    if (wcspbrk( filename, L"*?" )) return create_wildcard_dlls( name, filename, delete );

    add_handled_dll( filename );

    if (!(h = create_dest_file( name, delete ))) return TRUE;  /* not a fake dll */
    if (h == INVALID_HANDLE_VALUE) return FALSE;

    if (!wcscmp( source, L"*" ))  /* '*' source means create fake file for a non-existent dll */
    {
        ret = build_fake_dll( h, name );
    }
    else if ((buffer = load_fake_dll( source, &size )))
    {
        DWORD written;

        ret = (WriteFile( h, buffer, size, &written, NULL ) && written == size);
        if (ret) register_fake_dll( name, buffer, size, &delay_copy );
        else ERR( "failed to write to %s (error=%lu)\n", debugstr_w(name), GetLastError() );
    }

    CloseHandle( h );
    if (!ret) DeleteFileW( name );

    delay_copy_files( &delay_copy );
    return ret;
}


/***********************************************************************
 *            cleanup_fake_dlls
 */
void cleanup_fake_dlls(void)
{
    if (file_buffer) VirtualFree( file_buffer, 0, MEM_RELEASE );
    file_buffer = NULL;
    free( handled_dlls );
    handled_dlls = NULL;
    handled_count = handled_total = 0;
    if (registrar) IRegistrar_Release( registrar );
    registrar = NULL;
}
