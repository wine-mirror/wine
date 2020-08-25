/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MMSYSTEM time functions
 *
 * Copyright 1993 Martin Ayotte
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
#include <errno.h>
#include <time.h>

#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"

#include "winemm.h"

#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmtime);

typedef struct tagWINE_TIMERENTRY {
    struct list                 entry;
    UINT                        wDelay;
    UINT                        wResol;
    LPTIMECALLBACK              lpFunc; /* can be lots of things */
    DWORD_PTR                   dwUser;
    UINT16                      wFlags;
    UINT16                      wTimerID;
    DWORD                       dwTriggerTime;
} WINE_TIMERENTRY, *LPWINE_TIMERENTRY;

static struct list timer_list = LIST_INIT(timer_list);

static CRITICAL_SECTION TIME_cbcrst;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &TIME_cbcrst,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": TIME_cbcrst") }
};
static CRITICAL_SECTION TIME_cbcrst = { &critsect_debug, -1, 0, 0, 0, 0 };

static    HANDLE                TIME_hMMTimer;
static    BOOL                  TIME_TimeToDie = TRUE;
static    CONDITION_VARIABLE    TIME_cv;

/* link timer at the appropriate spot in the list */
static inline void link_timer( WINE_TIMERENTRY *timer )
{
    WINE_TIMERENTRY *next;

    LIST_FOR_EACH_ENTRY( next, &timer_list, WINE_TIMERENTRY, entry )
        if ((int)(next->dwTriggerTime - timer->dwTriggerTime) >= 0) break;

    list_add_before( &next->entry, &timer->entry );
}

/*
 * Some observations on the behavior of winmm on Windows.
 *
 * First, the call to timeBeginPeriod(xx) can never be used to
 * lower the timer resolution (i.e. increase the update
 * interval), only to increase the timer resolution (i.e. lower
 * the update interval).
 *
 * Second, a brief survey of a variety of Win 2k and Win X
 * machines showed that a 'standard' (aka default) timer
 * resolution was 1 ms (Win9x is documented as being 1).  However, one 
 * machine had a standard timer resolution of 10 ms.
 *
 * Further, timeBeginPeriod(xx) also affects the resolution of
 * wait calls such as NtDelayExecution() and
 * NtWaitForMultipleObjects() which by default round up their
 * timeout to the nearest multiple of 15.625ms across all Windows
 * versions. In Wine all of those currently work with sub-1ms
 * accuracy.
 *
 * Effective time resolution is a global value that is the max
 * of the resolutions (i.e. min of update intervals) requested by
 * all the processes. A lot of programs seem to do
 * timeBeginPeriod(1) forcing it onto everyone else.
 *
 * Defaulting to 1ms accuracy in winmm should be safe.
 *
 * Additionally, a survey of Event behaviors shows that
 * if we request a Periodic event every 50 ms, then Windows
 * makes sure to trigger that event 20 times in the next
 * second.  If delays prevent that from happening on exact
 * schedule, Windows will trigger the events as close
 * to the original schedule as is possible, and will eventually
 * bring the event triggers back onto a schedule that is
 * consistent with what would have happened if there were
 * no delays.
 *
 *   Jeremy White, October 2004
 *   Arkadiusz Hiler, August 2020
 */
#define MMSYSTIME_MININTERVAL (1)
#define MMSYSTIME_MAXINTERVAL (65535)

/**************************************************************************
 *           TIME_MMSysTimeCallback
 */
