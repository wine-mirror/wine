/*
 * Win32 processes
 *
 * Copyright 1996, 1998 Alexandre Julliard
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "wine/winbase16.h"
#include "wine/exception.h"
#include "process.h"
#include "main.h"
#include "module.h"
#include "neexe.h"
#include "file.h"
#include "global.h"
#include "heap.h"
#include "task.h"
#include "ldt.h"
#include "syslevel.h"
#include "thread.h"
#include "winerror.h"
#include "pe_image.h"
#include "server.h"
#include "options.h"
#include "callback.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(process);
DECLARE_DEBUG_CHANNEL(relay);
DECLARE_DEBUG_CHANNEL(win32);


static ENVDB initial_envdb;
static STARTUPINFOA initial_startup;
static char **main_exe_argv;
static char *main_exe_name;
static HFILE main_exe_file = -1;


/***********************************************************************
 *           PROCESS_IdToPDB
 *
 * Convert a process id to a PDB, making sure it is valid.
 */
PDB *PROCESS_IdToPDB( DWORD pid )
{
    if (!pid || pid == GetCurrentProcessId()) return PROCESS_Current();
    return NULL;
}


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
void PROCESS_CallUserSignalProc( UINT uCode, HMODULE hModule )
{
    DWORD flags = PROCESS_Current()->flags;
    DWORD startup_flags = PROCESS_Current()->env_db->startup_info->dwFlags;
    DWORD dwFlags = 0;

    /* Determine dwFlags */

    if ( !(flags & PDB32_WIN16_PROC) ) dwFlags |= USIG_FLAGS_WIN32;

    if ( !(flags & PDB32_CONSOLE_PROC) ) dwFlags |= USIG_FLAGS_GUI;

    if ( dwFlags & USIG_FLAGS_GUI )
    {
        /* Feedback defaults to ON */
        if ( !(startup_flags & STARTF_FORCEOFFFEEDBACK) )
            dwFlags |= USIG_FLAGS_FEEDBACK;
    }
    else
    {
        /* Feedback defaults to OFF */
        if (startup_flags & STARTF_FORCEONFEEDBACK)
            dwFlags |= USIG_FLAGS_FEEDBACK;
    }

    /* Convert module handle to 16-bit */

    if ( HIWORD( hModule ) )
        hModule = MapHModuleLS( hModule );

    /* Call USER signal proc */

    if ( Callout.UserSignalProc )
    {
        if ( uCode == USIG_THREAD_INIT || uCode == USIG_THREAD_EXIT )
            Callout.UserSignalProc( uCode, GetCurrentThreadId(), dwFlags, hModule );
        else
            Callout.UserSignalProc( uCode, GetCurrentProcessId(), dwFlags, hModule );
    }
}


/***********************************************************************
 *           process_init
 *
 * Main process initialisation code
 */
static BOOL process_init( char *argv[] )
{
    struct init_process_request *req;
    PDB *pdb = PROCESS_Current();

    /* store the program name */
    argv0 = argv[0];

    /* Fill the initial process structure */
    pdb->exit_code              = STILL_ACTIVE;
    pdb->threads                = 1;
    pdb->running_threads        = 1;
    pdb->ring0_threads          = 1;
    pdb->env_db                 = &initial_envdb;
    pdb->group                  = pdb;
    pdb->priority               = 8;  /* Normal */
    pdb->winver                 = 0xffff; /* to be determined */
    initial_envdb.startup_info  = &initial_startup;

    /* Setup the server connection */
    NtCurrentTeb()->socket = CLIENT_InitServer();
    if (CLIENT_InitThread()) return FALSE;

    /* Retrieve startup info from the server */
    req = get_req_buffer();
    req->ldt_copy  = ldt_copy;
    req->ldt_flags = ldt_flags_copy;
    req->ppid      = getppid();
    if (server_call( REQ_INIT_PROCESS )) return FALSE;
    main_exe_file               = req->exe_file;
    if (req->filename[0]) main_exe_name = strdup( req->filename );
    initial_startup.dwFlags     = req->start_flags;
    initial_startup.wShowWindow = req->cmd_show;
    initial_envdb.hStdin   = initial_startup.hStdInput  = req->hstdin;
    initial_envdb.hStdout  = initial_startup.hStdOutput = req->hstdout;
    initial_envdb.hStderr  = initial_startup.hStdError  = req->hstderr;

    /* Remember TEB selector of initial process for emergency use */
    SYSLEVEL_EmergencyTeb = NtCurrentTeb()->teb_sel;

    /* Create the system and process heaps */
    if (!HEAP_CreateSystemHeap()) return FALSE;
    pdb->heap = HeapCreate( HEAP_GROWABLE, 0, 0 );

    /* Copy the parent environment */
    if (!ENV_BuildEnvironment()) return FALSE;

    /* Create the SEGPTR heap */
    if (!(SegptrHeap = HeapCreate( HEAP_WINE_SEGPTR, 0, 0 ))) return FALSE;

    /* Initialize the critical sections */
    InitializeCriticalSection( &pdb->crit_section );
    InitializeCriticalSection( &initial_envdb.section );

    /* Initialize syslevel handling */
    SYSLEVEL_Init();

    /* Parse command line arguments */
    OPTIONS_ParseOptions( argv );

    return MAIN_MainInit();
}


