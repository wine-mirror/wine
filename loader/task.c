/*
 * Task functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "wine/winbase16.h"
#include "user.h"
#include "callback.h"
#include "drive.h"
#include "file.h"
#include "global.h"
#include "instance.h"
#include "message.h"
#include "miscemu.h"
#include "module.h"
#include "neexe.h"
#include "pe_image.h"
#include "process.h"
#include "queue.h"
#include "selectors.h"
#include "stackframe.h"
#include "task.h"
#include "thread.h"
#include "toolhelp.h"
#include "winnt.h"
#include "winsock.h"
#include "syslevel.h"
#include "debugtools.h"
#include "dosexe.h"
#include "services.h"
#include "server.h"

DEFAULT_DEBUG_CHANNEL(task)
DECLARE_DEBUG_CHANNEL(relay)
DECLARE_DEBUG_CHANNEL(toolhelp)

  /* Min. number of thunks allocated when creating a new segment */
#define MIN_THUNKS  32

  /* Pointer to function to switch to a larger stack */
int (*IF1632_CallLargeStack)( int (*func)(), void *arg ) = NULL;

static THHOOK DefaultThhook = { 0 };
THHOOK *pThhook = &DefaultThhook;

#define hCurrentTask (pThhook->CurTDB)
#define hFirstTask   (pThhook->HeadTDB)
#define hLockedTask  (pThhook->LockTDB)

static UINT16 nTaskCount = 0;

static HTASK initial_task;

/***********************************************************************
 *	     TASK_InstallTHHook
 */
