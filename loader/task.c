/*
 * Task functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windows.h"
#include "task.h"
#include "callback.h"
#include "dos_fs.h"
#include "file.h"
#include "global.h"
#include "instance.h"
#include "message.h"
#include "miscemu.h"
#include "module.h"
#include "neexe.h"
#include "options.h"
#include "peexe.h"
#include "pe_image.h"
#include "process.h"
#include "queue.h"
#include "selectors.h"
#include "stackframe.h"
#include "thread.h"
#include "toolhelp.h"
#include "winnt.h"
#include "stddebug.h"
#include "debug.h"
#include "dde_proc.h"

#ifndef WINELIB
#include "debugger.h"
#endif

  /* Min. number of thunks allocated when creating a new segment */
#define MIN_THUNKS  32

extern void USER_AppExit( HTASK16, HINSTANCE16, HQUEUE16 );

  /* Saved 16-bit stack for current process (Win16 only) */
WORD IF1632_Saved16_ss = 0;
WORD IF1632_Saved16_sp = 0;

  /* Saved 32-bit stack for current process (Win16 only) */
DWORD IF1632_Saved32_esp = 0;

  /* Original Unix stack */
DWORD IF1632_Original32_esp;

static HTASK16 hFirstTask = 0;
static HTASK16 hCurrentTask = 0;
static HTASK16 hTaskToKill = 0;
static HTASK16 hLockedTask = 0;
static UINT16 nTaskCount = 0;
static HGLOBAL16 hDOSEnvironment = 0;

  /* TASK_Reschedule() 16-bit entry point */
static FARPROC16 TASK_RescheduleProc;

#ifdef WINELIB
#define TASK_SCHEDULE()  TASK_Reschedule()
#else
#define TASK_SCHEDULE()  CallTo16_word_(TASK_RescheduleProc)
#endif

static HGLOBAL16 TASK_CreateDOSEnvironment(void);
static void	 TASK_YieldToSystem(TDB*);

/***********************************************************************
 *           TASK_Init
 */
BOOL32 TASK_Init(void)
{
    TASK_RescheduleProc = MODULE_GetWndProcEntry16( "TASK_Reschedule" );
    if (!(hDOSEnvironment = TASK_CreateDOSEnvironment()))
        fprintf( stderr, "Not enough memory for DOS Environment\n" );
    return (hDOSEnvironment != 0);
}


/***********************************************************************
 *	     TASK_GetNextTask
 */
HTASK16 TASK_GetNextTask( HTASK16 hTask )
{
    TDB* pTask = (TDB*)GlobalLock16(hTask);

    if (pTask->hNext) return pTask->hNext;
    return (hFirstTask != hTask) ? hFirstTask : 0; 
}


/***********************************************************************
 *           TASK_CreateDOSEnvironment
 *
 * Create the original DOS environment.
 */
static HGLOBAL16 TASK_CreateDOSEnvironment(void)
{
    static const char program_name[] = "KRNL386.EXE";
    char **e, *p;
    int initial_size, size, i, winpathlen, sysdirlen;
    HGLOBAL16 handle;

    extern char **environ;

    /* DOS environment format:
     * ASCIIZ   string 1
     * ASCIIZ   string 2
     * ...
     * ASCIIZ   string n
     * ASCIIZ   PATH=xxx
     * BYTE     0
     * WORD     1
     * ASCIIZ   program name (e.g. C:\WINDOWS\SYSTEM\KRNL386.EXE)
     */

    /* First compute the size of the fixed part of the environment */

    for (i = winpathlen = 0; ; i++)
    {
        int len = DIR_GetDosPath( i, NULL, 0 );
        if (!len) break;
        winpathlen += len + 1;
    }
    if (!winpathlen) winpathlen = 1;
    sysdirlen  = GetSystemDirectory32A( NULL, 0 ) + 1;
    initial_size = 5 + winpathlen +           /* PATH=xxxx */
                   1 +                        /* BYTE 0 at end */
                   sizeof(WORD) +             /* WORD 1 */
                   sysdirlen +                /* program directory */
                   strlen(program_name) + 1;  /* program name */

    /* Compute the total size of the Unix environment (except path) */

    for (e = environ, size = initial_size; *e; e++)
    {
	if (lstrncmpi32A(*e, "path=", 5))
	{
            int len = strlen(*e) + 1;
            if (size + len >= 32767)
            {
                fprintf( stderr, "Warning: environment larger than 32k.\n" );
                break;
            }
            size += len;
	}
    }


    /* Now allocate the environment */

    if (!(handle = GlobalAlloc16( GMEM_FIXED, size ))) return 0;
    p = (char *)GlobalLock16( handle );

    /* And fill it with the Unix environment */

    for (e = environ, size = initial_size; *e; e++)
    {
	if (lstrncmpi32A(*e, "path=", 5))
	{
            int len = strlen(*e) + 1;
            if (size + len >= 32767) break;
            strcpy( p, *e );
            size += len;
            p    += len;
	}
    }

    /* Now add the path */

    strcpy( p, "PATH=" );
    for (i = 0, p += 5; ; i++)
    {
        if (!DIR_GetDosPath( i, p, winpathlen )) break;
        p += strlen(p);
        *p++ = ';';
    }
    if (p[-1] == ';') p[-1] = '\0';
    else p++;

    /* Now add the program name */

    *p++ = '\0';
    PUT_WORD( p, 1 );
    p += sizeof(WORD);
    GetSystemDirectory32A( p, sysdirlen );
    strcat( p, "\\" );
    strcat( p, program_name );

    /* Display it */

    p = (char *) GlobalLock16( handle );
    dprintf_task(stddeb, "Master DOS environment at %p\n", p);
    for (; *p; p += strlen(p) + 1) dprintf_task(stddeb, "    %s\n", p);
    dprintf_task( stddeb, "Progname: %s\n", p+3 );

    return handle;
}


/***********************************************************************
 *           TASK_LinkTask
 */
