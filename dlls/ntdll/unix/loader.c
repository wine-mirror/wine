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
#include "wine/port.h"

#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <langinfo.h>
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#ifdef HAVE_PWD_H
# include <pwd.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#ifdef HAVE_SYS_PRCTL_H
# include <sys/prctl.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef __APPLE__
# include <CoreFoundation/CoreFoundation.h>
# define LoadResource MacLoadResource
# define GetCurrentThread MacGetCurrentThread
# include <CoreServices/CoreServices.h>
# undef LoadResource
# undef GetCurrentThread
# include <pthread.h>
# include <mach-o/getsect.h>
# include <crt_externs.h>
# include <spawn.h>
# ifndef _POSIX_SPAWN_DISABLE_ASLR
#  define _POSIX_SPAWN_DISABLE_ASLR 0x0100
# endif
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winnt.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "unix_private.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

extern IMAGE_NT_HEADERS __wine_spec_nt_header;
extern void CDECL __wine_set_unix_funcs( int version, const struct unix_funcs *funcs );

#ifdef __GNUC__
static void fatal_error( const char *err, ... ) __attribute__((noreturn, format(printf,1,2)));
#endif

extern int __wine_main_argc;
extern char **__wine_main_argv;
extern char **__wine_main_environ;

#if defined(linux) || defined(__APPLE__)
static const BOOL use_preloader = TRUE;
#else
static const BOOL use_preloader = FALSE;
#endif

static const BOOL is_win64 = (sizeof(void *) > sizeof(int));

static int main_argc;
static char **main_argv;
static char **main_envp;
static char *argv0;
static const char *bin_dir;
static const char *dll_dir;
static const char *data_dir;
static const char *build_dir;
static const char *config_dir;
static const char **dll_paths;
static SIZE_T dll_path_maxlen;

static CPTABLEINFO unix_table;

static inline void *get_rva( const IMAGE_NT_HEADERS *nt, ULONG_PTR addr )
{
    return (BYTE *)nt + addr;
}

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
        void *ptr = root + entry->u2.s2.OffsetToDirectory;
        if (entry->u2.s2.DataIsDirectory) fixup_so_resources( ptr, root, delta );
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
        setrlimit( limit, &rlimit );
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

    memcpy( ret, dir, len );
    if (len && ret[len - 1] != '/') ret[len++] = '/';
    strcpy( ret + len, name );
    return ret;
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
        const char *home = getenv( "HOME" );
        if (!home)
        {
            struct passwd *pwd = getpwuid( getuid() );
            if (pwd) home = pwd->pw_dir;
        }
        if (!home) fatal_error( "could not determine your home directory\n" );
        if (home[0] != '/') fatal_error( "your home directory %s is not an absolute path\n", home );
        config_dir = build_path( home, ".wine" );
    }
}

static void init_paths( int argc, char *argv[], char *envp[] )
{
    Dl_info info;

    __wine_main_argc = main_argc = argc;
    __wine_main_argv = main_argv = argv;
    __wine_main_environ = main_envp = envp;
    argv0 = strdup( argv[0] );

    if (!dladdr( init_paths, &info ) || !(dll_dir = realpath_dirname( info.dli_fname )))
        fatal_error( "cannot get path to ntdll.so\n" );

    if (!(build_dir = remove_tail( dll_dir, "/dlls/ntdll" )))
    {
#if defined(__linux__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__)
        bin_dir = realpath_dirname( "/proc/self/exe" );
#elif defined (__FreeBSD__) || defined(__DragonFly__)
        bin_dir = realpath_dirname( "/proc/curproc/file" );
#else
        bin_dir = realpath_dirname( argv0 );
#endif
        if (!bin_dir) bin_dir = build_path( dll_dir, DLL_TO_BINDIR );
        data_dir = build_path( bin_dir, BIN_TO_DATADIR );
    }

    set_dll_path();
    set_config_dir();
}

#if !defined(__APPLE__) && !defined(__ANDROID__)  /* these platforms always use UTF-8 */

