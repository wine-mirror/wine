/*
 * Timer functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "windows.h"
#include "message.h"


typedef struct tagTIMER
{
    HWND             hwnd;
    WORD             msg;  /* WM_TIMER or WM_SYSTIMER */
    WORD             id;
    WORD             timeout;
    struct tagTIMER *next;
    DWORD            expires;
    FARPROC          proc;
} TIMER;

#define NB_TIMERS            34
#define NB_RESERVED_TIMERS    2  /* for SetSystemTimer */

static TIMER TimersArray[NB_TIMERS];

static TIMER * pNextTimer = NULL;  /* Next timer to expire */


/***********************************************************************
 *           TIMER_InsertTimer
 *
 * Insert the timer at its place in the chain.
 */
static void TIMER_InsertTimer( TIMER * pTimer )
{
    if (!pNextTimer || (pTimer->expires < pNextTimer->expires))
    {
	pTimer->next = pNextTimer;
	pNextTimer = pTimer;
    }
    else
    {
        TIMER * ptr = pNextTimer;	
	while (ptr->next && (pTimer->expires >= ptr->next->expires))
	    ptr = ptr->next;
	pTimer->next = ptr;
	ptr->next = pTimer;
    }
}


/***********************************************************************
 *           TIMER_RemoveTimer
 *
 * Remove the timer from the chain.
 */
static void TIMER_RemoveTimer( TIMER * pTimer )
{
    if (pTimer == pNextTimer) pNextTimer = pTimer->next;
    else
    {
	TIMER * ptr = pNextTimer;
	while (ptr && (ptr->next != pTimer)) ptr = ptr->next;
	if (ptr) ptr->next = pTimer->next;
    }
    pTimer->next = NULL;
}


/***********************************************************************
 *           TIMER_NextExpire
 *
 * Return time until next timer expiration (-1 if none).
 */
static DWORD TIMER_NextExpire( DWORD curTime )
{
    if (!pNextTimer) return -1;
    if (pNextTimer->expires <= curTime) return 0;
    return pNextTimer->expires - curTime;
}


/***********************************************************************
 *           TIMER_CheckTimer
 *
 * Check whether a timer has expired, and post a message if necessary.
 * Return TRUE if msg posted, and return time until next expiration in 'next'.
 */
BOOL TIMER_CheckTimer( DWORD *next )
{
    TIMER * pTimer = pNextTimer;
    DWORD curTime = GetTickCount();
    
    if ((*next = TIMER_NextExpire( curTime )) != 0) return FALSE;

    PostMessage( pTimer->hwnd, pTimer->msg, pTimer->id, (LONG)pTimer->proc );
    TIMER_RemoveTimer( pTimer );

      /* If timeout == 0, the timer has been removed by KillTimer */
    if (pTimer->timeout)
    {
	  /* Restart the timer */
	pTimer->expires = curTime + pTimer->timeout;
	TIMER_InsertTimer( pTimer );
    }
    *next = TIMER_NextExpire( curTime );
    return TRUE;
}


/***********************************************************************
 *           TIMER_SetTimer
 */
static WORD TIMER_SetTimer( HWND hwnd, WORD id, WORD timeout,
			    FARPROC proc, BOOL sys )
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
    pTimer->expires = GetTickCount() + timeout;
    pTimer->proc    = proc;
    TIMER_InsertTimer( pTimer );
    MSG_IncTimerCount( GetTaskQueue(0) );
    if (!id)
	return TRUE;
    else
	return id;
}


/***********************************************************************
 *           TIMER_KillTimer
 */
static BOOL TIMER_KillTimer( HWND hwnd, WORD id, BOOL sys )
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

    pTimer->hwnd    = 0;
    pTimer->msg     = 0;
    pTimer->id      = 0;
    pTimer->timeout = 0;
    pTimer->proc    = 0;
    TIMER_RemoveTimer( pTimer );
    MSG_DecTimerCount( GetTaskQueue(0) );
    return TRUE;
}


/***********************************************************************
 *           SetTimer   (USER.10)
 */
WORD SetTimer( HWND hwnd, WORD id, WORD timeout, FARPROC proc )
{
#ifdef DEBUG_TIMER    
    printf( "SetTimer: %d %d %d %p\n", hwnd, id, timeout, proc );
#endif
    return TIMER_SetTimer( hwnd, id, timeout, proc, FALSE );
}


/***********************************************************************
 *           SetSystemTimer   (USER.11)
 */
WORD SetSystemTimer( HWND hwnd, WORD id, WORD timeout, FARPROC proc )
{
#ifdef DEBUG_TIMER    
    printf( "SetSystemTimer: %d %d %d %p\n", hwnd, id, timeout, proc );
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
