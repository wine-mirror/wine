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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wine/exception.h"
#include "wine/library.h"
#include "drive.h"
#include "module.h"
#include "file.h"
#include "heap.h"
#include "thread.h"
#include "winerror.h"
#include "wincon.h"
#include "wine/server.h"
#include "options.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(process);
WINE_DECLARE_DEBUG_CHANNEL(relay);
WINE_DECLARE_DEBUG_CHANNEL(snoop);
WINE_DECLARE_DEBUG_CHANNEL(win32);

struct _ENVDB;

/* Win32 process database */
typedef struct _PDB
{
    LONG             header[2];        /* 00 Kernel object header */
    HMODULE          module;           /* 08 Main exe module (NT) */
    void            *event;            /* 0c Pointer to an event object (unused) */
    DWORD            exit_code;        /* 10 Process exit code */
    DWORD            unknown2;         /* 14 Unknown */
    HANDLE           heap;             /* 18 Default process heap */
    HANDLE           mem_context;      /* 1c Process memory context */
    DWORD            flags;            /* 20 Flags */
    void            *pdb16;            /* 24 DOS PSP */
    WORD             PSP_sel;          /* 28 Selector to DOS PSP */
    WORD             imte;             /* 2a IMTE for the process module */
    WORD             threads;          /* 2c Number of threads */
    WORD             running_threads;  /* 2e Number of running threads */
    WORD             free_lib_count;   /* 30 Recursion depth of FreeLibrary calls */
    WORD             ring0_threads;    /* 32 Number of ring 0 threads */
    HANDLE           system_heap;      /* 34 System heap to allocate handles */
    HTASK            task;             /* 38 Win16 task */
    void            *mem_map_files;    /* 3c Pointer to mem-mapped files */
    struct _ENVDB   *env_db;           /* 40 Environment database */
    void            *handle_table;     /* 44 Handle table */
    struct _PDB     *parent;           /* 48 Parent process */
    void            *modref_list;      /* 4c MODREF list */
    void            *thread_list;      /* 50 List of threads */
    void            *debuggee_CB;      /* 54 Debuggee context block */
    void            *local_heap_free;  /* 58 Head of local heap free list */
    DWORD            unknown4;         /* 5c Unknown */
    CRITICAL_SECTION crit_section;     /* 60 Critical section */
    DWORD            unknown5[3];      /* 78 Unknown */
    void            *console;          /* 84 Console */
    DWORD            tls_bits[2];      /* 88 TLS in-use bits */
    DWORD            process_dword;    /* 90 Unknown */
    struct _PDB     *group;            /* 94 Process group */
    void            *exe_modref;       /* 98 MODREF for the process EXE */
    void            *top_filter;       /* 9c Top exception filter */
    DWORD            priority;         /* a0 Priority level */
    HANDLE           heap_list;        /* a4 Head of process heap list */
    void            *heap_handles;     /* a8 Head of heap handles list */
    DWORD            unknown6;         /* ac Unknown */
    void            *console_provider; /* b0 Console provider (??) */
    WORD             env_selector;     /* b4 Selector to process environment */
    WORD             error_mode;       /* b6 Error mode */
    HANDLE           load_done_evt;    /* b8 Event for process loading done */
    void            *UTState;          /* bc Head of Univeral Thunk list */
    DWORD            unknown8;         /* c0 Unknown (NT) */
    LCID             locale;           /* c4 Locale to be queried by GetThreadLocale (NT) */
} PDB;

PDB current_process;

/* Process flags */
#define PDB32_DEBUGGED      0x0001  /* Process is being debugged */
#define PDB32_WIN16_PROC    0x0008  /* Win16 process */
#define PDB32_DOS_PROC      0x0010  /* Dos process */
#define PDB32_CONSOLE_PROC  0x0020  /* Console process */
#define PDB32_FILE_APIS_OEM 0x0040  /* File APIs are OEM */
#define PDB32_WIN32S_PROC   0x8000  /* Win32s process */

static char main_exe_name[MAX_PATH];
static char *main_exe_name_ptr = main_exe_name;
static HANDLE main_exe_file;
static unsigned int server_startticks;

int main_create_flags = 0;

/* memory/environ.c */
extern struct _ENVDB *ENV_InitStartupInfo( size_t info_size, char *main_exe_name,
                                           size_t main_exe_size );
extern BOOL ENV_BuildCommandLine( char **argv );
extern STARTUPINFOA current_startupinfo;

/* scheduler/pthread.c */
extern void PTHREAD_init_done(void);

extern void RELAY_InitDebugLists(void);
extern BOOL MAIN_MainInit(void);

typedef WORD (WINAPI *pUserSignalProc)( UINT, DWORD, DWORD, HMODULE16 );

/***********************************************************************
 *           PROCESS_CallUserSignalProc
 *
 * FIXME:  Some of the signals aren't sent correctly!
 *
 * The exact meaning of the USER signals is undocumented, but this
 * should cover the basic idea:
 *
 * USIG_DLL_UNLOAD_WIN16
 *     This is sent when a 16-bit module is unloaded.
 *
 * USIG_DLL_UNLOAD_WIN32
 *     This is sent when a 32-bit module is unloaded.
 *
 * USIG_DLL_UNLOAD_ORPHANS
 *     This is sent after the last Win3.1 module is unloaded,
 *     to allow removal of orphaned menus.
 *
 * USIG_FAULT_DIALOG_PUSH
 * USIG_FAULT_DIALOG_POP
 *     These are called to allow USER to prepare for displaying a
 *     fault dialog, even though the fault might have happened while
 *     inside a USER critical section.
 *
 * USIG_THREAD_INIT
 *     This is called from the context of a new thread, as soon as it
 *     has started to run.
 *
 * USIG_THREAD_EXIT
 *     This is called, still in its context, just before a thread is
 *     about to terminate.
 *
 * USIG_PROCESS_CREATE
 *     This is called, in the parent process context, after a new process
 *     has been created.
 *
 * USIG_PROCESS_INIT
 *     This is called in the new process context, just after the main thread
 *     has started execution (after the main thread's USIG_THREAD_INIT has
 *     been sent).
 *
 * USIG_PROCESS_LOADED
 *     This is called after the executable file has been loaded into the
 *     new process context.
 *
 * USIG_PROCESS_RUNNING
 *     This is called immediately before the main entry point is called.
 *
 * USIG_PROCESS_EXIT
 *     This is called in the context of a process that is about to
 *     terminate (but before the last thread's USIG_THREAD_EXIT has
 *     been sent).
 *
 * USIG_PROCESS_DESTROY
 *     This is called after a process has terminated.
 *
 *
 * The meaning of the dwFlags bits is as follows:
 *
 * USIG_FLAGS_WIN32
 *     Current process is 32-bit.
 *
 * USIG_FLAGS_GUI
 *     Current process is a (Win32) GUI process.
 *
 * USIG_FLAGS_FEEDBACK
 *     Current process needs 'feedback' (determined from the STARTUPINFO
 *     flags STARTF_FORCEONFEEDBACK / STARTF_FORCEOFFFEEDBACK).
 *
 * USIG_FLAGS_FAULT
 *     The signal is being sent due to a fault.
 */
void PROCESS_CallUserSignalProc( UINT uCode, HMODULE16 hModule )
{
    DWORD dwFlags = 0;
    HMODULE user;
    pUserSignalProc proc;

    if (!(user = GetModuleHandleA( "user32.dll" ))) return;
    if (!(proc = (pUserSignalProc)GetProcAddress( user, "UserSignalProc" ))) return;

    /* Determine dwFlags */

    if ( !(current_process.flags & PDB32_WIN16_PROC) ) dwFlags |= USIG_FLAGS_WIN32;
    if ( !(current_process.flags & PDB32_CONSOLE_PROC) ) dwFlags |= USIG_FLAGS_GUI;

    if ( dwFlags & USIG_FLAGS_GUI )
    {
        /* Feedback defaults to ON */
        if ( !(current_startupinfo.dwFlags & STARTF_FORCEOFFFEEDBACK) )
            dwFlags |= USIG_FLAGS_FEEDBACK;
    }
    else
    {
        /* Feedback defaults to OFF */
        if (current_startupinfo.dwFlags & STARTF_FORCEONFEEDBACK)
            dwFlags |= USIG_FLAGS_FEEDBACK;
    }

    /* Call USER signal proc */

    if ( uCode == USIG_THREAD_INIT || uCode == USIG_THREAD_EXIT )
        proc( uCode, GetCurrentThreadId(), dwFlags, hModule );
    else
        proc( uCode, GetCurrentProcessId(), dwFlags, hModule );
}