/* charset to codepage map, sorted by name */
static const struct { const char *name; UINT cp; } charset_names[] =
{
    { "ANSIX341968", 20127 },
    { "BIG5", 950 },
    { "BIG5HKSCS", 950 },
    { "CP1250", 1250 },
    { "CP1251", 1251 },
    { "CP1252", 1252 },
    { "CP1253", 1253 },
    { "CP1254", 1254 },
    { "CP1255", 1255 },
    { "CP1256", 1256 },
    { "CP1257", 1257 },
    { "CP1258", 1258 },
    { "CP932", 932 },
    { "CP936", 936 },
    { "CP949", 949 },
    { "CP950", 950 },
    { "EUCJP", 20932 },
    { "EUCKR", 949 },
    { "GB18030", 936  /* 54936 */ },
    { "GB2312", 936 },
    { "GBK", 936 },
    { "IBM037", 37 },
    { "IBM1026", 1026 },
    { "IBM424", 20424 },
    { "IBM437", 437 },
    { "IBM500", 500 },
    { "IBM850", 850 },
    { "IBM852", 852 },
    { "IBM855", 855 },
    { "IBM857", 857 },
    { "IBM860", 860 },
    { "IBM861", 861 },
    { "IBM862", 862 },
    { "IBM863", 863 },
    { "IBM864", 864 },
    { "IBM865", 865 },
    { "IBM866", 866 },
    { "IBM869", 869 },
    { "IBM874", 874 },
    { "IBM875", 875 },
    { "ISO88591", 28591 },
    { "ISO885913", 28603 },
    { "ISO885915", 28605 },
    { "ISO88592", 28592 },
    { "ISO88593", 28593 },
    { "ISO88594", 28594 },
    { "ISO88595", 28595 },
    { "ISO88596", 28596 },
    { "ISO88597", 28597 },
    { "ISO88598", 28598 },
    { "ISO88599", 28599 },
    { "KOI8R", 20866 },
    { "KOI8U", 21866 },
    { "TIS620", 28601 },
    { "UTF8", CP_UTF8 }
};

static void load_unix_cptable( unsigned int cp )
{
    const char *dir = build_dir ? build_dir : data_dir;
    struct stat st;
    char *name;
    void *data;
    int fd;

    if (!(name = malloc( strlen(dir) + 22 ))) return;
    sprintf( name, "%s/nls/c_%03u.nls", dir, cp );
    if ((fd = open( name, O_RDONLY )) != -1)
    {
        fstat( fd, &st );
        if ((data = malloc( st.st_size )) && st.st_size > 0x10000 &&
            read( fd, data, st.st_size ) == st.st_size)
        {
            RtlInitCodePageTable( data, &unix_table );
        }
        else
        {
            free( data );
        }
        close( fd );
    }
    else ERR( "failed to load %s\n", name );
    free( name );
}

static void init_unix_codepage(void)
{
    char charset_name[16];
    const char *name;
    size_t i, j;
    int min = 0, max = ARRAY_SIZE(charset_names) - 1;

    setlocale( LC_CTYPE, "" );
    if (!(name = nl_langinfo( CODESET ))) return;

    /* remove punctuation characters from charset name */
    for (i = j = 0; name[i] && j < sizeof(charset_name)-1; i++)
    {
        if (name[i] >= '0' && name[i] <= '9') charset_name[j++] = name[i];
        else if (name[i] >= 'A' && name[i] <= 'Z') charset_name[j++] = name[i];
        else if (name[i] >= 'a' && name[i] <= 'z') charset_name[j++] = name[i] + ('A' - 'a');
    }
    charset_name[j] = 0;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        int res = strcmp( charset_names[pos].name, charset_name );
        if (!res)
        {
            if (charset_names[pos].cp != CP_UTF8) load_unix_cptable( charset_names[pos].cp );
            return;
        }
        if (res > 0) max = pos - 1;
        else min = pos + 1;
    }
    ERR( "unrecognized charset '%s'\n", name );
}

#else  /* __APPLE__ || __ANDROID__ */

static void init_unix_codepage(void) { }

#endif  /* __APPLE__ || __ANDROID__ */


/*********************************************************************
 *                  get_version
 */
const char * CDECL get_version(void)
{
    return PACKAGE_VERSION;
}


