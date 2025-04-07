/*
 * Unix interface for loader functions
 *
 * Copyright (C) 2020 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <spawn.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dlfcn.h>
#ifdef HAVE_PWD_H
# include <pwd.h>
#endif
#ifdef HAVE_ELF_H
# include <elf.h>
#endif
#ifdef HAVE_LINK_H
# include <link.h>
#endif
#ifdef HAVE_SYS_AUXV_H
# include <sys/auxv.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif
#include <limits.h>
#ifdef HAVE_SYS_SYSCTL_H
# include <sys/sysctl.h>
#endif
#ifdef __APPLE__
# include <CoreFoundation/CoreFoundation.h>
# define LoadResource MacLoadResource
# define GetCurrentThread MacGetCurrentThread
# include <CoreServices/CoreServices.h>
# undef LoadResource
# undef GetCurrentThread
# include <pthread.h>
# include <mach/mach.h>
# include <mach/mach_error.h>
# include <mach-o/getsect.h>
# include <crt_externs.h>
# ifndef _POSIX_SPAWN_DISABLE_ASLR
#  define _POSIX_SPAWN_DISABLE_ASLR 0x0100
# endif
# define environ (*_NSGetEnviron())
#else
  extern char **environ;
#endif
#ifdef __ANDROID__
# include <jni.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "winbase.h"
#include "winnls.h"
#include "winioctl.h"
#include "winternl.h"
#include "unix_private.h"
#include "wine/list.h"
#include "ntsyscalls.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);

void *pDbgUiRemoteBreakin = NULL;
void *pKiRaiseUserExceptionDispatcher = NULL;
void *pKiUserExceptionDispatcher = NULL;
void *pKiUserApcDispatcher = NULL;
void *pKiUserCallbackDispatcher = NULL;
void *pKiUserEmulationDispatcher = NULL;
void *pLdrInitializeThunk = NULL;
void *pRtlUserThreadStart = NULL;
void *p__wine_ctrl_routine = NULL;
SYSTEM_DLL_INIT_BLOCK *pLdrSystemDllInitBlock = NULL;

static void * const syscalls[] =
{
#define SYSCALL_ENTRY(id,name,args) name,
#ifdef _WIN64
    ALL_SYSCALLS64
#else
    ALL_SYSCALLS32
#endif
#undef SYSCALL_ENTRY
};

static BYTE syscall_args[ARRAY_SIZE(syscalls)] =
{
#define SYSCALL_ENTRY(id,name,args) args,
#ifdef _WIN64
    ALL_SYSCALLS64
#else
    ALL_SYSCALLS32
#endif
#undef SYSCALL_ENTRY
};

SYSTEM_SERVICE_TABLE KeServiceDescriptorTable[4] =
{
    { (ULONG_PTR *)syscalls, NULL, ARRAY_SIZE(syscalls), syscall_args }
};

#ifdef __GNUC__
static void fatal_error( const char *err, ... ) __attribute__((noreturn, format(printf,1,2)));
#endif

static const char *bin_dir;
static const char *dll_dir;
static const char *ntdll_dir;
static const char *alt_build_dir;
static SIZE_T dll_path_maxlen;

const char *home_dir = NULL;
const char *data_dir = NULL;
const char *build_dir = NULL;
const char *config_dir = NULL;
const char *wineloader = NULL;
const char **dll_paths = NULL;
const char **system_dll_paths = NULL;
const char *user_name = NULL;
SECTION_IMAGE_INFORMATION main_image_info = { NULL };

/* adjust an array of pointers to make them into RVAs */
static inline void fixup_rva_ptrs( void *array, BYTE *base, unsigned int count )
{
    BYTE **src = array;
    DWORD *dst = array;

    for ( ; count; count--, src++, dst++) *dst = *src ? *src - base : 0;
}

/* fixup an array of RVAs by adding the specified delta */
static inline void fixup_rva_dwords( DWORD *ptr, int delta, unsigned int count )
{
    for ( ; count; count--, ptr++) if (*ptr) *ptr += delta;
}


/* fixup an array of name/ordinal RVAs by adding the specified delta */
static inline void fixup_rva_names( UINT_PTR *ptr, int delta )
{
    for ( ; *ptr; ptr++) if (!(*ptr & IMAGE_ORDINAL_FLAG)) *ptr += delta;
}


/* fixup RVAs in the resource directory */
static void fixup_so_resources( IMAGE_RESOURCE_DIRECTORY *dir, BYTE *root, int delta )
{
    IMAGE_RESOURCE_DIRECTORY_ENTRY *entry = (IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    unsigned int i;

    for (i = 0; i < dir->NumberOfNamedEntries + dir->NumberOfIdEntries; i++, entry++)
    {
        void *ptr = root + entry->OffsetToDirectory;
        if (entry->DataIsDirectory) fixup_so_resources( ptr, root, delta );
        else fixup_rva_dwords( &((IMAGE_RESOURCE_DATA_ENTRY *)ptr)->OffsetToData, delta, 1 );
    }
}

/* die on a fatal error; use only during initialization */
static void fatal_error( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "wine: " );
    vfprintf( stderr, err, args );
    va_end( args );
    exit(1);
}

static void set_max_limit( int limit )
{
    struct rlimit rlimit;

    if (!getrlimit( limit, &rlimit ))
    {
        rlimit.rlim_cur = rlimit.rlim_max;

        if (!setrlimit( limit, &rlimit ))
            return;

#if defined(__APPLE__) && defined(RLIMIT_NOFILE) && defined(OPEN_MAX)
        if (limit == RLIMIT_NOFILE)
        {
            unsigned int nlimit = 0;
            size_t size;

            /* On Leopard, setrlimit(RLIMIT_NOFILE, ...) fails on attempts to set
             * rlim_cur above OPEN_MAX (even if rlim_max > OPEN_MAX).
             *
             * In later versions it can be set to kern.maxfilesperproc (from
             * sysctl). In Big Sur and later it can be set to rlim_max. */
            size = sizeof(nlimit);
            if (sysctlbyname("kern.maxfilesperproc", &nlimit, &size, NULL, 0) != 0 || nlimit < OPEN_MAX)
                rlimit.rlim_cur = OPEN_MAX;
            else
                rlimit.rlim_cur = nlimit;

            if (!setrlimit( limit, &rlimit ))
            {
                TRACE("Fallback 1: RLIMIT_NOFILE to kern.maxfilesperproc\n");
                return;
            }

            rlimit.rlim_cur = OPEN_MAX;
            if (!setrlimit( limit, &rlimit ))
            {
                TRACE("Fallback 2: RLIMIT_NOFILE to OPEN_MAX(%d)\n", OPEN_MAX);
                return;
            }
        }
#endif
        WARN("Failed to raise limit %d\n", limit);
    }
}

/* canonicalize path and return its directory name */
static char *realpath_dirname( const char *name )
{
    char *p, *fullpath = realpath( name, NULL );

    if (fullpath)
    {
        p = strrchr( fullpath, '/' );
        if (p == fullpath) p++;
        if (p) *p = 0;
    }
    return fullpath;
}

/* if string ends with tail, remove it */
static char *remove_tail( const char *str, const char *tail )
{
    size_t len = strlen( str );
    size_t tail_len = strlen( tail );
    char *ret;

    if (len < tail_len) return NULL;
    if (strcmp( str + len - tail_len, tail )) return NULL;
    ret = malloc( len - tail_len + 1 );
    memcpy( ret, str, len - tail_len );
    ret[len - tail_len] = 0;
    return ret;
}

/* build a path from the specified dir and name */
static char *build_path( const char *dir, const char *name )
{
    size_t len = strlen( dir );
    char *ret = malloc( len + strlen( name ) + 2 );

    if (len)
    {
        memcpy( ret, dir, len );
        if (ret[len - 1] != '/') ret[len++] = '/';
        if (name[0] == '/') name++;
    }
    strcpy( ret + len, name );
    return ret;
}

/* build a path with the relative dir from 'from' to 'dest' appended to base */
static char *build_relative_path( const char *base, const char *from, const char *dest )
{
    const char *start;
    char *ret;
    unsigned int dotdots = 0;

    for (;;)
    {
        while (*from == '/') from++;
        while (*dest == '/') dest++;
        start = dest;  /* save start of next path element */
        if (!*from) break;

        while (*from && *from != '/' && *from == *dest) { from++; dest++; }
        if ((!*from || *from == '/') && (!*dest || *dest == '/')) continue;

        do  /* count remaining elements in 'from' */
        {
            dotdots++;
            while (*from && *from != '/') from++;
            while (*from == '/') from++;
        }
        while (*from);
        break;
    }

    ret = malloc( strlen(base) + 3 * dotdots + strlen(start) + 2 );
    strcpy( ret, base );
    while (dotdots--) strcat( ret, "/.." );

    if (!start[0]) return ret;
    strcat( ret, "/" );
    strcat( ret, start );
    return ret;
}

/* build a path to a binary and exec it */
static int build_path_and_exec( pid_t *pid, const char *dir, const char *name, char **argv )
{
    int ret;

    argv[0] = build_path( dir, name );
    ret = posix_spawn( pid, argv[0], NULL, NULL, argv, environ );
    free( argv[0] );
    return ret;
}


static const char *get_so_dir( WORD machine )
{
    switch (machine)
    {
    case IMAGE_FILE_MACHINE_I386:  return "/i386-unix";
    case IMAGE_FILE_MACHINE_AMD64: return "/x86_64-unix";
    case IMAGE_FILE_MACHINE_ARMNT: return "/arm-unix";
    case IMAGE_FILE_MACHINE_ARM64: return "/aarch64-unix";
    default: return "";
    }
}

