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
#include "process.h"
#include "thread.h"
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
    struct create_change_notification_request *req = get_req_buffer();

    FIXME("this is not supported yet (non-trivial).\n");
    req->subtree = bWatchSubtree;
    req->filter  = dwNotifyFilter;
    server_call( REQ_CREATE_CHANGE_NOTIFICATION );
    return req->handle;
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