static int TIME_MMSysTimeCallback(void)
{
    WINE_TIMERENTRY *timer, *to_free;
    int delta_time;

    /* since timeSetEvent() and timeKillEvent() can be called
     * from 16 bit code, there are cases where win16 lock is
     * locked upon entering timeSetEvent(), and then the mm timer
     * critical section is locked. This function cannot call the
     * timer callback with the crit sect locked (because callback
     * may need to acquire Win16 lock, thus providing a deadlock
     * situation).
     * To cope with that, we just copy the WINE_TIMERENTRY struct
     * that need to trigger the callback, and call it without the
     * mm timer crit sect locked.
     */

    for (;;)
    {
        struct list *ptr = list_head( &timer_list );
        if (!ptr)
        {
            delta_time = -1;
            break;
        }

        timer = LIST_ENTRY( ptr, WINE_TIMERENTRY, entry );
        delta_time = timer->dwTriggerTime - timeGetTime();
        if (delta_time > 0) break;

        list_remove( &timer->entry );
        if (timer->wFlags & TIME_PERIODIC)
        {
            timer->dwTriggerTime += timer->wDelay;
            link_timer( timer );  /* restart it */
            to_free = NULL;
        }
        else to_free = timer;

        switch(timer->wFlags & (TIME_CALLBACK_EVENT_SET|TIME_CALLBACK_EVENT_PULSE))
        {
        case TIME_CALLBACK_EVENT_SET:
            SetEvent(timer->lpFunc);
            break;
        case TIME_CALLBACK_EVENT_PULSE:
            PulseEvent(timer->lpFunc);
            break;
        case TIME_CALLBACK_FUNCTION:
            {
                DWORD_PTR user = timer->dwUser;
                UINT16 id = timer->wTimerID;
                UINT16 flags = timer->wFlags;
                LPTIMECALLBACK func = timer->lpFunc;

                if (flags & TIME_KILL_SYNCHRONOUS) EnterCriticalSection(&TIME_cbcrst);
                LeaveCriticalSection(&WINMM_cs);

                func(id, 0, user, 0, 0);

                EnterCriticalSection(&WINMM_cs);
                if (flags & TIME_KILL_SYNCHRONOUS) LeaveCriticalSection(&TIME_cbcrst);
            }
            break;
        }
        HeapFree( GetProcessHeap(), 0, to_free );
    }
    return delta_time;
}

/**************************************************************************
 *           TIME_MMSysTimeThread
 */
static DWORD CALLBACK TIME_MMSysTimeThread(LPVOID arg)
{
    int sleep_time;
    BOOL ret;

    TRACE("Starting main winmm thread\n");

    EnterCriticalSection(&WINMM_cs);
    while (! TIME_TimeToDie) 
    {
        sleep_time = TIME_MMSysTimeCallback();

        if (sleep_time < 0)
            break;
        if (sleep_time == 0)
            continue;

        ret = SleepConditionVariableCS(&TIME_cv, &WINMM_cs, sleep_time);
        if (!ret && GetLastError() != ERROR_TIMEOUT)
        {
            ERR("Unexpected error in poll: %s(%d)\n", strerror(errno), errno);
            break;
         }
    }
    CloseHandle(TIME_hMMTimer);
    TIME_hMMTimer = NULL;
    LeaveCriticalSection(&WINMM_cs);
    TRACE("Exiting main winmm thread\n");
    FreeLibraryAndExitThread(arg, 0);
    return 0;
}

/**************************************************************************
 * 				TIME_MMTimeStart
 */
static void TIME_MMTimeStart(void)
{
    HMODULE mod;
    TIME_TimeToDie = 0;
    if (TIME_hMMTimer) return;

    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)TIME_MMSysTimeThread, &mod);
    TIME_hMMTimer = CreateThread(NULL, 0, TIME_MMSysTimeThread, mod, 0, NULL);
    SetThreadPriority(TIME_hMMTimer, THREAD_PRIORITY_TIME_CRITICAL);
}

/**************************************************************************
 * 				TIME_MMTimeStop
 */
void	TIME_MMTimeStop(void)
{
    if (TIME_hMMTimer) {
        EnterCriticalSection(&WINMM_cs);
        if (TIME_hMMTimer) {
            ERR("Timer still active?!\n");
            CloseHandle(TIME_hMMTimer);
        }
        DeleteCriticalSection(&TIME_cbcrst);
    }
}

/**************************************************************************
 * 				timeGetSystemTime	[WINMM.@]
 */
MMRESULT WINAPI timeGetSystemTime(LPMMTIME lpTime, UINT wSize)
{
    if (wSize >= sizeof(*lpTime)) {
        lpTime->wType = TIME_MS;
        lpTime->u.ms = timeGetTime();
    }

    return 0;
}

/**************************************************************************
 * 				timeGetTime		[WINMM.@]
 */
DWORD WINAPI timeGetTime(void)
{
    LARGE_INTEGER now, freq;

    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&freq);

    return (now.QuadPart * 1000) / freq.QuadPart;
}

/**************************************************************************
 * 				timeSetEvent		[WINMM.@]
 */
