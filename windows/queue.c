/* * Message queues related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <stdlib.h>
#include <signal.h>
#include "wine/winbase16.h"
#include "wine/winuser16.h"
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
#include <assert.h>
#include "debug.h"

#define MAX_QUEUE_SIZE   120  /* Max. size of a message queue */

static HQUEUE16 hFirstQueue = 0;
static HQUEUE16 hExitingQueue = 0;
static HQUEUE16 hmemSysMsgQueue = 0;
static MESSAGEQUEUE *sysMsgQueue = NULL;
static PERQUEUEDATA *pQDataWin16 = NULL;  /* Global perQData for Win16 tasks */

static MESSAGEQUEUE *pMouseQueue = NULL;  /* Queue for last mouse message */
static MESSAGEQUEUE *pKbdQueue = NULL;    /* Queue for last kbd message */

HQUEUE16 hCursorQueue = 0;
HQUEUE16 hActiveQueue = 0;


/***********************************************************************
 *           PERQDATA_CreateInstance
 *
 * Creates an instance of a reference counted PERQUEUEDATA element
 * for the message queue. perQData is stored globally for 16 bit tasks.
 *
 * Note: We don't implement perQdata exactly the same way Windows does.
 * Each perQData element is reference counted since it may be potentially
 * shared by multiple message Queues (via AttachThreadInput).
 * We only store the current values for Active, Capture and focus windows
 * currently.
 */
PERQUEUEDATA * PERQDATA_CreateInstance( )
{
    PERQUEUEDATA *pQData;
    
    BOOL16 bIsWin16 = 0;
    
    TRACE(msg,"()\n");

    /* Share a single instance of perQData for all 16 bit tasks */
    if ( ( bIsWin16 = THREAD_IsWin16( THREAD_Current() ) ) )
    {
        /* If previously allocated, just bump up ref count */
        if ( pQDataWin16 )
        {
            PERQDATA_Addref( pQDataWin16 );
            return pQDataWin16;
        }
    }

    /* Allocate PERQUEUEDATA from the system heap */
    if (!( pQData = (PERQUEUEDATA *) HeapAlloc( SystemHeap, 0,
                                                    sizeof(PERQUEUEDATA) ) ))
        return 0;

    /* Initialize */
    pQData->hWndCapture = pQData->hWndFocus = pQData->hWndActive = 0;
    pQData->ulRefCount = 1;
    pQData->nCaptureHT = HTCLIENT;

    /* Note: We have an independent critical section for the per queue data
     * since this may be shared by different threads. see AttachThreadInput()
     */
    InitializeCriticalSection( &pQData->cSection );

    /* Save perQData globally for 16 bit tasks */
    if ( bIsWin16 )
        pQDataWin16 = pQData;
        
    return pQData;
}


/***********************************************************************
 *           PERQDATA_Addref
 *
 * Increment reference count for the PERQUEUEDATA instance
 * Returns reference count for debugging purposes
 */
ULONG PERQDATA_Addref( PERQUEUEDATA *pQData )
{
    assert(pQData != 0 );
    TRACE(msg,"(): current refcount %lu ...\n", pQData->ulRefCount);

    EnterCriticalSection( &pQData->cSection );
    ++pQData->ulRefCount;
    LeaveCriticalSection( &pQData->cSection );

    return pQData->ulRefCount;
}


/***********************************************************************
 *           PERQDATA_Release
 *
 * Release a reference to a PERQUEUEDATA instance.
 * Destroy the instance if no more references exist
 * Returns reference count for debugging purposes
 */
ULONG PERQDATA_Release( PERQUEUEDATA *pQData )
{
    assert(pQData != 0 );
    TRACE(msg,"(): current refcount %lu ...\n",
          (LONG)pQData->ulRefCount );

    EnterCriticalSection( &pQData->cSection );
    if ( --pQData->ulRefCount == 0 )
    {
        LeaveCriticalSection( &pQData->cSection );
        DeleteCriticalSection( &pQData->cSection );

        TRACE(msg,"(): deleting PERQUEUEDATA instance ...\n" );

        /* Deleting our global 16 bit perQData? */
        if ( pQData == pQDataWin16 )
            pQDataWin16 = 0;
            
        /* Free the PERQUEUEDATA instance */
        HeapFree( SystemHeap, 0, pQData );

        return 0;
    }
    LeaveCriticalSection( &pQData->cSection );

    return pQData->ulRefCount;
}


