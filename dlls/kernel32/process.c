/*
 * Win32 processes
 *
 * Copyright 1996, 1998 Alexandre Julliard
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

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_PRCTL_H
# include <sys/prctl.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <pthread.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "winbase.h"
#include "wincon.h"
#include "kernel_private.h"
#include "psapi.h"
#include "wine/exception.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/unicode.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(process);
WINE_DECLARE_DEBUG_CHANNEL(relay);

#ifdef __APPLE__
extern char **__wine_get_main_environment(void);
#else
extern char **__wine_main_environ;
static char **__wine_get_main_environment(void) { return __wine_main_environ; }
#endif

typedef struct
{
    LPSTR lpEnvAddress;
    LPSTR lpCmdLine;
    LPSTR lpCmdShow;
    DWORD dwReserved;
} LOADPARMS32;

static BOOL is_wow64;
static const BOOL is_win64 = (sizeof(void *) > sizeof(int));

HMODULE kernel32_handle = 0;
SYSTEM_BASIC_INFORMATION system_info = { 0 };

const WCHAR DIR_Windows[] = {'C',':','\\','w','i','n','d','o','w','s',0};
const WCHAR DIR_System[] = {'C',':','\\','w','i','n','d','o','w','s',
                            '\\','s','y','s','t','e','m','3','2',0};
const WCHAR *DIR_SysWow64 = NULL;

/* Process flags */
#define PDB32_DEBUGGED      0x0001  /* Process is being debugged */
#define PDB32_WIN16_PROC    0x0008  /* Win16 process */
#define PDB32_DOS_PROC      0x0010  /* Dos process */
#define PDB32_CONSOLE_PROC  0x0020  /* Console process */
#define PDB32_FILE_APIS_OEM 0x0040  /* File APIs are OEM */
#define PDB32_WIN32S_PROC   0x8000  /* Win32s process */

static const WCHAR exeW[] = {'.','e','x','e',0};
static const WCHAR comW[] = {'.','c','o','m',0};
static const WCHAR batW[] = {'.','b','a','t',0};
static const WCHAR cmdW[] = {'.','c','m','d',0};
static const WCHAR pifW[] = {'.','p','i','f',0};
static WCHAR winevdm[] = {'C',':','\\','w','i','n','d','o','w','s',
                          '\\','s','y','s','t','e','m','3','2',
                          '\\','w','i','n','e','v','d','m','.','e','x','e',0};

static const char * const cpu_names[] = { "x86", "x86_64", "PowerPC", "ARM", "ARM64" };

static void exec_process( LPCWSTR name );

/* return values for get_binary_info */
enum binary_type
{
    BINARY_UNKNOWN = 0,
    BINARY_PE,
    BINARY_WIN16,
    BINARY_UNIX_EXE,
    BINARY_UNIX_LIB
};


/***********************************************************************
 *           contains_path
 */
static inline BOOL contains_path( LPCWSTR name )
{
    return ((*name && (name[1] == ':')) || strchrW(name, '/') || strchrW(name, '\\'));
}


/***********************************************************************
 *           is_special_env_var
 *
 * Check if an environment variable needs to be handled specially when
 * passed through the Unix environment (i.e. prefixed with "WINE").
 */
static inline BOOL is_special_env_var( const char *var )
{
    return (!strncmp( var, "PATH=", sizeof("PATH=")-1 ) ||
            !strncmp( var, "PWD=", sizeof("PWD=")-1 ) ||
            !strncmp( var, "HOME=", sizeof("HOME=")-1 ) ||
            !strncmp( var, "TEMP=", sizeof("TEMP=")-1 ) ||
            !strncmp( var, "TMP=", sizeof("TMP=")-1 ) ||
            !strncmp( var, "QT_", sizeof("QT_")-1 ) ||
            !strncmp( var, "VK_", sizeof("VK_")-1 ));
}


/***********************************************************************
 *           is_path_prefix
 */
static inline unsigned int is_path_prefix( const WCHAR *prefix, const WCHAR *filename )
{
    unsigned int len = strlenW( prefix );

    if (strncmpiW( filename, prefix, len ) || filename[len] != '\\') return 0;
    while (filename[len] == '\\') len++;
    return len;
}


/***********************************************************************
 *           is_64bit_arch
 */
static inline BOOL is_64bit_arch( client_cpu_t cpu )
{
    return (cpu == CPU_x86_64 || cpu == CPU_ARM64);
}


/***********************************************************************
 *           get_pe_info
 */
static NTSTATUS get_pe_info( HANDLE handle, pe_image_info_t *info )
{
    NTSTATUS status;
    HANDLE mapping;

    status = NtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY,
                              NULL, NULL, PAGE_READONLY, SEC_IMAGE, handle );
    if (status) return status;

    SERVER_START_REQ( get_mapping_info )
    {
        req->handle = wine_server_obj_handle( mapping );
        req->access = SECTION_QUERY;
        wine_server_set_reply( req, info, sizeof(*info) );
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    CloseHandle( mapping );
    return status;
}


/***********************************************************************
 *           get_binary_info
 */
static enum binary_type get_binary_info( HANDLE hfile, pe_image_info_t *info )
{
    union
    {
        struct
        {
            unsigned char magic[4];
            unsigned char class;
            unsigned char data;
            unsigned char ignored1[10];
            unsigned short type;
            unsigned short machine;
            unsigned char ignored2[8];
            unsigned int phoff;
            unsigned char ignored3[12];
            unsigned short phnum;
        } elf;
        struct
        {
            unsigned char magic[4];
            unsigned char class;
            unsigned char data;
            unsigned char ignored1[10];
            unsigned short type;
            unsigned short machine;
            unsigned char ignored2[12];
            unsigned __int64 phoff;
            unsigned char ignored3[16];
            unsigned short phnum;
        } elf64;
        struct
        {
            unsigned int magic;
            unsigned int cputype;
            unsigned int cpusubtype;
            unsigned int filetype;
        } macho;
        IMAGE_DOS_HEADER mz;
    } header;

    DWORD len;
    NTSTATUS status;

    memset( info, 0, sizeof(*info) );

    status = get_pe_info( hfile, info );
    switch (status)
    {
    case STATUS_SUCCESS:
        return BINARY_PE;
    case STATUS_INVALID_IMAGE_WIN_32:
        return BINARY_PE;
    case STATUS_INVALID_IMAGE_WIN_64:
        return BINARY_PE;
    case STATUS_INVALID_IMAGE_WIN_16:
    case STATUS_INVALID_IMAGE_NE_FORMAT:
    case STATUS_INVALID_IMAGE_PROTECT:
        return BINARY_WIN16;
    }

    /* Seek to the start of the file and read the header information. */
    if (SetFilePointer( hfile, 0, NULL, SEEK_SET ) == -1) return BINARY_UNKNOWN;
    if (!ReadFile( hfile, &header, sizeof(header), &len, NULL ) || len != sizeof(header))
        return BINARY_UNKNOWN;

    if (!memcmp( header.elf.magic, "\177ELF", 4 ))
    {
#ifdef WORDS_BIGENDIAN
        BOOL byteswap = (header.elf.data == 1);
#else
        BOOL byteswap = (header.elf.data == 2);
#endif
        if (byteswap)
        {
            header.elf.type = RtlUshortByteSwap( header.elf.type );
            header.elf.machine = RtlUshortByteSwap( header.elf.machine );
        }
        switch(header.elf.machine)
        {
        case 3:   info->cpu = CPU_x86; break;
        case 20:  info->cpu = CPU_POWERPC; break;
        case 40:  info->cpu = CPU_ARM; break;
        case 62:  info->cpu = CPU_x86_64; break;
        case 183: info->cpu = CPU_ARM64; break;
        }
        switch(header.elf.type)
        {
        case 2:
            return BINARY_UNIX_EXE;
        case 3:
        {
            LARGE_INTEGER phoff;
            unsigned short phnum;
            unsigned int type;
            if (header.elf.class == 2)
            {
                phoff.QuadPart = byteswap ? RtlUlonglongByteSwap( header.elf64.phoff ) : header.elf64.phoff;
                phnum = byteswap ? RtlUshortByteSwap( header.elf64.phnum ) : header.elf64.phnum;
            }
            else
            {
                phoff.QuadPart = byteswap ? RtlUlongByteSwap( header.elf.phoff ) : header.elf.phoff;
                phnum = byteswap ? RtlUshortByteSwap( header.elf.phnum ) : header.elf.phnum;
            }
            while (phnum--)
            {
                if (SetFilePointerEx( hfile, phoff, NULL, FILE_BEGIN ) == -1) return BINARY_UNKNOWN;
                if (!ReadFile( hfile, &type, sizeof(type), &len, NULL ) || len < sizeof(type))
                    return BINARY_UNKNOWN;
                if (byteswap) type = RtlUlongByteSwap( type );
                if (type == 3) return BINARY_UNIX_EXE;
                phoff.QuadPart += (header.elf.class == 2) ? 56 : 32;
            }
            return BINARY_UNIX_LIB;
        }
        }
    }
    /* Mach-o File with Endian set to Big Endian or Little Endian */
    else if (header.macho.magic == 0xfeedface || header.macho.magic == 0xcefaedfe ||
             header.macho.magic == 0xfeedfacf || header.macho.magic == 0xcffaedfe)
    {
        if (header.macho.magic == 0xcefaedfe || header.macho.magic == 0xcffaedfe)
        {
            header.macho.filetype = RtlUlongByteSwap( header.macho.filetype );
            header.macho.cputype = RtlUlongByteSwap( header.macho.cputype );
        }
        switch(header.macho.cputype)
        {
        case 0x00000007: info->cpu = CPU_x86; break;
        case 0x01000007: info->cpu = CPU_x86_64; break;
        case 0x0000000c: info->cpu = CPU_ARM; break;
        case 0x0100000c: info->cpu = CPU_ARM64; break;
        case 0x00000012: info->cpu = CPU_POWERPC; break;
        }
        switch(header.macho.filetype)
        {
        case 2: return BINARY_UNIX_EXE;
        case 8: return BINARY_UNIX_LIB;
        }
    }
    return BINARY_UNKNOWN;
}


/***************************************************************************
 *	get_builtin_path
 *
 * Get the path of a builtin module when the native file does not exist.
 */
static BOOL get_builtin_path( const WCHAR *libname, const WCHAR *ext, WCHAR *filename,
                              UINT size, BOOL *is_64bit )
{
    WCHAR *file_part;
    UINT len;
    void *redir_disabled = 0;

    *is_64bit = (sizeof(void*) > sizeof(int));

    /* builtin names cannot be empty or contain spaces */
    if (!libname[0] || strchrW( libname, ' ' ) || strchrW( libname, '\t' )) return FALSE;

    if (is_wow64 && Wow64DisableWow64FsRedirection( &redir_disabled ))
        Wow64RevertWow64FsRedirection( redir_disabled );

    if (contains_path( libname ))
    {
        if (RtlGetFullPathName_U( libname, size * sizeof(WCHAR),
                                  filename, &file_part ) > size * sizeof(WCHAR))
            return FALSE;  /* too long */

        if ((len = is_path_prefix( DIR_System, filename )))
        {
            if (is_wow64 && redir_disabled) *is_64bit = TRUE;
        }
        else if (DIR_SysWow64 && (len = is_path_prefix( DIR_SysWow64, filename )))
        {
            *is_64bit = FALSE;
        }
        else return FALSE;

        if (filename + len != file_part) return FALSE;
    }
    else
    {
        len = strlenW( DIR_System );
        if (strlenW(libname) + len + 2 >= size) return FALSE;  /* too long */
        memcpy( filename, DIR_System, len * sizeof(WCHAR) );
        file_part = filename + len;
        if (file_part > filename && file_part[-1] != '\\') *file_part++ = '\\';
        strcpyW( file_part, libname );
        if (is_wow64 && redir_disabled) *is_64bit = TRUE;
    }
    if (ext && !strchrW( file_part, '.' ))
    {
        if (file_part + strlenW(file_part) + strlenW(ext) + 1 > filename + size)
            return FALSE;  /* too long */
        strcatW( file_part, ext );
    }
    return TRUE;
}


/***********************************************************************
 *           open_exe_file
 *
 * Open a specific exe file, taking load order into account.
 * Returns the file handle or 0 for a builtin exe.
 */
static HANDLE open_exe_file( const WCHAR *name, BOOL *is_64bit )
{
    HANDLE handle;

    TRACE("looking for %s\n", debugstr_w(name) );

    if ((handle = CreateFileW( name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_DELETE,
                               NULL, OPEN_EXISTING, 0, 0 )) == INVALID_HANDLE_VALUE)
    {
        WCHAR buffer[MAX_PATH];
        /* file doesn't exist, check for builtin */
        if (contains_path( name ) && get_builtin_path( name, NULL, buffer, sizeof(buffer), is_64bit ))
            handle = 0;
    }
    return handle;
}


/***********************************************************************
 *           find_exe_file
 *
 * Open an exe file, and return the full name and file handle.
 * Returns FALSE if file could not be found.
 */
static BOOL find_exe_file( const WCHAR *name, WCHAR *buffer, int buflen, HANDLE *handle )
{
    TRACE("looking for %s\n", debugstr_w(name) );

    if (!SearchPathW( NULL, name, exeW, buflen, buffer, NULL ) &&
        /* no builtin found, try native without extension in case it is a Unix app */
        !SearchPathW( NULL, name, NULL, buflen, buffer, NULL )) return FALSE;

    TRACE( "Trying native exe %s\n", debugstr_w(buffer) );
    *handle = CreateFileW( buffer, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_DELETE,
                           NULL, OPEN_EXISTING, 0, 0 );
    return (*handle != INVALID_HANDLE_VALUE);
}


/***********************************************************************
 *           build_initial_environment
 *
 * Build the Win32 environment from the Unix environment
 */
static BOOL build_initial_environment(void)
{
    SIZE_T size = 1;
    char **e;
    WCHAR *p, *endptr;
    void *ptr;
    char **env = __wine_get_main_environment();

    /* Compute the total size of the Unix environment */
    for (e = env; *e; e++)
    {
        if (is_special_env_var( *e )) continue;
        size += MultiByteToWideChar( CP_UNIXCP, 0, *e, -1, NULL, 0 );
    }
    size *= sizeof(WCHAR);

    /* Now allocate the environment */
    ptr = NULL;
    if (NtAllocateVirtualMemory(NtCurrentProcess(), &ptr, 0, &size,
                                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE) != STATUS_SUCCESS)
        return FALSE;

    NtCurrentTeb()->Peb->ProcessParameters->Environment = p = ptr;
    endptr = p + size / sizeof(WCHAR);

    /* And fill it with the Unix environment */
    for (e = env; *e; e++)
    {
        char *str = *e;

        /* skip Unix special variables and use the Wine variants instead */
        if (!strncmp( str, "WINE", 4 ))
        {
            if (is_special_env_var( str + 4 )) str += 4;
            else if (!strncmp( str, "WINEPRELOADRESERVE=", 19 )) continue;  /* skip it */
        }
        else if (is_special_env_var( str )) continue;  /* skip it */

        MultiByteToWideChar( CP_UNIXCP, 0, str, -1, p, endptr - p );
        p += strlenW(p) + 1;
    }
    *p = 0;
    return TRUE;
}


/***********************************************************************
 *           set_registry_variables
 *
 * Set environment variables by enumerating the values of a key;
 * helper for set_registry_environment().
 * Note that Windows happily truncates the value if it's too big.
 */
static void set_registry_variables( HANDLE hkey, ULONG type )
{
    static const WCHAR pathW[] = {'P','A','T','H'};
    static const WCHAR sep[] = {';',0};
    UNICODE_STRING env_name, env_value;
    NTSTATUS status;
    DWORD size;
    int index;
    char buffer[1024*sizeof(WCHAR) + sizeof(KEY_VALUE_FULL_INFORMATION)];
    WCHAR tmpbuf[1024];
    UNICODE_STRING tmp;
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;

    tmp.Buffer = tmpbuf;
    tmp.MaximumLength = sizeof(tmpbuf);

    for (index = 0; ; index++)
    {
        status = NtEnumerateValueKey( hkey, index, KeyValueFullInformation,
                                      buffer, sizeof(buffer), &size );
        if (status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW)
            break;
        if (info->Type != type)
            continue;
        env_name.Buffer = info->Name;
        env_name.Length = env_name.MaximumLength = info->NameLength;
        env_value.Buffer = (WCHAR *)(buffer + info->DataOffset);
        env_value.Length = info->DataLength;
        env_value.MaximumLength = sizeof(buffer) - info->DataOffset;
        if (env_value.Length && !env_value.Buffer[env_value.Length/sizeof(WCHAR)-1])
            env_value.Length -= sizeof(WCHAR);  /* don't count terminating null if any */
        if (!env_value.Length) continue;
        if (info->Type == REG_EXPAND_SZ)
        {
            status = RtlExpandEnvironmentStrings_U( NULL, &env_value, &tmp, NULL );
            if (status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW) continue;
            RtlCopyUnicodeString( &env_value, &tmp );
        }
        /* PATH is magic */
        if (env_name.Length == sizeof(pathW) &&
            !strncmpiW( env_name.Buffer, pathW, ARRAY_SIZE( pathW )) &&
            !RtlQueryEnvironmentVariable_U( NULL, &env_name, &tmp ))
        {
            RtlAppendUnicodeToString( &tmp, sep );
            if (RtlAppendUnicodeStringToString( &tmp, &env_value )) continue;
            RtlCopyUnicodeString( &env_value, &tmp );
        }
        RtlSetEnvironmentVariable( NULL, &env_name, &env_value );
    }
}


