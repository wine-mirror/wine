/*
 * SYSTEM DLL routines
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "selectors.h"
#include "sig_context.h"
#include "miscemu.h"
#include "debug.h"

typedef struct
{
    SYSTEMTIMERPROC callback;  /* NULL if not in use */
    INT32     rate;
    INT32     ticks;
} SYSTEM_TIMER;

#define NB_SYS_TIMERS   8
#define SYS_TIMER_RATE  54925

static SYSTEM_TIMER SYS_Timers[NB_SYS_TIMERS];
static int SYS_NbTimers = 0;
static BOOL32 SYS_TimersDisabled = FALSE;


/***********************************************************************
 *           SYSTEM_TimerTick
 */
static HANDLER_DEF(SYSTEM_TimerTick)
{
    int i;
    HANDLER_INIT();

    for (i = 0; i < NB_SYS_TIMERS; i++)
    {
        if (!SYS_Timers[i].callback) continue;
        if ((SYS_Timers[i].ticks -= SYS_TIMER_RATE) <= 0)
        {
            SYS_Timers[i].ticks += SYS_Timers[i].rate;
            SYS_Timers[i].callback( i+1 );
        }
    }
}

/**********************************************************************
 *           SYSTEM_StartTicks
 *
 * Start the system tick timer.
 */
static void SYSTEM_StartTicks(void)
{
    static BOOL32 handler_installed = FALSE;

    if (!handler_installed)
    {
        handler_installed = TRUE;
	SIGNAL_SetHandler( SIGALRM, SYSTEM_TimerTick, 1 );
    }
#ifndef __EMX__ /* FIXME: Time don't work... Use BIOS directly instead */
    {
        struct itimerval vt_timer;

        vt_timer.it_interval.tv_sec = 0;
        vt_timer.it_interval.tv_usec = 54929;
        vt_timer.it_value = vt_timer.it_interval;
        setitimer( ITIMER_REAL, &vt_timer, NULL );
    }
#endif
}


/**********************************************************************
 *           SYSTEM_StopTicks
 *
 * Stop the system tick timer.
 */
static void SYSTEM_StopTicks(void)
{
#ifndef __EMX__ /* FIXME: Time don't work... Use BIOS directly instead */
    struct itimerval vt_timer;

    vt_timer.it_interval.tv_sec = 0;
    vt_timer.it_interval.tv_usec = 0;
    vt_timer.it_value = vt_timer.it_interval;
    setitimer( ITIMER_REAL, &vt_timer, NULL );
#endif
}


/***********************************************************************
 *           InquireSystem   (SYSTEM.1)
 *
 * Note: the function always takes 2 WORD arguments, contrary to what
 *       "Undocumented Windows" says.
  */
DWORD WINAPI InquireSystem( WORD code, WORD arg )
{
    WORD drivetype;

    switch(code)
    {
    case 0:  /* Get timer resolution */
        return SYS_TIMER_RATE;

    case 1:  /* Get drive type */
        drivetype = GetDriveType16( arg );
        return MAKELONG( drivetype, drivetype );

    case 2:  /* Enable one-drive logic */
        FIXME(system, "Case %d: set single-drive %d not supported\n", code, arg );
        return 0;
    }
    WARN(system, "Unknown code %d\n", code );
    return 0;
}


/***********************************************************************
 *           CreateSystemTimer   (SYSTEM.2)
 */
WORD WINAPI CreateSystemTimer( WORD rate, SYSTEMTIMERPROC callback )
{
    int i;
    for (i = 0; i < NB_SYS_TIMERS; i++)
        if (!SYS_Timers[i].callback)  /* Found one */
        {
            SYS_Timers[i].rate = (UINT32)rate * 1000;
            if (SYS_Timers[i].rate < SYS_TIMER_RATE)
                SYS_Timers[i].rate = SYS_TIMER_RATE;
            SYS_Timers[i].ticks = SYS_Timers[i].rate;
            SYS_Timers[i].callback = callback;
            if ((++SYS_NbTimers == 1) && !SYS_TimersDisabled)
                SYSTEM_StartTicks();
            return i + 1;  /* 0 means error */
        }
    return 0;
}


/***********************************************************************
 *           KillSystemTimer   (SYSTEM.3)
 *
 * Note: do not confuse this function with USER.182
 */
WORD WINAPI SYSTEM_KillSystemTimer( WORD timer )
{
    if (!timer || (timer > NB_SYS_TIMERS)) return timer;  /* Error */
    SYS_Timers[timer-1].callback = NULL;
    if ((!--SYS_NbTimers) && !SYS_TimersDisabled) SYSTEM_StopTicks();
    return 0;
}


/***********************************************************************
 *           EnableSystemTimers   (SYSTEM.4)
 */
void WINAPI EnableSystemTimers(void)
{
    SYS_TimersDisabled = FALSE;
    if (SYS_NbTimers) SYSTEM_StartTicks();
}


/***********************************************************************
 *           DisableSystemTimers   (SYSTEM.5)
 */
void WINAPI DisableSystemTimers(void)
{
    SYS_TimersDisabled = TRUE;
    if (SYS_NbTimers) SYSTEM_StopTicks();
}
