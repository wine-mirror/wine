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
# include <spawn.h>
# ifndef _POSIX_SPAWN_DISABLE_ASLR
#  define _POSIX_SPAWN_DISABLE_ASLR 0x0100
# endif
#endif
#ifdef __ANDROID__
# include <jni.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winnt.h"
#include "winbase.h"
#include "winnls.h"
#include "winioctl.h"
#include "winternl.h"
#include "unix_private.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);

#ifdef __i386__
static const char so_dir[] = "/i386-unix";
#elif defined(__x86_64__)
static const char so_dir[] = "/x86_64-unix";
#elif defined(__arm__)
static const char so_dir[] = "/arm-unix";
#elif defined(__aarch64__)
static const char so_dir[] = "/aarch64-unix";
#else
static const char so_dir[] = "";
#endif

void     (WINAPI *pDbgUiRemoteBreakin)( void *arg ) = NULL;
NTSTATUS (WINAPI *pKiRaiseUserExceptionDispatcher)(void) = NULL;
NTSTATUS (WINAPI *pKiUserExceptionDispatcher)(EXCEPTION_RECORD*,CONTEXT*) = NULL;
void     (WINAPI *pKiUserApcDispatcher)(CONTEXT*,ULONG_PTR,ULONG_PTR,ULONG_PTR,PNTAPCFUNC) = NULL;
void     (WINAPI *pKiUserCallbackDispatcher)(ULONG,void*,ULONG) = NULL;
void     (WINAPI *pLdrInitializeThunk)(CONTEXT*,void**,ULONG_PTR,ULONG_PTR) = NULL;
void     (WINAPI *pRtlUserThreadStart)( PRTL_THREAD_START_ROUTINE entry, void *arg ) = NULL;
void     (WINAPI *p__wine_ctrl_routine)(void*);
SYSTEM_DLL_INIT_BLOCK *pLdrSystemDllInitBlock = NULL;

static NTSTATUS (CDECL *p__wine_set_unix_funcs)( int version, const struct unix_funcs *funcs );
static void *p__wine_syscall_dispatcher;

