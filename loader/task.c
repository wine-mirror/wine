/*
 * Task functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "windows.h"
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
#include "peexe.h"
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
#include "thread.h"
#include "syslevel.h"
#include "debug.h"
#include "dosexe.h"
#include "dde_proc.h"
#include "server.h"

  /* Min. number of thunks allocated when creating a new segment */
#define MIN_THUNKS  32

  /* Pointer to function to switch to a larger stack */
int (*IF1632_CallLargeStack)( int (*func)(), void *arg ) = NULL;


static THHOOK DefaultThhook = { 0 };
THHOOK *pThhook = &DefaultThhook;

#define hCurrentTask (pThhook->CurTDB)
#define hFirstTask   (pThhook->HeadTDB)
#define hLockedTask  (pThhook->LockTDB)

static HTASK16 hTaskToKill = 0;
static UINT16 nTaskCount = 0;

static void TASK_YieldToSystem(TDB*);

extern BOOL32 THREAD_InitDone;


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
static BOOL32 TASK_FreeThunk( HTASK16 hTask, SEGPTR thunk )
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
static void TASK_CallToStart(void)
{
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
    NE_MODULE *pModule = NE_GetPtr( pTask->hModule );
    SEGTABLEENTRY *pSegTable = NE_SEG_TABLE( pModule );

    SET_CUR_THREAD( pTask->thdb );
    CLIENT_InitThread();

    /* Terminate the stack frame chain */
    memset(THREAD_STACK16( pTask->thdb ), '\0', sizeof(STACK16FRAME));

    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        /* FIXME: all this is an ugly hack */

        LPTHREAD_START_ROUTINE entry = (LPTHREAD_START_ROUTINE)
                RVA_PTR(pModule->module32, OptionalHeader.AddressOfEntryPoint);
        DWORD size = PE_HEADER(pModule->module32)->OptionalHeader.SizeOfStackReserve;
        DWORD id;
        THDB *thdb;

        pTask->userhandler = (USERSIGNALPROC)&USER_SignalProc;
        if (pModule->heap_size)
            LocalInit( pTask->hInstance, 0, pModule->heap_size );

        InitApp( pTask->hModule );
        PE_InitializeDLLs( PROCESS_Current(), DLL_PROCESS_ATTACH, (LPVOID)-1 );
        TRACE(relay, "(entryproc=%p)\n", entry );

#if 1
        ExitProcess( entry(NULL) );
#else
        CreateThread( NULL, size, entry, NULL, 0, &id );
        thdb = THREAD_ID_TO_THDB( id );

        while ( thdb->exit_code == 0x103 )
        {
            WaitEvent( 0 );
            QUEUE_Signal( pTask->hSelf );
        }

        ExitProcess( thdb->exit_code );
#endif
    }
    else if (pModule->dos_image)
    {
        DOSVM_Enter( NULL );
        ExitProcess( 0 );
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
        CONTEXT context;

        memset( &context, 0, sizeof(context) );
        CS_reg(&context)  = GlobalHandleToSel(pSegTable[pModule->cs - 1].hSeg);
        DS_reg(&context)  = GlobalHandleToSel(pSegTable[pModule->dgroup - 1].hSeg);
        ES_reg(&context)  = pTask->hPDB;
        EIP_reg(&context) = pModule->ip;
        EBX_reg(&context) = pModule->stack_size;
        ECX_reg(&context) = pModule->heap_size;
        EDI_reg(&context) = context.SegDs;

        TRACE(task, "Starting main program: cs:ip=%04lx:%04x ds=%04lx ss:sp=%04x:%04x\n",
                      CS_reg(&context), IP_reg(&context), DS_reg(&context),
                      SELECTOROF(pTask->thdb->cur_stack),
                      OFFSETOF(pTask->thdb->cur_stack) );

        Callbacks->CallRegisterShortProc( &context, 0 );
        /* This should never return */
        ERR( task, "Main program returned! (should never happen)\n" );
        ExitProcess( 1 );
    }
}


/***********************************************************************
 *           TASK_Create
 *
 * NOTE: This routine might be called by a Win32 thread. We don't have
 *       any real problems with that, since we operated merely on a private
 *       TDB structure that is not yet linked into the task list.
 */
