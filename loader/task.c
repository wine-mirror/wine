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
#include "global.h"
#include "instance.h"
#include "miscemu.h"
#include "module.h"
#include "neexe.h"
#include "selectors.h"
#include "toolhelp.h"
#include "wine.h"
#include "stddebug.h"
#include "debug.h"

  /* Min. number of thunks allocated when creating a new segment */
#define MIN_THUNKS  32

  /* 32-bit stack size for each task */
  /* Must not be greater than 64k, or MAKE_SEGPTR won't work */
#define STACK32_SIZE 0x10000


static HTASK hFirstTask = 0;
static HTASK hCurrentTask = 0;
static HTASK hTaskToKill = 0;
static HTASK hLockedTask = 0;
static WORD nTaskCount = 0;

  /* TASK_Reschedule() 16-bit entry point */
static FARPROC TASK_RescheduleProc;

#define TASK_SCHEDULE()  CallTo16_word_(TASK_RescheduleProc,0)


/***********************************************************************
 *           TASK_Init
 */
BOOL TASK_Init(void)
{
    TASK_RescheduleProc = (FARPROC)GetWndProcEntry16( "TASK_Reschedule" );
    return TRUE;
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


/***********************************************************************
 *           TASK_FreeThunk
 *
 * Free a MakeProcInstance() thunk.
 */
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


/***********************************************************************
 *           TASK_CallToStart
 *
 * 32-bit entry point for a new task. This function is responsible for
 * setting up the registers and jumping to the 16-bit entry point.
 */
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
    exit(1);
}


/***********************************************************************
 *           TASK_CreateTask
 */
HTASK TASK_CreateTask( HMODULE hModule, HANDLE hInstance, HANDLE hPrevInstance,
                       HANDLE hEnvironment, char *cmdLine, WORD cmdShow )
{
    HTASK hTask;
    TDB *pTask;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegTable;
    LPSTR name;
    char *stack16Top, *stack32Top;
    STACK16FRAME *frame16;
    STACK32FRAME *frame32;
    extern DWORD CALL16_RetAddr_word;

    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return 0;
    pSegTable = NE_SEG_TABLE( pModule );

      /* Allocate the task structure */

    hTask = GLOBAL_Alloc( GMEM_FIXED | GMEM_ZEROINIT, sizeof(TDB),
                          hModule, FALSE, FALSE, FALSE );
    if (!hTask) return 0;
    pTask = (TDB *)GlobalLock( hTask );

      /* Fill the task structure */

    pTask->nEvents       = 1;  /* So the task can be started */
    pTask->hSelf         = hTask;
    pTask->flags         = 0;
    pTask->version       = pModule->expected_version;
    pTask->hInstance     = hInstance;
    pTask->hPrevInstance = hPrevInstance;
    pTask->hModule       = hModule;
    pTask->hParent       = hCurrentTask;
    pTask->curdrive      = 'C' - 'A' + 0x80;
    pTask->magic         = TDB_MAGIC;
    pTask->nCmdShow      = cmdShow;
    strcpy( pTask->curdir, "WINDOWS" );

      /* Create the thunks block */

    TASK_CreateThunks( hTask, (int)&pTask->thunks - (int)pTask, 7 );

      /* Copy the module name */

    name = MODULE_GetModuleName( hModule );
    strncpy( pTask->module_name, name, sizeof(pTask->module_name) );

      /* Fill the PDB */

    pTask->pdb.int20 = 0x20cd;
    pTask->pdb.dispatcher[0] = 0x9a;  /* ljmp */
    *(DWORD *)&pTask->pdb.dispatcher[1] = MODULE_GetEntryPoint( GetModuleHandle("KERNEL"), 102 );  /* KERNEL.102 is DOS3Call() */
    pTask->pdb.savedint22 = INT_GetHandler( 0x22 );
    pTask->pdb.savedint23 = INT_GetHandler( 0x23 );
    pTask->pdb.savedint24 = INT_GetHandler( 0x24 );
    pTask->pdb.environment = hEnvironment;
    strncpy( pTask->pdb.cmdLine + 1, cmdLine, 126 );
    pTask->pdb.cmdLine[127] = '\0';
    pTask->pdb.cmdLine[0] = strlen( pTask->pdb.cmdLine + 1 );

      /* Get the compatibility flags */

    pTask->compat_flags = GetProfileInt( name, "Compatibility", 0 );

      /* Allocate a selector for the PDB */

    pTask->hPDB = GLOBAL_CreateBlock( GMEM_FIXED, &pTask->pdb, sizeof(PDB),
                                      hModule, FALSE, FALSE, FALSE );

      /* Allocate a code segment alias for the TDB */

    pTask->hCSAlias = GLOBAL_CreateBlock( GMEM_FIXED, (void *)pTask,
                                          sizeof(TDB), pTask->hPDB, TRUE,
                                          FALSE, FALSE );

      /* Set the owner of the environment block */

    FarSetOwner( pTask->pdb.environment, pTask->hPDB );

      /* Default DTA overwrites command-line */

    pTask->dta = MAKELONG( (int)&pTask->pdb.cmdLine - (int)&pTask->pdb,
                           pTask->hPDB );

      /* Allocate the 32-bit stack */

    pTask->hStack32 = GLOBAL_Alloc( GMEM_FIXED, STACK32_SIZE, pTask->hPDB,
                                    FALSE, FALSE, FALSE );

      /* Create the 32-bit stack frame */

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
    pTask->sp = (pModule->sp != 0) ? pModule->sp :
                pSegTable[pModule->ss-1].minsize + pModule->stack_size;
    stack16Top = (char *)PTR_SEG_OFF_TO_LIN( pTask->ss, pTask->sp );
    frame16 = (STACK16FRAME *)stack16Top - 1;
    frame16->saved_ss = pTask->ss;
    frame16->saved_sp = pTask->sp;
    frame16->ds = pTask->hInstance;
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

      /* Add the task to the linked list */

    TASK_LinkTask( hTask );

    dprintf_task( stddeb, "CreateTask: module='%s' cmdline='%s' task=%04x\n",
                  name, cmdLine, hTask );

    return hTask;
}