static void * const syscalls[] =
{
    NtAcceptConnectPort,
    NtAccessCheck,
    NtAccessCheckAndAuditAlarm,
    NtAddAtom,
    NtAdjustGroupsToken,
    NtAdjustPrivilegesToken,
    NtAlertResumeThread,
    NtAlertThread,
    NtAlertThreadByThreadId,
    NtAllocateLocallyUniqueId,
    NtAllocateUuids,
    NtAllocateVirtualMemory,
    NtAllocateVirtualMemoryEx,
    NtAreMappedFilesTheSame,
    NtAssignProcessToJobObject,
    NtCallbackReturn,
    NtCancelIoFile,
    NtCancelIoFileEx,
    NtCancelSynchronousIoFile,
    NtCancelTimer,
    NtClearEvent,
    NtClose,
    NtCompareObjects,
    NtCompleteConnectPort,
    NtConnectPort,
    NtContinue,
    NtCreateDebugObject,
    NtCreateDirectoryObject,
    NtCreateEvent,
    NtCreateFile,
    NtCreateIoCompletion,
    NtCreateJobObject,
    NtCreateKey,
    NtCreateKeyTransacted,
    NtCreateKeyedEvent,
    NtCreateLowBoxToken,
    NtCreateMailslotFile,
    NtCreateMutant,
    NtCreateNamedPipeFile,
    NtCreatePagingFile,
    NtCreatePort,
    NtCreateSection,
    NtCreateSemaphore,
    NtCreateSymbolicLinkObject,
    NtCreateThread,
    NtCreateThreadEx,
    NtCreateTimer,
    NtCreateUserProcess,
    NtDebugActiveProcess,
    NtDebugContinue,
    NtDelayExecution,
    NtDeleteAtom,
    NtDeleteFile,
    NtDeleteKey,
    NtDeleteValueKey,
    NtDeviceIoControlFile,
    NtDisplayString,
    NtDuplicateObject,
    NtDuplicateToken,
    NtEnumerateKey,
    NtEnumerateValueKey,
    NtFilterToken,
    NtFindAtom,
    NtFlushBuffersFile,
    NtFlushInstructionCache,
    NtFlushKey,
    NtFlushProcessWriteBuffers,
    NtFlushVirtualMemory,
    NtFreeVirtualMemory,
    NtFsControlFile,
    NtGetContextThread,
    NtGetCurrentProcessorNumber,
    NtGetNextThread,
    NtGetNlsSectionPtr,
    NtGetWriteWatch,
    NtImpersonateAnonymousToken,
    NtInitializeNlsFiles,
    NtInitiatePowerAction,
    NtIsProcessInJob,
    NtListenPort,
    NtLoadDriver,
    NtLoadKey,
    NtLoadKey2,
    NtLoadKeyEx,
    NtLockFile,
    NtLockVirtualMemory,
    NtMakeTemporaryObject,
    NtMapViewOfSection,
    NtMapViewOfSectionEx,
    NtNotifyChangeDirectoryFile,
    NtNotifyChangeKey,
    NtNotifyChangeMultipleKeys,
    NtOpenDirectoryObject,
    NtOpenEvent,
    NtOpenFile,
    NtOpenIoCompletion,
    NtOpenJobObject,
    NtOpenKey,
    NtOpenKeyEx,
    NtOpenKeyTransacted,
    NtOpenKeyTransactedEx,
    NtOpenKeyedEvent,
    NtOpenMutant,
    NtOpenProcess,
    NtOpenProcessToken,
    NtOpenProcessTokenEx,
    NtOpenSection,
    NtOpenSemaphore,
    NtOpenSymbolicLinkObject ,
    NtOpenThread,
    NtOpenThreadToken,
    NtOpenThreadTokenEx,
    NtOpenTimer,
    NtPowerInformation,
    NtPrivilegeCheck,
    NtProtectVirtualMemory,
    NtPulseEvent,
    NtQueryAttributesFile,
    NtQueryDefaultLocale,
    NtQueryDefaultUILanguage,
    NtQueryDirectoryFile,
    NtQueryDirectoryObject,
    NtQueryEaFile,
    NtQueryEvent,
    NtQueryFullAttributesFile,
    NtQueryInformationAtom,
    NtQueryInformationFile,
    NtQueryInformationJobObject,
    NtQueryInformationProcess,
    NtQueryInformationThread,
    NtQueryInformationToken,
    NtQueryInstallUILanguage,
    NtQueryIoCompletion,
    NtQueryKey,
    NtQueryLicenseValue,
    NtQueryMultipleValueKey,
    NtQueryMutant,
    NtQueryObject,
    NtQueryPerformanceCounter,
    NtQuerySection,
    NtQuerySecurityObject,
    NtQuerySemaphore ,
    NtQuerySymbolicLinkObject,
    NtQuerySystemEnvironmentValue,
    NtQuerySystemEnvironmentValueEx,
    NtQuerySystemInformation,
    NtQuerySystemInformationEx,
    NtQuerySystemTime,
    NtQueryTimer,
    NtQueryTimerResolution,
    NtQueryValueKey,
    NtQueryVirtualMemory,
    NtQueryVolumeInformationFile,
    NtQueueApcThread,
    NtRaiseException,
    NtRaiseHardError,
    NtReadFile,
    NtReadFileScatter,
    NtReadVirtualMemory,
    NtRegisterThreadTerminatePort,
    NtReleaseKeyedEvent,
    NtReleaseMutant,
    NtReleaseSemaphore,
    NtRemoveIoCompletion,
    NtRemoveIoCompletionEx,
    NtRemoveProcessDebug,
    NtRenameKey,
    NtReplaceKey,
    NtReplyWaitReceivePort,
    NtRequestWaitReplyPort,
    NtResetEvent,
    NtResetWriteWatch,
    NtRestoreKey,
    NtResumeProcess,
    NtResumeThread,
    NtSaveKey,
    NtSecureConnectPort,
    NtSetContextThread,
    NtSetDebugFilterState,
    NtSetDefaultLocale,
    NtSetDefaultUILanguage,
    NtSetEaFile,
    NtSetEvent,
    NtSetInformationDebugObject,
    NtSetInformationFile,
    NtSetInformationJobObject,
    NtSetInformationKey,
    NtSetInformationObject,
    NtSetInformationProcess,
    NtSetInformationThread,
    NtSetInformationToken,
    NtSetInformationVirtualMemory,
    NtSetIntervalProfile,
    NtSetIoCompletion,
    NtSetLdtEntries,
    NtSetSecurityObject,
    NtSetSystemInformation,
    NtSetSystemTime,
    NtSetThreadExecutionState,
    NtSetTimer,
    NtSetTimerResolution,
    NtSetValueKey,
    NtSetVolumeInformationFile,
    NtShutdownSystem,
    NtSignalAndWaitForSingleObject,
    NtSuspendProcess,
    NtSuspendThread,
    NtSystemDebugControl,
    NtTerminateJobObject,
    NtTerminateProcess,
    NtTerminateThread,
    NtTestAlert,
    NtTraceControl,
    NtUnloadDriver,
    NtUnloadKey,
    NtUnlockFile,
    NtUnlockVirtualMemory,
    NtUnmapViewOfSection,
    NtUnmapViewOfSectionEx,
    NtWaitForAlertByThreadId,
    NtWaitForDebugEvent,
    NtWaitForKeyedEvent,
    NtWaitForMultipleObjects,
    NtWaitForSingleObject,
#ifndef _WIN64
    NtWow64AllocateVirtualMemory64,
    NtWow64GetNativeSystemInformation,
    NtWow64ReadVirtualMemory64,
    NtWow64WriteVirtualMemory64,
#endif
    NtWriteFile,
    NtWriteFileGather,
    NtWriteVirtualMemory,
    NtYieldExecution,
    __wine_dbg_write,
    __wine_unix_call,
    __wine_unix_spawnvp,
    wine_nt_to_unix_file_name,
    wine_server_call,
    wine_server_fd_to_handle,
    wine_server_handle_to_fd,
    wine_unix_to_nt_file_name,
};

static BYTE syscall_args[ARRAY_SIZE(syscalls)];

SYSTEM_SERVICE_TABLE KeServiceDescriptorTable[4];

#ifdef __GNUC__
static void fatal_error( const char *err, ... ) __attribute__((noreturn, format(printf,1,2)));
#endif

#if defined(linux) || defined(__APPLE__)
static const BOOL use_preloader = TRUE;
#else
static const BOOL use_preloader = FALSE;
#endif

static char *argv0;
static const char *bin_dir;
static const char *dll_dir;
static const char *ntdll_dir;
static SIZE_T dll_path_maxlen;
static int *p___wine_main_argc;
static char ***p___wine_main_argv;
static WCHAR ***p___wine_main_wargv;

const char *home_dir = NULL;
const char *data_dir = NULL;
const char *build_dir = NULL;
const char *config_dir = NULL;
const char **dll_paths = NULL;
const char **system_dll_paths = NULL;
const char *user_name = NULL;
SECTION_IMAGE_INFORMATION main_image_info = { NULL };
static HMODULE ntdll_module;
static const IMAGE_EXPORT_DIRECTORY *ntdll_exports;

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
        if (setrlimit( limit, &rlimit ) != 0)
        {
#if defined(__APPLE__) && defined(RLIMIT_NOFILE) && defined(OPEN_MAX)
            /* On Leopard, setrlimit(RLIMIT_NOFILE, ...) fails on attempts to set
             * rlim_cur above OPEN_MAX (even if rlim_max > OPEN_MAX). */
            if (limit == RLIMIT_NOFILE && rlimit.rlim_cur > OPEN_MAX)
            {
                rlimit.rlim_cur = OPEN_MAX;
                setrlimit( limit, &rlimit );
            }
#endif
        }
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
    if (name[0] == '/') name++;
    strcpy( ret + len, name );
    return ret;
}


