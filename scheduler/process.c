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
#include "wine/library.h"
#include "drive.h"
#include "module.h"
#include "file.h"
#include "thread.h"
#include "winerror.h"
#include "server.h"
#include "options.h"
#include "callback.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(process);
DECLARE_DEBUG_CHANNEL(relay);
DECLARE_DEBUG_CHANNEL(win32);

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

static char **main_exe_argv;
static char main_exe_name[MAX_PATH];
static HANDLE main_exe_file;

unsigned int server_startticks;

/* memory/environ.c */
extern struct _ENVDB *ENV_BuildEnvironment(void);
extern BOOL ENV_BuildCommandLine( char **argv );
extern STARTUPINFOA current_startupinfo;

/* scheduler/pthread.c */
extern void PTHREAD_init_done(void);

extern BOOL MAIN_MainInit(void);


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
    DWORD flags = current_process.flags;
    DWORD startup_flags = current_startupinfo.dwFlags;
    DWORD dwFlags = 0;

    if (!Callout.UserSignalProc) return;

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

    /* Call USER signal proc */

    if ( uCode == USIG_THREAD_INIT || uCode == USIG_THREAD_EXIT )
        Callout.UserSignalProc( uCode, GetCurrentThreadId(), dwFlags, hModule );
    else
        Callout.UserSignalProc( uCode, GetCurrentProcessId(), dwFlags, hModule );
}


/***********************************************************************
 *           set_console_handles
 *
 * Set the console handles to use stdin/stdout.
 */
static void set_console_handles( HANDLE console )
{
    wine_server_send_fd( 0 );
    wine_server_send_fd( 1 );

    SERVER_START_REQ( set_console_fd )
    {
        req->handle = console;
        req->fd_in  = 0;
        req->fd_out = 1;
        req->pid    = 0;
        SERVER_CALL();
    }
    SERVER_END_REQ;
}


/***********************************************************************
 *           process_init
 *
 * Main process initialisation code
 */
