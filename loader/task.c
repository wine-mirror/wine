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
#include "directory.h"
#include "dos_fs.h"
#include "file.h"
#include "debugger.h"
#include "global.h"
#include "instance.h"
#include "message.h"
#include "miscemu.h"
#include "module.h"
#include "neexe.h"
#include "options.h"
#include "selectors.h"
#include "toolhelp.h"
#include "stddebug.h"
#include "debug.h"
#include "dde_proc.h"

  /* Min. number of thunks allocated when creating a new segment */
#define MIN_THUNKS  32

  /* 32-bit stack size for each task */
  /* Must not be greater than 64k, or MAKE_SEGPTR won't work */
#define STACK32_SIZE 0x10000

/* ------ Internal variables ------ */

static HTASK hFirstTask = 0;
static HTASK hCurrentTask = 0;
static HTASK hTaskToKill = 0;
static HTASK hLockedTask = 0;
static WORD nTaskCount = 0;
static HANDLE hDOSEnvironment = 0;

/* ------ Internal declarations ------ */

  /* TASK_Reschedule() 16-bit entry point */
static FARPROC TASK_RescheduleProc;

#ifdef WINELIB
#define TASK_SCHEDULE()  TASK_Reschedule();
#else
#define TASK_SCHEDULE()  CallTo16_word_(TASK_RescheduleProc,0)
#endif

static HANDLE TASK_CreateDOSEnvironment(void);

/***********************************************************************
 *           TASK_Init
 */
BOOL TASK_Init(void)
{
    TASK_RescheduleProc = (FARPROC)GetWndProcEntry16( "TASK_Reschedule" );
    if (!(hDOSEnvironment = TASK_CreateDOSEnvironment()))
        fprintf( stderr, "Not enough memory for DOS Environment\n" );
    return (hDOSEnvironment != 0);
}


/***********************************************************************
 *           TASK_CreateDOSEnvironment
 *
 * Create the original DOS environment.
 */