void TASK_InstallTHHook( THHOOK *pNewThhook )
{
     THHOOK *pOldThhook = pThhook;

     pThhook = pNewThhook? pNewThhook : &DefaultThhook;

     *pThhook = *pOldThhook;
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
void TASK_CallToStart(void)
{
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
    NE_MODULE *pModule = NE_GetPtr( pTask->hModule );
    SEGTABLEENTRY *pSegTable = NE_SEG_TABLE( pModule );
    CONTEXT86 context;

    SYSLEVEL_EnterWin16Lock();

    /* Add task to 16-bit scheduler pool if necessary */
    if ( hCurrentTask != GetCurrentTask() )
        TASK_Reschedule();

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

    memset( &context, 0, sizeof(context) );
    CS_reg(&context)  = GlobalHandleToSel16(pSegTable[pModule->cs - 1].hSeg);
    DS_reg(&context)  = GlobalHandleToSel16(pTask->hInstance);
    ES_reg(&context)  = pTask->hPDB;
    EIP_reg(&context) = pModule->ip;
    EBX_reg(&context) = pModule->stack_size;
    ECX_reg(&context) = pModule->heap_size;
    EDI_reg(&context) = pTask->hInstance;
    ESI_reg(&context) = pTask->hPrevInstance;

    TRACE("Starting main program: cs:ip=%04lx:%04lx ds=%04lx ss:sp=%04x:%04x\n",
          CS_reg(&context), EIP_reg(&context), DS_reg(&context),
          SELECTOROF(pTask->teb->cur_stack),
          OFFSETOF(pTask->teb->cur_stack) );

    Callbacks->CallRegisterShortProc( &context, 0 );
}


/***********************************************************************
 *           TASK_Create
 *
 * NOTE: This routine might be called by a Win32 thread. Thus, we need
 *       to be careful to protect global data structures. We do this
 *       by entering the Win16Lock while linking the task into the
 *       global task list.
 */
BOOL TASK_Create( NE_MODULE *pModule, UINT16 cmdShow)
{
    HTASK16 hTask;
    TDB *pTask;
    LPSTR cmd_line;
    char name[10];
    PDB *pdb32 = PROCESS_Current();

      /* Allocate the task structure */

    hTask = GLOBAL_Alloc( GMEM_FIXED | GMEM_ZEROINIT, sizeof(TDB),
                          pModule->self, FALSE, FALSE, FALSE );
    if (!hTask) return FALSE;
    pTask = (TDB *)GlobalLock16( hTask );

    /* Fill the task structure */

    pTask->nEvents       = 0;
    pTask->hSelf         = hTask;
    pTask->flags         = 0;

    if (pModule->flags & NE_FFLAGS_WIN32)
    	pTask->flags 	|= TDBF_WIN32;
    if (pModule->lpDosTask)
    	pTask->flags 	|= TDBF_WINOLDAP;

    pTask->hInstance     = pModule->self;
    pTask->hPrevInstance = 0;
    /* NOTE: for 16-bit tasks, the instance handles are updated later on
             in NE_InitProcess */

    pTask->version       = pModule->expected_version;
    pTask->hModule       = pModule->self;
    pTask->hParent       = GetCurrentTask();
    pTask->magic         = TDB_MAGIC;
    pTask->nCmdShow      = cmdShow;
    pTask->teb           = NtCurrentTeb();
    pTask->curdrive      = DRIVE_GetCurrentDrive() | 0x80;
    strcpy( pTask->curdir, "\\" );
    lstrcpynA( pTask->curdir + 1, DRIVE_GetDosCwd( DRIVE_GetCurrentDrive() ),
                 sizeof(pTask->curdir) - 1 );

      /* Create the thunks block */

    TASK_CreateThunks( hTask, (int)&pTask->thunks - (int)pTask, 7 );

      /* Copy the module name */

    GetModuleName16( pModule->self, name, sizeof(name) );
    strncpy( pTask->module_name, name, sizeof(pTask->module_name) );

      /* Allocate a selector for the PDB */

    pTask->hPDB = GLOBAL_CreateBlock( GMEM_FIXED, &pTask->pdb, sizeof(PDB16),
                                    pModule->self, FALSE, FALSE, FALSE, NULL );

      /* Fill the PDB */

    pTask->pdb.int20 = 0x20cd;
    pTask->pdb.dispatcher[0] = 0x9a;  /* ljmp */
    PUT_DWORD(&pTask->pdb.dispatcher[1], (DWORD)NE_GetEntryPoint(
           GetModuleHandle16("KERNEL"), 102 ));  /* KERNEL.102 is DOS3Call() */
    pTask->pdb.savedint22 = INT_GetPMHandler( 0x22 );
    pTask->pdb.savedint23 = INT_GetPMHandler( 0x23 );
    pTask->pdb.savedint24 = INT_GetPMHandler( 0x24 );
    pTask->pdb.fileHandlesPtr =
        PTR_SEG_OFF_TO_SEGPTR( GlobalHandleToSel16(pTask->hPDB),
                               (int)&((PDB16 *)0)->fileHandles );
    pTask->pdb.hFileHandles = 0;
    memset( pTask->pdb.fileHandles, 0xff, sizeof(pTask->pdb.fileHandles) );
    pTask->pdb.environment    = pdb32->env_db->env_sel;
    pTask->pdb.nbFiles        = 20;

    /* Fill the command line */

    cmd_line = pdb32->env_db->cmd_line;
    while (*cmd_line && (*cmd_line != ' ') && (*cmd_line != '\t')) cmd_line++;
    while ((*cmd_line == ' ') || (*cmd_line == '\t')) cmd_line++;
    lstrcpynA( pTask->pdb.cmdLine+1, cmd_line, sizeof(pTask->pdb.cmdLine)-1);
    pTask->pdb.cmdLine[0] = strlen( pTask->pdb.cmdLine + 1 );

      /* Get the compatibility flags */

    pTask->compat_flags = GetProfileIntA( "Compatibility", name, 0 );

      /* Allocate a code segment alias for the TDB */

    pTask->hCSAlias = GLOBAL_CreateBlock( GMEM_FIXED, (void *)pTask,
                                          sizeof(TDB), pTask->hPDB, TRUE,
                                          FALSE, FALSE, NULL );

      /* Set the owner of the environment block */

    FarSetOwner16( pTask->pdb.environment, pTask->hPDB );

      /* Default DTA overwrites command-line */

    pTask->dta = PTR_SEG_OFF_TO_SEGPTR( pTask->hPDB, 
                                (int)&pTask->pdb.cmdLine - (int)&pTask->pdb );

    /* Create scheduler event for 16-bit tasks */

    if ( !(pTask->flags & TDBF_WIN32) )
    {
        pTask->hEvent = CreateEventA( NULL, TRUE, FALSE, NULL );
        pTask->hEvent = ConvertToGlobalHandle( pTask->hEvent );
    }

    /* Enter task handle into thread and process */
 
    pTask->teb->htask16 = pTask->teb->process->task = hTask;
    if (!initial_task) initial_task = hTask;

    TRACE("module='%s' cmdline='%s' task=%04x\n", name, cmd_line, hTask );

    /* Add the task to the linked list */

    SYSLEVEL_EnterWin16Lock();
    TASK_LinkTask( hTask );
    SYSLEVEL_LeaveWin16Lock();

    return TRUE;
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

    pTask->magic = 0xdead; /* invalidate signature */

    /* Delete the Win32 part of the task */

/*    PROCESS_FreePDB( pTask->teb->process ); FIXME */
/*    K32OBJ_DecCount( &pTask->teb->header ); FIXME */

    /* Free the selector aliases */

    GLOBAL_FreeBlock( pTask->hCSAlias );
    GLOBAL_FreeBlock( pTask->hPDB );

    /* Free the task module */

    FreeModule16( pTask->hModule );

    /* Free the task structure itself */

    GlobalFree16( hTask );

    /* Free all memory used by this task (including the 32-bit stack, */
    /* the environment block and the thunk segments). */

    GlobalFreeAll16( hPDB );
}

/***********************************************************************
 *           TASK_KillTask
 */
void TASK_KillTask( HTASK16 hTask )
{
    TDB *pTask; 

    /* Enter the Win16Lock to protect global data structures */
    SYSLEVEL_EnterWin16Lock();

    if ( !hTask ) hTask = GetCurrentTask();
    pTask = (TDB *)GlobalLock16( hTask );
    if ( !pTask ) 
    {
        SYSLEVEL_LeaveWin16Lock();
        return;
    }

    TRACE("Killing task %04x\n", hTask );

#ifdef MZ_SUPPORTED
{
    /* Kill DOS VM task */
    NE_MODULE *pModule = NE_GetPtr( pTask->hModule );
    if ( pModule->lpDosTask )
        MZ_KillModule( pModule->lpDosTask );
}
#endif

    /* Perform USER cleanup */

    TASK_CallTaskSignalProc( USIG16_TERMINATION, hTask );
    PROCESS_CallUserSignalProc( USIG_PROCESS_EXIT, 0 );
    PROCESS_CallUserSignalProc( USIG_THREAD_EXIT, 0 );
    PROCESS_CallUserSignalProc( USIG_PROCESS_DESTROY, 0 );

    if (nTaskCount <= 1)
    {
        TRACE("this is the last task, exiting\n" );
        ExitKernel16();
    }

    /* FIXME: Hack! Send a message to the initial task so that
     * the GetMessage wakes up and the initial task can check whether
     * it is the only remaining one and terminate itself ...
     * The initial task should probably install hooks or something
     * to get informed about task termination :-/
     */
    Callout.PostAppMessage16( initial_task, WM_NULL, 0, 0 );

    /* Remove the task from the list to be sure we never switch back to it */
    TASK_UnlinkTask( hTask );
    if( nTaskCount )
    {
        TDB* p = (TDB *)GlobalLock16( hFirstTask );
        while( p )
        {
            if( p->hYieldTo == hTask ) p->hYieldTo = 0;
            p = (TDB *)GlobalLock16( p->hNext );
        }
    }

    pTask->nEvents = 0;

    if ( hLockedTask == hTask )
        hLockedTask = 0;

    TASK_DeleteTask( hTask );

    /* When deleting the current task ... */
    if ( hTask == hCurrentTask )
    {
        DWORD lockCount;

        /* ... schedule another one ... */
        TASK_Reschedule();

        /* ... and completely release the Win16Lock, just in case. */
        ReleaseThunkLock( &lockCount );

        return;
    }

    SYSLEVEL_LeaveWin16Lock();
}

/***********************************************************************
 *           TASK_Reschedule
 *
 * This is where all the magic of task-switching happens!
 *
 * 16-bit Windows performs non-preemptive (cooperative) multitasking.
 * This means that each 16-bit task runs until it voluntarily yields 
 * control, at which point the scheduler gets active and selects the
 * next task to run.
 *
 * In Wine, all processes, even 16-bit ones, are scheduled preemptively
 * by the standard scheduler of the underlying OS.  As many 16-bit apps
 * *rely* on the behaviour of the Windows scheduler, however, we have
 * to simulate that behaviour.
 *
 * This is achieved as follows: every 16-bit task is at time (except
 * during task creation and deletion) in one of two states: either it
 * is the one currently running, then the global variable hCurrentTask
 * contains its task handle, or it is not currently running, then it
 * is blocked on a special scheduler event, a global handle to which
 * is stored in the task struct.
 *
 * When the current task yields control, this routine gets called. Its
 * purpose is to determine the next task to be active, signal the 
 * scheduler event of that task, and then put the current task to sleep
 * waiting for *its* scheduler event to get signalled again.
 *
 * This routine can get called in a few other special situations as well:
 *
 * - On creation of a 16-bit task, the Unix process executing the task
 *   calls TASK_Reschedule once it has completed its initialization.
 *   At this point, the task needs to be blocked until its scheduler
 *   event is signalled the first time (this will be done by the parent
 *   process to get the task up and running).
 *
 * - When the task currently running terminates itself, this routine gets
 *   called and has to schedule another task, *without* blocking the 
 *   terminating task.
 *
 * - When a 32-bit thread posts an event for a 16-bit task, it might be
 *   the case that *no* 16-bit task is currently running.  In this case
 *   the task that has now an event pending is to be scheduled.
 *
 */
void TASK_Reschedule(void)
{
    TDB *pOldTask = NULL, *pNewTask = NULL;
    HTASK16 hOldTask = 0, hNewTask = 0;
    enum { MODE_YIELD, MODE_SLEEP, MODE_WAKEUP } mode;
    DWORD lockCount;

    SYSLEVEL_EnterWin16Lock();

    /* Check what we need to do */
    hOldTask = GetCurrentTask();
    pOldTask = (TDB *)GlobalLock16( hOldTask );
    TRACE( "entered with hCurrentTask %04x by hTask %04x (pid %ld)\n", 
           hCurrentTask, hOldTask, (long) getpid() );

    if ( pOldTask && THREAD_IsWin16( NtCurrentTeb() ) )
    {
        /* We are called by an active (non-deleted) 16-bit task */

        /* If we don't even have a current task, or else the current
           task has yielded, we'll need to schedule a new task and
           (possibly) put the calling task to sleep.  Otherwise, we
           only block the caller. */

        if ( !hCurrentTask || hCurrentTask == hOldTask )
            mode = MODE_YIELD;
        else
            mode = MODE_SLEEP;
    }
    else
    {
        /* We are called by a deleted 16-bit task or a 32-bit thread */

        /* The only situation where we need to do something is if we
           now do not have a current task.  Then, we'll need to wake up
           some task that has events pending. */

        if ( !hCurrentTask || hCurrentTask == hOldTask )
            mode = MODE_WAKEUP;
        else
        {
            /* nothing to do */
            SYSLEVEL_LeaveWin16Lock();
            return;
        }
    }

    /* Find a task to yield to: check for DirectedYield() */
    if ( mode == MODE_YIELD && pOldTask && pOldTask->hYieldTo )
    {
        hNewTask = pOldTask->hYieldTo;
        pNewTask = (TDB *)GlobalLock16( hNewTask );
        if( !pNewTask || !pNewTask->nEvents) hNewTask = 0;
        pOldTask->hYieldTo = 0;
    }

    /* Find a task to yield to: check for pending events */
    if ( (mode == MODE_YIELD || mode == MODE_WAKEUP) && !hNewTask )
    {
        hNewTask = hFirstTask;
        while (hNewTask)
        {
            pNewTask = (TDB *)GlobalLock16( hNewTask );

            TRACE( "\ttask = %04x, events = %i\n", hNewTask, pNewTask->nEvents );

            if (pNewTask->nEvents) break;
            hNewTask = pNewTask->hNext;
        }
        if (hLockedTask && (hNewTask != hLockedTask)) hNewTask = 0;
    }

    /* If we are still the task with highest priority, just return ... */
    if ( mode == MODE_YIELD && hNewTask == hCurrentTask )
    {
        TRACE("returning to the current task (%04x)\n", hCurrentTask );
        SYSLEVEL_LeaveWin16Lock();

        /* Allow Win32 threads to thunk down even while a Win16 task is
           in a tight PeekMessage() or Yield() loop ... */
        ReleaseThunkLock( &lockCount );
        RestoreThunkLock( lockCount );
        return;
    }

    /* If no task to yield to found, suspend 16-bit scheduler ... */
    if ( mode == MODE_YIELD && !hNewTask )
    {
        TRACE("No currently active task\n");
        hCurrentTask = 0;
    }

    /* If we found a task to wake up, do it ... */
    if ( (mode == MODE_YIELD || mode == MODE_WAKEUP) && hNewTask )
    {
        TRACE("Switching to task %04x (%.8s)\n",
                      hNewTask, pNewTask->module_name );

        pNewTask->priority++;
        TASK_UnlinkTask( hNewTask );
        TASK_LinkTask( hNewTask );
        pNewTask->priority--;

        hCurrentTask = hNewTask;
        SetEvent( pNewTask->hEvent );

        /* This is set just in case some app reads it ... */
        pNewTask->ss_sp = pNewTask->teb->cur_stack;
    }

    /* If we need to put the current task to sleep, do it ... */
    if ( (mode == MODE_YIELD || mode == MODE_SLEEP) && hOldTask != hCurrentTask )
    {
        ResetEvent( pOldTask->hEvent );

        ReleaseThunkLock( &lockCount );
        SYSLEVEL_CheckNotLevel( 1 );
        WaitForSingleObject( pOldTask->hEvent, INFINITE );
        RestoreThunkLock( lockCount );
    }

    SYSLEVEL_LeaveWin16Lock();
}

/***********************************************************************
 *           InitTask  (KERNEL.91)
 *
 * Called by the application startup code.
 */
void WINAPI InitTask16( CONTEXT86 *context )
{
    TDB *pTask;
    INSTANCEDATA *pinstance;
    SEGPTR ptr;

    EAX_reg(context) = 0;
    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return;

    /* Note: we need to trust that BX/CX contain the stack/heap sizes, 
       as some apps, notably Visual Basic apps, *modify* the heap/stack
       size of the instance data segment before calling InitTask() */

    /* Initialize the INSTANCEDATA structure */
    pinstance = (INSTANCEDATA *)PTR_SEG_OFF_TO_LIN(CURRENT_DS, 0);
    pinstance->stackmin    = OFFSETOF( pTask->teb->cur_stack ) + sizeof( STACK16FRAME );
    pinstance->stackbottom = pinstance->stackmin; /* yup, that's right. Confused me too. */
    pinstance->stacktop    = ( pinstance->stackmin > BX_reg(context)? 
                               pinstance->stackmin - BX_reg(context) : 0 ) + 150; 

    /* Initialize the local heap */
    if ( CX_reg(context) )
        LocalInit16( GlobalHandleToSel16(pTask->hInstance), 0, CX_reg(context) );

    /* Initialize implicitly loaded DLLs */
    NE_InitializeDLLs( pTask->hModule );
    NE_DllProcessAttach( pTask->hModule );

    /* Registers on return are:
     * ax     1 if OK, 0 on error
     * cx     stack limit in bytes
     * dx     cmdShow parameter
     * si     instance handle of the previous instance
     * di     instance handle of the new task
     * es:bx  pointer to command-line inside PSP
     *
     * 0 (=%bp) is pushed on the stack
     */
    ptr = stack16_push( sizeof(WORD) );
    *(WORD *)PTR_SEG_TO_LIN(ptr) = 0;
    ESP_reg(context) -= 2;

    EAX_reg(context) = 1;
        
    if (!pTask->pdb.cmdLine[0]) EBX_reg(context) = 0x80;
    else
    {
        LPBYTE p = &pTask->pdb.cmdLine[1];
        while ((*p == ' ') || (*p == '\t')) p++;
        EBX_reg(context) = 0x80 + (p - pTask->pdb.cmdLine);
    }
    ECX_reg(context) = pinstance->stacktop;
    EDX_reg(context) = pTask->nCmdShow;
    ESI_reg(context) = (DWORD)pTask->hPrevInstance;
    EDI_reg(context) = (DWORD)pTask->hInstance;
    ES_reg (context) = (WORD)pTask->hPDB;
}


/***********************************************************************
 *           WaitEvent  (KERNEL.30)
 */
BOOL16 WINAPI WaitEvent16( HTASK16 hTask )
{
    TDB *pTask;

    if (!hTask) hTask = GetCurrentTask();
    pTask = (TDB *)GlobalLock16( hTask );

    if ( !THREAD_IsWin16( NtCurrentTeb() ) )
    {
        FIXME("called for Win32 thread (%04x)!\n", NtCurrentTeb()->teb_sel);
        return TRUE;
    }

    if (pTask->nEvents > 0)
    {
        pTask->nEvents--;
        return FALSE;
    }
    TASK_Reschedule();

    /* When we get back here, we have an event */

    if (pTask->nEvents > 0) pTask->nEvents--;
    return TRUE;
}


/***********************************************************************
 *           PostEvent  (KERNEL.31)
 */
void WINAPI PostEvent16( HTASK16 hTask )
{
    TDB *pTask;

    if (!hTask) hTask = GetCurrentTask();
    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return;

    if ( !THREAD_IsWin16( pTask->teb ) )
    {
        FIXME("called for Win32 thread (%04x)!\n", pTask->teb->teb_sel );
        return;
    }

    pTask->nEvents++;

    /* If we are a 32-bit task, we might need to wake up the 16-bit scheduler */
    if ( !THREAD_IsWin16( NtCurrentTeb() ) )
        TASK_Reschedule();
}


/***********************************************************************
 *           SetPriority  (KERNEL.32)
 */
void WINAPI SetPriority16( HTASK16 hTask, INT16 delta )
{
    TDB *pTask;
    INT16 newpriority;

    if (!hTask) hTask = GetCurrentTask();
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
HTASK16 WINAPI LockCurrentTask16( BOOL16 bLock )
{
    if (bLock) hLockedTask = GetCurrentTask();
    else hLockedTask = 0;
    return hLockedTask;
}


/***********************************************************************
 *           IsTaskLocked  (KERNEL.122)
 */
HTASK16 WINAPI IsTaskLocked16(void)
{
    return hLockedTask;
}


/***********************************************************************
 *           OldYield  (KERNEL.117)
 */
void WINAPI OldYield16(void)
{
    TDB *pCurTask = (TDB *)GlobalLock16( GetCurrentTask() );

    if ( !THREAD_IsWin16( NtCurrentTeb() ) )
    {
        FIXME("called for Win32 thread (%04x)!\n", NtCurrentTeb()->teb_sel);
        return;
    }

    if (pCurTask) pCurTask->nEvents++;  /* Make sure we get back here */
    TASK_Reschedule();
    if (pCurTask) pCurTask->nEvents--;
}

/***********************************************************************
 *           WIN32_OldYield16  (KERNEL.447)
 */
void WINAPI WIN32_OldYield16(void)
{
   DWORD count;

   ReleaseThunkLock(&count);
   RestoreThunkLock(count);
}

/***********************************************************************
 *           DirectedYield  (KERNEL.150)
 */
void WINAPI DirectedYield16( HTASK16 hTask )
{
    TDB *pCurTask = (TDB *)GlobalLock16( GetCurrentTask() );

    if ( !THREAD_IsWin16( NtCurrentTeb() ) )
    {
        FIXME("called for Win32 thread (%04x)!\n", NtCurrentTeb()->teb_sel);
        return;
    }

    TRACE("%04x: DirectedYield(%04x)\n", pCurTask->hSelf, hTask );

    pCurTask->hYieldTo = hTask;
    OldYield16();

    TRACE("%04x: back from DirectedYield(%04x)\n", pCurTask->hSelf, hTask );
}

/***********************************************************************
 *           Yield16  (KERNEL.29)
 */
void WINAPI Yield16(void)
{
    TDB *pCurTask = (TDB *)GlobalLock16( GetCurrentTask() );

    if (pCurTask) pCurTask->hYieldTo = 0;
    if (pCurTask && pCurTask->hQueue) Callout.UserYield16();
    else OldYield16();
}

/***********************************************************************
 *           KERNEL_490  (KERNEL.490)
 */
HTASK16 WINAPI KERNEL_490( HTASK16 someTask )
{
    if ( !someTask ) return 0;

    FIXME("(%04x): stub\n", someTask );
    return 0;
}

/***********************************************************************
 *           MakeProcInstance16  (KERNEL.51)
 */
FARPROC16 WINAPI MakeProcInstance16( FARPROC16 func, HANDLE16 hInstance )
{
    BYTE *thunk,*lfunc;
    SEGPTR thunkaddr;
    WORD hInstanceSelector;

    hInstanceSelector = GlobalHandleToSel16(hInstance);

    TRACE("(%08lx, %04x);", (DWORD)func, hInstance);

    if (!HIWORD(func)) {
      /* Win95 actually protects via SEH, but this is better for debugging */
      ERR("Ouch ! Called with invalid func 0x%08lx !\n", (DWORD)func);
      return (FARPROC16)0;
    }

    if (hInstance)
    {
	if ( (!(hInstance & 4)) ||
	     ((hInstance != 0xffff) && IS_SELECTOR_FREE(hInstance|7)) )
 	{
	    ERR("Invalid hInstance (%04x) passed to MakeProcInstance !\n",
		hInstance);
	    return 0;
	}
    }

    if ( (CURRENT_DS != hInstanceSelector)
      && (hInstance != 0)
      && (hInstance != 0xffff) )
    {
	/* calling MPI with a foreign DSEG is invalid ! */
        ERR("Problem with hInstance? Got %04x, using %04x instead\n",
                   hInstance,CURRENT_DS);
    }

    /* Always use the DSEG that MPI was entered with.
     * We used to set hInstance to GetTaskDS16(), but this should be wrong
     * as CURRENT_DS provides the DSEG value we need.
     * ("calling" DS, *not* "task" DS !) */
    hInstanceSelector = CURRENT_DS;
    hInstance = GlobalHandle16(hInstanceSelector);

    /* no thunking for DLLs */
    if (NE_GetPtr(FarGetOwner16(hInstance))->flags & NE_FFLAGS_LIBMODULE)
	return func;

    thunkaddr = TASK_AllocThunk( GetCurrentTask() );
    if (!thunkaddr) return (FARPROC16)0;
    thunk = PTR_SEG_TO_LIN( thunkaddr );
    lfunc = PTR_SEG_TO_LIN( func );

    TRACE("(%08lx,%04x): got thunk %08lx\n",
          (DWORD)func, hInstance, (DWORD)thunkaddr );
    if (((lfunc[0]==0x8c) && (lfunc[1]==0xd8)) || /* movw %ds, %ax */
    	((lfunc[0]==0x1e) && (lfunc[1]==0x58))    /* pushw %ds, popw %ax */
    ) {
    	FIXME("This was the (in)famous \"thunk useless\" warning. We thought we have to overwrite with nop;nop;, but this isn't true.\n");
    }

    *thunk++ = 0xb8;    /* movw instance, %ax */
    *thunk++ = (BYTE)(hInstanceSelector & 0xff);
    *thunk++ = (BYTE)(hInstanceSelector >> 8);
    *thunk++ = 0xea;    /* ljmp func */
    *(DWORD *)thunk = (DWORD)func;
    return (FARPROC16)thunkaddr;
    /* CX reg indicates if thunkaddr != NULL, implement if needed */
}


/***********************************************************************
 *           FreeProcInstance16  (KERNEL.52)
 */
void WINAPI FreeProcInstance16( FARPROC16 func )
{
    TRACE("(%08lx)\n", (DWORD)func );
    TASK_FreeThunk( GetCurrentTask(), (SEGPTR)func );
}

/**********************************************************************
 *	    TASK_GetCodeSegment
 * 
 * Helper function for GetCodeHandle/GetCodeInfo: Retrieve the module 
 * and logical segment number of a given code segment.
 *
 * 'proc' either *is* already a pair of module handle and segment number,
 * in which case there's nothing to do.  Otherwise, it is a pointer to
 * a function, and we need to retrieve the code segment.  If the pointer
 * happens to point to a thunk, we'll retrieve info about the code segment
 * where the function pointed to by the thunk resides, not the thunk itself.
 *
 * FIXME: if 'proc' is a SNOOP16 return stub, we should retrieve info about
 *        the function the snoop code will return to ...
 *
 */
static BOOL TASK_GetCodeSegment( FARPROC16 proc, NE_MODULE **ppModule, 
                                 SEGTABLEENTRY **ppSeg, int *pSegNr )
{
    NE_MODULE *pModule = NULL;
    SEGTABLEENTRY *pSeg = NULL;
    int segNr;

    /* Try pair of module handle / segment number */
    pModule = (NE_MODULE *) GlobalLock16( HIWORD( proc ) );
    if ( pModule && pModule->magic == IMAGE_OS2_SIGNATURE )
    {
        segNr = LOWORD( proc );
        if ( segNr && segNr <= pModule->seg_count )
            pSeg = NE_SEG_TABLE( pModule ) + segNr-1;
    }

    /* Try thunk or function */
    else 
    {
        BYTE *thunk = (BYTE *)PTR_SEG_TO_LIN( proc );
        WORD selector;

        if ((thunk[0] == 0xb8) && (thunk[3] == 0xea))
            selector = thunk[6] + (thunk[7] << 8);
        else
            selector = HIWORD( proc );

        pModule = NE_GetPtr( GlobalHandle16( selector ) );
        pSeg = pModule? NE_SEG_TABLE( pModule ) : NULL;

        if ( pModule )
            for ( segNr = 1; segNr <= pModule->seg_count; segNr++, pSeg++ )
                if ( GlobalHandleToSel16(pSeg->hSeg) == selector )
                    break;

        if ( pModule && segNr > pModule->seg_count )
            pSeg = NULL;
    }

    /* Abort if segment not found */

    if ( !pModule || !pSeg )
        return FALSE;

    /* Return segment data */

    if ( ppModule ) *ppModule = pModule;
    if ( ppSeg    ) *ppSeg    = pSeg;
    if ( pSegNr   ) *pSegNr   = segNr;

    return TRUE;
}

/**********************************************************************
 *	    GetCodeHandle    (KERNEL.93)
 */
HANDLE16 WINAPI GetCodeHandle16( FARPROC16 proc )
{
    SEGTABLEENTRY *pSeg;

    if ( !TASK_GetCodeSegment( proc, NULL, &pSeg, NULL ) )
        return (HANDLE16)0;

    return pSeg->hSeg;
}

/**********************************************************************
 *	    GetCodeInfo    (KERNEL.104)
 */
BOOL16 WINAPI GetCodeInfo16( FARPROC16 proc, SEGINFO *segInfo )
{
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSeg;
    int segNr;

    if ( !TASK_GetCodeSegment( proc, &pModule, &pSeg, &segNr ) )
        return FALSE;

    /* Fill in segment information */

    segInfo->offSegment = pSeg->filepos;
    segInfo->cbSegment  = pSeg->size;
    segInfo->flags      = pSeg->flags;
    segInfo->cbAlloc    = pSeg->minsize;
    segInfo->h          = pSeg->hSeg;
    segInfo->alignShift = pModule->alignment;

    if ( segNr == pModule->dgroup )
        segInfo->cbAlloc += pModule->heap_size + pModule->stack_size;

    /* Return module handle in %es */

    CURRENT_STACK16->es = GlobalHandleToSel16( pModule->self );

    return TRUE;
}


/**********************************************************************
 *          DefineHandleTable16    (KERNEL.94)
 */
BOOL16 WINAPI DefineHandleTable16( WORD wOffset )
{
    return TRUE;  /* FIXME */
}


/***********************************************************************
 *           SetTaskQueue  (KERNEL.34)
 */
HQUEUE16 WINAPI SetTaskQueue16( HTASK16 hTask, HQUEUE16 hQueue )
{
    HQUEUE16 hPrev;
    TDB *pTask;

    if (!hTask) hTask = GetCurrentTask();
    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return 0;

    hPrev = pTask->hQueue;
    pTask->hQueue = hQueue;

    return hPrev;
}


/***********************************************************************
 *           GetTaskQueue  (KERNEL.35)
 */
HQUEUE16 WINAPI GetTaskQueue16( HTASK16 hTask )
{
    TDB *pTask;

    if (!hTask) hTask = GetCurrentTask();
    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return 0;
    return pTask->hQueue;
}

/***********************************************************************
 *           SetThreadQueue  (KERNEL.463)
 */
HQUEUE16 WINAPI SetThreadQueue16( DWORD thread, HQUEUE16 hQueue )
{
    TEB *teb = thread? THREAD_IdToTEB( thread ) : NtCurrentTeb();
    HQUEUE16 oldQueue = teb? teb->queue : 0;

    if ( teb )
    {
        teb->queue = hQueue;

        if ( GetTaskQueue16( teb->process->task ) == oldQueue )
            SetTaskQueue16( teb->process->task, hQueue );
    }

    return oldQueue;
}

/***********************************************************************
 *           GetThreadQueue  (KERNEL.464)
 */
HQUEUE16 WINAPI GetThreadQueue16( DWORD thread )
{
    TEB *teb = NULL;
    if ( !thread )
        teb = NtCurrentTeb();
    else if ( HIWORD(thread) )
        teb = THREAD_IdToTEB( thread );
    else if ( IsTask16( (HTASK16)thread ) )
        teb = ((TDB *)GlobalLock16( (HANDLE16)thread ))->teb;

    return (HQUEUE16)(teb? teb->queue : 0);
}

/***********************************************************************
 *           SetFastQueue  (KERNEL.624)
 */
VOID WINAPI SetFastQueue16( DWORD thread, HANDLE hQueue )
{
    TEB *teb = NULL;
    if ( !thread )
        teb = NtCurrentTeb();
    else if ( HIWORD(thread) )
        teb = THREAD_IdToTEB( thread );
    else if ( IsTask16( (HTASK16)thread ) )
        teb = ((TDB *)GlobalLock16( (HANDLE16)thread ))->teb;

    if ( teb ) teb->queue = (HQUEUE16) hQueue;
}

/***********************************************************************
 *           GetFastQueue  (KERNEL.625)
 */
HANDLE WINAPI GetFastQueue16( void )
{
    TEB *teb = NtCurrentTeb();
    if (!teb) return 0;

    if (!teb->queue)
        Callout.InitThreadInput16( 0, THREAD_IsWin16(teb)? 4 : 5 );

    if (!teb->queue)
        FIXME("(): should initialize thread-local queue, expect failure!\n" );

    return (HANDLE)teb->queue;
}

/***********************************************************************
 *           SwitchStackTo   (KERNEL.108)
 */
void WINAPI SwitchStackTo16( WORD seg, WORD ptr, WORD top )
{
    TDB *pTask;
    STACK16FRAME *oldFrame, *newFrame;
    INSTANCEDATA *pData;
    UINT16 copySize;

    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return;
    if (!(pData = (INSTANCEDATA *)GlobalLock16( seg ))) return;
    TRACE("old=%04x:%04x new=%04x:%04x\n",
          SELECTOROF( pTask->teb->cur_stack ),
          OFFSETOF( pTask->teb->cur_stack ), seg, ptr );

    /* Save the old stack */

    oldFrame = THREAD_STACK16( pTask->teb );
    /* pop frame + args and push bp */
    pData->old_ss_sp   = pTask->teb->cur_stack + sizeof(STACK16FRAME)
                           + 2 * sizeof(WORD);
    *(WORD *)PTR_SEG_TO_LIN(pData->old_ss_sp) = oldFrame->bp;
    pData->stacktop    = top;
    pData->stackmin    = ptr;
    pData->stackbottom = ptr;

    /* Switch to the new stack */

    /* Note: we need to take the 3 arguments into account; otherwise,
     * the stack will underflow upon return from this function.
     */
    copySize = oldFrame->bp - OFFSETOF(pData->old_ss_sp);
    copySize += 3 * sizeof(WORD) + sizeof(STACK16FRAME);
    pTask->teb->cur_stack = PTR_SEG_OFF_TO_SEGPTR( seg, ptr - copySize );
    newFrame = THREAD_STACK16( pTask->teb );

    /* Copy the stack frame and the local variables to the new stack */

    memmove( newFrame, oldFrame, copySize );
    newFrame->bp = ptr;
    *(WORD *)PTR_SEG_OFF_TO_LIN( seg, ptr ) = 0;  /* clear previous bp */
}


/***********************************************************************
 *           SwitchStackBack   (KERNEL.109)
 */
void WINAPI SwitchStackBack16( CONTEXT86 *context )
{
    STACK16FRAME *oldFrame, *newFrame;
    INSTANCEDATA *pData;

    if (!(pData = (INSTANCEDATA *)GlobalLock16(SELECTOROF(NtCurrentTeb()->cur_stack))))
        return;
    if (!pData->old_ss_sp)
    {
        WARN("No previous SwitchStackTo\n" );
        return;
    }
    TRACE("restoring stack %04x:%04x\n",
          SELECTOROF(pData->old_ss_sp), OFFSETOF(pData->old_ss_sp) );

    oldFrame = CURRENT_STACK16;

    /* Pop bp from the previous stack */

    BP_reg(context) = *(WORD *)PTR_SEG_TO_LIN(pData->old_ss_sp);
    pData->old_ss_sp += sizeof(WORD);

    /* Switch back to the old stack */

    NtCurrentTeb()->cur_stack = pData->old_ss_sp - sizeof(STACK16FRAME);
    SS_reg(context)  = SELECTOROF(pData->old_ss_sp);
    ESP_reg(context) = OFFSETOF(pData->old_ss_sp) - sizeof(DWORD); /*ret addr*/
    pData->old_ss_sp = 0;

    /* Build a stack frame for the return */

    newFrame = CURRENT_STACK16;
    newFrame->frame32     = oldFrame->frame32;
    newFrame->module_cs   = oldFrame->module_cs;
    newFrame->callfrom_ip = oldFrame->callfrom_ip;
    newFrame->entry_ip    = oldFrame->entry_ip;
}


/***********************************************************************
 *           GetTaskQueueDS16  (KERNEL.118)
 */
void WINAPI GetTaskQueueDS16(void)
{
    CURRENT_STACK16->ds = GlobalHandleToSel16( GetTaskQueue16(0) );
}


/***********************************************************************
 *           GetTaskQueueES16  (KERNEL.119)
 */
void WINAPI GetTaskQueueES16(void)
{
    CURRENT_STACK16->es = GlobalHandleToSel16( GetTaskQueue16(0) );
}


/***********************************************************************
 *           GetCurrentTask   (KERNEL.36)
 */
HTASK16 WINAPI GetCurrentTask(void)
{
    return PROCESS_Current()->task;
}

DWORD WINAPI WIN16_GetCurrentTask(void)
{
    /* This is the version used by relay code; the first task is */
    /* returned in the high word of the result */
    return MAKELONG( GetCurrentTask(), hFirstTask );
}


/***********************************************************************
 *           GetCurrentPDB   (KERNEL.37)
 *
 * UNDOC: returns PSP of KERNEL in high word
 */
DWORD WINAPI GetCurrentPDB16(void)
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return 0;
    return MAKELONG(pTask->hPDB, 0); /* FIXME */
}


/***********************************************************************
 *           GetCurPID16   (KERNEL.157)
 */
DWORD WINAPI GetCurPID16( DWORD unused )
{
    return 0;
}


/***********************************************************************
 *           GetInstanceData   (KERNEL.54)
 */
INT16 WINAPI GetInstanceData16( HINSTANCE16 instance, WORD buffer, INT16 len )
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
WORD WINAPI GetExeVersion16(void)
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return 0;
    return pTask->version;
}


