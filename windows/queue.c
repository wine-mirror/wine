/*
 * Message queues related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <stdlib.h>
#include "module.h"
#include "queue.h"
#include "task.h"
#include "win.h"
#include "stddebug.h"
#include "debug.h"

#define MAX_QUEUE_SIZE   120  /* Max. size of a message queue */

static HQUEUE16 hFirstQueue = 0;
static HQUEUE16 hmemSysMsgQueue = 0;
static MESSAGEQUEUE *pMouseQueue = NULL;  /* Queue for last mouse message */
static MESSAGEQUEUE *pKbdQueue = NULL;    /* Queue for last kbd message */
static HQUEUE16 hDoomedQueue = 0;
static MESSAGEQUEUE *sysMsgQueue = NULL;

/***********************************************************************
 *	     QUEUE_GetDoomedQueue/QUEUE_SetDoomedQueue
 */
HQUEUE QUEUE_GetDoomedQueue()
{
  return hDoomedQueue;
}
void QUEUE_SetDoomedQueue(HQUEUE hQueue)
{
  hDoomedQueue = hQueue;
}

/***********************************************************************
 *	     QUEUE_DumpQueue
 */
void QUEUE_DumpQueue( HQUEUE16 hQueue )
{
    MESSAGEQUEUE *pq; 

    if (!(pq = (MESSAGEQUEUE*) GlobalLock16( hQueue )) ||
        GlobalSize16(hQueue) < sizeof(MESSAGEQUEUE)+pq->queueSize*sizeof(QMSG))
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
             pq->hPrevSendingTask, pq->wakeBits, pq->wakeMask, pq->hCurHook);
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
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock16( hQueue );
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
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );

    dprintf_msg(stddeb,"Creating message queue...\n");

    queueSize = sizeof(MESSAGEQUEUE) + size * sizeof(QMSG);
    if (!(hQueue = GlobalAlloc16( GMEM_FIXED | GMEM_ZEROINIT, queueSize )))
        return 0;
    msgQueue = (MESSAGEQUEUE *) GlobalLock16( hQueue );
    msgQueue->self = hQueue;
    msgQueue->msgSize = sizeof(QMSG);
    msgQueue->queueSize = size;
    msgQueue->wWinVersion = pTask ? pTask->version : 0;
    GlobalUnlock16( hQueue );
    return hQueue;
}


/***********************************************************************
 *	     QUEUE_DeleteMsgQueue
 *
 * Unlinks and deletes a message queue.
 */
BOOL32 QUEUE_DeleteMsgQueue( HQUEUE16 hQueue )
{
    MESSAGEQUEUE * msgQueue = (MESSAGEQUEUE*)GlobalLock16(hQueue);
    HQUEUE16 *pPrev;

    dprintf_msg(stddeb,"Deleting message queue %04x\n", hQueue);

    if (!hQueue || !msgQueue)
    {
	dprintf_msg(stddeb,"DeleteMsgQueue: invalid argument.\n");
	return 0;
    }

    pPrev = &hFirstQueue;
    while (*pPrev && (*pPrev != hQueue))
    {
        MESSAGEQUEUE *msgQ = (MESSAGEQUEUE*)GlobalLock16(*pPrev);
        pPrev = &msgQ->next;
    }
    if (*pPrev) *pPrev = msgQueue->next;
    msgQueue->self = 0;
    GlobalFree16( hQueue );
    return 1;
}


/***********************************************************************
 *           QUEUE_CreateSysMsgQueue
 *
 * Create the system message queue, and set the double-click speed.
 * Must be called only once.
 */
