/*
 * 8253/8254 Programmable Interval Timer (PIT) emulation
 *
 * Copyright 2003 Jukka Heinonen
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "dosexe.h"
#include "wine/debug.h"
#include "wingdi.h"
#include "winuser.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

/*
 * FIXME: Move timer ioport handling here and remove 
 *        Dosvm.GetTimer/Dosvm.SetTimer.
 * FIXME: Use QueryPerformanceCounter for
 *        more precise GetTimer implementation.
 * FIXME: Use QueryPerformanceCounter (or GetTimer implementation)
 *        in timer tick routine to compensate for lost ticks. 
 *        This should also make it possible to
 *        emulate really fast timers.
 * FIXME: Support special timer modes in addition to periodic mode.
 * FIXME: Make sure that there are only limited number
 *        of pending timer IRQ events queued. This makes sure that
 *        timer handling does not eat all available memory even
 *        if IRQ handling stops for some reason (suspended process?). 
 *        This is easy to do by using DOSRELAY parameter.
 * FIXME: Use timeSetEvent, NtSetEvent or timer thread for more precise
 *        timing.
 * FIXME: Move Win16 timer emulation code here.
 */

/* The PC clocks ticks at 1193180 Hz. */
#define TIMER_FREQ 1193180

/* Unique system timer identifier. */
static UINT_PTR TIMER_id = 0;

/* Time when timer IRQ was last queued. */
static DWORD TIMER_stamp = 0;

/* Timer ticks between timer IRQs. */
static UINT TIMER_ticks = 0;


/***********************************************************************
 *              TIMER_TimerProc
 */
static void CALLBACK TIMER_TimerProc( HWND     hwnd,
                                      UINT     uMsg,
                                      UINT_PTR idEvent,
                                      DWORD    dwTime )
{
    TIMER_stamp = dwTime;
    DOSVM_QueueEvent( 0, DOS_PRIORITY_REALTIME, NULL, NULL );
}


/***********************************************************************
 *              TIMER_DoSetTimer
 */
static void WINAPI TIMER_DoSetTimer( ULONG_PTR arg )
{
    INT millis = MulDiv( arg, 1000, TIMER_FREQ );

    /* sanity check - too fast timer */
    if (millis < 1)
        millis = 1;

    TRACE_(int)( "setting timer tick delay to %d ms\n", millis );

    if (TIMER_id)
        KillTimer( NULL, TIMER_id );

    TIMER_id = SetTimer( NULL, 0, millis, TIMER_TimerProc );
    TIMER_stamp = GetTickCount();
    TIMER_ticks = arg;
}


/***********************************************************************
 *              GetTimer (WINEDOS.@)
 */
UINT WINAPI DOSVM_GetTimer( void )
{
    if (!DOSVM_IsWin16())
    {
        DWORD millis = GetTickCount() - TIMER_stamp;
        INT   ticks = MulDiv( millis, TIMER_FREQ, 1000 );

        /* sanity check - tick wrap or suspended process or update race */
        if (ticks < 0 || ticks >= TIMER_ticks)
            ticks = 0;

        return ticks;
    }

    return 0;
}


/***********************************************************************
 *              SetTimer (WINEDOS.@)
 */
void WINAPI DOSVM_SetTimer( UINT ticks )
{
    if (!DOSVM_IsWin16())
        MZ_RunInThread( TIMER_DoSetTimer, ticks );
}


/***********************************************************************
 *              DOSVM_Int08Handler
 *
 * DOS interrupt 08h handler (IRQ0 - TIMER).
 */
void WINAPI DOSVM_Int08Handler( CONTEXT86 *context )
{
    BIOSDATA *bios_data      = BIOS_DATA;
    CONTEXT86 nested_context = *context;
    FARPROC16 int1c_proc     = DOSVM_GetRMHandler( 0x1c );
    
    nested_context.SegCs = SELECTOROF(int1c_proc);
    nested_context.Eip   = OFFSETOF(int1c_proc);

    /*
     * Update BIOS ticks since midnight.
     *
     * FIXME: What to do when number of ticks exceeds ticks per day?
     */
    bios_data->Ticks++;

    /*
     * If IRQ is called from protected mode, convert
     * context into VM86 context. Stack is invalidated so
     * that DPMI_CallRMProc allocates a new stack.
     */
    if (!ISV86(&nested_context))
    {
        nested_context.EFlags |= V86_FLAG;
        nested_context.SegSs = 0;
    }

    /*
     * Call interrupt 0x1c.
     */
    DPMI_CallRMProc( &nested_context, NULL, 0, TRUE );

    DOSVM_AcknowledgeIRQ( context );
}