/***********************************************************************
 *           load_system_dlls
 *
 * Load system DLLs into the initial process (and initialize them)
 */
static int load_system_dlls(void)
{
    char driver[MAX_PATH];

    PROFILE_GetWineIniString( "Wine", "GraphicsDriver", "x11drv", driver, sizeof(driver) );
    if (!LoadLibraryA( driver ))
    {
        MESSAGE( "Could not load graphics driver '%s'\n", driver );
        return 0;
    }

    if (!LoadLibraryA("USER32.DLL")) return 0;

    /* Get pointers to USER routines called by KERNEL */
    THUNK_InitCallout();

    /* Call FinalUserInit routine */
    Callout.FinalUserInit16();

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

    return 1;
}


/***********************************************************************
 *           build_command_line
 *
 * Build the command-line of a process from the argv array.
 */
static inline char *build_command_line( char **argv )
{
    int len, quote;
    char *cmdline, *p, **arg;

    for (arg = argv, len = 0; *arg; arg++) len += strlen(*arg) + 1;
    if ((quote = (strchr( argv[0], ' ' ) != NULL))) len += 2;
    if (!(p = cmdline = HeapAlloc( GetProcessHeap(), 0, len ))) return NULL;
    arg = argv;
    if (quote)
    {
        *p++ = '\"';
        strcpy( p, *arg );
        p += strlen(p);
        *p++ = '\"';
        *p++ = ' ';
        arg++;
    }
    while (*arg)
    {
        strcpy( p, *arg );
        p += strlen(p);
        *p++ = ' ';
        arg++;
    }
    if (p > cmdline) p--;  /* remove last space */
    *p = 0;
    return cmdline;
}


/***********************************************************************
 *           start_process
 *
 * Startup routine of a new process. Runs on the new process stack.
 */
static void start_process(void)
{
    struct init_process_done_request *req = get_req_buffer();
    int debugged, console_app;
    HMODULE16 hModule16;
    UINT cmdShow = SW_SHOWNORMAL;
    LPTHREAD_START_ROUTINE entry;
    PDB *pdb = PROCESS_Current();
    HMODULE module = pdb->exe_modref->module;

    /* Increment EXE refcount */
    pdb->exe_modref->refCount++;

    /* build command line */
    if (!(pdb->env_db->cmd_line = build_command_line( main_exe_argv ))) goto error;

    /* Retrieve entry point address */
    entry = (LPTHREAD_START_ROUTINE)RVA_PTR( module, OptionalHeader.AddressOfEntryPoint );
    console_app = (PE_HEADER(module)->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI);

    if (console_app) pdb->flags |= PDB32_CONSOLE_PROC;

    /* Signal the parent process to continue */
    req->module = (void *)module;
    req->entry  = entry;
    req->name   = &pdb->exe_modref->filename;
    req->gui    = !console_app;
    server_call( REQ_INIT_PROCESS_DONE );
    debugged = req->debugged;

    /* Install signal handlers; this cannot be done before, since we cannot
     * send exceptions to the debugger before the create process event that
     * is sent by REQ_INIT_PROCESS_DONE */
    if (!SIGNAL_Init()) goto error;

    /* Load KERNEL (necessary for TASK_Create) */
    if (!LoadLibraryA( "KERNEL32" )) goto error;

    /* Create 16-bit dummy module */
    if ((hModule16 = MODULE_CreateDummyModule( pdb->exe_modref->filename, module )) < 32)
        ExitProcess( hModule16 );

    if (pdb->env_db->startup_info->dwFlags & STARTF_USESHOWWINDOW)
        cmdShow = pdb->env_db->startup_info->wShowWindow;
    if (!TASK_Create( (NE_MODULE *)GlobalLock16( hModule16 ), cmdShow,
                      NtCurrentTeb(), NULL, 0 ))
        goto error;

    /* Load the system dlls */
    if (!load_system_dlls()) goto error;

    EnterCriticalSection( &pdb->crit_section );
    PE_InitTls();
    MODULE_DllProcessAttach( pdb->exe_modref, (LPVOID)1 );
    LeaveCriticalSection( &pdb->crit_section );

    /* Call UserSignalProc ( USIG_PROCESS_RUNNING ... ) only for non-GUI win32 apps */
    if (console_app) PROCESS_CallUserSignalProc( USIG_PROCESS_RUNNING, 0 );

    TRACE_(relay)( "Starting Win32 process (entryproc=%p)\n", entry );
    if (debugged) DbgBreakPoint();
    /* FIXME: should use _PEB as parameter for NT 3.5 programs !
     * Dunno about other OSs */
    ExitThread( entry(NULL) );

 error:
    ExitProcess( GetLastError() );
}