/***********************************************************************
 *           set_registry_environment
 *
 * Set the environment variables specified in the registry.
 *
 * Note: Windows handles REG_SZ and REG_EXPAND_SZ in one pass with the
 * consequence that REG_EXPAND_SZ cannot be used reliably as it depends
 * on the order in which the variables are processed. But on Windows it
 * does not really matter since they only use %SystemDrive% and
 * %SystemRoot% which are predefined. But Wine defines these in the
 * registry, so we need two passes.
 */
static BOOL set_registry_environment( BOOL volatile_only )
{
    static const WCHAR env_keyW[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                     'M','a','c','h','i','n','e','\\',
                                     'S','y','s','t','e','m','\\',
                                     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                     'C','o','n','t','r','o','l','\\',
                                     'S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r','\\',
                                     'E','n','v','i','r','o','n','m','e','n','t',0};
    static const WCHAR envW[] = {'E','n','v','i','r','o','n','m','e','n','t',0};
    static const WCHAR volatile_envW[] = {'V','o','l','a','t','i','l','e',' ','E','n','v','i','r','o','n','m','e','n','t',0};

    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE hkey;
    BOOL ret = FALSE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    /* first the system environment variables */
    RtlInitUnicodeString( &nameW, env_keyW );
    if (!volatile_only && NtOpenKey( &hkey, KEY_READ, &attr ) == STATUS_SUCCESS)
    {
        set_registry_variables( hkey, REG_SZ );
        set_registry_variables( hkey, REG_EXPAND_SZ );
        NtClose( hkey );
        ret = TRUE;
    }

    /* then the ones for the current user */
    if (RtlOpenCurrentUser( KEY_READ, &attr.RootDirectory ) != STATUS_SUCCESS) return ret;
    RtlInitUnicodeString( &nameW, envW );
    if (!volatile_only && NtOpenKey( &hkey, KEY_READ, &attr ) == STATUS_SUCCESS)
    {
        set_registry_variables( hkey, REG_SZ );
        set_registry_variables( hkey, REG_EXPAND_SZ );
        NtClose( hkey );
    }

    RtlInitUnicodeString( &nameW, volatile_envW );
    if (NtOpenKey( &hkey, KEY_READ, &attr ) == STATUS_SUCCESS)
    {
        set_registry_variables( hkey, REG_SZ );
        set_registry_variables( hkey, REG_EXPAND_SZ );
        NtClose( hkey );
    }

    NtClose( attr.RootDirectory );
    return ret;
}


/***********************************************************************
 *           get_reg_value
 */
static WCHAR *get_reg_value( HKEY hkey, const WCHAR *name )
{
    char buffer[1024 * sizeof(WCHAR) + sizeof(KEY_VALUE_PARTIAL_INFORMATION)];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    DWORD len, size = sizeof(buffer);
    WCHAR *ret = NULL;
    UNICODE_STRING nameW;

    RtlInitUnicodeString( &nameW, name );
    if (NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, buffer, size, &size ))
        return NULL;

    if (size <= FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data )) return NULL;
    len = (size - FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data )) / sizeof(WCHAR);

    if (info->Type == REG_EXPAND_SZ)
    {
        UNICODE_STRING value, expanded;

        value.MaximumLength = len * sizeof(WCHAR);
        value.Buffer = (WCHAR *)info->Data;
        if (!value.Buffer[len - 1]) len--;  /* don't count terminating null if any */
        value.Length = len * sizeof(WCHAR);
        expanded.Length = expanded.MaximumLength = 1024 * sizeof(WCHAR);
        if (!(expanded.Buffer = HeapAlloc( GetProcessHeap(), 0, expanded.MaximumLength ))) return NULL;
        if (!RtlExpandEnvironmentStrings_U( NULL, &value, &expanded, NULL )) ret = expanded.Buffer;
        else RtlFreeUnicodeString( &expanded );
    }
    else if (info->Type == REG_SZ)
    {
        if ((ret = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) )))
        {
            memcpy( ret, info->Data, len * sizeof(WCHAR) );
            ret[len] = 0;
        }
    }
    return ret;
}

/* set an environment variable for one of the wine path variables */
static void set_wine_path_variable( const WCHAR *name, const char *unix_path )
{
    UNICODE_STRING nt_name, var_name;
    ANSI_STRING unix_name;

    RtlInitUnicodeString( &var_name, name );
    if (unix_path)
    {
        RtlInitAnsiString( &unix_name, unix_path );
        if (wine_unix_to_nt_file_name( &unix_name, &nt_name )) return;
        RtlSetEnvironmentVariable( NULL, &var_name, &nt_name );
        RtlFreeUnicodeString( &nt_name );
    }
    else RtlSetEnvironmentVariable( NULL, &var_name, NULL );
}


/***********************************************************************
 *           set_additional_environment
 *
 * Set some additional environment variables not specified in the registry.
 */
static void set_additional_environment(void)
{
    static const WCHAR profile_keyW[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                         'M','a','c','h','i','n','e','\\',
                                         'S','o','f','t','w','a','r','e','\\',
                                         'M','i','c','r','o','s','o','f','t','\\',
                                         'W','i','n','d','o','w','s',' ','N','T','\\',
                                         'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                         'P','r','o','f','i','l','e','L','i','s','t',0};
    static const WCHAR profiles_valueW[] = {'P','r','o','f','i','l','e','s','D','i','r','e','c','t','o','r','y',0};
    static const WCHAR public_valueW[] = {'P','u','b','l','i','c',0};
    static const WCHAR computernameW[] = {'C','O','M','P','U','T','E','R','N','A','M','E',0};
    static const WCHAR allusersW[] = {'A','L','L','U','S','E','R','S','P','R','O','F','I','L','E',0};
    static const WCHAR programdataW[] = {'P','r','o','g','r','a','m','D','a','t','a',0};
    static const WCHAR publicW[] = {'P','U','B','L','I','C',0};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    WCHAR *profile_dir = NULL, *program_data_dir = NULL, *public_dir = NULL;
    WCHAR buf[32];
    HANDLE hkey;
    DWORD len;

    /* ComputerName */
    len = ARRAY_SIZE( buf );
    if (GetComputerNameW( buf, &len ))
        SetEnvironmentVariableW( computernameW, buf );

    /* set the ALLUSERSPROFILE variables */

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, profile_keyW );
    if (!NtOpenKey( &hkey, KEY_READ, &attr ))
    {
        profile_dir = get_reg_value( hkey, profiles_valueW );
        program_data_dir = get_reg_value( hkey, programdataW );
        public_dir = get_reg_value( hkey, public_valueW );
        NtClose( hkey );
    }

    if (program_data_dir)
    {
        SetEnvironmentVariableW( allusersW, program_data_dir );
        SetEnvironmentVariableW( programdataW, program_data_dir );
    }

    if (public_dir)
    {
        SetEnvironmentVariableW( publicW, public_dir );
    }

    HeapFree( GetProcessHeap(), 0, profile_dir );
    HeapFree( GetProcessHeap(), 0, program_data_dir );
    HeapFree( GetProcessHeap(), 0, public_dir );
}

/***********************************************************************
 *           set_wow64_environment
 *
 * Set the environment variables that change across 32/64/Wow64.
 */
static void set_wow64_environment(void)
{
    static const WCHAR archW[]    = {'P','R','O','C','E','S','S','O','R','_','A','R','C','H','I','T','E','C','T','U','R','E',0};
    static const WCHAR arch6432W[] = {'P','R','O','C','E','S','S','O','R','_','A','R','C','H','I','T','E','W','6','4','3','2',0};
    static const WCHAR x86W[] = {'x','8','6',0};
    static const WCHAR versionW[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                     'M','a','c','h','i','n','e','\\',
                                     'S','o','f','t','w','a','r','e','\\',
                                     'M','i','c','r','o','s','o','f','t','\\',
                                     'W','i','n','d','o','w','s','\\',
                                     'C','u','r','r','e','n','t','V','e','r','s','i','o','n',0};
    static const WCHAR progdirW[]   = {'P','r','o','g','r','a','m','F','i','l','e','s','D','i','r',0};
    static const WCHAR progdir86W[] = {'P','r','o','g','r','a','m','F','i','l','e','s','D','i','r',' ','(','x','8','6',')',0};
    static const WCHAR progfilesW[] = {'P','r','o','g','r','a','m','F','i','l','e','s',0};
    static const WCHAR progw6432W[] = {'P','r','o','g','r','a','m','W','6','4','3','2',0};
    static const WCHAR commondirW[]   = {'C','o','m','m','o','n','F','i','l','e','s','D','i','r',0};
    static const WCHAR commondir86W[] = {'C','o','m','m','o','n','F','i','l','e','s','D','i','r',' ','(','x','8','6',')',0};
    static const WCHAR commonfilesW[] = {'C','o','m','m','o','n','P','r','o','g','r','a','m','F','i','l','e','s',0};
    static const WCHAR commonw6432W[] = {'C','o','m','m','o','n','P','r','o','g','r','a','m','W','6','4','3','2',0};
    static const WCHAR winedlldirW[] = {'W','I','N','E','D','L','L','D','I','R','%','u',0};
    static const WCHAR winehomedirW[] = {'W','I','N','E','H','O','M','E','D','I','R',0};
    static const WCHAR winedatadirW[] = {'W','I','N','E','D','A','T','A','D','I','R',0};
    static const WCHAR winebuilddirW[] = {'W','I','N','E','B','U','I','L','D','D','I','R',0};
    static const WCHAR wineconfigdirW[] = {'W','I','N','E','C','O','N','F','I','G','D','I','R',0};

    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    const char *path;
    WCHAR buf[64];
    WCHAR *value;
    HANDLE hkey;
    DWORD i;

    /* set the Wine paths */

    set_wine_path_variable( winedatadirW, wine_get_data_dir() );
    set_wine_path_variable( winehomedirW, getenv("HOME") );
    set_wine_path_variable( winebuilddirW, wine_get_build_dir() );
    set_wine_path_variable( wineconfigdirW, wine_get_config_dir() );
    for (i = 0; (path = wine_dll_enum_load_path( i )); i++)
    {
        sprintfW( buf, winedlldirW, i );
        set_wine_path_variable( buf, path );
    }
    sprintfW( buf, winedlldirW, i );
    set_wine_path_variable( buf, NULL );

    /* set the PROCESSOR_ARCHITECTURE variable */

    if (GetEnvironmentVariableW( arch6432W, buf, ARRAY_SIZE( buf )))
    {
        if (is_win64)
        {
            SetEnvironmentVariableW( archW, buf );
            SetEnvironmentVariableW( arch6432W, NULL );
        }
    }
    else if (GetEnvironmentVariableW( archW, buf, ARRAY_SIZE( buf )))
    {
        if (is_wow64)
        {
            SetEnvironmentVariableW( arch6432W, buf );
            SetEnvironmentVariableW( archW, x86W );
        }
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, versionW );
    if (NtOpenKey( &hkey, KEY_READ | KEY_WOW64_64KEY, &attr )) return;

    /* set the ProgramFiles variables */

    if ((value = get_reg_value( hkey, progdirW )))
    {
        if (is_win64 || is_wow64) SetEnvironmentVariableW( progw6432W, value );
        if (is_win64 || !is_wow64) SetEnvironmentVariableW( progfilesW, value );
        HeapFree( GetProcessHeap(), 0, value );
    }
    if (is_wow64 && (value = get_reg_value( hkey, progdir86W )))
    {
        SetEnvironmentVariableW( progfilesW, value );
        HeapFree( GetProcessHeap(), 0, value );
    }

    /* set the CommonProgramFiles variables */

    if ((value = get_reg_value( hkey, commondirW )))
    {
        if (is_win64 || is_wow64) SetEnvironmentVariableW( commonw6432W, value );
        if (is_win64 || !is_wow64) SetEnvironmentVariableW( commonfilesW, value );
        HeapFree( GetProcessHeap(), 0, value );
    }
    if (is_wow64 && (value = get_reg_value( hkey, commondir86W )))
    {
        SetEnvironmentVariableW( commonfilesW, value );
        HeapFree( GetProcessHeap(), 0, value );
    }

    NtClose( hkey );
}

/***********************************************************************
 *              set_library_wargv
 *
 * Set the Wine library Unicode argv global variables.
 */
static void set_library_wargv( char **argv )
{
    int argc;
    char *q;
    WCHAR *p;
    WCHAR **wargv;
    DWORD total = 0;

    for (argc = 0; argv[argc]; argc++)
        total += MultiByteToWideChar( CP_UNIXCP, 0, argv[argc], -1, NULL, 0 );

    wargv = RtlAllocateHeap( GetProcessHeap(), 0,
                             total * sizeof(WCHAR) + (argc + 1) * sizeof(*wargv) );
    p = (WCHAR *)(wargv + argc + 1);
    for (argc = 0; argv[argc]; argc++)
    {
        DWORD reslen = MultiByteToWideChar( CP_UNIXCP, 0, argv[argc], -1, p, total );
        wargv[argc] = p;
        p += reslen;
        total -= reslen;
    }
    wargv[argc] = NULL;

    /* convert argv back from Unicode since it has to be in the Ansi codepage not the Unix one */

    for (argc = 0; wargv[argc]; argc++)
        total += WideCharToMultiByte( CP_ACP, 0, wargv[argc], -1, NULL, 0, NULL, NULL );

    argv = RtlAllocateHeap( GetProcessHeap(), 0, total + (argc + 1) * sizeof(*argv) );
    q = (char *)(argv + argc + 1);
    for (argc = 0; wargv[argc]; argc++)
    {
        DWORD reslen = WideCharToMultiByte( CP_ACP, 0, wargv[argc], -1, q, total, NULL, NULL );
        argv[argc] = q;
        q += reslen;
        total -= reslen;
    }
    argv[argc] = NULL;

    __wine_main_argc = argc;
    __wine_main_argv = argv;
    __wine_main_wargv = wargv;
}


/***********************************************************************
 *              update_library_argv0
 *
 * Update the argv[0] global variable with the binary we have found.
 */
static void update_library_argv0( const WCHAR *argv0 )
{
    DWORD len = strlenW( argv0 );

    if (len > strlenW( __wine_main_wargv[0] ))
    {
        __wine_main_wargv[0] = RtlAllocateHeap( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) );
    }
    strcpyW( __wine_main_wargv[0], argv0 );

    len = WideCharToMultiByte( CP_ACP, 0, argv0, -1, NULL, 0, NULL, NULL );
    if (len > strlen( __wine_main_argv[0] ) + 1)
    {
        __wine_main_argv[0] = RtlAllocateHeap( GetProcessHeap(), 0, len );
    }
    WideCharToMultiByte( CP_ACP, 0, argv0, -1, __wine_main_argv[0], len, NULL, NULL );
}


/***********************************************************************
 *           build_command_line
 *
 * Build the command line of a process from the argv array.
 *
 * Note that it does NOT necessarily include the file name.
 * Sometimes we don't even have any command line options at all.
 *
 * We must quote and escape characters so that the argv array can be rebuilt
 * from the command line:
 * - spaces and tabs must be quoted
 *   'a b'   -> '"a b"'
 * - quotes must be escaped
 *   '"'     -> '\"'
 * - if '\'s are followed by a '"', they must be doubled and followed by '\"',
 *   resulting in an odd number of '\' followed by a '"'
 *   '\"'    -> '\\\"'
 *   '\\"'   -> '\\\\\"'
 * - '\'s are followed by the closing '"' must be doubled,
 *   resulting in an even number of '\' followed by a '"'
 *   ' \'    -> '" \\"'
 *   ' \\'    -> '" \\\\"'
 * - '\'s that are not followed by a '"' can be left as is
 *   'a\b'   == 'a\b'
 *   'a\\b'  == 'a\\b'
 */
static BOOL build_command_line( WCHAR **argv )
{
    int len;
    WCHAR **arg;
    LPWSTR p;
    RTL_USER_PROCESS_PARAMETERS* rupp = NtCurrentTeb()->Peb->ProcessParameters;

    if (rupp->CommandLine.Buffer) return TRUE; /* already got it from the server */

    len = 0;
    for (arg = argv; *arg; arg++)
    {
        BOOL has_space;
        int bcount;
        WCHAR* a;

        has_space=FALSE;
        bcount=0;
        a=*arg;
        if( !*a ) has_space=TRUE;
        while (*a!='\0') {
            if (*a=='\\') {
                bcount++;
            } else {
                if (*a==' ' || *a=='\t') {
                    has_space=TRUE;
                } else if (*a=='"') {
                    /* doubling of '\' preceding a '"',
                     * plus escaping of said '"'
                     */
                    len+=2*bcount+1;
                }
                bcount=0;
            }
            a++;
        }
        len+=(a-*arg)+1 /* for the separating space */;
        if (has_space)
            len+=2+bcount; /* for the quotes and doubling of '\' preceding the closing quote */
    }

    if (!(rupp->CommandLine.Buffer = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR))))
        return FALSE;

    p = rupp->CommandLine.Buffer;
    rupp->CommandLine.Length = (len - 1) * sizeof(WCHAR);
    rupp->CommandLine.MaximumLength = len * sizeof(WCHAR);
    for (arg = argv; *arg; arg++)
    {
        BOOL has_space,has_quote;
        WCHAR* a;
        int bcount;

        /* Check for quotes and spaces in this argument */
        has_space=has_quote=FALSE;
        a=*arg;
        if( !*a ) has_space=TRUE;
        while (*a!='\0') {
            if (*a==' ' || *a=='\t') {
                has_space=TRUE;
                if (has_quote)
                    break;
            } else if (*a=='"') {
                has_quote=TRUE;
                if (has_space)
                    break;
            }
            a++;
        }

        /* Now transfer it to the command line */
        if (has_space)
            *p++='"';
        if (has_quote || has_space) {
            bcount=0;
            a=*arg;
            while (*a!='\0') {
                if (*a=='\\') {
                    *p++=*a;
                    bcount++;
                } else {
                    if (*a=='"') {
                        int i;

                        /* Double all the '\\' preceding this '"', plus one */
                        for (i=0;i<=bcount;i++)
                            *p++='\\';
                        *p++='"';
                    } else {
                        *p++=*a;
                    }
                    bcount=0;
                }
                a++;
            }
        } else {
            WCHAR* x = *arg;
            while ((*p=*x++)) p++;
        }
        if (has_space) {
            int i;

            /* Double all the '\' preceding the closing quote */
            for (i=0;i<bcount;i++)
                *p++='\\';
            *p++='"';
        }
        *p++=' ';
    }
    if (p > rupp->CommandLine.Buffer)
        p--;  /* remove last space */
    *p = '\0';

    return TRUE;
}


