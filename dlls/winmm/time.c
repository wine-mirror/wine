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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <errno.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"

#include "winemm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmtime);

static    HANDLE                TIME_hMMTimer;
static    LPWINE_TIMERENTRY 	TIME_TimersList;
static    CRITICAL_SECTION      TIME_cbcrst;
static    BOOL                  TIME_TimeToDie = TRUE;
static    int                   TIME_fdWake[2] = { -1, -1 };

/*
 * Some observations on the behavior of winmm on Windows.
 * First, the call to timeBeginPeriod(xx) can never be used
 * to raise the timer resolution, only lower it.
 *
 * Second, a brief survey of a variety of Win 2k and Win X
 * machines showed that a 'standard' (aka default) timer
 * resolution was 1 ms (Win9x is documented as being 1).  However, one 
 * machine had a standard timer resolution of 10 ms.
 *
 * Further, if we set our default resolution to 1,
 * the implementation of timeGetTime becomes GetTickCount(),
 * and we can optimize the code to reduce overhead.
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
 */
#define MMSYSTIME_MININTERVAL (1)
#define MMSYSTIME_MAXINTERVAL (65535)


static	void	TIME_TriggerCallBack(LPWINE_TIMERENTRY lpTimer)
{
    TRACE("%04x:CallBack => lpFunc=%p wTimerID=%04X dwUser=%08X dwTriggerTime %d(delta %d)\n",
	  GetCurrentThreadId(), lpTimer->lpFunc, lpTimer->wTimerID, lpTimer->dwUser,
          lpTimer->dwTriggerTime, GetTickCount() - lpTimer->dwTriggerTime);

    /* - TimeProc callback that is called here is something strange, under Windows 3.1x it is called
     * 		during interrupt time,  is allowed to execute very limited number of API calls (like
     *	    	PostMessage), and must reside in DLL (therefore uses stack of active application). So I
     *       guess current implementation via SetTimer has to be improved upon.
     */
    switch (lpTimer->wFlags & 0x30) {
    case TIME_CALLBACK_FUNCTION:
	if (lpTimer->wFlags & WINE_TIMER_IS32)
	    (lpTimer->lpFunc)(lpTimer->wTimerID, 0, lpTimer->dwUser, 0, 0);
	else if (pFnCallMMDrvFunc16)
	    pFnCallMMDrvFunc16((DWORD)lpTimer->lpFunc, lpTimer->wTimerID, 0,
                               lpTimer->dwUser, 0, 0);
	break;
    case TIME_CALLBACK_EVENT_SET:
	SetEvent((HANDLE)lpTimer->lpFunc);
	break;
    case TIME_CALLBACK_EVENT_PULSE:
	PulseEvent((HANDLE)lpTimer->lpFunc);
	break;
    default:
	FIXME("Unknown callback type 0x%04x for mmtime callback (%p), ignored.\n",
	      lpTimer->wFlags, lpTimer->lpFunc);
	break;
    }
}

/**************************************************************************
 *           TIME_MMSysTimeCallback
 */
static int TIME_MMSysTimeCallback(LPWINE_MM_IDATA iData)
{
static    int				nSizeLpTimers;
static    LPWINE_TIMERENTRY		lpTimers;

    LPWINE_TIMERENTRY   timer, *ptimer, *next_ptimer;
    int			idx;
    DWORD               cur_time;
    int delta_time, ret_time = -1;

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
     * the hKillTimeEvent is used to mark the section where we 
     * handle the callbacks so we can do synchronous kills.
     * EPP 99/07/13, updated 04/01/10
     */
    idx = 0;
    cur_time = GetTickCount();

    EnterCriticalSection(&iData->cs);
    for (ptimer = &TIME_TimersList; *ptimer != NULL; ) {
        timer = *ptimer;
        next_ptimer = &timer->lpNext;
        delta_time = timer->dwTriggerTime - cur_time;
        if (delta_time <= 0)
        {
            if (timer->lpFunc) {
                if (idx == nSizeLpTimers) {
                    if (lpTimers) 
                        lpTimers = (LPWINE_TIMERENTRY)
                            HeapReAlloc(GetProcessHeap(), 0, lpTimers,
                                        ++nSizeLpTimers * sizeof(WINE_TIMERENTRY));
                    else 
                        lpTimers = (LPWINE_TIMERENTRY)
                        HeapAlloc(GetProcessHeap(), 0,
                                  ++nSizeLpTimers * sizeof(WINE_TIMERENTRY));
                }
                lpTimers[idx++] = *timer;

            }

            /* Update the time after we make the copy to preserve
               the original trigger time    */
            timer->dwTriggerTime += timer->wDelay;

            /* TIME_ONESHOT is defined as 0 */
            if (!(timer->wFlags & TIME_PERIODIC))
            {
                /* unlink timer from timers list */
                *ptimer = *next_ptimer;
                HeapFree(GetProcessHeap(), 0, timer);

                /* We don't need to trigger oneshots again */
                delta_time = -1;
            }
            else
            {
                /* Compute when this event needs this function
                    to be called again */
                delta_time = timer->dwTriggerTime - cur_time;
                if (delta_time < 0) delta_time = 0;
            }
        }

        /* Determine when we need to return to this function */
        if (delta_time != -1)
        {
            if (ret_time == -1 || ret_time > delta_time) ret_time = delta_time;
        }

        ptimer = next_ptimer;
    }
    LeaveCriticalSection(&iData->cs);

    EnterCriticalSection(&TIME_cbcrst);
    while (idx > 0) TIME_TriggerCallBack(&lpTimers[--idx]);
    LeaveCriticalSection(&TIME_cbcrst);

    /* Finally, adjust the recommended wait time downward
       by the amount of time the processing routines 
       actually took */
    if (ret_time != -1)
    {
        ret_time -= GetTickCount() - cur_time;
        if (ret_time < 0) ret_time = 0;
    }

    /* We return the amount of time our caller should sleep
       before needing to check in on us again       */
    return ret_time;
}

