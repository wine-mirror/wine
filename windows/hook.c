/*
 * Windows hook functions
 *
 * Copyright 1994, 1995 Alexandre Julliard
 *
 * Based on investigations by Alex Korobka
 */

/*
 * Warning!
 * A HHOOK is a 32-bit handle for compatibility with Windows 3.0 where it was
 * a pointer to the next function. Now it is in fact composed of a USER heap
 * handle in the low 16 bits and of a HOOK_MAGIC value in the high 16 bits
 * (except for WINELIB32 where it is a 32-bit handle).  -- AJ
 */

#include "hook.h"
#include "queue.h"
#include "user.h"
#include "stddebug.h"
#include "debug.h"

  /* This should probably reside in USER heap */
static HANDLE HOOK_systemHooks[WH_NB_HOOKS] = { 0, };


/***********************************************************************
 *           HOOK_GetNextHook
 *
 * Get the next hook of a given hook.
 */
static HANDLE HOOK_GetNextHook( HANDLE hook )
{
    HOOKDATA *data = (HOOKDATA *)USER_HEAP_LIN_ADDR( hook );
    if (!data || !hook) return 0;
    if (data->next) return data->next;
    if (!data->ownerQueue) return 0;  /* Already system hook */
    /* Now start enumerating the system hooks */
    return HOOK_systemHooks[data->id - WH_FIRST_HOOK];
}


/***********************************************************************
 *           HOOK_GetHook
 *
 * Get the first hook for a given type.
 */
static HANDLE HOOK_GetHook( short id , HQUEUE hQueue )
{
    MESSAGEQUEUE *queue;
    HANDLE hook = 0;

    if ((queue = (MESSAGEQUEUE *)GlobalLock( GetTaskQueue(hQueue) )) != NULL)
        hook = queue->hooks[id - WH_FIRST_HOOK];
    if (!hook) hook = HOOK_systemHooks[id - WH_FIRST_HOOK];
    return hook;
}


/***********************************************************************
 *           HOOK_SetHook
 *
 * Install a given hook.
 */
HANDLE HOOK_SetHook( short id, HOOKPROC proc, HINSTANCE hInst, HTASK hTask )
{
    HOOKDATA *data;
    HANDLE handle;
    HQUEUE hQueue = 0;

    if ((id < WH_FIRST_HOOK) || (id > WH_LAST_HOOK)) return 0;
    if (!(hInst = GetExePtr( hInst ))) return 0;

    dprintf_hook( stddeb, "Setting hook %d: %08lx %04x %04x\n",
                  id, (DWORD)proc, hInst, hTask );

    if (hTask)  /* Task-specific hook */
    {
	if ((id == WH_JOURNALRECORD) || (id == WH_JOURNALPLAYBACK) ||
	    (id == WH_SYSMSGFILTER)) return 0;  /* System-only hooks */
        if (!(hQueue = GetTaskQueue( hTask ))) return 0;
    }

    if (id == WH_JOURNALPLAYBACK || id == WH_CBT ||
        id == WH_DEBUG || id == WH_SHELL)
    {
	fprintf( stdnimp, "Unimplemented hook set: (%d,%08lx,%04x,%04x)!\n",
                 id, (DWORD)proc, hInst, hTask );
    }

    /* Create the hook structure */

    if (!(handle = (HANDLE)USER_HEAP_ALLOC( sizeof(HOOKDATA) ))) return 0;
    data = (HOOKDATA *) USER_HEAP_LIN_ADDR( handle );
    data->proc        = proc;
    data->id          = id;
    data->ownerQueue  = hQueue;
    data->ownerModule = hInst;
    data->inHookProc  = 0;
    dprintf_hook( stddeb, "Setting hook %d: ret=%04x\n", id, handle );

    /* Insert it in the correct linked list */

    if (hQueue)
    {
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock( hQueue );
        data->next = queue->hooks[id - WH_FIRST_HOOK];
        queue->hooks[id - WH_FIRST_HOOK] = handle;
    }
    else
    {
        data->next = HOOK_systemHooks[id - WH_FIRST_HOOK];
        HOOK_systemHooks[id - WH_FIRST_HOOK] = handle;
    }
    return handle;
}


/***********************************************************************
 *           HOOK_RemoveHook
 *
 * Remove a hook from the list.
 */
