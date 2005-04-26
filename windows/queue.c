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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "message.h"
#include "win.h"
#include "user_private.h"
#include "thread.h"
#include "wine/debug.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(msg);


/***********************************************************************
 *           QUEUE_CreateMsgQueue
 *
 * Creates a message queue. Doesn't link it into queue list!
 */
static HQUEUE16 QUEUE_CreateMsgQueue(void)
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

    SERVER_START_REQ( get_msg_queue )
    {
        wine_server_call_err( req );
        handle = reply->handle;
    }
    SERVER_END_REQ;
    if (!handle)
    {
        ERR_(msg)("Cannot get thread queue\n");
        GlobalFree16( hQueue );
        return 0;
    }
    msgQueue->server_queue = handle;
    msgQueue->self = hQueue;
    return hQueue;
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
    HQUEUE16 hQueue = NtCurrentTeb()->queue;

    if (!hQueue)
    {
        if (!(hQueue = QUEUE_CreateMsgQueue())) return NULL;
        SetThreadQueue16( 0, hQueue );
    }

    return GlobalLock16( hQueue );
}



/***********************************************************************
 *	     QUEUE_DeleteMsgQueue
 *
 * Delete a message queue.
 */
void QUEUE_DeleteMsgQueue(void)
{
    HQUEUE16 hQueue = NtCurrentTeb()->queue;
    MESSAGEQUEUE * msgQueue;

    if (!hQueue) return;  /* thread doesn't have a queue */

    TRACE("(): Deleting message queue %04x\n", hQueue);

    if (!(msgQueue = GlobalLock16( hQueue )))
    {
        ERR("invalid thread queue\n");
        return;
    }

    SetThreadQueue16( 0, 0 );
    CloseHandle( msgQueue->server_queue );
    GlobalFree16( hQueue );
}


/***********************************************************************
 *		InitThreadInput   (USER.409)
 */
HQUEUE16 WINAPI InitThreadInput16( WORD unknown, WORD flags )
{
    MESSAGEQUEUE *queue = QUEUE_Current();
    return queue ? queue->self : 0;
}
