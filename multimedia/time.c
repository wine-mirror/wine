/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MMSYTEM time functions
 *
 * Copyright 1993 Martin Ayotte
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "windows.h"
#include "win.h"
#include "ldt.h"
#include "module.h"
#include "callback.h"
#include "user.h"
#include "driver.h"
#include "mmsystem.h"
#include "debug.h"
#include "xmalloc.h"
#include "options.h"

#define USE_FAKE_MM_TIMERS

static MMTIME16 mmSysTimeMS;
static MMTIME16 mmSysTimeSMPTE;

typedef struct tagTIMERENTRY {
    UINT32			wDelay;
    UINT32			wResol;
    FARPROC16 			lpFunc;
    HINSTANCE32			hInstance;
    DWORD			dwUser;
    UINT32			wFlags;
    UINT32			wTimerID;
    UINT32			wCurTime;
    UINT32			isWin32;
    struct tagTIMERENTRY*	Next;
} TIMERENTRY, *LPTIMERENTRY;

static LPTIMERENTRY lpTimerList = NULL;

#ifdef USE_FAKE_MM_TIMERS
static DWORD dwLastCBTick = 0;
static BOOL32 bUseFakeTimers = FALSE;
static WORD wInCallBackLoop = 0;
#endif

/*
 * FIXME
 * We're using "1" as the mininum resolution to the timer,
 * as Windows 95 does, according to the docs. Maybe it should
 * depend on the computers resources!
 */
#define MMSYSTIME_MININTERVAL (1)
#define MMSYSTIME_MAXINTERVAL (65535)

static	void	TIME_TriggerCallBack(LPTIMERENTRY lpTimer, DWORD dwCurrent)
{
    lpTimer->wCurTime = lpTimer->wDelay;
    
    if (lpTimer->lpFunc != (FARPROC16) NULL) {
	TRACE(mmtime, "before CallBack16 (%lu)!\n", dwCurrent);
	TRACE(mmtime, "lpFunc=%p wTimerID=%04X dwUser=%08lX !\n",
	      lpTimer->lpFunc, lpTimer->wTimerID, lpTimer->dwUser);
	TRACE(mmtime, "hInstance=%04X !\n", lpTimer->hInstance);
	
	
	/* - TimeProc callback that is called here is something strange, under Windows 3.1x it is called 
	 * 		during interrupt time,  is allowed to execute very limited number of API calls (like
	 *	    	PostMessage), and must reside in DLL (therefore uses stack of active application). So I 
	 *       guess current implementation via SetTimer has to be improved upon.		
	 */
	switch (lpTimer->wFlags & 0x30) {
	case TIME_CALLBACK_FUNCTION:
		if (lpTimer->isWin32)
		    lpTimer->lpFunc(lpTimer->wTimerID,0,lpTimer->dwUser,0,0);
		else
		    Callbacks->CallTimeFuncProc(lpTimer->lpFunc,
						lpTimer->wTimerID,0,
						lpTimer->dwUser,0,0);
		break;
	case TIME_CALLBACK_EVENT_SET:
		SetEvent((HANDLE32)lpTimer->lpFunc);
		break;
	case TIME_CALLBACK_EVENT_PULSE:
		PulseEvent((HANDLE32)lpTimer->lpFunc);
		break;
	default:
		FIXME(mmtime,"Unknown callback type 0x%04x for mmtime callback (%p),ignored.\n",lpTimer->wFlags,lpTimer->lpFunc);
		break;
	}
	TRACE(mmtime, "after CallBack16 !\n");
	fflush(stdout);
    }
    if (lpTimer->wFlags & TIME_ONESHOT)
	timeKillEvent32(lpTimer->wTimerID);
}

/**************************************************************************
 *           TIME_MMSysTimeCallback
 */
static VOID WINAPI TIME_MMSysTimeCallback( HWND32 hwnd, UINT32 msg,
                                    UINT32 id, DWORD dwTime )
{
    LPTIMERENTRY lpTimer;
    
    mmSysTimeMS.u.ms += MMSYSTIME_MININTERVAL;
    mmSysTimeSMPTE.u.smpte.frame++;
    
#ifdef USE_FAKE_MM_TIMERS
    if (bUseFakeTimers)
	dwLastCBTick = GetTickCount();

    if (!wInCallBackLoop++)
#endif
	for (lpTimer = lpTimerList; lpTimer != NULL; lpTimer = lpTimer->Next) {
	    if (lpTimer->wCurTime < MMSYSTIME_MININTERVAL) {
		TIME_TriggerCallBack(lpTimer, dwTime);			
	    } else {
		lpTimer->wCurTime -= MMSYSTIME_MININTERVAL;
	    }
	}
#ifdef USE_FAKE_MM_TIMERS
    wInCallBackLoop--;
#endif
}

/**************************************************************************
 * 				StartMMTime			[internal]
 */
