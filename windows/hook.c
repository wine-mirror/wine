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
#include "user.h"
#include "stddebug.h"
#include "debug.h"

HHOOK systemHooks[LAST_HOOK-FIRST_HOOK+1] = { 0, };

  /* Task-specific hooks should probably be in the task structure */
HHOOK taskHooks[LAST_HOOK-FIRST_HOOK+1] = { 0, };



/***********************************************************************
 *           SetWindowsHook   (USER.121)
 */
FARPROC SetWindowsHook( short id, HOOKPROC proc )
{
  HHOOK hhook = SetWindowsHookEx( id, proc, 0, 0 );
  HOOKDATA *data = PTR_SEG_TO_LIN(hhook);
  if (data != NULL)  {
    data = PTR_SEG_TO_LIN(data->next);
    if (data != NULL)  {
      return data->proc;
    }
  }
  return 0;
}


/***********************************************************************
 *           UnhookWindowsHook   (USER.234)
 */
BOOL UnhookWindowsHook( short id, FARPROC hproc )
{
  HHOOK *prevHook,hhook;
  
  prevHook = &TASK_HOOK(id);
  while (*prevHook)  {
    HOOKDATA *data = (HOOKDATA *)PTR_SEG_TO_LIN(*prevHook);

    if (data->proc == hproc) {
      hhook = *prevHook;
      *prevHook = data->next;
#ifdef WINELIB32
      USER_HEAP_FREE((HANDLE)hhook);
#else
      USER_HEAP_FREE(LOWORD(hhook));
#endif
      return TRUE;
    }
  }
  prevHook = &SYSTEM_HOOK(id);
  while (*prevHook) {
    HOOKDATA *data = (HOOKDATA *)PTR_SEG_TO_LIN(*prevHook);
    if (data->proc == hproc) {
      hhook = *prevHook;
      *prevHook = data->next;
#ifdef WINELIB32
      USER_HEAP_FREE((HANDLE)hhook);
#else
      USER_HEAP_FREE(LOWORD(hhook));
#endif
      return TRUE;
    }
  }
  return FALSE;
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
BOOL CallMsgFilter( SEGPTR msg, short code )
{
    if (CALL_TASK_HOOK( WH_MSGFILTER, code, 0, (LPARAM)msg )) 
	return TRUE;
    return CALL_SYSTEM_HOOK( WH_SYSMSGFILTER, code, 0, (LPARAM)msg );
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
    if (id != WH_GETMESSAGE && id != WH_CALLWNDPROC) {
	fprintf( stdnimp, "Unimplemented hook set: (%d,%08lx,"NPFMT","NPFMT")!\n",
                 id, (DWORD)proc, hinst, htask );
    }
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
    
    handle = (HANDLE) USER_HEAP_ALLOC( sizeof(*data) );
    if (!handle) return 0;
    data   = (HOOKDATA *) USER_HEAP_LIN_ADDR( handle );

    data->next  = *prevHook;
    data->proc  = proc;
    data->id    = id;
    data->htask = htask;
    *prevHook   = (HHOOK)USER_HEAP_SEG_ADDR(handle);
    return *prevHook;
}


/***********************************************************************
 *           UnhookWindowHookEx   (USER.292)
 */
BOOL UnhookWindowsHookEx( HHOOK hhook )
{
    HOOKDATA *data = (HOOKDATA *)PTR_SEG_TO_LIN(hhook);
    HHOOK *prevHook;

    if (!data) return FALSE;
    prevHook = data->htask ? &TASK_HOOK(data->id) : &SYSTEM_HOOK(data->id);
    while (*prevHook && (*prevHook != hhook)) {    
	prevHook = &((HOOKDATA *)*prevHook)->next;
    }
    if (!*prevHook) return FALSE;
    *prevHook = data->next;
#ifdef WINELIB32
    USER_HEAP_FREE( (HANDLE)hhook );
#else
    USER_HEAP_FREE( hhook & 0xffff );
#endif
    return TRUE;
}


/***********************************************************************
 *           CallNextHookEx   (USER.293)
 */
DWORD CallNextHookEx( HHOOK hhook, short code, WPARAM wParam, LPARAM lParam )
{
    HOOKDATA *data = (HOOKDATA *)PTR_SEG_TO_LIN(hhook);
    if (data == NULL || !data->next) return 0;
    else return INTERNAL_CALL_HOOK( data->next, code, wParam, lParam );
}
