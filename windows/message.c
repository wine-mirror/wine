/*
 * Message queues related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

/*
 * This code assumes that there is only one Windows task (hence
 * one message queue).
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993, 1994";

#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>

#include "message.h"
#include "win.h"
#include "gdi.h"
#include "wineopts.h"
#include "sysmetrics.h"
#include "hook.h"
#include "win.h"
#include "event.h"
#include "winpos.h"
#include "stddebug.h"
/* #define DEBUG_MSG */
#include "debug.h"


#define HWND_BROADCAST  ((HWND)0xffff)

#define MAX_QUEUE_SIZE   120  /* Max. size of a message queue */


extern BOOL TIMER_CheckTimer( LONG *next, MSG *msg,
			      HWND hwnd, BOOL remove );  /* timer.c */

  /* System message queue (for hardware events) */
static HANDLE hmemSysMsgQueue = 0;
static MESSAGEQUEUE * sysMsgQueue = NULL;

  /* Application message queue (should be a list, one queue per task) */
static HANDLE hmemAppMsgQueue = 0;
static MESSAGEQUEUE * appMsgQueue = NULL;

  /* Double-click time */
static int doubleClickSpeed = 452;


/***********************************************************************
 *           MSG_CreateMsgQueue
 *
 * Create a message queue.
 */
static HANDLE MSG_CreateMsgQueue( int size )
{
    HANDLE hQueue;
    MESSAGEQUEUE * msgQueue;
    int queueSize;

    queueSize = sizeof(MESSAGEQUEUE) + size * sizeof(QMSG);
    if (!(hQueue = GlobalAlloc( GMEM_FIXED, queueSize ))) return 0;
    msgQueue = (MESSAGEQUEUE *) GlobalLock( hQueue );
    msgQueue->next = 0;
    msgQueue->hTask = 0;
    msgQueue->msgSize = sizeof(QMSG);
    msgQueue->msgCount = 0;
    msgQueue->nextMessage = 0;
    msgQueue->nextFreeMessage = 0;
    msgQueue->queueSize = size;
    msgQueue->GetMessageTimeVal = 0;
    msgQueue->GetMessagePosVal = 0;
    msgQueue->GetMessageExtraInfoVal = 0;
    msgQueue->lParam = 0;
    msgQueue->wParam = 0;
    msgQueue->msg = 0;
    msgQueue->hWnd = 0;
    msgQueue->wPostQMsg = 0;
    msgQueue->wExitCode = 0;
    msgQueue->InSendMessageHandle = 0;
    msgQueue->wPaintCount = 0;
    msgQueue->wTimerCount = 0;
    msgQueue->tempStatus = 0;
    msgQueue->status = 0;
    GlobalUnlock( hQueue );
    return hQueue;
}


/***********************************************************************
 *           MSG_CreateSysMsgQueue
 *
 * Create the system message queue, and set the double-click speed.
 * Must be called only once.
 */
BOOL MSG_CreateSysMsgQueue( int size )
{
    if (size > MAX_QUEUE_SIZE) size = MAX_QUEUE_SIZE;
    else if (size <= 0) size = 1;
    if (!(hmemSysMsgQueue = MSG_CreateMsgQueue( size ))) return FALSE;
    sysMsgQueue = (MESSAGEQUEUE *) GlobalLock( hmemSysMsgQueue );
    doubleClickSpeed = GetProfileInt( "windows", "DoubleClickSpeed", 452 );
    return TRUE;
}


/***********************************************************************
 *           MSG_AddMsg
 *
 * Add a message to the queue. Return FALSE if queue is full.
 */