/***********************************************************************
 *           PERQDATA_GetFocusWnd
 *
 * Get the focus hwnd member in a threadsafe manner
 */
HWND32 PERQDATA_GetFocusWnd( PERQUEUEDATA *pQData )
{
    HWND32 hWndFocus;
    assert(pQData != 0 );

    EnterCriticalSection( &pQData->cSection );
    hWndFocus = pQData->hWndFocus;
    LeaveCriticalSection( &pQData->cSection );

    return hWndFocus;
}


/***********************************************************************
 *           PERQDATA_SetFocusWnd
 *
 * Set the focus hwnd member in a threadsafe manner
 */
HWND32 PERQDATA_SetFocusWnd( PERQUEUEDATA *pQData, HWND32 hWndFocus )
{
    HWND32 hWndFocusPrv;
    assert(pQData != 0 );

    EnterCriticalSection( &pQData->cSection );
    hWndFocusPrv = pQData->hWndFocus;
    pQData->hWndFocus = hWndFocus;
    LeaveCriticalSection( &pQData->cSection );

    return hWndFocusPrv;
}


/***********************************************************************
 *           PERQDATA_GetActiveWnd
 *
 * Get the active hwnd member in a threadsafe manner
 */
HWND32 PERQDATA_GetActiveWnd( PERQUEUEDATA *pQData )
{
    HWND32 hWndActive;
    assert(pQData != 0 );

    EnterCriticalSection( &pQData->cSection );
    hWndActive = pQData->hWndActive;
    LeaveCriticalSection( &pQData->cSection );

    return hWndActive;
}


/***********************************************************************
 *           PERQDATA_SetActiveWnd
 *
 * Set the active focus hwnd member in a threadsafe manner
 */
HWND32 PERQDATA_SetActiveWnd( PERQUEUEDATA *pQData, HWND32 hWndActive )
{
    HWND32 hWndActivePrv;
    assert(pQData != 0 );

    EnterCriticalSection( &pQData->cSection );
    hWndActivePrv = pQData->hWndActive;
    pQData->hWndActive = hWndActive;
    LeaveCriticalSection( &pQData->cSection );

    return hWndActivePrv;
}


/***********************************************************************
 *           PERQDATA_GetCaptureWnd
 *
 * Get the capture hwnd member in a threadsafe manner
 */
HWND32 PERQDATA_GetCaptureWnd( PERQUEUEDATA *pQData )
{
    HWND32 hWndCapture;
    assert(pQData != 0 );

    EnterCriticalSection( &pQData->cSection );
    hWndCapture = pQData->hWndCapture;
    LeaveCriticalSection( &pQData->cSection );

    return hWndCapture;
}


/***********************************************************************
 *           PERQDATA_SetCaptureWnd
 *
 * Set the capture hwnd member in a threadsafe manner
 */
HWND32 PERQDATA_SetCaptureWnd( PERQUEUEDATA *pQData, HWND32 hWndCapture )
{
    HWND32 hWndCapturePrv;
    assert(pQData != 0 );

    EnterCriticalSection( &pQData->cSection );
    hWndCapturePrv = pQData->hWndCapture;
    pQData->hWndCapture = hWndCapture;
    LeaveCriticalSection( &pQData->cSection );

    return hWndCapturePrv;
}


/***********************************************************************
 *           PERQDATA_GetCaptureInfo
 *
 * Get the capture info member in a threadsafe manner
 */
INT16 PERQDATA_GetCaptureInfo( PERQUEUEDATA *pQData )
{
    INT16 nCaptureHT;
    assert(pQData != 0 );

    EnterCriticalSection( &pQData->cSection );
    nCaptureHT = pQData->nCaptureHT;
    LeaveCriticalSection( &pQData->cSection );

    return nCaptureHT;
}


/***********************************************************************
 *           PERQDATA_SetCaptureInfo
 *
 * Set the capture info member in a threadsafe manner
 */
INT16 PERQDATA_SetCaptureInfo( PERQUEUEDATA *pQData, INT16 nCaptureHT )
{
    INT16 nCaptureHTPrv;
    assert(pQData != 0 );

    EnterCriticalSection( &pQData->cSection );
    nCaptureHTPrv = pQData->nCaptureHT;
    pQData->nCaptureHT = nCaptureHT;
    LeaveCriticalSection( &pQData->cSection );

    return nCaptureHTPrv;
}