HTASK16 TASK_Create( THDB *thdb, NE_MODULE *pModule, HINSTANCE16 hInstance,
                     HINSTANCE16 hPrevInstance, UINT16 cmdShow)
{
    HTASK16 hTask;
    TDB *pTask;
    LPSTR cmd_line;
    WORD sp;
    char *stack32Top;
    char name[10];
    STACK16FRAME *frame16;
    STACK32FRAME *frame32;
    PDB32 *pdb32 = thdb->process;
    SEGTABLEENTRY *pSegTable = NE_SEG_TABLE( pModule );

      /* Allocate the task structure */

    hTask = GLOBAL_Alloc( GMEM_FIXED | GMEM_ZEROINIT, sizeof(TDB),
                          pModule->self, FALSE, FALSE, FALSE );
    if (!hTask) return 0;
    pTask = (TDB *)GlobalLock16( hTask );

    /* Fill the task structure */

    pTask->nEvents       = 1;  /* So the task can be started */
    pTask->hSelf         = hTask;
    pTask->flags         = 0;

    if (pModule->flags & NE_FFLAGS_WIN32)
    	pTask->flags 	|= TDBF_WIN32;
    if (pModule->lpDosTask)
    	pTask->flags 	|= TDBF_WINOLDAP;

    pTask->version       = pModule->expected_version;
    pTask->hInstance     = hInstance;
    pTask->hPrevInstance = hPrevInstance;
    pTask->hModule       = pModule->self;
    pTask->hParent       = GetCurrentTask();
    pTask->magic         = TDB_MAGIC;
    pTask->nCmdShow      = cmdShow;
    pTask->thdb          = thdb;
    pTask->curdrive      = DRIVE_GetCurrentDrive() | 0x80;
    strcpy( pTask->curdir, "\\" );
    lstrcpyn32A( pTask->curdir + 1, DRIVE_GetDosCwd( DRIVE_GetCurrentDrive() ),
                 sizeof(pTask->curdir) - 1 );

      /* Create the thunks block */

    TASK_CreateThunks( hTask, (int)&pTask->thunks - (int)pTask, 7 );

      /* Copy the module name */

    GetModuleName( pModule->self, name, sizeof(name) );
    strncpy( pTask->module_name, name, sizeof(pTask->module_name) );

      /* Allocate a selector for the PDB */

    pTask->hPDB = GLOBAL_CreateBlock( GMEM_FIXED, &pTask->pdb, sizeof(PDB),
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
        PTR_SEG_OFF_TO_SEGPTR( GlobalHandleToSel(pTask->hPDB),
                               (int)&((PDB *)0)->fileHandles );
    pTask->pdb.hFileHandles = 0;
    memset( pTask->pdb.fileHandles, 0xff, sizeof(pTask->pdb.fileHandles) );
    pTask->pdb.environment    = pdb32->env_db->env_sel;
    pTask->pdb.nbFiles        = 20;

    /* Fill the command line */

    cmd_line = pdb32->env_db->cmd_line;
    while (*cmd_line && (*cmd_line != ' ') && (*cmd_line != '\t')) cmd_line++;
    while ((*cmd_line == ' ') || (*cmd_line == '\t')) cmd_line++;
    lstrcpyn32A( pTask->pdb.cmdLine+1, cmd_line, sizeof(pTask->pdb.cmdLine)-1);
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

    /* Create the 16-bit stack frame */

    if (!(sp = pModule->sp))
        sp = pSegTable[pModule->ss-1].minsize + pModule->stack_size;
    sp &= ~1;
    pTask->thdb->cur_stack = PTR_SEG_OFF_TO_SEGPTR( pTask->hInstance, sp );
    pTask->thdb->cur_stack -= 2*sizeof(STACK16FRAME);
    frame16 = (STACK16FRAME *)PTR_SEG_TO_LIN( pTask->thdb->cur_stack );
    frame16->ebp = sp + (int)&((STACK16FRAME *)0)->bp;
    frame16->bp = LOWORD(frame16->ebp);
    frame16->ds = frame16->es = pTask->hInstance;
    frame16->fs = 0;
    frame16->entry_point = 0;
    frame16->entry_cs = 0;
    frame16->mutex_count = 1; /* TASK_Reschedule is called from 16-bit code */
    /* The remaining fields will be initialized in TASK_Reschedule */

    /* Create the 32-bit stack frame */

    stack32Top = (char*)pTask->thdb->teb.stack_top;
    frame16->frame32 = frame32 = (STACK32FRAME *)stack32Top - 1;
    frame32->frame16 = pTask->thdb->cur_stack + sizeof(STACK16FRAME);
    frame32->edi     = 0;
    frame32->esi     = 0;
    frame32->edx     = 0;
    frame32->ecx     = 0;
    frame32->ebx     = 0;
    frame32->retaddr = (DWORD)TASK_CallToStart;
    /* The remaining fields will be initialized in TASK_Reschedule */

    if (!THREAD_Current()->cur_stack)
        THREAD_Current()->cur_stack = pTask->thdb->cur_stack;


    TRACE(task, "module='%s' cmdline='%s' task=%04x\n",
          name, cmd_line, hTask );

    return hTask;
}

/***********************************************************************
 *           TASK_StartTask
 *
 * NOTE: This routine might be called by a Win32 thread. Thus, we need
 *       to be careful to protect global data structures. We do this
 *       by entering the Win16Lock while linking the task into the
 *       global task list.
 */
void TASK_StartTask( HTASK16 hTask )
{
    /* Add the task to the linked list */

    SYSLEVEL_EnterWin16Lock();
    TASK_LinkTask( hTask );
    SYSLEVEL_LeaveWin16Lock();

    TRACE(task, "linked task %04x\n", hTask );

    /* Get the task up and running. If we ourselves are a 16-bit task,
       we simply Yield(). If we are 32-bit however, we need to signal
       the main process somehow (NOT YET IMPLEMENTED!) */

    if ( GetCurrentTask() )
        if ( THREAD_IsWin16( THREAD_Current() ) )
            Yield16();
        else
            FIXME(task, "Don't know how to start 16-bit task from 32-bit thread. Move the mouse!\n");
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

    K32OBJ_DecCount( &pTask->thdb->process->header );
    K32OBJ_DecCount( &pTask->thdb->header );

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
    TDB* pTask = (TDB*) GlobalLock16( GetCurrentTask() );
    NE_MODULE* pModule = NE_GetPtr( pTask->hModule );
    if (!pTask) USER_ExitWindows();  /* No current task yet */

    if ( !THREAD_IsWin16( THREAD_Current() ) )
    {
        FIXME(task, "called for Win32 thread (%04x)!\n", THREAD_Current()->teb_sel);
        return;
    }

    /* Enter the Win16Lock to protect global data structures
       NOTE: We never explicitly leave it again. This shouldn't matter
             though, as it will be released in TASK_Reschedule and this
             task won't ever get scheduled again ... */

    SYSLEVEL_EnterWin16Lock();

    assert(hCurrentTask == GetCurrentTask());

    TRACE(task, "Killing task %04x\n", hCurrentTask );

    /* Delete active sockets */

    if( pTask->pwsi )
	WINSOCK_DeleteTaskWSI( pTask, pTask->pwsi );

#ifdef MZ_SUPPORTED
    /* Kill DOS VM task */
    if ( pModule->lpDosTask )
        MZ_KillModule( pModule->lpDosTask );
#endif

    /* Perform USER cleanup */

    if (pTask->userhandler)
        pTask->userhandler( hCurrentTask, USIG_TERMINATION, 0,
                            pTask->hInstance, pTask->hQueue );

    if (hTaskToKill && (hTaskToKill != hCurrentTask))
    {
        /* If another task is already marked for destruction, */
        /* we can kill it now, as we are in another context.  */ 
        TASK_DeleteTask( hTaskToKill );
    }

    if (nTaskCount <= 1)
    {
        TRACE(task, "this is the last task, exiting\n" );
        USER_ExitWindows();
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

    ERR(task,"Return of the living dead %04x!!!\n", hCurrentTask);
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
    STACK16FRAME *newframe16;

    /* NOTE: As we are entered from 16-bit code, we hold the Win16Lock.
             We hang onto it thoughout most of this routine, so that accesses
             to global variables (most notably the task list) are protected. */
    assert(hCurrentTask == GetCurrentTask());

    TRACE(task, "entered with hTask %04x (pid %d)\n", hCurrentTask, getpid());

#ifdef CONFIG_IPC
    /* FIXME: What about the Win16Lock ??? */
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

    if (!hTask) EVENT_WaitNetEvent( FALSE, TRUE );

    while (!hTask)
    {
        /* Find a task that has an event pending */

        hTask = hFirstTask;
        while (hTask)
        {
            pNewTask = (TDB *)GlobalLock16( hTask );

	    TRACE(task, "\ttask = %04x, events = %i\n", hTask, pNewTask->nEvents);

            if (pNewTask->nEvents) break;
            hTask = pNewTask->hNext;
        }
        if (hLockedTask && (hTask != hLockedTask)) hTask = 0;
        if (hTask) break;

        /* No task found, wait for some events to come in */

        /* NOTE: We release the Win16Lock while waiting for events. This is to enable
                 Win32 threads to thunk down to 16-bit temporarily. Since Win16
                 tasks won't execute and Win32 threads are not allowed to enter 
                 TASK_Reschedule anyway, there should be no re-entrancy problem ... */

        SYSLEVEL_ReleaseWin16Lock();
        EVENT_WaitNetEvent( TRUE, TRUE );
        SYSLEVEL_RestoreWin16Lock();
    }

    if (hTask == hCurrentTask) 
    {
       TRACE(task, "returning to the current task(%04x)\n", hTask );
       return;  /* Nothing to do */
    }
    pNewTask = (TDB *)GlobalLock16( hTask );
    TRACE(task, "Switching to task %04x (%.8s)\n",
                  hTask, pNewTask->module_name );

     /* Make the task the last in the linked list (round-robin scheduling) */

    pNewTask->priority++;
    TASK_UnlinkTask( hTask );
    TASK_LinkTask( hTask );
    pNewTask->priority--;

    /* Finish initializing the new task stack if necessary */

    newframe16 = THREAD_STACK16( pNewTask->thdb );
    if (!newframe16->entry_cs)
    {
        STACK16FRAME *oldframe16 = CURRENT_STACK16;
        STACK32FRAME *oldframe32 = oldframe16->frame32;
        STACK32FRAME *newframe32 = newframe16->frame32;
        newframe16->entry_ip     = oldframe16->entry_ip;
        newframe16->entry_cs     = oldframe16->entry_cs;
        newframe16->ip           = oldframe16->ip;
        newframe16->cs           = oldframe16->cs;
        newframe32->ebp          = oldframe32->ebp;
        newframe32->restore_addr = oldframe32->restore_addr;
        newframe32->codeselector = oldframe32->codeselector;
    }
    
    /* Switch to the new stack */

    /* NOTE: We need to release/restore the Win16Lock, as the task
             switched to might be at another recursion level than
             the old task ... */

    SYSLEVEL_ReleaseWin16Lock();

    hCurrentTask = hTask;
    SET_CUR_THREAD( pNewTask->thdb );
    pNewTask->ss_sp = pNewTask->thdb->cur_stack;

    SYSLEVEL_RestoreWin16Lock();
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

  if ( !THREAD_IsWin16( THREAD_Current() ) )
  {
    FIXME(task, "called for Win32 thread (%04x)!\n", THREAD_Current()->teb_sel);
    return;
  }

  Callbacks->CallTaskRescheduleProc();

  if( pTask )
  {
    pQ = (MESSAGEQUEUE*)GlobalLock16(pTask->hQueue);
    if( pQ && pQ->flags & QUEUE_FLAG_XEVENT &&
	    !(pQ->wakeBits & (QS_SENDMESSAGE | QS_SMRESULT)) )
    {
      pQ->flags &= ~QUEUE_FLAG_XEVENT;
      EVENT_WaitNetEvent( FALSE, FALSE );
    }
  }
}


/***********************************************************************
 *           InitTask  (KERNEL.91)
 *
 * Called by the application startup code.
 */
void WINAPI InitTask( CONTEXT *context )
{
    TDB *pTask;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegTable;
    INSTANCEDATA *pinstance;
    LONG stacklow, stackhi;

    if (context) EAX_reg(context) = 0;
    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return;
    if (!(pModule = NE_GetPtr( pTask->hModule ))) return;

    /* This is a hack to install task USER signal handler before 
     * implicitly loaded DLLs are initialized (see windows/user.c) */

    pTask->userhandler = (USERSIGNALPROC)&USER_SignalProc;

    /* Initialize implicitly loaded DLLs */
    NE_InitializeDLLs( pTask->hModule );

    if (context)
    {
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
        SEGPTR ptr = STACK16_PUSH( pTask->thdb, sizeof(WORD) );
        *(WORD *)PTR_SEG_TO_LIN(ptr) = 0;
        SP_reg(context) -= 2;

        EAX_reg(context) = 1;
        
	if (!pTask->pdb.cmdLine[0]) EBX_reg(context) = 0x80;
	else
        {
            LPBYTE p = &pTask->pdb.cmdLine[1];
            while ((*p == ' ') || (*p == '\t')) p++;
            EBX_reg(context) = 0x80 + (p - pTask->pdb.cmdLine);
        }
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
    pinstance->stackmin    = OFFSETOF( pTask->thdb->cur_stack );
}


/***********************************************************************
 *           WaitEvent  (KERNEL.30)
 */
BOOL16 WINAPI WaitEvent( HTASK16 hTask )
{
    TDB *pTask;

    if (!hTask) hTask = GetCurrentTask();
    pTask = (TDB *)GlobalLock16( hTask );

    if ( !THREAD_IsWin16( THREAD_Current() ) )
    {
        FIXME(task, "called for Win32 thread (%04x)!\n", THREAD_Current()->teb_sel);
        return TRUE;
    }

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
void WINAPI PostEvent( HTASK16 hTask )
{
    TDB *pTask;

    if (!hTask) hTask = GetCurrentTask();
    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return;

    if ( !THREAD_IsWin16( THREAD_Current() ) )
    {
        FIXME(task, "called for Win32 thread (%04x)!\n", THREAD_Current()->teb_sel);
        memset(0, 0, 4096);
        return;
    }

    pTask->nEvents++;
}


/***********************************************************************
 *           SetPriority  (KERNEL.32)
 */
void WINAPI SetPriority( HTASK16 hTask, INT16 delta )
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
HTASK16 WINAPI LockCurrentTask( BOOL16 bLock )
{
    if (bLock) hLockedTask = GetCurrentTask();
    else hLockedTask = 0;
    return hLockedTask;
}


/***********************************************************************
 *           IsTaskLocked  (KERNEL.122)
 */
HTASK16 WINAPI IsTaskLocked(void)
{
    return hLockedTask;
}


/***********************************************************************
 *           OldYield  (KERNEL.117)
 */
void WINAPI OldYield(void)
{
    TDB *pCurTask = (TDB *)GlobalLock16( GetCurrentTask() );

    if ( !THREAD_IsWin16( THREAD_Current() ) )
    {
        FIXME(task, "called for Win32 thread (%04x)!\n", THREAD_Current()->teb_sel);
        return;
    }

    if (pCurTask) pCurTask->nEvents++;  /* Make sure we get back here */
    TASK_YieldToSystem(pCurTask);
    if (pCurTask) pCurTask->nEvents--;
}


/***********************************************************************
 *           DirectedYield  (KERNEL.150)
 */
void WINAPI DirectedYield( HTASK16 hTask )
{
    TDB *pCurTask = (TDB *)GlobalLock16( GetCurrentTask() );

    if ( !THREAD_IsWin16( THREAD_Current() ) )
    {
        FIXME(task, "called for Win32 thread (%04x)!\n", THREAD_Current()->teb_sel);
        return;
    }

    pCurTask->hYieldTo = hTask;
    OldYield();
}


/***********************************************************************
 *           UserYield  (USER.332)
 */
void WINAPI UserYield(void)
{
    TDB *pCurTask = (TDB *)GlobalLock16( GetCurrentTask() );
    MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock16( pCurTask->hQueue );

    if ( !THREAD_IsWin16( THREAD_Current() ) )
    {
        FIXME(task, "called for Win32 thread (%04x)!\n", THREAD_Current()->teb_sel);
        return;
    }

    /* Handle sent messages */
    while (queue && (queue->wakeBits & QS_SENDMESSAGE))
        QUEUE_ReceiveMessage( queue );

    OldYield();

    queue = (MESSAGEQUEUE *)GlobalLock16( pCurTask->hQueue );
    while (queue && (queue->wakeBits & QS_SENDMESSAGE))
        QUEUE_ReceiveMessage( queue );
}


/***********************************************************************
 *           Yield16  (KERNEL.29)
 */
void WINAPI Yield16(void)
{
    TDB *pCurTask = (TDB *)GlobalLock16( GetCurrentTask() );

    if ( !THREAD_IsWin16( THREAD_Current() ) )
    {
        FIXME(task, "called for Win32 thread (%04x)!\n", THREAD_Current()->teb_sel);
        return;
    }

    if (pCurTask) pCurTask->hYieldTo = 0;
    if (pCurTask && pCurTask->hQueue) UserYield();
    else OldYield();
}


/***********************************************************************
 *           MakeProcInstance16  (KERNEL.51)
 */
FARPROC16 WINAPI MakeProcInstance16( FARPROC16 func, HANDLE16 hInstance )
{
    BYTE *thunk,*lfunc;
    SEGPTR thunkaddr;

    if (!func) {
      ERR(task, "Ouch ! MakeProcInstance called with func == NULL !\n");
      return (FARPROC16)0; /* Windows seems to do the same */
    }
    if (!hInstance) hInstance = CURRENT_DS;
    thunkaddr = TASK_AllocThunk( GetCurrentTask() );
    if (!thunkaddr) return (FARPROC16)0;
    thunk = PTR_SEG_TO_LIN( thunkaddr );
    lfunc = PTR_SEG_TO_LIN( func );

    TRACE(task, "(%08lx,%04x): got thunk %08lx\n",
                  (DWORD)func, hInstance, (DWORD)thunkaddr );
    if (((lfunc[0]==0x8c) && (lfunc[1]==0xd8)) ||
    	((lfunc[0]==0x1e) && (lfunc[1]==0x58))
    ) {
    	FIXME(task,"thunk would be useless for %p, overwriting with nop;nop;\n", func );
	lfunc[0]=0x90; /* nop */
	lfunc[1]=0x90; /* nop */
    }
    
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
void WINAPI FreeProcInstance16( FARPROC16 func )
{
    TRACE(task, "(%08lx)\n", (DWORD)func );
    TASK_FreeThunk( GetCurrentTask(), (SEGPTR)func );
}


/**********************************************************************
 *	    GetCodeHandle    (KERNEL.93)
 */
HANDLE16 WINAPI GetCodeHandle( FARPROC16 proc )
{
    HANDLE16 handle;
    BYTE *thunk = (BYTE *)PTR_SEG_TO_LIN( proc );

    /* Return the code segment containing 'proc'. */
    /* Not sure if this is really correct (shouldn't matter that much). */

    /* Check if it is really a thunk */
    if ((thunk[0] == 0xb8) && (thunk[3] == 0xea))
        handle = GlobalHandle16( thunk[6] + (thunk[7] << 8) );
    else
        handle = GlobalHandle16( HIWORD(proc) );

    return handle;
}

/**********************************************************************
 *	    GetCodeInfo    (KERNEL.104)
 */
VOID WINAPI GetCodeInfo( FARPROC16 proc, SEGINFO *segInfo )
{
    BYTE *thunk = (BYTE *)PTR_SEG_TO_LIN( proc );
    NE_MODULE *pModule = NULL;
    SEGTABLEENTRY *pSeg = NULL;
    WORD segNr;

    /* proc is either a thunk, or else a pair of module handle
       and segment number. In the first case, we also need to
       extract module and segment number. */

    if ((thunk[0] == 0xb8) && (thunk[3] == 0xea))
    {
        WORD selector = thunk[6] + (thunk[7] << 8);
        pModule = NE_GetPtr( GlobalHandle16( selector ) );
        pSeg = pModule? NE_SEG_TABLE( pModule ) : NULL;

        if ( pModule )
            for ( segNr = 0; segNr < pModule->seg_count; segNr++, pSeg++ )
                if ( GlobalHandleToSel(pSeg->hSeg) == selector )
                    break;

        if ( pModule && segNr >= pModule->seg_count )
            pSeg = NULL;
    }
    else
    {
        pModule = NE_GetPtr( HIWORD( proc ) );
        segNr   = LOWORD( proc );

        if ( pModule && segNr < pModule->seg_count )
            pSeg = NE_SEG_TABLE( pModule ) + segNr;
    }

    /* fill in segment information */

    segInfo->offSegment = pSeg? pSeg->filepos : 0;
    segInfo->cbSegment  = pSeg? pSeg->size : 0;
    segInfo->flags      = pSeg? pSeg->flags : 0;
    segInfo->cbAlloc    = pSeg? pSeg->minsize : 0;
    segInfo->h          = pSeg? pSeg->hSeg : 0;
    segInfo->alignShift = pModule? pModule->alignment : 0;
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
HQUEUE16 WINAPI SetTaskQueue( HTASK16 hTask, HQUEUE16 hQueue )
{
    HQUEUE16 hPrev;
    TDB *pTask;

    if (!hTask) hTask = GetCurrentTask();
    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return 0;

    hPrev = pTask->hQueue;
    pTask->hQueue = hQueue;

    TIMER_SwitchQueue( hPrev, hQueue );

    return hPrev;
}


/***********************************************************************
 *           GetTaskQueue  (KERNEL.35)
 */
HQUEUE16 WINAPI GetTaskQueue( HTASK16 hTask )
{
    TDB *pTask;

    if (!hTask) hTask = GetCurrentTask();
    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return 0;
    return pTask->hQueue;
}

/***********************************************************************
 *           SetThreadQueue  (KERNEL.463)
 */
HQUEUE16 WINAPI SetThreadQueue( DWORD thread, HQUEUE16 hQueue )
{
    THDB *thdb = THREAD_IdToTHDB( thread );
    HQUEUE16 oldQueue = thdb? thdb->teb.queue : 0;

    if ( thdb )
    {
        thdb->teb.queue = hQueue;

        if ( GetTaskQueue( thdb->process->task ) == oldQueue )
            SetTaskQueue( thdb->process->task, hQueue );
    }

    return oldQueue;
}

/***********************************************************************
 *           GetThreadQueue  (KERNEL.464)
 */
HQUEUE16 WINAPI GetThreadQueue( DWORD thread )
{
    THDB *thdb = THREAD_IdToTHDB( thread );
    return (HQUEUE16)(thdb? thdb->teb.queue : 0);
}

/***********************************************************************
 *           SetFastQueue  (KERNEL.624)
 */
VOID WINAPI SetFastQueue( DWORD thread, HANDLE32 hQueue )
{
    THDB *thdb = THREAD_IdToTHDB( thread );
    if ( thdb ) thdb->teb.queue = (HQUEUE16) hQueue;
}

/***********************************************************************
 *           GetFastQueue  (KERNEL.625)
 */
HANDLE32 WINAPI GetFastQueue( void )
{
    THDB *thdb = THREAD_Current();
    if (!thdb) return 0;

    if (!(thdb->teb.queue))
        FIXME( task, "(): should initialize thread-local queue, expect failure!\n" );

    return (HANDLE32)thdb->teb.queue;
}

/***********************************************************************
 *           SwitchStackTo   (KERNEL.108)
 */
void WINAPI SwitchStackTo( WORD seg, WORD ptr, WORD top )
{
    TDB *pTask;
    STACK16FRAME *oldFrame, *newFrame;
    INSTANCEDATA *pData;
    UINT16 copySize;

    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return;
    if (!(pData = (INSTANCEDATA *)GlobalLock16( seg ))) return;
    TRACE(task, "old=%04x:%04x new=%04x:%04x\n",
                  SELECTOROF( pTask->thdb->cur_stack ),
                  OFFSETOF( pTask->thdb->cur_stack ), seg, ptr );

    /* Save the old stack */

    oldFrame = THREAD_STACK16( pTask->thdb );
    /* pop frame + args and push bp */
    pData->old_ss_sp   = pTask->thdb->cur_stack - sizeof(STACK16FRAME)
                           - 2 * sizeof(WORD);
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
    pTask->thdb->cur_stack = PTR_SEG_OFF_TO_SEGPTR( seg, ptr - copySize );
    newFrame = THREAD_STACK16( pTask->thdb );

    /* Copy the stack frame and the local variables to the new stack */

    memmove( newFrame, oldFrame, copySize );
    newFrame->bp = ptr;
    *(WORD *)PTR_SEG_OFF_TO_LIN( seg, ptr ) = 0;  /* clear previous bp */
}


/***********************************************************************
 *           SwitchStackBack   (KERNEL.109)
 */
void WINAPI SwitchStackBack( CONTEXT *context )
{
    TDB *pTask;
    STACK16FRAME *oldFrame, *newFrame;
    INSTANCEDATA *pData;

    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return;
    if (!(pData = (INSTANCEDATA *)GlobalLock16(SELECTOROF(pTask->thdb->cur_stack))))
        return;
    if (!pData->old_ss_sp)
    {
        WARN( task, "No previous SwitchStackTo\n" );
        return;
    }
    TRACE(task, "restoring stack %04x:%04x\n",
		 SELECTOROF(pData->old_ss_sp), OFFSETOF(pData->old_ss_sp) );

    oldFrame = THREAD_STACK16( pTask->thdb );

    /* Pop bp from the previous stack */

    BP_reg(context) = *(WORD *)PTR_SEG_TO_LIN(pData->old_ss_sp);
    pData->old_ss_sp += sizeof(WORD);

    /* Switch back to the old stack */

    pTask->thdb->cur_stack = pData->old_ss_sp - sizeof(STACK16FRAME);
    SS_reg(context)  = SELECTOROF(pData->old_ss_sp);
    ESP_reg(context) = OFFSETOF(pData->old_ss_sp) - sizeof(DWORD); /*ret addr*/
    pData->old_ss_sp = 0;

    /* Build a stack frame for the return */

    newFrame = THREAD_STACK16( pTask->thdb );
    newFrame->frame32 = oldFrame->frame32;
    if (TRACE_ON(relay))
    {
        newFrame->entry_ip = oldFrame->entry_ip;
        newFrame->entry_cs = oldFrame->entry_cs;
    }
}


/***********************************************************************
 *           GetTaskQueueDS  (KERNEL.118)
 */
void WINAPI GetTaskQueueDS( CONTEXT *context )
{
    DS_reg(context) = GlobalHandleToSel( GetTaskQueue(0) );
}


/***********************************************************************
 *           GetTaskQueueES  (KERNEL.119)
 */
void WINAPI GetTaskQueueES( CONTEXT *context )
{
    ES_reg(context) = GlobalHandleToSel( GetTaskQueue(0) );
}


/***********************************************************************
 *           GetCurrentTask   (KERNEL.36)
 */
HTASK16 WINAPI GetCurrentTask(void)
{
    return THREAD_InitDone? PROCESS_Current()->task : 0;
}

DWORD WINAPI WIN16_GetCurrentTask(void)
{
    /* This is the version used by relay code; the first task is */
    /* returned in the high word of the result */
    return MAKELONG( GetCurrentTask(), hFirstTask );
}


/***********************************************************************
 *           GetCurrentPDB   (KERNEL.37)
 */
HANDLE16 WINAPI GetCurrentPDB(void)
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return 0;
    return pTask->hPDB;
}


/***********************************************************************
 *           GetInstanceData   (KERNEL.54)
 */
INT16 WINAPI GetInstanceData( HINSTANCE16 instance, WORD buffer, INT16 len )
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
WORD WINAPI GetExeVersion(void)
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
    UINT16 oldMode;

    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return 0;
    oldMode = pTask->error_mode;
    pTask->error_mode = mode;
    pTask->thdb->process->error_mode = mode;
    return oldMode;
}


/***********************************************************************
 *           SetErrorMode32   (KERNEL32.486)
 */
UINT32 WINAPI SetErrorMode32( UINT32 mode )
{
    return SetErrorMode16( (UINT16)mode );
}


/***********************************************************************
 *           GetDOSEnvironment   (KERNEL.131)
 */
SEGPTR WINAPI GetDOSEnvironment(void)
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return 0;
    return PTR_SEG_OFF_TO_SEGPTR( pTask->pdb.environment, 0 );
}


/***********************************************************************
 *           GetNumTasks   (KERNEL.152)
 */
UINT16 WINAPI GetNumTasks(void)
{
    return nTaskCount;
}


/***********************************************************************
 *           GetTaskDS   (KERNEL.155)
 *
 * Note: this function apparently returns a DWORD with LOWORD == HIWORD.
 * I don't think we need to bother with this.
 */
HINSTANCE16 WINAPI GetTaskDS(void)
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return 0;
    return pTask->hInstance;
}


/***********************************************************************
 *           IsTask   (KERNEL.320)
 */
BOOL16 WINAPI IsTask( HTASK16 hTask )
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return FALSE;
    if (GlobalSize16( hTask ) < sizeof(TDB)) return FALSE;
    return (pTask->magic == TDB_MAGIC);
}


/***********************************************************************
 *           SetTaskSignalProc   (KERNEL.38)
 *
 * Real 16-bit interface is provided by the THUNK_SetTaskSignalProc.
 */
FARPROC16 WINAPI SetTaskSignalProc( HTASK16 hTask, FARPROC16 proc )
{
    TDB *pTask;
    FARPROC16 oldProc;

    if (!hTask) hTask = GetCurrentTask();
    if (!(pTask = (TDB *)GlobalLock16( hTask ))) return NULL;
    oldProc = (FARPROC16)pTask->userhandler;
    pTask->userhandler = (USERSIGNALPROC)proc;
    return oldProc;
}


/***********************************************************************
 *           SetSigHandler   (KERNEL.140)
 */
WORD WINAPI SetSigHandler( FARPROC16 newhandler, FARPROC16* oldhandler,
                           UINT16 *oldmode, UINT16 newmode, UINT16 flag )
{
    FIXME(task,"(%p,%p,%p,%d,%d), unimplemented.\n",
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
 */
VOID WINAPI GlobalNotify( FARPROC16 proc )
{
    TDB *pTask;

    if (!(pTask = (TDB *)GlobalLock16( GetCurrentTask() ))) return;
    pTask->discardhandler = proc;
}


/***********************************************************************
 *           GetExePtr   (KERNEL.133)
 */
static HMODULE16 GetExePtrHelper( HANDLE16 handle, HTASK16 *hTask )
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

    owner = FarGetOwner( handle );
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

HMODULE16 WINAPI GetExePtr( HANDLE16 handle )
{
    HTASK16 dummy;
    return GetExePtrHelper( handle, &dummy );
}

void WINAPI WIN16_GetExePtr( CONTEXT *context )
{
    WORD *stack = PTR_SEG_OFF_TO_LIN(SS_reg(context), SP_reg(context));
    HANDLE16 handle = (HANDLE16)stack[2];
    HTASK16 hTask = 0;
    HMODULE16 hModule;

    hModule = GetExePtrHelper( handle, &hTask );

    AX_reg(context) = CX_reg(context) = hModule;
    if (hTask) ES_reg(context) = hTask;
}

/***********************************************************************
 *           TaskFirst   (TOOLHELP.63)
 */
BOOL16 WINAPI TaskFirst( TASKENTRY *lpte )
{
    lpte->hNext = hFirstTask;
    return TaskNext( lpte );
}


/***********************************************************************
 *           TaskNext   (TOOLHELP.64)
 */
BOOL16 WINAPI TaskNext( TASKENTRY *lpte )
{
    TDB *pTask;
    INSTANCEDATA *pInstData;

    TRACE(toolhelp, "(%p): task=%04x\n", lpte, lpte->hNext );
    if (!lpte->hNext) return FALSE;
    pTask = (TDB *)GlobalLock16( lpte->hNext );
    if (!pTask || pTask->magic != TDB_MAGIC) return FALSE;
    pInstData = (INSTANCEDATA *)PTR_SEG_OFF_TO_LIN( pTask->hInstance, 0 );
    lpte->hTask         = lpte->hNext;
    lpte->hTaskParent   = pTask->hParent;
    lpte->hInst         = pTask->hInstance;
    lpte->hModule       = pTask->hModule;
    lpte->wSS           = SELECTOROF( pTask->thdb->cur_stack );
    lpte->wSP           = OFFSETOF( pTask->thdb->cur_stack );
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
BOOL16 WINAPI TaskFindHandle( TASKENTRY *lpte, HTASK16 hTask )
{
    lpte->hNext = hTask;
    return TaskNext( lpte );
}


/***********************************************************************
 *           GetAppCompatFlags16   (KERNEL.354)
 */
DWORD WINAPI GetAppCompatFlags16( HTASK16 hTask )
{
    return GetAppCompatFlags32( hTask );
}


/***********************************************************************
 *           GetAppCompatFlags32   (USER32.206)
 */
DWORD WINAPI GetAppCompatFlags32( HTASK32 hTask )
{
    TDB *pTask;

    if (!hTask) hTask = GetCurrentTask();
    if (!(pTask=(TDB *)GlobalLock16( (HTASK16)hTask ))) return 0;
    if (GlobalSize16(hTask) < sizeof(TDB)) return 0;
    return pTask->compat_flags;
}
