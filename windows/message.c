/*
 * Message queues related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#include "message.h"
#include "win.h"
#include "gdi.h"
#include "sysmetrics.h"
#include "hook.h"
#include "spy.h"
#include "winpos.h"
#include "atom.h"
#include "dde.h"
#include "queue.h"
#include "stddebug.h"
/* #define DEBUG_MSG */
#include "debug.h"

#define HWND_BROADCAST16  ((HWND16)0xffff)
#define HWND_BROADCAST32  ((HWND32)0xffffffff)

extern BYTE* 	KeyStateTable;				 /* event.c */
extern WPARAM	lastEventChar;				 /* event.c */

extern BOOL TIMER_CheckTimer( LONG *next, MSG16 *msg,
			      HWND hwnd, BOOL remove );  /* timer.c */

DWORD MSG_WineStartTicks;  				 /* Ticks at Wine startup */

static WORD doubleClickSpeed = 452;

/***********************************************************************
 *           MSG_TranslateMouseMsg
 *
 * Translate an mouse hardware event into a real mouse message.
 * Return value indicates whether the translated message must be passed
 * to the user.
 * Actions performed:
 * - Find the window for this message.
 * - Translate button-down messages in double-clicks.
 * - Send the WM_NCHITTEST message to find where the cursor is.
 * - Activate the window if needed.
 * - Translate the message into a non-client message, or translate
 *   the coordinates to client coordinates.
 * - Send the WM_SETCURSOR message.
 */
static BOOL MSG_TranslateMouseMsg( MSG16 *msg, BOOL remove )
{
    WND *pWnd;
    BOOL eatMsg = FALSE;
    INT16 hittest;
    static DWORD lastClickTime = 0;
    static WORD  lastClickMsg = 0;
    static POINT16 lastClickPos = { 0, 0 };
    POINT16 pt = msg->pt;
    MOUSEHOOKSTRUCT16 hook = { msg->pt, 0, HTCLIENT, 0 };

    BOOL mouseClick = ((msg->message == WM_LBUTTONDOWN) ||
		       (msg->message == WM_RBUTTONDOWN) ||
		       (msg->message == WM_MBUTTONDOWN));

      /* Find the window */

    if (GetCapture())
    {
	msg->hwnd = GetCapture();
	ScreenToClient16( msg->hwnd, &pt );
	msg->lParam = MAKELONG( pt.x, pt.y );
        /* No need to further process the message */
        hook.hwnd = msg->hwnd;
        return !HOOK_CallHooks( WH_MOUSE, remove ? HC_ACTION : HC_NOREMOVE,
                                msg->message, (LPARAM)MAKE_SEGPTR(&hook));
    }
   
    hittest = WINPOS_WindowFromPoint( msg->pt, &pWnd );
    msg->hwnd = pWnd->hwndSelf;
    if ((hittest != HTERROR) && mouseClick)
    {
        HWND hwndTop = WIN_GetTopParent( msg->hwnd );

        /* Send the WM_PARENTNOTIFY message */

        WIN_SendParentNotify( msg->hwnd, msg->message, 0,
                              MAKELONG( msg->pt.x, msg->pt.y ) );

        /* Activate the window if needed */

        if (msg->hwnd != GetActiveWindow() && msg->hwnd != GetDesktopWindow())
        {
            LONG ret = SendMessage16( msg->hwnd, WM_MOUSEACTIVATE, hwndTop,
                                    MAKELONG( hittest, msg->message ) );

            if ((ret == MA_ACTIVATEANDEAT) || (ret == MA_NOACTIVATEANDEAT))
                eatMsg = TRUE;

            if (((ret == MA_ACTIVATE) || (ret == MA_ACTIVATEANDEAT)) 
                && hwndTop != GetActiveWindow() )
                WINPOS_SetActiveWindow( hwndTop, TRUE , TRUE );
        }
    }

      /* Send the WM_SETCURSOR message */

    SendMessage16( msg->hwnd, WM_SETCURSOR, (WPARAM)msg->hwnd,
                   MAKELONG( hittest, msg->message ));
    if (eatMsg) return FALSE;

      /* Check for double-click */

    if (mouseClick)
    {
	BOOL dbl_click = FALSE;

	if ((msg->message == lastClickMsg) &&
	    (msg->time - lastClickTime < doubleClickSpeed) &&
	    (abs(msg->pt.x - lastClickPos.x) < SYSMETRICS_CXDOUBLECLK/2) &&
	    (abs(msg->pt.y - lastClickPos.y) < SYSMETRICS_CYDOUBLECLK/2))
	    dbl_click = TRUE;

	if (dbl_click && (hittest == HTCLIENT))
	{
	    /* Check whether window wants the double click message. */
            dbl_click = (pWnd->class->style & CS_DBLCLKS) != 0;
	}

	if (dbl_click) switch(msg->message)
	{
	    case WM_LBUTTONDOWN: msg->message = WM_LBUTTONDBLCLK; break;
	    case WM_RBUTTONDOWN: msg->message = WM_RBUTTONDBLCLK; break;
	    case WM_MBUTTONDOWN: msg->message = WM_MBUTTONDBLCLK; break;
	}

	if (remove)
	{
	    lastClickTime = msg->time;
	    lastClickMsg  = msg->message;
	    lastClickPos  = msg->pt;
	}
    }

      /* Build the translated message */

    if (hittest == HTCLIENT)
        ScreenToClient16( msg->hwnd, &pt );
    else
    {
	msg->wParam = hittest;
	msg->message += WM_NCLBUTTONDOWN - WM_LBUTTONDOWN;
    }
    msg->lParam = MAKELONG( pt.x, pt.y );
    
    hook.hwnd = msg->hwnd;
    hook.wHitTestCode = hittest;
    return !HOOK_CallHooks( WH_MOUSE, remove ? HC_ACTION : HC_NOREMOVE,
                            msg->message, (LPARAM)MAKE_SEGPTR(&hook));
}