/***********************************************************************
 *           SetErrorMode16   (KERNEL.107)
 */
UINT16 WINAPI SetErrorMode16( UINT16 mode )
{
    TDB *pTask;
    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return 0;
    pTask->error_mode = mode;
    return SetErrorMode( mode );
}


/***********************************************************************
 *           GetDOSEnvironment   (KERNEL.131)
 */
SEGPTR WINAPI GetDOSEnvironment16(void)
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return 0;
    return PTR_SEG_OFF_TO_SEGPTR( pTask->pdb.environment, 0 );
}


/***********************************************************************
 *           GetNumTasks   (KERNEL.152)
 */
UINT16 WINAPI GetNumTasks16(void)
{
    return nTaskCount;
}


/***********************************************************************
 *           GetTaskDS   (KERNEL.155)
 *
 * Note: this function apparently returns a DWORD with LOWORD == HIWORD.
 * I don't think we need to bother with this.
 */
HINSTANCE16 WINAPI GetTaskDS16(void)
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return 0;
    return GlobalHandleToSel16(pTask->hInstance);
}

/***********************************************************************
 *           GetDummyModuleHandleDS   (KERNEL.602)
 */
WORD WINAPI GetDummyModuleHandleDS16(void)
{
    TDB *pTask;
    WORD selector;

    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return 0;
    if (!(pTask->flags & TDBF_WIN32)) return 0;
    selector = GlobalHandleToSel16( pTask->hModule );
    CURRENT_DS = selector;
    return selector;
}

