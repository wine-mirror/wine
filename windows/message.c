/*
 * Message queues related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>

#include "message.h"
#include "win.h"
#include "gdi.h"
#include "sysmetrics.h"
#include "hook.h"
#include "event.h"
#include "spy.h"
#include "winpos.h"
#include "atom.h"
#include "dde.h"
#include "stddebug.h"
/* #define DEBUG_MSG */
#include "debug.h"

#define HWND_BROADCAST  ((HWND)0xffff)
#define MAX_QUEUE_SIZE   120  /* Max. size of a message queue */


extern BOOL TIMER_CheckTimer( LONG *next, MSG *msg,
			      HWND hwnd, BOOL remove );  /* timer.c */

/* ------- Internal Queues ------ */

static HANDLE hmemSysMsgQueue = 0;
static MESSAGEQUEUE *sysMsgQueue = NULL;

HANDLE hActiveQ_G      = 0;
static HANDLE hFirstQueue = 0;

/* ------- Miscellaneous ------ */
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
    if (!(hQueue = GlobalAlloc( GMEM_FIXED | GMEM_ZEROINIT, queueSize )))
        return 0;
    msgQueue = (MESSAGEQUEUE *) GlobalLock( hQueue );
    msgQueue->msgSize = sizeof(QMSG);
    msgQueue->queueSize = size;
    msgQueue->wWinVersion = 0;  /* FIXME? */
    GlobalUnlock( hQueue );
    return hQueue;
}

/***********************************************************************
 *	     MSG_DeleteMsgQueue
 */