/***********************************************************************
 *           MSG_TranslateKeyboardMsg
 *
 * Translate an keyboard hardware event into a real message.
 * Return value indicates whether the translated message must be passed
 * to the user.
 */
static BOOL MSG_TranslateKeyboardMsg( MSG16 *msg, BOOL remove )
{
      /* Should check Ctrl-Esc and PrintScreen here */

    msg->hwnd = GetFocus();
    if (!msg->hwnd)
    {
	  /* Send the message to the active window instead,  */
	  /* translating messages to their WM_SYS equivalent */

	msg->hwnd = GetActiveWindow();

	if( msg->message < WM_SYSKEYDOWN )
	    msg->message += WM_SYSKEYDOWN - WM_KEYDOWN;
    }
    return !HOOK_CallHooks( WH_KEYBOARD, remove ? HC_ACTION : HC_NOREMOVE,
                            msg->wParam, msg->lParam );
}


/***********************************************************************
 *           MSG_PeekHardwareMsg
 *
 * Peek for a hardware message matching the hwnd and message filters.
 */
static BOOL MSG_PeekHardwareMsg( MSG16 *msg, HWND hwnd, WORD first, WORD last,
                                 BOOL remove )
{
    MESSAGEQUEUE *sysMsgQueue = QUEUE_GetSysQueue();
    int i, pos = sysMsgQueue->nextMessage;

    /* If the queue is empty, attempt to fill it */
    if (!sysMsgQueue->msgCount && XPending(display)) EVENT_WaitXEvent( 0 );

    for (i = 0; i < sysMsgQueue->msgCount; i++, pos++)
    {
        if (pos >= sysMsgQueue->queueSize) pos = 0;
	*msg = sysMsgQueue->messages[pos].msg;

          /* Translate message */

        if ((msg->message >= WM_MOUSEFIRST) && (msg->message <= WM_MOUSELAST))
        {
            if (!MSG_TranslateMouseMsg( msg, remove )) continue;
        }
        else if ((msg->message >= WM_KEYFIRST) && (msg->message <= WM_KEYLAST))
        {
            if (!MSG_TranslateKeyboardMsg( msg, remove )) continue;
        }
        else  /* Non-standard hardware event */
        {
            HARDWAREHOOKSTRUCT16 hook = { msg->hwnd, msg->message,
                                          msg->wParam, msg->lParam };
            if (HOOK_CallHooks( WH_HARDWARE, remove ? HC_ACTION : HC_NOREMOVE,
                                0, (LPARAM)MAKE_SEGPTR(&hook) )) continue;
        }

          /* Check message against filters */

        if (hwnd && (msg->hwnd != hwnd)) continue;
        if ((first || last) && 
            ((msg->message < first) || (msg->message > last))) continue;
        if ((msg->hwnd != GetDesktopWindow()) && 
            (GetWindowTask(msg->hwnd) != GetCurrentTask()))
            continue;  /* Not for this task */
        if (remove)
        {
            MSG16 tmpMsg = *msg; /* FIXME */
            HOOK_CallHooks( WH_JOURNALRECORD, HC_ACTION,
                            0, (LPARAM)MAKE_SEGPTR(&tmpMsg) );
            QUEUE_RemoveMsg( sysMsgQueue, pos );
        }
        return TRUE;
    }
    return FALSE;
}