static void TASK_LinkTask( HTASK16 hTask )
{
    HTASK16 *prevTask;
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return;
    prevTask = &hFirstTask;
    while (*prevTask)
    {
        TDB *prevTaskPtr = (TDB *)GlobalLock16( *prevTask );
        if (prevTaskPtr->priority >= pTask->priority) break;
        prevTask = &prevTaskPtr->hNext;
    }
    pTask->hNext = *prevTask;
    *prevTask = hTask;
    nTaskCount++;
}


/***********************************************************************
 *           TASK_UnlinkTask
 */
static void TASK_UnlinkTask( HTASK16 hTask )
{
    HTASK16 *prevTask;
    TDB *pTask;

    prevTask = &hFirstTask;
    while (*prevTask && (*prevTask != hTask))
    {
        pTask = (TDB *)GlobalLock16( *prevTask );
        prevTask = &pTask->hNext;
    }
    if (*prevTask)
    {
        pTask = (TDB *)GlobalLock16( *prevTask );
        *prevTask = pTask->hNext;
        pTask->hNext = 0;
        nTaskCount--;
    }
}


/***********************************************************************
 *           TASK_CreateThunks
 *
 * Create a thunk free-list in segment 'handle', starting from offset 'offset'
 * and containing 'count' entries.
 */
static void TASK_CreateThunks( HGLOBAL16 handle, WORD offset, WORD count )
{
    int i;
    WORD free;
    THUNKS *pThunk;

    pThunk = (THUNKS *)((BYTE *)GlobalLock16( handle ) + offset);
    pThunk->next = 0;
    pThunk->magic = THUNK_MAGIC;
    pThunk->free = (int)&pThunk->thunks - (int)pThunk;
    free = pThunk->free;
    for (i = 0; i < count-1; i++)
    {
        free += 8;  /* Offset of next thunk */
        pThunk->thunks[4*i] = free;
    }
    pThunk->thunks[4*i] = 0;  /* Last thunk */
}


/***********************************************************************
 *           TASK_AllocThunk
 *
 * Allocate a thunk for MakeProcInstance().
 */
static SEGPTR TASK_AllocThunk( HTASK16 hTask )
{
    TDB *pTask;
    THUNKS *pThunk;
    WORD sel, base;
    
    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return 0;
    sel = pTask->hCSAlias;
    pThunk = &pTask->thunks;
    base = (int)pThunk - (int)pTask;
    while (!pThunk->free)
    {
        sel = pThunk->next;
        if (!sel)  /* Allocate a new segment */
        {
            sel = GLOBAL_Alloc( GMEM_FIXED, sizeof(THUNKS) + (MIN_THUNKS-1)*8,
                                pTask->hPDB, TRUE, FALSE, FALSE );
            if (!sel) return (SEGPTR)0;
            TASK_CreateThunks( sel, 0, MIN_THUNKS );
            pThunk->next = sel;
        }
        pThunk = (THUNKS *)GlobalLock16( sel );
        base = 0;
    }
    base += pThunk->free;
    pThunk->free = *(WORD *)((BYTE *)pThunk + pThunk->free);
    return PTR_SEG_OFF_TO_SEGPTR( sel, base );
}


/***********************************************************************
 *           TASK_FreeThunk
 *
 * Free a MakeProcInstance() thunk.
 */
static BOOL TASK_FreeThunk( HTASK16 hTask, SEGPTR thunk )
{
    TDB *pTask;
    THUNKS *pThunk;
    WORD sel, base;
    
    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return 0;
    sel = pTask->hCSAlias;
    pThunk = &pTask->thunks;
    base = (int)pThunk - (int)pTask;
    while (sel && (sel != HIWORD(thunk)))
    {
        sel = pThunk->next;
        pThunk = (THUNKS *)GlobalLock16( sel );
        base = 0;
    }
    if (!sel) return FALSE;
    *(WORD *)((BYTE *)pThunk + LOWORD(thunk) - base) = pThunk->free;
    pThunk->free = LOWORD(thunk) - base;
    return TRUE;
}


/***********************************************************************
 *           TASK_CallToStart
 *
 * 32-bit entry point for a new task. This function is responsible for
 * setting up the registers and jumping to the 16-bit entry point.
 */
#ifndef WINELIB
static void TASK_CallToStart(void)
{
    int cs_reg, ds_reg, ip_reg;
    int exit_code = 1;
    TDB *pTask = (TDB *)GlobalLock16( hCurrentTask );
    NE_MODULE *pModule = MODULE_GetPtr( pTask->hModule );
    SEGTABLEENTRY *pSegTable = NE_SEG_TABLE( pModule );

    IF1632_Saved16_ss = pTask->ss;
    IF1632_Saved16_sp = pTask->sp;

    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        /* FIXME: all this is an ugly hack */

        extern void InitTask( CONTEXT *context );
        extern void PE_InitializeDLLs( HMODULE16 hModule );

        InitTask( NULL );
        InitApp( pTask->hModule );
        __asm__ __volatile__("movw %w0,%%fs"::"r" (pCurrentThread->teb_sel));
        PE_InitializeDLLs( pTask->hModule );
        exit_code = CallTaskStart32((FARPROC32)(pModule->pe_module->load_addr + 
                pModule->pe_module->pe_header->opt_coff.AddressOfEntryPoint) );
        TASK_KillCurrentTask( exit_code );
    }
    else
    {
        /* Registers at initialization must be:
         * ax   zero
         * bx   stack size in bytes
         * cx   heap size in bytes
         * si   previous app instance
         * di   current app instance
         * bp   zero
         * es   selector to the PSP
         * ds   dgroup of the application
         * ss   stack selector
         * sp   top of the stack
         */

        cs_reg = pSegTable[pModule->cs - 1].selector;
        ip_reg = pModule->ip;
        ds_reg = pSegTable[pModule->dgroup - 1].selector;

        dprintf_task( stddeb, "Starting main program: cs:ip=%04x:%04x ds=%04x ss:sp=%04x:%04x\n",
                      cs_reg, ip_reg, ds_reg,
                      IF1632_Saved16_ss, IF1632_Saved16_sp);

        CallTo16_regs_( (FARPROC16)(cs_reg << 16 | ip_reg), ds_reg,
                        pTask->hPDB /*es*/, 0 /*bp*/, 0 /*ax*/,
                        pModule->stack_size /*bx*/, pModule->heap_size /*cx*/,
                        0 /*dx*/, 0 /*si*/, ds_reg /*di*/ );
        /* This should never return */
        fprintf( stderr, "TASK_CallToStart: Main program returned!\n" );
        TASK_KillCurrentTask( 1 );
    }
}
#endif


