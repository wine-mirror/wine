/*
 * Timer functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <X11/Intrinsic.h>

#include "windows.h"

extern XtAppContext XT_app_context;


typedef struct
{
    HWND          hwnd;
    WORD          msg;  /* WM_TIMER or WM_SYSTIMER */
    WORD          id;
    WORD          timeout;
    FARPROC       proc;
    XtIntervalId  xtid;
} TIMER;

#define NB_TIMERS            34
#define NB_RESERVED_TIMERS    2  /* for SetSystemTimer */

static TIMER TimersArray[NB_TIMERS];


/***********************************************************************
 *           TIMER_callback
 */
static void TIMER_callback( XtPointer data, XtIntervalId * xtid )
{
    TIMER * pTimer = (TIMER *) data;
    
    pTimer->xtid = 0;  /* In case the timer procedure calls KillTimer */
    
    if (pTimer->proc)
    {
	CallWindowProc(pTimer->proc, pTimer->hwnd, pTimer->msg, 
		       pTimer->id, GetTickCount());
    }
    else 
	PostMessage( pTimer->hwnd, pTimer->msg, pTimer->id, 0 );

      /* If timeout == 0, the timer has been removed by KillTimer */
    if (pTimer->timeout)
	pTimer->xtid = XtAppAddTimeOut( XT_app_context, pTimer->timeout,
				        TIMER_callback, pTimer );
}


/***********************************************************************
 *           TIMER_SetTimer
 */
WORD TIMER_SetTimer( HWND hwnd, WORD id, WORD timeout, FARPROC proc, BOOL sys )
{
    int i;
    TIMER * pTimer;
    
    if (!timeout) return 0;
    if (!hwnd && !proc) return 0;
    
      /* Find a free timer */
    
    for (i = 0, pTimer = TimersArray; i < NB_TIMERS; i++, pTimer++)
	if (!pTimer->timeout) break;

    if (i >= NB_TIMERS) return 0;
    if (!sys && (i >= NB_TIMERS-NB_RESERVED_TIMERS)) return 0;
    if (!hwnd) id = i + 1;
    
      /* Add the timer */

    pTimer->hwnd    = hwnd;
    pTimer->msg     = sys ? WM_SYSTIMER : WM_TIMER;
    pTimer->id      = id;
    pTimer->timeout = timeout;
    pTimer->proc    = proc;
    pTimer->xtid    = XtAppAddTimeOut( XT_app_context, timeout,
				       TIMER_callback, pTimer );
    return id;
}


/***********************************************************************
 *           TIMER_KillTimer
 */
BOOL TIMER_KillTimer( HWND hwnd, WORD id, BOOL sys )
{
    int i;
    TIMER * pTimer;
    
      /* Find the timer */
    
    for (i = 0, pTimer = TimersArray; i < NB_TIMERS; i++, pTimer++)
	if ((pTimer->hwnd == hwnd) && (pTimer->id == id) &&
	    (pTimer->timeout != 0)) break;
    if (i >= NB_TIMERS) return FALSE;
    if (!sys && (i >= NB_TIMERS-NB_RESERVED_TIMERS)) return FALSE;
    if (!sys && (pTimer->msg != WM_TIMER)) return FALSE;
    else if (sys && (pTimer->msg != WM_SYSTIMER)) return FALSE;    

      /* Delete the timer */

    if (pTimer->xtid) XtRemoveTimeOut( pTimer->xtid );
    pTimer->hwnd    = 0;
    pTimer->msg     = 0;
    pTimer->id      = 0;
    pTimer->timeout = 0;
    pTimer->proc    = 0;
    pTimer->xtid    = 0;
    return TRUE;
}


/***********************************************************************
 *           SetTimer   (USER.10)
 */
WORD SetTimer( HWND hwnd, WORD id, WORD timeout, FARPROC proc )
{
#ifdef DEBUG_TIMER    
    printf( "SetTimer: %d %d %d %08x\n", hwnd, id, timeout, proc );
#endif
    return TIMER_SetTimer( hwnd, id, timeout, proc, FALSE );
}


/***********************************************************************
 *           SetSystemTimer   (USER.11)
 */
WORD SetSystemTimer( HWND hwnd, WORD id, WORD timeout, FARPROC proc )
{
#ifdef DEBUG_TIMER    
    printf( "SetSystemTimer: %d %d %d %08x\n", hwnd, id, timeout, proc );
#endif
    return TIMER_SetTimer( hwnd, id, timeout, proc, TRUE );
}


/***********************************************************************
 *           KillTimer   (USER.12)
 */
BOOL KillTimer( HWND hwnd, WORD id )
{
#ifdef DEBUG_TIMER
    printf( "KillTimer: %d %d\n", hwnd, id );
#endif
    return TIMER_KillTimer( hwnd, id, FALSE );
}


/***********************************************************************
 *           KillSystemTimer   (USER.182)
 */
BOOL KillSystemTimer( HWND hwnd, WORD id )
{
#ifdef DEBUG_TIMER
    printf( "KillSystemTimer: %d %d\n", hwnd, id );
#endif
    return TIMER_KillTimer( hwnd, id, TRUE );
}