/***********************************************************************
 *           TASK_DeleteTask
 */
void TASK_DeleteTask( HTASK hTask )
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock( hTask ))) return;

      /* Free the task module */

    FreeModule( pTask->hModule );

      /* Free all memory used by this task (including the 32-bit stack, */
      /* the environment block and the thunk segments). */

    GlobalFreeAll( pTask->hPDB );

      /* Free the selector aliases */

    GLOBAL_FreeBlock( pTask->hCSAlias );
    GLOBAL_FreeBlock( pTask->hPDB );

      /* Free the task structure itself */

    GlobalFree( hTask );
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
        exit(0);
    }

    /* Remove the task from the list to be sure we never switch back to it */
    TASK_UnlinkTask( hCurrentTask );
    
    hTaskToKill = hCurrentTask;
    hLockedTask = 0;
    Yield();
    /* We never return from Yield() */
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

      /* First check if there's a task to kill */

    if (hTaskToKill && (hTaskToKill != hCurrentTask))
        TASK_DeleteTask( hTaskToKill );

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
    IF1632_Saved16_ss   = pNewTask->ss;
    IF1632_Saved16_sp   = pNewTask->sp;
    IF1632_Saved32_esp  = pNewTask->esp;
    IF1632_Stack32_base = WIN16_GlobalLock( pNewTask->hStack32 );
}


/***********************************************************************
 *           InitTask  (KERNEL.91)
 */
void InitTask( struct sigcontext_struct context )
{
    static int firstTask = 1;
    TDB *pTask;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegTable;
    INSTANCEDATA *pinstance;
    LONG stacklow, stackhi;

    context.sc_eax = 0;
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

    NE_InitializeDLLs( pTask->hModule );

    /* Registers on return are:
     * ax     1 if OK, 0 on error
     * cx     stack limit in bytes
     * dx     cmdShow parameter
     * si     instance handle of the previous instance
     * di     instance handle of the new task
     * es:bx  pointer to command-line inside PSP
     */
    context.sc_eax = 1;
    context.sc_ebx = 0x81;
    context.sc_ecx = pModule->stack_size;
    context.sc_edx = pTask->nCmdShow;
    context.sc_esi = pTask->hPrevInstance;
    context.sc_edi = pTask->hInstance;
    context.sc_es  = pTask->hPDB;

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
    pinstance->stackmin    = IF1632_Saved16_sp;
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
WORD IsTaskLocked(void)
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
    BYTE *thunk;
    SEGPTR thunkaddr;
    
    thunkaddr = TASK_AllocThunk( hCurrentTask );
    if (!thunkaddr) return (FARPROC)0;
    thunk = PTR_SEG_TO_LIN( thunkaddr );

    dprintf_task( stddeb, "MakeProcInstance(%08lx,%04x): got thunk %08lx\n",
                  (SEGPTR)func, hInstance, (SEGPTR)thunkaddr );
    
    *thunk++ = 0xb8;    /* movw instance, %ax */
    *thunk++ = (BYTE)(hInstance & 0xff);
    *thunk++ = (BYTE)(hInstance >> 8);
    *thunk++ = 0xea;    /* ljmp func */
    *(DWORD *)thunk = (DWORD)func;
    return (FARPROC)thunkaddr;
}


/***********************************************************************
 *           FreeProcInstance  (KERNEL.52)
 */
void FreeProcInstance( FARPROC func )
{
    dprintf_task( stddeb, "FreeProcInstance(%08lx)\n", (SEGPTR)func );
    TASK_FreeThunk( hCurrentTask, (SEGPTR)func );
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

    printf( "STUB: GetCodeHandle(%08lx) returning %04x\n",
            (DWORD)proc, handle );
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
 *           GetCurrentTask   (KERNEL.36)
 */
HTASK GetCurrentTask(void)
{
      /* Undocumented: first task is returned in high word */
    return MAKELONG( hCurrentTask, hFirstTask );
}


/***********************************************************************
 *           GetCurrentPDB   (KERNEL.37)
 */
WORD GetCurrentPDB(void)
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
 *           GetNumTasks   (KERNEL.152)
 */
WORD GetNumTasks(void)
{
    return nTaskCount;
}


/***********************************************************************
 *           GetTaskDS   (KERNEL.155)
 */
WORD GetTaskDS(void)
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

      /* Check the owner for module handle */

    owner = FarGetOwner( handle );
    if (!(ptr = GlobalLock( owner ))) return 0;
    if (((NE_MODULE *)ptr)->magic == NE_SIGNATURE) return owner;

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

    dprintf_toolhelp( stddeb, "TaskNext(%p): task=%04x\n", lpte, lpte->hNext );
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
