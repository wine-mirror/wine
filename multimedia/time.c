/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MMSYTEM time functions
 *
 * Copyright 1993 Martin Ayotte
 */

#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "winbase.h"
#include "wine/winbase16.h" /* GetTaskDS */
#include "callback.h"
#include "mmsystem.h"
#include "services.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mmtime)

typedef struct tagTIMERENTRY {
    UINT			wDelay;
    UINT			wResol;
    FARPROC16 			lpFunc;
    HINSTANCE			hInstance;
    DWORD			dwUser;
    UINT			wFlags;
    UINT			wTimerID;
    UINT			wCurTime;
    UINT			isWin32;
    struct tagTIMERENTRY*	lpNext;
} TIMERENTRY, *LPTIMERENTRY;

static DWORD		mmSysTimeMS;
static LPTIMERENTRY 	lpTimerList = NULL;
static HANDLE 		hMMTimer = 0;
CRITICAL_SECTION	mmTimeCritSect;

/*
 * FIXME
 * We're using "1" as the mininum resolution to the timer,
 * as Windows 95 does, according to the docs. Maybe it should
 * depend on the computers resources!
 */
#define MMSYSTIME_MININTERVAL /* (1) */ (10)
#define MMSYSTIME_MAXINTERVAL (65535)

static	void	TIME_TriggerCallBack(LPTIMERENTRY lpTimer, DWORD dwCurrent)
{
    if (lpTimer->lpFunc != (FARPROC16) NULL) {
	TRACE("before CallBack (%lu)!\n", dwCurrent);
	TRACE("lpFunc=%p wTimerID=%04X dwUser=%08lX !\n",
	      lpTimer->lpFunc, lpTimer->wTimerID, lpTimer->dwUser);
	TRACE("hInstance=%04X !\n", lpTimer->hInstance);	
	
	/* - TimeProc callback that is called here is something strange, under Windows 3.1x it is called 
	 * 		during interrupt time,  is allowed to execute very limited number of API calls (like
	 *	    	PostMessage), and must reside in DLL (therefore uses stack of active application). So I 
	 *       guess current implementation via SetTimer has to be improved upon.		
	 */
	switch (lpTimer->wFlags & 0x30) {
	case TIME_CALLBACK_FUNCTION:
	    if (lpTimer->isWin32)
		((LPTIMECALLBACK)lpTimer->lpFunc)(lpTimer->wTimerID, 0, lpTimer->dwUser, 0, 0);
	    else
		Callbacks->CallTimeFuncProc(lpTimer->lpFunc,
					    lpTimer->wTimerID, 0,
					    lpTimer->dwUser, 0, 0);
	    break;
	case TIME_CALLBACK_EVENT_SET:
	    SetEvent((HANDLE)lpTimer->lpFunc);
	    break;
	case TIME_CALLBACK_EVENT_PULSE:
	    PulseEvent((HANDLE)lpTimer->lpFunc);
	    break;
	default:
	    FIXME("Unknown callback type 0x%04x for mmtime callback (%p), ignored.\n", lpTimer->wFlags, lpTimer->lpFunc);
	    break;
	}
	TRACE("after CallBack !\n");
    }
}

/**************************************************************************
 *           TIME_MMSysTimeCallback
 */
static void CALLBACK TIME_MMSysTimeCallback( ULONG_PTR dummy )
{
    LPTIMERENTRY lpTimer, lpNextTimer;

    mmSysTimeMS += MMSYSTIME_MININTERVAL;

    EnterCriticalSection(&mmTimeCritSect);

    for (lpTimer = lpTimerList; lpTimer != NULL; ) {
	lpNextTimer = lpTimer->lpNext;
	if (lpTimer->wCurTime < MMSYSTIME_MININTERVAL) {
	    lpTimer->wCurTime = lpTimer->wDelay;    
	    TIME_TriggerCallBack(lpTimer, mmSysTimeMS);
	    if (lpTimer->wFlags & TIME_ONESHOT)
		timeKillEvent(lpTimer->wTimerID);
	} else {
	    lpTimer->wCurTime -= MMSYSTIME_MININTERVAL;
	}
	lpTimer = lpNextTimer;
    }

    LeaveCriticalSection(&mmTimeCritSect);
}

/**************************************************************************
 * 				MULTIMEDIA_MMTimeInit		[internal]
 */