/***********************************************************************
 *           init_current_directory
 *
 * Initialize the current directory from the Unix cwd or the parent info.
 */
static void init_current_directory( CURDIR *cur_dir )
{
    UNICODE_STRING dir_str;
    const char *pwd;
    char *cwd;
    int size;

    /* if we received a cur dir from the parent, try this first */

    if (cur_dir->DosPath.Length)
    {
        if (RtlSetCurrentDirectory_U( &cur_dir->DosPath ) == STATUS_SUCCESS) goto done;
    }

    /* now try to get it from the Unix cwd */

    for (size = 256; ; size *= 2)
    {
        if (!(cwd = HeapAlloc( GetProcessHeap(), 0, size ))) break;
        if (getcwd( cwd, size )) break;
        HeapFree( GetProcessHeap(), 0, cwd );
        if (errno == ERANGE) continue;
        cwd = NULL;
        break;
    }

    /* try to use PWD if it is valid, so that we don't resolve symlinks */

    pwd = getenv( "PWD" );
    if (cwd)
    {
        struct stat st1, st2;

        if (!pwd || stat( pwd, &st1 ) == -1 ||
            (!stat( cwd, &st2 ) && (st1.st_dev != st2.st_dev || st1.st_ino != st2.st_ino)))
            pwd = cwd;
    }

    if (pwd)
    {
        ANSI_STRING unix_name;
        UNICODE_STRING nt_name;
        RtlInitAnsiString( &unix_name, pwd );
        if (!wine_unix_to_nt_file_name( &unix_name, &nt_name ))
        {
            UNICODE_STRING dos_path;
            /* skip the \??\ prefix, nt_name is 0 terminated */
            RtlInitUnicodeString( &dos_path, nt_name.Buffer + 4 );
            RtlSetCurrentDirectory_U( &dos_path );
            RtlFreeUnicodeString( &nt_name );
        }
    }

    if (!cur_dir->DosPath.Length)  /* still not initialized */
    {
        MESSAGE("Warning: could not find DOS drive for current working directory '%s', "
                "starting in the Windows directory.\n", cwd ? cwd : "" );
        RtlInitUnicodeString( &dir_str, DIR_Windows );
        RtlSetCurrentDirectory_U( &dir_str );
    }
    HeapFree( GetProcessHeap(), 0, cwd );

done:
    TRACE( "starting in %s %p\n", debugstr_w( cur_dir->DosPath.Buffer ), cur_dir->Handle );
}


/***********************************************************************
 *           init_windows_dirs
 */
static void init_windows_dirs(void)
{
    static const WCHAR default_syswow64W[] = {'C',':','\\','w','i','n','d','o','w','s',
                                              '\\','s','y','s','w','o','w','6','4',0};

    if (is_win64 || is_wow64)   /* SysWow64 is always defined on 64-bit */
    {
        DIR_SysWow64 = default_syswow64W;
        memcpy( winevdm, default_syswow64W, sizeof(default_syswow64W) - sizeof(WCHAR) );
    }
}


/***********************************************************************
 *           start_wineboot
 *
 * Start the wineboot process if necessary. Return the handles to wait on.
 */
static void start_wineboot( HANDLE handles[2] )
{
    static const WCHAR wineboot_eventW[] = {'_','_','w','i','n','e','b','o','o','t','_','e','v','e','n','t',0};

    handles[1] = 0;
    if (!(handles[0] = CreateEventW( NULL, TRUE, FALSE, wineboot_eventW )))
    {
        ERR( "failed to create wineboot event, expect trouble\n" );
        return;
    }
    if (GetLastError() != ERROR_ALREADY_EXISTS)  /* we created it */
    {
        static const WCHAR wineboot[] = {'\\','w','i','n','e','b','o','o','t','.','e','x','e',0};
        static const WCHAR args[] = {' ','-','-','i','n','i','t',0};
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        void *redir;
        WCHAR app[MAX_PATH];
        WCHAR cmdline[MAX_PATH + ARRAY_SIZE( wineboot ) + ARRAY_SIZE( args )];

        memset( &si, 0, sizeof(si) );
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput  = 0;
        si.hStdOutput = 0;
        si.hStdError  = GetStdHandle( STD_ERROR_HANDLE );

        GetSystemDirectoryW( app, MAX_PATH - ARRAY_SIZE( wineboot ));
        lstrcatW( app, wineboot );

        Wow64DisableWow64FsRedirection( &redir );
        strcpyW( cmdline, app );
        strcatW( cmdline, args );
        if (CreateProcessW( app, cmdline, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi ))
        {
            TRACE( "started wineboot pid %04x tid %04x\n", pi.dwProcessId, pi.dwThreadId );
            CloseHandle( pi.hThread );
            handles[1] = pi.hProcess;
        }
        else
        {
            ERR( "failed to start wineboot, err %u\n", GetLastError() );
            CloseHandle( handles[0] );
            handles[0] = 0;
        }
        Wow64RevertWow64FsRedirection( redir );
    }
}


#ifdef __i386__
extern DWORD call_process_entry( PEB *peb, LPTHREAD_START_ROUTINE entry );
__ASM_GLOBAL_FUNC( call_process_entry,
                    "pushl %ebp\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                    "movl %esp,%ebp\n\t"
                    __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                    "pushl 4(%ebp)\n\t"  /* deliberately mis-align the stack by 8, Doom 3 needs this */
                    "pushl 4(%ebp)\n\t"  /* Driller expects readable address at this offset */
                    "pushl 4(%ebp)\n\t"
                    "pushl 8(%ebp)\n\t"
                    "call *12(%ebp)\n\t"
                    "leave\n\t"
                    __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                    __ASM_CFI(".cfi_same_value %ebp\n\t")
                    "ret" )

extern void WINAPI start_process( LPTHREAD_START_ROUTINE entry, PEB *peb ) DECLSPEC_HIDDEN;
extern void WINAPI start_process_wrapper(void) DECLSPEC_HIDDEN;
__ASM_GLOBAL_FUNC( start_process_wrapper,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %ebx\n\t"  /* arg */
                   "pushl %eax\n\t"  /* entry */
                   "call " __ASM_NAME("start_process") )
#else
static inline DWORD call_process_entry( PEB *peb, LPTHREAD_START_ROUTINE entry )
{
    return entry( peb );
}
static void WINAPI start_process( LPTHREAD_START_ROUTINE entry, PEB *peb );
#define start_process_wrapper start_process
#endif

/***********************************************************************
 *           start_process
 *
 * Startup routine of a new process. Runs on the new process stack.
 */
void WINAPI start_process( LPTHREAD_START_ROUTINE entry, PEB *peb )
{
    BOOL being_debugged;

    if (!entry)
    {
        ERR( "%s doesn't have an entry point, it cannot be executed\n",
             debugstr_w(peb->ProcessParameters->ImagePathName.Buffer) );
        ExitThread( 1 );
    }

    TRACE_(relay)( "\1Starting process %s (entryproc=%p)\n",
                   debugstr_w(peb->ProcessParameters->ImagePathName.Buffer), entry );

    __TRY
    {
        if (!CheckRemoteDebuggerPresent( GetCurrentProcess(), &being_debugged ))
            being_debugged = FALSE;

        SetLastError( 0 );  /* clear error code */
        if (being_debugged) DbgBreakPoint();
        ExitThread( call_process_entry( peb, entry ));
    }
    __EXCEPT(UnhandledExceptionFilter)
    {
        TerminateProcess( GetCurrentProcess(), GetExceptionCode() );
    }
    __ENDTRY
    abort();  /* should not be reached */
}


/***********************************************************************
 *           set_process_name
 *
 * Change the process name in the ps output.
 */
static void set_process_name( int argc, char *argv[] )
{
    BOOL shift_strings;
    char *p, *name;
    int i;

#ifdef HAVE_SETPROCTITLE
    setproctitle("-%s", argv[1]);
    shift_strings = FALSE;
#else
    p = argv[0];

    shift_strings = (argc >= 2);
    for (i = 1; i < argc; i++)
    {
        p += strlen(p) + 1;
        if (p != argv[i])
        {
            shift_strings = FALSE;
            break;
        }
    }
#endif

    if (shift_strings)
    {
        int offset = argv[1] - argv[0];
        char *end = argv[argc-1] + strlen(argv[argc-1]) + 1;
        memmove( argv[0], argv[1], end - argv[1] );
        memset( end - offset, 0, offset );
        for (i = 1; i < argc; i++)
            argv[i-1] = argv[i] - offset;
        argv[i-1] = NULL;
    }
    else
    {
        /* remove argv[0] */
        memmove( argv, argv + 1, argc * sizeof(argv[0]) );
    }

    name = argv[0];
    if ((p = strrchr( name, '\\' ))) name = p + 1;
    if ((p = strrchr( name, '/' ))) name = p + 1;

#if defined(HAVE_SETPROGNAME)
    setprogname( name );
#endif

#ifdef HAVE_PRCTL
#ifndef PR_SET_NAME
# define PR_SET_NAME 15
#endif
    prctl( PR_SET_NAME, name );
#endif  /* HAVE_PRCTL */
}


/***********************************************************************
 *           __wine_kernel_init
 *
 * Wine initialisation: load and start the main exe file.
 */
void * CDECL __wine_kernel_init(void)
{
    static const WCHAR kernel32W[] = {'k','e','r','n','e','l','3','2',0};
    static const WCHAR dotW[] = {'.',0};

    WCHAR *p, main_exe_name[MAX_PATH+1];
    PEB *peb = NtCurrentTeb()->Peb;
    RTL_USER_PROCESS_PARAMETERS *params = peb->ProcessParameters;
    HANDLE boot_events[2];
    BOOL got_environment = TRUE;

    /* Initialize everything */

    setbuf(stdout,NULL);
    setbuf(stderr,NULL);
    kernel32_handle = GetModuleHandleW(kernel32W);
    IsWow64Process( GetCurrentProcess(), &is_wow64 );
    RtlSetUnhandledExceptionFilter( UnhandledExceptionFilter );

    LOCALE_Init();

    if (!params->Environment)
    {
        /* Copy the parent environment */
        if (!build_initial_environment()) exit(1);

        /* convert old configuration to new format */
        convert_old_config();

        got_environment = set_registry_environment( FALSE );
        set_additional_environment();
    }

    init_windows_dirs();
    init_current_directory( &params->CurrentDirectory );

    set_process_name( __wine_main_argc, __wine_main_argv );
    set_library_wargv( __wine_main_argv );
    boot_events[0] = boot_events[1] = 0;

    if (peb->ProcessParameters->ImagePathName.Buffer)
    {
        strcpyW( main_exe_name, peb->ProcessParameters->ImagePathName.Buffer );
    }
    else
    {
        BOOL is_64bit;

        if (!SearchPathW( NULL, __wine_main_wargv[0], exeW, MAX_PATH, main_exe_name, NULL ) &&
            !get_builtin_path( __wine_main_wargv[0], exeW, main_exe_name, MAX_PATH, &is_64bit ))
        {
            MESSAGE( "wine: cannot find '%s'\n", __wine_main_argv[0] );
            ExitProcess( GetLastError() );
        }
        update_library_argv0( main_exe_name );
        if (!build_command_line( __wine_main_wargv )) goto error;
        start_wineboot( boot_events );
    }

    /* if there's no extension, append a dot to prevent LoadLibrary from appending .dll */
    p = strrchrW( main_exe_name, '.' );
    if (!p || strchrW( p, '/' ) || strchrW( p, '\\' )) strcatW( main_exe_name, dotW );

    TRACE( "starting process name=%s argv[0]=%s\n",
           debugstr_w(main_exe_name), debugstr_w(__wine_main_wargv[0]) );

    RtlInitUnicodeString( &NtCurrentTeb()->Peb->ProcessParameters->DllPath,
                          MODULE_get_dll_load_path( main_exe_name, -1 ));

    if (boot_events[0])
    {
        DWORD timeout = 2 * 60 * 1000, count = 1;

        if (boot_events[1]) count++;
        if (!got_environment) timeout = 5 * 60 * 1000;  /* initial prefix creation can take longer */
        if (WaitForMultipleObjects( count, boot_events, FALSE, timeout ) == WAIT_TIMEOUT)
            ERR( "boot event wait timed out\n" );
        CloseHandle( boot_events[0] );
        if (boot_events[1]) CloseHandle( boot_events[1] );
        /* reload environment now that wineboot has run */
        set_registry_environment( got_environment );
        set_additional_environment();
    }
    set_wow64_environment();

    if (!(peb->ImageBaseAddress = LoadLibraryExW( main_exe_name, 0, DONT_RESOLVE_DLL_REFERENCES )))
    {
        DWORD_PTR args[1];
        WCHAR msgW[1024];
        char msg[1024];
        DWORD error = GetLastError();

        /* if Win16/DOS format, or unavailable address, exec a new process with the proper setup */
        if (error == ERROR_BAD_EXE_FORMAT ||
            error == ERROR_INVALID_ADDRESS ||
            error == ERROR_NOT_ENOUGH_MEMORY)
        {
            if (!getenv("WINEPRELOADRESERVE")) exec_process( main_exe_name );
            /* if we get back here, it failed */
        }
        else if (error == ERROR_MOD_NOT_FOUND)
        {
            if (!strcmpiW( main_exe_name, winevdm ) && __wine_main_argc > 3)
            {
                /* args 1 and 2 are --app-name full_path */
                MESSAGE( "wine: could not run %s: 16-bit/DOS support missing\n",
                         debugstr_w(__wine_main_wargv[3]) );
                ExitProcess( ERROR_BAD_EXE_FORMAT );
            }
            MESSAGE( "wine: cannot find %s\n", debugstr_w(main_exe_name) );
            ExitProcess( ERROR_FILE_NOT_FOUND );
        }
        args[0] = (DWORD_PTR)main_exe_name;
        FormatMessageW( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        NULL, error, 0, msgW, ARRAY_SIZE( msgW ), (__ms_va_list *)args );
        WideCharToMultiByte( CP_UNIXCP, 0, msgW, -1, msg, sizeof(msg), NULL, NULL );
        MESSAGE( "wine: %s", msg );
        ExitProcess( error );
    }

    if (!params->CurrentDirectory.Handle) chdir("/"); /* avoid locking removable devices */

    return start_process_wrapper;

 error:
    ExitProcess( GetLastError() );
}


/***********************************************************************
 *           build_argv
 *
 * Build an argv array from a command-line.
 * 'reserved' is the number of args to reserve before the first one.
 */
static char **build_argv( const UNICODE_STRING *cmdlineW, int reserved )
{
    int argc;
    char** argv;
    char *arg,*s,*d,*cmdline;
    int in_quotes,bcount,len;

    len = WideCharToMultiByte( CP_UNIXCP, 0, cmdlineW->Buffer, cmdlineW->Length / sizeof(WCHAR),
                               NULL, 0, NULL, NULL );
    if (!(cmdline = HeapAlloc( GetProcessHeap(), 0, len + 1 ))) return NULL;
    WideCharToMultiByte( CP_UNIXCP, 0, cmdlineW->Buffer, cmdlineW->Length / sizeof(WCHAR),
                         cmdline, len, NULL, NULL );
    cmdline[len++] = 0;

    argc=reserved+1;
    bcount=0;
    in_quotes=0;
    s=cmdline;
    while (1) {
        if (*s=='\0' || ((*s==' ' || *s=='\t') && !in_quotes)) {
            /* space */
            argc++;
            /* skip the remaining spaces */
            while (*s==' ' || *s=='\t') {
                s++;
            }
            if (*s=='\0')
                break;
            bcount=0;
            continue;
        } else if (*s=='\\') {
            /* '\', count them */
            bcount++;
        } else if ((*s=='"') && ((bcount & 1)==0)) {
            if (in_quotes && s[1] == '"') {
               s++;
            } else {
               /* unescaped '"' */
               in_quotes=!in_quotes;
               bcount=0;
            }
        } else {
            /* a regular character */
            bcount=0;
        }
        s++;
    }
    if (!(argv = HeapAlloc( GetProcessHeap(), 0, argc*sizeof(*argv) + len )))
    {
        HeapFree( GetProcessHeap(), 0, cmdline );
        return NULL;
    }

    arg = d = s = (char *)(argv + argc);
    memcpy( d, cmdline, len );
    bcount=0;
    in_quotes=0;
    argc=reserved;
    while (*s) {
        if ((*s==' ' || *s=='\t') && !in_quotes) {
            /* Close the argument and copy it */
            *d=0;
            argv[argc++]=arg;

            /* skip the remaining spaces */
            do {
                s++;
            } while (*s==' ' || *s=='\t');

            /* Start with a new argument */
            arg=d=s;
            bcount=0;
        } else if (*s=='\\') {
            /* '\\' */
            *d++=*s++;
            bcount++;
        } else if (*s=='"') {
            /* '"' */
            if ((bcount & 1)==0) {
                /* Preceded by an even number of '\', this is half that
                 * number of '\', plus a '"' which we discard.
                 */
                d-=bcount/2;
                s++;
                if(in_quotes && *s == '"') {
                  *d++='"';
                  s++;
                } else {
                  in_quotes=!in_quotes;
                }
            } else {
                /* Preceded by an odd number of '\', this is half that
                 * number of '\' followed by a '"'
                 */
                d=d-bcount/2-1;
                *d++='"';
                s++;
            }
            bcount=0;
        } else {
            /* a regular character */
            *d++=*s++;
            bcount=0;
        }
    }
    if (*arg) {
        *d='\0';
        argv[argc++]=arg;
    }
    argv[argc]=NULL;

    HeapFree( GetProcessHeap(), 0, cmdline );
    return argv;
}