/**********************************************************************
 *           SetDoubleClickTime   (USER.20)
 */
void SetDoubleClickTime( WORD interval )
{
    doubleClickSpeed = interval ? interval : 500;
}		


/**********************************************************************
 *           GetDoubleClickTime   (USER.21)
 */
WORD GetDoubleClickTime()
{
    return doubleClickSpeed;
}		


/***********************************************************************
 *           MSG_GetHardwareMessage
 *
 * Like GetMessage(), but only return mouse and keyboard events.
 * Used internally for window moving and resizing. Mouse messages
 * are not translated.
 * Warning: msg->hwnd is always 0.
 */
BOOL MSG_GetHardwareMessage( LPMSG16 msg )
{
#if 0
    int pos;
    XEvent event;
    MESSAGEQUEUE *sysMsgQueue = QUEUE_GetSysQueue();

    while(1)
    {    
	if ((pos = QUEUE_FindMsg( sysMsgQueue, 0, 0, 0 )) != -1)
	{
	    *msg = sysMsgQueue->messages[pos].msg;
	    QUEUE_RemoveMsg( sysMsgQueue, pos );
	    break;
	}
	XNextEvent( display, &event );
	EVENT_ProcessEvent( &event );
    }
#endif
    MSG_PeekMessage( msg, 0, WM_KEYFIRST, WM_MOUSELAST, PM_REMOVE, 0 );
    return TRUE;
}


/***********************************************************************
 *           MSG_SendMessage
 *
 * Implementation of an inter-task SendMessage.
 */
LRESULT MSG_SendMessage( HQUEUE hDestQueue, HWND hwnd, UINT msg,
                         WPARAM wParam, LPARAM lParam )
{
    MESSAGEQUEUE *queue, *destQ;

    if (!(queue = (MESSAGEQUEUE*)GlobalLock16( GetTaskQueue(0) ))) return 0;
    if (!(destQ = (MESSAGEQUEUE*)GlobalLock16( hDestQueue ))) return 0;

    if (IsTaskLocked())
    {
        fprintf( stderr, "SendMessage: task is locked\n" );
        return 0;
    }

    if (queue->hWnd)
    {
        fprintf( stderr, "Nested SendMessage() not supported\n" );
        return 0;
    }
    queue->hWnd   = hwnd;
    queue->msg    = msg;
    queue->wParam = wParam;
    queue->lParam = lParam;
    queue->hPrevSendingTask = destQ->hSendingTask;
    destQ->hSendingTask = GetTaskQueue(0);
    QUEUE_SetWakeBit( destQ, QS_SENDMESSAGE );

    /* Wait for the result */

    printf( "SendMessage %04x to %04x\n", msg, hDestQueue );

    if (!(queue->wakeBits & QS_SMRESULT))
    {
        DirectedYield( hDestQueue );
        QUEUE_WaitBits( QS_SMRESULT );
    }
    printf( "SendMessage %04x to %04x: got %08x\n",
            msg, hDestQueue, queue->SendMessageReturn );
    queue->wakeBits &= ~QS_SMRESULT;
    return queue->SendMessageReturn;
}


/***********************************************************************
 *           ReplyMessage   (USER.115)
 */
void ReplyMessage( LRESULT result )
{
    MESSAGEQUEUE *senderQ;
    MESSAGEQUEUE *queue;

    printf( "ReplyMessage\n " );
    if (!(queue = (MESSAGEQUEUE*)GlobalLock16( GetTaskQueue(0) ))) return;
    if (!(senderQ = (MESSAGEQUEUE*)GlobalLock16( queue->InSendMessageHandle)))
        return;
    for (;;)
    {
        if (queue->wakeBits & QS_SENDMESSAGE) QUEUE_ReceiveMessage( queue );
        else if (senderQ->wakeBits & QS_SMRESULT) Yield();
        else break;
    }
    printf( "ReplyMessage: res = %08x\n", result );
    senderQ->SendMessageReturn = result;
    queue->InSendMessageHandle = 0;
    QUEUE_SetWakeBit( senderQ, QS_SMRESULT );
    DirectedYield( queue->hSendingTask );
}


