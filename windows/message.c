/*
 * Message queues related functions
 *
 * Copyright 1993 Alexandre Julliard
 */

/*
 * This code assumes that there is only one Windows task (hence
 * one message queue).
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>

#include "message.h"
#include "win.h"
#include "wineopts.h"
#include "sysmetrics.h"

#define MAX_QUEUE_SIZE   120  /* Max. size of a message queue */

extern BOOL TIMER_CheckTimer( DWORD *next );      /* timer.c */
extern void EVENT_ProcessEvent( XEvent *event );  /* event.c */
  
extern Display * display;

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
  
    SpyMessage(msg->hwnd, msg->message, msg->wParam, msg->lParam);
    
    if (!msgQueue) return FALSE;
    pos = msgQueue->nextFreeMessage;

      /* Check if queue is full */
    if ((pos == msgQueue->nextMessage) && (msgQueue->msgCount > 0))
	return FALSE;

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
    QMSG * qmsg;
    
    if (!msgQueue) return;
    qmsg = &msgQueue->messages[pos];

    if (pos >= msgQueue->nextMessage)
    {
	int count = pos - msgQueue->nextMessage;
	if (count) memmove( &msgQueue->messages[msgQueue->nextMessage+1],
			    &msgQueue->messages[msgQueue->nextMessage],
			    count * sizeof(QMSG) );
	msgQueue->nextMessage++;
	if (msgQueue->nextMessage >= msgQueue->queueSize)
	    msgQueue->nextMessage = 0;
    }
    else
    {
	int count = msgQueue->nextFreeMessage - pos;
	if (count) memmove( &msgQueue->messages[pos],
			    &msgQueue->messages[pos+1], count * sizeof(QMSG) );
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
 * Actions performed:
 * - Translate button down messages in double-clicks.
 * - Send the WM_NCHITTEST message to find where the cursor is.
 * - Translate the message into a non-client message, or translate
 *   the coordinates to client coordinates.
 */
static void MSG_TranslateMouseMsg( MSG *msg )
{
    static DWORD lastClickTime = 0;
    static WORD  lastClickMsg = 0;
    static POINT lastClickPos = { 0, 0 };

    LONG hittest_result = SendMessage( msg->hwnd, WM_NCHITTEST, 0,
				       MAKELONG( msg->pt.x, msg->pt.y ) );

    if ((msg->message == WM_LBUTTONDOWN) ||
        (msg->message == WM_RBUTTONDOWN) ||
        (msg->message == WM_MBUTTONDOWN))
    {
	BOOL dbl_click = FALSE;

	  /* Check for double-click */

	if ((msg->message == lastClickMsg) &&
	    (msg->time - lastClickTime < doubleClickSpeed) &&
	    (abs(msg->pt.x - lastClickPos.x) < SYSMETRICS_CXDOUBLECLK/2) &&
	    (abs(msg->pt.y - lastClickPos.y) < SYSMETRICS_CYDOUBLECLK/2))
	    dbl_click = TRUE;

	if (dbl_click && (hittest_result == HTCLIENT))
	{
	    /* Check whether window wants the double click message. */
	    WND * wndPtr = WIN_FindWndPtr( msg->hwnd );
	    if (!wndPtr || !(wndPtr->flags & WIN_DOUBLE_CLICKS))
		dbl_click = FALSE;
	}

	if (dbl_click) switch(msg->message)
	{
	    case WM_LBUTTONDOWN: msg->message = WM_LBUTTONDBLCLK; break;
	    case WM_RBUTTONDOWN: msg->message = WM_RBUTTONDBLCLK; break;
	    case WM_MBUTTONDOWN: msg->message = WM_MBUTTONDBLCLK; break;
	}

	lastClickTime = msg->time;
	lastClickMsg  = msg->message;
	lastClickPos  = msg->pt;
    }

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
 * Note: the position is in screen coordinates.
 */
void hardware_event( HWND hwnd, WORD message, WORD wParam, LONG lParam,
		     WORD xPos, WORD yPos, DWORD time, DWORD extraInfo )
{
    MSG msg;

    msg.hwnd    = hwnd;
    msg.message = message;
    msg.wParam  = wParam;
    msg.lParam  = lParam;
    msg.time    = time;
    msg.pt.x    = xPos;
    msg.pt.y    = yPos;    
    if (!MSG_AddMsg( sysMsgQueue, &msg, extraInfo ))
	printf( "hardware_event: Queue is full\n" );
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
 *           MSG_PeekMessage
 */
static BOOL MSG_PeekMessage( MESSAGEQUEUE * msgQueue, LPMSG msg, HWND hwnd,
			     WORD first, WORD last, WORD flags, BOOL peek )
{
    int pos, mask;
    DWORD nextExp;  /* Next timer expiration time */
    XEvent event;

    if (first || last)
    {
	mask = QS_POSTMESSAGE;  /* Always selectioned */
	if ((first <= WM_KEYLAST) && (last >= WM_KEYFIRST)) mask |= QS_KEY;
	if ((first <= WM_MOUSELAST) && (last >= WM_MOUSEFIRST)) mask |= QS_MOUSE;
	if ((first <= WM_TIMER) && (last >= WM_TIMER)) mask |= WM_TIMER;
	if ((first <= WM_SYSTIMER) && (last >= WM_SYSTIMER)) mask |= WM_TIMER;
	if ((first <= WM_PAINT) && (last >= WM_PAINT)) mask |= WM_PAINT;
    }
    else mask = QS_MOUSE | QS_KEY | QS_POSTMESSAGE | QS_TIMER | QS_PAINT;

    while (XPending( display ))
    {
	XNextEvent( display, &event );
	EVENT_ProcessEvent( &event );
    }    

    while(1)
    {    
	  /* First handle a WM_QUIT message */
	if (msgQueue->wPostQMsg)
	{
	    msg->hwnd    = hwnd;
	    msg->message = WM_QUIT;
	    msg->wParam  = msgQueue->wExitCode;
	    msg->lParam  = 0;
	    break;
	}

	  /* Then handle a message put by SendMessage() */
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
	pos = MSG_FindMsg( sysMsgQueue, hwnd, first, last );
	if (pos != -1)
	{
	    QMSG *qmsg = &sysMsgQueue->messages[pos];
	    *msg = qmsg->msg;
	    msgQueue->GetMessageTimeVal      = msg->time;
	    msgQueue->GetMessagePosVal       = *(DWORD *)&msg->pt;
	    msgQueue->GetMessageExtraInfoVal = qmsg->extraInfo;

	    if (flags & PM_REMOVE) MSG_RemoveMsg( sysMsgQueue, pos );
	    if ((msg->message >= WM_MOUSEFIRST) &&
		(msg->message <= WM_MOUSELAST)) MSG_TranslateMouseMsg( msg );
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
	    if (TIMER_CheckTimer( &nextExp ))
		continue;  /* Restart the whole search */

	  /* Wait until something happens */
	if (peek) return FALSE;
	if (!XPending( display ) && (nextExp != -1))
	{
	    fd_set read_set;
	    struct timeval timeout;
	    int fd = ConnectionNumber(display);
	    FD_ZERO( &read_set );
	    FD_SET( fd, &read_set );
	    timeout.tv_sec = nextExp / 1000;
	    timeout.tv_usec = (nextExp % 1000) * 1000;
	    if (select( fd+1, &read_set, NULL, NULL, &timeout ) != 1)
		continue;  /* On timeout or error, restart from the start */
	}
	XNextEvent( display, &event );
	EVENT_ProcessEvent( &event );
    }

      /* We got a message */
    if (peek) return TRUE;
    else return (msg->message != WM_QUIT);
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
    return MSG_PeekMessage( appMsgQueue, msg, hwnd, first, last, PM_REMOVE, FALSE );
}


/***********************************************************************
 *           PostMessage   (USER.110)
 */
BOOL PostMessage( HWND hwnd, WORD message, WORD wParam, LONG lParam )
{
    MSG msg;
    
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

    SpyMessage(hwnd, msg, wParam, lParam);
    
    wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;
    return CallWindowProc( wndPtr->lpfnWndProc, hwnd, msg, wParam, lParam );
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
#ifdef DEBUG_MSG
	printf( "Translating key message\n" );
#endif
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
    
#ifdef DEBUG_MSG
    printf( "Dispatch message hwnd=%08x msg=0x%x w=%d l=%d time=%u pt=%d,%d\n",
	    msg->hwnd, msg->message, msg->wParam, msg->lParam, 
	    msg->time, msg->pt.x, msg->pt.y );
#endif

      /* Process timer messages */
    if ((msg->message == WM_TIMER) || (msg->message == WM_SYSTIMER))
    {
	if (msg->lParam)
	    return CallWindowProc( (FARPROC)msg->lParam, msg->hwnd,
				   msg->message, msg->wParam, GetTickCount() );
    }

    if (!msg->hwnd) return 0;
    if (!(wndPtr = WIN_FindWndPtr( msg->hwnd ))) return 0;
    if (!wndPtr->lpfnWndProc) return 0;
    painting = (msg->message == WM_PAINT);
    if (painting) wndPtr->flags |= WIN_NEEDS_BEGINPAINT;
    retval = CallWindowProc( wndPtr->lpfnWndProc, msg->hwnd, msg->message,
			     msg->wParam, msg->lParam );
    if (painting && (wndPtr->flags & WIN_NEEDS_BEGINPAINT))
    {
#ifdef DEBUG_WIN
	printf( "BeginPaint not called on WM_PAINT for hwnd %d!\n", msg->hwnd);
#endif
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
#ifdef DEBUG_MSG
    printf( "RegisterWindowMessage: '%s'\n", str );
#endif
    return GlobalAddAtom( str );
}