/***********************************************************************
 *           build_envp
 *
 * Build the environment of a new child process.
 */
static char **build_envp( const WCHAR *envW )
{
    static const char * const unix_vars[] = { "PATH", "TEMP", "TMP", "HOME" };

    const WCHAR *end;
    char **envp;
    char *env, *p;
    int count = 1, length;
    unsigned int i;

    for (end = envW; *end; count++) end += strlenW(end) + 1;
    end++;
    length = WideCharToMultiByte( CP_UNIXCP, 0, envW, end - envW, NULL, 0, NULL, NULL );
    if (!(env = HeapAlloc( GetProcessHeap(), 0, length ))) return NULL;
    WideCharToMultiByte( CP_UNIXCP, 0, envW, end - envW, env, length, NULL, NULL );

    for (p = env; *p; p += strlen(p) + 1)
        if (is_special_env_var( p )) length += 4; /* prefix it with "WINE" */

    for (i = 0; i < ARRAY_SIZE( unix_vars ); i++)
    {
        if (!(p = getenv(unix_vars[i]))) continue;
        length += strlen(unix_vars[i]) + strlen(p) + 2;
        count++;
    }

    if ((envp = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*envp) + length )))
    {
        char **envptr = envp;
        char *dst = (char *)(envp + count);

        /* some variables must not be modified, so we get them directly from the unix env */
        for (i = 0; i < ARRAY_SIZE( unix_vars ); i++)
        {
            if (!(p = getenv(unix_vars[i]))) continue;
            *envptr++ = strcpy( dst, unix_vars[i] );
            strcat( dst, "=" );
            strcat( dst, p );
            dst += strlen(dst) + 1;
        }

        /* now put the Windows environment strings */
        for (p = env; *p; p += strlen(p) + 1)
        {
            if (*p == '=') continue;  /* skip drive curdirs, this crashes some unix apps */
            if (!strncmp( p, "WINEPRELOADRESERVE=", sizeof("WINEPRELOADRESERVE=")-1 )) continue;
            if (!strncmp( p, "WINELOADERNOEXEC=", sizeof("WINELOADERNOEXEC=")-1 )) continue;
            if (!strncmp( p, "WINESERVERSOCKET=", sizeof("WINESERVERSOCKET=")-1 )) continue;
            if (is_special_env_var( p ))  /* prefix it with "WINE" */
            {
                *envptr++ = strcpy( dst, "WINE" );
                strcat( dst, p );
            }
            else
            {
                *envptr++ = strcpy( dst, p );
            }
            dst += strlen(dst) + 1;
        }
        *envptr = 0;
    }
    HeapFree( GetProcessHeap(), 0, env );
    return envp;
}


/***********************************************************************
 *           fork_and_exec
 *
 * Fork and exec a new Unix binary, checking for errors.
 */
static int fork_and_exec( const RTL_USER_PROCESS_PARAMETERS *params, const char *newdir )
{
    int fd[2], stdin_fd = -1, stdout_fd = -1, stderr_fd = -1;
    int pid, err;
    char *filename, **argv, **envp;

    if (!(filename = wine_get_unix_file_name( params->ImagePathName.Buffer ))) return -1;

#ifdef HAVE_PIPE2
    if (pipe2( fd, O_CLOEXEC ) == -1)
#endif
    {
        if (pipe(fd) == -1)
        {
            SetLastError( ERROR_TOO_MANY_OPEN_FILES );
            HeapFree( GetProcessHeap(), 0, filename );
            return -1;
        }
        fcntl( fd[0], F_SETFD, FD_CLOEXEC );
        fcntl( fd[1], F_SETFD, FD_CLOEXEC );
    }

    wine_server_handle_to_fd( params->hStdInput, FILE_READ_DATA, &stdin_fd, NULL );
    wine_server_handle_to_fd( params->hStdOutput, FILE_WRITE_DATA, &stdout_fd, NULL );
    wine_server_handle_to_fd( params->hStdError, FILE_WRITE_DATA, &stderr_fd, NULL );

    argv = build_argv( &params->CommandLine, 0 );
    envp = build_envp( params->Environment );

    if (!(pid = fork()))  /* child */
    {
        if (!(pid = fork()))  /* grandchild */
        {
            close( fd[0] );

            if (params->ConsoleFlags || params->ConsoleHandle == KERNEL32_CONSOLE_ALLOC ||
                (params->hStdInput == INVALID_HANDLE_VALUE && params->hStdOutput == INVALID_HANDLE_VALUE))
            {
                int nullfd = open( "/dev/null", O_RDWR );
                setsid();
                /* close stdin and stdout */
                if (nullfd != -1)
                {
                    dup2( nullfd, 0 );
                    dup2( nullfd, 1 );
                    close( nullfd );
                }
            }
            else
            {
                if (stdin_fd != -1)
                {
                    dup2( stdin_fd, 0 );
                    close( stdin_fd );
                }
                if (stdout_fd != -1)
                {
                    dup2( stdout_fd, 1 );
                    close( stdout_fd );
                }
                if (stderr_fd != -1)
                {
                    dup2( stderr_fd, 2 );
                    close( stderr_fd );
                }
            }

            /* Reset signals that we previously set to SIG_IGN */
            signal( SIGPIPE, SIG_DFL );

            if (newdir) chdir(newdir);

            if (argv && envp) execve( filename, argv, envp );
        }

        if (pid <= 0)  /* grandchild if exec failed or child if fork failed */
        {
            err = errno;
            write( fd[1], &err, sizeof(err) );
            _exit(1);
        }

        _exit(0); /* child if fork succeeded */
    }
    HeapFree( GetProcessHeap(), 0, argv );
    HeapFree( GetProcessHeap(), 0, envp );
    HeapFree( GetProcessHeap(), 0, filename );
    if (stdin_fd != -1) close( stdin_fd );
    if (stdout_fd != -1) close( stdout_fd );
    if (stderr_fd != -1) close( stderr_fd );
    close( fd[1] );
    if (pid != -1)
    {
        /* reap child */
        do {
            err = waitpid(pid, NULL, 0);
        } while (err < 0 && errno == EINTR);

        if (read( fd[0], &err, sizeof(err) ) > 0)  /* exec or second fork failed */
        {
            errno = err;
            pid = -1;
        }
    }
    if (pid == -1) FILE_SetDosError();
    close( fd[0] );
    return pid;
}


static inline const WCHAR *get_params_string( const RTL_USER_PROCESS_PARAMETERS *params,
                                              const UNICODE_STRING *str )
{
    if (params->Flags & PROCESS_PARAMS_FLAG_NORMALIZED) return str->Buffer;
    return (const WCHAR *)((const char *)params + (UINT_PTR)str->Buffer);
}

static inline DWORD append_string( void **ptr, const RTL_USER_PROCESS_PARAMETERS *params,
                                   const UNICODE_STRING *str )
{
    const WCHAR *buffer = get_params_string( params, str );
    memcpy( *ptr, buffer, str->Length );
    *ptr = (WCHAR *)*ptr + str->Length / sizeof(WCHAR);
    return str->Length;
}

/***********************************************************************
 *           create_startup_info
 */
static startup_info_t *create_startup_info( const RTL_USER_PROCESS_PARAMETERS *params, DWORD *info_size )
{
    startup_info_t *info;
    DWORD size;
    void *ptr;

    size = sizeof(*info);
    size += params->CurrentDirectory.DosPath.Length;
    size += params->DllPath.Length;
    size += params->ImagePathName.Length;
    size += params->CommandLine.Length;
    size += params->WindowTitle.Length;
    size += params->Desktop.Length;
    size += params->ShellInfo.Length;
    size += params->RuntimeInfo.Length;
    size = (size + 1) & ~1;
    *info_size = size;

    if (!(info = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, size ))) return NULL;

    info->console_flags = params->ConsoleFlags;
    info->console       = wine_server_obj_handle( params->ConsoleHandle );
    info->hstdin        = wine_server_obj_handle( params->hStdInput );
    info->hstdout       = wine_server_obj_handle( params->hStdOutput );
    info->hstderr       = wine_server_obj_handle( params->hStdError );
    info->x             = params->dwX;
    info->y             = params->dwY;
    info->xsize         = params->dwXSize;
    info->ysize         = params->dwYSize;
    info->xchars        = params->dwXCountChars;
    info->ychars        = params->dwYCountChars;
    info->attribute     = params->dwFillAttribute;
    info->flags         = params->dwFlags;
    info->show          = params->wShowWindow;

    ptr = info + 1;
    info->curdir_len = append_string( &ptr, params, &params->CurrentDirectory.DosPath );
    info->dllpath_len = append_string( &ptr, params, &params->DllPath );
    info->imagepath_len = append_string( &ptr, params, &params->ImagePathName );
    info->cmdline_len = append_string( &ptr, params, &params->CommandLine );
    info->title_len = append_string( &ptr, params, &params->WindowTitle );
    info->desktop_len = append_string( &ptr, params, &params->Desktop );
    info->shellinfo_len = append_string( &ptr, params, &params->ShellInfo );
    info->runtime_len = append_string( &ptr, params, &params->RuntimeInfo );
    return info;
}


/***********************************************************************
 *           create_process_params
 */
