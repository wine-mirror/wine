/*
 * Misc. USER functions
 *
 * Copyright 1993 Robert J. Amstadt
 *	     1996 Alex Korobka 
 */

#include <stdio.h>
#include <stdlib.h>
#include "windows.h"
#include "heap.h"
#include "gdi.h"
#include "user.h"
#include "task.h"
#include "queue.h"
#include "win.h"
#include "hook.h"
#include "debug.h"
#include "toolhelp.h"
#include "message.h"

WORD USER_HeapSel = 0;

extern BOOL32 MENU_SwitchTPWndTo(HTASK16);
extern void QUEUE_FlushMessages(HQUEUE16);

/***********************************************************************
 *           GetFreeSystemResources   (USER.284)
 */
WORD GetFreeSystemResources( WORD resType )
{
    int userPercent, gdiPercent;

    switch(resType)
    {
    case GFSR_USERRESOURCES:
        userPercent = (int)LOCAL_CountFree( USER_HeapSel ) * 100 /
                               LOCAL_HeapSize( USER_HeapSel );
        gdiPercent  = 100;
        break;

    case GFSR_GDIRESOURCES:
        gdiPercent  = (int)LOCAL_CountFree( GDI_HeapSel ) * 100 /
                               LOCAL_HeapSize( GDI_HeapSel );
        userPercent = 100;
        break;

    case GFSR_SYSTEMRESOURCES:
        userPercent = (int)LOCAL_CountFree( USER_HeapSel ) * 100 /
                               LOCAL_HeapSize( USER_HeapSel );
        gdiPercent  = (int)LOCAL_CountFree( GDI_HeapSel ) * 100 /
                               LOCAL_HeapSize( GDI_HeapSel );
        break;

    default:
        return 0;
    }
    return (WORD)MIN( userPercent, gdiPercent );
}


/***********************************************************************
 *           SystemHeapInfo   (TOOLHELP.71)
 */
BOOL16 SystemHeapInfo( SYSHEAPINFO *pHeapInfo )
{
    pHeapInfo->wUserFreePercent = GetFreeSystemResources( GFSR_USERRESOURCES );
    pHeapInfo->wGDIFreePercent  = GetFreeSystemResources( GFSR_GDIRESOURCES );
    pHeapInfo->hUserSegment = USER_HeapSel;
    pHeapInfo->hGDISegment  = GDI_HeapSel;
    return TRUE;
}


/***********************************************************************
 *           TimerCount   (TOOLHELP.80)
 */
BOOL16 TimerCount( TIMERINFO *pTimerInfo )
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


/**********************************************************************
 *           InitApp   (USER.5)
 */
INT16 InitApp( HINSTANCE16 hInstance )
{
    int queueSize;

      /* Create task message queue */
    queueSize = GetProfileInt32A( "windows", "DefaultQueueSize", 8 );
    if (!SetMessageQueue32( queueSize )) return 0;

    return 1;
}

/**********************************************************************
 *					USER_AppExit
 */
void USER_AppExit( HTASK16 hTask, HINSTANCE16 hInstance, HQUEUE16 hQueue )
{
    /* FIXME: empty clipboard if needed, maybe destroy menus (Windows
     *	      only complains about them but does nothing);
     */

    WND* desktop = WIN_GetDesktop();

    /* Patch desktop window */
    if( desktop->hmemTaskQ == hQueue )
	desktop->hmemTaskQ = GetTaskQueue(TASK_GetNextTask(hTask));

    /* Patch resident popup menu window */
    MENU_SwitchTPWndTo(0);

    TIMER_RemoveQueueTimers( hQueue );

    QUEUE_FlushMessages( hQueue );
    HOOK_FreeQueueHooks( hQueue );

    QUEUE_SetDoomedQueue( hQueue );
    WIN_ResetQueueWindows( desktop->child, hQueue, (HQUEUE16)0);
    QUEUE_SetDoomedQueue( 0 );

    /* Free the message queue */

    QUEUE_DeleteMsgQueue( hQueue );
}


/***********************************************************************
 *           USER_ExitWindows
 *
 * Clean-up everything and exit the Wine process.
 * This is the back-end of ExitWindows(), called when all windows
 * have agreed to be terminated.
 */
void USER_ExitWindows(void)
{
    /* Do the clean-up stuff */

    WriteOutProfiles();
    SHELL_SaveRegistry();

    exit(0);
}


/***********************************************************************
 *           ExitWindows16   (USER.7)
 */
BOOL16 ExitWindows16( DWORD dwReturnCode, UINT16 wReserved )
{
    return ExitWindowsEx( EWX_LOGOFF, 0xffffffff );
}


/***********************************************************************
 *           ExitWindowsEx   (USER32.195)
 */
BOOL32 ExitWindowsEx( UINT32 flags, DWORD reserved )
{
    int i;
    BOOL32 result;
    WND **list, **ppWnd;
        
    /* We have to build a list of all windows first, as in EnumWindows */

    if (!(list = WIN_BuildWinArray( WIN_GetDesktop() ))) return FALSE;

    /* Send a WM_QUERYENDSESSION message to every window */

    for (ppWnd = list, i = 0; *ppWnd; ppWnd++, i++)
    {
        /* Make sure that the window still exists */
        if (!IsWindow32( (*ppWnd)->hwndSelf )) continue;
	if (!SendMessage16( (*ppWnd)->hwndSelf, WM_QUERYENDSESSION, 0, 0 ))
            break;
    }
    result = !(*ppWnd);

    /* Now notify all windows that got a WM_QUERYENDSESSION of the result */

    for (ppWnd = list; i > 0; i--, ppWnd++)
    {
        if (!IsWindow32( (*ppWnd)->hwndSelf )) continue;
	SendMessage16( (*ppWnd)->hwndSelf, WM_ENDSESSION, result, 0 );
    }
    HeapFree( SystemHeap, 0, list );

    if (result) USER_ExitWindows();
    return FALSE;
}
