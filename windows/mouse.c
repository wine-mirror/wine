/*
 * MOUSE driver
 * 
 * Copyright 1998 Ulrich Weigand
 * 
 */

#include <string.h>

#include "debugtools.h"
#include "callback.h"
#include "builtin16.h"
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
    static BOOL initDone = FALSE;
    
    THUNK_Free( (FARPROC)DefMouseEventProc );
    DefMouseEventProc = lpMouseEventProc;

    /* Now initialize the mouse driver */
    if (initDone == FALSE) MOUSE_Driver->pInit();
    initDone = TRUE;
}

static VOID WINAPI MOUSE_CallMouseEventProc( FARPROC16 proc,
                                             DWORD dwFlags, DWORD dx, DWORD dy,
                                             DWORD cButtons, DWORD dwExtraInfo )
{
    CONTEXT86 context;

    memset( &context, 0, sizeof(context) );
    CS_reg(&context)  = SELECTOROF( proc );
    EIP_reg(&context) = OFFSETOF( proc );
    AX_reg(&context)  = (WORD)dwFlags;
    BX_reg(&context)  = (WORD)dx;
    CX_reg(&context)  = (WORD)dy;
    DX_reg(&context)  = (WORD)cButtons;
    SI_reg(&context)  = LOWORD( dwExtraInfo );
    DI_reg(&context)  = HIWORD( dwExtraInfo );

    CallTo16RegisterShort( &context, 0 );
}

VOID WINAPI WIN16_MOUSE_Enable( FARPROC16 proc )
{
    LPMOUSE_EVENT_PROC thunk = 
      (LPMOUSE_EVENT_PROC)THUNK_Alloc( proc, (RELAY)MOUSE_CallMouseEventProc );

    MOUSE_Enable( thunk );
}

/***********************************************************************
 *           MOUSE_Disable                       (MOUSE.3)
 */
VOID WINAPI MOUSE_Disable(VOID)
{
    THUNK_Free( (FARPROC)DefMouseEventProc );
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

    if (mouseStatus & MOUSEEVENTF_MOVE) {
      if (mouseStatus & MOUSEEVENTF_ABSOLUTE) {
	/* Relative mouse movements seems not to be scaled as absolute ones */
	posX = (((long)posX << 16) + width-1)  / width;
	posY = (((long)posY << 16) + height-1) / height;
      }
    }

    wme.magic    = WINE_MOUSEEVENT_MAGIC;
    wme.time     = time;
    wme.hWnd     = hWnd;
    wme.keyState = keyState;
    
    bOldWarpPointer = MOUSE_Driver->pEnableWarpPointer(FALSE);
    /* To avoid deadlocks, we have to suspend all locks on windows structures
       before the program control is passed to the mouse driver */
    iWndsLocks = WIN_SuspendWndsLock();
    DefMouseEventProc( mouseStatus, posX, posY, 0, (DWORD)&wme );
    WIN_RestoreWndsLock(iWndsLocks);
    MOUSE_Driver->pEnableWarpPointer(bOldWarpPointer);
}
