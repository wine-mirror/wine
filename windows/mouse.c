/*
 * MOUSE driver
 * 
 * Copyright 1998 Ulrich Weigand
 * 
 */

#include "debugtools.h"
#include "mouse.h"
#include "monitor.h"
#include "winuser.h"
#include "win.h"

DEFAULT_DEBUG_CHANNEL(event)

/**********************************************************************/

MOUSE_DRIVER *MOUSE_Driver = NULL;

static LPMOUSE_EVENT_PROC DefMouseEventProc = NULL;

/***********************************************************************
 *           MOUSE_Inquire                       (MOUSE.1)
 */
WORD WINAPI MOUSE_Inquire(LPMOUSEINFO mouseInfo)
{
    mouseInfo->msExist = TRUE;
    mouseInfo->msRelative = FALSE;
    mouseInfo->msNumButtons = 2;
    mouseInfo->msRate = 34;  /* the DDK says so ... */
    mouseInfo->msXThreshold = 0;
    mouseInfo->msYThreshold = 0;
    mouseInfo->msXRes = 0;
    mouseInfo->msYRes = 0;
    mouseInfo->msMouseCommPort = 0;

    return sizeof(MOUSEINFO);
}

/***********************************************************************
 *           MOUSE_Enable                        (MOUSE.2)
 */
VOID WINAPI MOUSE_Enable(LPMOUSE_EVENT_PROC lpMouseEventProc)
{
    DefMouseEventProc = lpMouseEventProc;
}

/***********************************************************************
 *           MOUSE_Disable                       (MOUSE.3)
 */
VOID WINAPI MOUSE_Disable(VOID)
{
    DefMouseEventProc = 0;
}

/***********************************************************************
 *           MOUSE_SendEvent
 */
void MOUSE_SendEvent( DWORD mouseStatus, DWORD posX, DWORD posY, 
                      DWORD keyState, DWORD time, HWND hWnd )
{
    int width  = MONITOR_GetWidth (&MONITOR_PrimaryMonitor);
    int height = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
    int iWndsLocks;
    WINE_MOUSEEVENT wme;
    BOOL bOldWarpPointer;

    if ( !DefMouseEventProc ) return;

    TRACE("(%04lX,%ld,%ld)\n", mouseStatus, posX, posY );

    mouseStatus |= MOUSEEVENTF_ABSOLUTE;
    posX = (((long)posX << 16) + width-1)  / width;
    posY = (((long)posY << 16) + height-1) / height;

    wme.magic    = WINE_MOUSEEVENT_MAGIC;
    wme.keyState = keyState;
    wme.time     = time;
    wme.hWnd     = hWnd;

    bOldWarpPointer = MOUSE_Driver->pEnableWarpPointer(FALSE);
    /* To avoid deadlocks, we have to suspend all locks on windows structures
       before the program control is passed to the mouse driver */
    iWndsLocks = WIN_SuspendWndsLock();
    DefMouseEventProc( mouseStatus, posX, posY, 0, (DWORD)&wme );
    WIN_RestoreWndsLock(iWndsLocks);
    MOUSE_Driver->pEnableWarpPointer(bOldWarpPointer);
}