BOOL MSG_DeleteMsgQueue( HANDLE hQueue )
{
 MESSAGEQUEUE * msgQueue = (MESSAGEQUEUE*)GlobalLock(hQueue);

 if( !hQueue )
   {
	dprintf_msg(stddeb,"DeleteMsgQueue: invalid argument.\n");
	return 0;
   }

 if( !msgQueue ) return 0;

 if( hQueue == hFirstQueue )
	hFirstQueue = msgQueue->next;
 else if( hFirstQueue )
	{
	  MESSAGEQUEUE *msgQ = (MESSAGEQUEUE*)GlobalLock(hFirstQueue);

	  /* walk up queue list and relink if needed */
	  while( msgQ->next )
	    {
		if( msgQ->next == hQueue )
		    msgQ->next = msgQueue->next;

		/* should check for intertask sendmessages here */


	        msgQ = (MESSAGEQUEUE*)GlobalLock(msgQ->next);
	    }
	}

 GlobalFree( hQueue );
 return 1;
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
static int MSG_AddMsg( HANDLE hQueue, MSG * msg, DWORD extraInfo )
{
    int pos;
    MESSAGEQUEUE *msgQueue;

    if (!(msgQueue = (MESSAGEQUEUE *)GlobalLock( hQueue ))) return FALSE;
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

    dprintf_msg(stddeb,"MSG_FindMsg: hwnd=0x"NPFMT"\n\n", hwnd );

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
 *	     MSG_GetQueueTask
 */
HTASK MSG_GetQueueTask( HANDLE hQueue )
{
    MESSAGEQUEUE *msgQ = GlobalLock( hQueue );

    return (msgQ) ? msgQ->hTask : 0 ;
}

/***********************************************************************
 *           MSG_GetWindowForEvent
 *
 * Find the window and hittest for a mouse event.
 */
static INT MSG_GetWindowForEvent( POINT pt, HWND *phwnd )
{
    WND *wndPtr;
    HWND hwnd;
    INT hittest = HTERROR;
    INT x, y;

    *phwnd = hwnd = GetDesktopWindow();
    x = pt.x;
    y = pt.y;
    while (hwnd)
    {
	  /* If point is in window, and window is visible, and it  */
          /* is enabled (or it's a top-level window), then explore */
	  /* its children. Otherwise, go to the next window.       */

	wndPtr = WIN_FindWndPtr( hwnd );
	if ((wndPtr->dwStyle & WS_VISIBLE) &&
            (!(wndPtr->dwStyle & WS_DISABLED) ||
             !(wndPtr->dwStyle & WS_CHILD)) &&
            (x >= wndPtr->rectWindow.left) &&
            (x < wndPtr->rectWindow.right) &&
	    (y >= wndPtr->rectWindow.top) &&
            (y < wndPtr->rectWindow.bottom))
	{
	    *phwnd = hwnd;
	    x -= wndPtr->rectClient.left;
	    y -= wndPtr->rectClient.top;
              /* If window is minimized or disabled, ignore its children */
            if ((wndPtr->dwStyle & WS_MINIMIZE) ||
                (wndPtr->dwStyle & WS_DISABLED)) break;
	    hwnd = wndPtr->hwndChild;
	}
	else hwnd = wndPtr->hwndNext;
    }

    /* Make point relative to parent again */

    wndPtr = WIN_FindWndPtr( *phwnd );
    x += wndPtr->rectClient.left;
    y += wndPtr->rectClient.top;

    /* Send the WM_NCHITTEST message */

    while (*phwnd)
    {
        wndPtr = WIN_FindWndPtr( *phwnd );
        if (wndPtr->dwStyle & WS_DISABLED) hittest = HTERROR;
        else hittest = (INT)SendMessage( *phwnd, WM_NCHITTEST, 0,
                                         MAKELONG( pt.x, pt.y ) );
        if (hittest != HTTRANSPARENT) break;  /* Found the window */
        hwnd = wndPtr->hwndNext;
        while (hwnd)
        {
            WND *nextPtr = WIN_FindWndPtr( hwnd );
            if ((nextPtr->dwStyle & WS_VISIBLE) &&
                (x >= nextPtr->rectWindow.left) &&
                (x < nextPtr->rectWindow.right) &&
                (y >= nextPtr->rectWindow.top) &&
                (y < nextPtr->rectWindow.bottom)) break;
            hwnd = nextPtr->hwndNext;
        }
        if (hwnd) *phwnd = hwnd; /* Found a suitable sibling */
        else  /* Go back to the parent */
        {
            if (!(*phwnd = wndPtr->hwndParent)) break;
            wndPtr = WIN_FindWndPtr( *phwnd );
            x += wndPtr->rectClient.left;
            y += wndPtr->rectClient.top;
        }
    }

    if (!*phwnd) *phwnd = GetDesktopWindow();
    return hittest;
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
    INT hittest;
    static DWORD lastClickTime = 0;
    static WORD  lastClickMsg = 0;
    static POINT lastClickPos = { 0, 0 };
    POINT pt = msg->pt;
    MOUSEHOOKSTRUCT hook = { msg->pt, 0, HTCLIENT, 0 };

    BOOL mouseClick = ((msg->message == WM_LBUTTONDOWN) ||
		       (msg->message == WM_RBUTTONDOWN) ||
		       (msg->message == WM_MBUTTONDOWN));

      /* Find the window */

    if (GetCapture())
    {
	msg->hwnd = GetCapture();
	ScreenToClient( msg->hwnd, &pt );
	msg->lParam = MAKELONG( pt.x, pt.y );
        /* No need to further process the message */
        hook.hwnd = msg->hwnd;
        return !HOOK_CallHooks( WH_MOUSE, remove ? HC_ACTION : HC_NOREMOVE,
                                msg->message, (LPARAM)MAKE_SEGPTR(&hook));
    }
   
    if ((hittest = MSG_GetWindowForEvent( msg->pt, &msg->hwnd )) != HTERROR)
    {

        /* Send the WM_PARENTNOTIFY message */

        if (mouseClick) WIN_SendParentNotify( msg->hwnd, msg->message, 0,
                                            MAKELONG( msg->pt.x, msg->pt.y ) );

        /* Activate the window if needed */

        if (mouseClick)
        {
            HWND hwndTop = WIN_GetTopParent( msg->hwnd );
            if (hwndTop != GetActiveWindow())
            {
                LONG ret = SendMessage( msg->hwnd, WM_MOUSEACTIVATE,
					(WPARAM)hwndTop,
                                        MAKELONG( hittest, msg->message ) );
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
    }

      /* Send the WM_SETCURSOR message */

    SendMessage( msg->hwnd, WM_SETCURSOR, (WPARAM)msg->hwnd,
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

    if (hittest == HTCLIENT)
        ScreenToClient( msg->hwnd, &pt );
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
static BOOL MSG_TranslateKeyboardMsg( MSG *msg, BOOL remove )
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
    return !HOOK_CallHooks( WH_KEYBOARD, remove ? HC_ACTION : HC_NOREMOVE,
                            msg->wParam, msg->lParam );
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
            HARDWAREHOOKSTRUCT hook = { msg->hwnd, msg->message,
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
            MSG tmpMsg = *msg; /* FIXME */
            HOOK_CallHooks( WH_JOURNALRECORD, HC_ACTION,
                            0, (LPARAM)MAKE_SEGPTR(&tmpMsg) );
            MSG_RemoveMsg( sysMsgQueue, pos );
        }
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
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( hQueue ))) return;
    queue->wPaintCount++;
    queue->status |= QS_PAINT;
    queue->tempStatus |= QS_PAINT;    
}


/***********************************************************************
 *           MSG_DecPaintCount
 */
void MSG_DecPaintCount( HANDLE hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( hQueue ))) return;
    queue->wPaintCount--;
    if (!queue->wPaintCount) queue->status &= ~QS_PAINT;
}


/***********************************************************************
 *           MSG_IncTimerCount
 */
void MSG_IncTimerCount( HANDLE hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( hQueue ))) return;
    queue->wTimerCount++;
    queue->status |= QS_TIMER;
    queue->tempStatus |= QS_TIMER;
}