static const char *get_pe_dir( WORD machine )
{
    if (!machine) machine = current_machine;

    switch(machine)
    {
    case IMAGE_FILE_MACHINE_I386:  return "/i386-windows";
    case IMAGE_FILE_MACHINE_AMD64: return "/x86_64-windows";
    case IMAGE_FILE_MACHINE_ARMNT: return "/arm-windows";
    case IMAGE_FILE_MACHINE_ARM64: return "/aarch64-windows";
    default: return "";
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

static void init_paths( char *argv[] )
{
    Dl_info info;

    argv0 = strdup( argv[0] );

    if (!dladdr( init_paths, &info ) || !(ntdll_dir = realpath_dirname( info.dli_fname )))
        fatal_error( "cannot get path to ntdll.so\n" );

    if (!(build_dir = remove_tail( ntdll_dir, "/dlls/ntdll" )))
    {
        if (!(dll_dir = remove_tail( ntdll_dir, so_dir ))) dll_dir = ntdll_dir;
#if (defined(__linux__) && !defined(__ANDROID__)) || defined(__FreeBSD_kernel__) || defined(__NetBSD__)
        bin_dir = realpath_dirname( "/proc/self/exe" );
#elif defined (__FreeBSD__) || defined(__DragonFly__)
        {
            static int pathname[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
            size_t path_size = PATH_MAX;
            char *path = malloc( path_size );
            if (path && !sysctl( pathname, sizeof(pathname)/sizeof(pathname[0]), path, &path_size, NULL, 0 ))
                bin_dir = realpath_dirname( path );
            free( path );
        }
#else
        bin_dir = realpath_dirname( argv0 );
#endif
        if (!bin_dir) bin_dir = build_path( dll_dir, DLL_TO_BINDIR );
        data_dir = build_path( bin_dir, BIN_TO_DATADIR );
    }

    set_dll_path();
    set_system_dll_path();
    set_home_dir();
    set_config_dir();
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

static NTSTATUS loader_exec( const char *loader, char **argv, WORD machine )
{
    char *p, *path;

    if (build_dir)
    {
        argv[1] = build_path( build_dir, (machine == IMAGE_FILE_MACHINE_AMD64) ? "loader/wine64" : "loader/wine" );
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
NTSTATUS exec_wineloader( char **argv, int socketfd, const pe_image_info_t *pe_info )
{
    WORD machine = pe_info->machine;
    ULONGLONG res_start = pe_info->base;
    ULONGLONG res_end = pe_info->base + pe_info->map_size;
    const char *loader = argv0;
    const char *loader_env = getenv( "WINELOADER" );
    char preloader_reserve[64], socket_env[64];
    BOOL is_child_64bit;

    if (pe_info->image_flags & IMAGE_FLAGS_WineFakeDll) res_start = res_end = 0;
    if (pe_info->image_flags & IMAGE_FLAGS_ComPlusNativeReady) machine = native_machine;

    is_child_64bit = is_machine_64bit( machine );

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

    return loader_exec( loader, argv, machine );
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
void start_server( BOOL debug )
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
static NTSTATUS map_so_dll( const IMAGE_NT_HEADERS *nt_descr, HMODULE module )
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

    if (anon_mmap_fixed( addr, size, PROT_READ | PROT_WRITE, 0 ) != addr) return STATUS_NO_MEMORY;

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

static void load_ntdll_functions( HMODULE module )
{
    ntdll_exports = get_module_data_dir( module, IMAGE_FILE_EXPORT_DIRECTORY, NULL );
    assert( ntdll_exports );

#define GET_FUNC(name) \
    if (!(p##name = (void *)find_named_export( module, ntdll_exports, #name ))) \
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
    GET_FUNC( __wine_set_unix_funcs );
    GET_FUNC( __wine_syscall_dispatcher );
#ifdef __i386__
    {
        void **p__wine_ldt_copy;
        GET_FUNC( __wine_ldt_copy );
        *p__wine_ldt_copy = &__wine_ldt_copy;
    }
#endif
#undef GET_FUNC
}

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

    /* also set the 32-bit LdrSystemDllInitBlock */
    memcpy( (void *)(ULONG_PTR)pLdrSystemDllInitBlock->pLdrSystemDllInitBlock,
            pLdrSystemDllInitBlock, sizeof(*pLdrSystemDllInitBlock) );
}

/* reimplementation of LdrProcessRelocationBlock */
static const IMAGE_BASE_RELOCATION *process_relocation_block( void *module, const IMAGE_BASE_RELOCATION *rel,
                                                              INT_PTR delta )
{
    char *page = get_rva( module, rel->VirtualAddress );
    UINT count = (rel->SizeOfBlock - sizeof(*rel)) / sizeof(USHORT);
    USHORT *relocs = (USHORT *)(rel + 1);

    while (count--)
    {
        USHORT offset = *relocs & 0xfff;
        switch (*relocs >> 12)
        {
        case IMAGE_REL_BASED_ABSOLUTE:
            break;
        case IMAGE_REL_BASED_HIGH:
            *(short *)(page + offset) += HIWORD(delta);
            break;
        case IMAGE_REL_BASED_LOW:
            *(short *)(page + offset) += LOWORD(delta);
            break;
        case IMAGE_REL_BASED_HIGHLOW:
            *(int *)(page + offset) += delta;
            break;
        case IMAGE_REL_BASED_DIR64:
            *(INT64 *)(page + offset) += delta;
            break;
        case IMAGE_REL_BASED_THUMB_MOV32:
        {
            DWORD *inst = (DWORD *)(page + offset);
            WORD lo = ((inst[0] << 1) & 0x0800) + ((inst[0] << 12) & 0xf000) +
                      ((inst[0] >> 20) & 0x0700) + ((inst[0] >> 16) & 0x00ff);
            WORD hi = ((inst[1] << 1) & 0x0800) + ((inst[1] << 12) & 0xf000) +
                      ((inst[1] >> 20) & 0x0700) + ((inst[1] >> 16) & 0x00ff);
            DWORD imm = MAKELONG( lo, hi ) + delta;

            lo = LOWORD( imm );
            hi = HIWORD( imm );
            inst[0] = (inst[0] & 0x8f00fbf0) + ((lo >> 1) & 0x0400) + ((lo >> 12) & 0x000f) +
                                               ((lo << 20) & 0x70000000) + ((lo << 16) & 0xff0000);
            inst[1] = (inst[1] & 0x8f00fbf0) + ((hi >> 1) & 0x0400) + ((hi >> 12) & 0x000f) +
                                               ((hi << 20) & 0x70000000) + ((hi << 16) & 0xff0000);
            break;
        }
        default:
            FIXME("Unknown/unsupported relocation %x\n", *relocs);
            return NULL;
        }
        relocs++;
    }
    return (IMAGE_BASE_RELOCATION *)relocs;  /* return address of next block */
}

static void relocate_ntdll( void *module )
{
    const IMAGE_NT_HEADERS *nt = get_rva( module, ((IMAGE_DOS_HEADER *)module)->e_lfanew );
    const IMAGE_BASE_RELOCATION *rel, *end;
    const IMAGE_SECTION_HEADER *sec;
    ULONG protect_old[96], i, size;
    INT_PTR delta;

    ERR( "ntdll could not be mapped at preferred address (%p), expect trouble\n", module );

    if (!(rel = get_module_data_dir( module, IMAGE_DIRECTORY_ENTRY_BASERELOC, &size ))) return;

    sec = (IMAGE_SECTION_HEADER *)((char *)&nt->OptionalHeader + nt->FileHeader.SizeOfOptionalHeader);
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++)
    {
        void *addr = get_rva( module, sec[i].VirtualAddress );
        SIZE_T size = sec[i].SizeOfRawData;
        NtProtectVirtualMemory( NtCurrentProcess(), &addr, &size, PAGE_READWRITE, &protect_old[i] );
    }

    end = (IMAGE_BASE_RELOCATION *)((const char *)rel + size);
    delta = (char *)module - (char *)nt->OptionalHeader.ImageBase;
    while (rel && rel < end - 1 && rel->SizeOfBlock) rel = process_relocation_block( module, rel, delta );

    for (i = 0; i < nt->FileHeader.NumberOfSections; i++)
    {
        void *addr = get_rva( module, sec[i].VirtualAddress );
        SIZE_T size = sec[i].SizeOfRawData;
        NtProtectVirtualMemory( NtCurrentProcess(), &addr, &size, protect_old[i], &protect_old[i] );
    }
}


static void *callback_module;

/***********************************************************************
 *           load_builtin_callback
 *
 * Load a library in memory; callback function for wine_dll_register
 */
static void load_builtin_callback( void *module, const char *filename )
{
    callback_module = module;
}


/***********************************************************************
 *           load_libwine
 */
static void load_libwine(void)
{
#ifdef __APPLE__
#define LIBWINE "libwine.1.dylib"
#else
#define LIBWINE "libwine.so.1"
#endif
    typedef void (*load_dll_callback_t)( void *, const char * );
    void (*p_wine_dll_set_callback)( load_dll_callback_t load );
    char ***p___wine_main_environ;

    char *path;
    void *handle;

    if (build_dir) path = build_path( build_dir, "libs/wine/" LIBWINE );
    else path = build_path( ntdll_dir, LIBWINE );

    handle = dlopen( path, RTLD_NOW );
    free( path );
    if (!handle && !(handle = dlopen( LIBWINE, RTLD_NOW ))) return;

    p_wine_dll_set_callback = dlsym( handle, "wine_dll_set_callback" );
    p___wine_main_argc      = dlsym( handle, "__wine_main_argc" );
    p___wine_main_argv      = dlsym( handle, "__wine_main_argv" );
    p___wine_main_wargv     = dlsym( handle, "__wine_main_wargv" );
    p___wine_main_environ   = dlsym( handle, "__wine_main_environ" );

    if (p_wine_dll_set_callback) p_wine_dll_set_callback( load_builtin_callback );
    if (p___wine_main_environ) *p___wine_main_environ = main_envp;
}


/***********************************************************************
 *           fill_builtin_image_info
 */
static void fill_builtin_image_info( void *module, pe_image_info_t *info )
{
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)module;
    const IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)((const BYTE *)dos + dos->e_lfanew);

    info->base            = nt->OptionalHeader.ImageBase;
    info->entry_point     = nt->OptionalHeader.AddressOfEntryPoint;
    info->map_size        = nt->OptionalHeader.SizeOfImage;
    info->stack_size      = nt->OptionalHeader.SizeOfStackReserve;
    info->stack_commit    = nt->OptionalHeader.SizeOfStackCommit;
    info->zerobits        = 0;
    info->subsystem       = nt->OptionalHeader.Subsystem;
    info->subsystem_minor = nt->OptionalHeader.MinorSubsystemVersion;
    info->subsystem_major = nt->OptionalHeader.MajorSubsystemVersion;
    info->osversion_major = nt->OptionalHeader.MajorOperatingSystemVersion;
    info->osversion_minor = nt->OptionalHeader.MinorOperatingSystemVersion;
    info->image_charact   = nt->FileHeader.Characteristics;
    info->dll_charact     = nt->OptionalHeader.DllCharacteristics;
    info->machine         = nt->FileHeader.Machine;
    info->contains_code   = TRUE;
    info->image_flags     = IMAGE_FLAGS_WineBuiltin;
    info->loader_flags    = 0;
    info->header_size     = nt->OptionalHeader.SizeOfHeaders;
    info->file_size       = nt->OptionalHeader.SizeOfImage;
    info->checksum        = nt->OptionalHeader.CheckSum;
    info->dbg_offset      = 0;
    info->dbg_size        = 0;
}


/***********************************************************************
 *           dlopen_dll
 */
static NTSTATUS dlopen_dll( const char *so_name, UNICODE_STRING *nt_name, void **ret_module,
                            pe_image_info_t *image_info, BOOL prefer_native )
{
    void *module, *handle;
    const IMAGE_NT_HEADERS *nt;

    callback_module = (void *)1;
    handle = dlopen( so_name, RTLD_NOW );
    if (!handle)
    {
        WARN( "failed to load .so lib %s: %s\n", debugstr_a(so_name), dlerror() );
        return STATUS_INVALID_IMAGE_FORMAT;
    }
    if (callback_module != (void *)1)  /* callback was called */
    {
        if (!callback_module) return STATUS_NO_MEMORY;
        WARN( "got old-style builtin library %s, constructors won't work\n", debugstr_a(so_name) );
        module = callback_module;
        if (get_builtin_so_handle( module )) goto already_loaded;
    }
    else if ((nt = dlsym( handle, "__wine_spec_nt_header" )))
    {
        module = (HMODULE)((nt->OptionalHeader.ImageBase + 0xffff) & ~0xffff);
        if (get_builtin_so_handle( module )) goto already_loaded;
        if (map_so_dll( nt, module ))
        {
            dlclose( handle );
            return STATUS_NO_MEMORY;
        }
    }
    else  /* already loaded .so */
    {
        WARN( "%s already loaded?\n", debugstr_a(so_name));
        return STATUS_INVALID_IMAGE_FORMAT;
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

already_loaded:
    fill_builtin_image_info( module, image_info );
    *ret_module = module;
    dlclose( handle );
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           ntdll_init_syscalls
 */
NTSTATUS ntdll_init_syscalls( ULONG id, SYSTEM_SERVICE_TABLE *table, void **dispatcher )
{
    struct syscall_info
    {
        void  *dispatcher;
        USHORT limit;
        BYTE  args[1];
    } *info = (struct syscall_info *)dispatcher;

    if (id > 3) return STATUS_INVALID_PARAMETER;
    if (info->limit != table->ServiceLimit)
    {
        ERR( "syscall count mismatch %u / %lu\n", info->limit, table->ServiceLimit );
        NtTerminateProcess( GetCurrentProcess(), STATUS_INVALID_PARAMETER );
    }
    info->dispatcher = __wine_syscall_dispatcher;
    memcpy( table->ArgumentTable, info->args, table->ServiceLimit );
    KeServiceDescriptorTable[id] = *table;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           __wine_unix_call
 */
NTSTATUS WINAPI __wine_unix_call( unixlib_handle_t handle, unsigned int code, void *args )
{
    return ((unixlib_entry_t*)(UINT_PTR)handle)[code]( args );
}


/***********************************************************************
 *           load_so_dll
 */
static NTSTATUS CDECL load_so_dll( UNICODE_STRING *nt_name, void **module )
{
    static const WCHAR soW[] = {'.','s','o',0};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING redir;
    pe_image_info_t info;
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

    status = dlopen_dll( unix_name, nt_name, module, &info, FALSE );
    free( unix_name );
    free( redir.Buffer );
    return status;
}


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
                                      ULONG_PTR zero_bits, WORD machine, BOOL prefer_native )
{
    NTSTATUS status;
    HANDLE mapping;

    *module = NULL;
    status = open_dll_file( name, attr, &mapping );
    if (!status)
    {
        status = virtual_map_builtin_module( mapping, module, size, image_info, zero_bits, machine, prefer_native );
        NtClose( mapping );
    }
    return status;
}


/***********************************************************************
 *           open_builtin_so_file
 */
static NTSTATUS open_builtin_so_file( const char *name, OBJECT_ATTRIBUTES *attr, void **module,
                                      SECTION_IMAGE_INFORMATION *image_info,
                                      WORD machine, BOOL prefer_native )
{
    NTSTATUS status;
    int fd;

    *module = NULL;
    if (machine != current_machine) return STATUS_DLL_NOT_FOUND;
    if ((fd = open( name, O_RDONLY )) == -1) return STATUS_DLL_NOT_FOUND;

    if (check_library_arch( fd ))
    {
        pe_image_info_t info;

        status = dlopen_dll( name, attr->ObjectName, module, &info, prefer_native );
        if (!status) virtual_fill_image_information( &info, image_info );
        else if (status != STATUS_IMAGE_ALREADY_LOADED)
        {
            ERR( "failed to load .so lib %s\n", debugstr_a(name) );
            status = STATUS_PROCEDURE_NOT_FOUND;
        }
    }
    else status = STATUS_IMAGE_MACHINE_TYPE_MISMATCH;

    close( fd );
    return status;
}


/***********************************************************************
 *           find_builtin_dll
 */
static NTSTATUS find_builtin_dll( UNICODE_STRING *nt_name, void **module, SIZE_T *size_ptr,
                                  SECTION_IMAGE_INFORMATION *image_info,
                                  ULONG_PTR zero_bits, WORD machine, BOOL prefer_native )
{
    unsigned int i, pos, namepos, namelen, maxlen = 0;
    unsigned int len = nt_name->Length / sizeof(WCHAR);
    char *ptr = NULL, *file, *ext = NULL;
    const char *pe_dir = get_pe_dir( machine );
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status = STATUS_DLL_NOT_FOUND;
    BOOL found_image = FALSE;

    for (i = namepos = 0; i < len; i++)
        if (nt_name->Buffer[i] == '/' || nt_name->Buffer[i] == '\\') namepos = i + 1;
    len -= namepos;
    if (!len) return STATUS_DLL_NOT_FOUND;
    InitializeObjectAttributes( &attr, nt_name, 0, 0, NULL );

    if (build_dir) maxlen = strlen(build_dir) + sizeof("/programs/") + len;
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
    file[--pos] = '/';

    if (build_dir)
    {
        /* try as a dll */
        ptr = file + pos;
        namelen = len + 1;
        file[pos + len + 1] = 0;
        if (ext && !strcmp( ext, ".dll" )) namelen -= 4;
        ptr = prepend( ptr, ptr, namelen );
        ptr = prepend( ptr, "/dlls", sizeof("/dlls") - 1 );
        ptr = prepend( ptr, build_dir, strlen(build_dir) );
        status = open_builtin_pe_file( ptr, &attr, module, size_ptr, image_info, zero_bits, machine, prefer_native );
        if (status != STATUS_DLL_NOT_FOUND) goto done;
        strcpy( file + pos + len + 1, ".so" );
        status = open_builtin_so_file( ptr, &attr, module, image_info, machine, prefer_native );
        if (status != STATUS_DLL_NOT_FOUND) goto done;

        /* now as a program */
        ptr = file + pos;
        namelen = len + 1;
        file[pos + len + 1] = 0;
        if (ext && !strcmp( ext, ".exe" )) namelen -= 4;
        ptr = prepend( ptr, ptr, namelen );
        ptr = prepend( ptr, "/programs", sizeof("/programs") - 1 );
        ptr = prepend( ptr, build_dir, strlen(build_dir) );
        status = open_builtin_pe_file( ptr, &attr, module, size_ptr, image_info, zero_bits, machine, prefer_native );
        if (status != STATUS_DLL_NOT_FOUND) goto done;
        strcpy( file + pos + len + 1, ".so" );
        status = open_builtin_so_file( ptr, &attr, module, image_info, machine, prefer_native );
        if (status != STATUS_DLL_NOT_FOUND) goto done;
    }

    for (i = 0; dll_paths[i]; i++)
    {
        ptr = file + pos;
        file[pos + len + 1] = 0;
        ptr = prepend( ptr, pe_dir, strlen(pe_dir) );
        ptr = prepend( ptr, dll_paths[i], strlen(dll_paths[i]) );
        status = open_builtin_pe_file( ptr, &attr, module, size_ptr, image_info, zero_bits, machine, prefer_native );
        /* use so dir for unix lib */
        ptr = file + pos;
        ptr = prepend( ptr, so_dir, strlen(so_dir) );
        ptr = prepend( ptr, dll_paths[i], strlen(dll_paths[i]) );
        if (status != STATUS_DLL_NOT_FOUND) goto done;
        strcpy( file + pos + len + 1, ".so" );
        status = open_builtin_so_file( ptr, &attr, module, image_info, machine, prefer_native );
        if (status != STATUS_DLL_NOT_FOUND) goto done;
        file[pos + len + 1] = 0;
        ptr = prepend( file + pos, dll_paths[i], strlen(dll_paths[i]) );
        status = open_builtin_pe_file( ptr, &attr, module, size_ptr, image_info, zero_bits, machine, prefer_native );
        if (status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH)
        {
            found_image = TRUE;
            continue;
        }
        if (status != STATUS_DLL_NOT_FOUND) goto done;
        strcpy( file + pos + len + 1, ".so" );
        status = open_builtin_so_file( ptr, &attr, module, image_info, machine, prefer_native );
        if (status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH) found_image = TRUE;
        else if (status != STATUS_DLL_NOT_FOUND) goto done;
    }

    if (found_image) status = STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
    WARN( "cannot find builtin library for %s\n", debugstr_us(nt_name) );
done:
    if (status >= 0 && ext)
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
NTSTATUS load_builtin( const pe_image_info_t *image_info, WCHAR *filename,
                       void **module, SIZE_T *size, ULONG_PTR zero_bits )
{
    WORD machine = image_info->machine;  /* request same machine as the native one */
    NTSTATUS status;
    UNICODE_STRING nt_name;
    SECTION_IMAGE_INFORMATION info;
    enum loadorder loadorder;

    init_unicode_string( &nt_name, filename );
    loadorder = get_load_order( &nt_name );

    if (loadorder == LO_DISABLED) return STATUS_DLL_NOT_FOUND;

    if (image_info->image_flags & IMAGE_FLAGS_WineBuiltin)
    {
        if (loadorder == LO_NATIVE) return STATUS_DLL_NOT_FOUND;
        loadorder = LO_BUILTIN_NATIVE;  /* load builtin, then fallback to the file we found */
    }
    else if (image_info->image_flags & IMAGE_FLAGS_WineFakeDll)
    {
        TRACE( "%s is a fake Wine dll\n", debugstr_w(filename) );
        if (loadorder == LO_NATIVE) return STATUS_DLL_NOT_FOUND;
        loadorder = LO_BUILTIN;  /* builtin with no fallback since mapping a fake dll is not useful */
    }

    switch (loadorder)
    {
    case LO_NATIVE:
    case LO_NATIVE_BUILTIN:
        return STATUS_IMAGE_ALREADY_LOADED;
    case LO_BUILTIN:
        return find_builtin_dll( &nt_name, module, size, &info, zero_bits, machine, FALSE );
    default:
        status = find_builtin_dll( &nt_name, module, size, &info, zero_bits, machine, (loadorder == LO_DEFAULT) );
        if (status == STATUS_DLL_NOT_FOUND || status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH)
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
    static const WCHAR sysx8664[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\','s','y','s','x','8','6','6','4','\\',0};
    static const WCHAR sysarm64[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\','s','y','s','a','r','m','6','4','\\',0};

    if (machine == native_machine) machine = IMAGE_FILE_MACHINE_TARGET_HOST;

    switch (machine)
    {
    case IMAGE_FILE_MACHINE_TARGET_HOST: return system32;
    case IMAGE_FILE_MACHINE_I386:        return syswow64;
    case IMAGE_FILE_MACHINE_ARMNT:       return sysarm32;
    case IMAGE_FILE_MACHINE_AMD64:       return sysx8664;
    case IMAGE_FILE_MACHINE_ARM64:       return sysarm64;
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
                                 enum loadorder loadorder )
{
    static const WCHAR soW[] = {'.','s','o',0};
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    pe_image_info_t pe_info;
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
        *module = NULL;
        status = NtMapViewOfSection( mapping, NtCurrentProcess(), module, 0, 0, NULL, &size,
                                     ViewShare, 0, PAGE_EXECUTE_READ );
        if (!status)
        {
            NtQuerySection( mapping, SectionImageInformation, info, sizeof(*info), NULL );
            if (info->u.s.ComPlusNativeReady) info->Machine = native_machine;
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
                        WCHAR **image, void **module )
{
    enum loadorder loadorder = LO_INVALID;
    UNICODE_STRING nt_name;
    WCHAR *tmp = NULL;
    BOOL contains_path;
    NTSTATUS status;
    SIZE_T size;
    struct stat st;
    WORD machine;

    /* special case for Unix file name */
    if (unix_name && unix_name[0] == '/' && !stat( unix_name, &st ))
    {
        if ((status = unix_to_nt_file_name( unix_name, image ))) goto failed;
        init_unicode_string( &nt_name, *image );
        loadorder = get_load_order( &nt_name );
        status = open_main_image( *image, module, &main_image_info, loadorder );
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

    status = open_main_image( *image, module, &main_image_info, loadorder );
    if (status != STATUS_DLL_NOT_FOUND) return status;

    /* if path is in system dir, we can load the builtin even if the file itself doesn't exist */
    if (loadorder != LO_NATIVE && is_builtin_path( &nt_name, &machine ))
    {
        status = find_builtin_dll( &nt_name, module, &size, &main_image_info, 0, machine, FALSE );
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
    NTSTATUS status;
    SIZE_T size;

    *image = malloc( sizeof("\\??\\C:\\windows\\system32\\start.exe") * sizeof(WCHAR) );
    wcscpy( *image, get_machine_wow64_dir( current_machine ));
    wcscat( *image, startW );
    init_unicode_string( &nt_name, *image );
    status = find_builtin_dll( &nt_name, module, &size, &main_image_info, 0, current_machine, FALSE );
    if (status)
    {
        MESSAGE( "wine: failed to load start.exe: %x\n", status );
        NtTerminateProcess( GetCurrentProcess(), status );
    }
    return status;
}


#ifdef __FreeBSD__
/* The PT_LOAD segments are sorted in increasing order, and the first
 * starts at the beginning of the ELF file. By parsing the file, we can
 * find that first PT_LOAD segment, from which we can find the base
 * address it wanted, and knowing mapbase where the binary was actually
 * loaded, use them to work out the relocbase offset. */
static BOOL get_relocbase(caddr_t mapbase, caddr_t *relocbase)
{
    Elf_Half i;
#ifdef _WIN64
    const Elf64_Ehdr *elf_header = (Elf64_Ehdr*) mapbase;
#else
    const Elf32_Ehdr *elf_header = (Elf32_Ehdr*) mapbase;
#endif
    const Elf_Phdr *prog_header = (const Elf_Phdr *)(mapbase + elf_header->e_phoff);

    for (i = 0; i < elf_header->e_phnum; i++)
    {
         if (prog_header->p_type == PT_LOAD)
         {
             caddr_t desired_base = (caddr_t)((prog_header->p_vaddr / prog_header->p_align) * prog_header->p_align);
             *relocbase = (caddr_t) (mapbase - desired_base);
             return TRUE;
         }
         prog_header++;
    }
    return FALSE;
}
#endif

/*************************************************************************
 *              init_builtin_dll
 */
static void CDECL init_builtin_dll( void *module )
{
#ifdef HAVE_DLINFO
    void *handle = NULL;
    struct link_map *map;
    void (*init_func)(int, char **, char **) = NULL;
    void (**init_array)(int, char **, char **) = NULL;
    ULONG_PTR i, init_arraysz = 0;
#ifdef _WIN64
    const Elf64_Dyn *dyn;
#else
    const Elf32_Dyn *dyn;
#endif

    if (!(handle = get_builtin_so_handle( module ))) return;
    if (dlinfo( handle, RTLD_DI_LINKMAP, &map )) map = NULL;
    release_builtin_module( module );
    if (!map) return;

    for (dyn = map->l_ld; dyn->d_tag; dyn++)
    {
        caddr_t relocbase = (caddr_t)map->l_addr;

#ifdef __FreeBSD__
        /* On older FreeBSD versions, l_addr was the absolute load address, now it's the relocation offset. */
        if (offsetof(struct link_map, l_addr) == 0)
            if (!get_relocbase(map->l_addr, &relocbase))
                return;
#endif
        switch (dyn->d_tag)
        {
        case 0x60009990: init_array = (void *)(relocbase + dyn->d_un.d_val); break;
        case 0x60009991: init_arraysz = dyn->d_un.d_val; break;
        case 0x60009992: init_func = (void *)(relocbase + dyn->d_un.d_val); break;
        }
    }

    TRACE( "%p: got init_func %p init_array %p %lu\n", module, init_func, init_array, init_arraysz );

    if (init_func) init_func( main_argc, main_argv, main_envp );

    if (init_array)
        for (i = 0; i < init_arraysz / sizeof(*init_array); i++)
            init_array[i]( main_argc, main_argv, main_envp );
#endif
}


/***********************************************************************
 *           load_ntdll
 */
static void load_ntdll(void)
{
    static WCHAR path[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\',
                           's','y','s','t','e','m','3','2','\\','n','t','d','l','l','.','d','l','l',0};
    const char *pe_dir = get_pe_dir( current_machine );
    NTSTATUS status;
    SECTION_IMAGE_INFORMATION info;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    void *module;
    SIZE_T size = 0;
    char *name;

    init_unicode_string( &str, path );
    InitializeObjectAttributes( &attr, &str, 0, 0, NULL );

    name = malloc( strlen( ntdll_dir ) + strlen( pe_dir ) + sizeof("/ntdll.dll.so") );
    if (build_dir) sprintf( name, "%s/ntdll.dll", ntdll_dir );
    else sprintf( name, "%s%s/ntdll.dll", dll_dir, pe_dir );
    status = open_builtin_pe_file( name, &attr, &module, &size, &info, 0, current_machine, FALSE );
    if (status == STATUS_DLL_NOT_FOUND)
    {
        sprintf( name, "%s/ntdll.dll.so", ntdll_dir );
        status = open_builtin_so_file( name, &attr, &module, &info, current_machine, FALSE );
    }
    if (status == STATUS_IMAGE_NOT_AT_BASE) relocate_ntdll( module );
    else if (status) fatal_error( "failed to load %s error %x\n", name, status );
    free( name );
    load_ntdll_functions( module );
    ntdll_module = module;
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
    NTSTATUS status;
    HANDLE handle, mapping;
    SIZE_T size;
    char *name;
    void *ptr;
    UINT i;

    init_unicode_string( &str, path );
    InitializeObjectAttributes( &attr, &str, 0, 0, NULL );

    name = malloc( strlen( ntdll_dir ) + strlen( pe_dir ) + sizeof("/apisetschema.dll") );
    if (build_dir) sprintf( name, "%s/dlls/apisetschema/apisetschema.dll", build_dir );
    else sprintf( name, "%s%s/apisetschema.dll", dll_dir, pe_dir );
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
        sec = (IMAGE_SECTION_HEADER *)((char *)&nt->OptionalHeader + nt->FileHeader.SizeOfOptionalHeader);

        for (i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
        {
            if (memcmp( (char *)sec->Name, ".apiset", 8 )) continue;
            map = (API_SET_NAMESPACE *)((char *)ptr + sec->PointerToRawData);
            if (sec->PointerToRawData < size &&
                size - sec->PointerToRawData >= sec->Misc.VirtualSize &&
                map->Version == 6 &&
                map->Size <= sec->Misc.VirtualSize)
            {
                NtCurrentTeb()->Peb->ApiSetMap = map;
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
    NTSTATUS status;
    SIZE_T size;
    WCHAR *path = malloc( sizeof("\\??\\C:\\windows\\system32\\ntdll.dll") * sizeof(WCHAR) );

    wcscpy( path, get_machine_wow64_dir( machine ));
    wcscat( path, ntdllW );
    init_unicode_string( &nt_name, path );
    status = find_builtin_dll( &nt_name, &module, &size, &info, 0, machine, FALSE );
    switch (status)
    {
    case STATUS_IMAGE_NOT_AT_BASE:
        relocate_ntdll( module );
        /* fall through */
    case STATUS_SUCCESS:
        load_ntdll_wow64_functions( module );
        TRACE("loaded %s at %p\n", debugstr_w(path), module );
        break;
    default:
        ERR( "failed to load %s error %x\n", debugstr_w(path), status );
        break;
    }
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
 *           unix_funcs
 */
static struct unix_funcs unix_funcs =
{
    load_so_dll,
    init_builtin_dll,
    unwind_builtin_dll,
    RtlGetSystemTimePrecise,
#ifdef __aarch64__
    NtCurrentTeb,
#endif
};


/***********************************************************************
 *           start_main_thread
 */
static void start_main_thread(void)
{
    SYSTEM_SERVICE_TABLE syscall_table = { (ULONG_PTR *)syscalls, NULL, ARRAY_SIZE(syscalls), syscall_args };
    NTSTATUS status;
    TEB *teb = virtual_alloc_first_teb();

    signal_init_threading();
    signal_alloc_thread( teb );
    signal_init_thread( teb );
    dbg_init();
    startup_info_size = server_init_process();
    virtual_map_user_shared_data();
    init_cpu_info();
    init_files();
    load_libwine();
    init_startup_info();
    if (p___wine_main_argc) *p___wine_main_argc = main_argc;
    if (p___wine_main_argv) *p___wine_main_argv = main_argv;
    if (p___wine_main_wargv) *p___wine_main_wargv = main_wargv;
    *(ULONG_PTR *)&peb->CloudFileFlags = get_image_address();
    set_load_order_app_name( main_wargv[0] );
    init_thread_stack( teb, 0, 0, 0 );
    NtCreateKeyedEvent( &keyed_event, GENERIC_READ | GENERIC_WRITE, NULL, 0 );
    load_ntdll();
    if (main_image_info.Machine != current_machine) load_wow64_ntdll( main_image_info.Machine );
    load_apiset_dll();
    ntdll_init_syscalls( 0, &syscall_table, p__wine_syscall_dispatcher );
    status = p__wine_set_unix_funcs( NTDLL_UNIXLIB_VERSION, &unix_funcs );
    if (status == STATUS_REVISION_MISMATCH)
    {
        ERR( "ntdll library version mismatch\n" );
        NtTerminateProcess( GetCurrentProcess(), status );
    }
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

    init_paths( argv );
    virtual_init();
    init_environment( argc, argv, environ );

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
    return 1;  /* we have a preloader on x86/arm */
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
        printf( "%s\n", wine_build );
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
    init_paths( argv );

    if (!getenv( "WINELOADERNOEXEC" ))  /* first time around */
    {
        check_command_line( argc, argv );
        if (pre_exec())
        {
            static char noexec[] = "WINELOADERNOEXEC=1";
            char **new_argv = malloc( (argc + 2) * sizeof(*argv) );

            memcpy( new_argv + 1, argv, (argc + 1) * sizeof(*argv) );
            putenv( noexec );
            loader_exec( argv0, new_argv, current_machine );
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
    init_environment( argc, argv, envp );

#ifdef __APPLE__
    apple_main_thread();
#endif
    start_main_thread();
}
