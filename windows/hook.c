/*
 * Windows hook functions
 *
 * Copyright 1994 Alexandre Julliard
 */

/* Warning!
 * HHOOK is not a real handle, but a 32-bit pointer to a HOOKDATA structure.
 * This is for compatibility with Windows 3.0 where HHOOK was a HOOKPROC.
 */

#include "hook.h"


HHOOK systemHooks[LAST_HOOK-FIRST_HOOK+1] = { 0, };

  /* Task-specific hooks should probably be in the task structure */
HHOOK taskHooks[LAST_HOOK-FIRST_HOOK+1] = { 0, };



/***********************************************************************
 *           SetWindowsHook   (USER.121)
 */
HHOOK SetWindowsHook( short id, HOOKPROC proc )
{
    return SetWindowsHookEx( id, proc, 0, 0 );
}


/***********************************************************************
 *           UnhookWindowsHook   (USER.234)
 */
BOOL UnhookWindowsHook( short id, HHOOK hhook )
{
    return UnhookWindowsHookEx( hhook );
}


/***********************************************************************
 *           DefHookProc   (USER.235)
 */
DWORD DefHookProc( short code, WORD wParam, DWORD lParam, HHOOK *hhook )
{
    return CallNextHookEx( *hhook, code, wParam, lParam );
}


/***********************************************************************
 *           CallMsgFilter   (USER.123)
 */
BOOL CallMsgFilter( LPMSG msg, short code )
{
    if (CALL_SYSTEM_HOOK( WH_SYSMSGFILTER, code, 0, (LPARAM)msg )) return TRUE;
    else return CALL_TASK_HOOK( WH_MSGFILTER, code, 0, (LPARAM)msg );
}


/***********************************************************************
 *           SetWindowsHookEx   (USER.291)
 */
HHOOK SetWindowsHookEx( short id, HOOKPROC proc, HINSTANCE hinst, HTASK htask )
{
    HOOKDATA *data;
    HANDLE handle;
    HHOOK *prevHook;

    if ((id < FIRST_HOOK) || (id > LAST_HOOK)) return 0;
    if (htask)  /* Task-specific hook */
    {
	if ((id == WH_JOURNALRECORD) || (id == WH_JOURNALPLAYBACK) ||
	    (id == WH_SYSMSGFILTER)) return 0;
	prevHook = &TASK_HOOK( id );
    }
    else  /* System-wide hook */
    {
	prevHook = &SYSTEM_HOOK( id );
    }
    
    handle = (HANDLE) USER_HEAP_ALLOC( GMEM_MOVEABLE, sizeof(*data) );
    if (!handle) return 0;
    data   = (HOOKDATA *) USER_HEAP_ADDR( handle );

    data->next  = *prevHook;
    data->proc  = proc;
    data->id    = id;
    data->htask = htask;
    *prevHook   = (HHOOK)data;
    return (HHOOK)data;
}


/***********************************************************************
 *           UnhookWindowHookEx   (USER.292)
 */
BOOL UnhookWindowsHookEx( HHOOK hhook )
{
    HOOKDATA *data = (HOOKDATA *)hhook;
    HHOOK *prevHook;

    if (!data) return FALSE;
    prevHook = data->htask ? &TASK_HOOK(data->id) : &SYSTEM_HOOK(data->id);
    while (*prevHook && (*prevHook != hhook))
	prevHook = &((HOOKDATA *)*prevHook)->next;
    if (!*prevHook) return FALSE;
    *prevHook = data->next;
    USER_HEAP_FREE( hhook & 0xffff );
    return TRUE;
}


/***********************************************************************
 *           CallNextHookEx   (USER.293)
 */
DWORD CallNextHookEx( HHOOK hhook, short code, WPARAM wParam, LPARAM lParam )
{
    HOOKDATA *data = (HOOKDATA *)hhook;
    if (!data->next) return 0;
    else return INTERNAL_CALL_HOOK( data->next, code, wParam, lParam );
}