/***********************************************************************
 *           MSG_DecTimerCount
 */
void MSG_DecTimerCount( HANDLE hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( hQueue ))) return;
    queue->wTimerCount--;
    if (!queue->wTimerCount) queue->status &= ~QS_TIMER;
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
 *           SetMessageQueue  (USER.266)
 */
BOOL SetMessageQueue( int size )
{
    HGLOBAL  	  hQueue    = 0;
    HGLOBAL  	  hNextQueue= hFirstQueue;
    MESSAGEQUEUE *queuePrev = NULL;
    MESSAGEQUEUE *queuePtr;

    if ((size > MAX_QUEUE_SIZE) || (size <= 0)) return TRUE;

      /* Free the old message queue */
    if ((hQueue = GetTaskQueue(0)) != 0)
    {
	MESSAGEQUEUE *queuePtr = (MESSAGEQUEUE *)GlobalLock( hQueue );
	
	if( queuePtr )
	{
	    MESSAGEQUEUE *msgQ = (MESSAGEQUEUE*)GlobalLock(hFirstQueue);

	    hNextQueue = queuePtr->next;

            if( msgQ )
		if( hQueue != hFirstQueue )
		    while( msgQ->next )
		    { 
			if( msgQ->next == hQueue )
			{ 
			    queuePrev = msgQ;
			    break;
			} 
			msgQ = (MESSAGEQUEUE*)GlobalLock(msgQ->next);
		    }
	    GlobalUnlock( hQueue );
	    MSG_DeleteMsgQueue( hQueue );
	}
    }
  
    if( !(hQueue = MSG_CreateMsgQueue( size ))) 
    {
	if(queuePrev) 
	    /* it did have a queue */
	    queuePrev->next = hNextQueue;
	return FALSE;
    }
  
    queuePtr = (MESSAGEQUEUE *)GlobalLock( hQueue );
    queuePtr->hTask = GetCurrentTask();
    queuePtr->next  = hNextQueue;

    if( !queuePrev )  
         hFirstQueue = hQueue;
    else
	 queuePrev->next = hQueue;

    SetTaskQueue( 0, hQueue );
    return TRUE;
}


/***********************************************************************
 *           GetWindowTask  (USER.224)
 */
HTASK GetWindowTask( HWND hwnd )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    MESSAGEQUEUE *queuePtr;

    if (!wndPtr) return 0;
    queuePtr = (MESSAGEQUEUE *)GlobalLock( wndPtr->hmemTaskQ );
    if (!queuePtr) return 0;
    return queuePtr->hTask;
}


/***********************************************************************
 *           PostQuitMessage   (USER.6)
 */
void PostQuitMessage( int exitCode )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetTaskQueue(0) ))) return;
    queue->wPostQMsg = TRUE;
    queue->wExitCode = exitCode;
}


/***********************************************************************
 *           GetQueueStatus   (USER.334)
 */