static const char *get_pe_dir( WORD machine )
{
    switch(machine)
    {
    case IMAGE_FILE_MACHINE_I386:  return "/i386-windows";
    case IMAGE_FILE_MACHINE_AMD64: return "/x86_64-windows";
    case IMAGE_FILE_MACHINE_ARMNT: return "/arm-windows";
    case IMAGE_FILE_MACHINE_ARM64: return "/aarch64-windows";
    default: return "";
    }
}

static WORD get_alt_machine( WORD machine )
{
    switch (machine)
    {
    case IMAGE_FILE_MACHINE_I386:  return IMAGE_FILE_MACHINE_AMD64;
    case IMAGE_FILE_MACHINE_AMD64: return IMAGE_FILE_MACHINE_I386;
    case IMAGE_FILE_MACHINE_ARMNT: return IMAGE_FILE_MACHINE_ARM64;
    case IMAGE_FILE_MACHINE_ARM64: return IMAGE_FILE_MACHINE_ARMNT;
    default: return machine;
    }
}

static void set_dll_path(void)
{
    char *p, *path = getenv( "WINEDLLPATH" );
    int i, count = 0;

    if (path) for (p = path, count = 1; *p; p++) if (*p == ':') count++;

    dll_paths = malloc( (count + 2) * sizeof(*dll_paths) );
    count = 0;

    if (!build_dir) dll_paths[count++] = dll_dir;

    if (path)
    {
        path = strdup(path);
        for (p = strtok( path, ":" ); p; p = strtok( NULL, ":" )) dll_paths[count++] = strdup( p );
        free( path );
    }

    for (i = 0; i < count; i++) dll_path_maxlen = max( dll_path_maxlen, strlen(dll_paths[i]) );
    dll_paths[count] = NULL;
}


static void set_system_dll_path(void)
{
    const char *p, *path = SYSTEMDLLPATH;
    int count = 0;

    if (path && *path) for (p = path, count = 1; *p; p++) if (*p == ':') count++;

    system_dll_paths = malloc( (count + 1) * sizeof(*system_dll_paths) );
    count = 0;

    if (path && *path)
    {
        char *path_copy = strdup(path);
        for (p = strtok( path_copy, ":" ); p; p = strtok( NULL, ":" ))
            system_dll_paths[count++] = strdup( p );
        free( path_copy );
    }
    system_dll_paths[count] = NULL;
}


static void set_home_dir(void)
{
    const char *home = getenv( "HOME" );
    const char *name = getenv( "USER" );
    const char *p;

    if (!home || !name)
    {
        struct passwd *pwd = getpwuid( getuid() );
        if (pwd)
        {
            if (!home) home = pwd->pw_dir;
            if (!name) name = pwd->pw_name;
        }
        if (!name) name = "wine";
    }
    if ((p = strrchr( name, '/' ))) name = p + 1;
    if ((p = strrchr( name, '\\' ))) name = p + 1;
    home_dir = strdup( home );
    user_name = strdup( name );
}


static void set_config_dir(void)
{
    char *p, *dir;
    const char *prefix = getenv( "WINEPREFIX" );

    if (prefix)
    {
        if (prefix[0] != '/')
            fatal_error( "invalid directory %s in WINEPREFIX: not an absolute path\n", prefix );
        config_dir = dir = strdup( prefix );
        for (p = dir + strlen(dir) - 1; p > dir && *p == '/'; p--) *p = 0;
    }
    else
    {
        if (!home_dir) fatal_error( "could not determine your home directory\n" );
        if (home_dir[0] != '/') fatal_error( "the home directory %s is not an absolute path\n", home_dir );
        config_dir = build_path( home_dir, ".wine" );
    }
}

static void init_paths(void)
{
    Dl_info info;

    if (!dladdr( init_paths, &info ) || !(ntdll_dir = realpath_dirname( info.dli_fname )))
        fatal_error( "cannot get path to ntdll.so\n" );

    if ((build_dir = remove_tail( ntdll_dir, "/dlls/ntdll" )))
    {
        wineloader = build_path( build_dir, "loader/wine" );
        alt_build_dir = realpath_dirname( build_path( build_dir, "loader-wow64" ));
    }
    else
    {
        if (!(dll_dir = remove_tail( ntdll_dir, get_so_dir(current_machine) ))) dll_dir = ntdll_dir;
        bin_dir = build_relative_path( dll_dir, LIBDIR "/wine", BINDIR );
        data_dir = build_relative_path( dll_dir, LIBDIR "/wine", DATADIR "/wine" );
        wineloader = build_path( ntdll_dir, "wine" );
    }

    set_dll_path();
    set_system_dll_path();
    set_home_dir();
    set_config_dir();
}


/***********************************************************************
 *           get_alternate_wineloader
 */
char *get_alternate_wineloader( WORD machine )
{
    const char *arch;
    BOOL force_wow64 = (arch = getenv( "WINEARCH" )) && !strcmp( arch, "wow64" );
    char *ret = NULL;

    if (is_win64)
    {
        if (force_wow64) return NULL;
        if (machine != get_alt_machine( current_machine )) return NULL;
    }
    else
    {
        if (!force_wow64 && machine == current_machine) return NULL;
        machine = get_alt_machine( current_machine );
    }

    if (!build_dir)
        asprintf( &ret, "%s%s/wine", dll_dir, get_so_dir( machine ));
    else if (alt_build_dir)
        asprintf( &ret, "%s/loader/wine", alt_build_dir );

    return ret;
}


static void preloader_exec( char **argv )
{
#ifdef HAVE_WINE_PRELOADER
    asprintf( &argv[0], "%s-preloader", argv[1] );
#ifdef __APPLE__
    {
        posix_spawnattr_t attr;
        posix_spawnattr_init( &attr );
        posix_spawnattr_setflags( &attr, POSIX_SPAWN_SETEXEC | _POSIX_SPAWN_DISABLE_ASLR );
        posix_spawn( NULL, argv[0], NULL, &attr, argv, *_NSGetEnviron() );
        posix_spawnattr_destroy( &attr );
    }
#endif
    execv( argv[0], argv );
    free( argv[0] );
#endif
    execv( argv[1], argv + 1 );
}

/* exec the appropriate wine loader for the specified machine */
static NTSTATUS loader_exec( char **argv, WORD machine )
{
    if (((argv[1] = get_alternate_wineloader( machine )))) preloader_exec( argv );

    argv[1] = strdup( wineloader );
    preloader_exec( argv );
    return STATUS_INVALID_IMAGE_FORMAT;
}


/***********************************************************************
 *           exec_wineloader
 *
 * argv[0] and argv[1] must be reserved for the preloader and loader respectively.
 */
NTSTATUS exec_wineloader( char **argv, int socketfd, const struct pe_image_info *pe_info )
{
    WORD machine = pe_info->machine;
    ULONGLONG res_start = pe_info->base;
    ULONGLONG res_end = pe_info->base + pe_info->map_size;
    char preloader_reserve[64], socket_env[64];

    if (pe_info->wine_fakedll) res_start = res_end = 0;
    if (pe_info->image_flags & IMAGE_FLAGS_ComPlusNativeReady) machine = native_machine;

    signal( SIGPIPE, SIG_DFL );

    snprintf( socket_env, sizeof(socket_env), "WINESERVERSOCKET=%u", socketfd );
    snprintf( preloader_reserve, sizeof(preloader_reserve), "WINEPRELOADRESERVE=%x%08x-%x%08x",
             (UINT)(res_start >> 32), (UINT)res_start, (UINT)(res_end >> 32), (UINT)res_end );

    putenv( preloader_reserve );
    putenv( socket_env );

    return loader_exec( argv, machine );
}


/***********************************************************************
 *           exec_wineserver
 *
 * Exec a new wine server.
 */
static int exec_wineserver( pid_t *pid, char **argv )
{
    char *path;

    if (!is_win64 && alt_build_dir)  /* look for 64-bit server */
        return build_path_and_exec( pid, alt_build_dir, "server/wineserver", argv );

    if (build_dir)
        return build_path_and_exec( pid, build_dir, "server/wineserver", argv );

    if (!build_path_and_exec( pid, bin_dir, "wineserver", argv )) return 0;
    if ((path = getenv( "WINESERVER" )) && !build_path_and_exec( pid, "", path, argv )) return 0;

    if ((path = getenv( "PATH" )))
    {
        for (path = strtok( strdup( path ), ":" ); path; path = strtok( NULL, ":" ))
            if (!build_path_and_exec( pid, path, "wineserver", argv )) return 0;
    }
    return build_path_and_exec( pid, BINDIR, "wineserver", argv );
}


/***********************************************************************
 *           start_server
 *
 * Start a new wine server.
 */
void start_server( BOOL debug )
{
    static BOOL started;  /* we only try once */
    char *argv[3];
    static char debug_flag[] = "-d";

    if (!started)
    {
        int status;
        pid_t pid;

        argv[1] = debug ? debug_flag : NULL;
        argv[2] = NULL;
        if (exec_wineserver( &pid, argv )) fatal_error( "could not exec wineserver\n" );
        waitpid( pid, &status, 0 );
        status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        if (status == 2) return;  /* server lock held by someone else, will retry later */
        if (status) exit(status);  /* server failed */
        started = TRUE;
    }
}


/***********************************************************************
 *           KeAddSystemServiceTable
 */
BOOLEAN KeAddSystemServiceTable( ULONG_PTR *funcs, ULONG_PTR *counters, ULONG limit,
                                 BYTE *arguments, ULONG index )
{
    if (index >= ARRAY_SIZE(KeServiceDescriptorTable)) return FALSE;
    KeServiceDescriptorTable[index].ServiceTable  = funcs;
    KeServiceDescriptorTable[index].CounterTable  = counters;
    KeServiceDescriptorTable[index].ServiceLimit  = limit;
    KeServiceDescriptorTable[index].ArgumentTable = arguments;
    return TRUE;
}


/*************************************************************************
 *		map_so_dll
 *
 * Map a builtin dll in memory and fixup RVAs.
 */