static HANDLE TASK_CreateDOSEnvironment(void)
{
    static const char program_name[] = "KRNL386.EXE";
    char **e, *p;
    int initial_size, size, i, winpathlen, sysdirlen;
    HANDLE handle;

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
    sysdirlen  = GetSystemDirectory( NULL, 0 ) + 1;
    initial_size = 5 + winpathlen +           /* PATH=xxxx */
                   1 +                        /* BYTE 0 at end */
                   sizeof(WORD) +             /* WORD 1 */
                   sysdirlen +                /* program directory */
                   strlen(program_name) + 1;  /* program name */

    /* Compute the total size of the Unix environment (except path) */

    for (e = environ, size = initial_size; *e; e++)
    {
	if (lstrncmpi(*e, "path=", 5))
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

    if (!(handle = GlobalAlloc( GMEM_FIXED, size ))) return 0;
    p = (char *)GlobalLock( handle );

    /* And fill it with the Unix environment */

    for (e = environ, size = initial_size; *e; e++)
    {
	if (lstrncmpi(*e, "path=", 5))
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
    *(WORD *)p = 1;
    p += sizeof(WORD);
    GetSystemDirectory( p, sysdirlen );
    strcat( p, "\\" );
    strcat( p, program_name );

    /* Display it */

    p = (char *) GlobalLock( handle );
    dprintf_task(stddeb, "Master DOS environment at %p\n", p);
    for (; *p; p += strlen(p) + 1) dprintf_task(stddeb, "    %s\n", p);
    dprintf_task( stddeb, "Progname: %s\n", p+3 );

    return handle;
}


/***********************************************************************
 *           TASK_LinkTask
 */
static void TASK_LinkTask( HTASK hTask )
{
    HTASK *prevTask;
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock( hTask ))) return;
    prevTask = &hFirstTask;
    while (*prevTask)
    {
        TDB *prevTaskPtr = (TDB *)GlobalLock( *prevTask );
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
static void TASK_UnlinkTask( HTASK hTask )
{
    HTASK *prevTask;
    TDB *pTask;

    prevTask = &hFirstTask;
    while (*prevTask && (*prevTask != hTask))
    {
        pTask = (TDB *)GlobalLock( *prevTask );
        prevTask = &pTask->hNext;
    }
    if (*prevTask)
    {
        pTask = (TDB *)GlobalLock( *prevTask );
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
static void TASK_CreateThunks( HGLOBAL handle, WORD offset, WORD count )
{
    int i;
    WORD free;
    THUNKS *pThunk;

    pThunk = (THUNKS *)((BYTE *)GlobalLock( handle ) + offset);
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
#ifndef WINELIB32
static SEGPTR TASK_AllocThunk( HTASK hTask )
{
    TDB *pTask;
    THUNKS *pThunk;
    WORD sel, base;
    
    if (!(pTask = (TDB *)GlobalLock( hTask ))) return 0;
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
        pThunk = (THUNKS *)GlobalLock( sel );
        base = 0;
    }
    base += pThunk->free;
    pThunk->free = *(WORD *)((BYTE *)pThunk + pThunk->free);
    return MAKELONG( base, sel );
}
#endif


/***********************************************************************
 *           TASK_FreeThunk
 *
 * Free a MakeProcInstance() thunk.
 */
#ifndef WINELIB32
static BOOL TASK_FreeThunk( HTASK hTask, SEGPTR thunk )
{
    TDB *pTask;
    THUNKS *pThunk;
    WORD sel, base;
    
    if (!(pTask = (TDB *)GlobalLock( hTask ))) return 0;
    sel = pTask->hCSAlias;
    pThunk = &pTask->thunks;
    base = (int)pThunk - (int)pTask;
    while (sel && (sel != HIWORD(thunk)))
    {
        sel = pThunk->next;
        pThunk = (THUNKS *)GlobalLock( sel );
        base = 0;
    }
    if (!sel) return FALSE;
    *(WORD *)((BYTE *)pThunk + LOWORD(thunk) - base) = pThunk->free;
    pThunk->free = LOWORD(thunk) - base;
    return TRUE;
}
#endif


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
    TDB *pTask = (TDB *)GlobalLock( hCurrentTask );
    NE_MODULE *pModule = (NE_MODULE *)GlobalLock( pTask->hModule );
    SEGTABLEENTRY *pSegTable = NE_SEG_TABLE( pModule );

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

    IF1632_Saved16_ss = pTask->ss;
    IF1632_Saved16_sp = pTask->sp;
    dprintf_task( stddeb, "Starting main program: cs:ip=%04x:%04x ds=%04x ss:sp=%04x:%04x\n",
                 cs_reg, ip_reg, ds_reg,
                 IF1632_Saved16_ss, IF1632_Saved16_sp);

    CallTo16_regs_( (FARPROC)(cs_reg << 16 | ip_reg), ds_reg,
                   pTask->hPDB /*es*/, 0 /*bp*/, 0 /*ax*/,
                   pModule->stack_size /*bx*/, pModule->heap_size /*cx*/,
                   0 /*dx*/, 0 /*si*/, ds_reg /*di*/ );

    /* This should never return */
    fprintf( stderr, "TASK_CallToStart: Main program returned!\n" );
    TASK_KillCurrentTask( 1 );
}
#endif


/***********************************************************************
 *           TASK_CreateTask
 */
HTASK TASK_CreateTask( HMODULE hModule, HANDLE hInstance, HANDLE hPrevInstance,
                       HANDLE hEnvironment, char *cmdLine, WORD cmdShow )
{
    HTASK hTask;
    TDB *pTask;
    HANDLE hParentEnv;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegTable;
    LPSTR name;
    char filename[256];
#ifndef WINELIB32
    char *stack16Top, *stack32Top;
    STACK16FRAME *frame16;
    STACK32FRAME *frame32;
    extern DWORD CALL16_RetAddr_word;
#endif
    
    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return 0;
    pSegTable = NE_SEG_TABLE( pModule );

      /* Allocate the task structure */

    hTask = GLOBAL_Alloc( GMEM_FIXED | GMEM_ZEROINIT, sizeof(TDB),
                          hModule, FALSE, FALSE, FALSE );
    if (!hTask) return 0;
    pTask = (TDB *)GlobalLock( hTask );

      /* Allocate the new environment block */

    if (!(hParentEnv = hEnvironment))
    {
        TDB *pParent = (TDB *)GlobalLock( hCurrentTask );
        hParentEnv = pParent ? pParent->pdb.environment : hDOSEnvironment;
    }
    /* FIXME: do we really need to make a copy also when */
    /*        we don't use the parent environment? */
    if (!(hEnvironment = GlobalAlloc( GMEM_FIXED, GlobalSize( hParentEnv ) )))
    {
        GlobalFree( hTask );
        return 0;
    }
    memcpy( GlobalLock( hEnvironment ), GlobalLock( hParentEnv ),
            GlobalSize( hParentEnv ) );

      /* Get current directory */
    
    GetModuleFileName( hModule, filename, sizeof(filename) );
    name = strrchr(filename, '\\');
    if (name) *(name+1) = 0;

      /* Fill the task structure */

    pTask->nEvents       = 1;  /* So the task can be started */
    pTask->hSelf         = hTask;
    pTask->flags         = 0;
    pTask->version       = pModule->expected_version;
    pTask->hInstance     = hInstance;
    pTask->hPrevInstance = hPrevInstance;
    pTask->hModule       = hModule;
    pTask->hParent       = hCurrentTask;
#ifdef WINELIB
    pTask->curdrive      = 'C' - 'A' + 0x80;
    strcpy( pTask->curdir, "\\" );
#else
    pTask->curdrive      = filename[0] - 'A' + 0x80;
    strcpy( pTask->curdir, filename+2 );
#endif
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
#ifndef WINELIB
    pTask->pdb.dispatcher[0] = 0x9a;  /* ljmp */
    *(DWORD *)&pTask->pdb.dispatcher[1] = MODULE_GetEntryPoint( GetModuleHandle("KERNEL"), 102 );  /* KERNEL.102 is DOS3Call() */
    pTask->pdb.savedint22 = INT_GetHandler( 0x22 );
    pTask->pdb.savedint23 = INT_GetHandler( 0x23 );
    pTask->pdb.savedint24 = INT_GetHandler( 0x24 );
    pTask->pdb.fileHandlesPtr = (SEGPTR)MAKELONG( 0x18,
                                              GlobalHandleToSel(pTask->hPDB) );
#else
    pTask->pdb.fileHandlesPtr = pTask->pdb.fileHandles;
#endif
    memset( pTask->pdb.fileHandles, 0xff, sizeof(pTask->pdb.fileHandles) );
    pTask->pdb.environment    = hEnvironment;
    pTask->pdb.nbFiles        = 20;
    lstrcpyn( pTask->pdb.cmdLine + 1, cmdLine, 127 );
    pTask->pdb.cmdLine[0] = strlen( pTask->pdb.cmdLine + 1 );

      /* Get the compatibility flags */

    pTask->compat_flags = GetProfileInt( name, "Compatibility", 0 );

      /* Allocate a code segment alias for the TDB */

    pTask->hCSAlias = GLOBAL_CreateBlock( GMEM_FIXED, (void *)pTask,
                                          sizeof(TDB), pTask->hPDB, TRUE,
                                          FALSE, FALSE, NULL );

      /* Set the owner of the environment block */

    FarSetOwner( pTask->pdb.environment, pTask->hPDB );

      /* Default DTA overwrites command-line */

    pTask->dta = MAKELONG( (int)&pTask->pdb.cmdLine - (int)&pTask->pdb,
                           pTask->hPDB );

      /* Allocate the 32-bit stack */

#ifndef WINELIB
    pTask->hStack32 = GLOBAL_Alloc( GMEM_FIXED, STACK32_SIZE, pTask->hPDB,
                                    FALSE, FALSE, FALSE );

      /* Create the 32-bit stack frame */

    *(DWORD *)GlobalLock(pTask->hStack32) = 0xDEADBEEF;
    stack32Top = (char*)GlobalLock(pTask->hStack32) + STACK32_SIZE;
    frame32 = (STACK32FRAME *)stack32Top - 1;
    frame32->saved_esp = (DWORD)stack32Top;
    frame32->edi = 0;
    frame32->esi = 0;
    frame32->edx = 0;
    frame32->ecx = 0;
    frame32->ebx = 0;
    frame32->ebp = 0;
    frame32->retaddr = (DWORD)TASK_CallToStart;
    frame32->codeselector = WINE_CODE_SELECTOR;
    pTask->esp = (DWORD)frame32;

      /* Create the 16-bit stack frame */

    pTask->ss = hInstance;
    pTask->sp = ((pModule->sp != 0) ? pModule->sp :
                 pSegTable[pModule->ss-1].minsize + pModule->stack_size) & ~1;
    stack16Top = (char *)PTR_SEG_OFF_TO_LIN( pTask->ss, pTask->sp );
    frame16 = (STACK16FRAME *)stack16Top - 1;
    frame16->saved_ss = 0; /*pTask->ss;*/
    frame16->saved_sp = 0; /*pTask->sp;*/
    frame16->ds = frame16->es = pTask->hInstance;
    frame16->entry_point = 0;
    frame16->ordinal_number = 24;  /* WINPROCS.24 is TASK_Reschedule */
    frame16->dll_id = 24; /* WINPROCS */
    frame16->bp = 0;
    frame16->ip = LOWORD( CALL16_RetAddr_word );
    frame16->cs = HIWORD( CALL16_RetAddr_word );
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

    if (Options.debug)
    {
        DBG_ADDR addr = { pSegTable[pModule->cs-1].selector, pModule->ip };
        fprintf( stderr, "Task '%s': ", name );
        DEBUG_AddBreakpoint( &addr );
    }
#endif

      /* Add the task to the linked list */

    TASK_LinkTask( hTask );

    dprintf_task( stddeb, "CreateTask: module='%s' cmdline='%s' task="NPFMT"\n",
                  name, cmdLine, hTask );

    return hTask;
}


/***********************************************************************
 *           TASK_DeleteTask
 */
static void TASK_DeleteTask( HTASK hTask )
{
    TDB *pTask;
    HANDLE hPDB;

    if (!(pTask = (TDB *)GlobalLock( hTask ))) return;
    hPDB = pTask->hPDB;

    /* Free the task module */

    FreeModule( pTask->hModule );

    /* Close all open files of this task */

    FILE_CloseAllFiles( pTask->hPDB );

    /* Free the message queue */

    MSG_DeleteMsgQueue( pTask->hQueue );

    /* Free the selector aliases */

    GLOBAL_FreeBlock( pTask->hCSAlias );
    GLOBAL_FreeBlock( pTask->hPDB );

    /* Free the task structure itself */

    GlobalFree( hTask );

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
void TASK_KillCurrentTask( int exitCode )
{
    if (hTaskToKill && (hTaskToKill != hCurrentTask))
    {
        /* If another task is already marked for destruction, */
        /* we call kill it now, as we are in another context. */
        TASK_DeleteTask( hTaskToKill );
    }

    if (nTaskCount <= 1)
    {
        dprintf_task( stddeb, "Killing the last task, exiting\n" );
        ExitWindows( 0, 0 );
    }

    /* Remove the task from the list to be sure we never switch back to it */
    TASK_UnlinkTask( hCurrentTask );
    
    hTaskToKill = hCurrentTask;
    hLockedTask = 0;

    Yield();
    /* We should never return from this Yield() */

    fprintf(stderr,"It's alive! Alive!!!\n");
    exit(1);
}


/***********************************************************************
 *           TASK_Reschedule
 *
 * This is where all the magic of task-switching happens!
 *
 * This function should only be called via the TASK_SCHEDULE() macro, to make
 * sure that all the context is saved correctly.
 */
void TASK_Reschedule(void)
{
    TDB *pOldTask = NULL, *pNewTask;
    HTASK hTask = 0;

#ifdef CONFIG_IPC
    dde_reschedule();
#endif
      /* First check if there's a task to kill */

    if (hTaskToKill && (hTaskToKill != hCurrentTask))
    {
        TASK_DeleteTask( hTaskToKill );
        hTaskToKill = 0;
    }

      /* If current task is locked, simply return */

    if (hLockedTask) return;

      /* Find a task to yield to */

    pOldTask = (TDB *)GlobalLock( hCurrentTask );
    if (pOldTask && pOldTask->hYieldTo)
    {
        /* If a task is stored in hYieldTo of the current task (put there */
        /* by DirectedYield), yield to it only if it has events pending.  */
        hTask = pOldTask->hYieldTo;
        if (!(pNewTask = (TDB *)GlobalLock( hTask )) || !pNewTask->nEvents)
            hTask = 0;
    }

    if (!hTask)
    {
        hTask = hFirstTask;
        while (hTask)
        {
            pNewTask = (TDB *)GlobalLock( hTask );
            if (pNewTask->nEvents && (hTask != hCurrentTask)) break;
            hTask = pNewTask->hNext;
        }
    }

     /* If there's a task to kill, switch to any other task, */
     /* even if it doesn't have events pending. */

    if (!hTask && hTaskToKill) hTask = hFirstTask;

    if (!hTask) return;  /* Do nothing */

    pNewTask = (TDB *)GlobalLock( hTask );
    dprintf_task( stddeb, "Switching to task "NPFMT" (%.8s)\n",
                  hTask, pNewTask->module_name );

      /* Save the stacks of the previous task (if any) */

#ifndef WINELIB /* FIXME: JBP: IF1632 not allowed in libwine.a */
    if (pOldTask)
    {
        pOldTask->ss  = IF1632_Saved16_ss;
        pOldTask->sp  = IF1632_Saved16_sp;
        pOldTask->esp = IF1632_Saved32_esp;
    }
    else IF1632_Original32_esp = IF1632_Saved32_esp;
#endif

     /* Make the task the last in the linked list (round-robin scheduling) */

    pNewTask->priority++;
    TASK_UnlinkTask( hTask );
    TASK_LinkTask( hTask );
    pNewTask->priority--;

      /* Switch to the new stack */

    hCurrentTask = hTask;
#ifndef WINELIB /* FIXME: JBP: IF1632 not allowed in libwine.a */
    IF1632_Saved16_ss   = pNewTask->ss;
    IF1632_Saved16_sp   = pNewTask->sp;
    IF1632_Saved32_esp  = pNewTask->esp;
    IF1632_Stack32_base = WIN16_GlobalLock( pNewTask->hStack32 );
#endif
}


/***********************************************************************
 *           InitTask  (KERNEL.91)
 */
#ifdef WINELIB
void InitTask(void)
#else
void InitTask( struct sigcontext_struct context )
#endif
{
    static int firstTask = 1;
    TDB *pTask;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegTable;
    INSTANCEDATA *pinstance;
    LONG stacklow, stackhi;

#ifndef WINELIB
    EAX_reg(&context) = 0;
#endif
    if (!(pTask = (TDB *)GlobalLock( hCurrentTask ))) return;
    if (!(pModule = (NE_MODULE *)GlobalLock( pTask->hModule ))) return;

    if (firstTask)
    {
        extern BOOL WIDGETS_Init(void);
        extern BOOL WIN_CreateDesktopWindow(void);

        /* Perform global initialisations that need a task context */

          /* Initialize built-in window classes */
        if (!WIDGETS_Init()) return;

          /* Create desktop window */
        if (!WIN_CreateDesktopWindow()) return;

        firstTask = 0;
    }

#ifndef WINELIB
    NE_InitializeDLLs( pTask->hModule );

    /* Registers on return are:
     * ax     1 if OK, 0 on error
     * cx     stack limit in bytes
     * dx     cmdShow parameter
     * si     instance handle of the previous instance
     * di     instance handle of the new task
     * es:bx  pointer to command-line inside PSP
     */
    EAX_reg(&context) = 1;
    EBX_reg(&context) = 0x81;
    ECX_reg(&context) = pModule->stack_size;
    EDX_reg(&context) = pTask->nCmdShow;
    ESI_reg(&context) = (DWORD)pTask->hPrevInstance;
    EDI_reg(&context) = (DWORD)pTask->hInstance;
    ES_reg (&context) = (WORD)pTask->hPDB;

    /* Initialize the local heap */
    if ( pModule->heap_size )
    {
        LocalInit( pTask->hInstance, 0, pModule->heap_size );
    }    
#endif


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
BOOL WaitEvent( HTASK hTask )
{
    TDB *pTask;

    if (!hTask) hTask = hCurrentTask;
    pTask = (TDB *)GlobalLock( hTask );
    if (pTask->nEvents > 0)
    {
        pTask->nEvents--;
        return FALSE;
    }
    TASK_SCHEDULE();
    /* When we get back here, we have an event (or the task is the only one) */
    if (pTask->nEvents > 0) pTask->nEvents--;
    return TRUE;
}


/***********************************************************************
 *           PostEvent  (KERNEL.31)
 */
void PostEvent( HTASK hTask )
{
    TDB *pTask;

    if (!hTask) hTask = hCurrentTask;
    if (!(pTask = (TDB *)GlobalLock( hTask ))) return;
    pTask->nEvents++;
}


/***********************************************************************
 *           SetPriority  (KERNEL.32)
 */
void SetPriority( HTASK hTask, int delta )
{
    TDB *pTask;
    int newpriority;

    if (!hTask) hTask = hCurrentTask;
    if (!(pTask = (TDB *)GlobalLock( hTask ))) return;
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
HTASK LockCurrentTask( BOOL bLock )
{
    if (bLock) hLockedTask = hCurrentTask;
    else hLockedTask = 0;
    return hLockedTask;
}


/***********************************************************************
 *           IsTaskLocked  (KERNEL.122)
 */
HTASK IsTaskLocked(void)
{
    return hLockedTask;
}


/***********************************************************************
 *           OldYield  (KERNEL.117)
 */
void OldYield(void)
{
    TDB *pCurTask;

    pCurTask = (TDB *)GlobalLock( hCurrentTask );
    if (pCurTask) pCurTask->nEvents++;  /* Make sure we get back here */
    TASK_SCHEDULE();
    if (pCurTask) pCurTask->nEvents--;
}


/***********************************************************************
 *           DirectedYield  (KERNEL.150)
 */
void DirectedYield( HTASK hTask )
{
    TDB *pCurTask;

    if ((pCurTask = (TDB *)GlobalLock( hCurrentTask )) != NULL)
        pCurTask->hYieldTo = hTask;

    OldYield();
}


/***********************************************************************
 *           Yield  (KERNEL.29)
 */
void Yield(void)
{
    DirectedYield( 0 );
}


/***********************************************************************
 *           MakeProcInstance  (KERNEL.51)
 */
FARPROC MakeProcInstance( FARPROC func, HANDLE hInstance )
{
#ifdef WINELIB32
    return func; /* func can be called directly in Win32 */
#else
    BYTE *thunk;
    SEGPTR thunkaddr;
    
    thunkaddr = TASK_AllocThunk( hCurrentTask );
    if (!thunkaddr) return (FARPROC)0;
    thunk = PTR_SEG_TO_LIN( thunkaddr );

    dprintf_task( stddeb, "MakeProcInstance("SPFMT","NPFMT"): got thunk "SPFMT"\n",
                  (SEGPTR)func, hInstance, (SEGPTR)thunkaddr );
    
    *thunk++ = 0xb8;    /* movw instance, %ax */
    *thunk++ = (BYTE)(hInstance & 0xff);
    *thunk++ = (BYTE)(hInstance >> 8);
    *thunk++ = 0xea;    /* ljmp func */
    *(DWORD *)thunk = (DWORD)func;
    return (FARPROC)thunkaddr;
#endif
}


/***********************************************************************
 *           FreeProcInstance  (KERNEL.52)
 */
void FreeProcInstance( FARPROC func )
{
#ifndef WINELIB32
    dprintf_task( stddeb, "FreeProcInstance("SPFMT")\n", (SEGPTR)func );
    TASK_FreeThunk( hCurrentTask, (SEGPTR)func );
#endif
}


/**********************************************************************
 *	    GetCodeHandle    (KERNEL.93)
 */
HANDLE GetCodeHandle( FARPROC proc )
{
    HANDLE handle;
    BYTE *thunk = (BYTE *)PTR_SEG_TO_LIN( proc );

    /* Return the code segment containing 'proc'. */
    /* Not sure if this is really correct (shouldn't matter that much). */

    /* Check if it is really a thunk */
    if ((thunk[0] == 0xb8) && (thunk[3] == 0xea))
        handle = GlobalHandle( thunk[6] + (thunk[7] << 8) );
    else
        handle = GlobalHandle( HIWORD(proc) );

    return handle;
}


/***********************************************************************
 *           SetTaskQueue  (KERNEL.34)
 */
HGLOBAL SetTaskQueue( HANDLE hTask, HGLOBAL hQueue )
{
    HGLOBAL hPrev;
    TDB *pTask;

    if (!hTask) hTask = hCurrentTask;
    if (!(pTask = (TDB *)GlobalLock( hTask ))) return 0;
    hPrev = pTask->hQueue;
    pTask->hQueue = hQueue;
    return hPrev;
}


/***********************************************************************
 *           GetTaskQueue  (KERNEL.35)
 */
HGLOBAL GetTaskQueue( HANDLE hTask )
{
    TDB *pTask;

    if (!hTask) hTask = hCurrentTask;
    if (!(pTask = (TDB *)GlobalLock( hTask ))) return 0;
    return pTask->hQueue;
}


/***********************************************************************
 *           GetTaskQueueDS  (KERNEL.118)
 */
#ifndef WINELIB
void GetTaskQueueDS( struct sigcontext_struct context )
{
    DS_reg(&context) = GlobalHandleToSel( GetTaskQueue(0) );
}
#endif  /* WINELIB */


/***********************************************************************
 *           GetTaskQueueES  (KERNEL.119)
 */
#ifndef WINELIB
void GetTaskQueueES( struct sigcontext_struct context )
{
    ES_reg(&context) = GlobalHandleToSel( GetTaskQueue(0) );
}
#endif  /* WINELIB */


/***********************************************************************
 *           GetCurrentTask   (KERNEL.36)
 */
HTASK GetCurrentTask(void)
{
      /* Undocumented: first task is returned in high word */
#ifdef WINELIB32
    return hCurrentTask;
#else
    return MAKELONG( hCurrentTask, hFirstTask );
#endif
}


/***********************************************************************
 *           GetCurrentPDB   (KERNEL.37)
 */
HANDLE GetCurrentPDB(void)
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock( hCurrentTask ))) return 0;
    return pTask->hPDB;
}


/***********************************************************************
 *           GetInstanceData   (KERNEL.54)
 */
int GetInstanceData( HANDLE instance, WORD buffer, int len )
{
    char *ptr = (char *)GlobalLock( instance );
    if (!ptr || !len) return 0;
    if ((int)buffer + len >= 0x10000) len = 0x10000 - buffer;
    memcpy( ptr + buffer, (char *)GlobalLock( CURRENT_DS ) + buffer, len );
    return len;
}


/***********************************************************************
 *           SetErrorMode   (KERNEL.107)
 */
UINT SetErrorMode( UINT mode )
{
    TDB *pTask;
    UINT oldMode;

    if (!(pTask = (TDB *)GlobalLock( hCurrentTask ))) return 0;
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

    if (!(pTask = (TDB *)GlobalLock( hCurrentTask ))) return 0;
    return (SEGPTR)WIN16_GlobalLock( pTask->pdb.environment );
}


/***********************************************************************
 *           GetNumTasks   (KERNEL.152)
 */
WORD GetNumTasks(void)
{
    return nTaskCount;
}


/***********************************************************************
 *           GetTaskDS   (KERNEL.155)
 */
HINSTANCE GetTaskDS(void)
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock( hCurrentTask ))) return 0;
    return pTask->hInstance;
}