BOOL	MULTIMEDIA_MMTimeInit(void)
{
    InitializeCriticalSection(&mmTimeCritSect);
    MakeCriticalSectionGlobal(&mmTimeCritSect);
    
    return TRUE;
}

/**************************************************************************
 * 				MULTIMEDIA_MMTimeStart		[internal]
 */
static	BOOL	MULTIMEDIA_MMTimeStart(void)
{
    /* one could think it's possible to stop the service thread activity when no more
     * mm timers are active, but this would require to handle delta (in time) between
     * GetTickCount() and timeGetTime() which are not -currently- synchronized.
     */
    if (!hMMTimer) {
	mmSysTimeMS = GetTickCount();
	hMMTimer = SERVICE_AddTimer( MMSYSTIME_MININTERVAL*1000L, TIME_MMSysTimeCallback, 0 );
    }

    return TRUE;
}

/**************************************************************************
 * 				timeGetSystemTime	[WINMM.140]
 */
MMRESULT WINAPI timeGetSystemTime(LPMMTIME lpTime, UINT wSize)
{
    TRACE("(%p, %u);\n", lpTime, wSize);

    if (wSize >= sizeof(*lpTime)) {
	MULTIMEDIA_MMTimeStart();
	lpTime->wType = TIME_MS;
	lpTime->u.ms = mmSysTimeMS;

	TRACE("=> %lu\n", mmSysTimeMS);
    }

    return 0;
}

/**************************************************************************
 * 				timeGetSystemTime	[MMSYSTEM.601]
 */
MMRESULT16 WINAPI timeGetSystemTime16(LPMMTIME16 lpTime, UINT16 wSize)
{
    TRACE("(%p, %u);\n", lpTime, wSize);

    if (wSize >= sizeof(*lpTime)) {
	MULTIMEDIA_MMTimeStart();
	lpTime->wType = TIME_MS;
	lpTime->u.ms = mmSysTimeMS;

	TRACE("=> %lu\n", mmSysTimeMS);
    }

    return 0;
}

/**************************************************************************
 * 				timeSetEventInternal	[internal]
 */
static	WORD	timeSetEventInternal(UINT wDelay, UINT wResol,
				     FARPROC16 lpFunc, DWORD dwUser,
				     UINT wFlags, UINT16 isWin32)
{
    WORD 		wNewID = 0;
    LPTIMERENTRY	lpNewTimer;
    LPTIMERENTRY	lpTimer;
    
    TRACE("(%u, %u, %p, %08lX, %04X);\n", wDelay, wResol, lpFunc, dwUser, wFlags);

    lpNewTimer = (LPTIMERENTRY)malloc(sizeof(TIMERENTRY));
    if (lpNewTimer == NULL)
	return 0;

    MULTIMEDIA_MMTimeStart();

    lpNewTimer->wCurTime = wDelay;
    lpNewTimer->wDelay = wDelay;
    lpNewTimer->wResol = wResol;
    lpNewTimer->lpFunc = lpFunc;
    lpNewTimer->isWin32 = isWin32;
    lpNewTimer->hInstance = GetTaskDS16();
    lpNewTimer->dwUser = dwUser;
    lpNewTimer->wFlags = wFlags;

    TRACE("hInstance=%04X,lpFunc=0x%08lx !\n", lpNewTimer->hInstance, (DWORD)lpFunc);

    EnterCriticalSection(&mmTimeCritSect);

    for (lpTimer = lpTimerList; lpTimer != NULL; lpTimer = lpTimer->lpNext) {
	wNewID = MAX(wNewID, lpTimer->wTimerID);
    }
    
    lpNewTimer->lpNext = lpTimerList;
    lpTimerList = lpNewTimer;
    lpNewTimer->wTimerID = wNewID + 1;

    LeaveCriticalSection(&mmTimeCritSect);

    return wNewID + 1;
}

/**************************************************************************
 * 				timeSetEvent		[MMSYSTEM.602]
 */
MMRESULT WINAPI timeSetEvent(UINT wDelay, UINT wResol, LPTIMECALLBACK lpFunc,
			     DWORD dwUser, UINT wFlags)
{
    return timeSetEventInternal(wDelay, wResol, (FARPROC16)lpFunc, dwUser, wFlags, 1);
}

/**************************************************************************
 * 				timeSetEvent		[MMSYSTEM.602]
 */