/***********************************************************************
 *           TASK_CreateTask
 */
HTASK16 TASK_CreateTask( HMODULE16 hModule, HINSTANCE16 hInstance,
                         HINSTANCE16 hPrevInstance, HANDLE16 hEnvironment,
                         LPCSTR cmdLine, UINT16 cmdShow )
{
    HTASK16 hTask;
    TDB *pTask;
    PDB32 *pdb32;
    HGLOBAL16 hParentEnv;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegTable;
    LPSTR name;
    char filename[256];
    char *stack16Top, *stack32Top;
    STACK16FRAME *frame16;
    STACK32FRAME *frame32;
#ifndef WINELIB32
    extern DWORD CALLTO16_RetAddr_word;
#endif
    
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    pSegTable = NE_SEG_TABLE( pModule );

      /* Allocate the task structure */

    hTask = GLOBAL_Alloc( GMEM_FIXED | GMEM_ZEROINIT, sizeof(TDB),
                          hModule, FALSE, FALSE, FALSE );
    if (!hTask) return 0;
    pTask = (TDB *)GlobalLock16( hTask );

      /* Allocate the new environment block */

    if (!(hParentEnv = hEnvironment))
    {
        TDB *pParent = (TDB *)GlobalLock16( hCurrentTask );
        hParentEnv = pParent ? pParent->pdb.environment : hDOSEnvironment;
    }
    /* FIXME: do we really need to make a copy also when */
    /*        we don't use the parent environment? */
    if (!(hEnvironment = GlobalAlloc16( GMEM_FIXED, GlobalSize16(hParentEnv))))
    {
        GlobalFree16( hTask );
        return 0;
    }
    memcpy( GlobalLock16( hEnvironment ), GlobalLock16( hParentEnv ),
            GlobalSize16( hParentEnv ) );

      /* Get current directory */
    
    GetModuleFileName16( hModule, filename, sizeof(filename) );
    name = strrchr(filename, '\\');
    if (name) *(name+1) = 0;

      /* Fill the task structure */

    pTask->nEvents       = 1;  /* So the task can be started */
    pTask->hSelf         = hTask;
    pTask->flags         = 0;

    if (pModule->flags & NE_FFLAGS_WIN32)
    	pTask->flags 	|= TDBF_WIN32;

    pTask->version       = pModule->expected_version;
    pTask->hInstance     = hInstance;
    pTask->hPrevInstance = hPrevInstance;
    pTask->hModule       = hModule;
    pTask->hParent       = hCurrentTask;
    pTask->curdrive      = filename[0] - 'A' + 0x80;
    strcpy( pTask->curdir, filename+2 );
    pTask->magic         = TDB_MAGIC;
    pTask->nCmdShow      = cmdShow;

      /* Create the thunks block */

    TASK_CreateThunks( hTask, (int)&pTask->thunks - (int)pTask, 7 );

      /* Copy the module name */

    name = MODULE_GetModuleName( hModule );
    strncpy( pTask->module_name, name, sizeof(pTask->module_name) );

      /* Allocate a selector for the PDB */

    pTask->hPDB = GLOBAL_CreateBlock( GMEM_FIXED, &pTask->pdb, sizeof(PDB),
                                      hModule, FALSE, FALSE, FALSE, NULL );

      /* Fill the PDB */

    pTask->pdb.int20 = 0x20cd;
    pTask->pdb.dispatcher[0] = 0x9a;  /* ljmp */
#ifndef WINELIB
    *(FARPROC16 *)&pTask->pdb.dispatcher[1] = MODULE_GetEntryPoint( GetModuleHandle("KERNEL"), 102 );  /* KERNEL.102 is DOS3Call() */
#endif
    pTask->pdb.savedint22 = INT_GetHandler( 0x22 );
    pTask->pdb.savedint23 = INT_GetHandler( 0x23 );
    pTask->pdb.savedint24 = INT_GetHandler( 0x24 );
    pTask->pdb.fileHandlesPtr =
        PTR_SEG_OFF_TO_SEGPTR( GlobalHandleToSel(pTask->hPDB),
                               (int)&((PDB *)0)->fileHandles );
    memset( pTask->pdb.fileHandles, 0xff, sizeof(pTask->pdb.fileHandles) );
    pTask->pdb.environment    = hEnvironment;
    pTask->pdb.nbFiles        = 20;
    lstrcpyn32A( pTask->pdb.cmdLine + 1, cmdLine, 127 );
    pTask->pdb.cmdLine[0] = strlen( pTask->pdb.cmdLine + 1 );

      /* Get the compatibility flags */

    pTask->compat_flags = GetProfileInt32A( "Compatibility", name, 0 );

      /* Allocate a code segment alias for the TDB */

    pTask->hCSAlias = GLOBAL_CreateBlock( GMEM_FIXED, (void *)pTask,
                                          sizeof(TDB), pTask->hPDB, TRUE,
                                          FALSE, FALSE, NULL );

      /* Set the owner of the environment block */

    FarSetOwner( pTask->pdb.environment, pTask->hPDB );

      /* Default DTA overwrites command-line */

    pTask->dta = PTR_SEG_OFF_TO_SEGPTR( pTask->hPDB, 
                                (int)&pTask->pdb.cmdLine - (int)&pTask->pdb );

    /* Create the Win32 part of the task */

    pdb32 = PROCESS_Create();
    pTask->thdb = THREAD_Create( pdb32, 0 );

    /* Create the 32-bit stack frame */

    stack32Top = (char*)pTask->thdb->teb.stack_top;
    frame32 = (STACK32FRAME *)stack32Top - 1;
    frame32->saved_esp = (DWORD)stack32Top;
    frame32->edi = 0;
    frame32->esi = 0;
    frame32->edx = 0;
    frame32->ecx = 0;
    frame32->ebx = 0;
    frame32->ebp = 0;
#ifndef WINELIB
    frame32->retaddr = (DWORD)TASK_CallToStart;
    frame32->codeselector = WINE_CODE_SELECTOR;
#endif
    pTask->esp = (DWORD)frame32;

      /* Create the 16-bit stack frame */

    pTask->ss = hInstance;
    pTask->sp = ((pModule->sp != 0) ? pModule->sp :
                 pSegTable[pModule->ss-1].minsize + pModule->stack_size) & ~1;
    stack16Top = (char *)PTR_SEG_OFF_TO_LIN( pTask->ss, pTask->sp );
    frame16 = (STACK16FRAME *)stack16Top - 1;
    frame16->saved_ss = 0;
    frame16->saved_sp = 0;
    frame16->ds = frame16->es = pTask->hInstance;
    frame16->entry_point = 0;
    frame16->entry_ip = OFFSETOF(TASK_RescheduleProc) + 14;
    frame16->entry_cs = SELECTOROF(TASK_RescheduleProc);
    frame16->bp = 0;
#ifndef WINELIB
    frame16->ip = LOWORD( CALLTO16_RetAddr_word );
    frame16->cs = HIWORD( CALLTO16_RetAddr_word );
#endif  /* WINELIB */
    pTask->sp -= sizeof(STACK16FRAME);

      /* If there's no 16-bit stack yet, use a part of the new task stack */
      /* This is only needed to have a stack to switch from on the first  */
      /* call to DirectedYield(). */

    if (!IF1632_Saved16_ss)
    {
        IF1632_Saved16_ss = pTask->ss;
        IF1632_Saved16_sp = pTask->sp;
    }

      /* Add a breakpoint at the start of the task */

#ifndef WINELIB
    if (Options.debug)
    {
        if (pModule->flags & NE_FFLAGS_WIN32)
        {
            DBG_ADDR addr = { 0, pModule->pe_module->load_addr + 
                              pModule->pe_module->pe_header->opt_coff.AddressOfEntryPoint };
            fprintf( stderr, "Win32 task '%s': ", name );
            DEBUG_AddBreakpoint( &addr );
        }
        else
        {
            DBG_ADDR addr = { pSegTable[pModule->cs-1].selector, pModule->ip };
            fprintf( stderr, "Win16 task '%s': ", name );
            DEBUG_AddBreakpoint( &addr );
        }
    }
#endif  /* WINELIB */

      /* Add the task to the linked list */

    TASK_LinkTask( hTask );

    dprintf_task( stddeb, "CreateTask: module='%s' cmdline='%s' task=%04x\n",
                  name, cmdLine, hTask );

    return hTask;
}


