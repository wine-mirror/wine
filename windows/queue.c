/*
 * Message queues related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <stdlib.h>
#include <signal.h>
#include "miscemu.h"
#include "module.h"
#include "queue.h"
#include "task.h"
#include "win.h"
#include "clipboard.h"
#include "hook.h"
#include "heap.h"
#include "thread.h"
#include "process.h"
#include "debug.h"

#define MAX_QUEUE_SIZE   120  /* Max. size of a message queue */

static HQUEUE16 hFirstQueue = 0;
static HQUEUE16 hExitingQueue = 0;
static HQUEUE16 hmemSysMsgQueue = 0;
static MESSAGEQUEUE *sysMsgQueue = NULL;

static MESSAGEQUEUE *pMouseQueue = NULL;  /* Queue for last mouse message */
static MESSAGEQUEUE *pKbdQueue = NULL;    /* Queue for last kbd message */

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
        WARN(msg, "%04x is not a queue handle\n", hQueue );
        return;
    }

    DUMP(    "next: %12.4x  Intertask SendMessage:\n"
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
    char module[10];
    HQUEUE16 hQueue = hFirstQueue;

    DUMP( "Queue Size Msgs Task\n" );
    while (hQueue)
    {
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock16( hQueue );
        if (!queue)
        {
            WARN( msg, "Bad queue handle %04x\n", hQueue );
            return;
        }
        if (!GetModuleName( queue->hTask, module, sizeof(module )))
            strcpy( module, "???" );
        DUMP( "%04x %5d %4d %04x %s\n", hQueue, queue->msgSize,
                 queue->msgCount, queue->hTask, module );
        hQueue = queue->next;
    }
    DUMP( "\n" );
}


/***********************************************************************
 *	     QUEUE_IsExitingQueue
 */
BOOL32 QUEUE_IsExitingQueue( HQUEUE16 hQueue )
{
    return (hExitingQueue && (hQueue == hExitingQueue));
}


/***********************************************************************
 *	     QUEUE_SetExitingQueue
 */