MMRESULT16 WINAPI timeSetEvent16(UINT16 wDelay, UINT16 wResol, LPTIMECALLBACK16 lpFunc, 
				 DWORD dwUser, UINT16 wFlags)
{
    return timeSetEventInternal(wDelay, wResol, (FARPROC16)lpFunc, dwUser, wFlags, 0);
}

/**************************************************************************
 * 				timeKillEvent		[WINMM.142]
 */
MMRESULT WINAPI timeKillEvent(UINT wID)
{
    LPTIMERENTRY*	lpTimer;
    MMRESULT		ret = MMSYSERR_INVALPARAM;

    EnterCriticalSection(&mmTimeCritSect);

    for (lpTimer = &lpTimerList; *lpTimer; lpTimer = &((*lpTimer)->lpNext)) {
	if (wID == (*lpTimer)->wTimerID) {
	    LPTIMERENTRY xlptimer = (*lpTimer)->lpNext;
	    
	    free(*lpTimer);
	    *lpTimer = xlptimer;
	    ret = TIMERR_NOERROR;
	    break;
	}
    }

    LeaveCriticalSection(&mmTimeCritSect);

    return ret;
}

/**************************************************************************
 * 				timeKillEvent		[MMSYSTEM.603]
 */
MMRESULT16 WINAPI timeKillEvent16(UINT16 wID)
{
    return timeKillEvent(wID);
}

/**************************************************************************
 * 				timeGetDevCaps		[WINMM.139]
 */
MMRESULT WINAPI timeGetDevCaps(LPTIMECAPS lpCaps, UINT wSize)
{
    TRACE("(%p, %u) !\n", lpCaps, wSize);

    lpCaps->wPeriodMin = MMSYSTIME_MININTERVAL;
    lpCaps->wPeriodMax = MMSYSTIME_MAXINTERVAL;
    return 0;
}

/**************************************************************************
 * 				timeGetDevCaps		[MMSYSTEM.604]
 */
MMRESULT16 WINAPI timeGetDevCaps16(LPTIMECAPS16 lpCaps, UINT16 wSize)
{
    TRACE("(%p, %u) !\n", lpCaps, wSize);

    lpCaps->wPeriodMin = MMSYSTIME_MININTERVAL;
    lpCaps->wPeriodMax = MMSYSTIME_MAXINTERVAL;
    return 0;
}

/**************************************************************************
 * 				timeBeginPeriod		[WINMM.137]
 */
MMRESULT WINAPI timeBeginPeriod(UINT wPeriod)
{
    TRACE("(%u) !\n", wPeriod);

    if (wPeriod < MMSYSTIME_MININTERVAL || wPeriod > MMSYSTIME_MAXINTERVAL) 
	return TIMERR_NOCANDO;
    return 0;
}

/**************************************************************************
 * 				timeBeginPeriod16	[MMSYSTEM.605]
 */
MMRESULT16 WINAPI timeBeginPeriod16(UINT16 wPeriod)
{
    TRACE("(%u) !\n", wPeriod);

    if (wPeriod < MMSYSTIME_MININTERVAL || wPeriod > MMSYSTIME_MAXINTERVAL) 
	return TIMERR_NOCANDO;
    return 0;
}

/**************************************************************************
 * 				timeEndPeriod		[WINMM.138]
 */
MMRESULT WINAPI timeEndPeriod(UINT wPeriod)
{
    TRACE("(%u) !\n", wPeriod);

    if (wPeriod < MMSYSTIME_MININTERVAL || wPeriod > MMSYSTIME_MAXINTERVAL) 
	return TIMERR_NOCANDO;
    return 0;
}

/**************************************************************************
 * 				timeEndPeriod16		[MMSYSTEM.606]
 */
MMRESULT16 WINAPI timeEndPeriod16(UINT16 wPeriod)
{
    TRACE("(%u) !\n", wPeriod);

    if (wPeriod < MMSYSTIME_MININTERVAL || wPeriod > MMSYSTIME_MAXINTERVAL) 
	return TIMERR_NOCANDO;
    return 0;
}

/**************************************************************************
 * 				timeGetTime    [MMSYSTEM.607][WINMM.141]
 */
DWORD WINAPI timeGetTime(void)
{
    MULTIMEDIA_MMTimeStart();
    return mmSysTimeMS;
}