/***********************************************************************
 *           TASK_DeleteTask
 */
static void TASK_DeleteTask( HTASK16 hTask )
{
    TDB *pTask;
    HGLOBAL16 hPDB;

    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return;
    hPDB = pTask->hPDB;

    /* Delete the Win32 part of the task */

    PROCESS_Destroy( &pTask->thdb->process->header );
    THREAD_Destroy( &pTask->thdb->header );

    /* Free the task module */

    FreeModule16( pTask->hModule );

    /* Free the selector aliases */

    GLOBAL_FreeBlock( pTask->hCSAlias );
    GLOBAL_FreeBlock( pTask->hPDB );

    /* Free the task structure itself */

    GlobalFree16( hTask );

    /* Free all memory used by this task (including the 32-bit stack, */
    /* the environment block and the thunk segments). */

    GlobalFreeAll( hPDB );
}


/***********************************************************************
 *           TASK_KillCurrentTask
 *
 * Kill the currently running task. As it's not possible to kill the
 * current task like this, it is simply marked for destruction, and will
 * be killed when either TASK_Reschedule or this function is called again 
 * in the context of another task.
 */
void TASK_KillCurrentTask( INT16 exitCode )
{
    extern void EXEC_ExitWindows(void);

    TDB* pTask = (TDB*) GlobalLock16( hCurrentTask );
    if (!pTask) EXEC_ExitWindows();  /* No current task yet */

    /* Perform USER cleanup */

    USER_AppExit( hCurrentTask, pTask->hInstance, pTask->hQueue );

    if (hTaskToKill && (hTaskToKill != hCurrentTask))
    {
        /* If another task is already marked for destruction, */
        /* we can kill it now, as we are in another context.  */ 
        TASK_DeleteTask( hTaskToKill );
    }

    if (nTaskCount <= 1)
    {
        dprintf_task( stddeb, "Killing the last task, exiting\n" );
        EXEC_ExitWindows();
    }

    /* Remove the task from the list to be sure we never switch back to it */
    TASK_UnlinkTask( hCurrentTask );
    if( nTaskCount )
    {
        TDB* p = (TDB *)GlobalLock16( hFirstTask );
        while( p )
        {
            if( p->hYieldTo == hCurrentTask ) p->hYieldTo = 0;
            p = (TDB *)GlobalLock16( p->hNext );
        }
    }

    hTaskToKill = hCurrentTask;
    hLockedTask = 0;

    pTask->nEvents = 0;
    TASK_YieldToSystem(pTask);

    /* We should never return from this Yield() */

    fprintf(stderr,"Return of the living dead %04x!!!\n", hCurrentTask);
    exit(1);
}

/***********************************************************************
 *           TASK_Reschedule
 *
 * This is where all the magic of task-switching happens!
 *
 * Note: This function should only be called via the TASK_YieldToSystem()
 *       wrapper, to make sure that all the context is saved correctly.
 *   
 *       It must not call functions that may yield control.
 */