static BOOL process_init( char *argv[] )
{
    BOOL ret;
    int create_flags = 0;

    /* store the program name */
    argv0 = argv[0];
    main_exe_argv = argv;

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
    SERVER_START_VAR_REQ( init_process, sizeof(main_exe_name)-1 )
    {
        req->ldt_copy  = &wine_ldt_copy;
        req->ppid      = getppid();
        if ((ret = !SERVER_CALL_ERR()))
        {
            size_t len = server_data_size( req );
            memcpy( main_exe_name, server_data_ptr(req), len );
            main_exe_name[len] = 0;
            main_exe_file = req->exe_file;
            create_flags  = req->create_flags;
            current_startupinfo.dwFlags     = req->start_flags;
            server_startticks               = req->server_start;
            current_startupinfo.wShowWindow = req->cmd_show;
            current_startupinfo.hStdInput   = req->hstdin;
            current_startupinfo.hStdOutput  = req->hstdout;
            current_startupinfo.hStdError   = req->hstderr;
        }
    }
    SERVER_END_VAR_REQ;
    if (!ret) return FALSE;

    SetStdHandle( STD_INPUT_HANDLE,  current_startupinfo.hStdInput );
    SetStdHandle( STD_OUTPUT_HANDLE, current_startupinfo.hStdOutput );
    SetStdHandle( STD_ERROR_HANDLE,  current_startupinfo.hStdError );
    if (create_flags & CREATE_NEW_CONSOLE)
        set_console_handles( current_startupinfo.hStdOutput );

    /* Create the process heap */
    current_process.heap = HeapCreate( HEAP_GROWABLE, 0, 0 );

    /* Now we can use the pthreads routines */
    PTHREAD_init_done();

    /* Copy the parent environment */
    if (!(current_process.env_db = ENV_BuildEnvironment())) return FALSE;

    /* Parse command line arguments */
    OPTIONS_ParseOptions( argv );

    return MAIN_MainInit();
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

    /* use original argv[0] as name for the main module */
    if (!main_exe_name[0])
    {
        if (!GetLongPathNameA( full_argv0, main_exe_name, sizeof(main_exe_name) ))
            lstrcpynA( main_exe_name, full_argv0, sizeof(main_exe_name) );
    }

    /* Retrieve entry point address */
    entry = (LPTHREAD_START_ROUTINE)((char*)current_process.module +
                         PE_HEADER(current_process.module)->OptionalHeader.AddressOfEntryPoint);
    console_app = (PE_HEADER(current_process.module)->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI);
    if (console_app) current_process.flags |= PDB32_CONSOLE_PROC;

    /* Signal the parent process to continue */
    SERVER_START_REQ( init_process_done )
    {
        req->module   = (void *)current_process.module;
        req->entry    = entry;
        req->name     = main_exe_name;
        req->exe_file = main_exe_file;
        req->gui      = !console_app;
        SERVER_CALL();
        debugged = req->debugged;
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

    RtlAcquirePebLock();
    PE_InitTls();
    MODULE_DllProcessAttach( NULL, (LPVOID)1 );
    RtlReleasePebLock();

    /* Get pointers to USER routines called by KERNEL */
    THUNK_InitCallout();

    /* Call FinalUserInit routine */
    if (Callout.FinalUserInit16) Callout.FinalUserInit16();

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
 *           open_winelib_app
 *
 * Try to open the Winelib app .so file based on argv[0] or WINEPRELOAD.
 */
void *open_winelib_app( char *argv[] )
{
    void *ret = NULL;
    char *tmp;
    const char *name;
    char errStr[100];

    if ((name = getenv( "WINEPRELOAD" )))
    {
        if (!(ret = wine_dll_load_main_exe( name, 0, errStr, sizeof(errStr) )))
        {
            MESSAGE( "%s: could not load '%s' as specified in the WINEPRELOAD environment variable: %s\n",
                     argv[0], name, errStr );
            ExitProcess(1);
        }
    }
    else
    {
        const char *argv0 = main_exe_name;
        if (!*argv0)
        {
            /* if argv[0] is "wine", don't try to load anything */
            argv0 = argv[0];
            if (!(name = strrchr( argv0, '/' ))) name = argv0;
            else name++;
            if (!strcmp( name, "wine" )) return NULL;
        }

        /* now try argv[0] with ".so" appended */
        if ((tmp = HeapAlloc( GetProcessHeap(), 0, strlen(argv0) + 4 )))
        {
            strcpy( tmp, argv0 );
            strcat( tmp, ".so" );
            /* search in PATH only if there was no '/' in argv[0] */
            ret = wine_dll_load_main_exe( tmp, (name == argv0), errStr, sizeof(errStr) );
            if (!ret && !argv[1])
            {
                /* if no argv[1], this will be better than displaying usage */
                MESSAGE( "%s: could not load library '%s' as Winelib application: %s\n",
                         argv[0], tmp, errStr );
                ExitProcess(1);
            }
            HeapFree( GetProcessHeap(), 0, tmp );
        }
    }
    return ret;
}

/***********************************************************************
 *           PROCESS_InitWine
 *
 * Wine initialisation: load and start the main exe file.
 */
void PROCESS_InitWine( int argc, char *argv[], LPSTR win16_exe_name, HANDLE *win16_exe_file )
{
    DWORD stack_size = 0;

    /* Initialize everything */
    if (!process_init( argv )) exit(1);

    if (open_winelib_app( argv )) goto found; /* try to open argv[0] as a winelib app */

    main_exe_argv = ++argv;  /* remove argv[0] (wine itself) */

    if (!main_exe_name[0])
    {
        if (!argv[0]) OPTIONS_Usage();

        /* open the exe file */
        if (!SearchPathA( NULL, argv[0], ".exe", sizeof(main_exe_name), main_exe_name, NULL ) &&
            !SearchPathA( NULL, argv[0], NULL, sizeof(main_exe_name), main_exe_name, NULL ))
        {
            MESSAGE( "%s: cannot find '%s'\n", argv0, argv[0] );
            goto error;
        }
    }

    if (!main_exe_file)
    {
        if ((main_exe_file = CreateFileA( main_exe_name, GENERIC_READ, FILE_SHARE_READ,
                                          NULL, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)
        {
            MESSAGE( "%s: cannot open '%s'\n", argv0, main_exe_name );
            goto error;
        }
    }

    /* first try Win32 format; this will fail if the file is not a PE binary */
    if ((current_process.module = PE_LoadImage( main_exe_file, main_exe_name, 0 )))
    {
        if (PE_HEADER(current_process.module)->FileHeader.Characteristics & IMAGE_FILE_DLL)
            ExitProcess( ERROR_BAD_EXE_FORMAT );
        goto found;
    }

    /* it must be 16-bit or DOS format */
    NtCurrentTeb()->tibflags &= ~TEBF_WIN32;
    current_process.flags |= PDB32_WIN16_PROC;
    strcpy( win16_exe_name, main_exe_name );
    main_exe_name[0] = 0;
    *win16_exe_file = main_exe_file;
    main_exe_file = 0;
    _EnterWin16Lock();

 found:
    /* build command line */
    if (!ENV_BuildCommandLine( main_exe_argv )) goto error;

    /* create 32-bit module for main exe */
    if (!(current_process.module = BUILTIN32_LoadExeModule( current_process.module ))) goto error;
    stack_size = PE_HEADER(current_process.module)->OptionalHeader.SizeOfStackReserve;

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
            if (memcmp( p, "PATH=", 5 ) &&
                memcmp( p, "HOME=", 5 ) &&
                memcmp( p, "WINEPRELOAD=", 12 ) &&
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
        char **argv = build_argv( cmdline, filename ? 0 : 2 );
        char **envp = build_envp( env, extra_env );
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
    if (extra_env) HeapFree( GetProcessHeap(), 0, extra_env );
    return pid;
}


/***********************************************************************
 *           PROCESS_Create
 *
 * Create a new process. If hFile is a valid handle we have an exe
 * file, and we exec a new copy of wine to load it; otherwise we
 * simply exec the specified filename as a Unix process.
 */
BOOL PROCESS_Create( HANDLE hFile, LPCSTR filename, LPSTR cmd_line, LPCSTR env,
                     LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                     BOOL inherit, DWORD flags, LPSTARTUPINFOA startup,
                     LPPROCESS_INFORMATION info, LPCSTR lpCurrentDirectory )
{
    BOOL ret;
    int pid;
    const char *unixfilename = NULL;
    const char *unixdir = NULL;
    DOS_FULL_NAME full_dir, full_name;
    HANDLE load_done_evt = 0;
    HANDLE process_info;

    info->hThread = info->hProcess = 0;

    if (lpCurrentDirectory)
    {
        if (DOSFS_GetFullName( lpCurrentDirectory, TRUE, &full_dir ))
            unixdir = full_dir.long_name;
    }
    else
    {
        char buf[MAX_PATH];
        if (GetCurrentDirectoryA(sizeof(buf),buf))
        {
            if (DOSFS_GetFullName( buf, TRUE, &full_dir ))
                unixdir = full_dir.long_name;
        }
    }

    /* create the process on the server side */

    SERVER_START_VAR_REQ( new_process, MAX_PATH )
    {
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

        if (!hFile)  /* unix process */
        {
            unixfilename = filename;
            if (DOSFS_GetFullName( filename, TRUE, &full_name ))
                unixfilename = full_name.long_name;
            lstrcpynA( server_data_ptr(req), unixfilename, MAX_PATH );
        }
        else  /* new wine process */
        {
            if (!GetLongPathNameA( filename, server_data_ptr(req), MAX_PATH ))
                lstrcpynA( server_data_ptr(req), filename, MAX_PATH );
        }
        ret = !SERVER_CALL_ERR();
        process_info = req->info;
    }
    SERVER_END_VAR_REQ;
    if (!ret) return FALSE;

    /* fork and execute */

    pid = fork_and_exec( unixfilename, cmd_line, env, unixdir );

    /* wait for the new process info to be ready */

    ret = FALSE;
    if ((pid != -1) && (WaitForSingleObject( process_info, 2000 ) == STATUS_WAIT_0))
    {
        SERVER_START_REQ( get_new_process_info )
        {
            req->info     = process_info;
            req->pinherit = (psa && (psa->nLength >= sizeof(*psa)) && psa->bInheritHandle);
            req->tinherit = (tsa && (tsa->nLength >= sizeof(*tsa)) && tsa->bInheritHandle);
            if ((ret = !SERVER_CALL_ERR()))
            {
                info->dwProcessId = (DWORD)req->pid;
                info->dwThreadId  = (DWORD)req->tid;
                info->hProcess    = req->phandle;
                info->hThread     = req->thandle;
                load_done_evt     = req->event;
            }
        }
        SERVER_END_REQ;
    }
    CloseHandle( process_info );
    if (!ret) goto error;

    /* Wait until process is initialized (or initialization failed) */
    if (load_done_evt)
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
    if (load_done_evt) CloseHandle( load_done_evt );
    if (info->hThread) CloseHandle( info->hThread );
    if (info->hProcess) CloseHandle( info->hProcess );
    return FALSE;
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
        SERVER_CALL();
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
        return current_process.load_done_evt;

    case GPD_HINSTANCE16:
        return GetTaskDS16();

    case GPD_WINDOWS_VERSION:
        return GetExeVersion16();

    case GPD_THDB:
        return (DWORD)NtCurrentTeb() - 0x10 /* FIXME */;

    case GPD_PDB:
        return (DWORD)&current_process;

    case GPD_STARTF_SHELLDATA: /* return stdoutput handle from startupinfo ??? */
        return current_startupinfo.hStdOutput;

    case GPD_STARTF_HOTKEY: /* return stdinput handle from startupinfo ??? */
        return current_startupinfo.hStdInput;

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
        req->pid     = (void *)id;
        req->access  = access;
        req->inherit = inherit;
        if (!SERVER_CALL_ERR()) ret = req->handle;
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
        if (!SERVER_CALL_ERR()) ret = (DWORD)req->pid;
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
        ret = !SERVER_CALL_ERR();
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
        if (!SERVER_CALL_ERR()) ret = req->priority;
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
        ret = !SERVER_CALL_ERR();
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
        if (!SERVER_CALL_ERR())
        {
            if (lpProcessAffinityMask) *lpProcessAffinityMask = req->process_affinity;
            if (lpSystemAffinityMask) *lpSystemAffinityMask = req->system_affinity;
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
BOOL WINAPI SetProcessWorkingSetSize(HANDLE hProcess,DWORD minset,
                                       DWORD maxset)
{
    FIXME("(0x%08x,%ld,%ld): stub - harmless\n",hProcess,minset,maxset);
    if(( minset == (DWORD)-1) && (maxset == (DWORD)-1)) {
        /* Trim the working set to zero */
        /* Swap the process out of physical RAM */
    }
    return TRUE;
}

/***********************************************************************
 *           GetProcessWorkingSetSize    (KERNEL32.@)
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
 *		ReadProcessMemory (WIN32S16.6)
 */
BOOL WINAPI ReadProcessMemory( HANDLE process, LPCVOID addr, LPVOID buffer, DWORD size,
                               LPDWORD bytes_read )
{
    unsigned int offset = (unsigned int)addr % sizeof(int);
    unsigned int pos = 0, len, max;
    int res;

    if (bytes_read) *bytes_read = size;

    /* first time, read total length to check for permissions */
    len = (size + offset + sizeof(int) - 1) / sizeof(int);
    max = min( REQUEST_MAX_VAR_SIZE, len * sizeof(int) );

    for (;;)
    {
        SERVER_START_VAR_REQ( read_process_memory, max )
        {
            req->handle = process;
            req->addr   = (char *)addr + pos - offset;
            req->len    = len;
            if (!(res = SERVER_CALL_ERR()))
            {
                size_t result = server_data_size( req );
                if (result > size + offset) result = size + offset;
                memcpy( (char *)buffer + pos, server_data_ptr(req) + offset, result - offset );
                size -= result - offset;
                pos += result - offset;
            }
        }
        SERVER_END_VAR_REQ;
        if (res)
        {
            if (bytes_read) *bytes_read = 0;
            return FALSE;
        }
        if (!size) return TRUE;
        max = min( REQUEST_MAX_VAR_SIZE, size );
        len = (max + sizeof(int) - 1) / sizeof(int);
        offset = 0;
    }
}


/***********************************************************************
 *           WriteProcessMemory    		(KERNEL32.@)
 */
BOOL WINAPI WriteProcessMemory( HANDLE process, LPVOID addr, LPVOID buffer, DWORD size,
                                LPDWORD bytes_written )
{
    unsigned int first_offset, last_offset;
    unsigned int pos = 0, len, max, first_mask, last_mask;
    int res;

    if (!size)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (bytes_written) *bytes_written = size;

    /* compute the mask for the first int */
    first_mask = ~0;
    first_offset = (unsigned int)addr % sizeof(int);
    memset( &first_mask, 0, first_offset );

    /* compute the mask for the last int */
    last_offset = (size + first_offset) % sizeof(int);
    last_mask = 0;
    memset( &last_mask, 0xff, last_offset ? last_offset : sizeof(int) );

    /* for the first request, use the total length */
    len = (size + first_offset + sizeof(int) - 1) / sizeof(int);
    max = min( REQUEST_MAX_VAR_SIZE, len * sizeof(int) );

    for (;;)
    {
        SERVER_START_VAR_REQ( write_process_memory, max )
        {
            req->handle = process;
            req->addr = (char *)addr - first_offset + pos;
            req->len = len;
            req->first_mask = (!pos) ? first_mask : ~0;
            if (size + first_offset <= max)  /* last round */
            {
                req->last_mask = last_mask;
                max = size + first_offset;
            }
            else req->last_mask = ~0;

            memcpy( (char *)server_data_ptr(req) + first_offset, (char *)buffer + pos,
                    max - first_offset );
            if (!(res = SERVER_CALL_ERR()))
            {
                pos += max - first_offset;
                size -= max - first_offset;
            }
        }
        SERVER_END_VAR_REQ;
        if (res)
        {
            if (bytes_written) *bytes_written = 0;
            return FALSE;
        }
        if (!size) return TRUE;
        first_offset = 0;
        len = min( size + sizeof(int) - 1, REQUEST_MAX_VAR_SIZE ) / sizeof(int);
        max = len * sizeof(int);
    }
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
        ret = !SERVER_CALL_ERR();
        if (ret && lpExitCode) *lpExitCode = req->exit_code;
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
    return 0xffffffff;
}
