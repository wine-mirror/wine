/*
 * Task functions
 *
 * Copyright 1995 Alexandre Julliard
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "winbase.h"
#include "wingdi.h"
#include "winnt.h"
#include "winuser.h"

#include "wine/winbase16.h"
#include "drive.h"
#include "file.h"
#include "global.h"
#include "instance.h"
#include "module.h"
#include "winternl.h"
#include "selectors.h"
#include "wine/server.h"
#include "syslevel.h"
#include "stackframe.h"
#include "task.h"
#include "thread.h"
#include "toolhelp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(task);
WINE_DECLARE_DEBUG_CHANNEL(relay);
WINE_DECLARE_DEBUG_CHANNEL(toolhelp);

  /* Min. number of thunks allocated when creating a new segment */
#define MIN_THUNKS  32


static THHOOK DefaultThhook;
THHOOK *pThhook = &DefaultThhook;

#define hCurrentTask (pThhook->CurTDB)
#define hFirstTask   (pThhook->HeadTDB)
#define hLockedTask  (pThhook->LockTDB)

static UINT16 nTaskCount = 0;

static HTASK16 initial_task;

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
    TDB* pTask = TASK_GetPtr( hTask );

    if (pTask->hNext) return pTask->hNext;
    return (hFirstTask != hTask) ? hFirstTask : 0;
}


/***********************************************************************
 *	     TASK_GetPtr
 */
TDB *TASK_GetPtr( HTASK16 hTask )
{
    return GlobalLock16( hTask );
}


/***********************************************************************
 *	     TASK_GetCurrent
 */
TDB *TASK_GetCurrent(void)
{
    return TASK_GetPtr( GetCurrentTask() );
}


/***********************************************************************
 *           TASK_LinkTask
 */
