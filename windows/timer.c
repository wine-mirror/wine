/*
 * Timer functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include "windows.h"
#include "queue.h"
#include "stddebug.h"
/* #define DEBUG_TIMER */
#include "debug.h"


typedef struct tagTIMER
{
    HWND             hwnd;
    HQUEUE	     hq;
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

  /* Duration from 'time' until expiration of the timer */
#define EXPIRE_TIME(pTimer,time) \
          (((pTimer)->expires <= (time)) ? 0 : (pTimer)->expires - (time))


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
	pTimer->next = ptr->next;
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
    TIMER **ppTimer = &pNextTimer;

    while (*ppTimer && (*ppTimer != pTimer)) ppTimer = &(*ppTimer)->next;
    if (*ppTimer) *ppTimer = pTimer->next;
    pTimer->next = NULL;
}


/***********************************************************************
 *           TIMER_ClearTimer
 *
 * Clear and remove a timer.
 */
static void TIMER_ClearTimer( TIMER * pTimer )
{
    TIMER_RemoveTimer( pTimer );
    QUEUE_DecTimerCount( pTimer->hq );
    pTimer->hwnd    = 0;
    pTimer->msg     = 0;
    pTimer->id      = 0;
    pTimer->timeout = 0;
    pTimer->proc    = 0;
}


/***********************************************************************
 *           TIMER_SwitchQueue
 */
void TIMER_SwitchQueue(HQUEUE old, HQUEUE new)
{
    TIMER * pT = pNextTimer;

    while (pT)
    {
        if (pT->hq == old) pT->hq = new;
        pT = pT->next;
    }
}


/***********************************************************************
 *           TIMER_RemoveWindowTimers
 *
 * Remove all timers for a given window.
 */
void TIMER_RemoveWindowTimers( HWND hwnd )
{
    int i;
    TIMER *pTimer;

    for (i = NB_TIMERS, pTimer = TimersArray; i > 0; i--, pTimer++)
	if ((pTimer->hwnd == hwnd) && pTimer->timeout)
            TIMER_ClearTimer( pTimer );
}


/***********************************************************************
 *           TIMER_RemoveQueueTimers
 *
 * Remove all timers for a given queue.
 */
void TIMER_RemoveQueueTimers( HQUEUE hqueue )
{
    int i;
    TIMER *pTimer;

    for (i = NB_TIMERS, pTimer = TimersArray; i > 0; i--, pTimer++)
	if ((pTimer->hq == hqueue) && pTimer->timeout)
            TIMER_ClearTimer( pTimer );
}


/***********************************************************************
 *           TIMER_RestartTimers
 *
 * Restart an expired timer.
 */
static void TIMER_RestartTimer( TIMER * pTimer, DWORD curTime )
{
    TIMER_RemoveTimer( pTimer );
    pTimer->expires = curTime + pTimer->timeout;
    TIMER_InsertTimer( pTimer );
}

			       
/***********************************************************************
 *           TIMER_GetNextExp
 *
 * Return next timer expiration time, or -1 if none.
 */
LONG TIMER_GetNextExp(void)
{
    return pNextTimer ? EXPIRE_TIME( pNextTimer, GetTickCount() ) : -1;
}


/***********************************************************************
 *           TIMER_CheckTimer
 *
 * Check whether a timer has expired, and create a message if necessary.
 * Otherwise, return time until next timer expiration in 'next'.
 * If 'hwnd' is not NULL, only consider timers for this window.
 * If 'remove' is TRUE, remove all expired timers up to the returned one.
 */