/*********************************************************************
 *                  get_build_id
 */
const char * CDECL get_build_id(void)
{
    extern const char wine_build[];
    return wine_build;
}


/*********************************************************************
 *                  get_host_version
 */
void CDECL get_host_version( const char **sysname, const char **release )
{
#ifdef HAVE_SYS_UTSNAME_H
    static struct utsname buf;
    static BOOL init_done;

    if (!init_done)
    {
        uname( &buf );
        init_done = TRUE;
    }
    if (sysname) *sysname = buf.sysname;
    if (release) *release = buf.release;
#else
    if (sysname) *sysname = "";
    if (release) *release = "";
#endif
}


/*************************************************************************
 *		get_main_args
 *
 * Return the initial arguments.
 */
static void CDECL get_main_args( int *argc, char **argv[], char **envp[] )
{
    *argc = main_argc;
    *argv = main_argv;
    *envp = main_envp;
}


/*************************************************************************
 *		get_paths
 *
 * Return the various configuration paths.
 */
static void CDECL get_paths( const char **builddir, const char **datadir, const char **configdir )
{
    *builddir  = build_dir;
    *datadir   = data_dir;
    *configdir = config_dir;
}


/*************************************************************************
 *		get_dll_path
 *
 * Return the various configuration paths.
 */
static void CDECL get_dll_path( const char ***paths, SIZE_T *maxlen )
{
    *paths = dll_paths;
    *maxlen = dll_path_maxlen;
}


/*************************************************************************
 *		get_unix_codepage
 *
 * Return the Unix codepage data.
 */
static void CDECL get_unix_codepage( CPTABLEINFO *table )
{
    *table = unix_table;
}


static void preloader_exec( char **argv )
{
    if (use_preloader)
    {
        static const char *preloader = "wine-preloader";
        char *p;

        if (!(p = strrchr( argv[1], '/' ))) p = argv[1];
        else p++;

        if (strlen(p) > 2 && !strcmp( p + strlen(p) - 2, "64" )) preloader = "wine64-preloader";
        argv[0] = malloc( p - argv[1] + strlen(preloader) + 1 );
        memcpy( argv[0], argv[1], p - argv[1] );
        strcpy( argv[0] + (p - argv[1]), preloader );

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
    }
    execv( argv[1], argv + 1 );
}

