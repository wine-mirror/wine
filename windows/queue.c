/*
 * Message queues related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include "wine/debug.h"
#include "wine/server.h"
#include "spy.h"

WINE_DEFAULT_DEBUG_CHANNEL(msg);


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

    if (!(queue = QUEUE_Current())) return 0;
    pQData = queue->pQData;

    EnterCriticalSection( &pQData->cSection );
    hWndCapture = pQData->hWndCapture;
    *hittest = pQData->nCaptureHT;
    LeaveCriticalSection( &pQData->cSection );
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

    if (!(queue = QUEUE_Current())) return 0;
    pQData = queue->pQData;

    EnterCriticalSection( &pQData->cSection );
    hWndCapturePrv = pQData->hWndCapture;
    pQData->hWndCapture = hWndCapture;
    pQData->nCaptureHT = hittest;
    LeaveCriticalSection( &pQData->cSection );
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
 *	     QUEUE_Current
 *
 * Get the current thread queue, creating it if required.
 * QUEUE_Unlock is not needed since the queue can only be deleted by
 * the current thread anyway.
 */
MESSAGEQUEUE *QUEUE_Current(void)
{
    MESSAGEQUEUE *queue;
    HQUEUE16 hQueue;

    if (!(hQueue = GetThreadQueue16(0)))
    {
        if (!(hQueue = InitThreadInput16( 0, 0 ))) return NULL;
    }

    if ((queue = GlobalLock16( hQueue )))
    {
        if (queue->magic != QUEUE_MAGIC) queue = NULL;
    }
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
            wine_server_call_err( req );
            handle = reply->handle;
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
void QUEUE_DeleteMsgQueue(void)
{
    HQUEUE16 hQueue = GetThreadQueue16(0);
    MESSAGEQUEUE * msgQueue;

    if (!hQueue) return;  /* thread doesn't have a queue */

    TRACE("(): Deleting message queue %04x\n", hQueue);

    if (!(msgQueue = QUEUE_Lock(hQueue)))
    {
        ERR("invalid thread queue\n");
        return;
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
    SetThreadQueue16( 0, 0 );

    /* free up resource used by MESSAGEQUEUE structure */
    msgQueue->lockCount--;
    QUEUE_Unlock( msgQueue );
}


/***********************************************************************
 *		GetWindowTask (USER.224)
 */
HTASK16 WINAPI GetWindowTask16( HWND16 hwnd )
{
    HTASK16 retvalue;
    MESSAGEQUEUE *queue;

    WND *wndPtr = WIN_FindWndPtr16( hwnd );
    if (!wndPtr) return 0;

    queue = QUEUE_Lock( wndPtr->hmemTaskQ );
    WIN_ReleaseWndPtr(wndPtr);

    if (!queue) return 0;

    retvalue = queue->teb->htask16;
    QUEUE_Unlock( queue );

    return retvalue;
}

/***********************************************************************
 *		InitThreadInput   (USER.409)
 */
HQUEUE16 WINAPI InitThreadInput16( WORD unknown, WORD flags )
{
    MESSAGEQUEUE *queuePtr;
    HQUEUE16 hQueue = NtCurrentTeb()->queue;

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
        NtCurrentTeb()->queue = hQueue;
        HeapUnlock( GetProcessHeap() );

        QUEUE_Unlock( queuePtr );
    }

    return hQueue;
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
        wine_server_call( req );
        ret = MAKELONG( reply->changed_bits & flags, reply->wake_bits & flags );
    }
    SERVER_END_REQ;
    return ret;
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
        wine_server_call( req );
        ret = reply->wake_bits & (QS_KEY | QS_MOUSEBUTTON);
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

    if (!(queue = QUEUE_Current())) return 0;
    return queue->GetMessagePosVal;
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

    if (!(queue = QUEUE_Current())) return 0;
    return queue->GetMessageTimeVal;
}


/***********************************************************************
 *		GetMessageExtraInfo (USER.288)
 *		GetMessageExtraInfo (USER32.@)
 */
LONG WINAPI GetMessageExtraInfo(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = QUEUE_Current())) return 0;
    return queue->GetMessageExtraInfoVal;
}
