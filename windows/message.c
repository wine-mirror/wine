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

#ifdef __NetBSD__
#define HZ 100
#endif

#include "message.h"
#include "win.h"


#define MAX_QUEUE_SIZE   120  /* Max. size of a message queue */

extern HWND WIN_FindWinToRepaint( HWND hwnd );

extern Display * XT_display;
extern Screen * XT_screen;
extern XtAppContext XT_app_context;

static MESSAGEQUEUE * msgQueue = NULL;


/***********************************************************************
 *           MSG_GetMessageType
 *
 */
int MSG_GetMessageType( int msg )
{
    if ((msg >= WM_KEYFIRST) && (msg <= WM_KEYLAST)) return QS_KEY;
    else if ((msg >= WM_MOUSEFIRST) && (msg <= WM_MOUSELAST))
    {
	if (msg == WM_MOUSEMOVE) return QS_MOUSEMOVE;
	else return QS_MOUSEBUTTON;
    }
    else if (msg == WM_PAINT) return QS_PAINT;
    else if (msg == WM_TIMER) return QS_TIMER;
    return QS_POSTMESSAGE;
}


/***********************************************************************
 *           MSG_AddMsg
 *
 * Add a message to the queue. Return FALSE if queue is full.
 */
int MSG_AddMsg( MSG * msg, DWORD extraInfo )
{
    int pos, type;
  
    if (!msgQueue) return FALSE;
    pos = msgQueue->nextFreeMessage;

      /* No need to store WM_PAINT messages */
    if (msg->message == WM_PAINT) return TRUE;
        
      /* Check if queue is full */
    if ((pos == msgQueue->nextMessage) && (msgQueue->msgCount > 0))
	return FALSE;

      /* Store message */
    msgQueue->messages[pos].msg = *msg;
    msgQueue->messages[pos].extraInfo = extraInfo;

      /* Store message type */
    type = MSG_GetMessageType( msg->message );   
    msgQueue->status |= type;
    msgQueue->tempStatus |= type;
    
    if (pos < msgQueue->queueSize-1) pos++;
    else pos = 0;
    msgQueue->nextFreeMessage = pos;
    msgQueue->msgCount++;
    
    return TRUE;
}


/***********************************************************************
 *           MSG_FindMsg
 *
 * Find a message matching the given parameters. Return -1 if none available.
 */
int MSG_FindMsg( HWND hwnd, int first, int last )
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
void MSG_RemoveMsg( int pos )
{
    int i, type;
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

      /* Recalc status */
    type = 0;
    pos = msgQueue->nextMessage;
    for (i = 0; i < msgQueue->msgCount; i++)
    {
	type |= MSG_GetMessageType( msgQueue->messages[pos].msg.message );
	if (++pos >= msgQueue->queueSize-1) pos = 0;
    }
    msgQueue->status = (msgQueue->status & QS_SENDMESSAGE) | type;
    msgQueue->tempStatus = 0;
}


/***********************************************************************
 *           SetMessageQueue  (USER.266)
 */
BOOL SetMessageQueue( int size )
{
    int queueSize;
  
      /* Free the old message queue */
    if (msgQueue) free(msgQueue);
  
    if ((size > MAX_QUEUE_SIZE) || (size <= 0)) return FALSE;
  
    queueSize = sizeof(MESSAGEQUEUE) + size * sizeof(QMSG);
    msgQueue = (MESSAGEQUEUE *) malloc(queueSize);
    if (!msgQueue) return FALSE;

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
    msgQueue->tempStatus = 0;
    msgQueue->status = 0;

    return TRUE;
}


/***********************************************************************
 *           PostQuitMessage   (USER.6)
 */
void PostQuitMessage( int exitCode )
{
    if (!msgQueue) return;
    msgQueue->wPostQMsg = TRUE;
    msgQueue->wExitCode = exitCode;
}


/***********************************************************************
 *           GetQueueStatus   (USER.334)
 */
DWORD GetQueueStatus( int flags )
{
    unsigned long ret = (msgQueue->status << 16) | msgQueue->tempStatus;
    if (flags & QS_PAINT)
    {
	if (WIN_FindWinToRepaint(0)) ret |= QS_PAINT | (QS_PAINT << 16);
    }
    msgQueue->tempStatus = 0;
    return ret & ((flags << 16) | flags);
}


