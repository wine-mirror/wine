/*
 * Win32 file change notification functions
 *
 * FIXME: this is VERY difficult to implement with proper Unix support
 * at the wineserver side.
 * (Unix doesn't really support this)
 * See http://x57.deja.com/getdoc.xp?AN=575483053 for possible solutions.
 *
 * Copyright 1998 Ulrich Weigand
 */

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "winbase.h"
#include "winerror.h"
#include "heap.h"
#include "server.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(file);

/****************************************************************************
 *		FindFirstChangeNotificationA (KERNEL32.248)
 */
HANDLE WINAPI FindFirstChangeNotificationA( LPCSTR lpPathName, BOOL bWatchSubtree,
                                            DWORD dwNotifyFilter ) 
{
    HANDLE ret = -1;

    FIXME("this is not supported yet (non-trivial).\n");

    SERVER_START_REQ
    {
        struct create_change_notification_request *req = server_alloc_req( sizeof(*req), 0 );
        req->subtree = bWatchSubtree;
        req->filter  = dwNotifyFilter;
        if (!server_call( REQ_CREATE_CHANGE_NOTIFICATION )) ret = req->handle;
    }
    SERVER_END_REQ;
    return ret;
}

/****************************************************************************
 *		FindFirstChangeNotificationW (KERNEL32.249)
 */
HANDLE WINAPI FindFirstChangeNotificationW( LPCWSTR lpPathName,
                                                BOOL bWatchSubtree,
                                                DWORD dwNotifyFilter) 
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, lpPathName );
    HANDLE ret = FindFirstChangeNotificationA( nameA, bWatchSubtree, 
                                                          dwNotifyFilter );
    if (nameA) HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}

/****************************************************************************
 *		FindNextChangeNotification (KERNEL32.252)
 */
BOOL WINAPI FindNextChangeNotification( HANDLE handle ) 
{
    /* FIXME: do something */
    return TRUE;
}

/****************************************************************************
 *		FindCloseChangeNotification (KERNEL32.247)
 */
BOOL WINAPI FindCloseChangeNotification( HANDLE handle) 
{
    return CloseHandle( handle );
}

