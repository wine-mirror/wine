/*
 * MMSYTEM time functions
 *
 * Copyright 1993 Martin Ayotte
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static BOOL32 mmTimeStarted = FALSE;
static MMTIME16 mmSysTimeMS;
static MMTIME16 mmSysTimeSMPTE;

/* this is used to avoid infinite loop in timeGetTime, because
   of the faked multimedia timers, and will desappear as soon as
   the timers are implemented correctly.
 */
static int time_called=0;

typedef struct tagTIMERENTRY {
    UINT32	wDelay;
    UINT32	wResol;
    FARPROC16 lpFunc;
    HINSTANCE32	hInstance;
    DWORD	dwUser;
    UINT32	wFlags;
    UINT32	wTimerID;
    UINT32	wCurTime;
    UINT32	iswin32;
    struct tagTIMERENTRY *Next;
    DWORD	triggertime;
} TIMERENTRY, *LPTIMERENTRY;

static LPTIMERENTRY lpTimerList = NULL;

/*
 * FIXME
 * We're using "1" as the mininum resolution to the timer,
 * as Windows 95 does, according to the docs. Maybe it should
 * depend on the computers resources!
 */
#define MMSYSTIME_MININTERVAL (1)
#define MMSYSTIME_MAXINTERVAL (65535)


/**************************************************************************
 *           check_MMtimers
 */
static VOID check_MMtimers()
{
    LPTIMERENTRY lpTimer = lpTimerList;
    DWORD	curtick = GetTickCount();

    while (lpTimer != NULL) {
    	if (lpTimer->triggertime <= curtick) {
	    lpTimer->wCurTime = lpTimer->wDelay;

	    if (lpTimer->lpFunc != (FARPROC16) NULL) {
		TRACE(mmtime, "before CallBack16 !\n");
		TRACE(mmtime, "lpFunc=%p wTimerID=%04X dwUser=%08lX !\n",
			lpTimer->lpFunc, lpTimer->wTimerID, lpTimer->dwUser);
		TRACE(mmtime, "hInstance=%04X !\n", lpTimer->hInstance);


/*        - TimeProc callback that is called here is something strange, under Windows 3.1x it is called 
 *          during interrupt time,  is allowed to execute very limited number of API calls (like
 *	    PostMessage), and must reside in DLL (therefore uses stack of active application). So I 
 *          guess current implementation via SetTimer has to be improved upon.		
 */
 		if (lpTimer->iswin32)
			lpTimer->lpFunc(lpTimer->wTimerID,0,lpTimer->dwUser,0,0);
		else
			Callbacks->CallTimeFuncProc(lpTimer->lpFunc,
						    lpTimer->wTimerID,0,
						    lpTimer->dwUser,0,0
			);

		TRACE(mmtime, "after CallBack16 !\n");
	    }
	    if (lpTimer->wFlags & TIME_ONESHOT)
		timeKillEvent32(lpTimer->wTimerID);
	}
	lpTimer = lpTimer->Next;
    }
}

/**************************************************************************
 *           TIME_MMSysTimeCallback
 */
static VOID TIME_MMSysTimeCallback( HWND32 hwnd, UINT32 msg,
                                    UINT32 id, DWORD dwTime )
{
    LPTIMERENTRY lpTimer = lpTimerList;
    mmSysTimeMS.u.ms += MMSYSTIME_MININTERVAL;
    mmSysTimeSMPTE.u.smpte.frame++;
    while (lpTimer != NULL) {
	lpTimer->wCurTime--;
	if (lpTimer->wCurTime == 0) {
	    lpTimer->wCurTime = lpTimer->wDelay;

	    if (lpTimer->lpFunc != (FARPROC16) NULL) {
		TRACE(mmtime, "before CallBack16 !\n");
		TRACE(mmtime, "lpFunc=%p wTimerID=%04X dwUser=%08lX !\n",
			lpTimer->lpFunc, lpTimer->wTimerID, lpTimer->dwUser);
		TRACE(mmtime, "hInstance=%04X !\n", lpTimer->hInstance);

/* This is wrong (lpFunc is NULL all the time)

   	        lpFunc = MODULE_GetEntryPoint( lpTimer->hInstance,
                         MODULE_GetOrdinal(lpTimer->hInstance,"TimerCallBack" ));
		TRACE(mmtime, "lpFunc=%08lx !\n", lpFunc);
*/


/*        - TimeProc callback that is called here is something strange, under Windows 3.1x it is called 
 *          during interrupt time,  is allowed to execute very limited number of API calls (like
 *	    PostMessage), and must reside in DLL (therefore uses stack of active application). So I 
 *          guess current implementation via SetTimer has to be improved upon.		
 */
 		if (lpTimer->iswin32)
			lpTimer->lpFunc(lpTimer->wTimerID,0,lpTimer->dwUser,0,0);
		else
			Callbacks->CallTimeFuncProc(lpTimer->lpFunc,
						    lpTimer->wTimerID,0,
						    lpTimer->dwUser,0,0
			);

		TRACE(mmtime, "after CallBack16 !\n");
		fflush(stdout);
	    }
	    if (lpTimer->wFlags & TIME_ONESHOT)
		timeKillEvent32(lpTimer->wTimerID);
	}
	lpTimer = lpTimer->Next;
    }
}