/***********************************************************************
 *           MSG_PeekMessage
 */
static BOOL MSG_PeekMessage( LPMSG16 msg, HWND hwnd, WORD first, WORD last,
                             WORD flags, BOOL peek )
{
    int pos, mask;
    MESSAGEQUEUE *msgQueue;
    HQUEUE	  hQueue;
    LONG nextExp;  /* Next timer expiration time */

#ifdef CONFIG_IPC
    DDE_TestDDE(hwnd);	/* do we have dde handling in the window ?*/
    DDE_GetRemoteMessage();
#endif  /* CONFIG_IPC */

    mask = QS_POSTMESSAGE | QS_SENDMESSAGE;  /* Always selected */
    if (first || last)
    {
	if ((first <= WM_KEYLAST) && (last >= WM_KEYFIRST)) mask |= QS_KEY;
	if ((first <= WM_MOUSELAST) && (last >= WM_MOUSEFIRST)) mask |= QS_MOUSE;
	if ((first <= WM_TIMER) && (last >= WM_TIMER)) mask |= QS_TIMER;
	if ((first <= WM_SYSTIMER) && (last >= WM_SYSTIMER)) mask |= QS_TIMER;
	if ((first <= WM_PAINT) && (last >= WM_PAINT)) mask |= QS_PAINT;
    }
    else mask |= QS_MOUSE | QS_KEY | QS_TIMER | QS_PAINT;

    if (IsTaskLocked()) flags |= PM_NOYIELD;

    while(1)
    {    
	hQueue   = GetTaskQueue(0);
        msgQueue = (MESSAGEQUEUE *)GlobalLock16( hQueue );
        if (!msgQueue) return FALSE;
        msgQueue->changeBits = 0;

        /* First handle a message put by SendMessage() */

	if (msgQueue->wakeBits & QS_SENDMESSAGE)
            QUEUE_ReceiveMessage( msgQueue );
    
        /* Now find a normal message */

        if (((msgQueue->wakeBits & mask) & QS_POSTMESSAGE) &&
            ((pos = QUEUE_FindMsg( msgQueue, hwnd, first, last )) != -1))
        {
            QMSG *qmsg = &msgQueue->messages[pos];
            *msg = qmsg->msg;
            msgQueue->GetMessageTimeVal      = msg->time;
            msgQueue->GetMessagePosVal       = *(DWORD *)&msg->pt;
            msgQueue->GetMessageExtraInfoVal = qmsg->extraInfo;

            if (flags & PM_REMOVE) QUEUE_RemoveMsg( msgQueue, pos );
            break;
        }

        /* Now find a hardware event */

        if (((msgQueue->wakeBits & mask) & (QS_MOUSE | QS_KEY)) &&
            MSG_PeekHardwareMsg( msg, hwnd, first, last, flags & PM_REMOVE ))
        {
            /* Got one */
	    msgQueue->GetMessageTimeVal      = msg->time;
	    msgQueue->GetMessagePosVal       = *(DWORD *)&msg->pt;
	    msgQueue->GetMessageExtraInfoVal = 0;  /* Always 0 for now */
            break;
        }

        /* Now handle a WM_QUIT message */

	if (msgQueue->wPostQMsg)
	{
	    msg->hwnd    = hwnd;
	    msg->message = WM_QUIT;
	    msg->wParam  = msgQueue->wExitCode;
	    msg->lParam  = 0;
	    break;
	}

        /* Check again for SendMessage */

 	if (msgQueue->wakeBits & QS_SENDMESSAGE)
            QUEUE_ReceiveMessage( msgQueue );

        /* Now find a WM_PAINT message */

	if ((msgQueue->wakeBits & mask) & QS_PAINT)
	{
	    msg->hwnd = WIN_FindWinToRepaint( hwnd , hQueue );
	    msg->message = WM_PAINT;
	    msg->wParam = 0;
	    msg->lParam = 0;
            if( msg->hwnd &&
              (!hwnd || msg->hwnd == hwnd || IsChild(hwnd,msg->hwnd)) )
              {
                WND* wndPtr = WIN_FindWndPtr(msg->hwnd);

	        /* FIXME: WM_PAINTICON should be sent sometimes */

                if( wndPtr->flags & WIN_INTERNAL_PAINT && !wndPtr->hrgnUpdate)
                  {
                    wndPtr->flags &= ~WIN_INTERNAL_PAINT;
                    QUEUE_DecPaintCount( hQueue );
                  }
                break;
              }
	}

        /* Check for timer messages, but yield first */

        if (!(flags & PM_NOYIELD))
        {
            UserYield();
            if (msgQueue->wakeBits & QS_SENDMESSAGE)
                QUEUE_ReceiveMessage( msgQueue );
        }
	if ((msgQueue->wakeBits & mask) & QS_TIMER)
	{
	    if (TIMER_CheckTimer( &nextExp, msg, hwnd, flags & PM_REMOVE ))
		break;  /* Got a timer msg */
	}
	else nextExp = -1;  /* No timeout needed */

        if (peek)
        {
            if (!(flags & PM_NOYIELD)) UserYield();
            return FALSE;
        }
        msgQueue->wakeMask = mask;
        QUEUE_WaitBits( mask );
    }

      /* We got a message */
    if (peek) return TRUE;
    else return (msg->message != WM_QUIT);
}