static BOOL HOOK_RemoveHook( HANDLE hook )
{
    HOOKDATA *data;
    HANDLE *prevHook;

    dprintf_hook( stddeb, "Removing hook %04x\n", hook );

    if (!(data = (HOOKDATA *)USER_HEAP_LIN_ADDR(hook))) return FALSE;
    if (data->inHookProc)
    {
        /* Mark it for deletion later on */
        dprintf_hook( stddeb, "Hook still running, deletion delayed\n" );
        data->proc = (FARPROC)0;
        return TRUE;
    }

    /* Remove it from the linked list */

    if (data->ownerQueue)
    {
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock( data->ownerQueue );
        if (!queue) return FALSE;
        prevHook = &queue->hooks[data->id - WH_FIRST_HOOK];
    }
    else prevHook = &HOOK_systemHooks[data->id - WH_FIRST_HOOK];

    while (*prevHook && *prevHook != hook)
        prevHook = &((HOOKDATA *)USER_HEAP_LIN_ADDR(*prevHook))->next;

     if (!*prevHook) return FALSE;
    *prevHook = data->next;
    USER_HEAP_FREE( hook );
    return TRUE;
}


/***********************************************************************
 *           HOOK_CallHook
 *
 * Call a hook procedure.
 */
static DWORD HOOK_CallHook( HANDLE hook, short code,
                            WPARAM wParam, LPARAM lParam )
{
    HOOKDATA *data;
    MESSAGEQUEUE *queue;
    HANDLE prevHook;
    DWORD ret;

    /* Find the first hook with a valid proc */

    for (;;)
    {
        if (!hook) return 0;
        if (!(data = (HOOKDATA *)USER_HEAP_LIN_ADDR(hook))) return 0;
        if (data->proc) break;
        hook = data->next;
    }

    /* Now call it */

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetTaskQueue(0) ))) return 0;
    prevHook = queue->hCurHook;
    queue->hCurHook = hook;
    data->inHookProc = 1;

    dprintf_hook( stddeb, "Calling hook %04x: %d %04lx %08lx\n",
                  hook, code, (DWORD)wParam, lParam );
    ret = CallHookProc( data->proc, code, wParam, lParam );
    dprintf_hook( stddeb, "Ret hook %04x = %08lx\n", hook, ret );

    data->inHookProc = 0;
    queue->hCurHook = prevHook;
    if (!data->proc) HOOK_RemoveHook( hook );
    return ret;
}


/***********************************************************************
 *           HOOK_CallHooks
 *
 * Call a hook chain.
 */
DWORD HOOK_CallHooks( short id, short code, WPARAM wParam, LPARAM lParam )
{
    HANDLE hook = HOOK_GetHook( id , 0 );
    if (!hook) return 0;
    return HOOK_CallHook( hook, code, wParam, lParam );
}


/***********************************************************************
 *	     HOOK_FreeModuleHooks
 */
void HOOK_FreeModuleHooks( HMODULE hModule )
{
 /* remove all system hooks registered by this module */

  HOOKDATA*     hptr;
  HHOOK         hook, next;
  int           id;

  for( id = WH_FIRST_HOOK; id <= WH_LAST_HOOK; id++ )
    {
       hook = HOOK_systemHooks[id - WH_FIRST_HOOK];
       while( hook )
          if( (hptr = (HOOKDATA *)USER_HEAP_LIN_ADDR(hook)) )
	    {
	      next = hptr->next;
	      if( hptr->ownerModule == hModule )
                {
                  hptr->inHookProc = 0;
                  HOOK_RemoveHook(hook);
                }
	      hook = next;
	    }
	  else hook = 0;
    }
}

/***********************************************************************
 *	     HOOK_FreeQueueHooks
 */
void HOOK_FreeQueueHooks( HQUEUE hQueue )
{
  /* remove all hooks registered by this queue */

  HOOKDATA*	hptr = NULL;
  HHOOK 	hook, next;
  int 		id;

  for( id = WH_FIRST_HOOK; id <= WH_LAST_HOOK; id++ )
    {
       hook = HOOK_GetHook( id, hQueue );
       while( hook )
	{
	  next = HOOK_GetNextHook(hook);

	  hptr = (HOOKDATA *)USER_HEAP_LIN_ADDR(hook);
	  if( hptr && hptr->ownerQueue == hQueue )
	    {
	      hptr->inHookProc = 0;
	      HOOK_RemoveHook(hook);
	    }
	  hook = next;
	}
    }
}