/**************************************************************************
 * 				StartMMTime			[internal]
 */
static void StartMMTime()
{
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
	SetTimer32( 0, 1, MMSYSTIME_MININTERVAL, TIME_MMSysTimeCallback );
    }
}

/**************************************************************************
 * 				timeGetSystemTime	[WINMM.140]
 */
MMRESULT32 WINAPI timeGetSystemTime32(LPMMTIME32 lpTime, UINT32 wSize)
{
    TRACE(mmsys, "(%p, %u);\n", lpTime, wSize);
    if (!mmTimeStarted)
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
    if (!mmTimeStarted)
	StartMMTime();
    lpTime->wType = TIME_MS;
    lpTime->u.ms = mmSysTimeMS.u.ms;
    return 0;
}

/**************************************************************************
 * 				timeSetEvent		[MMSYSTEM.602]
 */
MMRESULT32 WINAPI timeSetEvent32(UINT32 wDelay,UINT32 wResol,
				 LPTIMECALLBACK32 lpFunc,DWORD dwUser,
				 UINT32 wFlags)
{
    WORD wNewID = 0;
    LPTIMERENTRY lpNewTimer;
    LPTIMERENTRY lpTimer = lpTimerList;

    TRACE(mmtime, "(%u, %u, %p, %08lX, %04X);\n",
		  wDelay, wResol, lpFunc, dwUser, wFlags);
    if (!mmTimeStarted)
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
    lpNewTimer->triggertime = wDelay+GetTickCount();
    lpNewTimer->wDelay = wDelay;
    lpNewTimer->wResol = wResol;
    lpNewTimer->lpFunc = (FARPROC16) lpFunc;
    lpNewTimer->iswin32 = 1;
    lpNewTimer->hInstance = GetTaskDS();
	TRACE(mmtime, "hInstance=%04X !\n", lpNewTimer->hInstance);
	TRACE(mmtime, "lpFunc=%p !\n", 
				lpFunc);
    lpNewTimer->dwUser = dwUser;
    lpNewTimer->wFlags = wFlags;
    return lpNewTimer->wTimerID;
}

/**************************************************************************
 * 				timeSetEvent		[MMSYSTEM.602]
 */
MMRESULT16 WINAPI timeSetEvent16(UINT16 wDelay, UINT16 wResol,
				 LPTIMECALLBACK16 lpFunc,DWORD dwUser,
				 UINT16 wFlags)
{
    WORD wNewID = 0;
    LPTIMERENTRY lpNewTimer;
    LPTIMERENTRY lpTimer = lpTimerList;
    TRACE(mmtime, "(%u, %u, %p, %08lX, %04X);\n",
		  wDelay, wResol, lpFunc, dwUser, wFlags);
    if (!mmTimeStarted)
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
    lpNewTimer->triggertime = wDelay+GetTickCount();
    lpNewTimer->wResol = wResol;
    lpNewTimer->lpFunc = (FARPROC16) lpFunc;
    lpNewTimer->iswin32 = 0;
    lpNewTimer->hInstance = GetTaskDS();
	TRACE(mmtime, "hInstance=%04X !\n", lpNewTimer->hInstance);
	TRACE(mmtime, "(lpFunc)=%p !\n", 
				PTR_SEG_TO_LIN(lpFunc));
    lpNewTimer->dwUser = dwUser;
    lpNewTimer->wFlags = wFlags;
    return lpNewTimer->wTimerID;
}

/**************************************************************************
 * 				timeKillEvent		[WINMM.142]
 */
MMRESULT32 WINAPI timeKillEvent32(UINT32 wID)
{
    LPTIMERENTRY xlptimer,*lpTimer = &lpTimerList;
    while (*lpTimer) {
	if (wID == (*lpTimer)->wTimerID) {
	    xlptimer = (*lpTimer)->Next;
	    free(*lpTimer);
	    *lpTimer = xlptimer;
	    return TRUE;
	}
	lpTimer = &((*lpTimer)->Next);
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
    if (!mmTimeStarted)
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
    if (!mmTimeStarted)
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
    if (!mmTimeStarted)
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
    if (!mmTimeStarted)
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
  /* FIXME all this function should have to be is
       long result;
       struct timeval time;
       TRACE(mmtime,"timeGetTime(); !\n");
       gettimeofday(&time, 0);
       result = (((long)time.tv_sec * (long)1000) + ((long)time.tv_usec / (long)1000));

     but the multimedia timers are not implemented correctly, so all the rest
     is a workaround to fake them.
       
   */  
    static DWORD lasttick=0;
    DWORD	newtick;

    TRACE(mmtime, "!\n");
    if (!mmTimeStarted)
	StartMMTime();
    newtick = GetTickCount();
    mmSysTimeMS.u.ms+=newtick-lasttick; /* FIXME: faked timer */
    if (newtick!=lasttick)

    if (!time_called) { /* to avoid infinite recursion if timeGetTime is called
			   inside check_MMtimers
			 */
      time_called++;
    	check_MMtimers();
      time_called--;
    }    //check_MMtimers();
    lasttick = newtick;
    TRACE(mmtime, "Time = %ld\n",mmSysTimeMS.u.ms);


    return mmSysTimeMS.u.ms;
}