static RTL_USER_PROCESS_PARAMETERS *create_process_params( LPCWSTR filename, LPCWSTR cmdline,
                                                           LPCWSTR cur_dir, void *env, DWORD flags,
                                                           const STARTUPINFOW *startup )
{
    RTL_USER_PROCESS_PARAMETERS *params;
    UNICODE_STRING imageW, curdirW, cmdlineW, titleW, desktopW, runtimeW, newdirW;
    WCHAR imagepath[MAX_PATH];
    WCHAR *envW = env;

    if(!GetLongPathNameW( filename, imagepath, MAX_PATH ))
        lstrcpynW( imagepath, filename, MAX_PATH );
    if(!GetFullPathNameW( imagepath, MAX_PATH, imagepath, NULL ))
        lstrcpynW( imagepath, filename, MAX_PATH );

    if (env && !(flags & CREATE_UNICODE_ENVIRONMENT))  /* convert environment to unicode */
    {
        char *e = env;
        DWORD lenW;

        while (*e) e += strlen(e) + 1;
        e++;  /* final null */
        lenW = MultiByteToWideChar( CP_ACP, 0, env, e - (char *)env, NULL, 0 );
        if ((envW = HeapAlloc( GetProcessHeap(), 0, lenW * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, env, e - (char *)env, envW, lenW );
    }

    newdirW.Buffer = NULL;
    if (cur_dir)
    {
        if (RtlDosPathNameToNtPathName_U( cur_dir, &newdirW, NULL, NULL ))
            cur_dir = newdirW.Buffer + 4;  /* skip \??\ prefix */
        else
            cur_dir = NULL;
    }
    RtlInitUnicodeString( &imageW, imagepath );
    RtlInitUnicodeString( &curdirW, cur_dir );
    RtlInitUnicodeString( &cmdlineW, cmdline );
    RtlInitUnicodeString( &titleW, startup->lpTitle ? startup->lpTitle : imagepath );
    RtlInitUnicodeString( &desktopW, startup->lpDesktop );
    runtimeW.Buffer = (WCHAR *)startup->lpReserved2;
    runtimeW.Length = runtimeW.MaximumLength = startup->cbReserved2;
    if (RtlCreateProcessParametersEx( &params, &imageW, NULL, cur_dir ? &curdirW : NULL,
                                      &cmdlineW, envW, &titleW, &desktopW,
                                      NULL, &runtimeW, PROCESS_PARAMS_FLAG_NORMALIZED ))
    {
        if (envW != env) HeapFree( GetProcessHeap(), 0, envW );
        return NULL;
    }

    if (flags & CREATE_NEW_PROCESS_GROUP) params->ConsoleFlags = 1;
    if (flags & CREATE_NEW_CONSOLE) params->ConsoleHandle = KERNEL32_CONSOLE_ALLOC;

    if (startup->dwFlags & STARTF_USESTDHANDLES)
    {
        params->hStdInput  = startup->hStdInput;
        params->hStdOutput = startup->hStdOutput;
        params->hStdError  = startup->hStdError;
    }
    else if (flags & DETACHED_PROCESS)
    {
        params->hStdInput  = INVALID_HANDLE_VALUE;
        params->hStdOutput = INVALID_HANDLE_VALUE;
        params->hStdError  = INVALID_HANDLE_VALUE;
    }
    else
    {
        params->hStdInput  = NtCurrentTeb()->Peb->ProcessParameters->hStdInput;
        params->hStdOutput = NtCurrentTeb()->Peb->ProcessParameters->hStdOutput;
        params->hStdError  = NtCurrentTeb()->Peb->ProcessParameters->hStdError;
    }

    if (flags & CREATE_NEW_CONSOLE)
    {
        /* this is temporary (for console handles). We have no way to control that the handle is invalid in child process otherwise */
        if (is_console_handle(params->hStdInput))  params->hStdInput  = INVALID_HANDLE_VALUE;
        if (is_console_handle(params->hStdOutput)) params->hStdOutput = INVALID_HANDLE_VALUE;
        if (is_console_handle(params->hStdError))  params->hStdError  = INVALID_HANDLE_VALUE;
    }
    else
    {
        if (is_console_handle(params->hStdInput))  params->hStdInput  = (HANDLE)((UINT_PTR)params->hStdInput & ~3);
        if (is_console_handle(params->hStdOutput)) params->hStdOutput = (HANDLE)((UINT_PTR)params->hStdOutput & ~3);
        if (is_console_handle(params->hStdError))  params->hStdError  = (HANDLE)((UINT_PTR)params->hStdError & ~3);
    }

    params->dwX             = startup->dwX;
    params->dwY             = startup->dwY;
    params->dwXSize         = startup->dwXSize;
    params->dwYSize         = startup->dwYSize;
    params->dwXCountChars   = startup->dwXCountChars;
    params->dwYCountChars   = startup->dwYCountChars;
    params->dwFillAttribute = startup->dwFillAttribute;
    params->dwFlags         = startup->dwFlags;
    params->wShowWindow     = startup->wShowWindow;

    if (envW != env) HeapFree( GetProcessHeap(), 0, envW );
    return params;
}

/***********************************************************************
 *           get_alternate_loader
 *
 * Get the name of the alternate (32 or 64 bit) Wine loader.
 */
static const char *get_alternate_loader( char **ret_env )
{
    char *env;
    const char *loader = NULL;
    const char *loader_env = getenv( "WINELOADER" );

    *ret_env = NULL;

    if (wine_get_build_dir()) loader = is_win64 ? "loader/wine" : "loader/wine64";

    if (loader_env)
    {
        int len = strlen( loader_env );
        if (!is_win64)
        {
            if (!(env = HeapAlloc( GetProcessHeap(), 0, sizeof("WINELOADER=") + len + 2 ))) return NULL;
            strcpy( env, "WINELOADER=" );
            strcat( env, loader_env );
            strcat( env, "64" );
        }
        else
        {
            if (!(env = HeapAlloc( GetProcessHeap(), 0, sizeof("WINELOADER=") + len ))) return NULL;
            strcpy( env, "WINELOADER=" );
            strcat( env, loader_env );
            len += sizeof("WINELOADER=") - 1;
            if (!strcmp( env + len - 2, "64" )) env[len - 2] = 0;
        }
        if (!loader)
        {
            if ((loader = strrchr( env, '/' ))) loader++;
            else loader = env;
        }
        *ret_env = env;
    }
    if (!loader) loader = is_win64 ? "wine" : "wine64";
    return loader;
}

#ifdef __APPLE__
/***********************************************************************
 *           terminate_main_thread
 *
 * On some versions of Mac OS X, the execve system call fails with
 * ENOTSUP if the process has multiple threads.  Wine is always multi-
 * threaded on Mac OS X because it specifically reserves the main thread
 * for use by the system frameworks (see apple_main_thread() in
 * libs/wine/loader.c).  So, when we need to exec without first forking,
 * we need to terminate the main thread first.  We do this by installing
 * a custom run loop source onto the main run loop and signaling it.
 * The source's "perform" callback is pthread_exit and it will be
 * executed on the main thread, terminating it.
 *
 * Returns TRUE if there's still hope the main thread has terminated or
 * will soon.  Return FALSE if we've given up.
 */
static BOOL terminate_main_thread(void)
{
    static int delayms;

    if (!delayms)
    {
        CFRunLoopSourceContext source_context = { 0 };
        CFRunLoopSourceRef source;

        source_context.perform = pthread_exit;
        if (!(source = CFRunLoopSourceCreate( NULL, 0, &source_context )))
            return FALSE;

        CFRunLoopAddSource( CFRunLoopGetMain(), source, kCFRunLoopCommonModes );
        CFRunLoopSourceSignal( source );
        CFRunLoopWakeUp( CFRunLoopGetMain() );
        CFRelease( source );

        delayms = 20;
    }

    if (delayms > 1000)
        return FALSE;

    usleep(delayms * 1000);
    delayms *= 2;

    return TRUE;
}
#endif


/***********************************************************************
 *           set_stdio_fd
 */
static void set_stdio_fd( int stdin_fd, int stdout_fd )
{
    int fd = -1;

    if (stdin_fd == -1 || stdout_fd == -1)
    {
        fd = open( "/dev/null", O_RDWR );
        if (stdin_fd == -1) stdin_fd = fd;
        if (stdout_fd == -1) stdout_fd = fd;
    }

    dup2( stdin_fd, 0 );
    dup2( stdout_fd, 1 );
    if (fd != -1) close( fd );
}


/***********************************************************************
 *           spawn_loader
 */
static pid_t spawn_loader( const RTL_USER_PROCESS_PARAMETERS *params, int socketfd,
                          const char *unixdir, char *winedebug, const pe_image_info_t *pe_info )
{
    pid_t pid;
    int stdin_fd = -1, stdout_fd = -1;
    char *wineloader = NULL;
    const char *loader = NULL;
    char **argv;

    argv = build_argv( &params->CommandLine, 1 );

    if (!is_win64 ^ !is_64bit_arch( pe_info->cpu ))
        loader = get_alternate_loader( &wineloader );

    wine_server_handle_to_fd( params->hStdInput, FILE_READ_DATA, &stdin_fd, NULL );
    wine_server_handle_to_fd( params->hStdOutput, FILE_WRITE_DATA, &stdout_fd, NULL );

    if (!(pid = fork()))  /* child */
    {
        if (!(pid = fork()))  /* grandchild */
        {
            char preloader_reserve[64], socket_env[64];
            ULONGLONG res_start = pe_info->base;
            ULONGLONG res_end   = pe_info->base + pe_info->map_size;

            if (params->ConsoleFlags || params->ConsoleHandle == KERNEL32_CONSOLE_ALLOC ||
                (params->hStdInput == INVALID_HANDLE_VALUE && params->hStdOutput == INVALID_HANDLE_VALUE))
            {
                setsid();
                set_stdio_fd( -1, -1 );  /* close stdin and stdout */
            }
            else set_stdio_fd( stdin_fd, stdout_fd );

            if (stdin_fd != -1) close( stdin_fd );
            if (stdout_fd != -1) close( stdout_fd );

            /* Reset signals that we previously set to SIG_IGN */
            signal( SIGPIPE, SIG_DFL );

            sprintf( socket_env, "WINESERVERSOCKET=%u", socketfd );
            sprintf( preloader_reserve, "WINEPRELOADRESERVE=%x%08x-%x%08x",
                     (ULONG)(res_start >> 32), (ULONG)res_start, (ULONG)(res_end >> 32), (ULONG)res_end );

            putenv( preloader_reserve );
            putenv( socket_env );
            if (winedebug) putenv( winedebug );
            if (wineloader) putenv( wineloader );
            if (unixdir) chdir(unixdir);

            if (argv) wine_exec_wine_binary( loader, argv, getenv("WINELOADER") );
            _exit(1);
        }

        _exit(pid == -1);
    }

    if (pid != -1)
    {
        /* reap child */
        pid_t wret;
        do {
            wret = waitpid(pid, NULL, 0);
        } while (wret < 0 && errno == EINTR);
    }

    if (stdin_fd != -1) close( stdin_fd );
    if (stdout_fd != -1) close( stdout_fd );
    HeapFree( GetProcessHeap(), 0, wineloader );
    HeapFree( GetProcessHeap(), 0, argv );
    return pid;
}

/***********************************************************************
 *           exec_loader
 */
static NTSTATUS exec_loader( const RTL_USER_PROCESS_PARAMETERS *params, int socketfd,
                             const pe_image_info_t *pe_info )
{
    char *wineloader = NULL;
    const char *loader = NULL;
    char **argv;
    char preloader_reserve[64], socket_env[64];
    ULONGLONG res_start = pe_info->base;
    ULONGLONG res_end   = pe_info->base + pe_info->map_size;

    if (!(argv = build_argv( &params->CommandLine, 1 ))) return STATUS_NO_MEMORY;

    if (!is_win64 ^ !is_64bit_arch( pe_info->cpu ))
        loader = get_alternate_loader( &wineloader );

    /* Reset signals that we previously set to SIG_IGN */
    signal( SIGPIPE, SIG_DFL );

    sprintf( socket_env, "WINESERVERSOCKET=%u", socketfd );
    sprintf( preloader_reserve, "WINEPRELOADRESERVE=%x%08x-%x%08x",
             (ULONG)(res_start >> 32), (ULONG)res_start, (ULONG)(res_end >> 32), (ULONG)res_end );

    putenv( preloader_reserve );
    putenv( socket_env );
    if (wineloader) putenv( wineloader );

    do
    {
        wine_exec_wine_binary( loader, argv, getenv("WINELOADER") );
    }
#ifdef __APPLE__
    while (errno == ENOTSUP && terminate_main_thread());
#else
    while (0);
#endif

    HeapFree( GetProcessHeap(), 0, wineloader );
    HeapFree( GetProcessHeap(), 0, argv );
    return STATUS_INVALID_IMAGE_FORMAT;
}

/* creates a struct security_descriptor and contained information in one contiguous piece of memory */
static NTSTATUS alloc_object_attributes( const SECURITY_ATTRIBUTES *attr, struct object_attributes **ret,
                                         data_size_t *ret_len )
{
    unsigned int len = sizeof(**ret);
    PSID owner = NULL, group = NULL;
    ACL *dacl, *sacl;
    BOOLEAN dacl_present, sacl_present, defaulted;
    PSECURITY_DESCRIPTOR sd = NULL;
    NTSTATUS status;

    *ret = NULL;
    *ret_len = 0;

    if (attr) sd = attr->lpSecurityDescriptor;

    if (sd)
    {
        len += sizeof(struct security_descriptor);

        if ((status = RtlGetOwnerSecurityDescriptor( sd, &owner, &defaulted ))) return status;
        if ((status = RtlGetGroupSecurityDescriptor( sd, &group, &defaulted ))) return status;
        if ((status = RtlGetSaclSecurityDescriptor( sd, &sacl_present, &sacl, &defaulted ))) return status;
        if ((status = RtlGetDaclSecurityDescriptor( sd, &dacl_present, &dacl, &defaulted ))) return status;
        if (owner) len += RtlLengthSid( owner );
        if (group) len += RtlLengthSid( group );
        if (sacl_present && sacl) len += sacl->AclSize;
        if (dacl_present && dacl) len += dacl->AclSize;
    }

    len = (len + 3) & ~3;  /* DWORD-align the entire structure */

    *ret = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len );
    if (!*ret) return STATUS_NO_MEMORY;

    (*ret)->attributes = (attr && attr->bInheritHandle) ? OBJ_INHERIT : 0;

    if (sd)
    {
        struct security_descriptor *descr = (struct security_descriptor *)(*ret + 1);
        unsigned char *ptr = (unsigned char *)(descr + 1);

        descr->control = ((SECURITY_DESCRIPTOR *)sd)->Control & ~SE_SELF_RELATIVE;
        if (owner) descr->owner_len = RtlLengthSid( owner );
        if (group) descr->group_len = RtlLengthSid( group );
        if (sacl_present && sacl) descr->sacl_len = sacl->AclSize;
        if (dacl_present && dacl) descr->dacl_len = dacl->AclSize;

        memcpy( ptr, owner, descr->owner_len );
        ptr += descr->owner_len;
        memcpy( ptr, group, descr->group_len );
        ptr += descr->group_len;
        memcpy( ptr, sacl, descr->sacl_len );
        ptr += descr->sacl_len;
        memcpy( ptr, dacl, descr->dacl_len );
        (*ret)->sd_len = (sizeof(*descr) + descr->owner_len + descr->group_len + descr->sacl_len +
                          descr->dacl_len + sizeof(WCHAR) - 1) & ~(sizeof(WCHAR) - 1);
    }
    *ret_len = len;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           replace_process
 *
 * Replace the existing process by exec'ing a new one.
 */
static BOOL replace_process( HANDLE handle, const RTL_USER_PROCESS_PARAMETERS *params,
                             const pe_image_info_t *pe_info )
{
    NTSTATUS status;
    int socketfd[2];

    if (socketpair( PF_UNIX, SOCK_STREAM, 0, socketfd ) == -1)
    {
        SetLastError( ERROR_TOO_MANY_OPEN_FILES );
        return FALSE;
    }
#ifdef SO_PASSCRED
    else
    {
        int enable = 1;
        setsockopt( socketfd[0], SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable) );
    }
#endif
    wine_server_send_fd( socketfd[1] );
    close( socketfd[1] );

    SERVER_START_REQ( exec_process )
    {
        req->socket_fd      = socketfd[1];
        req->exe_file       = wine_server_obj_handle( handle );
        req->cpu            = pe_info->cpu;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    switch (status)
    {
    case STATUS_INVALID_IMAGE_WIN_64:
        ERR( "64-bit application %s not supported in 32-bit prefix\n",
             debugstr_w( params->ImagePathName.Buffer ));
        break;
    case STATUS_INVALID_IMAGE_FORMAT:
        ERR( "%s not supported on this installation (%s binary)\n",
             debugstr_w( params->ImagePathName.Buffer ), cpu_names[pe_info->cpu] );
        break;
    case STATUS_SUCCESS:
        status = exec_loader( params, socketfd[0], pe_info );
        break;
    }
    close( socketfd[0] );
    SetLastError( RtlNtStatusToDosError( status ));
    return FALSE;
}


/***********************************************************************
 *           create_process
 *
 * Create a new process. If hFile is a valid handle we have an exe
 * file, otherwise it is a Winelib app.
 */
static BOOL create_process( HANDLE hFile, LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                            BOOL inherit, DWORD flags, const RTL_USER_PROCESS_PARAMETERS *params,
                            LPPROCESS_INFORMATION info, LPCSTR unixdir, const pe_image_info_t *pe_info )
{
    NTSTATUS status;
    BOOL success = FALSE;
    HANDLE process_info, process_handle = 0;
    struct object_attributes *objattr;
    data_size_t attr_len;
    WCHAR *env_end;
    char *winedebug = NULL;
    startup_info_t *startup_info;
    DWORD startup_info_size;
    int socketfd[2];
    pid_t pid;
    int err;

    /* create the socket for the new process */

    if (socketpair( PF_UNIX, SOCK_STREAM, 0, socketfd ) == -1)
    {
        SetLastError( ERROR_TOO_MANY_OPEN_FILES );
        return FALSE;
    }
#ifdef SO_PASSCRED
    else
    {
        int enable = 1;
        setsockopt( socketfd[0], SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable) );
    }
#endif

    if (!(startup_info = create_startup_info( params, &startup_info_size )))
    {
        close( socketfd[0] );
        close( socketfd[1] );
        return FALSE;
    }
    env_end = params->Environment;
    while (*env_end)
    {
        static const WCHAR WINEDEBUG[] = {'W','I','N','E','D','E','B','U','G','=',0};
        if (!winedebug && !strncmpW( env_end, WINEDEBUG, ARRAY_SIZE( WINEDEBUG ) - 1 ))
        {
            DWORD len = WideCharToMultiByte( CP_UNIXCP, 0, env_end, -1, NULL, 0, NULL, NULL );
            if ((winedebug = HeapAlloc( GetProcessHeap(), 0, len )))
                WideCharToMultiByte( CP_UNIXCP, 0, env_end, -1, winedebug, len, NULL, NULL );
        }
        env_end += strlenW(env_end) + 1;
    }
    env_end++;

    wine_server_send_fd( socketfd[1] );
    close( socketfd[1] );

    /* create the process on the server side */

    alloc_object_attributes( psa, &objattr, &attr_len );
    SERVER_START_REQ( new_process )
    {
        req->inherit_all    = inherit;
        req->create_flags   = flags;
        req->socket_fd      = socketfd[1];
        req->exe_file       = wine_server_obj_handle( hFile );
        req->access         = PROCESS_ALL_ACCESS;
        req->cpu            = pe_info->cpu;
        req->info_size      = startup_info_size;
        wine_server_add_data( req, objattr, attr_len );
        wine_server_add_data( req, startup_info, startup_info_size );
        wine_server_add_data( req, params->Environment, (env_end - params->Environment) * sizeof(WCHAR) );
        if (!(status = wine_server_call( req )))
        {
            info->dwProcessId = (DWORD)reply->pid;
            process_handle    = wine_server_ptr_handle( reply->handle );
        }
        process_info = wine_server_ptr_handle( reply->info );
    }
    SERVER_END_REQ;
    HeapFree( GetProcessHeap(), 0, objattr );

    if (!status)
    {
        alloc_object_attributes( tsa, &objattr, &attr_len );
        SERVER_START_REQ( new_thread )
        {
            req->process    = wine_server_obj_handle( process_handle );
            req->access     = THREAD_ALL_ACCESS;
            req->suspend    = !!(flags & CREATE_SUSPENDED);
            req->request_fd = -1;
            wine_server_add_data( req, objattr, attr_len );
            if (!(status = wine_server_call( req )))
            {
                info->hProcess = process_handle;
                info->hThread = wine_server_ptr_handle( reply->handle );
                info->dwThreadId = reply->tid;
            }
        }
        SERVER_END_REQ;
        HeapFree( GetProcessHeap(), 0, objattr );
    }

    if (status)
    {
        switch (status)
        {
        case STATUS_INVALID_IMAGE_WIN_64:
            ERR( "64-bit application %s not supported in 32-bit prefix\n",
                 debugstr_w( params->ImagePathName.Buffer ));
            break;
        case STATUS_INVALID_IMAGE_FORMAT:
            ERR( "%s not supported on this installation (%s binary)\n",
                 debugstr_w( params->ImagePathName.Buffer ), cpu_names[pe_info->cpu] );
            break;
        }
        close( socketfd[0] );
        CloseHandle( process_handle );
        HeapFree( GetProcessHeap(), 0, startup_info );
        HeapFree( GetProcessHeap(), 0, winedebug );
        SetLastError( RtlNtStatusToDosError( status ));
        return FALSE;
    }

    HeapFree( GetProcessHeap(), 0, startup_info );

    /* create the child process */

    pid = spawn_loader( params, socketfd[0], unixdir, winedebug, pe_info );

    close( socketfd[0] );
    HeapFree( GetProcessHeap(), 0, winedebug );
    if (pid == -1)
    {
        FILE_SetDosError();
        goto error;
    }

    /* wait for the new process info to be ready */

    WaitForSingleObject( process_info, INFINITE );
    SERVER_START_REQ( get_new_process_info )
    {
        req->info = wine_server_obj_handle( process_info );
        wine_server_call( req );
        success = reply->success;
        err = reply->exit_code;
    }
    SERVER_END_REQ;

    if (!success)
    {
        SetLastError( err ? err : ERROR_INTERNAL_ERROR );
        goto error;
    }
    CloseHandle( process_info );
    return success;

error:
    CloseHandle( process_info );
    CloseHandle( info->hProcess );
    CloseHandle( info->hThread );
    info->hProcess = info->hThread = 0;
    info->dwProcessId = info->dwThreadId = 0;
    return FALSE;
}


/***********************************************************************
 *           get_vdm_params
 *
 * Build the parameters needed to launch a new VDM process.
 */
static RTL_USER_PROCESS_PARAMETERS *get_vdm_params( const RTL_USER_PROCESS_PARAMETERS *params,
                                                    pe_image_info_t *pe_info )
{
    static const WCHAR argsW[] = {'%','s',' ','-','-','a','p','p','-','n','a','m','e',' ','"','%','s','"',' ','%','s',0};

    WCHAR *new_cmd_line;
    RTL_USER_PROCESS_PARAMETERS *new_params;
    UNICODE_STRING imageW, cmdlineW;

    new_cmd_line = HeapAlloc(GetProcessHeap(), 0,
			     (strlenW(params->ImagePathName.Buffer) +
                              strlenW(params->CommandLine.Buffer) +
                              strlenW(winevdm) + 16) * sizeof(WCHAR));
    if (!new_cmd_line)
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return NULL;
    }
    sprintfW( new_cmd_line, argsW, winevdm, params->ImagePathName.Buffer, params->CommandLine.Buffer );
    RtlInitUnicodeString( &imageW, winevdm );
    RtlInitUnicodeString( &cmdlineW, new_cmd_line );
    if (RtlCreateProcessParametersEx( &new_params, &imageW, &params->DllPath,
                                      &params->CurrentDirectory.DosPath, &cmdlineW,
                                      params->Environment, &params->WindowTitle, &params->Desktop,
                                      &params->ShellInfo, &params->RuntimeInfo,
                                      PROCESS_PARAMS_FLAG_NORMALIZED ))
    {
        HeapFree( GetProcessHeap(), 0, new_cmd_line );
        SetLastError( ERROR_OUTOFMEMORY );
        return FALSE;
    }
    new_params->hStdInput       = params->hStdInput;
    new_params->hStdOutput      = params->hStdOutput;
    new_params->hStdError       = params->hStdError;
    new_params->dwX             = params->dwX;
    new_params->dwY             = params->dwY;
    new_params->dwXSize         = params->dwXSize;
    new_params->dwYSize         = params->dwYSize;
    new_params->dwXCountChars   = params->dwXCountChars;
    new_params->dwYCountChars   = params->dwYCountChars;
    new_params->dwFillAttribute = params->dwFillAttribute;
    new_params->dwFlags         = params->dwFlags;
    new_params->wShowWindow     = params->wShowWindow;

    memset( pe_info, 0, sizeof(*pe_info) );
    pe_info->cpu = CPU_x86;
    return new_params;
}


/***********************************************************************
 *           create_vdm_process
 *
 * Create a new VDM process for a 16-bit or DOS application.
 */
static BOOL create_vdm_process( LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                                BOOL inherit, DWORD flags, const RTL_USER_PROCESS_PARAMETERS *params,
                                LPPROCESS_INFORMATION info, LPCSTR unixdir )
{
    BOOL ret;
    pe_image_info_t pe_info;
    RTL_USER_PROCESS_PARAMETERS *new_params;

    if (!(new_params = get_vdm_params( params, &pe_info ))) return FALSE;

    ret = create_process( 0, psa, tsa, inherit, flags, new_params, info, unixdir, &pe_info );
    RtlDestroyProcessParameters( new_params );
    return ret;
}