void TASK_Reschedule(void)
{
    TDB *pOldTask = NULL, *pNewTask;
    HTASK16 hTask = 0;

#ifdef CONFIG_IPC
    dde_reschedule();
#endif
      /* First check if there's a task to kill */

    if (hTaskToKill && (hTaskToKill != hCurrentTask))
    {
        TASK_DeleteTask( hTaskToKill );
        hTaskToKill = 0;
    }

    /* Find a task to yield to */

    pOldTask = (TDB *)GlobalLock16( hCurrentTask );
    if (pOldTask && pOldTask->hYieldTo)
    {
        /* check for DirectedYield() */

        hTask = pOldTask->hYieldTo;
        pNewTask = (TDB *)GlobalLock16( hTask );
        if( !pNewTask || !pNewTask->nEvents) hTask = 0;
        pOldTask->hYieldTo = 0;
    }

    /* extract hardware events only! */

    if (!hTask) EVENT_WaitXEvent( FALSE, TRUE );

    while (!hTask)
    {
        /* Find a task that has an event pending */

        hTask = hFirstTask;
        while (hTask)
        {
            pNewTask = (TDB *)GlobalLock16( hTask );

	    dprintf_task( stddeb, "\ttask = %04x, events = %i\n", hTask, pNewTask->nEvents);

            if (pNewTask->nEvents) break;
            hTask = pNewTask->hNext;
        }
        if (hLockedTask && (hTask != hLockedTask)) hTask = 0;
        if (hTask) break;

        /* No task found, wait for some events to come in */

        EVENT_WaitXEvent( TRUE, TRUE );
    }

    if (hTask == hCurrentTask) 
    {
       dprintf_task( stddeb, "returning to the current task(%04x)\n", hTask );
       return;  /* Nothing to do */
    }
    pNewTask = (TDB *)GlobalLock16( hTask );
    dprintf_task( stddeb, "Switching to task %04x (%.8s)\n",
                  hTask, pNewTask->module_name );

      /* Save the stacks of the previous task (if any) */

    if (pOldTask)
    {
        pOldTask->ss  = IF1632_Saved16_ss;
        pOldTask->sp  = IF1632_Saved16_sp;
        pOldTask->esp = IF1632_Saved32_esp;
    }
    else IF1632_Original32_esp = IF1632_Saved32_esp;

     /* Make the task the last in the linked list (round-robin scheduling) */

    pNewTask->priority++;
    TASK_UnlinkTask( hTask );
    TASK_LinkTask( hTask );
    pNewTask->priority--;

      /* Switch to the new stack */

    hCurrentTask = hTask;
    pCurrentThread = pNewTask->thdb;
    pCurrentProcess = pCurrentThread->process;
    IF1632_Saved16_ss   = pNewTask->ss;
    IF1632_Saved16_sp   = pNewTask->sp;
    IF1632_Saved32_esp  = pNewTask->esp;
}


/***********************************************************************
 *           TASK_YieldToSystem
 *
 * Scheduler interface, this way we ensure that all "unsafe" events are
 * processed outside the scheduler.
 */
void TASK_YieldToSystem(TDB* pTask)
{
  MESSAGEQUEUE*		pQ;

  TASK_SCHEDULE();

  if( pTask )
  {
    pQ = (MESSAGEQUEUE*)GlobalLock16(pTask->hQueue);
    if( pQ && pQ->flags & QUEUE_FLAG_XEVENT )
    {
      pQ->flags &= ~QUEUE_FLAG_XEVENT;
      EVENT_WaitXEvent( FALSE, FALSE );
    }
  }
}


/***********************************************************************
 *           InitTask  (KERNEL.91)
 */
void InitTask( CONTEXT *context )
{
    TDB *pTask;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegTable;
    INSTANCEDATA *pinstance;
    LONG stacklow, stackhi;

    if (context) EAX_reg(context) = 0;
    if (!(pTask = (TDB *)GlobalLock16( hCurrentTask ))) return;
    if (!(pModule = MODULE_GetPtr( pTask->hModule ))) return;

#ifndef WINELIB
    NE_InitializeDLLs( pTask->hModule );
#endif

    if (context)
    {
        /* Registers on return are:
         * ax     1 if OK, 0 on error
         * cx     stack limit in bytes
         * dx     cmdShow parameter
         * si     instance handle of the previous instance
         * di     instance handle of the new task
         * es:bx  pointer to command-line inside PSP
         */
        EAX_reg(context) = 1;
        EBX_reg(context) = 0x81;
        ECX_reg(context) = pModule->stack_size;
        EDX_reg(context) = pTask->nCmdShow;
        ESI_reg(context) = (DWORD)pTask->hPrevInstance;
        EDI_reg(context) = (DWORD)pTask->hInstance;
        ES_reg (context) = (WORD)pTask->hPDB;
    }

    /* Initialize the local heap */
    if ( pModule->heap_size )
    {
        LocalInit( pTask->hInstance, 0, pModule->heap_size );
    }    

    /* Initialize the INSTANCEDATA structure */
    pSegTable = NE_SEG_TABLE( pModule );
    stacklow = pSegTable[pModule->ss - 1].minsize;
    stackhi  = stacklow + pModule->stack_size;
    if (stackhi > 0xffff) stackhi = 0xffff;
    pinstance = (INSTANCEDATA *)PTR_SEG_OFF_TO_LIN(CURRENT_DS, 0);
    pinstance->stackbottom = stackhi; /* yup, that's right. Confused me too. */
    pinstance->stacktop    = stacklow; 
#ifndef WINELIB
    pinstance->stackmin    = IF1632_Saved16_sp;
#endif
}


/***********************************************************************
 *           WaitEvent  (KERNEL.30)
 */