/***********************************************************************
 *           MSG_InternalGetMessage
 *
 * GetMessage() function for internal use. Behave like GetMessage(),
 * but also call message filters and optionally send WM_ENTERIDLE messages.
 * 'hwnd' must be the handle of the dialog or menu window.
 * 'code' is the message filter value (MSGF_??? codes).
 */
BOOL MSG_InternalGetMessage( SEGPTR msg, HWND hwnd, HWND hwndOwner, short code,
			     WORD flags, BOOL sendIdle ) 
{
    for (;;)
    {
	if (sendIdle)
	{
	    if (!MSG_PeekMessage( (MSG16 *)PTR_SEG_TO_LIN(msg),
                                  0, 0, 0, flags, TRUE ))
	    {
		  /* No message present -> send ENTERIDLE and wait */
                if (IsWindow(hwndOwner))
                    SendMessage16( hwndOwner, WM_ENTERIDLE,
                                   code, (LPARAM)hwnd );
		MSG_PeekMessage( (MSG16 *)PTR_SEG_TO_LIN(msg),
                                 0, 0, 0, flags, FALSE );
	    }
	}
	else  /* Always wait for a message */
	    MSG_PeekMessage( (MSG16 *)PTR_SEG_TO_LIN(msg),
                             0, 0, 0, flags, FALSE );

	if (!CallMsgFilter( msg, code ))
            return (((MSG16 *)PTR_SEG_TO_LIN(msg))->message != WM_QUIT);

	  /* Message filtered -> remove it from the queue */
	  /* if it's still there. */
	if (!(flags & PM_REMOVE))
	    MSG_PeekMessage( (MSG16 *)PTR_SEG_TO_LIN(msg),
                             0, 0, 0, PM_REMOVE, TRUE );
    }
}


/***********************************************************************
 *           PeekMessage16   (USER.109)
 */
BOOL16 PeekMessage16( LPMSG16 msg, HWND16 hwnd, UINT16 first,
                      UINT16 last, UINT16 flags )
{
    return MSG_PeekMessage( msg, hwnd, first, last, flags, TRUE );
}


/***********************************************************************
 *           GetMessage   (USER.108)
 */
BOOL GetMessage( SEGPTR msg, HWND hwnd, UINT first, UINT last ) 
{
    MSG16 *lpmsg = (MSG16 *)PTR_SEG_TO_LIN(msg);
    MSG_PeekMessage( lpmsg,
                     hwnd, first, last, PM_REMOVE, FALSE );

    dprintf_msg(stddeb,"message %04x, hwnd %04x, filter(%04x - %04x)\n", lpmsg->message,
		     				                 hwnd, first, last );
    HOOK_CallHooks( WH_GETMESSAGE, HC_ACTION, 0, (LPARAM)msg );
    return (lpmsg->message != WM_QUIT);
}


/***********************************************************************
 *           PostMessage   (USER.110)
 */