/**************************************************************************
 *           TIME_MMSysTimeThread
 */
static DWORD CALLBACK TIME_MMSysTimeThread(LPVOID arg)
{
    LPWINE_MM_IDATA iData = (LPWINE_MM_IDATA)arg;
    int sleep_time, ret;
    char readme[16];
    struct pollfd pfd;

    pfd.fd = TIME_fdWake[0];
    pfd.events = POLLIN;

    TRACE("Starting main winmm thread\n");

    /* FIXME:  As an optimization, we could have
               this thread die when there are no more requests
               pending, and then get recreated on the first
               new event; it's not clear if that would be worth
               it or not.                 */

    while (! TIME_TimeToDie) 
    {
        sleep_time = TIME_MMSysTimeCallback(iData);

        if (sleep_time == 0)
            continue;

        if ((ret = poll(&pfd, 1, sleep_time)) < 0)
        {
            if (errno != EINTR && errno != EAGAIN)
            {
                ERR("Unexpected error in poll: %s(%d)\n", strerror(errno), errno);
                break;
            }
         }

        while (ret > 0) ret = read(TIME_fdWake[0], readme, sizeof(readme));
    }
    TRACE("Exiting main winmm thread\n");
    return 0;
}

/**************************************************************************
 * 				TIME_MMTimeStart
 */
void	TIME_MMTimeStart(void)
{
    if (!TIME_hMMTimer) {
        TIME_TimersList = NULL;
        if (pipe(TIME_fdWake) < 0)
        {
            TIME_fdWake[0] = TIME_fdWake[1] = -1;
            ERR("Cannot create pipe: %s\n", strerror(errno));
        } else {
            fcntl(TIME_fdWake[0], F_SETFL, O_NONBLOCK);
            fcntl(TIME_fdWake[1], F_SETFL, O_NONBLOCK);
        }
        TIME_TimeToDie = FALSE;
        TIME_hMMTimer = CreateThread(NULL, 0, TIME_MMSysTimeThread, &WINMM_IData, 0, NULL);
        SetThreadPriority(TIME_hMMTimer, THREAD_PRIORITY_TIME_CRITICAL);
        InitializeCriticalSection(&TIME_cbcrst);
        TIME_cbcrst.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": WINMM.TIME_cbcrst");
    }
}

/**************************************************************************
 * 				TIME_MMTimeStop
 */
void	TIME_MMTimeStop(void)
{
    if (TIME_hMMTimer) {
        const char a='a';

        TIME_TimeToDie = TRUE;
        write(TIME_fdWake[1], &a, sizeof(a));

        WaitForSingleObject(TIME_hMMTimer, INFINITE);
        close(TIME_fdWake[0]);
        close(TIME_fdWake[1]);
        TIME_fdWake[0] = TIME_fdWake[1] = -1;
        CloseHandle(TIME_hMMTimer);
        TIME_hMMTimer = 0;
        TIME_cbcrst.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&TIME_cbcrst);
        TIME_TimersList = NULL;
    }
}

/**************************************************************************
 * 				timeGetSystemTime	[WINMM.@]
 */
