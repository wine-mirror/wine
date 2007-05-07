/*
 * Misc Toolhelp functions
 *
 * Copyright 1996 Marcus Meissner
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <ctype.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "winternl.h"

#include "wine/winbase16.h"
#include "toolhelp.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(toolhelp);

/* FIXME: to make this work, we have to call back all these registered
 * functions from all over the WINE code. Someone with more knowledge than
 * me please do that. -Marcus
 */
 
static struct notify
{
    HTASK16   htask;
    FARPROC16 lpfnCallback;
    WORD     wFlags;
} *notifys = NULL;

static int nrofnotifys = 0;

static FARPROC16 HookNotify = NULL;


/***********************************************************************
 *           TaskFindHandle   (TOOLHELP.65)
 */
BOOL16 WINAPI TaskFindHandle16( TASKENTRY *lpte, HTASK16 hTask )
{
    lpte->hNext = hTask;
    return TaskNext16( lpte );
}


/***********************************************************************
 *		NotifyRegister (TOOLHELP.73)
 */
BOOL16 WINAPI NotifyRegister16( HTASK16 htask, FARPROC16 lpfnCallback,
                              WORD wFlags )
{
    int	i;

    FIXME("(%x,%x,%x), semi-stub.\n",
                      htask, (DWORD)lpfnCallback, wFlags );
    if (!htask) htask = GetCurrentTask();
    for (i=0;i<nrofnotifys;i++)
        if (notifys[i].htask==htask)
            break;
    if (i==nrofnotifys) {
        if (notifys==NULL)
            notifys=HeapAlloc( GetProcessHeap(), 0,
                                               sizeof(struct notify) );
        else
            notifys=HeapReAlloc( GetProcessHeap(), 0, notifys,
                                        sizeof(struct notify)*(nrofnotifys+1));
        if (!notifys) return FALSE;
        nrofnotifys++;
    }
    notifys[i].htask=htask;
    notifys[i].lpfnCallback=lpfnCallback;
    notifys[i].wFlags=wFlags;
    return TRUE;
}

/***********************************************************************
 *		NotifyUnregister (TOOLHELP.74)
 */
BOOL16 WINAPI NotifyUnregister16( HTASK16 htask )
{
    int	i;

    FIXME("(%x), semi-stub.\n", htask );
    if (!htask) htask = GetCurrentTask();
    for (i=nrofnotifys;i--;)
        if (notifys[i].htask==htask)
            break;
    if (i==-1)
        return FALSE;
    memcpy(notifys+i,notifys+(i+1),sizeof(struct notify)*(nrofnotifys-i-1));
    notifys=HeapReAlloc( GetProcessHeap(), 0, notifys,
                                        (nrofnotifys-1)*sizeof(struct notify));
    nrofnotifys--;
    return TRUE;
}

/***********************************************************************
 *		StackTraceCSIPFirst (TOOLHELP.67)
 */
BOOL16 WINAPI StackTraceCSIPFirst16(STACKTRACEENTRY *ste, WORD wSS, WORD wCS, WORD wIP, WORD wBP)
{
    FIXME("(%p, ss %04x, cs %04x, ip %04x, bp %04x): stub.\n", ste, wSS, wCS, wIP, wBP);
    return TRUE;
}

/***********************************************************************
 *		StackTraceFirst (TOOLHELP.66)
 */
BOOL16 WINAPI StackTraceFirst16(STACKTRACEENTRY *ste, HTASK16 Task)
{
    FIXME("(%p, %04x), stub.\n", ste, Task);
    return TRUE;
}

/***********************************************************************
 *		StackTraceNext (TOOLHELP.68)
 */
BOOL16 WINAPI StackTraceNext16(STACKTRACEENTRY *ste)
{
    FIXME("(%p), stub.\n", ste);
    return TRUE;
}

/***********************************************************************
 *		InterruptRegister (TOOLHELP.75)
 */
BOOL16 WINAPI InterruptRegister16( HTASK16 task, FARPROC callback )
{
    FIXME("(%04x, %p), stub.\n", task, callback);
    return TRUE;
}

/***********************************************************************
 *		InterruptUnRegister (TOOLHELP.76)
 */
BOOL16 WINAPI InterruptUnRegister16( HTASK16 task )
{
    FIXME("(%04x), stub.\n", task);
    return TRUE;
}

/***********************************************************************
 *           TimerCount   (TOOLHELP.80)
 */
BOOL16 WINAPI TimerCount16( TIMERINFO *pTimerInfo )
{
    /* FIXME
     * In standard mode, dwmsSinceStart = dwmsThisVM
     *
     * I tested this, under Windows in enhanced mode, and
     * if you never switch VM (ie start/stop DOS) these
     * values should be the same as well.
     *
     * Also, Wine should adjust for the hardware timer
     * to reduce the amount of error to ~1ms.
     * I can't be bothered, can you?
     */
    pTimerInfo->dwmsSinceStart = pTimerInfo->dwmsThisVM = GetTickCount();
    return TRUE;
}

/***********************************************************************
 *           SystemHeapInfo   (TOOLHELP.71)
 */
BOOL16 WINAPI SystemHeapInfo16( SYSHEAPINFO *pHeapInfo )
{
    STACK16FRAME* stack16 = MapSL((SEGPTR)NtCurrentTeb()->WOW32Reserved);
    HANDLE16 oldDS = stack16->ds;
    WORD user = LoadLibrary16( "USER.EXE" );
    WORD gdi = LoadLibrary16( "GDI.EXE" );
    stack16->ds = user;
    pHeapInfo->wUserFreePercent = (int)LocalCountFree16() * 100 / LocalHeapSize16();
    stack16->ds = gdi;
    pHeapInfo->wGDIFreePercent  = (int)LocalCountFree16() * 100 / LocalHeapSize16();
    stack16->ds = oldDS;
    pHeapInfo->hUserSegment = user;
    pHeapInfo->hGDISegment  = gdi;
    FreeLibrary16( user );
    FreeLibrary16( gdi );
    return TRUE;
}


/***********************************************************************
 *           ToolHelpHook                             (KERNEL.341)
 *	see "Undocumented Windows"
 */
FARPROC16 WINAPI ToolHelpHook16(FARPROC16 lpfnNotifyHandler)
{
	FARPROC16 tmp;

	FIXME("(%p), stub.\n", lpfnNotifyHandler);
	tmp = HookNotify;
	HookNotify = lpfnNotifyHandler;
	/* just return previously installed notification function */
	return tmp;
}