BOOL PostMessage( HWND hwnd, WORD message, WORD wParam, LONG lParam )
{
    MSG16 	msg;
    WND 	*wndPtr;

    msg.hwnd    = hwnd;
    msg.message = message;
    msg.wParam  = wParam;
    msg.lParam  = lParam;
    msg.time    = GetTickCount();
    msg.pt.x    = 0;
    msg.pt.y    = 0;

#ifdef CONFIG_IPC
    if (DDE_PostMessage(&msg))
       return TRUE;
#endif  /* CONFIG_IPC */
    
    if (hwnd == HWND_BROADCAST16)
    {
        dprintf_msg(stddeb,"PostMessage // HWND_BROADCAST !\n");
        for (wndPtr = WIN_GetDesktop()->child; wndPtr; wndPtr = wndPtr->next)
        {
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
            {
                dprintf_msg(stddeb,"BROADCAST Message to hWnd=%04x m=%04X w=%04X l=%08lX !\n",
                            wndPtr->hwndSelf, message, wParam, lParam);
                PostMessage( wndPtr->hwndSelf, message, wParam, lParam );
            }
        }
        dprintf_msg(stddeb,"PostMessage // End of HWND_BROADCAST !\n");
        return TRUE;
    }

    wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr || !wndPtr->hmemTaskQ) return FALSE;

    return QUEUE_AddMsg( wndPtr->hmemTaskQ, &msg, 0 );
}

/***********************************************************************
 *           PostAppMessage   (USER.116)
 */
BOOL PostAppMessage( HTASK hTask, WORD message, WORD wParam, LONG lParam )
{
    MSG16 msg;

    if (GetTaskQueue(hTask) == 0) return FALSE;
    msg.hwnd    = 0;
    msg.message = message;
    msg.wParam  = wParam;
    msg.lParam  = lParam;
    msg.time    = GetTickCount();
    msg.pt.x    = 0;
    msg.pt.y    = 0;

    return QUEUE_AddMsg( GetTaskQueue(hTask), &msg, 0 );
}


/***********************************************************************
 *           SendMessage16   (USER.111)
 */
LRESULT SendMessage16( HWND16 hwnd, UINT16 msg, WPARAM16 wParam, LPARAM lParam)
{
    WND * wndPtr;
    LRESULT ret;
    struct
    {
	LPARAM   lParam;
	WPARAM16 wParam;
	UINT16   wMsg;
	HWND16   hWnd;
    } msgstruct = { lParam, wParam, msg, hwnd };

#ifdef CONFIG_IPC
    MSG DDE_msg = { hwnd, msg, wParam, lParam };
    if (DDE_SendMessage(&DDE_msg)) return TRUE;
#endif  /* CONFIG_IPC */

    if (hwnd == HWND_BROADCAST16)
    {
        dprintf_msg(stddeb,"SendMessage // HWND_BROADCAST !\n");
        for (wndPtr = WIN_GetDesktop()->child; wndPtr; wndPtr = wndPtr->next)
        {
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
            {
                dprintf_msg(stddeb,"BROADCAST Message to hWnd=%04x m=%04X w=%04lX l=%08lX !\n",
                            wndPtr->hwndSelf, msg, (DWORD)wParam, lParam);
                SendMessage16( wndPtr->hwndSelf, msg, wParam, lParam );
	    }
        }
        dprintf_msg(stddeb,"SendMessage // End of HWND_BROADCAST !\n");
        return TRUE;
    }


    HOOK_CallHooks( WH_CALLWNDPROC, HC_ACTION, 1,
                    (LPARAM)MAKE_SEGPTR(&msgstruct) );
    hwnd   = msgstruct.hWnd;
    msg    = msgstruct.wMsg;
    wParam = msgstruct.wParam;
    lParam = msgstruct.lParam;

    if (!(wndPtr = WIN_FindWndPtr( hwnd )))
    {
        fprintf( stderr, "SendMessage16: invalid hwnd %04x\n", hwnd );
        return 0;
    }
    if (wndPtr->hmemTaskQ != GetTaskQueue(0))
    {
#if 0
        fprintf( stderr, "SendMessage16: intertask message not supported\n" );
        return 0;
#endif
        return MSG_SendMessage( wndPtr->hmemTaskQ, hwnd, msg, wParam, lParam );
    }

    SPY_EnterMessage( SPY_SENDMESSAGE16, hwnd, msg, wParam, lParam );
    ret = CallWindowProc16( (WNDPROC16)wndPtr->winproc,
                            hwnd, msg, wParam, lParam );
    SPY_ExitMessage( SPY_RESULT_OK16, hwnd, msg, ret );
    return ret;
}