/***********************************************************************
 *           GetInputState   (USER.335)
 */
BOOL GetInputState()
{
    return msgQueue->status & (QS_KEY | QS_MOUSEBUTTON);
}


/***********************************************************************
 *           MSG_PeekMessage
 */
BOOL MSG_PeekMessage( LPMSG msg, HWND hwnd, WORD first, WORD last, WORD flags )
{
    int pos;

      /* First handle a WM_QUIT message */
    if (msgQueue->wPostQMsg)
    {
	msg->hwnd    = hwnd;
	msg->message = WM_QUIT;
	msg->wParam  = msgQueue->wExitCode;
	msg->lParam  = 0;
	return TRUE;
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
		return TRUE;
	    }
	}
	
    }
    
      /* Now find a normal message */
    pos = MSG_FindMsg( hwnd, first, last );
    if (pos != -1)
    {
	QMSG *qmsg = &msgQueue->messages[pos];
	*msg = qmsg->msg;
	msgQueue->GetMessageTimeVal      = msg->time;
	msgQueue->GetMessagePosVal       = *(DWORD *)&msg->pt;
	msgQueue->GetMessageExtraInfoVal = qmsg->extraInfo;

	if (flags & PM_REMOVE) MSG_RemoveMsg(pos);
    	return TRUE;
    }

      /* If nothing else, return a WM_PAINT message */
    if ((!first && !last) || ((first <= WM_PAINT) && (last >= WM_PAINT)))
    {
	msg->hwnd = WIN_FindWinToRepaint( hwnd );
	msg->message = WM_PAINT;
	msg->wParam = 0;
	msg->lParam = 0;
	return (msg->hwnd != 0);
    }	
    return FALSE;
}


/***********************************************************************
 *           PeekMessage   (USER.109)
 */
BOOL PeekMessage( LPMSG msg, HWND hwnd, WORD first, WORD last, WORD flags )
{
    while (XtAppPending( XT_app_context ))
	XtAppProcessEvent( XT_app_context, XtIMAll );

    return MSG_PeekMessage( msg, hwnd, first, last, flags );
}


/***********************************************************************
 *           GetMessage   (USER.108)
 */
BOOL GetMessage( LPMSG msg, HWND hwnd, WORD first, WORD last ) 
{
    while(1)
    {
	if (MSG_PeekMessage( msg, hwnd, first, last, PM_REMOVE )) break;
	XtAppProcessEvent( XT_app_context, XtIMAll );
    }

    return (msg->message != WM_QUIT);
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
    
    return MSG_AddMsg( &msg, 0 );
}


/***********************************************************************
 *           SendMessage   (USER.111)
 */
LONG SendMessage( HWND hwnd, WORD msg, WORD wParam, LONG lParam )
{
    LONG retval = 0;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (wndPtr)
    {
	retval = CallWindowProc( wndPtr->lpfnWndProc, hwnd, msg, 
				 wParam, lParam );
	GlobalUnlock( hwnd );
    }
    return retval;
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
    LONG retval = 0;
    WND * wndPtr = WIN_FindWndPtr( msg->hwnd );

#ifdef DEBUG_MSG
    printf( "Dispatch message hwnd=%08x msg=%d w=%d l=%d time=%u pt=%d,%d\n",
	    msg->hwnd, msg->message, msg->wParam, msg->lParam, 
	    msg->time, msg->pt.x, msg->pt.y );
#endif
    if (wndPtr) 
    {
	retval = CallWindowProc(wndPtr->lpfnWndProc, msg->hwnd, msg->message,
				msg->wParam, msg->lParam );
	GlobalUnlock( msg->hwnd );
    }
    return retval;
}


/***********************************************************************
 *           GetMessagePos   (USER.119)
 */
DWORD GetMessagePos(void)
{
    return msgQueue->GetMessagePosVal;
}


/***********************************************************************
 *           GetMessageTime   (USER.120)
 */
LONG GetMessageTime(void)
{
    return msgQueue->GetMessageTimeVal;
}

/***********************************************************************
 *           GetMessageExtraInfo   (USER.288)
 */
LONG GetMessageExtraInfo(void)
{
    return msgQueue->GetMessageExtraInfoVal;
}

