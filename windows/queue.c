/*
 * Message queues related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <stdlib.h>
#include <signal.h>
#include "module.h"
#include "queue.h"
#include "task.h"
#include "win.h"
#include "hook.h"
#include "thread.h"
#include "process.h"
#include "stddebug.h"
#include "debug.h"

#define MAX_QUEUE_SIZE   120  /* Max. size of a message queue */

static HQUEUE16 hFirstQueue = 0;
static HQUEUE16 hDoomedQueue = 0;
static HQUEUE16 hmemSysMsgQueue = 0;
static MESSAGEQUEUE *sysMsgQueue = NULL;

static MESSAGEQUEUE *pMouseQueue = NULL;  /* Queue for last mouse message */
static MESSAGEQUEUE *pKbdQueue = NULL;    /* Queue for last kbd message */

extern void SIGNAL_MaskAsyncEvents(BOOL32);

MESSAGEQUEUE *pCursorQueue = NULL; 
MESSAGEQUEUE *pActiveQueue = NULL;

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
    HQUEUE16 hQueue = hFirstQueue;

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
 *	     QUEUE_IsDoomedQueue
 */
BOOL32 QUEUE_IsDoomedQueue( HQUEUE16 hQueue )
{
    return (hDoomedQueue && (hQueue == hDoomedQueue));
}


/***********************************************************************
 *	     QUEUE_SetDoomedQueue
 */
void QUEUE_SetDoomedQueue( HQUEUE16 hQueue )
{
    hDoomedQueue = hQueue;
}


/***********************************************************************
 *           QUEUE_CreateMsgQueue
 *
 * Creates a message queue. Doesn't link it into queue list!
 */
static HQUEUE16 QUEUE_CreateMsgQueue( int size )
{
    HQUEUE16 hQueue;
    MESSAGEQUEUE * msgQueue;
    int queueSize;
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );

    dprintf_msg(stddeb,"Creating message queue...\n");

    queueSize = sizeof(MESSAGEQUEUE) + size * sizeof(QMSG);
    if (!(hQueue = GlobalAlloc16( GMEM_FIXED | GMEM_ZEROINIT, queueSize )))
        return 0;
    msgQueue = (MESSAGEQUEUE *) GlobalLock16( hQueue );
    msgQueue->self        = hQueue;
    msgQueue->msgSize     = sizeof(QMSG);
    msgQueue->queueSize   = size;
    msgQueue->wakeBits    = msgQueue->changeBits = QS_SMPARAMSFREE;
    msgQueue->wWinVersion = pTask ? pTask->version : 0;
    GlobalUnlock16( hQueue );
    return hQueue;
}


/***********************************************************************
 *	     QUEUE_DeleteMsgQueue
 *
 * Unlinks and deletes a message queue.
 *
 * Note: We need to mask asynchronous events to make sure PostMessage works
 * even in the signal handler.
 */