BOOL TIMER_CheckTimer( LONG *next, MSG16 *msg, HWND hwnd, BOOL remove )
{
    TIMER * pTimer = pNextTimer;
    DWORD curTime = GetTickCount();

    if (hwnd)  /* Find first timer for this window */
	while (pTimer && (pTimer->hwnd != hwnd)) pTimer = pTimer->next;

    if (!pTimer) *next = -1;
    else *next = EXPIRE_TIME( pTimer, curTime );
    if (*next != 0) return FALSE;  /* No timer expired */

    if (remove)	/* Restart all timers before pTimer, and then pTimer itself */
    {
	while (pNextTimer != pTimer) TIMER_RestartTimer( pNextTimer, curTime );
	TIMER_RestartTimer( pTimer, curTime );
    }

    dprintf_timer(stddeb, "Timer expired: %p, %04x, %04x, %04x, %08lx\n", 
		  pTimer, pTimer->hwnd, pTimer->msg, pTimer->id, (DWORD)pTimer->proc);
      /* Build the message */
    msg->hwnd    = pTimer->hwnd;
    msg->message = pTimer->msg;
    msg->wParam  = pTimer->id;
    msg->lParam  = (LONG)pTimer->proc;
    msg->time    = curTime;
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
/*    if (!hwnd && !proc) return 0; */

      /* Check if there's already a timer with the same hwnd and id */

    for (i = 0, pTimer = TimersArray; i < NB_TIMERS; i++, pTimer++)
        if ((pTimer->hwnd == hwnd) && (pTimer->id == id) &&
            (pTimer->timeout != 0))
        {
              /* Got one: set new values and return */
            pTimer->timeout = timeout;
            pTimer->expires = GetTickCount() + timeout;
            pTimer->proc    = proc;
            TIMER_RemoveTimer( pTimer );
            TIMER_InsertTimer( pTimer );
            return id;
        }

      /* Find a free timer */
    
    for (i = 0, pTimer = TimersArray; i < NB_TIMERS; i++, pTimer++)
	if (!pTimer->timeout) break;

    if (i >= NB_TIMERS) return 0;
    if (!sys && (i >= NB_TIMERS-NB_RESERVED_TIMERS)) return 0;
    if (!hwnd) id = i + 1;
    
      /* Add the timer */

    pTimer->hwnd    = hwnd;
    pTimer->hq	    = (hwnd) ? GetTaskQueue( GetWindowTask( hwnd ) )
			     : GetTaskQueue( 0 );
    pTimer->msg     = sys ? WM_SYSTIMER : WM_TIMER;
    pTimer->id      = id;
    pTimer->timeout = timeout;
    pTimer->expires = GetTickCount() + timeout;
    pTimer->proc    = proc;
    dprintf_timer(stddeb, "Timer added: %p, %04x, %04x, %04x, %08lx\n", 
		  pTimer, pTimer->hwnd, pTimer->msg, pTimer->id, (DWORD)pTimer->proc);
    TIMER_InsertTimer( pTimer );
    QUEUE_IncTimerCount( pTimer->hq );
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

    TIMER_ClearTimer( pTimer );
    return TRUE;
}


/***********************************************************************
 *           SetTimer   (USER.10)
 */
WORD SetTimer( HWND hwnd, WORD id, WORD timeout, FARPROC proc )
{
    dprintf_timer(stddeb, "SetTimer: %04x %d %d %08lx\n", hwnd, id, timeout, (LONG)proc );
    return TIMER_SetTimer( hwnd, id, timeout, proc, FALSE );
}


/***********************************************************************
 *           SetSystemTimer   (USER.11)
 */
WORD SetSystemTimer( HWND hwnd, WORD id, WORD timeout, FARPROC proc )
{
    dprintf_timer(stddeb, "SetSystemTimer: %04x %d %d %08lx\n", 
		  hwnd, id, timeout, (LONG)proc );
    return TIMER_SetTimer( hwnd, id, timeout, proc, TRUE );
}


/***********************************************************************
 *           KillTimer   (USER.12)
 */
BOOL KillTimer( HWND hwnd, WORD id )
{
    dprintf_timer(stddeb, "KillTimer: %04x %d\n", hwnd, id );
    return TIMER_KillTimer( hwnd, id, FALSE );
}


/***********************************************************************
 *           KillSystemTimer   (USER.182)
 */
BOOL KillSystemTimer( HWND hwnd, WORD id )
{
    dprintf_timer(stddeb, "KillSystemTimer: %04x %d\n", hwnd, id );
    return TIMER_KillTimer( hwnd, id, TRUE );
}