/***********************************************************************
 *           IsTask   (KERNEL.320)
 */
BOOL16 WINAPI IsTask16( HTASK16 hTask )
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return FALSE;
    if (GlobalSize16( hTask ) < sizeof(TDB)) return FALSE;
    return (pTask->magic == TDB_MAGIC);
}


/***********************************************************************
 *           IsWinOldApTask16   (KERNEL.158)
 */
BOOL16 WINAPI IsWinOldApTask16( HTASK16 hTask )
{
    /* should return bit 0 of byte 0x48 in PSP */
    return FALSE;
}

/***********************************************************************
 *           SetTaskSignalProc   (KERNEL.38)
 */
FARPROC16 WINAPI SetTaskSignalProc( HTASK16 hTask, FARPROC16 proc )
{
    TDB *pTask;
    FARPROC16 oldProc;

    if (!hTask) hTask = GetCurrentTask();
    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return NULL;
    oldProc = pTask->userhandler;
    pTask->userhandler = proc;
    return oldProc;
}

/***********************************************************************
 *           TASK_CallTaskSignalProc
 */
/* ### start build ### */
extern WORD CALLBACK TASK_CallTo16_word_wwwww(FARPROC16,WORD,WORD,WORD,WORD,WORD);
/* ### stop build ### */
void TASK_CallTaskSignalProc( UINT16 uCode, HANDLE16 hTaskOrModule )
{
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
    if ( !pTask || !pTask->userhandler ) return;

    TASK_CallTo16_word_wwwww( pTask->userhandler, 
                              hTaskOrModule, uCode, 0, 
                              pTask->hInstance, pTask->hQueue );
}

