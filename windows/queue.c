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


static HQUEUE16 hExitingQueue = 0;
static PERQUEUEDATA *pQDataWin16 = NULL;  /* Global perQData for Win16 tasks */

HQUEUE16 hActiveQueue = 0;


/***********************************************************************
 *           PERQDATA_Addref
 *
 * Increment reference count for the PERQUEUEDATA instance
 * Returns reference count for debugging purposes
 */
static void PERQDATA_Addref( PERQUEUEDATA *pQData )
{
    assert(pQData != 0 );
    TRACE_(msg)("(): current refcount %lu ...\n", pQData->ulRefCount);

    InterlockedIncrement( &pQData->ulRefCount );
}


/***********************************************************************
 *           PERQDATA_Release
 *
 * Release a reference to a PERQUEUEDATA instance.
 * Destroy the instance if no more references exist
 * Returns reference count for debugging purposes
 */
static void PERQDATA_Release( PERQUEUEDATA *pQData )
{
    assert(pQData != 0 );
    TRACE_(msg)("(): current refcount %lu ...\n",
          (LONG)pQData->ulRefCount );

    if (!InterlockedDecrement( &pQData->ulRefCount ))
    {
        DeleteCriticalSection( &pQData->cSection );

        TRACE_(msg)("(): deleting PERQUEUEDATA instance ...\n" );

        /* Deleting our global 16 bit perQData? */
        if ( pQData == pQDataWin16 ) pQDataWin16 = 0;

        /* Free the PERQUEUEDATA instance */
        HeapFree( GetProcessHeap(), 0, pQData );
    }
}


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
static PERQUEUEDATA * PERQDATA_CreateInstance(void)
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
HWND PERQDATA_GetCaptureWnd( INT *hittest )
{
    MESSAGEQUEUE *queue;
    PERQUEUEDATA *pQData;
    HWND hWndCapture;

    if (!(queue = QUEUE_Lock( GetFastQueue16() ))) return 0;
    pQData = queue->pQData;

    EnterCriticalSection( &pQData->cSection );
    hWndCapture = pQData->hWndCapture;
    *hittest = pQData->nCaptureHT;
    LeaveCriticalSection( &pQData->cSection );

    QUEUE_Unlock( queue );
    return hWndCapture;
}


/***********************************************************************
 *           PERQDATA_SetCaptureWnd
 *
 * Set the capture hwnd member in a threadsafe manner
 */
HWND PERQDATA_SetCaptureWnd( HWND hWndCapture, INT hittest )
{
    MESSAGEQUEUE *queue;
    PERQUEUEDATA *pQData;
    HWND hWndCapturePrv;

    if (!(queue = QUEUE_Lock( GetFastQueue16() ))) return 0;
    pQData = queue->pQData;

    EnterCriticalSection( &pQData->cSection );
    hWndCapturePrv = pQData->hWndCapture;
    pQData->hWndCapture = hWndCapture;
    pQData->nCaptureHT = hittest;
    LeaveCriticalSection( &pQData->cSection );

    QUEUE_Unlock( queue );
    return hWndCapturePrv;
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
            if (queue->server_queue)
                CloseHandle( queue->server_queue );
            GlobalFree16( queue->self );
        }
    
        HeapUnlock( GetProcessHeap() );
    }
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
    if (hExitingQueue == hQueue) hExitingQueue = 0;

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
 *		GetMessagePos (USER.119)
 *		GetMessagePos (USER32.@)
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
 *		GetMessageTime (USER.120)
 *		GetMessageTime (USER32.@)
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
 *		GetMessageExtraInfo (USER.288)
 *		GetMessageExtraInfo (USER32.@)
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