static NTSTATUS loader_exec( const char *loader, char **argv, int is_child_64bit )
{
    char *p, *path;

    if (build_dir)
    {
        argv[1] = build_path( build_dir, is_child_64bit ? "loader/wine64" : "loader/wine" );
        preloader_exec( argv );
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    if ((p = strrchr( loader, '/' ))) loader = p + 1;

    argv[1] = build_path( bin_dir, loader );
    preloader_exec( argv );

    argv[1] = getenv( "WINELOADER" );
    if (argv[1]) preloader_exec( argv );

    if ((path = getenv( "PATH" )))
    {
        for (p = strtok( strdup( path ), ":" ); p; p = strtok( NULL, ":" ))
        {
            argv[1] = build_path( p, loader );
            preloader_exec( argv );
        }
    }

    argv[1] = build_path( BINDIR, loader );
    preloader_exec( argv );
    return STATUS_INVALID_IMAGE_FORMAT;
}


/***********************************************************************
 *           exec_wineloader
 *
 * argv[0] and argv[1] must be reserved for the preloader and loader respectively.
 */
static NTSTATUS CDECL exec_wineloader( char **argv, int socketfd, int is_child_64bit,
                                       ULONGLONG res_start, ULONGLONG res_end )
{
    const char *loader = argv0;
    const char *loader_env = getenv( "WINELOADER" );
    char preloader_reserve[64], socket_env[64];

    if (!is_win64 ^ !is_child_64bit)
    {
        /* remap WINELOADER to the alternate 32/64-bit version if necessary */
        if (loader_env)
        {
            int len = strlen( loader_env );
            char *env = malloc( sizeof("WINELOADER=") + len + 2 );

            if (!env) return STATUS_NO_MEMORY;
            strcpy( env, "WINELOADER=" );
            strcat( env, loader_env );
            if (is_child_64bit)
            {
                strcat( env, "64" );
            }
            else
            {
                len += sizeof("WINELOADER=") - 1;
                if (!strcmp( env + len - 2, "64" )) env[len - 2] = 0;
            }
            loader = env;
            putenv( env );
        }
        else loader = is_child_64bit ? "wine64" : "wine";
    }

    signal( SIGPIPE, SIG_DFL );

    sprintf( socket_env, "WINESERVERSOCKET=%u", socketfd );
    sprintf( preloader_reserve, "WINEPRELOADRESERVE=%x%08x-%x%08x",
             (ULONG)(res_start >> 32), (ULONG)res_start, (ULONG)(res_end >> 32), (ULONG)res_end );

    putenv( preloader_reserve );
    putenv( socket_env );

    return loader_exec( loader, argv, is_child_64bit );
}


/***********************************************************************
 *           exec_wineserver
 *
 * Exec a new wine server.
 */
static void exec_wineserver( char **argv )
{
    char *path;

    if (build_dir)
    {
        if (!is_win64)  /* look for 64-bit server */
        {
            char *loader = realpath_dirname( build_path( build_dir, "loader/wine64" ));
            if (loader)
            {
                argv[0] = build_path( loader, "../server/wineserver" );
                execv( argv[0], argv );
            }
        }
        argv[0] = build_path( build_dir, "server/wineserver" );
        execv( argv[0], argv );
        return;
    }

    argv[0] = build_path( bin_dir, "wineserver" );
    execv( argv[0], argv );

    argv[0] = getenv( "WINESERVER" );
    if (argv[0]) execv( argv[0], argv );

    if ((path = getenv( "PATH" )))
    {
        for (path = strtok( strdup( path ), ":" ); path; path = strtok( NULL, ":" ))
        {
            argv[0] = build_path( path, "wineserver" );
            execvp( argv[0], argv );
        }
    }

    argv[0] = build_path( BINDIR, "wineserver" );
    execv( argv[0], argv );
}


/***********************************************************************
 *           start_server
 *
 * Start a new wine server.
 */
static void CDECL start_server( BOOL debug )
{
    static BOOL started;  /* we only try once */
    char *argv[3];
    static char debug_flag[] = "-d";

    if (!started)
    {
        int status;
        int pid = fork();
        if (pid == -1) fatal_error( "fork: %s", strerror(errno) );
        if (!pid)
        {
            argv[1] = debug ? debug_flag : NULL;
            argv[2] = NULL;
            exec_wineserver( argv );
            fatal_error( "could not exec wineserver\n" );
        }
        waitpid( pid, &status, 0 );
        status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        if (status == 2) return;  /* server lock held by someone else, will retry later */
        if (status) exit(status);  /* server failed */
        started = TRUE;
    }
}


/*************************************************************************
 *		map_so_dll
 *
 * Map a builtin dll in memory and fixup RVAs.
 */
static NTSTATUS CDECL map_so_dll( const IMAGE_NT_HEADERS *nt_descr, HMODULE module )
{
    static const char builtin_signature[32] = "Wine builtin DLL";
    IMAGE_DATA_DIRECTORY *dir;
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sec;
    BYTE *addr = (BYTE *)module;
    DWORD code_start, code_end, data_start, data_end, align_mask;
    int delta, nb_sections = 2;  /* code + data */
    unsigned int i;
    DWORD size = (sizeof(IMAGE_DOS_HEADER)
                  + sizeof(builtin_signature)
                  + sizeof(IMAGE_NT_HEADERS)
                  + nb_sections * sizeof(IMAGE_SECTION_HEADER));

    if (wine_anon_mmap( addr, size, PROT_READ | PROT_WRITE, MAP_FIXED ) != addr) return STATUS_NO_MEMORY;

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

    *nt = *nt_descr;

    delta      = (const BYTE *)nt_descr - addr;
    align_mask = nt->OptionalHeader.SectionAlignment - 1;
    code_start = (size + align_mask) & ~align_mask;
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
            fixup_rva_dwords( &imports->u.OriginalFirstThunk, delta, 1 );
            fixup_rva_dwords( &imports->Name, delta, 1 );
            fixup_rva_dwords( &imports->FirstThunk, delta, 1 );
            if (imports->u.OriginalFirstThunk)
                fixup_rva_names( (UINT_PTR *)(addr + imports->u.OriginalFirstThunk), delta );
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
    return STATUS_SUCCESS;
}

static const IMAGE_EXPORT_DIRECTORY *get_export_dir( HMODULE module )
{
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)module;
    const IMAGE_NT_HEADERS *nt;
    DWORD addr;

    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return NULL;
    nt = (IMAGE_NT_HEADERS *)((const BYTE *)dos + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return NULL;
    addr = nt->OptionalHeader.DataDirectory[IMAGE_FILE_EXPORT_DIRECTORY].VirtualAddress;
    if (!addr) return NULL;
    return (IMAGE_EXPORT_DIRECTORY *)((BYTE *)module + addr);
}

static ULONG_PTR find_ordinal_export( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports, DWORD ordinal )
{
    const DWORD *functions = (const DWORD *)((BYTE *)module + exports->AddressOfFunctions);

    if (ordinal >= exports->NumberOfFunctions) return 0;
    if (!functions[ordinal]) return 0;
    return (ULONG_PTR)module + functions[ordinal];
}

static ULONG_PTR find_named_export( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                                    const IMAGE_IMPORT_BY_NAME *name )
{
    const WORD *ordinals = (const WORD *)((BYTE *)module + exports->AddressOfNameOrdinals);
    const DWORD *names = (const DWORD *)((BYTE *)module + exports->AddressOfNames);
    int min = 0, max = exports->NumberOfNames - 1;

    /* first check the hint */
    if (name->Hint <= max)
    {
        char *ename = (char *)module + names[name->Hint];
        if (!strcmp( ename, (char *)name->Name ))
            return find_ordinal_export( module, exports, ordinals[name->Hint] );
    }

    /* then do a binary search */
    while (min <= max)
    {
        int res, pos = (min + max) / 2;
        char *ename = (char *)module + names[pos];
        if (!(res = strcmp( ename, (char *)name->Name )))
            return find_ordinal_export( module, exports, ordinals[pos] );
        if (res > 0) max = pos - 1;
        else min = pos + 1;
    }
    return 0;
}

static void fixup_ntdll_imports( const IMAGE_NT_HEADERS *nt, HMODULE ntdll_module )
{
    const IMAGE_EXPORT_DIRECTORY *ntdll_exports = get_export_dir( ntdll_module );
    const IMAGE_IMPORT_DESCRIPTOR *descr;
    const IMAGE_THUNK_DATA *import_list;
    IMAGE_THUNK_DATA *thunk_list;

    assert( ntdll_exports );

    descr = get_rva( nt, nt->OptionalHeader.DataDirectory[IMAGE_FILE_IMPORT_DIRECTORY].VirtualAddress );
    while (descr->Name)
    {
        /* ntdll must be the only import */
        assert( !strcmp( get_rva( nt, descr->Name ), "ntdll.dll" ));

        thunk_list = get_rva( nt, (DWORD)descr->FirstThunk );
        if (descr->u.OriginalFirstThunk)
            import_list = get_rva( nt, (DWORD)descr->u.OriginalFirstThunk );
        else
            import_list = thunk_list;


        while (import_list->u1.Ordinal)
        {
            if (IMAGE_SNAP_BY_ORDINAL( import_list->u1.Ordinal ))
            {
                int ordinal = IMAGE_ORDINAL( import_list->u1.Ordinal ) - ntdll_exports->Base;
                thunk_list->u1.Function = find_ordinal_export( ntdll_module, ntdll_exports, ordinal );
                if (!thunk_list->u1.Function) ERR( "ordinal %u not found\n", ordinal );
            }
            else  /* import by name */
            {
                IMAGE_IMPORT_BY_NAME *pe_name = get_rva( nt, import_list->u1.AddressOfData );
                thunk_list->u1.Function = find_named_export( ntdll_module, ntdll_exports, pe_name );
                if (!thunk_list->u1.Function) ERR( "%s not found\n", pe_name->Name );
            }
            import_list++;
            thunk_list++;
        }

        descr++;
    }
}

/***********************************************************************
 *           load_ntdll
 */
static HMODULE load_ntdll(void)
{
    const IMAGE_NT_HEADERS *nt;
    HMODULE module;
    Dl_info info;
    char *name;
    void *handle;

    name = build_path( dll_dir, "ntdll.dll.so" );
    if (!dladdr( load_ntdll, &info )) fatal_error( "cannot get path to ntdll.so\n" );
    name = malloc( strlen(info.dli_fname) + 5 );
    strcpy( name, info.dli_fname );
    strcpy( name + strlen(info.dli_fname) - 3, ".dll.so" );
    if (!(handle = dlopen( name, RTLD_NOW ))) fatal_error( "failed to load %s: %s\n", name, dlerror() );
    if (!(nt = dlsym( handle, "__wine_spec_nt_header" )))
        fatal_error( "NT header not found in %s (too old?)\n", name );
    dll_dir = realpath_dirname( name );
    free( name );
    module = (HMODULE)((nt->OptionalHeader.ImageBase + 0xffff) & ~0xffff);
    map_so_dll( nt, module );
    return module;
}


/***********************************************************************
 *           unix_funcs
 */
static struct unix_funcs unix_funcs =
{
    get_main_args,
    get_paths,
    get_dll_path,
    get_unix_codepage,
    get_version,
    get_build_id,
    get_host_version,
    exec_wineloader,
    start_server,
    map_so_dll,
    mmap_add_reserved_area,
    mmap_remove_reserved_area,
    mmap_is_in_reserved_area,
    mmap_enum_reserved_areas,
    dbg_init,
    __wine_dbg_get_channel_flags,
    __wine_dbg_strdup,
    __wine_dbg_output,
    __wine_dbg_header,
};


#ifdef __APPLE__
struct apple_stack_info
{
    void *stack;
    size_t desired_size;
};

static void *apple_wine_thread( void *arg )
{
    __wine_set_unix_funcs( NTDLL_UNIXLIB_VERSION, &unix_funcs );
    return NULL;
}

/***********************************************************************
 *           apple_alloc_thread_stack
 *
 * Callback for mmap_enum_reserved_areas to allocate space for
 * the secondary thread's stack.
 */
#ifndef _WIN64
static int CDECL apple_alloc_thread_stack( void *base, size_t size, void *arg )
{
    struct apple_stack_info *info = arg;

    /* For mysterious reasons, putting the thread stack at the very top
     * of the address space causes subsequent execs to fail, even on the
     * child side of a fork.  Avoid the top 16MB. */
    char * const limit = (char*)0xff000000;
    if ((char *)base >= limit) return 0;
    if (size > limit - (char*)base)
        size = limit - (char*)base;
    if (size < info->desired_size) return 0;
    info->stack = wine_anon_mmap( (char *)base + size - info->desired_size,
                                  info->desired_size, PROT_READ|PROT_WRITE, MAP_FIXED );
    return (info->stack != (void *)-1);
}
#endif

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
    int success = 0;
    pthread_t thread;
    pthread_attr_t attr;

    if (!pthread_attr_init( &attr ))
    {
#ifndef _WIN64
        struct apple_stack_info info;

        /* Try to put the new thread's stack in the reserved area.  If this
         * fails, just let it go wherever.  It'll be a waste of space, but we
         * can go on. */
        if (!pthread_attr_getstacksize( &attr, &info.desired_size ) &&
            mmap_enum_reserved_areas( apple_alloc_thread_stack, &info, 1 ))
        {
            mmap_remove_reserved_area( info.stack, info.desired_size );
            pthread_attr_setstackaddr( &attr, (char*)info.stack + info.desired_size );
        }
#endif

        if (!pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE ) &&
            !pthread_create( &thread, &attr, apple_wine_thread, NULL ))
            success = 1;

        pthread_attr_destroy( &attr );
    }
    if (!success) exit(1);
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

    /* Multi-processing Services can get confused about the main thread if the
     * first time it's used is on a secondary thread.  Use it here to make sure
     * that doesn't happen. */
    MPTaskIsPreemptive(MPCurrentTaskID());

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