/***********************************************************************
 *           SendMessage32A   (USER32.453)
 */
LRESULT SendMessage32A(HWND32 hwnd, UINT32 msg, WPARAM32 wParam, LPARAM lParam)
{
    WND * wndPtr;
    LRESULT ret;

    if (hwnd == HWND_BROADCAST32)
    {
        for (wndPtr = WIN_GetDesktop()->child; wndPtr; wndPtr = wndPtr->next)
        {
            /* FIXME: should use something like EnumWindows here */
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
                SendMessage32A( wndPtr->hwndSelf, msg, wParam, lParam );
        }
        return TRUE;
    }

    /* FIXME: call hooks */

    if (!(wndPtr = WIN_FindWndPtr( hwnd )))
    {
        fprintf( stderr, "SendMessage32A: invalid hwnd %08x\n", hwnd );
        return 0;
    }
    if (wndPtr->hmemTaskQ != GetTaskQueue(0))
    {
        fprintf( stderr, "SendMessage32A: intertask message not supported\n" );
        return 0;
    }

    SPY_EnterMessage( SPY_SENDMESSAGE32, hwnd, msg, wParam, lParam );
    ret = CallWindowProc32A( (WNDPROC32)wndPtr->winproc,
                             hwnd, msg, wParam, lParam );
    SPY_ExitMessage( SPY_RESULT_OK32, hwnd, msg, ret );
    return ret;
}


/***********************************************************************
 *           SendMessage32W   (USER32.458)
 */
LRESULT SendMessage32W(HWND32 hwnd, UINT32 msg, WPARAM32 wParam, LPARAM lParam)
{
    WND * wndPtr;
    LRESULT ret;

    if (hwnd == HWND_BROADCAST32)
    {
        for (wndPtr = WIN_GetDesktop()->child; wndPtr; wndPtr = wndPtr->next)
        {
            /* FIXME: should use something like EnumWindows here */
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
                SendMessage32W( wndPtr->hwndSelf, msg, wParam, lParam );
        }
        return TRUE;
    }

    /* FIXME: call hooks */

    if (!(wndPtr = WIN_FindWndPtr( hwnd )))
    {
        fprintf( stderr, "SendMessage32W: invalid hwnd %08x\n", hwnd );
        return 0;
    }
    if (wndPtr->hmemTaskQ != GetTaskQueue(0))
    {
        fprintf( stderr, "SendMessage32W: intertask message not supported\n" );
        return 0;
    }

    SPY_EnterMessage( SPY_SENDMESSAGE32, hwnd, msg, wParam, lParam );
    ret = CallWindowProc32W( (WNDPROC32)wndPtr->winproc,
                             hwnd, msg, wParam, lParam );
    SPY_ExitMessage( SPY_RESULT_OK32, hwnd, msg, ret );
    return ret;
}


/***********************************************************************
 *           WaitMessage    (USER.112)
 */
void WaitMessage( void )
{
    MSG16 msg;
    MESSAGEQUEUE *queue;
    LONG nextExp = -1;  /* Next timer expiration time */

#ifdef CONFIG_IPC
    DDE_GetRemoteMessage();
#endif  /* CONFIG_IPC */
    
    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return;
    if ((queue->wPostQMsg) || 
        (queue->wakeBits & (QS_SENDMESSAGE | QS_PAINT)) ||
        (queue->msgCount) || (QUEUE_GetSysQueue()->msgCount) )
        return;
    if ((queue->wakeBits & QS_TIMER) && 
        TIMER_CheckTimer( &nextExp, &msg, 0, FALSE))
        return;
    /* FIXME: (dde) must check DDE & X-events simultaneously */
    EVENT_WaitXEvent( nextExp );
}


/***********************************************************************
 *           TranslateMessage   (USER.113)
 *
 * This should call ToAscii but it is currently broken
 */

#define ASCII_CHAR_HACK 0x0800