BOOL32 QUEUE_DeleteMsgQueue( HQUEUE16 hQueue )
{
    MESSAGEQUEUE * msgQueue = (MESSAGEQUEUE*)GlobalLock16(hQueue);
    HQUEUE16  senderQ;
    HQUEUE16 *pPrev;

    dprintf_msg(stddeb,"Deleting message queue %04x\n", hQueue);

    if (!hQueue || !msgQueue)
    {
	dprintf_msg(stddeb,"DeleteMsgQueue: invalid argument.\n");
	return 0;
    }
    if( pCursorQueue == msgQueue ) pCursorQueue = NULL;
    if( pActiveQueue == msgQueue ) pActiveQueue = NULL;

    /* flush sent messages */
    senderQ = msgQueue->hSendingTask;
    while( senderQ )
    {
      MESSAGEQUEUE* sq = (MESSAGEQUEUE*)GlobalLock16(senderQ);
      if( !sq ) break;
      sq->SendMessageReturn = 0L;
      QUEUE_SetWakeBit( sq, QS_SMRESULT );
      senderQ = sq->hPrevSendingTask;
    }

    SIGNAL_MaskAsyncEvents( TRUE );

    pPrev = &hFirstQueue;
    while (*pPrev && (*pPrev != hQueue))
    {
        MESSAGEQUEUE *msgQ = (MESSAGEQUEUE*)GlobalLock16(*pPrev);
        pPrev = &msgQ->next;
    }
    if (*pPrev) *pPrev = msgQueue->next;
    msgQueue->self = 0;

    SIGNAL_MaskAsyncEvents( FALSE );

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
    dprintf_msg(stddeb,"SetWakeBit: queue = %04x (wm=%04x), bit = %04x\n", 
	                queue->self, queue->wakeMask, bit );

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

    dprintf_msg(stddeb,"WaitBits: q %04x waiting for %04x\n", GetTaskQueue(0), bits);

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

	    queue->wakeMask = 0;
            QUEUE_ReceiveMessage( queue );
	    continue;				/* nested sm crux */
        }

        queue->wakeMask = bits | QS_SENDMESSAGE;
	if(queue->changeBits & bits) continue;
	
	dprintf_msg(stddeb,"wb: (%04x) wakeMask is %04x, waiting\n", queue->self, queue->wakeMask);

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
    MESSAGEQUEUE *senderQ = NULL;
    HQUEUE16      prevSender = 0;
    QSMCTRL*      prevCtrlPtr = NULL;
    LRESULT       result = 0;

    dprintf_msg(stddeb, "ReceiveMessage, queue %04x\n", queue->self );
    if (!(queue->wakeBits & QS_SENDMESSAGE) ||
        !(senderQ = (MESSAGEQUEUE*)GlobalLock16( queue->hSendingTask))) 
	{ dprintf_msg(stddeb,"\trcm: nothing to do\n"); return; }

    if( !senderQ->hPrevSendingTask )
    {
      queue->wakeBits &= ~QS_SENDMESSAGE;	/* no more sent messages */
      queue->changeBits &= ~QS_SENDMESSAGE;
    }

    /* Save current state on stack */
    prevSender                 = queue->InSendMessageHandle;
    prevCtrlPtr		       = queue->smResultCurrent;

    /* Remove sending queue from the list */
    queue->InSendMessageHandle = queue->hSendingTask;
    queue->smResultCurrent     = senderQ->smResultInit;
    queue->hSendingTask	       = senderQ->hPrevSendingTask;

    dprintf_msg(stddeb, "\trcm: smResultCurrent = %08x, prevCtrl = %08x\n", 
				(unsigned)queue->smResultCurrent, (unsigned)prevCtrlPtr );
    QUEUE_SetWakeBit( senderQ, QS_SMPARAMSFREE );

    dprintf_msg(stddeb, "\trcm: calling wndproc - %04x %04x %04x %08x\n",
            senderQ->hWnd, senderQ->msg, senderQ->wParam, (unsigned)senderQ->lParam );

    if (IsWindow32( senderQ->hWnd ))
    {
        DWORD extraInfo = queue->GetMessageExtraInfoVal;
        queue->GetMessageExtraInfoVal = senderQ->GetMessageExtraInfoVal;

        result = CallWindowProc16( (WNDPROC16)GetWindowLong16(senderQ->hWnd, GWL_WNDPROC),
				   senderQ->hWnd, senderQ->msg, senderQ->wParam, senderQ->lParam );

        queue->GetMessageExtraInfoVal = extraInfo;  /* Restore extra info */
	dprintf_msg(stddeb,"\trcm: result =  %08x\n", (unsigned)result );
    }
    else dprintf_msg(stddeb,"\trcm: bad hWnd\n");

    /* Return the result to the sender task */
    ReplyMessage16( result );

    queue->InSendMessageHandle = prevSender;
    queue->smResultCurrent     = prevCtrlPtr;

    dprintf_msg(stddeb,"ReceiveMessage: done!\n");
}

/***********************************************************************
 *           QUEUE_FlushMessage
 * 
 * Try to reply to all pending sent messages on exit.
 */