#ifdef __ANDROID__

static int pre_exec(void)
{
#if defined(__i386__) || defined(__x86_64__)
    return 1;  /* we have a preloader */
#else
    return 0;  /* no exec needed */
#endif
}

#elif defined(__linux__) && (defined(__i386__) || defined(__arm__))

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
#ifdef __i386__
    return 1;  /* we have a preloader on x86 */
#else
    return 0;
#endif
}

#elif defined(__linux__) && (defined(__x86_64__) || defined(__aarch64__))

static int pre_exec(void)
{
    return 1;  /* we have a preloader on x86-64/arm64 */
}

#elif defined(__APPLE__) && (defined(__i386__) || defined(__x86_64__))

static int pre_exec(void)
{
    return 1;  /* we have a preloader */
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

#else

static int pre_exec(void)
{
    return 0;  /* no exec needed */
}

#endif


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
 *           check_command_line
 *
 * Check if command line is one that needs to be handled specially.
 */
static void check_command_line( int argc, char *argv[] )
{
    static const char usage[] =
        "Usage: wine PROGRAM [ARGUMENTS...]   Run the specified program\n"
        "       wine --help                   Display this help and exit\n"
        "       wine --version                Output version information and exit";

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
        printf( "%s\n", get_build_id() );
        exit(0);
    }
}