BOOL16 WaitEvent( HTASK16 hTask )
{
    TDB *pTask;

    if (!hTask) hTask = hCurrentTask;
    pTask = (TDB *)GlobalLock16( hTask );
    if (pTask->nEvents > 0)
    {
        pTask->nEvents--;
        return FALSE;
    }
    TASK_YieldToSystem(pTask);

    /* When we get back here, we have an event */

    if (pTask->nEvents > 0) pTask->nEvents--;
    return TRUE;
}


/***********************************************************************
 *           PostEvent  (KERNEL.31)
 */
void PostEvent( HTASK16 hTask )
{
    TDB *pTask;

    if (!hTask) hTask = hCurrentTask;
    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return;
    pTask->nEvents++;
}


/***********************************************************************
 *           SetPriority  (KERNEL.32)
 */
void SetPriority( HTASK16 hTask, INT16 delta )
{
    TDB *pTask;
    INT16 newpriority;

    if (!hTask) hTask = hCurrentTask;
    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return;
    newpriority = pTask->priority + delta;
    if (newpriority < -32) newpriority = -32;
    else if (newpriority > 15) newpriority = 15;

    pTask->priority = newpriority + 1;
    TASK_UnlinkTask( hTask );
    TASK_LinkTask( hTask );
    pTask->priority--;
}


/***********************************************************************
 *           LockCurrentTask  (KERNEL.33)
 */
HTASK16 LockCurrentTask( BOOL16 bLock )
{
    if (bLock) hLockedTask = hCurrentTask;
    else hLockedTask = 0;
    return hLockedTask;
}


/***********************************************************************
 *           IsTaskLocked  (KERNEL.122)
 */
HTASK16 IsTaskLocked(void)
{
    return hLockedTask;
}


/***********************************************************************
 *           OldYield  (KERNEL.117)
 */
void OldYield(void)
{
    TDB *pCurTask;

    pCurTask = (TDB *)GlobalLock16( hCurrentTask );
    if (pCurTask) pCurTask->nEvents++;  /* Make sure we get back here */
    TASK_YieldToSystem(pCurTask);
    if (pCurTask) pCurTask->nEvents--;
}


/***********************************************************************
 *           DirectedYield  (KERNEL.150)
 */
void DirectedYield( HTASK16 hTask )
{
    TDB *pCurTask = (TDB *)GlobalLock16( hCurrentTask );
    pCurTask->hYieldTo = hTask;
    OldYield();
}


/***********************************************************************
 *           UserYield  (USER.332)
 */
void UserYield(void)
{
    TDB *pCurTask = (TDB *)GlobalLock16( hCurrentTask );
    MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock16( pCurTask->hQueue );
    /* Handle sent messages */
    if (queue && (queue->wakeBits & QS_SENDMESSAGE))
        QUEUE_ReceiveMessage( queue );

    OldYield();

    queue = (MESSAGEQUEUE *)GlobalLock16( pCurTask->hQueue );
    if (queue && (queue->wakeBits & QS_SENDMESSAGE))
        QUEUE_ReceiveMessage( queue );
}


/***********************************************************************
 *           Yield  (KERNEL.29)
 */
void Yield(void)
{
    TDB *pCurTask = (TDB *)GlobalLock16( hCurrentTask );
    if (pCurTask) pCurTask->hYieldTo = 0;
    if (pCurTask && pCurTask->hQueue) UserYield();
    else OldYield();
}


/***********************************************************************
 *           MakeProcInstance16  (KERNEL.51)
 */
FARPROC16 MakeProcInstance16( FARPROC16 func, HANDLE16 hInstance )
{
    BYTE *thunk;
    SEGPTR thunkaddr;
    
    if (__winelib) return func; /* func can be called directly in Winelib */
    thunkaddr = TASK_AllocThunk( hCurrentTask );
    if (!thunkaddr) return (FARPROC16)0;
    thunk = PTR_SEG_TO_LIN( thunkaddr );

    dprintf_task( stddeb, "MakeProcInstance(%08lx,%04x): got thunk %08lx\n",
                  (DWORD)func, hInstance, (DWORD)thunkaddr );
    
    *thunk++ = 0xb8;    /* movw instance, %ax */
    *thunk++ = (BYTE)(hInstance & 0xff);
    *thunk++ = (BYTE)(hInstance >> 8);
    *thunk++ = 0xea;    /* ljmp func */
    *(DWORD *)thunk = (DWORD)func;
    return (FARPROC16)thunkaddr;
}


/***********************************************************************
 *           FreeProcInstance16  (KERNEL.52)
 */
void FreeProcInstance16( FARPROC16 func )
{
    dprintf_task( stddeb, "FreeProcInstance(%08lx)\n", (DWORD)func );
    if (!__winelib) TASK_FreeThunk( hCurrentTask, (SEGPTR)func );
}


/**********************************************************************
 *	    GetCodeHandle    (KERNEL.93)
 */
HANDLE16 GetCodeHandle( FARPROC16 proc )
{
    HANDLE16 handle;
    BYTE *thunk = (BYTE *)PTR_SEG_TO_LIN( proc );

    if (__winelib) return 0;

    /* Return the code segment containing 'proc'. */
    /* Not sure if this is really correct (shouldn't matter that much). */

    /* Check if it is really a thunk */
    if ((thunk[0] == 0xb8) && (thunk[3] == 0xea))
        handle = GlobalHandle16( thunk[6] + (thunk[7] << 8) );
    else
        handle = GlobalHandle16( HIWORD(proc) );

    return handle;
}


/***********************************************************************
 *           SetTaskQueue  (KERNEL.34)
 */
HQUEUE16 SetTaskQueue( HTASK16 hTask, HQUEUE16 hQueue )
{
    HQUEUE16 hPrev;
    TDB *pTask;

    if (!hTask) hTask = hCurrentTask;
    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return 0;

    hPrev = pTask->hQueue;
    pTask->hQueue = hQueue;

    TIMER_SwitchQueue( hPrev, hQueue );

    return hPrev;
}


/***********************************************************************
 *           GetTaskQueue  (KERNEL.35)
 */
