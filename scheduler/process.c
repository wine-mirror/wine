/*
 * Win32 processes
 *
 * Copyright 1996, 1998 Alexandre Julliard
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "process.h"
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
#include "task.h"
#include "server.h"
#include "callback.h"
#include "debug.h"


/* The initial process PDB */
static PDB initial_pdb;

static PDB *PROCESS_First = &initial_pdb;

/***********************************************************************
 *           PROCESS_Current
 */
PDB *PROCESS_Current(void)
{
    return THREAD_Current()->process;
}

/***********************************************************************
 *           PROCESS_Initial
 *
 * FIXME: This works only while running all processes in the same
 *        address space (or, at least, the initial process is mapped
 *        into all address spaces as is KERNEL32 in Windows 95)
 *
 */
PDB *PROCESS_Initial(void)
{
    return &initial_pdb;
}

/***********************************************************************
 *           PROCESS_QueryInfo
 *
 * Retrieve information about a process
 */
static BOOL PROCESS_QueryInfo( HANDLE handle,
                                 struct get_process_info_reply *reply )
{
    struct get_process_info_request req;
    req.handle = handle;
    CLIENT_SendRequest( REQ_GET_PROCESS_INFO, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitSimpleReply( reply, sizeof(*reply), NULL );
}

/***********************************************************************
 *           PROCESS_IsCurrent
 *
 * Check if a handle is to the current process
 */
BOOL PROCESS_IsCurrent( HANDLE handle )
{
    struct get_process_info_reply reply;
    return (PROCESS_QueryInfo( handle, &reply ) &&
            (reply.pid == PROCESS_Current()->server_pid));
}


/***********************************************************************
 *           PROCESS_IdToPDB
 *
 * Convert a process id to a PDB, making sure it is valid.
 */
PDB *PROCESS_IdToPDB( DWORD id )
{
    PDB *pdb;

    if (!id) return PROCESS_Current();
    pdb = PROCESS_First;
    while (pdb)
    {
        if ((DWORD)pdb->server_pid == id) return pdb;
        pdb = pdb->next;
    }
    SetLastError( ERROR_INVALID_PARAMETER );
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
void PROCESS_CallUserSignalProc( UINT uCode, DWORD dwThreadOrProcessId, HMODULE hModule )
{
    PDB *pdb = PROCESS_Current();
    STARTUPINFOA *startup = pdb->env_db? pdb->env_db->startup_info : NULL;
    DWORD dwFlags = 0;

    /* Determine dwFlags */

    if ( !(pdb->flags & PDB32_WIN16_PROC) )
        dwFlags |= USIG_FLAGS_WIN32;

    if ( !(pdb->flags & PDB32_CONSOLE_PROC) )
        dwFlags |= USIG_FLAGS_GUI;

    if ( dwFlags & USIG_FLAGS_GUI )
    {
        /* Feedback defaults to ON */
        if ( !(startup && (startup->dwFlags & STARTF_FORCEOFFFEEDBACK)) )
            dwFlags |= USIG_FLAGS_FEEDBACK;
    }
    else
    {
        /* Feedback defaults to OFF */
        if ( startup && (startup->dwFlags & STARTF_FORCEONFEEDBACK) )
            dwFlags |= USIG_FLAGS_FEEDBACK;
    }

    /* Get thread or process ID */

    if ( dwThreadOrProcessId == 0 )
        if ( uCode == USIG_THREAD_INIT || uCode == USIG_THREAD_EXIT )
            dwThreadOrProcessId = GetCurrentThreadId();
        else
            dwThreadOrProcessId = GetCurrentProcessId();

    /* Convert module handle to 16-bit */

    if ( HIWORD( hModule ) )
        hModule = MapHModuleLS( hModule );

    /* Call USER signal proc */

    if ( Callout.UserSignalProc )
        Callout.UserSignalProc( uCode, dwThreadOrProcessId, dwFlags, hModule );
}


/***********************************************************************
 *           PROCESS_BuildEnvDB
 *
 * Build the env DB for the initial process
 */
static BOOL PROCESS_BuildEnvDB( PDB *pdb )
{
    /* Allocate the env DB (FIXME: should not be on the system heap) */

    if (!(pdb->env_db = HeapAlloc(SystemHeap,HEAP_ZERO_MEMORY,sizeof(ENVDB))))
        return FALSE;
    InitializeCriticalSection( &pdb->env_db->section );

    /* Allocate startup info */
    if (!(pdb->env_db->startup_info = 
          HeapAlloc( SystemHeap, HEAP_ZERO_MEMORY, sizeof(STARTUPINFOA) )))
        return FALSE;

    /* Allocate the standard handles */

    pdb->env_db->hStdin  = FILE_DupUnixHandle( 0, GENERIC_READ );
    pdb->env_db->hStdout = FILE_DupUnixHandle( 1, GENERIC_WRITE );
    pdb->env_db->hStderr = FILE_DupUnixHandle( 2, GENERIC_WRITE );

    /* Build the command-line */

    pdb->env_db->cmd_line = HEAP_strdupA( SystemHeap, 0, "kernel32" );

    /* Build the environment strings */

    return ENV_BuildEnvironment( pdb );
}


/***********************************************************************
 *           PROCESS_InheritEnvDB
 */
static BOOL PROCESS_InheritEnvDB( PDB *pdb, LPCSTR cmd_line, LPCSTR env,
                                    BOOL inherit_handles, STARTUPINFOA *startup )
{
    if (!(pdb->env_db = HeapAlloc(pdb->heap, HEAP_ZERO_MEMORY, sizeof(ENVDB))))
        return FALSE;
    InitializeCriticalSection( &pdb->env_db->section );

    /* Copy the parent environment */

    if (!ENV_InheritEnvironment( pdb, env )) return FALSE;

    /* Copy the command line */

    if (!(pdb->env_db->cmd_line = HEAP_strdupA( pdb->heap, 0, cmd_line )))
        return FALSE;

    /* Remember startup info */
    if (!(pdb->env_db->startup_info = 
          HeapAlloc( pdb->heap, HEAP_ZERO_MEMORY, sizeof(STARTUPINFOA) )))
        return FALSE;
    *pdb->env_db->startup_info = *startup;

    /* Inherit the standard handles */
    if (pdb->env_db->startup_info->dwFlags & STARTF_USESTDHANDLES)
    {
        pdb->env_db->hStdin  = pdb->env_db->startup_info->hStdInput;
        pdb->env_db->hStdout = pdb->env_db->startup_info->hStdOutput;
        pdb->env_db->hStderr = pdb->env_db->startup_info->hStdError;
    }
    else if (inherit_handles)
    {
        pdb->env_db->hStdin  = pdb->parent->env_db->hStdin;
        pdb->env_db->hStdout = pdb->parent->env_db->hStdout;
        pdb->env_db->hStderr = pdb->parent->env_db->hStderr;
    }
    /* else will be done later on in PROCESS_Create */

    return TRUE;
}


/***********************************************************************
 *           PROCESS_CreateEnvDB
 *
 * Create the env DB for a newly started process.
 */
static BOOL PROCESS_CreateEnvDB(void)
{
    struct init_process_request req;
    struct init_process_reply reply;
    STARTUPINFOA *startup;
    ENVDB *env_db;
    PDB *pdb = PROCESS_Current();

    /* Retrieve startup info from the server */

    req.dummy = 0;
    CLIENT_SendRequest( REQ_INIT_PROCESS, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) return FALSE;

    /* Allocate the env DB */

    if (!(env_db = HeapAlloc( pdb->heap, HEAP_ZERO_MEMORY, sizeof(ENVDB) )))
        return FALSE;
    pdb->env_db = env_db;
    InitializeCriticalSection( &env_db->section );

    /* Allocate and fill the startup info */
    if (!(startup = HeapAlloc( pdb->heap, HEAP_ZERO_MEMORY, sizeof(STARTUPINFOA) )))
        return FALSE;
    pdb->env_db->startup_info = startup;
    startup->dwFlags = reply.start_flags;
    pdb->env_db->hStdin  = startup->hStdInput  = reply.hstdin;
    pdb->env_db->hStdout = startup->hStdOutput = reply.hstdout;
    pdb->env_db->hStderr = startup->hStdError  = reply.hstderr;

#if 0  /* FIXME */
    /* Copy the parent environment */

    if (!ENV_InheritEnvironment( pdb, env )) return FALSE;

    /* Copy the command line */

    if (!(pdb->env_db->cmd_line = HEAP_strdupA( pdb->heap, 0, cmd_line )))
        return FALSE;
#endif
    return TRUE;
}


/***********************************************************************
 *           PROCESS_FreePDB
 *
 * Free a PDB and all associated storage.
 */
void PROCESS_FreePDB( PDB *pdb )
{
    PDB **pptr = &PROCESS_First;

    ENV_FreeEnvironment( pdb );
    while (*pptr && (*pptr != pdb)) pptr = &(*pptr)->next;
    if (*pptr) *pptr = pdb->next;
    if (pdb->heap && (pdb->heap != pdb->system_heap)) HeapDestroy( pdb->heap );
    HeapFree( SystemHeap, 0, pdb );
}


/***********************************************************************
 *           PROCESS_CreatePDB
 *
 * Allocate and fill a PDB structure.
 * Runs in the context of the parent process.
 */
static PDB *PROCESS_CreatePDB( PDB *parent, BOOL inherit )
{
    PDB *pdb = HeapAlloc( SystemHeap, HEAP_ZERO_MEMORY, sizeof(PDB) );

    if (!pdb) return NULL;
    pdb->exit_code       = 0x103; /* STILL_ACTIVE */
    pdb->threads         = 1;
    pdb->running_threads = 1;
    pdb->ring0_threads   = 1;
    pdb->system_heap     = SystemHeap;
    pdb->parent          = parent;
    pdb->group           = pdb;
    pdb->priority        = 8;  /* Normal */
    pdb->heap            = pdb->system_heap;  /* will be changed later on */
    pdb->next            = PROCESS_First;
    PROCESS_First = pdb;
    return pdb;
}


/***********************************************************************
 *           PROCESS_Init
 */
BOOL PROCESS_Init(void)
{
    THDB *thdb;
    int server_fd;

    /* Start the server */
    server_fd = CLIENT_InitServer();

    /* Fill the initial process structure */
    initial_pdb.exit_code       = 0x103; /* STILL_ACTIVE */
    initial_pdb.threads         = 1;
    initial_pdb.running_threads = 1;
    initial_pdb.ring0_threads   = 1;
    initial_pdb.group           = &initial_pdb;
    initial_pdb.priority        = 8;  /* Normal */
    initial_pdb.flags           = PDB32_WIN16_PROC;

    /* Initialize virtual memory management */
    if (!VIRTUAL_Init()) return FALSE;

    /* Create the initial thread structure and socket pair */
    if (!(thdb = THREAD_CreateInitialThread( &initial_pdb, server_fd ))) return FALSE;

    /* Remember TEB selector of initial process for emergency use */
    SYSLEVEL_EmergencyTeb = thdb->teb_sel;

    /* Create the system heap */
    if (!(SystemHeap = HeapCreate( HEAP_GROWABLE, 0x10000, 0 ))) return FALSE;
    initial_pdb.system_heap = initial_pdb.heap = SystemHeap;

    /* Create the environment DB of the first process */
    if (!PROCESS_BuildEnvDB( &initial_pdb )) return FALSE;

    /* Create the SEGPTR heap */
    if (!(SegptrHeap = HeapCreate( HEAP_WINE_SEGPTR, 0, 0 ))) return FALSE;

    /* Initialize the first process critical section */
    InitializeCriticalSection( &initial_pdb.crit_section );

    return TRUE;
}


/***********************************************************************
 *           PROCESS_Start
 *
 * Startup routine of a new process. Called in the context of the new process.
 */
void PROCESS_Start(void)
{
    DWORD size, commit;
    UINT cmdShow = 0;
    LPTHREAD_START_ROUTINE entry;
    THDB *thdb = THREAD_Current();
    PDB *pdb = thdb->process;
    TDB *pTask = (TDB *)GlobalLock16( pdb->task );
    NE_MODULE *pModule = NE_GetPtr( pTask->hModule );
    OFSTRUCT *ofs = (OFSTRUCT *)((char*)(pModule) + (pModule)->fileinfo);

    PROCESS_CallUserSignalProc( USIG_THREAD_INIT, 0, 0 );  /* for initial thread */

#if 0
    /* Initialize the critical section */

    InitializeCriticalSection( &pdb->crit_section );

    /* Create the heap */

    size  = PE_HEADER(pModule->module32)->OptionalHeader.SizeOfHeapReserve;
    commit = PE_HEADER(pModule->module32)->OptionalHeader.SizeOfHeapCommit;
    if (!(pdb->heap = HeapCreate( HEAP_GROWABLE, size, commit ))) goto error;
    pdb->heap_list = pdb->heap;

    /* Create the environment db */

    if (!PROCESS_CreateEnvDB()) goto error;

    if (pdb->env_db->startup_info->dwFlags & STARTF_USESHOWWINDOW)
        cmdShow = pdb->env_db->startup_info->wShowWindow;
    if (!TASK_Create( thdb, pModule, 0, 0, cmdShow )) goto error;

#endif

    PROCESS_CallUserSignalProc( USIG_PROCESS_INIT, 0, 0 );

    /* Map system DLLs into this process (from initial process) */
    /* FIXME: this is a hack */
    pdb->modref_list = PROCESS_Initial()->modref_list;

    /* Create 32-bit MODREF */
    if (!PE_CreateModule( pModule->module32, ofs, 0, FALSE )) goto error;

    PROCESS_CallUserSignalProc( USIG_PROCESS_LOADED, 0, 0 );   /* FIXME: correct location? */

    /* Initialize thread-local storage */

    PE_InitTls();

    if ( pdb->flags & PDB32_CONSOLE_PROC )
        AllocConsole();

    /* Now call the entry point */

    MODULE_InitializeDLLs( 0, DLL_PROCESS_ATTACH, (LPVOID)1 );

    PROCESS_CallUserSignalProc( USIG_PROCESS_RUNNING, 0, 0 );

    entry = (LPTHREAD_START_ROUTINE)RVA_PTR(pModule->module32,
                                            OptionalHeader.AddressOfEntryPoint);
    TRACE(relay, "(entryproc=%p)\n", entry );
    ExitProcess( entry(NULL) );

 error:
    ExitProcess(1);
}


/***********************************************************************
 *           PROCESS_Create
 *
 * Create a new process database and associated info.
 */
PDB *PROCESS_Create( NE_MODULE *pModule, LPCSTR cmd_line, LPCSTR env,
                     HINSTANCE16 hInstance, HINSTANCE16 hPrevInstance,
                     LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                     BOOL inherit, STARTUPINFOA *startup,
                     PROCESS_INFORMATION *info )
{
    DWORD size, commit;
    int server_thandle;
    struct new_process_request req;
    struct new_process_reply reply;
    UINT cmdShow = 0;
    THDB *thdb = NULL;
    PDB *parent = PROCESS_Current();
    PDB *pdb = PROCESS_CreatePDB( parent, inherit );

    if (!pdb) return NULL;
    info->hThread = info->hProcess = INVALID_HANDLE_VALUE;

    /* Create the process on the server side */

    req.inherit     = (psa && (psa->nLength >= sizeof(*psa)) && psa->bInheritHandle);
    req.inherit_all = inherit;
    req.start_flags = startup->dwFlags;
    if (startup->dwFlags & STARTF_USESTDHANDLES)
    {
        req.hstdin  = startup->hStdInput;
        req.hstdout = startup->hStdOutput;
        req.hstderr = startup->hStdError;
    }
    else
    {
        req.hstdin  = GetStdHandle( STD_INPUT_HANDLE );
        req.hstdout = GetStdHandle( STD_OUTPUT_HANDLE );
        req.hstderr = GetStdHandle( STD_ERROR_HANDLE );
    }
    CLIENT_SendRequest( REQ_NEW_PROCESS, -1, 2,
                        &req, sizeof(req), cmd_line, strlen(cmd_line) + 1 );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) goto error;
    pdb->server_pid   = reply.pid;
    info->hProcess    = reply.handle;
    info->dwProcessId = (DWORD)pdb->server_pid;

    /* Initialize the critical section */

    InitializeCriticalSection( &pdb->crit_section );

    /* Setup process flags */

    if ( !pModule->module32 )
        pdb->flags |= PDB32_WIN16_PROC;

    else if ( PE_HEADER(pModule->module32)->OptionalHeader.Subsystem
              == IMAGE_SUBSYSTEM_WINDOWS_CUI )
        pdb->flags |= PDB32_CONSOLE_PROC;

    /* Create the heap */

    if (pModule->module32)
    {
	size  = PE_HEADER(pModule->module32)->OptionalHeader.SizeOfHeapReserve;
	commit = PE_HEADER(pModule->module32)->OptionalHeader.SizeOfHeapCommit;
    }
    else
    {
	size = 0x10000;
	commit = 0;
    }
    if (!(pdb->heap = HeapCreate( HEAP_GROWABLE, size, commit ))) goto error;
    pdb->heap_list = pdb->heap;

    /* Inherit the env DB from the parent */

    if (!PROCESS_InheritEnvDB( pdb, cmd_line, env, inherit, startup )) goto error;

    /* Call USER signal proc */

    PROCESS_CallUserSignalProc( USIG_PROCESS_CREATE, info->dwProcessId, 0 );

    /* Create the main thread */

    if (pModule->module32)
        size = PE_HEADER(pModule->module32)->OptionalHeader.SizeOfStackReserve;
    else
        size = 0;
    if (!(thdb = THREAD_Create( pdb, 0L, size, hInstance == 0, tsa, &server_thandle ))) 
        goto error;
    info->hThread     = server_thandle;
    info->dwThreadId  = (DWORD)thdb->server_tid;
    thdb->startup     = PROCESS_Start;

    /* Duplicate the standard handles */

    if ((!(pdb->env_db->startup_info->dwFlags & STARTF_USESTDHANDLES)) && !inherit)
    {
        DuplicateHandle( GetCurrentProcess(), pdb->parent->env_db->hStdin,
                         info->hProcess, &pdb->env_db->hStdin, 0, TRUE, DUPLICATE_SAME_ACCESS );
        DuplicateHandle( GetCurrentProcess(), pdb->parent->env_db->hStdout,
                         info->hProcess, &pdb->env_db->hStdout, 0, TRUE, DUPLICATE_SAME_ACCESS );
        DuplicateHandle( GetCurrentProcess(), pdb->parent->env_db->hStderr,
                         info->hProcess, &pdb->env_db->hStderr, 0, TRUE, DUPLICATE_SAME_ACCESS );
    }

    /* Create a Win16 task for this process */

    if (startup->dwFlags & STARTF_USESHOWWINDOW)
        cmdShow = startup->wShowWindow;

    if ( !TASK_Create( thdb, pModule, hInstance, hPrevInstance, cmdShow) )
        goto error;


    /* Map system DLLs into this process (from initial process) */
    /* FIXME: this is a hack */
    pdb->modref_list = PROCESS_Initial()->modref_list;
    

    /* Start the task */

    TASK_StartTask( pdb->task );

    return pdb;

error:
    if (info->hThread != INVALID_HANDLE_VALUE) CloseHandle( info->hThread );
    if (info->hProcess != INVALID_HANDLE_VALUE) CloseHandle( info->hProcess );
    PROCESS_FreePDB( pdb );
    return NULL;
}


/***********************************************************************
 *           ExitProcess   (KERNEL32.100)
 */
void WINAPI ExitProcess( DWORD status )
{
    MODULE_InitializeDLLs( 0, DLL_PROCESS_DETACH, (LPVOID)1 );

    if ( THREAD_IsWin16( THREAD_Current() ) )
        TASK_KillCurrentTask( status );

    TASK_KillTask( 0 );
    TerminateProcess( GetCurrentProcess(), status );
}


/******************************************************************************
 *           TerminateProcess   (KERNEL32.684)
 */
BOOL WINAPI TerminateProcess( HANDLE handle, DWORD exit_code )
{
    struct terminate_process_request req;
    req.handle    = handle;
    req.exit_code = exit_code;
    CLIENT_SendRequest( REQ_TERMINATE_PROCESS, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
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

    TRACE( win32, "(%ld, %d)\n", dwProcessID, offset );
    if ( !process ) return 0;

    switch ( offset ) 
    {
    case GPD_APP_COMPAT_FLAGS:
        pTask = (TDB *)GlobalLock16( process->task );
        return pTask? pTask->compat_flags : 0;

    case GPD_LOAD_DONE_EVENT:
        return process->load_done_evt;

    case GPD_HINSTANCE16:
        pTask = (TDB *)GlobalLock16( process->task );
        return pTask? pTask->hInstance : 0;

    case GPD_WINDOWS_VERSION:
        pTask = (TDB *)GlobalLock16( process->task );
        return pTask? pTask->version : 0;

    case GPD_THDB:
        if ( process != PROCESS_Current() ) return 0;
        return (DWORD)THREAD_Current();

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
        return (DWORD)process->parent->server_pid;

    case GPD_FLAGS:
        return process->flags;

    case GPD_USERDATA:
        return process->process_dword;

    default:
        ERR( win32, "Unknown offset %d\n", offset );
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

    TRACE( win32, "(%ld, %d)\n", dwProcessID, offset );
    if ( !process ) return;

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
        ERR( win32, "Not allowed to modify offset %d\n", offset );
        break;

    case GPD_USERDATA:
        process->process_dword = value; 
        break;

    default:
        ERR( win32, "Unknown offset %d\n", offset );
        break;
    }
}


/***********************************************************************
 *           GetCurrentProcess   (KERNEL32.198)
 */
HANDLE WINAPI GetCurrentProcess(void)
{
    return CURRENT_PROCESS_PSEUDOHANDLE;
}


/*********************************************************************
 *           OpenProcess   (KERNEL32.543)
 */
HANDLE WINAPI OpenProcess( DWORD access, BOOL inherit, DWORD id )
{
    struct open_process_request req;
    struct open_process_reply reply;

    req.pid     = (void *)id;
    req.access  = access;
    req.inherit = inherit;
    CLIENT_SendRequest( REQ_OPEN_PROCESS, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) return 0;
    return reply.handle;
}			      


/***********************************************************************
 *           GetCurrentProcessId   (KERNEL32.199)
 */
DWORD WINAPI GetCurrentProcessId(void)
{
    return (DWORD)PROCESS_Current()->server_pid;
}


/***********************************************************************
 *           GetProcessHeap    (KERNEL32.259)
 */
HANDLE WINAPI GetProcessHeap(void)
{
    PDB *pdb = PROCESS_Current();
    return pdb->heap ? pdb->heap : SystemHeap;
}


/***********************************************************************
 *           GetThreadLocale    (KERNEL32.295)
 */
LCID WINAPI GetThreadLocale(void)
{
    return PROCESS_Current()->locale;
}


/***********************************************************************
 *           SetPriorityClass   (KERNEL32.503)
 */
BOOL WINAPI SetPriorityClass( HANDLE hprocess, DWORD priorityclass )
{
    struct set_process_info_request req;
    req.handle   = hprocess;
    req.priority = priorityclass;
    req.mask     = SET_PROCESS_INFO_PRIORITY;
    CLIENT_SendRequest( REQ_SET_PROCESS_INFO, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
}


/***********************************************************************
 *           GetPriorityClass   (KERNEL32.250)
 */
DWORD WINAPI GetPriorityClass(HANDLE hprocess)
{
    struct get_process_info_reply reply;
    if (!PROCESS_QueryInfo( hprocess, &reply )) return 0;
    return reply.priority;
}


/***********************************************************************
 *          SetProcessAffinityMask   (KERNEL32.662)
 */
BOOL WINAPI SetProcessAffinityMask( HANDLE hProcess, DWORD affmask )
{
    struct set_process_info_request req;
    req.handle   = hProcess;
    req.affinity = affmask;
    req.mask     = SET_PROCESS_INFO_AFFINITY;
    CLIENT_SendRequest( REQ_SET_PROCESS_INFO, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
}

/**********************************************************************
 *          GetProcessAffinityMask    (KERNEL32.373)
 */
BOOL WINAPI GetProcessAffinityMask( HANDLE hProcess,
                                      LPDWORD lpProcessAffinityMask,
                                      LPDWORD lpSystemAffinityMask )
{
    struct get_process_info_reply reply;
    if (!PROCESS_QueryInfo( hProcess, &reply )) return FALSE;
    if (lpProcessAffinityMask) *lpProcessAffinityMask = reply.process_affinity;
    if (lpSystemAffinityMask) *lpSystemAffinityMask = reply.system_affinity;
    return TRUE;
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
    FIXME(process,"(0x%08x,%ld,%ld): stub - harmless\n",hProcess,minset,maxset);
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
	FIXME(process,"(0x%08x,%p,%p): stub\n",hProcess,minset,maxset);
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
	ERR(process,"invalid priority level 0x%08lx\n", level);
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
    FIXME(process,"(%d,%d): stub\n",hprocess,disableboost);
    /* Say we can do it. I doubt the program will notice that we don't. */
    return TRUE;
}

/***********************************************************************
 *           ReadProcessMemory    		(KERNEL32)
 * FIXME: check this, if we ever run win32 binaries in different addressspaces
 *	  ... and add a sizecheck
 */
BOOL WINAPI ReadProcessMemory( HANDLE hProcess, LPCVOID lpBaseAddress,
                                 LPVOID lpBuffer, DWORD nSize,
                                 LPDWORD lpNumberOfBytesRead )
{
	memcpy(lpBuffer,lpBaseAddress,nSize);
	if (lpNumberOfBytesRead) *lpNumberOfBytesRead = nSize;
	return TRUE;
}

/***********************************************************************
 *           WriteProcessMemory    		(KERNEL32)
 * FIXME: check this, if we ever run win32 binaries in different addressspaces
 *	  ... and add a sizecheck
 */
BOOL WINAPI WriteProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress,
                                 LPVOID lpBuffer, DWORD nSize,
                                 LPDWORD lpNumberOfBytesWritten )
{
	memcpy(lpBaseAddress,lpBuffer,nSize);
	if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = nSize;
	return TRUE;
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
    struct get_process_info_reply reply;
    if (!PROCESS_QueryInfo( hProcess, &reply )) return FALSE;
    if (lpExitCode) *lpExitCode = reply.exit_code;
    return TRUE;
}


/***********************************************************************
 * GetProcessHeaps [KERNEL32.376]
 */
DWORD WINAPI GetProcessHeaps(DWORD nrofheaps,HANDLE *heaps) {
	FIXME(win32,"(%ld,%p), incomplete implementation.\n",nrofheaps,heaps);

	if (nrofheaps) {
		heaps[0] = GetProcessHeap();
		/* ... probably SystemHeap too ? */
		return 1;
	}
	/* number of available heaps */
	return 1;
}