static NTSTATUS map_so_dll( const IMAGE_NT_HEADERS *nt_descr, HMODULE module )
{
    static const char builtin_signature[32] = "Wine builtin DLL";
    IMAGE_DATA_DIRECTORY *dir;
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sec;
    BYTE *addr = (BYTE *)module;
    DWORD code_start, code_end, data_start, data_end;
    DWORD align_mask = nt_descr->OptionalHeader.SectionAlignment - 1;
    int delta, nb_sections = 2;  /* code + data */
    unsigned int i;

    code_start = (sizeof(IMAGE_DOS_HEADER)
                  + sizeof(builtin_signature)
                  + sizeof(IMAGE_NT_HEADERS)
                  + nb_sections * sizeof(IMAGE_SECTION_HEADER)
                  + align_mask) & ~align_mask;

    if (anon_mmap_fixed( addr, code_start, PROT_READ | PROT_WRITE, 0 ) != addr) return STATUS_NO_MEMORY;

    dos = (IMAGE_DOS_HEADER *)addr;
    nt  = (IMAGE_NT_HEADERS *)((BYTE *)(dos + 1) + sizeof(builtin_signature));
    sec = (IMAGE_SECTION_HEADER *)(nt + 1);

    /* build the DOS and NT headers */

    dos->e_magic    = IMAGE_DOS_SIGNATURE;
    dos->e_cblp     = 0x90;
    dos->e_cp       = 3;
    dos->e_cparhdr  = (sizeof(*dos) + 0xf) / 0x10;
    dos->e_minalloc = 0;
    dos->e_maxalloc = 0xffff;
    dos->e_ss       = 0x0000;
    dos->e_sp       = 0x00b8;
    dos->e_lfanew   = sizeof(*dos) + sizeof(builtin_signature);
    memcpy( dos + 1, builtin_signature, sizeof(builtin_signature) );

    *nt = *nt_descr;

    delta      = (const BYTE *)nt_descr - addr;
    data_start = delta & ~align_mask;
#ifdef __APPLE__
    {
        Dl_info dli;
        unsigned long data_size;
        /* need the mach_header, not the PE header, to give to getsegmentdata(3) */
        dladdr(addr, &dli);
        code_end   = getsegmentdata(dli.dli_fbase, "__DATA", &data_size) - addr;
        data_end   = (code_end + data_size + align_mask) & ~align_mask;
    }
#else
    code_end   = data_start;
    data_end   = (nt->OptionalHeader.SizeOfImage + delta + align_mask) & ~align_mask;
#endif

    fixup_rva_ptrs( &nt->OptionalHeader.AddressOfEntryPoint, addr, 1 );

    nt->FileHeader.NumberOfSections                = nb_sections;
    nt->OptionalHeader.BaseOfCode                  = code_start;
#ifndef _WIN64
    nt->OptionalHeader.BaseOfData                  = data_start;
#endif
    nt->OptionalHeader.SizeOfCode                  = code_end - code_start;
    nt->OptionalHeader.SizeOfInitializedData       = data_end - data_start;
    nt->OptionalHeader.SizeOfUninitializedData     = 0;
    nt->OptionalHeader.SizeOfImage                 = data_end;
    nt->OptionalHeader.ImageBase                   = (ULONG_PTR)addr;

    /* build the code section */

    memcpy( sec->Name, ".text", sizeof(".text") );
    sec->SizeOfRawData = code_end - code_start;
    sec->Misc.VirtualSize = sec->SizeOfRawData;
    sec->VirtualAddress   = code_start;
    sec->PointerToRawData = code_start;
    sec->Characteristics  = (IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ);
    sec++;

    /* build the data section */

    memcpy( sec->Name, ".data", sizeof(".data") );
    sec->SizeOfRawData = data_end - data_start;
    sec->Misc.VirtualSize = sec->SizeOfRawData;
    sec->VirtualAddress   = data_start;
    sec->PointerToRawData = data_start;
    sec->Characteristics  = (IMAGE_SCN_CNT_INITIALIZED_DATA |
                             IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ);
    sec++;

    for (i = 0; i < nt->OptionalHeader.NumberOfRvaAndSizes; i++)
        fixup_rva_dwords( &nt->OptionalHeader.DataDirectory[i].VirtualAddress, delta, 1 );

    /* build the import directory */

    dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_IMPORT_DIRECTORY];
    if (dir->Size)
    {
        IMAGE_IMPORT_DESCRIPTOR *imports = (IMAGE_IMPORT_DESCRIPTOR *)(addr + dir->VirtualAddress);

        while (imports->Name)
        {
            fixup_rva_dwords( &imports->OriginalFirstThunk, delta, 1 );
            fixup_rva_dwords( &imports->Name, delta, 1 );
            fixup_rva_dwords( &imports->FirstThunk, delta, 1 );
            if (imports->OriginalFirstThunk)
                fixup_rva_names( (UINT_PTR *)(addr + imports->OriginalFirstThunk), delta );
            if (imports->FirstThunk)
                fixup_rva_names( (UINT_PTR *)(addr + imports->FirstThunk), delta );
            imports++;
        }
    }

    /* build the resource directory */

    dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_RESOURCE_DIRECTORY];
    if (dir->Size)
    {
        void *ptr = addr + dir->VirtualAddress;
        fixup_so_resources( ptr, ptr, delta );
    }

    /* build the export directory */

    dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_EXPORT_DIRECTORY];
    if (dir->Size)
    {
        IMAGE_EXPORT_DIRECTORY *exports = (IMAGE_EXPORT_DIRECTORY *)(addr + dir->VirtualAddress);

        fixup_rva_dwords( &exports->Name, delta, 1 );
        fixup_rva_dwords( &exports->AddressOfFunctions, delta, 1 );
        fixup_rva_dwords( &exports->AddressOfNames, delta, 1 );
        fixup_rva_dwords( &exports->AddressOfNameOrdinals, delta, 1 );
        fixup_rva_dwords( (DWORD *)(addr + exports->AddressOfNames), delta, exports->NumberOfNames );
        fixup_rva_ptrs( addr + exports->AddressOfFunctions, addr, exports->NumberOfFunctions );
    }

    /* build the delay import directory */

    dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
    if (dir->Size)
    {
        IMAGE_DELAYLOAD_DESCRIPTOR *imports = (IMAGE_DELAYLOAD_DESCRIPTOR *)(addr + dir->VirtualAddress);

        while (imports->DllNameRVA)
        {
            fixup_rva_dwords( &imports->DllNameRVA, delta, 1 );
            fixup_rva_dwords( &imports->ModuleHandleRVA, delta, 1 );
            fixup_rva_dwords( &imports->ImportAddressTableRVA, delta, 1 );
            fixup_rva_dwords( &imports->ImportNameTableRVA, delta, 1 );
            fixup_rva_dwords( &imports->BoundImportAddressTableRVA, delta, 1 );
            fixup_rva_dwords( &imports->UnloadInformationTableRVA, delta, 1 );
            fixup_rva_names( (UINT_PTR *)(addr + imports->ImportNameTableRVA), delta );
            imports++;
        }
    }

    return STATUS_SUCCESS;
}

static ULONG_PTR find_ordinal_export( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports, DWORD ordinal )
{
    const DWORD *functions = (const DWORD *)((BYTE *)module + exports->AddressOfFunctions);

    if (ordinal >= exports->NumberOfFunctions) return 0;
    if (!functions[ordinal]) return 0;
    return (ULONG_PTR)module + functions[ordinal];
}

static ULONG_PTR find_named_export( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                                    const char *name )
{
    const WORD *ordinals = (const WORD *)((BYTE *)module + exports->AddressOfNameOrdinals);
    const DWORD *names = (const DWORD *)((BYTE *)module + exports->AddressOfNames);
    int min = 0, max = exports->NumberOfNames - 1;

    while (min <= max)
    {
        int res, pos = (min + max) / 2;
        char *ename = (char *)module + names[pos];
        if (!(res = strcmp( ename, name ))) return find_ordinal_export( module, exports, ordinals[pos] );
        if (res > 0) max = pos - 1;
        else min = pos + 1;
    }
    return 0;
}

static inline void *get_rva( void *module, ULONG_PTR addr )
{
    return (BYTE *)module + addr;
}

static const void *get_module_data_dir( HMODULE module, ULONG dir, ULONG *size )
{
    const IMAGE_NT_HEADERS *nt = get_rva( module, ((IMAGE_DOS_HEADER *)module)->e_lfanew );
    const IMAGE_DATA_DIRECTORY *data;

    if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        data = &((const IMAGE_NT_HEADERS64 *)nt)->OptionalHeader.DataDirectory[dir];
    else if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        data = &((const IMAGE_NT_HEADERS32 *)nt)->OptionalHeader.DataDirectory[dir];
    else
        return NULL;
    if (!data->VirtualAddress || !data->Size) return NULL;
    if (size) *size = data->Size;
    return get_rva( module, data->VirtualAddress );
}

/***********************************************************************
 *           fill_builtin_image_info
 */
static void fill_builtin_image_info( void *module, struct pe_image_info *info )
{
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)module;
    const IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)((const BYTE *)dos + dos->e_lfanew);

    memset( info, 0, sizeof(*info) );
    info->base            = nt->OptionalHeader.ImageBase;
    info->entry_point     = nt->OptionalHeader.AddressOfEntryPoint;
    info->map_size        = nt->OptionalHeader.SizeOfImage;
    info->stack_size      = nt->OptionalHeader.SizeOfStackReserve;
    info->stack_commit    = nt->OptionalHeader.SizeOfStackCommit;
    info->subsystem       = nt->OptionalHeader.Subsystem;
    info->subsystem_minor = nt->OptionalHeader.MinorSubsystemVersion;
    info->subsystem_major = nt->OptionalHeader.MajorSubsystemVersion;
    info->osversion_major = nt->OptionalHeader.MajorOperatingSystemVersion;
    info->osversion_minor = nt->OptionalHeader.MinorOperatingSystemVersion;
    info->image_charact   = nt->FileHeader.Characteristics;
    info->dll_charact     = nt->OptionalHeader.DllCharacteristics;
    info->machine         = nt->FileHeader.Machine;
    info->contains_code   = TRUE;
    info->wine_builtin    = TRUE;
    info->header_size     = nt->OptionalHeader.SizeOfHeaders;
    info->file_size       = nt->OptionalHeader.SizeOfImage;
    info->checksum        = nt->OptionalHeader.CheckSum;
}