MMRESULT WINAPI timeSetEvent(UINT wDelay, UINT wResol, LPTIMECALLBACK lpFunc,
                            DWORD_PTR dwUser, UINT wFlags)
{
    WORD 		wNewID = 0;
    LPWINE_TIMERENTRY	lpNewTimer;
    LPWINE_TIMERENTRY	lpTimer;

    TRACE("(%u, %u, %p, %08lX, %04X);\n", wDelay, wResol, lpFunc, dwUser, wFlags);

    if (wDelay < MMSYSTIME_MININTERVAL || wDelay > MMSYSTIME_MAXINTERVAL)
	return 0;

    lpNewTimer = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_TIMERENTRY));
    if (lpNewTimer == NULL)
	return 0;

    lpNewTimer->wDelay = wDelay;
    lpNewTimer->dwTriggerTime = timeGetTime() + wDelay;

    /* FIXME - wResol is not respected, although it is not clear
               that we could change our precision meaningfully  */
    lpNewTimer->wResol = wResol;
    lpNewTimer->lpFunc = lpFunc;
    lpNewTimer->dwUser = dwUser;
    lpNewTimer->wFlags = wFlags;

    EnterCriticalSection(&WINMM_cs);

    LIST_FOR_EACH_ENTRY( lpTimer, &timer_list, WINE_TIMERENTRY, entry )
        wNewID = max(wNewID, lpTimer->wTimerID);

    link_timer( lpNewTimer );
    lpNewTimer->wTimerID = wNewID + 1;

    TIME_MMTimeStart();

    LeaveCriticalSection(&WINMM_cs);

    /* Wake the service thread in case there is work to be done */
    WakeConditionVariable(&TIME_cv);

    TRACE("=> %u\n", wNewID + 1);

    return wNewID + 1;
}

/**************************************************************************
 * 				timeKillEvent		[WINMM.@]
 */
MMRESULT WINAPI timeKillEvent(UINT wID)
{
    WINE_TIMERENTRY *lpSelf = NULL, *lpTimer;
    DWORD wFlags;

    TRACE("(%u)\n", wID);
    EnterCriticalSection(&WINMM_cs);
    /* remove WINE_TIMERENTRY from list */
    LIST_FOR_EACH_ENTRY( lpTimer, &timer_list, WINE_TIMERENTRY, entry )
    {
	if (wID == lpTimer->wTimerID) {
            lpSelf = lpTimer;
            list_remove( &lpTimer->entry );
	    break;
	}
    }
    if (list_empty(&timer_list)) {
        TIME_TimeToDie = 1;
        WakeConditionVariable(&TIME_cv);
    }
    LeaveCriticalSection(&WINMM_cs);

    if (!lpSelf)
    {
        WARN("wID=%u is not a valid timer ID\n", wID);
        return MMSYSERR_INVALPARAM;
    }
    wFlags = lpSelf->wFlags;
    if (wFlags & TIME_KILL_SYNCHRONOUS)
        EnterCriticalSection(&TIME_cbcrst);
    HeapFree(GetProcessHeap(), 0, lpSelf);
    if (wFlags & TIME_KILL_SYNCHRONOUS)
        LeaveCriticalSection(&TIME_cbcrst);
    return TIMERR_NOERROR;
}

/**************************************************************************
 * 				timeGetDevCaps		[WINMM.@]
 */
MMRESULT WINAPI timeGetDevCaps(LPTIMECAPS lpCaps, UINT wSize)
{
    TRACE("(%p, %u)\n", lpCaps, wSize);

    if (lpCaps == 0) {
        WARN("invalid lpCaps\n");
        return TIMERR_NOCANDO;
    }

    if (wSize < sizeof(TIMECAPS)) {
        WARN("invalid wSize\n");
        return TIMERR_NOCANDO;
    }

    lpCaps->wPeriodMin = MMSYSTIME_MININTERVAL;
    lpCaps->wPeriodMax = MMSYSTIME_MAXINTERVAL;
    return TIMERR_NOERROR;
}

/**************************************************************************
 * 				timeBeginPeriod		[WINMM.@]
 */
MMRESULT WINAPI timeBeginPeriod(UINT wPeriod)
{
    if (wPeriod < MMSYSTIME_MININTERVAL || wPeriod > MMSYSTIME_MAXINTERVAL)
	return TIMERR_NOCANDO;

    if (wPeriod > MMSYSTIME_MININTERVAL)
    {
        WARN("Stub; we set our timer resolution at minimum\n");
    }

    return 0;
}

/**************************************************************************
 * 				timeEndPeriod		[WINMM.@]
 */
MMRESULT WINAPI timeEndPeriod(UINT wPeriod)
{
    if (wPeriod < MMSYSTIME_MININTERVAL || wPeriod > MMSYSTIME_MAXINTERVAL)
	return TIMERR_NOCANDO;

    if (wPeriod > MMSYSTIME_MININTERVAL)
    {
        WARN("Stub; we set our timer resolution at minimum\n");
    }
    return 0;
}