/***********************************************************************
 *           PROCESS_Start
 *
 * Startup routine of a new Win32 process once the main module has been loaded.
 * The filename is free'd by this routine.
 */
static void PROCESS_Start( HMODULE main_module, LPSTR filename ) WINE_NORETURN;
static void PROCESS_Start( HMODULE main_module, LPSTR filename )
{
    if (!filename)
    {
        /* if no explicit filename, use argv[0] */
        if (!(filename = malloc( MAX_PATH ))) ExitProcess(1);
        if (!GetLongPathNameA( full_argv0, filename, MAX_PATH ))
            lstrcpynA( filename, full_argv0, MAX_PATH );
    }

    /* load main module */
    if (PE_HEADER(main_module)->FileHeader.Characteristics & IMAGE_FILE_DLL)
        ExitProcess( ERROR_BAD_EXE_FORMAT );

    /* Create 32-bit MODREF */
    if (!PE_CreateModule( main_module, filename, 0, FALSE ))
        goto error;
    free( filename );

    /* allocate main thread stack */
    if (!THREAD_InitStack( NtCurrentTeb(),
                           PE_HEADER(main_module)->OptionalHeader.SizeOfStackReserve, TRUE ))
        goto error;

    /* switch to the new stack */
    SYSDEPS_SwitchToThreadStack( start_process );

 error:
    ExitProcess( GetLastError() );
}


/***********************************************************************
 *           PROCESS_InitWine
 *
 * Wine initialisation: load and start the main exe file.
 */
void PROCESS_InitWine( int argc, char *argv[] )
{
    DWORD type;

    /* Initialize everything */
    if (!process_init( argv )) exit(1);

    main_exe_argv = ++argv;  /* remove argv[0] (wine itself) */

    if (!main_exe_name)
    {
        char buffer[MAX_PATH];
        if (!argv[0]) OPTIONS_Usage();

        /* open the exe file */
        if (!SearchPathA( NULL, argv[0], ".exe", sizeof(buffer), buffer, NULL ) &&
            !SearchPathA( NULL, argv[0], NULL, sizeof(buffer), buffer, NULL ))
        {
            MESSAGE( "%s: cannot find '%s'\n", argv0, argv[0] );
            goto error;
        }
        if (!(main_exe_name = strdup(buffer)))
        {
            MESSAGE( "%s: out of memory\n", argv0 );
            ExitProcess(1);
        }
    }

    if (main_exe_file == INVALID_HANDLE_VALUE)
    {
        if ((main_exe_file = CreateFileA( main_exe_name, GENERIC_READ, FILE_SHARE_READ,
                                          NULL, OPEN_EXISTING, 0, -1 )) == INVALID_HANDLE_VALUE)
        {
            MESSAGE( "%s: cannot open '%s'\n", argv0, main_exe_name );
            goto error;
        }
    }

    if (!MODULE_GetBinaryType( main_exe_file, main_exe_name, &type ))
    {
        MESSAGE( "%s: unrecognized executable '%s'\n", argv0, main_exe_name );
        goto error;
    }

    switch (type)
    {
    case SCS_32BIT_BINARY:
        {
            HMODULE main_module = PE_LoadImage( main_exe_file, main_exe_name );
            if (main_module) PROCESS_Start( main_module, main_exe_name );
        }
        break;

    case SCS_WOW_BINARY:
        {
            HMODULE main_module;
            /* create 32-bit module for main exe */
            if (!(main_module = BUILTIN32_LoadExeModule())) goto error;
            NtCurrentTeb()->tibflags &= ~TEBF_WIN32;
            PROCESS_Current()->flags |= PDB32_WIN16_PROC;
            SYSLEVEL_EnterWin16Lock();
            PROCESS_Start( main_module, NULL );
        }
        break;

    case SCS_DOS_BINARY:
        FIXME( "DOS binaries support is broken at the moment; feel free to fix it...\n" );
        SetLastError( ERROR_BAD_FORMAT );
        break;

    case SCS_PIF_BINARY:
    case SCS_POSIX_BINARY:
    case SCS_OS216_BINARY:
    default:
        MESSAGE( "%s: unrecognized executable '%s'\n", argv0, main_exe_name );
        SetLastError( ERROR_BAD_FORMAT );
        break;
    }
 error:
    ExitProcess( GetLastError() );
}


/***********************************************************************
 *           PROCESS_InitWinelib
 *
 * Initialisation of a new Winelib process.
 */
