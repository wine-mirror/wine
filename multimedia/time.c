/*
 * MMSYTEM time functions
 *
 * Copyright 1993 Martin Ayotte
 */

#ifndef WINELIB

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
#include "stddebug.h"
#include "debug.h"

static BOOL mmTimeStarted = FALSE;
static MMTIME mmSysTimeMS;
static MMTIME mmSysTimeSMPTE;

typedef struct tagTIMERENTRY {
    WORD wDelay;
    WORD wResol;
    FARPROC16 lpFunc;
    HINSTANCE hInstance;
    DWORD dwUser;
    WORD wFlags;
    WORD wTimerID;
    WORD wCurTime;
    struct tagTIMERENTRY *Next;
    struct tagTIMERENTRY *Prev;
} TIMERENTRY, *LPTIMERENTRY;

static LPTIMERENTRY lpTimerList = NULL;

/**************************************************************************
 *           TIME_MMSysTimeCallback
 */
static VOID TIME_MMSysTimeCallback( HWND32 hwnd, UINT32 msg,
                                    UINT32 id, DWORD dwTime )
{
    LPTIMERENTRY lpTimer = lpTimerList;
    mmSysTimeMS.u.ms += 33;
    mmSysTimeSMPTE.u.smpte.frame++;
    while (lpTimer != NULL) {
	lpTimer->wCurTime--;
	if (lpTimer->wCurTime == 0) {
	    lpTimer->wCurTime = lpTimer->wDelay;
	    if (lpTimer->lpFunc != (FARPROC16) NULL) {
		dprintf_mmtime(stddeb, "MMSysTimeCallback // before CallBack16 !\n");
		dprintf_mmtime(stddeb, "MMSysTimeCallback // lpFunc=%p wTimerID=%04X dwUser=%08lX !\n",
			lpTimer->lpFunc, lpTimer->wTimerID, lpTimer->dwUser);
		dprintf_mmtime(stddeb, "MMSysTimeCallback // hInstance=%04X !\n", lpTimer->hInstance);

/* This is wrong (lpFunc is NULL all the time)

   	        lpFunc = MODULE_GetEntryPoint( lpTimer->hInstance,
                         MODULE_GetOrdinal(lpTimer->hInstance,"TimerCallBack" ));
		dprintf_mmtime(stddeb, "MMSysTimeCallback // lpFunc=%08lx !\n", lpFunc);
*/


/*        - TimeProc callback that is called here is something strange, under Windows 3.1x it is called 
 *          during interrupt time,  is allowed to execute very limited number of API calls (like
 *	    PostMessage), and must reside in DLL (therefore uses stack of active application). So I 
 *          guess current implementation via SetTimer has to be improved upon.		
 */

		CallTimeFuncProc(lpTimer->lpFunc, lpTimer->wTimerID, 
                                    0, lpTimer->dwUser, 0, 0);

		dprintf_mmtime(stddeb, "MMSysTimeCallback // after CallBack16 !\n");
		fflush(stdout);
	    }
	    if (lpTimer->wFlags & TIME_ONESHOT)
		timeKillEvent(lpTimer->wTimerID);
	}
	lpTimer = lpTimer->Next;
    }
}

/**************************************************************************
 * 				StartMMTime			[internal]
 */
void StartMMTime()
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
	SetTimer32( 0, 1, 33, TIME_MMSysTimeCallback );
    }
}

/**************************************************************************
 * 				timeGetSystemTime	[MMSYSTEM.601]
 */
WORD timeGetSystemTime(LPMMTIME lpTime, WORD wSize)
{
    dprintf_mmsys(stddeb, "timeGetSystemTime(%p, %u);\n", lpTime, wSize);
    if (!mmTimeStarted)
	StartMMTime();
    return 0;
}

/**************************************************************************
 * 				timeSetEvent		[MMSYSTEM.602]
 */
