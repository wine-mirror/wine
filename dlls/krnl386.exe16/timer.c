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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include "dosexe.h"
#include "wine/debug.h"
#include "wingdi.h"
#include "winuser.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

/*
 * FIXME: Use QueryPerformanceCounter for
 *        more precise GetTimer implementation.
 * FIXME: Use QueryPerformanceCounter (or GetTimer implementation)
 *        in timer tick routine to compensate for lost ticks. 
 *        This should also make it possible to
 *        emulate really fast timers.
 * FIXME: Support special timer modes in addition to periodic mode.
 * FIXME: Use timeSetEvent, NtSetEvent or timer thread for more precise
 *        timing.
 * FIXME: Move Win16 timer emulation code here.
 */

/* The PC clocks ticks at 1193180 Hz. */
#define TIMER_FREQ 1193180

/* How many timer IRQs can be pending at any time. */
#define TIMER_MAX_PENDING 20

/* Unique system timer identifier. */
static UINT_PTR TIMER_id = 0;

/* Time when timer IRQ was last queued. */
static DWORD TIMER_stamp = 0;

/* Timer ticks between timer IRQs. */
static UINT TIMER_ticks = 0;

/* Number of pending timer IRQs. */
static LONG TIMER_pending = 0;

/* Number of milliseconds between IRQs. */
static DWORD TIMER_millis = 0;

/*********************************************************************** 
 *              TIMER_Relay
 *
 * Decrement the number of pending IRQs after IRQ handler has been 
 * called. This function will be called even if application has its 
 * own IRQ handler that does not jump to builtin IRQ handler.
 */
static void TIMER_Relay( CONTEXT *context, void *data )
{
    InterlockedDecrement( &TIMER_pending );
}


/***********************************************************************
 *              TIMER_TimerProc
 */
static void CALLBACK TIMER_TimerProc( HWND     hwnd,
                                      UINT     uMsg,
                                      UINT_PTR idEvent,
                                      DWORD    dwTime )
{
    LONG pending = InterlockedIncrement( &TIMER_pending );
    DWORD delta = (dwTime >= TIMER_stamp) ?
        (dwTime - TIMER_stamp) : (0xffffffff - (TIMER_stamp - dwTime));

    if (pending >= TIMER_MAX_PENDING)
    {

        if (delta >= 60000)
        {
            ERR( "DOS timer has been stuck for 60 seconds...\n" );
            TIMER_stamp = dwTime;
        }

        InterlockedDecrement( &TIMER_pending );
    }
    else
    {
        DWORD i;

        /* Calculate the number of valid timer interrupts we can generate */
        DWORD count = delta / TIMER_millis;

        /* Forward the timestamp with the time used */
        TIMER_stamp += (count * TIMER_millis);

        /* Generate interrupts */
        for(i=0;i<count;i++)
        {
          DOSVM_QueueEvent( 0, DOS_PRIORITY_REALTIME, TIMER_Relay, NULL );
        }
    }
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

    /* Remember number of milliseconds to wait */
    TIMER_millis = millis;
}


/***********************************************************************
 *              DOSVM_SetTimer
 */
void DOSVM_SetTimer( UINT ticks )
{
    /* PIT interprets zero as a maximum length delay. */
    if (ticks == 0)
        ticks = 0x10000;

    if (!DOSVM_IsWin16())
        MZ_RunInThread( TIMER_DoSetTimer, ticks );
}


/***********************************************************************
 *              DOSVM_Int08Handler
 *
 * DOS interrupt 08h handler (IRQ0 - TIMER).
 */
void WINAPI DOSVM_Int08Handler( CONTEXT *context )
{
    BIOSDATA *bios_data      = DOSVM_BiosData();
    CONTEXT nested_context = *context;
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