/***********************************************************************
 *           SetSigHandler   (KERNEL.140)
 */
WORD WINAPI SetSigHandler16( FARPROC16 newhandler, FARPROC16* oldhandler,
                           UINT16 *oldmode, UINT16 newmode, UINT16 flag )
{
    FIXME("(%p,%p,%p,%d,%d), unimplemented.\n",
	  newhandler,oldhandler,oldmode,newmode,flag );

    if (flag != 1) return 0;
    if (!newmode) newhandler = NULL;  /* Default handler */
    if (newmode != 4)
    {
        TDB *pTask;

        if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return 0;
        if (oldmode) *oldmode = pTask->signal_flags;
        pTask->signal_flags = newmode;
        if (oldhandler) *oldhandler = pTask->sighandler;
        pTask->sighandler = newhandler;
    }
    return 0;
}


/***********************************************************************
 *           GlobalNotify   (KERNEL.154)
 *
 * Note that GlobalNotify does _not_ return the old NotifyProc
 * -- contrary to LocalNotify !!
 */
VOID WINAPI GlobalNotify16( FARPROC16 proc )
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return;
    pTask->discardhandler = proc;
}


/***********************************************************************
 *           GetExePtr   (KERNEL.133)
 */
static inline HMODULE16 GetExePtrHelper( HANDLE16 handle, HTASK16 *hTask )
{
    char *ptr;
    HANDLE16 owner;

      /* Check for module handle */

    if (!(ptr = GlobalLock16( handle ))) return 0;
    if (((NE_MODULE *)ptr)->magic == IMAGE_OS2_SIGNATURE) return handle;

      /* Search for this handle inside all tasks */

    *hTask = hFirstTask;
    while (*hTask)
    {
        TDB *pTask = (TDB *)GlobalLock16( *hTask );
        if ((*hTask == handle) ||
            (pTask->hInstance == handle) ||
            (pTask->hQueue == handle) ||
            (pTask->hPDB == handle)) return pTask->hModule;
        *hTask = pTask->hNext;
    }

      /* Check the owner for module handle */

    owner = FarGetOwner16( handle );
    if (!(ptr = GlobalLock16( owner ))) return 0;
    if (((NE_MODULE *)ptr)->magic == IMAGE_OS2_SIGNATURE) return owner;

      /* Search for the owner inside all tasks */

    *hTask = hFirstTask;
    while (*hTask)
    {
        TDB *pTask = (TDB *)GlobalLock16( *hTask );
        if ((*hTask == owner) ||
            (pTask->hInstance == owner) ||
            (pTask->hQueue == owner) ||
            (pTask->hPDB == owner)) return pTask->hModule;
        *hTask = pTask->hNext;
    }

    return 0;
}