/***********************************************************************
 *	     QUEUE_Lock
 *
 * Function for getting a 32 bit pointer on queue strcture. For thread
 * safeness programmers should use this function instead of GlobalLock to
 * retrieve a pointer on the structure. QUEUE_Unlock should also be called
 * when access to the queue structure is not required anymore.
 */
MESSAGEQUEUE *QUEUE_Lock( HQUEUE16 hQueue )
{
    MESSAGEQUEUE *queue;

    SYSTEM_LOCK();
    queue = GlobalLock16( hQueue );
    if ( !queue || (queue->magic != QUEUE_MAGIC) )
    {
        SYSTEM_UNLOCK();
        return NULL;
    }

    queue->lockCount++;
    SYSTEM_UNLOCK();

    return queue;
}


/***********************************************************************
 *	     QUEUE_Unlock
 *
 * Use with QUEUE_Lock to get a thread safe acces to message queue
 * structure
 */
void QUEUE_Unlock( MESSAGEQUEUE *queue )
{
    if (queue)
    {
        SYSTEM_LOCK();

        if ( --queue->lockCount == 0 )
        {
            DeleteCriticalSection ( &queue->cSection );
            if (queue->hEvent)
                CloseHandle( queue->hEvent );
            GlobalFree16( queue->self );
        }
    
        SYSTEM_UNLOCK();
    }
}


/***********************************************************************
 *	     QUEUE_DumpQueue
 */
void QUEUE_DumpQueue( HQUEUE16 hQueue )
{
    MESSAGEQUEUE *pq; 

    if (!(pq = (MESSAGEQUEUE*) QUEUE_Lock( hQueue )) )
    {
        WARN(msg, "%04x is not a queue handle\n", hQueue );
        return;
    }

    DUMP(    "next: %12.4x  Intertask SendMessage:\n"
             "thread: %10p  ----------------------\n"
             "hWnd: %12.8x\n"
             "firstMsg: %8p   msg:     %11.8x\n"
             "lastMsg:  %8p   wParam:   %10.8x\n"
             "msgCount: %8.4x   lParam:   %10.8x\n"
             "lockCount: %7.4x   lRet:   %12.8x\n"
             "wWinVer: %9.4x  ISMH: %10.4x\n"
             "paints: %10.4x  hSendTask: %5.4x\n"
             "timers: %10.4x  hPrevSend: %5.4x\n"
             "wakeBits: %8.4x\n"
             "wakeMask: %8.4x\n"
             "hCurHook: %8.4x\n",
             pq->next, pq->thdb, pq->hWnd32, pq->firstMsg, pq->msg32,
             pq->lastMsg, pq->wParam32, pq->msgCount, (unsigned)pq->lParam,
             (unsigned)pq->lockCount, (unsigned)pq->SendMessageReturn,
             pq->wWinVersion, pq->InSendMessageHandle,
             pq->wPaintCount, pq->hSendingTask, pq->wTimerCount,
             pq->hPrevSendingTask, pq->wakeBits, pq->wakeMask, pq->hCurHook);

    QUEUE_Unlock( pq );
}


/***********************************************************************
 *	     QUEUE_WalkQueues
 */