void QUEUE_SetExitingQueue( HQUEUE16 hQueue )
{
    hExitingQueue = hQueue;
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

    TRACE(msg,"Creating message queue...\n");

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

    TRACE(msg,"Deleting message queue %04x\n", hQueue);

    if (!hQueue || !msgQueue)
    {
	WARN(msg, "invalid argument.\n");
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
 *           QUEUE_Signal
 */
void QUEUE_Signal( HTASK16 hTask )
{
    PDB32 *pdb;
    THREAD_ENTRY *entry;
    int wakeup = FALSE;

    TDB *pTask = (TDB *)GlobalLock16( hTask );
    if ( !pTask ) return;

    TRACE(msg, "calling SYNC_MsgWakeUp\n");

    /* Wake up thread waiting for message */
    /* NOTE: This should really wake up *the* thread that owns
             the queue. Since we dont't have thread-local message
             queues yet, we wake up all waiting threads ... */
    SYSTEM_LOCK();
    pdb = pTask->thdb->process;
    entry = pdb? pdb->thread_list->next : NULL;

    if (entry)
        for (;;)
        {
            if (entry->thread->wait_struct.wait_msg)
            {
                SYNC_MsgWakeUp( entry->thread );
                wakeup = TRUE;
            }
            if (entry == pdb->thread_list) break;
            entry = entry->next;
        }
    SYSTEM_UNLOCK();

/*    if ( !wakeup )*/
        PostEvent( hTask );
}

/***********************************************************************
 *           QUEUE_Wait
 */
void QUEUE_Wait( void )
{
    if ( THREAD_IsWin16( THREAD_Current() ) )
        WaitEvent( 0 );
    else
    {
        TRACE(msg, "current task is 32-bit, calling SYNC_DoWait\n");
        SYNC_DoWait( 0, NULL, FALSE, INFINITE32, FALSE, TRUE );
    }
}


/***********************************************************************
 *           QUEUE_SetWakeBit
 *
 * See "Windows Internals", p.449
 */
void QUEUE_SetWakeBit( MESSAGEQUEUE *queue, WORD bit )
{
    TRACE(msg,"queue = %04x (wm=%04x), bit = %04x\n", 
	                queue->self, queue->wakeMask, bit );

    if (bit & QS_MOUSE) pMouseQueue = queue;
    if (bit & QS_KEY) pKbdQueue = queue;
    queue->changeBits |= bit;
    queue->wakeBits   |= bit;
    if (queue->wakeMask & bit)
    {
        queue->wakeMask = 0;
        QUEUE_Signal( queue->hTask );
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

    TRACE(msg,"q %04x waiting for %04x\n", GetTaskQueue(0), bits);

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
	
	TRACE(msg,"%04x) wakeMask is %04x, waiting\n", queue->self, queue->wakeMask);

        QUEUE_Wait();
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

    TRACE(msg, "ReceiveMessage, queue %04x\n", queue->self );
    if (!(queue->wakeBits & QS_SENDMESSAGE) ||
        !(senderQ = (MESSAGEQUEUE*)GlobalLock16( queue->hSendingTask))) 
	{ TRACE(msg,"\trcm: nothing to do\n"); return; }

    if( !senderQ->hPrevSendingTask )
        QUEUE_ClearWakeBit( queue, QS_SENDMESSAGE );   /* no more sent messages */

    /* Save current state on stack */
    prevSender                 = queue->InSendMessageHandle;
    prevCtrlPtr		       = queue->smResultCurrent;

    /* Remove sending queue from the list */
    queue->InSendMessageHandle = queue->hSendingTask;
    queue->smResultCurrent     = senderQ->smResultInit;
    queue->hSendingTask	       = senderQ->hPrevSendingTask;

    TRACE(msg, "\trcm: smResultCurrent = %08x, prevCtrl = %08x\n", 
				(unsigned)queue->smResultCurrent, (unsigned)prevCtrlPtr );
    QUEUE_SetWakeBit( senderQ, QS_SMPARAMSFREE );

    TRACE(msg, "\trcm: calling wndproc - %04x %04x %04x%04x %08x\n",
                senderQ->hWnd, senderQ->msg, senderQ->wParamHigh,
                senderQ->wParam, (unsigned)senderQ->lParam );

    if (IsWindow32( senderQ->hWnd ))
    {
        WND *wndPtr = WIN_FindWndPtr( senderQ->hWnd );
        DWORD extraInfo = queue->GetMessageExtraInfoVal;
        queue->GetMessageExtraInfoVal = senderQ->GetMessageExtraInfoVal;

        if (senderQ->flags & QUEUE_SM_WIN32)
        {
            WPARAM32 wParam = MAKELONG( senderQ->wParam, senderQ->wParamHigh );
            TRACE(msg, "\trcm: msg is Win32\n" );
            if (senderQ->flags & QUEUE_SM_UNICODE)
                result = CallWindowProc32W( wndPtr->winproc,
                                            senderQ->hWnd, senderQ->msg,
                                            wParam, senderQ->lParam );
            else
                result = CallWindowProc32A( wndPtr->winproc,
                                            senderQ->hWnd, senderQ->msg,
                                            wParam, senderQ->lParam );
        }
        else  /* Win16 message */
            result = CallWindowProc16( (WNDPROC16)wndPtr->winproc,
                                       senderQ->hWnd, senderQ->msg,
                                       senderQ->wParam, senderQ->lParam );

        queue->GetMessageExtraInfoVal = extraInfo;  /* Restore extra info */
	TRACE(msg,"\trcm: result =  %08x\n", (unsigned)result );
    }
    else WARN(msg, "\trcm: bad hWnd\n");

    /* Return the result to the sender task */
    ReplyMessage16( result );

    queue->InSendMessageHandle = prevSender;
    queue->smResultCurrent     = prevCtrlPtr;

    TRACE(msg,"done!\n");
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

    TRACE(msg,"Flushing queue %04x:\n", hQueue );

    while( senderQ )
    {
      if( !CtrlPtr )
	   CtrlPtr = senderQ->smResultInit;

      TRACE(msg,"\tfrom queue %04x, smResult %08x\n", queue->hSendingTask, (unsigned)CtrlPtr );

      if( !(queue->hSendingTask = senderQ->hPrevSendingTask) )
        QUEUE_ClearWakeBit( queue, QS_SENDMESSAGE );

      QUEUE_SetWakeBit( senderQ, QS_SMPARAMSFREE );
      
      queue->smResultCurrent = CtrlPtr;
      while( senderQ->wakeBits & QS_SMRESULT ) OldYield();

      senderQ->SendMessageReturn = 0;
      senderQ->smResult = queue->smResultCurrent;
      QUEUE_SetWakeBit( senderQ, QS_SMRESULT);

      senderQ = (MESSAGEQUEUE*)GlobalLock16( queue->hSendingTask);
      CtrlPtr = NULL;
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
        WARN( msg,"Queue is full!\n" );
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

    TRACE(msg,"hwnd=%04x pos=%d\n", hwnd, pos );

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
        WARN(msg, "couldn't find queue\n"); 
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
void WINAPI PostQuitMessage16( INT16 exitCode )
{
    PostQuitMessage32( exitCode );
}


/***********************************************************************
 *           PostQuitMessage32   (USER32.421)
 *
 * PostQuitMessage() posts a message to the system requesting an
 * application to terminate execution. As a result of this function,
 * the WM_QUIT message is posted to the application, and
 * PostQuitMessage() returns immediately.  The exitCode parameter
 * specifies an application-defined exit code, which appears in the
 * _wParam_ parameter of the WM_QUIT message posted to the application.  
 *
 * CONFORMANCE
 *
 *  ECMA-234, Win32
 */
void WINAPI PostQuitMessage32( INT32 exitCode )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return;
    queue->wPostQMsg = TRUE;
    queue->wExitCode = (WORD)exitCode;
}


/***********************************************************************
 *           GetWindowTask16   (USER.224)
 */
HTASK16 WINAPI GetWindowTask16( HWND16 hwnd )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr) return 0;
    return QUEUE_GetQueueTask( wndPtr->hmemTaskQ );
}

/***********************************************************************
 *           GetWindowThreadProcessId   (USER32.313)
 */
DWORD WINAPI GetWindowThreadProcessId( HWND32 hwnd, LPDWORD process )
{
    HTASK16 htask;
    TDB	*tdb;

    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr) return 0;
    htask=QUEUE_GetQueueTask( wndPtr->hmemTaskQ );
    tdb = (TDB*)GlobalLock16(htask);
    if (!tdb || !tdb->thdb) return 0;
    if (process) *process = PDB_TO_PROCESS_ID( tdb->thdb->process );
    return THDB_TO_THREAD_ID( tdb->thdb );
}


/***********************************************************************
 *           SetMessageQueue16   (USER.266)
 */
BOOL16 WINAPI SetMessageQueue16( INT16 size )
{
    return SetMessageQueue32( size );
}


/***********************************************************************
 *           SetMessageQueue32   (USER32.494)
 */
BOOL32 WINAPI SetMessageQueue32( INT32 size )
{
    HQUEUE16 hQueue, hNewQueue;
    MESSAGEQUEUE *queuePtr;

    TRACE(msg,"task %04x size %i\n", GetCurrentTask(), size); 

    if ((size > MAX_QUEUE_SIZE) || (size <= 0)) return FALSE;

    if( !(hNewQueue = QUEUE_CreateMsgQueue( size ))) 
    {
	WARN(msg, "failed!\n");
	return FALSE;
    }
    queuePtr = (MESSAGEQUEUE *)GlobalLock16( hNewQueue );

    SIGNAL_MaskAsyncEvents( TRUE );

    /* Copy data and free the old message queue */
    if ((hQueue = GetTaskQueue(0)) != 0) 
    {
       MESSAGEQUEUE *oldQ = (MESSAGEQUEUE *)GlobalLock16( hQueue );
       memcpy( &queuePtr->wParamHigh, &oldQ->wParamHigh,
			(int)oldQ->messages - (int)(&oldQ->wParamHigh) );
       HOOK_ResetQueueHooks( hNewQueue );
       if( WIN_GetDesktop()->hmemTaskQ == hQueue )
	   WIN_GetDesktop()->hmemTaskQ = hNewQueue;
       WIN_ResetQueueWindows( WIN_GetDesktop(), hQueue, hNewQueue );
       CLIPBOARD_ResetLock( hQueue, hNewQueue );
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
DWORD WINAPI GetQueueStatus16( UINT16 flags )
{
    MESSAGEQUEUE *queue;
    DWORD ret;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return 0;
    ret = MAKELONG( queue->changeBits, queue->wakeBits );
    queue->changeBits = 0;
    return ret & MAKELONG( flags, flags );
}

/***********************************************************************
 *           GetQueueStatus32   (USER32.283)
 */
DWORD WINAPI GetQueueStatus32( UINT32 flags )
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
BOOL16 WINAPI GetInputState16(void)
{
    return GetInputState32();
}

/***********************************************************************
 *           WaitForInputIdle   (USER32.577)
 */
DWORD WINAPI WaitForInputIdle (HANDLE32 hProcess, DWORD dwTimeOut)
{
  FIXME (msg, "(hProcess=%d, dwTimeOut=%ld): stub\n", hProcess, dwTimeOut);

  return WAIT_TIMEOUT;
}


/***********************************************************************
 *           GetInputState32   (USER32.244)
 */
BOOL32 WINAPI GetInputState32(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) )))
        return FALSE;
    return queue->wakeBits & (QS_KEY | QS_MOUSEBUTTON);
}