/***********************************************************************
 *           get_basename
 */
inline static const char *get_basename( const char *name )
{
    char *p;

    if ((p = strrchr( name, '/' ))) name = p + 1;
    if ((p = strrchr( name, '\\' ))) name = p + 1;
    return name;
}


/***********************************************************************
 *           open_builtin_exe_file
 *
 * Open an exe file for a builtin exe.
 */
static void *open_builtin_exe_file( const char *name, char *error, int error_size, int test_only )
{
    char exename[MAX_PATH], *p;
    const char *basename = get_basename(name);

    if (strlen(basename) >= sizeof(exename)) return NULL;
    strcpy( exename, basename );
    for (p = exename; *p; p++) *p = FILE_tolower(*p);
    return wine_dll_load_main_exe( exename, error, error_size, test_only );
}


/***********************************************************************
 *           open_exe_file
 *
 * Open a specific exe file, taking load order into account.
 * Returns the file handle or 0 for a builtin exe.
 */
static HANDLE open_exe_file( const char *name )
{
    enum loadorder_type loadorder[LOADORDER_NTYPES];
    char buffer[MAX_PATH];
    HANDLE handle;
    int i;

    TRACE("looking for %s\n", debugstr_a(name) );

    if ((handle = CreateFileA( name, GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, 0, 0 )) == INVALID_HANDLE_VALUE)
    {
        /* file doesn't exist, check for builtin */
        if (!FILE_contains_path( name )) goto error;
        if (!MODULE_GetBuiltinPath( name, "", buffer, sizeof(buffer) )) goto error;
        name = buffer;
    }

    MODULE_GetLoadOrder( loadorder, name, TRUE );

    for(i = 0; i < LOADORDER_NTYPES; i++)
    {
        if (loadorder[i] == LOADORDER_INVALID) break;
        switch(loadorder[i])
        {
        case LOADORDER_DLL:
            TRACE( "Trying native exe %s\n", debugstr_a(name) );
            if (handle != INVALID_HANDLE_VALUE) return handle;
            break;
        case LOADORDER_BI:
            TRACE( "Trying built-in exe %s\n", debugstr_a(name) );
            if (open_builtin_exe_file( name, NULL, 0, 1 ))
            {
                if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
                return 0;
            }
        default:
            break;
        }
    }
    if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);

 error:
    SetLastError( ERROR_FILE_NOT_FOUND );
    return INVALID_HANDLE_VALUE;
}


/***********************************************************************
 *           find_exe_file
 *
 * Open an exe file, and return the full name and file handle.
 * Returns FALSE if file could not be found.
 * If file exists but cannot be opened, returns TRUE and set handle to INVALID_HANDLE_VALUE.
 * If file is a builtin exe, returns TRUE and sets handle to 0.
 */
static BOOL find_exe_file( const char *name, char *buffer, int buflen, HANDLE *handle )
{
    enum loadorder_type loadorder[LOADORDER_NTYPES];
    int i;

    TRACE("looking for %s\n", debugstr_a(name) );

    if (!SearchPathA( NULL, name, ".exe", buflen, buffer, NULL ) &&
        !MODULE_GetBuiltinPath( name, ".exe", buffer, buflen ))
    {
        /* no builtin found, try native without extension in case it is a Unix app */

        if (SearchPathA( NULL, name, NULL, buflen, buffer, NULL ))
        {
            TRACE( "Trying native/Unix binary %s\n", debugstr_a(buffer) );
            if ((*handle = CreateFileA( buffer, GENERIC_READ, FILE_SHARE_READ,
                                        NULL, OPEN_EXISTING, 0, 0 )) != INVALID_HANDLE_VALUE)
                return TRUE;
        }
        return FALSE;
    }

    MODULE_GetLoadOrder( loadorder, buffer, TRUE );

    for(i = 0; i < LOADORDER_NTYPES; i++)
    {
        if (loadorder[i] == LOADORDER_INVALID) break;
        switch(loadorder[i])
        {
        case LOADORDER_DLL:
            TRACE( "Trying native exe %s\n", debugstr_a(buffer) );
            if ((*handle = CreateFileA( buffer, GENERIC_READ, FILE_SHARE_READ,
                                        NULL, OPEN_EXISTING, 0, 0 )) != INVALID_HANDLE_VALUE)
                return TRUE;
            if (GetLastError() != ERROR_FILE_NOT_FOUND) return TRUE;
            break;
        case LOADORDER_BI:
            TRACE( "Trying built-in exe %s\n", debugstr_a(buffer) );
            if (open_builtin_exe_file( buffer, NULL, 0, 1 ))
            {
                *handle = 0;
                return TRUE;
            }
            break;
        default:
            break;
        }
    }
    SetLastError( ERROR_FILE_NOT_FOUND );
    return FALSE;
}


/***********************************************************************
 *           process_init
 *
 * Main process initialisation code
 */
static BOOL process_init( char *argv[] )
{
    BOOL ret;
    size_t info_size = 0;

    /* store the program name */
    argv0 = argv[0];

    /* Fill the initial process structure */
    current_process.exit_code       = STILL_ACTIVE;
    current_process.threads         = 1;
    current_process.running_threads = 1;
    current_process.ring0_threads   = 1;
    current_process.group           = &current_process;
    current_process.priority        = 8;  /* Normal */

    /* Setup the server connection */
    CLIENT_InitServer();

    /* Retrieve startup info from the server */
    SERVER_START_REQ( init_process )
    {
        req->ldt_copy  = &wine_ldt_copy;
        req->ppid      = getppid();
        if ((ret = !wine_server_call_err( req )))
        {
            main_exe_file     = reply->exe_file;
            main_create_flags = reply->create_flags;
            info_size         = reply->info_size;
            server_startticks = reply->server_start;
            current_startupinfo.hStdInput   = reply->hstdin;
            current_startupinfo.hStdOutput  = reply->hstdout;
            current_startupinfo.hStdError   = reply->hstderr;
        }
    }
    SERVER_END_REQ;
    if (!ret) return FALSE;

    /* Create the process heap */
    current_process.heap = HeapCreate( HEAP_GROWABLE, 0, 0 );

    if (main_create_flags == 0 &&
	current_startupinfo.hStdInput  == 0 &&
	current_startupinfo.hStdOutput == 0 &&
	current_startupinfo.hStdError  == 0)
    {
	/* no parent, and no new console requested, create a simple console with bare handles to
	 * unix stdio input & output streams (aka simple console)
	 */
        HANDLE handle;
        wine_server_fd_to_handle( 0, GENERIC_READ|SYNCHRONIZE, TRUE, &handle );
        SetStdHandle( STD_INPUT_HANDLE, handle );
        wine_server_fd_to_handle( 1, GENERIC_WRITE|SYNCHRONIZE, TRUE, &handle );
        SetStdHandle( STD_OUTPUT_HANDLE, handle );
        wine_server_fd_to_handle( 1, GENERIC_WRITE|SYNCHRONIZE, TRUE, &handle );
        SetStdHandle( STD_ERROR_HANDLE, handle );
    }
    else if (!(main_create_flags & (DETACHED_PROCESS|CREATE_NEW_CONSOLE)))
    {
	SetStdHandle( STD_INPUT_HANDLE,  current_startupinfo.hStdInput  );
	SetStdHandle( STD_OUTPUT_HANDLE, current_startupinfo.hStdOutput );
	SetStdHandle( STD_ERROR_HANDLE,  current_startupinfo.hStdError  );
    }

    /* Now we can use the pthreads routines */
    PTHREAD_init_done();

    /* Copy the parent environment */
    if (!(current_process.env_db = ENV_InitStartupInfo( info_size, main_exe_name,
                                                        sizeof(main_exe_name) )))
        return FALSE;

    /* Parse command line arguments */
    OPTIONS_ParseOptions( !info_size ? argv : NULL );

    ret = MAIN_MainInit();
    if (TRACE_ON(relay) || TRACE_ON(snoop)) RELAY_InitDebugLists();

    return ret;
}


