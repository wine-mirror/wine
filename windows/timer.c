/*
 * Timer functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include "winuser.h"
#include "queue.h"
#include "task.h"
#include "winproc.h"
#include "debug.h"


typedef struct tagTIMER
{
    HWND32           hwnd;
    HQUEUE16         hq;
    UINT16           msg;  /* WM_TIMER or WM_SYSTIMER */
    UINT32           id;
    UINT32           timeout;
    struct tagTIMER *next;
    DWORD            expires;  /* Next expiration, or 0 if already expired */
    HWINDOWPROC      proc;
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
    if (!pTimer->expires) QUEUE_DecTimerCount( pTimer->hq );
}


/***********************************************************************
 *           TIMER_ClearTimer
 *
 * Clear and remove a timer.
 */
static void TIMER_ClearTimer( TIMER * pTimer )
{
    TIMER_RemoveTimer( pTimer );
    pTimer->hwnd    = 0;
    pTimer->msg     = 0;
    pTimer->id      = 0;
    pTimer->timeout = 0;
    WINPROC_FreeProc( pTimer->proc, WIN_PROC_TIMER );
}


/***********************************************************************
 *           TIMER_SwitchQueue
 */
void TIMER_SwitchQueue( HQUEUE16 old, HQUEUE16 new )
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
void TIMER_RemoveWindowTimers( HWND32 hwnd )
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
void TIMER_RemoveQueueTimers( HQUEUE16 hqueue )
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
 *           TIMER_GetNextExpiration
 *
 * Return next timer expiration time, or -1 if none.
 */
LONG TIMER_GetNextExpiration(void)
{
    return pNextTimer ? EXPIRE_TIME( pNextTimer, GetTickCount() ) : -1;
}


/***********************************************************************
 *           TIMER_ExpireTimers
 *
 * Mark expired timers and wake the appropriate queues.
 */
void TIMER_ExpireTimers(void)
{
    TIMER *pTimer = pNextTimer;
    DWORD curTime = GetTickCount();

    while (pTimer && !pTimer->expires)  /* Skip already expired timers */
        pTimer = pTimer->next;
    while (pTimer && (pTimer->expires <= curTime))
    {
        pTimer->expires = 0;
        QUEUE_IncTimerCount( pTimer->hq );
        pTimer = pTimer->next;
    }
}


/***********************************************************************
 *           TIMER_GetTimerMsg
 *
 * Build a message for an expired timer.
 */
