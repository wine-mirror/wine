/*
 * Misc. USER functions
 *
 * Copyright 1993 Robert J. Amstadt
 */

#include <stdio.h>
#include <stdlib.h>
#include "windows.h"
#include "gdi.h"
#include "user.h"
#include "win.h"
#include "toolhelp.h"

#define USER_HEAP_SIZE          0x10000

#ifndef WINELIB
LPSTR USER_Heap = NULL;
WORD USER_HeapSel = 0;


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
BOOL SystemHeapInfo( SYSHEAPINFO *pHeapInfo )
{
    pHeapInfo->wUserFreePercent = GetFreeSystemResources( GFSR_USERRESOURCES );
    pHeapInfo->wGDIFreePercent  = GetFreeSystemResources( GFSR_GDIRESOURCES );
    pHeapInfo->hUserSegment = USER_HeapSel;
    pHeapInfo->hGDISegment  = GDI_HeapSel;
    return TRUE;
}


/***********************************************************************
 *           USER_HeapInit
 */
BOOL USER_HeapInit(void)
{
    if (!(USER_HeapSel = GlobalAlloc(GMEM_FIXED,USER_HEAP_SIZE))) return FALSE;
    USER_Heap = GlobalLock( USER_HeapSel );
    LocalInit( USER_HeapSel, 0, USER_HEAP_SIZE-1 );
    return TRUE;
}
#endif


/***********************************************************************
 *           TimerCount   (TOOLHELP.80)
 */
BOOL TimerCount( TIMERINFO *pTimerInfo )
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
 *					USER_InitApp
 */
int USER_InitApp(HINSTANCE hInstance)
{
    int queueSize;

      /* Create task message queue */
    queueSize = GetProfileInt( "windows", "DefaultQueueSize", 8 );
    if (!SetMessageQueue( queueSize )) return 0;

    return 1;
}
