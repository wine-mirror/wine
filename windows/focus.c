/*
 * Focus functions
 *
 * Copyright 1993 David Metcalfe
 *           1994 Alexandre Julliard
 * 	     1995 Alex Korobka
 *
 */

#include "wine/winuser16.h"
#include "win.h"
#include "winpos.h"
#include "hook.h"
#include "message.h"
#include "task.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(win)


/*****************************************************************
 *	         FOCUS_SwitchFocus 
 * pMsgQ is the queue whose perQData focus is to be modified
 */
void FOCUS_SwitchFocus( MESSAGEQUEUE *pMsgQ, HWND hFocusFrom, HWND hFocusTo )
{
    WND *pFocusTo = WIN_FindWndPtr( hFocusTo );

    PERQDATA_SetFocusWnd( pMsgQ->pQData, hFocusTo );
    
#if 0
    if (hFocusFrom) SendMessageA( hFocusFrom, WM_KILLFOCUS, hFocusTo, 0 );
#else
    /* FIXME: must be SendMessage16() because 32A doesn't do
     * intertask at this time */
    if (hFocusFrom) SendMessage16( hFocusFrom, WM_KILLFOCUS, hFocusTo, 0 );
#endif

    if( !pFocusTo || hFocusTo != PERQDATA_GetFocusWnd( pMsgQ->pQData ) )
    {
        WIN_ReleaseWndPtr(pFocusTo);
	return;
    }

    /* According to API docs, the WM_SETFOCUS message is sent AFTER the window
       has received the keyboard focus. */

    pFocusTo->pDriver->pSetFocus(pFocusTo);

    WIN_ReleaseWndPtr(pFocusTo);
#if 0
    SendMessageA( hFocusTo, WM_SETFOCUS, hFocusFrom, 0 );
#else
    SendMessage16( hFocusTo, WM_SETFOCUS, hFocusFrom, 0 );
#endif
}


/*****************************************************************
 *               SetFocus16   (USER.22)
 */
HWND16 WINAPI SetFocus16( HWND16 hwnd )
{
    return (HWND16)SetFocus( hwnd );
}


/*****************************************************************
 *               SetFocus32   (USER32.481)
 */
HWND WINAPI SetFocus( HWND hwnd )
{
    HWND hWndFocus = 0, hwndTop = hwnd;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    MESSAGEQUEUE *pMsgQ = 0, *pCurMsgQ = 0;
    BOOL16 bRet = 0;

    /* Get the messageQ for the current thread */
    if (!(pCurMsgQ = (MESSAGEQUEUE *)QUEUE_Lock( GetFastQueue16() )))
    {
        WARN( win, "\tCurrent message queue not found. Exiting!\n" );
        goto CLEANUP;
    }

    if (wndPtr)
    {
	  /* Check if we can set the focus to this window */

	while ( (wndPtr->dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD  )
	{
	    if ( wndPtr->dwStyle & ( WS_MINIMIZE | WS_DISABLED) )
		 goto CLEANUP;
            WIN_UpdateWndPtr(&wndPtr,wndPtr->parent);
            if (!wndPtr) goto CLEANUP;
	    hwndTop = wndPtr->hwndSelf;
	}

        /* Retrieve the message queue associated with this window */
        pMsgQ = (MESSAGEQUEUE *)QUEUE_Lock( wndPtr->hmemTaskQ );
        if ( !pMsgQ )
        {
            WARN( win, "\tMessage queue not found. Exiting!\n" );
            goto CLEANUP;
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
	if( HOOK_CallHooks16( WH_CBT, HCBT_SETFOCUS, (WPARAM16)hwnd,
			      (LPARAM)hWndFocus) )
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
        if( HOOK_CallHooks16( WH_CBT, HCBT_SETFOCUS, 0, (LPARAM)hWndFocus ) )
            goto CLEANUP;

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
    if ( pCurMsgQ )
        QUEUE_Unlock( pCurMsgQ );

    WIN_ReleaseWndPtr(wndPtr);
    return bRet ? hWndFocus : 0;
}


/*****************************************************************
 *               GetFocus16   (USER.23)
 */
HWND16 WINAPI GetFocus16(void)
{
    return (HWND16)GetFocus();
}


/*****************************************************************
 *               GetFocus32   (USER32.240)
 */
HWND WINAPI GetFocus(void)
{
    MESSAGEQUEUE *pCurMsgQ = 0;
    HWND hwndFocus = 0;

    /* Get the messageQ for the current thread */
    if (!(pCurMsgQ = (MESSAGEQUEUE *)QUEUE_Lock( GetFastQueue16() )))
    {
        WARN( win, "\tCurrent message queue not found. Exiting!\n" );
        return 0;
    }

    /* Get the current focus from the perQ data of the current message Q */
    hwndFocus = PERQDATA_GetFocusWnd( pCurMsgQ->pQData );

    QUEUE_Unlock( pCurMsgQ );

    return hwndFocus;
}
