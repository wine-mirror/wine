/*
 * Message queues related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <stdlib.h>
#include "module.h"
#include "queue.h"
#include "win.h"
#include "stddebug.h"
#include "debug.h"

#define MAX_QUEUE_SIZE   120  /* Max. size of a message queue */

static HQUEUE hFirstQueue = 0;
static HQUEUE hmemSysMsgQueue = 0;
static MESSAGEQUEUE *sysMsgQueue = NULL;


/***********************************************************************
 *	     QUEUE_DumpQueue
 */
void QUEUE_DumpQueue( HQUEUE hQueue )
{
    MESSAGEQUEUE *pq; 

    if (!(pq = (MESSAGEQUEUE*) GlobalLock( hQueue )) ||
        GlobalSize(hQueue) < sizeof(MESSAGEQUEUE) + pq->queueSize*sizeof(QMSG))
    {
        fprintf( stderr, "%04x is not a queue handle\n", hQueue );
        return;
    }

    fprintf( stderr,
             "next: %12.4x  Intertask SendMessage:\n"
             "hTask: %11.4x  ----------------------\n"
             "msgSize: %9.4x  hWnd: %10.4x\n"
             "msgCount: %8.4x  msg: %11.4x\n"
             "msgNext: %9.4x  wParam: %8.4x\n"
             "msgFree: %9.4x  lParam: %8.8x\n"
             "qSize: %11.4x  lRet: %10.8x\n"
             "wWinVer: %9.4x  ISMH: %10.4x\n"
             "paints: %10.4x  hSendTask: %5.4x\n"
             "timers: %10.4x  hPrevSend: %5.4x\n"
             "wakeBits: %8.4x\n"
             "wakeMask: %8.4x\n"
             "hCurHook: %8.4x\n",
             pq->next, pq->hTask, pq->msgSize, pq->hWnd, 
             pq->msgCount, pq->msg, pq->nextMessage, pq->wParam,
             pq->nextFreeMessage, (unsigned)pq->lParam, pq->queueSize,
             (unsigned)pq->SendMessageReturn, pq->wWinVersion, pq->InSendMessageHandle,
             pq->wPaintCount, pq->hSendingTask, pq->wTimerCount,
             pq->hPrevSendingTask, pq->status, pq->wakeMask, pq->hCurHook);
}


/***********************************************************************
 *	     QUEUE_WalkQueues
 */
void QUEUE_WalkQueues(void)
{
    HQUEUE hQueue = hFirstQueue;

    fprintf( stderr, "Queue Size Msgs Task\n" );
    while (hQueue)
    {
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock( hQueue );
        if (!queue)
        {
            fprintf( stderr, "*** Bad queue handle %04x\n", hQueue );
            return;
        }
        fprintf( stderr, "%04x %5d %4d %04x %s\n",
                 hQueue, queue->msgSize, queue->msgCount, queue->hTask,
                 MODULE_GetModuleName( GetExePtr(queue->hTask) ) );
        hQueue = queue->next;
    }
    fprintf( stderr, "\n" );
}


/***********************************************************************
 *           QUEUE_CreateMsgQueue
 *
 * Creates a message queue. Doesn't link it into queue list!
 */