/***********************************************************************
 *           SetWindowsHook   (USER.121)
 */
FARPROC SetWindowsHook( short id, HOOKPROC proc )
{
#ifdef WINELIB
    HINSTANCE hInst = 0;
#else
    HINSTANCE hInst = FarGetOwner( HIWORD(proc) );
#endif
    /* WH_MSGFILTER is the only task-specific hook for SetWindowsHook() */
    HTASK hTask = (id == WH_MSGFILTER) ? GetCurrentTask() : 0;

    HANDLE handle = HOOK_SetHook( id, proc, hInst, hTask );
    if (!handle) return (FARPROC)-1;
    if (!((HOOKDATA *)USER_HEAP_LIN_ADDR( handle ))->next) return 0;
    /* Not sure if the return value is correct; should not matter much
     * since it's never used (see DefHookProc). -- AJ */
#ifdef WINELIB32
    return (FARPROC)handle;
#else
    return (FARPROC)MAKELONG( handle, HOOK_MAGIC );
#endif
}


/***********************************************************************
 *           UnhookWindowsHook   (USER.234)
 */
BOOL UnhookWindowsHook( short id, HOOKPROC proc )
{
    HANDLE hook = HOOK_GetHook( id , 0 );

    dprintf_hook( stddeb, "UnhookWindowsHook: %d %08lx\n", id, (DWORD)proc );

    while (hook)
    {
        HOOKDATA *data = (HOOKDATA *)USER_HEAP_LIN_ADDR(hook);
        if (data->proc == proc) break;
        hook = HOOK_GetNextHook( hook );
    }
    if (!hook) return FALSE;
    return HOOK_RemoveHook( hook );
}


/***********************************************************************
 *           DefHookProc   (USER.235)
 */
DWORD DefHookProc( short code, WORD wParam, DWORD lParam, HHOOK *hhook )
{
    /* Note: the *hhook parameter is never used, since we rely on the
     * current hook value from the task queue to find the next hook. */
    MESSAGEQUEUE *queue;
    HANDLE next;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetTaskQueue(0) ))) return 0;
    if (!(next = HOOK_GetNextHook( queue->hCurHook ))) return 0;
    return HOOK_CallHook( next, code, wParam, lParam );
}


/***********************************************************************
 *           CallMsgFilter   (USER.123)
 */
BOOL CallMsgFilter( SEGPTR msg, INT code )
{
    if (GetSysModalWindow()) return FALSE;
    if (HOOK_CallHooks( WH_SYSMSGFILTER, code, 0, (LPARAM)msg )) return TRUE;
    return HOOK_CallHooks( WH_MSGFILTER, code, 0, (LPARAM)msg );
}


/***********************************************************************
 *           SetWindowsHookEx   (USER.291)
 */
HHOOK SetWindowsHookEx( short id, HOOKPROC proc, HINSTANCE hInst, HTASK hTask )
{
    HANDLE handle = HOOK_SetHook( id, proc, hInst, hTask );
#ifdef WINELIB32
    return (HHOOK)handle;
#else
    return MAKELONG( handle, HOOK_MAGIC );
#endif
}


/***********************************************************************
 *           UnhookWindowHookEx   (USER.292)
 */
BOOL UnhookWindowsHookEx( HHOOK hhook )
{
#ifdef WINELIB32
    return HOOK_RemoveHook( (HANDLE)hhook );
#else
    if (HIWORD(hhook) != HOOK_MAGIC) return FALSE;  /* Not a new format hook */
    return HOOK_RemoveHook( LOWORD(hhook) );
#endif
}


/***********************************************************************
 *           CallNextHookEx   (USER.293)
 */
LRESULT CallNextHookEx( HHOOK hhook, INT code, WPARAM wParam, LPARAM lParam )
{
    HANDLE next;
#ifdef WINELIB32
    if (!(next = HOOK_GetNextHook( (HANDLE)hhook ))) return 0;
#else
    if (HIWORD(hhook) != HOOK_MAGIC) return 0;  /* Not a new format hook */
    if (!(next = HOOK_GetNextHook( LOWORD(hhook) ))) return 0;
#endif
    return HOOK_CallHook( next, code, wParam, lParam );
}
