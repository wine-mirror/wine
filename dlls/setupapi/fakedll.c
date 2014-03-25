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

#define COBJMACROS
#define ATL_INITGUID
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
#include "ole2.h"
#include "atliface.h"

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
static BOOL build_fake_dll( HANDLE file, const WCHAR *name )
{
    static const WCHAR dotexeW[] = { '.','e','x','e',0 };
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    struct dll_info info;
    const WCHAR *ext;
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
#if defined __x86_64__
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
#elif defined __aarch64__
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_ARM64;
#elif defined __arm__
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_ARMNT;
#elif defined __powerpc__
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_POWERPC;
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

    ext = strrchrW( name, '.' );
    if (!ext || strcmpiW( ext, dotexeW )) nt->FileHeader.Characteristics |= IMAGE_FILE_DLL;

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
    len = strlenW( name );
    if (build_dir) maxlen = strlen(build_dir) + sizeof("/programs/") + len;
    while ((path = wine_dll_enum_load_path( i++ ))) maxlen = max( maxlen, strlen(path) );
    maxlen += sizeof("/fakedlls") + len + sizeof(".fake");

    if (!(file = HeapAlloc( GetProcessHeap(), 0, maxlen ))) return NULL;

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

static BOOL next_xml_attr(xmlbuf_t* xmlbuf, xmlstr_t* name, xmlstr_t* value,
                          BOOL* error, BOOL* end)
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
        *end = TRUE;
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

static void get_manifest_filename( const xmlstr_t *arch, const xmlstr_t *name, const xmlstr_t *key,
                                   const xmlstr_t *version, const xmlstr_t *lang, WCHAR *buffer, DWORD size )
{
    static const WCHAR trailerW[] = {'_','d','e','a','d','b','e','e','f',0};
    DWORD pos;

    pos = MultiByteToWideChar( CP_UTF8, 0, arch->ptr, arch->len, buffer, size );
    buffer[pos++] = '_';
    pos += MultiByteToWideChar( CP_UTF8, 0, name->ptr, name->len, buffer + pos, size - pos );
    buffer[pos++] = '_';
    pos += MultiByteToWideChar( CP_UTF8, 0, key->ptr, key->len, buffer + pos, size - pos );
    buffer[pos++] = '_';
    pos += MultiByteToWideChar( CP_UTF8, 0, version->ptr, version->len, buffer + pos, size - pos );
    buffer[pos++] = '_';
    pos += MultiByteToWideChar( CP_UTF8, 0, lang->ptr, lang->len, buffer + pos, size - pos );
    memcpy( buffer + pos, trailerW, sizeof(trailerW) );
    strlwrW( buffer );
}

static BOOL create_winsxs_dll( const WCHAR *dll_name, const xmlstr_t *arch, const xmlstr_t *name,
                               const xmlstr_t *key, const xmlstr_t *version, const xmlstr_t *lang,
                               const void *dll_data, size_t dll_size )
{
    static const WCHAR winsxsW[] = {'w','i','n','s','x','s','\\'};
    WCHAR *path;
    const WCHAR *filename;
    DWORD pos, written, path_len;
    HANDLE handle;
    BOOL ret = FALSE;

    if (!(filename = strrchrW( dll_name, '\\' ))) filename = dll_name;
    else filename++;

    path_len = GetWindowsDirectoryW( NULL, 0 ) + 1 + sizeof(winsxsW)/sizeof(WCHAR)
        + arch->len + name->len + key->len + version->len + 18 + strlenW( filename ) + 1;

    path = HeapAlloc( GetProcessHeap(), 0, path_len * sizeof(WCHAR) );
    pos = GetWindowsDirectoryW( path, path_len );
    path[pos++] = '\\';
    memcpy( path + pos, winsxsW, sizeof(winsxsW) );
    pos += sizeof(winsxsW) / sizeof(WCHAR);
    get_manifest_filename( arch, name, key, version, lang, path + pos, path_len - pos );
    pos += strlenW( path + pos );
    path[pos++] = '\\';
    strcpyW( path + pos, filename );
    handle = create_dest_file( path );
    if (handle && handle != INVALID_HANDLE_VALUE)
    {
        TRACE( "creating %s\n", debugstr_w(path) );
        ret = (WriteFile( handle, dll_data, dll_size, &written, NULL ) && written == dll_size);
        if (!ret) ERR( "failed to write to %s (error=%u)\n", debugstr_w(path), GetLastError() );
        CloseHandle( handle );
        if (!ret) DeleteFileW( path );
    }
    HeapFree( GetProcessHeap(), 0, path );
    return ret;
}

static BOOL create_manifest( const xmlstr_t *arch, const xmlstr_t *name, const xmlstr_t *key,
                             const xmlstr_t *version, const xmlstr_t *lang, const void *data, DWORD len )
{
    static const WCHAR winsxsW[] = {'w','i','n','s','x','s','\\','m','a','n','i','f','e','s','t','s','\\'};
    static const WCHAR extensionW[] = {'.','m','a','n','i','f','e','s','t',0};
    WCHAR *path;
    DWORD pos, written, path_len;
    HANDLE handle;
    BOOL ret = FALSE;

    path_len = GetWindowsDirectoryW( NULL, 0 ) + 1 + sizeof(winsxsW)/sizeof(WCHAR)
        + arch->len + name->len + key->len + version->len + 18 + sizeof(extensionW)/sizeof(WCHAR);

    path = HeapAlloc( GetProcessHeap(), 0, path_len * sizeof(WCHAR) );
    pos = GetWindowsDirectoryW( path, MAX_PATH );
    path[pos++] = '\\';
    memcpy( path + pos, winsxsW, sizeof(winsxsW) );
    pos += sizeof(winsxsW) / sizeof(WCHAR);
    get_manifest_filename( arch, name, key, version, lang, path + pos, MAX_PATH - pos );
    strcatW( path + pos, extensionW );
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
        if (!ret) ERR( "failed to write to %s (error=%u)\n", debugstr_w(path), GetLastError() );
        CloseHandle( handle );
        if (!ret) DeleteFileW( path );
    }
    HeapFree( GetProcessHeap(), 0, path );
    return ret;
}

