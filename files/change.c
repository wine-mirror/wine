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
 *		FindFirstChangeNotification32A (KERNEL32.248)
 */
HANDLE WINAPI FindFirstChangeNotificationA( LPCSTR lpPathName,
                                                BOOL bWatchSubtree,
                                                DWORD dwNotifyFilter ) 
{
    struct create_change_notification_request req;
    struct create_change_notification_reply reply;

    req.subtree = bWatchSubtree;
    req.filter  = dwNotifyFilter;
    CLIENT_SendRequest( REQ_CREATE_CHANGE_NOTIFICATION, -1, 1, &req, sizeof(req) );
    CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL );
    return reply.handle;
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

