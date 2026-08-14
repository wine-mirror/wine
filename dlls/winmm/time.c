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

#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"

#include "winemm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmtime);

typedef struct tagWINE_TIMERENTRY {
    UINT                        wDelay;
    UINT                        wResol;
    LPTIMECALLBACK              lpFunc; /* can be lots of things */
    DWORD_PTR                   dwUser;
    UINT16                      wFlags;
    UINT16                      wTimerID;
    DWORD                       dwTriggerTime;
} WINE_TIMERENTRY, *LPWINE_TIMERENTRY;

static WINE_TIMERENTRY timers[16];
static UINT timers_created;

static CRITICAL_SECTION TIME_cbcrst;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &TIME_cbcrst,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": TIME_cbcrst") }
};
static CRITICAL_SECTION TIME_cbcrst = { &critsect_debug, -1, 0, 0, 0, 0 };

static    HANDLE                TIME_hMMTimer;
static    CONDITION_VARIABLE    TIME_cv;

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
    WINE_TIMERENTRY *timer;
    int i, delta_time;

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
        for (i = 0; i < ARRAY_SIZE(timers); i++)
            if (timers[i].wTimerID) break;
        if (i == ARRAY_SIZE(timers)) return -1;
        timer = timers + i;
        for (i++; i < ARRAY_SIZE(timers); i++)
        {
            if (!timers[i].wTimerID) continue;
            if (timers[i].dwTriggerTime < timer->dwTriggerTime)
                timer = timers + i;
        }

        delta_time = timer->dwTriggerTime - timeGetTime();
        if (delta_time > 0) break;

        if (timer->wFlags & TIME_PERIODIC)
            timer->dwTriggerTime += timer->wDelay;

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
                if (id != timer->wTimerID) timer = NULL;
            }
            break;
        }
        if (timer && !(timer->wFlags & TIME_PERIODIC))
            timer->wTimerID = 0;
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
    while (1)
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
 * 				timeSetEvent		[WINMM.@]
 */
MMRESULT WINAPI timeSetEvent(UINT wDelay, UINT wResol, LPTIMECALLBACK lpFunc,
                            DWORD_PTR dwUser, UINT wFlags)
{
    WORD new_id = 0;
    int i;

    TRACE("(%u, %u, %p, %08IX, %04X);\n", wDelay, wResol, lpFunc, dwUser, wFlags);

    if (wDelay < MMSYSTIME_MININTERVAL || wDelay > MMSYSTIME_MAXINTERVAL)
	return 0;

    EnterCriticalSection(&WINMM_cs);

    for (i = 0; i < ARRAY_SIZE(timers); i++)
        if (!timers[i].wTimerID) break;
    if (i == ARRAY_SIZE(timers))
    {
        LeaveCriticalSection(&WINMM_cs);
        return 0;
    }

    new_id = ARRAY_SIZE(timers)*(++timers_created) + i;
    if (!new_id) new_id = ARRAY_SIZE(timers)*(++timers_created) + i;

    timers[i].wDelay = wDelay;
    timers[i].dwTriggerTime = timeGetTime() + wDelay;

    /* FIXME - wResol is not respected, although it is not clear
       that we could change our precision meaningfully  */
    timers[i].wResol = wResol;
    timers[i].lpFunc = lpFunc;
    timers[i].dwUser = dwUser;
    timers[i].wFlags = wFlags;
    timers[i].wTimerID = new_id;

    TIME_MMTimeStart();

    LeaveCriticalSection(&WINMM_cs);

    /* Wake the service thread in case there is work to be done */
    WakeConditionVariable(&TIME_cv);

    TRACE("=> %u\n", new_id);

    return new_id;
}

/**************************************************************************
 * 				timeKillEvent		[WINMM.@]
 */
MMRESULT WINAPI timeKillEvent(UINT wID)
{
    WINE_TIMERENTRY *timer;
    WORD flags;

    TRACE("(%u)\n", wID);
    EnterCriticalSection(&WINMM_cs);

    timer = &timers[wID % ARRAY_SIZE(timers)];
    if (timer->wTimerID != wID)
    {
        LeaveCriticalSection(&WINMM_cs);
        WARN("wID=%u is not a valid timer ID\n", wID);
        return TIMERR_NOCANDO;
    }

    timer->wTimerID = 0;
    flags = timer->wFlags;
    LeaveCriticalSection(&WINMM_cs);

    if (flags & TIME_KILL_SYNCHRONOUS)
    {
        EnterCriticalSection(&TIME_cbcrst);
        LeaveCriticalSection(&TIME_cbcrst);
    }
    WakeConditionVariable(&TIME_cv);
    return TIMERR_NOERROR;
}