static void StartMMTime()
{
    static BOOL32 	mmTimeStarted = FALSE;
    
    if (!mmTimeStarted) {
	mmTimeStarted = TRUE;
	mmSysTimeMS.wType = TIME_MS;
	mmSysTimeMS.u.ms = 0;
	mmSysTimeSMPTE.wType = TIME_SMPTE;
	mmSysTimeSMPTE.u.smpte.hour = 0;
	mmSysTimeSMPTE.u.smpte.min = 0;
	mmSysTimeSMPTE.u.smpte.sec = 0;
	mmSysTimeSMPTE.u.smpte.frame = 0;
	mmSysTimeSMPTE.u.smpte.fps = 0;
	mmSysTimeSMPTE.u.smpte.dummy = 0;
	SetTimer32( 0, 0, MMSYSTIME_MININTERVAL, TIME_MMSysTimeCallback );
#ifdef USE_FAKE_MM_TIMERS
	bUseFakeTimers = PROFILE_GetWineIniBool("options", "MMFakeTimers", TRUE);
	TRACE(mmtime, "FakeTimer=%c\n", bUseFakeTimers ? 'Y' : 'N');
	if (bUseFakeTimers)
	    dwLastCBTick = GetTickCount();
#endif
    }
}

/**************************************************************************
 * 				timeGetSystemTime	[WINMM.140]
 */
MMRESULT32 WINAPI timeGetSystemTime32(LPMMTIME32 lpTime, UINT32 wSize)
{
    TRACE(mmsys, "(%p, %u);\n", lpTime, wSize);
    StartMMTime();
    lpTime->wType = TIME_MS;
    lpTime->u.ms = mmSysTimeMS.u.ms;
    return 0;
}

/**************************************************************************
 * 				timeGetSystemTime	[MMSYSTEM.601]
 */
MMRESULT16 WINAPI timeGetSystemTime16(LPMMTIME16 lpTime, UINT16 wSize)
{
    TRACE(mmsys, "(%p, %u);\n", lpTime, wSize);
    StartMMTime();
    lpTime->wType = TIME_MS;
    lpTime->u.ms = mmSysTimeMS.u.ms;
    return 0;
}

static	WORD	timeSetEventInternal(UINT32 wDelay,UINT32 wResol,
				     FARPROC16 lpFunc,DWORD dwUser,
				     UINT32 wFlags, UINT16 isWin32)
{
    WORD 		wNewID = 0;
    LPTIMERENTRY	lpNewTimer;
    LPTIMERENTRY	lpTimer = lpTimerList;
    
    TRACE(mmtime, "(%u, %u, %p, %08lX, %04X);\n",
	  wDelay, wResol, lpFunc, dwUser, wFlags);
    StartMMTime();
    lpNewTimer = (LPTIMERENTRY)xmalloc(sizeof(TIMERENTRY));
    if (lpNewTimer == NULL)
	return 0;
    while (lpTimer != NULL) {
	wNewID = MAX(wNewID, lpTimer->wTimerID);
	lpTimer = lpTimer->Next;
    }
    
    lpNewTimer->Next = lpTimerList;
    lpTimerList = lpNewTimer;
    lpNewTimer->wTimerID = wNewID + 1;
    lpNewTimer->wCurTime = wDelay;
    lpNewTimer->wDelay = wDelay;
    lpNewTimer->wResol = wResol;
    lpNewTimer->lpFunc = lpFunc;
    lpNewTimer->isWin32 = isWin32;
    lpNewTimer->hInstance = GetTaskDS();
    TRACE(mmtime, "hInstance=%04X !\n", lpNewTimer->hInstance);
    TRACE(mmtime, "lpFunc=%p !\n", isWin32 ? lpFunc : PTR_SEG_TO_LIN(lpFunc));
    lpNewTimer->dwUser = dwUser;
    lpNewTimer->wFlags = wFlags;
    return lpNewTimer->wTimerID;
}

/**************************************************************************
 * 				timeSetEvent		[MMSYSTEM.602]
 */
MMRESULT32 WINAPI timeSetEvent32(UINT32 wDelay,UINT32 wResol,
				 LPTIMECALLBACK32 lpFunc,DWORD dwUser,
				 UINT32 wFlags)
{
    return timeSetEventInternal(wDelay, wResol, (FARPROC16)lpFunc, 
				dwUser, wFlags, 1);
}

/**************************************************************************
 * 				timeSetEvent		[MMSYSTEM.602]
 */
MMRESULT16 WINAPI timeSetEvent16(UINT16 wDelay, UINT16 wResol,
				 LPTIMECALLBACK16 lpFunc,DWORD dwUser,
				 UINT16 wFlags)
{
    return timeSetEventInternal(wDelay, wResol, (FARPROC16)lpFunc, 
				dwUser, wFlags, 0);
}

/**************************************************************************
 * 				timeKillEvent		[WINMM.142]
 */
