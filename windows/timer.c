/*
 * Timer functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include "winuser.h"
#include "queue.h"
#include "task.h"
#include "winproc.h"
#include "services.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(timer)


typedef struct tagTIMER
{
    HWND           hwnd;
    HQUEUE16         hq;
    UINT16           msg;  /* WM_TIMER or WM_SYSTIMER */
    UINT           id;
    UINT           timeout;
    struct tagTIMER *next;
    DWORD            expires;  /* Next expiration, or 0 if already expired */
    HWINDOWPROC      proc;
} TIMER;

#define NB_TIMERS            34
#define NB_RESERVED_TIMERS    2  /* for SetSystemTimer */

static TIMER TimersArray[NB_TIMERS];

static TIMER * pNextTimer = NULL;  /* Next timer to expire */

static CRITICAL_SECTION csTimer;

  /* Duration from 'time' until expiration of the timer */
#define EXPIRE_TIME(pTimer,time) \
          (((pTimer)->expires <= (time)) ? 0 : (pTimer)->expires - (time))


/***********************************************************************
 *           TIMER_Init
 *
 * Initialize critical section for the timer.
 */
BOOL TIMER_Init( void )
{
    InitializeCriticalSection( &csTimer );
    MakeCriticalSectionGlobal( &csTimer );

    return TRUE;
}


/***********************************************************************
 *           TIMER_InsertTimer
 *
 * Insert the timer at its place in the chain.
 */
static void TIMER_InsertTimer( TIMER * pTimer )
{
    EnterCriticalSection( &csTimer );
    
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
    
    LeaveCriticalSection( &csTimer );
}


/***********************************************************************
 *           TIMER_RemoveTimer
 *
 * Remove the timer from the chain.
 */