/***********************************************************************
 *           GetMessagePos   (USER.119) (USER32.272)
 * 
 * The GetMessagePos() function returns a long value representing a
 * cursor position, in screen coordinates, when the last message
 * retrieved by the GetMessage() function occurs. The x-coordinate is
 * in the low-order word of the return value, the y-coordinate is in
 * the high-order word. The application can use the MAKEPOINT()
 * macro to obtain a POINT structure from the return value. 
 *
 * For the current cursor position, use GetCursorPos().
 *
 * RETURNS
 *
 * Cursor position of last message on success, zero on failure.
 *
 * CONFORMANCE
 *
 * ECMA-234, Win32
 *
 */
DWORD WINAPI GetMessagePos(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return 0;
    return queue->GetMessagePosVal;
}


/***********************************************************************
 *           GetMessageTime   (USER.120) (USER32.273)
 *
 * GetMessageTime() returns the message time for the last message
 * retrieved by the function. The time is measured in milliseconds with
 * the same offset as GetTickCount().
 *
 * Since the tick count wraps, this is only useful for moderately short
 * relative time comparisons.
 *
 * RETURNS
 *
 * Time of last message on success, zero on failure.
 *
 * CONFORMANCE
 *
 * ECMA-234, Win32
 *  
 */
LONG WINAPI GetMessageTime(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return 0;
    return queue->GetMessageTimeVal;
}


/***********************************************************************
 *           GetMessageExtraInfo   (USER.288) (USER32.271)
 */
LONG WINAPI GetMessageExtraInfo(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) ))) return 0;
    return queue->GetMessageExtraInfoVal;
}