/***********************************************************************
 *           create_cmd_process
 *
 * Create a new cmd shell process for a .BAT file.
 */
static BOOL create_cmd_process( LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                                BOOL inherit, DWORD flags, LPSTARTUPINFOW startup,
                                const RTL_USER_PROCESS_PARAMETERS *params,
                                LPPROCESS_INFORMATION info )

{
    static const WCHAR comspecW[] = {'C','O','M','S','P','E','C',0};
    static const WCHAR cmdW[] = {'\\','c','m','d','.','e','x','e',0};
    static const WCHAR slashscW[] = {' ','/','s','/','c',' ',0};
    static const WCHAR quotW[] = {'"',0};
    WCHAR comspec[MAX_PATH];
    WCHAR *newcmdline, *cur_dir = NULL;
    BOOL ret;

    if (!GetEnvironmentVariableW( comspecW, comspec, ARRAY_SIZE( comspec )))
    {
        GetSystemDirectoryW( comspec, ARRAY_SIZE( comspec ) - ARRAY_SIZE( cmdW ));
        strcatW( comspec, cmdW );
    }
    if (!(newcmdline = HeapAlloc( GetProcessHeap(), 0,
                                  (strlenW(comspec) + 7 +
                                   strlenW(params->CommandLine.Buffer) + 2) * sizeof(WCHAR))))
        return FALSE;

    strcpyW( newcmdline, comspec );
    strcatW( newcmdline, slashscW );
    strcatW( newcmdline, quotW );
    strcatW( newcmdline, params->CommandLine.Buffer );
    strcatW( newcmdline, quotW );
    if (params->CurrentDirectory.DosPath.Length) cur_dir = params->CurrentDirectory.DosPath.Buffer;
    ret = CreateProcessW( comspec, newcmdline, psa, tsa, inherit,
                          flags | CREATE_UNICODE_ENVIRONMENT, params->Environment, cur_dir,
                          startup, info );
    HeapFree( GetProcessHeap(), 0, newcmdline );
    return ret;
}


/*************************************************************************
 *               get_file_name
 *
 * Helper for CreateProcess: retrieve the file name to load from the
 * app name and command line. Store the file name in buffer, and
 * return a possibly modified command line.
 * Also returns a handle to the opened file if it's a Windows binary.
 */
static LPWSTR get_file_name( LPCWSTR appname, LPWSTR cmdline, LPWSTR buffer,
                             int buflen, HANDLE *handle, BOOL *is_64bit )
{
    static const WCHAR quotesW[] = {'"','%','s','"',0};

    WCHAR *name, *pos, *first_space, *ret = NULL;
    const WCHAR *p;

    /* if we have an app name, everything is easy */

    if (appname)
    {
        /* use the unmodified app name as file name */
        lstrcpynW( buffer, appname, buflen );
        *handle = open_exe_file( buffer, is_64bit );
        if (!(ret = cmdline) || !cmdline[0])
        {
            /* no command-line, create one */
            if ((ret = HeapAlloc( GetProcessHeap(), 0, (strlenW(appname) + 3) * sizeof(WCHAR) )))
                sprintfW( ret, quotesW, appname );
        }
        return ret;
    }

    /* first check for a quoted file name */

    if ((cmdline[0] == '"') && ((p = strchrW( cmdline + 1, '"' ))))
    {
        int len = p - cmdline - 1;
        /* extract the quoted portion as file name */
        if (!(name = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) ))) return NULL;
        memcpy( name, cmdline + 1, len * sizeof(WCHAR) );
        name[len] = 0;

        if (!find_exe_file( name, buffer, buflen, handle )) goto done;
        ret = cmdline;  /* no change necessary */
        goto done;
    }

    /* now try the command-line word by word */

    if (!(name = HeapAlloc( GetProcessHeap(), 0, (strlenW(cmdline) + 1) * sizeof(WCHAR) )))
        return NULL;
    pos = name;
    p = cmdline;
    first_space = NULL;

    for (;;)
    {
        while (*p && *p != ' ' && *p != '\t') *pos++ = *p++;
        *pos = 0;
        if (find_exe_file( name, buffer, buflen, handle ))
        {
            ret = cmdline;
            break;
        }
        if (!first_space) first_space = pos;
        if (!(*pos++ = *p++)) break;
    }

    if (!ret)
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
    }
    else if (first_space)  /* build a new command-line with quotes */
    {
        if (!(ret = HeapAlloc( GetProcessHeap(), 0, (strlenW(cmdline) + 3) * sizeof(WCHAR) )))
            goto done;
        sprintfW( ret, quotesW, name );
        strcatW( ret, p );
    }

 done:
    HeapFree( GetProcessHeap(), 0, name );
    return ret;
}

/**********************************************************************
 *      CreateProcessInternalW    (KERNEL32.@)
 */
BOOL WINAPI CreateProcessInternalW( HANDLE token, LPCWSTR app_name, LPWSTR cmd_line,
                                    LPSECURITY_ATTRIBUTES process_attr, LPSECURITY_ATTRIBUTES thread_attr,
                                    BOOL inherit, DWORD flags, LPVOID env, LPCWSTR cur_dir,
                                    LPSTARTUPINFOW startup_info, LPPROCESS_INFORMATION info,
                                    HANDLE *new_token )
{
    BOOL retv = FALSE;
    HANDLE hFile = 0;
    char *unixdir = NULL;
    WCHAR name[MAX_PATH];
    WCHAR *tidy_cmdline, *p;
    RTL_USER_PROCESS_PARAMETERS *params = NULL;
    pe_image_info_t pe_info;
    enum binary_type type;
    BOOL is_64bit;

    /* Process the AppName and/or CmdLine to get module name and path */

    TRACE("app %s cmdline %s\n", debugstr_w(app_name), debugstr_w(cmd_line) );

    if (token) FIXME("Creating a process with a token is not yet implemented\n");
    if (new_token) FIXME("No support for returning created process token\n");

    if (!(tidy_cmdline = get_file_name( app_name, cmd_line, name, ARRAY_SIZE( name ), &hFile, &is_64bit )))
        return FALSE;
    if (hFile == INVALID_HANDLE_VALUE) goto done;

    /* Warn if unsupported features are used */

    if (flags & (IDLE_PRIORITY_CLASS | HIGH_PRIORITY_CLASS | REALTIME_PRIORITY_CLASS |
                 CREATE_NEW_PROCESS_GROUP | CREATE_SEPARATE_WOW_VDM | CREATE_SHARED_WOW_VDM |
                 CREATE_DEFAULT_ERROR_MODE | CREATE_NO_WINDOW |
                 PROFILE_USER | PROFILE_KERNEL | PROFILE_SERVER))
        WARN("(%s,...): ignoring some flags in %x\n", debugstr_w(name), flags);

    if (cur_dir)
    {
        if (!(unixdir = wine_get_unix_file_name( cur_dir )))
        {
            SetLastError(ERROR_DIRECTORY);
            goto done;
        }
    }
    else
    {
        WCHAR buf[MAX_PATH];
        if (GetCurrentDirectoryW(MAX_PATH, buf)) unixdir = wine_get_unix_file_name( buf );
    }

    info->hThread = info->hProcess = 0;
    info->dwProcessId = info->dwThreadId = 0;

    if (!(params = create_process_params( name, tidy_cmdline, cur_dir, env, flags, startup_info )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto done;
    }

    if (!hFile)
    {
        memset( &pe_info, 0, sizeof(pe_info) );
        type = BINARY_UNIX_LIB;
        /* assume current arch */
#if defined(__i386__) || defined(__x86_64__)
        pe_info.cpu = is_64bit ? CPU_x86_64 : CPU_x86;
#elif defined(__powerpc__)
        pe_info.cpu = CPU_POWERPC;
#elif defined(__arm__)
        pe_info.cpu = CPU_ARM;
#elif defined(__aarch64__)
        pe_info.cpu = CPU_ARM64;
#endif
    }
    else type = get_binary_info( hFile, &pe_info );

    switch (type)
    {
    case BINARY_PE:
        if (pe_info.image_charact & IMAGE_FILE_DLL)
        {
            TRACE( "not starting %s since it is a dll\n", debugstr_w(name) );
            SetLastError( ERROR_BAD_EXE_FORMAT );
            break;
        }
        TRACE( "starting %s as Win%d binary (%s-%s, %s)\n",
               debugstr_w(name), is_64bit_arch(pe_info.cpu) ? 64 : 32,
               wine_dbgstr_longlong(pe_info.base), wine_dbgstr_longlong(pe_info.base + pe_info.map_size),
               cpu_names[pe_info.cpu] );
        retv = create_process( hFile, process_attr, thread_attr,
                               inherit, flags, params, info, unixdir, &pe_info );
        break;
    case BINARY_WIN16:
        TRACE( "starting %s as Win16/DOS binary\n", debugstr_w(name) );
        retv = create_vdm_process( process_attr, thread_attr, inherit, flags, params, info, unixdir );
        break;
    case BINARY_UNIX_LIB:
        TRACE( "starting %s as %d-bit Winelib app\n",
               debugstr_w(name), is_64bit_arch(pe_info.cpu) ? 64 : 32 );
        retv = create_process( hFile, process_attr, thread_attr,
                               inherit, flags, params, info, unixdir, &pe_info );
        break;
    case BINARY_UNKNOWN:
        /* check for .com or .bat extension */
        if ((p = strrchrW( name, '.' )))
        {
            if (!strcmpiW( p, comW ) || !strcmpiW( p, pifW ))
            {
                TRACE( "starting %s as DOS binary\n", debugstr_w(name) );
                retv = create_vdm_process( process_attr, thread_attr,
                                           inherit, flags, params, info, unixdir );
                break;
            }
            if (!strcmpiW( p, batW ) || !strcmpiW( p, cmdW ) )
            {
                TRACE( "starting %s as batch binary\n", debugstr_w(name) );
                retv = create_cmd_process( process_attr, thread_attr,
                                           inherit, flags, startup_info, params, info );
                break;
            }
        }
        /* fall through */
    case BINARY_UNIX_EXE:
        /* unknown file, try as unix executable */
        TRACE( "starting %s as Unix binary\n", debugstr_w(name) );
        retv = (fork_and_exec( params, unixdir ) != -1);
        break;
    }
    if (hFile) CloseHandle( hFile );

 done:
    RtlDestroyProcessParameters( params );
    if (tidy_cmdline != cmd_line) HeapFree( GetProcessHeap(), 0, tidy_cmdline );
    HeapFree( GetProcessHeap(), 0, unixdir );
    if (retv)
        TRACE( "started process pid %04x tid %04x\n", info->dwProcessId, info->dwThreadId );
    return retv;
}


/**********************************************************************
 *       CreateProcessInternalA          (KERNEL32.@)
 */
BOOL WINAPI CreateProcessInternalA( HANDLE token, LPCSTR app_name, LPSTR cmd_line,
                                    LPSECURITY_ATTRIBUTES process_attr, LPSECURITY_ATTRIBUTES thread_attr,
                                    BOOL inherit, DWORD flags, LPVOID env, LPCSTR cur_dir,
                                    LPSTARTUPINFOA startup_info, LPPROCESS_INFORMATION info,
                                    HANDLE *new_token )
{
    BOOL ret = FALSE;
    WCHAR *app_nameW = NULL, *cmd_lineW = NULL, *cur_dirW = NULL;
    UNICODE_STRING desktopW, titleW;
    STARTUPINFOW infoW;

    desktopW.Buffer = NULL;
    titleW.Buffer = NULL;
    if (app_name && !(app_nameW = FILE_name_AtoW( app_name, TRUE ))) goto done;
    if (cmd_line && !(cmd_lineW = FILE_name_AtoW( cmd_line, TRUE ))) goto done;
    if (cur_dir && !(cur_dirW = FILE_name_AtoW( cur_dir, TRUE ))) goto done;

    if (startup_info->lpDesktop) RtlCreateUnicodeStringFromAsciiz( &desktopW, startup_info->lpDesktop );
    if (startup_info->lpTitle) RtlCreateUnicodeStringFromAsciiz( &titleW, startup_info->lpTitle );

    memcpy( &infoW, startup_info, sizeof(infoW) );
    infoW.lpDesktop = desktopW.Buffer;
    infoW.lpTitle = titleW.Buffer;

    if (startup_info->lpReserved)
      FIXME("StartupInfo.lpReserved is used, please report (%s)\n",
            debugstr_a(startup_info->lpReserved));

    ret = CreateProcessInternalW( token, app_nameW, cmd_lineW, process_attr, thread_attr,
                                  inherit, flags, env, cur_dirW, &infoW, info, new_token );
done:
    HeapFree( GetProcessHeap(), 0, app_nameW );
    HeapFree( GetProcessHeap(), 0, cmd_lineW );
    HeapFree( GetProcessHeap(), 0, cur_dirW );
    RtlFreeUnicodeString( &desktopW );
    RtlFreeUnicodeString( &titleW );
    return ret;
}


/**********************************************************************
 *       CreateProcessA          (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateProcessA( LPCSTR app_name, LPSTR cmd_line, LPSECURITY_ATTRIBUTES process_attr,
                                              LPSECURITY_ATTRIBUTES thread_attr, BOOL inherit,
                                              DWORD flags, LPVOID env, LPCSTR cur_dir,
                                              LPSTARTUPINFOA startup_info, LPPROCESS_INFORMATION info )
{
    return CreateProcessInternalA( NULL, app_name, cmd_line, process_attr, thread_attr,
                                   inherit, flags, env, cur_dir, startup_info, info, NULL );
}


/**********************************************************************
 *       CreateProcessW          (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateProcessW( LPCWSTR app_name, LPWSTR cmd_line, LPSECURITY_ATTRIBUTES process_attr,
                                              LPSECURITY_ATTRIBUTES thread_attr, BOOL inherit, DWORD flags,
                                              LPVOID env, LPCWSTR cur_dir, LPSTARTUPINFOW startup_info,
                                              LPPROCESS_INFORMATION info )
{
    return CreateProcessInternalW( NULL, app_name, cmd_line, process_attr, thread_attr,
                                   inherit, flags, env, cur_dir, startup_info, info, NULL );
}


/**********************************************************************
 *       CreateProcessAsUserA          (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateProcessAsUserA( HANDLE token, LPCSTR app_name, LPSTR cmd_line,
                                                    LPSECURITY_ATTRIBUTES process_attr,
                                                    LPSECURITY_ATTRIBUTES thread_attr,
                                                    BOOL inherit, DWORD flags, LPVOID env, LPCSTR cur_dir,
                                                    LPSTARTUPINFOA startup_info,
                                                    LPPROCESS_INFORMATION info )
{
    return CreateProcessInternalA( token, app_name, cmd_line, process_attr, thread_attr,
                                   inherit, flags, env, cur_dir, startup_info, info, NULL );
}


/**********************************************************************
 *       CreateProcessAsUserW          (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateProcessAsUserW( HANDLE token, LPCWSTR app_name, LPWSTR cmd_line,
                                                    LPSECURITY_ATTRIBUTES process_attr,
                                                    LPSECURITY_ATTRIBUTES thread_attr,
                                                    BOOL inherit, DWORD flags, LPVOID env, LPCWSTR cur_dir,
                                                    LPSTARTUPINFOW startup_info,
                                                    LPPROCESS_INFORMATION info )
{
    return CreateProcessInternalW( token, app_name, cmd_line, process_attr, thread_attr,
                                   inherit, flags, env, cur_dir, startup_info, info, NULL );
}


/**********************************************************************
 *       exec_process
 */
static void exec_process( LPCWSTR name )
{
    HANDLE hFile;
    WCHAR *p;
    STARTUPINFOW startup_info = { sizeof(startup_info) };
    RTL_USER_PROCESS_PARAMETERS *params, *new_params;
    pe_image_info_t pe_info;
    BOOL is_64bit;

    hFile = open_exe_file( name, &is_64bit );
    if (!hFile || hFile == INVALID_HANDLE_VALUE) return;

    if (!(params = create_process_params( name, GetCommandLineW(), NULL, NULL, 0, &startup_info )))
        return;

    /* Determine executable type */

    switch (get_binary_info( hFile, &pe_info ))
    {
    case BINARY_PE:
        if (pe_info.image_charact & IMAGE_FILE_DLL) break;
        TRACE( "starting %s as Win%d binary (%s-%s, %s)\n",
               debugstr_w(name), is_64bit_arch(pe_info.cpu) ? 64 : 32,
               wine_dbgstr_longlong(pe_info.base), wine_dbgstr_longlong(pe_info.base + pe_info.map_size),
               cpu_names[pe_info.cpu] );
        replace_process( hFile, params, &pe_info );
        break;
    case BINARY_UNIX_LIB:
        TRACE( "%s is a Unix library, starting as Winelib app\n", debugstr_w(name) );
        replace_process( hFile, params, &pe_info );
        break;
    case BINARY_UNKNOWN:
        /* check for .com or .pif extension */
        if (!(p = strrchrW( name, '.' ))) break;
        if (strcmpiW( p, comW ) && strcmpiW( p, pifW )) break;
        /* fall through */
    case BINARY_WIN16:
        TRACE( "starting %s as Win16/DOS binary\n", debugstr_w(name) );
        if (!(new_params = get_vdm_params( params, &pe_info ))) break;
        replace_process( 0, new_params, &pe_info );
        RtlDestroyProcessParameters( new_params );
        break;
    default:
        break;
    }
    CloseHandle( hFile );
    RtlDestroyProcessParameters( params );
}


/***********************************************************************
 *           wait_input_idle
 *
 * Wrapper to call WaitForInputIdle USER function
 */
typedef DWORD (WINAPI *WaitForInputIdle_ptr)( HANDLE hProcess, DWORD dwTimeOut );

static DWORD wait_input_idle( HANDLE process, DWORD timeout )
{
    HMODULE mod = GetModuleHandleA( "user32.dll" );
    if (mod)
    {
        WaitForInputIdle_ptr ptr = (WaitForInputIdle_ptr)GetProcAddress( mod, "WaitForInputIdle" );
        if (ptr) return ptr( process, timeout );
    }
    return 0;
}


/***********************************************************************
 *           WinExec   (KERNEL32.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH WinExec( LPCSTR lpCmdLine, UINT nCmdShow )
{
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    char *cmdline;
    UINT ret;

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = nCmdShow;

    /* cmdline needs to be writable for CreateProcess */
    if (!(cmdline = HeapAlloc( GetProcessHeap(), 0, strlen(lpCmdLine)+1 ))) return 0;
    strcpy( cmdline, lpCmdLine );

    if (CreateProcessA( NULL, cmdline, NULL, NULL, FALSE,
                        0, NULL, NULL, &startup, &info ))
    {
        /* Give 30 seconds to the app to come up */
        if (wait_input_idle( info.hProcess, 30000 ) == WAIT_FAILED)
            WARN("WaitForInputIdle failed: Error %d\n", GetLastError() );
        ret = 33;
        /* Close off the handles */
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }
    else if ((ret = GetLastError()) >= 32)
    {
        FIXME("Strange error set by CreateProcess: %d\n", ret );
        ret = 11;
    }
    HeapFree( GetProcessHeap(), 0, cmdline );
    return ret;
}