static void TIMER_RemoveTimer( TIMER * pTimer )
{
    TIMER **ppTimer = &pNextTimer;

    EnterCriticalSection( &csTimer );
    
    while (*ppTimer && (*ppTimer != pTimer)) ppTimer = &(*ppTimer)->next;
    if (*ppTimer) *ppTimer = pTimer->next;
    pTimer->next = NULL;
    
    LeaveCriticalSection( &csTimer );
    
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
    TIMER * pT;

    EnterCriticalSection( &csTimer );

    pT = pNextTimer;
    while (pT)
    {
        if (pT->hq == old) pT->hq = new;
        pT = pT->next;
    }
    
    LeaveCriticalSection( &csTimer );
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

    EnterCriticalSection( &csTimer );
    
    for (i = NB_TIMERS, pTimer = TimersArray; i > 0; i--, pTimer++)
	if ((pTimer->hwnd == hwnd) && pTimer->timeout)
            TIMER_ClearTimer( pTimer );
    
    LeaveCriticalSection( &csTimer );
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

    EnterCriticalSection( &csTimer );
    
    for (i = NB_TIMERS, pTimer = TimersArray; i > 0; i--, pTimer++)
	if ((pTimer->hq == hqueue) && pTimer->timeout)
            TIMER_ClearTimer( pTimer );
    
    LeaveCriticalSection( &csTimer );
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
 *           TIMER_CheckTimers
 *
 * Mark expired timers and wake the appropriate queues.
 */
static void CALLBACK TIMER_CheckTimers( ULONG_PTR forceTimer )
{
    static HANDLE ServiceHandle  = INVALID_HANDLE_VALUE;
    static LONG   ServiceTimeout = 0;

    TIMER *pTimer;
    DWORD curTime = GetTickCount();

    EnterCriticalSection( &csTimer );

    TRACE(timer, "Called at %ld (%s)\n", curTime, forceTimer? "manual" : "auto" );
    
    pTimer = pNextTimer;
    
    while (pTimer && !pTimer->expires)  /* Skip already expired timers */
        pTimer = pTimer->next;
    while (pTimer && (pTimer->expires <= curTime))
    {
        pTimer->expires = 0;
        QUEUE_IncTimerCount( pTimer->hq );
        pTimer = pTimer->next;
    }

    /* Install service callback with appropriate timeout, so that
       we get called again once the next timer has expired */

    if (pTimer)
    {
        LONG timeout = pTimer->expires - curTime;

        if ( forceTimer || timeout != ServiceTimeout )
        {
            if ( ServiceHandle != INVALID_HANDLE_VALUE ) 
              SERVICE_Delete( ServiceHandle );

            ServiceHandle = SERVICE_AddTimer( timeout * 1000L, 
                                              TIMER_CheckTimers, FALSE );
            ServiceTimeout = timeout;

            TRACE(timer, "Installed service callback with timeout %ld\n", timeout );
        }
    }
    else
    {
        if ( ServiceHandle != INVALID_HANDLE_VALUE )
        {
            SERVICE_Delete( ServiceHandle );
            ServiceHandle = INVALID_HANDLE_VALUE;
            ServiceTimeout = 0;

            TRACE(timer, "Deleted service callback\n" );
        }
    }
    
    LeaveCriticalSection( &csTimer );
}


/***********************************************************************
 *           TIMER_GetTimerMsg
 *
 * Build a message for an expired timer.
 */
BOOL TIMER_GetTimerMsg( MSG *msg, HWND hwnd,
                          HQUEUE16 hQueue, BOOL remove )
{
    TIMER *pTimer;
    DWORD curTime = GetTickCount();

    EnterCriticalSection( &csTimer );

    pTimer = pNextTimer;
    
    if (hwnd)  /* Find first timer for this window */
	while (pTimer && (pTimer->hwnd != hwnd)) pTimer = pTimer->next;
    else   /* Find first timer for this queue */
	while (pTimer && (pTimer->hq != hQueue)) pTimer = pTimer->next;

    if (!pTimer || (pTimer->expires > curTime))
    {
        LeaveCriticalSection( &csTimer );
        return FALSE; /* No timer */
    }
    
    TRACE(timer, "Timer expired: %04x, %04x, %04x, %08lx\n", 
		   pTimer->hwnd, pTimer->msg, pTimer->id, (DWORD)pTimer->proc);

    if (remove)	
    {
        TIMER_RestartTimer( pTimer, curTime );  /* Restart it */
        TIMER_CheckTimers( TRUE );
    }

      /* Build the message */
    msg->hwnd    = pTimer->hwnd;
    msg->message = pTimer->msg;
    msg->wParam  = pTimer->id;
    msg->lParam  = (LONG)pTimer->proc;
    msg->time    = curTime;

    LeaveCriticalSection( &csTimer );
    
    return TRUE;
}


/***********************************************************************
 *           TIMER_SetTimer
 */
static UINT TIMER_SetTimer( HWND hwnd, UINT id, UINT timeout,
                              WNDPROC16 proc, WINDOWPROCTYPE type, BOOL sys )
{
    int i;
    TIMER * pTimer;

    if (!timeout) return 0;

    EnterCriticalSection( &csTimer );
    
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
            TIMER_CheckTimers( TRUE );
            LeaveCriticalSection( &csTimer );
            return id;
        }

      /* Find a free timer */
    
    for (i = 0, pTimer = TimersArray; i < NB_TIMERS; i++, pTimer++)
	if (!pTimer->timeout) break;

    if ( (i >= NB_TIMERS) ||
         (!sys && (i >= NB_TIMERS-NB_RESERVED_TIMERS)) )
    {
        LeaveCriticalSection( &csTimer );
        return 0;
    }
    
    if (!hwnd) id = i + 1;
    
      /* Add the timer */

    pTimer->hwnd    = hwnd;
    pTimer->hq	    = (hwnd) ? GetThreadQueue16( GetWindowThreadProcessId( hwnd, NULL ) )
			     : GetFastQueue16( );
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
    TIMER_CheckTimers( TRUE );
    
    LeaveCriticalSection( &csTimer );
    
    if (!id) return TRUE;
    else return id;
}


/***********************************************************************
 *           TIMER_KillTimer
 */
static BOOL TIMER_KillTimer( HWND hwnd, UINT id, BOOL sys )
{
    int i;
    TIMER * pTimer;
    
    EnterCriticalSection( &csTimer );
    
    /* Find the timer */
    
    for (i = 0, pTimer = TimersArray; i < NB_TIMERS; i++, pTimer++)
	if ((pTimer->hwnd == hwnd) && (pTimer->id == id) &&
	    (pTimer->timeout != 0)) break;

    if ( (i >= NB_TIMERS) ||
         (!sys && (i >= NB_TIMERS-NB_RESERVED_TIMERS)) ||
         (!sys && (pTimer->msg != WM_TIMER)) ||
         (sys && (pTimer->msg != WM_SYSTIMER)) )
    {
        LeaveCriticalSection( &csTimer );
        return FALSE;
    }

    /* Delete the timer */

    TIMER_ClearTimer( pTimer );
    
    LeaveCriticalSection( &csTimer );
    
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
UINT WINAPI SetTimer( HWND hwnd, UINT id, UINT timeout,
                          TIMERPROC proc )
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
UINT WINAPI SetSystemTimer( HWND hwnd, UINT id, UINT timeout,
                                TIMERPROC proc )
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
BOOL WINAPI KillTimer( HWND hwnd, UINT id )
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
BOOL WINAPI KillSystemTimer( HWND hwnd, UINT id )
{
    TRACE(timer, "%04x %d\n", hwnd, id );
    return TIMER_KillTimer( hwnd, id, TRUE );
}
