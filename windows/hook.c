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
 * handle in the low 16 bits and of a HOOK_MAGIC value in the high 16 bits.
 */

#define NO_TRANSITION_TYPES  /* This file is Win32-clean */
#include "hook.h"
#include "queue.h"
#include "user.h"
#include "stddebug.h"
#include "debug.h"

#pragma pack(1)

  /* Hook data (pointed to by a HHOOK) */
typedef struct
{
    HANDLE16   next;               /* 00 Next hook in chain */
    HOOKPROC16 proc WINE_PACKED;   /* 02 Hook procedure */
    INT16      id;                 /* 06 Hook id (WH_xxx) */
    HQUEUE16   ownerQueue;         /* 08 Owner queue (0 for system hook) */
    HMODULE16  ownerModule;        /* 0a Owner module */
    WORD       inHookProc;         /* 0c TRUE if in this->proc */
} HOOKDATA;

#pragma pack(4)

#define HOOK_MAGIC  ((int)'H' | (int)'K' << 8)  /* 'HK' */

  /* This should probably reside in USER heap */
static HANDLE16 HOOK_systemHooks[WH_NB_HOOKS] = { 0, };


/***********************************************************************
 *           HOOK_GetNextHook
 *
 * Get the next hook of a given hook.
 */
static HANDLE16 HOOK_GetNextHook( HANDLE16 hook )
{
    HOOKDATA *data = (HOOKDATA *)USER_HEAP_LIN_ADDR( hook );
    if (!data || !hook) return 0;
    if (data->next) return data->next;
    if (!data->ownerQueue) return 0;  /* Already system hook */
    /* Now start enumerating the system hooks */
    return HOOK_systemHooks[data->id - WH_MINHOOK];
}


/***********************************************************************
 *           HOOK_GetHook
 *
 * Get the first hook for a given type.
 */
HANDLE16 HOOK_GetHook( INT16 id , HQUEUE16 hQueue )
{
    MESSAGEQUEUE *queue;
    HANDLE16 hook = 0;

    if ((queue = (MESSAGEQUEUE *)GlobalLock16( hQueue )) != NULL)
        hook = queue->hooks[id - WH_MINHOOK];
    if (!hook) hook = HOOK_systemHooks[id - WH_MINHOOK];
    return hook;
}


/***********************************************************************
 *           HOOK_GetProc16
 */
HOOKPROC16 HOOK_GetProc16( HHOOK hhook )
{
    HOOKDATA *data;
    if (HIWORD(hhook) != HOOK_MAGIC) return NULL;
    if (!(data = (HOOKDATA *)USER_HEAP_LIN_ADDR( LOWORD(hhook) ))) return NULL;
    return data->proc;
}


/***********************************************************************
 *           HOOK_SetHook
 *
 * Install a given hook.
 */
static HANDLE16 HOOK_SetHook( INT16 id, HOOKPROC16 proc, HINSTANCE16 hInst,
                              HTASK16 hTask )
{
    HOOKDATA *data;
    HANDLE16 handle;
    HQUEUE16 hQueue = 0;

    if ((id < WH_MINHOOK) || (id > WH_MAXHOOK)) return 0;
    if (!(hInst = GetExePtr( hInst ))) return 0;

    dprintf_hook( stddeb, "Setting hook %d: %08x %04x %04x\n",
                  id, (UINT32)proc, hInst, hTask );

    if (hTask)  /* Task-specific hook */
    {
	if ((id == WH_JOURNALRECORD) || (id == WH_JOURNALPLAYBACK) ||
	    (id == WH_SYSMSGFILTER)) return 0;  /* System-only hooks */
        if (!(hQueue = GetTaskQueue( hTask ))) return 0;
    }

    if (id == WH_DEBUG)
    {
        fprintf( stdnimp,"WH_DEBUG is broken in 16-bit Windows.\n");
        return 0;
    }
    else if (id == WH_CBT || id == WH_SHELL)
    {
	fprintf( stdnimp, "Half-implemented hook set: (%s,%08lx,%04x,%04x)!\n",
                 (id==WH_CBT)?"WH_CBT":"WH_SHELL", (DWORD)proc, hInst, hTask );
    }

    if (id == WH_JOURNALPLAYBACK) EnableHardwareInput(FALSE);
     
    /* Create the hook structure */

    if (!(handle = USER_HEAP_ALLOC( sizeof(HOOKDATA) ))) return 0;
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
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock16( hQueue );
        data->next = queue->hooks[id - WH_MINHOOK];
        queue->hooks[id - WH_MINHOOK] = handle;
    }
    else
    {
        data->next = HOOK_systemHooks[id - WH_MINHOOK];
        HOOK_systemHooks[id - WH_MINHOOK] = handle;
    }
    return handle;
}


/***********************************************************************
 *           HOOK_RemoveHook
 *
 * Remove a hook from the list.
 */