HQUEUE16 GetTaskQueue( HTASK16 hTask )
{
    TDB *pTask;

    if (!hTask) hTask = hCurrentTask;
    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return 0;
    return pTask->hQueue;
}


/***********************************************************************
 *           SwitchStackTo   (KERNEL.108)
 */
void SwitchStackTo( WORD seg, WORD ptr, WORD top )
{
    TDB *pTask;
    STACK16FRAME *oldFrame, *newFrame;
    INSTANCEDATA *pData;
    UINT16 copySize;

    if (!(pTask = (TDB *)GlobalLock16( hCurrentTask ))) return;
    if (!(pData = (INSTANCEDATA *)GlobalLock16( seg ))) return;
    dprintf_task( stddeb, "SwitchStackTo: old=%04x:%04x new=%04x:%04x\n",
                  IF1632_Saved16_ss, IF1632_Saved16_sp, seg, ptr );

    /* Save the old stack */

    oldFrame           = CURRENT_STACK16;
    pData->old_sp      = IF1632_Saved16_sp;
    pData->old_ss      = IF1632_Saved16_ss;
    pData->stacktop    = top;
    pData->stackmin    = ptr;
    pData->stackbottom = ptr;

    /* Switch to the new stack */

    IF1632_Saved16_ss = pTask->ss = seg;
    IF1632_Saved16_sp = pTask->sp = ptr - sizeof(STACK16FRAME);
    newFrame = CURRENT_STACK16;

    /* Copy the stack frame and the local variables to the new stack */

    copySize = oldFrame->bp - pData->old_sp;
    memcpy( newFrame, oldFrame, MAX( copySize, sizeof(STACK16FRAME) ));
}


/***********************************************************************
 *           SwitchStackBack   (KERNEL.109)
 *
 * Note: the function is declared as 'register' in the spec file in order
 * to make sure all registers are preserved, but we don't use them in any
 * way, so we don't need a CONTEXT* argument.
 */
void SwitchStackBack(void)
{
    TDB *pTask;
    STACK16FRAME *oldFrame, *newFrame;
    INSTANCEDATA *pData;

    if (!(pTask = (TDB *)GlobalLock16( hCurrentTask ))) return;
    if (!(pData = (INSTANCEDATA *)GlobalLock16( IF1632_Saved16_ss ))) return;
    if (!pData->old_ss)
    {
        fprintf( stderr, "SwitchStackBack: no previous SwitchStackTo\n" );
        return;
    }
    dprintf_task( stddeb, "SwitchStackBack: restoring stack %04x:%04x\n",
                  pData->old_ss, pData->old_sp );

    oldFrame = CURRENT_STACK16;

    /* Switch back to the old stack */

    IF1632_Saved16_ss = pTask->ss = pData->old_ss;
    IF1632_Saved16_sp = pTask->sp = pData->old_sp;
    pData->old_ss = pData->old_sp = 0;

    /* Build a stack frame for the return */

    newFrame = CURRENT_STACK16;
    newFrame->saved_ss = oldFrame->saved_ss;
    newFrame->saved_sp = oldFrame->saved_sp;
    newFrame->entry_ip = oldFrame->entry_ip;
    newFrame->entry_cs = oldFrame->entry_cs;
    newFrame->bp       = oldFrame->bp;
    newFrame->ip       = oldFrame->ip;
    newFrame->cs       = oldFrame->cs;
}


/***********************************************************************
 *           GetTaskQueueDS  (KERNEL.118)
 */
#ifndef WINELIB
void GetTaskQueueDS( CONTEXT *context )
{
    DS_reg(context) = GlobalHandleToSel( GetTaskQueue(0) );
}
#endif  /* WINELIB */


/***********************************************************************
 *           GetTaskQueueES  (KERNEL.119)
 */
#ifndef WINELIB
void GetTaskQueueES( CONTEXT *context )
{
    ES_reg(context) = GlobalHandleToSel( GetTaskQueue(0) );
}
#endif  /* WINELIB */


/***********************************************************************
 *           GetCurrentTask   (KERNEL.36)
 */
HTASK16 GetCurrentTask(void)
{
    return hCurrentTask;
}

DWORD WIN16_GetCurrentTask(void)
{
    /* This is the version used by relay code; the first task is */
    /* returned in the high word of the result */
    return MAKELONG( hCurrentTask, hFirstTask );
}


/***********************************************************************
 *           GetCurrentPDB   (KERNEL.37)
 */
HANDLE16 GetCurrentPDB(void)
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( hCurrentTask ))) return 0;
    return pTask->hPDB;
}


/***********************************************************************
 *           GetInstanceData   (KERNEL.54)
 */
INT16 GetInstanceData( HINSTANCE16 instance, WORD buffer, INT16 len )
{
    char *ptr = (char *)GlobalLock16( instance );
    if (!ptr || !len) return 0;
    if ((int)buffer + len >= 0x10000) len = 0x10000 - buffer;
    memcpy( (char *)GlobalLock16(CURRENT_DS) + buffer, ptr + buffer, len );
    return len;
}


/***********************************************************************
 *           GetExeVersion   (KERNEL.105)
 */
WORD GetExeVersion(void)
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( hCurrentTask ))) return 0;
    return pTask->version;
}


/***********************************************************************
 *           SetErrorMode   (KERNEL.107)
 */
UINT SetErrorMode( UINT mode )
{
    TDB *pTask;
    UINT oldMode;

    if (!(pTask = (TDB *)GlobalLock16( hCurrentTask ))) return 0;
    oldMode = pTask->error_mode;
    pTask->error_mode = mode;
    return oldMode;
}


/***********************************************************************
 *           GetDOSEnvironment   (KERNEL.131)
 */
SEGPTR GetDOSEnvironment(void)
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( hCurrentTask ))) return 0;
    return (SEGPTR)WIN16_GlobalLock16( pTask->pdb.environment );
}


/***********************************************************************
 *           GetNumTasks   (KERNEL.152)
 */
