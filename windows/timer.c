/*
 * Timer functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "winuser.h"
#include "queue.h"
#include "task.h"
#include "winproc.h"
#include "services.h"
#include "message.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(timer)


typedef struct tagTIMER
{
    HWND           hwnd;
    HQUEUE16       hq;
    UINT16         msg;  /* WM_TIMER or WM_SYSTIMER */
    UINT           id;
    UINT           timeout;
    HANDLE         hService;
    BOOL           expired;
    HWINDOWPROC    proc;
} TIMER;

#define NB_TIMERS            34
#define NB_RESERVED_TIMERS    2  /* for SetSystemTimer */

#define SYS_TIMER_RATE  54925

static TIMER TimersArray[NB_TIMERS];

static CRITICAL_SECTION csTimer;


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
 *           TIMER_ClearTimer
 *
 * Clear and remove a timer.
 */
static void TIMER_ClearTimer( TIMER * pTimer )
{
    if ( pTimer->hService != INVALID_HANDLE_VALUE ) 
    {
        SERVICE_Delete( pTimer->hService );
        pTimer->hService = INVALID_HANDLE_VALUE;
    }

    if ( pTimer->expired ) 
    {
        QUEUE_DecTimerCount( pTimer->hq );
        pTimer->expired = FALSE;
    }

    pTimer->hwnd    = 0;
    pTimer->msg     = 0;
    pTimer->id      = 0;
    pTimer->timeout = 0;
    WINPROC_FreeProc( pTimer->proc, WIN_PROC_TIMER );
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
 *           TIMER_CheckTimer
 */
static void CALLBACK TIMER_CheckTimer( ULONG_PTR timer_ptr )
{
    TIMER *pTimer = (TIMER *)timer_ptr;
    HQUEUE16 wakeQueue = 0;

    EnterCriticalSection( &csTimer );

    /* Paranoid check to prevent a race condition ... */
    if ( !pTimer->timeout )
    {
        LeaveCriticalSection( &csTimer );
        return;
    }

    if ( !pTimer->expired )
    {
        TRACE("Timer expired: %04x, %04x, %04x, %08lx\n", 
                     pTimer->hwnd, pTimer->msg, pTimer->id, (DWORD)pTimer->proc);

        pTimer->expired = TRUE;
        wakeQueue = pTimer->hq;
    }

    LeaveCriticalSection( &csTimer );

    /* Note: This has to be done outside the csTimer critical section,
             otherwise we'll get deadlocks. */

    if ( wakeQueue )
        QUEUE_IncTimerCount( wakeQueue );
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
    int i;

    EnterCriticalSection( &csTimer );

    for (i = 0, pTimer = TimersArray; i < NB_TIMERS; i++, pTimer++)
        if (    pTimer->timeout != 0 && pTimer->expired
             && (hwnd? (pTimer->hwnd == hwnd) : (pTimer->hq == hQueue)) )
            break;

    if ( i == NB_TIMERS )
    {
        LeaveCriticalSection( &csTimer );
        return FALSE; /* No timer */
    }
    
    TRACE("Timer got message: %04x, %04x, %04x, %08lx\n", 
		   pTimer->hwnd, pTimer->msg, pTimer->id, (DWORD)pTimer->proc);

    if (remove)	
        pTimer->expired = FALSE;

      /* Build the message */
    msg->hwnd    = pTimer->hwnd;
    msg->message = pTimer->msg;
    msg->wParam  = pTimer->id;
    msg->lParam  = (LONG)pTimer->proc;
    msg->time    = GetTickCount();

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

    if (!timeout)
      {       /* timeout==0 is a legal argument  UB 990821*/
       WARN("Timeout== 0 not implemented, using timeout=1\n");
        timeout=1; 
      }
    EnterCriticalSection( &csTimer );
    
      /* Check if there's already a timer with the same hwnd and id */

    for (i = 0, pTimer = TimersArray; i < NB_TIMERS; i++, pTimer++)
        if ((pTimer->hwnd == hwnd) && (pTimer->id == id) &&
            (pTimer->timeout != 0))
        {
            TIMER_ClearTimer( pTimer );
            break;
        }

    if ( i == NB_TIMERS )
    {
          /* Find a free timer */
    
        for (i = 0, pTimer = TimersArray; i < NB_TIMERS; i++, pTimer++)
            if (!pTimer->timeout) break;

        if ( (i >= NB_TIMERS) ||
             (!sys && (i >= NB_TIMERS-NB_RESERVED_TIMERS)) )
        {
            LeaveCriticalSection( &csTimer );
            return 0;
        }
    }

    if (!hwnd) id = i + 1;
    
      /* Add the timer */

    pTimer->hwnd    = hwnd;
    pTimer->hq	    = (hwnd) ? GetThreadQueue16( GetWindowThreadProcessId( hwnd, NULL ) )
			     : GetFastQueue16( );
    pTimer->msg     = sys ? WM_SYSTIMER : WM_TIMER;
    pTimer->id      = id;
    pTimer->timeout = timeout;
    pTimer->proc    = (HWINDOWPROC)0;
    if (proc) WINPROC_SetProc( &pTimer->proc, proc, type, WIN_PROC_TIMER );

    pTimer->expired  = FALSE;
    pTimer->hService = SERVICE_AddTimer( max( timeout * 1000L, SYS_TIMER_RATE ),
                                       TIMER_CheckTimer, (ULONG_PTR)pTimer );
    
    TRACE("Timer added: %p, %04x, %04x, %04x, %08lx\n", 
		   pTimer, pTimer->hwnd, pTimer->msg, pTimer->id,
                   (DWORD)pTimer->proc );

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
    TRACE("%04x %d %d %08lx\n",
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
    TRACE("%04x %d %d %08lx\n",
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
    TRACE("%04x %d %d %08lx\n", 
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
    TRACE("%04x %d %d %08lx\n", 
                   hwnd, id, timeout, (LONG)proc );
    return TIMER_SetTimer( hwnd, id, timeout, (WNDPROC16)proc,
                           WIN_PROC_32A, TRUE );
}


/***********************************************************************
 *           KillTimer16   (USER.12)
 */
BOOL16 WINAPI KillTimer16( HWND16 hwnd, UINT16 id )
{
    TRACE("%04x %d\n", hwnd, id );
    return TIMER_KillTimer( hwnd, id, FALSE );
}


/***********************************************************************
 *           KillTimer32   (USER32.354)
 */
BOOL WINAPI KillTimer( HWND hwnd, UINT id )
{
    TRACE("%04x %d\n", hwnd, id );
    return TIMER_KillTimer( hwnd, id, FALSE );
}


/***********************************************************************
 *           KillSystemTimer16   (USER.182)
 */
BOOL16 WINAPI KillSystemTimer16( HWND16 hwnd, UINT16 id )
{
    TRACE("%04x %d\n", hwnd, id );
    return TIMER_KillTimer( hwnd, id, TRUE );
}


/***********************************************************************
 *           KillSystemTimer32   (USER32.353)
 */
BOOL WINAPI KillSystemTimer( HWND hwnd, UINT id )
{
    TRACE("%04x %d\n", hwnd, id );
    return TIMER_KillTimer( hwnd, id, TRUE );
}