static BOOL32 HOOK_RemoveHook( HANDLE16 hook )
{
    HOOKDATA *data;
    HANDLE16 *prevHook;

    dprintf_hook( stddeb, "Removing hook %04x\n", hook );

    if (!(data = (HOOKDATA *)USER_HEAP_LIN_ADDR(hook))) return FALSE;
    if (data->inHookProc)
    {
        /* Mark it for deletion later on */
        dprintf_hook( stddeb, "Hook still running, deletion delayed\n" );
        data->proc = (HOOKPROC16)0;
        return TRUE;
    }

    if (data->id == WH_JOURNALPLAYBACK) EnableHardwareInput(TRUE);
     
    /* Remove it from the linked list */

    if (data->ownerQueue)
    {
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock16( data->ownerQueue );
        if (!queue) return FALSE;
        prevHook = &queue->hooks[data->id - WH_MINHOOK];
    }
    else prevHook = &HOOK_systemHooks[data->id - WH_MINHOOK];

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
static LRESULT HOOK_CallHook( HANDLE16 hook, INT16 code,
                              WPARAM16 wParam, LPARAM lParam )
{
    HOOKDATA *data;
    MESSAGEQUEUE *queue;
    HANDLE16 prevHook;
    LRESULT ret;

    /* Find the first hook with a valid proc */

    for (;;)
    {
        if (!hook) return 0;
        if (!(data = (HOOKDATA *)USER_HEAP_LIN_ADDR(hook))) return 0;
        if (data->proc) break;
        hook = data->next;
    }

    /* Now call it */

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return 0;
    prevHook = queue->hCurHook;
    queue->hCurHook = hook;
    data->inHookProc = 1;

    dprintf_hook( stddeb, "Calling hook %04x: proc=%p %d %04lx %08lx\n",
                  hook, data->proc, code, (DWORD)wParam, lParam );
    ret = data->proc( code, wParam, lParam );
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
LRESULT HOOK_CallHooks( INT16 id, INT16 code, WPARAM16 wParam, LPARAM lParam )
{
    HANDLE16 hook = HOOK_GetHook( id , GetTaskQueue(0) );
    if (!hook) return 0;
    return HOOK_CallHook( hook, code, wParam, lParam );
}


/***********************************************************************
 *	     HOOK_FreeModuleHooks
 */
void HOOK_FreeModuleHooks( HMODULE16 hModule )
{
 /* remove all system hooks registered by this module */

  HOOKDATA*     hptr;
  HHOOK         hook, next;
  int           id;

  for( id = WH_MINHOOK; id <= WH_MAXHOOK; id++ )
    {
       hook = HOOK_systemHooks[id - WH_MINHOOK];
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
void HOOK_FreeQueueHooks( HQUEUE16 hQueue )
{
  /* remove all hooks registered by this queue */

  HOOKDATA*	hptr = NULL;
  HHOOK 	hook, next;
  int 		id;

  for( id = WH_MINHOOK; id <= WH_MAXHOOK; id++ )
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
 *           SetWindowsHook16   (USER.121)
 */
FARPROC16 SetWindowsHook16( INT16 id, HOOKPROC16 proc )
{
    HTASK16 	hTask = (id == WH_MSGFILTER) ? GetCurrentTask() : 0;
    HANDLE16 	handle = HOOK_SetHook( id, proc, 0, hTask );

    return (handle) ? (FARPROC16)MAKELONG( handle, HOOK_MAGIC ) : NULL;
}


/***********************************************************************
 *           UnhookWindowsHook16   (USER.234)
 */
BOOL16 UnhookWindowsHook16( INT16 id, HOOKPROC16 proc )
{
    HANDLE16 hook = HOOK_GetHook( id, GetTaskQueue(0) );

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
LRESULT DefHookProc( INT16 code, WPARAM16 wParam, LPARAM lParam, HHOOK *hhook )
{
    /* Note: the *hhook parameter is never used, since we rely on the
     * current hook value from the task queue to find the next hook. */
    MESSAGEQUEUE *queue;
    HANDLE16 next;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return 0;
    if (!(next = HOOK_GetNextHook( queue->hCurHook ))) return 0;
    return HOOK_CallHook( next, code, wParam, lParam );
}


/***********************************************************************
 *           CallMsgFilter   (USER.123)
 */
BOOL16 CallMsgFilter( SEGPTR msg, INT16 code )
{
    if (GetSysModalWindow16()) return FALSE;
    if (HOOK_CallHooks( WH_SYSMSGFILTER, code, 0, (LPARAM)msg )) return TRUE;
    return HOOK_CallHooks( WH_MSGFILTER, code, 0, (LPARAM)msg );
}


/***********************************************************************
 *           SetWindowsHookEx16   (USER.291)
 */
HHOOK SetWindowsHookEx16( INT16 id, HOOKPROC16 proc, HINSTANCE16 hInst,
                          HTASK16 hTask )
{
    HANDLE16 handle = HOOK_SetHook( id, proc, hInst, hTask );
    return (handle) ? MAKELONG( handle, HOOK_MAGIC ) : NULL;
}


/***********************************************************************
 *           UnhookWindowsHookEx16   (USER.292)
 */
BOOL16 UnhookWindowsHookEx16( HHOOK hhook )
{
    if (HIWORD(hhook) != HOOK_MAGIC) return FALSE;  /* Not a new format hook */
    return HOOK_RemoveHook( LOWORD(hhook) );
}


/***********************************************************************
 *           CallNextHookEx   (USER.293)
 */
LRESULT CallNextHookEx(HHOOK hhook, INT16 code, WPARAM16 wParam, LPARAM lParam)
{
    HANDLE16 next;
    if (HIWORD(hhook) != HOOK_MAGIC) return 0;  /* Not a new format hook */
    if (!(next = HOOK_GetNextHook( LOWORD(hhook) ))) return 0;
    return HOOK_CallHook( next, code, wParam, lParam );
}