BOOL32 QUEUE_CreateSysMsgQueue( int size )
{
    if (size > MAX_QUEUE_SIZE) size = MAX_QUEUE_SIZE;
    else if (size <= 0) size = 1;
    if (!(hmemSysMsgQueue = QUEUE_CreateMsgQueue( size ))) return FALSE;
    sysMsgQueue = (MESSAGEQUEUE *) GlobalLock16( hmemSysMsgQueue );
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
 *           QUEUE_SetWakeBit
 *
 * See "Windows Internals", p.449
 */
void QUEUE_SetWakeBit( MESSAGEQUEUE *queue, WORD bit )
{
    if (bit & QS_MOUSE) pMouseQueue = queue;
    if (bit & QS_KEY) pKbdQueue = queue;
    queue->changeBits |= bit;
    queue->wakeBits   |= bit;
    if (queue->wakeMask & bit)
    {
        queue->wakeMask = 0;
        PostEvent( queue->hTask );
    }
}


/***********************************************************************
 *           QUEUE_ClearWakeBit
 */
void QUEUE_ClearWakeBit( MESSAGEQUEUE *queue, WORD bit )
{
    queue->changeBits &= ~bit;
    queue->wakeBits   &= ~bit;
}


/***********************************************************************
 *           QUEUE_WaitBits
 *
 * See "Windows Internals", p.447
 */
void QUEUE_WaitBits( WORD bits )
{
    MESSAGEQUEUE *queue;

    for (;;)
    {
        if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return;
        if (queue->changeBits & bits)
        {
            /* One of the bits is set; we can return */
            queue->wakeMask = 0;
            return;
        }
        if (queue->wakeBits & QS_SENDMESSAGE)
        {
            /* Process the sent message immediately */
            QUEUE_ReceiveMessage( queue );
        }
        queue->wakeMask = bits | QS_SENDMESSAGE;
        WaitEvent( 0 );
    }
}


/***********************************************************************
 *           QUEUE_ReceiveMessage
 *
 * This routine is called when a sent message is waiting for the queue.
 */
void QUEUE_ReceiveMessage( MESSAGEQUEUE *queue )
{
    MESSAGEQUEUE *senderQ;
    HWND hwnd;
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
    LRESULT result = 0;
    HQUEUE oldSender;

    printf( "ReceiveMessage\n" );
    if (!(queue->wakeBits & QS_SENDMESSAGE)) return;
    if (!(senderQ = (MESSAGEQUEUE*)GlobalLock16( queue->hSendingTask))) return;

    /* Remove sending queue from the list */
    oldSender                  = queue->InSendMessageHandle;
    queue->InSendMessageHandle = queue->hSendingTask;
    queue->hSendingTask        = senderQ->hPrevSendingTask;
    senderQ->hPrevSendingTask  = 0;
    if (!queue->hSendingTask)
    {
        queue->wakeBits &= ~QS_SENDMESSAGE;
        queue->changeBits &= ~QS_SENDMESSAGE;
    }

    /* Get the parameters from the sending task */
    hwnd   = senderQ->hWnd;
    msg    = senderQ->msg;
    wParam = senderQ->wParam;
    lParam = senderQ->lParam;
    senderQ->hWnd = 0;
    QUEUE_SetWakeBit( senderQ, QS_SMPARAMSFREE );

    printf( "ReceiveMessage: calling wnd proc %04x %04x %04x %08x\n",
            hwnd, msg, wParam, lParam );

    /* Call the window procedure */
    /* FIXME: should we use CallWindowProc here? */
    if (IsWindow( hwnd ))
    {
        DWORD extraInfo = queue->GetMessageExtraInfoVal;
        queue->GetMessageExtraInfoVal = senderQ->GetMessageExtraInfoVal;
        result = SendMessage16( hwnd, msg, wParam, lParam );
        queue->GetMessageExtraInfoVal = extraInfo;  /* Restore extra info */
    }

    printf( "ReceiveMessage: wnd proc %04x %04x %04x %08x ret = %08x\n",
            hwnd, msg, wParam, lParam, result );

    /* Return the result to the sender task */
    ReplyMessage( result );

    queue->InSendMessageHandle = oldSender;
}


/***********************************************************************
 *           QUEUE_AddMsg
 *
 * Add a message to the queue. Return FALSE if queue is full.
 */
BOOL32 QUEUE_AddMsg( HQUEUE16 hQueue, MSG16 * msg, DWORD extraInfo )
{
    int pos;
    MESSAGEQUEUE *msgQueue;

    if (!(msgQueue = (MESSAGEQUEUE *)GlobalLock16( hQueue ))) return FALSE;
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
    QUEUE_SetWakeBit( msgQueue, QS_POSTMESSAGE );
    return TRUE;
}


/***********************************************************************
 *           QUEUE_FindMsg
 *
 * Find a message matching the given parameters. Return -1 if none available.
 */
int QUEUE_FindMsg( MESSAGEQUEUE * msgQueue, HWND32 hwnd, int first, int last )
{
    int i, pos = msgQueue->nextMessage;

    dprintf_msg(stddeb,"QUEUE_FindMsg: hwnd=%04x pos=%d\n", hwnd, pos );

    if (!msgQueue->msgCount) return -1;
    if (!hwnd && !first && !last) return pos;
        
    for (i = 0; i < msgQueue->msgCount; i++)
    {
	MSG16 * msg = &msgQueue->messages[pos].msg;

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
    if (!msgQueue->msgCount) msgQueue->wakeBits &= ~QS_POSTMESSAGE;
}


/***********************************************************************
 *           QUEUE_WakeSomeone
 *
 * Wake a queue upon reception of a hardware event.
 */
static void QUEUE_WakeSomeone( UINT message )
{
    HWND hwnd;
    WORD wakeBit;
    HQUEUE hQueue;
    MESSAGEQUEUE *queue = NULL;

    if ((message >= WM_KEYFIRST) && (message <= WM_KEYLAST)) wakeBit = QS_KEY;
    else wakeBit = (message == WM_MOUSEMOVE) ? QS_MOUSEMOVE : QS_MOUSEBUTTON;

    if (!(hwnd = GetSysModalWindow16()))
    {
        if (wakeBit == QS_KEY)
        {
            if (!(hwnd = GetFocus())) hwnd = GetActiveWindow();
        }
        else hwnd = GetCapture();
    }
    if (hwnd)
    {
        WND *wndPtr = WIN_FindWndPtr( hwnd );
        if (wndPtr) queue = (MESSAGEQUEUE *)GlobalLock16( wndPtr->hmemTaskQ );
    }
    else if (!(queue = pMouseQueue))
    {
        hQueue = hFirstQueue;
        while (hQueue)
        {
            queue = GlobalLock16( hQueue );
            if (queue->wakeBits & wakeBit) break;
            hQueue = queue->next;
        }
    }
    if (!queue) printf( "WakeSomeone: no one found\n" );
    if (queue) QUEUE_SetWakeBit( queue, wakeBit );
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
    MSG16 *msg;
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
    QUEUE_WakeSomeone( message );
}

		    
/***********************************************************************
 *	     QUEUE_GetQueueTask
 */
HTASK16 QUEUE_GetQueueTask( HQUEUE16 hQueue )
{
    MESSAGEQUEUE *queue = GlobalLock16( hQueue );
    return (queue) ? queue->hTask : 0 ;
}


/***********************************************************************
 *           QUEUE_IncPaintCount
 */
void QUEUE_IncPaintCount( HQUEUE16 hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( hQueue ))) return;
    queue->wPaintCount++;
    QUEUE_SetWakeBit( queue, QS_PAINT );
}