static int MSG_AddMsg( MESSAGEQUEUE * msgQueue, MSG * msg, DWORD extraInfo )
{
    int pos;
  
    if (!msgQueue) return FALSE;
    pos = msgQueue->nextFreeMessage;

      /* Check if queue is full */
    if ((pos == msgQueue->nextMessage) && (msgQueue->msgCount > 0)) {
		fprintf(stderr,"MSG_AddMsg // queue is full !\n");
		return FALSE;
		}

      /* Store message */
    msgQueue->messages[pos].msg = *msg;
    msgQueue->messages[pos].extraInfo = extraInfo;
    if (pos < msgQueue->queueSize-1) pos++;
    else pos = 0;
    msgQueue->nextFreeMessage = pos;
    msgQueue->msgCount++;
    msgQueue->status |= QS_POSTMESSAGE;
    msgQueue->tempStatus |= QS_POSTMESSAGE;
    return TRUE;
}


/***********************************************************************
 *           MSG_FindMsg
 *
 * Find a message matching the given parameters. Return -1 if none available.
 */
static int MSG_FindMsg(MESSAGEQUEUE * msgQueue, HWND hwnd, int first, int last)
{
    int i, pos = msgQueue->nextMessage;

    if (!msgQueue->msgCount) return -1;
    if (!hwnd && !first && !last) return pos;
        
    for (i = 0; i < msgQueue->msgCount; i++)
    {
	MSG * msg = &msgQueue->messages[pos].msg;

	if (!hwnd || (msg->hwnd == hwnd))
	{
	    if (!first && !last) return pos;
	    if ((msg->message >= first) && (msg->message <= last)) return pos;
	}
	if (pos < msgQueue->queueSize-1) pos++;
	else pos = 0;
    }
    return -1;
}


/***********************************************************************
 *           MSG_RemoveMsg
 *
 * Remove a message from the queue (pos must be a valid position).
 */