BOOL32 TIMER_GetTimerMsg( MSG32 *msg, HWND32 hwnd,
                          HQUEUE16 hQueue, BOOL32 remove )
{
    TIMER *pTimer = pNextTimer;
    DWORD curTime = GetTickCount();

    if (hwnd)  /* Find first timer for this window */
	while (pTimer && (pTimer->hwnd != hwnd)) pTimer = pTimer->next;
    else   /* Find first timer for this queue */
	while (pTimer && (pTimer->hq != hQueue)) pTimer = pTimer->next;

    if (!pTimer || (pTimer->expires > curTime)) return FALSE; /* No timer */
    if (remove)	TIMER_RestartTimer( pTimer, curTime );  /* Restart it */

    TRACE(timer, "Timer expired: %04x, %04x, %04x, %08lx\n", 
		   pTimer->hwnd, pTimer->msg, pTimer->id, (DWORD)pTimer->proc);

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
static UINT32 TIMER_SetTimer( HWND32 hwnd, UINT32 id, UINT32 timeout,
                              WNDPROC16 proc, WINDOWPROCTYPE type, BOOL32 sys )
{
    int i;
    TIMER * pTimer;

    if (!timeout) return 0;

      /* Check if there's already a timer with the same hwnd and id */

    for (i = 0, pTimer = TimersArray; i < NB_TIMERS; i++, pTimer++)
        if ((pTimer->hwnd == hwnd) && (pTimer->id == id) &&
            (pTimer->timeout != 0))
        {
              /* Got one: set new values and return */
            TIMER_RemoveTimer( pTimer );
            pTimer->timeout = timeout;
            WINPROC_FreeProc( pTimer->proc, WIN_PROC_TIMER );
            pTimer->proc = (HWINDOWPROC)0;
            if (proc) WINPROC_SetProc( &pTimer->proc, proc,
                                       type, WIN_PROC_TIMER );
            pTimer->expires = GetTickCount() + timeout;
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
    pTimer->hq	    = (hwnd) ? GetThreadQueue( GetWindowThreadProcessId( hwnd, NULL ) )
			     : GetFastQueue( );
    pTimer->msg     = sys ? WM_SYSTIMER : WM_TIMER;
    pTimer->id      = id;
    pTimer->timeout = timeout;
    pTimer->expires = GetTickCount() + timeout;
    pTimer->proc    = (HWINDOWPROC)0;
    if (proc) WINPROC_SetProc( &pTimer->proc, proc, type, WIN_PROC_TIMER );
    TRACE(timer, "Timer added: %p, %04x, %04x, %04x, %08lx\n", 
		   pTimer, pTimer->hwnd, pTimer->msg, pTimer->id,
                   (DWORD)pTimer->proc );
    TIMER_InsertTimer( pTimer );
    if (!id) return TRUE;
    else return id;
}


/***********************************************************************
 *           TIMER_KillTimer
 */
static BOOL32 TIMER_KillTimer( HWND32 hwnd, UINT32 id, BOOL32 sys )
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
 *           SetTimer16   (USER.10)
 */
UINT16 WINAPI SetTimer16( HWND16 hwnd, UINT16 id, UINT16 timeout,
                          TIMERPROC16 proc )
{
    TRACE(timer, "%04x %d %d %08lx\n",
                   hwnd, id, timeout, (LONG)proc );
    return TIMER_SetTimer( hwnd, id, timeout, (WNDPROC16)proc,
                           WIN_PROC_16, FALSE );
}


/***********************************************************************
 *           SetTimer32   (USER32.511)
 */
UINT32 WINAPI SetTimer32( HWND32 hwnd, UINT32 id, UINT32 timeout,
                          TIMERPROC32 proc )
{
    TRACE(timer, "%04x %d %d %08lx\n",
                   hwnd, id, timeout, (LONG)proc );
    return TIMER_SetTimer( hwnd, id, timeout, (WNDPROC16)proc,
                           WIN_PROC_32A, FALSE );
}


/***********************************************************************
 *           SetSystemTimer16   (USER.11)
 */
UINT16 WINAPI SetSystemTimer16( HWND16 hwnd, UINT16 id, UINT16 timeout,
                                TIMERPROC16 proc )
{
    TRACE(timer, "%04x %d %d %08lx\n", 
                   hwnd, id, timeout, (LONG)proc );
    return TIMER_SetTimer( hwnd, id, timeout, (WNDPROC16)proc,
                           WIN_PROC_16, TRUE );
}


/***********************************************************************
 *           SetSystemTimer32   (USER32.509)
 */
UINT32 WINAPI SetSystemTimer32( HWND32 hwnd, UINT32 id, UINT32 timeout,
                                TIMERPROC32 proc )
{
    TRACE(timer, "%04x %d %d %08lx\n", 
                   hwnd, id, timeout, (LONG)proc );
    return TIMER_SetTimer( hwnd, id, timeout, (WNDPROC16)proc,
                           WIN_PROC_32A, TRUE );
}


/***********************************************************************
 *           KillTimer16   (USER.12)
 */
BOOL16 WINAPI KillTimer16( HWND16 hwnd, UINT16 id )
{
    TRACE(timer, "%04x %d\n", hwnd, id );
    return TIMER_KillTimer( hwnd, id, FALSE );
}


/***********************************************************************
 *           KillTimer32   (USER32.354)
 */
BOOL32 WINAPI KillTimer32( HWND32 hwnd, UINT32 id )
{
    TRACE(timer, "%04x %d\n", hwnd, id );
    return TIMER_KillTimer( hwnd, id, FALSE );
}


/***********************************************************************
 *           KillSystemTimer16   (USER.182)
 */
BOOL16 WINAPI KillSystemTimer16( HWND16 hwnd, UINT16 id )
{
    TRACE(timer, "%04x %d\n", hwnd, id );
    return TIMER_KillTimer( hwnd, id, TRUE );
}


/***********************************************************************
 *           KillSystemTimer32   (USER32.353)
 */
BOOL32 WINAPI KillSystemTimer32( HWND32 hwnd, UINT32 id )
{
    TRACE(timer, "%04x %d\n", hwnd, id );
    return TIMER_KillTimer( hwnd, id, TRUE );
}