void QUEUE_WalkQueues(void)
{
    char module[10];
    HQUEUE16 hQueue = hFirstQueue;

    DUMP( "Queue Msgs Thread   Task Module\n" );
    while (hQueue)
    {
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)QUEUE_Lock( hQueue );
        if (!queue)
        {
            WARN( msg, "Bad queue handle %04x\n", hQueue );
            return;
        }
        if (!GetModuleName( queue->thdb->process->task, module, sizeof(module )))
            strcpy( module, "???" );
        DUMP( "%04x %4d %p %04x %s\n", hQueue,queue->msgCount,
              queue->thdb, queue->thdb->process->task, module );
        hQueue = queue->next;
        QUEUE_Unlock( queue );
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
static HQUEUE16 QUEUE_CreateMsgQueue( BOOL16 bCreatePerQData )
{
    HQUEUE16 hQueue;
    MESSAGEQUEUE * msgQueue;
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );

    TRACE(msg,"(): Creating message queue...\n");

    if (!(hQueue = GlobalAlloc16( GMEM_FIXED | GMEM_ZEROINIT,
                                  sizeof(MESSAGEQUEUE) )))
        return 0;

    msgQueue = (MESSAGEQUEUE *) GlobalLock16( hQueue );
    if ( !msgQueue )
        return 0;

    msgQueue->self        = hQueue;
    msgQueue->wakeBits    = msgQueue->changeBits = QS_SMPARAMSFREE;
    msgQueue->wWinVersion = pTask ? pTask->version : 0;
    
    InitializeCriticalSection( &msgQueue->cSection );
    MakeCriticalSectionGlobal( &msgQueue->cSection );

    /* Create an Event object for waiting on message, used by win32 thread
       only */
    if ( !THREAD_IsWin16( THREAD_Current() ) )
    {
        msgQueue->hEvent = CreateEvent32A( NULL, FALSE, FALSE, NULL);

        if (msgQueue->hEvent == 0)
        {
            WARN(msg, "CreateEvent32A is not able to create an event object");
            return 0;
        }
        msgQueue->hEvent = ConvertToGlobalHandle( msgQueue->hEvent );
    }
    else
        msgQueue->hEvent = 0;
         
    msgQueue->lockCount = 1;
    msgQueue->magic = QUEUE_MAGIC;
    
    /* Create and initialize our per queue data */
    msgQueue->pQData = bCreatePerQData ? PERQDATA_CreateInstance() : NULL;
    
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
    MESSAGEQUEUE * msgQueue = (MESSAGEQUEUE*)QUEUE_Lock(hQueue);
    HQUEUE16  senderQ;
    HQUEUE16 *pPrev;

    TRACE(msg,"(): Deleting message queue %04x\n", hQueue);

    if (!hQueue || !msgQueue)
    {
	WARN(msg, "invalid argument.\n");
	return 0;
    }

    msgQueue->magic = 0;
    
    if( hCursorQueue == hQueue ) hCursorQueue = 0;
    if( hActiveQueue == hQueue ) hActiveQueue = 0;

    /* flush sent messages */
    senderQ = msgQueue->hSendingTask;
    while( senderQ )
    {
      MESSAGEQUEUE* sq = (MESSAGEQUEUE*)QUEUE_Lock(senderQ);
      if( !sq ) break;
      sq->SendMessageReturn = 0L;
      QUEUE_SetWakeBit( sq, QS_SMRESULT );
      senderQ = sq->hPrevSendingTask;
      QUEUE_Unlock(sq);
    }

    SYSTEM_LOCK();

    /* Release per queue data if present */
    if ( msgQueue->pQData )
    {
        PERQDATA_Release( msgQueue->pQData );
        msgQueue->pQData = 0;
    }
    
    /* remove the message queue from the global link list */
    pPrev = &hFirstQueue;
    while (*pPrev && (*pPrev != hQueue))
    {
        MESSAGEQUEUE *msgQ = (MESSAGEQUEUE*)GlobalLock16(*pPrev);

        /* sanity check */
        if ( !msgQ || (msgQ->magic != QUEUE_MAGIC) )
        {
            /* HQUEUE link list is corrupted, try to exit gracefully */
            WARN( msg, "HQUEUE link list corrupted!\n");
            pPrev = 0;
            break;
        }
        pPrev = &msgQ->next;
    }
    if (pPrev && *pPrev) *pPrev = msgQueue->next;
    msgQueue->self = 0;

    SYSTEM_UNLOCK();

    /* free up resource used by MESSAGEQUEUE strcture */
    msgQueue->lockCount--;
    QUEUE_Unlock( msgQueue );
    
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
    /* Note: We dont need perQ data for the system message queue */
    if (!(hmemSysMsgQueue = QUEUE_CreateMsgQueue( FALSE )))
        return FALSE;
    
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
 *           QUEUE_Wait
 */
static void QUEUE_Wait( DWORD wait_mask )
{
    if ( THREAD_IsWin16( THREAD_Current() ) )
        WaitEvent( 0 );
    else
    {
        TRACE(msg, "current task is 32-bit, calling SYNC_DoWait\n");
        MsgWaitForMultipleObjects( 0, NULL, FALSE, INFINITE32, wait_mask );
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
        
        /* Wake up thread waiting for message */
        if ( THREAD_IsWin16( queue->thdb ) )
            PostEvent( queue->thdb->process->task );
        else
        {
            SetEvent( queue->hEvent );
        }
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

    TRACE(msg,"q %04x waiting for %04x\n", GetFastQueue(), bits);

    for (;;)
    {
        if (!(queue = (MESSAGEQUEUE *)QUEUE_Lock( GetFastQueue() ))) return;

        if (queue->changeBits & bits)
        {
            /* One of the bits is set; we can return */
            queue->wakeMask = 0;
            QUEUE_Unlock( queue );
            return;
        }
        if (queue->wakeBits & QS_SENDMESSAGE)
        {
            /* Process the sent message immediately */

	    queue->wakeMask = 0;
            QUEUE_ReceiveMessage( queue );
            QUEUE_Unlock( queue );
	    continue;				/* nested sm crux */
        }

        queue->wakeMask = bits | QS_SENDMESSAGE;
        if(queue->changeBits & bits)
        {
            QUEUE_Unlock( queue );
            continue;
        }
	
	TRACE(msg,"%04x) wakeMask is %04x, waiting\n", queue->self, queue->wakeMask);

        QUEUE_Wait( queue->wakeMask );

        QUEUE_Unlock( queue );
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
        !(senderQ = (MESSAGEQUEUE*)QUEUE_Lock( queue->hSendingTask)))
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

    TRACE(msg, "\trcm: calling wndproc - %08x %08x %08x %08x\n",
                senderQ->hWnd32, senderQ->msg32,
                senderQ->wParam32, (unsigned)senderQ->lParam );

    if (IsWindow32( senderQ->hWnd32 ))
    {
        WND *wndPtr = WIN_FindWndPtr( senderQ->hWnd32 );
        DWORD extraInfo = queue->GetMessageExtraInfoVal;
        queue->GetMessageExtraInfoVal = senderQ->GetMessageExtraInfoVal;

        if (senderQ->flags & QUEUE_SM_WIN32)
        {
            TRACE(msg, "\trcm: msg is Win32\n" );
            if (senderQ->flags & QUEUE_SM_UNICODE)
                result = CallWindowProc32W( wndPtr->winproc,
                                            senderQ->hWnd32, senderQ->msg32,
                                            senderQ->wParam32, senderQ->lParam );
            else
                result = CallWindowProc32A( wndPtr->winproc,
                                            senderQ->hWnd32, senderQ->msg32,
                                            senderQ->wParam32, senderQ->lParam );
        }
        else  /* Win16 message */
            result = CallWindowProc16( (WNDPROC16)wndPtr->winproc,
                                       (HWND16) senderQ->hWnd32,
                                       (UINT16) senderQ->msg32,
                                       LOWORD (senderQ->wParam32),
                                       senderQ->lParam );

        queue->GetMessageExtraInfoVal = extraInfo;  /* Restore extra info */
	TRACE(msg,"\trcm: result =  %08x\n", (unsigned)result );
    }
    else WARN(msg, "\trcm: bad hWnd\n");

    QUEUE_Unlock( senderQ );
    
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
  MESSAGEQUEUE *queue = (MESSAGEQUEUE*)QUEUE_Lock( hQueue );

  if( queue )
  {
    MESSAGEQUEUE *senderQ = (MESSAGEQUEUE*)QUEUE_Lock( queue->hSendingTask );
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

      QUEUE_Unlock( senderQ );

      senderQ = (MESSAGEQUEUE*)QUEUE_Lock( queue->hSendingTask );
      CtrlPtr = NULL;
    }
    queue->InSendMessageHandle = 0;
    
    QUEUE_Unlock( queue );
  }  

}

/***********************************************************************
 *           QUEUE_AddMsg
 *
 * Add a message to the queue. Return FALSE if queue is full.
 */
BOOL32 QUEUE_AddMsg( HQUEUE16 hQueue, MSG32 *msg, DWORD extraInfo )
{
    MESSAGEQUEUE *msgQueue;
    QMSG         *qmsg;


    if (!(msgQueue = (MESSAGEQUEUE *)QUEUE_Lock( hQueue ))) return FALSE;

    /* allocate new message in global heap for now */
    if (!(qmsg = (QMSG *) HeapAlloc( SystemHeap, 0, sizeof(QMSG) ) ))
    {
        QUEUE_Unlock( msgQueue );
        return 0;
    }

    EnterCriticalSection( &msgQueue->cSection );

      /* Store message */
    qmsg->msg = *msg;
    qmsg->extraInfo = extraInfo;

    /* insert the message in the link list */
    qmsg->nextMsg = 0;
    qmsg->prevMsg = msgQueue->lastMsg;

    if (msgQueue->lastMsg)
        msgQueue->lastMsg->nextMsg = qmsg;

    /* update first and last anchor in message queue */
    msgQueue->lastMsg = qmsg;
    if (!msgQueue->firstMsg)
        msgQueue->firstMsg = qmsg;
    
    msgQueue->msgCount++;

    LeaveCriticalSection( &msgQueue->cSection );

    QUEUE_SetWakeBit( msgQueue, QS_POSTMESSAGE );
    QUEUE_Unlock( msgQueue );
    
    return TRUE;
}



/***********************************************************************
 *           QUEUE_FindMsg
 *
 * Find a message matching the given parameters. Return -1 if none available.
 */
QMSG* QUEUE_FindMsg( MESSAGEQUEUE * msgQueue, HWND32 hwnd, int first, int last )
{
    QMSG* qmsg;

    EnterCriticalSection( &msgQueue->cSection );

    if (!msgQueue->msgCount)
        qmsg = 0;
    else if (!hwnd && !first && !last)
        qmsg = msgQueue->firstMsg;
    else
    {
        /* look in linked list for message matching first and last criteria */
        for (qmsg = msgQueue->firstMsg; qmsg; qmsg = qmsg->nextMsg)
    {
            MSG32 *msg = &(qmsg->msg);

	if (!hwnd || (msg->hwnd == hwnd))
	{
                if (!first && !last)
                    break;   /* found it */
                
                if ((msg->message >= first) && (msg->message <= last))
                    break;   /* found it */
            }
	}
    }
    
    LeaveCriticalSection( &msgQueue->cSection );

    return qmsg;
}



/***********************************************************************
 *           QUEUE_RemoveMsg
 *
 * Remove a message from the queue (pos must be a valid position).
 */
void QUEUE_RemoveMsg( MESSAGEQUEUE * msgQueue, QMSG *qmsg )
{
    EnterCriticalSection( &msgQueue->cSection );

    /* set the linked list */
    if (qmsg->prevMsg)
        qmsg->prevMsg->nextMsg = qmsg->nextMsg;

    if (qmsg->nextMsg)
        qmsg->nextMsg->prevMsg = qmsg->prevMsg;

    if (msgQueue->firstMsg == qmsg)
        msgQueue->firstMsg = qmsg->nextMsg;

    if (msgQueue->lastMsg == qmsg)
        msgQueue->lastMsg = qmsg->prevMsg;

    /* deallocate the memory for the message */
    HeapFree( SystemHeap, 0, qmsg );
    
    msgQueue->msgCount--;
    if (!msgQueue->msgCount) msgQueue->wakeBits &= ~QS_POSTMESSAGE;

    LeaveCriticalSection( &msgQueue->cSection );
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
    HQUEUE16     hQueue = 0;
    MESSAGEQUEUE *queue = NULL;

    if (hCursorQueue)
        hQueue = hCursorQueue;

    if( (message >= WM_KEYFIRST) && (message <= WM_KEYLAST) )
    {
       wakeBit = QS_KEY;
       if( hActiveQueue )
           hQueue = hActiveQueue;
    }
    else 
    {
       wakeBit = (message == WM_MOUSEMOVE) ? QS_MOUSEMOVE : QS_MOUSEBUTTON;
       if( (hwnd = GetCapture32()) )
	 if( (wndPtr = WIN_FindWndPtr( hwnd )) ) 
           {
               hQueue = wndPtr->hmemTaskQ;
           }
    }

    if( (hwnd = GetSysModalWindow16()) )
    {
      if( (wndPtr = WIN_FindWndPtr( hwnd )) )
            hQueue = wndPtr->hmemTaskQ;
    }

    if (hQueue)
        queue = QUEUE_Lock( hQueue );
    
    if( !queue ) 
    {
        queue = QUEUE_Lock( hFirstQueue );
      while( queue )
      {
        if (queue->wakeMask & wakeBit) break;
          
            QUEUE_Unlock(queue);
            queue = QUEUE_Lock( queue->next );
      }
      if( !queue )
      { 
        WARN(msg, "couldn't find queue\n"); 
        return; 
      }
    }

    QUEUE_SetWakeBit( queue, wakeBit );

    QUEUE_Unlock( queue );
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
    MSG32 *msg;
    QMSG  *qmsg = sysMsgQueue->lastMsg;
    int  mergeMsg = 0;

    if (!sysMsgQueue) return;

      /* Merge with previous event if possible */

    if ((message == WM_MOUSEMOVE) && sysMsgQueue->lastMsg)
    {
        msg = &(sysMsgQueue->lastMsg->msg);
        
	if ((msg->message == message) && (msg->wParam == wParam))
        {
            /* Merge events */
            qmsg = sysMsgQueue->lastMsg;
            mergeMsg = 1;
    }
    }

    if (!mergeMsg)
    {
        /* Should I limit the number of message in
          the system message queue??? */

        /* Don't merge allocate a new msg in the global heap */
        
        if (!(qmsg = (QMSG *) HeapAlloc( SystemHeap, 0, sizeof(QMSG) ) ))
        return;
        
        /* put message at the end of the linked list */
        qmsg->nextMsg = 0;
        qmsg->prevMsg = sysMsgQueue->lastMsg;

        if (sysMsgQueue->lastMsg)
            sysMsgQueue->lastMsg->nextMsg = qmsg;

        /* set last and first anchor index in system message queue */
        sysMsgQueue->lastMsg = qmsg;
        if (!sysMsgQueue->firstMsg)
            sysMsgQueue->firstMsg = qmsg;
        
        sysMsgQueue->msgCount++;
    }

      /* Store message */
    msg = &(qmsg->msg);
    msg->hwnd    = 0;
    msg->message = message;
    msg->wParam  = wParam;
    msg->lParam  = lParam;
    msg->time    = time;
    msg->pt.x    = xPos;
    msg->pt.y    = yPos;
    qmsg->extraInfo = extraInfo;

    QUEUE_WakeSomeone( message );
}

		    
/***********************************************************************
 *	     QUEUE_GetQueueTask
 */
HTASK16 QUEUE_GetQueueTask( HQUEUE16 hQueue )
{
    HTASK16 hTask = 0;
    
    MESSAGEQUEUE *queue = QUEUE_Lock( hQueue );

    if (queue)
{
        hTask = queue->thdb->process->task;
        QUEUE_Unlock( queue );
}

    return hTask;
}



/***********************************************************************
 *           QUEUE_IncPaintCount
 */
void QUEUE_IncPaintCount( HQUEUE16 hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)QUEUE_Lock( hQueue ))) return;
    queue->wPaintCount++;
    QUEUE_SetWakeBit( queue, QS_PAINT );
    QUEUE_Unlock( queue );
}