DWORD GetQueueStatus( int flags )
{
    MESSAGEQUEUE *queue;
    DWORD ret;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetTaskQueue(0) ))) return 0;
    ret = MAKELONG( queue->tempStatus, queue->status );
    queue->tempStatus = 0;
    return ret & MAKELONG( flags, flags );
}


/***********************************************************************
 *           GetInputState   (USER.335)
 */
BOOL GetInputState()
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetTaskQueue(0) ))) return FALSE;
    return queue->status & (QS_KEY | QS_MOUSEBUTTON);
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

	timeout.tv_usec = (maxWait % 1000) * 1000;
	timeout.tv_sec = maxWait / 1000;

#ifdef CONFIG_IPC
	sigsetjmp(env_wait_x, 1);
	stop_wait_op= CONT;
	    
	if (DDE_GetRemoteMessage()) {
	    while(DDE_GetRemoteMessage())
		;
	    return TRUE;
	}
	stop_wait_op= STOP_WAIT_X;
	/* The code up to the next "stop_wait_op= CONT" must be reentrant  */
	if (select( fd+1, &read_set, NULL, NULL, &timeout ) != 1 &&
	    !XPending(display)) {
	    stop_wait_op= CONT;
	    return FALSE;
	} else {
	    stop_wait_op= CONT;
	}
#else  /* CONFIG_IPC */
	if (select( fd+1, &read_set, NULL, NULL, &timeout ) != 1)
            return FALSE;  /* Timeout or error */
#endif  /* CONFIG_IPC */

    }

    /* Process the event (and possibly others that occurred in the meantime) */
    do
    {

#ifdef CONFIG_IPC
	if (DDE_GetRemoteMessage())
        {
	    while(DDE_GetRemoteMessage()) ;
	    return TRUE;
	}
#endif  /* CONFIG_IPC */

        XNextEvent( display, &event );
        EVENT_ProcessEvent( &event );
    }
    while (XPending( display ));
    return TRUE;
}


/***********************************************************************
 *           MSG_PeekMessage
 */
static BOOL MSG_PeekMessage( LPMSG msg, HWND hwnd, WORD first, WORD last,
                             WORD flags, BOOL peek )
{
    int pos, mask;
    MESSAGEQUEUE *msgQueue;
    LONG nextExp;  /* Next timer expiration time */

#ifdef CONFIG_IPC
    DDE_TestDDE(hwnd);	/* do we have dde handling in the window ?*/
    DDE_GetRemoteMessage();
#endif  /* CONFIG_IPC */
    
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
        msgQueue = (MESSAGEQUEUE *)GlobalLock( GetTaskQueue(0) );
        if (!msgQueue) return FALSE;

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

        Yield();

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
BOOL MSG_InternalGetMessage( SEGPTR msg, HWND hwnd, HWND hwndOwner, short code,
			     WORD flags, BOOL sendIdle ) 
{
    for (;;)
    {
	if (sendIdle)
	{
	    if (!MSG_PeekMessage( (MSG *)PTR_SEG_TO_LIN(msg),
                                  0, 0, 0, flags, TRUE ))
	    {
		  /* No message present -> send ENTERIDLE and wait */
		SendMessage( hwndOwner, WM_ENTERIDLE, code, (LPARAM)hwnd );
		MSG_PeekMessage( (MSG *)PTR_SEG_TO_LIN(msg),
                                 0, 0, 0, flags, FALSE );
	    }
	}
	else  /* Always wait for a message */
	    MSG_PeekMessage( (MSG *)PTR_SEG_TO_LIN(msg),
                             0, 0, 0, flags, FALSE );

	if (!CallMsgFilter( msg, code ))
            return (((MSG *)PTR_SEG_TO_LIN(msg))->message != WM_QUIT);

	  /* Message filtered -> remove it from the queue */
	  /* if it's still there. */
	if (!(flags & PM_REMOVE))
	    MSG_PeekMessage( (MSG *)PTR_SEG_TO_LIN(msg),
                             0, 0, 0, PM_REMOVE, TRUE );
    }
}


/***********************************************************************
 *           PeekMessage   (USER.109)
 */
BOOL PeekMessage( LPMSG msg, HWND hwnd, WORD first, WORD last, WORD flags )
{
    return MSG_PeekMessage( msg, hwnd, first, last, flags, TRUE );
}


/***********************************************************************
 *           GetMessage   (USER.108)
 */
BOOL GetMessage( SEGPTR msg, HWND hwnd, UINT first, UINT last ) 
{
    MSG_PeekMessage( (MSG *)PTR_SEG_TO_LIN(msg),
                     hwnd, first, last, PM_REMOVE, FALSE );
    HOOK_CallHooks( WH_GETMESSAGE, HC_ACTION, 0, (LPARAM)msg );
    return (((MSG *)PTR_SEG_TO_LIN(msg))->message != WM_QUIT);
}



/***********************************************************************
 *           PostMessage   (USER.110)
 */
BOOL PostMessage( HWND hwnd, WORD message, WORD wParam, LONG lParam )
{
    MSG 	msg;
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
    
    if (hwnd == HWND_BROADCAST) {
      dprintf_msg(stddeb,"PostMessage // HWND_BROADCAST !\n");
      hwnd = GetTopWindow(GetDesktopWindow());
      while (hwnd) {
	if (!(wndPtr = WIN_FindWndPtr(hwnd))) break;
	if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION) {
	  dprintf_msg(stddeb,"BROADCAST Message to hWnd="NPFMT" m=%04X w=%04X l=%08lX !\n",
		      hwnd, message, wParam, lParam);
	  PostMessage(hwnd, message, wParam, lParam);
	}
	hwnd = wndPtr->hwndNext;
      }
      dprintf_msg(stddeb,"PostMessage // End of HWND_BROADCAST !\n");
      return TRUE;
    }

    wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr || !wndPtr->hmemTaskQ) return FALSE;

    return MSG_AddMsg( wndPtr->hmemTaskQ, &msg, 0 );
}