void QUEUE_FlushMessages( HQUEUE16 hQueue )
{
  MESSAGEQUEUE *queue = (MESSAGEQUEUE*)GlobalLock16( hQueue );

  if( queue )
  {
    MESSAGEQUEUE *senderQ = (MESSAGEQUEUE*)GlobalLock16( queue->hSendingTask);
    QSMCTRL*      CtrlPtr = queue->smResultCurrent;

    while( senderQ )
    {
      if( !(queue->hSendingTask = senderQ->hPrevSendingTask) )
            queue->wakeBits &= ~QS_SENDMESSAGE;
      QUEUE_SetWakeBit( senderQ, QS_SMPARAMSFREE );
      
      queue->smResultCurrent = CtrlPtr;
      while( senderQ->wakeBits & QS_SMRESULT ) OldYield();

      senderQ->SendMessageReturn = 0;
      senderQ->smResult = queue->smResultCurrent;
      QUEUE_SetWakeBit( senderQ, QS_SMRESULT);

      if( (senderQ = (MESSAGEQUEUE*)GlobalLock16( queue->hSendingTask)) )
	   CtrlPtr = senderQ->smResultInit;
    }
    queue->InSendMessageHandle = 0;
  }  
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

    SIGNAL_MaskAsyncEvents( TRUE );

    if (!(msgQueue = (MESSAGEQUEUE *)GlobalLock16( hQueue ))) return FALSE;
    pos = msgQueue->nextFreeMessage;

      /* Check if queue is full */
    if ((pos == msgQueue->nextMessage) && (msgQueue->msgCount > 0))
    {
	SIGNAL_MaskAsyncEvents( FALSE );
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

    SIGNAL_MaskAsyncEvents( FALSE );

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
    SIGNAL_MaskAsyncEvents( TRUE );

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

    SIGNAL_MaskAsyncEvents( FALSE );
}


/***********************************************************************
 *           QUEUE_WakeSomeone
 *
 * Wake a queue upon reception of a hardware event.
 */
static void QUEUE_WakeSomeone( UINT32 message )
{
    WND*	  wndPtr = NULL;
    WORD          wakeBit;
    HWND32 hwnd;
    MESSAGEQUEUE *queue = pCursorQueue;

    if( (message >= WM_KEYFIRST) && (message <= WM_KEYLAST) )
    {
       wakeBit = QS_KEY;
       if( pActiveQueue ) queue = pActiveQueue;
    }
    else 
    {
       wakeBit = (message == WM_MOUSEMOVE) ? QS_MOUSEMOVE : QS_MOUSEBUTTON;
       if( (hwnd = GetCapture32()) )
	 if( (wndPtr = WIN_FindWndPtr( hwnd )) ) 
	   queue = (MESSAGEQUEUE *)GlobalLock16( wndPtr->hmemTaskQ );
    }

    if( (hwnd = GetSysModalWindow16()) )
      if( (wndPtr = WIN_FindWndPtr( hwnd )) )
        queue = (MESSAGEQUEUE *)GlobalLock16( wndPtr->hmemTaskQ );

    if( !queue ) 
    {
      queue = GlobalLock16( hFirstQueue );
      while( queue )
      {
        if (queue->wakeMask & wakeBit) break;
        queue = GlobalLock16( queue->next );
      }
      if( !queue )
      { 
        dprintf_msg(stddeb,"WakeSomeone: couldn't find queue\n"); 
        return; 
      }
    }

    QUEUE_SetWakeBit( queue, wakeBit );
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
        if (message != WM_MOUSEMOVE) MessageBeep32(0);
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
 *           PostQuitMessage16   (USER.6)
 */
void PostQuitMessage16( INT16 exitCode )
{
    PostQuitMessage32( exitCode );
}


/***********************************************************************
 *           PostQuitMessage32   (USER32.420)
 */
void PostQuitMessage32( INT32 exitCode )
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
 *           GetWindowThreadProcessId   (USER32.312)
 */
DWORD GetWindowThreadProcessId( HWND32 hwnd, LPDWORD process )
{
    HTASK16 htask;
    TDB	*tdb;

    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr) return 0;
    htask=QUEUE_GetQueueTask( wndPtr->hmemTaskQ );
    tdb = (TDB*)GlobalLock16(htask);
    if (!tdb) return 0;
    if (tdb->thdb) {
    	if (process)
		*process = (DWORD)tdb->thdb->process;
	return (DWORD)tdb->thdb;
    }
    return 0;

}


/***********************************************************************
 *           SetMessageQueue16   (USER.266)
 */
BOOL16 SetMessageQueue16( INT16 size )
{
    return SetMessageQueue32( size );
}


/***********************************************************************
 *           SetMessageQueue32   (USER32.493)
 */
BOOL32 SetMessageQueue32( INT32 size )
{
    HQUEUE16 hQueue, hNewQueue;
    MESSAGEQUEUE *queuePtr;

    dprintf_msg(stddeb,"SetMessageQueue: task %04x size %i\n", GetCurrentTask(), size); 

    if ((size > MAX_QUEUE_SIZE) || (size <= 0)) return TRUE;

    if( !(hNewQueue = QUEUE_CreateMsgQueue( size ))) 
    {
	dprintf_msg(stddeb,"SetMessageQueue: failed!\n");
	return FALSE;
    }
    queuePtr = (MESSAGEQUEUE *)GlobalLock16( hNewQueue );

    SIGNAL_MaskAsyncEvents( TRUE );

    /* Copy data and free the old message queue */
    if ((hQueue = GetTaskQueue(0)) != 0) 
    {
       MESSAGEQUEUE *oldQ = (MESSAGEQUEUE *)GlobalLock16( hQueue );
       memcpy( &queuePtr->reserved2, &oldQ->reserved2, 
			(int)oldQ->messages - (int)(&oldQ->reserved2) );
       HOOK_ResetQueueHooks( hNewQueue );
       if( WIN_GetDesktop()->hmemTaskQ == hQueue )
	   WIN_GetDesktop()->hmemTaskQ = hNewQueue;
       WIN_ResetQueueWindows( WIN_GetDesktop()->child, hQueue, hNewQueue );
       QUEUE_DeleteMsgQueue( hQueue );
    }

    /* Link new queue into list */
    queuePtr->hTask = GetCurrentTask();
    queuePtr->next  = hFirstQueue;
    hFirstQueue = hNewQueue;
    
    if( !queuePtr->next ) pCursorQueue = queuePtr;
    SetTaskQueue( 0, hNewQueue );
    
    SIGNAL_MaskAsyncEvents( FALSE );
    return TRUE;
}


/***********************************************************************
 *           GetQueueStatus16   (USER.334)
 */
DWORD GetQueueStatus16( UINT16 flags )
{
    MESSAGEQUEUE *queue;
    DWORD ret;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return 0;
    ret = MAKELONG( queue->changeBits, queue->wakeBits );
    queue->changeBits = 0;
    return ret & MAKELONG( flags, flags );
}


/***********************************************************************
 *           GetInputState16   (USER.335)
 */
BOOL16 GetInputState16(void)
{
    return GetInputState32();
}


/***********************************************************************
 *           GetInputState32   (USER32.243)
 */
BOOL32 GetInputState32(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) )))
        return FALSE;
    return queue->wakeBits & (QS_KEY | QS_MOUSEBUTTON);
}


/***********************************************************************
 *           GetMessagePos   (USER.119) (USER32.271)
 */
DWORD GetMessagePos(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return 0;
    return queue->GetMessagePosVal;
}


/***********************************************************************
 *           GetMessageTime   (USER.120) (USER32.272)
 */
LONG GetMessageTime(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return 0;
    return queue->GetMessageTimeVal;
}


/***********************************************************************
 *           GetMessageExtraInfo   (USER.288) (USER32.270)
 */
LONG GetMessageExtraInfo(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return 0;
    return queue->GetMessageExtraInfoVal;
}
