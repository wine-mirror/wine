/*
 * Message queues related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <string.h>
#include <signal.h>
#include <assert.h>
#include "windef.h"
#include "wingdi.h"
#include "winerror.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "queue.h"
#include "win.h"
#include "user.h"
#include "hook.h"
#include "thread.h"
#include "debugtools.h"
#include "server.h"
#include "spy.h"

DEFAULT_DEBUG_CHANNEL(msg);

#define MAX_QUEUE_SIZE   120  /* Max. size of a message queue */

static HQUEUE16 hExitingQueue = 0;
static PERQUEUEDATA *pQDataWin16 = NULL;  /* Global perQData for Win16 tasks */

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
    
    TRACE_(msg)("()\n");

    /* Share a single instance of perQData for all 16 bit tasks */
    if ( ( bIsWin16 = !(NtCurrentTeb()->tibflags & TEBF_WIN32) ) )
    {
        /* If previously allocated, just bump up ref count */
        if ( pQDataWin16 )
        {
            PERQDATA_Addref( pQDataWin16 );
            return pQDataWin16;
        }
    }

    /* Allocate PERQUEUEDATA from the system heap */
    if (!( pQData = (PERQUEUEDATA *) HeapAlloc( GetProcessHeap(), 0,
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
    /* FIXME: not all per queue data critical sections should be global */
    MakeCriticalSectionGlobal( &pQData->cSection );

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
    TRACE_(msg)("(): current refcount %lu ...\n", pQData->ulRefCount);

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
    TRACE_(msg)("(): current refcount %lu ...\n",
          (LONG)pQData->ulRefCount );

    EnterCriticalSection( &pQData->cSection );
    if ( --pQData->ulRefCount == 0 )
    {
        LeaveCriticalSection( &pQData->cSection );
        DeleteCriticalSection( &pQData->cSection );

        TRACE_(msg)("(): deleting PERQUEUEDATA instance ...\n" );

        /* Deleting our global 16 bit perQData? */
        if ( pQData == pQDataWin16 )
            pQDataWin16 = 0;
            
        /* Free the PERQUEUEDATA instance */
        HeapFree( GetProcessHeap(), 0, pQData );

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
HWND PERQDATA_GetFocusWnd( PERQUEUEDATA *pQData )
{
    HWND hWndFocus;
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
HWND PERQDATA_SetFocusWnd( PERQUEUEDATA *pQData, HWND hWndFocus )
{
    HWND hWndFocusPrv;
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
HWND PERQDATA_GetActiveWnd( PERQUEUEDATA *pQData )
{
    HWND hWndActive;
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
HWND PERQDATA_SetActiveWnd( PERQUEUEDATA *pQData, HWND hWndActive )
{
    HWND hWndActivePrv;
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
HWND PERQDATA_GetCaptureWnd( PERQUEUEDATA *pQData )
{
    HWND hWndCapture;
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
HWND PERQDATA_SetCaptureWnd( PERQUEUEDATA *pQData, HWND hWndCapture )
{
    HWND hWndCapturePrv;
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
 * Function for getting a 32 bit pointer on queue structure. For thread
 * safeness programmers should use this function instead of GlobalLock to
 * retrieve a pointer on the structure. QUEUE_Unlock should also be called
 * when access to the queue structure is not required anymore.
 */
MESSAGEQUEUE *QUEUE_Lock( HQUEUE16 hQueue )
{
    MESSAGEQUEUE *queue;

    HeapLock( GetProcessHeap() );  /* FIXME: a bit overkill */
    queue = GlobalLock16( hQueue );
    if ( !queue || (queue->magic != QUEUE_MAGIC) )
    {
        HeapUnlock( GetProcessHeap() );
        return NULL;
    }

    queue->lockCount++;
    HeapUnlock( GetProcessHeap() );
    return queue;
}


/***********************************************************************
 *	     QUEUE_Unlock
 *
 * Use with QUEUE_Lock to get a thread safe access to message queue
 * structure
 */
void QUEUE_Unlock( MESSAGEQUEUE *queue )
{
    if (queue)
    {
        HeapLock( GetProcessHeap() );  /* FIXME: a bit overkill */

        if ( --queue->lockCount == 0 )
        {
            DeleteCriticalSection ( &queue->cSection );
            if (queue->server_queue)
                CloseHandle( queue->server_queue );
            GlobalFree16( queue->self );
        }
    
        HeapUnlock( GetProcessHeap() );
    }
}


/***********************************************************************
 *	     QUEUE_DumpQueue
 */
void QUEUE_DumpQueue( HQUEUE16 hQueue )
{
    MESSAGEQUEUE *pq; 

    if (!(pq = QUEUE_Lock( hQueue )) )
    {
        WARN_(msg)("%04x is not a queue handle\n", hQueue );
        return;
    }

    EnterCriticalSection( &pq->cSection );

    DPRINTF( "thread: %10p  Intertask SendMessage:\n"
             "firstMsg: %8p   lastMsg:  %8p\n"
             "lockCount: %7.4x\n"
             "paints: %10.4x\n"
             "hCurHook: %8.4x\n",
             pq->teb, pq->firstMsg, pq->lastMsg,
             (unsigned)pq->lockCount, pq->wPaintCount,
             pq->hCurHook);

    LeaveCriticalSection( &pq->cSection );

    QUEUE_Unlock( pq );
}


/***********************************************************************
 *	     QUEUE_IsExitingQueue
 */
BOOL QUEUE_IsExitingQueue( HQUEUE16 hQueue )
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
    HANDLE handle;
    MESSAGEQUEUE * msgQueue;

    TRACE_(msg)("(): Creating message queue...\n");

    if (!(hQueue = GlobalAlloc16( GMEM_FIXED | GMEM_ZEROINIT,
                                  sizeof(MESSAGEQUEUE) )))
        return 0;

    msgQueue = (MESSAGEQUEUE *) GlobalLock16( hQueue );
    if ( !msgQueue )
        return 0;

    if (bCreatePerQData)
    {
        SERVER_START_REQ( get_msg_queue )
        {
            SERVER_CALL_ERR();
            handle = req->handle;
        }
        SERVER_END_REQ;
        if (!handle)
        {
            ERR_(msg)("Cannot get thread queue");
            GlobalFree16( hQueue );
            return 0;
        }
        msgQueue->server_queue = handle;
    }

    msgQueue->self = hQueue;

    InitializeCriticalSection( &msgQueue->cSection );
    MakeCriticalSectionGlobal( &msgQueue->cSection );

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
BOOL QUEUE_DeleteMsgQueue( HQUEUE16 hQueue )
{
    MESSAGEQUEUE * msgQueue = QUEUE_Lock(hQueue);

    TRACE_(msg)("(): Deleting message queue %04x\n", hQueue);

    if (!hQueue || !msgQueue)
    {
	ERR_(msg)("invalid argument.\n");
	return 0;
    }

    msgQueue->magic = 0;

    if( hActiveQueue == hQueue ) hActiveQueue = 0;

    HeapLock( GetProcessHeap() );  /* FIXME: a bit overkill */

    /* Release per queue data if present */
    if ( msgQueue->pQData )
    {
        PERQDATA_Release( msgQueue->pQData );
        msgQueue->pQData = 0;
    }

    msgQueue->self = 0;

    HeapUnlock( GetProcessHeap() );

    /* free up resource used by MESSAGEQUEUE structure */
    msgQueue->lockCount--;
    QUEUE_Unlock( msgQueue );
    
    return 1;
}


/***********************************************************************
 *           handle_sent_message
 *
 * Handle the reception of a sent message by calling the corresponding window proc
 */
static void handle_sent_message( QMSG *msg )
{
    LRESULT result = 0;
    MESSAGEQUEUE *queue = QUEUE_Lock( GetFastQueue16() );
    DWORD extraInfo = queue->GetMessageExtraInfoVal; /* save ExtraInfo */
    WND *wndPtr = WIN_FindWndPtr( msg->msg.hwnd );

    TRACE( "got hwnd %x msg %x (%s) wp %x lp %lx\n",
           msg->msg.hwnd, msg->msg.message, SPY_GetMsgName(msg->msg.message),
           msg->msg.wParam, msg->msg.lParam );

    queue->GetMessageExtraInfoVal = msg->extraInfo;

    /* call the right version of CallWindowProcXX */
    switch(msg->type)
    {
    case QMSG_WIN16:
        result = CallWindowProc16( (WNDPROC16)wndPtr->winproc,
                                   (HWND16) msg->msg.hwnd,
                                   (UINT16) msg->msg.message,
                                   LOWORD(msg->msg.wParam),
                                   msg->msg.lParam );
        break;
    case QMSG_WIN32A:
        result = CallWindowProcA( wndPtr->winproc, msg->msg.hwnd, msg->msg.message,
                                  msg->msg.wParam, msg->msg.lParam );
        break;
    case QMSG_WIN32W:
        result = CallWindowProcW( wndPtr->winproc, msg->msg.hwnd, msg->msg.message,
                                  msg->msg.wParam, msg->msg.lParam );
        break;
    }

    queue->GetMessageExtraInfoVal = extraInfo;  /* Restore extra info */
    WIN_ReleaseWndPtr(wndPtr);
    QUEUE_Unlock( queue );

    SERVER_START_REQ( reply_message )
    {
        req->result = result;
        req->remove = 1;
        SERVER_CALL();
    }
    SERVER_END_REQ;
}


/***********************************************************************
 *           process_sent_messages
 *
 * Process all pending sent messages
 */
static void process_sent_messages(void)
{
    QMSG msg;
    unsigned int res;

    for (;;)
    {
        SERVER_START_REQ( get_message )
        {
            req->flags = GET_MSG_REMOVE | GET_MSG_SENT_ONLY;
            req->get_win   = 0;
            req->get_first = 0;
            req->get_last  = ~0;
            if (!(res = SERVER_CALL()))
            {
                msg.type        = req->type;
                msg.msg.hwnd    = req->win;
                msg.msg.message = req->msg;
                msg.msg.wParam  = req->wparam;
                msg.msg.lParam  = req->lparam;
                msg.msg.time    = req->time;
                msg.msg.pt.x    = req->x;
                msg.msg.pt.y    = req->y;
                msg.extraInfo   = req->info;
            }
        }
        SERVER_END_REQ;

        if (res) break;
        handle_sent_message( &msg );
    }
}



/***********************************************************************
 *           QUEUE_SetWakeBit
 *
 * See "Windows Internals", p.449
 */
void QUEUE_SetWakeBit( MESSAGEQUEUE *queue, WORD set, WORD clear )
{
    TRACE_(msg)("queue = %04x, set = %04x, clear = %04x\n",
                queue->self, set, clear );
    if (!queue->server_queue) return;

    SERVER_START_REQ( set_queue_bits )
    {
        req->handle    = queue->server_queue;
        req->set       = set;
        req->clear     = clear;
        req->mask_cond = 0;
        SERVER_CALL();
    }
    SERVER_END_REQ;
}


/***********************************************************************
 *           QUEUE_ClearWakeBit
 */
void QUEUE_ClearWakeBit( MESSAGEQUEUE *queue, WORD bit )
{
    QUEUE_SetWakeBit( queue, 0, bit );
}


/***********************************************************************
 *           QUEUE_WaitBits
 *
 * See "Windows Internals", p.447
 *
 * return values:
 *    0 if exit with timeout
 *    1 otherwise
 */
int QUEUE_WaitBits( WORD bits, DWORD timeout )
{
    MESSAGEQUEUE *queue;
    HQUEUE16 hQueue;

    TRACE_(msg)("q %04x waiting for %04x\n", GetFastQueue16(), bits);

    hQueue = GetFastQueue16();
    if (!(queue = QUEUE_Lock( hQueue ))) return 0;
    
    for (;;)
    {
        unsigned int wake_bits = 0, changed_bits = 0;
        DWORD dwlc;

        SERVER_START_REQ( set_queue_mask )
        {
            req->wake_mask    = QS_SENDMESSAGE;
            req->changed_mask = bits | QS_SENDMESSAGE;
            req->skip_wait    = 1;
            if (!SERVER_CALL())
            {
                wake_bits    = req->wake_bits;
                changed_bits = req->changed_bits;
            }
        }
        SERVER_END_REQ;

        if (changed_bits & bits)
        {
            /* One of the bits is set; we can return */
            QUEUE_Unlock( queue );
            return 1;
        }
        if (wake_bits & QS_SENDMESSAGE)
        {
            /* Process the sent message immediately */
            process_sent_messages();
            continue;  /* nested sm crux */
        }

        TRACE_(msg)("(%04x) mask=%08x, bits=%08x, changed=%08x, waiting\n",
                    queue->self, bits, wake_bits, changed_bits );

        ReleaseThunkLock( &dwlc );
        if (dwlc) TRACE_(msg)("had win16 lock\n");

        if (USER_Driver.pMsgWaitForMultipleObjectsEx)
            USER_Driver.pMsgWaitForMultipleObjectsEx( 1, &queue->server_queue, timeout, 0, 0 );
        else
            WaitForSingleObject( queue->server_queue, timeout );
        if (dwlc) RestoreThunkLock( dwlc );
    }
}


/***********************************************************************
 *           QUEUE_FindMsg
 *
 * Find a message matching the given parameters. Return FALSE if none available.
 */
BOOL QUEUE_FindMsg( HWND hwnd, UINT first, UINT last, BOOL remove, QMSG *msg )
{
    BOOL ret = FALSE;

    if (!first && !last) last = ~0;

    for (;;)
    {
        SERVER_START_REQ( get_message )
        {
            req->flags     = remove ? GET_MSG_REMOVE : 0;
            req->get_win   = hwnd;
            req->get_first = first;
            req->get_last  = last;
            if ((ret = !SERVER_CALL()))
            {
                msg->kind        = req->kind;
                msg->type        = req->type;
                msg->msg.hwnd    = req->win;
                msg->msg.message = req->msg;
                msg->msg.wParam  = req->wparam;
                msg->msg.lParam  = req->lparam;
                msg->msg.time    = req->time;
                msg->msg.pt.x    = req->x;
                msg->msg.pt.y    = req->y;
                msg->extraInfo   = req->info;
            }
        }
        SERVER_END_REQ;

        if (!ret || (msg->kind != SEND_MESSAGE)) break;
        handle_sent_message( msg );
    }

    if (ret) TRACE( "got hwnd %x msg %x (%s) wp %x lp %lx\n",
                    msg->msg.hwnd, msg->msg.message, SPY_GetMsgName(msg->msg.message),
                    msg->msg.wParam, msg->msg.lParam );
    return ret;
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
    HeapFree( GetProcessHeap(), 0, qmsg );

    LeaveCriticalSection( &msgQueue->cSection );
}


/***********************************************************************
 *           QUEUE_CleanupWindow
 *
 * Cleanup the queue to account for a window being deleted.
 */
void QUEUE_CleanupWindow( HWND hwnd )
{
    SERVER_START_REQ( cleanup_window_queue )
    {
        req->win = hwnd;
        SERVER_CALL();
    }
    SERVER_END_REQ;
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
        hTask = queue->teb->htask16;
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

    if (!(queue = QUEUE_Lock( hQueue ))) return;
    EnterCriticalSection( &queue->cSection );
    queue->wPaintCount++;
    LeaveCriticalSection( &queue->cSection );
    QUEUE_SetWakeBit( queue, QS_PAINT, 0 );
    QUEUE_Unlock( queue );
}


/***********************************************************************
 *           QUEUE_DecPaintCount
 */
void QUEUE_DecPaintCount( HQUEUE16 hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = QUEUE_Lock( hQueue ))) return;
    EnterCriticalSection( &queue->cSection );
    queue->wPaintCount--;
    if (!queue->wPaintCount) QUEUE_ClearWakeBit( queue, QS_PAINT );
    LeaveCriticalSection( &queue->cSection );
    QUEUE_Unlock( queue );
}


/***********************************************************************
 *		PostQuitMessage (USER.6)
 */
void WINAPI PostQuitMessage16( INT16 exitCode )
{
    PostQuitMessage( exitCode );
}


/***********************************************************************
 *		PostQuitMessage (USER32.@)
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
void WINAPI PostQuitMessage( INT exitCode )
{
    PostThreadMessageW( GetCurrentThreadId(), WM_QUIT, exitCode, 0 );
}


/***********************************************************************
 *		GetWindowTask (USER.224)
 */
HTASK16 WINAPI GetWindowTask16( HWND16 hwnd )
{
    HTASK16 retvalue;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr) return 0;
    retvalue = QUEUE_GetQueueTask( wndPtr->hmemTaskQ );
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}

/***********************************************************************
 *		GetWindowThreadProcessId (USER32.@)
 */
DWORD WINAPI GetWindowThreadProcessId( HWND hwnd, LPDWORD process )
{
    DWORD retvalue;
    MESSAGEQUEUE *queue;

    WND *wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return 0;

    queue = QUEUE_Lock( wndPtr->hmemTaskQ );
    WIN_ReleaseWndPtr(wndPtr);

    if (!queue) return 0;

    if ( process ) *process = (DWORD)queue->teb->pid;
    retvalue = (DWORD)queue->teb->tid;

    QUEUE_Unlock( queue );
    return retvalue;
}


/***********************************************************************
 *		SetMessageQueue (USER.266)
 */
BOOL16 WINAPI SetMessageQueue16( INT16 size )
{
    return SetMessageQueue( size );
}


/***********************************************************************
 *		SetMessageQueue (USER32.@)
 */
BOOL WINAPI SetMessageQueue( INT size )
{
    /* now obsolete the message queue will be expanded dynamically
     as necessary */

    /* access the queue to create it if it's not existing */
    GetFastQueue16();

    return TRUE;
}

/***********************************************************************
 *		InitThreadInput (USER.409)
 */
HQUEUE16 WINAPI InitThreadInput16( WORD unknown, WORD flags )
{
    HQUEUE16 hQueue;
    MESSAGEQUEUE *queuePtr;

    TEB *teb = NtCurrentTeb();

    if (!teb)
        return 0;

    hQueue = teb->queue;
    
    if ( !hQueue )
    {
        /* Create thread message queue */
        if( !(hQueue = QUEUE_CreateMsgQueue( TRUE )))
        {
            ERR_(msg)("failed!\n");
            return FALSE;
	}
        
        /* Link new queue into list */
        queuePtr = QUEUE_Lock( hQueue );
        queuePtr->teb = NtCurrentTeb();

        HeapLock( GetProcessHeap() );  /* FIXME: a bit overkill */
        SetThreadQueue16( 0, hQueue );
        teb->queue = hQueue;
        HeapUnlock( GetProcessHeap() );
        
        QUEUE_Unlock( queuePtr );
    }

    return hQueue;
}

/***********************************************************************
 *		GetQueueStatus (USER.334)
 */
DWORD WINAPI GetQueueStatus16( UINT16 flags )
{
    return GetQueueStatus( flags );
}

/***********************************************************************
 *		GetQueueStatus (USER32.@)
 */
DWORD WINAPI GetQueueStatus( UINT flags )
{
    DWORD ret = 0;

    SERVER_START_REQ( get_queue_status )
    {
        req->clear = 1;
        SERVER_CALL();
        ret = MAKELONG( req->changed_bits & flags, req->wake_bits & flags );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *		GetInputState (USER.335)
 */
BOOL16 WINAPI GetInputState16(void)
{
    return GetInputState();
}

/***********************************************************************
 *		GetInputState   (USER32.@)
 */
BOOL WINAPI GetInputState(void)
{
    DWORD ret = 0;

    SERVER_START_REQ( get_queue_status )
    {
        req->clear = 0;
        SERVER_CALL();
        ret = req->wake_bits & (QS_KEY | QS_MOUSEBUTTON);
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *		WaitForInputIdle (USER32.@)
 */
DWORD WINAPI WaitForInputIdle (HANDLE hProcess, DWORD dwTimeOut)
{
    DWORD cur_time, ret;
    HANDLE idle_event = -1;

    SERVER_START_REQ( wait_input_idle )
    {
        req->handle = hProcess;
        req->timeout = dwTimeOut;
        if (!(ret = SERVER_CALL_ERR())) idle_event = req->event;
    }
    SERVER_END_REQ;
    if (ret) return 0xffffffff;  /* error */
    if (!idle_event) return 0;  /* no event to wait on */

    cur_time = GetTickCount();

    TRACE_(msg)("waiting for %x\n", idle_event );
    while ( dwTimeOut > GetTickCount() - cur_time || dwTimeOut == INFINITE ) 
    {
        ret = MsgWaitForMultipleObjects ( 1, &idle_event, FALSE, dwTimeOut, QS_SENDMESSAGE );
        if ( ret == ( WAIT_OBJECT_0 + 1 )) 
        {
            process_sent_messages();
            continue;
        }
        if ( ret == WAIT_TIMEOUT || ret == 0xFFFFFFFF ) 
        {
            TRACE_(msg)("timeout or error\n");
            return ret;
        }
        else 
        {
            TRACE_(msg)("finished\n");
            return 0;
        }
    }

    return WAIT_TIMEOUT;
}

/***********************************************************************
 *		UserYield (USER.332)
 *		UserYield16 (USER32.@)
 */
void WINAPI UserYield16(void)
{
    /* Handle sent messages */
    process_sent_messages();

    /* Yield */
    OldYield16();

    /* Handle sent messages again */
    process_sent_messages();
}

/***********************************************************************
 *		GetMessagePos (USER.119) (USER32.@)
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

    if (!(queue = QUEUE_Lock( GetFastQueue16() ))) return 0;
    ret = queue->GetMessagePosVal;
    QUEUE_Unlock( queue );

    return ret;
}


/***********************************************************************
 *		GetMessageTime (USER.120) (USER32.@)
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

    if (!(queue = QUEUE_Lock( GetFastQueue16() ))) return 0;
    ret = queue->GetMessageTimeVal;
    QUEUE_Unlock( queue );
    
    return ret;
}


/***********************************************************************
 *		GetMessageExtraInfo (USER.288) (USER32.@)
 */
LONG WINAPI GetMessageExtraInfo(void)
{
    MESSAGEQUEUE *queue;
    LONG ret;

    if (!(queue = QUEUE_Lock( GetFastQueue16() ))) return 0;
    ret = queue->GetMessageExtraInfoVal;
    QUEUE_Unlock( queue );

    return ret;
}


/**********************************************************************
 *		AttachThreadInput (USER32.@) Attaches input of 1 thread to other
 *
 * Attaches the input processing mechanism of one thread to that of
 * another thread.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 *
 * TODO:
 *    1. Reset the Key State (currenly per thread key state is not maintained)
 */
BOOL WINAPI AttachThreadInput( 
    DWORD idAttach,   /* [in] Thread to attach */
    DWORD idAttachTo, /* [in] Thread to attach to */
    BOOL fAttach)   /* [in] Attach or detach */
{
    MESSAGEQUEUE *pSrcMsgQ = 0, *pTgtMsgQ = 0;
    BOOL16 bRet = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    /* A thread cannot attach to itself */
    if ( idAttach == idAttachTo )
        goto CLEANUP;

    /* According to the docs this method should fail if a
     * "Journal record" hook is installed. (attaches all input queues together)
     */
    if ( HOOK_IsHooked( WH_JOURNALRECORD ) )
        goto CLEANUP;
        
    /* Retrieve message queues corresponding to the thread id's */
    pTgtMsgQ = QUEUE_Lock( GetThreadQueue16( idAttach ) );
    pSrcMsgQ = QUEUE_Lock( GetThreadQueue16( idAttachTo ) );

    /* Ensure we have message queues and that Src and Tgt threads
     * are not system threads.
     */
    if ( !pSrcMsgQ || !pTgtMsgQ || !pSrcMsgQ->pQData || !pTgtMsgQ->pQData )
        goto CLEANUP;

    if (fAttach)   /* Attach threads */
    {
        /* Only attach if currently detached  */
        if ( pTgtMsgQ->pQData != pSrcMsgQ->pQData )
        {
            /* First release the target threads perQData */
            PERQDATA_Release( pTgtMsgQ->pQData );
        
            /* Share a reference to the source threads perQDATA */
            PERQDATA_Addref( pSrcMsgQ->pQData );
            pTgtMsgQ->pQData = pSrcMsgQ->pQData;
        }
    }
    else    /* Detach threads */
    {
        /* Only detach if currently attached */
        if ( pTgtMsgQ->pQData == pSrcMsgQ->pQData )
        {
            /* First release the target threads perQData */
            PERQDATA_Release( pTgtMsgQ->pQData );
        
            /* Give the target thread its own private perQDATA once more */
            pTgtMsgQ->pQData = PERQDATA_CreateInstance();
        }
    }

    /* TODO: Reset the Key State */

    bRet = 1;      /* Success */
    
CLEANUP:

    /* Unlock the queues before returning */
    if ( pSrcMsgQ )
        QUEUE_Unlock( pSrcMsgQ );
    if ( pTgtMsgQ )
        QUEUE_Unlock( pTgtMsgQ );
    
    return bRet;
}