static HQUEUE QUEUE_CreateMsgQueue( int size )
{
    HQUEUE hQueue;
    MESSAGEQUEUE * msgQueue;
    int queueSize;

    dprintf_msg(stddeb,"Creating message queue...\n");

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
 *	     QUEUE_DeleteMsgQueue
 *
 * Unlinks and deletes a message queue.
 */
BOOL QUEUE_DeleteMsgQueue( HQUEUE hQueue )
{
    MESSAGEQUEUE * msgQueue = (MESSAGEQUEUE*)GlobalLock(hQueue);
    HQUEUE *pPrev;

    dprintf_msg(stddeb,"Deleting message queue %04x\n", hQueue);

    if (!hQueue || !msgQueue)
    {
	dprintf_msg(stddeb,"DeleteMsgQueue: invalid argument.\n");
	return 0;
    }

    pPrev = &hFirstQueue;
    while (*pPrev && (*pPrev != hQueue))
    {
        MESSAGEQUEUE *msgQ = (MESSAGEQUEUE*)GlobalLock(*pPrev);
        pPrev = &msgQ->next;
    }
    if (*pPrev) *pPrev = msgQueue->next;
    GlobalFree( hQueue );
    return 1;
}


/***********************************************************************
 *           QUEUE_CreateSysMsgQueue
 *
 * Create the system message queue, and set the double-click speed.
 * Must be called only once.
 */
BOOL QUEUE_CreateSysMsgQueue( int size )
{
    if (size > MAX_QUEUE_SIZE) size = MAX_QUEUE_SIZE;
    else if (size <= 0) size = 1;
    if (!(hmemSysMsgQueue = QUEUE_CreateMsgQueue( size ))) return FALSE;
    sysMsgQueue = (MESSAGEQUEUE *) GlobalLock( hmemSysMsgQueue );
    return TRUE;
}


/***********************************************************************
 *           QUEUE_GetSysQueue
 */
MESSAGEQUEUE *QUEUE_GetSysQueue(void)
{
    return sysMsgQueue;
}


/***********************************************************************
 *           QUEUE_AddMsg
 *
 * Add a message to the queue. Return FALSE if queue is full.
 */
BOOL QUEUE_AddMsg( HQUEUE hQueue, MSG * msg, DWORD extraInfo )
{
    int pos;
    MESSAGEQUEUE *msgQueue;

    if (!(msgQueue = (MESSAGEQUEUE *)GlobalLock( hQueue ))) return FALSE;
    pos = msgQueue->nextFreeMessage;

      /* Check if queue is full */
    if ((pos == msgQueue->nextMessage) && (msgQueue->msgCount > 0))
    {
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
 *           QUEUE_FindMsg
 *
 * Find a message matching the given parameters. Return -1 if none available.
 */
int QUEUE_FindMsg( MESSAGEQUEUE * msgQueue, HWND hwnd, int first, int last )
{
    int i, pos = msgQueue->nextMessage;

    dprintf_msg(stddeb,"MSG_FindMsg: hwnd=%04x\n\n", hwnd );

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
 *           QUEUE_RemoveMsg
 *
 * Remove a message from the queue (pos must be a valid position).
 */
void QUEUE_RemoveMsg( MESSAGEQUEUE * msgQueue, int pos )
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
 *	     QUEUE_GetQueueTask
 */
HTASK QUEUE_GetQueueTask( HQUEUE hQueue )
{
    MESSAGEQUEUE *queue = GlobalLock( hQueue );
    return (queue) ? queue->hTask : 0 ;
}


/***********************************************************************
 *           QUEUE_IncPaintCount
 */
void QUEUE_IncPaintCount( HQUEUE hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( hQueue ))) return;
    queue->wPaintCount++;
    queue->status |= QS_PAINT;
    queue->tempStatus |= QS_PAINT;    
}


/***********************************************************************
 *           QUEUE_DecPaintCount
 */
void QUEUE_DecPaintCount( HQUEUE hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( hQueue ))) return;
    queue->wPaintCount--;
    if (!queue->wPaintCount) queue->status &= ~QS_PAINT;
}


/***********************************************************************
 *           QUEUE_IncTimerCount
 */
void QUEUE_IncTimerCount( HQUEUE hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( hQueue ))) return;
    queue->wTimerCount++;
    queue->status |= QS_TIMER;
    queue->tempStatus |= QS_TIMER;
}


/***********************************************************************
 *           QUEUE_DecTimerCount
 */
void QUEUE_DecTimerCount( HQUEUE hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( hQueue ))) return;
    queue->wTimerCount--;
    if (!queue->wTimerCount) queue->status &= ~QS_TIMER;
}


/***********************************************************************
 *           PostQuitMessage   (USER.6)
 */
void PostQuitMessage( INT exitCode )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetTaskQueue(0) ))) return;
    queue->wPostQMsg = TRUE;
    queue->wExitCode = (WORD)exitCode;
}


/***********************************************************************
 *           GetWindowTask   (USER.224)
 */
HTASK GetWindowTask( HWND hwnd )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr) return 0;
    return QUEUE_GetQueueTask( wndPtr->hmemTaskQ );
}


/***********************************************************************
 *           SetMessageQueue   (USER.266)
 */
BOOL SetMessageQueue( int size )
{
    HQUEUE hQueue, hNewQueue;
    MESSAGEQUEUE *queuePtr;

    dprintf_msg(stddeb,"SetMessageQueue: task %04x size %i\n", GetCurrentTask(), size); 

    if ((size > MAX_QUEUE_SIZE) || (size <= 0)) return TRUE;

    if( !(hNewQueue = QUEUE_CreateMsgQueue( size ))) 
    {
	dprintf_msg(stddeb,"SetMessageQueue: failed!\n");
	return FALSE;
    }

    /* Free the old message queue */
    if ((hQueue = GetTaskQueue(0)) != 0) QUEUE_DeleteMsgQueue( hQueue );

    /* Link new queue into list */
    queuePtr = (MESSAGEQUEUE *)GlobalLock( hNewQueue );
    queuePtr->hTask = GetCurrentTask();
    queuePtr->next  = hFirstQueue;
    hFirstQueue = hNewQueue;

    SetTaskQueue( 0, hNewQueue );
    return TRUE;
}


/***********************************************************************
 *           GetQueueStatus   (USER.334)
 */
DWORD GetQueueStatus( UINT flags )
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
