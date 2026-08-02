/*
 * SYSTEM DLL routines
 *
 * Copyright 1996 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wownt32.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(system);

typedef struct
{
    FARPROC16       callback16;
    INT             rate;
    INT             ticks;
} SYSTEM_TIMER;

#define NB_SYS_TIMERS   8
#define SYS_TIMER_RATE  54925

static SYSTEM_TIMER SYS_Timers[NB_SYS_TIMERS];
static int SYS_NbTimers = 0;
static HANDLE SYS_timer;
static HANDLE SYS_thread;
static BOOL SYS_timers_disabled;


/***********************************************************************
 *           SYSTEM_TimerTick
 */
static void CALLBACK SYSTEM_TimerTick( LPVOID arg, DWORD low, DWORD high )
{
    int i;

    if (SYS_timers_disabled) return;
    for (i = 0; i < NB_SYS_TIMERS; i++)
    {
        if (!SYS_Timers[i].callback16) continue;
        if ((SYS_Timers[i].ticks -= SYS_TIMER_RATE) <= 0)
        {
            FARPROC16 proc = SYS_Timers[i].callback16;
            CONTEXT context;

            SYS_Timers[i].ticks += SYS_Timers[i].rate;

            memset( &context, 0, sizeof(context) );
            context.SegCs = SELECTOROF( proc );
            context.Eip   = OFFSETOF( proc );
            context.Ebp   = CURRENT_SP + FIELD_OFFSET(STACK16FRAME, bp);
            context.Eax   = i + 1;

            WOWCallback16Ex( 0, WCB16_REGS, 0, NULL, (DWORD *)&context );
        }
    }
}


/***********************************************************************
 *           SYSTEM_TimerThread
 */
static DWORD CALLBACK SYSTEM_TimerThread( void *dummy )
{
    LARGE_INTEGER when;

    if (!(SYS_timer = CreateWaitableTimerA( NULL, FALSE, NULL ))) return 0;

    when.u.LowPart = when.u.HighPart = 0;
    SetWaitableTimer( SYS_timer, &when, (SYS_TIMER_RATE+500)/1000, SYSTEM_TimerTick, 0, FALSE );
    for (;;) SleepEx( INFINITE, TRUE );
}


/**********************************************************************
 *           SYSTEM_StartTicks
 *
 * Start the system tick timer.
 */
static void SYSTEM_StartTicks(void)
{
    if (!SYS_thread) SYS_thread = CreateThread( NULL, 0, SYSTEM_TimerThread, NULL, 0, NULL );
}


/**********************************************************************
 *           SYSTEM_StopTicks
 *
 * Stop the system tick timer.
 */
static void SYSTEM_StopTicks(void)
{
    if (SYS_thread)
    {
        CancelWaitableTimer( SYS_timer );
        TerminateThread( SYS_thread, 0 );
        CloseHandle( SYS_thread );
        CloseHandle( SYS_timer );
        SYS_thread = 0;
    }
}


/***********************************************************************
 *           InquireSystem   (SYSTEM.1)
 *
 * Note: the function always takes 2 WORD arguments, contrary to what
 *       "Undocumented Windows" says.
  */
DWORD WINAPI InquireSystem16( WORD code, WORD arg )
{
    WORD drivetype;
    WCHAR root[3];

    switch(code)
    {
    case 0:  /* Get timer resolution */
        return SYS_TIMER_RATE;

    case 1:  /* Get drive type */
        root[0] = 'A' + arg;
        root[1] = ':';
        root[2] = 0;
        drivetype = GetDriveTypeW( root );
        if (drivetype == DRIVE_CDROM) drivetype = DRIVE_REMOTE;
        else if (drivetype == DRIVE_NO_ROOT_DIR) drivetype = DRIVE_UNKNOWN;
        return MAKELONG( drivetype, drivetype );

    case 2:  /* Enable one-drive logic */
        FIXME("Case %d: set single-drive %d not supported\n", code, arg );
        return 0;
    }
    WARN("Unknown code %d\n", code );
    return 0;
}


/***********************************************************************
 *           CreateSystemTimer   (SYSTEM.2)
 */
WORD WINAPI CreateSystemTimer16( WORD rate, FARPROC16 proc )
{
    int i;
    for (i = 0; i < NB_SYS_TIMERS; i++)
        if (!SYS_Timers[i].callback16)  /* Found one */
        {
            SYS_Timers[i].rate = (UINT)rate * 1000;
            if (SYS_Timers[i].rate < SYS_TIMER_RATE)
                SYS_Timers[i].rate = SYS_TIMER_RATE;
            SYS_Timers[i].ticks = SYS_Timers[i].rate;
            SYS_Timers[i].callback16 = proc;
            if (++SYS_NbTimers == 1) SYSTEM_StartTicks();
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
    if ( !timer || timer > NB_SYS_TIMERS || !SYS_Timers[timer-1].callback16 )
        return timer;  /* Error */
    SYS_Timers[timer-1].callback16 = 0;
    if (!--SYS_NbTimers) SYSTEM_StopTicks();
    return 0;
}


/***********************************************************************
 *           EnableSystemTimers   (SYSTEM.4)
 */
void WINAPI EnableSystemTimers16(void)
{
    SYS_timers_disabled = FALSE;
}


/***********************************************************************
 *           DisableSystemTimers   (SYSTEM.5)
 */
void WINAPI DisableSystemTimers16(void)
{
    SYS_timers_disabled = TRUE;
}


/***********************************************************************
 *           GetSystemMSecCount (SYSTEM.6)
 */
DWORD WINAPI GetSystemMSecCount16(void)
{
    return GetTickCount();
}


/***********************************************************************
 *           Get80x87SaveSize   (SYSTEM.7)
 */
WORD WINAPI Get80x87SaveSize16(void)
{
    return 94;
}


/***********************************************************************
 *           Save80x87State   (SYSTEM.8)
 */
void WINAPI Save80x87State16( char *ptr )
{
#ifdef __i386__
    __asm__(".byte 0x66; fsave %0; fwait" : "=m" (ptr) );
#endif
}


/***********************************************************************
 *           Restore80x87State   (SYSTEM.9)
 */
void WINAPI Restore80x87State16( const char *ptr )
{
#ifdef __i386__
    __asm__(".byte 0x66; frstor %0" : : "m" (ptr) );
#endif
}


/***********************************************************************
 *           A20_Proc  (SYSTEM.20)
 */
void WINAPI A20_Proc16( WORD unused )
{
    /* this is also a NOP in Windows */
}