/***********************************************************************
 *           QUEUE_DecPaintCount
 */
void QUEUE_DecPaintCount( HQUEUE16 hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)QUEUE_Lock( hQueue ))) return;
    queue->wPaintCount--;
    if (!queue->wPaintCount) queue->wakeBits &= ~QS_PAINT;
    QUEUE_Unlock( queue );
}


/***********************************************************************
 *           QUEUE_IncTimerCount
 */
void QUEUE_IncTimerCount( HQUEUE16 hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)QUEUE_Lock( hQueue ))) return;
    queue->wTimerCount++;
    QUEUE_SetWakeBit( queue, QS_TIMER );
    QUEUE_Unlock( queue );
}


/***********************************************************************
 *           QUEUE_DecTimerCount
 */
void QUEUE_DecTimerCount( HQUEUE16 hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)QUEUE_Lock( hQueue ))) return;
    queue->wTimerCount--;
    if (!queue->wTimerCount) queue->wakeBits &= ~QS_TIMER;
    QUEUE_Unlock( queue );
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

    if (!(queue = (MESSAGEQUEUE *)QUEUE_Lock( GetFastQueue() ))) return;
    queue->wPostQMsg = TRUE;
    queue->wExitCode = (WORD)exitCode;
    QUEUE_Unlock( queue );
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
    /* now obsolete the message queue will be expanded dynamically
     as necessary */

    /* access the queue to create it if it's not existing */
    GetFastQueue();

    return TRUE;
}