/**********************************************************************
 *	    LoadModule    (KERNEL32.@)
 */
DWORD WINAPI LoadModule( LPCSTR name, LPVOID paramBlock )
{
    LOADPARMS32 *params = paramBlock;
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    DWORD ret;
    LPSTR cmdline, p;
    char filename[MAX_PATH];
    BYTE len;

    if (!name) return ERROR_FILE_NOT_FOUND;

    if (!SearchPathA( NULL, name, ".exe", sizeof(filename), filename, NULL ) &&
        !SearchPathA( NULL, name, NULL, sizeof(filename), filename, NULL ))
        return GetLastError();

    len = (BYTE)params->lpCmdLine[0];
    if (!(cmdline = HeapAlloc( GetProcessHeap(), 0, strlen(filename) + len + 2 )))
        return ERROR_NOT_ENOUGH_MEMORY;

    strcpy( cmdline, filename );
    p = cmdline + strlen(cmdline);
    *p++ = ' ';
    memcpy( p, params->lpCmdLine + 1, len );
    p[len] = 0;

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    if (params->lpCmdShow)
    {
        startup.dwFlags = STARTF_USESHOWWINDOW;
        startup.wShowWindow = ((WORD *)params->lpCmdShow)[1];
    }

    if (CreateProcessA( filename, cmdline, NULL, NULL, FALSE, 0,
                        params->lpEnvAddress, NULL, &startup, &info ))
    {
        /* Give 30 seconds to the app to come up */
        if (wait_input_idle( info.hProcess, 30000 ) == WAIT_FAILED)
            WARN("WaitForInputIdle failed: Error %d\n", GetLastError() );
        ret = 33;
        /* Close off the handles */
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }
    else if ((ret = GetLastError()) >= 32)
    {
        FIXME("Strange error set by CreateProcess: %u\n", ret );
        ret = 11;
    }

    HeapFree( GetProcessHeap(), 0, cmdline );
    return ret;
}


/***********************************************************************
 *           ExitProcess   (KERNEL32.@)
 *
 * Exits the current process.
 *
 * PARAMS
 *  status [I] Status code to exit with.
 *
 * RETURNS
 *  Nothing.
 */
#ifdef __i386__
__ASM_STDCALL_FUNC( ExitProcess, 4, /* Shrinker depend on this particular ExitProcess implementation */
                   "pushl %ebp\n\t"
                   ".byte 0x8B, 0xEC\n\t" /* movl %esp, %ebp */
                   ".byte 0x6A, 0x00\n\t" /* pushl $0 */
                   ".byte 0x68, 0x00, 0x00, 0x00, 0x00\n\t" /* pushl $0 - 4 bytes immediate */
                   "pushl 8(%ebp)\n\t"
                   "call " __ASM_STDCALL("RtlExitUserProcess",4) "\n\t"
                   "leave\n\t"
                   "ret $4" )
#else

void WINAPI ExitProcess( DWORD status )
{
    RtlExitUserProcess( status );
}

#endif

/***********************************************************************
 * GetExitCodeProcess           [KERNEL32.@]
 *
 * Gets termination status of specified process.
 *
 * PARAMS
 *   hProcess   [in]  Handle to the process.
 *   lpExitCode [out] Address to receive termination status.
 *
 * RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 */
BOOL WINAPI GetExitCodeProcess( HANDLE hProcess, LPDWORD lpExitCode )
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION pbi;

    status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi,
                                       sizeof(pbi), NULL);
    if (status == STATUS_SUCCESS)
    {
        if (lpExitCode) *lpExitCode = pbi.ExitStatus;
        return TRUE;
    }
    SetLastError( RtlNtStatusToDosError(status) );
    return FALSE;
}


/***********************************************************************
 *           GetProcessFlags    (KERNEL32.@)
 */
DWORD WINAPI GetProcessFlags( DWORD processid )
{
    IMAGE_NT_HEADERS *nt;
    DWORD flags = 0;

    if (processid && processid != GetCurrentProcessId()) return 0;

    if ((nt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress )))
    {
        if (nt->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
            flags |= PDB32_CONSOLE_PROC;
    }
    if (!AreFileApisANSI()) flags |= PDB32_FILE_APIS_OEM;
    if (IsDebuggerPresent()) flags |= PDB32_DEBUGGED;
    return flags;
}


/*********************************************************************
 *           CloseHandle    (KERNEL32.@)
 *
 * Closes a handle.
 *
 * PARAMS
 *  handle [I] Handle to close.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE, check GetLastError().
 */
