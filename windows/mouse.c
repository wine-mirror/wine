/*
 * MOUSE driver
 * 
 * Copyright 1998 Ulrich Weigand
 * 
 */

#include <assert.h>
#include "windows.h"
#include "gdi.h"
#include "mouse.h"
#include "debug.h"
#include "debugtools.h"


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
                      DWORD keyState, DWORD time, HWND32 hWnd )
{
    WINE_MOUSEEVENT wme;

    if ( !DefMouseEventProc ) return;

    TRACE( event, "(%04lX,%ld,%ld)\n", mouseStatus, posX, posY );

    mouseStatus |= MOUSEEVENTF_ABSOLUTE;
    posX = (((long)posX << 16) + screenWidth-1)  / screenWidth;
    posY = (((long)posY << 16) + screenHeight-1) / screenHeight;

    wme.magic    = WINE_MOUSEEVENT_MAGIC;
    wme.keyState = keyState;
    wme.time     = time;
    wme.hWnd     = hWnd;

    DefMouseEventProc( mouseStatus, posX, posY, 0, (DWORD)&wme );
}