/***********************************************************************
 *           InitThreadInput   (USER.409)
 */
HQUEUE16 WINAPI InitThreadInput( WORD unknown, WORD flags )
{
    HQUEUE16 hQueue;
    MESSAGEQUEUE *queuePtr;

    THDB *thdb = THREAD_Current();

    if (!thdb)
        return 0;

    hQueue = thdb->teb.queue;
    
    if ( !hQueue )
    {
        /* Create thread message queue */
        if( !(hQueue = QUEUE_CreateMsgQueue( TRUE )))
        {
            WARN(msg, "failed!\n");
            return FALSE;
    }
        
        /* Link new queue into list */
        queuePtr = (MESSAGEQUEUE *)QUEUE_Lock( hQueue );
        queuePtr->thdb = THREAD_Current();

        SYSTEM_LOCK();
        SetThreadQueue( 0, hQueue );
        thdb->teb.queue = hQueue;
            
        queuePtr->next  = hFirstQueue;
        hFirstQueue = hQueue;
        SYSTEM_UNLOCK();
        
        QUEUE_Unlock( queuePtr );
    }

    return hQueue;
}

/***********************************************************************
 *           GetQueueStatus16   (USER.334)
 */
DWORD WINAPI GetQueueStatus16( UINT16 flags )
{
    MESSAGEQUEUE *queue;
    DWORD ret;

    if (!(queue = (MESSAGEQUEUE *)QUEUE_Lock( GetFastQueue() ))) return 0;
    ret = MAKELONG( queue->changeBits, queue->wakeBits );
    queue->changeBits = 0;
    QUEUE_Unlock( queue );
    
    return ret & MAKELONG( flags, flags );
}