/***********************************************************************
 *           IsTask   (KERNEL.320)
 */
BOOL IsTask( HTASK hTask )
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock( hTask ))) return FALSE;
    if (GlobalSize( hTask ) < sizeof(TDB)) return FALSE;
    return (pTask->magic == TDB_MAGIC);
}


/***********************************************************************
 *           GetExePtr   (KERNEL.133)
 */
HMODULE GetExePtr( HANDLE handle )
{
    char *ptr;
    HTASK hTask;
    HANDLE owner;

      /* Check for module handle */

    if (!(ptr = GlobalLock( handle ))) return 0;
    if (((NE_MODULE *)ptr)->magic == NE_SIGNATURE) return handle;
	/* Fake modules describing PE modules have a PE signature */
    if (((NE_MODULE *)ptr)->magic == PE_SIGNATURE) return handle;

      /* Check the owner for module handle */

#ifndef WINELIB
    owner = FarGetOwner( handle );
#else
    owner = NULL;
#endif
    if (!(ptr = GlobalLock( owner ))) return 0;
    if (((NE_MODULE *)ptr)->magic == NE_SIGNATURE) return owner;
    if (((NE_MODULE *)ptr)->magic == PE_SIGNATURE) return owner;

      /* Search for this handle and its owner inside all tasks */

    hTask = hFirstTask;
    while (hTask)
    {
        TDB *pTask = (TDB *)GlobalLock( hTask );
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
BOOL TaskFirst( TASKENTRY *lpte )
{
    lpte->hNext = hFirstTask;
    return TaskNext( lpte );
}


/***********************************************************************
 *           TaskNext   (TOOLHELP.64)
 */
BOOL TaskNext( TASKENTRY *lpte )
{
    TDB *pTask;
    INSTANCEDATA *pInstData;

    dprintf_toolhelp( stddeb, "TaskNext(%p): task="NPFMT"\n", lpte, lpte->hNext );
    if (!lpte->hNext) return FALSE;
    pTask = (TDB *)GlobalLock( lpte->hNext );
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
BOOL TaskFindHandle( TASKENTRY *lpte, HTASK hTask )
{
    lpte->hNext = hTask;
    return TaskNext( lpte );
}