/***********************************************************************
 *           __wine_main
 *
 * Main entry point called by the wine loader.
 */
void __wine_main( int argc, char *argv[], char *envp[] )
{
    HMODULE module;

    init_paths( argc, argv, envp );

    if (!getenv( "WINELOADERNOEXEC" ))  /* first time around */
    {
        static char noexec[] = "WINELOADERNOEXEC=1";

        putenv( noexec );
        check_command_line( argc, argv );
        if (pre_exec())
        {
            char **new_argv = malloc( (argc + 2) * sizeof(*argv) );
            memcpy( new_argv + 1, argv, (argc + 1) * sizeof(*argv) );
            loader_exec( argv0, new_argv, is_win64 );
            fatal_error( "could not exec the wine loader\n" );
        }
    }

#ifdef RLIMIT_NOFILE
    set_max_limit( RLIMIT_NOFILE );
#endif
#ifdef RLIMIT_AS
    set_max_limit( RLIMIT_AS );
#endif

    virtual_init();

    module = load_ntdll();
    fixup_ntdll_imports( &__wine_spec_nt_header, module );

    set_process_name( argc, argv );
    init_unix_codepage();

#ifdef __APPLE__
    apple_main_thread();
#endif
    __wine_set_unix_funcs( NTDLL_UNIXLIB_VERSION, &unix_funcs );
}


