/*
 * Focus functions
 *
 * Copyright 1993 David Metcalfe
 *           1994 Alexandre Julliard
 * 	     1995 Alex Korobka
 *
 */

#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "win.h"
#include "winpos.h"
#include "hook.h"
#include "message.h"
#include "queue.h"
#include "user.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win);


/*****************************************************************
 *	         FOCUS_SwitchFocus 
 * pMsgQ is the queue whose perQData focus is to be modified
 */
void FOCUS_SwitchFocus( MESSAGEQUEUE *pMsgQ, HWND hFocusFrom, HWND hFocusTo )
{
    PERQDATA_SetFocusWnd( pMsgQ->pQData, hFocusTo );

    if (hFocusFrom) SendMessageA( hFocusFrom, WM_KILLFOCUS, hFocusTo, 0 );

    if( !hFocusTo || hFocusTo != PERQDATA_GetFocusWnd( pMsgQ->pQData ) )
    {
        return;
    }

    /* According to API docs, the WM_SETFOCUS message is sent AFTER the window
       has received the keyboard focus. */
    if (USER_Driver.pSetFocus) USER_Driver.pSetFocus(hFocusTo);

    SendMessageA( hFocusTo, WM_SETFOCUS, hFocusFrom, 0 );
}


/*****************************************************************
 *		SetFocus (USER.22)
 */
HWND16 WINAPI SetFocus16( HWND16 hwnd )
{
    return (HWND16)SetFocus( hwnd );
}


/*****************************************************************
 *		SetFocus (USER32.@)
 */
HWND WINAPI SetFocus( HWND hwnd )
{
    HWND hWndFocus = 0, hwndTop = hwnd;
    MESSAGEQUEUE *pMsgQ = 0, *pCurMsgQ = 0;
    BOOL bRet = 0;

    /* Get the messageQ for the current thread */
    if (!(pCurMsgQ = QUEUE_Current()))
    {
        WARN("\tCurrent message queue not found. Exiting!\n" );
        return 0;
    }

    if (hwnd)
    {
        /* Check if we can set the focus to this window */
        WND *wndPtr;

        hwnd = WIN_GetFullHandle( hwnd );
        for (;;)
        {
            HWND parent;
            LONG style = GetWindowLongW( hwndTop, GWL_STYLE );
            if (style & (WS_MINIMIZE | WS_DISABLED)) return 0;
            parent = GetAncestor( hwndTop, GA_PARENT );
            if (!parent || parent == GetDesktopWindow()) break;
            hwndTop = parent;
        }

        if (!(wndPtr = WIN_FindWndPtr( hwndTop ))) return 0;

        /* Retrieve the message queue associated with this window */
        pMsgQ = (MESSAGEQUEUE *)QUEUE_Lock( wndPtr->hmemTaskQ );
        WIN_ReleaseWndPtr( wndPtr );
        if ( !pMsgQ )
        {
            WARN("\tMessage queue not found. Exiting!\n" );
            return 0;
        }

        /* Make sure that message queue for the window we are setting focus to
         * shares the same perQ data as the current threads message queue.
         * In other words you can't set focus to a window owned by a different
         * thread unless AttachThreadInput has been called previously.
         * (see AttachThreadInput and SetFocus docs)
         */
        if ( pCurMsgQ->pQData != pMsgQ->pQData )
            goto CLEANUP;

        /* Get the current focus window from the perQ data */
        hWndFocus = PERQDATA_GetFocusWnd( pMsgQ->pQData );
        
        if( hwnd == hWndFocus )
        {
	    bRet = 1;      /* Success */
	    goto CLEANUP;  /* Nothing to do */
        }
        
	/* call hooks */
	if( HOOK_CallHooksA( WH_CBT, HCBT_SETFOCUS, (WPARAM)hwnd, (LPARAM)hWndFocus) )
	    goto CLEANUP;

        /* activate hwndTop if needed. */
	if (hwndTop != GetActiveWindow())
	{
	    if (!WINPOS_SetActiveWindow(hwndTop, 0, 0)) goto CLEANUP;

	    if (!IsWindow( hwnd )) goto CLEANUP;  /* Abort if window destroyed */
	}

        /* Get the current focus window from the perQ data */
        hWndFocus = PERQDATA_GetFocusWnd( pMsgQ->pQData );
        
        /* Change focus and send messages */
        FOCUS_SwitchFocus( pMsgQ, hWndFocus, hwnd );
    }
    else /* NULL hwnd passed in */
    {
        if( HOOK_CallHooksA( WH_CBT, HCBT_SETFOCUS, 0, (LPARAM)hWndFocus ) )
            return 0;

        /* Get the current focus from the perQ data of the current message Q */
        hWndFocus = PERQDATA_GetFocusWnd( pCurMsgQ->pQData );

      /* Change focus and send messages */
        FOCUS_SwitchFocus( pCurMsgQ, hWndFocus, hwnd );
    }

    bRet = 1;      /* Success */
    
CLEANUP:

    /* Unlock the queues before returning */
    if ( pMsgQ )
        QUEUE_Unlock( pMsgQ );

    return bRet ? hWndFocus : 0;
}


/*****************************************************************
 *		GetFocus (USER.23)
 */
HWND16 WINAPI GetFocus16(void)
{
    return (HWND16)GetFocus();
}


/*****************************************************************
 *		GetFocus (USER32.@)
 */
HWND WINAPI GetFocus(void)
{
    MESSAGEQUEUE *pCurMsgQ = 0;

    /* Get the messageQ for the current thread */
    if (!(pCurMsgQ = QUEUE_Current()))
    {
        WARN("\tCurrent message queue not found. Exiting!\n" );
        return 0;
    }

    /* Get the current focus from the perQ data of the current message Q */
    return PERQDATA_GetFocusWnd( pCurMsgQ->pQData );
}