/***********************************************************************
 *           start_process
 *
 * Startup routine of a new process. Runs on the new process stack.
 */
static void start_process(void)
{
    int debugged, console_app;
    LPTHREAD_START_ROUTINE entry;
    WINE_MODREF *wm;
    HANDLE main_file = main_exe_file;
    IMAGE_NT_HEADERS *nt;

    /* use original argv[0] as name for the main module */
    if (!main_exe_name[0])
    {
        if (!GetLongPathNameA( full_argv0, main_exe_name, sizeof(main_exe_name) ))
            lstrcpynA( main_exe_name, full_argv0, sizeof(main_exe_name) );
    }

    if (main_file)
    {
        UINT drive_type = GetDriveTypeA( main_exe_name );
        /* don't keep the file handle open on removable media */
        if (drive_type == DRIVE_REMOVABLE || drive_type == DRIVE_CDROM) main_file = 0;
    }

    /* Retrieve entry point address */
    nt = RtlImageNtHeader( current_process.module );
    entry = (LPTHREAD_START_ROUTINE)((char*)current_process.module +
                                     nt->OptionalHeader.AddressOfEntryPoint);
    console_app = (nt->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI);
    if (console_app) current_process.flags |= PDB32_CONSOLE_PROC;

    /* Signal the parent process to continue */
    SERVER_START_REQ( init_process_done )
    {
        req->module      = (void *)current_process.module;
        req->module_size = nt->OptionalHeader.SizeOfImage;
        req->entry    = entry;
        /* API requires a double indirection */
        req->name     = &main_exe_name_ptr;
        req->exe_file = main_file;
        req->gui      = !console_app;
        wine_server_add_data( req, main_exe_name, strlen(main_exe_name) );
        wine_server_call( req );
        debugged = reply->debugged;
    }
    SERVER_END_REQ;

    /* Install signal handlers; this cannot be done before, since we cannot
     * send exceptions to the debugger before the create process event that
     * is sent by REQ_INIT_PROCESS_DONE */
    if (!SIGNAL_Init()) goto error;

    /* create the main modref and load dependencies */
    if (!(wm = PE_CreateModule( current_process.module, main_exe_name, 0, 0, FALSE )))
        goto error;
    wm->refCount++;

    if (main_exe_file) CloseHandle( main_exe_file ); /* we no longer need it */

    MODULE_DllProcessAttach( NULL, (LPVOID)1 );

    /* Note: The USIG_PROCESS_CREATE signal is supposed to be sent in the
     *       context of the parent process.  Actually, the USER signal proc
     *       doesn't really care about that, but it *does* require that the
     *       startup parameters are correctly set up, so that GetProcessDword
     *       works.  Furthermore, before calling the USER signal proc the
     *       16-bit stack must be set up, which it is only after TASK_Create
     *       in the case of a 16-bit process. Thus, we send the signal here.
     */
    PROCESS_CallUserSignalProc( USIG_PROCESS_CREATE, 0 );
    PROCESS_CallUserSignalProc( USIG_THREAD_INIT, 0 );
    PROCESS_CallUserSignalProc( USIG_PROCESS_INIT, 0 );
    PROCESS_CallUserSignalProc( USIG_PROCESS_LOADED, 0 );
    /* Call UserSignalProc ( USIG_PROCESS_RUNNING ... ) only for non-GUI win32 apps */
    if (console_app) PROCESS_CallUserSignalProc( USIG_PROCESS_RUNNING, 0 );

    if (TRACE_ON(relay))
        DPRINTF( "%08lx:Starting process %s (entryproc=%p)\n",
                 GetCurrentThreadId(), main_exe_name, entry );
    if (debugged) DbgBreakPoint();
    /* FIXME: should use _PEB as parameter for NT 3.5 programs !
     * Dunno about other OSs */
    SetLastError(0);  /* clear error code */
    ExitThread( entry(NULL) );

 error:
    ExitProcess( GetLastError() );
}


/***********************************************************************
 *           PROCESS_InitWine
 *
 * Wine initialisation: load and start the main exe file.
 */
void PROCESS_InitWine( int argc, char *argv[], LPSTR win16_exe_name, HANDLE *win16_exe_file )
{
    char error[100], *p;
    DWORD stack_size = 0;

    /* Initialize everything */
    if (!process_init( argv )) exit(1);

    argv++;  /* remove argv[0] (wine itself) */

    TRACE( "starting process name=%s file=%x argv[0]=%s\n",
           debugstr_a(main_exe_name), main_exe_file, debugstr_a(argv[0]) );

    if (!main_exe_name[0])
    {
        if (!argv[0]) OPTIONS_Usage();

        if (!find_exe_file( argv[0], main_exe_name, sizeof(main_exe_name), &main_exe_file ))
        {
            MESSAGE( "%s: cannot find '%s'\n", argv0, argv[0] );
            ExitProcess(1);
        }
        if (main_exe_file == INVALID_HANDLE_VALUE)
        {
            MESSAGE( "%s: cannot open '%s'\n", argv0, main_exe_name );
            ExitProcess(1);
        }
    }

    if (!main_exe_file)  /* no file handle -> Winelib app */
    {
        TRACE( "starting Winelib app %s\n", debugstr_a(main_exe_name) );
        if (open_builtin_exe_file( main_exe_name, error, sizeof(error), 0 ))
            goto found;
        MESSAGE( "%s: cannot open builtin library for '%s': %s\n", argv0, main_exe_name, error );
        ExitProcess(1);
    }

    switch( MODULE_GetBinaryType( main_exe_file ))
    {
    case BINARY_PE_EXE:
        TRACE( "starting Win32 binary %s\n", debugstr_a(main_exe_name) );
        if ((current_process.module = PE_LoadImage( main_exe_file, main_exe_name, 0 ))) goto found;
        MESSAGE( "%s: could not load '%s' as Win32 binary\n", argv0, main_exe_name );
        ExitProcess(1);
    case BINARY_PE_DLL:
        MESSAGE( "%s: '%s' is a DLL, not an executable\n", argv0, main_exe_name );
        ExitProcess(1);
    case BINARY_UNKNOWN:
        /* check for .com extension */
        if (!(p = strrchr( main_exe_name, '.' )) || FILE_strcasecmp( p, ".com" ))
        {
            MESSAGE( "%s: cannot determine executable type for '%s'\n", argv0, main_exe_name );
            ExitProcess(1);
        }
        /* fall through */
    case BINARY_WIN16:
    case BINARY_DOS:
        TRACE( "starting Win16/DOS binary %s\n", debugstr_a(main_exe_name) );
        NtCurrentTeb()->tibflags &= ~TEBF_WIN32;
        current_process.flags |= PDB32_WIN16_PROC;
        strcpy( win16_exe_name, main_exe_name );
        main_exe_name[0] = 0;
        *win16_exe_file = main_exe_file;
        main_exe_file = 0;
        _EnterWin16Lock();
        goto found;
    case BINARY_OS216:
        MESSAGE( "%s: '%s' is an OS/2 binary, not supported\n", argv0, main_exe_name );
        ExitProcess(1);
    case BINARY_UNIX_EXE:
        MESSAGE( "%s: '%s' is a Unix binary, not supported\n", argv0, main_exe_name );
        ExitProcess(1);
    case BINARY_UNIX_LIB:
        {
            DOS_FULL_NAME full_name;
            const char *name = main_exe_name;
            UNICODE_STRING nameW;

            TRACE( "starting Winelib app %s\n", debugstr_a(main_exe_name) );
            RtlCreateUnicodeStringFromAsciiz(&nameW, name);
            if (DOSFS_GetFullName( nameW.Buffer, TRUE, &full_name )) name = full_name.long_name;
            RtlFreeUnicodeString(&nameW);
            CloseHandle( main_exe_file );
            main_exe_file = 0;
            if (wine_dlopen( name, RTLD_NOW, error, sizeof(error) ))
            {
                if ((p = strrchr( main_exe_name, '.' )) && !strcmp( p, ".so" )) *p = 0;
                goto found;
            }
            MESSAGE( "%s: could not load '%s': %s\n", argv0, main_exe_name, error );
            ExitProcess(1);
        }
    }

 found:
    /* build command line */
    if (!ENV_BuildCommandLine( argv )) goto error;

    /* create 32-bit module for main exe */
    if (!(current_process.module = BUILTIN32_LoadExeModule( current_process.module ))) goto error;
    stack_size = RtlImageNtHeader(current_process.module)->OptionalHeader.SizeOfStackReserve;

    /* allocate main thread stack */
    if (!THREAD_InitStack( NtCurrentTeb(), stack_size )) goto error;

    /* switch to the new stack */
    SYSDEPS_SwitchToThreadStack( start_process );

 error:
    ExitProcess( GetLastError() );
}