static void TASK_LinkTask( HTASK16 hTask )
{
    HTASK16 *prevTask;
    TDB *pTask;

    if (!(pTask = TASK_GetPtr( hTask ))) return;
    prevTask = &hFirstTask;
    while (*prevTask)
    {
        TDB *prevTaskPtr = TASK_GetPtr( *prevTask );
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
        pTask = TASK_GetPtr( *prevTask );
        prevTask = &pTask->hNext;
    }
    if (*prevTask)
    {
        pTask = TASK_GetPtr( *prevTask );
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
static SEGPTR TASK_AllocThunk(void)
{
    TDB *pTask;
    THUNKS *pThunk;
    WORD sel, base;

    if (!(pTask = TASK_GetCurrent())) return 0;
    sel = pTask->hCSAlias;
    pThunk = &pTask->thunks;
    base = (int)pThunk - (int)pTask;
    while (!pThunk->free)
    {
        sel = pThunk->next;
        if (!sel)  /* Allocate a new segment */
        {
            sel = GLOBAL_Alloc( GMEM_FIXED, sizeof(THUNKS) + (MIN_THUNKS-1)*8,
                                pTask->hPDB, WINE_LDT_FLAGS_CODE );
            if (!sel) return (SEGPTR)0;
            TASK_CreateThunks( sel, 0, MIN_THUNKS );
            pThunk->next = sel;
        }
        pThunk = (THUNKS *)GlobalLock16( sel );
        base = 0;
    }
    base += pThunk->free;
    pThunk->free = *(WORD *)((BYTE *)pThunk + pThunk->free);
    return MAKESEGPTR( sel, base );
}


/***********************************************************************
 *           TASK_FreeThunk
 *
 * Free a MakeProcInstance() thunk.
 */
static BOOL TASK_FreeThunk( SEGPTR thunk )
{
    TDB *pTask;
    THUNKS *pThunk;
    WORD sel, base;

    if (!(pTask = TASK_GetCurrent())) return 0;
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
 *           TASK_Create
 *
 * NOTE: This routine might be called by a Win32 thread. Thus, we need
 *       to be careful to protect global data structures. We do this
 *       by entering the Win16Lock while linking the task into the
 *       global task list.
 */
static TDB *TASK_Create( NE_MODULE *pModule, UINT16 cmdShow, TEB *teb, LPCSTR cmdline, BYTE len )
{
    HTASK16 hTask;
    TDB *pTask;
    char name[10];
    FARPROC16 proc;
    HMODULE16 hModule = pModule ? pModule->self : 0;

      /* Allocate the task structure */

    hTask = GlobalAlloc16( GMEM_FIXED | GMEM_ZEROINIT, sizeof(TDB) );
    if (!hTask) return NULL;
    pTask = TASK_GetPtr( hTask );
    FarSetOwner16( hTask, hModule );

    /* Fill the task structure */

    pTask->hSelf = hTask;

    if (teb && teb->tibflags & TEBF_WIN32)
    {
        pTask->flags        |= TDBF_WIN32;
        pTask->hInstance     = hModule;
        pTask->hPrevInstance = 0;
        /* NOTE: for 16-bit tasks, the instance handles are updated later on
           in NE_InitProcess */
    }

    pTask->version       = pModule ? pModule->expected_version : 0x0400;
    pTask->hModule       = hModule;
    pTask->hParent       = GetCurrentTask();
    pTask->magic         = TDB_MAGIC;
    pTask->nCmdShow      = cmdShow;
    pTask->teb           = teb;
    pTask->curdrive      = DRIVE_GetCurrentDrive() | 0x80;
    strcpy( pTask->curdir, "\\" );
    WideCharToMultiByte(CP_ACP, 0, DRIVE_GetDosCwd(DRIVE_GetCurrentDrive()), -1,
                        pTask->curdir + 1, sizeof(pTask->curdir) - 1, NULL, NULL);
    pTask->curdir[sizeof(pTask->curdir) - 1] = 0; /* ensure 0 termination */

      /* Create the thunks block */

    TASK_CreateThunks( hTask, (int)&pTask->thunks - (int)pTask, 7 );

      /* Copy the module name */

    if (hModule)
    {
        GetModuleName16( hModule, name, sizeof(name) );
        strncpy( pTask->module_name, name, sizeof(pTask->module_name) );
    }

      /* Allocate a selector for the PDB */

    pTask->hPDB = GLOBAL_CreateBlock( GMEM_FIXED, &pTask->pdb, sizeof(PDB16),
                                      hModule, WINE_LDT_FLAGS_DATA );

      /* Fill the PDB */

    pTask->pdb.int20 = 0x20cd;
    pTask->pdb.dispatcher[0] = 0x9a;  /* ljmp */
    proc = GetProcAddress16( GetModuleHandle16("KERNEL"), "DOS3Call" );
    memcpy( &pTask->pdb.dispatcher[1], &proc, sizeof(proc) );
    pTask->pdb.savedint22 = 0;
    pTask->pdb.savedint23 = 0;
    pTask->pdb.savedint24 = 0;
    pTask->pdb.fileHandlesPtr =
        MAKESEGPTR( GlobalHandleToSel16(pTask->hPDB), (int)&((PDB16 *)0)->fileHandles );
    pTask->pdb.hFileHandles = 0;
    memset( pTask->pdb.fileHandles, 0xff, sizeof(pTask->pdb.fileHandles) );
    /* FIXME: should we make a copy of the environment? */
    pTask->pdb.environment    = SELECTOROF(GetDOSEnvironment16());
    pTask->pdb.nbFiles        = 20;

    /* Fill the command line */

    if (!cmdline)
    {
        cmdline = GetCommandLineA();
        /* remove the first word (program name) */
        if (*cmdline == '"')
            if (!(cmdline = strchr( cmdline+1, '"' ))) cmdline = GetCommandLineA();
        while (*cmdline && (*cmdline != ' ') && (*cmdline != '\t')) cmdline++;
        while ((*cmdline == ' ') || (*cmdline == '\t')) cmdline++;
        len = strlen(cmdline);
    }
    if (len >= sizeof(pTask->pdb.cmdLine)) len = sizeof(pTask->pdb.cmdLine)-1;
    pTask->pdb.cmdLine[0] = len;
    memcpy( pTask->pdb.cmdLine + 1, cmdline, len );
    /* pTask->pdb.cmdLine[len+1] = 0; */

    TRACE("module='%s' cmdline='%.*s' task=%04x\n", name, len, cmdline, hTask );

      /* Get the compatibility flags */

    pTask->compat_flags = GetProfileIntA( "Compatibility", name, 0 );

      /* Allocate a code segment alias for the TDB */

    pTask->hCSAlias = GLOBAL_CreateBlock( GMEM_FIXED, (void *)pTask,
                                          sizeof(TDB), pTask->hPDB, WINE_LDT_FLAGS_CODE );

      /* Default DTA overwrites command line */

    pTask->dta = MAKESEGPTR( pTask->hPDB, (int)&pTask->pdb.cmdLine - (int)&pTask->pdb );

    /* Create scheduler event for 16-bit tasks */

    if ( !(pTask->flags & TDBF_WIN32) )
        NtCreateEvent( &pTask->hEvent, EVENT_ALL_ACCESS, NULL, TRUE, FALSE );

    /* Enter task handle into thread */

    if (teb) teb->htask16 = hTask;
    if (!initial_task) initial_task = hTask;

    return pTask;
}


/***********************************************************************
 *           TASK_DeleteTask
 */
static void TASK_DeleteTask( HTASK16 hTask )
{
    TDB *pTask;
    HGLOBAL16 hPDB;

    if (!(pTask = TASK_GetPtr( hTask ))) return;
    hPDB = pTask->hPDB;

    pTask->magic = 0xdead; /* invalidate signature */

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
 *           TASK_CreateMainTask
 *
 * Create a task for the main (32-bit) process.
 */
void TASK_CreateMainTask(void)
{
    TDB *pTask;
    STARTUPINFOA startup_info;
    UINT cmdShow = 1; /* SW_SHOWNORMAL but we don't want to include winuser.h here */

    GetStartupInfoA( &startup_info );
    if (startup_info.dwFlags & STARTF_USESHOWWINDOW) cmdShow = startup_info.wShowWindow;
    pTask = TASK_Create( NULL, cmdShow, NtCurrentTeb(), NULL, 0 );
    if (!pTask)
    {
        ERR("could not create task for main process\n");
        ExitProcess(1);
    }

    /* Add the task to the linked list */
    /* (no need to get the win16 lock, we are the only thread at this point) */
    TASK_LinkTask( pTask->hSelf );
}


/* startup routine for a new 16-bit thread */
static DWORD CALLBACK task_start( TDB *pTask )
{
    DWORD ret;

    NtCurrentTeb()->tibflags &= ~TEBF_WIN32;
    NtCurrentTeb()->htask16 = pTask->hSelf;

    _EnterWin16Lock();
    TASK_LinkTask( pTask->hSelf );
    pTask->teb = NtCurrentTeb();
    ret = NE_StartTask();
    _LeaveWin16Lock();
    return ret;
}


/***********************************************************************
 *           TASK_SpawnTask
 *
 * Spawn a new 16-bit task.
 */
HTASK16 TASK_SpawnTask( NE_MODULE *pModule, WORD cmdShow,
                        LPCSTR cmdline, BYTE len, HANDLE *hThread )
{
    TDB *pTask;

    if (!(pTask = TASK_Create( pModule, cmdShow, NULL, cmdline, len ))) return 0;
    if (!(*hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)task_start, pTask, 0, NULL )))
    {
        TASK_DeleteTask( pTask->hSelf );
        return 0;
    }
    return pTask->hSelf;
}


/***********************************************************************
 *           TASK_ExitTask
 */
void TASK_ExitTask(void)
{
    TDB *pTask;
    DWORD lockCount;

    /* Enter the Win16Lock to protect global data structures */
    _EnterWin16Lock();

    pTask = TASK_GetCurrent();
    if ( !pTask )
    {
        _LeaveWin16Lock();
        return;
    }

    TRACE("Killing task %04x\n", pTask->hSelf );

    /* Perform USER cleanup */

    TASK_CallTaskSignalProc( USIG16_TERMINATION, pTask->hSelf );

    /* Remove the task from the list to be sure we never switch back to it */
    TASK_UnlinkTask( pTask->hSelf );

    if (!nTaskCount || (nTaskCount == 1 && hFirstTask == initial_task))
    {
        TRACE("this is the last task, exiting\n" );
        ExitKernel16();
    }

    if( nTaskCount )
    {
        TDB* p = TASK_GetPtr( hFirstTask );
        while( p )
        {
            if( p->hYieldTo == pTask->hSelf ) p->hYieldTo = 0;
            p = TASK_GetPtr( p->hNext );
        }
    }

    pTask->nEvents = 0;

    if ( hLockedTask == pTask->hSelf )
        hLockedTask = 0;

    TASK_DeleteTask( pTask->hSelf );

    /* ... and completely release the Win16Lock, just in case. */
    ReleaseThunkLock( &lockCount );
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

    context->Eax = 0;
    if (!(pTask = TASK_GetCurrent())) return;

    /* Note: we need to trust that BX/CX contain the stack/heap sizes,
       as some apps, notably Visual Basic apps, *modify* the heap/stack
       size of the instance data segment before calling InitTask() */

    /* Initialize the INSTANCEDATA structure */
    pinstance = MapSL( MAKESEGPTR(CURRENT_DS, 0) );
    pinstance->stackmin    = OFFSETOF( pTask->teb->cur_stack ) + sizeof( STACK16FRAME );
    pinstance->stackbottom = pinstance->stackmin; /* yup, that's right. Confused me too. */
    pinstance->stacktop    = ( pinstance->stackmin > LOWORD(context->Ebx) ?
                               pinstance->stackmin - LOWORD(context->Ebx) : 0 ) + 150;

    /* Initialize the local heap */
    if (LOWORD(context->Ecx))
        LocalInit16( GlobalHandleToSel16(pTask->hInstance), 0, LOWORD(context->Ecx) );

    /* Initialize implicitly loaded DLLs */
    NE_InitializeDLLs( pTask->hModule );
    NE_DllProcessAttach( pTask->hModule );

    /* Registers on return are:
     * ax     1 if OK, 0 on error
     * cx     stack limit in bytes
     * dx     cmdShow parameter
     * si     instance handle of the previous instance
     * di     instance handle of the new task
     * es:bx  pointer to command line inside PSP
     *
     * 0 (=%bp) is pushed on the stack
     */
    ptr = stack16_push( sizeof(WORD) );
    *(WORD *)MapSL(ptr) = 0;
    context->Esp -= 2;

    context->Eax = 1;

    if (!pTask->pdb.cmdLine[0]) context->Ebx = 0x80;
    else
    {
        LPBYTE p = &pTask->pdb.cmdLine[1];
        while ((*p == ' ') || (*p == '\t')) p++;
        context->Ebx = 0x80 + (p - pTask->pdb.cmdLine);
    }
    context->Ecx   = pinstance->stacktop;
    context->Edx   = pTask->nCmdShow;
    context->Esi   = (DWORD)pTask->hPrevInstance;
    context->Edi   = (DWORD)pTask->hInstance;
    context->SegEs = (WORD)pTask->hPDB;
}


/***********************************************************************
 *           WaitEvent  (KERNEL.30)
 */
BOOL16 WINAPI WaitEvent16( HTASK16 hTask )
{
    TDB *pTask;

    if (!hTask) hTask = GetCurrentTask();
    pTask = TASK_GetPtr( hTask );

    if (pTask->flags & TDBF_WIN32)
    {
        FIXME("called for Win32 thread (%04x)!\n", NtCurrentTeb()->teb_sel);
        return TRUE;
    }

    if (pTask->nEvents > 0)
    {
        pTask->nEvents--;
        return FALSE;
    }

    if (pTask->teb == NtCurrentTeb())
    {
        DWORD lockCount;

        NtResetEvent( pTask->hEvent, NULL );
        ReleaseThunkLock( &lockCount );
        SYSLEVEL_CheckNotLevel( 1 );
        WaitForSingleObject( pTask->hEvent, INFINITE );
        RestoreThunkLock( lockCount );
        if (pTask->nEvents > 0) pTask->nEvents--;
    }
    else FIXME("for other task %04x cur=%04x\n",pTask->hSelf,GetCurrentTask());

    return TRUE;
}


/***********************************************************************
 *           PostEvent  (KERNEL.31)
 */
void WINAPI PostEvent16( HTASK16 hTask )
{
    TDB *pTask;

    if (!hTask) hTask = GetCurrentTask();
    if (!(pTask = TASK_GetPtr( hTask ))) return;

    if (pTask->flags & TDBF_WIN32)
    {
        FIXME("called for Win32 thread (%04x)!\n", pTask->teb->teb_sel );
        return;
    }

    pTask->nEvents++;

    if (pTask->nEvents == 1) NtSetEvent( pTask->hEvent, NULL );
}


/***********************************************************************
 *           SetPriority  (KERNEL.32)
 */
void WINAPI SetPriority16( HTASK16 hTask, INT16 delta )
{
    TDB *pTask;
    INT16 newpriority;

    if (!hTask) hTask = GetCurrentTask();
    if (!(pTask = TASK_GetPtr( hTask ))) return;
    newpriority = pTask->priority + delta;
    if (newpriority < -32) newpriority = -32;
    else if (newpriority > 15) newpriority = 15;

    pTask->priority = newpriority + 1;
    TASK_UnlinkTask( pTask->hSelf );
    TASK_LinkTask( pTask->hSelf );
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
   DWORD count;

   ReleaseThunkLock(&count);
   RestoreThunkLock(count);
}

/***********************************************************************
 *           WIN32_OldYield  (KERNEL.447)
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
    TDB *pCurTask = TASK_GetCurrent();

    if (!pCurTask || (pCurTask->flags & TDBF_WIN32)) OldYield16();
    else
    {
        TRACE("%04x: DirectedYield(%04x)\n", pCurTask->hSelf, hTask );
        pCurTask->hYieldTo = hTask;
        OldYield16();
        TRACE("%04x: back from DirectedYield(%04x)\n", pCurTask->hSelf, hTask );
    }
}

/***********************************************************************
 *           Yield  (KERNEL.29)
 */
void WINAPI Yield16(void)
{
    TDB *pCurTask = TASK_GetCurrent();

    if (pCurTask) pCurTask->hYieldTo = 0;
    if (pCurTask && pCurTask->hQueue)
    {
        HMODULE mod = GetModuleHandleA( "user32.dll" );
        if (mod)
        {
            FARPROC proc = GetProcAddress( mod, "UserYield16" );
            if (proc)
            {
                proc();
                return;
            }
        }
    }
    OldYield16();
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
 *           MakeProcInstance  (KERNEL.51)
 */
FARPROC16 WINAPI MakeProcInstance16( FARPROC16 func, HANDLE16 hInstance )
{
    BYTE *thunk,*lfunc;
    SEGPTR thunkaddr;
    WORD hInstanceSelector;

    hInstanceSelector = GlobalHandleToSel16(hInstance);

    TRACE("(%08lx, %04x);\n", (DWORD)func, hInstance);

    if (!HIWORD(func)) {
      /* Win95 actually protects via SEH, but this is better for debugging */
      WARN("Ouch ! Called with invalid func 0x%08lx !\n", (DWORD)func);
      return (FARPROC16)0;
    }

    if ( (GlobalHandleToSel16(CURRENT_DS) != hInstanceSelector)
      && (hInstance != 0)
      && (hInstance != 0xffff) )
    {
	/* calling MPI with a foreign DSEG is invalid ! */
        WARN("Problem with hInstance? Got %04x, using %04x instead\n",
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

    thunkaddr = TASK_AllocThunk();
    if (!thunkaddr) return (FARPROC16)0;
    thunk = MapSL( thunkaddr );
    lfunc = MapSL( (SEGPTR)func );

    TRACE("(%08lx,%04x): got thunk %08lx\n",
          (DWORD)func, hInstance, (DWORD)thunkaddr );
    if (((lfunc[0]==0x8c) && (lfunc[1]==0xd8)) || /* movw %ds, %ax */
    	((lfunc[0]==0x1e) && (lfunc[1]==0x58))    /* pushw %ds, popw %ax */
    ) {
    	WARN("This was the (in)famous \"thunk useless\" warning. We thought we have to overwrite with nop;nop;, but this isn't true.\n");
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
 *           FreeProcInstance  (KERNEL.52)
 */
void WINAPI FreeProcInstance16( FARPROC16 func )
{
    TRACE("(%08lx)\n", (DWORD)func );
    TASK_FreeThunk( (SEGPTR)func );
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
    int segNr=0;

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
        BYTE *thunk = MapSL( (SEGPTR)proc );
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
 *          DefineHandleTable    (KERNEL.94)
 */
BOOL16 WINAPI DefineHandleTable16( WORD wOffset )
{
    FIXME("(%04x): stub ?\n", wOffset);
    return TRUE;
}


/***********************************************************************
 *           SetTaskQueue  (KERNEL.34)
 */
HQUEUE16 WINAPI SetTaskQueue16( HTASK16 hTask, HQUEUE16 hQueue )
{
    HQUEUE16 hPrev;
    TDB *pTask;

    if (!hTask) hTask = GetCurrentTask();
    if (!(pTask = TASK_GetPtr( hTask ))) return 0;

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
    if (!(pTask = TASK_GetPtr( hTask ))) return 0;
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

        if ( GetTaskQueue16( teb->htask16 ) == oldQueue )
            SetTaskQueue16( teb->htask16, hQueue );
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
        teb = (TASK_GetPtr( (HANDLE16)thread ))->teb;

    return (HQUEUE16)(teb? teb->queue : 0);
}

/***********************************************************************
 *           SetFastQueue  (KERNEL.624)
 */
VOID WINAPI SetFastQueue16( DWORD thread, HQUEUE16 hQueue )
{
    TEB *teb = NULL;
    if ( !thread )
        teb = NtCurrentTeb();
    else if ( HIWORD(thread) )
        teb = THREAD_IdToTEB( thread );
    else if ( IsTask16( (HTASK16)thread ) )
        teb = (TASK_GetPtr( (HANDLE16)thread ))->teb;

    if ( teb ) teb->queue = hQueue;
}

/***********************************************************************
 *           GetFastQueue  (KERNEL.625)
 */
HQUEUE16 WINAPI GetFastQueue16( void )
{
    HQUEUE16 ret = NtCurrentTeb()->queue;

    if (!ret) FIXME("(): should initialize thread-local queue, expect failure!\n" );
    return ret;
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

    if (!(pTask = TASK_GetCurrent())) return;
    if (!(pData = (INSTANCEDATA *)GlobalLock16( seg ))) return;
    TRACE("old=%04x:%04x new=%04x:%04x\n",
          SELECTOROF( pTask->teb->cur_stack ),
          OFFSETOF( pTask->teb->cur_stack ), seg, ptr );

    /* Save the old stack */

    oldFrame = THREAD_STACK16( pTask->teb );
    /* pop frame + args and push bp */
    pData->old_ss_sp   = pTask->teb->cur_stack + sizeof(STACK16FRAME)
                           + 2 * sizeof(WORD);
    *(WORD *)MapSL(pData->old_ss_sp) = oldFrame->bp;
    pData->stacktop    = top;
    pData->stackmin    = ptr;
    pData->stackbottom = ptr;

    /* Switch to the new stack */

    /* Note: we need to take the 3 arguments into account; otherwise,
     * the stack will underflow upon return from this function.
     */
    copySize = oldFrame->bp - OFFSETOF(pData->old_ss_sp);
    copySize += 3 * sizeof(WORD) + sizeof(STACK16FRAME);
    pTask->teb->cur_stack = MAKESEGPTR( seg, ptr - copySize );
    newFrame = THREAD_STACK16( pTask->teb );

    /* Copy the stack frame and the local variables to the new stack */

    memmove( newFrame, oldFrame, copySize );
    newFrame->bp = ptr;
    *(WORD *)MapSL( MAKESEGPTR( seg, ptr ) ) = 0;  /* clear previous bp */
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

    context->Ebp = (context->Ebp & ~0xffff) | *(WORD *)MapSL(pData->old_ss_sp);
    pData->old_ss_sp += sizeof(WORD);

    /* Switch back to the old stack */

    NtCurrentTeb()->cur_stack = pData->old_ss_sp - sizeof(STACK16FRAME);
    context->SegSs = SELECTOROF(pData->old_ss_sp);
    context->Esp   = OFFSETOF(pData->old_ss_sp) - sizeof(DWORD); /*ret addr*/
    pData->old_ss_sp = 0;

    /* Build a stack frame for the return */

    newFrame = CURRENT_STACK16;
    newFrame->frame32     = oldFrame->frame32;
    newFrame->module_cs   = oldFrame->module_cs;
    newFrame->callfrom_ip = oldFrame->callfrom_ip;
    newFrame->entry_ip    = oldFrame->entry_ip;
}


/***********************************************************************
 *           GetTaskQueueDS  (KERNEL.118)
 */
void WINAPI GetTaskQueueDS16(void)
{
    CURRENT_STACK16->ds = GlobalHandleToSel16( GetTaskQueue16(0) );
}


/***********************************************************************
 *           GetTaskQueueES  (KERNEL.119)
 */
void WINAPI GetTaskQueueES16(void)
{
    CURRENT_STACK16->es = GlobalHandleToSel16( GetTaskQueue16(0) );
}


/***********************************************************************
 *           GetCurrentTask   (KERNEL32.@)
 */
HTASK16 WINAPI GetCurrentTask(void)
{
    return NtCurrentTeb()->htask16;
}

/***********************************************************************
 *           GetCurrentTask   (KERNEL.36)
 */
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

    if (!(pTask = TASK_GetCurrent())) return 0;
    return MAKELONG(pTask->hPDB, 0); /* FIXME */
}


/***********************************************************************
 *           GetCurPID   (KERNEL.157)
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

    if (!(pTask = TASK_GetCurrent())) return 0;
    return pTask->version;
}


/***********************************************************************
 *           SetErrorMode   (KERNEL.107)
 */
UINT16 WINAPI SetErrorMode16( UINT16 mode )
{
    TDB *pTask;
    if (!(pTask = TASK_GetCurrent())) return 0;
    pTask->error_mode = mode;
    return SetErrorMode( mode );
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

    if (!(pTask = TASK_GetCurrent())) return 0;
    return GlobalHandleToSel16(pTask->hInstance);
}

/***********************************************************************
 *           GetDummyModuleHandleDS   (KERNEL.602)
 */
WORD WINAPI GetDummyModuleHandleDS16(void)
{
    TDB *pTask;
    WORD selector;

    if (!(pTask = TASK_GetCurrent())) return 0;
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

    if (!(pTask = TASK_GetPtr( hTask ))) return FALSE;
    if (GlobalSize16( hTask ) < sizeof(TDB)) return FALSE;
    return (pTask->magic == TDB_MAGIC);
}


/***********************************************************************
 *           IsWinOldApTask   (KERNEL.158)
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
    if (!(pTask = TASK_GetPtr( hTask ))) return NULL;
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
    TDB *pTask = TASK_GetCurrent();
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

        if (!(pTask = TASK_GetCurrent())) return 0;
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

    if (!(pTask = TASK_GetCurrent())) return;
    pTask->discardhandler = proc;
}


/***********************************************************************
 *           GetExePtrHelper
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
        TDB *pTask = TASK_GetPtr( *hTask );
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
        TDB *pTask = TASK_GetPtr( *hTask );
        if ((*hTask == owner) ||
            (pTask->hInstance == owner) ||
            (pTask->hQueue == owner) ||
            (pTask->hPDB == owner)) return pTask->hModule;
        *hTask = pTask->hNext;
    }

    return 0;
}

/***********************************************************************
 *           GetExePtr   (KERNEL.133)
 */
HMODULE16 WINAPI WIN16_GetExePtr( HANDLE16 handle )
{
    HTASK16 hTask = 0;
    HMODULE16 hModule = GetExePtrHelper( handle, &hTask );
    STACK16FRAME *frame = CURRENT_STACK16;
    frame->ecx = hModule;
    if (hTask) frame->es = hTask;
    return hModule;
}


/***********************************************************************
 *           K228   (KERNEL.228)
 */
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

    /* make sure that task and hInstance are valid (skip initial Wine task !) */
    while (1) {
        pTask = TASK_GetPtr( lpte->hNext );
        if (!pTask || pTask->magic != TDB_MAGIC) return FALSE;
        if (pTask->hInstance)
            break;
        lpte->hNext = pTask->hNext;
    }
    pInstData = MapSL( MAKESEGPTR( GlobalHandleToSel16(pTask->hInstance), 0 ) );
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


typedef INT (WINAPI *MessageBoxA_funcptr)(HWND hWnd, LPCSTR text, LPCSTR title, UINT type);

/**************************************************************************
 *           FatalAppExit   (KERNEL.137)
 */
void WINAPI FatalAppExit16( UINT16 action, LPCSTR str )
{
    TDB *pTask = TASK_GetCurrent();

    if (!pTask || !(pTask->error_mode & SEM_NOGPFAULTERRORBOX))
    {
        HMODULE mod = GetModuleHandleA( "user32.dll" );
        if (mod)
        {
            MessageBoxA_funcptr pMessageBoxA = (MessageBoxA_funcptr)GetProcAddress( mod, "MessageBoxA" );
            if (pMessageBoxA)
            {
                pMessageBoxA( 0, str, NULL, MB_SYSTEMMODAL | MB_OK );
                goto done;
            }
        }
        ERR( "%s\n", debugstr_a(str) );
    }
 done:
    ExitThread(0xff);
}


/***********************************************************************
 *           TerminateApp   (TOOLHELP.77)
 *
 * See "Undocumented Windows".
 */
void WINAPI TerminateApp16(HTASK16 hTask, WORD wFlags)
{
    if (hTask && hTask != GetCurrentTask())
    {
        FIXME("cannot terminate task %x\n", hTask);
        return;
    }

    if (wFlags & NO_UAE_BOX)
    {
        UINT16 old_mode;
        old_mode = SetErrorMode16(0);
        SetErrorMode16(old_mode|SEM_NOGPFAULTERRORBOX);
    }
    FatalAppExit16( 0, NULL );

    /* hmm, we're still alive ?? */

    /* check undocumented flag */
    if (!(wFlags & 0x8000))
        TASK_CallTaskSignalProc( USIG16_TERMINATION, hTask );

    /* UndocWin says to call int 0x21/0x4c exit=0xff here,
       but let's just call ExitThread */
    ExitThread(0xff);
}


/***********************************************************************
 *           GetAppCompatFlags   (KERNEL.354)
 */
DWORD WINAPI GetAppCompatFlags16( HTASK16 hTask )
{
    TDB *pTask;

    if (!hTask) hTask = GetCurrentTask();
    if (!(pTask=TASK_GetPtr( hTask ))) return 0;
    if (GlobalSize16(hTask) < sizeof(TDB)) return 0;
    return pTask->compat_flags;
}