HMODULE16 WINAPI WIN16_GetExePtr( HANDLE16 handle )
{
    HTASK16 hTask = 0;
    HMODULE16 hModule = GetExePtrHelper( handle, &hTask );
    STACK16FRAME *frame = CURRENT_STACK16;
    frame->ecx = hModule;
    if (hTask) frame->es = hTask;
    return hModule;
}

HMODULE16 WINAPI GetExePtr( HANDLE16 handle )
{
    HTASK16 hTask = 0;
    return GetExePtrHelper( handle, &hTask );
}


/***********************************************************************
 *           TaskFirst   (TOOLHELP.63)
 */
BOOL16 WINAPI TaskFirst16( TASKENTRY *lpte )
{
    lpte->hNext = hFirstTask;
    return TaskNext16( lpte );
}


/***********************************************************************
 *           TaskNext   (TOOLHELP.64)
 */
BOOL16 WINAPI TaskNext16( TASKENTRY *lpte )
{
    TDB *pTask;
    INSTANCEDATA *pInstData;

    TRACE_(toolhelp)("(%p): task=%04x\n", lpte, lpte->hNext );
    if (!lpte->hNext) return FALSE;
    pTask = (TDB *)GlobalLock16( lpte->hNext );
    if (!pTask || pTask->magic != TDB_MAGIC) return FALSE;
    pInstData = (INSTANCEDATA *)PTR_SEG_OFF_TO_LIN( GlobalHandleToSel16(pTask->hInstance), 0 );
    lpte->hTask         = lpte->hNext;
    lpte->hTaskParent   = pTask->hParent;
    lpte->hInst         = pTask->hInstance;
    lpte->hModule       = pTask->hModule;
    lpte->wSS           = SELECTOROF( pTask->teb->cur_stack );
    lpte->wSP           = OFFSETOF( pTask->teb->cur_stack );
    lpte->wStackTop     = pInstData->stacktop;
    lpte->wStackMinimum = pInstData->stackmin;
    lpte->wStackBottom  = pInstData->stackbottom;
    lpte->wcEvents      = pTask->nEvents;
    lpte->hQueue        = pTask->hQueue;
    lstrcpynA( lpte->szModule, pTask->module_name, sizeof(lpte->szModule) );
    lpte->wPSPOffset    = 0x100;  /*??*/
    lpte->hNext         = pTask->hNext;
    return TRUE;
}


/***********************************************************************
 *           TaskFindHandle   (TOOLHELP.65)
 */
BOOL16 WINAPI TaskFindHandle16( TASKENTRY *lpte, HTASK16 hTask )
{
    lpte->hNext = hTask;
    return TaskNext16( lpte );
}


/***********************************************************************
 *           GetAppCompatFlags16   (KERNEL.354)
 */
DWORD WINAPI GetAppCompatFlags16( HTASK16 hTask )
{
    return GetAppCompatFlags( hTask );
}


/***********************************************************************
 *           GetAppCompatFlags   (USER32.206)
 */
DWORD WINAPI GetAppCompatFlags( HTASK hTask )
{
    TDB *pTask;

    if (!hTask) hTask = GetCurrentTask();
    if (!(pTask=(TDB *)GlobalLock16( (HTASK16)hTask ))) return 0;
    if (GlobalSize16(hTask) < sizeof(TDB)) return 0;
    return pTask->compat_flags;
}