BOOL TranslateMessage( LPMSG16 msg )
{
    UINT message = msg->message;
    /* BYTE wparam[2]; */
    
    if ((message == WM_KEYDOWN) || (message == WM_KEYUP) ||
	(message == WM_SYSKEYDOWN) || (message == WM_SYSKEYUP))
    {
	dprintf_msg(stddeb, "Translating key %04x, scancode %04x\n", msg->wParam, 
							      HIWORD(msg->lParam) );

	if( HIWORD(msg->lParam) & ASCII_CHAR_HACK )

	/*  if( ToAscii( msg->wParam, HIWORD(msg->lParam), (LPSTR)&KeyStateTable,
				      wparam, 0 ) ) 
         */
	      {
     		message += 2 - (message & 0x0001); 

	        PostMessage( msg->hwnd, message, lastEventChar, msg->lParam );

	        return TRUE;
	      }
    }
    return FALSE;
}


/***********************************************************************
 *           DispatchMessage   (USER.114)
 */
LONG DispatchMessage( const MSG16* msg )
{
    WND * wndPtr;
    LONG retval;
    int painting;
    
      /* Process timer messages */
    if ((msg->message == WM_TIMER) || (msg->message == WM_SYSTIMER))
    {
	if (msg->lParam)
        {
/*            HOOK_CallHooks( WH_CALLWNDPROC, HC_ACTION, 0, FIXME ); */
	    return CallWindowProc16( (WNDPROC16)msg->lParam, msg->hwnd,
                                   msg->message, msg->wParam, GetTickCount() );
        }
    }

    if (!msg->hwnd) return 0;
    if (!(wndPtr = WIN_FindWndPtr( msg->hwnd ))) return 0;
    if (!wndPtr->winproc) return 0;
    painting = (msg->message == WM_PAINT);
    if (painting) wndPtr->flags |= WIN_NEEDS_BEGINPAINT;
/*    HOOK_CallHooks( WH_CALLWNDPROC, HC_ACTION, 0, FIXME ); */

    SPY_EnterMessage( SPY_DISPATCHMESSAGE16, msg->hwnd, msg->message,
                      msg->wParam, msg->lParam );
    retval = CallWindowProc16( wndPtr->winproc, msg->hwnd, msg->message,
                               msg->wParam, msg->lParam );
    SPY_ExitMessage( SPY_RESULT_OK16, msg->hwnd, msg->message, retval );

    if (painting && (wndPtr = WIN_FindWndPtr( msg->hwnd )) &&
        (wndPtr->flags & WIN_NEEDS_BEGINPAINT) && wndPtr->hrgnUpdate)
    {
	fprintf(stderr, "BeginPaint not called on WM_PAINT for hwnd %04x!\n", 
		msg->hwnd);
	wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;
        /* Validate the update region to avoid infinite WM_PAINT loop */
        ValidateRect32( msg->hwnd, NULL );
    }
    return retval;
}


/***********************************************************************
 *           RegisterWindowMessage16   (USER.118)
 */
WORD RegisterWindowMessage16( SEGPTR str )
{
    dprintf_msg(stddeb, "RegisterWindowMessage16: %08lx\n", (DWORD)str );
    return GlobalAddAtom16( str );
}


/***********************************************************************
 *           RegisterWindowMessage32A   (USER32.436)
 */
WORD RegisterWindowMessage32A( LPCSTR str )
{
    dprintf_msg(stddeb, "RegisterWindowMessage32A: %s\n", str );
    return GlobalAddAtom32A( str );
}


/***********************************************************************
 *           RegisterWindowMessage32W   (USER32.437)
 */
WORD RegisterWindowMessage32W( LPCWSTR str )
{
    dprintf_msg(stddeb, "RegisterWindowMessage32W: %p\n", str );
    return GlobalAddAtom32W( str );
}


/***********************************************************************
 *           GetTickCount    (USER.13) (KERNEL32.299)
 */
DWORD GetTickCount(void)
{
    struct timeval t;
    gettimeofday( &t, NULL );
    return ((t.tv_sec * 1000) + (t.tv_usec / 1000)) - MSG_WineStartTicks;
}


/***********************************************************************
 *           GetCurrentTime    (USER.15)
 *
 * (effectively identical to GetTickCount)
 */
DWORD GetCurrentTime(void)
{
    return GetTickCount();
}


/***********************************************************************
 *           InSendMessage    (USER.192)
 */
BOOL InSendMessage()
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) )))
        return 0;
    return (BOOL)queue->InSendMessageHandle;
}