/***********************************************************************
 *           QUEUE_DecPaintCount
 */
void QUEUE_DecPaintCount( HQUEUE16 hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( hQueue ))) return;
    queue->wPaintCount--;
    if (!queue->wPaintCount) queue->wakeBits &= ~QS_PAINT;
}


/***********************************************************************
 *           QUEUE_IncTimerCount
 */
void QUEUE_IncTimerCount( HQUEUE16 hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( hQueue ))) return;
    queue->wTimerCount++;
    QUEUE_SetWakeBit( queue, QS_TIMER );
}


/***********************************************************************
 *           QUEUE_DecTimerCount
 */
void QUEUE_DecTimerCount( HQUEUE16 hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( hQueue ))) return;
    queue->wTimerCount--;
    if (!queue->wTimerCount) queue->wakeBits &= ~QS_TIMER;
}


/***********************************************************************
 *           PostQuitMessage   (USER.6)
 */
void PostQuitMessage( INT exitCode )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return;
    queue->wPostQMsg = TRUE;
    queue->wExitCode = (WORD)exitCode;
}


/***********************************************************************
 *           GetWindowTask16   (USER.224)
 */
HTASK16 GetWindowTask16( HWND16 hwnd )
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
    queuePtr = (MESSAGEQUEUE *)GlobalLock16( hNewQueue );
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

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return 0;
    ret = MAKELONG( queue->changeBits, queue->wakeBits );
    queue->changeBits = 0;
    return ret & MAKELONG( flags, flags );
}


/***********************************************************************
 *           GetInputState   (USER.335)
 */
BOOL GetInputState()
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return FALSE;
    return queue->wakeBits & (QS_KEY | QS_MOUSEBUTTON);
}


/***********************************************************************
 *           GetMessagePos   (USER.119)
 */
DWORD GetMessagePos(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return 0;
    return queue->GetMessagePosVal;
}


/***********************************************************************
 *           GetMessageTime   (USER.120)
 */
LONG GetMessageTime(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return 0;
    return queue->GetMessageTimeVal;
}


/***********************************************************************
 *           GetMessageExtraInfo   (USER.288)
 */
LONG GetMessageExtraInfo(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return 0;
    return queue->GetMessageExtraInfoVal;
}