/***********************************************************************
 *           PostAppMessage   (USER.116)
 */
BOOL PostAppMessage( HTASK hTask, WORD message, WORD wParam, LONG lParam )
{
    MSG 	msg;

    if (GetTaskQueue(hTask) == 0) return FALSE;
    msg.hwnd    = 0;
    msg.message = message;
    msg.wParam  = wParam;
    msg.lParam  = lParam;
    msg.time    = GetTickCount();
    msg.pt.x    = 0;
    msg.pt.y    = 0;

    return MSG_AddMsg( GetTaskQueue(hTask), &msg, 0 );
}


/***********************************************************************
 *           SendMessage   (USER.111)
 */
LRESULT SendMessage( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    WND * wndPtr;
    LONG ret;
    struct
    {
	LPARAM lParam;
	WPARAM wParam;
	UINT wMsg;
	HWND hWnd;
    } msgstruct = { lParam, wParam, msg, hwnd };

#ifdef CONFIG_IPC
    MSG DDE_msg = { hwnd, msg, wParam, lParam };
    if (DDE_SendMessage(&DDE_msg)) return TRUE;
#endif  /* CONFIG_IPC */

    if (hwnd == HWND_BROADCAST)
    {
        dprintf_msg(stddeb,"SendMessage // HWND_BROADCAST !\n");
        hwnd = GetTopWindow(GetDesktopWindow());
        while (hwnd)
	{
            if (!(wndPtr = WIN_FindWndPtr(hwnd))) break;
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
            {
                dprintf_msg(stddeb,"BROADCAST Message to hWnd="NPFMT" m=%04X w=%04lX l=%08lX !\n",
                            hwnd, msg, (DWORD)wParam, lParam);
                 ret |= SendMessage( hwnd, msg, wParam, lParam );
	    }
            hwnd = wndPtr->hwndNext;
        }
        dprintf_msg(stddeb,"SendMessage // End of HWND_BROADCAST !\n");
        return TRUE;
    }

    EnterSpyMessage(SPY_SENDMESSAGE, hwnd, msg, wParam, lParam);

    HOOK_CallHooks( WH_CALLWNDPROC, HC_ACTION, 1, (LPARAM)MAKE_SEGPTR(&msgstruct) );
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) 
    {
        ExitSpyMessage(SPY_RESULT_INVALIDHWND,hwnd,msg,0);
        return 0;
    }
    ret = CallWindowProc( wndPtr->lpfnWndProc, msgstruct.hWnd, msgstruct.wMsg,
                          msgstruct.wParam, msgstruct.lParam );
    ExitSpyMessage(SPY_RESULT_OK,hwnd,msg,ret);
    return ret;
}