MMRESULT WINAPI timeGetSystemTime(LPMMTIME lpTime, UINT wSize)
{

    if (wSize >= sizeof(*lpTime)) {
	lpTime->wType = TIME_MS;
	lpTime->u.ms = GetTickCount();

    }

    return 0;
}

/**************************************************************************
 * 				TIME_SetEventInternal	[internal]
 */
WORD	TIME_SetEventInternal(UINT wDelay, UINT wResol,
                              LPTIMECALLBACK lpFunc, DWORD dwUser, UINT wFlags)
{
    WORD 		wNewID = 0;
    LPWINE_TIMERENTRY	lpNewTimer;
    LPWINE_TIMERENTRY	lpTimer;
    const char c = 'c';

    TRACE("(%u, %u, %p, %08X, %04X);\n", wDelay, wResol, lpFunc, dwUser, wFlags);

    if (wDelay < MMSYSTIME_MININTERVAL || wDelay > MMSYSTIME_MAXINTERVAL)
	return 0;

    lpNewTimer = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_TIMERENTRY));
    if (lpNewTimer == NULL)
	return 0;

    TIME_MMTimeStart();

    lpNewTimer->wDelay = wDelay;
    lpNewTimer->dwTriggerTime = GetTickCount() + wDelay;

    /* FIXME - wResol is not respected, although it is not clear
               that we could change our precision meaningfully  */
    lpNewTimer->wResol = wResol;
    lpNewTimer->lpFunc = lpFunc;
    lpNewTimer->dwUser = dwUser;
    lpNewTimer->wFlags = wFlags;

    EnterCriticalSection(&WINMM_IData.cs);

    for (lpTimer = TIME_TimersList; lpTimer != NULL; lpTimer = lpTimer->lpNext) {
	wNewID = max(wNewID, lpTimer->wTimerID);
    }

    lpNewTimer->lpNext = TIME_TimersList;
    TIME_TimersList = lpNewTimer;
    lpNewTimer->wTimerID = wNewID + 1;

    LeaveCriticalSection(&WINMM_IData.cs);

    /* Wake the service thread in case there is work to be done */
    write(TIME_fdWake[1], &c, sizeof(c));

    TRACE("=> %u\n", wNewID + 1);

    return wNewID + 1;
}

/**************************************************************************
 * 				timeSetEvent		[WINMM.@]
 */
MMRESULT WINAPI timeSetEvent(UINT wDelay, UINT wResol, LPTIMECALLBACK lpFunc,
                            DWORD_PTR dwUser, UINT wFlags)
{
    if (wFlags & WINE_TIMER_IS32)
	WARN("Unknown windows flag... wine internally used.. ooch\n");

    return TIME_SetEventInternal(wDelay, wResol, lpFunc,
                                 dwUser, wFlags|WINE_TIMER_IS32);
}

/**************************************************************************
 * 				timeKillEvent		[WINMM.@]
 */
MMRESULT WINAPI timeKillEvent(UINT wID)
{
    LPWINE_TIMERENTRY   lpSelf = NULL, *lpTimer;

    TRACE("(%u)\n", wID);
    EnterCriticalSection(&WINMM_IData.cs);
    /* remove WINE_TIMERENTRY from list */
    for (lpTimer = &TIME_TimersList; *lpTimer; lpTimer = &(*lpTimer)->lpNext) {
	if (wID == (*lpTimer)->wTimerID) {
            lpSelf = *lpTimer;
            /* unlink timer of id 'wID' */
            *lpTimer = (*lpTimer)->lpNext;
	    break;
	}
    }
    LeaveCriticalSection(&WINMM_IData.cs);

    if (!lpSelf)
    {
        WARN("wID=%u is not a valid timer ID\n", wID);
        return MMSYSERR_INVALPARAM;
    }
    if (lpSelf->wFlags & TIME_KILL_SYNCHRONOUS)
        EnterCriticalSection(&TIME_cbcrst);
    HeapFree(GetProcessHeap(), 0, lpSelf);
    if (lpSelf->wFlags & TIME_KILL_SYNCHRONOUS)
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

/**************************************************************************
 * 				timeGetTime    [MMSYSTEM.607]
 * 				timeGetTime    [WINMM.@]
 */
DWORD WINAPI timeGetTime(void)
{
#if defined(COMMENTOUTPRIORTODELETING)
    DWORD       count;

    /* FIXME: releasing the win16 lock here is a temporary hack (I hope)
     * that lets mciavi32.dll run correctly
     */
    if (pFnReleaseThunkLock) pFnReleaseThunkLock(&count);
    if (pFnRestoreThunkLock) pFnRestoreThunkLock(count);
#endif

    return GetTickCount();
}