void PROCESS_InitWinelib( int argc, char *argv[] )
{
    HMODULE main_module;

    if (!process_init( argv )) exit(1);

    /* create 32-bit module for main exe */
    if (!(main_module = BUILTIN32_LoadExeModule())) ExitProcess( GetLastError() );

    main_exe_argv = argv;
    PROCESS_Start( main_module, NULL );
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
    char **argv;
    int count = reserved + 1;
    char *p = cmdline;

    /* if first word is quoted store it as a single arg */
    if (*cmdline == '\"')
    {
        if ((p = strchr( cmdline + 1, '\"' )))
        {
            p++;
            count++;
        }
        else p = cmdline;
    }
    while (*p)
    {
        while (*p && isspace(*p)) p++;
        if (!*p) break;
        count++;
        while (*p && !isspace(*p)) p++;
    }

    if ((argv = malloc( count * sizeof(*argv) )))
    {
        char **argvptr = argv + reserved;
        p = cmdline;
        if (*cmdline == '\"')
        {
            if ((p = strchr( cmdline + 1, '\"' )))
            {
                *argvptr++ = cmdline + 1;
                *p++ = 0;
            }
            else p = cmdline;
        }
        while (*p)
        {
            while (*p && isspace(*p)) *p++ = 0;
            if (!*p) break;
            *argvptr++ = p;
            while (*p && !isspace(*p)) p++;
        }
        *argvptr = 0;
    }
    return argv;
}


/***********************************************************************
 *           build_envp
 *
 * Build the environment of a new child process.
 */
static char **build_envp( const char *env )
{
    const char *p;
    char **envp;
    int count;

    for (p = env, count = 0; *p; count++) p += strlen(p) + 1;
    count += 3;
    if ((envp = malloc( count * sizeof(*envp) )))
    {
        extern char **environ;
        char **envptr = envp;
        char **unixptr = environ;
        /* first put PATH, HOME and WINEPREFIX from the unix env */
        for (unixptr = environ; unixptr && *unixptr; unixptr++)
            if (!memcmp( *unixptr, "PATH=", 5 ) ||
                !memcmp( *unixptr, "HOME=", 5 ) ||
                !memcmp( *unixptr, "WINEPREFIX=", 11 )) *envptr++ = *unixptr;
        /* now put the Windows environment strings */
        for (p = env; *p; p += strlen(p) + 1)
        {
            if (memcmp( p, "PATH=", 5 ) &&
                memcmp( p, "HOME=", 5 ) &&
                memcmp( p, "WINEPREFIX=", 11 )) *envptr++ = (char *)p;
        }
        *envptr = 0;
    }
    return envp;
}


/***********************************************************************
 *           find_wine_binary
 *
 * Locate the Wine binary to exec for a new Win32 process.
 */