static void register_manifest( const WCHAR *dll_name, const char *manifest, DWORD len,
                               const void *dll_data, size_t dll_size )
{
#ifdef __i386__
    static const char current_arch[] = "x86";
#elif defined __x86_64__
    static const char current_arch[] = "amd64";
#else
    static const char current_arch[] = "none";
#endif
    xmlbuf_t buffer;
    xmlstr_t elem, attr_name, attr_value;
    xmlstr_t name, version, arch, key, lang;
    BOOL end = FALSE, error;

    buffer.ptr = manifest;
    buffer.end = manifest + len;
    name.ptr = version.ptr = arch.ptr = key.ptr = lang.ptr = NULL;
    name.len = version.len = arch.len = key.len = lang.len = 0;

    while (next_xml_elem( &buffer, &elem ))
    {
        if (!xmlstr_cmp( &elem, "assemblyIdentity" )) continue;
        while (next_xml_attr( &buffer, &attr_name, &attr_value, &error, &end ))
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
                char *new_buffer = HeapAlloc( GetProcessHeap(), 0, len + sizeof(current_arch) );
                memcpy( new_buffer, manifest, arch.ptr - manifest );
                strcpy( new_buffer + (arch.ptr - manifest), current_arch );
                memcpy( new_buffer + strlen(new_buffer), arch.ptr, len - (arch.ptr - manifest) );
                arch.ptr = current_arch;
                arch.len = strlen( current_arch );
                if (create_winsxs_dll( dll_name, &arch, &name, &key, &version, &lang, dll_data, dll_size ))
                    create_manifest( &arch, &name, &key, &version, &lang, new_buffer, len + arch.len );
                HeapFree( GetProcessHeap(), 0, new_buffer );
            }
            else
            {
                if (create_winsxs_dll( dll_name, &arch, &name, &key, &version, &lang, dll_data, dll_size ))
                    create_manifest( &arch, &name, &key, &version, &lang, manifest, len );
            }
        }
    }
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
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, lenW * sizeof(WCHAR) ))) return FALSE;
    MultiByteToWideChar( CP_UTF8, 0, str, lenA, buffer, lenW );
    buffer[lenW - 1] = 0;
    *hr = IRegistrar_StringRegister( registrar, buffer );
    HeapFree( GetProcessHeap(), 0, buffer );
    return TRUE;
}

static void register_fake_dll( const WCHAR *name, const void *data, size_t size )
{
    static const WCHAR atlW[] = {'a','t','l','1','0','0','.','d','l','l',0};
    static const WCHAR moduleW[] = {'M','O','D','U','L','E',0};
    static const WCHAR regtypeW[] = {'W','I','N','E','_','R','E','G','I','S','T','R','Y',0};
    static const WCHAR manifestW[] = {'W','I','N','E','_','M','A','N','I','F','E','S','T',0};
    const IMAGE_RESOURCE_DIRECTORY *resdir;
    LDR_RESOURCE_INFO info;
    HRESULT hr = S_OK;
    HMODULE module = (HMODULE)((ULONG_PTR)data | 1);
    HRSRC rsrc;

    if ((rsrc = FindResourceW( module, manifestW, MAKEINTRESOURCEW(RT_MANIFEST) )))
    {
        char *manifest = LoadResource( module, rsrc );
        register_manifest( name, manifest, SizeofResource( module, rsrc ), data, size );
    }

    info.Type = (ULONG_PTR)regtypeW;
    if (LdrFindResourceDirectory_U( module, &info, 1, &resdir )) return;

    if (!registrar)
    {
        HRESULT (WINAPI *pAtlCreateRegistrar)(IRegistrar**);
        HMODULE atl = LoadLibraryW( atlW );

        if ((pAtlCreateRegistrar = (void *)GetProcAddress( atl, "AtlCreateRegistrar" )))
            hr = pAtlCreateRegistrar( &registrar );
        else
            hr = E_NOINTERFACE;

        if (!registrar)
        {
            ERR( "failed to create IRegistrar: %x\n", hr );
            return;
        }
    }

    TRACE( "registering %s\n", debugstr_w(name) );
    IRegistrar_ClearReplacements( registrar );
    IRegistrar_AddReplacement( registrar, moduleW, name );
    EnumResourceNamesW( module, regtypeW, register_resource, (LONG_PTR)&hr );
    if (FAILED(hr)) ERR( "failed to register %s: %x\n", debugstr_w(name), hr );
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
            if (ret) register_fake_dll( dest, data, size );
            else DeleteFileW( dest );
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
        if (ret) register_fake_dll( name, buffer, size );
        else ERR( "failed to write to %s (error=%u)\n", debugstr_w(name), GetLastError() );
    }
    else
    {
        WARN( "fake dll %s not found for %s\n", debugstr_w(source), debugstr_w(name) );
        ret = build_fake_dll( h, name );
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
    if (registrar) IRegistrar_Release( registrar );
    registrar = NULL;
}