/***********************************************************************
 *           build_argv
 *
 * Build an argv array from a command-line.
 * The command-line is modified to insert nulls.
 * 'reserved' is the number of args to reserve before the first one.
 */
static char **build_argv( char *cmdline, int reserved )
{
    int argc;
    char** argv;
    char *arg,*s,*d;
    int in_quotes,bcount;

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
            /* unescaped '"' */
            in_quotes=!in_quotes;
            bcount=0;
        } else {
            /* a regular character */
            bcount=0;
        }
        s++;
    }
    argv=malloc(argc*sizeof(*argv));
    if (!argv)
        return NULL;

    arg=d=s=cmdline;
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
                /* Preceeded by an even number of '\', this is half that
                 * number of '\', plus a '"' which we discard.
                 */
                d-=bcount/2;
                s++;
                in_quotes=!in_quotes;
            } else {
                /* Preceeded by an odd number of '\', this is half that
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

    return argv;
}


/***********************************************************************
 *           build_envp
 *
 * Build the environment of a new child process.
 */
static char **build_envp( const char *env, const char *extra_env )
{
    const char *p;
    char **envp;
    int count = 0;

    if (extra_env) for (p = extra_env; *p; count++) p += strlen(p) + 1;
    for (p = env; *p; count++) p += strlen(p) + 1;
    count += 3;

    if ((envp = malloc( count * sizeof(*envp) )))
    {
        extern char **environ;
        char **envptr = envp;
        char **unixptr = environ;
        /* first the extra strings */
        if (extra_env) for (p = extra_env; *p; p += strlen(p) + 1) *envptr++ = (char *)p;
        /* then put PATH, HOME and WINEPREFIX from the unix env */
        for (unixptr = environ; unixptr && *unixptr; unixptr++)
            if (!memcmp( *unixptr, "PATH=", 5 ) ||
                !memcmp( *unixptr, "HOME=", 5 ) ||
                !memcmp( *unixptr, "WINEPREFIX=", 11 )) *envptr++ = *unixptr;
        /* now put the Windows environment strings */
        for (p = env; *p; p += strlen(p) + 1)
        {
            if (!memcmp( p, "PATH=", 5 ))  /* store PATH as WINEPATH */
            {
                char *winepath = malloc( strlen(p) + 5 );
                strcpy( winepath, "WINE" );
                strcpy( winepath + 4, p );
                *envptr++ = winepath;
            }
            else if (memcmp( p, "HOME=", 5 ) &&
                     memcmp( p, "WINEPATH=", 9 ) &&
                     memcmp( p, "WINEPREFIX=", 11 )) *envptr++ = (char *)p;
        }
        *envptr = 0;
    }
    return envp;
}


/***********************************************************************
 *           exec_wine_binary
 *
 * Locate the Wine binary to exec for a new Win32 process.
 */
static void exec_wine_binary( char **argv, char **envp )
{
    const char *path, *pos, *ptr;

    /* first, try for a WINELOADER environment variable */
    argv[0] = getenv("WINELOADER");
    if (argv[0])
        execve( argv[0], argv, envp );

    /* next, try bin directory */
    argv[0] = BINDIR "/wine";
    execve( argv[0], argv, envp );

    /* now try the path of argv0 of the current binary */
    if (!(argv[0] = malloc( strlen(full_argv0) + 6 ))) return;
    if ((ptr = strrchr( full_argv0, '/' )))
    {
        memcpy( argv[0], full_argv0, ptr - full_argv0 );
        strcpy( argv[0] + (ptr - full_argv0), "/wine" );
        execve( argv[0], argv, envp );
    }
    free( argv[0] );

    /* now search in the Unix path */
    if ((path = getenv( "PATH" )))
    {
        if (!(argv[0] = malloc( strlen(path) + 6 ))) return;
        pos = path;
        for (;;)
        {
            while (*pos == ':') pos++;
            if (!*pos) break;
            if (!(ptr = strchr( pos, ':' ))) ptr = pos + strlen(pos);
            memcpy( argv[0], pos, ptr - pos );
            strcpy( argv[0] + (ptr - pos), "/wine" );
            execve( argv[0], argv, envp );
            pos = ptr;
        }
    }
    free( argv[0] );
}


/***********************************************************************
 *           fork_and_exec
 *
 * Fork and exec a new Unix process, checking for errors.
 */
static int fork_and_exec( const char *filename, char *cmdline,
                          const char *env, const char *newdir )
{
    int fd[2];
    int pid, err;
    char *extra_env = NULL;

    if (!env)
    {
        env = GetEnvironmentStringsA();
        extra_env = DRIVE_BuildEnv();
    }

    if (pipe(fd) == -1)
    {
        FILE_SetDosError();
        return -1;
    }
    fcntl( fd[1], F_SETFD, 1 );  /* set close on exec */
    if (!(pid = fork()))  /* child */
    {
        char **argv = build_argv( cmdline, filename ? 0 : 1 );
        char **envp = build_envp( env, extra_env );
        close( fd[0] );

        /* Reset signals that we previously set to SIG_IGN */
        signal( SIGPIPE, SIG_DFL );
        signal( SIGCHLD, SIG_DFL );

        if (newdir) chdir(newdir);

        if (argv && envp)
        {
            if (!filename) exec_wine_binary( argv, envp );
            else execve( filename, argv, envp );
        }
        err = errno;
        write( fd[1], &err, sizeof(err) );
        _exit(1);
    }
    close( fd[1] );
    if ((pid != -1) && (read( fd[0], &err, sizeof(err) ) > 0))  /* exec failed */
    {
        errno = err;
        pid = -1;
    }
    if (pid == -1) FILE_SetDosError();
    close( fd[0] );
    if (extra_env) HeapFree( GetProcessHeap(), 0, extra_env );
    return pid;
}


/***********************************************************************
 *           create_process
 *
 * Create a new process. If hFile is a valid handle we have an exe
 * file, otherwise it is a Winelib app.
 */
static BOOL create_process( HANDLE hFile, LPCSTR filename, LPSTR cmd_line, LPCSTR env,
                            LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                            BOOL inherit, DWORD flags, LPSTARTUPINFOA startup,
                            LPPROCESS_INFORMATION info, LPCSTR unixdir )
{
    BOOL ret, success = FALSE;
    HANDLE process_info;
    startup_info_t startup_info;

    /* fill the startup info structure */

    startup_info.size        = sizeof(startup_info);
    /* startup_info.filename_len is set below */
    startup_info.cmdline_len = cmd_line ? strlen(cmd_line) : 0;
    startup_info.desktop_len = startup->lpDesktop ? strlen(startup->lpDesktop) : 0;
    startup_info.title_len   = startup->lpTitle ? strlen(startup->lpTitle) : 0;
    startup_info.x           = startup->dwX;
    startup_info.y           = startup->dwY;
    startup_info.cx          = startup->dwXSize;
    startup_info.cy          = startup->dwYSize;
    startup_info.x_chars     = startup->dwXCountChars;
    startup_info.y_chars     = startup->dwYCountChars;
    startup_info.attribute   = startup->dwFillAttribute;
    startup_info.cmd_show    = startup->wShowWindow;
    startup_info.flags       = startup->dwFlags;

    /* create the process on the server side */

    SERVER_START_REQ( new_process )
    {
        char buf[MAX_PATH];
        LPCSTR nameptr;

        req->inherit_all  = inherit;
        req->create_flags = flags;
        req->use_handles  = (startup->dwFlags & STARTF_USESTDHANDLES) != 0;
        req->exe_file     = hFile;
        if (startup->dwFlags & STARTF_USESTDHANDLES)
        {
            req->hstdin  = startup->hStdInput;
            req->hstdout = startup->hStdOutput;
            req->hstderr = startup->hStdError;
        }
        else
        {
            req->hstdin  = GetStdHandle( STD_INPUT_HANDLE );
            req->hstdout = GetStdHandle( STD_OUTPUT_HANDLE );
            req->hstderr = GetStdHandle( STD_ERROR_HANDLE );
        }

        if (GetLongPathNameA( filename, buf, MAX_PATH ))
            nameptr = buf;
        else
            nameptr = filename;

        startup_info.filename_len = strlen(nameptr);
        wine_server_add_data( req, &startup_info, sizeof(startup_info) );
        wine_server_add_data( req, nameptr, startup_info.filename_len );
        wine_server_add_data( req, cmd_line, startup_info.cmdline_len );
        wine_server_add_data( req, startup->lpDesktop, startup_info.desktop_len );
        wine_server_add_data( req, startup->lpTitle, startup_info.title_len );

        ret = !wine_server_call_err( req );
        process_info = reply->info;
    }
    SERVER_END_REQ;
    if (!ret) return FALSE;

    /* fork and execute */

    if (fork_and_exec( NULL, cmd_line, env, unixdir ) == -1)
    {
        CloseHandle( process_info );
        return FALSE;
    }

    /* wait for the new process info to be ready */

    WaitForSingleObject( process_info, INFINITE );
    SERVER_START_REQ( get_new_process_info )
    {
        req->info     = process_info;
        req->pinherit = (psa && (psa->nLength >= sizeof(*psa)) && psa->bInheritHandle);
        req->tinherit = (tsa && (tsa->nLength >= sizeof(*tsa)) && tsa->bInheritHandle);
        if ((ret = !wine_server_call_err( req )))
        {
            info->dwProcessId = (DWORD)reply->pid;
            info->dwThreadId  = (DWORD)reply->tid;
            info->hProcess    = reply->phandle;
            info->hThread     = reply->thandle;
            success           = reply->success;
        }
    }
    SERVER_END_REQ;

    if (ret && !success)  /* new process failed to start */
    {
        DWORD exitcode;
        if (GetExitCodeProcess( info->hProcess, &exitcode )) SetLastError( exitcode );
        CloseHandle( info->hThread );
        CloseHandle( info->hProcess );
        ret = FALSE;
    }
    CloseHandle( process_info );
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
static LPSTR get_file_name( LPCSTR appname, LPSTR cmdline, LPSTR buffer,
                            int buflen, HANDLE *handle )
{
    char *name, *pos, *ret = NULL;
    const char *p;

    /* if we have an app name, everything is easy */

    if (appname)
    {
        /* use the unmodified app name as file name */
        lstrcpynA( buffer, appname, buflen );
        *handle = open_exe_file( buffer );
        if (!(ret = cmdline) || !cmdline[0])
        {
            /* no command-line, create one */
            if ((ret = HeapAlloc( GetProcessHeap(), 0, strlen(appname) + 3 )))
                sprintf( ret, "\"%s\"", appname );
        }
        return ret;
    }

    if (!cmdline)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    /* first check for a quoted file name */

    if ((cmdline[0] == '"') && ((p = strchr( cmdline + 1, '"' ))))
    {
        int len = p - cmdline - 1;
        /* extract the quoted portion as file name */
        if (!(name = HeapAlloc( GetProcessHeap(), 0, len + 1 ))) return NULL;
        memcpy( name, cmdline + 1, len );
        name[len] = 0;

        if (find_exe_file( name, buffer, buflen, handle ))
            ret = cmdline;  /* no change necessary */
        goto done;
    }

    /* now try the command-line word by word */

    if (!(name = HeapAlloc( GetProcessHeap(), 0, strlen(cmdline) + 1 ))) return NULL;
    pos = name;
    p = cmdline;

    while (*p)
    {
        do *pos++ = *p++; while (*p && *p != ' ');
        *pos = 0;
        if (find_exe_file( name, buffer, buflen, handle ))
        {
            ret = cmdline;
            break;
        }
    }

    if (!ret || !strchr( name, ' ' )) goto done;  /* no change necessary */

    /* now build a new command-line with quotes */

    if (!(ret = HeapAlloc( GetProcessHeap(), 0, strlen(cmdline) + 3 ))) goto done;
    sprintf( ret, "\"%s\"%s", name, p );

 done:
    HeapFree( GetProcessHeap(), 0, name );
    return ret;
}


/**********************************************************************
 *       CreateProcessA          (KERNEL32.@)
 */
BOOL WINAPI CreateProcessA( LPCSTR app_name, LPSTR cmd_line, LPSECURITY_ATTRIBUTES process_attr,
                            LPSECURITY_ATTRIBUTES thread_attr, BOOL inherit,
                            DWORD flags, LPVOID env, LPCSTR cur_dir,
                            LPSTARTUPINFOA startup_info, LPPROCESS_INFORMATION info )
{
    BOOL retv = FALSE;
    HANDLE hFile = 0;
    const char *unixdir = NULL;
    DOS_FULL_NAME full_dir;
    char name[MAX_PATH];
    LPSTR tidy_cmdline;
    char *p;

    /* Process the AppName and/or CmdLine to get module name and path */

    TRACE("app %s cmdline %s\n", debugstr_a(app_name), debugstr_a(cmd_line) );

    if (!(tidy_cmdline = get_file_name( app_name, cmd_line, name, sizeof(name), &hFile )))
        return FALSE;
    if (hFile == INVALID_HANDLE_VALUE) goto done;

    /* Warn if unsupported features are used */

    if (flags & NORMAL_PRIORITY_CLASS)
        FIXME("(%s,...): NORMAL_PRIORITY_CLASS ignored\n", name);
    if (flags & IDLE_PRIORITY_CLASS)
        FIXME("(%s,...): IDLE_PRIORITY_CLASS ignored\n", name);
    if (flags & HIGH_PRIORITY_CLASS)
        FIXME("(%s,...): HIGH_PRIORITY_CLASS ignored\n", name);
    if (flags & REALTIME_PRIORITY_CLASS)
        FIXME("(%s,...): REALTIME_PRIORITY_CLASS ignored\n", name);
    if (flags & CREATE_NEW_PROCESS_GROUP)
        FIXME("(%s,...): CREATE_NEW_PROCESS_GROUP ignored\n", name);
    if (flags & CREATE_UNICODE_ENVIRONMENT)
        FIXME("(%s,...): CREATE_UNICODE_ENVIRONMENT ignored\n", name);
    if (flags & CREATE_SEPARATE_WOW_VDM)
        FIXME("(%s,...): CREATE_SEPARATE_WOW_VDM ignored\n", name);
    if (flags & CREATE_SHARED_WOW_VDM)
        FIXME("(%s,...): CREATE_SHARED_WOW_VDM ignored\n", name);
    if (flags & CREATE_DEFAULT_ERROR_MODE)
        FIXME("(%s,...): CREATE_DEFAULT_ERROR_MODE ignored\n", name);
    if (flags & CREATE_NO_WINDOW)
        FIXME("(%s,...): CREATE_NO_WINDOW ignored\n", name);
    if (flags & PROFILE_USER)
        FIXME("(%s,...): PROFILE_USER ignored\n", name);
    if (flags & PROFILE_KERNEL)
        FIXME("(%s,...): PROFILE_KERNEL ignored\n", name);
    if (flags & PROFILE_SERVER)
        FIXME("(%s,...): PROFILE_SERVER ignored\n", name);
    if (startup_info->lpDesktop)
        FIXME("(%s,...): startup_info->lpDesktop %s ignored\n",
              name, debugstr_a(startup_info->lpDesktop));
    if (startup_info->dwFlags & STARTF_RUNFULLSCREEN)
        FIXME("(%s,...): STARTF_RUNFULLSCREEN ignored\n", name);
    if (startup_info->dwFlags & STARTF_FORCEONFEEDBACK)
        FIXME("(%s,...): STARTF_FORCEONFEEDBACK ignored\n", name);
    if (startup_info->dwFlags & STARTF_FORCEOFFFEEDBACK)
        FIXME("(%s,...): STARTF_FORCEOFFFEEDBACK ignored\n", name);
    if (startup_info->dwFlags & STARTF_USEHOTKEY)
        FIXME("(%s,...): STARTF_USEHOTKEY ignored\n", name);

    if (cur_dir)
    {
        UNICODE_STRING cur_dirW;
        RtlCreateUnicodeStringFromAsciiz(&cur_dirW, cur_dir);
        if (DOSFS_GetFullName( cur_dirW.Buffer, TRUE, &full_dir ))
            unixdir = full_dir.long_name;
        RtlFreeUnicodeString(&cur_dirW);
    }
    else
    {
        WCHAR buf[MAX_PATH];
        if (GetCurrentDirectoryW(MAX_PATH, buf))
        {
            if (DOSFS_GetFullName( buf, TRUE, &full_dir )) unixdir = full_dir.long_name;
        }
    }

    info->hThread = info->hProcess = 0;
    info->dwProcessId = info->dwThreadId = 0;

    /* Determine executable type */

    if (!hFile)  /* builtin exe */
    {
        TRACE( "starting %s as Winelib app\n", debugstr_a(name) );
        retv = create_process( 0, name, tidy_cmdline, env, process_attr, thread_attr,
                               inherit, flags, startup_info, info, unixdir );
        goto done;
    }

    switch( MODULE_GetBinaryType( hFile ))
    {
    case BINARY_PE_EXE:
    case BINARY_WIN16:
    case BINARY_DOS:
        TRACE( "starting %s as Windows binary\n", debugstr_a(name) );
        retv = create_process( hFile, name, tidy_cmdline, env, process_attr, thread_attr,
                               inherit, flags, startup_info, info, unixdir );
        break;
    case BINARY_OS216:
        FIXME( "%s is OS/2 binary, not supported\n", debugstr_a(name) );
        SetLastError( ERROR_BAD_EXE_FORMAT );
        break;
    case BINARY_PE_DLL:
        TRACE( "not starting %s since it is a dll\n", debugstr_a(name) );
        SetLastError( ERROR_BAD_EXE_FORMAT );
        break;
    case BINARY_UNIX_LIB:
        TRACE( "%s is a Unix library, starting as Winelib app\n", debugstr_a(name) );
        retv = create_process( hFile, name, tidy_cmdline, env, process_attr, thread_attr,
                               inherit, flags, startup_info, info, unixdir );
        break;
    case BINARY_UNKNOWN:
        /* check for .com extension */
        if ((p = strrchr( name, '.' )) && !FILE_strcasecmp( p, ".com" ))
        {
            TRACE( "starting %s as DOS binary\n", debugstr_a(name) );
            retv = create_process( hFile, name, tidy_cmdline, env, process_attr, thread_attr,
                                   inherit, flags, startup_info, info, unixdir );
            break;
        }
        /* fall through */
    case BINARY_UNIX_EXE:
        {
            /* unknown file, try as unix executable */
            UNICODE_STRING nameW;
            DOS_FULL_NAME full_name;
            const char *unixfilename = name;

            TRACE( "starting %s as Unix binary\n", debugstr_a(name) );

            RtlCreateUnicodeStringFromAsciiz(&nameW, name);
            if (DOSFS_GetFullName( nameW.Buffer, TRUE, &full_name )) unixfilename = full_name.long_name;
            RtlFreeUnicodeString(&nameW);
            retv = (fork_and_exec( unixfilename, tidy_cmdline, env, unixdir ) != -1);
        }
        break;
    }
    CloseHandle( hFile );

 done:
    if (tidy_cmdline != cmd_line) HeapFree( GetProcessHeap(), 0, tidy_cmdline );
    return retv;
}


/**********************************************************************
 *       CreateProcessW          (KERNEL32.@)
 * NOTES
 *  lpReserved is not converted
 */
BOOL WINAPI CreateProcessW( LPCWSTR app_name, LPWSTR cmd_line, LPSECURITY_ATTRIBUTES process_attr,
                            LPSECURITY_ATTRIBUTES thread_attr, BOOL inherit, DWORD flags,
                            LPVOID env, LPCWSTR cur_dir, LPSTARTUPINFOW startup_info,
                            LPPROCESS_INFORMATION info )
{
    BOOL ret;
    STARTUPINFOA StartupInfoA;

    LPSTR app_nameA = HEAP_strdupWtoA (GetProcessHeap(),0,app_name);
    LPSTR cmd_lineA = HEAP_strdupWtoA (GetProcessHeap(),0,cmd_line);
    LPSTR cur_dirA = HEAP_strdupWtoA (GetProcessHeap(),0,cur_dir);

    memcpy (&StartupInfoA, startup_info, sizeof(STARTUPINFOA));
    StartupInfoA.lpDesktop = HEAP_strdupWtoA (GetProcessHeap(),0,startup_info->lpDesktop);
    StartupInfoA.lpTitle = HEAP_strdupWtoA (GetProcessHeap(),0,startup_info->lpTitle);

    TRACE_(win32)("(%s,%s,...)\n", debugstr_w(app_name), debugstr_w(cmd_line));

    if (startup_info->lpReserved)
      FIXME_(win32)("StartupInfo.lpReserved is used, please report (%s)\n",
                    debugstr_w(startup_info->lpReserved));

    ret = CreateProcessA( app_nameA,  cmd_lineA, process_attr, thread_attr,
                          inherit, flags, env, cur_dirA, &StartupInfoA, info );

    HeapFree( GetProcessHeap(), 0, cur_dirA );
    HeapFree( GetProcessHeap(), 0, cmd_lineA );
    HeapFree( GetProcessHeap(), 0, StartupInfoA.lpDesktop );
    HeapFree( GetProcessHeap(), 0, StartupInfoA.lpTitle );

    return ret;
}


/***********************************************************************
 *           ExitProcess   (KERNEL32.@)
 */
void WINAPI ExitProcess( DWORD status )
{
    MODULE_DllProcessDetach( TRUE, (LPVOID)1 );
    SERVER_START_REQ( terminate_process )
    {
        /* send the exit code to the server */
        req->handle    = GetCurrentProcess();
        req->exit_code = status;
        wine_server_call( req );
    }
    SERVER_END_REQ;
    exit( status );
}

/***********************************************************************
 *           ExitProcess   (KERNEL.466)
 */
void WINAPI ExitProcess16( WORD status )
{
    DWORD count;
    ReleaseThunkLock( &count );
    ExitProcess( status );
}

/******************************************************************************
 *           TerminateProcess   (KERNEL32.@)
 */
BOOL WINAPI TerminateProcess( HANDLE handle, DWORD exit_code )
{
    NTSTATUS status = NtTerminateProcess( handle, exit_code );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *           GetProcessDword    (KERNEL.485)
 *           GetProcessDword    (KERNEL32.18)
 * 'Of course you cannot directly access Windows internal structures'
 */
DWORD WINAPI GetProcessDword( DWORD dwProcessID, INT offset )
{
    DWORD x, y;

    TRACE_(win32)("(%ld, %d)\n", dwProcessID, offset );

    if (dwProcessID && dwProcessID != GetCurrentProcessId())
    {
        ERR("%d: process %lx not accessible\n", offset, dwProcessID);
        return 0;
    }

    switch ( offset )
    {
    case GPD_APP_COMPAT_FLAGS:
        return GetAppCompatFlags16(0);

    case GPD_LOAD_DONE_EVENT:
        return (DWORD)current_process.load_done_evt;

    case GPD_HINSTANCE16:
        return GetTaskDS16();

    case GPD_WINDOWS_VERSION:
        return GetExeVersion16();

    case GPD_THDB:
        return (DWORD)NtCurrentTeb() - 0x10 /* FIXME */;

    case GPD_PDB:
        return (DWORD)&current_process;

    case GPD_STARTF_SHELLDATA: /* return stdoutput handle from startupinfo ??? */
        return (DWORD)current_startupinfo.hStdOutput;

    case GPD_STARTF_HOTKEY: /* return stdinput handle from startupinfo ??? */
        return (DWORD)current_startupinfo.hStdInput;

    case GPD_STARTF_SHOWWINDOW:
        return current_startupinfo.wShowWindow;

    case GPD_STARTF_SIZE:
        x = current_startupinfo.dwXSize;
        if ( (INT)x == CW_USEDEFAULT ) x = CW_USEDEFAULT16;
        y = current_startupinfo.dwYSize;
        if ( (INT)y == CW_USEDEFAULT ) y = CW_USEDEFAULT16;
        return MAKELONG( x, y );

    case GPD_STARTF_POSITION:
        x = current_startupinfo.dwX;
        if ( (INT)x == CW_USEDEFAULT ) x = CW_USEDEFAULT16;
        y = current_startupinfo.dwY;
        if ( (INT)y == CW_USEDEFAULT ) y = CW_USEDEFAULT16;
        return MAKELONG( x, y );

    case GPD_STARTF_FLAGS:
        return current_startupinfo.dwFlags;

    case GPD_PARENT:
        return 0;

    case GPD_FLAGS:
        return current_process.flags;

    case GPD_USERDATA:
        return current_process.process_dword;

    default:
        ERR_(win32)("Unknown offset %d\n", offset );
        return 0;
    }
}

/***********************************************************************
 *           SetProcessDword    (KERNEL.484)
 * 'Of course you cannot directly access Windows internal structures'
 */
void WINAPI SetProcessDword( DWORD dwProcessID, INT offset, DWORD value )
{
    TRACE_(win32)("(%ld, %d)\n", dwProcessID, offset );

    if (dwProcessID && dwProcessID != GetCurrentProcessId())
    {
        ERR("%d: process %lx not accessible\n", offset, dwProcessID);
        return;
    }

    switch ( offset )
    {
    case GPD_APP_COMPAT_FLAGS:
    case GPD_LOAD_DONE_EVENT:
    case GPD_HINSTANCE16:
    case GPD_WINDOWS_VERSION:
    case GPD_THDB:
    case GPD_PDB:
    case GPD_STARTF_SHELLDATA:
    case GPD_STARTF_HOTKEY:
    case GPD_STARTF_SHOWWINDOW:
    case GPD_STARTF_SIZE:
    case GPD_STARTF_POSITION:
    case GPD_STARTF_FLAGS:
    case GPD_PARENT:
    case GPD_FLAGS:
        ERR_(win32)("Not allowed to modify offset %d\n", offset );
        break;

    case GPD_USERDATA:
        current_process.process_dword = value;
        break;

    default:
        ERR_(win32)("Unknown offset %d\n", offset );
        break;
    }
}


/*********************************************************************
 *           OpenProcess   (KERNEL32.@)
 */
HANDLE WINAPI OpenProcess( DWORD access, BOOL inherit, DWORD id )
{
    HANDLE ret = 0;
    SERVER_START_REQ( open_process )
    {
        req->pid     = id;
        req->access  = access;
        req->inherit = inherit;
        if (!wine_server_call_err( req )) ret = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}

/*********************************************************************
 *           MapProcessHandle   (KERNEL.483)
 */
DWORD WINAPI MapProcessHandle( HANDLE handle )
{
    DWORD ret = 0;
    SERVER_START_REQ( get_process_info )
    {
        req->handle = handle;
        if (!wine_server_call_err( req )) ret = (DWORD)reply->pid;
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           SetPriorityClass   (KERNEL32.@)
 */
BOOL WINAPI SetPriorityClass( HANDLE hprocess, DWORD priorityclass )
{
    BOOL ret;
    SERVER_START_REQ( set_process_info )
    {
        req->handle   = hprocess;
        req->priority = priorityclass;
        req->mask     = SET_PROCESS_INFO_PRIORITY;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           GetPriorityClass   (KERNEL32.@)
 */
DWORD WINAPI GetPriorityClass(HANDLE hprocess)
{
    DWORD ret = 0;
    SERVER_START_REQ( get_process_info )
    {
        req->handle = hprocess;
        if (!wine_server_call_err( req )) ret = reply->priority;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *          SetProcessAffinityMask   (KERNEL32.@)
 */
BOOL WINAPI SetProcessAffinityMask( HANDLE hProcess, DWORD affmask )
{
    BOOL ret;
    SERVER_START_REQ( set_process_info )
    {
        req->handle   = hProcess;
        req->affinity = affmask;
        req->mask     = SET_PROCESS_INFO_AFFINITY;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/**********************************************************************
 *          GetProcessAffinityMask    (KERNEL32.@)
 */
BOOL WINAPI GetProcessAffinityMask( HANDLE hProcess,
                                      LPDWORD lpProcessAffinityMask,
                                      LPDWORD lpSystemAffinityMask )
{
    BOOL ret = FALSE;
    SERVER_START_REQ( get_process_info )
    {
        req->handle = hProcess;
        if (!wine_server_call_err( req ))
        {
            if (lpProcessAffinityMask) *lpProcessAffinityMask = reply->process_affinity;
            if (lpSystemAffinityMask) *lpSystemAffinityMask = reply->system_affinity;
            ret = TRUE;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           GetProcessVersion    (KERNEL32.@)
 */
DWORD WINAPI GetProcessVersion( DWORD processid )
{
    IMAGE_NT_HEADERS *nt;

    if (processid && processid != GetCurrentProcessId())
    {
	FIXME("should use ReadProcessMemory\n");
        return 0;
    }
    if ((nt = RtlImageNtHeader( current_process.module )))
        return ((nt->OptionalHeader.MajorSubsystemVersion << 16) |
                nt->OptionalHeader.MinorSubsystemVersion);
    return 0;
}

/***********************************************************************
 *           GetProcessFlags    (KERNEL32.@)
 */
DWORD WINAPI GetProcessFlags( DWORD processid )
{
    if (processid && processid != GetCurrentProcessId()) return 0;
    return current_process.flags;
}


/***********************************************************************
 *		SetProcessWorkingSetSize	[KERNEL32.@]
 * Sets the min/max working set sizes for a specified process.
 *
 * PARAMS
 *    hProcess [I] Handle to the process of interest
 *    minset   [I] Specifies minimum working set size
 *    maxset   [I] Specifies maximum working set size
 *
 * RETURNS  STD
 */
BOOL WINAPI SetProcessWorkingSetSize(HANDLE hProcess, SIZE_T minset,
                                     SIZE_T maxset)
{
    FIXME("(0x%08x,%ld,%ld): stub - harmless\n",hProcess,minset,maxset);
    if(( minset == (SIZE_T)-1) && (maxset == (SIZE_T)-1)) {
        /* Trim the working set to zero */
        /* Swap the process out of physical RAM */
    }
    return TRUE;
}

/***********************************************************************
 *           GetProcessWorkingSetSize    (KERNEL32.@)
 */
BOOL WINAPI GetProcessWorkingSetSize(HANDLE hProcess, PSIZE_T minset,
                                     PSIZE_T maxset)
{
	FIXME("(0x%08x,%p,%p): stub\n",hProcess,minset,maxset);
	/* 32 MB working set size */
	if (minset) *minset = 32*1024*1024;
	if (maxset) *maxset = 32*1024*1024;
	return TRUE;
}

/***********************************************************************
 *           SetProcessShutdownParameters    (KERNEL32.@)
 *
 * CHANGED - James Sutherland (JamesSutherland@gmx.de)
 * Now tracks changes made (but does not act on these changes)
 */
static DWORD shutdown_flags = 0;
static DWORD shutdown_priority = 0x280;

BOOL WINAPI SetProcessShutdownParameters(DWORD level, DWORD flags)
{
    FIXME("(%08lx, %08lx): partial stub.\n", level, flags);
    shutdown_flags = flags;
    shutdown_priority = level;
    return TRUE;
}


/***********************************************************************
 * GetProcessShutdownParameters                 (KERNEL32.@)
 *
 */
BOOL WINAPI GetProcessShutdownParameters( LPDWORD lpdwLevel, LPDWORD lpdwFlags )
{
    *lpdwLevel = shutdown_priority;
    *lpdwFlags = shutdown_flags;
    return TRUE;
}


/***********************************************************************
 *           SetProcessPriorityBoost    (KERNEL32.@)
 */
BOOL WINAPI SetProcessPriorityBoost(HANDLE hprocess,BOOL disableboost)
{
    FIXME("(%d,%d): stub\n",hprocess,disableboost);
    /* Say we can do it. I doubt the program will notice that we don't. */
    return TRUE;
}


/***********************************************************************
 *		ReadProcessMemory (KERNEL32.@)
 */
BOOL WINAPI ReadProcessMemory( HANDLE process, LPCVOID addr, LPVOID buffer, SIZE_T size,
                               SIZE_T *bytes_read )
{
    DWORD res;

    SERVER_START_REQ( read_process_memory )
    {
        req->handle = process;
        req->addr   = (void *)addr;
        wine_server_set_reply( req, buffer, size );
        if ((res = wine_server_call_err( req ))) size = 0;
    }
    SERVER_END_REQ;
    if (bytes_read) *bytes_read = size;
    return !res;
}


/***********************************************************************
 *           WriteProcessMemory    		(KERNEL32.@)
 */
BOOL WINAPI WriteProcessMemory( HANDLE process, LPVOID addr, LPCVOID buffer, SIZE_T size,
                                SIZE_T *bytes_written )
{
    static const int zero;
    unsigned int first_offset, last_offset, first_mask, last_mask;
    DWORD res;

    if (!size)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* compute the mask for the first int */
    first_mask = ~0;
    first_offset = (unsigned int)addr % sizeof(int);
    memset( &first_mask, 0, first_offset );

    /* compute the mask for the last int */
    last_offset = (size + first_offset) % sizeof(int);
    last_mask = 0;
    memset( &last_mask, 0xff, last_offset ? last_offset : sizeof(int) );

    SERVER_START_REQ( write_process_memory )
    {
        req->handle     = process;
        req->addr       = (char *)addr - first_offset;
        req->first_mask = first_mask;
        req->last_mask  = last_mask;
        if (first_offset) wine_server_add_data( req, &zero, first_offset );
        wine_server_add_data( req, buffer, size );
        if (last_offset) wine_server_add_data( req, &zero, sizeof(int) - last_offset );

        if ((res = wine_server_call_err( req ))) size = 0;
    }
    SERVER_END_REQ;
    if (bytes_written) *bytes_written = size;
    {
        char dummy[32];
        SIZE_T read;
        ReadProcessMemory( process, addr, dummy, sizeof(dummy), &read );
    }
    return !res;
}


/***********************************************************************
 *		RegisterServiceProcess (KERNEL.491)
 *		RegisterServiceProcess (KERNEL32.@)
 *
 * A service process calls this function to ensure that it continues to run
 * even after a user logged off.
 */
DWORD WINAPI RegisterServiceProcess(DWORD dwProcessId, DWORD dwType)
{
	/* I don't think that Wine needs to do anything in that function */
	return 1; /* success */
}

/***********************************************************************
 * GetExitCodeProcess [KERNEL32.@]
 *
 * Gets termination status of specified process
 *
 * RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 */
BOOL WINAPI GetExitCodeProcess(
    HANDLE hProcess,    /* [in] handle to the process */
    LPDWORD lpExitCode) /* [out] address to receive termination status */
{
    BOOL ret;
    SERVER_START_REQ( get_process_info )
    {
        req->handle = hProcess;
        ret = !wine_server_call_err( req );
        if (ret && lpExitCode) *lpExitCode = reply->exit_code;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           SetErrorMode   (KERNEL32.@)
 */
UINT WINAPI SetErrorMode( UINT mode )
{
    UINT old = current_process.error_mode;
    current_process.error_mode = mode;
    return old;
}


/**************************************************************************
 *              SetFileApisToOEM   (KERNEL32.@)
 */
VOID WINAPI SetFileApisToOEM(void)
{
    current_process.flags |= PDB32_FILE_APIS_OEM;
}


/**************************************************************************
 *              SetFileApisToANSI   (KERNEL32.@)
 */
VOID WINAPI SetFileApisToANSI(void)
{
    current_process.flags &= ~PDB32_FILE_APIS_OEM;
}


/******************************************************************************
 * AreFileApisANSI [KERNEL32.@]  Determines if file functions are using ANSI
 *
 * RETURNS
 *    TRUE:  Set of file functions is using ANSI code page
 *    FALSE: Set of file functions is using OEM code page
 */
BOOL WINAPI AreFileApisANSI(void)
{
    return !(current_process.flags & PDB32_FILE_APIS_OEM);
}


/***********************************************************************
 *           GetTickCount       (KERNEL32.@)
 *
 * Returns the number of milliseconds, modulo 2^32, since the start
 * of the wineserver.
 */
DWORD WINAPI GetTickCount(void)
{
    struct timeval t;
    gettimeofday( &t, NULL );
    return ((t.tv_sec * 1000) + (t.tv_usec / 1000)) - server_startticks;
}


/**********************************************************************
 * TlsAlloc [KERNEL32.@]  Allocates a TLS index.
 *
 * Allocates a thread local storage index
 *
 * RETURNS
 *    Success: TLS Index
 *    Failure: 0xFFFFFFFF
 */
DWORD WINAPI TlsAlloc( void )
{
    DWORD i, mask, ret = 0;
    DWORD *bits = current_process.tls_bits;
    RtlAcquirePebLock();
    if (*bits == 0xffffffff)
    {
        bits++;
        ret = 32;
        if (*bits == 0xffffffff)
        {
            RtlReleasePebLock();
            SetLastError( ERROR_NO_MORE_ITEMS );
            return 0xffffffff;
        }
    }
    for (i = 0, mask = 1; i < 32; i++, mask <<= 1) if (!(*bits & mask)) break;
    *bits |= mask;
    RtlReleasePebLock();
    NtCurrentTeb()->tls_array[ret+i] = 0; /* clear the value */
    return ret + i;
}


/**********************************************************************
 * TlsFree [KERNEL32.@]  Releases a TLS index.
 *
 * Releases a thread local storage index, making it available for reuse
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI TlsFree(
    DWORD index) /* [in] TLS Index to free */
{
    DWORD mask = (1 << (index & 31));
    DWORD *bits = current_process.tls_bits;
    if (index >= 64)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (index >= 32) bits++;
    RtlAcquirePebLock();
    if (!(*bits & mask))  /* already free? */
    {
        RtlReleasePebLock();
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    *bits &= ~mask;
    NtCurrentTeb()->tls_array[index] = 0;
    /* FIXME: should zero all other thread values */
    RtlReleasePebLock();
    return TRUE;
}


/**********************************************************************
 * TlsGetValue [KERNEL32.@]  Gets value in a thread's TLS slot
 *
 * RETURNS
 *    Success: Value stored in calling thread's TLS slot for index
 *    Failure: 0 and GetLastError returns NO_ERROR
 */
LPVOID WINAPI TlsGetValue(
    DWORD index) /* [in] TLS index to retrieve value for */
{
    if (index >= 64)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    SetLastError( ERROR_SUCCESS );
    return NtCurrentTeb()->tls_array[index];
}


/**********************************************************************
 * TlsSetValue [KERNEL32.@]  Stores a value in the thread's TLS slot.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI TlsSetValue(
    DWORD index,  /* [in] TLS index to set value for */
    LPVOID value) /* [in] Value to be stored */
{
    if (index >= 64)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    NtCurrentTeb()->tls_array[index] = value;
    return TRUE;
}


/***********************************************************************
 *           GetCurrentProcess   (KERNEL32.@)
 */
#undef GetCurrentProcess
HANDLE WINAPI GetCurrentProcess(void)
{
    return (HANDLE)0xffffffff;
}