static void exec_wine_binary( char **argv, char **envp )
{
    const char *path, *pos, *ptr;

    /* first try bin directory */
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

    /* finally try the current directory */
    argv[0] = "./wine";
    execve( argv[0], argv, envp );
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

    if (pipe(fd) == -1)
    {
        FILE_SetDosError();
        return -1;
    }
    fcntl( fd[1], F_SETFD, 1 );  /* set close on exec */
    if (!(pid = fork()))  /* child */
    {
        char **argv = build_argv( cmdline, filename ? 0 : 2 );
        char **envp = build_envp( env );
        close( fd[0] );

	if (newdir) chdir(newdir);

        if (argv && envp)
        {
            if (!filename)
            {
                argv[1] = "--";
                exec_wine_binary( argv, envp );
            }
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
    return pid;
}


/***********************************************************************
 *           PROCESS_Create
 *
 * Create a new process. If hFile is a valid handle we have an exe
 * file, and we exec a new copy of wine to load it; otherwise we
 * simply exec the specified filename as a Unix process.
 */
BOOL PROCESS_Create( HFILE hFile, LPCSTR filename, LPSTR cmd_line, LPCSTR env, 
                     LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                     BOOL inherit, DWORD flags, LPSTARTUPINFOA startup,
                     LPPROCESS_INFORMATION info, LPCSTR lpCurrentDirectory )
{
    int pid;
    const char *unixfilename = NULL;
    const char *unixdir = NULL;
    DOS_FULL_NAME full_name;
    HANDLE load_done_evt = -1;
    struct new_process_request *req = get_req_buffer();
    struct wait_process_request *wait_req = get_req_buffer();

    info->hThread = info->hProcess = INVALID_HANDLE_VALUE;
    
    /* create the process on the server side */

    req->inherit_all  = inherit;
    req->create_flags = flags;
    req->start_flags  = startup->dwFlags;
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
    req->cmd_show = startup->wShowWindow;
    req->alloc_fd = 0;

    if (lpCurrentDirectory) {
        if (DOSFS_GetFullName( lpCurrentDirectory, TRUE, &full_name ))
	    unixdir = full_name.long_name;
    } else {
	CHAR	buf[260];
	if (GetCurrentDirectoryA(sizeof(buf),buf)) {
	    if (DOSFS_GetFullName( buf, TRUE, &full_name ))
		unixdir = full_name.long_name;
	}
    }

    if (hFile == -1)  /* unix process */
    {
        unixfilename = filename;
        if (DOSFS_GetFullName( filename, TRUE, &full_name )) unixfilename = full_name.long_name;
        req->filename[0] = 0;
    }
    else  /* new wine process */
    {
        if (!GetLongPathNameA( filename, req->filename, server_remaining(req->filename) ))
            lstrcpynA( req->filename, filename, server_remaining(req->filename) );
    }
    if (server_call( REQ_NEW_PROCESS )) return FALSE;

    /* fork and execute */

    pid = fork_and_exec( unixfilename, cmd_line, env ? env : GetEnvironmentStringsA(), unixdir );

    wait_req->cancel   = (pid == -1);
    wait_req->pinherit = (psa && (psa->nLength >= sizeof(*psa)) && psa->bInheritHandle);
    wait_req->tinherit = (tsa && (tsa->nLength >= sizeof(*tsa)) && tsa->bInheritHandle);
    wait_req->timeout  = 2000;
    if (server_call( REQ_WAIT_PROCESS ) || (pid == -1)) goto error;
    info->dwProcessId = (DWORD)wait_req->pid;
    info->dwThreadId  = (DWORD)wait_req->tid;
    info->hProcess    = wait_req->phandle;
    info->hThread     = wait_req->thandle;
    load_done_evt     = wait_req->event;

    /* Wait until process is initialized (or initialization failed) */
    if (load_done_evt != -1)
    {
        DWORD res;
        HANDLE handles[2];

        handles[0] = info->hProcess;
        handles[1] = load_done_evt;
        res = WaitForMultipleObjects( 2, handles, FALSE, INFINITE );
        CloseHandle( load_done_evt );
        if (res == STATUS_WAIT_0)  /* the process died */
        {
            DWORD exitcode;
            if (GetExitCodeProcess( info->hProcess, &exitcode )) SetLastError( exitcode );
            CloseHandle( info->hThread );
            CloseHandle( info->hProcess );
            return FALSE;
        }
    }
    return TRUE;

error:
    if (load_done_evt != -1) CloseHandle( load_done_evt );
    if (info->hThread != INVALID_HANDLE_VALUE) CloseHandle( info->hThread );
    if (info->hProcess != INVALID_HANDLE_VALUE) CloseHandle( info->hProcess );
    return FALSE;
}


/***********************************************************************
 *           ExitProcess   (KERNEL32.100)
 */
void WINAPI ExitProcess( DWORD status )
{
    struct terminate_process_request *req = get_req_buffer();

    MODULE_DllProcessDetach( TRUE, (LPVOID)1 );
    /* send the exit code to the server */
    req->handle    = GetCurrentProcess();
    req->exit_code = status;
    server_call( REQ_TERMINATE_PROCESS );
    exit( status );
}

/***********************************************************************
 *           ExitProcess16   (KERNEL.466)
 */
void WINAPI ExitProcess16( WORD status )
{
    SYSLEVEL_ReleaseWin16Lock();
    ExitProcess( status );
}

/******************************************************************************
 *           TerminateProcess   (KERNEL32.684)
 */
BOOL WINAPI TerminateProcess( HANDLE handle, DWORD exit_code )
{
    BOOL ret;
    struct terminate_process_request *req = get_req_buffer();
    req->handle    = handle;
    req->exit_code = exit_code;
    if ((ret = !server_call( REQ_TERMINATE_PROCESS )) && req->self) exit( exit_code );
    return ret;
}


/***********************************************************************
 *           GetProcessDword    (KERNEL32.18) (KERNEL.485)
 * 'Of course you cannot directly access Windows internal structures'
 */
DWORD WINAPI GetProcessDword( DWORD dwProcessID, INT offset )
{
    PDB *process = PROCESS_IdToPDB( dwProcessID );
    TDB *pTask;
    DWORD x, y;

    TRACE_(win32)("(%ld, %d)\n", dwProcessID, offset );
    if ( !process )
    {
        ERR("%d: process %lx not accessible\n", offset, dwProcessID);
        return 0;
    }

    switch ( offset ) 
    {
    case GPD_APP_COMPAT_FLAGS:
        pTask = (TDB *)GlobalLock16( GetCurrentTask() );
        return pTask? pTask->compat_flags : 0;

    case GPD_LOAD_DONE_EVENT:
        return process->load_done_evt;

    case GPD_HINSTANCE16:
        pTask = (TDB *)GlobalLock16( GetCurrentTask() );
        return pTask? pTask->hInstance : 0;

    case GPD_WINDOWS_VERSION:
        pTask = (TDB *)GlobalLock16( GetCurrentTask() );
        return pTask? pTask->version : 0;

    case GPD_THDB:
        if ( process != PROCESS_Current() ) return 0;
        return (DWORD)NtCurrentTeb() - 0x10 /* FIXME */;

    case GPD_PDB:
        return (DWORD)process;

    case GPD_STARTF_SHELLDATA: /* return stdoutput handle from startupinfo ??? */
        return process->env_db->startup_info->hStdOutput;

    case GPD_STARTF_HOTKEY: /* return stdinput handle from startupinfo ??? */
        return process->env_db->startup_info->hStdInput;

    case GPD_STARTF_SHOWWINDOW:
        return process->env_db->startup_info->wShowWindow;

    case GPD_STARTF_SIZE:
        x = process->env_db->startup_info->dwXSize;
        if ( x == CW_USEDEFAULT ) x = CW_USEDEFAULT16;
        y = process->env_db->startup_info->dwYSize;
        if ( y == CW_USEDEFAULT ) y = CW_USEDEFAULT16;
        return MAKELONG( x, y );

    case GPD_STARTF_POSITION:
        x = process->env_db->startup_info->dwX;
        if ( x == CW_USEDEFAULT ) x = CW_USEDEFAULT16;
        y = process->env_db->startup_info->dwY;
        if ( y == CW_USEDEFAULT ) y = CW_USEDEFAULT16;
        return MAKELONG( x, y );

    case GPD_STARTF_FLAGS:
        return process->env_db->startup_info->dwFlags;

    case GPD_PARENT:
        return 0;

    case GPD_FLAGS:
        return process->flags;

    case GPD_USERDATA:
        return process->process_dword;

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
    PDB *process = PROCESS_IdToPDB( dwProcessID );

    TRACE_(win32)("(%ld, %d)\n", dwProcessID, offset );
    if ( !process )
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
        process->process_dword = value; 
        break;

    default:
        ERR_(win32)("Unknown offset %d\n", offset );
        break;
    }
}


/*********************************************************************
 *           OpenProcess   (KERNEL32.543)
 */
HANDLE WINAPI OpenProcess( DWORD access, BOOL inherit, DWORD id )
{
    HANDLE ret = 0;
    struct open_process_request *req = get_req_buffer();

    req->pid     = (void *)id;
    req->access  = access;
    req->inherit = inherit;
    if (!server_call( REQ_OPEN_PROCESS )) ret = req->handle;
    return ret;
}			      

/*********************************************************************
 *           MapProcessHandle   (KERNEL.483)
 */
DWORD WINAPI MapProcessHandle( HANDLE handle )
{
    DWORD ret = 0;
    struct get_process_info_request *req = get_req_buffer();
    req->handle = handle;
    if (!server_call( REQ_GET_PROCESS_INFO )) ret = (DWORD)req->pid;
    return ret;
}

/***********************************************************************
 *           SetPriorityClass   (KERNEL32.503)
 */
BOOL WINAPI SetPriorityClass( HANDLE hprocess, DWORD priorityclass )
{
    struct set_process_info_request *req = get_req_buffer();
    req->handle   = hprocess;
    req->priority = priorityclass;
    req->mask     = SET_PROCESS_INFO_PRIORITY;
    return !server_call( REQ_SET_PROCESS_INFO );
}


/***********************************************************************
 *           GetPriorityClass   (KERNEL32.250)
 */
DWORD WINAPI GetPriorityClass(HANDLE hprocess)
{
    DWORD ret = 0;
    struct get_process_info_request *req = get_req_buffer();
    req->handle = hprocess;
    if (!server_call( REQ_GET_PROCESS_INFO )) ret = req->priority;
    return ret;
}


/***********************************************************************
 *          SetProcessAffinityMask   (KERNEL32.662)
 */
BOOL WINAPI SetProcessAffinityMask( HANDLE hProcess, DWORD affmask )
{
    struct set_process_info_request *req = get_req_buffer();
    req->handle   = hProcess;
    req->affinity = affmask;
    req->mask     = SET_PROCESS_INFO_AFFINITY;
    return !server_call( REQ_SET_PROCESS_INFO );
}

/**********************************************************************
 *          GetProcessAffinityMask    (KERNEL32.373)
 */
BOOL WINAPI GetProcessAffinityMask( HANDLE hProcess,
                                      LPDWORD lpProcessAffinityMask,
                                      LPDWORD lpSystemAffinityMask )
{
    BOOL ret = FALSE;
    struct get_process_info_request *req = get_req_buffer();
    req->handle = hProcess;
    if (!server_call( REQ_GET_PROCESS_INFO ))
    {
        if (lpProcessAffinityMask) *lpProcessAffinityMask = req->process_affinity;
        if (lpSystemAffinityMask) *lpSystemAffinityMask = req->system_affinity;
        ret = TRUE;
    }
    return ret;
}


/***********************************************************************
 *           GetStdHandle    (KERNEL32.276)
 */
HANDLE WINAPI GetStdHandle( DWORD std_handle )
{
    PDB *pdb = PROCESS_Current();

    switch(std_handle)
    {
    case STD_INPUT_HANDLE:  return pdb->env_db->hStdin;
    case STD_OUTPUT_HANDLE: return pdb->env_db->hStdout;
    case STD_ERROR_HANDLE:  return pdb->env_db->hStderr;
    }
    SetLastError( ERROR_INVALID_PARAMETER );
    return INVALID_HANDLE_VALUE;
}


/***********************************************************************
 *           SetStdHandle    (KERNEL32.506)
 */
BOOL WINAPI SetStdHandle( DWORD std_handle, HANDLE handle )
{
    PDB *pdb = PROCESS_Current();
    /* FIXME: should we close the previous handle? */
    switch(std_handle)
    {
    case STD_INPUT_HANDLE:
        pdb->env_db->hStdin = handle;
        return TRUE;
    case STD_OUTPUT_HANDLE:
        pdb->env_db->hStdout = handle;
        return TRUE;
    case STD_ERROR_HANDLE:
        pdb->env_db->hStderr = handle;
        return TRUE;
    }
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}

/***********************************************************************
 *           GetProcessVersion    (KERNEL32)
 */
DWORD WINAPI GetProcessVersion( DWORD processid )
{
    TDB *pTask;
    PDB *pdb = PROCESS_IdToPDB( processid );

    if (!pdb) return 0;
    if (!(pTask = (TDB *)GlobalLock16( pdb->task ))) return 0;
    return (pTask->version&0xff) | (((pTask->version >>8) & 0xff)<<16);
}

/***********************************************************************
 *           GetProcessFlags    (KERNEL32)
 */
DWORD WINAPI GetProcessFlags( DWORD processid )
{
    PDB *pdb = PROCESS_IdToPDB( processid );
    if (!pdb) return 0;
    return pdb->flags;
}

/***********************************************************************
 *		SetProcessWorkingSetSize	[KERNEL32.662]
 * Sets the min/max working set sizes for a specified process.
 *
 * PARAMS
 *    hProcess [I] Handle to the process of interest
 *    minset   [I] Specifies minimum working set size
 *    maxset   [I] Specifies maximum working set size
 *
 * RETURNS  STD
 */
BOOL WINAPI SetProcessWorkingSetSize(HANDLE hProcess,DWORD minset,
                                       DWORD maxset)
{
    FIXME("(0x%08x,%ld,%ld): stub - harmless\n",hProcess,minset,maxset);
    if(( minset == -1) && (maxset == -1)) {
        /* Trim the working set to zero */
        /* Swap the process out of physical RAM */
    }
    return TRUE;
}

/***********************************************************************
 *           GetProcessWorkingSetSize    (KERNEL32)
 */
BOOL WINAPI GetProcessWorkingSetSize(HANDLE hProcess,LPDWORD minset,
                                       LPDWORD maxset)
{
	FIXME("(0x%08x,%p,%p): stub\n",hProcess,minset,maxset);
	/* 32 MB working set size */
	if (minset) *minset = 32*1024*1024;
	if (maxset) *maxset = 32*1024*1024;
	return TRUE;
}

/***********************************************************************
 *           SetProcessShutdownParameters    (KERNEL32)
 *
 * CHANGED - James Sutherland (JamesSutherland@gmx.de)
 * Now tracks changes made (but does not act on these changes)
 * NOTE: the definition for SHUTDOWN_NORETRY was done on guesswork.
 * It really shouldn't be here, but I'll move it when it's been checked!
 */
#define SHUTDOWN_NORETRY 1
static unsigned int shutdown_noretry = 0;
static unsigned int shutdown_priority = 0x280L;
BOOL WINAPI SetProcessShutdownParameters(DWORD level,DWORD flags)
{
    if (flags & SHUTDOWN_NORETRY)
      shutdown_noretry = 1;
    else
      shutdown_noretry = 0;
    if (level > 0x100L && level < 0x3FFL)
      shutdown_priority = level;
    else
      {
	ERR("invalid priority level 0x%08lx\n", level);
	return FALSE;
      }
    return TRUE;
}


/***********************************************************************
 * GetProcessShutdownParameters                 (KERNEL32)
 *
 */
BOOL WINAPI GetProcessShutdownParameters( LPDWORD lpdwLevel,
					    LPDWORD lpdwFlags )
{
  (*lpdwLevel) = shutdown_priority;
  (*lpdwFlags) = (shutdown_noretry * SHUTDOWN_NORETRY);
  return TRUE;
}
/***********************************************************************
 *           SetProcessPriorityBoost    (KERNEL32)
 */
BOOL WINAPI SetProcessPriorityBoost(HANDLE hprocess,BOOL disableboost)
{
    FIXME("(%d,%d): stub\n",hprocess,disableboost);
    /* Say we can do it. I doubt the program will notice that we don't. */
    return TRUE;
}


/***********************************************************************
 *           ReadProcessMemory    		(KERNEL32)
 */
BOOL WINAPI ReadProcessMemory( HANDLE process, LPCVOID addr, LPVOID buffer, DWORD size,
                               LPDWORD bytes_read )
{
    struct read_process_memory_request *req = get_req_buffer();
    unsigned int offset = (unsigned int)addr % sizeof(int);
    unsigned int max = server_remaining( req->data );  /* max length in one request */
    unsigned int pos;

    if (bytes_read) *bytes_read = size;

    /* first time, read total length to check for permissions */
    req->handle = process;
    req->addr   = (char *)addr - offset;
    req->len    = (size + offset + sizeof(int) - 1) / sizeof(int);
    if (server_call( REQ_READ_PROCESS_MEMORY )) goto error;

    if (size <= max - offset)
    {
        memcpy( buffer, (char *)req->data + offset, size );
        return TRUE;
    }

    /* now take care of the remaining data */
    memcpy( buffer, (char *)req->data + offset, max - offset );
    pos = max - offset;
    size -= pos;
    while (size)
    {
        if (max > size) max = size;
        req->handle = process;
        req->addr   = (char *)addr + pos;
        req->len    = (max + sizeof(int) - 1) / sizeof(int);
        if (server_call( REQ_READ_PROCESS_MEMORY )) goto error;
        memcpy( (char *)buffer + pos, (char *)req->data, max );
        size -= max;
        pos += max;
    }
    return TRUE;

 error:
    if (bytes_read) *bytes_read = 0;
    return FALSE;
}


/***********************************************************************
 *           WriteProcessMemory    		(KERNEL32)
 */
BOOL WINAPI WriteProcessMemory( HANDLE process, LPVOID addr, LPVOID buffer, DWORD size,
                                LPDWORD bytes_written )
{
    unsigned int first_offset, last_offset;
    struct write_process_memory_request *req = get_req_buffer();
    unsigned int max = server_remaining( req->data );  /* max length in one request */
    unsigned int pos, last_mask;

    if (!size)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (bytes_written) *bytes_written = size;

    /* compute the mask for the first int */
    req->first_mask = ~0;
    first_offset = (unsigned int)addr % sizeof(int);
    memset( &req->first_mask, 0, first_offset );

    /* compute the mask for the last int */
    last_offset = (size + first_offset) % sizeof(int);
    last_mask = 0;
    memset( &last_mask, 0xff, last_offset ? last_offset : sizeof(int) );

    req->handle = process;
    req->addr = (char *)addr - first_offset;
    /* for the first request, use the total length */
    req->len = (size + first_offset + sizeof(int) - 1) / sizeof(int);

    if (size + first_offset < max)  /* we can do it in one round */
    {
        memcpy( (char *)req->data + first_offset, buffer, size );
        req->last_mask = last_mask;
        if (server_call( REQ_WRITE_PROCESS_MEMORY )) goto error;
        return TRUE;
    }

    /* needs multiple server calls */

    memcpy( (char *)req->data + first_offset, buffer, max - first_offset );
    req->last_mask = ~0;
    if (server_call( REQ_WRITE_PROCESS_MEMORY )) goto error;
    pos = max - first_offset;
    size -= pos;
    while (size)
    {
        if (size <= max)  /* last one */
        {
            req->last_mask = last_mask;
            max = size;
        }
        req->handle = process;
        req->addr = (char *)addr + pos;
        req->len = (max + sizeof(int) - 1) / sizeof(int);
        req->first_mask = ~0;
        memcpy( req->data, (char *) buffer + pos, max );
        if (server_call( REQ_WRITE_PROCESS_MEMORY )) goto error;
        pos += max;
        size -= max;
    }
    return TRUE;

 error:
    if (bytes_written) *bytes_written = 0;
    return FALSE;

}


/***********************************************************************
 *           RegisterServiceProcess             (KERNEL, KERNEL32)
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
 * GetExitCodeProcess [KERNEL32.325]
 *
 * Gets termination status of specified process
 * 
 * RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 */
BOOL WINAPI GetExitCodeProcess(
    HANDLE hProcess,  /* [I] handle to the process */
    LPDWORD lpExitCode) /* [O] address to receive termination status */
{
    BOOL ret = FALSE;
    struct get_process_info_request *req = get_req_buffer();
    req->handle = hProcess;
    if (!server_call( REQ_GET_PROCESS_INFO ))
    {
        if (lpExitCode) *lpExitCode = req->exit_code;
        ret = TRUE;
    }
    return ret;
}


/***********************************************************************
 *           SetErrorMode   (KERNEL32.486)
 */
UINT WINAPI SetErrorMode( UINT mode )
{
    UINT old = PROCESS_Current()->error_mode;
    PROCESS_Current()->error_mode = mode;
    return old;
}

/***********************************************************************
 *           GetCurrentProcess   (KERNEL32.198)
 */
#undef GetCurrentProcess
HANDLE WINAPI GetCurrentProcess(void)
{
    return 0xffffffff;
}