MMRESULT32 WINAPI timeKillEvent32(UINT32 wID)
{
    LPTIMERENTRY*	lpTimer;
    
    for (lpTimer = &lpTimerList; *lpTimer; lpTimer = &((*lpTimer)->Next)) {
	if (wID == (*lpTimer)->wTimerID) {
	    LPTIMERENTRY xlptimer = (*lpTimer)->Next;
	    
	    free(*lpTimer);
	    *lpTimer = xlptimer;
	    return TRUE;
	}
    }
    return 0;
}

/**************************************************************************
 * 				timeKillEvent		[MMSYSTEM.603]
 */
MMRESULT16 WINAPI timeKillEvent16(UINT16 wID)
{
    return timeKillEvent32(wID);
}

/**************************************************************************
 * 				timeGetDevCaps		[WINMM.139]
 */
MMRESULT32 WINAPI timeGetDevCaps32(LPTIMECAPS32 lpCaps,UINT32 wSize)
{
    TRACE(mmtime, "(%p, %u) !\n", lpCaps, wSize);
    StartMMTime();
    lpCaps->wPeriodMin = MMSYSTIME_MININTERVAL;
    lpCaps->wPeriodMax = MMSYSTIME_MAXINTERVAL;
    return 0;
}

/**************************************************************************
 * 				timeGetDevCaps		[MMSYSTEM.604]
 */
MMRESULT16 WINAPI timeGetDevCaps16(LPTIMECAPS16 lpCaps, UINT16 wSize)
{
    TRACE(mmtime, "(%p, %u) !\n", lpCaps, wSize);
    StartMMTime();
    lpCaps->wPeriodMin = MMSYSTIME_MININTERVAL;
    lpCaps->wPeriodMax = MMSYSTIME_MAXINTERVAL;
    return 0;
}

/**************************************************************************
 * 				timeBeginPeriod		[WINMM.137]
 */
MMRESULT32 WINAPI timeBeginPeriod32(UINT32 wPeriod)
{
    TRACE(mmtime, "(%u) !\n", wPeriod);
    StartMMTime();
    if (wPeriod < MMSYSTIME_MININTERVAL || wPeriod > MMSYSTIME_MAXINTERVAL) 
	return TIMERR_NOCANDO;
    return 0;
}
/**************************************************************************
 * 				timeBeginPeriod		[MMSYSTEM.605]
 */
MMRESULT16 WINAPI timeBeginPeriod16(UINT16 wPeriod)
{
    TRACE(mmtime, "(%u) !\n", wPeriod);
    StartMMTime();
    if (wPeriod < MMSYSTIME_MININTERVAL || wPeriod > MMSYSTIME_MAXINTERVAL) 
	return TIMERR_NOCANDO;
    return 0;
}

/**************************************************************************
 * 				timeEndPeriod		[WINMM.138]
 */
MMRESULT32 WINAPI timeEndPeriod32(UINT32 wPeriod)
{
    TRACE(mmtime, "(%u) !\n", wPeriod);
    StartMMTime();
    if (wPeriod < MMSYSTIME_MININTERVAL || wPeriod > MMSYSTIME_MAXINTERVAL) 
	return TIMERR_NOCANDO;
    return 0;
}

/**************************************************************************
 * 				timeEndPeriod		[MMSYSTEM.606]
 */
MMRESULT16 WINAPI timeEndPeriod16(UINT16 wPeriod)
{
    TRACE(mmtime, "(%u) !\n", wPeriod);
    if (wPeriod < MMSYSTIME_MININTERVAL || wPeriod > MMSYSTIME_MAXINTERVAL) 
	return TIMERR_NOCANDO;
    return 0;
}

/**************************************************************************
 * 				timeGetTime    [MMSYSTEM.607][WINMM.141]
 */
DWORD WINAPI timeGetTime()
{
    DWORD	dwNewTick = GetTickCount();

    StartMMTime();
#ifdef USE_FAKE_MM_TIMERS
    if (bUseFakeTimers) {
	if (wInCallBackLoop++) { 
	    DWORD		dwDelta;
	    
	    
	    if (dwNewTick < dwLastCBTick) {
		ERR(mmtime, "dwNewTick(%lu) < dwLastCBTick(%lu)\n", dwNewTick, dwLastCBTick);
	    }
	    dwDelta = dwNewTick - dwLastCBTick;
	    if (dwDelta > MMSYSTIME_MININTERVAL) {
		LPTIMERENTRY lpTimer;
		
		mmSysTimeMS.u.ms += dwDelta; /* FIXME: faked timer */
		dwLastCBTick = dwNewTick;
		for (lpTimer = lpTimerList; lpTimer != NULL; lpTimer = lpTimer->Next) {
		    if (lpTimer->wCurTime < dwDelta) {
			TIME_TriggerCallBack(lpTimer, dwNewTick);
		    } else {
			lpTimer->wCurTime -= dwDelta;
		    }
		}
		dwNewTick = mmSysTimeMS.u.ms;
	    }
	}
	wInCallBackLoop--;
    }
#endif
    return dwNewTick;
}