BOOL WINAPI CloseHandle( HANDLE handle )
{
    NTSTATUS status;

    /* stdio handles need special treatment */
    if (handle == (HANDLE)STD_INPUT_HANDLE)
        handle = InterlockedExchangePointer( &NtCurrentTeb()->Peb->ProcessParameters->hStdInput, 0 );
    else if (handle == (HANDLE)STD_OUTPUT_HANDLE)
        handle = InterlockedExchangePointer( &NtCurrentTeb()->Peb->ProcessParameters->hStdOutput, 0 );
    else if (handle == (HANDLE)STD_ERROR_HANDLE)
        handle = InterlockedExchangePointer( &NtCurrentTeb()->Peb->ProcessParameters->hStdError, 0 );

    if (is_console_handle(handle))
        return CloseConsoleHandle(handle);

    status = NtClose( handle );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/*********************************************************************
 *           GetHandleInformation   (KERNEL32.@)
 */
BOOL WINAPI GetHandleInformation( HANDLE handle, LPDWORD flags )
{
    OBJECT_DATA_INFORMATION info;
    NTSTATUS status = NtQueryObject( handle, ObjectDataInformation, &info, sizeof(info), NULL );

    if (status) SetLastError( RtlNtStatusToDosError(status) );
    else if (flags)
    {
        *flags = 0;
        if (info.InheritHandle) *flags |= HANDLE_FLAG_INHERIT;
        if (info.ProtectFromClose) *flags |= HANDLE_FLAG_PROTECT_FROM_CLOSE;
    }
    return !status;
}


/*********************************************************************
 *           SetHandleInformation   (KERNEL32.@)
 */
BOOL WINAPI SetHandleInformation( HANDLE handle, DWORD mask, DWORD flags )
{
    OBJECT_DATA_INFORMATION info;
    NTSTATUS status;

    /* if not setting both fields, retrieve current value first */
    if ((mask & (HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE)) !=
        (HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE))
    {
        if ((status = NtQueryObject( handle, ObjectDataInformation, &info, sizeof(info), NULL )))
        {
            SetLastError( RtlNtStatusToDosError(status) );
            return FALSE;
        }
    }
    if (mask & HANDLE_FLAG_INHERIT)
        info.InheritHandle = (flags & HANDLE_FLAG_INHERIT) != 0;
    if (mask & HANDLE_FLAG_PROTECT_FROM_CLOSE)
        info.ProtectFromClose = (flags & HANDLE_FLAG_PROTECT_FROM_CLOSE) != 0;

    status = NtSetInformationObject( handle, ObjectDataInformation, &info, sizeof(info) );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/*********************************************************************
 *           DuplicateHandle   (KERNEL32.@)
 */
BOOL WINAPI DuplicateHandle( HANDLE source_process, HANDLE source,
                             HANDLE dest_process, HANDLE *dest,
                             DWORD access, BOOL inherit, DWORD options )
{
    NTSTATUS status;

    if (is_console_handle(source))
    {
        /* FIXME: this test is not sufficient, we need to test process ids, not handles */
        if (source_process != dest_process ||
            source_process != GetCurrentProcess())
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        *dest = DuplicateConsoleHandle( source, access, inherit, options );
        return (*dest != INVALID_HANDLE_VALUE);
    }
    status = NtDuplicateObject( source_process, source, dest_process, dest,
                                access, inherit ? OBJ_INHERIT : 0, options );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *           ConvertToGlobalHandle  (KERNEL32.@)
 */
HANDLE WINAPI ConvertToGlobalHandle(HANDLE hSrc)
{
    HANDLE ret = INVALID_HANDLE_VALUE;
    DuplicateHandle( GetCurrentProcess(), hSrc, GetCurrentProcess(), &ret, 0, FALSE,
                     DUP_HANDLE_MAKE_GLOBAL | DUP_HANDLE_SAME_ACCESS | DUP_HANDLE_CLOSE_SOURCE );
    return ret;
}


/***********************************************************************
 *           SetHandleContext   (KERNEL32.@)
 */
BOOL WINAPI SetHandleContext(HANDLE hnd,DWORD context)
{
    FIXME("(%p,%d), stub. In case this got called by WSOCK32/WS2_32: "
          "the external WINSOCK DLLs won't work with WINE, don't use them.\n",hnd,context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *           GetHandleContext   (KERNEL32.@)
 */
DWORD WINAPI GetHandleContext(HANDLE hnd)
{
    FIXME("(%p), stub. In case this got called by WSOCK32/WS2_32: "
          "the external WINSOCK DLLs won't work with WINE, don't use them.\n",hnd);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/***********************************************************************
 *           CreateSocketHandle   (KERNEL32.@)
 */
HANDLE WINAPI CreateSocketHandle(void)
{
    FIXME("(), stub. In case this got called by WSOCK32/WS2_32: "
          "the external WINSOCK DLLs won't work with WINE, don't use them.\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return INVALID_HANDLE_VALUE;
}


/***********************************************************************
 *          SetProcessAffinityMask   (KERNEL32.@)
 */
BOOL WINAPI SetProcessAffinityMask( HANDLE hProcess, DWORD_PTR affmask )
{
    NTSTATUS status;

    status = NtSetInformationProcess(hProcess, ProcessAffinityMask,
                                     &affmask, sizeof(DWORD_PTR));
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}


/**********************************************************************
 *          GetProcessAffinityMask    (KERNEL32.@)
 */
BOOL WINAPI GetProcessAffinityMask( HANDLE hProcess, PDWORD_PTR process_mask, PDWORD_PTR system_mask )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (process_mask)
    {
        if ((status = NtQueryInformationProcess( hProcess, ProcessAffinityMask,
                                                 process_mask, sizeof(*process_mask), NULL )))
            SetLastError( RtlNtStatusToDosError(status) );
    }
    if (system_mask && status == STATUS_SUCCESS)
    {
        SYSTEM_BASIC_INFORMATION info;

        if ((status = NtQuerySystemInformation( SystemBasicInformation, &info, sizeof(info), NULL )))
            SetLastError( RtlNtStatusToDosError(status) );
        else
            *system_mask = info.ActiveProcessorsAffinityMask;
    }
    return !status;
}


/***********************************************************************
 *           GetProcessVersion    (KERNEL32.@)
 */
DWORD WINAPI GetProcessVersion( DWORD pid )
{
    HANDLE process;
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION pbi;
    SIZE_T count;
    PEB peb;
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    DWORD ver = 0;

    if (!pid || pid == GetCurrentProcessId())
    {
        IMAGE_NT_HEADERS *pnt;

        if ((pnt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress )))
            return ((pnt->OptionalHeader.MajorSubsystemVersion << 16) |
                    pnt->OptionalHeader.MinorSubsystemVersion);
        return 0;
    }

    process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!process) return 0;

    status = NtQueryInformationProcess(process, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    if (status) goto err;

    status = NtReadVirtualMemory(process, pbi.PebBaseAddress, &peb, sizeof(peb), &count);
    if (status || count != sizeof(peb)) goto err;

    memset(&dos, 0, sizeof(dos));
    status = NtReadVirtualMemory(process, peb.ImageBaseAddress, &dos, sizeof(dos), &count);
    if (status || count != sizeof(dos)) goto err;
    if (dos.e_magic != IMAGE_DOS_SIGNATURE) goto err;

    memset(&nt, 0, sizeof(nt));
    status = NtReadVirtualMemory(process, (char *)peb.ImageBaseAddress + dos.e_lfanew, &nt, sizeof(nt), &count);
    if (status || count != sizeof(nt)) goto err;
    if (nt.Signature != IMAGE_NT_SIGNATURE) goto err;

    ver = MAKELONG(nt.OptionalHeader.MinorSubsystemVersion, nt.OptionalHeader.MajorSubsystemVersion);

err:
    CloseHandle(process);

    if (status != STATUS_SUCCESS)
        SetLastError(RtlNtStatusToDosError(status));

    return ver;
}


/***********************************************************************
 *		SetProcessWorkingSetSize	[KERNEL32.@]
 * Sets the min/max working set sizes for a specified process.
 *
 * PARAMS
 *    process  [I] Handle to the process of interest
 *    minset   [I] Specifies minimum working set size
 *    maxset   [I] Specifies maximum working set size
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI SetProcessWorkingSetSize(HANDLE process, SIZE_T minset, SIZE_T maxset)
{
    return SetProcessWorkingSetSizeEx(process, minset, maxset, 0);
}

/***********************************************************************
 *           K32EmptyWorkingSet (KERNEL32.@)
 */
BOOL WINAPI K32EmptyWorkingSet(HANDLE hProcess)
{
    return SetProcessWorkingSetSize(hProcess, (SIZE_T)-1, (SIZE_T)-1);
}


/***********************************************************************
 *           GetProcessWorkingSetSize    (KERNEL32.@)
 */
BOOL WINAPI GetProcessWorkingSetSize(HANDLE process, SIZE_T *minset, SIZE_T *maxset)
{
    return GetProcessWorkingSetSizeEx(process, minset, maxset, NULL);
}


/***********************************************************************
 *		ReadProcessMemory (KERNEL32.@)
 */
BOOL WINAPI ReadProcessMemory( HANDLE process, LPCVOID addr, LPVOID buffer, SIZE_T size,
                               SIZE_T *bytes_read )
{
    NTSTATUS status = NtReadVirtualMemory( process, addr, buffer, size, bytes_read );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *           WriteProcessMemory    		(KERNEL32.@)
 */
BOOL WINAPI WriteProcessMemory( HANDLE process, LPVOID addr, LPCVOID buffer, SIZE_T size,
                                SIZE_T *bytes_written )
{
    NTSTATUS status = NtWriteVirtualMemory( process, addr, buffer, size, bytes_written );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/******************************************************************
 *		GetProcessIoCounters (KERNEL32.@)
 */
BOOL WINAPI GetProcessIoCounters(HANDLE hProcess, PIO_COUNTERS ioc)
{
    NTSTATUS    status;

    status = NtQueryInformationProcess(hProcess, ProcessIoCounters, 
                                       ioc, sizeof(*ioc), NULL);
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/******************************************************************
 *		QueryFullProcessImageNameA (KERNEL32.@)
 */
BOOL WINAPI QueryFullProcessImageNameA(HANDLE hProcess, DWORD dwFlags, LPSTR lpExeName, PDWORD pdwSize)
{
    BOOL retval;
    DWORD pdwSizeW = *pdwSize;
    LPWSTR lpExeNameW = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *pdwSize * sizeof(WCHAR));

    retval = QueryFullProcessImageNameW(hProcess, dwFlags, lpExeNameW, &pdwSizeW);

    if(retval)
        retval = (0 != WideCharToMultiByte(CP_ACP, 0, lpExeNameW, -1,
                               lpExeName, *pdwSize, NULL, NULL));
    if(retval)
        *pdwSize = strlen(lpExeName);

    HeapFree(GetProcessHeap(), 0, lpExeNameW);
    return retval;
}

/******************************************************************
 *		QueryFullProcessImageNameW (KERNEL32.@)
 */
BOOL WINAPI QueryFullProcessImageNameW(HANDLE hProcess, DWORD dwFlags, LPWSTR lpExeName, PDWORD pdwSize)
{
    BYTE buffer[sizeof(UNICODE_STRING) + MAX_PATH*sizeof(WCHAR)];  /* this buffer should be enough */
    UNICODE_STRING *dynamic_buffer = NULL;
    UNICODE_STRING *result = NULL;
    NTSTATUS status;
    DWORD needed;

    /* FIXME: On Windows, ProcessImageFileName return an NT path. In Wine it
     * is a DOS path and we depend on this. */
    status = NtQueryInformationProcess(hProcess, ProcessImageFileName, buffer,
                                       sizeof(buffer) - sizeof(WCHAR), &needed);
    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        dynamic_buffer = HeapAlloc(GetProcessHeap(), 0, needed + sizeof(WCHAR));
        status = NtQueryInformationProcess(hProcess, ProcessImageFileName, (LPBYTE)dynamic_buffer, needed, &needed);
        result = dynamic_buffer;
    }
    else
        result = (PUNICODE_STRING)buffer;

    if (status) goto cleanup;

    if (dwFlags & PROCESS_NAME_NATIVE)
    {
        WCHAR drive[3];
        WCHAR device[1024];
        DWORD ntlen, devlen;

        if (result->Buffer[1] != ':' || result->Buffer[0] < 'A' || result->Buffer[0] > 'Z')
        {
            /* We cannot convert it to an NT device path so fail */
            status = STATUS_NO_SUCH_DEVICE;
            goto cleanup;
        }

        /* Find this drive's NT device path */
        drive[0] = result->Buffer[0];
        drive[1] = ':';
        drive[2] = 0;
        if (!QueryDosDeviceW(drive, device, ARRAY_SIZE(device)))
        {
            status = STATUS_NO_SUCH_DEVICE;
            goto cleanup;
        }

        devlen = lstrlenW(device);
        ntlen = devlen + (result->Length/sizeof(WCHAR) - 2);
        if (ntlen + 1 > *pdwSize)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            goto cleanup;
        }
        *pdwSize = ntlen;

        memcpy(lpExeName, device, devlen * sizeof(*device));
        memcpy(lpExeName + devlen, result->Buffer + 2, result->Length - 2 * sizeof(WCHAR));
        lpExeName[*pdwSize] = 0;
        TRACE("NT path: %s\n", debugstr_w(lpExeName));
    }
    else
    {
        if (result->Length/sizeof(WCHAR) + 1 > *pdwSize)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            goto cleanup;
        }

        *pdwSize = result->Length/sizeof(WCHAR);
        memcpy( lpExeName, result->Buffer, result->Length );
        lpExeName[*pdwSize] = 0;
    }

cleanup:
    HeapFree(GetProcessHeap(), 0, dynamic_buffer);
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/***********************************************************************
 *           K32GetProcessImageFileNameA (KERNEL32.@)
 */
DWORD WINAPI K32GetProcessImageFileNameA( HANDLE process, LPSTR file, DWORD size )
{
    return QueryFullProcessImageNameA(process, PROCESS_NAME_NATIVE, file, &size) ? size : 0;
}

/***********************************************************************
 *           K32GetProcessImageFileNameW (KERNEL32.@)
 */
DWORD WINAPI K32GetProcessImageFileNameW( HANDLE process, LPWSTR file, DWORD size )
{
    return QueryFullProcessImageNameW(process, PROCESS_NAME_NATIVE, file, &size) ? size : 0;
}

/***********************************************************************
 *           K32EnumProcesses (KERNEL32.@)
 */
BOOL WINAPI K32EnumProcesses(DWORD *lpdwProcessIDs, DWORD cb, DWORD *lpcbUsed)
{
    SYSTEM_PROCESS_INFORMATION *spi;
    ULONG size = 0x4000;
    void *buf = NULL;
    NTSTATUS status;

    do {
        size *= 2;
        HeapFree(GetProcessHeap(), 0, buf);
        buf = HeapAlloc(GetProcessHeap(), 0, size);
        if (!buf)
            return FALSE;

        status = NtQuerySystemInformation(SystemProcessInformation, buf, size, NULL);
    } while(status == STATUS_INFO_LENGTH_MISMATCH);

    if (status != STATUS_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, buf);
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    spi = buf;

    for (*lpcbUsed = 0; cb >= sizeof(DWORD); cb -= sizeof(DWORD))
    {
        *lpdwProcessIDs++ = HandleToUlong(spi->UniqueProcessId);
        *lpcbUsed += sizeof(DWORD);

        if (spi->NextEntryOffset == 0)
            break;

        spi = (SYSTEM_PROCESS_INFORMATION *)(((PCHAR)spi) + spi->NextEntryOffset);
    }

    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
}

/***********************************************************************
 *           K32QueryWorkingSet (KERNEL32.@)
 */
BOOL WINAPI K32QueryWorkingSet( HANDLE process, LPVOID buffer, DWORD size )
{
    NTSTATUS status;

    TRACE( "(%p, %p, %d)\n", process, buffer, size );

    status = NtQueryVirtualMemory( process, NULL, MemoryWorkingSetList, buffer, size, NULL );

    if (status)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           K32QueryWorkingSetEx (KERNEL32.@)
 */
BOOL WINAPI K32QueryWorkingSetEx( HANDLE process, LPVOID buffer, DWORD size )
{
    NTSTATUS status;

    TRACE( "(%p, %p, %d)\n", process, buffer, size );

    status = NtQueryVirtualMemory( process, NULL, MemoryWorkingSetList, buffer,  size, NULL );

    if (status)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           K32GetProcessMemoryInfo (KERNEL32.@)
 *
 * Retrieve memory usage information for a given process
 *
 */
BOOL WINAPI K32GetProcessMemoryInfo(HANDLE process,
                                    PPROCESS_MEMORY_COUNTERS pmc, DWORD cb)
{
    NTSTATUS status;
    VM_COUNTERS vmc;

    if (cb < sizeof(PROCESS_MEMORY_COUNTERS))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    status = NtQueryInformationProcess(process, ProcessVmCounters,
                                       &vmc, sizeof(vmc), NULL);

    if (status)
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    pmc->cb = sizeof(PROCESS_MEMORY_COUNTERS);
    pmc->PageFaultCount = vmc.PageFaultCount;
    pmc->PeakWorkingSetSize = vmc.PeakWorkingSetSize;
    pmc->WorkingSetSize = vmc.WorkingSetSize;
    pmc->QuotaPeakPagedPoolUsage = vmc.QuotaPeakPagedPoolUsage;
    pmc->QuotaPagedPoolUsage = vmc.QuotaPagedPoolUsage;
    pmc->QuotaPeakNonPagedPoolUsage = vmc.QuotaPeakNonPagedPoolUsage;
    pmc->QuotaNonPagedPoolUsage = vmc.QuotaNonPagedPoolUsage;
    pmc->PagefileUsage = vmc.PagefileUsage;
    pmc->PeakPagefileUsage = vmc.PeakPagefileUsage;

    return TRUE;
}

/***********************************************************************
 * ProcessIdToSessionId   (KERNEL32.@)
 * This function is available on Terminal Server 4SP4 and Windows 2000
 */
BOOL WINAPI ProcessIdToSessionId( DWORD procid, DWORD *sessionid_ptr )
{
    if (procid != GetCurrentProcessId())
        FIXME("Unsupported for other processes.\n");

    *sessionid_ptr = NtCurrentTeb()->Peb->SessionId;
    return TRUE;
}


/***********************************************************************
 *		RegisterServiceProcess (KERNEL32.@)
 *
 * A service process calls this function to ensure that it continues to run
 * even after a user logged off.
 */
DWORD WINAPI RegisterServiceProcess(DWORD dwProcessId, DWORD dwType)
{
    /* I don't think that Wine needs to do anything in this function */
    return 1; /* success */
}


/***********************************************************************
 *           GetCurrentProcess   (KERNEL32.@)
 *
 * Get a handle to the current process.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  A handle representing the current process.
 */
HANDLE WINAPI KERNEL32_GetCurrentProcess(void)
{
    return (HANDLE)~(ULONG_PTR)0;
}

/***********************************************************************
 *           GetLogicalProcessorInformation     (KERNEL32.@)
 */
BOOL WINAPI GetLogicalProcessorInformation(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer, PDWORD pBufLen)
{
    NTSTATUS status;

    TRACE("(%p,%p)\n", buffer, pBufLen);

    if(!pBufLen)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    status = NtQuerySystemInformation( SystemLogicalProcessorInformation, buffer, *pBufLen, pBufLen);

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           GetLogicalProcessorInformationEx   (KERNEL32.@)
 */
BOOL WINAPI GetLogicalProcessorInformationEx(LOGICAL_PROCESSOR_RELATIONSHIP relationship, SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buffer, DWORD *len)
{
    NTSTATUS status;

    TRACE("(%u,%p,%p)\n", relationship, buffer, len);

    if (!len)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    status = NtQuerySystemInformationEx( SystemLogicalProcessorInformationEx, &relationship, sizeof(relationship),
        buffer, *len, len );
    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           CmdBatNotification   (KERNEL32.@)
 *
 * Notifies the system that a batch file has started or finished.
 *
 * PARAMS
 *  bBatchRunning [I]  TRUE if a batch file has started or 
 *                     FALSE if a batch file has finished executing.
 *
 * RETURNS
 *  Unknown.
 */
BOOL WINAPI CmdBatNotification( BOOL bBatchRunning )
{
    FIXME("%d\n", bBatchRunning);
    return FALSE;
}

/***********************************************************************
 *           RegisterApplicationRestart       (KERNEL32.@)
 */
HRESULT WINAPI RegisterApplicationRestart(PCWSTR pwzCommandLine, DWORD dwFlags)
{
    FIXME("(%s,%d)\n", debugstr_w(pwzCommandLine), dwFlags);

    return S_OK;
}

/**********************************************************************
 *           WTSGetActiveConsoleSessionId     (KERNEL32.@)
 */
DWORD WINAPI WTSGetActiveConsoleSessionId(void)
{
    static int once;
    if (!once++) FIXME("stub\n");
    /* Return current session id. */
    return NtCurrentTeb()->Peb->SessionId;
}

/**********************************************************************
 *           GetSystemDEPPolicy     (KERNEL32.@)
 */
DEP_SYSTEM_POLICY_TYPE WINAPI GetSystemDEPPolicy(void)
{
    FIXME("stub\n");
    return OptIn;
}

/**********************************************************************
 *           SetProcessDEPPolicy     (KERNEL32.@)
 */
BOOL WINAPI SetProcessDEPPolicy(DWORD newDEP)
{
    FIXME("(%d): stub\n", newDEP);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *           ApplicationRecoveryFinished     (KERNEL32.@)
 */
VOID WINAPI ApplicationRecoveryFinished(BOOL success)
{
    FIXME(": stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/**********************************************************************
 *           ApplicationRecoveryInProgress     (KERNEL32.@)
 */
HRESULT WINAPI ApplicationRecoveryInProgress(PBOOL canceled)
{
    FIXME(":%p stub\n", canceled);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return E_FAIL;
}

/**********************************************************************
 *           RegisterApplicationRecoveryCallback     (KERNEL32.@)
 */
HRESULT WINAPI RegisterApplicationRecoveryCallback(APPLICATION_RECOVERY_CALLBACK callback, PVOID param, DWORD pingint, DWORD flags)
{
    FIXME("%p, %p, %d, %d: stub\n", callback, param, pingint, flags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return E_FAIL;
}

/***********************************************************************
 *           GetApplicationRestartSettings       (KERNEL32.@)
 */
HRESULT WINAPI GetApplicationRestartSettings(HANDLE process, WCHAR *cmdline, DWORD *size, DWORD *flags)
{
    FIXME("%p, %p, %p, %p)\n", process, cmdline, size, flags);
    return E_NOTIMPL;
}

/**********************************************************************
 *           GetNumaHighestNodeNumber     (KERNEL32.@)
 */
BOOL WINAPI GetNumaHighestNodeNumber(PULONG highestnode)
{
    *highestnode = 0;
    FIXME("(%p): semi-stub\n", highestnode);
    return TRUE;
}

/**********************************************************************
 *           GetNumaNodeProcessorMask     (KERNEL32.@)
 */
BOOL WINAPI GetNumaNodeProcessorMask(UCHAR node, PULONGLONG mask)
{
    FIXME("(%c %p): stub\n", node, mask);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *           GetNumaNodeProcessorMaskEx     (KERNEL32.@)
 */
BOOL WINAPI GetNumaNodeProcessorMaskEx(USHORT node, PGROUP_AFFINITY mask)
{
    FIXME("(%hu %p): stub\n", node, mask);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *           GetNumaAvailableMemoryNode     (KERNEL32.@)
 */
BOOL WINAPI GetNumaAvailableMemoryNode(UCHAR node, PULONGLONG available_bytes)
{
    FIXME("(%c %p): stub\n", node, available_bytes);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *           GetNumaAvailableMemoryNodeEx     (KERNEL32.@)
 */
BOOL WINAPI GetNumaAvailableMemoryNodeEx(USHORT node, PULONGLONG available_bytes)
{
    FIXME("(%hu %p): stub\n", node, available_bytes);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           GetNumaProcessorNode (KERNEL32.@)
 */
BOOL WINAPI GetNumaProcessorNode(UCHAR processor, PUCHAR node)
{
    SYSTEM_INFO si;

    TRACE("(%d, %p)\n", processor, node);

    GetSystemInfo( &si );
    if (processor < si.dwNumberOfProcessors)
    {
        *node = 0;
        return TRUE;
    }

    *node = 0xFF;
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

/***********************************************************************
 *           GetNumaProcessorNodeEx (KERNEL32.@)
 */
BOOL WINAPI GetNumaProcessorNodeEx(PPROCESSOR_NUMBER processor, PUSHORT node_number)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           GetNumaProximityNode (KERNEL32.@)
 */
BOOL WINAPI GetNumaProximityNode(ULONG  proximity_id, PUCHAR node_number)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           GetNumaProximityNodeEx (KERNEL32.@)
 */
BOOL WINAPI GetNumaProximityNodeEx(ULONG  proximity_id, PUSHORT node_number)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *           GetProcessDEPPolicy     (KERNEL32.@)
 */
BOOL WINAPI GetProcessDEPPolicy(HANDLE process, LPDWORD flags, PBOOL permanent)
{
    NTSTATUS status;
    ULONG dep_flags;

    TRACE("(%p %p %p)\n", process, flags, permanent);

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessExecuteFlags,
                                        &dep_flags, sizeof(dep_flags), NULL );
    if (!status)
    {

        if (flags)
        {
            *flags = 0;
            if (dep_flags & MEM_EXECUTE_OPTION_DISABLE)
                *flags |= PROCESS_DEP_ENABLE;
            if (dep_flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION)
                *flags |= PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION;
        }

        if (permanent)
            *permanent = (dep_flags & MEM_EXECUTE_OPTION_PERMANENT) != 0;

    }
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/**********************************************************************
 *           FlushProcessWriteBuffers     (KERNEL32.@)
 */
VOID WINAPI FlushProcessWriteBuffers(void)
{
    static int once = 0;

    if (!once++)
        FIXME(": stub\n");
}

/***********************************************************************
 *           UnregisterApplicationRestart       (KERNEL32.@)
 */
HRESULT WINAPI UnregisterApplicationRestart(void)
{
    FIXME(": stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return S_OK;
}

/***********************************************************************
 *           CreateUmsCompletionList   (KERNEL32.@)
 */
BOOL WINAPI CreateUmsCompletionList(PUMS_COMPLETION_LIST *list)
{
    FIXME( "%p: stub\n", list );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           CreateUmsThreadContext   (KERNEL32.@)
 */
BOOL WINAPI CreateUmsThreadContext(PUMS_CONTEXT *ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           DeleteUmsCompletionList   (KERNEL32.@)
 */
BOOL WINAPI DeleteUmsCompletionList(PUMS_COMPLETION_LIST list)
{
    FIXME( "%p: stub\n", list );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           DeleteUmsThreadContext   (KERNEL32.@)
 */
BOOL WINAPI DeleteUmsThreadContext(PUMS_CONTEXT ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           DequeueUmsCompletionListItems   (KERNEL32.@)
 */
BOOL WINAPI DequeueUmsCompletionListItems(void *list, DWORD timeout, PUMS_CONTEXT *ctx)
{
    FIXME( "%p,%08x,%p: stub\n", list, timeout, ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           EnterUmsSchedulingMode   (KERNEL32.@)
 */
BOOL WINAPI EnterUmsSchedulingMode(UMS_SCHEDULER_STARTUP_INFO *info)
{
    FIXME( "%p: stub\n", info );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           ExecuteUmsThread   (KERNEL32.@)
 */
BOOL WINAPI ExecuteUmsThread(PUMS_CONTEXT ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           GetCurrentUmsThread   (KERNEL32.@)
 */
PUMS_CONTEXT WINAPI GetCurrentUmsThread(void)
{
    FIXME( "stub\n" );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           GetNextUmsListItem   (KERNEL32.@)
 */
PUMS_CONTEXT WINAPI GetNextUmsListItem(PUMS_CONTEXT ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return NULL;
}

/***********************************************************************
 *           GetUmsCompletionListEvent   (KERNEL32.@)
 */
BOOL WINAPI GetUmsCompletionListEvent(PUMS_COMPLETION_LIST list, HANDLE *event)
{
    FIXME( "%p,%p: stub\n", list, event );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           QueryUmsThreadInformation   (KERNEL32.@)
 */
BOOL WINAPI QueryUmsThreadInformation(PUMS_CONTEXT ctx, UMS_THREAD_INFO_CLASS class,
                                       void *buf, ULONG length, ULONG *ret_length)
{
    FIXME( "%p,%08x,%p,%08x,%p: stub\n", ctx, class, buf, length, ret_length );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           SetUmsThreadInformation   (KERNEL32.@)
 */
BOOL WINAPI SetUmsThreadInformation(PUMS_CONTEXT ctx, UMS_THREAD_INFO_CLASS class,
                                     void *buf, ULONG length)
{
    FIXME( "%p,%08x,%p,%08x: stub\n", ctx, class, buf, length );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           UmsThreadYield   (KERNEL32.@)
 */
BOOL WINAPI UmsThreadYield(void *param)
{
    FIXME( "%p: stub\n", param );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/**********************************************************************
 *           BaseFlushAppcompatCache     (KERNEL32.@)
 */
BOOL WINAPI BaseFlushAppcompatCache(void)
{
    FIXME(": stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