static int add_area( void *base, size_t size, void *arg )
{
    mmap_add_reserved_area( base, size );
    return 0;
}

/***********************************************************************
 *           __wine_init_unix_lib
 *
 * Lib entry point called by ntdll.dll.so if not yet initialized.
 */
NTSTATUS __cdecl __wine_init_unix_lib( HMODULE module, const void *ptr_in, void *ptr_out )
{
    const IMAGE_NT_HEADERS *nt = ptr_in;

#ifdef __APPLE__
    extern char **__wine_get_main_environment(void);
    char **envp = __wine_get_main_environment();
#else
    char **envp = __wine_main_environ;
#endif
    init_paths( __wine_main_argc, __wine_main_argv, envp );

    map_so_dll( nt, module );
    fixup_ntdll_imports( &__wine_spec_nt_header, module );
    set_process_name( __wine_main_argc, __wine_main_argv );
    init_unix_codepage();
    *(struct unix_funcs **)ptr_out = &unix_funcs;
    wine_mmap_enum_reserved_areas( add_area, NULL, 0 );
    return STATUS_SUCCESS;
}

BOOL WINAPI DECLSPEC_HIDDEN DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    if (reason == DLL_PROCESS_ATTACH) LdrDisableThreadCalloutsForDll( inst );
    return TRUE;
}
