/*
 * Win32 processes
 *
 * Copyright 1996, 1998 Alexandre Julliard
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "wine/winbase16.h"
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
#include "debugtools.h"

DECLARE_DEBUG_CHANNEL(process)
DECLARE_DEBUG_CHANNEL(relay)
DECLARE_DEBUG_CHANNEL(win32)


/* The initial process PDB */
static PDB initial_pdb;

static PDB *PROCESS_First = &initial_pdb;

/***********************************************************************
 *           PROCESS_WalkProcess
 */
void PROCESS_WalkProcess(void)
{
    PDB  *pdb;
    char *name;

    pdb = PROCESS_First;
    MESSAGE( " pid        PDB         #th  modref     module \n" );
    while(pdb)
    {
        if (pdb == &initial_pdb)
            name = "initial PDB";
        else
            name = (pdb->exe_modref) ? pdb->exe_modref->shortname : "";

        MESSAGE( " %8p %8p %5d  %8p %s\n", pdb->server_pid, pdb,
               pdb->threads, pdb->exe_modref, name);
        pdb = pdb->next;
    }
    return;
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
 *           PROCESS_IsCurrent
 *
 * Check if a handle is to the current process
 */
BOOL PROCESS_IsCurrent( HANDLE handle )
{
    struct get_process_info_request *req = get_req_buffer();
    req->handle = handle;
    return (!server_call( REQ_GET_PROCESS_INFO ) &&
            (req->pid == PROCESS_Current()->server_pid));
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
static void PROCESS_CallUserSignalProcHelper( UINT uCode, DWORD dwThreadOrProcessId,
                                              HMODULE hModule, DWORD flags, DWORD startup_flags )
{
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
        Callout.UserSignalProc( uCode, dwThreadOrProcessId, dwFlags, hModule );
}

/* Call USER signal proc for the current thread/process */
void PROCESS_CallUserSignalProc( UINT uCode, HMODULE hModule )
{
    DWORD dwThreadOrProcessId;

    /* Get thread or process ID */
    if ( uCode == USIG_THREAD_INIT || uCode == USIG_THREAD_EXIT )
        dwThreadOrProcessId = GetCurrentThreadId();
    else
        dwThreadOrProcessId = GetCurrentProcessId();

    PROCESS_CallUserSignalProcHelper( uCode, dwThreadOrProcessId, hModule,
                                      PROCESS_Current()->flags,
                                      PROCESS_Current()->env_db->startup_info->dwFlags );
}


/***********************************************************************
 *           PROCESS_CreateEnvDB
 *
 * Create the env DB for a newly started process.
 */
static BOOL PROCESS_CreateEnvDB(void)
{
    struct init_process_request *req = get_req_buffer();
    STARTUPINFOA *startup;
    ENVDB *env_db;
    char cmd_line[4096];
    PDB *pdb = PROCESS_Current();

    /* Allocate the env DB */

    if (!(env_db = HeapAlloc( pdb->heap, HEAP_ZERO_MEMORY, sizeof(ENVDB) )))
        return FALSE;
    pdb->env_db = env_db;
    InitializeCriticalSection( &env_db->section );

    /* Allocate and fill the startup info */
    if (!(startup = HeapAlloc( pdb->heap, HEAP_ZERO_MEMORY, sizeof(STARTUPINFOA) )))
        return FALSE;
    env_db->startup_info = startup;

    /* Retrieve startup info from the server */

    if (server_call( REQ_INIT_PROCESS )) return FALSE;
    startup->dwFlags     = req->start_flags;
    startup->wShowWindow = req->cmd_show;
    env_db->hStdin  = startup->hStdInput  = req->hstdin;
    env_db->hStdout = startup->hStdOutput = req->hstdout;
    env_db->hStderr = startup->hStdError  = req->hstderr;
    lstrcpynA( cmd_line, req->cmdline, sizeof(cmd_line) );

    /* Copy the parent environment */

    if (!ENV_InheritEnvironment( pdb, req->env_ptr )) return FALSE;

    /* Copy the command line */

    if (!(pdb->env_db->cmd_line = HEAP_strdupA( pdb->heap, 0, cmd_line )))
        return FALSE;

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
    pdb->winver          = 0xffff; /* to be determined */
    PROCESS_First = pdb;
    return pdb;
}


/***********************************************************************
 *           PROCESS_Init
 */
BOOL PROCESS_Init(void)
{
    TEB *teb;
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
    initial_pdb.winver          = 0xffff; /* to be determined */

    /* Initialize virtual memory management */
    if (!VIRTUAL_Init()) return FALSE;

    /* Create the initial thread structure and socket pair */
    if (!(teb = THREAD_CreateInitialThread( &initial_pdb, server_fd ))) return FALSE;

    /* Remember TEB selector of initial process for emergency use */
    SYSLEVEL_EmergencyTeb = teb->teb_sel;

    /* Create the system heap */
    if (!(SystemHeap = HeapCreate( HEAP_GROWABLE, 0x10000, 0 ))) return FALSE;
    initial_pdb.system_heap = initial_pdb.heap = SystemHeap;

    /* Create the environment DB of the first process */
    if (!PROCESS_CreateEnvDB()) return FALSE;

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
    UINT cmdShow = 0;
    LPTHREAD_START_ROUTINE entry = NULL;
    PDB *pdb = PROCESS_Current();
    NE_MODULE *pModule = NE_GetPtr( pdb->module );
    OFSTRUCT *ofs = (OFSTRUCT *)((char*)(pModule) + (pModule)->fileinfo);
    IMAGE_OPTIONAL_HEADER *header = !pModule->module32? NULL :
                                    &PE_HEADER(pModule->module32)->OptionalHeader;

    /* Get process type */
    enum { PROC_DOS, PROC_WIN16, PROC_WIN32 } type;
    if ( pdb->flags & PDB32_DOS_PROC )
        type = PROC_DOS;
    else if ( pdb->flags & PDB32_WIN16_PROC )
        type = PROC_WIN16;
    else
        type = PROC_WIN32;

    /* Initialize the critical section */
    InitializeCriticalSection( &pdb->crit_section );

    /* Create the heap */
    if (!(pdb->heap = HeapCreate( HEAP_GROWABLE, 
                                  header? header->SizeOfHeapReserve : 0x10000,
                                  header? header->SizeOfHeapCommit  : 0 ))) 
        goto error;
    pdb->heap_list = pdb->heap;

    /* Create the environment db */
    if (!PROCESS_CreateEnvDB()) goto error;

    /* Create a task for this process */
    if (pdb->env_db->startup_info->dwFlags & STARTF_USESHOWWINDOW)
        cmdShow = pdb->env_db->startup_info->wShowWindow;
    if (!TASK_Create( pModule, cmdShow ))
        goto error;

    /* Perform Win16 specific process initialization */
    if ( type == PROC_WIN16 )
        if ( !NE_InitProcess( pModule ) )
            goto error;

    /* Note: The USIG_PROCESS_CREATE signal is supposed to be sent in the
     *       context of the parent process.  Actually, the USER signal proc
     *       doesn't really care about that, but it *does* require that the
     *       startup parameters are correctly set up, so that GetProcessDword
     *       works.  Furthermore, before calling the USER signal proc the 
     *       16-bit stack must be set up, which it is only after TASK_Create
     *       in the case of a 16-bit process. Thus, we send the signal here.
     */

    /* Load USER32.DLL before calling UserSignalProc (relay debugging!) */
    LoadLibraryA( "USER32.DLL" );

    PROCESS_CallUserSignalProc( USIG_PROCESS_CREATE, 0 );

    PROCESS_CallUserSignalProc( USIG_THREAD_INIT, 0 );  /* for initial thread */

    PROCESS_CallUserSignalProc( USIG_PROCESS_INIT, 0 );

    /* Signal the parent process to continue */
    SetEvent( pdb->load_done_evt );
    CloseHandle( pdb->load_done_evt );
    pdb->load_done_evt = INVALID_HANDLE_VALUE;

    /* Perform Win32 specific process initialization */
    if ( type == PROC_WIN32 )
    {
        /* Send the debug event to the debugger */
        entry = (LPTHREAD_START_ROUTINE)RVA_PTR(pModule->module32,
                                                OptionalHeader.AddressOfEntryPoint);
        if (pdb->flags & PDB32_DEBUGGED)
            DEBUG_SendCreateProcessEvent( -1 /*FIXME*/, pModule->module32, entry );

        /* Create 32-bit MODREF */
        if (!PE_CreateModule( pModule->module32, ofs, 0, FALSE )) goto error;

        /* Increment EXE refcount */
        assert( pdb->exe_modref );
        pdb->exe_modref->refCount++;

        /* Initialize thread-local storage */
        PE_InitTls();
    }

    PROCESS_CallUserSignalProc( USIG_PROCESS_LOADED, 0 );   /* FIXME: correct location? */

    if ( (pdb->flags & PDB32_CONSOLE_PROC) || (pdb->flags & PDB32_DOS_PROC) )
        AllocConsole();

    if ( type == PROC_WIN32 )
    {
        EnterCriticalSection( &pdb->crit_section );
        MODULE_DllProcessAttach( pdb->exe_modref, (LPVOID)1 );
        LeaveCriticalSection( &pdb->crit_section );
    }

    /* Now call the entry point */
    PROCESS_CallUserSignalProc( USIG_PROCESS_RUNNING, 0 );

    switch ( type )
    {
    case PROC_DOS:
        TRACE_(relay)( "Starting DOS process\n" );
        DOSVM_Enter( NULL );
        ERR_(relay)( "DOSVM_Enter returned; should not happen!\n" );
        ExitProcess( 0 );

    case PROC_WIN16:
        TRACE_(relay)( "Starting Win16 process\n" );
        TASK_CallToStart();
        ERR_(relay)( "TASK_CallToStart returned; should not happen!\n" );
        ExitProcess( 0 );

    case PROC_WIN32:
        TRACE_(relay)( "Starting Win32 process (entryproc=%p)\n", entry );
        ExitProcess( entry(NULL) );
    }

 error:
    ExitProcess( GetLastError() );
}


/***********************************************************************
 *           PROCESS_Create
 *
 * Create a new process database and associated info.
 */
PDB *PROCESS_Create( NE_MODULE *pModule, LPCSTR cmd_line, LPCSTR env,
                     LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                     BOOL inherit, DWORD flags, STARTUPINFOA *startup,
                     PROCESS_INFORMATION *info )
{
    HANDLE handles[2], load_done_evt = INVALID_HANDLE_VALUE;
    DWORD exitcode, size;
    BOOL alloc_stack16;
    int server_thandle;
    struct new_process_request *req = get_req_buffer();
    TEB *teb = NULL;
    PDB *parent = PROCESS_Current();
    PDB *pdb = PROCESS_CreatePDB( parent, inherit );

    if (!pdb) return NULL;
    info->hThread = info->hProcess = INVALID_HANDLE_VALUE;

    /* Create the process on the server side */

    req->inherit      = (psa && (psa->nLength >= sizeof(*psa)) && psa->bInheritHandle);
    req->inherit_all  = inherit;
    req->create_flags = flags;
    req->start_flags  = startup->dwFlags;
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
    req->env_ptr = (void*)env;  /* FIXME: hack */
    lstrcpynA( req->cmdline, cmd_line, server_remaining(req->cmdline) );
    if (server_call( REQ_NEW_PROCESS )) goto error;
    pdb->server_pid   = req->pid;
    info->hProcess    = req->handle;
    info->dwProcessId = (DWORD)pdb->server_pid;

    if ((flags & DEBUG_PROCESS) ||
        ((parent->flags & PDB32_DEBUGGED) && !(flags & DEBUG_ONLY_THIS_PROCESS)))
        pdb->flags |= PDB32_DEBUGGED;

    if (pModule->module32)   /* Win32 process */
    {
        IMAGE_OPTIONAL_HEADER *header = &PE_HEADER(pModule->module32)->OptionalHeader;
        size = header->SizeOfStackReserve;
        if (header->Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI) 
            pdb->flags |= PDB32_CONSOLE_PROC;
        alloc_stack16 = TRUE;
    }
    else if (!pModule->dos_image) /* Win16 process */
    {
        alloc_stack16 = FALSE;
        size = 0;
        pdb->flags |= PDB32_WIN16_PROC;
    }
    else  /* DOS process */
    {
        alloc_stack16 = FALSE;
        size = 0;
        pdb->flags |= PDB32_DOS_PROC;
    }

    /* Create the main thread */

    if (!(teb = THREAD_Create( pdb, 0L, size, alloc_stack16, tsa, &server_thandle ))) 
        goto error;
    info->hThread     = server_thandle;
    info->dwThreadId  = (DWORD)teb->tid;
    teb->startup = PROCESS_Start;

    /* Create the load-done event */
    load_done_evt = CreateEventA( NULL, TRUE, FALSE, NULL );
    DuplicateHandle( GetCurrentProcess(), load_done_evt,
                     info->hProcess, &pdb->load_done_evt, 0, TRUE, DUPLICATE_SAME_ACCESS );

    /* Pass module to new process (FIXME: hack) */
    pdb->module = pModule->self;
    SYSDEPS_SpawnThread( teb );

    /* Wait until process is initialized (or initialization failed) */
    handles[0] = info->hProcess;
    handles[1] = load_done_evt;

    switch ( WaitForMultipleObjects( 2, handles, FALSE, INFINITE ) )
    {
    default: 
        ERR_(process)( "WaitForMultipleObjects failed\n" );
        break;

    case 0:
        /* Child initialization code returns error condition as exitcode */
        if ( GetExitCodeProcess( info->hProcess, &exitcode ) )
            SetLastError( exitcode );
        goto error;

    case 1:
        /* Get 16-bit task up and running */
        if ( pdb->flags & PDB32_WIN16_PROC )
        {
            /* Post event to start the task */
            PostEvent16( pdb->task );

            /* If we ourselves are a 16-bit task, we Yield() directly. */
            if ( parent->flags & PDB32_WIN16_PROC )
                OldYield16();
        }
        break;
    } 

    CloseHandle( load_done_evt );
    load_done_evt = INVALID_HANDLE_VALUE;

    return pdb;

error:
    if (load_done_evt != INVALID_HANDLE_VALUE) CloseHandle( load_done_evt );
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
    EnterCriticalSection( &PROCESS_Current()->crit_section );
    MODULE_DllProcessDetach( TRUE, (LPVOID)1 );
    LeaveCriticalSection( &PROCESS_Current()->crit_section );

    TASK_KillTask( 0 );
    TerminateProcess( GetCurrentProcess(), status );
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
    struct terminate_process_request *req = get_req_buffer();
    req->handle    = handle;
    req->exit_code = exit_code;
    return !server_call( REQ_TERMINATE_PROCESS );
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
        return (DWORD)process->parent->server_pid;

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
    FIXME_(process)("(0x%08x,%ld,%ld): stub - harmless\n",hProcess,minset,maxset);
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
	FIXME_(process)("(0x%08x,%p,%p): stub\n",hProcess,minset,maxset);
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
	ERR_(process)("invalid priority level 0x%08lx\n", level);
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
    FIXME_(process)("(%d,%d): stub\n",hprocess,disableboost);
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
 * GetProcessHeaps [KERNEL32.376]
 */
DWORD WINAPI GetProcessHeaps(DWORD nrofheaps,HANDLE *heaps) {
	FIXME_(win32)("(%ld,%p), incomplete implementation.\n",nrofheaps,heaps);

	if (nrofheaps) {
		heaps[0] = GetProcessHeap();
		/* ... probably SystemHeap too ? */
		return 1;
	}
	/* number of available heaps */
	return 1;
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