/***********************************************************************
 *           GetQueueStatus32   (USER32.283)
 */
DWORD WINAPI GetQueueStatus32( UINT32 flags )
{
    MESSAGEQUEUE *queue;
    DWORD ret;

    if (!(queue = (MESSAGEQUEUE *)QUEUE_Lock( GetFastQueue() ))) return 0;
    ret = MAKELONG( queue->changeBits, queue->wakeBits );
    queue->changeBits = 0;
    QUEUE_Unlock( queue );
    
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
    BOOL32 ret;

    if (!(queue = (MESSAGEQUEUE *)QUEUE_Lock( GetFastQueue() )))
        return FALSE;
    ret = queue->wakeBits & (QS_KEY | QS_MOUSEBUTTON);
    QUEUE_Unlock( queue );

    return ret;
}

/***********************************************************************
 *           UserYield  (USER.332)
 */
void WINAPI UserYield(void)
{
    TDB *pCurTask = (TDB *)GlobalLock16( GetCurrentTask() );
    MESSAGEQUEUE *queue = (MESSAGEQUEUE *)QUEUE_Lock( pCurTask->hQueue );

    if ( !THREAD_IsWin16( THREAD_Current() ) )
    {
        FIXME(task, "called for Win32 thread (%04x)!\n", THREAD_Current()->teb_sel);
        QUEUE_Unlock( queue );
        return;
    }

    /* Handle sent messages */
    while (queue && (queue->wakeBits & QS_SENDMESSAGE))
        QUEUE_ReceiveMessage( queue );

    QUEUE_Unlock( queue );
    
    OldYield();

    queue = (MESSAGEQUEUE *)QUEUE_Lock( pCurTask->hQueue );
    while (queue && (queue->wakeBits & QS_SENDMESSAGE))
        QUEUE_ReceiveMessage( queue );

    QUEUE_Unlock( queue );
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
    DWORD ret;

    if (!(queue = (MESSAGEQUEUE *)QUEUE_Lock( GetFastQueue() ))) return 0;
    ret = queue->GetMessagePosVal;
    QUEUE_Unlock( queue );

    return ret;
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
    LONG ret;

    if (!(queue = (MESSAGEQUEUE *)QUEUE_Lock( GetFastQueue() ))) return 0;
    ret = queue->GetMessageTimeVal;
    QUEUE_Unlock( queue );
    
    return ret;
}


/***********************************************************************
 *           GetMessageExtraInfo   (USER.288) (USER32.271)
 */
LONG WINAPI GetMessageExtraInfo(void)
{
    MESSAGEQUEUE *queue;
    LONG ret;

    if (!(queue = (MESSAGEQUEUE *)QUEUE_Lock( GetFastQueue() ))) return 0;
    ret = queue->GetMessageExtraInfoVal;
    QUEUE_Unlock( queue );

    return ret;
}