static void MSG_RemoveMsg( MESSAGEQUEUE * msgQueue, int pos )
{
    if (pos >= msgQueue->nextMessage)
    {
	for ( ; pos > msgQueue->nextMessage; pos--)
	    msgQueue->messages[pos] = msgQueue->messages[pos-1];
	msgQueue->nextMessage++;
	if (msgQueue->nextMessage >= msgQueue->queueSize)
	    msgQueue->nextMessage = 0;
    }
    else
    {
	for ( ; pos < msgQueue->nextFreeMessage; pos++)
	    msgQueue->messages[pos] = msgQueue->messages[pos+1];
	if (msgQueue->nextFreeMessage) msgQueue->nextFreeMessage--;
	else msgQueue->nextFreeMessage = msgQueue->queueSize-1;
    }
    msgQueue->msgCount--;
    if (!msgQueue->msgCount) msgQueue->status &= ~QS_POSTMESSAGE;
    msgQueue->tempStatus = 0;
}


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
static BOOL MSG_TranslateMouseMsg( MSG *msg, BOOL remove )
{
    BOOL eatMsg = FALSE;
    LONG hittest_result;
    static DWORD lastClickTime = 0;
    static WORD  lastClickMsg = 0;
    static POINT lastClickPos = { 0, 0 };

    BOOL mouseClick = ((msg->message == WM_LBUTTONDOWN) ||
		       (msg->message == WM_RBUTTONDOWN) ||
		       (msg->message == WM_MBUTTONDOWN));

      /* Find the window */

    if (GetCapture())
    {
	msg->hwnd = GetCapture();
	msg->lParam = MAKELONG( msg->pt.x, msg->pt.y );
	ScreenToClient( msg->hwnd, (LPPOINT)&msg->lParam );
	return TRUE;  /* No need to further process the message */
    }
    else msg->hwnd = WindowFromPoint( msg->pt );

      /* Send the WM_NCHITTEST message */

    hittest_result = SendMessage( msg->hwnd, WM_NCHITTEST, 0,
				  MAKELONG( msg->pt.x, msg->pt.y ) );
    while ((hittest_result == HTTRANSPARENT) && (msg->hwnd))
    {
	msg->hwnd = WINPOS_NextWindowFromPoint( msg->hwnd, msg->pt );
	if (msg->hwnd)
	    hittest_result = SendMessage( msg->hwnd, WM_NCHITTEST, 0,
					  MAKELONG( msg->pt.x, msg->pt.y ) );
    }
    if (!msg->hwnd) msg->hwnd = GetDesktopWindow();

      /* Send the WM_PARENTNOTIFY message */

    if (mouseClick) WIN_SendParentNotify( msg->hwnd, msg->message,
					  MAKELONG( msg->pt.x, msg->pt.y ) );

      /* Activate the window if needed */

    if (mouseClick)
    {
	HWND parent, hwndTop = msg->hwnd;	
	while ((parent = GetParent(hwndTop)) != 0) hwndTop = parent;
	if (hwndTop != GetActiveWindow())
	{
	    LONG ret = SendMessage( msg->hwnd, WM_MOUSEACTIVATE, hwndTop,
				    MAKELONG( hittest_result, msg->message ) );
	    if ((ret == MA_ACTIVATEANDEAT) || (ret == MA_NOACTIVATEANDEAT))
		eatMsg = TRUE;
	    if ((ret == MA_ACTIVATE) || (ret == MA_ACTIVATEANDEAT))
	    {
		SetWindowPos( hwndTop, HWND_TOP, 0, 0, 0, 0,
			      SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );
		WINPOS_ChangeActiveWindow( hwndTop, TRUE );
	    }
	}
    }

      /* Send the WM_SETCURSOR message */

    SendMessage( msg->hwnd, WM_SETCURSOR, msg->hwnd,
		 MAKELONG( hittest_result, msg->message ));
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

	if (dbl_click && (hittest_result == HTCLIENT))
	{
	    /* Check whether window wants the double click message. */
	    WND * wndPtr = WIN_FindWndPtr( msg->hwnd );
            if (!wndPtr || !(WIN_CLASS_STYLE(wndPtr) & CS_DBLCLKS))
                dbl_click = FALSE;
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

    msg->lParam = MAKELONG( msg->pt.x, msg->pt.y );
    if (hittest_result == HTCLIENT)
    {
	ScreenToClient( msg->hwnd, (LPPOINT)&msg->lParam );
    }
    else
    {
	msg->wParam = hittest_result;
	msg->message += WM_NCLBUTTONDOWN - WM_LBUTTONDOWN;
    }
    
    return TRUE;
}


/***********************************************************************
 *           MSG_TranslateKeyboardMsg
 *
 * Translate an keyboard hardware event into a real message.
 * Return value indicates whether the translated message must be passed
 * to the user.
 */
static BOOL MSG_TranslateKeyboardMsg( MSG *msg )
{
      /* Should check Ctrl-Esc and PrintScreen here */

    msg->hwnd = GetFocus();
    if (!msg->hwnd)
    {
	  /* Send the message to the active window instead,  */
	  /* translating messages to their WM_SYS equivalent */
	msg->hwnd = GetActiveWindow();
	msg->message += WM_SYSKEYDOWN - WM_KEYDOWN;
    }
    return TRUE;
}


/***********************************************************************
 *           MSG_PeekHardwareMsg
 *
 * Peek for a hardware message matching the hwnd and message filters.
 */
static BOOL MSG_PeekHardwareMsg( MSG *msg, HWND hwnd, WORD first, WORD last,
                                 BOOL remove )
{
    int i, pos = sysMsgQueue->nextMessage;

    for (i = 0; i < sysMsgQueue->msgCount; i++, pos++)
    {
	*msg = sysMsgQueue->messages[pos].msg;

          /* Translate message */

        if ((msg->message >= WM_MOUSEFIRST) && (msg->message <= WM_MOUSELAST))
        {
            if (!MSG_TranslateMouseMsg( msg, remove )) continue;
        }
        else if ((msg->message >= WM_KEYFIRST) && (msg->message <= WM_KEYLAST))
        {
            if (!MSG_TranslateKeyboardMsg( msg )) continue;
        }
        else continue;  /* Should never happen */

          /* Check message against filters */

        if (hwnd && (msg->hwnd != hwnd)) continue;
        if ((first || last) && 
            ((msg->message < first) || (msg->message > last))) continue;
        if (remove) MSG_RemoveMsg( sysMsgQueue, pos );
        return TRUE;
    }
    return FALSE;
}


/**********************************************************************
 *		SetDoubleClickTime  (USER.20)
 */
void SetDoubleClickTime( WORD interval )
{
    if (interval == 0)
	doubleClickSpeed = 500;
    else
	doubleClickSpeed = interval;
}		


/**********************************************************************
 *		GetDoubleClickTime  (USER.21)
 */
WORD GetDoubleClickTime()
{
	return (WORD)doubleClickSpeed;
}		


/***********************************************************************
 *           MSG_IncPaintCount
 */
void MSG_IncPaintCount( HANDLE hQueue )
{
    if (hQueue != hmemAppMsgQueue) return;
    appMsgQueue->wPaintCount++;
    appMsgQueue->status |= QS_PAINT;
    appMsgQueue->tempStatus |= QS_PAINT;    
}


/***********************************************************************
 *           MSG_DecPaintCount
 */
void MSG_DecPaintCount( HANDLE hQueue )
{
    if (hQueue != hmemAppMsgQueue) return;
    appMsgQueue->wPaintCount--;
    if (!appMsgQueue->wPaintCount) appMsgQueue->status &= ~QS_PAINT;
}


/***********************************************************************
 *           MSG_IncTimerCount
 */
void MSG_IncTimerCount( HANDLE hQueue )
{
    if (hQueue != hmemAppMsgQueue) return;
    appMsgQueue->wTimerCount++;
    appMsgQueue->status |= QS_TIMER;
    appMsgQueue->tempStatus |= QS_TIMER;
}


/***********************************************************************
 *           MSG_DecTimerCount
 */
void MSG_DecTimerCount( HANDLE hQueue )
{
    if (hQueue != hmemAppMsgQueue) return;
    appMsgQueue->wTimerCount--;
    if (!appMsgQueue->wTimerCount) appMsgQueue->status &= ~QS_TIMER;
}


/***********************************************************************
 *           hardware_event
 *
 * Add an event to the system message queue.
 * Note: the position is relative to the desktop window.
 */
void hardware_event( WORD message, WORD wParam, LONG lParam,
		     int xPos, int yPos, DWORD time, DWORD extraInfo )
{
    MSG *msg;
    int pos;
  
    if (!sysMsgQueue) return;
    pos = sysMsgQueue->nextFreeMessage;

      /* Merge with previous event if possible */

    if ((message == WM_MOUSEMOVE) && sysMsgQueue->msgCount)
    {
        if (pos > 0) pos--;
        else pos = sysMsgQueue->queueSize - 1;
	msg = &sysMsgQueue->messages[pos].msg;
	if ((msg->message == message) && (msg->wParam == wParam))
            sysMsgQueue->msgCount--;  /* Merge events */
        else
            pos = sysMsgQueue->nextFreeMessage;  /* Don't merge */
    }

      /* Check if queue is full */

    if ((pos == sysMsgQueue->nextMessage) && sysMsgQueue->msgCount)
    {
        /* Queue is full, beep (but not on every mouse motion...) */
        if (message != WM_MOUSEMOVE) MessageBeep(0);
        return;
    }

      /* Store message */

    msg = &sysMsgQueue->messages[pos].msg;
    msg->hwnd    = 0;
    msg->message = message;
    msg->wParam  = wParam;
    msg->lParam  = lParam;
    msg->time    = time;
    msg->pt.x    = xPos & 0xffff;
    msg->pt.y    = yPos & 0xffff;
    sysMsgQueue->messages[pos].extraInfo = extraInfo;
    if (pos < sysMsgQueue->queueSize - 1) pos++;
    else pos = 0;
    sysMsgQueue->nextFreeMessage = pos;
    sysMsgQueue->msgCount++;
}

		    
/***********************************************************************
 *           MSG_GetHardwareMessage
 *
 * Like GetMessage(), but only return mouse and keyboard events.
 * Used internally for window moving and resizing. Mouse messages
 * are not translated.
 * Warning: msg->hwnd is always 0.
 */
BOOL MSG_GetHardwareMessage( LPMSG msg )
{
    int pos;
    XEvent event;

    while(1)
    {    
	if ((pos = MSG_FindMsg( sysMsgQueue, 0, 0, 0 )) != -1)
	{
	    *msg = sysMsgQueue->messages[pos].msg;
	    MSG_RemoveMsg( sysMsgQueue, pos );
	    break;
	}
	XNextEvent( display, &event );
	EVENT_ProcessEvent( &event );
    }
    return TRUE;
}


/***********************************************************************
 *           SetTaskQueue  (KERNEL.34)
 */
WORD SetTaskQueue( HANDLE hTask, HANDLE hQueue )
{
    HANDLE prev = hmemAppMsgQueue;
    hmemAppMsgQueue = hQueue;
    return prev;
}


/***********************************************************************
 *           GetTaskQueue  (KERNEL.35)
 */
WORD GetTaskQueue( HANDLE hTask )
{
    return hmemAppMsgQueue;
}


/***********************************************************************
 *           SetMessageQueue  (USER.266)
 */
BOOL SetMessageQueue( int size )
{
    HANDLE hQueue;

    if ((size > MAX_QUEUE_SIZE) || (size <= 0)) return TRUE;

      /* Free the old message queue */
    if ((hQueue = GetTaskQueue(0)) != 0)
    {
	GlobalUnlock( hQueue );
	GlobalFree( hQueue );
    }
  
    if (!(hQueue = MSG_CreateMsgQueue( size ))) return FALSE;
    SetTaskQueue( 0, hQueue );
    appMsgQueue = (MESSAGEQUEUE *)GlobalLock( hQueue );
    return TRUE;
}


/***********************************************************************
 *           PostQuitMessage   (USER.6)
 */
void PostQuitMessage( int exitCode )
{
    if (!appMsgQueue) return;
    appMsgQueue->wPostQMsg = TRUE;
    appMsgQueue->wExitCode = exitCode;
}


/***********************************************************************
 *           GetQueueStatus   (USER.334)
 */
DWORD GetQueueStatus( int flags )
{
    unsigned long ret = (appMsgQueue->status << 16) | appMsgQueue->tempStatus;
    appMsgQueue->tempStatus = 0;
    return ret & ((flags << 16) | flags);
}


/***********************************************************************
 *           GetInputState   (USER.335)
 */
BOOL GetInputState()
{
    return appMsgQueue->status & (QS_KEY | QS_MOUSEBUTTON);
}


/***********************************************************************
 *           MSG_Synchronize
 *
 * Synchronize with the X server. Should not be used too often.
 */
void MSG_Synchronize()
{
    XEvent event;

    XSync( display, False );
    while (XPending( display ))
    {
	XNextEvent( display, &event );
	EVENT_ProcessEvent( &event );
    }    
}


/***********************************************************************
 *           MSG_WaitXEvent
 *
 * Wait for an X event, but at most maxWait milliseconds (-1 for no timeout).
 * Return TRUE if an event is pending, FALSE on timeout or error
 * (for instance lost connection with the server).
 */
BOOL MSG_WaitXEvent( LONG maxWait )
{
    fd_set read_set;
    struct timeval timeout;
    XEvent event;
    int fd = ConnectionNumber(display);

    if (!XPending(display) && (maxWait != -1))
    {
        FD_ZERO( &read_set );
        FD_SET( fd, &read_set );
        timeout.tv_sec = maxWait / 1000;
        timeout.tv_usec = (maxWait % 1000) * 1000;
        if (select( fd+1, &read_set, NULL, NULL, &timeout ) != 1)
            return FALSE;  /* Timeout or error */
    }

    /* Process the event (and possibly others that occurred in the meantime) */
    do
    {
        XNextEvent( display, &event );
        EVENT_ProcessEvent( &event );
    }
    while (XPending( display ));
    return TRUE;
}


/***********************************************************************
 *           MSG_PeekMessage
 */
static BOOL MSG_PeekMessage( MESSAGEQUEUE * msgQueue, LPMSG msg, HWND hwnd,
			     WORD first, WORD last, WORD flags, BOOL peek )
{
    int pos, mask;
    LONG nextExp;  /* Next timer expiration time */

    if (first || last)
    {
	mask = QS_POSTMESSAGE;  /* Always selectioned */
	if ((first <= WM_KEYLAST) && (last >= WM_KEYFIRST)) mask |= QS_KEY;
	if ((first <= WM_MOUSELAST) && (last >= WM_MOUSEFIRST)) mask |= QS_MOUSE;
	if ((first <= WM_TIMER) && (last >= WM_TIMER)) mask |= QS_TIMER;
	if ((first <= WM_SYSTIMER) && (last >= WM_SYSTIMER)) mask |= QS_TIMER;
	if ((first <= WM_PAINT) && (last >= WM_PAINT)) mask |= QS_PAINT;
    }
    else mask = QS_MOUSE | QS_KEY | QS_POSTMESSAGE | QS_TIMER | QS_PAINT;

    while(1)
    {    
	  /* First handle a message put by SendMessage() */
	if (msgQueue->status & QS_SENDMESSAGE)
	{
	    if (!hwnd || (msgQueue->hWnd == hwnd))
	    {
		if ((!first && !last) || 
		    ((msgQueue->msg >= first) && (msgQueue->msg <= last)))
		{
		    msg->hwnd    = msgQueue->hWnd;
		    msg->message = msgQueue->msg;
		    msg->wParam  = msgQueue->wParam;
		    msg->lParam  = msgQueue->lParam;
		    if (flags & PM_REMOVE) msgQueue->status &= ~QS_SENDMESSAGE;
		    break;
		}
	    }
	}
    
	  /* Now find a normal message */
	pos = MSG_FindMsg( msgQueue, hwnd, first, last );
	if (pos != -1)
	{
	    QMSG *qmsg = &msgQueue->messages[pos];
	    *msg = qmsg->msg;
	    msgQueue->GetMessageTimeVal      = msg->time;
	    msgQueue->GetMessagePosVal       = *(DWORD *)&msg->pt;
	    msgQueue->GetMessageExtraInfoVal = qmsg->extraInfo;

	    if (flags & PM_REMOVE) MSG_RemoveMsg( msgQueue, pos );
	    break;
	}

	  /* Now find a hardware event */
        if (MSG_PeekHardwareMsg( msg, hwnd, first, last, flags & PM_REMOVE ))
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

	  /* Now find a WM_PAINT message */
	if ((msgQueue->status & QS_PAINT) && (mask & QS_PAINT))
	{
	    msg->hwnd = WIN_FindWinToRepaint( hwnd );
	    msg->message = WM_PAINT;
	    msg->wParam = 0;
	    msg->lParam = 0;
	    if (msg->hwnd != 0) break;
	}

	  /* Finally handle WM_TIMER messages */
	if ((msgQueue->status & QS_TIMER) && (mask & QS_TIMER))
	{
	    if (TIMER_CheckTimer( &nextExp, msg, hwnd, flags & PM_REMOVE ))
		break;  /* Got a timer msg */
	}
	else nextExp = -1;  /* No timeout needed */

	  /* Wait until something happens */
        if (peek)
        {
            if (!MSG_WaitXEvent( 0 )) return FALSE;  /* No pending event */
        }
        else  /* Wait for an event, then restart the loop */
            MSG_WaitXEvent( nextExp );
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
BOOL MSG_InternalGetMessage( LPMSG msg, HWND hwnd, HWND hwndOwner, short code,
			     WORD flags, BOOL sendIdle ) 
{
    for (;;)
    {
	if (sendIdle)
	{
	    if (!MSG_PeekMessage( appMsgQueue, msg, 0, 0, 0, flags, TRUE ))
	    {
		  /* No message present -> send ENTERIDLE and wait */
		SendMessage( hwndOwner, WM_ENTERIDLE, code, (LPARAM)hwnd );
		MSG_PeekMessage( appMsgQueue, msg, 0, 0, 0, flags, FALSE );
	    }
	}
	else  /* Always wait for a message */
	    MSG_PeekMessage( appMsgQueue, msg, 0, 0, 0, flags, FALSE );

	if (!CallMsgFilter( msg, code )) return (msg->message != WM_QUIT);

	  /* Message filtered -> remove it from the queue */
	  /* if it's still there. */
	if (!(flags & PM_REMOVE))
	    MSG_PeekMessage( appMsgQueue, msg, 0, 0, 0, PM_REMOVE, TRUE );
    }
}


/***********************************************************************
 *           PeekMessage   (USER.109)
 */
BOOL PeekMessage( LPMSG msg, HWND hwnd, WORD first, WORD last, WORD flags )
{
    return MSG_PeekMessage( appMsgQueue, msg, hwnd, first, last, flags, TRUE );
}


/***********************************************************************
 *           GetMessage   (USER.108)
 */
BOOL GetMessage( LPMSG msg, HWND hwnd, WORD first, WORD last ) 
{
    MSG_PeekMessage( appMsgQueue, msg, hwnd, first, last, PM_REMOVE, FALSE );
    CALL_SYSTEM_HOOK( WH_GETMESSAGE, 0, 0, (LPARAM)msg );
    CALL_TASK_HOOK( WH_GETMESSAGE, 0, 0, (LPARAM)msg );
    return (msg->message != WM_QUIT);
}



/***********************************************************************
 *           PostMessage   (USER.110)
 */
BOOL PostMessage( HWND hwnd, WORD message, WORD wParam, LONG lParam )
{
    MSG 	msg;
    WND 	*wndPtr;

	if (hwnd == HWND_BROADCAST) {
	    dprintf_msg(stddeb,"PostMessage // HWND_BROADCAST !\n");
	    hwnd = GetTopWindow(GetDesktopWindow());
	    while (hwnd) {
	        if (!(wndPtr = WIN_FindWndPtr(hwnd))) break;
			if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION) {
				dprintf_msg(stddeb,"BROADCAST Message to hWnd=%04X m=%04X w=%04X l=%08X !\n", 
							hwnd, message, wParam, lParam);
				PostMessage(hwnd, message, wParam, lParam);
				}
/*				{
				char	str[128];
				GetWindowText(hwnd, str, sizeof(str));
				dprintf_msg(stddeb, "BROADCAST GetWindowText()='%s' !\n", str); 
				}*/
			hwnd = wndPtr->hwndNext;
			}
		dprintf_msg(stddeb,"PostMessage // End of HWND_BROADCAST !\n");
		return TRUE;
		}

	wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr || !wndPtr->hmemTaskQ) return FALSE;
    msg.hwnd    = hwnd;
    msg.message = message;
    msg.wParam  = wParam;
    msg.lParam  = lParam;
    msg.time    = GetTickCount();
    msg.pt.x    = 0;
    msg.pt.y    = 0;

    return MSG_AddMsg( appMsgQueue, &msg, 0 );
}


/***********************************************************************
 *           SendMessage   (USER.111)
 */
LONG SendMessage( HWND hwnd, WORD msg, WORD wParam, LONG lParam )
{
    WND * wndPtr;

    wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    return CallWindowProc( wndPtr->lpfnWndProc, hwnd, msg, wParam, lParam );
}


/***********************************************************************
 *           WaitMessage    (USER.112)
 */
void WaitMessage( void )
{
    MSG msg;
    LONG nextExp = -1;  /* Next timer expiration time */

    if ((appMsgQueue->wPostQMsg) || 
        (appMsgQueue->status & (QS_SENDMESSAGE | QS_PAINT)) ||
        (appMsgQueue->msgCount) || (sysMsgQueue->msgCount) )
        return;
    if ((appMsgQueue->status & QS_TIMER) && 
        TIMER_CheckTimer( &nextExp, &msg, 0, FALSE))
        return;
    MSG_WaitXEvent( nextExp );
}


/***********************************************************************
 *           TranslateMessage   (USER.113)
 */
BOOL TranslateMessage( LPMSG msg )
{
    int message = msg->message;
    
    if ((message == WM_KEYDOWN) || (message == WM_KEYUP) ||
	(message == WM_SYSKEYDOWN) || (message == WM_SYSKEYUP))
    {
	dprintf_msg(stddeb, "Translating key message\n" );
	return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           DispatchMessage   (USER.114)
 */
LONG DispatchMessage( LPMSG msg )
{
    WND * wndPtr;
    LONG retval;
    int painting;
    
    dprintf_msg(stddeb, "Dispatch message hwnd=%04x msg=0x%x w=%d l=%ld time=%lu pt=%d,%d\n",
	    msg->hwnd, msg->message, msg->wParam, msg->lParam, 
	    msg->time, msg->pt.x, msg->pt.y );

      /* Process timer messages */
    if ((msg->message == WM_TIMER) || (msg->message == WM_SYSTIMER))
    {
	if (msg->lParam)
	    return CallWindowProc( (WNDPROC)msg->lParam, msg->hwnd,
				   msg->message, msg->wParam, GetTickCount() );
    }

    if (!msg->hwnd) return 0;
    if (!(wndPtr = WIN_FindWndPtr( msg->hwnd ))) return 0;
    if (!wndPtr->lpfnWndProc) return 0;
    painting = (msg->message == WM_PAINT);
    if (painting) wndPtr->flags |= WIN_NEEDS_BEGINPAINT;
    retval = CallWindowProc( wndPtr->lpfnWndProc, msg->hwnd, msg->message,
			     msg->wParam, msg->lParam );
    if (painting && IsWindow(msg->hwnd) &&
        (wndPtr->flags & WIN_NEEDS_BEGINPAINT))
    {
	fprintf(stderr, "BeginPaint not called on WM_PAINT for hwnd %d!\n", 
		msg->hwnd);
	wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;
    }
    return retval;
}


/***********************************************************************
 *           GetMessagePos   (USER.119)
 */
DWORD GetMessagePos(void)
{
    return appMsgQueue->GetMessagePosVal;
}


/***********************************************************************
 *           GetMessageTime   (USER.120)
 */
LONG GetMessageTime(void)
{
    return appMsgQueue->GetMessageTimeVal;
}


/***********************************************************************
 *           GetMessageExtraInfo   (USER.288)
 */
LONG GetMessageExtraInfo(void)
{
    return appMsgQueue->GetMessageExtraInfoVal;
}


/***********************************************************************
 *           RegisterWindowMessage   (USER.118)
 */
WORD RegisterWindowMessage( LPCSTR str )
{
	WORD	wRet;
    dprintf_msg(stddeb, "RegisterWindowMessage: '%s'\n", str );
	wRet = GlobalAddAtom( str );
    return wRet;
}


/***********************************************************************
 *           GetTickCount    (USER.13)
 */
DWORD GetTickCount()
{
    struct timeval t;
    gettimeofday( &t, NULL );
    return (t.tv_sec * 1000) + (t.tv_usec / 1000);
}
