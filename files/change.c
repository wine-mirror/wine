/*
 * Win32 file change notification functions
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
#include "debug.h"


/****************************************************************************
 *		FindFirstChangeNotificationA (KERNEL32.248)
 */
HANDLE WINAPI FindFirstChangeNotificationA( LPCSTR lpPathName, BOOL bWatchSubtree,
                                            DWORD dwNotifyFilter ) 
{
    struct create_change_notification_request *req = get_req_buffer();

    req->subtree = bWatchSubtree;
    req->filter  = dwNotifyFilter;
    server_call( REQ_CREATE_CHANGE_NOTIFICATION );
    return req->handle;
}

/****************************************************************************
 *		FindFirstChangeNotification32W (KERNEL32.249)
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