/***********************************************************************
 *           dlopen_dll
 */
static NTSTATUS dlopen_dll( const char *so_name, UNICODE_STRING *nt_name, void **ret_module,
                            struct pe_image_info *image_info, BOOL prefer_native )
{
    void *module, *handle;
    const IMAGE_NT_HEADERS *nt;

    handle = dlopen( so_name, RTLD_NOW );
    if (!handle)
    {
        WARN( "failed to load .so lib %s: %s\n", debugstr_a(so_name), dlerror() );
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    if (!(nt = dlsym( handle, "__wine_spec_nt_header" )))
    {
        ERR( "invalid .so library %s, too old?\n", debugstr_a(so_name));
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    module = (HMODULE)((nt->OptionalHeader.ImageBase + 0xffff) & ~0xffff);
    if (get_builtin_so_handle( module ))  /* already loaded */
    {
        fill_builtin_image_info( module, image_info );
        *ret_module = module;
        dlclose( handle );
        return STATUS_SUCCESS;
    }

    if (map_so_dll( nt, module ))
    {
        dlclose( handle );
        return STATUS_NO_MEMORY;
    }

    fill_builtin_image_info( module, image_info );
    if (prefer_native && (image_info->dll_charact & IMAGE_DLLCHARACTERISTICS_PREFER_NATIVE))
    {
        TRACE( "%s has prefer-native flag, ignoring builtin\n", debugstr_a(so_name) );
        dlclose( handle );
        return STATUS_IMAGE_ALREADY_LOADED;
    }

    if (virtual_create_builtin_view( module, nt_name, image_info, handle ))
    {
        dlclose( handle );
        return STATUS_NO_MEMORY;
    }
    *ret_module = module;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           load_so_dll
 */
static NTSTATUS load_so_dll( void *args )
{
    static const WCHAR soW[] = {'.','s','o',0};
    struct load_so_dll_params *params = args;
    UNICODE_STRING *nt_name = &params->nt_name;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING redir;
    struct pe_image_info info;
    char *unix_name;
    NTSTATUS status;
    DWORD len;

    if (get_load_order( nt_name ) == LO_DISABLED) return STATUS_DLL_NOT_FOUND;
    InitializeObjectAttributes( &attr, nt_name, OBJ_CASE_INSENSITIVE, 0, 0 );
    get_redirect( &attr, &redir );

    if (nt_to_unix_file_name( &attr, &unix_name, FILE_OPEN ))
    {
        free( redir.Buffer );
        return STATUS_DLL_NOT_FOUND;
    }

    /* remove .so extension from Windows name */
    len = nt_name->Length / sizeof(WCHAR);
    if (len > 3 && !wcsicmp( nt_name->Buffer + len - 3, soW )) nt_name->Length -= 3 * sizeof(WCHAR);

    status = dlopen_dll( unix_name, nt_name, params->module, &info, FALSE );
    free( unix_name );
    free( redir.Buffer );
    return status;
}


static const unixlib_entry_t unix_call_funcs[] =
{
    load_so_dll,
    unwind_builtin_dll,
    unixcall_wine_dbg_write,
    unixcall_wine_server_call,
    unixcall_wine_server_fd_to_handle,
    unixcall_wine_server_handle_to_fd,
    unixcall_wine_spawnvp,
    system_time_precise,
};


#ifdef _WIN64

static NTSTATUS wow64_load_so_dll( void *args ) { return STATUS_INVALID_IMAGE_FORMAT; }
static NTSTATUS wow64_unwind_builtin_dll( void *args ) { return STATUS_UNSUCCESSFUL; }

const unixlib_entry_t unix_call_wow64_funcs[] =
{
    wow64_load_so_dll,
    wow64_unwind_builtin_dll,
    wow64_wine_dbg_write,
    wow64_wine_server_call,
    wow64_wine_server_fd_to_handle,
    wow64_wine_server_handle_to_fd,
    wow64_wine_spawnvp,
    system_time_precise,
};

#endif  /* _WIN64 */


/* check if the library is the correct architecture */
/* only returns false for a valid library of the wrong arch */
static int check_library_arch( int fd )
{
#ifdef __APPLE__
    struct  /* Mach-O header */
    {
        unsigned int magic;
        unsigned int cputype;
    } header;

    if (read( fd, &header, sizeof(header) ) != sizeof(header)) return 1;
    if (header.magic != 0xfeedface) return 1;
    if (sizeof(void *) == sizeof(int)) return !(header.cputype >> 24);
    else return (header.cputype >> 24) == 1; /* CPU_ARCH_ABI64 */
#else
    struct  /* ELF header */
    {
        unsigned char magic[4];
        unsigned char class;
        unsigned char data;
        unsigned char version;
    } header;

    if (read( fd, &header, sizeof(header) ) != sizeof(header)) return 1;
    if (memcmp( header.magic, "\177ELF", 4 )) return 1;
    if (header.version != 1 /* EV_CURRENT */) return 1;
#ifdef WORDS_BIGENDIAN
    if (header.data != 2 /* ELFDATA2MSB */) return 1;
#else
    if (header.data != 1 /* ELFDATA2LSB */) return 1;
#endif
    if (sizeof(void *) == sizeof(int)) return header.class == 1; /* ELFCLASS32 */
    else return header.class == 2; /* ELFCLASS64 */
#endif
}

static inline char *prepend( char *buffer, const char *str, size_t len )
{
    return memcpy( buffer - len, str, len );
}

static inline char *prepend_build_dir_path( char *ptr, const char *ext, const char *arch_dir,
                                            const char *top_dir, const char *build_dir )
{
    char *name = ptr;
    unsigned int namelen = strlen(name), extlen = strlen(ext);

    if (namelen > extlen && !strcmp( name + namelen - extlen, ext )) namelen -= extlen;
    ptr = prepend( ptr, arch_dir, strlen(arch_dir) );
    ptr = prepend( ptr, name, namelen );
    ptr = prepend( ptr, top_dir, strlen(top_dir) );
    ptr = prepend( ptr, build_dir, strlen(build_dir) );
    return ptr;
}


/***********************************************************************
 *	open_dll_file
 *
 * Open a file for a new dll. Helper for open_builtin_pe_file.
 */
static NTSTATUS open_dll_file( const char *name, OBJECT_ATTRIBUTES *attr, HANDLE *mapping )
{
    LARGE_INTEGER size;
    NTSTATUS status;
    HANDLE handle;

    if ((status = open_unix_file( &handle, name, GENERIC_READ | SYNCHRONIZE, attr, 0,
                                  FILE_SHARE_READ | FILE_SHARE_DELETE, FILE_OPEN,
                                  FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE, NULL, 0 )))
    {
        if (status != STATUS_OBJECT_PATH_NOT_FOUND && status != STATUS_OBJECT_NAME_NOT_FOUND)
        {
            /* if the file exists but failed to open, report the error */
            struct stat st;
            if (!stat( name, &st )) return status;
        }
        /* otherwise continue searching */
        return STATUS_DLL_NOT_FOUND;
    }

    size.QuadPart = 0;
    status = NtCreateSection( mapping, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY |
                              SECTION_MAP_READ | SECTION_MAP_EXECUTE,
                              NULL, &size, PAGE_EXECUTE_READ, SEC_IMAGE, handle );
    NtClose( handle );
    return status;
}


/***********************************************************************
 *           open_builtin_pe_file
 */
static NTSTATUS open_builtin_pe_file( const char *name, OBJECT_ATTRIBUTES *attr, void **module,
                                      SIZE_T *size, SECTION_IMAGE_INFORMATION *image_info,
                                      ULONG_PTR limit_low, ULONG_PTR limit_high,
                                      WORD machine, BOOL prefer_native )
{
    NTSTATUS status;
    HANDLE mapping;

    *module = NULL;
    status = open_dll_file( name, attr, &mapping );
    if (!status)
    {
        status = virtual_map_builtin_module( mapping, module, size, image_info,
                                             limit_low, limit_high, machine, prefer_native );
        NtClose( mapping );
    }
    return status;
}


/***********************************************************************
 *           open_builtin_so_file
 */
static NTSTATUS open_builtin_so_file( char *name, OBJECT_ATTRIBUTES *attr, void **module,
                                      SECTION_IMAGE_INFORMATION *image_info, USHORT search_machine,
                                      USHORT load_machine, BOOL prefer_native )
{
    NTSTATUS status = STATUS_DLL_NOT_FOUND;
    int fd;
    char *end = name + strlen( name );

    if (search_machine != current_machine) return status;
    if (load_machine && load_machine != current_machine) return status;

    *module = NULL;
    strcpy( end, ".so" );
    if ((fd = open( name, O_RDONLY )) == -1) goto done;

    if (check_library_arch( fd ))
    {
        struct pe_image_info info;

        status = dlopen_dll( name, attr->ObjectName, module, &info, prefer_native );
        if (!status) virtual_fill_image_information( &info, image_info );
        else if (status != STATUS_IMAGE_ALREADY_LOADED)
        {
            ERR( "failed to load .so lib %s\n", debugstr_a(name) );
            status = STATUS_PROCEDURE_NOT_FOUND;
        }
    }
    else status = STATUS_NOT_SUPPORTED;

    close( fd );
 done:
    *end = 0;
    return status;
}


/***********************************************************************
 *           find_builtin_dll
 */
static NTSTATUS find_builtin_dll( UNICODE_STRING *nt_name, void **module, SIZE_T *size_ptr,
                                  SECTION_IMAGE_INFORMATION *image_info, ULONG_PTR limit_low,
                                  ULONG_PTR limit_high, USHORT search_machine,
                                  USHORT load_machine, BOOL prefer_native )
{
    unsigned int i, pos, namepos, maxlen = 0;
    unsigned int len = nt_name->Length / sizeof(WCHAR);
    char *ptr = NULL, *file, *ext = NULL;
    const char *pe_dir = get_pe_dir( search_machine );
    const char *so_dir = get_so_dir( current_machine );
    const char *pe_build_dir = build_dir;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status = STATUS_DLL_NOT_FOUND;
    BOOL found_image = FALSE;

    for (i = namepos = 0; i < len; i++)
        if (nt_name->Buffer[i] == '/' || nt_name->Buffer[i] == '\\') namepos = i + 1;
    len -= namepos;
    if (!len) return STATUS_DLL_NOT_FOUND;
    InitializeObjectAttributes( &attr, nt_name, 0, 0, NULL );

    if (build_dir)
    {
        if (alt_build_dir && search_machine == get_alt_machine( current_machine ))
            pe_build_dir = alt_build_dir;
        maxlen = max( strlen(build_dir), strlen(pe_build_dir) ) + sizeof("/programs/") + len;
    }
    maxlen = max( maxlen, dll_path_maxlen + 1 ) + len + sizeof("/aarch64-windows") + sizeof(".so");

    if (!(file = malloc( maxlen ))) return STATUS_NO_MEMORY;

    pos = maxlen - len - sizeof(".so");
    /* we don't want to depend on the current codepage here */
    for (i = 0; i < len; i++)
    {
        if (nt_name->Buffer[namepos + i] > 127) goto done;
        file[pos + i] = (char)nt_name->Buffer[namepos + i];
        if (file[pos + i] >= 'A' && file[pos + i] <= 'Z') file[pos + i] += 'a' - 'A';
        else if (file[pos + i] == '.') ext = file + pos + i;
    }
    file[pos + len] = 0;
    file[--pos] = '/';

    if (build_dir)
    {
        /* try as a dll */
        ptr = prepend_build_dir_path( file + pos, ".dll", pe_dir, "/dlls", pe_build_dir );
        status = open_builtin_pe_file( ptr, &attr, module, size_ptr, image_info,
                                       limit_low, limit_high, load_machine, prefer_native );
        ptr = prepend_build_dir_path( file + pos, ".dll", "", "/dlls", build_dir );
        if (status != STATUS_DLL_NOT_FOUND) goto done;
        status = open_builtin_so_file( ptr, &attr, module, image_info,
                                       search_machine, load_machine, prefer_native );
        if (status != STATUS_DLL_NOT_FOUND) goto done;

        /* now as a program */
        ptr = prepend_build_dir_path( file + pos, ".exe", pe_dir, "/programs", pe_build_dir );
        status = open_builtin_pe_file( ptr, &attr, module, size_ptr, image_info,
                                       limit_low, limit_high, load_machine, prefer_native );
        ptr = prepend_build_dir_path( file + pos, ".exe", "", "/programs", build_dir );
        if (status != STATUS_DLL_NOT_FOUND) goto done;
        status = open_builtin_so_file( ptr, &attr, module, image_info,
                                       search_machine, load_machine, prefer_native );
        if (status != STATUS_DLL_NOT_FOUND) goto done;
    }

    for (i = 0; dll_paths[i]; i++)
    {
        ptr = file + pos;
        ptr = prepend( ptr, pe_dir, strlen(pe_dir) );
        ptr = prepend( ptr, dll_paths[i], strlen(dll_paths[i]) );
        status = open_builtin_pe_file( ptr, &attr, module, size_ptr, image_info, limit_low, limit_high,
                                       load_machine, prefer_native );
        /* use so dir for unix lib */
        ptr = file + pos;
        ptr = prepend( ptr, so_dir, strlen(so_dir) );
        ptr = prepend( ptr, dll_paths[i], strlen(dll_paths[i]) );
        if (status != STATUS_DLL_NOT_FOUND) goto done;
        status = open_builtin_so_file( ptr, &attr, module, image_info,
                                       search_machine, load_machine, prefer_native );
        if (status != STATUS_DLL_NOT_FOUND) goto done;
        ptr = prepend( file + pos, dll_paths[i], strlen(dll_paths[i]) );
        status = open_builtin_pe_file( ptr, &attr, module, size_ptr, image_info, limit_low, limit_high,
                                       load_machine, prefer_native );
        if (status == STATUS_NOT_SUPPORTED)
        {
            found_image = TRUE;
            continue;
        }
        if (status != STATUS_DLL_NOT_FOUND) goto done;
        status = open_builtin_so_file( ptr, &attr, module, image_info,
                                       search_machine, load_machine, prefer_native );
        if (status == STATUS_NOT_SUPPORTED) found_image = TRUE;
        else if (status != STATUS_DLL_NOT_FOUND) goto done;
    }

    if (found_image) status = STATUS_NOT_SUPPORTED;
    WARN( "cannot find builtin library for %s\n", debugstr_us(nt_name) );
done:
    if (NT_SUCCESS(status) && ext)
    {
        strcpy( ext, ".so" );
        load_builtin_unixlib( *module, ptr );
    }
    free( file );
    return status;
}


/***********************************************************************
 *           load_builtin
 *
 * Load the builtin dll if specified by load order configuration.
 * Return STATUS_IMAGE_ALREADY_LOADED if we should keep the native one that we have found.
 */
NTSTATUS load_builtin( const struct pe_image_info *image_info, WCHAR *filename, USHORT machine,
                       SECTION_IMAGE_INFORMATION *info, void **module, SIZE_T *size,
                       ULONG_PTR limit_low, ULONG_PTR limit_high )
{
    NTSTATUS status;
    UNICODE_STRING nt_name;
    USHORT search_machine = image_info->machine;
    enum loadorder loadorder;

    init_unicode_string( &nt_name, filename );
    loadorder = get_load_order( &nt_name );

    if (loadorder == LO_DISABLED) return STATUS_DLL_NOT_FOUND;

    if (image_info->wine_builtin)
    {
        if (loadorder == LO_NATIVE) return STATUS_DLL_NOT_FOUND;
        loadorder = LO_BUILTIN_NATIVE;  /* load builtin, then fallback to the file we found */
    }
    else if (image_info->wine_fakedll)
    {
        TRACE( "%s is a fake Wine dll\n", debugstr_w(filename) );
        if (loadorder == LO_NATIVE) return STATUS_DLL_NOT_FOUND;
        loadorder = LO_BUILTIN;  /* builtin with no fallback since mapping a fake dll is not useful */
    }

    if (is_arm64ec() && image_info->is_hybrid && search_machine == IMAGE_FILE_MACHINE_AMD64)
        search_machine = current_machine;

    switch (loadorder)
    {
    case LO_NATIVE:
    case LO_NATIVE_BUILTIN:
        return STATUS_IMAGE_ALREADY_LOADED;
    case LO_BUILTIN:
        return find_builtin_dll( &nt_name, module, size, info, limit_low, limit_high,
                                 search_machine, machine, FALSE );
    default:
        status = find_builtin_dll( &nt_name, module, size, info, limit_low, limit_high,
                                   search_machine, machine, (loadorder == LO_DEFAULT) );
        if (status == STATUS_DLL_NOT_FOUND || status == STATUS_NOT_SUPPORTED)
            return STATUS_IMAGE_ALREADY_LOADED;
        return status;
    }
}


/***************************************************************************
 *	get_machine_wow64_dir
 *
 * cf. GetSystemWow64Directory2.
 */
static const WCHAR *get_machine_wow64_dir( WORD machine )
{
    static const WCHAR system32[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\','s','y','s','t','e','m','3','2','\\',0};
    static const WCHAR syswow64[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\','s','y','s','w','o','w','6','4','\\',0};
    static const WCHAR sysarm32[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\','s','y','s','a','r','m','3','2','\\',0};

    if (machine == native_machine) machine = IMAGE_FILE_MACHINE_TARGET_HOST;

    switch (machine)
    {
    case IMAGE_FILE_MACHINE_TARGET_HOST: return system32;
    case IMAGE_FILE_MACHINE_I386:        return syswow64;
    case IMAGE_FILE_MACHINE_ARMNT:       return sysarm32;
    default: return NULL;
    }
}


/***************************************************************************
 *	is_builtin_path
 *
 * Check if path is inside a system directory, to support loading builtins
 * when the corresponding file doesn't exist yet.
 */
BOOL is_builtin_path( const UNICODE_STRING *path, WORD *machine )
{
    unsigned int i, len = path->Length / sizeof(WCHAR), dirlen;
    const WCHAR *sysdir, *p = path->Buffer;

    /* only fake builtin existence during prefix bootstrap */
    if (!is_prefix_bootstrap) return FALSE;

    for (i = 0; i < supported_machines_count; i++)
    {
        sysdir = get_machine_wow64_dir( supported_machines[i] );
        if (!sysdir) continue;
        dirlen = wcslen( sysdir );
        if (len <= dirlen) continue;
        if (wcsnicmp( p, sysdir, dirlen )) continue;
        /* check for remaining path components */
        for (p += dirlen, len -= dirlen; len; p++, len--) if (*p == '\\') return FALSE;
        *machine = supported_machines[i];
        return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           open_main_image
 */
static NTSTATUS open_main_image( WCHAR *image, void **module, SECTION_IMAGE_INFORMATION *info,
                                 enum loadorder loadorder, USHORT machine )
{
    static const WCHAR soW[] = {'.','s','o',0};
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    struct pe_image_info pe_info;
    SIZE_T size = 0;
    char *unix_name;
    NTSTATUS status;
    HANDLE mapping;
    WCHAR *p;

    if (loadorder == LO_DISABLED) NtTerminateProcess( GetCurrentProcess(), STATUS_DLL_NOT_FOUND );

    init_unicode_string( &nt_name, image );
    InitializeObjectAttributes( &attr, &nt_name, OBJ_CASE_INSENSITIVE, 0, NULL );
    if (nt_to_unix_file_name( &attr, &unix_name, FILE_OPEN )) return STATUS_DLL_NOT_FOUND;

    status = open_dll_file( unix_name, &attr, &mapping );
    if (!status)
    {
        status = virtual_map_module( mapping, module, &size, info, 0, 0, machine );
        if (status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH && info->ComPlusNativeReady)
        {
            info->Machine = native_machine;
            status = STATUS_SUCCESS;
        }
        NtClose( mapping );
    }
    else if (status == STATUS_INVALID_IMAGE_NOT_MZ && loadorder != LO_NATIVE)
    {
        /* remove .so extension from Windows name */
        p = image + wcslen(image);
        if (p - image > 3 && !wcsicmp( p - 3, soW ))
        {
            p[-3] = 0;
            nt_name.Length -= 3 * sizeof(WCHAR);
        }
        status = dlopen_dll( unix_name, &nt_name, module, &pe_info, FALSE );
        if (!status) virtual_fill_image_information( &pe_info, info );
    }
    free( unix_name );
    return status;
}


/***********************************************************************
 *           load_main_exe
 */
NTSTATUS load_main_exe( const WCHAR *dos_name, const char *unix_name, const WCHAR *curdir,
                        USHORT load_machine, WCHAR **image, void **module )
{
    enum loadorder loadorder = LO_INVALID;
    UNICODE_STRING nt_name;
    WCHAR *tmp = NULL;
    BOOL contains_path;
    unsigned int status;
    SIZE_T size;
    struct stat st;
    USHORT search_machine;

    /* special case for Unix file name */
    if (unix_name && unix_name[0] == '/' && !stat( unix_name, &st ))
    {
        if ((status = unix_to_nt_file_name( unix_name, image ))) goto failed;
        init_unicode_string( &nt_name, *image );
        loadorder = get_load_order( &nt_name );
        status = open_main_image( *image, module, &main_image_info, loadorder, load_machine );
        if (status != STATUS_DLL_NOT_FOUND) return status;
        free( *image );
    }

    if (!dos_name)
    {
        dos_name = tmp = malloc( (strlen(unix_name) + 1) * sizeof(WCHAR) );
        ntdll_umbstowcs( unix_name, strlen(unix_name) + 1, tmp, strlen(unix_name) + 1 );
    }
    contains_path = (wcschr( dos_name, '/' ) ||
                     wcschr( dos_name, '\\' ) ||
                     (dos_name[0] && dos_name[1] == ':'));

    if ((status = get_full_path( dos_name, curdir, image ))) goto failed;
    free( tmp );

    init_unicode_string( &nt_name, *image );
    if (loadorder == LO_INVALID) loadorder = get_load_order( &nt_name );

    status = open_main_image( *image, module, &main_image_info, loadorder, load_machine );
    if (status != STATUS_DLL_NOT_FOUND) return status;

    /* if path is in system dir, we can load the builtin even if the file itself doesn't exist */
    if (loadorder != LO_NATIVE && is_builtin_path( &nt_name, &search_machine ))
    {
        status = find_builtin_dll( &nt_name, module, &size, &main_image_info, 0, 0,
                                   search_machine, load_machine, FALSE );
        if (status != STATUS_DLL_NOT_FOUND) return status;
    }
    if (!contains_path) return STATUS_DLL_NOT_FOUND;

failed:
    MESSAGE( "wine: failed to open %s: %x\n",
             unix_name ? debugstr_a(unix_name) : debugstr_w(dos_name), status );
    NtTerminateProcess( GetCurrentProcess(), status );
    return status;  /* unreached */
}


/***********************************************************************
 *           load_start_exe
 *
 * Load start.exe as main image.
 */
NTSTATUS load_start_exe( WCHAR **image, void **module )
{
    static const WCHAR startW[] = {'s','t','a','r','t','.','e','x','e',0};
    UNICODE_STRING nt_name;
    unsigned int status;
    SIZE_T size;

    *image = malloc( sizeof("\\??\\C:\\windows\\system32\\start.exe") * sizeof(WCHAR) );
    wcscpy( *image, get_machine_wow64_dir( current_machine ));
    wcscat( *image, startW );
    init_unicode_string( &nt_name, *image );
    status = find_builtin_dll( &nt_name, module, &size, &main_image_info, 0, 0, current_machine, 0, FALSE );
    if (!NT_SUCCESS(status))
    {
        MESSAGE( "wine: failed to load start.exe: %x\n", status );
        NtTerminateProcess( GetCurrentProcess(), status );
    }
    return status;
}


/***********************************************************************
 *           load_ntdll_functions
 */
static void load_ntdll_functions( HMODULE module )
{
    void **p__wine_syscall_dispatcher;
    void **p__wine_unix_call_dispatcher;
    void **p__wine_unix_call_dispatcher_arm64ec = NULL;
    unixlib_handle_t *p__wine_unixlib_handle;
    const IMAGE_EXPORT_DIRECTORY *exports;

    exports = get_module_data_dir( module, IMAGE_DIRECTORY_ENTRY_EXPORT, NULL );
    assert( exports );

#define GET_FUNC(name) \
    if (!(p##name = (void *)find_named_export( module, exports, #name ))) \
        ERR( "%s not found\n", #name )

    GET_FUNC( DbgUiRemoteBreakin );
    GET_FUNC( KiRaiseUserExceptionDispatcher );
    GET_FUNC( KiUserExceptionDispatcher );
    GET_FUNC( KiUserApcDispatcher );
    GET_FUNC( KiUserCallbackDispatcher );
    GET_FUNC( LdrInitializeThunk );
    GET_FUNC( LdrSystemDllInitBlock );
    GET_FUNC( RtlUserThreadStart );
    GET_FUNC( __wine_ctrl_routine );
    GET_FUNC( __wine_syscall_dispatcher );
    GET_FUNC( __wine_unix_call_dispatcher );
    GET_FUNC( __wine_unixlib_handle );
    if (is_arm64ec())
    {
        GET_FUNC( __wine_unix_call_dispatcher_arm64ec );
        GET_FUNC( KiUserEmulationDispatcher );
    }
    *p__wine_syscall_dispatcher = __wine_syscall_dispatcher;
    *p__wine_unixlib_handle = (UINT_PTR)unix_call_funcs;
    if (p__wine_unix_call_dispatcher_arm64ec)
    {
        /* redirect __wine_unix_call_dispatcher to __wine_unix_call_dispatcher_arm64ec */
        *p__wine_unix_call_dispatcher = *p__wine_unix_call_dispatcher_arm64ec;
        *p__wine_unix_call_dispatcher_arm64ec = __wine_unix_call_dispatcher;
    }
    else *p__wine_unix_call_dispatcher = __wine_unix_call_dispatcher;
#undef GET_FUNC
}


/***********************************************************************
 *           load_ntdll_wow64_functions
 */
static void load_ntdll_wow64_functions( HMODULE module )
{
    const IMAGE_EXPORT_DIRECTORY *exports;

    exports = get_module_data_dir( module, IMAGE_FILE_EXPORT_DIRECTORY, NULL );
    assert( exports );

    pLdrSystemDllInitBlock->ntdll_handle = (ULONG_PTR)module;

#define GET_FUNC(name) pLdrSystemDllInitBlock->p##name = find_named_export( module, exports, #name )
    GET_FUNC( KiUserApcDispatcher );
    GET_FUNC( KiUserCallbackDispatcher );
    GET_FUNC( KiUserExceptionDispatcher );
    GET_FUNC( LdrInitializeThunk );
    GET_FUNC( LdrSystemDllInitBlock );
    GET_FUNC( RtlUserThreadStart );
    GET_FUNC( RtlpFreezeTimeBias );
    GET_FUNC( RtlpQueryProcessDebugInformationRemote );
#undef GET_FUNC

    p__wine_ctrl_routine = (void *)find_named_export( module, exports, "__wine_ctrl_routine" );

#ifdef _WIN64
    {
        unixlib_handle_t *p__wine_unixlib_handle = (void *)find_named_export( module, exports,
                                                                              "__wine_unixlib_handle" );
        *p__wine_unixlib_handle = (UINT_PTR)unix_call_wow64_funcs;
    }
#endif

    /* also set the 32-bit LdrSystemDllInitBlock */
    memcpy( (void *)(ULONG_PTR)pLdrSystemDllInitBlock->pLdrSystemDllInitBlock,
            pLdrSystemDllInitBlock, sizeof(*pLdrSystemDllInitBlock) );
}


/***********************************************************************
 *           redirect_arm64ec_rva
 *
 * Redirect an address through the arm64ec redirection table.
 */
ULONG_PTR redirect_arm64ec_rva( void *base, ULONG_PTR rva, const IMAGE_ARM64EC_METADATA *metadata )
{
    const IMAGE_ARM64EC_REDIRECTION_ENTRY *map = get_rva( base, metadata->RedirectionMetadata );
    int min = 0, max = metadata->RedirectionMetadataCount - 1;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        if (map[pos].Source == rva) return map[pos].Destination;
        if (map[pos].Source < rva) min = pos + 1;
        else max = pos - 1;
    }
    return rva;
}


/***********************************************************************
 *           redirect_ntdll_functions
 *
 * Redirect ntdll functions on arm64ec.
 */
static void redirect_ntdll_functions( HMODULE module )
{
    const IMAGE_LOAD_CONFIG_DIRECTORY *loadcfg;
    const IMAGE_ARM64EC_METADATA *metadata;

    if (!(loadcfg = get_module_data_dir( module, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, NULL ))) return;
    if (!(metadata = (void *)loadcfg->CHPEMetadataPointer)) return;
#define REDIRECT(name) \
    p##name = get_rva( module, redirect_arm64ec_rva( module, (char *)p##name - (char *)module, metadata ))
    REDIRECT( DbgUiRemoteBreakin );
    REDIRECT( KiRaiseUserExceptionDispatcher );
    REDIRECT( KiUserExceptionDispatcher );
    REDIRECT( KiUserApcDispatcher );
    REDIRECT( KiUserCallbackDispatcher );
    REDIRECT( KiUserEmulationDispatcher );
    REDIRECT( LdrInitializeThunk );
    REDIRECT( RtlUserThreadStart );
#undef REDIRECT
}


/***********************************************************************
 *           load_ntdll
 */
static void load_ntdll(void)
{
    static WCHAR path[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\',
                           's','y','s','t','e','m','3','2','\\','n','t','d','l','l','.','d','l','l',0};
    const char *pe_dir = get_pe_dir( current_machine );
    USHORT machine = current_machine;
    unsigned int status;
    SECTION_IMAGE_INFORMATION info;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    void *module;
    SIZE_T size = 0;
    char *name = NULL;

    init_unicode_string( &str, path );
    InitializeObjectAttributes( &attr, &str, 0, 0, NULL );

    if (build_dir) asprintf( &name, "%s%s/ntdll.dll", ntdll_dir, pe_dir );
    else asprintf( &name, "%s%s/ntdll.dll", dll_dir, pe_dir );

    if (is_arm64ec()) machine = main_image_info.Machine;
    status = open_builtin_pe_file( name, &attr, &module, &size, &info, 0, 0, machine, FALSE );
    if (status == STATUS_DLL_NOT_FOUND)
    {
        free( name );
        asprintf( &name, "%s/ntdll.dll%c.so", ntdll_dir, 0 );
        status = open_builtin_so_file( name, &attr, &module, &info, machine, 0, FALSE );
    }
    if (status == STATUS_IMAGE_NOT_AT_BASE) status = virtual_relocate_module( module );
    if (status) fatal_error( "failed to load %s error %x\n", name, status );
    free( name );
    load_ntdll_functions( module );
    if (is_arm64ec()) redirect_ntdll_functions( module );
}


/***********************************************************************
 *           load_apiset_dll
 */
static void load_apiset_dll(void)
{
    static WCHAR path[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\',
                           's','y','s','t','e','m','3','2','\\',
                           'a','p','i','s','e','t','s','c','h','e','m','a','.','d','l','l',0};
    const char *pe_dir = get_pe_dir( current_machine );
    const IMAGE_NT_HEADERS *nt;
    const IMAGE_SECTION_HEADER *sec;
    API_SET_NAMESPACE *map;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    unsigned int status;
    HANDLE handle, mapping;
    SIZE_T size;
    char *name = NULL;
    void *ptr;
    UINT i;

    init_unicode_string( &str, path );
    InitializeObjectAttributes( &attr, &str, 0, 0, NULL );

    if (build_dir) asprintf( &name, "%s/dlls/apisetschema%s/apisetschema.dll", build_dir, pe_dir );
    else asprintf( &name, "%s%s/apisetschema.dll", dll_dir, pe_dir );
    status = open_unix_file( &handle, name, GENERIC_READ | SYNCHRONIZE, &attr, 0,
                             FILE_SHARE_READ | FILE_SHARE_DELETE, FILE_OPEN,
                             FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE, NULL, 0 );
    free( name );

    if (!status)
    {
        status = NtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ,
                                  NULL, NULL, PAGE_READONLY, SEC_COMMIT, handle );
        NtClose( handle );
    }
    if (!status)
    {
        status = map_section( mapping, &ptr, &size, PAGE_READONLY );
        NtClose( mapping );
    }
    if (!status)
    {
        nt = get_rva( ptr, ((IMAGE_DOS_HEADER *)ptr)->e_lfanew );
        sec = IMAGE_FIRST_SECTION( nt );

        for (i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
        {
            if (memcmp( (char *)sec->Name, ".apiset", 8 )) continue;
            map = (API_SET_NAMESPACE *)((char *)ptr + sec->PointerToRawData);
            if (sec->PointerToRawData < size &&
                size - sec->PointerToRawData >= sec->Misc.VirtualSize &&
                map->Version == 6 &&
                map->Size <= sec->Misc.VirtualSize)
            {
                peb->ApiSetMap = map;
                if (wow_peb) wow_peb->ApiSetMap = PtrToUlong(map);
                TRACE( "loaded %s apiset at %p\n", debugstr_w(path), map );
                return;
            }
            break;
        }
        NtUnmapViewOfSection( NtCurrentProcess(), ptr );
        status = STATUS_APISET_NOT_PRESENT;
    }
    ERR( "failed to load apiset: %x\n", status );
}


/***********************************************************************
 *           load_wow64_ntdll
 */
static void load_wow64_ntdll( USHORT machine )
{
    static const WCHAR ntdllW[] = {'n','t','d','l','l','.','d','l','l',0};
    SECTION_IMAGE_INFORMATION info;
    UNICODE_STRING nt_name;
    void *module;
    unsigned int status;
    SIZE_T size;
    const WCHAR *wow64_dir;
    WCHAR *path;

    if (machine == current_machine) return;
    if (!(wow64_dir = get_machine_wow64_dir( machine ))) return;

    path = malloc( sizeof("\\??\\C:\\windows\\system32\\ntdll.dll") * sizeof(WCHAR) );
    wcscpy( path, wow64_dir );
    wcscat( path, ntdllW );
    init_unicode_string( &nt_name, path );
    status = find_builtin_dll( &nt_name, &module, &size, &info, 0, 0, machine, 0, FALSE );
    if (status == STATUS_IMAGE_NOT_AT_BASE) status = virtual_relocate_module( module );
    if (status) fatal_error( "failed to load %s error %x\n", debugstr_w(path), status );
    load_ntdll_wow64_functions( module );
    TRACE("loaded %s at %p\n", debugstr_w(path), module );
    free( path );
}


/***********************************************************************
 *           get_image_address
 */
static ULONG_PTR get_image_address(void)
{
#ifdef HAVE_GETAUXVAL
    ULONG_PTR size, num, phdr_addr = getauxval( AT_PHDR );
    ElfW(Phdr) *phdr;

    if (!phdr_addr) return 0;
    phdr = (ElfW(Phdr) *)phdr_addr;
    size = getauxval( AT_PHENT );
    num = getauxval( AT_PHNUM );
    while (num--)
    {
        if (phdr->p_type == PT_PHDR) return phdr_addr - phdr->p_offset;
        phdr = (ElfW(Phdr) *)((char *)phdr + size);
    }
#elif defined(__APPLE__) && defined(TASK_DYLD_INFO)
    struct task_dyld_info dyld_info;
    mach_msg_type_number_t size = TASK_DYLD_INFO_COUNT;

    if (task_info(mach_task_self(), TASK_DYLD_INFO, (task_info_t)&dyld_info, &size) == KERN_SUCCESS)
        return dyld_info.all_image_info_addr;
#endif
    return 0;
}

/***********************************************************************
 *           start_main_thread
 */
static void start_main_thread(void)
{
    TEB *teb = virtual_alloc_first_teb();

    signal_init_threading();
    signal_alloc_thread( teb );
    dbg_init();
    startup_info_size = server_init_process();
    virtual_map_user_shared_data();
    init_cpu_info();
    init_files();
    init_startup_info();
    *(ULONG_PTR *)&peb->CloudFileFlags = get_image_address();
    set_load_order_app_name( main_wargv[0] );
    init_thread_stack( teb, 0, 0, 0 );
    NtCreateKeyedEvent( &keyed_event, GENERIC_READ | GENERIC_WRITE, NULL, 0 );
    load_ntdll();
    load_wow64_ntdll( main_image_info.Machine );
    load_apiset_dll();
    server_init_process_done();
}

#ifdef __ANDROID__

#ifndef WINE_JAVA_CLASS
#define WINE_JAVA_CLASS "org/winehq/wine/WineActivity"
#endif

JavaVM *java_vm = NULL;
jobject java_object = 0;
unsigned short java_gdt_sel = 0;

/* main Wine initialisation */
static jstring wine_init_jni( JNIEnv *env, jobject obj, jobjectArray cmdline, jobjectArray environment )
{
    char **argv;
    char *str;
    char error[1024];
    int i, argc, length;

    /* get the command line array */

    argc = (*env)->GetArrayLength( env, cmdline );
    for (i = length = 0; i < argc; i++)
    {
        jobject str_obj = (*env)->GetObjectArrayElement( env, cmdline, i );
        length += (*env)->GetStringUTFLength( env, str_obj ) + 1;
    }

    argv = malloc( (argc + 1) * sizeof(*argv) + length );
    str = (char *)(argv + argc + 1);
    for (i = 0; i < argc; i++)
    {
        jobject str_obj = (*env)->GetObjectArrayElement( env, cmdline, i );
        length = (*env)->GetStringUTFLength( env, str_obj );
        (*env)->GetStringUTFRegion( env, str_obj, 0,
                                    (*env)->GetStringLength( env, str_obj ), str );
        argv[i] = str;
        str[length] = 0;
        str += length + 1;
    }
    argv[argc] = NULL;

    /* set the environment variables */

    if (environment)
    {
        int count = (*env)->GetArrayLength( env, environment );
        for (i = 0; i < count - 1; i += 2)
        {
            jobject var_obj = (*env)->GetObjectArrayElement( env, environment, i );
            jobject val_obj = (*env)->GetObjectArrayElement( env, environment, i + 1 );
            const char *var = (*env)->GetStringUTFChars( env, var_obj, NULL );

            if (val_obj)
            {
                const char *val = (*env)->GetStringUTFChars( env, val_obj, NULL );
                setenv( var, val, 1 );
                if (!strcmp( var, "LD_LIBRARY_PATH" ))
                {
                    void (*update_func)( const char * ) = dlsym( RTLD_DEFAULT,
                                                                 "android_update_LD_LIBRARY_PATH" );
                    if (update_func) update_func( val );
                }
                else if (!strcmp( var, "WINEDEBUGLOG" ))
                {
                    int fd = open( val, O_WRONLY | O_CREAT | O_APPEND, 0666 );
                    if (fd != -1)
                    {
                        dup2( fd, 2 );
                        close( fd );
                    }
                }
                (*env)->ReleaseStringUTFChars( env, val_obj, val );
            }
            else unsetenv( var );

            (*env)->ReleaseStringUTFChars( env, var_obj, var );
        }
    }

    java_object = (*env)->NewGlobalRef( env, obj );

    main_argc = argc;
    main_argv = argv;

    init_paths();
    virtual_init();
    init_environment();

#ifdef __i386__
    {
        unsigned short java_fs;
        __asm__( "mov %%fs,%0" : "=r" (java_fs) );
        if (!(java_fs & 4)) java_gdt_sel = java_fs;
        __asm__( "mov %0,%%fs" :: "r" (0) );
        start_main_thread();
        __asm__( "mov %0,%%fs" :: "r" (java_fs) );
    }
#else
    start_main_thread();
#endif
    return (*env)->NewStringUTF( env, error );
}

jint JNI_OnLoad( JavaVM *vm, void *reserved )
{
    static const JNINativeMethod method =
    {
        "wine_init", "([Ljava/lang/String;[Ljava/lang/String;)Ljava/lang/String;", wine_init_jni
    };

    JNIEnv *env;
    jclass class;

    java_vm = vm;
    if ((*vm)->AttachCurrentThread( vm, &env, NULL ) != JNI_OK) return JNI_ERR;
    if (!(class = (*env)->FindClass( env, WINE_JAVA_CLASS ))) return JNI_ERR;
    (*env)->RegisterNatives( env, class, &method, 1 );
    return JNI_VERSION_1_6;
}

#endif  /* __ANDROID__ */

#ifdef __APPLE__
static void *apple_wine_thread( void *arg )
{
    start_main_thread();
    return NULL;
}

/***********************************************************************
 *           apple_create_wine_thread
 *
 * Spin off a secondary thread to complete Wine initialization, leaving
 * the original thread for the Mac frameworks.
 *
 * Invoked as a CFRunLoopSource perform callback.
 */
static void apple_create_wine_thread( void *arg )
{
    pthread_t thread;
    pthread_attr_t attr;

    pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
    /* Use the same QoS class as the process main thread (user-interactive). */
    if (&pthread_attr_set_qos_class_np)
        pthread_attr_set_qos_class_np( &attr, QOS_CLASS_USER_INTERACTIVE, 0 );
    if (pthread_create( &thread, &attr, apple_wine_thread, NULL )) exit(1);
    pthread_attr_destroy( &attr );
}


/***********************************************************************
 *           apple_main_thread
 *
 * Park the process's original thread in a Core Foundation run loop for
 * use by the Mac frameworks, especially receiving and handling
 * distributed notifications.  Spin off a new thread for the rest of the
 * Wine initialization.
 */
static void apple_main_thread(void)
{
    CFRunLoopSourceContext source_context = { 0 };
    CFRunLoopSourceRef source;

    if (!pthread_main_np()) return;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    /* Multi-processing Services can get confused about the main thread if the
     * first time it's used is on a secondary thread.  Use it here to make sure
     * that doesn't happen. */
    MPTaskIsPreemptive(MPCurrentTaskID());
#pragma clang diagnostic pop

    /* Give ourselves the best chance of having the distributed notification
     * center scheduled on this thread's run loop.  In theory, it's scheduled
     * in the first thread to ask for it. */
    CFNotificationCenterGetDistributedCenter();

    /* We use this run loop source for two purposes.  First, a run loop exits
     * if it has no more sources scheduled.  So, we need at least one source
     * to keep the run loop running.  Second, although it's not critical, it's
     * preferable for the Wine initialization to not proceed until we know
     * the run loop is running.  So, we signal our source immediately after
     * adding it and have its callback spin off the Wine thread. */
    source_context.perform = apple_create_wine_thread;
    source = CFRunLoopSourceCreate( NULL, 0, &source_context );
    CFRunLoopAddSource( CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes );
    CFRunLoopSourceSignal( source );
    CFRelease( source );
    CFRunLoopRun(); /* Should never return, except on error. */
}
#endif  /* __APPLE__ */


#if defined(__linux__) && !defined(__ANDROID__) && (defined(__i386__) || defined(__arm__))

static void check_vmsplit( void *stack )
{
    if (stack < (void *)0x80000000)
    {
        /* if the stack is below 0x80000000, assume we can safely try a munmap there */
        if (munmap( (void *)0x80000000, 1 ) == -1 && errno == EINVAL)
            ERR( "Warning: memory above 0x80000000 doesn't seem to be accessible.\n"
                 "Wine requires a 3G/1G user/kernel memory split to work properly.\n" );
    }
}

static int pre_exec(void)
{
    int temp;

    check_vmsplit( &temp );
    return 1;  /* we have a preloader on x86/arm */
}

#elif (defined(__FreeBSD__) || defined (__FreeBSD_kernel__) || defined(__DragonFly__))

static int pre_exec(void)
{
    struct rlimit rl;

    rl.rlim_cur = 0x02000000;
    rl.rlim_max = 0x02000000;
    setrlimit( RLIMIT_DATA, &rl );
    return 1;
}

#elif defined(__APPLE__)

static int pre_exec(void)
{
    if (build_dir)
    {
        char *path = getenv( "DYLD_LIBRARY_PATH" );
        if (path) asprintf( &path, "%s/dlls/ntdll:%s/dlls/win32u:%s", build_dir, build_dir, path );
        else asprintf( &path, "%s/dlls/ntdll:%s/dlls/win32u", build_dir, build_dir );
        setenv( "DYLD_LIBRARY_PATH", path, 1 );
        return 1;
    }
#ifdef HAVE_WINE_PRELOADER
    return 1;
#else
    return 0;
#endif
}

#else

static int pre_exec(void)
{
#ifdef HAVE_WINE_PRELOADER
    return 1;  /* we have a preloader */
#else
    return 0;  /* no exec needed */
#endif
}

#endif


static void reexec_loader( int argc, char *argv[], char *extra_arg )
{
    static char noexec[] = "WINELOADERNOEXEC=1";
    WORD machine = current_machine;
    char **new_argv;

    /* have to exec if we have a preloader, or an argument, or if we are the initial wrapper */
    if (!pre_exec() && !extra_arg && dlsym( RTLD_DEFAULT, "wine_main_preload_info" )) return;

    if (extra_arg)
    {
        new_argv = malloc( (argc + 3) * sizeof(*argv) );
        memcpy( new_argv + 3, argv + 1, argc * sizeof(*argv) );
        new_argv[2] = extra_arg;
    }
    else
    {
        new_argv = malloc( (argc + 2) * sizeof(*argv) );
        memcpy( new_argv + 2, argv + 1, argc * sizeof(*argv) );
    }

    /* default to 32-bit loader to support 32-bit prefixes */
    if (machine == IMAGE_FILE_MACHINE_AMD64) machine = IMAGE_FILE_MACHINE_I386;

    putenv( noexec );
    loader_exec( new_argv, machine );
    fatal_error( "could not exec the wine loader\n" );
}

/***********************************************************************
 *           check_command_line
 *
 * Check if command line is one that needs to be handled specially.
 */
static void check_command_line( int argc, char *argv[] )
{
    char *basename;
    static const char usage[] =
        "Usage: wine PROGRAM [ARGUMENTS...]   Run the specified program\n"
        "       wine --help                   Display this help and exit\n"
        "       wine --version                Output version information and exit";

    if ((basename = strrchr( argv[0], '/' ))) basename++;
    else basename = argv[0];

    if (strcmp( basename, "wine" )) /* check if there's a builtin exe corresponding to the base name */
    {
        const char *pe_dir = get_pe_dir( current_machine );
        char *exe;

        if (build_dir)
        {
            asprintf( &exe, "%s/programs/%s%s/%s.exe", build_dir, basename, pe_dir, basename );
            if (!access( exe, R_OK )) reexec_loader( argc, argv, basename );
            free( exe );
        }
        else
        {
            for (int i = 0; dll_paths[i]; i++)
            {
                asprintf( &exe, "%s%s/%s.exe", dll_paths[i], pe_dir, basename );
                if (!access( exe, R_OK )) reexec_loader( argc, argv, basename );
                free( exe );
            }
        }
    }

    if (argc <= 1)
    {
        fprintf( stderr, "%s\n", usage );
        exit(1);
    }
    if (!strcmp( argv[1], "--help" ))
    {
        printf( "%s\n", usage );
        exit(0);
    }
    if (!strcmp( argv[1], "--version" ))
    {
        printf( "%s\n", wine_build );
        exit(0);
    }

    reexec_loader( argc, argv, NULL );
}


/***********************************************************************
 *           __wine_main
 *
 * Main entry point called by the wine loader.
 */
DECLSPEC_EXPORT void __wine_main( int argc, char *argv[] )
{
    main_argc = argc;
    main_argv = argv;

    init_paths();
    if (!getenv( "WINELOADERNOEXEC" ) || argc <= 1) check_command_line( argc, argv );

#ifdef RLIMIT_NOFILE
    set_max_limit( RLIMIT_NOFILE );
#endif
#ifdef RLIMIT_AS
    set_max_limit( RLIMIT_AS );
#endif
#ifdef RLIMIT_NICE
    set_max_limit( RLIMIT_NICE );
#endif

    virtual_init();
    init_environment();

#ifdef __APPLE__
    apple_main_thread();
#endif
    start_main_thread();
}