/***********************************************************************
 *           WaitMessage    (USER.112)
 */
void WaitMessage( void )
{
    MSG msg;
    MESSAGEQUEUE *queue;
    LONG nextExp = -1;  /* Next timer expiration time */

#ifdef CONFIG_IPC
    DDE_GetRemoteMessage();
#endif  /* CONFIG_IPC */
    
    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetTaskQueue(0) ))) return;
    if ((queue->wPostQMsg) || 
        (queue->status & (QS_SENDMESSAGE | QS_PAINT)) ||
        (queue->msgCount) || (sysMsgQueue->msgCount) )
        return;
    if ((queue->status & QS_TIMER) && 
        TIMER_CheckTimer( &nextExp, &msg, 0, FALSE))
        return;
    /* FIXME: (dde) must check DDE & X-events simultaneously */
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
    
    EnterSpyMessage( SPY_DISPATCHMESSAGE, msg->hwnd, msg->message,
                     msg->wParam, msg->lParam );

      /* Process timer messages */
    if ((msg->message == WM_TIMER) || (msg->message == WM_SYSTIMER))
    {
	if (msg->lParam)
        {
#ifndef WINELIB32
            HINSTANCE ds = msg->hwnd ? WIN_GetWindowInstance( msg->hwnd )
                                     : (HINSTANCE)CURRENT_DS;
#endif
/*            HOOK_CallHooks( WH_CALLWNDPROC, HC_ACTION, 0, FIXME ); */
	    return CallWndProc( (WNDPROC)msg->lParam, ds, msg->hwnd,
                                msg->message, msg->wParam, GetTickCount() );
        }
    }

    if (!msg->hwnd) return 0;
    if (!(wndPtr = WIN_FindWndPtr( msg->hwnd ))) return 0;
    if (!wndPtr->lpfnWndProc) return 0;
    painting = (msg->message == WM_PAINT);
    if (painting) wndPtr->flags |= WIN_NEEDS_BEGINPAINT;
/*    HOOK_CallHooks( WH_CALLWNDPROC, HC_ACTION, 0, FIXME ); */
    retval = CallWindowProc( wndPtr->lpfnWndProc, msg->hwnd, msg->message,
			     msg->wParam, msg->lParam );
    if (painting && IsWindow(msg->hwnd) &&
        (wndPtr->flags & WIN_NEEDS_BEGINPAINT))
    {
	fprintf(stderr, "BeginPaint not called on WM_PAINT for hwnd "NPFMT"!\n", 
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
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetTaskQueue(0) ))) return 0;
    return queue->GetMessagePosVal;
}


/***********************************************************************
 *           GetMessageTime   (USER.120)
 */
LONG GetMessageTime(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetTaskQueue(0) ))) return 0;
    return queue->GetMessageTimeVal;
}


/***********************************************************************
 *           GetMessageExtraInfo   (USER.288)
 */
LONG GetMessageExtraInfo(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetTaskQueue(0) ))) return 0;
    return queue->GetMessageExtraInfoVal;
}


/***********************************************************************
 *           RegisterWindowMessage   (USER.118)
 */
WORD RegisterWindowMessage( SEGPTR str )
{
    dprintf_msg(stddeb, "RegisterWindowMessage: '"SPFMT"'\n", str );
    return GlobalAddAtom( str );
}


/***********************************************************************
 *           GetTickCount    (USER.13) (KERNEL32.299)
 */
DWORD GetTickCount(void)
{
    struct timeval t;
    gettimeofday( &t, NULL );
    return (t.tv_sec * 1000) + (t.tv_usec / 1000);
}

/***********************************************************************
 *           GetCurrentTime  (effectively identical to GetTickCount)
 */
DWORD GetCurrentTime(void)
{
  return GetTickCount();
}

/***********************************************************************
 *			InSendMessage	(USER.192
 *
 * According to the book, this should return true iff the current message
 * was send from another application. In that case, the application should
 * invoke ReplyMessage before calling message relevant API.
 * Currently, Wine will always return FALSE, as there is no other app.
 */
BOOL InSendMessage()
{
	return FALSE;
}