WORD timeSetEvent(WORD wDelay, WORD wResol, LPTIMECALLBACK lpFunc,
		  DWORD dwUser, WORD wFlags)
{
    WORD wNewID = 0;
    LPTIMERENTRY lpNewTimer;
    LPTIMERENTRY lpTimer = lpTimerList;
    dprintf_mmtime(stddeb, "timeSetEvent(%u, %u, %p, %08lX, %04X);\n",
		  wDelay, wResol, lpFunc, dwUser, wFlags);
    if (!mmTimeStarted)
	StartMMTime();
    lpNewTimer = (LPTIMERENTRY) malloc(sizeof(TIMERENTRY));
    if (lpNewTimer == NULL)
	return 0;
    while (lpTimer != NULL) {
	wNewID = MAX(wNewID, lpTimer->wTimerID);
	if (lpTimer->Next == NULL)
	    break;
	lpTimer = lpTimer->Next;
    }
    if (lpTimerList == NULL) {
	lpTimerList = lpNewTimer;
	lpNewTimer->Prev = NULL;
    } else {
	lpTimer->Next = lpNewTimer;
	lpNewTimer->Prev = lpTimer;
    }
    lpNewTimer->Next = NULL;
    lpNewTimer->wTimerID = wNewID + 1;
    lpNewTimer->wCurTime = wDelay;
    lpNewTimer->wDelay = wDelay;
    lpNewTimer->wResol = wResol;
    lpNewTimer->lpFunc = (FARPROC16) lpFunc;
    lpNewTimer->hInstance = GetTaskDS();
	dprintf_mmtime(stddeb, "timeSetEvent // hInstance=%04X !\n", lpNewTimer->hInstance);
	dprintf_mmtime(stddeb, "timeSetEvent // PTR_SEG_TO_LIN(lpFunc)=%p !\n", 
				PTR_SEG_TO_LIN(lpFunc));
    lpNewTimer->dwUser = dwUser;
    lpNewTimer->wFlags = wFlags;
    return lpNewTimer->wTimerID;
}

/**************************************************************************
 * 				timeKillEvent		[MMSYSTEM.603]
 */
WORD timeKillEvent(WORD wID)
{
    LPTIMERENTRY lpTimer = lpTimerList;
    while (lpTimer != NULL) {
	if (wID == lpTimer->wTimerID) {
	    if (lpTimer->Prev != NULL)
		lpTimer->Prev->Next = lpTimer->Next;
	    if (lpTimer->Next != NULL)
		lpTimer->Next->Prev = lpTimer->Prev;
	    free(lpTimer);
	    if (lpTimer==lpTimerList)
	    	lpTimerList=NULL;
	    return TRUE;
	}
	lpTimer = lpTimer->Next;
    }
    return 0;
}

/**************************************************************************
 * 				timeGetDevCaps		[MMSYSTEM.604]
 */
WORD timeGetDevCaps(LPTIMECAPS lpCaps, WORD wSize)
{
    dprintf_mmsys(stddeb, "timeGetDevCaps(%p, %u) !\n", lpCaps, wSize);
    return 0;
}

/**************************************************************************
 * 				timeBeginPeriod		[MMSYSTEM.605]
 */
WORD timeBeginPeriod(WORD wPeriod)
{
    dprintf_mmsys(stddeb, "timeBeginPeriod(%u) !\n", wPeriod);
    if (!mmTimeStarted)
	StartMMTime();
    return 0;
}

/**************************************************************************
 * 				timeEndPeriod		[MMSYSTEM.606]
 */
WORD timeEndPeriod(WORD wPeriod)
{
    dprintf_mmsys(stddeb, "timeEndPeriod(%u) !\n", wPeriod);
    return 0;
}

/**************************************************************************
 * 				timeGetTime    		[MMSYSTEM.607]
 */
DWORD timeGetTime()
{
    dprintf_mmsys(stddeb, "timeGetTime(); !\n");
    if (!mmTimeStarted)
	StartMMTime();
    return 0;
}

#endif /* WINELIB */