UINT16 GetNumTasks(void)
{
    return nTaskCount;
}


/***********************************************************************
 *           GetTaskDS   (KERNEL.155)
 *
 * Note: this function apparently returns a DWORD with LOWORD == HIWORD.
 * I don't think we need to bother with this.
 */
HINSTANCE16 GetTaskDS(void)
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( hCurrentTask ))) return 0;
    return pTask->hInstance;
}


/***********************************************************************
 *           IsTask   (KERNEL.320)
 */
BOOL16 IsTask( HTASK16 hTask )
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return FALSE;
    if (GlobalSize16( hTask ) < sizeof(TDB)) return FALSE;
    return (pTask->magic == TDB_MAGIC);
}


/***********************************************************************
 *           SetTaskSignalProc   (KERNEL.38)
 */
FARPROC16 SetTaskSignalProc( HTASK16 hTask, FARPROC16 proc )
{
    TDB *pTask;
    FARPROC16 oldProc;

    if (!hTask) hTask = hCurrentTask;
    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return NULL;
    oldProc = pTask->userhandler;
    pTask->userhandler = proc;
    return oldProc;
}


/***********************************************************************
 *           SetSigHandler   (KERNEL.140)
 */
WORD SetSigHandler( FARPROC16 newhandler, FARPROC16* oldhandler,
                    UINT16 *oldmode, UINT16 newmode, UINT16 flag )
{
    fprintf(stdnimp,"SetSigHandler(%p,%p,%p,%d,%d), unimplemented.\n",
            newhandler,oldhandler,oldmode,newmode,flag );

    if (flag != 1) return 0;
    if (!newmode) newhandler = NULL;  /* Default handler */
    if (newmode != 4)
    {
        TDB *pTask;

        if (!(pTask = (TDB *)GlobalLock16( hCurrentTask ))) return 0;
        if (oldmode) *oldmode = pTask->signal_flags;
        pTask->signal_flags = newmode;
        if (oldhandler) *oldhandler = pTask->sighandler;
        pTask->sighandler = newhandler;
    }
    return 0;
}


/***********************************************************************
 *           GlobalNotify   (KERNEL.154)
 */
VOID GlobalNotify( FARPROC16 proc )
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( hCurrentTask ))) return;
    pTask->discardhandler = proc;
}


/***********************************************************************
 *           GetExePtr   (KERNEL.133)
 */
HMODULE16 GetExePtr( HANDLE16 handle )
{
    char *ptr;
    HTASK16 hTask;
    HANDLE16 owner;

      /* Check for module handle */

    if (!(ptr = GlobalLock16( handle ))) return 0;
    if (((NE_MODULE *)ptr)->magic == NE_SIGNATURE) return handle;

      /* Check the owner for module handle */

    owner = FarGetOwner( handle );
    if (!(ptr = GlobalLock16( owner ))) return 0;
    if (((NE_MODULE *)ptr)->magic == NE_SIGNATURE) return owner;

      /* Search for this handle and its owner inside all tasks */

    hTask = hFirstTask;
    while (hTask)
    {
        TDB *pTask = (TDB *)GlobalLock16( hTask );
        if ((hTask == handle) ||
            (pTask->hInstance == handle) ||
            (pTask->hQueue == handle) ||
            (pTask->hPDB == handle)) return pTask->hModule;
        if ((hTask == owner) ||
            (pTask->hInstance == owner) ||
            (pTask->hQueue == owner) ||
            (pTask->hPDB == owner)) return pTask->hModule;
        hTask = pTask->hNext;
    }
    return 0;
}


/***********************************************************************
 *           TaskFirst   (TOOLHELP.63)
 */
BOOL16 TaskFirst( TASKENTRY *lpte )
{
    lpte->hNext = hFirstTask;
    return TaskNext( lpte );
}


/***********************************************************************
 *           TaskNext   (TOOLHELP.64)
 */
BOOL16 TaskNext( TASKENTRY *lpte )
{
    TDB *pTask;
    INSTANCEDATA *pInstData;

    dprintf_toolhelp( stddeb, "TaskNext(%p): task=%04x\n", lpte, lpte->hNext );
    if (!lpte->hNext) return FALSE;
    pTask = (TDB *)GlobalLock16( lpte->hNext );
    if (!pTask || pTask->magic != TDB_MAGIC) return FALSE;
    pInstData = (INSTANCEDATA *)PTR_SEG_OFF_TO_LIN( pTask->hInstance, 0 );
    lpte->hTask         = lpte->hNext;
    lpte->hTaskParent   = pTask->hParent;
    lpte->hInst         = pTask->hInstance;
    lpte->hModule       = pTask->hModule;
    lpte->wSS           = pTask->ss;
    lpte->wSP           = pTask->sp;
    lpte->wStackTop     = pInstData->stacktop;
    lpte->wStackMinimum = pInstData->stackmin;
    lpte->wStackBottom  = pInstData->stackbottom;
    lpte->wcEvents      = pTask->nEvents;
    lpte->hQueue        = pTask->hQueue;
    strncpy( lpte->szModule, pTask->module_name, 8 );
    lpte->szModule[8]   = '\0';
    lpte->wPSPOffset    = 0x100;  /*??*/
    lpte->hNext         = pTask->hNext;
    return TRUE;
}


/***********************************************************************
 *           TaskFindHandle   (TOOLHELP.65)
 */
BOOL16 TaskFindHandle( TASKENTRY *lpte, HTASK16 hTask )
{
    lpte->hNext = hTask;
    return TaskNext( lpte );
}


/***********************************************************************
 *           GetAppCompatFlags   (KERNEL.354) (USER32.205)
 */
DWORD GetAppCompatFlags( HTASK32 hTask )
{
    TDB *pTask;

    if (!hTask) hTask = GetCurrentTask();
    if (!(pTask=(TDB *)GlobalLock16( (HTASK16)hTask ))) return 0;
    if (GlobalSize16(hTask) < sizeof(TDB)) return 0;
    return pTask->compat_flags;
}
